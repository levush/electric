/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dbcontrol.c
 * Database system controller
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
#include "tech.h"
#include "edialogs.h"
#include "usr.h"

#ifdef INTERNATIONAL
#  ifdef MACOS
#    define PACKAGE x_("macelectric")
#    define LOCALEDIR x_("lib:international")
#  else
#    define PACKAGE x_("electric")
#    define LOCALEDIR x_("lib/international")
#  endif
#endif

/*
 * The current Electric version.
 * Version numbering consists of 3 parts: the major number, the minor number,
 * and the detail.  The detail part is optional lettering at the end which
 * runs from "a" to "z", then "aa" to "az", then "ba" to "bz", etc.
 * A released version has no detail, and is considered to be a HIGHER version
 * than those with detail (which are prereleases).  For example:
 *    version "6.04a"  has major=6, minor=4, detail=1 ("a")
 *    version "6.05bc" has major=6, minor=5, detail=55 ("bc")
 *    version "6.03"   has major=6, minor=3, and detail=1000
 * The routine "io_getversion()" parses these version strings.
 */
CHAR *el_version = x_("7.00");

       BOOLEAN db_multiprocessing;	/* true if multiprocessing */

/* prototypes for local routines */
static void db_freeallmemory(void);

/*
 * the primary initialization of Electric
 */
void osprimaryosinit(void)
{
	REGISTER INTBIG i;
	REGISTER TECHNOLOGY *tech, *lasttech, *realtech;
	REGISTER CONSTRAINT *constr;
	REGISTER CLUSTER *clus;
	REGISTER LIBRARY *lib;
	CHAR libname[10], libfile[10];
	extern TECHNOLOGY el_technologylist[];
	REGISTER void *infstr;

#ifdef INTERNATIONAL
#  if HAVE_SETLOCALE
	setlocale(LC_ALL, x_(""));
#  endif
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	/* a test */
	estrcpy(libname, gettext(x_("Redo")));
#endif

	/* initialize the memory allocation system */
	db_multiprocessing = FALSE;
	db_initclusters();

	/* internal initialization of the constraint solvers */
	el_curconstraint = &el_constraints[0];
	for(i=0; el_constraints[i].conname != 0; i++)
	{
		constr = &el_constraints[i];
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("constraint:"));
		addstringtoinfstr(infstr, constr->conname);
		constr->cluster = alloccluster(returninfstr(infstr));
		if (namesame(constr->conname, DEFCONSTR) == 0) el_curconstraint = constr;
	}

	/* internal initialization of the tools */
	for(el_maxtools=0; el_tools[el_maxtools].toolname != 0; el_maxtools++)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("tool:"));
		addstringtoinfstr(infstr, el_tools[el_maxtools].toolname);
		el_tools[el_maxtools].cluster = alloccluster(returninfstr(infstr));
		el_tools[el_maxtools].toolindex = el_maxtools;
	}

	/* initialize the database */
	db_initdatabase();
	db_initgeometry();
	db_initlanguages();
	db_inittechnologies();

	/* internal initialization of the technologies */
	lasttech = NOTECHNOLOGY;
	for(el_maxtech=0; el_technologylist[el_maxtech].techname != 0; el_maxtech++)
	{
		tech = &el_technologylist[el_maxtech];

		/* create the real technology object */
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("tech:"));
		addstringtoinfstr(infstr, tech->techname);
		clus = alloccluster(returninfstr(infstr));
		realtech = alloctechnology(clus);
		if (realtech == NOTECHNOLOGY) error(_("No memory for technologies"));

		/* link it in */
		if (lasttech == NOTECHNOLOGY) el_technologies = realtech; else
			lasttech->nexttechnology = realtech;
		lasttech = realtech;

		/* initialize the real technology object */
		(void)allocstring(&realtech->techname, tech->techname, clus);
		realtech->techindex = el_maxtech;
		realtech->deflambda = tech->deflambda;
		realtech->parse = tech->parse;
		realtech->cluster = clus;
		(void)allocstring(&realtech->techdescript, TRANSLATE(tech->techdescript), clus);
		realtech->layercount = tech->layercount;
		realtech->layers = tech->layers;
		realtech->arcprotocount = tech->arcprotocount;
		realtech->arcprotos = tech->arcprotos;
		realtech->nodeprotocount = tech->nodeprotocount;
		realtech->nodeprotos = tech->nodeprotos;
		realtech->variables = tech->variables;
		realtech->init = tech->init;
		realtech->term = tech->term;
		realtech->setmode = tech->setmode;
		realtech->request = tech->request;
		realtech->nodepolys = tech->nodepolys;
		realtech->nodeEpolys = tech->nodeEpolys;
		realtech->shapenodepoly = tech->shapenodepoly;
		realtech->shapeEnodepoly = tech->shapeEnodepoly;
		realtech->allnodepolys = tech->allnodepolys;
		realtech->allnodeEpolys = tech->allnodeEpolys;
		realtech->nodesizeoffset = tech->nodesizeoffset;
		realtech->shapeportpoly = tech->shapeportpoly;
		realtech->arcpolys = tech->arcpolys;
		realtech->shapearcpoly = tech->shapearcpoly;
		realtech->allarcpolys = tech->allarcpolys;
		realtech->arcwidthoffset = tech->arcwidthoffset;
		realtech->userbits = tech->userbits;
	}

	/* select a current technology */
	el_curtech = el_technologies;
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		if (namesame(tech->techname, DEFTECH) == 0) el_curtech = tech;
	el_curlayouttech = el_curtech;

	/* setup a change batch for initialization work */
	/* db_preparenewbatch(&el_tools[0]); */

	/* pass 1 initialization of the constraint solvers */
	for(i=0; el_constraints[i].conname != 0; i++)
		(*el_constraints[i].init)(&el_constraints[i]);

	/* pass 1 initialization of technologies */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		if (tech->init != 0)
		{
			if ((*tech->init)(tech, 0))
				error(_("User pass 1 error initializing %s technology"), tech->techname);
		}
		if (tech_doinitprocess(tech))
			error(_("Pass 1 error initializing %s technology"), tech->techname);
	}

	/* pass 1 initialization of the tools */
	for(i=0; i<el_maxtools; i++)
		(*el_tools[i].init)(0, 0, &el_tools[i]);

	/* create the first library */
	el_curlib = NOLIBRARY;
	estrcpy(libname, x_("noname"));
	estrcpy(libfile, x_("noname"));
	lib = newlibrary(libname, libfile);
	selectlibrary(lib, TRUE);
}

