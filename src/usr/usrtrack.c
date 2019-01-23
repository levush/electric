/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrtrack.c
 * User interface tool: cursor tracking routines
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

/*
 * the code in this module makes me quiver with trepidation whenever it
 * breaks ... smr
 */
#include "global.h"
#include "egraphics.h"
#include "usr.h"
#include "usrtrack.h"
#include "efunction.h"
#include "tecart.h"
#include "tecgen.h"
#include "sim.h"

#define MAXTRACE 400
#define MINSLIDEDELAY 60			/* ticks between shifts of window when dragging to edge */
static INTBIG us_tracelist;
static INTBIG us_tracedata[MAXTRACE];
static GRAPHICS us_highl = {LAYERH, 0, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
	0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};

static INTBIG us_dragx, us_dragy, us_dragox, us_dragoy, us_dragoffx,
	us_dragoffy, us_lastcurx, us_lastcury, us_dragpoint,
	us_dragnodetotal, us_cantdrag, us_draglowval, us_draghighval, us_measureshown,
	us_firstmeasurex, us_firstmeasurey, us_dragshown, us_dragangle, us_dragstate,
	us_dragfport, us_dragextra, us_dragnobox, us_dragstayonhigh, us_dragstill,
	us_dragcorner, us_dragspecial;
static BOOLEAN     us_dragjoinfactor;
static BOOLEAN     us_multidragshowinvpin, us_multidragshowoffset;
static INTBIG      us_multidragmostzeros;
static UINTBIG     us_dragdescript[TEXTDESCRIPTSIZE];
static INTBIG      us_arrowamount;				/* for slider tracking */
static INTBIG      us_arrowlx, us_arrowhx, us_arrowly, us_arrowhy;
static WINDOWPART *us_dragwindow;
static POLYGON    *us_dragpoly = NOPOLYGON;
static NODEPROTO  *us_dragnodeproto;
static PORTPROTO  *us_dragportproto, *us_dragfromport;
static NODEINST   *us_dragnodeinst, **us_dragnodelist;
static ARCINST    *us_dragarcinst;
static GEOM       *us_dragobject, **us_dragobjectlist;
static GEOM       *us_dragfromgeom;
static INTBIG      us_dragaddr, us_dragtype;
static INTBIG      us_dragstartx, us_dragstarty;
static INTBIG      us_dragwirepathcount;
static INTBIG      us_dragwirepath[8];
static HIGHLIGHT   us_draghigh;
static CHAR        us_dragmessage[100];
static UINTBIG     us_beforepantime = 0;
static INTBIG      us_initthumbcoord;
static WINDOWPART  us_trackww;
static INTBIG      us_dragtextcount;
static CHAR      **us_dragtexts;
static void      (*us_trackingcallback)(INTBIG);

/* prototypes for local routines */
static void us_finddoibegin(BOOLEAN);
static void us_multidragdraw(INTBIG, INTBIG, INTBIG);
static void us_invertstretch(void);
static void us_wanttoinvert(INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty, WINDOWPART *w);
static void us_drawdistance(void);
static void us_panatscreenedge(INTBIG *x, INTBIG *y);
static void us_wanttoinvert(INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty, WINDOWPART *w);

/************************* NODE TRACE *************************/

/* initialization routine when reading a cursor trace */
void us_tracebegin(void)
{
	us_tracelist = 0;
	us_highl.col = HIGHLIT;
}

/* cursor advance routine when reading a cursor trace */
BOOLEAN us_tracedown(INTBIG x, INTBIG y)
{
	INTBIG fx, fy, tx, ty;

	if (us_tracelist >= MAXTRACE) return(TRUE);
	if (us_tracelist != 0)
	{
		fx = us_tracedata[us_tracelist-2];
		fy = us_tracedata[us_tracelist-1];
		tx = x;   ty = y;
		if ((el_curwindowpart->state&INPLACEEDIT) != 0)
		{
			xform(fx, fy, &fx, &fy, el_curwindowpart->outofcell);
			xform(tx, ty, &tx, &ty, el_curwindowpart->outofcell);
		}
		screendrawline(el_curwindowpart, fx, fy, tx, ty, &us_highl, 0);
	}
	us_tracedata[us_tracelist++] = x;
	us_tracedata[us_tracelist++] = y;
	return(FALSE);
}

/* termination routine when reading a cursor trace */
void us_traceup(void)
{
	REGISTER INTBIG i;
	INTBIG fx, fy, tx, ty;

	if (us_tracelist == 0)
	{
		(void)setval((INTBIG)us_tool, VTOOL, us_commandvarname('T'), (INTBIG)x_("nothing"),
			VSTRING|VDONTSAVE);
		return;
	}
	(void)setval((INTBIG)us_tool, VTOOL, us_commandvarname('T'), (INTBIG)us_tracedata,
		(INTBIG)(VINTEGER|VISARRAY|(us_tracelist<<VLENGTHSH)|VDONTSAVE));
	us_highl.col = 0;
	for(i=2; i<us_tracelist; i += 2)
	{
		fx = us_tracedata[us_tracelist-2];
		fy = us_tracedata[us_tracelist-1];
		tx = us_tracedata[us_tracelist];
		ty = us_tracedata[us_tracelist+1];
		if ((el_curwindowpart->state&INPLACEEDIT) != 0)
		{
			xform(fx, fy, &fx, &fy, el_curwindowpart->outofcell);
			xform(tx, ty, &tx, &ty, el_curwindowpart->outofcell);
		}
		screendrawline(el_curwindowpart, us_tracedata[i-2], us_tracedata[i-1],
			us_tracedata[i], us_tracedata[i+1], &us_highl, 0);
	}
}

/* preinitialization routine when creating or moving a point on a node */
void us_pointinit(NODEINST *node, INTBIG point)
{
	us_dragnodeinst = node;
	us_dragpoint = point;
}

/* initialization routine when creating or moving a point on a node */
void us_pointbegin(void)
{
	/* initialize polygon */
	if (us_dragpoly == NOPOLYGON) us_dragpoly = allocpolygon(4, us_tool->cluster);
	us_dragpoly->desc = &us_highl;

	us_dragshown = 0;
	(void)getxy(&us_dragx, &us_dragy);
	us_dragwindow = el_curwindowpart;
}

/* initialization routine when finding and moving a point on a node */
void us_findpointbegin(void)
{
	REGISTER VARIABLE *highvar;
	HIGHLIGHT newhigh;

	/* initialize polygon */
	if (us_dragpoly == NOPOLYGON) us_dragpoly = allocpolygon(4, us_tool->cluster);
	us_dragpoly->desc = &us_highl;

	us_dragshown = 0;
	if (getxy(&us_dragx, &us_dragy)) return;
	gridalign(&us_dragx, &us_dragy, 1, el_curlib->curnodeproto);
	us_setcursorpos(NOWINDOWFRAME, us_dragx, us_dragy);
	us_dragwindow = el_curwindowpart;

	/* select the current point on the node */
	highvar = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (highvar == NOVARIABLE) return;
	(void)us_makehighlight(((CHAR **)highvar->addr)[0], &newhigh);
	newhigh.fromport = NOPORTPROTO;
	newhigh.cell = getcurcell();
	us_setfind(&newhigh, 1, 0, 0, 0);

	/* get the point to be moved */
	highvar = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (highvar == NOVARIABLE) return;
	(void)us_makehighlight(((CHAR **)highvar->addr)[0], &newhigh);
	us_dragpoint = newhigh.frompoint;
}

/* cursor advance routine when creating a point on a node */
BOOLEAN us_addpdown(INTBIG x, INTBIG y)
{
	REGISTER INTBIG p, size;
	REGISTER INTBIG cx, cy;
	XARRAY trans;
	REGISTER VARIABLE *var;

	/* pan the screen if the edge was hit */
	us_panatscreenedge(&x, &y);

	/* grid align the cursor value */
	if (us_setxy(x, y) || us_dragwindow != el_curwindowpart)
	{
		(void)us_setxy(us_lastcurx, us_lastcury);
		return(FALSE);
	}
	(void)getxy(&us_lastcurx, &us_lastcury);
	gridalign(&us_lastcurx, &us_lastcury, 1, el_curlib->curnodeproto);
	us_setcursorpos(NOWINDOWFRAME, us_lastcurx, us_lastcury);

	/* if the lines are already being shown, erase them */
	if (us_dragshown != 0)
	{
		/* if it didn't actually move, go no further */
		if (us_dragx == us_lastcurx && us_dragy == us_lastcury) return(FALSE);

		/* undraw the box */
		us_highl.col = 0;
		us_showpoly(us_dragpoly, us_dragwindow);
	}

	makerot(us_dragnodeinst, trans);
	var = gettrace(us_dragnodeinst);
	if (var == NOVARIABLE)
	{
		us_dragpoly->xv[0] = us_lastcurx;
		us_dragpoly->yv[0] = us_lastcury;
		us_dragpoly->count = 1;
		us_dragpoly->style = CROSS;
	} else
	{
		size = getlength(var) / 2;
		cx = (us_dragnodeinst->lowx + us_dragnodeinst->highx) / 2;
		cy = (us_dragnodeinst->lowy + us_dragnodeinst->highy) / 2;
		p = 0;
		if (us_dragpoint > 0 && us_dragpoint <= size)
		{
			xform(((INTBIG *)var->addr)[(us_dragpoint-1)*2] + cx,
				((INTBIG *)var->addr)[(us_dragpoint-1)*2+1] + cy,
					&us_dragpoly->xv[p], &us_dragpoly->yv[p], trans);
			p++;
		}
		us_dragpoly->xv[p] = us_lastcurx;
		us_dragpoly->yv[p] = us_lastcury;
		p++;
		if (us_dragpoint < size)
		{
			xform(((INTBIG *)var->addr)[us_dragpoint*2] + cx,
				((INTBIG *)var->addr)[us_dragpoint*2+1] + cy,
					&us_dragpoly->xv[p], &us_dragpoly->yv[p], trans);
			p++;
		}
		us_dragpoly->count = p;
		us_dragpoly->style = OPENED;
	}

	us_dragx = us_lastcurx;
	us_dragy = us_lastcury;

	/* draw the new box */
	us_highl.col = HIGHLIT;
	us_showpoly(us_dragpoly, us_dragwindow);
	us_dragshown = 1;
	return(FALSE);
}

/* cursor advance routine when moving a point on a node */
BOOLEAN us_movepdown(INTBIG x, INTBIG y)
{
	REGISTER INTBIG p, size;
	REGISTER INTBIG cx, cy;
	XARRAY trans;
	REGISTER VARIABLE *var;

	/* pan the screen if the edge was hit */
	us_panatscreenedge(&x, &y);

	/* grid align the cursor value */
	if (us_setxy(x, y) || us_dragwindow != el_curwindowpart)
	{
		(void)us_setxy(us_lastcurx, us_lastcury);
		return(FALSE);
	}
	(void)getxy(&us_lastcurx, &us_lastcury);
	gridalign(&us_lastcurx, &us_lastcury, 1, el_curlib->curnodeproto);
	us_setcursorpos(NOWINDOWFRAME, us_lastcurx, us_lastcury);

	/* if the lines are already being shown, erase them */
	if (us_dragshown != 0)
	{
		/* if it didn't actually move, go no further */
		if (us_dragx == us_lastcurx && us_dragy == us_lastcury) return(FALSE);

		/* undraw the box */
		us_highl.col = 0;
		us_showpoly(us_dragpoly, us_dragwindow);
	}

	makerot(us_dragnodeinst, trans);
	var = gettrace(us_dragnodeinst);
	if (var == NOVARIABLE) return(FALSE);

	size = getlength(var) / 2;
	cx = (us_dragnodeinst->lowx + us_dragnodeinst->highx) / 2;
	cy = (us_dragnodeinst->lowy + us_dragnodeinst->highy) / 2;
	p = 0;
	if (us_dragpoint > 1)
	{
		xform(((INTBIG *)var->addr)[(us_dragpoint-2)*2] + cx,
			((INTBIG *)var->addr)[(us_dragpoint-2)*2+1] + cy,
				&us_dragpoly->xv[p], &us_dragpoly->yv[p], trans);
		p++;
	}
	us_dragpoly->xv[p] = us_lastcurx;
	us_dragpoly->yv[p] = us_lastcury;
	p++;
	if (us_dragpoint < size)
	{
		xform(((INTBIG *)var->addr)[us_dragpoint*2] + cx,
			((INTBIG *)var->addr)[us_dragpoint*2+1] + cy,
				&us_dragpoly->xv[p], &us_dragpoly->yv[p], trans);
		p++;
	}
	us_dragpoly->count = p;
	us_dragpoly->style = OPENED;

	us_dragx = us_lastcurx;
	us_dragy = us_lastcury;

	/* draw the new box */
	us_highl.col = HIGHLIT;
	us_showpoly(us_dragpoly, us_dragwindow);
	us_dragshown = 1;
	return(FALSE);
}

/************************* ARC CURVATURE *************************/

/* initialization routine when changing an arc's curvature */
void us_arccurveinit(ARCINST *ai)
{
	us_dragarcinst = ai;
	us_dragwindow = el_curwindowpart;
}

/*
 * Routine for changing the curvature of an arc so that it pass through (x,y)
 */
BOOLEAN us_arccurvedown(INTBIG x, INTBIG y)
{
	INTBIG xcur, ycur, r;

	/* grid align the cursor value */
	if (us_dragwindow != el_curwindowpart) return(FALSE);
	if (us_setxy(x, y)) return(FALSE);
	(void)getxy(&xcur, &ycur);

	/* get radius that lets arc curve through this point */
	r = us_curvearcthroughpoint(us_dragarcinst, xcur, ycur);

	startobjectchange((INTBIG)us_dragarcinst, VARCINST);
	(void)setvalkey((INTBIG)us_dragarcinst, VARCINST, el_arc_radius_key, r, VINTEGER);
	(void)modifyarcinst(us_dragarcinst, 0, 0, 0, 0, 0);
	endobjectchange((INTBIG)us_dragarcinst, VARCINST);
	us_endchanges(NOWINDOWPART);
	return(FALSE);
}

/*
 * Routine for changing the curvature of an arc so that it curves around (x,y)
 * (that coordinate is the center)
 */
BOOLEAN us_arccenterdown(INTBIG x, INTBIG y)
{
	INTBIG xcur, ycur;
	REGISTER INTBIG r;

	/* grid align the cursor value */
	if (us_dragwindow != el_curwindowpart) return(FALSE);
	if (us_setxy(x, y)) return(FALSE);
	(void)getxy(&xcur, &ycur);

	/* get radius that lets arc curve about this point */
	r = us_curvearcaboutpoint(us_dragarcinst, xcur, ycur);

	startobjectchange((INTBIG)us_dragarcinst, VARCINST);
	(void)setvalkey((INTBIG)us_dragarcinst, VARCINST, el_arc_radius_key, r, VINTEGER);
	(void)modifyarcinst(us_dragarcinst, 0, 0, 0, 0, 0);
	endobjectchange((INTBIG)us_dragarcinst, VARCINST);
	us_endchanges(NOWINDOWPART);
	return(FALSE);
}

