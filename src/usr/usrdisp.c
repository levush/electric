/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrdisp.c
 * User interface tool: miscellaneous display control
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
#include "tech.h"
#include "tecgen.h"

static INTBIG us_stopevent = 0;

/* for drawing cell name, outline or instance name (color changes) */
GRAPHICS us_cellgra = {LAYERO, 0, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};

/* for drawing outlines (color changes) */
GRAPHICS us_box = {LAYERA, 0, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};

/* for drawing highlight layer (color changes) */
GRAPHICS us_hbox = {LAYERH, 0, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};

/* for drawing normal menu border on (nothing changes) */
GRAPHICS us_nmbox = {LAYERA, MENBOR, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};

/* for drawing arbitrary graphics (bits and color change) */
GRAPHICS us_arbit = {0, 0, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};

/* for erasing polygons (nothing changes) */
GRAPHICS us_ebox = {LAYERA, ALLOFF, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};

/* for drawing grid lines (nothing changes) */
GRAPHICS us_gbox = {LAYERG, GRID, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};

/* for erasing grid layer (nothing changes) */
GRAPHICS us_egbox = {LAYERG, ALLOFF, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};

/* for drawing patterned cell contents (when resolution is too small, color changes) */
static GRAPHICS us_graybox = {LAYERO, GRAY, PATTERNED, PATTERNED,
					{0x0000, /*                  */
					0x0303,  /*       XX      XX */
					0x4848,  /*  X  X    X  X    */
					0x0303,  /*       XX      XX */
					0x0000,  /*                  */
					0x3030,  /*   XX      XX     */
					0x8484,  /* X    X  X    X   */
					0x3030,  /*   XX      XX     */
					0x0000,  /*                  */
					0x0303,  /*       XX      XX */
					0x4848,  /*  X  X    X  X    */
					0x0303,  /*       XX      XX */
					0x0000,  /*                  */
					0x3030,  /*   XX      XX     */
					0x8484,  /* X    X  X    X   */
					0x3030}, /*   XX      XX     */
					NOVARIABLE, 0};

/* for drawing dimmed background (when editing in-place) */
static GRAPHICS us_dimbox = {LAYERH, HIGHLIT, PATTERNED, PATTERNED,
					{0x0404, /*      X       X   */
					0x0000,  /*                  */
					0x4040,  /*  X       X       */
					0x0000,  /*                  */
					0x0404,  /*      X       X   */
					0x0000,  /*                  */
					0x4040,  /*  X       X       */
					0x0000,  /*                  */
					0x0404,  /*      X       X   */
					0x0000,  /*                  */
					0x4040,  /*  X       X       */
					0x0000,  /*                  */
					0x0404,  /*      X       X   */
					0x0000,  /*                  */
					0x4040,  /*  X       X       */
					0x0000}, /*                  */
					NOVARIABLE, 0};

#define MAXGRID         75
#define MINGRID          4	/* the minimum grid has 4 pixels spacing */
#define TXTMAXCELLSIZE  36	/* maximum point size of cell text */

/* prototypes for local routines */
static void   us_showemptywindow(WINDOWPART*);
static void   us_graphicsarcs(PORTPROTO*, INTBIG*, INTBIG*);
static void   us_combinelayers(ARCINST*, INTBIG*, INTBIG*);
static INTBIG us_drawall(INTBIG, INTBIG, INTBIG, INTBIG, NODEPROTO*, XARRAY, INTBIG, BOOLEAN);
static INTBIG us_drawarcinstpeek(ARCINST*, XARRAY, INTBIG);
static INTBIG us_drawnodeinstpeek(NODEINST*, XARRAY, INTBIG);
static void   us_drawcellcontents(NODEPROTO *cell, WINDOWPART *w, BOOLEAN now);
static int    us_sortshownports(const void *e1, const void *e2);

/******************** WINDOW CONTROL ********************/

/*
 * routine to draw the border of window "w"
 */
void us_drawwindow(WINDOWPART *w, INTBIG color)
{
	WINDOWPART ww;
	static POLYGON *poly = NOPOLYGON;
	INTBIG lx, hx, ly, hy;

	/* get polygon */
	(void)needstaticpolygon(&poly, 5, us_tool->cluster);

	/* don't draw window border if it is a whole-screen window */
	if (estrcmp(w->location, x_("entire")) == 0) return;

	/* don't draw window border around popup text editors */
	if ((w->state & WINDOWTYPE) == POPTEXTWINDOW) return;

	us_gettruewindowbounds(w, &lx, &hx, &ly, &hy);
	lx--;   hx++;
	ly--;   hy++;

	ww.screenlx = ww.uselx = lx;
	ww.screenhx = ww.usehx = hx;
	ww.screenly = ww.usely = ly;
	ww.screenhy = ww.usehy = hy;
	ww.frame = w->frame;
	ww.state = DISPWINDOW;
	computewindowscale(&ww);
	maketruerectpoly(ww.uselx, ww.usehx, ww.usely, ww.usehy, poly);
	poly->desc = &us_box;
	poly->style = CLOSEDRECT;
	us_box.col = color;
	us_showpoly(poly, &ww);
}

/*
 * Routine to get the actual drawing area in window "w".  This excludes borders and
 * sliders.  The bounds are returned in (lx/hx/ly/hy).
 */
void us_gettruewindowbounds(WINDOWPART *w, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy)
{
	*lx = w->uselx;   *hx = w->usehx;
	*ly = w->usely;   *hy = w->usehy;
	if ((w->state&WINDOWMODE) != 0)
	{
		*lx -= WINDOWMODEBORDERSIZE;   *hx += WINDOWMODEBORDERSIZE;
		*ly -= WINDOWMODEBORDERSIZE;   *hy += WINDOWMODEBORDERSIZE;
	}
	if ((w->state&WINDOWTYPE) == DISPWINDOW)
	{
		*hx += DISPLAYSLIDERSIZE;
		*ly -= DISPLAYSLIDERSIZE;
	}
	if ((w->state&WINDOWTYPE) == WAVEFORMWINDOW)
	{
		*lx -= DISPLAYSLIDERSIZE;
		*ly -= DISPLAYSLIDERSIZE;
	}
}

/*
 * routine to adjust screen boundaries to fill the view
 */
void us_fullview(NODEPROTO *np, INTBIG *screenlx, INTBIG *screenhx, INTBIG *screenly,
	INTBIG *screenhy)
{
	INTBIG fsx, fsy, nlx, nhx, nly, nhy, xc, yc, tsx, tsy;
	REGISTER INTBIG lambda, first, oldlx, oldhx, oldly, oldhy, i, xw, yw, frameinfo;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER VARIABLE *var;
	REGISTER WINDOWPART *oldwin;
	REGISTER CHAR *str;
	static POLYGON *poly = NOPOLYGON;

	/* only one size allowed in windows with frames */
	frameinfo = framesize(&fsx, &fsy, np);
	if (frameinfo == 0)
	{
		*screenlx = -fsx/2;
		*screenly = -fsy/2;
		*screenhx = fsx/2;
		*screenhy = fsy/2;
		return;
	}

	(void)needstaticpolygon(&poly, 4, us_tool->cluster);
	lambda = el_curlib->lambda[el_curtech->techindex];
	if (el_curwindowpart != NOWINDOWPART)
	{
		oldlx = el_curwindowpart->screenlx;
		oldhx = el_curwindowpart->screenhx;
		oldly = el_curwindowpart->screenly;
		oldhy = el_curwindowpart->screenhy;
		el_curwindowpart->screenlx = np->lowx;
		el_curwindowpart->screenhx = np->highx;
		if (el_curwindowpart->screenlx == el_curwindowpart->screenhx)
		{
			el_curwindowpart->screenlx -= lambda;
			el_curwindowpart->screenhx += lambda;
		}
		el_curwindowpart->screenly = np->lowy;
		el_curwindowpart->screenhy = np->highy;
		if (el_curwindowpart->screenly == el_curwindowpart->screenhy)
		{
			el_curwindowpart->screenly -= lambda;
			el_curwindowpart->screenhy += lambda;
		}
		us_squarescreen(el_curwindowpart, NOWINDOWPART, FALSE,
			&el_curwindowpart->screenlx, &el_curwindowpart->screenhx,
			&el_curwindowpart->screenly, &el_curwindowpart->screenhy, 1);
		computewindowscale(el_curwindowpart);
	}

	/* must recompute this by hand because cell bounds don't include cell-centers or big text */
	first = 1;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		us_getnodebounds(ni, &nlx, &nhx, &nly, &nhy);
		if (first != 0)
		{
			*screenlx = nlx;
			*screenhx = nhx;
			*screenly = nly;
			*screenhy = nhy;
			first = 0;
		} else
		{
			*screenlx = mini(*screenlx, nlx);
			*screenhx = maxi(*screenhx, nhx);
			*screenly = mini(*screenly, nly);
			*screenhy = maxi(*screenhy, nhy);
		}
	}

	/* include all arcs in the cell */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		us_getarcbounds(ai, &nlx, &nhx, &nly, &nhy);
		*screenlx = mini(*screenlx, nlx);
		*screenhx = maxi(*screenhx, nhx);
		*screenly = mini(*screenly, nly);
		*screenhy = maxi(*screenhy, nhy);
	}

	/* include all displayed cell variables */
	oldwin = setvariablewindow(el_curwindowpart);
	for(i = 0; i < np->numvar; i++)
	{
		var = &np->firstvar[i];
		if ((var->type&VDISPLAY) == 0) continue;
		str = describedisplayedvariable(var, -1, -1);

		/* determine center of text (cell variables are offset from (0,0)) */
		poly->xv[0] = poly->yv[0] = 0;
		poly->count = 1;
		poly->style = FILLED;
		adjustdisoffset((INTBIG)np, VNODEPROTO, np->tech, poly, var->textdescript);
		getcenter(poly, &xc, &yc);

		/* determine size from text */
		screensettextinfo(el_curwindowpart, np->tech, var->textdescript);
		screengettextsize(el_curwindowpart, str, &tsx, &tsy);
		xw = muldiv(tsx, el_curwindowpart->screenhx-el_curwindowpart->screenlx,
			el_curwindowpart->usehx-el_curwindowpart->uselx);
		yw = muldiv(tsy, el_curwindowpart->screenhy-el_curwindowpart->screenly,
			el_curwindowpart->usehy-el_curwindowpart->usely);
		us_buildtexthighpoly(0, 0, 0, 0, xc, yc, xw, yw, poly->style, poly);
		getbbox(poly, &nlx, &nhx, &nly, &nhy);
		if (first != 0)
		{
			*screenlx = nlx;
			*screenhx = nhx;
			*screenly = nly;
			*screenhy = nhy;
			first = 0;
		} else
		{
			*screenlx = mini(*screenlx, nlx);
			*screenhx = maxi(*screenhx, nhx);
			*screenly = mini(*screenly, nly);
			*screenhy = maxi(*screenhy, nhy);
		}
	}
	(void)setvariablewindow(oldwin);

	/* set default size if nothing is there */
	if (first != 0)
	{
		*screenlx = np->lowx;
		*screenhx = np->highx;
		*screenly = np->lowy;
		*screenhy = np->highy;
	}

	/* if there is frame information, include it */
	if (frameinfo == 1)
	{
		*screenlx = mini(*screenlx, -fsx/2);
		*screenhx = maxi(*screenhx, fsx/2);
		*screenly = mini(*screenly, -fsy/2);
		*screenhy = maxi(*screenhy, fsy/2);
	}

	if (*screenlx >= *screenhx && *screenly >= *screenhy)
	{
		*screenlx -= 25 * lambda;
		*screenhx += 25 * lambda;
		*screenly -= 25 * lambda;
		*screenhy += 25 * lambda;
	}
	if (el_curwindowpart != NOWINDOWPART)
	{
		el_curwindowpart->screenlx = oldlx;
		el_curwindowpart->screenhx = oldhx;
		el_curwindowpart->screenly = oldly;
		el_curwindowpart->screenhy = oldhy;
		computewindowscale(el_curwindowpart);
	}
}

/*
 * Routine to determine the bounds of node "ni" and return it in "nlx", "nhx",
 * "nly", and "nhy".
 */
