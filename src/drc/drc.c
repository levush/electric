/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: drc.c
 * Design-rule check tool
 * WRitten by: Steven M. Rubin, Static Free Software
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
#if DRCTOOL

#include "global.h"
#include "egraphics.h"
#include "drc.h"
#include "efunction.h"
#include "tech.h"
#include "tecgen.h"
#include "tecschem.h"
#include "edialogs.h"
#include "usr.h"
#include "usrdiacom.h"

/* the DRC tool table */
static COMCOMP dr_drccp = {NOKEYWORD, topofcells, nextcells, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("cell to be rechecked"), M_("check current cell")};
static KEYWORD drcnotopt[] =
{
	{x_("verbose"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("reasonable"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP drcnotp = {drcnotopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Design Rule Checker negative action"), 0};
static COMCOMP drcfip = {NOKEYWORD, topofcells, nextcells, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("cell to ignored for short check DRC"), 0};
static COMCOMP drcfup = {NOKEYWORD, topofcells, nextcells, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("cell to un-ignored for short check DRC"), 0};
static KEYWORD drcfopt[] =
{
	{x_("select-run"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("run"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("ignore"),       1,{&drcfip,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("unignore"),     1,{&drcfup,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP drcfp = {drcfopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Short Detecting Design Rule Checker action"), 0};
static KEYWORD drcnopt[] =
{
	{x_("run"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("select-run"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("reset-check-dates"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("just-1-error"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("all-errors"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP drcbatchp = {drcnopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Batch Design Rule Checker action"), 0};
static KEYWORD drcopt[] =
{
	{x_("check"),                 1,{&dr_drccp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("forget-ignored-errors"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("not"),                   1,{&drcnotp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("verbose"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("batch"),                 1,{&drcbatchp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("shortcheck"),            1,{&drcfp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("ecadcheck"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("reasonable"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("quick-check"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("quick-select-check"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP dr_drcp = {drcopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("Design Rule Checker action"), M_("show defaults")};

/* dcheck modules */
#define NODCHECK ((DCHECK *)-1)

#define NODENEW    1					/* this nodeinst was created */
#define ARCNEW     2					/* this arcinst was created */
#define NODEMOD    3					/* this nodeinst was modified */
#define ARCMOD     4					/* this arcinst was modified */
#define NODEKILL   5					/* this nodeinst was deleted */
#define ARCKILL    6					/* this arcinst was deleted */

typedef struct Idcheck
{
	NODEPROTO *cell;					/* cell containing object to be checked */
	INTBIG     entrytype;				/* type of check */
	void      *entryaddr;				/* object being checked */
	struct Idcheck *nextdcheck;			/* next in list */
} DCHECK;
static DCHECK   *dr_firstdcheck;
static DCHECK   *dr_dcheckfree;

static INTBIG    dr_verbosekey;						/* key for "DRC_verbose" */
       INTBIG    dr_ignore_listkey;					/* key for "DRC_ignore_list" */
       INTBIG    dr_lastgooddrckey;					/* key for "DRC_last_good_drc" */
static INTBIG    dr_incrementalonkey;				/* key for "DRC_incrementalon" */
static INTBIG    dr_optionskey;						/* key for "DRC_options" */
       INTBIG    dr_max_distanceskey;				/* key for "DRC_max_distances" */
       INTBIG    dr_wide_limitkey;					/* key for "DRC_wide_limit" */
       INTBIG    dr_min_widthkey;					/* key for "DRC_min_width" */
       INTBIG    dr_min_width_rulekey;				/* key for "DRC_min_width_rule" */
       INTBIG    dr_min_node_sizekey;				/* key for "DRC_min_node_size" */
       INTBIG    dr_min_node_size_rulekey;			/* key for "DRC_min_node_size_rule" */
       INTBIG    dr_connected_distanceskey;			/* key for "DRC_min_connected_distances" */
       INTBIG    dr_connected_distances_rulekey;	/* key for "DRC_min_connected_distances_rule" */
       INTBIG    dr_unconnected_distanceskey;		/* key for "DRC_min_unconnected_distances" */
       INTBIG    dr_unconnected_distances_rulekey;	/* key for "DRC_min_unconnected_distances_rule" */
       INTBIG    dr_connected_distancesWkey;		/* key for "DRC_min_connected_distances_wide" */
       INTBIG    dr_connected_distancesW_rulekey;	/* key for "DRC_min_connected_distances_wide_rule" */
       INTBIG    dr_unconnected_distancesWkey;		/* key for "DRC_min_unconnected_distances_wide" */
       INTBIG    dr_unconnected_distancesW_rulekey;	/* key for "DRC_min_unconnected_distances_wide_rule" */
       INTBIG    dr_connected_distancesMkey;		/* key for "DRC_min_connected_distances_multi" */
       INTBIG    dr_connected_distancesM_rulekey;	/* key for "DRC_min_connected_distances_multi_rule" */
       INTBIG    dr_unconnected_distancesMkey;		/* key for "DRC_min_unconnected_distances_multi" */
       INTBIG    dr_unconnected_distancesM_rulekey;	/* key for "DRC_min_unconnected_distances_multi_rule" */
       INTBIG    dr_edge_distanceskey;				/* key for "DRC_min_edge_distances" */
       INTBIG    dr_edge_distances_rulekey;			/* key for "DRC_min_edge_distances_rule" */
#ifdef SURROUNDRULES
       INTBIG    dr_surround_layer_pairskey;		/* key for "DRC_surround_layer_pairs" */
       INTBIG    dr_surround_distanceskey;			/* key for "DRC_surround_distances" */
	   INTBIG    dr_surround_rulekey;				/* key for "DRC_surround_rule" */
#endif
       BOOLEAN   dr_logerrors;						/* TRUE to log errors in error reporting system */
       TOOL     *dr_tool;							/* the DRC tool object */

CHAR *dr_variablenames[] =
{
	x_("DRC_max_distances"),
	x_("DRC_wide_limit"),
	x_("DRC_min_width"),
	x_("DRC_min_width_rule"),
	x_("DRC_min_node_size"),
	x_("DRC_min_node_size_rule"),
	x_("DRC_min_connected_distances"),
	x_("DRC_min_connected_distances_rule"),
	x_("DRC_min_unconnected_distances"),
	x_("DRC_min_unconnected_distances_rule"),
	x_("DRC_min_connected_distances_wide"),
	x_("DRC_min_connected_distances_wide_rule"),
	x_("DRC_min_unconnected_distances_wide"),
	x_("DRC_min_unconnected_distances_wide_rule"),
	x_("DRC_min_connected_distances_multi"),
	x_("DRC_min_connected_distances_multi_rule"),
	x_("DRC_min_unconnected_distances_multi"),
	x_("DRC_min_unconnected_distances_multi_rule"),
	x_("DRC_min_edge_distances"),
	x_("DRC_min_edge_distances_rule"),
	0
};

/* prototypes for local routines */
static DCHECK    *dr_allocdcheck(void);
static NODEPROTO *dr_checkthiscell(INTBIG, CHAR*);
static void       dr_freedcheck(DCHECK*);
static void       dr_optionsdlog(void);
static void       dr_rulesdloghook(void);
static void       dr_queuecheck(void*, NODEPROTO*, INTBIG);
static void       dr_unignore(NODEPROTO*, BOOLEAN, void*);
static void       dr_unqueuecheck(LIBRARY *lib);
static INTBIG     dr_examinecell(NODEPROTO *np, BOOLEAN report);
static void       dr_schdocheck(GEOM *geom);
static void       dr_checkentirecell(NODEPROTO *cell, INTBIG count, NODEINST **nodestocheck,
					BOOLEAN *validity, BOOLEAN justarea);
static BOOLEAN    dr_schcheckcolinear(ARCINST *ai, ARCINST *oai);
static void       dr_schcheckobjectvicinity(GEOM *topgeom, GEOM *geom, XARRAY trans);
static void       dr_checkschematiccell(NODEPROTO *cell, BOOLEAN justthis);
static void       dr_checkschematiccellrecursively(NODEPROTO *cell);
static BOOLEAN    dr_schcheckpoly(GEOM *geom, POLYGON *poly, GEOM *otopgeom, GEOM *ogeom, XARRAY otrans, BOOLEAN cancross);
static BOOLEAN    dr_schcheckpolygonvicinity(GEOM *geom, POLYGON *poly);
static BOOLEAN    dr_checkpolyagainstpoly(GEOM *geom, POLYGON *poly, GEOM *ogeom, POLYGON *opoly, BOOLEAN cancross);

/******************** CONTROL ********************/

void dr_init(INTBIG *argc, CHAR1 *argv[], TOOL *thistool)
{
	/* only initialize during pass 1 */
	if (thistool == NOTOOL || thistool == 0) return;

	dr_tool = thistool;

	/* initialize the design-rule checker lists */
	dr_dcheckfree = NODCHECK;
	dr_firstdcheck = NODCHECK;
	dr_logerrors = TRUE;

	dr_verbosekey = makekey(x_("DRC_verbose"));
	dr_ignore_listkey = makekey(x_("DRC_ignore_list"));
	dr_lastgooddrckey = makekey(x_("DRC_last_good_drc"));
	dr_incrementalonkey = makekey(x_("DRC_incrementalon"));
	dr_optionskey = makekey(x_("DRC_options"));
	DiaDeclareHook(x_("drcopt"), &dr_drcp, dr_optionsdlog);
	DiaDeclareHook(x_("drcrules"), &dr_drccp, dr_rulesdloghook);
}

void dr_done(void)
{
#ifdef DEBUGMEMORY
	REGISTER DCHECK *d;

	while (dr_firstdcheck != NODCHECK)
	{
		d = dr_firstdcheck;
		dr_firstdcheck = dr_firstdcheck->nextdcheck;
		dr_freedcheck(d);
	}
	while (dr_dcheckfree != NODCHECK)
	{
		d = dr_dcheckfree;
		dr_dcheckfree = dr_dcheckfree->nextdcheck;
		efree((CHAR *)d);
	}
	drcb_term();
	dr_quickterm();
#endif
}

void dr_set(INTBIG count, CHAR *par[])
{
	REGISTER BOOLEAN negate;
	REGISTER INTBIG l, i, errs, options;
	REGISTER CHAR *pp;
	REGISTER FILE *f;
	REGISTER NODEPROTO *np;
	REGISTER VARIABLE *var;
	REGISTER TECHNOLOGY *tech;
	CHAR *newpar[3], *truename, *abortseq;
	static INTBIG filetypedrac = -1;
	REGISTER void *infstr;

	if (count == 0)
	{
		var = getvalkey((INTBIG)dr_tool, VTOOL, VINTEGER, dr_verbosekey);
		if (var == NOVARIABLE) i = 0; else i = var->addr;
		if (i == 0) ttyputmsg(M_("Design rule checker is brief")); else
 			ttyputmsg(M_("Design rule checker is verbose"));
		return;
	}

	options = dr_getoptionsvalue();
	l = estrlen(pp = par[0]);
	negate = FALSE;
	if (namesamen(pp, x_("not"), l) == 0)
	{
		negate = TRUE;
		if (count <= 1)
		{
			count = ttygetparam(M_("DRC negate option:"), &drcnotp, MAXPARS-1, &par[1]) + 1;
			if (count <= 1)
			{
				ttyputerr(M_("Aborted"));
				return;
			}
		}
		l = estrlen(pp = par[1]);
	}

	if (namesamen(pp, x_("forget-ignored-errors"), l) == 0)
	{
		np = dr_checkthiscell(count, par[1]);
		if (np == NONODEPROTO) return;

		/* erase all ignore lists on all objects */
		var = getvalkey((INTBIG)np, VNODEPROTO, VGEOM|VISARRAY, dr_ignore_listkey);
		if (var != NOVARIABLE)
			(void)delvalkey((INTBIG)np, VNODEPROTO, dr_ignore_listkey);

		ttyputmsg(M_("Ignored violations turned back on."));
		return;
	}
	if (namesamen(pp, x_("check"), l) == 0)
	{
		/* make sure network tool is on */
		if ((net_tool->toolstate&TOOLON) == 0)
		{
			ttyputerr(M_("Network tool must be running...turning it on"));
			toolturnon(net_tool);
			ttyputerr(M_("...now reissue the DRC command"));
			return;
		}

		np = dr_checkthiscell(count, par[1]);
		if (np == NONODEPROTO) return;
		errs = dr_examinecell(np, TRUE);
		return;
	}

	if (namesamen(pp, x_("ecadcheck"), l) == 0)
	{
		np = getcurcell();
		if (np == NONODEPROTO)
		{
			ttyputerr(_("Must be editing a cell to check with ECAD's Dracula"));
			return;
		}
		tech = np->tech;
		var = getval((INTBIG)tech, VTECHNOLOGY, VSTRING|VISARRAY, x_("DRC_ecad_deck"));
		if (var == NOVARIABLE)
		{
			ttyputerr(_("Cannot find an ECAD deck in the %s technology"), tech->techname);
			return;
		}

		/* create the file */
		if (filetypedrac < 0)
			filetypedrac = setupfiletype(x_("rul"), x_("*.map"), MACFSTAG('TEXT'), FALSE, x_("drac"), _("Dracula"));
		f = xcreate(x_("ecaddrc.RUL"), filetypedrac, _("ECAD DRC Control File"), &truename);
		if (f == NULL)
		{
			if (truename != 0) ttyputerr(_("Cannot write %s"), truename);
			return;
		}

		/* write the deck */
		l = getlength(var);
		for(i=0; i<l; i++)
		{
			pp = ((CHAR **)var->addr)[i];
			if (estrncmp(pp, x_(" PRIMARY ="), 10) == 0)
			{
				xprintf(f, x_(" PRIMARY = %s\n"), np->protoname);
				continue;
			}
			if (estrncmp(pp, x_(" INDISK ="), 9) == 0)
			{
				xprintf(f, x_(" INDISK = %s.cif\n"), np->protoname);
				continue;
			}
			xprintf(f, x_("%s\n"), pp);
		}

		/* finished with control deck */
		xclose(f);
		ttyputverbose(M_("Wrote 'ecaddrc.RUL'.  Now generating CIF for cell %s"), describenodeproto(np));

		/* tell I/O to write CIF */
		newpar[0] = x_("cif");
		newpar[1] = x_("output");
		newpar[2] = x_("include-cloak-layer");
		telltool(io_tool, 3, newpar);
		(void)asktool(io_tool, x_("write"), (INTBIG)np->lib, (INTBIG)x_("cif"));
		newpar[0] = x_("cif");
		newpar[1] = x_("output");
		newpar[2] = x_("ignore-cloak-layer");
		telltool(io_tool, 3, newpar);
		return;
	}

	if (namesamen(pp, x_("batch"), l) == 0)
	{
		if (count == 1)
		{
			ttyputusage(x_("telltool drc batch OPTIONS"));
			return;
		}
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("run"), l) == 0)
		{
			/* make sure network tool is on */
			if ((net_tool->toolstate&TOOLON) == 0)
			{
				ttyputerr(M_("Network tool must be running...turning it on"));
				toolturnon(net_tool);
				ttyputerr(M_("...now reissue the DRC command"));
				return;
			}

			/* make sure there is a cell to check */
			np = getcurcell();
			if (np == NONODEPROTO)
			{
				ttyputerr(_("No current cell"));
				return;
			}

			/* see if a dialog should be displayed */
			infstr = initinfstr();
			addstringtoinfstr(infstr, _("Checking hierarchically..."));
			abortseq = getinterruptkey();
			if (abortseq != 0)
				formatinfstr(infstr, _(" (type %s to abort)"), abortseq);
			ttyputmsg(returninfstr(infstr));
			(void)drcb_check(np, TRUE, FALSE);
			return;
		}
		if (namesamen(pp, x_("select-run"), l) == 0)
		{
			/* make sure network tool is on */
			if ((net_tool->toolstate&TOOLON) == 0)
			{
				ttyputerr(M_("Network tool must be running...turning it on"));
				toolturnon(net_tool);
				ttyputerr(M_("...now reissue the DRC command"));
				return;
			}

			/* make sure there is a cell to check */
			np = getcurcell();
			if (np == NONODEPROTO)
			{
				ttyputerr(_("No current cell"));
				return;
			}

			/* see if a dialog should be displayed */
			infstr = initinfstr();
			addstringtoinfstr(infstr, _("Checking selected area hierarchically..."));
			abortseq = getinterruptkey();
			if (abortseq != 0)
				formatinfstr(infstr, _(" (type %s to abort)"), abortseq);
			ttyputmsg(returninfstr(infstr));
			(void)drcb_check(np, TRUE, TRUE);
			return;
		}
		if (namesamen(pp, x_("just-1-error"), l) == 0)
		{
			(void)setvalkey((INTBIG)dr_tool, VTOOL, dr_optionskey,
				options | DRCFIRSTERROR, VINTEGER);
			ttyputmsg(M_("Hierarchical DRC will stop after first error in a cell"));
			return;
		}
		if (namesamen(pp, x_("all-errors"), l) == 0)
		{
			(void)setvalkey((INTBIG)dr_tool, VTOOL, dr_optionskey,
				options & ~DRCFIRSTERROR, VINTEGER);
			ttyputmsg(M_("Hierarchical DRC will find all errors in a cell"));
			return;
		}
		if (namesamen(pp, x_("reset-check-dates"), l) == 0)
		{
			dr_reset_dates();
			ttyputmsg(M_("All date information about valid DRC is reset"));
			return;
		}
		ttyputbadusage(x_("telltool drc batch"));
		return;
	}

	if (namesamen(pp, x_("quick-check"), l) == 0 && l >= 7)
	{
		/* get the current cell */
		np = getcurcell();
		if (np == NONODEPROTO)
		{
			ttyputerr(_("No current cell"));
			return;
		}
		dr_checkentirecell(np, 0, 0, 0, FALSE);
		return;
	}

	if (namesamen(pp, x_("quick-select-check"), l) == 0 && l >= 7)
	{
		/* get the current cell */
		np = getcurcell();
		if (np == NONODEPROTO)
		{
			ttyputerr(_("No current cell"));
			return;
		}
		dr_checkentirecell(np, 0, 0, 0, TRUE);
		return;
	}

	if (namesamen(pp, x_("shortcheck"), l) == 0)
	{
		if (count < 2)
		{
			ttyputusage(x_("telltool drc shortcheck (run|ignore|unignore)"));
			return;
		}
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("run"), l) == 0)
		{
			/* make sure network tool is on */
			if ((net_tool->toolstate&TOOLON) == 0)
			{
				ttyputerr(M_("Network tool must be running...turning it on"));
				toolturnon(net_tool);
				ttyputerr(M_("...now reissue the DRC command"));
				return;
			}
			dr_flatwrite(getcurcell());
			return;
		}
		if (namesamen(pp, x_("ignore"), l) == 0)
		{
			if (count < 3)
			{
				ttyputusage(x_("telltool drc shortcheck ignore CELL"));
				return;
			}
			dr_flatignore(par[2]);
			return;
		}
		if (namesamen(pp, x_("unignore"), l) == 0)
		{
			if (count < 3)
			{
				ttyputusage(x_("telltool drc shortcheck unignore CELL"));
				return;
			}
			dr_flatunignore(par[2]);
			return;
		}
		ttyputbadusage(x_("telltool drc shortcheck"));
		return;
	}

	if (namesamen(pp, x_("verbose"), l) == 0)
	{
		if (!negate)
		{
			(void)setvalkey((INTBIG)dr_tool, VTOOL, dr_verbosekey, 1, VINTEGER|VDONTSAVE);
			ttyputverbose(M_("DRC verbose"));
		} else
		{
			if (getvalkey((INTBIG)dr_tool, VTOOL, VINTEGER, dr_verbosekey) != NOVARIABLE)
				(void)delvalkey((INTBIG)dr_tool, VTOOL, dr_verbosekey);
			ttyputverbose(M_("DRC brief"));
		}
		return;
	}

	if (namesamen(pp, x_("reasonable"), l) == 0)
	{
		if (!negate)
		{
			(void)setvalkey((INTBIG)dr_tool, VTOOL, dr_optionskey,
				options | DRCREASONABLE, VINTEGER);
			ttyputverbose(M_("DRC will consider only reasonable number of polygons"));
		} else
		{
			(void)setvalkey((INTBIG)dr_tool, VTOOL, dr_optionskey,
				options & ~DRCREASONABLE, VINTEGER);
			ttyputverbose(M_("DRC will consider all polygons"));
		}
		return;
	}
	ttyputbadusage(x_("telltool drc"));
}

/*
 * make request of the DRC tool:
 * "check-instances" TAKES: CELL, COUNT, NODEINST array, BOOLEAN validity-array
 */
INTBIG dr_request(CHAR *command, va_list ap)
{
	REGISTER NODEPROTO *cell;
	REGISTER NODEINST **nodelist;
	REGISTER BOOLEAN *validity;
	REGISTER INTBIG arg1, arg2, arg3, arg4, count;

	if (namesame(command, x_("check-instances")) == 0)
	{
		/* get the arguments */
		arg1 = va_arg(ap, INTBIG);
		arg2 = va_arg(ap, INTBIG);
		arg3 = va_arg(ap, INTBIG);
		arg4 = va_arg(ap, INTBIG);

		cell = (NODEPROTO *)arg1;
		count = arg2;
		nodelist = (NODEINST **)arg3;
		validity = (BOOLEAN *)arg4;
		dr_checkentirecell(cell, count, nodelist, validity, FALSE);
		return(0);
	}
	return(0);
}

void dr_examinenodeproto(NODEPROTO *np)
{
	REGISTER INTBIG i, errs;
	REGISTER CHAR *pt;
	if (dr_examinecell(np, FALSE) != 0)
	{
		errs = numerrors();
		for(i=0; i<errs; i++)
		{
			pt = reportnexterror(0, 0, 0);
			ttyputmsg(x_("%s"), pt);
		}

		/* reset error pointers to start of list */
		termerrorlogging(FALSE);
	}
}

/*
 * Routine to examine cell "np" nonhierarchically and return the number of errors found.
 */
INTBIG dr_examinecell(NODEPROTO *np, BOOLEAN report)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER INTBIG errorcount;

	if (stopping(STOPREASONDRC)) return(0);

	/* do not check things in hidden libraries (such as the clipboard) */
	if ((np->lib->userbits&HIDDENLIBRARY) != 0) return(0);

	/* make sure network tool is on */
	if ((net_tool->toolstate&TOOLON) == 0) return(0);

	/* handle schematics separately */
	if (np->cellview == el_schematicview || np->tech == sch_tech)
	{
		dr_checkschematiccell(np, TRUE);
		return(0);
	}

	/* clear errors */
	initerrorlogging(_("DRC"));

	drcb_initincrementalcheck(np);
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		(void)drcb_checkincremental(ni->geom, TRUE);
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		(void)drcb_checkincremental(ai->geom, TRUE);
	errorcount = numerrors();
	if (report)
		ttyputmsg(_("Cell %s checked at this level only (%ld errors)"),
			describenodeproto(np), errorcount);
	termerrorlogging(TRUE);
	return(errorcount);
}

void dr_slice(void)
{
	REGISTER DCHECK *d, *nextd;
	REGISTER NODEPROTO *np;
	REGISTER BOOLEAN didcheck;
	REGISTER VARIABLE *var;

	/* see if the tool should be off */
	var = getvalkey((INTBIG)dr_tool, VTOOL, VINTEGER, dr_incrementalonkey);
	if (var != NOVARIABLE && var->addr == 0)
	{
		toolturnoff(dr_tool, FALSE);
		return;
	}

	if (dr_firstdcheck == NODCHECK) return;

	/* make sure network tool is on */
	if ((net_tool->toolstate&TOOLON) == 0)
	{
		ttyputerr(M_("Network tool must be running...turning it on"));
		toolturnon(net_tool);
		return;
	}

	/* mark this activity */
	setactivity(_("DRC"));

	/* first clear the ignore information on any objects that changed */
	for(d = dr_firstdcheck; d != NODCHECK; d = d->nextdcheck)
	{
		if (d->entrytype == NODEMOD || d->entrytype == NODEKILL)
			dr_unignore(d->cell, TRUE, d->entryaddr);
		if (d->entrytype == ARCMOD || d->entrytype == ARCKILL)
			dr_unignore(d->cell, FALSE, d->entryaddr);
	}

	/* clear errors */
	dr_logerrors = FALSE;

	np = NONODEPROTO;
	didcheck = FALSE;
	for(d = dr_firstdcheck; d != NODCHECK; d = nextd)
	{
		nextd = d->nextdcheck;
		if (!stopping(STOPREASONDRC))
		{
			if (d->entrytype == NODEMOD || d->entrytype == NODENEW)
			{
				NODEINST *ni;

				ni = (NODEINST *)d->entryaddr;
				if ((ni->userbits&DEADN) == 0)
				{
					if (d->cell != np)
					{
						np = ni->parent;
						drcb_initincrementalcheck(np);
						didcheck = TRUE;
					}
					(void)drcb_checkincremental(ni->geom, FALSE);
				}
			}
			if (d->entrytype == ARCMOD || d->entrytype == ARCNEW)
			{
				ARCINST *ai;

				ai = (ARCINST *)d->entryaddr;
				if ((ai->userbits&DEADA) == 0)
				{
					if (d->cell != np)
					{
						np = ai->parent;
						drcb_initincrementalcheck(np);
						didcheck = TRUE;
					}
					(void)drcb_checkincremental(ai->geom, FALSE);
				}
			}
		}
		dr_freedcheck(d);
	}
	dr_firstdcheck = NODCHECK;
	dr_logerrors = TRUE;
}

/******************** DATABASE CHANGES ********************/

void dr_modifynodeinst(NODEINST *ni,INTBIG oldlx,INTBIG oldly,INTBIG oldhx,INTBIG oldhy,
	INTBIG oldrot,INTBIG oldtran)
{
	dr_queuecheck(ni, ni->parent, NODEMOD);
}

void dr_modifyarcinst(ARCINST *ai,INTBIG oldxA, INTBIG oldyA, INTBIG oldxB, INTBIG oldyB,
	INTBIG oldwid, INTBIG oldlen)
{
	dr_queuecheck(ai, ai->parent, ARCMOD);
}

void dr_newobject(INTBIG addr, INTBIG type)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;

	if ((type&VTYPE) == VNODEINST)
	{
		ni = (NODEINST *)addr;
		dr_queuecheck(ni, ni->parent, NODENEW);
	} else if ((type&VTYPE) == VARCINST)
	{
		ai = (ARCINST *)addr;
		dr_queuecheck(ai, ai->parent, ARCNEW);
	}
}

void dr_killobject(INTBIG addr, INTBIG type)
{
	REGISTER DCHECK *d, *lastd, *nextd;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;

	/* remove any others in the list that point to this deleted object */
	lastd = NODCHECK;
	for(d = dr_firstdcheck; d != NODCHECK; d = nextd)
	{
		nextd = d->nextdcheck;
		if (d->entryaddr == (void *)addr)
		{
			if (lastd == NODCHECK) dr_firstdcheck = nextd; else
				lastd->nextdcheck = nextd;
			dr_freedcheck(d);
			continue;
		}
		lastd = d;
	}

	if ((type&VTYPE) == VNODEINST)
	{
		ni = (NODEINST *)addr;
		dr_queuecheck(ni, ni->parent, NODEKILL);
	} else if ((type&VTYPE) == VARCINST)
	{
		ai = (ARCINST *)addr;
		dr_queuecheck(ai, ai->parent, ARCKILL);
	}
}

void dr_eraselibrary(LIBRARY *lib)
{
	dr_unqueuecheck(lib);
}

void dr_newvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG newtype)
{
	if (type == VTECHNOLOGY)
	{
		if (key == el_techstate_key)
		{
			/* code cannot be called by multiple procesors: uses globals */
			NOT_REENTRANT;

			dr_curtech = NOTECHNOLOGY;
		}
	}
}

/******************** SUPPORT ROUTINES ********************/

/*
 * routine to allocate a new dcheck module from the pool (if any) or memory
 */
DCHECK *dr_allocdcheck(void)
{
	REGISTER DCHECK *d;

	if (dr_dcheckfree == NODCHECK)
	{
		d = (DCHECK *)emalloc(sizeof (DCHECK), dr_tool->cluster);
		if (d == 0) return(NODCHECK);
	} else
	{
		/* take module from free list */
		d = dr_dcheckfree;
		dr_dcheckfree = (DCHECK *)d->nextdcheck;
	}
	return(d);
}

/*
 * routine to return dcheck module "d" to the pool of free modules
 */
void dr_freedcheck(DCHECK *d)
{
	d->nextdcheck = dr_dcheckfree;
	dr_dcheckfree = d;
}

void dr_queuecheck(void *addr, NODEPROTO *cell, INTBIG function)
{
	REGISTER DCHECK *d;

	/* do not examine anything in a hidden library (such as the clipboard) */
	if ((cell->lib->userbits&HIDDENLIBRARY) != 0) return;

	d = dr_allocdcheck();
	if (d == NODCHECK)
	{
		ttyputnomemory();
		return;
	}
	d->entrytype = function;
	d->cell = cell;
	d->entryaddr = addr;
	d->nextdcheck = dr_firstdcheck;
	dr_firstdcheck = d;
}

/*
 * Routine to remove all check objects that are in library "lib"
 * (because the library has been deleted)
 */
void dr_unqueuecheck(LIBRARY *lib)
{
	REGISTER DCHECK *d, *nextd, *lastd;

	lastd = NODCHECK;
	for(d = dr_firstdcheck; d != NODCHECK; d = nextd)
	{
		nextd = d->nextdcheck;
		if (d->cell == NONODEPROTO || d->cell->lib == lib)
		{
			if (lastd == NODCHECK) dr_firstdcheck = nextd; else
				lastd->nextdcheck = nextd;
			dr_freedcheck(d);
			continue;
		}
		lastd = d;
	}
}

/*
 * routine to delete DRC ignore information on object "pos"
 */
void dr_unignore(NODEPROTO *np, BOOLEAN objisnode, void *addr)
{
	REGISTER GEOM **ignorelist, *p1, *p2, *geom;
	REGISTER VARIABLE *var;
	REGISTER INTBIG i, len, pt;

	var = getvalkey((INTBIG)np, VNODEPROTO, VGEOM|VISARRAY, dr_ignore_listkey);

	/* if the list is empty there is nothing to do */
	if (var == NOVARIABLE) return;

	/* see if this entry is mentioned in the list */
	len = getlength(var);
	for(i=0; i<len; i++)
	{
		geom = ((GEOM **)var->addr)[i];
		if (geom->entryisnode == objisnode && geom->entryaddr.blind == addr) break;
	}
	if (i >= len) return;

	/* create a new list without the entry */
	ignorelist = (GEOM **)emalloc((len * (sizeof (GEOM *))), el_tempcluster);
	if (ignorelist == 0)
	{
		ttyputnomemory();
		return;
	}

	/* search list and remove entries that mention this module */
	pt = 0;
	for(i=0; i<len; i += 2)
	{
		p1 = ((GEOM **)var->addr)[i];
		p2 = ((GEOM **)var->addr)[i+1];
		if (p1->entryisnode == objisnode && p1->entryaddr.blind == addr) continue;
		if (p2->entryisnode == objisnode && p2->entryaddr.blind == addr) continue;
		ignorelist[pt++] = p1;   ignorelist[pt++] = p2;
	}

	/* set the list back in place */
	if (pt > 0) (void)setvalkey((INTBIG)np, VNODEPROTO, dr_ignore_listkey,
		(INTBIG)ignorelist, (INTBIG)(VGEOM|VISARRAY|(pt<<VLENGTHSH)|VDONTSAVE)); else
			(void)delvalkey((INTBIG)np, VNODEPROTO, dr_ignore_listkey);
	efree((CHAR *)ignorelist);
}

/*
 * routine to determine the node prototype specified in "name" (given that
 * there were "count" parameters to the command and that "name" should be in
 * the second).  Returns NONODEPROTO if there is an error.
 */
NODEPROTO *dr_checkthiscell(INTBIG count, CHAR *name)
{
	REGISTER NODEPROTO *np;

	if (count < 2)
	{
		np = getcurcell();
		if (np == NONODEPROTO) ttyputerr(_("No current cell"));
		return(np);
	}
	np = getnodeproto(name);
	if (np == NONODEPROTO)
	{
		ttyputerr(_("No cell named %s"), name);
		return(NONODEPROTO);
	}
	if (np->primindex != 0)
	{
		ttyputerr(_("Can only check cells, not primitives"));
		return(NONODEPROTO);
	}
	return(np);
}

/*
 * Routine to obtain the current value of the DRC options.
 * Note that this cannot be automatically maintained by the "newvariable()" hook
 * because the DRC isn't always on.
 */
INTBIG dr_getoptionsvalue(void)
{
	REGISTER VARIABLE *var;

	var = getvalkey((INTBIG)dr_tool, VTOOL, VINTEGER, dr_optionskey);
	if (var == NOVARIABLE) return(0);
	return(var->addr);
}

/*
 * Turns off all saved date information about valid DRC.
 */
void dr_reset_dates(void)
{
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;
	REGISTER VARIABLE *var;

	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER, dr_lastgooddrckey);
			if (var == NOVARIABLE) continue;
			nextchangequiet();
			(void)delvalkey((INTBIG)np, VNODEPROTO, dr_lastgooddrckey);
		}
	}
}

/*
 * Routine to check all of cell "cell".  Returns number of errors found.
 */
void dr_checkentirecell(NODEPROTO *cell, INTBIG count, NODEINST **nodestocheck, BOOLEAN *validity,
	BOOLEAN justarea)
{
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;
	REGISTER INTBIG errorcount;

	if (cell->cellview == el_schematicview || cell->tech == sch_tech)
	{
		/* hierarchical check of schematics */
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				np->temp1 = 0;
		initerrorlogging(_("Schematic DRC"));
		dr_checkschematiccellrecursively(cell);
		errorcount = numerrors();
		if (errorcount != 0) ttyputmsg(_("TOTAL OF %ld ERRORS FOUND"), errorcount);
		termerrorlogging(TRUE);
		return;
	}
	dr_quickcheck(cell, count, nodestocheck, validity, justarea);
}

/****************************** SCHEMATIC DRC ******************************/

void dr_checkschematiccellrecursively(NODEPROTO *cell)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *cnp;

	cell->temp1 = 1;
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex != 0) continue;
		cnp = contentsview(ni->proto);
		if (cnp == NONODEPROTO) cnp = ni->proto;
		if (cnp->temp1 != 0) continue;

		/* ignore documentation icon */
		if (isiconof(ni->proto, cell)) continue;

		dr_checkschematiccellrecursively(cnp);
	}

	/* now check this cell */
	ttyputmsg(_("Checking schematic cell %s"), describenodeproto(cell));
	dr_checkschematiccell(cell, FALSE);
}

void dr_checkschematiccell(NODEPROTO *cell, BOOLEAN justthis)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER INTBIG errorcount, initialerrorcount, thiserrors;
	REGISTER CHAR *indent;

	if (justthis) initerrorlogging(_("Schematic DRC"));
	initialerrorcount = numerrors();
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		dr_schdocheck(ni->geom);
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		dr_schdocheck(ai->geom);
	errorcount = numerrors();
	thiserrors = errorcount - initialerrorcount;
	if (justthis) indent = x_(""); else
		indent = x_("   ");
	if (thiserrors == 0) ttyputmsg(_("%sNo errors found"), indent); else
		ttyputmsg(_("%s%ld errors found"), indent, thiserrors);
	if (justthis) termerrorlogging(TRUE);
}

/*
 * Routine to check schematic object "geom".
 */
void dr_schdocheck(GEOM *geom)
{
	REGISTER NODEPROTO *cell, *np;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	ARCINST **thearcs;
	REGISTER PORTARCINST *pi;
	REGISTER PORTPROTO *pp;
	REGISTER VARIABLE *var, *fvar;
	REGISTER BOOLEAN checkdangle;
	REGISTER INTBIG i, j, fun, signals, portwidth, nodesize;
	INTBIG x, y;
	void *err, *infstr;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	cell = geomparent(geom);
	if (geom->entryisnode)
	{
		ni = geom->entryaddr.ni;

		/* check for bus pins that don't connect to any bus arcs */
		if (ni->proto == sch_buspinprim)
		{
			/* proceed only if it has no exports on it */
			if (ni->firstportexpinst == NOPORTEXPINST)
			{
				/* must not connect to any bus arcs */
				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
					if (pi->conarcinst->proto == sch_busarc) break;
				if (pi == NOPORTARCINST)
				{
					err = logerror(_("Bus pin does not connect to any bus arcs"), cell, 0);
					addgeomtoerror(err, geom, TRUE, 0, 0);
					return;
				}
			}

			/* flag bus pin if more than 1 wire is connected */
			i = 0;
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				if (pi->conarcinst->proto == sch_wirearc) i++;
			if (i > 1)
			{
				err = logerror(_("Wire arcs cannot connect through a bus pin"), cell, 0);
				addgeomtoerror(err, geom, TRUE, 0, 0);
				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
					if (pi->conarcinst->proto == sch_wirearc)
						addgeomtoerror(err, pi->conarcinst->geom, TRUE, 0, 0);
				return;
			}
		}

		/* check all pins */
		if ((ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH == NPPIN)
		{
			/* may be stranded if there are no exports or arcs */
			if (ni->firstportexpinst == NOPORTEXPINST && ni->firstportarcinst == NOPORTARCINST)
			{
				/* see if the pin has displayable variables on it */
				for(i=0; i<ni->numvar; i++)
				{
					var = &ni->firstvar[i];
					if ((var->type&VDISPLAY) != 0) break;
				}
				if (i >= ni->numvar)
				{
					err = logerror(_("Stranded pin (not connected or exported)"), cell, 0);
					addgeomtoerror(err, geom, TRUE, 0, 0);
					return;
				}
			}

			if (isinlinepin(ni, &thearcs))
			{
				err = logerror(_("Unnecessary pin (between 2 arcs)"), cell, 0);
				addgeomtoerror(err, geom, TRUE, 0, 0);
				return;
			}

			if (invisiblepinwithoffsettext(ni, &x, &y, FALSE))
			{
				err = logerror(_("Invisible pin has text in different location"), cell, 0);
				addgeomtoerror(err, geom, TRUE, 0, 0);
				addlinetoerror(err, (ni->lowx+ni->highx)/2, (ni->lowy+ni->highy)/2, x, y);
				return;
			}
		}

		/* check parameters */
		if (ni->proto->primindex == 0)
		{
			np = contentsview(ni->proto);
			if (np == NONODEPROTO) np = ni->proto;

			/* ensure that this node matches the parameter list */
			for(i=0; i<ni->numvar; i++)
			{
				var = &ni->firstvar[i];
				if (TDGETISPARAM(var->textdescript) == 0) continue;
				fvar = NOVARIABLE;
				for(j=0; j<np->numvar; j++)
				{
					fvar = &np->firstvar[j];
					if (TDGETISPARAM(fvar->textdescript) == 0) continue;
					if (namesame(makename(var->key), makename(fvar->key)) == 0) break;
				}
				if (j >= np->numvar)
				{
					/* this node's parameter is no longer on the cell: delete from instance */
					infstr = initinfstr();
					formatinfstr(infstr, _("Parameter '%s' on node %s is invalid and has been deleted"),
						truevariablename(var), describenodeinst(ni));
					err = logerror(returninfstr(infstr), cell, 0);
					addgeomtoerror(err, geom, TRUE, 0, 0);
					startobjectchange((INTBIG)ni, VNODEINST);
					(void)delvalkey((INTBIG)ni, VNODEINST, var->key);
					endobjectchange((INTBIG)ni, VNODEINST);
					i--;
				} else
				{
					/* this node's parameter is still on the cell: make sure units are OK */
					if (TDGETUNITS(var->textdescript) != TDGETUNITS(fvar->textdescript))
					{
						infstr = initinfstr();
						formatinfstr(infstr, _("Parameter '%s' on node %s had incorrect units (now fixed)"),
							truevariablename(var), describenodeinst(ni));
						err = logerror(returninfstr(infstr), cell, 0);
						addgeomtoerror(err, geom, TRUE, 0, 0);
						startobjectchange((INTBIG)ni, VNODEINST);
						TDCOPY(descript, var->textdescript);
						TDSETUNITS(descript, TDGETUNITS(fvar->textdescript));
						modifydescript((INTBIG)ni, VNODEINST, var, descript);
						endobjectchange((INTBIG)ni, VNODEINST);
					}

					/* make sure visibility is OK */
					if (TDGETINTERIOR(fvar->textdescript) != 0)
					{
						if ((var->type&VDISPLAY) != 0)
						{
							infstr = initinfstr();
							formatinfstr(infstr, _("Parameter '%s' on node %s should not be visible (now fixed)"),
								truevariablename(var), describenodeinst(ni));
							err = logerror(returninfstr(infstr), cell, 0);
							addgeomtoerror(err, geom, TRUE, 0, 0);
							startobjectchange((INTBIG)ni, VNODEINST);
							var->type &= ~VDISPLAY;
							endobjectchange((INTBIG)ni, VNODEINST);
						}
					} else
					{
						if ((var->type&VDISPLAY) == 0)
						{
							infstr = initinfstr();
							formatinfstr(infstr, _("Parameter '%s' on node %s should be visible (now fixed)"),
								truevariablename(var), describenodeinst(ni));
							err = logerror(returninfstr(infstr), cell, 0);
							addgeomtoerror(err, geom, TRUE, 0, 0);
							startobjectchange((INTBIG)ni, VNODEINST);
							var->type |= VDISPLAY;
							endobjectchange((INTBIG)ni, VNODEINST);
						}
					}
				}
			}
		}
	} else
	{
		ai = geom->entryaddr.ai;

		/* check for being floating if it does not have a visible name on it */
		var = getvalkey((INTBIG)ai, VARCINST, -1, el_arc_name_key);
		if (var == NOVARIABLE || (var->type&VDISPLAY) == 0) checkdangle = TRUE; else
			checkdangle = FALSE;
		if (checkdangle)
		{
			/* do not check for dangle when busses are on named networks */
			if (ai->proto == sch_busarc)
			{
				if (ai->network->namecount > 0 && ai->network->tempname == 0) checkdangle = FALSE;
			}
		}
		if (checkdangle)
		{
			/* check to see if this arc is floating */
			for(i=0; i<2; i++)
			{
				ni = ai->end[i].nodeinst;

				/* OK if not a pin */
				fun = nodefunction(ni);
				if (fun != NPPIN) continue;

				/* OK if it has exports on it */
				if (ni->firstportexpinst != NOPORTEXPINST) continue;

				/* OK if it connects to more than 1 arc */
				if (ni->firstportarcinst == NOPORTARCINST) continue;
				if (ni->firstportarcinst->nextportarcinst != NOPORTARCINST) continue;

				/* the arc dangles */
				err = logerror(_("Arc dangles"), cell, 0);
				addgeomtoerror(err, geom, TRUE, 0, 0);
				return;
			}
		}


		/* check to see if its width is sensible */
		if (ai->network == NONETWORK) signals = 1; else
		{
			signals = ai->network->buswidth;
			if (signals < 1) signals = 1;
		}
		for(i=0; i<2; i++)
		{
			ni = ai->end[i].nodeinst;
			if (ni->proto->primindex != 0) continue;
			np = contentsview(ni->proto);
			if (np == NONODEPROTO) np = ni->proto;
			pp = equivalentport(ni->proto, ai->end[i].portarcinst->proto, np);
			if (pp == NOPORTPROTO)
			{
				infstr = initinfstr();
				formatinfstr(infstr, _("Arc %s connects to port %s of node %s, but there is no equivalent port in cell %s"),
					describearcinst(ai), ai->end[i].portarcinst->proto->protoname, describenodeinst(ni), describenodeproto(np));
				err = logerror(returninfstr(infstr), cell, 0);
				addgeomtoerror(err, geom, TRUE, 0, 0);
				addgeomtoerror(err, ni->geom, TRUE, 0, 0);
				continue;
			}
			portwidth = pp->network->buswidth;
			if (portwidth < 1) portwidth = 1;
			nodesize = ni->arraysize;
			if (nodesize <= 0) nodesize = 1;
			if (signals != portwidth && signals != portwidth*nodesize)
			{
				infstr = initinfstr();
				formatinfstr(infstr, _("Arc %s (%ld wide) connects to port %s of node %s (%ld wide)"),
					describearcinst(ai), signals, pp->protoname, describenodeinst(ni), portwidth);
				err = logerror(returninfstr(infstr), cell, 0);
				addgeomtoerror(err, geom, TRUE, 0, 0);
				addgeomtoerror(err, ni->geom, TRUE, 0, 0);
			}
		}
	}

	/* check for overlap */
	dr_schcheckobjectvicinity(geom, geom, el_matid);
}

/*
 * Routine to check whether object "geom" has a DRC violation with a neighboring object.
 */
void dr_schcheckobjectvicinity(GEOM *topgeom, GEOM *geom, XARRAY trans)
{
	REGISTER INTBIG i, total;
	REGISTER NODEINST *ni, *subni;
	REGISTER ARCINST *ai, *subai;
	XARRAY xformr, xformt, subrot, localtrans;
	static POLYGON *poly = NOPOLYGON;

	needstaticpolygon(&poly, 4, dr_tool->cluster);
	if (geom->entryisnode)
	{
		ni = geom->entryaddr.ni;
		makerot(ni, xformr);
		transmult(xformr, trans, localtrans);
		if (ni->proto->primindex == 0)
		{
			if ((ni->userbits&NEXPAND) != 0)
			{
				/* expand the instance */
				maketrans(ni, xformt);
				transmult(xformt, localtrans, subrot);
				for(subni = ni->proto->firstnodeinst; subni != NONODEINST; subni = subni->nextnodeinst)
					dr_schcheckobjectvicinity(topgeom, subni->geom, subrot); 
				for(subai = ni->proto->firstarcinst; subai != NOARCINST; subai = subai->nextarcinst)
					dr_schcheckobjectvicinity(topgeom, subai->geom, subrot); 
			}
		} else
		{
			/* primitive */
			total = nodepolys(ni, 0, NOWINDOWPART);
			for(i=0; i<total; i++)
			{
				shapenodepoly(ni, i, poly);
				xformpoly(poly, localtrans);
				(void)dr_schcheckpolygonvicinity(topgeom, poly);
			}
		}
	} else
	{
		ai = geom->entryaddr.ai;
		total = arcpolys(ai, NOWINDOWPART);
		for(i=0; i<total; i++)
		{
			shapearcpoly(ai, i, poly);
			xformpoly(poly, trans);
			(void)dr_schcheckpolygonvicinity(topgeom, poly);
		}
	}
}

/*
 * Routine to check whether polygon "poly" from object "geom" has a DRC violation
 * with a neighboring object.  Returns TRUE if an error was found.
 */
BOOLEAN dr_schcheckpolygonvicinity(GEOM *geom, POLYGON *poly)
{
	REGISTER NODEINST *ni, *oni;
	REGISTER ARCINST *ai, *oai;
	REGISTER NODEPROTO *cell;
	REGISTER GEOM *ogeom;
	REGISTER PORTARCINST *pi;
	REGISTER NETWORK *net;
	REGISTER BOOLEAN connected;
	REGISTER INTBIG i, sea;

	/* don't check text */
	if (poly->style == TEXTCENT || poly->style == TEXTTOP ||
		poly->style == TEXTBOT || poly->style == TEXTLEFT ||
		poly->style == TEXTRIGHT || poly->style == TEXTTOPLEFT ||
		poly->style == TEXTBOTLEFT || poly->style == TEXTTOPRIGHT ||
		poly->style == TEXTBOTRIGHT || poly->style == TEXTBOX ||
		poly->style == GRIDDOTS) return(FALSE);

	cell = geomparent(geom);
	if (geom->entryisnode) ni = geom->entryaddr.ni; else ai = geom->entryaddr.ai;
	sea = initsearch(geom->lowx, geom->highx, geom->lowy, geom->highy, cell);
	for(;;)
	{
		ogeom = nextobject(sea);
		if (ogeom == NOGEOM) break;

		/* canonicalize so that errors are found only once */
		if ((INTBIG)geom <= (INTBIG)ogeom) continue;

		/* what type of object was found in area */
		if (ogeom->entryisnode)
		{
			/* found node nearby */
			oni = ogeom->entryaddr.ni;
			if (geom->entryisnode)
			{
				/* this is node, nearby is node: see if two nodes touch */
				for(net = cell->firstnetwork; net != NONETWORK; net = net->nextnetwork)
					net->temp1 = 0;
				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
					pi->conarcinst->network->temp1 |= 1;
				for(pi = oni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
					pi->conarcinst->network->temp1 |= 2;
				for(net = cell->firstnetwork; net != NONETWORK; net = net->nextnetwork)
					if (net->temp1 == 3) break;
				if (net != NONETWORK) continue;
			} else
			{			
				/* this is arc, nearby is node: see if electrically connected */
				for(pi = oni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
					if (pi->conarcinst->network == ai->network) break;
				if (pi != NOPORTARCINST) continue;
			}

			/* no connection: check for touching another */
			if (dr_schcheckpoly(geom, poly, ogeom, ogeom, el_matid, FALSE))
			{
				termsearch(sea);
				return(TRUE);
			}
		} else
		{
			/* found arc nearby */
			oai = ogeom->entryaddr.ai;
			if (geom->entryisnode)
			{
				/* this is node, nearby is arc: see if electrically connected */
				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
					if (pi->conarcinst->network == oai->network) break;
				if (pi != NOPORTARCINST) continue;

				if (dr_schcheckpoly(geom, poly, ogeom, ogeom, el_matid, FALSE))
				{
					termsearch(sea);
					return(TRUE);
				}
			} else
			{
				/* this is arc, nearby is arc: check for colinearity */
				if (dr_schcheckcolinear(ai, oai))
				{
					termsearch(sea);
					return(TRUE);
				}

				/* if not connected, check to see if they touch */
				connected = FALSE;
				if (ai->network == oai->network) connected = TRUE; else
				{
					if (ai->network->buswidth > 1 && oai->network->buswidth <= 1)
					{
						for(i=0; i<ai->network->buswidth; i++)
							if (ai->network->networklist[i] == oai->network) break;
						if (i < ai->network->buswidth) connected = TRUE;
					} else if (oai->network->buswidth > 1 && ai->network->buswidth <= 1)
					{
						for(i=0; i<oai->network->buswidth; i++)
							if (oai->network->networklist[i] == ai->network) break;
						if (i < oai->network->buswidth) connected = TRUE;
					}
				}
				if (!connected)
				{
					if (dr_schcheckpoly(geom, poly, ogeom, ogeom, el_matid, TRUE))
					{
						termsearch(sea);
						return(TRUE);
					}
				}
			}
		}
	}
	return(TRUE);
}

/*
 * Check polygon "poly" from object "geom" against
 * geom "ogeom" transformed by "otrans" (and really on top-level object "otopgeom").
 * Returns TRUE if an error was found.
 */
BOOLEAN dr_schcheckpoly(GEOM *geom, POLYGON *poly, GEOM *otopgeom, GEOM *ogeom, XARRAY otrans,
	BOOLEAN cancross)
{
	REGISTER INTBIG i, total;
	REGISTER NODEINST *ni, *subni;
	REGISTER ARCINST *ai, *subai;
	XARRAY xformr, xformt, thistrans, subrot;
	static POLYGON *opoly = NOPOLYGON;

	needstaticpolygon(&opoly, 4, dr_tool->cluster);
	if (ogeom->entryisnode)
	{
		ni = ogeom->entryaddr.ni;
		makerot(ni, xformr);
		transmult(xformr, otrans, thistrans);
		if (ni->proto->primindex == 0)
		{
			maketrans(ni, xformt);
			transmult(xformt, thistrans, subrot);
			for(subni = ni->proto->firstnodeinst; subni != NONODEINST; subni = subni->nextnodeinst)
			{
				if (dr_schcheckpoly(geom, poly, otopgeom, subni->geom, subrot, cancross))
					return(TRUE);
			}
			for(subai = ni->proto->firstarcinst; subai != NOARCINST; subai = subai->nextarcinst)
			{
				if (dr_schcheckpoly(geom, poly, otopgeom, subai->geom, subrot, cancross))
					return(TRUE);
			}
		} else
		{
			total = nodepolys(ni, 0, NOWINDOWPART);
			for(i=0; i<total; i++)
			{
				shapenodepoly(ni, i, opoly);
				xformpoly(opoly, thistrans);
				if (dr_checkpolyagainstpoly(geom, poly, otopgeom, opoly, cancross))
					return(TRUE);
			}
		}
	} else
	{
		ai = ogeom->entryaddr.ai;
		total = arcpolys(ai, NOWINDOWPART);
		for(i=0; i<total; i++)
		{
			shapearcpoly(ai, i, opoly);
			xformpoly(opoly, otrans);
			if (dr_checkpolyagainstpoly(geom, poly, otopgeom, opoly, cancross))
				return(TRUE);
		}
	}
	return(FALSE);
}

/*
 * Check polygon "poly" from object "geom" against
 * polygon "opoly" from object "ogeom".
 * If "cancross" is TRUE, they can cross each other (but an endpoint cannot touch).
 * Returns TRUE if an error was found.
 */
BOOLEAN dr_checkpolyagainstpoly(GEOM *geom, POLYGON *poly, GEOM *ogeom, POLYGON *opoly, BOOLEAN cancross)
{
	REGISTER void *err;
	REGISTER INTBIG i;

	if (cancross)
	{
		for(i=0; i<poly->count; i++)
		{
			if (polydistance(opoly, poly->xv[i], poly->yv[i]) <= 0) break;
		}
		if (i >= poly->count)
		{
			/* none in "poly" touched one in "opoly", try other way */
			for(i=0; i<opoly->count; i++)
			{
				if (polydistance(poly, opoly->xv[i], opoly->yv[i]) <= 0) break;
			}
			if (i >= opoly->count) return(FALSE);
		}
	} else
	{
		if (!polyintersect(poly, opoly)) return(FALSE);
	}

	/* report the error */
	err = logerror(_("Objects touch"), geomparent(geom), 0);
	addgeomtoerror(err, geom, TRUE, 0, 0);
	addgeomtoerror(err, ogeom, TRUE, 0, 0);
	return(TRUE);
}

/*
 * Routine to check whether arc "ai" is colinear with another.
 * Returns TRUE if an error was found.
 */
BOOLEAN dr_schcheckcolinear(ARCINST *ai, ARCINST *oai)
{
	REGISTER INTBIG lowx, highx, lowy, highy, olow, ohigh, ang, oang, fx, fy, tx, ty,
		ofx, ofy, otx, oty, dist, gdist, frx, fry, tox, toy, ca, sa;
	REGISTER void *err;

	/* get information about the other line */
	fx = ai->end[0].xpos;   fy = ai->end[0].ypos;
	tx = ai->end[1].xpos;   ty = ai->end[1].ypos;
	ofx = oai->end[0].xpos;   ofy = oai->end[0].ypos;
	otx = oai->end[1].xpos;   oty = oai->end[1].ypos;
	if (ofx == otx && ofy == oty) return(FALSE);

	/* see if they are colinear */
	lowx = mini(fx, tx);
	highx = maxi(fx, tx);
	lowy = mini(fy, ty);
	highy = maxi(fy, ty);
	if (fx == tx)
	{
		/* vertical line */
		olow = mini(ofy, oty);
		ohigh = maxi(ofy, oty);
		if (ofx != fx || otx != fx) return(FALSE);
		if (lowy >= ohigh || highy <= olow) return(FALSE);
		ang = 900;
	} else if (fy == ty)
	{
		/* horizontal line */
		olow = mini(ofx, otx);
		ohigh = maxi(ofx, otx);
		if (ofy != fy || oty != fy) return(FALSE);
		if (lowx >= ohigh || highx <= olow) return(FALSE);
		ang = 0;
	} else
	{
		/* general case */
		ang = figureangle(fx, fy, tx, ty);
		oang = figureangle(ofx, ofy, otx, oty);
		if (ang != oang && mini(ang, oang) + 1800 != maxi(ang, oang)) return(FALSE);
		if (muldiv(ofx-fx, ty-fy, tx-fx) != ofy-fy) return(FALSE);
		if (muldiv(otx-fx, ty-fy, tx-fx) != oty-fy) return(FALSE);
		olow = mini(ofy, oty);
		ohigh = maxi(ofy, oty);
		if (lowy >= ohigh || highy <= olow) return(FALSE);
		olow = mini(ofx, otx);
		ohigh = maxi(ofx, otx);
		if (lowx >= ohigh || highx <= olow) return(FALSE);
	}
	err = logerror(_("Arcs overlap"), ai->parent, 0);
	addgeomtoerror(err, ai->geom, TRUE, 0, 0);
	addgeomtoerror(err, oai->geom, TRUE, 0, 0);

	/* add information that shows the arcs */
	ang = (ang + 900) % 3600;
	dist = ai->parent->lib->lambda[ai->parent->tech->techindex] * 2;
	gdist = dist / 2;
	ca = cosine(ang);   sa = sine(ang);
	frx = fx + mult(dist, ca);
	fry = fy + mult(dist, sa);
	tox = tx + mult(dist, ca);
	toy = ty + mult(dist, sa);
	fx = fx + mult(gdist, ca);
	fy = fy + mult(gdist, sa);
	tx = tx + mult(gdist, ca);
	ty = ty + mult(gdist, sa);
	addlinetoerror(err, frx, fry, tox, toy);
	addlinetoerror(err, frx, fry, fx, fy);
	addlinetoerror(err, tx, ty, tox, toy);

	frx = ofx - mult(dist, ca);
	fry = ofy - mult(dist, sa);
	tox = otx - mult(dist, ca);
	toy = oty - mult(dist, sa);
	ofx = ofx - mult(gdist, ca);
	ofy = ofy - mult(gdist, sa);
	otx = otx - mult(gdist, ca);
	oty = oty - mult(gdist, sa);
	addlinetoerror(err, frx, fry, tox, toy);
	addlinetoerror(err, frx, fry, ofx, ofy);
	addlinetoerror(err, otx, oty, tox, toy);
	return(TRUE);
}

/****************************** DIALOGS ******************************/

/* DRC: Options */
static DIALOGITEM dr_optionsdialogitems[] =
{
 /*  1 */ {0, {304,184,328,248}, BUTTON, N_("OK")},
 /*  2 */ {0, {304,44,328,108}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {144,8,160,224}, MESSAGE, N_("Incremental and hierarchical:")},
 /*  4 */ {0, {272,32,288,160}, BUTTON, N_("Edit Rules Deck")},
 /*  5 */ {0, {32,32,48,117}, CHECK, N_("On")},
 /*  6 */ {0, {192,216,208,272}, EDITTEXT, x_("")},
 /*  7 */ {0, {8,8,24,169}, MESSAGE, N_("Incremental DRC:")},
 /*  8 */ {0, {64,8,80,169}, MESSAGE, N_("Hierarchical DRC:")},
 /*  9 */ {0, {248,8,264,169}, MESSAGE, N_("Dracula DRC Interface:")},
 /* 10 */ {0, {112,32,128,192}, BUTTON, N_("Clear valid DRC dates")},
 /* 11 */ {0, {88,32,104,228}, CHECK, N_("Just 1 error per cell")},
 /* 12 */ {0, {216,32,232,296}, CHECK, N_("Ignore center cuts in large contacts")},
 /* 13 */ {0, {192,48,208,212}, MESSAGE, N_("Number of processors:")},
 /* 14 */ {0, {168,32,184,217}, CHECK, N_("Use multiple processors")},
 /* 15 */ {0, {56,8,57,296}, DIVIDELINE, x_("")},
 /* 16 */ {0, {136,8,137,296}, DIVIDELINE, x_("")},
 /* 17 */ {0, {240,8,241,296}, DIVIDELINE, x_("")},
 /* 18 */ {0, {296,8,297,296}, DIVIDELINE, x_("")}
};
static DIALOG dr_optionsdialog = {{50,75,387,381}, N_("DRC Options"), 0, 18, dr_optionsdialogitems, 0, 0};

/* special items for the "DRC Options" dialog: */
#define DDRO_DRACULADECK   4		/* Edit dracula rules (button) */
#define DDRO_DRCON         5		/* Incremental DRC is on (check) */
#define DDRO_NUMPROC       6		/* Number of processors to use (edit text) */
#define DDRO_CLEARDATES   10		/* Clear valid DRC dates (button) */
#define DDRO_JUST1ERROR   11		/* Find 1 error per cell (check) */
#define DDRO_FEWERCUTS    12		/* Ignore center cuts in large contacts (check) */
#define DDRO_USEMULTIPROC 14		/* Use multiple processors (check) */

void dr_optionsdlog(void)
{
	REGISTER INTBIG itemHit, options, oldoptions, i, l, numproc, orignumproc;
	REGISTER CHAR *qual;
	INTBIG dummy, clearvaliddrcdates;
	CHAR header[200], *dummyfile[1];
	REGISTER VARIABLE *var;
	REGISTER WINDOWPART *w;
	REGISTER EDITOR *ed;
	REGISTER void *infstr, *dia;

	dia = DiaInitDialog(&dr_optionsdialog);
	if (dia == 0) return;
	DiaUnDimItem(dia, DDRO_CLEARDATES);

	oldoptions = dr_getoptionsvalue();
	if ((oldoptions&DRCFIRSTERROR) != 0) DiaSetControl(dia, DDRO_JUST1ERROR, 1);
	if ((oldoptions&DRCREASONABLE) != 0) DiaSetControl(dia, DDRO_FEWERCUTS, 1);
	if ((dr_tool->toolstate & TOOLON) != 0) DiaSetControl(dia, DDRO_DRCON, 1);
	if (graphicshas(CANDOTHREADS))
	{
		DiaUnDimItem(dia, DDRO_USEMULTIPROC);
		orignumproc = numproc = (oldoptions&DRCNUMPROC) >> DRCNUMPROCSH;
		if (numproc == 0) numproc = enumprocessors();
		esnprintf(header, 200, x_("%ld"), numproc);
		DiaSetText(dia, DDRO_NUMPROC, header);
		if ((oldoptions&DRCMULTIPROC) != 0)
		{
			DiaSetControl(dia, DDRO_USEMULTIPROC, 1);
			DiaUnDimItem(dia, DDRO_NUMPROC);
		} else
		{
			DiaDimItem(dia, DDRO_NUMPROC);
		}
	} else
	{
		DiaDimItem(dia, DDRO_USEMULTIPROC);
		DiaDimItem(dia, DDRO_NUMPROC);
	}

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL || itemHit == DDRO_DRACULADECK) break;
		if (itemHit == DDRO_DRCON || itemHit == DDRO_JUST1ERROR ||
			itemHit == DDRO_FEWERCUTS || itemHit == DDRO_USEMULTIPROC)
		{
			i = 1 - DiaGetControl(dia, itemHit);
			DiaSetControl(dia, itemHit, i);
			if (itemHit == DDRO_USEMULTIPROC)
			{
				if (i != 0) DiaUnDimItem(dia, DDRO_NUMPROC); else
					DiaDimItem(dia, DDRO_NUMPROC);
			}
			continue;
		}
		if (itemHit == DDRO_CLEARDATES)
		{
			clearvaliddrcdates = 1;
			DiaDimItem(dia, DDRO_CLEARDATES);
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		options = 0;
		if (DiaGetControl(dia, DDRO_JUST1ERROR) != 0) options |= DRCFIRSTERROR;
		if (DiaGetControl(dia, DDRO_FEWERCUTS) != 0) options |= DRCREASONABLE;
		if (graphicshas(CANDOTHREADS))
		{
			if (DiaGetControl(dia, DDRO_USEMULTIPROC) != 0) options |= DRCMULTIPROC;
			numproc = eatoi(DiaGetText(dia, DDRO_NUMPROC));
			if (DiaGetControl(dia, DDRO_USEMULTIPROC) == 0)
				numproc = orignumproc;
			options = (options & ~DRCNUMPROC) | (numproc << DRCNUMPROCSH);
		}
		if (options != oldoptions)
	 		(void)setvalkey((INTBIG)dr_tool, VTOOL, dr_optionskey, options, VINTEGER);

		/* change state of DRC */
		if (DiaGetControl(dia, DDRO_DRCON) != 0)
		{
			if ((dr_tool->toolstate & TOOLON) == 0)
			{
				toolturnon(dr_tool);
				setvalkey((INTBIG)dr_tool, VTOOL, dr_incrementalonkey, 1, VINTEGER);
			}
		} else
		{
			if ((dr_tool->toolstate & TOOLON) != 0)
			{
				toolturnoff(dr_tool, FALSE);
				setvalkey((INTBIG)dr_tool, VTOOL, dr_incrementalonkey, 0, VINTEGER);
			}
		}

		/* clear valid DRC dates if requested */
		if (clearvaliddrcdates != 0) dr_reset_dates();
	}
	DiaDoneDialog(dia);

	if (itemHit == DDRO_DRACULADECK)
	{
		/* now edit the dracula */
		qual = x_("DRC_ecad_deck");
		esnprintf(header, 200, _("ECAD deck for technology %s"), el_curtech->techname);

		var = getval((INTBIG)el_curtech, VTECHNOLOGY, -1, qual);
		if (var == NOVARIABLE)
		{
			dummyfile[0] = x_("");
			var = setval((INTBIG)el_curtech, VTECHNOLOGY, qual, (INTBIG)dummyfile,
				VSTRING|VISARRAY|(1<<VLENGTHSH));
			if (var == NOVARIABLE)
			{
				ttyputerr(_("Cannot create DRC_ecad_deck on the technology"));
				return;
			}
		} else
			var->type &= ~VDONTSAVE;

		/* get a new window, put an editor in it */
		w = us_wantnewwindow(0);
		if (w == NOWINDOWPART) return;
		if (us_makeeditor(w, header, &dummy, &dummy) == NOWINDOWPART) return;
		ed = w->editor;
		ed->editobjqual = qual;
		ed->editobjaddr = (CHAR *)el_curtech;
		ed->editobjtype = VTECHNOLOGY;
		ed->editobjvar = var;
		us_suspendgraphics(w);

		l = getlength(var);
		for(i=0; i<l; i++)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, describevariable(var, i, -1));
			us_addline(w, i, returninfstr(infstr));
		}
		us_resumegraphics(w);
		w->changehandler = us_varchanges;
	}
}

/*
 * Routine to invoke the Design Rule Editing dialog on the current technology.
 */
void dr_rulesdloghook(void)
{
	REGISTER INTBIG i, changed, truelen, truelenR, truelencon, truelenuncon, truelenconW, truelenunconW,
		truelenconM, truelenunconM, truelenedge, truelenconR, truelenunconR, truelenconWR, truelenunconWR,
		truelenconMR, truelenunconMR, truelenedgeR;
	REGISTER VARIABLE *varcon, *varuncon, *varconW, *varunconW, *varconM, *varunconM,
		*varedge, *varconR, *varunconR, *varconWR, *varunconWR, *varconMR, *varunconMR,
		*varedgeR, *varmindist, *varmindistR, *var, *varminsize, *varminsizeR;
	REGISTER DRCRULES *rules;

	/* create a RULES structure */
	rules = dr_allocaterules(el_curtech->layercount, el_curtech->nodeprotocount, el_curtech->techname);
	if (rules == NODRCRULES) return;

	/* fill in the technology name and layer names */
	for(i=0; i<rules->numlayers; i++)
		(void)allocstring(&rules->layernames[i], layername(el_curtech, i), el_tempcluster);

	/* get the distances */
	varmindist = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VFRACT|VISARRAY,
		dr_min_widthkey);
	varmindistR = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VSTRING|VISARRAY,
		dr_min_width_rulekey);
	varcon = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VFRACT|VISARRAY,
		dr_connected_distanceskey);
	varuncon = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VFRACT|VISARRAY,
		dr_unconnected_distanceskey);
	varconW = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VFRACT|VISARRAY,
		dr_connected_distancesWkey);
	varunconW = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VFRACT|VISARRAY,
		dr_unconnected_distancesWkey);
	varconM = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VFRACT|VISARRAY,
		dr_connected_distancesMkey);
	varunconM = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VFRACT|VISARRAY,
		dr_unconnected_distancesMkey);
	varedge = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VFRACT|VISARRAY,
		dr_edge_distanceskey);
	varminsize = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VFRACT|VISARRAY,
		dr_min_node_sizekey);
	varconR = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VSTRING|VISARRAY,
		dr_connected_distances_rulekey);
	varunconR = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VSTRING|VISARRAY,
		dr_unconnected_distances_rulekey);
	varconWR = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VSTRING|VISARRAY,
		dr_connected_distancesW_rulekey);
	varunconWR = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VSTRING|VISARRAY,
		dr_unconnected_distancesW_rulekey);
	varconMR = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VSTRING|VISARRAY,
		dr_connected_distancesM_rulekey);
	varunconMR = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VSTRING|VISARRAY,
		dr_unconnected_distancesM_rulekey);
	varedgeR = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VSTRING|VISARRAY,
		dr_edge_distances_rulekey);
	varminsizeR = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VSTRING|VISARRAY,
		dr_min_node_size_rulekey);

	/* put the distances into the rules structure */
	if (varmindist == NOVARIABLE) truelen = 0; else
		truelen = getlength(varmindist);
	if (varmindistR == NOVARIABLE) truelenR = 0; else
		truelenR = getlength(varmindistR);
	for(i=0; i<rules->numlayers; i++)
	{
		if (i < truelen)
			rules->minwidth[i] = ((INTBIG *)varmindist->addr)[i];
		if (i < truelenR && ((CHAR **)varmindistR->addr)[i][0] != 0)
			(void)reallocstring(&rules->minwidthR[i], ((CHAR **)varmindistR->addr)[i], el_tempcluster);
	}
	if (varcon == NOVARIABLE) truelencon = 0; else
		truelencon = getlength(varcon);
	if (varuncon == NOVARIABLE) truelenuncon = 0; else
		truelenuncon = getlength(varuncon);
	if (varconW == NOVARIABLE) truelenconW = 0; else
		truelenconW = getlength(varconW);
	if (varunconW == NOVARIABLE) truelenunconW = 0; else
		truelenunconW = getlength(varunconW);
	if (varconM == NOVARIABLE) truelenconM = 0; else
		truelenconM = getlength(varconM);
	if (varunconM == NOVARIABLE) truelenunconM = 0; else
		truelenunconM = getlength(varunconM);
	if (varedge == NOVARIABLE) truelenedge = 0; else
		truelenedge = getlength(varedge);

	if (varconR == NOVARIABLE) truelenconR = 0; else
		truelenconR = getlength(varconR);
	if (varunconR == NOVARIABLE) truelenunconR = 0; else
		truelenunconR = getlength(varunconR);
	if (varconWR == NOVARIABLE) truelenconWR = 0; else
		truelenconWR = getlength(varconWR);
	if (varunconWR == NOVARIABLE) truelenunconWR = 0; else
		truelenunconWR = getlength(varunconWR);
	if (varconMR == NOVARIABLE) truelenconMR = 0; else
		truelenconMR = getlength(varconMR);
	if (varunconMR == NOVARIABLE) truelenunconMR = 0; else
		truelenunconMR = getlength(varunconMR);
	if (varedgeR == NOVARIABLE) truelenedgeR = 0; else
		truelenedgeR = getlength(varedgeR);

	for(i=0; i<rules->utsize; i++)
	{
		if (i < truelencon)
			rules->conlist[i] = ((INTBIG *)varcon->addr)[i];
		if (i < truelenuncon)
			rules->unconlist[i] = ((INTBIG *)varuncon->addr)[i];
		if (i < truelenconW)
			rules->conlistW[i] = ((INTBIG *)varconW->addr)[i];
		if (i < truelenunconW)
			rules->unconlistW[i] = ((INTBIG *)varunconW->addr)[i];
		if (i < truelenconM)
			rules->conlistM[i] = ((INTBIG *)varconM->addr)[i];
		if (i < truelenunconM)
			rules->unconlistM[i] = ((INTBIG *)varunconM->addr)[i];
		if (i < truelenedge)
			rules->edgelist[i] = ((INTBIG *)varedge->addr)[i];

		if (i < truelenconR && ((CHAR **)varconR->addr)[i][0] != 0)
			(void)reallocstring(&rules->conlistR[i], ((CHAR **)varconR->addr)[i], el_tempcluster);
		if (i < truelenunconR && ((CHAR **)varunconR->addr)[i][0] != 0)
			(void)reallocstring(&rules->unconlistR[i], ((CHAR **)varunconR->addr)[i], el_tempcluster);
		if (i < truelenconWR && ((CHAR **)varconWR->addr)[i][0] != 0)
			(void)reallocstring(&rules->conlistWR[i], ((CHAR **)varconWR->addr)[i], el_tempcluster);
		if (i < truelenunconWR && ((CHAR **)varunconWR->addr)[i][0] != 0)
			(void)reallocstring(&rules->unconlistWR[i], ((CHAR **)varunconWR->addr)[i], el_tempcluster);
		if (i < truelenconMR && ((CHAR **)varconMR->addr)[i][0] != 0)
			(void)reallocstring(&rules->conlistMR[i], ((CHAR **)varconMR->addr)[i], el_tempcluster);
		if (i < truelenunconMR && ((CHAR **)varunconMR->addr)[i][0] != 0)
			(void)reallocstring(&rules->unconlistMR[i], ((CHAR **)varunconMR->addr)[i], el_tempcluster);
		if (i < truelenedgeR && ((CHAR **)varedgeR->addr)[i][0] != 0)
			(void)reallocstring(&rules->edgelistR[i], ((CHAR **)varedgeR->addr)[i], el_tempcluster);
	}
	var = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VFRACT, dr_wide_limitkey);
	if (var != NOVARIABLE) rules->widelimit = var->addr;

	/* get the node minimum size information */
	if (varminsize == NOVARIABLE) truelen = 0; else
		truelen = getlength(varminsize);
	if (varminsizeR == NOVARIABLE) truelenR = 0; else
		truelenR = getlength(varminsizeR);
	for(i=0; i<rules->numnodes; i++)
	{
		rules->nodenames[i] = el_curtech->nodeprotos[i]->nodename;
		if (i < truelen)
		{
			rules->minnodesize[i*2] = ((INTBIG *)varminsize->addr)[i*2];
			rules->minnodesize[i*2+1] = ((INTBIG *)varminsize->addr)[i*2+1];
		}
		if (i < truelenR && ((CHAR **)varminsizeR->addr)[i][0] != 0)
			(void)reallocstring(&rules->minnodesizeR[i], ((CHAR **)varminsizeR->addr)[i], el_tempcluster);
	}

	/* run the DRC editing dialog */
	changed = dr_rulesdlog(el_curtech, rules);

	/* if values changed, update variables */
	if (changed != 0)
	{
		if ((changed&RULECHANGEMINWID) != 0)
			setvalkey((INTBIG)el_curtech, VTECHNOLOGY, dr_min_widthkey,
				(INTBIG)rules->minwidth, VFRACT|VISARRAY|(rules->numlayers<<VLENGTHSH));
		if ((changed&RULECHANGEMINWIDR) != 0)
			setvalkey((INTBIG)el_curtech, VTECHNOLOGY, dr_min_width_rulekey,
				(INTBIG)rules->minwidthR, VSTRING|VISARRAY|(rules->numlayers<<VLENGTHSH));
		if ((changed&RULECHANGECONSPA) != 0)
			setvalkey((INTBIG)el_curtech, VTECHNOLOGY, dr_connected_distanceskey,
				(INTBIG)rules->conlist, VFRACT|VISARRAY|(rules->utsize<<VLENGTHSH));
		if ((changed&RULECHANGEUCONSPA) != 0)
			setvalkey((INTBIG)el_curtech, VTECHNOLOGY, dr_unconnected_distanceskey,
				(INTBIG)rules->unconlist, VFRACT|VISARRAY|(rules->utsize<<VLENGTHSH));
		if ((changed&RULECHANGECONSPAW) != 0)
			setvalkey((INTBIG)el_curtech, VTECHNOLOGY, dr_connected_distancesWkey,
				(INTBIG)rules->conlistW, VFRACT|VISARRAY|(rules->utsize<<VLENGTHSH));
		if ((changed&RULECHANGEUCONSPAW) != 0)
			setvalkey((INTBIG)el_curtech, VTECHNOLOGY, dr_unconnected_distancesWkey,
				(INTBIG)rules->unconlistW, VFRACT|VISARRAY|(rules->utsize<<VLENGTHSH));
		if ((changed&RULECHANGECONSPAM) != 0)
			setvalkey((INTBIG)el_curtech, VTECHNOLOGY, dr_connected_distancesMkey,
				(INTBIG)rules->conlistM, VFRACT|VISARRAY|(rules->utsize<<VLENGTHSH));
		if ((changed&RULECHANGEUCONSPAM) != 0)
			setvalkey((INTBIG)el_curtech, VTECHNOLOGY, dr_unconnected_distancesMkey,
				(INTBIG)rules->unconlistM, VFRACT|VISARRAY|(rules->utsize<<VLENGTHSH));
		if ((changed&RULECHANGEEDGESPA) != 0)
			setvalkey((INTBIG)el_curtech, VTECHNOLOGY, dr_edge_distanceskey,
				(INTBIG)rules->edgelist, VFRACT|VISARRAY|(rules->utsize<<VLENGTHSH));
		if ((changed&RULECHANGECONSPAR) != 0)
			setvalkey((INTBIG)el_curtech, VTECHNOLOGY, dr_connected_distances_rulekey,
				(INTBIG)rules->conlistR, VSTRING|VISARRAY|(rules->utsize<<VLENGTHSH));
		if ((changed&RULECHANGEUCONSPAR) != 0)
			setvalkey((INTBIG)el_curtech, VTECHNOLOGY, dr_unconnected_distances_rulekey,
				(INTBIG)rules->unconlistR, VSTRING|VISARRAY|(rules->utsize<<VLENGTHSH));
		if ((changed&RULECHANGECONSPAWR) != 0)
			setvalkey((INTBIG)el_curtech, VTECHNOLOGY, dr_connected_distancesW_rulekey,
				(INTBIG)rules->conlistWR, VSTRING|VISARRAY|(rules->utsize<<VLENGTHSH));
		if ((changed&RULECHANGEUCONSPAWR) != 0)
			setvalkey((INTBIG)el_curtech, VTECHNOLOGY, dr_unconnected_distancesW_rulekey,
				(INTBIG)rules->unconlistWR, VSTRING|VISARRAY|(rules->utsize<<VLENGTHSH));
		if ((changed&RULECHANGECONSPAMR) != 0)
			setvalkey((INTBIG)el_curtech, VTECHNOLOGY, dr_connected_distancesM_rulekey,
				(INTBIG)rules->conlistMR, VSTRING|VISARRAY|(rules->utsize<<VLENGTHSH));
		if ((changed&RULECHANGEUCONSPAMR) != 0)
			setvalkey((INTBIG)el_curtech, VTECHNOLOGY, dr_unconnected_distancesM_rulekey,
				(INTBIG)rules->unconlistMR, VSTRING|VISARRAY|(rules->utsize<<VLENGTHSH));
		if ((changed&RULECHANGEEDGESPAR) != 0)
			setvalkey((INTBIG)el_curtech, VTECHNOLOGY, dr_edge_distances_rulekey,
				(INTBIG)rules->edgelistR, VSTRING|VISARRAY|(rules->utsize<<VLENGTHSH));
		if ((changed&RULECHANGEWIDLIMIT) != 0)
			setvalkey((INTBIG)el_curtech, VTECHNOLOGY, dr_wide_limitkey,
				rules->widelimit, VFRACT);

		if ((changed&RULECHANGEMINSIZE) != 0)
			setvalkey((INTBIG)el_curtech, VTECHNOLOGY, dr_min_node_sizekey,
				(INTBIG)rules->minnodesize, VFRACT|VISARRAY|((rules->numnodes*2)<<VLENGTHSH));
		if ((changed&RULECHANGEMINSIZER) != 0)
			setvalkey((INTBIG)el_curtech, VTECHNOLOGY, dr_min_node_size_rulekey,
				(INTBIG)rules->minnodesizeR, VSTRING|VISARRAY|(rules->numnodes<<VLENGTHSH));

		/* reset valid DRC dates */
		dr_reset_dates();
	}

	/* free the rules structure */
	dr_freerules(rules);
}