/************************* TEXT GRAB-POINT *************************/

/* preinitialization routine when positioning text's grab point */
void us_textgrabinit(UINTBIG *olddescript, INTBIG xw, INTBIG yw, INTBIG xc, INTBIG yc,
	GEOM *geom)
{
	/* save text extent information */
	TDCOPY(us_dragdescript, olddescript);
	us_dragx = xc;          us_dragy = yc;
	us_dragox = xw;         us_dragoy = yw;
	us_dragobject = geom;
}

/* initialization routine when positioning text's grab-point */
void us_textgrabbegin(void)
{
	/* initialize polygon */
	if (us_dragpoly == NOPOLYGON) us_dragpoly = allocpolygon(4, us_tool->cluster);
	us_dragpoly->desc = &us_highl;

	us_dragshown = 0;
	us_dragwindow = el_curwindowpart;
}

/* cursor advance routine when positioning text's grab-point */
BOOLEAN us_textgrabdown(INTBIG x, INTBIG y)
{
	UINTBIG descript[TEXTDESCRIPTSIZE];
	REGISTER INTBIG style;
	REGISTER NODEINST *ni;
	XARRAY trans;

	/* get the cursor value */
	if (us_setxy(x, y) || us_dragwindow != el_curwindowpart)
	{
		(void)us_setxy(us_lastcurx, us_lastcury);
		return(FALSE);
	}
	(void)getxy(&us_lastcurx, &us_lastcury);

	/* if the box is already being shown, erase it */
	if (us_dragshown != 0)
	{
		/* undraw the box */
		us_highl.col = 0;
		us_showpoly(us_dragpoly, us_dragwindow);
	}

	/* convert the cursor position into a text descriptor */
	TDCOPY(descript, us_dragdescript);
	us_figuregrabpoint(descript, us_lastcurx, us_lastcury,
		us_dragx, us_dragy, us_dragox, us_dragoy);
	us_rotatedescriptI(us_dragobject, descript);

	/* convert the text descriptor into a GRAPHICS style */
	switch (TDGETPOS(descript))
	{
		case VTPOSCENT:      style = TEXTCENT;      break;
		case VTPOSBOXED:     style = TEXTBOX;       break;
		case VTPOSUP:        style = TEXTBOT;       break;
		case VTPOSDOWN:      style = TEXTTOP;       break;
		case VTPOSLEFT:      style = TEXTRIGHT;     break;
		case VTPOSRIGHT:     style = TEXTLEFT;      break;
		case VTPOSUPLEFT:    style = TEXTBOTRIGHT;  break;
		case VTPOSUPRIGHT:   style = TEXTBOTLEFT;   break;
		case VTPOSDOWNLEFT:  style = TEXTTOPRIGHT;  break;
		case VTPOSDOWNRIGHT: style = TEXTTOPLEFT;   break;
	}
	if (us_dragobject->entryisnode)
	{
		ni = us_dragobject->entryaddr.ni;
		makeangle(ni->rotation, ni->transpose, trans);
		style = rotatelabel(style, TDGETROTATION(descript), trans);
	}

	/* convert the text descriptor into a POLYGON */
	us_buildtexthighpoly(us_dragobject->lowx, us_dragobject->highx,
		us_dragobject->lowy, us_dragobject->highy, us_dragx, us_dragy, us_dragox, us_dragoy,
			style, us_dragpoly);

	/* draw the new box */
	us_highl.col = HIGHLIT;
	us_showpoly(us_dragpoly, us_dragwindow);
	us_dragshown = 1;
	return(FALSE);
}

/************************* ROTATE *************************/

/* preinitialization routine when rotating a node */
void us_rotateinit(NODEINST *node)
{
	us_dragnodeinst = node;
}

/* initialization routine when rotating a node */
void us_rotatebegin(void)
{
	/* initialize polygon */
	if (us_dragpoly == NOPOLYGON) us_dragpoly = allocpolygon(4, us_tool->cluster);
	us_dragpoly->desc = &us_highl;

	(void)getxy(&us_dragx, &us_dragy);
	us_dragshown = 0;
	us_dragwindow = el_curwindowpart;
	us_dragangle = figureangle((us_dragnodeinst->lowx+us_dragnodeinst->highx)/2,
		(us_dragnodeinst->lowy+us_dragnodeinst->highy)/2, us_dragx, us_dragy);
}

/* cursor advance routine when rotating the current node */
BOOLEAN us_rotatedown(INTBIG x, INTBIG y)
{
	REGISTER INTBIG lx, hx, ly, hy;
	REGISTER INTBIG newangle;
	REGISTER INTSML saver, savet;
	XARRAY trans;
	INTBIG xl, yl, xh, yh;

	/* grid align the cursor value */
	if (us_setxy(x, y) || us_dragwindow != el_curwindowpart)
	{
		(void)us_setxy(us_lastcurx, us_lastcury);
		return(FALSE);
	}
	(void)getxy(&us_lastcurx, &us_lastcury);

	/* if the box is already being shown, erase it */
	if (us_dragshown != 0)
	{
		/* undraw the box */
		us_highl.col = 0;
		us_showpoly(us_dragpoly, us_dragwindow);
	}

	/* compute new angle from node center to cursor */
	newangle = figureangle((us_dragnodeinst->lowx+us_dragnodeinst->highx)/2,
		(us_dragnodeinst->lowy+us_dragnodeinst->highy)/2, us_lastcurx, us_lastcury);

	/* compute highlighted box about node */
	nodesizeoffset(us_dragnodeinst, &xl, &yl, &xh, &yh);
	lx = us_dragnodeinst->lowx+xl;   hx = us_dragnodeinst->highx-xh;
	ly = us_dragnodeinst->lowy+yl;   hy = us_dragnodeinst->highy-yh;
	makerot(us_dragnodeinst, trans);
	maketruerectpoly(lx, hx, ly, hy, us_dragpoly);
	us_dragpoly->style = CLOSEDRECT;
	xformpoly(us_dragpoly, trans);

	/* rotate this box according to the cursor */
	saver = us_dragnodeinst->rotation;
	savet = us_dragnodeinst->transpose;
	us_dragnodeinst->transpose = 0;
	us_dragnodeinst->rotation = (INTSML)(newangle-us_dragangle);
	while (us_dragnodeinst->rotation < 0) us_dragnodeinst->rotation += 3600;
	while (us_dragnodeinst->rotation > 3600) us_dragnodeinst->rotation -= 3600;
	makerot(us_dragnodeinst, trans);
	xformpoly(us_dragpoly, trans);
	us_dragnodeinst->rotation = saver;
	us_dragnodeinst->transpose = savet;

	/* draw the new box */
	us_highl.col = HIGHLIT;
	us_showpoly(us_dragpoly, us_dragwindow);
	us_dragshown = 1;
	return(FALSE);
}

/************************* SIZE *************************/

/* preinitialization routine when changing the size of a node */
void us_sizeinit(NODEINST *node)
{
	/* initialize polygon */
	if (us_dragpoly == NOPOLYGON) us_dragpoly = allocpolygon(4, us_tool->cluster);
	us_dragpoly->desc = &us_highl;
	us_dragnodeinst = node;
	us_dragshown = 0;
	us_dragwindow = el_curwindowpart;
	us_dragcorner = -1;
}

INTBIG us_sizeterm(void)
{
	return(us_dragcorner);
}

/* preinitialization routine when changing the size of an arc */
void us_sizeainit(ARCINST *arc)
{
	us_dragarcinst = arc;
}

/* initialization routine when changing the size of an arc */
void us_sizeabegin(void)
{
	/* initialize polygon */
	if (us_dragpoly == NOPOLYGON) us_dragpoly = allocpolygon(4, us_tool->cluster);
	us_dragpoly->desc = &us_highl;

	us_dragshown = 0;
	us_dragwindow = el_curwindowpart;
}

/* cursor advance routine when stretching the current arc */
BOOLEAN us_sizeadown(INTBIG x, INTBIG y)
{
	REGISTER INTBIG wid;

	/* pan the screen if the edge was hit */
	us_panatscreenedge(&x, &y);

	/* grid align the cursor value */
	if (us_setxy(x, y) || us_dragwindow != el_curwindowpart)
	{
		(void)us_setxy(us_lastcurx, us_lastcury);
		return(FALSE);
	}
	(void)getxy(&us_lastcurx, &us_lastcury);
	gridalign(&us_lastcurx, &us_lastcury, 2, el_curlib->curnodeproto);
	us_setcursorpos(NOWINDOWFRAME, us_lastcurx, us_lastcury);

	/* if the box is already being shown, erase it */
	if (us_dragshown != 0)
	{
		/* if it didn't actually move, go no further */
		if (us_dragx == us_lastcurx && us_dragy == us_lastcury) return(FALSE);

		/* undraw the box */
		us_highl.col = 0;
		us_showpoly(us_dragpoly, us_dragwindow);
	}

	/* compute new size of the arcinst */
	if (us_dragarcinst->end[0].xpos == us_dragarcinst->end[1].xpos)
		wid = abs(us_lastcurx - us_dragarcinst->end[0].xpos) * 2; else
			wid = abs(us_lastcury - us_dragarcinst->end[0].ypos) * 2;
	makearcpoly(us_dragarcinst->length, wid, us_dragarcinst, us_dragpoly, CLOSED);
	us_dragx = us_lastcurx;
	us_dragy = us_lastcury;

	/* draw the new box */
	us_highl.col = HIGHLIT;
	us_showpoly(us_dragpoly, us_dragwindow);
	us_dragshown = 1;
	return(FALSE);
}

/* cursor advance routine when stretching the current node about far corner */
BOOLEAN us_sizedown(INTBIG x, INTBIG y)
{
	REGISTER INTBIG lx, hx, ly, hy, otherx, othery, dist, bestdist;
	INTBIG xl, xh, yl, yh, rx, ry;
	REGISTER INTBIG corner, bits;
	XARRAY trans;

	/* pan the screen if the edge was hit */
	us_panatscreenedge(&x, &y);

	/* grid align the cursor value */
	if (us_setxy(x, y) || us_dragwindow != el_curwindowpart)
	{
		(void)us_setxy(us_lastcurx, us_lastcury);
		return(FALSE);
	}
	(void)getxy(&us_lastcurx, &us_lastcury);
	gridalign(&us_lastcurx, &us_lastcury, 1, el_curlib->curnodeproto);
	us_setcursorpos(NOWINDOWFRAME, us_lastcurx, us_lastcury);

	/* if the box is already being shown, erase it */
	if (us_dragshown != 0)
	{
		/* if it didn't actually move, go no further */
		if (us_dragx == us_lastcurx && us_dragy == us_lastcury) return(FALSE);

		/* undraw the box */
		us_highl.col = 0;
		us_showpoly(us_dragpoly, us_dragwindow);
	}

	nodesizeoffset(us_dragnodeinst, &xl, &yl, &xh, &yh);
	lx = us_dragnodeinst->lowx+xl;   hx = us_dragnodeinst->highx-xh;
	ly = us_dragnodeinst->lowy+yl;   hy = us_dragnodeinst->highy-yh;
	makerot(us_dragnodeinst, trans);

	/* determine which corner is fixed by finding farthest from cursor */
	corner = us_dragcorner;
	if (corner < 0)
	{
		xform(lx, ly, &rx, &ry, trans);
		bestdist = abs(rx - us_lastcurx) + abs(ry - us_lastcury);
		corner = 1;	/* lower-left */
		xform(hx, ly, &rx, &ry, trans);
		dist = abs(rx - us_lastcurx) + abs(ry - us_lastcury);
		if (dist < bestdist)
		{
			bestdist = dist;   corner = 2;	/* lower-right */
		}
		xform(lx, hy, &rx, &ry, trans);
		dist = abs(rx - us_lastcurx) + abs(ry - us_lastcury);
		if (dist < bestdist)
		{
			bestdist = dist;   corner = 3;	/* upper-left */
		}
		xform(hx, hy, &rx, &ry, trans);
		dist = abs(rx - us_lastcurx) + abs(ry - us_lastcury);
		if (dist < bestdist) corner = 4;	/* upper-right */
		if (us_dragcorner < 0) us_dragcorner = corner;
	}

	bits = getbuckybits();
	switch (corner)
	{
		case 1:		/* lower-left */
			otherx = hx;   othery = hy;
			if ((bits&CONTROLDOWN) != 0)
			{
				xform(lx, ly, &rx, &ry, trans);
				if (abs(rx-us_lastcurx) < abs(ry-us_lastcury)) us_lastcurx = rx; else
					us_lastcury = ry;
			}
			break;
		case 2:		/* lower-right */
			otherx = lx;   othery = hy;
			if ((bits&CONTROLDOWN) != 0)
			{
				xform(hx, ly, &rx, &ry, trans);
				if (abs(rx-us_lastcurx) < abs(ry-us_lastcury)) us_lastcurx = rx; else
					us_lastcury = ry;
			}
			break;
		case 3:		/* upper-left */
			otherx = hx;   othery = ly;
			if ((bits&CONTROLDOWN) != 0)
			{
				xform(lx, hy, &rx, &ry, trans);
				if (abs(rx-us_lastcurx) < abs(ry-us_lastcury)) us_lastcurx = rx; else
					us_lastcury = ry;
			}
			break;
		case 4:		/* upper-right */
			otherx = lx;   othery = ly;
			if ((bits&CONTROLDOWN) != 0)
			{
				xform(hx, hy, &rx, &ry, trans);
				if (abs(rx-us_lastcurx) < abs(ry-us_lastcury)) us_lastcurx = rx; else
					us_lastcury = ry;
			}
			break;
	}
	xform(otherx, othery, &rx, &ry, trans);
	maketruerectpoly(mini(rx, us_lastcurx), maxi(rx, us_lastcurx),
		mini(ry, us_lastcury), maxi(ry, us_lastcury), us_dragpoly);
	us_dragpoly->style = CLOSEDRECT;
	us_dragx = us_lastcurx;
	us_dragy = us_lastcury;

	/* draw the new box */
	us_highl.col = HIGHLIT;
	us_showpoly(us_dragpoly, us_dragwindow);
	us_dragshown = 1;
	return(FALSE);
}