void us_getnodebounds(NODEINST *ni, INTBIG *nlx, INTBIG *nhx, INTBIG *nly, INTBIG *nhy)
{
	REGISTER INTBIG i, nodexfvalid, lambda, shei, swid, portstyle;
	INTBIG lx, hx, ly, hy, xp, yp, newxc, newyc;
	INTBIG wid, hei;
	REGISTER VARIABLE *var;
	XARRAY trans;
	REGISTER PORTEXPINST *pe;
	REGISTER PORTPROTO *pp;
	static POLYGON *poly = NOPOLYGON;

	*nlx = ni->geom->lowx;
	*nhx = ni->geom->highx;
	*nly = ni->geom->lowy;
	*nhy = ni->geom->highy;

	/* include any displayable variables with an offset or array */
	if (el_curwindowpart != NOWINDOWPART)
	{
		for(i=0; i<ni->numvar; i++)
		{
			var = &ni->firstvar[i];
			if ((var->type&VDISPLAY) == 0) continue;

			(void)needstaticpolygon(&poly, 4, us_tool->cluster);
			makedisparrayvarpoly(ni->geom, el_curwindowpart, var, poly);
			getbbox(poly, &lx, &hx, &ly, &hy);
			*nlx = mini(*nlx, lx);
			*nhx = maxi(*nhx, hx);
			*nly = mini(*nly, ly);
			*nhy = maxi(*nhy, hy);
		}
	}

	/* include exports */
	nodexfvalid = 0;
	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
	{
		pp = pe->exportproto;
		if (nodexfvalid == 0)
		{
			nodexfvalid = 1;
			makeangle(ni->rotation, ni->transpose, trans);
			lambda = figurelambda(ni->geom);
		}
		newxc = TDGETXOFF(pp->textdescript);
		newxc = newxc * lambda / 4;
		newyc = TDGETYOFF(pp->textdescript);
		newyc = newyc * lambda / 4;
		xform(newxc, newyc, &newxc, &newyc, trans);
		portposition(ni, pe->proto, &xp, &yp);
		xp += newxc;
		yp += newyc;
		lx = hx = xp;   ly = hy = yp;
		portstyle = us_useroptions & EXPORTLABELS;
		if (el_curwindowpart != NOWINDOWPART &&
			(portstyle == EXPORTSFULL || portstyle == EXPORTSSHORT))
		{
			/* adjust for size of port text */
			screensettextinfo(el_curwindowpart, pp->parent->tech, pp->textdescript);
			screengettextsize(el_curwindowpart,
				us_displayedportname(pp, portstyle >> EXPORTLABELSSH), &wid, &hei);
			swid = roundfloat((float)wid / el_curwindowpart->scalex);
			shei = roundfloat((float)hei / el_curwindowpart->scaley);
			switch (TDGETPOS(pp->textdescript))
			{
				case VTPOSCENT:     case VTPOSDOWN:       case VTPOSUP:
					lx -= swid/2;   hx += swid/2;         break;
				case VTPOSRIGHT:    case VTPOSDOWNRIGHT:  case VTPOSUPRIGHT:
					hx += swid;     break;
				case VTPOSLEFT:     case VTPOSDOWNLEFT:   case VTPOSUPLEFT:
					lx -= swid;     break;
			}
			switch (TDGETPOS(pp->textdescript))
			{
				case VTPOSCENT:     case VTPOSRIGHT:      case VTPOSLEFT:
					ly -= shei/2;   hy += shei/2;         break;
				case VTPOSDOWN:     case VTPOSDOWNRIGHT:  case VTPOSDOWNLEFT:
					ly -= shei;     break;
				case VTPOSUP:       case VTPOSUPRIGHT:    case VTPOSUPLEFT:
					hy += shei;     break;
			}
		}
		*nlx = mini(*nlx, lx);
		*nhx = maxi(*nhx, hx);
		*nly = mini(*nly, ly);
		*nhy = maxi(*nhy, hy);
	}
}

/*
 * Routine to determine the bounds of node "ni" and return it in "nlx", "nhx",
 * "nly", and "nhy".
 */
void us_getarcbounds(ARCINST *ai, INTBIG *nlx, INTBIG *nhx, INTBIG *nly, INTBIG *nhy)
{
	REGISTER INTBIG i;
	INTBIG lx, hx, ly, hy;
	REGISTER VARIABLE *var;
	static POLYGON *poly = NOPOLYGON;

	*nlx = ai->geom->lowx;
	*nhx = ai->geom->highx;
	*nly = ai->geom->lowy;
	*nhy = ai->geom->highy;

	/* include any displayable variables with an offset */
	if (el_curwindowpart != NOWINDOWPART)
	{
		for(i=0; i<ai->numvar; i++)
		{
			var = &ai->firstvar[i];
			if ((var->type&VDISPLAY) == 0) continue;
			(void)needstaticpolygon(&poly, 4, us_tool->cluster);
			makedisparrayvarpoly(ai->geom, el_curwindowpart, var, poly);
			getbbox(poly, &lx, &hx, &ly, &hy);
			*nlx = mini(*nlx, lx);
			*nhx = maxi(*nhx, hx);
			*nly = mini(*nly, ly);
			*nhy = maxi(*nhy, hy);
		}
	}
}

/*
 * routine to adjust "screenlx/hx/ly/hy" for window "w" so that it defines
 * a square screen and leaves some extra space around the values.
 * If "formerw" is a valid window, this is the former window from which
 * a new one in "w" was created, so adjust without scaling.
 * Scaling a window means that one dimension gets bigger or the other
 * gets smaller to make the square relationship hold.  If "largescale" is
 * true, the new window will use the larger of the two scales rather
 * than the smaller.
 */
void us_squarescreen(WINDOWPART *w, WINDOWPART *formerw, BOOLEAN largescale, INTBIG *screenlx,
	INTBIG *screenhx, INTBIG *screenly, INTBIG *screenhy, INTBIG exact)
{
	REGISTER INTBIG i, units, image, design, lambda;
	float prod1, prod2, prodswap, fslx, fshx, fsly, fshy, bump, ysize, xsize;

	if (formerw != NOWINDOWPART)
	{
		*screenlx = muldiv(((formerw->usehx-formerw->uselx) - (w->usehx-w->uselx))/2,
			formerw->screenhx-formerw->screenlx, formerw->usehx-formerw->uselx) + formerw->screenlx;
		*screenly = muldiv(((formerw->usehy-formerw->usely) - (w->usehy-w->usely))/2,
			formerw->screenhy-formerw->screenly, formerw->usehy-formerw->usely) + formerw->screenly;
		*screenhx = w->screenlx + muldiv(w->usehx-w->uselx, formerw->screenhx-formerw->screenlx,
			formerw->usehx-formerw->uselx);
		*screenhy = w->screenly + muldiv(w->usehy-w->usely, formerw->screenhy-formerw->screenly,
			formerw->usehy-formerw->usely);
		return;
	}

	fslx = (float)*screenlx;   fshx = (float)*screenhx;
	fsly = (float)*screenly;   fshy = (float)*screenhy;
	xsize = (float)(w->usehx - w->uselx);
	ysize = (float)(w->usehy - w->usely);
	prod1 = (fshx - fslx) * ysize;
	prod2 = (fshy - fsly) * xsize;
	if (prod1 != prod2)
	{
		/* reverse the sense if the larger scale is desired */
		if (largescale)
		{
			prodswap = prod1;   prod1 = prod2;   prod2 = prodswap;
		}

		/* adjust the scale */
		if (prod1 > prod2)
		{
			/* screen extent is too wide for window */
			if (exact != 0) bump = 0.0; else
			{
				bump = (fshx - fslx) / 20.0f;
				if (bump == 0.0) bump = 1.0;
				fshx += bump;   fslx -= bump;
			}
			bump = (fshx - fslx) * ysize / xsize - (fshy - fsly);
			fsly -= bump/2.0f;
			fshy += bump/2.0f;
		} else
		{
			/* screen extent is too tall for window */
			if (exact != 0) bump = 0.0; else
			{
				bump = (fshy - fsly) / 20.0f;
				if (bump == 0.0) bump = 1.0;
				fshy += bump;   fsly -= bump;
			}
			bump = (fshy - fsly) * xsize / ysize - (fshx - fslx);
			fslx -= bump/2;
			fshx += bump/2;
		}

		/* put it back into the integer extent fields */
		if (fslx < -MAXINTBIG/2) *screenlx = -MAXINTBIG/2; else *screenlx = (INTBIG)fslx;
		if (fshx >  MAXINTBIG/2) *screenhx =  MAXINTBIG/2; else *screenhx = (INTBIG)fshx;
		if (fsly < -MAXINTBIG/2) *screenly = -MAXINTBIG/2; else *screenly = (INTBIG)fsly;
		if (fshy >  MAXINTBIG/2) *screenhy =  MAXINTBIG/2; else *screenhy = (INTBIG)fshy;
	}

	if ((us_tool->toolstate&INTEGRAL) != 0)
	{
		/* adjust window so that it matches well with screen pixels */
		if (*screenhx == *screenlx) return;
		if (w->curnodeproto == NONODEPROTO)
			lambda = el_curlib->lambda[el_curtech->techindex]; else
				lambda = lambdaofcell(w->curnodeproto);
		design = (*screenhx - *screenlx) / lambda;
		if (design <= 0) design = 1;
		image = w->usehx - w->uselx;
		if (image > design)
		{
			/* force integral number of pixels per lambda unit */
			i = (muldiv(image, lambda, image / design) - (*screenhx - *screenlx)) / 2;
			(*screenhx) += i;   (*screenlx) -= i;
			i = muldiv(*screenhx - *screenlx, w->usehy - w->usely,
				w->usehx - w->uselx) - (*screenhy - *screenly);
			(*screenly) -= i/2;
			(*screenhy) += i/2;
		} else
		{
			/* force integral number of lambda units per pixel */
			units = muldiv(design, lambda, image);
			i = ((units * (w->usehx - w->uselx)) - (*screenhx - *screenlx)) / 2;
			(*screenhx) += i;   (*screenlx) -= i;
			i = muldiv(*screenhx - *screenlx, w->usehy - w->usely,
				w->usehx - w->uselx) - (*screenhy - *screenly);
			(*screenly) -= i/2;
			(*screenhy) += i/2;
		}
	}
}

/*
 * routine to redisplay everything (all the way down to the bottom) in the
 * window "w"
 */
WINDOWPART *us_subwindow(INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy, WINDOWPART *w)
{
	static WINDOWPART ww;

	/* redefine the window to clip to this boundary */
	ww.uselx = applyxscale(w, lx-w->screenlx) + w->uselx;
	ww.usely = applyyscale(w, ly-w->screenly) + w->usely;
	ww.usehx = applyxscale(w, hx-w->screenlx) + w->uselx;
	ww.usehy = applyyscale(w, hy-w->screenly) + w->usely;
	ww.screenlx = muldiv(w->screenhx-w->screenlx, ww.uselx-w->uselx, w->usehx-w->uselx) +
		w->screenlx;
	ww.screenhx = muldiv(w->screenhx-w->screenlx, ww.usehx-w->uselx, w->usehx-w->uselx) +
		w->screenlx;
	ww.screenly = muldiv(w->screenhy-w->screenly, ww.usely-w->usely, w->usehy-w->usely) +
		w->screenly;
	ww.screenhy = muldiv(w->screenhy-w->screenly, ww.usehy-w->usely, w->usehy-w->usely) +
		w->screenly;
	ww.frame = w->frame;
	ww.curnodeproto = w->curnodeproto;
	ww.state = DISPWINDOW;
	computewindowscale(&ww);
	return(&ww);
}

void us_showwindowmode(WINDOWPART *w)
{
	REGISTER INTBIG x, y, numcolors, colorindex, high;
	INTBIG lx, hx, ly, hy, colors[5];

	us_gettruewindowbounds(w, &lx, &hx, &ly, &hy);
	numcolors = 0;
	if ((w->state&WINDOWSIMMODE) != 0) colors[numcolors++] = RED;
	if ((w->state&WINDOWTECEDMODE) != 0) colors[numcolors++] = YELLOW;
	if ((w->state&WINDOWOUTLINEEDMODE) != 0) colors[numcolors++] = BLUE;
	if (numcolors == 0) return;
	if (numcolors == 1)
	{
		/* just 1 border color: draw it simply */
		us_box.col = colors[0];
		screendrawbox(w, lx, hx, ly, ly+WINDOWMODEBORDERSIZE-1, &us_box);
		screendrawbox(w, lx, hx, hy-WINDOWMODEBORDERSIZE+1, hy, &us_box);
		screendrawbox(w, lx, lx+WINDOWMODEBORDERSIZE-1, ly, hy, &us_box);
		screendrawbox(w, hx-WINDOWMODEBORDERSIZE+1, hx, ly, hy, &us_box);
		return;
	}

	/* multiple border colors: interleave them */
	colorindex = 0;
	for(x = lx; x<hx; x += 30)
	{
		us_box.col = colors[colorindex++];
		if (colorindex >= numcolors) colorindex = 0;
		high = x + 30;
		if (high > hx) high = hx;
		screendrawbox(w, x, high, ly, ly+WINDOWMODEBORDERSIZE-1, &us_box);
		screendrawbox(w, x, high, hy-WINDOWMODEBORDERSIZE+1, hy, &us_box);
	}
	colorindex = 0;
	for(y=ly; y<hy; y += 30)
	{
		us_box.col = colors[colorindex++];
		if (colorindex >= numcolors) colorindex = 0;
		high = y + 30;
		if (high > hy) high = hy;
		screendrawbox(w, lx, lx+WINDOWMODEBORDERSIZE-1, y, high, &us_box);
		screendrawbox(w, hx-WINDOWMODEBORDERSIZE+1, hx, y, high, &us_box);
	}
}

/*
 * routine to erase window "w" and draw everything that should be in it.
 */
