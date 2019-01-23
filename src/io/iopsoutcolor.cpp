/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iopsoutcolor.cpp
 * Input/output tool: Color Postscript output
 * Written by: David Harris, 4/20/01 (David_Harris@hmc.edu)
 * Updated to speed up handling transparencies, 1/20/02 (David Harris)
 * Integrated into Electric: 2/03 (Steven Rubin)
 *
 * Copyright (c) 2003 Static Free Software.
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
 * Things still to do:
 *    circles and arcs
 *    rotation of the plot
 *    eps
 *    plot text (an option?)
 */

/* 
	This code generates postscript plots of an IC layout in CIF format.
	It handles color better than existing freely available
	postscript generators.  It does not handle arbitrary rotations.

	To use ICPLOT
	look at the output with RoPS
	To print from windows in E-sized format (34" wide) *** outdated
		use RoPS to print to the plotter (C3PO)
		Application Page Size: ANSI E
		set the options to Color, then under Advanced to E-sized sheet
		be sure plotter driver under Windows is set to E-size (a D-size driver also exists)

	*** updated 12/10/01 for qbert
		use RoPS to print to the plotter (C3PO)
		Application Page Size: ANSI A, fit to page ANSI E
		Advanced tab: process document in computer
		be sure plotter driver under Windows is set to E-size (a D-size driver also exists)


	Limitations:
	the code to handle quad trees is rather messy now

	ideas:
		draw highlights around edges
		center port labels
		give options about aspect ratio / page size
		put date in postscript and on caption
		print layers on caption

	To do 1/28/03:
		draw outlines around edges
		handle black & white mode
		center labels
*/

#include "config.h"
#include "global.h"
#include "egraphics.h"
#include "efunction.h"
#include "eio.h"
#include "usr.h"
#include <math.h>

/* macros */

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

/* Constants */

#define MAXLAYERS 1000
#define TREETHRESHOLD 500
#define SMALL_NUM 0.000001

/* Structures */

typedef struct POLYL {
	INTBIG *coords;
	INTBIG numcoords;
	INTBIG layer;
	struct POLYL *next;
} POLYL;

typedef struct OUTLINEL {
	struct OUTLINEL *next;
} OUTLINEL;

typedef struct BOXL {
	INTBIG pos[4];  /* 0: dx, 1: dy, 2: left, 3: bot */
	INTSML layer;
	INTSML visible;
	struct BOXL *next;
} BOXL;

typedef struct LINEL {
	INTBIG x[2];
	INTBIG y;
	INTSML visible;
	struct LINEL *next;
} LINEL;

typedef struct LABELL {
	CHAR   *label;
	INTBIG  pos[4];
	INTBIG  style;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	struct LABELL *next;
} LABELL;

typedef struct CELLL {
	INTBIG	cellNum;
	BOXL   *box;
	POLYL  *poly;
	LABELL *label;
	CELLL  *next;
	struct CELLINSTL *inst;
} CELLL;

typedef struct CELLINSTL {
	float      transform[9];
	CELLL     *inst;
	CELLINSTL *next;
} CELLINSTL;

typedef struct {
	INTBIG pos[4]; /* 0: dx, 1: dy, 2: left, 3: bot */
	INTSML layer;
	INTSML visible;
} BOXELEM;

typedef struct BOXQUADTREE {
	INTBIG numBoxes;
	BOXELEM *boxes;
	INTBIG bounds[4]; /* 0: dx, 1: dy, 2: left, 3: bot */
	struct BOXQUADTREE *tl;
	struct BOXQUADTREE *tr;
	struct BOXQUADTREE *bl;
	struct BOXQUADTREE *br;
	struct BOXQUADTREE *parent;
	INTBIG level;
} BOXQUADTREE;

typedef struct LAYERMAP {
	INTBIG layernumber;
	TECHNOLOGY *tech;
	INTBIG mix1, mix2; /* layer made from mix of previous layers, or -1 if not */
	double r, g, b;
	double opacity;
	INTBIG foreground;
} LAYERMAP;

typedef struct
{
	INTBIG layer;
	TECHNOLOGY *tech;
	float height;
	INTBIG r, g, b;
	float  opacity;
	INTBIG foreground;
} LAYERSORT;

/* Globals */
static INTBIG       io_pscolor_cifBoundaries[4];
static CELLL       *io_pscolor_cells = NULL;
static LAYERMAP     io_pscolor_layers[MAXLAYERS];
static BOXL        *io_pscolor_flattenedbox[MAXLAYERS];
static BOXQUADTREE *io_pscolor_boxQuadTrees[MAXLAYERS];
static POLYL       *io_pscolor_flattenedpoly[MAXLAYERS];
static INTBIG       io_pscolor_numLayers;
static INTBIG       io_pscolor_numBoxes = 0;
static INTBIG       io_pscolor_numCells = 0;
static INTBIG       io_pscolor_numPolys = 0;
static INTBIG       io_pscolor_numInstances = 0;
static INTBIG       io_pscolor_cellnumber;
static BOOLEAN      io_pscolor_curvewarning;
static CHAR         io_pscolor_font[80];
static INTBIG       io_pscolor_totalBoxes = 0;

/* not used yet */
#if 0
static LINEL       *io_pscolor_horizsegs[MAXLAYERS];
static OUTLINEL    *io_pscolor_flattenedoutline[MAXLAYERS];

static LINEL       *io_pscolor_genSegments(BOXL *boxes, INTBIG dir);
static int          io_pscolor_lineCompare(const void *a, const void *b);
static INTBIG       io_pscolor_countBoxes(BOXL *boxes);
static OUTLINEL    *io_pscolor_combineSegments(LINEL *horiz, LINEL *vert);
static void         io_pscolor_checkOverlap(CELLL*, BOXL*, BOXL*, INTBIG);
static void			io_pscolor_printBoxes(void);
static void         io_pscolor_freeLine(LINEL*);
#endif

/* Function Prototypes */
static void         io_pscolor_initICPlot(void);
static void         io_pscolor_getLayerMap(TECHNOLOGY *tech);
static void         io_pscolor_extractDatabase(NODEPROTO*);
static void         io_pscolor_genOverlapShapesAfterFlattening(void);
static void         io_pscolor_coaf1(BOXQUADTREE*, BOXQUADTREE*, INTBIG);
static void         io_pscolor_coaf2(BOXQUADTREE*, BOXQUADTREE*, INTBIG);
static void         io_pscolor_coaf3(BOXQUADTREE*, BOXQUADTREE*, INTBIG);
static void         io_pscolor_coaf4(BOXQUADTREE*, BOXQUADTREE*, INTBIG, INTBIG);
static void         io_pscolor_checkOverlapAfterFlattening(BOXQUADTREE*, BOXQUADTREE*, INTBIG);
static BOXQUADTREE *io_pscolor_makeBoxQuadTree(INTBIG);
static void         io_pscolor_recursivelyMakeBoxQuadTree(BOXQUADTREE*);
static void         io_pscolor_splitTree(INTBIG, BOXELEM*, BOXQUADTREE*, INTBIG, INTBIG, INTBIG, INTBIG);
static void         io_pscolor_flatten(void);
static void         io_pscolor_recursiveFlatten(CELLL*, float[]);
static void         io_pscolor_writePS(NODEPROTO*, BOOLEAN, INTBIG, INTBIG, INTBIG);
static void         io_pscolor_mergeBoxes(void);
static INTBIG       io_pscolor_mergeBoxPair(BOXL*, BOXL*);
static BOXL        *io_pscolor_copybox(BOXL*, float[]);
static POLYL       *io_pscolor_copypoly(POLYL*, float[]);
static CELLL       *io_pscolor_insertCell(CELLL*, CELLL*);
static BOXL        *io_pscolor_insertbox(BOXL*, BOXL*);
static POLYL       *io_pscolor_insertpoly(POLYL*, POLYL*);
static LABELL      *io_pscolor_insertlabel(LABELL*, LABELL*);
static CELLINSTL   *io_pscolor_insertInst(CELLINSTL*, CELLINSTL*);
static void         io_pscolor_matrixMul(float[], float[], float[]);
static void         io_pscolor_newIdentityMatrix(float[]);
static void         io_pscolor_transformBox(INTBIG*, INTBIG*, float[]);
static void         io_pscolor_transformPoly(POLYL*, POLYL*, float[]);
static void         io_pscolor_printStatistics(void);
static void         io_pscolor_freeMem(void);
static void         io_pscolor_freeBoxQuadTree(BOXQUADTREE*);
static void         io_pscolor_freeCell(CELLL*);
static void         io_pscolor_freeBox(BOXL*);
static void         io_pscolor_freePoly(POLYL*);
static void         io_pscolor_freeLabel(LABELL*);
static void         io_pscolor_freeCellInst(CELLINSTL*);
static void         io_pscolor_plotPolygon(POLYGON *poly, CELLL *curCell);
static INTBIG       io_pscolor_truefontsize(INTBIG font, TECHNOLOGY *tech);
static int          io_pscolor_sortlayers(const void *e1, const void *e2);
static void         io_pscolor_plotline(INTBIG layer, INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty, INTBIG texture, CELLL *curCell);