/* cursor advance routine when stretching the current node about center */
BOOLEAN us_sizecdown(INTBIG x, INTBIG y)
{
	REGISTER INTBIG dx, dy, cx, cy;

	/* pan the screen if the edge was hit */
	us_panatscreenedge(&x, &y);

	/* grid align the cursor value */
	if (us_setxy(x, y) || us_dragwindow != el_curwindowpart)
	{
		(void)us_setxy(us_lastcurx, us_lastcury);
		return(FALSE);
	}
	(void)getxy(&us_lastcurx, &us_lastcury);
	gridalign(&us_lastcurx, &us_lastcury, 2, el_curlib->curnodeproto);
	us_setcursorpos(NOWINDOWFRAME, us_lastcurx, us_lastcury);

	/* if the box is already being shown, erase it */
	if (us_dragshown != 0)
	{
		/* if it didn't actually move, go no further */
		if (us_dragx == us_lastcurx && us_dragy == us_lastcury) return(FALSE);

		/* undraw the box */
		us_highl.col = 0;
		us_showpoly(us_dragpoly, us_dragwindow);
	}

	/* compute new size of the nodeinst */
	cx = (us_dragnodeinst->lowx+us_dragnodeinst->highx) / 2;
	cy = (us_dragnodeinst->lowy+us_dragnodeinst->highy) / 2;
	dx = abs(cx - us_lastcurx);
	dy = abs(cy - us_lastcury);

	maketruerectpoly(cx-dx, cx+dx, cy-dy, cy+dy, us_dragpoly);
	us_dragpoly->style = CLOSEDRECT;
	us_dragx = us_lastcurx;
	us_dragy = us_lastcury;

	/* draw the new box */
	us_highl.col = HIGHLIT;
	us_showpoly(us_dragpoly, us_dragwindow);
	us_dragshown = 1;
	return(FALSE);
}

/************************* FIND AREA *************************/

/* pre-initialization routine when doing "find area-XXXX" */
void us_findinit(INTBIG sizex, INTBIG sizey)
{
	us_dragox = sizex;
	us_dragoy = sizey;
}

/* initialization routine when doing "find area-move" */
void us_findmbegin(void)
{
	INTBIG xcur, ycur;

	/* initialize polygon */
	if (us_dragpoly == NOPOLYGON) us_dragpoly = allocpolygon(4, us_tool->cluster);
	us_dragpoly->desc = &us_highl;

	us_dragshown = 0;
	(void)getxy(&xcur, &ycur);
	maketruerectpoly(xcur, xcur+us_dragox, ycur, ycur+us_dragoy, us_dragpoly);
	us_dragpoly->style = CLOSEDRECT;
	us_dragx = xcur;   us_dragy = ycur;
	us_dragwindow = el_curwindowpart;
}

/* initialization routine when doing "find area-size" */
void us_findsbegin(void)
{
	INTBIG xcur, ycur;

	/* initialize polygon */
	if (us_dragpoly == NOPOLYGON) us_dragpoly = allocpolygon(4, us_tool->cluster);
	us_dragpoly->desc = &us_highl;

	us_dragshown = 0;
	(void)getxy(&xcur, &ycur);
	maketruerectpoly(us_dragox, xcur, us_dragoy, ycur, us_dragpoly);
	us_dragpoly->style = CLOSEDRECT;
	us_dragx = xcur;   us_dragy = ycur;
	us_dragwindow = el_curwindowpart;
}

/* initialization routine when doing "find area-define" */
void us_finddbegin(void)
{
	INTBIG xcur, ycur;

	/* initialize polygon */
	if (us_dragpoly == NOPOLYGON) us_dragpoly = allocpolygon(4, us_tool->cluster);
	us_dragpoly->desc = &us_highl;

	us_dragshown = 0;
	(void)getxy(&xcur, &ycur);
	us_setcursorpos(NOWINDOWFRAME, xcur, ycur);
	us_dragox = xcur;
	us_dragoy = ycur;
	maketruerectpoly(us_dragox, xcur, us_dragoy, ycur, us_dragpoly);
	us_dragpoly->style = CLOSEDRECT;
	us_dragx = xcur;   us_dragy = ycur;
	us_dragwindow = el_curwindowpart;
}

/* posttermination routine when doing "find area-define" */
void us_finddterm(INTBIG *x, INTBIG *y)
{
	*x = us_dragox;
	*y = us_dragoy;
}

/************************* FIND INTERACTIVE *************************/

void us_findiinit(INTBIG findport, INTBIG extrainfo, INTBIG findangle, INTBIG stayonhighlighted,
	INTBIG findstill, INTBIG findnobox, INTBIG findspecial)
{
	us_dragfport = findport;
	us_dragextra = (extrainfo != 0 ? 1 : 0);
	us_dragangle = findangle;
	us_dragstill = findstill;
	us_dragnobox = findnobox;
	us_dragspecial = findspecial;
	us_dragpoint = -1;
	us_dragstayonhigh = stayonhighlighted;
}

void us_findcibegin(void)
{
	us_finddoibegin(TRUE);
}

void us_findibegin(void)
{
	us_finddoibegin(FALSE);
}

static UINTBIG us_initialtime, us_motiondelayafterselection;

void us_finddoibegin(BOOLEAN complement)
{
	REGISTER VARIABLE *highvar;
	REGISTER NODEINST *ni;
	REGISTER PORTPROTO *pp;
	REGISTER VARIABLE *var;
	REGISTER INTBIG len, another;
	HIGHLIGHT newhigh, oldhigh;
	INTBIG i;
	INTBIG xcur, ycur, dist, bestdist;

	/* remember initial time of this action */
	us_initialtime = ticktime();
	var = getvalkey((INTBIG)us_tool, VTOOL, VFRACT, us_motiondelaykey);
	if (var == NOVARIABLE) us_motiondelayafterselection = 30; else
		us_motiondelayafterselection = var->addr * 30 / WHOLE;

	/* initialize polygon */
	(void)needstaticpolygon(&us_dragpoly, 4, us_tool->cluster);
	us_dragpoly->desc = &us_highl;
	us_dragstate = -1;
	us_dragshown = 0;
	us_dragtextcount = 0;
	us_dragwindow = el_curwindowpart;

	/* get cursor coordinates */
	if (getxy(&xcur, &ycur)) return;
	us_dragx = xcur;   us_dragy = ycur;
	us_dragox = us_dragx;   us_dragoy = us_dragy;
	us_dragnodeproto = getcurcell();

	/* see what is highlighted */
	highvar = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (highvar != NOVARIABLE)
	{
		len = getlength(highvar);
		(void)us_makehighlight(((CHAR **)highvar->addr)[0], &newhigh);
	} else
	{
		len = 0;
		newhigh.status = 0;
		newhigh.fromgeom = NOGEOM;
		newhigh.fromport = NOPORTPROTO;
		newhigh.fromvar = NOVARIABLE;
		newhigh.fromvarnoeval = NOVARIABLE;
		newhigh.frompoint = 0;
	}

	/* see what is under the cursor */
	if (us_dragstayonhigh != 0 && highvar != NOVARIABLE)
	{
		/* see if the cursor is over something highlighted */
		for(i=0; i<len; i++)
		{
			(void)us_makehighlight(((CHAR **)highvar->addr)[i], &oldhigh);
			if (us_cursoroverhigh(&oldhigh, xcur, ycur, el_curwindowpart)) break;
		}
		if (i < len)
		{
			/* re-find the closest port when one node is selected */
			(void)us_makehighlight(((CHAR **)highvar->addr)[i], &newhigh);
			if (len == 1 && (newhigh.status&HIGHFROM) != 0 &&
				newhigh.fromgeom->entryisnode)
			{
				ni = newhigh.fromgeom->entryaddr.ni;
				newhigh.fromport = NOPORTPROTO;
				for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				{
					shapeportpoly(ni, pp, us_dragpoly, FALSE);
					us_dragpoly->desc = &us_highl;

					/* get distance of desired point to polygon */
					dist = polydistance(us_dragpoly, xcur, ycur);
					if (dist < 0)
					{
						newhigh.fromport = pp;
						break;
					}
					if (newhigh.fromport == NOPORTPROTO) bestdist = dist;
					if (dist > bestdist) continue;
					bestdist = dist;   newhigh.fromport = pp;
				}
			}
		} else
		{
			/* control NOT held, nothing under cursor */
			us_findobject(xcur, ycur, el_curwindowpart, &newhigh, 0, 0, us_dragfport, 1, us_dragspecial);
		}
	} else
	{
		another = 0;
		if (complement)
		{
			/* control-shift held or nothing highlighted and shift held */
			if (len > 0)
			{
				for(i=0; i<len; i++)
				{
					(void)us_makehighlight(((CHAR **)highvar->addr)[i], &newhigh);
					if (us_cursoroverhigh(&newhigh, xcur, ycur, el_curwindowpart)) break;
				}
				if (i < len)
				{
					/* remove current highlight, find another */
					another = 1;
					us_delhighlight(&newhigh);
					highvar = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
					if (highvar == NOVARIABLE) len = 0; else
						len = getlength(highvar);
				}
				us_findobject(xcur, ycur, el_curwindowpart, &newhigh, 0, another, us_dragfport,
					1, us_dragspecial);
			}
		} else
		{
			/* control held or nothing highlighted */
			us_findobject(xcur, ycur, el_curwindowpart, &newhigh, 0, another, us_dragfport,
				1, us_dragspecial);
		}
	}
	if (newhigh.status == 0)
	{
		/* nothing under cursor: select an area */
		if (complement) us_dragstate = 2; else us_dragstate = 1;
		maketruerectpoly(us_dragx, us_dragx, us_dragy, us_dragy, us_dragpoly);
		us_dragpoly->style = CLOSEDRECT;
		return;
	}

	/* grid the cursor coordinates when over something */
	gridalign(&us_dragx, &us_dragy, 1, us_dragnodeproto);
	us_setcursorpos(NOWINDOWFRAME, us_dragx, us_dragy);
	us_dragox = us_dragx;   us_dragoy = us_dragy;

	/* clear port info if not requested or multiple highlights */
	if (us_dragfport == 0) newhigh.fromport = NOPORTPROTO;

	/* do not show invisible pins (text on them is shown separately) */
	us_multidragshowinvpin = FALSE;

	/* see if object under cursor is already highlighted */
	for(i=0; i<len; i++)
	{
		if (us_makehighlight(((CHAR **)highvar->addr)[i], &oldhigh)) continue;
		if (oldhigh.cell != newhigh.cell) continue;
		if ((oldhigh.status&HIGHTYPE) != (newhigh.status&HIGHTYPE)) continue;
		if (oldhigh.fromgeom != newhigh.fromgeom) continue;
		if ((oldhigh.status&HIGHTYPE) == HIGHTEXT)
		{
			if (oldhigh.fromvar != newhigh.fromvar) continue;
			if (oldhigh.fromport != NOPORTPROTO && newhigh.fromport != NOPORTPROTO &&
				oldhigh.fromport != newhigh.fromport) continue;
		}
		break;
	}

	/* is this a normal command or SHIFTed? */
	if (!complement)
	{
		/* normal find/move */
		newhigh.cell = getcurcell();
		if (i >= len || us_dragpoint >= 0)
		{
			/* object is not highlighted: select only it and move */
			us_setfind(&newhigh, (us_dragpoint < 0 ? 0 : 1),
				(us_dragextra != 0 ? HIGHEXTRA : 0), 0, us_dragnobox);
		} else
		{
			/* object is already highlighted: rehighlight and move */
			us_delhighlight(&newhigh);
			us_addhighlight(&newhigh);
		}
	} else
	{
		/* SHIFT held: find/move */
		if (i >= len)
		{
			/* object not highlighted: add it and move */
			newhigh.cell = getcurcell();
			us_setfind(&newhigh, 0, (us_dragextra != 0 ? HIGHEXTRA : 0), 1, us_dragnobox);
		} else
		{
			/* object highlighted: unhighlight it and quit */
			us_delhighlight(&newhigh);
			return;
		}
	}

	/* can only drag if in "Mac" mode */
	us_cantdrag = 1;
	if (us_dragstayonhigh == 0) return;

	/* do no motion if stillness requested */
	if (us_dragstill != 0 || (us_dragnodeproto->userbits&NPLOCKED) != 0)
	{
		us_dragstate = -1;
		return;
	}

	/* get the objects to be moved */
	highvar = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (highvar == NOVARIABLE) return;

	/* get the selected nodes, arcs, and text (mark nodes with nonzero "temp1") */
	us_dragobjectlist = us_gethighlighted(WANTNODEINST | WANTARCINST,
		&us_dragtextcount, &us_dragtexts);

	/* stop if nothing selected */
	if (us_dragobjectlist[0] == NOGEOM && us_dragtextcount == 0) return;
	us_cantdrag = 0;

	/* stop if from different cells */
	for(i=0; us_dragobjectlist[i] != NOGEOM; i++)
		if (us_dragnodeproto != geomparent(us_dragobjectlist[i])) return;

	/* count the number of nodes */
	us_dragnodetotal = 0;
	for(ni = us_dragnodeproto->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		if (ni->temp1 != 0) us_dragnodetotal++;

	/* build a list that includes all nodes touching selected arcs */
	if (us_dragnodetotal != 0)
	{
		us_dragnodelist = (NODEINST **)emalloc((us_dragnodetotal * (sizeof (NODEINST *))),
			el_tempcluster);
		if (us_dragnodelist == 0) return;
		for(i=0, ni = us_dragnodeproto->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if (ni->temp1 != 0)
				us_dragnodelist[i++] = ni;
		}
	}

	/* save and turn off all highlighting */
	us_pushhighlight();
	us_clearhighlightcount();

	/* setup for moving */
	us_multidragbegin();
	us_multidragshowoffset = TRUE;
	us_dragstate = 0;
}

BOOLEAN us_findidown(INTBIG x, INTBIG y)
{
	REGISTER UINTBIG now;
	BOOLEAN ret;

	/* make the drag */
	ret = TRUE;
	switch (us_dragstate)
	{
		case 0:
			/* pan the screen if the edge was hit */
			us_panatscreenedge(&x, &y);
			now = eventtime();
			if (now < us_initialtime + us_motiondelayafterselection) ret = 0; else
				ret = us_multidragdown(x, y);
			break;
		case 1:
		case 2:
			ret = us_stretchdown(x, y);
			break;
	}
	return(ret);
}

void us_findiup(void)
{
	INTBIG xcur, ycur;
	CHAR **list;
	REGISTER INTBIG i, len, k, slx, shx, sly, shy, bits, angle, total;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER BOOLEAN domove;
	REGISTER UINTBIG now;
	HIGHLIGHT newhigh, oldhigh;
	REGISTER VARIABLE *highvar;
	static POLYGON *poly = NOPOLYGON;

	(void)needstaticpolygon(&poly, 4, us_tool->cluster);
	if (us_dragstate == 0)
	{
		us_multidragup();
		now = ticktime();
		if (now >= us_initialtime + us_motiondelayafterselection &&
			us_cantdrag == 0 && el_pleasestop == 0)
		{
			if (getxy(&xcur, &ycur)) return;
			gridalign(&xcur, &ycur, 1, us_dragnodeproto);
			us_setcursorpos(NOWINDOWFRAME, xcur, ycur);
			bits = getbuckybits();
			if ((bits&CONTROLDOWN) != 0) angle = us_dragangle; else angle = 0;
			us_getslide(angle, us_dragox, us_dragoy, xcur, ycur, &us_dragx, &us_dragy);
			if (us_dragx != us_dragox || us_dragy != us_dragoy)
			{
				/* eliminate locked nodes */
				domove = TRUE;
				for(i=0; i<us_dragnodetotal; i++)
				{
					ni = us_dragnodelist[i];
					if (us_cantedit(ni->parent, ni, TRUE)) { domove = FALSE;   break; }
				}

				/* also check for locked nodes at the end of selected arcs */
				if (domove)
				{
					for(i=0; us_dragobjectlist[i] != NOGEOM; i++)
					{
						if (!us_dragobjectlist[i]->entryisnode)
						{
							ai = us_dragobjectlist[i]->entryaddr.ai;
							if (us_cantedit(us_dragnodeproto, ai->end[0].nodeinst, TRUE) ||
								us_cantedit(us_dragnodeproto, ai->end[1].nodeinst, TRUE))
							{
								domove = FALSE;
								break;
							}
						}
					}
				}

				/* do the move */
				if (domove)
				{
					us_manymove(us_dragobjectlist, us_dragnodelist, us_dragnodetotal,
						us_dragx-us_dragox, us_dragy-us_dragoy);
					us_moveselectedtext(us_dragtextcount, us_dragtexts,
						us_dragobjectlist, us_dragx - us_dragox, us_dragy - us_dragoy);
				}
			}
		}
		if (us_dragnodetotal > 0) efree((CHAR *)us_dragnodelist);
		us_pophighlight(TRUE);
	} else if (us_dragstate == 1)
	{
		/* get area of drag */
		us_invertdragup();
		slx = mini(us_dragox, us_dragx);   shx = maxi(us_dragox, us_dragx);
		sly = mini(us_dragoy, us_dragy);   shy = maxi(us_dragoy, us_dragy);
		us_clearhighlightcount();

		total = us_selectarea(us_dragnodeproto, slx, shx, sly, shy, us_dragspecial,
			us_dragfport, us_dragnobox, &list);
		if (total > 0)
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_highlightedkey, (INTBIG)list,
				VSTRING|VISARRAY|(total<<VLENGTHSH)|VDONTSAVE);
	} else if (us_dragstate == 2)
	{
		/* special case when complementing highlight */
		us_invertdragup();
		slx = mini(us_dragox, us_dragx);   shx = maxi(us_dragox, us_dragx);
		sly = mini(us_dragoy, us_dragy);   shy = maxi(us_dragoy, us_dragy);
		highvar = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
		if (highvar != NOVARIABLE) len = getlength(highvar); else len = 0;

		total = us_selectarea(us_dragnodeproto, slx, shx, sly, shy, us_dragspecial,
			us_dragfport, us_dragnobox, &list);

		for(i=0; i<total; i++)
		{
			if (us_makehighlight(list[i], &newhigh)) continue;

			/* see if this object is already highlighted */
			for(k=0; k<len; k++)
			{
				if (us_makehighlight(((CHAR **)highvar->addr)[k], &oldhigh)) continue;
				if (oldhigh.cell != newhigh.cell) continue;
				if ((oldhigh.status&HIGHTYPE) != (newhigh.status&HIGHTYPE)) continue;
				if ((oldhigh.status&HIGHTYPE) == HIGHFROM)
				{
					if (oldhigh.fromgeom != newhigh.fromgeom) continue;
				} else if ((oldhigh.status&HIGHTYPE) == HIGHTEXT)
				{
					if (oldhigh.fromvar != newhigh.fromvar) continue;
					if (oldhigh.fromvar == NOVARIABLE)
					{
						if (oldhigh.fromgeom != newhigh.fromgeom) continue;
						if (oldhigh.fromport != newhigh.fromport) continue;
					}
				}
				break;
			}
			if (k < len) us_delhighlight(&newhigh); else
				us_addhighlight(&newhigh);

			/* recache what is highlighted */
			highvar = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
			if (highvar != NOVARIABLE) len = getlength(highvar); else len = 0;
		}
	}
}

