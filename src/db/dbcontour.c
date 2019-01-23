/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dbcontour.c
 * Database file for gathering contours
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
#include "dbcontour.h"
#include "tecart.h"
#include "tecgen.h"
#include "eio.h"
#include <math.h>

static INTBIG      db_contourthresh, db_bestthresh, db_worstthresh;

/* prototypes for local routines */
static BOOLEAN db_addsegment(CONTOUR *con, CONTOURELEMENTTYPE type, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, INTBIG cx, INTBIG cy, NODEINST *ni);
static BOOLEAN db_addelementtocontour(CONTOUR *con, NODEINST *ni);
static BOOLEAN db_contourendpoints(NODEINST *ni, INTBIG *fx, INTBIG *fy, INTBIG *tx, INTBIG *ty);
static void    db_getcontoursegmentparameters(void);

/*
 * routine to examine all of the geometry in cell "np" and build a contour structure for it.
 * If "usearray" is nonzero, it is a NONODEINST-terminated array of nodes to consider.
 * To allow for gaps in the data, "bestthresh" is the initial error threshold that will be
 * allowed, and "worstthresh" is the greatest error threshold that will be allowed (usually
 * 2 or 3 orders of magnitude larger than "bestthresh").  If both thresholds are zero,
 * endpoints must meet precisely.  Returns a list of contours for this cell (NOCONTOUR on
 * error).
 */
CONTOUR *gathercontours(NODEPROTO *np, NODEINST **usearray, INTBIG bestthresh, INTBIG worstthresh)
{
	REGISTER GEOM *obj;
	REGISTER INTBIG sea, fdist, tdist, bestdist, dist, ex, ey, nodecount, foundthresh;
	INTBIG fx, fy, tx, ty;
	double startoffset, endangle;
	REGISTER INTBIG l;
	REGISTER NODEINST *ni, *oni, *bestni;
	REGISTER CONTOUR *con, *firstcontour;

	db_bestthresh = bestthresh;
	db_worstthresh = worstthresh;

	firstcontour = NOCONTOUR;
	if (usearray != 0)
	{
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			ni->temp1 = 1;
		for(l=0; usearray[l] != NONODEINST; l++)
			usearray[l]->temp1 = 0;
	} else
	{
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			ni->temp1 = 0;
	}

	/* preprocess irrelevant nodes */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->temp1 != 0) continue;
		if (ni->proto->primindex == 0) ni->temp1 = 1;
		if (ni->proto == gen_invispinprim) ni->temp1 = 1;

		/* handle circles now */
		if (ni->proto == art_circleprim || ni->proto == art_thickcircleprim)
		{
			getarcdegrees(ni, &startoffset, &endangle);
			if (startoffset == 0.0 && endangle == 0.0)
			{
				/* make the contour with the circle */
				con = (CONTOUR *)emalloc(sizeof (CONTOUR), db_cluster);
				if (con == 0) return(NOCONTOUR);
				con->valid = 1;
				con->childtotal = 0;
				con->nextcontour = firstcontour;
				firstcontour = con;
				con->firstcontourelement = NOCONTOURELEMENT;
				(void)db_addsegment(con, CIRCLESEGMENTTYPE, ni->highx, (ni->lowy+ni->highy)/2, 0, 0,
					(ni->lowx+ni->highx)/2, (ni->lowy+ni->highy)/2, ni);
				ni->temp1 = 1;
			}
		}
	}

	nodecount = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->temp1 != 0) continue;

		/* check for interrupt */
		if ((nodecount%25) == 0)
		{
			if (stopping(STOPREASONCONTOUR)) break;
		}

		/* start a contour with this node */
		con = (CONTOUR *)emalloc(sizeof (CONTOUR), db_cluster);
		if (con == 0) return(NOCONTOUR);
		con->firstcontourelement = NOCONTOURELEMENT;
		con->childtotal = 0;
		if (db_addelementtocontour(con, ni))
		{
			db_contourthresh = db_bestthresh;
			foundthresh = db_contourthresh;
			ni->temp1 = 1;
			for(;;)
			{
				/* see if the contour is closed */
				if (labs(con->firstcontourelement->sx - con->lastcontourelement->ex) <= db_contourthresh &&
					labs(con->firstcontourelement->sy - con->lastcontourelement->ey) <= db_contourthresh) break;

				/* gather bounding box of contour */
				ex = con->lastcontourelement->ex;
				ey = con->lastcontourelement->ey;

				/* search for another element to be added */
				sea = initsearch(ex-db_contourthresh, ex+db_contourthresh, ey-db_contourthresh, ey+db_contourthresh, ni->parent);
				bestni = NONODEINST;
				for(;;)
				{
					obj = nextobject(sea);
					if (obj == NOGEOM) break;
					if (!obj->entryisnode) continue;
					oni = obj->entryaddr.ni;
					if (oni->temp1 != 0) continue;
					if (db_contourendpoints(oni, &fx, &fy, &tx, &ty)) continue;
					fdist = computedistance(fx, fy, ex, ey);
					tdist = computedistance(tx, ty, ex, ey);
					dist = mini(fdist, tdist);

					/* LINTED "bestdist" used in proper order */
					if (bestni == NONODEINST || dist < bestdist)
					{
						bestni = oni;
						bestdist = dist;
					}
				}
				if (bestni != NONODEINST)
				{
					if (db_addelementtocontour(con, bestni))
					{
						bestni->temp1 = 1;
						if (db_contourthresh > foundthresh) foundthresh = db_contourthresh;
						db_contourthresh = db_bestthresh;
						continue;
					}
				}
				if (db_contourthresh == db_worstthresh) break;
				db_contourthresh *= 10;
			}
		}

		if (con->firstcontourelement == NOCONTOURELEMENT || con->lastcontourelement == NOCONTOURELEMENT)
		{
			/* contour empty: kill it */
			killcontour(con);
			continue;
		}

		/* see if this contour is valid */
		con->valid = 0;
		if (con->firstcontourelement != con->lastcontourelement &&
			labs(con->firstcontourelement->sx - con->lastcontourelement->ex) <= foundthresh &&
			labs(con->firstcontourelement->sy - con->lastcontourelement->ey) <= foundthresh)
				con->valid = 1;

		/* valid: link it in */
		con->nextcontour = firstcontour;
		firstcontour = con;
	}
	return(firstcontour);
}

