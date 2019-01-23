/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrcom.c
 * User interface tool: parameter parsing routines
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
#include "efunction.h"
#include "usr.h"
#include "drc.h"
#include "usredtec.h"
#include "tecart.h"

/****************************** PARSING CODE ******************************/

#define COMMANDS            01		/* user commands */
#define MACROS              02		/* macro names */
#define PORTS               04		/* ports names on a cell */
#define EXPORTS            010		/* exported ports on the node */
#define WINDOWS            020		/* window names */
#define HIGHLIGHTS         040		/* highlight names */
#define ARCS              0100		/* arc prototype names */

#define CELLS             0400		/* cell names */
#define PRIMS            01000		/* primitive node prototype names */
#define PURES            02000		/* pure-layer node prototype names */
#define LIBS             04000		/* library names */
#define LAYERS          010000		/* layer names */
#define POPUPS          020000		/* popup menu names */
#define CONSTRAINTS     040000		/* constraint solver names */
#define MBUTTONS       0100000		/* mouse button names */
#define VIEWS          0200000		/* view names (including "unknown") */
#define NODEFUN        0400000		/* node functions */
#define ARCFUN        01000000		/* arc functions */
#define EDTECSTUFF   036000000		/* technology edit parsing */
#define EDTECPORT     02000000		/*   port connections in technology edit */
#define EDTECLAY      04000000		/*   layer names from technology edit */
#define EDTECARC      06000000		/*   arc names from technology edit */
#define EDTECNODE    010000000		/*   node names from technology edit */
#define EDTECTECH    012000000		/*   editable technologies for tech edit */
#define NETWORKS    0100000000		/* network names */
#define WORDSET     0200000000		/* the word "SET-MINIMUM-SIZE" */
#define WORDCLEAR   0400000000		/* the word "CLEAR-MINIMUM-SIZE" */
#define WORDNODE   01000000000		/* the word "NODE" */
#define WORDARC    02000000000		/* the word "ARC" */
#define WORDNEXT   04000000000		/* the word "NEXT-PROTO" */
#define WORDPREV  010000000000		/* the word "PREV-PROTO" */
#define WORDTHIS  020000000000		/* the word "THIS-PROTO" */

static INTBIG       us_posbits, us_posswitch, us_possearch;
static INTBIG       us_poslcommands;			/* COMMANDS */
static INTBIG       us_posmacrovar;				/* MACROS */
static PORTPROTO   *us_posports;				/* PORTS */
static PORTEXPINST *us_posexports;				/* EXPORTS */
static WINDOWPART  *us_poswindow;				/* WINDOWPARTS */
static INTBIG       us_poshigh;					/* HIGHLIGHTS */
static ARCPROTO    *us_posarcs;					/* ARCS */
static NODEPROTO   *us_posnodeprotos;			/* CELLS */
static NODEPROTO   *us_posprims;				/* PRIMS */
static LIBRARY     *us_poslibs;					/* LIBS */
static INTBIG       us_poslayers;				/* LAYERS */
static POPUPMENU   *us_pospopupmenu;			/* POPUPS */
static INTBIG       us_posconstraint;			/* CONSTRAINTS */
static INTBIG       us_posmbuttons;				/* MBUTTONS */
static VIEW        *us_posviews;				/* VIEWS */
static INTBIG       us_posfunction;				/* NODEFUN/ARCFUN */
static INTBIG       us_posspecial;				/* EDTECPORT */
static TECHNOLOGY  *us_postechs;				/* EDTECTECH */
static INTBIG       us_postecedsequencepos;		/* EDTECLAY */
static INTBIG       us_postecedsequencecount;	/* EDTECLAY */
static NODEPROTO  **us_postecedsequence;		/* EDTECLAY */

/* prototypes for local routines */
static INTBIG  us_paramcommands(CHAR*, COMCOMP*[], CHAR);
static BOOLEAN us_topofgetproto(CHAR**);
static BOOLEAN us_topofarcs(CHAR**);
static BOOLEAN us_topofpures(CHAR**);
static BOOLEAN us_topofnodefun(CHAR**);
static BOOLEAN us_topofarcfun(CHAR**);
static BOOLEAN us_topofedtecport(CHAR**);
static BOOLEAN us_topofedtecslay(CHAR**);
static BOOLEAN us_topofedtecclay(CHAR**);
static BOOLEAN us_topofedtectech(CHAR**);
static BOOLEAN us_topofeditor(CHAR**);
static CHAR   *us_nextparse(void);
static CHAR   *us_nexteditor(void);
static INTBIG  us_parambind(CHAR*, COMCOMP*[], CHAR);
static INTBIG  us_parambindpop(CHAR*, COMCOMP*[], CHAR);
static INTBIG  us_paramconstraintb(CHAR*, COMCOMP*[], CHAR);
static INTBIG  us_paramconstrainta(CHAR*, COMCOMP*[], CHAR);
static INTBIG  us_nextecho(CHAR*, COMCOMP*[], CHAR);
static INTBIG  us_parammenusbp(CHAR*, COMCOMP*[], CHAR);
static INTBIG  us_paramportep(CHAR*, COMCOMP*[], CHAR);
static INTBIG  us_paramsize(CHAR*, COMCOMP*[], CHAR);
static INTBIG  us_paramsedtecport(CHAR*, COMCOMP*[], CHAR);
static INTBIG  us_nexttechnologyedlp(CHAR*, COMCOMP*[], CHAR);
static INTBIG  us_paramtechnology(CHAR*, COMCOMP*[], CHAR);
static INTBIG  us_paramtechnologyb(CHAR*, COMCOMP*[], CHAR);
static INTBIG  us_paramtechnologya(CHAR*, COMCOMP*[], CHAR);
static INTBIG  us_paramtelltoolb(CHAR*, COMCOMP*[], CHAR);
static INTBIG  us_paramtelltoola(CHAR*, COMCOMP*[], CHAR);
static INTBIG  us_paramvisiblelayers(CHAR*, COMCOMP*[], CHAR);
static INTBIG  us_paramsedtecpcol(CHAR*, COMCOMP*[], CHAR);

/* parsing routines for a command or macro */
BOOLEAN us_topofcommands(CHAR **c)
{
	us_poslcommands = 0;
	us_posmacrovar = 0;
	us_pospopupmenu = us_firstpopupmenu;
	us_posbits = COMMANDS | MACROS | POPUPS;
	return(TRUE);
}

INTBIG us_paramcommands(CHAR *word, COMCOMP *arr[], CHAR breakc)
{
	REGISTER INTBIG i, count, ind;
	CHAR *build[MAXPARS];
	REGISTER VARIABLE *var;

	if (*word == 0) return(0);

	/* see if it is a normal command */
	for(ind=0; us_lcommand[ind].name != 0; ind++)
		if (namesame(word, us_lcommand[ind].name) == 0)
	{
		count = us_lcommand[ind].params;
		for(i=0; i<count; i++) arr[i] = us_lcommand[ind].par[i];
		return(count);
	}

	/* see if it is a macro */
	var = us_getmacro(word);
	if (var != NOVARIABLE && getlength(var) >= 3)
	{
		count = us_parsecommand(((CHAR **)var->addr)[2], build);
		for(i=0; i<count; i++) arr[i] = (COMCOMP *)myatoi(build[i]);
		return(count);
	}
	return(0);
}

/* parsing routines for a macro */
BOOLEAN us_topofmacros(CHAR **c)
{
	us_posmacrovar = 0;
	us_posbits = MACROS;
	return(TRUE);
}

/* parsing routines for a popup menu */
BOOLEAN us_topofpopupmenu(CHAR **c)
{
	us_pospopupmenu = us_firstpopupmenu;
	us_posbits = POPUPS;
	return(TRUE);
}

/* parsing routines for a port on the current node */
BOOLEAN us_topofports(CHAR **c)
{
	HIGHLIGHT high;
	REGISTER VARIABLE *var;

	us_posbits = PORTS;
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var == NOVARIABLE) return(FALSE);
	(void)us_makehighlight(((CHAR **)var->addr)[0], &high);

	if ((high.status&HIGHFROM) != 0 && high.fromgeom != NOGEOM &&
		high.fromgeom->entryisnode)
			us_posports = high.fromgeom->entryaddr.ni->proto->firstportproto; else
				us_posports = NOPORTPROTO;
	if (us_posports == NOPORTPROTO) return(FALSE);
	return(TRUE);
}

/* parsing routines for an export on the current cell */
BOOLEAN us_topofcports(CHAR **c)
{
	REGISTER NODEPROTO *np;

	np = getcurcell();
	us_posbits = PORTS;
	if (np == NONODEPROTO) us_posports = NOPORTPROTO; else
		us_posports = np->firstportproto;
	if (us_posports == NOPORTPROTO) return(FALSE);
	return(TRUE);
}

/* parsing routines for an export on the current node */
BOOLEAN us_topofexpports(CHAR **c)
{
	HIGHLIGHT high;
	REGISTER VARIABLE *var;

	us_posbits = EXPORTS;
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var == NOVARIABLE) return(FALSE);
	(void)us_makehighlight(((CHAR **)var->addr)[0], &high);
	us_posexports = NOPORTEXPINST;
	if ((high.status&HIGHFROM) != 0 && high.fromgeom != NOGEOM &&
		high.fromgeom->entryisnode)
			us_posexports = high.fromgeom->entryaddr.ni->firstportexpinst;
	if (us_posexports == NOPORTEXPINST) return(FALSE);
	return(TRUE);
}

/* parsing routines for a window */
BOOLEAN us_topofwindows(CHAR **c)
{
	us_posbits = WINDOWS;
	us_poswindow = el_topwindowpart;
	return(TRUE);
}

/* parsing routines for a layer name */
BOOLEAN us_topoflayers(CHAR **c)
{
	us_posbits = LAYERS;
	us_poslayers = el_curtech->layercount - 1;
	return(TRUE);
}

/* parsing routines for a highlight */
BOOLEAN us_topofhighlight(CHAR **c)
{
	us_posbits = HIGHLIGHTS;
	us_poshigh = 0;
	return(TRUE);
}

/* parsing routines for a node or arc prototype */
BOOLEAN us_topofarcnodes(CHAR **c)
{
	REGISTER CHAR *pt;
	REGISTER TECHNOLOGY *t;

	us_posbits = ARCS | CELLS | PRIMS;
	us_posnodeprotos = el_curlib->firstnodeproto;
	us_posprims = el_curtech->firstnodeproto;
	us_posarcs = el_curtech->firstarcproto;

	/* see if a technology specification was given */
	for(pt = *c; *pt != 0; pt++) if (*pt == ':') break;
	if (*pt == ':')
	{
		*pt = 0;
		t = gettechnology(*c);
		*pt++ = ':';
		if (t != NOTECHNOLOGY)
		{
			*c = pt;
			us_posbits = ARCS | PRIMS;
			us_posprims = t->firstnodeproto;
			us_posarcs = t->firstarcproto;
		}
	}
	return(TRUE);
}

/* parsing routines for a "getproto" command */
BOOLEAN us_topofgetproto(CHAR **c)
{
	(void)us_topofarcnodes(c);
	us_posbits |= WORDNODE | WORDARC | WORDNEXT | WORDPREV | WORDTHIS;
	return(TRUE);
}

/* parsing routines for a node prototype */
BOOLEAN us_topofnodes(CHAR **c)
{
	REGISTER BOOLEAN i;

	i = us_topofarcnodes(c);
	us_posbits &= ~ARCS;
	return(i);
}

/* parsing routines for a cell */
BOOLEAN us_topofcells(CHAR **c)
{
	REGISTER CHAR *pt;
	REGISTER LIBRARY *lib;

	/* by default, assume the current library */
	us_posbits = CELLS;
	us_posnodeprotos = el_curlib->firstnodeproto;

	/* see if a library specification was given */
	for(pt = *c; *pt != 0; pt++) if (*pt == ':') break;
	if (*pt == ':')
	{
		*pt = 0;
		lib = getlibrary(*c);
		*pt++ = ':';
		if (lib != NOLIBRARY)
		{
			*c = pt;
			us_posnodeprotos = lib->firstnodeproto;
		}
	}
	return(TRUE);
}

/* parsing routines for a primitives nodeproto */
BOOLEAN us_topofprims(CHAR **c)
{
	REGISTER CHAR *pt;
	REGISTER TECHNOLOGY *t;

	/* by default, assume the current technology */
	us_posbits = PRIMS;
	us_posprims = el_curtech->firstnodeproto;

	/* see if a technology specification was given */
	for(pt = *c; *pt != 0; pt++) if (*pt == ':') break;
	if (*pt == ':')
	{
		*pt = 0;
		t = gettechnology(*c);
		*pt++ = ':';
		if (t != NOTECHNOLOGY)
		{
			*c = pt;
			us_posprims = t->firstnodeproto;
		}
	}
	return(TRUE);
}

/* parsing routines for a pure-layer nodeproto */
BOOLEAN us_topofpures(CHAR **c)
{
	us_posbits = PURES;
	us_posprims = el_curtech->firstnodeproto;
	return(TRUE);
}

/* parsing routines for an arc prototype */
BOOLEAN us_topofarcs(CHAR **c)
{
	REGISTER CHAR *pt;
	REGISTER TECHNOLOGY *t;

	/* by default, assume the current technology */
	us_posbits = ARCS;
	us_posarcs = el_curtech->firstarcproto;

	/* see if a technology specification was given */
	for(pt = *c; *pt != 0; pt++) if (*pt == ':') break;
	if (*pt == ':')
	{
		*pt = 0;
		t = gettechnology(*c);
		*pt++ = ':';
		if (t != NOTECHNOLOGY)
		{
			*c = pt;
			us_posarcs = t->firstarcproto;
		}
	}
	return(TRUE);
}

/* parsing routines for a constraint solver */
BOOLEAN us_topofconstraints(CHAR **c)
{
	us_posbits = CONSTRAINTS;
	us_posconstraint = 0;
	return(TRUE);
}

/* parsing routines for a mouse button */
BOOLEAN us_topofmbuttons(CHAR **c)
{
	us_posbits = MBUTTONS;
	us_posmbuttons = 0;
	return(TRUE);
}

/* parsing routines for a node function name */
BOOLEAN us_topofnodefun(CHAR **c)
{
	us_posbits = NODEFUN;
	us_posfunction = 0;
	return(TRUE);
}

/* parsing routines for an arc function name */
BOOLEAN us_topofarcfun(CHAR **c)
{
	us_posbits = ARCFUN;
	us_posfunction = 0;
	return(TRUE);
}

/* parsing routines for port information when technology editing */
BOOLEAN us_topofedtecport(CHAR **c)
{
	us_posbits = EDTECPORT;
	us_posnodeprotos = el_curlib->firstnodeproto;
	us_posspecial = 0;
	return(TRUE);
}