/*
 * Routine to determine whether geometry object "geom" is inside the area
 * (slx <= X <= shx, sly <= Y <= shy).  Returns true if so.
 */
BOOLEAN us_geominrect(GEOM *geom, INTBIG slx, INTBIG shx, INTBIG sly, INTBIG shy)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	INTBIG plx, ply, phx, phy;
	REGISTER INTBIG wid, lx, hx, ly, hy;
	XARRAY trans;
	static POLYGON *poly = NOPOLYGON;

	(void)needstaticpolygon(&poly, 4, us_tool->cluster);
	if (geom->entryisnode)
	{
		/* handle nodes */
		ni = geom->entryaddr.ni;
		makerot(ni, trans);

		/* get the true bounds of the node */
		nodesizeoffset(ni, &plx, &ply, &phx, &phy);
		lx = ni->lowx+plx;   hx = ni->highx-phx;
		ly = ni->lowy+ply;   hy = ni->highy-phy;
		maketruerectpoly(lx, hx, ly, hy, poly);
		poly->style = FILLEDRECT;

		/* transform to account for node orientation */
		xformpoly(poly, trans);

		/* see if it is in the selected area */
		if ((us_useroptions&MUSTENCLOSEALL) != 0)
		{
			getbbox(poly, &plx, &phx, &ply, &phy);
			if (plx < slx || phx > shx || ply < sly || phy > shy)
				return(FALSE);
		} else
		{
			if (!polyinrect(poly, slx, shx, sly, shy)) return(FALSE);
		}
		return(TRUE);
	}

	/* handle arcs */
	ai = geom->entryaddr.ai;
	wid = ai->width - arcwidthoffset(ai);
	if (wid == 0) wid = lambdaofarc(ai);
	if (curvedarcoutline(ai, poly, FILLED, wid))
		makearcpoly(ai->length, wid, ai, poly, FILLED);
	if ((us_useroptions&MUSTENCLOSEALL) != 0)
	{
		getbbox(poly, &plx, &phx, &ply, &phy);
		if (plx < slx || phx > shx || ply < sly || phy > shy)
			return(FALSE);
	} else
	{	
		if (!polyinrect(poly, slx, shx, sly, shy)) return(FALSE);
	}
	return(TRUE);
}

/************************* CREATE INSERT/BREAKPOINT *************************/

/* pre-initialization routine when inserting type "np" in arc "ai" */
void us_createinsinit(ARCINST *ai, NODEPROTO *np)
{
	us_dragarcinst = ai;
	us_dragnodeproto = np;
}

/* initialization routine when inserting a breakpoint */
void us_createinsbegin(void)
{
	XARRAY trans;
	INTBIG cx, cy, lx, ly, hx, hy, xcur, ycur, pxs, pys;
	BOOLEAN centeredprimitives;
	REGISTER NODEINST *ni;
	NODEINST node;

	/* initialize polygon */
	if (us_dragpoly == NOPOLYGON) us_dragpoly = allocpolygon(4, us_tool->cluster);
	us_dragpoly->desc = &us_highl;

	(void)getxy(&xcur, &ycur);

	us_dragshown = 0;
	ni = &node;   initdummynode(ni);
	ni->proto = us_dragnodeproto;
	ni->rotation = 0;
	ni->transpose = 0;
	defaultnodesize(us_dragnodeproto, &pxs, &pys);
	ni->lowx = xcur;   ni->highx = xcur + pxs;
	ni->lowy = ycur;   ni->highy = ycur + pys;
	makerot(ni, trans);
	nodeprotosizeoffset(us_dragnodeproto, &lx, &ly, &hx, &hy, el_curwindowpart->curnodeproto);
	maketruerectpoly(ni->lowx+lx, ni->highx-hx, ni->lowy+ly, ni->highy-hy, us_dragpoly);
	us_dragpoly->style = CLOSEDRECT;
	xformpoly(us_dragpoly, trans);
	if ((us_useroptions&CENTEREDPRIMITIVES) != 0) centeredprimitives = TRUE; else
		centeredprimitives = FALSE;
	corneroffset(NONODEINST, us_dragnodeproto, 0, 0, &cx, &cy,
		centeredprimitives);
	us_dragoffx = cx;
	us_dragoffy = cy;
	us_dragx = xcur;   us_dragy = ycur;
	us_dragwindow = el_curwindowpart;
	us_dragobject = NOGEOM;
	us_dragportproto = NOPORTPROTO;
}

/* cursor advance routine when creating inside an arc */
BOOLEAN us_createinsdown(INTBIG x, INTBIG y)
{
	REGISTER INTBIG i;
	INTBIG dx, dy, pxs, pys;

	/* grid align the cursor value */
	if (us_setxy(x, y) || us_dragwindow != el_curwindowpart)
	{
		(void)us_setxy(us_lastcurx, us_lastcury);
		return(FALSE);
	}
	(void)getxy(&us_lastcurx, &us_lastcury);
	gridalign(&us_lastcurx, &us_lastcury, 1, el_curlib->curnodeproto);
	us_setcursorpos(NOWINDOWFRAME, us_lastcurx, us_lastcury);

	/* find closest point along arc */
	dx = us_lastcurx;   dy = us_lastcury;
	(void)closestpointtosegment(us_dragarcinst->end[0].xpos, us_dragarcinst->end[0].ypos,
		us_dragarcinst->end[1].xpos, us_dragarcinst->end[1].ypos, &dx, &dy);
	defaultnodesize(us_dragnodeproto, &pxs, &pys);
	dx -= pxs/2;
	dy -= pys/2;

	/* if the box is already being shown, erase it */
	if (us_dragshown != 0)
	{
		/* if it didn't actually move, go no further */
		if (us_dragx == dx && us_dragy == dy) return(FALSE);

		/* undraw the box */
		us_highl.col = 0;
		us_showpoly(us_dragpoly, us_dragwindow);
	}

	/* advance the box */
	for(i = 0; i < us_dragpoly->count; i++)
	{
		us_dragpoly->xv[i] += dx - us_dragx;
		us_dragpoly->yv[i] += dy - us_dragy;
	}
	us_dragx = dx;
	us_dragy = dy;

	/* draw the new box */
	us_highl.col = HIGHLIT;
	us_showpoly(us_dragpoly, us_dragwindow);
	us_dragshown = 1;
	return(FALSE);
}

/************************* CREATE NODE *************************/

/* pre-initialization routine when creating an object */
void us_createinit(INTBIG cornerx, INTBIG cornery, NODEPROTO *np, INTBIG angle,
	BOOLEAN join, GEOM *fromgeom, PORTPROTO *fromport)
{
	us_dragox = cornerx;
	us_dragoy = cornery;
	us_dragnodeproto = np;
	us_dragangle = angle;
	us_dragjoinfactor = join;
	us_dragfromgeom = fromgeom;
	us_dragfromport = fromport;
}

/* initialization routine when creating an object */
void us_createbegin(void)
{
	XARRAY trans;
	INTBIG cx, cy, lx, ly, hx, hy, xcur, ycur, pxs, pys;
	REGISTER NODEINST *ni;
	NODEINST node;
	BOOLEAN centeredprimitives;
	REGISTER WINDOWFRAME *curframe;

	/* initialize polygon */
	(void)needstaticpolygon(&us_dragpoly, 4, us_tool->cluster);
	us_dragpoly->desc = &us_highl;

	(void)getxy(&xcur, &ycur);
	curframe = getwindowframe(TRUE);
	if ((us_tool->toolstate&MENUON) != 0)
	{
		if (curframe == us_menuframe ||
			(us_menuframe == NOWINDOWFRAME && ycur >= us_menuly && ycur <= us_menuhy &&
				xcur >= us_menulx && xcur <= us_menuhx))
		{
			el_pleasestop = 1;
		}
	}

	us_dragshown = 0;
	ni = &node;   initdummynode(ni);
	ni->proto = us_dragnodeproto;
	ni->rotation = (INTSML)(us_dragangle%3600);
	ni->transpose = (INTSML)(us_dragangle/3600);
	defaultnodesize(us_dragnodeproto, &pxs, &pys);
	if ((us_useroptions&CENTEREDPRIMITIVES) != 0) centeredprimitives = TRUE; else
		centeredprimitives = FALSE;
	corneroffset(NONODEINST, us_dragnodeproto, us_dragangle%3600,
		us_dragangle/3600, &cx, &cy, centeredprimitives);

	/* adjust size if technologies differ */
	us_adjustfornodeincell(us_dragnodeproto, el_curwindowpart->curnodeproto, &cx, &cy);
	us_adjustfornodeincell(us_dragnodeproto, el_curwindowpart->curnodeproto, &pxs, &pys);
	ni->lowx = xcur;   ni->highx = xcur + pxs;
	ni->lowy = ycur;   ni->highy = ycur + pys;
	makerot(ni, trans);
	nodeprotosizeoffset(us_dragnodeproto, &lx, &ly, &hx, &hy, el_curwindowpart->curnodeproto);
	maketruerectpoly(ni->lowx+lx, ni->highx-hx, ni->lowy+ly, ni->highy-hy, us_dragpoly);
	us_dragpoly->style = CLOSEDRECT;
	xformpoly(us_dragpoly, trans);
	us_dragx = xcur + cx;   us_dragy = ycur + cy;
	us_dragwindow = el_curwindowpart;
	us_dragwirepathcount = 0;
}