/*
 * Main entry point for color PostScript output.  The cell being written is "np".
 * The size of the paper is "pagewid" wide and "pagehei" tall with a margin of
 * "pagemargin" (in 1/75 of an inch).  If "useplotter" is TRUE, this is an infinitely-tall
 * plotter, so height is not a consideration.  If "epsformat" is TRUE, write encapsulated
 * PostScript.
 */
/* extern "C" extern TECHNOLOGY *sch_tech, *gen_tech, *art_tech; */
void io_pscolorplot(NODEPROTO *np, BOOLEAN epsformat, BOOLEAN useplotter,
	INTBIG pagewid, INTBIG pagehei, INTBIG pagemargin)
{
	REGISTER TECHNOLOGY *tech;

	strcpy(io_pscolor_font, x_("Helvetica"));

	io_pscolor_initICPlot();

	/* initialize layer maps for the current technology */
	io_pscolor_numLayers = 0;
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		tech->temp1 = 0;
/* io_pscolor_getLayerMap(sch_tech); */
/* io_pscolor_getLayerMap(art_tech); */
/* io_pscolor_getLayerMap(gen_tech); */
	io_pscolor_getLayerMap(el_curtech);

	io_pscolor_curvewarning = FALSE;
	io_pscolor_extractDatabase(np);
	io_pscolor_mergeBoxes(); 
/*	io_pscolor_printBoxes(); */
	io_pscolor_flatten();
/*	io_pscolor_printBoxes(); */
	io_pscolor_genOverlapShapesAfterFlattening();
/*	io_pscolor_printBoxes(); */
	io_pscolor_writePS(np, useplotter, pagewid, pagehei, pagemargin);
	io_pscolor_printStatistics();
	io_pscolor_freeMem();
}

void io_pscolor_initICPlot(void)
{
	INTBIG i;
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;

	io_pscolor_numBoxes = io_pscolor_numCells = io_pscolor_numPolys =
		io_pscolor_totalBoxes = io_pscolor_numInstances = 0;
	io_pscolor_cifBoundaries[0] = 1<<30;
	io_pscolor_cifBoundaries[1] = 1<<30;
	io_pscolor_cifBoundaries[2] = -1<<30;
	io_pscolor_cifBoundaries[3] = -1<<30;

	for (i=0; i<MAXLAYERS; i++) {
		io_pscolor_boxQuadTrees[i] = NULL;
	}

	/* mark all cells as "not written" */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = 0;
	io_pscolor_cellnumber = 1;
}

/*
 * Routine to get the print colors and load them into the layer map.
 */
void io_pscolor_getLayerMap(TECHNOLOGY *tech)
{
	INTBIG i, j, k, fun, *printcolors, curLayer, startlayer;
	float thickness;
	REGISTER LAYERSORT *ls;

	/* see if this technology has already been done */
	if (tech->temp1 != 0) return;
	tech->temp1 = 1;

	/* read layer map */
	startlayer = io_pscolor_numLayers;
	printcolors = us_getprintcolors(tech);
	if (printcolors == 0) return;
	ls = (LAYERSORT *)emalloc(tech->layercount * (sizeof (LAYERSORT)), io_tool->cluster);
	if (ls == 0) return;
	j = 0;
	for(i=0; i<tech->layercount; i++)
	{
		fun = layerfunction(tech, i);
		if ((fun&LFPSEUDO) != 0) continue;
		if (get3dfactors(tech, i, &ls[j].height, &thickness)) break;
		ls[j].height += thickness/2;
		ls[j].layer = i;
		ls[j].tech = tech;
		ls[j].r = printcolors[i*5];
		ls[j].g = printcolors[i*5+1];
		ls[j].b = printcolors[i*5+2];
		ls[j].opacity = (float)printcolors[i*5+3]/WHOLE;
		ls[j].foreground = printcolors[i*5+4];
		j++;
	}

	/* sort by layer height */
	esort(ls, j, sizeof (LAYERSORT), io_pscolor_sortlayers);

	/* load the layer information */
	for(i=0; i<j; i++)
	{
		if (io_pscolor_numLayers >= MAXLAYERS)
		{
			ttyputerr("More than %ld layers", MAXLAYERS);
			break;
		}
		io_pscolor_layers[io_pscolor_numLayers].layernumber = ls[i].layer;
		io_pscolor_layers[io_pscolor_numLayers].tech = ls[i].tech;
		io_pscolor_layers[io_pscolor_numLayers].opacity = ls[i].opacity;
		io_pscolor_layers[io_pscolor_numLayers].foreground = ls[i].foreground;
		io_pscolor_layers[io_pscolor_numLayers].r = ls[i].r/255.0f;
		io_pscolor_layers[io_pscolor_numLayers].g = ls[i].g/255.0f;
		io_pscolor_layers[io_pscolor_numLayers].b = ls[i].b/255.0f;
		io_pscolor_layers[io_pscolor_numLayers].mix1 = -1;
		io_pscolor_layers[io_pscolor_numLayers].mix2 = -1;
		if (io_pscolor_layers[io_pscolor_numLayers].opacity < 1)
		{
			/* create new layers to provide transparency */
			curLayer = io_pscolor_numLayers;
			for (k=startlayer; k < curLayer; k++)
			{
				if (io_pscolor_layers[k].foreground == 1)
				{
					io_pscolor_layers[++io_pscolor_numLayers].opacity = 1;
					io_pscolor_layers[io_pscolor_numLayers].layernumber = io_pscolor_layers[k].layernumber + 1000*io_pscolor_layers[curLayer].layernumber+1000000;
					io_pscolor_layers[io_pscolor_numLayers].tech = io_pscolor_layers[k].tech;
					io_pscolor_layers[io_pscolor_numLayers].foreground = 1;
					io_pscolor_layers[io_pscolor_numLayers].r = io_pscolor_layers[curLayer].r*io_pscolor_layers[curLayer].opacity +
						io_pscolor_layers[k].r*(1-io_pscolor_layers[curLayer].opacity);
					io_pscolor_layers[io_pscolor_numLayers].g = io_pscolor_layers[curLayer].g*io_pscolor_layers[curLayer].opacity +
						io_pscolor_layers[k].g*(1-io_pscolor_layers[curLayer].opacity);
					io_pscolor_layers[io_pscolor_numLayers].b = io_pscolor_layers[curLayer].b*io_pscolor_layers[curLayer].opacity +
						io_pscolor_layers[k].b*(1-io_pscolor_layers[curLayer].opacity);
					io_pscolor_layers[io_pscolor_numLayers].mix1 = k;
					io_pscolor_layers[io_pscolor_numLayers].mix2 = curLayer;
				}
			}
		}
		io_pscolor_numLayers++;
	}
	efree((CHAR *)ls);
}

/*
 * Helper routine for "io_pscolor_getLayerMap()" to sort layers by height.
 */
int io_pscolor_sortlayers(const void *e1, const void *e2)
{
	LAYERSORT *ls1, *ls2;
	float diff;

	ls1 = (LAYERSORT *)e1;
	ls2 = (LAYERSORT *)e2;
	diff = ls1->height - ls2->height;
	if (diff == 0.0) return(0);
	if (diff < 0.0) return(-1);
	return(1);
}