void us_redisplay(WINDOWPART *w)
{
	WINDOWPART ww;
	static POLYGON *poly = NOPOLYGON;
	INTBIG lx, hx, ly, hy, numicons;
	REGISTER BOOLEAN drawexplorericon;

	/* get polygon */
	(void)needstaticpolygon(&poly, 7, us_tool->cluster);

	/* draw fat colored border if in a mode */
	if ((w->state&WINDOWMODE) != 0)
	{
		us_gettruewindowbounds(w, &lx, &hx, &ly, &hy);
		ww.screenlx = ww.uselx = lx;
		ww.screenhx = ww.usehx = hx;
		ww.screenly = ww.usely = ly;
		ww.screenhy = ww.usehy = hy;
		ww.frame = w->frame;
		ww.state = DISPWINDOW;
		computewindowscale(&ww);
		poly->desc = &us_box;
		us_box.col = RED;
		poly->style = FILLEDRECT;

		us_showwindowmode(w);
	}

	/* draw sliders if a display window */
	if ((w->state&WINDOWTYPE) == DISPWINDOW)
	{
		lx = w->uselx;   hx = w->usehx;
		ly = w->usely;   hy = w->usehy;

		/* the slider on the bottom and right */
		numicons = 0;
		drawexplorericon = us_windowgetsexploericon(w);
		if (drawexplorericon) numicons++;
		w->usehx += DISPLAYSLIDERSIZE;
		w->usely -= DISPLAYSLIDERSIZE;
		us_drawverticalslider(w, hx, ly, hy, FALSE);
		us_drawhorizontalslider(w, ly, lx, hx, numicons);
		w->usehx -= DISPLAYSLIDERSIZE;
		w->usely += DISPLAYSLIDERSIZE;

		/* the corner */
		us_drawslidercorner(w, hx+1, hx+DISPLAYSLIDERSIZE, ly-DISPLAYSLIDERSIZE, ly-1, FALSE);

		/* fill in the current slider positions */
		us_drawdispwindowsliders(w);
	}

	if ((us_state&NONOVERLAPPABLEDISPLAY) != 0) us_redisplaynow(w, TRUE); else
		us_redisplaynow(w, FALSE);
}

/*
 * Routine to return TRUE if an "explorer icon" should be drawn in this window.
 */
BOOLEAN us_windowgetsexploericon(WINDOWPART *w)
{
	REGISTER WINDOWPART *ow;

	if ((w->state&WINDOWTYPE) == EXPLORERWINDOW) return(TRUE);
	if ((w->state&WINDOWTYPE) != DISPWINDOW) return(FALSE);
	for(ow = el_topwindowpart; ow != NOWINDOWPART; ow = ow->nextwindowpart)
	{
		if (ow->frame != w->frame) continue;
		if ((ow->state&WINDOWTYPE) == EXPLORERWINDOW) return(FALSE);
	}
	return(TRUE);
}

/*
 * Routine to draw a grey box in the corner where two sliders meet.
 */
void us_drawslidercorner(WINDOWPART *win, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy, BOOLEAN drawtopright)
{
	static POLYGON *poly = NOPOLYGON;
	WINDOWPART ww;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	ww.screenlx = ww.uselx = lx;
	ww.screenhx = ww.usehx = hx;
	ww.screenly = ww.usely = ly;
	ww.screenhy = ww.usehy = hy;
	ww.frame = win->frame;
	ww.state = DISPWINDOW;
	computewindowscale(&ww);
	poly->desc = &us_box;
	us_box.col = DGRAY;
	poly->style = FILLEDRECT;
	maketruerectpoly(lx, hx, ly, hy, poly);
	us_showpoly(poly, &ww);

	/* draw the top and right edge */
	if (drawtopright)
	{
		us_box.col = BLACK;
		gra_drawbox(win, lx, lx+11, ly+11, ly+11, &us_box);
		gra_drawbox(win, lx+11, lx+11, ly, ly+11, &us_box);
	}
}

/*
 * Routine to determine the location of the thumb in the sliders of window "w"
 * and to draw those thumbs.
 */
void us_drawdispwindowsliders(WINDOWPART *w)
{
	REGISTER INTBIG lx, hx, ly, hy, numicons;
	REGISTER NODEPROTO *np;
	INTBIG cellsizex, cellsizey;
	REGISTER INTBIG screensizex, screensizey,
		thumbsizex, thumbsizey, thumbareax, thumbareay, thumbposx, thumbposy;

	if ((w->state&WINDOWTYPE) != DISPWINDOW) return;

	/* default: no thumb drawn in sliders */
	w->thumblx = 0;  w->thumbhx = -1;
	w->thumbly = 0;  w->thumbhy = -1;

	/* see how many icons appear on the left side of the horizontal slider */
	numicons = 0;
	if (us_windowgetsexploericon(w)) numicons++;

	np = w->curnodeproto;
	lx = w->uselx + numicons*DISPLAYSLIDERSIZE;
	hx = w->usehx;
	ly = w->usely;
	hy = w->usehy;
	if (np != NONODEPROTO)
	{
		/* determine amount of the cell that is shown */
		cellsizex = np->highx - np->lowx;
		cellsizey = np->highy - np->lowy;
		if (cellsizex > 0 && cellsizey >= 0)
		{
			screensizex = w->screenhx - w->screenlx;
			screensizey = w->screenhy - w->screenly;
			thumbareax = (hx - lx - DISPLAYSLIDERSIZE*2-4) / 2;
			thumbareay = (hy - ly - DISPLAYSLIDERSIZE*2-4) / 2;
			if (cellsizex <= screensizex)
			{
				thumbsizex = thumbareax;
				thumbposx = muldiv(w->screenhx - (np->highx + np->lowx)/2, thumbareax,
					w->screenhx-w->screenlx) + (hx - lx - thumbareax) / 2 + lx;
			} else
			{
				thumbsizex = thumbareax * screensizex / cellsizex;
				if (thumbsizex < 20) thumbsizex = 20;
				thumbposx = muldiv((w->screenhx + w->screenlx)/2 - np->lowx, thumbareax,
					np->highx-np->lowx) + (hx - lx - thumbareax) / 2 + lx;
			}
			if (cellsizey <= screensizey)
			{
				thumbsizey = thumbareay;
				thumbposy = muldiv(w->screenhy - (np->highy + np->lowy)/2, thumbareay,
					w->screenhy-w->screenly) + (hy - ly - thumbareay) / 2 + ly;
			} else
			{
				thumbsizey = thumbareay * screensizey / cellsizey;
				if (thumbsizey < 20) thumbsizey = 20;
				thumbposy = muldiv((w->screenhy + w->screenly)/2 - np->lowy, thumbareay,
					np->highy-np->lowy) + (hy - ly - thumbareay) / 2 + ly;
			}
			w->thumblx = thumbposx-thumbsizex/2;  w->thumbhx = thumbposx+thumbsizex/2;
			w->thumbly = thumbposy-thumbsizey/2;  w->thumbhy = thumbposy+thumbsizey/2;
			if (w->thumblx < lx + DISPLAYSLIDERSIZE + 2) w->thumblx = lx + DISPLAYSLIDERSIZE + 2;
			if (w->thumbhx < lx + DISPLAYSLIDERSIZE + 20) w->thumbhx = lx + DISPLAYSLIDERSIZE + 20;
			if (w->thumbhx > hx - DISPLAYSLIDERSIZE - 2) w->thumbhx = hx - DISPLAYSLIDERSIZE - 2;
			if (w->thumblx > hx - DISPLAYSLIDERSIZE - 20) w->thumblx = hx - DISPLAYSLIDERSIZE - 20;

			if (w->thumbly < ly + DISPLAYSLIDERSIZE + 2) w->thumbly = ly + DISPLAYSLIDERSIZE + 2;
			if (w->thumbhy < ly + DISPLAYSLIDERSIZE + 20) w->thumbhy = ly + DISPLAYSLIDERSIZE + 20;
			if (w->thumbhy > hy - DISPLAYSLIDERSIZE - 2) w->thumbhy = hy - DISPLAYSLIDERSIZE - 2;
			if (w->thumbly > hy - DISPLAYSLIDERSIZE - 20) w->thumbly = hy - DISPLAYSLIDERSIZE - 20;
		}
	}

	/* prepare a window in which to draw the thumbs */
	lx = w->uselx;   hx = w->usehx;
	ly = w->usely;   hy = w->usehy;

	/* now draw the thumbs */
	w->usehx += DISPLAYSLIDERSIZE;
	w->usely -= DISPLAYSLIDERSIZE;
	us_drawverticalsliderthumb(w, hx, ly, hy, w->thumbly, w->thumbhy);
	us_drawhorizontalsliderthumb(w, ly, lx, hx, w->thumblx, w->thumbhx, numicons);
	if (numicons > 0)
		us_drawexplorericon(w, lx, w->usely);
	w->usehx -= DISPLAYSLIDERSIZE;
	w->usely += DISPLAYSLIDERSIZE;
}

#define ARROWPOINTS 7
static INTBIG us_xarrowpoint[] = {1, DISPLAYSLIDERSIZE/2, DISPLAYSLIDERSIZE/2, DISPLAYSLIDERSIZE-1,
	DISPLAYSLIDERSIZE-1, DISPLAYSLIDERSIZE/2, DISPLAYSLIDERSIZE/2};
static INTBIG us_yarrowpoint[] = {1+DISPLAYSLIDERSIZE/2, DISPLAYSLIDERSIZE,
	1+DISPLAYSLIDERSIZE/3*2, 1+DISPLAYSLIDERSIZE/3*2, 2+DISPLAYSLIDERSIZE/3,
	2+DISPLAYSLIDERSIZE/3, 2};

/*
 * Routine to draw a vertical slider in window "w" whose left edge is "lx" and which
 * runs vertically from "ly" to "hy".  The slider has arrows and borders, but no thumb.
 * If "onleft" is true, this slider is on the left.
 */
void us_drawverticalslider(WINDOWPART *w, INTBIG lx, INTBIG ly, INTBIG hy, BOOLEAN onleft)
{
	static POLYGON *poly = NOPOLYGON;
	REGISTER INTBIG i;
	WINDOWPART ww;

	/* get polygon */
	(void)needstaticpolygon(&poly, 7, us_tool->cluster);

	ww.screenlx = ww.uselx = w->uselx;
	ww.screenhx = ww.usehx = w->usehx;
	ww.screenly = ww.usely = w->usely;
	ww.screenhy = ww.usehy = w->usehy;
	ww.frame = w->frame;
	ww.state = DISPWINDOW;
	computewindowscale(&ww);

	/* erase the area */
	poly->desc = &us_box;
	us_box.col = WHITE;
	poly->style = FILLEDRECT;
	maketruerectpoly(lx+1, lx+DISPLAYSLIDERSIZE, ly-1, hy, poly);
	us_showpoly(poly, &ww);

	/* draw arrows */
	poly->count = ARROWPOINTS;
	us_box.col = BLACK;
	poly->style = FILLED;
	for(i=0; i<ARROWPOINTS; i++)
	{
		poly->xv[i] = lx + us_yarrowpoint[i];
		poly->yv[i] = ly + us_xarrowpoint[i];
	}
	us_showpoly(poly, &ww);
	for(i=0; i<ARROWPOINTS; i++)
	{
		poly->xv[i] = lx + us_yarrowpoint[i];
		poly->yv[i] = hy - us_xarrowpoint[i];
	}
	us_showpoly(poly, &ww);

	/* draw border lines */
	poly->count = 2;
	us_box.col = BLACK;
	poly->style = OPENED;
	if (!onleft)
	{
		poly->xv[0] = lx+1;   poly->yv[0] = ly-DISPLAYSLIDERSIZE;
		poly->xv[1] = lx+1;   poly->yv[1] = hy;
		us_showpoly(poly, &ww);
	} else
	{
		poly->xv[0] = lx+DISPLAYSLIDERSIZE+1;   poly->yv[0] = ly-DISPLAYSLIDERSIZE;
		poly->xv[1] = lx+DISPLAYSLIDERSIZE+1;   poly->yv[1] = hy;
		us_showpoly(poly, &ww);
	}

	/* draw borders enclosing vertical arrows */
	poly->xv[0] = lx+1;                   poly->yv[0] = hy-DISPLAYSLIDERSIZE-1;
	poly->xv[1] = lx+DISPLAYSLIDERSIZE;   poly->yv[1] = hy-DISPLAYSLIDERSIZE-1;
	us_showpoly(poly, &ww);
	poly->xv[0] = lx+1;                   poly->yv[0] = ly+DISPLAYSLIDERSIZE+1;
	poly->xv[1] = lx+DISPLAYSLIDERSIZE;   poly->yv[1] = ly+DISPLAYSLIDERSIZE+1;
	us_showpoly(poly, &ww);
}

/*
 * Routine to draw a horizontal slider in window "w" whose top edge is "hy" and which
 * runs horizontally from "lx" to "hx".  The slider has arrows and borders, but no thumb.
 */