/*
 * Routine to allocate the space for a DRC rules object that describes "layercount" layers.
 * Returns NODRCRULES on error.
 */
DRCRULES *dr_allocaterules(INTBIG layercount, INTBIG nodecount, CHAR *techname)
{
	DRCRULES *rules;
	REGISTER INTBIG i;

	rules = (DRCRULES *)emalloc(sizeof (DRCRULES), el_tempcluster);
	if (rules == 0) return(NODRCRULES);

	(void)allocstring(&rules->techname, techname, el_tempcluster);

	/* cache the spacing rules for this technology */
	rules->numlayers = layercount;
	rules->utsize = (layercount * layercount - layercount) / 2 + layercount;
	rules->layernames = (CHAR **)emalloc(rules->numlayers * (sizeof (CHAR *)), el_tempcluster);
	if (rules->layernames == 0) return(NODRCRULES);
	rules->minwidth = (INTBIG *)emalloc(rules->numlayers * SIZEOFINTBIG, el_tempcluster);
	if (rules->minwidth == 0) return(NODRCRULES);
	rules->minwidthR = (CHAR **)emalloc(rules->numlayers * (sizeof (CHAR *)), el_tempcluster);
	if (rules->minwidthR == 0) return(NODRCRULES);
	rules->conlist = (INTBIG *)emalloc(rules->utsize * SIZEOFINTBIG, el_tempcluster);
	if (rules->conlist == 0) return(NODRCRULES);
	rules->unconlist = (INTBIG *)emalloc(rules->utsize * SIZEOFINTBIG, el_tempcluster);
	if (rules->unconlist == 0) return(NODRCRULES);
	rules->conlistW = (INTBIG *)emalloc(rules->utsize * SIZEOFINTBIG, el_tempcluster);
	if (rules->conlistW == 0) return(NODRCRULES);
	rules->unconlistW = (INTBIG *)emalloc(rules->utsize * SIZEOFINTBIG, el_tempcluster);
	if (rules->unconlistW == 0) return(NODRCRULES);
	rules->conlistM = (INTBIG *)emalloc(rules->utsize * SIZEOFINTBIG, el_tempcluster);
	if (rules->conlistM == 0) return(NODRCRULES);
	rules->unconlistM = (INTBIG *)emalloc(rules->utsize * SIZEOFINTBIG, el_tempcluster);
	if (rules->unconlistM == 0) return(NODRCRULES);
	rules->edgelist = (INTBIG *)emalloc(rules->utsize * SIZEOFINTBIG, el_tempcluster);
	if (rules->edgelist == 0) return(NODRCRULES);

	rules->conlistR = (CHAR **)emalloc(rules->utsize * (sizeof (CHAR *)), el_tempcluster);
	if (rules->conlistR == 0) return(NODRCRULES);
	rules->unconlistR = (CHAR **)emalloc(rules->utsize * (sizeof (CHAR *)), el_tempcluster);
	if (rules->unconlistR == 0) return(NODRCRULES);
	rules->conlistWR = (CHAR **)emalloc(rules->utsize * (sizeof (CHAR *)), el_tempcluster);
	if (rules->conlistWR == 0) return(NODRCRULES);
	rules->unconlistWR = (CHAR **)emalloc(rules->utsize * (sizeof (CHAR *)), el_tempcluster);
	if (rules->unconlistWR == 0) return(NODRCRULES);
	rules->conlistMR = (CHAR **)emalloc(rules->utsize * (sizeof (CHAR *)), el_tempcluster);
	if (rules->conlistMR == 0) return(NODRCRULES);
	rules->unconlistMR = (CHAR **)emalloc(rules->utsize * (sizeof (CHAR *)), el_tempcluster);
	if (rules->unconlistMR == 0) return(NODRCRULES);
	rules->edgelistR = (CHAR **)emalloc(rules->utsize * (sizeof (CHAR *)), el_tempcluster);
	if (rules->edgelistR == 0) return(NODRCRULES);

	for(i=0; i<rules->numlayers; i++)
	{
		rules->minwidth[i] = XX;
		(void)allocstring(&rules->minwidthR[i], x_(""), el_tempcluster);
		rules->layernames[i] = 0;
	}
	for(i=0; i<rules->utsize; i++)
	{
		rules->conlist[i] = XX;
		rules->unconlist[i] = XX;
		rules->conlistW[i] = XX;
		rules->unconlistW[i] = XX;
		rules->conlistM[i] = XX;
		rules->unconlistM[i] = XX;
		rules->edgelist[i] = XX;
		(void)allocstring(&rules->conlistR[i], x_(""), el_tempcluster);
		(void)allocstring(&rules->unconlistR[i], x_(""), el_tempcluster);
		(void)allocstring(&rules->conlistWR[i], x_(""), el_tempcluster);
		(void)allocstring(&rules->unconlistWR[i], x_(""), el_tempcluster);
		(void)allocstring(&rules->conlistMR[i], x_(""), el_tempcluster);
		(void)allocstring(&rules->unconlistMR[i], x_(""), el_tempcluster);
		(void)allocstring(&rules->edgelistR[i], x_(""), el_tempcluster);
	}
	rules->widelimit = K10;

	/* allocate space for node rules */
	rules->numnodes = nodecount;
	if (nodecount > 0)
	{
		rules->nodenames = (CHAR **)emalloc(nodecount * (sizeof (CHAR *)), el_tempcluster);
		if (rules->nodenames == 0) return(NODRCRULES);
		rules->minnodesize = (INTBIG *)emalloc(nodecount * 2 * SIZEOFINTBIG, el_tempcluster);
		if (rules->minnodesize == 0) return(NODRCRULES);
		rules->minnodesizeR = (CHAR **)emalloc(nodecount * (sizeof (CHAR *)), el_tempcluster);
		if (rules->minnodesizeR == 0) return(NODRCRULES);
		for(i=0; i<nodecount; i++)
		{
			rules->minnodesize[i*2] = XX;
			rules->minnodesize[i*2+1] = XX;
			(void)allocstring(&rules->minnodesizeR[i], x_(""), el_tempcluster);
		}
	}
	return(rules);
}