void io_pscolor_extractDatabase(NODEPROTO *cell)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp;
	CELLL *curCell, *subCell;
	CELLINSTL *curInst;
	BOXL *curbox;
	float transt[9], transr[9];
	LABELL *curlabel;
	INTBIG i;
	REGISTER INTBIG tot;
	INTBIG xp, yp;
	static POLYGON *poly = NOPOLYGON;
	XARRAY trans;

	/* check for subcells that haven't been written yet */
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex != 0) continue;
		if (ni->proto->temp1 != 0) continue;
		if ((ni->userbits&NEXPAND) == 0) continue;
		io_pscolor_extractDatabase(ni->proto);
	}

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, io_tool->cluster);

	/* create a cell */
	curCell = (CELLL *)emalloc(sizeof(CELLL), io_tool->cluster);
	curCell->cellNum = io_pscolor_cellnumber++;
	curCell->box = NULL;
	curCell->poly = NULL;
	curCell->label = NULL;
	curCell->inst = NULL;
	curCell->next = NULL;
	io_pscolor_numCells++;
	cell->temp1 = (INTBIG)curCell;

	/* examine all nodes in the cell */
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		np = ni->proto;
		if (np->primindex == 0)
		{
			/* instance */
			if ((ni->userbits&NEXPAND) == 0)
			{
				/* look for a black layer */
				for(i=0; i<io_pscolor_numLayers; i++)
					if (io_pscolor_layers[i].r == 0 && io_pscolor_layers[i].g == 0 &&
						io_pscolor_layers[i].b == 0 && io_pscolor_layers[i].opacity == 1) break;
				if (i < io_pscolor_numLayers)
				{
					/* draw a box by plotting 4 lines */
					curbox = (BOXL *)emalloc(sizeof(BOXL), io_tool->cluster);
					curbox->layer = (INTSML)i;
					curbox->next = NULL;
					curbox->visible = 1;
					curbox->pos[0] = 1;
					curbox->pos[1] = ni->geom->highy - ni->geom->lowy;
					curbox->pos[2] = ni->geom->lowx;
					curbox->pos[3] = ni->geom->lowy;
					curCell->box = io_pscolor_insertbox(curbox, curCell->box);

					curbox = (BOXL *)emalloc(sizeof(BOXL), io_tool->cluster);
					curbox->layer = (INTSML)i;
					curbox->next = NULL;
					curbox->visible = 1;
					curbox->pos[0] = ni->geom->highx - ni->geom->lowx;
					curbox->pos[1] = 1;
					curbox->pos[2] = ni->geom->lowx;
					curbox->pos[3] = ni->geom->lowy;
					curCell->box = io_pscolor_insertbox(curbox, curCell->box);

					curbox = (BOXL *)emalloc(sizeof(BOXL), io_tool->cluster);
					curbox->layer = (INTSML)i;
					curbox->next = NULL;
					curbox->visible = 1;
					curbox->pos[0] = 1;
					curbox->pos[1] = ni->geom->highy - ni->geom->lowy;
					curbox->pos[2] = ni->geom->highx;
					curbox->pos[3] = ni->geom->lowy;
					curCell->box = io_pscolor_insertbox(curbox, curCell->box);

					curbox = (BOXL *)emalloc(sizeof(BOXL), io_tool->cluster);
					curbox->layer = (INTSML)i;
					curbox->next = NULL;
					curbox->visible = 1;
					curbox->pos[0] = ni->geom->highx - ni->geom->lowx;
					curbox->pos[1] = 1;
					curbox->pos[2] = ni->geom->lowx;
					curbox->pos[3] = ni->geom->highy;
					curCell->box = io_pscolor_insertbox(curbox, curCell->box);

					/* add the cell name */
					curlabel = (LABELL *)emalloc(sizeof(LABELL), io_tool->cluster);
					(void)allocstring(&curlabel->label, describenodeproto(ni->proto), io_tool->cluster);
					curlabel->pos[0] = ni->lowx;
					curlabel->pos[1] = ni->highx;
					curlabel->pos[2] = ni->lowy;
					curlabel->pos[3] = ni->highy;
					curlabel->style = TEXTBOX;
					TDCOPY(curlabel->descript, ni->textdescript);
					curCell->label = io_pscolor_insertlabel(curlabel, curCell->label);			
				}
			} else
			{
				/* expanded instance: make the invocation */
				subCell = (CELLL *)np->temp1;
				curInst = (CELLINSTL *)emalloc(sizeof(CELLINSTL), io_tool->cluster);
				curInst->next = NULL;
				curInst->inst = subCell;
				io_pscolor_newIdentityMatrix(curInst->transform);

				/* account for instance position */
				io_pscolor_newIdentityMatrix(transt);
				transt[2*3+0] -= (float)((np->lowx + np->highx) / 2);
				transt[2*3+1] -= (float)((np->lowy + np->highy) / 2);
				io_pscolor_matrixMul(curInst->transform, curInst->transform, transt);

				/* account for instance rotation */
				io_pscolor_newIdentityMatrix(transr);
				float rotation;
				rotation = (float)(ni->rotation / 1800.0 * EPI);
				transr[0*3+0] = (float)cos(rotation); if (fabs(transr[0]) < SMALL_NUM) transr[0] = 0;
				transr[0*3+1] = (float)sin(rotation); if (fabs(transr[1]) < SMALL_NUM) transr[1] = 0;
				transr[1*3+0] = -(float)sin(rotation); if (fabs(transr[3]) < SMALL_NUM) transr[3] = 0;
				transr[1*3+1] = (float)cos(rotation); if (fabs(transr[4]) < SMALL_NUM) transr[4] = 0;
				
				io_pscolor_matrixMul(curInst->transform, curInst->transform, transr);

				/* account for instance transposition */
				if (ni->transpose != 0)
				{
					io_pscolor_newIdentityMatrix(transr);
					transr[0*3+0] = 0;
					transr[1*3+1] = 0;
					transr[0*3+1] = -1;
					transr[1*3+0] = -1;
					io_pscolor_matrixMul(curInst->transform, curInst->transform, transr);
				}

				/* account for instance location */
				io_pscolor_newIdentityMatrix(transt);
				transt[2*3+0] = (float)((ni->lowx + ni->highx) / 2);
				transt[2*3+1] = (float)((ni->lowy + ni->highy) / 2);
				io_pscolor_matrixMul(curInst->transform, curInst->transform, transt);
				curCell->inst = io_pscolor_insertInst(curInst, curCell->inst);
			}
		} else
		{
			/* primitive: generate its layers */
			makerot(ni, trans);
			tot = nodepolys(ni, 0, NOWINDOWPART);
			for(i=0; i<tot; i++)
			{
				shapenodepoly(ni, i, poly);
				xformpoly(poly, trans);
				io_pscolor_plotPolygon(poly, curCell);
			}
		}
	}

	/* add geometry for all arcs */
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		tot = arcpolys(ai, NOWINDOWPART);
		for(i=0; i<tot; i++)
		{
			shapearcpoly(ai, i, poly);
			io_pscolor_plotPolygon(poly, curCell);
		}
	}

	/* add the name of all exports */
	for(pp = cell->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		curlabel = (LABELL *)emalloc(sizeof(LABELL), io_tool->cluster);
		(void)allocstring(&curlabel->label, pp->protoname, io_tool->cluster);
		portposition(pp->subnodeinst, pp->subportproto, &xp, &yp);
		curlabel->pos[0] = curlabel->pos[1] = xp;
		curlabel->pos[2] = curlabel->pos[3] = yp;
		curlabel->style = TEXTCENT;
		TDCOPY(curlabel->descript, pp->textdescript);
		curCell->label = io_pscolor_insertlabel(curlabel, curCell->label);			
	}

	/* add the completed cell to the list */
	io_pscolor_cells = io_pscolor_insertCell(curCell, io_pscolor_cells);
}

void io_pscolor_plotPolygon(POLYGON *poly, CELLL *curCell)
{
	BOXL *curbox;
	POLYL *curpoly;
	LABELL *curlabel;
	REGISTER INTBIG j, k, type;
	INTBIG lx, hx, ly, hy, x, y;
	REGISTER BOOLEAN isabox;

	if (poly->tech == NOTECHNOLOGY) return;
	io_pscolor_getLayerMap(poly->tech);
	for(j=0; j<io_pscolor_numLayers; j++)
		if (io_pscolor_layers[j].layernumber == poly->layer &&
			io_pscolor_layers[j].tech == poly->tech) break;
	if (j >= io_pscolor_numLayers)
		return;
	isabox = isbox(poly, &lx, &hx, &ly, &hy);
	if (!isabox) getbbox(poly, &lx, &hx, &ly, &hy);

	switch (poly->style)
	{
		case FILLED:
		case FILLEDRECT:
			if (isabox)
			{
				curbox = (BOXL *)emalloc(sizeof(BOXL), io_tool->cluster);
				curbox->layer = (INTSML)j;
				curbox->next = NULL;
				curbox->visible = 1;
				curbox->pos[0] = hx-lx;
				curbox->pos[1] = hy-ly;
				curbox->pos[2] = (lx+hx)/2;
				curbox->pos[3] = (ly+hy)/2;
				curbox->pos[2] -= curbox->pos[0]/2; /* adjust center x to left edge; */
				curbox->pos[3] -= curbox->pos[1]/2; /* adjust center y to bottom edge */
				curCell->box = io_pscolor_insertbox(curbox, curCell->box);
			} else
			{
				curpoly = (POLYL *)emalloc(sizeof(POLYL), io_tool->cluster);
				if (curpoly == 0) return;
				curpoly->layer = (INTSML)j;
				curpoly->next = NULL;
				curpoly->numcoords = poly->count*2;
				curpoly->coords = (INTBIG *)emalloc(curpoly->numcoords * SIZEOFINTBIG, io_tool->cluster);
				if (curpoly->coords == 0) return;
				for(j=0; j<curpoly->numcoords; j++)
				{
					if ((j%2) == 0) curpoly->coords[j] = poly->xv[j/2]; else
						curpoly->coords[j] = poly->yv[j/2];
				}
				curCell->poly = io_pscolor_insertpoly(curpoly, curCell->poly);			
			}
			break;

		case CLOSED:
		case CLOSEDRECT:
			if (isabox)
			{
				io_pscolor_plotline(j, lx, ly, lx, hy, 0, curCell);
				io_pscolor_plotline(j, lx, hy, hx, hy, 0, curCell);
				io_pscolor_plotline(j, hx, hy, hx, ly, 0, curCell);
				io_pscolor_plotline(j, hx, ly, lx, ly, 0, curCell);
				break;
			}
			/* FALLTHROUGH */

		case OPENED:
		case OPENEDT1:
		case OPENEDT2:
		case OPENEDT3:
			switch (poly->style)
			{
				case OPENEDT1: type = 1; break;
				case OPENEDT2: type = 2; break;
				case OPENEDT3: type = 3; break;
				default:       type = 0; break;
			}
			for (k = 1; k < poly->count; k++)
				io_pscolor_plotline(j, poly->xv[k-1], poly->yv[k-1], poly->xv[k], poly->yv[k], type, curCell);
			if (poly->style == CLOSED)
			{
				k = poly->count - 1;
				io_pscolor_plotline(j, poly->xv[k], poly->yv[k], poly->xv[0], poly->yv[0], type, curCell);
			}
			break;

		case VECTORS:
			for(k=0; k<poly->count; k += 2)
				io_pscolor_plotline(j, poly->xv[k], poly->yv[k], poly->xv[k+1], poly->yv[k+1], 0, curCell);
			break;

		case CROSS:
		case BIGCROSS:
			getcenter(poly, &x, &y);
			io_pscolor_plotline(j, x-5, y, x+5, y, 0, curCell);
			io_pscolor_plotline(j, x, y+5, x, y-5, 0, curCell);
			break;

		case CROSSED:
			io_pscolor_plotline(j, lx, ly, lx, hy, 0, curCell);
			io_pscolor_plotline(j, lx, hy, hx, hy, 0, curCell);
			io_pscolor_plotline(j, hx, hy, hx, ly, 0, curCell);
			io_pscolor_plotline(j, hx, ly, lx, ly, 0, curCell);
			io_pscolor_plotline(j, hx, hy, lx, ly, 0, curCell);
			io_pscolor_plotline(j, hx, ly, lx, hy, 0, curCell);
			break;

		case DISC:
		case CIRCLE:
		case THICKCIRCLE:
		case CIRCLEARC:
		case THICKCIRCLEARC:
			if (!io_pscolor_curvewarning)
				ttyputmsg(_("Warning: the 'merged color' PostScript option ignores curves.  Use other color options"));
			io_pscolor_curvewarning = TRUE;
			break;

		case TEXTCENT:
		case TEXTTOP:
		case TEXTBOT:
		case TEXTLEFT:
		case TEXTRIGHT:
		case TEXTTOPLEFT:
		case TEXTBOTLEFT:
		case TEXTTOPRIGHT:
		case TEXTBOTRIGHT:
		case TEXTBOX:
			curlabel = (LABELL *)emalloc(sizeof(LABELL), io_tool->cluster);
			(void)allocstring(&curlabel->label, poly->string, io_tool->cluster);
			curlabel->pos[0] = lx;
			curlabel->pos[1] = hx;
			curlabel->pos[2] = ly;
			curlabel->pos[3] = hy;
			curlabel->style = poly->style;
			TDCOPY(curlabel->descript, poly->textdescript);
			curCell->label = io_pscolor_insertlabel(curlabel, curCell->label);			
			break;
	}
}

