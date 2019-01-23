/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrcheck.c
 * User interface tool: database consistency check
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
#include "efunction.h"
#include "tecgen.h"
#include "tecschem.h"
#include "usr.h"
#include "network.h"
#include "sim.h"
#include "drc.h"
#include "eio.h"

extern ARCPROTO **gen_upconn;

typedef struct
{
	INTBIG  type;
	INTBIG  addr;
	CHAR   *name;
} VARLIST;

/* these variables should always have VDONTSAVE set on them */
VARLIST us_temporaryvariables[] =
{
	{VTECHNOLOGY,0,                 x_("DRC_max_distances")},
	{VTECHNOLOGY,0,                 x_("TECH_arc_width_offset")},
	{VTECHNOLOGY,0,                 x_("TECH_node_width_offset")},
	{VTECHNOLOGY,0,                 x_("TECH_layer_names")},
	{VTECHNOLOGY,0,                 x_("TECH_layer_function")},
	{VTECHNOLOGY,0,                 x_("USER_layer_letters")},
	{VLIBRARY,   0,                 x_("LIB_former_version")},
	{VNODEINST,  0,                 x_("NODE_original_name")},
	{VNODEPROTO, 0,                 x_("USER_descent_path")},
	{VNODEPROTO, 0,                 x_("USER_descent_view")},
	{VTOOL,      (INTBIG)&dr_tool, x_("DRC_pointout")},
	{VTOOL,      (INTBIG)&dr_tool, x_("DRC_ignore_list")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_current_constraint")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_have_lisp")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_have_tcl")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_have_java")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_have_cadence")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_have_suntools")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_macro_*")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_highlight_*")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_binding_popup_*")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_windowview_*")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_machine")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_color_map")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_binding_keys")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_binding_buttons")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_binding_buttons")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_current_window")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_current_technology")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_current_node")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_highlighted")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_highlightstack")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_ignore_option_changes")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_interactive_angle")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_macrobuilding")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_macrorunning")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_menu_x")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_menu_y")},
	{VTOOL,      (INTBIG)&us_tool,  x_("USER_menu_position")},
};

/* prototypes for local routines */
static void    us_checklibrary(LIBRARY*, BOOLEAN, INTBIG*, INTBIG*);
static BOOLEAN us_checkvariables(INTBIG, INTBIG, INTSML*, VARIABLE*, LIBRARY*);
static INTBIG  us_checkrtree(RTNODE*, RTNODE*, NODEPROTO*, INTBIG*, INTBIG*);
static BOOLEAN us_checktextdescript(INTBIG, INTBIG, UINTBIG *descript);
static CHAR   *us_describeobject(INTBIG addr, INTBIG type);
static BOOLEAN us_checkforuniquenodenames(NODEPROTO *np);

void us_checkdatabase(BOOLEAN verbose)
{
	REGISTER NODEPROTO *np, *pnt, *lastnp;
	REGISTER NODEINST *ni;
	REGISTER LIBRARY *lib;
	REGISTER INTBIG co, i;
	INTBIG warnings, errors;
	REGISTER TECHNOLOGY *tech;
	REGISTER TOOL *tool;
	REGISTER ARCPROTO *ap;
	REGISTER VIEW *v;

	us_clearhighlightcount();
	errors = warnings = 0;

	if (verbose) ttyputmsg(_("Counting node instances"));

	/* zero the count of each primitive and complex nodeproto */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = 0;
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = 0;
	}

	/* now count the instances in all libraries */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto != NONODEPROTO)
					ni->proto->temp1++;
			}
		}
	}

	if (verbose) ttyputmsg(_("Comparing node instances"));

	/* see if the counts are correct */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		co = 0;  ni = np->firstinst;
		while (ni != NONODEINST) { co++; ni = ni->nextinst; }
		if (co != np->temp1)
		{
			ttyputmsg(_("Technology %s, node %s: says %ld nodes, has %ld"),
				tech->techname, describenodeproto(np), co, np->temp1);
			warnings++;
		}
	}
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		co = 0;  ni = np->firstinst;
		while (ni != NONODEINST) { co++; ni = ni->nextinst; }
		if (co != np->temp1)
		{
			ttyputmsg(_("Cell %s: says %ld nodes, has %ld"), describenodeproto(np), co, np->temp1);
			warnings++;
			lib->userbits |= LIBCHANGEDMAJOR;
		}
	}

	/* re-compute list of node instances if it is bad */
	if (warnings != 0)
	{
		ttyputmsg(_("Repairing instance lists"));

		/* first erase lists at each nodeproto */
		for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
			for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				np->firstinst = NONODEINST;
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				np->firstinst = NONODEINST;

		/* next re-construct lists from every cell in all libraries */
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				pnt = ni->proto;
				if (pnt->firstinst != NONODEINST) pnt->firstinst->previnst = ni;
				ni->nextinst = pnt->firstinst;
				ni->previnst = NONODEINST;
				pnt->firstinst = ni;
			}
		}
	}

	/* check variables in the technologies */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		if (us_checkvariables((INTBIG)tech, VTECHNOLOGY, &tech->numvar, tech->firstvar, NOLIBRARY))
			warnings++;
		for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			if (us_checkvariables((INTBIG)ap, VARCPROTO, &ap->numvar, ap->firstvar, NOLIBRARY))
				warnings++;

		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (us_checkvariables((INTBIG)np, VNODEPROTO, &np->numvar, np->firstvar, NOLIBRARY))
				warnings++;
			if (np->tech != tech)
			{
				ttyputmsg(_("Primitive %s: has wrong technology"), describenodeproto(np));
				np->tech = tech;
				warnings++;
			}
		}
	}

	/* check variables in the tools */
	for(i=0; i<el_maxtools; i++)
	{
		tool = &el_tools[i];
		if (us_checkvariables((INTBIG)tool, VTOOL, &tool->numvar, tool->firstvar, NOLIBRARY))
			warnings++;
	}

	/* check variables in the views */
	for(v = el_views; v != NOVIEW; v = v->nextview)
	{
		if (us_checkvariables((INTBIG)v, VVIEW, &v->numvar, v->firstvar, NOLIBRARY))
			warnings++;
		if ((v->viewstate&MULTIPAGEVIEW) != 0)
		{
			if (namesamen(v->viewname, x_("schematic-page-"), 15) != 0)
			{
				ttyputmsg(_("    View %s should not be multipage"), v->viewname);
				v->viewstate &= ~MULTIPAGEVIEW;
				warnings++;
			}
		} else
		{
			if (namesamen(v->viewname, x_("schematic-page-"), 15) == 0)
			{
				ttyputmsg(_("    View %s should be multipage"), v->viewname);
				v->viewstate |= MULTIPAGEVIEW;
				warnings++;
			}
		}
	}

	/* check nodeproto lists in the libraries */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		lastnp = NONODEPROTO;
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->prevnodeproto != lastnp)
			{
				ttyputmsg(_("Library %s: invalid prevnodeproto"), lib->libname);
				np->prevnodeproto = lastnp;
				warnings++;
				lib->userbits |= LIBCHANGEDMAJOR;
			}
			lastnp = np;
		}
		if (lib->tailnodeproto != lastnp)
		{
			ttyputmsg(_("Library %s: invalid tailnodeproto"), lib->libname);
			lib->tailnodeproto = lastnp;
			warnings++;
			lib->userbits |= LIBCHANGEDMAJOR;
		}
	}

	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
		us_checklibrary(lib, verbose, &warnings, &errors);
	}

	if (errors == 0 && warnings == 0)
	{
		ttyputmsg(_("All libraries checked: no problems found."));
		return;
	}
	if (errors != 0)
	{
		ttyputmsg(_("DATABASE HAS ERRORS THAT CANNOT BE REPAIRED..."));
		ttyputmsg(_("...Try writing the file as a Readable Dump and reading it back"));
		ttyputmsg(_("...Or try copying error-free cells to another library"));
		return;
	}
	ttyputmsg(_("DATABASE HAD ERRORS THAT WERE REPAIRED..."));
	ttyputmsg(_("...Re-issue the check command to make sure"));
}

