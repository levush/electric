/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: compact.c
 * One-dimensional compactor tool
 * Written by: Nora Ryan, Schlumberger Palo Alto Research
 * Rewritten by: Steven M. Rubin, Schlumberger Palo Alto Research
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

/*
 * When compacting cell instances, the system only examines polygons
 * within a protection frame of the cell border.  This frame is the largest
 * design rule distance in the technology.  If the cell border is irregular,
 * there may be objects that are not seen, causing the cell to overlap
 * more than it should.
 */

#include "config.h"
#if COMTOOL

#include "global.h"
#include "compact.h"
#include "efunction.h"
#include "edialogs.h"
#include "conlay.h"
#include "usr.h"

#define DEFAULT_VAL -99999999
#define HORIZONTAL          0
#define VERTICAL            1


#define NOCOMPPOLYLIST ((COMPPOLYLIST *)-1)

typedef struct Ipolylist
{
	POLYGON          *poly;
	TECHNOLOGY       *tech;
	INTBIG            networknum;
	struct Ipolylist *nextpolylist;
} COMPPOLYLIST;


#define NOOBJECT ((OBJECT *)-1)

typedef struct Iobject
{
	union u_inst
	{
		NODEINST   *ni;
		ARCINST    *ai;
	} inst;
	BOOLEAN         isnode;
	COMPPOLYLIST   *firstpolylist;
	INTBIG          lowx, highx, lowy, highy;
	struct Iobject *nextobject;
} OBJECT;


#define NOLINE ((LINE *)-1)

typedef struct Iline
{
	INTBIG        val;
	INTBIG        low, high;
	INTBIG        top, bottom;
	OBJECT       *firstobject;
	struct Iline *nextline;
	struct Iline *prevline;
} LINE;


#define NOTECHARRAY ((TECH_ARRAY *)-1)

typedef struct Itecharray
{
	INTBIG            *layer;
	INTBIG             taindex;
	struct Itecharray *nexttecharray;
} TECH_ARRAY;
static TECH_ARRAY *com_conv_layer;

