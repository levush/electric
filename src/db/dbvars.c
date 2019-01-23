/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dbvars.c
 * Database object variables module
 * Written by: Steven M. Rubin, Static Free Software
 *
 * Copyright (c) 2000 Static Free Software.
 *
 * Electric(tm) is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Electric(tm) is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Electric(tm); see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, Mass 02111-1307, USA.
 *
 * Static Free Software
 * 4119 Alpine Road
 * Portola Valley, California 94028
 * info@staticfreesoft.com
 */

#include "global.h"
#include "database.h"
#include "egraphics.h"
#include "edialogs.h"
#include "usr.h"
#include "drc.h"
#include "erc.h"
#include "rout.h"
#include "network.h"
#include "logeffort.h"
#include "sim.h"
#include "simirsim.h"
#include "eio.h"
#include "tecart.h"
#include "tecmocmos.h"
#include "tecmocmossub.h"
#include "tecschem.h"

/***** option saving structures *****/

/* this define should match the one in "usrdiacom.c" */
#define SAVEDBITWORDS 2

typedef struct
{
	BOOLEAN   dirty;
	INTBIG    saveflag;
	INTBIG   *addr;
	INTBIG    type;
	INTBIG    partial;
	CHAR     *variablename;
	UINTBIG   initialbits1, initialbits2;
	UINTBIG   maskbits1, maskbits2;
	CHAR     *meaning;
} OPTIONVARPROTO;

typedef struct
{
	UINTBIG         bits[SAVEDBITWORDS];
	CHAR           *command;
	OPTIONVARPROTO *variables;
} OPTIONVARCOMS;

typedef struct
{
	INTBIG    addr;
	INTBIG    type;
	CHAR     *variablename;
	INTBIG    oldtype;
} OPTIONVARCACHE;

static OPTIONVARCACHE *db_optionvarcache;
static INTBIG          db_optionvarcachecount = 0;

/***** for parameterized cells *****/

#define NOPARCELL      ((PARCELL *)-1)

typedef struct Iparcell
{
	NODEPROTO       *cell;
	CHAR            *parname;
	CHAR            *shortname;
	struct Iparcell *nextparcell;
} PARCELL;

static PARCELL *db_parcellfree = NOPARCELL;
static PARCELL *db_firstparcell = NOPARCELL;

       BOOLEAN db_onanobject = FALSE;	/* TRUE if code evaluation is "on an object" (getval, etc.) */
       INTBIG  db_onobjectaddr;			/* the address of the first object that we are on */
       INTBIG  db_onobjecttype;			/* the type of the first object that we are on */
	   WINDOWPART *db_onobjectwin;		/* the window of the first object that we are on */
       INTBIG  db_lastonobjectaddr;		/* the address of the last object that we were on */
       INTBIG  db_lastonobjecttype;		/* the type of the last object that we were on */

/***** for finding variables on objects *****/

#define SPECNAME  0		/* look for special names in structure */
#define VARNAME   1		/* look for variable names in structure */
#define PORTCNAME 2		/* look for port names on nodeproto */
#define ARCINAME  3		/* look for nodeinst/arcinst names in cell */
#define NODEINAME 4		/* look for nodeinst/arcinst names in cell */
#define PORTANAME 5		/* look for port arc names on nodeinst */
#define PORTENAME 6		/* look for export names on nodeinst */
#define ARCNAME   7		/* look for arcproto names in technology */
#define NODENAME  8		/* look for nodeproto names in technology */
#define NULLNAME  9		/* no more names in structure */

typedef struct
{
	CHAR   *name;
	UINTBIG type;
	INTBIG *ptr;
} NAMEVARS;

#define MAXAPACK	5				/* maximum nested attribute searches */
static struct attrsearch
{
	INTSML      *numvar;			/* address of object variable count */
	VARIABLE   **firstvar;			/* address of object variable list */
	NAMEVARS    *vars;				/* special names in this object */
	INTBIG       proto;				/* address of object prototype */
	INTBIG       object;			/* address of this object */
	INTBIG       state;				/* current name type under test */
	INTBIG       varindex;			/* current index for variable search */
	INTBIG       specindex;			/* current index for name search */
	NODEPROTO   *nodeprotoname;		/* current cell object */
	ARCPROTO    *arcprotoname;		/* current arcproto object */
	PORTARCINST *portarcinstname;	/* current portarcinst object */
	PORTEXPINST *portexpinstname;	/* current portexpinst object */
	PORTPROTO   *portprotoname;		/* current portproto object */
	ARCINST     *arciname;			/* current arcinst object */
	NODEINST    *nodeiname;			/* current nodeinst object */
} db_attrpacket[MAXAPACK];
static INTBIG db_apackindex = 0;
static struct attrsearch *db_thisapack;
static INTBIG db_realaddress;

/* prototypes for local routines */
static PARCELL  *db_allocparcell(void);
static INTBIG    db_assignvalue(void *loc, INTBIG datasize);
static INTBIG    db_binarysearch(INTSML, VARIABLE*, INTBIG);
static BOOLEAN   db_delindvar(INTBIG, INTBIG, VARIABLE*, INTBIG);
static CHAR     *db_describetype(INTBIG type);
static void      db_freeparcell(PARCELL *pc);
static INTBIG    db_getdatasize(INTBIG);
static BOOLEAN   db_getindvar(VARIABLE*, INTBIG, INTBIG*);
static INTBIG    db_getkey(CHAR*);
static void      db_initarcinstlist(ARCINST*);
static void      db_initarcprotolist(ARCPROTO*);
static void      db_initconstraintlist(CONSTRAINT*);
static void      db_initgeomlist(GEOM*);
static void      db_initgraphicslist(GRAPHICS*);
static void      db_initliblist(LIBRARY*);
static void      db_initnetworklist(NETWORK*);
static void      db_initnodeinstlist(NODEINST*);
static void      db_initnodeprotolist(NODEPROTO*);
static void      db_initpolygonlist(POLYGON*);
static void      db_initportarcinstlist(PORTARCINST*);
static void      db_initportexpinstlist(PORTEXPINST*);
static void      db_initportprotolist(PORTPROTO*);
static void      db_initrtnodelist(RTNODE*);
static void      db_inittechlist(TECHNOLOGY*);
static void      db_inittoollist(TOOL*);
static void      db_initviewlist(VIEW*);
static void      db_initwindowframelist(WINDOWFRAME*);
static void      db_initwindowlist(WINDOWPART*);
static BOOLEAN   db_insindvar(INTBIG, INTBIG, VARIABLE*, INTBIG, INTBIG);
#ifdef DEBUGMEMORY
  static BOOLEAN db_setindvar(INTBIG, INTBIG, VARIABLE*, INTBIG, INTBIG, CHAR*, INTBIG);
  static BOOLEAN db_setval(UCHAR1*, UCHAR1*, INTBIG, CLUSTER*, CHAR*, INTBIG);
  static BOOLEAN db_setvalkey(INTSML*, VARIABLE**, INTBIG, INTBIG, INTBIG, INTBIG, CLUSTER*, CHAR*, INTBIG);
  static BOOLEAN db_setvalvar(VARIABLE*, INTBIG, INTBIG, CLUSTER*, CHAR*, INTBIG);
#else
  static BOOLEAN db_setindvar(INTBIG, INTBIG, VARIABLE*, INTBIG, INTBIG);
  static BOOLEAN db_setval(UCHAR1*, UCHAR1*, INTBIG, CLUSTER*);
  static BOOLEAN db_setvalkey(INTSML*, VARIABLE**, INTBIG, INTBIG, INTBIG, INTBIG, CLUSTER*);
  static BOOLEAN db_setvalvar(VARIABLE*, INTBIG, INTBIG, CLUSTER*);
#endif
static void      db_makefontassociationdescript(UINTBIG *descript, INTBIG *fontusage, INTBIG numfonts);
static void      db_makefontassociationvar(INTSML numvar, VARIABLE *firstvar, INTBIG *fontusage, INTBIG numfonts);
static void      db_makefontassociations(LIBRARY *lib);
static void      db_renamevar(INTSML, VARIABLE*, INTBIG, INTBIG);
static CLUSTER  *db_whichcluster(INTBIG, INTBIG);

/*
 * Routine to free all memory associated with this module.
 */
void db_freevariablememory(void)
{
	REGISTER INTBIG i;
	REGISTER PARCELL *pc;

	for(i=0; i<el_numnames; i++) efree((CHAR *)el_namespace[i]);
	if (el_numnames != 0) efree((CHAR *)el_namespace);
	if (db_optionvarcachecount > 0) efree((CHAR *)db_optionvarcache);

	while (db_firstparcell != NOPARCELL)
	{
		pc = db_firstparcell;
		db_firstparcell = pc->nextparcell;
		efree((CHAR *)pc->parname);
		efree((CHAR *)pc->shortname);
		db_freeparcell(pc);
	}
	while (db_parcellfree != NOPARCELL)
	{
		pc = db_parcellfree;
		db_parcellfree = pc->nextparcell;
		efree((CHAR *)pc);
	}
}

/************************* ACCESSING VARIABLES *************************/

/*
 * routine to find the variable with name "name", which must be of type
 * "want", on the object whose address is "addr" and type is "type".  If
 * "want" is -1 then any type will do.  If the variable does not exist or
 * is the wrong type, NOVARIABLE is returned.  If the variable is code, it
 * will be evaluated and a pseudo-variable will be returned that holds the
 * value.
 */
VARIABLE *getval(INTBIG addr, INTBIG type, INTBIG want, CHAR *name)
{
	VARIABLE *var;
	REGISTER VARIABLE *retvar;
	REGISTER CHAR *pp;
	REGISTER INTBIG search;
	REGISTER BOOLEAN oldonobjectstate;

	/* accumulate the object that the "getval" is on (should use mutex) */
	oldonobjectstate = db_onanobject;
	if (!db_onanobject)
	{
		db_onanobject = TRUE;
		db_onobjectaddr = addr;
		db_onobjecttype = type;
	}
	db_lastonobjectaddr = addr;
	db_lastonobjecttype = type;

	retvar = NOVARIABLE;
	search = initobjlist(addr, type, FALSE);
	if (search != 0)
	{
		for(;;)
		{
			pp = nextobjectlist(&var, search);
			if (pp == 0) break;
			if (namesame(pp, name) == 0)
			{
				/* variable found: make sure it is right type */
				if (want == -1 || ((INTBIG)(var->type & (VTYPE|VISARRAY))) == want)
				{
					retvar = evalvar(var, addr, type);
					if (retvar != var && retvar != NOVARIABLE)
					{
						/* check the type again */
						if (want != -1 && ((INTBIG)(retvar->type & (VTYPE|VISARRAY))) != want)
							retvar = NOVARIABLE;
					}
				}
				break;
			}
		}
	}

	/* complete the accumulation of the "getval" object (should use mutex) */
	db_onanobject = oldonobjectstate;

	/* return the result */
	return(retvar);
}

/*
 * routine to find the variable with name "name", which must be of type
 * "want", on the object whose address is "addr" and type is "type".  If
 * "want" is -1 then any type will do.  If the variable does not exist or
 * is the wrong type, NOVARIABLE is returned.  If the variable is code, it
 * will not be evaluated.
 */
VARIABLE *getvalnoeval(INTBIG addr, INTBIG type, INTBIG want, CHAR *name)
{
	VARIABLE *var;
	REGISTER VARIABLE *retvar;
	REGISTER CHAR *pp;
	REGISTER INTBIG search;
	REGISTER BOOLEAN oldonobjectstate;

	/* accumulate the object that the "getval" is on (should use mutex) */
	oldonobjectstate = db_onanobject;
	if (!db_onanobject)
	{
		db_onanobject = TRUE;
		db_onobjectaddr = addr;
		db_onobjecttype = type;
	}
	db_lastonobjectaddr = addr;
	db_lastonobjecttype = type;

	retvar = NOVARIABLE;
	search = initobjlist(addr, type, FALSE);
	if (search != 0)
	{
		for(;;)
		{
			pp = nextobjectlist(&var, search);
			if (pp == 0) break;
			if (namesame(pp, name) == 0)
			{
				/* variable found: make sure it is right type */
				if (want == -1 || ((INTBIG)(var->type & (VTYPE|VISARRAY))) == want)
					retvar = var;
				break;
			}
		}
	}

	/* complete the accumulation of the "getval" object (should use mutex) */
	db_onanobject = oldonobjectstate;

	/* return the result */
	return(retvar);
}

/*
 * routine to find the variable with key "key", which must be of type "want",
 * on the object whose address is "addr" and type is "type".  If the
 * variable does not exist or is the wrong type, NOVARIABLE is returned.
 * If the variable is code, it will be evaluated and a pseudo-variable
 * will be returned that holds the value.
 */
VARIABLE *getvalkey(INTBIG addr, INTBIG type, INTBIG want, INTBIG key)
{
	REGISTER INTBIG med;
	INTSML *mynumvar;
	REGISTER VARIABLE *var, *retvar;
	VARIABLE **myfirstvar;
	REGISTER BOOLEAN oldonobjectstate;

	/* accumulate the object that the "getval" is on (should use mutex) */
	oldonobjectstate = db_onanobject;
	if (!db_onanobject)
	{
		db_onanobject = TRUE;
		db_onobjectaddr = addr;
		db_onobjecttype = type;
	}
	db_lastonobjectaddr = addr;
	db_lastonobjecttype = type;

	/* get the firstvar/numvar into the globals */
	retvar = NOVARIABLE;
	if (!db_getvarptr(addr, type, &myfirstvar, &mynumvar))
	{
		/* binary search variable space for the key */
		med = db_binarysearch(*mynumvar, *myfirstvar, key);
		if (med >= 0)
		{
			/* get the variable */
			var = &(*myfirstvar)[med];

			/* make sure it is right type if a type was specified */
			if (want == -1 || ((INTBIG)(var->type & (VTYPE|VISARRAY))) == want)
			{
				retvar = evalvar(var, addr, type);
				if (retvar != var && retvar != NOVARIABLE)
				{
					/* check the type again */
					if (want != -1 && ((INTBIG)(retvar->type & (VTYPE|VISARRAY))) != want)
						retvar = NOVARIABLE;
				}
			}
		}
	}

	/* complete the accumulation of the "getval" object (should use mutex) */
	db_onanobject = oldonobjectstate;

	/* return the result */
	return(retvar);
}

/*
 * routine to find the variable with key "key", which must be of type "want",
 * on the object whose address is "addr" and type is "type".  If the
 * variable does not exist or is the wrong type, NOVARIABLE is returned.
 * If the variable is code, it will not be evaluated.
 */
VARIABLE *getvalkeynoeval(INTBIG addr, INTBIG type, INTBIG want, INTBIG key)
{
	REGISTER INTBIG med;
	INTSML *mynumvar;
	REGISTER VARIABLE *var, *retvar;
	VARIABLE **myfirstvar;
	REGISTER BOOLEAN oldonobjectstate;

	/* accumulate the object that the "getval" is on (should use mutex) */
	oldonobjectstate = db_onanobject;
	if (!db_onanobject)
	{
		db_onanobject = TRUE;
		db_onobjectaddr = addr;
		db_onobjecttype = type;
	}
	db_lastonobjectaddr = addr;
	db_lastonobjecttype = type;

	/* get the firstvar/numvar into the globals */
	retvar = NOVARIABLE;
	if (!db_getvarptr(addr, type, &myfirstvar, &mynumvar))
	{
		/* binary search variable space for the key */
		med = db_binarysearch(*mynumvar, *myfirstvar, key);
		if (med >= 0)
		{
			/* get the variable */
			var = &(*myfirstvar)[med];

			/* make sure it is right type if a type was specified */
			if (want == -1 || ((INTBIG)(var->type & (VTYPE|VISARRAY))) == want)
				retvar = var;
		}
	}

	/* complete the accumulation of the "getval" object (should use mutex) */
	db_onanobject = oldonobjectstate;

	/* return the result */
	return(retvar);
}

/*
 * Routine to set the current window to use when examining the value of variables.
 * Returns the previous value of the default window.
 */
WINDOWPART *setvariablewindow(WINDOWPART *win)
{
	REGISTER WINDOWPART *lastwin;

	lastwin = db_onobjectwin;
	db_onobjectwin = win;
	return(lastwin);
}

/*
 * Routine to return the address and type of the current object that is being querried with
 * "getval()", "getvalkey()", "getvalnoeval()", or "getvalkeynoeval()".
 * Returns TRUE if there is a valid object, FALSE if not.
 */
BOOLEAN getcurrentvariableenvironment(INTBIG *addr, INTBIG *type, WINDOWPART **win)
{
	if (db_onanobject)
	{
		*addr = db_onobjectaddr;
		*type = db_onobjecttype;
		*win = db_onobjectwin;
		return(db_onanobject);
	}
	return(FALSE);
}

/*
 * Routine to return the last address and type of the object from which a "getval()",
 * "getvalkey()", "getvalnoeval()", or "getvalkeynoeval()" was called.
 * Returns TRUE if there is a valid object, FALSE if not.
 */
BOOLEAN getlastvariableobject(INTBIG *addr, INTBIG *type)
{
	if (db_onanobject)
	{
		*addr = db_lastonobjectaddr;
		*type = db_lastonobjecttype;
		return(db_onanobject);
	}
	return(FALSE);
}		

#define NUMEVALS 100

/*
 * Routine to return a VARIABLE that can be used as a placeholder for a real one.
 * This is needed when a VARIABLE is code (a dummy one must be created to hold the
 * evaluation).  It is also needed when generating VARIABLEs for objects that are part
 * of the fixed C structures.
 */
VARIABLE *db_dummyvariable(void)
{
	REGISTER VARIABLE *var;
	static INTBIG evalnum = 0;
	static VARIABLE retvar[NUMEVALS];

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	/* prepare to evaluate the variable */
	evalnum++;
	if (evalnum >= NUMEVALS) evalnum = 0;
	var = &retvar[evalnum];
	return(var);
}

/*
 * routine to evaluate variable "var" and replace it with its true value if it is code
 */
VARIABLE *evalvar(VARIABLE *var, INTBIG addr, INTBIG type)
{
	REGISTER INTBIG language;
	REGISTER VARIABLE *thevar;

	/* if this isn't code or even a variable, just return the input */
	if (var == NOVARIABLE) return(var);
	language = var->type & (VCODE1|VCODE2);
	if (language == 0) return(var);

	/* set the environment */
	if (addr == 0) db_onanobject = FALSE; else
	{
		db_onanobject = TRUE;
		db_lastonobjectaddr = db_onobjectaddr = addr;
		db_lastonobjecttype = db_onobjecttype = type;
	}

	/* evaluate the variable */
	thevar = doquerry((CHAR *)var->addr, language, var->type & ~(VCODE1|VCODE2));

	/* remove the environment */
	db_onanobject = FALSE;

	if (thevar == NOVARIABLE) return(var);
	thevar->key = var->key;
	TDCOPY(thevar->textdescript, var->textdescript);
	return(thevar);
}

/*
 * routine to get an entry in the array variable "name" in object "addr"
 * which is of type "type".  Entry "aindex" is placed into "value".  Returns
 * true on error.
 */
BOOLEAN getind(INTBIG addr, INTBIG type, CHAR *name, INTBIG aindex, INTBIG *value)
{
	VARIABLE *var;
	REGISTER CHAR *pp;
	REGISTER INTBIG search;

	search = initobjlist(addr, type, FALSE);
	if (search == 0) return(TRUE);
	for(;;)
	{
		pp = nextobjectlist(&var, search);
		if (pp == 0) break;
		if (namesame(pp, name) == 0) break;
	}

	/* variable must exist */
	if (pp == 0) return(TRUE);

	if ((var->type&VCANTSET) != 0) return(TRUE);

	return(db_getindvar(var, aindex, value));
}

/*
 * routine to get an entry in the array variable whose key is "key" in object
 * "addr" which is of type "type".  Entry "aindex" is placed in "value".
 * The routine returns true upon error.
 */
BOOLEAN getindkey(INTBIG addr, INTBIG type, INTBIG key, INTBIG aindex, INTBIG *value)
{
	VARIABLE **myfirstvar;
	INTSML *mynumvar;
	REGISTER INTBIG med;

	/* get the attribute list into the globals */
	if (db_getvarptr(addr, type, &myfirstvar, &mynumvar)) return(TRUE);

	/* binary search variable space for the key */
	med = db_binarysearch(*mynumvar, *myfirstvar, key);

	/* variable must exist */
	if (med < 0) return(TRUE);

	if (((*myfirstvar)[med].type&VCANTSET) != 0) return(TRUE);

	/* get the variable */
	return(db_getindvar(&(*myfirstvar)[med], aindex, value));
}

BOOLEAN db_parentvaldefaulted = FALSE;

/*
 * Routine to examine the hierarchy and find the variable "name".
 * Looks as many as "height" levels up the hierarchy (arbitrary if zero).
 * Returns the variable if found.
 */
VARIABLE *getparentval(CHAR *name, INTBIG height)
{
	REGISTER INTBIG key;

	key = makekey(name);
	return(getparentvalkey(key, height));
}

/*
 * Routine to examine the hierarchy and find the variable with key "key".
 * Looks as many as "height" levels up the hierarchy (arbitrary if zero).
 * Returns the variable if found.
 */
VARIABLE *getparentvalkey(INTBIG key, INTBIG height)
{
	NODEINST **localnilist;
	INTBIG localdepth, *localindexlist, oldclimb;
	REGISTER NODEINST *ni;
	REGISTER INTBIG i, climbed;
	REGISTER VARIABLE *var, *varne;

	/* search hierarchical path */
	gettraversalpath(el_curlib->curnodeproto, db_onobjectwin, &localnilist, &localindexlist, &localdepth, height);
	climbed = 0;
	oldclimb = getpopouthierarchy();

	for(i = localdepth - 1; i >= 0; i--)
	{
		ni = localnilist[i];

		/*
		 * because evaluation of this variable may involve examination of the
		 * hierarchy, the hierarchy stack must get shortened
		 */
		climbed++;
		popouthierarchy(oldclimb+climbed);

		/* look for the variable on the instance up the hierarchy */
		var = getvalkey((INTBIG)ni, VNODEINST, -1, key);

		/* restore hierarchy stack */
		popouthierarchy(oldclimb);

		/* return value if found */
		if (var != NOVARIABLE)
		{
			/* make sure the variable is of type "parameter" */
			if (TDGETISPARAM(var->textdescript) == 0)
			{
				ttyputmsg(_("Warning: Cell %s, node %s has attribute %s used as a parameter"),
					describenodeproto(ni->parent), describenodeinst(ni), truevariablename(var));

				/* repair the bit */
				popouthierarchy(climbed);
				varne = getvalkeynoeval((INTBIG)ni, VNODEINST, -1, key);
				popouthierarchy(0);
				if (varne != NOVARIABLE)
				{
					TDSETISPARAM(varne->textdescript, VTISPARAMETER);
					ni->parent->lib->userbits |= LIBCHANGEDMINOR;
				}
			}
			return(var);
		}

		/* stop climbing hierarchy if limited */
		if (height != 0)
		{
			height--;
			if (height <= 0) break;
		}
	}
	db_parentvaldefaulted = TRUE;
	return(NOVARIABLE);
}

/*
 * Routine to return TRUE if the any call to "getparentval()" or "getparentvalkey()" returned
 * a default value.  Resets the flag at each call.
 */
BOOLEAN parentvaldefaulted(void)
{
	REGISTER BOOLEAN retval;

	retval = db_parentvaldefaulted;
	db_parentvaldefaulted = FALSE;
	return(retval);
}

/*
 * routine to modify the text descriptor on variable "var", which resides on
 * object "addr" of type "type".  The new descriptor is set to "newdescript"
 */
void modifydescript(INTBIG addr, INTBIG type, VARIABLE *var, UINTBIG *newdescript)
{
	UINTBIG olddescript[TEXTDESCRIPTSIZE];

	if (var != NOVARIABLE)
	{
		TDCOPY(olddescript, var->textdescript);
		TDCOPY(var->textdescript, newdescript);
	}

	/* handle change control, constraint, and broadcast */
	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		/* tell constraint system about killed variable */
		(*el_curconstraint->modifydescript)(addr, type, var->key, olddescript);

		db_setchangecell(db_whichnodeproto(addr, type));

		/* record the change */
		(void)db_change(addr, DESCRIPTMOD, type, var->key, var->type, olddescript[0],
			olddescript[1], 0);
	}
	db_donextchangequietly = FALSE;

	/* mark a change to the database */
	db_changetimestamp++;
}

/*
 * routine to return the length of the array in variable "var".  If it is not
 * an array, the value -1 is returned.
 */
INTBIG getlength(VARIABLE *var)
{
	REGISTER INTBIG len;

	if ((var->type&VISARRAY) == 0) return(-1);
	len = (var->type&VLENGTH) >> VLENGTHSH;
	if (len != 0) return(len);

	if ((var->type&VTYPE) == VCHAR)
	{
		for(len=0; ((((CHAR *)var->addr)[len])&0377) != 0377; len++) ;
	} else if ((var->type&VTYPE) == VFLOAT)
	{
		for(len=0; ((float *)var->addr)[len] != -1; len++) ;
	} else if ((var->type&VTYPE) == VDOUBLE)
	{
		for(len=0; ((double *)var->addr)[len] != -1; len++) ;
	} else if ((var->type&VTYPE) == VSHORT)
	{
		for(len=0; ((INTSML *)var->addr)[len] != -1; len++) ;
	} else
	{
		for(len=0; ((INTBIG *)var->addr)[len] != -1; len++) ;
	}
	return(len);
}

/*
 * routine to convert a variable name in "name" to a key and return it.
 * If an error is detected, the routine returns -1.
 */
INTBIG makekey(CHAR *name)
{
	REGISTER CHAR **newname;
	REGISTER INTBIG med, i;

	/* search namespace for this name */
	med = db_getkey(name);
	if (med >= 0) return((INTBIG)el_namespace[med]);

	/* convert return value to proper index for new name */
	med = -med-1;

	/* make sure the name is valid */
	for(i=0; name[i] != 0; i++) if (name[i] == ' ' || name[i] == '\t')
		return(db_error(DBBADNAME|DBMAKEKEY));

	/* allocate space for new list */
	newname = (CHAR **)emalloc(((el_numnames+1)*(sizeof (CHAR *))), db_cluster);
	if (newname == 0) return(db_error(DBNOMEM|DBMAKEKEY));

	/* copy old list up to the new entry */
	for(i=0; i<med; i++) newname[i] = el_namespace[i];

	/* add the new entry */
	if (allocstring(&newname[med], name, db_cluster)) return(-1);

	/* copy old list after the new entry */
	for(i=med; i<el_numnames; i++) newname[i+1] = el_namespace[i];

	/* clean-up */
	if (el_numnames != 0) efree((CHAR *)el_namespace);
	el_namespace = newname;
	el_numnames++;
	return((INTBIG)el_namespace[med]);
}

/*
 * routine to convert a variable key in "key" to a name and return it.
 * Because of the techniques used, this is trivial.
 */
CHAR *makename(INTBIG key)
{
	return((CHAR *)key);
}

/*
 * routine to initialize a search of the variable names on object "addr"
 * of type "type".  Subsequent calls to "nextobjectlist" will return
 * variable names and fill the parameter with the variable addresses.
 * If "restrictdirect" is nonzero, only list those attributes directly on the
 * object (and not those in linked lists).  The routine returns zero upon
 * error, and otherwise returns a value that must be passed to the subsequent
 * "nextobjectlist" calls
 */
INTBIG initobjlist(INTBIG addr, INTBIG type, BOOLEAN restrictdirect)
{
	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	db_thisapack = &db_attrpacket[db_apackindex++];
	if (db_apackindex >= MAXAPACK) db_apackindex = 0;
	switch (type&VTYPE)
	{
		case VNODEINST:    db_initnodeinstlist((NODEINST *)addr);       break;
		case VNODEPROTO:   db_initnodeprotolist((NODEPROTO *)addr);     break;
		case VPORTARCINST: db_initportarcinstlist((PORTARCINST *)addr); break;
		case VPORTEXPINST: db_initportexpinstlist((PORTEXPINST *)addr); break;
		case VPORTPROTO:   db_initportprotolist((PORTPROTO *)addr);     break;
		case VARCINST:     db_initarcinstlist((ARCINST *)addr);         break;
		case VARCPROTO:    db_initarcprotolist((ARCPROTO *)addr);       break;
		case VGEOM:        db_initgeomlist((GEOM *)addr);               break;
		case VLIBRARY:     db_initliblist((LIBRARY *)addr);             break;
		case VTECHNOLOGY:  db_inittechlist((TECHNOLOGY *)addr);         break;
		case VTOOL:        db_inittoollist((TOOL *)addr);               break;
		case VRTNODE:      db_initrtnodelist((RTNODE *)addr);           break;
		case VNETWORK:     db_initnetworklist((NETWORK *)addr);         break;
		case VVIEW:        db_initviewlist((VIEW *)addr);               break;
		case VWINDOWPART:  db_initwindowlist((WINDOWPART *)addr);       break;
		case VGRAPHICS:    db_initgraphicslist((GRAPHICS *)addr);       break;
		case VCONSTRAINT:  db_initconstraintlist((CONSTRAINT *)addr);   break;
		case VWINDOWFRAME: db_initwindowframelist((WINDOWFRAME *)addr); break;
		case VPOLYGON:     db_initpolygonlist((POLYGON *)addr);         break;
		default:
			(void)db_error(DBBADOBJECT|DBINITOBJLIST);
			return(0);
	}
	if (restrictdirect)
		if (db_thisapack->state != NULLNAME) db_thisapack->state = SPECNAME;
	return((INTBIG)db_thisapack);
}

/*
 * routine to return the next name in the list (and the associated variable)
 */