/*
 * Routine to find the two endpoints of node "ni" and place them in (fx,fy) and (tx,ty).
 * Returns true if there are no two endpoints.
 */
BOOLEAN db_contourendpoints(NODEINST *ni, INTBIG *fx, INTBIG *fy, INTBIG *tx, INTBIG *ty)
{
	REGISTER VARIABLE *var;
	REGISTER INTBIG len, cx, cy;
	double startoffset, endangle;
	XARRAY trans;

	if (ni->proto == art_openedpolygonprim || ni->proto == art_closedpolygonprim)
	{
		var = gettrace(ni);
		if (var == NOVARIABLE) return(TRUE);
		len = getlength(var);
		makerot(ni, trans);
		cx = (ni->lowx + ni->highx) / 2;
		cy = (ni->lowy + ni->highy) / 2;
		xform(((INTBIG *)var->addr)[0]+cx, ((INTBIG *)var->addr)[1]+cy, fx, fy, trans);
		xform(((INTBIG *)var->addr)[len-2]+cx, ((INTBIG *)var->addr)[len-1]+cy, tx, ty, trans);
		return(FALSE);
	}
	if (ni->proto == art_circleprim || ni->proto == art_thickcircleprim)
	{
		getarcdegrees(ni, &startoffset, &endangle);
		if (startoffset == 0.0 && endangle == 0.0) return(TRUE);
		getarcendpoints(ni, startoffset, endangle, fx, fy, tx, ty);
		return(FALSE);
	}
	return(TRUE);
}

/*
 * routine to add the node "ni" to the contour "con".  Returns true if it was able to make a connection.
 */