/* initialization routine when creating along an angle */
void us_createabegin(void)
{
	XARRAY trans;
	INTBIG cx, cy, lx, ly, hx, hy, xcur, ycur, pxs, pys;
	REGISTER NODEINST *ni;
	NODEINST node;
	REGISTER BOOLEAN centeredprimitives;

	/* initialize polygon */
	if (us_dragpoly == NOPOLYGON) us_dragpoly = allocpolygon(4, us_tool->cluster);
	us_dragpoly->desc = &us_highl;

	(void)getxy(&xcur, &ycur);

	/* determine starting coordinate */
	if ((us_dragfromport->userbits&PORTISOLATED) != 0)
	{
		/* use prefered location on isolated ports */
		us_dragpoly->xv[0] = xcur;   us_dragpoly->yv[0] = ycur;   us_dragpoly->count = 1;
		shapeportpoly(us_dragfromgeom->entryaddr.ni, us_dragfromport, us_dragpoly, TRUE);
		us_dragstartx = xcur;   us_dragstarty = ycur;
		closestpoint(us_dragpoly, &us_dragstartx, &us_dragstarty);
	} else
	{
		us_portposition(us_dragfromgeom->entryaddr.ni, us_dragfromport,
			&us_dragstartx, &us_dragstarty);
	}

	us_dragshown = 0;
	ni = &node;   initdummynode(ni);
	ni->proto = us_dragnodeproto;
	ni->rotation = 0;
	ni->transpose = 0;
	defaultnodesize(us_dragnodeproto, &pxs, &pys);
	if ((us_useroptions&CENTEREDPRIMITIVES) != 0) centeredprimitives = TRUE; else
		centeredprimitives = FALSE;
	corneroffset(NONODEINST, us_dragnodeproto, 0, 0, &cx, &cy, centeredprimitives);

	/* adjust size if technologies differ */
	us_adjustfornodeincell(us_dragnodeproto, us_dragwindow->curnodeproto, &cx, &cy);
	us_adjustfornodeincell(us_dragnodeproto, us_dragwindow->curnodeproto, &pxs, &pys);

	ni->lowx = xcur;   ni->highx = xcur + pxs;
	ni->lowy = ycur;   ni->highy = ycur + pys;
	makerot(ni, trans);
	nodeprotosizeoffset(us_dragnodeproto, &lx, &ly, &hx, &hy, el_curwindowpart->curnodeproto);
	maketruerectpoly(ni->lowx+lx, ni->highx-hx, ni->lowy+ly, ni->highy-hy, us_dragpoly);
	us_dragpoly->style = CLOSEDRECT;
	xformpoly(us_dragpoly, trans);

	us_dragoffx = cx;
	us_dragoffy = cy;
	us_dragx = xcur;   us_dragy = ycur;
	us_dragwindow = el_curwindowpart;
	us_dragobject = NOGEOM;
	us_dragportproto = NOPORTPROTO;
}

/* cursor advance routine when creating along an angle */
BOOLEAN us_createadown(INTBIG x, INTBIG y)
{
	REGISTER INTBIG dx, dy, dist, bestdist, count, samepath, alignment;
	REGISTER INTBIG i;
	INTBIG pxs, pys, unalx, unaly, fakecoords[8];
	REGISTER PORTPROTO *pp, *foundpp;
	REGISTER NODEINST *ni, *fakefromnode, *faketonode;
	ARCINST *ai1, *ai2, *ai3;
	NODEINST *con1, *con2;
	GEOM *fakefromgeom, *faketogeom;
	PORTPROTO *fakefromport, *faketoport;
	REGISTER GEOM *foundgeom;
	static POLYGON *poly = NOPOLYGON;

	/* get the polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* pan the screen if the edge was hit */
	us_panatscreenedge(&x, &y);

	/* grid align the cursor value */
	if (us_setxy(x, y) || us_dragwindow != el_curwindowpart)
	{
		(void)us_setxy(us_lastcurx, us_lastcury);
		return(FALSE);
	}
	(void)getxy(&us_lastcurx, &us_lastcury);
	unalx = us_lastcurx;   unaly = us_lastcury;
	gridalign(&us_lastcurx, &us_lastcury, 1, us_dragwindow->curnodeproto);
	us_setcursorpos(NOWINDOWFRAME, us_lastcurx, us_lastcury);

	us_getslide(us_dragangle, us_dragox, us_dragoy, us_lastcurx, us_lastcury, &pxs, &pys);
	dx = pxs - us_dragoffx;
	dy = pys - us_dragoffy;

	/* find the object under the cursor */
	foundpp = NOPORTPROTO;
	foundgeom = NOGEOM;
	if (us_dragjoinfactor)
	{
		defaultnodesize(us_dragnodeproto, &pxs, &pys);
		us_adjustfornodeincell(us_dragnodeproto, us_dragwindow->curnodeproto, &pxs, &pys);
		alignment = muldiv(us_alignment_ratio, el_curlib->lambda[el_curtech->techindex], WHOLE);
		foundgeom = us_getclosest(us_lastcurx-us_dragoffx+pxs/2, us_lastcury-us_dragoffy+pys/2,
			alignment/2, us_dragwindow->curnodeproto);

		/* determine closest port if a node was found */
		if (foundgeom != NOGEOM && foundgeom->entryisnode)
		{
			/* find the closest port if this is a nodeinst and no port hit directly */
			ni = foundgeom->entryaddr.ni;
			for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			{
				shapeportpoly(ni, pp, poly, FALSE);

				/* get distance of desired point to polygon */
				dist = polydistance(poly, unalx, unaly);
				if (dist < 0)
				{
					foundpp = pp;
					break;
				}
				if (foundpp == NOPORTPROTO) bestdist = dist;
				if (dist > bestdist) continue;
				bestdist = dist;   foundpp = pp;
			}
		}
	}

	/* determine what is being connected */
	count = 0;
	fakefromgeom = us_dragfromgeom;   fakefromport = us_dragfromport;
	faketogeom = foundgeom;           faketoport = foundpp;
	if (fakefromgeom == NOGEOM) fakefromnode = NONODEINST; else
		fakefromnode = us_getnodeonarcinst(&fakefromgeom, &fakefromport, faketogeom,
			faketoport, pxs, pys, 1);
	if (faketogeom == NOGEOM) faketonode = NONODEINST; else
		faketonode = us_getnodeonarcinst(&faketogeom, &faketoport, fakefromgeom,
			fakefromport, pxs, pys, 1);
	if (fakefromnode != NONODEINST && faketonode != NONODEINST)
	{
		if (!us_figuredrawpath(us_dragfromgeom, us_dragfromport,
			foundgeom, foundpp, &unalx, &unaly))
		{
			ai1 = us_makeconnection(fakefromnode, fakefromport,
				gen_universalarc, faketonode, faketoport, gen_universalarc,
				gen_univpinprim, unalx, unaly, &ai2, &ai3, &con1, &con2,
				us_dragangle, FALSE, fakecoords, TRUE);
			if (ai1 != NOARCINST) count = 4;
			if (ai2 != NOARCINST) count = 6;
			if (ai3 != NOARCINST) count = 8;
		}
	}
	if (count == 0)
	{
		/* should draw a line from the node to the new pin */
		if (us_dragfromgeom != NOGEOM && us_dragfromgeom->entryisnode)
		{
			fakecoords[0] = us_dragstartx;
			fakecoords[1] = us_dragstarty;
			fakecoords[2] = dx + us_dragoffx;
			fakecoords[3] = dy + us_dragoffy;
			count = 4;
		}
	}

	/* see if the connection path is the same as before */
	samepath = 0;
	if (count == us_dragwirepathcount)
	{
		for(i=0; i<count; i++) if (fakecoords[i] != us_dragwirepath[i]) break;
		if (i >= count) samepath = 1;
	}

	/* remove previous highlighting */
	if (us_dragobject != NOGEOM)
	{
		if (us_dragshown != 0)
		{
			/* undraw the box */
			us_highl.col = 0;
			us_showpoly(us_dragpoly, us_dragwindow);
			us_dragshown = 0;
		}
		if (us_dragobject == foundgeom && us_dragportproto == foundpp && samepath != 0) return(FALSE);
		us_highlighteverywhere(us_dragobject, us_dragportproto, 0, FALSE, ALLOFF, FALSE);
	} else
	{
		if (us_dragshown != 0)
		{
			/* if it didn't actually move, go no further */
			if (us_dragx == dx && us_dragy == dy && samepath != 0 &&
				us_dragobject == foundgeom) return(FALSE);

			/* undraw the box */
			us_highl.col = 0;
			us_showpoly(us_dragpoly, us_dragwindow);
			us_dragshown = 0;
		}
	}

	/* undraw the previous wire path */
	if (us_dragwirepathcount > 0)
	{
		poly->style = OPENED;
		poly->count = us_dragwirepathcount/2;
		for(i=0; i<poly->count; i++)
		{
			poly->xv[i] = us_dragwirepath[i*2];
			poly->yv[i] = us_dragwirepath[i*2+1];
		}
		poly->desc = &us_highl;
		us_highl.col = 0;
		us_showpoly(poly, us_dragwindow);
	}

	/* advance the box */
	for(i = 0; i < us_dragpoly->count; i++)
	{
		us_dragpoly->xv[i] += dx - us_dragx;
		us_dragpoly->yv[i] += dy - us_dragy;
	}
	us_dragx = dx;
	us_dragy = dy;

	/* include any wiring path */
	for(i=0; i<count; i++)
		us_dragwirepath[i] = fakecoords[i];
	us_dragwirepathcount = count;

	/* set highlighted destination object */
	us_dragobject = foundgeom;
	us_dragportproto = foundpp;

	/* draw the previous wire path */
	if (us_dragwirepathcount > 0)
	{
		poly->style = OPENED;
		poly->count = us_dragwirepathcount/2;
		for(i=0; i<poly->count; i++)
		{
			poly->xv[i] = us_dragwirepath[i*2];
			poly->yv[i] = us_dragwirepath[i*2+1];
		}
		poly->desc = &us_highl;
		us_highl.col = HIGHLIT;
		us_showpoly(poly, us_dragwindow);
	}

	/* draw the new highlight */
	if (us_dragobject != NOGEOM)
	{
		us_highlighteverywhere(us_dragobject, us_dragportproto, 0, FALSE, HIGHLIT, FALSE);
	} else
	{
		us_highl.col = HIGHLIT;
		us_showpoly(us_dragpoly, us_dragwindow);
		us_dragshown = 1;
	}
	return(FALSE);
}

void us_createajoinedobject(GEOM **thegeom, PORTPROTO **theport)
{
	*theport = us_dragportproto;
	*thegeom = us_dragobject;
}

/* termination routine when creating along an angle */
void us_createaup(void)
{
	REGISTER INTBIG i;

	if (us_dragshown != 0)
	{
		/* undraw the box */
		us_highl.col = 0;
		us_showpoly(us_dragpoly, us_dragwindow);
	}
	if (us_dragobject != NOGEOM)
		us_highlighteverywhere(us_dragobject, us_dragportproto, 0, FALSE, ALLOFF, FALSE);

	/* undraw the previous wire path */
	if (us_dragwirepathcount > 0)
	{
		us_dragpoly->style = OPENED;
		us_dragpoly->count = us_dragwirepathcount/2;
		for(i=0; i<us_dragpoly->count; i++)
		{
			us_dragpoly->xv[i] = us_dragwirepath[i*2];
			us_dragpoly->yv[i] = us_dragwirepath[i*2+1];
		}
		us_highl.col = 0;
		us_showpoly(us_dragpoly, us_dragwindow);
	}
}

/************************* DUPLICATE and multiple MOVE *************************/

/* preinitialization routine when duplicating objects */
void us_multidraginit(INTBIG xc, INTBIG yc, GEOM **geomlist, NODEINST **nodelist,
	INTBIG total, INTBIG angle, BOOLEAN showoffset)
{
	us_multidragshowoffset = showoffset;
	us_dragox = xc;
	us_dragoy = yc;
	us_dragobjectlist = geomlist;
	us_dragnodelist = nodelist;
	us_dragnodetotal = total;
	us_dragangle = angle;
	us_dragextra = 0;
	us_cantdrag = 0;

	/* show invisible pins */
	us_multidragshowinvpin = TRUE;
}

static INTBIG us_multidragoffx, us_multidragoffy;

