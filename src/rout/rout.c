/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: rout.c
 * Routines for the wire routing tool
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

#include "config.h"
#if ROUTTOOL

#include "global.h"
#include "rout.h"
#include "usr.h"
#include "tecschem.h"
#include "tecgen.h"
#include "edialogs.h"
#include "efunction.h"

/* the ROUTER tool table */
static COMCOMP routercellp = {NOKEYWORD, topofcells, nextcells, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Cell to stitch"), 0};
static COMCOMP routerarcp = {NOKEYWORD, topofarcs, nextarcs, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Arc prototype to use in stitching (* to reset)"), 0};
static KEYWORD routerautoopt[] =
{
	{x_("stitch-now"),             1,{&routercellp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("highlighted-stitch-now"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP routerautop = {routerautoopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Auto-stitching action"), 0};
static KEYWORD routermimicopt[] =
{
	{x_("do-now"),                 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("mimic-selected"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("enable"),                 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("stitch-and-unstitch"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("stitch-only"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("port-general"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("port-specific"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("any-arc-count"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("same-arc-count"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP routermimicp = {routermimicopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Mimic-stitching action"), 0};
static COMCOMP routerriverp = {NOKEYWORD, topofcells, nextcells, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Cell to river-route"), 0};
static COMCOMP routermazesetboundaryp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("routing boundary"), 0};
static COMCOMP routermazesetgridxp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("routing grid x"), 0};
static COMCOMP routermazesetgridyp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("routing grid y"), 0};
static COMCOMP routermazesetoffsetxp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("routing offset x"), 0};
static COMCOMP routermazesetoffsetyp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("routing offset y"), 0};
static KEYWORD routermazesetopt[] =
{
	{x_("boundary"),			 1,{&routermazesetboundaryp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("grid"),				 2,{&routermazesetgridxp,&routermazesetgridyp,NOKEY,NOKEY,NOKEY}},
	{x_("offset"),			     2,{&routermazesetoffsetxp,&routermazesetoffsetyp,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP routermazesetp = {routermazesetopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("set operation"), 0 };
static KEYWORD routermazerouteopt[] =
{
	{x_("cell"),	 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("selected"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP routermazeroutep = {routermazerouteopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("routing operation"), 0 };
static KEYWORD routermazeopt[] =
{
	{x_("set"),	    1, {&routermazesetp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("route"),   1, {&routermazeroutep,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP routermazep = {routermazeopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("Maze routing operation"), 0 };
static KEYWORD routernopt[] =
{
	{x_("select"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP routernp = {routernopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Negating action"), 0};
static KEYWORD routeropt[] =
{
	{x_("auto-stitch"),    1,{&routerautop,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("mimic-stitch"),   1,{&routermimicp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("river-route"),    1,{&routerriverp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("maze-route"),     1,{&routermazep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("use-arc"),        1,{&routerarcp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("select"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no-stitch"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("unroute"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("copy-topology"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("paste-topology"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("not"),            1,{&routernp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP ro_routerp = {routeropt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("Routing action"), 0};

       RCHECK      *ro_firstrcheck;
static RCHECK      *ro_rcheckfree;

static TOOL        *ro_source;			/* source of batch of changes */
       INTBIG       ro_statekey;		/* variable key for "ROUT_state" */
       INTBIG       ro_state;			/* cached value for "ROUT_state" */
static INTBIG       ro_optionskey;		/* variable key for "ROUT_options" */
       INTBIG       ro_preferedkey;		/* variable key for "ROUT_prefered_arc" */
	   NODEPROTO   *ro_copiedtopocell;	/* cell whose topology is being copied */
       TOOL        *ro_tool;			/* the Router tool object */
       POLYLIST    *ro_autostitchplist;	/* for auto-stitching */

static ROUTACTIVITY ro_curroutactivity;	/* routing activity in the current slice */
       ROUTACTIVITY ro_lastactivity;	/* last routing activity */
	   NODEINST    *ro_deletednodes[2];	/* nodes at end of last deleted arc */
       PORTPROTO   *ro_deletedports[2];	/* ports on nodes at end of last deleted arc */

/* working memory for "ro_findnetends()" */
static INTBIG       ro_findnetlisttotal = 0;
static NODEINST   **ro_findnetlistni;
static PORTPROTO  **ro_findnetlistpp;
static INTBIG      *ro_findnetlistx;
static INTBIG      *ro_findnetlisty;

/* prototypes for local routines */
static RCHECK *ro_allocrcheck(void);
static void    ro_queuercheck(NODEINST*);
static void    ro_unroutecurrent(void);
static BOOLEAN ro_unroutenet(NETWORK *net);
static void    ro_optionsdlog(void);
static void    ro_pastetopology(NODEPROTO *np);
static int     ro_sortinstspatially(const void *e1, const void *e2);
static BOOLEAN ro_makeunroutedconnection(NODEINST *fni, PORTPROTO *fpp, NODEINST *tni, PORTPROTO *tpp);

/*
 * routine to allocate a new check module from the pool (if any) or memory
 */
RCHECK *ro_allocrcheck(void)
{
	REGISTER RCHECK *r;

	if (ro_rcheckfree == NORCHECK)
	{
		r = (RCHECK *)emalloc(sizeof (RCHECK), ro_tool->cluster);
		if (r == 0) return(NORCHECK);
	} else
	{
		r = ro_rcheckfree;
		ro_rcheckfree = (RCHECK *)r->nextcheck;
	}
	return(r);
}

/*
 * routine to return check module "r" to the pool of free modules
 */
void ro_freercheck(RCHECK *r)
{
	r->nextcheck = ro_rcheckfree;
	ro_rcheckfree = r;
}

/*
 * routine to queue nodeinst "ni" to be checked during the next slice of
 * the router
 */
void ro_queuercheck(NODEINST *ni)
{
	REGISTER RCHECK *r;

	r = ro_allocrcheck();
	if (r == NORCHECK)
	{
		ttyputnomemory();
		return;
	}
	r->entity = ni;
	r->nextcheck = ro_firstrcheck;
	ro_firstrcheck = r;
}

void ro_init(INTBIG *argc, CHAR1 *argv[], TOOL *thistool)
{
	Q_UNUSED( argc );
	Q_UNUSED( argv );

	if (thistool == 0)
	{
		/* pass 3: nothing to do */
		/* EMPTY */ 
	} else if (thistool == NOTOOL)
	{
		/* pass 2: nothing to do */
		/* EMPTY */ 
	} else
	{
		/* pass 1: initialize */
		ro_tool = thistool;

		/* set default mode for router: cell stitching */
		ro_statekey = makekey(x_("ROUT_state"));
		ro_optionskey = makekey(x_("ROUT_options"));
		ro_preferedkey = makekey(x_("ROUT_prefered_arc"));
		nextchangequiet();
		ro_state = NOSTITCH;
		(void)setvalkey((INTBIG)ro_tool, VTOOL, ro_statekey, ro_state, VINTEGER|VDONTSAVE);

		/* initialize the free lists */
		ro_rcheckfree = NORCHECK;
		ro_firstrcheck = NORCHECK;

		/* no last activity to mimic */
		ro_lastactivity.numcreatedarcs = ro_lastactivity.numcreatednodes = 0;
		ro_lastactivity.numdeletedarcs = ro_lastactivity.numdeletednodes = 0;

		/* no cell topology was copied */
		ro_copiedtopocell = NONODEPROTO;

		/* setup polylist */
		ro_autostitchplist = allocpolylist(ro_tool->cluster);
		if (ro_autostitchplist == 0) return;

		/* delcare dialogs */
		DiaDeclareHook(x_("routeopt"), &routerarcp, ro_optionsdlog);
	}
}

void ro_done(void)
{
#ifdef DEBUGMEMORY
	RCHECK *r;

	/* free all check modules */
	while (ro_firstrcheck != NORCHECK)
	{
		r = ro_firstrcheck;
		ro_firstrcheck = ro_firstrcheck->nextcheck;
		ro_freercheck(r);
	}
	while (ro_rcheckfree != NORCHECK)
	{
		r = ro_rcheckfree;
		ro_rcheckfree = ro_rcheckfree->nextcheck;
		efree((CHAR *)r);
	}

	if (ro_findnetlisttotal > 0)
	{
		efree((CHAR *)ro_findnetlistni);
		efree((CHAR *)ro_findnetlistpp);
		efree((CHAR *)ro_findnetlistx);
		efree((CHAR *)ro_findnetlisty);
	}

	freepolylist(ro_autostitchplist);
	ro_freeautomemory();
	ro_freemimicmemory();
	ro_freerivermemory();
	ro_freemazememory();
#endif
}

void ro_set(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG l, savestate;
	REGISTER NODEPROTO *np;
	REGISTER ARCPROTO *ap;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni;
	REGISTER CHAR *pp;
	REGISTER GEOM **list;
	REGISTER VARIABLE *var;

	if (count == 0)
	{
		switch (ro_state&STITCHMODE)
		{
			case NOSTITCH:
				ttyputmsg(M_("No stitching being done"));          break;
			case AUTOSTITCH:
				ttyputmsg(M_("Default is auto-stitch routing"));   break;
			case MIMICSTITCH:
				ttyputmsg(M_("Default is mimic-stitch routing"));  break;
		}
		var = getvalkey((INTBIG)ro_tool, VTOOL, VARCPROTO, ro_preferedkey);
		if (var != NOVARIABLE)
			ttyputmsg(M_("Default arc for river routing is %s"),
				describearcproto(((ARCPROTO *)var->addr)));
		return;
	}

	l = estrlen(pp = par[0]);

	if (namesamen(pp, x_("no-stitch"), l) == 0)
	{
		(void)setvalkey((INTBIG)ro_tool, VTOOL, ro_statekey,
			(ro_state & ~STITCHMODE) | NOSTITCH, VINTEGER|VDONTSAVE);
		return;
	}

	if (namesamen(pp, x_("auto-stitch"), l) == 0)
	{
		if (count <= 1) return;
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("enable"), l) == 0)
		{
			(void)setvalkey((INTBIG)ro_tool, VTOOL, ro_statekey,
				(ro_state & ~STITCHMODE) | AUTOSTITCH, VINTEGER|VDONTSAVE);
			return;
		}
		if (namesamen(pp, x_("stitch-now"), l) == 0)
		{
			if (count >= 3)
			{
				np = getnodeproto(par[2]);
				if (np == NONODEPROTO)
				{
					ttyputerr(_("No cell named %s"), par[2]);
					return;
				}
				if (np->primindex != 0)
				{
					ttyputerr(M_("Can only stitch cells, not primitives"));
					return;
				}
				if (np->lib != el_curlib)
				{
					ttyputerr(_("Can only stitch cells in the current library"));
					return;
				}
			} else
			{
				np = getcurcell();
				if (np == NONODEPROTO)
				{
					ttyputerr(_("No current cell"));
					return;
				}
			}

			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				/* ignore recursive references (showing icon in contents) */
				if (isiconof(ni->proto, np)) continue;
				ro_queuercheck(ni);
			}

			/* fake enabling of auto-stitching */
			savestate = ro_state;
			ro_state |= AUTOSTITCH;

			/* do the stitching */
			ro_slice();

			/* restore routing state */
			ro_state = savestate;
			ttyputverbose(M_("Cell %s stitched"), describenodeproto(np));
			return;
		}
		if (namesamen(pp, x_("highlighted-stitch-now"), l) == 0)
		{
			list = (GEOM **)asktool(us_tool, x_("get-all-nodes"));
			if (list[0] == NOGEOM)
			{
				ttyputerr(_("Select an area to be stitched"));
				return;
			}
			for(l=0; list[l] != NOGEOM; l++)
				ro_queuercheck(list[l]->entryaddr.ni);

			/* fake enabling of auto-stitching */
			savestate = ro_state;
			ro_state |= AUTOSTITCH;

			/* do the stitching */
			ro_slice();

			/* restore routing state */
			ro_state = savestate;
			ttyputverbose(M_("Stitching complete"));
			return;
		}
		ttyputbadusage(x_("telltool routing auto-stitch"));
		return;
	}

	if (namesamen(pp, x_("mimic-stitch"), l) == 0 && l > 1)
	{
		if (count < 2)
		{
			ttyputusage(x_("telltool routing mimic-stitch OPTION"));
			return;
		}
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("do-now"), l) == 0)
		{
			ro_mimicstitch(TRUE);
			return;
		}
		if (namesamen(pp, x_("mimic-selected"), l) == 0)
		{
			ai = (ARCINST *)us_getobject(VARCINST, FALSE);
			if (ai == NOARCINST) return;
			ro_lastactivity.numdeletedarcs = 0;
			ro_lastactivity.numcreatedarcs = 1;
			ro_lastactivity.createdarcs[0] = ai;

			ro_mimicstitch(TRUE);
			return;
		}
		if (namesamen(pp, x_("enable"), l) == 0)
		{
			(void)setvalkey((INTBIG)ro_tool, VTOOL, ro_statekey,
				(ro_state & ~STITCHMODE) | MIMICSTITCH, VINTEGER|VDONTSAVE);
			return;
		}
		if (namesamen(pp, x_("stitch-and-unstitch"), l) == 0 && l >= 8)
		{
			(void)setvalkey((INTBIG)ro_tool, VTOOL, ro_optionskey,
				ro_getoptions() | MIMICUNROUTES, VINTEGER);
			ttyputverbose(M_("Mimic stitcher will route and unroute"));
			return;
		}
		if (namesamen(pp, x_("stitch-only"), l) == 0 && l >= 8)
		{
			(void)setvalkey((INTBIG)ro_tool, VTOOL, ro_optionskey,
				ro_getoptions() & ~MIMICUNROUTES, VINTEGER);
			ttyputverbose(M_("Mimic stitcher will only route, not unroute"));
			return;
		}
		if (namesamen(pp, x_("port-general"), l) == 0 && l >= 8)
		{
			(void)setvalkey((INTBIG)ro_tool, VTOOL, ro_optionskey,
				ro_getoptions() | MIMICIGNOREPORTS, VINTEGER);
			ttyputverbose(M_("Mimic stitcher will route to any port"));
			return;
		}
		if (namesamen(pp, x_("port-specific"), l) == 0 && l >= 8)
		{
			(void)setvalkey((INTBIG)ro_tool, VTOOL, ro_optionskey,
				ro_getoptions() & ~MIMICIGNOREPORTS, VINTEGER);
			ttyputverbose(M_("Mimic stitcher will rout only to similar ports"));
			return;
		}
		if (namesamen(pp, x_("any-arc-count"), l) == 0 && l >= 8)
		{
			(void)setvalkey((INTBIG)ro_tool, VTOOL, ro_optionskey,
				ro_getoptions() & ~MIMICONSIDERARCCOUNT, VINTEGER);
			ttyputverbose(M_("Mimic stitcher will route without regard to arc count"));
			return;
		}
		if (namesamen(pp, x_("same-arc-count"), l) == 0 && l >= 8)
		{
			(void)setvalkey((INTBIG)ro_tool, VTOOL, ro_optionskey,
				ro_getoptions() | MIMICONSIDERARCCOUNT, VINTEGER);
			ttyputverbose(M_("Mimic stitcher will rout only where arc counts are the same"));
			return;
		}
		ttyputbadusage(x_("telltool routing mimic-stitching"));
		return;
	}

	if (namesamen(pp, x_("river-route"), l) == 0)
	{
		if (count >= 2)
		{
			np = getnodeproto(par[1]);
			if (np == NONODEPROTO)
			{
				ttyputerr(_("No cell named %s"), par[1]);
				return;
			}
			if (np->primindex != 0)
			{
				ttyputerr(M_("Can only route cells, not primitives"));
				return;
			}
		} else
		{
			np = getcurcell();
			if (np == NONODEPROTO)
			{
				ttyputerr(_("No current cell"));
				return;
			}
		}

		if (ro_river(np)) ttyputverbose(M_("Cell %s river-routed"), describenodeproto(np)); else
			ttyputmsg(_("Routing not successful"));			
		return;
	}

	if (namesamen(pp, x_("maze-route"), l) == 0 && l > 1)
	{
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("route"), l) == 0)
		{
			if (count <= 2) return;
			l = estrlen(pp = par[2]);
			if (namesamen(pp, x_("cell"), l) == 0)
			{
				ro_mazeroutecell();
				return;
			}
			if (namesamen(pp, x_("selected"), l) == 0)
			{
				ro_mazerouteselected();
				return;
			}
			ttyputbadusage(x_("telltool routing maze-route"));
			return;
		}
		if (namesamen(pp, x_("set"), l) == 0)
		{
			if (count <= 2)
			{
				ttyputmsg(M_("Routing grid: %sx%s"), latoa(ro_mazegridx, 0), latoa(ro_mazegridy, 0));
				ttyputmsg(M_("Routing offset: (%s,%s)"), latoa(ro_mazeoffsetx, 0), latoa(ro_mazeoffsety, 0));
				ttyputmsg(M_("Routing boundary: %s"), latoa(ro_mazeboundary, 0));
				return;
			}

			l = estrlen(pp = par[2]);
			if (namesamen(pp, x_("grid"), l) == 0)
			{
				ro_mazegridx = atola(par[3], 0);
				ro_mazegridy = atola(par[4], 0);
				ttyputmsg(M_("Routing grid: %sx%s"), latoa(ro_mazegridx, 0), latoa(ro_mazegridy, 0));
				return;
			}
			if (namesamen(pp, x_("offset"), l) == 0)
			{
				ro_mazeoffsetx = atola(par[3], 0);
				ro_mazeoffsety = atola(par[4], 0);
				ttyputmsg(M_("Routing offset: (%s,%s)"), latoa(ro_mazeoffsetx, 0), latoa(ro_mazeoffsety, 0));
				return;
			}
			if (namesamen(pp, x_("boundary"), l) == 0)
			{
				ro_mazeboundary = atola(par[3], 0);
				ttyputmsg(M_("Routing boundary: %s"), latoa(ro_mazeboundary, 0));
				return;
			}

			ttyputbadusage(x_("telltool routing maze-route set"));
			return;
		}
	}

	if (namesamen(pp, x_("copy-topology"), l) == 0)
	{
		np = getcurcell();
		if (np == NONODEPROTO)
		{
			ttyputerr(_("No current cell"));
			return;
		}
		ro_copiedtopocell = np;
		return;
	}
	if (namesamen(pp, x_("paste-topology"), l) == 0 && l >= 3)
	{
		np = getcurcell();
		if (np == NONODEPROTO)
		{
			ttyputerr(_("No current cell"));
			return;
		}
		ro_pastetopology(np);
		return;
	}

	if (namesamen(pp, x_("use-arc"), l) == 0)
	{
		if (count < 2)
		{
			ttyputusage(x_("telltool routing use-arc ARCPROTO"));
			return;
		}
		if (estrcmp(par[1], x_("*")) == 0)
		{
			if (getvalkey((INTBIG)ro_tool, VTOOL, VARCPROTO, ro_preferedkey) != NOVARIABLE)
				(void)delvalkey((INTBIG)ro_tool, VTOOL, ro_preferedkey);
			ttyputverbose(M_("No arc will be presumed for stitching"));
			return;
		}
		ap = getarcproto(par[1]);
		if (ap == NOARCPROTO)
		{
			ttyputerr(M_("No arc prototype called %s"), par[1]);
			return;
		}
		ttyputverbose(M_("Default stitching arc will be %s"), describearcproto(ap));
		(void)setvalkey((INTBIG)ro_tool, VTOOL, ro_preferedkey, (INTBIG)ap, VARCPROTO);
		return;
	}

	if (namesamen(pp, x_("unroute"), l) == 0)
	{
		ro_unroutecurrent();
		return;
	}

	if (namesamen(pp, x_("not"), l) == 0)
	{
		if (count <= 1)
		{
			ttyputusage(x_("telltool routing not OPTION"));
			return;
		}
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("select"), l) == 0)
		{
			(void)setvalkey((INTBIG)ro_tool, VTOOL, ro_statekey,
				ro_state & ~SELECT, VINTEGER|VDONTSAVE);
			ttyputverbose(M_("Wire placement not subject to approval"));
			return;
		}
		ttyputbadusage(x_("telltool routing not"));
		return;
	}

	if (namesamen(pp, x_("select"), l) == 0)
	{
		(void)setvalkey((INTBIG)ro_tool, VTOOL, ro_statekey,
			ro_state | SELECT, VINTEGER|VDONTSAVE);
		ttyputverbose(M_("Wire placement subject to approval"));
		return;
	}

	ttyputbadusage(x_("telltool routing"));
}

void ro_slice(void)
{
	if ((ro_state&(SELSKIP|SELDONE)) != 0)
	{
		ro_state &= ~(SELSKIP|SELDONE);
		(void)setvalkey((INTBIG)ro_tool, VTOOL, ro_statekey, ro_state, VINTEGER|VDONTSAVE);
	}

	switch (ro_state&STITCHMODE)
	{
		case AUTOSTITCH:
			ro_autostitch();
			setactivity(_("Cells stitched"));
			break;
		case MIMICSTITCH:
			ro_mimicstitch(FALSE);
			setactivity(_("Cells stitched"));
			break;
	}
}

/******************** DATABASE CHANGES ********************/

void ro_startbatch(TOOL *source, BOOLEAN undoredo)
{
	Q_UNUSED( undoredo );

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	ro_source = source;

	/* clear the record of routing activity in this batch */
	ro_curroutactivity.numcreatedarcs = ro_curroutactivity.numcreatednodes = 0;
	ro_curroutactivity.numdeletedarcs = ro_curroutactivity.numdeletednodes = 0;
}

void ro_endbatch(void)
{
	REGISTER INTBIG i;

	/* if there was any routing activity not by this tool, set this to the last one */
	if (ro_curroutactivity.numcreatedarcs != 0 || ro_curroutactivity.numcreatednodes != 0 ||
		ro_curroutactivity.numdeletedarcs != 0 || ro_curroutactivity.numdeletednodes != 0)
	{
		ro_lastactivity.numcreatedarcs = ro_curroutactivity.numcreatedarcs;
		for(i=0; i<ro_lastactivity.numcreatedarcs; i++)
			ro_lastactivity.createdarcs[i] = ro_curroutactivity.createdarcs[i];
		ro_lastactivity.numcreatednodes = ro_curroutactivity.numcreatednodes;
		for(i=0; i<ro_lastactivity.numcreatednodes; i++)
			ro_lastactivity.creatednodes[i] = ro_curroutactivity.creatednodes[i];
		ro_lastactivity.numdeletedarcs = ro_curroutactivity.numdeletedarcs;
		for(i=0; i<ro_lastactivity.numdeletedarcs; i++)
			ro_lastactivity.deletedarcs[i] = ro_curroutactivity.deletedarcs[i];
		ro_lastactivity.numdeletednodes = ro_curroutactivity.numdeletednodes;
		for(i=0; i<ro_lastactivity.numdeletednodes; i++)
			ro_lastactivity.deletednodes[i] = ro_curroutactivity.deletednodes[i];
	}
}

void ro_modifynodeinst(NODEINST *ni, INTBIG oldlx, INTBIG oldly, INTBIG oldhx,
	INTBIG oldhy, INTBIG oldrot, INTBIG oldtran)
{
	Q_UNUSED( oldlx );
	Q_UNUSED( oldly );
	Q_UNUSED( oldhx );
	Q_UNUSED( oldhy );
	Q_UNUSED( oldrot );
	Q_UNUSED( oldtran );

	if ((ro_state&STITCHMODE) == AUTOSTITCH)
	{
		if (ro_source != ro_tool) ro_queuercheck(ni);
	}
}

void ro_modifyarcinst(ARCINST *ai, INTBIG oldxA, INTBIG oldyA, INTBIG oldxB,
	INTBIG oldyB, INTBIG oldwid, INTBIG oldlen)
{
	Q_UNUSED( ai );
	Q_UNUSED( oldxA );
	Q_UNUSED( oldyA );
	Q_UNUSED( oldxB );
	Q_UNUSED( oldyB );
	Q_UNUSED( oldwid );
	Q_UNUSED( oldlen );
}

void ro_modifyportproto(PORTPROTO *pp, NODEINST *oldsubni, PORTPROTO *oldsubpp)
{
	Q_UNUSED( pp );
	Q_UNUSED( oldsubni );
	Q_UNUSED( oldsubpp );
}

void ro_newobject(INTBIG addr, INTBIG type)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;

	if (ro_source == ro_tool) return;
	if ((type&VTYPE) == VNODEINST)
	{
		ni = (NODEINST *)addr;
		if (ro_curroutactivity.numcreatednodes < 3)
			ro_curroutactivity.creatednodes[ro_curroutactivity.numcreatednodes++] = ni;
		if ((ro_state&STITCHMODE) == AUTOSTITCH)
			ro_queuercheck(ni);
	} else if ((type&VTYPE) == VARCINST)
	{
		ai = (ARCINST *)addr;
		if (ro_curroutactivity.numcreatedarcs < 3)
			ro_curroutactivity.createdarcs[ro_curroutactivity.numcreatedarcs++] = ai;
	}
}

void ro_killobject(INTBIG addr, INTBIG type)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;

	if (ro_source == ro_tool) return;
	if ((type&VTYPE) == VNODEINST)
	{
		ni = (NODEINST *)addr;
		if (ro_curroutactivity.numdeletednodes < 3)
			ro_curroutactivity.deletednodes[ro_curroutactivity.numdeletednodes++] = ni;
	} else if ((type&VTYPE) == VARCINST)
	{
		ai = (ARCINST *)addr;
		if (ro_curroutactivity.numdeletedarcs < 2)
			ro_curroutactivity.deletedarcs[ro_curroutactivity.numdeletedarcs++] = ai;

		ro_deletednodes[0] = ai->end[0].nodeinst;
		ro_deletednodes[1] = ai->end[1].nodeinst;
		ro_deletedports[0] = ai->end[0].portarcinst->proto;
		ro_deletedports[1] = ai->end[1].portarcinst->proto;
	}
}

void ro_newvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG newtype)
{
	REGISTER VARIABLE *var;

	if ((newtype&VCREF) != 0) return;
	if (key == ro_statekey)
	{
		var = getvalkey(addr, type, VINTEGER, key);
		if (var != NOVARIABLE) ro_state = var->addr;
		return;
	}
}

/*********************** CODE TO UNROUTE (CONVERT TO UNROUTED WIRE) ***********************/

/* Routine to convert the current network(s) to an unrouted wire */
void ro_unroutecurrent(void)
{
	GEOM **list;
	REGISTER NETWORK *net;
	REGISTER NODEPROTO *np;
	REGISTER ARCINST *ai;
	REGISTER INTBIG i, selectcount;
	REGISTER BOOLEAN found;

	/* see what is highlighted */
	list = us_gethighlighted(WANTARCINST, 0, 0);
	for(selectcount=0; list[selectcount] != NOGEOM; selectcount++) ;
	if (selectcount == 0)
	{
		ttyputerr(_("Must select arcs to unroute"));
		return;
	}
	np = geomparent(list[0]);

	/* convert requested nets */
	us_clearhighlightcount();
	found = TRUE;
	while (found)
	{
		/* no net found yet */
		found = FALSE;

		/* find a net to be routed */
		for(net = np->firstnetwork; net != NONETWORK;
			net = net->nextnetwork)
		{
			/* is the net included in the selection list? */
			for(i=0; i<selectcount; i++)
			{
				if (list[i] == NOGEOM) continue;
				if (list[i]->entryisnode) continue;
				ai = list[i]->entryaddr.ai;
				if (ai->network == net) break;
			}
			if (i >= selectcount) continue;

			/* yes: remove all arcs that have this net from the selection */
			for(i=0; i<selectcount; i++)
			{
				if (list[i] == NOGEOM) continue;
				if (list[i]->entryisnode) continue;
				ai = list[i]->entryaddr.ai;
				if (ai->network == net) list[i] = NOGEOM;
			}

			/* now unroute the net */
			if (ro_unroutenet(net)) return;
			found = TRUE;
			break;
		}
	}
}

BOOLEAN ro_unroutenet(NETWORK *net)
{
	INTBIG *xlist, *ylist, count;
	NODEINST **nilist;
	PORTPROTO **pplist;
	REGISTER ARCINST *ai, *nextai;
	REGISTER NODEINST *ni, *nextni;
	REGISTER NODEPROTO *np;
	REGISTER INTBIG bits, wid, i, j, dist, bestdist, first, found, besti, bestj;
	REGISTER INTBIG *covered;

	/* convert this net and mark arcs and nodes on it */
	count = ro_findnetends(net, &nilist, &pplist, &xlist, &ylist);

	/* remove marked nodes and arcs */
	np = net->parent;
	for(ai = np->firstarcinst; ai != NOARCINST; ai = nextai)
	{
		nextai = ai->nextarcinst;
		if (ai->temp1 == 0) continue;
		startobjectchange((INTBIG)ai, VARCINST);
		if (killarcinst(ai))
		{
			ttyputerr(_("Error deleting arc"));
			return(TRUE);
		}
	}
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = nextni)
	{
		nextni = ni->nextnodeinst;
		if (ni->temp1 == 0) continue;
		startobjectchange((INTBIG)ni, VNODEINST);
		if (killnodeinst(ni))
		{
			ttyputerr(_("Error deleting intermediate node"));
			return(TRUE);
		}
	}

	/* now create the new unrouted wires */
	bits = us_makearcuserbits(gen_unroutedarc);
	wid = defaultarcwidth(gen_unroutedarc);
	covered = (INTBIG *)emalloc(count * SIZEOFINTBIG, el_tempcluster);
	if (covered == 0) return(FALSE);
	for(i=0; i<count; i++) covered[i] = 0;
	for(first=0; ; first++)
	{
		found = 1;
		bestdist = besti = bestj = 0;
		for(i=0; i<count; i++)
		{
			for(j=i+1; j<count; j++)
			{
				if (first != 0)
				{
					if (covered[i] + covered[j] != 1) continue;
				}
				dist = computedistance(xlist[i], ylist[i], xlist[j], ylist[j]);

				/* LINTED "bestdist" used in proper order */
				if (found == 0 && dist >= bestdist) continue;
				found = 0;
				bestdist = dist;
				besti = i;
				bestj = j;
			}
		}
		if (found != 0) break;

		covered[besti] = covered[bestj] = 1;
		ai = newarcinst(gen_unroutedarc, wid, bits,
			nilist[besti], pplist[besti], xlist[besti], ylist[besti],
			nilist[bestj], pplist[bestj], xlist[bestj], ylist[bestj], np);
		if (ai == NOARCINST)
		{
			ttyputerr(_("Could not create unrouted arc"));
			return(TRUE);
		}
		endobjectchange((INTBIG)ai, VARCINST);
		(void)asktool(us_tool, x_("show-object"), (INTBIG)ai->geom);
	}
	return(FALSE);
}

/*
 * Routine to find the endpoints of network "net" and store them in the array
 * "ni/pp/xp/yp".  Returns the number of nodes in the array.
 * As a side effect, sets "temp1" on nodes and arcs to nonzero if they are part
 * of the network.
 */
INTBIG ro_findnetends(NETWORK *net, NODEINST ***nilist, PORTPROTO ***pplist,
	INTBIG **xplist, INTBIG **yplist)
{
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER PORTARCINST *pi, *thispi;
	REGISTER BOOLEAN term;
	INTBIG listcount, newtotal, i, j;
	NODEINST **newlistni;
	PORTPROTO **newlistpp;
	INTBIG *newlistx;
	INTBIG *newlisty;

	/* initialize */
	np = net->parent;
	listcount = 0;
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst) ai->temp1 = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst) ni->temp1 = 0;

	/* look at every arc and see if it is part of the network */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->network != net) continue;
		ai->temp1 = 1;

		/* see if an end of the arc is a network "end" */
		for(i=0; i<2; i++)
		{
			ni = ai->end[i].nodeinst;
			thispi = ai->end[i].portarcinst;
			term = FALSE;
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				if (pi != thispi && pi->conarcinst->network == net) break;
			if (pi == NOPORTARCINST) term = TRUE;
			if (ni->firstportexpinst != NOPORTEXPINST) term = TRUE;
			if (ni->proto->primindex == 0) term = TRUE;
			if (term)
			{
				/* valid network end: see if it is in the list */
				for(j=0; j<listcount; j++)
					if (ni == ro_findnetlistni[j] && thispi->proto == ro_findnetlistpp[j])
						break;
				if (j < listcount) continue;

				/* add it to the list */
				if (listcount >= ro_findnetlisttotal)
				{
					newtotal = listcount * 2;
					if (newtotal == 0) newtotal = 10;
					newlistni = (NODEINST **)emalloc(newtotal * (sizeof (NODEINST *)),
						ro_tool->cluster);
					newlistpp = (PORTPROTO **)emalloc(newtotal * (sizeof (PORTPROTO *)),
						ro_tool->cluster);
					newlistx = (INTBIG *)emalloc(newtotal * SIZEOFINTBIG,
						ro_tool->cluster);
					newlisty = (INTBIG *)emalloc(newtotal * SIZEOFINTBIG,
						ro_tool->cluster);
					for(j=0; j<listcount; j++)
					{
						newlistni[j] = ro_findnetlistni[j];
						newlistpp[j] = ro_findnetlistpp[j];
						newlistx[j] = ro_findnetlistx[j];
						newlisty[j] = ro_findnetlisty[j];
					}
					if (ro_findnetlisttotal > 0)
					{
						efree((CHAR *)ro_findnetlistni);
						efree((CHAR *)ro_findnetlistpp);
						efree((CHAR *)ro_findnetlistx);
						efree((CHAR *)ro_findnetlisty);
					}
					ro_findnetlistni = newlistni;
					ro_findnetlistpp = newlistpp;
					ro_findnetlistx = newlistx;
					ro_findnetlisty = newlisty;
					ro_findnetlisttotal = newtotal;
				}
				ro_findnetlistni[listcount] = ni;
				ro_findnetlistpp[listcount] = thispi->proto;
				ro_findnetlistx[listcount] = ai->end[i].xpos;
				ro_findnetlisty[listcount] = ai->end[i].ypos;
				listcount++;
			} else
			{
				/* not a network end: mark the node for removal */
				ni->temp1 = 1;
			}
		}
	}
	*nilist = ro_findnetlistni;
	*pplist = ro_findnetlistpp;
	*xplist = ro_findnetlistx;
	*yplist = ro_findnetlisty;
	return(listcount);
}

INTBIG ro_getoptions(void)
{
	REGISTER VARIABLE *var;

	var = getvalkey((INTBIG)ro_tool, VTOOL, VINTEGER, ro_optionskey);
	if (var != NOVARIABLE) return(var->addr);
	return(0);
}

/****************************** TOPOLOGY COPYING ******************************/

/*
 * Routine to copy the topology of cell "ro_copiedtopocell" to cell "tonp".
 */
void ro_pastetopology(NODEPROTO *tonp)
{
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni, *oni, *tni, *otni, *fni, *ofni, **fromlist, **tolist;
	REGISTER ARCINST *ai, *oai;
	REGISTER PORTPROTO *tpp, *otpp, *fpp;
	REGISTER PORTARCINST *fpi, *ofpi;
	REGISTER NETWORK *net;
	REGISTER INTBIG fromcount, tocount, i, j, fun, ofun;
	REGISTER CHAR *matchname;
	REGISTER VARIABLE *var, *ovar;

	if (ro_copiedtopocell == NONODEPROTO)
	{
		ttyputerr(_("Must copy topology before pasting it"));
		return;
	}

	/* first validate the source cell */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			if (np == ro_copiedtopocell) break;
		if (np != NONODEPROTO) break;
	}
	if (lib == NOLIBRARY)
	{
		ttyputerr(_("Copied cell is no longer valid"));
		return;
	}

	/* make sure copy goes to a different cell */
	if (ro_copiedtopocell == tonp)
	{
		ttyputerr(_("Topology must be copied to a different cell"));
		return;
	}

	/* reset association pointers in the destination cell */
	for(ni = tonp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		ni->temp1 = 0;

	/* look for associations */
	for(ni = tonp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->temp1 != 0) continue;

		/* ignore connecting nodes */
		fun = NPUNKNOWN;
		if (ni->proto->primindex != 0)
		{
			fun = nodefunction(ni);
			if (fun == NPUNKNOWN || fun == NPPIN || fun == NPCONTACT ||
				fun == NPNODE) continue;
		}

		/* count the number of this type of node in the two cells */
		fromcount = 0;
		for(oni = ro_copiedtopocell->firstnodeinst; oni != NONODEINST; oni = oni->nextnodeinst)
		{
			if (ni->proto->primindex == 0)
			{
				if (insamecellgrp(oni->proto, ni->proto)) fromcount++;
			} else
			{
				ofun = nodefunction(oni);
				if (ofun == fun) fromcount++;
			}
		}
		tocount = 0;
		for(oni = tonp->firstnodeinst; oni != NONODEINST; oni = oni->nextnodeinst)
		{
			if (ni->proto->primindex == 0)
			{
				if (oni->proto == ni->proto) tocount++;
			} else
			{
				ofun = nodefunction(oni);
				if (ofun == fun) tocount++;
			}
		}

		/* problem if the numbers don't match */
		if (tocount != fromcount)
		{
			if (fromcount == 0) continue;
			ttyputerr(_("Warning: %s has %ld of %s but cell %s has %ld"),
				describenodeproto(ro_copiedtopocell), fromcount, describenodeproto(ni->proto),
				describenodeproto(tonp), tocount);
			return;
		}

		/* gather all of the instances */
		fromlist = (NODEINST **)emalloc(fromcount * (sizeof (NODEINST *)), ro_tool->cluster);
		if (fromlist == 0) return;
		fromcount = 0;
		for(oni = ro_copiedtopocell->firstnodeinst; oni != NONODEINST; oni = oni->nextnodeinst)
		{
			if (ni->proto->primindex == 0)
			{
				if (insamecellgrp(oni->proto, ni->proto)) fromlist[fromcount++] = oni;
			} else
			{
				ofun = nodefunction(oni);
				if (ofun == fun) fromlist[fromcount++] = oni;
			}
		}
		tolist = (NODEINST **)emalloc(tocount * (sizeof (NODEINST *)), ro_tool->cluster);
		if (tolist == 0) return;
		tocount = 0;
		for(oni = tonp->firstnodeinst; oni != NONODEINST; oni = oni->nextnodeinst)
		{
			if (ni->proto->primindex == 0)
			{
				if (oni->proto == ni->proto) tolist[tocount++] = oni;
			} else
			{
				ofun = nodefunction(oni);
				if (ofun == fun) tolist[tocount++] = oni;
			}
		}

		/* look for name matches */
		for(i=0; i<fromcount; i++)
		{
			var = getvalkey((INTBIG)fromlist[i], VNODEINST, VSTRING, el_node_name_key);
			if (var == NOVARIABLE) continue;
			for(j=0; j<tocount; j++)
			{
				if (tolist[j] == NONODEINST) continue;
				ovar = getvalkey((INTBIG)tolist[j], VNODEINST, VSTRING, el_node_name_key);
				if (ovar == NOVARIABLE) continue;
				if (namesame((CHAR *)var->addr, (CHAR *)ovar->addr) == 0) break;
			}
			if (j < tocount)
			{
				/* name match found: set the association */
				tolist[j]->temp1 = (INTBIG)fromlist[i];
				fromlist[i] = NONODEINST;
			}
		}

		/* remove the matched instances */
		j = 0;
		for(i=0; i<fromcount; i++)
			if (fromlist[i] != NONODEINST) fromlist[j++] = fromlist[i];
		fromcount = j;
		j = 0;
		for(i=0; i<tocount; i++)
			if (tolist[i]->temp1 == 0) tolist[j++] = tolist[i];
		tocount = j;
		if (fromcount != tocount)
		{
			ttyputerr(_("Error: after name match, there are %ld instances of %s in source and %ld in destination"),
				fromcount, describenodeproto(ni->proto), tocount);
			return;
		}

		/* sort the rest by position and force matches based on that */
		if (fromcount == 0) continue;
		esort(fromlist, fromcount, sizeof (NODEINST *), ro_sortinstspatially);
		esort(tolist, tocount, sizeof (NODEINST *), ro_sortinstspatially);
		for(i=0; i<tocount; i++)
			tolist[i]->temp1 = (INTBIG)fromlist[i];

		/* free the arrays */
		efree((CHAR *)fromlist);
		efree((CHAR *)tolist);
	}

	/* association made, now copy the topology */
	for(tni = tonp->firstnodeinst; tni != NONODEINST; tni = tni->nextnodeinst)
	{
		if (tni->temp1 == 0) continue;
		fni = (NODEINST *)tni->temp1;

		/* look for another node that may match */
		for(otni = tonp->firstnodeinst; otni != NONODEINST; otni = otni->nextnodeinst)
		{
			if (tni == otni) continue;
			if (otni->temp1 == 0) continue;
			ofni = (NODEINST *)otni->temp1;

			/* see if they share a connection in the original */
			ofpi = NOPORTARCINST;
			for(fpi = fni->firstportarcinst; fpi != NOPORTARCINST; fpi = fpi->nextportarcinst)
			{
				ai = fpi->conarcinst;
				for(ofpi = ofni->firstportarcinst; ofpi != NOPORTARCINST; ofpi = ofpi->nextportarcinst)
				{
					oai = ofpi->conarcinst;
					if (oai->network == ai->network) break;
				}
				if (ofpi != NOPORTARCINST) break;
			}
			if (fpi == NOPORTARCINST) continue;

			/* this connection should be repeated in the other cell */
			for(tpp = tni->proto->firstportproto; tpp != NOPORTPROTO; tpp = tpp->nextportproto)
				if (namesame(tpp->protoname, fpi->proto->protoname) == 0) break;
			if (tpp == NOPORTPROTO) continue;
			for(otpp = tni->proto->firstportproto; otpp != NOPORTPROTO; otpp = otpp->nextportproto)
				if (namesame(otpp->protoname, ofpi->proto->protoname) == 0) break;
			if (otpp == NOPORTPROTO) continue;

			/* make the connection from "tni", port "tpp" to "otni" port "otpp" */
			if (ro_makeunroutedconnection(tni, tpp, otni, otpp)) return;
		}
	}

	/* add in any exported but unconnected pins */
	for(tni = tonp->firstnodeinst; tni != NONODEINST; tni = tni->nextnodeinst)
	{
		if (tni->temp1 != 0) continue;
		if (tni->proto->primindex == 0) continue;
		if (tni->firstportexpinst == NOPORTEXPINST) continue;
		fun = nodefunction(tni);
		if (fun != NPPIN && fun != NPCONTACT) continue;

		/* find that export in the source cell */
		tpp = tni->proto->firstportproto;
		matchname = tni->firstportexpinst->exportproto->protoname;
		net = NONETWORK;
		for(fpp = ro_copiedtopocell->firstportproto; fpp != NOPORTPROTO; fpp = fpp->nextportproto)
		{
			net = fpp->network;
			if (fpp->network->buswidth > 1)
			{
				for(i=0; i<fpp->network->buswidth; i++)
				{
					net = fpp->network->networklist[i];
					if (net->namecount <= 0) continue;
					if (namesame(networkname(net, 0), matchname) == 0) break;
				}
				if (i < fpp->network->buswidth) break;
			} else
			{
				if (namesame(fpp->protoname, matchname) == 0) break;
			}
		}
		if (fpp == NOPORTPROTO) continue;

		/* check to see if this is connected elsewhere in the "to" cell */
		ofpi = NOPORTARCINST;
		for(otni = tonp->firstnodeinst; otni != NONODEINST; otni = otni->nextnodeinst)
		{
			if (otni->temp1 == 0) continue;
			ofni = (NODEINST *)otni->temp1;

			/* see if they share a connection in the original */
			for(ofpi = ofni->firstportarcinst; ofpi != NOPORTARCINST; ofpi = ofpi->nextportarcinst)
			{
				ai = ofpi->conarcinst;
				if (ai->network == net) break;
			}
			if (ofpi != NOPORTARCINST) break;
		}
		if (otni != NONODEINST)
		{
			/* find the proper port in this cell */
			for(otpp = otni->proto->firstportproto; otpp != NOPORTPROTO; otpp = otpp->nextportproto)
				if (namesame(otpp->protoname, ofpi->proto->protoname) == 0) break;
			if (otpp == NOPORTPROTO) continue;

			/* make the connection from "tni", port "tpp" to "otni" port "otpp" */
			if (ro_makeunroutedconnection(tni, tpp, otni, otpp)) return;
		}
	}
}

/*
 * Helper routine to run an unrouted wire between node "fni", port "fpp" and node "tni", port
 * "tpp".  If the connection is already there, the routine doesn't make another.
 * Returns true on error.
 */
BOOLEAN ro_makeunroutedconnection(NODEINST *fni, PORTPROTO *fpp, NODEINST *tni, PORTPROTO *tpp)
{
	REGISTER PORTARCINST *fpi, *tpi;
	REGISTER ARCINST *fai, *tai, *ai;
	INTBIG fx, fy, tx, ty;
	REGISTER INTBIG wid, bits;
	REGISTER NODEPROTO *np;

	/* see if they are already connected */
	for(fpi = fni->firstportarcinst; fpi != NOPORTARCINST; fpi = fpi->nextportarcinst)
		if (fpi->proto == fpp) break;
	for(tpi = tni->firstportarcinst; tpi != NOPORTARCINST; tpi = tpi->nextportarcinst)
		if (tpi->proto == tpp) break;
	if (fpi != NOPORTARCINST && tpi != NOPORTARCINST)
	{
		fai = fpi->conarcinst;
		tai = tpi->conarcinst;
		if (fai->network == tai->network) return(FALSE);
	}

	/* make the connection from "tni", port "tpp" to "otni" port "otpp" */
	portposition(fni, fpp, &fx, &fy);
	portposition(tni, tpp, &tx, &ty);
	bits = us_makearcuserbits(gen_unroutedarc);
	wid = defaultarcwidth(gen_unroutedarc);
	np = fni->parent;
	ai = newarcinst(gen_unroutedarc, wid, bits, fni, fpp, fx, fy, tni, tpp, tx, ty, np);
	if (ai == NOARCINST) return(TRUE);
	endobjectchange((INTBIG)ai, VARCINST);
	return(FALSE);
}

/*
 * Helper routine for sorting instances spatially
 */
int ro_sortinstspatially(const void *e1, const void *e2)
{
	REGISTER NODEINST *n1, *n2;
	REGISTER INTBIG x1, y1, x2, y2;

	n1 = *((NODEINST **)e1);
	n2 = *((NODEINST **)e2);
	x1 = (n1->lowx + n1->highx) / 2;
	y1 = (n1->lowy + n1->highy) / 2;
	x2 = (n2->lowx + n2->highx) / 2;
	y2 = (n2->lowy + n2->highy) / 2;
	if (y1 == y2) return(x1 - x2);
	return(y1 - y2);
}

/****************************** ROUTING OPTIONS DIALOG ******************************/

/* Routing Options */
static DIALOGITEM ro_optionsdialogitems[] =
{
 /*  1 */ {0, {264,264,288,328}, BUTTON, N_("OK")},
 /*  2 */ {0, {264,12,288,76}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,88,48,272}, POPUP, x_("")},
 /*  4 */ {0, {32,8,48,88}, MESSAGE, N_("Currently:")},
 /*  5 */ {0, {8,8,24,220}, MESSAGE, N_("Arc to use in stitching routers:")},
 /*  6 */ {0, {68,8,84,268}, CHECK, N_("Mimic stitching can unstitch")},
 /*  7 */ {0, {116,24,132,348}, CHECK, N_("Ports must match")},
 /*  8 */ {0, {236,8,252,268}, CHECK, N_("Mimic stitching runs interactively")},
 /*  9 */ {0, {92,8,108,268}, MESSAGE, N_("Mimic stitching restrictions:")},
 /* 10 */ {0, {188,24,204,348}, CHECK, N_("Node types must match")},
 /* 11 */ {0, {164,24,180,348}, CHECK, N_("Nodes sizes must match")},
 /* 12 */ {0, {140,24,156,348}, CHECK, N_("Number of existing arcs must match")},
 /* 13 */ {0, {212,24,228,348}, CHECK, N_("Cannot have other arcs in the same direction")}
};
static DIALOG ro_optionsdialog = {{50,75,347,433}, N_("Routing Options"), 0, 13, ro_optionsdialogitems, 0, 0};

/* special items for the "Routing Options" dialog: */
#define DROO_ARCLIST         3		/* Arc list (popup) */
#define DROO_MUNSTITCH       6		/* Mimic can unstitch (check) */
#define DROO_MPORTSPECIFIC   7		/* Mimic port-specific (check) */
#define DROO_MINTERACTIVE    8		/* Mimic is interactive (check) */
#define DROO_MNODEPSECIFIC  10		/* Mimic node-specific (check) */
#define DROO_MNODESIZESPEC  11		/* Mimic node-size-specific (check) */
#define DROO_MARCCOUNTSPEC  12		/* Mimic arc-count-specific (check) */
#define DROO_MOTHARCDIRSPEC 13		/* Mimic other-arc-in-this-direction-specific (check) */

void ro_optionsdlog(void)
{
	INTBIG itemHit, numarcnames, initialindex, i, options, oldoptions;
	REGISTER VARIABLE *var;
	REGISTER ARCPROTO *ap, **arcs, *oldap;
	CHAR **arcnames, *paramstart[3];
	REGISTER void *dia;

	/* gather list of arcs */
	var = getvalkey((INTBIG)ro_tool, VTOOL, VARCPROTO, ro_preferedkey);
	if (var == NOVARIABLE) oldap = NOARCPROTO; else
		oldap = (ARCPROTO *)var->addr;
	numarcnames = 0;
	for(ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
		numarcnames++;
	arcs = (ARCPROTO **)emalloc((numarcnames+1) * (sizeof (ARCPROTO *)), el_tempcluster);
	arcnames = (CHAR **)emalloc((numarcnames+1) * (sizeof (CHAR *)), el_tempcluster);
	if (arcs == 0 || arcnames == 0) return;
	arcs[0] = NOARCPROTO;
	arcnames[0] = x_("DEFAULT ARC");
	numarcnames = 1;
	initialindex = 0;
	for(ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
	{
		arcs[numarcnames] = ap;
		arcnames[numarcnames] = ap->protoname;
		if (ap == oldap) initialindex = numarcnames;
		numarcnames++;
	}

	/* display the router arc dialog box */
	dia = DiaInitDialog(&ro_optionsdialog);
	if (dia == 0) return;
	DiaSetPopup(dia, DROO_ARCLIST, numarcnames, arcnames);
	DiaSetPopupEntry(dia, DROO_ARCLIST, initialindex);
	efree((CHAR *)arcnames);
	ap = arcs[initialindex];
	oldoptions = options = ro_getoptions();
	if ((options&MIMICUNROUTES) != 0) DiaSetControl(dia, DROO_MUNSTITCH, 1);
	if ((options&MIMICIGNOREPORTS) == 0) DiaSetControl(dia, DROO_MPORTSPECIFIC, 1);
	if ((options&MIMICONSIDERARCCOUNT) != 0) DiaSetControl(dia, DROO_MARCCOUNTSPEC, 1);
	if ((options&MIMICIGNORENODETYPE) == 0) DiaSetControl(dia, DROO_MNODEPSECIFIC, 1);
	if ((options&MIMICOTHARCTHISDIR) == 0) DiaSetControl(dia, DROO_MOTHARCDIRSPEC, 1);
	if ((options&MIMICINTERACTIVE) != 0) DiaSetControl(dia, DROO_MINTERACTIVE, 1);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DROO_MUNSTITCH || itemHit == DROO_MPORTSPECIFIC ||
			itemHit == DROO_MINTERACTIVE || itemHit == DROO_MNODEPSECIFIC ||
			itemHit == DROO_MNODESIZESPEC || itemHit == DROO_MARCCOUNTSPEC ||
			itemHit == DROO_MOTHARCDIRSPEC)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
		if (itemHit == DROO_ARCLIST)
		{
			/* find this arc */
			i = DiaGetPopupEntry(dia, DROO_ARCLIST);
			ap = arcs[i];
		}
	}

	if (itemHit != CANCEL)
	{
		if (DiaGetControl(dia, DROO_MUNSTITCH) != 0) options |= MIMICUNROUTES; else
			options &= ~MIMICUNROUTES;
		if (DiaGetControl(dia, DROO_MPORTSPECIFIC) == 0) options |= MIMICIGNOREPORTS; else
			options &= ~MIMICIGNOREPORTS;
		if (DiaGetControl(dia, DROO_MARCCOUNTSPEC) != 0) options |= MIMICONSIDERARCCOUNT; else
			options &= ~MIMICONSIDERARCCOUNT;
		if (DiaGetControl(dia, DROO_MNODEPSECIFIC) == 0) options |= MIMICIGNORENODETYPE; else
			options &= ~MIMICIGNORENODETYPE;
		if (DiaGetControl(dia, DROO_MOTHARCDIRSPEC) == 0) options |= MIMICOTHARCTHISDIR; else
			options &= ~MIMICOTHARCTHISDIR;
		if (DiaGetControl(dia, DROO_MNODESIZESPEC) == 0) options |= MIMICIGNORENODESIZE; else
			options &= ~MIMICIGNORENODESIZE;
		if (DiaGetControl(dia, DROO_MINTERACTIVE) != 0) options |= MIMICINTERACTIVE; else
			options &= ~MIMICINTERACTIVE;
		if (options != oldoptions)
			(void)setvalkey((INTBIG)ro_tool, VTOOL, ro_optionskey, options, VINTEGER);
		paramstart[0] = x_("use-arc");
		if (ap != oldap)
		{
			if (ap == NOARCPROTO) paramstart[1] = x_("*"); else
				paramstart[1] = ap->protoname;
			telltool(ro_tool, 2, paramstart);
		}
	}
	DiaDoneDialog(dia);
	efree((CHAR *)arcs);
}

#endif  /* ROUTTOOL - at top */
