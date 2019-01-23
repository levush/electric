/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: compensate.c
 * Tool for compensating geometry
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
#if COMPENTOOL

#include "global.h"
#include "egraphics.h"
#include "dbcontour.h"
#include "edialogs.h"
#include "tecart.h"
#include "tecgen.h"
#include "usr.h"
#include "compensate.h"
#include <math.h>

/*********** tolerances ***********/

/* These are tolerances that have been found useful in dealing with imperfect data */

/*
 * The ARCSLOP factor determines how far the ends of arcs will extend in order to
 * make a connection to a tangent line.  If, for example, it is determined that the
 * tangent connects to a point on the circle that is beyond the ends of the arc segment,
 * but within this factor, then the arc is extended to the connection point.
 */
#define ARCSLOP              50						/* 5 degrees slop at ends of arc */

/*
 * The two thresholds BESTTHRESH and WORSTTHRESH are the distances that will be accepted
 * when gathering contour information.  At the end of a contour segment (an arc or a line),
 * an area that is BESTTHRESH in size will be examined to find the next contour segment.
 * If nothing is found in that area, the threshold will be expanded by a factor of 10 and
 * tried again.  This repeats until an area that is WORSTTHRESH in size is examined, at which
 * point the contour gathering will fail.
 */
#define BESTTHRESH           0.0001		/* 0.0001 mm (1/10 micron) */
#define WORSTTHRESH          0.1		/* 1/10 of a millimeter */

/*
 * The SMALLANGLETHRESH is the threshold for arc size that is acceptable for straightening
 * out in blending rule 2.1a.  If an arc is larger than this, it is not straightened.
 */
#define SMALLANGLETHRESH     200					/* 20 degrees */

/*
 * The CIRCLETANGENTTHRESH is the distance that will be allowed between a circle and a tangent
 * point on a line.  If the point is within this distance, it will be considered to be tangent
 * to the circle.
 */
#define CIRCLETANGENTTHRESH  0.0001		/* 1/10000 of a millimeter */

/*********** debugging ***********/

/* #define DEBDUMP 1 */		/* uncomment for debugging output */

#ifdef DEBDUMP				/* nonzero for debugging */
  FILE *compen_io;
#endif

static void compen_debugdump(CHAR *msg, ...);

/*********** automatic feature detection ***********/

typedef struct
{
	INTBIG     type;			/* 0 for circle, 1 for slot */
	INTBIG     diameter;		/* circle diameter or slot width */
	INTBIG     length;			/* slot length */
	INTBIG     percentage;		/* circle compensation or slot endcap compensation */
	INTBIG     lenpercentage;	/* slot length (side) compensation */
} AUTOFEATURE;

/*********** the COMPENSATION tool table ***********/