/* initialization routine when duplicating objects */
void us_multidragbegin(void)
{
	REGISTER INTBIG i;
	REGISTER PORTARCINST *pi;
	REGISTER ARCINST *ai;

	/* initialize polygon */
	if (us_dragpoly == NOPOLYGON) us_dragpoly = allocpolygon(4, us_tool->cluster);
	us_dragpoly->desc = &us_highl;

	(void)getxy(&us_dragx, &us_dragy);
	gridalign(&us_dragx, &us_dragy, 1, el_curlib->curnodeproto);
	us_setcursorpos(NOWINDOWFRAME, us_dragx, us_dragy);
	us_dragwindow = el_curwindowpart;

	/* preprocess all arcs that stretch to moved nodes if in verbose mode */
	if (us_dragextra != 0)
	{
		/* reset clock (temp2) and clear display nature bits (temp1) on arcs */
		for(i=0; i<us_dragnodetotal; i++)
			for(pi = us_dragnodelist[i]->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			ai = pi->conarcinst;
			ai->temp1 = ai->temp2 = 0;
		}

		/* mark number of ends that move on each arc */
		for(i=0; i<us_dragnodetotal; i++)
			for(pi = us_dragnodelist[i]->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				pi->conarcinst->temp1++;

		/* do not move arcs that are already in move list */
		for(i=0; us_dragobjectlist[i] != NOGEOM; i++)
		{
			if (us_dragobjectlist[i]->entryisnode) continue;
			ai = us_dragobjectlist[i]->entryaddr.ai;
			ai->temp1 = 0;
		}
	}

	us_multidragmostzeros = -1;
	us_multidragoffx = us_dragx-us_dragox;
	us_multidragoffy = us_dragy-us_dragoy;
	us_multidragdraw(HIGHLIT, us_multidragoffx, us_multidragoffy);
	us_dragshown = 1;
}

/* cursor advance routine when duplicating objects */
BOOLEAN us_multidragdown(INTBIG x, INTBIG y)
{
	INTBIG nx, ny, bits, angle;

	/* pan the screen if the edge was hit */
	us_panatscreenedge(&x, &y);

	/* grid align the cursor value */
	if (us_setxy(x, y) || us_dragwindow != el_curwindowpart)
	{
		(void)us_setxy(us_lastcurx, us_lastcury);
		return(FALSE);
	}
	(void)getxy(&us_lastcurx, &us_lastcury);
	gridalign(&us_lastcurx, &us_lastcury, 1, el_curlib->curnodeproto);
	us_setcursorpos(NOWINDOWFRAME, us_lastcurx, us_lastcury);
	bits = getbuckybits();
	if ((bits&CONTROLDOWN) != 0) angle = us_dragangle; else angle = 0;
	us_getslide(angle, us_dragox, us_dragoy, us_lastcurx, us_lastcury, &nx, &ny);
	us_lastcurx = nx;   us_lastcury = ny;

	/* warn if moving and can't */
	if (us_cantdrag != 0)
	{
		if (us_dragx != us_lastcurx || us_dragy != us_lastcury)
		{
			if (us_cantdrag == 1)
				us_abortcommand(_("Sorry, changes are currently disallowed"));
			us_cantdrag++;
			return(FALSE);
		}
	}

	/* if the highlighting is already being shown, erase it */
	if (us_dragshown != 0)
	{
		/* if it didn't actually move, go no further */
		if (us_dragx == us_lastcurx && us_dragy == us_lastcury) return(FALSE);

		/* undraw highlighting */
		us_multidragdraw(0, us_multidragoffx, us_multidragoffy);
	}

	us_dragx = us_lastcurx;
	us_dragy = us_lastcury;
	us_multidragoffx = us_dragx-us_dragox;
	us_multidragoffy = us_dragy-us_dragoy;
	us_multidragdraw(HIGHLIT, us_multidragoffx, us_multidragoffy);
	us_dragshown = 1;
	return(FALSE);
}

void us_multidragup(void)
{
	if (us_dragshown != 0)
		us_multidragdraw(0, us_multidragoffx, us_multidragoffy);
}

void us_multidragdraw(INTBIG col, INTBIG dx, INTBIG dy)
{
	REGISTER INTBIG i, thisend, portcount, endfixed, wid, xzeros, yzeros, savestate;
	INTBIG lx, ly, hx, hy, xc, yc, xw, yw, j;
	CHAR coords[100], xcoord[50], ycoord[50], *pt;
	XARRAY trans;
	REGISTER BOOLEAN moveotherx, moveothery;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER PORTARCINST *pi;
	REGISTER PORTPROTO *pp;
	REGISTER TECHNOLOGY *tech;

	us_highl.col = col;
	if (us_dragextra != 0) us_dragextra++;
	for(i=0; us_dragobjectlist[i] != NOGEOM; i++)
	{
		if (!us_dragobjectlist[i]->entryisnode) continue;
		ni = us_dragobjectlist[i]->entryaddr.ni;

		/* if node is an invisible pin with text, ignore the pin */
		if (ni->proto == gen_invispinprim)
		{
			for(j=0; j<ni->numvar; j++)
				if ((ni->firstvar[j].type&VDISPLAY) != 0) break;
			if (j < ni->numvar)
			{
				if (us_multidragshowinvpin)
				{
					makerot(ni, trans);
					xform((ni->lowx+ni->highx)/2, (ni->lowy+ni->highy)/2,
						&us_dragpoly->xv[0], &us_dragpoly->yv[0], trans);
					us_dragpoly->xv[0] += dx;
					us_dragpoly->yv[0] += dy;
					us_dragpoly->count = 1;
					us_dragpoly->style = CROSS;
					us_showpoly(us_dragpoly, us_dragwindow);
				}
				continue;
			}
		}

		makerot(ni, trans);
		nodesizeoffset(ni, &lx, &ly, &hx, &hy);
		maketruerectpoly(ni->lowx+lx, ni->highx-hx, ni->lowy+ly, ni->highy-hy, us_dragpoly);
		us_dragpoly->style = CLOSEDRECT;
		xformpoly(us_dragpoly, trans);
		for(j=0; j<us_dragpoly->count; j++)
		{
			us_dragpoly->xv[j] += dx;
			us_dragpoly->yv[j] += dy;
		}
		us_showpoly(us_dragpoly, us_dragwindow);

		/* draw more if in verbose mode */
		if (us_dragextra != 0)
		{
			/* if only 1 node selected, show its ports */
			if (us_dragnodetotal == 1)
			{
				portcount = 0;
				for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				{
					/* compute the port bounds */
					shapeportpoly(ni, pp, us_dragpoly, FALSE);

					/* see if the polygon is a single point */
					for(j=1; j<us_dragpoly->count; j++)
						if (us_dragpoly->xv[j] != us_dragpoly->xv[j-1] ||
							us_dragpoly->yv[j] != us_dragpoly->yv[j-1]) break;
					if (j < us_dragpoly->count)
					{
						/* not a single point, draw its outline */
						switch (us_dragpoly->style)
						{
							case FILLEDRECT: us_dragpoly->style = CLOSEDRECT;  break;
							case FILLED:     us_dragpoly->style = CLOSED;      break;
							case DISC:       us_dragpoly->style = CIRCLE;      break;
							case OPENEDT1:
							case OPENEDT2:
							case OPENEDT3:
							case OPENEDO1:   us_dragpoly->style = OPENED;      break;
						}
					} else
					{
						/* single point port: make it a cross */
						us_dragpoly->count = 1;
						us_dragpoly->style = CROSS;
					}
					for(j=0; j<us_dragpoly->count; j++)
					{
						us_dragpoly->xv[j] += dx;
						us_dragpoly->yv[j] += dy;
					}

					/* draw the port */
					us_showpoly(us_dragpoly, us_dragwindow);

					/* stop if interrupted */
					portcount++;
					if ((portcount%20) == 0)
					{
						if (stopping(STOPREASONDISPLAY)) break;
					}
				}
			}

			/* rubber-band arcs to this node */
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			{
				ai = pi->conarcinst;

				/* ignore if not drawing this arc or already drawn */
				if (ai->temp1 == 0 || ai->temp2 == us_dragextra) continue;
				ai->temp2 = us_dragextra;

				/* get original endpoint of arc */
				us_dragpoly->xv[0] = ai->end[0].xpos;
				us_dragpoly->yv[0] = ai->end[0].ypos;
				us_dragpoly->xv[1] = ai->end[1].xpos;
				us_dragpoly->yv[1] = ai->end[1].ypos;
				us_dragpoly->count = 2;
				us_dragpoly->style = OPENED;

				/* offset this end by amount node moves */
				if (ai->end[0].portarcinst == pi) thisend = 0; else thisend = 1;
				us_dragpoly->xv[thisend] += dx;
				us_dragpoly->yv[thisend] += dy;

				/* offset other end if both connect to moved nodes or arc rigid */
				endfixed = 0;
				if (ai->temp1 >= 2 || (ai->userbits&FIXED) != 0) endfixed = 1;

				moveotherx = moveothery = FALSE;
				if (endfixed != 0)
				{
					 moveotherx = moveothery = TRUE;
				} else if ((ai->userbits&FIXANG) != 0)
				{
					if (ai->end[0].xpos != ai->end[1].xpos && ai->end[0].ypos != ai->end[1].ypos)
					{
						moveotherx = moveothery = TRUE;
					} else
					{
						if (ai->end[0].xpos == ai->end[1].xpos) moveotherx = TRUE;
						if (ai->end[0].ypos == ai->end[1].ypos) moveothery = TRUE;
					}
				}
				if (moveotherx) us_dragpoly->xv[1-thisend] += dx;
				if (moveothery) us_dragpoly->yv[1-thisend] += dy;

				/* draw line */
				us_showpoly(us_dragpoly, us_dragwindow);
			}
		}
	}

	for(i=0; us_dragobjectlist[i] != NOGEOM; i++)
	{
		if (us_dragobjectlist[i]->entryisnode) continue;
		ai = us_dragobjectlist[i]->entryaddr.ai;
		wid = ai->width - arcwidthoffset(ai);
		makearcpoly(ai->length, wid, ai, us_dragpoly, CLOSED);
		for(j=0; j<us_dragpoly->count; j++)
		{
			us_dragpoly->xv[j] += dx;
			us_dragpoly->yv[j] += dy;
		}
		us_showpoly(us_dragpoly, us_dragwindow);
	}

	/* update selected text, too */
	for(i=0; i<us_dragtextcount; i++)
	{
		(void)us_makehighlight(us_dragtexts[i], &us_draghigh);
		if ((us_draghigh.status&HIGHTYPE) != HIGHTEXT) continue;

		/* get object and extent */
		us_gethighaddrtype(&us_draghigh, &us_dragaddr, &us_dragtype);
		tech = us_getobjectinfo(us_dragaddr, us_dragtype, &lx, &hx, &ly, &hy);

		/* determine number of lines of text and text size */
		us_gethightextsize(&us_draghigh, &xw, &yw, el_curwindowpart);
		us_gethightextcenter(&us_draghigh, &xc, &yc, &j);
		us_dragpoly->style = j;

#if 0		/* clip range of text descriptor */
		lambda = el_curlib->lambda[tech->techindex];
		xcur = (us_lastcurx - us_dragx) * 4 / lambda;
		us_lastcurx = us_lastcurx - us_dragx + us_dragoffx;
		if (xcur > 1023)
			us_lastcurx = 1023 * lambda / 4 + us_dragoffx; else
				if (xcur < -1023)
					us_lastcurx = -1023 * lambda / 4 + us_dragoffx;
		ycur = (us_lastcury - us_dragy) * 4 / lambda;
		us_lastcury = us_lastcury - us_dragy + us_dragoffy;
		if (ycur > 1023)
			us_lastcury = 1023 * lambda / 4 + us_dragoffy; else
				if (ycur < -1023)
					us_lastcury = -1023 * lambda / 4 + us_dragoffy;
#endif

		/* draw the descriptor */
		us_buildtexthighpoly(lx, hx, ly, hy, xc+dx, yc+dy,
			xw, yw, us_dragpoly->style, us_dragpoly);

		/* draw the new box */
		us_showpoly(us_dragpoly, us_dragwindow);
	}

	/* show the distance moved */
	if (us_multidragshowoffset)
	{
		estrcpy(xcoord, latoa(dx, 0));
		for(pt = xcoord; *pt != 0; pt++) if (*pt == '.') break;
		if (*pt == 0) xzeros = -1; else xzeros = estrlen(pt) - 1;
		estrcpy(ycoord, latoa(dy, 0));
		for(pt = ycoord; *pt != 0; pt++) if (*pt == '.') break;
		if (*pt == 0) yzeros = -1; else yzeros = estrlen(pt) - 1;
		if (xzeros > us_multidragmostzeros) us_multidragmostzeros = xzeros;
		if (yzeros > us_multidragmostzeros) us_multidragmostzeros = yzeros;
		if (xzeros < us_multidragmostzeros)
		{
			if (xzeros < 0) { estrcat(xcoord, x_(".")); xzeros++; }
			for(i=xzeros; i<us_multidragmostzeros; i++) estrcat(xcoord, x_("0"));
		}
		if (yzeros < us_multidragmostzeros)
		{
			if (yzeros < 0) { estrcat(ycoord, x_(".")); yzeros++; }
			for(i=yzeros; i<us_multidragmostzeros; i++) estrcat(ycoord, x_("0"));
		}

		esnprintf(coords, 100, x_("(%s,%s)"), xcoord, ycoord);
		TDCLEAR(us_dragpoly->textdescript);
		TDSETSIZE(us_dragpoly->textdescript, TXTSETPOINTS(20));
		us_dragpoly->style = TEXTCENT;
		us_dragpoly->count = 1;
		us_dragpoly->xv[0] = (us_dragwindow->screenlx + us_dragwindow->screenhx) / 2;
		us_dragpoly->yv[0] = (us_dragwindow->screenly + us_dragwindow->screenhy) / 2;
		us_dragpoly->string = coords;
		savestate = us_dragwindow->state;
		us_dragwindow->state &= ~INPLACEEDIT;
		us_showpoly(us_dragpoly, us_dragwindow);
		us_dragwindow->state = savestate;
	}
}

/************************* DISTANCE TRACKING *************************/

void us_distanceinit(void)
{
	us_measureshown = 0;
	us_dragwindow = el_curwindowpart;
}

BOOLEAN us_distancedown(INTBIG x, INTBIG y)
{
	REGISTER INTBIG dist, lastshown;

	/* initialize polygon */
	if (us_dragpoly == NOPOLYGON) us_dragpoly = allocpolygon(4, us_tool->cluster);
	us_dragpoly->desc = &us_highl;

	/* pan the screen if the edge was hit */
	lastshown = us_measureshown;
	us_panatscreenedge(&x, &y);
	if (lastshown != us_measureshown)
	{
		us_highl.col = HIGHLIT;
		us_drawdistance();
		us_measureshown = 1;
	}

	/* grid align the cursor value */
	if (us_setxy(x, y)) return(FALSE);
	if ((us_dragwindow->state&WINDOWTYPE) != DISPWINDOW ||
		us_dragwindow->curnodeproto == NONODEPROTO) return(FALSE);
	us_dragnodeproto = us_dragwindow->curnodeproto;
	(void)getxy(&us_dragx, &us_dragy);
	gridalign(&us_dragx, &us_dragy, 1, us_dragnodeproto);
	us_setcursorpos(NOWINDOWFRAME, us_dragx, us_dragy);

	if (us_measureshown == 0)
	{
		us_firstmeasurex = us_dragx;   us_firstmeasurey = us_dragy;
	} else
	{
		/* if it didn't actually move, go no further */
		if (us_dragx == us_lastcurx && us_dragy == us_lastcury) return(FALSE);

		/* undraw the distance */
		us_highl.col = 0;
		us_drawdistance();
	}

	/* remember this measured distance */
	us_lastmeasurex = us_dragx - us_firstmeasurex;
	us_lastmeasurey = us_dragy - us_firstmeasurey;
	us_validmesaure = TRUE;

	/* draw crosses at the end */
	us_highl.col = HIGHLIT;
	us_lastcurx = us_dragx;   us_lastcury = us_dragy;
	TDCLEAR(us_dragpoly->textdescript);
	TDSETSIZE(us_dragpoly->textdescript, TXTSETPOINTS(12));
	us_dragpoly->tech = el_curtech;
	dist = computedistance(us_firstmeasurex, us_firstmeasurey, us_dragx, us_dragy);
	(void)estrcpy(us_dragmessage, _("Distance: "));
	(void)estrcat(us_dragmessage, latoa(dist, 0));
	(void)estrcat(us_dragmessage, x_(" (dX="));
	(void)estrcat(us_dragmessage, latoa(us_dragx - us_firstmeasurex, 0));
	(void)estrcat(us_dragmessage, x_(" dY="));
	(void)estrcat(us_dragmessage, latoa(us_dragy - us_firstmeasurey, 0));
	(void)estrcat(us_dragmessage, x_(")"));
	us_dragpoly->string = us_dragmessage;
	us_drawdistance();
	us_measureshown = 1;
	return(FALSE);
}

void us_drawdistance(void)
{
	REGISTER WINDOWPART *w;
	INTBIG fx, fy, tx, ty;

	us_dragpoly->count = 1;
	us_dragpoly->style = TEXTBOTLEFT;
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		if (w->curnodeproto != us_dragnodeproto) continue;
		fx = us_firstmeasurex;   fy = us_firstmeasurey;
		tx = us_lastcurx;        ty = us_lastcury;
		if (clipline(&fx, &fy, &tx, &ty, w->screenlx, w->screenhx,
			w->screenly, w->screenhy)) continue;
		us_dragpoly->xv[0] = (fx+tx) / 2;
		us_dragpoly->yv[0] = (fy+ty) / 2;
		us_showpoly(us_dragpoly, w);
	}

	/* draw the line */
	us_dragpoly->xv[0] = us_firstmeasurex;  us_dragpoly->yv[0] = us_firstmeasurey;
	us_dragpoly->xv[1] = us_lastcurx;       us_dragpoly->yv[1] = us_lastcury;
	us_dragpoly->count = 2;
	us_dragpoly->style = OPENED;
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		if (w->curnodeproto == us_dragnodeproto)
			us_showpoly(us_dragpoly, w);

	/* draw crosses at the end */
	us_dragpoly->xv[0] = us_firstmeasurex;     us_dragpoly->yv[0] = us_firstmeasurey;
	us_dragpoly->count = 1;
	us_dragpoly->style = CROSS;
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		if (w->curnodeproto == us_dragnodeproto)
			us_showpoly(us_dragpoly, w);
	us_dragpoly->xv[0] = us_lastcurx;   us_dragpoly->yv[0] = us_lastcury;
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		if (w->curnodeproto == us_dragnodeproto)
			us_showpoly(us_dragpoly, w);
}