/*
 * Routine to free the DRCRULES structure in "rules".
 */
void dr_freerules(DRCRULES *rules)
{
	REGISTER INTBIG i;

	efree((CHAR *)rules->techname);
	efree((CHAR *)rules->conlist);
	efree((CHAR *)rules->unconlist);
	efree((CHAR *)rules->conlistW);
	efree((CHAR *)rules->unconlistW);
	efree((CHAR *)rules->conlistM);
	efree((CHAR *)rules->unconlistM);
	efree((CHAR *)rules->edgelist);
	efree((CHAR *)rules->minwidth);
	for(i=0; i<rules->utsize; i++)
	{
		efree((CHAR *)rules->conlistR[i]);
		efree((CHAR *)rules->unconlistR[i]);
		efree((CHAR *)rules->conlistWR[i]);
		efree((CHAR *)rules->unconlistWR[i]);
		efree((CHAR *)rules->conlistMR[i]);
		efree((CHAR *)rules->unconlistMR[i]);
		efree((CHAR *)rules->edgelistR[i]);
	}
	for(i=0; i<rules->numlayers; i++)
	{
		efree((CHAR *)rules->minwidthR[i]);
		if (rules->layernames[i] != 0)
			efree((CHAR *)rules->layernames[i]);
	}
	for(i=0; i<rules->numnodes; i++)
		efree((CHAR *)rules->minnodesizeR[i]);
	if (rules->numnodes > 0)
	{
		efree((CHAR *)rules->nodenames);
		efree((CHAR *)rules->minnodesize);
		efree((CHAR *)rules->minnodesizeR);
	}
	efree((CHAR *)rules->conlistR);
	efree((CHAR *)rules->unconlistR);
	efree((CHAR *)rules->conlistWR);
	efree((CHAR *)rules->unconlistWR);
	efree((CHAR *)rules->conlistMR);
	efree((CHAR *)rules->unconlistMR);
	efree((CHAR *)rules->edgelistR);
	efree((CHAR *)rules->minwidthR);
	efree((CHAR *)rules->layernames);
}