static COMCOMP compen_setf1p = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("metal thickness (or 'dialog' to set interactively)"), 0};
static COMCOMP compen_setf2p = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Laser Writing System edge compensation"), 0};
static COMCOMP compen_setf3p = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Global compensation"), 0};
static COMCOMP compen_setf4p = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Minimum feature size"), 0};
static COMCOMP compen_setp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("compensation percentage for the selected object(s)"), 0};
static KEYWORD compenopt[] =
{
	{x_("compensate"),                  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("uncompensate"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("detect-special-features"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("detect-duplicate-geometry"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("detect-layer-intersections"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("remove-percentage"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("next-contour-selection"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("prev-contour-selection"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("examine"),                     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("connect-points"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("percentage"),                  1,{&compen_setp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("global-factors"),              4,{&compen_setf1p,&compen_setf2p,&compen_setf3p,&compen_setf4p,NOKEY}},
	{x_("make-compensation-table"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("illuminate-percentages"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("show-contours"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("show-non-contours"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("draw-pre-compensation"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP compen_compensatep = {compenopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("Compensation action"), M_("show defaults")};

/*********** for coloring artwork ***********/

#define COLORS 14
static INTBIG compen_colors[COLORS+1] = {GREEN, CYAN, MAGENTA, YELLOW, ORANGE, PURPLE,
	LRED, LGREEN, LBLUE, LGRAY, DGREEN, GRAY, DRED, BROWN, 0};
static float compen_percentages[COLORS];
static GRAPHICS compen_rline = {LAYERO, RED, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
	0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};

/*********** additional data on contours ***********/

typedef struct
{
	INTBIG      origsx, origsy;		/* starting coordinate (line, arc, circle) */
	INTBIG      origex, origey;		/* ending coordinate (line, arc) */
	INTBIG      lowx, highx;		/* X bounds of the element */
	INTBIG      lowy, highy;		/* Y bounds of the element */
	float       percentage;			/* compensation amount */
	INTBIG      explicitpercentage;	/* nonzero if percentage value was explicitly given */
	CHAR       *DXFlayer;			/* DXF layer */
} USERDATA;

/*********** contour list associations ***********/

#define NOCELLCONTOURS ((CELLCONTOURS *)-1)

typedef struct Icellcontours
{
	NODEPROTO             *cell;	/* cell with a contours */
	CONTOUR               *contour;	/* contours associated with this cell */
	BOOLEAN                deleted;	/* nonzero if this has been deleted */
	struct Icellcontours  *next;	/* next in list */
} CELLCONTOURS;

static CELLCONTOURS *compen_firstcellcontours = NOCELLCONTOURS;
static INTBIG         compen_deletedcellcontours = 0;

/*********** miscellaneous information ***********/

#define PRECOMPMAGIC	0		/* magic number (0xFEED) */
#define PRECOMPTYPE     1		/* type of element */
#define PRECOMPPERC     2		/* percentage of compensation */
#define PRECOMPEXPPERC  3		/* nonzero if compensation explicitly given */

#define PRECOMPLINESX   4		/* starting X of line */
#define PRECOMPLINESY   5		/* starting Y of line */
#define PRECOMPLINEEX   6		/* ending X of line */
#define PRECOMPLINEEY   7		/* ending Y of line */
#define PRECOMPLINESIZE 8		/* size of line descriptor */

#define PRECOMPCIRCSX   4		/* edge X of circle */
#define PRECOMPCIRCSY   5		/* edge Y of circle */
#define PRECOMPCIRCRAD  6		/* radius of circle */
#define PRECOMPCIRCSIZE 7		/* size of circle descriptor */

#define PRECOMPARCSX    4		/* starting X of arc */
#define PRECOMPARCSY    5		/* starting Y of arc */
#define PRECOMPARCEX    6		/* ending X of arc */
#define PRECOMPARCEY    7		/* ending Y of arc */
#define PRECOMPARCRAD   8		/* radius of arc */
#define PRECOMPARCSIZE  9		/* size of arc descriptor */

#define DEFAULTMETALTHICKNESS       0.076
#define DEFAULTLRSCOMPENSATION      0.0057912
#define DEFAULTGLOBALCOMPENSATION  95.0
#define DEFAULTMINIMUMSIZE          0.0505

static TOOL      *compen_tool;
static INTBIG     compen_gds_layerkey;			/* key for "IO_gds_layer" */
static INTBIG     compen_dxf_layerkey;			/* key for "IO_dxf_layer" */
static INTBIG     compen_percentagekey;			/* key for "COMPEN_percentage" */
static INTBIG     compen_precomppositionkey;	/* key for "COMPEN_pre_compensation_position" */
static INTBIG     compen_metalthicknesskey;		/* key for "COMPEN_metalthickness" */
static INTBIG     compen_lrscompensationkey;	/* key for "COMPEN_lrscompensation" */
static INTBIG     compen_globalcompensationkey;	/* key for "COMPEN_globalcompensation" */
static INTBIG     compen_minimumsizekey;		/* key for "COMPEN_minimumsize" */
static INTBIG     compen_circletangentthresh;

/*********** prototypes for local routines ***********/

static void     compen_advanceselection(short next);
static INTBIG   compen_angoffset(double a1, double a2);
static BOOLEAN  compen_arcintersection(CONTOURELEMENT *arcconel, INTBIG x, INTBIG y, INTBIG otherx,
					INTBIG othery, INTBIG *ix, INTBIG *iy);
static BOOLEAN  compen_arctangent(CONTOURELEMENT *arcconel, INTBIG prefx, INTBIG prefy,
					CONTOURELEMENT *otherconel, INTBIG x, INTBIG y, INTBIG *ix, INTBIG *iy);
static void     compen_addchild(CONTOUR *parent, CONTOUR *child);
static void     compen_adjustgeometry(CONTOUR *contourlist, NODEPROTO *np);
static void     compen_assigndepth(CONTOUR *con, INTBIG depth);
static void     compen_assignpercentages(CONTOUR *contourlist, NODEPROTO *np);
static void     compen_cleancontours(CONTOUR *contourlist, NODEPROTO *np, BOOLEAN blend);
static void     compen_compensatecell(NODEPROTO *np);
static void     compen_connectpoints(void);
static INTBIG   compen_detectduplicates(NODEPROTO *np);
static INTBIG   compen_detectfeatures(NODEPROTO *np);
static void     compen_detectlayerintersections(NODEPROTO *np);
static void     compen_detectnoncontours(NODEPROTO *np);
static void     compen_dofactorsdialog(void);
static void     compen_drawcircle(INTBIG centerx, INTBIG centery, INTBIG x, INTBIG y, GRAPHICS *desc);
static void     compen_drawcirclearc(INTBIG centerx, INTBIG centery, INTBIG x1, INTBIG y1, INTBIG x2,
					INTBIG y2, GRAPHICS *desc);
static void     compen_drawcontours(NODEPROTO *np);
static void     compen_drawline(INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, GRAPHICS *desc);
static void     compen_drawprecompensation(NODEPROTO *np, GEOM **list);
static void     compen_examine(void);
static double   compen_figureangle(INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty);
static void     compen_forcemeeting(CONTOUR *con, CONTOURELEMENT *firstconel, CONTOURELEMENT *secondconel,
					BOOLEAN blend);
static CONTOUR *compen_getcontourlist(NODEPROTO *np);
static void     compen_getglobalfactors(float *metalthickness, float *lrscompensation,
					float *globalcompensation, float *minimumsize, LIBRARY *lib);
static void     compen_illuminatepercentages(BOOLEAN showonside);
static void     compen_insertbridge(CONTOURELEMENT *firstconel, CONTOURELEMENT *secondconel);
static BOOLEAN  compen_insert15degreesegment(INTBIG linesx, INTBIG linesy, INTBIG *lineex, INTBIG *lineey,
					INTBIG fromx, INTBIG fromy);
static void     compen_insertcontour(CONTOUR *newone, CONTOUR *toplevel);
static BOOLEAN  compen_intersect(INTBIG x1, INTBIG y1, double fang1, INTBIG x2, INTBIG y2, double fang2,
					INTBIG *x, INTBIG *y);
static BOOLEAN  compen_isinside(CONTOUR *lower, CONTOUR *higher);
static void     compen_movegeometry(CONTOUR *contourlist, NODEPROTO *np);
static void     compen_ordercontours(CONTOUR *contourlist, NODEPROTO *np);
static void     compen_orientcontours(CONTOUR *contourlist, NODEPROTO *np);
static INTBIG   compen_pointoffarc(CONTOURELEMENT *arcconel, INTBIG x, INTBIG y);
static void     compen_printdistance(double px, double py, INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty);
static void     compen_removecellcontours(NODEPROTO *np);
static void     compen_setglobalfactors(float metalthickness, float lrscompensation,
					float globalcompensation, float minimumsize, LIBRARY *lib);
static void     compen_setnodecompensation(NODEINST *ni, float amt, INTBIG mode);
static void     compen_straighten(CONTOURELEMENT *firstconel, CONTOURELEMENT *secondconel);
static float    compen_truecompensation(float percentage, float metalthickness, float lrscompensation);
static void     compen_uncompensatecell(NODEPROTO *np);

/******************************** CONTROL ********************************/

void compen_init(INTBIG *argc, CHAR1 *argv[], TOOL *thistool)
{
	/* miscellaneous initialization during pass 3 */
	if (thistool == 0)
	{
		compen_percentagekey = makekey(x_("COMPEN_percentage"));
		compen_precomppositionkey = makekey(x_("COMPEN_pre_compensation_position"));
		compen_globalcompensationkey = makekey(x_("COMPEN_globalcompensation"));
		compen_metalthicknesskey = makekey(x_("COMPEN_metalthickness"));
		compen_lrscompensationkey = makekey(x_("COMPEN_lrscompensation"));
		compen_minimumsizekey = makekey(x_("COMPEN_minimumsize"));
		compen_gds_layerkey = makekey(x_("IO_gds_layer"));
		compen_dxf_layerkey = makekey(x_("IO_dxf_layer"));
		compen_circletangentthresh = scalefromdispunit((float)CIRCLETANGENTTHRESH, DISPUNITMM);
		return;
	}

	/* ignore pass 2 */
	if (thistool == NOTOOL) return;

	/* copy tool pointer during pass 1 */
	compen_tool = thistool;
}

void compen_set(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG l, i, mode;
	REGISTER CHAR *pp;
	REGISTER NODEPROTO *np;
	REGISTER GEOM **list;
	float per1, per2, per3, comp1peredge, comp2peredge, comp3peredge, amt,
		metalthickness, lrscompensation, globalcompensation, minimumsize;

	l = estrlen(pp = par[0]);
	if (namesamen(pp, x_("compensate"), l) == 0)
	{
		np = getcurcell();
		if (np == NONODEPROTO)
		{
			ttyputerr(M_("No current cell"));
			return;
		}
		compen_compensatecell(np);
		return;
	}
	if (namesamen(pp, x_("connect-points"), l) == 0)
	{
		compen_connectpoints();
		return;
	}
	if (namesamen(pp, x_("detect-duplicate-geometry"), l) == 0 && l >= 8)
	{
		np = getcurcell();
		if (np == NONODEPROTO)
		{
			ttyputerr(M_("No current cell"));
			return;
		}
		i = compen_detectduplicates(np);
		if (i == 0) ttyputmsg(M_("No duplicate nodes found")); else
			ttyputmsg(M_("Detected %ld duplicate %s"), i, makeplural(M_("node"), i));
		return;
	}
	if (namesamen(pp, x_("detect-layer-intersections"), l) == 0 && l >= 8)
	{
		np = getcurcell();
		if (np == NONODEPROTO)
		{
			ttyputerr(M_("No current cell"));
			return;
		}
		compen_detectlayerintersections(np);
		return;
	}
	if (namesamen(pp, x_("detect-special-features"), l) == 0 && l >= 8)
	{
		np = getcurcell();
		if (np == NONODEPROTO)
		{
			ttyputerr(M_("No current cell"));
			return;
		}
		i = compen_detectfeatures(np);
		if (i == 0) ttyputmsg(M_("No features found")); else
		{
			compen_illuminatepercentages(TRUE);
			ttyputmsg(M_("Detected %ld %s"), i, makeplural(M_("feature"), i));
		}
		return;
	}
	if (namesamen(pp, x_("draw-pre-compensation"), l) == 0)
	{
		np = getcurcell();
		if (np == NONODEPROTO)
		{
			ttyputerr(M_("No current cell"));
			return;
		}
		list = us_gethighlighted(WANTNODEINST, 0, 0);
		compen_drawprecompensation(np, list);
		return;
	}
	if (namesamen(pp, x_("examine"), l) == 0 && l >= 1)
	{
		compen_examine();
		return;
	}
	if (namesamen(pp, x_("global-factors"), l) == 0)
	{
		if (count == 5)
		{
			metalthickness = (float)eatof(par[1]);
			lrscompensation = (float)eatof(par[2]);
			globalcompensation = (float)eatof(par[3]);
			minimumsize = (float)eatof(par[4]);
			compen_setglobalfactors(metalthickness, lrscompensation, globalcompensation, minimumsize,
				el_curlib);
		} else if (count == 2 && namesame(par[1], x_("dialog")) == 0)
		{
			compen_dofactorsdialog();
			return;
		} else if (count != 0)
		{
			ttyputusage(x_("telltool compensation global-factors ([dialog] | [METAL-THICKNESS LRS-COMPENSATION GLOBAL-COMPENSATION MINIMUM-FEATURE-SIZE])"));
			return;
		}
		compen_getglobalfactors(&metalthickness, &lrscompensation, &globalcompensation, &minimumsize,
			el_curlib);
		ttyputmsg(M_("Metal thickness is %g; LRS Compensation is %g; Global Compensation is %g; Minimum feature size is %g"),
			metalthickness, lrscompensation, globalcompensation, minimumsize);
		return;
	}
	if (namesamen(pp, x_("illuminate-percentages"), l) == 0)
	{
		compen_illuminatepercentages(TRUE);
		return;
	}
	if (namesamen(pp, x_("make-compensation-table"), l) == 0)
	{
		compen_getglobalfactors(&metalthickness, &lrscompensation, &globalcompensation, &minimumsize,
			el_curlib);
		ttyputmsg(M_("METRIC"));
		ttyputmsg(x_(" "));
		ttyputmsg(M_("Metal Thickness is %g        LRS Comp is %g (mm total), %g (mm per side)"),
			metalthickness, lrscompensation, lrscompensation/2.0);
		ttyputmsg(x_(" "));
		ttyputmsg(M_("ETCH    ADJUSTED 1/2 COMP        ETCH    ADJUSTED 1/2 COMP        ETCH    ADJUSTED 1/2 COMP"));
		ttyputmsg(M_("COMP    APPLIED PER EDGE         COMP    APPLIED PER EDGE         COMP    APPLIED PER EDGE"));
		for(i=165; i >= 124; i--)
		{
			per1 = (float)i;	per2 = (float)(i-42);	per3 = (float)(i-84);
			comp1peredge = compen_truecompensation(per1, metalthickness, lrscompensation);
			comp2peredge = compen_truecompensation(per2, metalthickness, lrscompensation);
			comp3peredge = compen_truecompensation(per3, metalthickness, lrscompensation);
			ttyputmsg(x_("%3g%%        %.6f             %3g%%        %.6f             %3g%%        %.6f"),
				per1, comp1peredge, per2, comp2peredge, per3, comp3peredge);
		}
		return;
	}
	if (namesamen(pp, x_("next-contour-selection"), l) == 0)
	{
		compen_advanceselection(1);
		return;
	}
	if (namesamen(pp, x_("percentage"), l) == 0 && l >= 2)
	{
		if (count > 2)
		{
			ttyputusage(x_("telltool compensation percentage [PERCENTAGE]"));
			return;
		}
		if (count == 1) mode = 1; else
		{
			mode = 0;
			amt = (float)eatof(par[1]);
			if (amt < 0.0)
			{
				ttyputerr(M_("Cannot set negative percentages of compensation"));
				return;
			}
		}
		list = us_gethighlighted(WANTNODEINST, 0, 0);
		if (list[0] == NOGEOM)
		{
			ttyputerr(M_("Select nodes before setting compensation percentages"));
			return;
		}
		for(i=0; list[i] != NOGEOM; i++)
			compen_setnodecompensation(list[i]->entryaddr.ni, amt, mode);
		if (mode == 0) compen_illuminatepercentages(FALSE);
		return;
	}
	if (namesamen(pp, x_("prev-contour-selection"), l) == 0 && l >= 2)
	{
		compen_advanceselection(0);
		return;
	}
	if (namesamen(pp, x_("remove-percentage"), l) == 0)
	{
		list = us_gethighlighted(WANTNODEINST, 0, 0);
		if (list[0] == NOGEOM)
		{
			ttyputerr(M_("Select nodes before removing compensation percentages"));
			return;
		}
		for(i=0; list[i] != NOGEOM; i++)
			compen_setnodecompensation(list[i]->entryaddr.ni, 0.0, -1);
		compen_illuminatepercentages(FALSE);
		return;
	}
	if (namesamen(pp, x_("show-contours"), l) == 0 && l >= 6)
	{
		np = getcurcell();
		if (np == NONODEPROTO)
		{
			ttyputerr(M_("No current cell"));
			return;
		}
		compen_drawcontours(np);
		return;
	}
	if (namesamen(pp, x_("show-non-contours"), l) == 0 && l >= 6)
	{
		np = getcurcell();
		if (np == NONODEPROTO)
		{
			ttyputerr(M_("No current cell"));
			return;
		}
		compen_detectnoncontours(np);
		return;
	}
	if (namesamen(pp, x_("uncompensate"), l) == 0)
	{
		np = getcurcell();
		if (np == NONODEPROTO)
		{
			ttyputerr(M_("No current cell"));
			return;
		}
		compen_uncompensatecell(np);
		return;
	}
	ttyputbadusage(x_("telltool compensation"));
}

void compen_done(void) {}

void compen_slice(void)
{
	REGISTER CELLCONTOURS *fc, *lastfc, *nextfc;
	REGISTER CONTOUR *con, *nextcon;
	REGISTER CONTOURELEMENT *conel;
	REGISTER USERDATA *ud;

	if (compen_deletedcellcontours == 0) return;
	lastfc = NOCELLCONTOURS;
	for(fc = compen_firstcellcontours; fc != NOCELLCONTOURS; fc = nextfc)
	{
		nextfc = fc->next;
		if (fc->deleted)
		{
			/* kill the contour information */
			for(con = fc->contour; con != NOCONTOUR; con = nextcon)
			{
				nextcon = con->nextcontour;
				for(conel = con->firstcontourelement; conel != NOCONTOURELEMENT; conel = conel->nextcontourelement)
				{
					ud = (USERDATA *)conel->userdata;
					if (ud->DXFlayer != 0) efree(ud->DXFlayer);
					efree((CHAR *)ud);
				}
				killcontour(con);
			}

			/* remove from the list and deallocate */
			if (lastfc == NOCELLCONTOURS) compen_firstcellcontours = fc->next; else
				lastfc->next = fc->next;
			efree((CHAR *)fc);
			continue;
		}
		lastfc = fc;
	}
	compen_deletedcellcontours = 0;
}

/******************************** DATABASE CHANGES ********************************/

void compen_modifynodeinst(NODEINST *ni, INTBIG olx, INTBIG oly, INTBIG ohx, INTBIG ohy,
	INTBIG orot, INTBIG otran)
{
	/* if a node changes, force recaching of the contour information */
	compen_removecellcontours(ni->parent);
}

void compen_killobject(INTBIG addr, INTBIG type)
{
	/* if a node is deleted, force recaching of the contour information */
	if (type == VNODEINST)
		compen_removecellcontours(((NODEINST *)addr)->parent);
}

/******************************** COMMANDS ********************************/

/*
 * Routine to describe the currently selected object(s).
 */
void compen_examine(void)
{
	REGISTER VARIABLE *var;
	REGISTER NODEINST *ni;
	REGISTER INTBIG len, cx, cy, radius;
	REGISTER INTBIG i, j, prev, first;
	INTBIG bx, by, p1x, p1y, p2x, p2y, fx, fy, tx, ty;
	REGISTER GEOM **list;
	HIGHLIGHT thishigh, otherhigh;
	double startoffset, endangle, rot;
	XARRAY trans;
	REGISTER void *infstr;

	/* special case when two objects are highlighted */
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var != NOVARIABLE)
	{
		len = getlength(var);
		if (len == 2)
		{
			if (!us_makehighlight(((CHAR **)var->addr)[0], &thishigh) &&
				!us_makehighlight(((CHAR **)var->addr)[1], &otherhigh))
			{
				/* describe these two objects */
				if ((thishigh.status&HIGHSNAP) != 0 && (otherhigh.status&HIGHSNAP) != 0)
				{
					us_getsnappoint(&thishigh, &p1x, &p1y);
					us_getsnappoint(&otherhigh, &p2x, &p2y);
					ttyputmsg(M_("Distance between snap points is %s (%s in X and %s in Y)"),
						latoa(computedistance(p1x,p1y, p2x,p2y), 0), latoa(labs(p2x-p1x), 0), latoa(labs(p2y-p1y), 0));
					return;
				}
			}
		}
	}

	/* get what is highlighted */
	list = us_gethighlighted(WANTNODEINST, 0, 0);
	if (list[0] == NOGEOM)
	{
		ttyputerr(M_("Nothing is selected"));
		return;
	}

	/* describe each selected object */
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (!list[i]->entryisnode) continue;
		ni = list[i]->entryaddr.ni;

		if (ni->proto->primindex == 0)
		{
			/* describe instance */
			infstr = initinfstr();
			formatinfstr(infstr, M_("Instance of %s"), describenodeinst(ni));
			ttyputmsg(x_("%s"), returninfstr(infstr));
			corneroffset(ni, ni->proto, ni->rotation, ni->transpose, &bx, &by, FALSE);
			bx += ni->lowx;   by += ni->lowy;
			ttyputmsg(M_("   at point (%s, %s)"), latoa(bx, 0), latoa(by, 0));
			ttyputmsg(M_("   rotation = %d"), (ni->rotation+5)/10);
			continue;
		}

		/* describe it */
		infstr = initinfstr();
		if (ni->proto == art_openedpolygonprim || ni->proto == art_closedpolygonprim)
		{
			addstringtoinfstr(infstr, M_("Line"));
		} else if (ni->proto == art_circleprim || ni->proto == art_thickcircleprim)
		{
			getarcdegrees(ni, &startoffset, &endangle);
			if (startoffset == 0.0 && endangle == 0.0) addstringtoinfstr(infstr, M_("Circle")); else
				addstringtoinfstr(infstr, M_("Arc"));
		} else if (ni->proto == gen_invispinprim)
		{
			addstringtoinfstr(infstr, M_("Text"));
		} else if (ni->proto == gen_cellcenterprim)
		{
			addstringtoinfstr(infstr, M_("Cell center"));
		} else
		{
			addstringtoinfstr(infstr, M_("Unknown object"));
		}

		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
		if (var != NOVARIABLE)
			formatinfstr(infstr, x_(" [%s]"), (CHAR *)var->addr);
		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, compen_dxf_layerkey);
		if (var != NOVARIABLE)
			formatinfstr(infstr, M_(", DXF Layer: %s"), (CHAR *)var->addr);
		var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, compen_gds_layerkey);
		if (var != NOVARIABLE)
			formatinfstr(infstr, M_(", GDS Layer: %ld"), var->addr);
		ttyputmsg(x_("%s"), returninfstr(infstr));

		cx = (ni->lowx + ni->highx) / 2;
		cy = (ni->lowy + ni->highy) / 2;
		if (ni->proto == art_openedpolygonprim || ni->proto == art_closedpolygonprim)
		{
			var = gettrace(ni);
			if (var != NOVARIABLE)
			{
				len = getlength(var);
				makerot(ni, trans);
				first = 1;
				for(j=0; j<len; j += 2)
				{
					if (j == 0)
					{
						if (ni->proto == art_openedpolygonprim) continue;
						prev = len - 2;
					} else prev = j - 2;
					xform(((INTBIG *)var->addr)[prev]+cx, ((INTBIG *)var->addr)[prev+1]+cy, &fx, &fy, trans);
					xform(((INTBIG *)var->addr)[j]+cx, ((INTBIG *)var->addr)[j+1]+cy, &tx, &ty, trans);
					if (first != 0) ttyputmsg(M_("   from point (%s, %s)"), latoa(fx, 0), latoa(fy, 0));
					first = 0;
					ttyputmsg(M_("     to point (%s, %s)"), latoa(tx, 0), latoa(ty, 0));
					if (ty == fy && tx == fx)
					{
						ttyputerr(M_("Domain error examining line"));
						break;
					}
					rot = atan2((double)(ty-fy), (double)(tx-fx));
					if (rot < 0.0) rot += EPI*2.0;
					ttyputmsg(M_("       length = %s,  angle = %g"), latoa(computedistance(fx,fy,tx,ty), 0), rot*180.0/EPI);
					ttyputmsg(M_("       delta X = %s,  delta Y = %s"), latoa(tx-fx, 0), latoa(ty-fy, 0));
				}
			}
		} else if (ni->proto == art_circleprim || ni->proto == art_thickcircleprim)
		{
			getarcdegrees(ni, &startoffset, &endangle);
			if (startoffset == 0.0 && endangle == 0.0)
			{
				ttyputmsg(M_("   center point (%s, %s)"), latoa(cx, 0), latoa(cy, 0));
				radius = (ni->highx - ni->lowx) / 2;
				ttyputmsg(M_("          radius = %s"), latoa(radius, 0));
				ttyputmsg(M_("        diameter = %s"), latoa(radius*2, 0));
				ttyputmsg(M_("   circumference = %s"), latoa(roundfloat((float)(radius*2.0*EPI)), 0));
			} else
			{
				ttyputmsg(M_("   center point (%s, %s)"), latoa(cx, 0), latoa(cy, 0));
				radius = (ni->highx - ni->lowx) / 2;
				ttyputmsg(M_("        radius = %s"), latoa(radius, 0));
				startoffset += ((double)ni->rotation) * EPI / 1800.0;
				if (ni->transpose != 0)
				{
					startoffset = 1.5 * EPI - startoffset - endangle;
					if (startoffset < 0.0) startoffset += EPI * 2.0;
				}
				fx = cx + rounddouble(cos(startoffset) * (double)radius);
				fy = cy + rounddouble(sin(startoffset) * (double)radius);
				tx = cx + rounddouble(cos(startoffset+endangle) * (double)radius);
				ty = cy + rounddouble(sin(startoffset+endangle) * (double)radius);
				ttyputmsg(M_("   start angle = %g, point (%s, %s)"), startoffset*180.0/EPI,
					latoa(fx, 0), latoa(fy, 0));
				ttyputmsg(M_("     end angle = %g, point (%s, %s)"), (startoffset+endangle)*180.0/EPI,
					latoa(tx, 0), latoa(ty, 0));
			}
		} else
		{
			ttyputmsg(M_("   center point (%s, %s)"), latoa(cx, 0), latoa(cy, 0));
		}

		var = getvalkey((INTBIG)ni, VNODEINST, VFLOAT, compen_percentagekey);
		if (var == NOVARIABLE)
			ttyputmsg(M_("   No compensation value set")); else
				ttyputmsg(M_("   Has %g%% compensation set on it"), castfloat(var->addr));
	}
}

/*
 * Routine to move the selection to the next (if "next" is nonzero) or previous one
 * in the contour.
 */
void compen_advanceselection(short next)
{
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER CONTOUR *con, *contourlist;
	REGISTER CONTOURELEMENT *conel, *prevconel, *thisconel;
	HIGHLIGHT newhigh;

	ni = (NODEINST *)us_getobject(VNODEINST, FALSE);
	if (ni == NONODEINST)
	{
		ttyputerr(M_("Must select a single node before advancing to next in contour"));
		return;
	}

	/* make sure there are contours for this cell */
	np = getcurcell();
	contourlist = compen_getcontourlist(np);
	if (contourlist == NOCONTOUR)
	{
		ttyputerr(M_("Sorry, no contour information can be found in this cell"));
		return;
	}

	/* find the node in the contour list */
	for(con = contourlist; con != NOCONTOUR; con = con->nextcontour)
	{
		for(conel = con->firstcontourelement; conel != NOCONTOURELEMENT; conel = conel->nextcontourelement)
			if (conel->ni == ni) break;
		if (conel != NOCONTOURELEMENT) break;
	}
	if (con == NOCONTOUR)
	{
		ttyputerr(M_("This node is not in a contour"));
		return;
	}

	if (next != 0)
	{
		/* get next contour element */
		for(;;)
		{
			conel = conel->nextcontourelement;
			if (conel == NOCONTOURELEMENT) conel = con->firstcontourelement;
			if (conel->ni != NONODEINST) break;
		}
	} else
	{
		/* get previous contour element */
		for(;;)
		{
			prevconel = NOCONTOURELEMENT;
			for(thisconel = con->firstcontourelement; thisconel != NOCONTOURELEMENT;
				thisconel = thisconel->nextcontourelement)
			{
				if (thisconel == conel) break;
				prevconel = thisconel;
			}
			conel = prevconel;
			if (prevconel == NOCONTOURELEMENT)
			{
				for(thisconel = con->firstcontourelement; thisconel != NOCONTOURELEMENT;
					thisconel = thisconel->nextcontourelement) conel = thisconel;
			}

			if (conel->ni != NONODEINST) break;
		}
	}

	/* highlight it */
	newhigh.status = HIGHFROM;
	newhigh.fromgeom = conel->ni->geom;
	newhigh.fromport = conel->ni->proto->firstportproto;
	newhigh.fromvar = NOVARIABLE;
	newhigh.fromvarnoeval = NOVARIABLE;
	newhigh.frompoint = 0;
	newhigh.cell = conel->ni->parent;
	us_setfind(&newhigh, 0, 0, 0, 1);
}

void compen_drawprecompensation(NODEPROTO *np, GEOM **list)
{
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var, *vartrace;
	INTBIG fx, fy, tx, ty;
	REGISTER INTBIG i, total, badprecomps, goodprecomps;
	double px, py, startoffset, endangle;
	REGISTER INTBIG cx, cy, oldradius, newradius, *precomparray, compamt, afx, afy, atx, aty;

	if (list[0] == NOGEOM)
	{
		/* nothing in list: select all */
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			ni->temp1 = 1;
	} else
	{
		/* mark only the desired nodes */
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			ni->temp1 = 0;
		for(i=0; list[i] != NOGEOM; i++)
			list[i]->entryaddr.ni->temp1 = 1;
	}
	total = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		if (ni->temp1 != 0) total++;

	compen_rline.col = RED;
	badprecomps = goodprecomps = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->temp1 == 0) continue;
		var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER|VISARRAY, compen_precomppositionkey);
		if (var == NOVARIABLE) continue;
		precomparray = (INTBIG *)var->addr;
		if (precomparray[PRECOMPMAGIC] != 0xFEED)
		{
			badprecomps = 1;
			(void)delvalkey((INTBIG)ni, VNODEINST, compen_precomppositionkey);
			continue;
		}
		goodprecomps = 1;
		compamt = precomparray[PRECOMPPERC];
		switch (precomparray[PRECOMPTYPE])
		{
			case ARCSEGMENTTYPE:
			case REVARCSEGMENTTYPE:
				if (total == 1)
				{
					/* display difference information */
					getarcdegrees(ni, &startoffset, &endangle);
					if (startoffset != 0.0 || endangle != 0.0)
					{
						cx = (ni->lowx + ni->highx) / 2;   cy = (ni->lowy + ni->highy) / 2;
						fx = precomparray[PRECOMPARCSX];
						fy = precomparray[PRECOMPARCSY];
						tx = precomparray[PRECOMPARCEX];
						ty = precomparray[PRECOMPARCEY];
						oldradius = precomparray[PRECOMPARCRAD];
						ttyputmsg(M_("Arc ran from (%s,%s) to (%s,%s) with radius %s before compensation"),
							latoa(fx, 0), latoa(fy, 0), latoa(tx, 0), latoa(ty, 0), latoa(oldradius, 0));
						newradius = (ni->highx - ni->lowx) / 2;
						getarcendpoints(ni, startoffset, endangle, &fx, &fy, &tx, &ty);
						ttyputmsg(M_("        from (%s,%s) to (%s,%s) with radius %s after compensation"),
							latoa(fx, 0), latoa(fy, 0), latoa(tx, 0), latoa(ty, 0), latoa(newradius, 0));
						ttyputmsg(M_("Compensated %ld%%, radius changed by %s"), compamt,
							latoa(labs(newradius-oldradius), 0));
					}
				}
				if (precomparray[PRECOMPTYPE] == ARCSEGMENTTYPE)
				{
					compen_drawcirclearc((ni->lowx + ni->highx) / 2, (ni->lowy + ni->highy) / 2,
						precomparray[PRECOMPARCEX], precomparray[PRECOMPARCEY], precomparray[PRECOMPARCSX],
							precomparray[PRECOMPARCSY], &compen_rline);
				} else
				{
					compen_drawcirclearc((ni->lowx + ni->highx) / 2, (ni->lowy + ni->highy) / 2,
						precomparray[PRECOMPARCSX], precomparray[PRECOMPARCSY], precomparray[PRECOMPARCEX],
							precomparray[PRECOMPARCEY], &compen_rline);
				}
				break;

			case CIRCLESEGMENTTYPE:
				cx = (ni->lowx + ni->highx) / 2;   cy = (ni->lowy + ni->highy) / 2;
				if (total == 1)
				{
					/* display difference information */
					oldradius = precomparray[PRECOMPCIRCRAD];
					newradius = computedistance(cx, cy, ni->highx, cy);
					ttyputmsg(M_("Circle had %s radius before compensation"), latoa(oldradius, 0));
					ttyputmsg(M_("           %s radius after compensation"), latoa(newradius, 0));
					ttyputmsg(M_("Compensated %ld%%, radius changed by %s"), compamt,
						latoa(labs(newradius-oldradius), 0));
				}
				compen_drawcircle(cx, cy, precomparray[PRECOMPCIRCSX],
					precomparray[PRECOMPCIRCSY], &compen_rline);
				break;

			case LINESEGMENTTYPE:
			case BRIDGESEGMENTTYPE:
				if (total == 1)
				{
					/* display difference information */
					vartrace = gettrace(ni);
					if (vartrace != NOVARIABLE)
					{
						fx = precomparray[PRECOMPLINESX];
						fy = precomparray[PRECOMPLINESY];
						tx = precomparray[PRECOMPLINEEX];
						ty = precomparray[PRECOMPLINEEY];
						ttyputmsg(M_("Line ran from (%s,%s) to (%s,%s) before compensation"), latoa(fx, 0),
							latoa(fy, 0), latoa(tx, 0), latoa(ty, 0));
						cx = (ni->lowx + ni->highx) / 2;   cy = (ni->lowy + ni->highy) / 2;
						afx = ((INTBIG *)vartrace->addr)[0] + cx;
						afy = ((INTBIG *)vartrace->addr)[1] + cy;
						atx = ((INTBIG *)vartrace->addr)[2] + cx;
						aty = ((INTBIG *)vartrace->addr)[3] + cy;
						ttyputmsg(M_("         from (%s,%s) to (%s,%s) after compensation"), latoa(afx, 0),
							latoa(afy, 0), latoa(atx, 0), latoa(aty, 0));
						px = ((double)(fx+tx)) * 0.5;   py = ((double)(fy+ty)) * 0.5;
						compen_printdistance(px, py, afx, afy, atx, aty);
						ttyputmsg(M_("Compensated %ld%%"), compamt);
					}
				}
				compen_drawline(precomparray[PRECOMPLINESX], precomparray[PRECOMPLINESY], precomparray[PRECOMPLINEEX],
					precomparray[PRECOMPLINEEY], &compen_rline);
				break;
		}
	}
	if (badprecomps != 0)
	{
		ttyputerr(M_("Sorry, some precompensation information is obsolete"));
		return;
	}
	if (goodprecomps == 0)
		ttyputmsg(M_("No precompensation information is available"));
}

/*
 * Routine to compensate cell "uncompnp" and create a new, compensated one.
 */
void compen_compensatecell(NODEPROTO *uncompnp)
{
	REGISTER CONTOUR *con, *contourlist;
	REGISTER VARIABLE *var;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER INTBIG total;
	CHAR *par[2], *compname;
	REGISTER void *infstr;

	/* see if this cell is already compensated */
	if (uncompnp->cellview == el_compview)
	{
		ttyputerr(M_("This cell is already compensated"));
		return;
	}
	for(ni = uncompnp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER|VISARRAY, compen_precomppositionkey);
		if (var == NOVARIABLE) continue;
		ttyputerr(M_("This cell is already compensated"));
		(void)changecellview(uncompnp, el_compview);
		return;
	}

	/* see if there is a compensated cell that is newer */
	FOR_CELLGROUP(np, uncompnp)
		if (np->cellview == el_compview && np->revisiondate > uncompnp->revisiondate)
	{
		par[0] = describenodeproto(np);
		us_editcell(1, par);
		return;
	}

#ifdef DEBDUMP
	{
		CHAR *truename;
		compen_io = xcreate(x_("compen dump"), el_filetypetext, 0, &truename);
	}
#endif
	ttyputmsg(M_("Compensating..."));

	/* make a copy with the appropriate view */
	infstr = initinfstr();
	addstringtoinfstr(infstr, uncompnp->protoname);
	addstringtoinfstr(infstr, x_("{comp}"));
	compname = returninfstr(infstr);
	np = getnodeproto(compname);
	if (np != NONODEPROTO)
	{
		(void)killnodeproto(np);
	}
	np = copynodeproto(uncompnp, uncompnp->lib, compname, FALSE);
	if (np == NONODEPROTO)
	{
		ttyputerr(M_("Could not create compensated view of this cell"));
		return;
	}

	/* recompute bounds */
	(*el_curconstraint->solve)(np);

	/* collect contours in this cell */
	contourlist = compen_getcontourlist(np);
	if (contourlist == NOCONTOUR) return;

	/* compute bounding boxes */
	total = 0;
	for(con = contourlist; con != NOCONTOUR; con = con->nextcontour)
	{
		getcontourbbox(con, &con->lx, &con->hx, &con->ly, &con->hy);
		total++;
	}

	/* make sure all ends meet exactly */
	compen_cleancontours(contourlist, np, FALSE);

	/* make sure the contours go in the same direction */
	compen_orientcontours(contourlist, np);

	/* order the contours */
	compen_ordercontours(contourlist, np);

	/* assign percentages and compensate */
	compen_assignpercentages(contourlist, np);
	compen_movegeometry(contourlist, np);

	/* blend endpoints correctly */
	compen_cleancontours(contourlist, np, TRUE);

	compen_adjustgeometry(contourlist, np);
	ttyputmsg(M_("...Done Compensating"));

#ifdef DEBDUMP
	xclose(compen_io);
#endif

	/* show the compensated cell */
	par[0] = describenodeproto(np);
	us_editcell(1, par);
}

/*
 * Routine to uncompensate cell "np" by reverting to the old one or creating it if necessary.
 */
void compen_uncompensatecell(NODEPROTO *np)
{
	REGISTER CONTOUR *con, *contourlist;
	REGISTER CONTOURELEMENT *conel;
	REGISTER INTBIG lx, hx, ly, hy, radius, xc, yc, *prevdata;
	INTBIG coord[6];
	REGISTER VARIABLE *var;
	CHAR *par[2];
	REGISTER NODEPROTO *uncompnp;
	REGISTER NODEINST *ni;
	REGISTER INTBIG newrot, badprecomps;
	double fx, fy, srot, erot, fswap;
	float percentage;

	/* uncompensation is easy if the original is in a different view */
	if (np->cellview == el_compview)
	{
		FOR_CELLGROUP(uncompnp, np)
			if (uncompnp->cellview != el_compview)
		{
			par[0] = describenodeproto(uncompnp);
			us_editcell(1, par);
			return;
		}
	}

	/* see if this cell is compensated */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER|VISARRAY, compen_precomppositionkey);
		if (var != NOVARIABLE) break;
	}
	if (ni == NONODEINST)
	{
		ttyputerr(M_("Cannot uncompensate: compensation has not been done"));
		return;
	}
	if (np->cellview != el_compview) (void)changecellview(np, el_compview);

	/* make a copy of this in an uncompensated cell */
	uncompnp = copynodeproto(np, np->lib, np->protoname, FALSE);
	if (uncompnp == NONODEPROTO)
	{
		ttyputerr(M_("Could not create uncompensated view of this cell"));
		return;
	}
	(void)changecellview(uncompnp, el_unknownview);

	/* recompute bounds */
	(*el_curconstraint->solve)(uncompnp);

	/* collect contours in this cell */
	contourlist = compen_getcontourlist(uncompnp);
	if (contourlist == NOCONTOUR) return;

	/* loop through every element in every contour */
	badprecomps = 0;
	for(con = contourlist; con != NOCONTOUR; con = con->nextcontour)
	{
		if (con->valid == 0) continue;
		for(conel = con->firstcontourelement; conel != NOCONTOURELEMENT; conel = conel->nextcontourelement)
		{
			if (conel->ni == NONODEINST) continue;

			/* restore previous */
			if (conel->elementtype == BRIDGESEGMENTTYPE)
			{
				/* delete node: previous didn't exist */
				startobjectchange((INTBIG)conel->ni, VNODEINST);
				(void)killnodeinst(conel->ni);
				conel->ni = NONODEINST;
				continue;
			}

			var = getvalkey((INTBIG)conel->ni, VNODEINST, VINTEGER|VISARRAY, compen_precomppositionkey);
			if (var == NOVARIABLE) continue;
			prevdata = (INTBIG *)var->addr;
			if (prevdata[PRECOMPMAGIC] != 0xFEED)
			{
				badprecomps = 1;
				(void)delvalkey((INTBIG)conel->ni, VNODEINST, compen_precomppositionkey);
				continue;
			}
			if (prevdata[PRECOMPTYPE] == BRIDGESEGMENTTYPE)
			{
				/* delete node: previous didn't exist */
				startobjectchange((INTBIG)conel->ni, VNODEINST);
				(void)killnodeinst(conel->ni);
				conel->ni = NONODEINST;
				continue;
			}

			/* start changes */
			startobjectchange((INTBIG)conel->ni, VNODEINST);

			switch (conel->elementtype)
			{
				case LINESEGMENTTYPE:
					if (prevdata[PRECOMPTYPE] != LINESEGMENTTYPE) break;

					/* create the new line node */
					lx = mini(prevdata[PRECOMPLINESX], prevdata[PRECOMPLINEEX]);
					hx = maxi(prevdata[PRECOMPLINESX], prevdata[PRECOMPLINEEX]);
					ly = mini(prevdata[PRECOMPLINESY], prevdata[PRECOMPLINEEY]);
					hy = maxi(prevdata[PRECOMPLINESY], prevdata[PRECOMPLINEEY]);
					modifynodeinst(conel->ni, lx - conel->ni->lowx, ly - conel->ni->lowy,
						hx - conel->ni->highx, hy - conel->ni->highy, 0, 0);

					/* store line coordinates */
					xc = (lx + hx) / 2;   yc = (ly + hy) / 2;
					coord[0] = prevdata[PRECOMPLINESX] - xc;		coord[1] = prevdata[PRECOMPLINESY] - yc;
					coord[2] = prevdata[PRECOMPLINEEX] - xc;		coord[3] = prevdata[PRECOMPLINEEY] - yc;
					(void)setvalkey((INTBIG)conel->ni, VNODEINST, el_trace_key, (INTBIG)coord,
						VINTEGER|VISARRAY|(4<<VLENGTHSH));

					/* store compensation percentage (if not a global value) */
					if (prevdata[PRECOMPEXPPERC] != 0)
					{
						percentage = (float)prevdata[PRECOMPPERC];
						(void)setvalkey((INTBIG)conel->ni, VNODEINST, compen_percentagekey,
							castint(percentage), VFLOAT);
					}
					break;

				case CIRCLESEGMENTTYPE:
					if (prevdata[PRECOMPTYPE] != CIRCLESEGMENTTYPE) break;

					/* set new size according to new radius */
					radius = prevdata[PRECOMPCIRCRAD];
					lx = conel->cx - radius;   hx = conel->cx + radius;
					ly = conel->cy - radius;   hy = conel->cy + radius;
					modifynodeinst(conel->ni, lx - conel->ni->lowx, ly - conel->ni->lowy,
						hx - conel->ni->highx, hy - conel->ni->highy, 0, 0);
					break;

				case ARCSEGMENTTYPE:
				case REVARCSEGMENTTYPE:
					if (prevdata[PRECOMPTYPE] != ARCSEGMENTTYPE && prevdata[PRECOMPTYPE] != REVARCSEGMENTTYPE) break;

					/* set new size and arc information */
					radius = prevdata[PRECOMPARCRAD];
					lx = conel->cx - radius;   hx = conel->cx + radius;
					ly = conel->cy - radius;   hy = conel->cy + radius;

					fx = prevdata[PRECOMPARCSX] - conel->cx;   fy = prevdata[PRECOMPARCSY] - conel->cy;
					if (fy == 0.0 && fx == 0.0)
					{
						ttyputerr(M_("Domain error uncompensating arc end 1"));
						break;
					}
					srot = atan2(fy, fx);
					if (srot < 0.0) srot += EPI*2.0;
					fx = prevdata[PRECOMPARCEX] - conel->cx;   fy = prevdata[PRECOMPARCEY] - conel->cy;
					if (fy == 0.0 && fx == 0.0)
					{
						ttyputerr(M_("Domain error uncompensating arc end 2"));
						break;
					}
					erot = atan2(fy, fx);
					if (erot < 0.0) erot += EPI*2.0;
					if (prevdata[PRECOMPTYPE] == REVARCSEGMENTTYPE)
					{
						fswap = srot;   srot = erot;   erot = fswap;
					}
					erot -= srot;
					if (erot < 0.0) erot += EPI*2.0;
					newrot = rounddouble(srot * 1800.0 / EPI);
					srot -= ((double)newrot) * EPI / 1800.0;
					modifynodeinst(conel->ni, lx - conel->ni->lowx, ly - conel->ni->lowy,
						hx - conel->ni->highx, hy - conel->ni->highy,
							newrot - conel->ni->rotation, -conel->ni->transpose);
					setarcdegrees(conel->ni, srot, erot);
					break;

				default:
					break;
			}

			/* delete previous compensation data */
			(void)delvalkey((INTBIG)conel->ni, VNODEINST, compen_precomppositionkey);

			/* end of changes */
			endobjectchange((INTBIG)conel->ni, VNODEINST);
		}
	}
	if (badprecomps != 0)
	{
		ttyputerr(M_("Sorry, some precompensation information is obsolete"));
		return;
	}

	/* show the compensated cell */
	par[0] = describenodeproto(uncompnp);
	us_editcell(1, par);
	compen_illuminatepercentages(FALSE);
}

/*
 * Routine to automatically detect special features described in
 * "library:~.COMPEN_automatic_features".  It has this format:
 *    "circle DIAMETERinMM PERCENTAGE"
 *    "slot LENGTHinMM WIDTHinMM ENDCAPCOMP SIDECOMP
 */
INTBIG compen_detectfeatures(NODEPROTO *np)
{
	REGISTER INTBIG len, i, autofeatures, diameter, radius2, radius4, length;
	REGISTER INTBIG featuresfound;
	REGISTER VARIABLE *var;
	REGISTER CONTOUR *con, *contourlist;
	REGISTER CONTOURELEMENT *conel, *conel1, *conel2, *conel3, *conel4;
	REGISTER AUTOFEATURE *af;
	REGISTER CHAR *pp, *pporig, **featurelist;
	float percentage;

	/* collect contours in this cell */
	contourlist = compen_getcontourlist(np);
	if (contourlist == NOCONTOUR) return(0);

	/* get the "text" list of automatic feature descriptions */
	var = getval((INTBIG)el_curlib, VLIBRARY, VSTRING | VISARRAY, x_("COMPEN_automatic_features"));
	if (var == NOVARIABLE) return(0);
	featurelist = (CHAR **)var->addr;
	len = getlength(var);

	/* make space for the "internal" list */
	af = (AUTOFEATURE *)emalloc(len * (sizeof (AUTOFEATURE)), compen_tool->cluster);
	if (af == 0) return(0);

	/* convert text list to internal list */
	autofeatures = 0;
	for(i=0; i<len; i++)
	{
		pporig = pp = featurelist[i];
		while (*pp == ' ' || *pp == '\t') pp++;
		if (*pp == ';' || *pp == 0) continue;
		if (namesamen(pp, x_("circle"), 6) == 0)
		{
			/* circle feature */
			pp += 6;
			while (*pp == ' ' || *pp == '\t') pp++;
			af[autofeatures].type = 0;
			af[autofeatures].diameter = scalefromdispunit((float)eatof(pp), DISPUNITMM);
			while (*pp != ' ' && *pp != '\t' && *pp != 0) pp++;
			while (*pp == ' ' || *pp == '\t') pp++;
			af[autofeatures].percentage = eatoi(pp);
			if (*pp != 0)
			{
				autofeatures++;
				continue;
			}
		} else if (namesamen(pp, x_("slot"), 4) == 0)
		{
			/* slot feature */
			pp += 4;
			while (*pp == ' ' || *pp == '\t') pp++;
			af[autofeatures].type = 1;
			af[autofeatures].length = scalefromdispunit((float)eatof(pp), DISPUNITMM);
			while (*pp != ' ' && *pp != '\t' && *pp != 0) pp++;
			while (*pp == ' ' || *pp == '\t') pp++;

			af[autofeatures].diameter = scalefromdispunit((float)eatof(pp), DISPUNITMM);
			while (*pp != ' ' && *pp != '\t' && *pp != 0) pp++;
			while (*pp == ' ' || *pp == '\t') pp++;

			af[autofeatures].lenpercentage = eatoi(pp);
			while (*pp != ' ' && *pp != '\t' && *pp != 0) pp++;
			while (*pp == ' ' || *pp == '\t') pp++;

			af[autofeatures].percentage = eatoi(pp);
			if (*pp != 0)
			{
				autofeatures++;
				continue;
			}
		}
		ttyputerr(M_("Error in automatic feature compensation specification: '%s'"), pporig);
		break;
	}

	/* now detect special coutours */
	featuresfound = 0;
	for(con = contourlist; con != NOCONTOUR; con = con->nextcontour)
	{
		if (con->valid == 0) continue;
		conel = con->firstcontourelement;

		if (conel->elementtype == CIRCLESEGMENTTYPE)
		{
			/* check for circles */
			diameter = computedistance(conel->sx, conel->sy, conel->cx, conel->cy) * 2;
			for(i=0; i<autofeatures; i++)
			{
				if (af[i].type != 0) continue;
				if (diameter == af[i].diameter)
				{
					percentage = (float)af[i].percentage;
					(void)setvalkey((INTBIG)conel->ni, VNODEINST, compen_percentagekey,
						castint(percentage), VFLOAT);
					featuresfound++;
					break;
				}
			}
		} else
		{
			/* check for slots */
			i = 0;
			for(; conel != NOCONTOURELEMENT; conel = conel->nextcontourelement) i++;
			if (i == 4)
			{
				conel1 = con->firstcontourelement;
				conel2 = conel1->nextcontourelement;
				conel3 = conel2->nextcontourelement;
				conel4 = conel3->nextcontourelement;
				if (conel1->elementtype == ARCSEGMENTTYPE || conel1->elementtype == REVARCSEGMENTTYPE)
				{
					conel = conel1;   conel1 = conel2;   conel2 = conel3;   conel3 = conel4;   conel4 = conel;
				}
				if (conel1->elementtype == LINESEGMENTTYPE && conel3->elementtype == LINESEGMENTTYPE &&
					(conel2->elementtype == ARCSEGMENTTYPE || conel2->elementtype == REVARCSEGMENTTYPE) &&
					(conel4->elementtype == ARCSEGMENTTYPE || conel4->elementtype == REVARCSEGMENTTYPE))
				{
					radius2 = computedistance(conel2->sx, conel2->sy, conel2->cx, conel2->cy);
					radius4 = computedistance(conel4->sx, conel4->sy, conel4->cx, conel4->cy);
					length = computedistance(conel2->cx, conel2->cy, conel4->cx, conel4->cy);
					length += radius2 + radius4;
					for(i=0; i<autofeatures; i++)
					{
						if (af[i].type != 1) continue;
						if (radius2 == radius4 && radius2 * 2 == af[i].diameter && length == af[i].length)
						{
							percentage = (float)af[i].percentage;
							(void)setvalkey((INTBIG)conel1->ni, VNODEINST, compen_percentagekey,
								castint(percentage), VFLOAT);
							(void)setvalkey((INTBIG)conel3->ni, VNODEINST, compen_percentagekey,
								castint(percentage), VFLOAT);

							percentage = (float)af[i].lenpercentage;
							(void)setvalkey((INTBIG)conel2->ni, VNODEINST, compen_percentagekey,
								castint(percentage), VFLOAT);
							(void)setvalkey((INTBIG)conel4->ni, VNODEINST, compen_percentagekey,
								castint(percentage), VFLOAT);
							featuresfound++;
							break;
						}
					}
				}
			}
		}
	}

	efree((CHAR *)af);
	return(featuresfound);
}

/*
 * routine to examine all contours in the cell and find where dielectric crosses stainless.  At those
 * points, the dielectric is broken into two pieces.
 */
void compen_detectlayerintersections(NODEPROTO *np)
{
	REGISTER CONTOUR *con, *ocon, *contourlist;
	REGISTER CONTOURELEMENT *conel, *oconel;
	REGISTER USERDATA *ud, *oud;
	REGISTER double ang, oang;
	INTBIG ix, iy;

	/* collect contours in this cell */
	contourlist = compen_getcontourlist(np);
	if (contourlist == NOCONTOUR) return;

	/* compute the bounding boxes */
	for(con = contourlist; con != NOCONTOUR; con = con->nextcontour)
	{
		if (con->valid == 0) continue;
		for(conel = con->firstcontourelement; conel != NOCONTOURELEMENT; conel = conel->nextcontourelement)
		{
			ud = (USERDATA *)conel->userdata;
			getcontourelementbbox(conel, &ud->lowx, &ud->highx, &ud->lowy, &ud->highy);
		}
	}

	/* examine all geometry */
	for(con = contourlist; con != NOCONTOUR; con = con->nextcontour)
	{
		if (con->valid == 0) continue;
		for(conel = con->firstcontourelement; conel != NOCONTOURELEMENT; conel = conel->nextcontourelement)
		{
			/* ignore if this is a circle or not dielectric layer */
			if (conel->elementtype == CIRCLESEGMENTTYPE) continue;
			ud = (USERDATA *)conel->userdata;
			if (ud->DXFlayer == 0) continue;
			if (namesame(ud->DXFlayer, x_("DIELECTRIC")) != 0) continue;

			/* found piece of dielectric: see if stainless intersects it */
			for(ocon = contourlist; ocon != NOCONTOUR; ocon = ocon->nextcontour)
			{
				if (ocon->valid == 0) continue;
				for(oconel = ocon->firstcontourelement; oconel != NOCONTOURELEMENT; oconel = oconel->nextcontourelement)
				{
					/* ignore if this is a circle or not stainless layer */
					if (oconel->elementtype == CIRCLESEGMENTTYPE) continue;
					oud = (USERDATA *)oconel->userdata;
					if (oud->DXFlayer == 0) continue;
					if (namesame(oud->DXFlayer, x_("SST")) != 0) continue;

					/* trivial intersection test */
					if (ud->highx < oud->lowx || ud->lowx > oud->highx ||
						ud->highy < oud->lowy || ud->lowy > oud->highy) continue;

					/* more advanced test */
					if (conel->elementtype == LINESEGMENTTYPE || conel->elementtype == BRIDGESEGMENTTYPE)
					{
						if (oconel->elementtype == LINESEGMENTTYPE || oconel->elementtype == BRIDGESEGMENTTYPE)
						{
							/* handle line-to-line intersection */
							ang = compen_figureangle(conel->sx, conel->sy, conel->ex, conel->ey);
							oang = compen_figureangle(oconel->sx, oconel->sy, oconel->ex, oconel->ey);
							if (!compen_intersect(conel->sx, conel->sy, ang, oconel->sx, oconel->sy, oang, &ix, &iy))
							{
								if (ix >= mini(conel->sx, conel->ex) && ix <= maxi(conel->sx, conel->ex) &&
									iy >= mini(conel->sy, conel->ey) && iy <= maxi(conel->sy, conel->ey) &&
									ix >= mini(oconel->sx, oconel->ex) && ix <= maxi(oconel->sx, oconel->ex) &&
									iy >= mini(oconel->sy, oconel->ey) && iy <= maxi(oconel->sy, oconel->ey))
								{
									INTBIG fx, fy;
									compen_rline.col = GREEN;
									if ((el_curwindowpart->state&INPLACEEDIT) != 0) 
										xform(ix, iy, &ix, &iy, el_curwindowpart->outofcell);
									fx = applyxscale(el_curwindowpart, ix-el_curwindowpart->screenlx) + el_curwindowpart->uselx;
									fy = applyyscale(el_curwindowpart, iy-el_curwindowpart->screenly) + el_curwindowpart->usely;
									if (fx > el_curwindowpart->uselx+2 && fx < el_curwindowpart->usehx-2 &&
										fy > el_curwindowpart->usely+2 && fy < el_curwindowpart->usehy-2)
									{
										screendrawline(el_curwindowpart, fx-3, fy-3, fx+3, fy+3, &compen_rline, 0);
										screendrawline(el_curwindowpart, fx-3, fy+3, fx-3, fy+3, &compen_rline, 0);
									}
								}
							}
						} else
						{
							/* handle line-to-arc intersection */
							/* EMPTY */ 
						}
					} else
					{
						if (oconel->elementtype == LINESEGMENTTYPE || oconel->elementtype == BRIDGESEGMENTTYPE)
						{
							/* handle arc-to-line intersection */
							/* EMPTY */ 
						} else
						{
							/* handle arc-to-arc intersection */
							/* EMPTY */ 
						}
					}
				}
			}
		}
	}
}

/*
 * Routine to detect and highlight all duplicate nodes in cell "np".
 */
INTBIG compen_detectduplicates(NODEPROTO *np)
{
	REGISTER NODEINST *ni, *oni;
	REGISTER INTBIG cx, cy, sea, len, olen, i, ctrx, ctry;
	INTBIG fx, fy, tx, ty, ofx, ofy, otx, oty;
	REGISTER VARIABLE *var, *ovar;
	REGISTER GEOM *geom;
	XARRAY trans;
	double startoffset, endangle, ostartoffset, oendangle, dx, dy;
	REGISTER void *infstr;

	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst) ni->temp1 = 0;

	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->temp1 != 0) continue;
		cx = (ni->lowx + ni->highx) / 2;
		cy = (ni->lowy + ni->highy) / 2;
		sea = initsearch(cx, cx, cy, cy, np);
		for(;;)
		{
			geom = nextobject(sea);
			if (geom == NOGEOM) break;
			if (!geom->entryisnode) continue;
			oni = geom->entryaddr.ni;
			if (oni == ni) continue;
			if (oni->temp1 != 0) continue;
			if (oni->proto != ni->proto) continue;

			/* could be a duplicate: make specific tests */
			if (ni->proto == art_openedpolygonprim)
			{
				var = gettrace(ni);
				if (var == NOVARIABLE) len = 0; else
					len = getlength(var);
				ovar = gettrace(oni);
				if (ovar == NOVARIABLE) olen = 0; else
					olen = getlength(ovar);
				if (len == 4 && olen == 4)
				{
					/* special test for colinearity */
					makerot(ni, trans);
					ctrx = (ni->highx + ni->lowx) / 2;
					ctry = (ni->highy + ni->lowy) / 2;
					xform(((INTBIG *)var->addr)[0]+ctrx, ((INTBIG *)var->addr)[1]+ctry, &fx, &fy,
						trans);
					xform(((INTBIG *)var->addr)[2]+ctrx, ((INTBIG *)var->addr)[3]+ctry, &tx, &ty,
						trans);

					makerot(oni, trans);
					ctrx = (oni->highx + oni->lowx) / 2;
					ctry = (oni->highy + oni->lowy) / 2;
					xform(((INTBIG *)ovar->addr)[0]+ctrx, ((INTBIG *)ovar->addr)[1]+ctry, &ofx, &ofy,
						trans);
					xform(((INTBIG *)ovar->addr)[2]+ctrx, ((INTBIG *)ovar->addr)[3]+ctry, &otx, &oty,
						trans);

					/* other line must be inside bounds of current one */
					if (mini(ofx, otx) < mini(fx, tx) || maxi(ofx, otx) > maxi(fx, tx)) continue;
					if (mini(ofy, oty) < mini(fy, ty) || maxi(ofy, oty) > maxi(fy, ty)) continue;

					/* both endpoints of other line must be on current one */
					dx = (double)(tx - fx);   dy = (double)(ty - fy);
					if (((double)(ofx-fx))*dy != ((double)(ofy-fy))*dx) continue;
					if (((double)(otx-fx))*dy != ((double)(oty-fy))*dx) continue;

					/* node "oni" is colinear with "ni" */
					oni->temp1 = 1;
					continue;
				}
			}

			/* make sure size and orientation are the same */
			if (oni->lowx != ni->lowx || oni->highx != ni->highx ||
				oni->lowy != ni->lowy || oni->highy != ni->highy) continue;
			if (oni->rotation != ni->rotation || oni->transpose != ni->transpose) continue;

			if (ni->proto == art_circleprim || ni->proto == art_thickcircleprim)
			{
				getarcdegrees(ni, &startoffset, &endangle);
				getarcdegrees(oni, &ostartoffset, &oendangle);
				if (startoffset != ostartoffset || endangle != oendangle) continue;
			} else if (ni->proto == art_openedpolygonprim || ni->proto == art_closedpolygonprim)
			{
				var = gettrace(ni);
				if (var == NOVARIABLE) len = 0; else
					len = getlength(var);
				ovar = gettrace(oni);
				if (ovar == NOVARIABLE) olen = 0; else
					olen = getlength(ovar);
				if (len != olen) continue;
				for(i=0; i<len; i++) if (((INTBIG *)var->addr)[i] != ((INTBIG *)ovar->addr)[i]) break;
				if (i < len) continue;
			}

			/* node "oni" is a duplicate */
			oni->temp1 = 1;
		}
	}

	/* count the number of duplicates */
	i = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		if (ni->temp1 != 0) i++;
	if (i > 0)
	{
		(void)asktool(us_tool, x_("clear"));
		infstr = initinfstr();
		i = 0;
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if (ni->temp1 == 0) continue;
			if (i != 0) addtoinfstr(infstr, '\n');
			i++;
			formatinfstr(infstr, x_("CELL=%s FROM=0%lo;0%lo;0;NOBBOX"),
				describenodeproto(np), (INTBIG)ni->geom, (INTBIG)ni->proto->firstportproto);
		}
		(void)asktool(us_tool, x_("show-multiple"), (INTBIG)returninfstr(infstr));
	}
	return(i);
}

/*
 * Routine to detect and highlight all nodes that are not in contours in cell "np".
 */
void compen_detectnoncontours(NODEPROTO *np)
{
	REGISTER CONTOUR *con, *contourlist;
	REGISTER CONTOURELEMENT *conel;
	REGISTER NODEINST *ni;
	REGISTER INTBIG i;
	REGISTER void *infstr;

	contourlist = compen_getcontourlist(np);
	if (contourlist == NOCONTOUR) return;

	/* mark all nodes as "not in contour" */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		ni->temp1 = 0;
		if (ni->proto->primindex == 0) ni->temp1 = 1;
		if (ni->proto == gen_cellcenterprim) ni->temp1 = 1;
	}

	/* mark nodes that are in contours */
	for(con = contourlist; con != NOCONTOUR; con = con->nextcontour)
	{
		if (con->valid == 0) continue;
		for(conel = con->firstcontourelement; conel != NOCONTOURELEMENT; conel = conel->nextcontourelement)
		{
			if (conel->ni == NONODEINST) continue;
			conel->ni->temp1 = 1;
		}
	}

	/* count nodes that are not in contours */
	i = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		if (ni->temp1 == 0) i++;
	if (i == 0)
	{
		ttyputmsg(M_("All geometry is in a contour"));
		return;
	}

	(void)asktool(us_tool, x_("clear"));
	infstr = initinfstr();
	i = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->temp1 != 0) continue;
		if (i != 0) addtoinfstr(infstr, '\n');
		i++;
		formatinfstr(infstr, x_("CELL=%s FROM=0%lo;0%lo;0;NOBBOX"),
			describenodeproto(np), (INTBIG)ni->geom, (INTBIG)ni->proto->firstportproto);
	}
	(void)asktool(us_tool, x_("show-multiple"), (INTBIG)returninfstr(infstr));
}