void us_distanceup(void)
{
	if (us_measureshown != 0)
	{
		/* leave the results in the messages window */
		ttyputmsg(x_("%s"), us_dragpoly->string);
	}
}

/************************* SLIDER THUMB TRACKING *************************/

/*
 * initialization method for tracking the horizontal thumb.  The cursor X coordinate
 * is "x" and the window is "w".  The horizontal slider's top side is "hy" and it
 * runs horizontally from "lx" to "hx" (including arrows).  The routine "callback"
 * is invoked with the difference each time.
 */
void us_hthumbbegin(INTBIG x, WINDOWPART *w, INTBIG hy, INTBIG lx, INTBIG hx,
	void (*callback)(INTBIG))
{
	/* initialize polygon */
	if (us_dragpoly == NOPOLYGON) us_dragpoly = allocpolygon(4, us_tool->cluster);
	us_dragpoly->desc = &us_highl;

	us_dragshown = 0;
	us_dragwindow = w;
	us_initthumbcoord = x;
	us_trackingcallback = callback;

	us_trackww.screenlx = us_trackww.uselx = lx;
	us_trackww.screenhx = us_trackww.usehx = hx;
	us_trackww.screenly = us_trackww.usely = hy-DISPLAYSLIDERSIZE;
	us_trackww.screenhy = us_trackww.usehy = hy;
	us_trackww.frame = w->frame;
	us_trackww.state = DISPWINDOW;
	computewindowscale(&us_trackww);
}

/* tracking method for the horizontal thumb */
BOOLEAN us_hthumbdown(INTBIG x, INTBIG y)
{
	REGISTER INTBIG dx;

	if (us_dragshown != 0)
	{
		/* if it didn't actually move, go no further */
		if (x == us_lastcurx) return(FALSE);

		/* undraw the outline */
		us_highl.col = 0;
		us_dragpoly->xv[0] = us_dragwindow->thumblx + (us_lastcurx-us_initthumbcoord)+1;
		us_dragpoly->yv[0] = us_trackww.usehy-3;
		us_dragpoly->xv[1] = us_dragwindow->thumbhx + (us_lastcurx-us_initthumbcoord)-1;
		us_dragpoly->yv[1] = us_trackww.usehy-DISPLAYSLIDERSIZE+1;
		us_dragpoly->count = 2;
		us_dragpoly->style = FILLEDRECT;
		us_showpoly(us_dragpoly, &us_trackww);
	}

	/* draw the outline */
	us_highl.col = HIGHLIT;
	if (us_dragwindow->thumblx + (x-us_initthumbcoord) <= us_trackww.uselx+DISPLAYSLIDERSIZE)
		 x = us_trackww.uselx+DISPLAYSLIDERSIZE+1 - us_dragwindow->thumblx + us_initthumbcoord;
	if (us_dragwindow->thumbhx + (x-us_initthumbcoord) >= us_trackww.usehx-DISPLAYSLIDERSIZE)
		 x = us_trackww.usehx-DISPLAYSLIDERSIZE-1 - us_dragwindow->thumbhx + us_initthumbcoord;
	us_dragpoly->xv[0] = us_dragwindow->thumblx + (x-us_initthumbcoord)+1;
	us_dragpoly->yv[0] = us_trackww.usehy-3;
	us_dragpoly->xv[1] = us_dragwindow->thumbhx + (x-us_initthumbcoord)-1;
	us_dragpoly->yv[1] = us_trackww.usehy-DISPLAYSLIDERSIZE+1;
	us_dragpoly->count = 2;
	us_dragpoly->style = FILLEDRECT;
	us_showpoly(us_dragpoly, &us_trackww);

	us_dragshown = 1;
	us_lastcurx = x;

	if (us_trackingcallback != 0)
	{
		dx = us_lastcurx-us_initthumbcoord;
		us_initthumbcoord = us_lastcurx;
		(*us_trackingcallback)(dx);
	}
	return(FALSE);
}

void us_hthumbtrackingcallback(INTBIG delta)
{
	REGISTER NODEPROTO *np;
	REGISTER INTBIG cellsizex, screensizex, thumbareax, dsx;

	np = us_dragwindow->curnodeproto;
	if (np == NONODEPROTO) return;
	cellsizex = np->highx - np->lowx;
	screensizex = us_dragwindow->screenhx - us_dragwindow->screenlx;
	thumbareax = (us_dragwindow->usehx - us_dragwindow->uselx - DISPLAYSLIDERSIZE*2-4) / 2;
	if (cellsizex <= screensizex)
	{
		dsx = delta * (us_dragwindow->screenhx-us_dragwindow->screenlx) / thumbareax;
	} else
	{
		dsx = delta * (np->highx-np->lowx) / thumbareax;
	}
	us_slideleft(dsx);
	us_endbatch();
}

/* completion method for tracking the horozintal thumb in a regular edit window */
void us_hthumbdone(void)
{
	if (us_dragshown != 0)
	{
		/* undraw the outline */
		us_highl.col = 0;
		us_dragpoly->xv[0] = us_dragwindow->thumblx + (us_lastcurx-us_initthumbcoord)+1;
		us_dragpoly->yv[0] = us_trackww.usehy-3;
		us_dragpoly->xv[1] = us_dragwindow->thumbhx + (us_lastcurx-us_initthumbcoord)-1;
		us_dragpoly->yv[1] = us_trackww.usehy-DISPLAYSLIDERSIZE+1;
		us_dragpoly->count = 2;
		us_dragpoly->style = FILLEDRECT;
		us_showpoly(us_dragpoly, &us_trackww);
	}
}

/*
 * initialization method for tracking the vertical thumb.  The cursor Y coordinate
 * is "y" and the window is "w".  The vertical slider's left side is "hx" and it
 * runs vertically from "ly" to "hy" (including arrows).  The total number of
 * lines of text is "totallines" and the slider is on the left side if "onleft"
 * is nonzero.
 */
void us_vthumbbegin(INTBIG y, WINDOWPART *w, INTBIG hx, INTBIG ly, INTBIG hy,
	BOOLEAN onleft, void (*callback)(INTBIG))
{
	/* initialize polygon */
	if (us_dragpoly == NOPOLYGON) us_dragpoly = allocpolygon(4, us_tool->cluster);
	us_dragpoly->desc = &us_highl;

	us_dragshown = 0;
	us_dragwindow = w;
	us_initthumbcoord = y;
	us_dragpoint = -1;
	us_trackingcallback = callback;

	if (onleft != 0)
	{
		/* slider is on the left (simulation window) */
		us_trackww.screenlx = us_trackww.uselx = hx;
		us_trackww.screenhx = us_trackww.usehx = w->usehx;
		us_dragoffx = hx-2;
	} else
	{
		/* slider is on the right (edit, text edit, and explorer window) */
		us_trackww.screenlx = us_trackww.uselx = w->uselx;
		us_trackww.screenhx = us_trackww.usehx = hx+DISPLAYSLIDERSIZE;
		us_dragoffx = hx;
	}
	us_trackww.screenly = ly;
	us_trackww.usely = ly;
	us_trackww.screenhy = hy;
	us_trackww.usehy = hy;
	us_trackww.frame = w->frame;
	us_trackww.state = DISPWINDOW;
	computewindowscale(&us_trackww);
}

/* tracking method for the vertical thumb */
BOOLEAN us_vthumbdown(INTBIG x, INTBIG y)
{
	REGISTER INTBIG dy;

	if (us_dragshown != 0)
	{
		/* if it didn't actually move, go no further */
		if (y == us_lastcury) return(FALSE);

		/* undraw the outline */
		us_highl.col = 0;
		us_dragpoly->xv[0] = us_dragoffx+3;
		us_dragpoly->yv[0] = us_dragwindow->thumbly + (us_lastcury-us_initthumbcoord)+1;
		us_dragpoly->yv[1] = us_dragwindow->thumbhy + (us_lastcury-us_initthumbcoord)-1;
		us_dragpoly->xv[1] = us_dragoffx+DISPLAYSLIDERSIZE-1;
		us_dragpoly->count = 2;
		us_dragpoly->style = FILLEDRECT;
		us_showpoly(us_dragpoly, &us_trackww);
	}

	/* draw the outline */
	us_highl.col = HIGHLIT;
	if (us_dragwindow->thumbly + (y-us_initthumbcoord) <= us_trackww.usely+DISPLAYSLIDERSIZE)
		 y = us_trackww.usely+DISPLAYSLIDERSIZE+1 - us_dragwindow->thumbly + us_initthumbcoord;
	if (us_dragwindow->thumbhy + (y-us_initthumbcoord) >= us_trackww.usehy-DISPLAYSLIDERSIZE)
		 y = us_trackww.usehy-DISPLAYSLIDERSIZE-1 - us_dragwindow->thumbhy + us_initthumbcoord;
	us_dragpoly->xv[0] = us_dragoffx+3;
	us_dragpoly->yv[0] = us_dragwindow->thumbly + (y-us_initthumbcoord)+1;
	us_dragpoly->yv[1] = us_dragwindow->thumbhy + (y-us_initthumbcoord)-1;
	us_dragpoly->xv[1] = us_dragoffx+DISPLAYSLIDERSIZE-1;
	us_dragpoly->count = 2;
	us_dragpoly->style = FILLEDRECT;
	us_showpoly(us_dragpoly, &us_trackww);

	us_dragshown = 1;
	us_lastcury = y;

	if (us_trackingcallback != 0)
	{
		dy = us_lastcury-us_initthumbcoord;
		us_initthumbcoord = us_lastcury;
		(*us_trackingcallback)(dy);
	}
	return(FALSE);
}

void us_vthumbtrackingcallback(INTBIG delta)
{
	REGISTER NODEPROTO *np;
	REGISTER INTBIG cellsizey, screensizey, thumbareay, dsy;

	/* adjust the screen */
	np = us_dragwindow->curnodeproto;
	if (np == NONODEPROTO) return;
	cellsizey = np->highy - np->lowy;
	screensizey = us_dragwindow->screenhy - us_dragwindow->screenly;
	thumbareay = (us_dragwindow->usehy - us_dragwindow->usely - DISPLAYSLIDERSIZE*2-4) / 2;
	if (cellsizey <= screensizey)
	{
		dsy = delta * (us_dragwindow->screenhy-us_dragwindow->screenly) / thumbareay;
	} else
	{
		dsy = delta * (np->highy-np->lowy) / thumbareay;
	}
	us_slideup(-dsy);
	us_endbatch();
}

/* completion method for tracking the vertical thumb in regular edit window */
void us_vthumbdone(void)
{
	if (us_dragshown != 0)
	{
		/* undraw the outline */
		us_highl.col = 0;
		us_dragpoly->xv[0] = us_dragoffx+3;
		us_dragpoly->yv[0] = us_dragwindow->thumbly + (us_lastcury-us_initthumbcoord)+1;
		us_dragpoly->yv[1] = us_dragwindow->thumbhy + (us_lastcury-us_initthumbcoord)-1;
		us_dragpoly->xv[1] = us_dragoffx+DISPLAYSLIDERSIZE-1;
		us_dragpoly->count = 2;
		us_dragpoly->style = FILLEDRECT;
		us_showpoly(us_dragpoly, &us_trackww);
	}
}

/* initialization method for tracking the arrows on sliders */
void us_arrowclickbegin(WINDOWPART *w, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy, INTBIG amount)
{
	us_dragwindow = w;
	us_arrowlx = lx;
	us_arrowhx = hx;
	us_arrowly = ly;
	us_arrowhy = hy;
	us_arrowamount = amount;
}

BOOLEAN us_varrowdown(INTBIG x, INTBIG y)
{
	if (x < us_arrowlx || x > us_arrowhx) return(FALSE);
	if (y < us_arrowly || y > us_arrowhy) return(FALSE);
	if (y >= us_dragwindow->thumbly && y <= us_dragwindow->thumbhy) return(FALSE);
	us_slideup(us_arrowamount);
	us_endbatch();
	return(FALSE);
}

BOOLEAN us_harrowdown(INTBIG x, INTBIG y)
{
	if (x < us_arrowlx || x > us_arrowhx) return(FALSE);
	if (y < us_arrowly || y > us_arrowhy) return(FALSE);
	if (x >= us_dragwindow->thumblx && x <= us_dragwindow->thumbhx) return(FALSE);
	us_slideleft(us_arrowamount);
	us_endbatch();
	return(FALSE);
}

/************************* WINDOW PARTITION DIVIDER TRACKING *************************/

void us_vpartdividerbegin(INTBIG x, INTBIG ly, INTBIG hy, WINDOWFRAME *wf)
{
	/* initialize polygon */
	if (us_dragpoly == NOPOLYGON) us_dragpoly = allocpolygon(4, us_tool->cluster);
	us_dragpoly->desc = &us_highl;

	us_dragshown = 0;
	us_initthumbcoord = x;
	us_draglowval = ly;
	us_draghighval = hy;

	us_trackww.screenlx = us_trackww.uselx = 0;
	us_trackww.screenhx = us_trackww.usehx = wf->swid;
	us_trackww.screenly = us_trackww.usely = 0;
	us_trackww.screenhy = us_trackww.usehy = wf->shei;
	us_trackww.frame = wf;
	us_trackww.state = DISPWINDOW;
	computewindowscale(&us_trackww);
}

BOOLEAN us_vpartdividerdown(INTBIG x, INTBIG y)
{
	if (us_dragshown != 0)
	{
		/* if it didn't actually move, go no further */
		if (x == us_lastcurx) return(FALSE);

		/* undraw the line */
		us_highl.col = 0;
		us_dragpoly->xv[0] = us_lastcurx;
		us_dragpoly->yv[0] = us_draglowval;
		us_dragpoly->xv[1] = us_lastcurx;
		us_dragpoly->yv[1] = us_draghighval;
		us_dragpoly->count = 2;
		us_dragpoly->style = VECTORS;
		us_showpoly(us_dragpoly, &us_trackww);
	}

	us_lastcurx = x;
	if (us_lastcurx >= us_trackww.usehx) us_lastcurx = us_trackww.usehx-1;
	if (us_lastcurx < us_trackww.uselx) us_lastcurx = us_trackww.uselx;

	/* draw the outline */
	us_highl.col = HIGHLIT;
	us_dragpoly->xv[0] = us_lastcurx;
	us_dragpoly->yv[0] = us_draglowval;
	us_dragpoly->xv[1] = us_lastcurx;
	us_dragpoly->yv[1] = us_draghighval;
	us_dragpoly->count = 2;
	us_dragpoly->style = VECTORS;
	us_showpoly(us_dragpoly, &us_trackww);

	us_dragshown = 1;
	return(FALSE);
}