BOOLEAN db_addelementtocontour(CONTOUR *con, NODEINST *ni)
{
	REGISTER VARIABLE *var;
	REGISTER INTBIG i, len, cx, cy, lastx, lasty, firstx, firsty;
	INTBIG sx, sy, ex, ey, x, y;
	double startoffset, endangle;
	XARRAY trans;

	cx = (ni->lowx + ni->highx) / 2;
	cy = (ni->lowy + ni->highy) / 2;
	if (ni->proto == art_openedpolygonprim || ni->proto == art_closedpolygonprim)
	{
		var = gettrace(ni);
		if (var == NOVARIABLE) return(FALSE);
		len = getlength(var);
		makerot(ni, trans);
		xform(((INTBIG *)var->addr)[len-2]+cx, ((INTBIG *)var->addr)[len-1]+cy, &x, &y, trans);
		if (con->firstcontourelement == NOCONTOURELEMENT ||
			(labs(con->lastcontourelement->ex - x) <= db_contourthresh &&
				labs(con->lastcontourelement->ey - y) <= db_contourthresh))
		{
			for(i=len-2; i >= 0; i -= 2)
			{
				xform(((INTBIG *)var->addr)[i]+cx, ((INTBIG *)var->addr)[i+1]+cy, &x, &y, trans);
				if (i == len-2)
				{
					firstx = x;
					firsty = y;
				} else
				{
					/* LINTED "lastx" and "lasty" are used in proper order */
					if (!db_addsegment(con, LINESEGMENTTYPE, lastx, lasty, x, y, 0, 0, ni))
						return(FALSE);
				}
				lastx = x;
				lasty = y;
			}
		} else
		{
			for(i=0; i<len; i += 2)
			{
				xform(((INTBIG *)var->addr)[i]+cx, ((INTBIG *)var->addr)[i+1]+cy, &x, &y, trans);
				if (i == 0)
				{
					firstx = x;
					firsty = y;
				} else
				{
					if (!db_addsegment(con, LINESEGMENTTYPE, lastx, lasty, x, y, 0, 0, ni))
						return(FALSE);
				}
				lastx = x;
				lasty = y;
			}
		}
		if (ni->proto == art_closedpolygonprim)
		{
			if (!db_addsegment(con, LINESEGMENTTYPE, lastx, lasty, firstx, firsty, 0, 0, ni))
				return(FALSE);
		}
		return(TRUE);
	}
	if (ni->proto == art_circleprim || ni->proto == art_thickcircleprim)
	{
		getarcdegrees(ni, &startoffset, &endangle);
		if (startoffset == 0.0 && endangle == 0.0) return(FALSE);
		getarcendpoints(ni, startoffset, endangle, &sx, &sy, &ex, &ey);
		if (db_addsegment(con, ARCSEGMENTTYPE, sx, sy, ex, ey, cx, cy, ni))
			return(TRUE);
		return(FALSE);
	}
	return(FALSE);
}

/*
 * routine to add the line/arc segment from (x1,y1) to (x2,y2) to the contour "con".  The type of the segment
 * is "type".  If an arc or circle, then (cx/cy) are the center.  The original node is in "ni".
 * Returns true if it makes the connection.
 */