/****************************** DESIGN-RULE EDITING DIALOG ******************************/

/* DRC: Rules */
static DIALOGITEM dr_rulesdialogitems[] =
{
 /*  1 */ {0, {8,516,32,580}, BUTTON, N_("OK")},
 /*  2 */ {0, {40,516,64,580}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,100,188,278}, SCROLL, x_("")},
 /*  4 */ {0, {32,8,48,95}, RADIO, N_("Layers:")},
 /*  5 */ {0, {212,8,435,300}, SCROLL, x_("")},
 /*  6 */ {0, {8,100,24,294}, MESSAGE|INACTIVE, x_("")},
 /*  7 */ {0, {8,8,24,95}, MESSAGE, N_("Technology:")},
 /*  8 */ {0, {192,8,208,88}, MESSAGE, N_("To Layer:")},
 /*  9 */ {0, {88,452,104,523}, MESSAGE, N_("Size")},
 /* 10 */ {0, {88,528,104,583}, MESSAGE, N_("Rule")},
 /* 11 */ {0, {112,308,128,424}, MESSAGE, N_("Minimum Width:")},
 /* 12 */ {0, {112,454,128,502}, EDITTEXT, x_("")},
 /* 13 */ {0, {112,514,128,596}, EDITTEXT, x_("")},
 /* 14 */ {0, {180,308,196,387}, MESSAGE, N_("Normal:")},
 /* 15 */ {0, {204,324,220,450}, MESSAGE, N_("When connected:")},
 /* 16 */ {0, {204,454,220,502}, EDITTEXT, x_("")},
 /* 17 */ {0, {204,514,220,595}, EDITTEXT, x_("")},
 /* 18 */ {0, {232,324,248,450}, MESSAGE, N_("Not connected:")},
 /* 19 */ {0, {232,454,248,502}, EDITTEXT, x_("")},
 /* 20 */ {0, {232,514,248,595}, EDITTEXT, x_("")},
 /* 21 */ {0, {288,308,304,520}, MESSAGE, N_("Wide (when bigger than this):")},
 /* 22 */ {0, {312,324,328,450}, MESSAGE, N_("When connected:")},
 /* 23 */ {0, {312,454,328,502}, EDITTEXT, x_("")},
 /* 24 */ {0, {312,514,328,595}, EDITTEXT, x_("")},
 /* 25 */ {0, {340,324,356,450}, MESSAGE, N_("Not connected:")},
 /* 26 */ {0, {340,454,356,502}, EDITTEXT, x_("")},
 /* 27 */ {0, {340,514,356,595}, EDITTEXT, x_("")},
 /* 28 */ {0, {368,308,384,448}, MESSAGE, N_("Multiple cuts:")},
 /* 29 */ {0, {392,324,408,450}, MESSAGE, N_("When connected:")},
 /* 30 */ {0, {392,454,408,502}, EDITTEXT, x_("")},
 /* 31 */ {0, {392,514,408,595}, EDITTEXT, x_("")},
 /* 32 */ {0, {420,324,436,450}, MESSAGE, N_("Not connected:")},
 /* 33 */ {0, {420,454,436,502}, EDITTEXT, x_("")},
 /* 34 */ {0, {420,514,436,595}, EDITTEXT, x_("")},
 /* 35 */ {0, {180,452,196,523}, MESSAGE, N_("Distance")},
 /* 36 */ {0, {180,528,196,583}, MESSAGE, N_("Rule")},
 /* 37 */ {0, {24,328,48,488}, BUTTON, N_("Factory Reset of Rules")},
 /* 38 */ {0, {288,526,304,574}, EDITTEXT, x_("")},
 /* 39 */ {0, {260,324,276,450}, MESSAGE, N_("Edge:")},
 /* 40 */ {0, {260,454,276,502}, EDITTEXT, x_("")},
 /* 41 */ {0, {260,514,276,595}, EDITTEXT, x_("")},
 /* 42 */ {0, {192,104,208,300}, CHECK, N_("Show only lines with rules")},
 /* 43 */ {0, {56,8,72,95}, RADIO, N_("Nodes:")},
 /* 44 */ {0, {136,308,152,424}, MESSAGE, N_("Minimum Height:")},
 /* 45 */ {0, {136,454,152,502}, EDITTEXT, x_("")}
};
static DIALOG dr_rulesdialog = {{50,75,495,681}, N_("Design Rules"), 0, 45, dr_rulesdialogitems, 0, 0};

