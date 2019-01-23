/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dbchange.c
 * Database change manager
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
#include "edialogs.h"
#include "sim.h"
#include "usr.h"

static INTBIG       db_batchtally;				/* number of batches in list */
static INTBIG       db_maximumbatches;			/* limit of number of batches */
static INTBIG       db_batchnumber;				/* batch number counter */
static CHANGEBATCH *db_currentbatch;			/* current batch of changes */
static CHANGEBATCH *db_headbatch;				/* most recent change in the list */
static CHANGEBATCH *db_tailbatch;				/* oldest change in the list */
static CHANGEBATCH *db_donebatch;				/* last done batch in the list */
static CHANGEBATCH *db_changebatchfree;			/* head of free change batches */
static CHANGE      *db_changefree;				/* head of free change modules */
static CHANGECELL  *db_changecellfree;			/* head of free change cell modules */
static TOOL        *db_currenttool;				/* current tool being given a slice */
       BOOLEAN      db_donextchangequietly = FALSE;	/* true to do next change quietly */
       BOOLEAN      db_dochangesquietly = FALSE;/* true to do changes quietly */
       UINTBIG      db_changetimestamp = 0;		/* timestamp for changes to database */
       INTBIG       db_broadcasting = 0;		/* nonzero if broadcasting */

/* prototypes for local routines */
static CHANGE      *db_allocchange(void);
static void         db_freechange(CHANGE*);
static CHANGECELL  *db_allocchangecell(void);
static CHANGEBATCH *db_allocchangebatch(void);
static void         db_freechangebatch(CHANGEBATCH*);
static void         db_killbatch(CHANGEBATCH*);
static CHANGEBATCH *db_preparenewbatch(void);
static void         db_erasebatch(CHANGEBATCH*);
static void         db_freechangecell(CHANGECELL*);
static void         db_loadhistorylist(INTBIG, void*);
static void         db_broadcastchange(CHANGE*, BOOLEAN, BOOLEAN);
static BOOLEAN      db_reversechange(CHANGE*);
static CHAR        *db_describechange(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static BOOLEAN      db_majorvariable(INTBIG type, INTBIG key);
static void         db_setdirty(INTBIG change, INTBIG obj, INTBIG a1, INTBIG a2, INTBIG a3, INTBIG a4);

/*
 * Routine to free all memory associated with this module.
 */
void db_freechangememory(void)
{
	REGISTER CHANGE *c;
	REGISTER CHANGECELL *cf;
	REGISTER CHANGEBATCH *b;

	while (db_changefree != NOCHANGE)
	{
		c = db_changefree;
		db_changefree = db_changefree->nextchange;
		efree((CHAR *)c);
	}
	while (db_changecellfree != NOCHANGECELL)
	{
		cf = db_changecellfree;
		db_changecellfree = db_changecellfree->nextchangecell;
		efree((CHAR *)cf);
	}
	while (db_changebatchfree != NOCHANGEBATCH)
	{
		b = db_changebatchfree;
		db_changebatchfree = db_changebatchfree->nextchangebatch;
		efree((CHAR *)b);
	}
}

/************************* CHANGE CONTROL *************************/

/*
 * Routine to initialize the change control system.
 */
void db_initializechanges(void)
{
	/* initialize change lists */
	db_changefree = NOCHANGE;
	db_changecellfree = NOCHANGECELL;
	db_changebatchfree = NOCHANGEBATCH;

	/* initialize changes */
	db_batchtally = 0;
	db_batchnumber = 1;
	db_maximumbatches = maxi(el_maxtools + el_maxtools/2 + 1, 20);
	db_currentbatch = db_headbatch = db_tailbatch = db_donebatch = NOCHANGEBATCH;
}

/*
 * Routine to record and broadcast a change.  The change is to object "obj" and the type of
 * change is in "change".  The arguments to the change depend on the type, and are in "a1"
 * through "a6".  Returns NOCHANGE upon error
 */
CHANGE *db_change(INTBIG obj, INTBIG change, INTBIG a1, INTBIG a2, INTBIG a3, INTBIG a4,
	INTBIG a5, INTBIG a6)
{
	REGISTER BOOLEAN firstchange;
	REGISTER CHANGE *c;
	static CHAR *broadcasttype[] = {M_("nodeinstnew"), M_("nodeinstkill"), M_("nodeinstmod"),
		M_("arcinstnew"), M_("arcinstkill"), M_("arcinstmod"), M_("portprotonew"),
		M_("portprotokill"), M_("portprotomod"), M_("nodeprotonew"), M_("nodeprotokill"),
		M_("nodeprotomod"), M_("objectstart"), M_("objectend"), M_("objectnew"), M_("objectkill"),
		M_("variablenew"), M_("variablekill"), M_("variablemod"), M_("variableinsert"),
		M_("variabledelete"), M_("descriptmod")};

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	if (db_broadcasting != 0)
	{
		ttyputerr(_("Recieved this change during broadcast of type %s:"),
			TRANSLATE(broadcasttype[db_broadcasting-1]));
		ttyputmsg(x_("%s"), db_describechange(change, obj, a1, a2, a3, a4, a5, a6));
	}

	/* determine what needs to be marked as changed */
	db_setdirty(change, obj, a1, a2, a3, a4);

	/* get change module */
	c = db_allocchange();
	if (c == NOCHANGE)
	{
		ttyputnomemory();
		return(NOCHANGE);
	}

	/* insert new change module into linked list */
	firstchange = FALSE;
	if (db_currentbatch == NOCHANGEBATCH)
	{
		db_currentbatch = db_preparenewbatch();
		if (db_currentbatch == NOCHANGEBATCH)
		{
			ttyputnomemory();
			return(NOCHANGE);
		}
		firstchange = TRUE;
		db_currentbatch->changehead = c;
		c->prevchange = NOCHANGE;
	} else
	{
		c->prevchange = db_currentbatch->changetail;
		db_currentbatch->changetail->nextchange = c;
	}
	db_currentbatch->changetail = c;
	c->nextchange = NOCHANGE;

	/* save the change information */
	c->changetype = change;     c->entryaddr = obj;
	c->p1 = a1;   c->p2 = a2;   c->p3 = a3;
	c->p4 = a4;   c->p5 = a5;   c->p6 = a6;

	/* broadcast the change */
	db_broadcastchange(c, firstchange, FALSE);
	return(c);
}

void db_setdirty(INTBIG change, INTBIG obj, INTBIG a1, INTBIG a2, INTBIG a3, INTBIG a4)
{
	REGISTER INTBIG i, major;
	REGISTER PORTPROTO *pp;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER LIBRARY *lib;

	np = NONODEPROTO;
	lib = NOLIBRARY;
	major = 0;
	switch (change)
	{
		case NODEINSTNEW:
		case NODEINSTKILL:
		case NODEINSTMOD:
			ni = (NODEINST *)obj;
			np = ni->parent;
			lib = np->lib;
			major = 1;
			break;
		case ARCINSTNEW:
		case ARCINSTKILL:
		case ARCINSTMOD:
			ai = (ARCINST *)obj;
			np = ai->parent;
			lib = np->lib;
			major = 1;
			break;
		case PORTPROTONEW:
		case PORTPROTOKILL:
		case PORTPROTOMOD:
			pp = (PORTPROTO *)obj;
			np = pp->parent;
			lib = np->lib;
			major = 1;
			break;
		case NODEPROTONEW:
			np = (NODEPROTO *)obj;
			lib = np->lib;
			major = 1;
			break;
		case NODEPROTOKILL:
			major = 1;
			/* FALLTHROUGH */ 
		case NODEPROTOMOD:
			lib = ((NODEPROTO *)obj)->lib;
			break;
		case OBJECTNEW:
		case OBJECTKILL:
			np = db_whichnodeproto(obj, a1);
			lib = whichlibrary(obj, a1);
			break;
		case VARIABLENEW:
		case VARIABLEMOD:
		case VARIABLEINS:
		case VARIABLEDEL:
			if ((a3&VDONTSAVE) != 0) { lib = NOLIBRARY;   np = NONODEPROTO; } else
			{
				np = db_whichnodeproto(obj, a1);
				lib = whichlibrary(obj, a1);

				/* special cases that make the change "major" */
				if (db_majorvariable(a1, a2)) major = 1;
			}
			break;
		case VARIABLEKILL:
			if ((a4&VDONTSAVE) != 0) { lib = NOLIBRARY;   np = NONODEPROTO; } else
			{
				np = db_whichnodeproto(obj, a1);
				lib = whichlibrary(obj, a1);

				/* special cases that make the change "major" */
				if (db_majorvariable(a1, a2)) major = 1;
			}
			break;
		case DESCRIPTMOD:
			if ((a3&VDONTSAVE) != 0) { lib = NOLIBRARY;   np = NONODEPROTO; } else
			{
				np = db_whichnodeproto(obj, a1);
				lib = whichlibrary(obj, a1);
			}
			break;
	}

	/* set "changed" and "dirty" bits */
	if (np != NONODEPROTO)
	{
		if (major != 0) np->revisiondate = (UINTBIG)getcurrenttime();
		np->adirty = 0;
		for(i=0; i<el_maxtools; i++)
			if ((el_tools[i].toolstate & TOOLON) == 0) np->adirty |= 1 << i;
	}
	if (lib != NOLIBRARY)
	{
		if (major != 0) lib->userbits |= LIBCHANGEDMAJOR; else
			lib->userbits |= LIBCHANGEDMINOR;
	}
}

BOOLEAN db_majorvariable(INTBIG type, INTBIG key)
{
	switch (type)
	{
		case VNODEPROTO:
			/* when changing text in text-only cell */
			if (key == el_cell_message_key) return(TRUE);
			break;
		case VNODEINST:
			/* when changing node name */
			if (key == el_node_name_key || key == sim_weaknodekey) return(TRUE);
			break;
		case VARCINST:
			/* when changing arc name */
			if (key == el_arc_name_key) return(TRUE);
			break;
	}
	return(FALSE);
}

/*
 * Routine to request that the next change be made "quietly" (i.e. no constraints,
 * change control, or broadcast).
 */
void nextchangequiet(void)
{
	db_donextchangequietly = TRUE;
}

/*
 * Routine to set the nature of subsequent changes to be "quiet".  TRUE
 * means no constraints, change control, or broadcast are done.
 */
void changesquiet(BOOLEAN quiet)
{
	db_dochangesquietly = quiet;
}

/*
 * Routine to broadcast change "c" to all tools that are on.  If "firstchange" is nonzero,
 * this is the first change of a batch, and a "startbatch" change will also be broadcast.
 * "undoredo" is true if this is an undo/redo batch.
 */
void db_broadcastchange(CHANGE *c, BOOLEAN firstchange, BOOLEAN undoredo)
{
	REGISTER INTBIG i, oldbroadcasting;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	/* start the batch if this is the first change */
	oldbroadcasting = db_broadcasting;
	db_broadcasting = c->changetype+1;
	if (firstchange)
	{
		/* broadcast a start-batch on the first change */
		for(i=0; i<el_maxtools; i++)
			if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].startbatch != 0)
				(*el_tools[i].startbatch)(db_currentbatch->tool, undoredo);
	}
	switch (c->changetype)
	{
		case NODEINSTNEW:
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].newobject != 0)
					(*el_tools[i].newobject)(c->entryaddr, VNODEINST);
			break;
		case NODEINSTKILL:
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].killobject != 0)
					(*el_tools[i].killobject)(c->entryaddr, VNODEINST);
			break;
		case NODEINSTMOD:
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].modifynodeinst != 0)
					(*el_tools[i].modifynodeinst)((NODEINST *)c->entryaddr, c->p1, c->p2,
						c->p3, c->p4, c->p5, c->p6);
			break;
		case ARCINSTNEW:
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].newobject != 0)
					(*el_tools[i].newobject)(c->entryaddr, VARCINST);
			break;
		case ARCINSTKILL:
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].killobject != 0)
					(*el_tools[i].killobject)(c->entryaddr, VARCINST);
			break;
		case ARCINSTMOD:
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].modifyarcinst != 0)
					(*el_tools[i].modifyarcinst)((ARCINST *)c->entryaddr, c->p1, c->p2,
						c->p3, c->p4, c->p5, c->p6);
			break;
		case PORTPROTONEW:
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].newobject != 0)
					(*el_tools[i].newobject)(c->entryaddr, VPORTPROTO);
			break;
		case PORTPROTOKILL:
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].killobject != 0)
					(*el_tools[i].killobject)(c->entryaddr, VPORTPROTO);
			break;
		case PORTPROTOMOD:
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].modifyportproto != 0)
					(*el_tools[i].modifyportproto)((PORTPROTO *)c->entryaddr, (NODEINST *)c->p1,
						(PORTPROTO *)c->p2);
			break;
		case NODEPROTONEW:
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].newobject != 0)
					(*el_tools[i].newobject)(c->entryaddr, VNODEPROTO);
			break;
		case NODEPROTOKILL:
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].killobject != 0)
					(*el_tools[i].killobject)(c->entryaddr, VNODEPROTO);
			break;
		case NODEPROTOMOD:
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].modifynodeproto != 0)
					(*el_tools[i].modifynodeproto)((NODEPROTO *)c->entryaddr);
			break;
		case OBJECTSTART:
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].startobjectchange != 0)
					(*el_tools[i].startobjectchange)(c->entryaddr, c->p1);
			break;
		case OBJECTEND:
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].endobjectchange != 0)
					(*el_tools[i].endobjectchange)(c->entryaddr, c->p1);
			break;
		case OBJECTNEW:
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].newobject != 0)
					(*el_tools[i].newobject)(c->entryaddr, c->p1);
			break;
		case OBJECTKILL:
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].killobject != 0)
					(*el_tools[i].killobject)(c->entryaddr, c->p1);
			break;
		case VARIABLENEW:
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].newvariable != 0)
					(*el_tools[i].newvariable)(c->entryaddr, c->p1, c->p2, c->p3);
			break;
		case VARIABLEKILL:
			descript[0] = c->p5;
			descript[1] = c->p6;
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].killvariable != 0)
					(*el_tools[i].killvariable)(c->entryaddr, c->p1, c->p2, c->p3, c->p4, descript);
			break;
		case VARIABLEMOD:
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].modifyvariable != 0)
					(*el_tools[i].modifyvariable)(c->entryaddr, c->p1, c->p2, c->p3, c->p4, c->p5);
			break;
		case VARIABLEINS:
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].insertvariable != 0)
					(*el_tools[i].insertvariable)(c->entryaddr, c->p1, c->p2, c->p3, c->p4);
			break;
		case VARIABLEDEL:
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].deletevariable != 0)
					(*el_tools[i].deletevariable)(c->entryaddr, c->p1, c->p2, c->p3, c->p4, c->p5);
			break;
		case DESCRIPTMOD:
			descript[0] = c->p4;
			descript[1] = c->p5;
			for(i=0; i<el_maxtools; i++)
				if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].modifydescript != 0)
					(*el_tools[i].modifydescript)(c->entryaddr, c->p1, c->p2, descript);
			break;
	}
	db_broadcasting = oldbroadcasting;
}