/*
 * Routine to create a line that runs between the two highlighted snap points.
 */
void compen_connectpoints(void)
{
	REGISTER NODEPROTO *np;
	REGISTER VARIABLE *var, *vargds1, *vargds2, *vardxf1, *vardxf2;
	INTBIG len, p1x, p1y, p2x, p2y, cx, cy, v[4];
	HIGHLIGHT thishigh, otherhigh;
	REGISTER NODEINST *ni;

	np = getcurcell();
	if (np == NONODEPROTO)
	{
		ttyputerr(M_("No current cell"));
		return;
	}
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var != NOVARIABLE)
	{
		len = getlength(var);
		if (len == 2)
		{
			if (!us_makehighlight(((CHAR **)var->addr)[0], &thishigh) &&
				!us_makehighlight(((CHAR **)var->addr)[1], &otherhigh))
			{
				/* describe these two objects */
				if ((thishigh.status&HIGHSNAP) != 0 && (otherhigh.status&HIGHSNAP) != 0)
				{
					/* get GDS and DXF information from the two sides */
					if (thishigh.fromgeom == NOGEOM) vargds1 = vardxf1 = NOVARIABLE; else
					{
						if (thishigh.fromgeom->entryisnode)
						{
							vargds1 = getvalkey((INTBIG)thishigh.fromgeom->entryaddr.ni, VNODEINST,
								VINTEGER, compen_gds_layerkey);
							vardxf1 = getvalkey((INTBIG)thishigh.fromgeom->entryaddr.ni, VNODEINST,
								VSTRING, compen_dxf_layerkey);
						} else
						{
							vargds1 = getvalkey((INTBIG)thishigh.fromgeom->entryaddr.ai, VARCINST,
								VINTEGER, compen_gds_layerkey);
							vardxf1 = getvalkey((INTBIG)thishigh.fromgeom->entryaddr.ai, VARCINST,
								VSTRING, compen_dxf_layerkey);
						}
					}
					if (otherhigh.fromgeom == NOGEOM) vargds2 = vardxf2 = NOVARIABLE; else
					{
						if (otherhigh.fromgeom->entryisnode)
						{
							vargds2 = getvalkey((INTBIG)otherhigh.fromgeom->entryaddr.ni, VNODEINST,
								VINTEGER, compen_gds_layerkey);
							vardxf2 = getvalkey((INTBIG)otherhigh.fromgeom->entryaddr.ni, VNODEINST,
								VSTRING, compen_dxf_layerkey);
						} else
						{
							vargds2 = getvalkey((INTBIG)otherhigh.fromgeom->entryaddr.ai, VARCINST,
								VINTEGER, compen_gds_layerkey);
							vardxf2 = getvalkey((INTBIG)otherhigh.fromgeom->entryaddr.ai, VARCINST,
								VSTRING, compen_dxf_layerkey);
						}
					}

					/* make the line */
					us_getsnappoint(&thishigh, &p1x, &p1y);
					us_getsnappoint(&otherhigh, &p2x, &p2y);
					ni = newnodeinst(art_openedpolygonprim, mini(p1x,p2x), maxi(p1x,p2x),
						mini(p1y,p2y), maxi(p1y,p2y), 0, 0, np);
					if (ni == NONODEINST)
					{
						ttyputerr(M_("Could not create connecting line"));
						return;
					}
					cx = (p1x + p2x) / 2;
					cy = (p1y + p2y) / 2;
					v[0] = p1x - cx;   v[1] = p1y - cy;
					v[2] = p2x - cx;   v[3] = p2y - cy;
					(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)v,
						VINTEGER|VISARRAY|(4<<VLENGTHSH));

					/* set GDS if appropriate */
					if (vargds1 != NOVARIABLE || vargds2 != NOVARIABLE)
					{
						if (vargds1 != NOVARIABLE && vargds2 != NOVARIABLE)
						{
							if (vargds1->addr != vargds2->addr)
								ttyputmsg(M_("Warning, one side has GDS layer %ld, the other has %ld"),
									vargds1->addr, vargds2->addr);
						}
						if (vargds1 == NOVARIABLE) vargds1 = vargds2;
						(void)setvalkey((INTBIG)ni, VNODEINST, compen_gds_layerkey,
							vargds1->addr, VINTEGER);
					}

					/* set DXF if appropriate */
					if (vardxf1 != NOVARIABLE || vardxf2 != NOVARIABLE)
					{
						if (vardxf1 != NOVARIABLE && vardxf2 != NOVARIABLE)
						{
							if (estrcmp((CHAR *)vardxf1->addr, (CHAR *)vardxf2->addr) != 0)
								ttyputmsg(M_("Warning, one side has DXF layer '%s', the other has '%s'"),
									(CHAR *)vardxf1->addr, (CHAR *)vardxf2->addr);
						}
						if (vardxf1 == NOVARIABLE) vardxf1 = vardxf2;
						(void)setvalkey((INTBIG)ni, VNODEINST, compen_dxf_layerkey,
							vardxf1->addr, VSTRING);
					}

					endobjectchange((INTBIG)ni, VNODEINST);
					return;
				}
			}
		}
	}
	ttyputerr(M_("Must select two snap points to connect them with a line"));
}