/*
 * the secondary initialization of Electric
 */
void ossecondaryinit(INTBIG argc, CHAR1 *argv[])
{
	REGISTER INTBIG i;
	REGISTER TECHNOLOGY *tech;

	/* pass 2 initialization of the constraint solvers */
	for(i=0; el_constraints[i].conname != 0; i++)
		(*el_constraints[i].init)(NOCONSTRAINT);

	/* pass 2 initialization of technologies */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		if (tech->init != 0)
		{
			if ((*tech->init)(tech, 1))
				error(_("User pass 2 error initializing %s technology"), tech->techname);
		}
		if (tech_doaddportsandvars(tech))
			error(_("Pass 2 error initializing %s technology"), tech->techname);
	}

	/* pass 2 initialization of the tools */
	for(i=0; i<el_maxtools; i++) (*el_tools[i].init)(&argc, argv, NOTOOL);

	/* pass 3 initialization of technologies */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		if (tech->init != 0)
		{
			if ((*tech->init)(tech, 2))
				error(_("User pass 3 error initializing %s technology"), tech->techname);
		}
	}

	/* register and initialize technology-variable caching functions */
	db_inittechcache();

	/* pass 3 initialization of the tools */
	for(i=0; i<el_maxtools; i++) (*el_tools[i].init)(&argc, argv, 0);

	/* delete initialization change batches so it is ready for real changes */
	noundoallowed();

	/* make sure the integer sizes are correct */
	if (sizeof(INTHUGE) < 8)
		error(x_("INTHUGE must be 64 bits wide (fix 'global.h')\n"));
	if (sizeof(INTBIG) < 4)
		error(x_("INTBIG must be 32 bits wide (fix 'global.h')\n"));
	if (sizeof(INTSML) < 2)
		error(x_("INTSML must be 16 bits wide (fix 'global.h')\n"));
	if (sizeof(INTBIG) < sizeof(CHAR *))
		error(x_("INTBIG must be able to hold a pointer (%d bytes) bits wide (fix 'global.h')\n"),
			sizeof(CHAR *));
}

/* the body of the main loop of Electric */
void tooltimeslice(void)
{
	REGISTER INTBIG s;
	REGISTER TOOL *tool;

	for(s=0; s<el_maxtools; s++)
	{
		tool = &el_tools[s];
		if ((tool->toolstate & TOOLON) == 0) continue;
		if (tool->slice == 0) continue;

		/* let the tool have a whack */
		db_setcurrenttool(tool);
		(*tool->slice)();

		/* announce end of broadcast of changes */
		db_endbatch();
	}
}

void telltool(TOOL *tool, INTBIG count, CHAR *par[])
{
	if (tool->setmode == 0) return;
	(*tool->setmode)(count, par);
}

INTBIG asktool(TOOL *tool, CHAR *command, ...)
{
	va_list ap;
	INTBIG result;

	if (tool->request == 0) return(0);
	var_start(ap, command);
	result = (*tool->request)(command, ap);
	va_end(ap);
	return(result);
}

/*
 * routine to turn tool "tool" back on.  The user interface will notice this change
 * and inform the tool of each cell that changed while it was off (via "examinenodeproto").
 */
void toolturnon(TOOL *tool)
{
	/* turn on the tool */
	(void)setval((INTBIG)tool, VTOOL, x_("toolstate"), tool->toolstate|TOOLON, VINTEGER);
}

/*
 * routine to turn tool "tool" off.  If "permanently" is true, this change
 * will not be undoable
 */