void us_drawhorizontalslider(WINDOWPART *w, INTBIG hy, INTBIG lx, INTBIG hx, INTBIG numicons)
{
	static POLYGON *poly = NOPOLYGON;
	REGISTER INTBIG i;
	WINDOWPART ww;

	/* get polygon */
	(void)needstaticpolygon(&poly, 7, us_tool->cluster);

	ww.screenlx = ww.uselx = w->uselx;
	ww.screenhx = ww.usehx = w->usehx;
	ww.screenly = ww.usely = w->usely;
	ww.screenhy = ww.usehy = w->usehy;
	ww.frame = w->frame;
	ww.state = DISPWINDOW;
	computewindowscale(&ww);

	/* erase the area */
	lx += numicons * DISPLAYSLIDERSIZE;
	poly->desc = &us_box;
	us_box.col = WHITE;
	poly->style = FILLEDRECT;
	maketruerectpoly(lx, hx+1, hy-DISPLAYSLIDERSIZE, hy-1, poly);
	us_showpoly(poly, &ww);

	/* draw arrows */
	poly->count = ARROWPOINTS;
	us_box.col = BLACK;
	poly->style = FILLED;
	for(i=0; i<ARROWPOINTS; i++)
	{
		poly->xv[i] = lx + us_xarrowpoint[i];
		poly->yv[i] = hy - us_yarrowpoint[i];
	}
	us_showpoly(poly, &ww);
	for(i=0; i<ARROWPOINTS; i++)
	{
		poly->xv[i] = hx - us_xarrowpoint[i];
		poly->yv[i] = hy - us_yarrowpoint[i];
	}
	us_showpoly(poly, &ww);

	/* draw border lines */
	poly->count = 2;
	us_box.col = BLACK;
	poly->style = OPENED;
	poly->xv[0] = lx;                     poly->yv[0] = hy-1;
	poly->xv[1] = hx+DISPLAYSLIDERSIZE;   poly->yv[1] = hy-1;
	us_showpoly(poly, &ww);

	/* draw borders enclosing vertical arrows */
	poly->xv[0] = lx+DISPLAYSLIDERSIZE+1;   poly->yv[0] = hy-DISPLAYSLIDERSIZE;
	poly->xv[1] = lx+DISPLAYSLIDERSIZE+1;   poly->yv[1] = hy-1;
	us_showpoly(poly, &ww);
	poly->xv[0] = hx-DISPLAYSLIDERSIZE-1;   poly->yv[0] = hy-DISPLAYSLIDERSIZE;
	poly->xv[1] = hx-DISPLAYSLIDERSIZE-1;   poly->yv[1] = hy-1;
	us_showpoly(poly, &ww);
}

/*
 * Routine to draw the thumb in the vertical slider of window "w".  The slider has
 * its left edge at "hx" and runs from "ly" to "hy".  The thumb runs from
 * "lt" to "ht" (if "lt" is greater than "ht", don't draw a thumb).
 */
void us_drawverticalsliderthumb(WINDOWPART *w, INTBIG hx, INTBIG ly, INTBIG hy,
	INTBIG lt, INTBIG ht)
{
	static POLYGON *poly = NOPOLYGON;
	WINDOWPART ww;

	/* get polygon */
	(void)needstaticpolygon(&poly, 7, us_tool->cluster);

	ww.screenlx = ww.uselx = w->uselx;
	ww.screenhx = ww.usehx = w->usehx;
	ww.screenly = ww.usely = w->usely;
	ww.screenhy = ww.usehy = w->usehy;
	ww.frame = w->frame;
	ww.state = DISPWINDOW;
	computewindowscale(&ww);

	poly->desc = &us_box;

	/* erase the vertical slider on the right */
	us_box.col = LGRAY;
	poly->style = FILLEDRECT;
	maketruerectpoly(hx+2, hx+DISPLAYSLIDERSIZE, ly+DISPLAYSLIDERSIZE+2, hy-DISPLAYSLIDERSIZE-2, poly);
	us_showpoly(poly, &ww);

	/* stop now if no thumb requested */
	if (lt > ht) return;

	/* draw the vertical thumb */
	us_box.col = BLACK;
	maketruerectpoly(hx+2, hx+DISPLAYSLIDERSIZE, lt, ht, poly);
	us_showpoly(poly, &ww);

	/* draw lines around the vertical thumb */
	poly->count = 4;
	poly->style = CLOSED;
	us_box.col = WHITE;
	poly->xv[0] = hx+3;                     poly->yv[0] = lt+1;
	poly->xv[1] = hx+3;                     poly->yv[1] = ht-1;
	poly->xv[2] = hx+DISPLAYSLIDERSIZE-1;   poly->yv[2] = ht-1;
	poly->xv[3] = hx+DISPLAYSLIDERSIZE-1;   poly->yv[3] = lt+1;
	us_showpoly(poly, &ww);
}

/*
 * Routine to draw the thumb in the horizontal slider of window "w".  The slider has
 * its top edge at "ly" and runs from "lx" to "hx".  The thumb runs from
 * "lt" to "ht" (if "lt" is greater than "ht", don't draw a thumb).
 * "numicons" is the number of icons on the left that will be drawn.
 */
void us_drawhorizontalsliderthumb(WINDOWPART *w, INTBIG ly, INTBIG lx, INTBIG hx, INTBIG lt, INTBIG ht, INTBIG numicons)
{
	static POLYGON *poly = NOPOLYGON;
	WINDOWPART ww;

	/* get polygon */
	(void)needstaticpolygon(&poly, 7, us_tool->cluster);

	ww.screenlx = ww.uselx = w->uselx;
	ww.screenhx = ww.usehx = w->usehx;
	ww.screenly = ww.usely = w->usely;
	ww.screenhy = ww.usehy = w->usehy;
	ww.frame = w->frame;
	ww.state = DISPWINDOW;
	computewindowscale(&ww);

	poly->desc = &us_box;
	lx += numicons * DISPLAYSLIDERSIZE;

	/* erase the horizontal slider on the bottom */
	us_box.col = LGRAY;
	poly->style = FILLEDRECT;
	maketruerectpoly(lx+DISPLAYSLIDERSIZE+2, hx-DISPLAYSLIDERSIZE-2, ly-DISPLAYSLIDERSIZE, ly-2, poly);
	us_showpoly(poly, &ww);

	/* stop now if no thumb requested */
	if (lt > ht) return;

	/* draw the horizontal thumb */
	us_box.col = BLACK;
	maketruerectpoly(lt, ht, ly-DISPLAYSLIDERSIZE, ly-2, poly);
	us_showpoly(poly, &ww);

	/* draw lines around the horizontal thumb */
	poly->count = 4;
	poly->style = CLOSED;
	us_box.col = WHITE;
	poly->xv[0] = lt+1;   poly->yv[0] = ly-DISPLAYSLIDERSIZE+1;
	poly->xv[1] = lt+1;   poly->yv[1] = ly-3;
	poly->xv[2] = ht-1;   poly->yv[2] = ly-3;
	poly->xv[3] = ht-1;   poly->yv[3] = ly-DISPLAYSLIDERSIZE+1;
	us_showpoly(poly, &ww);
}

/*
 * routine to erase window "w" and draw everything that should be in it.
 * If "now" is true, draw everything immediately.  Otherwise, draw
 * overlapping layers first and queue opaque layers for later.
 */
void us_redisplaynow(WINDOWPART *w, BOOLEAN now)
{
	REGISTER NODEPROTO *cell;
	REGISTER VARIABLE *var;
	REGISTER TECHNOLOGY *tech;
	REGISTER INTBIG i, j, lambda, alignment;
	INTBIG cenx, ceny, dummy;
	static POLYGON *poly = NOPOLYGON;

	/* begin accumulation of polygons if doing 3D drawing */
	if ((w->state&WINDOWTYPE) == DISP3DWINDOW) us_3dstartdrawing(w);

	/* erase the window */
	us_erasewindow(w);
	cell = w->curnodeproto;
	if (cell == NONODEPROTO)
	{
		us_showemptywindow(w);
		return;
	}

	/* set the environment window for hierarchy traversal */
	(void)setvariablewindow(w);

	/* show the background if editing in place */
	if ((w->state&INPLACEEDIT) != 0 && w->topnodeproto != NONODEPROTO)
	{
		/* indicate that the in-place surround is being drawn */
		w->state = (w->state & ~INPLACEEDIT) | SURROUNDINPLACEEDIT;

		/* draw the surround */
		us_drawcellcontents(w->topnodeproto, w, TRUE);

		/* dim the entire cell */
		(void)needstaticpolygon(&poly, 4, us_tool->cluster);
		maketruerectpoly(w->screenlx, w->screenhx, w->screenly, w->screenhy, poly);
		poly->desc = &us_dimbox;
		poly->style = FILLEDRECT;
		(*us_displayroutine)(poly, w);

		/* clear the area of the cell */
		makerectpoly(cell->lowx, cell->highx, cell->lowy, cell->highy, poly);
		poly->style = FILLED;
		xformpoly(poly, w->outofcell);
		poly->desc = &us_hbox;   us_hbox.col = 0;
		(*us_displayroutine)(poly, w);

		/* go back to drawing in-place */
		w->state = (w->state & ~SURROUNDINPLACEEDIT) | INPLACEEDIT;
	}

	/* draw the frame if there is one */
	if ((w->state&WINDOWTYPE) != DISP3DWINDOW)
	{
		j = framepolys(cell);
		if (j != 0)
		{
			/* get polygon */
			(void)needstaticpolygon(&poly, 6, us_tool->cluster);
			for(i=0; i<j; i++)
			{
				framepoly(i, poly, cell);
				(*us_displayroutine)(poly, w);
			}
		}
	}

	us_drawcellcontents(cell, w, now);

	/* show this cell and export variables */
	us_drawnodeprotovariables(cell, el_matid, w, FALSE);

	/* re-draw grid if on */
	if ((w->state&(GRIDON|GRIDTOOSMALL)) == GRIDON &&
		(w->state&WINDOWTYPE) != DISP3DWINDOW)
	{
		/* get polygon */
		(void)needstaticpolygon(&poly, 6, us_tool->cluster);

		/* grid spacing */
		lambda = lambdaofcell(cell);
		poly->xv[0] = muldiv(w->gridx, lambda, WHOLE);
		poly->yv[0] = muldiv(w->gridy, lambda, WHOLE);

		/* initial grid location */
		var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_gridfloatskey);
		if (var == NOVARIABLE || var->addr == 0)
		{
			poly->xv[1] = w->screenlx / poly->xv[0] * poly->xv[0];
			poly->yv[1] = w->screenly / poly->yv[0] * poly->yv[0];
		} else
		{
			grabpoint(cell, &cenx, &ceny);
			tech = cell->tech;
			if (tech == NOTECHNOLOGY) tech = el_curtech;
			alignment = muldiv(us_alignment_ratio, el_curlib->lambda[tech->techindex], WHOLE);
			poly->xv[1] = us_alignvalue(cenx, alignment, &dummy);
			poly->xv[1] += (w->screenlx-poly->xv[1]) / poly->xv[0] * poly->xv[0];
			poly->yv[1] = us_alignvalue(ceny, alignment, &dummy);
			poly->yv[1] += (w->screenly-poly->yv[1]) / poly->yv[0] * poly->yv[0];
		}

		/* display screen extent */
		poly->xv[2] = w->uselx;     poly->yv[2] = w->usely;
		poly->xv[3] = w->usehx;     poly->yv[3] = w->usehy;

		/* object space extent */
		poly->xv[4] = w->screenlx;  poly->yv[4] = w->screenly;
		poly->xv[5] = w->screenhx;  poly->yv[5] = w->screenhy;
		poly->count = 6;
		poly->style = GRIDDOTS;
		poly->desc = &us_gbox;
		(*us_displayroutine)(poly, w);
		flushscreen();
	}

	/* reset environment window for hierarchy traversal */
	(void)setvariablewindow(NOWINDOWPART);

	/* end accumulation of polygons if doing 3D drawing */
	if ((w->state&WINDOWTYPE) == DISP3DWINDOW) us_3denddrawing();
}

/*
 * Routine to draw the nodes and arcs in cell "cell".  Draw it in 1 pass if "now" is TRUE.
 */
void us_drawcellcontents(NODEPROTO *cell, WINDOWPART *w, BOOLEAN now)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER INTBIG ret;

	/* initialize hierarchy traversal */
	begintraversehierarchy();

	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (now)
		{
			if (us_drawcell(ni, LAYERA, el_matid, 3, w) < 0) break;
		} else
		{
			ret = us_drawcell(ni, LAYERA, el_matid, 1, w);
			if (ret < 0) break;
			if (ret == 2) us_queueopaque(ni->geom, FALSE);
		}
	}

	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (now)
		{
			if (us_drawarcinst(ai, LAYERA, el_matid, 3, w) < 0) break;
		} else
		{
			ret = us_drawarcinst(ai, LAYERA, el_matid, 1, w);
			if (ret < 0) break;
			if (ret == 2) us_queueopaque(ai->geom, FALSE);
		}
	}

	/* stop hierarchy traversal */
	endtraversehierarchy();
}

/* erase the screen area in window frame "mw" (all windows if "mw" is zero) */
void us_erasescreen(WINDOWFRAME *mw)
{
	WINDOWPART ww;
	static POLYGON *poly = NOPOLYGON;
	INTBIG swid, shei;
	REGISTER WINDOWFRAME *frame;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	for(frame = el_firstwindowframe; frame != NOWINDOWFRAME; frame = frame->nextwindowframe)
	{
		if (mw != NOWINDOWFRAME && frame != mw) continue;
		getwindowframesize(frame, &swid, &shei);
		ww.screenlx = ww.uselx = 0;   ww.screenhx = ww.usehx = swid-1;
		ww.screenly = ww.usely = 0;   ww.screenhy = ww.usehy = shei-1;
		ww.frame = frame;
		ww.state = DISPWINDOW;
		computewindowscale(&ww);
		maketruerectpoly(0, swid-1, 0, shei-1, poly);
		poly->desc = &us_ebox;
		poly->style = FILLEDRECT;
		(*us_displayroutine)(poly, &ww);
	}
}