/*
 * Routine to draw all contours in cell "np".
 */
void compen_drawcontours(NODEPROTO *np)
{
	static POLYGON *objc = NOPOLYGON;
	REGISTER CONTOURELEMENT *conel;
	REGISTER CONTOUR *con, *contourlist;
	static INTBIG color = 0;

	if (el_curwindowpart == NOWINDOWPART) return;
	(void)needstaticpolygon(&objc, 4, compen_tool->cluster);

	contourlist = compen_getcontourlist(np);
	if (contourlist == NOCONTOUR) return;
	for(con = contourlist; con != NOCONTOUR; con = con->nextcontour)
	{
		if (con->valid == 0) continue;
		compen_rline.col = compen_colors[color++];
		if (color >= COLORS) color = 0;
		for(conel = con->firstcontourelement; conel != NOCONTOURELEMENT; conel = conel->nextcontourelement)
		{
			switch (conel->elementtype)
			{
				case CIRCLESEGMENTTYPE:
					compen_drawcircle(conel->cx, conel->cy, conel->sx, conel->sy, &compen_rline);
					break;
				case ARCSEGMENTTYPE:
				case REVARCSEGMENTTYPE:
					if (conel->elementtype == REVARCSEGMENTTYPE)
					{
						compen_drawcirclearc(conel->cx, conel->cy, conel->sx, conel->sy, conel->ex, conel->ey, &compen_rline);
					} else
					{
						compen_drawcirclearc(conel->cx, conel->cy, conel->ex, conel->ey, conel->sx, conel->sy, &compen_rline);
					}
					break;
				case LINESEGMENTTYPE:
				case BRIDGESEGMENTTYPE:
					compen_drawline(conel->sx, conel->sy, conel->ex, conel->ey, &compen_rline);
					break;
			}
		}
	}
}

/*********** Compensation factors dialog ***********/

static DIALOGITEM compen_factorsdialogitems[] =
{
 /*  1 */ {0, {108,176,132,240}, BUTTON, N_("OK")},
 /*  2 */ {0, {108,16,132,80}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,168,24,258}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,8,24,133}, MESSAGE, N_("Metal Thickness:")},
 /*  5 */ {0, {32,168,48,258}, EDITTEXT, x_("")},
 /*  6 */ {0, {32,8,48,145}, MESSAGE, N_("LRS Compensation:")},
 /*  7 */ {0, {56,168,72,258}, EDITTEXT, x_("")},
 /*  8 */ {0, {56,8,72,160}, MESSAGE, N_("Global Compensation:")},
 /*  9 */ {0, {80,168,96,258}, EDITTEXT, x_("")},
 /* 10 */ {0, {80,8,96,165}, MESSAGE, N_("Minimum feature size:")}
};
static DIALOG compen_factorsdialog = {{50,75,197,343}, N_("Compensation Factors"), 0, 8, compen_factorsdialogitems, 0, 0};

/* special items for the "Compensation Factors" dialog: */
#define DCMF_METALTHICK    3		/* Metal Thickness (edit text) */
#define DCMF_LRSCOMP       5		/* LRS Compensation (edit text) */
#define DCMF_GLOBALCOMP    7		/* Global Compensation (edit text) */
#define DCMF_MINFEATURE    9		/* Minimum feature size (edit text) */