/*
 * Routine to add a line from (fx,fy) to (tx,ty) on layer "layer" to the cell "curCell".
 */
void io_pscolor_plotline(INTBIG layer, INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty, INTBIG texture, CELLL *curCell)
{
	POLYL *curpoly;

	curpoly = (POLYL *)emalloc(sizeof(POLYL), io_tool->cluster);
	if (curpoly == 0) return;
	curpoly->layer = (INTSML)layer;
	curpoly->next = NULL;
	curpoly->numcoords = 4;
	curpoly->coords = (INTBIG *)emalloc(4 * SIZEOFINTBIG, io_tool->cluster);
	if (curpoly->coords == 0) return;
	curpoly->coords[0] = fx;
	curpoly->coords[1] = fy;
	curpoly->coords[2] = tx;
	curpoly->coords[3] = ty;
	curCell->poly = io_pscolor_insertpoly(curpoly, curCell->poly);
}

void io_pscolor_genOverlapShapesAfterFlattening(void)
{
	BOXQUADTREE *q1, *q2;
	INTBIG i;

	/* traverse the list of boxes and create new layers where overlaps occur */
	ttyputmsg(_("Generating overlap after flattening for %ld layers..."), io_pscolor_numLayers);
	for (i=0; i<io_pscolor_numLayers; i++) {
		if (io_pscolor_layers[i].mix1 != -1 && io_pscolor_layers[i].mix2 != -1) {
			q1 = io_pscolor_makeBoxQuadTree(io_pscolor_layers[i].mix1);
			q2 = io_pscolor_makeBoxQuadTree(io_pscolor_layers[i].mix2);
			io_pscolor_coaf1(q1, q2, i);
		}
	}
}

BOXQUADTREE *io_pscolor_makeBoxQuadTree(INTBIG layer)
{
	BOXL *g;
	INTBIG i, numBoxes = 0;

	/* return if the quadtree is already generated */
	if (io_pscolor_boxQuadTrees[layer] != NULL) return io_pscolor_boxQuadTrees[layer];

	/* otherwise find number of boxes on this layer */
	g = io_pscolor_flattenedbox[layer];
	while (g != NULL) {
		if (g->visible) numBoxes++;
		g = g->next;
	}

	/* and convert the list into a quad tree for faster processing */

	/* first allocate the quad tree */
	io_pscolor_boxQuadTrees[layer] = (BOXQUADTREE *)emalloc(sizeof(BOXQUADTREE), io_tool->cluster);
	io_pscolor_boxQuadTrees[layer]->bounds[0] = io_pscolor_cifBoundaries[0];
	io_pscolor_boxQuadTrees[layer]->bounds[1] = io_pscolor_cifBoundaries[1];
	io_pscolor_boxQuadTrees[layer]->bounds[2] = io_pscolor_cifBoundaries[2];
	io_pscolor_boxQuadTrees[layer]->bounds[3] = io_pscolor_cifBoundaries[3];
	io_pscolor_boxQuadTrees[layer]->tl = NULL;
	io_pscolor_boxQuadTrees[layer]->tr = NULL;
	io_pscolor_boxQuadTrees[layer]->bl = NULL;
	io_pscolor_boxQuadTrees[layer]->br = NULL;
	io_pscolor_boxQuadTrees[layer]->numBoxes = numBoxes;
	io_pscolor_boxQuadTrees[layer]->boxes = (BOXELEM *)emalloc(numBoxes*sizeof(BOXELEM), io_tool->cluster);
	io_pscolor_boxQuadTrees[layer]->parent = NULL;
	io_pscolor_boxQuadTrees[layer]->level = 0;

	/* then copy the boxes into the tree */
	g = io_pscolor_flattenedbox[layer];
	i = 0;
	while (g != NULL) {
		if (g->visible) {
			io_pscolor_boxQuadTrees[layer]->boxes[i].pos[0] = g->pos[0];
			io_pscolor_boxQuadTrees[layer]->boxes[i].pos[1] = g->pos[1];
			io_pscolor_boxQuadTrees[layer]->boxes[i].pos[2] = g->pos[2];
			io_pscolor_boxQuadTrees[layer]->boxes[i].pos[3] = g->pos[3];
			io_pscolor_boxQuadTrees[layer]->boxes[i].layer = g->layer;
			io_pscolor_boxQuadTrees[layer]->boxes[i].visible = 1;
			i++;
		}
		g = g->next;
	}

	/* if there are too many boxes in this layer of the tree, create subtrees */
	if (numBoxes > TREETHRESHOLD) 
		io_pscolor_recursivelyMakeBoxQuadTree(io_pscolor_boxQuadTrees[layer]);
	
	return io_pscolor_boxQuadTrees[layer];
}

void io_pscolor_recursivelyMakeBoxQuadTree(BOXQUADTREE *q)
{
	INTBIG i, numBoxes;

	q->tl = (BOXQUADTREE *)emalloc(sizeof(BOXQUADTREE), io_tool->cluster);
	q->tl->parent = q; q->tl->level = q->level*10+1;
	q->tr = (BOXQUADTREE *)emalloc(sizeof(BOXQUADTREE), io_tool->cluster);
	q->tr->parent = q; q->tr->level = q->level*10+2;
	q->bl = (BOXQUADTREE *)emalloc(sizeof(BOXQUADTREE), io_tool->cluster);
	q->bl->parent = q; q->bl->level = q->level*10+3;
	q->br = (BOXQUADTREE *)emalloc(sizeof(BOXQUADTREE), io_tool->cluster);
	q->br->parent = q; q->br->level = q->level*10+4;

	/* split boxes into subtrees where they fit */
	io_pscolor_splitTree(q->numBoxes, q->boxes, q->bl, q->bounds[0], q->bounds[1],
		(q->bounds[0]+q->bounds[2])/2, (q->bounds[1]+q->bounds[3])/2);
	io_pscolor_splitTree(q->numBoxes, q->boxes, q->br, (q->bounds[0]+q->bounds[2])/2, q->bounds[1],
		q->bounds[2], (q->bounds[1]+q->bounds[3])/2);
	io_pscolor_splitTree(q->numBoxes, q->boxes, q->tl, q->bounds[0], (q->bounds[1]+q->bounds[3])/2,
		(q->bounds[0]+q->bounds[2])/2, q->bounds[3]);
	io_pscolor_splitTree(q->numBoxes, q->boxes, q->tr, (q->bounds[0]+q->bounds[2])/2,
		(q->bounds[1]+q->bounds[3])/2, q->bounds[2], q->bounds[3]);

	/* and leave boxes that span the subtrees at the top level */
	numBoxes = 0;
	for (i=0; i<q->numBoxes; i++) {
		if (q->boxes[i].visible) {
			/* q->boxes[numBoxes] = q->boxes[i]; */
			q->boxes[numBoxes].layer = q->boxes[i].layer;
			q->boxes[numBoxes].visible = 1;
			q->boxes[numBoxes].pos[0] = q->boxes[i].pos[0];
			q->boxes[numBoxes].pos[1] = q->boxes[i].pos[1];
			q->boxes[numBoxes].pos[2] = q->boxes[i].pos[2];
			q->boxes[numBoxes].pos[3] = q->boxes[i].pos[3];
			numBoxes++;
		}
	}

	q->numBoxes = numBoxes;
}