/*
 * Routine to terminate a batch of changes by broadcasting the endbatch
 * and clearing the "change" words.
 */
void db_endbatch(void)
{
	REGISTER INTBIG i;

	/* if no changes were recorded, stop */
	if (db_currentbatch == NOCHANGEBATCH) return;

	/* changes made: apply final constraints to this batch of changes */
	(*el_curconstraint->solve)(NONODEPROTO);

	for(i=0; i<el_maxtools; i++)
		if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].endbatch != 0)
			(*el_tools[i].endbatch)();

	db_currentbatch = NOCHANGEBATCH;
}

/************************* BATCH CONTROL *************************/

/*
 * Routine to allocate and initialize a new batch of changes.
 * Returns NOCHANGEBATCH on error.
 */
CHANGEBATCH *db_preparenewbatch(void)
{
	REGISTER CHANGEBATCH *killbatch, *thisbatch;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	/* first kill off any undone batches */
	noredoallowed();

	/* allocate a new change batch */
	thisbatch = db_allocchangebatch();
	if (thisbatch == NOCHANGEBATCH) return(NOCHANGEBATCH);
	thisbatch->changehead = NOCHANGE;
	thisbatch->batchnumber = db_batchnumber++;
	thisbatch->done = TRUE;
	thisbatch->nextchangebatch = NOCHANGEBATCH;

	/* put at head of list */
	if (db_headbatch == NOCHANGEBATCH)
	{
		thisbatch->lastchangebatch = NOCHANGEBATCH;
		db_headbatch = db_tailbatch = thisbatch;
	} else
	{
		thisbatch->lastchangebatch = db_headbatch;
		db_headbatch->nextchangebatch = thisbatch;
		db_headbatch = thisbatch;
	}
	db_donebatch = thisbatch;

	/* kill last batch if list is full */
	db_batchtally++;
	if (db_batchtally > db_maximumbatches)
	{
		killbatch = db_tailbatch;
		db_tailbatch = db_tailbatch->nextchangebatch;
		db_tailbatch->lastchangebatch = NOCHANGEBATCH;
		db_killbatch(killbatch);
	}

	/* miscellaneous initialization */
	thisbatch->firstchangecell = NOCHANGECELL;
	thisbatch->tool = db_currenttool;
	thisbatch->activity = 0;
	return(thisbatch);
}