void compen_dofactorsdialog(void)
{
	CHAR line[256];
	INTBIG itemHit;
	float metalthickness, lrscompensation, globalcompensation, minimumsize;
	REGISTER void *dia;

	/* show the "about" dialog */
	dia = DiaInitDialog(&compen_factorsdialog);
	if (dia == 0) return;
	compen_getglobalfactors(&metalthickness, &lrscompensation, &globalcompensation,
		&minimumsize, el_curlib);
	esnprintf(line, 256, x_("%g"), metalthickness);
	DiaSetText(dia, DCMF_METALTHICK, line);
	esnprintf(line, 256, x_("%g"), lrscompensation);
	DiaSetText(dia, DCMF_LRSCOMP, line);
	esnprintf(line, 256, x_("%g"), globalcompensation);
	DiaSetText(dia, DCMF_GLOBALCOMP, line);
	esnprintf(line, 256, x_("%g"), minimumsize);
	DiaSetText(dia, DCMF_MINFEATURE, line);
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
	}
	if (itemHit == OK)
	{
		metalthickness = (float)eatof(DiaGetText(dia, DCMF_METALTHICK));
		lrscompensation = (float)eatof(DiaGetText(dia, DCMF_LRSCOMP));
		globalcompensation = (float)eatof(DiaGetText(dia, DCMF_GLOBALCOMP));
		minimumsize = (float)eatof(DiaGetText(dia, DCMF_MINFEATURE));
		compen_setglobalfactors(metalthickness, lrscompensation, globalcompensation,
			minimumsize, el_curlib);
	}
	DiaDoneDialog(dia);
}

/*
 * routine to manipulate compensation percentages on node "ni".
 * If "mode" is zero, set percentage to "amt".
 * If "mode" is positive, report percentage amount.
 * If "mode" is negative, remove percentage amount.
 */
void compen_setnodecompensation(NODEINST *ni, float amt, INTBIG mode)
{
	REGISTER VARIABLE *var;
	CHAR name[100];
	double startoffset, endangle;

	if (mode > 0)
	{
		/* get compensation percentage */
		estrcpy(name, M_("Unknown"));
		if (ni->proto == art_openedpolygonprim || ni->proto == art_closedpolygonprim)
		{
			estrcpy(name, M_("Line"));
		} else if (ni->proto == art_circleprim || ni->proto == art_thickcircleprim)
		{
			getarcdegrees(ni, &startoffset, &endangle);
			if (startoffset != 0.0 || endangle != 0.0) estrcpy(name, M_("Arc")); else
				estrcpy(name, M_("Circle"));
		}
		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
		if (var != NOVARIABLE)
		{
			estrcat(name, x_(" ["));
			estrcat(name, (CHAR *)var->addr);
			estrcat(name, x_("]"));
		}

		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, compen_dxf_layerkey);
		if (var != NOVARIABLE)
		{
			estrcat(name, M_(" on layer "));
			estrcat(name, (CHAR *)var->addr);
		}

		var = getvalkey((INTBIG)ni, VNODEINST, VFLOAT, compen_percentagekey);
		if (var == NOVARIABLE)
			ttyputmsg(M_("%s has no compensation set on it"), name); else
				ttyputmsg(M_("%s has %g%% compensation set on it"), name, castfloat(var->addr));
	} else if (mode == 0)
	{
		/* set compensation percentage */
		(void)setvalkey((INTBIG)ni, VNODEINST, compen_percentagekey, castint(amt), VFLOAT);
	} else
	{
		/* remove compensation percentage */
		if (getvalkey((INTBIG)ni, VNODEINST, VFLOAT, compen_percentagekey) != NOVARIABLE)
			(void)delvalkey((INTBIG)ni, VNODEINST, compen_percentagekey);
	}
}

/*
 * routine to set color information on all nodes in the current cell with compensation
 * percentage factors
 */
void compen_illuminatepercentages(BOOLEAN showonside)
{
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var, *varcolor;
	REGISTER INTBIG cindex, usedcolors, bestindex, position;
	REGISTER INTBIG cx, cy;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	float amt, bestvalue;
	static POLYGON *objc = NOPOLYGON;
	CHAR perstring[100];
	INTBIG shown[COLORS];

	np = getcurcell();
	if (np == NONODEPROTO)
	{
		ttyputerr(M_("No current cell"));
		return;
	}

	(void)needstaticpolygon(&objc, 4, compen_tool->cluster);

	/* push highlighting */
	us_pushhighlight();
	us_clearhighlightcount();
	usedcolors = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* see if there is a percentage on this node */
		var = getvalkey((INTBIG)ni, VNODEINST, VFLOAT, compen_percentagekey);
		varcolor = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, art_colorkey);
		if (var == NOVARIABLE)
		{
			if (varcolor != NOVARIABLE)
			{
				startobjectchange((INTBIG)ni, VNODEINST);
				(void)delvalkey((INTBIG)ni, VNODEINST, art_colorkey);
				endobjectchange((INTBIG)ni, VNODEINST);
			}
			continue;
		}

		/* see if the percentage is in the table.  Add if not */
		amt = castfloat(var->addr);
		for(cindex=0; cindex<usedcolors; cindex++)
			if (compen_percentages[cindex] == amt) break;
		if (cindex >= usedcolors && usedcolors < COLORS)
		{
			cindex = usedcolors++;
			compen_percentages[cindex] = amt;
		}

		/* see if there is a color on this node */
		if (varcolor == NOVARIABLE)
		{
			/* none: add it */
			startobjectchange((INTBIG)ni, VNODEINST);
			setvalkey((INTBIG)ni, VNODEINST, art_colorkey, compen_colors[cindex], VINTEGER);
			endobjectchange((INTBIG)ni, VNODEINST);
		} else
		{
			/* see if the color is correct */
			if (cindex < usedcolors && compen_colors[cindex] != varcolor->addr)
			{
				startobjectchange((INTBIG)ni, VNODEINST);
				setvalkey((INTBIG)ni, VNODEINST, art_colorkey, compen_colors[cindex], VINTEGER);
				endobjectchange((INTBIG)ni, VNODEINST);
			}
		}
	}

	/* illustrate the percentage colors */
	if (showonside)
	{
		TDCLEAR(descript);
		TDSETSIZE(descript, TXTSETPOINTS(12));
		screensettextinfo(el_curwindowpart, NOTECHNOLOGY, descript);
		for(cindex=0; cindex<usedcolors; cindex++) shown[cindex] = 0;
		for(position=0; ; position++)
		{
			bestindex = -1;
			bestvalue = 100000.0;
			for(cindex = 0; cindex < usedcolors; cindex++)
			{
				if (shown[cindex] != 0) continue;
				if (compen_percentages[cindex] > bestvalue) continue;
				bestvalue = compen_percentages[cindex];
				bestindex = cindex;
			}
			if (bestindex < 0) break;

			shown[bestindex] = 1;
			compen_rline.col = compen_colors[bestindex];
			cx = el_curwindowpart->uselx;
			cy = el_curwindowpart->usely + (el_curwindowpart->usehy-el_curwindowpart->usely) / usedcolors * position;
			esnprintf(perstring, 100, x_("%g%%"), compen_percentages[bestindex]);
			screendrawtext(el_curwindowpart, cx, cy, perstring, &compen_rline);
		}
	}

	/* restore highlighting */
	us_pophighlight(FALSE);
}

/*
 * Routine to get global factors "metalthickness", "lrscompensation", "globalcompensation", and
 * "minimumsize" from library "lib".
 */
void compen_getglobalfactors(float *metalthickness, float *lrscompensation, float *globalcompensation,
	float *minimumsize, LIBRARY *lib)
{
	REGISTER VARIABLE *var;

	var = getvalkey((INTBIG)lib, VLIBRARY, VFLOAT, compen_metalthicknesskey);
	if (var == NOVARIABLE) *metalthickness = (float)DEFAULTMETALTHICKNESS; else
		*metalthickness = castfloat(var->addr);
	var = getvalkey((INTBIG)lib, VLIBRARY, VFLOAT, compen_lrscompensationkey);
	if (var == NOVARIABLE) *lrscompensation = (float)DEFAULTLRSCOMPENSATION; else
		*lrscompensation = castfloat(var->addr);
	var = getvalkey((INTBIG)lib, VLIBRARY, VFLOAT, compen_globalcompensationkey);
	if (var == NOVARIABLE) *globalcompensation = (float)DEFAULTGLOBALCOMPENSATION; else
		*globalcompensation = castfloat(var->addr);
	var = getvalkey((INTBIG)lib, VLIBRARY, VFLOAT, compen_minimumsizekey);
	if (var == NOVARIABLE) *minimumsize = (float)DEFAULTMINIMUMSIZE; else
		*minimumsize = castfloat(var->addr);
}

/*
 * Routine to set global factors "metalthickness", "lrscompensation", "globalcompensation", and
 * "minimumsize" onto library "lib".
 */
void compen_setglobalfactors(float metalthickness, float lrscompensation, float globalcompensation,
	float minimumsize, LIBRARY *lib)
{
	setvalkey((INTBIG)lib, VLIBRARY, compen_metalthicknesskey,
		(INTBIG)castint(metalthickness), VFLOAT);
	setvalkey((INTBIG)lib, VLIBRARY, compen_lrscompensationkey,
		(INTBIG)castint(lrscompensation), VFLOAT);
	setvalkey((INTBIG)lib, VLIBRARY, compen_globalcompensationkey,
		(INTBIG)castint(globalcompensation), VFLOAT);
	setvalkey((INTBIG)lib, VLIBRARY, compen_minimumsizekey,
		(INTBIG)castint(minimumsize), VFLOAT);
}

/******************************** ACTUAL COMPENSATION ********************************/

/*
 * routine to examine all of the node instances in the cell and assign any compensation
 * percentages to the appropriate contour elements in the data structure.  Also warns the user
 * of any nodes with compensation percentages that are not in a contour data structure.
 */
void compen_assignpercentages(CONTOUR *contourlist, NODEPROTO *np)
{
	REGISTER NODEINST *ni;
	REGISTER INTBIG total;
	BOOLEAN first;
	REGISTER VARIABLE *var;
	REGISTER CONTOUR *con;
	REGISTER CONTOURELEMENT *conel;
	REGISTER USERDATA *ud;
	float metalthickness, lrscompensation, globalcompensation, minimumsize;
	REGISTER void *infstr;

	/* get global compensation */
	compen_getglobalfactors(&metalthickness, &lrscompensation, &globalcompensation, &minimumsize,
		np->lib);

	/* mark all nodes that have compensation percentages */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		ni->temp1 = 0;
		var = getvalkey((INTBIG)ni, VNODEINST, VFLOAT, compen_percentagekey);
		if (var != NOVARIABLE) ni->temp1 = 1;
	}

	/* unmark those nodes that appear in contours */
	for(con = contourlist; con != NOCONTOUR; con = con->nextcontour)
	{
		if (con->valid == 0) continue;
		for(conel = con->firstcontourelement; conel != NOCONTOURELEMENT; conel = conel->nextcontourelement)
		{
			ud = (USERDATA *)conel->userdata;
			ud->percentage = globalcompensation;
			ud->explicitpercentage = 0;
			if (conel->ni != NONODEINST)
			{
				var = getvalkey((INTBIG)conel->ni, VNODEINST, VFLOAT, compen_percentagekey);
				if (var != NOVARIABLE)
				{
					ud->percentage = castfloat(var->addr);
					ud->explicitpercentage = 1;
					conel->ni->temp1 = 0;
				}
			}
		}
	}

	/* warn about nodes that are not in contours */
	total = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		if (ni->temp1 != 0) total++;
	if (total != 0)
	{
		ttyputerr(M_("%ld node(s) with compensation information are not in contours"), total);
		(void)asktool(us_tool, x_("clear"));
		infstr = initinfstr();
		first = FALSE;
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if (ni->temp1 == 0) continue;
			if (first) addtoinfstr(infstr, '\n');
			first = TRUE;
			formatinfstr(infstr, x_("CELL=%s FROM=0%lo;0%lo;0;NOBBOX"),
				describenodeproto(np), (INTBIG)ni->geom, (INTBIG)ni->proto->firstportproto);
		}
		(void)asktool(us_tool, x_("show-multiple"), (INTBIG)returninfstr(infstr));
	}
}

/*
 * routine to do the actual compensation to the geometry in the contours.
 */
void compen_movegeometry(CONTOUR *contourlist, NODEPROTO *np)
{
	REGISTER CONTOUR *con;
	REGISTER CONTOURELEMENT *conel;
	REGISTER INTBIG orthosine, orthocosine, distance, radius, closex, closey, radius1, radius2,
		testx1, testy1, testx2, testy2;
	INTBIG x1, y1, x2, y2;
	REGISTER INTBIG ang, orthoang;
	REGISTER USERDATA *ud;
	double dang, dorthoang, dorthosin, dorthocos, dstartsine, dstartcosine, dendsine, dendcosine;
	float metalthickness, lrscompensation, globalcompensation, minimumsize, truecomp;

	/* get global factors */
	compen_getglobalfactors(&metalthickness, &lrscompensation, &globalcompensation, &minimumsize,
		np->lib);

	/* loop through every element in every contour */
	for(con = contourlist; con != NOCONTOUR; con = con->nextcontour)
	{
		if (con->valid == 0) continue;
		for(conel = con->firstcontourelement; conel != NOCONTOURELEMENT; conel = conel->nextcontourelement)
		{
			/* remember original positions */
			ud = (USERDATA *)conel->userdata;
			ud->origsx = conel->sx;
			ud->origsy = conel->sy;
			ud->origex = conel->ex;
			ud->origey = conel->ey;

			if (ud->percentage < 0.0) continue;

			/* get the actual distance to compensate from the percentage */
			truecomp = compen_truecompensation(ud->percentage, metalthickness, lrscompensation);
			distance = scalefromdispunit(truecomp, DISPUNITMM);
			switch (conel->elementtype)
			{
				case LINESEGMENTTYPE:
				case BRIDGESEGMENTTYPE:
					/* compute the angle that is perpendicular to the line in the direction of compensation */
					dang = compen_figureangle(conel->sx, conel->sy, conel->ex, conel->ey);
					if ((con->depth&1) != 0)
					{
						dorthoang = dang - EPI/2.0;
						if (dorthoang < 0.0) dorthoang += EPI*2.0;
					} else
					{
						dorthoang = dang + EPI/2.0;
						if (dorthoang > EPI*2.0) dorthoang -= EPI*2.0;
					}

					/* move the endpoints by the appropriate distance in the compensation direction */
					dorthosin = sin(dorthoang);   dorthocos = cos(dorthoang);
					conel->sx = conel->sx + rounddouble(dorthocos * (double)distance);
					conel->sy = conel->sy + rounddouble(dorthosin * (double)distance);
					conel->ex = conel->ex + rounddouble(dorthocos * (double)distance);
					conel->ey = conel->ey + rounddouble(dorthosin * (double)distance);
					break;

				case ARCSEGMENTTYPE:
				case REVARCSEGMENTTYPE:
					/* get the first line segment that approximates this arc */
					initcontoursegmentgeneration(conel);
					(void)nextcontoursegmentgeneration(&x1, &y1, &x2, &y2);

					/* compute the angle that is perpendicular to the approximating line */
					ang = figureangle(x1, y1, x2, y2);		/* OK to do this in tenth-degree approximations */
					if ((con->depth&1) != 0) orthoang = (ang+2700) % 3600; else
						 orthoang = (ang+900) % 3600;

					/*
					 * move the starting point by the appropriate distance in the compensation direction.
					 * this is only an approximate to the compensated starting point since it is derived
					 * from an approximating line segment
					 */
					orthosine = sine(orthoang);   orthocosine = cosine(orthoang);
					closex = x1 + mult(orthocosine, distance);
					closey = y1 + mult(orthosine, distance);

					/* now compute the arc radius and the two possible changes to it */
					radius = computedistance(conel->cx, conel->cy, conel->sx, conel->sy);
					radius1 = radius + distance;
					radius2 = radius - distance;

					/* compute the precise starting point location for the two possible radius changes */
					dang = compen_figureangle(conel->cx, conel->cy, conel->sx, conel->sy);
					dstartsine = sin(dang);   dstartcosine = cos(dang);
					dang = compen_figureangle(conel->cx, conel->cy, conel->ex, conel->ey);
					dendsine = sin(dang);   dendcosine = cos(dang);
					testx1 = conel->cx + rounddouble(dstartcosine * (double)radius1);
					testy1 = conel->cy + rounddouble(dstartsine * (double)radius1);
					testx2 = conel->cx + rounddouble(dstartcosine * (double)radius2);
					testy2 = conel->cy + rounddouble(dstartsine * (double)radius2);

					/* see which starting point is closest to the approximate one */
					if (computedistance(closex,closey, testx1,testy1) <
						computedistance(closex,closey, testx2,testy2))
					{
						/* first radius is correct */
						conel->sx = testx1;
						conel->sy = testy1;
						conel->ex = conel->cx + rounddouble(dendcosine * (double)radius1);
						conel->ey = conel->cy + rounddouble(dendsine * (double)radius1);
					} else
					{
						/* second radius is correct */
						conel->sx = testx2;
						conel->sy = testy2;
						conel->ex = conel->cx + rounddouble(dendcosine * (double)radius2);
						conel->ey = conel->cy + rounddouble(dendsine * (double)radius2);
					}
					break;

				case CIRCLESEGMENTTYPE:
					/* compute the radius of the circle */
					radius = computedistance(conel->cx, conel->cy, conel->sx, conel->sy);

					/* adjust the radius by the proper amount */
					if ((con->depth&1) != 0) radius -= distance; else
						radius += distance;

					/* set the new compensated starting point */
					conel->sx = conel->cx + radius;
					conel->sy = conel->cy;
					break;
			}
		}
	}
}

/*
 * Routine to adjust the actual nodes to match the compensated information in cell "np"
 */