static void       dr_loaddrcdialog(DRCRULES *rules, INTBIG *validlayers, void *dia);
static void       dr_loaddrcruleobjects(DRCRULES *rules, INTBIG *validlayers, void *dia);
static void       dr_loaddrcdistance(DRCRULES *rules, void *dia);
static CHAR      *dr_loadoptlistline(DRCRULES *rules, INTBIG dindex, INTBIG lindex, INTBIG onlyvalid);
static INTBIG     dr_rulesdialoggetlayer(DRCRULES *rules, INTBIG item, void *dia);

/* special items for the "DRC Rules" dialog: */
#define DDRR_FROMLAYER     3		/* From layer (scroll) */
#define DDRR_SHOWLAYERS    4		/* Show layers in upper list (check) */
#define DDRR_TOLAYER       5		/* To layer (scroll) */
#define DDRR_TECHNAME      6		/* Technology name (stat text) */
#define DDRR_TOLAYER_L     8		/* To layer label (message) */
#define DDRR_MINWIDTH     12		/* Minimum width (edit text) */
#define DDRR_MINWIDTHR    13		/* Minimum width rule (edit text) */
#define DDRR_NORMDIST_L   14		/* Normal distance label (message) */
#define DDRR_CONDIST_L    15		/* Connected distance label (message) */
#define DDRR_CONDIST      16		/* Connected distance (edit text) */
#define DDRR_CONDISTR     17		/* Connected distance rule (edit text) */
#define DDRR_UCONDIST_L   18		/* Unconnected distance label (message) */
#define DDRR_UCONDIST     19		/* Unconnected distance (edit text) */
#define DDRR_UCONDISTR    20		/* Unconnected distance rule (edit text) */
#define DDRR_WIDEDIST_L   21		/* Wide distance label (message) */
#define DDRR_CONDISTW_L   22		/* Connected wide distance label (message) */
#define DDRR_CONDISTW     23		/* Connected wide distance (edit text) */
#define DDRR_CONDISTWR    24		/* Connected wide distance rule (edit text) */
#define DDRR_UCONDISTW_L  25		/* Unconnected wide distance label (message) */
#define DDRR_UCONDISTW    26		/* Unconnected wide distance (edit text) */
#define DDRR_UCONDISTWR   27		/* Unconnected wide distance rule (edit text) */
#define DDRR_MULTIDIST_L  28		/* Multicut distance label (message) */
#define DDRR_CONDISTM_L   29		/* Connected multi distance label (message) */
#define DDRR_CONDISTM     30		/* Connected multi distance (edit text) */
#define DDRR_CONDISTMR    31		/* Connected multi distance rule (edit text) */
#define DDRR_UCONDISTM_L  32		/* Unconnected multi distance label (message) */
#define DDRR_UCONDISTM    33		/* Unconnected multi distance (edit text) */
#define DDRR_UCONDISTMR   34		/* Unconnected multi distance rule (edit text) */
#define DDRR_DIST_L       35		/* Spacing distance label (message) */
#define DDRR_DISTR_L      36		/* Spacing rule label (message) */
#define DDRR_FACTORYRST   37		/* Factor Reset (button) */
#define DDRR_WIDELIMIT    38		/* Wide limit (edit text) */
#define DDRR_EDGEDIST_L   39		/* Edge distance label (message) */
#define DDRR_EDGEDIST     40		/* Edge distance (edit text) */
#define DDRR_EDGEDISTR    41		/* Edge distance rule (edit text) */
#define DDRR_VALIDRULES   42		/* Only show lines with rules (check) */
#define DDRR_SHOWNODES    43		/* Show nodes in upper list (check) */
#define DDRR_MINHEIGHT_L  44		/* Minimum height label (message) */
#define DDRR_MINHEIGHT    45		/* Minimum height (edit text) */