/*
 * Routine to completely delete change batch "batch".
 */
void db_killbatch(CHANGEBATCH *batch)
{
	db_erasebatch(batch);
	db_batchtally--;
	db_freechangebatch(batch);
}

/*
 * routine to erase the contents of change batch "batch"
 */
void db_erasebatch(CHANGEBATCH *batch)
{
	REGISTER CHANGE *c, *nextc;
	REGISTER CHANGECELL *cc, *nextcc;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
	REGISTER WINDOWPART *w;
	REGISTER EDITOR *e;

	/* erase the change information */
	for(c = batch->changehead; c != NOCHANGE; c = nextc)
	{
		nextc = c->nextchange;
		if (c->changetype == NODEINSTKILL)
		{
			/* now free the nodeinst */
			ni = (NODEINST *)c->entryaddr;
			freegeom(ni->geom);
			freenodeinst(ni);
		} else if (c->changetype == ARCINSTKILL)
		{
			ai = (ARCINST *)c->entryaddr;
			freeportarcinst(ai->end[0].portarcinst);
			freeportarcinst(ai->end[1].portarcinst);
			freegeom(ai->geom);
			freearcinst(ai);
		} else if (c->changetype == PORTPROTOKILL)
		{
			pp = (PORTPROTO *)c->entryaddr;
			efree(pp->protoname);
			freeportproto(pp);
		} else if (c->changetype == NODEPROTOKILL)
		{
			np = (NODEPROTO *)c->entryaddr;
			freenodeproto(np);
		} else if (c->changetype == VARIABLEKILL)
		{
			db_freevar(c->p3, c->p4);
		} else if (c->changetype == OBJECTKILL)
		{
			switch (c->p1&VTYPE)
			{
				case VVIEW:
					freeview((VIEW *)c->entryaddr);
					break;
				case VWINDOWPART:
					w = (WINDOWPART *)c->entryaddr;
					if ((w->state&WINDOWTYPE) == TEXTWINDOW || (w->state&WINDOWTYPE) == POPTEXTWINDOW)
					{
						e = w->editor;
						if ((w->state&WINDOWTYPE) == POPTEXTWINDOW)
							screenrestorebox(e->savedbox, -1);
						us_freeeditor(e);
					}
					freewindowpart(w);
					break;
			}
		} else if (c->changetype == VARIABLEDEL ||
			c->changetype == VARIABLEMOD)
		{
			/* free the array entry if it is allocated */
			if ((c->p3&(VCODE1|VCODE2)) != 0 || (c->p3&VTYPE) == VSTRING)
			{
				efree((CHAR *)c->p5);
			}
		}
		db_freechange(c);
	}
	batch->changehead = batch->changetail = NOCHANGE;

	/* erase the change cell information */
	for(cc = batch->firstchangecell; cc != NOCHANGECELL; cc = nextcc)
	{
		nextcc = cc->nextchangecell;
		db_freechangecell(cc);
	}
	batch->firstchangecell = NOCHANGECELL;

	/* erase the activity information */
	if (batch->activity != 0) efree(batch->activity);
	batch->activity = 0;
}

/*
 * Routine to prevent undo by deleting all batches.
 */
void noundoallowed(void)
{
	REGISTER CHANGEBATCH *batch, *prevbatch;

	/* properly terminate the current batch */
	db_endbatch();

	/* kill them all */
	for(batch = db_headbatch; batch != NOCHANGEBATCH; batch = prevbatch)
	{
		prevbatch = batch->lastchangebatch;
		db_killbatch(batch);
	}

	/* clear pointers */
	db_currentbatch = NOCHANGEBATCH;
	db_headbatch = NOCHANGEBATCH;
	db_tailbatch = NOCHANGEBATCH;
	db_donebatch = NOCHANGEBATCH;
}

/*
 * Routine to prevent redo by deleting all undone batches.
 */
void noredoallowed(void)
{
	REGISTER CHANGEBATCH *killbatch;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	while (db_headbatch != NOCHANGEBATCH && db_headbatch != db_donebatch)
	{
		killbatch = db_headbatch;
		db_headbatch = db_headbatch->lastchangebatch;
		if (db_headbatch != NOCHANGEBATCH) db_headbatch->nextchangebatch = NOCHANGEBATCH;
		db_killbatch(killbatch);
	}
}

/*
 * routine to set the size of the history list to "newsize".  The size of
 * the list is returned.  If "newsize" is not positive, the size is not
 * changed, just returned.
 */
INTBIG historylistsize(INTBIG newsize)
{
	REGISTER CHANGEBATCH *batch;
	REGISTER INTBIG oldsize;

	if (newsize <= 0) return(db_maximumbatches);

	oldsize = db_maximumbatches;
	db_maximumbatches = newsize;
	while (db_batchtally > db_maximumbatches)
	{
		batch = db_tailbatch;
		db_tailbatch = db_tailbatch->nextchangebatch;
		db_tailbatch->lastchangebatch = NOCHANGEBATCH;
		db_killbatch(batch);
	}
	return(oldsize);
}

/*
 * routine to set the tool for the current (or next) batch of changes
 */
void db_setcurrenttool(TOOL *tool)
{
	db_currenttool = tool;
}

/*
 * routine to get the current batch of changes
 */
CHANGEBATCH *db_getcurrentbatch(void)
{
	return(db_currentbatch);
}

/*
 * routine to get the identifying number for the current batch of changes
 */
INTBIG getcurrentbatchnumber(void)
{
	if (db_currentbatch == NOCHANGEBATCH) return(-1);
	return(db_currentbatch->batchnumber);
}

/*
 * routine to set the activity message for the current batch of changes
 */
void setactivity(CHAR *message)
{
	if (db_currentbatch == NOCHANGEBATCH) return;		/* should save for later !!! */
	if (db_currentbatch->activity != 0) efree(db_currentbatch->activity);
	(void)allocstring(&db_currentbatch->activity, message, db_cluster);
}

/************************* UNDO/REDO *************************/

/*
 * Routine to return the nature of the next possible undo/redo batch.
 * If "undo" is true, examine the next undo batch, otherwise the next redo batch.
 * Returns zero if there is no batch available.
 * Returns positive if there is a batch with major changes in it.
 * Returns negative if there is a batch with minor changes in it.
 */