/* parsing routines for a layer name when technology editing */
BOOLEAN us_topofedteclay(CHAR **c)
{
	LIBRARY *liblist[1];

	us_posbits = EDTECLAY;
	us_postecedsequencepos = 0;
	liblist[0] = el_curlib;
	us_postecedsequencecount = us_teceditfindsequence(liblist, 1, x_("layer-"),
		x_("EDTEC_layersequence"), &us_postecedsequence);
	return(TRUE);
}

/*
 * parsing routines for a layer name or "SET-MINIMUM-SIZE",
 * when technology editing
 */
BOOLEAN us_topofedtecslay(CHAR **c)
{
	LIBRARY *liblist[1];

	us_posbits = EDTECLAY|WORDSET;
	us_postecedsequencepos = 0;
	liblist[0] = el_curlib;
	us_postecedsequencecount = us_teceditfindsequence(liblist, 1, x_("layer-"),
		x_("EDTEC_layersequence"), &us_postecedsequence);
	return(TRUE);
}

/*
 * parsing routines for a layer name or "CLEAR-MINIMUM-SIZE",
 * when technology editing
 */
BOOLEAN us_topofedtecclay(CHAR **c)
{
	LIBRARY *liblist[1];

	us_posbits = EDTECLAY|WORDCLEAR;
	us_postecedsequencepos = 0;
	liblist[0] = el_curlib;
	us_postecedsequencecount = us_teceditfindsequence(liblist, 1, x_("layer-"),
		x_("EDTEC_layersequence"), &us_postecedsequence);
	return(TRUE);
}

/* parsing routines for an arc name when technology editing */
BOOLEAN us_topofedtecarc(CHAR **c)
{
	us_posbits = EDTECARC;
	us_posnodeprotos = el_curlib->firstnodeproto;
	return(TRUE);
}

/* parsing routines for a node name when technology editing */
BOOLEAN us_topofedtecnode(CHAR **c)
{
	us_posbits = EDTECNODE;
	us_posnodeprotos = el_curlib->firstnodeproto;
	return(TRUE);
}

/* parsing routines for editable technology name when tech editing */
BOOLEAN us_topofedtectech(CHAR **c)
{
	us_posbits = EDTECTECH;
	us_postechs = el_technologies;
	return(TRUE);
}

/* parsing routines for a nodeproto/arcproto/library/macro/portproto */
BOOLEAN us_topofallthings(CHAR **c)
{
	REGISTER NODEPROTO *np;
	REGISTER CHAR *pt;
	REGISTER LIBRARY *lib;

	us_posbits = PRIMS | CELLS | ARCS | LIBS | MACROS | PORTS | POPUPS | CONSTRAINTS | NETWORKS;
	us_posprims = el_curtech->firstnodeproto;
	us_posnodeprotos = el_curlib->firstnodeproto;
	us_posarcs = el_curtech->firstarcproto;
	us_poslibs = el_curlib;
	us_posmacrovar = 0;
	us_pospopupmenu = us_firstpopupmenu;
	us_posconstraint = 0;
	np = getcurcell();
	if (np != NONODEPROTO) us_posports = np->firstportproto; else
	{
		us_posports = NOPORTPROTO;
		us_posbits &= ~PORTS;
	}
	(void)topofnets(c);

	/* see if a library specification was given */
	for(pt = *c; *pt != 0; pt++) if (*pt == ':') break;
	if (*pt == ':')
	{
		*pt = 0;
		lib = getlibrary(*c);
		*pt++ = ':';
		if (lib != NOLIBRARY)
		{
			*c = pt;
			us_posnodeprotos = lib->firstnodeproto;
		}
	}
	return(TRUE);
}

CHAR *us_nextparse(void)
{
	CHAR *nextname;
	INTBIG unimportant, funct;
	INTBIG bits;
	REGISTER TECHNOLOGY *tech;
	REGISTER void *infstr;

	if ((us_posbits&COMMANDS) != 0)
	{
		if (us_poslcommands < us_longcount) return(us_lcommand[us_poslcommands++].name);
		us_posbits &= ~COMMANDS;
	}
	if ((us_posbits&MACROS) != 0)
	{
		while (us_posmacrovar < us_tool->numvar)
		{
			nextname = makename(us_tool->firstvar[us_posmacrovar].key);
			us_posmacrovar++;
			if (namesamen(nextname, x_("USER_macro_"), 11) == 0) return(&nextname[11]);
		}
		us_posbits &= ~MACROS;
	}
	if ((us_posbits&PORTS) != 0)
	{
		if (us_posports != NOPORTPROTO)
		{
			nextname = us_posports->protoname;
			us_posports = us_posports->nextportproto;
			return(nextname);
		}
		us_posbits &= ~PORTS;
	}
	if ((us_posbits&EXPORTS) != 0)
	{
		if (us_posexports != NOPORTEXPINST)
		{
			nextname = us_posexports->exportproto->protoname;
			us_posexports = us_posexports->nextportexpinst;
			return(nextname);
		}
		us_posbits &= ~EXPORTS;
	}
	if ((us_posbits&WINDOWS) != 0)
	{
		if (us_poswindow != NOWINDOWPART)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, describenodeproto(us_poswindow->curnodeproto));
			addtoinfstr(infstr, '(');
			addstringtoinfstr(infstr, us_poswindow->location);
			addtoinfstr(infstr, ')');
			us_poswindow = us_poswindow->nextwindowpart;
			return(returninfstr(infstr));
		}
		us_posbits &= ~WINDOWS;
	}
	if ((us_posbits&HIGHLIGHTS) != 0)
	{
		while (us_poshigh < us_tool->numvar)
		{
			nextname = (CHAR *)us_tool->firstvar[us_poshigh].key;
			us_poshigh++;
			if (namesamen(nextname, x_("USER_highlight_"), 15) == 0) return(&nextname[15]);
		}
		us_posbits &= ~HIGHLIGHTS;
	}
	if ((us_posbits&ARCS) != 0)
	{
		if (us_posarcs != NOARCPROTO)
		{
			nextname = describearcproto(us_posarcs);
			us_posarcs = us_posarcs->nextarcproto;
			return(nextname);
		}
		us_posbits &= ~ARCS;
	}
	if ((us_posbits&CELLS) != 0)
	{
		if (us_posnodeprotos != NONODEPROTO)
		{
			nextname = describenodeproto(us_posnodeprotos);
			us_posnodeprotos = us_posnodeprotos->nextnodeproto;
			return(nextname);
		}
		us_posbits &= ~CELLS;
	}
	if ((us_posbits&PRIMS) != 0)
	{
		if (us_posprims != NONODEPROTO)
		{
			nextname = us_posprims->protoname;
			us_posprims = us_posprims->nextnodeproto;
			return(nextname);
		}
		us_posbits &= ~PRIMS;
	}
	if ((us_posbits&PURES) != 0)
	{
		while (us_posprims != NONODEPROTO)
		{
			nextname = us_posprims->protoname;
			funct = (us_posprims->userbits&NFUNCTION)>>NFUNCTIONSH;
			bits = us_posprims->userbits & NNOTUSED;
			us_posprims = us_posprims->nextnodeproto;
			if (bits != 0) continue;
			if (funct == NPNODE) return(nextname);
		}
		us_posbits &= ~PURES;
	}
	if ((us_posbits&LIBS) != 0)
	{
		if (us_poslibs != NOLIBRARY)
		{
			nextname = us_poslibs->libname;
			us_poslibs = us_poslibs->nextlibrary;
			return(nextname);
		}
		us_posbits &= ~LIBS;
	}
	if ((us_posbits&LAYERS) != 0)
	{
		if (us_poslayers >= 0)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, us_layerletters(el_curtech, us_poslayers));
			addtoinfstr(infstr, '(');
			addstringtoinfstr(infstr, layername(el_curtech, us_poslayers));
			addtoinfstr(infstr, ')');
			us_poslayers--;
			return(returninfstr(infstr));
		}
		us_posbits &= ~LAYERS;
	}
	if ((us_posbits&POPUPS) != 0)
	{
		if (us_pospopupmenu != NOPOPUPMENU)
		{
			nextname = us_pospopupmenu->name;
			us_pospopupmenu = us_pospopupmenu->nextpopupmenu;
			return(nextname);
		}
		us_posbits &= ~POPUPS;
	}
	if ((us_posbits&CONSTRAINTS) != 0)
	{
		if (el_constraints[us_posconstraint].conname != 0)
		{
			nextname = el_constraints[us_posconstraint].conname;
			us_posconstraint++;
			return(nextname);
		}
		us_posbits &= ~CONSTRAINTS;
	}
	if ((us_posbits&MBUTTONS) != 0)
	{
		if (us_posmbuttons < buttoncount())
		{
			nextname = buttonname(us_posmbuttons, &unimportant);
			us_posmbuttons++;
			return(nextname);
		}
		us_posbits &= ~MBUTTONS;
	}
	if ((us_posbits&VIEWS) != 0)
	{
		if (us_posviews == (VIEW *)0)
		{
			us_posviews = el_views;
			return(x_("unknown"));
		}
		if (us_posviews != NOVIEW)
		{
			nextname = us_posviews->viewname;
			us_posviews = us_posviews->nextview;
			return(nextname);
		}
		us_posbits &= ~VIEWS;
	}
	if ((us_posbits&NODEFUN) != 0)
	{
		nextname = nodefunctionname(us_posfunction, NONODEINST);
		us_posfunction++;
		if (*nextname != 0) return(nextname);
		us_posbits &= ~NODEFUN;
	}
	if ((us_posbits&ARCFUN) != 0)
	{
		nextname = arcfunctionname(us_posfunction);
		us_posfunction++;
		if (*nextname != 0) return(nextname);
		us_posbits &= ~ARCFUN;
	}
	if ((us_posbits&EDTECSTUFF) == EDTECPORT)
	{
		while (us_posnodeprotos != NONODEPROTO)
		{
			nextname = us_posnodeprotos->protoname;
			us_posnodeprotos = us_posnodeprotos->nextnodeproto;
			if (namesamen(nextname, x_("arc-"), 4) == 0) return(&nextname[4]);
		}
		switch (us_posspecial++)
		{
			case 0: return(x_("PORT-ANGLE"));
			case 1: return(x_("PORT-ANGLE-RANGE"));
		}
		us_posbits &= ~EDTECSTUFF;
	}
	if ((us_posbits&EDTECSTUFF) == EDTECLAY)
	{
		if (us_postecedsequencepos < us_postecedsequencecount)
		{
			nextname = us_postecedsequence[us_postecedsequencepos]->protoname;
			us_postecedsequencepos++;
			return(&nextname[6]);
		}
		efree((CHAR *)us_postecedsequence);
		us_posbits &= ~EDTECSTUFF;
	}
	if ((us_posbits&EDTECSTUFF) == EDTECARC)
	{
		while (us_posnodeprotos != NONODEPROTO)
		{
			nextname = us_posnodeprotos->protoname;
			us_posnodeprotos = us_posnodeprotos->nextnodeproto;
			if (namesamen(nextname, x_("arc-"), 4) == 0) return(&nextname[4]);
		}
		us_posbits &= ~EDTECSTUFF;
	}
	if ((us_posbits&EDTECSTUFF) == EDTECNODE)
	{
		while (us_posnodeprotos != NONODEPROTO)
		{
			nextname = us_posnodeprotos->protoname;
			us_posnodeprotos = us_posnodeprotos->nextnodeproto;
			if (namesamen(nextname, x_("node-"), 5) == 0) return(&nextname[5]);
		}
		us_posbits &= ~EDTECSTUFF;
	}
	if ((us_posbits&EDTECSTUFF) == EDTECTECH)
	{
		while (us_postechs != NOTECHNOLOGY)
		{
			tech = us_postechs;
			us_postechs = us_postechs->nexttechnology;
			if ((tech->userbits&NONSTANDARD) == 0) return(tech->techname);
		}
		us_posbits &= ~EDTECSTUFF;
	}
	if ((us_posbits&NETWORKS) != 0)
	{
		nextname = nextnets();
		if (nextname != 0) return(nextname);
		us_posbits &= ~NETWORKS;
	}
	if ((us_posbits&WORDSET) != 0)
	{
		us_posbits &= ~WORDSET;
		return(x_("SET-MINIMUM-SIZE"));
	}
	if ((us_posbits&WORDCLEAR) != 0)
	{
		us_posbits &= ~WORDCLEAR;
		return(x_("CLEAR-MINIMUM-SIZE"));
	}
	if ((us_posbits&WORDNODE) != 0)
	{
		us_posbits &= ~WORDNODE;
		return(x_("NODE"));
	}
	if ((us_posbits&WORDARC) != 0)
	{
		us_posbits &= ~WORDARC;
		return(x_("ARC"));
	}
	if ((us_posbits&WORDNEXT) != 0)
	{
		us_posbits &= ~WORDNEXT;
		return(x_("NEXT-PROTO"));
	}
	if ((us_posbits&WORDPREV) != 0)
	{
		us_posbits &= ~WORDPREV;
		return(x_("PREV-PROTO"));
	}
	if ((us_posbits&WORDTHIS) != 0)
	{
		us_posbits &= ~WORDTHIS;
		return(x_("THIS-PROTO"));
	}
	return(0);
}