/*
 * Routine to display a message in windows with no cell
 */
void us_showemptywindow(WINDOWPART *w)
{
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	if ((w->state&WINDOWTYPE) == DISP3DWINDOW) return;
	makerectpoly(w->screenlx, w->screenhx, w->screenly, w->screenhy, poly);
	if ((w->state&INPLACEEDIT) != 0)
		xformpoly(poly, w->outofcell);
	poly->style = TEXTBOX;
	poly->string = _("No cell in this window");
	TDCLEAR(poly->textdescript);
	TDSETSIZE(poly->textdescript, TXTSETPOINTS(20));
	poly->tech = el_curtech;
	poly->desc = &us_box;
	us_box.col = MENTXT;
	(*us_displayroutine)(poly, w);
}

/******************** GRID CONTROL ********************/

/*
 * routine to turn the grid on or off in window "w", according to "state"
 */
void us_gridset(WINDOWPART *w, INTBIG state)
{
	REGISTER NODEPROTO *np;
	REGISTER INTBIG x0, y0, lambda;

	if (stopping(STOPREASONDISPLAY)) return;

	if ((state&GRIDON) == 0)
	{
		(void)setval((INTBIG)w, VWINDOWPART, x_("state"), w->state & ~GRIDON, VINTEGER);
		return;
	}

	if ((w->state&WINDOWTYPE) == WAVEFORMWINDOW)
	{
		(void)setval((INTBIG)w, VWINDOWPART, x_("state"), (w->state & ~GRIDTOOSMALL) | GRIDON, VINTEGER);
		return;
	}

	np = w->curnodeproto;
	if (np == NONODEPROTO)
	{
		ttyputmsg(_("Edit a cell before manipulating the grid"));
		return;
	}

	np = w->curnodeproto;
	if (np != NONODEPROTO)
	{
		lambda = np->lib->lambda[np->tech->techindex];
		x0 = applyxscale(w, muldiv(w->gridx, lambda, WHOLE));
		y0 = applyxscale(w, muldiv(w->gridy, lambda, WHOLE));
		if (x0 <= 2 || y0 <= 2)
		{
			if ((w->state&GRIDTOOSMALL) == 0)
			{
				ttyputverbose(M_("Grid too small, turned off"));
				(void)setval((INTBIG)w, VWINDOWPART, x_("state"), w->state | GRIDTOOSMALL | GRIDON, VINTEGER);
			}
			return;
		}
	}

	if ((w->state&GRIDTOOSMALL) != 0) ttyputverbose(M_("Grid turned back on"));
	(void)setval((INTBIG)w, VWINDOWPART, x_("state"), (w->state & ~GRIDTOOSMALL) | GRIDON, VINTEGER);
}

/******************** PORT SELECTION ******************/

typedef struct
{
	INTBIG     portx, porty;
	PORTPROTO *portpp;
	INTBIG     portangle;
} SHOWNPORTS;

/*
 * routine to identify the port locations by number in a nodeinst.  If "count" is
 * nonzero, identify "count" nodes in the list "nilist".  Otherwise identify the
 * exports of cell "np".  Draw these ports in layer "on" and use numbers instead
 * of names if "usenumbers" is true.
 */
void us_identifyports(INTBIG count, NODEINST **nilist, NODEPROTO *np, INTBIG on, BOOLEAN usenumbers)
{
	REGISTER PORTPROTO *pp, *subpp;
	REGISTER NODEPROTO *parent;
	REGISTER NODEINST *subni, *ni;
	REGISTER INTBIG digitindentx, digitindenty, total, *portx, *porty, numperside,
		leftsidecount, topsidecount, rightsidecount, botsidecount, bestoff,
		i, j, dist, bestdist, ignored, halfsizex, halfsizey;
	INTBIG x, y, xout, yout, tsx, tsy, sidex[4], sidey[4];
	CHAR line[80];
	REGISTER WINDOWPART *w;
	REGISTER SHOWNPORTS *portlist;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* determine the window to use */
	if (count == 0) parent = np; else parent = nilist[0]->parent;
	if (parent == getcurcell()) w = el_curwindowpart; else
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
			if (parent == w->curnodeproto) break;
	if (w == NOWINDOWPART) return;

	/* determine spacing of lines from edge of window */
	digitindentx = (w->screenhx - w->screenlx) / 15;
	digitindenty = (w->screenhy - w->screenly) / 15;

	/* count the ports */
	total = ignored = 0;
	if (count == 0)
	{
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			subni = pp->subnodeinst;
			subpp = pp->subportproto;
			portposition(subni, subpp, &x, &y);
			if ((w->state&INPLACEEDIT) != 0)
				xform(x, y, &x, &y, w->outofcell);
			if (x < w->screenlx || x > w->screenhx ||
				y < w->screenly || y > w->screenhy) ignored++; else
					total++;
		}
	} else
	{
		for(i=0; i<count; i++)
		{
			ni = nilist[i];
			for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			{
				portposition(ni, pp, &x, &y);
				if ((w->state&INPLACEEDIT) != 0)
					xform(x, y, &x, &y, w->outofcell);
				if (x < w->screenlx || x > w->screenhx ||
					y < w->screenly || y > w->screenhy) ignored++; else
						total++;
			}
		}
	}
	if (total == 0)
	{
		if (ignored <= 0)
		{
			if (count == 0)
				ttyputmsg(_("There are no ports on cell %s"), describenodeproto(np)); else
					ttyputmsg(_("There are no ports on the node(s)"));
		} else
		{
			ttyputmsg(_("All %ld ports are outside of the window"), ignored);
		}
		return;
	}

	/* allocate space for the port information */
	portx = (INTBIG *)emalloc(total * SIZEOFINTBIG, el_tempcluster);
	if (portx == 0) return;
	porty = (INTBIG *)emalloc(total * SIZEOFINTBIG, el_tempcluster);
	if (porty == 0) return;
	portlist = (SHOWNPORTS *)emalloc(total * (sizeof (SHOWNPORTS)), el_tempcluster);
	if (portlist == 0) return;
	numperside = (total + 3) / 4;
	leftsidecount = topsidecount = rightsidecount = botsidecount = numperside;
	if (leftsidecount + topsidecount + rightsidecount + botsidecount > total)
		botsidecount--;
	if (leftsidecount + topsidecount + rightsidecount + botsidecount > total)
		topsidecount--;
	if (leftsidecount + topsidecount + rightsidecount + botsidecount > total)
		rightsidecount--;
	j = 0;
	for(i=0; i<leftsidecount; i++)
	{
		portx[j] = w->screenlx + digitindentx;
		porty[j] = (w->screenhy - w->screenly) / (leftsidecount+1) * (i+1) + w->screenly;
		j++;
	}
	for(i=0; i<topsidecount; i++)
	{
		portx[j] = (w->screenhx - w->screenlx) / (topsidecount+1) * (i+1) + w->screenlx;
		porty[j] = w->screenhy - digitindenty;
		j++;
	}
	for(i=0; i<rightsidecount; i++)
	{
		portx[j] = w->screenhx - digitindentx;
		porty[j] = w->screenhy - (w->screenhy - w->screenly) / (rightsidecount+1) * (i+1);
		j++;
	}
	for(i=0; i<botsidecount; i++)
	{
		portx[j] = w->screenhx - (w->screenhx - w->screenlx) / (botsidecount+1) * (i+1);
		porty[j] = w->screenly + digitindenty;
		j++;
	}
	for(i=0; i<total; i++)
	{
		if ((w->state&INPLACEEDIT) != 0)
			xform(portx[i], porty[i], &portx[i], &porty[i], w->intocell);
	}

	/* associate ports with display locations */
	if (count == 0)
	{
		i = 0;
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			subni = pp->subnodeinst;
			subpp = pp->subportproto;
			portposition(subni, subpp, &x, &y);
			xout = x;   yout = y;
			if ((w->state&INPLACEEDIT) != 0)
				xform(xout, yout, &xout, &yout, w->outofcell);
			if (xout < w->screenlx || xout > w->screenhx ||
				yout < w->screenly || yout > w->screenhy) continue;

			portlist[i].portx = x;
			portlist[i].porty = y;
			portlist[i].portpp = pp;
			i++;
		}
	} else
	{
		i = 0;
		for(j=0; j<count; j++)
		{
			ni = nilist[j];
			for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			{
				portposition(ni, pp, &x, &y);
				xout = x;   yout = y;
				if ((w->state&INPLACEEDIT) != 0)
					xform(xout, yout, &xout, &yout, w->outofcell);
				if (xout < w->screenlx || xout > w->screenhx ||
					yout < w->screenly || yout > w->screenhy) continue;

				portlist[i].portx = x;
				portlist[i].porty = y;
				portlist[i].portpp = pp;
				i++;
			}
		}
	}

	/* build a sorted list of ports around the center */
	x = y = 0;
	for(i=0; i<total; i++)
	{
		x += portlist[i].portx;
		y += portlist[i].porty;
	}
	x /= total;   y /= total;
	for(i=0; i<total; i++)
	{
		if (x == portlist[i].portx && y == portlist[i].porty)
			portlist[i].portangle = 0; else
				portlist[i].portangle = -figureangle(x, y, portlist[i].portx, portlist[i].porty);
	}
	esort(portlist, total, sizeof(SHOWNPORTS), us_sortshownports);

	/* figure out the best rotation offset */
	bestdist = 0;
	for(i=0; i<total; i++)
	{
		dist = 0;
		for(j=0; j<total; j++)
			dist += computedistance(portx[j],porty[j],
				portlist[(j+i)%total].portx,portlist[(j+i)%total].porty);
		if (dist < bestdist || i == 0)
		{
			bestoff = i;
			bestdist = dist;
		}
	}

	/* show the ports */
	for(i=0; i<total; i++)
	{
		if (usenumbers)
		{
			(void)esnprintf(line, 80, x_("%ld"), i);
			poly->string = line;
		} else
		{
			poly->string = portlist[(bestoff+i)%total].portpp->protoname;
		}

		/* draw the port index number */
		poly->xv[0] = portx[i];   poly->yv[0] = porty[i];   poly->count = 1;
		poly->desc = &us_hbox;
		poly->style = TEXTCENT;
		TDCLEAR(poly->textdescript);
		TDSETSIZE(poly->textdescript, TXTSETPOINTS(20));
		poly->tech = el_curtech;
		us_hbox.col = HIGHLIT&on;
		us_showpoly(poly, w);
		(void)estrcpy(line, x_(""));

		/* draw the line from the port to the index number */
		poly->xv[0] = portlist[(bestoff+i)%total].portx;
		poly->yv[0] = portlist[(bestoff+i)%total].porty;
		screensettextinfo(w, el_curtech, poly->textdescript);
		screengettextsize(w, poly->string, &tsx, &tsy);
		halfsizex = (INTBIG)((tsx+2) / w->scalex / 2.0f + 0.5f);
		halfsizey = (INTBIG)((tsy+2) / w->scaley / 2.0f + 0.5f);
		sidex[0] = portx[i]-halfsizex;   sidey[0] = porty[i];
		sidex[1] = portx[i]+halfsizex;   sidey[1] = porty[i];
		sidex[2] = portx[i];             sidey[2] = porty[i]-halfsizey;
		sidex[3] = portx[i];             sidey[3] = porty[i]+halfsizey;
		for(j=0; j<4; j++)
		{
			dist = computedistance(poly->xv[0], poly->yv[0], sidex[j], sidey[j]);
			if (j != 0 && dist >= bestdist) continue;
			bestdist = dist;
			poly->xv[1] = sidex[j];   poly->yv[1] = sidey[j];
		}
		poly->count = 2;
		poly->desc = &us_arbit;
		poly->style = VECTORS;
		us_arbit.bits = LAYERH;   us_arbit.col = HIGHLIT&on;
		us_showpoly(poly, w);
	}
	flushscreen();
	efree((CHAR *)portx);
	efree((CHAR *)porty);
	efree((CHAR *)portlist);
	if (ignored > 0)
		ttyputmsg(_("Could not display %ld %s (outside of the window)"), ignored,
			makeplural(_("port"), ignored));
}

/*
 * Helper routine for "esort" that makes shown ports go in proper circular order.
 */
int us_sortshownports(const void *e1, const void *e2)
{
	REGISTER SHOWNPORTS *s1, *s2;

	s1 = (SHOWNPORTS *)e1;
	s2 = (SHOWNPORTS *)e2;
	return(s1->portangle - s2->portangle);
}

/******************** NODE/ARC DISPLAY ********************/