INTBIG undonature(BOOLEAN undo)
{
	REGISTER CHANGEBATCH *batch;
	REGISTER CHANGE *c;
	REGISTER INTBIG retval;

	if (undo)
	{
		/* examine the next batch to be undone */
		if (db_donebatch == NOCHANGEBATCH) return(0);
		batch = db_donebatch;
	} else
	{
		/* examine the next batch to be redone */
		if (db_donebatch == NOCHANGEBATCH) return(0);
		batch = db_donebatch->nextchangebatch;
		if (batch == NOCHANGEBATCH) return(0);
	}

	/* determine the nature of the batch */
	retval = -1;
	for(c = batch->changetail; c != NOCHANGE; c = c->prevchange)
	{
		switch (c->changetype)
		{
			case NODEINSTNEW:
			case NODEINSTKILL:
			case NODEINSTMOD:
			case ARCINSTNEW:
			case ARCINSTKILL:
			case ARCINSTMOD:
			case PORTPROTONEW:
			case PORTPROTOKILL:
			case PORTPROTOMOD:
			case NODEPROTONEW:
			case NODEPROTOKILL:
			case NODEPROTOMOD:
			case DESCRIPTMOD:
				retval = 1;
				break;
			case VARIABLENEW:
				if ((c->p3&VCREF) != 0) retval = 1;
				break;
		}
	}
	return(retval);
}

/*
 * routine to undo backward through the list of change batches.  If there is no batch to
 * undo, the routine returns zero.  If there is a batch, it is undone and the
 * routine returns the batch number (a positive value) if the batch involved changes to
 * nodes/arcs/ports/cells or the negative batch number if the batch only involves minor
 * changes (variables, etc.).  The tool that produced the batch is returned in "tool".
 */
INTBIG undoabatch(TOOL **tool)
{
	REGISTER CHANGEBATCH *batch, *savebatch;
	REGISTER CHANGE *c;
	REGISTER INTBIG i, retval;
	REGISTER BOOLEAN firstchange;

	/* get the most recent batch of changes */
	if (db_donebatch == NOCHANGEBATCH) return(0);
	batch = db_donebatch;
	db_donebatch = db_donebatch->lastchangebatch;
	*tool = batch->tool;

	/* look through the changes in this batch */
	firstchange = TRUE;
	retval = -batch->batchnumber;
	savebatch = db_currentbatch;
	for(c = batch->changetail; c != NOCHANGE; c = c->prevchange)
	{
		/* reverse the change */
		if (db_reversechange(c))
			retval = batch->batchnumber;

		/* now broadcast this change */
		db_currentbatch = batch;
		db_broadcastchange(c, firstchange, TRUE);
		firstchange = FALSE;
	}

	/* broadcast the end-batch */
	for(i=0; i<el_maxtools; i++)
		if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].endbatch != 0)
			(*el_tools[i].endbatch)();
	db_currentbatch = savebatch;

	/* mark that this batch is undone */
	batch->done = FALSE;
	return(retval);
}

/*
 * routine to redo forward through the list of change batches.  If there is no batch to
 * redo, the routine returns zero.  If there is a batch, it is redone and the
 * routine returns the batch number (a positive value) if the batch involved changes to
 * nodes/arcs/ports/cells or the negative batch number if the batch only involves minor
 * changes (variables, etc.).  The tool that produced the batch is returned in "tool".
 */
INTBIG redoabatch(TOOL **tool)
{
	REGISTER CHANGEBATCH *batch, *savebatch;
	REGISTER CHANGE *c;
	REGISTER BOOLEAN firstchange;
	REGISTER INTBIG i, retval;

	/* get the most recent batch of changes */
	if (db_donebatch == NOCHANGEBATCH)
	{
		if (db_tailbatch == NOCHANGEBATCH) return(0);
		batch = db_tailbatch;
	} else
	{
		batch = db_donebatch->nextchangebatch;
		if (batch == NOCHANGEBATCH) return(0);
	}
	db_donebatch = batch;
	*tool = batch->tool;

	/* look through the changes in this batch */
	firstchange = TRUE;
	retval = -batch->batchnumber;
	savebatch = db_currentbatch;
	for(c = batch->changehead; c != NOCHANGE; c = c->nextchange)
	{
		/* reverse the change */
		if (db_reversechange(c))
			retval = batch->batchnumber;

		/* now broadcast this change */
		db_currentbatch = batch;
		db_broadcastchange(c, firstchange, TRUE);
		firstchange = FALSE;
	}

	/* broadcast the end-batch */
	for(i=0; i<el_maxtools; i++)
		if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].endbatch != 0)
			(*el_tools[i].endbatch)();
	db_currentbatch = savebatch;

	/* mark that this batch is redone */
	batch->done = TRUE;
	return(retval);
}

/*
 * Routine to undo the effects of change "c".
 * Returns true if the change is important.
 */