CHAR *nextobjectlist(VARIABLE **actualvar, INTBIG ain)
{
	struct attrsearch *apack;
	REGISTER INTBIG datasize;
	REGISTER BOOLEAN indir;
	REGISTER CHAR *retval;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	static CHAR line[50];
	REGISTER VARIABLE *var, *nvar;

	apack = (struct attrsearch *)ain;
	for(;;) switch (apack->state)
	{
		case SPECNAME:	/* looking for special names in object structure */
			if (apack->vars[apack->specindex].name != 0)
			{
				/* get next special name */
				*actualvar = var = db_dummyvariable();
				var->key = apack->specindex;
				var->type = apack->vars[apack->specindex].type;
				TDCLEAR(var->textdescript);
				TDSETSIZE(var->textdescript, TXTSETQLAMBDA(4));
				var->addr = (INTBIG)apack->vars[apack->specindex].ptr - apack->proto + apack->object;
				db_realaddress = var->addr;
				indir = FALSE;
				if ((var->type&VISARRAY) == 0) indir = TRUE; else
					if ((var->type&VCREF) == 0) indir = TRUE;
				if (indir)
				{
					datasize = db_getdatasize(var->type);
					var->addr = db_assignvalue((void *)var->addr, datasize);
				}
				var->type |= VCREF;
				return(apack->vars[apack->specindex++].name);
			}
			/* no more special names, go to next case */
			apack->state = VARNAME;
			break;
		case VARNAME:	/* looking for variable names in object */
			if (apack->varindex < *apack->numvar)
			{
				/* get next variable name */
				*actualvar = &(*apack->firstvar)[apack->varindex];
				return((CHAR *)(*apack->firstvar)[apack->varindex++].key);
			}
			/* no more variable names, go to terminal case */
			apack->state = NULLNAME;
			break;
		case ARCINAME:	/* looking for arcs in the nodeproto */
			if (apack->arciname != NOARCINST)
			{
				/* get next arcinst name */
				ai = apack->arciname;
				apack->arciname = apack->arciname->nextarcinst;
				*actualvar = var = db_dummyvariable();
				var->key = -1;
				var->type = VARCINST;
				TDCLEAR(var->textdescript);
				TDSETSIZE(var->textdescript, TXTSETQLAMBDA(4));
				var->addr = (INTBIG)ai;
				nvar = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
				if (nvar != NOVARIABLE) return((CHAR *)nvar->addr);
				(void)esnprintf(line, 50, x_("arc%ld"), (INTBIG)ai);
				return(line);
			}
			apack->state = NODEINAME;
			break;
		case NODEINAME:	/* looking for nodes in the nodeproto */
			if (apack->nodeiname != NONODEINST)
			{
				/* get next nodeinst name */
				ni = apack->nodeiname;
				apack->nodeiname = apack->nodeiname->nextnodeinst;
				*actualvar = var = db_dummyvariable();
				var->key = -1;
				var->type = VNODEINST;
				TDCLEAR(var->textdescript);
				TDSETSIZE(var->textdescript, TXTSETQLAMBDA(4));
				var->addr = (INTBIG)ni;
				nvar = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
				if (nvar != NOVARIABLE) return((CHAR *)nvar->addr);
				(void)esnprintf(line, 50, x_("node%ld"), (INTBIG)ni);
				return(line);
			}
			/* no more nodeinst names, go to next case */
			apack->state = PORTCNAME;
			break;
		case PORTCNAME:	/* looking for port names on nodeproto */
			if (apack->portprotoname != NOPORTPROTO)
			{
				/* get next port name */
				*actualvar = var = db_dummyvariable();
				var->key = -1;
				TDCLEAR(var->textdescript);
				TDSETSIZE(var->textdescript, TXTSETQLAMBDA(4));
				var->type = VPORTPROTO;   var->addr = (INTBIG)apack->portprotoname;
				retval = apack->portprotoname->protoname;
				apack->portprotoname = apack->portprotoname->nextportproto;
				return(retval);
			}
			/* no more port names, go to next case */
			apack->state = SPECNAME;
			break;
		case PORTANAME:	/* looking for portarcinst names on nodeinst */
			if (apack->portarcinstname != NOPORTARCINST)
			{
				/* get next portarcinst name */
				*actualvar = var = db_dummyvariable();
				var->key = -1;
				var->type = VPORTARCINST;
				TDCLEAR(var->textdescript);
				TDSETSIZE(var->textdescript, TXTSETQLAMBDA(4));
				var->addr = (INTBIG)apack->portarcinstname;
				retval = apack->portarcinstname->proto->protoname;
				apack->portarcinstname = apack->portarcinstname->nextportarcinst;
				return(retval);
			}
			/* no more portarcinst names, go to next case */
			apack->state = PORTENAME;
			break;
		case PORTENAME:	/* looking for portexpinst names on nodeinst */
			if (apack->portexpinstname != NOPORTEXPINST)
			{
				/* get next portexpinst name */
				*actualvar = var = db_dummyvariable();
				var->key = -1;
				var->type = VPORTEXPINST;
				TDCLEAR(var->textdescript);
				TDSETSIZE(var->textdescript, TXTSETQLAMBDA(4));
				var->addr = (INTBIG)apack->portexpinstname;
				retval = apack->portexpinstname->proto->protoname;
				apack->portexpinstname = apack->portexpinstname->nextportexpinst;
				return(retval);
			}
			/* no more portexpinst names, go to next case */
			apack->state = SPECNAME;
			break;
		case ARCNAME:	/* looking for arcs in the technology */
			if (apack->arcprotoname != NOARCPROTO)
			{
				/* get next arcproto name */
				*actualvar = var = db_dummyvariable();
				var->key = -1;
				TDCLEAR(var->textdescript);
				TDSETSIZE(var->textdescript, TXTSETQLAMBDA(4));
				var->type = VARCPROTO;   var->addr = (INTBIG)apack->arcprotoname;
				retval = apack->arcprotoname->protoname;
				apack->arcprotoname = apack->arcprotoname->nextarcproto;
				return(retval);
			}
			/* no more arcproto names, go to next case */
			apack->state = NODENAME;
			break;
		case NODENAME:	/* looking for cells in the tech/lib */
			if (apack->nodeprotoname != NONODEPROTO)
			{
				/* get next cell name */
				*actualvar = var = db_dummyvariable();
				var->key = -1;
				TDCLEAR(var->textdescript);
				TDSETSIZE(var->textdescript, TXTSETQLAMBDA(4));
				var->type = VNODEPROTO;   var->addr = (INTBIG)apack->nodeprotoname;
				retval = describenodeproto(apack->nodeprotoname);
				apack->nodeprotoname = apack->nodeprotoname->nextnodeproto;
				return(retval);
			}
			/* no more nodeproto names, go to next case */
			apack->state = SPECNAME;
			break;
		case NULLNAME:
			*actualvar = NOVARIABLE;
			return(0);
	}
}

/********************** DELETING AND RENAMING VARIABLES *********************/

/*
 * delete the variable "name" from object whose address is "addr" and type
 * is "type".  Returns true if there is an error.
 */
BOOLEAN delval(INTBIG addr, INTBIG type, CHAR *name)
{
	REGISTER INTBIG key;

	key = makekey(name);
	if (key == -1)
	{
		db_donextchangequietly = FALSE;
		return(TRUE);
	}

	return(delvalkey(addr, type, key));
}

/*
 * delete the variable with the key "key" from object whose address is
 * "addr" and type is "type".  Returns true if there is an error.
 */
BOOLEAN delvalkey(INTBIG addr, INTBIG type, INTBIG key)
{
	REGISTER VARIABLE *var;
	REGISTER CHAR *varname;
	VARIABLE **myfirstvar;
	INTSML *mynumvar;
	REGISTER INTSML i, j;
	REGISTER INTBIG oldaddr, oldtype;
	UINTBIG olddescript[TEXTDESCRIPTSIZE];

	/* get the firstvar/numvar pointers */
	if (db_getvarptr(addr, type, &myfirstvar, &mynumvar))
	{
		db_donextchangequietly = FALSE;
		return(TRUE);
	}

	/* search for the variable in the list */
	for(i=0; i<*mynumvar; i++)
		if ((*myfirstvar)[i].key == key) break;
	if (i >= *mynumvar)
	{
		if (key != -1) varname = makename(key); else
			varname = _("FIXED VARIABLE");
		ttyputmsg(_("Internal error: tried to delete %s on object of type %s.  No worry."),
			varname, db_describetype(type));
		db_donextchangequietly = FALSE;
		(void)db_error(DBNOVAR|DBDELVALKEY);
		return(TRUE);
	}

	/* delete the variable */
	var = &(*myfirstvar)[i];
	oldaddr = var->addr;
	oldtype = var->type;
	TDCOPY(olddescript, var->textdescript);

	/* delete the entry in the variable list */
	(*mynumvar)--;
	for(j = i; j < *mynumvar; j++)
	{
		(*myfirstvar)[j].key  = (*myfirstvar)[j+1].key;
		(*myfirstvar)[j].type = (*myfirstvar)[j+1].type;
		TDCOPY((*myfirstvar)[j].textdescript, (*myfirstvar)[j+1].textdescript);
		(*myfirstvar)[j].addr = (*myfirstvar)[j+1].addr;
	}
	if (*mynumvar == 0) efree((CHAR *)*myfirstvar);

	/* handle change control, constraint, and broadcast */
	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		/* tell constraint system about killed variable */
		(*el_curconstraint->killvariable)(addr, type, key, oldaddr, oldtype, olddescript);

		/* record the change */
		(void)db_change((INTBIG)addr, VARIABLEKILL, type, key, oldaddr, oldtype, olddescript[0],
			olddescript[1]);
	} else
	{
		/* not going through change control, so delete the memory now */
		db_freevar(oldaddr, oldtype);
	}
	db_donextchangequietly = FALSE;

	/* mark a change to the database */
	if (type != VTOOL || addr != (INTBIG)us_tool) db_changetimestamp++;
	return(FALSE);
}

/*
 * routine to rename variable "oldname" to "newname" everywhere in the
 * database.
 * This routine does not delete the old name from the namespace but it does
 * rename the variable wherever it exists, so the old name is no longer
 * in use.  NOTE: this does not check variables on R-tree nodes.
 */
void renameval(CHAR *oldname, CHAR *newname)
{
	REGISTER INTBIG oldkey, newkey;
	REGISTER INTSML i;
	REGISTER NODEPROTO *np;
	REGISTER ARCPROTO *ap;
	REGISTER PORTPROTO *pp;
	REGISTER NETWORK *net;
	REGISTER VIEW *v;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER LIBRARY *lib;
	REGISTER TECHNOLOGY *tech;

	/* get key of old name */
	oldkey = db_getkey(oldname);
	if (oldkey < 0) return;
	oldkey = (INTBIG)el_namespace[oldkey];

	/* make sure new name is different */
	if (namesame(oldname, newname) == 0) return;

	/* make new name key */
	newkey = makekey(newname);
	if (newkey < 0) return;

	/* search the libraries */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		db_renamevar(lib->numvar, lib->firstvar, oldkey, newkey);
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			db_renamevar(np->numvar, np->firstvar, oldkey, newkey);
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			{
				db_renamevar(pp->numvar, pp->firstvar, oldkey, newkey);
				db_renamevar(pp->subportexpinst->numvar,
					pp->subportexpinst->firstvar, oldkey, newkey);
			}
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				db_renamevar(ni->numvar, ni->firstvar, oldkey, newkey);
				db_renamevar(ni->geom->numvar, ni->geom->firstvar, oldkey, newkey);
			}
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			{
				db_renamevar(ai->numvar, ai->firstvar, oldkey, newkey);
				db_renamevar(ai->geom->numvar, ai->geom->firstvar, oldkey, newkey);
				db_renamevar(ai->end[0].portarcinst->numvar,
					ai->end[0].portarcinst->firstvar, oldkey, newkey);
				db_renamevar(ai->end[1].portarcinst->numvar,
					ai->end[1].portarcinst->firstvar, oldkey, newkey);
			}
			for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
				db_renamevar(net->numvar, net->firstvar, oldkey, newkey);
		}
	}

	/* search the tools */
	for(i=0; i<el_maxtools; i++)
		db_renamevar(el_tools[i].numvar, el_tools[i].firstvar, oldkey, newkey);

	/* search the views */
	for(v = el_views; v != NOVIEW; v = v->nextview)
		db_renamevar(v->numvar, v->firstvar, oldkey, newkey);

	/* search the technologies */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		db_renamevar(tech->numvar, tech->firstvar, oldkey, newkey);
		for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			db_renamevar(ap->numvar, ap->firstvar, oldkey, newkey);
		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			db_renamevar(np->numvar, np->firstvar, oldkey, newkey);
	}

	/* mark a change to the database */
	db_changetimestamp++;
}

/************************* SETTING ENTIRE VARIABLES *************************/

/*
 * routine to set the value of entry "name" in object "addr" which is of
 * type "type".  The entry is set to "newaddr" which has type "newtype".
 * The routine returns the variable address (NOVARIABLE on error).
 */
#ifdef DEBUGMEMORY
VARIABLE *_setval(INTBIG addr, INTBIG type, CHAR *name, INTBIG newaddr, INTBIG newtype,
	CHAR *module, INTBIG line)
#else
VARIABLE *setval(INTBIG addr, INTBIG type, CHAR *name, INTBIG newaddr, INTBIG newtype)
#endif
{
	VARIABLE *var;
	REGISTER CHAR *pp;
	REGISTER INTBIG key, search;

	/* look for an attribute that already has this name */
	search = initobjlist(addr, type, FALSE);
	if (search == 0)
	{
		db_donextchangequietly = FALSE;
		return(NOVARIABLE);
	}
	for(;;)
	{
		pp = nextobjectlist(&var, search);
		if (pp == 0) break;
		if (namesame(pp, name) == 0) break;
	}

	/* if the attribute exists, change its value */
	if (pp != 0)
	{
		if ((var->type&VCANTSET) != 0)
		{
			db_donextchangequietly = FALSE;
			return((VARIABLE *)db_error(DBVARFIXED|DBSETVAL));
		}

		/* handle change control, constraint, and broadcast */
		if (!db_donextchangequietly && !db_dochangesquietly)
		{
			/* tell constraint system about killed variable */
			(*el_curconstraint->killvariable)(addr, type, var->key, var->addr, var->type, var->textdescript);

			/* record a change for removal of the old variable */
			(void)db_change((INTBIG)addr, VARIABLEKILL, type, var->key,
				var->addr, var->type, var->textdescript[0], var->textdescript[1]);
		}

#ifdef DEBUGMEMORY
		if (db_setvalvar(var, newaddr, newtype, db_whichcluster(addr, type), module, line))
#else
		if (db_setvalvar(var, newaddr, newtype, db_whichcluster(addr, type)))
#endif
		{
			db_donextchangequietly = FALSE;
			return((VARIABLE *)db_error(DBNOMEM|DBSETVAL));
		}

		/* handle change control, constraint, and broadcast */
		if (!db_donextchangequietly && !db_dochangesquietly)
		{
			/* tell constraint system about new variable */
			(*el_curconstraint->newvariable)(addr, type, var->key, var->type);

			(void)db_change((INTBIG)addr, VARIABLENEW, type, var->key, var->type, 0, 0, 0);
		}
		db_donextchangequietly = FALSE;

		/* if nodeproto name changed, rebuild hash table of nodeproto names */
		if (type == VNODEPROTO && namesame(name, x_("protoname")) == 0)
			db_buildnodeprotohashtable(((NODEPROTO *)addr)->lib);

		/* if portproto name changed, rebuild hash table of portproto names */
		if (type == VPORTPROTO && namesame(name, x_("protoname")) == 0)
			db_buildportprotohashtable(((PORTPROTO *)addr)->parent);

		/* mark a change to the database */
		if (type != VTOOL || addr != (INTBIG)us_tool) db_changetimestamp++;
		return(var);
	}

	/* create new variable: first ensure the name starts with a letter */
	if (!isalpha(*name))
	{
		db_donextchangequietly = FALSE;
		return((VARIABLE *)db_error(DBBADNAME|DBSETVAL));
	}

	/* get the key of the new name */
	key = makekey(name);
	if (key == -1)
	{
		db_donextchangequietly = FALSE;
		return(NOVARIABLE);
	}

	/* set the variable by its key */
#ifdef DEBUGMEMORY
	return(_setvalkey(addr, type, key, newaddr, newtype, module, line));
#else
	return(setvalkey(addr, type, key, newaddr, newtype));
#endif
}

/*
 * routine to set the variable on the object whose address is "addr", type
 * is "type", and key is "key".  The variable is set to "newaddr" with type
 * "newtype".  The routine returns the variable address (NOVARIABLE on error).
 */
#ifdef DEBUGMEMORY
VARIABLE *_setvalkey(INTBIG addr, INTBIG type, INTBIG key, INTBIG newaddr,
	INTBIG newtype, CHAR *module, INTBIG line)
#else
VARIABLE *setvalkey(INTBIG addr, INTBIG type, INTBIG key, INTBIG newaddr,
	INTBIG newtype)
#endif
{
	REGISTER INTBIG med;
	VARIABLE **myfirstvar;
	INTSML *mynumvar;

	/* get the attribute list pointers */
	if (db_getvarptr(addr, type, &myfirstvar, &mynumvar))
	{
		db_donextchangequietly = FALSE;
		return(NOVARIABLE);
	}

	/* binary search variable space for the key */
	med = db_binarysearch(*mynumvar, *myfirstvar, key);
	if (med >= 0)
	{
		if (((*myfirstvar)[med].type&VCANTSET) != 0)
		{
			db_donextchangequietly = FALSE;
			return((VARIABLE *)db_error(DBVARFIXED|DBSETVALKEY));
		}

		/* handle change control, constraint, and broadcast */
		if (!db_donextchangequietly && !db_dochangesquietly)
		{
			/* tell constraint system about killed variable */
			(*el_curconstraint->killvariable)(addr, type, key,
				(*myfirstvar)[med].addr, (*myfirstvar)[med].type, (*myfirstvar)[med].textdescript);

			/* record a change for removal of the old variable */
			(void)db_change((INTBIG)addr, VARIABLEKILL, type, key,
				(*myfirstvar)[med].addr, (*myfirstvar)[med].type, (*myfirstvar)[med].textdescript[0],
					(*myfirstvar)[med].textdescript[1]);
		}
	}

	/* set the new variable */
#ifdef DEBUGMEMORY
	if (db_setvalkey(mynumvar, myfirstvar, med, key, newaddr, newtype, db_whichcluster(addr, type),
		module, line))
#else
	if (db_setvalkey(mynumvar, myfirstvar, med, key, newaddr, newtype,
		db_whichcluster(addr, type)))
#endif
	{
		db_donextchangequietly = FALSE;
		return((VARIABLE *)db_error(DBNOMEM|DBSETVALKEY));
	}

	/* handle change control, constraint, and broadcast */
	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		/* tell constraint system about new variable */
		(*el_curconstraint->newvariable)(addr, type, key, newtype);

		/* record the change */
		(void)db_change((INTBIG)addr, VARIABLENEW, type, key, newtype, 0, 0, 0);
	}
	db_donextchangequietly = FALSE;
	if (med < 0) med = -1 - med;

	/* mark a change to the database */
	if (type != VTOOL || addr != (INTBIG)us_tool) db_changetimestamp++;
	return(&(*myfirstvar)[med]);
}

/*
 * routine to copy the variables from object "fromaddr" (which has type
 * "fromtype") to object "toaddr" (which has type "totype").  Returns true
 * on error.  If "uniquenames" is true, make sure nodes and arcs have unique names.
 */
BOOLEAN copyvars(INTBIG fromaddr, INTBIG fromtype, INTBIG toaddr, INTBIG totype, BOOLEAN uniquenames)
{
	REGISTER INTSML i;
	REGISTER INTBIG key, addr, type;
	INTBIG lx, hx, ly, hy;
	REGISTER CHAR *objname, *newname;
	REGISTER GEOM *geom;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	INTSML *numvar;
	VARIABLE **firstvar;
	REGISTER VARIABLE *var;

	if (db_getvarptr(fromaddr, fromtype, &firstvar, &numvar)) return(TRUE);

	for(i=0; i<(*numvar); i++)
	{
		key = (*firstvar)[i].key;
		addr = (*firstvar)[i].addr;
		type = (*firstvar)[i].type;
		if (uniquenames)
		{
			if (totype == VNODEINST && key == el_node_name_key)
			{
				/* if the node name wasn't displayable, do not copy */
				if ((type&VDISPLAY) == 0) continue;

				/* find a unique node name */
				ni = (NODEINST *)toaddr;
				objname = (CHAR *)addr;
				newname = us_uniqueobjectname(objname, ni->parent, VNODEINST, ni);
				if (namesame(newname, objname) != 0) addr = (INTBIG)newname;
			} else if (totype == VARCINST && key == el_arc_name_key)
			{
				/* if the arc name wasn't displayable, do not copy */
				if ((type&VDISPLAY) == 0) continue;

				/* find a unique node name */
				ai = (ARCINST *)toaddr;
				objname = (CHAR *)addr;
				newname = us_uniqueobjectname(objname, ai->parent, VARCINST, ai);
				if (namesame(newname, objname) != 0) addr = (INTBIG)newname;
			}
		}

		var = setvalkey(toaddr, totype, key, addr, type);
		if (var == NOVARIABLE) return(TRUE);
		TDCOPY(var->textdescript, (*firstvar)[i].textdescript);
	}

	/* variables may affect geometry size */
	if (totype == VNODEINST || totype == VARCINST)
	{
		if (totype == VNODEINST)
		{
			ni = (NODEINST *)toaddr;
			geom = ni->geom;
			np = ni->parent;
		} else
		{
			ai = (ARCINST *)toaddr;
			geom = ai->geom;
			np = ai->parent;
		}
		boundobj(geom, &lx, &hx, &ly, &hy);
		if (lx != geom->lowx || hx != geom->highx ||
			ly != geom->lowy || hy != geom->highy)
				updategeom(geom, np);
	}
	return(FALSE);
}

/*********************** SETTING ENTRIES IN VARIABLES ***********************/

/*
 * routine to set an entry in the array variable "name" in object "addr"
 * which is of type "type".  Entry "aindex" is set to "newaddr".
 * The routine returns true upon error.
 */
#ifdef DEBUGMEMORY
BOOLEAN _setind(INTBIG addr, INTBIG type, CHAR *name, INTBIG aindex, INTBIG newaddr,
	CHAR *module, INTBIG line)
#else
BOOLEAN setind(INTBIG addr, INTBIG type, CHAR *name, INTBIG aindex, INTBIG newaddr)
#endif
{
	VARIABLE *var;
	REGISTER CHAR *pp;
	REGISTER INTBIG search;
	REGISTER BOOLEAN retval;

	search = initobjlist(addr, type, FALSE);
	if (search == 0)
	{
		db_donextchangequietly = FALSE;
		return(TRUE);
	}
	for(;;)
	{
		pp = nextobjectlist(&var, search);
		if (pp == 0) break;
		if (namesame(pp, name) == 0) break;
	}

	/* variable must exist */
	if (pp == 0)
	{
		db_donextchangequietly = FALSE;
		(void)db_error(DBNOVAR|DBSETIND);
		return(TRUE);
	}

	if ((var->type&VCANTSET) != 0)
	{
		db_donextchangequietly = FALSE;
		(void)db_error(DBVARFIXED|DBSETIND);
		return(TRUE);
	}

#ifdef DEBUGMEMORY
	retval = db_setindvar(addr, type, var, aindex, newaddr, module, line);
#else
	retval = db_setindvar(addr, type, var, aindex, newaddr);
#endif
	db_donextchangequietly = FALSE;
	if (retval)
	{
		(void)db_error(DBNOMEM|DBSETIND);
		return(TRUE);
	}

	/* mark a change to the database */
	db_changetimestamp++;
	return(FALSE);
}

/*
 * routine to set an entry in the array variable whose key is "key" in object
 * "addr" which is of type "type".  Entry "aindex" is set to "newaddr".
 * The routine returns nonzero upon error.
 */
#ifdef DEBUGMEMORY
BOOLEAN _setindkey(INTBIG addr, INTBIG type, INTBIG key, INTBIG aindex, INTBIG newaddr,
	CHAR *module, INTBIG line)
#else
BOOLEAN setindkey(INTBIG addr, INTBIG type, INTBIG key, INTBIG aindex, INTBIG newaddr)
#endif
{
	VARIABLE **myfirstvar;
	INTSML *mynumvar;
	REGISTER INTBIG med;
	REGISTER BOOLEAN retval;

	/* get the attribute list into the globals */
	if (db_getvarptr(addr, type, &myfirstvar, &mynumvar))
	{
		db_donextchangequietly = FALSE;
		return(TRUE);
	}

	/* binary search variable space for the key */
	med = db_binarysearch(*mynumvar, *myfirstvar, key);

	/* variable must exist */
	if (med < 0)
	{
		db_donextchangequietly = FALSE;
		(void)db_error(DBNOVAR|DBSETINDKEY);
		return(TRUE);
	}

	if (((*myfirstvar)[med].type&VCANTSET) != 0)
	{
		db_donextchangequietly = FALSE;
		(void)db_error(DBVARFIXED|DBSETINDKEY);
		return(TRUE);
	}

	/* set the variable */
#ifdef DEBUGMEMORY
	retval = db_setindvar(addr, type, &(*myfirstvar)[med], aindex, newaddr, module, line);
#else
	retval = db_setindvar(addr, type, &(*myfirstvar)[med], aindex, newaddr);
#endif
	db_donextchangequietly = FALSE;
	if (retval != 0)
	{
		(void)db_error(DBNOMEM|DBSETINDKEY);
		return(TRUE);
	}

	/* mark a change to the database */
	db_changetimestamp++;
	return(FALSE);
}

/*
 * routine to insert an entry in the array variable "name" in object "addr"
 * which is of type "type".  Entry "aindex" is set to "newaddr" and all entries
 * equal to or greater than "aindex" are moved up (i.e. "newaddr" is inserted
 * before entry "aindex").  The routine returns true upon error.
 */
BOOLEAN insind(INTBIG addr, INTBIG type, CHAR *name, INTBIG aindex, INTBIG newaddr)
{
	VARIABLE *var;
	REGISTER CHAR *pp;
	REGISTER INTBIG search;
	REGISTER BOOLEAN retval;

	search = initobjlist(addr, type, FALSE);
	if (search == 0)
	{
		db_donextchangequietly = FALSE;
		return(TRUE);
	}
	for(;;)
	{
		pp = nextobjectlist(&var, search);
		if (pp == 0) break;
		if (namesame(pp, name) == 0) break;
	}

	/* variable must exist */
	if (pp == 0)
	{
		db_donextchangequietly = FALSE;
		(void)db_error(DBNOVAR|DBINSIND);
		return(TRUE);
	}

	/* cannot insert if variable is frozen or is in C data structures */
	if ((var->type&VCANTSET) != 0 || (var->type&VCREF) != 0)
	{
		db_donextchangequietly = FALSE;
		(void)db_error(DBVARFIXED|DBINSIND);
		return(TRUE);
	}

	retval = db_insindvar(addr, type, var, aindex, newaddr);
	db_donextchangequietly = FALSE;
	if (retval != 0)
	{
		(void)db_error(DBNOMEM|DBINSIND);
		return(TRUE);
	}

	/* mark a change to the database */
	db_changetimestamp++;
	return(FALSE);
}

/*
 * routine to insert an entry in the array variable whose key is "key" in object
 * "addr" which is of type "type".  Entry "aindex" is set to "newaddr" and all entries
 * equal to or greater than "aindex" are moved up (i.e. "newaddr" is inserted
 * before entry "aindex").  The routine returns true upon error.
 */
BOOLEAN insindkey(INTBIG addr, INTBIG type, INTBIG key, INTBIG aindex, INTBIG newaddr)
{
	VARIABLE **myfirstvar;
	INTSML *mynumvar;
	REGISTER INTBIG med;
	REGISTER BOOLEAN retval;

	/* get the attribute list into the globals */
	if (db_getvarptr(addr, type, &myfirstvar, &mynumvar))
	{
		db_donextchangequietly = FALSE;
		return(TRUE);
	}

	/* binary search variable space for the key */
	med = db_binarysearch(*mynumvar, *myfirstvar, key);

	/* variable must exist */
	if (med < 0)
	{
		db_donextchangequietly = FALSE;
		(void)db_error(DBNOVAR|DBINSINDKEY);
		return(TRUE);
	}

	/* cannot insert if variable is frozen or is in C data structures */
	if (((*myfirstvar)[med].type&VCANTSET) != 0 || ((*myfirstvar)[med].type&VCREF) != 0)
	{
		db_donextchangequietly = FALSE;
		(void)db_error(DBVARFIXED|DBINSINDKEY);
		return(TRUE);
	}

	/* set the variable */
	retval = db_insindvar(addr, type, &(*myfirstvar)[med], aindex, newaddr);
	db_donextchangequietly = FALSE;
	if (retval != 0)
	{
		(void)db_error(DBNOMEM|DBINSINDKEY);
		return(TRUE);
	}

	/* mark a change to the database */
	db_changetimestamp++;
	return(FALSE);
}

/*
 * routine to delete entry "aindex" in the array variable "name" in object "addr"
 * which is of type "type".  The routine returns true upon error.
 */
BOOLEAN delind(INTBIG addr, INTBIG type, CHAR *name, INTBIG aindex)
{
	VARIABLE *var;
	REGISTER CHAR *pp;
	REGISTER INTBIG search;
	REGISTER BOOLEAN retval;

	search = initobjlist(addr, type, FALSE);
	if (search == 0)
	{
		db_donextchangequietly = FALSE;
		return(TRUE);
	}
	for(;;)
	{
		pp = nextobjectlist(&var, search);
		if (pp == 0) break;
		if (namesame(pp, name) == 0) break;
	}

	/* variable must exist */
	if (pp == 0)
	{
		db_donextchangequietly = FALSE;
		(void)db_error(DBNOVAR|DBDELIND);
		return(TRUE);
	}

	if ((var->type&VCANTSET) != 0)
	{
		db_donextchangequietly = FALSE;
		(void)db_error(DBVARFIXED|DBDELIND);
		return(TRUE);
	}

	retval = db_delindvar(addr, type, var, aindex);
	db_donextchangequietly = FALSE;
	if (retval)
	{
		(void)db_error(DBNOMEM|DBDELIND);
		return(TRUE);
	}

	/* mark a change to the database */
	db_changetimestamp++;
	return(FALSE);
}

/*
 * routine to delete entry "aindex" in the array variable whose key is "key" in object
 * "addr" which is of type "type".  The routine returns true upon error.
 */