/*
 * Routine to implement a dialog for design rule editing.  This is called from the design-rule checker
 * to edit current rules, and it is called by the technology editor to edit rules from technologies
 * under construction.  Takes a technology name ("techname"), number of layers ("numlayers"), and
 * the layer names ("layernames").  Takes all of the design-rule tables:
 *   "minwidth" and "minwidthR", the minimum width and rule for each layer
 *   "conlist" and "conlistR", the distances and rules between connected layers
 *   "unconlist" and "unconlistR", the distances and rules between unconnected layers
 *   "conlistW" and "conlistWR", the distances and rules between connected layers that are wide
 *   "unconlistW" and "unconlistWR", the distances and rules between unconnected layers that are wide
 *   "conlistM" and "conlistMR", the distances and rules between connected layers on multi-cut contacts
 *   "unconlistM" and "unconlistMR", the distances and rules between unconnected layers on multi-cut contacts
 * Returns bits that indicate which values changed.
 */
INTBIG dr_rulesdlog(TECHNOLOGY *tech, DRCRULES *rules)
{
	REGISTER INTBIG itemHit, i, j, dist, layer1, layer2, temp, dindex, *validlayers, changed;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER NODEPROTO *np;
	NODEINST node;
	ARCINST arc;
	REGISTER ARCPROTO *ap;
	REGISTER CHAR *pt, *varname;
	REGISTER POLYGON *poly;
	REGISTER void *dia;

	/* determine which layers are valid */
	validlayers = (INTBIG *)emalloc(rules->numlayers * SIZEOFINTBIG, dr_tool->cluster);
	if (validlayers == 0) return(0);
	if (tech == NOTECHNOLOGY)
	{
		/* designing rules in technology edit: all layers are valid */
		for(i=0; i<rules->numlayers; i++) validlayers[i] = 1;
	} else
	{
		/* editing rules for technology: find used layers only */
		poly = allocpolygon(4, dr_tool->cluster);
		for(i=0; i<rules->numlayers; i++) validlayers[i] = 0;
		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if ((np->userbits&NNOTUSED) != 0) continue;
			ni = &node;   initdummynode(ni);
			ni->proto = np;
			ni->lowx = np->lowx;   ni->highx = np->highx;
			ni->lowy = np->lowy;   ni->highy = np->highy;
			j = nodepolys(ni, 0, NOWINDOWPART);
			for(i=0; i<j; i++)
			{
				shapenodepoly(ni, i, poly);
				if (poly->tech != tech) continue;
				validlayers[poly->layer] = 1;
			}
		}
		for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
		{
			if ((ap->userbits&ANOTUSED) != 0) continue;
			ai = &arc;   initdummyarc(ai);
			ai->proto = ap;
			ai->width = ap->nominalwidth;
			j = arcpolys(ai, NOWINDOWPART);
			for(i=0; i<j; i++)
			{
				shapearcpoly(ai, i, poly);
				if (poly->tech != tech) continue;
				validlayers[poly->layer] = 1;
			}
		}
		freepolygon(poly);
	}

	dia = DiaInitDialog(&dr_rulesdialog);
	if (dia == 0) return(0);
	DiaInitTextDialog(dia, DDRR_FROMLAYER, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0,
		SCSELMOUSE|SCREPORT);
	DiaInitTextDialog(dia, DDRR_TOLAYER, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0,
		SCSELMOUSE|SCREPORT);
	if (tech == NOTECHNOLOGY) DiaDimItem(dia, DDRR_FACTORYRST); else
		DiaUnDimItem(dia, DDRR_FACTORYRST);

	/* make the layer list */
	DiaSetControl(dia, DDRR_SHOWLAYERS, 1);
	dr_loaddrcruleobjects(rules, validlayers, dia);

	DiaSetText(dia, DDRR_TECHNAME, rules->techname);
	DiaSetText(dia, DDRR_WIDELIMIT, frtoa(rules->widelimit));

	dr_loaddrcdialog(rules, validlayers, dia);
	dr_loaddrcdistance(rules, dia);
	changed = 0;

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL || itemHit == DDRR_FACTORYRST) break;
		if (itemHit == DDRR_SHOWLAYERS || itemHit == DDRR_SHOWNODES)
		{
			DiaSetControl(dia, DDRR_SHOWLAYERS, 0);
			DiaSetControl(dia, DDRR_SHOWNODES, 0);
			DiaSetControl(dia, itemHit, 1);
			dr_loaddrcruleobjects(rules, validlayers, dia);
			dr_loaddrcdialog(rules, validlayers, dia);
			continue;
		}
		if (itemHit == DDRR_CONDIST || itemHit == DDRR_UCONDIST ||
			itemHit == DDRR_CONDISTW || itemHit == DDRR_UCONDISTW ||
			itemHit == DDRR_CONDISTM || itemHit == DDRR_UCONDISTM ||
			itemHit == DDRR_EDGEDIST)
		{
			/* typed into distance field */
			i = dr_rulesdialoggetlayer(rules, DDRR_TOLAYER, dia);
			if (i < 0) continue;
			pt = DiaGetText(dia, itemHit);
			while (*pt == ' ' || *pt == '\t') pt++;
			if (*pt == 0) dist = -WHOLE; else
				dist = atofr(pt);
			layer1 = dr_rulesdialoggetlayer(rules, DDRR_FROMLAYER, dia);
			if (layer1 < 0) continue;
			layer2 = i;
			if (layer1 > layer2) { temp = layer1; layer1 = layer2;  layer2 = temp; }
			dindex = (layer1+1) * (layer1/2) + (layer1&1) * ((layer1+1)/2);
			dindex = layer2 + rules->numlayers * layer1 - dindex;
			if (itemHit == DDRR_CONDIST)
			{
				if (rules->conlist[dindex] != dist) changed |= RULECHANGECONSPA;
				rules->conlist[dindex] = dist;
			} else if (itemHit == DDRR_UCONDIST)
			{
				if (rules->unconlist[dindex] != dist) changed |= RULECHANGEUCONSPA;
				rules->unconlist[dindex] = dist;
			} else if (itemHit == DDRR_CONDISTW)
			{
				if (rules->conlistW[dindex] != dist) changed |= RULECHANGECONSPAW;
				rules->conlistW[dindex] = dist;
			} else if (itemHit == DDRR_UCONDISTW)
			{
				if (rules->unconlistW[dindex] != dist) changed |= RULECHANGEUCONSPAW;
				rules->unconlistW[dindex] = dist;
			} else if (itemHit == DDRR_CONDISTM)
			{
				if (rules->conlistM[dindex] != dist) changed |= RULECHANGECONSPAM;
				rules->conlistM[dindex] = dist;
			} else if (itemHit == DDRR_UCONDISTM)
			{
				if (rules->unconlistM[dindex] != dist) changed |= RULECHANGEUCONSPAM;
				rules->unconlistM[dindex] = dist;
			} else if (itemHit == DDRR_EDGEDIST)
			{
				if (rules->edgelist[dindex] != dist) changed |= RULECHANGEEDGESPA;
				rules->edgelist[dindex] = dist;
			}

			pt = dr_loadoptlistline(rules, dindex, i, 0);
			DiaSetScrollLine(dia, DDRR_TOLAYER, DiaGetCurLine(dia, DDRR_TOLAYER), pt);
			continue;
		}
		if (itemHit == DDRR_CONDISTR || itemHit == DDRR_UCONDISTR ||
			itemHit == DDRR_CONDISTWR || itemHit == DDRR_UCONDISTWR ||
			itemHit == DDRR_CONDISTMR || itemHit == DDRR_UCONDISTMR ||
			itemHit == DDRR_EDGEDISTR)
		{
			/* typed into rule field */
			i = dr_rulesdialoggetlayer(rules, DDRR_TOLAYER, dia);
			if (i < 0) continue;
			pt = DiaGetText(dia, itemHit);
			layer1 = dr_rulesdialoggetlayer(rules, DDRR_FROMLAYER, dia);
			if (layer1 < 0) continue;
			layer2 = i;
			if (layer1 > layer2) { temp = layer1; layer1 = layer2;  layer2 = temp; }
			dindex = (layer1+1) * (layer1/2) + (layer1&1) * ((layer1+1)/2);
			dindex = layer2 + rules->numlayers * layer1 - dindex;
			if (itemHit == DDRR_CONDISTR)
			{
				if (estrcmp(rules->conlistR[dindex], pt) != 0) changed |= RULECHANGECONSPAR;
				(void)reallocstring(&rules->conlistR[dindex], pt, el_tempcluster);
			} else if (itemHit == DDRR_UCONDISTR)
			{
				if (estrcmp(rules->unconlistR[dindex], pt) != 0) changed |= RULECHANGEUCONSPAR;
				(void)reallocstring(&rules->unconlistR[dindex], pt, el_tempcluster);
			} else if (itemHit == DDRR_CONDISTWR)
			{
				if (estrcmp(rules->conlistWR[dindex], pt) != 0) changed |= RULECHANGECONSPAWR;
				(void)reallocstring(&rules->conlistWR[dindex], pt, el_tempcluster);
			} else if (itemHit == DDRR_UCONDISTWR)
			{
				if (estrcmp(rules->unconlistWR[dindex], pt) != 0) changed |= RULECHANGEUCONSPAWR;
				(void)reallocstring(&rules->unconlistWR[dindex], pt, el_tempcluster);
			} else if (itemHit == DDRR_CONDISTMR)
			{
				if (estrcmp(rules->conlistMR[dindex], pt) != 0) changed |= RULECHANGECONSPAMR;
				(void)reallocstring(&rules->conlistMR[dindex], pt, el_tempcluster);
			} else if (itemHit == DDRR_UCONDISTMR)
			{
				if (estrcmp(rules->unconlistMR[dindex], pt) != 0) changed |= RULECHANGEUCONSPAMR;
				(void)reallocstring(&rules->unconlistMR[dindex], pt, el_tempcluster);
			} else if (itemHit == DDRR_EDGEDISTR)
			{
				if (estrcmp(rules->edgelistR[dindex], pt) != 0) changed |= RULECHANGEEDGESPAR;
				(void)reallocstring(&rules->edgelistR[dindex], pt, el_tempcluster);
			}
			continue;
		}
		if (itemHit == DDRR_WIDELIMIT)
		{
			pt = DiaGetText(dia, DDRR_WIDELIMIT);
			dist = atofr(pt);
			if (rules->widelimit != dist) changed |= RULECHANGEWIDLIMIT;
			rules->widelimit = dist;
			continue;
		}
		if (itemHit == DDRR_MINWIDTH)
		{
			/* typed into min-width field */
			pt = DiaGetText(dia, DDRR_MINWIDTH);
			while (*pt == ' ' || *pt == '\t') pt++;
			if (*pt == 0) dist = -WHOLE; else
				dist = atofr(pt);
			if (DiaGetControl(dia, DDRR_SHOWLAYERS) == 0)
			{
				/* set the node minimum width */
				i = DiaGetCurLine(dia, DDRR_FROMLAYER);
				if (rules->minnodesize[i*2] != dist) changed |= RULECHANGEMINSIZE;
				rules->minnodesize[i*2] = dist;
			} else
			{
				/* set the layer minimum width */
				layer1 = dr_rulesdialoggetlayer(rules, DDRR_FROMLAYER, dia);
				if (rules->minwidth[layer1] != dist) changed |= RULECHANGEMINWID;
				rules->minwidth[layer1] = dist;
			}
			continue;
		}
		if (itemHit == DDRR_MINHEIGHT)
		{
			/* typed into min-width field */
			pt = DiaGetText(dia, DDRR_MINHEIGHT);
			while (*pt == ' ' || *pt == '\t') pt++;
			if (*pt == 0) dist = -WHOLE; else
				dist = atofr(pt);

			/* set the node minimum height */
			i = DiaGetCurLine(dia, DDRR_FROMLAYER);
			if (rules->minnodesize[i*2+1] != dist) changed |= RULECHANGEMINSIZE;
			rules->minnodesize[i*2+1] = dist;
			continue;
		}
		if (itemHit == DDRR_MINWIDTHR)
		{
			/* typed into min-width rule field */
			pt = DiaGetText(dia, DDRR_MINWIDTHR);
			while (*pt == ' ' || *pt == '\t') pt++;
			if (DiaGetControl(dia, DDRR_SHOWLAYERS) == 0)
			{
				/* set the node minimum width */
				i = DiaGetCurLine(dia, DDRR_FROMLAYER);
				if (estrcmp(rules->minnodesizeR[i], pt) != 0) changed |= RULECHANGEMINSIZER;
				(void)reallocstring(&rules->minnodesizeR[i], pt, el_tempcluster);
			} else
			{
				/* set the layer minimum width */
				layer1 = dr_rulesdialoggetlayer(rules, DDRR_FROMLAYER, dia);
				if (estrcmp(rules->minwidthR[layer1], pt) != 0) changed |= RULECHANGEMINWIDR;
				(void)reallocstring(&rules->minwidthR[layer1], pt, el_tempcluster);
			}
			continue;
		}
		if (itemHit == DDRR_FROMLAYER)
		{
			/* changed layer popup */
			dr_loaddrcdialog(rules, validlayers, dia);
			dr_loaddrcdistance(rules, dia);
			continue;
		}
		if (itemHit == DDRR_TOLAYER)
		{
			/* clicked on new entry in scroll list of "to" layers */
			dr_loaddrcdistance(rules, dia);
			continue;
		}
		if (itemHit == DDRR_VALIDRULES)
		{
			DiaSetControl(dia, itemHit, 1-DiaGetControl(dia, itemHit));
			dr_loaddrcdialog(rules, validlayers, dia);
			dr_loaddrcdistance(rules, dia);
			continue;
		}
	}

	DiaDoneDialog(dia);

	if (itemHit != OK) changed = 0;
	if (itemHit == DDRR_FACTORYRST)
	{
		/* "factory reset" was clicked, start by deleting all DRC variables */
		for(j=0; dr_variablenames[j] != 0; j++)
		{
			if (getval((INTBIG)tech, VTECHNOLOGY, -1, dr_variablenames[j]) == NOVARIABLE) continue;
			(void)delval((INTBIG)tech, VTECHNOLOGY, dr_variablenames[j]);
		}

		/* set all technology-defined DRC variables */
		for(i=0; tech->variables[i].name != 0; i++)
		{
			varname = tech->variables[i].name;
			for(j=0; dr_variablenames[j] != 0; j++)
				if (namesame(varname, dr_variablenames[j]) == 0) break;
			if (dr_variablenames[j] == 0) continue;

			/* reset the variable */
			if (setval((INTBIG)tech, VTECHNOLOGY, tech->variables[i].name,
				(INTBIG)tech->variables[i].value, tech->variables[i].type & ~VDONTSAVE) == NOVARIABLE) break;
		}

		/* call the technology-specific routine to do any extra work */
		(void)asktech(tech, x_("factory-reset"));

		/* also clear valid DRC dates */
		dr_reset_dates();
	}
	efree((CHAR *)validlayers);
	return(changed);
}