void compen_adjustgeometry(CONTOUR *contourlist, NODEPROTO *np)
{
	REGISTER CONTOUR *con;
	REGISTER CONTOURELEMENT *conel;
	INTBIG lx, hx, ly, hy;
	REGISTER INTBIG radius, xc, yc;
	INTBIG coord[10];
	REGISTER INTBIG newrot;
	double fx, fy, srot, erot, fswap;
	REGISTER NODEINST *ni, *nextni;
	REGISTER USERDATA *ud;

	/* mark and delete all nodes that created lines */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		ni->temp1 = 0;
	for(con = contourlist; con != NOCONTOUR; con = con->nextcontour)
	{
		if (con->valid == 0) continue;
		for(conel = con->firstcontourelement; conel != NOCONTOURELEMENT; conel = conel->nextcontourelement)
			if (conel->ni != NONODEINST && conel->elementtype == LINESEGMENTTYPE)
		{
			conel->ni->temp1 = 1;
			conel->ni = NONODEINST;
		}
	}
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = nextni)
	{
		nextni = ni->nextnodeinst;
		if (ni->temp1 == 0) continue;
		startobjectchange((INTBIG)ni, VNODEINST);
		(void)killnodeinst(ni);
	}

	/* loop through every element in every contour */
	for(con = contourlist; con != NOCONTOUR; con = con->nextcontour)
	{
		if (con->valid == 0) continue;
		for(conel = con->firstcontourelement; conel != NOCONTOURELEMENT; conel = conel->nextcontourelement)
		{
			if (conel->ni == NONODEINST)
			{
				if (conel->elementtype != LINESEGMENTTYPE && conel->elementtype != BRIDGESEGMENTTYPE)
				{
					ttyputmsg(M_("Error adjusting nonline segment with no original geometry"));
					continue;
				}

				/* create the new line node */
				lx = mini(conel->sx, conel->ex);   hx = maxi(conel->sx, conel->ex);
				ly = mini(conel->sy, conel->ey);   hy = maxi(conel->sy, conel->ey);
				xc  = (lx + hx) / 2;	yc = (ly + hy) / 2;
				conel->ni = newnodeinst(art_openedpolygonprim, lx, hx, ly, hy, 0, 0, np);
				if (conel->ni == NONODEINST) return;

				/* preserve original information */
				ud = (USERDATA *)conel->userdata;
				coord[PRECOMPMAGIC] = 0xFEED;
				coord[PRECOMPTYPE] = conel->elementtype;
				coord[PRECOMPPERC] = (INTBIG)ud->percentage;
				coord[PRECOMPEXPPERC] = ud->explicitpercentage;
				coord[PRECOMPLINESX] = ud->origsx;
				coord[PRECOMPLINESY] = ud->origsy;
				coord[PRECOMPLINEEX] = ud->origex;
				coord[PRECOMPLINEEY] = ud->origey;
				(void)setvalkey((INTBIG)conel->ni, VNODEINST, compen_precomppositionkey, (INTBIG)coord,
					VINTEGER|VISARRAY|(PRECOMPLINESIZE<<VLENGTHSH));

				/* store line coordinates */
				coord[0] = conel->sx - xc;		coord[1] = conel->sy - yc;
				coord[2] = conel->ex - xc;		coord[3] = conel->ey - yc;
				(void)setvalkey((INTBIG)conel->ni, VNODEINST, el_trace_key, (INTBIG)coord,
					VINTEGER|VISARRAY|(4<<VLENGTHSH));

				if (ud->DXFlayer != 0)
					(void)setvalkey((INTBIG)conel->ni, VNODEINST, compen_dxf_layerkey, (INTBIG)ud->DXFlayer, VSTRING);

				/* assign new GDS layer if this is hole contour */
				if ((con->depth&1) != 0)
					setvalkey((INTBIG)conel->ni, VNODEINST, compen_gds_layerkey, 2, VINTEGER);

				/* end of changes */
				endobjectchange((INTBIG)conel->ni, VNODEINST);
				continue;
			}

			switch (conel->elementtype)
			{
				case CIRCLESEGMENTTYPE:
					/* start changes */
					startobjectchange((INTBIG)conel->ni, VNODEINST);

					/* preserve original information */
					ud = (USERDATA *)conel->userdata;
					coord[PRECOMPMAGIC] = 0xFEED;
					coord[PRECOMPTYPE] = CIRCLESEGMENTTYPE;
					coord[PRECOMPPERC] = (INTBIG)ud->percentage;
					coord[PRECOMPEXPPERC] = ud->explicitpercentage;
					coord[PRECOMPCIRCSX] = ud->origsx;
					coord[PRECOMPCIRCSY] = ud->origsy;
					coord[PRECOMPCIRCRAD] = computedistance(conel->cx, conel->cy, ud->origsx, ud->origsy);
					(void)setvalkey((INTBIG)conel->ni, VNODEINST, compen_precomppositionkey, (INTBIG)coord,
						VINTEGER|VISARRAY|(PRECOMPCIRCSIZE<<VLENGTHSH));

					/* set new size according to new radius */
					radius = computedistance(conel->cx, conel->cy, conel->sx, conel->sy);
					lx = conel->cx - radius;   hx = conel->cx + radius;
					ly = conel->cy - radius;   hy = conel->cy + radius;
					modifynodeinst(conel->ni, lx - conel->ni->lowx, ly - conel->ni->lowy,
						hx - conel->ni->highx, hy - conel->ni->highy, 0, 0);

					/* remove compensation percentage color coding if it is there */
					if (getvalkey((INTBIG)conel->ni, VNODEINST, VINTEGER, art_colorkey) != NOVARIABLE)
						(void)delvalkey((INTBIG)conel->ni, VNODEINST, art_colorkey);

					/* assign new GDS layer if this is hole contour */
					if ((con->depth&1) != 0)
						setvalkey((INTBIG)conel->ni, VNODEINST, compen_gds_layerkey, 2, VINTEGER);

					/* end of changes */
					endobjectchange((INTBIG)conel->ni, VNODEINST);
					break;

				case ARCSEGMENTTYPE:
				case REVARCSEGMENTTYPE:
					/* start changes */
					startobjectchange((INTBIG)conel->ni, VNODEINST);

					/* preserve original information */
					ud = (USERDATA *)conel->userdata;
					coord[PRECOMPMAGIC] = 0xFEED;
					coord[PRECOMPTYPE] = conel->elementtype;
					coord[PRECOMPPERC] = (INTBIG)ud->percentage;
					coord[PRECOMPEXPPERC] = ud->explicitpercentage;
					coord[PRECOMPARCSX] = ud->origsx;
					coord[PRECOMPARCSY] = ud->origsy;
					coord[PRECOMPARCEX] = ud->origex;
					coord[PRECOMPARCEY] = ud->origey;
					coord[PRECOMPARCRAD] = computedistance(conel->cx, conel->cy, ud->origsx, ud->origsy);
					(void)setvalkey((INTBIG)conel->ni, VNODEINST, compen_precomppositionkey, (INTBIG)coord,
						VINTEGER|VISARRAY|(PRECOMPARCSIZE<<VLENGTHSH));

					/* set new size and arc information */
					radius = computedistance(conel->cx, conel->cy, conel->sx, conel->sy);
					lx = conel->cx - radius;   hx = conel->cx + radius;
					ly = conel->cy - radius;   hy = conel->cy + radius;
					fx = conel->sx - conel->cx;   fy = conel->sy - conel->cy;
					if (fy == 0.0 && fx == 0.0)
					{
						ttyputerr(M_("Domain error compensating arc end 1"));
						break;
					}
					srot = atan2(fy, fx);
					if (srot < 0.0) srot += EPI*2.0;
					fx = conel->ex - conel->cx;   fy = conel->ey - conel->cy;
					if (fy == 0.0 && fx == 0.0)
					{
						ttyputerr(M_("Domain error compensating arc end 2"));
						break;
					}
					erot = atan2(fy, fx);
					if (erot < 0.0) erot += EPI*2.0;
					if (conel->elementtype == REVARCSEGMENTTYPE)
					{
						fswap = srot;   srot = erot;   erot = fswap;
					}
					erot -= srot;
					if (erot < 0.0) erot += EPI*2.0;
					newrot = rounddouble(srot * 1800.0 / EPI);
					srot -= ((double)newrot) * EPI / 1800.0;
					modifynodeinst(conel->ni, lx - conel->ni->lowx, ly - conel->ni->lowy,
						hx - conel->ni->highx, hy - conel->ni->highy,
							newrot - conel->ni->rotation, -conel->ni->transpose);
					setarcdegrees(conel->ni, srot, erot);

					/* remove compensation percentage color coding if it is there */
					if (getvalkey((INTBIG)conel->ni, VNODEINST, VINTEGER, art_colorkey) != NOVARIABLE)
						(void)delvalkey((INTBIG)conel->ni, VNODEINST, art_colorkey);

					/* assign new GDS layer if this is hole contour */
					if ((con->depth&1) != 0)
						setvalkey((INTBIG)conel->ni, VNODEINST, compen_gds_layerkey, 2, VINTEGER);

					/* end of changes */
					endobjectchange((INTBIG)conel->ni, VNODEINST);
					break;

				default:
					break;
			}
		}
	}
}

/******************************** CONTOUR CLEANING/BLENDING ********************************/

/*
 * routine to clean-up the contours by adjusting the geometry.  If "blend" is true, blend endpoints
 * precisely.  Otherwise, simply insert bridge-line segments where the ends don't meet.
 */
void compen_cleancontours(CONTOUR *contourlist, NODEPROTO *np, BOOLEAN blend)
{
	REGISTER CONTOUR *con;
	REGISTER CONTOURELEMENT *conel, *lastconel, *nextconel;

	if (!blend) compen_debugdump(M_("CLEANING QUICKLY")); else
		compen_debugdump(M_("\nBLENDING PROPERLY"));

	for(con = contourlist; con != NOCONTOUR; con = con->nextcontour)
	{
		if (con->valid == 0) continue;

		/* remove bridge segments */
		lastconel = NOCONTOURELEMENT;
		for(conel = con->firstcontourelement; conel != NOCONTOURELEMENT; conel = nextconel)
		{
			nextconel = conel->nextcontourelement;
			if (conel->elementtype == BRIDGESEGMENTTYPE)
			{
				if (lastconel == NOCONTOURELEMENT) con->firstcontourelement = nextconel; else
					lastconel->nextcontourelement = nextconel;
				efree((CHAR *)conel);
			} else lastconel = conel;
		}
		con->lastcontourelement = lastconel;

		/* force all segments to meet */
		for(conel = con->firstcontourelement; conel != NOCONTOURELEMENT; conel = nextconel)
		{
			nextconel = conel->nextcontourelement;
			if (conel != con->firstcontourelement)
				compen_forcemeeting(con, lastconel, conel, blend);
			lastconel = conel;
		}
		compen_forcemeeting(con, lastconel, con->firstcontourelement, blend);
	}
}

/*
 * routine to clean-up two contour elements "firstconel" and "secondconel" on contour "con" by adjusting
 * their geometry.  If "blend" is true, blend endpoints precisely.  Otherwise, simply insert
 * bridge-line segments where the ends don't meet.
 */
void compen_forcemeeting(CONTOUR *con, CONTOURELEMENT *firstconel, CONTOURELEMENT *secondconel, BOOLEAN blend)
{
	REGISTER INTBIG a1, a2, impliedintersection, icount;
	double da1, da2, dang;
	INTBIG ix, iy, ix1, iy1, ix2, iy2;
	REGISTER INTBIG arcrad, newix, newiy;
	REGISTER USERDATA *ud1, *ud2;
	CHAR *firstname, *secondname;

	firstname = secondname = x_("");
#ifdef DEBDUMP
	if (firstconel->ni != NONODEINST)
	{
		REGISTER VARIABLE *var;
		var = getvalkey((INTBIG)firstconel->ni, VNODEINST, VSTRING, el_node_name_key);
		if (var != NOVARIABLE) firstname = (CHAR *)var->addr;
	}
	if (secondconel->ni != NONODEINST)
	{
		REGISTER VARIABLE *var;
		var = getvalkey((INTBIG)secondconel->ni, VNODEINST, VSTRING, el_node_name_key);
		if (var != NOVARIABLE) secondname = (CHAR *)var->addr;
	}
#endif

	/* cannot make circles meet anything else */
	if (firstconel->elementtype == CIRCLESEGMENTTYPE ||
		secondconel->elementtype == CIRCLESEGMENTTYPE) return;

	/* if they already meet, exit */
	if (firstconel->ex == secondconel->sx && firstconel->ey == secondconel->sy) return;

	/* if not blending, simply insert a bridge line segment */
	if (!blend)
	{
		compen_insertbridge(firstconel, secondconel);
		return;
	}

	/*
	 * see if there is an implied intersection point.
	 * If segments meet with more than 5 degrees difference, blending occurs at implied point
	 * OK to do these in tenth-degree approximations
	 */
	impliedintersection = 0;
	if (firstconel->elementtype == LINESEGMENTTYPE)
	{
		if (firstconel->sx == firstconel->ex && firstconel->sy == firstconel->ey)
		{
			a1 = 0;
			impliedintersection = 1;
		} else
			a1 = figureangle(firstconel->sx, firstconel->sy, firstconel->ex, firstconel->ey);
	} else if (firstconel->elementtype == ARCSEGMENTTYPE ||
		firstconel->elementtype == REVARCSEGMENTTYPE)
	{
		a1 = (figureangle(firstconel->cx, firstconel->cy, firstconel->ex, firstconel->ey) + 900) % 3600;
	}
	if (secondconel->elementtype == LINESEGMENTTYPE)
	{
		if (secondconel->sx == secondconel->ex && secondconel->sy == secondconel->ey)
		{
			a2 = 0;
			impliedintersection = 1;
		} else
			a2 = figureangle(secondconel->ex, secondconel->ey, secondconel->sx, secondconel->sy);
	} else if (secondconel->elementtype == ARCSEGMENTTYPE ||
		secondconel->elementtype == REVARCSEGMENTTYPE)
	{
		a2 = (figureangle(secondconel->cx, secondconel->cy, secondconel->sx, secondconel->sy) + 900) % 3600;
	}
	a1 %= 1800;   a2 %= 1800;
	if (abs(a1-a2) > 50) impliedintersection = 1;

	/* handle line-to-line blending (rule 1.0) */
	if (firstconel->elementtype == LINESEGMENTTYPE && secondconel->elementtype == LINESEGMENTTYPE)
	{
		/* see if the lines are parallel */
		da1 = compen_figureangle(firstconel->ex, firstconel->ey, firstconel->sx, firstconel->sy);
		da2 = compen_figureangle(secondconel->sx, secondconel->sy, secondconel->ex, secondconel->ey);
		if (!compen_intersect(firstconel->ex, firstconel->ey, da1, secondconel->sx, secondconel->sy, da2, &ix, &iy))
		{
			/* lines are not parallel: adjust the segments to meet at the intersection point */
			compen_debugdump(M_("Line [%s] from (%s,%s) to (%s,%s) and line [%s] from (%s,%s) to (%s,%s)"),
				firstname, latoa(firstconel->sx, 0), latoa(firstconel->sy, 0), latoa(firstconel->ex, 0),
					latoa(firstconel->ey, 0), secondname, latoa(secondconel->sx, 0), latoa(secondconel->sy, 0),
						latoa(secondconel->ex, 0), latoa(secondconel->ey, 0));
			compen_debugdump(M_("  meet at (%s,%s)"), latoa(ix, 0), latoa(iy, 0));
			firstconel->ex = ix;
			firstconel->ey = iy;
			secondconel->sx = ix;
			secondconel->sy = iy;
			return;
		}

		compen_debugdump(M_("Parallel lines [%s] from (%s,%s) to (%s,%s) and [%s] from (%s,%s) to (%s,%s)!"),
			firstname, latoa(firstconel->sx, 0), latoa(firstconel->sy, 0), latoa(firstconel->ex, 0),
				latoa(firstconel->ey, 0), secondname, latoa(secondconel->sx, 0), latoa(secondconel->sy, 0),
					latoa(secondconel->ex, 0), latoa(secondconel->ey, 0));

		/* lines are parallel: must insert a bridge segment */
		/* try bridge segment 15 degrees from end of first segment to the intersection of the second segment */
		dang = da1 + 15.0/180.0*EPI;   if (dang > EPI*2.0) dang -= EPI*2.0;
		(void)compen_intersect(firstconel->ex, firstconel->ey, dang, secondconel->sx, secondconel->sy, da2, &ix, &iy);
		if (ix >= mini(secondconel->sx, secondconel->ex) && ix <= maxi(secondconel->sx, secondconel->ex) &&
			iy >= mini(secondconel->sy, secondconel->ey) && iy <= maxi(secondconel->sy, secondconel->ey))
		{
			secondconel->sx = ix;
			secondconel->sy = iy;
			compen_insertbridge(firstconel, secondconel);
			return;
		}

		/* try bridge segment 15 degrees in the other direction */
		dang = da1 - 15.0/180.0*EPI;   if (dang < 0.0) dang += EPI*2.0;
		(void)compen_intersect(firstconel->ex, firstconel->ey, dang, secondconel->sx, secondconel->sy, da2, &ix, &iy);
		if (ix >= mini(secondconel->sx, secondconel->ex) && ix <= maxi(secondconel->sx, secondconel->ex) &&
			iy >= mini(secondconel->sy, secondconel->ey) && iy <= maxi(secondconel->sy, secondconel->ey))
		{
			secondconel->sx = ix;
			secondconel->sy = iy;
			compen_insertbridge(firstconel, secondconel);
			return;
		}

		/* can't get 15 degree slope to work: just insert the bridge */
		compen_insertbridge(firstconel, secondconel);
		return;
	}

	/* handle line-to-arc blending (rule 2.0) */
	if ((firstconel->elementtype == ARCSEGMENTTYPE || firstconel->elementtype == REVARCSEGMENTTYPE) &&
		secondconel->elementtype == LINESEGMENTTYPE)
	{
		compen_debugdump(M_("Arc [%s] from (%s,%s) to (%s,%s) and line [%s] from (%s,%s) to (%s,%s)"), firstname,
			latoa(firstconel->sx, 0), latoa(firstconel->sy, 0), latoa(firstconel->ex, 0), latoa(firstconel->ey, 0),
				secondname, latoa(secondconel->sx, 0), latoa(secondconel->sy, 0), latoa(secondconel->ex, 0),
					latoa(secondconel->ey, 0));
		if (impliedintersection != 0)
		{
			icount = circlelineintersection(firstconel->cx, firstconel->cy, firstconel->sx,
				firstconel->sy, secondconel->sx, secondconel->sy, secondconel->ex,
					secondconel->ey, &ix1, &iy1, &ix2, &iy2, compen_circletangentthresh);

			/* if both points are on the circle: choose the closest */
			if (icount == 2)
			{
				if (computedistance(secondconel->sx, secondconel->sy, ix1, iy1) >
					computedistance(secondconel->sx, secondconel->sy, ix2, iy2))
				{
					ix1 = ix2;   iy1 = iy2;
				}
			}
			if (icount > 0)
			{
				compen_debugdump(M_("   Rule 2.3 applies for explicit intersection at (%s,%s)"),
					latoa(ix1, 0), latoa(iy1, 0));
				secondconel->sx = ix1;   secondconel->sy = iy1;
				firstconel->ex = ix1;    firstconel->ey = iy1;
				return;
			}
		}

		/* see if the line intersects the arc (rule 2.2/2.3) */
		if (compen_arcintersection(firstconel, secondconel->sx, secondconel->sy, secondconel->ex, secondconel->ey,
			&ix, &iy))
		{
			/* see if it is rule 2.2 or 2.3 */
			ud1 = (USERDATA *)firstconel->userdata;
			ud2 = (USERDATA *)secondconel->userdata;
			if (fabs(ud1->percentage - ud2->percentage) > 5.0)
			{
				if (compen_insert15degreesegment(secondconel->ex, secondconel->ey, &secondconel->sx, &secondconel->sy,
					firstconel->ex, firstconel->ey))
				{
					compen_insertbridge(firstconel, secondconel);
					compen_debugdump(M_("   Rule 2.2 applies"));
					return;
				}
				compen_debugdump(M_("   Rule 2.2 should be used but cannot"));
			}
			compen_debugdump(M_("   Rule 2.3 applies, intersection at (%s,%s)"), latoa(ix, 0), latoa(iy, 0));
			secondconel->sx = ix;   secondconel->sy = iy;
			firstconel->ex = ix;    firstconel->ey = iy;
			return;
		}

		/* line does not intersect arc: extend arc tangent to line (rule 2.1) */
		if (compen_arctangent(firstconel, firstconel->ex, firstconel->ey, secondconel, secondconel->sx, secondconel->sy,
			&ix, &iy))
		{
			firstconel->ex = ix;    firstconel->ey = iy;
			compen_debugdump(M_("   Rule 2.1 applies, intersection at (%s,%s)"), latoa(ix, 0), latoa(iy, 0));
			compen_insertbridge(firstconel, secondconel);
			return;
		}

		/* failure to implement rule 2: replace smaller arc with straight line */
		compen_debugdump(M_("   Rule 2.1a applies (straightening)"));
		compen_straighten(firstconel, secondconel);
		return;
	}
	if (firstconel->elementtype == LINESEGMENTTYPE &&
		(secondconel->elementtype == ARCSEGMENTTYPE || secondconel->elementtype == REVARCSEGMENTTYPE))
	{
		compen_debugdump(M_("Arc [%s] from (%s,%s) to (%s,%s) and line [%s] from (%s,%s) to (%s,%s)"), secondname,
			latoa(secondconel->sx, 0), latoa(secondconel->sy, 0), latoa(secondconel->ex, 0), latoa(secondconel->ey, 0),
				firstname, latoa(firstconel->sx, 0), latoa(firstconel->sy, 0), latoa(firstconel->ex, 0),
					latoa(firstconel->ey, 0));
		if (impliedintersection != 0)
		{
			icount = circlelineintersection(secondconel->cx, secondconel->cy, secondconel->sx,
				secondconel->sy, firstconel->ex, firstconel->ey, firstconel->sx,
					firstconel->sy, &ix1, &iy1, &ix2, &iy2, compen_circletangentthresh);

			/* if both points are on the circle: choose the closest */
			if (icount == 2)
			{
				if (computedistance(firstconel->ex, firstconel->ey, ix1, iy1) >
					computedistance(firstconel->ex, firstconel->ey, ix2, iy2))
				{
					ix1 = ix2;   iy1 = iy2;
				}
			}
			if (icount > 0)
			{
				compen_debugdump(M_("   Rule 2.3 applies for explicit intersection at (%s,%s)"),
					latoa(ix1, 0), latoa(iy1, 0));
				secondconel->sx = ix1;   secondconel->sy = iy1;
				firstconel->ex = ix1;    firstconel->ey = iy1;
				return;
			}
		}

		/* see if the line intersects the arc (rule 2.2/2.3) */
		if (compen_arcintersection(secondconel, firstconel->ex, firstconel->ey, firstconel->sx, firstconel->sy,
			&ix, &iy))
		{
			/* see if it is rule 2.2 or 2.3 */
			ud1 = (USERDATA *)firstconel->userdata;
			ud2 = (USERDATA *)secondconel->userdata;
			if (fabs(ud1->percentage - ud2->percentage) > 5.0)
			{
				/* use bridge segment 15 degrees from end of arc to the intersection of the line */
				if (compen_insert15degreesegment(firstconel->sx, firstconel->sy, &firstconel->ex, &firstconel->ey,
					secondconel->sx, secondconel->sy))
				{
					compen_insertbridge(firstconel, secondconel);
					compen_debugdump(M_("   Rule 2.2 applies"));
					return;
				}
				compen_debugdump(M_("   Rule 2.2 should be used but cannot"));
			}
			compen_debugdump(M_("   Rule 2.3 applies, intersection at (%s,%s)"), latoa(ix, 0), latoa(iy, 0));
			secondconel->sx = ix;   secondconel->sy = iy;
			firstconel->ex = ix;    firstconel->ey = iy;
			return;
		}

		/* line does not intersect arc: extend arc tangent to line (rule 2.1) */
		if (compen_arctangent(secondconel, secondconel->sx, secondconel->sy, firstconel, firstconel->ex, firstconel->ey,
			&ix, &iy))
		{
			compen_debugdump(M_("   Rule 2.1 applies, intersection at (%s,%s)"), latoa(ix, 0), latoa(iy, 0));
			secondconel->sx = ix;   secondconel->sy = iy;
			compen_insertbridge(firstconel, secondconel);
			return;
		}

		/* failure to implement rule 2: replace smaller arc with straight line */
		compen_debugdump(M_("   Rule 2.1a applies (straightening)"));
		compen_straighten(firstconel, secondconel);
		return;
	}

	/* handle arc-to-arc blending (rule 3.0) */
	if ((firstconel->elementtype == ARCSEGMENTTYPE || firstconel->elementtype == REVARCSEGMENTTYPE) &&
		(secondconel->elementtype == ARCSEGMENTTYPE || secondconel->elementtype == REVARCSEGMENTTYPE))
	{
		compen_debugdump(M_("Arc [%s] from (%s,%s) to (%s,%s) and Arc [%s] from (%s,%s) to (%s,%s)"), secondname,
			latoa(secondconel->sx, 0), latoa(secondconel->sy, 0), latoa(secondconel->ex, 0), latoa(secondconel->ey, 0),
				firstname, latoa(firstconel->sx, 0), latoa(firstconel->sy, 0), latoa(firstconel->ex, 0),
					latoa(firstconel->ey, 0));
		if (firstconel->elementtype == secondconel->elementtype)
		{
			/* curvature is the same: use rules 3.0 */
			/* draw from first endpoint to tangent on second arc */
			if (compen_arctangent(secondconel, secondconel->sx, secondconel->sy, firstconel, firstconel->ex, firstconel->ey,
				&ix, &iy))
			{
				compen_debugdump(M_("   Rule 3.0 applies, intersection at (%s,%s)"), latoa(ix, 0), latoa(iy, 0));
				secondconel->sx = ix;   secondconel->sy = iy;
				compen_insertbridge(firstconel, secondconel);
				return;
			}

			/* draw from second endpoint to tangent on first arc */
			if (compen_arctangent(firstconel, firstconel->ex, firstconel->ey, secondconel, secondconel->sx, secondconel->sy,
				&ix, &iy))
			{
				compen_debugdump(M_("   Rule 3.0 applies, intersection at (%s,%s)"), latoa(ix, 0), latoa(iy, 0));
				firstconel->ex = ix;    firstconel->ey = iy;
				compen_insertbridge(firstconel, secondconel);
				return;
			}

			/* failure to implement rule 3.0: replace smaller arc with straight line */
			compen_debugdump(M_("   Rule 3.0 fails (straightening)"));
			compen_straighten(firstconel, secondconel);
			return;
		} else
		{
			/* curvature is different: use rules 3.0a and 3.0b */
			arcrad = computedistance(firstconel->cx, firstconel->cy, firstconel->sx, firstconel->sy);
			ix = (firstconel->ex + secondconel->sx) / 2;
			iy = (firstconel->ey + secondconel->sy) / 2;
			dang = compen_figureangle(firstconel->ex, firstconel->ey, secondconel->sx, secondconel->sy);
			dang += EPI / 2.0;   if (dang > EPI*2.0) dang -= EPI*2.0;
			newix = ix + rounddouble(cos(dang) * (double)arcrad);
			newiy = iy + rounddouble(sin(dang) * (double)arcrad);

			/* see if each arc has an intersection perpendicular to the midpoint (rule 3.0a) */
			if (compen_arcintersection(firstconel, ix, iy, newix, newiy, &ix1, &iy1) &&
				compen_arcintersection(secondconel, ix, iy, newix, newiy, &ix2, &iy2))
			{
				compen_debugdump(M_("   Rule 3.0a applies, intersection from (%s,%s) to (%s,%s)"),
					latoa(ix1, 0), latoa(iy1, 0), latoa(ix2, 0), latoa(iy2, 0));
				firstconel->ex = ix1;    firstconel->ey = iy1;
				secondconel->ex = ix2;   secondconel->ey = iy2;
				compen_insertbridge(firstconel, secondconel);
				return;
			}

			/* use tangent points (rule 3.0b) */
			if (compen_arctangent(firstconel, firstconel->ex, firstconel->ey, NOCONTOURELEMENT, ix, iy, &ix1, &iy1) &&
				compen_arctangent(secondconel, secondconel->sx, secondconel->sy, NOCONTOURELEMENT, ix, iy, &ix2, &iy2))
			{
				compen_debugdump(M_("   Rule 3.0b applies, intersection from (%s,%s) to (%s,%s)"),
					latoa(ix1, 0), latoa(iy1, 0), latoa(ix2, 0), latoa(iy2, 0));
				firstconel->ex = ix1;    firstconel->ey = iy1;
				secondconel->ex = ix2;   secondconel->ey = iy2;
				compen_insertbridge(firstconel, secondconel);
				return;
			}

			/* cannot figure it out: just insert the bridge */
			compen_debugdump(M_("   No 3.0a/b Rule applies!"));
			compen_insertbridge(firstconel, secondconel);
			return;
		}
	}

	/* this should never happen!!! */
	compen_insertbridge(firstconel, secondconel);
}