/*
 * routine to draw nodeinst "ni" when transformed through "prevtrans".
 * If "on" is nonzero, draw it, otherwise erase it.  If "layers" is
 * 1, only transparent layers are drawn.  If "layers" is 2, only
 * opaque layers are drawn.  If "layers" is 3, all layers are drawn.
 * The nodeinst is drawin in window "w".  The routine returns a layers code
 * to indicate any other layers that must be drawn.  It returns negative
 * if the display has been interrupted.  It is assumed that this
 * nodeinst is in the current cell and must be transformed properly first.
 */
INTBIG us_drawcell(NODEINST *ni, INTBIG on, XARRAY prevtrans, INTBIG layers, WINDOWPART *w)
{
	XARRAY localtran, trans;
	REGISTER INTBIG res;

	/* make transformation matrix within the current nodeinst */
	if (ni->rotation == 0 && ni->transpose == 0)
	{
		res = us_drawnodeinst(ni, on, prevtrans, layers, w);
	} else
	{
		makerot(ni, localtran);
		transmult(localtran, prevtrans, trans);
		res = us_drawnodeinst(ni, on, trans, layers, w);
	}
	if (res == -2) return(-1);
	if (res == -1) res = 0;
	return(res);
}

/*
 * routine to draw nodeinst "ni" when transformed through "prevtrans".
 * If "on" is nonzero, draw it, otherwise erase it.  The parameter
 * "layers" determines which parts of the nodeinst to draw: 1 for
 * transparent layers, 2 for opaque layers, and 3 for both.
 * No assumptions about the location of the nodeinst are made: it is
 * displayed exactly as it is transformed.  The routine returns
 * -2 if display has been interrupted, -1 if the nodeinst is off the screen,
 * 1 if there are transparent layers to be drawn, 2 if there are opaque layers
 * to be drawn, and 0 if nothing else needs to be drawn.
 */