BOOLEAN db_reversechange(CHANGE *c)
{
	REGISTER PORTPROTO *pp, *oldsubpp;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni, *oldsubni;
	REGISTER ARCINST *ai;
	REGISTER VARIABLE *var;
	REGISTER VIEW *v, *lastv, *view;
	REGISTER WINDOWPART *w;
	REGISTER CHAR *attname;
	CHAR *storage;
	INTBIG oldval;
	REGISTER INTSML oldshort;

	/* determine what needs to be marked as changed */
	db_setdirty(c->changetype, c->entryaddr, c->p1, c->p2, c->p3, c->p4);

	switch (c->changetype)
	{
		case NODEINSTNEW:
			ni = (NODEINST *)c->entryaddr;
			db_retractnodeinst(ni);
			c->changetype = NODEINSTKILL;
			return(TRUE);
		case NODEINSTKILL:
			ni = (NODEINST *)c->entryaddr;
			db_enternodeinst(ni);
			c->changetype = NODEINSTNEW;
			return(TRUE);
		case NODEINSTMOD:
			ni = (NODEINST *)c->entryaddr;
			oldval = ni->lowx;       ni->lowx = c->p1;       c->p1 = oldval;
			oldval = ni->lowy;       ni->lowy = c->p2;       c->p2 = oldval;
			oldval = ni->highx;      ni->highx = c->p3;      c->p3 = oldval;
			oldval = ni->highy;      ni->highy = c->p4;      c->p4 = oldval;
			oldshort = ni->rotation;   ni->rotation = (INTSML)c->p5;   c->p5 = oldshort;
			oldshort = ni->transpose;  ni->transpose = (INTSML)c->p6;  c->p6 = oldshort;
			/* don't need to call "updategeom()" because it is done by "OBJECTSTART" */
			return(TRUE);
		case ARCINSTNEW:
			ai = (ARCINST *)c->entryaddr;
			db_retractarcinst(ai);
			c->changetype = ARCINSTKILL;
			return(TRUE);
		case ARCINSTKILL:
			ai = (ARCINST *)c->entryaddr;
			db_addportarcinst(ai->end[0].nodeinst, ai->end[0].portarcinst);
			db_addportarcinst(ai->end[1].nodeinst, ai->end[1].portarcinst);
			db_enterarcinst(ai);
			c->changetype = ARCINSTNEW;
			return(TRUE);
		case ARCINSTMOD:
			ai = (ARCINST *)c->entryaddr;
			oldval = ai->end[0].xpos;  ai->end[0].xpos = c->p1;   c->p1 = oldval;
			oldval = ai->end[0].ypos;  ai->end[0].ypos = c->p2;   c->p2 = oldval;
			oldval = ai->end[1].xpos;  ai->end[1].xpos = c->p3;   c->p3 = oldval;
			oldval = ai->end[1].ypos;  ai->end[1].ypos = c->p4;   c->p4 = oldval;
			oldval = ai->width;        ai->width = c->p5;         c->p5 = oldval;
			oldval = ai->length;       ai->length = c->p6;        c->p6 = oldval;
			determineangle(ai);
			(void)setshrinkvalue(ai, TRUE);
			/* don't need to call "updategeom()" because it is done by "OBJECTSTART" */
			return(TRUE);
		case PORTPROTONEW:
			pp = (PORTPROTO *)c->entryaddr;
			db_retractportproto(pp);
			c->changetype = PORTPROTOKILL;
			return(TRUE);
		case PORTPROTOKILL:
			pp = (PORTPROTO *)c->entryaddr;
			db_enterportproto(pp);
			c->changetype = PORTPROTONEW;
			return(TRUE);
		case PORTPROTOMOD:
			pp = (PORTPROTO *)c->entryaddr;
			oldsubni = pp->subnodeinst;
			oldsubpp = pp->subportproto;
			db_changeport(pp, (NODEINST *)c->p1, (PORTPROTO *)c->p2);
			c->p1 = (INTBIG)oldsubni;   c->p2 = (INTBIG)oldsubpp;
			return(TRUE);
		case NODEPROTONEW:
			np = (NODEPROTO *)c->entryaddr;
			db_retractnodeproto(np);
			c->changetype = NODEPROTOKILL;
			return(TRUE);
		case NODEPROTOKILL:
			np = (NODEPROTO *)c->entryaddr;
			db_insertnodeproto(np);
			c->changetype = NODEPROTONEW;
			return(TRUE);
		case NODEPROTOMOD:
			np = (NODEPROTO *)c->entryaddr;
			oldval = np->lowx;    np->lowx = c->p1;    c->p1 = oldval;
			oldval = np->highx;   np->highx = c->p2;   c->p2 = oldval;
			oldval = np->lowy;    np->lowy = c->p3;    c->p3 = oldval;
			oldval = np->highy;   np->highy = c->p4;   c->p4 = oldval;
			return(TRUE);
		case OBJECTSTART:
			c->changetype = OBJECTEND;
			if (c->p1 == VNODEINST)
			{
				ni = (NODEINST *)c->entryaddr;
				updategeom(ni->geom, ni->parent);
			} else if (c->p1 == VARCINST)
			{
				ai = (ARCINST *)c->entryaddr;
				updategeom(ai->geom, ai->parent);
			}
			break;
		case OBJECTEND:
			c->changetype = OBJECTSTART;
			break;
		case OBJECTNEW:
			/* args: addr, type */
			switch (c->p1&VTYPE)
			{
				case VVIEW:
					/* find the view */
					view = (VIEW *)c->entryaddr;
					lastv = NOVIEW;
					for(v = el_views; v != NOVIEW; v = v->nextview)
					{
						if (v == view) break;
						lastv = v;
					}
					if (v != NOVIEW)
					{
						/* delete the view */
						if (lastv == NOVIEW) el_views = v->nextview; else
							lastv->nextview = v->nextview;
					}
					break;
				case VWINDOWPART:
					w = (WINDOWPART *)c->entryaddr;
					db_retractwindowpart(w);
					break;
			}
			c->changetype = OBJECTKILL;
			break;
		case OBJECTKILL:
			/* args: addr, type */
			switch (c->p1&VTYPE)
			{
				case VVIEW:
					view = (VIEW *)c->entryaddr;
					view->nextview = el_views;
					el_views = view;
					break;
				case VWINDOWPART:
					w = (WINDOWPART *)c->entryaddr;
					(void)db_enterwindowpart(w);
					break;
			}
			c->changetype = OBJECTNEW;
			break;
		case VARIABLENEW:
			/* args: addr, type, key, newtype, newdescript[0], newdescript[1] */
			if ((c->p3&VCREF) != 0)
			{
				/* change to fixed attribute */
				attname = changedvariablename(c->p1, c->p2, c->p3);
				var = getvalnoeval(c->entryaddr, c->p1, c->p3 & (VTYPE|VISARRAY), attname);
				if (var == NOVARIABLE)
				{
					ttyputmsg(_("Warning: Could not find attribute %s on object %s"),
						attname, describeobject(c->entryaddr, c->p1));
					break;
				}
				c->p3 = var->addr;
				c->p4 = var->type;
				c->p5 = var->textdescript[0];
				c->p6 = var->textdescript[1];
				c->changetype = VARIABLEKILL;
				if (c->p1 == VNODEINST || c->p1 == VARCINST) return(TRUE);
				if (c->p1 == VPORTPROTO)
				{
					if (estrcmp(attname, x_("textdescript")) == 0 ||
						estrcmp(attname, x_("userbits")) == 0 ||
						estrcmp(attname, x_("protoname")) == 0) return(TRUE);
				}
				break;
			}

			/* change to variable attribute: get current value */
			var = getvalkeynoeval(c->entryaddr, c->p1, c->p3&(VTYPE|VISARRAY), c->p2);
			if (var == NOVARIABLE)
			{
				ttyputmsg(_("Warning: Could not find attribute %s on object %s"),
					makename(c->p2), describeobject(c->entryaddr, c->p1));
				break;
			}
			c->p3 = var->addr;
			c->p4 = var->type;
			c->p5 = var->textdescript[0];
			c->p6 = var->textdescript[1];
			c->changetype = VARIABLEKILL;
			var->type = VINTEGER;		/* fake it out so no memory is deallocated */
			nextchangequiet();
			(void)delvalkey(c->entryaddr, c->p1, c->p2);
			if (c->p1 == VNODEINST || c->p1 == VARCINST) return(TRUE);
			break;
		case VARIABLEKILL:
			/* args: addr, type, key, oldaddr, oldtype, olddescript */
			if ((c->p4&VCREF) != 0)
			{
				attname = changedvariablename(c->p1, c->p2, c->p4);
				nextchangequiet();
				var = setval(c->entryaddr, c->p1, attname, c->p3, c->p4);
				if (var == NOVARIABLE)
				{
					ttyputmsg(_("Warning: Could not set attribute %s on object %s"),
						attname, describeobject(c->entryaddr, c->p1));
					break;
				}
			} else
			{
				nextchangequiet();
				var = setvalkey(c->entryaddr, c->p1, c->p2, c->p3, c->p4);
				if (var == NOVARIABLE)
				{
					ttyputmsg(_("Warning: Could not set attribute %s on object %s"),
						makename(c->p2), describeobject(c->entryaddr, c->p1));
					break;
				}
				var->textdescript[0] = c->p5;
				var->textdescript[1] = c->p6;
			}
			db_freevar(c->p3, c->p4);
			c->p3 = c->p4;
			c->changetype = VARIABLENEW;
			if (c->p1 == VNODEINST || c->p1 == VARCINST) return(TRUE);
			break;
		case VARIABLEMOD:
			/* args: addr, type, key, vartype, aindex, oldvalue */
			if ((c->p3&VCREF) != 0)
			{
				attname = changedvariablename(c->p1, c->p2, c->p3);
				if (getind(c->entryaddr, c->p1, attname, c->p4, &oldval))
				{
					ttyputmsg(_("Warning: Could not find index %ld of attribute %s on object %s"),
						c->p4, attname, describeobject(c->entryaddr, c->p1));
					break;
				}
				nextchangequiet();
				(void)setind(c->entryaddr, c->p1, attname, c->p4, c->p5);
				c->p5 = oldval;
				if (c->p1 == VPORTPROTO)
				{
					if (estrcmp(attname, x_("textdescript")) == 0) return(TRUE);
				}
			} else
			{
				if (getindkey(c->entryaddr, c->p1, c->p2, c->p4, &oldval))
				{
					ttyputmsg(_("Warning: Could not find index %ld of attribute %s on object %s"),
						c->p4, makename(c->p2), describeobject(c->entryaddr, c->p1));
					break;
				}
				if ((c->p3&(VCODE1|VCODE2)) != 0 || (c->p3&VTYPE) == VSTRING)
				{
					/* because this change is done quietly, the memory is freed and must be saved */
					(void)allocstring(&storage, (CHAR *)oldval, db_cluster);
					oldval = (INTBIG)storage;
				}
				nextchangequiet();
				(void)setindkey(c->entryaddr, c->p1, c->p2, c->p4, c->p5);
				c->p5 = oldval;
			}
			if (c->p1 == VNODEINST || c->p1 == VARCINST) return(TRUE);
			break;
		case VARIABLEINS:
			/* args: addr, type, key, vartype, aindex */
			if (getindkey(c->entryaddr, c->p1, c->p2, c->p4, &oldval))
			{
				ttyputmsg(_("Warning: Could not find index %ld of attribute %s on object %s"),
					c->p4, makename(c->p2), describeobject(c->entryaddr, c->p1));
				break;
			}
			nextchangequiet();
			(void)delindkey(c->entryaddr, c->p1, c->p2, c->p4);
			c->changetype = VARIABLEDEL;
			c->p5 = oldval;
			if (c->p1 == VNODEINST || c->p1 == VARCINST) return(TRUE);
			break;
		case VARIABLEDEL:
			/* args: addr, type, key, vartype, aindex, oldvalue */
			nextchangequiet();
			(void)insindkey(c->entryaddr, c->p1, c->p2, c->p4, c->p5);
			c->changetype = VARIABLEINS;
			if (c->p1 == VNODEINST || c->p1 == VARCINST) return(TRUE);
			break;
		case DESCRIPTMOD:
			var = getvalkeynoeval(c->entryaddr, c->p1, -1, c->p2);
			if (var == NOVARIABLE) break;
			oldval = var->textdescript[0];   var->textdescript[0] = c->p4;   c->p4 = oldval;
			oldval = var->textdescript[1];   var->textdescript[1] = c->p5;   c->p5 = oldval;
			return(TRUE);
	}
	return(FALSE);
}