/*
 * routine to see if a segment can be inserted between point (fromx,fromy) and the line segment (linesx,linesy) to
 * (*lineex,*lineey).  The segment must be 15 degrees offset from the line segment and must fall on the segment.
 * If such a line is possible, then the point (*lineex,*lineey) is adjusted to be at that point and the routine
 * returns true.
 */
BOOLEAN compen_insert15degreesegment(INTBIG linesx, INTBIG linesy, INTBIG *lineex, INTBIG *lineey, INTBIG fromx, INTBIG fromy)
{
	double a1, ang;
	INTBIG ix, iy;

	a1 = compen_figureangle(*lineex, *lineey, linesx, linesy);
	ang = a1 + 15.0/180.0*EPI;
	if (ang > EPI*2.0) ang -= EPI*2.0;
	(void)compen_intersect(fromx, fromy, ang, *lineex, *lineey, a1, &ix, &iy);
	if (ix >= mini(linesx, *lineex) && ix <= maxi(linesx, *lineex) &&
		iy >= mini(linesy, *lineey) && iy <= maxi(linesy, *lineey))
	{
		*lineex = ix;
		*lineey = iy;
		return(TRUE);
	}

	ang = a1 - 15.0/180.0*EPI;
	if (ang < 0.0) ang += EPI*2.0;
	(void)compen_intersect(fromx, fromy, ang, *lineex, *lineey, a1, &ix, &iy);
	if (ix >= mini(linesx, *lineex) && ix <= maxi(linesx, *lineex) &&
		iy >= mini(linesy, *lineey) && iy <= maxi(linesy, *lineey))
	{
		*lineex = ix;
		*lineey = iy;
		return(TRUE);
	}
	return(FALSE);
}


/*
 * routine to straighten out one of the curved segments "firstconel" or "secondconel" to implement
 * rule 2.1a (when the curves are too small to be blended properly).
 */
void compen_straighten(CONTOURELEMENT *firstconel, CONTOURELEMENT *secondconel)
{
	REGISTER INTBIG ang1, ang2;
	double startoffset, endangle;

	/* get the angles of the two segments */
	ang1 = ang2 = SMALLANGLETHRESH+1;
	if (firstconel->elementtype == ARCSEGMENTTYPE || firstconel->elementtype == REVARCSEGMENTTYPE)
	{
		getarcdegrees(firstconel->ni, &startoffset, &endangle);
		if (startoffset != 0.0 || endangle != 0.0)
			ang1 = rounddouble(startoffset + endangle);
	}
	if (secondconel->elementtype == ARCSEGMENTTYPE || secondconel->elementtype == REVARCSEGMENTTYPE)
	{
		getarcdegrees(secondconel->ni, &startoffset, &endangle);
		if (startoffset != 0.0 || endangle != 0.0)
			ang2 = rounddouble(startoffset + endangle);
	}

	/* if both are small angle arcs, choose the smaller one */
	if (ang1 <= SMALLANGLETHRESH && ang2 <= SMALLANGLETHRESH)
	{
		if (ang1 < ang2) ang2 = SMALLANGLETHRESH+1; else
			ang1 = SMALLANGLETHRESH+1;
	}

	/* straighten out the smaller angle arc */
	if (ang1 <= SMALLANGLETHRESH)
	{
		/* straighten out first element and make it join first */
		firstconel->elementtype = LINESEGMENTTYPE;
		firstconel->ex = secondconel->sx;
		firstconel->ey = secondconel->sy;
		return;
	}
	if (ang2 <= SMALLANGLETHRESH)
	{
		/* straighten out second element and make it join first */
		secondconel->elementtype = LINESEGMENTTYPE;
		secondconel->sx = firstconel->ex;
		secondconel->sy = firstconel->ey;
		return;
	}

	/* no small arcs: just insert a straight line */
	compen_insertbridge(firstconel, secondconel);
}

/*
 * routine to determine whether the line from (x,y) to (otherx,othery) intersects the arc element
 * "arcconel".  If it does, the routine returns true and sets the intersection point to (ix,iy).
 * The first coordinate of the line (x,y) is presumed to be the closest to the desired arc
 * intersection point.
 */
BOOLEAN compen_arcintersection(CONTOURELEMENT *arcconel, INTBIG x, INTBIG y, INTBIG otherx, INTBIG othery,
	INTBIG *ix, INTBIG *iy)
{
	REGISTER INTBIG icount, off1, off2;
	INTBIG ix1, iy1, ix2, iy2;

	icount = circlelineintersection(arcconel->cx, arcconel->cy, arcconel->sx, arcconel->sy,
		x, y, otherx, othery, &ix1, &iy1, &ix2, &iy2, compen_circletangentthresh);

	/* eliminate points that are not on the arc (unless both are off) */
	if (icount == 2)
	{
		off1 = compen_pointoffarc(arcconel, ix1, iy1);
		off2 = compen_pointoffarc(arcconel, ix2, iy2);
		if (off1 == 0 && off2 != 0)
		{
			icount = 1;
		} else if (off1 != 0 && off2 == 0)
		{
			icount = 1;
			ix1 = ix2;   iy1 = iy2;
		}
	}

	/* if both points are on the arc: choose the closest */
	if (icount == 2)
	{
		if (computedistance(x, y, ix1, iy1) > computedistance(x, y, ix2, iy2))
		{
			ix1 = ix2;   iy1 = iy2;
		}
	}

	/* if there is an intersection point, return it */
	if (icount >= 1)
	{
		*ix = ix1;
		*iy = iy1;
		return(TRUE);
	}
	return(FALSE);
}

/*
 * routine to find the tangent point(s) on an arc that connect to a given point.
 * The arc is "arcconel" and the end of the arc that *SHOULD* be close to the tangents is
 * (prefx, prefy).  The point is (x,y) and it may be on arc "otherconel" (if it is not
 * NOCONTOURELEMENT). If a tangent is found, it is put in (ix,iy) and the routine returns true.
 */
BOOLEAN compen_arctangent(CONTOURELEMENT *arcconel, INTBIG prefx, INTBIG prefy,
	CONTOURELEMENT *otherconel, INTBIG x, INTBIG y, INTBIG *ix, INTBIG *iy)
{
	INTBIG ix1, iy1, ix2, iy2, x1, y1, x2, y2;
	REGISTER INTBIG pt1offarc, pt2offarc, ang, angt1, angt2, diff1, diff2;

	if (circletangents(x, y, arcconel->cx, arcconel->cy, arcconel->sx, arcconel->sy, &ix1, &iy1, &ix2, &iy2))
		return(FALSE);
	pt1offarc = compen_pointoffarc(arcconel, ix1, iy1);
	pt2offarc = compen_pointoffarc(arcconel, ix2, iy2);

	/* decide which tangent to use if both are possible */
	if (pt1offarc == 0 && pt2offarc == 0)
	{
		/* use minimum distance to distinguish the proper tangent */
		if (computedistance(prefx, prefy, ix1, iy1) > computedistance(prefx, prefy, ix2, iy2))
			pt1offarc = 1; else
				pt2offarc = 1;

		/* if there is a contour element on the point, make sure the tangent is in the right direction */
		if (otherconel != NOCONTOURELEMENT)
		{
			initcontoursegmentgeneration(otherconel);
			(void)nextcontoursegmentgeneration(&x1, &y1, &x2, &y2);
			if (x1 != x || y1 != y)
			{
				for(;;)
				{
					if (nextcontoursegmentgeneration(&x2, &y2, &x1, &y1)) break;
				}
			}

			/* check angle about (x,y) between (x2,y2) and intersection points */
			if ((x != x2 || y != y2) && (x != ix1 || y != iy1) && (x != ix2 || y != iy2))
			{
				ang = figureangle(x, y, x2, y2);
				angt1 = figureangle(x, y, ix1, iy1);
				angt2 = figureangle(x, y, ix2, iy2);
				diff1 = abs(angt1-ang);   if (diff1 > 1800) diff1 = 3600 - diff1;
				diff2 = abs(angt2-ang);   if (diff2 > 1800) diff2 = 3600 - diff2;
				if (diff1 > diff2) { pt1offarc = 0; pt2offarc = 1; } else
					{ pt1offarc = 1; pt2offarc = 0; }
			}
		}
	}

	/* see if either tangent can be used if both are off of the arc */
	if (pt1offarc != 0 && pt2offarc != 0)
	{
		if (pt1offarc < pt2offarc)
		{
			if (pt1offarc < ARCSLOP) pt1offarc = 0;
		} else
		{
			if (pt2offarc < ARCSLOP) pt2offarc = 0;
		}
	}

	/* return the selected tangent */
	if (pt1offarc == 0)
	{
		*ix = ix1;
		*iy = iy1;
		return(TRUE);
	}
	if (pt2offarc == 0)
	{
		*ix = ix2;
		*iy = iy2;
		return(TRUE);
	}

	/* failure to find a tangent */
	return(FALSE);
}

/*
 * routine to insert a bridge segment between contour elements "firstconel" and "secondconel".
 */
void compen_insertbridge(CONTOURELEMENT *firstconel, CONTOURELEMENT *secondconel)
{
	REGISTER CONTOURELEMENT *bridgeconel, *swapconel;
	REGISTER USERDATA *ud, *ud1, *ud2;

	/* make sure the two elements are in the right order */
	if (secondconel->nextcontourelement == firstconel ||
		(firstconel->nextcontourelement != secondconel && secondconel->nextcontourelement == NOCONTOURELEMENT))
	{
		swapconel = firstconel;   firstconel = secondconel;   secondconel = swapconel;
	}

	/* stop if no bridge needed */
	if (firstconel->ex == secondconel->sx && firstconel->ey == secondconel->sy) return;

	/* create the bridge element */
	bridgeconel = (CONTOURELEMENT *)emalloc(sizeof (CONTOURELEMENT), compen_tool->cluster);
	if (bridgeconel == 0) return;
	ud = (USERDATA *)emalloc(sizeof (USERDATA), compen_tool->cluster);
	if (ud == 0) return;
	bridgeconel->userdata = (INTBIG)ud;
	bridgeconel->elementtype = BRIDGESEGMENTTYPE;
	bridgeconel->ni = NONODEINST;
	ud->origsx = bridgeconel->sx = firstconel->ex;
	ud->origsy = bridgeconel->sy = firstconel->ey;
	ud->origex = bridgeconel->ex = secondconel->sx;
	ud->origey = bridgeconel->ey = secondconel->sy;
	ud->DXFlayer = 0;
	ud->percentage = -1.0;
	ud->lowx = ud->highx = 0;
	ud->lowy = ud->highy = 0;
	ud1 = (USERDATA *)firstconel->userdata;
	ud2 = (USERDATA *)secondconel->userdata;
	if (ud1->DXFlayer != 0)
		(void)allocstring(&ud->DXFlayer, ud1->DXFlayer, compen_tool->cluster); else
			if (ud2->DXFlayer != 0)
				(void)allocstring(&ud->DXFlayer, ud2->DXFlayer, compen_tool->cluster);
	bridgeconel->nextcontourelement = firstconel->nextcontourelement;
	firstconel->nextcontourelement = bridgeconel;
	compen_debugdump(M_("Inserting bridge from (%s,%s) to (%s,%s)"), latoa(bridgeconel->sx, 0),
		latoa(bridgeconel->sy, 0), latoa(bridgeconel->ex, 0), latoa(bridgeconel->ey, 0));
}

