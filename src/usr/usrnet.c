/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrnet.c
 * User interface tool: network manipulation routines
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
#include "egraphics.h"
#include "usr.h"
#include "usrtrack.h"
#include "usrdiacom.h"
#include "database.h"
#include "conlay.h"
#include "efunction.h"
#include "edialogs.h"
#include "tech.h"
#include "tecgen.h"
#include "tecart.h"
#include "tecschem.h"
#include "sim.h"
#include <math.h>

#define EXACTSELECTDISTANCE  5			/* number of pixels for selection */
#define MAXCELLEXPLHSCROLL 200			/* maximum horizontal scroll */

/* working memory for "us_pickhigherinstance()" */
static INTBIG      us_pickinstlistsize = 0;
static NODEPROTO **us_pickinstcelllist;
static INTBIG     *us_pickinstinstcount;
static NODEINST  **us_pickinstinstlist;

/* working memory for "us_makenewportproto()" */
static INTBIG us_netportlimit = 0, *us_netportlocation;

/* working memory for "us_addexplorernode()" */
static INTBIG us_explorertempstringtotal = 0;
static CHAR  *us_explorertempstring;

/* for constructing implant coverage polygons */
#define NOCOVERRECT ((COVERRECT *)-1)

typedef struct Icoverrect
{
	INTBIG             lx, hx, ly, hy;
	INTBIG             contributions;
	INTBIG             layer;
	TECHNOLOGY        *tech;
	struct Icoverrect *nextcoverrect;
} COVERRECT;

/* for queuing of exports */
typedef struct
{
	NODEINST  *ni;
	PORTPROTO *pp;
	PORTPROTO *origpp;
} QUEUEDEXPORT;

static QUEUEDEXPORT *us_queuedexport;
static INTBIG        us_queuedexportcount;
static INTBIG        us_queuedexporttotal = 0;

/* for "explorer" window */
#define NOEXPLORERNODE ((EXPLORERNODE *)-1)

/* the meaning of EXPLORERNODE->flags */
#define EXNODEOPEN          1
#define EXNODESHOWN         2
#define EXNODEPLACEHOLDER   4

/* the meaning of EXPLORERNODE->enodetype */
#define EXNODETYPELIBRARY   0
#define EXNODETYPECELL      1
#define EXNODETYPENODE      2
#define EXNODETYPEARC       3
#define EXNODETYPENETWORK   4
#define EXNODETYPEEXPORT    5
#define EXNODETYPEERROR     6
#define EXNODETYPEERRORGEOM 7
#define EXNODETYPESIMBRANCH 8
#define EXNODETYPESIMLEAF   9
#define EXNODETYPELABEL    10

/* the meaning of EXPLORERNODE->addr when the "enodetype" is EXNODETYPELABEL */
#define EXLABELTYPEHIVIEW      0		/* hierarchical view */
#define EXLABELTYPECONVIEW     1		/* contents view */
#define EXLABELTYPEERRVIEW     2		/* error view */
#define EXLABELTYPESIMVIEW     3		/* simulation view */
#define EXLABELTYPENODELIST    4		/* node list in contents view */
#define EXLABELTYPEARCLIST     5		/* arc list in contents view */
#define EXLABELTYPEEXPORTLIST  6		/* export list in contents view */
#define EXLABELTYPENETLIST     7		/* network list in contents view */
#define EXLABELTYPELAYERCELLS  8		/* layer cells in contents view */
#define EXLABELTYPEARCCELLS    9		/* arc cells in contents view */
#define EXLABELTYPENODECELLS  10		/* node cells in contents view */
#define EXLABELTYPEMISCCELLS  11		/* miscellaneous cells in contents view */

typedef struct Iexplorernode
{
	INTBIG                addr;
	INTBIG                enodetype;
	INTBIG                flags;
	INTBIG                count;
	INTBIG                x, y;
	INTBIG                textwidth;
	BOOLEAN               allocated;
	struct Iexplorernode *subexplorernode;
	struct Iexplorernode *nextsubexplorernode;
	struct Iexplorernode *nextexplorernode;
	struct Iexplorernode *parent;
} EXPLORERNODE;

#define NOEXPWINDOW ((EXPWINDOW *)-1)

typedef struct Iexplorerwindow
{
	EXPLORERNODE  *firstnode;
	EXPLORERNODE  *nodehierarchy;			/* the top of the hierarchy tree */
	EXPLORERNODE  *nodecontents;			/* the top of the contents tree */
	EXPLORERNODE  *nodeerrors;				/* the top of the errors tree */
	EXPLORERNODE  *nodesimulation;			/* the top of the simulation tree */
	EXPLORERNODE  *nodeselected;			/* the selected explorer node */
	EXPLORERNODE  *nodenowselected;			/* the newly selected explorer node */
	EXPLORERNODE  *nodeused;
	INTBIG         firstline;				/* first visible line in explorer window */
	INTBIG         firstchar;				/* first visible character in explorer window */
	INTBIG         totallines;
	INTBIG         depth;					/* size of explorer depth */
	INTBIG         stacksize;				/* space allocated for node stacks */
	EXPLORERNODE **stack;					/* stack of explorer nodes */
	BOOLEAN       *stackdone;				/* stack of explorer nodes that are done */
	INTBIG         sliderpart;				/* 0:down arrow 1:below thumb 2:thumb 3:above thumb 4:up arrow */
	INTBIG         deltasofar;				/* for thumb tracking */
	INTBIG         initialthumb;			/* for thumb tracking */
} EXPWINDOW;

static WINDOWPART    *us_explorertrackwindow;				/* current window when arrows are clicked */
static EXPLORERNODE  *us_explorernodefree = NOEXPLORERNODE;

static INTBIG         us_exploretextsize;
static INTBIG         us_exploretextheight;
static INTBIG         us_exploretextwidth;

/* prototypes for local routines */
static void          us_recursivelysearch(RTNODE*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, HIGHLIGHT*, HIGHLIGHT*, HIGHLIGHT*, HIGHLIGHT*, HIGHLIGHT*, INTBIG*, INTBIG*, INTBIG, INTBIG, WINDOWPART*, INTBIG);
static void          us_checkoutobject(GEOM*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, HIGHLIGHT*, HIGHLIGHT*, HIGHLIGHT*, HIGHLIGHT*, HIGHLIGHT*, INTBIG*, INTBIG*, INTBIG, INTBIG, WINDOWPART*);
static void          us_fartextsearch(NODEPROTO*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, HIGHLIGHT*, HIGHLIGHT *, HIGHLIGHT *, HIGHLIGHT*, HIGHLIGHT*, INTBIG*, INTBIG*, INTBIG, INTBIG, WINDOWPART*, INTBIG);
static void          us_initarctext(ARCINST*, INTBIG, WINDOWPART*);
static BOOLEAN       us_getarctext(ARCINST*, WINDOWPART*, POLYGON*, VARIABLE**, VARIABLE**);
static INTBIG        us_findnewplace(INTBIG*, INTBIG, INTBIG, INTBIG);
static void          us_setbestsnappoint(HIGHLIGHT *best, INTBIG wantx, INTBIG wanty, INTBIG newx, INTBIG newy, BOOLEAN tan, BOOLEAN perp);
static void          us_selectsnappoly(HIGHLIGHT *best, POLYGON *poly, INTBIG wantx, INTBIG wanty);
static BOOLEAN       us_pointonarc(INTBIG x, INTBIG y, POLYGON *poly);
static void          us_intersectsnapline(HIGHLIGHT *best, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2,
						POLYGON *interpoly, INTBIG wantx, INTBIG wanty);
static void          us_intersectsnappoly(HIGHLIGHT *best, POLYGON *poly, POLYGON *interpoly, INTBIG wantx, INTBIG wanty);
static void          us_adjustonetangent(HIGHLIGHT *high, POLYGON *poly, INTBIG x, INTBIG y);
static void          us_adjustoneperpendicular(HIGHLIGHT *high, POLYGON *poly, INTBIG x, INTBIG y);
static void          us_xformpointtonode(INTBIG x, INTBIG y, NODEINST *ni, INTBIG *xo, INTBIG *yo);
static INTBIG        us_alignvalueround(INTBIG value);
static INTBIG        us_disttoobject(INTBIG x, INTBIG y, GEOM *geom);
static void          us_addtocoverlist(POLYGON *poly, COVERRECT **crlist);
static void          us_choparc(ARCINST *ai, INTBIG keepend, INTBIG ix, INTBIG iy);
static void          us_createhierarchicalexplorertree(EXPWINDOW *ew, EXPLORERNODE *en, NODEPROTO *cell);
static EXPLORERNODE *us_allocexplorernode(EXPWINDOW *ew);
static void          us_freeexplorernode(EXPLORERNODE *en);
static void          us_addexplorernode(EXPLORERNODE *en, EXPLORERNODE **head, EXPLORERNODE *parent);
static BOOLEAN       us_expandexplorerdepth(EXPWINDOW *ew);
static void          us_buildexplorerstruct(WINDOWPART *w, EXPWINDOW *ew);
static INTBIG        us_scannewexplorerstruct(EXPWINDOW *ew, EXPLORERNODE *firsten, EXPLORERNODE *oldfirsten, INTBIG line);
static void          us_highlightexplorernode(WINDOWPART *w);
static INTBIG        us_iconposition(PORTPROTO *pp, INTBIG style);
static BOOLEAN       us_explorerarrowdown(INTBIG x, INTBIG y);
static BOOLEAN       us_explorerarrowleft(INTBIG x, INTBIG y);
static void          us_filltextpoly(CHAR *str, WINDOWPART *win, INTBIG xc, INTBIG yc, XARRAY trans,
						TECHNOLOGY *tech, UINTBIG *descript, GEOM *geom, POLYGON *poly);
static BOOLEAN       us_indestlib(NODEPROTO *np, LIBRARY *lib);
static void          us_ehthumbtrackingtextcallback(INTBIG delta);
static void          us_evthumbtrackingtextcallback(INTBIG delta);
static BOOLEAN       us_alreadycellcenter(NODEPROTO *np);
static void          us_manhattantravel(NODEINST*, BOOLEAN);
static int           us_queuedexportnameascending(const void *e1, const void *e2);
static ARCINST      *us_pastarctoarc(ARCINST *destarc, ARCINST *srcarc);
static NODEINST     *us_pastnodetonode(NODEINST *destnode, NODEINST *srcnode);
static BOOLEAN       us_linethroughbox(INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty,
						INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy, INTBIG *intx, INTBIG *inty);
static void          us_makecontactstack(ARCINST *ai, INTBIG end, ARCPROTO *ap,
						NODEINST **conni, PORTPROTO **conpp);
static INTBIG        us_findpathtoarc(PORTPROTO *pp, ARCPROTO *ap, INTBIG depth);
static BOOLEAN       us_makeiconexport(PORTPROTO *pp, INTBIG style, INTBIG index,
						INTBIG xpos, INTBIG ypos, INTBIG xbbpos, INTBIG ybbpos, NODEPROTO *np);
static void          us_buildexplorersturcthierarchical(EXPWINDOW *ew, EXPLORERNODE *whichtree);
static void          us_buildexplorersturctcontents(EXPWINDOW *ew, EXPLORERNODE *whichtree);
static void          us_buildexplorernodecontents(EXPWINDOW *ew, NODEPROTO *np, EXPLORERNODE *whichtree);
static CHAR         *us_describeexplorernode(EXPLORERNODE *en, BOOLEAN purename);
static void          us_clearexplorererrors(void);
static void          us_clearexplorersimulation(void);
static void          us_buildexplorersturcterrors(EXPWINDOW *ew, EXPLORERNODE *whichtree);
static void          us_buildexplorersturctsimulation(WINDOWPART *w, EXPWINDOW *ew, EXPLORERNODE *whichtree);
static void          us_explorrecurseexpand(WINDOWPART *w, EXPWINDOW *ew, EXPLORERNODE *en, BOOLEAN expand);
static void          us_addtoexplorerwindowcopy(void *infstr, INTBIG depth, EXPLORERNODE *en);
static void          us_explorerdofunction(WINDOWPART *win, EXPLORERNODE *en, INTBIG funct);
static void          us_explorertoggleopenness(WINDOWPART *w, EXPLORERNODE *en);
static void          us_explorerdelete(EXPLORERNODE *en);
static void          us_explorerrename(EXPLORERNODE *en, WINDOWPART *w);
static EXPWINDOW    *us_getcurrentexplorerwindow(void);

/*
 * Routine to free all memory associated with this module.
 */
void us_freenetmemory(void)
{
	REGISTER EXPLORERNODE *en;

	if (us_pickinstlistsize != 0)
	{
		efree((CHAR *)us_pickinstcelllist);
		efree((CHAR *)us_pickinstinstlist);
		efree((CHAR *)us_pickinstinstcount);
	}
	if (us_netportlimit > 0) efree((CHAR *)us_netportlocation);
	if (us_queuedexporttotal > 0) efree((CHAR *)us_queuedexport);
	if (us_explorertempstringtotal > 0) efree(us_explorertempstring);

	while (us_explorernodefree != NOEXPLORERNODE)
	{
		en = us_explorernodefree;
		us_explorernodefree = en->nextexplorernode;
		if (en->allocated) efree((CHAR *)en);
	}
}

/*********************************** CELL SUPPORT ***********************************/

/*
 * routine to recursively expand the cell "ni" by "amount" levels.
 * "sofar" is the number of levels that this has already been expanded.
 */
void us_doexpand(NODEINST *ni, INTBIG amount, INTBIG sofar)
{
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ono;

	if ((ni->userbits & NEXPAND) == 0)
	{
		/* expanded the cell */
		ni->userbits |= NEXPAND;
		ni->parent->lib->userbits |= LIBCHANGEDMINOR;

		/* if depth limit reached, quit */
		if (++sofar >= amount) return;
	}

	/* explore insides of this one */
	np = ni->proto;
	for(ono = np->firstnodeinst; ono != NONODEINST; ono = ono->nextnodeinst)
	{
		if (ono->proto->primindex != 0) continue;

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(ono->proto, np)) continue;
		us_doexpand(ono, amount, sofar);
	}
}

void us_dounexpand(NODEINST *ni)
{
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ono;

	if ((ni->userbits & NEXPAND) == 0) return;

	np = ni->proto;
	for(ono = np->firstnodeinst; ono != NONODEINST; ono = ono->nextnodeinst)
	{
		if (ono->proto->primindex != 0) continue;

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(ono->proto, np)) continue;
		if ((ono->userbits & NEXPAND) != 0) us_dounexpand(ono);
	}

	/* expanded the cell */
	if ((ni->userbits & WANTEXP) != 0)
	{
		ni->userbits &= ~NEXPAND;
		ni->parent->lib->userbits |= LIBCHANGEDMINOR;
	}
}

INTBIG us_setunexpand(NODEINST *ni, INTBIG amount)
{
	REGISTER INTBIG depth;
	REGISTER NODEINST *ono;
	REGISTER NODEPROTO *np;

	ni->userbits &= ~WANTEXP;
	if ((ni->userbits & NEXPAND) == 0) return(0);
	np = ni->proto;
	depth = 0;
	for(ono = np->firstnodeinst; ono != NONODEINST; ono = ono->nextnodeinst)
	{
		if (ono->proto->primindex != 0) continue;

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(ono->proto, np)) continue;
		if ((ono->userbits & NEXPAND) != 0)
			depth = maxi(depth, us_setunexpand(ono, amount));
	}
	if (depth < amount) ni->userbits |= WANTEXP;
	return(depth+1);
}

/*
 * Routine to recursively walk the hierarchy from "np", marking all cells below it.
 */
void us_recursivemark(NODEPROTO *np)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *cnp;

	if (np->temp1 != 0) return;
	np->temp1 = 1;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex != 0) continue;
		us_recursivemark(ni->proto);
		cnp = contentsview(ni->proto);
		if (cnp != NONODEPROTO) us_recursivemark(cnp);
	}
}

/*
 * Routine to adjust selected objects in the cell to even grid units.
 */
void us_regridselected(void)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER GEOM **list;
	REGISTER PORTPROTO *pp;
	REGISTER INTBIG bodyxoffset, bodyyoffset, portxoffset, portyoffset, pxo, pyo, i, j, tot,
		adjustednodes, adjustedarcs, end1xoff, end1yoff, end2xoff, end2yoff, ang;
	INTBIG lx, hx, ly, hy;
	REGISTER BOOLEAN mixedportpos;
	REGISTER PORTARCINST *pi;
	INTBIG px, py, cx, cy;
	XARRAY transr;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	if (us_alignment_ratio <= 0)
	{
		us_abortcommand(_("No alignment given: set Alignment Options first"));
		return;
	}
	list = us_gethighlighted(WANTNODEINST|WANTARCINST, 0, 0);
	if (list[0] == NOGEOM)
	{
		us_abortcommand(_("Must select something before aligning it to the grid"));
		return;
	}

	/* save and remove highlighting */
	us_pushhighlight();
	us_clearhighlightcount();

	adjustednodes = adjustedarcs = 0;
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (!list[i]->entryisnode) continue;
		ni = list[i]->entryaddr.ni;

		/* ignore pins */
		if (((ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH) == NPPIN) continue;
		us_getnodedisplayposition(ni, &cx, &cy);
		bodyxoffset = us_alignvalueround(cx) - cx;
		bodyyoffset = us_alignvalueround(cy) - cy;

		portxoffset = bodyxoffset;
		portyoffset = bodyyoffset;
		mixedportpos = FALSE;
		for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			portposition(ni, pp, &px, &py);
			pxo = us_alignvalueround(px) - px;
			pyo = us_alignvalueround(py) - py;
			if (pp == ni->proto->firstportproto)
			{
				portxoffset = pxo;   portyoffset = pyo;
			} else
			{
				if (portxoffset != pxo || portyoffset != pyo) mixedportpos = TRUE;
			}
		}
		if (!mixedportpos)
		{
			bodyxoffset = portxoffset;   bodyyoffset = portyoffset;
		}

		/* if a primitive has an offset, see if the node edges are aligned */
		if (bodyxoffset != 0 || bodyyoffset != 0)
		{
			if (ni->proto->primindex != 0)
			{
				makerot(ni, transr);
				tot = nodepolys(ni, 0, NOWINDOWPART);
				for(j=0; j<tot; j++)
				{
					shapenodepoly(ni, j, poly);
					xformpoly(poly, transr);
					if (!isbox(poly, &lx, &hx, &ly, &hy)) continue;
					if (us_alignvalueround(hx) == hx &&
						us_alignvalueround(lx) == lx) bodyxoffset = 0;
					if (us_alignvalueround(hy) == hy &&
						us_alignvalueround(ly) == ly) bodyyoffset = 0;
					if (bodyxoffset == 0 && bodyyoffset == 0) break;
				}
			}
		}

		/* move the node */
		if (bodyxoffset != 0 || bodyyoffset != 0)
		{
			/* turn off all constraints on arcs */
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			{
				ai = pi->conarcinst;
				ai->temp1 = ai->userbits;
				ai->userbits &= ~(FIXED|FIXANG);
			}
			startobjectchange((INTBIG)ni, VNODEINST);
			modifynodeinst(ni, bodyxoffset, bodyyoffset, bodyxoffset, bodyyoffset, 0, 0);
			endobjectchange((INTBIG)ni, VNODEINST);
			adjustednodes++;

			/* restore arc constraints */
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			{
				ai = pi->conarcinst;
				ai->userbits = ai->temp1;
			}
		}
	}
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (list[i]->entryisnode) continue;
		ai = list[i]->entryaddr.ai;
		end1xoff = us_alignvalueround(ai->end[0].xpos) - ai->end[0].xpos;
		end1yoff = us_alignvalueround(ai->end[0].ypos) - ai->end[0].ypos;
		end2xoff = us_alignvalueround(ai->end[1].xpos) - ai->end[1].xpos;
		end2yoff = us_alignvalueround(ai->end[1].ypos) - ai->end[1].ypos;
		if (end1xoff == 0 && end2xoff == 0 && end1yoff == 0 && end2yoff == 0) continue;

		if (!db_stillinport(ai, 0, ai->end[0].xpos+end1xoff, ai->end[0].ypos+end1yoff))
		{
			if (!db_stillinport(ai, 0, ai->end[0].xpos, ai->end[0].ypos)) continue;
			end1xoff = end1yoff = 0;
		}
		if (!db_stillinport(ai, 1, ai->end[1].xpos+end2xoff, ai->end[1].ypos+end2yoff))
		{
			if (!db_stillinport(ai, 1, ai->end[1].xpos, ai->end[1].ypos)) continue;
			end2xoff = end2yoff = 0;
		}

		/* make sure an arc does not change angle */
		ang = (ai->userbits&AANGLE) >> AANGLESH;
		if (ang == 0 || ang == 180)
		{
			/* horizontal arc: both DY values must be the same */
			if (end1yoff != end2yoff) end1yoff = end2yoff = 0;
		} else if (ang == 90 || ang == 270)
		{
			/* vertical arc: both DX values must be the same */
			if (end1xoff != end2xoff) end1xoff = end2xoff = 0;
		}
		if (end1xoff != 0 || end2xoff != 0 || end1yoff != 0 || end2yoff != 0)
		{
			ai->temp1 = ai->userbits;
			ai->userbits &= ~(FIXED|FIXANG);
			startobjectchange((INTBIG)ai, VARCINST);
			modifyarcinst(ai, 0, end1xoff, end1yoff, end2xoff, end2yoff);
			endobjectchange((INTBIG)ai, VARCINST);
			adjustedarcs++;
			ai->userbits = ai->temp1;
		}
	}

	/* restore highlighting and show results */
	us_pophighlight(TRUE);
	if (adjustednodes == 0 && adjustedarcs == 0) ttyputmsg(_("No adjustments necessary")); else
		ttyputmsg(_("Adjusted %ld %s and %ld %s"),
			adjustednodes, makeplural(_("node"), adjustednodes),
				adjustedarcs, makeplural(_("arc"), adjustedarcs));
}

/*
 * Routine to create or modify implant (well/substrate) nodes that cover their components
 * cleanly.
 */
void us_coverimplant(void)
{
	COVERRECT *crlist;
	REGISTER COVERRECT *cr, *ocr, *lastcr, *nextcr;
	REGISTER NODEPROTO *np, *cell;
	REGISTER INTBIG i, total, domerge;
	REGISTER INTBIG fun, count;
	REGISTER NODEINST *ni, *nextni;
	REGISTER ARCINST *ai;
	HIGHLIGHT high;
	XARRAY trans;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* generate an initial coverage list from all nodes and arcs in the cell */
	crlist = NOCOVERRECT;
	cell = us_needcell();
	if (cell == NONODEPROTO) return;
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = nextni)
	{
		nextni = ni->nextnodeinst;
		if (ni->proto->primindex == 0) continue;
		fun = nodefunction(ni);
		if (fun == NPNODE)
		{
			startobjectchange((INTBIG)ni, VNODEINST);
			(void)killnodeinst(ni);
			continue;
		}
		makerot(ni, trans);
		total = nodepolys(ni, 0, NOWINDOWPART);
		for(i=0; i<total; i++)
		{
			shapenodepoly(ni, i, poly);
			fun = layerfunction(ni->proto->tech, poly->layer) & LFTYPE;
			if (fun != LFIMPLANT && fun != LFSUBSTRATE && fun != LFWELL) continue;
			xformpoly(poly, trans);
			us_addtocoverlist(poly, &crlist);
		}
	}
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		total = arcpolys(ai, NOWINDOWPART);
		for(i=0; i<total; i++)
		{
			shapearcpoly(ai, i, poly);
			fun = layerfunction(ai->proto->tech, poly->layer) & LFTYPE;
			if (fun != LFIMPLANT && fun != LFSUBSTRATE && fun != LFWELL) continue;
			us_addtocoverlist(poly, &crlist);
		}
	}

	/* merge the coverage rectangles that touch */
	for(cr = crlist; cr != NOCOVERRECT; cr = cr->nextcoverrect)
	{
		lastcr = NOCOVERRECT;
		for(ocr = cr->nextcoverrect; ocr != NOCOVERRECT; ocr = nextcr)
		{
			nextcr = ocr->nextcoverrect;
			domerge = 0;
			if (cr->layer == ocr->layer && cr->tech == ocr->tech)
			{
				if (cr->hx >= ocr->lx && cr->lx <= ocr->hx && 
					cr->hy >= ocr->ly && cr->ly <= ocr->hy) domerge = 1;
			}
			if (domerge != 0)
			{
				/* merge them */
				if (ocr->lx < cr->lx) cr->lx = ocr->lx;
				if (ocr->hx > cr->hx) cr->hx = ocr->hx;
				if (ocr->ly < cr->ly) cr->ly = ocr->ly;
				if (ocr->hy > cr->hy) cr->hy = ocr->hy;
				cr->contributions += ocr->contributions;

				if (lastcr == NOCOVERRECT) cr->nextcoverrect = ocr->nextcoverrect; else
					lastcr->nextcoverrect = ocr->nextcoverrect;
				efree((CHAR *)ocr);
				continue;
			}
			lastcr = ocr;
		}
	}
#if 0		/* merge coverage rectangles across open space */
	for(cr = crlist; cr != NOCOVERRECT; cr = cr->nextcoverrect)
	{
		lastcr = NOCOVERRECT;
		for(ocr = cr->nextcoverrect; ocr != NOCOVERRECT; ocr = nextcr)
		{
			nextcr = ocr->nextcoverrect;
			domerge = 0;
			if (cr->layer == ocr->layer && cr->tech == ocr->tech)
			{
				REGISTER COVERRECT *icr;
				REGISTER INTBIG mlx, mhx, mly, mhy;

				mlx = mini(cr->lx, ocr->lx);
				mhx = maxi(cr->hx, ocr->hx);
				mly = mini(cr->ly, ocr->ly);
				mhy = maxi(cr->hy, ocr->hy);
				for(icr = crlist; icr != NOCOVERRECT; icr = icr->nextcoverrect)
				{
					if (icr == cr || icr == ocr) continue;
					if (icr->layer == cr->layer && icr->tech == cr->tech) continue;
					if (icr->lx < mhx && icr->hx > mlx && icr->ly < mhy && icr->hy > mly)
						break;
				}
				if (icr == NOCOVERRECT) domerge = 1;
			}
			if (domerge != 0)
			{
				/* merge them */
				if (ocr->lx < cr->lx) cr->lx = ocr->lx;
				if (ocr->hx > cr->hx) cr->hx = ocr->hx;
				if (ocr->ly < cr->ly) cr->ly = ocr->ly;
				if (ocr->hy > cr->hy) cr->hy = ocr->hy;
				cr->contributions += ocr->contributions;

				if (lastcr == NOCOVERRECT) cr->nextcoverrect = ocr->nextcoverrect; else
					lastcr->nextcoverrect = ocr->nextcoverrect;
				efree((CHAR *)ocr);
				continue;
			}
			lastcr = ocr;
		}
	}
#endif

	count = 0;
	while (crlist != NOCOVERRECT)
	{
		cr = crlist;
		crlist = crlist->nextcoverrect;

		if (cr->contributions > 1)
		{
			np = getpurelayernode(cr->tech, cr->layer, 0);
			ni = newnodeinst(np, cr->lx, cr->hx, cr->ly, cr->hy, 0, 0, cell);
			if (ni == NONODEINST) continue;
			ni->userbits |= HARDSELECTN;
			endobjectchange((INTBIG)ni, VNODEINST);
			if (count == 0) us_clearhighlightcount();
			high.status = HIGHFROM;
			high.fromgeom = ni->geom;
			high.fromport = NOPORTPROTO;
			high.cell = cell;
			us_addhighlight(&high);
			count++;
		}
		efree((CHAR *)cr);
	}
	if (count == 0) ttyputmsg(_("No implant areas added")); else
		ttyputmsg(_("Added %ld implant %s"), count, makeplural(_("area"), count));
}

void us_addtocoverlist(POLYGON *poly, COVERRECT **crlist)
{
	REGISTER COVERRECT *cr;
	INTBIG lx, hx, ly, hy;

	getbbox(poly, &lx, &hx, &ly, &hy);
	for(cr = *crlist; cr != NOCOVERRECT; cr = cr->nextcoverrect)
	{
		if (cr->layer != poly->layer) continue;
		if (cr->tech != poly->tech) continue;
		if (hx < cr->lx || lx > cr->hx || hy < cr->ly || ly > cr->hy) continue;

		/* this polygon touches another: merge into it */
		if (lx < cr->lx) cr->lx = lx;
		if (hx > cr->hx) cr->hx = hx;
		if (ly < cr->ly) cr->ly = ly;
		if (hy > cr->hy) cr->hy = hy;
		cr->contributions++;
		return;
	}

	/* nothing in this area: create a new one */
	cr = (COVERRECT *)emalloc(sizeof (COVERRECT), el_tempcluster);
	if (cr == 0) return;
	cr->lx = lx;   cr->hx = hx;
	cr->ly = ly;   cr->hy = hy;
	cr->layer = poly->layer;
	cr->tech = poly->tech;
	cr->contributions = 1;
	cr->nextcoverrect = *crlist;
	*crlist = cr;
}

/*
 * Routine to round "value" to the nearest "us_alignment_ratio" units.
 */
INTBIG us_alignvalueround(INTBIG value)
{
	REGISTER INTBIG alignment;

	alignment = muldiv(us_alignment_ratio, el_curlib->lambda[el_curtech->techindex], WHOLE);
	if (alignment == 0) return(value);
	if (value > 0)
		return((value + alignment/2) / alignment * alignment);
	return((value - alignment/2) / alignment * alignment);
}

/*
 * Routine to determine whether cell "np" has a cell center in it.  If so,
 * an error is displayed and the routine returns true.
 */
BOOLEAN us_alreadycellcenter(NODEPROTO *np)
{
	REGISTER NODEINST *ni;

	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		if (ni->proto == gen_cellcenterprim)
	{
		us_abortcommand(_("This cell already has a cell-center"));
		return(TRUE);
	}
	return(FALSE);
}

/*
 * Routine to return true if cell "cell" belongs in technology "tech".
 */
BOOLEAN us_cellfromtech(NODEPROTO *cell, TECHNOLOGY *tech)
{
	if (tech == sch_tech)
	{
		/* schematic: accept {ic}, {sch}, and {pN} */
		if (cell->cellview == el_iconview) return(TRUE);
		if (cell->cellview == el_schematicview) return(TRUE);
		if ((cell->cellview->viewstate&MULTIPAGEVIEW) != 0) return(TRUE);
	} else
	{
		/* not schematic: accept unknown, {lay} */
		if (cell->cellview == el_unknownview) return(TRUE);
		if (cell->cellview == el_layoutview) return(TRUE);
	}
	return(FALSE);
}

/*
 * Routine to return the type of node to create, given that it will be put
 * in "cell" and that the icon should be used if "getcontents" is true.
 * Gives an error and returns NONODEPROTO on error.
 */
NODEPROTO *us_nodetocreate(BOOLEAN getcontents, NODEPROTO *cell)
{
	REGISTER NODEPROTO *np, *rnp;
	CHAR *par[3];
	REGISTER void *infstr;

	np = us_curnodeproto;
	if (np == NONODEPROTO)
	{
		us_abortcommand(_("No selected node proto to create"));
		return(NONODEPROTO);
	}

	/* disallow placement of text cells */
	if (np->primindex == 0)
	{
		if ((np->cellview->viewstate&TEXTVIEW) != 0)
		{
			us_abortcommand(_("Cannot place an instance of a text-only cell"));
			return(NONODEPROTO);
		}
	}

	/* use the icon cell if one exists and contents wasn't requested */
	if (!getcontents)
	{
		if (np->primindex == 0 && (np->cellview == el_schematicview ||
			(np->cellview->viewstate&MULTIPAGEVIEW) != 0))
		{
			rnp = iconview(np);
			if (rnp != NONODEPROTO)
			{
				/* there is an icon: see if the user wants to use it */
				infstr = initinfstr();
				formatinfstr(infstr, _("You requested creation of schematic cell: %s.  "),
					describenodeproto(np));
				formatinfstr(infstr, _("Don't you really want to place its icon?"));
				if (us_yesnodlog(returninfstr(infstr), par) == 0) return(NONODEPROTO);
				if (namesame(par[0], x_("yes")) == 0) np = rnp;
			}
		}
	}

	/* create a node instance: check for recursion first */
	if (isachildof(cell, np))
	{
		us_abortcommand(_("Cannot create the node: it would be recursive"));
		return(NONODEPROTO);
	}

	/* disallow creation of second cell center */
	if (np == gen_cellcenterprim && us_alreadycellcenter(cell)) return(NONODEPROTO);
	return(np);
}

/*
 * recursive helper routine for "us_copycell" which copies cell "fromnp"
 * to a new cell called "toname" in library "tolib" with the new view type
 * "toview".  All needed subcells are copied (unless "nosubcells" is true).
 * All shared view cells referenced by variables are copied too
 * (unless "norelatedviews" is true).  If "useexisting" is TRUE, any subcells
 * that already exist in the destination library will be used there instead of
 * creating a cross-library reference.
 * If "move" is nonzero, delete the original after copying, and update all
 * references to point to the new cell.  If "subdescript" is empty, the operation
 * is a top-level request.  Otherwise, this is for a subcell, so only create a
 * new cell if one with the same name and date doesn't already exists.
 */
NODEPROTO *us_copyrecursively(NODEPROTO *fromnp, CHAR *toname, LIBRARY *tolib,
	VIEW *toview, BOOLEAN verbose, BOOLEAN move, CHAR *subdescript, BOOLEAN norelatedviews,
	BOOLEAN nosubcells, BOOLEAN useexisting)
{
	REGISTER NODEPROTO *np, *onp, *newfromnp, *beforenewfromnp, *fromnpwalk;
	REGISTER NODEINST *ni;
	REGISTER GEOM **list;
	REGISTER INTBIG i;
	REGISTER LIBRARY *lib;
	REGISTER BOOLEAN found;
	REGISTER CHAR *newname;
	REGISTER void *infstr;
	         UINTBIG fromnp_creationdate, fromnp_revisiondate;

	fromnp_creationdate = fromnp->creationdate;
	fromnp_revisiondate = fromnp->revisiondate;

	/* see if the cell is already there */
	for(newfromnp = tolib->firstnodeproto; newfromnp != NONODEPROTO;
		newfromnp = newfromnp->nextnodeproto)
	{
		if (namesame(newfromnp->protoname, toname) != 0) continue;
		if (newfromnp->cellview != toview) continue;
		if (newfromnp->creationdate != fromnp->creationdate) continue;
		if (newfromnp->revisiondate != fromnp->revisiondate) continue;
		if (*subdescript != 0) return(newfromnp);
		break;
	}

	/* copy subcells */
	if (!nosubcells)
	{
		found = TRUE;
		while (found)
		{
			found = FALSE;
			for(ni = fromnp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				np = ni->proto;
				if (np->primindex != 0) continue;

				/* allow cross-library references to stay */
				if (np->lib != fromnp->lib) continue;
				if (np->lib == tolib) continue;

				/* see if the cell is already there */
				if (us_indestlib(np, tolib)) continue;

				/* copy subcell if not already there */
				onp = us_copyrecursively(np, np->protoname, tolib, np->cellview,
					verbose, move, x_("subcell "), norelatedviews, nosubcells, useexisting);
				if (onp == NONODEPROTO)
				{
					newname = describenodeproto(np);
					if (move) ttyputerr(_("Move of subcell %s failed"), newname); else
						ttyputerr(_("Copy of subcell %s failed"), newname);
					return(NONODEPROTO);
				}
				found = TRUE;
				break;
			}
		}
	}

	/* also copy equivalent views */
	if (!norelatedviews)
	{
		/* first copy the icons */
		found = TRUE;
		fromnpwalk = fromnp;
		while (found)
		{
			found = FALSE;
			FOR_CELLGROUP(np, fromnpwalk)
			{
				if (np->cellview != el_iconview) continue;

				/* see if the cell is already there */
				if (us_indestlib(np, tolib)) continue;

				/* copy equivalent view if not already there */
				if (move) fromnpwalk = np->nextcellgrp; /* if np is moved (i.e. deleted), circular linked list is broken */
				onp = us_copyrecursively(np, np->protoname, tolib, np->cellview,
					verbose, move, _("alternate view "), TRUE, nosubcells, useexisting);
				if (onp == NONODEPROTO)
				{
					if (move) ttyputerr(_("Move of alternate view %s failed"), describenodeproto(np)); else
						ttyputerr(_("Copy of alternate view %s failed"), describenodeproto(np));
					return(NONODEPROTO);
				}
				found = TRUE;
				break;
			}
		}

		/* now copy the rest */
		found = TRUE;
		while (found)
		{
			found = FALSE;
			FOR_CELLGROUP(np, fromnpwalk)
			{
				if (np->cellview == el_iconview) continue;

				/* see if the cell is already there */
				if (us_indestlib(np, tolib)) continue;

				/* copy equivalent view if not already there */
				if (move) fromnpwalk = np->nextcellgrp; /* if np is moved (i.e. deleted), circular linked list is broken */
				onp = us_copyrecursively(np, np->protoname, tolib, np->cellview,
					verbose, move, _("alternate view "), TRUE, nosubcells, useexisting);
				if (onp == NONODEPROTO)
				{
					if (move) ttyputerr(_("Move of alternate view %s failed"), describenodeproto(np)); else
						ttyputerr(_("Copy of alternate view %s failed"), describenodeproto(np));
					return(NONODEPROTO);
				}
				found = TRUE;
				break;
			}
		}
	}

	/* see if the cell is NOW there */
	beforenewfromnp = NONODEPROTO;
	for(newfromnp = tolib->firstnodeproto; newfromnp != NONODEPROTO;
		newfromnp = newfromnp->nextnodeproto)
	{
		if (namesame(newfromnp->protoname, toname) != 0) continue;
		if (newfromnp->cellview != toview) continue;
		if (newfromnp->creationdate != fromnp_creationdate) continue;
		if (!move && newfromnp->revisiondate != fromnp_revisiondate) continue; /* moving icon of schematic changes schematic's revision date */
		if (*subdescript != 0) return(newfromnp);
		break;
	}
	if (beforenewfromnp == newfromnp || newfromnp == NONODEPROTO)
	{
		/* copy the cell */
		if (*toview->sviewname != 0)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, toname);
			addtoinfstr(infstr, '{');
			addstringtoinfstr(infstr, toview->sviewname);
			addtoinfstr(infstr, '}');
			newname = returninfstr(infstr);
		} else newname = toname;
		newfromnp = copynodeproto(fromnp, tolib, newname, useexisting);
		if (newfromnp == NONODEPROTO)
		{
			if (move)
			{
				ttyputerr(_("Move of %s%s failed"), subdescript,
					describenodeproto(fromnp));
			} else
			{
				ttyputerr(_("Copy of %s%s failed"), subdescript,
					describenodeproto(fromnp));
			}
			return(NONODEPROTO);
		}

		/* ensure that the copied cell is the right size */
		(*el_curconstraint->solve)(newfromnp);

		/* if moving, adjust pointers and kill original cell */
		if (move)
		{
			/* ensure that the copied cell is the right size */
			(*el_curconstraint->solve)(newfromnp);

			/* clear highlighting if the current node is being replaced */
			list = us_gethighlighted(WANTNODEINST, 0, 0);
			for(i=0; list[i] != NOGEOM; i++)
			{
				if (!list[i]->entryisnode) continue;
				ni = list[i]->entryaddr.ni;
				if (ni->proto == fromnp) break;
			}
			if (list[i] != NOGEOM) us_clearhighlightcount();

			/* now replace old instances with the moved one */
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			{
				for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				{
					found = TRUE;
					while (found)
					{
						found = FALSE;
						for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
						{
							if (ni->proto == fromnp)
							{
								if (replacenodeinst(ni, newfromnp, FALSE, FALSE) == NONODEINST)
									ttyputerr(_("Error moving node %s in cell %s"),
										describenodeinst(ni), describenodeproto(np));
								found = TRUE;
								break;
							}
						}
					}
				}
			}
			toolturnoff(net_tool, FALSE);
			us_dokillcell(fromnp);
			toolturnon(net_tool);
			fromnp = NONODEPROTO;
		}

		if (verbose)
		{
			if (el_curlib != tolib)
			{
				infstr = initinfstr();
				if (move)
				{
					formatinfstr(infstr, _("Moved %s%s:%s to library %s"), subdescript,
						el_curlib->libname, nldescribenodeproto(newfromnp), tolib->libname);
				} else
				{
					formatinfstr(infstr, _("Copied %s%s:%s to library %s"), subdescript,
						el_curlib->libname, nldescribenodeproto(newfromnp), tolib->libname);
				}
				ttyputmsg(x_("%s"), returninfstr(infstr));
			} else
			{
				ttyputmsg(_("Copied %s%s"), subdescript, describenodeproto(newfromnp));
			}
		}
	}
	return(newfromnp);
}

/*
 * Routine to return true if a cell like "np" exists in library "lib".
 */
BOOLEAN us_indestlib(NODEPROTO *np, LIBRARY *lib)
{
	REGISTER NODEPROTO *onp;

	for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
	{
		if (namesame(onp->protoname, np->protoname) != 0) continue;
		if (onp->cellview != np->cellview) continue;
		if (onp->creationdate != np->creationdate) continue;
		if (onp->revisiondate != np->revisiondate) continue;
		return(TRUE);
	}
	return(FALSE);
}

/*
 * Routine to determine whether the line from (fx,fy) to (tx,ty) intersects the
 * box defined from "lx<=X<=hx" and "ly<=Y<=hy".  If there is an intersection,
 * the center is placed in (intx,inty) and the routine returns true.
 */
BOOLEAN us_linethroughbox(INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty,
	INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy, INTBIG *intx, INTBIG *inty)
{
	INTBIG xint[4], yint[4], ix, iy;
	REGISTER INTBIG intcount;
	REGISTER BOOLEAN didin;

	intcount = 0;

	/* see which side has the intersection point */
	didin = segintersect(fx, fy, tx, ty, lx, ly, hx, ly, &ix, &iy);
	if (didin)
	{
		xint[intcount] = ix;
		yint[intcount] = iy;
		intcount++;
	}
	didin = segintersect(fx, fy, tx, ty, lx, hy, hx, hy, &ix, &iy);
	if (didin)
	{
		xint[intcount] = ix;
		yint[intcount] = iy;
		intcount++;
	}
	didin = segintersect(fx, fy, tx, ty, lx, ly, lx, hy, &ix, &iy);
	if (didin)
	{
		xint[intcount] = ix;
		yint[intcount] = iy;
		intcount++;
	}
	didin = segintersect(fx, fy, tx, ty, hx, ly, hx, hy, &ix, &iy);
	if (didin)
	{
		xint[intcount] = ix;
		yint[intcount] = iy;
		intcount++;
	}

	/* see if two points hit */
	if (intcount == 2)
	{
		*intx = (xint[0] + xint[1]) / 2;
		*inty = (yint[0] + yint[1]) / 2;
		return(TRUE);
	}
	return(FALSE);
}

/*
 * Routine to remove geometry in the selected area, shortening arcs that enter
 * from outside.
 */
void us_erasegeometry(NODEPROTO *np)
{
	INTBIG lx, hx, ly, hy, lx1, hx1, ly1, hy1, ix, iy, sx, sy;
	REGISTER INTBIG i, in0, in1, keepend, killend, keepx, keepy, killx, killy,
		clx, chx, cly, chy, cx, cy;
	REGISTER INTBIG ang, didin;
	REGISTER ARCINST *ai, *ai1, *ai2, *nextai;
	REGISTER NODEINST *ni, *nextni;
	REGISTER NODEPROTO *pin;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* disallow erasing if lock is on */
	if (us_cantedit(np, NONODEINST, TRUE)) return;

	np = us_getareabounds(&lx, &hx, &ly, &hy);
	if (np == NONODEPROTO)
	{
		us_abortcommand(_("Outline an area first"));
		return;
	}

	/* grid the area */
	gridalign(&lx, &ly, 1, np);
	gridalign(&hx, &hy, 1, np);

	/* all arcs that cross the area but have no endpoint inside need a pin inserted */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = nextai)
	{
		nextai = ai->nextarcinst;

		/* if an end is inside, ignore */
		if (ai->end[0].xpos > lx && ai->end[0].xpos < hx &&
			ai->end[0].ypos > ly && ai->end[0].ypos < hy) continue;
		if (ai->end[1].xpos > lx && ai->end[1].xpos < hx &&
			ai->end[1].ypos > ly && ai->end[1].ypos < hy) continue;

		/* if length is zero, ignore */
		if (ai->end[0].xpos == ai->end[1].xpos &&
			ai->end[0].ypos == ai->end[1].ypos) continue;

		/* if the arc doesn't intersect the area, ignore */
		if (!us_linethroughbox(ai->end[0].xpos, ai->end[0].ypos,
			ai->end[1].xpos, ai->end[1].ypos, lx, hx, ly, hy, &ix, &iy))
				continue;

		/* create a pin at this point */
		pin = getpinproto(ai->proto);
		defaultnodesize(pin, &sx, &sy);
		clx = ix - sx/2;
		chx = clx + sx;
		cly = iy - sy/2;
		chy = cly + sy;
		ni = newnodeinst(pin, clx, chx, cly, chy, 0, 0, np);
		if (ni == NONODEINST) continue;
		endobjectchange((INTBIG)ni, VNODEINST);
		ai1 = newarcinst(ai->proto, ai->width, ai->userbits, ai->end[0].nodeinst,
			ai->end[0].portarcinst->proto, ai->end[0].xpos, ai->end[0].ypos,
			ni, ni->proto->firstportproto, ix, iy, np);
		if (ai1 == NOARCINST) continue;
		(void)copyvars((INTBIG)ai, VARCINST, (INTBIG)ai1, VARCINST, FALSE);
		endobjectchange((INTBIG)ai1, VARCINST);
		ai2 = newarcinst(ai->proto, ai->width, ai->userbits, ai->end[1].nodeinst,
			ai->end[1].portarcinst->proto, ai->end[1].xpos, ai->end[1].ypos,
			ni, ni->proto->firstportproto, ix, iy, np);
		if (ai2 == NOARCINST) continue;
		(void)copyvars((INTBIG)ai, VARCINST, (INTBIG)ai2, VARCINST, FALSE);
		endobjectchange((INTBIG)ai1, VARCINST);
		startobjectchange((INTBIG)ai, VARCINST);
		(void)killarcinst(ai);
	}

	/* mark all nodes as "able to be deleted" */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		ni->temp1 = 0;

	/* truncate all arcs that cross the area */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = nextai)
	{
		nextai = ai->nextarcinst;

		/* get the extent of this arc */
		i = ai->width - arcwidthoffset(ai);
		if (curvedarcoutline(ai, poly, CLOSED, i))
			makearcpoly(ai->length, i, ai, poly, FILLED);
		getbbox(poly, &lx1, &hx1, &ly1, &hy1);

		/* if the arc is outside of the area, ignore it */
		if (lx1 >= hx || hx1 <= lx || ly1 >= hy || hy1 <= ly) continue;

		/* if the arc is inside of the area, delete it */
		if (lx1 >= lx && hx1 <= hx && ly1 >= ly && hy1 <= hy)
		{
			/* delete the arc */
			startobjectchange((INTBIG)ai, VARCINST);
			if (killarcinst(ai)) ttyputerr(_("Error killing arc"));
			continue;
		}

		/* partially inside the area: truncate the arc */
		if (ai->end[0].xpos > lx && ai->end[0].xpos < hx &&
			ai->end[0].ypos > ly && ai->end[0].ypos < hy) in0 = 1; else
				in0 = 0;
		if (ai->end[1].xpos > lx && ai->end[1].xpos < hx &&
			ai->end[1].ypos > ly && ai->end[1].ypos < hy) in1 = 1; else
				in1 = 0;
		if (in0 == in1) continue;
		if (in0 == 0) keepend = 0; else keepend = 1;
		killend = 1 - keepend;
		keepx = ai->end[keepend].xpos;   keepy = ai->end[keepend].ypos;
		killx = ai->end[killend].xpos;   killy = ai->end[killend].ypos;
		clx = lx - i/2;
		chx = hx + i/2;
		cly = ly - i/2;
		chy = hy + i/2;
		ang = figureangle(keepx, keepy, killx, killy);

		/* see which side has the intersection point */
		didin = intersect(keepx, keepy, ang, lx, hy, 0, &ix, &iy);
		if (didin >= 0)
		{
			if (ix >= lx && ix <= hx &&
				ix >= mini(keepx, killx) &&
				ix <= maxi(keepx, killx) &&
				iy >= mini(keepy, killy) &&
				iy <= maxi(keepy, killy))
			{
				/* intersects the top edge */
				(void)intersect(keepx, keepy, ang, clx, chy, 0, &ix, &iy);
				us_choparc(ai, keepend, ix, iy);
				continue;
			}
		}
		didin = intersect(keepx, keepy, ang, lx, ly, 0, &ix, &iy);
		if (didin >= 0)
		{
			if (ix >= lx && ix <= hx &&
				ix >= mini(keepx, killx) &&
				ix <= maxi(keepx, killx) &&
				iy >= mini(keepy, killy) &&
				iy <= maxi(keepy, killy))
			{
				/* intersects the bottom edge */
				(void)intersect(keepx, keepy, ang, clx, cly, 0, &ix, &iy);
				us_choparc(ai, keepend, ix, iy);
				continue;
			}
		}
		didin = intersect(keepx, keepy, ang, lx, ly, 900, &ix, &iy);
		if (didin >= 0)
		{
			if (iy >= ly && iy <= hy &&
				ix >= mini(keepx, killx) &&
				ix <= maxi(keepx, killx) &&
				iy >= mini(keepy, killy) &&
				iy <= maxi(keepy, killy))
			{
				/* intersects the bottom edge */
				(void)intersect(keepx, keepy, ang, clx, cly, 900, &ix, &iy);
				us_choparc(ai, keepend, ix, iy);
				continue;
			}
		}
		didin = intersect(keepx, keepy, ang, hx, ly, 900, &ix, &iy);
		if (didin >= 0)
		{
			if (iy >= ly && iy <= hy &&
				ix >= mini(keepx, killx) &&
				ix <= maxi(keepx, killx) &&
				iy >= mini(keepy, killy) &&
				iy <= maxi(keepy, killy))
			{
				/* intersects the bottom edge */
				(void)intersect(keepx, keepy, ang, chx, cly, 900, &ix, &iy);
				us_choparc(ai, keepend, ix, iy);
				continue;
			}
		}
	}

	/* now remove nodes in the area */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = nextni)
	{
		nextni = ni->nextnodeinst;
		if (ni->temp1 != 0) continue;

		/* if the node is outside of the area, ignore it */
		cx = (ni->lowx + ni->highx) / 2;
		cy = (ni->lowy + ni->highy) / 2;
		if (cx > hx || cx < lx || cy > hy || cy < ly) continue;

		/* if it cannot be modified, stop */
		if (us_cantedit(np, ni, TRUE)) continue;

		/* delete the node */
		while(ni->firstportexpinst != NOPORTEXPINST)
			(void)killportproto(np, ni->firstportexpinst->exportproto);
		startobjectchange((INTBIG)ni, VNODEINST);
		if (killnodeinst(ni)) ttyputerr(_("Error killing node"));
	}
}

void us_choparc(ARCINST *ai, INTBIG keepend, INTBIG ix, INTBIG iy)
{
	REGISTER NODEPROTO *pin;
	REGISTER NODEINST *ni, *oldni;
	REGISTER ARCINST *newai;
	REGISTER PORTPROTO *oldpp;
	REGISTER ARCPROTO *ap;
	REGISTER INTBIG size, lx, hx, ly, hy, wid, bits, oldx, oldy;

	ap = ai->proto;
	pin = getpinproto(ap);
	if (pin == NONODEPROTO) return;
	wid = ai->width;
	bits = ai->userbits;
	if (pin->tech == sch_tech)
	{
		size = pin->highx - pin->lowx;
	} else
	{
		size = wid - arcwidthoffset(ai);
	}
	lx = ix - size/2;   hx = lx + size;
	ly = iy - size/2;   hy = ly + size;
	ni = newnodeinst(pin, lx, hx, ly, hy, 0, 0, ai->parent);
	if (ni == NONODEINST) return;
	ni->temp1 = 1;
	endobjectchange((INTBIG)ni, VNODEINST);
	oldni = ai->end[keepend].nodeinst;
	oldpp = ai->end[keepend].portarcinst->proto;
	oldx = ai->end[keepend].xpos;
	oldy = ai->end[keepend].ypos;
	newai = newarcinst(ap, wid, bits, oldni, oldpp, oldx, oldy, ni, ni->proto->firstportproto,
		ix, iy, ai->parent);
	if (newai == NOARCINST) return;
		(void)copyvars((INTBIG)ai, VARCINST, (INTBIG)newai, VARCINST, FALSE);
	endobjectchange((INTBIG)newai, VARCINST);
	startobjectchange((INTBIG)ai, VARCINST);
	if (killarcinst(ai))
		ttyputerr(_("Error killing arc"));
}

/*
 * routine to clean-up cell "np" as follows:
 *   remove stranded pins
 *   collapse redundant arcs
 *   highlight zero-size nodes
 *   removes duplicate arcs
 *   highlight oversize pins that allow arcs to connect without touching
 *   move unattached and invisible pins with text in a different location
 *   resize oversized pins that don't have oversized arcs on them
 * Returns TRUE if changes are made.
 */
BOOLEAN us_cleanupcell(NODEPROTO *np, BOOLEAN justthis)
{
	REGISTER NODEINST *ni, *nextni;
	REGISTER PORTARCINST *pi, *opi;
	REGISTER INTBIG i, pinsremoved, pinsscaled, zerosize, negsize, sx, sy, oversizearc,
		oversize, oversizex, oversizey, oversizepin, textmoved, dlx, dhx, dly, dhy,
		otherend, ootherend, duparcs;
	ARCINST *ai, *oai, **thearcs;
	BOOLEAN spoke;
	REGISTER VARIABLE *var;
	INTBIG lx, hx, ly, hy, x, y;
	HIGHLIGHT newhigh;
	static POLYGON *poly = NOPOLYGON, *opoly = NOPOLYGON;
	REGISTER void *infstr;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);
	(void)needstaticpolygon(&opoly, 4, us_tool->cluster);

	/* look for unused pins that can be deleted */
	pinsremoved = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = nextni)
	{
		nextni = ni->nextnodeinst;
		if ((ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH != NPPIN) continue;

		/* if the pin is an export, save it */
		if (ni->firstportexpinst != NOPORTEXPINST) continue;

		/* if the pin is not connected or displayed, delete it */
		if (ni->firstportarcinst == NOPORTARCINST)
		{
			/* see if the pin has displayable variables on it */
			for(i=0; i<ni->numvar; i++)
			{
				var = &ni->firstvar[i];
				if ((var->type&VDISPLAY) != 0) break;
			}
			if (i >= ni->numvar)
			{
				/* disallow erasing if lock is on */
				if (us_cantedit(np, ni, TRUE)) continue;

				/* no displayable variables: delete it */
				startobjectchange((INTBIG)ni, VNODEINST);
				if (killnodeinst(ni)) ttyputerr(_("Error from killnodeinst"));
				pinsremoved++;
			}
			continue;
		}

		/* if the pin is connected to two arcs along the same slope, delete it */
		if (isinlinepin(ni, &thearcs))
		{
			/* remove the pin and reconnect the arcs */
			if (us_erasepassthru(ni, FALSE, &ai) >= 0)
				pinsremoved++;
		}
	}

	/* look for oversized pins that can be reduced in size */
	pinsscaled = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if ((ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH != NPPIN) continue;

		/* if the pin is standard size, leave it alone */
		oversizex = (ni->highx - ni->lowx) - (ni->proto->highx - ni->proto->lowx);
		if (oversizex < 0) oversizex = 0;
		oversizey = (ni->highy - ni->lowy) - (ni->proto->highy - ni->proto->lowy);
		if (oversizey < 0) oversizey = 0;
		if (oversizex == 0 && oversizey == 0) continue;

		/* all arcs must connect in the pin center */
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			ai = pi->conarcinst;
			if (ai->end[0].nodeinst == ni)
			{
				if (ai->end[0].xpos != (ni->lowx+ni->highx)/2) break;
				if (ai->end[0].ypos != (ni->lowy+ni->highy)/2) break;
			}
			if (ai->end[1].nodeinst == ni)
			{
				if (ai->end[1].xpos != (ni->lowx+ni->highx)/2) break;
				if (ai->end[1].ypos != (ni->lowy+ni->highy)/2) break;
			}
		}
		if (pi != NOPORTARCINST) continue;

		/* look for arcs that are oversized */
		oversizearc = 0;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			ai = pi->conarcinst;
			oversize = ai->width - ai->proto->nominalwidth;
			if (oversize < 0) oversize = 0;
			if (oversize > oversizearc) oversizearc = oversize;
		}

		/* if an arc covers the pin, leave the pin */
		if (oversizearc >= oversizex && oversizearc >= oversizey) continue;

		dlx = dhx = dly = dhy = 0;
		if (oversizearc < oversizex)
		{
			dlx =  (oversizex - oversizearc) / 2;
			dhx = -(oversizex - oversizearc) / 2;
		}
		if (oversizearc < oversizey)
		{
			dly =  (oversizey - oversizearc) / 2;
			dhy = -(oversizey - oversizearc) / 2;
		}
		startobjectchange((INTBIG)ni, VNODEINST);
		modifynodeinst(ni, dlx, dly, dhx, dhy, 0, 0);
		endobjectchange((INTBIG)ni, VNODEINST);

		pinsscaled++;
	}

	/* look for pins that are invisible and have text in different location */
	textmoved = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (invisiblepinwithoffsettext(ni, &x, &y, TRUE))
			textmoved++;
	}

	/* highlight oversize pins that allow arcs to connect without touching */
	oversizepin = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if ((ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH != NPPIN) continue;

		/* make sure all arcs touch each other */
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			ai = pi->conarcinst;
			i = ai->width - arcwidthoffset(ai);
			if (curvedarcoutline(ai, poly, CLOSED, i))
				makearcpoly(ai->length, i, ai, poly, FILLED);
			for(opi = pi->nextportarcinst; opi != NOPORTARCINST; opi = opi->nextportarcinst)
			{
				oai = opi->conarcinst;
				i = ai->width - arcwidthoffset(oai);
				if (curvedarcoutline(oai, opoly, CLOSED, i))
					makearcpoly(oai->length, i, oai, opoly, FILLED);
				if (polyseparation(poly, opoly) <= 0) continue;
				if (justthis)
				{
					newhigh.status = HIGHFROM;
					newhigh.cell = np;
					newhigh.fromgeom = ni->geom;
					newhigh.fromport = NOPORTPROTO;
					newhigh.frompoint = 0;
					us_addhighlight(&newhigh);
				}
				oversizepin++;
			}
		}
	}

	/* look for duplicate arcs */
	duparcs = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		for(;;)
		{
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			{
				ai = pi->conarcinst;
				if (ai->end[0].portarcinst == pi) otherend = 1; else otherend = 0;
				for(opi = pi->nextportarcinst; opi != NOPORTARCINST; opi = opi->nextportarcinst)
				{
					if (pi->proto != opi->proto) continue;
					oai = opi->conarcinst;
					if (ai->proto != oai->proto) continue;
					if (oai->end[0].portarcinst == opi) ootherend = 1; else ootherend = 0;
					if (ai->end[otherend].nodeinst != oai->end[ootherend].nodeinst) continue;
					if (ai->end[otherend].portarcinst->proto != oai->end[ootherend].portarcinst->proto)
						continue;

					/* this arc is a duplicate */
					startobjectchange((INTBIG)oai, VARCINST);
					killarcinst(oai);
					duparcs++;
					break;
				}
				if (opi != NOPORTARCINST) break;
			}
			if (pi == NOPORTARCINST) break;
		}
	}

	/* now highlight negative or zero-size nodes */
	zerosize = negsize = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto == gen_cellcenterprim ||
			ni->proto == gen_invispinprim ||
			ni->proto == gen_essentialprim) continue;
		nodesizeoffset(ni, &lx, &ly, &hx, &hy);
		sx = ni->highx - ni->lowx - lx - hx;
		sy = ni->highy - ni->lowy - ly - hy;
		if (sx > 0 && sy > 0) continue;
		if (justthis)
		{
			newhigh.status = HIGHFROM;
			newhigh.cell = np;
			newhigh.fromgeom = ni->geom;
			newhigh.fromport = NOPORTPROTO;
			newhigh.frompoint = 0;
			us_addhighlight(&newhigh);
		}
		if (sx < 0 || sy < 0) negsize++; else
			zerosize++;
	}

	if (pinsremoved == 0 && pinsscaled == 0 && zerosize == 0 &&
		negsize == 0 && textmoved == 0 && oversizepin == 0 && duparcs == 0)
	{
		if (justthis) ttyputmsg(_("Nothing to clean"));
		return(FALSE);
	}

	/* report what was cleaned */
	infstr = initinfstr();
	if (!justthis)
	{
		formatinfstr(infstr, _("Cell %s:"), describenodeproto(np));
	}
	spoke = FALSE;
	if (pinsremoved != 0)
	{
		formatinfstr(infstr, _("Removed %ld %s"), pinsremoved, makeplural(_("pin"), pinsremoved));
		spoke = TRUE;
	}
	if (duparcs != 0)
	{
		if (spoke) addstringtoinfstr(infstr, x_("; "));
		formatinfstr(infstr, _("Removed %ld duplicate %s"), duparcs, makeplural(_("arc"), duparcs));
		spoke = TRUE;
	}
	if (pinsscaled != 0)
	{
		if (spoke) addstringtoinfstr(infstr, x_("; "));
		formatinfstr(infstr, _("Shrunk %ld %s"), pinsscaled, makeplural(_("pin"), pinsscaled));
		spoke = TRUE;
	}
	if (zerosize != 0)
	{
		if (spoke) addstringtoinfstr(infstr, x_("; "));
		if (justthis)
		{
			formatinfstr(infstr, _("Highlighted %ld zero-size %s"), zerosize,
				makeplural(_("node"), zerosize));
		} else
		{
			formatinfstr(infstr, _("Found %ld zero-size %s"), zerosize,
				makeplural(_("node"), zerosize));
		}
		spoke = TRUE;
	}
	if (negsize != 0)
	{
		if (spoke) addstringtoinfstr(infstr, x_("; "));
		if (justthis)
		{
			formatinfstr(infstr, _("Highlighted %ld negative-size %s"), negsize,
				makeplural(_("node"), negsize));
		} else
		{
			formatinfstr(infstr, _("Found %ld negative-size %s"), negsize,
				makeplural(_("node"), negsize));
		}
		spoke = TRUE;
	}
	if (oversizepin != 0)
	{
		if (spoke) addstringtoinfstr(infstr, x_("; "));
		if (justthis)
		{
			formatinfstr(infstr, _("Highlighted %ld oversize %s with arcs that don't touch"),
				oversizepin, makeplural(_("pin"), oversizepin));
		} else
		{
			formatinfstr(infstr, _("Found %ld oversize %s with arcs that don't touch"),
				oversizepin, makeplural(_("pin"), oversizepin));
		}
		spoke = TRUE;
	}
	if (textmoved != 0)
	{
		if (spoke) addstringtoinfstr(infstr, x_("; "));
		formatinfstr(infstr, _("Moved text on %ld %s with offset text"), textmoved,
			makeplural(_("pin"), textmoved));
	}
	ttyputmsg(x_("%s"), returninfstr(infstr));
	return(TRUE);
}

/*
 * Routine to skeletonize cell "np" and create a new one called "newname"
 * in library "newlib".  Proceeds quietly if "quiet" is true.  Returns the
 * skeleton cell (NONODEPROTO on error).
 */
NODEPROTO *us_skeletonize(NODEPROTO *np, CHAR *newname, LIBRARY *newlib, BOOLEAN quiet)
{
	REGISTER NODEPROTO *onp;
	REGISTER NODEINST *ni, *newni;
	REGISTER PORTPROTO *pp, *opp, *rpp, *npp;
	REGISTER ARCINST *ai;
	REGISTER INTBIG lowx, highx, lowy, highy, xc, yc, i;
	INTBIG newx, newy, px, py, opx, opy;
	REGISTER INTBIG newang, newtran;
	XARRAY trans, localtrans, ntrans;
	REGISTER void *infstr;

	/* cannot skeletonize text-only views */
	if ((np->cellview->viewstate&TEXTVIEW) != 0)
	{
		us_abortcommand(_("Cannot skeletonize textual views: only layout"));
		return(NONODEPROTO);
	}

	/* warn if skeletonizing nonlayout views */
	if (np->cellview != el_unknownview && np->cellview != el_layoutview)
		ttyputmsg(_("Warning: skeletonization only makes sense for layout cells, not %s"),
			np->cellview->viewname);

	onp = np;
	np = us_newnodeproto(newname, newlib);
	if (np == NONODEPROTO)
	{
		us_abortcommand(_("Cannot create %s"), newname);
		return(NONODEPROTO);
	}

	/* place all exports in the new cell */
	lowx = highx = (onp->lowx + onp->highx) / 2;
	lowy = highy = (onp->lowy + onp->highy) / 2;
	for(pp = onp->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		/* make a transformation matrix for the node that is an export */
		pp->temp1 = 0;
		ni = pp->subnodeinst;
		rpp = pp->subportproto;
		newang = ni->rotation;
		newtran = ni->transpose;
		makerot(ni, trans);
		while (ni->proto->primindex == 0)
		{
			maketrans(ni, localtrans);
			transmult(localtrans, trans, ntrans);
			ni = rpp->subnodeinst;
			rpp = rpp->subportproto;
			if (ni->transpose == 0) newang = ni->rotation + newang; else
				newang = ni->rotation + 3600 - newang;
			newtran = (newtran + ni->transpose) & 1;
			makerot(ni, localtrans);
			transmult(localtrans, ntrans, trans);
		}

		/* create this node */
		xc = (ni->lowx + ni->highx) / 2;   yc = (ni->lowy + ni->highy) / 2;
		xform(xc, yc, &newx, &newy, trans);
		newx -= (ni->highx - ni->lowx) / 2;
		newy -= (ni->highy - ni->lowy) / 2;
		newang = newang % 3600;   if (newang < 0) newang += 3600;
		newni = newnodeinst(ni->proto, newx, newx+ni->highx-ni->lowx,
			newy, newy+ni->highy-ni->lowy, newtran, newang, np);
		if (newni == NONODEINST)
		{
			us_abortcommand(_("Cannot create node in this cell"));
			return(NONODEPROTO);
		}
		endobjectchange((INTBIG)newni, VNODEINST);
		lowx = mini(lowx, newx);
		highx = maxi(highx, newx+ni->highx-ni->lowx);
		lowy = mini(lowy, newy);
		highy = maxi(highy, newy+ni->highy-ni->lowy);

		/* export the port from the node */
		npp = newportproto(np, newni, rpp, pp->protoname);
		if (npp == NOPORTPROTO)
		{
			us_abortcommand(_("Could not create port %s"), pp->protoname);
			return(NONODEPROTO);
		}
		npp->userbits = pp->userbits;
		TDCOPY(npp->textdescript, pp->textdescript);
		if (copyvars((INTBIG)pp, VPORTPROTO, (INTBIG)npp, VPORTPROTO, FALSE))
			return(NONODEPROTO);
		pp->temp1 = (INTBIG)newni;
		pp->temp2 = (INTBIG)rpp;
	}

	/* connect electrically-equivalent ports */
	for(pp = onp->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		for(opp = pp->nextportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
		{
			if (pp->network != opp->network) continue;
			if (pp->temp1 == 0 || opp->temp1 == 0) continue;
			portposition((NODEINST *)pp->temp1, (PORTPROTO *)pp->temp2, &px, &py);
			portposition((NODEINST *)opp->temp1, (PORTPROTO *)opp->temp2, &opx, &opy);
			ai = newarcinst(gen_universalarc, defaultarcwidth(gen_universalarc),
				us_makearcuserbits(gen_universalarc),
					(NODEINST *)pp->temp1, (PORTPROTO *)pp->temp2, px, py,
						(NODEINST *)opp->temp1, (PORTPROTO *)opp->temp2, opx, opy, np);
			if (ai == NOARCINST)
			{
				us_abortcommand(_("Cannot create connecting arc in this cell"));
				return(NONODEPROTO);
			}
			endobjectchange((INTBIG)ai, VARCINST);
		}
	}

	/* copy the cell center and essential-bounds nodes if they exist */
	for(ni = onp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto != gen_cellcenterprim && ni->proto != gen_essentialprim) continue;
		newni = newnodeinst(ni->proto, ni->lowx, ni->highx,
			ni->lowy, ni->highy, ni->transpose, ni->rotation, np);
		if (newni == NONODEINST)
		{
			us_abortcommand(_("Cannot create node in this cell"));
			return(NONODEPROTO);
		}
		newni->userbits |= HARDSELECTN;
		if (ni->proto == gen_cellcenterprim) newni->userbits |= NVISIBLEINSIDE;
		endobjectchange((INTBIG)newni, VNODEINST);
		lowx = mini(lowx, ni->lowx);
		highx = maxi(highx, ni->highx);
		lowy = mini(lowy, ni->lowy);
		highy = maxi(highy, ni->highy);
		break;
	}

	/* make sure cell is the same size */
	if (lowx > onp->lowx)
	{
		i = (onp->highy+onp->lowy)/2 - (gen_invispinprim->highy-gen_invispinprim->lowy)/2;
		(void)newnodeinst(gen_invispinprim, onp->lowx, onp->lowx+gen_invispinprim->highx-gen_invispinprim->lowx,
			i, i+gen_invispinprim->highy-gen_invispinprim->lowy, 0, 0, np);
	}
	if (highx < onp->highx)
	{
		i = (onp->highy+onp->lowy)/2 - (gen_invispinprim->highy-gen_invispinprim->lowy)/2;
		(void)newnodeinst(gen_invispinprim, onp->highx-(gen_invispinprim->highx-gen_invispinprim->lowx), onp->highx,
			i, i+gen_invispinprim->highy-gen_invispinprim->lowy, 0, 0, np);
	}
	if (lowy > onp->lowy)
	{
		i = (onp->highx+onp->lowx)/2 - (gen_invispinprim->highx-gen_invispinprim->lowx)/2;
		(void)newnodeinst(gen_invispinprim, i, i+gen_invispinprim->highx-gen_invispinprim->lowx,
			onp->lowy, onp->lowy+gen_invispinprim->highy-gen_invispinprim->lowy, 0, 0, np);
	}
	if (highy < onp->highy)
	{
		i = (onp->highx+onp->lowx)/2 - (gen_invispinprim->highx-gen_invispinprim->lowx)/2;
		(void)newnodeinst(gen_invispinprim, i, i+gen_invispinprim->highx-gen_invispinprim->lowx,
			onp->highy-(gen_invispinprim->highy-gen_invispinprim->lowy), onp->highy, 0, 0,np);
	}

	/* place the actual origin of the cell inside */
	infstr = initinfstr();
	addstringtoinfstr(infstr, onp->lib->libfile);
	addtoinfstr(infstr, ':');
	addstringtoinfstr(infstr, nldescribenodeproto(onp));
	(void)setval((INTBIG)np, VNODEPROTO, x_("FACET_original_facet"),
		(INTBIG)returninfstr(infstr), VSTRING);

	if (!quiet)
		ttyputmsg(_("Cell %s created with a skeletal representation of %s"),
			describenodeproto(np), describenodeproto(onp));
	return(np);
}

/*
 * routine to generate an icon in library "lib" with name "iconname" from the
 * port list in "fpp".  The icon cell is called "pt".  The icon cell is
 * returned (NONODEPROTO on error).
 */
NODEPROTO *us_makeiconcell(PORTPROTO *fpp, CHAR *iconname, CHAR *pt,
	LIBRARY *lib)
{
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *bbni, *pinni;
	REGISTER PORTPROTO *pp, **pplist;
	REGISTER LIBRARY *savelib;
	REGISTER INTBIG leftside, rightside, bottomside, topside, count, i, total,
		leadlength, leadspacing, xsize, ysize, xpos, ypos, xbbpos, ybbpos, spacing,
		style, index, lambda;
	REGISTER VARIABLE *var;
	extern COMCOMP us_noyesp;
	CHAR *result[2];
	REGISTER void *infstr;

	/* see if the icon already exists and issue a warning if so */
	savelib = el_curlib;   el_curlib = lib;
	np = getnodeproto(pt);
	el_curlib = savelib;
	if (np != NONODEPROTO)
	{
		infstr = initinfstr();
		formatinfstr(infstr, _("Warning: Icon %s already exists.  Create a new version?"), pt);
		i = ttygetparam(returninfstr(infstr), &us_noyesp, 1, result);
		if (i <= 0 || (result[0][0] != 'y' && result[0][0] != 'Y')) return(NONODEPROTO);
	}

	/* get icon style controls */
	var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_icon_style"));
	if (var != NOVARIABLE) style = var->addr; else style = ICONSTYLEDEFAULT;
	var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_icon_lead_length"));
	if (var != NOVARIABLE) leadlength = var->addr; else leadlength = ICONLEADDEFLEN;
	var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_icon_lead_spacing"));
	if (var != NOVARIABLE) leadspacing = var->addr; else leadspacing = ICONLEADDEFSEP;

	/* make a sorted list of exports */
	count = 0;
	for(pp = fpp; pp != NOPORTPROTO; pp = pp->nextportproto) count++;
	if (count != 0)
	{
		pplist = (PORTPROTO **)emalloc(count * (sizeof (PORTPROTO *)), el_tempcluster);
		if (pplist == 0) return(NONODEPROTO);
		count = 0;
		for(pp = fpp; pp != NOPORTPROTO; pp = pp->nextportproto) pplist[count++] = pp;

		/* sort the list by name */
		esort(pplist, count, sizeof (PORTPROTO *), sort_exportnameascending);

		/* reverse sort order if requested */
		if ((style&ICONSTYLEREVEXPORT) != 0)
		{
			for(i=0; i<count/2; i++)
			{
				index = count - i - 1;
				pp = pplist[i];   pplist[i] = pplist[index];   pplist[index] = pp;
			}
		}
	}

	/* get lambda */
	lambda = lib->lambda[sch_tech->techindex];

	/* create the new icon cell */
	np = newnodeproto(pt, lib);
	if (np == NONODEPROTO)
	{
		us_abortcommand(_("Cannot create icon %s"), pt);
		return(NONODEPROTO);
	}
	np->userbits |= WANTNEXPAND;

	/* determine number of inputs and outputs */
	leftside = rightside = bottomside = topside = 0;
	for(i=0; i<count; i++)
	{
		pp = pplist[i];
		if ((pp->userbits&BODYONLY) != 0) continue;
		index = us_iconposition(pp, style);
		switch (index)
		{
			case 0: pp->temp1 = leftside++;    break;
			case 1: pp->temp1 = rightside++;   break;
			case 2: pp->temp1 = topside++;     break;
			case 3: pp->temp1 = bottomside++;  break;
		}
	}

	/* determine the size of the "black box" core */
	ysize = maxi(maxi(leftside, rightside), 5) * leadspacing * lambda;
	xsize = maxi(maxi(topside, bottomside), 3) * leadspacing * lambda;

	/* create the "black box" */
	if ((style&ICONSTYLEDRAWNOBODY) != 0) bbni = NONODEINST; else
	{
		bbni = newnodeinst(art_boxprim, 0, xsize, 0, ysize, 0, 0, np);
		if (bbni == NONODEINST) return(NONODEPROTO);
		(void)setvalkey((INTBIG)bbni, VNODEINST, art_colorkey, RED, VINTEGER);

		/* put the original cell name on it */
		var = setvalkey((INTBIG)bbni, VNODEINST, sch_functionkey, (INTBIG)iconname, VSTRING|VDISPLAY);
		if (var != NOVARIABLE) defaulttextdescript(var->textdescript, bbni->geom);
	}

	/* create the Cell Center instance */
	if ((us_useroptions&CELLCENTERALWAYS) != 0)
	{
		pinni = newnodeinst(gen_cellcenterprim, 0, 0, 0, 0, 0, 0, np);
		if (pinni == NONODEINST) return(NONODEPROTO);
		pinni->userbits |= HARDSELECTN|NVISIBLEINSIDE;
	}

	/* place pins around the Black Box */
	total = 0;
	for(i=0; i<count; i++)
	{
		pp = pplist[i];
		if ((pp->userbits&BODYONLY) != 0) continue;

		/* determine location of the port */
		index = us_iconposition(pp, style);
		spacing = leadspacing * lambda;
		switch (index)
		{
			case 0:		/* left side */
				xpos = -leadlength * lambda;
				xbbpos = 0;
				if (leftside*2 < rightside) spacing = leadspacing * 2 * lambda;
				ybbpos = ypos = ysize - ((ysize - (leftside-1)*spacing) / 2 + pp->temp1 * spacing);
				break;
			case 1:		/* right side */
				xpos = xsize + leadlength * lambda;
				xbbpos = xsize;
				if (rightside*2 < leftside) spacing = leadspacing * 2 * lambda;
				ybbpos = ypos = ysize - ((ysize - (rightside-1)*spacing) / 2 + pp->temp1 * spacing);
				break;
			case 2:		/* top */
				if (topside*2 < bottomside) spacing = leadspacing * 2 * lambda;
				xbbpos = xpos = xsize - ((xsize - (topside-1)*spacing) / 2 + pp->temp1 * spacing);
				ypos = ysize + leadlength * lambda;
				ybbpos = ysize;
				break;
			case 3:		/* bottom */
				if (bottomside*2 < topside) spacing = leadspacing * 2 * lambda;
				xbbpos = xpos = xsize - ((xsize - (bottomside-1)*spacing) / 2 + pp->temp1 * spacing);
				ypos = -leadlength * lambda;
				ybbpos = 0;
				break;
		}

		if (us_makeiconexport(pp, style, index, xpos, ypos, xbbpos, ybbpos, np))
			total++;
	}

	/* if no body, leads, or cell center is drawn, and there is only 1 export, add more */
	if ((style&ICONSTYLEDRAWNOBODY) != 0 &&
		(style&ICONSTYLEDRAWNOLEADS) != 0 &&
		(us_useroptions&CELLCENTERALWAYS) == 0 &&
		total <= 1)
	{
		bbni = newnodeinst(gen_invispinprim, 0, xsize, 0, ysize, 0, 0, np);
		if (bbni == NONODEINST) return(NONODEPROTO);
	}

	if (count > 0) efree((CHAR *)pplist);
	(*el_curconstraint->solve)(np);
	return(np);
}

/*
 * Helper routine to create an export in icon "np".  The export is from original port "pp",
 * is on side "index" (0: left, 1: right, 2: top, 3: bottom), is at (xpos,ypos), and
 * connects to the central box at (xbbpos,ybbpos).  Returns TRUE if the export is created.
 * It uses icon style "style".
 */
BOOLEAN us_makeiconexport(PORTPROTO *pp, INTBIG style, INTBIG index,
	INTBIG xpos, INTBIG ypos, INTBIG xbbpos, INTBIG ybbpos, NODEPROTO *np)
{
	REGISTER NODEPROTO *pintype;
	REGISTER NODEINST *pinni, *ni;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *port, *bpp;
	REGISTER ARCPROTO *wiretype;
	REGISTER INTBIG xoffset, yoffset, pinsizex, pinsizey, lambda, wid, hei;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	lambda = el_curlib->lambda[sch_tech->techindex];

	/* determine type of pin and lead */
	switch ((style&ICONSTYLETECH) >> ICONSTYLETECHSH)
	{
		case 0:		/* generic */
			pintype = gen_invispinprim;
			pinsizex = pinsizey = 0;
			break;
		case 1:		/* schematic */
			pintype = sch_buspinprim;
			pinsizex = pintype->highx - pintype->lowx;
			pinsizey = pintype->highy - pintype->lowy;
			break;
	}
	wiretype = sch_wirearc;
	if (pp->subnodeinst != NONODEINST)
	{
		bpp = pp;
		while (bpp->subnodeinst->proto->primindex == 0) bpp = bpp->subportproto;
		if (bpp->subnodeinst->proto == sch_buspinprim)
			wiretype = sch_busarc;
	}

	/* if the export is on the body (no leads) then move it in */
	if ((style&ICONSTYLEDRAWNOLEADS) != 0)
	{
		xpos = xbbpos;   ypos = ybbpos;
		style &= ~ICONSTYLEPORTLOC;
	}

	/* make the pin with the port */
	pinni = newnodeinst(pintype, xpos-pinsizex/2, xpos+pinsizex/2,
		ypos-pinsizey/2, ypos+pinsizey/2, 0, 0, np);
	if (pinni == NONODEINST) return(FALSE);

	/* export the port that should be on this pin */
	port = newportproto(np, pinni, pintype->firstportproto, pp->protoname);
	if (port != NOPORTPROTO)
	{
		TDCOPY(descript, port->textdescript);
		switch ((style&ICONSTYLEPORTSTYLE) >> ICONSTYLEPORTSTYLESH)
		{
			case 0:		/* Centered */
				TDSETPOS(descript, VTPOSCENT);
				break;
			case 1:		/* Inward */
				switch (index)
				{
					case 0: TDSETPOS(descript, VTPOSRIGHT);  break;	/* left */
					case 1: TDSETPOS(descript, VTPOSLEFT);   break;	/* right */
					case 2: TDSETPOS(descript, VTPOSDOWN);   break;	/* top */
					case 3: TDSETPOS(descript, VTPOSUP);     break;	/* bottom */
				}
				break;
			case 2:		/* Outward */
				switch (index)
				{
					case 0: TDSETPOS(descript, VTPOSLEFT);   break;	/* left */
					case 1: TDSETPOS(descript, VTPOSRIGHT);  break;	/* right */
					case 2: TDSETPOS(descript, VTPOSUP);     break;	/* top */
					case 3: TDSETPOS(descript, VTPOSDOWN);   break;	/* bottom */
				}
				break;
		}
		switch ((style&ICONSTYLEPORTLOC) >> ICONSTYLEPORTLOCSH)
		{
			case 0:		/* port on body */
				xoffset = xbbpos - xpos;   yoffset = ybbpos - ypos;
				break;
			case 1:		/* port on lead end */
				xoffset = yoffset = 0;
				break;
			case 2:		/* port on lead middle */
				xoffset = (xpos+xbbpos) / 2 - xpos;
				yoffset = (ypos+ybbpos) / 2 - ypos;
				break;
		}
		TDSETOFF(descript, xoffset * 4 / lambda, yoffset * 4 / lambda);
		TDCOPY(port->textdescript, descript);
		port->userbits = (port->userbits & ~(STATEBITS|PORTDRAWN)) |
			(pp->userbits & (STATEBITS|PORTDRAWN));
		if (copyvars((INTBIG)pp, VPORTPROTO, (INTBIG)port, VPORTPROTO, FALSE))
			return(TRUE);
	}
	endobjectchange((INTBIG)pinni, VNODEINST);

	/* add lead if requested */
	if ((style&ICONSTYLEDRAWNOLEADS) == 0)
	{
		pintype = getpinproto(wiretype);
		wid = pintype->highx - pintype->lowx;
		hei = pintype->highy - pintype->lowy;
		ni = newnodeinst(pintype, xbbpos-wid/2, xbbpos+wid/2, ybbpos-hei/2, ybbpos+hei/2, 0, 0, np);
		if (ni != NONODEINST)
		{
			endobjectchange((INTBIG)ni, VNODEINST);
			ai = newarcinst(wiretype, defaultarcwidth(wiretype), us_makearcuserbits(wiretype),
				pinni, pinni->proto->firstportproto, xpos, ypos, ni, pintype->firstportproto,
					xbbpos, ybbpos, np);
			if (ai != NOARCINST) endobjectchange((INTBIG)ai, VARCINST);
		}
	}
	return(TRUE);
}

/*
 * Routine to determine the side of the icon that port "pp" belongs on, given that
 * the icon style is "style".
 */
INTBIG us_iconposition(PORTPROTO *pp, INTBIG style)
{
	REGISTER INTBIG index;
	REGISTER UINTBIG character;

	character = pp->userbits & STATEBITS;

	/* special detection for power and ground ports */
	if (portispower(pp)) character = PWRPORT;
	if (portisground(pp)) character = GNDPORT;

	/* see which side this type of port sits on */
	switch (character)
	{
		case INPORT:    index = (style & ICONSTYLESIDEIN) >> ICONSTYLESIDEINSH;         break;
		case OUTPORT:   index = (style & ICONSTYLESIDEOUT) >> ICONSTYLESIDEOUTSH;       break;
		case BIDIRPORT: index = (style & ICONSTYLESIDEBIDIR) >> ICONSTYLESIDEBIDIRSH;   break;
		case PWRPORT:   index = (style & ICONSTYLESIDEPOWER) >> ICONSTYLESIDEPOWERSH;   break;
		case GNDPORT:   index = (style & ICONSTYLESIDEGROUND) >> ICONSTYLESIDEGROUNDSH; break;
		case CLKPORT:
		case C1PORT:
		case C2PORT:
		case C3PORT:
		case C4PORT:
		case C5PORT:
		case C6PORT:
			index = (style & ICONSTYLESIDECLOCK) >> ICONSTYLESIDECLOCKSH;
			break;
		default:
			index = (style & ICONSTYLESIDEIN) >> ICONSTYLESIDEINSH;
			break;
	}
	return(index);
}

/*
 * Routine to return the name of the technology that is used in cell "np".
 * Distinction is made between analog and digital schematics.
 */
CHAR *us_techname(NODEPROTO *np)
{
	REGISTER TECHNOLOGY *tech;
	REGISTER NODEINST *ni;
	REGISTER CHAR *techname;

	if ((np->userbits&TECEDITCELL) != 0) return(x_("TECHEDIT"));
	tech = np->tech;
	if (tech == NOTECHNOLOGY) return(x_(""));
	techname = tech->techname;
	if (tech == sch_tech)
	{
		/* see if it is analog or digital */
		techname = x_("schematic, analog");
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if (ni->proto == sch_bufprim || ni->proto == sch_andprim ||
				ni->proto == sch_orprim || ni->proto == sch_xorprim ||
				ni->proto == sch_ffprim || ni->proto == sch_muxprim)
					return(x_("schematic, digital"));
		}
	}
	return(techname);
}

/*
 * Routine to delete cell "np".  Validity checks are assumed to be made (i.e. the
 * cell is not used and is not locked).
 */
void us_dokillcell(NODEPROTO *np)
{
	REGISTER NODEPROTO *curcell, *prevversion;
	REGISTER NODEINST *ni;
	REGISTER BOOLEAN iscurrent, killresult;
	REGISTER WINDOWPART *w, *nextw, *neww;

	/* delete random references to this cell */
	curcell = getcurcell();
	if (np == curcell)
	{
		(void)setval((INTBIG)el_curlib, VLIBRARY, x_("curnodeproto"), (INTBIG)NONODEPROTO, VNODEPROTO);
		us_clearhighlightcount();
	}

	/* close windows that reference this cell */
	for(w = el_topwindowpart; w != NOWINDOWPART; w = nextw)
	{
		nextw = w->nextwindowpart;
		if (w->curnodeproto != np) continue;
		if (w == el_curwindowpart) iscurrent = TRUE; else iscurrent = FALSE;
		if (iscurrent)
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)NOWINDOWPART,
				VWINDOWPART|VDONTSAVE);
		startobjectchange((INTBIG)us_tool, VTOOL);
		neww = newwindowpart(w->location, w);
		if (neww == NOWINDOWPART) return;

		/* adjust the new window to account for borders in the old one */
		if ((w->state&WINDOWTYPE) != DISPWINDOW)
		{
			neww->usehx -= DISPLAYSLIDERSIZE;
			neww->usely += DISPLAYSLIDERSIZE;
		}
		if ((w->state&WINDOWTYPE) == WAVEFORMWINDOW)
		{
			neww->uselx -= DISPLAYSLIDERSIZE;
			neww->usely -= DISPLAYSLIDERSIZE;
		}
		if ((w->state&WINDOWMODE) != 0)
		{
			neww->uselx -= WINDOWMODEBORDERSIZE;   neww->usehx += WINDOWMODEBORDERSIZE;
			neww->usely -= WINDOWMODEBORDERSIZE;   neww->usehy += WINDOWMODEBORDERSIZE;
		}

		neww->curnodeproto = NONODEPROTO;
		neww->buttonhandler = DEFAULTBUTTONHANDLER;
		neww->charhandler = DEFAULTCHARHANDLER;
		neww->changehandler = DEFAULTCHANGEHANDLER;
		neww->termhandler = DEFAULTTERMHANDLER;
		neww->redisphandler = DEFAULTREDISPHANDLER;
		neww->state = (neww->state & ~(WINDOWTYPE|WINDOWMODE)) | DISPWINDOW;
		killwindowpart(w);
		endobjectchange((INTBIG)us_tool, VTOOL);
		if (iscurrent)
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)neww,
				VWINDOWPART|VDONTSAVE);
	}

	prevversion = np->prevversion;
	toolturnoff(net_tool, FALSE);
	killresult = killnodeproto(np);
	toolturnon(net_tool);
	if (killresult)
	{
		ttyputerr(_("Error killing cell"));
		return;
	}

	/* see if this was the latest version of a cell */
	if (prevversion != NONODEPROTO)
	{
		/* newest version was deleted: rename next older version */
		for(ni = prevversion->firstinst; ni != NONODEINST; ni = ni->nextinst)
		{
			if ((ni->userbits&NEXPAND) != 0) continue;
			startobjectchange((INTBIG)ni, VNODEINST);
			endobjectchange((INTBIG)ni, VNODEINST);
		}
	}

	/* update status display if necessary */
	if (us_curnodeproto != NONODEPROTO && us_curnodeproto->primindex == 0)
	{
		if (np == us_curnodeproto)
		{
			if ((us_state&NONPERSISTENTCURNODE) != 0) us_setnodeproto(NONODEPROTO); else
				us_setnodeproto(el_curtech->firstnodeproto);
		}
	}
}

/*
 * Routine to compare the contents of two cells and return true if they are the same.
 * If "explain" is positive, tell why they differ.
 */
BOOLEAN us_samecontents(NODEPROTO *np1, NODEPROTO *np2, INTBIG explain)
{
	REGISTER NODEINST *ni1, *ni2;
	REGISTER GEOM *geom;
	REGISTER ARCINST *ai1, *ai2;
	REGISTER PORTPROTO *pp1, *pp2;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER INTBIG sea, cx, cy, i, lambda1, lambda2;

	/* make sure the nodes are the same */
	lambda1 = lambdaofcell(np1);
	lambda2 = lambdaofcell(np2);
	for(ni2 = np2->firstnodeinst; ni2 != NONODEINST; ni2 = ni2->nextnodeinst)
		ni2->temp1 = 0;
	for(ni1 = np1->firstnodeinst; ni1 != NONODEINST; ni1 = ni1->nextnodeinst)
	{
		/* find the node in the other cell */
		ni1->temp1 = 0;
		cx = (ni1->lowx + ni1->highx) / 2;
		cy = (ni1->lowy + ni1->highy) / 2;
		sea = initsearch(cx, cx, cy, cy, np2);
		for(;;)
		{
			geom = nextobject(sea);
			if (geom == NOGEOM) break;
			if (!geom->entryisnode) continue;
			ni2 = geom->entryaddr.ni;
			if (ni1->lowx != ni2->lowx || ni1->highx != ni2->highx || 
				ni1->lowy != ni2->lowy || ni1->highy != ni2->highy) continue;
			if (ni1->rotation != ni2->rotation || ni1->transpose != ni2->transpose) continue;
			if (ni1->proto->primindex != ni2->proto->primindex) continue;
			if (ni1->proto->primindex != 0)
			{
				/* make sure the two primitives are the same */
				if (ni1->proto != ni2->proto) continue;
			} else
			{
				/* make sure the two cells are the same */
				if (namesame(ni1->proto->protoname, ni2->proto->protoname) != 0)
					continue;
				if (ni1->proto->cellview != ni2->proto->cellview) continue;
			}

			/* the nodes match */
			ni1->temp1 = (INTBIG)ni2;
			ni2->temp1 = (INTBIG)ni1;
			termsearch(sea);
			break;
		}
		if (ni1->temp1 == 0)
		{
			if (explain > 0)
				ttyputmsg(_("No equivalent to node %s at (%s,%s) in cell %s"),
					describenodeinst(ni1), latoa((ni1->lowx+ni1->highx)/2, lambda1),
						latoa((ni1->lowy+ni1->highy)/2, lambda1), describenodeproto(np1));
			return(FALSE);
		}
	}
	for(ni2 = np2->firstnodeinst; ni2 != NONODEINST; ni2 = ni2->nextnodeinst)
	{
		if (ni2->temp1 != 0) continue;
		if (explain > 0)
			ttyputmsg(_("No equivalent to node %s at (%s,%s) in cell %s"),
				describenodeinst(ni2), latoa((ni2->lowx+ni2->highx)/2, lambda2),
					latoa((ni2->lowy+ni2->highy)/2, lambda2), describenodeproto(np2));
		return(FALSE);
	}

	/* all nodes match up, now check the arcs */
	for(ai2 = np2->firstarcinst; ai2 != NOARCINST; ai2 = ai2->nextarcinst)
		ai2->temp1 = 0;
	for(ai1 = np1->firstarcinst; ai1 != NOARCINST; ai1 = ai1->nextarcinst)
	{
		ai1->temp1 = 0;
		ni2 = (NODEINST *)ai1->end[0].nodeinst->temp1;
		for(pi = ni2->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			ai2 = pi->conarcinst;
			if (ai2->proto != ai1->proto) continue;
			if (ai2->width != ai1->width) continue;
			for(i=0; i<2; i++)
			{
				if (ai2->end[i].xpos != ai1->end[i].xpos) break;
				if (ai2->end[i].ypos != ai1->end[i].ypos) break;
			}
			if (i >= 2) break;
		}
		if (pi == NOPORTARCINST)
		{
			if (explain > 0)
				ttyputmsg(_("No equivalent to arc %s from (%s,%s) to (%s,%s) in cell %s"),
					describearcinst(ai1), latoa(ai1->end[0].xpos, lambda1), latoa(ai1->end[0].ypos, lambda1),
						latoa(ai1->end[1].xpos, lambda1), latoa(ai1->end[1].ypos, lambda1), describenodeproto(np1));
			return(FALSE);
		}
		ai1->temp1 = (INTBIG)ai2;
		ai2->temp1 = (INTBIG)ai1;
	}
	for(ai2 = np2->firstarcinst; ai2 != NOARCINST; ai2 = ai2->nextarcinst)
	{
		if (ai2->temp1 != 0) continue;
		if (explain > 0)
			ttyputmsg(_("No equivalent to arc %s from (%s,%s) to (%s,%s) in cell %s"),
				describearcinst(ai2), latoa(ai2->end[0].xpos, lambda2), latoa(ai2->end[0].ypos, lambda2),
					latoa(ai2->end[1].xpos, lambda2), latoa(ai2->end[1].ypos, lambda2), describenodeproto(np2));
		return(FALSE);
	}

	/* now match the ports */
	for(pp2 = np2->firstportproto; pp2 != NOPORTPROTO; pp2 = pp2->nextportproto)
		pp2->temp1 = 0;
	for(pp1 = np1->firstportproto; pp1 != NOPORTPROTO; pp1 = pp1->nextportproto)
	{
		pp1->temp1 = 0;
		ni2 = (NODEINST *)pp1->subnodeinst->temp1;
		for(pe = ni2->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		{
			pp2 = pe->exportproto;
			if (namesame(pp1->protoname, pp2->protoname) != 0) continue;
			break;
		}
		if (pe == NOPORTEXPINST)
		{
			if (explain > 0)
				ttyputmsg(_("No equivalent to port %s in cell %s"), pp1->protoname,
					describenodeproto(np1));
			return(FALSE);
		}
		pp1->temp1 = (INTBIG)pp2;
		pp2->temp1 = (INTBIG)pp1;
	}
	for(pp2 = np2->firstportproto; pp2 != NOPORTPROTO; pp2 = pp2->nextportproto)
	{
		if (pp2->temp1 != 0) continue;
		if (explain > 0)
			ttyputmsg(_("No equivalent to port %s in cell %s"), pp2->protoname,
				describenodeproto(np2));
		return(FALSE);
	}

	/* cells match! */
	return(TRUE);
}

/*
 * Routine to create cell "name" in library "lib".  Returns NONODEPROTO on error.
 * Also makes icons expanded and places cell centers if requested.
 */
NODEPROTO *us_newnodeproto(CHAR *name, LIBRARY *lib)
{
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;

	np = newnodeproto(name, lib);
	if (np == NONODEPROTO) return(NONODEPROTO);

	/* icon cells should always be expanded */
	if (np->cellview == el_iconview) np->userbits |= WANTNEXPAND;

	/* place a cell-center if requested */
	if ((us_useroptions&CELLCENTERALWAYS) != 0 &&
		(np->cellview->viewstate&TEXTVIEW) == 0)
	{
		ni = newnodeinst(gen_cellcenterprim, 0, 0, 0, 0, 0, 0, np);
		if (ni != NONODEINST)
		{
			endobjectchange((INTBIG)ni, VNODEINST);
			ni->userbits |= HARDSELECTN|NVISIBLEINSIDE;
		}
	}
	return(np);
}

/*********************************** EXPLORER WINDOWS ***********************************/

/*
 * Routine to free the former explorer structure and build a new one.
 */
void us_createexplorerstruct(WINDOWPART *w)
{
	REGISTER EXPWINDOW *ew;
	REGISTER WINDOWPART *ow;
	REGISTER INTBIG lx, hx, ly, hy;

	/* create an explorer-window object */
	ew = (EXPWINDOW *)emalloc(sizeof (EXPWINDOW), us_tool->cluster);
	if (ew == 0) return;

	/* store the explorerwindow structure in the window structure */
	w->expwindow = ew;

	/* build a new explorer structure */
	ew->nodeused = NOEXPLORERNODE;
	ew->stacksize = 0;
	ew->firstnode = NOEXPLORERNODE;
	ew->nodeselected = NOEXPLORERNODE;
	ew->firstline = 0;
	ew->firstchar = 0;
	us_buildexplorerstruct(w, ew);
	adviseofchanges(us_clearexplorererrors);

	/* set up the window object */
	startobjectchange((INTBIG)w, VWINDOWPART);
	(void)setval((INTBIG)w, VWINDOWPART, x_("buttonhandler"), (INTBIG)us_explorebuttonhandler,
		VADDRESS);
	(void)setval((INTBIG)w, VWINDOWPART, x_("charhandler"), (INTBIG)us_explorecharhandler,
		VADDRESS);
	(void)setval((INTBIG)w, VWINDOWPART, x_("redisphandler"), (INTBIG)us_exploreredisphandler,
		VADDRESS);
	(void)setval((INTBIG)w, VWINDOWPART, x_("state"),
		(w->state & ~(WINDOWTYPE|WINDOWMODE)) | EXPLORERWINDOW, VINTEGER);
	endobjectchange((INTBIG)w, VWINDOWPART);

	/* redisplay the other window's horizontal slider */
	for(ow = el_topwindowpart; ow != NOWINDOWPART; ow = ow->nextwindowpart)
	{
		if (ow->frame != w->frame) continue;
		if ((ow->state&WINDOWTYPE) != DISPWINDOW) continue;

		/* prepare a window in which to draw the thumbs */
		lx = ow->uselx;   hx = ow->usehx;
		ly = ow->usely;   hy = ow->usehy;
		ow->usehx += DISPLAYSLIDERSIZE;
		ow->usely -= DISPLAYSLIDERSIZE;
		us_drawhorizontalslider(ow, ly, lx, hx, 0);
		us_drawhorizontalsliderthumb(ow, ly, lx, hx, ow->thumblx, ow->thumbhx, 0);
		ow->usehx -= DISPLAYSLIDERSIZE;
		ow->usely += DISPLAYSLIDERSIZE;
	}
}

/*
 * Routine called to deallocate the explorer window structures in "w".
 * Called prior to destruction of the window.
 */
void us_deleteexplorerstruct(WINDOWPART *w)
{
	REGISTER EXPWINDOW *ew;
	REGISTER EXPLORERNODE *en;

	ew = (EXPWINDOW *)w->expwindow;
	if (ew == NOEXPWINDOW) return;

	/* free all existing explorer nodes (no longer necessary) */
	while (ew->nodeused != NOEXPLORERNODE)
	{
		en = ew->nodeused;
		ew->nodeused = en->nextexplorernode;
		us_freeexplorernode(en);
	}
}

/*
 * Routine to build an explorer structure from the current database.
 */
void us_buildexplorerstruct(WINDOWPART *w, EXPWINDOW *ew)
{
	REGISTER WINDOWPART *ww;

	/* see if there is a simulation window in the other half */
	for(ww = el_topwindowpart; ww != NOWINDOWPART; ww = ww->nextwindowpart)
	{
		if (ww == w) continue;
		if (ww->frame != w->frame) continue;
		if ((ww->state&WINDOWTYPE) == WAVEFORMWINDOW) break;
	}
	if (ww == NOWINDOWPART)
	{
		/* create an explorer node for the hierarchical view */
		ew->nodehierarchy = us_allocexplorernode(ew);
		if (ew->nodehierarchy == NOEXPLORERNODE) return;
		ew->nodehierarchy->flags = EXNODEPLACEHOLDER;
		ew->nodehierarchy->enodetype = EXNODETYPELABEL;
		ew->nodehierarchy->addr = EXLABELTYPEHIVIEW;
		us_addexplorernode(ew->nodehierarchy, &ew->firstnode, NOEXPLORERNODE);

		/* create an explorer node for the contents view */
		ew->nodecontents = us_allocexplorernode(ew);
		if (ew->nodecontents == NOEXPLORERNODE) return;
		ew->nodecontents->flags = EXNODEPLACEHOLDER;
		ew->nodecontents->enodetype = EXNODETYPELABEL;
		ew->nodecontents->addr = EXLABELTYPECONVIEW;
		us_addexplorernode(ew->nodecontents, &ew->firstnode, NOEXPLORERNODE);

		/* create an explorer node for the errors view */
		ew->nodeerrors = us_allocexplorernode(ew);
		if (ew->nodeerrors == NOEXPLORERNODE) return;
		ew->nodeerrors->flags = EXNODEPLACEHOLDER;
		ew->nodeerrors->enodetype = EXNODETYPELABEL;
		ew->nodeerrors->addr = EXLABELTYPEERRVIEW;
		us_addexplorernode(ew->nodeerrors, &ew->firstnode, NOEXPLORERNODE);

		ew->nodesimulation = 0;
	} else
	{
		/* create an explorer node for the simulation signal view */
		ew->nodesimulation = us_allocexplorernode(ew);
		if (ew->nodesimulation == NOEXPLORERNODE) return;
		ew->nodesimulation->flags = EXNODEPLACEHOLDER;
		ew->nodesimulation->enodetype = EXNODETYPELABEL;
		ew->nodesimulation->addr = EXLABELTYPESIMVIEW;
		us_addexplorernode(ew->nodesimulation, &ew->firstnode, NOEXPLORERNODE);

		ew->nodehierarchy = 0;
		ew->nodecontents = 0;
		ew->nodeerrors = 0;
	}
}

/*
 * Routine to build an error-explorer structure from the current database.
 */
void us_buildexplorersturcterrors(EXPWINDOW *ew, EXPLORERNODE *whichtree)
{
	REGISTER void *curerror, *eg;
	REGISTER INTBIG i, geomcount;
	REGISTER EXPLORERNODE *en, *suben;

	whichtree->flags &= ~EXNODEPLACEHOLDER;
	curerror = 0;
	for(;;)
	{
		curerror = getnexterror(curerror);
		if (curerror == 0) break;

		en = us_allocexplorernode(ew);
		if (en == NOEXPLORERNODE) break;
		en->enodetype = EXNODETYPEERROR;
		en->addr = (INTBIG)curerror;
		us_addexplorernode(en, &whichtree->subexplorernode, whichtree);

		/* add any geometry objects */
		geomcount = getnumerrorgeom(curerror);
		for(i=0; i<geomcount; i++)
		{
			eg = geterrorgeom(curerror, i);
			suben = us_allocexplorernode(ew);
			if (suben == NOEXPLORERNODE) return;
			suben->enodetype = EXNODETYPEERRORGEOM;
			suben->addr = (INTBIG)eg;
			us_addexplorernode(suben, &en->subexplorernode, en);
		}
	}
}

/*
 * Routine to clear the error-explorer information (because the error list has changed).
 */
void us_clearexplorererrors(void)
{
	REGISTER WINDOWPART *w;
	REGISTER EXPLORERNODE *en, *suben, *lasten;
	REGISTER EXPWINDOW *ew;

	/* see if there is an explorer window */
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		if ((w->state&WINDOWTYPE) != EXPLORERWINDOW) continue;
		ew = (EXPWINDOW *)w->expwindow;
		if ((ew->nodeerrors->flags&EXNODEPLACEHOLDER) == 0)
		{
			ew->nodeerrors->flags |= EXNODEPLACEHOLDER;
			ew->nodeerrors->flags &= ~EXNODEOPEN;

			/* remove all error entries */
			lasten = NOEXPLORERNODE;
			for(en = ew->nodeused; en != NOEXPLORERNODE; en = en->nextexplorernode)
			{
				if (en->enodetype == EXNODETYPEERROR || en->enodetype == EXNODETYPEERRORGEOM)
				{
					if (lasten == NOEXPLORERNODE) ew->nodeused = en->nextexplorernode; else
						lasten->nextexplorernode = en->nextexplorernode;
					continue;
				}
				lasten = en;
			}
			while (ew->nodeerrors->subexplorernode != NOEXPLORERNODE)
			{
				en = ew->nodeerrors->subexplorernode;
				ew->nodeerrors->subexplorernode = en->nextsubexplorernode;
				while (en->subexplorernode != NOEXPLORERNODE)
				{
					suben = en->subexplorernode;
					en->subexplorernode = suben->nextsubexplorernode;
					us_freeexplorernode(suben);
				}
				us_freeexplorernode(en);
			}
			ew->nodeselected = NOEXPLORERNODE;
		}
		us_exploreredisphandler(w);
	}
}

static EXPLORERNODE *us_explorertopsimnode;
static EXPWINDOW    *us_explorercursimwindow;


CHAR *us_explorersimnodename(void *ptr);
void *us_explorersimnewbranch(CHAR *branchname, void *parentnode);
void *us_explorersimfindbranch(CHAR *branchname, void *parentnode);
void *us_explorersimnewleaf(CHAR *leafname, void *parentnode);

/*
 * Routine to build a simulation-signal-explorer structure from the current database.
 */
void us_buildexplorersturctsimulation(WINDOWPART *w, EXPWINDOW *ew, EXPLORERNODE *whichtree)
{
	REGISTER WINDOWPART *ww;

	/* see if this explorer window shares a space with a waveform window */
	for(ww = el_topwindowpart; ww != NOWINDOWPART; ww = ww->nextwindowpart)
	{
		if (ww == w) continue;
		if (ww->frame != w->frame) continue;
		if ((ww->state&WINDOWTYPE) == WAVEFORMWINDOW) break;
	}
	if (ww == NOWINDOWPART) return;
	whichtree->flags &= ~EXNODEPLACEHOLDER;
	us_explorertopsimnode = whichtree;
	us_explorercursimwindow = ew;
	sim_reportsignals(ww, us_explorersimnewbranch, us_explorersimfindbranch,
		us_explorersimnewleaf, us_explorersimnodename);
}

/*
 * Callback routine (called from "us_buildexplorersturctsimulation()") to return
 * the name of simulation-explorer node "ptr".
 */
CHAR *us_explorersimnodename(void *ptr)
{
	REGISTER EXPLORERNODE *en;

	en = (EXPLORERNODE *)ptr;
	return((CHAR *)en->addr);
}

/*
 * Callback routine (called from "us_buildexplorersturctsimulation()") to return
 * the branch of "parentnode" named "branchname".  Returns 0 if not found.
 */
void *us_explorersimfindbranch(CHAR *branchname, void *parentnode)
{
	REGISTER EXPLORERNODE *en, *suben;

	en = (EXPLORERNODE *)parentnode;
	if (en == 0) en = us_explorertopsimnode;
	for(suben = en->subexplorernode; suben != NOEXPLORERNODE; suben = suben->nextsubexplorernode)
		if (namesame(branchname, (CHAR *)suben->addr) == 0) return(suben);
	return(0);
}

/*
 * Callback routine (called from "us_buildexplorersturctsimulation()") to create
 * a new branch (under "parentnode") called "branchname".  Returns the branch node.
 */
void *us_explorersimnewbranch(CHAR *branchname, void *parentnode)
{
	REGISTER EXPLORERNODE *en, *suben;
	CHAR *pt;

	en = (EXPLORERNODE *)parentnode;
	if (en == 0) en = us_explorertopsimnode; else
		en->enodetype = EXNODETYPESIMBRANCH;

	suben = us_allocexplorernode(us_explorercursimwindow);
	if (suben == NOEXPLORERNODE) return(0);
	suben->enodetype = EXNODETYPESIMBRANCH;
	allocstring(&pt, branchname, us_tool->cluster);
	suben->addr = (INTBIG)pt;
	us_addexplorernode(suben, &en->subexplorernode, en);
	return(suben);
}

/*
 * Callback routine (called from "us_buildexplorersturctsimulation()") to create
 * a new leaf (under "parentnode") called "leafname".  Returns the leaf node.
 */
void *us_explorersimnewleaf(CHAR *leafname, void *parentnode)
{
	REGISTER EXPLORERNODE *en, *suben;
	CHAR *pt;

	en = (EXPLORERNODE *)parentnode;
	if (en == 0) en = us_explorertopsimnode; else
		en->enodetype = EXNODETYPESIMBRANCH;

	suben = us_allocexplorernode(us_explorercursimwindow);
	if (suben == NOEXPLORERNODE) return(0);
	suben->enodetype = EXNODETYPESIMLEAF;
	allocstring(&pt, leafname, us_tool->cluster);
	suben->addr = (INTBIG)pt;
	us_addexplorernode(suben, &en->subexplorernode, en);
	return(suben);
}

void us_clearexplorersimulation(void)
{
	REGISTER WINDOWPART *w;
	REGISTER EXPLORERNODE *en, *suben, *lasten;
	REGISTER EXPWINDOW *ew;

	/* see if there is an explorer window */
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		if ((w->state&WINDOWTYPE) != EXPLORERWINDOW) continue;
		ew = (EXPWINDOW *)w->expwindow;
		if ((ew->nodesimulation->flags&EXNODEPLACEHOLDER) == 0)
		{
			ew->nodesimulation->flags |= EXNODEPLACEHOLDER;
			ew->nodesimulation->flags &= ~EXNODEOPEN;

			/* remove all error entries */
			lasten = NOEXPLORERNODE;
			for(en = ew->nodeused; en != NOEXPLORERNODE; en = en->nextexplorernode)
			{
				if (en->enodetype == EXNODETYPESIMBRANCH || en->enodetype == EXNODETYPESIMLEAF)
				{
					if (lasten == NOEXPLORERNODE) ew->nodeused = en->nextexplorernode; else
						lasten->nextexplorernode = en->nextexplorernode;
					continue;
				}
				lasten = en;
			}
			while (ew->nodesimulation->subexplorernode != NOEXPLORERNODE)
			{
				en = ew->nodesimulation->subexplorernode;
				ew->nodesimulation->subexplorernode = en->nextsubexplorernode;
				while (en->subexplorernode != NOEXPLORERNODE)
				{
					suben = en->subexplorernode;
					en->subexplorernode = suben->nextsubexplorernode;
					us_freeexplorernode(suben);
				}
				us_freeexplorernode(en);
			}
			ew->nodeselected = NOEXPLORERNODE;
		}
		us_exploreredisphandler(w);
	}
}

/*
 * Routine to build a hierarchical-explorer structure from the current database.
 */
void us_buildexplorersturcthierarchical(EXPWINDOW *ew, EXPLORERNODE *whichtree)
{
	REGISTER EXPLORERNODE *en, *sen;
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np, *inp, *cnp;

	/* scan each library */
	whichtree->flags &= ~EXNODEPLACEHOLDER;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if ((lib->userbits&HIDDENLIBRARY) != 0) continue;

		/* create an explorer node for this library */
		en = us_allocexplorernode(ew);
		if (en == NOEXPLORERNODE) break;
		if (lib == el_curlib) en->flags = EXNODEOPEN;
		en->enodetype = EXNODETYPELIBRARY;
		en->addr = (INTBIG)lib;
		us_addexplorernode(en, &whichtree->subexplorernode, whichtree);

		/* find top-level cells in the library */
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->firstinst != NONODEINST) continue;

			/* if any view or version is in use, this cell isn't top-level */
			FOR_CELLGROUP(cnp, np)
			{
				for(inp = cnp; inp != NONODEPROTO; inp = inp->prevversion)
					if (inp->firstinst != NONODEINST) break;
				if (inp != NONODEPROTO) break;
			}
			if (cnp != NONODEPROTO) continue;

			/* create an explorer node for this cell */
			sen = us_allocexplorernode(ew);
			if (sen == NOEXPLORERNODE) break;
			sen->enodetype = EXNODETYPECELL;
			sen->addr = (INTBIG)np;
			us_addexplorernode(sen, &en->subexplorernode, en);

			/* add explorer nodes for everything under this cell */
			us_createhierarchicalexplorertree(ew, sen, np);
		}
	}
}

/*
 * Routine to build a hierarchical explorer structure starting at node "en".
 */
void us_createhierarchicalexplorertree(EXPWINDOW *ew, EXPLORERNODE *en, NODEPROTO *np)
{
	REGISTER NODEPROTO *subnp, *cnp, *onp;
	REGISTER NODEINST *ni;
	REGISTER EXPLORERNODE *sen;

	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		subnp = ni->proto;
		if (subnp->primindex != 0) continue;

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(subnp, np)) continue;
		for(sen = en->subexplorernode; sen != NOEXPLORERNODE; sen = sen->nextsubexplorernode)
		{
			if (sen->enodetype != EXNODETYPECELL) continue;
			if ((NODEPROTO *)sen->addr == subnp) break;
		}
		if (sen == NOEXPLORERNODE)
		{
			sen = us_allocexplorernode(ew);
			if (sen == NOEXPLORERNODE) break;
			sen->enodetype = EXNODETYPECELL;
			sen->addr = (INTBIG)subnp;
			us_addexplorernode(sen, &en->subexplorernode, en);
			us_createhierarchicalexplorertree(ew, sen, subnp);
		}
		sen->count++;

		/* include associated cells here */
		FOR_CELLGROUP(cnp, subnp)
		{
			for(onp = cnp; onp != NONODEPROTO; onp = onp->prevversion)
			{
				if (onp == subnp) continue;
				for(sen = en->subexplorernode; sen != NOEXPLORERNODE; sen = sen->nextsubexplorernode)
				{
					if (sen->enodetype != EXNODETYPECELL) continue;
					if ((NODEPROTO *)sen->addr == onp) break;
				}
				if (sen == NOEXPLORERNODE)
				{
					sen = us_allocexplorernode(ew);
					if (sen == NOEXPLORERNODE) break;
					sen->enodetype = EXNODETYPECELL;
					sen->addr = (INTBIG)onp;
					us_addexplorernode(sen, &en->subexplorernode, en);
					us_createhierarchicalexplorertree(ew, sen, onp);
				}
				if (onp == contentsview(subnp)) sen->count++;
			}
		}
	}
}

/*
 * Routine to build a contents-explorer structure from the current database.
 */
void us_buildexplorersturctcontents(EXPWINDOW *ew, EXPLORERNODE *whichtree)
{
	REGISTER EXPLORERNODE *en, *sen, *ten;
	REGISTER INTBIG i;
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;

	/* scan each library */
	whichtree->flags &= ~EXNODEPLACEHOLDER;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if ((lib->userbits&HIDDENLIBRARY) != 0) continue;

		/* create an explorer node for this library */
		en = us_allocexplorernode(ew);
		if (en == NOEXPLORERNODE) break;
		en->enodetype = EXNODETYPELIBRARY;
		en->addr = (INTBIG)lib;
		us_addexplorernode(en, &whichtree->subexplorernode, whichtree);

		/* see if this library contains technology-edit primitives */
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			if ((np->userbits&TECEDITCELL) != 0) break;
		if (np != NONODEPROTO)
		{
			/* technology edit library: show it specially */
			for(i=0; i<4; i++)
			{
				ten = us_allocexplorernode(ew);
				if (ten == NOEXPLORERNODE) break;
				ten->flags = 0;
				ten->enodetype = EXNODETYPELABEL;
				switch (i)
				{
					case 0: ten->addr = EXLABELTYPELAYERCELLS;  break;
					case 1: ten->addr = EXLABELTYPEARCCELLS;    break;
					case 2: ten->addr = EXLABELTYPENODECELLS;   break;
					case 3: ten->addr = EXLABELTYPEMISCCELLS;   break;
				}
				us_addexplorernode(ten, &en->subexplorernode, en);
				for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				{
					/* create an explorer node for this cell */
					if (namesamen(np->protoname, x_("layer-"), 6) == 0)
					{
						if (i != 0) continue;
					} else if (namesamen(np->protoname, x_("arc-"), 4) == 0)
					{
						if (i != 1) continue;
					} else if (namesamen(np->protoname, x_("node-"), 5) == 0)
					{
						if (i != 2) continue;
					} else
					{
						if (i != 3) continue;
					}
					sen = us_allocexplorernode(ew);
					if (sen == NOEXPLORERNODE) break;
					sen->flags = EXNODEPLACEHOLDER;
					sen->enodetype = EXNODETYPECELL;
					sen->addr = (INTBIG)np;
					us_addexplorernode(sen, &ten->subexplorernode, ten);
				}
			}
		} else
		{
			/* normal library: load all cells */
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				/* create an explorer node for this cell */
				sen = us_allocexplorernode(ew);
				if (sen == NOEXPLORERNODE) break;
				sen->flags = EXNODEPLACEHOLDER;
				sen->enodetype = EXNODETYPECELL;
				sen->addr = (INTBIG)np;
				us_addexplorernode(sen, &en->subexplorernode, en);
			}
		}
	}
}

void us_buildexplorernodecontents(EXPWINDOW *ew, NODEPROTO *np, EXPLORERNODE *whichtree)
{
	REGISTER EXPLORERNODE *len, *ben;
	REGISTER PORTPROTO *pp;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER NETWORK *net;
	REGISTER VARIABLE *var;

	whichtree->flags &= ~EXNODEPLACEHOLDER;

	/* add all nodes in the cell */
	len = us_allocexplorernode(ew);
	if (len == NOEXPLORERNODE) return;
	len->enodetype = EXNODETYPELABEL;
	len->addr = EXLABELTYPENODELIST;
	us_addexplorernode(len, &whichtree->subexplorernode, whichtree);
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
		if (var == NOVARIABLE) continue;
		ben = us_allocexplorernode(ew);
		if (ben == NOEXPLORERNODE) break;
		ben->enodetype = EXNODETYPENODE;
		ben->addr = (INTBIG)ni;
		us_addexplorernode(ben, &len->subexplorernode, len);
	}

	/* add all arcs in the cell */
	len = us_allocexplorernode(ew);
	if (len == NOEXPLORERNODE) return;
	len->enodetype = EXNODETYPELABEL;
	len->addr = EXLABELTYPEARCLIST;
	us_addexplorernode(len, &whichtree->subexplorernode, whichtree);
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
		if (var == NOVARIABLE) continue;
		ben = us_allocexplorernode(ew);
		if (ben == NOEXPLORERNODE) break;
		ben->enodetype = EXNODETYPEARC;
		ben->addr = (INTBIG)ai;
		us_addexplorernode(ben, &len->subexplorernode, len);
	}

	/* add all exports in the cell */
	len = us_allocexplorernode(ew);
	if (len == NOEXPLORERNODE) return;
	len->enodetype = EXNODETYPELABEL;
	len->addr = EXLABELTYPEEXPORTLIST;
	us_addexplorernode(len, &whichtree->subexplorernode, whichtree);
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		ben = us_allocexplorernode(ew);
		if (ben == NOEXPLORERNODE) break;
		ben->enodetype = EXNODETYPEEXPORT;
		ben->addr = (INTBIG)pp;
		us_addexplorernode(ben, &len->subexplorernode, len);
	}

	/* add all networks in the cell */
	len = us_allocexplorernode(ew);
	if (len == NOEXPLORERNODE) return;
	len->enodetype = EXNODETYPELABEL;
	len->addr = EXLABELTYPENETLIST;
	us_addexplorernode(len, &whichtree->subexplorernode, whichtree);
	for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
	{
		ben = us_allocexplorernode(ew);
		if (ben == NOEXPLORERNODE) break;
		ben->enodetype = EXNODETYPENETWORK;
		ben->addr = (INTBIG)net;
		us_addexplorernode(ben, &len->subexplorernode, len);
	}
}

/*
 * Routine to add explorer node "en" to the list headed by "head".
 * That tree is under node "parent" (which is NOEXPLORERNODE if at the top).
 * The node is inserted alphabetically.
 */
void us_addexplorernode(EXPLORERNODE *en, EXPLORERNODE **head, EXPLORERNODE *parent)
{
	EXPLORERNODE *aen, *lasten;
	CHAR *name, *aname;
	REGISTER INTBIG len;

	lasten = NOEXPLORERNODE;
	name = us_describeexplorernode(en, TRUE);
	if (name == 0 || en->enodetype == EXNODETYPELABEL)
	{
		/* no sorting needed: just find the end of the list */
		for(aen = *head; aen != NOEXPLORERNODE; aen = aen->nextsubexplorernode)
			lasten = aen;
	} else
	{
		/* remember this name in a global string */
		len = estrlen(name) + 1;
		if (len > us_explorertempstringtotal)
		{
			if (us_explorertempstringtotal > 0) efree(us_explorertempstring);
			us_explorertempstringtotal = 0;
			us_explorertempstring = (CHAR *)emalloc(len * SIZEOFCHAR, us_tool->cluster);
			if (us_explorertempstring == 0) return;
			us_explorertempstringtotal = len;
		}
		estrcpy(us_explorertempstring, name);

		for(aen = *head; aen != NOEXPLORERNODE; aen = aen->nextsubexplorernode)
		{
			aname = us_describeexplorernode(aen, TRUE);
			if (namesame(us_explorertempstring, aname) < 0) break;
			lasten = aen;
		}
	}
	if (lasten == NOEXPLORERNODE)
	{
		en->nextsubexplorernode = *head;
		*head = en;
	} else
	{
		en->nextsubexplorernode = lasten->nextsubexplorernode;
		lasten->nextsubexplorernode = en;
	}
	en->parent = parent;
}

/*
 * Routine to describe explorer node "en".  If "purename" is TRUE, shows only
 * the name.  Otherwise, gives the text as it will appear in the explorer.
 * Returns 0 if it cannot determine a name.
 */
CHAR *us_describeexplorernode(EXPLORERNODE *en, BOOLEAN purename)
{
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;
	REGISTER NETWORK *net;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER VARIABLE *var;
	REGISTER void *infstr;
	static CHAR genname[50];

	switch (en->enodetype)
	{
		case EXNODETYPELIBRARY:
			lib = (LIBRARY *)en->addr;
			if (purename) return(lib->libname);
			infstr = initinfstr();
			if (lib == el_curlib) addstringtoinfstr(infstr, _("CURRENT "));
			addstringtoinfstr(infstr, _("LIBRARY: "));
			addstringtoinfstr(infstr, lib->libname);
			return(returninfstr(infstr));
		case EXNODETYPECELL:
			np = (NODEPROTO *)en->addr;
			if (purename || en->count <= 0) return(nldescribenodeproto(np));
			infstr = initinfstr();
			formatinfstr(infstr, x_("%s (%ld)"), nldescribenodeproto((NODEPROTO *)en->addr), en->count);
			return(returninfstr(infstr));
		case EXNODETYPEEXPORT:
			return(((PORTPROTO *)en->addr)->protoname);
		case EXNODETYPENODE:
			ni = (NODEINST *)en->addr;
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
			if (var == NOVARIABLE) return(0);
			return((CHAR *)var->addr);
		case EXNODETYPEARC:
			ai = (ARCINST *)en->addr;
			var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
			if (var == NOVARIABLE) return(0);
			return((CHAR *)var->addr);
		case EXNODETYPENETWORK:
			net = (NETWORK *)en->addr;
			if (net->namecount != 0) return(networkname(net, 0));
			esnprintf(genname, 50, x_("NET%ld"), (INTBIG)net);
			return(genname);
		case EXNODETYPEERROR:
			if (purename) return(0);
			return(describeerror((void *)en->addr));
		case EXNODETYPEERRORGEOM:
			if (purename) return(0);
			return(describeerrorgeom((void *)en->addr));
		case EXNODETYPESIMBRANCH:
		case EXNODETYPESIMLEAF:
			if (purename) return(0);
			return((CHAR *)en->addr);
		case EXNODETYPELABEL:
			switch (en->addr)
			{
				case EXLABELTYPEHIVIEW:      return(_("***** HIERARCHICAL VIEW"));
				case EXLABELTYPECONVIEW:     return(_("***** CONTENTS VIEW"));
				case EXLABELTYPEERRVIEW:     return(_("***** ERRORS LIST"));
				case EXLABELTYPESIMVIEW:     return(_("***** SIMULATION SIGNAL LIST"));
				case EXLABELTYPENODELIST:    return(_("NODE LIST"));
				case EXLABELTYPEARCLIST:     return(_("ARC LIST"));
				case EXLABELTYPEEXPORTLIST:  return(_("EXPORT LIST"));
				case EXLABELTYPENETLIST:     return(_("NETWORK LIST"));
				case EXLABELTYPELAYERCELLS:  return(_("TECHNOLOGY EDITOR LAYERS"));
				case EXLABELTYPEARCCELLS:    return(_("TECHNOLOGY EDITOR ARCS"));
				case EXLABELTYPENODECELLS:   return(_("TECHNOLOGY EDITOR NODES"));
				case EXLABELTYPEMISCCELLS:   return(_("TECHNOLOGY EDITOR MISCELLANEOUS"));
			}
			break;
	}
	return(0);
}

/*
 * Routine to increment the stack depth "ew->depth" and to expand the stack
 * globals "ew->stack" and "ew->stackdone" if necessary.
 */
BOOLEAN us_expandexplorerdepth(EXPWINDOW *ew)
{
	REGISTER INTBIG newlimit, i;
	REGISTER EXPLORERNODE **stk;
	REGISTER BOOLEAN *done;

	if (ew->depth >= ew->stacksize)
	{
		newlimit = ew->depth + 10;
		stk = (EXPLORERNODE **)emalloc(newlimit * (sizeof (EXPLORERNODE *)), us_tool->cluster);
		if (stk == 0) return(TRUE);
		done = (BOOLEAN *)emalloc(newlimit * (sizeof (BOOLEAN)), us_tool->cluster);
		if (done == 0) return(TRUE);
		for(i=0; i<ew->depth; i++)
		{
			stk[i] = ew->stack[i];
			done[i] = ew->stackdone[i];
		}
		
		if (ew->stacksize > 0)
		{
			efree((CHAR *)ew->stack);
			efree((CHAR *)ew->stackdone);
		}
		ew->stack = stk;
		ew->stackdone = done;
		ew->stacksize = newlimit;
	}
	ew->depth++;
	return(FALSE);
}

/*
 * Routine to figure out which explorer window is the current one.  First tries to
 * find one in the same frame as the current window.  Next looks for any explorer.
 * Returns NOEXPWINDOW if none found.
 */
EXPWINDOW *us_getcurrentexplorerwindow(void)
{
	REGISTER WINDOWPART *w;

	if (el_curwindowpart != NOWINDOWPART)
	{
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if ((w->state&WINDOWTYPE) != EXPLORERWINDOW) continue;
			if (w->frame == el_curwindowpart->frame) return((EXPWINDOW *)w->expwindow);
		}
	}

	/* see if there is any explorer window */
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		if ((w->state&WINDOWTYPE) == EXPLORERWINDOW) return((EXPWINDOW *)w->expwindow);

	return(NOEXPWINDOW);
}

/*
 * Routine to copy the current explorer node text to the clipboard.
 */
void us_copyexplorerwindow(void)
{
	REGISTER void *infstr;
	REGISTER EXPWINDOW *ew;
	
	ew = us_getcurrentexplorerwindow();
	if (ew == NOEXPWINDOW) return;

	if (ew->nodeselected == NOEXPLORERNODE)
	{
		ttyputerr(_("Select a line in the explorer before copying it"));
		return;
	}
	infstr = initinfstr();
	us_addtoexplorerwindowcopy(infstr, 0, ew->nodeselected);
	setcutbuffer(returninfstr(infstr));
}

/*
 * Helper routine for "us_copyexplorerwindow()" to construct a copy of the selected explorer node.
 */
void us_addtoexplorerwindowcopy(void *infstr, INTBIG depth, EXPLORERNODE *en)
{
	REGISTER CHAR *pt;
	REGISTER EXPLORERNODE *sen;
	REGISTER INTBIG i;

	pt = us_describeexplorernode(en, FALSE);
	if (pt == 0) return;
	for(i=0; i<depth; i++) addstringtoinfstr(infstr, x_("    "));
	formatinfstr(infstr, x_("%s\n"), pt);
	if (en->subexplorernode == NOEXPLORERNODE) return;
	if ((en->flags&EXNODEOPEN) == 0 || (en->flags&EXNODEPLACEHOLDER) != 0) return;
	for(sen = en->subexplorernode; sen != NOEXPLORERNODE; sen = sen->nextsubexplorernode)
	{
		us_addtoexplorerwindowcopy(infstr, depth+1, sen);
	}
}

/*
 * Routine called when the hierarchy has changed: rebuilds the explorer
 * structure and preserves as much information as possible when
 * redisplaying it.
 */
void us_redoexplorerwindow(void)
{
	REGISTER WINDOWPART *w;
	REGISTER EXPWINDOW *ew;
	REGISTER INTBIG lasttopline, hierarchyflags, contentsflags, errorsflags, simflags;
	REGISTER EXPLORERNODE *en, *oldtopnode, *oldusedtop;

	/* see if there is an explorer window */
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		if ((w->state&WINDOWTYPE) != EXPLORERWINDOW) continue;
		ew = (EXPWINDOW *)w->expwindow;

		/* remember the former list of explorer nodes */
		if (ew->nodehierarchy == 0) hierarchyflags = EXNODEPLACEHOLDER; else
			hierarchyflags = ew->nodehierarchy->flags;
		if (ew->nodecontents == 0) contentsflags = EXNODEPLACEHOLDER; else
			contentsflags = ew->nodecontents->flags;
		if (ew->nodeerrors == 0) errorsflags = EXNODEPLACEHOLDER; else
			errorsflags = ew->nodeerrors->flags;
		if (ew->nodesimulation == 0) simflags = EXNODEPLACEHOLDER; else
			simflags = ew->nodesimulation->flags;
		oldusedtop = ew->nodeused;
		ew->nodeused = NOEXPLORERNODE;
		oldtopnode = ew->firstnode;
		ew->firstnode = NOEXPLORERNODE;

		/* rebuild the explorer structure */
		us_buildexplorerstruct(w, ew);

		/* restore state of each section */
		if ((hierarchyflags&EXNODEPLACEHOLDER) == 0)
			us_buildexplorersturcthierarchical(ew, ew->nodehierarchy);
		if ((contentsflags&EXNODEPLACEHOLDER) == 0)
			us_buildexplorersturctcontents(ew, ew->nodecontents);
		if ((errorsflags&EXNODEPLACEHOLDER) == 0)
			us_buildexplorersturcterrors(ew, ew->nodeerrors);
		if ((simflags&EXNODEPLACEHOLDER) == 0)
			us_buildexplorersturctsimulation(w, ew, ew->nodesimulation);

		/* find the place where the former "first line" used to be, copy expansion */
		ew->depth = 0;
		lasttopline = ew->firstline;
		ew->firstline = 0;
		ew->nodenowselected = NOEXPLORERNODE;
		(void)us_scannewexplorerstruct(ew, ew->firstnode, oldtopnode, 0);
		ew->nodeselected = ew->nodenowselected;
		if (lasttopline == 0) ew->firstline = 0;
		us_exploreredisphandler(w);

		/* free the former list of explorer nodes */
		while (oldusedtop != NOEXPLORERNODE)
		{
			en = oldusedtop;
			oldusedtop = en->nextexplorernode;
			us_freeexplorernode(en);
		}
	}
}

/*
 * Helper routine for "us_redoexplorerwindow" which scans for the proper "top line" in the
 * explorer window and copies the "expansion" bits from the old structure.  The new
 * explorer node being examined is "firsten", and the old one (if there is one) is
 * "oldfirsten".  The current line number in the explorer window is "line", and the
 * routine returns the line number after this node has been scanned.
 */
INTBIG us_scannewexplorerstruct(EXPWINDOW *ew, EXPLORERNODE *firsten, EXPLORERNODE *oldfirsten, INTBIG line)
{
	REGISTER EXPLORERNODE *en, *oen;
	REGISTER INTBIG newline;

	/* push the explorer stack */
	if (us_expandexplorerdepth(ew)) return(line);

	for(en = firsten; en != NOEXPLORERNODE; en = en->nextsubexplorernode)
	{
		/* record the position in the stack */
		ew->stack[ew->depth-1] = en;

		line++;

		/* copy the expansion bits */
		oen = NOEXPLORERNODE;
		if (oldfirsten != NOEXPLORERNODE)
		{
			for(oen = oldfirsten; oen != NOEXPLORERNODE; oen = oen->nextsubexplorernode)
			{
				if (oen->addr == en->addr) break;
			}
			if (oen != NOEXPLORERNODE)
			{
				if (oen == ew->nodeselected)
					ew->nodenowselected = en;
				en->flags = (en->flags & ~EXNODEOPEN) | (oen->flags & EXNODEOPEN);
			}
		}

		/* now scan children */
		if (en->subexplorernode != NOEXPLORERNODE)
		{
			if (oen != NOEXPLORERNODE) oen = oen->subexplorernode;
			newline = us_scannewexplorerstruct(ew, en->subexplorernode, oen, line);
			if ((en->flags&EXNODEOPEN) != 0) line = newline;
		}
	}

	/* pop the explorer stack */
	ew->depth--;
	return(line);
}

/*
 * Routine to return the currently selected cell in the explorer window.
 * Returns NONODEPROTO if none selected.
 */
NODEPROTO *us_currentexplorernode(void)
{
	REGISTER EXPWINDOW *ew;

	ew = us_getcurrentexplorerwindow();
	if (ew == NOEXPWINDOW) return(NONODEPROTO);
	if (ew->nodeselected == NOEXPLORERNODE) return(NONODEPROTO);
	switch (ew->nodeselected->enodetype)
	{
		case EXNODETYPECELL:
			return((NODEPROTO *)ew->nodeselected->addr);
		case EXNODETYPENODE:
			return(((NODEINST *)ew->nodeselected->addr)->parent);
		case EXNODETYPEARC:
			return(((ARCINST *)ew->nodeselected->addr)->parent);
		case EXNODETYPENETWORK:
			return(((NETWORK *)ew->nodeselected->addr)->parent);
		case EXNODETYPEEXPORT:
			return(((PORTPROTO *)ew->nodeselected->addr)->parent);
	}
	return(NONODEPROTO);
}

/*
 * Routine to initialize the dumping of the explorer text in window "w".
 */
void us_explorerreadoutinit(WINDOWPART *w)
{
	REGISTER EXPWINDOW *ew;

	ew = (EXPWINDOW *)w->expwindow;
	ew->depth = 0;
	(void)us_expandexplorerdepth(ew);
	ew->stack[ew->depth-1] = ew->firstnode;
}

/*
 * Routine to get the next line of text in the explorer window "w".  Returns
 * zero at the end.  Sets "indent" to the amount of indentation of this line.
 * Sets "style" to the style of the line (1 for empty box, 2 for "+" in box,
 * and 3 for "-" in box).  "donelist" is set to the address of an array of
 * BOOLEANs that indicates, for each level of indentation, whether the column is
 * done or needs a continuation bar in it.
 */
CHAR *us_explorerreadoutnext(WINDOWPART *w, INTBIG *indent, INTBIG *style, BOOLEAN **donelist,
	void **ven)
{
	REGISTER CHAR *name;
	REGISTER EXPLORERNODE *en;
	REGISTER INTBIG i;
	REGISTER EXPWINDOW *ew;

	ew = (EXPWINDOW *)w->expwindow;

	/* pop out if at the end of a depth traversal */
	while (ew->stack[ew->depth-1] == NOEXPLORERNODE)
	{
		ew->depth--;
		if (ew->depth == 0) return(0);
	}

	/* get the current explorer node */
	en = ew->stack[ew->depth-1];
	ew->stack[ew->depth-1] = en->nextsubexplorernode;

	*indent = ew->depth-1;
	name = us_describeexplorernode(en, FALSE);
	if (name == 0) name = x_("?");
	*style = 1;
	if (en->subexplorernode != NOEXPLORERNODE || (en->flags&EXNODEPLACEHOLDER) != 0)
	{
		/* indicate "-" in the box: it has children */
		*style = 3;
		if ((en->flags&EXNODEOPEN) == 0)
		{
			/* indicate a "+" because it is not expanded */
			*style = 2;
		}
	}

	/* fill in the indication of which levels are at the end */
	for(i=0; i<ew->depth; i++)
	{
		ew->stackdone[i] = FALSE;
		if (i < ew->depth-2 && ew->stack[i+1] == NOEXPLORERNODE)
			ew->stackdone[i] = TRUE;
	}
	*donelist = ew->stackdone;

	/* now draw children */
	if ((en->flags&EXNODEOPEN) != 0 && en->subexplorernode != NOEXPLORERNODE)
	{
		if (us_expandexplorerdepth(ew)) return(0);
		ew->stack[ew->depth-1] = en->subexplorernode;
	}

	if (ven != 0)
		*((EXPLORERNODE **)ven) = en;
	return(name);
}

/*
 * Redisplay routine for explorer window "w".
 */
void us_exploreredisphandler(WINDOWPART *w)
{
	INTBIG wid, hei, indent, style;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	REGISTER INTBIG visiblelines, thumbsize, thumbtop, thumbarea, line, xpos, ypos,
		topypos, lastboxxpos, i, l, boxsize, yposbox, dashindent, leftmargin, thumbcenter;
	BOOLEAN *done;
	REGISTER CHAR *name;
	CHAR onechar[2];
	EXPLORERNODE *en;
	REGISTER VARIABLE *var;
	WINDOWPART ww;
	extern GRAPHICS us_cellgra;
	REGISTER EXPWINDOW *ew;

	ew = (EXPWINDOW *)w->expwindow;
	if (ew == NOEXPWINDOW) return;
	us_erasewindow(w);

	/* determine text size */
	var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_facet_explorer_textsize"));
	if (var == NOVARIABLE) us_exploretextsize = DEFAULTEXPLORERTEXTSIZE; else
		us_exploretextsize = var->addr;
	TDCLEAR(descript);
	TDSETSIZE(descript, TXTSETPOINTS(us_exploretextsize));
	screensettextinfo(w, NOTECHNOLOGY, descript);
	screengettextsize(w, x_("Xy"), &wid, &hei);
	us_exploretextheight = hei;
	screengettextsize(w, x_("W"), &wid, &hei);
	us_exploretextwidth = wid;
	visiblelines = (w->usehy - w->usely - DISPLAYSLIDERSIZE) / us_exploretextheight;

	/* reset indication of which lines are visible */
	for(en = ew->nodeused; en != NOEXPLORERNODE; en = en->nextexplorernode)
		en->flags &= ~EXNODESHOWN;

	/* redraw the explorer information */
	us_explorerreadoutinit(w);
	us_cellgra.col = BLACK;
	ew->totallines = 0;
	leftmargin = w->uselx + 5;
	lastboxxpos = -1;
	boxsize = us_exploretextheight * 2 / 3;
	dashindent = boxsize / 5;
	for(line = 0; ; line++)
	{
		name = us_explorerreadoutnext(w, &indent, &style, &done, (void **)&en);
		if (name == 0) break;
		ew->totallines++;
		if (line < ew->firstline) continue;

		xpos = leftmargin + indent * us_exploretextheight - ew->firstchar*us_exploretextwidth;
		ypos = w->usehy - (line-ew->firstline+1) * us_exploretextheight;
		if (ypos < w->usely+DISPLAYSLIDERSIZE) continue;

		/* draw the box */
		yposbox = ypos + (us_exploretextheight-boxsize) / 2;
		if (xpos >= w->uselx)
		{
			screendrawline(w, xpos,         yposbox,         xpos,         yposbox+boxsize, &us_cellgra, 0);
			screendrawline(w, xpos,         yposbox+boxsize, xpos+boxsize, yposbox+boxsize, &us_cellgra, 0);
			screendrawline(w, xpos+boxsize, yposbox+boxsize, xpos+boxsize, yposbox,         &us_cellgra, 0);
			screendrawline(w, xpos+boxsize, yposbox,         xpos,         yposbox,         &us_cellgra, 0);
			if (style >= 2)
			{
				/* draw the "-" in the box: it has children */
				screendrawline(w, xpos+dashindent, yposbox+boxsize/2, xpos+boxsize-dashindent, yposbox+boxsize/2,
					&us_cellgra, 0);
				if (style == 2)
				{
					/* make the "-" a "+" because it is not expanded */
					screendrawline(w, xpos+boxsize/2, yposbox+dashindent, xpos+boxsize/2, yposbox+boxsize-dashindent,
						&us_cellgra, 0);
				}
			}

			/* draw the connecting lines */
			if (indent > 0)
			{
				us_cellgra.col = DGRAY;
				l = xpos-us_exploretextheight+boxsize/2;
				if (l < w->uselx) l = w->uselx;
				screendrawline(w, xpos, yposbox+boxsize/2, l, yposbox+boxsize/2, &us_cellgra, 0);

				for(i=0; i<indent; i++)
				{
					if (done[i]) continue;
					l = leftmargin + i * us_exploretextheight + boxsize/2 -
						ew->firstchar*us_exploretextwidth;
					if (l < w->uselx) continue;
					if (l == lastboxxpos)
						topypos = yposbox+boxsize/2+us_exploretextheight-boxsize/2; else
							topypos = yposbox+boxsize/2+us_exploretextheight;
					if (topypos > w->usehy) topypos = w->usehy-1;
					screendrawline(w, l, yposbox+boxsize/2, l, topypos, &us_cellgra, 0);
				}
				us_cellgra.col = BLACK;
			}
		}
		lastboxxpos = xpos + boxsize/2;

		/* show the text */
		l = xpos + boxsize + boxsize/2;
		while (l < w->uselx && *name != 0)
		{
			onechar[0] = *name++;
			onechar[1] = 0;
			screengettextsize(w, onechar, &wid, &hei);
			l += wid;
		}
		if (*name == 0) wid = hei = 0; else
		{
			screendrawtext(w, l, ypos, name, &us_cellgra);
			screengettextsize(w, name, &wid, &hei);
		}
		en->textwidth = wid;
		en->flags |= EXNODESHOWN;
		en->x = xpos+boxsize+boxsize/2;   en->y = yposbox;
	}

	if (ew->nodeselected != NOEXPLORERNODE) us_highlightexplorernode(w);

	/* draw the vertical slider on the right */
	ww.screenlx = ww.uselx = w->uselx;
	ww.screenhx = ww.usehx = w->usehx;
	ww.screenly = ww.usely = w->usely;
	ww.screenhy = ww.usehy = w->usehy;
	ww.frame = w->frame;
	ww.state = DISPWINDOW;
	computewindowscale(&ww);
	us_drawverticalslider(w, w->usehx-DISPLAYSLIDERSIZE, w->usely+DISPLAYSLIDERSIZE, w->usehy, FALSE);

	thumbsize = visiblelines * 100 / ew->totallines;
	if (thumbsize >= 100) thumbsize = 100;
	if (thumbsize != 100 || ew->firstline != 0)
	{
		thumbtop = ew->firstline * 100 / ew->totallines;
		thumbarea = w->usehy - w->usely - DISPLAYSLIDERSIZE*3;

		w->thumbhy = w->usehy - DISPLAYSLIDERSIZE - thumbarea * thumbtop / 100;
		w->thumbly = w->thumbhy - thumbarea * thumbsize / 100;
		if (w->thumbhy > w->usehy-DISPLAYSLIDERSIZE-2) w->thumbhy = w->usehy-DISPLAYSLIDERSIZE-2;
		if (w->thumbly < w->usely+DISPLAYSLIDERSIZE*2+2) w->thumbly = w->usely+DISPLAYSLIDERSIZE*2+2;
		us_drawverticalsliderthumb(w, w->usehx-DISPLAYSLIDERSIZE, w->usely+DISPLAYSLIDERSIZE,
			w->usehy, w->thumbly, w->thumbhy);
	}

	/* draw the horizontal slider on the bottom */
	us_drawhorizontalslider(w, w->usely+DISPLAYSLIDERSIZE, w->uselx, w->usehx-DISPLAYSLIDERSIZE, 1);
	thumbsize = (w->usehx - w->uselx - DISPLAYSLIDERSIZE*4) / 20;
	thumbarea = w->usehx - w->uselx - DISPLAYSLIDERSIZE*4 - thumbsize;
	thumbcenter = w->uselx + DISPLAYSLIDERSIZE*2 + ew->firstchar * thumbarea /
		MAXCELLEXPLHSCROLL + thumbsize/2;
	w->thumblx = thumbcenter - thumbsize/2;
	w->thumbhx = thumbcenter + thumbsize/2;
	us_drawhorizontalsliderthumb(w, w->usely+DISPLAYSLIDERSIZE, w->uselx,
		w->usehx-DISPLAYSLIDERSIZE, w->thumblx, w->thumbhx, 1);

	/* draw the explorer icon */
	us_drawexplorericon(w, w->uselx, w->usely);
}

/* coordinate pairs for the dots in the explorer icon (measured from low X/Y) */
static INTBIG us_explorerdots[] = {
	1,10, 1,9, 1,8, 1,7, 1,6, 1,5, 1,4, 1,3,
	2,10, 2,9, 2,8, 2,7, 2,6, 2,5, 2,4, 2,3,
	3,8, 3,7, 3,3, 3,2,
	4,8, 4,7, 4,3, 4,2,
	6,8, 6,7, 6,3, 6,2,
	7,9, 7,6, 7,4, 7,1,
	8,9, 8,6, 8,4, 8,1,
	9,8, 9,7, 9,3, 9,2,
	-1,-1};

void us_drawexplorericon(WINDOWPART *w, INTBIG lx, INTBIG ly)
{
	us_drawwindowicon(w, lx, ly, us_explorerdots);
}

void us_drawwindowicon(WINDOWPART *w, INTBIG lx, INTBIG ly, INTBIG *dots)
{
	REGISTER INTBIG x, y, i;
	extern GRAPHICS us_box;

	/* erase the icon area */
	us_box.col = WHITE;
	gra_drawbox(w, lx, lx+DISPLAYSLIDERSIZE-1, ly, ly+DISPLAYSLIDERSIZE-1, &us_box);

	/* draw the top and right edge */
	us_box.col = BLACK;
	gra_drawbox(w, lx, lx+11, ly+11, ly+11, &us_box);
	gra_drawbox(w, lx+11, lx+11, ly, ly+11, &us_box);

	/* draw the icon dots */
	for(i=0; dots[i] >= 0; i += 2)
	{
		x = lx + dots[i];
		y = ly + dots[i+1];
		gra_drawbox(w, x, x, y, y, &us_box);
	}
}

void us_highlightexplorernode(WINDOWPART *w)
{
	REGISTER INTBIG lowx, highx, lowy, highy;
	REGISTER EXPWINDOW *ew;

	ew = (EXPWINDOW *)w->expwindow;
	if ((ew->nodeselected->flags&EXNODESHOWN) == 0) return;
	lowx = ew->nodeselected->x;
	highx = lowx + ew->nodeselected->textwidth;
	if (lowx < w->uselx) lowx = w->uselx;
	if (highx > w->usehx-DISPLAYSLIDERSIZE) highx = w->usehx-DISPLAYSLIDERSIZE;
	lowy = ew->nodeselected->y - 2;
	highy = lowy + us_exploretextheight;
	if (lowy < w->usely+DISPLAYSLIDERSIZE) lowy = w->usely+DISPLAYSLIDERSIZE;
	if (highy > w->usehy) highy = w->usehy;
	screeninvertbox(w, lowx, highx, lowy, highy);
}

/*
 * Routine called when up or down slider arrows are clicked.
 */
BOOLEAN us_explorerarrowdown(INTBIG x, INTBIG y)
{
	INTBIG visiblelines;
	REGISTER EXPWINDOW *ew;

	ew = (EXPWINDOW *)us_explorertrackwindow->expwindow;
	if (x < us_explorertrackwindow->usehx - DISPLAYSLIDERSIZE) return(FALSE);
	visiblelines = (us_explorertrackwindow->usehy - us_explorertrackwindow->usely - DISPLAYSLIDERSIZE) /
		us_exploretextheight;
	switch (ew->sliderpart)
	{
		case 0:   /* down arrow clicked */
			if (y > us_explorertrackwindow->usely + DISPLAYSLIDERSIZE*2) return(FALSE);
			us_explorevpan(us_explorertrackwindow, 1);
			break;
		case 1:   /* clicked below thumb */
			if (y > us_explorertrackwindow->thumbly) return(FALSE);
			if (ew->totallines - ew->firstline <= visiblelines) return(FALSE);
			ew->firstline += visiblelines-1;
			if (ew->totallines - ew->firstline < visiblelines)
				ew->firstline = ew->totallines - visiblelines;
			us_exploreredisphandler(us_explorertrackwindow);
			break;
		case 2:   /* clicked on thumb (not done here) */
			break;
		case 3:   /* clicked above thumb */
			if (y < us_explorertrackwindow->thumbhy) return(FALSE);
			ew->firstline -= visiblelines-1;
			if (ew->firstline < 0) ew->firstline = 0;
			us_exploreredisphandler(us_explorertrackwindow);
			break;
		case 4:   /* up arrow clicked */
			if (y <= us_explorertrackwindow->usehy - DISPLAYSLIDERSIZE) return(FALSE);
			us_explorevpan(us_explorertrackwindow, -1);
			break;
	}
	return(FALSE);
}

void us_explorevpan(WINDOWPART *w, INTBIG dy)
{
	INTBIG visiblelines;
	REGISTER EXPWINDOW *ew;

	ew = (EXPWINDOW *)w->expwindow;
	visiblelines = (w->usehy - w->usely - DISPLAYSLIDERSIZE) / us_exploretextheight;
	if (dy > 0)
	{
		/* down shift */
		if (ew->totallines - ew->firstline <= visiblelines) return;
		ew->firstline++;
		us_exploreredisphandler(w);
	} else if (dy < 0)
	{
		/* up shift */
		if (ew->firstline > 0)
		{
			ew->firstline--;
			us_exploreredisphandler(w);
		}
	}
}

/*
 * Routine called when left or right slider arrows are clicked.
 */
BOOLEAN us_explorerarrowleft(INTBIG x, INTBIG y)
{
	INTBIG visiblechars, lx;
	REGISTER EXPWINDOW *ew;

	ew = (EXPWINDOW *)us_explorertrackwindow->expwindow;
	if (y > us_explorertrackwindow->usely + DISPLAYSLIDERSIZE) return(FALSE);
	lx = us_explorertrackwindow->uselx + DISPLAYSLIDERSIZE;
	visiblechars = (us_explorertrackwindow->usehx - lx - DISPLAYSLIDERSIZE) /
		us_exploretextwidth;
	switch (ew->sliderpart)
	{
		case 0:   /* left arrow clicked */
			if (x > lx + DISPLAYSLIDERSIZE) return(FALSE);
			us_explorehpan(us_explorertrackwindow, -1);
			break;
		case 1:   /* clicked to left of thumb */
			if (x > us_explorertrackwindow->thumblx) return(FALSE);
			ew->firstchar -= visiblechars - 5;
			if (ew->firstchar < 0) ew->firstchar = 0;
			us_exploreredisphandler(us_explorertrackwindow);
			break;
		case 2:   /* clicked on thumb (not done here) */
			break;
		case 3:   /* clicked to right of thumb */
			if (x < us_explorertrackwindow->thumbhx) return(FALSE);
			ew->firstchar += visiblechars - 5;
			if (ew->firstchar > MAXCELLEXPLHSCROLL)
				ew->firstchar = MAXCELLEXPLHSCROLL;
			us_exploreredisphandler(us_explorertrackwindow);
			break;
		case 4:   /* right arrow clicked */
			if (x < us_explorertrackwindow->usehx - DISPLAYSLIDERSIZE*2) return(FALSE);
			us_explorehpan(us_explorertrackwindow, 1);
			break;
	}
	return(FALSE);
}

void us_explorehpan(WINDOWPART *w, INTBIG dy)
{
	REGISTER EXPWINDOW *ew;

	ew = (EXPWINDOW *)w->expwindow;
	if (dy > 0)
	{
		/* left shift */
		if (ew->firstchar < MAXCELLEXPLHSCROLL)
		{
			ew->firstchar++;
			us_exploreredisphandler(w);
		}
	} else if (dy < 0)
	{
		/* right shift */
		if (ew->firstchar > 0)
		{
			ew->firstchar--;
			us_exploreredisphandler(w);
		}
	}
}

/*
 * Button handler for explorer window "w".  Button "but" was pushed at (x, y).
 */
void us_explorebuttonhandler(WINDOWPART *w, INTBIG but, INTBIG x, INTBIG y)
{
	REGISTER EXPLORERNODE *en;
	INTBIG visiblelines, i, boxsize;
	REGISTER POPUPMENUITEM *mi;
	POPUPMENU *pm, *cpm;
	BOOLEAN waitfordown;
	REGISTER INTBIG numcontexts;
	REGISTER EXPWINDOW *ew;

	ew = (EXPWINDOW *)w->expwindow;

	/* changes to the mouse-wheel are handled by the user interface */
	if (wheelbutton(but))
	{
		us_buttonhandler(w, but, x, y);
		return;
	}

	if (x >= w->usehx - DISPLAYSLIDERSIZE)
	{
		/* click in vertical slider */
		visiblelines = (w->usehy - w->usely - DISPLAYSLIDERSIZE) / us_exploretextheight;
		if (y < w->usely + DISPLAYSLIDERSIZE) return;
		if (y <= w->usely + DISPLAYSLIDERSIZE*2)
		{
			/* down arrow: shift base line (may repeat) */
			us_explorertrackwindow = w;
			ew->sliderpart = 0;
			trackcursor(FALSE, us_nullup, us_nullvoid, us_explorerarrowdown, us_nullchar,
				us_nullvoid, TRACKNORMAL);
			return;
		}
		if (y > w->usehy - DISPLAYSLIDERSIZE)
		{
			/* up arrow: shift base line (may repeat) */
			us_explorertrackwindow = w;
			ew->sliderpart = 4;
			trackcursor(FALSE, us_nullup, us_nullvoid, us_explorerarrowdown, us_nullchar,
				us_nullvoid, TRACKNORMAL);
			return;
		}
		if (y < w->thumbly)
		{
			/* below thumb: shift way down (may repeat) */
			us_explorertrackwindow = w;
			ew->sliderpart = 1;
			trackcursor(FALSE, us_nullup, us_nullvoid, us_explorerarrowdown, us_nullchar,
				us_nullvoid, TRACKNORMAL);
			return;
		}
		if (y > w->thumbhy)
		{
			/* above thumb: shift way up (may repeat) */
			us_explorertrackwindow = w;
			ew->sliderpart = 3;
			trackcursor(FALSE, us_nullup, us_nullvoid, us_explorerarrowdown, us_nullchar,
				us_nullvoid, TRACKNORMAL);
			return;
		}

		/* on the thumb: track its motion */
		if (visiblelines >= ew->totallines) return;
		us_explorertrackwindow = w;
		ew->deltasofar = 0;
		ew->initialthumb = w->thumbhy;
		us_vthumbbegin(y, w, w->usehx-DISPLAYSLIDERSIZE, w->usely+DISPLAYSLIDERSIZE,
			w->usehy, FALSE, us_evthumbtrackingtextcallback);
		trackcursor(FALSE, us_nullup, us_nullvoid, us_vthumbdown, us_nullchar,
			us_vthumbdone, TRACKNORMAL);
		return;
	}

	if (y <= w->usely + DISPLAYSLIDERSIZE)
	{
		/* click in horizontal slider */
		if (x <= w->uselx + DISPLAYSLIDERSIZE)
		{
			/* explorerwindow icon */
			el_curwindowpart = w;
			us_killcurrentwindow(TRUE);
			return;
		}
		if (x <= w->uselx + DISPLAYSLIDERSIZE*2)
		{
			/* left arrow: shift text (may repeat) */
			us_explorertrackwindow = w;
			ew->sliderpart = 0;
			trackcursor(FALSE, us_nullup, us_nullvoid, us_explorerarrowleft, us_nullchar,
				us_nullvoid, TRACKNORMAL);
			return;
		}
		if (x >= w->usehx - DISPLAYSLIDERSIZE*2)
		{
			/* up arrow: shift base line (may repeat) */
			us_explorertrackwindow = w;
			ew->sliderpart = 4;
			trackcursor(FALSE, us_nullup, us_nullvoid, us_explorerarrowleft, us_nullchar,
				us_nullvoid, TRACKNORMAL);
			return;
		}
		if (x < w->thumblx)
		{
			/* to left of thumb: shift way down (may repeat) */
			us_explorertrackwindow = w;
			ew->sliderpart = 1;
			trackcursor(FALSE, us_nullup, us_nullvoid, us_explorerarrowleft, us_nullchar,
				us_nullvoid, TRACKNORMAL);
			return;
		}
		if (x > w->thumbhx)
		{
			/* to right of thumb: shift way up (may repeat) */
			us_explorertrackwindow = w;
			ew->sliderpart = 3;
			trackcursor(FALSE, us_nullup, us_nullvoid, us_explorerarrowleft, us_nullchar,
				us_nullvoid, TRACKNORMAL);
			return;
		}

		/* on the thumb: track its motion */
		us_explorertrackwindow = w;
		ew->deltasofar = 0;
		ew->initialthumb = w->thumblx;
		us_hthumbbegin(x, w, w->usely+DISPLAYSLIDERSIZE, w->uselx,
			w->usehx-DISPLAYSLIDERSIZE, us_ehthumbtrackingtextcallback);
		trackcursor(FALSE, us_nullup, us_nullvoid, us_hthumbdown, us_nullchar,
			us_hthumbdone, TRACKNORMAL);
		return;
	}

	/* deselect */
	if (ew->nodeselected != NOEXPLORERNODE) us_highlightexplorernode(w);
	ew->nodeselected = NOEXPLORERNODE;

	boxsize = us_exploretextheight * 2 / 3;
	for(en = ew->nodeused; en != NOEXPLORERNODE; en = en->nextexplorernode)
	{
		if ((en->flags&EXNODESHOWN) == 0) continue;
		if (y < en->y || y >= en->y + us_exploretextheight) continue;
		if (x >= en->x - boxsize - boxsize/2 && x <= en->x - boxsize/2)
		{
			/* hit the box to the left of the name */
			us_explorertoggleopenness(w, en);
			return;
		}

		if (x >= en->x && x <= en->x+en->textwidth)
		{
			/* hit the name of a cell/library: highlight it */
			ew->nodeselected = en;
			us_highlightexplorernode(w);

			if (doublebutton(but))
			{
				/* do the first context function */
				us_explorerdofunction(w, en, 0);
			}

			if (contextbutton(but))
			{
				/* show the context menu */
				pm = (POPUPMENU *)emalloc(sizeof(POPUPMENU), el_tempcluster);
				if (pm == 0) return;
				pm->name = x_("noname");
				mi = (POPUPMENUITEM *)emalloc((5 * (sizeof (POPUPMENUITEM))), el_tempcluster);
				if (mi == 0) return;
				pm->list = mi;
				for(i=0; i<5; i++)
				{
					mi[i].valueparse = NOCOMCOMP;
					mi[i].response = NOUSERCOM;
					mi[i].value = 0;
					mi[i].maxlen = -1;
				}
				numcontexts = 0;
				switch (en->enodetype)
				{
					case EXNODETYPELIBRARY:
						mi[numcontexts++].attribute = _("Make Library Current");
						mi[numcontexts++].attribute = _("Delete Library");
						mi[numcontexts++].attribute = _("Rename Library");
						mi[numcontexts++].attribute = _("Open Entire Tree below Here");
						mi[numcontexts++].attribute = _("Close Entire Tree below Here");
						break;
					case EXNODETYPECELL:
						mi[numcontexts++].attribute = _("Edit Cell");
						mi[numcontexts++].attribute = _("Delete Cell");
						mi[numcontexts++].attribute = _("Rename Cell");
						mi[numcontexts++].attribute = _("Open Entire Tree below Here");
						mi[numcontexts++].attribute = _("Close Entire Tree below Here");
						break;
					case EXNODETYPENODE:
						mi[numcontexts++].attribute = _("Show Node");
						break;
					case EXNODETYPEARC:
						mi[numcontexts++].attribute = _("Show Arc");
						break;
					case EXNODETYPENETWORK:
						mi[numcontexts++].attribute = _("Show Network");
						break;
					case EXNODETYPEEXPORT:
						mi[numcontexts++].attribute = _("Show Export");
						break;
					case EXNODETYPEERROR:
						mi[numcontexts++].attribute = _("Show Error");
						break;
					case EXNODETYPEERRORGEOM:
						mi[numcontexts++].attribute = _("Show Object");
						break;
					case EXNODETYPESIMBRANCH:
						mi[numcontexts++].attribute = _("Open Entire Tree below Here");
						mi[numcontexts++].attribute = _("Close Entire Tree below Here");
						break;
					case EXNODETYPESIMLEAF:
						mi[numcontexts++].attribute = _("Overlay to Waveform Window");
						mi[numcontexts++].attribute = _("Add to Waveform Window");
						break;
					case EXNODETYPELABEL:
						mi[numcontexts++].attribute = _("Open/Close");
						mi[numcontexts++].attribute = _("Open Entire Tree below Here");
						mi[numcontexts++].attribute = _("Close Entire Tree below Here");
						break;
				}
				pm->total = numcontexts;
				cpm = pm;
				waitfordown = FALSE;
				mi = us_popupmenu(&cpm, &waitfordown, FALSE, -1, -1, 0);
				if (mi != 0 && mi != NOPOPUPMENUITEM)
				{
					i = mi - pm->list;
					us_explorerdofunction(w, en, i);
				}
				efree((CHAR *)pm->list);
				efree((CHAR *)pm);
				return;
			}
		}
	}
}

/*
 * Routine to toggle the open/closed state of explorer node "en" in window "w".
 */
void us_explorertoggleopenness(WINDOWPART *w, EXPLORERNODE *en)
{
	REGISTER EXPWINDOW *ew;

	ew = (EXPWINDOW *)w->expwindow;
	if ((en->flags & EXNODEOPEN) != 0) en->flags &= ~EXNODEOPEN; else
	{
		if ((en->flags&EXNODEPLACEHOLDER) != 0)
		{
			if (en == ew->nodehierarchy)
			{
				us_buildexplorersturcthierarchical(ew, en);
			} else if (en == ew->nodecontents)
			{
				us_buildexplorersturctcontents(ew, en);
			} else if (en == ew->nodeerrors)
			{
				us_buildexplorersturcterrors(ew, en);
			} else if (en == ew->nodesimulation)
			{
				us_buildexplorersturctsimulation(w, ew, en);
			} else if (en->enodetype == EXNODETYPECELL)
			{
				us_buildexplorernodecontents(ew, (NODEPROTO *)en->addr, en);
			}
		}
		en->flags |= EXNODEOPEN;
	}
	us_exploreredisphandler(w);
}

/*
 * Routine to perform function "funct" on explorer node "en".
 * The function numbers are from the context menu that can be popped-up on that
 * node.  Double-clicking does the first function.
 */
void us_explorerdofunction(WINDOWPART *w, EXPLORERNODE *en, INTBIG funct)
{
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp;
	REGISTER NETWORK *net;
	REGISTER BOOLEAN found, first, overlay;
	REGISTER INTBIG i;
	REGISTER void *infstr, *altinfstr;
	REGISTER CHAR *pt;
	REGISTER WINDOWPART *ow;
	INTBIG lx, hx, ly, hy;
	CHAR *strname;
	REGISTER EXPWINDOW *ew;

	ew = (EXPWINDOW *)w->expwindow;
	switch (en->enodetype)
	{
		case EXNODETYPELIBRARY:
			switch (funct)
			{
				case 0:		/* make this the current library */
					us_switchtolibrary((LIBRARY *)en->addr);
					return;
				case 1:		/* delete the library */
					us_explorerdelete(en);
					return;
				case 2:		/* rename the library */
					us_explorerrename(en, w);
					return;
				case 3:		/* open entire tree below here */
					us_explorrecurseexpand(w, ew, en, TRUE);
					us_exploreredisphandler(w);
					return;
				case 4:		/* close entire tree below here */
					us_explorrecurseexpand(w, ew, en, FALSE);
					us_exploreredisphandler(w);
					return;
			}
			break;
		case EXNODETYPECELL:
			switch (funct)
			{
				case 0:		/* show this cell */
					np = (NODEPROTO *)en->addr;
					for(ow = el_topwindowpart; ow != NOWINDOWPART; ow = ow->nextwindowpart)
					{
						if (ow == w) continue;
						if (estrcmp(w->location, x_("entire")) != 0)
							if (ow->frame != w->frame) continue;
						break;
					}
					us_fullview(np, &lx, &hx, &ly, &hy);
					if (ow == NOWINDOWPART)
					{
						/* no other window can be found: create one */
						us_switchtocell(np, lx, hx, ly, hy, NONODEINST, NOPORTPROTO, TRUE, FALSE, FALSE);
					} else
					{
						us_highlightwindow(ow, FALSE);
						us_switchtocell(np, lx, hx, ly, hy, NONODEINST, NOPORTPROTO, FALSE, FALSE, FALSE);
					}
					return;
				case 1:		/* delete this cell */
					us_explorerdelete(en);
					return;
				case 2:		/* rename the cell */
					us_explorerrename(en, w);
					return;
				case 3:		/* open entire tree below here */
					us_explorrecurseexpand(w, ew, en, TRUE);
					us_exploreredisphandler(w);
					return;
				case 4:		/* close entire tree below here */
					us_explorrecurseexpand(w, ew, en, FALSE);
					us_exploreredisphandler(w);
					return;
			}
			break;
		case EXNODETYPENODE:
			switch (funct)
			{
				case 0:		/* show this node */
					ni = (NODEINST *)en->addr;
					infstr = initinfstr();
					formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0"),
						describenodeproto(ni->parent), (INTBIG)ni->geom);
					us_setmultiplehighlight(returninfstr(infstr), FALSE);
					us_showallhighlight();
					us_endchanges(NOWINDOWPART);
					return;
			}
			break;
		case EXNODETYPEARC:
			switch (funct)
			{
				case 0:		/* show this arc */
					ai = (ARCINST *)en->addr;
					infstr = initinfstr();
					formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0"),
						describenodeproto(ai->parent), (INTBIG)ai->geom);
					us_setmultiplehighlight(returninfstr(infstr), FALSE);
					us_showallhighlight();
					us_endchanges(NOWINDOWPART);
					return;
			}
			break;
		case EXNODETYPENETWORK:
			switch (funct)
			{
				case 0:		/* show this network */
					net = (NETWORK *)en->addr;
					np = net->parent;
					infstr = initinfstr();
					first = FALSE;
					for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
					{
						found = FALSE;
						if (ai->network == net) found = TRUE; else
						{
							if (net->buswidth > 1)
							{
								for(i=0; i<net->buswidth; i++)
									if (net->networklist[i] == ai->network) break;
								if (i < net->buswidth) found = TRUE;
							}
							if (ai->network->buswidth > 1)
							{
								for(i=0; i<ai->network->buswidth; i++)
									if (ai->network->networklist[i] == net) break;
								if (i < ai->network->buswidth) found = TRUE;
							}
						}
						if (!found) continue;
						if (first) addtoinfstr(infstr, '\n');
						first = TRUE;
						formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0"),
							describenodeproto(np), (INTBIG)ai->geom);
					}
					for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
					{
						if (pp->network != net) continue;
						if (first) addtoinfstr(infstr, '\n');
						first = TRUE;
						formatinfstr(infstr, x_("CELL=%s TEXT=0%lo;0%lo;0"),
							describenodeproto(np), (INTBIG)pp->subnodeinst->geom,
								(INTBIG)pp);
					}
					us_setmultiplehighlight(returninfstr(infstr), FALSE);
					us_showallhighlight();
					us_endchanges(NOWINDOWPART);
					return;
			}
			break;
		case EXNODETYPEEXPORT:
			switch (funct)
			{
				case 0:		/* show this export */
					pp = (PORTPROTO *)en->addr;
					infstr = initinfstr();
					formatinfstr(infstr, x_("CELL=%s TEXT=0%lo;0%lo;0"),
						describenodeproto(pp->parent), (INTBIG)pp->subnodeinst->geom,
							(INTBIG)pp);
					us_setmultiplehighlight(returninfstr(infstr), FALSE);
					us_showallhighlight();
					us_endchanges(NOWINDOWPART);
					return;
			}
			break;
		case EXNODETYPEERROR:
			switch (funct)
			{
				case 0:
					reporterror((void *)en->addr);
					return;
			}
			break;
		case EXNODETYPEERRORGEOM:
			switch (funct)
			{
				case 0:
					us_clearhighlightcount();
					showerrorgeom((void *)en->addr);
					return;
			}
			break;
		case EXNODETYPESIMBRANCH:
			switch (funct)
			{
				case 0:		/* open entire tree below here */
					us_explorrecurseexpand(w, ew, en, TRUE);
					us_exploreredisphandler(w);
					return;
				case 1:		/* close entire tree below here */
					us_explorrecurseexpand(w, ew, en, FALSE);
					us_exploreredisphandler(w);
					return;
			}
			break;
		case EXNODETYPESIMLEAF:
			switch (funct)
			{
				case 0:		/* overlay signal to waveform */
				case 1:		/* add signal to waveform */
					infstr = initinfstr();
					for(;;)
					{
						if (en->enodetype != EXNODETYPESIMBRANCH &&
							en->enodetype != EXNODETYPESIMLEAF) break;
						altinfstr = initinfstr();
						addstringtoinfstr(altinfstr, (CHAR *)en->addr);
						pt = returninfstr(infstr);
						if (*pt != 0) formatinfstr(altinfstr, x_("%s%s"), sim_signalseparator(), pt);
						infstr = altinfstr;

						en = en->parent;
						if (en == NOEXPLORERNODE) break;
					}
					allocstring(&strname, returninfstr(infstr), el_tempcluster);
					if (funct == 0) overlay = TRUE; else overlay = FALSE;
					sim_addsignal(w, strname, overlay);
					efree(strname);
					return;
			}
			break;
		case EXNODETYPELABEL:
			switch (funct)
			{
				case 0:		/* toggle open/closed state */
					us_explorertoggleopenness(w, en);
					return;
				case 1:		/* open entire tree below here */
					us_explorrecurseexpand(w, ew, en, TRUE);
					us_exploreredisphandler(w);
					return;
				case 2:		/* close entire tree below here */
					us_explorrecurseexpand(w, ew, en, FALSE);
					us_exploreredisphandler(w);
					return;
			}
			break;
	}
}

/*
 * Routine to recursively open or close all nodes below "en".  Opens if "expand" is TRUE,
 * closes if "expand" is FALSE.
 */
void us_explorrecurseexpand(WINDOWPART *w, EXPWINDOW *ew, EXPLORERNODE *en, BOOLEAN expand)
{
	REGISTER EXPLORERNODE *suben;

	if (expand)
	{
		if ((en->flags&EXNODEPLACEHOLDER) != 0)
		{
			if (en == ew->nodehierarchy)
			{
				us_buildexplorersturcthierarchical(ew, en);
			} else if (en == ew->nodecontents)
			{
				us_buildexplorersturctcontents(ew, en);
			} else if (en == ew->nodeerrors)
			{
				us_buildexplorersturcterrors(ew, en);
			} else if (en == ew->nodesimulation)
			{
				us_buildexplorersturctsimulation(w, ew, en);
			} else if (en->enodetype == EXNODETYPECELL)
			{
				us_buildexplorernodecontents(ew, (NODEPROTO *)en->addr, en);
			}
		}
		en->flags |= EXNODEOPEN;
	} else
		en->flags &= ~EXNODEOPEN;
	for(suben = en->subexplorernode; suben != NOEXPLORERNODE; suben = suben->nextsubexplorernode)
		us_explorrecurseexpand(w, ew, suben, expand);
}

void us_evthumbtrackingtextcallback(INTBIG delta)
{
	REGISTER INTBIG point, thumbarea, thumbpos, visiblelines;
	REGISTER EXPWINDOW *ew;

	ew = (EXPWINDOW *)us_explorertrackwindow->expwindow;
	ew->deltasofar += delta;
	visiblelines = (us_explorertrackwindow->usehy - us_explorertrackwindow->usely + DISPLAYSLIDERSIZE) /
		us_exploretextheight;
	thumbpos = ew->initialthumb + ew->deltasofar;
	thumbarea = us_explorertrackwindow->usehy - us_explorertrackwindow->usely - DISPLAYSLIDERSIZE*3;
	point = (us_explorertrackwindow->usehy - DISPLAYSLIDERSIZE - thumbpos) * ew->totallines / thumbarea;
	if (point < 0) point = 0;
	if (point > ew->totallines-visiblelines) point = ew->totallines-visiblelines;
	ew->firstline = point;
	us_exploreredisphandler(us_explorertrackwindow);
}

void us_ehthumbtrackingtextcallback(INTBIG delta)
{
	REGISTER INTBIG point, thumbarea, thumbpos, thumbsize, visiblechars, lx;
	REGISTER EXPWINDOW *ew;

	ew = (EXPWINDOW *)us_explorertrackwindow->expwindow;
	lx = us_explorertrackwindow->uselx + DISPLAYSLIDERSIZE;
	ew->deltasofar += delta;
	visiblechars = (us_explorertrackwindow->usehx - lx - DISPLAYSLIDERSIZE) /
		us_exploretextwidth;
	thumbpos = ew->initialthumb + ew->deltasofar;
	thumbsize = (us_explorertrackwindow->usehx - lx - DISPLAYSLIDERSIZE*3) / 20;
	thumbarea = us_explorertrackwindow->usehx - lx - DISPLAYSLIDERSIZE*3 - thumbsize;
	point = thumbpos * MAXCELLEXPLHSCROLL / thumbarea - lx -
		DISPLAYSLIDERSIZE;
	if (point < 0) point = 0;
	if (point > MAXCELLEXPLHSCROLL) point = MAXCELLEXPLHSCROLL;
	ew->firstchar = point;
	us_exploreredisphandler(us_explorertrackwindow);
}

BOOLEAN us_explorecharhandler(WINDOWPART *w, INTSML cmd, INTBIG special)
{
	REGISTER EXPWINDOW *ew;

	ew = (EXPWINDOW *)w->expwindow;
	if (cmd == DELETEKEY || cmd == BACKSPACEKEY)
	{
		/* delete selected cell */
		if (ew->nodeselected == NOEXPLORERNODE)
		{
			ttyputerr(_("Select a cell name before deleting it"));
			return(FALSE);
		}
		us_explorerdelete(ew->nodeselected);
		return(FALSE);
	}
	return(us_charhandler(w, cmd, special));
}

/*
 * Routine to delete the object pointed to by explorer node "en" (either a
 * cell or a library).
 */
void us_explorerdelete(EXPLORERNODE *en)
{
	REGISTER NODEPROTO *np;
	extern COMCOMP us_yesnop;
	REGISTER INTBIG i;
	REGISTER void *infstr;
	CHAR *pars[3];

	if (en->enodetype == EXNODETYPELIBRARY)
	{
		pars[0] = x_("kill");
		pars[1] = ((LIBRARY *)en->addr)->libname;
		us_library(2, pars);
		return;
	}
	if (en->enodetype == EXNODETYPECELL)
	{
		np = (NODEPROTO *)en->addr;
		if (np->firstinst != NONODEINST)
		{
			ttyputerr(_("Can only delete top-level cells"));
			return;
		}
		infstr = initinfstr();
		formatinfstr(infstr, _("Are you sure you want to delete cell %s"),
			describenodeproto(np));
		i = ttygetparam(returninfstr(infstr), &us_yesnop, 3, pars);
		if (i == 1)
		{
			if (pars[0][0] == 'y')
			{
				us_clearhighlightcount();
				pars[0] = describenodeproto(np);
				us_killcell(1, pars);
			}
		}
		return;
	}
}

/*
 * Routine to rename the object pointed to by explorer node "en" (either a
 * cell or a library).
 */
void us_explorerrename(EXPLORERNODE *en, WINDOWPART *w)
{
	REGISTER LIBRARY *savelib;

	if (en->enodetype == EXNODETYPELIBRARY)
	{
		savelib = el_curlib;
		selectlibrary((LIBRARY *)en->addr, FALSE);
		us_renamedlog(VLIBRARY);
		selectlibrary(savelib, FALSE);
		us_exploreredisphandler(w);
		return;
	}
	if (en->enodetype == EXNODETYPECELL)
	{
		savelib = el_curlib;
		selectlibrary(((NODEPROTO *)en->addr)->lib, FALSE);
		us_renamedlog(VNODEPROTO);
		selectlibrary(savelib, FALSE);
		us_exploreredisphandler(w);
		return;
	}
}

#define EXPLORERNODECHUNKSIZE 50

/*
 * Routine to allocate a new "explorer node".  Returs NOEXPLORERNODE on error.
 */
EXPLORERNODE *us_allocexplorernode(EXPWINDOW *ew)
{
	REGISTER EXPLORERNODE *en, *enlist;
	REGISTER INTBIG i;

	if (us_explorernodefree == NOEXPLORERNODE)
	{
		/* free list is empty: load it up */
		enlist = (EXPLORERNODE *)emalloc(EXPLORERNODECHUNKSIZE * (sizeof (EXPLORERNODE)), us_tool->cluster);
		if (enlist == 0) return(NOEXPLORERNODE);
		for(i=0; i<EXPLORERNODECHUNKSIZE; i++)
		{
			en = &enlist[i];
			if (i == 0) en->allocated = TRUE; else
				en->allocated = FALSE;
			us_freeexplorernode(en);
		}
	}

	/* take one off of the free list */
	en = us_explorernodefree;
	us_explorernodefree = en->nextexplorernode;

	/* initialize it */
	en->subexplorernode = NOEXPLORERNODE;
	en->nextsubexplorernode = NOEXPLORERNODE;
	en->addr = 0;
	en->enodetype = EXNODETYPELABEL;
	en->flags = 0;
	en->count = 0;

	/* put into the global linked list */
	en->nextexplorernode = ew->nodeused;
	ew->nodeused = en;
	return(en);
}

/*
 * Routine to free explorer node "en" to the pool of unused nodes.
 */
void us_freeexplorernode(EXPLORERNODE *en)
{
	if (en->enodetype == EXNODETYPESIMBRANCH || en->enodetype == EXNODETYPESIMLEAF)
		efree((CHAR *)en->addr);
	en->nextexplorernode = us_explorernodefree;
	us_explorernodefree = en;
}

/*********************************** ARC SUPPORT ***********************************/

/*
 * routine to modify the arcs in list "list".  The "change" field has the
 * following meaning:
 *  change=0   make arc rigid
 *  change=1   make arc un-rigid
 *  change=2   make arc fixed-angle
 *  change=3   make arc not fixed-angle
 *  change=4   make arc slidable
 *  change=5   make arc nonslidable
 *  change=6   make arc temporarily rigid
 *  change=7   make arc temporarily un-rigid
 *  change=8   make arc ends extend by half width
 *  change=9   make arc ends not extend by half width
 *  change=10  make arc directional
 *  change=11  make arc not directional
 *  change=12  make arc negated
 *  change=13  make arc not negated
 *  change=14  make arc skip the tail end
 *  change=15  make arc not skip the tail end
 *  change=16  make arc skip the head end
 *  change=17  make arc not skip the head end
 *  change=18  reverse ends of the arc
 *  change=19  clear arc constraints
 *  change=20  print arc constraints
 *  change=21  set special arc constraint from "prop"
 *  change=22  add to special arc constraint from "prop"
 *  change=23  toggle arc rigid
 *  change=24  toggle arc fixed-angle
 *  change=25  toggle arc slidable
 *  change=26  toggle arc ends extend
 *  change=27  toggle arc directional
 *  change=28  toggle arc negated
 *  change=29  toggle arc skip the tail
 *  change=30  toggle arc skip the head
 * If "redraw" is nonzero then the arcs are re-drawn to reflect the change.
 * The routine returns the number of arcs that were modified (-1 if an error
 * occurred).
 */
INTBIG us_modarcbits(INTBIG change, BOOLEAN redraw, CHAR *prop, GEOM **list)
{
	REGISTER INTBIG total, affected;
	REGISTER INTBIG newvalue;
	REGISTER ARCINST *ai;

	/* must be a cell in the window */
	if (us_needcell() == NONODEPROTO) return(-1);

	/* run through the list */
	affected = 0;
	for(total=0; list[total] != NOGEOM; total++)
	{
		if (list[total]->entryisnode) continue;
		ai = list[total]->entryaddr.ai;

		/* notify system that arc is to be re-drawn */
		if (redraw) startobjectchange((INTBIG)ai, VARCINST);

		/* change the arc state bits */
		switch (change)
		{
			case 0:			/* arc rigid */
				if (!(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPERIGID, 0))
					affected++;
				break;
			case 1:			/* arc un-rigid */
				if (!(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPEUNRIGID, 0))
					affected++;
				break;
			case 2:			/* arc fixed-angle */
				if (!(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPEFIXEDANGLE, 0))
					affected++;
				break;
			case 3:			/* arc not fixed-angle */
				if (!(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPENOTFIXEDANGLE, 0))
					affected++;
				break;
			case 4:			/* arc slidable */
				if (!(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPESLIDABLE, 0))
					affected++;
				break;
			case 5:			/* arc nonslidable */
				if (!(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPENOTSLIDABLE, 0))
					affected++;
				break;
			case 6:			/* arc temporarily rigid */
				if (!(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPETEMPRIGID, 0))
					affected++;
				break;
			case 7:			/* arc temporarily un-rigid */
				if (!(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPETEMPUNRIGID, 0))
					affected++;
				break;
			case 8:			/* arc ends extend by half width */
				if ((ai->userbits&NOEXTEND) != 0) affected++;
				(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits & ~NOEXTEND, VINTEGER);
				break;
			case 9:			/* arc ends not extend by half width */
				if ((ai->userbits&NOEXTEND) == 0) affected++;
				(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits|NOEXTEND, VINTEGER);
				break;
			case 10:			/* arc directional */
				if ((ai->userbits&ISDIRECTIONAL) == 0) affected++;
				(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits|ISDIRECTIONAL, VINTEGER);
				break;
			case 11:			/* arc not directional */
				if ((ai->userbits&ISDIRECTIONAL) != 0) affected++;
				(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits & ~ISDIRECTIONAL, VINTEGER);
				break;
			case 12:			/* arc negated */
				if ((ai->userbits&ISNEGATED) == 0) affected++;
				newvalue = ai->userbits | ISNEGATED;

				/* don't put the negation circle on a pin */
				if (((ai->end[0].nodeinst->proto->userbits&NFUNCTION) >> NFUNCTIONSH) == NPPIN &&
					((ai->end[1].nodeinst->proto->userbits&NFUNCTION) >> NFUNCTIONSH) != NPPIN)
						newvalue |= REVERSEEND;

				/* prefer output negation to input negation */
				if ((ai->end[0].portarcinst->proto->userbits&STATEBITS) == INPORT &&
					(ai->end[1].portarcinst->proto->userbits&STATEBITS) == OUTPORT)
						newvalue |= REVERSEEND;

				(void)setval((INTBIG)ai, VARCINST, x_("userbits"), newvalue, VINTEGER);
				break;
			case 13:			/* arc not negated */
				if ((ai->userbits&ISNEGATED) != 0) affected++;
				(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits & ~ISNEGATED, VINTEGER);
				break;
			case 14:			/* arc skip the tail end */
				if ((ai->userbits&NOTEND0) == 0) affected++;
				(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits|NOTEND0, VINTEGER);
				break;
			case 15:			/* arc not skip the tail end */
				if ((ai->userbits&NOTEND0) != 0) affected++;
				(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits & ~NOTEND0, VINTEGER);
				break;
			case 16:			/* arc skip the head end */
				if ((ai->userbits&NOTEND1) == 0) affected++;
				(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits|NOTEND1, VINTEGER);
				break;
			case 17:			/* arc not skip the head end */
				if ((ai->userbits&NOTEND1) != 0) affected++;
				(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits & ~NOTEND1, VINTEGER);
				break;
			case 18:			/* reverse ends of the arc */
				affected++;
				newvalue = (ai->userbits & ~REVERSEEND) | ((~ai->userbits) & REVERSEEND);
				(void)setval((INTBIG)ai, VARCINST, x_("userbits"), newvalue, VINTEGER);
				break;
			case 19:			/* clear special arc properties */
				if (!(*el_curconstraint->setobject)((INTBIG)ai, VARCINST,
					CHANGETYPEFIXEDANGLE, (INTBIG)x_(""))) affected++;
				break;
			case 20:			/* print special arc properties */
				if (!(*el_curconstraint->setobject)((INTBIG)ai, VARCINST,
					CHANGETYPENOTFIXEDANGLE, (INTBIG)x_(""))) affected++;
				break;
			case 21:			/* set special arc properties */
				if (!(*el_curconstraint->setobject)((INTBIG)ai, VARCINST,
					CHANGETYPERIGID, (INTBIG)prop)) affected++;
				break;
			case 22:			/* add to special arc properties */
				if (!(*el_curconstraint->setobject)((INTBIG)ai, VARCINST,
					CHANGETYPEUNRIGID, (INTBIG)prop)) affected++;
				break;
			case 23:			/* arc toggle rigid */
				if ((ai->userbits&FIXED) != 0)
					(void)(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPEUNRIGID, 0); else
						(void)(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPERIGID, 0);
				affected++;
				break;
			case 24:			/* arc toggle fixed-angle */
				if ((ai->userbits&FIXANG) != 0)
					(void)(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPENOTFIXEDANGLE, 0); else
						(void)(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPEFIXEDANGLE, 0);
				affected++;
				break;
			case 25:			/* arc toggle slidable */
				if ((ai->userbits&CANTSLIDE) != 0)
					(void)(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPESLIDABLE, 0); else
						(void)(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPENOTSLIDABLE, 0);
				affected++;
				break;
			case 26:			/* arc toggle ends extend */
				if ((ai->userbits&NOEXTEND) != 0) newvalue = ai->userbits & ~NOEXTEND; else
					newvalue = ai->userbits | NOEXTEND;
				(void)setval((INTBIG)ai, VARCINST, x_("userbits"), newvalue, VINTEGER);
				affected++;
				break;
			case 27:			/* arc toggle directional */
				if ((ai->userbits&ISDIRECTIONAL) != 0) newvalue = ai->userbits & ~ISDIRECTIONAL; else
					newvalue = ai->userbits | ISDIRECTIONAL;
				(void)setval((INTBIG)ai, VARCINST, x_("userbits"), newvalue, VINTEGER);
				affected++;
				break;
			case 28:			/* arc toggle negated */
				if ((ai->userbits&ISNEGATED) != 0) newvalue = ai->userbits & ~ISNEGATED; else
					newvalue = ai->userbits | ISNEGATED;

				if ((newvalue&ISNEGATED) != 0)
				{
					/* don't put the negation circle on a pin */
					if (((ai->end[0].nodeinst->proto->userbits&NFUNCTION) >> NFUNCTIONSH) == NPPIN &&
						((ai->end[1].nodeinst->proto->userbits&NFUNCTION) >> NFUNCTIONSH) != NPPIN)
							newvalue |= REVERSEEND;

					/* prefer output negation to input negation */
					if ((ai->end[0].portarcinst->proto->userbits&STATEBITS) == INPORT &&
						(ai->end[1].portarcinst->proto->userbits&STATEBITS) == OUTPORT)
							newvalue |= REVERSEEND;
				}

				(void)setval((INTBIG)ai, VARCINST, x_("userbits"), newvalue, VINTEGER);
				affected++;
				break;
			case 29:			/* arc toggle skip the tail */
				if ((ai->userbits&NOTEND0) != 0) newvalue = ai->userbits & ~NOTEND0; else
					newvalue = ai->userbits | NOTEND0;
				(void)setval((INTBIG)ai, VARCINST, x_("userbits"), newvalue, VINTEGER);
				affected++;
				break;
			case 30:			/* arc toggle skip the head */
				if ((ai->userbits&NOTEND1) != 0) newvalue = ai->userbits & ~NOTEND1; else
					newvalue = ai->userbits | NOTEND1;
				(void)setval((INTBIG)ai, VARCINST, x_("userbits"), newvalue, VINTEGER);
				affected++;
				break;
		}

		/* notify the system that the arc is done and can be re-drawn */
		if (redraw) endobjectchange((INTBIG)ai, VARCINST);
	}

	return(affected);
}

/*
 * routine to set the arc name of arc "ai" to the name "name".  If "name" is
 * zero, remove the name
 */
void us_setarcname(ARCINST *ai, CHAR *name)
{
	REGISTER VARIABLE *var;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	startobjectchange((INTBIG)ai, VARCINST);
	if (name == 0) (void)delvalkey((INTBIG)ai, VARCINST, el_arc_name_key); else
	{
		TDCLEAR(descript);
		defaulttextdescript(descript, ai->geom);
		var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
		if (var != NOVARIABLE)
		{
			if ((var->type&VDISPLAY) != 0)
				TDCOPY(descript, var->textdescript);
		}
		var = setvalkey((INTBIG)ai, VARCINST, el_arc_name_key, (INTBIG)name, VSTRING|VDISPLAY);
		if (var == NOVARIABLE) return;
		TDCOPY(var->textdescript, descript);

		/* for zero-width arcs, adjust the location */
		if (ai->width == 0)
		{
			if (ai->end[0].ypos == ai->end[1].ypos)
			{
				/* zero-width horizontal arc has name above */
				TDSETPOS(descript, VTPOSUP);
				modifydescript((INTBIG)ai, VARCINST, var, descript);
			} else if (ai->end[0].xpos == ai->end[1].xpos)
			{
				/* zero-width vertical arc has name to right */
				TDSETPOS(descript, VTPOSRIGHT);
				modifydescript((INTBIG)ai, VARCINST, var, descript);
			}
		}
	}
	endobjectchange((INTBIG)ai, VARCINST);
}

/*
 * routine to determine the "userbits" to use for an arc of type "ap".
 */
INTBIG us_makearcuserbits(ARCPROTO *ap)
{
	REGISTER INTBIG bits, protobits;
	REGISTER VARIABLE *var;

	var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_arcstylekey);
	if (var != NOVARIABLE) protobits = var->addr; else
	{
		var = getvalkey((INTBIG)ap, VARCPROTO, VINTEGER, us_arcstylekey);
		if (var != NOVARIABLE) protobits = var->addr; else
			protobits = ap->userbits;
	}
	bits = 0;
	if ((protobits&WANTFIXANG) != 0)      bits |= FIXANG;
	if ((protobits&WANTFIX) != 0)         bits |= FIXED;
	if ((protobits&WANTCANTSLIDE) != 0)   bits |= CANTSLIDE;
	if ((protobits&WANTNOEXTEND) != 0)    bits |= NOEXTEND;
	if ((protobits&WANTNEGATED) != 0)     bits |= ISNEGATED;
	if ((protobits&WANTDIRECTIONAL) != 0) bits |= ISDIRECTIONAL;
	return(bits);
}

/*
 * Routine to recompute the "FAR TEXT" bit on arc "ai".
 */
void us_computearcfartextbit(ARCINST *ai)
{
	REGISTER INTBIG i, sizelimit;
	INTBIG xw, yw;
	REGISTER VARIABLE *var;
	REGISTER NODEPROTO *savecell;
	HIGHLIGHT high;

	/* must have a window in which to work */
	if (el_curwindowpart == NOWINDOWPART) return;
	ai->userbits &= ~AHASFARTEXT;
	high.status = HIGHTEXT;
	high.cell = ai->parent;
	high.fromgeom = ai->geom;
	high.frompoint = 0;
	high.fromport = NOPORTPROTO;
	high.fromvarnoeval = NOVARIABLE;
	sizelimit = lambdaofarc(ai) * FARTEXTLIMIT * 2;
	for(i=0; i<ai->numvar; i++)
	{
		var = &ai->firstvar[i];
		if ((var->type&VDISPLAY) == 0) continue;
		if (abs(TDGETXOFF(var->textdescript)) >= FARTEXTLIMIT*4 ||
			abs(TDGETYOFF(var->textdescript)) >= FARTEXTLIMIT*4)
		{
			ai->userbits |= AHASFARTEXT;
			return;
		}
		high.fromvar = var;
		savecell = el_curwindowpart->curnodeproto;
		el_curwindowpart->curnodeproto = ai->parent;
		us_gethightextsize(&high, &xw, &yw, el_curwindowpart);
		el_curwindowpart->curnodeproto = savecell;
		if (xw > sizelimit || yw > sizelimit)
		{
			ai->userbits |= AHASFARTEXT;
			return;
		}
	}
}

/*
 * Routine to return the curvature for arc "ai" that will allow it to
 * curve about (xcur, ycur), a center point.
 */
INTBIG us_curvearcaboutpoint(ARCINST *ai, INTBIG xcur, INTBIG ycur)
{
	INTBIG x1, y1, x2, y2, ix, iy;
	REGISTER INTBIG acx, acy, r;
	REGISTER INTBIG ang;

	/* get true center of arc through cursor */
	ang = ((ai->userbits&AANGLE) >> AANGLESH) * 10;
	acx = (ai->end[0].xpos + ai->end[1].xpos) / 2;
	acy = (ai->end[0].ypos + ai->end[1].ypos) / 2;
	(void)intersect(xcur, ycur, ang, acx, acy, (ang+900)%3600, &ix, &iy);
	r = computedistance(ai->end[0].xpos, ai->end[0].ypos, ix, iy);

	/* now see if this point will be re-created */
	(void)findcenters(r, ai->end[0].xpos,ai->end[0].ypos,
		ai->end[1].xpos, ai->end[1].ypos, ai->length, &x1,&y1, &x2,&y2);
	if (abs(x1-ix)+abs(y1-iy) < abs(x2-ix)+abs(y2-iy)) r = -r;
	return(r);
}

/*
 * Routine to return the curvature for arc "ai" that will allow it to
 * curve through (xcur, ycur), an edge point.
 */
INTBIG us_curvearcthroughpoint(ARCINST *ai, INTBIG xcur, INTBIG ycur)
{
	INTBIG x1, y1, x2, y2;
	REGISTER INTBIG r0x, r0y, r1x, r1y, r2x, r2y, rpx, rpy, r02x, r02y, rcx, rcy, r;
	REGISTER float u, v, t;

	r0x = ai->end[0].xpos;   r0y = ai->end[0].ypos;
	r1x = ai->end[1].xpos;   r1y = ai->end[1].ypos;
	r2x = xcur;              r2y = ycur;
	r02x = r2x-r0x;          r02y = r2y-r0y;
	rpx = r0y-r1y;           rpy = r1x-r0x;
	u = (float)r02x;   u *= (r2x-r1x);
	v = (float)r02y;   v *= (r2y-r1y);
	t = u + v;
	u = (float)r02x;   u *= rpx;
	v = (float)r02y;   v *= rpy;
	t /= (u + v) * 2.0f;
	rcx = r0x + (INTBIG)((r1x-r0x)/2 + t*rpx);
	rcy = r0y + (INTBIG)((r1y-r0y)/2 + t*rpy);

	/* now see if this point will be re-created */
	r = computedistance(r0x, r0y, rcx, rcy);
	if (!findcenters(r, r0x, r0y, r1x, r1y, ai->length, &x1, &y1, &x2, &y2))
	{
		if (abs(x1-rcx)+abs(y1-rcy) < abs(x2-rcx)+abs(y2-rcy)) r = -r;
	} else
	{
		rcx = r0x + (r1x-r0x)/2;
		rcy = r0y + (r1y-r0y)/2;
		r = computedistance(r0x, r0y, rcx, rcy) + 1;
	}
	return(r);
}

/*
 * Routine to replace arcs in "list" (that match the first arc there) with another of type
 * "ap", adding layer-change contacts
 * as needed to keep the connections.  If "connected" is true, replace all such arcs
 * connected to this.  If "thiscell" is true, replace all such arcs in the cell.
 */
void us_replaceallarcs(NODEPROTO *cell, GEOM **list, ARCPROTO *ap, BOOLEAN connected, BOOLEAN thiscell)
{
	REGISTER INTBIG lx, hx, ly, hy, cx, cy, wid, bits, i;
	INTBIG xs, ys;
	REGISTER NODEINST *ni, *newni, *nextni;
	NODEINST *ni0, *ni1;
	REGISTER ARCINST *ai, *newai, *nextai, *oldai;
	PORTPROTO *pp0, *pp1;
	REGISTER PORTARCINST *pi;
	REGISTER NODEPROTO *pin;

	if (list[0] == NOGEOM) return;
	if (list[0]->entryisnode) return;
	oldai = list[0]->entryaddr.ai;
	us_clearhighlightcount();

	/* mark the pin nodes that must be changed */
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		ni->temp1 = 0;
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		ai->temp1 = 0;

	for(i=0; list[i] != NOGEOM; i++)
	{
		if (list[i]->entryisnode) continue;
		ai = list[i]->entryaddr.ai;
		if (ai->proto != oldai->proto) continue;
		ai->temp1 = 1;
	}
	if (connected)
	{
		for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			if (ai->proto == oldai->proto && ai->network == oldai->network)
				ai->temp1 = 1;
	}
	if (thiscell)
	{
		for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			if (ai->proto == oldai->proto)
				ai->temp1 = 1;
	}
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex == 0) continue;
		if (ni->firstportexpinst != NOPORTEXPINST) continue;
		if (nodefunction(ni) != NPPIN) continue;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			if (pi->conarcinst->temp1 == 0) break;
		if (pi == NOPORTARCINST) ni->temp1 = 1;
	}

	/* now create new pins where they belong */
	pin = getpinproto(ap);
	defaultnodesize(pin, &xs, &ys);
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->temp1 == 0) continue;
		cx = (ni->lowx + ni->highx) / 2;
		cy = (ni->lowy + ni->highy) / 2;
		lx = cx - xs / 2;   hx = lx + xs;
		ly = cy - ys / 2;   hy = ly + ys;
		newni = newnodeinst(pin, lx, hx, ly, hy, 0, 0, cell);
		if (newni == NONODEINST) return;
		endobjectchange((INTBIG)newni, VNODEINST);
		newni->temp1 = 0;
		ni->temp1 = (INTBIG)newni;
	}

	/* now create new arcs to replace the old ones */
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->temp1 == 0) continue;
		ni0 = ai->end[0].nodeinst;
		if (ni0->temp1 != 0)
		{
			ni0 = (NODEINST *)ni0->temp1;
			pp0 = ni0->proto->firstportproto;
		} else
		{
			/* need contacts to get to the right level */
			us_makecontactstack(ai, 0, ap, &ni0, &pp0);
			if (ni0 == NONODEINST) return;
		}
		ni1 = ai->end[1].nodeinst;
		if (ni1->temp1 != 0)
		{
			ni1 = (NODEINST *)ni1->temp1;
			pp1 = ni1->proto->firstportproto;
		} else
		{
			/* need contacts to get to the right level */
			us_makecontactstack(ai, 1, ap, &ni1, &pp1);
			if (ni1 == NONODEINST) return;
		}

		wid = defaultarcwidth(ap);
		if (ai->width > wid) wid = ai->width;
		bits = us_makearcuserbits(ap);
		newai = newarcinst(ap, wid, bits, ni0, pp0, ai->end[0].xpos, ai->end[0].ypos,
			ni1, pp1, ai->end[1].xpos, ai->end[1].ypos, cell);
		if (newai == NOARCINST) return;
		(void)copyvars((INTBIG)ai, VARCINST, (INTBIG)newai, VARCINST, FALSE);
		newai->temp1 = 0;
		endobjectchange((INTBIG)newai, VARCINST);
	}

	/* now remove the previous arcs and nodes */
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = nextai)
	{
		nextai = ai->nextarcinst;
		if (ai->temp1 == 0) continue;
		startobjectchange((INTBIG)ai, VARCINST);
		(void)killarcinst(ai);
	}
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = nextni)
	{
		nextni = ni->nextnodeinst;
		if (ni->temp1 == 0) continue;
		startobjectchange((INTBIG)ni, VNODEINST);
		(void)killnodeinst(ni);
	}
}

NODEPROTO *us_contactstack[100];
ARCPROTO  *us_contactstackarc[100];

/*
 * Routine to examine end "end" of arc "ai" and return a node at that position which
 * can connect to arcs of type "ap".  This may require creation of one or more contacts
 * to change layers.
 */
void us_makecontactstack(ARCINST *ai, INTBIG end, ARCPROTO *ap, NODEINST **conni,
	PORTPROTO **conpp)
{
	REGISTER NODEINST *lastni, *newni;
	REGISTER ARCPROTO *typ;
	REGISTER ARCINST *newai;
	REGISTER NODEPROTO *np, *cell;
	REGISTER PORTPROTO *lastpp;
	REGISTER INTBIG i, depth, cx, cy, lx, hx, ly, hy, wid, bits;
	INTBIG xs, ys;

	lastni = ai->end[end].nodeinst;
	lastpp = ai->end[end].portarcinst->proto;
	for(np = ap->tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		np->temp1 = 0;
	depth = us_findpathtoarc(lastpp, ap, 0);
	if (depth < 0)
	{
		*conni = NONODEINST;
		return;
	}

	/* create the contacts */
	cell = ai->parent;
	*conni = lastni;
	*conpp = lastpp;
	cx = ai->end[end].xpos;   cy = ai->end[end].ypos;
	for(i=0; i<depth; i++)
	{
		defaultnodesize(us_contactstack[i], &xs, &ys);
		lx = cx - xs / 2;   hx = lx + xs;
		ly = cy - ys / 2;   hy = ly + ys;
		newni = newnodeinst(us_contactstack[i], lx, hx, ly, hy, 0, 0, cell);
		if (newni == NONODEINST)
		{
			*conni = NONODEINST;
			return;
		}
		endobjectchange((INTBIG)newni, VNODEINST);
		*conni = newni;
		*conpp = newni->proto->firstportproto;
		typ = us_contactstackarc[i];
		wid = defaultarcwidth(typ);
		bits = us_makearcuserbits(typ);
		newai = newarcinst(typ, wid, bits, lastni, lastpp, cx, cy, *conni, *conpp, cx, cy, cell);
		if (newai == NOARCINST)
		{
			*conni = NONODEINST;
			return;
		}
		endobjectchange((INTBIG)newai, VARCINST);
	}
}

INTBIG us_findpathtoarc(PORTPROTO *pp, ARCPROTO *ap, INTBIG depth)
{
	REGISTER INTBIG i, j, fun, bestdepth, newdepth;
	REGISTER NODEPROTO *bestnp, *nextnp;
	REGISTER PORTPROTO *nextpp;
	REGISTER ARCPROTO *thisap, *bestap;
	REGISTER TECHNOLOGY *tech;

	/* see if the connection is made */
	for(i=0; pp->connects[i] != NOARCPROTO; i++)
		if (pp->connects[i] == ap) return(depth);

	/* look for a contact */
	bestnp = NONODEPROTO;
	tech = ap->tech;
	for(nextnp = tech->firstnodeproto; nextnp != NONODEPROTO; nextnp = nextnp->nextnodeproto)
	{
		if (nextnp->temp1 != 0) continue;
		fun = (nextnp->userbits&NFUNCTION) >> NFUNCTIONSH;
		if (fun != NPCONTACT) continue;

		/* see if this contact connects to the destination */
		nextpp = nextnp->firstportproto;
		for(i=0; nextpp->connects[i] != NOARCPROTO; i++)
		{
			thisap = nextpp->connects[i];
			if (thisap->tech != tech) continue;
			for(j=0; pp->connects[j] != NOARCPROTO; j++)
				if (pp->connects[j] == thisap) break;
			if (pp->connects[j] != NOARCPROTO) break;
		}
		if (nextpp->connects[i] == NOARCPROTO) continue;

		/* this contact is part of the chain */
		us_contactstack[depth] = nextnp;
		nextnp->temp1 = 1;
		newdepth = us_findpathtoarc(nextpp, ap, depth+1);
		nextnp->temp1 = 0;
		if (newdepth < 0) continue;
		if (bestnp == NONODEPROTO || newdepth < bestdepth)
		{
			bestdepth = newdepth;
			bestnp = nextnp;
			bestap = nextpp->connects[i];
		}
	}
	if (bestnp != NONODEPROTO)
	{
		us_contactstack[depth] = bestnp;
		us_contactstackarc[depth] = bestap;
		bestnp->temp1 = 1;
		newdepth = us_findpathtoarc(bestnp->firstportproto, ap, depth+1);
		bestnp->temp1 = 0;
		return(newdepth);
	}
	return(-1);
}

/*********************************** NODE SUPPORT ***********************************/

/*
 * routine to travel through the network starting at nodeinst "ni" and set
 * "bit" in all nodes connected with arcs that do not spread.
 */
void us_nettravel(NODEINST *ni, INTBIG bit)
{
	REGISTER INTBIG i;
	REGISTER PORTARCINST *pi;

	if ((ni->userbits & bit) != 0) return;
	ni->userbits |= bit;

	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		if ((pi->conarcinst->userbits & ARCFLAGBIT) != 0) continue;
		for(i=0; i<2; i++) us_nettravel(pi->conarcinst->end[i].nodeinst, bit);
	}
}

/*
 * Routine to return the proper "WIPED" bit for node "ni".
 */
UINTBIG us_computewipestate(NODEINST *ni)
{
	REGISTER PORTARCINST *pi;
	REGISTER ARCINST *ai;

	if ((ni->proto->userbits&ARCSWIPE) == 0) return(0);
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		ai = pi->conarcinst;
		if ((ai->proto->userbits&CANWIPE) != 0) return(WIPED);
	}
	return(0);
}

/*
 * Routine to spread around node "ni" in direction "direction" and distance "amount".
 * Returns nonzero if something was moved.
 */
INTBIG us_spreadaround(NODEINST *ni, INTBIG amount, CHAR *direction)
{
	REGISTER NODEINST *no1, *no2, *inno;
	REGISTER NODEPROTO *cell;
	REGISTER ARCINST *ai;
	INTBIG plx, ply, phx, phy;
	REGISTER INTBIG xc1, yc1, xc2, yc2, i, slx, shx, sly, shy;
	REGISTER INTBIG doit, again, moved;
	REGISTER BOOLEAN mustbehor;

	cell = ni->parent;
	nodesizeoffset(ni, &plx, &ply, &phx, &phy);
	slx = ni->lowx + plx;
	shx = ni->highx - phx;
	sly = ni->lowy + ply;
	shy = ni->highy - phy;

	/* initialize by turning the marker bits off */
	for(inno = cell->firstnodeinst; inno != NONODEINST; inno = inno->nextnodeinst)
		inno->userbits &= ~NODEFLAGBIT;
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		ai->userbits &= ~ARCFLAGBIT;

	/* set "already done" flag for nodes manhattan connected on spread line */
	if (*direction == 'l' || *direction == 'r') mustbehor = FALSE; else
		mustbehor = TRUE;
	us_manhattantravel(ni, mustbehor);

	/* set "already done" flag for nodes that completely cover spread node or are in its line */
	for(inno = cell->firstnodeinst; inno != NONODEINST; inno = inno->nextnodeinst)
	{
		nodesizeoffset(inno, &plx, &ply, &phx, &phy);
		if (*direction == 'l' || *direction == 'r')
		{
			if (inno->lowx + plx < slx && inno->highx - phx > shx)
				inno->userbits |= NODEFLAGBIT;
			if ((inno->lowx+inno->highx)/2 == (slx+shx)/2)
				inno->userbits |= NODEFLAGBIT;
		} else
		{
			if (inno->lowy + ply < sly && inno->highy - phy > shy)
				inno->userbits |= NODEFLAGBIT;
			if ((inno->lowy+inno->highy)/2 == (sly+shy)/2)
				inno->userbits |= NODEFLAGBIT;
		}
	}

	/* mark those arcinsts that should stretch during spread */
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		no1 = ai->end[0].nodeinst;   no2 = ai->end[1].nodeinst;
		xc1 = (no1->lowx + no1->highx) / 2;   yc1 = (no1->lowy + no1->highy) / 2;
		xc2 = (no2->lowx + no2->highx) / 2;   yc2 = (no2->lowy + no2->highy) / 2;

		/* if one node is along spread line, make it "no1" */
		if ((no2->userbits&NODEFLAGBIT) != 0)
		{
			inno = no1;  no1 = no2;  no2 = inno;
			i = xc1;     xc1 = xc2;  xc2 = i;
			i = yc1;     yc1 = yc2;  yc2 = i;
		}

		/* if both nodes are along spread line, leave arc alone */
		if ((no2->userbits&NODEFLAGBIT) != 0) continue;

		i = 1;

		if ((no1->userbits&NODEFLAGBIT) != 0)
		{
			/* handle arcs connected to spread line */
			switch (*direction)
			{
				case 'l': if (xc2 <= slx) i = 0;   break;
				case 'r': if (xc2 >= shx) i = 0;  break;
				case 'u': if (yc2 >= shy) i = 0;  break;
				case 'd': if (yc2 <= sly) i = 0;   break;
			}
		} else
		{
			/* handle arcs that cross the spread line */
			switch (*direction)
			{
				case 'l': if (xc1 > slx && xc2 <= slx) i = 0; else
					if (xc2 > slx && xc1 <= slx) i = 0;
					break;
				case 'r': if (xc1 < shx && xc2 >= shx) i = 0; else
					if (xc2 < shx && xc1 >= shx) i = 0;
					break;
				case 'u': if (yc1 > shy && yc2 <= shy) i = 0; else
					if (yc2 > shy && yc1 <= shy) i = 0;
					break;
				case 'd': if (yc1 < sly && yc2 >= sly) i = 0; else
					if (yc2 < sly && yc1 >= sly) i = 0;
					break;
			}
		}
		if (i == 0) ai->userbits |= ARCFLAGBIT;
	}

	/* now look at every nodeinst in the cell */
	moved = 0;
	again = 1;
	while (again)
	{
		again = 0;
		for(inno = cell->firstnodeinst; inno != NONODEINST; inno = inno->nextnodeinst)
		{
			if (stopping(STOPREASONSPREAD)) break;

			/* ignore this nodeinst if it has been spread already */
			if ((inno->userbits & NODEFLAGBIT) != 0) continue;

			/* make sure nodeinst is on proper side of requested spread */
			xc1 = (inno->highx+inno->lowx) / 2;
			yc1 = (inno->highy+inno->lowy) / 2;
			doit = 0;
			switch (*direction)
			{
				case 'l': if (xc1 < slx)  doit++;   break;
				case 'r': if (xc1 > shx) doit++;   break;
				case 'u': if (yc1 > shy) doit++;   break;
				case 'd': if (yc1 < sly)  doit++;   break;
			}
			if (doit == 0) continue;

			/* set every connecting nodeinst to be "spread" */
			for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			{
				if ((ai->userbits & ARCFLAGBIT) != 0)
				{
					/* make arc temporarily unrigid */
					(void)(*el_curconstraint->setobject)((INTBIG)ai, VARCINST,
						CHANGETYPETEMPUNRIGID, 0);
				} else
				{
					/* make arc temporarily rigid */
					(void)(*el_curconstraint->setobject)((INTBIG)ai, VARCINST,
						CHANGETYPETEMPRIGID, 0);
				}
			}
			us_nettravel(inno, NODEFLAGBIT);

			/* move this nodeinst in proper direction to do spread */
			startobjectchange((INTBIG)inno, VNODEINST);
			switch(*direction)
			{
				case 'l':
					modifynodeinst(inno, -amount, 0, -amount, 0, 0,0);
					break;
				case 'r':
					modifynodeinst(inno, amount, 0, amount, 0, 0,0);
					break;
				case 'u':
					modifynodeinst(inno, 0, amount, 0, amount, 0,0);
					break;
				case 'd':
					modifynodeinst(inno, 0, -amount, 0, -amount, 0,0);
					break;
			}
			endobjectchange((INTBIG)inno, VNODEINST);

			/* set loop iteration flag and node spread flag */
			moved++;
			again++;
			break;
		}
	}

	/* report what was moved */
	(*el_curconstraint->solve)(cell);
	return(moved);
}

/*
 * Helper routine for "us_rotate()" to mark selected nodes that need not be
 * connected with an invisible arc.
 */
void us_spreadrotateconnection(NODEINST *theni)
{
	REGISTER PORTARCINST *pi;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni;
	REGISTER INTBIG other;

	for(pi = theni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		ai = pi->conarcinst;
		if (ai->temp1 == 0) continue;
		if (ai->end[0].portarcinst == pi) other = 1; else other = 0;
		ni = ai->end[other].nodeinst;
		if (ni->temp1 != 0) continue;
		ni->temp1 = 1;
		us_spreadrotateconnection(ni);
	}
}

/*
 * Routine to recompute the "FAR TEXT" bit on node "ni".
 */
void us_computenodefartextbit(NODEINST *ni)
{
	REGISTER INTBIG i, sizelimit;
	INTBIG xw, yw;
	REGISTER VARIABLE *var;
	REGISTER PORTEXPINST *pe;
	REGISTER NODEPROTO *savecell;
	HIGHLIGHT high;

	/* make sure there is a window */
	if (el_curwindowpart == NOWINDOWPART) return;

	ni->userbits &= ~NHASFARTEXT;
	high.status = HIGHTEXT;
	high.cell = ni->parent;
	high.fromgeom = ni->geom;
	high.frompoint = 0;
	high.fromport = NOPORTPROTO;
	high.fromvarnoeval = NOVARIABLE;
	sizelimit = lambdaofnode(ni) * FARTEXTLIMIT * 2;
	for(i=0; i<ni->numvar; i++)
	{
		var = &ni->firstvar[i];
		if ((var->type&VDISPLAY) == 0) continue;
		if ((TDGETSIZE(var->textdescript)&TXTQLAMBDA) == 0)
		{
			ni->userbits |= NHASFARTEXT;
			return;
		}
		if (abs(TDGETXOFF(var->textdescript)) >= FARTEXTLIMIT*4 ||
			abs(TDGETYOFF(var->textdescript)) >= FARTEXTLIMIT*4)
		{
			ni->userbits |= NHASFARTEXT;
			return;
		}
		high.fromvar = var;
		savecell = el_curwindowpart->curnodeproto;
		el_curwindowpart->curnodeproto = ni->parent;
		us_gethightextsize(&high, &xw, &yw, el_curwindowpart);
		el_curwindowpart->curnodeproto = savecell;
		if (xw > sizelimit || yw > sizelimit)
		{
			ni->userbits |= NHASFARTEXT;
			return;
		}
	}
	high.fromvar = NOVARIABLE;
	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
	{
		if (abs(TDGETXOFF(pe->exportproto->textdescript)) >= FARTEXTLIMIT*4 ||
			abs(TDGETYOFF(pe->exportproto->textdescript)) >= FARTEXTLIMIT*4)
		{
			ni->userbits |= NHASFARTEXT;
			return;
		}
		if ((TDGETSIZE(pe->exportproto->textdescript)&TXTQLAMBDA) == 0)
		{
			ni->userbits |= NHASFARTEXT;
			return;
		}
		high.fromport = pe->exportproto;
		savecell = el_curwindowpart->curnodeproto;
		el_curwindowpart->curnodeproto = ni->parent;
		us_gethightextsize(&high, &xw, &yw, el_curwindowpart);
		el_curwindowpart->curnodeproto = savecell;
		if (xw > sizelimit || yw > sizelimit)
		{
			ni->userbits |= NHASFARTEXT;
			return;
		}
	}
}

/*
 * routine to recursively travel along all arcs coming out of nodeinst "ni"
 * and set the NODEFLAGBIT bit in the connecting nodeinst "userbits" if that node
 * is connected horizontally (if "hor" is true) or connected vertically (if
 * "hor" is zero).  This is called from "spread" to propagate along manhattan
 * arcs that are in the correct orientation (along the spread line).
 */
void us_manhattantravel(NODEINST *ni, BOOLEAN hor)
{
	REGISTER PORTARCINST *pi;
	REGISTER NODEINST *other;
	REGISTER ARCINST *ai;

	ni->userbits |= NODEFLAGBIT;
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		ai = pi->conarcinst;
		if (hor)
		{
			/* "hor" is nonzero: only want horizontal arcs */
			if (((ai->userbits&AANGLE)>>AANGLESH) != 0 &&
				((ai->userbits&AANGLE)>>AANGLESH) != 180) continue;
		} else
		{
			/* "hor" is zero: only want vertical arcs */
			if (((ai->userbits&AANGLE)>>AANGLESH) != 90 &&
				((ai->userbits&AANGLE)>>AANGLESH) != 270) continue;
		}
		if (ai->end[0].portarcinst == pi) other = ai->end[1].nodeinst; else
			other = ai->end[0].nodeinst;
		if ((other->userbits&NODEFLAGBIT) != 0) continue;
		us_manhattantravel(other, hor);
	}
}

/*
 * routine to replace node "oldni" with a new one of type "newnp"
 * and return the new node.  Also removes any node-specific variables.
 */
NODEINST *us_replacenodeinst(NODEINST *oldni, NODEPROTO *newnp, BOOLEAN ignoreportnames,
	BOOLEAN allowmissingports)
{
	REGISTER NODEINST *newni;
	REGISTER VARIABLE *var, *cvar;
	REGISTER NODEPROTO *cnp;
	REGISTER PORTARCINST *pi;
	REGISTER INTBIG i, j;
	typedef struct
	{
		CHAR *variablename;
		NODEPROTO **prim;
	} POSSIBLEVARIABLES;
	static POSSIBLEVARIABLES killvariables[] =
	{
		{x_("ATTR_length"),            &sch_transistorprim},
		{x_("ATTR_length"),            &sch_transistor4prim},
		{x_("ATTR_width"),             &sch_transistorprim},
		{x_("ATTR_width"),             &sch_transistor4prim},
		{x_("ATTR_area"),              &sch_transistorprim},
		{x_("ATTR_area"),              &sch_transistor4prim},
		{x_("SIM_spice_model"),        &sch_sourceprim},
		{x_("SIM_spice_model"),        &sch_transistorprim},
		{x_("SIM_spice_model"),        &sch_transistor4prim},
		{x_("SCHEM_meter_type"),       &sch_meterprim},
		{x_("SCHEM_diode"),            &sch_diodeprim},
		{x_("SCHEM_capacitance"),      &sch_capacitorprim},
		{x_("SCHEM_resistance"),       &sch_resistorprim},
		{x_("SCHEM_inductance"),       &sch_inductorprim},
		{x_("SCHEM_function"),         &sch_bboxprim},
		{0, 0}
	};

	/* first start changes to node and all arcs touching this node */
	startobjectchange((INTBIG)oldni, VNODEINST);
	for(pi = oldni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		startobjectchange((INTBIG)pi->conarcinst, VARCINST);

	/* replace the node */
	newni = replacenodeinst(oldni, newnp, ignoreportnames, allowmissingports);
	if (newni != NONODEINST)
	{
		/* remove variables that make no sense */
		for(i=0; killvariables[i].variablename != 0; i++)
		{
			if (newni->proto == *killvariables[i].prim) continue;
			var = getval((INTBIG)newni, VNODEINST, -1, killvariables[i].variablename);
			if (var != NOVARIABLE)
				(void)delval((INTBIG)newni, VNODEINST, killvariables[i].variablename);
		}

		/* remove parameters that don't exist on the new object */
		for(i=0; i<newni->numvar; i++)
		{
			var = &newni->firstvar[i];
			if (TDGETISPARAM(var->textdescript) == 0) continue;

			/* see if this parameter exists on the new prototype */
			cnp = contentsview(newnp);
			if (cnp == NONODEPROTO) cnp = newnp;
			for(j=0; j<cnp->numvar; j++)
			{
				cvar = &cnp->firstvar[j];
				if (var->key != cvar->key) continue;
				if (TDGETISPARAM(cvar->textdescript) != 0) break;
			}
			if (j >= cnp->numvar)
			{
				(void)delvalkey((INTBIG)newni, VNODEINST, var->key);
				i--;
			}
		}

		/* now inherit parameters that now do exist */
		us_inheritattributes(newni);

		/* remove node name if it is not visible */
		var = getvalkey((INTBIG)newni, VNODEINST, -1, el_node_name_key);
		if (var != NOVARIABLE && (var->type&VDISPLAY) == 0)
			(void)delvalkey((INTBIG)newni, VNODEINST, el_node_name_key);

		/* end changes to node and all arcs touching this node */
		endobjectchange((INTBIG)newni, VNODEINST);
		for(pi = newni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			endobjectchange((INTBIG)pi->conarcinst, VARCINST);
	}
	return(newni);
}

/*
 * Routine to erase all of the objects in cell "np" that are in the NOGEOM-terminated
 * list of GEOM modules "list".
 */
void us_eraseobjectsinlist(NODEPROTO *np, GEOM **list)
{
	REGISTER INTBIG i, otherend;
	REGISTER INTBIG deletedobjects;
	REGISTER NODEINST *ni, *nextni, *ni1, *ni2;
	ARCINST *ai;
	REGISTER PORTARCINST *pi;

	/* mark all nodes touching arcs that are killed */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst) ni->temp1 = 0;
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (list[i]->entryisnode) continue;
		ai = list[i]->entryaddr.ai;
		ai->end[0].nodeinst->temp1 = 1;
		ai->end[1].nodeinst->temp1 = 1;
	}

	/* also mark all nodes on arcs that will be erased */
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (!list[i]->entryisnode) continue;
		ni = list[i]->entryaddr.ni;
		if (ni->temp1 != 0) ni->temp1 = 2;
	}

	/* also mark all nodes on the other end of arcs connected to erased nodes */
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (!list[i]->entryisnode) continue;
		ni = list[i]->entryaddr.ni;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			ai = pi->conarcinst;
			if (ai->end[0].portarcinst == pi) otherend = 1; else
				otherend = 0;
			if (ai->end[otherend].nodeinst->temp1 == 0)
				ai->end[otherend].nodeinst->temp1 = 1;
		}
	}

	/* see if this is a major change */
	deletedobjects = 0;
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (!list[i]->entryisnode) deletedobjects++; else
		{
			ni = list[i]->entryaddr.ni;
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				deletedobjects++;
		}
	}

	/* if this change is too vast, simply turn off network tool while it happens */
	if (deletedobjects > 100) toolturnoff(net_tool, FALSE);

	/* now kill all of the arcs */
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (list[i]->entryisnode) continue;
		ai = list[i]->entryaddr.ai;

		/* see if nodes need to be undrawn to account for "Steiner Point" changes */
		ni1 = ai->end[0].nodeinst;   ni2 = ai->end[1].nodeinst;
		if (ni1->temp1 == 1 && (ni1->proto->userbits&WIPEON1OR2) != 0)
			startobjectchange((INTBIG)ni1, VNODEINST);
		if (ni2->temp1 == 1 && (ni2->proto->userbits&WIPEON1OR2) != 0)
			startobjectchange((INTBIG)ni2, VNODEINST);

		startobjectchange((INTBIG)ai, VARCINST);
		if (killarcinst(ai)) ttyputerr(_("Error killing arc"));

		/* see if nodes need to be redrawn to account for "Steiner Point" changes */
		if (ni1->temp1 == 1 && (ni1->proto->userbits&WIPEON1OR2) != 0)
			endobjectchange((INTBIG)ni1, VNODEINST);
		if (ni2->temp1 == 1 && (ni2->proto->userbits&WIPEON1OR2) != 0)
			endobjectchange((INTBIG)ni2, VNODEINST);
	}

	/* next kill all of the nodes */
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (!list[i]->entryisnode) continue;
		ni = list[i]->entryaddr.ni;
		us_erasenodeinst(ni);
	}

	/* kill all pin nodes that touched an arc and no longer do */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = nextni)
	{
		nextni = ni->nextnodeinst;
		if (ni->temp1 == 0) continue;
		if (ni->proto->primindex == 0) continue;
		if (((ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH) != NPPIN) continue;
		if (ni->firstportarcinst != NOPORTARCINST || ni->firstportexpinst != NOPORTEXPINST)
			continue;
		us_erasenodeinst(ni);
	}

	/* kill all unexported pin or bus nodes left in the middle of arcs */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = nextni)
	{
		nextni = ni->nextnodeinst;
		if (ni->temp1 == 0) continue;
		if (ni->proto->primindex == 0) continue;
		if (((ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH) != NPPIN) continue;
		if (ni->firstportexpinst != NOPORTEXPINST) continue;
		(void)us_erasepassthru(ni, FALSE, &ai);
	}

	/* if network was turned off, turn it back on */
	if (deletedobjects > 100) toolturnon(net_tool);
}

/*
 * Routine to erase node "ni" and all associated arcs, exports, etc.
 */
void us_erasenodeinst(NODEINST *ni)
{
	REGISTER PORTARCINST *pi, *npi;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni1, *ni2;

	/* erase all connecting arcs to this nodeinst */
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = npi)
	{
		npi = pi->nextportarcinst;

		/* don't delete if already dead */
		ai = pi->conarcinst;
		if ((ai->userbits&DEADA) != 0) continue;

		/* see if nodes need to be undrawn to account for "Steiner Point" changes */
		ni1 = ai->end[0].nodeinst;   ni2 = ai->end[1].nodeinst;
		if ((ni1->proto->userbits&WIPEON1OR2) != 0) startobjectchange((INTBIG)ni1, VNODEINST);
		if ((ni2->proto->userbits&WIPEON1OR2) != 0) startobjectchange((INTBIG)ni2, VNODEINST);

		startobjectchange((INTBIG)ai, VARCINST);
		if (killarcinst(ai)) ttyputerr(_("Error killing arc"));

		/* see if nodes need to be redrawn to account for "Steiner Point" changes */
		if ((ni1->proto->userbits&WIPEON1OR2) != 0) endobjectchange((INTBIG)ni1, VNODEINST);
		if ((ni2->proto->userbits&WIPEON1OR2) != 0) endobjectchange((INTBIG)ni2, VNODEINST);
	}

	/* see if this nodeinst is a port of the cell */
	startobjectchange((INTBIG)ni, VNODEINST);
	if (ni->firstportexpinst != NOPORTEXPINST) us_undoportproto(ni, NOPORTPROTO);

	/* now erase the nodeinst */
	if (killnodeinst(ni)) ttyputerr(_("Error from killnodeinst"));
}

#define NORECONNECT ((RECONNECT *)-1)

typedef struct Ireconnect
{
	NETWORK *net;					/* network for this reconnection */
	INTBIG arcsfound;				/* number of arcs found on this reconnection */
	INTBIG reconx[2], recony[2];	/* coordinate at other end of arc */
	INTBIG origx[2], origy[2];		/* coordinate where arc hits deleted node */
	INTBIG dx[2], dy[2];			/* distance between ends */
	NODEINST *reconno[2];			/* node at other end of arc */
	PORTPROTO *reconpt[2];			/* port at other end of arc */
	ARCINST *reconar[2];			/* arcinst being reconnected */
	ARCPROTO *ap;					/* prototype of new arc */
	INTBIG wid;						/* width of new arc */
	INTBIG bits;					/* user bits of new arc */
	struct Ireconnect *nextreconnect;
} RECONNECT;

/*
 * routine to kill a node between two arcs and join the arc as one.  Returns an error
 * code according to its success.  If it worked, the new arc is placed in "newai".
 */
INTBIG us_erasepassthru(NODEINST *ni, BOOLEAN allowdiffs, ARCINST **newai)
{
	INTBIG i, j, retval;
	PORTARCINST *pi;
	REGISTER ARCINST *ai;
	REGISTER NODEPROTO *cell;
	RECONNECT *firstrecon, *re, *nextre;

	/* disallow erasing if lock is on */
	cell = ni->parent;
	if (us_cantedit(cell, ni, TRUE)) return(-1);

	/* look for pairs arcs that will get reconnected */
	firstrecon = NORECONNECT;
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		/* ignore arcs that connect from the node to itself */
		ai = pi->conarcinst;
		if (ai->end[0].nodeinst == ni && ai->end[1].nodeinst == ni) continue;

		/* find a "reconnect" object with this network */
		for(re = firstrecon; re != NORECONNECT; re = re->nextreconnect)
			if (re->net == ai->network) break;
		if (re == NORECONNECT)
		{
			re = (RECONNECT *)emalloc(sizeof (RECONNECT), us_tool->cluster);
			if (re == 0) return(-1);
			re->net = ai->network;
			re->arcsfound = 0;
			re->nextreconnect = firstrecon;
			firstrecon = re;
		}
		j = re->arcsfound;
		re->arcsfound++;
		if (re->arcsfound > 2) continue;
		re->reconar[j] = ai;
		for(i=0; i<2; i++) if (ai->end[i].nodeinst != ni)
		{
			re->reconno[j] = ai->end[i].nodeinst;
			re->reconpt[j] = ai->end[i].portarcinst->proto;
			re->reconx[j] = ai->end[i].xpos;
			re->origx[j] = ai->end[1-i].xpos;
			re->dx[j] = re->reconx[j] - re->origx[j];
			re->recony[j] = ai->end[i].ypos;
			re->origy[j] = ai->end[1-i].ypos;
			re->dy[j] = re->recony[j] - re->origy[j];
		}
	}

	/* examine all of the reconnection situations */
	for(re = firstrecon; re != NORECONNECT; re = re->nextreconnect)
	{
		if (re->arcsfound != 2) continue;

		/* verify that the two arcs to merge have the same type */
		if (re->reconar[0]->proto != re->reconar[1]->proto) { re->arcsfound = -1; continue; }
		re->ap = re->reconar[0]->proto;

		if (!allowdiffs)
		{
			/* verify that the two arcs to merge have the same width */
			if (re->reconar[0]->width != re->reconar[1]->width) { re->arcsfound = -2; continue; }

			/* verify that the two arcs have the same slope */
			if ((re->dx[1]*re->dy[0]) != (re->dx[0]*re->dy[1])) { re->arcsfound = -3; continue; }
			if (re->origx[0] != re->origx[1] || re->origy[0] != re->origy[1])
			{
				/* did not connect at the same location: be sure that angle is consistent */
				if (re->dx[0] != 0 || re->dy[0] != 0)
				{
					if (((re->origx[0]-re->origx[1])*re->dy[0]) !=
						(re->dx[0]*(re->origy[0]-re->origy[1]))) { re->arcsfound = -3; continue; }
				} else if (re->dx[1] != 0 || re->dy[1] != 0)
				{
					if (((re->origx[0]-re->origx[1])*re->dy[1]) !=
						(re->dx[1]*(re->origy[0]-re->origy[1]))) { re->arcsfound = -3; continue; }
				} else { re->arcsfound = -3; continue; }
			}
		}

		/* remember facts about the new arcinst */
		re->wid = re->reconar[0]->width;
		re->bits = re->reconar[0]->userbits | re->reconar[1]->userbits;

		/* special code to handle directionality */
		if ((re->bits&(ISDIRECTIONAL|ISNEGATED|NOTEND0|NOTEND1|REVERSEEND)) != 0)
		{
			/* reverse ends if the arcs point the wrong way */
			for(i=0; i<2; i++)
			{
				if (re->reconar[i]->end[i].nodeinst == ni)
				{
					if ((re->reconar[i]->userbits&REVERSEEND) == 0)
						re->reconar[i]->userbits |= REVERSEEND; else
							re->reconar[i]->userbits &= ~REVERSEEND;
				}
			}
			re->bits = re->reconar[0]->userbits | re->reconar[1]->userbits;

			/* two negations make a positive */
			if ((re->reconar[0]->userbits&ISNEGATED) != 0 &&
				(re->reconar[1]->userbits&ISNEGATED) != 0) re->bits &= ~ISNEGATED;
		}
	}

	/* see if any reconnection will be done */
	for(re = firstrecon; re != NORECONNECT; re = re->nextreconnect)
	{
		retval = re->arcsfound;
		if (retval == 2) break;
	}

	/* erase the nodeinst if reconnection will be done (this will erase connecting arcs) */
	if (retval == 2) us_erasenodeinst(ni);

	/* reconnect the arcs */
	for(re = firstrecon; re != NORECONNECT; re = re->nextreconnect)
	{
		if (re->arcsfound != 2) continue;

		/* make the new arcinst */
		*newai = newarcinst(re->ap, re->wid, re->bits, re->reconno[0], re->reconpt[0],
			re->reconx[0], re->recony[0], re->reconno[1], re->reconpt[1], re->reconx[1], re->recony[1],
				cell);
		if (*newai == NOARCINST) { re->arcsfound = -5; continue; }

		(void)copyvars((INTBIG)re->reconar[0], VARCINST, (INTBIG)*newai, VARCINST, FALSE);
		(void)copyvars((INTBIG)re->reconar[1], VARCINST, (INTBIG)*newai, VARCINST, FALSE);
		endobjectchange((INTBIG)*newai, VARCINST);
		(*newai)->changed = 0;
	}

	/* deallocate */
	for(re = firstrecon; re != NORECONNECT; re = nextre)
	{
		nextre = re->nextreconnect;
		efree((CHAR *)re);
	}
	return(retval);
}

/*
 * Routine to set the variable "key" on object "geom" to the new value "newvalue".  The former
 * type of that variable is "oldtype".
 */
void us_setvariablevalue(GEOM *geom, INTBIG key, CHAR *newvalue, INTBIG oldtype, UINTBIG *descript)
{
	INTBIG newval, newtype, units;
	REGISTER VARIABLE *var;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;

	if ((oldtype&(VCODE1|VCODE2)) != 0)
	{
		newval = (INTBIG)newvalue;
	} else
	{
		if (descript == 0) units = 0; else
			units = TDGETUNITS(descript);
		getsimpletype(newvalue, &newtype, &newval, units);
		oldtype = (oldtype & ~VTYPE) | newtype;
	}
	us_pushhighlight();
	us_clearhighlightcount();
	if (geom->entryisnode)
	{
		ni = geom->entryaddr.ni;
		startobjectchange((INTBIG)ni, VNODEINST);
		var = setvalkey((INTBIG)ni, VNODEINST, key, newval, oldtype);
		if (var != NOVARIABLE && descript != 0)
			TDCOPY(var->textdescript, descript);
		endobjectchange((INTBIG)ni, VNODEINST);
	} else
	{
		ai = geom->entryaddr.ai;
		startobjectchange((INTBIG)ai, VARCINST);
		var = setvalkey((INTBIG)ai, VARCINST, key, newval, oldtype);
		if (var != NOVARIABLE && descript != 0)
			TDCOPY(var->textdescript, descript);
		endobjectchange((INTBIG)ai, VARCINST);
	}
	us_pophighlight(FALSE);
}

/*
 * Routine to set the variable "key" on object "geom" to the new value "value".
 */
void us_setfloatvariablevalue(GEOM *geom, INTBIG key, VARIABLE *oldvar, float value)
{
	REGISTER VARIABLE *var;
	REGISTER BOOLEAN haddescript;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	if (oldvar == NOVARIABLE) haddescript = FALSE; else
	{
		haddescript = TRUE;
		TDCOPY(descript, oldvar->textdescript);
	}

	us_pushhighlight();
	us_clearhighlightcount();
	if (geom->entryisnode)
	{
		ni = geom->entryaddr.ni;
		startobjectchange((INTBIG)ni, VNODEINST);
		var = setvalkey((INTBIG)ni, VNODEINST, key, castint(value), VFLOAT|VDISPLAY);
		if (var != NOVARIABLE && haddescript)
			TDCOPY(var->textdescript, descript);
		endobjectchange((INTBIG)ni, VNODEINST);
	} else
	{
		ai = geom->entryaddr.ai;
		startobjectchange((INTBIG)ai, VARCINST);
		var = setvalkey((INTBIG)ai, VARCINST, key, castint(value), VFLOAT|VDISPLAY);
		if (var != NOVARIABLE && haddescript)
			TDCOPY(var->textdescript, descript);
		endobjectchange((INTBIG)ai, VARCINST);
	}
	us_pophighlight(FALSE);
}

/*
 * Routine to align the "total" nodes in "nodelist".
 * If "horizontal" is true, align them horizontally according to "direction"
 * (0 for left, 1 for right, 2 for center).
 * If "horizontal is false, align them veritcally according to "direction"
 * (0 for top, 1 for bottom, 2 for center).
 */
void us_alignnodes(INTBIG total, NODEINST **nodelist, BOOLEAN horizontal, INTBIG direction)
{
	REGISTER INTBIG *dlx, *dly, *dhx, *dhy, *dxf, i, lx, hx, ly, hy;
	REGISTER NODEINST *ni;

	if (total <= 0) return;
	dlx = (INTBIG *)emalloc(total * SIZEOFINTBIG, el_tempcluster);
	if (dlx == 0) return;
	dhx = (INTBIG *)emalloc(total * SIZEOFINTBIG, el_tempcluster);
	if (dhx == 0) return;
	dly = (INTBIG *)emalloc(total * SIZEOFINTBIG, el_tempcluster);
	if (dly == 0) return;
	dhy = (INTBIG *)emalloc(total * SIZEOFINTBIG, el_tempcluster);
	if (dhy == 0) return;
	dxf = (INTBIG *)emalloc(total * SIZEOFINTBIG, el_tempcluster);
	if (dxf == 0) return;

	/* get bounds */
	for(i=0; i<total; i++)
	{
		ni = nodelist[i];
		if (i == 0)
		{
			lx = ni->geom->lowx;
			hx = ni->geom->highx;
			ly = ni->geom->lowy;
			hy = ni->geom->highy;
		} else
		{
			if (ni->geom->lowx < lx) lx = ni->geom->lowx;
			if (ni->geom->highx > hx) hx = ni->geom->highx;
			if (ni->geom->lowy < ly) ly = ni->geom->lowy;
			if (ni->geom->highy > hy) hy = ni->geom->highy;
		}
	}

	/* determine motion */
	for(i=0; i<total; i++)
	{
		dlx[i] = dhx[i] = dly[i] = dhy[i] = dxf[i] = 0;
		ni = nodelist[i];
		if (horizontal)
		{
			/* horizontal alignment */
			switch (direction)
			{
				case 0:		/* align to left */
					dlx[i] = dhx[i] = lx - ni->geom->lowx;
					break;
				case 1:		/* align to right */
					dlx[i] = dhx[i] = hx - ni->geom->highx;
					break;
				case 2:		/* align to center */
					dlx[i] = dhx[i] = (lx + hx) / 2 - (ni->geom->lowx + ni->geom->highx) / 2;
					break;
			}
		} else
		{
			/* vertical alignment */
			switch (direction)
			{
				case 0:		/* align to top */
					dly[i] = dhy[i] = hy - ni->geom->highy;
					break;
				case 1:		/* align to bottom */
					dly[i] = dhy[i] = ly - ni->geom->lowy;
					break;
				case 2:		/* align to center */
					dly[i] = dhy[i] = (ly + hy) / 2 - (ni->geom->lowy + ni->geom->highy) / 2;
					break;
			}
		}
	}

	us_pushhighlight();
	us_clearhighlightcount();
	for(i=0; i<total; i++) startobjectchange((INTBIG)nodelist[i], VNODEINST);
	modifynodeinsts(total, nodelist, dlx, dly, dhx, dhy, dxf, dxf);
	for(i=0; i<total; i++) endobjectchange((INTBIG)nodelist[i], VNODEINST);
	us_pophighlight(TRUE);

	efree((CHAR *)dlx);
	efree((CHAR *)dhx);
	efree((CHAR *)dly);
	efree((CHAR *)dhy);
	efree((CHAR *)dxf);
}

/*********************************** ARRAYING FROM A FILE ***********************************/

#define MAXLINE 200

#define NOARRAYALIGN ((ARRAYALIGN *)-1)

typedef struct Iarrayalign
{
	CHAR *cell;
	CHAR *inport;
	CHAR *outport;
	struct Iarrayalign *nextarrayalign;
} ARRAYALIGN;

#define NOPORTASSOCIATE ((PORTASSOCIATE *)-1)

typedef struct Iportassociate
{
	NODEINST *ni;
	PORTPROTO *pp;
	PORTPROTO *corepp;
	struct Iportassociate *nextportassociate;
} PORTASSOCIATE;

/*
 * Routine to read file "file" and create an array.  The file has this format:
 *   celllibrary LIBFILE [copy]
 *   facet CELLNAME
 *   core CELLNAME
 *   align CELLNAME INPORT OUTPORT
 *   place CELLNAME [gap=DIST] [padport=coreport]* [export padport=padexport]
 *   rotate (c | cc)
 */
void us_arrayfromfile(CHAR *file)
{
	FILE *io;
	CHAR *truename, line[MAXLINE], *pt, *start, save, *par[2],
		*libname, *style, *cellname, *exportname;
	REGISTER INTBIG lineno, gap, gapx, gapy, lx, ly, hx, hy, len, i, copycells, angle;
	INTBIG ax, ay, ox, oy, cx, cy;
	REGISTER PORTPROTO *pp, *exportpp;
	REGISTER LIBRARY *lib, *savelib;
	REGISTER NODEPROTO *np, *cell, *corenp;
	REGISTER NODEINST *ni, *lastni;
	REGISTER ARCINST *ai;
	REGISTER TECHNOLOGY *savetech;
	REGISTER ARRAYALIGN *aa, *firstaa;
	REGISTER PORTASSOCIATE *pa, *firstpa;
	static INTBIG filetypearray = -1;
	REGISTER void *infstr;

	if (filetypearray < 0)
		filetypearray = setupfiletype(x_("arr"), x_("*.arr"), MACFSTAG('TEXT'), FALSE, x_("arrfile"), _("Array"));
	io = xopen(file, filetypearray, 0, &truename);
	if (io == 0) return;

	firstaa = NOARRAYALIGN;
	firstpa = NOPORTASSOCIATE;
	lineno = 0;
	angle = 0;
	copycells = 0;
	cell = corenp = NONODEPROTO;
	lastni = NONODEINST;
	lib = NOLIBRARY;
	for(;;)
	{
		if (xfgets(line, MAXLINE, io) != 0) break;
		lineno++;
		pt = line;
		while (*pt == ' ' || *pt == '\t') pt++;
		if (*pt == 0 || *pt == ';') continue;
		start = pt;
		while (*pt != ' ' && *pt != '\t' && *pt != 0) pt++;
		if (*pt == 0)
		{
			us_abortcommand(_("Line %ld: too short"), lineno);
			break;
		}
		*pt++ = 0;
		if (namesame(start, x_("celllibrary")) == 0)
		{
			while (*pt == ' ' || *pt == '\t') pt++;
			start = pt;
			while (*pt != ' ' && *pt != '\t' && *pt != 0) pt++;
			if (*pt != 0) *pt++ = 0;
			libname = skippath(start);
			style = x_("binary");
			len = estrlen(libname);
			for(i=len-1; i>0; i--) if (libname[i] == '.') break;
			if (i > 0)
			{
				libname[i] = 0;
				if (namesame(&libname[i+1], x_("txt")) == 0) style = x_("text");
			}
			lib = getlibrary(libname);
			if (i > 0) libname[i] = '.';
			if (lib == NOLIBRARY)
			{
				lib = newlibrary(libname, start);
				if (asktool(io_tool, x_("read"), (INTBIG)lib, (INTBIG)style, 0) != 0)
				{
					us_abortcommand(_("Line %ld: cannot read library %s"), lineno,
						start);
					break;
				}
			}
			if (*pt != 0)
			{
				while (*pt == ' ' || *pt == '\t') pt++;
				if (namesame(pt, x_("copy")) == 0) copycells = 1;
			}
			continue;
		}
		if (namesame(start, x_("facet")) == 0)
		{
			while (*pt == ' ' || *pt == '\t') pt++;
			start = pt;
			while (*pt != ' ' && *pt != '\t' && *pt != 0) pt++;
			*pt = 0;
			cell = us_newnodeproto(start, el_curlib);
			if (cell == NONODEPROTO)
			{
				us_abortcommand(_("Line %ld: unable to create cell '%s'"), lineno, start);
				break;
			}
			continue;
		}
		if (namesame(start, x_("core")) == 0)
		{
			while (*pt == ' ' || *pt == '\t') pt++;
			start = pt;
			while (*pt != ' ' && *pt != '\t' && *pt != 0) pt++;
			*pt = 0;
			corenp = getnodeproto(start);
			if (corenp == NONODEPROTO)
			{
				us_abortcommand(_("Line %ld: cannot find core cell '%s'"), lineno, start);
				break;
			}
			continue;
		}
		if (namesame(start, x_("rotate")) == 0)
		{
			while (*pt == ' ' || *pt == '\t') pt++;
			start = pt;
			while (*pt != ' ' && *pt != '\t' && *pt != 0) pt++;
			*pt++ = 0;
			if (namesame(start, x_("c")) == 0) angle = (angle + 2700) % 3600; else
				if (namesame(start, x_("cc")) == 0) angle = (angle + 900) % 3600; else
			{
				us_abortcommand(_("Line %ld: incorrect rotation: %s"), lineno, start);
				break;
			}
			continue;
		}
		if (namesame(start, x_("align")) == 0)
		{
			aa = (ARRAYALIGN *)emalloc(sizeof (ARRAYALIGN), el_tempcluster);
			if (aa == 0) break;
			while (*pt == ' ' || *pt == '\t') pt++;
			start = pt;
			while (*pt != ' ' && *pt != '\t' && *pt != 0) pt++;
			if (*pt == 0)
			{
				us_abortcommand(_("Line %ld: missing 'in port' name"), lineno);
				break;
			}
			*pt++ = 0;
			(void)allocstring(&aa->cell, start, el_tempcluster);

			while (*pt == ' ' || *pt == '\t') pt++;
			start = pt;
			while (*pt != ' ' && *pt != '\t' && *pt != 0) pt++;
			if (*pt == 0)
			{
				us_abortcommand(_("Line %ld: missing 'out port'"), lineno);
				break;
			}
			*pt++ = 0;
			(void)allocstring(&aa->inport, start, el_tempcluster);

			while (*pt == ' ' || *pt == '\t') pt++;
			start = pt;
			while (*pt != ' ' && *pt != '\t' && *pt != 0) pt++;
			*pt = 0;
			(void)allocstring(&aa->outport, start, el_tempcluster);
			aa->nextarrayalign = firstaa;
			firstaa = aa;
			continue;
		}
		if (namesame(start, x_("place")) == 0)
		{
			if (cell == NONODEPROTO)
			{
				us_abortcommand(_("Line %ld: no 'facet' line specified for 'place'"),
					lineno);
				break;
			}
			while (*pt == ' ' || *pt == '\t') pt++;
			start = pt;
			while (*pt != ' ' && *pt != '\t' && *pt != 0) pt++;
			save = *pt;
			*pt = 0;

			if (copycells != 0)
			{
				/* copying pads into this library: see if it is already there */
				np = getnodeproto(start);
				if (np == NONODEPROTO && lib != NOLIBRARY && lib != el_curlib)
				{
					/* not there: copy from pads library */
					savelib = el_curlib;
					el_curlib = lib;
					np = getnodeproto(start);
					el_curlib = savelib;
					if (np != NONODEPROTO)
					{
						np = us_copyrecursively(np, np->protoname,
							el_curlib, np->cellview, FALSE, FALSE, x_(""), FALSE, FALSE, FALSE);
					}
				}
			} else
			{
				/* simply make reference to the pads in the other library */
				infstr = initinfstr();
				formatinfstr(infstr, x_("%s:%s"), lib->libname, start);
				np = getnodeproto(returninfstr(infstr));
			}
			if (np == NONODEPROTO)
			{
				us_abortcommand(_("Line %ld: cannot find cell '%s'"), lineno, start);
				break;
			}
			*pt = save;
			gap = 0;
			exportpp = NOPORTPROTO;
			while (*pt != 0)
			{
				while (*pt == ' ' || *pt == '\t') pt++;
				if (*pt == 0) break;
				start = pt;
				while (*pt != ' ' && *pt != '\t' && *pt != '=' && *pt != 0) pt++;
				save = *pt;
				*pt = 0;
				if (namesame(start, x_("gap")) == 0)
				{
					*pt = save;
					if (*pt != '=')
					{
						us_abortcommand(_("Line %ld: missing '=' after 'gap'"), lineno);
						break;
					}
					pt++;
					while (*pt == ' ' || *pt == '\t') pt++;
					gap = atola(pt, 0);
					while (*pt != ' ' && *pt != '\t' && *pt != 0) pt++;
				} else if (namesame(start, x_("export")) == 0)
				{
					/* export a pad port */
					*pt = save;
					while (*pt == ' ' || *pt == '\t') pt++;
					if (*pt == 0)
					{
						us_abortcommand(_("Line %ld: missing port name after 'export'"),
							lineno);
						break;
					}
					start = pt;
					while (*pt != ' ' && *pt != '\t' && *pt != '=' && *pt != 0) pt++;
					save = *pt;
					*pt = 0;
					exportpp = getportproto(np, start);
					if (exportpp == NOPORTPROTO)
					{
						us_abortcommand(_("Line %ld: no port '%s' on cell '%s'"),
							lineno, start, describenodeproto(np));
						break;
					}
					*pt = save;
					while (*pt == ' ' || *pt == '\t') pt++;
					if (*pt++ != '=')
					{
						us_abortcommand(_("Line %ld: missing '=' name after 'export PORT'"),
							lineno);
						break;
					}
					while (*pt == ' ' || *pt == '\t') pt++;
					exportname = pt;
					while (*pt != ' ' && *pt != '\t' && *pt != 0) pt++;
					save = *pt;
					*pt = 0;
					if (*exportname == 0)
					{
						us_abortcommand(_("Line %ld: missing export name after 'export PORT='"),
							lineno);
						break;
					}
					if (save != 0) pt++;
				} else
				{
					pa = (PORTASSOCIATE *)emalloc(sizeof (PORTASSOCIATE), el_tempcluster);
					if (pa == 0) break;
					pa->ni = NONODEINST;
					pa->pp = getportproto(np, start);
					if (pa->pp == NOPORTPROTO)
					{
						us_abortcommand(_("Line %ld: no port '%s' on cell '%s'"),
							lineno, start, describenodeproto(np));
						break;
					}
					*pt = save;
					if (*pt != '=')
					{
						us_abortcommand(_("Line %ld: missing '=' after pad port name"),
							lineno);
						break;
					}
					pt++;
					while (*pt == ' ' || *pt == '\t') pt++;
					start = pt;
					while (*pt != ' ' && *pt != '\t' && *pt != 0) pt++;
					save = *pt;
					*pt = 0;
					if (corenp == NONODEPROTO)
					{
						us_abortcommand(_("Line %ld: no core cell for association"),
							lineno);
						break;
					}
					pa->corepp = getportproto(corenp, start);
					if (pa->corepp == NOPORTPROTO)
					{
						us_abortcommand(_("Line %ld: no port '%s' on cell '%s'"),
							lineno, start, describenodeproto(corenp));
						break;
					}
					*pt = save;
					pa->nextportassociate = firstpa;
					firstpa = pa;
				}
			}

			/* place the pad */
			if (lastni != NONODEINST)
			{
				/* find the "outport" on the last node */
				cellname = nldescribenodeproto(lastni->proto);
				for(aa = firstaa; aa != NOARRAYALIGN; aa = aa->nextarrayalign)
					if (namesame(aa->cell, cellname) == 0) break;
				if (aa == NOARRAYALIGN)
				{
					us_abortcommand(_("Line %ld: no port alignment given for cell %s"),
						lineno, describenodeproto(lastni->proto));
					break;
				}
				pp = getportproto(lastni->proto, aa->outport);
				if (pp == NOPORTPROTO)
				{
					us_abortcommand(_("Line %ld: no port called '%s' on cell %s"),
						lineno, aa->outport, describenodeproto(lastni->proto));
					break;
				}
				portposition(lastni, pp, &ax, &ay);

				/* find the "inport" on the new node */
				cellname = nldescribenodeproto(np);
				for(aa = firstaa; aa != NOARRAYALIGN; aa = aa->nextarrayalign)
					if (namesame(aa->cell, cellname) == 0) break;
				if (aa == NOARRAYALIGN)
				{
					us_abortcommand(_("Line %ld: no port alignment given for cell %s"),
						lineno, describenodeproto(np));
					break;
				}
				pp = getportproto(np, aa->inport);
				if (pp == NOPORTPROTO)
				{
					us_abortcommand(_("Line %ld: no port called '%s' on cell %s"),
						lineno, aa->inport, describenodeproto(np));
					break;
				}
			}
			corneroffset(NONODEINST, np, angle, 0, &ox, &oy, FALSE);
			lx = ox;   hx = lx + (np->highx - np->lowx);
			ly = oy;   hy = ly + (np->highy - np->lowy);
			ni = newnodeinst(np, lx, hx, ly, hy, 0, angle, cell);
			if (ni == NONODEINST)
			{
				us_abortcommand(_("Line %ld: problem creating %s instance"),
					lineno, describenodeproto(np));
				break;
			}
			if (lastni != NONODEINST)
			{
				switch (angle)
				{
					case 0:    gapx =  gap;   gapy =    0;   break;
					case 900:  gapx =    0;   gapy =  gap;   break;
					case 1800: gapx = -gap;   gapy =    0;   break;
					case 2700: gapx =    0;   gapy = -gap;   break;
				}
				portposition(ni, pp, &ox, &oy);
				modifynodeinst(ni, ax-ox+gapx, ay-oy+gapy, ax-ox+gapx, ay-oy+gapy, 0, 0);
			}
			endobjectchange((INTBIG)ni, VNODEINST);
			if (exportpp != NOPORTPROTO)
			{
				pp = newportproto(cell, ni, exportpp, exportname);
				endobjectchange((INTBIG)pp, VPORTPROTO);
			}
			lastni = ni;

			/* fill in the port associations */
			for(pa = firstpa; pa != NOPORTASSOCIATE; pa = pa->nextportassociate)
				if (pa->ni == NONODEINST) pa->ni = ni;
			continue;
		}
		us_abortcommand(_("Line %ld: unknown keyword '%s'"), lineno, start);
		break;
	}

	/* place the core if one was specified */
	if (corenp != NONODEPROTO)
	{
		(*el_curconstraint->solve)(cell);
		cx = (cell->lowx + cell->highx) / 2;
		cy = (cell->lowy + cell->highy) / 2;
		lx = cx - (corenp->highx - corenp->lowx) / 2;
		ly = cy - (corenp->highy - corenp->lowy) / 2;
		corneroffset(NONODEINST, corenp, 0, 0, &ax, &ay, FALSE);
		cx = lx + ax;   cy = ly + ay;
		savetech = el_curtech;   el_curtech = corenp->tech;
		gridalign(&cx, &cy, 1, cell);
		el_curtech = savetech;
		lx = cx - ax;   ly = cy - ay;
		hx = lx + (corenp->highx - corenp->lowx);
		hy = ly + (corenp->highy - corenp->lowy);

		ni = newnodeinst(corenp, lx, hx, ly, hy, 0, 0, cell);
		if (ni != NONODEINST)
		{
			endobjectchange((INTBIG)ni, VNODEINST);
		}

		/* attach unrouted wires */
		for(pa = firstpa; pa != NOPORTASSOCIATE; pa = pa->nextportassociate)
		{
			if (pa->ni == NONODEINST) continue;
			portposition(ni, pa->corepp, &ox, &oy);
			portposition(pa->ni, pa->pp, &ax, &ay);
			ai = newarcinst(gen_unroutedarc, gen_unroutedarc->nominalwidth,
				us_makearcuserbits(gen_unroutedarc), ni, pa->corepp, ox, oy,
					pa->ni, pa->pp, ax, ay, cell);
			if (ai != NOARCINST)
			{
				endobjectchange((INTBIG)ai, VARCINST);
			}
		}
	}

	/* done with the array file */
	xclose(io);

	/* cleanup memory */
	while (firstpa != NOPORTASSOCIATE)
	{
		pa = firstpa;
		firstpa = pa->nextportassociate;
		efree((CHAR *)pa);
	}
	while (firstaa != NOARRAYALIGN)
	{
		aa = firstaa;
		firstaa = aa->nextarrayalign;
		efree(aa->cell);
		efree(aa->inport);
		efree(aa->outport);
		efree((CHAR *)aa);
	}

	/* show the new cell */
	par[0] = describenodeproto(cell);
	us_editcell(1, par);
}

/*********************************** NODE AND ARC SUPPORT ***********************************/

/*
 * routine to yank the contents of complex node instance "topno" into its
 * parent cell.
 */
void us_yankonenode(NODEINST *topno)
{
	REGISTER NODEINST *ni, *newni, **nodelist;
	REGISTER ARCINST *ai, *newar, **arclist;
	REGISTER PORTARCINST *pi, *nextpi;
	REGISTER PORTEXPINST *pe, *nextpe;
	REGISTER PORTPROTO *pp;
	REGISTER NODEPROTO *np;
	REGISTER ARCPROTO *ap;
	NODEINST *noa[2];
	PORTPROTO *pta[2];
	REGISTER INTBIG wid, i, j, total, oldbits, lowx, highx, lowy, highy;
	XARRAY localtrans, localrot, trans;
	INTBIG nox[2], noy[2], newxc, newyc, xc, yc;
	INTBIG newang;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* make transformation matrix for this cell */
	np = topno->proto;
	maketrans(topno, localtrans);
	makerot(topno, localrot);
	transmult(localtrans, localrot, trans);

	/* build a list of nodes to copy */
	total = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst) total++;
	if (total == 0) return;
	nodelist = (NODEINST **)emalloc((total * (sizeof (NODEINST *))), el_tempcluster);
	if (nodelist == 0) return;
	for(i=0, ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		nodelist[i++] = ni;

	/* sort the nodes by name */
	esort(nodelist, total, sizeof (NODEINST *), us_sortnodesbyname);

	/* copy the nodes */
	for(i=0; i<total; i++)
	{
		ni = nodelist[i];

		/* do not yank "cell center" or "essential bounds" primitives */
		if (ni->proto == gen_cellcenterprim || ni->proto == gen_essentialprim)
		{
			ni->temp1 = (INTBIG)NONODEINST;
			continue;
		}

		/* this "center" computation is unstable for odd size nodes */
		xc = (ni->lowx + ni->highx) / 2;   yc = (ni->lowy + ni->highy) / 2;
		xform(xc, yc, &newxc, &newyc, trans);
		lowx = ni->lowx + newxc - xc;
		lowy = ni->lowy + newyc - yc;
		highx = ni->highx + newxc - xc;
		highy = ni->highy + newyc - yc;
		if (ni->transpose == 0) newang = ni->rotation + topno->rotation; else
			newang = ni->rotation + 3600 - topno->rotation;
		newang = newang % 3600;   if (newang < 0) newang += 3600;
		newni = newnodeinst(ni->proto, lowx, highx, lowy, highy,
			(ni->transpose+topno->transpose)&1, newang, topno->parent);
		if (newni == NONODEINST)
		{
			us_abortcommand(_("Cannot create node in this cell"));
			return;
		}
		ni->temp1 = (INTBIG)newni;
		newni->userbits = ni->userbits;
		TDCOPY(newni->textdescript, ni->textdescript);
		(void)copyvars((INTBIG)ni, VNODEINST, (INTBIG)newni, VNODEINST, TRUE);
		endobjectchange((INTBIG)newni, VNODEINST);
	}
	efree((CHAR *)nodelist);

	/* see if there are any arcs to extract */
	total = 0;
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst) total++;
	if (total != 0)
	{
		/* build a list of arcs to copy */
		arclist = (ARCINST **)emalloc((total * (sizeof (ARCINST *))), el_tempcluster);
		if (arclist == 0) return;
		for(i=0, ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			arclist[i++] = ai;

		/* sort the arcs by name */
		esort(arclist, total, sizeof (ARCINST *), us_sortarcsbyname);

		/* copy the arcs */
		for(i=0; i<total; i++)
		{
			ai = arclist[i];

			/* ignore arcs connected to nodes that didn't get yanked */
			if ((NODEINST *)ai->end[0].nodeinst->temp1 == NONODEINST ||
				(NODEINST *)ai->end[1].nodeinst->temp1 == NONODEINST) continue;

			xform(ai->end[0].xpos, ai->end[0].ypos, &nox[0], &noy[0], trans);
			xform(ai->end[1].xpos, ai->end[1].ypos, &nox[1], &noy[1], trans);

			/* make sure end 0 fits in the port */
			shapeportpoly((NODEINST *)ai->end[0].nodeinst->temp1, ai->end[0].portarcinst->proto, poly, FALSE);
			if (!isinside(nox[0], noy[0], poly))
				portposition((NODEINST *)ai->end[0].nodeinst->temp1,
					ai->end[0].portarcinst->proto, &nox[0], &noy[0]);

			/* make sure end 1 fits in the port */
			shapeportpoly((NODEINST *)ai->end[1].nodeinst->temp1, ai->end[1].portarcinst->proto, poly, FALSE);
			if (!isinside(nox[1], noy[1], poly))
				portposition((NODEINST *)ai->end[1].nodeinst->temp1,
					ai->end[1].portarcinst->proto, &nox[1], &noy[1]);

			newar = newarcinst(ai->proto, ai->width, ai->userbits, (NODEINST *)ai->end[0].nodeinst->temp1,
				ai->end[0].portarcinst->proto, nox[0], noy[0], (NODEINST *)ai->end[1].nodeinst->temp1,
					ai->end[1].portarcinst->proto, nox[1], noy[1], topno->parent);
			if (newar == NOARCINST)
			{
				us_abortcommand(_("Cannot create arc in this cell"));
				return;
			}
			(void)copyvars((INTBIG)ai, VARCINST, (INTBIG)newar, VARCINST, TRUE);
			for(j=0; j<2; j++)
				(void)copyvars((INTBIG)ai->end[j].portarcinst, VPORTARCINST,
					(INTBIG)newar->end[j].portarcinst, VPORTARCINST, FALSE);
			endobjectchange((INTBIG)newar, VARCINST);
		}

		/* replace arcs to this cell */
		for(pi = topno->firstportarcinst; pi != NOPORTARCINST; pi = nextpi)
		{
			/* remember facts about this arcinst */
			nextpi = pi->nextportarcinst;
			ai = pi->conarcinst;  ap = ai->proto;
			if ((ai->userbits&DEADA) != 0) continue;
			wid = ai->width;  oldbits = ai->userbits;
			for(i=0; i<2; i++)
			{
				noa[i] = ai->end[i].nodeinst;
				pta[i] = ai->end[i].portarcinst->proto;
				nox[i] = ai->end[i].xpos;   noy[i] = ai->end[i].ypos;
				if (noa[i] != topno) continue;
				noa[i] = (NODEINST *)ai->end[i].portarcinst->proto->subnodeinst->temp1;
				pta[i] = ai->end[i].portarcinst->proto->subportproto;
			}
			if (noa[0] == NONODEINST || noa[1] == NONODEINST) continue;
			startobjectchange((INTBIG)ai, VARCINST);
			if (killarcinst(ai)) ttyputerr(_("Error killing arc"));
			newar = newarcinst(ap, wid, oldbits, noa[0], pta[0], nox[0], noy[0],
				noa[1], pta[1], nox[1], noy[1], topno->parent);
			if (newar == NOARCINST)
			{
				us_abortcommand(_("Cannot create arc to this cell"));
				return;
			}

			/* copy variables (this presumes killed arc is not yet deallocated) */
			(void)copyvars((INTBIG)ai, VARCINST, (INTBIG)newar, VARCINST, TRUE);
			for(i=0; i<2; i++)
				(void)copyvars((INTBIG)ai->end[i].portarcinst, VPORTARCINST,
					(INTBIG)newar->end[i].portarcinst, VPORTARCINST, FALSE);
			endobjectchange((INTBIG)newar, VARCINST);
		}
		efree((CHAR *)arclist);
	}

	/* replace the exports */
	for(pe = topno->firstportexpinst; pe != NOPORTEXPINST; pe = nextpe)
	{
		nextpe = pe->nextportexpinst;
		pp = pe->proto;
		if ((NODEINST *)pp->subnodeinst->temp1 == NONODEINST) continue;
		if (moveportproto(topno->parent, pe->exportproto,
			(NODEINST *)pp->subnodeinst->temp1, pp->subportproto))
				ttyputerr(_("Moveportproto error"));
	}

	/* copy the exports if requested */
	if ((us_useroptions&DUPCOPIESPORTS) != 0)
	{
		/* initialize for queueing creation of new exports */
		us_initqueuedexports();

		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			ni = (NODEINST *)pp->subnodeinst->temp1;
			if (ni == NONODEINST) continue;

			/* don't copy if the port is already exported */
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
				if (pe->proto == pp->subportproto) break;
			if (pe != NOPORTEXPINST) continue;

			/* copy the port */
			(void)us_queuenewexport(ni, pp->subportproto, pp);
		}

		/* create any queued exports */
		us_createqueuedexports();
	}

	/* delete the cell */
	startobjectchange((INTBIG)topno, VNODEINST);
	if (killnodeinst(topno)) ttyputerr(_("Killnodeinst error"));
}

/*
 * Routine to cut the selected nodes and arcs from the current cell.
 * They are copied to the "clipboard" cell in the "clipboard" library.
 */
void us_cutobjects(WINDOWPART *w)
{
	REGISTER NODEPROTO *np;
	REGISTER GEOM **list;
	REGISTER VIEW *saveview;

	/* get objects to cut */
	np = us_needcell();
	if (np == NONODEPROTO) return;
	list = us_gethighlighted(WANTNODEINST | WANTARCINST, 0, 0);
	if (list[0] == NOGEOM)
	{
		us_abortcommand(_("First select objects to copy"));
		return;
	}

	/* remove contents of clipboard */
	us_clearclipboard();

	/* copy objects to clipboard */
	saveview = us_clipboardcell->cellview;
	us_clipboardcell->cellview = np->cellview;
	us_copylisttocell(list, np, us_clipboardcell, FALSE, FALSE, FALSE);
	us_clipboardcell->cellview = saveview;

	/* then delete it all */
	us_clearhighlightcount();
	us_eraseobjectsinlist(np, list);
}

/*
 * Routine to copy the selected nodes and arcs from the current cell.
 * They are copied to the "clipboard" cell in the "clipboard" library.
 */
void us_copyobjects(WINDOWPART *w)
{
	REGISTER NODEPROTO *np;
	REGISTER GEOM **list;
	REGISTER VIEW *saveview;

	/* get objects to copy */
	np = us_needcell();
	if (np == NONODEPROTO) return;
	list = us_gethighlighted(WANTNODEINST | WANTARCINST, 0, 0);
	if (list[0] == NOGEOM)
	{
		us_abortcommand(_("First select objects to copy"));
		return;
	}

	/* remove contents of clipboard */
	us_clearclipboard();

	/* copy objects to clipboard */
	saveview = us_clipboardcell->cellview;
	us_clipboardcell->cellview = np->cellview;
	us_copylisttocell(list, np, us_clipboardcell, FALSE, FALSE, FALSE);
	us_clipboardcell->cellview = saveview;
}

/*
 * Routine to clear the contents of the clipboard.
 */
void us_clearclipboard(void)
{
	while (us_clipboardcell->firstarcinst != NOARCINST)
	{
		startobjectchange((INTBIG)us_clipboardcell->firstarcinst, VARCINST);
		(void)killarcinst(us_clipboardcell->firstarcinst);
	}
	while (us_clipboardcell->firstportproto != NOPORTPROTO)
		(void)killportproto(us_clipboardcell, us_clipboardcell->firstportproto);
	while (us_clipboardcell->firstnodeinst != NONODEINST)
	{
		startobjectchange((INTBIG)us_clipboardcell->firstnodeinst, VNODEINST);
		killnodeinst(us_clipboardcell->firstnodeinst);
	}
}

/*
 * Routine to paste nodes and arcs from the clipboard to the current cell.
 * They are copied from the "clipboard" cell in the "clipboard" library.
 */
void us_pasteobjects(WINDOWPART *w)
{
	REGISTER NODEPROTO *np;
	REGISTER GEOM **list, **highlist;
	REGISTER INTBIG total, ntotal, atotal, i, overlaid;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER BOOLEAN interactiveplace;

	/* get objects to paste */
	np = us_needcell();
	if (np == NONODEPROTO) return;
	ntotal = atotal = 0;
	for(ni = us_clipboardcell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		ntotal++;
	for(ai = us_clipboardcell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		atotal++;
	total = ntotal + atotal;
	if (total == 0)
	{
		us_abortcommand(_("Nothing in the clipboard to paste"));
		return;
	}

	/* special case of pasting on top of selected objects */
	highlist = us_gethighlighted(WANTNODEINST|WANTARCINST, 0, 0);
	if (highlist[0] != NOGEOM)
	{
		/* can only paste a single object onto selection */
		if (ntotal == 2 && atotal == 1)
		{
			ai = us_clipboardcell->firstarcinst;
			ni = us_clipboardcell->firstnodeinst;
			if (ni == ai->end[0].nodeinst)
			{
				if (ni->nextnodeinst == ai->end[1].nodeinst) ntotal = 0;
			} else if (ni == ai->end[1].nodeinst)
			{
				if (ni->nextnodeinst == ai->end[0].nodeinst) ntotal = 0;
			}
			total = ntotal + atotal;
		}
		if (total > 1)
		{
			ttyputerr(_("Can only paste a single object on top of selected objects"));
			return;
		}
		us_clearhighlightcount();
		overlaid = 0;
		for(i=0; highlist[i] != NOGEOM; i++)
		{
			if (highlist[i]->entryisnode && ntotal == 1)
			{
				ni = highlist[i]->entryaddr.ni;
				highlist[i] = 0;
				ni = us_pastnodetonode(ni, us_clipboardcell->firstnodeinst);
				if (ni != NONODEINST)
				{
					highlist[i] = ni->geom;
					overlaid = 1;
				}
			} else if (!highlist[i]->entryisnode && atotal == 1)
			{
				ai = highlist[i]->entryaddr.ai;
				highlist[i] = 0;
				ai = us_pastarctoarc(ai, us_clipboardcell->firstarcinst);
				if (ai != NOARCINST)
				{
					highlist[i] = ai->geom;
					overlaid = 1;
				}
			}
		}
		if (overlaid == 0)
			ttyputmsg(_("Nothing was pasted"));
		return;
	}

	list = (GEOM **)emalloc((total+1) * (sizeof (GEOM *)), el_tempcluster);
	if (list == 0) return;
	total = 0;
	for(ni = us_clipboardcell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		list[total++] = ni->geom;
	for(ai = us_clipboardcell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		list[total++] = ai->geom;
	list[total] = NOGEOM;

	/* paste them into the current cell */
	interactiveplace = TRUE;
	us_copylisttocell(list, us_clipboardcell, np, TRUE, interactiveplace, FALSE);
	efree((CHAR *)list);
}

/*
 * Routine to "paste" node "srcnode" onto node "destnode", making them the same.
 * Returns the address of the destination node (NONODEINST on error).
 */
NODEINST *us_pastnodetonode(NODEINST *destnode, NODEINST *srcnode)
{
	REGISTER INTBIG dx, dy, dlx, dhx, dly, dhy, i, movebits, newbits;
	REGISTER VARIABLE *srcvar, *destvar;
	REGISTER BOOLEAN checkagain;

	/* make sure they have the same type */
	if (destnode->proto != srcnode->proto)
	{
		destnode = us_replacenodeinst(destnode, srcnode->proto, TRUE, TRUE);
		if (destnode == NONODEINST) return(NONODEINST);
	}

	/* make the sizes the same if they are primitives */
	startobjectchange((INTBIG)destnode, VNODEINST);
	if (destnode->proto->primindex != 0)
	{
		dx = (srcnode->highx - srcnode->lowx) - (destnode->highx - destnode->lowx);
		dy = (srcnode->highy - srcnode->lowy) - (destnode->highy - destnode->lowy);
		if (dx != 0 || dy != 0)
		{
			dlx = -dx/2;   dhx = dx/2;
			dly = -dy/2;   dhy = dy/2;
			modifynodeinst(destnode, dlx, dly, dhx, dhy, 0, 0);
		}
	}

	/* remove variables that are not on the pasted object */
	checkagain = TRUE;
	while (checkagain)
	{
		checkagain = FALSE;
		for(i=0; i<destnode->numvar; i++)
		{
			destvar = &destnode->firstvar[i];
			srcvar = getvalkey((INTBIG)srcnode, VNODEINST, -1, destvar->key);
			if (srcvar != NOVARIABLE) continue;
			delvalkey((INTBIG)destnode, VNODEINST, destvar->key);
			checkagain = TRUE;
			break;
		}
	}

	/* make sure all variables are on the node */
	for(i=0; i<srcnode->numvar; i++)
	{
		srcvar = &srcnode->firstvar[i];
		destvar = setvalkey((INTBIG)destnode, VNODEINST, srcvar->key, srcvar->addr, srcvar->type);
		TDCOPY(destvar->textdescript, srcvar->textdescript);
	}

	/* copy any special user bits */
	movebits = NEXPAND | NTECHBITS | HARDSELECTN | NVISIBLEINSIDE;
	newbits = (destnode->userbits & ~movebits) | (srcnode->userbits & movebits);
	setval((INTBIG)destnode, VNODEINST, x_("userbits"), newbits, VINTEGER);

	endobjectchange((INTBIG)destnode, VNODEINST);
	return(destnode);
}

/*
 * Routine to "paste" arc "srcarc" onto arc "destarc", making them the same.
 * Returns the address of the destination arc (NOARCINST on error).
 */
ARCINST *us_pastarctoarc(ARCINST *destarc, ARCINST *srcarc)
{
	REGISTER INTBIG dw, i, movebits, newbits;
	REGISTER VARIABLE *srcvar, *destvar;
	REGISTER BOOLEAN checkagain;

	/* make sure they have the same type */
	startobjectchange((INTBIG)destarc, VARCINST);
	if (destarc->proto != srcarc->proto)
	{
		destarc = replacearcinst(destarc, srcarc->proto);
		if (destarc == NOARCINST) return(NOARCINST);
	}

	/* make the widths the same */
	dw = srcarc->width - destarc->width;
	if (dw != 0)
		(void)modifyarcinst(destarc, dw, 0, 0, 0, 0);

	/* remove variables that are not on the pasted object */
	checkagain = TRUE;
	while (checkagain)
	{
		checkagain = FALSE;
		for(i=0; i<destarc->numvar; i++)
		{
			destvar = &destarc->firstvar[i];
			srcvar = getvalkey((INTBIG)srcarc, VARCINST, -1, destvar->key);
			if (srcvar != NOVARIABLE) continue;
			delvalkey((INTBIG)destarc, VARCINST, destvar->key);
			checkagain = TRUE;
			break;
		}
	}

	/* make sure all variables are on the arc */
	for(i=0; i<srcarc->numvar; i++)
	{
		srcvar = &srcarc->firstvar[i];
		destvar = setvalkey((INTBIG)destarc, VARCINST, srcvar->key, srcvar->addr, srcvar->type);
		TDCOPY(destvar->textdescript, srcvar->textdescript);
	}

	/* make sure the constraints and other userbits are the same */
	movebits = FIXED | FIXANG | NOEXTEND | ISNEGATED | ISDIRECTIONAL |
		NOTEND0 | NOTEND1 | REVERSEEND | CANTSLIDE | HARDSELECTA;
	newbits = (destarc->userbits & ~movebits) | (srcarc->userbits & movebits);
	setval((INTBIG)destarc, VARCINST, x_("userbits"), newbits, VINTEGER);

	endobjectchange((INTBIG)destarc, VARCINST);
	return(destarc);
}

/*
 * Routine to copy the list of objects in "list" (NOGEOM terminated) from "fromcell"
 * to "tocell".  If "highlight" is true, highlight the objects in the new cell.
 * If "interactiveplace" is true, interactively select the location in the new cell.
 */
void us_copylisttocell(GEOM **list, NODEPROTO *fromcell, NODEPROTO *tocell, BOOLEAN highlight,
	BOOLEAN interactiveplace, BOOLEAN showoffset)
{
	REGISTER NODEINST *ni, *newni, **nodelist;
	REGISTER ARCINST *ai, *newar, **arclist;
	REGISTER PORTEXPINST *pe;
	REGISTER INTBIG i, j, angle;
	REGISTER INTBIG wid, dx, dy, arccount, bits;
	INTBIG xcur, ycur, lx, hx, ly, hy, bestlx, bestly, total;
	BOOLEAN centeredprimitives, first;
	REGISTER void *infstr;
	REGISTER VARIABLE *var;
	static POLYGON *poly = NOPOLYGON;

	/* make sure the destination cell can be modified */
	if (us_cantedit(tocell, NONODEINST, TRUE)) return;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* make sure they are all in the same cell */
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (fromcell != geomparent(list[i]))
		{
			us_abortcommand(_("All duplicated objects must be in the same cell"));
			return;
		}
	}

	/* set the technology of the new cell from the old cell if the new is empty */
	if (tocell->firstnodeinst == NONODEINST ||
		(tocell->firstnodeinst->proto == gen_cellcenterprim &&
			tocell->firstnodeinst->nextnodeinst == NONODEINST))
	{
		tocell->tech = fromcell->tech;
	}

	/* mark all nodes (including those touched by highlighted arcs) */
	for(ni = fromcell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		ni->temp1 = 0;
	for(i=0; list[i] != NOGEOM; i++) if (!list[i]->entryisnode)
	{
		ai = list[i]->entryaddr.ai;
		ai->end[0].nodeinst->temp1 = ai->end[1].nodeinst->temp1 = 1;
	}
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (!list[i]->entryisnode) continue;
		ni = list[i]->entryaddr.ni;

		/* check for cell instance lock */
		if (ni->proto->primindex == 0 && (tocell->userbits&NPILOCKED) != 0)
		{
			if (us_cantedit(tocell, ni, TRUE)) continue;
		}
		ni->temp1 = 1;
	}

	/* count the number of nodes */
	for(total=0, ni = fromcell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		if (ni->temp1 != 0) total++;
	if (total == 0) return;

	/* build a list that includes all nodes touching copied arcs */
	nodelist = (NODEINST **)emalloc((total * (sizeof (NODEINST *))), el_tempcluster);
	if (nodelist == 0) return;
	for(i=0, ni = fromcell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		if (ni->temp1 != 0) nodelist[i++] = ni;

	/* check for recursion */
	for(i=0; i<total; i++)
		if (nodelist[i]->proto->primindex == 0)
	{
		if (isachildof(tocell, nodelist[i]->proto))
		{
			us_abortcommand(_("Cannot: that would be recursive"));
			efree((CHAR *)nodelist);
			return;
		}
	}

	/* figure out lower-left corner of this collection of objects */
	us_getlowleft(nodelist[0], &bestlx, &bestly);
	for(i=1; i<total; i++)
	{
		us_getlowleft(nodelist[i], &lx, &ly);
		if (lx < bestlx) bestlx = lx;
		if (ly < bestly) bestly = ly;
	}
	for(i=0; list[i] != NOGEOM; i++) if (!list[i]->entryisnode)
	{
		ai = list[i]->entryaddr.ai;
		wid = ai->width - arcwidthoffset(ai);
		makearcpoly(ai->length, wid, ai, poly, FILLED);
		getbbox(poly, &lx, &hx, &ly, &hy);
		if (lx < bestlx) bestlx = lx;
		if (ly < bestly) bestly = ly;
	}

	/* adjust this corner so that, after grid alignment, objects are in the same location */
	gridalign(&bestlx, &bestly, 1, tocell);

	/* special case when moving one node: account for cell center */
	if (total == 1 && list[1] == NOGEOM)
	{
		ni = nodelist[0];
		if ((us_useroptions&CENTEREDPRIMITIVES) != 0) centeredprimitives = TRUE; else
			centeredprimitives = FALSE;
		corneroffset(ni, ni->proto, ni->rotation, ni->transpose, &lx, &ly,
			centeredprimitives);
		bestlx = ni->lowx + lx;
		bestly = ni->lowy + ly;

		if (ni->proto->primindex != 0 && (us_useroptions&CENTEREDPRIMITIVES) == 0)
		{
			/* adjust this corner so that, after grid alignment, objects are in the same location */
			gridalign(&bestlx, &bestly, 1, tocell);
		}
	}

	/* remove highlighting if planning to highlight new stuff */
	if (highlight) us_clearhighlightcount();
	if (interactiveplace)
	{
		/* adjust the cursor position if selecting interactively */
		if ((us_tool->toolstate&INTERACTIVE) != 0)
		{
			var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_interactiveanglekey);
			if (var == NOVARIABLE) angle = 0; else
				angle = var->addr;
			us_multidraginit(bestlx, bestly, list, nodelist, total, angle, showoffset);
			trackcursor(FALSE, us_ignoreup, us_multidragbegin, us_multidragdown,
				us_stoponchar, us_multidragup, TRACKDRAGGING);
			if (el_pleasestop != 0)
			{
				efree((CHAR *)nodelist);
				return;
			}
			if (us_demandxy(&xcur, &ycur))
			{
				efree((CHAR *)nodelist);
				return;
			}
			bits = getbuckybits();
			if ((bits&CONTROLDOWN) != 0)
				us_getslide(angle, bestlx, bestly, xcur, ycur, &xcur, &ycur);
		} else
		{
			/* get aligned cursor co-ordinates */
			if (us_demandxy(&xcur, &ycur))
			{
				efree((CHAR *)nodelist);
				return;
			}
		}
		gridalign(&xcur, &ycur, 1, tocell);

		dx = xcur-bestlx;
		dy = ycur-bestly;
	} else
	{
		if (!us_dupdistset)
		{
			us_dupdistset = TRUE;
			us_dupx = us_dupy = el_curlib->lambda[el_curtech->techindex] * 10;
		}
		dx = us_dupx;
		dy = us_dupy;
	}

	/* initialize for queueing creation of new exports */
	us_initqueuedexports();

	/* sort the nodes by name */
	esort(nodelist, total, sizeof (NODEINST *), us_sortnodesbyname);

	/* create the new objects */
	for(i=0; i<total; i++)
	{
		ni = nodelist[i];
		newni = newnodeinst(ni->proto, ni->lowx+dx, ni->highx+dx, ni->lowy+dy,
			ni->highy+dy, ni->transpose, ni->rotation, tocell);
		if (newni == NONODEINST)
		{
			us_abortcommand(_("Cannot create node"));
			efree((CHAR *)nodelist);
			return;
		}
		newni->userbits = ni->userbits & ~(WIPED|NSHORT);
		TDCOPY(newni->textdescript, ni->textdescript);
		(void)copyvars((INTBIG)ni, VNODEINST, (INTBIG)newni, VNODEINST, TRUE);
		endobjectchange((INTBIG)newni, VNODEINST);
		ni->temp1 = (INTBIG)newni;
		if (i == 0) us_dupnode = newni;

		/* copy the ports, too */
		if ((us_useroptions&DUPCOPIESPORTS) != 0)
		{
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
			{
				if (us_queuenewexport(newni, pe->proto, pe->exportproto))
				{
					efree((CHAR *)nodelist);
					return;
				}
			}
		}
	}

	/* create any queued exports */
	us_createqueuedexports();

	/* create a list of arcs to be copied */
	arccount = 0;
	for(i=0; list[i] != NOGEOM; i++)
		if (!list[i]->entryisnode) arccount++;
	if (arccount > 0)
	{
		arclist = (ARCINST **)emalloc(arccount * (sizeof (ARCINST *)), el_tempcluster);
		if (arclist == 0) return;
		arccount = 0;
		for(i=0; list[i] != NOGEOM; i++)
			if (!list[i]->entryisnode)
				arclist[arccount++] = list[i]->entryaddr.ai;

		/* sort the arcs by name */
		esort(arclist, arccount, sizeof (ARCINST *), us_sortarcsbyname);

		for(i=0; i<arccount; i++)
		{
			ai = arclist[i];
			newar = newarcinst(ai->proto, ai->width, ai->userbits,
				(NODEINST *)ai->end[0].nodeinst->temp1, ai->end[0].portarcinst->proto, ai->end[0].xpos+dx,
					ai->end[0].ypos+dy, (NODEINST *)ai->end[1].nodeinst->temp1,
						ai->end[1].portarcinst->proto, ai->end[1].xpos+dx, ai->end[1].ypos+dy, tocell);
			if (newar == NOARCINST)
			{
				us_abortcommand(_("Cannot create arc"));
				efree((CHAR *)nodelist);
				efree((CHAR *)arclist);
				return;
			}
			(void)copyvars((INTBIG)ai, VARCINST, (INTBIG)newar, VARCINST, TRUE);
			for(j=0; j<2; j++)
				(void)copyvars((INTBIG)ai->end[j].portarcinst, VPORTARCINST,
					(INTBIG)newar->end[j].portarcinst, VPORTARCINST, FALSE);
			endobjectchange((INTBIG)newar, VARCINST);
			ai->temp1 = (INTBIG)newar;
		}
		efree((CHAR *)arclist);
	}

	/* highlight the copy */
	if (highlight)
	{
		infstr = initinfstr();
		first = FALSE;
		for(i=0; i<total; i++)
		{
			ni = (NODEINST *)nodelist[i]->temp1;

			/* special case for displayable text on invisible pins */
			if (ni->proto == gen_invispinprim)
			{
				for(j=0; j<ni->numvar; j++)
				{
					var = &ni->firstvar[j];
					if ((var->type&VDISPLAY) != 0) break;
				}
				if (j < ni->numvar)
				{
					if (first) addtoinfstr(infstr, '\n');
					first = TRUE;
					formatinfstr(infstr, x_("CELL=%s TEXT=0%lo;-1;%s"),
						describenodeproto(tocell), (INTBIG)ni->geom, makename(var->key));
					continue;
				}
			}
			if (first) addtoinfstr(infstr, '\n');
			first = TRUE;
			formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0"),
				describenodeproto(tocell), (INTBIG)ni->geom);
		}
		for(i=0; list[i] != NOGEOM; i++) if (!list[i]->entryisnode)
		{
			ai = (ARCINST *)list[i]->entryaddr.ai->temp1;
			addtoinfstr(infstr, '\n');
			formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0"),
				describenodeproto(tocell), (INTBIG)ai->geom);
		}
		us_setmultiplehighlight(returninfstr(infstr), FALSE);
		us_showallhighlight();
	}
	efree((CHAR *)nodelist);
}

/*
 * Initialization routine for queuing export creation.  After this, call "us_queuenewexport"
 * many times, and then "us_createqueuedexports()" to actually create the exports.
 */
void us_initqueuedexports(void)
{
	us_queuedexportcount = 0;
}

/*
 * Routine to queue the creation of an export from port "pp" of node "ni".
 * The port is being copied from an original port "origpp".  Returns true on error.
 */
BOOLEAN us_queuenewexport(NODEINST *ni, PORTPROTO *pp, PORTPROTO *origpp)
{
	REGISTER INTBIG newtotal, i;
	QUEUEDEXPORT  *newqe;

	if (us_queuedexportcount >= us_queuedexporttotal)
	{
		newtotal = us_queuedexporttotal * 2;
		if (newtotal == 0) newtotal = 10;
		newqe = (QUEUEDEXPORT *)emalloc(newtotal * (sizeof (QUEUEDEXPORT)), us_tool->cluster);
		if (newqe == 0) return(TRUE);
		for(i=0; i<us_queuedexportcount; i++)
			newqe[i] = us_queuedexport[i];
		if (us_queuedexporttotal > 0) efree((CHAR *)us_queuedexport);
		us_queuedexport = newqe;
		us_queuedexporttotal = newtotal;
	}
	us_queuedexport[us_queuedexportcount].ni = ni;
	us_queuedexport[us_queuedexportcount].pp = pp;
	us_queuedexport[us_queuedexportcount].origpp = origpp;
	us_queuedexportcount++;
	return(FALSE);
}

/*
 * Helper routine for "esort" that makes queued exports go in ascending name order.
 */
int us_queuedexportnameascending(const void *e1, const void *e2)
{
	REGISTER QUEUEDEXPORT *qe1, *qe2;
	REGISTER PORTPROTO *c1, *c2;

	qe1 = (QUEUEDEXPORT *)e1;
	qe2 = (QUEUEDEXPORT *)e2;
	c1 = qe1->origpp;
	c2 = qe2->origpp;
	return(namesamenumeric(c1->protoname, c2->protoname));
}

/*
 * Helper routine for "esort" that makes arcs with names go in ascending name order.
 */
int us_sortarcsbyname(const void *e1, const void *e2)
{
	REGISTER ARCINST *ai1, *ai2;
	REGISTER VARIABLE *var1, *var2;
	REGISTER CHAR *pt1, *pt2;
	CHAR empty[1];

	ai1 = *((ARCINST **)e1);
	ai2 = *((ARCINST **)e2);
	var1 = getvalkey((INTBIG)ai1, VARCINST, -1, el_arc_name_key);
	var2 = getvalkey((INTBIG)ai2, VARCINST, -1, el_arc_name_key);
	empty[0] = 0;
	if (var1 == NOVARIABLE) pt1 = empty; else pt1 = (CHAR *)var1->addr;
	if (var2 == NOVARIABLE) pt2 = empty; else pt2 = (CHAR *)var2->addr;
	return(namesamenumeric(pt1, pt2));
}

/*
 * Helper routine for "esort" that makes nodes with names go in ascending name order.
 */
int us_sortnodesbyname(const void *e1, const void *e2)
{
	REGISTER NODEINST *ni1, *ni2;
	REGISTER VARIABLE *var1, *var2;
	REGISTER CHAR *pt1, *pt2;
	CHAR empty[1];

	ni1 = *((NODEINST **)e1);
	ni2 = *((NODEINST **)e2);
	var1 = getvalkey((INTBIG)ni1, VNODEINST, -1, el_node_name_key);
	var2 = getvalkey((INTBIG)ni2, VNODEINST, -1, el_node_name_key);
	empty[0] = 0;
	if (var1 == NOVARIABLE) pt1 = empty; else pt1 = (CHAR *)var1->addr;
	if (var2 == NOVARIABLE) pt2 = empty; else pt2 = (CHAR *)var2->addr;
	return(namesamenumeric(pt1, pt2));
}

/*
 * Termination routine for export queueing.  Call this to actually create the
 * queued exports.
 */
void us_createqueuedexports(void)
{
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER PORTPROTO *newpp, *pp, *origpp;
	REGISTER CHAR *portname;
	REGISTER INTBIG i;

	/* sort the ports by their original name */
	esort(us_queuedexport, us_queuedexportcount, sizeof (QUEUEDEXPORT),
		us_queuedexportnameascending);

	for(i=0; i<us_queuedexportcount; i++)
	{
		ni = us_queuedexport[i].ni;
		pp = us_queuedexport[i].pp;
		origpp = us_queuedexport[i].origpp;

		np = ni->parent;
		portname = us_uniqueportname(origpp->protoname, np);
		newpp = us_makenewportproto(np, ni, pp, portname, -1, origpp->userbits, origpp->textdescript);
		if (newpp == NOPORTPROTO) return;
		TDCOPY(newpp->textdescript, origpp->textdescript);
		if (copyvars((INTBIG)origpp, VPORTPROTO, (INTBIG)newpp, VPORTPROTO, FALSE))
			return;
		if (copyvars((INTBIG)origpp->subportexpinst, VPORTEXPINST,
			(INTBIG)newpp->subportexpinst, VPORTEXPINST, FALSE)) return;
	}
}

/*
 * routine to move the arcs in the GEOM module list "list" (terminated by
 * NOGEOM) and the "total" nodes in the list "nodelist" by (dx, dy).
 */
void us_manymove(GEOM **list, NODEINST **nodelist, INTBIG total, INTBIG dx, INTBIG dy)
{
	REGISTER NODEINST *ni, **nis;
	REGISTER ARCINST *ai, *oai;
	REGISTER PORTARCINST *pi;
	REGISTER NODEPROTO *np;
	REGISTER INTBIG i, j, k, otherend, *dlx, *dhx, *dly, *dhy, *drot, *dtrn, valid,
		arcangle;
	NODEINST *nilist[2];
	INTBIG e[2], ix, iy, deltax[2], deltay[2], deltadummy[2];

	/* special case if moving only one node */
	if (total == 1 && list[1] == NOGEOM)
	{
		ni = nodelist[0];
		startobjectchange((INTBIG)ni, VNODEINST);
		modifynodeinst(ni, dx, dy, dx, dy, 0, 0);
		endobjectchange((INTBIG)ni, VNODEINST);
		return;
	}

	/* special case if moving diagonal fixed-angle arcs connected to single manhattan arcs */
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (list[i]->entryisnode) break;
		ai = list[i]->entryaddr.ai;
		if (ai->end[0].xpos == ai->end[1].xpos ||
			ai->end[0].ypos == ai->end[1].ypos) break;
		if ((ai->userbits&FIXANG) == 0) break;
		if ((ai->userbits&FIXED) != 0) break;
		for(j=0; j<2; j++)
		{
			ni = ai->end[j].nodeinst;
			oai = NOARCINST;
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			{
				if (pi->conarcinst == ai) continue;
				if (oai == NOARCINST) oai = pi->conarcinst; else
					oai = NOARCINST;
			}
			if (oai == NOARCINST) break;
			if (oai->end[0].xpos != oai->end[1].xpos &&
				oai->end[0].ypos != oai->end[1].ypos) break;
		}
		if (j < 2) break;
	}
	if (list[i] == NOGEOM)
	{
		/* meets the test: make the special move to slide other orthogonal arcs */
		for(i=0; list[i] != NOGEOM; i++)
		{
			ai = list[i]->entryaddr.ai;
			deltax[0] = deltay[0] = deltax[1] = deltay[1] = 0;
			arcangle = ((ai->userbits&AANGLE)>>AANGLESH) * 10;
			for(j=0; j<2; j++)
			{
				ni = ai->end[j].nodeinst;
				nilist[j] = ni;
				oai = NOARCINST;
				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
					if (pi->conarcinst != ai) break;
				oai = pi->conarcinst;
				if (oai == NOARCINST) break;
				if (oai->end[0].xpos == oai->end[1].xpos)
				{
					intersect(oai->end[0].xpos, oai->end[0].ypos, 900,
						ai->end[0].xpos+dx, ai->end[0].ypos+dy, arcangle, &ix, &iy);
					deltax[j] = ix - ai->end[j].xpos;
					deltay[j] = iy - ai->end[j].ypos;
				} else if (oai->end[0].ypos == oai->end[1].ypos)
				{
					intersect(oai->end[0].xpos, oai->end[0].ypos, 0,
						ai->end[0].xpos+dx, ai->end[0].ypos+dy, arcangle, &ix, &iy);
					deltax[j] = ix - ai->end[j].xpos;
					deltay[j] = iy - ai->end[j].ypos;
				}
			}
			if (j < 2) continue;
			startobjectchange((INTBIG)nilist[0], VNODEINST);
			startobjectchange((INTBIG)nilist[1], VNODEINST);
			deltadummy[0] = deltadummy[1] = 0;
			modifynodeinsts(2, nilist, deltax, deltay, deltax, deltay, deltadummy, deltadummy);
			endobjectchange((INTBIG)nilist[0], VNODEINST);
			endobjectchange((INTBIG)nilist[1], VNODEINST);
		}
		return;
	}

	/* special case if moving only arcs and they slide */
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (list[i]->entryisnode) break;
		ai = list[i]->entryaddr.ai;

		/* see if the arc moves in its ports */
		if ((ai->userbits&(FIXED|CANTSLIDE)) != 0) break;
		if (!db_stillinport(ai, 0, ai->end[0].xpos+dx, ai->end[0].ypos+dy)) break;
		if (!db_stillinport(ai, 1, ai->end[1].xpos+dx, ai->end[1].ypos+dy)) break;
	}
	if (list[i] == NOGEOM) total = 0;

	/* remember the location of every node */
	np = geomparent(list[0]);
	if (np == NONODEPROTO) return;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		ni->temp1 = ni->lowx;
		ni->temp2 = ni->lowy;
	}
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		ai->temp1 = (ai->end[0].xpos + ai->end[1].xpos) / 2;
		ai->temp2 = (ai->end[0].ypos + ai->end[1].ypos) / 2;
	}

	/* look at all nodes and move them appropriately */
	nis = (NODEINST **)emalloc(total * (sizeof (NODEINST *)), el_tempcluster);
	if (nis == 0) return;
	dlx = (INTBIG *)emalloc(total * SIZEOFINTBIG, el_tempcluster);
	if (dlx == 0) return;
	dhx = (INTBIG *)emalloc(total * SIZEOFINTBIG, el_tempcluster);
	if (dhx == 0) return;
	dly = (INTBIG *)emalloc(total * SIZEOFINTBIG, el_tempcluster);
	if (dly == 0) return;
	dhy = (INTBIG *)emalloc(total * SIZEOFINTBIG, el_tempcluster);
	if (dhy == 0) return;
	drot = (INTBIG *)emalloc(total * SIZEOFINTBIG, el_tempcluster);
	if (drot == 0) return;
	dtrn = (INTBIG *)emalloc(total * SIZEOFINTBIG, el_tempcluster);
	if (dtrn == 0) return;
	valid = 0;
	for(i=0; i<total; i++)
	{
		ni = nodelist[i];
		if (ni->lowx == ni->temp1 && ni->lowy == ni->temp2)
		{
			nis[valid] = ni;
			dlx[valid] = dx-(ni->lowx-ni->temp1);
			dly[valid] = dy-(ni->lowy-ni->temp2);
			dhx[valid] = dx-(ni->lowx-ni->temp1);
			dhy[valid] = dy-(ni->lowy-ni->temp2);
			drot[valid] = dtrn[valid] = 0;
			valid++;
		}
	}
	for(j=0; list[j] != NOGEOM; j++)
		if (!list[j]->entryisnode)
			(void)(*el_curconstraint->setobject)((INTBIG)list[j]->entryaddr.ai,
				VARCINST, CHANGETYPETEMPRIGID, 0);
	for(i=0; i<valid; i++)
		startobjectchange((INTBIG)nis[i], VNODEINST);
	modifynodeinsts(valid, nis, dlx, dly, dhx, dhy, drot, dtrn);
	for(i=0; i<valid; i++)
		endobjectchange((INTBIG)nis[i], VNODEINST);
	efree((CHAR *)nis);
	efree((CHAR *)dlx);
	efree((CHAR *)dly);
	efree((CHAR *)dhx);
	efree((CHAR *)dhy);
	efree((CHAR *)drot);
	efree((CHAR *)dtrn);

	/* look at all arcs and move them appropriately */
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (list[i]->entryisnode) continue;
		ai = list[i]->entryaddr.ai;
		if (ai->temp1 != (ai->end[0].xpos + ai->end[1].xpos) / 2 ||
			ai->temp2 != (ai->end[0].ypos + ai->end[1].ypos) / 2) continue;

		/* see if the arc moves in its ports */
		if ((ai->userbits&(FIXED|CANTSLIDE)) != 0) e[0] = e[1] = 0; else
		{
			e[0] = db_stillinport(ai, 0, ai->end[0].xpos+dx, ai->end[0].ypos+dy);
			e[1] = db_stillinport(ai, 1, ai->end[1].xpos+dx, ai->end[1].ypos+dy);
		}

		/* if both ends slide in their port, move the arc */
		if (e[0] != 0 && e[1] != 0)
		{
			startobjectchange((INTBIG)ai, VARCINST);
			(void)modifyarcinst(ai, 0, dx, dy, dx, dy);
			endobjectchange((INTBIG)ai, VARCINST);
			continue;
		}

		/* if neither end can slide in its port, move the nodes */
		if (e[0] == 0 && e[1] == 0)
		{
			for(k=0; k<2; k++)
			{
				ni = ai->end[k].nodeinst;
				if (ni->lowx != ni->temp1 || ni->lowy != ni->temp2) continue;

				/* fix all arcs that aren't sliding */
				for(j=0; list[j] != NOGEOM; j++)
				{
					if (list[j]->entryisnode) continue;
					oai = list[j]->entryaddr.ai;
					if (oai->temp1 != (oai->end[0].xpos + oai->end[1].xpos) / 2 ||
						oai->temp2 != (oai->end[0].ypos + oai->end[1].ypos) / 2) continue;
					if (db_stillinport(oai, 0, ai->end[0].xpos+dx, ai->end[0].ypos+dy) ||
						db_stillinport(oai, 1, ai->end[1].xpos+dx, ai->end[1].ypos+dy))
							continue;
					(void)(*el_curconstraint->setobject)((INTBIG)oai,
						VARCINST, CHANGETYPETEMPRIGID, 0);
				}
				startobjectchange((INTBIG)ni, VNODEINST);
				modifynodeinst(ni, dx-(ni->lowx-ni->temp1), dy-(ni->lowy-ni->temp2),
					dx-(ni->lowx-ni->temp1), dy-(ni->lowy-ni->temp2), 0, 0);
				endobjectchange((INTBIG)ni, VNODEINST);
			}
			continue;
		}

		/* only one end is slidable: move other node and the arc */
		for(k=0; k<2; k++)
		{
			if (e[k] != 0) continue;
			ni = ai->end[k].nodeinst;
			if (ni->lowx == ni->temp1 && ni->lowy == ni->temp2)
			{
				/* node "ni" hasn't moved yet but must because arc motion forces it */
				for(j=0; list[j] != NOGEOM; j++)
				{
					if (list[j]->entryisnode) continue;
					oai = list[j]->entryaddr.ai;
					if (oai->temp1 != (oai->end[0].xpos + oai->end[1].xpos) / 2 ||
						oai->temp2 != (oai->end[0].ypos + oai->end[1].ypos) / 2) continue;
					if (oai->end[0].nodeinst == ni) otherend = 1; else otherend = 0;
					if (db_stillinport(oai, otherend, ai->end[otherend].xpos+dx,
						ai->end[otherend].ypos+dy)) continue;
					(void)(*el_curconstraint->setobject)((INTBIG)oai,
						VARCINST, CHANGETYPETEMPRIGID, 0);
				}
				startobjectchange((INTBIG)ni, VNODEINST);
				modifynodeinst(ni, dx-(ni->lowx-ni->temp1), dy-(ni->lowy-ni->temp2),
					dx-(ni->lowx-ni->temp1), dy-(ni->lowy-ni->temp2), 0, 0);
				endobjectchange((INTBIG)ni, VNODEINST);

				if (ai->temp1 != (ai->end[0].xpos + ai->end[1].xpos) / 2 ||
					ai->temp2 != (ai->end[0].ypos + ai->end[1].ypos) / 2) continue;
				startobjectchange((INTBIG)ai, VARCINST);
				(void)modifyarcinst(ai, 0, dx, dy, dx, dy);
				endobjectchange((INTBIG)ai, VARCINST);
			}
		}
	}
}

/*********************************** PORT SUPPORT ***********************************/

/*
 * routine to obtain details about a "port" command in "count" and "par".
 * The node under consideration is in "ni", and the port under consideration
 * if "ppt" (which is NOPORTPROTO if no particular port is under consideration).
 * The port characteristic bits are set in "bits" and the parts of these bits
 * that are set have mask bits set into "mask".  The port to be affected is
 * returned.  If "wantexp" is true, the desired port should already be
 * exported (otherwise it should not be an export).  If "intendedname" is set,
 * it is the name that will be given to the port when it is exported.  The referenced
 * name is returned in "refname".  The routine returns NOPORTPROTO if there is an error.
 */
PORTPROTO *us_portdetails(PORTPROTO *ppt, INTBIG count, CHAR *par[], NODEINST *ni,
	INTBIG *bits, INTBIG *mask, BOOLEAN wantexp, CHAR *intendedname, CHAR **refname)
{
	REGISTER PORTPROTO *wantpp, *pp;
	HIGHLIGHT high;
	INTBIG x, y;
	REGISTER INTBIG i, l, m, pindex;
	BOOLEAN specify;
	REGISTER INTBIG onlx, only, onhx, onhy, bestx, besty;
	REGISTER PORTEXPINST *pe;
	REGISTER NODEPROTO *np;
	REGISTER VARIABLE *var;
	REGISTER CHAR *pt;
	NODEINST *nilist[1];
	CHAR *newpar;
	static struct
	{
		CHAR  *name;
		INTBIG  significant;
		UINTBIG bits, mask;
	} portparse[] =
	{
		{x_("input"),         1, INPORT,           STATEBITS},
		{x_("output"),        1, OUTPORT,          STATEBITS},
		{x_("bidirectional"), 2, BIDIRPORT,        STATEBITS},
		{x_("power"),         1, PWRPORT,          STATEBITS},
		{x_("ground"),        1, GNDPORT,          STATEBITS},
		{x_("clock1"),        6, C1PORT,           STATEBITS},
		{x_("clock2"),        6, C2PORT,           STATEBITS},
		{x_("clock3"),        6, C3PORT,           STATEBITS},
		{x_("clock4"),        6, C4PORT,           STATEBITS},
		{x_("clock5"),        6, C5PORT,           STATEBITS},
		{x_("clock6"),        6, C6PORT,           STATEBITS},
		{x_("clock"),         1, CLKPORT,          STATEBITS},
		{x_("refout"),        4, REFOUTPORT,       STATEBITS},
		{x_("refin"),         4, REFINPORT,        STATEBITS},
		{x_("refbase"),       4, REFBASEPORT,      STATEBITS},
		{x_("none"),          1, 0,                STATEBITS|PORTDRAWN|BODYONLY},
		{x_("always-drawn"),  1, PORTDRAWN,        PORTDRAWN},
		{x_("body-only"),     2, BODYONLY,         BODYONLY},
		{NULL, 0, 0, 0}
	};

	/* quick sanity check first */
	np = ni->proto;
	if (np->firstportproto == NOPORTPROTO)
	{
		us_abortcommand(_("This node has no ports"));
		return(NOPORTPROTO);
	}

	/* prepare to parse parameters */
	wantpp = NOPORTPROTO;
	specify = FALSE;
	*refname = x_("");
	*bits = *mask = 0;

	/* look at all parameters */
	for(i=0; i<count; i++)
	{
		l = estrlen(pt = par[i]);

		/* see if a referenced name is given */
		if (namesamen(pt, x_("refname"), l) == 0 && l >= 4 && i+1 < count)
		{
			i++;
			*refname = par[i];
			continue;
		}

		/* check the basic characteristics from the table */
		for(m=0; portparse[m].name != 0; m++)
			if (namesamen(pt, portparse[m].name, l) == 0 && l >= portparse[m].significant)
		{
			*bits |= portparse[m].bits;
			*mask |= portparse[m].mask;
			break;
		}
		if (portparse[m].name != 0) continue;

		if (namesamen(pt, x_("specify"), l) == 0 && l >= 1)
		{ specify = TRUE; continue; }
		if (namesamen(pt, x_("use"), l) == 0 && l >= 1)
		{
			if (i+1 >= count)
			{
				ttyputusage(x_("port use PORTNAME"));
				return(NOPORTPROTO);
			}
			i++;
			if (!wantexp)
			{
				/* want to export: look for any port on the node */
				wantpp = getportproto(np, par[i]);
				if (wantpp == NOPORTPROTO)
				{
					us_abortcommand(_("No port called %s"), par[i]);
					return(NOPORTPROTO);
				}
			} else
			{
				/* want exports: look specificially for them */
				for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
					if (namesame(pe->exportproto->protoname, par[i]) == 0) break;
				if (pe != NOPORTEXPINST) wantpp = pe->exportproto; else
				{
					us_abortcommand(_("No exports called %s"), par[i]);
					return(NOPORTPROTO);
				}
			}
			continue;
		}
		ttyputbadusage(x_("port"));
		return(NOPORTPROTO);
	}

	/* if no port explicitly found, use default (if any) */
	if (wantpp == NOPORTPROTO) wantpp = ppt;

	/* if no port found and heuristics are allowed, try them */
	if (wantpp == NOPORTPROTO && !specify)
	{
		/* if there is only one possible port, use it */
		if (!wantexp)
		{
			/* look for only nonexport */
			pindex = 0;
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			{
				for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
					if (pe->proto == pp) break;
				if (pe != NOPORTEXPINST) continue;
				pindex++;
				wantpp = pp;
			}
			if (pindex != 1) wantpp = NOPORTPROTO;
		} else
		{
			/* if there is one export, return it */
			pe = ni->firstportexpinst;
			if (pe != NOPORTEXPINST && pe->nextportexpinst == NOPORTEXPINST)
				wantpp = pe->exportproto;
		}

		/* if a port is highlighted, use it */
		if (wantpp == NOPORTPROTO)
		{
			var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
			if (var != NOVARIABLE)
			{
				if (getlength(var) == 1)
				{
					(void)us_makehighlight(((CHAR **)var->addr)[0], &high);
					if ((high.status&HIGHTYPE) == HIGHFROM && high.fromport != NOPORTPROTO)
					{
						pp = high.fromport;
						if (!wantexp) wantpp = pp; else
						{
							for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
								if (pe->proto == pp) break;
							if (pe != NOPORTEXPINST) wantpp = pe->exportproto; else
							{
								us_abortcommand(_("Port %s must be an export first"), pp->protoname);
								return(NOPORTPROTO);
							}
						}
					}
				}
			}
		}

		/* if exporting port with the same name as the subportinst, use it */
		if (wantpp == NOPORTPROTO && *intendedname != 0 && !wantexp)
		{
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				if (namesame(intendedname, pp->protoname) == 0)
			{
				wantpp = pp;
				break;
			}
		}

		/* if port is on one edge of the cell and is being exported, use it */
		if (wantpp == NOPORTPROTO && !wantexp)
		{
			if (ni->geom->lowx == ni->parent->lowx) onlx = 1; else onlx = 0;
			if (ni->geom->highx == ni->parent->highx) onhx = 1; else onhx = 0;
			if (ni->geom->lowy == ni->parent->lowy) only = 1; else only = 0;
			if (ni->geom->highy == ni->parent->highy) onhy = 1; else onhy = 0;
			if (onlx+onhx+only+onhy == 1)
			{
				/* look for one port on the node that is on the proper edge */
				bestx = (ni->lowx+ni->highx)/2;
				besty = (ni->lowy+ni->highy)/2;
				wantpp = NOPORTPROTO;
				for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				{
					portposition(ni, pp, &x, &y);
					if (onlx != 0 && x == bestx) wantpp = NOPORTPROTO;
					if (onlx != 0 && x < bestx)
					{
						wantpp = pp;  bestx = x;
					}
					if (onhx != 0 && x == bestx) wantpp = NOPORTPROTO;
					if (onhx != 0 && x > bestx)
					{
						wantpp = pp;  bestx = x;
					}
					if (only != 0 && y == besty) wantpp = NOPORTPROTO;
					if (only != 0 && y < besty)
					{
						wantpp = pp;  besty = y;
					}
					if (onhy != 0 && y == besty) wantpp = NOPORTPROTO;
					if (onhy != 0 && y > besty)
					{
						wantpp = pp;  besty = y;
					}
				}
			}
		}
	}

	/* if port is on offpage connector, use characteristics to choose the end */
	if (wantpp == NOPORTPROTO && ni->proto == sch_offpageprim)
	{
		if (((*bits)&STATEBITS) == OUTPORT) wantpp = ni->proto->firstportproto->nextportproto; else
			wantpp = ni->proto->firstportproto;
	}

	/* give up and ask the port name wanted */
	if (wantpp == NOPORTPROTO)
	{
		nilist[0] = ni;
		us_identifyports(1, nilist, ni->proto, LAYERA, TRUE);
		ttyputerr(_("Which port of node %s is to be the port:"), describenodeproto(np));
		pindex = 0;
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			ttyputmsg(x_("%ld: %s"), ++pindex, pp->protoname);
		for(;;)
		{
			newpar = ttygetline(_("Select a number: "));
			if (newpar == 0 || *newpar == 0)
			{
				us_abortedmsg();
				break;
			}
			i = eatoi(newpar);
			if (i <= 0 || i > pindex)
			{
				ttyputerr(_("Please select a number from 1 to %ld (default aborts)"), pindex);
				continue;
			}

			/* convert to a port */
			x = 0;
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				if (++x == i) break;
			if (!wantexp) wantpp = pp; else
			{
				for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
					if (pe->proto == pp) break;
				if (pe == NOPORTEXPINST)
				{
					ttyputerr(_("That port is not an export"));
					continue;
				}
				wantpp = pe->exportproto;
			}
			break;
		}
		us_identifyports(1, nilist, ni->proto, ALLOFF, TRUE);
	}

	/* finally, return the port */
	return(wantpp);
}

/*
 * routine to recursively delete ports at nodeinst "ni" and all arcs connected
 * to them anywhere.  If "spt" is not NOPORTPROTO, delete only that portproto
 * on this nodeinst (and its hierarchically related ports).  Otherwise delete
 * all portprotos on this nodeinst.
 */
void us_undoportproto(NODEINST *ni, PORTPROTO *spt)
{
	REGISTER PORTEXPINST *pe, *nextpe;

	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = nextpe)
	{
		nextpe = pe->nextportexpinst;
		if (spt != NOPORTPROTO && spt != pe->exportproto) continue;
		if (killportproto(pe->exportproto->parent, pe->exportproto))
			ttyputerr(_("killportproto error"));
	}
}

/*
 * Routine to synchronize the ports in cells in the current library with
 * like-named cells in library "olib".
 */
void us_portsynchronize(LIBRARY *olib)
{
	REGISTER NODEPROTO *np, *onp;
	REGISTER PORTPROTO *pp, *opp;
	REGISTER INTBIG lx, hx, ly, hy, newports, nocells;
	REGISTER NODEINST *ni, *oni;

	newports = nocells = 0;
	for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		/* find this cell in the other library */
		for(onp = olib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
		{
			if (namesame(np->protoname, onp->protoname) != 0) continue;

			/* synchronize the ports */
			for(opp = onp->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
			{
				/* see if that other cell's port is in this one */
				for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
					if (namesame(opp->protoname, pp->protoname) == 0) break;
				if (pp != NOPORTPROTO) continue;

				/* must add port "opp" to cell "np" */
				oni = opp->subnodeinst;
				if (oni->proto->primindex == 0)
				{
					if (nocells == 0)
						ttyputerr(_("Cannot yet make exports that come from other cell instances (i.e. export %s in cell %s)"),
							opp->protoname, describenodeproto(onp));
					nocells = 1;
					continue;
				}

				/* presume that the cells have the same coordinate system */
				lx = oni->lowx;
				hx = oni->highx;
				ly = oni->lowy;
				hy = oni->highy;

				ni = newnodeinst(oni->proto, lx, hx, ly, hy, oni->transpose, oni->rotation, np);
				if (ni == NONODEINST) continue;
				pp = newportproto(np, ni, opp->subportproto, opp->protoname);
				if (pp == NOPORTPROTO) return;
				pp->userbits = opp->userbits;
				TDCOPY(pp->textdescript, opp->textdescript);
				if (copyvars((INTBIG)opp, VPORTPROTO, (INTBIG)pp, VPORTPROTO, FALSE))
					return;
				endobjectchange((INTBIG)ni, VNODEINST);
				newports++;
			}
		}
	}
	ttyputmsg(_("Created %ld new %s"), newports, makeplural(_("port"), newports));
}

/*
 * routine to determine a path down from the currently highlighted port.  Returns
 * the subnode and subport in "hini" and "hipp" (sets them to NONODEINST and
 * NOPORTPROTO if no lower path is defined).
 */
void us_findlowerport(NODEINST **hini, PORTPROTO **hipp)
{
	HIGHLIGHT high;
	REGISTER VARIABLE *var;
	REGISTER INTBIG len;
	NODEINST *ni;
	REGISTER NODEPROTO *np, *onp;
	PORTPROTO *pp;

	/* presume no lower port */
	*hini = NONODEINST;
	*hipp = NOPORTPROTO;

	/* must be 1 highlighted object */
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var == NOVARIABLE) return;
	len = getlength(var);
	if (len > 1) return;

	/* get the highlighted object */
	if (us_makehighlight(((CHAR **)var->addr)[0], &high)) return;

	/* see if it is a port */
	if ((high.status&HIGHTYPE) == HIGHTEXT)
	{
		if (high.fromvar != NOVARIABLE || high.fromport == NOPORTPROTO) return;
		pp = high.fromport->subportproto;
	} else
	{
		if ((high.status&HIGHTYPE) != HIGHFROM || high.fromport == NOPORTPROTO) return;
		pp = high.fromport;
	}

	/* see if port is on instance */
	ni = high.fromgeom->entryaddr.ni;
	np = ni->proto;
	if (np->primindex != 0) return;

	/* describe source of the port */
	*hini = pp->subnodeinst;
	*hipp = pp->subportproto;

	/* see if port is on an icon */
	if (np->cellview == el_iconview && !isiconof(np, ni->parent))
	{
		onp = contentsview(np);
		if (onp != NONODEPROTO)
		{
			*hipp = equivalentport(np, pp, onp);
			if (*hipp != NOPORTPROTO)
			{
				*hini = (*hipp)->subnodeinst;
				*hipp = (*hipp)->subportproto;
			}
		}
	}
}

/*
 * Routine to interactively select an instance of "inp" higher in the hierarchy.
 * Returns the instance that was selected (NONODEINST if none).
 */
NODEINST *us_pickhigherinstance(NODEPROTO *inp)
{
	REGISTER NODEPROTO **newcelllist;
	REGISTER NODEINST *ni, **newinstlist;
	REGISTER POPUPMENU *pm;
	POPUPMENU *cpopup;
	REGISTER POPUPMENUITEM *mi, *selected;
	BOOLEAN butstate;
	REGISTER INTBIG cellcount, i, k, *newinstcount;
	CHAR buf[50];
	REGISTER void *infstr;

	/* make a list of choices, up the hierarchy */
	cellcount = 0;
	for(ni = inp->firstinst; ni != NONODEINST; ni = ni->nextinst)
	{
		/* ignore if this is an icon in a contents */
		if (isiconof(ni->proto, ni->parent)) continue;

		/* ignore instances on the clipboard */
		if ((ni->parent->lib->userbits&HIDDENLIBRARY) != 0) continue;

		/* ignore this instance if it is a duplicate */
		for(i=0; i<cellcount; i++) if (us_pickinstcelllist[i] == ni->parent) break;
		if (i < cellcount)
		{
			us_pickinstinstcount[i]++;
			continue;
		}

		/* ensure room in the list */
		if (cellcount >= us_pickinstlistsize)
		{
			k = us_pickinstlistsize + 32;
			newcelllist = (NODEPROTO **)emalloc(k * (sizeof (NODEPROTO *)), us_tool->cluster);
			newinstlist = (NODEINST **)emalloc(k * (sizeof (NODEINST *)), us_tool->cluster);
			newinstcount = (INTBIG *)emalloc(k * SIZEOFINTBIG, us_tool->cluster);
			if (newcelllist == 0 || newinstlist == 0 || newinstcount == 0) return(0);
			for(i=0; i<cellcount; i++)
			{
				newcelllist[i] = us_pickinstcelllist[i];
				newinstlist[i] = us_pickinstinstlist[i];
				newinstcount[i] = us_pickinstinstcount[i];
			}
			if (us_pickinstlistsize != 0)
			{
				efree((CHAR *)us_pickinstcelllist);
				efree((CHAR *)us_pickinstinstlist);
				efree((CHAR *)us_pickinstinstcount);
			}
			us_pickinstcelllist = newcelllist;
			us_pickinstinstlist = newinstlist;
			us_pickinstinstcount = newinstcount;
			us_pickinstlistsize = k;
		}

		us_pickinstcelllist[cellcount] = ni->parent;
		us_pickinstinstlist[cellcount]  = ni;
		us_pickinstinstcount[cellcount] = 1;
		cellcount++;
	}

	/* if no instances of this cell found, exit */
	if (cellcount == 0) return(NONODEINST);

	/* if only one instance, answer is easy */
	if (cellcount == 1)
	{
		return(us_pickinstinstlist[0]);
	}

	/* make a menu of all cells connected to this export */
	pm = (POPUPMENU *)emalloc(sizeof(POPUPMENU), el_tempcluster);
	if (pm == 0) return(NONODEINST);

	mi = (POPUPMENUITEM *)emalloc(cellcount * (sizeof (POPUPMENUITEM)), el_tempcluster);
	if (mi == 0)
	{
		efree((CHAR *)pm);
		return(NONODEINST);
	}
	for (i=0; i<cellcount; i++)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, describenodeproto(us_pickinstcelllist[i]));
		if (us_pickinstinstcount[i] > 1)
		{
			(void)esnprintf(buf, 50, _(" (%ld instances)"), us_pickinstinstcount[i]);
			addstringtoinfstr(infstr, buf);
		}
		(void)allocstring(&mi[i].attribute, returninfstr(infstr), el_tempcluster);
		mi[i].value = 0;
		mi[i].valueparse = NOCOMCOMP;
		mi[i].maxlen = -1;
		mi[i].response = NOUSERCOM;
		mi[i].changed = FALSE;
	}
	pm->name = x_("noname");
	pm->list = mi;
	pm->total = cellcount;
	pm->header = _("Which cell up the hierarchy?");

	/* display and select from the menu */
	butstate = FALSE;
	cpopup = pm;
	selected = us_popupmenu(&cpopup, &butstate, TRUE, -1, -1, 0);

	/* free up allocated menu space */
	for (k=0; k<cellcount; k++)
		efree(mi[k].attribute);
	efree((CHAR *)mi);
	efree((CHAR *)pm);

	/* stop if display doesn't support popup menus */
	if (selected == 0) return(NONODEINST);
	if (selected == NOPOPUPMENUITEM) return(NONODEINST);
	for (i=0; i<cellcount; i++)
	{
		if (selected != &mi[i]) continue;
		return(us_pickinstinstlist[i]);
	}

	return(NONODEINST);
}

/*
 * routine to re-export port "pp" on nodeinst "ni".  Returns true if there
 * is an error
 */
BOOLEAN us_reexportport(PORTPROTO *pp, NODEINST *ni)
{
	REGISTER VARIABLE *var;
	CHAR *portname, *sportname, *pt;
	REGISTER PORTPROTO *ppt;
	REGISTER void *infstr;

	/* generate an initial guess for the new port name */
	infstr = initinfstr();

	/* add in local node name if applicable */
	var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
	if (var != NOVARIABLE)
	{
		/* see if the original name has array markers */
		for(pt = pp->protoname; *pt != 0; pt++)
			if (*pt == '[') break;
		if (*pt == '[')
		{
			/* arrayed name: add node name to array index */
			*pt = 0;
			addstringtoinfstr(infstr, pp->protoname);
			addstringtoinfstr(infstr, (CHAR *)var->addr);
			*pt = '[';
			addstringtoinfstr(infstr, pt);
		} else
		{
			/* simple name: add node name to the end */
			addstringtoinfstr(infstr, pp->protoname);
			addstringtoinfstr(infstr, (CHAR *)var->addr);
		}
	} else
	{
		/* no node name: just gather the export name */
		addstringtoinfstr(infstr, pp->protoname);
	}

	(void)allocstring(&sportname, returninfstr(infstr), el_tempcluster);
	portname = us_uniqueportname(sportname, ni->parent);
	efree(sportname);

	/* make the export */
	ttyputmsg(_("Making export %s from node %s, port %s"), portname, describenodeinst(ni), pp->protoname);
	startobjectchange((INTBIG)ni, VNODEINST);
	ppt = newportproto(ni->parent, ni, pp, portname);
	if (ppt == NOPORTPROTO)
	{
		us_abortcommand(_("Error creating export %s"), portname);
		return(TRUE);
	}
	TDCOPY(ppt->textdescript, pp->textdescript);
	ppt->userbits = pp->userbits;
	if (copyvars((INTBIG)pp, VPORTPROTO, (INTBIG)ppt, VPORTPROTO, FALSE))
		return(FALSE);
	endobjectchange((INTBIG)ni, VNODEINST);
	return(FALSE);
}

/*
 * routine to rename port "pp" to be "pt"
 */
void us_renameport(PORTPROTO *pp, CHAR *pt)
{
	CHAR *ch, *newname;
	REGISTER BOOLEAN badname;
	REGISTER PORTPROTO *opp, *app;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np, *anp;

	(void)allocstring(&newname, pt, us_tool->cluster);
	badname = FALSE;
	for(ch = newname; *ch != 0; ch++)
	{
		if (*ch > ' ' && *ch < 0177) continue;
		*ch = 'X';
		badname = TRUE;
	}
	if (badname)
		ttyputerr(_("Port has invalid characters, renamed to '%s'"), newname);

	/* check for duplicate name */
	if (estrcmp(pp->protoname, newname) == 0)
	{
		ttyputmsg(_("Port name has not changed"));
		efree((CHAR *)newname);
		return;
	}

	np = pp->parent;
	opp = getportproto(np, newname);
	if (opp == pp) opp = NOPORTPROTO;
	if (opp == NOPORTPROTO) ch = newname; else
	{
		ch = us_uniqueportname(newname, np);
		ttyputmsg(_("Already a port called %s, calling this %s"), newname, ch);
	}

	/* see if an associated icon/contents cell will also be affected */
	if (np->cellview == el_iconview) anp = contentsview(np); else
		anp = iconview(np);
	if (anp == NONODEPROTO) app = NOPORTPROTO; else
	{
		app = equivalentport(np, pp, anp);
		opp = getportproto(anp, ch);
		if (opp != NOPORTPROTO && opp != app) app = NOPORTPROTO;
	}

	/* erase all instances of this nodeproto on display */
	startobjectchange((INTBIG)pp->subnodeinst, VNODEINST);
	for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
	{
		if ((ni->userbits & NEXPAND) == 0) startobjectchange((INTBIG)ni, VNODEINST);
	}
	if (app != NOPORTPROTO)
	{
		startobjectchange((INTBIG)app->subnodeinst, VNODEINST);
		for(ni = anp->firstinst; ni != NONODEINST; ni = ni->nextinst)
		{
			/* mark the library with the instance as "changed" */
			ni->parent->lib->userbits |= LIBCHANGEDMAJOR;

			/* redraw if not expanded */
			if ((ni->userbits & NEXPAND) == 0) startobjectchange((INTBIG)ni, VNODEINST);
		}
	}

	/* change the export name */
	ttyputverbose(M_("Export %s renamed to %s"), pp->protoname, ch);
	(void)setval((INTBIG)pp, VPORTPROTO, x_("protoname"), (INTBIG)ch, VSTRING);
	if (app != NOPORTPROTO)
		(void)setval((INTBIG)app, VPORTPROTO, x_("protoname"), (INTBIG)ch, VSTRING);

	/* redraw all instances of this nodeproto on display */
	for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
	{
		if ((ni->userbits & NEXPAND) == 0) endobjectchange((INTBIG)ni, VNODEINST);
	}
	endobjectchange((INTBIG)pp->subnodeinst, VNODEINST);

	/* tell the network maintainter to reevaluate this cell */
	(void)asktool(net_tool, x_("re-number"), (INTBIG)np);

	if (app != NOPORTPROTO)
	{
		for(ni = anp->firstinst; ni != NONODEINST; ni = ni->nextinst)
		{
			if ((ni->userbits & NEXPAND) == 0) endobjectchange((INTBIG)ni, VNODEINST);
		}
		endobjectchange((INTBIG)app->subnodeinst, VNODEINST);

		/* tell the network maintainter to reevaluate this cell */
		(void)asktool(net_tool, x_("re-number"), (INTBIG)anp);
	}
	efree((CHAR *)newname);
}

/*
 * routine to create a port in cell "np" called "portname".  The port resides on
 * node "ni", port "pp" in the cell.  The userbits field will have "bits" set where
 * "mask" points.  The text descriptor is "textdescript".
 * Also, check across icon/contents boundary to create a parallel port if possible
 */
PORTPROTO *us_makenewportproto(NODEPROTO *np, NODEINST *ni, PORTPROTO *pp,
	CHAR *portname, INTBIG mask, INTBIG bits, UINTBIG *textdescript)
{
	REGISTER NODEPROTO *onp;
	REGISTER NODEINST *boxni;
	REGISTER VARIABLE *var;
	INTBIG xpos, ypos, xbbpos, ybbpos, x, y, portcount, portlen, *newportlocation,
		lambda, style;
	REGISTER INTBIG boxlx, boxhx, boxly, boxhy, rangelx, rangehx, leadlength,
		rangely, rangehy, index;
	REGISTER PORTPROTO *ppt, *opp;

	startobjectchange((INTBIG)ni, VNODEINST);
	ppt = newportproto(np, ni, pp, portname);
	if (ppt == NOPORTPROTO)
	{
		us_abortcommand(_("Error creating the port"));
		us_pophighlight(FALSE);
		return(NOPORTPROTO);
	}
	if ((mask&STATEBITS) != 0) ppt->userbits = (ppt->userbits & ~STATEBITS) | (bits & STATEBITS);
	if ((mask&PORTDRAWN) != 0) ppt->userbits = (ppt->userbits & ~PORTDRAWN) | (bits & PORTDRAWN);
	if ((mask&BODYONLY) != 0)  ppt->userbits = (ppt->userbits & ~BODYONLY) | (bits & BODYONLY);
	if (ni->proto->primindex == 0)
		TDCOPY(ppt->textdescript, textdescript);

	/* if this is a port on an offpage connector, adjust the position sensibly */
	if (ni->proto == sch_offpageprim)
	{
		if (namesame(pp->protoname, "y") == 0) TDSETPOS(ppt->textdescript, VTPOSRIGHT); else
			TDSETPOS(ppt->textdescript, VTPOSLEFT);
	}
	endobjectchange((INTBIG)ni, VNODEINST);

	/* ignore new port if not intended for icon */
	if ((pp->userbits&BODYONLY) != 0) return(ppt);

	/* see if there is an associated icon cell */
	onp = NONODEPROTO;
	if (np->cellview != el_iconview) onp = iconview(np);
	if (onp == NONODEPROTO) return(ppt);

	/* icon cell found, quit if this port is already there */
	opp = getportproto(onp, ppt->protoname);
	if (opp != NOPORTPROTO) return(ppt);

	/* get icon style controls */
	var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_icon_style"));
	if (var != NOVARIABLE) style = var->addr; else style = ICONSTYLEDEFAULT;
	var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_icon_lead_length"));
	if (var != NOVARIABLE) leadlength = var->addr; else leadlength = ICONLEADDEFLEN;

	/* find the box in the icon cell */
	for(boxni = onp->firstnodeinst; boxni != NONODEINST; boxni = boxni->nextnodeinst)
		if (boxni->proto == sch_bboxprim) break;
	if (boxni == NONODEINST)
	{
		boxlx = boxhx = (onp->lowx + onp->highx) / 2;
		boxly = boxhy = (onp->lowy + onp->highy) / 2;
		rangelx = onp->lowx;
		rangehx = onp->highx;
		rangely = onp->lowy;
		rangehy = onp->highy;
	} else
	{
		rangelx = boxlx = boxni->lowx;
		rangehx = boxhx = boxni->highx;
		rangely = boxly = boxni->lowy;
		rangehy = boxhy = boxni->highy;
	}

	/* count the number of ports */
	portlen = 0;
	for(opp = onp->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto) portlen++;

	if (portlen > us_netportlimit)
	{
		newportlocation = (INTBIG *)emalloc(portlen * SIZEOFINTBIG, us_tool->cluster);
		if (newportlocation == 0) return(ppt);
		if (us_netportlimit > 0) efree((CHAR *)us_netportlocation);
		us_netportlocation = newportlocation;
		us_netportlimit = portlen;
	}
	portcount = 0;
	lambda = el_curlib->lambda[sch_tech->techindex];
	index = us_iconposition(ppt, style);
	switch (index)
	{
		case 0:		/* left side */
			for(opp = onp->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
			{
				portposition(opp->subnodeinst, opp->subportproto, &x, &y);
				if (opp == onp->firstportproto) xpos = x; else
				{
					if (x < xpos) xpos = x;
				}
				if (x < boxlx) us_netportlocation[portcount++] = y;
			}
			ybbpos = ypos = us_findnewplace(us_netportlocation, portcount, rangely, rangehy);
			xbbpos = boxlx;
			break;
		case 1:		/* right side */
			for(opp = onp->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
			{
				portposition(opp->subnodeinst, opp->subportproto, &x, &y);
				if (opp == onp->firstportproto) xpos = x; else
				{
					if (x > xpos) xpos = x;
				}
				if (x > boxhx) us_netportlocation[portcount++] = y;
			}
			ybbpos = ypos = us_findnewplace(us_netportlocation, portcount, rangely, rangehy);
			xbbpos = boxhx;
			break;
		case 2:		/* top */
			for(opp = onp->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
			{
				portposition(opp->subnodeinst, opp->subportproto, &x, &y);
				if (opp == onp->firstportproto) ypos = y; else
				{
					if (y > ypos) ypos = y;
				}
				if (y > boxhy) us_netportlocation[portcount++] = x;
			}
			xbbpos = xpos = us_findnewplace(us_netportlocation, portcount, rangelx, rangehx);
			ybbpos = boxhy;
			break;
		case 3:		/* bottom */
			for(opp = onp->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
			{
				portposition(opp->subnodeinst, opp->subportproto, &x, &y);
				if (opp == onp->firstportproto) ypos = y; else
				{
					if (y < ypos) ypos = y;
				}
				if (y < boxly) us_netportlocation[portcount++] = x;
			}
			xbbpos = xpos = us_findnewplace(us_netportlocation, portcount, rangelx, rangehx);
			ybbpos = boxly;
			break;
	}

	/* make the icon export */
	gridalign(&xpos, &ypos, 1, onp);
	gridalign(&xbbpos, &ybbpos, 1, onp);
	(void)us_makeiconexport(ppt, style, index, xpos, ypos, xbbpos, ybbpos, onp);
	return(ppt);
}

/*
 * routine to find the largest gap in an integer array that is within the bounds
 * "low" to "high".  The array is sorted by this routine.
 */
INTBIG us_findnewplace(INTBIG *arr, INTBIG count, INTBIG low, INTBIG high)
{
	REGISTER INTBIG i, gapwid, gappos;

	/* easy if nothing in the array */
	if (count <= 0) return((low + high) / 2);

	/* first sort the array */
	esort(arr, count, SIZEOFINTBIG, sort_intbigascending);

	/* now find the widest gap */
	gapwid = 0;
	gappos = (low + high) / 2;
	for(i=1; i<count; i++)
	{
		if (arr[i] - arr[i-1] > gapwid)
		{
			gapwid = arr[i] - arr[i-1];
			gappos = (arr[i-1] + arr[i]) / 2;
		}
	}
	if (arr[0] - low > gapwid)
	{
		gapwid = arr[0] - low;
		gappos = (low + arr[0]) / 2;
	}
	if (high - arr[count-1] > gapwid)
	{
		gapwid = high - arr[count-1];
		gappos = (arr[count-1] + high) / 2;
	}
	return(gappos);
}

/*
 * Helper routine for "us_show()" that makes exports go in ascending order
 * by name within type
 */
int us_exportnametypeascending(const void *e1, const void *e2)
{
	REGISTER PORTPROTO *pp1, *pp2;
	REGISTER UINTBIG s1, s2;

	pp1 = *((PORTPROTO **)e1);
	pp2 = *((PORTPROTO **)e2);
	s1 = (pp1->userbits & STATEBITS) >> 1;
	s2 = (pp2->userbits & STATEBITS) >> 1;
	if (s1 != s2) return(s1-s2);
	return(namesamenumeric(pp1->protoname, pp2->protoname));
}

/*
 * Helper routine for "us_show()" that makes exports go in ascending order
 * by index within name
 */
int us_exportnameindexascending(const void *e1, const void *e2)
{
	REGISTER PORTPROTO *pp1, *pp2;

	pp1 = *((PORTPROTO **)e1);
	pp2 = *((PORTPROTO **)e2);
	return(namesamenumeric(pp1->protoname, pp2->protoname));
}

/*
 * Helper routine for "us_library()" that makes libraries go in ascending order
 * by their "temp1" field
 */
int us_librarytemp1ascending(const void *e1, const void *e2)
{
	REGISTER LIBRARY *lib1, *lib2;

	lib1 = *((LIBRARY **)e1);
	lib2 = *((LIBRARY **)e2);
	return(lib1->temp1 - lib2->temp1);
}

/*
 * Helper routine for "us_getproto" to sort popup menu items in ascending order.
 */
int us_sortpopupmenuascending(const void *e1, const void *e2)
{
	REGISTER POPUPMENUITEM *pm1, *pm2;

	pm1 = (POPUPMENUITEM *)e1;
	pm2 = (POPUPMENUITEM *)e2;
	return(namesame(pm1->attribute, pm2->attribute));
}

/*
 * Helper routine to add all marked arc prototypes to the infinite string.
 * Marking is done by having the "temp1" field be nonzero.
 */
void us_addpossiblearcconnections(void *infstr)
{
	REGISTER TECHNOLOGY *tech;
	REGISTER INTBIG i;
	REGISTER ARCPROTO *ap;

	i = 0;
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			if (ap->temp1 == 0) i++;
	if (i == 0) addstringtoinfstr(infstr, _(" EVERYTHING")); else
	{
		i = 0;
		for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		{
			if (tech == gen_tech) continue;
			for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			{
				if (ap->temp1 == 0) continue;
				if (i != 0) addtoinfstr(infstr, ',');
				i++;
				formatinfstr(infstr, x_(" %s"), ap->protoname);
			}
		}
	}
}

/*********************************** FINDING ***********************************/

/*
 * routine to find an object/port close to (wantx, wanty) in the current cell.
 * If there is more than one object/port under the cursor, they are returned
 * in reverse sequential order, provided that the most recently found
 * object is described in "curhigh".  The next close object is placed in
 * "curhigh".  If "exclusively" is nonzero, find only nodes or arcs of the
 * current prototype.  If "another" is nonzero, this is the second find,
 * and should not consider text objects.  If "findport" is nonzero, port selection
 * is also desired.  If "under" is nonzero, only find objects exactly under the
 * desired cursor location.  If "special" is nonzero, special selection rules apply.
 */
void us_findobject(INTBIG wantx, INTBIG wanty, WINDOWPART *win, HIGHLIGHT *curhigh,
	INTBIG exclusively, INTBIG another, INTBIG findport, INTBIG under, INTBIG special)
{
	HIGHLIGHT best, lastdirect, prevdirect, bestdirect;
	REGISTER PORTPROTO *pp;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER INTBIG dist;
	REGISTER VARIABLE *var;
	VARIABLE *varnoeval;
	REGISTER INTBIG i, tot, phase, startphase;
	INTBIG looping, bestdist;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* initialize */
	bestdist = MAXINTBIG;
	looping = 0;
	best.fromgeom = NOGEOM;             best.status = 0;
	bestdirect.fromgeom = NOGEOM;       bestdirect.status = 0;
	lastdirect.fromgeom = NOGEOM;       lastdirect.status = 0;
	prevdirect.fromgeom = NOGEOM;       prevdirect.status = 0;

	/* ignore cells if requested */
	startphase = 0;
	if (special == 0 && (us_useroptions&NOINSTANCESELECT) != 0) startphase = 1;

	/* search the relevant objects in the circuit */
	np = win->curnodeproto;
	for(phase = startphase; phase < 3; phase++)
	{
		us_recursivelysearch(np->rtree, exclusively, another, findport,
			under, special, curhigh, &best, &bestdirect, &lastdirect, &prevdirect, &looping,
				&bestdist, wantx, wanty, win, phase);
		us_fartextsearch(np, exclusively, another, findport,
			under, special, curhigh, &best, &bestdirect, &lastdirect, &prevdirect, &looping,
				&bestdist, wantx, wanty, win, phase);
	}

	/* check for displayable variables on the cell */
	tot = tech_displayablecellvars(np, win, &tech_oneprocpolyloop);
	for(i=0; i<tot; i++)
	{
		var = tech_filldisplayablecellvar(np, poly, win, &varnoeval, &tech_oneprocpolyloop);

		/* cell variables are offset from (0,0) */
		us_maketextpoly(poly->string, win, 0, 0, NONODEINST, np->tech,
			var->textdescript, poly);
		poly->style = FILLED;
		dist = polydistance(poly, wantx, wanty);
		if (dist < 0)
		{
			if ((curhigh->status&HIGHTYPE) == HIGHTEXT && curhigh->fromgeom == NOGEOM &&
				(curhigh->fromvar == var || curhigh->fromport == NOPORTPROTO))
			{
				looping = 1;
				prevdirect.status = lastdirect.status;
				prevdirect.fromgeom = lastdirect.fromgeom;
				prevdirect.fromvar = lastdirect.fromvar;
				prevdirect.fromvarnoeval = lastdirect.fromvarnoeval;
				prevdirect.fromport = lastdirect.fromport;
			}
			lastdirect.status = HIGHTEXT;
			lastdirect.fromgeom = NOGEOM;
			lastdirect.fromport = NOPORTPROTO;
			lastdirect.fromvar = var;
			lastdirect.fromvarnoeval = varnoeval;
			if (dist < bestdist)
			{
				bestdirect.status = HIGHTEXT;
				bestdirect.fromgeom = NOGEOM;
				bestdirect.fromvar = var;
				bestdirect.fromvarnoeval = varnoeval;
				bestdirect.fromport = NOPORTPROTO;
			}
		}

		/* see if it is closer than others */
		if (dist < bestdist)
		{
			best.status = HIGHTEXT;
			best.fromgeom = NOGEOM;
			best.fromvar = var;
			best.fromvarnoeval = varnoeval;
			best.fromport = NOPORTPROTO;
			bestdist = dist;
		}
	}

	/* use best direct hit if one exists, otherwise best any-kind-of-hit */
	if (bestdirect.status != 0)
	{
		curhigh->status = bestdirect.status;
		curhigh->fromgeom = bestdirect.fromgeom;
		curhigh->fromvar = bestdirect.fromvar;
		curhigh->fromvarnoeval = bestdirect.fromvarnoeval;
		curhigh->fromport = bestdirect.fromport;
		curhigh->snapx = bestdirect.snapx;
		curhigh->snapy = bestdirect.snapy;
	} else
	{
		if (under == 0)
		{
			curhigh->status = best.status;
			curhigh->fromgeom = best.fromgeom;
			curhigh->fromvar = best.fromvar;
			curhigh->fromvarnoeval = best.fromvarnoeval;
			curhigh->fromport = best.fromport;
			curhigh->snapx = best.snapx;
			curhigh->snapy = best.snapy;
		} else
		{
			curhigh->status = 0;
			curhigh->fromgeom = NOGEOM;
			curhigh->fromvar = NOVARIABLE;
			curhigh->fromvarnoeval = NOVARIABLE;
			curhigh->fromport = NOPORTPROTO;
			curhigh->frompoint = 0;
		}
	}

	/* see if looping through direct hits */
	if (looping != 0)
	{
		/* made direct hit on previously selected object: looping through */
		if (prevdirect.status != 0)
		{
			curhigh->status = prevdirect.status;
			curhigh->fromgeom = prevdirect.fromgeom;
			curhigh->fromvar = prevdirect.fromvar;
			curhigh->fromvarnoeval = prevdirect.fromvarnoeval;
			curhigh->fromport = prevdirect.fromport;
			curhigh->snapx = prevdirect.snapx;
			curhigh->snapy = prevdirect.snapy;
		} else if (lastdirect.status != 0)
		{
			curhigh->status = lastdirect.status;
			curhigh->fromgeom = lastdirect.fromgeom;
			curhigh->fromvar = lastdirect.fromvar;
			curhigh->fromvarnoeval = lastdirect.fromvarnoeval;
			curhigh->fromport = lastdirect.fromport;
			curhigh->snapx = lastdirect.snapx;
			curhigh->snapy = lastdirect.snapy;
		}
	}

	if (curhigh->fromgeom == NOGEOM) curhigh->cell = np; else
		curhigh->cell = geomparent(curhigh->fromgeom);

	/* quit now if nothing found */
	if (curhigh->status == 0) return;

	/* reevaluate if this is code */
	if ((curhigh->status&HIGHTYPE) == HIGHTEXT && curhigh->fromvar != NOVARIABLE &&
		curhigh->fromvarnoeval != NOVARIABLE &&
			curhigh->fromvar != curhigh->fromvarnoeval)
				curhigh->fromvar = evalvar(curhigh->fromvarnoeval, 0, 0);

	/* find the closest port if this is a nodeinst and no port hit directly */
	if ((curhigh->status&HIGHTYPE) == HIGHFROM && curhigh->fromgeom->entryisnode &&
		curhigh->fromport == NOPORTPROTO)
	{
		ni = curhigh->fromgeom->entryaddr.ni;
		for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			shapeportpoly(ni, pp, poly, FALSE);

			/* get distance of desired point to polygon */
			dist = polydistance(poly, wantx, wanty);
			if (dist < 0)
			{
				curhigh->fromport = pp;
				break;
			}
			if (curhigh->fromport == NOPORTPROTO) bestdist = dist;
			if (dist > bestdist) continue;
			bestdist = dist;   curhigh->fromport = pp;
		}
	}
}

/*
 * routine to search cell "np" for "far text" objects that are close to (wantx, wanty)
 * in window "win".  Those that are found are passed to "us_checkoutobject"
 * for proximity evaluation, along with the evaluation parameters "curhigh",
 * "best", "bestdirect", "lastdirect", "prevdirect", "looping", "bestdist",
 * "exclusively", "another", "findport", and "under".  The "phase" value ranges
 * from 0 to 2 according to the type of object desired.
 */
void us_fartextsearch(NODEPROTO *np, INTBIG exclusively, INTBIG another, INTBIG findport,
	INTBIG under, INTBIG findspecial, HIGHLIGHT *curhigh, HIGHLIGHT *best, HIGHLIGHT *bestdirect,
	HIGHLIGHT *lastdirect, HIGHLIGHT *prevdirect, INTBIG *looping, INTBIG *bestdist,
	INTBIG wantx, INTBIG wanty, WINDOWPART *win, INTBIG phase)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;

	switch (phase)
	{
		case 0:			/* only allow complex nodes */
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if ((ni->userbits&NHASFARTEXT) == 0) continue;
				if (ni->proto->primindex != 0) continue;
				us_checkoutobject(ni->geom, 1, exclusively, another, findport, findspecial,
					curhigh, best, bestdirect, lastdirect, prevdirect, looping,
						bestdist, wantx, wanty, win);
			}
			break;
		case 1:			/* only allow arcs */
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			{
				if ((ai->userbits&AHASFARTEXT) == 0) continue;
				us_checkoutobject(ai->geom, 1, exclusively, another, findport, findspecial,
					curhigh, best, bestdirect, lastdirect, prevdirect, looping,
						bestdist, wantx, wanty, win);
			}
			break;
		case 2:			/* only allow primitive nodes */
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if ((ni->userbits&NHASFARTEXT) == 0) continue;
				if (ni->proto->primindex == 0) continue;
				us_checkoutobject(ni->geom, 1, exclusively, another, findport, findspecial,
					curhigh, best, bestdirect, lastdirect, prevdirect, looping,
						bestdist, wantx, wanty, win);
			}
			break;
	}
}

/*
 * routine to search R-tree "rtree" for objects that are close to (wantx, wanty)
 * in window "win".  Those that are found are passed to "us_checkoutobject"
 * for proximity evaluation, along with the evaluation parameters "curhigh",
 * "best", "bestdirect", "lastdirect", "prevdirect", "looping", "bestdist",
 * "exclusively", "another", "findport", and "under".  The "phase" value ranges
 * from 0 to 2 according to the type of object desired.
 */
void us_recursivelysearch(RTNODE *rtree, INTBIG exclusively, INTBIG another, INTBIG findport,
	INTBIG under, INTBIG findspecial, HIGHLIGHT *curhigh, HIGHLIGHT *best, HIGHLIGHT *bestdirect,
	HIGHLIGHT *lastdirect, HIGHLIGHT *prevdirect, INTBIG *looping, INTBIG *bestdist,
	INTBIG wantx, INTBIG wanty, WINDOWPART *win, INTBIG phase)
{
	REGISTER GEOM *geom;
	REGISTER INTBIG i, bestrt;
	BOOLEAN found;
	REGISTER INTBIG disttort, bestdisttort, slop, directhitdist;
	INTBIG lx, hx, ly, hy;

	found = FALSE;
	bestdisttort = MAXINTBIG;
	slop = el_curlib->lambda[el_curtech->techindex] * FARTEXTLIMIT;
	directhitdist = muldiv(EXACTSELECTDISTANCE, win->screenhx - win->screenlx, win->usehx - win->uselx);
	if (directhitdist > slop) slop = directhitdist;
	for(i=0; i<rtree->total; i++)
	{
		db_rtnbbox(rtree, i, &lx, &hx, &ly, &hy);

		/* accumulate best R-tree module in case none are direct hits */
		disttort = abs(wantx - (lx+hx)/2) + abs(wanty - (ly+hy)/2);
		if (disttort < bestdisttort)
		{
			bestdisttort = disttort;
			bestrt = i;
		}

		/* see if this R-tree node is a direct hit */
		if (exclusively == 0 &&
			(lx > wantx+slop || hx < wantx-slop || ly > wanty+slop || hy < wanty-slop)) continue;
		found = TRUE;

		/* search it */
		if (rtree->flag != 0)
		{
			if (stopping(STOPREASONSELECT)) break;
			geom = (GEOM *)rtree->pointers[i];
			switch (phase)
			{
				case 0:			/* only allow complex nodes */
					if (!geom->entryisnode) break;
					if (geom->entryaddr.ni->proto->primindex != 0) break;
					us_checkoutobject(geom, 0, exclusively, another, findport, findspecial,
						curhigh, best, bestdirect, lastdirect, prevdirect, looping,
							bestdist, wantx, wanty, win);
					break;
				case 1:			/* only allow arcs */
					if (geom->entryisnode) break;
					us_checkoutobject(geom, 0, exclusively, another, findport, findspecial,
						curhigh, best, bestdirect, lastdirect, prevdirect, looping,
							bestdist, wantx, wanty, win);
					break;
				case 2:			/* only allow primitive nodes */
					if (!geom->entryisnode) break;
					if (geom->entryaddr.ni->proto->primindex == 0) break;
					us_checkoutobject(geom, 0, exclusively, another, findport, findspecial,
						curhigh, best, bestdirect, lastdirect, prevdirect, looping,
							bestdist, wantx, wanty, win);
					break;
			}
		} else us_recursivelysearch((RTNODE *)rtree->pointers[i], exclusively,
			another, findport, under, findspecial, curhigh, best, bestdirect, lastdirect,
				prevdirect, looping, bestdist, wantx, wanty, win, phase);
	}

	if (found) return;
	if (bestdisttort == MAXINTBIG) return;
	if (under != 0) return;

	/* nothing found, use the closest */
	if (rtree->flag != 0)
	{
		geom = (GEOM *)rtree->pointers[bestrt];
		switch (phase)
		{
			case 0:			/* only allow complex nodes */
				if (!geom->entryisnode) break;
				if (geom->entryaddr.ni->proto->primindex != 0) break;
				us_checkoutobject(geom, 0, exclusively, another, findport, findspecial,
					curhigh, best, bestdirect, lastdirect, prevdirect, looping,
						bestdist, wantx, wanty, win);
				break;
			case 1:			/* only allow arcs */
				if (geom->entryisnode) break;
				us_checkoutobject(geom, 0, exclusively, another, findport, findspecial,
					curhigh, best, bestdirect, lastdirect, prevdirect, looping,
						bestdist, wantx, wanty, win);
				break;
			case 2:			/* only allow primitive nodes */
				if (!geom->entryisnode) break;
				if (geom->entryaddr.ni->proto->primindex == 0) break;
				us_checkoutobject(geom, 0, exclusively, another, findport, findspecial,
					curhigh, best, bestdirect, lastdirect, prevdirect, looping,
						bestdist, wantx, wanty, win);
				break;
		}
	} else us_recursivelysearch((RTNODE *)rtree->pointers[bestrt], exclusively,
		another, findport, under, findspecial, curhigh, best, bestdirect, lastdirect,
			prevdirect, looping, bestdist, wantx, wanty, win, phase);
}

/*
 * search helper routine to include object "geom" in the search for the
 * closest object to the cursor position at (wantx, wanty) in window "win".
 * If "fartext" is nonzero, only look for far-away text on the object.
 * If "exclusively" is nonzero, ignore nodes or arcs that are not of the
 * current type.  If "another" is nonzero, ignore text objects.  If "findport"
 * is nonzero, ports are being selected so cell names should not.  The closest
 * object is "*bestdist" away and is described in "best".  The closest direct
 * hit is in "bestdirect".  If that direct hit is the same as the last hit
 * (kept in "curhigh") then the last direct hit (kept in "lastdirect") is
 * moved to the previous direct hit (kept in "prevdirect") and the "looping"
 * flag is set.  This indicates that the "prevdirect" object should be used
 * (if it exists) and that the "lastdirect" object should be used failing that.
 */
void us_checkoutobject(GEOM *geom, INTBIG fartext, INTBIG exclusively, INTBIG another,
	INTBIG findport, INTBIG findspecial, HIGHLIGHT *curhigh, HIGHLIGHT *best,
	HIGHLIGHT *bestdirect, HIGHLIGHT *lastdirect, HIGHLIGHT *prevdirect, INTBIG *looping,
	INTBIG *bestdist, INTBIG wantx, INTBIG wanty, WINDOWPART *win)
{
	REGISTER PORTPROTO *pp;
	VARIABLE *var, *varnoeval;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	static POLYGON *poly = NOPOLYGON;
	PORTPROTO *port;
	REGISTER INTBIG i, dist, directhitdist;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* compute threshold for direct hits */
	directhitdist = muldiv(EXACTSELECTDISTANCE, win->screenhx - win->screenlx, win->usehx - win->uselx);

	if (geom->entryisnode)
	{
		/* examine a node object */
		ni = geom->entryaddr.ni;

		/* do not "find" hard-to-find nodes if "findspecial" is not set */
		if (findspecial == 0 && (ni->userbits&HARDSELECTN) != 0) return;

		/* do not include primitives that have all layers invisible */
		if (ni->proto->primindex != 0 && (ni->proto->userbits&NINVISIBLE) != 0)
			return;

		/* skip if being exclusive */
		if (exclusively != 0 && ni->proto != us_curnodeproto) return;

		/* try text on the node (if not searching for "another") */
		if (another == 0 && exclusively == 0)
		{
			us_initnodetext(ni, findspecial, win);
			for(;;)
			{
				if (us_getnodetext(ni, win, poly, &var, &varnoeval, &port)) break;

				/* get distance of desired point to polygon */
				dist = polydistance(poly, wantx, wanty);

				/* direct hit */
				if (dist < directhitdist)
				{
					if (curhigh->fromgeom == geom && (curhigh->status&HIGHTYPE) == HIGHTEXT &&
						curhigh->fromvar == var && curhigh->fromport == port)
					{
						*looping = 1;
						prevdirect->status = lastdirect->status;
						prevdirect->fromgeom = lastdirect->fromgeom;
						prevdirect->fromvar = lastdirect->fromvar;
						prevdirect->fromvarnoeval = lastdirect->fromvarnoeval;
						prevdirect->fromport = lastdirect->fromport;
					}
					lastdirect->status = HIGHTEXT;
					lastdirect->fromgeom = geom;
					lastdirect->fromport = port;
					lastdirect->fromvar = var;
					lastdirect->fromvarnoeval = varnoeval;
					if (dist < *bestdist)
					{
						bestdirect->status = HIGHTEXT;
						bestdirect->fromgeom = geom;
						bestdirect->fromvar = var;
						bestdirect->fromvarnoeval = varnoeval;
						bestdirect->fromport = port;
					}
				}

				/* see if it is closer than others */
				if (dist < *bestdist)
				{
					best->status = HIGHTEXT;
					best->fromgeom = geom;
					best->fromvar = var;
					best->fromvarnoeval = varnoeval;
					best->fromport = port;
					*bestdist = dist;
				}
			}
		}

		if (fartext != 0) return;

		/* do not "find" Invisible-Pins if they have text or exports */
		if (ni->proto == gen_invispinprim)
		{
			if (ni->firstportexpinst != NOPORTEXPINST) return;
			for(i=0; i<ni->numvar; i++)
			{
				var = &ni->firstvar[i];
				if ((var->type&VDISPLAY) != 0) return;
			}
		}

		/* get the distance to the object */
		dist = us_disttoobject(wantx, wanty, geom);

		/* direct hit */
		if (dist < directhitdist)
		{
			if (curhigh->fromgeom == geom && (curhigh->status&HIGHTYPE) != HIGHTEXT)
			{
				*looping = 1;
				prevdirect->status = lastdirect->status;
				prevdirect->fromgeom = lastdirect->fromgeom;
				prevdirect->fromvar = lastdirect->fromvar;
				prevdirect->fromvarnoeval = lastdirect->fromvarnoeval;
				prevdirect->fromport = lastdirect->fromport;
				prevdirect->snapx = lastdirect->snapx;
				prevdirect->snapy = lastdirect->snapy;

				/* see if there is another port under the cursor */
				if (curhigh->fromport != NOPORTPROTO)
				{
					for(pp = curhigh->fromport->nextportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
					{
						shapeportpoly(ni, pp, poly, FALSE);
						if (isinside(wantx, wanty, poly))
						{
							prevdirect->status = HIGHFROM;
							prevdirect->fromgeom = geom;
							prevdirect->fromport = pp;
							break;
						}
					}
				}
			}
			lastdirect->status = HIGHFROM;
			lastdirect->fromgeom = geom;
			lastdirect->fromport = NOPORTPROTO;
			us_selectsnap(lastdirect, wantx, wanty);
			if (dist < *bestdist)
			{
				bestdirect->status = HIGHFROM;
				bestdirect->fromgeom = geom;
				bestdirect->fromport = NOPORTPROTO;
				us_selectsnap(bestdirect, wantx, wanty);
			}
		}

		/* see if it is closer than others */
		if (dist < *bestdist)
		{
			best->status = HIGHFROM;
			best->fromgeom = geom;
			best->fromport = NOPORTPROTO;
			us_selectsnap(best, wantx, wanty);
			*bestdist = dist;
		}
	} else
	{
		/* examine an arc object */
		ai = geom->entryaddr.ai;

		/* do not "find" hard-to-find arcs if "findspecial" is not set */
		if (findspecial == 0 && (ai->userbits&HARDSELECTA) != 0) return;

		/* do not include arcs that have all layers invisible */
		if ((ai->proto->userbits&AINVISIBLE) != 0) return;

		/* skip if being exclusive */
		if (exclusively != 0 && ai->proto != us_curarcproto) return;

		/* try text on the arc (if not searching for "another") */
		if (exclusively == 0)
		{
			us_initarctext(ai, findspecial, win);
			for(;;)
			{
				if (us_getarctext(ai, win, poly, &var, &varnoeval)) break;

				/* get distance of desired point to polygon */
				dist = polydistance(poly, wantx, wanty);

				/* direct hit */
				if (dist < directhitdist)
				{
					if (curhigh->fromgeom == geom && (curhigh->status&HIGHTYPE) == HIGHTEXT &&
						curhigh->fromvar == var)
					{
						*looping = 1;
						prevdirect->status = lastdirect->status;
						prevdirect->fromgeom = lastdirect->fromgeom;
						prevdirect->fromvar = lastdirect->fromvar;
						prevdirect->fromvarnoeval = lastdirect->fromvarnoeval;
						prevdirect->fromport = lastdirect->fromport;
					}
					lastdirect->status = HIGHTEXT;
					lastdirect->fromgeom = geom;
					lastdirect->fromvar = var;
					lastdirect->fromvarnoeval = varnoeval;
					lastdirect->fromport = NOPORTPROTO;
					if (dist < *bestdist)
					{
						bestdirect->status = HIGHTEXT;
						bestdirect->fromgeom = geom;
						bestdirect->fromvar = var;
						bestdirect->fromvarnoeval = varnoeval;
						us_selectsnap(bestdirect, wantx, wanty);
						bestdirect->fromport = NOPORTPROTO;
					}
				}

				/* see if it is closer than others */
				if (dist < *bestdist)
				{
					best->status = HIGHTEXT;
					best->fromgeom = geom;
					best->fromvar = var;
					best->fromvarnoeval = varnoeval;
					best->fromport = NOPORTPROTO;
					us_selectsnap(best, wantx, wanty);
					*bestdist = dist;
				}
			}
		}

		if (fartext != 0) return;

		/* get distance to arc */
		dist = us_disttoobject(wantx, wanty, geom);

		/* direct hit */
		if (dist < directhitdist)
		{
			if (curhigh->fromgeom == geom && (curhigh->status&HIGHTYPE) != HIGHTEXT)
			{
				*looping = 1;
				prevdirect->status = lastdirect->status;
				prevdirect->fromgeom = lastdirect->fromgeom;
				prevdirect->fromvar = lastdirect->fromvar;
				prevdirect->fromvarnoeval = lastdirect->fromvarnoeval;
				prevdirect->fromport = lastdirect->fromport;
				prevdirect->snapx = lastdirect->snapx;
				prevdirect->snapy = lastdirect->snapy;
			}
			lastdirect->status = HIGHFROM;
			lastdirect->fromgeom = geom;
			lastdirect->fromport = NOPORTPROTO;
			us_selectsnap(lastdirect, wantx, wanty);
			if (dist < *bestdist)
			{
				bestdirect->status = HIGHFROM;
				bestdirect->fromgeom = geom;
				bestdirect->fromvar = NOVARIABLE;
				bestdirect->fromvarnoeval = NOVARIABLE;
				bestdirect->fromport = NOPORTPROTO;
				us_selectsnap(bestdirect, wantx, wanty);
			}
		}

		/* see if it is closer than others */
		if (dist < *bestdist)
		{
			best->status = HIGHFROM;
			best->fromgeom = geom;
			best->fromvar = NOVARIABLE;
			best->fromvarnoeval = NOVARIABLE;
			best->fromport = NOPORTPROTO;
			us_selectsnap(best, wantx, wanty);
			*bestdist = dist;
		}
	}
}

/*
 * routine to determine whether the cursor (xcur, ycur) is over the object in "high".
 */
BOOLEAN us_cursoroverhigh(HIGHLIGHT *high, INTBIG xcur, INTBIG ycur, WINDOWPART *win)
{
	VARIABLE *var, *varnoeval;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER NODEPROTO *np;
	static POLYGON *poly = NOPOLYGON;
	PORTPROTO *port;
	REGISTER INTBIG i, tot;
	REGISTER INTBIG directhitdist;

	/* must be in the same cell */
	if (high->cell != win->curnodeproto) return(FALSE);

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* compute threshold for direct hits */
	directhitdist = muldiv(EXACTSELECTDISTANCE, win->screenhx - win->screenlx,
		win->usehx - win->uselx);

	/* could be selected text */
	if ((high->status&HIGHTEXT) != 0)
	{
		if (high->fromgeom == NOGEOM)
		{
			np = high->cell;
			tot = tech_displayablecellvars(np, win, &tech_oneprocpolyloop);
			for(i=0; i<tot; i++)
			{
				var = tech_filldisplayablecellvar(np, poly, win, &varnoeval, &tech_oneprocpolyloop);
				if (high->fromvarnoeval != varnoeval) continue;

				/* cell variables are offset from (0,0) */
				us_maketextpoly(poly->string, win, 0, 0, NONODEINST, np->tech,
					var->textdescript, poly);
				poly->style = FILLED;
				if (polydistance(poly, xcur, ycur) <= directhitdist) return(TRUE);
			}
			return(FALSE);
		}

		/* examine all text on the object */
		if (high->fromgeom->entryisnode)
		{
			ni = high->fromgeom->entryaddr.ni;
			us_initnodetext(ni, 1, win);
		} else
		{
			ai = high->fromgeom->entryaddr.ai;
			us_initarctext(ai, 1, win);
		}

		for(;;)
		{
			if (high->fromgeom->entryisnode)
			{
				if (us_getnodetext(ni, win, poly, &var, &varnoeval, &port)) break;
			} else
			{
				if (us_getarctext(ai, win, poly, &var, &varnoeval)) break;
				port = NOPORTPROTO;
			}
			if (high->fromvar != var || high->fromport != port) continue;

			/* accept if on */
			if (polydistance(poly, xcur, ycur) <= directhitdist) return(TRUE);
		}
		return(FALSE);
	}

	/* must be a single node or arc selected */
	if ((high->status&HIGHFROM) == 0) return(FALSE);

	/* see if the point is over the object */
	if (us_disttoobject(xcur, ycur, high->fromgeom) <= directhitdist) return(TRUE);
	return(FALSE);
}

/*
 * routine to add snapping selection to the highlight in "best".
 */
void us_selectsnap(HIGHLIGHT *best, INTBIG wantx, INTBIG wanty)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER INTBIG j, k;
	INTBIG cx, cy;
	static POLYGON *poly = NOPOLYGON;
	XARRAY trans;

	if ((us_state&SNAPMODE) == SNAPMODENONE) return;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	if (best->fromgeom->entryisnode)
	{
		ni = best->fromgeom->entryaddr.ni;
		if (ni->proto->primindex == 0)
		{
			if ((us_state&SNAPMODE) == SNAPMODECENTER)
			{
				corneroffset(ni, ni->proto, ni->rotation, ni->transpose, &cx, &cy, FALSE);
				us_setbestsnappoint(best, wantx, wanty, ni->lowx+cx, ni->lowy+cy, FALSE, FALSE);
			}
			return;
		}
		makerot(ni, trans);
		k = nodepolys(ni, 0, NOWINDOWPART);
		for(j=0; j<k; j++)
		{
			shapenodepoly(ni, j, poly);
			xformpoly(poly, trans);
			us_selectsnappoly(best, poly, wantx, wanty);
		}
	} else
	{
		ai = best->fromgeom->entryaddr.ai;
		k = arcpolys(ai, NOWINDOWPART);
		for(j=0; j<k; j++)
		{
			shapearcpoly(ai, j, poly);
			us_selectsnappoly(best, poly, wantx, wanty);
		}
	}
}

void us_selectsnappoly(HIGHLIGHT *best, POLYGON *poly, INTBIG wantx, INTBIG wanty)
{
	REGISTER INTBIG j, k, radius, sea;
	REGISTER BOOLEAN tan, perp;
	INTBIG testx, testy;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER GEOM *geom;
	XARRAY trans;
	REGISTER INTBIG angle, otherang, i, last;
	static POLYGON *interpoly = NOPOLYGON;

	switch (us_state&SNAPMODE)
	{
		case SNAPMODECENTER:
			if (poly->style == CIRCLE || poly->style == THICKCIRCLE ||
				poly->style == DISC || poly->style == CIRCLEARC ||
				poly->style == THICKCIRCLEARC)
			{
				us_setbestsnappoint(best, wantx, wanty, poly->xv[0], poly->yv[0], FALSE, FALSE);
			} else if (poly->style != OPENED && poly->style != OPENEDT1 && poly->style != OPENEDT2 &&
				poly->style != OPENEDT3 && poly->style != CLOSED)
			{
				getcenter(poly, &testx, &testy);
				us_setbestsnappoint(best, wantx, wanty, testx, testy, FALSE, FALSE);
			}
			break;
		case SNAPMODEMIDPOINT:
			if (poly->style == OPENED || poly->style == OPENEDT1 || poly->style == OPENEDT2 ||
				poly->style == OPENEDT3 || poly->style == CLOSED)
			{
				for(i=0; i<poly->count; i++)
				{
					if (i == 0)
					{
						if (poly->style != CLOSED) continue;
						last = poly->count - 1;
					} else last = i-1;
					testx = (poly->xv[last] + poly->xv[i]) / 2;
					testy = (poly->yv[last] + poly->yv[i]) / 2;
					us_setbestsnappoint(best, wantx, wanty, testx, testy, FALSE, FALSE);
				}
			} else if (poly->style == VECTORS)
			{
				for(i=0; i<poly->count; i += 2)
				{
					testx = (poly->xv[i+1] + poly->xv[i]) / 2;
					testy = (poly->yv[i+1] + poly->yv[i]) / 2;
					us_setbestsnappoint(best, wantx, wanty, testx, testy, FALSE, FALSE);
				}
			}
			break;
		case SNAPMODEENDPOINT:
			if (poly->style == OPENED || poly->style == OPENEDT1 || poly->style == OPENEDT2 ||
				poly->style == OPENEDT3 || poly->style == CLOSED)
			{
				for(i=0; i<poly->count; i++)
					us_setbestsnappoint(best, wantx, wanty, poly->xv[i], poly->yv[i], FALSE, FALSE);
			} else if (poly->style == VECTORS)
			{
				for(i=0; i<poly->count; i += 2)
				{
					us_setbestsnappoint(best, wantx, wanty, poly->xv[i], poly->yv[i], FALSE, FALSE);
					us_setbestsnappoint(best, wantx, wanty, poly->xv[i+1], poly->yv[i+1], FALSE, FALSE);
				}
			} else if (poly->style == CIRCLEARC || poly->style == THICKCIRCLEARC)
			{
				us_setbestsnappoint(best, wantx, wanty, poly->xv[1], poly->yv[1], FALSE, FALSE);
				us_setbestsnappoint(best, wantx, wanty, poly->xv[2], poly->yv[2], FALSE, FALSE);
			}
			break;
		case SNAPMODETANGENT:
		case SNAPMODEPERP:
			if (poly->style == OPENED || poly->style == OPENEDT1 || poly->style == OPENEDT2 ||
				poly->style == OPENEDT3 || poly->style == CLOSED)
			{
				for(i=0; i<poly->count; i++)
				{
					if (i == 0)
					{
						if (poly->style != CLOSED) continue;
						last = poly->count - 1;
					} else last = i-1;
					angle = figureangle(poly->xv[last],poly->yv[last], poly->xv[i],poly->yv[i]);
					otherang = (angle+900) % 3600;
					if (intersect(poly->xv[last],poly->yv[last], angle, wantx, wanty, otherang,
						&testx, &testy) >= 0)
					{
						if (testx >= mini(poly->xv[last], poly->xv[i]) &&
							testx <= maxi(poly->xv[last], poly->xv[i]) &&
							testy >= mini(poly->yv[last], poly->yv[i]) &&
							testy <= maxi(poly->yv[last], poly->yv[i]))
						{
							us_setbestsnappoint(best, wantx, wanty, testx, testy, FALSE, TRUE);
						}
					}
				}
			} else if (poly->style == VECTORS)
			{
				for(i=0; i<poly->count; i += 2)
				{
					angle = figureangle(poly->xv[i],poly->yv[i], poly->xv[i+1],poly->yv[i+1]);
					otherang = (angle+900) % 3600;
					if (intersect(poly->xv[i],poly->yv[i], angle, wantx, wanty, otherang,
						&testx, &testy) >= 0)
					{
						if (testx >= mini(poly->xv[i], poly->xv[i+1]) &&
							testx <= maxi(poly->xv[i], poly->xv[i+1]) &&
							testy >= mini(poly->yv[i], poly->yv[i+1]) &&
							testy <= maxi(poly->yv[i], poly->yv[i+1]))
						{
							us_setbestsnappoint(best, wantx, wanty, testx, testy, FALSE, TRUE);
						}
					}
				}
			} else if (poly->style == CIRCLE || poly->style == THICKCIRCLE ||
				poly->style == DISC || poly->style == CIRCLEARC ||
				poly->style == THICKCIRCLEARC)
			{
				if (poly->xv[0] == wantx && poly->yv[0] == wanty) break;
				angle = figureangle(poly->xv[0],poly->yv[0], wantx,wanty);
				radius = computedistance(poly->xv[0],poly->yv[0], poly->xv[1],poly->yv[1]);
				testx = poly->xv[0] + mult(radius, cosine(angle));
				testy = poly->yv[0] + mult(radius, sine(angle));
				if ((poly->style == CIRCLEARC || poly->style == THICKCIRCLEARC) &&
					!us_pointonarc(testx, testy, poly)) break;
				if ((us_state&SNAPMODE) == SNAPMODETANGENT) tan = TRUE; else tan = FALSE;
				if ((us_state&SNAPMODE) == SNAPMODEPERP) perp = TRUE; else perp = FALSE;
				us_setbestsnappoint(best, wantx, wanty, testx, testy, tan, perp);
			}
			break;
		case SNAPMODEQUAD:
			if (poly->style == CIRCLE || poly->style == THICKCIRCLE || poly->style == DISC)
			{
				radius = computedistance(poly->xv[0],poly->yv[0], poly->xv[1],poly->yv[1]);
				us_setbestsnappoint(best, wantx, wanty, poly->xv[0]+radius, poly->yv[0], FALSE, FALSE);
				us_setbestsnappoint(best, wantx, wanty, poly->xv[0]-radius, poly->yv[0], FALSE, FALSE);
				us_setbestsnappoint(best, wantx, wanty, poly->xv[0], poly->yv[0]+radius, FALSE, FALSE);
				us_setbestsnappoint(best, wantx, wanty, poly->xv[0], poly->yv[0]-radius, FALSE, FALSE);
			} else if (poly->style == CIRCLEARC || poly->style == THICKCIRCLEARC)
			{
				radius = computedistance(poly->xv[0],poly->yv[0], poly->xv[1],poly->yv[1]);
				if (us_pointonarc(poly->xv[0]+radius, poly->yv[0], poly))
					us_setbestsnappoint(best, wantx, wanty, poly->xv[0]+radius, poly->yv[0], FALSE, FALSE);
				if (us_pointonarc(poly->xv[0]-radius, poly->yv[0], poly))
					us_setbestsnappoint(best, wantx, wanty, poly->xv[0]-radius, poly->yv[0], FALSE, FALSE);
				if (us_pointonarc(poly->xv[0], poly->yv[0]+radius, poly))
					us_setbestsnappoint(best, wantx, wanty, poly->xv[0], poly->yv[0]+radius, FALSE, FALSE);
				if (us_pointonarc(poly->xv[0], poly->yv[0]-radius, poly))
					us_setbestsnappoint(best, wantx, wanty, poly->xv[0], poly->yv[0]-radius, FALSE, FALSE);
			}
			break;
		case SNAPMODEINTER:
			/* get intersection polygon */
			(void)needstaticpolygon(&interpoly, 4, us_tool->cluster);

			/* search in area around this object */
			sea = initsearch(best->fromgeom->lowx, best->fromgeom->highx, best->fromgeom->lowy,
				best->fromgeom->highy, geomparent(best->fromgeom));
			for(;;)
			{
				geom = nextobject(sea);
				if (geom == NOGEOM) break;
				if (geom == best->fromgeom) continue;
				if (geom->entryisnode)
				{
					ni = geom->entryaddr.ni;
					if (ni->proto->primindex == 0) continue;
					makerot(ni, trans);
					k = nodepolys(ni, 0, NOWINDOWPART);
					for(j=0; j<k; j++)
					{
						shapenodepoly(ni, j, interpoly);
						xformpoly(interpoly, trans);
						us_intersectsnappoly(best, poly, interpoly, wantx, wanty);
					}
				} else
				{
					ai = geom->entryaddr.ai;
					k = arcpolys(ai, NOWINDOWPART);
					for(j=0; j<k; j++)
					{
						shapearcpoly(ai, j, interpoly);
						us_intersectsnappoly(best, poly, interpoly, wantx, wanty);
					}
				}
			}
			break;
	}
}

/*
 * routine to find the intersection between polygons "poly" and "interpoly" and set this as the snap
 * point in highlight "best" (the cursor is at (wantx,wanty).
 */
void us_intersectsnappoly(HIGHLIGHT *best, POLYGON *poly, POLYGON *interpoly, INTBIG wantx, INTBIG wanty)
{
	REGISTER POLYGON *swappoly;
	REGISTER INTBIG i, last;

	if (interpoly->style == OPENED || interpoly->style == OPENEDT1 || interpoly->style == OPENEDT2 ||
		interpoly->style == OPENEDT3 || interpoly->style == CLOSED || interpoly->style == VECTORS)
	{
		swappoly = poly;   poly = interpoly;   interpoly = swappoly;
	}

	if (poly->style == OPENED || poly->style == OPENEDT1 || poly->style == OPENEDT2 ||
		poly->style == OPENEDT3 || poly->style == CLOSED)
	{
		for(i=0; i<poly->count; i++)
		{
			if (i == 0)
			{
				if (poly->style != CLOSED) continue;
				last = poly->count - 1;
			} else last = i-1;
			us_intersectsnapline(best, poly->xv[last],poly->yv[last], poly->xv[i],poly->yv[i],
				interpoly, wantx, wanty);
		}
		return;
	}
	if (poly->style == VECTORS)
	{
		for(i=0; i<poly->count; i += 2)
		{
			us_intersectsnapline(best, poly->xv[i],poly->yv[i], poly->xv[i+1],poly->yv[i+1],
				interpoly, wantx, wanty);
		}
		return;
	}
}

/*
 * routine to find the intersection between the line from (x1,y1) to (x2,y2) and polygon "interpoly".
 * This is set this as the snap point in highlight "best" (the cursor is at (wantx,wanty).
 */
void us_intersectsnapline(HIGHLIGHT *best, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2,
	POLYGON *interpoly, INTBIG wantx, INTBIG wanty)
{
	REGISTER INTBIG i, last, angle, interangle;
	INTBIG ix, iy, ix1, iy1, ix2, iy2;

	angle = figureangle(x1,y1, x2,y2);
	if (interpoly->style == OPENED || interpoly->style == OPENEDT1 || interpoly->style == OPENEDT2 ||
		interpoly->style == OPENEDT3 || interpoly->style == CLOSED)
	{
		for(i=0; i<interpoly->count; i++)
		{
			if (i == 0)
			{
				if (interpoly->style != CLOSED) continue;
				last = interpoly->count - 1;
			} else last = i-1;
			interangle = figureangle(interpoly->xv[last],interpoly->yv[last], interpoly->xv[i],interpoly->yv[i]);
			if (intersect(x1,y1, angle, interpoly->xv[last],interpoly->yv[last], interangle, &ix, &iy) < 0)
				continue;
			if (ix < mini(x1,x2)) continue;
			if (ix > maxi(x1,x2)) continue;
			if (iy < mini(y1,y2)) continue;
			if (iy > maxi(y1,y2)) continue;
			if (ix < mini(interpoly->xv[last],interpoly->xv[i])) continue;
			if (ix > maxi(interpoly->xv[last],interpoly->xv[i])) continue;
			if (iy < mini(interpoly->yv[last],interpoly->yv[i])) continue;
			if (iy > maxi(interpoly->yv[last],interpoly->yv[i])) continue;
			us_setbestsnappoint(best, wantx, wanty, ix, iy, FALSE, FALSE);
		}
		return;
	}
	if (interpoly->style == VECTORS)
	{
		for(i=0; i<interpoly->count; i += 2)
		{
			interangle = figureangle(interpoly->xv[i],interpoly->yv[i], interpoly->xv[i+1],interpoly->yv[i+1]);
			if (intersect(x1,y1, angle, interpoly->xv[i],interpoly->yv[i], interangle, &ix, &iy) < 0)
				continue;
			if (ix < mini(x1,x2)) continue;
			if (ix > maxi(x1,x2)) continue;
			if (iy < mini(y1,y2)) continue;
			if (iy > maxi(y1,y2)) continue;
			if (ix < mini(interpoly->xv[i],interpoly->xv[i+1])) continue;
			if (ix > maxi(interpoly->xv[i],interpoly->xv[i+1])) continue;
			if (iy < mini(interpoly->yv[i],interpoly->yv[i+1])) continue;
			if (iy > maxi(interpoly->yv[i],interpoly->yv[i+1])) continue;
			us_setbestsnappoint(best, wantx, wanty, ix, iy, FALSE, FALSE);
		}
		return;
	}
	if (interpoly->style == CIRCLEARC || interpoly->style == THICKCIRCLEARC)
	{
		i = circlelineintersection(interpoly->xv[0], interpoly->yv[0], interpoly->xv[1], interpoly->yv[1],
			x1, y1, x2, y2, &ix1, &iy1, &ix2, &iy2, 0);
		if (i >= 1)
		{
			if (us_pointonarc(ix1, iy1, interpoly))
				us_setbestsnappoint(best, wantx, wanty, ix1, iy1, FALSE, FALSE);
			if (i >= 2)
			{
				if (us_pointonarc(ix2, iy2, interpoly))
					us_setbestsnappoint(best, wantx, wanty, ix2, iy2, FALSE, FALSE);
			}
		}
		return;
	}
	if (interpoly->style == CIRCLE || interpoly->style == THICKCIRCLE)
	{
		i = circlelineintersection(interpoly->xv[0], interpoly->yv[0], interpoly->xv[1], interpoly->yv[1],
			x1, y1, x2, y2, &ix1, &iy1, &ix2, &iy2, 0);
		if (i >= 1)
		{
			us_setbestsnappoint(best, wantx, wanty, ix1, iy1, FALSE, FALSE);
			if (i >= 2)
			{
				us_setbestsnappoint(best, wantx, wanty, ix2, iy2, FALSE, FALSE);
			}
		}
		return;
	}
}

/*
 * Routine to adjust the two highlight modules "firsthigh" and "secondhigh" to account for the
 * fact that one or both has a tangent snap point that must be tangent to the other's snap point.
 */
void us_adjusttangentsnappoints(HIGHLIGHT *firsthigh, HIGHLIGHT *secondhigh)
{
	INTBIG fx, fy, sx, sy, pfx[4], pfy[4], psx[4], psy[4], ix1, iy1, ix2, iy2;
	REGISTER INTBIG frad, srad, rad, dist, bestdist;
	REGISTER INTBIG j, k, dps, bestone;
	double ang, oang, dx, dy;
	static POLYGON *firstpoly = NOPOLYGON, *secondpoly = NOPOLYGON;
	POLYGON *swappoly;
	HIGHLIGHT *swaphighlight;
	REGISTER NODEINST *ni;
	XARRAY trans;

	/* get polygon describing first object */
	if ((firsthigh->status&HIGHSNAPTAN) != 0)
	{
		(void)needstaticpolygon(&firstpoly, 4, us_tool->cluster);
		if (!firsthigh->fromgeom->entryisnode) return;
		ni = firsthigh->fromgeom->entryaddr.ni;
		if (ni->proto->primindex == 0) return;
		makerot(ni, trans);
		k = nodepolys(ni, 0, NOWINDOWPART);
		for(j=0; j<k; j++)
		{
			shapenodepoly(ni, j, firstpoly);
			if (firstpoly->style == CIRCLEARC || firstpoly->style == THICKCIRCLEARC ||
				firstpoly->style == CIRCLE || firstpoly->style == THICKCIRCLE ||
					firstpoly->style == DISC) break;
		}
		if (j >= k) return;
		xformpoly(firstpoly, trans);
	}

	/* get polygon describing second object */
	if ((secondhigh->status&HIGHSNAPTAN) != 0)
	{
		(void)needstaticpolygon(&secondpoly, 4, us_tool->cluster);
		if (!secondhigh->fromgeom->entryisnode) return;
		ni = secondhigh->fromgeom->entryaddr.ni;
		if (ni->proto->primindex == 0) return;
		makerot(ni, trans);
		k = nodepolys(ni, 0, NOWINDOWPART);
		for(j=0; j<k; j++)
		{
			shapenodepoly(ni, j, secondpoly);
			if (secondpoly->style == CIRCLEARC || secondpoly->style == THICKCIRCLEARC ||
				secondpoly->style == CIRCLE || secondpoly->style == THICKCIRCLE ||
					secondpoly->style == DISC) break;
		}
		if (j >= k) return;
		xformpoly(secondpoly, trans);
	}

	if ((firsthigh->status&HIGHSNAPTAN) != 0)
	{
		if ((secondhigh->status&HIGHSNAPTAN) != 0)
		{
			/* tangent on both curves: find radii and make sure first is larger */
			frad = computedistance(firstpoly->xv[0], firstpoly->yv[0],
				firstpoly->xv[1], firstpoly->yv[1]);
			srad = computedistance(secondpoly->xv[0], secondpoly->yv[0],
				secondpoly->xv[1], secondpoly->yv[1]);
			if (frad < srad)
			{
				swappoly = firstpoly;       firstpoly = secondpoly;   secondpoly = swappoly;
				swaphighlight = firsthigh;  firsthigh = secondhigh;   secondhigh = swaphighlight;
				rad = frad;                 frad = srad;              srad = rad;
			}

			/* find tangent lines along outside of two circles */
			dps = 0;
			if (frad == srad)
			{
				/* special case when radii are equal: construct simple outside tangent lines */
				dx = (double)(secondpoly->xv[0]-firstpoly->xv[0]);
				dy = (double)(secondpoly->yv[0]-firstpoly->yv[0]);
				if (dx == 0.0 && dy == 0.0)
				{
					us_abortcommand(_("Domain error during tangent computation"));
					return;
				}
				ang = atan2(dy, dx);
				oang = ang + EPI / 2.0;
				if (oang > EPI * 2.0) oang -= EPI * 2.0;
				pfx[dps] = firstpoly->xv[0] + rounddouble(cos(oang) * (double)frad);
				pfy[dps] = firstpoly->yv[0] + rounddouble(sin(oang) * (double)frad);
				psx[dps] = secondpoly->xv[0] + rounddouble(cos(oang) * (double)srad);
				psy[dps] = secondpoly->yv[0] + rounddouble(sin(oang) * (double)srad);
				dps++;

				oang = ang - EPI / 2.0;
				if (oang < -EPI * 2.0) oang += EPI * 2.0;
				pfx[dps] = firstpoly->xv[0] + rounddouble(cos(oang) * (double)frad);
				pfy[dps] = firstpoly->yv[0] + rounddouble(sin(oang) * (double)frad);
				psx[dps] = secondpoly->xv[0] + rounddouble(cos(oang) * (double)srad);
				psy[dps] = secondpoly->yv[0] + rounddouble(sin(oang) * (double)srad);
				dps++;
			} else
			{
				if (!circletangents(secondpoly->xv[0], secondpoly->yv[0],
					firstpoly->xv[0], firstpoly->yv[0], firstpoly->xv[0]+frad-srad, firstpoly->yv[0],
						&ix1, &iy1, &ix2, &iy2))
				{
					dx = (double)(ix1-firstpoly->xv[0]);   dy = (double)(iy1-firstpoly->yv[0]);
					if (dx == 0.0 && dy == 0.0)
					{
						us_abortcommand(_("Domain error during tangent computation"));
						return;
					}
					ang = atan2(dy, dx);
					pfx[dps] = firstpoly->xv[0] + rounddouble(cos(ang) * (double)frad);
					pfy[dps] = firstpoly->yv[0] + rounddouble(sin(ang) * (double)frad);
					psx[dps] = secondpoly->xv[0] + rounddouble(cos(ang) * (double)srad);
					psy[dps] = secondpoly->yv[0] + rounddouble(sin(ang) * (double)srad);
					dps++;

					dx = (double)(ix2-firstpoly->xv[0]);   dy = (double)(iy2-firstpoly->yv[0]);
					if (dx == 0.0 && dy == 0.0)
					{
						us_abortcommand(_("Domain error during tangent computation"));
						return;
					}
					ang = atan2(dy, dx);
					pfx[dps] = firstpoly->xv[0] + rounddouble(cos(ang) * (double)frad);
					pfy[dps] = firstpoly->yv[0] + rounddouble(sin(ang) * (double)frad);
					psx[dps] = secondpoly->xv[0] + rounddouble(cos(ang) * (double)srad);
					psy[dps] = secondpoly->yv[0] + rounddouble(sin(ang) * (double)srad);
					dps++;
				}
			}

			/* find tangent lines that cross between two circles */
			if (!circletangents(secondpoly->xv[0], secondpoly->yv[0],
				firstpoly->xv[0], firstpoly->yv[0], firstpoly->xv[0]+frad+srad, firstpoly->yv[0],
					&ix1, &iy1, &ix2, &iy2))
			{
				dx = (double)(ix1-firstpoly->xv[0]);   dy = (double)(iy1-firstpoly->yv[0]);
				if (dx == 0.0 && dy == 0.0)
				{
					us_abortcommand(_("Domain error during tangent computation"));
					return;
				}
				ang = atan2(dy, dx);
				pfx[dps] = firstpoly->xv[0] + rounddouble(cos(ang) * (double)frad);
				pfy[dps] = firstpoly->yv[0] + rounddouble(sin(ang) * (double)frad);
				psx[dps] = secondpoly->xv[0] - rounddouble(cos(ang) * (double)srad);
				psy[dps] = secondpoly->yv[0] - rounddouble(sin(ang) * (double)srad);
				dps++;

				dx = (double)(ix2-firstpoly->xv[0]);   dy = (double)(iy2-firstpoly->yv[0]);
				if (dx == 0.0 && dy == 0.0)
				{
					us_abortcommand(_("Domain error during tangent computation"));
					return;
				}
				ang = atan2(dy, dx);
				pfx[dps] = firstpoly->xv[0] + rounddouble(cos(ang) * (double)frad);
				pfy[dps] = firstpoly->yv[0] + rounddouble(sin(ang) * (double)frad);
				psx[dps] = secondpoly->xv[0] - rounddouble(cos(ang) * (double)srad);
				psy[dps] = secondpoly->yv[0] - rounddouble(sin(ang) * (double)srad);
				dps++;
			}

			/* screen out points that are not on arcs */
			k = 0;
			for(j=0; j<dps; j++)
			{
				if ((firstpoly->style == CIRCLEARC || firstpoly->style == THICKCIRCLEARC) &&
					!us_pointonarc(pfx[j], pfy[j], firstpoly)) continue;
				if ((secondpoly->style == CIRCLEARC || secondpoly->style == THICKCIRCLEARC) &&
					!us_pointonarc(psx[j], psy[j], secondpoly)) continue;
				pfx[k] = pfx[j];   pfy[k] = pfy[j];
				psx[k] = psx[j];   psy[k] = psy[j];
				k++;
			}
			dps = k;
			if (dps == 0) return;

			/* now find the tangent line that is closest to the snap points */
			us_getsnappoint(firsthigh, &fx, &fy);
			us_getsnappoint(secondhigh, &sx, &sy);
			for(j=0; j<dps; j++)
			{
				dist = computedistance(pfx[j],pfy[j], fx,fy) + computedistance(psx[j],psy[j], sx,sy);

				/* LINTED "bestdist" used in proper order */
				if (j == 0 || dist < bestdist)
				{
					bestdist = dist;
					bestone = j;
				}
			}

			/* set the best one */
			us_xformpointtonode(pfx[bestone], pfy[bestone], firsthigh->fromgeom->entryaddr.ni,
				&firsthigh->snapx, &firsthigh->snapy);
			us_xformpointtonode(psx[bestone], psy[bestone], secondhigh->fromgeom->entryaddr.ni,
				&secondhigh->snapx, &secondhigh->snapy);
		} else
		{
			/* compute tangent to first object */
			us_getsnappoint(secondhigh, &sx, &sy);
			us_adjustonetangent(firsthigh, firstpoly, sx, sy);
		}
	} else
	{
		if ((secondhigh->status&HIGHSNAPTAN) != 0)
		{
			us_getsnappoint(firsthigh, &fx, &fy);
			us_adjustonetangent(secondhigh, secondpoly, fx, fy);
		}
	}
}

/*
 * Routine to adjust the snap point on "high" so that it is tangent to its curved
 * polygon "poly" and runs through (x, y).
 */
void us_adjustonetangent(HIGHLIGHT *high, POLYGON *poly, INTBIG x, INTBIG y)
{
	REGISTER NODEINST *ni;
	INTBIG ix1, iy1, ix2, iy2, fx, fy;
	REGISTER INTBIG xv, yv;

	if (!high->fromgeom->entryisnode) return;
	ni = high->fromgeom->entryaddr.ni;
	us_getsnappoint(high, &fx, &fy);
	if (circletangents(x, y, poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1],
		&ix1, &iy1, &ix2, &iy2)) return;
	if (computedistance(fx, fy, ix1, iy1) > computedistance(fx, fy, ix2, iy2))
	{
		xv = ix1;   ix1 = ix2;   ix2 = xv;
		yv = iy1;   iy1 = iy2;   iy2 = yv;
	}

	if ((poly->style != CIRCLEARC && poly->style != THICKCIRCLEARC) ||
		us_pointonarc(ix1, iy1, poly))
	{
		us_xformpointtonode(ix1, iy1, ni, &high->snapx, &high->snapy);
		return;
	}
	if ((poly->style != CIRCLEARC && poly->style != THICKCIRCLEARC) ||
		us_pointonarc(ix2, iy2, poly))
	{
		us_xformpointtonode(ix2, iy2, ni, &high->snapx, &high->snapy);
		return;
	}
}

/*
 * Routine to adjust the two highlight modules "firsthigh" and "secondhigh" to account for the
 * fact that one or both has a perpendicular snap point that must be perpendicular
 * to the other's snap point.
 */
void us_adjustperpendicularsnappoints(HIGHLIGHT *firsthigh, HIGHLIGHT *secondhigh)
{
	INTBIG fx, fy;
	static POLYGON *secondpoly = NOPOLYGON;
	REGISTER NODEINST *ni;
	XARRAY trans;

	if ((secondhigh->status&HIGHSNAPPERP) != 0)
	{
		/* get polygon describing second object */
		(void)needstaticpolygon(&secondpoly, 4, us_tool->cluster);
		if (!secondhigh->fromgeom->entryisnode) return;
		ni = secondhigh->fromgeom->entryaddr.ni;
		if (ni->proto->primindex == 0) return;
		makerot(ni, trans);
		(void)nodepolys(ni, 0, NOWINDOWPART);
		shapenodepoly(ni, 0, secondpoly);
		xformpoly(secondpoly, trans);

		us_getsnappoint(firsthigh, &fx, &fy);
		us_adjustoneperpendicular(secondhigh, secondpoly, fx, fy);
	}
}

/*
 * Routine to adjust the snap point on "high" so that it is perpendicular to
 * polygon "poly" and point (x, y).
 */
void us_adjustoneperpendicular(HIGHLIGHT *high, POLYGON *poly, INTBIG x, INTBIG y)
{
	REGISTER NODEINST *ni;
	REGISTER INTBIG rad;
	INTBIG ix, iy;
	REGISTER INTBIG ang;

	if (!high->fromgeom->entryisnode) return;
	ni = high->fromgeom->entryaddr.ni;

	if (poly->style == CIRCLE || poly->style == THICKCIRCLE ||
		poly->style == CIRCLEARC || poly->style == THICKCIRCLEARC)
	{
		/* compute perpendicular point */
		ang = figureangle(poly->xv[0], poly->yv[0], x, y);
		rad = computedistance(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1]);
		ix = poly->xv[0] + mult(cosine(ang), rad);
		iy = poly->yv[0] + mult(sine(ang), rad);
		if (poly->style == CIRCLEARC || poly->style == THICKCIRCLEARC ||
			!us_pointonarc(ix, iy, poly)) return;
		us_xformpointtonode(ix, iy, ni, &high->snapx, &high->snapy);
		return;
	}

	/* handle straight line perpendiculars */
	ix = x;   iy = y;
	(void)closestpointtosegment(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1], &ix, &iy);
	if (ix != x || iy != y) us_xformpointtonode(ix, iy, ni, &high->snapx, &high->snapy);
}

/*
 * routine to determine whether the point (x, y) is on the arc in "poly".
 * returns true if so.
 */
BOOLEAN us_pointonarc(INTBIG x, INTBIG y, POLYGON *poly)
{
	REGISTER INTBIG angle, startangle, endangle;

	if (poly->style != CIRCLEARC && poly->style != THICKCIRCLEARC) return(FALSE);

	angle = figureangle(poly->xv[0], poly->yv[0], x, y);
	endangle = figureangle(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1]);
	startangle = figureangle(poly->xv[0], poly->yv[0], poly->xv[2], poly->yv[2]);

	if (endangle > startangle)
	{
		if (angle >= startangle && angle <= endangle) return(TRUE);
	} else
	{
		if (angle >= startangle || angle <= endangle) return(TRUE);
	}
	return(FALSE);
}

/*
 * routine to get the true coordinate of the snap point in "high" and place it in (x,y).
 */
void us_getsnappoint(HIGHLIGHT *high, INTBIG *x, INTBIG *y)
{
	REGISTER NODEINST *ni;
	XARRAY trans;
	INTBIG xt, yt;

	if (high->fromgeom->entryisnode)
	{
		ni = high->fromgeom->entryaddr.ni;
		makeangle(ni->rotation, ni->transpose, trans);
		xform(high->snapx, high->snapy, &xt, &yt, trans);
		*x = (ni->highx + ni->lowx) / 2 + xt;
		*y = (ni->highy + ni->lowy) / 2 + yt;
	} else
	{
		*x = (high->fromgeom->highx + high->fromgeom->lowx) / 2 + high->snapx;
		*y = (high->fromgeom->highy + high->fromgeom->lowy) / 2 + high->snapy;
	}
}

void us_setbestsnappoint(HIGHLIGHT *best, INTBIG wantx, INTBIG wanty, INTBIG newx, INTBIG newy,
	BOOLEAN tan, BOOLEAN perp)
{
	REGISTER INTBIG olddist, newdist;
	INTBIG oldx, oldy;

	if ((best->status & HIGHSNAP) != 0)
	{
		us_getsnappoint(best, &oldx, &oldy);
		olddist = computedistance(wantx, wanty, oldx, oldy);
		newdist = computedistance(wantx, wanty, newx, newy);
		if (newdist >= olddist) return;
	}

	/* set the snap point */
	if (best->fromgeom->entryisnode)
	{
		us_xformpointtonode(newx, newy, best->fromgeom->entryaddr.ni, &best->snapx, &best->snapy);
	} else
	{
		best->snapx = newx - (best->fromgeom->highx + best->fromgeom->lowx) / 2;
		best->snapy = newy - (best->fromgeom->highy + best->fromgeom->lowy) / 2;
	}
	best->status |= HIGHSNAP;
	if (tan) best->status |= HIGHSNAPTAN;
	if (perp) best->status |= HIGHSNAPPERP;
}

void us_xformpointtonode(INTBIG x, INTBIG y, NODEINST *ni, INTBIG *xo, INTBIG *yo)
{
	XARRAY trans;
	INTBIG xv, yv;

	if (ni->transpose != 0) makeangle(ni->rotation, ni->transpose, trans); else
		makeangle((3600 - ni->rotation)%3600, 0, trans);
	xv = x - (ni->highx + ni->lowx) / 2;
	yv = y - (ni->highy + ni->lowy) / 2;
	xform(xv, yv, xo, yo, trans);
}

/*
 * routine to return the object that is closest to point (rdx, rdy)
 * or within "slop" of that point in cell "cell".  Searches nodes first.
 * This is used in the "create join-angle" command.
 */
GEOM *us_getclosest(INTBIG rdx, INTBIG rdy, INTBIG slop, NODEPROTO *cell)
{
	REGISTER GEOM *geom, *highgeom, *bestgeom;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER INTBIG sea, bestdist, dist;
	static POLYGON *poly = NOPOLYGON;
	REGISTER VARIABLE *var;
	HIGHLIGHT high;

	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	highgeom = NOGEOM;
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var != NOVARIABLE)
	{
		if (getlength(var) == 1)
		{
			(void)us_makehighlight(((CHAR **)var->addr)[0], &high);
			highgeom = high.fromgeom;
		}
	}

	/* see if there is a direct hit on another node */
	sea = initsearch(rdx-slop, rdx+slop, rdy-slop, rdy+slop, cell);
	bestdist = MAXINTBIG;
	for(;;)
	{
		geom = nextobject(sea);
		if (geom == NOGEOM) break;
		if (geom == highgeom) continue;
		if (!geom->entryisnode) continue;
		ni = geom->entryaddr.ni;
		if ((ni->userbits&HARDSELECTN) != 0) continue;
		if (ni->proto->primindex != 0 && (ni->proto->userbits&NINVISIBLE) != 0)
			continue;
		dist = us_disttoobject(rdx, rdy, geom);
		if (dist > bestdist) continue;
		bestdist = dist;
		bestgeom = geom;
	}
	if (bestdist < 0) return(bestgeom);

	/* look at arcs second */
	bestdist = MAXINTBIG;
	sea = initsearch(rdx-slop, rdx+slop, rdy-slop, rdy+slop, cell);
	for(;;)
	{
		geom = nextobject(sea);
		if (geom == NOGEOM) break;
		if (geom == highgeom) continue;
		if (geom->entryisnode) continue;
		ai = geom->entryaddr.ai;
		if ((ai->userbits&HARDSELECTA) != 0) continue;
		if ((ai->proto->userbits&AINVISIBLE) != 0) continue;
		dist = us_disttoobject(rdx, rdy, geom);
		if (dist > bestdist) continue;
		bestdist = dist;
		bestgeom = geom;
	}
	if (bestdist < 0) return(bestgeom);
	return(NOGEOM);
}

/*
 * Routine to return the distance from point (x,y) to object "geom".
 * Negative values are direct hits.
 */
INTBIG us_disttoobject(INTBIG x, INTBIG y, GEOM *geom)
{
	XARRAY trans;
	REGISTER INTBIG wid, bestdist, dist, fun;
	REGISTER INTBIG count, box;
	static POLYGON *poly = NOPOLYGON;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	INTBIG plx, ply, phx, phy;

	(void)needstaticpolygon(&poly, 4, us_tool->cluster);
	if (geom->entryisnode)
	{
		ni = geom->entryaddr.ni;
		makerot(ni, trans);

		/* special case for MOS transistors: examine the gate/active tabs */
		fun = (ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH;
		if (fun == NPTRANMOS || fun == NPTRAPMOS || fun == NPTRADMOS)
		{
			count = nodepolys(ni, 0, NOWINDOWPART);
			bestdist = MAXINTBIG;
			for(box=0; box<count; box++)
			{
				shapenodepoly(ni, box, poly);
				fun = layerfunction(ni->proto->tech, poly->layer) & LFTYPE;
				if (!layerispoly(fun) && fun != LFDIFF) continue;
				xformpoly(poly, trans);
				dist = polydistance(poly, x, y);
				if (dist < bestdist) bestdist = dist;
			}
			return(bestdist);
		}

		/* special case for 1-polygon primitives: check precise distance to cursor */
		if (ni->proto->primindex != 0 && (ni->proto->userbits&NEDGESELECT) != 0)
		{
			count = nodepolys(ni, 0, NOWINDOWPART);
			bestdist = MAXINTBIG;
			for(box=0; box<count; box++)
			{
				shapenodepoly(ni, box, poly);
				if ((poly->desc->colstyle&INVISIBLE) != 0) continue;
				xformpoly(poly, trans);
				dist = polydistance(poly, x, y);
				if (dist < bestdist) bestdist = dist;
			}
			return(bestdist);
		}

		/* get the bounds of the node in a polygon */
		nodesizeoffset(ni, &plx, &ply, &phx, &phy);
		maketruerectpoly(ni->lowx+plx, ni->highx-phx, ni->lowy+ply, ni->highy-phy, poly);
		poly->style = FILLEDRECT;
		xformpoly(poly, trans);
		return(polydistance(poly, x, y));
	}

	/* determine distance to arc */
	ai = geom->entryaddr.ai;

	/* if arc is selectable precisely, check distance to cursor */
	if ((ai->proto->userbits&AEDGESELECT) != 0)
	{
		count = arcpolys(ai, NOWINDOWPART);
		bestdist = MAXINTBIG;
		for(box=0; box<count; box++)
		{
			shapearcpoly(ai, box, poly);
			if ((poly->desc->colstyle&INVISIBLE) != 0) continue;
			dist = polydistance(poly, x, y);
			if (dist < bestdist) bestdist = dist;
		}
		return(bestdist);
	}

	/* standard distance to the arc */
	wid = ai->width - arcwidthoffset(ai);
	if (wid == 0) wid = lambdaofarc(ai);
	if (curvedarcoutline(ai, poly, FILLED, wid))
		makearcpoly(ai->length, wid, ai, poly, FILLED);
	return(polydistance(poly, x, y));
}

/*********************************** TEXT ON NODES/ARCS ***********************************/

static INTSML       us_nodearcvarptr;
static INTBIG       us_nodearcvarcount;
static INTSML       us_portvarptr;
static INTBIG       us_portvarcount;
static BOOLEAN      us_nodenameflg;
static PORTEXPINST *us_nodeexpptr;
static PORTPROTO   *us_portptr;

void us_initnodetext(NODEINST *ni, INTBIG findspecial, WINDOWPART *win)
{
	us_nodearcvarptr = 0;
	us_nodearcvarcount = tech_displayablenvars(ni, win, &tech_oneprocpolyloop);
	us_portvarptr = 0;
	us_portvarcount = 0;

	us_nodenameflg = TRUE;
	if (findspecial != 0)
	{
		/* only select cell instance names if visible */
		if ((us_useroptions&HIDETXTINSTNAME) == 0)
			us_nodenameflg = FALSE;
	} else
	{
		/* if the "special" option is not set and node text is disabled, skip it */
		if ((us_useroptions&NOTEXTSELECT) != 0) us_nodearcvarptr = ni->numvar;
	}
	if ((us_useroptions&HIDETXTEXPORT) != 0) us_nodeexpptr = NOPORTEXPINST; else
		us_nodeexpptr = ni->firstportexpinst;
}

BOOLEAN us_getnodetext(NODEINST *ni, WINDOWPART *win, POLYGON *poly, VARIABLE **var,
	VARIABLE **varnoeval, PORTPROTO **port)
{
	INTBIG xc, yc, lx, hx, ly, hy;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	REGISTER INTBIG portstyle, i;
	REGISTER PORTEXPINST *pe;
	XARRAY trans;

	for(;;)
	{
		if (!us_nodenameflg)
		{
			us_nodenameflg = TRUE;
			if (ni->proto->primindex == 0 && (ni->userbits&NEXPAND) == 0)
			{
				*var = *varnoeval = NOVARIABLE;
				*port = NOPORTPROTO;
				us_maketextpoly(describenodeproto(ni->proto), win,
					(ni->lowx + ni->highx) / 2, (ni->lowy + ni->highy) / 2,
						ni, ni->parent->tech, ni->textdescript, poly);
				poly->style = FILLED;
				return(FALSE);
			}
		}
		if (us_nodearcvarptr < us_nodearcvarcount)
		{
			*var = tech_filldisplayablenvar(ni, poly, win, varnoeval, &tech_oneprocpolyloop);
			makerot(ni, trans);
			TDCOPY(descript, (*var)->textdescript);
			if (TDGETPOS(descript) == VTPOSBOXED)
			{
				xformpoly(poly, trans);
				getbbox(poly, &lx, &hx, &ly, &hy);
				us_filltextpoly(poly->string, win, (lx + hx) / 2, (ly + hy) / 2,
					trans, ni->parent->tech, descript, ni->geom, poly);
				for(i=0; i<poly->count; i++)
				{
					if (poly->xv[i] < lx) poly->xv[i] = lx;
					if (poly->xv[i] > hx) poly->xv[i] = hx;
					if (poly->yv[i] < ly) poly->yv[i] = ly;
					if (poly->yv[i] > hy) poly->yv[i] = hy;
				}
			} else
			{
				xform(poly->xv[0], poly->yv[0], &poly->xv[0], &poly->yv[0], trans);
				us_filltextpoly(poly->string, win, poly->xv[0], poly->yv[0],
					trans, ni->parent->tech, descript, ni->geom, poly);
			}
			*port = NOPORTPROTO;
			us_nodearcvarptr++;
			poly->style = FILLED;
			return(FALSE);
		}

		if (us_portvarptr < us_portvarcount)
		{
			*var = tech_filldisplayableportvar(us_portptr, poly, win, varnoeval, &tech_oneprocpolyloop);
			portposition(us_portptr->subnodeinst, us_portptr->subportproto, &xc, &yc);
			us_maketextpoly(poly->string, win, xc, yc, us_portptr->subnodeinst,
				us_portptr->subnodeinst->parent->tech, (*var)->textdescript, poly);
			*port = us_portptr;
			us_portvarptr++;
			poly->style = FILLED;
			return(FALSE);
		}

		/* check exports on the node */
		if (us_nodeexpptr != NOPORTEXPINST)
		{
			pe = us_nodeexpptr;
			us_nodeexpptr = pe->nextportexpinst;
			us_portptr = pe->exportproto;
			us_portvarcount = tech_displayableportvars(us_portptr, win, &tech_oneprocpolyloop);
			us_portvarptr = 0;

			portstyle = us_useroptions & EXPORTLABELS;
			if (portstyle == EXPORTSCROSS) continue;
			*port = pe->exportproto;
			*var = *varnoeval = NOVARIABLE;

			/* build polygon that surrounds text */
			portposition(ni, (*port)->subportproto, &xc, &yc);
			us_maketextpoly(us_displayedportname(*port, portstyle >> EXPORTLABELSSH),
				win, xc, yc, ni, ni->parent->tech, (*port)->textdescript, poly);
			poly->style = FILLED;
			return(FALSE);
		}
		break;
	}

	return(TRUE);
}

void us_initarctext(ARCINST *ai, INTBIG findspecial, WINDOWPART *win)
{
	us_nodearcvarptr = 0;
	us_nodearcvarcount = tech_displayableavars(ai, win, &tech_oneprocpolyloop);
	if (findspecial == 0)
	{
		/* if the "special" option is not set and arc text is disabled, skip it */
		if ((us_useroptions&NOTEXTSELECT) != 0) us_nodearcvarptr = ai->numvar;
	}
}

BOOLEAN us_getarctext(ARCINST *ai, WINDOWPART *win, POLYGON *poly, VARIABLE **var, VARIABLE **varnoeval)
{
	if (us_nodearcvarptr < us_nodearcvarcount)
	{
		*var = tech_filldisplayableavar(ai, poly, win, varnoeval, &tech_oneprocpolyloop);
		us_maketextpoly(poly->string, win,
			(ai->end[0].xpos + ai->end[1].xpos) / 2, (ai->end[0].ypos + ai->end[1].ypos) / 2,
				NONODEINST, ai->parent->tech, (*var)->textdescript, poly);
		us_nodearcvarptr++;
		poly->style = FILLED;
		return(FALSE);
	}
	return(TRUE);
}

/*
 * routine to build a polygon in "poly" that has four points describing the
 * text in "str" with descriptor "descript".  The text is in window "win"
 * on an object whose center is (xc,yc) and is on node "ni" (or not if NONODEINST)
 * and uses technology "tech".
 */
void us_maketextpoly(CHAR *str, WINDOWPART *win, INTBIG xc, INTBIG yc, NODEINST *ni,
	TECHNOLOGY *tech, UINTBIG *descript, POLYGON *poly)
{
	INTBIG newxc, newyc, lambda;
	XARRAY trans;
	REGISTER GEOM *geom;

	/* determine location of text */
	if (ni == NONODEINST)
	{
		transid(trans);
		lambda = el_curlib->lambda[tech->techindex];
	} else
	{
		makeangle(ni->rotation, ni->transpose, trans);
		lambda = ni->parent->lib->lambda[tech->techindex];
	}

	newxc = TDGETXOFF(descript);
	newxc = newxc * lambda / 4;
	newyc = TDGETYOFF(descript);
	newyc = newyc * lambda / 4;
	xform(newxc, newyc, &newxc, &newyc, trans);
	xc += newxc;   yc += newyc;
	if (ni == NONODEINST) geom = NOGEOM; else
		geom = ni->geom;
	us_filltextpoly(str, win, xc, yc, trans, tech, descript, geom, poly);
}

void us_filltextpoly(CHAR *str, WINDOWPART *win, INTBIG xc, INTBIG yc, XARRAY trans,
	TECHNOLOGY *tech, UINTBIG *descript, GEOM *geom, POLYGON *poly)
{
	INTBIG xw, yw;

	/* determine size of text */
	us_gettextscreensize(str, descript, win, tech, geom, &xw, &yw);

	switch (TDGETPOS(descript))
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
	poly->style = rotatelabel(poly->style, TDGETROTATION(descript), trans);

	switch (poly->style)
	{
		case TEXTTOP:                    yc -= yw/2;   break;
		case TEXTBOT:                    yc += yw/2;   break;
		case TEXTLEFT:     xc += xw/2;                 break;
		case TEXTRIGHT:    xc -= xw/2;                 break;
		case TEXTTOPLEFT:  xc += xw/2;   yc -= yw/2;   break;
		case TEXTBOTLEFT:  xc += xw/2;   yc += yw/2;   break;
		case TEXTTOPRIGHT: xc -= xw/2;   yc -= yw/2;   break;
		case TEXTBOTRIGHT: xc -= xw/2;   yc += yw/2;   break;
	}

	/* construct polygon with actual size */
	poly->xv[0] = xc - xw/2;   poly->yv[0] = yc - yw/2;
	poly->xv[1] = xc - xw/2;   poly->yv[1] = yc + yw/2;
	poly->xv[2] = xc + xw/2;   poly->yv[2] = yc + yw/2;
	poly->xv[3] = xc + xw/2;   poly->yv[3] = yc - yw/2;
	poly->count = 4;
	poly->layer = -1;
	poly->style = CLOSED;
}

/*
 * Routine to determine the size (in database units) of the string "str", drawn in window "w"
 * with text descriptor "descript".  The text is on object "geom", technology "tech".  The size
 * is returned in (xw,yw).
 */
void us_gettextscreensize(CHAR *str, UINTBIG *descript, WINDOWPART *w, TECHNOLOGY *tech, GEOM *geom,
	INTBIG *xw, INTBIG *yw)
{
	REGISTER INTBIG lambda, newsize, oldsize, abssize, xabssize, yabssize,
		sizex, sizey;
	INTBIG sslx, sshx, ssly, sshy;
	float sscalex, sscaley;
	INTBIG tsx, tsy;
	REGISTER LIBRARY *lib;
	static BOOLEAN canscalefonts, fontscalingunknown = TRUE;
	REGISTER BOOLEAN reltext;

	/* see if relative font scaling should be done */
	if (fontscalingunknown)
	{
		fontscalingunknown = FALSE;
		canscalefonts = graphicshas(CANSCALEFONTS);
	}
	reltext = FALSE;
	if (canscalefonts)
	{
		if ((TDGETSIZE(descript)&TXTQLAMBDA) != 0) reltext = TRUE;
	}
	if (TDGETPOS(descript) == VTPOSBOXED)
	{
		if (geom == NOGEOM) TDSETPOS(descript, VTPOSCENT); else
		{
			sizex = roundfloat((geom->highx - geom->lowx) * w->scalex);
			sizey = roundfloat((geom->highy - geom->lowy) * w->scaley);
		}
	}
	if (reltext)
	{
		/* relative size text */
		if (w->curnodeproto == NONODEPROTO) lib = el_curlib; else
			lib = w->curnodeproto->lib;
		lambda = lib->lambda[tech->techindex];
		sslx = w->screenlx;   w->screenlx = w->uselx * lambda / 12;
		sshx = w->screenhx;   w->screenhx = w->usehx * lambda / 12;
		ssly = w->screenly;   w->screenly = w->usely * lambda / 12;
		sshy = w->screenhy;   w->screenhy = w->usehy * lambda / 12;
		sscalex = w->scalex;   sscaley = w->scaley;
		computewindowscale(w);

		if (TDGETPOS(descript) == VTPOSBOXED)
		{
			for(;;)
			{
				screensettextinfo(w, tech, descript);
				screengettextsize(w, str, &tsx, &tsy);
				if (tsx <= sizex && tsy <= sizey) break;
				newsize = TXTGETQLAMBDA(TDGETSIZE(descript)) - 1;
				if (newsize <= 0) break;
				TDSETSIZE(descript, TXTSETQLAMBDA(newsize));
			}
		} else
		{
			screensettextinfo(w, tech, descript);
			screengettextsize(w, str, &tsx, &tsy);
		}
		*xw = muldiv(tsx, w->screenhx-w->screenlx, w->usehx-w->uselx);
		*yw = muldiv(tsy, w->screenhy-w->screenly, w->usehy-w->usely);
		w->screenlx = sslx;   w->screenhx = sshx;
		w->screenly = ssly;   w->screenhy = sshy;
		w->scalex = sscalex;  w->scaley = sscaley;
	} else
	{
		/* absolute size text */
		if (TDGETPOS(descript) == VTPOSBOXED)
		{
			for(;;)
			{
				screensettextinfo(w, tech, descript);
				screengettextsize(w, str, &tsx, &tsy);
				if (tsx <= sizex && tsy <= sizey) break;
				oldsize = TDGETSIZE(descript);
				abssize = TXTGETPOINTS(oldsize);

				/* jump quickly to the proper font size */
				if (tsx <= sizex) xabssize = abssize; else
					xabssize = abssize * sizex / tsx;
				if (tsy <= sizey) yabssize = abssize; else
					yabssize = abssize * sizey / tsy;
				newsize = mini(xabssize, yabssize);
				if (newsize < 4) break;
				TDSETSIZE(descript, TXTSETPOINTS(newsize));
			}
		} else
		{
			screensettextinfo(w, tech, descript);
			screengettextsize(w, str, &tsx, &tsy);
		}
		*xw = muldiv(tsx, w->screenhx-w->screenlx, w->usehx-w->uselx);
		*yw = muldiv(tsy, w->screenhy-w->screenly, w->usehy-w->usely);
	}
}