/************************* CHANGE CELLS *************************/

/*
 * routine to add cell "cell" to the list of cells that are being
 * changed in this batch.
 */
void db_setchangecell(NODEPROTO *cell)
{
	REGISTER CHANGECELL *cc;

	if (db_currentbatch == NOCHANGEBATCH) return;
	if (db_currentbatch->firstchangecell == NOCHANGECELL ||
		db_currentbatch->firstchangecell->changecell != cell)
	{
		cc = db_allocchangecell();
		cc->nextchangecell = db_currentbatch->firstchangecell;
		db_currentbatch->firstchangecell = cc;
		cc->changecell = cell;
		cc->forcedlook = FALSE;
	}
}

void db_removechangecell(NODEPROTO *np)
{
	REGISTER CHANGECELL *cc, *lastcc, *nextcc;

	if (db_currentbatch == NOCHANGEBATCH) return;
	lastcc = NOCHANGECELL;
	for(cc = db_currentbatch->firstchangecell; cc != NOCHANGECELL; cc = nextcc)
	{
		nextcc = cc->nextchangecell;
		if (cc->changecell == np)
		{
			if (lastcc == NOCHANGECELL) db_currentbatch->firstchangecell = nextcc; else
				lastcc->nextchangecell = nextcc;
			db_freechangecell(cc);
			return;
		}
		lastcc = cc;
	}
}

/*
 * Routine to ensure that cell "np" is given a hierarchical analysis by the
 * constraint system.
 */
void db_forcehierarchicalanalysis(NODEPROTO *np)
{
	REGISTER CHANGECELL *cc;

	if (db_currentbatch == NOCHANGEBATCH) return;
	for(cc = db_currentbatch->firstchangecell; cc != NOCHANGECELL; cc = cc->nextchangecell)
		if (cc->changecell == np)
	{
		cc->forcedlook = TRUE;
		return;
	}

	/* if not in the list, create the entry and try again */
	db_setchangecell(np);
	db_forcehierarchicalanalysis(np);
}

/************************* USER INTERFACE *************************/

/*
 * routine to display history list entry "which" (if "which" is negative,
 * display the entire list
 */
void showhistorylist(INTBIG which)
{
	REGISTER INTBIG node, arcinst, portproto, nproto, object, variable;
	REGISTER INTBIG lobatch, hibatch;
	REGISTER CHANGEBATCH *batchreport;
	REGISTER CHANGE *c;
	CHAR line[50];
	REGISTER void *infstr;

	/* specific display of a batch */
	if (which >= 0)
	{
		lobatch = hibatch = db_tailbatch->batchnumber;
		for(batchreport = db_tailbatch; batchreport != NOCHANGEBATCH;
			batchreport = batchreport->nextchangebatch)
		{
			if (batchreport->batchnumber < lobatch) lobatch = batchreport->batchnumber;
			if (batchreport->batchnumber > hibatch) hibatch = batchreport->batchnumber;
			if (batchreport->batchnumber != which) continue;
			infstr = initinfstr();
			formatinfstr(infstr, M_("Batch %ld from %s tool"), batchreport->batchnumber,
				batchreport->tool->toolname);
			if (batchreport->activity != 0)
				formatinfstr(infstr, M_(", caused by '%s'"), batchreport->activity);
			addstringtoinfstr(infstr, M_(" is:"));
			ttyputmsg(x_("%s"), returninfstr(infstr));

			for(c = batchreport->changehead; c != NOCHANGE; c = c->nextchange)
				ttyputmsg(x_("%s"), db_describechange(c->changetype, c->entryaddr, c->p1, c->p2,
					c->p3, c->p4, c->p5, c->p6));
			return;
		}
		ttyputmsg(M_("Batch %ld is not in the list (range is %ld to %ld)"), which, lobatch, hibatch);
		return;
	}

	/* display the change batches */
	ttyputmsg(M_("%ld batches (limit is %ld):"), db_batchtally-1, db_maximumbatches);
	for(batchreport = db_tailbatch; batchreport != NOCHANGEBATCH;
		batchreport = batchreport->nextchangebatch)
	{
		node = arcinst = portproto = nproto = object = variable = 0;
		for(c = batchreport->changehead; c != NOCHANGE; c = c->nextchange)
			switch (c->changetype)
		{
				case NODEINSTNEW:
				case NODEINSTKILL:
				case NODEINSTMOD:
					node++;
					break;
				case ARCINSTNEW:
				case ARCINSTKILL:
				case ARCINSTMOD:
					arcinst++;
					break;
				case PORTPROTONEW:
				case PORTPROTOKILL:
				case PORTPROTOMOD:
					portproto++;
					break;
				case NODEPROTONEW:
				case NODEPROTOKILL:
				case NODEPROTOMOD:
					nproto++;
					break;
				case OBJECTNEW:
				case OBJECTKILL:
					object++;
					break;
				case VARIABLENEW:
				case VARIABLEKILL:
				case VARIABLEMOD:
				case VARIABLEINS:
				case VARIABLEDEL:
					variable++;
					break;
				case DESCRIPTMOD:
					if ((VARIABLE *)c->p1 != NOVARIABLE) variable++; else
						if ((PORTPROTO *)c->p3 != NOPORTPROTO) portproto++; else
							node++;
					break;
		}

		infstr = initinfstr();
		(void)esnprintf(line, 50, x_("%ld {"), batchreport->batchnumber);
		addstringtoinfstr(infstr, line);
		if (batchreport->activity != 0) addstringtoinfstr(infstr, batchreport->activity);
		addstringtoinfstr(infstr, M_("} affects"));

		if (node != 0)
		{
			(void)esnprintf(line, 50, x_(" %ld %s"), node, makeplural(M_("node"), node));
			addstringtoinfstr(infstr, line);
		}
		if (arcinst != 0)
		{
			(void)esnprintf(line, 50, x_(" %ld %s"), arcinst, makeplural(M_("arc"), arcinst));
			addstringtoinfstr(infstr, line);
		}
		if (portproto != 0)
		{
			(void)esnprintf(line, 50, x_(" %ld %s"), portproto, makeplural(M_("port"), portproto));
			addstringtoinfstr(infstr, line);
		}
		if (nproto != 0)
		{
			(void)esnprintf(line, 50, x_(" %ld %s"), nproto, makeplural(M_("cell"), nproto));
			addstringtoinfstr(infstr, line);
		}
		if (object != 0)
		{
			(void)esnprintf(line, 50, x_(" %ld %s"), object, makeplural(M_("object"), object));
			addstringtoinfstr(infstr, line);
		}
		if (variable != 0)
		{
			(void)esnprintf(line, 50, x_(" %ld %s"), variable, makeplural(M_("variable"), variable));
			addstringtoinfstr(infstr, line);
		}
		ttyputmsg(x_("%s"), returninfstr(infstr));
	}
}