/******************************** CONTOUR NESTING DETERMINATION ********************************/

/*
 * routine to examine each contour and create a parent/child tree that indicates nexting.
 * From this tree, the depth field is determined for each contour.
 */
void compen_ordercontours(CONTOUR *contourlist, NODEPROTO *np)
{
	REGISTER CONTOUR *con;
	CONTOUR toplevel;
	REGISTER INTBIG i;

	/* initialize the tree, including the static top-level */
	for(con = contourlist; con != NOCONTOUR; con = con->nextcontour) con->childcount = 0;
	toplevel.childtotal = 0;
	toplevel.childcount = 0;

	/* look at every contour and place it in the tree */
	for(con = contourlist; con != NOCONTOUR; con = con->nextcontour)
	{
		if (con->valid == 0) continue;
		compen_insertcontour(con, &toplevel);
	}

	/* now compute depth factors */
	for(i=0; i<toplevel.childcount; i++)
		compen_assigndepth(toplevel.children[i], 0);
}

/*
 * routine to recursively assign contour depth from the nesting tree.
 */
void compen_assigndepth(CONTOUR *con, INTBIG depth)
{
	REGISTER INTBIG i;

	con->depth = (INTSML)depth;
	for(i=0; i<con->childcount; i++)
		compen_assigndepth(con->children[i], depth+1);
}

/*
 * routine to return true if contour "lower" is inside of contour "higher"
 */
BOOLEAN compen_isinside(CONTOUR *lower, CONTOUR *higher)
{
	REGISTER INTBIG x, y, rad, dist;
	INTBIG x1, y1, x2, y2;
	REGISTER CONTOURELEMENT *conel;
	REGISTER INTBIG angles, ang, lastp, tang, thisp;

	/* trivial reject if bounding boxes don't overlap */
	if (lower->hx < higher->lx || lower->lx > higher->hx ||
		lower->hy < higher->ly || lower->ly > higher->hy) return(FALSE);

	/* general polygon containment by summing angles to vertices */
	x = lower->firstcontourelement->sx;
	y = lower->firstcontourelement->sy;

	/* special case if contour is a circle */
	conel = higher->firstcontourelement;
	if (conel->elementtype == CIRCLESEGMENTTYPE)
	{
		rad = computedistance(conel->cx, conel->cy, conel->sx, conel->sy);
		dist = computedistance(conel->cx, conel->cy, x, y);
		if (dist <= rad) return(TRUE);
		return(FALSE);
	}

	ang = 0;
	angles = 0;
	lastp = figureangle(x, y, higher->firstcontourelement->sx, higher->firstcontourelement->sy);
	for(conel = higher->firstcontourelement; conel != NOCONTOURELEMENT; conel = conel->nextcontourelement)
	{
		if (conel->elementtype == ARCSEGMENTTYPE || conel->elementtype == REVARCSEGMENTTYPE)
		{
			initcontoursegmentgeneration(conel);
			for(;;)
			{
				if (nextcontoursegmentgeneration(&x1, &y1, &x2, &y2)) break;
				thisp = figureangle(x, y, x2, y2);
				tang = lastp - thisp;
				if (tang < -1800) tang += 3600;
				if (tang > 1800) tang -= 3600;
				ang += tang;
				lastp = thisp;
				angles++;
			}
		} else
		{
			thisp = figureangle(x, y, conel->ex, conel->ey);
			tang = lastp - thisp;
			if (tang < -1800) tang += 3600;
			if (tang > 1800) tang -= 3600;
			ang += tang;
			lastp = thisp;
			angles++;
		}
	}

	if (abs(ang) <= angles) return(FALSE);
	return(TRUE);
}

/*
 * routine to recursively insert contour "newone" into the nesting tree
 * that starts at contour "thislevel".
 */
void compen_insertcontour(CONTOUR *newone, CONTOUR *thislevel)
{
	REGISTER INTBIG i, j, oldtotal;

	/* see if anything at this level is inside of the contour */
	for(i=0; i<thislevel->childcount; i++)
		if (compen_isinside(thislevel->children[i], newone)) break;
	if (i < thislevel->childcount)
	{
		/* contour encloses something at this level: split the level */
		oldtotal = thislevel->childcount;
		thislevel->childcount = 0;
		for(j=0; j<oldtotal; j++)
		{
			if (compen_isinside(thislevel->children[j], newone))
				compen_addchild(newone, thislevel->children[j]); else
					compen_addchild(thislevel, thislevel->children[j]);
		}
	}

	/* see if this contour is inside any on this level */
	for(i=0; i<thislevel->childcount; i++)
	{
		if (compen_isinside(newone, thislevel->children[i]))
		{
			compen_insertcontour(newone, thislevel->children[i]);
			return;
		}
	}

	/* not inside of these, add to the list */
	compen_addchild(thislevel, newone);
}

/*
 * routine to insert contour "child" into parent contour "parent".
 */
void compen_addchild(CONTOUR *parent, CONTOUR *child)
{
	REGISTER INTBIG i, newtotal;
	REGISTER CONTOUR **newchildren;

	if (parent->childcount >= parent->childtotal)
	{
		newtotal = parent->childcount+5;
		newchildren = (CONTOUR **)emalloc(newtotal * (sizeof (CONTOUR *)),
			compen_tool->cluster);
		if (newchildren == 0) return;
		for(i=0; i<parent->childcount; i++) newchildren[i] = parent->children[i];
		if (parent->childtotal > 0) efree((CHAR *)parent->children);
		parent->children = newchildren;
		parent->childtotal = (INTSML)newtotal;
	}
	parent->children[parent->childcount++] = child;
}

/******************************** CONTOUR ORIENTATION ********************************/

#define SCALEFACTOR 2000

/*
 * routine to make sure all contours run clockwise.
 */
void compen_orientcontours(CONTOUR *contourlist, NODEPROTO *np)
{
	REGISTER CONTOUR *con;
	REGISTER CONTOURELEMENT *conel, *lastconel, *nextconel;
	REGISTER INTBIG swap, xd, yd, xfactor, yfactor, area;
	INTBIG x1, y1, x2, y2;

	for(con = contourlist; con != NOCONTOUR; con = con->nextcontour)
	{
		if (con->valid == 0) continue;
		if (con->hx-con->lx < SCALEFACTOR) xfactor = 1; else xfactor = (con->hx-con->lx) / SCALEFACTOR;
		if (con->hy-con->ly < SCALEFACTOR) yfactor = 1; else yfactor = (con->hy-con->ly) / SCALEFACTOR;
		area = 0;
		for(conel = con->firstcontourelement; conel != NOCONTOURELEMENT; conel = conel->nextcontourelement)
		{
			if (conel->elementtype == ARCSEGMENTTYPE || conel->elementtype == REVARCSEGMENTTYPE)
			{
				initcontoursegmentgeneration(conel);
				for(;;)
				{
					if (nextcontoursegmentgeneration(&x1, &y1, &x2, &y2)) break;
					x1 /= xfactor;   y1 /= yfactor;
					x2 /= xfactor;   y2 /= yfactor;
					xd = x2 - x1;    yd = y2 + y1;
					area += xd * yd / 2;
				}
			} else if (conel->elementtype == BRIDGESEGMENTTYPE || conel->elementtype == LINESEGMENTTYPE)
			{
				xd = conel->ex/xfactor - conel->sx/xfactor;
				yd = conel->ey/yfactor + conel->sy/yfactor;
				area += xd * yd / 2;
			}
		}

		/* reverse the contour if the area is negative */
		if (area < 0)
		{
			lastconel = NOCONTOURELEMENT;
			for(conel = con->firstcontourelement; conel != NOCONTOURELEMENT; conel = nextconel)
			{
				nextconel = conel->nextcontourelement;
				conel->nextcontourelement = lastconel;
				lastconel = conel;
				switch (conel->elementtype)
				{
					case LINESEGMENTTYPE:
					case BRIDGESEGMENTTYPE:
					case ARCSEGMENTTYPE:
					case REVARCSEGMENTTYPE:
						swap = conel->sx;    conel->sx = conel->ex;    conel->ex = swap;
						swap = conel->sy;    conel->sy = conel->ey;    conel->ey = swap;
						if (conel->elementtype == ARCSEGMENTTYPE) conel->elementtype = REVARCSEGMENTTYPE; else
							if (conel->elementtype == REVARCSEGMENTTYPE) conel->elementtype = ARCSEGMENTTYPE;
						break;
					default:
						break;
				}
			}
			con->firstcontourelement = lastconel;
		}
	}
}

/******************************** SUPPORT ********************************/

/*
 * Routine to print the distance from (px,py) to the nearest point on the line
 * from (fx,fy) to (tx,ty).
 */
void compen_printdistance(double px, double py, INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty)
{
	double lineangle, perpangle, fa1, fb1, fc1, fa2, fb2, fc2, fswap, ix, iy, dx, dy, dist;

	/* determine angle of line from (fx,fy) to (tx,ty) */
	if (ty == fy && tx == fx)
	{
		ttyputerr(M_("Domain error examining distance"));
		return;
	}
	lineangle = atan2((double)(ty-fy), (double)(tx-fx));
	if (lineangle < 0.0) lineangle += EPI*2.0;

	/* determine perpendicular angle */
	perpangle = lineangle + EPI / 2.0;
	if (perpangle > EPI*2.0) perpangle -= EPI*2.0;

	fa1 = sin(lineangle);   fb1 = -cos(lineangle);
	fc1 = -fa1 * ((double)fx) - fb1 * ((double)fy);
	fa2 = sin(perpangle);   fb2 = -cos(perpangle);
	fc2 = -fa2 * px - fb2 * py;
	if (fabs(fa1) < fabs(fa2))
	{
		fswap = fa1;   fa1 = fa2;   fa2 = fswap;
		fswap = fb1;   fb1 = fb2;   fb2 = fswap;
		fswap = fc1;   fc1 = fc2;   fc2 = fswap;
	}
	iy = (fa2 * fc1 / fa1 - fc2) / (fb2 - fa2*fb1/fa1);
	ix = (-fb1 * iy - fc1) / fa1;
	dx = ix - px;   dy = iy - py;
	dist = sqrt(dx*dx + dy*dy);
	ttyputmsg(M_("         line moved by %s"), latoa(rounddouble(dist), 0));
}

void compen_drawline(INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, GRAPHICS *desc)
{
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 2, us_tool->cluster);

	poly->xv[0] = x1;        poly->yv[0] = y1;
	poly->xv[1] = x2;        poly->yv[1] = y2;
	poly->count = 2;
	poly->style = OPENED;
	poly->desc = desc;
	us_showpoly(poly, el_curwindowpart);
}

void compen_drawcircle(INTBIG centerx, INTBIG centery, INTBIG x, INTBIG y, GRAPHICS *desc)
{
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 2, us_tool->cluster);

	poly->xv[0] = centerx;   poly->yv[0] = centery;
	poly->xv[1] = x;         poly->yv[1] = y;
	poly->count = 2;
	poly->style = CIRCLE;
	poly->desc = desc;
	us_showpoly(poly, el_curwindowpart);
}

void compen_drawcirclearc(INTBIG centerx, INTBIG centery, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, GRAPHICS *desc)
{
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 3, us_tool->cluster);

	poly->xv[0] = centerx;   poly->yv[0] = centery;
	poly->xv[1] = x1;        poly->yv[1] = y1;
	poly->xv[2] = x2;        poly->yv[2] = y2;
	poly->count = 3;
	poly->style = CIRCLEARC;
	poly->desc = desc;
	us_showpoly(poly, el_curwindowpart);
}

/*
 * routine to determine the actual compensation amount given the percentage (and
 * making use of the two globals: metal thickness and LRS compensation adjustment).
 */
float compen_truecompensation(float percentage, float metalthickness, float lrscompensation)
{
	return(metalthickness * percentage / 200.0f - lrscompensation/2.0f);
}

/*
 * Routine to return zero if point (x,y) is on the arc segment "arcconel".
 * Returns the angulur distance off of the arc if not.
 */
INTBIG compen_pointoffarc(CONTOURELEMENT *arcconel, INTBIG x, INTBIG y)
{
	double as, ae, a;

	as = compen_figureangle(arcconel->cx, arcconel->cy, arcconel->sx, arcconel->sy) * 1800.0 / EPI;
	ae = compen_figureangle(arcconel->cx, arcconel->cy, arcconel->ex, arcconel->ey) * 1800.0 / EPI;
	a = compen_figureangle(arcconel->cx, arcconel->cy, x, y) * 1800.0 / EPI;
	if (arcconel->elementtype == ARCSEGMENTTYPE)
	{
		if (ae > as)
		{
			if (a >= as && a <= ae) return(0);
			return(mini(compen_angoffset(a,as), compen_angoffset(a,ae)));
		}
		if (a >= as || a <= ae) return(0);
		return(mini(compen_angoffset(a,as), compen_angoffset(a,ae)));
	}

	if (as > ae)
	{
		if (a >= ae && a <= as) return(0);
		return(mini(compen_angoffset(a,as), compen_angoffset(a,ae)));
	}
	if (a >= ae || a <= as) return(0);
	return(mini(compen_angoffset(a,as), compen_angoffset(a,ae)));
}

INTBIG compen_angoffset(double a1, double a2)
{
	REGISTER double dist;

	dist = fabs(a1 - a2);
	if (dist > 1800.0) dist -= 3600.0;
	return(rounddouble(fabs(dist)));
}

double compen_figureangle(INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty)
{
	double ang;

	if (ty == fy && tx == fx)
	{
		ttyputerr(M_("Domain error computing angle"));
		return(0.0);
	}
	ang = atan2((double)ty-fy, (double)tx-fx);
	if (ang < 0.0) ang += EPI*2.0;
	return(ang);
}

BOOLEAN compen_intersect(INTBIG x1, INTBIG y1, double fang1, INTBIG x2, INTBIG y2, double fang2, INTBIG *x, INTBIG *y)
{
	double fa1, fb1, fc1, fa2, fb2, fc2, fswap, fy;

	/* cannot handle lines if they are at the same angle */
	if (fang1 == fang2) return(TRUE);

	/* also at the same angle if off by 180 degrees */
	if (fang1 < fang2)
	{
		if (fang1 + EPI == fang2) return(TRUE);
	} else
	{
		if (fang1 - EPI == fang2) return(TRUE);
	}

	fa1 = sin(fang1);   fb1 = -cos(fang1);
	fc1 = -fa1 * ((double)x1) - fb1 * ((double)y1);
	fa2 = sin(fang2);   fb2 = -cos(fang2);
	fc2 = -fa2 * ((double)x2) - fb2 * ((double)y2);
	if (fabs(fa1) < fabs(fa2))
	{
		fswap = fa1;   fa1 = fa2;   fa2 = fswap;
		fswap = fb1;   fb1 = fb2;   fb2 = fswap;
		fswap = fc1;   fc1 = fc2;   fc2 = fswap;
	}
	fy = (fa2 * fc1 / fa1 - fc2) / (fb2 - fa2*fb1/fa1);
	*y = rounddouble(fy);
	*x = rounddouble((-fb1 * fy - fc1) / fa1);
	return(FALSE);
}

/*
 * Routine to queue all contours on cell "np" for deletion.
 */
void compen_removecellcontours(NODEPROTO *np)
{
	REGISTER CELLCONTOURS *fc;

	for(fc = compen_firstcellcontours; fc != NOCELLCONTOURS; fc = fc->next)
		if (fc->cell == np && !fc->deleted) break;
	if (fc == NOCELLCONTOURS) return;

	/* mark this as deleted and queue cleanup */
	fc->deleted = TRUE;
	compen_deletedcellcontours = 1;
}

/*
 * routine to get the list of contours currently stored on "np".  If there
 * is nothing stored, compute it and store it.
 * Returns NOCONTOUR if there is no list and no contours.
 */
CONTOUR *compen_getcontourlist(NODEPROTO *np)
{
	REGISTER USERDATA *ud;
	REGISTER CONTOUR *con, *contourlist;
	REGISTER CONTOURELEMENT *conel;
	REGISTER INTBIG total, bestthresh, worstthresh;
	REGISTER CELLCONTOURS *fc;
	REGISTER VARIABLE *var;

	/* see if the data is already there */
	for(fc = compen_firstcellcontours; fc != NOCELLCONTOURS; fc = fc->next)
		if (fc->cell == np && !fc->deleted) return(fc->contour);

	/* gather the contours in this cell */
	ttyputmsg(M_("Gathering contours..."));
	bestthresh = scalefromdispunit((float)BESTTHRESH, DISPUNITMM);
	worstthresh = scalefromdispunit((float)WORSTTHRESH, DISPUNITMM);
	contourlist = gathercontours(np, 0, bestthresh, worstthresh);

	/* count 'em */
	total = 0;
	for(con = contourlist; con != NOCONTOUR; con = con->nextcontour)
		if (con->valid != 0) total++;
	ttyputmsg(M_("...Done gathering, found %ld contours"), total);

	/* add user data to each contour element */
	for(con = contourlist; con != NOCONTOUR; con = con->nextcontour)
	{
		con->userdata = 0;
		for(conel = con->firstcontourelement; conel != NOCONTOURELEMENT; conel = conel->nextcontourelement)
		{
			ud = (USERDATA *)emalloc(sizeof (USERDATA), compen_tool->cluster);
			if (ud == 0) return(0);
			conel->userdata = (INTBIG)ud;
			ud->DXFlayer = 0;
			if (conel->ni != NONODEINST)
			{
				var = getvalkey((INTBIG)conel->ni, VNODEINST, VSTRING, compen_dxf_layerkey);
				if (var != NOVARIABLE)
				{
					(void)allocstring(&ud->DXFlayer, (CHAR *)var->addr, compen_tool->cluster);
				}
			}
		}
	}

	/* store the list on the cell */
	if (contourlist != NOCONTOUR)
	{
		fc = (CELLCONTOURS *)emalloc(sizeof (CELLCONTOURS), compen_tool->cluster);
		if (fc == 0) return(NOCONTOUR);
		fc->cell = np;
		fc->contour = contourlist;
		fc->deleted = FALSE;
		fc->next = compen_firstcellcontours;
		compen_firstcellcontours = fc;

		/* turn on the tool so that it can track changes to the cell and force contour upgrades */
		toolturnon(compen_tool);
	}

	/* return the list */
	return(contourlist);
}

void compen_debugdump(CHAR *msg, ...)
{
#ifdef DEBDUMP
	va_list ap;
	CHAR line[256];

	var_start(ap, msg);
	evsnprintf(line, 256, msg, ap);
	va_end(ap);
	xprintf(compen_io, x_("%s\n"), line);
#endif
}

#endif  /* COMPENTOOL - at top */