void io_pscolor_splitTree(INTBIG numBoxes, BOXELEM *boxes, BOXQUADTREE *q,
						  INTBIG left, INTBIG bot, INTBIG right, INTBIG top)
{
	INTBIG i;
	INTBIG count = 0;

	/* count how many boxes are in subtree */
	for (i = 0; i<numBoxes; i++) {
		if (boxes[i].visible && boxes[i].pos[2] >= left && boxes[i].pos[3] >= bot && 
			(boxes[i].pos[2]+boxes[i].pos[0]) <= right && (boxes[i].pos[3]+boxes[i].pos[1]) <= top) count++;
	}
	/* and copy them into a new array for the subtree */
	q->boxes = (BOXELEM *)emalloc(count*sizeof(BOXELEM), io_tool->cluster);
	count = 0;
	for (i=0; i<numBoxes; i++) {
		if (boxes[i].visible && boxes[i].pos[2] >= left && boxes[i].pos[3] >= bot && 
			(boxes[i].pos[2]+boxes[i].pos[0]) <= right && (boxes[i].pos[3]+boxes[i].pos[1]) <= top) {
			/*q->boxes[count] = boxes[i]; */
			q->boxes[count].layer = boxes[i].layer;
			q->boxes[count].visible = 1;
			q->boxes[count].pos[0] = boxes[i].pos[0];
			q->boxes[count].pos[1] = boxes[i].pos[1];
			q->boxes[count].pos[2] = boxes[i].pos[2];
			q->boxes[count].pos[3] = boxes[i].pos[3];
			boxes[i].visible = 0; /* mark box for removal from upper level */
			count++;
		}
	}
		
	q->numBoxes = count;
	q->bounds[0] = left;
	q->bounds[1] = bot;
	q->bounds[2] = right;
	q->bounds[3] = top;
	q->tl = NULL;
	q->tr = NULL;
	q->bl = NULL;
	q->br = NULL;
	
	if (count > TREETHRESHOLD) {
		io_pscolor_recursivelyMakeBoxQuadTree(q);
	}
}

#if 0
void io_pscolor_printBoxes(void)
{
	CELLL *c;
	BOXL *b1;
	int i;

	c = io_pscolor_cells;
	while (c != NULL) {
		ttyputerr("cell\n");
		b1 = c->box;
		while (b1 != NULL) {
			ttyputerr("  box layer %d visible %d coords %d %d %d %d\n", b1->layer, b1->visible, b1->pos[0], b1->pos[1], b1->pos[2], b1->pos[3]);
			b1 = b1->next;
		}
		c = c->next;
	}
	for (i=0; i<MAXLAYERS; i++) {
		b1 = io_pscolor_flattenedbox[i];
		if (b1 != NULL) ttyputerr("BOX layer %d num %d \n", i, io_pscolor_layers[i].layernumber);
		while (b1 != NULL) {
			ttyputerr("  box layer %d visible %d coords %d %d %d %d\n", b1->layer, b1->visible, b1->pos[0], b1->pos[1], b1->pos[2], b1->pos[3]);
			b1 = b1->next;
		}
	}
}
#endif

void io_pscolor_mergeBoxes(void)
{
	CELLL *c;
	BOXL *b1, *b2;
	INTBIG changed;
	INTBIG numMerged = 0;

	ttyputmsg(_("Merging boxes for %ld cells..."), io_pscolor_numCells);
	c = io_pscolor_cells;
	while (c != NULL) {
		do {
			changed = 0;
			b1 = c->box;
			while (b1 != NULL) {
				b2 = b1->next;
				while (b2 != NULL) {
					if (b1->layer == b2->layer && b1->visible && b2->visible) 
						if (io_pscolor_mergeBoxPair(b1, b2)) changed = 1;
					b2 = b2->next;
				}
				b1 = b1->next;
			}
		} while (changed);
		c = c->next;
		numMerged++;
	}
}

INTBIG io_pscolor_mergeBoxPair(BOXL *b1, BOXL *b2)
{
	INTBIG t[3], b[3], l[3], r[3];

	t[0] = b1->pos[3] + b1->pos[1];
	b[0] = b1->pos[3];
	l[0] = b1->pos[2];
	r[0] = b1->pos[2] + b1->pos[0];
	t[1] = b2->pos[3] + b2->pos[1];
	b[1] = b2->pos[3];
	l[1] = b2->pos[2];
	r[1] = b2->pos[2] + b2->pos[0];

	/* if the boxes coincide, expand the first one and hide the second one */

	if (t[0] == t[1] && b[0] == b[1] && (min(r[0], r[1]) > max(l[0], l[1]))) {
		l[2] = min(l[0], l[1]);
		r[2] = max(r[0], r[1]);
		b1->pos[0] = (r[2]-l[2]);
		b1->pos[1] = (t[0]-b[0]);
		b1->pos[2] = (l[2]);
		b1->pos[3] = (b[0]);
		b2->visible = 0;
		return 1;
	}
	else if (r[0] == r[1] && l[0] == l[1] && (min(t[0], t[1]) > max(b[0], b[1]))) {
		b[2] = min(b[0], b[1]);
		t[2] = max(t[0], t[1]);
		b1->pos[0] = (r[0]-l[0]);
		b1->pos[1] = (t[2]-b[2]);
		b1->pos[2] = (l[0]);
		b1->pos[3] = (b[2]);
		b2->visible = 0;
		return 1;
	}
	/* if one completely covers another, hide the covered box */
	else if (r[0] >= r[1] && l[0] <= l[1] && t[0] >= t[1] && b[0] <= b[1]) {
		b2->visible = 0;
		return 1;
	}
	else if (r[1] >= r[0] && l[1] <= l[0] && t[1] >= t[0] && b[1] <= b[0]) {
		b1->visible = 0;
		return 1;
	}
	return 0;
}

#if 0
void io_pscolor_checkOverlap(CELLL *c, BOXL *b1, BOXL *b2, INTBIG layerNum)
{
	INTBIG t[3], b[3], l[3], r[3];
	BOXL *curbox;

	t[0] = b1->pos[3] + b1->pos[1];
	b[0] = b1->pos[3];
	l[0] = b1->pos[2];
	r[0] = b1->pos[2] + b1->pos[0];
	t[1] = b2->pos[3] + b2->pos[1];
	b[1] = b2->pos[3];
	l[1] = b2->pos[2];
	r[1] = b2->pos[2] + b2->pos[0];

	t[2] = t[0] < t[1] ? t[0] : t[1];
	b[2] = b[0] > b[1] ? b[0] : b[1];
	l[2] = l[0] > l[1] ? l[0] : l[1];
	r[2] = r[0] < r[1] ? r[0] : r[1];

	if (t[2] > b[2] && r[2] > l[2]) {
		/* create overlap layer */
		curbox = (BOXL *)emalloc(sizeof(BOXL), io_tool->cluster);
		curbox->layer = (INTSML)layerNum;
		curbox->next = NULL;
		curbox->pos[0] = (r[2]-l[2]);
		curbox->pos[1] = (t[2]-b[2]);
		curbox->pos[2] = (l[2]);
		curbox->pos[3] = (b[2]);
		curbox->visible = 1;
		if (t[2] == t[0] && b[2] == b[0] && l[2] == l[0] && r[2] == r[0]) 
			b1->visible = 0;
		if (t[2] == t[1] && b[2] == b[1] && l[2] == l[1] && r[2] == r[1]) 
			b2->visible = 0;
		c->box = io_pscolor_insertbox(curbox, c->box);
	}
}
#endif

void io_pscolor_coaf1(BOXQUADTREE *q1, BOXQUADTREE *q2, INTBIG layernum)
{
	io_pscolor_coaf2(q1, q2, layernum);
	if (q1->tl != NULL) {
		io_pscolor_coaf3(q1->tl, q2, layernum);
		io_pscolor_coaf3(q1->tr, q2, layernum);
		io_pscolor_coaf3(q1->bl, q2, layernum);
		io_pscolor_coaf3(q1->br, q2, layernum);
		if (q2->tl != NULL) {
			io_pscolor_coaf1(q1->tl, q2->tl, layernum);
			io_pscolor_coaf1(q1->tr, q2->tr, layernum);
			io_pscolor_coaf1(q1->bl, q2->bl, layernum);
			io_pscolor_coaf1(q1->br, q2->br, layernum);
		}
		else {
			io_pscolor_coaf4(q1->tl, q2, layernum, 0);
			io_pscolor_coaf4(q1->tr, q2, layernum, 0);
			io_pscolor_coaf4(q1->bl, q2, layernum, 0);
			io_pscolor_coaf4(q1->br, q2, layernum, 0);
		}
	}
}

void io_pscolor_coaf2(BOXQUADTREE *q1, BOXQUADTREE *q2, INTBIG layernum)
{
	io_pscolor_checkOverlapAfterFlattening(q1, q2, layernum);
	if (q2->tl != NULL) {
		io_pscolor_coaf2(q1, q2->tl, layernum);
		io_pscolor_coaf2(q1, q2->tr, layernum);
		io_pscolor_coaf2(q1, q2->bl, layernum);
		io_pscolor_coaf2(q1, q2->br, layernum);
	}
}