void us_vpartdividerdone(void)
{
	INTBIG lx, hx, ly, hy;
	REGISTER WINDOWPART *w;

	if (us_dragshown != 0)
	{
		/* undraw the outline */
		us_highl.col = 0;
		us_dragpoly->xv[0] = us_lastcurx;
		us_dragpoly->yv[0] = us_draglowval;
		us_dragpoly->xv[1] = us_lastcurx;
		us_dragpoly->yv[1] = us_draghighval;
		us_dragpoly->count = 2;
		us_dragpoly->style = VECTORS;
		us_showpoly(us_dragpoly, &us_trackww);

		/* adjust the screen */
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if (w->frame != us_trackww.frame) continue;
			if (estrcmp(w->location, x_("entire")) == 0) continue;

			us_gettruewindowbounds(w, &lx, &hx, &ly, &hy);
			lx--;   hx++;
			ly--;   hy++;
			if (us_initthumbcoord >= lx-2 && us_initthumbcoord <= lx+2)
			{
				w->hratio = ((hx - us_lastcurx) * w->hratio + (hx - lx)/2) / (hx - lx);
				if (w->hratio > 95)
				{
					el_curwindowpart = w;
					us_killcurrentwindow(FALSE);
					return;
				}
				if (w->hratio < 5)
				{
					el_curwindowpart = w;
					us_killcurrentwindow(TRUE);
					return;
				}
			}
			if (us_initthumbcoord >= hx-2 && us_initthumbcoord <= hx+2)
			{
				w->hratio = ((us_lastcurx - lx) * w->hratio + (hx - lx)/2) / (hx - lx);
				if (w->hratio > 95)
				{
					el_curwindowpart = w;
					us_killcurrentwindow(FALSE);
					return;
				}
				if (w->hratio < 5)
				{
					el_curwindowpart = w;
					us_killcurrentwindow(TRUE);
					return;
				}
			}
		}

		/* remember the explorer percentage */
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if ((w->state&WINDOWTYPE) != EXPLORERWINDOW) continue;
			us_explorerratio = w->hratio;
		}
		us_beginchanges();
		us_drawmenu(1, us_trackww.frame);
		us_endchanges(NOWINDOWPART);
		us_state |= HIGHLIGHTSET;
		us_showallhighlight();
	}
}

void us_hpartdividerbegin(INTBIG y, INTBIG lx, INTBIG hx, WINDOWFRAME *wf)
{
	/* initialize polygon */
	if (us_dragpoly == NOPOLYGON) us_dragpoly = allocpolygon(4, us_tool->cluster);
	us_dragpoly->desc = &us_highl;

	us_dragshown = 0;
	us_initthumbcoord = y;
	us_draglowval = lx;
	us_draghighval = hx;

	us_trackww.screenlx = us_trackww.uselx = 0;
	us_trackww.screenhx = us_trackww.usehx = wf->swid;
	us_trackww.screenly = us_trackww.usely = 0;
	us_trackww.screenhy = us_trackww.usehy = wf->shei;
	us_trackww.frame = wf;
	us_trackww.state = DISPWINDOW;
	computewindowscale(&us_trackww);
}

BOOLEAN us_hpartdividerdown(INTBIG x, INTBIG y)
{
	if (us_dragshown != 0)
	{
		/* if it didn't actually move, go no further */
		if (y == us_lastcury) return(FALSE);

		/* undraw the line */
		us_highl.col = 0;
		us_dragpoly->xv[0] = us_draglowval;
		us_dragpoly->yv[0] = us_lastcury;
		us_dragpoly->xv[1] = us_draghighval;
		us_dragpoly->yv[1] = us_lastcury;
		us_dragpoly->count = 2;
		us_dragpoly->style = VECTORS;
		us_showpoly(us_dragpoly, &us_trackww);
	}

	us_lastcury = y;
	if (us_lastcury >= us_trackww.usehy) us_lastcury = us_trackww.usehy-1;
	if (us_lastcury < us_trackww.usely) us_lastcury = us_trackww.usely;

	/* draw the outline */
	us_highl.col = HIGHLIT;
	us_dragpoly->xv[0] = us_draglowval;
	us_dragpoly->yv[0] = us_lastcury;
	us_dragpoly->xv[1] = us_draghighval;
	us_dragpoly->yv[1] = us_lastcury;
	us_dragpoly->count = 2;
	us_dragpoly->style = VECTORS;
	us_showpoly(us_dragpoly, &us_trackww);

	us_dragshown = 1;
	return(FALSE);
}

void us_hpartdividerdone(void)
{
	INTBIG lx, hx, ly, hy;
	REGISTER WINDOWPART *w;

	if (us_dragshown != 0)
	{
		/* undraw the outline */
		us_highl.col = 0;
		us_dragpoly->xv[0] = us_draglowval;
		us_dragpoly->yv[0] = us_lastcury;
		us_dragpoly->xv[1] = us_draghighval;
		us_dragpoly->yv[1] = us_lastcury;
		us_dragpoly->count = 2;
		us_dragpoly->style = VECTORS;
		us_showpoly(us_dragpoly, &us_trackww);

		/* adjust the screen */
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if (w->frame != us_trackww.frame) continue;
			if (estrcmp(w->location, x_("entire")) == 0) continue;

			us_gettruewindowbounds(w, &lx, &hx, &ly, &hy);
			lx--;   hx++;
			ly--;   hy++;
			if (us_initthumbcoord >= ly-2 && us_initthumbcoord <= ly+2)
			{
				w->vratio = ((hy - us_lastcury) * w->vratio + (hy - ly)/2) / (hy - ly);
				if (w->vratio > 95)
				{
					el_curwindowpart = w;
					us_killcurrentwindow(FALSE);
					return;
				}
				if (w->vratio < 5)
				{
					el_curwindowpart = w;
					us_killcurrentwindow(TRUE);
					return;
				}
			}
			if (us_initthumbcoord >= hy-2 && us_initthumbcoord <= hy+2)
			{
				w->vratio = ((us_lastcury - ly) * w->vratio + (hy - ly)/2) / (hy - ly);
				if (w->vratio > 95)
				{
					el_curwindowpart = w;
					us_killcurrentwindow(FALSE);
					return;
				}
				if (w->vratio < 5)
				{
					el_curwindowpart = w;
					us_killcurrentwindow(TRUE);
					return;
				}
			}
		}
		us_beginchanges();
		us_drawmenu(1, us_trackww.frame);
		us_endchanges(NOWINDOWPART);
		us_state |= HIGHLIGHTSET;
		us_showallhighlight();
	}
}

/************************* MISCELLANEOUS *************************/

/* cursor advance routine when stretching the highlight object */
BOOLEAN us_stretchdown(INTBIG x, INTBIG y)
{
	/* pan the screen if the edge was hit */
	us_panatscreenedge(&x, &y);

	/* grid align the cursor value */
	if (us_setxy(x, y) || us_dragwindow != el_curwindowpart)
	{
		(void)us_setxy(us_lastcurx, us_lastcury);
		return(FALSE);
	}
	(void)getxy(&us_lastcurx, &us_lastcury);
	us_setcursorpos(NOWINDOWFRAME, us_lastcurx, us_lastcury);

	/* if the box is already being shown, erase it */
	if (us_dragshown != 0)
	{
		/* if it didn't actually move, go no further */
		if (us_dragx == us_lastcurx && us_dragy == us_lastcury) return(FALSE);

		/* undraw the box */
		us_invertstretch();
	}

	/* advance the box */
	us_dragx = us_lastcurx;
	us_dragy = us_lastcury;

	/* draw the new box */
	us_invertstretch();
	us_dragshown = 1;
	return(FALSE);
}

void us_invertstretch(void)
{
	us_wanttoinvert(us_dragox, us_dragoy, us_dragox, us_dragy,  us_dragwindow);
	us_wanttoinvert(us_dragox, us_dragy,  us_dragx,  us_dragy,  us_dragwindow);
	us_wanttoinvert(us_dragx,  us_dragy,  us_dragx,  us_dragoy, us_dragwindow);
	us_wanttoinvert(us_dragx,  us_dragoy, us_dragox, us_dragoy, us_dragwindow);
}

/*
 * routine to clip and possibly draw a line from (fx,fy) to (tx,ty) in
 * window "w" with description "desc", texture "texture"
 */
void us_wanttoinvert(INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty, WINDOWPART *w)
{
	if ((w->state&INPLACEEDIT) != 0)
	{
		xform(fx, fy, &fx, &fy, w->outofcell);
		xform(tx, ty, &tx, &ty, w->outofcell);
	}
	fx = applyxscale(w, fx-w->screenlx) + w->uselx;
	fy = applyyscale(w, fy-w->screenly) + w->usely;
	tx = applyxscale(w, tx-w->screenlx) + w->uselx;
	ty = applyyscale(w, ty-w->screenly) + w->usely;
	if (clipline(&fx, &fy, &tx, &ty, w->uselx, w->usehx, w->usely, w->usehy)) return;
	screeninvertline(w, fx, fy, tx, ty);
}

/* cursor advance routine when creating or moving an object */
BOOLEAN us_dragdown(INTBIG x, INTBIG y)
{
	REGISTER INTBIG i;

	if (el_pleasestop != 0) return(TRUE);

	/* pan the screen if the edge was hit */
	us_panatscreenedge(&x, &y);

	/* grid align the cursor value */
	if (us_setxy(x, y) || us_dragwindow != el_curwindowpart)
	{
		(void)us_setxy(us_lastcurx, us_lastcury);
		return(FALSE);
	}
	(void)getxy(&us_lastcurx, &us_lastcury);
	gridalign(&us_lastcurx, &us_lastcury, 1, us_dragwindow->curnodeproto);
	us_setcursorpos(NOWINDOWFRAME, us_lastcurx, us_lastcury);

	/* if the box is already being shown, erase it */
	if (us_dragshown != 0)
	{
		/* if it didn't actually move, go no further */
		if (us_dragx == us_lastcurx && us_dragy == us_lastcury) return(FALSE);

		/* undraw the box */
		us_highl.col = 0;
		us_showpoly(us_dragpoly, us_dragwindow);
	}

	/* advance the box */
	for(i = 0; i < us_dragpoly->count; i++)
	{
		us_dragpoly->xv[i] += us_lastcurx - us_dragx;
		us_dragpoly->yv[i] += us_lastcury - us_dragy;
	}
	us_dragx = us_lastcurx;
	us_dragy = us_lastcury;

	/* draw the new box */
	us_highl.col = HIGHLIT;
	us_showpoly(us_dragpoly, us_dragwindow);
	us_dragshown = 1;
	return(FALSE);
}

/* termination routine when stretching an area */
void us_invertdragup(void)
{
	if (us_dragshown != 0) us_invertstretch();
}

/* termination routine when creating or moving an object */
void us_dragup(void)
{
	if (us_dragshown != 0)
	{
		/* undraw the box */
		us_highl.col = 0;
		us_showpoly(us_dragpoly, us_dragwindow);
	}
}

/*
 * Routine to pan the screen if the cursor at (x,y) hits the edge of window "us_dragwindow".
 * Adjusts the coordinates to be on the screen.
 */
void us_panatscreenedge(INTBIG *x, INTBIG *y)
{
	REGISTER UINTBIG now;
	REGISTER BOOLEAN didpan;

	/* pan screen if drag went over edge */
	didpan = FALSE;
	if (*x >= us_dragwindow->usehx)
	{
		*x = us_dragwindow->usehx;
		now = ticktime();
		if (now - us_beforepantime >= MINSLIDEDELAY)
		{
			us_beforepantime = now;
			us_slideleft((us_dragwindow->screenhx - us_dragwindow->screenlx) / 10);
			didpan = TRUE;
		}
	}
	if (*x <= us_dragwindow->uselx)
	{
		*x = us_dragwindow->uselx;
		now = ticktime();
		if (now - us_beforepantime >= MINSLIDEDELAY)
		{
			us_beforepantime = now;
			us_slideleft(-(us_dragwindow->screenhx - us_dragwindow->screenlx) / 10);
			didpan = TRUE;
		}
	}
	if (*y >= us_dragwindow->usehy)
	{
		*y = us_dragwindow->usehy;
		now = ticktime();
		if (now - us_beforepantime >= MINSLIDEDELAY)
		{
			us_beforepantime = now;
			us_slideup(-(us_dragwindow->screenhy - us_dragwindow->screenly) / 10);
			didpan = TRUE;
		}
	}
	if (*y <= us_dragwindow->usely)
	{
		*y = us_dragwindow->usely;
		now = ticktime();
		if (now - us_beforepantime >= MINSLIDEDELAY)
		{
			us_beforepantime = now;
			us_slideup((us_dragwindow->screenhy - us_dragwindow->screenly) / 10);
			didpan = TRUE;
		}
	}
	if (didpan)
	{
		us_endchanges(NOWINDOWPART);
		us_dragshown = 0;
	}
}

BOOLEAN us_nullup(INTBIG x, INTBIG y)  { return(FALSE); }

void us_nullvoid(void) {}

BOOLEAN us_nullchar(INTBIG x, INTBIG y, INTSML ch) { return(FALSE); }

/*
 * routine to ignore all cursor motion while the button is up
 */
BOOLEAN us_ignoreup(INTBIG x, INTBIG y)
{
	gridalign(&x, &y, 1, el_curlib->curnodeproto);
	us_setcursorpos(NOWINDOWFRAME, x, y);
	return(FALSE);
}

/*
 * routine to cause termination of tracking when a key is typed
 */
BOOLEAN us_stoponchar(INTBIG x, INTBIG y, INTSML chr)
{
	if (chr == 'a' || chr == 'A')
	{
		el_pleasestop = 1;
		(void)stopping(STOPREASONTRACK);
	}
	return(TRUE);
}

/*
 * routine to cause termination of tracking and pop stacked highlighting
 * when a key is typed
 */
BOOLEAN us_stopandpoponchar(INTBIG x, INTBIG y, INTSML chr)
{
	if (chr == 'a' || chr == 'A')
	{
		el_pleasestop = 1;
		(void)stopping(STOPREASONTRACK);
		us_pophighlight(FALSE);
	}
	return(TRUE);
}