/* prototypes for local routines */
static void com_buildtechnologies(void);
static OBJECT *com_make_ni_object(NODEINST*, OBJECT*, XARRAY, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static void com_fillnode(NODEINST*);
static LINE *com_sort(LINE*, INTBIG);
static void com_computeline_hi_and_low(LINE*, INTBIG);
static BOOLEAN com_lineup_firstrow(LINE*, LINE*, INTBIG, INTBIG);
static INTBIG com_findleastlow(LINE*, INTBIG);
static BOOLEAN com_compact(LINE*, LINE*, INTBIG, BOOLEAN, NODEPROTO*);
static INTBIG com_checkinst(OBJECT*, LINE*, INTBIG, OBJECT**, COMPPOLYLIST **, COMPPOLYLIST **, NODEPROTO*);
static INTBIG com_minseparate(OBJECT*, INTBIG, COMPPOLYLIST*, LINE*, INTBIG, OBJECT**, COMPPOLYLIST **, NODEPROTO*);
static INTBIG com_check(INTBIG, INTBIG, OBJECT*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, OBJECT*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static BOOLEAN com_cropnodeinst(COMPPOLYLIST*, INTBIG*, INTBIG*, INTBIG*, INTBIG*, INTBIG, INTBIG);
static BOOLEAN com_in_bound(INTBIG, INTBIG, INTBIG, INTBIG);
static void com_fixed_nonfixed(LINE*, LINE*);
static void com_undo_fixed_nonfixed(LINE*, LINE*);
static void com_noslide(LINE*);
static void com_slide(LINE*);
static BOOLEAN com_move(LINE*, INTBIG, INTBIG, BOOLEAN);
static INTBIG com_convertpintonode_layer(INTBIG, TECHNOLOGY*);
static void com_subsmash(NODEPROTO*);
static void com_add_poly_polylist(POLYGON*, OBJECT*, INTBIG, TECHNOLOGY*);
static OBJECT *com_allocate_object(void);
static void com_add_object_to_object(OBJECT**, OBJECT*);
static LINE *com_make_object_line(LINE*, OBJECT*);
static void com_clearspace(LINE*);
static void com_freeobject(OBJECT*);
static CHAR *com_describeline(LINE*);
static void com_addlinedescription(void*, LINE*);
static CHAR *com_describeobject(OBJECT*);
static void com_createobjects(NODEINST*, INTBIG, OBJECT**, OBJECT**);
static OBJECT *com_make_ai_object(ARCINST*, OBJECT*, XARRAY, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static BOOLEAN com_examineonecell(NODEPROTO*, INTBIG, BOOLEAN);
static void comp_optionsdlog(void);

/* the COMPACTOR tool table */
static COMCOMP comcp = {NOKEYWORD, topofcells, nextcells, NOPARAMS,
	0, x_(" \t"), M_("cell to be rechecked"), M_("default is current cell")};
static KEYWORD comnotopt[] =
{
	{x_("spread"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP comnotp = {comnotopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("Compactor negative action"), 0};
static KEYWORD comhvopt[] =
{
	{x_("horizontal"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("vertical"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP comhvp = {comhvopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("Compactor direction"), 0};
static KEYWORD comopt[] =
{
	{x_("check"),          2,{&comhvp,&comcp,NOKEY,NOKEY,NOKEY}},
	{x_("not"),            1,{&comnotp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("debug-toggle"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("spread"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP com_compactp = {comopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("Compactor action"), M_("show defaults")};

       TOOL     *com_tool;
static INTBIG    com_spread_key;		/* variable key for "COM_spread" */
static BOOLEAN   com_debug;
static INTBIG    com_maxboundary, com_lowbound;
static INTBIG    com_flatindex;			/* counter for unique network numbers */
static INTBIG    com_do_axis;

/************************ CONTROL ***********************/

void com_init(INTBIG *argc, CHAR1 *argv[], TOOL *thistool)
{
	/* ignore pass 3 initialization */
	if (thistool == 0) return;

	/* miscellaneous initialization during pass 2 */
	if (thistool == NOTOOL)
	{
		com_spread_key = makekey(x_("COM_spread"));
		nextchangequiet();
		(void)setvalkey((INTBIG)com_tool, VTOOL, com_spread_key, 0, VINTEGER|VDONTSAVE);
		com_debug = FALSE;
		com_do_axis = VERTICAL;
		com_conv_layer = NOTECHARRAY;
		registertechnologycache(com_buildtechnologies, 0, 0);
		DiaDeclareHook(x_("compopt"), &com_compactp, comp_optionsdlog);
		return;
	}

	/* copy tool pointer during pass 1 */
	com_tool = thistool;
}

/*
 * routine called when the technology configuration changes.  It recaches
 * the layer conversion information in the global linked list "com_conv_layer"
 */
void com_buildtechnologies(void)
{
	REGISTER TECHNOLOGY *tech;
	REGISTER TECH_ARRAY *cur_tech_array, *next_techarray;
	REGISTER INTBIG j, k;
	REGISTER INTBIG layer;

	/* first free the former list of layer conversions */
	for(cur_tech_array = com_conv_layer; cur_tech_array != NOTECHARRAY;
		cur_tech_array = next_techarray)
	{
		next_techarray = cur_tech_array->nexttecharray;
		efree((CHAR *)cur_tech_array->layer);
		efree((CHAR *)cur_tech_array);
	}
	com_conv_layer = NOTECHARRAY;

	/* initialize technology layer conversion array */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		cur_tech_array = (TECH_ARRAY *)emalloc((sizeof(TECH_ARRAY)), com_tool->cluster);
		if (cur_tech_array == 0) return;
		cur_tech_array->taindex = tech->techindex;
		cur_tech_array->nexttecharray = com_conv_layer;
		com_conv_layer = cur_tech_array;

		cur_tech_array->layer = (INTBIG *)emalloc(SIZEOFINTBIG * tech->layercount,
			com_tool->cluster);

		/* define the layer translation for pseudo layers and regular layers */
		for(j = 0; j < tech->layercount; j++)
		{
			cur_tech_array->layer[j] = j;
			layer = layerfunction(tech, j);
			if ((layer & LFPSEUDO) == 0) continue;

			/* layer is a pseudo layer */
			layer &= ~LFPSEUDO;	/* layer to be matched */
			for(k = 0; k < tech->layercount; k++)
				if (layerfunction(tech, k) == layer)
			{
				/* found matching layer */
				cur_tech_array->layer[j] = k;
				break;
			}
		}
	}
}

void com_done(void)
{
#ifdef DEBUGMEMORY
	REGISTER TECH_ARRAY *cur_tech_array, *next_techarray;

	/* first free the former list of layer conversions */
	for(cur_tech_array = com_conv_layer; cur_tech_array != NOTECHARRAY;
		cur_tech_array = next_techarray)
	{
		next_techarray = cur_tech_array->nexttecharray;
		efree((CHAR *)cur_tech_array->layer);
		efree((CHAR *)cur_tech_array);
	}
	com_conv_layer = NOTECHARRAY;
#endif
}

void com_slice(void)
{
	REGISTER BOOLEAN change;
	REGISTER NODEPROTO *np;
	CHAR *arg[2];

	np = getcurcell();
	if (np == NONODEPROTO) return;

	/* make sure network tool is on */
	if ((net_tool->toolstate&TOOLON) == 0)
	{
		ttyputerr(_("Network tool must be running...turning it on"));
		toolturnon(net_tool);
		return;
	}

	/* special handling for technology-edit cells */
	if (el_curwindowpart != NOWINDOWPART && np != NONODEPROTO &&
		(el_curwindowpart->state&WINDOWMODE) == WINDOWTECEDMODE)
	{
		/* special compaction of a technology edit cell can be done by the tec-editor */
		arg[0] = x_("compact-current-cell");
		us_tecedentry(1, arg);
		toolturnoff(com_tool, FALSE);
		return;
	}

	/* on alternate slices, do vertical then horizontal compaction */
	if (com_do_axis == VERTICAL) com_do_axis = HORIZONTAL; else
		com_do_axis = VERTICAL;

	change = com_examineonecell(np, com_do_axis, TRUE);

	if (!change)
	{
		ttyputmsg(_("No further change.  Compactor turned off"));
		toolturnoff(com_tool, FALSE);
	} else setactivity(_("Compaction"));
}

void com_set(INTBIG count, CHAR *par[])
{
	REGISTER NODEPROTO *np;
	REGISTER INTBIG l;
	REGISTER CHAR *pp;
	REGISTER VARIABLE *var;

	if (count == 0)
	{
		var = getvalkey((INTBIG)com_tool, VTOOL, VINTEGER, com_spread_key);
		if (var != NOVARIABLE)
		{
			if (var->addr != 0) ttyputmsg(M_("Compactor will spread if necessary")); else
				ttyputmsg(M_("Compactor will not spread designs"));
		}
		return;
	}

	l = estrlen(pp = par[0]);
	if (namesamen(pp, x_("not"), l) == 0)
	{
		if (count <= 1)
		{
			count = ttygetparam(M_("COMPACTOR negate option:"), &comnotp, MAXPARS-1,
				&par[1]) + 1;
			if (count <= 1)
			{
				ttyputerr(M_("Aborted"));
				return;
			}
		}
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("spread"), l) == 0)
		{
			(void)setvalkey((INTBIG)com_tool, VTOOL, com_spread_key, 0, VINTEGER|VDONTSAVE);
			ttyputverbose(M_("Compactor will not spread designs"));
			return;
		}
		ttyputbadusage(x_("telltool compaction not"));
		return;
	}

	if (namesamen(pp, x_("spread"), l) == 0)
	{
		(void)setvalkey((INTBIG)com_tool, VTOOL, com_spread_key, 1, VINTEGER|VDONTSAVE);
		ttyputverbose(M_("Compactor will spread if necessary"));
		return;
	}

	if (namesamen(pp, x_("check"), l) == 0)
	{
		/* make sure network tool is on */
		if ((net_tool->toolstate&TOOLON) == 0)
		{
			ttyputerr(M_("Network tool must be running...turning it on"));
			toolturnon(net_tool);
			ttyputerr(M_("...now reissue the compaction command"));
			return;
		}

		if (count <= 1)
		{
			count = ttygetparam(M_("Direction to compact:"), &comhvp, MAXPARS-1,
				&par[1]) + 1;
			if (count <= 1)
			{
				ttyputerr(M_("Aborted"));
				return;
			}
		}
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("horizontal"), l) == 0) com_do_axis = HORIZONTAL; else
		if (namesamen(pp, x_("vertical"), l) == 0) com_do_axis = VERTICAL; else
		{
			ttyputbadusage(x_("telltool compaction check"));
			return;
		}

		if (count >= 3)
		{
			np = getnodeproto(par[2]);
			if (np == NONODEPROTO)
			{
				ttyputerr(M_("No cell named %s"), par[2]);
				return;
			}
			if (np->primindex != 0)
			{
				ttyputerr(M_("Can only compact cells, not primitives"));
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

		(void)com_examineonecell(np, com_do_axis, FALSE);
		ttyputmsg(_("Cell %s compacted"), describenodeproto(np));
		return;
	}

	if (namesamen(pp, x_("debug-toggle"), l) == 0)
	{
		if (!com_debug)
		{
			com_debug = TRUE;
			ttyputmsg(M_("Compactor will print debugging information"));
		} else
		{
			com_debug = FALSE;
			ttyputmsg(M_("Compactor will not print debugging information"));
		}
		return;
	}
	ttyputbadusage(x_("telltool compaction"));
}

/************************ COMPACTION ***********************/

/*
 * routine to do vertical compaction (if "axis" is VERTICAL) or horizontal
 * compaction (if "axis" is HORIZONTAL) to cell "np".  Displays state if
 * "verbose" is nonzero.  Returns true if a change was made.
 */
BOOLEAN com_examineonecell(NODEPROTO *np, INTBIG axis, BOOLEAN verbose)
{
	REGISTER NODEINST *ni;
	REGISTER PORTPROTO *pp, *opp;
	REGISTER OBJECT *cur_object, **thisobject, **otherobject;
	OBJECT *object1, *object2;
	REGISTER LINE *linecomp, *linestretch, *cur_line;
	REGISTER BOOLEAN change;
	REGISTER INTBIG i, max;
	REGISTER void *infstr;

	if (stopping(STOPREASONCOMPACT)) return(FALSE);

	if (np == NONODEPROTO)
	{
		ttyputmsg(_("No current cell to compact"));
		return(FALSE);
	}

	/* determine maximum drc surround for entire technology */
	com_maxboundary = 0;
	for(i=0; i<el_curtech->layercount; i++)
	{
		max = maxdrcsurround(el_curtech, np->lib, i);
		if (max > com_maxboundary) com_maxboundary = max;
	}
	if (com_maxboundary < 0) com_maxboundary = 0;

	if (verbose)
	{
		if (axis == HORIZONTAL) ttyputmsg(_("Doing a horizontal compaction")); else
			ttyputmsg(_("Doing a vertical compaction"));
	}

	/* number ports of cell "np" in the "temp1" field */
	com_flatindex = 1;
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		/* see if this port is on the same net as previously examined one */
		for(opp = np->firstportproto; opp != pp; opp = opp->nextportproto)
			if (opp->network == pp->network) break;
		if (opp != pp) pp->temp1 = opp->temp1; else
			pp->temp1 = com_flatindex++;
	}

	/* copy port numbering onto arcs */
	com_subsmash(np);

	/* clear "seen" information on every node */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		ni->temp1 = 0;

	/* clear object information */
	linecomp = NOLINE;
	otherobject = &object2;
	*otherobject = NOOBJECT;

	/* now check every object */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* clear "thisobject" before calling com_createobject */
		thisobject = &object1;
		*thisobject = NOOBJECT;
		com_createobjects(ni, axis, thisobject, otherobject);

		/* create object of layout */
		if (*thisobject != NOOBJECT)
			linecomp = com_make_object_line(linecomp, *thisobject);
	}

	/* create list of perpendicular line which need to be set stretchable */
	if (*otherobject == NOOBJECT) linestretch = NOLINE; else
		linestretch = com_make_object_line(NOLINE, *otherobject);

	/* print the stretch object */
	if (com_debug)
	{
		infstr = initinfstr();
		if (linestretch != NOLINE)
			for(cur_object = linestretch->firstobject; cur_object != NOOBJECT;
				cur_object = cur_object->nextobject)
		{
			addtoinfstr(infstr, ' ');
			if (!cur_object->isnode)
				addstringtoinfstr(infstr, describearcinst(cur_object->inst.ai)); else
					addstringtoinfstr(infstr, describenodeinst(cur_object->inst.ni));
		}
		ttyputmsg(M_("Arcs that stretch:%s"), returninfstr(infstr));
	}

	/* sort the compacting line of objects */
	linecomp = com_sort(linecomp, axis);

	/* print the merged structure */
	if (com_debug)
	{
		if (axis == VERTICAL) ttyputmsg(M_("Bottom-to-top:")); else
			ttyputmsg(M_("Left-to-right:"));
		for(cur_line = linecomp; cur_line != NOLINE; cur_line = cur_line->nextline)
			ttyputmsg(M_("  Line: %s"), com_describeline(cur_line));
	}

	/* compute bounds for each line */
	for(cur_line = linecomp; cur_line != NOLINE; cur_line = cur_line->nextline)
		com_computeline_hi_and_low(cur_line, axis);

	/* prevent the stretching line from sliding */
	com_noslide(linestretch);

	/* set rigidity properly */
	com_fixed_nonfixed(linecomp, linestretch);

	/* do the compaction */
	com_lowbound = com_findleastlow(linecomp, axis);
	change = com_lineup_firstrow(linecomp, linestretch, axis, com_lowbound);
	change = com_compact(linecomp, linestretch, axis, change, np);

	/* restore rigidity if no changes were made */
	if (!change) com_undo_fixed_nonfixed(linecomp, linestretch);

	/* allow the streteching line to slide again */
	com_slide(linestretch);

	/* free the object data */
	com_clearspace(linecomp);
	com_clearspace(linestretch);
	return(change);
}

/******************** SUPPORT ROUTINES ********************/

void com_createobjects(NODEINST *ni, INTBIG axis, OBJECT **thisobject,
	OBJECT **otherobject)
{
	REGISTER OBJECT *new_object, *second_object;
	REGISTER PORTARCINST *pi;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *other_end;
	REGISTER INTBIG bd_low, bd_high, st_low, st_high;

	/* if node has already been examined, quit now */
	if (ni->temp1 != 0) return;
	ni->temp1 = 1;

	/* if this is the first object, add it */
	if (*thisobject == NOOBJECT)
		*thisobject = com_make_ni_object(ni, NOOBJECT, el_matid, axis, 0,0,0,0);
	if (axis == HORIZONTAL)
	{
		st_low = (*thisobject)->lowx;
		st_high = (*thisobject)->highx;
	} else
	{
		st_low = (*thisobject)->lowy;
		st_high = (*thisobject)->highy;
	}

	/* for each arc on node, find node at other end and add to object */
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		ai = pi->conarcinst;
		if (ai->end[0].nodeinst != ni) other_end = ai->end[0].nodeinst; else
			other_end = ai->end[1].nodeinst;

		/* stop if other end has already been examined */
		if (other_end->temp1 != 0) continue;

		new_object = com_make_ai_object(ai, NOOBJECT, el_matid, axis, 0,0,0,0);

		second_object = com_make_ni_object(other_end, NOOBJECT, el_matid,
			axis, 0,0,0,0);

		if (axis == HORIZONTAL)
		{
			bd_low = second_object->lowx;
			bd_high = second_object->highx;
		} else
		{
			bd_low = second_object->lowy;
			bd_high = second_object->highy;
		}
		if (bd_high > st_low && bd_low < st_high)
		{
			com_add_object_to_object(thisobject, new_object);
			com_add_object_to_object(thisobject, second_object);
			com_createobjects(other_end, axis, thisobject, otherobject);
		} else
		{
			/* arcs in object to be used later in fixed_non_fixed */
			com_add_object_to_object(otherobject, new_object);
			com_freeobject(second_object);
		}
	}
}

/*
 * routine to build a "object" structure that describes arc "ai".  If "object"
 * is NOOBJECT, this arc is at the top level, and a new OBJECT should be
 * constructed for it.  Otherwise, the arc is in a subcell and it must be
 * transformed through "newtrans" and clipped to the two protection frames
 * defined by "low1", "high1" and "low2", "high2" before being added to "object".
 */
OBJECT *com_make_ai_object(ARCINST *ai, OBJECT *object, XARRAY newtrans,
	INTBIG axis, INTBIG low1, INTBIG high1, INTBIG low2, INTBIG high2)
{
	REGISTER OBJECT *new_object;
	REGISTER INTBIG j, tot;
	INTBIG lx, hx, ly, hy;
	REGISTER POLYGON *poly;

	/* create the object if at the top level */
	if (object == NOOBJECT)
	{
		new_object = com_allocate_object();
		if (new_object == NOOBJECT) return(NOOBJECT);
		new_object->inst.ai = (ARCINST *)ai;
		new_object->isnode = FALSE;
		new_object->nextobject = NOOBJECT;
		new_object->firstpolylist = NOCOMPPOLYLIST;
		new_object->lowx = ai->geom->lowx;
		new_object->highx = ai->geom->highx;
		new_object->lowy = ai->geom->lowy;
		new_object->highy = ai->geom->highy;
	} else new_object = object;

	tot = arcpolys(ai, NOWINDOWPART);
	for(j=0; j<tot; j++)
	{
		poly = allocpolygon(4, com_tool->cluster);
		shapearcpoly(ai, j, poly);
		if (poly->layer < 0)
		{
			freepolygon(poly);
			continue;
		}

		/* make sure polygon is within protection frame */
		if (object != NOOBJECT)
		{
			xformpoly(poly, newtrans);
			getbbox(poly, &lx, &hx, &ly, &hy);
			if (axis == HORIZONTAL)
			{
				if ((hx < low1 || lx > high1) && (hx < low2 || lx > high2))
				{
					freepolygon(poly);
					continue;
				}
			} else
			{
				if ((hy < low1 || ly > high1) && (hy < low2 || ly > high2))
				{
					freepolygon(poly);
					continue;
				}
			}
		}

		/* add the polygon */
		com_add_poly_polylist(poly, new_object, ai->temp1,
			ai->proto->tech);
	}
	return(new_object);
}

/*
 * routine to build a object describing node "ni" in axis "axis".  If "object"
 * is NOOBJECT, this node is at the top level, and a new OBJECT should be
 * constructed for it.  Otherwise, the node is in a subcell and it must be
 * transformed through "newtrans" and clipped to the two protection frames
 * defined by "low1" to "high1" and "low2" to "high2" before being added to
 * "object".
 */
OBJECT *com_make_ni_object(NODEINST *ni, OBJECT *object, XARRAY newtrans,
	INTBIG axis, INTBIG low1, INTBIG high1, INTBIG low2, INTBIG high2)
{
	REGISTER OBJECT *new_object;
	XARRAY trans, t1, temp;
	INTBIG lx, hx, ly, hy;
	REGISTER INTBIG pindex;
	REGISTER INTBIG j, tot;
	REGISTER NODEINST *subni;
	REGISTER ARCINST *subai;
	REGISTER POLYGON *poly;

	if (object == NOOBJECT)
	{
		new_object = com_allocate_object();
		if (new_object == NOOBJECT) return(NOOBJECT);
		new_object->inst.ni = ni;
		new_object->isnode = TRUE;
		new_object->nextobject = NOOBJECT;
		new_object->firstpolylist = NOCOMPPOLYLIST;
		new_object->lowx = ni->geom->lowx;
		new_object->highx = ni->geom->highx;
		new_object->lowy = ni->geom->lowy;
		new_object->highy = ni->geom->highy;
	} else new_object = object;

	/* propagate global network info to local port prototypes on "ni" */
	com_fillnode(ni);

	/* create pseudo-object for complex ni */
	if (ni->proto->primindex == 0)
	{
		/* compute transformation matrix from subnode to this space */
		makerot(ni, t1);
		transmult(t1, newtrans, temp);
		maketrans(ni, t1);
		transmult(t1, temp, trans);

		/*
		 * create a line for cell "ni->proto" at the current location and
		 * translation.  Put only the instances which are within com_maxboundary
		 * of the perimeter of the cell.
		 */
		com_subsmash(ni->proto);

		/* compute protection frame if at the top level */
		if (object == NOOBJECT)
		{
			if (axis == HORIZONTAL)
			{
				low1 = ni->geom->lowx;
				high1 = ni->geom->lowx + com_maxboundary;
				low2 = ni->geom->highx - com_maxboundary;
				high2 = ni->geom->highx;
			} else
			{
				low1 = ni->geom->lowy;
				high1 = ni->geom->lowy + com_maxboundary;
				low2 = ni->geom->highy - com_maxboundary;
				high2 = ni->geom->highy;
			}
		}

		/* include polygons from those nodes and arcs in the protection frame */
		for(subni = ni->proto->firstnodeinst; subni != NONODEINST;
			subni = subni->nextnodeinst)
				(void)com_make_ni_object(subni, new_object, trans, axis,
					low1, high1, low2, high2);
		for(subai = ni->proto->firstarcinst; subai != NOARCINST;
			subai = subai->nextarcinst)
				(void)com_make_ai_object(subai, new_object, trans, axis,
					low1, high1, low2, high2);
	} else
	{
		makerot(ni, temp);
		transmult(temp, newtrans, trans);
		tot = nodeEpolys(ni, 0, NOWINDOWPART);
		for(j=0; j<tot; j++)
		{
			poly = allocpolygon(4, com_tool->cluster);
			shapeEnodepoly(ni, j, poly);
			xformpoly(poly, trans);

			/* make sure polygon is within protection frame */
			if (object != NOOBJECT)
			{
				getbbox(poly, &lx, &hx, &ly, &hy);
				if (axis == HORIZONTAL)
				{
					if ((hx < low1 || lx > high1) && (hx < low2 || lx > high2))
					{
						freepolygon(poly);
						continue;
					}
				} else
				{
					if ((hy < low1 || ly > high1) && (hy < low2 || ly > high2))
					{
						freepolygon(poly);
						continue;
					}
				}
			}

			if (poly->portproto == NOPORTPROTO) pindex = -1; else
				pindex = poly->portproto->temp1;
			com_add_poly_polylist(poly, new_object, pindex, ni->proto->tech);
		}
	}
	return(new_object);
}

void com_fillnode(NODEINST *ni)
{
	REGISTER PORTPROTO *pp, *opp;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;

	/* initialize network information for this node instance */
	for(pp = ni->proto->firstportproto; pp!=NOPORTPROTO; pp = pp->nextportproto)
		pp->temp1 = 0;

	/* set network numbers from arcs */
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		pp = pi->proto;
		if (pp->temp1 != 0) continue;
		pp->temp1 = pi->conarcinst->temp1;
	}

	/* set network numbers from exports */
	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
	{
		pp = pe->proto;
		if (pp->temp1 != 0) continue;
		pp->temp1 = pe->exportproto->temp1;
	}

	/* look for unconnected ports and assign new network numbers */
	for(pp = ni->proto->firstportproto; pp!=NOPORTPROTO; pp = pp->nextportproto)
	{
		if (pp->temp1 != 0) continue;

		/* look for similar connected port */
		for(opp = ni->proto->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
			if (opp->network == pp->network && opp->temp1 != 0)
		{
			pp->temp1 = opp->temp1;
			break;
		}
		if (pp->temp1 == 0) pp->temp1 = com_flatindex++;
	}
}

/*
 * routine to sort line by center val from least to greatest
 */
LINE *com_sort(LINE *line, INTBIG axis)
{
	REGISTER LINE *new_line, *cur_line, *bestline;
	REGISTER OBJECT *cur_object;
	REGISTER INTBIG bestval;
	float ave, ctr, len, totallen;
	REGISTER BOOLEAN first;

	if (line == NOLINE)
	{
		ttyputerr(_("Error: com_sort called with null argument"));
		return(NOLINE);
	}

	/* first figure out the weighting factor that will be sorted */
	for(cur_line = line; cur_line != NOLINE; cur_line = cur_line->nextline)
	{
		ave = totallen = 0.0;
		for(cur_object = cur_line->firstobject; cur_object != NOOBJECT;
			cur_object = cur_object->nextobject)
		{
			if (axis == HORIZONTAL)
			{
				len = (float)(cur_object->highy - cur_object->lowy);
				ctr = (cur_object->lowx+cur_object->highx) / 2.0f;
			} else
			{
				len = (float)(cur_object->highx - cur_object->lowx);
				ctr = (cur_object->lowy+cur_object->highy) / 2.0f;
			}

			ctr *= len;
			totallen += len;
			ave += ctr;
		}
		if (totallen != 0) ave /= totallen;
		cur_line->val = (INTBIG)ave;
	}

	/* now sort on the "val" field */
	new_line = NOLINE;
	for(;;)
	{
		if (line == NOLINE) break;
		first = TRUE;
		for(cur_line = line; cur_line != NOLINE; cur_line = cur_line->nextline)
		{
			if (first)
			{
				bestval = cur_line->val;
				bestline = cur_line;
				first = FALSE;
			} else if (cur_line->val > bestval)
			{
				bestval = cur_line->val;
				bestline = cur_line;
			}
		}

		/* remove bestline from the list */
		if (bestline->prevline == NOLINE) line = bestline->nextline; else
			bestline->prevline->nextline = bestline->nextline;
		if (bestline->nextline != NOLINE)
			bestline->nextline->prevline = bestline->prevline;

		/* insert at the start of this list */
		if (new_line != NOLINE) new_line->prevline = bestline;
		bestline->nextline = new_line;
		bestline->prevline = NOLINE;
		new_line = bestline;
	}
	return(new_line);
}

void com_computeline_hi_and_low(LINE *line, INTBIG axis)
{
	REGISTER BOOLEAN first_time;
	REGISTER INTBIG lx, hx, ly, hy;
	REGISTER OBJECT *cur_object;

	/* find smallest and highest vals for the each object */
	first_time = TRUE;
	for(cur_object = line->firstobject; cur_object != NOOBJECT;
		cur_object = cur_object->nextobject)
	{
		if (!cur_object->isnode) continue;
		if (first_time)
		{
			lx = cur_object->lowx;
			hx = cur_object->highx;
			ly = cur_object->lowy;
			hy = cur_object->highy;
			first_time = FALSE;
		} else
		{
			if (cur_object->lowx < lx) lx = cur_object->lowx;
			if (cur_object->highx > hx) hx = cur_object->highx;
			if (cur_object->lowy < ly) ly = cur_object->lowy;
			if (cur_object->highy > hy) hy = cur_object->highy;
		}
	}
	if (axis == HORIZONTAL)
	{
		line->low = lx;
		line->high = hx;
		line->top = hy;
		line->bottom = ly;
	} else
	{
		line->low = ly;
		line->high = hy;
		line->top = hx;
		line->bottom = lx;
	}
}

BOOLEAN com_lineup_firstrow(LINE *line, LINE *other_line, INTBIG axis, INTBIG lowbound)
{
	REGISTER INTBIG i;
	REGISTER BOOLEAN change;

	change = FALSE;
	i = line->low - lowbound;
	if (i > 0)
	{
		if (com_debug)
			ttyputmsg(M_("Lining up the first row to low edge %s"), latoa(lowbound, 0));
		if (axis == HORIZONTAL) change = com_move(line, i, 0, change); else
			change = com_move(line, 0, i, change);
		com_fixed_nonfixed(line, other_line);
	}
	return(change);
}

/*
 * find least low of the line. re-set first line low in the list
 * finds the smallest low value (lowx for VERTICAL, lowy for HORIZ case)
 * stores it in line->low.
 */
INTBIG com_findleastlow(LINE *line, INTBIG axis)
{
	REGISTER BOOLEAN first_time;
	REGISTER INTBIG low, thislow;
	REGISTER OBJECT *cur_object;

	if (line == NOLINE) return(0);

	/* find smallest low for the each object */
	first_time = TRUE;
	for(cur_object = line->firstobject; cur_object != NOOBJECT;
		cur_object = cur_object->nextobject)
	{
		if (!cur_object->isnode) continue;
		if (axis == HORIZONTAL) thislow = cur_object->lowx; else
			thislow = cur_object->lowy;

		/* LINTED "low" used in proper order */
		if (!first_time) low = mini(low, thislow); else
		{
			low = thislow;
			first_time = FALSE;
		}
	}
	line->low = low;

	return(low);
}

BOOLEAN com_compact(LINE *line, LINE *other_line, INTBIG axis, BOOLEAN change,
	NODEPROTO *cell)
{
	REGISTER LINE *cur_line, *prev_line;
	REGISTER OBJECT *cur_object, *thisoreason, *otheroreason;
	REGISTER COMPPOLYLIST *thispreason, *otherpreason;
	OBJECT *oreason;
	COMPPOLYLIST *preason, *opreason;
	REGISTER INTBIG this_motion, best_motion, spread;
	REGISTER VARIABLE *var;
	REGISTER void *infstr;

	spread = 0;
	var = getvalkey((INTBIG)com_tool, VTOOL, VINTEGER, com_spread_key);
	if (var != NOVARIABLE && var->addr != 0) spread++;

	/* loop through all lines that may compact */
	for(cur_line = line->nextline; cur_line != NOLINE; cur_line = cur_line->nextline)
	{
		/* look at every object in the line that may compact */
		best_motion = DEFAULT_VAL;
		thisoreason = otheroreason = NOOBJECT;
		for(cur_object = cur_line->firstobject; cur_object != NOOBJECT;
			cur_object = cur_object->nextobject)
		{
			/* look at all previous lines */
			for(prev_line = cur_line->prevline; prev_line != NOLINE;
				prev_line = prev_line->prevline)
			{
				/* no need to test this line if it is farther than best motion */
				if (best_motion != DEFAULT_VAL &&
					cur_line->low - prev_line->high > best_motion) continue;

				/* simple object compaction */
				this_motion = com_checkinst(cur_object, prev_line, axis,
					&oreason, &opreason, &preason, cell);
				if (this_motion == DEFAULT_VAL) continue;
				if (best_motion == DEFAULT_VAL || this_motion < best_motion)
				{
					best_motion = this_motion;
					if (oreason != NOOBJECT)
					{
						thisoreason = oreason;
						otheroreason = cur_object;
						thispreason = opreason;
						otherpreason = preason;
					}
				}
			}
		}

		if (best_motion == DEFAULT_VAL)
		{
			/* no constraints: allow overlap */
			best_motion = cur_line->low - com_lowbound;
		}
		if (com_debug)
		{
			ttyputmsg(M_("Moving object '%s' by %s"), com_describeline(cur_line),
				latoa(best_motion, 0));
			if (thisoreason != NOOBJECT && otheroreason != NOOBJECT)
			{
				infstr = initinfstr();
				if (thispreason->poly->tech != otherpreason->poly->tech ||
					thispreason->poly->layer != otherpreason->poly->layer)
				{
					formatinfstr(infstr, M_("  Limit is %s, layer %s to %s, layer %s"),
						com_describeobject(thisoreason),
							layername(thispreason->poly->tech, thispreason->poly->layer),
								com_describeobject(otheroreason),
									layername(otherpreason->poly->tech,
										otherpreason->poly->layer));
				} else
				{
					formatinfstr(infstr, M_("  Limit is %s to %s, layer %s"),
						com_describeobject(thisoreason), com_describeobject(otheroreason),
							layername(thispreason->poly->tech, thispreason->poly->layer));
				}
				ttyputmsg(x_("%s"), returninfstr(infstr));
			}
		}
		if (best_motion > 0 || (spread != 0 && best_motion < 0))
		{
			if (axis == HORIZONTAL)
				change = com_move(cur_line, best_motion, 0, change); else
					change = com_move(cur_line, 0, best_motion, change);
			com_fixed_nonfixed(line, other_line);
		}
	}
	return(change);
}

INTBIG com_checkinst(OBJECT *object, LINE *line, INTBIG axis, OBJECT **oreason,
	COMPPOLYLIST **opreason, COMPPOLYLIST **preason, NODEPROTO *cell)
{
	REGISTER INTBIG layer;
	REGISTER INTBIG this_motion, best_motion;
	REGISTER POLYGON *poly;
	REGISTER COMPPOLYLIST *polys;
	COMPPOLYLIST *subpreason;
	OBJECT *suboreason;

	best_motion = DEFAULT_VAL;
	*oreason = NOOBJECT;
	for(polys = object->firstpolylist; polys != NOCOMPPOLYLIST;
		polys = polys->nextpolylist)
	{
		poly = polys->poly;

		/* translate any pseudo layers for this node */
		layer = com_convertpintonode_layer(poly->layer, polys->tech);

		/* find distance line can move toward this poly */
		this_motion = com_minseparate(object, layer, polys, line, axis,
			&suboreason, &subpreason, cell);
		if (this_motion == DEFAULT_VAL) continue;
		if (best_motion == DEFAULT_VAL || this_motion < best_motion)
		{
			best_motion = this_motion;
			*oreason = suboreason;
			*opreason = subpreason;
			*preason = polys;
		}
	}
	return(best_motion);
}

/*
 * this routine finds the minimum distance which is necessary between polygon
 * "obj" (from object "object" on layer "nlayer" with network connectivity
 * "nindex") and the previous line in "line".  It returns the amount
 * to move this object to get it closest to the line (DEFAULT_VAL if they can
 * overlap).  The object "reason" is set to the object that is causing the
 * constraint.
 */
INTBIG com_minseparate(OBJECT *object, INTBIG nlayer, COMPPOLYLIST *npolys,
	LINE *line, INTBIG axis, OBJECT **oreason, COMPPOLYLIST **preason,
	NODEPROTO *cell)
{
	REGISTER INTBIG bound, dist, geom_lo, ai_hi, ni_hi,
		best_motion, this_motion, fun, minsize, nminsize;
	REGISTER INTBIG pindex, nindex, layer;
	REGISTER BOOLEAN con;
	REGISTER POLYGON *poly, *npoly;
	INTBIG lx, hx, ly, hy, xl, xh, yl, yh, edge;
	REGISTER OBJECT *cur_object;
	REGISTER COMPPOLYLIST *polys;
	REGISTER TECHNOLOGY *tech;

	*oreason = NOOBJECT;
	npoly = npolys->poly;
	nminsize = polyminsize(npoly);
	tech = npolys->tech;
	nindex = npolys->networknum;

	/* see how far around the box it is necessary to search */
	bound = maxdrcsurround(tech, cell->lib, nlayer);

	/* if there is no separation, allow them to sit on top of each other */
	if (bound < 0) return(DEFAULT_VAL);

	/* can only handle orthogonal rectangles for now */
	if (!isbox(npoly, &lx, &hx, &ly, &hy)) return(bound);

	best_motion = DEFAULT_VAL;
	if (axis == HORIZONTAL) geom_lo = object->lowx; else
		geom_lo = object->lowy;

	/* search the line */
	for(cur_object = line->firstobject; cur_object != NOOBJECT;
		cur_object = cur_object->nextobject)
	{
		if (cur_object->isnode)
		{
			if (axis == HORIZONTAL) ni_hi = cur_object->highx; else
				ni_hi = cur_object->highy;

			if ((axis == HORIZONTAL && com_in_bound(ly-bound, hy+bound,
				cur_object->lowy, cur_object->highy)) ||
					(axis == VERTICAL && com_in_bound(lx-bound, hx+bound,
						cur_object->lowx, cur_object->highx)))
			{
				/* examine every layer in this object */
				for(polys = cur_object->firstpolylist; polys != NOCOMPPOLYLIST;
					polys = polys->nextpolylist)
				{
					/* don't check between technologies */
					if (polys->tech != tech) continue;

					poly = polys->poly;
					layer = com_convertpintonode_layer(poly->layer, tech);
					pindex = polys->networknum;

					/* see whether the two objects are electrically connected */
					if (pindex == nindex && pindex != -1) con = TRUE;
						else con = FALSE;
					if (!isbox(poly, &xl, &xh, &yl, &yh))
					{
						this_motion = geom_lo - ni_hi - bound;
						if (this_motion == DEFAULT_VAL) continue;
						if (this_motion < best_motion || best_motion == DEFAULT_VAL)
						{
							best_motion = this_motion;
							*oreason = cur_object;
							*preason = polys;
						}
						continue;
					}

					/* see how close they can get */
					minsize = polyminsize(poly);
					dist = drcmindistance(tech, cell->lib, nlayer, nminsize,
						layer, minsize, con, FALSE, &edge, 0);
					if (dist < 0) continue;

					/*
					 * special rule for ignoring distance:
					 *   the layers are the same and either:
					 *     they connect and are *NOT* contact layers
					 *   or:
					 *     they don't connect and are implant layers (substrate/well)
					 */
					if (nlayer == layer)
					{
						fun = layerfunction(tech, nlayer) & LFTYPE;
						if (con)
						{
							if (!layeriscontact(fun)) continue;
						} else
						{
							if (fun == LFSUBSTRATE || fun == LFWELL ||
								fun == LFIMPLANT) continue;
						}
					}

					/*
					 * if the two layers are located on the y-axis so
					 * that there is no necessary contraint between them
					 */
					if ((axis == HORIZONTAL && !com_in_bound(ly-dist, hy+dist, yl, yh)) ||
						(axis == VERTICAL && !com_in_bound(lx-dist, hx+dist, xl, xh)))
							continue;

					/* check the distance */
					this_motion = com_check(nlayer, nindex, object, xl, xh, yl,
						yh, layer, pindex, cur_object, lx, hx, ly, hy, dist, axis);
					if (this_motion == DEFAULT_VAL) continue;
					if (this_motion < best_motion || best_motion == DEFAULT_VAL)
					{
						best_motion = this_motion;
						*oreason = cur_object;
						*preason = polys;
					}
				}
			}
		} else
		{
			if (axis == HORIZONTAL) ai_hi = cur_object->highx; else
				ai_hi = cur_object->highy;

			if ((axis == HORIZONTAL && com_in_bound(ly-bound, hy+bound,
				cur_object->lowy, cur_object->highy)) ||
					(axis == VERTICAL && com_in_bound(lx-bound, hx+bound,
						cur_object->lowx, cur_object->highx)))
			{
				/* prepare to examine every layer in this arcinst */
				for(polys = cur_object->firstpolylist; polys != NOCOMPPOLYLIST;
					polys = polys->nextpolylist)
				{
					/* don't check between technologies */
					if (polys->tech != tech) continue;

					poly = polys->poly;

					/* see whether the two objects are electrically connected */
					pindex = polys->networknum;
					if (nindex == -1 || pindex == nindex) con = TRUE; else con = FALSE;

					/* warning: non-manhattan arcs are ignored here */
					if (!isbox(poly, &xl, &xh, &yl, &yh))
					{
						this_motion = geom_lo - ai_hi - bound;
						if (this_motion == DEFAULT_VAL) continue;
						if (this_motion < best_motion || best_motion == DEFAULT_VAL)
						{
							best_motion = this_motion;
							*oreason = cur_object;
							*preason = polys;
						}
						continue;
					}

					/* see how close they can get */
					minsize = polyminsize(poly);
					dist = drcmindistance(tech, cell->lib, nlayer, nminsize,
						poly->layer, minsize, con, FALSE, &edge, 0);
					if (dist < 0) continue;

					/*
					 * if the two layers are so located on the y-axis so
					 * that there is no necessary contraint between them
					 */
					if ((axis == HORIZONTAL && !com_in_bound(ly-dist, hy+dist, yl, yh)) ||
						(axis == VERTICAL && !com_in_bound(lx-dist, hx+dist, xl, xh)))
							continue;

					/* check the distance */
					this_motion = com_check(nlayer, nindex, object, xl, xh, yl,
						yh, poly->layer, pindex, cur_object, lx, hx, ly, hy, dist, axis);
					if (this_motion == DEFAULT_VAL) continue;
					if (this_motion < best_motion || best_motion == DEFAULT_VAL)
					{
						best_motion = this_motion;
						*oreason = cur_object;
						*preason = polys;
					}
				}
			}
		}
	}
	return(best_motion);
}

/*
 * routine to see if the object in "object1" on layer "layer1" with electrical
 * index "index1" comes within "dist" from the object in "object2" on layer
 * "layer2" with electrical index "index2" in the perpendicular axis to "axis".
 * The bounds of object "object1" are (lx1-hx1,ly1-hy1), and the bounds of object
 * "object2" are (lx2-hx2,ly2-hy2).  If the objects are in bounds, the spacing
 * between them is returned.  Otherwise, DEFAULT_VAL is returned.
 */
INTBIG com_check(INTBIG layer1, INTBIG index1, OBJECT *object1, INTBIG lx1, INTBIG hx1,
	INTBIG ly1, INTBIG hy1, INTBIG layer2, INTBIG index2, OBJECT *object2, INTBIG lx2,
	INTBIG hx2, INTBIG ly2, INTBIG hy2, INTBIG dist, INTBIG axis)
{
	/* crop out parts of a box covered by a similar layer on the other node */
	if (object1->isnode)
	{
		if (com_cropnodeinst(object1->firstpolylist, &lx2, &hx2, &ly2, &hy2,
			layer2, index2))
				return(DEFAULT_VAL);
	}
	if (object2->isnode)
	{
		if (com_cropnodeinst(object2->firstpolylist, &lx1, &hx1, &ly1, &hy1,
			layer1, index1))
				return(DEFAULT_VAL);
	}

	/* now compare the box extents */
	if (axis == HORIZONTAL)
	{
		if (hy1+dist > ly2 && ly1-dist < hy2) return(lx2 - hx1 - dist);
	} else if (hx1+dist > lx2 && lx1-dist < hx2) return(ly2 - hy1 - dist);
	return(DEFAULT_VAL);
}

/*
 * routine to crop the box on layer "nlayer", electrical index "nindex"
 * and bounds (lx-hx, ly-hy) against the nodeinst "ni".  Only those layers
 * in the nodeinst that are the same layer and the same electrical index
 * are checked.  The routine returns true if the bounds are reduced
 * to nothing.
 */
BOOLEAN com_cropnodeinst(COMPPOLYLIST *polys, INTBIG *bx, INTBIG *by,
	INTBIG *ux, INTBIG *uy, INTBIG nlayer, INTBIG nindex)
{
	INTBIG xl, xh, yl, yh;
	REGISTER INTBIG temp;
	REGISTER COMPPOLYLIST *cur_polys;

	for(cur_polys = polys; cur_polys != NOCOMPPOLYLIST;
		cur_polys = cur_polys->nextpolylist)
	{
		if (cur_polys->networknum != nindex) continue;
		if (cur_polys->poly->layer != nlayer) continue;
		if (!isbox(cur_polys->poly, &xl, &xh, &yl, &yh)) continue;
		if (xl > xh) { temp = xl; xl = xh; xh = temp; }
		if (yl > yh) { temp = yl; yl = yh; yh = temp; }
		temp = cropbox(bx, ux, by, uy, xl, xh, yl, yh);
		if (temp > 0) return(TRUE);
	}
	return(FALSE);
}

BOOLEAN com_in_bound(INTBIG ll, INTBIG lh, INTBIG rl, INTBIG rh)
{
	if (rh > ll && rl < lh) return(1);
	return(0);
}

/*
 * this routine temporarily makes all arcs in fixline rigid and those
 * in nfixline nonrigid in order to move fixline over
 */
void com_fixed_nonfixed(LINE *fixline, LINE *nfixline)
{
	REGISTER LINE *cur_line;
	REGISTER OBJECT *cur_object;

	for(cur_line = fixline; cur_line != NOLINE; cur_line = cur_line->nextline)
	{
		for(cur_object = cur_line->firstobject; cur_object != NOOBJECT;
			cur_object = cur_object->nextobject)
				if (!cur_object->isnode)    /* arc rigid */
					(void)(*el_curconstraint->setobject)((INTBIG)cur_object->inst.ai,
						VARCINST, CHANGETYPETEMPRIGID, 0);
	}
	for(cur_line = nfixline; cur_line != NOLINE; cur_line = cur_line->nextline)
	{
		for(cur_object = cur_line->firstobject; cur_object != NOOBJECT;
			cur_object = cur_object->nextobject)
				if (!cur_object->isnode)   /* arc unrigid */
					(void)(*el_curconstraint->setobject)((INTBIG)cur_object->inst.ai,
						VARCINST, CHANGETYPETEMPUNRIGID, 0);
	}
}

/*
 * this routine resets temporary changes to arcs in fixline and nfixline
 * so that they are back to their default values.
 */
void com_undo_fixed_nonfixed(LINE *fixline, LINE *nfixline)
{
	REGISTER LINE *cur_line;
	REGISTER OBJECT *cur_object;

	for(cur_line = fixline; cur_line != NOLINE; cur_line = cur_line->nextline)
	{
		for(cur_object = cur_line->firstobject; cur_object != NOOBJECT;
			cur_object = cur_object->nextobject)
				if (!cur_object->isnode)
					(void)(*el_curconstraint->setobject)
						((INTBIG)cur_object->inst.ai, VARCINST,
							CHANGETYPEREMOVETEMP, 0);
	}
	for(cur_line = nfixline; cur_line != NOLINE; cur_line = cur_line->nextline)
	{
		for(cur_object = cur_line->firstobject; cur_object != NOOBJECT;
			cur_object = cur_object->nextobject)
				if (!cur_object->isnode)
					(void)(*el_curconstraint->setobject)
						((INTBIG)cur_object->inst.ai, VARCINST,
							CHANGETYPEREMOVETEMP, 0);
	}
}

/*
 * set the CANTSLIDE bit of userbits for each object in line so that this
 * line will not slide.
 */
void com_noslide(LINE *line)
{
	REGISTER OBJECT *cur_object;
	REGISTER ARCINST *ai;
	REGISTER LINE *cur_line;

	for(cur_line = line; cur_line != NOLINE; cur_line = cur_line->nextline)
	{
		for(cur_object = cur_line->firstobject; cur_object != NOOBJECT;
			cur_object = cur_object->nextobject)
		{
			if (!cur_object->isnode)
			{
				ai = cur_object->inst.ai;
				ai->temp2 = ai->userbits;
				ai->userbits |= CANTSLIDE;
			}
		}
	}
}

/*
 * restore the CANTSLIDE bit of userbits for each object in line
 */
void com_slide(LINE *line)
{
	REGISTER OBJECT *cur_object;
	REGISTER ARCINST *ai;
	REGISTER LINE *cur_line;

	for(cur_line = line; cur_line != NOLINE; cur_line = cur_line->nextline)
	{
		for(cur_object = cur_line->firstobject; cur_object != NOOBJECT;
			cur_object = cur_object->nextobject)
		{
			if (!cur_object->isnode)
			{
				ai = cur_object->inst.ai;
				ai->userbits = (ai->userbits & ~CANTSLIDE) |
					(ai->temp2&CANTSLIDE);
			}
		}
	}
}

/*
 * moves a object of instances distance (movex, movey), and returns a true if
 * there is actually a move
 */
BOOLEAN com_move(LINE *line, INTBIG movex, INTBIG movey, BOOLEAN change)
{
	REGISTER OBJECT *cur_object;
	REGISTER COMPPOLYLIST *polys;
	REGISTER INTBIG move;
	REGISTER INTBIG i;

	if (movex == 0) move = movey; else move = movex;
	if (line == NOLINE) return(FALSE);
	if (!change && move != 0)
	{
		(void)asktool(us_tool, x_("clear"));
		change = TRUE;
	}

	for(cur_object = line->firstobject; cur_object != NOOBJECT;
		cur_object = cur_object->nextobject)
	{
		if (cur_object->isnode)
		{
			startobjectchange((INTBIG)cur_object->inst.ni, VNODEINST);
			modifynodeinst(cur_object->inst.ni, -movex, -movey, -movex, -movey, 0, 0);
			endobjectchange((INTBIG)cur_object->inst.ni, VNODEINST);
			break;
		}
	}

	for(cur_object = line->firstobject; cur_object != NOOBJECT;
		cur_object = cur_object->nextobject)
	{
		cur_object->lowx -= movex;
		cur_object->highx -= movex;
		cur_object->lowy -= movey;
		cur_object->highy -= movey;
		for(polys = cur_object->firstpolylist; polys != NOCOMPPOLYLIST;
			polys = polys->nextpolylist)
		{
			for(i=0; i<polys->poly->count; i++)
			{
				polys->poly->xv[i] -= movex;
				polys->poly->yv[i] -= movey;
			}
		}
	}
	line->high -= move;
	line->low -= move;

	return(change);
}

/*
 * convert a PIN layer to its corresponding real layer.  Need this since the
 * design rules require "real" layers.
 */
INTBIG com_convertpintonode_layer(INTBIG layer, TECHNOLOGY *tech)
{
	REGISTER TECH_ARRAY *cur_technology;
	REGISTER INTBIG newlay;

	/* null layers do not convert */
	if (layer < 0) return(layer);

	/* make sure range of layers is valid */
	if (layer >= tech->layercount)
	{
		ttyputmsg(_("Compactor cannot find layer %ld of technology %s"),
			layer, tech->techname);
		return(layer);
	}

	/* find this technology */
	for(cur_technology = com_conv_layer; cur_technology != NOTECHARRAY;
		cur_technology = cur_technology->nexttecharray)
			if (cur_technology->taindex == tech->techindex) break;

	/* no technology: warn and return */
	if (cur_technology == NOTECHARRAY)
	{
		ttyputmsg(_("Compactor cannot find technology %s"), tech->techname);
		return(layer);
	}

	/* convert the layer */
	newlay = cur_technology->layer[layer];

	/* make sure conversion is valid */
	if (newlay >= tech->layercount)
	{
		ttyputmsg(_("Converted layer %ld to bogus layer %ld in technology %s"),
			layer, newlay, tech->techname);
		return(layer);
	}
	return(newlay);
}

/*
 * copy network information from ports to arcs in cell "topcell"
 */
void com_subsmash(NODEPROTO *topcell)
{
	REGISTER PORTPROTO *pp;
	REGISTER ARCINST *ai, *oai;

	/* first erase the arc node information */
	for(ai = topcell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		ai->temp1 = 0;

	/* copy network information from ports to arcs */
	for(ai = topcell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		/* ignore arcs that have already been numbered */
		if (ai->temp1 != 0) continue;

		/* see if this arc connects to a port */
		for(pp = topcell->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			if (pp->network == ai->network)
		{
			/* propagate port numbers into all connecting arcs */
			for(oai = topcell->firstarcinst; oai != NOARCINST; oai = oai->nextarcinst)
				if (oai->network == ai->network) oai->temp1 = pp->temp1;
			break;
		}

		/* if not connected to a port, this is an internal network */
		if (pp == NOPORTPROTO)
		{
			/* copy new net number to all of these connected arcs */
			for(oai = topcell->firstarcinst; oai != NOARCINST; oai = oai->nextarcinst)
				if (oai->network == ai->network) oai->temp1 = com_flatindex;
			com_flatindex++;
		}
	}
}

/**************************** ALLOCATION ****************************/

/*
 * routine to link polygon "poly" into object "object" with network number
 * "networknum"
 */
void com_add_poly_polylist(POLYGON *poly, OBJECT *object, INTBIG networknum,
	TECHNOLOGY *tech)
{
	REGISTER COMPPOLYLIST *new_polys;

	new_polys = (COMPPOLYLIST *)emalloc((sizeof(COMPPOLYLIST)), com_tool->cluster);
	if (new_polys == 0) return;
	new_polys->poly = poly;
	new_polys->tech = tech;
	new_polys->networknum = networknum;
	new_polys->nextpolylist = object->firstpolylist;
	object->firstpolylist = new_polys;
}

OBJECT *com_allocate_object(void)
{
	REGISTER OBJECT *cur_object;

	cur_object = (OBJECT *)emalloc((sizeof(OBJECT)), com_tool->cluster);
	if (cur_object == 0) return(NOOBJECT);
	cur_object->nextobject = NOOBJECT;
	return(cur_object);
}

/*
 * add object add_object to the beginning of list "*object"
 */
void com_add_object_to_object(OBJECT **object, OBJECT *add_object)
{
	add_object->nextobject = *object;
	*object = add_object;
}

/*
 * create a new line with the element object and add it to the beginning of
 * the given line
 */
LINE *com_make_object_line(LINE *line, OBJECT *object)
{
	REGISTER LINE *new_line;

	new_line = (LINE *)emalloc((sizeof(LINE)), com_tool->cluster);
	if (new_line == 0) return(NOLINE);
	new_line->nextline = line;
	new_line->prevline = NOLINE;
	new_line->firstobject = object;
	if (line != NOLINE) line->prevline = new_line;
	return(new_line);
}

/* free up allocated space for all objects and lines */
void com_clearspace(LINE *line)
{
	REGISTER LINE *cur_line, *next_line;
	REGISTER OBJECT *this_object;

	for(cur_line = line; cur_line != NOLINE; cur_line = next_line)
	{
		next_line = cur_line->nextline;

		/* erase all objects in the line */
		while (cur_line->firstobject != NOOBJECT)
		{
			this_object = cur_line->firstobject;
			cur_line->firstobject = this_object->nextobject;
			com_freeobject(this_object);
		}

		/* erase the line */
		efree((CHAR *)cur_line);
	}
}

void com_freeobject(OBJECT *cur_object)
{
	REGISTER COMPPOLYLIST *polys;

	/* erase all polygons in the object */
	while (cur_object->firstpolylist != NOCOMPPOLYLIST)
	{
		polys = cur_object->firstpolylist;
		cur_object->firstpolylist = polys->nextpolylist;
		freepolygon(polys->poly);
		efree((CHAR *)polys);
	}

	/* erase the object */
	efree((CHAR *)cur_object);
}

/******************** DEBUGGING ********************/

/* debugging routine to return a description of line "cur_line" */
CHAR *com_describeline(LINE *cur_line)
{
	REGISTER void *infstr;

	infstr = initinfstr();
	com_addlinedescription(infstr, cur_line);
	return(returninfstr(infstr));
}

void com_addlinedescription(void *infstr, LINE *cur_line)
{
	REGISTER OBJECT *cur_object;

	for(cur_object = cur_line->firstobject; cur_object != NOOBJECT;
		cur_object = cur_object->nextobject)
	{
		if (cur_object != cur_line->firstobject) addtoinfstr(infstr, ' ');
		if (!cur_object->isnode)
			addstringtoinfstr(infstr, describearcinst(cur_object->inst.ai)); else
				addstringtoinfstr(infstr, describenodeinst(cur_object->inst.ni));
	}
}

/* debugging routine to return a description of object "cur_object" */
CHAR *com_describeobject(OBJECT *cur_object)
{
	if (!cur_object->isnode)
		return(describearcinst(cur_object->inst.ai));
	return(describenodeinst(cur_object->inst.ni));
}

/****************************** DIALOG ******************************/

/* Compaction Options */
static DIALOGITEM com_optionsdialogitems[] =
{
 /*  1 */ {0, {64,92,88,156}, BUTTON, N_("OK")},
 /*  2 */ {0, {64,8,88,72}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,154}, CHECK, N_("Allow spreading")},
 /*  4 */ {0, {32,8,48,154}, CHECK, N_("Verbose")}
};
static DIALOG com_optionsdialog = {{50,75,147,243}, N_("Compaction Options"), 0, 4, com_optionsdialogitems, 0, 0};

/* special items for the "Compaction Options" dialog: */
#define DCMO_CANSPREAD   3		/* Allow spreading (check) */
#define DCMO_VERBOSE     4		/* Verbose (check) */

void comp_optionsdlog(void)
{
	REGISTER INTBIG itemHit, spread, origspread;
	REGISTER VARIABLE *var;
	REGISTER void *dia;

	dia = DiaInitDialog(&com_optionsdialog);
	if (dia == 0) return;
	var = getvalkey((INTBIG)com_tool, VTOOL, VINTEGER, com_spread_key);
	if (var == NOVARIABLE) origspread = 0; else origspread = var->addr;
	if (origspread != 0) DiaSetControl(dia, DCMO_CANSPREAD, 1);
	if (com_debug) DiaSetControl(dia, DCMO_VERBOSE, 1);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DCMO_CANSPREAD || itemHit == DCMO_VERBOSE)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		if (DiaGetControl(dia, DCMO_CANSPREAD) != 0) spread = 1; else
			spread = 0;
		if (spread != origspread)
			(void)setvalkey((INTBIG)com_tool, VTOOL, com_spread_key, spread, VINTEGER);
		if (DiaGetControl(dia, DCMO_VERBOSE) != 0) com_debug = TRUE; else
			com_debug = FALSE;
	}
	DiaDoneDialog(dia);
}

#endif  /* COMTOOL - at top */