CHAR *db_describechange(INTBIG changetype, INTBIG entryaddr, INTBIG p1, INTBIG p2,
	INTBIG p3, INTBIG p4, INTBIG p5, INTBIG p6)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp;
	REGISTER NODEPROTO *np;
	REGISTER INTBIG lambda;
	VARIABLE myvar;
	REGISTER void *infstr;

	switch (changetype)
	{
		case NODEINSTNEW:
			ni = (NODEINST *)entryaddr;
			infstr = initinfstr();
			formatinfstr(infstr, M_(" Node '%s' created in cell %s"), describenodeinst(ni),
				describenodeproto(ni->parent));
			return(returninfstr(infstr));
		case NODEINSTKILL:
			ni = (NODEINST *)entryaddr;
			infstr = initinfstr();
			formatinfstr(infstr, M_(" Node '%s' killed in cell %s"), describenodeinst(ni),
				describenodeproto(ni->parent));
			return(returninfstr(infstr));
		case NODEINSTMOD:
			ni = (NODEINST *)entryaddr;
			lambda = lambdaofnode(ni);
			infstr = initinfstr();
			formatinfstr(infstr, M_(" Node '%s' changed in cell %s [was %sx%s, center (%s,%s), rotated %ld"),
				describenodeinst(ni), describenodeproto(ni->parent), latoa(p3-p1, lambda),
				latoa(p4-p2, lambda), latoa((p1+p3)/2, lambda), latoa((p2+p4)/2, lambda), p5);
			if (p6 != 0) addstringtoinfstr(infstr, M_(", transposed"));
			addstringtoinfstr(infstr, x_("]"));
			return(returninfstr(infstr));
		case ARCINSTNEW:
			ai = (ARCINST *)entryaddr;
			infstr = initinfstr();
			formatinfstr(infstr, M_(" Arc '%s' created in cell %s"), describearcinst(ai),
				describenodeproto(ai->parent));
			return(returninfstr(infstr));
		case ARCINSTKILL:
			ai = (ARCINST *)entryaddr;
			infstr = initinfstr();
			formatinfstr(infstr, M_(" Arc '%s' killed in cell %s"), describearcinst(ai),
				describenodeproto(ai->parent));
			return(returninfstr(infstr));
		case ARCINSTMOD:
			ai = (ARCINST *)entryaddr;
			lambda = lambdaofarc(ai);
			infstr = initinfstr();
			formatinfstr(infstr, M_(" Arc '%s' modified in cell %s [was %s wide %s long from (%s,%s) to (%s,%s)]"),
				describearcinst(ai), describenodeproto(ai->parent), latoa(p5, lambda), latoa(p6, lambda),
					latoa(p1, lambda), latoa(p2, lambda), latoa(p3, lambda), latoa(p4, lambda));
			return(returninfstr(infstr));
		case PORTPROTONEW:
			pp = (PORTPROTO *)entryaddr;
			infstr = initinfstr();
			formatinfstr(infstr, M_(" Port '%s' created in cell %s"), pp->protoname,
				describenodeproto(pp->parent));
			return(returninfstr(infstr));
		case PORTPROTOKILL:
			pp = (PORTPROTO *)entryaddr;
			infstr = initinfstr();
			formatinfstr(infstr, M_(" Port '%s' killed in cell %s"), pp->protoname,
				describenodeproto(pp->parent));
			return(returninfstr(infstr));
		case PORTPROTOMOD:
			pp = (PORTPROTO *)entryaddr;
			infstr = initinfstr();
			formatinfstr(infstr, M_(" Port '%s' moved in cell %s [was on node %s, subport %s]"),
				pp->protoname, describenodeproto(pp->parent), describenodeinst((NODEINST *)p1),
				((PORTPROTO *)p2)->protoname);
			return(returninfstr(infstr));
		case NODEPROTONEW:
			np = (NODEPROTO *)entryaddr;
			infstr = initinfstr();
			formatinfstr(infstr, M_(" Cell '%s' created"), describenodeproto(np));
			return(returninfstr(infstr));
		case NODEPROTOKILL:
			np = (NODEPROTO *)entryaddr;
			infstr = initinfstr();
			formatinfstr(infstr, M_(" Cell %s' killed"), describenodeproto(np));
			return(returninfstr(infstr));
		case NODEPROTOMOD:
			np = (NODEPROTO *)entryaddr;
			lambda = lambdaofcell(np);
			infstr = initinfstr();
			formatinfstr(infstr, M_(" Cell '%s' modified [was from %s<=X<=%s %s<=Y<=%s]"),
				describenodeproto(np), latoa(p1, lambda), latoa(p2, lambda),
					latoa(p3, lambda), latoa(p4, lambda));
			return(returninfstr(infstr));
		case OBJECTSTART:
			/* args: addr, type */
			infstr = initinfstr();
			formatinfstr(infstr, M_(" Start change to object %s"), describeobject(entryaddr, p1));
			return(returninfstr(infstr));
		case OBJECTEND:
			/* args: addr, type */
			infstr = initinfstr();
			formatinfstr(infstr, M_(" End change to object %s"), describeobject(entryaddr, p1));
			return(returninfstr(infstr));
		case OBJECTNEW:
			/* args: addr, type */
			infstr = initinfstr();
			formatinfstr(infstr, M_(" Object %s created"), describeobject(entryaddr, p1));
			return(returninfstr(infstr));
		case OBJECTKILL:
			/* args: addr, type */
			infstr = initinfstr();
			formatinfstr(infstr, M_(" Object %s deleted"), describeobject(entryaddr, p1));
			return(returninfstr(infstr));
		case VARIABLENEW:
			/* args: addr, type, key, newtype */
			infstr = initinfstr();
			formatinfstr(infstr, M_(" Variable '%s' created on %s"), changedvariablename(p1, p2, p3),
				describeobject(entryaddr, p1));
			return(returninfstr(infstr));
		case VARIABLEKILL:
			/* args: addr, type, key, oldaddr, oldtype, olddescript[0], olddescript[1] */
			infstr = initinfstr();
			myvar.key = p2;  myvar.type = p4;  myvar.addr = p3;
			formatinfstr(infstr, M_(" Variable '%s' killed on %s [was %s]"), changedvariablename(p1, p2, p4),
				describeobject(entryaddr, p1), describevariable(&myvar, -1, -1));
			return(returninfstr(infstr));
		case VARIABLEMOD:
			/* args: addr, type, key, vartype, aindex, oldvalue */
			infstr = initinfstr();
			formatinfstr(infstr, M_(" Variable '%s[%ld]' changed on %s"), changedvariablename(p1, p2, p3), p4,
				describeobject(entryaddr, p1));
			if ((p3&VCREF) == 0)
			{
				myvar.key = p2;  myvar.type = p3 & ~VISARRAY;
				myvar.addr = p5;
				formatinfstr(infstr, M_(" [was '%s']"), describevariable(&myvar, -1, -1));
			}
			return(returninfstr(infstr));
		case VARIABLEINS:
			/* args: addr, type, key, vartype, aindex */
			infstr = initinfstr();
			formatinfstr(infstr, M_(" Variable '%s[%ld]' inserted on %s"), changedvariablename(p1, p2, p3), p4,
				describeobject(entryaddr, p1));
			return(returninfstr(infstr));
		case VARIABLEDEL:
			/* args: addr, type, key, vartype, aindex, oldvalue */
			infstr = initinfstr();
			formatinfstr(infstr, M_(" Variable '%s[%ld]' deleted on %s"), changedvariablename(p1, p2, p3), p4,
				describeobject(entryaddr, p1));
			return(returninfstr(infstr));
		case DESCRIPTMOD:
			infstr = initinfstr();
			formatinfstr(infstr, M_(" Text descriptor on variable %s changed [was 0%lo/0%lo]"),
				makename(p2), p4, p5);
			return(returninfstr(infstr));
	}
	return(x_("?"));
}