INTBIG us_drawnodeinst(NODEINST *ni, INTBIG on, XARRAY prevtrans, INTBIG layers, WINDOWPART *w)
{
	REGISTER INTBIG i, j, res, displaytotal, low, high, cutsizex, cutsizey, portstyle,
		ratio, swap, lambda, size;
	INTBIG color, bits, moretodo;
	XARRAY localtran, subrot, trans, *xptr, cliptrans;
	INTBIG bx, by, ux, uy, portcliplx, portcliphx, portcliply, portcliphy, drawport,
		reasonable;
	static POLYGON *poly = NOPOLYGON;
	REGISTER GRAPHICS *gra;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER NODEINST *ino;
	REGISTER ARCINST *iar;
	REGISTER VARIABLE *var;

	/* initialize for drawing of cells */
	us_stopevent++;
	if ((us_stopevent%25) == 0)
		if (stopping(STOPREASONDISPLAY)) return(-2);

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	moretodo = 0;
	np = ni->proto;

	/* get outline of nodeinst in the window */
	if ((w->state&INPLACEEDIT) == 0) transcpy(prevtrans, cliptrans); else
		transmult(prevtrans, w->outofcell, cliptrans);
	if (ismanhattan(cliptrans))
	{
		/* manhattan orientation, so only examine 2 diagonal corner points */
		xform(ni->lowx, ni->lowy, &bx, &by, cliptrans);
		xform(ni->highx, ni->highy, &ux, &uy, cliptrans);
		if (bx > ux) { swap = bx;   bx = ux;   ux = swap; }
		if (by > uy) { swap = by;   by = uy;   uy = swap; }
	} else
	{
		/* nonmanhattan orientation, must examine all 4 corner points */
		bx = ni->lowx;   ux = ni->highx;
		by = ni->lowy;   uy = ni->highy;
		xformbox(&bx, &ux, &by, &uy, cliptrans);
	}

	/* check for being off screen and return error message */
	if (bx > w->screenhx || ux < w->screenlx || by > w->screenhy || uy < w->screenly)
		return(-1);

	/* primitive nodeinst: ask the technology how to draw it */
	if (np->primindex != 0)
	{
		i = nodepolys(ni, &reasonable, w);

		/* if the amount of geometry can be reduced, do so at large scales */
		if (i != reasonable)
		{
			cutsizex = applyxscale(w, ni->highx - ni->lowx);
			cutsizey = applyyscale(w, ni->highy - ni->lowy);
			if (cutsizex*cutsizey < i * 75) i = reasonable;
		}
		if ((us_state&NONOVERLAPPABLEDISPLAY) != 0)
		{
			/* no overlappable display: just draw everything */
			low = 0;   high = i;
		} else
		{
			if ((np->userbits&NHASOPA) == 0) j = i; else
			{
				j = (np->userbits&NFIRSTOPA) >> NFIRSTOPASH;
				if (i < j) j = i;
			}
			if ((layers&1) != 0) low = 0; else low = j;
			if ((layers&2) != 0) high = i; else high = j;
			if (low > 0) moretodo |= 1;
			if (high < i) moretodo |= 2;
		}

		/* don't draw invisible pins to alternative output */
		if (us_displayroutine != us_showpoly)
			if (np == gen_invispinprim && low == 0) low++;

		for(j=low; j<high; j++)
		{
			/* get description of this layer */
			shapenodepoly(ni, j, poly);

			/* ignore if this layer is not being displayed */
			gra = poly->desc;
			if ((gra->colstyle&INVISIBLE) != 0) continue;

			/* ignore if no bits are to be drawn */
			if ((on&gra->col) == 0 && on != 0) continue;

			/* draw the nodeinst */
			xformpoly(poly, prevtrans);

			/* save the color information and update it */
			i = gra->col;
			gra->col &= on;
			/* draw the nodeinst and restore the color */
			if ((w->state&WINDOWTYPE) == DISP3DWINDOW) us_3dshowpoly(poly, w); else
				(*us_displayroutine)(poly, w);
			gra->col = i;
		}
	} else
	{
		/* draw a cell */
		if (on == 0)
		{
			/* cell off: undraw the cell center if it is defined */
			poly->desc = &us_cellgra;   us_cellgra.col = ALLOFF;
			var = getvalkey((INTBIG)ni->proto, VNODEPROTO, VINTEGER|VISARRAY, el_prototype_center_key);
			if (var != NOVARIABLE)
			{
				poly->xv[0] = ((INTBIG *)var->addr)[0] + (ni->lowx+ni->highx)/2 -
					(ni->proto->lowx+ni->proto->highx)/2;
				poly->yv[0] = ((INTBIG *)var->addr)[1] + (ni->lowy+ni->highy)/2 -
					(ni->proto->lowy+ni->proto->highy)/2;
				poly->count = 1;
				poly->style = CROSS;
				xformpoly(poly, prevtrans);
				(*us_displayroutine)(poly, w);
			}

			/* cell is off, so simply blank the area */
			us_getnodebounds(ni, &bx, &ux, &by, &uy);

			/* extend the bounds by 1 pixel */
			i = roundfloat(1.0f / w->scalex);
			if (i <= 0) i = 1;
			maketruerectpoly(bx-i, ux+i, by-i, uy+i, poly);
			xformpoly(poly, prevtrans);

			/* because "us_getnodebounds()" returns rotated area, "prevtrans" over-rotates */
			if (ni->rotation != 0 || ni->transpose != 0)
			{
				makerotI(ni, localtran);
				xformpoly(poly, localtran);
			}
			poly->style = FILLEDRECT;
			(*us_displayroutine)(poly, w);
			return(0);
		}

		/* if drawing in-place surround, do not draw the cell being edited */
		if ((w->state&SURROUNDINPLACEEDIT) != 0)
		{
			NODEINST **nilist;
			INTBIG *indexlist, depth, inplacedepth;
			gettraversalpath(ni->parent, w, &nilist, &indexlist, &depth, 0);
			inplacedepth = w->inplacedepth-1;
			if (ni == w->inplacestack[inplacedepth])
			{
				if (depth >= inplacedepth)
				{
					for(i=0; i<inplacedepth; i++)
						if (w->inplacestack[i] != nilist[depth-inplacedepth+i]) break;
					if (i >= inplacedepth) return(0);
				}
			}
		}

		/* transform into the nodeinst for display of its guts */
		maketrans(ni, localtran);
		transmult(localtran, prevtrans, subrot);

		/* get cell rectangle */
		maketruerectpoly(ni->lowx, ni->highx, ni->lowy, ni->highy, poly);
		poly->style = CLOSEDRECT;
		xformpoly(poly, prevtrans);
		getbbox(poly, &portcliplx, &portcliphx, &portcliply, &portcliphy);
		(void)us_makescreen(&portcliplx, &portcliply, &portcliphx, &portcliphy, w);

		/* if cell is not expanded, draw its outline, name and ports */
		if ((ni->userbits & NEXPAND) == 0)
		{
			/* draw the unexpanded cell */
			if ((layers&2) != 0)
			{
				/* draw the cell outline as four vectors */
				poly->desc = &us_cellgra;   us_cellgra.col = el_colcell;
				poly->layer = -1;
				if ((w->state&WINDOWTYPE) == DISP3DWINDOW) us_3dshowpoly(poly, w); else
				{
					(*us_displayroutine)(poly, w);
				}

				/* write the cell name */
				if (TDGETPOS(ni->textdescript) == VTPOSBOXED)
				{
					makerectpoly(ni->lowx, ni->highx, ni->lowy, ni->highy, poly);
					poly->style = FILLED;
				} else
				{
					poly->count = 1;
					poly->xv[0] = (ni->lowx + ni->highx) / 2;
					poly->yv[0] = (ni->lowy + ni->highy) / 2;
				}
				adjustdisoffset((INTBIG)ni, VNODEINST, ni->proto->tech, poly, ni->textdescript);
				xformpoly(poly, prevtrans);
				poly->desc = &us_cellgra;   us_cellgra.col = el_colcelltxt;
				poly->string = describenodeproto(ni->proto);
				size = TDGETSIZE(poly->textdescript);
				if (TXTGETQLAMBDA(size) != 0)
				{
					size = truefontsize(size, w, poly->tech);
					if (size > TXTMAXCELLSIZE) size = TXTMAXCELLSIZE;
					TDSETSIZE(poly->textdescript, TXTSETPOINTS(size));
				}
				if ((w->state&WINDOWTYPE) == DISP3DWINDOW) us_3dshowpoly(poly, w); else
					(*us_displayroutine)(poly, w);

				/* see if there are displayable variables on the cell */
				displaytotal = tech_displayablenvars(ni, w, &tech_oneprocpolyloop);
				for(i = 0; i < displaytotal; i++)
				{
					(void)tech_filldisplayablenvar(ni, poly, w, 0, &tech_oneprocpolyloop);
					xformpoly(poly, prevtrans);
					if ((w->state&WINDOWTYPE) == DISP3DWINDOW) us_3dshowpoly(poly, w); else
						(*us_displayroutine)(poly, w);
				}

				/* draw the cell center if it is defined */
				var = getvalkey((INTBIG)ni->proto, VNODEPROTO, VINTEGER|VISARRAY, el_prototype_center_key);
				if (var != NOVARIABLE)
				{
					poly->xv[0] = ((INTBIG *)var->addr)[0] + (ni->lowx+ni->highx)/2 -
						(ni->proto->lowx+ni->proto->highx)/2;
					poly->yv[0] = ((INTBIG *)var->addr)[1] + (ni->lowy+ni->highy)/2 -
						(ni->proto->lowy+ni->proto->highy)/2;
					poly->count = 1;
					poly->style = CROSS;
					xformpoly(poly, prevtrans);
					if ((w->state&WINDOWTYPE) == DISP3DWINDOW) us_3dshowpoly(poly, w); else
						(*us_displayroutine)(poly, w);
				}
			} else moretodo |= 2;
			if (ni->parent == w->curnodeproto)
			{
				for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				{
					portstyle = (us_useroptions&PORTLABELS) >> PORTLABELSSH;
					if ((pp->userbits&PORTDRAWN) == 0)
					{
						/* if there is an arc or further export on this port, don't draw it */
						for (pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
							if (pi->proto == pp) break;
						for (pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
							if (pe->proto == pp) break;
						if (pi != NOPORTARCINST || pe != NOPORTEXPINST) continue;
					}

					/* don't bother plotting if no bits are to be drawn */
					us_graphicsarcs(pp, &bits, &color);
					if ((bits&LAYEROE) != 0 && (layers&2) == 0)
					{ moretodo |= 2;  continue; }
					if ((bits&LAYEROE) == 0 && (layers&1) == 0)
					{ moretodo |= 1;  continue; }

					if (pp->subnodeinst->rotation == 0 && pp->subnodeinst->transpose == 0)
					{
						us_writeprotoname(pp, LAYERA, subrot, bits, color, w, portcliplx,
							portcliphx, portcliply, portcliphy, portstyle);
					} else
					{
						makerot(pp->subnodeinst, localtran);
						transmult(localtran, subrot, trans);
						us_writeprotoname(pp, LAYERA, trans, bits, color, w, portcliplx,
							portcliphx, portcliply, portcliphy, portstyle);
					}
				}
			}
		} else
		{
			/* if the resolution is too fine, just draw texture pattern */
			if ((us_useroptions&DRAWTINYCELLS) == 0 && (w->state&WINDOWTYPE) != DISP3DWINDOW)
			{
				lambda = lambdaofcell(ni->parent);
				ratio = lambda * us_tinyratio / WHOLE;
				if ((w->screenhx - w->screenlx) / ratio > w->usehx - w->uselx &&
					(w->screenhy - w->screenly) / ratio > w->usehy - w->usely)
				{
					/* cannot sensibly draw the contents at this resolution: just draw a pattern */
					poly->desc = &us_graybox;   us_graybox.col = el_colcell;
					poly->style = FILLEDRECT;
					(*us_displayroutine)(poly, w);
					poly->desc = &us_cellgra;   us_cellgra.col = el_colcell;
					if (poly->style == FILLEDRECT) poly->style = CLOSEDRECT; else
						poly->style = CLOSED;
					(*us_displayroutine)(poly, w);
					return(0);
				}
			}

			/* write cell that is expanded */
			if ((layers&2) == 0) moretodo |= 2; else
			{
				/* write ports that must always be displayed */
				if (ni->parent == w->curnodeproto)
				{
					for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
					{
						drawport = 0;
						portstyle = (us_useroptions&PORTLABELS) >> PORTLABELSSH;
						if ((pp->userbits&PORTDRAWN) != 0) drawport = 1;
						for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
							if (pi->proto == pp) break;
						for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
							if (pe->proto == pp) break;
						if (pi == NOPORTARCINST && pe == NOPORTEXPINST)
							drawport = 1;

						/* determine the transformation for the instance */
						ino = pp->subnodeinst;
						if (ino->rotation == 0 && ino->transpose == 0)
						{
							xptr = (XARRAY *)subrot;
						} else
						{
							makerot(ino, localtran);
							transmult(localtran, subrot, trans);
							xptr = (XARRAY *)trans;
						}

						/* draw the export if requested */
						if (drawport != 0)
							us_writeprotoname(pp, LAYERA, *xptr, LAYERO, el_colcelltxt, w,
								portcliplx, portcliphx, portcliply, portcliphy, portstyle);

						/* draw attributes on exported ports */
						us_drawportprotovariables(pp, LAYERA, *xptr, w, TRUE);
					}
				}
			}

			/* descend hierarchy to this node */
			downhierarchy(ni, ni->proto, 0);

			/* search through cell */
			for(ino = np->firstnodeinst; ino != NONODEINST; ino = ino->nextnodeinst)
			{
				if ((ino->userbits&NVISIBLEINSIDE) != 0) continue;
				res = us_drawcell(ino, LAYERA, subrot, layers, w);
				if (res < 0) break;
				moretodo |= res;
			}
			for(iar = np->firstarcinst; iar != NOARCINST; iar = iar->nextarcinst)
			{
				res = us_drawarcinst(iar, LAYERA, subrot, layers, w);
				if (res < 0) { res = 0;   break; }
				moretodo |= res;
			}
			uphierarchy();

			/* draw text */
			if ((layers&2) != 0)
			{
				/* see if there are displayable variables on the cell instance */
				displaytotal = tech_displayablenvars(ni, w, &tech_oneprocpolyloop);
				for(i = 0; i < displaytotal; i++)
				{
					(void)tech_filldisplayablenvar(ni, poly, w, 0, &tech_oneprocpolyloop);
					xformpoly(poly, prevtrans);
					if ((w->state&WINDOWTYPE) == DISP3DWINDOW) us_3dshowpoly(poly, w); else
						(*us_displayroutine)(poly, w);
				}

				/* draw cell variables on expanded cells */
				us_drawnodeprotovariables(ni->proto, subrot, w, TRUE);
			}
		}
	}

	/* write export names if appropriate */
	if ((us_useroptions&HIDETXTEXPORT) == 0)
	{
		if (ni->firstportexpinst != NOPORTEXPINST && ni->parent == w->curnodeproto)
		{
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
			{
				if (on == 0)
				{
					us_writeprotoname(pe->exportproto, on, prevtrans, LAYERO,
						el_colcelltxt&on, w, 0, 0, 0, 0,
							(us_useroptions&EXPORTLABELS)>>EXPORTLABELSSH);
					us_drawportprotovariables(pe->exportproto, on, prevtrans, w, FALSE);
				} else
				{
					us_queueexport(pe->exportproto);
				}
			}
		}
	}
	return(moretodo);
}

/*
 * Routine to draw all attributes on cell "np".  The cell is in window "w"
 * and is transformed by "trans".  If "ignoreinherit" is true, ignore inheritable
 * attributes (because this is a subcell, and the inherited instance attributes are to be
 * shown).
 */
void us_drawnodeprotovariables(NODEPROTO *np, XARRAY trans, WINDOWPART *w, BOOLEAN ignoreinherit)
{
	REGISTER INTBIG i, displaytotal;
	REGISTER VARIABLE *var;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	displaytotal = tech_displayablecellvars(np, w, &tech_oneprocpolyloop);
	for(i = 0; i < displaytotal; i++)
	{
		var = tech_filldisplayablecellvar(np, poly, w, 0, &tech_oneprocpolyloop);
		if (ignoreinherit && TDGETINHERIT(var->textdescript) != 0) continue;
		xformpoly(poly, trans);
		if ((w->state&WINDOWTYPE) == DISP3DWINDOW) us_3dshowpoly(poly, w); else
			(*us_displayroutine)(poly, w);
	}
}

/*
 * Routine to draw all attributes on port "pp" with color "on".  The port is in window "w"
 * and is transformed by "trans".  If "ignoreinherit" is true, ignore inheritable
 * attributes (because this is a subcell, and the inherited instance attributes are to be
 * shown).
 */
void us_drawportprotovariables(PORTPROTO *pp, INTBIG on, XARRAY trans, WINDOWPART *w, BOOLEAN ignoreinherit)
{
	REGISTER INTBIG i, displaytotal;
	REGISTER VARIABLE *var;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	displaytotal = tech_displayableportvars(pp, w, &tech_oneprocpolyloop);
	for(i = 0; i < displaytotal; i++)
	{
		var = tech_filldisplayableportvar(pp, poly, w, 0, &tech_oneprocpolyloop);
		if (ignoreinherit && TDGETINHERIT(var->textdescript) != 0) continue;
		xformpoly(poly, trans);
		if (on == 0)
		{
			us_arbit = *poly->desc;
			us_arbit.col = 0;
			poly->desc = &us_arbit;
		}
		if ((w->state&WINDOWTYPE) == DISP3DWINDOW) us_3dshowpoly(poly, w); else
			(*us_displayroutine)(poly, w);
	}
}

/*
 * routine to determine the color of port "pp" by combining the colors
 * of all arcs that can connect to that port into the integers
 * "bitplanes" and "color".  The arcs under consideration are kept
 * in the array "connections"
 */
void us_graphicsarcs(PORTPROTO *pp, INTBIG *bitplanes, INTBIG *color)
{
	REGISTER INTBIG i;
	REGISTER ARCPROTO *outside;
	REGISTER ARCINST *ai;
	ARCINST arc;
	REGISTER TECHNOLOGY *tech;

	*bitplanes = *color = 0;
	outside = NOARCPROTO;
	ai = &arc;   initdummyarc(ai);
	ai->width = ai->length = 2000;
	ai->end[0].xpos = ai->end[0].ypos = ai->end[1].xpos = 0;
	ai->end[1].ypos = 2000;
	tech = NOTECHNOLOGY;
	for(i=0; pp->connects[i] != NOARCPROTO; i++)
	{
		ai->proto = pp->connects[i];
		if (tech != NOTECHNOLOGY && ai->proto->tech != tech)
		{
			outside = ai->proto;
			continue;
		}
		tech = ai->proto->tech;
		us_combinelayers(ai, bitplanes, color);
	}
	if (*bitplanes == 0 && outside != NOARCPROTO)
	{
		ai->proto = outside;
		us_combinelayers(ai, bitplanes, color);
	}
}

/*
 * helper routine for "us_graphicsarcs" that builds the integer "bitplanes"
 * and "color" with the layers in arcinst "ai".
 */
void us_combinelayers(ARCINST *ai, INTBIG *bitplanes, INTBIG *color)
{
	REGISTER INTBIG j, k;
	REGISTER INTBIG b, c;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	k = arcpolys(ai, NOWINDOWPART);
	for(j=0; j<k; j++)
	{
		shapearcpoly(ai, j, poly);
		b = poly->desc->bits;
		c = poly->desc->col;
		*bitplanes |= b;
		if (b == LAYERO) *color = c; else *color |= c;
	}
}

/*
 * routine to draw an arcinst.  Returns indicator of what else needs to
 * be drawn.  Returns negative if display interrupted
 */
INTBIG us_drawarcinst(ARCINST *ai, INTBIG on, XARRAY trans, INTBIG layers, WINDOWPART *w)
{
	REGISTER INTBIG i, j, low, high;
	REGISTER INTBIG moretodo;
	REGISTER ARCPROTO *ap;
	static POLYGON *poly = NOPOLYGON;

	/* ask the technology how to draw the arcinst */
	us_stopevent++;
	if ((us_stopevent%25) == 0)
		if (stopping(STOPREASONDISPLAY)) return(-1);

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	ap = ai->proto;
	i = arcpolys(ai, NOWINDOWPART);
	moretodo = 0;
	if ((us_state&NONOVERLAPPABLEDISPLAY) != 0)
	{
		/* no overlappable display: just draw everything */
		low = 0;   high = i;
	} else
	{
		if ((ap->userbits&AHASOPA) == 0) j = i; else
			j = mini((ap->userbits&AFIRSTOPA) >> AFIRSTOPASH, i);
		if (layers&1) low = 0; else low = j;
		if (layers&2) high = i; else high = j;
		if (low > 0) moretodo |= 1;
		if (high < i) moretodo |= 2;
	}

	/* get the endpoints of the arcinst */
	for(j=low; j<high; j++)
	{
		shapearcpoly(ai, j, poly);

		/* ignore if this layer is not to be drawn */
		if ((poly->desc->colstyle&INVISIBLE) != 0) continue;

		/* don't bother plotting if no bits are to be drawn */
		if ((on&poly->desc->col) == 0 && on != 0) continue;

		/* draw the arcinst */
		xformpoly(poly, trans);

		/* save the color information and update it */
		i = poly->desc->col;
		poly->desc->col &= on;

		/* draw the nodeinst and restore the color */
		if ((w->state&WINDOWTYPE) == DISP3DWINDOW) us_3dshowpoly(poly, w); else
			(*us_displayroutine)(poly, w);
		poly->desc->col = i;
	}
	return(moretodo);
}

/*
 * routine to write the name of portproto "pp" at the appropriate location,
 * transformed through "prevtrans".  The color of the text is "color" and
 * the bit planes in which to write the text is in "bits".  The text is
 * written if "on" is nonzero, erased if zero.  The port is written in window
 * "w".  If "portcliplx" is not equal to "portcliphx", then clip the port text
 * to within that range in X and to "portcliply/portcliphy" in Y.
 * The "dispstyle" of the port is:
 *   0  full port name written
 *   1  port drawn as a cross
 *   2  no port indication drawn
 *   3  short port name written
 */
void us_writeprotoname(PORTPROTO *pp, INTBIG on, XARRAY prevtrans, INTBIG bits,
	INTBIG color, WINDOWPART *w, INTBIG portcliplx, INTBIG portcliphx, INTBIG portcliply,
	INTBIG portcliphy, INTBIG dispstyle)
{
	XARRAY localtran, tempt1, tempt2, *t1, *t2, *swapt;
	INTBIG px, py;
	INTBIG wid, hei;
	REGISTER INTBIG sx, sy, lambda;
	static POLYGON *poly = NOPOLYGON;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *cell;
	REGISTER PORTPROTO *rpp;
	REGISTER TECHNOLOGY *tech;

	/* quit now if labels are off */
	if (dispstyle == 2) return;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* get technology and lambda */
	cell = pp->subnodeinst->parent;
	tech = cell->tech;
	lambda = cell->lib->lambda[tech->techindex];

	/* account for port offset */
	t1 = (XARRAY *)tempt1;   t2 = (XARRAY *)tempt2;
	if (dispstyle != 1 &&
		(TDGETXOFF(pp->textdescript) != 0 || TDGETYOFF(pp->textdescript) != 0))
	{
		transid(localtran);
		localtran[2][0] = TDGETXOFF(pp->textdescript);
		localtran[2][0] = localtran[2][0] * lambda / 4;
		localtran[2][1] = TDGETYOFF(pp->textdescript);
		localtran[2][1] = localtran[2][1] * lambda / 4;
		transmult(localtran, prevtrans, *t1);
	} else transcpy(prevtrans, *t1);

	/* build the rest of the transformation to the bottom */
	ni = pp->subnodeinst;
	rpp = pp->subportproto;
	while (ni->proto->primindex == 0)
	{
		maketrans(ni, localtran);
		transmult(localtran, *t1, *t2);
		swapt = t1;   t1 = t2;   t2 = swapt;
		ni = rpp->subnodeinst;
		rpp = rpp->subportproto;
		if (ni->rotation != 0 || ni->transpose != 0)
		{
			makerot(ni, localtran);
			transmult(localtran, *t1, *t2);
			swapt = t1;   t1 = t2;   t2 = swapt;
		}
		if (dispstyle != 1 && ni->proto->primindex == 0 &&
			(TDGETXOFF(rpp->textdescript) != 0 || TDGETYOFF(rpp->textdescript) != 0))
		{
			transid(localtran);
			localtran[2][0] = TDGETXOFF(rpp->textdescript);
			localtran[2][0] = localtran[2][0] * lambda / 4;
			localtran[2][1] = TDGETYOFF(rpp->textdescript);
			localtran[2][1] = localtran[2][1] * lambda / 4;
			transmult(localtran, *t1, *t2);
			swapt = t1;   t1 = t2;   t2 = swapt;
		}
	}

	/* get the center position of the port */
	shapetransportpoly(ni, rpp, poly, *t1);
	if (dispstyle == 1 || TDGETPOS(pp->textdescript) != VTPOSBOXED)
	{
		getcenter(poly, &px, &py);
		poly->xv[0] = px;   poly->yv[0] = py;   poly->count = 1;
	}

	/* simple description if only drawing ports as crosses */
	if (dispstyle == 1) poly->style = CROSS; else
	{
		/* writing text name: setup polygon */
		TDCOPY(poly->textdescript, pp->textdescript);
		poly->tech = pp->parent->tech;

		/* convert text description to polygon description */
		switch (TDGETPOS(pp->textdescript))
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

		/* get shortened name if requested */
		poly->string = us_displayedportname(pp, dispstyle);

		/* acount for node rotation, which port position */
		poly->style = rotatelabel(poly->style, TDGETROTATION(poly->textdescript), prevtrans);

		/* clip the port text to the cell bounds, if requested */
		if (portcliplx != portcliphx)
		{
			sx = applyxscale(w, px-w->screenlx) + w->uselx;
			sy = applyyscale(w, py-w->screenly) + w->usely;
			if (sx < portcliplx || sx > portcliphx || sy < portcliply || sy > portcliphy)
				return;
			screensettextinfo(w, tech, poly->textdescript);
			screengettextsize(w, poly->string, &wid, &hei);   wid++;   hei++;
			switch (poly->style)
			{
				case TEXTCENT:
				case TEXTBOT:
				case TEXTTOP:
					if (portcliphx - portcliplx < wid) poly->style = CROSS; else
					{
						if (sx-wid/2 < portcliplx) sx = portcliplx + wid/2;
						if (sx+wid/2 > portcliphx) sx = portcliphx - wid/2;
					}
					break;
				case TEXTRIGHT:
				case TEXTBOTRIGHT:
				case TEXTTOPRIGHT:
					if (portcliphx - portcliplx < wid) poly->style = CROSS; else
					{
						if (sx-wid < portcliplx) sx = portcliplx + wid;
						if (sx > portcliphx) sx = portcliphx;
					}
					break;
				case TEXTLEFT:
				case TEXTBOTLEFT:
				case TEXTTOPLEFT:
					if (portcliphx - portcliplx < wid) poly->style = CROSS; else
					{
						if (sx < portcliplx) sx = portcliplx;
						if (sx+wid > portcliphx) sx = portcliphx - wid;
					}
					break;
			}
			switch (poly->style)
			{
				case TEXTCENT:
				case TEXTRIGHT:
				case TEXTLEFT:
					if (portcliphy - portcliply < hei) poly->style = CROSS; else
					{
						if (sy-hei/2 < portcliply) sy = portcliply + hei/2;
						if (sy+hei/2 > portcliphy) sy = portcliphy - hei/2;
					}
					break;
				case TEXTBOT:
				case TEXTBOTRIGHT:
				case TEXTBOTLEFT:
					if (portcliphy - portcliply < hei) poly->style = CROSS; else
					{
						if (sy < portcliply) sy = portcliply;
						if (sy+hei > portcliphy) sy = portcliphy - hei;
					}
					break;
				case TEXTTOP:
				case TEXTTOPRIGHT:
				case TEXTTOPLEFT:
					if (portcliphy - portcliply < hei) poly->style = CROSS; else
					{
						if (sy-hei < portcliply) sy = portcliply + hei;
						if (sy > portcliphy) sy = portcliphy;
					}
					break;
			}
			poly->xv[0] = muldiv(sx - w->uselx, w->screenhx - w->screenlx,
				w->usehx - w->uselx) + w->screenlx;
			poly->yv[0] = muldiv(sy - w->usely, w->screenhy - w->screenly,
				w->usehy - w->usely) + w->screenly;
		}
	}

	/* set the graphics information */
	poly->desc = &us_arbit;
	poly->desc->col = color&on;
	poly->desc->bits = bits;
	poly->layer = -1;
	if ((w->state&WINDOWTYPE) == DISP3DWINDOW) us_3dshowpoly(poly, w); else
		(*us_displayroutine)(poly, w);
}

/*
 * routine to return the displayed port name for port "pp", given a style of "dispstyle"
 * (0: full, 1: crosses, 2: nothing, 3: short)
 */
CHAR *us_displayedportname(PORTPROTO *pp, INTBIG dispstyle)
{
	REGISTER CHAR *pt;
	REGISTER void *infstr;

	if (dispstyle == 3)
	{
		for(pt = pp->protoname; *pt != 0; pt++) if (ispunct(*pt)) break;
		if (*pt != 0)
		{
			infstr = initinfstr();
			for(pt = pp->protoname; !ispunct(*pt); pt++) addtoinfstr(infstr, *pt);
			return(returninfstr(infstr));
		}
	}
	return(pp->protoname);
}

/******************** PEEK DISPLAY ********************/

static WINDOWPART *us_peekwindow;

void us_dopeek(INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy, NODEPROTO *np, WINDOWPART *w)
{
	REGISTER INTBIG ret;

	us_peekwindow = w;

	/* draw everything in this area */
	begintraversehierarchy();
	ret = us_drawall(lx, hx, ly, hy, np, el_matid, 1, TRUE);
	endtraversehierarchy();

	if (ret == 2)
	{
		begintraversehierarchy();
		(void)us_drawall(lx, hx, ly, hy, np, el_matid, 2, TRUE);
		endtraversehierarchy();
	}
}

/*
 * routine to draw everything between "lx" and "hx" in X and between "ly"
 * and "hy" in Y of cell "np", given a transformation matrix of "trans".
 * If "layers" is 1 then only the transparent layers will be drawn and if
 * "layers" is 2 then only the opaque layers will be drawn.  Drawing is done
 * in window "w".  The routine returns the other layers that need to be drawn
 * (1 or 2).  If it returns -1, the display has been aborted.
 */
INTBIG us_drawall(INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy, NODEPROTO *np, XARRAY trans,
	INTBIG layers, BOOLEAN toplevel)
{
	XARRAY localtrans, localrot, thist, subrot, bound, *tbound, *tthis;
	INTBIG newlx, newhx, newly, newhy;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *subnt;
	REGISTER ARCINST *ai;
	REGISTER INTBIG search;
	REGISTER INTBIG i, moretodo;
	REGISTER GEOM *look;

	moretodo = 0;
	search = initsearch(lx, hx, ly, hy, np);
	for(;;)
	{
		/* check for interrupts */
		us_stopevent++;
		if ((us_stopevent%25) == 0)
			if (stopping(STOPREASONDISPLAY)) { termsearch(search);   break; }

		/* get the next object to draw */
		look = nextobject(search);
		if (look == NOGEOM) break;
		if (!look->entryisnode)
		{
			ai = look->entryaddr.ai;
			i = us_drawarcinstpeek(ai, trans, layers);
			if (i != -1) moretodo |= i;
		} else
		{
			ni = look->entryaddr.ni;
			if (ni->rotation == 0 && ni->transpose == 0)
			{
				transcpy(trans, thist);
				tthis = (XARRAY *)thist;
			} else
			{
				makerot(ni, localrot);   transmult(localrot, trans, thist);
				tthis = (XARRAY *)thist;
			}
			if (ni->proto->primindex != 0)
			{
				/* node is primitive: draw it */
				if (toplevel || (ni->userbits&NVISIBLEINSIDE) == 0)
				{
					i = us_drawnodeinstpeek(ni, *tthis, layers);
					if (i != -1) moretodo |= i;
				}
			} else
			{
				/* node is complex: recurse */
				subnt = ni->proto;
				downhierarchy(ni, subnt, 0);
				maketransI(ni, localtrans);
				if (ni->rotation == 0 && ni->transpose == 0)
					tbound = (XARRAY *)localtrans; else
				{
					makerotI(ni, localrot);
					transmult(localrot, localtrans, bound);
					tbound = (XARRAY *)bound;
				}

				/* compute bounding area inside of sub-cell */
				newlx = lx;   newly = ly;   newhx = hx;   newhy = hy;
				xformbox(&newlx, &newhx, &newly, &newhy, *tbound);

				/* compute new matrix for sub-cell display */
				localtrans[2][0] = -localtrans[2][0];
				localtrans[2][1] = -localtrans[2][1];
				transmult(localtrans, *tthis, subrot);
				i = us_drawall(newlx, newhx, newly, newhy, subnt, subrot, layers, FALSE);
				if (i != -1) moretodo |= i;
				uphierarchy();
			}
		}
	}
	if (stopping(STOPREASONDISPLAY)) return(-1);
	return(moretodo);
}

INTBIG us_drawarcinstpeek(ARCINST *ai, XARRAY trans, INTBIG layers)
{
	REGISTER INTBIG i, j, low, high;
	REGISTER INTBIG moretodo;
	REGISTER ARCPROTO *ap;
	REGISTER GRAPHICS *gra;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* determine which polygons to draw */
	ap = ai->proto;
	i = arcpolys(ai, us_peekwindow);
	moretodo = 0;
	if ((us_state&NONOVERLAPPABLEDISPLAY) != 0)
	{
		/* no overlappable display: just draw everything */
		low = 0;   high = i;
	} else
	{
		if ((ap->userbits&AHASOPA) == 0) j = i; else
		{
			j = (ap->userbits&AFIRSTOPA) >> AFIRSTOPASH;
			if (i < j) j = i;
		}
		if (layers&1) low = 0; else low = j;
		if (layers&2) high = i; else high = j;
		if (low > 0) moretodo |= 1;
		if (high < i) moretodo |= 2;
	}

	/* get the polygons of the arcinst */
	for(j=low; j<high; j++)
	{
		shapearcpoly(ai, j, poly);

		/* ignore if this layer is not to be drawn */
		gra = poly->desc;
		if ((gra->colstyle&INVISIBLE) != 0) continue;

		/* don't bother plotting if nothing to be drawn */
		if (gra->col == ALLOFF || gra->bits == LAYERN) continue;

		/* transform the polygon */
		xformpoly(poly, trans);

		/* draw the polygon */
		if ((us_peekwindow->state&WINDOWTYPE) == DISP3DWINDOW) us_3dshowpoly(poly, us_peekwindow); else
			(*us_displayroutine)(poly, us_peekwindow);
	}
	return(moretodo);
}

INTBIG us_drawnodeinstpeek(NODEINST *ni, XARRAY prevtrans, INTBIG layers)
{
	REGISTER INTBIG i, j, low, high, cutsizex, cutsizey;
	INTBIG moretodo;
	INTBIG reasonable;
	static POLYGON *poly = NOPOLYGON;
	REGISTER GRAPHICS *gra;
	REGISTER NODEPROTO *np;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	np = ni->proto;

	/* determine which polygons to draw */
	i = nodepolys(ni, &reasonable, us_peekwindow);

	/* if the amount of geometry can be reduced, do so at large scales */
	if (i != reasonable)
	{
		cutsizex = applyxscale(us_peekwindow, ni->highx - ni->lowx);
		cutsizey = applyyscale(us_peekwindow, ni->highy - ni->lowy);
		if (cutsizex*cutsizey < i * 75) i = reasonable;
	}
	moretodo = 0;
	if ((us_state&NONOVERLAPPABLEDISPLAY) != 0)
	{
		/* no overlappable display: just draw everything */
		low = 0;   high = i;
	} else
	{
		if ((np->userbits&NHASOPA) == 0) j = i; else
		{
			j = (np->userbits&NFIRSTOPA) >> NFIRSTOPASH;
			if (i < j) j = i;
		}
		if ((layers&1) != 0) low = 0; else low = j;
		if ((layers&2) != 0) high = i; else high = j;
		if (low > 0) moretodo |= 1;
		if (high < i) moretodo |= 2;
	}

	/* don't draw invisible pins to alternative output */
	if (np == gen_invispinprim && low == 0) low++;

	/* get the polygons of the nodeinst */
	for(j=low; j<high; j++)
	{
		/* get description of this layer */
		shapenodepoly(ni, j, poly);

		/* ignore if this layer is not being displayed */
		gra = poly->desc;
		if ((gra->colstyle&INVISIBLE) != 0) continue;

		/* ignore if no bits are to be drawn */
		if (gra->col == ALLOFF || gra->bits == LAYERN) continue;

		/* transform the polygon */
		xformpoly(poly, prevtrans);

		/* draw the polygon */
		if ((us_peekwindow->state&WINDOWTYPE) == DISP3DWINDOW) us_3dshowpoly(poly, us_peekwindow); else
			(*us_displayroutine)(poly, us_peekwindow);
	}
	return(moretodo);
}