/* parsing routines for a var */
BOOLEAN us_topofvars(CHAR **c)
{
	REGISTER CHAR *pp, save, *zap;
	REGISTER INTBIG i, j;
	BOOLEAN res;
	CHAR *qual;
	INTBIG objaddr, objtype;
	REGISTER VARIABLE *var;

	/* ignore any "$" at the start of the variable */
	pp = *c;
	for(i = estrlen(pp)-1; i >= 0; i--) if (pp[i] == '$') break;
	if (pp[i] == '$')
	{
		/* see if the variable is in parenthesis (and is complete) */
		if (pp[++i] == '(')
		{
			i++;
			for(j=i; pp[j] != 0; j++) if (pp[j] == ')') return(FALSE);
		}
		pp = &pp[i];
	}

	/* look for point where ambiguity may exist */
	zap = pp;
	for(i = estrlen(pp)-1; i >= 0; i--) if (pp[i] == '.' || pp[i] == ':')
	{
		*c = &pp[i+1];
		zap = &pp[i];

		/* if this is a ":" construct, special case it */
		if (*zap == ':')
		{
			if (namesamen(pp, x_("technology"), i) == 0 && i >= 1)
			{
				us_posswitch = -2;   return(topoftechs(c));
			}
			if (namesamen(pp, x_("library"), i) == 0 && i >= 1)
			{
				us_posswitch = -3;   return(topoflibs(c));
			}
			if (namesamen(pp, x_("tool"), i) == 0 && i >= 2)
			{
				us_posswitch = -4;   return(topoftools(c));
			}
			if (namesamen(pp, x_("arc"), i) == 0 && i >= 2)
			{
				us_posswitch = -5;   return(us_topofarcs(c));
			}
			if (namesamen(pp, x_("node"), i) == 0 && i >= 1)
			{
				us_posswitch = -6;   return(us_topofnodes(c));
			}
			if (namesamen(pp, x_("primitive"), i) == 0 && i >= 1)
			{
				us_posswitch = -7;   return(us_topofprims(c));
			}
			if (namesamen(pp, x_("cell"), i) == 0 && i >= 1)
			{
				us_posswitch = -8;   return(us_topofcells(c));
			}
			if (namesamen(pp, x_("view"), i) == 0 && i >= 1)
			{
				us_posswitch = -10;   return(topofviews(c));
			}
			if (namesamen(pp, x_("net"), i) == 0 && i >= 2)
			{
				us_posswitch = -11;   return(topofnets(c));
			}
			if (namesamen(pp, x_("window"), i) == 0 && i >= 1)
			{
				us_posswitch = -12;   return(us_topofwindows(c));
			}
		}
		break;
	}

	/* now determine the context of this point */
	save = *zap;   *zap = 0;
	res = us_evaluatevariable(pp, &objaddr, &objtype, &qual);
	if (!res && *qual != 0)
	{
		var = getval(objaddr, objtype, -1, qual);
		if (var != NOVARIABLE)
		{
			objaddr = var->addr;   objtype = var->type;
		} else res = TRUE;
	}
	*zap = save;

	/* initialize lists and return the description */
	if (res) us_posswitch = VINTEGER; else
	{
		us_posswitch = objtype & VTYPE;
		if (us_posswitch != VUNKNOWN && us_posswitch != VINTEGER &&
			us_posswitch != VADDRESS && us_posswitch != VCHAR &&
			us_posswitch != VSTRING && us_posswitch != VFLOAT &&
			us_posswitch != VDOUBLE && us_posswitch != VFRACT &&
			us_posswitch != VSHORT && us_posswitch != VBOOLEAN &&
		        us_posswitch != VGENERAL)
				us_possearch = initobjlist(objaddr, objtype, FALSE);
	}
	return(TRUE);
}
CHAR *us_nextvars(void)
{
	VARIABLE *var;

	switch (us_posswitch)
	{
		case -2:          return(nexttechs());
		case -3:          return(nextlibs());
		case -4:          return(nexttools());
		case -10:         return(nextviews());
		case -11:         return(nextnets());
		case -5:
		case -6:
		case -7:
		case -8:
		case -12:         return(us_nextparse());
		case VNODEINST:
		case VNODEPROTO:
		case VPORTARCINST:
		case VPORTEXPINST:
		case VPORTPROTO:
		case VARCINST:
		case VARCPROTO:
		case VGEOM:
		case VLIBRARY:
		case VTECHNOLOGY:
		case VTOOL:
		case VRTNODE:
		case VNETWORK:
		case VVIEW:
		case VWINDOWPART:
		case VGRAPHICS:
		case VCONSTRAINT:
		case VWINDOWFRAME:
		case VPOLYGON:
			return(nextobjectlist(&var, us_possearch));
	}
	return(0);
}

BOOLEAN us_topofeditor(CHAR **c)
{
	us_possearch = 0;
	return(TRUE);
}

CHAR *us_nexteditor(void)
{
	REGISTER CHAR *nextname;

	nextname = us_editortable[us_possearch].editorname;
	us_possearch++;
	return(nextname);
}

/****************************** PARSING TABLES ******************************/