void toolturnoff(TOOL *tool, BOOLEAN permanently)
{
	/* turn off the tool */
	if (permanently) tool->toolstate &= ~TOOLON; else
		(void)setval((INTBIG)tool, VTOOL, x_("toolstate"), tool->toolstate & ~TOOLON, VINTEGER);
}

/*
 * shutdown the design tool
 */
void bringdown(void)
{
	REGISTER INTBIG i;
	REGISTER TECHNOLOGY *tech;
	REGISTER TOOL *tool;
	REGISTER CONSTRAINT *con;

	/* go backwards through tools to shut down in proper order */
	for(i = el_maxtools-1; i >= 0; i--)
	{
		tool = &el_tools[i];
		if (tool->done != 0) (*tool->done)();
		if (tool->numvar != 0) db_freevars(&tool->firstvar, &tool->numvar);
	}

	/* terminate technologies */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		if (tech->term != 0) (*tech->term)();

	/* terminate any language interpreters */
	db_termlanguage();

	/* shut down the constraint solvers */
	for(i=0; el_constraints[i].conname != 0; i++)
	{
		con = &el_constraints[i];
		(*con->term)();
		if (con->numvar != 0) db_freevars(&con->firstvar, &con->numvar);
	}

	/* free up all memory */
	db_freeallmemory();

	/* check that there were no leaks */
	db_checkallmemoryfree();

	exitprogram();
}

void db_freeallmemory(void)
{
#ifdef DEBUGMEMORY
	REGISTER VIEW *v;
	REGISTER TECHNOLOGY *tech, *nexttech;
	REGISTER LIBRARY *lib;

	noundoallowed();

	/* free all views */
	while (el_views != NOVIEW)
	{
		v = el_views;
		el_views = el_views->nextview;
		efree((CHAR *)v->viewname);
		efree((CHAR *)v->sviewname);
		freeview(v);
	}

	/* free all technologies */
	for (tech = el_technologies; tech != NOTECHNOLOGY; tech = nexttech)
	{
		nexttech = tech->nexttechnology;
		freetechnology(tech);
	}

	/* free all libraries */
	while (el_curlib != NOLIBRARY)
	{
		lib = el_curlib;
		el_curlib = el_curlib->nextlibrary;
		eraselibrary(lib);
		efree(lib->libfile);
		efree(lib->libname);
		freelibrary(lib);
	}

	/* free all database memory */
	db_freetechnologymemory();
	db_freechangememory();
	db_freevariablememory();
	db_freenoprotomemory();
	db_freemathmemory();
	db_freemergememory();
	db_freegeomemory();
	db_freeerrormemory();
	db_freetextmemory();
	initerrorlogging(x_(""));
#  if LANGTCL
	db_freetclmemory();
#  endif
#endif
}

BOOLEAN stopping(INTBIG reason)
{
	extern CHAR *db_stoppingreason[];
	static UINTBIG lastchecktime = 0;
	REGISTER UINTBIG curtime;

	if ((us_tool->toolstate&TERMNOINTERRUPT) != 0) return(FALSE);

	/* only check once per second */
	curtime = ticktime();
	if (curtime - lastchecktime < 60) return(FALSE);
	lastchecktime = curtime;

	checkforinterrupt();
	if (el_pleasestop == 1)
	{
		flushscreen();
		ttyputmsg(_("%s stopped"), db_stoppingreason[reason]);
		el_pleasestop = 2;
	}
	return(el_pleasestop != 0);
}

/*
 * Routine to enable multiprocessor locks in the database (if "on" is TRUE).
 * Call this before firing-up threads so that database calls lock critical sections
 * properly.  Turn it off when thread usage has stopped.
 */
void setmultiprocesslocks(BOOLEAN on)
{
	static INTBIG threadcapability = 0;		/* 1: can  -1: can't */

	if (on)
	{
		if (threadcapability == 0)
		{
			if (graphicshas(CANDOTHREADS)) threadcapability = 1; else
				threadcapability = -1;
		}
		if (threadcapability < 0) return;
	}
	db_multiprocessing = on;
}

/*
 * Checking routine to ensure that no multiprocessor code is running.
 * Reports an error if so (because some non-reentrant code was called).
 */
void ensurenonparallel(CHAR *file, INTBIG line)
{
	if (db_multiprocessing)
	{
		ttyputerr(_("ERROR: non-reentrant code (line %ld of module %s) called by multiple processors"),
			line, file);
	}
}

/*
 * Routine to ensure that the mutual-exclusion lock at "mutex" is properly initialized
 * and unlocked.  If "showerror" is TRUE and an error occurs, display the error.
 * Returns TRUE on error.
 */
BOOLEAN ensurevalidmutex(void **mutex, BOOLEAN showerror)
{
	if (*mutex == 0)
	{
		*mutex = emakemutex();
		if (*mutex == 0)
		{
			if (!graphicshas(CANDOTHREADS)) return(FALSE);
			if (showerror)
				ttyputmsg(_("Error creating mutual-exclusion lock"));
			return(TRUE);
		}
	}
	emutexunlock(*mutex);
	return(FALSE);
}