void dr_loaddrcruleobjects(DRCRULES *rules, INTBIG *validlayers, void *dia)
{
	REGISTER INTBIG i;

	DiaLoadTextDialog(dia, DDRR_FROMLAYER, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0);
	if (DiaGetControl(dia, DDRR_SHOWLAYERS) == 0)
	{
		/* list the nodes */
		for(i=0; i<rules->numnodes; i++)
			DiaStuffLine(dia, DDRR_FROMLAYER, rules->nodenames[i]);
		DiaUnDimItem(dia, DDRR_MINHEIGHT_L);
		DiaUnDimItem(dia, DDRR_MINHEIGHT);

		DiaDimItem(dia, DDRR_CONDIST);
		DiaDimItem(dia, DDRR_CONDISTR);
		DiaDimItem(dia, DDRR_UCONDIST);
		DiaDimItem(dia, DDRR_UCONDISTR);
		DiaDimItem(dia, DDRR_CONDISTW);
		DiaDimItem(dia, DDRR_CONDISTWR);
		DiaDimItem(dia, DDRR_UCONDISTW);
		DiaDimItem(dia, DDRR_UCONDISTWR);
		DiaDimItem(dia, DDRR_CONDISTM);
		DiaDimItem(dia, DDRR_CONDISTMR);
		DiaDimItem(dia, DDRR_UCONDISTM);
		DiaDimItem(dia, DDRR_UCONDISTMR);
		DiaDimItem(dia, DDRR_WIDELIMIT);
		DiaDimItem(dia, DDRR_EDGEDIST);
		DiaDimItem(dia, DDRR_EDGEDISTR);
		DiaDimItem(dia, DDRR_VALIDRULES);
		DiaDimItem(dia, DDRR_NORMDIST_L);
		DiaDimItem(dia, DDRR_CONDIST_L);
		DiaDimItem(dia, DDRR_UCONDIST_L);
		DiaDimItem(dia, DDRR_EDGEDIST_L);
		DiaDimItem(dia, DDRR_WIDEDIST_L);
		DiaDimItem(dia, DDRR_CONDISTW_L);
		DiaDimItem(dia, DDRR_UCONDISTW_L);
		DiaDimItem(dia, DDRR_MULTIDIST_L);
		DiaDimItem(dia, DDRR_CONDISTM_L);
		DiaDimItem(dia, DDRR_UCONDISTM_L);
		DiaDimItem(dia, DDRR_DIST_L);
		DiaDimItem(dia, DDRR_DISTR_L);
		DiaDimItem(dia, DDRR_TOLAYER_L);
		DiaDimItem(dia, DDRR_TOLAYER);
	} else
	{
		/* list the layers */
		for(i=0; i<rules->numlayers; i++)
		{
			if (validlayers[i] == 0) continue;
			DiaStuffLine(dia, DDRR_FROMLAYER, rules->layernames[i]);
		}
		DiaDimItem(dia, DDRR_MINHEIGHT_L);
		DiaDimItem(dia, DDRR_MINHEIGHT);

		DiaUnDimItem(dia, DDRR_CONDIST);
		DiaUnDimItem(dia, DDRR_CONDISTR);
		DiaUnDimItem(dia, DDRR_UCONDIST);
		DiaUnDimItem(dia, DDRR_UCONDISTR);
		DiaUnDimItem(dia, DDRR_CONDISTW);
		DiaUnDimItem(dia, DDRR_CONDISTWR);
		DiaUnDimItem(dia, DDRR_UCONDISTW);
		DiaUnDimItem(dia, DDRR_UCONDISTWR);
		DiaUnDimItem(dia, DDRR_CONDISTM);
		DiaUnDimItem(dia, DDRR_CONDISTMR);
		DiaUnDimItem(dia, DDRR_UCONDISTM);
		DiaUnDimItem(dia, DDRR_UCONDISTMR);
		DiaUnDimItem(dia, DDRR_WIDELIMIT);
		DiaUnDimItem(dia, DDRR_EDGEDIST);
		DiaUnDimItem(dia, DDRR_EDGEDISTR);
		DiaUnDimItem(dia, DDRR_VALIDRULES);
		DiaUnDimItem(dia, DDRR_NORMDIST_L);
		DiaUnDimItem(dia, DDRR_CONDIST_L);
		DiaUnDimItem(dia, DDRR_UCONDIST_L);
		DiaUnDimItem(dia, DDRR_EDGEDIST_L);
		DiaUnDimItem(dia, DDRR_WIDEDIST_L);
		DiaUnDimItem(dia, DDRR_CONDISTW_L);
		DiaUnDimItem(dia, DDRR_UCONDISTW_L);
		DiaUnDimItem(dia, DDRR_MULTIDIST_L);
		DiaUnDimItem(dia, DDRR_CONDISTM_L);
		DiaUnDimItem(dia, DDRR_UCONDISTM_L);
		DiaUnDimItem(dia, DDRR_DIST_L);
		DiaUnDimItem(dia, DDRR_DISTR_L);
		DiaUnDimItem(dia, DDRR_TOLAYER_L);
		DiaUnDimItem(dia, DDRR_TOLAYER);
	}
	DiaSelectLine(dia, DDRR_FROMLAYER, 0);
}