BOOLEAN delindkey(INTBIG addr, INTBIG type, INTBIG key, INTBIG aindex)
{
	VARIABLE **myfirstvar;
	INTSML *mynumvar;
	REGISTER INTBIG med;
	REGISTER BOOLEAN retval;

	/* get the attribute list into the globals */
	if (db_getvarptr(addr, type, &myfirstvar, &mynumvar))
	{
		db_donextchangequietly = FALSE;
		return(TRUE);
	}

	/* binary search variable space for the key */
	med = db_binarysearch(*mynumvar, *myfirstvar, key);

	/* variable must exist */
	if (med < 0)
	{
		db_donextchangequietly = FALSE;
		(void)db_error(DBNOVAR|DBDELINDKEY);
		return(TRUE);
	}

	if (((*myfirstvar)[med].type&VCANTSET) != 0)
	{
		db_donextchangequietly = FALSE;
		(void)db_error(DBVARFIXED|DBDELINDKEY);
		return(TRUE);
	}

	/* set the variable */
	retval = db_delindvar(addr, type, &(*myfirstvar)[med], aindex);
	db_donextchangequietly = FALSE;
	if (retval != 0)
	{
		(void)db_error(DBNOMEM|DBDELINDKEY);
		return(TRUE);
	}

	/* mark a change to the database */
	db_changetimestamp++;
	return(FALSE);
}

/************************* OPTION VARIABLES *************************/

/*
 * Many user-settable options are stored in variables.  When libraries are saved,
 * these variables are saved, unless they are marked "VDONTSAVE".  To handle this
 * situation properly, the library "options" is saved with all option variables
 * intact, but all other libraries are saved with the option variables removed.
 * The routines below "remove" the option variables (by marking them VDONTSAVE)
 * and restore them after the library is written.
 */

#if COMTOOL
  extern TOOL *com_tool;
#endif
#if DRCTOOL
  extern TOOL *dr_tool;
#endif
#if ERCTOOL
  extern TOOL *erc_tool;
#endif
#if LOGEFFTOOL
  extern TOOL *le_tool;
#endif
#if ROUTTOOL
  extern TOOL *ro_tool;
#endif
#if SCTOOL
  extern TOOL *sc_tool;
#endif
#if SIMTOOL
  extern TOOL *sim_tool;
#endif
#if VHDLTOOL
  extern TOOL *vhdl_tool;
#endif