void us_checklibrary(LIBRARY *curlib, BOOLEAN verbose, INTBIG *warnings, INTBIG *errors)
{
	REGISTER NODEPROTO *np, *onp, *cnp;
	REGISTER PORTPROTO *pp, *opp, *subpp, *lpp;
	REGISTER PORTARCINST *pi, *lpi;
	REGISTER PORTEXPINST *pe, *lpe;
	REGISTER NODEINST *ni, *subni;
	REGISTER ARCINST *ai, *oai, *lastai;
	REGISTER VARIABLE *var, *ovar;
	REGISTER TECHNOLOGY *tech;
	REGISTER NETWORK *net;
	REGISTER VIEW *v;
	REGISTER INTBIG found, foundtot, foundtrue, cellcenters, neterrors, len, accumulatesize,
		totlx, tothx, totly, tothy, truelx, truehx, truely, truehy, i, j, *arr, problems,
		sx, sy, cx, cy, lambda, co, fo, cellgrouptimestamp;
	REGISTER UINTBIG wiped, oldbits;
	REGISTER time_t curdate;
	INTBIG lx, hx, ly, hy, position[2], growx, growy, shiftx, shifty;
	BOOLEAN canscalefonts, rebuildgroups;
	XARRAY trans;
	CHAR newframeinfo[10];
	REGISTER CHAR *ch, *ch2;
	REGISTER void *infstr1, *infstr2;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	curdate = getcurrenttime();

	/* cache font scaling ability */
	canscalefonts = graphicshas(CANSCALEFONTS);

	if (verbose) ttyputmsg(_("***** Checking library %s"), curlib->libname);

	/* check variables in the library */
	if (us_checkvariables((INTBIG)curlib, VLIBRARY, &curlib->numvar, curlib->firstvar, curlib))
	{
		(*warnings)++;
		curlib->userbits |= LIBCHANGEDMAJOR;
	}

	/* make sure counts are correct */
	co = 0;
	for(np = curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto) co++;
	if (co != curlib->numnodeprotos)
	{
		ttyputmsg(_("Library %s: found %ld nodeprotos but records %ld"),
			curlib->libname, co, curlib->numnodeprotos);
		db_buildnodeprotohashtable(curlib);
		(*warnings)++;
		curlib->userbits |= LIBCHANGEDMAJOR;
	}
	if (curlib->nodeprotohashtablesize > 0)
	{
		fo = 0;
		for(np = curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			if (np->newestversion == np) fo++;
		co = 0;
		for (i = 0; i < curlib->nodeprotohashtablesize; i++)
			if (curlib->nodeprotohashtable[i] != NONODEPROTO) co++;
		if (co != fo)
		{
			ttyputmsg(_("Library %s: has %ld cells, but hash table contains %ld"),
				curlib->libname, fo, co);
			db_buildnodeprotohashtable(curlib);
			(*warnings)++;
			curlib->userbits |= LIBCHANGEDMAJOR;
		}
	}

	/* check cell groups */
	for(np = curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		np->temp1 = 0;
	cellgrouptimestamp = 0;
	for(np = curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (np->newestversion != np)
		{
			if (np->nextcellgrp != np)
			{
				ttyputmsg(_("Library %s, cell %s: old version should not be in cell group"),
					curlib->libname, nldescribenodeproto(np));
				(*warnings)++;
				db_removecellfromgroup(np);
				np->nextcellgrp = np;
			}
			continue;
		}

		/* track cell groups */
		if (np->temp1 != 0) continue;

		/* mark those in this cellgroup */
		cellgrouptimestamp++;
		FOR_CELLGROUP(onp, np)
		{
			if (onp->temp1 == cellgrouptimestamp)
			{
				ttyputmsg(_("Library %s: cell %s has loop in cellgroup, NOT REPAIRED"),
					curlib->libname, nldescribenodeproto(np));
				(*errors)++;
				for(onp = curlib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
					ttyputmsg("   Cell %s, NEXTCELLGRP=%s", describenodeproto(onp), describenodeproto(onp->nextcellgrp));
				break;
			}
			if (onp->temp1 != 0)
			{
				ttyputmsg(_("Library %s: cells %s and %s have inconsistent cell groups, NOT REPAIRED"),
					curlib->libname, nldescribenodeproto(np), nldescribenodeproto(onp));
				(*errors)++;
				break;
			}
			onp->temp1 = cellgrouptimestamp;
		}
	}
	for(np = curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (np->newestversion != np)
		{
			if (np->temp1 != 0)
			{
				ttyputmsg(_("Library %s, cell %s: old version should not be in a cellgroup, NOT REPAIRED"),
					curlib->libname, nldescribenodeproto(np));
				(*errors)++;
			}
		} else
		{
			if (np->temp1 == 0)
			{
				ttyputmsg(_("Library %s, cell %s: not in a cellgroup, NOT REPAIRED"),
					curlib->libname, nldescribenodeproto(np));
				(*errors)++;
			}
		}
	}

	/* make sure cell names in groups are sensible */
	rebuildgroups = FALSE;
	for(np = curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		np->temp1 = 0;
	i = 1;
	for(np = curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		FOR_CELLGROUP(onp, np)
		{
			onp->temp1 = i;
		}
		i++;
	}
	for(np = curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (np->newestversion != np)
		{
			/* old version: should be in its own cell group */
			if (np->nextcellgrp != np)
			{
				ttyputmsg(_("Library %s, cell %s is old and should not be in a cell group"),
					curlib->libname, nldescribenodeproto(np));
				rebuildgroups = TRUE;
				(*warnings)++;
			}
			continue;
		}
		for(onp = np->nextnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
		{
			if (onp->newestversion != onp) continue;
			if (namesame(np->protoname, onp->protoname) != 0) continue;
			if (np->temp1 != onp->temp1)
			{
				ttyputmsg(_("Library %s, cells %s and %s should be in the same cell group"),
					curlib->libname, nldescribenodeproto(np), nldescribenodeproto(onp));
				rebuildgroups = TRUE;
				(*warnings)++;
			}
		}
	}
	if (rebuildgroups)
	{
		io_buildcellgrouppointersfromnames(curlib);
	}

	/* check every cell in the library */
	for(np = curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (stopping(STOPREASONCHECK)) return;

		if (verbose) ttyputmsg(_("Checking cell %s"), describenodeproto(np));

		/* check that nodeproto name exists */
		if (*np->protoname == 0)
		{
			ttyputmsg(_("Library %s: null nodeproto name; renamed to 'NULL'"), curlib->libname);
			(void)setval((INTBIG)np, VNODEPROTO, x_("protoname"), (INTBIG)x_("NULL"), VSTRING);
			(*warnings)++;
			curlib->userbits |= LIBCHANGEDMAJOR;
		}

		/* make sure nodeproto name has no blanks, tabs, colons, etc in it */
		found = 0;
		for(ch = np->protoname; *ch != 0; ch++)
		{
			if (*ch <= ' ' || *ch == ':' || *ch == ';' || *ch == '{' ||
				*ch == '}' || *ch >= 0177)
			{
				if (found == 0)
					ttyputmsg(_("Library %s, nodeproto %s: name cannot contain character '%c' (0%o)"),
						curlib->libname, np->protoname, (*ch)&0377, (*ch)&0377);
				found++;
				if (ch == np->protoname) *ch = 'X'; else *ch = '_';
			}
		}
		if (found != 0)
		{
			ttyputmsg(_("...renamed to '%s'"), np->protoname);
			db_buildnodeprotohashtable(curlib);
			(*warnings)++;
			curlib->userbits |= LIBCHANGEDMAJOR;
		}

		/* make sure the library is correct */
		if (np->lib != curlib)
		{
			ttyputmsg(_("Library %s, nodeproto %s: wrong library"), curlib->libname, nldescribenodeproto(np));
			np->lib = curlib;
			(*warnings)++;
			curlib->userbits |= LIBCHANGEDMAJOR;
		}

		/* check that nodeproto is in hash table */
		if (np->newestversion == np && curlib->nodeprotohashtablesize > 0 &&
			db_findnodeprotoname(np->protoname, np->cellview, np->lib) != np)
		{
			ttyputmsg(_("Library %s, nodeproto %s: not in hash table"), curlib->libname, nldescribenodeproto(np));
			db_buildnodeprotohashtable(curlib);
			(*warnings)++;
			curlib->userbits |= LIBCHANGEDMAJOR;
		}

		/* check variables in the cell */
		if (us_checkvariables((INTBIG)np, VNODEPROTO, &np->numvar, np->firstvar, curlib))
		{
			(*warnings)++;
			curlib->userbits |= LIBCHANGEDMAJOR;
		}

		/* check specific variables */
		var = getvalkey((INTBIG)np, VNODEPROTO, VSTRING, el_schematic_page_size_key);
		if (var != NOVARIABLE)
		{
			estrncpy(newframeinfo, (CHAR *)var->addr, 9);
			i = 0;
			if ((newframeinfo[0] < 'a' || newframeinfo[0] > 'e') && newframeinfo[0] != 'h' &&
				newframeinfo[0] != 'x')
			{
				i = 1;
				newframeinfo[0] = 'a';				
			}
			if (newframeinfo[1] != 0 && newframeinfo[1] != 'v')
			{
				i = 1;
				newframeinfo[1] = 0;
			}
			if (newframeinfo[1] == 'v' && newframeinfo[2] != 0)
			{
				i = 1;
				newframeinfo[2] = 0;
			}
			if (i != 0)
			{
				ttyputmsg(_("Cell %s: bad frame size info (was '%s', now '%s')"),
					describenodeproto(np), (CHAR *)var->addr, newframeinfo);
				setvalkey((INTBIG)np, VNODEPROTO, el_schematic_page_size_key,
					(INTBIG)newframeinfo, VSTRING);
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}
		}

		/* make sure the "cell" primindex is zero */
		if (np->primindex != 0)
		{
			ttyputmsg(_("Cell %s: flagged as primitive"), describenodeproto(np));
			np->primindex = 0;
			(*warnings)++;
			curlib->userbits |= LIBCHANGEDMAJOR;
		}

		/* make sure there is no technology */
		tech = whattech(np);
		if (np->tech != tech)
		{
			if (np->tech == NOTECHNOLOGY) ch = _("NOT SET"); else ch = np->tech->techname;
			if (tech == NOTECHNOLOGY) ch2 = _("NOT SET"); else ch2 = tech->techname;
			ttyputmsg(_("Cell %s: has wrong technology (was %s, should be %s)"), describenodeproto(np),
				ch, ch2);
			np->tech = tech;
			(*warnings)++;
			curlib->userbits |= LIBCHANGEDMAJOR;
		}

		/* check the view */
		for(v = el_views; v != NOVIEW; v = v->nextview)
			if (v == np->cellview) break;
		if (v == NOVIEW)
		{
			ttyputmsg(_("Cell %s: invalid view"), describenodeproto(np));
			np->cellview = el_views;
			(*warnings)++;
			curlib->userbits |= LIBCHANGEDMAJOR;
		}

		/* check the dates */
		if ((time_t)np->revisiondate > curdate)
		{
			ttyputmsg(_("Cell %s: invalid revision date (%s)"), describenodeproto(np),
				timetostring((time_t)np->revisiondate));
			np->revisiondate = curdate;
			(*warnings)++;
			curlib->userbits |= LIBCHANGEDMAJOR;
		}
		if ((time_t)np->creationdate > curdate)
		{
			ttyputmsg(_("Cell %s: invalid creation date (%s)"), describenodeproto(np),
				timetostring((time_t)np->creationdate));
			np->creationdate = curdate;
			(*warnings)++;
			curlib->userbits |= LIBCHANGEDMAJOR;
		}

		/* check the port protos on this cell */
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			/* make sure port name has no blanks, tabs, etc in it */
			found = 0;
			for(ch = pp->protoname; *ch != 0; ch++)
			{
				if (*ch <= ' ' || *ch >= 0177)
				{
					if (found == 0)
					{
						ttyputmsg(_("Cell %s, export %s: bad port name (has character 0%o)"),
							describenodeproto(np), pp->protoname, (*ch)&0377);
						(*warnings)++;
						curlib->userbits |= LIBCHANGEDMAJOR;
					}
					found++;
					*ch = 'X';
				}
			}
			if (found != 0)
			{
				ttyputmsg(_("...renamed to '%s'"), pp->protoname);
				db_buildportprotohashtable(np);
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
				break;
			}

			/* make sure port name exists */
			if (*pp->protoname == 0)
			{
				ttyputmsg(_("Cell %s: null port name, renamed to 'NULL'"), describenodeproto(np));
				(void)allocstring(&pp->protoname, x_("NULL"), np->lib->cluster);
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* make sure port name is unique */
			for(;;)
			{
				for(opp = np->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
					if (opp != pp)
				{
					if (namesame(pp->protoname, opp->protoname) != 0) continue;
					ttyputmsg(_("Cell %s, export %s: duplicate name"),
						describenodeproto(np), pp->protoname);
					(void)reallocstring(&pp->protoname, us_uniqueportname(pp->protoname, np),
						np->lib->cluster);
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
					break;
				}
				if (opp == NOPORTPROTO) break;
			}

			/* make sure subnodeinst of this port exists in cell */
			for(subni = np->firstnodeinst; subni != NONODEINST; subni = subni->nextnodeinst)
				if (subni == pp->subnodeinst) break;
			if (subni == NONODEINST)
			{
				ttyputmsg(_("Cell %s, export %s: subnode not there"), describenodeproto(np),
					pp->protoname);

				/* delete the export */
				lpp = NOPORTPROTO;
				for(opp = np->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
				{
					if (opp == pp) break;
					lpp = opp;
				}
				if (lpp == NOPORTPROTO) np->firstportproto = pp->nextportproto; else
					lpp->nextportproto = pp->nextportproto;
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
				continue;
			}

			/* make sure the port's node has an export pointer */
			for(pe = pp->subnodeinst->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
				if (pe == pp->subportexpinst) break;
			if (pe == NOPORTEXPINST)
			{
				ttyputmsg(_("Cell %s, export %s: not in node list"),
					describenodeproto(np), pp->protoname);
				pe = allocportexpinst(curlib->cluster);
				pe->nextportexpinst = subni->firstportexpinst;
				subni->firstportexpinst = pe;
				pe->proto = pp->subportproto;
				pe->exportproto = pp;
				pp->subportexpinst = pe;

				for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				{
					if (ni == pp->subnodeinst) continue;
					lpe = NOPORTEXPINST;
					for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
					{
						if (pe->exportproto == pp)
						{
							if (lpe == NOPORTEXPINST) ni->firstportexpinst = pe->nextportexpinst; else
								lpe->nextportexpinst = pe->nextportexpinst;
							continue;
						}
						lpe = pe;
					}
				}
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* make sure port on subnodeinst is correct */
			for(opp = subni->proto->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
				if (opp == pp->subportproto) break;
			if (opp == NOPORTPROTO)
			{
				ttyputmsg(_("Cell %s, export %s: subportproto not there"), describenodeproto(np),
					pp->protoname);
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
				if (subni->proto->firstportproto == NOPORTPROTO)
				{
					/* subnodeinst has no ports: delete this export */
					lpp = NOPORTPROTO;
					for(opp = np->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
					{
						if (opp == pp) break;
						lpp = opp;
					}
					if (lpp == NOPORTPROTO)
						np->firstportproto = pp->nextportproto; else
							lpp->nextportproto = pp->nextportproto;
					continue;
				} else
				{
					/* set the correct export */
					pp->subportproto = pp->subportexpinst->proto;
				}
			}

			/* make sure this port has valid connections */
			if (pp->connects == 0)
			{
				ttyputmsg(_("Cell %s, export %s: no connection list"), describenodeproto(np),
					pp->protoname);
				if (gen_upconn[0] != 0) pp->connects = gen_upconn; else
					pp->connects = &gen_upconn[1];
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* make sure this port has same arc connections as sub-port */
			if (pp->subportproto != NOPORTPROTO &&
				pp->connects != pp->subportproto->connects)
			{
				infstr1 = initinfstr();
				if (pp->connects == &gen_upconn[1]) addstringtoinfstr(infstr1, x_("ALL-PORTS")); else
				{
					for(i=0; pp->connects[i] != NOARCPROTO; i++)
					{
						if (i != 0) addstringtoinfstr(infstr1, x_(","));
						addstringtoinfstr(infstr1, pp->connects[i]->protoname);
					}
				}
				infstr2 = initinfstr();
				if (pp->subportproto->connects == &gen_upconn[1]) addstringtoinfstr(infstr2, x_("ALL-PORTS")); else
				{
					for(i=0; pp->subportproto->connects[i] != NOARCPROTO; i++)
					{
						if (i != 0) addstringtoinfstr(infstr2, x_(","));
						addstringtoinfstr(infstr2, pp->subportproto->connects[i]->protoname);
					}
				}
				ttyputmsg(_("Cell %s, export %s: connects to %s; should connect to %s"),
					describenodeproto(np), pp->protoname,
						returninfstr(infstr1), returninfstr(infstr2));
				pp->connects = pp->subportproto->connects;
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* make sure parent information in port is right */
			if (pp->parent != np)
			{
				ttyputmsg(_("Cell %s, export %s: bad parent"), describenodeproto(np), pp->protoname);
				pp->parent = np;
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* check the text descriptor */
			if (us_checktextdescript((INTBIG)pp, VPORTPROTO, pp->textdescript))
			{
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* make sure userbits of parent matches that of child */
			i = PORTANGLE | PORTARANGE | PORTISOLATED;
			for(subpp = pp->subportproto, subni = pp->subnodeinst; subni->proto->primindex == 0;
				subni = subpp->subnodeinst, subpp = subpp->subportproto) {}
			if ((subpp->userbits&i) != (pp->userbits&i))
			{
				j = (pp->userbits & ~i) | (subpp->userbits & i);
				ttyputmsg(_("Cell %s, export %s: state bits are 0%o, should be 0%o"),
					describenodeproto(np), pp->protoname, pp->userbits, j);
				pp->userbits = j;
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* ports on invisible pins must not have an offset */
			if (pp->subnodeinst->proto == gen_invispinprim)
			{
				if (TDGETXOFF(pp->textdescript) != 0 || TDGETYOFF(pp->textdescript) != 0)
				{
					TDSETOFF(pp->textdescript, 0, 0);
					ttyputmsg(_("Cell %s, export %s: should not have offset (on invisible pin)"),
						describenodeproto(np), pp->protoname);
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
				}
			}

			/* ports on wire pins cannot exist if instances of the cell are arrayed */
			if (pp->subnodeinst->proto == sch_wirepinprim)
			{
				for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
					if (ni->arraysize > 1) break;
				if (ni != NONODEINST)
				{
					ttyputmsg(_("Cell %s, export %s: should be on bus pin, not wire pin (instances are arrayed)"),
						describenodeproto(np), pp->protoname);
					subni = replacenodeinst(pp->subnodeinst, sch_buspinprim, FALSE, FALSE);
					if (subni != NONODEINST)
						us_computenodefartextbit(subni);
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
				}
			}

			/* check that port is in port hash table */
			if (np->portprotohashtablesize > 0 && getportproto(np, pp->protoname) != pp)
			{
				ttyputmsg(_("Cell %s, export %s: not in portproto hash"),
					describenodeproto(np), pp->protoname);
				db_buildportprotohashtable(np);
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* check variables in the port */
			if (us_checkvariables((INTBIG)pp, VPORTPROTO, &pp->numvar, pp->firstvar, curlib))
			{
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}
		}

		/* check portproto list */
		co = 0;
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto) co++;
		if (co != np->numportprotos)
		{
			ttyputmsg(_("Cell %s: found %ld portprotos but records %ld"),
				describenodeproto(np), co, np->numportprotos);
			db_buildportprotohashtable(np);
			(*warnings)++;
			curlib->userbits |= LIBCHANGEDMAJOR;
		}
		if (np->portprotohashtablesize > 0)
		{
			co = 0;
			for (i=0; i<np->portprotohashtablesize; i++)
				if (np->portprotohashtable[i] != NOPORTPROTO) co++;
			if (co != np->numportprotos)
			{
				ttyputmsg(_("Cell %s: numcell=%ld in hash=%ld"),
					describenodeproto(np), np->numportprotos, co);
				db_buildportprotohashtable(np);
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}
		}

		/* if this is an icon cell, check port equivalences to the layout */
		if (np->cellview == el_iconview)
		{
			/* do not consider unused old versions */
			if (np->firstinst != NONODEINST || np->newestversion == np)
			{
				onp = contentsview(np);
				if (onp != NONODEPROTO)
				{
					/* mark each port on the contents */
					for(opp = onp->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
						opp->temp1 = 0;

					/* look at each port on the icon */
					for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
					{
						/* see if there is an equivalent in the contents */
						for(opp = onp->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
							if (namesame(opp->protoname, pp->protoname) == 0)
						{
							opp->temp1 = 1;
							break;
						}
						if (opp == NOPORTPROTO)
						{
							ttyputmsg(_("Cell %s, export %s: no equivalent in cell %s, NOT REPAIRED"),
								describenodeproto(np), pp->protoname, describenodeproto(onp));
							(*errors)++;
						} else
						{
							/* make sure the characteristics match */
							if ((pp->userbits&STATEBITS) != (opp->userbits&STATEBITS))
							{
								ttyputmsg(_("Cell %s, export %s: characteristics differ from port in cell %s"),
									describenodeproto(np), pp->protoname, describenodeproto(onp));
								pp->userbits = (pp->userbits & ~STATEBITS) | (opp->userbits&STATEBITS);
								(*warnings)++;
								curlib->userbits |= LIBCHANGEDMAJOR;
							}
						}
					}
					for(opp = onp->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
						if (opp->temp1 == 0)
					{
						if ((opp->userbits&BODYONLY) == 0)
						{
							ttyputmsg(_("Cell %s, export %s: no equivalent in icon, NOT REPAIRED"),
								describenodeproto(onp), opp->protoname);
							(*errors)++;
						}
					}
				}
			}
		}
	}

	/* check geometry modules, nodes, and arcs for sensibility */
	for(np = curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (stopping(STOPREASONCHECK)) return;

		/* initialize cell bounds */
		foundtot = foundtrue = 0;
		totlx = tothx = totly = tothy = 0;
		truelx = truehx = truely = truehy = 0;
		lambda = lambdaofcell(np);

		if (verbose) ttyputmsg(_("Checking cell %s"), describenodeproto(np));

		/* check every nodeinst in the cell */
		cellcenters = 0;
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if (ni->proto == 0 || ni->proto == NONODEPROTO)
			{
				ttyputmsg(_("Cell %s: node instance with no prototype"), describenodeproto(np));
				ni->proto = gen_univpinprim;
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* see if a "cell-center" primitive is in the cell */
			if (ni->proto == gen_cellcenterprim)
			{
				position[0] = (ni->highx+ni->lowx) / 2;
				position[1] = (ni->highy+ni->lowy) / 2;
				if (cellcenters == 1)
				{
					ttyputmsg(_("Cell %s: multiple cell-center primitives"), describenodeproto(np));
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
				}
				cellcenters++;

				/* check the center information on the parent cell */
				var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER|VISARRAY, el_prototype_center_key);
				if (var == NOVARIABLE)
				{
					ttyputmsg(_("Cell %s: cell-center not recorded"), describenodeproto(np));
					(void)setvalkey((INTBIG)np, VNODEPROTO, el_prototype_center_key,
						(INTBIG)position, VINTEGER|VISARRAY|(2<<VLENGTHSH));
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
				} else if (((INTBIG *)var->addr)[0] != position[0] ||
					((INTBIG *)var->addr)[1] != position[1])
				{
					ttyputmsg(_("Cell %s: cell-center not properly recorded"), describenodeproto(np));
					(void)setvalkey((INTBIG)np, VNODEPROTO, el_prototype_center_key,
						(INTBIG)position, VINTEGER|VISARRAY|(2<<VLENGTHSH));
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
				}
			}

			/* check size of this nodeinst */
			if (ni->proto->primindex == 0)
			{
				/* instances of cells must be the size of the cells */
				if (ni->highx-ni->lowx != ni->proto->highx-ni->proto->lowx ||
					ni->highy-ni->lowy != ni->proto->highy-ni->proto->lowy)
				{
					ttyputmsg(_("Cell %s, node %s: node size was %sx%s, should be %sx%s"),
						describenodeproto(np), describenodeinst(ni), latoa(ni->highx-ni->lowx, lambda),
							latoa(ni->highy-ni->lowy, lambda), latoa(ni->proto->highx-ni->proto->lowx, lambda),
								latoa(ni->proto->highy-ni->proto->lowy, lambda));
					ni->lowx += ((ni->highx-ni->lowx) - (ni->proto->highx-ni->proto->lowx))/2;
					ni->highx = ni->lowx + ni->proto->highx-ni->proto->lowx;
					ni->lowy += ((ni->highy-ni->lowy) - (ni->proto->highy-ni->proto->lowy))/2;
					ni->highy = ni->lowy + ni->proto->highy-ni->proto->lowy;
					updategeom(ni->geom, np);
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
				}

				/* check text descriptor on these instances */
				if (us_checktextdescript((INTBIG)ni, VNODEINST, ni->textdescript))
				{
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
				}
			} else
			{
				/* primitive nodeinst must have positive size */
				if (ni->lowx > ni->highx || ni->lowy > ni->highy)
				{
					ttyputmsg(_("Cell %s, node %s: strange dimensions"), describenodeproto(np),
						describenodeinst(ni));
					if (ni->lowx > ni->highx)
					{
						i = ni->lowx;   ni->lowx = ni->highx;   ni->highx = i;
					}
					if (ni->lowy > ni->highy)
					{
						i = ni->lowy;   ni->lowy = ni->highy;   ni->highy = i;
					}
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
				}
				nodesizeoffset(ni, &lx, &ly, &hx, &hy);
				sx = ni->highx - ni->lowx - lx - hx;
				sy = ni->highy - ni->lowy - ly - hy;
				if (sx < 0 || sy < 0)
				{
					ttyputmsg(_("Cell %s, node %s: negative size"), describenodeproto(np),
						describenodeinst(ni));
					sx = ni->proto->highx - ni->proto->lowx;
					sy = ni->proto->highy - ni->proto->lowy;
					cx = (ni->lowx + ni->highx) / 2;
					cy = (ni->lowy + ni->highy) / 2;
					ni->lowx = cx - sx/2;   ni->highx = ni->lowx + sx;
					ni->lowy = cy - sy/2;   ni->highy = ni->lowy + sy;
					updategeom(ni->geom, np);
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
				}

				/* check trace information for validity */
				var = gettrace(ni);
				if (var != NOVARIABLE)
				{
					/* make sure the primitive can hold trace information */
					if ((ni->proto->userbits&HOLDSTRACE) == 0)
					{
						ttyputmsg(_("Cell %s, node %s: cannot hold outline information"),
							describenodeproto(np), describenodeinst(ni));
						nextchangequiet();
						(void)delvalkey((INTBIG)ni, VNODEINST, el_trace_key);
						(*warnings)++;
						curlib->userbits |= LIBCHANGEDMAJOR;
					} else
					{
						/* ensure points in different locations */
						len = getlength(var) / 2;
						arr = (INTBIG *)var->addr;
						for(i=1; i<len; i++)
							if (arr[0] != arr[i*2] || arr[1] != arr[i*2+1]) break;
						if (i >= len || len < 2)
						{
							ttyputmsg(_("Cell %s, node %s: zero-size outline (has %ld points)"),
								describenodeproto(np), describenodeinst(ni), len);
							nextchangequiet();
							(void)delvalkey((INTBIG)ni, VNODEINST, el_trace_key);
							(*warnings)++;
							curlib->userbits |= LIBCHANGEDMAJOR;
						}
					}
				}

				/* check "WIPED" bit in userbits */
				wiped = us_computewipestate(ni);
				if (wiped != (ni->userbits&WIPED))
				{
					ttyputmsg(_("Cell %s, node %s: incorrect visibility"),
						describenodeproto(np), describenodeinst(ni));
					ni->userbits = (ni->userbits & ~WIPED) | wiped;
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
				}
			}

			/* check orientation of this nodeinst */
			if ((ni->transpose != 0 && ni->transpose != 1) ||
				(ni->rotation < 0 || ni->rotation >= 3600))
			{
				ttyputmsg(_("Cell %s, node %s: strange orientation"), describenodeproto(np),
					describenodeinst(ni));
				if (ni->transpose != 0) ni->transpose = 1;
				ni->rotation = ni->rotation % 3600;
				if (ni->rotation < 0) ni->rotation += 3600;
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* check the arc connections on this nodeinst */
			for(lpi = NOPORTARCINST, pi = ni->firstportarcinst; pi != NOPORTARCINST;
				pi = pi->nextportarcinst)
			{
				pp = pi->proto;
				if (pp == NOPORTPROTO)
				{
					ttyputmsg(_("Cell %s, node %s: no portarc prototype"), describenodeproto(np),
						describenodeinst(ni));
					if (lpi == NOPORTARCINST)
						ni->firstportarcinst = pi->nextportarcinst; else
							lpi->nextportarcinst = pi->nextportarcinst;
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
					continue;
				}

				/* make sure the port prototype is correct */
				for(opp = ni->proto->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
					if (opp == pp) break;
				if (opp == NOPORTPROTO)
				{
					ttyputmsg(_("Cell %s, node %s, port %s: arc proto bad"), describenodeproto(np),
						describenodeinst(ni), pp->protoname);
					if (lpi == NOPORTARCINST)
						ni->firstportarcinst = pi->nextportarcinst; else
							lpi->nextportarcinst = pi->nextportarcinst;
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
					continue;
				}

				/* make sure connecting arcinst exists */
				if (pi->conarcinst == NOARCINST)
				{
					ttyputmsg(_("Cell %s, node %s, port %s: no arc"), describenodeproto(np),
						describenodeinst(ni), pp->protoname);
					if (lpi == NOPORTARCINST)
						ni->firstportarcinst = pi->nextportarcinst; else
							lpi->nextportarcinst = pi->nextportarcinst;
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
					continue;
				}

				/* make sure the arc is in the cell */
				for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
					if (ai == pi->conarcinst) break;
				if (ai == NOARCINST)
				{
					ttyputmsg(_("Cell %s, node %s, port %s: arc not in cell"), describenodeproto(np),
						describenodeinst(ni), pp->protoname);
					if (lpi == NOPORTARCINST)
						ni->firstportarcinst = pi->nextportarcinst; else
							lpi->nextportarcinst = pi->nextportarcinst;
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
					continue;
				}
				lpi = pi;

				/* check variables in the portarcinst */
				if (us_checkvariables((INTBIG)pi, VPORTARCINST, &pi->numvar, pi->firstvar, curlib))
				{
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
				}
			}

			/* check that the portarcinsts are in the proper sequence */
			pp = ni->proto->firstportproto;
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			{
				while (pp != pi->proto && pp != NOPORTPROTO)
					pp = pp->nextportproto;
				if (pp == NOPORTPROTO)
				{
					ttyputmsg(_("Cell %s, node %s: portarcinst out of order"),
						describenodeproto(np), describenodeinst(ni));
					/* for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
						ttyputmsg(_("   Port %s"), pi->proto->protoname); */
					pi = ni->firstportarcinst;
					ni->firstportarcinst = NOPORTARCINST;
					while (pi != NOPORTARCINST)
					{
						lpi = pi;
						pi = pi->nextportarcinst;
						db_addportarcinst(ni, lpi);
					}
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
					break;
				}
			}

			/* check the export connections on this nodeinst */
			for(lpe = NOPORTEXPINST, pe = ni->firstportexpinst; pe != NOPORTEXPINST;
				pe = pe->nextportexpinst)
			{
				pp = pe->proto;
				if (pp == NOPORTPROTO)
				{
					ttyputmsg(_("Cell %s, node %s: no portexp prototype"),
						describenodeproto(np), describenodeinst(ni));
					if (lpe == NOPORTEXPINST)
						ni->firstportexpinst = pe->nextportexpinst; else
							lpe->nextportexpinst = pe->nextportexpinst;
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
					continue;
				}

				/* make sure the port prototype is correct */
				for(opp = ni->proto->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
					if (opp == pp) break;
				if (opp == NOPORTPROTO)
				{
					ttyputmsg(_("Cell %s, node %s, port %s: exp proto bad"), describenodeproto(np),
						describenodeinst(ni), pp->protoname);
					if (lpe == NOPORTEXPINST)
						ni->firstportexpinst = pe->nextportexpinst; else
							lpe->nextportexpinst = pe->nextportexpinst;
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
					continue;
				}

				/* check validity of port if it is an export */
				if (pe->exportproto == NOPORTPROTO)
				{
					ttyputmsg(_("Cell %s, node %s, port %s: not an export"), describenodeproto(np),
						describenodeinst(ni), pp->protoname);
					if (lpe == NOPORTEXPINST)
						ni->firstportexpinst = pe->nextportexpinst; else
							lpe->nextportexpinst = pe->nextportexpinst;
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
					continue;
				}

				/* make sure exported portinst is in list */
				for(opp = np->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
					if (opp == pe->exportproto) break;
				if (opp == NOPORTPROTO)
				{
					ttyputmsg(_("Cell %s, node %s: export %s bad"), describenodeproto(np),
						describenodeinst(ni), pe->exportproto->protoname);
					if (lpe == NOPORTEXPINST)
						ni->firstportexpinst = pe->nextportexpinst; else
							lpe->nextportexpinst = pe->nextportexpinst;
					lpe = pe->nextportexpinst;
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
					continue;
				}
				lpe = pe;

				/* check variables in the portexpinst */
				if (us_checkvariables((INTBIG)pe, VPORTEXPINST, &pe->numvar, pe->firstvar, curlib))
				{
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
				}
			}

			/* check that the portexpinsts are in the proper sequence */
			pp = ni->proto->firstportproto;
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
			{
				while (pp != pe->proto && pp != NOPORTPROTO)
					pp = pp->nextportproto;
				if (pp == NOPORTPROTO)
				{
					ttyputmsg(_("Cell %s, node %s: export instance out of order"),
						describenodeproto(np), describenodeinst(ni));
					pe = ni->firstportexpinst;
					ni->firstportexpinst = NOPORTEXPINST;
					while (pe != NOPORTEXPINST)
					{
						lpe = pe;
						pe = pe->nextportexpinst;
						db_addportexpinst(ni, lpe);
					}
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
					break;
				}
			}

			/* make sure nodeinst parent is right */
			if (ni->parent != np)
			{
				ttyputmsg(_("Cell %s, node %s: wrong parent"), describenodeproto(np),
					describenodeinst(ni));
				ni->parent = np;
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* make sure geometry module pointer is right */
			ni->temp1 = 0;
			if (ni->geom->entryaddr.ni != ni)
			{
				ttyputmsg(_("Cell %s, node %s: bad geometry module"), describenodeproto(np),
					describenodeinst(ni));
				ni->geom->entryaddr.ni = ni;
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* make sure the geometry module is the right size */
			boundobj(ni->geom, &lx, &hx, &ly, &hy);
			if (lx != ni->geom->lowx || hx != ni->geom->highx ||
				ly != ni->geom->lowy || hy != ni->geom->highy)
			{
				ttyputmsg(_("Cell %s, node %s: geometry size bad (was %s<=X<=%s, %s<=Y<=%s, is %s<=X<=%s, %s<=Y<=%s)"),
					describenodeproto(np), describenodeinst(ni), latoa(ni->geom->lowx, lambda),
						latoa(ni->geom->highx, lambda), latoa(ni->geom->lowy, lambda),
						latoa(ni->geom->highy, lambda), latoa(lx, lambda), latoa(hx, lambda),
						latoa(ly, lambda), latoa(hy, lambda));
				updategeom(ni->geom, np);
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* accumulate cell size information if not a "cell-center" primitive */
			accumulatesize = 1;
			if (ni->proto == gen_cellcenterprim) accumulatesize = 0;
			if (ni->proto == gen_invispinprim)
			{
				for(i=0; i<ni->numvar; i++)
				{
					var = &ni->firstvar[i];
					if ((var->type&VDISPLAY) != 0 &&
						(TDGETINTERIOR(var->textdescript) != 0 ||
							TDGETINHERIT(var->textdescript) != 0))
					{
						accumulatesize = 0;
						break;
					}
				}
			}
			if (accumulatesize != 0)
			{
				if (foundtot == 0)
				{
					totlx = lx;   tothx = hx;   totly = ly;   tothy = hy;
					foundtot = 1;
				} else
				{
					if (lx < totlx) totlx = lx;
					if (hx > tothx) tothx = hx;
					if (ly < totly) totly = ly;
					if (hy > tothy) tothy = hy;
				}
			}
			if (foundtrue == 0)
			{
				truelx = lx;   truehx = hx;   truely = ly;   truehy = hy;
			} else
			{
				if (lx < truelx) truelx = lx;
				if (hx > truehx) truehx = hx;
				if (ly < truely) truely = ly;
				if (hy > truehy) truehy = hy;
			}
			foundtrue = 1;

			/* make sure nodeinst is not marked dead */
			if (ni->userbits&DEADN)
			{
				ttyputmsg(_("Cell %s, node %s: dead node"), describenodeproto(np),
					describenodeinst(ni));
				ni->userbits &= ~DEADN;
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* make sure the "far text" bit is set right */
			if (canscalefonts)
			{
				oldbits = ni->userbits;
				us_computenodefartextbit(ni);
				if (oldbits != ni->userbits)
				{
					if ((oldbits&NHASFARTEXT) == 0)
					{
						ttyputmsg(_("Cell %s, node %s: has distant text, not marked so"),
							describenodeproto(np), describenodeinst(ni));
					} else
					{
						ttyputmsg(_("Cell %s, node %s: does not have distant text, but marked so"),
							describenodeproto(np), describenodeinst(ni));
					}
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
				}
			}

			/* make sure array size is correct */
			i = ni->arraysize;
			net_setnodewidth(ni);
			if (i != ni->arraysize)
			{
				ttyputmsg(_("Cell %s, node %s: incorrect array size (was %ld, should be %ld)"),
					describenodeproto(np), describenodeinst(ni), i, ni->arraysize);
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* check variables in the nodeinst */
			if (us_checkvariables((INTBIG)ni, VNODEINST, &ni->numvar, ni->firstvar, curlib))
			{
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}
			if (us_checkvariables((INTBIG)ni->geom, VGEOM, &ni->geom->numvar, ni->geom->firstvar, curlib))
			{
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* make sure parameters on the node also exist on the cell */
			for(i=0; i<ni->numvar; i++)
			{
				var = &ni->firstvar[i];
				if (TDGETISPARAM(var->textdescript) != 0) break;
			}
			if (i < ni->numvar)
			{
				/* found parameter on the node: check against prototype */
				onp = ni->proto;
				cnp = contentsview(onp);
				if (cnp != NONODEPROTO) onp = cnp;
				for(i=0; i<ni->numvar; i++)
				{
					var = &ni->firstvar[i];
					if (TDGETISPARAM(var->textdescript) == 0) continue;
					for(j=0; j<onp->numvar; j++)
					{
						ovar = &onp->firstvar[j];
						if (TDGETISPARAM(ovar->textdescript) == 0) continue;
						if (ovar->key == var->key) break;
					}
					if (j >= onp->numvar)
					{
						ttyputmsg(_("Cell %s, node %s, variable %s: not a parameter"),
							describenodeproto(np), describenodeinst(ni), makename(var->key));
						(*warnings)++;
						TDSETISPARAM(var->textdescript, 0);
					}
				}
			}

			/* check for null names on nodes */
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
			if (var != NOVARIABLE)
			{
				ch = (CHAR *)var->addr;
				while (*ch == ' ' || *ch == '\t') ch++;
				if (*ch == 0)
				{
					ttyputmsg(_("Cell %s, node %s: empty node name"),
						describenodeproto(np), describenodeinst(ni));
					(*warnings)++;
					delvalkey((INTBIG)ni, VNODEINST, el_node_name_key);
				}
			}
		}

		/* make sure node names are unique */
		if (us_checkforuniquenodenames(np) != 0)
		{
			(*errors)++;
		}

		if (verbose) ttyputmsg(_("   checking arcs"));

		/* check every arcinst in the cell */
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		{
			/* check both arcinst ends */
			for(i=0; i<2; i++)
			{
				/* make sure it has a nodeinst and portarcinst */
				ni = ai->end[i].nodeinst;
				pi = ai->end[i].portarcinst;
				if (ni == NONODEINST && pi != NOPORTARCINST && pi->proto != NOPORTPROTO &&
					pi->proto->connects != 0)
				{
					/* try to repair */
					for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
					{
						for(lpi = ni->firstportarcinst; lpi != NOPORTARCINST; lpi = lpi->nextportarcinst)
							if (lpi == pi) break;
						if (lpi != NOPORTARCINST) break;
					}
					if (ni != NONODEINST)
					{
						ai->end[i].nodeinst = ni;
						ttyputmsg(_("Cell %s, arc %s: bad node end"), describenodeproto(np),
							describearcinst(ai));
						(*warnings)++;
						curlib->userbits |= LIBCHANGEDMAJOR;
					}
				}

				if (ni == NONODEINST || pi == NOPORTARCINST || pi->proto == NOPORTPROTO ||
					pi->proto->connects == 0)
				{
					/* try to repair */
					for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
					{
						for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
						{
							for(j=0; pp->connects[j] != NOARCPROTO; j++)
								if (pp->connects[j] == ai->proto) break;
							if (pp->connects[j] == NOARCPROTO) continue;
							shapeportpoly(ni, pp, poly, FALSE);
							if (isinside(ai->end[i].xpos, ai->end[i].ypos, poly)) break;
						}
						if (pp != NOPORTPROTO)
						{
							ai->end[i].nodeinst = ni;
							if (pi == NOPORTARCINST) pi = allocportarcinst(np->lib->cluster);
							if (pi->proto == NOPORTPROTO) pi->proto = pp;
							if (pi->conarcinst == NOARCINST) pi->conarcinst = ai;
							pi->nextportarcinst = ni->firstportarcinst;
							ni->firstportarcinst = pi;
							ttyputmsg(_("Cell %s, arc %s: bad node end port"), describenodeproto(np),
								describearcinst(ai));
							(*warnings)++;
							curlib->userbits |= LIBCHANGEDMAJOR;
							break;
						}
					}
					if (ni == NONODEINST)
					{
						ttyputmsg(_("Cell %s, arc %s: bad node end, deleting arc"), describenodeproto(np),
							describearcinst(ai));
						lastai = NOARCINST;
						for(oai = np->firstarcinst; oai != NOARCINST; oai = oai->nextarcinst)
						{
							if (oai == ai) break;
							lastai = oai;
						}
						if (lastai == NOARCINST) np->firstarcinst = ai->nextarcinst; else
							lastai->nextarcinst = ai->nextarcinst;
						(*warnings)++;
						curlib->userbits |= LIBCHANGEDMAJOR;
						break;
					}
					continue;
				}
				if (ai->end[i].portarcinst == NOPORTARCINST)
				{
					ttyputmsg(_("Cell %s, arc %s: bad port end, NOT REPAIRED"), describenodeproto(np),
						describearcinst(ai));
					(*errors)++;
					continue;
				}

				/* make sure portarcinst is on nodeinst */
				for(pi = ai->end[i].nodeinst->firstportarcinst; pi != NOPORTARCINST;
					pi = pi->nextportarcinst)
						if (ai->end[i].portarcinst == pi) break;
				if (pi == NOPORTARCINST)
				{
					ai->end[i].portarcinst = allocportarcinst(np->lib->cluster);
					pi = ai->end[i].portarcinst;
					pi->proto = NOPORTPROTO;
					for(pp = ai->end[i].nodeinst->proto->firstportproto; pp != NOPORTPROTO;
						pp = pp->nextportproto)
					{
						for(j=0; pp->connects[j] != NOARCPROTO; j++)
							if (pp->connects[j] == ai->proto) break;
						if (pp->connects[j] == NOARCPROTO) continue;
						pi->proto = pp;
						shapeportpoly(ai->end[i].nodeinst, pp, poly, FALSE);
						if (isinside(ai->end[i].xpos, ai->end[i].ypos, poly)) break;
					}
					if (pi->proto == NOPORTPROTO)
					{
						ttyputmsg(_("Cell %s, arc %s, end %ld: cannot connect, NOT REPAIRED"),
							describenodeproto(np), describearcinst(ai), i);
						(*errors)++;
						break;
					}
					pi->conarcinst = ai;
					db_addportarcinst(ai->end[i].nodeinst, pi);

					ttyputmsg(_("Cell %s, arc %s, end %ld: missing portarcinst"),
						describenodeproto(np), describearcinst(ai), i);
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
				}

				/* make sure nodeinst is not dead */
				if (ai->end[i].nodeinst->userbits&DEADN)
				{
					ttyputmsg(_("Cell %s, arc %s: dead end"), describenodeproto(np),
						describearcinst(ai));
					ai->end[i].nodeinst->userbits &= ~DEADN;
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
				}

				/* make sure the nodeinst resides in this cell */
				for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
					if (ni == ai->end[i].nodeinst) break;
				if (ni == NONODEINST)
				{
					ttyputmsg(_("Cell %s, arc %s, end %ld: no node, NOT REPAIRED"), describenodeproto(np),
						describearcinst(ai), i);
					(*errors)++;
				}

				/* make sure proto of portinst agrees with this arcinst */
				pp = ai->end[i].portarcinst->proto;
				for(j=0; pp->connects[j] != NOARCPROTO; j++)
					if (pp->connects[j] == ai->proto) break;
				if (pp->connects[j] == NOARCPROTO)
				{
					ttyputmsg(_("Cell %s, arc %s: can't connect to port %s"), describenodeproto(np),
						describearcinst(ai), ai->end[i].portarcinst->proto->protoname);

					/* see if it is possible to connect this arc to a different port */
					ni = ai->end[i].nodeinst;
					for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
					{
						for(j=0; pp->connects[j] != NOARCPROTO; j++)
							if (pp->connects[j] == ai->proto) break;
						if (pp->connects[j] == NOARCPROTO) continue;
						shapeportpoly(ni, pp, poly, FALSE);
						if (!isinside(ai->end[i].xpos, ai->end[i].ypos, poly)) continue;
						ai->end[i].portarcinst->proto = pp;
						break;
					}
					if (pp == NOPORTPROTO)
					{
						/* convert the arc to "universal" so that it can connect */
						ai->proto = el_technologies->firstarcproto;
					}
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
				}

				shapeportpoly(ai->end[i].nodeinst, ai->end[i].portarcinst->proto, poly, FALSE);
				if (!isinside(ai->end[i].xpos, ai->end[i].ypos, poly))
				{
					portposition(ai->end[i].nodeinst, ai->end[i].portarcinst->proto, &lx, &ly);
					if (lx != ai->end[i].xpos || ly != ai->end[i].ypos)
					{
						/* try to extend vertically */
						if ((ai->end[0].xpos == ai->end[1].xpos) &&
							isinside(ai->end[i].xpos, ly, poly)) ai->end[i].ypos = ly; else
								if ((ai->end[0].ypos == ai->end[1].ypos) &&
									isinside(lx, ai->end[i].ypos, poly))
										ai->end[i].xpos = lx; else
						{
							ai->end[i].xpos = lx;   ai->end[i].ypos = ly;
						}
						ttyputmsg(_("Cell %s, arc %s: end %ld not in port"), describenodeproto(np),
							describearcinst(ai), i);
						(*warnings)++;
						curlib->userbits |= LIBCHANGEDMAJOR;
					}
				}
			}

			/* make sure geometry modeule pointer is right */
			ai->temp1 = 0;
			if (ai->geom->entryaddr.ai != ai)
			{
				ttyputmsg(_("Cell %s, arc %s: bad geometry module"), describenodeproto(np),
					describearcinst(ai));
				ai->geom->entryaddr.ai = ai;
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* check arcinst length */
			i = computedistance(ai->end[0].xpos, ai->end[0].ypos, ai->end[1].xpos, ai->end[1].ypos);
			if (i != ai->length)
			{
				ttyputmsg(_("Cell %s, arc %s: bad length (was %s)"), describenodeproto(np),
					describearcinst(ai), latoa(ai->length, lambda));
				ai->length = i;
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* check arcinst width */
			if (ai->width < 0)
			{
				ttyputmsg(_("Cell %s, arc %s: negative width"), describenodeproto(np),
					describearcinst(ai));
				ai->width = -ai->width;
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* check arcinst end shrinkage value */
			if (setshrinkvalue(ai, FALSE) != 0)
			{
				ttyputmsg(_("Cell %s, arc %s: bad endshrink value"), describenodeproto(np),
					describearcinst(ai));
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* make sure the geometry module is the right size */
			boundobj(ai->geom, &lx, &hx, &ly, &hy);
			if (lx != ai->geom->lowx || hx != ai->geom->highx ||
				ly != ai->geom->lowy || hy != ai->geom->highy)
			{
				ttyputmsg(_("Cell %s, arc %s: geometry size bad"), describenodeproto(np),
					describearcinst(ai));
				updategeom(ai->geom, np);
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* accumulate cell size information */
			if (foundtrue == 0)
			{
				foundtrue = 1;
				truelx = lx;   truehx = hx;   truely = ly;   truehy = hy;
				truelx = lx;   truehx = hx;   truely = ly;   truehy = hy;
			} else
			{
				if (lx < totlx) totlx = lx;
				if (hx > tothx) tothx = hx;
				if (ly < totly) totly = ly;
				if (hy > tothy) tothy = hy;

				if (lx < truelx) truelx = lx;
				if (hx > truehx) truehx = hx;
				if (ly < truely) truely = ly;
				if (hy > truehy) truehy = hy;
			}

			/* make sure parent pointer is right */
			if (ai->parent != np)
			{
				ttyputmsg(_("Cell %s, arc %s: bad parent pointer"), describenodeproto(np),
					describearcinst(ai));
				ai->parent = np;
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* make sure the arcinst isn't dead */
			if ((ai->userbits&DEADA) != 0)
			{
				ttyputmsg(_("Cell %s, arc %s: dead arc"), describenodeproto(np), describearcinst(ai));
				ai->userbits &= ~DEADA;
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}

			/* make sure the "far text" bit is set right */
			if (canscalefonts)
			{
				oldbits = ai->userbits;
				us_computearcfartextbit(ai);
				if (oldbits != ai->userbits)
				{
					if ((oldbits&AHASFARTEXT) == 0)
					{
						ttyputmsg(_("Cell %s, arc %s: has distant text, not marked so"),
							describenodeproto(np), describearcinst(ai));
					} else
					{
						ttyputmsg(_("Cell %s, arc %s: does not have distant text, but marked so"),
							describenodeproto(np), describearcinst(ai));
					}
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
				}
			}

			/* check variables in the arcinst */
			if (us_checkvariables((INTBIG)ai, VARCINST, &ai->numvar, ai->firstvar, curlib))
			{
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}
			if (us_checkvariables((INTBIG)ai->geom, VGEOM, &ai->geom->numvar, ai->geom->firstvar, curlib))
			{
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}
		}

		/* now make sure cell size is correct */
		if (np->lowx != totlx || np->highx != tothx || np->lowy != totly || np->highy != tothy)
		{
			ttyputmsg(_("Cell %s: bounds are %s<=X<=%s %s<=Y<=%s, should be %s<=X<=%s %s<=Y<=%s"),
				describenodeproto(np), latoa(np->lowx, lambda), latoa(np->highx, lambda),
				latoa(np->lowy, lambda), latoa(np->highy, lambda), latoa(totlx, lambda),
				latoa(tothx, lambda), latoa(totly, lambda), latoa(tothy, lambda));
			for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
			{
				growx = ((tothx - np->highx) + (totlx - np->lowx)) / 2;
				growy = ((tothy - np->highy) + (totly - np->lowy)) / 2;
				if (ni->transpose != 0)
				{
					makeangle(ni->rotation, ni->transpose, trans);
				} else
				{
					makeangle((3600 - ni->rotation)%3600, 0, trans);
				}
				xform(growx, growy, &shiftx, &shifty, trans);
				ni->lowx += totlx - np->lowx - growx + shiftx;
				ni->highx += tothx - np->highx - growx + shiftx;
				ni->lowy += totly - np->lowy - growy + shifty;
				ni->highy += tothy - np->highy - growy + shifty;
				updategeom(ni->geom, ni->parent);
			}
			np->lowx = totlx;   np->highx = tothx;
			np->lowy = totly;   np->highy = tothy;
			(*warnings)++;
			curlib->userbits |= LIBCHANGEDMAJOR;
		}

		/* check the R-tree in this cell */
		problems = us_checkrtree(NORTNODE, np->rtree, np, warnings, errors);
		if (problems != 0)
			curlib->userbits |= LIBCHANGEDMAJOR;

		/* complete R-tree check by ensuring that all "temp1" were set */
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			if (ai->temp1 != 1)
		{
			ttyputmsg(_("Cell %s, arc %s: no R-tree object, NOT REPAIRED"), describenodeproto(np),
				describearcinst(ai));
			(*errors)++;
			problems++;
		}
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			if (ni->temp1 != 1)
		{
			ttyputmsg(_("Cell %s, node %s: no R-tree object, NOT REPAIRED"), describenodeproto(np),
				describenodeinst(ni));
			(*errors)++;
			problems++;
		}

		/* check overall R-tree size */
		if (np->rtree->lowx != truelx || np->rtree->highx != truehx ||
			np->rtree->lowy != truely || np->rtree->highy != truehy)
		{
			ttyputmsg(_("Cell %s: Incorrect R-tree size (was %s<=X<=%s, %s<=Y<=%s, is %s<=X<=%s, %s<=Y<=%s)"),
				describenodeproto(np), latoa(np->rtree->lowx, lambda), latoa(np->rtree->highx, lambda),
				latoa(np->rtree->lowy, lambda), latoa(np->rtree->highy, lambda), latoa(truelx, lambda),
				latoa(truehx, lambda), latoa(truely, lambda), latoa(truehy, lambda));
			np->rtree->lowx = truelx;
			np->rtree->highx = truehx;
			np->rtree->lowy = truely;
			np->rtree->highy = truehy;
			problems++;
			(*warnings)++;
			curlib->userbits |= LIBCHANGEDMAJOR;
		}

		/* if there were R-Tree problems, rebuild it */
		if (problems != 0)
		{
			if (geomstructure(np))
			{
				ttyputmsg(_("Cell %s: cannot rebuild R-tree, NOT REPAIRED"), describenodeproto(np));
				(*errors)++;
			}
			ttyputmsg(_("Cell %s: rebuilding R-tree"), describenodeproto(np));
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
				linkgeom(ai->geom, np);
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				linkgeom(ni->geom, np);
		}
	}

	/* check network information */
	if ((net_tool->toolstate&TOOLON) == 0) return;
	neterrors = 0;
	for(np = curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (stopping(STOPREASONCHECK)) return;
		if (verbose) ttyputmsg(_("Checking networks in %s"), describenodeproto(np));

		/* initialize for port count and arc reference count, check buswidth */
		for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		{
			if(net->buswidth < 1 || (net->globalnet >= 0 && net->buswidth != 1))
			{
				ttyputmsg(_("Cell %s, %s network %s: buswidth %d is wrong"),
					(net->globalnet >= 0 ? "global" : ""), describenodeproto(np),
					describenetwork(net), net->buswidth);
				neterrors++;
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}
			net->temp1 = net->temp2 = 0;
		}

		/* check network information on ports */
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			if (pp->network == NONETWORK)
			{
				ttyputmsg(_("Cell %s, export %s: no network"), describenodeproto(np), pp->protoname);
				neterrors++;
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			} else pp->network->temp2++;
		}

		/* check network information on arcs */
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		{
			if (ai->network == NONETWORK)
			{
				if (((ai->proto->userbits&AFUNCTION)>>AFUNCTIONSH) != APNONELEC)
				{
					ttyputmsg(_("Cell %s, arc %s: no network"), describenodeproto(np),
						describearcinst(ai));
					neterrors++;
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
				}
			} else
			{
				if (ai->network->buswidth > 1 &&
					((ai->proto->userbits&AFUNCTION)>>AFUNCTIONSH) != APBUS)
				{
					ttyputmsg(_("Cell %s, arc %s: not a bus but network %s is, NOT REPAIRED"), describenodeproto(np),
						describearcinst(ai), describenetwork(ai->network));
					(*errors)++;
				}
				ai->network->temp1++;
			}
			ai->temp1 = 0;

			/* make sure width of bus is the same as the port */
			if (ai->proto == sch_busarc)
			{
				for(i=0; i<2; i++)
				{
					ni = ai->end[i].nodeinst;
					if (ni->proto->primindex == 0)
					{
						pi = ai->end[i].portarcinst;
						len = pi->proto->network->buswidth;
						if (ni->arraysize > 1) len *= ni->arraysize;
						if (ai->network->buswidth != len)
						{
							if (ni->arraysize > 1)
							{
								ttyputmsg(_("Cell %s, arc %s: is %d wide but port (on %ld-wide node) is %ld"),
									describenodeproto(np), describearcinst(ai), ai->network->buswidth,
										ni->arraysize, len);
							} else
							{
								ttyputmsg(_("Cell %s, arc %s: is %d wide but port is %ld"),
									describenodeproto(np), describearcinst(ai), ai->network->buswidth,
										len);
							}
							neterrors++;
							(*warnings)++;
							curlib->userbits |= LIBCHANGEDMAJOR;
						}
					}
				}
			}
		}

		/* consider global nets in refcount */
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			/* ignore primitives and recursive references (showing icon in contents) */
			if (ni->proto->primindex != 0) continue;
			if (isiconof(ni->proto, np)) continue;
			onp = contentsview(ni->proto);
			if (onp == NONODEPROTO) onp = ni->proto;
			for(i=0; i<onp->globalnetcount; i++)
			{
				if(onp->globalnetworks[i] == NONETWORK) continue;
				if(i<2)
					j = (i<np->globalnetcount && np->globalnetworks[i] != NONETWORK ? i : -1);
				else
					j = net_findglobalnet(np,onp->globalnetnames[i]);
				if(j>=0 && j<np->globalnetcount && np->globalnetworks[j] != NONETWORK)
					np->globalnetworks[j]->temp1++; else
				{
					ttyputmsg(_("Cell %s, global %s of subcell %s not found"), describenodeproto(np),
						onp->globalnetnames[i], describenodeproto(onp));
					neterrors++;
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
				}
			}
		}

		/* check port and arc reference counts, prepare for arc counts */
		for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		{
			for(i=0; i<net->arccount; i++)
			{
				if (net->arctotal == 0) ((ARCINST *)net->arcaddr)->temp1++; else
					((ARCINST **)net->arcaddr)[i]->temp1++;
			}
			if (net->refcount != net->temp1)
			{
				ttyputmsg(_("Cell %s, network %s: reference count wrong"), describenodeproto(np),
					describenetwork(net));
				neterrors++;
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}
			if (net->portcount != net->temp2)
			{
				ttyputmsg(_("Cell %s, network %s: port count wrong"), describenodeproto(np),
					describenetwork(net));
				neterrors++;
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}
		}

		/* check arc names */
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		{
			if (ai->temp1 == 0) continue;
			if (ai->temp1 != 1)
			{
				ttyputmsg(_("Cell %s, arc %s: described by %ld networks"), describenodeproto(np),
					describearcinst(ai), ai->temp1);
				neterrors++;
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}
			if (ai->network->namecount > 0)
			{
				if (getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key) == NOVARIABLE)
				{
					ttyputmsg(_("Cell %s, arc %s: on network but has no name"), describenodeproto(np),
						describearcinst(ai));
					neterrors++;
					(*warnings)++;
					curlib->userbits |= LIBCHANGEDMAJOR;
				}
			}
		}

		/* check bus link counts */
		for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
			net->temp1 = 0;
		for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		{
			if (net->buswidth > 1) for(i=0; i<net->buswidth; i++)
				net->networklist[i]->temp1++;
		}
		for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
			if (net->buslinkcount != net->temp1)
		{
			ttyputmsg(_("Cell %s, network %s: bus link count wrong"), describenodeproto(np),
				describenetwork(net));
			neterrors++;
			(*warnings)++;
			curlib->userbits |= LIBCHANGEDMAJOR;
		}

		/* miscellaneous checks */
		for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		{
			if (net->parent != np)
			{
				ttyputmsg(_("Cell %s, network %s: wrong parent"), describenodeproto(np),
					describenetwork(net));
				neterrors++;
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}
			if (us_checkvariables((INTBIG)net, VNETWORK, &net->numvar, net->firstvar, curlib))
			{
				(*warnings)++;
				curlib->userbits |= LIBCHANGEDMAJOR;
			}
			if (net->globalnet >= 2)
			{
				if (net->globalnet >= np->globalnetcount)
				{
					ttyputmsg(_("Cell %s, network %s: on global net %d, but there are only %ld"),
						describenodeproto(np), describenetwork(net), net->globalnet, np->globalnetcount);
					(*warnings)++;
				} else
				{
					if (np->globalnetworks[net->globalnet] != net)
					{
						ttyputmsg(_("Cell %s, network %s: on global net %d which points to %s"),
							describenodeproto(np), describenetwork(net), net->globalnet,
								describenetwork(np->globalnetworks[net->globalnet]));
						(*warnings)++;
					}
				}
			}
		}

		/* check name hash */
		net_checknetprivate(np);
	}

	if (neterrors != 0) (void)asktool(net_tool, x_("total-re-number"));
}

BOOLEAN us_checkvariables(INTBIG addr, INTBIG type, INTSML *numvar, VARIABLE *firstvar, LIBRARY *olib)
{
	REGISTER INTBIG i, j, errors, ty, len;
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np, *vnp;
	REGISTER CHAR *varname, *matchname;
	REGISTER TECHNOLOGY *tech;
	REGISTER VARIABLE *var;

	if (*numvar < 0)
	{
		ttyputmsg(_("%s: variable count is negative"), us_describeobject(addr, type));
		*numvar = 0;
		lib = whichlibrary(addr, type);
		if (lib != NOLIBRARY) lib->userbits |= LIBCHANGEDMAJOR; else
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
				lib->userbits |= LIBCHANGEDMAJOR;
		return(TRUE);
	}

	errors = 0;
	for(i = 0; i < *numvar; i++)
	{
		var = &firstvar[i];
		varname = (CHAR *)var->key;

		/* see if this variable has been deprecated */
		if (isdeprecatedvariable(addr, type, varname))
		{
			ttyputmsg(_("%s: deprecated variable '%s' removed"),
				us_describeobject(addr, type), varname);
			(*numvar)--;
			for(j = i; j < *numvar; j++)
			{
				firstvar[j].key  = firstvar[j+1].key;
				firstvar[j].type = firstvar[j+1].type;
				firstvar[j].addr = firstvar[j+1].addr;
			}
			errors++;
			i--;
			continue;
		}

		/* see if this variable should not be saved */
		ty = var->type;
		if ((ty&VDONTSAVE) == 0)
		{
			for(j=0; us_temporaryvariables[j].type != 0; j++)
			{
				if (us_temporaryvariables[j].type != type) continue;
				if (us_temporaryvariables[j].addr != 0)
				{
					if (addr != *((INTBIG *)us_temporaryvariables[j].addr)) continue;
				}
				matchname = us_temporaryvariables[j].name;
				if (namesame(matchname, varname) == 0) break;
				len = estrlen(matchname) - 1;
				if (matchname[len] == '*')
				{
					if (namesamen(matchname, varname, len) == 0) break;
				}
			}
			if (us_temporaryvariables[j].type != 0)
			{
				ttyputmsg(_("%s: permanent variable '%s' made temporary"),
					us_describeobject(addr, type), varname);
				firstvar[i].type |= VDONTSAVE;
				errors++;
				continue;
			}
		}

		/* see if this variable can't be code */
		if ((var->type&(VCODE1|VCODE2)) != 0 &&
			((type == VNODEINST && var->key == sch_globalnamekey) ||
			 (type == VNODEINST && var->key == el_node_name_key) ||
			 (type == VARCINST && var->key == el_arc_name_key)))
		{
			ttyputmsg(_("%s: variable '%s' cannot be interpreted code"),
				us_describeobject(addr, type), varname);
			var->type = VSTRING | (var->type&(VDISPLAY|VDONTSAVE|VCANTSET));
			errors++;
		}

		/* make sure the variable is in the main namespace */
		for(j=0; j<el_numnames; j++)
			if (varname == el_namespace[j]) break;
		if (j >= el_numnames)
		{
			ttyputmsg(_("%s: variable '%s' is invalid"),
				us_describeobject(addr, type), varname);
			(*numvar)--;
			for(j = i; j < *numvar; j++)
			{
				firstvar[j].key  = firstvar[j+1].key;
				firstvar[j].type = firstvar[j+1].type;
				firstvar[j].addr = firstvar[j+1].addr;
			}
			errors++;
			i--;
			continue;
		}

		/* check parameters on cells have the "inheritable" bit set */
		if (type == VNODEPROTO && TDGETISPARAM(var->textdescript) != 0)
		{
			/* make sure the "inheritable" bit is set */
			if (TDGETINHERIT(var->textdescript) == 0)
			{
				ttyputmsg(_("%s: variable '%s' is parameter but not inheritable"),
					us_describeobject(addr, type), varname);
				TDSETINHERIT(var->textdescript, VTINHERIT);
				errors++;
				continue;
			}

			/* make sure it is visible */
			if ((var->type&VDISPLAY) == 0)
			{
				ttyputmsg(_("%s: variable '%s' is parameter but not visible"),
					us_describeobject(addr, type), varname);
				var->type |= VDISPLAY;
				errors++;
				continue;
			}
		}

		/* check validity of certain pointer variables */
		if ((ty&(VTYPE|VISARRAY)) == VNODEPROTO)
		{
			/* check validity of nonarray NODEPROTO pointers */
			vnp = (NODEPROTO *)firstvar[i].addr;
			if (vnp != NONODEPROTO)
			{
				np = NONODEPROTO;
				if (olib == NOLIBRARY)
				{
					for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
					{
						for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
							if (np == vnp) break;
						if (np != NONODEPROTO) break;
					}
				} else
				{
					for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
						if (np == vnp) break;
				}
				if (np == NONODEPROTO)
				{
					for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
					{
						for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
							if (np == vnp) break;
						if (np != NONODEPROTO) break;
					}
				}
				if (np == NONODEPROTO)
				{
					ttyputmsg(_("%s: bad cell variable '%s'"),
						us_describeobject(addr, type), varname);
					(*numvar)--;
					for(j = i; j < *numvar; j++)
					{
						firstvar[j].key  = firstvar[j+1].key;
						firstvar[j].type = firstvar[j+1].type;
						firstvar[j].addr = firstvar[j+1].addr;
					}
					errors++;
					i--;
				}
			}
		}
		if ((ty&VDISPLAY) != 0 &&
			us_checktextdescript(addr, type, firstvar[i].textdescript))
				errors++;
	}
	if (errors != 0)
	{
		lib = whichlibrary(addr, type);
		if (lib != NOLIBRARY) lib->userbits |= LIBCHANGEDMAJOR; else
		{
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			{
				if (lib->firstnodeproto != NONODEPROTO)
					lib->userbits |= LIBCHANGEDMAJOR;
			}
		}
		return(TRUE);
	}
	return(FALSE);
}

/*
 * Routine to ensure that cell "np" has unique node names.
 * Returns true if duplicate names are found.
 */
BOOLEAN us_checkforuniquenodenames(NODEPROTO *np)
{
	REGISTER INTBIG i, count;
	REGISTER BOOLEAN retval;
	REGISTER CHAR *lastname, *thisname, *warnedname;
	CHAR **namelist;

	count = net_gathernodenames(np, &namelist);
	if (count <= 1) return(FALSE);
	esort(namelist, count, sizeof (CHAR *), sort_stringascending);

	retval = FALSE;
	lastname = namelist[0];
	warnedname = 0;
	for(i=1; i<count; i++)
	{
		thisname = (CHAR *)namelist[i];
		if (lastname != 0 && thisname != 0)
		{
			if (namesame(lastname, thisname) == 0)
			{
				if (warnedname == 0 || namesame(warnedname, thisname) != 0)
				{
					ttyputerr(_("Cell %s: has multiple nodes with name '%s' (NOT REPAIRED)"),
						describenodeproto(np), thisname);
					retval = TRUE;
				}
				warnedname = thisname;
			}
		}
		lastname = thisname;
	}
	return(retval);
}

/*
 * Routine to describe object "addr" of type "type".
 */
CHAR *us_describeobject(INTBIG addr, INTBIG type)
{
	REGISTER NODEPROTO *np;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER NODEINST *ni;
	REGISTER GEOM *geom;
	REGISTER LIBRARY *lib;
	REGISTER VIEW *v;
	REGISTER TOOL *tool;
	REGISTER TECHNOLOGY *tech;
	REGISTER ARCPROTO *ap;
	REGISTER ARCINST *ai;
	REGISTER INTBIG thisend;
	REGISTER NETWORK *net;
	REGISTER PORTPROTO *pp;
	REGISTER void *infstr;

	infstr = initinfstr();
	switch (type)
	{
		case VNODEINST:
			ni = (NODEINST *)addr;
			formatinfstr(infstr, x_("Cell %s, node %s"), describenodeproto(ni->parent), describenodeinst(ni));
			break;
		case VNODEPROTO:
			np = (NODEPROTO *)addr;
			if (np->primindex != 0)
			{
				formatinfstr(infstr, x_("Technology %s, node %s"), np->tech->techname, np->protoname);
			} else
			{
				formatinfstr(infstr, x_("Cell %s"), describenodeproto(np));
			}
			break;
		case VPORTARCINST:
			pi = (PORTARCINST *)addr;
			ai = pi->conarcinst;
			if (ai->end[0].portarcinst == pi) thisend = 0; else
				thisend = 1;
			ni = ai->end[thisend].nodeinst;
			formatinfstr(infstr, x_("Cell %s, node %s, port %s"),
				describenodeproto(ai->parent), describenodeinst(ni), pi->proto->protoname);
			break;
		case VPORTEXPINST:
			pe = (PORTEXPINST *)addr;
			pp = pe->exportproto;
			formatinfstr(infstr, x_("Cell %s, node %s, export %s"),
				describenodeproto(pp->parent), describenodeinst(pp->subnodeinst), pp->protoname);
			break;
		case VPORTPROTO:
			pp = (PORTPROTO *)addr;
			formatinfstr(infstr, x_("Cell %s, export %s"), describenodeproto(pp->parent), pp->protoname);
			break;
		case VARCINST:
			ai = (ARCINST *)addr;
			formatinfstr(infstr, x_("Cell %s, arc %s"), describenodeproto(ai->parent), describearcinst(ai));
			break;
		case VARCPROTO:
			ap = (ARCPROTO *)addr;
			formatinfstr(infstr, x_("Arc %s"), describearcproto(ap));
			break;
		case VGEOM:
			geom = (GEOM *)addr;
			if (!geom->entryisnode)
			{
				ai = geom->entryaddr.ai;
				formatinfstr(infstr, x_("Cell %s, arc geom %s"), describenodeproto(ai->parent), describearcinst(ai));
			} else
			{
				ni = geom->entryaddr.ni;
				formatinfstr(infstr, x_("Cell %s, node geom %s"), describenodeproto(ni->parent), describenodeinst(ni));
			}
			break;
		case VLIBRARY:
			lib = (LIBRARY *)addr;
			formatinfstr(infstr, x_("Library %s"), lib->libname);
			break;
		case VTECHNOLOGY:
			tech = (TECHNOLOGY *)addr;
			formatinfstr(infstr, x_("Technology %s"), tech->techname);
			break;
		case VTOOL:
			tool = (TOOL *)addr;
			formatinfstr(infstr, x_("Tool %s"), tool->toolname);
			break;
		case VNETWORK:
			net = (NETWORK *)addr;
			formatinfstr(infstr, x_("Cell %s, network %s"), describenodeproto(net->parent), describenetwork(net));
			break;
		case VVIEW:
			v = (VIEW *)addr;
			formatinfstr(infstr, x_("View %s"), v->viewname);
			break;
	}
	return(returninfstr(infstr));
}

/*
 * Routine to check the text descriptor field in "descript" and return nonzero if there
 * are errors.
 */
BOOLEAN us_checktextdescript(INTBIG addr, INTBIG type, UINTBIG *descript)
{
	INTBIG face, maxfaces, disppart;
	CHAR **facelist;

	if ((type&VTYPE) != VNODEPROTO)
	{
		disppart = TDGETDISPPART(descript);
		if(disppart == VTDISPLAYNAMEVALINH || disppart == VTDISPLAYNAMEVALINHALL)
		{
			ttyputmsg(_("%s: textdescript has inherited display, changing to name-value-display"),
				  us_describeobject(addr, type));
			TDSETDISPPART(descript, VTDISPLAYNAMEVALUE);
			return(TRUE);
		}
	}

	if (!graphicshas(CANCHOOSEFACES)) return(FALSE);
	face = TDGETFACE(descript);
	maxfaces = screengetfacelist(&facelist, FALSE);
	if (face >= maxfaces)
	{
		TDSETFACE(descript, 0);
		ttyputmsg(_("%s: bad face part of textdescript (%ld)"),
			us_describeobject(addr, type), face);
		return(TRUE);
	}
	return(FALSE);
}

/*
 * routine to recursively check the R-tree structure at "rtn".  The expected
 * R-tree parent is "expectedparent" and the cell containing this structure
 * is "cell"
 */
INTBIG us_checkrtree(RTNODE *expectedparent, RTNODE *rtn, NODEPROTO *cell,
	INTBIG *warnings, INTBIG *errors)
{
	REGISTER INTBIG j;
	INTBIG lx, hx, ly, hy, lowestxv, highestxv, lowestyv, highestyv, problems;
	REGISTER GEOM *ge;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;

	problems = 0;

	/* sensibility check of this R-tree node */
	if (rtn->parent != expectedparent)
	{
		ttyputmsg(_("Cell %s: R-tree node has wrong parent"), describenodeproto(cell));
		rtn->parent = expectedparent;
		(*warnings)++;
		problems++;
	}
	if (rtn->total > MAXRTNODESIZE)
	{
		ttyputmsg(_("Cell %s: R-tree node has too many entries (%d)"), describenodeproto(cell),
			rtn->total);
		rtn->total = MAXRTNODESIZE;
		(*warnings)++;
		problems++;
	}
	if (rtn->total < MINRTNODESIZE && rtn->parent != NORTNODE)
	{
		ttyputmsg(_("Cell %s: R-tree node has too few entries (%d)"), describenodeproto(cell),
			rtn->total);
		(*warnings)++;
		problems++;
	}

	/* check children */
	for(j=0; j<rtn->total; j++)
	{
		if (rtn->flag == 0)
			problems += us_checkrtree(rtn, (RTNODE *)rtn->pointers[j], cell, warnings, errors); else
		{
			ge = (GEOM *)rtn->pointers[j];
			if (ge->entryisnode)
			{
				ni = ge->entryaddr.ni;
				if (ni->temp1 != 0)
				{
					ttyputmsg(_("Cell %s, node %s: has duplicate R-tree pointer"),
						describenodeproto(cell), describenodeinst(ni));
					(*warnings)++;
					problems++;
				}
				ni->temp1 = 1;
			} else
			{
				ai = ge->entryaddr.ai;
				if (ai->temp1 != 0)
				{
					ttyputmsg(_("Cell %s, arc %s: has duplicate R-tree pointer"),
						describenodeproto(cell), describearcinst(ai));
					(*warnings)++;
					problems++;
				}
				ai->temp1 = 1;
			}
		}
	}

	if (rtn->total != 0)
	{
		db_rtnbbox(rtn, 0, &lowestxv, &highestxv, &lowestyv, &highestyv);
		for(j=1; j<rtn->total; j++)
		{
			db_rtnbbox(rtn, j, &lx, &hx, &ly, &hy);
			if (lx < lowestxv) lowestxv = lx;
			if (hx > highestxv) highestxv = hx;
			if (ly < lowestyv) lowestyv = ly;
			if (hy > highestyv) highestyv = hy;
		}
		if (rtn->lowx != lowestxv || rtn->highx != highestxv ||
			rtn->lowy != lowestyv || rtn->highy != highestyv)
		{
			ttyputmsg(_("Cell %s: R-tree node has wrong bounds"), describenodeproto(cell));
			(*warnings)++;
			problems++;
			rtn->lowx = lowestxv;
			rtn->highx = highestxv;
			rtn->lowy = lowestyv;
			rtn->highy = highestyv;
		}
	}
	return(problems);
}