/* common parsing tables */
static KEYWORD onoffopt[] =
{
	{x_("on"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("off"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static KEYWORD yesnoopt[] =
{
	{x_("yes"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static KEYWORD yesnoalwaysopt[] =
{
	{x_("yes"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("always"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_yesnop = {yesnoopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("yes or no"), 0};
COMCOMP us_noyesp = {yesnoopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("yes or no"), 0};
COMCOMP us_noyesalwaysp = {yesnoalwaysopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("yes or no"), 0};
COMCOMP us_artlookp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("artwork look"), 0};
COMCOMP us_purelayerp = {NOKEYWORD, us_topofpures, us_nextparse, NOPARAMS,
	0, x_(" \t"), M_("primitive name"), 0};

/* for "arc" command */
COMCOMP us_arcnamep = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("local arc name"), 0};
static COMCOMP arcpropp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("arc constraints"), 0};
static KEYWORD arcpropaopt[] =
{
	{x_("add"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP arcpropap = {arcpropaopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("should this constraint replace or augment others"), 0};
static KEYWORD arccurveiopt[] =
{
	{x_("interactive"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP arccurveip = {arccurveiopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("should this curvature be done interactively"), 0};
static KEYWORD arcnotopt[] =
{
	{x_("manhattan"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("fixed-angle"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("rigid"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("slide"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("temp-rigid"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("directional"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("negated"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("ends-extend"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("center"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("curve"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("constraint"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("skip-tail"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("skip-head"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("name"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_arcnotp = {arcnotopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("arc style parameter to negate"), 0};
static KEYWORD arctogopt[] =
{
	{x_("fixed-angle"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("rigid"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("slide"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("directional"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("negated"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("ends-extend"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("skip-tail"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("skip-head"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_arctogp = {arctogopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("arc style parameter to toggle"), 0};
static KEYWORD arcopt[] =
{
	{x_("manhattan"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("fixed-angle"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("rigid"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("slide"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("temp-rigid"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("directional"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("negated"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("ends-extend"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("skip-head"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("skip-tail"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("center"),              1,{&arccurveip,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("curve"),               1,{&arccurveip,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("constraint"),          2,{&arcpropp,&arcpropap,NOKEY,NOKEY,NOKEY}},
	{x_("name"),                1,{&us_arcnamep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("not"),                 1,{&us_arcnotp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("toggle"),              1,{&us_arctogp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("reverse"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("shorten"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_arcp = {arcopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("arc style argument"), 0};

/* for "array" command */
COMCOMP us_arrayxp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("X array extent (or 'file' for array file)"), 0};
static COMCOMP arrayyp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Y array extent"), 0};
static COMCOMP arrayxop = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("X overlap distance"), 0};
static COMCOMP arrayyop = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Y overlap distance"), 0};
static KEYWORD arrayswitchopt[] =
{
	{x_("no-names"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("diagonal"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("only-drc-valid"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP arrayswitchp = {arrayswitchopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("array option switch"), M_("use names")};

/* for "bind" command */
extern COMCOMP us_userp;
INTBIG us_parambind(CHAR *i, COMCOMP *j[], CHAR c)
{ j[0] = &us_userp; return(1);}
COMCOMP us_bindkeyp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, us_parambind,
	INPUTOPT, x_(" \t"), M_("key to bind"), 0};
static COMCOMP bindmenugp = {NOKEYWORD, us_topofnodes, us_nextparse, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("glyph to use for menu display"), 0};
static COMCOMP bindmenump = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("message to place in menu display"), 0};
COMCOMP us_bindmenuxp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,us_parambind,
	0, x_(" \t"), M_("column of menu to bind"), 0};
COMCOMP us_bindmenuryp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("row of menu to bind"), 0};
static KEYWORD bindmenuxpopt[] =
{
	{x_("glyph"),      2,{&bindmenugp,&us_bindmenuryp,NOKEY,NOKEY,NOKEY}},
	{x_("message"),    2,{&bindmenump,&us_bindmenuryp,NOKEY,NOKEY,NOKEY}},
	{x_("background"), 2,{&bindmenump,&us_bindmenuryp,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP bindmenuyp = {bindmenuxpopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("row of menu to bind or 'glyph/message' specification"), 0};
COMCOMP us_bindbuttonp = {NOKEYWORD,us_topofmbuttons,us_nextparse,us_parambind,
	INPUTOPT, x_(" \t"), M_("mouse button to bind"), 0};
COMCOMP us_bindpoprep = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("entry in popup menu to bind"), 0};
static KEYWORD bindpopepopt[] =
{
	{x_("message"),    2,{&bindmenump,&us_bindpoprep,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP bindpopep = {bindpopepopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("entry in popup menu to bind or 'message' specification"), 0};
INTBIG us_parambindpop(CHAR *i, COMCOMP *j[], CHAR c)
{ j[0] = &bindpopep; j[1] = &us_userp; return(2);}
COMCOMP us_bindpopnp = {NOKEYWORD, us_topofpopupmenu, us_nextparse,
	us_parambindpop, INPUTOPT, x_(" \t"), M_("popup menu to bind"), 0};
static KEYWORD bindsetopt[] =
{
	{x_("button"),         1,{&us_bindbuttonp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("key"),            1,{&us_bindkeyp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("menu"),           2,{&bindmenuyp,&us_bindmenuxp,NOKEY,NOKEY,NOKEY}},
	{x_("popup"),          1,{&us_bindpopnp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("input-popup"),    1,{&us_bindpopnp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_bindsetp = {bindsetopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("what would you like to bind"), 0};
static KEYWORD bindnotopt[] =
{
	{x_("verbose"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP bindnotp = {bindnotopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("what binding condition would you like to negate"), 0};
static COMCOMP bindgbuttonp = {NOKEYWORD, us_topofmbuttons, us_nextparse,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("mouse button to get"), 0};
COMCOMP us_bindgkeyp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("key to get"), 0};
static COMCOMP bindgmenuxp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("column of menu to get"), 0};
static COMCOMP bindgmenuyp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("row of menu to get"), 0};
static COMCOMP bindgpopnp = {NOKEYWORD, us_topofpopupmenu, us_nextparse,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("popup menu to get"), 0};
static COMCOMP bindgpopip = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("index of popup menu to get"), 0};
static KEYWORD bindgetopt[] =
{
	{x_("button"),         1,{&bindgbuttonp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("key"),            1,{&us_bindgkeyp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("menu"),           2,{&bindgmenuyp,&bindgmenuxp,NOKEY,NOKEY,NOKEY}},
	{x_("popup"),          2,{&bindgpopnp,&bindgpopip,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP bindgetp = {bindgetopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("what binding would you like to obtain"), 0};
static COMCOMP bindgetlp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("command-interpreter letter to hold the binding"), 0};
static KEYWORD bindopt[] =
{
	{x_("verbose"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("not"),            1,{&bindnotp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("set"),            1,{&us_bindsetp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("get"),            2,{&bindgetlp,&bindgetp,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_bindp = {bindopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("binding option"), 0};

/* the "color" command */
COMCOMP us_colorentryp = {NOKEYWORD, us_topoflayers, us_nextparse, NOPARAMS,
	NOFILL|INPUTOPT, x_(" \t"), M_("table index or layer letter(s)"), 0};
static COMCOMP colorrvaluep = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("new value for red colormap entry"), 0};
static COMCOMP colorgvaluep = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("new value for green colormap entry"), 0};
static COMCOMP colorbvaluep = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("new value for blue colormap entry"), 0};
COMCOMP us_colorpvaluep = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("new value for raster pattern"), 0};
static COMCOMP colorhighap = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("amount to highlight (0 to 1)"), x_("0.5")};
COMCOMP us_colorhighp = {NOKEYWORD,us_topoflayers,us_nextparse,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("overlappable layer letter(s) to highlight"), 0};
COMCOMP us_colorreadp = {NOKEYWORD, topoffile, nextfile, NOPARAMS,
	NOFILL|INPUTOPT, x_(" \t"), M_("colormap file name"), 0};
COMCOMP us_colorwritep = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("colormap output file name"), 0};
static KEYWORD coloropt[] =
{
	{x_("default"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("black-background"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("white-background"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("entry"),            4,{&us_colorentryp,&colorrvaluep,&colorgvaluep,&colorbvaluep,NOKEY}},
	{x_("pattern"),          2,{&us_colorentryp,&us_colorpvaluep,NOKEY,NOKEY,NOKEY}},
	{x_("mix"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("highlight"),        2,{&us_colorhighp,&colorhighap,NOKEY,NOKEY,NOKEY}},
	{x_("read"),             1,{&us_colorreadp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("write"),            1,{&us_colorwritep,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_colorp = {coloropt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("colormap option"), 0};

/* the "commandfile" command */
static KEYWORD commandfileswitchopt[] =
{
	{x_("verbose"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP commandfileswitchp = {commandfileswitchopt, NOTOPLIST,
	NONEXTLIST, NOPARAMS, 0, x_(" \t"), M_("command file switch"), 0};
static COMCOMP commandfilep = {NOKEYWORD, topoffile, nextfile, NOPARAMS,
	NOFILL|INPUTOPT, x_(" \t"), M_("command file name"), 0};

/* the "constraint" command */
static COMCOMP constraintup = {NOKEYWORD, us_topofconstraints, us_nextparse,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("constraint solver to use"), 0};
static COMCOMP constraintap = {NOKEYWORD, NOTOPLIST, NONEXTLIST,
	us_paramconstraintb, 0, x_(" \t"), M_("message for constraint solver"), 0};
INTBIG us_paramconstraintb(CHAR *i, COMCOMP *j[], CHAR c)
	{ j[0]= &constraintap; return(1);}
INTBIG us_paramconstrainta(CHAR *pt, COMCOMP *j[], CHAR c)
{
	REGISTER INTBIG i;

	i = parse(pt, &constraintup, FALSE);
	if (i >= 0)
	{
		j[0] = el_constraints[i].parse;
		if (j[0] != NOCOMCOMP) return(1);
	}
	return(us_paramconstraintb(pt, j, c));
}
static COMCOMP constrainttp = {NOKEYWORD, us_topofconstraints, us_nextparse,
	us_paramconstrainta, INPUTOPT, x_(" \t"), M_("constraint to direct"), 0};
static KEYWORD constraintopt[] =
{
	{x_("tell"),     1,{&constrainttp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("use"),      1,{&constraintup,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP constraintp = {constraintopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("constraint solver action"), 0};

/* the "copycell" command */
COMCOMP us_copycellp = {NOKEYWORD, us_topofcells, us_nextparse, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("source cell name (or library:cell)"), 0};
COMCOMP us_copycelldp = {NOKEYWORD,us_topofcells,us_nextparse,NOPARAMS,
	NOFILL|INCLUDENOISE, x_(" \t"), M_("destination cell name"), M_("same cell name")};
extern COMCOMP copycellqp;
static KEYWORD copycellqopt[] =
{
	{x_("quiet"),                 1,{&copycellqp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("move"),                  1,{&copycellqp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("show-copy"),             1,{&copycellqp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("replace-copy"),          1,{&copycellqp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no-related-views"),      1,{&copycellqp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no-subcells"),           1,{&copycellqp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("use-existing-subcells"), 1,{&copycellqp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP copycellqp = {copycellqopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("options"), 0};

/* the "create" command */
static COMCOMP createap = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("angle increment from current object"), 0};
static COMCOMP createjp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("angle increment from current object to other object"), 0};
static COMCOMP createxp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("X coordinate of object"), 0};
static COMCOMP createyp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("Y coordinate of object"), 0};
static KEYWORD createopt[] =
{
	{x_("remain-highlighted"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("wait-for-down"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("contents"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("insert"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("icon"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("breakpoint"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("angle"),              1,{&createap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("join-angle"),         1,{&createjp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("to"),                 2,{&createxp,&createyp,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP createp = {createopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("creation option"), 0};

/* the "debug" command */
static KEYWORD debugcopt[] =
{
	{x_("verbose"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_debugcp = {debugcopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("database checking option"), 0};
static COMCOMP debugap = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("arena name to map"), 0};
static KEYWORD debugopt[] =
{
	/* standard */
	{x_("arena"),                   1,{&debugap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("check-database"),          1,{&us_debugcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("dialog-edit"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("examine-options"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("freeze-user-interface"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("internal-errors"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("namespace"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("options-changed"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("rtree"),                   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("translate"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("undo"),                    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	/* special case */
	{x_("erase-bits"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("prepare-tsmc-io"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("label-cell"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("make-parameters-visible"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("postscript-schematics"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP debugp = {debugopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("debugging option"), 0};

/* the "defarc" command */
COMCOMP us_defarcwidp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("default arc width"), 0};
COMCOMP us_defarcangp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("default arc angle increment"), 0};
COMCOMP us_defarcpinp = {NOKEYWORD, us_topofprims, us_nextparse, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("default pin to use for this arc"), 0};
static KEYWORD defarcnotopt[] =
{
	{x_("manhattan"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("fixed-angle"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("slide"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("rigid"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("directional"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("negated"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("ends-extend"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_defarcnotp = {defarcnotopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("default arc setting"), 0};
static KEYWORD defarcopt[] =
{
	{x_("not"),              1,{&us_defarcnotp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("default"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("angle"),            1,{&us_defarcangp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("pin"),              1,{&us_defarcpinp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("width"),            1,{&us_defarcwidp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("manhattan"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("fixed-angle"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("slide"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("rigid"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("directional"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("negated"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("ends-extend"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP defarcp = {defarcopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("default arc setting"), M_("describe current")};
COMCOMP us_defarcsp = {NOKEYWORD,topofarcs,nextarcs,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("arc prototype to set defaults (* for all)"), 0};

/* the "defnode" command */
COMCOMP us_defnodexsp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("default node X size"), 0};
COMCOMP us_defnodeysp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("default node Y size"), 0};
static COMCOMP defnodepp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("default node placement"), M_("90 degrees more")};
static KEYWORD defnodenotopt[] =
{
	{x_("alterable"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("centered-primitives"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("check-dates"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("copy-ports"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("expanded"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("from-cell-library"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("instances-locked"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("locked-primitives"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_defnodenotp = {defnodenotopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("default node NOT setting"), 0};
static KEYWORD defnodeopt[] =
{
	{x_("alterable"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("centered-primitives"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("check-dates"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("copy-ports"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("expanded"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("from-cell-library"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("instances-locked"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("locked-primitives"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("not"),                 1,{&us_defnodenotp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("placement"),           1,{&defnodepp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("size"),                2,{&us_defnodexsp,&us_defnodeysp,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_defnodep = {defnodeopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("default node setting"), M_("describe current")};
COMCOMP us_defnodesp = {NOKEYWORD, us_topofnodes, us_nextparse, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("node prototype to set defaults (* for all)"), 0};

/* the "echo" command */
static COMCOMP echop = {NOKEYWORD,NOTOPLIST,NONEXTLIST,us_nextecho,
	0, x_(" \t"), M_("argument to be echoed"), 0};
INTBIG us_nextecho(CHAR *i, COMCOMP *j[], CHAR c)
{ j[0] = &echop; return(1); }

/* the "editcell" command */
extern COMCOMP editcellwindowp;
static KEYWORD editcellwindowopt[] =
{
	{x_("non-redundant"),    1,{&editcellwindowp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("new-window"),       1,{&editcellwindowp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("in-place"),         1,{&editcellwindowp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP editcellwindowp = {editcellwindowopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("option to display in a new window"), 0};
COMCOMP us_editcellp = {NOKEYWORD,us_topofcells,us_nextparse,NOPARAMS,
	NOFILL|INPUTOPT|INCLUDENOISE, x_(" \t"), M_("cell cell to be edited ('-' to follow ports)"), 0};

/* the "erase" command */
static KEYWORD eraseopt[] =
{
	{x_("geometry"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clean-up"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clean-up-all"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("pass-through"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP erasep = {eraseopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("erase option"), 0};

/* the "find" command */
COMCOMP us_findnamep = {NOKEYWORD, us_topofhighlight, us_nextparse,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("highlight name"), 0};
COMCOMP us_findnnamep = {NOKEYWORD, us_topofhighlight, us_nextparse,
	NOPARAMS, NOFILL|INPUTOPT|INCLUDENOISE, x_(" \t"), M_("highlight name"), 0};
COMCOMP us_findnodep = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("name of node to highlight"), 0};
COMCOMP us_findexportp = {NOKEYWORD, us_topofcports, us_nextparse, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("name of export to highlight"), 0};
COMCOMP us_findarcp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("name of arc/network to highlight"), 0};
COMCOMP us_findintp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("angle increment from current position"), 0};
COMCOMP us_findobjap = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("address of object being selected"), 0};
static COMCOMP findvp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("displayable variable name to select"), 0};
static KEYWORD findsnapopt[] =
{
	{x_("none"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("center"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("midpoint"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("endpoint"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("tangent"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("perpendicular"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("quadrant"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("intersection"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP findsnapp = {findsnapopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("vertex snapping mode"), 0};
static KEYWORD findallopt[] =
{
	{x_("easy"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("hard"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP findallp = {findallopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("select-all option"), 0};
static KEYWORD finddropt[] =
{
	{x_("any-touching"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("only-enclosed"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP finddrp = {finddropt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("what dragging selects"), 0};
static KEYWORD findardopt[] =
{
	{x_("wait"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP findardp = {findardopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("whether to wait for a push"), 0};
extern COMCOMP findp, us_varbs2typ;
static KEYWORD findopt[] =
{
	{x_("all"),                1,{&findallp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("another"),            1,{&findp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("arc"),                1,{&us_findarcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("area-move"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("area-define"),        1,{&findardp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("area-size"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clear"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("comp-interactive"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("constraint-angle"),   1,{&us_findintp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("deselect-arcs"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("down-stack"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("dragging-selects"),   1,{&finddrp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("exclusively"),        1,{&findp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("export"),             1,{&us_findexportp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("extra-info"),         1,{&findp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("interactive"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("just-objects"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("more"),               1,{&findp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("name"),               1,{&us_findnamep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no-box"),             1,{&findp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("node"),               1,{&us_findnodep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("nonmanhattan"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("object"),             2,{&us_varbs2typ,&us_findobjap,NOKEY,NOKEY,NOKEY}},
	{x_("port"),               1,{&findp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("save"),               1,{&us_findnnamep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("set-easy-selection"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("set-hard-selection"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("similar"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("snap-mode"),          1,{&findsnapp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("special"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("still"),              1,{&findp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("up-stack"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("variable"),           1,{&findvp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("vertex"),             1,{&findp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("within"),             1,{&findp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP findp = {findopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("selection option"), 0};

/* the "getproto" command */
COMCOMP us_getproto1p = {NOKEYWORD, us_topofgetproto, us_nextparse, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("arc or node prototype"), 0};
static COMCOMP getproto2p = {NOKEYWORD, us_topofarcnodes, us_nextparse,
	NOPARAMS, 0, x_(" \t"), M_("arc or node prototype"), 0};

/* the "grid" command */
COMCOMP us_gridalip = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("grid cursor alignment value"), 0};
static COMCOMP gridedgesp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("grid edge alignment value"), 0};
static COMCOMP gridxp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("grid spacing in X"), 0};
static COMCOMP gridyp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("grid spacing in Y"), M_("same as X")};
static KEYWORD gridopt[] =
{
	{x_("alignment"),        1,{&us_gridalip,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("edges"),            1,{&gridedgesp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("size"),             2,{&gridxp,&gridyp,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_gridp = {gridopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("grid option"), M_("toggle grid")};

/* the "help" command */
static KEYWORD helpopt[] =
{
	{x_("manual"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("news"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("illustrate"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("pulldowns"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_helpp = {helpopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("news, manual, pulldowns, or illustrate"), M_("command-line help")};

/* the "if" command */
static KEYWORD ifropt[] =
{
	{x_("=="),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("!="),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("<"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("<="),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_(">"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_(">="),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP ifrp = {ifropt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("conditional relationship"), 0};
static COMCOMP iftp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("value to be compared"), 0};

/* the "interpret" command */
static COMCOMP interpretfip = {NOKEYWORD,topoffile, nextfile,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("file with code to execute"), 0};
static KEYWORD interpretfopt[] =
{
	{x_("lisp"),        1,{&interpretfip,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("tcl"),         1,{&interpretfip,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("java"),        1,{&interpretfip,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP interpretfp = {interpretfopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("language to execute"), 0};
COMCOMP us_interpretcp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("code to execute"), M_("invoke interpreter")};
static KEYWORD interpretopt[] =
{
	{x_("lisp"),        1,{&us_interpretcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("tcl"),         1,{&us_interpretcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("java"),        1,{&us_interpretcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("file"),        1,{&interpretfp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP interpretp = {interpretopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("language or code to execute"), M_("invoke interpreter")};

/* the "iterate" command */
static COMCOMP iteratemp = {NOKEYWORD, us_topofmacros, us_nextparse, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("macro to execute"), 0};
static COMCOMP iteratep = {NOKEYWORD, us_topofvars, us_nextvars, NOPARAMS,
	NOFILL|INPUTOPT|INCLUDENOISE, x_(" \t"), M_("number of times to repeat last command, or array variable to iterate"), x_("1")};

/* the "killcell" command */
static COMCOMP killcellp = {NOKEYWORD, topofcells, nextcells, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("name of cell cell to delete"), 0};

/* the "lambda" command */
COMCOMP us_lambdachp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("new value for lambda"), 0};
static KEYWORD lambdadunopt[] =
{
	{x_("lambda"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("inch"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("centimeter"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("millimeter"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("mil"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("micron"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("centimicron"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("nanometer"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP lambdadunp = {lambdadunopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("units to use when displaying distance"), 0};
static KEYWORD lambdaiunopt[] =
{
	{x_("half-decimicron"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("half-nanometer"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP lambdaiunp = {lambdaiunopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("smallest internal unit"), 0};
static KEYWORD lambdaopt[] =
{
	{x_("change-tech"),     1,{&us_lambdachp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("change-lib"),      1,{&us_lambdachp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("change-all-libs"), 1,{&us_lambdachp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("display-units"),   1,{&lambdadunp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("internal-units"),  1,{&lambdaiunp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP lambdap = {lambdaopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("lambda changing option"), M_("print current values")};

/* the "library" command */
COMCOMP us_libraryup = {NOKEYWORD, topoflibs, nextlibs, NOPARAMS,
	NOFILL|INPUTOPT|INCLUDENOISE, x_(" \t"), M_("name of library to use"), 0};
COMCOMP us_librarynp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("name of new library to create"), 0};
COMCOMP us_libraryrp = {NOKEYWORD,topoflibfile,nextfile,NOPARAMS,
	NOFILL|INPUTOPT, x_(" \t"), M_("name of library file to read"), 0};
COMCOMP us_librarywp = {NOKEYWORD, topoflibs, nextlibs, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("name of library to write"), 0};
static KEYWORD librarymcopt[] =
{
	{x_("make-current"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("merge"),        1,{&us_libraryup,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP libraryreadmcp = {librarymcopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("option to library input"), 0};
static KEYWORD libraryreadformatopt[] =
{
	{x_("binary"),       1,{&libraryreadmcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("cif"),          1,{&libraryreadmcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("def"),          1,{&libraryreadmcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("dxf"),          1,{&libraryreadmcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("edif"),         1,{&libraryreadmcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("gds"),          1,{&libraryreadmcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("lef"),          1,{&libraryreadmcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("sue"),          1,{&libraryreadmcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("text"),         1,{&libraryreadmcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("vhdl"),         1,{&libraryreadmcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("make-current"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("merge"),        1,{&us_libraryup,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP libraryreadformatp = {libraryreadformatopt, NOTOPLIST,
	NONEXTLIST, NOPARAMS, 0, x_(" \t"), M_("library read option"), x_("binary")};
static KEYWORD librarywriteformatopt[] =
{
	{x_("binary"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("cif"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("dxf"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("eagle"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("ecad"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("edif"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("gds"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("hpgl"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("l"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("lef"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("pads"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("postscript"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("quickdraw"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("skill"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("text"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_librarywriteformatp = {librarywriteformatopt,NOTOPLIST,
	NONEXTLIST,NOPARAMS, 0, x_(" \t"), M_("library write option"), x_("binary")};
static KEYWORD libraryksopt[] =
{
	{x_("safe"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP libraryksp = {libraryksopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("option to kill without saving"), M_("check first")};
COMCOMP us_librarykp = {NOKEYWORD, topoflibs, nextlibs, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("name of library to erase"), M_("current")};
COMCOMP us_librarydp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("default path for system libraries"), 0};
static KEYWORD libraryopt[] =
{
	{x_("use"),               1,{&us_libraryup,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("new"),               1,{&us_librarynp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("read"),              2,{&us_libraryrp,&libraryreadformatp,NOKEY,NOKEY,NOKEY}},
	{x_("write"),             2,{&us_librarywp,&us_librarywriteformatp,NOKEY,NOKEY,NOKEY}},
	{x_("save"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("purge"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("touch"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("kill"),              2,{&us_librarykp,&libraryksp,NOKEY,NOKEY,NOKEY}},
	{x_("default-path"),      1,{&us_librarydp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_libraryp = {libraryopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("library control option"), 0};

/* the "macbegin" command */
COMCOMP us_macparamp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("parameter to macro"), 0};
COMCOMP us_macbeginnp = {NOKEYWORD, us_topofmacros, us_nextparse, NOPARAMS,
	NOFILL|INPUTOPT|INCLUDENOISE, x_(" \t"), M_("macro to be defined"), 0};
extern COMCOMP macbeginop;
static KEYWORD macbeginopt[] =
{
	{x_("verbose"),     1,{&macbeginop,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no-execute"),  1,{&macbeginop,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP macbeginop = {macbeginopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("macro definition option"), 0};

/* the "menu" command */
static KEYWORD menuxopt[] =
{
	{x_("auto"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP menuxp = {menuxopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("number of columns in menu (or 'auto')"), 0};
static COMCOMP menuyp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("number of rows in menu"), 0};
static KEYWORD menusizeopt[] =
{
	{x_("size"),    2,{&menuxp,&menuyp,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP menusizep = {menusizeopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("menu size option"), 0};
static COMCOMP menupopsp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("number of entries in popup menu"), 0};
static COMCOMP menupophp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("header for popup menu"), 0};
static KEYWORD menupopopt[] =
{
	{x_("size"),    1,{&menupopsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("header"),  1,{&menupophp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP menupopp = {menupopopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("pop-up menu option"), 0};
static COMCOMP menupopnp = {NOKEYWORD, us_topofpopupmenu, us_nextparse, NOPARAMS,
	INPUTOPT|INCLUDENOISE, x_(" \t"), M_("name of popup menu"), 0};
extern COMCOMP us_userp, menusbp;
INTBIG us_parammenusbp(CHAR *i, COMCOMP *j[], CHAR c)
{ j[0] = &menusbp; return(1); }
COMCOMP menusbp = {NOKEYWORD, us_topofpopupmenu, us_nextparse, us_parammenusbp,
	INPUTOPT, x_(" \t"), M_("popup menu name for menu bar"), 0};
static KEYWORD menuopt[] =
{
	{x_("on"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("off"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("popup"),      2,{&menupopnp,&menupopp,NOKEY,NOKEY,NOKEY}},
	{x_("dopopup"),    1,{&us_userp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("setmenubar"), 1,{&menusbp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("top"),        1,{&menusizep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("bottom"),     1,{&menusizep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("left"),       1,{&menusizep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("right"),      1,{&menusizep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("size"),       2,{&menuxp,&menuyp,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_menup = {menuopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("menu control option"), 0};

/* the "mirror" command */
static KEYWORD mirrorgopt[] =
{
	{x_("about-grab-point"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("about-trace-point"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("sensibly"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP mirrorgp = {mirrorgopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("option to mirror about grab-point"), 0};
static KEYWORD mirroropt[] =
{
	{x_("horizontal"),    1,{&mirrorgp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("vertical"),      1,{&mirrorgp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_mirrorp = {mirroropt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("direction of mirroring"), 0};

/* the "move" command */
static KEYWORD movevaopt[] =
{
	{x_("top"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("bottom"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("center"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP movevap = {movevaopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("direction of vertical alignment"), 0};
static KEYWORD movehaopt[] =
{
	{x_("left"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("right"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("center"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP movehap = {movehaopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("direction of horizontal alignment"), 0};
static COMCOMP moveap = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("amount to move"), 0};
static COMCOMP movexp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("X position or offset (or 'dialog')"), 0};
static COMCOMP moveyp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("Y position or offset"), 0};
static COMCOMP moverap = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("angle increment from current position"), 0};
static KEYWORD moveopt[] =
{
	{x_("left"),       1,{&moveap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("right"),      1,{&moveap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("up"),         1,{&moveap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("down"),       1,{&moveap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("to"),         2,{&movexp,&moveyp,NOKEY,NOKEY,NOKEY}},
	{x_("by"),         2,{&movexp,&moveyp,NOKEY,NOKEY,NOKEY}},
	{x_("angle"),      1,{&moverap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("valign"),     1,{&movevap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("halign"),     1,{&movehap,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP movep = {moveopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("object motion options"), 0};

/* the "node" command */
static COMCOMP nodeexpp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("number of levels of depth to expand"), M_("infinite")};
static COMCOMP nodeuexpp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("number of levels of depth to un-expand"), M_("infinite")};
static COMCOMP nodenp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("local name to give node"), 0};
static COMCOMP nodetaxp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("X coordinate of trace point"), 0};
static COMCOMP nodetayp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Y coordinate of trace point"), 0};
static COMCOMP nodetcadp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Degrees of circle (default is 360)"), 0};
static COMCOMP nodetcarp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Resolution of annulus (number of segments to use)"), 0};
static COMCOMP nodetcaop = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Outer radius of annulus"), 0};
COMCOMP us_nodetcaip = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Inner radius of annulus"), 0};
COMCOMP us_nodetptmp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Message to place"), 0};
static COMCOMP nodetptsp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Size of text"), 0};
COMCOMP us_nodetptlp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Layer of text"), 0};
extern COMCOMP us_nodetp;
static KEYWORD nodetopt[] =
{
	{x_("store-trace"),       1,{&us_nodetp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("add-point"),         2,{&nodetaxp,&nodetayp,NOKEY,NOKEY,NOKEY}},
	{x_("move-point"),        2,{&nodetaxp,&nodetayp,NOKEY,NOKEY,NOKEY}},
	{x_("delete-point"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("wait-for-down"),     1,{&us_nodetp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("construct-annulus"), 4,{&us_nodetcaip,&nodetcaop,&nodetcarp,&nodetcadp,NOKEY}},
	{x_("place-text"),        3,{&us_nodetptlp,&nodetptsp,&us_nodetptmp,NOKEY,NOKEY}},
	{x_("fillet"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("next-point"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("prev-point"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("init-points"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_nodetp = {nodetopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("cursor trace option"), 0};
static KEYWORD nodenotopt[] =
{
	{x_("expand"),       1,{&nodeuexpp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("name"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP nodenotp = {nodenotopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("node state NOT options"), 0};
static KEYWORD nodeopt[] =
{
	{x_("cover-implant"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("regrid-cell"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("expand"),         1,{&nodeexpp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("name"),           1,{&nodenp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("trace"),          1,{&us_nodetp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("not"),            1,{&nodenotp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_nodep = {nodeopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("node state options"), 0};

/* the "offtool" command */
static KEYWORD offtooleopt[] =
{
	{x_("permanently"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP offtoolep = {offtooleopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("option to make this change nonundoable"), 0};
static COMCOMP offtoolp = {NOKEYWORD, topoftools, nexttools, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("tool to turn off"), 0};

/* the "ontool" command */
static COMCOMP ontoolp = {NOKEYWORD, topoftools, nexttools, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("tool to turn on"), 0};
COMCOMP us_onofftoolp = {NOKEYWORD, topoftools, nexttools, NOPARAMS,
	0, x_(" \t"), M_("tool to control"), 0};

/* the "outhier" command */
static COMCOMP outhierp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("number of levels to pop out"), x_("1")};

/* the "package" command */
COMCOMP us_packagep = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("name of new cell in which to package this circuitry"), 0};

/* the "port" command */
static COMCOMP portunp = {NOKEYWORD, us_topofexpports, us_nextparse, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("name of existing export"), 0};
extern COMCOMP us_portcp;
static KEYWORD portmodopt[] =
{
	{x_("input"),         1,{&us_portcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("output"),        1,{&us_portcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("bidirectional"), 1,{&us_portcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("power"),         1,{&us_portcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("ground"),        1,{&us_portcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clock"),         1,{&us_portcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clock1"),        1,{&us_portcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clock2"),        1,{&us_portcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clock3"),        1,{&us_portcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clock4"),        1,{&us_portcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clock5"),        1,{&us_portcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clock6"),        1,{&us_portcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("refin"),         1,{&us_portcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("refout"),        1,{&us_portcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("refbase"),       1,{&us_portcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("none"),          1,{&us_portcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("always-drawn"),  1,{&us_portcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("body-only"),     1,{&us_portcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("specify"),       1,{&us_portcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("use"),           2,{&portunp,&us_portcp,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_portcp = {portmodopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("port specification and characteristics"), 0};
static COMCOMP portecup = {NOKEYWORD, us_topofports, us_nextparse, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("name of port to export"), 0};
extern COMCOMP portecp;
static COMCOMP porterp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("reference port name"), 0};
static KEYWORD portmodeopt[] =
{
	{x_("input"),         1,{&portecp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("output"),        1,{&portecp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("bidirectional"), 1,{&portecp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("power"),         1,{&portecp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("ground"),        1,{&portecp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clock"),         1,{&portecp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clock1"),        1,{&portecp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clock2"),        1,{&portecp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clock3"),        1,{&portecp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clock4"),        1,{&portecp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clock5"),        1,{&portecp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clock6"),        1,{&portecp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("refin"),         1,{&portecp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("refout"),        1,{&portecp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("refname"),       2,{&porterp,&portecp,NOKEY,NOKEY,NOKEY}},
	{x_("refbase"),       1,{&portecp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("always-drawn"),  1,{&portecp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("body-only"),     1,{&portecp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("specify"),       1,{&portecp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("use"),           2,{&portecup,&portecp,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP portecp = {portmodeopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("port specification and characteristics"), 0};
INTBIG us_paramportep(CHAR *i, COMCOMP *j[], CHAR c)
{ j[0] = &portecp; return(1); }
COMCOMP us_portep = {NOKEYWORD, NOTOPLIST, NONEXTLIST, us_paramportep,
	INPUTOPT, x_(" \t"), M_("export name"), 0};
static KEYWORD portuopt[] =
{
	{x_("all"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("geometry"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("specify"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("use"),           1,{&portunp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP portup = {portuopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("port specification"), 0};
static KEYWORD portnopt[] =
{
	{x_("export"),        1,{&portup,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP portnp = {portnopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("port un-request"), 0};
static KEYWORD portlopt[] =
{
	{x_("short"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("long"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("crosses"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_portlp = {portlopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("port/export label display option"), 0};
static KEYWORD portmopt[] =
{
	{x_("remain-highlighted"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP portmp = {portmopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("port move highlighting option"), 0};
static COMCOMP portsp = {NOKEYWORD,topoflibs,nextlibs,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("library to synchronize with this"), 0};
static KEYWORD portopt[] =
{
	{x_("export"),                 1,{&us_portep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("not"),                    1,{&portnp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("re-export-all"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("highlighted-re-export"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("power-ground-re-export"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("synchronize-library"),    1,{&portsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("move"),                   1,{&portmp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("change"),                 1,{&us_portcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("labels"),                 1,{&us_portlp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("export-labels"),          1,{&us_portlp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("identify-cell"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("identify-node"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_portp = {portopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("control of ports"), 0};

/* the "quit" command */
static COMCOMP quitp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("exit the program"), 0};

/* the "rename" command */
COMCOMP us_renameop = {NOKEYWORD, us_topofallthings, us_nextparse, NOPARAMS,
	NOFILL|INPUTOPT, x_(" \t"), M_("name of object to be renamed"), 0};
COMCOMP us_renamenp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("new name for object"), 0};
COMCOMP us_renamecp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("which type of object(s) do you want renamed"), 0};

/* the "replace" command */
extern COMCOMP replacesp;
static KEYWORD replacesopt[] =
{
	{x_("universally"),         1,{&replacesp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("this-cell"),           1,{&replacesp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("this-library"),        1,{&replacesp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("connected"),           1,{&replacesp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("nodes-too"),           1,{&replacesp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("ignore-port-names"),   1,{&replacesp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("allow-missing-ports"), 1,{&replacesp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP replacesp = {replacesopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("replacement option"), 0};
COMCOMP us_replacep = {NOKEYWORD, us_topofarcnodes, us_nextparse, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("new prototype to use instead of this"), 0};

/* the "rotate" command */
static KEYWORD rotatemopt[] =
{
	{x_("sensibly"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("about-grab-point"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("about-trace-point"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP rotatemp = {rotatemopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("rotation option"), 0};
static KEYWORD rotateopt[] =
{
	{x_("sensibly"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("about-grab-point"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("about-trace-point"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("more"),              1,{&rotatemp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP rotatep = {rotateopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("rotation option"), 0};
static KEYWORD rotateaopt[] =
{
	{x_("interactively"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP rotateap = {rotateaopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("new angle or angle increment"), 0};

/* the "show" command */
static COMCOMP showbpp = {NOKEYWORD, us_topofpopupmenu, us_nextparse, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Popup menu to display"), 0};
static KEYWORD showbopt[] =
{
	{x_("short"),                 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("all"),                   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("key"),                   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("menu"),                  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("button"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("popup"),                 1,{&showbpp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP showbp = {showbopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("binding information to show"), 0};
static KEYWORD showeopt[] =
{
	{x_("authors"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_showep = {showeopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("option to list authors"), 0};
static KEYWORD showropt[] =
{
	{x_("current"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("next"),                  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("last"),                  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_showrp = {showropt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("error to show"), 0};
static COMCOMP showclp = {NOKEYWORD, topoflibs, nextlibs, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("library to show"), 0};
static COMCOMP showcmp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("cell names to show (with wildcards)"), 0};
extern COMCOMP showcp;
static KEYWORD showcopt[] =
{
	{x_("library"),           2,{&showclp,&showcp,NOKEY,NOKEY,NOKEY}},
	{x_("matching"),          2,{&showcmp,&showcp,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP showcp = {showcopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("cell listing option"), 0};
COMCOMP us_showdp = {NOKEYWORD, topofcells, nextcells, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("name of cell to show dates"), 0};
static COMCOMP showfmp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("cell names to show (with wildcards)"), 0};
static COMCOMP showfdp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("file to create with cell information"), 0};
extern COMCOMP showfep;
static KEYWORD showfeopt[] =
{
	{x_("dates"),             1,{&showfep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("library"),           2,{&showclp,&showfep,NOKEY,NOKEY,NOKEY}},
	{x_("edit"),              1,{&showfep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("matching"),          2,{&showfmp,&showfep,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP showfep = {showfeopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("cell listing"), 0};
static KEYWORD showfopt[] =
{
	{x_("placeholders"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("graphically"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("from-here-graphically"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("contained-in-this"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("recursive-nodes"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("not-below"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("dates"),                 1,{&showfep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("file"),                  2,{&showfdp,&showfep,NOKEY,NOKEY,NOKEY}},
	{x_("library"),               2,{&showclp,&showfep,NOKEY,NOKEY,NOKEY}},
	{x_("edit"),                  1,{&showfep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("matching"),              2,{&showfmp,&showfep,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_showfp = {showfopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("cell listing option"), 0};
static COMCOMP showmp = {NOKEYWORD, us_topofmacros, us_nextparse, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("macro to show"), 0};
static KEYWORD showoopt[] =
{
	{x_("short"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("long"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_showop = {showoopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("amount of information to describe about found object(s)"), 0};
extern COMCOMP showpp;
static KEYWORD showpopt[] =
{
	{x_("clock"),        1,{&showpp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("input"),        1,{&showpp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("output"),       1,{&showpp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("bidirectional"),1,{&showpp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("power"),        1,{&showpp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("ground"),       1,{&showpp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("generic"),      1,{&showpp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("reference"),    1,{&showpp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("all"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP showpp = {showpopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("port type to be shown"), 0};
static COMCOMP showhp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("which batch do you wish to examine"), M_("all")};
COMCOMP us_showup = {NOKEYWORD, topofcells, nextcells, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("name of cell to show usage"), 0};
static COMCOMP showvp = {NOKEYWORD, topofcells, nextcells, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("cell whose views are to be shown"), 0};
static KEYWORD showopt[] =
{
	{x_("tools"),                 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("bindings"),              1,{&showbp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("cells"),                 1,{&us_showfp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("coverage"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("dates"),                 1,{&us_showdp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("environment"),           1,{&us_showep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("error"),                 1,{&us_showrp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("history"),               1,{&showhp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("libraries"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("macros"),                1,{&showmp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("networks"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("object"),                1,{&us_showop,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("ports"),                 1,{&showpp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("primitives"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("solvers"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("technologies"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("usage"),                 1,{&us_showup,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("views"),                 1,{&showvp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_showp = {showopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("information to show"), 0};

/* the "size" command */
COMCOMP us_sizeyp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("new Y size of node"), 0};
COMCOMP us_sizewp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("new width of arc"), 0};
INTBIG us_paramsize(CHAR *word, COMCOMP *arr[], CHAR breakc)
{
	REGISTER GEOM **list;
	REGISTER INTBIG i;

	if (*word == 0) return(0);

	/* if known keywords are used, no more parsing */
	if (namesame(word, x_("corner-fixed")) == 0 || namesame(word, x_("center-fixed")) == 0 ||
		namesame(word, x_("grab-point-fixed")) == 0 || namesame(word, x_("nodes")) == 0 ||
		namesame(word, x_("arcs")) == 0 || namesame(word, x_("use-transformation")) == 0) return(0);

	/* get list of highlighted objects */
	list = us_gethighlighted(WANTARCINST|WANTNODEINST, 0, 0);
	if (list[0] == NOGEOM) return(0);
	for(i=0; list[i] != NOGEOM; i++) if (list[i]->entryisnode)
	{
		/* node found: get Y size too */
		arr[0] = &us_sizeyp;
		return(1);
	}
	return(0);
}
static KEYWORD sizexopt[] =
{
	{x_("corner-fixed"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("center-fixed"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("grab-point-fixed"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("nodes"),                   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("arcs"),                    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("use-transformation"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_sizep = {sizexopt, NOTOPLIST, NONEXTLIST, us_paramsize,
	INPUTOPT, x_(" \t"), M_("new width of arc, new X size of node, or option"), 0};

/* the "spread" command */
static KEYWORD spreaddopt[] =
{
	{x_("left"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("right"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("up"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("down"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_spreaddp = {spreaddopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("direction in which to spread open layout"), 0};
static COMCOMP spreadap = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("distance to spread open layout"), M_("design-rule spacing")};

/* the "system" command */
static KEYWORD systemopt[] =
{
	{x_("setstatusfont"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
#ifdef	WIN32
	{x_("print"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
#endif
	TERMKEY
};
static COMCOMP systemp = {systemopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("special system-dependent option"), 0};

/* the "technology" command */
COMCOMP us_technologycnnp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("new technology name"), 0};
static COMCOMP technologyetlp = {NOKEYWORD, us_topofedtectech, us_nextparse,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("technology to edit"), 0};
static COMCOMP technologyeppp = {NOKEYWORD, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("name of this port"), 0};
static KEYWORD technologyepopt[] =
{
	{x_("port"),               1,{&technologyeppp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("highlight"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("rectangle-filled"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("rectangle-outline"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("rectangle-crossed"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("polygon-filled"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("polygon-outline"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("lines-solid"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("lines-dotted"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("lines-dashed"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("lines-thicker"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("circle-outline"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("circle-filled"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("circle-half"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("circle-arc"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("text"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP technologyepp = {technologyepopt, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("shape/type of layer to create"), 0};
static KEYWORD technologyclcopt[] =
{
	{x_("none"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("transparent-1"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("transparent-2"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("transparent-3"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("transparent-4"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("transparent-5"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("white"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("black"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("red"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("blue"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("green"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("cyan"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("magenta"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("yellow"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("gray"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("orange"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("purple"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("brown"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("light-gray"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("dark-gray"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("light-red"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("dark-red"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("light-green"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("dark-green"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("light-blue"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("dark-blue"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP technologyclcp = {technologyclcopt, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("New color for this layer"), 0};
static KEYWORD technologyclsopt[] =
{
	{x_("solid"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("patterned"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("patterned/outlined"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP technologyclsp = {technologyclsopt, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("New style for this layer"), 0};
static COMCOMP technologyclip = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("CIF symbol for this layer"), 0};
static COMCOMP technologycldp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("DXF name(s) for this layer"), 0};
static COMCOMP technologyclgp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("GDS-II numbers for this layer"), 0};
static COMCOMP technologyclsrp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("SPICE resistance for this layer"), 0};
static COMCOMP technologyclscp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("SPICE capacitance for this layer"), 0};
static COMCOMP technologyclsecp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("SPICE edge capacitance for this layer"), 0};
static COMCOMP technologycldmw = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("DRC minimum width for this layer"), 0};
static COMCOMP technologycl3dh = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("3D height for this layer"), 0};
static COMCOMP technologycl3dt = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("3D thickness for this layer"), 0};
static COMCOMP technologyclpcrp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("red value"), x_("")};
static COMCOMP technologyclpcgp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("green value"), x_("")};
static COMCOMP technologyclpcbp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("blue value"), x_("")};
static COMCOMP technologyclpcop = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("opacity value"), x_("")};
static KEYWORD technologyclpcfopt[] =
{
	{x_("on"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("off"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP technologyclpcfp = {technologyclpcfopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("foreground value"), x_("")};
static KEYWORD technologyclpcopt[] =
{
	{x_("red"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("green"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("blue"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("opacity"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("foreground"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
INTBIG us_paramsedtecpcol(CHAR *word, COMCOMP *arr[], CHAR breakc)
{
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var;
	INTBIG r, g, b, o, f;
	HIGHLIGHT high;
	static CHAR technologycpbpdef[20];

	if (*word == 0) return(0);

	/* get the currently highlighted node */
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var == NOVARIABLE) return(0);
	if (getlength(var) != 1) return(0);
	(void)us_makehighlight(((CHAR **)var->addr)[0], &high);
	if (((high.status&HIGHTYPE) != HIGHFROM && (high.status&HIGHTYPE) != HIGHTEXT)) return(0);
	if (!high.fromgeom->entryisnode) return(0);
	ni = high.fromgeom->entryaddr.ni;
	var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, art_messagekey);
	us_teceditgetprintcol(var, &r, &g, &b, &o, &f);

	if (namesame(word, x_("red")) == 0)
	{
		arr[0] = &technologyclpcrp;
		technologyclpcrp.def = technologycpbpdef;
		(void)esnprintf(technologycpbpdef, 20, x_("%ld"), r);
		return(1);
	}

	if (namesame(word, x_("green")) == 0)
	{
		arr[0] = &technologyclpcgp;
		technologyclpcgp.def = technologycpbpdef;
		(void)esnprintf(technologycpbpdef, 20, x_("%ld"), g);
		return(1);
	}

	if (namesame(word, x_("blue")) == 0)
	{
		arr[0] = &technologyclpcbp;
		technologyclpcbp.def = technologycpbpdef;
		(void)esnprintf(technologycpbpdef, 20, x_("%ld"), b);
		return(1);
	}

	if (namesame(word, x_("opacity")) == 0)
	{
		arr[0] = &technologyclpcop;
		technologyclpcop.def = technologycpbpdef;
		(void)esnprintf(technologycpbpdef, 20, x_("%ld"), o);
		return(1);
	}

	if (namesame(word, x_("foreground")) == 0)
	{
		arr[0] = &technologyclpcfp;
		technologyclpcfp.def = technologycpbpdef;
		if (f == 0) estrcpy(technologycpbpdef, x_("off")); else
			estrcpy(technologycpbpdef, x_("on"));
		return(1);
	}
	return(0);
}
static COMCOMP technologyclpct = {technologyclpcopt, NOTOPLIST, NONEXTLIST, us_paramsedtecpcol,
	INPUTOPT, x_(" \t"), M_("Print colors for this layer"), 0};

static KEYWORD technologyclfopt[] =
{
	{x_("unknown"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("metal-1"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("metal-2"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("metal-3"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("metal-4"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("metal-5"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("metal-6"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("metal-7"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("metal-8"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("metal-9"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("metal-10"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("metal-11"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("metal-12"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("poly-1"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("poly-2"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("poly-3"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("gate"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("diffusion"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("implant"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("contact-1"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("contact-2"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("contact-3"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("contact-4"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("contact-5"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("contact-6"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("contact-7"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("contact-8"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("contact-9"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("contact-10"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("contact-11"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("contact-12"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("plug"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("overglass"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("resistor"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("capacitor"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("transistor"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("emitter"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("base"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("collector"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("substrate"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("well"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("guard"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("isolation"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("bus"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("art"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("control"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("p-type"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("n-type"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("depletion"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("enhancement"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("light"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("heavy"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("pseudo"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("nonelectrical"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("connects-metal"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("connects-poly"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("connects-diff"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("inside-transistor"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP technologyclfp = {technologyclfopt, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("New function for this layer"), 0};
static COMCOMP technologycllp = {NOKEYWORD, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("Letter to identify this layer"), 0};
static KEYWORD technologyclropt[] =
{
	{x_("Clear Pattern"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("Invert Pattern"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("Copy Pattern"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("Paste Pattern"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP technologyclrp = {technologyclropt, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("control of the layer pattern"), 0};
static COMCOMP technologyclasp = {NOKEYWORD, us_topofedtecslay, us_nextparse,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("new layer for this patch"), 0};
static COMCOMP technologyclacp = {NOKEYWORD, us_topofedtecclay, us_nextparse,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("new layer for this patch"), 0};
static COMCOMP technologycafp = {NOKEYWORD, us_topofarcfun, us_nextparse,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("New function for this arc"), 0};
static COMCOMP technologycamp = {yesnoopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Is this arc fixed-angle by default"), 0};
static COMCOMP technologycawp = {yesnoopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Does this arc erase connecting pins"), 0};
static COMCOMP technologycanp = {yesnoopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Does this arc extend beyond its endpoints"), 0};
static COMCOMP technologycaip = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Prefered angle increment for this arc"), 0};
static COMCOMP technologycnfp = {NOKEYWORD, us_topofnodefun, us_nextparse,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("New function for this node"), 0};
static COMCOMP technologycnsp = {yesnoopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Is this node a serpentine transistor"), 0};
static COMCOMP technologycnsqp = {yesnoopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Is this node square"), 0};
static COMCOMP technologycnwp = {yesnoopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Is this node invisible with 1 or 2 arcs"), 0};
static COMCOMP technologycnlp = {yesnoopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Can this node be locked"), 0};
static COMCOMP technologycnmp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Separation of multiple cuts for this node"), 0};
static COMCOMP technologyctlp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("New value of Lambda for this technology"), 0};
COMCOMP us_technologyctdp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("New description of this technology"), 0};
static COMCOMP technologycpynp = {yesnoopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("new value"), M_("yes")};
static COMCOMP technologycpbp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("new value"), x_("       ")};
INTBIG us_paramsedtecport(CHAR *word, COMCOMP *arr[], CHAR breakc)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER VARIABLE *var;
	REGISTER INTBIG len, j;
	HIGHLIGHT high;
	static CHAR technologycpynpdef[4];
	static CHAR technologycpbpdef[20];

	if (*word == 0) return(0);

	/* get the currently highlighted node */
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var == NOVARIABLE) return(0);
	if (getlength(var) != 1) return(0);
	(void)us_makehighlight(((CHAR **)var->addr)[0], &high);
	if (((high.status&HIGHTYPE) != HIGHFROM && (high.status&HIGHTYPE) != HIGHTEXT)) return(0);
	if (!high.fromgeom->entryisnode) return(0);
	ni = high.fromgeom->entryaddr.ni;

	if (namesame(word, x_("PORT-ANGLE")) == 0)
	{
		technologycpbp.def = technologycpbpdef;
		var = getval((INTBIG)ni, VNODEINST, VINTEGER, x_("EDTEC_portangle"));
		if (var != NOVARIABLE) (void)esnprintf(technologycpbp.def, 7, x_("%ld"), var->addr); else
			(void)estrcpy(technologycpbp.def, x_("0"));
		arr[0] = &technologycpbp;
		return(1);
	}

	if (namesame(word, x_("PORT-ANGLE-RANGE")) == 0)
	{
		technologycpbp.def = technologycpbpdef;
		var = getval((INTBIG)ni, VNODEINST, VINTEGER, x_("EDTEC_portrange"));
		if (var != NOVARIABLE) (void)esnprintf(technologycpbp.def, 7, x_("%ld"), var->addr); else
			(void)estrcpy(technologycpbp.def, x_("180"));
		arr[0] = &technologycpbp;
		return(1);
	}

	/* see if this is an arc that was named */
	for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		if (namesamen(np->protoname, x_("arc-"), 4) == 0 &&
			namesame(&np->protoname[4], word) == 0) break;
	if (np == NONODEPROTO) return(0);

	/* get information about this node */
	technologycpynp.def = technologycpynpdef;
	(void)estrcpy(technologycpynp.def, x_("no"));
	arr[0] = &technologycpynp;
	var = getval((INTBIG)ni, VNODEINST, VNODEPROTO|VISARRAY, x_("EDTEC_connects"));
	if (var != NOVARIABLE)
	{
		len = getlength(var);
		for(j=0; j<len; j++) if (((NODEPROTO **)var->addr)[j] == np)
		{
			(void)estrcpy(technologycpynp.def, x_("yes"));
			break;
		}
	}
	return(1);
}
static COMCOMP technologycpp = {NOKEYWORD, us_topofedtecport, us_nextparse,
	us_paramsedtecport, INPUTOPT|MULTIOPT, x_(" \t"), M_("options for this port"), 0};
COMCOMP us_technologyeenp = {NOKEYWORD, us_topofedtecnode, us_nextparse,
	NOPARAMS, INPUTOPT|INCLUDENOISE, x_(" \t"), M_("node in technology to edit"), 0};
COMCOMP us_technologyeeap = {NOKEYWORD, us_topofedtecarc, us_nextparse,
	NOPARAMS, INPUTOPT|INCLUDENOISE, x_(" \t"), M_("arc in technology to edit"), 0};
COMCOMP us_technologyeelp = {NOKEYWORD, us_topofedteclay, us_nextparse,
	NOPARAMS, INPUTOPT|INCLUDENOISE, x_(" \t"), M_("layer in technology to edit"), 0};
COMCOMP us_technologyedlp = {NOKEYWORD,topoflibs,nextlibs,us_nexttechnologyedlp,
	0, x_(" \t"), M_("argument to be echoed"), 0};
INTBIG us_nexttechnologyedlp(CHAR *i, COMCOMP *j[], CHAR c)
{ j[0] = &us_technologyedlp; return(1); }
static KEYWORD technologyeopt[] =
{
	{x_("library-to-tech-and-C"),    1,{&us_technologycnnp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("library-to-tech-and-Java"), 1,{&us_technologycnnp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("library-to-tech"),          1,{&us_technologycnnp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("tech-to-library"),          1,{&technologyetlp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("inquire-layer"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("place-layer"),              1,{&technologyepp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("change"),                   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("compact-current-cell"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("edit-node"),                1,{&us_technologyeenp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("edit-arc"),                 1,{&us_technologyeeap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("edit-layer"),               1,{&us_technologyeelp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("edit-subsequent"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("edit-colors"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("edit-design-rules"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("edit-misc-information"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("identify-layers"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("identify-ports"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("dependent-libraries"),      1,{&us_technologyedlp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("reorder-arcs"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("reorder-nodes"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("reorder-layers"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
INTBIG us_paramtechnology(CHAR *word, COMCOMP *arr[], CHAR breakc)
{
	REGISTER NODEINST *ni;
	REGISTER INTBIG ind, i, count;
	HIGHLIGHT high;
	REGISTER VARIABLE *var;

	if (*word == 0) return(0);
	if (namesame(word, x_("change")) != 0)
	{
		/* handle normal parameter addition if not the "change" option */
		for(ind=0; technologyeopt[ind].name != 0; ind++)
			if (namesame(word, technologyeopt[ind].name) == 0) break;
		if (technologyeopt[ind].name == 0) return(0);
		count = technologyeopt[ind].params;
		for(i=0; i<count; i++) arr[i] = technologyeopt[ind].par[i];
		return(count);
	}

	/* "change": get the currently highlighted node */
	if ((el_curwindowpart->state&WINDOWOUTLINEEDMODE) != 0) return(0);
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var == NOVARIABLE) return(0);
	if (getlength(var) != 1) return(0);
	(void)us_makehighlight(((CHAR **)var->addr)[0], &high);
	if (((high.status&HIGHTYPE) != HIGHFROM && (high.status&HIGHTYPE) != HIGHTEXT)) return(0);
	if (!high.fromgeom->entryisnode) return(0);
	ni = high.fromgeom->entryaddr.ni;
	switch (us_tecedgetoption(ni))
	{
		case ARCFIXANG:      arr[0] = &technologycamp;      return(1);
		case ARCFUNCTION:    arr[0] = &technologycafp;      return(1);
		case ARCINC:         arr[0] = &technologycaip;      return(1);
		case ARCNOEXTEND:    arr[0] = &technologycanp;      return(1);
		case ARCWIPESPINS:   arr[0] = &technologycawp;      return(1);
		case LAYER3DHEIGHT:  arr[0] = &technologycl3dh;     return(1);
		case LAYER3DTHICK:   arr[0] = &technologycl3dt;     return(1);
		case LAYERPRINTCOL:  arr[0] = &technologyclpct;     return(1);
		case LAYERCIF:       arr[0] = &technologyclip;      return(1);
		case LAYERCOLOR:     arr[0] = &technologyclcp;      return(1);
		case LAYERDXF:       arr[0] = &technologycldp;      return(1);
		case LAYERDRCMINWID: arr[0] = &technologycldmw;     return(1);
		case LAYERFUNCTION:  arr[0] = &technologyclfp;      return(1);
		case LAYERGDS:       arr[0] = &technologyclgp;      return(1);
		case LAYERLETTERS:   arr[0] = &technologycllp;      return(1);
		case LAYERPATCONT:   arr[0] = &technologyclrp;      return(1);
		case LAYERPATCH:
			var = getval((INTBIG)ni, VNODEINST, VSTRING, x_("EDTEC_minbox"));
			if (var == NOVARIABLE) arr[0] = &technologyclasp; else
				arr[0] = &technologyclacp;
			return(1);
		case LAYERSPICAP:    arr[0] = &technologyclscp;     return(1);
		case LAYERSPIECAP:   arr[0] = &technologyclsecp;    return(1);
		case LAYERSPIRES:    arr[0] = &technologyclsrp;     return(1);
		case LAYERSTYLE:     arr[0] = &technologyclsp;      return(1);
		case NODEFUNCTION:   arr[0] = &technologycnfp;      return(1);
		case NODELOCKABLE:   arr[0] = &technologycnlp;      return(1);
		case NODEMULTICUT:   arr[0] = &technologycnmp;      return(1);
		case NODESERPENTINE: arr[0] = &technologycnsp;      return(1);
		case NODESQUARE:     arr[0] = &technologycnsqp;     return(1);
		case NODEWIPES:      arr[0] = &technologycnwp;      return(1);
		case PORTOBJ:        arr[0] = &technologycpp;       return(1);
		case TECHDESCRIPT:   arr[0] = &us_technologyctdp;   return(1);
		case TECHLAMBDA:     arr[0] = &technologyctlp;      return(1);
	}
	return(0);
}
static COMCOMP technologyep = {technologyeopt, NOTOPLIST, NONEXTLIST,
	us_paramtechnology, INPUTOPT, x_(" \t"), M_("technology editing option"), 0};
COMCOMP us_technologyup = {NOKEYWORD, topoftechs, nexttechs, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("new technology to use"), 0};
static COMCOMP technologykp = {NOKEYWORD, topoftechs, nexttechs, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("technology to kill"), 0};
static COMCOMP technologycp = {NOKEYWORD, topoftechs, nexttechs, NOPARAMS,
	0, x_(" \t"), M_("technology to which conversion is desired"), 0};
static COMCOMP technologyap = {NOKEYWORD, NOTOPLIST, NONEXTLIST,
	us_paramtechnologyb, 0, x_(" \t"), M_("message for technology"), 0};
INTBIG us_paramtechnologyb(CHAR *i, COMCOMP *j[], CHAR c)
{ j[0]= &technologyap; return(1); }
INTBIG us_paramtechnologya(CHAR *pt, COMCOMP *j[], CHAR c)
{
	REGISTER TECHNOLOGY *t;

	t = gettechnology(pt);
	if (t != NOTECHNOLOGY)
	{
		j[0] = t->parse;
		if (j[0] != NOCOMCOMP) return(1);
	}
	return(us_paramtechnologyb(pt, j, c));
}
COMCOMP us_technologytp = {NOKEYWORD, topoftechs, nexttechs,
	us_paramtechnologya, INPUTOPT, x_(" \t"), M_("technology to direct"), 0};
static COMCOMP technologydp = {NOKEYWORD, topoftechs, nexttechs,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("technology to document"), 0};
static KEYWORD technologyasopt[] =
{
	{x_("on"),                  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("off"),                 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP technologyasp = {technologyasopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("technology auto-switching option"), 0};
static KEYWORD technologyopt[] =
{
	{x_("use"),        1,{&us_technologyup,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("convert"),    1,{&technologycp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("tell"),       1,{&us_technologytp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("edit"),       1,{&technologyep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("kill"),       1,{&technologykp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("document"),   1,{&technologydp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("autoswitch"), 1,{&technologyasp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_technologyp = {technologyopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("technology option"), 0};

/* the "telltool" command */
static COMCOMP telltoolap = {NOKEYWORD,NOTOPLIST,NONEXTLIST,us_paramtelltoolb,
	0, x_(" \t"), M_("message to be sent to tool"), 0};
INTBIG us_paramtelltoolb(CHAR *i, COMCOMP *j[], CHAR c)
{ j[0]= &telltoolap; return(1); }
INTBIG us_paramtelltoola(CHAR *pt, COMCOMP *j[], CHAR c)
{
	REGISTER INTBIG i;

	i = parse(pt, &offtoolp, FALSE);
	if (i >= 0)
	{
		j[0] = el_tools[i].parse;
		if (j[0] != NOCOMCOMP) return(1);
	}
	return(us_paramtelltoolb(pt, j, c));
}
COMCOMP us_telltoolp = {NOKEYWORD, topoftools, nexttools, us_paramtelltoola,
	INPUTOPT, x_(" \t"), M_("tool to instruct"), 0};
COMCOMP us_userp = {NOKEYWORD, us_topofcommands, us_nextparse, us_paramcommands,
	0, x_(" \t"), M_("Full Electric command"), 0};

/* the "terminal" command */
static COMCOMP terminalvp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("variable letter to fill"), 0};
static COMCOMP terminalpp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("prompt message"), 0};
static COMCOMP terminaltp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("type of input ('cell', etc.)"), 0};
static COMCOMP terminalsfp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("number of keystrokes between recording checkpoints"), 0};
static COMCOMP terminalspp = {NOKEYWORD, topoffile, nextfile, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("session playback file"), 0};
static KEYWORD terminalspaopt[] =
{
	{x_("all"),                  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP terminalspap = {terminalspaopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("session playback option"), 0};
static KEYWORD terminalsopt[] =
{
	{x_("playback"),             2,{&terminalspp,&terminalspap,NOKEY,NOKEY,NOKEY}},
	{x_("begin-record"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("end-record"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("rewind-record"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("checkpoint-frequency"), 1,{&terminalsfp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP terminalsp = {terminalsopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("session logging option"), 0};
static KEYWORD terminalnopt[] =
{
	{x_("lock-keys-on-error"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("only-informative-messages"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("use-electric-commands"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("display-dialogs"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("permanent-menu-highlighting"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("track-cursor-coordinates"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("audit"),                       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("enable-interrupts"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("beep"),                        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP terminalnp = {terminalnopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("terminal option to turn off"), 0};
static KEYWORD terminalopt[] =
{
	{x_("input"),                       3,{&terminalvp,&terminalpp,&terminaltp,NOKEY,NOKEY}},
	{x_("session"),                     1,{&terminalsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("not"),                         1,{&terminalnp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("lock-keys-on-error"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("only-informative-messages"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("use-electric-commands"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("display-dialogs"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("permanent-menu-highlighting"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("track-cursor-coordinates"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("get-location"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("audit"),                       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("enable-interrupts"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("beep"),                        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clear"),                       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP terminalp = {terminalopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("terminal control option"), 0};

/* the "text" command */
static KEYWORD textsopt[] =
{
	{x_("4p"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("6p"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("8p"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("10p"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("12p"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("14p"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("16p"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("18p"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("20p"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("hl"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("1l"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("2l"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("3l"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("4l"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("5l"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("6l"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("up"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("down"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_textsp = {textsopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("New size for highlighted text"), 0};
COMCOMP us_textdsp = {textsopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Size for new text"), 0};
KEYWORD us_texttopt[] =
{
	{x_("centered"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("boxed"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("up"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("down"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("left"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("right"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("up-left"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("up-right"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("down-left"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("down-right"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static KEYWORD textssopt[] =
{
	{x_("none"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("inside"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("outside"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP texttp = {us_texttopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("New style for highlighted text"), 0};
static COMCOMP textdtp = {us_texttopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Style for new text"), 0};
static COMCOMP textdhp = {textssopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Horizontal style for new text"), 0};
static COMCOMP textdvp = {textssopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Vertical style for new text"), 0};
static COMCOMP textep = {NOKEYWORD, us_topofeditor, us_nexteditor, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Editor to use for text"), 0};
static COMCOMP textrp = {NOKEYWORD, topoffile, nextfile, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("File to read into this cell"), 0};
static COMCOMP textwp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("File to write with this cell"), 0};
COMCOMP us_textfp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("String to find in text window"), 0};
static KEYWORD textopt[] =
{
	{x_("style"),                    1,{&texttp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("size"),                     1,{&us_textsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("default-style"),            1,{&textdtp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("default-horizontal-style"), 1,{&textdhp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("default-vertical-style"),   1,{&textdvp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("default-node-size"),        1,{&us_textdsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("default-arc-size"),         1,{&us_textdsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("default-export-size"),      1,{&us_textdsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("default-nonlayout-text-size"), 1,{&us_textdsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("default-instance-size"),    1,{&us_textdsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("default-cell-size"),        1,{&us_textdsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("default-interior-only"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("default-exterior"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("editor"),                   1,{&textep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("write"),                    1,{&textwp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("read"),                     1,{&textrp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("cut"),                      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("copy"),                     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("paste"),                    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("find"),                     1,{&us_textfp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("easy-text-selection"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("hard-text-selection"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("interior-only"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("exterior"),                 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP textp = {textopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("text manipulation option"), 0};

/* the "undo" command */
static COMCOMP undorp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("number of changes to redo"), M_("redo 1 major change")};
static COMCOMP undosp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("number of changes to save"), 0};
static KEYWORD undoopt[] =
{
	{x_("redo"),          1,{&undorp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("save"),          1,{&undosp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clear"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP undop = {undoopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("number of changes to undo, or undo control option"),
		M_("undo 1 major change")};

/* the "var" command */
static COMCOMP varmoddxp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("X offset of variable"), 0};
static COMCOMP varmoddyp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("Y offset of variable"), 0};
static COMCOMP varmoddsp = {us_texttopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Text style of variable"), 0};
static KEYWORD varmodopt[] =
{
	{x_("lisp"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("tcl"),                 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("java"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("display"),             3,{&varmoddxp,&varmoddyp,&varmoddsp,NOKEY,NOKEY}},
	{x_("na-va-display"),       3,{&varmoddxp,&varmoddyp,&varmoddsp,NOKEY,NOKEY}},
	{x_("in-na-va-display"),    3,{&varmoddxp,&varmoddyp,&varmoddsp,NOKEY,NOKEY}},
	{x_("inall-na-va-display"), 3,{&varmoddxp,&varmoddyp,&varmoddsp,NOKEY,NOKEY}},
	{x_("temporary"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("fractional"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("float"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("cannot-change"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP varmodp = {varmodopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("modifier to the variable"), 0};
static KEYWORD varhcnopt[] =
{
	{x_("language"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("display"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("temporary"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("cannot-change"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("interior-only"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP varhcnp = {varhcnopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("how to unchange the variable"), 0};
static KEYWORD varhcopt[] =
{
	{x_("not"),                 1,{&varhcnp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("lisp"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("tcl"),                 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("java"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("display"),             3,{&varmoddxp,&varmoddyp,&varmoddsp,NOKEY,NOKEY}},
	{x_("na-va-display"),       3,{&varmoddxp,&varmoddyp,&varmoddsp,NOKEY,NOKEY}},
	{x_("in-na-va-display"),    3,{&varmoddxp,&varmoddyp,&varmoddsp,NOKEY,NOKEY}},
	{x_("inall-na-va-display"), 3,{&varmoddxp,&varmoddyp,&varmoddsp,NOKEY,NOKEY}},
	{x_("temporary"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("cannot-change"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("interior-only"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP varhcp = {varhcopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("how to change the variable"), 0};
COMCOMP us_varvalp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("new value of variable"), 0};
COMCOMP us_varvalcp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("this variable cannot be set!"), 0};
static COMCOMP varupdp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("modification to variable"), 0};
COMCOMP us_varvsp = {NOKEYWORD, us_topofvars, us_nextvars, NOPARAMS,
	NOFILL|INPUTOPT|INCLUDENOISE, x_(" \t"), M_("variable name to set"), 0};
COMCOMP us_varvep = {NOKEYWORD, us_topofvars, us_nextvars, NOPARAMS,
	NOFILL|INPUTOPT, x_(" \t"), M_("variable name to examine"), 0};
static COMCOMP varvcp = {NOKEYWORD, us_topofvars, us_nextvars, NOPARAMS,
	NOFILL|INPUTOPT, x_(" \t"), M_("variable name to change"), 0};
static COMCOMP varvpp = {NOKEYWORD, us_topofvars, us_nextvars, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("variable to pick in menu"), 0};
static COMCOMP varvtp = {NOKEYWORD, us_topofvars, us_nextvars, NOPARAMS,
	INPUTOPT|INCLUDENOISE, x_(" \t"), M_("variable to edit in window"), 0};
static COMCOMP varqhp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("header string for edit window"), 0};
static KEYWORD varqopt[] =
{
	{x_("in-place"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("header"),              1,{&varqhp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_varqp = {varqopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("text editing option"), 0};
COMCOMP us_varvdp = {NOKEYWORD, us_topofvars, us_nextvars, NOPARAMS,
	NOFILL, x_(" \t"), M_("variable name to delete"), 0};
COMCOMP us_varvmp = {NOKEYWORD,us_topofvars,us_nextvars,NOPARAMS,
	NOFILL|INPUTOPT, x_(" \t"), M_("variable name to modify"), 0};
COMCOMP us_varbdp = {NOKEYWORD, us_topofvars, us_nextvars, NOPARAMS,
	NOFILL|INPUTOPT|INCLUDENOISE, x_(" \t"), M_("variable name to set"), 0};
COMCOMP us_varbs1p = {NOKEYWORD, us_topofvars, us_nextvars, NOPARAMS,
	NOFILL|INPUTOPT|INCLUDENOISE, x_(" \t"), M_("first operand variable name"), 0};
static COMCOMP varbs2p = {NOKEYWORD, us_topofvars, us_nextvars, NOPARAMS,
	NOFILL|INPUTOPT|INCLUDENOISE, x_(" \t"), M_("second operand variable name"), 0};
static COMCOMP varbs2tp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	NOFILL|INPUTOPT, x_(" \t"), M_("pattern matching string"), 0};
static KEYWORD varbs2tyopt[] =
{
	{x_("unknown"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("integer"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("short"),                 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("fixed-point"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("address"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("character"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("string"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("float"),                 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("double"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("nodeinst"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("nodeproto"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("cell"),                  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("portarcinst"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("portexpinst"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("portproto"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("arcinst"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("arcproto"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("geometry"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("r-tree"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("library"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("technology"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("tool"),                  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("network"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("view"),                  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("window"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("graphics"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("constraint"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("general"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_varbs2typ = {varbs2tyopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	NOFILL|INPUTOPT, x_(" \t"), M_("type to request"), 0};
static KEYWORD varbopt[] =
{
	{x_("set"),                 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("concat"),              1,{&varbs2p,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("type"),                1,{&us_varbs2typ,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("select"),              1,{&varbs2p,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("pattern"),             1,{&varbs2tp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP varbop = {varbopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("operator to perform on variables"), 0};
static KEYWORD varoopt[] =
{
	{x_("ignore"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("track"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("save"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_varop = {varoopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("options control"), 0};
static KEYWORD varopt[] =
{
	{x_("options"),           1,{&us_varop,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("set"),               3,{&us_varvsp,&us_varvalp,&varmodp,NOKEY,NOKEY}},
	{x_("examine"),           1,{&us_varvep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("change"),            2,{&varvcp,&varhcp,NOKEY,NOKEY,NOKEY}},
	{x_("delete"),            1,{&us_varvdp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("pick"),              1,{&varvpp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("textedit"),          2,{&varvtp,&us_varqp,NOKEY,NOKEY,NOKEY}},
	{x_("vector"),            3,{&us_varbdp,&us_varbs1p,&varbop,NOKEY,NOKEY}},
	{x_("+"),                 3,{&us_varvmp,&varupdp,&varmodp,NOKEY,NOKEY}},
	{x_("-"),                 3,{&us_varvmp,&varupdp,&varmodp,NOKEY,NOKEY}},
	{x_("*"),                 3,{&us_varvmp,&varupdp,&varmodp,NOKEY,NOKEY}},
	{x_("/"),                 3,{&us_varvmp,&varupdp,&varmodp,NOKEY,NOKEY}},
	{x_("mod"),               3,{&us_varvmp,&varupdp,&varmodp,NOKEY,NOKEY}},
	{x_("and"),               3,{&us_varvmp,&varupdp,&varmodp,NOKEY,NOKEY}},
	{x_("or"),                3,{&us_varvmp,&varupdp,&varmodp,NOKEY,NOKEY}},
	{x_("|"),                 3,{&us_varvmp,&varupdp,&varmodp,NOKEY,NOKEY}},
	{x_("reinherit"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("total-reinherit"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("relocate"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("total-relocate"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("visible-all"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("visible-none"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("visible-default"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_varp = {varopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("option for manipulating variables"), 0};

/* the "view" command */
COMCOMP us_viewc1p = {NOKEYWORD, topofcells, nextcells, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("cell name whose view is to be changed"), 0};
COMCOMP us_viewc2p = {NOKEYWORD, topofviews, nextviews, NOPARAMS,
	0, x_(" \t"), M_("new view for the cell"), 0};
COMCOMP us_viewdp = {NOKEYWORD, topofviews, nextviews, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("view name to delete"), 0};
extern COMCOMP viewvhp;
static KEYWORD viewvhopt[] =
{
	{x_("vertical"),        1,{&viewvhp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no-title"),        1,{&viewvhp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP viewvhp = {viewvhopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("option to rotate frame"), 0};
static KEYWORD viewfopt[] =
{
	{x_("A"),                 1,{&viewvhp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("B"),                 1,{&viewvhp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("C"),                 1,{&viewvhp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("D"),                 1,{&viewvhp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("E"),                 1,{&viewvhp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("Half-A"),            1,{&viewvhp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("none"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("title-only"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_viewfp = {viewfopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("frame size to use for this cell"), 0};
COMCOMP us_viewn1p = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("new view name"), 0};
static COMCOMP viewn2p = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("abbreviation for new view name"), 0};
static KEYWORD viewtopt[] =
{
	{x_("text"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP viewn3p = {viewtopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("type of view (graphical)"), 0};
static KEYWORD viewopt[] =
{
	{x_("new"),               3,{&us_viewn1p,&viewn2p,&viewn3p,NOKEY,NOKEY}},
	{x_("delete"),            1,{&us_viewdp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("change"),            2,{&us_viewc1p,&us_viewc2p,NOKEY,NOKEY,NOKEY}},
	{x_("frame"),             1,{&us_viewfp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_viewp = {viewopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("option for manipulating cell views"), 0};

/* the "visiblelayers" command */
static KEYWORD visiblelayersnoptp[] =
{
	{x_("no-list"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP visiblelayersnp = {visiblelayersnoptp, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("option to supress layer listing"), 0};
INTBIG us_paramvisiblelayers(CHAR *i, COMCOMP *j[], CHAR c)
{ j[0] = &visiblelayersnp; return(1);}
COMCOMP us_visiblelayersp = {NOKEYWORD, us_topoflayers, us_nextparse,
	us_paramvisiblelayers, NOFILL|INPUTOPT, x_(" \t"),
		M_("layers to be made visible (* for all)"), M_("show layers")};

/* the "window" command */
static KEYWORD windowspopt[] =
{
	{x_("horizontal"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("vertical"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP windowspp = {windowspopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("way to split the window"), 0};
COMCOMP us_windowmp = {NOKEYWORD, us_topofwindows, us_nextparse, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("other window whose scale to match"), 0};
COMCOMP us_windowup = {NOKEYWORD, us_topofwindows, us_nextparse, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("window to use"), 0};
static COMCOMP windowap = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("amount to move window"), 0};
static COMCOMP windowsp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("amount to scale"), 0};
static KEYWORD windowzopt[] =
{
	{x_("integral"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("nonintegral"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP windowzp = {windowzopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("option to force window scale to align with pixels"),
		M_("show current state")};
static KEYWORD windownopt[] =
{
	{x_("standard"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("pen"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("tee"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP windownp = {windownopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("default cursor to use in window"), 0};
static KEYWORD windowtopt[] =
{
	{x_("draw"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("hash-out"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP windowtp = {windowtopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("how to draw tiny cells in window"), 0};
static COMCOMP windowdp = {onoffopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("mode in which cursor-based operations drag objects"),
		M_("show current state")};
COMCOMP us_windowrnamep = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("view name"), 0};
static COMCOMP windowsnamep = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	NOFILL|INPUTOPT|INCLUDENOISE, x_(" \t"), M_("view name"), 0};
static KEYWORD windowoopt[] =
{
	{x_("on"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("off"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP windowop = {windowoopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("whether to allow overlappable layers"), 0};
static KEYWORD windowsbaopt[] =
{
	{x_("align"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("angle"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("arc"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("cell"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("grid"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("lambda"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("network"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("node"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("package"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("part"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("project"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("root"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("selection"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("size"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("technology"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("x"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("y"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP windowsbap = {windowsbaopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("area in status bar"), 0};
static COMCOMP windowsblp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("line number (1-based)"), 0};
static COMCOMP windowsbsp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("starting percentage of position on line"), 0};
static COMCOMP windowsbep = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("ending percentage of position on line"), 0};
static COMCOMP windowsbtp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("title of status field"), 0};
static KEYWORD windowsbcopt[] =
{
	{x_("persistent"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("temporary"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP windowsbcp = {windowsbcopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("persistence of current node in status bar"), 0};
static KEYWORD windowsbopt[] =
{
	{x_("add"),          5,{&windowsbap,&windowsblp,&windowsbsp,&windowsbep,&windowsbtp}},
	{x_("delete"),       1,{&windowsbap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("current-node"), 1,{&windowsbcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP windowsbp = {windowsbopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("commands for status bar"), 0};
static KEYWORD window3dopt[] =
{
	{x_("begin"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("end"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("rotate"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("zoom"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("pan"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("twist"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_window3dp = {window3dopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("commands for 3D display"), 0};
static KEYWORD windowadopt[] =
{
	{x_("horizontal-tile"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("vertical-tile"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("cascade"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP windowadp = {windowadopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("commands for adjusting windows"), 0};
static KEYWORD windowopt[] =
{
	{x_("1-window"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("3-dimensional"),       1,{&us_window3dp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("all-displayed"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("adjust"),              1,{&windowadp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("center-highlight"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("cursor-centered"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("delete"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("down"),                1,{&windowap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("dragging"),            1,{&windowdp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("explore"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("grid-zoom"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("hide-attributes"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("highlight-displayed"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("in-zoom"),             1,{&windowsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("join"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("kill"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("left"),                1,{&windowap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("match"),               1,{&us_windowmp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("measure"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("move-display"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("name"),                1,{&windowsnamep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("new"),                 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("normal-cursor"),       2,{&windownp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("out-zoom"),            1,{&windowsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("outline-edit-toggle"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("overlappable-display"),1,{&windowop,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("overview"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("peek"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("right"),               1,{&windowap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("save"),                1,{&us_windowrnamep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("show-attributes"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("split"),               1,{&windowspp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("status-bar"),          1,{&windowsbp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("tiny-cells"),          1,{&windowtp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("trace-displayed"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("use"),                 1,{&us_windowup,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("up"),                  1,{&windowap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("zoom-scale"),          1,{&windowzp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP us_windowp = {windowopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("window display control option"), 0};

/* table of commands with routines */
struct commandinfo us_lcommand[] =
{
	{x_("arc"),          us_arc,           1,{&us_arcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("array"),        us_array,         5,{&us_arrayxp,&arrayyp,&arrayxop,&arrayyop, &arrayswitchp}},
	{x_("bind"),         us_bind,          1,{&us_bindp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("color"),        us_color,         1,{&us_colorp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("commandfile"),  us_commandfile,   2,{&commandfilep,&commandfileswitchp,NOKEY,NOKEY,NOKEY}},
	{x_("constraint"),   us_constraint,    1,{&constraintp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("copycell"),     us_copycell,      3,{&us_copycellp,&us_copycelldp,&copycellqp,NOKEY,NOKEY}},
	{x_("create"),       us_create,        1,{&createp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("debug"),        us_debug,         1,{&debugp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("defarc"),       us_defarc,        2,{&us_defarcsp,&defarcp,NOKEY,NOKEY,NOKEY}},
	{x_("defnode"),      us_defnode,       2,{&us_defnodesp,&us_defnodep,NOKEY,NOKEY,NOKEY}},
	{x_("duplicate"),    us_duplicate,     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("echo"),         us_echo,          1,{&echop,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("editcell"),     us_editcell,      2,{&us_editcellp,&editcellwindowp,NOKEY,NOKEY,NOKEY}},
	{x_("erase"),        us_erase,         1,{&erasep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("find"),         us_find,          1,{&findp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("getproto"),     us_getproto,      2,{&us_getproto1p,&getproto2p,NOKEY,NOKEY,NOKEY}},
	{x_("grid"),         us_grid,          1,{&us_gridp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("help"),         us_help,          1,{&us_helpp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("if"),           us_if,            4,{&iftp,&ifrp,&iftp,&us_userp,NOKEY}},
	{x_("interpret"),    us_interpret,     1,{&interpretp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("iterate"),      us_iterate,       2,{&iteratep,&iteratemp,NOKEY,NOKEY,NOKEY}},
	{x_("killcell"),     us_killcell,      1,{&killcellp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("lambda"),       us_lambda,        1,{&lambdap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("library"),      us_library,       1,{&us_libraryp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("macbegin"),     us_macbegin,      2,{&us_macbeginnp,&macbeginop,NOKEY,NOKEY,NOKEY}},
	{x_("macdone"),      us_macdone,       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("macend"),       us_macend,        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("menu"),         us_menu,          1,{&us_menup,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("mirror"),       us_mirror,        1,{&us_mirrorp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("move"),         us_move,          1,{&movep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("node"),         us_node,          1,{&us_nodep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("offtool"),      us_offtool,       2,{&offtoolp,&offtoolep,NOKEY,NOKEY,NOKEY}},
	{x_("ontool"),       us_ontool,        2,{&ontoolp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("outhier"),      us_outhier,       1,{&outhierp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("package"),      us_package,       1,{&us_packagep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("port"),         us_port,          1,{&us_portp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("quit"),         us_quit,          1,{&quitp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("redraw"),       us_redraw,        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("remember"),     us_remember,      1,{&us_userp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("rename"),       us_rename,        3,{&us_renameop,&us_renamenp,&us_renamecp,NOKEY,NOKEY}},
	{x_("replace"),      us_replace,       2,{&us_replacep,&replacesp,NOKEY,NOKEY,NOKEY}},
	{x_("rotate"),       us_rotate,        2,{&rotateap,&rotatep,NOKEY,NOKEY,NOKEY}},
	{x_("show"),         us_show,          1,{&us_showp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("size"),         us_size,          1,{&us_sizep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("spread"),       us_spread,        2,{&us_spreaddp,&spreadap,NOKEY,NOKEY,NOKEY}},
	{x_("system"),       us_system,        1,{&systemp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("technology"),   us_technology,    1,{&us_technologyp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("telltool"),      us_telltool,       1,{&us_telltoolp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("terminal"),     us_terminal,      1,{&terminalp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("text"),         us_text,          1,{&textp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("undo"),         us_undo,          1,{&undop,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("var"),          us_var,           1,{&us_varp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("view"),         us_view,          1,{&us_viewp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("visiblelayers"),us_visiblelayers, 1,{&us_visiblelayersp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("window"),       us_window,        1,{&us_windowp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("yanknode"),     us_yanknode,      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{NULL, NULL, 0, {NULL, NULL, NULL, NULL, NULL}} /* 0 */
};