static OPTIONVARPROTO db_ovpcopyright[] =			/* FILE: IO Options: Library Options */
{
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_copyright_file"), 0, 0, 0, 0, N_("Copyright file")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpforeignfilelibrary[] =	/* FILE: IO Options: Copyright Options */
{
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, BINOUTBACKUP, 0, N_("Library file backup")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, CHECKATWRITE, 0, N_("Check database after write")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpforeignfilecif[] =		/* FILE: IO Options: CIF Options */
{
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, CIFOUTEXACT, 0, N_("Output mimics display")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, CIFOUTMERGE, 0, N_("Output merges boxes")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, CIFINSQUARE, 0, N_("Input squares wires")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, CIFOUTNOTOPCALL, 0, N_("Output Instantiates top level")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, CIFOUTNORMALIZE, 0, N_("Normalize coordinates")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, 0, CIFRESHIGH, N_("Resolution check")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("IO_cif_layer_names"), 0, 0, 0, 0, N_("CIF layers")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("IO_cif_resolution"), 0, 0, 0, 0, N_("CIF resolution")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpforeignfilegds[] =		/* FILE: IO Options: GDS Options */
{
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, GDSINTEXT, 0, N_("Input includes text")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, GDSINEXPAND, 0, N_("Input expands cells")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, GDSINARRAYS, 0, N_("Input instantiates arrays")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, GDSOUTMERGE, 0, N_("Output merges boxes")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, GDSINIGNOREUKN, 0, N_("Input ignores unknown layers")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, 0, GDSOUTPINS, N_("Output generates pins at exports")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, 0, GDSOUTUC, N_("Output All Upper Case")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("IO_gds_layer_numbers"), 0, 0, 0, 0, N_("GDS layers")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("IO_gds_export_layer"), 0, 0, 0, 0, N_("Output export layer")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_curve_resolution"), 0, 0, 0, 0, N_("Maximum arc angle")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_curve_sag"), 0, 0, 0, 0, N_("Maximum arc sag")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpforeignfiledxf[] =		/* FILE: IO Options: DXF Options */
{
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, DXFFLATTENINPUT, 0, N_("Input flattens hierarchy")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, DXFALLLAYERS, 0, N_("Input reads all layers")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_dxf_units"), 0, 0, 0, 0, N_("DXF scale")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("IO_dxf_layer_names"), 0, 0, 0, 0, N_("DXF layers")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpforeignfileedif[] =		/* FILE: IO Options: EDIF Options */
{
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, EDIFSCHEMATIC, 0, N_("Use schematic view")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_edif_input_scale"), 0, 0, 0, 0, N_("Input scale")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
#ifdef FORCECADENCE
static OPTIONVARPROTO db_ovpforeignfileskill[] =	/* FILE: IO Options: SKILL Options */
{
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, SKILLNOHIER, 0, N_("Do not include subcells")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, 0, SKILLFLATHIER, N_("Flatten hierarchy")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("IO_skill_layer_names"), 0, 0, 0, 0, N_("SKILL layers")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
#endif
static OPTIONVARPROTO db_ovpforeignfilecdl[] =	/* FILE: IO Options: CDL Options */
{
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_cdl_library_name"), 0, 0, 0, 0, N_("Cadence Library Name")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_cdl_library_path"), 0, 0, 0, 0, N_("Cadence Library Path")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, 0, CDLNOBRACKETS, N_("Convert brackets")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpforeignfiledef[] =		/* FILE: IO Options: DEF Options */
{
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, DEFNOLOGICAL, 0, N_("Place logical interconnect")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, DEFNOPHYSICAL, 0, N_("Place physical interconnect")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpforeignfilesue[] =	/* FILE: IO Options: Sue Options */
{
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, 0, SUEUSE4PORTTRANS, N_("Make 4-port transistors")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};

static OPTIONVARPROTO db_ovpprint[] =				/* FILE: Print Options */
{
/*	{0, 0, 0,                  VNODEPROTO,  0, "IO_postscript_EPS_scale", 0, 0, 0, 0, N_("")},  *** cell specific ***/
/*	{0, 0, 0,                  VNODEPROTO,  0, "IO_postscript_filename", 0, 0, 0, 0, N_("")},  *** cell specific ***/
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, PLOTFOCUS|PLOTFOCUSDPY, 0, N_("Area to plot")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, PLOTDATES, 0, N_("Plot date in corner")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, EPSPSCRIPT, 0, N_("Encapsulated PostScript")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, HPGL2, 0, N_("Write HPGL/2")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, PSCOLOR1|PSCOLOR2, 0, N_("Color PostScript")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, PSPLOTTER, 0, N_("Printer/plotter")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, PSROTATE, 0, N_("Rotate plot 90 degrees")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_state"), 0, 0, 0, PSAUTOROTATE, N_("Auto-rotate plot to fit")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_postscript_width"), 0, 0, 0, 0, N_("Page width")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_postscript_height"), 0, 0, 0, 0, N_("Page height")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_postscript_margin"), 0, 0, 0, 0, N_("Page margin")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_print_resolution_scale"), 0, 0, 0, 0, N_("Print and Copy resolution factor")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_hpgl2_scale"), 0, 0, 0, 0, N_("HPGL/2 units per pixel")},
	{0, 0, (INTBIG*)&io_tool,  VTOOL,       0, x_("IO_default_printer"), 0, 0, 0, 0, N_("Default printer")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpnewnode[] =				/* EDIT: New Node Options */
{
/*	{0, 0, 0,                  VNODEPROTO,  0, "userbits", 0, 0, 0, 0, N_("")},  *** cell specific ***/
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, NOPRIMCHANGES, 0, N_("Disallow modification of locked primitives")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, NOMOVEAFTERDUP, 0, N_("Move after duplicate")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, DUPCOPIESPORTS, 0, N_("Duplicate/array/extract copies exports")},
	{0, 0, 0,                  VNODEPROTO,  0, x_("NODE_size_default"), 0, 0, 0, 0, N_("Size of new primitives")},
	{0, 0, 0,                  VNODEPROTO,  0, x_("USER_placement_angle"), 0, 0, 0, 0, N_("Rotation of new nodes")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_placement_angle"), 0, 0, 0, 0, N_("Rotation of new nodes")},
	{0, 0, (INTBIG*)&net_tool, VTOOL,       0, x_("NET_node_abbreviations"), 0, 0, 0, 0, N_("Primitive function abbreviations")},
	{0, 0, (INTBIG*)&net_tool, VTOOL,       0, x_("NET_node_abbreviation_length"), 0, 0, 0, 0, N_("Length of cell abbreviations")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpselect[] =				/* EDIT: Selection: Selection Options */
{
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, NOINSTANCESELECT, 0, N_("Easy selection of cell instances")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, NOTEXTSELECT, 0, N_("Easy selection of annotation text")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, CENTEREDPRIMITIVES, 0, N_("Center-based primitives")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, MUSTENCLOSEALL, 0, N_("Dragging must enclose entire object")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpcell[] =				/* CELLS: Cell Options */
{
/*	{0, 0, 0,                  VNODEPROTO,  0, "userbits", 0, 0, 0, 0},  *** cell specific ***/
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, DRAWTINYCELLS, 0, N_("Tiny cell instances hashed out")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, CHECKDATE, 0, N_("Check cell dates during creation")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, AUTOSWITCHTECHNOLOGY, 0, N_("Switch technology to match current cell")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, CELLCENTERALWAYS, 0, N_("Place cell center in new cells")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_tiny_lambda_per_pixel"), 0, 0, 0, 0, N_("Hash out scale")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_facet_explorer_textsize"), 0, 0, 0, 0, N_("Cell explorer text size")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpframe[] =				/* VIEW: Frame Options */
{
/*	{0, 0, 0,                  VNODEPROTO,  0, "FACET_schematic_page_size", 0, 0, 0, 0, N_("")},  *** cell specific ***/
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_drawing_company_name"), 0, 0, 0, 0, N_("Company name")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_drawing_designer_name"), 0, 0, 0, 0, N_("Designer name")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_drawing_project_name"), 0, 0, 0, 0, N_("Project name")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpicon[] =				/* VIEW: Icon Options */
{
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_icon_style"), 0, 0, ICONSTYLESIDEIN, 0, N_("Inputs on which side")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_icon_style"), 0, 0, ICONSTYLESIDEOUT, 0, N_("Outputs on which side")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_icon_style"), 0, 0, ICONSTYLESIDEBIDIR, 0, N_("Bidir. on which side")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_icon_style"), 0, 0, ICONSTYLESIDEPOWER, 0, N_("Power on which side")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_icon_style"), 0, 0, ICONSTYLESIDEGROUND, 0, N_("Ground on which side")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_icon_style"), 0, 0, ICONSTYLESIDECLOCK, 0, N_("Clock on which side")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_icon_style"), 0, 0, ICONSTYLEPORTLOC, 0, N_("Export location")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_icon_style"), 0, 0, ICONSTYLEPORTSTYLE, 0, N_("Export style")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_icon_style"), 0, 0, ICONSTYLETECH, 0, N_("Export technology")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_icon_style"), 0, 0, ICONSTYLEDRAWNOLEADS, 0, N_("Draw leads")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_icon_style"), 0, 0, ICONSTYLEDRAWNOBODY, 0, N_("Draw body")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_icon_style"), 0, 0, ICONSTYLEREVEXPORT, 0, N_("Reverse export order")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_icon_style"), 0, 0, ICONINSTLOC, 0, N_("Instance location")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_icon_lead_length"), 0, 0, 0, 0, N_("Lead length")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_icon_lead_spacing"), 0, 0, 0, 0, N_("Lead spacing")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpnewarc[] =				/* ARC: New Arc Options */
{
	{0, 0, 0,                  VARCPROTO,   0, x_("ARC_width_default"), 0, 0, 0, 0, N_("Width")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_arc_style"), 0, 0, WANTFIX, 0, N_("All rigid")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_arc_style"), 0, 0, WANTFIXANG, 0, N_("All fixed angle")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_arc_style"), 0, 0, WANTCANTSLIDE, 0, N_("All slidable")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_arc_style"), 0, 0, WANTNEGATED, 0, N_("All negated")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_arc_style"), 0, 0, WANTNOEXTEND, 0, N_("All ends extended")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_arc_style"), 0, 0, WANTDIRECTIONAL, 0, N_("All directional")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_arc_style"), 0, 0, AANGLEINC, 0, N_("All angle")},
	{0, 0, 0,                  VARCPROTO,   0, x_("USER_arc_style"), 0, 0, WANTFIX, 0, N_("Arc rigid")},
	{0, 0, 0,                  VARCPROTO,   0, x_("USER_arc_style"), 0, 0, WANTFIXANG, 0, N_("Arc fixed angle")},
	{0, 0, 0,                  VARCPROTO,   0, x_("USER_arc_style"), 0, 0, WANTCANTSLIDE, 0, N_("Arc slidable")},
	{0, 0, 0,                  VARCPROTO,   0, x_("USER_arc_style"), 0, 0, WANTNEGATED, 0, N_("Arc negated")},
	{0, 0, 0,                  VARCPROTO,   0, x_("USER_arc_style"), 0, 0, WANTNOEXTEND, 0, N_("Arc ends extended")},
	{0, 0, 0,                  VARCPROTO,   0, x_("USER_arc_style"), 0, 0, WANTDIRECTIONAL, 0, N_("Arc directional")},
	{0, 0, 0,                  VARCPROTO,   0, x_("USER_arc_style"), 0, 0, AANGLEINC, 0, N_("Arc angle")},
	{0, 0, 0,                  VARCPROTO,   0, x_("ARC_Default_Pin"), 0, 0, 0, 0, N_("Pin")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};

static OPTIONVARPROTO db_ovpgrid[] =				/* WINDOWS: Grid Options */
{
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_default_grid"), 0, 0, 0, 0, N_("Default grid spacing")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_grid_bold_spacing"), 0, 0, 0, 0, N_("Distance between bold dots")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_grid_floats"), 0, 0, 0, 0, N_("Align grid with circuit")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpalign[] =				/* WINDOWS: Alignment Options */
{
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_alignment_ratio"), 0, 0, 0, 0, N_("Alignment of cursor to grid")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_alignment_edge_ratio"), 0, 0, 0, 0, N_("Alignment of edges to grid")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovplayvis[] =				/* WINDOWS: Layer Visibility Options */
{
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, HIDETXTNODE, 0, N_("Node text")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, HIDETXTARC, 0, N_("Arc text")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, HIDETXTPORT, 0, N_("Port text")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, HIDETXTEXPORT, 0, N_("Export text")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, HIDETXTNONLAY, 0, N_("Nonlayout text")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, HIDETXTINSTNAME, 0, N_("Instance names")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, HIDETXTCELL, 0, N_("Cell text")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovplaypat[] =				/* WINDOWS: Layer Display Options */
{
	{0, 0, 0,                  VTECHNOLOGY, 1, x_("TECH_layer_pattern_"), 0, 0, 0, 0, N_("Layer pattern/color")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpcolor[] =				/* WINDOWS: Color Options */
{
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("USER_print_colors"), 0, 0, 0, 0, N_("Print colors")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_colormap_red"), 0, 0, 0, 0, N_("Red colormap")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_colormap_green"), 0, 0, 0, 0, N_("Green colormap")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_colormap_blue"), 0, 0, 0, 0, N_("Blue colormap")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpport[] =				/* WINDOWS: Port and Export Options */
{
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, PORTLABELS, 0, N_("Ports (on instances)")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, EXPORTLABELS, 0, N_("Exports (in cells)")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, MOVENODEWITHEXPORT, 0, N_("Move node with export name")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovptext[] =				/* WINDOWS: Text Options */
{
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_default_text_style"), 0, 0, 0, 0, N_("Text corner")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_default_text_smart_style"), 0, 0, 0, 0, N_("Smart placement")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_default_node_text_size"), 0, 0, 0, 0, N_("Node style")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_default_arc_text_size"), 0, 0, 0, 0, N_("Arc style")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_default_export_text_size"), 0, 0, 0, 0, N_("Export style")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_default_nonlayout_text_size"), 0, 0, 0, 0, N_("Nonlayout style")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_default_instance_text_size"), 0, 0, 0, 0, N_("Instance style")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_default_facet_text_size"), 0, 0, 0, 0, N_("Cell style")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_text_editor"), 0, 0, 0, 0, N_("Text editor")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovp3d[] =					/* WINDOWS: 3D Options */
{
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, NO3DPERSPECTIVE, 0, N_("Use perspective")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("TECH_layer_3dheight"), 0, 0, 0, 0, N_("Height")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("TECH_layer_3dthickness"), 0, 0, 0, 0, N_("Thickness")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpmsgloc[] =				/* WINDOWS: Save Messages Location */
{
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_messages_position"), 0, 0, 0, 0, N_("Messages window location")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpgeneral[] =				/* INFO: General Options */
{
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, NODATEORVERSION, 0, N_("Include date and version in output files")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, NOPROMPTBEFOREWRITE, 0, N_("Show file-selection dialog before writing netlists")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, BEEPAFTERLONGJOB, 0, N_("Beep after long jobs")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, EXPANDEDDIALOGSDEF, 0, N_("Expandable dialogs default to fullsize")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_optionflags"), 0, 0, NOEXTRASOUND, 0, N_("Click sounds when arcs are created")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_maximum_errors"), 0, 0, 0, 0, N_("Maximum errors to report")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_motion_delay"), 0, 0, 0, 0, N_("Motion delay after selection")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpquickkey[] =			/* INFO: Quick Key Options */
{
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_quick_keys"), 0, 0, 0, 0, N_("Quick keys")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovptechopt[] =				/* TECHNOLOGY: Technology Options */
{
	{0, 0, (INTBIG*)&mocmossub_tech,VTECHNOLOGY, 0, x_("TECH_state"), 0, 0, MOCMOSSUBMETALS, 0, N_("mocmossub number of metal layers")},
	{0, 0, (INTBIG*)&mocmossub_tech,VTECHNOLOGY, 0, x_("TECH_state"), 0, 0, MOCMOSSUBNOCONV, 0, N_("mocmossub converts to mocmos")},
	{0, 0, (INTBIG*)&mocmos_tech,VTECHNOLOGY, 0, x_("TECH_state"), 0, 0, MOCMOSMETALS, 0, N_("mocmos number of metal layers")},
	{0, 0, (INTBIG*)&mocmos_tech,VTECHNOLOGY, 0, x_("TECH_state"), 0, 0, MOCMOSSTICKFIGURE, 0, N_("mocmos stick figure")},
	{0, 0, (INTBIG*)&mocmos_tech,VTECHNOLOGY, 0, x_("TECH_state"), 0, 0, MOCMOSNOSTACKEDVIAS, 0, N_("mocmos stacked vias")},
	{0, 0, (INTBIG*)&mocmos_tech,VTECHNOLOGY, 0, x_("TECH_state"), 0, 0, MOCMOSALTAPRULES, 0, N_("mocmos alternate contact rules")},
	{0, 0, (INTBIG*)&mocmos_tech,VTECHNOLOGY, 0, x_("TECH_state"), 0, 0, MOCMOSTWOPOLY, 0, N_("mocmos number of polysilicon layers")},
	{0, 0, (INTBIG*)&mocmos_tech,VTECHNOLOGY, 0, x_("TECH_state"), 0, 0, MOCMOSSPECIALTRAN, 0, N_("mocmos shows special transistors")},
	{0, 0, (INTBIG*)&mocmos_tech,VTECHNOLOGY, 0, x_("TECH_state"), 0, 0, MOCMOSRULESET, 0, N_("mocmos rule set")},
	{0, 0, (INTBIG*)&art_tech, VTECHNOLOGY, 0, x_("TECH_state"), 0, 0, ARTWORKFILLARROWHEADS, 0, N_("artwork fills arrowheads")},
	{0, 0, (INTBIG*)&sch_tech, VTECHNOLOGY, 0, x_("TECH_layout_technology"), 0, 0, 0, 0, N_("Technology to use for schematics")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovptechunits[] =			/* TECHNOLOGY: Units */
{
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_display_units"), 0, 0, DISPLAYUNITS, 0, N_("Display units")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_display_units"), 0, 0, INTERNALUNITS, 0, N_("Internal units")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("USER_electrical_units"), 0, 0, 0, 0, N_("Electrical units")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
#if DRCTOOL
static OPTIONVARPROTO db_ovpdrc[] =					/* TOOLS: DRC: DRC Options */
{
	{0, 0, (INTBIG*)&dr_tool,  VTOOL,       0, x_("DRC_pointout"), 0, 0, 0, 0, N_("Incremental DRC highlights errors")},
	{0, 0, (INTBIG*)&dr_tool,  VTOOL,       0, x_("DRC_options"), 0, 0, DRCFIRSTERROR, 0, N_("Just 1 error per cell")},
	{0, 0, (INTBIG*)&dr_tool,  VTOOL,       0, x_("DRC_options"), 0, 0, DRCREASONABLE, 0, N_("Ignore center cuts in large contacts")},
	{0, 0, (INTBIG*)&dr_tool,  VTOOL,       0, x_("DRC_options"), 0, 0, DRCMULTIPROC|DRCNUMPROC, 0, N_("Use multiple processors")},
	{0, 0, (INTBIG*)&dr_tool,  VTOOL,       0, x_("DRC_incrementalon"), 0, 0, 0, 0, N_("Incremental DRC on")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("DRC_ecad_deck"), 0, 0, 0, 0, N_("Dracula rules deck")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpdrcr[] =				/* TOOLS: DRC: DRC Rules */
{
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("DRC_wide_limit"), 0, 0, 0, 0, N_("Wide rule size")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("DRC_min_width"), 0, 0, 0, 0, N_("Minimum width distance")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("DRC_min_width_rule"), 0, 0, 0, 0, N_("Minimum width rule")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("DRC_min_connected_distances"), 0, 0, 0, 0, N_("Normal connected distance")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("DRC_min_connected_distances_rule"), 0, 0, 0, 0, N_("Normal connected rule")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("DRC_min_unconnected_distances"), 0, 0, 0, 0, N_("Normal not-connected distance")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("DRC_min_unconnected_distances_rule"), 0, 0, 0, 0, N_("Normal not-connected rule")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("DRC_min_connected_distances_wide"), 0, 0, 0, 0, N_("Wide connected distance")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("DRC_min_connected_distances_wide_rule"), 0, 0, 0, 0, N_("Wide connected rule")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("DRC_min_unconnected_distances_wide"), 0, 0, 0, 0, N_("Wide not-connected spacing")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("DRC_min_unconnected_distances_wide_rule"), 0, 0, 0, 0, N_("Wide not-connected rule")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("DRC_min_connected_distances_multi"), 0, 0, 0, 0, N_("Multiple cuts connected distance")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("DRC_min_connected_distances_multi_rule"), 0, 0, 0, 0, N_("Multiple cuts connected rule")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("DRC_min_unconnected_distances_multi"), 0, 0, 0, 0, N_("Multiple cuts not-connected distance")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("DRC_min_unconnected_distances_multi_rule"), 0, 0, 0, 0, N_("Multiple cuts not-connected rule")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("DRC_min_edge_distances"), 0, 0, 0, 0, N_("Edge distance")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("DRC_min_edge_distances_rule"), 0, 0, 0, 0, N_("Edge rule")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("DRC_min_node_size"), 0, 0, 0, 0, N_("Minimum node size")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("DRC_min_node_size_rule"), 0, 0, 0, 0, N_("Minimum node size rule")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
#endif
#if SIMTOOL
static OPTIONVARPROTO db_ovpsimulation[] =			/* TOOLS: Simulation: Simulation Options */
{
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_als_no_update"), 0, 0, 0, 0, N_("Resimulate each change")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_als_num_events"), 0, 0, 0, 0, N_("ALS: maximum events")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_window_state"), 0, 0, ADVANCETIME, 0, N_("Auto advance time")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_window_state"), 0, 0, BUSBASEBITS, 0, N_("Base for bus values")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_window_state"), 0, 0, SIMENGINE, 0, N_("Simulation engine")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_window_state"), 0, 0, SHOWWAVEFORM, 0, N_("Show waveform window")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_window_state"), 0, 0, WAVEPLACE, 0, N_("Place waveform window")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_window_state"), 0, 0, FULLSTATE, 0, N_("Multistate display")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_irsim_state"), 0, 0, IRSIMPARASITICS, 0, N_("IRSIM parasitics")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_irsim_state"), 0, 0, IRSIMSHOWCOMMANDS, 0, N_("IRSIM show commands")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_irsim_parameter_file"), 0, 0, 0, 0, N_("IRSIM parameter file")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_window_color_str0"), 0, 0, 0, 0, N_("Strength 0 (off) color")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_window_color_str1"), 0, 0, 0, 0, N_("Strength 1 (node) color")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_window_color_str2"), 0, 0, 0, 0, N_("Strength 2 (gate) color")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_window_color_str3"), 0, 0, 0, 0, N_("Strength 3 (power) color")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_window_color_low"), 0, 0, 0, 0, N_("Low color")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_window_color_high"), 0, 0, 0, 0, N_("High color")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_window_color_X"), 0, 0, 0, 0, N_("Undefined (X) color")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_window_color_Z"), 0, 0, 0, 0, N_("Floating (Z) color")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpsimspice[] =			/* TOOLS: Simulation Interface: Spice Options */
{
/*	{0, 0, 0,                  VNODEPROTO,  0, "SIM_spice_behave_file", 0, 0, 0, 0, N_("")},  *** cell specific ***/
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_dontrun"), 0, 0, 0, 0, N_("Running SPICE")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_spice_level"), 0, 0, 0, 0, N_("SPICE level")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_spice_state"), 0, 0, SPICETYPE, 0, N_("SPICE engine")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_spice_state"), 0, 0, SPICEOUTPUT, 0, N_("SPICE output format")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_spice_state"), 0, 0, SPICEGLOBALPG, 0, N_("Force global VDD/GND")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_spice_state"), 0, 0, SPICEUSELAMBDAS, 0, N_("Write sizes in lambda")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_spice_parts"), 0, 0, 0, 0, N_("SPICE primitive set")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_spice_runarguments"), 0, 0, 0, 0, N_("SPICE execution arguments")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("SIM_spice_model_file"), 0, 0, 0, 0, N_("Use header cards from file")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("SIM_spice_trailer_file"), 0, 0, 0, 0, N_("Use trailer cards from file")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("SIM_spice_header_level1"), 0, 0, 0, 0, N_("Level 1 built-in header cards")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("SIM_spice_header_level2"), 0, 0, 0, 0, N_("Level 2 built-in header cards")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("SIM_spice_header_level3"), 0, 0, 0, 0, N_("Level 3 built-in header cards")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("SIM_spice_resistance"), 0, 0, 0, 0, N_("Resistance")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("SIM_spice_capacitance"), 0, 0, 0, 0, N_("Capacitance")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("SIM_spice_edge_capacitance"), 0, 0, 0, 0, N_("Edge capacitance")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("SIM_spice_min_resistance"), 0, 0, 0, 0, N_("Min. resistance")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("SIM_spice_min_capacitance"), 0, 0, 0, 0, N_("Min. capacitance")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpsimverilog[] =			/* TOOLS: Simulation Interface: Verilog Options */
{
/*	{0, 0, 0,                  VNODEPROTO,  0, "SIM_verilog_behave_file", 0, 0, 0, 0, N_("")},  *** cell specific ***/
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_verilog_state"), 0, 0, VERILOGUSEASSIGN, 0, N_("Use assign construct")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_verilog_state"), 0, 0, VERILOGUSETRIREG, 0, N_("Default wire is trireg")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpsimfasthenry[] =		/* TOOLS: Simulation Interface: FastHenry Options */
{
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_fasthenry_state"), 0, 0, FHUSESINGLEFREQ, 0, N_("Use single frequency")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_fasthenry_state"), 0, 0, FHMAKEMULTIPOLECKT, 0, N_("Make multipole subcircuit")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_fasthenry_state"), 0, 0, FHMAKEPOSTSCRIPTVIEW, 0, N_("Make PostScript view")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_fasthenry_state"), 0, 0, FHMAKESPICESUBCKT, 0, N_("Make SPICE subcircuit")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_fasthenry_state"), 0, 0, FHEXECUTETYPE, 0, N_("Action after writing deck")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_fasthenry_freqstart"), 0, 0, 0, 0, N_("FastHenry start frequency")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_fasthenry_freqend"), 0, 0, 0, 0, N_("FastHenry end frequency")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_fasthenry_runsperdecade"), 0, 0, 0, 0, N_("FastHenry runs per decade")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_fasthenry_numpoles"), 0, 0, 0, 0, N_("FastHenry number of poles")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_fasthenry_seglimit"), 0, 0, 0, 0, N_("FastHenry segment limit")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_fasthenry_thickness"), 0, 0, 0, 0, N_("FastHenry thickness")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_fasthenry_width_subdivs"), 0, 0, 0, 0, N_("FastHenry width subdivisions")},
	{0, 0, (INTBIG*)&sim_tool, VTOOL,       0, x_("SIM_fasthenry_height_subdivs"), 0, 0, 0, 0, N_("FastHenry height subdivisions")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
#endif
#if ERCTOOL
static OPTIONVARPROTO db_ovpercwell[] =					/* TOOLS: ERC: Well Check Options */
{
	{0, 0, (INTBIG*)&erc_tool, VTOOL,       0, x_("ERC_options"), 0, 0, PWELLCONOPTIONS, 0, N_("PWell contact checks")},
	{0, 0, (INTBIG*)&erc_tool, VTOOL,       0, x_("ERC_options"), 0, 0, NWELLCONOPTIONS, 0, N_("NWell contact checks")},
	{0, 0, (INTBIG*)&erc_tool, VTOOL,       0, x_("ERC_options"), 0, 0, PWELLONGROUND, 0, N_("Check PWell to ground")},
	{0, 0, (INTBIG*)&erc_tool, VTOOL,       0, x_("ERC_options"), 0, 0, NWELLONPOWER, 0, N_("Check NWell to power")},
	{0, 0, (INTBIG*)&erc_tool, VTOOL,       0, x_("ERC_options"), 0, 0, FINDEDGEDIST, 0, N_("Find farthest distance from contact to edge")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpercant[] =					/* TOOLS: ERC: Antenna Rules Options */
{
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("ERC_antenna_arc_ratio"), 0, 0, 0, 0, N_("Maximum antenna ratio")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
#endif
static OPTIONVARPROTO db_ovpncc[] =					/* TOOLS: Network: NCC Options */
{
	{0, 0, (INTBIG*)&net_tool, VTOOL,       0, x_("NET_ncc_options"), 0, 0, NCCRECURSE, 0, N_("Recurse through hierarchy")},
	{0, 0, (INTBIG*)&net_tool, VTOOL,       0, x_("NET_ncc_options"), 0, 0, NCCHIERARCHICAL, 0, N_("Expand hierarchy")},
	{0, 0, (INTBIG*)&net_tool, VTOOL,       0, x_("NET_ncc_options"), 0, 0, NCCNOMERGEPARALLEL, 0, N_("Merge parallel components")},
	{0, 0, (INTBIG*)&net_tool, VTOOL,       0, x_("NET_ncc_options"), 0, 0, NCCMERGESERIES, 0, N_("Merge series transistors")},
	{0, 0, (INTBIG*)&net_tool, VTOOL,       0, x_("NET_ncc_options"), 0, 0, NCCIGNOREPWRGND, 0, N_("Ignore power and ground")},
	{0, 0, (INTBIG*)&net_tool, VTOOL,       0, x_("NET_ncc_options"), 0, 0, NCCCHECKEXPORTNAMES, 0, N_("Check export names")},
	{0, 0, (INTBIG*)&net_tool, VTOOL,       0, x_("NET_ncc_options"), 0, 0, NCCCHECKSIZE, 0, N_("Check component sizes")},
	{0, 0, (INTBIG*)&net_tool, VTOOL,       0, x_("NET_ncc_options"), 0, 0, NCCINCLUDENOCOMPNETS, 0, N_("Allow no-component nets")},
	{0, 0, (INTBIG*)&net_tool, VTOOL,       0, x_("NET_ncc_options"), 0, 0, NCCRESISTINCLUSION, 0, N_("NCC automatic resistor adjustment")},
	{0, 0, (INTBIG*)&net_tool, VTOOL,       0, x_("NET_ncc_options"), 0, 0, NCCDISLOCAFTERMATCH|NCCENAFOCSYMGRPFRE|NCCENAFOCSYMGRPPRO|NCCSUPALLAMBREP|NCCVERBOSETEXT|NCCVERBOSEGRAPHICS|NCCGRAPHICPROGRESS|NCCENASTATISTICS, 0, N_("NCC Debugging settings")},
	{0, 0, (INTBIG*)&net_tool, VTOOL,       0, x_("NET_ncc_component_tolerance"), 0, 0, 0, 0, N_("Size tolerance (%)")},
	{0, 0, (INTBIG*)&net_tool, VTOOL,       0, x_("NET_ncc_component_tolerance_amt"), 0, 0, 0, 0, N_("Size tolerance (amt)")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
static OPTIONVARPROTO db_ovpnetwork[] =				/* TOOLS: Network: Network Options */
{
	{0, 0, (INTBIG*)&net_tool, VTOOL,       0, x_("NET_options"), 0, 0, NETCONPWRGND, 0, N_("Unify Power and Ground")},
	{0, 0, (INTBIG*)&net_tool, VTOOL,       0, x_("NET_options"), 0, 0, NETCONCOMMONNAME, 0, N_("Unify all like-named nets")},
	{0, 0, (INTBIG*)&net_tool, VTOOL,       0, x_("NET_options"), 0, 0, NETIGNORERESISTORS, 0, N_("Ignore Resistors")},
	{0, 0, (INTBIG*)&net_tool, VTOOL,       0, x_("NET_options"), 0, 0, NETDEFBUSBASE1, 0, N_("Bus starting index")},
	{0, 0, (INTBIG*)&net_tool, VTOOL,       0, x_("NET_options"), 0, 0, NETDEFBUSBASEDESC, 0, N_("Bus ascending/descending")},
	{0, 0, (INTBIG*)&net_tool, VTOOL,       0, x_("NET_unify_strings"), 0, 0, 0, 0, N_("Network unification prefix")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
#if LOGEFFTOOL
static OPTIONVARPROTO db_ovplogeffort[] =			/* TOOLS: Logical Effort: Logical Effort Options */
{
	{0, 0, (INTBIG*)&le_tool,  VTOOL,       0, x_("LE_state"), 0, 0, DISPLAYCAPACITANCE, 0, N_("Display capacitance")},
	{0, 0, (INTBIG*)&le_tool,  VTOOL,       0, x_("LE_state"), 0, 0, HIGHLIGHTCOMPONENTS, 0, N_("Highlight components")},
	{0, 0, (INTBIG*)&le_tool,  VTOOL,       0, x_("LE_state"), 0, 0, LEUSELESETTINGS, 0, N_("Use Local LE Settings")},
	{0, 0, (INTBIG*)&le_tool,  VTOOL,       0, x_("LE_maximum_stage_effort"), 0, 0, 0, 0, N_("Maximum stage gain")},
	{0, 0, (INTBIG*)&le_tool,  VTOOL,       0, x_("LE_global_fanout_effort"), 0, 0, 0, 0, N_("Global fanout")},
	{0, 0, (INTBIG*)&le_tool,  VTOOL,       0, x_("LE_convergence_epsilon"), 0, 0, 0, 0, N_("Convergence Epsilon")},
	{0, 0, (INTBIG*)&le_tool,  VTOOL,       0, x_("LE_diff_ratio_nmos"), 0, 0, 0, 0, N_("NMOS Diffusion ratio")},
	{0, 0, (INTBIG*)&le_tool,  VTOOL,       0, x_("LE_diff_ratio_pmos"), 0, 0, 0, 0, N_("PMOS Diffusion ratio")},
	{0, 0, (INTBIG*)&le_tool,  VTOOL,       0, x_("LE_gate_cap"), 0, 0, 0, 0, N_("Gate capacitance ratio")},
	{0, 0, (INTBIG*)&le_tool,  VTOOL,       0, x_("LE_def_wire_ratio"), 0, 0, 0, 0, N_("Default wire ratio")},
	{0, 0, (INTBIG*)&le_tool,  VTOOL,       0, x_("LE_max_iterations"), 0, 0, 0, 0, N_("Maximum iterations")},
	{0, 0, (INTBIG*)&le_tool,  VTOOL,       0, x_("LE_keeper_size_adj"), 0, 0, 0, 0, N_("Keeper size ratio")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
#endif
#if ROUTTOOL
static OPTIONVARPROTO db_ovprouting[] =				/* TOOLS: Routing: Routing Options */
{
	{0, 0, (INTBIG*)&ro_tool,  VTOOL,       0, x_("ROUT_prefered_arc"), 0, 0, 0, 0, N_("Prefered arc")},
	{0, 0, (INTBIG*)&ro_tool,  VTOOL,       0, x_("ROUT_options"), 0, 0, MIMICUNROUTES, 0, N_("Mimic stitching can unstitch")},
	{0, 0, (INTBIG*)&ro_tool,  VTOOL,       0, x_("ROUT_options"), 0, 0, MIMICIGNOREPORTS, 0, N_("Ports must match")},
	{0, 0, (INTBIG*)&ro_tool,  VTOOL,       0, x_("ROUT_options"), 0, 0, MIMICONSIDERARCCOUNT, 0, N_("Number of existing arcs must match")},
	{0, 0, (INTBIG*)&ro_tool,  VTOOL,       0, x_("ROUT_options"), 0, 0, MIMICIGNORENODETYPE, 0, N_("Node types must match")},
	{0, 0, (INTBIG*)&ro_tool,  VTOOL,       0, x_("ROUT_options"), 0, 0, MIMICIGNORENODESIZE, 0, N_("Nodes sizes must match")},
	{0, 0, (INTBIG*)&ro_tool,  VTOOL,       0, x_("ROUT_options"), 0, 0, MIMICINTERACTIVE, 0, N_("Mimic stitching runs interactively")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
#endif
#if VHDLTOOL
static OPTIONVARPROTO db_ovpvhdl[] =				/* TOOLS: VHDL: VHDL Options */
{
	{0, 0, (INTBIG*)&vhdl_tool,VTOOL,       0, x_("VHDL_vhdl_on_disk"), 0, 0, 0, 0, N_("VHDL stored in cell")},
	{0, 0, (INTBIG*)&vhdl_tool,VTOOL,       0, x_("VHDL_netlist_on_disk"), 0, 0, 0, 0, N_("Netlist stored in cell")},
	{0, 0, 0,                  VTECHNOLOGY, 0, x_("TECH_vhdl_names"), 0, 0, 0, 0, N_("VHDL primitive names")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
#endif
#if SCTOOL
static OPTIONVARPROTO db_ovpsilcomp[] =				/* TOOLS: Silicon Compiler: Silicon Compiler Options */
{
	{0, 0, (INTBIG*)&sc_tool,  VTOOL,       0, x_("SC_horiz_arc"), 0, 0, 0, 0, N_("Horizontal routing arc")},
	{0, 0, (INTBIG*)&sc_tool,  VTOOL,       0, x_("SC_vert_arc"), 0, 0, 0, 0, N_("Vertical routing arc")},
	{0, 0, (INTBIG*)&sc_tool,  VTOOL,       0, x_("SC_l1_width"), 0, 0, 0, 0, N_("Horizontal wire width")},
	{0, 0, (INTBIG*)&sc_tool,  VTOOL,       0, x_("SC_l2_width"), 0, 0, 0, 0, N_("Vertical wire width")},
	{0, 0, (INTBIG*)&sc_tool,  VTOOL,       0, x_("SC_pwr_width"), 0, 0, 0, 0, N_("Power wire width")},
	{0, 0, (INTBIG*)&sc_tool,  VTOOL,       0, x_("SC_main_pwr_width"), 0, 0, 0, 0, N_("Main power wire width")},
	{0, 0, (INTBIG*)&sc_tool,  VTOOL,       0, x_("SC_main_pwr_rail"), 0, 0, 0, 0, N_("Main power arc")},
	{0, 0, (INTBIG*)&sc_tool,  VTOOL,       0, x_("SC_pwell_size"), 0, 0, 0, 0, N_("P-Well height")},
	{0, 0, (INTBIG*)&sc_tool,  VTOOL,       0, x_("SC_pwell_offset"), 0, 0, 0, 0, N_("P-Well offset from bottom")},
	{0, 0, (INTBIG*)&sc_tool,  VTOOL,       0, x_("SC_nwell_size"), 0, 0, 0, 0, N_("N-Well height")},
	{0, 0, (INTBIG*)&sc_tool,  VTOOL,       0, x_("SC_nwell_offset"), 0, 0, 0, 0, N_("N-Well offset from top")},
	{0, 0, (INTBIG*)&sc_tool,  VTOOL,       0, x_("SC_via_size"), 0, 0, 0, 0, N_("Via size")},
	{0, 0, (INTBIG*)&sc_tool,  VTOOL,       0, x_("SC_min_spacing"), 0, 0, 0, 0, N_("Minimum metal spacing")},
	{0, 0, (INTBIG*)&sc_tool,  VTOOL,       0, x_("SC_feedthru_size"), 0, 0, 0, 0, N_("Feedthrough size")},
	{0, 0, (INTBIG*)&sc_tool,  VTOOL,       0, x_("SC_port_x_min_dist"), 0, 0, 0, 0, N_("Minimum port distance")},
	{0, 0, (INTBIG*)&sc_tool,  VTOOL,       0, x_("SC_active_dist"), 0, 0, 0, 0, N_("Minimum active distance")},
	{0, 0, (INTBIG*)&sc_tool,  VTOOL,       0, x_("SC_num_rows"), 0, 0, 0, 0, N_("Number of rows of cells")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
#endif
#if COMTOOL
static OPTIONVARPROTO db_ovpcompaction[] =			/* TOOLS: Compaction: Compaction Options */
{
	{0, 0, (INTBIG*)&com_tool, VTOOL,       0, x_("COM_spread"), 0, 0, 0, 0, N_("Spreads")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
#endif
#if	LANGJAVA
static OPTIONVARPROTO db_ovpjava[] =				/* WINDOWS: Java Options */
{
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("JAVA_flags"), 0, 0, JAVANOCOMPILER, 0, N_("Disable compiler")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("JAVA_flags"), 0, 0, JAVANOEVALUATE, 0, N_("Disable evaluation")},
	{0, 0, (INTBIG*)&us_tool,  VTOOL,       0, x_("JAVA_flags"), 0, 0, JAVAUSEJOSE, 0, N_("Enable Jose")},
	{0, 0, 0,                  0,           0, 0, 0, 0, 0, 0, x_("")}
};
#endif

#define OPTIONCIF                 01		/* CIF Options */
#define OPTIONGDS                 02		/* GDS Options */
#define OPTIONDXF                 04		/* DXF Options */
#define OPTIONEDIF               010		/* EDIF Options */
#define OPTIONPRINT              020		/* Print Options */
#define OPTIONFRAME              040		/* Frame Options */
#define OPTIONALIGNMENT         0100		/* Alignment Options */
#define OPTIONTEXT              0200		/* Text Options */
#define OPTIONNEWNODE           0400		/* New Node Options */
#define OPTIONNEWARC           01000		/* New Arc Options */
#define OPTIONDRC              02000		/* DRC Options */
#define OPTIONSIMULATION       04000		/* Simulation Options */
#define OPTIONSPICE           010000		/* Spice Options */
#define OPTIONVERILOG         020000		/* Verilog Options */
#define OPTIONVHDL            040000		/* VHDL Options */
#define OPTIONSILCOMP        0100000		/* Silicon Compiler Options */
#define OPTIONNETWORK        0200000		/* Network Options */
#define OPTIONROUTING        0400000		/* Routing Options */
#define OPTIONCOMPACTION    01000000		/* Compaction Options */
#define OPTIONGRID          02000000		/* Grid Options */
#define OPTIONLOGEFFORT     04000000		/* Logical Effort Options */
#define OPTIONPORT         010000000		/* Port Display Options */
#define OPTIONCOLOR        020000000		/* Color Options */
#define OPTIONQUICKKEY     040000000		/* Quick Key Options */
#define OPTIONSKILL       0100000000		/* SKILL Options */
#define OPTIONCELL        0200000000		/* Cell Options */
#define OPTIONSELECT      0400000000		/* Selection Options */
#define OPTION3D         01000000000		/* 3D Options */
#define OPTIONTECH       02000000000		/* Technology Options */
#define OPTIONERCWELL    04000000000		/* ERC Well Check Options */
#define OPTIONLAYPAT    010000000000		/* Layer Display Options */
#define OPTIONFASTHENRY 020000000000		/* FastHenry Options */
#define OPTIONDEF                 01		/* DEF Options */
#define OPTIONUNITS               02		/* Technology Units */
#define OPTIONICON                04		/* Icon Options */
#define OPTIONLIBRARY            010		/* Library Options */
#define OPTIONDRCR              0200		/* DRC Rules */
#define OPTIONJAVA              0400		/* Java Options */
#define OPTIONMSGLOC           01000		/* Save Messages Location */
#define OPTIONGENERAL          02000		/* General Options */
#define OPTIONLAYVIS           04000		/* Layer Visibility Options */
#define OPTIONNCC             010000		/* NCC Options */
#define OPTIONCDL             020000		/* CDL Options */
#define OPTIONATTRIBUTES      040000		/* Attribute Options */
#define OPTIONSUE            0100000		/* Sue Options */
#define OPTIONCOPYRIGHT      0200000		/* Copyright Options */
#define OPTIONERCANT        02000000		/* ERC Antenna Rules Options */

static OPTIONVARCOMS db_optionvarcoms[] =
{
	{{0,OPTIONLIBRARY},    N_("File / IO Options / Library Options"), db_ovpforeignfilelibrary},
	{{0,OPTIONCOPYRIGHT},  N_("File / IO Options / Copyright Options"), db_ovpcopyright},
	{{OPTIONCIF,0},        N_("File / IO Options / CIF Options"), db_ovpforeignfilecif},
	{{OPTIONGDS,0},        N_("File / IO Options / GDS II Options"), db_ovpforeignfilegds},
	{{OPTIONEDIF,0},       N_("File / IO Options / EDIF Options"), db_ovpforeignfileedif},
	{{0,OPTIONDEF},        N_("File / IO Options / DEF Options"), db_ovpforeignfiledef},
	{{0,OPTIONCDL},        N_("File / IO Options / CDL Options"), db_ovpforeignfilecdl},
	{{OPTIONDXF,0},        N_("File / IO Options / DXF Options"), db_ovpforeignfiledxf},
	{{0,OPTIONSUE},        N_("File / IO Options / Sue Options"), db_ovpforeignfilesue},
#ifdef FORCECADENCE
	{{OPTIONSKILL,0},      N_("File / IO Options / SKILL Options"), db_ovpforeignfileskill},
#endif
	{{OPTIONPRINT,0},      N_("File / Print Options"), db_ovpprint},
	{{OPTIONNEWNODE,0},    N_("Edit / New Node Options"), db_ovpnewnode},
	{{OPTIONSELECT,0},     N_("Edit / Selection / Selection Options"), db_ovpselect},
	{{OPTIONCELL,0},       N_("Cell / Cell Options"), db_ovpcell},
	{{OPTIONNEWARC,0},     N_("Arc / New Arc Options"), db_ovpnewarc},
	{{OPTIONPORT,0},       N_("Export / Port and Export Options"), db_ovpport},
	{{OPTIONFRAME,0},      N_("View / Frame Options"), db_ovpframe},
	{{0,OPTIONICON},       N_("View / Icon Options"), db_ovpicon},
	{{OPTIONGRID,0},       N_("Windows / Grid Options"), db_ovpgrid},
	{{OPTIONALIGNMENT,0},  N_("Windows / Alignment Options"), db_ovpalign},
	{{0,OPTIONLAYVIS},     N_("Windows / Layer Visibility"), db_ovplayvis},
	{{OPTIONCOLOR,0},      N_("Windows / Color Options"), db_ovpcolor},
	{{OPTIONLAYPAT,0},     N_("Windows / Layer Display Options"), db_ovplaypat},
	{{OPTIONTEXT,0},       N_("Windows / Text Options"), db_ovptext},
	{{OPTION3D,0},         N_("Windows / 3D Display / 3D Options"), db_ovp3d},
	{{0,OPTIONMSGLOC},     N_("Windows / Messages Window / Save Window Location"), db_ovpmsgloc},
	{{0,OPTIONGENERAL},    N_("Info / User Interface / General Options"), db_ovpgeneral},
	{{OPTIONQUICKKEY,0},   N_("Info / User Interface / Quick Key Options"), db_ovpquickkey},
	{{OPTIONTECH,0},       N_("Technology / Technology Options"), db_ovptechopt},
	{{0,OPTIONUNITS},      N_("Technology / Change Units"), db_ovptechunits},
#if DRCTOOL
	{{OPTIONDRC,0},        N_("Tools / DRC / DRC Options"), db_ovpdrc},
	{{OPTIONDRCR,1},       N_("Tools / DRC / DRC Rules"), db_ovpdrcr},
#endif
#if SIMTOOL
	{{OPTIONSIMULATION,0}, N_("Tools / Simulation (Built-in) / Simulation Options"), db_ovpsimulation},
	{{OPTIONSPICE,0},      N_("Tools / Simulation (SPICE)  / Spice Options"), db_ovpsimspice},
	{{OPTIONVERILOG,0},    N_("Tools / Simulation (Verilog)  / Verilog Options"), db_ovpsimverilog},
	{{OPTIONFASTHENRY,0},  N_("Tools / Simulation (Others)  / FastHenry Options"), db_ovpsimfasthenry},
#endif
#if ERCTOOL
	{{OPTIONERCWELL,0},    N_("Tools / Electrical Rules / Well Check Options"), db_ovpercwell},
	{{0,OPTIONERCANT},     N_("Tools / Electrical Rules / Antenna Rules Options"), db_ovpercant},
#endif
	{{0,OPTIONNCC},        N_("Tools / Network / NCC Control and Options"), db_ovpncc},
	{{OPTIONNETWORK,0},    N_("Tools / Network / Network Options"), db_ovpnetwork},
#if LOGEFFTOOL
	{{OPTIONLOGEFFORT,0},  N_("Tools / Logical Effort / Logical Effort Options"), db_ovplogeffort},
#endif
#if ROUTTOOL
	{{OPTIONROUTING,0},    N_("Tools / Routing / Routing Options"), db_ovprouting},
#endif
#if VHDLTOOL
	{{OPTIONVHDL,0},       N_("Tools / VHDL Compiler / VHDL Options"), db_ovpvhdl},
#endif
#if SCTOOL
	{{OPTIONSILCOMP,0},    N_("Tools / Silicon Compiler / Silicon Compiler Options"), db_ovpsilcomp},
#endif
#if COMTOOL
	{{OPTIONCOMPACTION,0}, N_("Tools / Compaction / Compaction Options"), db_ovpcompaction},
#endif
#if	LANGJAVA
	{{0,OPTIONJAVA},       N_("Tools / Language Interpreter / Java Options"), db_ovpjava}
#endif
};

/*
 * Routine to return the "name" of option "aindex" and its "bits".
 * Returns true if the aindex is out of range.
 */
BOOLEAN describeoptions(INTBIG aindex, CHAR **name, INTBIG *bits)
{
	REGISTER INTBIG optioncomcount, i;

	optioncomcount = sizeof(db_optionvarcoms) / sizeof(OPTIONVARCOMS);
	if (aindex < 0 || aindex >= optioncomcount) return(TRUE);
	*name = TRANSLATE(db_optionvarcoms[aindex].command);
	for(i=0; i<SAVEDBITWORDS; i++)
		bits[i] = db_optionvarcoms[aindex].bits[i];
	return(FALSE);
}

/*
 * Routine to cache the current value of all options so that it can be determined
 * later what has changed.
 */
void cacheoptionbitvalues(void)
{
	REGISTER INTBIG i, j, optioncomcount;
	REGISTER OPTIONVARPROTO *ovp;
	REGISTER VARIABLE *var;

	optioncomcount = sizeof(db_optionvarcoms) / sizeof(OPTIONVARCOMS);
	for(i=0; i<optioncomcount; i++)
	{
		ovp = db_optionvarcoms[i].variables;
		for(j=0; ovp[j].variablename != 0; j++)
		{
			if (ovp[j].partial != 0) continue;
			if (ovp[j].addr == 0) continue;
			var = getval(*ovp[j].addr, ovp[j].type, -1, ovp[j].variablename);
			if (var == NOVARIABLE) continue;
			if ((var->type&VTYPE) != VINTEGER) continue;
			if ((var->type&VISARRAY) != 0)
			{
				ovp[j].initialbits1 = ((INTBIG *)var->addr)[0];
				ovp[j].initialbits2 = ((INTBIG *)var->addr)[1];
			} else
			{
				ovp[j].initialbits1 = var->addr;
				ovp[j].initialbits2 = 0;
			}
		}
	}
}

/*
 * Routine to return true if variable "name" on object "addr" (type "type")
 * is an option variable.
 */
BOOLEAN isoptionvariable(INTBIG addr, INTBIG type, CHAR *name)
{
	REGISTER INTBIG i, j, len, optioncomcount;
	REGISTER BOOLEAN retval;
	REGISTER OPTIONVARPROTO *ovp;
	REGISTER VARIABLE *var;
	Q_UNUSED( addr );

	retval = FALSE;
	optioncomcount = sizeof(db_optionvarcoms) / sizeof(OPTIONVARCOMS);
	for(i=0; i<optioncomcount; i++)
	{
		ovp = db_optionvarcoms[i].variables;
		for(j=0; ovp[j].variablename != 0; j++)
		{
			if (ovp[j].type != type) continue;
			if (ovp[j].partial != 0)
			{
				len = estrlen(ovp[j].variablename);
				if (namesamen(ovp[j].variablename, name, len) != 0) continue;
			} else
			{
				if (namesame(ovp[j].variablename, name) != 0) continue;
			}

			/* see if option tracking has been disabled */
			var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_ignoreoptionchangeskey);
			if (var != NOVARIABLE) continue;
			ovp[j].dirty = TRUE;
			retval = TRUE;
		}
	}
	return(retval);
}

/*
 * Routine to list all options into item "item" of dialog "dia".
 */
void listalloptions(INTBIG item, void *dia)
{
	REGISTER INTBIG i, j, optioncomcount, namedoption;
	REGISTER OPTIONVARPROTO *ovp;
	REGISTER void *infstr;

	optioncomcount = sizeof(db_optionvarcoms) / sizeof(OPTIONVARCOMS);
	for(i=0; i<optioncomcount; i++)
	{
		namedoption = 0;
		ovp = db_optionvarcoms[i].variables;
		for(j=0; ovp[j].variablename != 0; j++)
		{
			if (namedoption == 0)
			{
				infstr = initinfstr();
				formatinfstr(infstr, x_("%s:"), TRANSLATE(db_optionvarcoms[i].command));
				DiaStuffLine(dia, item, returninfstr(infstr));
				namedoption = 1;
			}
			infstr = initinfstr();
			formatinfstr(infstr, x_("   %s"), TRANSLATE(ovp[j].meaning));
			DiaStuffLine(dia, item, returninfstr(infstr));
		}
	}
}

/*
 * Routine to display which options have changed.
 */
void explainoptionchanges(INTBIG item, void *dia)
{
	REGISTER INTBIG i, j, k, optioncomcount, namedoption, val1, val2, len;
	REGISTER CHAR *name;
	REGISTER BOOLEAN same;
	REGISTER TECHNOLOGY *tech;
	REGISTER NODEPROTO *np;
	REGISTER ARCPROTO *ap;
	REGISTER OPTIONVARPROTO *ovp;
	REGISTER VARIABLE *var;
	REGISTER void *infstr;

	optioncomcount = sizeof(db_optionvarcoms) / sizeof(OPTIONVARCOMS);
	for(i=0; i<optioncomcount; i++)
	{
		namedoption = 0;
		ovp = db_optionvarcoms[i].variables;
		for(j=0; ovp[j].variablename != 0; j++)
		{
			if (!ovp[j].dirty) continue;
			if (ovp[j].addr == 0)
			{
				same = TRUE;
				switch (ovp[j].type)
				{
					case VTECHNOLOGY:
						for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
						{
							if (ovp[j].partial == 0)
							{
								var = getval((INTBIG)tech, ovp[j].type, -1, ovp[j].variablename);
								if (var != NOVARIABLE && (var->type&VDONTSAVE) == 0) same = FALSE;
							} else
							{
								len = estrlen(ovp[j].variablename);
								for(k=0; k<tech->numvar; k++)
								{
									var = &tech->firstvar[k];
									name = makename(var->key);
									if (namesamen(ovp[j].variablename, name, len) == 0)
									{
										if ((var->type&VDONTSAVE) == 0) same = FALSE;
									}
								}
							}
						}
						break;
					case VNODEPROTO:
						for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
						{
							for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
							{
								var = getval((INTBIG)np, ovp[j].type, -1, ovp[j].variablename);
								if (var != NOVARIABLE && (var->type&VDONTSAVE) == 0) same = FALSE;
							}
							if (!same) break;
						}
						break;
					case VARCPROTO:
						for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
						{
							for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
							{
								var = getval((INTBIG)ap, ovp[j].type, -1, ovp[j].variablename);
								if (var != NOVARIABLE && (var->type&VDONTSAVE) == 0) same = FALSE;
							}
							if (!same) break;
						}
						break;
				}
				if (same) continue;
			} else
			{
				if (ovp[j].maskbits1 != 0 || ovp[j].maskbits2 != 0)
				{
					var = getval(*ovp[j].addr, ovp[j].type, -1, ovp[j].variablename);
					if (var == NOVARIABLE) val1 = val2 = 0; else
					{
						if ((var->type&VTYPE) != VINTEGER) continue;
						if ((var->type&VISARRAY) != 0)
						{
							val1 = ((INTBIG *)var->addr)[0];
							val2 = ((INTBIG *)var->addr)[1];
						} else
						{
							val1 = var->addr;
							val2 = 0;
						}
					}
					same = TRUE;
					if (ovp[j].maskbits1 != 0)
					{
						if ((val1 & ovp[j].maskbits1) != (ovp[j].initialbits1&ovp[j].maskbits1)) same = FALSE;
					}
					if (ovp[j].maskbits2 != 0)
					{
						if ((val2 & ovp[j].maskbits2) != (ovp[j].initialbits2&ovp[j].maskbits2)) same = FALSE;
					}
					if (same) continue;
				}
			}
			if (namedoption == 0)
			{
				infstr = initinfstr();
				formatinfstr(infstr, x_("%s:"), TRANSLATE(db_optionvarcoms[i].command));
				if (item < 0) ttyputmsg(x_("%s"), returninfstr(infstr)); else
					DiaStuffLine(dia, item, returninfstr(infstr));
				namedoption = 1;
			}
			infstr = initinfstr();
			formatinfstr(infstr, x_("   %s"), TRANSLATE(ovp[j].meaning));
			if (item < 0) ttyputmsg(x_("%s"), returninfstr(infstr)); else
				DiaStuffLine(dia, item, returninfstr(infstr));
		}
	}
}

void explainsavedoptions(INTBIG item, BOOLEAN onlychanged, void *dia)
{
	REGISTER INTBIG i, j, k, optioncomcount, namedoption, val1, val2, len;
	REGISTER CHAR *name;
	REGISTER OPTIONVARPROTO *ovp;
	REGISTER BOOLEAN dirty, found;
	REGISTER VARIABLE *var;
	REGISTER TECHNOLOGY *tech;
	REGISTER NODEPROTO *np;
	REGISTER ARCPROTO *ap;
	REGISTER void *infstr;

	optioncomcount = sizeof(db_optionvarcoms) / sizeof(OPTIONVARCOMS);
	for(i=0; i<optioncomcount; i++)
	{
		namedoption = 0;
		ovp = db_optionvarcoms[i].variables;
		for(j=0; ovp[j].variablename != 0; j++)
		{
			if (ovp[j].addr == 0)
			{
				found = FALSE;
				dirty = FALSE;		/* must figure it out for real */
				switch (ovp[j].type)
				{
					case VTECHNOLOGY:
						for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
						{
							if (ovp[j].partial == 0)
							{
								var = getval((INTBIG)tech, ovp[j].type, -1, ovp[j].variablename);
								if (var != NOVARIABLE && (var->type&VDONTSAVE) == 0) { found = TRUE;   break; }
							} else
							{
								len = estrlen(ovp[j].variablename);
								for(k=0; k<tech->numvar; k++)
								{
									var = &tech->firstvar[k];
									name = makename(var->key);
									if (namesamen(ovp[j].variablename, name, len) == 0)
									{
										if ((var->type&VDONTSAVE) == 0) { found = TRUE;   break; }
									}
								}
								if (found) break;
							}
						}
						break;
					case VNODEPROTO:
						for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
						{
							for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
							{
								var = getval((INTBIG)np, ovp[j].type, -1, ovp[j].variablename);
								if (var != NOVARIABLE && (var->type&VDONTSAVE) == 0) { found = TRUE;   break; }
							}
							if (found) break;
						}
						break;
					case VARCPROTO:
						for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
						{
							for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
							{
								var = getval((INTBIG)ap, ovp[j].type, -1, ovp[j].variablename);
								if (var != NOVARIABLE && (var->type&VDONTSAVE) == 0) { found = TRUE;   break; }
							}
							if (found) break;
						}
						break;
				}
				if (!found) continue;
			} else
			{
				var = getval(*ovp[j].addr, ovp[j].type, -1, ovp[j].variablename);
				if (var == NOVARIABLE || (var->type&VDONTSAVE) != 0) continue;
				dirty = FALSE;
				if ((ovp[j].maskbits1 != 0 || ovp[j].maskbits2 != 0) && (var->type&VTYPE) == VINTEGER)
				{
					if ((var->type&VISARRAY) != 0)
					{
						val1 = ((INTBIG *)var->addr)[0];
						val2 = ((INTBIG *)var->addr)[1];
					} else
					{
						val1 = var->addr;
						val2 = 0;
					}
					if (ovp[j].maskbits1 != 0)
					{
						if ((val1 & ovp[j].maskbits1) != (ovp[j].initialbits1&ovp[j].maskbits1)) dirty = TRUE;
					}
					if (ovp[j].maskbits2 != 0)
					{
						if ((val2 & ovp[j].maskbits2) != (ovp[j].initialbits2&ovp[j].maskbits2)) dirty = TRUE;
					}
				} else
				{
					if (ovp[j].dirty) dirty = TRUE;
				}
			}
			if (onlychanged && !dirty) continue;
			if (namedoption == 0)
			{
				infstr = initinfstr();
				formatinfstr(infstr, x_("%s:"), TRANSLATE(db_optionvarcoms[i].command));
				DiaStuffLine(dia, item, returninfstr(infstr));
				namedoption = 1;
			}
			infstr = initinfstr();
			if (!onlychanged && dirty) addstringtoinfstr(infstr, x_("*"));
			formatinfstr(infstr, x_("   %s"), TRANSLATE(ovp[j].meaning));
			DiaStuffLine(dia, item, returninfstr(infstr));
		}
	}
}

/*
 * Routine to return true if options have really changed.
 */
BOOLEAN optionshavechanged(void)
{
	REGISTER INTBIG i, j, k, optioncomcount, val1, val2, same, len;
	REGISTER CHAR *name;
	REGISTER OPTIONVARPROTO *ovp;
	REGISTER TECHNOLOGY *tech;
	REGISTER NODEPROTO *np;
	REGISTER ARCPROTO *ap;
	REGISTER VARIABLE *var;

	optioncomcount = sizeof(db_optionvarcoms) / sizeof(OPTIONVARCOMS);
	for(i=0; i<optioncomcount; i++)
	{
		ovp = db_optionvarcoms[i].variables;
		for(j=0; ovp[j].variablename != 0; j++)
		{
			if (!ovp[j].dirty) continue;
			if (ovp[j].addr == 0)
			{
				switch (ovp[j].type)
				{
					case VTECHNOLOGY:
						for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
						{
							if (ovp[j].partial == 0)
							{
								var = getval((INTBIG)tech, ovp[j].type, -1, ovp[j].variablename);
								if (var != NOVARIABLE && (var->type&VDONTSAVE) == 0) return(TRUE);
							} else
							{
								len = estrlen(ovp[j].variablename);
								for(k=0; k<tech->numvar; k++)
								{
									var = &tech->firstvar[k];
									name = makename(var->key);
									if (namesamen(ovp[j].variablename, name, len) == 0)
									{
										if ((var->type&VDONTSAVE) == 0) return(TRUE);
									}
								}
							}
						}
						break;
					case VNODEPROTO:
						for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
						{
							for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
							{
								var = getval((INTBIG)np, ovp[j].type, -1, ovp[j].variablename);
								if (var != NOVARIABLE && (var->type&VDONTSAVE) == 0) return(TRUE);
							}
						}
						break;
					case VARCPROTO:
						for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
						{
							for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
							{
								var = getval((INTBIG)ap, ovp[j].type, -1, ovp[j].variablename);
								if (var != NOVARIABLE && (var->type&VDONTSAVE) == 0) return(TRUE);
							}
						}
						break;
				}
				continue;
			}
			if (ovp[j].maskbits1 != 0 || ovp[j].maskbits2 != 0)
			{
				var = getval(*ovp[j].addr, ovp[j].type, -1, ovp[j].variablename);
				if (var == NOVARIABLE) val1 = val2 = 0; else
				{
					if ((var->type&VTYPE) != VINTEGER) continue;
					if ((var->type&VISARRAY) != 0)
					{
						val1 = ((INTBIG *)var->addr)[0];
						val2 = ((INTBIG *)var->addr)[1];
					} else
					{
						val1 = var->addr;
						val2 = 0;
					}
				}
				same = 1;
				if (ovp[j].maskbits1 != 0)
				{
					if ((val1 & ovp[j].maskbits1) != (ovp[j].initialbits1&ovp[j].maskbits1)) same = 0;
				}
				if (ovp[j].maskbits2 != 0)
				{
					if ((val2 & ovp[j].maskbits2) != (ovp[j].initialbits2&ovp[j].maskbits2)) same = 0;
				}
				if (same != 0) continue;
			}
			return(TRUE);
		}
	}
	return(FALSE);
}

/*
 * Routine to make all options "temporary" (set their "VDONTSAVE" bit so that they are
 * not saved to disk).  Any exceptions to this may be specified in the
 * "LIB_save_options" variable of "lib".  In addition, all such option variables are
 * remembered so that the "temporary" state can be restored later in
 * "us_restoreoptionstate()".  This is done before saving a library so that the
 * options are *NOT* saved with the data.  Only the "options" library has these
 * option variables saved with it.
 */
void makeoptionstemporary(LIBRARY *lib)
{
	REGISTER INTBIG teccount, arccount, nodecount, i, j, k, l, ind, optioncomcount,
		len;
	INTBIG savebits[SAVEDBITWORDS];
	REGISTER CHAR *name;
	REGISTER VARIABLE *var;
	REGISTER TECHNOLOGY *tech;
	REGISTER NODEPROTO *np;
	REGISTER ARCPROTO *ap;
	REGISTER OPTIONVARPROTO *ovp, *ovp2;

	/* deallocate any former options variable cache */
	if (db_optionvarcachecount > 0) efree((CHAR *)db_optionvarcache);
	db_optionvarcachecount = 0;

	/* see which options *SHOULD* be saved */
	for(i=0; i<SAVEDBITWORDS; i++) savebits[i] = 0;
	var = getval((INTBIG)lib, VLIBRARY, -1, x_("LIB_save_options"));
	if (var != NOVARIABLE)
	{
		if ((var->type&VISARRAY) == 0)
		{
			savebits[0] = var->addr;
		} else
		{
			len = getlength(var);
			for(i=0; i<len; i++) savebits[i] = ((INTBIG *)var->addr)[i];
		}
	}

	/* clear flags of which options to save */
	optioncomcount = sizeof(db_optionvarcoms) / sizeof(OPTIONVARCOMS);
	for(i=0; i<optioncomcount; i++)
	{
		ovp = db_optionvarcoms[i].variables;
		for(j=0; ovp[j].variablename != 0; j++) ovp[j].saveflag = 0;
	}

	/* set flags on options that should be saved */
	for(i=0; i<optioncomcount; i++)
	{
		/* see if this option package should be saved */
		for(j=0; j<SAVEDBITWORDS; j++)
			if ((db_optionvarcoms[i].bits[j]&savebits[j]) != 0) break;
		if (j >= SAVEDBITWORDS) continue;

		ovp = db_optionvarcoms[i].variables;
		for(j=0; ovp[j].variablename != 0; j++)
		{
			ovp[j].saveflag = 1;

			/* also save same option in other commands */
			for(k=0; k<optioncomcount; k++)
			{
				if (k == i) continue;
				ovp2 = db_optionvarcoms[k].variables;
				for(l=0; ovp2[l].variablename != 0; l++)
				{
					if (ovp[j].addr == ovp2[l].addr &&
						ovp[j].type == ovp2[l].type &&
						namesame(ovp[j].variablename, ovp2[l].variablename) == 0)
							ovp2[l].saveflag = 1;
				}
			}
		}
	}

	/* determine the number of option variables */
	teccount = arccount = nodecount = 0;
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		teccount++;
		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto) nodecount++;
		for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto) arccount++;
	}

	db_optionvarcachecount = 0;
	for(i=0; i<optioncomcount; i++)
	{
		ovp = db_optionvarcoms[i].variables;
		for(j=0; ovp[j].variablename != 0; j++)
		{
			if (ovp[j].saveflag != 0) continue;
			switch (ovp[j].type)
			{
				case VTECHNOLOGY:
					if (ovp[j].partial == 0) db_optionvarcachecount += teccount; else
					{
						len = estrlen(ovp[j].variablename);
						for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
						{
							for(k=0; k<tech->numvar; k++)
							{
								var = &tech->firstvar[k];
								name = makename(var->key);
								if (namesamen(ovp[j].variablename, name, len) == 0)
									db_optionvarcachecount++;
							}
						}
					}
					break;
				case VNODEPROTO:  db_optionvarcachecount += nodecount;  break;
				case VARCPROTO:   db_optionvarcachecount += arccount;   break;
				default:          db_optionvarcachecount++;             break;
			}
		}
	}

	/* allocate space for the option variable cache */
	db_optionvarcache = (OPTIONVARCACHE *)emalloc(db_optionvarcachecount * (sizeof (OPTIONVARCACHE)),
		us_tool->cluster);
	if (db_optionvarcache == 0)
	{
		db_optionvarcachecount = 0;
		return;
	}

	/* load the option variable cache */
	ind = 0;
	for(i=0; i<optioncomcount; i++)
	{
		ovp = db_optionvarcoms[i].variables;
		for(j=0; ovp[j].variablename != 0; j++)
		{
			if (ovp[j].saveflag != 0) continue;
			switch (ovp[j].type)
			{
				case VTECHNOLOGY:
					if (ovp[j].partial == 0)
					{
						for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
						{
							db_optionvarcache[ind].addr = (INTBIG)tech;
							db_optionvarcache[ind].type = VTECHNOLOGY;
							db_optionvarcache[ind].variablename = ovp[j].variablename;
							ind++;
						}
					} else
					{
						len = estrlen(ovp[j].variablename);
						for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
						{
							for(k=0; k<tech->numvar; k++)
							{
								var = &tech->firstvar[k];
								name = makename(var->key);
								if (namesamen(ovp[j].variablename, name, len) != 0) continue;
								db_optionvarcache[ind].addr = (INTBIG)tech;
								db_optionvarcache[ind].type = VTECHNOLOGY;
								db_optionvarcache[ind].variablename = name;
								ind++;
							}
						}
					}
					break;
				case VNODEPROTO:
					for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
					{
						for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
						{
							db_optionvarcache[ind].addr = (INTBIG)np;
							db_optionvarcache[ind].type = VNODEPROTO;
							db_optionvarcache[ind].variablename = ovp[j].variablename;
							ind++;
						}
					}
					break;
				case VARCPROTO:
					for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
					{
						for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
						{
							db_optionvarcache[ind].addr = (INTBIG)ap;
							db_optionvarcache[ind].type = VARCPROTO;
							db_optionvarcache[ind].variablename = ovp[j].variablename;
							ind++;
						}
					}
					break;
				default:
					db_optionvarcache[ind].addr = *ovp[j].addr;
					db_optionvarcache[ind].type = ovp[j].type;
					db_optionvarcache[ind].variablename = ovp[j].variablename;
					ind++;
					break;
			}
		}
	}

	/* now cache the parameter state and make them non-savable */
	for(i=0; i<db_optionvarcachecount; i++)
	{
		var = getval(db_optionvarcache[i].addr, db_optionvarcache[i].type, -1,
			db_optionvarcache[i].variablename);
		if (var == NOVARIABLE) continue;
		db_optionvarcache[i].oldtype = var->type;
	}
	for(i=0; i<db_optionvarcachecount; i++)
	{
		var = getval(db_optionvarcache[i].addr, db_optionvarcache[i].type, -1,
			db_optionvarcache[i].variablename);
		if (var == NOVARIABLE) continue;
		var->type |= VDONTSAVE;
	}

	/* finally, cache the font association information */
	db_makefontassociations(lib);
}

/*
 * Routine to look for non-default font usage and write the font name table.
 */
void db_makefontassociations(LIBRARY *lib)
{
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER PORTPROTO *pp;
	REGISTER INTBIG *fontusage, numfonts, i, used;
	CHAR **list, **fontassociations;
	REGISTER void *infstr;

	numfonts = screengetfacelist(&list, FALSE);
	fontusage = (INTBIG *)emalloc(numfonts * SIZEOFINTBIG, db_cluster);
	if (fontusage == 0) return;
	for(i=0; i<numfonts; i++) fontusage[i] = 0;
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			db_makefontassociationvar(ni->numvar, ni->firstvar, fontusage, numfonts);
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				db_makefontassociationvar(pi->numvar, pi->firstvar, fontusage, numfonts);
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
				db_makefontassociationvar(pe->numvar, pe->firstvar, fontusage, numfonts);
			if (ni->proto->primindex == 0)
				db_makefontassociationdescript(ni->textdescript, fontusage, numfonts);
		}
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			db_makefontassociationvar(ai->numvar, ai->firstvar, fontusage, numfonts);
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			db_makefontassociationvar(pp->numvar, pp->firstvar, fontusage, numfonts);
			db_makefontassociationdescript(pp->textdescript, fontusage, numfonts);
		}
		db_makefontassociationvar(np->numvar, np->firstvar, fontusage, numfonts);
	}
	db_makefontassociationvar(lib->numvar, lib->firstvar, fontusage, numfonts);

	/* now look for fonts that are in use */
	used = 0;
	for(i=1; i<numfonts; i++)
	{
		if (fontusage[i] != 0) used++;
	}
	if (used > 0)
	{
		fontassociations = (CHAR **)emalloc(used * (sizeof (CHAR *)), db_cluster);
		if (fontassociations == 0) return;
		used = 0;
		for(i=1; i<numfonts; i++)
		{
			if (fontusage[i] == 0) continue;
			infstr = initinfstr();
			formatinfstr(infstr, x_("%ld/%s"), i, list[i]);
			(void)allocstring(&fontassociations[used], returninfstr(infstr), db_cluster);
			used++;
		}
		nextchangequiet();
		(void)setval((INTBIG)lib, VLIBRARY, x_("LIB_font_associations"),
			(INTBIG)fontassociations, VSTRING|VISARRAY|(used<<VLENGTHSH));
		efree((CHAR *)fontassociations);
	}
	efree((CHAR *)fontusage);
}

/*
 * Helper routine for "db_makefontassociations"
 */
void db_makefontassociationvar(INTSML numvar, VARIABLE *firstvar, INTBIG *fontusage, INTBIG numfonts)
{
	REGISTER INTBIG i;
	REGISTER VARIABLE *var;

	for(i=0; i<numvar; i++)
	{
		var = &firstvar[i];
		if ((var->type&VDISPLAY) != 0)
			db_makefontassociationdescript(var->textdescript, fontusage, numfonts);
	}
}

/*
 * Helper routine for "db_makefontassociations"
 */
void db_makefontassociationdescript(UINTBIG *descript, INTBIG *fontusage, INTBIG numfonts)
{
	REGISTER INTBIG face;

	face = TDGETFACE(descript);
	if (face != 0)
	{
		if (face < numfonts)
			fontusage[face]++;
	}
}

/*
 * Routine to restore the "temporary" state of all option variables (used after
 * the library has been written.
 */
void restoreoptionstate(LIBRARY *lib)
{
	REGISTER INTBIG i;
	REGISTER VARIABLE *var;

	/* restore the parameter state */
	for(i=0; i<db_optionvarcachecount; i++)
	{
		var = getval(db_optionvarcache[i].addr, db_optionvarcache[i].type, -1,
			db_optionvarcache[i].variablename);
		if (var == NOVARIABLE) continue;
		var->type = db_optionvarcache[i].oldtype;
	}
	if (getval((INTBIG)lib, VLIBRARY, VSTRING|VISARRAY, x_("LIB_font_associations")) != NOVARIABLE)
	{
		nextchangequiet();
		(void)delval((INTBIG)lib, VLIBRARY, x_("LIB_font_associations"));
	}
}

/******************** PARAMETERIZED CELLS ********************/

/*
 * Routine to initialize the current list of parameterized cells.
 */
void initparameterizedcells(void)
{
	REGISTER PARCELL *pc;

	while (db_firstparcell != NOPARCELL)
	{
		pc = db_firstparcell;
		db_firstparcell = pc->nextparcell;
		efree((CHAR *)pc->parname);
		efree((CHAR *)pc->shortname);
		db_freeparcell(pc);
	}
}

/*
 * Routine to check to see if parameterization "parname" of cell "np" is in the
 * list of parameterized cells.  If found, returns pointer to the name (or
 * the short name if there is one).  If not found, returns 0.
 */
CHAR *inparameterizedcells(NODEPROTO *np, CHAR *parname)
{
	REGISTER PARCELL *pc;
	REGISTER NODEPROTO *onp;

	for(pc = db_firstparcell; pc != NOPARCELL; pc = pc->nextparcell)
	{
		FOR_CELLGROUP(onp, np)
			if (pc->cell == onp) break;
		if (onp == NONODEPROTO) continue;
		if (namesame(pc->parname, parname) == 0)
		{
			if (pc->shortname != 0) return(pc->shortname);
			return(pc->parname);
		}
	}
	return(0);
}

/*
 * Routine to add parameterization "parname" of cell "np" to the
 * list of parameterized cells.
 */
void addtoparameterizedcells(NODEPROTO *np, CHAR *parname, CHAR *shortname)
{
	REGISTER PARCELL *pc;

	pc = db_allocparcell();
	if (pc == NOPARCELL) return;
	pc->cell = np;
	(void)allocstring(&pc->parname, parname, db_cluster);
	if (shortname == 0) pc->shortname = 0; else
		(void)allocstring(&pc->shortname, shortname, db_cluster);
	pc->nextparcell = db_firstparcell;
	db_firstparcell = pc;
}

/*
 * Routine to create a parameterized name for node instance "ni".
 * If the node is not parameterized, returns zero.
 * If it returns a name, that name must be deallocated when done.
 */
CHAR *parameterizedname(NODEINST *ni, CHAR *cellname)
{
	REGISTER INTBIG i;
	REGISTER VARIABLE *var, *evar;
	CHAR *name;
	REGISTER void *infstr;
	REGISTER BOOLEAN oldonobjectstate;

	if (ni->proto->primindex != 0) return(0);
	for(i=0; i<ni->numvar; i++)
	{
		var = &ni->firstvar[i];
		if (TDGETISPARAM(var->textdescript) != 0) break;
	}
	if (i >= ni->numvar) return(0);
	infstr = initinfstr();
	addstringtoinfstr(infstr, cellname);
	for(i=0; i<ni->numvar; i++)
	{
		var = &ni->firstvar[i];
		if (TDGETISPARAM(var->textdescript) == 0) continue;

		/* accumulate the object that we are on (should use mutex) */
		oldonobjectstate = db_onanobject;
		if (!db_onanobject)
		{
			db_onanobject = TRUE;
			db_onobjectaddr = (INTBIG)ni;
			db_onobjecttype = VNODEINST;
		}
		db_lastonobjectaddr = (INTBIG)ni;
		db_lastonobjecttype = VNODEINST;
		evar = evalvar(var, (INTBIG)ni, VNODEINST);

		/* complete the accumulation of the "getval" object (should use mutex) */
		db_onanobject = oldonobjectstate;
		formatinfstr(infstr, x_("-%s-%s"), truevariablename(var),
			describevariable(evar, -1, -1));
	}
	(void)allocstring(&name, returninfstr(infstr), el_tempcluster);
	return(name);
}

/*
 * Routine to implement a parameterized cell object.  Returns
 * NOPARCELL on error.
 */
PARCELL *db_allocparcell(void)
{
	REGISTER PARCELL *pc;

	if (db_parcellfree != NOPARCELL)
	{
		pc = db_parcellfree;
		db_parcellfree = pc->nextparcell;
	} else
	{
		pc = (PARCELL *)emalloc(sizeof (PARCELL), db_cluster);
		if (pc == 0) return(NOPARCELL);
	}
	return(pc);
}

/*
 * Routine to return parameterized cell object "pc" to the free list.
 */
void db_freeparcell(PARCELL *pc)
{
	pc->nextparcell = db_parcellfree;
	db_parcellfree = pc;
}

/************************* DEPRECATED VARIABLES *************************/

CHAR *el_deprecatednodeprotovariables[] =
{
	x_("NET_last_good_ncc"),
	x_("NET_last_good_ncc_facet"),
	x_("SIM_window_signal_order"),
	0
};

struct
{
	TOOL **tooladdr;
	CHAR *varname;
} el_deprecatedtoolvariables[] =
{
	{&net_tool, x_("NET_auto_name")},
	{&net_tool, x_("NET_use_port_names")},
	{&net_tool, x_("NET_compare_hierarchy")},
	{&net_tool, x_("D")},
	{&net_tool, x_("<")},
	{&net_tool, x_("\001")},
	{&us_tool,  x_("USER_alignment_obj")},
	{&us_tool,  x_("USER_alignment_edge")},
	{&sim_tool, x_("SIM_fasthenry_defthickness")},
	{&dr_tool,  x_("DRC_pointout")},
	{0, 0}
};

BOOLEAN isdeprecatedvariable(INTBIG addr, INTBIG type, CHAR *name)
{
	REGISTER INTBIG i;

	if (type == VNODEPROTO)
	{
		for(i=0; el_deprecatednodeprotovariables[i] != 0; i++)
			if (namesame(name, el_deprecatednodeprotovariables[i]) == 0) return(TRUE);
		return(FALSE);
	}
	if (type == VTOOL)
	{
		for(i=0; el_deprecatedtoolvariables[i].tooladdr != 0; i++)
		{
			if (*el_deprecatedtoolvariables[i].tooladdr != (TOOL *)addr) continue;
			if (namesame(name, el_deprecatedtoolvariables[i].varname) == 0) return(TRUE);
		}
		return(FALSE);
	}
	return(FALSE);
}

/************************* DATA FOR TYPES *************************/

/*
 * In the "type" field of a variable, the VCREF bit is typically set
 * to indicate that the variable points into the fixed C structures, and
 * is not part of the extendable attributes in "firstvar/numvar".  However,
 * in the tables below, the VCREF bit is used to indicate dereferencing
 * in array attributes.  Those arrays that are fixed in size or otherwise
 * part of the C structure need no dereferencing, and have the VCREF bit set
 * (see RTNODE->pointers, GRAPHICS->raster, NODEINST->textdescript, PORTPROTO->textdescript,
 * and POLYGON->textdescript).  Those arrays that vary in length have only pointers in
 * their C structures, and need dereferencing (see PORTPROTO->connects, LIBRARY->lambda,
 * TECHNOLOGY->layers, NETWORK->arcaddr, and NETWORK->networklist).  These
 * do not have the VCREF bit set.
 */

static NODEINST db_ptni;
static NAMEVARS db_nodevars[] =
{
	{x_("arraysize"),         VINTEGER|VCANTSET,          (INTBIG*)&db_ptni.arraysize},
	{x_("firstportarcinst"),  VPORTARCINST|VCANTSET,      (INTBIG*)&db_ptni.firstportarcinst},
	{x_("firstportexpinst"),  VPORTEXPINST|VCANTSET,      (INTBIG*)&db_ptni.firstportexpinst},
	{x_("geom"),              VGEOM|VCANTSET,             (INTBIG*)&db_ptni.geom},
	{x_("highx"),             VINTEGER|VCANTSET,          (INTBIG*)&db_ptni.highx},
	{x_("highy"),             VINTEGER|VCANTSET,          (INTBIG*)&db_ptni.highy},
	{x_("lowx"),              VINTEGER|VCANTSET,          (INTBIG*)&db_ptni.lowx},
	{x_("lowy"),              VINTEGER|VCANTSET,          (INTBIG*)&db_ptni.lowy},
	{x_("nextinst"),          VNODEINST|VCANTSET,         (INTBIG*)&db_ptni.nextinst},
	{x_("nextnodeinst"),      VNODEINST|VCANTSET,         (INTBIG*)&db_ptni.nextnodeinst},
	{x_("parent"),            VNODEPROTO|VCANTSET,        (INTBIG*)&db_ptni.parent},
	{x_("previnst"),          VNODEINST|VCANTSET,         (INTBIG*)&db_ptni.previnst},
	{x_("prevnodeinst"),      VNODEINST|VCANTSET,         (INTBIG*)&db_ptni.prevnodeinst},
	{x_("proto"),             VNODEPROTO|VCANTSET,        (INTBIG*)&db_ptni.proto},
	{x_("rotation"),          VSHORT|VCANTSET,            (INTBIG*)&db_ptni.rotation},
	{x_("temp1"),             VINTEGER,                   (INTBIG*)&db_ptni.temp1},
	{x_("temp2"),             VINTEGER,                   (INTBIG*)&db_ptni.temp2},
	{x_("textdescript"),      VINTEGER|VISARRAY|(TEXTDESCRIPTSIZE<<VLENGTHSH)|VCREF, (INTBIG*)db_ptni.textdescript},
	{x_("transpose"),         VSHORT|VCANTSET,            (INTBIG*)&db_ptni.transpose},
	{x_("userbits"),          VINTEGER,                   (INTBIG*)&db_ptni.userbits},
	{NULL, 0, NULL}  /* 0 */
};

/*
 * entry 9 of the table below ("globalnetchar") gets changed by "db_initnodeprotolist"
 * entry 10 of the table below ("globalnetworks") gets changed by "db_initnodeprotolist"
 */
static NODEPROTO db_ptnp;
static NAMEVARS db_nodeprotovars[] =
{
	{x_("adirty"),            VINTEGER|VCANTSET,          (INTBIG*)&db_ptnp.adirty},
	{x_("cellview"),          VVIEW,                      (INTBIG*)&db_ptnp.cellview},
	{x_("creationdate"),      VINTEGER|VCANTSET,          (INTBIG*)&db_ptnp.creationdate},
	{x_("firstarcinst"),      VARCINST|VCANTSET,          (INTBIG*)&db_ptnp.firstarcinst},
	{x_("firstinst"),         VNODEINST|VCANTSET,         (INTBIG*)&db_ptnp.firstinst},
	{x_("firstnetwork"),      VNETWORK|VCANTSET,          (INTBIG*)&db_ptnp.firstnetwork},
	{x_("firstnodeinst"),     VNODEINST|VCANTSET,         (INTBIG*)&db_ptnp.firstnodeinst},
	{x_("firstportproto"),    VPORTPROTO|VCANTSET,        (INTBIG*)&db_ptnp.firstportproto},
	{x_("globalnetcount"),    VINTEGER|VCANTSET,          (INTBIG*)&db_ptnp.globalnetcount},
	{x_("globalnetchar"),     VINTEGER|VISARRAY|VCANTSET, (INTBIG*)&db_ptnp.globalnetchar},
	{x_("globalnetworks"),    VNETWORK|VISARRAY|VCANTSET, (INTBIG*)&db_ptnp.globalnetworks},
	{x_("highx"),             VINTEGER,                   (INTBIG*)&db_ptnp.highx},
	{x_("highy"),             VINTEGER,                   (INTBIG*)&db_ptnp.highy},
	{x_("lib"),               VLIBRARY|VCANTSET,          (INTBIG*)&db_ptnp.lib},
	{x_("lowx"),              VINTEGER,                   (INTBIG*)&db_ptnp.lowx},
	{x_("lowy"),              VINTEGER,                   (INTBIG*)&db_ptnp.lowy},
	{x_("netd"),              VUNKNOWN|VCANTSET,          (INTBIG*)&db_ptnp.netd},
	{x_("newestversion"),     VNODEPROTO|VCANTSET,        (INTBIG*)&db_ptnp.newestversion},
	{x_("nextcellgrp"),       VNODEPROTO|VCANTSET,        (INTBIG*)&db_ptnp.nextcellgrp},
	{x_("nextcont"),          VNODEPROTO|VCANTSET,        (INTBIG*)&db_ptnp.nextcont},
	{x_("nextnodeproto"),     VNODEPROTO|VCANTSET,        (INTBIG*)&db_ptnp.nextnodeproto},
	{x_("numportprotos"),     VINTEGER|VCANTSET,          (INTBIG*)&db_ptnp.numportprotos},
	{x_("portprotohashtablesize"),VINTEGER|VCANTSET,      (INTBIG*)&db_ptnp.portprotohashtablesize},
	{x_("prevnodeproto"),     VNODEPROTO|VCANTSET,        (INTBIG*)&db_ptnp.prevnodeproto},
	{x_("prevversion"),       VNODEPROTO|VCANTSET,        (INTBIG*)&db_ptnp.prevversion},
	{x_("primindex"),         VINTEGER|VCANTSET,          (INTBIG*)&db_ptnp.primindex},
	{x_("protoname"),         VSTRING,                    (INTBIG*)&db_ptnp.protoname},
	{x_("revisiondate"),      VINTEGER|VCANTSET,          (INTBIG*)&db_ptnp.revisiondate},
	{x_("rtree"),             VRTNODE|VCANTSET,           (INTBIG*)&db_ptnp.rtree},
	{x_("tech"),              VTECHNOLOGY|VCANTSET,       (INTBIG*)&db_ptnp.tech},
	{x_("temp1"),             VINTEGER,                   (INTBIG*)&db_ptnp.temp1},
	{x_("temp2"),             VINTEGER,                   (INTBIG*)&db_ptnp.temp2},
	{x_("userbits"),          VINTEGER,                   (INTBIG*)&db_ptnp.userbits},
	{x_("version"),           VINTEGER|VCANTSET,          (INTBIG*)&db_ptnp.version},
	{NULL, 0, NULL}  /* 0 */
};

static PORTARCINST db_ptpi;
static NAMEVARS db_portavars[] =
{
	{x_("conarcinst"),        VARCINST|VCANTSET,          (INTBIG*)&db_ptpi.conarcinst},
	{x_("nextportarcinst"),   VPORTARCINST|VCANTSET,      (INTBIG*)&db_ptpi.nextportarcinst},
	{x_("proto"),             VPORTPROTO|VCANTSET,        (INTBIG*)&db_ptpi.proto},
	{NULL, 0, NULL}  /* 0 */
};

static PORTEXPINST db_ptpe;
static NAMEVARS db_portevars[] =
{
	{x_("exportproto"),       VPORTPROTO|VCANTSET,        (INTBIG*)&db_ptpe.exportproto},
	{x_("nextportexpinst"),   VPORTEXPINST|VCANTSET,      (INTBIG*)&db_ptpe.nextportexpinst},
	{x_("proto"),             VPORTPROTO|VCANTSET,        (INTBIG*)&db_ptpe.proto},
	{NULL, 0, NULL}  /* 0 */
};

static PORTPROTO db_ptpp;
static NAMEVARS db_portprotovars[] =
{
	{x_("connects"),          VARCPROTO|VISARRAY|VCANTSET,(INTBIG*)&db_ptpp.connects},
	{x_("network"),           VNETWORK|VCANTSET,          (INTBIG*)&db_ptpp.network},
	{x_("nextportproto"),     VPORTPROTO|VCANTSET,        (INTBIG*)&db_ptpp.nextportproto},
	{x_("parent"),            VNODEPROTO|VCANTSET,        (INTBIG*)&db_ptpp.parent},
	{x_("protoname"),         VSTRING,                    (INTBIG*)&db_ptpp.protoname},
	{x_("subnodeinst"),       VNODEINST|VCANTSET,         (INTBIG*)&db_ptpp.subnodeinst},
	{x_("subportexpinst"),    VPORTEXPINST|VCANTSET,      (INTBIG*)&db_ptpp.subportexpinst},
	{x_("subportproto"),      VPORTPROTO|VCANTSET,        (INTBIG*)&db_ptpp.subportproto},
	{x_("temp1"),             VINTEGER,                   (INTBIG*)&db_ptpp.temp1},
	{x_("temp2"),             VINTEGER,                   (INTBIG*)&db_ptpp.temp2},
	{x_("textdescript"),      VINTEGER|VISARRAY|(TEXTDESCRIPTSIZE<<VLENGTHSH)|VCREF,(INTBIG*)db_ptpp.textdescript},
	{x_("userbits"),          VINTEGER,                   (INTBIG*)&db_ptpp.userbits},
	{NULL, 0, NULL}  /* 0 */
};

static ARCINST db_ptai;
static NAMEVARS db_arcvars[] =
{
	{x_("endshrink"),         VINTEGER|VCANTSET,          (INTBIG*)&db_ptai.endshrink},
	{x_("geom"),              VGEOM|VCANTSET,             (INTBIG*)&db_ptai.geom},
	{x_("length"),            VINTEGER|VCANTSET,          (INTBIG*)&db_ptai.length},
	{x_("network"),           VNETWORK|VCANTSET,          (INTBIG*)&db_ptai.network},
	{x_("nextarcinst"),       VARCINST|VCANTSET,          (INTBIG*)&db_ptai.nextarcinst},
	{x_("nodeinst1"),         VNODEINST|VCANTSET,         (INTBIG*)&db_ptai.end[0].nodeinst},
	{x_("nodeinst2"),         VNODEINST|VCANTSET,         (INTBIG*)&db_ptai.end[1].nodeinst},
	{x_("parent"),            VNODEPROTO|VCANTSET,        (INTBIG*)&db_ptai.parent},
	{x_("portarcinst1"),      VPORTARCINST|VCANTSET,      (INTBIG*)&db_ptai.end[0].portarcinst},
	{x_("portarcinst2"),      VPORTARCINST|VCANTSET,      (INTBIG*)&db_ptai.end[1].portarcinst},
	{x_("prevarcinst"),       VARCINST|VCANTSET,          (INTBIG*)&db_ptai.prevarcinst},
	{x_("proto"),             VARCPROTO|VCANTSET,         (INTBIG*)&db_ptai.proto},
	{x_("temp1"),             VINTEGER,                   (INTBIG*)&db_ptai.temp1},
	{x_("temp2"),             VINTEGER,                   (INTBIG*)&db_ptai.temp2},
	{x_("userbits"),          VINTEGER,                   (INTBIG*)&db_ptai.userbits},
	{x_("width"),             VINTEGER|VCANTSET,          (INTBIG*)&db_ptai.width},
	{x_("xpos1"),             VINTEGER|VCANTSET,          (INTBIG*)&db_ptai.end[0].xpos},
	{x_("xpos2"),             VINTEGER|VCANTSET,          (INTBIG*)&db_ptai.end[1].xpos},
	{x_("ypos1"),             VINTEGER|VCANTSET,          (INTBIG*)&db_ptai.end[0].ypos},
	{x_("ypos2"),             VINTEGER|VCANTSET,          (INTBIG*)&db_ptai.end[1].ypos},
	{NULL, 0, NULL}  /* 0 */
};

static ARCPROTO db_ptap;
static NAMEVARS db_arcprotovars[] =
{
	{x_("arcindex"),          VINTEGER|VCANTSET,          (INTBIG*)&db_ptap.arcindex},
	{x_("nextarcproto"),      VARCPROTO|VCANTSET,         (INTBIG*)&db_ptap.nextarcproto},
	{x_("nominalwidth"),      VINTEGER,                   (INTBIG*)&db_ptap.nominalwidth},
	{x_("protoname"),         VSTRING,                    (INTBIG*)&db_ptap.protoname},
	{x_("tech"),              VTECHNOLOGY|VCANTSET,       (INTBIG*)&db_ptap.tech},
	{x_("temp1"),             VINTEGER,                   (INTBIG*)&db_ptap.temp1},
	{x_("temp2"),             VINTEGER,                   (INTBIG*)&db_ptap.temp2},
	{x_("userbits"),          VINTEGER,                   (INTBIG*)&db_ptap.userbits},
	{NULL, 0, NULL}  /* 0 */
};

static GEOM db_ptgeom;
static NAMEVARS db_geomvars[] =
{
	{x_("entryisnode"),       VBOOLEAN|VCANTSET,          (INTBIG*)&db_ptgeom.entryisnode},
	{x_("entryaddr"),         VINTEGER|VCANTSET,          (INTBIG*)&db_ptgeom.entryaddr},
	{x_("highx"),             VINTEGER|VCANTSET,          (INTBIG*)&db_ptgeom.highx},
	{x_("highy"),             VINTEGER|VCANTSET,          (INTBIG*)&db_ptgeom.highy},
	{x_("lowx"),              VINTEGER|VCANTSET,          (INTBIG*)&db_ptgeom.lowx},
	{x_("lowy"),              VINTEGER|VCANTSET,          (INTBIG*)&db_ptgeom.lowy},
	{NULL, 0, NULL}  /* 0 */
};

static LIBRARY db_ptlib;
static NAMEVARS db_libvars[] =
{
	{x_("curnodeproto"),      VNODEPROTO,                 (INTBIG*)&db_ptlib.curnodeproto},
	{x_("firstnodeproto"),    VNODEPROTO|VCANTSET,        (INTBIG*)&db_ptlib.firstnodeproto},
	{x_("lambda"),            VINTEGER|VISARRAY|VCANTSET, (INTBIG*)&db_ptlib.lambda},
	{x_("libname"),           VSTRING,                    (INTBIG*)&db_ptlib.libname},
	{x_("libfile"),           VSTRING,                    (INTBIG*)&db_ptlib.libfile},
	{x_("nextlibrary"),       VLIBRARY|VCANTSET,          (INTBIG*)&db_ptlib.nextlibrary},
	{x_("nodeprotohashtablesize"),VINTEGER|VCANTSET,      (INTBIG*)&db_ptlib.nodeprotohashtablesize},
	{x_("numnodeprotos"),     VINTEGER|VCANTSET,          (INTBIG*)&db_ptlib.numnodeprotos},
	{x_("tailnodeproto"),     VNODEPROTO|VCANTSET,        (INTBIG*)&db_ptlib.tailnodeproto},
	{x_("temp1"),             VINTEGER,                   (INTBIG*)&db_ptlib.temp1},
	{x_("temp2"),             VINTEGER,                   (INTBIG*)&db_ptlib.temp2},
	{x_("userbits"),          VINTEGER,                   (INTBIG*)&db_ptlib.userbits},
	{NULL, 0, NULL}  /* 0 */
};

/*
 * entry 5 of the table below ("layers") gets changed by "db_inittechlist"
 */
static TECHNOLOGY db_pttech;
static NAMEVARS db_techvars[] =
{
	{x_("arcprotocount"),     VINTEGER|VCANTSET,          (INTBIG*)&db_pttech.arcprotocount},
	{x_("deflambda"),         VINTEGER|VCANTSET,          (INTBIG*)&db_pttech.deflambda},
	{x_("firstarcproto"),     VARCPROTO|VCANTSET,         (INTBIG*)&db_pttech.firstarcproto},
	{x_("firstnodeproto"),    VNODEPROTO|VCANTSET,        (INTBIG*)&db_pttech.firstnodeproto},
	{x_("layercount"),        VINTEGER|VCANTSET,          (INTBIG*)&db_pttech.layercount},
	{x_("layers"),            VGRAPHICS|VISARRAY|VCANTSET,(INTBIG*)&db_pttech.layers},
	{x_("nexttechnology"),    VTECHNOLOGY|VCANTSET,       (INTBIG*)&db_pttech.nexttechnology},
	{x_("nodeprotocount"),    VINTEGER|VCANTSET,          (INTBIG*)&db_pttech.nodeprotocount},
	{x_("techdescript"),      VSTRING|VCANTSET,           (INTBIG*)&db_pttech.techdescript},
	{x_("techindex"),         VINTEGER|VCANTSET,          (INTBIG*)&db_pttech.techindex},
	{x_("techname"),          VSTRING,                    (INTBIG*)&db_pttech.techname},
	{x_("temp1"),             VINTEGER,                   (INTBIG*)&db_pttech.temp1},
	{x_("temp2"),             VINTEGER,                   (INTBIG*)&db_pttech.temp2},
	{x_("userbits"),          VINTEGER,                   (INTBIG*)&db_pttech.userbits},
	{NULL, 0, NULL}  /* 0 */
};

static TOOL db_pttool;
static NAMEVARS db_toolvars[] =
{
	{x_("toolindex"),         VINTEGER|VCANTSET,          (INTBIG*)&db_pttool.toolindex},
	{x_("toolname"),          VSTRING|VCANTSET,           (INTBIG*)&db_pttool.toolname},
	{x_("toolstate"),         VINTEGER,                   (INTBIG*)&db_pttool.toolstate},
	{NULL, 0, NULL}  /* 0 */
};

/*
 * entry 6 of the table below ("pointers") gets changed by "db_initrtnodelist"
 */
static RTNODE db_ptrtn;
static NAMEVARS db_rtnvars[] =
{
	{x_("flag"),              VSHORT|VCANTSET,            (INTBIG*)&db_ptrtn.flag},
	{x_("highx"),             VINTEGER|VCANTSET,          (INTBIG*)&db_ptrtn.highx},
	{x_("highy"),             VINTEGER|VCANTSET,          (INTBIG*)&db_ptrtn.highy},
	{x_("lowx"),              VINTEGER|VCANTSET,          (INTBIG*)&db_ptrtn.lowx},
	{x_("lowy"),              VINTEGER|VCANTSET,          (INTBIG*)&db_ptrtn.lowy},
	{x_("parent"),            VRTNODE|VCANTSET,           (INTBIG*)&db_ptrtn.parent},
	{x_("pointers"),          VRTNODE|VISARRAY|VCANTSET|VCREF, (INTBIG*)db_ptrtn.pointers},
	{x_("total"),             VSHORT|VCANTSET,            (INTBIG*)&db_ptrtn.total},
	{NULL, 0, NULL}  /* 0 */
};

/*
 * entry 0 of the table below ("arcaddr") gets changed by "db_initnetworklist"
 * entry 6 of the table below ("networklist") also gets changed
 */
static NETWORK db_ptnet;
static NAMEVARS db_netvars[] =
{
	{x_("arcaddr"),           VARCINST|VISARRAY|VCANTSET, (INTBIG*)&db_ptnet.arcaddr},
	{x_("arccount"),          VSHORT|VCANTSET,            (INTBIG*)&db_ptnet.arccount},
	{x_("arctotal"),          VSHORT|VCANTSET,            (INTBIG*)&db_ptnet.arctotal},
	{x_("buslinkcount"),      VSHORT|VCANTSET,            (INTBIG*)&db_ptnet.buslinkcount},
	{x_("globalnet"),         VSHORT|VCANTSET,            (INTBIG*)&db_ptnet.globalnet},
	{x_("namecount"),         VSHORT|VCANTSET,            (INTBIG*)&db_ptnet.namecount},
	{x_("netnameaddr"),       VINTEGER|VCANTSET,          (INTBIG*)&db_ptnet.netnameaddr},
	{x_("networklist"),       VNETWORK|VISARRAY|VCANTSET, (INTBIG*)&db_ptnet.networklist},
	{x_("nextnetwork"),       VNETWORK|VCANTSET,          (INTBIG*)&db_ptnet.nextnetwork},
	{x_("parent"),            VNODEPROTO|VCANTSET,        (INTBIG*)&db_ptnet.parent},
	{x_("portcount"),         VSHORT|VCANTSET,            (INTBIG*)&db_ptnet.portcount},
	{x_("prevnetwork"),       VNETWORK|VCANTSET,          (INTBIG*)&db_ptnet.prevnetwork},
	{x_("refcount") ,         VSHORT|VCANTSET,            (INTBIG*)&db_ptnet.refcount},
	{x_("buswidth"),          VSHORT|VCANTSET,            (INTBIG*)&db_ptnet.buswidth},
	{x_("temp1"),             VINTEGER,                   (INTBIG*)&db_ptnet.temp1},
	{x_("temp2"),             VINTEGER,                   (INTBIG*)&db_ptnet.temp2},
	{NULL, 0, NULL}  /* 0 */
};

static VIEW db_ptview;
static NAMEVARS db_viewvars[] =
{
	{x_("nextview"),          VVIEW|VCANTSET,             (INTBIG*)&db_ptview.nextview},
	{x_("sviewname"),         VSTRING|VCANTSET,           (INTBIG*)&db_ptview.sviewname},
	{x_("temp1"),             VINTEGER,                   (INTBIG*)&db_ptview.temp1},
	{x_("temp2"),             VINTEGER,                   (INTBIG*)&db_ptview.temp2},
	{x_("viewname"),          VSTRING|VCANTSET,           (INTBIG*)&db_ptview.viewname},
	{x_("viewstate"),         VINTEGER|VCANTSET,          (INTBIG*)&db_ptview.viewstate},
	{NULL, 0, NULL}  /* 0 */
};

static WINDOWPART db_ptwin;
static NAMEVARS db_winvars[] =
{
	{x_("buttonhandler"),     VADDRESS,                   (INTBIG*)&db_ptwin.buttonhandler},
	{x_("changehandler"),     VADDRESS,                   (INTBIG*)&db_ptwin.changehandler},
	{x_("charhandler"),       VADDRESS,                   (INTBIG*)&db_ptwin.charhandler},
	{x_("curnodeproto"),      VNODEPROTO,                 (INTBIG*)&db_ptwin.curnodeproto},
	{x_("editor"),            VADDRESS,                   (INTBIG*)&db_ptwin.editor},
	{x_("expwindow"),         VADDRESS,                   (INTBIG*)&db_ptwin.expwindow},
	{x_("frame"),             VWINDOWFRAME|VCANTSET,      (INTBIG*)&db_ptwin.frame},
	{x_("framehx"),           VINTEGER,                   (INTBIG*)&db_ptwin.framehx},
	{x_("framehy"),           VINTEGER,                   (INTBIG*)&db_ptwin.framehy},
	{x_("framelx"),           VINTEGER,                   (INTBIG*)&db_ptwin.framelx},
	{x_("framely"),           VINTEGER,                   (INTBIG*)&db_ptwin.framely},
	{x_("gridx"),             VINTEGER,                   (INTBIG*)&db_ptwin.gridx},
	{x_("gridy"),             VINTEGER,                   (INTBIG*)&db_ptwin.gridy},
	{x_("hratio"),            VINTEGER,                   (INTBIG*)&db_ptwin.hratio},
	{x_("inplacedepth"),      VINTEGER,                   (INTBIG*)&db_ptwin.inplacedepth},
	{x_("inplacestack"),      VNODEINST|VISARRAY|(MAXINPLACEDEPTH<<VLENGTHSH)|VCREF,(INTBIG*)db_ptwin.inplacestack},
	{x_("intocell"),          VINTEGER|VISARRAY|(9<<VLENGTHSH)|VCREF,(INTBIG*)db_ptwin.intocell},
	{x_("linkedwindowpart"),  VWINDOWPART|VCANTSET,       (INTBIG*)&db_ptwin.linkedwindowpart},
	{x_("location"),          VSTRING|VCANTSET,           (INTBIG*)&db_ptwin.location},
	{x_("nextwindowpart"),    VWINDOWPART|VCANTSET,       (INTBIG*)&db_ptwin.nextwindowpart},
	{x_("outofcell"),         VINTEGER|VISARRAY|(9<<VLENGTHSH)|VCREF,(INTBIG*)db_ptwin.outofcell},
	{x_("prevwindowpart"),    VWINDOWPART|VCANTSET,       (INTBIG*)&db_ptwin.prevwindowpart},
	{x_("redisphandler"),     VADDRESS,                   (INTBIG*)&db_ptwin.redisphandler},
	{x_("scalex"),            VFLOAT,                     (INTBIG*)&db_ptwin.scalex},
	{x_("scaley"),            VFLOAT,                     (INTBIG*)&db_ptwin.scaley},
	{x_("screenhx"),          VINTEGER,                   (INTBIG*)&db_ptwin.screenhx},
	{x_("screenhy"),          VINTEGER,                   (INTBIG*)&db_ptwin.screenhy},
	{x_("screenlx"),          VINTEGER,                   (INTBIG*)&db_ptwin.screenlx},
	{x_("screenly"),          VINTEGER,                   (INTBIG*)&db_ptwin.screenly},
	{x_("state"),             VINTEGER,                   (INTBIG*)&db_ptwin.state},
	{x_("termhandler"),       VADDRESS,                   (INTBIG*)&db_ptwin.termhandler},
	{x_("thumbhx"),           VINTEGER,                   (INTBIG*)&db_ptwin.thumbhx},
	{x_("thumbhy"),           VINTEGER,                   (INTBIG*)&db_ptwin.thumbhy},
	{x_("thumblx"),           VINTEGER,                   (INTBIG*)&db_ptwin.thumblx},
	{x_("thumbly"),           VINTEGER,                   (INTBIG*)&db_ptwin.thumbly},
	{x_("topnodeproto"),      VNODEPROTO,                 (INTBIG*)&db_ptwin.topnodeproto},
	{x_("usehx"),             VINTEGER,                   (INTBIG*)&db_ptwin.usehx},
	{x_("usehy"),             VINTEGER,                   (INTBIG*)&db_ptwin.usehy},
	{x_("uselx"),             VINTEGER,                   (INTBIG*)&db_ptwin.uselx},
	{x_("usely"),             VINTEGER,                   (INTBIG*)&db_ptwin.usely},
	{x_("vratio"),            VINTEGER,                   (INTBIG*)&db_ptwin.vratio},
	{NULL, 0, NULL}  /* 0 */
};

static GRAPHICS db_ptgra;
static NAMEVARS db_gravars[] =
{
	{x_("bits"),              VINTEGER,                   (INTBIG*)&db_ptgra.bits},
	{x_("bwstyle"),           VSHORT,                     (INTBIG*)&db_ptgra.bwstyle},
	{x_("col"),               VINTEGER,                   (INTBIG*)&db_ptgra.col},
	{x_("colstyle"),          VSHORT,                     (INTBIG*)&db_ptgra.colstyle},
	{x_("raster"),            VSHORT|VISARRAY|(16<<VLENGTHSH)|VCREF, (INTBIG*)db_ptgra.raster},
	{NULL, 0, NULL}  /* 0 */
};

static CONSTRAINT db_ptcon;
static NAMEVARS db_convars[] =
{
	{x_("conname"),           VSTRING|VCANTSET,           (INTBIG*)&db_ptcon.conname},
	{NULL, 0, NULL}  /* 0 */
};

static WINDOWFRAME db_ptwf;
static NAMEVARS db_wfvars[] =
{
	{x_("floating"),          VBOOLEAN,                   (INTBIG*)&db_ptwf.floating},
	{x_("width"),             VINTEGER,                   (INTBIG*)&db_ptwf.swid},
	{x_("height"),            VINTEGER,                   (INTBIG*)&db_ptwf.shei},
	{x_("yreverse"),          VINTEGER,                   (INTBIG*)&db_ptwf.revy},
	{NULL, 0, NULL}  /* 0 */
};

/*
 * entry 0 of the table below ("xv") gets changed by "db_initpolygonlist"
 * entry 1 of the table below ("yv") also gets changed
 */
static POLYGON db_ptpoly;
static NAMEVARS db_polyvars[] =
{
	/* the first two entries here cannot be moved (see "db_initpolygonlist()") */
	{x_("xv"),                VINTEGER|VISARRAY|VCANTSET, (INTBIG*)&db_ptpoly.xv},
	{x_("yv"),                VINTEGER|VISARRAY|VCANTSET, (INTBIG*)&db_ptpoly.yv},
	{x_("limit"),             VINTEGER|VCANTSET,          (INTBIG*)&db_ptpoly.limit},
	{x_("count"),             VINTEGER|VCANTSET,          (INTBIG*)&db_ptpoly.count},
	{x_("style"),             VINTEGER,                   (INTBIG*)&db_ptpoly.style},
	{x_("string"),            VSTRING|VCANTSET,           (INTBIG*)&db_ptpoly.string},
	{x_("textdescript"),      VINTEGER|VISARRAY|(TEXTDESCRIPTSIZE<<VLENGTHSH)|VCREF,(INTBIG*)db_ptpoly.textdescript},
	{x_("tech"),              VTECHNOLOGY,                (INTBIG*)&db_ptpoly.tech},
	{x_("desc"),              VGRAPHICS|VCANTSET,         (INTBIG*)&db_ptpoly.desc},
	{x_("layer"),             VINTEGER,                   (INTBIG*)&db_ptpoly.layer},
	{x_("portproto"),         VPORTPROTO,                 (INTBIG*)&db_ptpoly.portproto},
	{x_("nextpolygon"),       VPOLYGON|VCANTSET,          (INTBIG*)&db_ptpoly.nextpolygon},
	{NULL, 0, NULL}  /* 0 */
};

/************************* CODE FOR TYPES *************************/

void db_initnodeinstlist(NODEINST *ni)
{
	if (ni == NONODEINST)
	{
		db_thisapack->state = NULLNAME;
		return;
	}
	db_thisapack->state = PORTANAME;		/* then PORTENAME, SPECNAME, VARNAME */
	db_thisapack->portarcinstname = ni->firstportarcinst;	/* PORTANAME */
	db_thisapack->portexpinstname = ni->firstportexpinst;	/* PORTENAME */
	db_thisapack->vars = db_nodevars;						/* SPECNAME */
	db_thisapack->proto = (INTBIG)&db_ptni;					/* SPECNAME */
	db_thisapack->object = (INTBIG)ni;						/* SPECNAME */
	db_thisapack->specindex = 0;							/* SPECNAME */
	db_thisapack->numvar = &ni->numvar;						/* VARNAME */
	db_thisapack->firstvar = &ni->firstvar;					/* VARNAME */
	db_thisapack->varindex = 0;								/* VARNAME */
}

void db_initnodeprotolist(NODEPROTO *np)
{
	if (np == NONODEPROTO)
	{
		db_thisapack->state = NULLNAME;
		return;
	}
	if (np->primindex == 0)
	{
		db_thisapack->state = ARCINAME;		/* then NODEINAME, PORTCNAME, SPECNAME, VARNAME */
		db_thisapack->arciname  = np->firstarcinst;			/* ARCINAME */
		db_thisapack->nodeiname = np->firstnodeinst;		/* NODEINAME */
	} else db_thisapack->state = PORTCNAME;	/* then SPECNAME, VARNAME */
	if (np->globalnetcount > 0)
	{
		db_nodeprotovars[9].type = VINTEGER|VISARRAY|(np->globalnetcount<<VLENGTHSH)|VCANTSET;
		db_nodeprotovars[10].type = VNETWORK|VISARRAY|(np->globalnetcount<<VLENGTHSH)|VCANTSET;
	} else
	{
		db_nodeprotovars[9].type = VINTEGER|VCANTSET;
		db_nodeprotovars[10].type = VINTEGER|VCANTSET;
	}
	db_thisapack->portprotoname = np->firstportproto;		/* PORTCNAME */
	db_thisapack->vars = db_nodeprotovars;					/* SPECNAME */
	db_thisapack->proto = (INTBIG)&db_ptnp;					/* SPECNAME */
	db_thisapack->object = (INTBIG)np;						/* SPECNAME */
	db_thisapack->specindex = 0;							/* SPECNAME */
	db_thisapack->numvar = &np->numvar;						/* VARNAME */
	db_thisapack->firstvar = &np->firstvar;					/* VARNAME */
	db_thisapack->varindex = 0;								/* VARNAME */
}

void db_initportarcinstlist(PORTARCINST *pi)
{
	if (pi == NOPORTARCINST)
	{
		db_thisapack->state = NULLNAME;
		return;
	}
	db_thisapack->state = SPECNAME;			/* then VARNAME */
	db_thisapack->vars = db_portavars;					/* SPECNAME */
	db_thisapack->proto = (INTBIG)&db_ptpi;				/* SPECNAME */
	db_thisapack->object = (INTBIG)pi;					/* SPECNAME */
	db_thisapack->specindex = 0;						/* SPECNAME */
	db_thisapack->numvar = &pi->numvar;					/* VARNAME */
	db_thisapack->firstvar = &pi->firstvar;				/* VARNAME */
	db_thisapack->varindex = 0;							/* VARNAME */
}

void db_initportexpinstlist(PORTEXPINST *pe)
{
	if (pe == NOPORTEXPINST)
	{
		db_thisapack->state = NULLNAME;
		return;
	}
	db_thisapack->state = SPECNAME;			/* then VARNAME */
	db_thisapack->vars = db_portevars;					/* SPECNAME */
	db_thisapack->proto = (INTBIG)&db_ptpe;				/* SPECNAME */
	db_thisapack->object = (INTBIG)pe;					/* SPECNAME */
	db_thisapack->specindex = 0;						/* SPECNAME */
	db_thisapack->numvar = &pe->numvar;					/* VARNAME */
	db_thisapack->firstvar = &pe->firstvar;				/* VARNAME */
	db_thisapack->varindex = 0;							/* VARNAME */
}

void db_initportprotolist(PORTPROTO *pp)
{
	if (pp == NOPORTPROTO)
	{
		db_thisapack->state = NULLNAME;
		return;
	}
	db_thisapack->state = SPECNAME;			/* then VARNAME */
	db_thisapack->vars = db_portprotovars;				/* SPECNAME */
	db_thisapack->proto = (INTBIG)&db_ptpp;				/* SPECNAME */
	db_thisapack->object = (INTBIG)pp;					/* SPECNAME */
	db_thisapack->specindex = 0;						/* SPECNAME */
	db_thisapack->numvar = &pp->numvar;					/* VARNAME */
	db_thisapack->firstvar = &pp->firstvar;				/* VARNAME */
	db_thisapack->varindex = 0;							/* VARNAME */
}

void db_initarcinstlist(ARCINST *ai)
{
	if (ai == NOARCINST)
	{
		db_thisapack->state = NULLNAME;
		return;
	}
	db_thisapack->state = SPECNAME;			/* then VARNAME */
	db_thisapack->vars = db_arcvars;					/* SPECNAME */
	db_thisapack->proto = (INTBIG)&db_ptai;				/* SPECNAME */
	db_thisapack->object = (INTBIG)ai;					/* SPECNAME */
	db_thisapack->specindex = 0;						/* SPECNAME */
	db_thisapack->numvar = &ai->numvar;					/* VARNAME */
	db_thisapack->firstvar = &ai->firstvar;				/* VARNAME */
	db_thisapack->varindex = 0;							/* VARNAME */
}

void db_initarcprotolist(ARCPROTO *ap)
{
	if (ap == NOARCPROTO)
	{
		db_thisapack->state = NULLNAME;
		return;
	}
	db_thisapack->state = SPECNAME;			/* then VARNAME */
	db_thisapack->vars = db_arcprotovars;				/* SPECNAME */
	db_thisapack->proto = (INTBIG)&db_ptap;				/* SPECNAME */
	db_thisapack->object = (INTBIG)ap;					/* SPECNAME */
	db_thisapack->specindex = 0;						/* SPECNAME */
	db_thisapack->numvar = &ap->numvar;					/* VARNAME */
	db_thisapack->firstvar = &ap->firstvar;				/* VARNAME */
	db_thisapack->varindex = 0;							/* VARNAME */
}

void db_initgeomlist(GEOM *geom)
{
	if (geom == NOGEOM)
	{
		db_thisapack->state = NULLNAME;
		return;
	}
	switch(geom->entryisnode)
	{
		case TRUE:  db_geomvars[1].type = VNODEINST|VCANTSET;   break;
		case FALSE: db_geomvars[1].type = VARCINST|VCANTSET;    break;
	}
	db_thisapack->state = SPECNAME;			/* then VARNAME */
	db_thisapack->vars = db_geomvars;					/* SPECNAME */
	db_thisapack->proto = (INTBIG)&db_ptgeom;			/* SPECNAME */
	db_thisapack->object = (INTBIG)geom;				/* SPECNAME */
	db_thisapack->specindex = 0;						/* SPECNAME */
	db_thisapack->numvar = &geom->numvar;				/* VARNAME */
	db_thisapack->firstvar = &geom->firstvar;			/* VARNAME */
	db_thisapack->varindex = 0;							/* VARNAME */
}

void db_initliblist(LIBRARY *lib)
{
	if (lib == NOLIBRARY)
	{
		db_thisapack->state = NULLNAME;
		return;
	}
	db_thisapack->state = NODENAME;			/* then SPECNAME, VARNAME */
	db_thisapack->nodeprotoname = lib->firstnodeproto;	/* NODENAME */
	db_thisapack->vars = db_libvars;					/* SPECNAME */
	db_thisapack->proto = (INTBIG)&db_ptlib;			/* SPECNAME */
	db_thisapack->object = (INTBIG)lib;					/* SPECNAME */
	db_thisapack->specindex = 0;						/* SPECNAME */
	db_thisapack->numvar = &lib->numvar;				/* VARNAME */
	db_thisapack->firstvar = &lib->firstvar;			/* VARNAME */
	db_thisapack->varindex = 0;							/* VARNAME */
}

void db_inittechlist(TECHNOLOGY *tech)
{
	if (tech == NOTECHNOLOGY)
	{
		db_thisapack->state = NULLNAME;
		return;
	}
	db_techvars[5].type = VGRAPHICS|VISARRAY|(tech->layercount<<VLENGTHSH)|VCANTSET;
	db_thisapack->state = ARCNAME;		/* then NODENAME, SPECNAME, VARNAME */
	db_thisapack->arcprotoname = tech->firstarcproto;	/* ARCNAME */
	db_thisapack->nodeprotoname = tech->firstnodeproto;	/* NODENAME */
	db_thisapack->vars = db_techvars;					/* SPECNAME */
	db_thisapack->proto = (INTBIG)&db_pttech;			/* SPECNAME */
	db_thisapack->object = (INTBIG)tech;				/* SPECNAME */
	db_thisapack->specindex = 0;						/* SPECNAME */
	db_thisapack->numvar = &tech->numvar;				/* VARNAME */
	db_thisapack->firstvar = &tech->firstvar;			/* VARNAME */
	db_thisapack->varindex = 0;							/* VARNAME */
}

void db_inittoollist(TOOL *tool)
{
	if (tool == NOTOOL)
	{
		db_thisapack->state = NULLNAME;
		return;
	}
	db_thisapack->state = SPECNAME;			/* then VARNAME */
	db_thisapack->vars = db_toolvars;					/* SPECNAME */
	db_thisapack->proto = (INTBIG)&db_pttool;			/* SPECNAME */
	db_thisapack->object = (INTBIG)tool;				/* SPECNAME */
	db_thisapack->specindex = 0;						/* SPECNAME */
	db_thisapack->numvar = &tool->numvar;				/* VARNAME */
	db_thisapack->firstvar = &tool->firstvar;			/* VARNAME */
	db_thisapack->varindex = 0;							/* VARNAME */
}

void db_initrtnodelist(RTNODE *rtn)
{
	if (rtn == NORTNODE)
	{
		db_thisapack->state = NULLNAME;
		return;
	}
	db_rtnvars[6].type = VISARRAY | (rtn->total<<VLENGTHSH) | VCANTSET | VCREF;
	if (rtn->flag == 0) db_rtnvars[6].type |= VRTNODE; else
		db_rtnvars[6].type |= VGEOM;
	db_thisapack->state = SPECNAME;			/* then VARNAME */
	db_thisapack->vars = db_rtnvars;					/* SPECNAME */
	db_thisapack->proto = (INTBIG)&db_ptrtn;			/* SPECNAME */
	db_thisapack->object = (INTBIG)rtn;					/* SPECNAME */
	db_thisapack->specindex = 0;						/* SPECNAME */
	db_thisapack->numvar = &rtn->numvar;				/* VARNAME */
	db_thisapack->firstvar = &rtn->firstvar;			/* VARNAME */
	db_thisapack->varindex = 0;							/* VARNAME */
}
/*
 * entry 6 of the table below ("layers") gets changed by "db_inittechlist"
 */

void db_initnetworklist(NETWORK *net)
{
	if (net == NONETWORK)
	{
		db_thisapack->state = NULLNAME;
		return;
	}
	if (net->arccount <= 0) db_netvars[0].type = VADDRESS|VCANTSET; else
		if (net->arctotal == 0) db_netvars[0].type = VARCINST|VCANTSET; else
			db_netvars[0].type = VARCINST|VISARRAY|(net->arccount<<VLENGTHSH)|VCANTSET;
	if (net->buswidth <= 1) db_netvars[6].type = VADDRESS|VCANTSET; else
		db_netvars[6].type = VNETWORK|VISARRAY|(net->buswidth<<VLENGTHSH)|VCANTSET;
	db_thisapack->state = SPECNAME;			/* then VARNAME */
	db_thisapack->vars = db_netvars;					/* SPECNAME */
	db_thisapack->proto = (INTBIG)&db_ptnet;			/* SPECNAME */
	db_thisapack->object = (INTBIG)net;					/* SPECNAME */
	db_thisapack->specindex = 0;						/* SPECNAME */
	db_thisapack->numvar = &net->numvar;				/* VARNAME */
	db_thisapack->firstvar = &net->firstvar;			/* VARNAME */
	db_thisapack->varindex = 0;							/* VARNAME */
}

void db_initviewlist(VIEW *view)
{
	if (view == NOVIEW)
	{
		db_thisapack->state = NULLNAME;
		return;
	}
	db_thisapack->state = SPECNAME;			/* then VARNAME */
	db_thisapack->vars = db_viewvars;					/* SPECNAME */
	db_thisapack->proto = (INTBIG)&db_ptview;			/* SPECNAME */
	db_thisapack->object = (INTBIG)view;				/* SPECNAME */
	db_thisapack->specindex = 0;						/* SPECNAME */
	db_thisapack->numvar = &view->numvar;				/* VARNAME */
	db_thisapack->firstvar = &view->firstvar;			/* VARNAME */
	db_thisapack->varindex = 0;							/* VARNAME */
}

void db_initwindowlist(WINDOWPART *win)
{
	if (win == NOWINDOWPART)
	{
		db_thisapack->state = NULLNAME;
		return;
	}
	db_thisapack->state = SPECNAME;			/* then VARNAME */
	db_thisapack->vars = db_winvars;					/* SPECNAME */
	db_thisapack->proto = (INTBIG)&db_ptwin;			/* SPECNAME */
	db_thisapack->object = (INTBIG)win;					/* SPECNAME */
	db_thisapack->specindex = 0;						/* SPECNAME */
	db_thisapack->numvar = &win->numvar;				/* VARNAME */
	db_thisapack->firstvar = &win->firstvar;			/* VARNAME */
	db_thisapack->varindex = 0;							/* VARNAME */
}

void db_initgraphicslist(GRAPHICS *gra)
{
	if (gra == NOGRAPHICS)
	{
		db_thisapack->state = NULLNAME;
		return;
	}
	db_thisapack->state = SPECNAME;			/* then VARNAME */
	db_thisapack->vars = db_gravars;					/* SPECNAME */
	db_thisapack->proto = (INTBIG)&db_ptgra;			/* SPECNAME */
	db_thisapack->object = (INTBIG)gra;					/* SPECNAME */
	db_thisapack->specindex = 0;						/* SPECNAME */
	db_thisapack->numvar = &gra->numvar;				/* VARNAME */
	db_thisapack->firstvar = &gra->firstvar;			/* VARNAME */
	db_thisapack->varindex = 0;							/* VARNAME */
}

void db_initconstraintlist(CONSTRAINT *con)
{
	if (con == NOCONSTRAINT)
	{
		db_thisapack->state = NULLNAME;
		return;
	}
	db_thisapack->state = SPECNAME;			/* then VARNAME */
	db_thisapack->vars = db_convars;					/* SPECNAME */
	db_thisapack->proto = (INTBIG)&db_ptcon;			/* SPECNAME */
	db_thisapack->object = (INTBIG)con;					/* SPECNAME */
	db_thisapack->specindex = 0;						/* SPECNAME */
	db_thisapack->numvar = &con->numvar;				/* VARNAME */
	db_thisapack->firstvar = &con->firstvar;			/* VARNAME */
	db_thisapack->varindex = 0;							/* VARNAME */
}

void db_initwindowframelist(WINDOWFRAME *wf)
{
	if (wf == NOWINDOWFRAME)
	{
		db_thisapack->state = NULLNAME;
		return;
	}
	db_thisapack->state = SPECNAME;			/* then VARNAME */
	db_thisapack->vars = db_wfvars;						/* SPECNAME */
	db_thisapack->proto = (INTBIG)&db_ptwf;				/* SPECNAME */
	db_thisapack->object = (INTBIG)wf;					/* SPECNAME */
	db_thisapack->specindex = 0;						/* SPECNAME */
	db_thisapack->numvar = &wf->numvar;					/* VARNAME */
	db_thisapack->firstvar = &wf->firstvar;				/* VARNAME */
	db_thisapack->varindex = 0;							/* VARNAME */
}

void db_initpolygonlist(POLYGON *poly)
{
	if (poly == NOPOLYGON)
	{
		db_thisapack->state = NULLNAME;
		return;
	}
	db_thisapack->state = SPECNAME;			/* then VARNAME */
	db_thisapack->vars = db_polyvars;					/* SPECNAME */
	db_thisapack->proto = (INTBIG)&db_ptpoly;			/* SPECNAME */
	db_thisapack->object = (INTBIG)poly;				/* SPECNAME */
	db_thisapack->specindex = 0;						/* SPECNAME */
	db_thisapack->numvar = &poly->numvar;				/* VARNAME */
	db_thisapack->firstvar = &poly->firstvar;			/* VARNAME */
	db_thisapack->varindex = 0;							/* VARNAME */
	db_polyvars[0].type = VINTEGER|VCANTSET|VISARRAY|(poly->count << VLENGTHSH);
	db_polyvars[1].type = VINTEGER|VCANTSET|VISARRAY|(poly->count << VLENGTHSH);
}

/************************* DISPLAYABLE VARIABLE ROUTINES *************************/

/*
 * this routine is used to determine the screen location of
 * letters in displayable text.  The displayable variable is
 * "var"; it resides on object "addr" of type "type"; uses technology
 * "tech"; and is being displayed in window "win".  The routine
 * calculates the position of line "line" and returns it in (x, y).
 * If "lowleft" is true, the returned position is the lower-left
 * corner of the text, otherwise it is the position of the center of the text.
 */
void getdisparrayvarlinepos(INTBIG addr, INTBIG type, TECHNOLOGY *tech, WINDOWPART *win, VARIABLE *var,
	INTBIG line, INTBIG *x, INTBIG *y, BOOLEAN lowleft)
{
	CHAR *string;
	INTBIG cx, cy, distx, disty;
	INTBIG tsx, tsy, style;
	REGISTER INTSML saverot, savetrn;
	REGISTER GEOM *geom;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
	REGISTER INTBIG screen, realscreen, len, lambda;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER LIBRARY *lib;
	REGISTER TECHNOLOGY *lambdatech;
	XARRAY trans;

	switch (type)
	{
		case VNODEINST:
			ni = (NODEINST *)addr;
			makerot(ni, trans);
			geom = ni->geom;
			lambdatech = ni->proto->tech;
			lib = ni->parent->lib;
			cx = (geom->lowx + geom->highx) / 2;
			cy = (geom->lowy + geom->highy) / 2;
			break;
		case VARCINST:
			ai = (ARCINST *)addr;
			geom = ai->geom;
			lambdatech = ai->proto->tech;
			lib = ai->parent->lib;
			cx = (geom->lowx + geom->highx) / 2;
			cy = (geom->lowy + geom->highy) / 2;
			break;
		case VPORTPROTO:
			pp = (PORTPROTO *)addr;
			ni = pp->subnodeinst;
			lambdatech = ni->proto->tech;
			lib = pp->parent->lib;
			saverot = ni->rotation;   savetrn = ni->transpose;
			ni->rotation = ni->transpose = 0;
			portposition(pp->subnodeinst, pp->subportproto, &cx, &cy);
			ni->rotation = saverot;   ni->transpose = savetrn;
			ni = pp->subnodeinst;
			makerot(ni, trans);
			break;
		case VNODEPROTO:
			np = (NODEPROTO *)addr;
			lambdatech = np->tech;
			lib = np->lib;
			cx = cy = 0;		/* cell variables are offset from (0,0) */
			break;
		default:
			lambdatech = NOTECHNOLOGY;
			lib = NOLIBRARY;
	}
	lambdatech = tech;
	if (lowleft)
	{
		distx = TDGETXOFF(var->textdescript);
		lambda = lib->lambda[lambdatech->techindex];
		distx = distx * lambda / 4;
		disty = TDGETYOFF(var->textdescript);
		disty = disty * lambda / 4;
		cx += distx;   cy += disty;
		if (type == VNODEINST || type == VPORTPROTO)
			xform(cx, cy, &cx, &cy, trans);
	}

	/* add variable name if requested */
	string = describedisplayedvariable(var, line, -1);
	screensettextinfo(win, tech, var->textdescript);
	if (*string != 0) screengettextsize(win, string, &tsx, &tsy); else
	{
		screengettextsize(win, x_("Xy"), &tsx, &tsy);
		tsx = 0;
	}

	switch (TDGETPOS(var->textdescript))
	{
		case VTPOSCENT:      style = TEXTCENT;      break;
		case VTPOSBOXED:     style = TEXTBOX;       break;
		case VTPOSUP:        style = TEXTBOT;       break;
		case VTPOSDOWN:      style = TEXTTOP;       break;
		case VTPOSLEFT:      style = TEXTRIGHT;     break;
		case VTPOSRIGHT:     style = TEXTLEFT;      break;
		case VTPOSUPLEFT:    style = TEXTBOTRIGHT;  break;
		case VTPOSUPRIGHT:   style = TEXTBOTLEFT;   break;
		case VTPOSDOWNLEFT:  style = TEXTTOPRIGHT;  break;
		case VTPOSDOWNRIGHT: style = TEXTTOPLEFT;   break;
	}
	if (type == VNODEINST || type == VPORTPROTO)
		style = rotatelabel(style, TDGETROTATION(var->textdescript), trans);

	realscreen = screen = applyyscale(win, cy - win->screenly) + win->usely;
	if ((var->type&VISARRAY) == 0) len = 1; else
		len = getlength(var);
	switch (style)
	{
		case TEXTBOX:
		case TEXTCENT:				/* text is centered about grab point */
		case TEXTLEFT:				/* text is to right of grab point */
		case TEXTRIGHT:				/* text is to left of grab point */
			if (lowleft) realscreen = screen - ((line+1)*tsy - len*tsy/2); else
				realscreen = screen - (line*tsy - len*tsy/2) - tsy/2;
			break;
		case TEXTBOT:				/* text is above grab point */
		case TEXTBOTRIGHT:			/* text is to upper-left of grab point */
		case TEXTBOTLEFT:			/* text is to upper-right of grab point */
			realscreen = screen + (len-line-1) * tsy;
			break;
		case TEXTTOP:				/* text is below grab point */
		case TEXTTOPRIGHT:			/* text is to lower-left of grab point */
		case TEXTTOPLEFT:		/* text is to lower-right of grab point */
			if (lowleft) realscreen = screen - (line+1) * tsy; else
				realscreen = screen - line * tsy;
			break;
	}
	*y = (INTBIG)((realscreen - win->usely) / win->scaley) + win->screenly;

	realscreen = screen = applyxscale(win, cx - win->screenlx) + win->uselx;
	switch (style)
	{
		case TEXTBOX:
		case TEXTCENT:				/* text is centered about grab point */
		case TEXTBOT:				/* text is above grab point */
		case TEXTTOP:				/* text is below grab point */
			if (lowleft) realscreen = screen - tsx/2;
			break;
		case TEXTRIGHT:				/* text is to left of grab point */
		case TEXTBOTRIGHT:			/* text is to upper-left of grab point */
		case TEXTTOPRIGHT:			/* text is to lower-left of grab point */
			if (lowleft) realscreen = screen - tsx;
			break;
	}
	*x = (INTBIG)((realscreen - win->uselx) / win->scalex) + win->screenlx;
}

/*
 * this routine fills polygon poly with an outline of a displayable
 * variable's text.  The displayable variable is "var", it resides
 * on object "geom", and is being displayed in window "win".
 * It is used to draw variable highlighting.
 */
void makedisparrayvarpoly(GEOM *geom, WINDOWPART *win, VARIABLE *var, POLYGON *poly)
{
	INTBIG centerobjx, centerobjy, realobjwidth, realobjheight, len, j,
		screenwidth, screenheight, lx, hx, ly, hy, sslx, sshx, ssly, sshy;
	float sscalex, sscaley;
	static BOOLEAN canscalefonts, fontscalingunknown = TRUE;
	REGISTER BOOLEAN reltext;
	REGISTER NODEINST *ni;
	REGISTER INTBIG distx, disty, lambda;
	REGISTER TECHNOLOGY *tech;
	REGISTER NODEPROTO *np;
	XARRAY trans;
	INTBIG tsx, tsy, lineheight, style;
	CHAR *string;

	if (poly->limit < 4) (void)extendpolygon(poly, 4);
	if (geom == NOGEOM)
	{
		/* cell variables are offset from (0,0) */
		lx = hx = ly = hy = 0;
		tech = el_curtech;
		lambda = el_curlib->lambda[tech->techindex];
	} else
	{
		boundobj(geom, &lx, &hx, &ly, &hy);
		lambda = figurelambda(geom);
		np = geomparent(geom);
		if (np->tech == NOTECHNOLOGY)
			np->tech = whattech(np);
		tech = np->tech;
	}
	distx = TDGETXOFF(var->textdescript);
	distx = distx * lambda / 4;
	disty = TDGETYOFF(var->textdescript);
	disty = disty * lambda / 4;
	centerobjx = (lx + hx) / 2 + distx;
	centerobjy = (ly + hy) / 2 + disty;

	/* rotate grab point if on a node */
	if (geom != NOGEOM && geom->entryisnode)
	{
		ni = geom->entryaddr.ni;
		centerobjx = (ni->lowx + ni->highx) / 2 + distx;
		centerobjy = (ni->lowy + ni->highy) / 2 + disty;
		makerot(ni, trans);
		xform(centerobjx, centerobjy, &centerobjx, &centerobjy, trans);
	}

	if (fontscalingunknown)
	{
		fontscalingunknown = FALSE;
		canscalefonts = graphicshas(CANSCALEFONTS);
	}
	reltext = FALSE;
	if (canscalefonts)
	{
		if ((TDGETSIZE(var->textdescript)&TXTQLAMBDA) != 0) reltext = TRUE;
	}
	if (reltext)
	{
		/* set window to a sensible range */
		sslx = win->screenlx;   win->screenlx = win->uselx * lambda / 12;
		sshx = win->screenhx;   win->screenhx = win->usehx * lambda / 12;
		ssly = win->screenly;   win->screenly = win->usely * lambda / 12;
		sshy = win->screenhy;   win->screenhy = win->usehy * lambda / 12;
		sscalex = win->scalex;   sscaley = win->scaley;
		computewindowscale(win);
	}

	screensettextinfo(win, tech, var->textdescript);
	screengettextsize(win, x_("Xy"), &tsx, &lineheight);
	if ((var->type&VISARRAY) == 0) len = 1; else
		len = getlength(var);
	screenwidth = 0;
	for(j=0; j<len; j++)
	{
		string = describedisplayedvariable(var, j, -1);
		screengettextsize(win, string, &tsx, &tsy);
		if (tsx > screenwidth) screenwidth = tsx;
	}

	screenheight = lineheight * len;
	realobjwidth = (INTBIG)(screenwidth / win->scalex);
	realobjheight = (INTBIG)(screenheight / win->scaley);

	if (reltext)
	{
		/* restore window */
		win->screenlx = sslx;   win->screenhx = sshx;
		win->screenly = ssly;   win->screenhy = sshy;
		win->scalex = sscalex;  win->scaley = sscaley;
	}

	switch (TDGETPOS(var->textdescript))
	{
		case VTPOSCENT:
		case VTPOSBOXED:     style = TEXTCENT;      break;
		case VTPOSUP:        style = TEXTBOT;       break;
		case VTPOSDOWN:      style = TEXTTOP;       break;
		case VTPOSLEFT:      style = TEXTRIGHT;     break;
		case VTPOSRIGHT:     style = TEXTLEFT;      break;
		case VTPOSUPLEFT:    style = TEXTBOTRIGHT;  break;
		case VTPOSUPRIGHT:   style = TEXTBOTLEFT;   break;
		case VTPOSDOWNLEFT:  style = TEXTTOPRIGHT;  break;
		case VTPOSDOWNRIGHT: style = TEXTTOPLEFT;   break;
	}
	if (geom != NOGEOM && geom->entryisnode)
	{
		makerot(geom->entryaddr.ni, trans);
		style = rotatelabel(style, TDGETROTATION(var->textdescript), trans);
	}

	/* set X position */
	switch (style)
	{
		case TEXTCENT:				/* text is centered about grab point */
		case TEXTBOT:				/* text is above grab point */
		case TEXTTOP:				/* text is below grab point */
			poly->xv[0] = poly->xv[1] = centerobjx - realobjwidth/2;
			poly->xv[2] = poly->xv[3] = centerobjx + realobjwidth/2;
			break;
		case TEXTLEFT:				/* text is to right of grab point */
		case TEXTBOTLEFT:			/* text is to upper-right of grab point */
		case TEXTTOPLEFT:			/* text is to lower-right of grab point */
			poly->xv[0] = poly->xv[1] = centerobjx;
			poly->xv[2] = poly->xv[3] = centerobjx + realobjwidth;
			break;
		case TEXTRIGHT:				/* text is to left of grab point */
		case TEXTBOTRIGHT:			/* text is to upper-left of grab point */
		case TEXTTOPRIGHT:			/* text is to lower-left of grab point */
			poly->xv[0] = poly->xv[1] = centerobjx - realobjwidth;
			poly->xv[2] = poly->xv[3] = centerobjx;
			break;
	}

	/* set Y position */
	switch (style)
	{
		case TEXTCENT:				/* text is centered about grab point */
		case TEXTLEFT:				/* text is to right of grab point */
		case TEXTRIGHT:				/* text is to left of grab point */
			poly->yv[0] = poly->yv[3] = centerobjy - realobjheight/2;
			poly->yv[1] = poly->yv[2] = centerobjy + realobjheight/2;
			break;
		case TEXTBOT:				/* text is above grab point */
		case TEXTBOTRIGHT:			/* text is to upper-left of grab point */
		case TEXTBOTLEFT:			/* text is to upper-right of grab point */
			poly->yv[0] = poly->yv[3] = centerobjy;
			poly->yv[1] = poly->yv[2] = centerobjy + realobjheight;
			break;
		case TEXTTOP:				/* text is below grab point */
		case TEXTTOPRIGHT:			/* text is to lower-left of grab point */
		case TEXTTOPLEFT:			/* text is to lower-right of grab point */
			poly->yv[0] = poly->yv[3] = centerobjy - realobjheight;
			poly->yv[1] = poly->yv[2] = centerobjy;
			break;
	}
	poly->count = 4;
}

/*
 * routine to return the name of the variable whose key is "key" and whose
 * type is "type"
 */
CHAR *changedvariablename(INTBIG type, INTBIG key, INTBIG subtype)
{
	if ((subtype&VCREF) == 0)
	{
		if (key == -1) return(_("NULL"));
		return(makename(key));
	}

	switch (type&VTYPE)
	{
		case VNODEINST:    return(db_nodevars[key].name);
		case VNODEPROTO:   return(db_nodeprotovars[key].name);
		case VPORTARCINST: return(db_portavars[key].name);
		case VPORTEXPINST: return(db_portevars[key].name);
		case VPORTPROTO:   return(db_portprotovars[key].name);
		case VARCINST:     return(db_arcvars[key].name);
		case VARCPROTO:    return(db_arcprotovars[key].name);
		case VGEOM:        return(db_geomvars[key].name);
		case VRTNODE:      return(db_rtnvars[key].name);
		case VLIBRARY:     return(db_libvars[key].name);
		case VTECHNOLOGY:  return(db_techvars[key].name);
		case VTOOL:        return(db_toolvars[key].name);
		case VVIEW:        return(db_viewvars[key].name);
		case VNETWORK:     return(db_netvars[key].name);
		case VWINDOWPART:  return(db_winvars[key].name);
		case VGRAPHICS:    return(db_gravars[key].name);
		case VCONSTRAINT:  return(db_convars[key].name);
		case VWINDOWFRAME: return(db_wfvars[key].name);
	}
	return(_("NULL"));
}

/*
 * routine to adjust the coordinate values in "poly" to account for the
 * display offset in the type field "textdescription".  Assumes that the
 * initial values are the center of the object.  The object on which this
 * text resides is in "geom".  Routine also sets the style and the font of
 * text message to use with this coordinate.
 */
void adjustdisoffset(INTBIG addr, INTBIG type, TECHNOLOGY *tech,
	POLYGON *poly, UINTBIG *textdescription)
{
	REGISTER INTBIG lambda;
	INTBIG distx, disty, plx, phx, ply, phy;
	REGISTER INTBIG i;
	REGISTER NODEINST *ni;
	XARRAY trans;

	lambda = el_curlib->lambda[tech->techindex];
	switch (type&VTYPE)
	{
		case VNODEINST: lambda = lambdaofnode((NODEINST *)addr);   break;
		case VARCINST:  lambda = lambdaofarc((ARCINST *)addr);     break;
	}
	distx = TDGETXOFF(textdescription);
	distx = distx * lambda / 4;
	disty = TDGETYOFF(textdescription);
	disty = disty * lambda / 4;
	if (TDGETPOS(textdescription) == VTPOSBOXED)
	{
		getbbox(poly, &plx, &phx, &ply, &phy);
		if (distx > 0) plx += distx*2; else
			if (distx < 0) phx += distx*2;
		if (disty > 0) ply += disty*2; else
			if (disty < 0) phy += disty*2;
		makerectpoly(plx, phx, ply, phy, poly);
	} else
	{
		/* just shift the polygon */
		for(i=0; i<poly->count; i++)
		{
			poly->xv[i] += distx;
			poly->yv[i] += disty;
		}
	}

	/* determine the text message style */
	switch (TDGETPOS(textdescription))
	{
		case VTPOSCENT:      poly->style = TEXTCENT;      break;
		case VTPOSBOXED:     poly->style = TEXTBOX;       break;
		case VTPOSUP:        poly->style = TEXTBOT;       break;
		case VTPOSDOWN:      poly->style = TEXTTOP;       break;
		case VTPOSLEFT:      poly->style = TEXTRIGHT;     break;
		case VTPOSRIGHT:     poly->style = TEXTLEFT;      break;
		case VTPOSUPLEFT:    poly->style = TEXTBOTRIGHT;  break;
		case VTPOSUPRIGHT:   poly->style = TEXTBOTLEFT;   break;
		case VTPOSDOWNLEFT:  poly->style = TEXTTOPRIGHT;  break;
		case VTPOSDOWNRIGHT: poly->style = TEXTTOPLEFT;   break;
	}
	if (type == VNODEINST)
	{
		ni = (NODEINST *)addr;
		makeangle(ni->rotation, ni->transpose, trans);
		poly->style = rotatelabel(poly->style, TDGETROTATION(textdescription), trans);
	}
	if (type == VPORTPROTO)
	{
		ni = ((PORTPROTO *)addr)->subnodeinst;
		makeangle(ni->rotation, ni->transpose, trans);
		poly->style = rotatelabel(poly->style, TDGETROTATION(textdescription), trans);
	}
	TDCOPY(poly->textdescript, textdescription);
	poly->tech = tech;
}

/************************* HELPER ROUTINES *************************/

/*
 * routine to convert a variable name in "name" to a key and return it.
 * If the name is not in the database, the routine returns a negative value
 * that is one less than the negative table index entry.
 */
INTBIG db_getkey(CHAR *name)
{
	REGISTER INTBIG hi, lo, med, lastmed, i;

	/* binary search name space for the variable name */
	i = lo = 0;   hi = lastmed = el_numnames;
	for(;;)
	{
		/* find mid-point: quit if done */
		med = (hi + lo) / 2;
		if (med == lastmed) break;
		lastmed = med;

		/* test the entry: return the key if a match */
		i = namesame(name, el_namespace[med]);
		if (i == 0) return(med);

		/* decide which way to search in list */
		if (i < 0) hi = med; else lo = med;
	}

	/* create a new position: adjust for position location */
	if (i > 0) med++;
	return(-med-1);
}

/*
 * routine to determine the appropriate memory cluster to use for the
 * variable whose address is "addr" and type is "type".
 */
CLUSTER *db_whichcluster(INTBIG addr, INTBIG type)
{
	REGISTER GEOM *geom;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;

	switch (type&VTYPE)
	{
		case VNODEINST:
			ni = (NODEINST *)addr;
			if (ni->parent == NONODEPROTO) return(db_cluster);
			return(ni->parent->lib->cluster);
		case VNODEPROTO:
			np = (NODEPROTO *)addr;
			if (np->primindex == 0) return(np->lib->cluster);
			return(np->tech->cluster);
		case VPORTARCINST:
			return(((PORTARCINST *)addr)->conarcinst->parent->lib->cluster);
		case VPORTEXPINST:
			return(((PORTEXPINST *)addr)->exportproto->parent->lib->cluster);
		case VPORTPROTO:
			np = ((PORTPROTO *)addr)->parent;
			if (np->primindex == 0) return(np->lib->cluster);
			return(np->tech->cluster);
		case VARCINST: return(((ARCINST *)addr)->parent->lib->cluster);
		case VARCPROTO: return(((ARCPROTO *)addr)->tech->cluster);
		case VGEOM: geom = (GEOM *)addr;
			if (geom->entryisnode)
				return(geom->entryaddr.ni->parent->lib->cluster);
			return(geom->entryaddr.ai->parent->lib->cluster);
		case VTECHNOLOGY: return(((TECHNOLOGY *)addr)->cluster);
		case VTOOL: return(((TOOL *)addr)->cluster);
		case VNETWORK: return(((NETWORK *)addr)->parent->lib->cluster);
		case VWINDOWPART: return(us_tool->cluster);
		case VWINDOWFRAME: return(us_tool->cluster);
	}
	return(db_cluster);
}

/*
 * routine to determine the appropriate cell associated with the
 * variable whose address is "addr" and type is "type".
 */
NODEPROTO *db_whichnodeproto(INTBIG addr, INTBIG type)
{
	REGISTER GEOM *geom;
	REGISTER NODEPROTO *np;

	switch (type&VTYPE)
	{
		case VNODEINST: return(((NODEINST *)addr)->parent);
		case VNODEPROTO:
			np = (NODEPROTO *)addr;
			if (np->primindex == 0) return(np); else return(NONODEPROTO);
		case VPORTARCINST: return(((PORTARCINST *)addr)->conarcinst->parent);
		case VPORTEXPINST: return(((PORTEXPINST *)addr)->exportproto->parent);
		case VPORTPROTO: return(((PORTPROTO *)addr)->parent);
		case VARCINST: return(((ARCINST *)addr)->parent);
		case VGEOM: geom = (GEOM *)addr;
			if (geom->entryisnode) return(geom->entryaddr.ni->parent);
			return(geom->entryaddr.ai->parent);
		case VNETWORK: return(((NETWORK *)addr)->parent);
	}
	return(NONODEPROTO);
}

/*
 * internal routine to set entry "med" in the list of variables at "*firstvar"
 * with "*numvar" entries (this entry has the key "key").  If "med" is negative
 * then the entry does not exist in the list and must be created.  The entry is
 * set to the address "newaddr" and type "newtype".  Memory is allocated from
 * cluster "cluster".  Returns true if there is an error.
 */
#ifdef DEBUGMEMORY
BOOLEAN db_setvalkey(INTSML *numvar, VARIABLE **firstvar, INTBIG med, INTBIG key,
	INTBIG newaddr, INTBIG newtype, CLUSTER *cluster, CHAR *module, INTBIG line)
#else
BOOLEAN db_setvalkey(INTSML *numvar, VARIABLE **firstvar, INTBIG med, INTBIG key,
	INTBIG newaddr, INTBIG newtype, CLUSTER *cluster)
#endif
{
	REGISTER VARIABLE *newv, *var;
	REGISTER INTBIG i;

	/* if there is no existing variable, create it */
	if (med < 0)
	{
		/* get the entry position in the list */
		med = -med - 1;

		/* allocate space for new list */
		newv = (VARIABLE *)emalloc(((*numvar+1)*(sizeof (VARIABLE))), cluster);
		if (newv == 0) return(TRUE);

		/* copy old list up to the new entry */
		for(i=0; i<med; i++)
		{
			newv[i].key  = (*firstvar)[i].key;
			newv[i].type = (*firstvar)[i].type;
			TDCOPY(newv[i].textdescript, (*firstvar)[i].textdescript);
			newv[i].addr = (*firstvar)[i].addr;
		}

		/* add the new entry */
		newv[med].key = key;
		newv[med].type = VUNKNOWN|VDONTSAVE;
		TDCLEAR(newv[med].textdescript);
		TDSETSIZE(newv[med].textdescript, TXTSETQLAMBDA(4));
		newv[med].addr = 0;
		var = &newv[med];

		/* copy old list after the new entry */
		for(i = med; i < *numvar; i++)
		{
			newv[i+1].key  = (*firstvar)[i].key;
			newv[i+1].type = (*firstvar)[i].type;
			TDCOPY(newv[i+1].textdescript, (*firstvar)[i].textdescript);
			newv[i+1].addr = (*firstvar)[i].addr;
		}

		/* clean-up */
		if (*numvar != 0) efree((CHAR *)*firstvar);
		*firstvar = newv;
		(*numvar)++;
	} else var = &(*firstvar)[med];

	/* set the variable */
#ifdef DEBUGMEMORY
	if (db_setvalvar(var, newaddr, newtype, cluster, module, line) != 0) return(TRUE);
#else
	if (db_setvalvar(var, newaddr, newtype, cluster) != 0) return(TRUE);
#endif

	return(FALSE);
}

/*
 * routine to set the value of the variable "var" to the value "newaddr" and
 * type "newtype".  Memory is allocated from cluster "cluster".  Returns
 * true upon error.
 */
#ifdef DEBUGMEMORY
BOOLEAN db_setvalvar(VARIABLE *var, INTBIG newaddr, INTBIG newtype,
	CLUSTER *cluster, CHAR *module, INTBIG line)
#else
BOOLEAN db_setvalvar(VARIABLE *var, INTBIG newaddr, INTBIG newtype,
	CLUSTER *cluster)
#endif
{
	REGISTER INTBIG i, len, datasize;
	INTBIG longval;
	REGISTER UCHAR1 *inaddr;

	/* if the variable or key is not valid then ignore this */
	if (var == NOVARIABLE)
	{
		ttyputerr(_("No valid variable"));
		return(TRUE);
	}
	if ((var->type&VCREF) != 0)
	{
		/* setting a fixed attribute on an object */
		if ((newtype&VISARRAY) != 0)
		{
			ttyputmsg(_("Cannot set array in C structure"));
			return(FALSE);
		}
		if (((INTBIG)(var->type&(VTYPE|VISARRAY))) != (newtype&(VTYPE|VISARRAY)))
		{
			ttyputerr(_("Type mismatch"));
			return(TRUE);
		}
#ifdef DEBUGMEMORY
		if (db_setval((UCHAR1 *)&newaddr, (UCHAR1 *)db_realaddress, newtype, cluster,
			module, line)) return(TRUE);
#else
		if (db_setval((UCHAR1 *)&newaddr, (UCHAR1 *)db_realaddress, newtype, cluster))
			return(TRUE);
#endif
		return(FALSE);
	}

	/* if this change isn't subject to undo, free previous memory now */
	if (db_donextchangequietly || db_dochangesquietly)
	{
		db_freevar(var->addr, var->type);
	}

	/* change the variable type */
	var->type = newtype;

	/* allocate and fill space on the new variables */
	if ((newtype&(VCODE1|VCODE2)) != 0)
	{
		if (var->key == sch_globalnamekey || var->key == el_node_name_key || var->key == el_arc_name_key)
		{
			ttyputmsg(_("Language code forbidden for variable %s"), makename(var->key));
			var->type = VSTRING|newtype&(VDISPLAY|VDONTSAVE|VCANTSET);
		}
#ifdef DEBUGMEMORY
		if (db_setval((UCHAR1 *)&newaddr, (UCHAR1 *)&var->addr, VSTRING, cluster,
			module, line)) return(TRUE);
#else
		if (db_setval((UCHAR1 *)&newaddr, (UCHAR1 *)&var->addr, VSTRING, cluster))
			return(TRUE);
#endif
	} else if ((newtype&VISARRAY) != 0)
	{
		var->addr = newaddr;
		len = getlength(var);
		datasize = db_getdatasize(newtype);
		if ((newtype&VLENGTH) == 0)
		{
#ifdef DEBUGMEMORY
			var->addr = (INTBIG)_emalloc((len+1)*datasize, cluster, module, line);
#else
			var->addr = (INTBIG)emalloc((len+1)*datasize, cluster);
#endif
			if (var->addr == 0) return(TRUE);
			for(i=0; i<datasize; i++)
				((UCHAR1 *)var->addr)[len*datasize+i] = 0xff;
		} else
		{
#ifdef DEBUGMEMORY
			var->addr = (INTBIG)_emalloc(len*datasize, cluster, module, line);
#else
			var->addr = (INTBIG)emalloc(len*datasize, cluster);
#endif
			if (var->addr == 0) return(TRUE);
		}
		for(i=0; i<len; i++)
		{
			if ((newtype&VTYPE) == VSHORT)
			{
				longval = ((INTSML *)newaddr)[i];
#ifdef DEBUGMEMORY
				if (db_setval((UCHAR1 *)&longval, &((UCHAR1 *)var->addr)[i*datasize], newtype,
					cluster, module, line)) return(TRUE);
#else
				if (db_setval((UCHAR1 *)&longval, &((UCHAR1 *)var->addr)[i*datasize], newtype,
					cluster)) return(TRUE);
#endif
			} else
			{
				inaddr = (UCHAR1 *)(newaddr + i*datasize);
#ifdef DEBUGMEMORY
				if (db_setval(inaddr, &((UCHAR1 *)var->addr)[i*datasize], newtype,
					cluster, module, line)) return(TRUE);
#else
				if (db_setval(inaddr, &((UCHAR1 *)var->addr)[i*datasize], newtype,
					cluster)) return(TRUE);
#endif
			}
		}
	} else
	{
#ifdef DEBUGMEMORY
		if (db_setval((UCHAR1 *)&newaddr, (UCHAR1 *)&var->addr, newtype, cluster,
			module, line)) return(TRUE);
#else
		if (db_setval((UCHAR1 *)&newaddr, (UCHAR1 *)&var->addr, newtype, cluster))
			return(TRUE);
#endif
	}

	return(FALSE);
}

/*
 * routine to get entry "aindex" of array variable "var" and place it in "value".
 * Returns true upon error.
 */
BOOLEAN db_getindvar(VARIABLE *var, INTBIG aindex, INTBIG *value)
{
	INTBIG type, len;
	REGISTER INTBIG datasize;
	REGISTER void *loc;

	/* if the variable or key is not valid then ignore this */
	if (var == NOVARIABLE) return(TRUE);

	/* if the variable is not an array, quit now */
	type = var->type;
	if ((type&VISARRAY) == 0) return(TRUE);

	/* ensure that the index is within the range of the array */
	len = getlength(var);
	if (aindex < 0) aindex = 0;
	if (aindex >= len) aindex = len - 1;

	/* determine the size of the array entries */
	datasize = db_getdatasize(type);

	/* set the entry */
	if ((type&VCREF) != 0)
	{
		/* get the address of the old array entry (fixed attributes) */
		loc = (void *)(db_realaddress + aindex*datasize);
	} else
	{
		/* get the address of the old array entry (variable attributes) */
		loc = &((CHAR *)var->addr)[aindex*datasize];
	}

	/* set an arbitrary attribute on an object */
	*value = db_assignvalue(loc, datasize);
	return(FALSE);
}

/*
 * routine to set entry "aindex" of array variable "var" (which is on object
 * "objaddr" of type "objtype") to the value "newaddr".  Returns nonzero
 * upon error.
 */
#ifdef DEBUGMEMORY
BOOLEAN db_setindvar(INTBIG objaddr, INTBIG objtype, VARIABLE *var, INTBIG aindex,
	INTBIG newaddr, CHAR *module, INTBIG line)
#else
BOOLEAN db_setindvar(INTBIG objaddr, INTBIG objtype, VARIABLE *var, INTBIG aindex,
	INTBIG newaddr)
#endif
{
	REGISTER INTBIG datasize;
	REGISTER INTBIG len, oldvalue, type;
	REGISTER void *loc;

	/* if the variable or key is not valid then ignore this */
	if (var == NOVARIABLE) return(TRUE);

	/* if the variable is not an array, quit now */
	type = var->type;
	if ((type&VISARRAY) == 0) return(TRUE);

	/* ensure that the index is within the range of the array */
	len = getlength(var);
	if (aindex < 0) aindex = 0;
	if (aindex >= len) aindex = len - 1;

	/* determine the size of the array entries */
	datasize = db_getdatasize(type);

	/* set the entry */
	if ((type&VCREF) != 0)
	{
		/* get the address of the old array entry (fixed attributes) */
		loc = (void *)(db_realaddress + aindex*datasize);
	} else
	{
		/* get the address of the old array entry (variable attributes) */
		loc = &((UCHAR1 *)var->addr)[aindex*datasize];
	}

	/* set an arbitrary attribute on an object */
	oldvalue = db_assignvalue(loc, datasize);
#ifdef DEBUGMEMORY
	if (db_setval((UCHAR1 *)&newaddr, (UCHAR1 *)loc, type,
		db_whichcluster(objaddr, objtype), module, line)) return(TRUE);
#else
	if (db_setval((UCHAR1 *)&newaddr, (UCHAR1 *)loc, type,
		db_whichcluster(objaddr, objtype))) return(TRUE);
#endif

	/* handle change control, constraint, and broadcast */
	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		/* tell constraint system about modified variable */
		(*el_curconstraint->modifyvariable)(objaddr, objtype, var->key, type,
			aindex, oldvalue);

		/* mark a change */
		(void)db_change((INTBIG)objaddr, VARIABLEMOD, objtype, var->key, type,
			aindex, oldvalue, 0);
	} else
	{
		if ((type&(VCODE1|VCODE2)) != 0 || (type&VTYPE) == VSTRING)
			efree((CHAR *)oldvalue);
	}

	return(FALSE);
}

/*
 * routine to insert the value "newaddr" before entry "aindex" of array
 * variable "var" (which is on object "objaddr" of type "objtype").
 * Returns true upon error.
 */
BOOLEAN db_insindvar(INTBIG objaddr, INTBIG objtype, VARIABLE *var, INTBIG aindex,
	INTBIG newaddr)
{
	REGISTER INTBIG i, j, len, origlen, newsize, truelen, type, *newdata;
	REGISTER INTBIG datasize;
	CLUSTER *cluster;

	/* if the variable or key is not valid then ignore this */
	if (var == NOVARIABLE) return(TRUE);

	/* if the variable is not an array, quit now */
	type = var->type;
	if ((type&VISARRAY) == 0) return(TRUE);
	origlen = len = getlength(var);

	/* ensure that the index is valid */
	if (aindex < 0) aindex = 0;

	/* determine the size of the new array */
	newsize = len;
	if (aindex > newsize) newsize = aindex;
	newsize++;
	truelen = newsize;

	/* if this is a variable-length array, include the terminator */
	if ((var->type&VLENGTH) == 0) { origlen++;   newsize++; }

	/* determine the size of the array entries */
	datasize = db_getdatasize(type);

	/* allocate a new array */
	cluster = db_whichcluster(objaddr, objtype);
	newdata = (INTBIG *)emalloc(datasize * newsize, cluster);
	if (newdata == 0) return(TRUE);

	/* copy and insert */
	j = 0;
	for(i=0; i<newsize; i++)
	{
		if (i == aindex)
		{
#ifdef DEBUGMEMORY
			if (db_setval((UCHAR1 *)&newaddr, &((UCHAR1 *)newdata)[aindex*datasize], type,
				cluster, (CHAR *)__FILE__, (INTBIG)__LINE__)) return(TRUE);
#else
			if (db_setval((UCHAR1 *)&newaddr, &((UCHAR1 *)newdata)[aindex*datasize], type,
				cluster)) return(TRUE);
#endif
			continue;
		}
		if (j < origlen)
		{
			switch (type&VTYPE)
			{
				case VCHAR:   ((CHAR   *)newdata)[i] = ((CHAR   *)var->addr)[j];   break;
				case VFLOAT:  ((float  *)newdata)[i] = ((float  *)var->addr)[j];   break;
				case VDOUBLE: ((double *)newdata)[i] = ((double *)var->addr)[j];   break;
				case VSHORT:  ((INTSML *)newdata)[i] = ((INTSML *)var->addr)[j];   break;
				default:      ((INTBIG *)newdata)[i] = ((INTBIG *)var->addr)[j];   break;
			}
			j++;
		} else
		{
			if ((type&(VCODE1|VCODE2)) != 0 || (type&VTYPE) == VSTRING)
			{
				(void)allocstring(&((CHAR **)newdata)[i], x_(""), cluster);
			} else
			{
				switch (type&VTYPE)
				{
					case VCHAR:   ((CHAR   *)newdata)[i] = 0;     break;
					case VFLOAT:  ((float  *)newdata)[i] = 0.0;   break;
					case VDOUBLE: ((double *)newdata)[i] = 0.0;   break;
					case VSHORT:  ((INTSML *)newdata)[i] = 0;     break;
					default:      ((INTBIG *)newdata)[i] = 0;     break;
				}
			}
		}
	}
	efree((CHAR *)var->addr);
	var->addr = (INTBIG)newdata;

	/* bump the array size */
	if ((var->type&VLENGTH) != 0)
		var->type = (var->type & ~VLENGTH) | (truelen << VLENGTHSH);

	/* handle change control, constraint, and broadcast */
	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		/* tell constraint system about inserted variable */
		(*el_curconstraint->insertvariable)(objaddr, objtype, var->key, aindex);

		/* mark a change */
		(void)db_change((INTBIG)objaddr, VARIABLEINS, objtype, var->key, type,
			aindex, 0, 0);
	}

	return(FALSE);
}

/*
 * routine to delete entry "aindex" of array variable "var" (which is on object
 * "objaddr" of type "objtype").  Returns true upon error.
 */
BOOLEAN db_delindvar(INTBIG objaddr, INTBIG objtype, VARIABLE *var, INTBIG aindex)
{
	REGISTER INTBIG i, len, truelen, oldvalue, type;
	REGISTER INTBIG datasize;
	REGISTER void *loc;

	/* if the variable or key is not valid then ignore this */
	if (var == NOVARIABLE) return(TRUE);

	/* if the variable is not an array, quit now */
	type = var->type;
	if ((type&VISARRAY) == 0) return(TRUE);

	/* ensure that the index is within the range of the array */
	truelen = len = getlength(var);

	/* can only delete valid line number */
	if (aindex < 0 || aindex >= len) return(TRUE);

	/* cannot delete last entry */
	if (truelen == 1) return(TRUE);

	/* if this is a variable-length array, include the terminator */
	if ((var->type&VLENGTH) == 0) len++;

	/* determine the size of the array entries */
	datasize = db_getdatasize(type);

	/* get the address of the old array entry */
	loc = &((CHAR *)var->addr)[aindex*datasize];
	oldvalue = db_assignvalue(loc, datasize);

	/* shift the data */
	for(i=aindex; i<len-1; i++)
	{
		switch (type&VTYPE)
		{
			case VCHAR:
				((CHAR *)var->addr)[i] = ((CHAR *)var->addr)[i+1];
				break;
			case VFLOAT:
				((float *)var->addr)[i] = ((float *)var->addr)[i+1];
				break;
			case VDOUBLE:
				((double *)var->addr)[i] = ((double *)var->addr)[i+1];
				break;
			case VSHORT:
				((INTSML *)var->addr)[i] = ((INTSML *)var->addr)[i+1];
				break;
			default:
				((INTBIG *)var->addr)[i] = ((INTBIG *)var->addr)[i+1];
				break;
		}
	}

	/* decrement the array size */
	if ((var->type&VLENGTH) != 0)
		var->type = (var->type & ~VLENGTH) | ((truelen-1) << VLENGTHSH);

	/* handle change control, constraint, and broadcast */
	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		/* tell constraint system about deleted variable */
		(*el_curconstraint->deletevariable)(objaddr, objtype, var->key, aindex, oldvalue);

		/* mark a change */
		(void)db_change((INTBIG)objaddr, VARIABLEDEL, objtype, var->key, type,
			aindex, oldvalue, 0);
	}

	return(FALSE);
}

/*
 * routine to set an individual variable entry at address "to" to the value
 * at address "from".  This type is of type "type" and if allocatable, is
 * filled from cluster "cluster".
 */
#ifdef DEBUGMEMORY
BOOLEAN db_setval(UCHAR1 *from, UCHAR1 *to, INTBIG type, CLUSTER *cluster,
	CHAR *module, INTBIG line)
#else
BOOLEAN db_setval(UCHAR1 *from, UCHAR1 *to, INTBIG type, CLUSTER *cluster)
#endif
{
	/* character sized variable */
	if ((type&VTYPE) == VCHAR || (type&VTYPE) == VBOOLEAN)
	{
		*to = *from;
		return(FALSE);
	}

	/* double sized variable */
	if ((type&VTYPE) == VDOUBLE)
	{
		((double *)to)[0] = ((double *)from)[0];
		return(FALSE);
	}

	/* float sized variable */
	if ((type&VTYPE) == VFLOAT)
	{
		((float *)to)[0] = ((float *)from)[0];
		return(FALSE);
	}

	/* short sized variable */
	if ((type&VTYPE) == VSHORT)
	{
		((INTSML *)to)[0] = (INTSML)(((INTBIG *)from)[0]);
		return(FALSE);
	}

	/* integer sized variable */
	if ((type&(VCODE1|VCODE2)) == 0 && (type&VTYPE) != VSTRING)
	{
		((INTBIG *)to)[0] = ((INTBIG *)from)[0];
		return(FALSE);
	}

	/* string or code variable */
#ifdef DEBUGMEMORY
	return(_allocstring((CHAR **)to, ((CHAR **)from)[0], cluster, module, line));
#else
	return(allocstring((CHAR **)to, ((CHAR **)from)[0], cluster));
#endif
}

/*
 * routine to erase a list of variables when the object is deleted
 */
void db_freevars(VARIABLE **firstvar, INTSML *numvar)
{
	REGISTER INTBIG i;
	REGISTER VARIABLE *var;
	if (*numvar == 0) return;
	for(i = 0; i < *numvar; i++)
	{
		var = &(*firstvar)[i];
		db_freevar(var->addr, var->type);
	}
	efree((CHAR *)*firstvar);
	*numvar = 0;
}

/*
 * routine to free any memory allocated to the variable whose address is "addr"
 * and whose type is "type"
 */
void db_freevar(INTBIG addr, INTBIG type)
{
	REGISTER INTBIG i, len;

	/* determine whether individual elements are allocated */
	if ((type&(VCODE1|VCODE2)) != 0 || (type&VTYPE) == VSTRING)
	{
		/* the old variable was allocated */
		if ((type&VISARRAY) != 0)
		{
			len = (type&VLENGTH) >> VLENGTHSH;
			if (len == 0)
			{
				for(i=0; ((INTBIG *)addr)[i] != -1; i++)
					efree((CHAR *)((INTBIG *)addr)[i]);
			} else for(i=0; i<len; i++) efree((CHAR *)((INTBIG *)addr)[i]);
		} else efree((CHAR *)addr);
	}
	if ((type&VISARRAY) != 0) efree((CHAR *)addr);
}

/*
 * routine to do a binary search on the variables in "numvar" and "firstvar"
 * for an entry with the key "key".  If found, its index is returned,
 * otherwise a negative value is returned that, when negated, is the position
 * in the list BEFORE which the key should be inserted (i.e. the value -1
 * means that the key belongs in the first entry and the value -3 means that
 * the key belongs after the second entry).
 */
INTBIG db_binarysearch(INTSML numvar, VARIABLE *firstvar, INTBIG key)
{
	REGISTER INTBIG hi, lo, med, lastmed;
	REGISTER INTBIG i=key;

	if (numvar == 0) return(-1);
	lo = 0;   hi = lastmed = numvar;
	for(;;)
	{
		/* find mid-point: quit if done */
		med = (hi + lo) / 2;
		if (med == lastmed) break;
		lastmed = med;

		/* test the entry: return the value if a match */
		i = firstvar[med].key;
		if (i == key) return(med);

		/* decide which way to search in list */
		if (i > key) hi = med; else lo = med;
	}
	if (i < key) med++;
	return(-med-1);
}

void db_renamevar(INTSML numvar, VARIABLE *firstvar, INTBIG oldkey,
	INTBIG newkey)
{
	REGISTER INTBIG i, j, k;
	REGISTER INTBIG oldaddr, oldtype;
	UINTBIG olddescript[TEXTDESCRIPTSIZE];
	REGISTER VARIABLE *var;

	if (numvar == 0) return;

	for(i=0; i<numvar; i++)
	{
		var = &firstvar[i];
		if (var->key != oldkey) continue;

		/* save the old information about this variable */
		oldtype = var->type;
		TDCOPY(olddescript, var->textdescript);
		oldaddr = var->addr;

		/* compact out the old key */
		for(j=i+1; j<numvar; j++)
		{
			firstvar[j-1].key  = firstvar[j].key;
			firstvar[j-1].type = firstvar[j].type;
			TDCOPY(firstvar[j-1].textdescript, firstvar[j].textdescript);
			firstvar[j-1].addr = firstvar[j].addr;
		}

		/* now insert the new key */
		for(j=0; j<numvar-1; j++) if (newkey < firstvar[j].key) break;
		for(k=numvar-1; k>j; k--)
		{
			firstvar[k].key  = firstvar[k-1].key;
			firstvar[k].type = firstvar[k-1].type;
			TDCOPY(firstvar[k].textdescript, firstvar[k-1].textdescript);
			firstvar[k].addr = firstvar[k-1].addr;
		}

		/* insert the renamed variable in the right place */
		firstvar[j].key = newkey;
		firstvar[j].type = oldtype;
		TDCOPY(firstvar[j].textdescript, olddescript);
		firstvar[j].addr = oldaddr;
	}
}

/*
 * routine to fill the parameters "fir" and "num" with the "firstvar" and
 * "numvar" attributes on object "addr", type "type".  Returns false if
 * successful, true if the object has no "firstvar/numvar".
 */
BOOLEAN db_getvarptr(INTBIG addr, INTBIG type, VARIABLE ***fir, INTSML **num)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER PORTPROTO *pp;
	REGISTER ARCINST *ai;
	REGISTER ARCPROTO *ap;
	REGISTER GEOM *g;
	REGISTER LIBRARY *lib;
	REGISTER TECHNOLOGY *tech;
	REGISTER TOOL *tool;
	REGISTER RTNODE *rtn;
	REGISTER NETWORK *net;
	REGISTER VIEW *v;

	switch (type&VTYPE)
	{
		case VNODEINST:
			ni = (NODEINST *)addr;
			*fir = &ni->firstvar;    *num = &ni->numvar;    break;
		case VNODEPROTO:
			np = (NODEPROTO *)addr;
			*fir = &np->firstvar;    *num = &np->numvar;    break;
		case VPORTARCINST:
			pi = (PORTARCINST *)addr;
			*fir = &pi->firstvar;    *num = &pi->numvar;    break;
		case VPORTEXPINST:
			pe = (PORTEXPINST *)addr;
			*fir = &pe->firstvar;    *num = &pe->numvar;    break;
		case VPORTPROTO:
			pp = (PORTPROTO *)addr;
			*fir = &pp->firstvar;    *num = &pp->numvar;    break;
		case VARCINST:
			ai = (ARCINST *)addr;
			*fir = &ai->firstvar;    *num = &ai->numvar;    break;
		case VARCPROTO:
			ap = (ARCPROTO *)addr;
			*fir = &ap->firstvar;    *num = &ap->numvar;    break;
		case VGEOM:
			g = (GEOM *)addr;
			*fir = &g->firstvar;     *num = &g->numvar;     break;
		case VLIBRARY:
			lib = (LIBRARY *)addr;
			*fir = &lib->firstvar;   *num = &lib->numvar;   break;
		case VTECHNOLOGY:
			tech = (TECHNOLOGY *)addr;
			*fir = &tech->firstvar;  *num = &tech->numvar;  break;
		case VTOOL:
			tool = (TOOL *)addr;
			*fir = &tool->firstvar;  *num = &tool->numvar;  break;
		case VRTNODE:
			rtn = (RTNODE *)addr;
			*fir = &rtn->firstvar;   *num = &rtn->numvar;   break;
		case VNETWORK:
			net = (NETWORK *)addr;
			*fir = &net->firstvar;   *num = &net->numvar;   break;
		case VVIEW:
			v = (VIEW *)addr;
			*fir = &v->firstvar;     *num = &v->numvar;     break;
		default: return(TRUE);
	}
	return(FALSE);
}

INTBIG db_getdatasize(INTBIG type)
{
	if ((type&VTYPE) == VCHAR || (type&VTYPE) == VBOOLEAN) return(SIZEOFCHAR);
	if ((type&VTYPE) == VSHORT) return(SIZEOFINTSML);
	if ((type&VTYPE) == VFLOAT) return(sizeof(float));
	if ((type&VTYPE) == VDOUBLE) return(sizeof(double));
	return(SIZEOFINTBIG);
}

INTBIG db_assignvalue(void *loc, INTBIG datasize)
{
	if (datasize == SIZEOFINTBIG)   return(*((INTBIG *)loc));
	if (datasize == SIZEOFINTSML)   return(*((INTSML *)loc));
	if (datasize == SIZEOFCHAR)     return(*((CHAR *)loc));
	if (datasize == sizeof(float))  return(castint(*((float *)loc)));
	if (datasize == sizeof(double)) return(castint((float)(*((double *)loc))));
	return(*((INTBIG *)loc));
}

CHAR *db_describetype(INTBIG type)
{
	REGISTER void *infstr;

	infstr = initinfstr();
	switch (type&VTYPE)
	{
		case VINTEGER:     addstringtoinfstr(infstr, _("Integer"));       break;
		case VADDRESS:     addstringtoinfstr(infstr, _("Address"));       break;
		case VCHAR:        addstringtoinfstr(infstr, _("Character"));     break;
		case VSTRING:      addstringtoinfstr(infstr, _("String"));        break;
		case VFLOAT:       addstringtoinfstr(infstr, _("Float"));         break;
		case VDOUBLE:      addstringtoinfstr(infstr, _("Double"));        break;
		case VNODEINST:    addstringtoinfstr(infstr, _("NodeInst"));      break;
		case VNODEPROTO:   addstringtoinfstr(infstr, _("NodeProto"));     break;
		case VPORTARCINST: addstringtoinfstr(infstr, _("PortArcInst"));   break;
		case VPORTEXPINST: addstringtoinfstr(infstr, _("PortExpInst"));   break;
		case VPORTPROTO:   addstringtoinfstr(infstr, _("PortProto"));     break;
		case VARCINST:     addstringtoinfstr(infstr, _("ArcInst"));       break;
		case VARCPROTO:    addstringtoinfstr(infstr, _("ArcProto"));      break;
		case VGEOM:        addstringtoinfstr(infstr, _("Geom"));          break;
		case VLIBRARY:     addstringtoinfstr(infstr, _("Library"));       break;
		case VTECHNOLOGY:  addstringtoinfstr(infstr, _("Technology"));    break;
		case VTOOL:        addstringtoinfstr(infstr, _("Tool"));          break;
		case VRTNODE:      addstringtoinfstr(infstr, _("RTNode"));        break;
		case VFRACT:       addstringtoinfstr(infstr, _("Fixed-Point"));   break;
		case VNETWORK:     addstringtoinfstr(infstr, _("Network"));       break;
		case VVIEW:        addstringtoinfstr(infstr, _("View"));          break;
		case VWINDOWPART:  addstringtoinfstr(infstr, _("Window"));        break;
		case VGRAPHICS:    addstringtoinfstr(infstr, _("Graphics"));      break;
		case VSHORT:       addstringtoinfstr(infstr, _("Short"));         break;
		case VBOOLEAN:     addstringtoinfstr(infstr, _("Boolean"));       break;
		case VCONSTRAINT:  addstringtoinfstr(infstr, _("Constraint"));    break;
		case VWINDOWFRAME: addstringtoinfstr(infstr, _("WindowFrame"));   break;
		case VGENERAL:     addstringtoinfstr(infstr, _("General"));       break;
		default:           addstringtoinfstr(infstr, _("Unknown"));       break;
	}
	if ((type&VISARRAY) != 0) addstringtoinfstr(infstr, _(" array"));
	return(returninfstr(infstr));
}