void io_pscolor_coaf3(BOXQUADTREE *q1, BOXQUADTREE *q2, INTBIG layernum)
{
	io_pscolor_checkOverlapAfterFlattening(q1, q2, layernum);
	if (q2->parent != NULL) 
		io_pscolor_coaf3(q1, q2->parent, layernum);
}

void io_pscolor_coaf4(BOXQUADTREE *q1, BOXQUADTREE *q2, INTBIG layernum, INTBIG check)
{
	if (check) {
		io_pscolor_coaf3(q1, q2, layernum);
	}
	if (q1->tl != NULL) {
		io_pscolor_coaf4(q1->tl, q2, layernum, 1);
		io_pscolor_coaf4(q1->tr, q2, layernum, 1);
		io_pscolor_coaf4(q1->bl, q2, layernum, 1);
		io_pscolor_coaf4(q1->br, q2, layernum, 1);
	}
}

void io_pscolor_checkOverlapAfterFlattening(BOXQUADTREE *q1, BOXQUADTREE *q2, INTBIG layerNum)
{
	INTBIG j, k;
	INTBIG t[3], b[3], l[3], r[3];
	BOXL *curbox;

	/* check overlap of boxes at this level of the quad tree */
	if (q1->numBoxes && q2->numBoxes) {
		for (j=0; j<q1->numBoxes; j++) {
			t[0] = q1->boxes[j].pos[3] + q1->boxes[j].pos[1];
			b[0] = q1->boxes[j].pos[3];
			l[0] = q1->boxes[j].pos[2];
			r[0] = q1->boxes[j].pos[2] + q1->boxes[j].pos[0];

			for (k=0; k < q2->numBoxes; k++) {
				t[1] = q2->boxes[k].pos[3] + q2->boxes[k].pos[1];
				b[1] = q2->boxes[k].pos[3];
				l[1] = q2->boxes[k].pos[2];
				r[1] = q2->boxes[k].pos[2] + q2->boxes[k].pos[0];

				t[2] = t[0] < t[1] ? t[0] : t[1];
				b[2] = b[0] > b[1] ? b[0] : b[1];
				l[2] = l[0] > l[1] ? l[0] : l[1];
				r[2] = r[0] < r[1] ? r[0] : r[1];

				if (t[2] > b[2] && r[2] > l[2]) {
					/* create overlap layer */
					curbox = (BOXL *)emalloc(sizeof(BOXL), io_tool->cluster);
					curbox->layer = (INTSML)layerNum;
					curbox->next = NULL;
					curbox->pos[0] = (r[2]-l[2]);
					curbox->pos[1] = (t[2]-b[2]);
					curbox->pos[2] = (l[2]);
					curbox->pos[3] = (b[2]);
					curbox->visible = 1;
					if (t[2] == t[0] && b[2] == b[0] && l[2] == l[0] && r[2] == r[0]) q1->boxes[j].visible = 0;
					if (t[2] == t[1] && b[2] == b[1] && l[2] == l[1] && r[2] == r[1]) q2->boxes[k].visible = 0;
					io_pscolor_flattenedbox[layerNum] = io_pscolor_insertbox(curbox, io_pscolor_flattenedbox[layerNum]);
				}
			}
		}
	}
}

void io_pscolor_flatten(void)
{
	float ident[9];
	INTBIG i;
	
	io_pscolor_newIdentityMatrix(ident);
	for (i=0; i<io_pscolor_numLayers; i++) {
		io_pscolor_flattenedbox[i] = NULL;
	}

	/* for now, assume last cell is top level.  Change this to recognize C *** at top of CIF */

	ttyputmsg(_("Flattening..."));
	io_pscolor_recursiveFlatten(io_pscolor_cells, ident);
}

void io_pscolor_recursiveFlatten(CELLL *top, float m[9])
{
	CELLINSTL *inst;
	BOXL *box, *newbox;
	POLYL *poly, *newpoly;
	float tm[9];

	/* add boxes from this cell */
	box = top->box;
	while (box != NULL) {
		if (box->visible) {
			newbox = io_pscolor_copybox(box, m);
			io_pscolor_flattenedbox[newbox->layer] = io_pscolor_insertbox(newbox, io_pscolor_flattenedbox[newbox->layer]);
		}
		box = box->next;
	}

	/* add polygons from this cell */
	poly = top->poly;
	while (poly != NULL) {
		newpoly = io_pscolor_copypoly(poly, m);
		io_pscolor_flattenedpoly[newpoly->layer] = io_pscolor_insertpoly(newpoly, io_pscolor_flattenedpoly[newpoly->layer]);
		poly = poly->next;
	}

	/* recursively traverse subinstances */
	inst = top->inst;
	while (inst != NULL) {
		io_pscolor_numInstances++;
		io_pscolor_matrixMul(tm, inst->transform, m);
		io_pscolor_recursiveFlatten(inst->inst, tm);
		inst = inst->next;
	}
}