BOOLEAN db_addsegment(CONTOUR *con, CONTOURELEMENTTYPE type, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2,
	INTBIG cx, INTBIG cy, NODEINST *ni)
{
	CONTOURELEMENT *conel;

	if (type != CIRCLESEGMENTTYPE && x1 == x2 && y1 == y2) return(FALSE);
	conel = (CONTOURELEMENT *)emalloc(sizeof (CONTOURELEMENT), db_cluster);
	if (conel == 0) return(FALSE);
	conel->elementtype = type;
	conel->ni = ni;
	conel->cx = cx;
	conel->cy = cy;
	if (con->firstcontourelement == NOCONTOURELEMENT)
	{
		conel->sx = x1;	conel->sy = y1;
		conel->ex = x2;	conel->ey = y2;
		conel->nextcontourelement = NOCONTOURELEMENT;
		con->firstcontourelement = conel;
		con->lastcontourelement = conel;
		return(TRUE);
	}

	/* see if one of these coordinates matches the end of the contour */
	if (labs(con->lastcontourelement->ex - x1) <= db_contourthresh &&
		labs(con->lastcontourelement->ey - y1) <= db_contourthresh)
	{
		conel->sx = x1;	conel->sy = y1;
		conel->ex = x2;	conel->ey = y2;
		conel->nextcontourelement = NOCONTOURELEMENT;
		con->lastcontourelement->nextcontourelement = conel;
		con->lastcontourelement = conel;
		return(TRUE);
	}
	if (labs(con->lastcontourelement->ex - x2) <= db_contourthresh &&
		labs(con->lastcontourelement->ey - y2) <= db_contourthresh)
	{
		conel->sx = x2;	conel->sy = y2;
		conel->ex = x1;	conel->ey = y1;
		if (type == ARCSEGMENTTYPE) conel->elementtype = REVARCSEGMENTTYPE;
		conel->nextcontourelement = NOCONTOURELEMENT;
		con->lastcontourelement->nextcontourelement = conel;
		con->lastcontourelement = conel;
		return(TRUE);
	}
	return(FALSE);
}

/*
 * routine to compute the bounding box of contour element "conel" and return it in "lx/hx/ly/hy"
 */
void getcontourelementbbox(CONTOURELEMENT *conel, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy)
{
	REGISTER INTBIG radius;

	switch (conel->elementtype)
	{
		case LINESEGMENTTYPE:
		case BRIDGESEGMENTTYPE:
			*lx = mini(conel->sx, conel->ex);
			*hx = maxi(conel->sx, conel->ex);
			*ly = mini(conel->sy, conel->ey);
			*hy = maxi(conel->sy, conel->ey);
			break;
		case ARCSEGMENTTYPE:
			arcbbox(conel->sx, conel->sy, conel->ex, conel->ey, conel->cx, conel->cy,
				lx, hx, ly, hy);
			break;
		case REVARCSEGMENTTYPE:
			arcbbox(conel->ex, conel->ey, conel->sx, conel->sy, conel->cx, conel->cy,
				lx, hx, ly, hy);
			break;
		case CIRCLESEGMENTTYPE:
			radius = computedistance(conel->sx, conel->sy, conel->cx, conel->cy);
			*lx = conel->cx - radius;
			*hx = conel->cx + radius;
			*ly = conel->cy - radius;
			*hy = conel->cy + radius;
			break;
	}
}

/*
 * routine to compute the bounding box of contour "con" and return it in "lx/hx/ly/hy"
 */
void getcontourbbox(CONTOUR *con, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy)
{
	REGISTER CONTOURELEMENT *conel;
	INTBIG llx, lhx, lly, lhy;

	for(conel = con->firstcontourelement; conel != NOCONTOURELEMENT; conel = conel->nextcontourelement)
	{
		getcontourelementbbox(conel, &llx, &lhx, &lly, &lhy);
		if (conel == con->firstcontourelement)
		{
			*lx = llx;   *hx = lhx;
			*ly = lly;   *hy = lhy;
		} else
		{
			if (llx < *lx) *lx = llx;
			if (lhx > *hx) *hx = lhx;
			if (lly < *ly) *ly = lly;
			if (lhy > *hy) *hy = lhy;
		}
	}
}

/*
 * routine to kill contour "con" and all memory associated with it
 */
void killcontour(CONTOUR *con)
{
	CONTOURELEMENT *conel, *nextconel;

	for(conel = con->firstcontourelement; conel != NOCONTOURELEMENT; conel = nextconel)
	{
		nextconel = conel->nextcontourelement;
		efree((CHAR *)conel);
	}
	if (con->childtotal > 0) efree((CHAR *)con->children);
	efree((CHAR *)con);
}

/******************************** ARC SEGMENT GENERATION ********************************/

#define DEFARCRES 210			/* 21 degrees */

static INTBIG   db_arcres, db_arcsag;
static POLYGON *db_contourpoly = NOPOLYGON;
static INTBIG   db_contoursegmentcount;