/* Undo dialog */
static DIALOGITEM db_undodialogitems[] =
{
 /*  1 */ {0, {468,608,492,680}, BUTTON, N_("OK")},
 /*  2 */ {0, {32,8,455,690}, SCROLL, x_("")},
 /*  3 */ {0, {468,16,492,88}, BUTTON, N_("Undo")},
 /*  4 */ {0, {8,8,24,241}, MESSAGE, N_("These are the recent changes:")},
 /*  5 */ {0, {8,344,24,456}, CHECK, N_("Show details")},
 /*  6 */ {0, {468,108,492,180}, BUTTON, N_("Redo")}
};
static DIALOG db_undodialog = {{50,75,555,775}, N_("Change History"), 0, 6, db_undodialogitems, 0, 0};

/* special items in the "undo" dialog: */
#define DUND_CHGLIST   2		/* Change list (scroll) */
#define DUND_UNDO      3		/* Undo (button) */
#define DUND_DETAILS   5		/* Show detail (check box) */
#define DUND_REDO      6		/* Redo (button) */

void db_undodlog(void)
{
	INTBIG itemHit, details;
	TOOL *tool;
	REGISTER void *dia;

	/* display the undo dialog box */
	dia = DiaInitDialog(&db_undodialog);
	if (dia == 0) return;
	DiaInitTextDialog(dia, DUND_CHGLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone,
		-1, SCHORIZBAR|SCSMALLFONT);
	details = 0;
	db_loadhistorylist(details, dia);

	/* default number of changes to undo */
	DiaSelectLine(dia, DUND_CHGLIST, 1000);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK) break;
		if (itemHit == DUND_DETAILS)
		{
			details = 1 - details;
			DiaSetControl(dia, DUND_DETAILS, details);
			db_loadhistorylist(details, dia);
			continue;
		}
		if (itemHit == DUND_UNDO)
		{
			(void)undoabatch(&tool);
			db_loadhistorylist(details, dia);
			continue;
		}
		if (itemHit == DUND_REDO)
		{
			(void)redoabatch(&tool);
			db_loadhistorylist(details, dia);
			continue;
		}
	}

	DiaDoneDialog(dia);
}

/*
 * helper routines for undo dialog
 */
void db_loadhistorylist(INTBIG details, void *dia)
{
	REGISTER INTBIG node, arcinst, portproto, nproto, object, variable, lineno, curline;
	REGISTER CHANGE *c;
	CHAR line[50];
	CHANGEBATCH *batch;
	REGISTER void *infstr;

	DiaLoadTextDialog(dia, DUND_CHGLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);
	lineno = 0;
	curline = -1;
	for(batch = db_tailbatch; batch != NOCHANGEBATCH; batch = batch->nextchangebatch)
	{
		node = arcinst = portproto = nproto = object = variable = 0;
		for(c = batch->changehead; c != NOCHANGE; c = c->nextchange)
			switch (c->changetype)
		{
				case NODEINSTNEW:
				case NODEINSTKILL:
				case NODEINSTMOD:
					node++;
					break;
				case ARCINSTNEW:
				case ARCINSTKILL:
				case ARCINSTMOD:
					arcinst++;
					break;
				case PORTPROTONEW:
				case PORTPROTOKILL:
				case PORTPROTOMOD:
					portproto++;
					break;
				case NODEPROTONEW:
				case NODEPROTOKILL:
				case NODEPROTOMOD:
					nproto++;
					break;
				case OBJECTNEW:
				case OBJECTKILL:
					object++;
					break;
				case VARIABLENEW:
				case VARIABLEKILL:
				case VARIABLEMOD:
				case VARIABLEINS:
				case VARIABLEDEL:
				case DESCRIPTMOD:
					variable++;
					break;
		}

		infstr = initinfstr();
		if (details != 0) addstringtoinfstr(infstr, M_("***** BATCH "));
		(void)esnprintf(line, 50, x_("%ld {"), batch->batchnumber);
		addstringtoinfstr(infstr, line);
		if (batch->activity != 0) addstringtoinfstr(infstr, batch->activity);
		addstringtoinfstr(infstr, M_("} affects"));

		if (node != 0)
		{
			(void)esnprintf(line, 50, x_(" %ld %s"), node, makeplural(M_("node"), node));
			addstringtoinfstr(infstr, line);
		}
		if (arcinst != 0)
		{
			(void)esnprintf(line, 50, x_(" %ld %s"), arcinst, makeplural(M_("arc"), arcinst));
			addstringtoinfstr(infstr, line);
		}
		if (portproto != 0)
		{
			(void)esnprintf(line, 50, x_(" %ld %s"), portproto, makeplural(M_("port"), portproto));
			addstringtoinfstr(infstr, line);
		}
		if (nproto != 0)
		{
			(void)esnprintf(line, 50, x_(" %ld %s"), nproto, makeplural(M_("cell"), nproto));
			addstringtoinfstr(infstr, line);
		}
		if (object != 0)
		{
			(void)esnprintf(line, 50, x_(" %ld %s"), object, makeplural(M_("object"), object));
			addstringtoinfstr(infstr, line);
		}
		if (variable != 0)
		{
			(void)esnprintf(line, 50, x_(" %ld %s"), variable, makeplural(M_("variable"), variable));
			addstringtoinfstr(infstr, line);
		}
		DiaStuffLine(dia, DUND_CHGLIST, returninfstr(infstr));
		if (batch == db_donebatch) curline = lineno;
		lineno++;

		if (details != 0)
		{
			for(c = batch->changehead; c != NOCHANGE; c = c->nextchange)
			{
				DiaStuffLine(dia, DUND_CHGLIST, db_describechange(c->changetype, c->entryaddr, c->p1, c->p2,
					c->p3, c->p4, c->p5, c->p6));
				lineno++;
			}
		}
	}
	DiaSelectLine(dia, DUND_CHGLIST, curline);
}

/************************* MEMORY ALLOCATION *************************/

/*
 * routine to allocate a change module from the pool (if any) or memory
 * the routine returns NOCHANGE if allocation fails.
 */
CHANGE *db_allocchange(void)
{
	REGISTER CHANGE *c;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	if (db_changefree == NOCHANGE)
	{
		c = (CHANGE *)emalloc((sizeof (CHANGE)), db_cluster);
		if (c == 0) return(NOCHANGE);
		return(c);
	}

	/* take change module from free list */
	c = db_changefree;
	db_changefree = db_changefree->nextchange;
	return(c);
}

/*
 * routine to free a change module
 */
void db_freechange(CHANGE *c)
{
	c->nextchange = db_changefree;
	db_changefree = c;
}

/*
 * routine to allocate a changecell module from the pool (if any) or memory
 * the routine returns NOCHANGECELL if allocation fails.
 */
CHANGECELL *db_allocchangecell(void)
{
	REGISTER CHANGECELL *c;

	if (db_changecellfree == NOCHANGECELL)
	{
		c = (CHANGECELL *)emalloc((sizeof (CHANGECELL)), db_cluster);
		if (c == 0) return(NOCHANGECELL);
		return(c);
	}

	/* take change module from free list */
	c = db_changecellfree;
	db_changecellfree = db_changecellfree->nextchangecell;
	return(c);
}

/*
 * routine to free a changecell module
 */
void db_freechangecell(CHANGECELL *c)
{
	c->nextchangecell = db_changecellfree;
	db_changecellfree = c;
}

/*
 * routine to allocate a change batch from the pool (if any) or memory
 * the routine returns NOCHANGEBATCH if allocation fails.
 */
CHANGEBATCH *db_allocchangebatch(void)
{
	REGISTER CHANGEBATCH *b;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	if (db_changebatchfree == NOCHANGEBATCH)
	{
		/* no free change batches...allocate one */
		b = (CHANGEBATCH *)emalloc((sizeof (CHANGEBATCH)), db_cluster);
		if (b == 0) return(NOCHANGEBATCH);
	} else
	{
		/* take batch from free list */
		b = db_changebatchfree;
		db_changebatchfree = db_changebatchfree->nextchangebatch;
	}
	return(b);
}

/*
 * routine to free a change batch
 */
void db_freechangebatch(CHANGEBATCH *b)
{
	b->nextchangebatch = db_changebatchfree;
	db_changebatchfree = b;
}