void io_pscolor_writePS(NODEPROTO *cell, BOOLEAN useplotter,
	INTBIG pageWidth, INTBIG pageHeight, INTBIG border)
{
	INTBIG i, j, pslx, pshx, psly, pshy, size, x, y, descenderoffset, rot, xoff, yoff,
		reducedwidth, reducedheight;
	CHAR *opname;
	POLYL *p;
	BOXL *g;
	LABELL *l;
	float scale;
	INTBIG w, h;
	time_t curdate;
	extern CHAR *io_psstringheader[];

	/* Header info */
	io_pswrite(x_("%%!PS-Adobe-1.0\n"));
	io_pswrite(x_("%%Title: %s\n"), describenodeproto(cell));
	if ((us_useroptions&NODATEORVERSION) == 0)
	{
		io_pswrite(x_("%%%%Creator: Electric VLSI Design System (David Harris's color PostScript generator) version %s\n"), el_version);
		curdate = getcurrenttime();
		io_pswrite(x_("%%%%CreationDate: %s\n"), timetostring(curdate));
	} else
	{
		io_pswrite(x_("%%%%Creator: Electric VLSI Design System (David Harris's color PostScript generator)\n"));
	}
	io_pswrite(x_("%%%%Pages: 1\n"));
	io_pswrite(x_("%%%%BoundingBox: %ld %ld %ld %ld\n"), io_pscolor_cifBoundaries[0],
		io_pscolor_cifBoundaries[1], io_pscolor_cifBoundaries[2], io_pscolor_cifBoundaries[3]);
	io_pswrite(x_("%%%%DocumentFonts: %s\n"), io_pscolor_font);
	io_pswrite(x_("%%%%EndComments\n"));

	/* Coordinate system */
	io_pswrite(x_("%% Min X: %d  min Y: %d  max X: %d  max Y: %d\n"), io_pscolor_cifBoundaries[0],
		io_pscolor_cifBoundaries[1], io_pscolor_cifBoundaries[2], io_pscolor_cifBoundaries[3]);
	reducedwidth = pageWidth - border*2;
	reducedheight = pageHeight - border*2;
	scale = reducedwidth/(float)(io_pscolor_cifBoundaries[2]-io_pscolor_cifBoundaries[0]);
	if (useplotter)
	{
		/* plotter: infinite height */
		io_pswrite(x_("%f %f scale\n"), scale, scale);
		x = io_pscolor_cifBoundaries[0]+border - ((INTBIG)(pageWidth/scale) -
			(io_pscolor_cifBoundaries[2]-io_pscolor_cifBoundaries[0])) / 2;
		y = io_pscolor_cifBoundaries[1]+border;
		io_pswrite(x_("%d neg %d neg translate\n"), x, y);
	} else
	{
		/* printer: fixed height */
		if (reducedheight/(float)(io_pscolor_cifBoundaries[3]-io_pscolor_cifBoundaries[1]) < scale)
			scale = reducedheight/(float)(io_pscolor_cifBoundaries[3]-io_pscolor_cifBoundaries[1]); 
		io_pswrite(x_("%f %f scale\n"), scale, scale);
		x = io_pscolor_cifBoundaries[0]+border - ((INTBIG)(pageWidth/scale) -
			(io_pscolor_cifBoundaries[2]-io_pscolor_cifBoundaries[0])) / 2;
		y = io_pscolor_cifBoundaries[1]+border - ((INTBIG)(pageHeight/scale) -
			(io_pscolor_cifBoundaries[3]-io_pscolor_cifBoundaries[1])) / 2;
		io_pswrite(x_("%d neg %d neg translate\n"), x, y);
	}

	/* set font */
	io_pswrite(x_("/DefaultFont /%s def\n"), io_pscolor_font);
	io_pswrite(x_("/scaleFont {\n"));
	io_pswrite(x_("    DefaultFont findfont\n"));
	io_pswrite(x_("    exch scalefont setfont} def\n"));

	/* define box command to make rectangles more memory efficient */
	io_pswrite(x_("\n/bx \n  { /h exch def /w exch def /x exch def /y exch def\n"));
	io_pswrite(x_("    newpath x y moveto w 0 rlineto 0 h rlineto w neg 0 rlineto closepath fill } def\n")); 

	/* draw layers */
	for (i=0; i<io_pscolor_numLayers; i++)
	{
		/* skip drawing layers that are white */
		if (io_pscolor_layers[i].r != 1 || io_pscolor_layers[i].g != 1 || io_pscolor_layers[i].b != 1)
		{
			g = io_pscolor_flattenedbox[i];
			p = io_pscolor_flattenedpoly[i];
			if (g != NULL || p != NULL)
			{
				io_pswrite(x_("\n%% Layer %s:%d"), io_pscolor_layers[i].tech->techname,
					io_pscolor_layers[i].layernumber);
				if (io_pscolor_layers[i].mix1 != -1)
					io_pswrite(x_(" mix of %d and %d"), io_pscolor_layers[io_pscolor_layers[i].mix1].layernumber,
						io_pscolor_layers[io_pscolor_layers[i].mix2].layernumber);
				io_pswrite(x_("\n%f %f %f setrgbcolor\n"), io_pscolor_layers[i].r,
					io_pscolor_layers[i].g, io_pscolor_layers[i].b);
			}
			while (g != NULL)
			{
				if (g->visible)
				{
					w = g->pos[0];
					h = g->pos[1];
					io_pswrite(x_("%d %d %d %d bx\n"),	g->pos[3], g->pos[2], w, h);	  
					io_pscolor_numBoxes++;
				}
				g = g->next;
				io_pscolor_totalBoxes++;
			}
			while (p != NULL)
			{
				if (p->numcoords > 2)
				{
					io_pswrite(x_("newpath %d %d moveto\n"), p->coords[0], p->coords[1]);
					for (j=2; j<p->numcoords; j+=2) {
						io_pswrite(x_("        %d %d lineto\n"), p->coords[j], p->coords[j+1]);
					}
					io_pswrite(x_("closepath %s\n"), i==0 ? x_("stroke") : x_("fill"));
					io_pscolor_numPolys++;
				}
				p = p->next;
			}
/*			LINEL *h = io_pscolor_horizsegs[i];
			io_pswrite(x_("\n0 0 0 setrgbcolor\n newpath\n"));
			while (h != NULL) {
				io_pswrite(x_("%d %d moveto %d %d lineto\n"), h->x[0], h->y, h->x[1], h->y);
				h = h->next;
			}
			io_pswrite(x_("closepath stroke\n"));
*/		
		}
	}

	/* label ports and cell instances */
	l = io_pscolor_cells->label;
	io_pswrite(x_("\n%% Port and Cell Instance Labels\n"));
	io_pswrite(x_("0 0 0 setrgbcolor\n"));
	for(i=0; io_psstringheader[i] != 0; i++)
		io_pswrite(x_("%s\n"), io_psstringheader[i]);
	while (l != NULL)
	{
		size = io_pscolor_truefontsize(TDGETSIZE(l->descript), el_curtech);
		pslx = l->pos[0];   pshx = l->pos[1];
		psly = l->pos[2];   pshy = l->pos[3];
		if (l->style == TEXTBOX)
		{
			io_pswrite(x_("%ld %ld %ld %ld "), (pslx+pshx)/2, (psly+pshy)/2, pshx-pslx, pshy-psly);
			io_pswritestring(l->label);
			io_pswrite(x_(" %f Boxstring\n"), size/scale);
		} else
		{
			switch (l->style)
			{
				case TEXTCENT:
					x = (pslx+pshx)/2;   y = (psly+pshy)/2;
					opname = x_("Centerstring");
					break;
				case TEXTTOP:
					x = (pslx+pshx)/2;   y = pshy;
					opname = x_("Topstring");
					break;
				case TEXTBOT:
					x = (pslx+pshx)/2;   y = psly;
					opname = x_("Botstring");
					break;
				case TEXTLEFT:
					x = pslx;   y = (psly+pshy)/2;
					opname = x_("Leftstring");
					break;
				case TEXTRIGHT:
					x = pshx;   y = (psly+pshy)/2;
					opname = x_("Rightstring");
					break;
				case TEXTTOPLEFT:
					x = pslx;   y = pshy;
					opname = x_("Topleftstring");
					break;
				case TEXTTOPRIGHT:
					x = pshx;   y = pshy;
					opname = x_("Toprightstring");
					break;
				case TEXTBOTLEFT:
					x = pslx;   y = psly;
					opname = x_("Botleftstring");
					break;
				case TEXTBOTRIGHT:
					x = pshx;   y = psly;
					opname = x_("Botrightstring");
					break;
			}
			descenderoffset = size / 12;
			rot = TDGETROTATION(l->descript);
			switch (rot)
			{
				case 0: y += descenderoffset;   break;
				case 1: x -= descenderoffset;   break;
				case 2: y -= descenderoffset;   break;
				case 3: x += descenderoffset;   break;
			}
			if (rot != 0)
			{
				if (rot == 1 || rot == 3)
				{
					switch (l->style)
					{
						case TEXTTOP:      opname = x_("Rightstring");     break;
						case TEXTBOT:      opname = x_("Leftstring");      break;
						case TEXTLEFT:     opname = x_("Botstring");       break;
						case TEXTRIGHT:    opname = x_("Topstring");       break;
						case TEXTTOPLEFT:  opname = x_("Botrightstring");  break;
						case TEXTBOTRIGHT: opname = x_("Topleftstring");   break;
					}
				}
				xoff = x;   yoff = y;
				x = y = 0;
				switch (rot)
				{
					case 1:		/* 90 degrees counterclockwise */
						io_pswrite(x_("%ld %ld translate 90 rotate\n"), xoff, yoff);
						break;
					case 2:		/* 180 degrees */
						io_pswrite(x_("%ld %ld translate 180 rotate\n"), xoff, yoff);
						break;
					case 3:		/* 90 degrees clockwise */
						io_pswrite(x_("%ld %ld translate 270 rotate\n"), xoff, yoff);
						break;
				}
			}
			io_pswrite(x_("%ld %ld "), x, y);
			io_pswritestring(l->label);
			io_pswrite(x_(" %ld %s\n"), size, opname);
			if (rot != 0)
			{
				switch (rot)
				{
					case 1:		/* 90 degrees counterclockwise */
						io_pswrite(x_("270 rotate %ld %ld translate\n"), -xoff, -yoff);
						break;
					case 2:		/* 180 degrees */
						io_pswrite(x_("180 rotate %ld %ld translate\n"), -xoff, -yoff);
						break;
					case 3:		/* 90 degrees clockwise */
						io_pswrite(x_("90 rotate %ld %ld translate\n"), -xoff, -yoff);
						break;
				}
			}
		}
		l = l->next;
	}
	
	/* Finish page */
	io_pswrite(x_("\nshowpage\n"));
}

INTBIG io_pscolor_truefontsize(INTBIG font, TECHNOLOGY *tech)
{
	REGISTER INTBIG  lambda, height;

	/* absolute font sizes are easy */
	if ((font&TXTPOINTS) != 0) return((font&TXTPOINTS) >> TXTPOINTSSH);

	/* detemine default, min, and max size of font */
	lambda = el_curlib->lambda[tech->techindex];
	height = TXTGETQLAMBDA(font);
	height = height * lambda / 4;
	return(height);
}

BOXL *io_pscolor_copybox(BOXL *g, float m[9])
{
	BOXL *newbox;
	INTBIG dx, dy;

	newbox = (BOXL *)emalloc(sizeof(BOXL), io_tool->cluster);
	newbox->layer = g->layer;
	newbox->next = NULL;
	io_pscolor_transformBox(newbox->pos, g->pos, m);
	newbox->visible = g->visible;

	/* Update bounding box */
	dx = newbox->pos[0];
	dy = newbox->pos[1];
	if (newbox->pos[2] < io_pscolor_cifBoundaries[0]) io_pscolor_cifBoundaries[0] = newbox->pos[2];
	if (newbox->pos[3] < io_pscolor_cifBoundaries[1]) io_pscolor_cifBoundaries[1] = newbox->pos[3];
	if (newbox->pos[2]+dx > io_pscolor_cifBoundaries[2]) io_pscolor_cifBoundaries[2] = newbox->pos[2]+dx;
	if (newbox->pos[3]+dy > io_pscolor_cifBoundaries[3]) io_pscolor_cifBoundaries[3] = newbox->pos[3]+dy;
	return newbox;
}

POLYL *io_pscolor_copypoly(POLYL *p, float m[9])
{
	POLYL *newpoly;
	INTBIG i;

	newpoly = (POLYL *)emalloc(sizeof(POLYL), io_tool->cluster);
	if (newpoly == 0) return(0);
	newpoly->layer = p->layer;
	newpoly->next = NULL;
	newpoly->numcoords = p->numcoords;
	newpoly->coords = (INTBIG *)emalloc(newpoly->numcoords * SIZEOFINTBIG, io_tool->cluster);
	if (newpoly->coords == 0) return(0);
	io_pscolor_transformPoly(newpoly, p, m);

	/* Update bounding box */
	for(i=0; i<newpoly->numcoords; i++)
	{
		if ((i%2) == 0)
		{
			if (newpoly->coords[i] < io_pscolor_cifBoundaries[0]) io_pscolor_cifBoundaries[0] = newpoly->coords[i];
			if (newpoly->coords[i] > io_pscolor_cifBoundaries[3]) io_pscolor_cifBoundaries[3] = newpoly->coords[i];
		} else
		{
			if (newpoly->coords[i] < io_pscolor_cifBoundaries[1]) io_pscolor_cifBoundaries[1] = newpoly->coords[i];
			if (newpoly->coords[i] > io_pscolor_cifBoundaries[2]) io_pscolor_cifBoundaries[2] = newpoly->coords[i];
		}
	}
	return newpoly;
}