/*
 * Routine to show the detail on the selected layer/node in the upper scroll area.
 */
void dr_loaddrcdialog(DRCRULES *rules, INTBIG *validlayers, void *dia)
{
	REGISTER INTBIG i, j, layer1, layer2, temp, dindex, onlyvalid, count;
	REGISTER CHAR *line;

	if (DiaGetControl(dia, DDRR_SHOWLAYERS) == 0)
	{
		/* show node information */
		j = DiaGetCurLine(dia, DDRR_FROMLAYER);
		if (j < 0) return;
		DiaSetText(dia, DDRR_MINWIDTH, frtoa(rules->minnodesize[j*2]));
		DiaSetText(dia, DDRR_MINHEIGHT, frtoa(rules->minnodesize[j*2+1]));
		DiaSetText(dia, DDRR_MINWIDTHR, rules->minnodesizeR[j]);
	} else
	{
		/* show layer information */
		onlyvalid = DiaGetControl(dia, DDRR_VALIDRULES);
		j = dr_rulesdialoggetlayer(rules, DDRR_FROMLAYER, dia);
		if (rules->minwidth[j] < 0) DiaSetText(dia, DDRR_MINWIDTH, x_("")); else
			DiaSetText(dia, DDRR_MINWIDTH, frtoa(rules->minwidth[j]));
		DiaSetText(dia, DDRR_MINWIDTHR, rules->minwidthR[j]);
		DiaLoadTextDialog(dia, DDRR_TOLAYER, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0);
		count = 0;
		for(i=0; i<rules->numlayers; i++)
		{
			if (validlayers[i] == 0) continue;
			layer1 = j;
			layer2 = i;
			if (layer1 > layer2) { temp = layer1; layer1 = layer2;  layer2 = temp; }
			dindex = (layer1+1) * (layer1/2) + (layer1&1) * ((layer1+1)/2);
			dindex = layer2 + rules->numlayers * layer1 - dindex;

			line = dr_loadoptlistline(rules, dindex, i, onlyvalid);
			if (*line == 0) continue;
			DiaStuffLine(dia, DDRR_TOLAYER, line);
			count++;
		}
		if (count > 0) DiaSelectLine(dia, DDRR_TOLAYER, 0);
	}
}

CHAR *dr_loadoptlistline(DRCRULES *rules, INTBIG dindex, INTBIG lindex, INTBIG onlyvalid)
{
	REGISTER CHAR *condist, *uncondist, *wcondist, *wuncondist,
		*mcondist, *muncondist, *edgedist;
	REGISTER void *infstr;

	if (rules->conlist[dindex] < 0) condist = x_(""); else
		condist = frtoa(rules->conlist[dindex]);
	if (rules->unconlist[dindex] < 0) uncondist = x_(""); else
		uncondist = frtoa(rules->unconlist[dindex]);
	if (rules->conlistW[dindex] < 0) wcondist = x_(""); else
		wcondist = frtoa(rules->conlistW[dindex]);
	if (rules->unconlistW[dindex] < 0) wuncondist = x_(""); else
		wuncondist = frtoa(rules->unconlistW[dindex]);
	if (rules->conlistM[dindex] < 0) mcondist = x_(""); else
		mcondist = frtoa(rules->conlistM[dindex]);
	if (rules->unconlistM[dindex] < 0) muncondist = x_(""); else
		muncondist = frtoa(rules->unconlistM[dindex]);
	if (rules->edgelist[dindex] < 0) edgedist = x_(""); else
		edgedist = frtoa(rules->edgelist[dindex]);
	if (onlyvalid != 0)
	{
		if (*condist == 0 && *uncondist == 0 && *wcondist == 0 &&
			*wuncondist == 0 && *mcondist == 0 && *muncondist == 0 &&
			*edgedist == 0) return(x_(""));
	}
	infstr = initinfstr();
	formatinfstr(infstr, x_("%s (%s/%s/%s/%s/%s/%s/%s)"), rules->layernames[lindex],
		condist, uncondist, wcondist, wuncondist, mcondist, muncondist, edgedist);
	return(returninfstr(infstr));
}

INTBIG dr_rulesdialoggetlayer(DRCRULES *rules, INTBIG item, void *dia)
{
	REGISTER INTBIG i, layer;
	REGISTER CHAR *lname, save, *endpos;

	i = DiaGetCurLine(dia, item);
	if (i < 0) return(-1);
	lname = DiaGetScrollLine(dia, item, i);
	for(endpos = lname; *endpos != 0; endpos++)
		if (endpos[0] == ' ' && endpos[1] == '(') break;
	save = *endpos;
	*endpos = 0;
	for(layer=0; layer<rules->numlayers; layer++)
		if (namesame(lname, rules->layernames[layer]) == 0) break;
	*endpos = save;
	if (layer >= rules->numlayers) return(-1);
	return(layer);
}

void dr_loaddrcdistance(DRCRULES *rules, void *dia)
{
	REGISTER INTBIG layer1, layer2, temp, dindex;

	if (DiaGetControl(dia, DDRR_SHOWLAYERS) == 0)
	{
		/* show node information */
	} else
	{
		/* show layer information */
		DiaSetText(dia, DDRR_CONDIST, x_(""));    DiaSetText(dia, DDRR_CONDISTR, x_(""));
		DiaSetText(dia, DDRR_UCONDIST, x_(""));   DiaSetText(dia, DDRR_UCONDISTR, x_(""));
		DiaSetText(dia, DDRR_CONDISTW, x_(""));   DiaSetText(dia, DDRR_CONDISTWR, x_(""));
		DiaSetText(dia, DDRR_UCONDISTW, x_(""));  DiaSetText(dia, DDRR_UCONDISTWR, x_(""));
		DiaSetText(dia, DDRR_CONDISTM, x_(""));   DiaSetText(dia, DDRR_CONDISTMR, x_(""));
		DiaSetText(dia, DDRR_UCONDISTM, x_(""));  DiaSetText(dia, DDRR_UCONDISTMR, x_(""));
		DiaSetText(dia, DDRR_EDGEDIST, x_(""));   DiaSetText(dia, DDRR_EDGEDISTR, x_(""));

		layer1 = dr_rulesdialoggetlayer(rules, DDRR_FROMLAYER, dia);
		if (layer1 < 0) return;
		layer2 = dr_rulesdialoggetlayer(rules, DDRR_TOLAYER, dia);
		if (layer2 < 0) return;

		if (layer1 > layer2) { temp = layer1; layer1 = layer2;  layer2 = temp; }
		dindex = (layer1+1) * (layer1/2) + (layer1&1) * ((layer1+1)/2);
		dindex = layer2 + rules->numlayers * layer1 - dindex;

		if (rules->conlist[dindex] >= 0)
			DiaSetText(dia, DDRR_CONDIST, frtoa(rules->conlist[dindex]));
		if (rules->unconlist[dindex] >= 0)
			DiaSetText(dia, DDRR_UCONDIST, frtoa(rules->unconlist[dindex]));
		if (rules->conlistW[dindex] >= 0)
			DiaSetText(dia, DDRR_CONDISTW, frtoa(rules->conlistW[dindex]));
		if (rules->unconlistW[dindex] >= 0)
			DiaSetText(dia, DDRR_UCONDISTW, frtoa(rules->unconlistW[dindex]));
		if (rules->conlistM[dindex] >= 0)
			DiaSetText(dia, DDRR_CONDISTM, frtoa(rules->conlistM[dindex]));
		if (rules->unconlistM[dindex] >= 0)
			DiaSetText(dia, DDRR_UCONDISTM, frtoa(rules->unconlistM[dindex]));
		if (rules->edgelist[dindex] >= 0)
			DiaSetText(dia, DDRR_EDGEDIST, frtoa(rules->edgelist[dindex]));

		DiaSetText(dia, DDRR_CONDISTR, rules->conlistR[dindex]);
		DiaSetText(dia, DDRR_UCONDISTR, rules->unconlistR[dindex]);
		DiaSetText(dia, DDRR_CONDISTWR, rules->conlistWR[dindex]);
		DiaSetText(dia, DDRR_UCONDISTWR, rules->unconlistWR[dindex]);
		DiaSetText(dia, DDRR_CONDISTMR, rules->conlistMR[dindex]);
		DiaSetText(dia, DDRR_UCONDISTMR, rules->unconlistMR[dindex]);
		DiaSetText(dia, DDRR_EDGEDISTR, rules->edgelistR[dindex]);
	}
}

#endif  /* DRCTOOL - at top */