void db_getcontoursegmentparameters(void)
{
	REGISTER VARIABLE *var;

	var = getval((INTBIG)io_tool, VTOOL, VINTEGER, x_("IO_curve_resolution"));
	if (var == NOVARIABLE) db_arcres = DEFARCRES; else
		db_arcres = var->addr;
	var = getval((INTBIG)io_tool, VTOOL, VINTEGER, x_("IO_curve_sag"));
	if (var == NOVARIABLE) db_arcsag = scalefromdispunit(0.0005f, DISPUNITMM); else
		db_arcsag = var->addr;
}

/*
 * routine to get the arc conversion parameters.  The "arcres" is the number of
 * tenth-degrees between line segments, and "arcsag" is the maximum distance that a
 * segment can sag from its true curvature.
 */
void getcontoursegmentparameters(INTBIG *arcres, INTBIG *arcsag)
{
	db_getcontoursegmentparameters();
	*arcres = db_arcres;
	*arcsag = db_arcsag;
}

/*
 * routine to set the arc conversion parameters.  The "arcres" is the number of
 * tenth-degrees between line segments, and "arcsag" is the maximum distance that a
 * segment can sag from its true curvature.
 */
void setcontoursegmentparameters(INTBIG arcres, INTBIG arcsag)
{
	db_arcres = arcres;
	db_arcsag = arcsag;

	(void)setval((INTBIG)io_tool, VTOOL, x_("IO_curve_resolution"), arcres, VINTEGER);
	(void)setval((INTBIG)io_tool, VTOOL, x_("IO_curve_sag"), arcsag, VINTEGER);
}

/*
 * routine to begin conversion of arc contour element "conel" into its line segments.
 */
void initcontoursegmentgeneration(CONTOURELEMENT *conel)
{
	REGISTER INTBIG swapx, swapy, radius, i, reverse;

	db_getcontoursegmentparameters();
	if (db_contourpoly == NOPOLYGON) db_contourpoly = allocpolygon(4, db_cluster);
	if (conel->elementtype == CIRCLESEGMENTTYPE)
	{
		radius = computedistance(conel->cx, conel->cy, conel->sx, conel->sy);
		circletopoly(conel->cx, conel->cy, radius, db_contourpoly, db_arcres, db_arcsag);
	} else if (conel->elementtype == REVARCSEGMENTTYPE)
	{
		arctopoly(conel->cx, conel->cy, conel->ex, conel->ey, conel->sx, conel->sy,
			db_contourpoly, db_arcres, db_arcsag);
		reverse = db_contourpoly->count - 1;
		for(i=0; i<db_contourpoly->count/2; i++)
		{
			swapx = db_contourpoly->xv[i];
			swapy = db_contourpoly->yv[i];

			db_contourpoly->xv[i] = db_contourpoly->xv[reverse-i];
			db_contourpoly->yv[i] = db_contourpoly->yv[reverse-i];

			db_contourpoly->xv[reverse-i] = swapx;
			db_contourpoly->yv[reverse-i] = swapy;
		}
	} else
	{
		arctopoly(conel->cx, conel->cy, conel->sx, conel->sy, conel->ex, conel->ey,
			db_contourpoly, db_arcres, db_arcsag);
	}

	db_contoursegmentcount = 0;
}

/*
 * routine to return the next line segment in the arc contour that was passed to
 * "compen_initsegmentgeneration".  The line runs from (x1,y1) to (x2,y2).
 * Returns true if there are no more line segments.
 */
BOOLEAN nextcontoursegmentgeneration(INTBIG *x1, INTBIG *y1, INTBIG *x2, INTBIG *y2)
{
	if (db_contoursegmentcount >= db_contourpoly->count-1) return(TRUE);
	*x1 = db_contourpoly->xv[db_contoursegmentcount];
	*y1 = db_contourpoly->yv[db_contoursegmentcount];

	*x2 = db_contourpoly->xv[db_contoursegmentcount+1];
	*y2 = db_contourpoly->yv[db_contoursegmentcount+1];
	db_contoursegmentcount++;
	return(FALSE);
}