CELLL* io_pscolor_insertCell(CELLL *c, CELLL *list)
{
	c->next = list;
	list = c;
	return list;
}

BOXL* io_pscolor_insertbox(BOXL *g, BOXL *list)
{
	g->next = list;
	list = g;
	return list;
}

POLYL* io_pscolor_insertpoly(POLYL *p, POLYL *list)
{
	p->next = list;
	list = p;
	return list;
}

LABELL* io_pscolor_insertlabel(LABELL *p, LABELL *list)
{
	p->next = list;
	list = p;
	return list;
}

CELLINSTL* io_pscolor_insertInst(CELLINSTL *i, CELLINSTL *list)
{
	i->next = list;
	list = i;
	return list;
}

void io_pscolor_matrixMul(float r[9], float a[9], float b[9])
{
	INTBIG i, j, k;
	float tmp[9];

	for (i=0; i<3; i++) {
		for (j=0; j<3; j++) {
			tmp[i*3+j] = 0;
			for (k=0; k<3; k++) {
				tmp[i*3+j] += a[i*3+k] * b[k*3+j];
			}
		}
	}
	for (i=0; i<3; i++) {
		for (j=0; j<3; j++) {
			r[i*3+j] = tmp[i*3+j];
		}
	}
}

void io_pscolor_newIdentityMatrix(float m[9])
{
	INTBIG i,j;

	for (i=0; i<3; i++) {
		for (j=0; j<3; j++) {
			m[i*3+j] = (i==j);
		}
	}
}

void io_pscolor_transformBox(INTBIG final[4], INTBIG initial[4], float m[9])
{
	INTBIG i,j;
	INTBIG pos[2];

	pos[0] = initial[2]+initial[0]/2;
	pos[1] = initial[3]+initial[1]/2;

	for (i=0; i < 2; i++) {
		final[i+2] = (INTBIG)m[6+i];
		for (j=0; j<2; j++) {
			final[i+2] += (INTBIG)m[i+j*3]*pos[j];
		}
	}

	if (m[1] == 0) { /* rotation */
		final[0] = initial[0];
		final[1] = initial[1];
	}
	else {
		final[0] = initial[1];
		final[1] = initial[0];
	}
	final[2] -= final[0]/2;
	final[3] -= final[1]/2;
}

void io_pscolor_transformPoly(POLYL *final, POLYL *initial, float m[9])
{
	INTBIG p, i,j;
	
	for (p=0; p<initial->numcoords/2; p++) {
		for (i=0; i <2; i++) {
			final->coords[p*2+i] = (INTBIG)m[6+i];
			for (j=0; j<2; j++) {
				final->coords[p*2+i] += (INTBIG)m[i+j*3]*initial->coords[p*2+j];
			}
		}
	}
}

void io_pscolor_printStatistics(void)
{
	ttyputmsg("Plotting statistics:");
	ttyputmsg("  %ld layers defined or transparencies implied in layer map", io_pscolor_numLayers);
	ttyputmsg("  %ld cells", io_pscolor_numCells);
	ttyputmsg("  %ld instances used", io_pscolor_numInstances);
	ttyputmsg("  %ld boxes generated", io_pscolor_numBoxes);
	ttyputmsg("  %ld polygons generated", io_pscolor_numPolys);
}

void io_pscolor_freeMem(void)
{
	INTBIG i;

	io_pscolor_freeCell(io_pscolor_cells);
	for (i=0; i<io_pscolor_numLayers; i++) {
		io_pscolor_freeBox(io_pscolor_flattenedbox[i]);
		io_pscolor_flattenedbox[i] = NULL;
		io_pscolor_freePoly(io_pscolor_flattenedpoly[i]);
		io_pscolor_flattenedpoly[i] = NULL;
		io_pscolor_freeBoxQuadTree(io_pscolor_boxQuadTrees[i]);
		io_pscolor_boxQuadTrees[i] = NULL;
	}
	io_pscolor_cells = NULL;
}

void io_pscolor_freeBoxQuadTree(BOXQUADTREE *q)
{
	if (q != NULL) {
		io_pscolor_freeBoxQuadTree(q->tl);
		io_pscolor_freeBoxQuadTree(q->tr);
		io_pscolor_freeBoxQuadTree(q->bl);
		io_pscolor_freeBoxQuadTree(q->br);
		efree((CHAR *)q->boxes);
		efree((CHAR *)q);
	}
}

void io_pscolor_freeCell(CELLL *c)
{
	CELLL *c2;

	while (c != NULL) {
		io_pscolor_freeCellInst(c->inst);
		io_pscolor_freeBox(c->box);
		io_pscolor_freePoly(c->poly);
		io_pscolor_freeLabel(c->label);
		c2 = c;
		c = c->next;
		efree((CHAR *)c2);
	}
}

void io_pscolor_freeBox(BOXL *b) 
{
	BOXL *b2;

	while (b != NULL) {
		b2 = b;
		b = b->next;
		efree((CHAR *)b2);
	}
}

void io_pscolor_freePoly(POLYL *b) 
{
	POLYL *b2;

	while (b != NULL) {
		b2 = b;
		b = b->next;
		efree((CHAR *)b2->coords);
		efree((CHAR *)b2);
	}
}

void io_pscolor_freeLabel(LABELL *b)
{
	LABELL *b2;

	while (b != NULL) {
		b2 = b;
		b = b->next;
		efree((CHAR *)b2->label);
		efree((CHAR *)b2);
	}
}

void io_pscolor_freeCellInst(CELLINSTL *b)
{
	CELLINSTL *b2;

	while (b != NULL) {
		b2 = b;
		b = b->next;
		efree((CHAR *)b2);
	}
}

#if 0
void io_pscolor_freeLine(LINEL *b)
{
	LINEL *b2;

	while (b != NULL) {
		b2 = b;
		b = b->next;
		efree((CHAR *)b2);
	}
}
#endif

/********************************* OUTLINE HANDLING *********************************/

#if 0
void genOutlines(void)
{
	INTBIG i;
	LINEL *horiz, *vert;

	ttyputmsg("Generating layer outlines...");
	for (i=0; i<io_pscolor_numLayers; i++) {
		if (io_pscolor_layers[i].mix1 == -1 && io_pscolor_layers[i].mix2 == -1) {
			horiz = io_pscolor_genSegments(io_pscolor_flattenedbox[i], 0);
			vert = io_pscolor_genSegments(io_pscolor_flattenedbox[i], 1);
			/*eliminateSegments(horiz); */
			/*eliminateSegments(vert); */
			io_pscolor_flattenedoutline[i] = io_pscolor_combineSegments(horiz, vert);
			io_pscolor_horizsegs[i] = horiz;
		}
	}
}

LINEL *io_pscolor_genSegments(BOXL *boxes, INTBIG dir)
{
	INTBIG numBoxes;
	INTBIG i;
	BOXL *b;
	LINEL *lines;
	LINEL *segs, *s2;

	numBoxes = io_pscolor_countBoxes(boxes);
	lines = (LINEL *)emalloc(2*numBoxes*sizeof(LINEL), io_tool->cluster);
	for (i=0, b=boxes; i<numBoxes; i++) {
		lines[2*i].visible = 1;
		lines[2*i].x[0] = b->pos[2+dir];
		lines[2*i].x[1] = b->pos[2+dir] + b->pos[0+dir];
		lines[2*i].y = b->pos[3-dir];
		lines[2*i+1].visible = 1;
		lines[2*i+1].x[0] = b->pos[2+dir];
		lines[2*i+1].x[1] = b->pos[2+dir] + b->pos[0+dir];
		lines[2*i+1].y = b->pos[3-dir] + b->pos[1-dir];
		b = b->next;
	}
	qsort(lines, 2*numBoxes, sizeof(LINEL), io_pscolor_lineCompare);
/*	for (i=0; i<2*numBoxes; i++) */
/*		ttyputmsg("seg %d: %d-%d, %d", i, lines[i].x[0], lines[i].x[1], lines[i].y); */

	/* eliminate segments (not implemented yet) */
	/* this may be difficult because boxes from different cells may overlap. */
	/* at this point I give up and drop it.  dh 1/31/03 */
	segs = NULL;
	for (i=0; i<2*numBoxes; i++) {
		s2 = (LINEL *)emalloc(sizeof(LINEL), io_tool->cluster);
		s2->x[0] = lines[i].x[0];
		s2->x[1] = lines[i].x[1];
		s2->y = lines[i].y;
		s2->visible = lines[i].visible;
		s2->next = segs;
		segs = s2;
	}

	/* free array */
	efree((CHAR *)lines);
	
	return segs;
}

int io_pscolor_lineCompare(const void *a, const void *b)
{
	return (((LINEL*)a)->y - ((LINEL*)b)->y);
}

INTBIG io_pscolor_countBoxes(BOXL *boxes)
{
	INTBIG i;
	BOXL *b;

	b = boxes; i = 0;
	while (b != NULL) {
		i++;
		b = b->next;
	}

	return i;
}

OUTLINEL *io_pscolor_combineSegments(LINEL *horiz, LINEL *vert)
{
	return NULL;
}
#endif
