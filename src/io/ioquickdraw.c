/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: ioquickdraw.c
 * Input/output analysis tool: plotter output on Macintosh
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
#ifdef MACOS

#include "global.h"
#include "egraphics.h"
#include "eio.h"
#include "usr.h"
#include "efunction.h"

#include <Fonts.h>
#include <Printing.h>
#include <Scrap.h>
#include <QDOffscreen.h>

#define THRESH	         2000000
#ifdef __MWERKS__
#  if __MWERKS__ >= 0x2300
#    define NEWCODEWARRIOR    1
#  endif
#endif
#ifdef NEWCODEWARRIOR
#  define EFONT            kFontIDTimes
#  define HFONT            kFontIDHelvetica
#else
#  define EFONT            times
#  define HFONT            helvetica
#endif

GRAPHICS io_black = {LAYERO, BLACK, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};
static WINDOWPART *io_plotwindow;
static INTBIG      io_plotrevy;
static INTBIG      io_slx, io_shx, io_sly, io_shy;
static INTBIG      io_ulx, io_uhx, io_uly, io_uhy;
static WindowPtr   io_plotwin;
static INTBIG      io_thislayer, io_maxlayer;

void io_setplotlayer(INTBIG);
void io_setbounds(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, WindowPtr);
void io_plotpoly(POLYGON*, WINDOWPART*);
void io_plotdot(INTBIG, INTBIG);
void io_plotline(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void io_plotpolygon(INTBIG*, INTBIG*, INTBIG, INTBIG, GRAPHICS*);
void io_plottext(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, CHAR*);
void io_xform(INTBIG*, INTBIG*);
void io_maccopyhighlighted(void);

/*
 * routine to write out a file to the Macintosh/Quickdraw system
 */
BOOLEAN io_writequickdrawlibrary(LIBRARY *lib)
{
	REGISTER NODEPROTO *np;
	INTBIG err, ix, swid, shei;
	INTBIG hlx, hhx, hly, hhy, gridlx, gridly, gridx, gridy, cx, cy, prod1, prod2, i,
		revy, slx, shx, sly, shy, ulx, uhx, uly, uhy, sxs, sys, dxs, dys, saver, saveg,
		saveb, white, *curstate, lambda;
	VARIABLE *var;
	REGISTER VARIABLE *varred, *vargreen, *varblue;
	static POLYGON *poly = NOPOLYGON;
	static THPrint hPrint = 0;
	TPPrPort plotwin;
	TPrStatus prstat;
	Rect sr, dr;
	CGrafPtr window, gra_getoffscreen(WINDOWPART *);
	REGISTER void *infstr;

	np = lib->curnodeproto;
	if (np == NONODEPROTO)
	{
		if ((el_curwindowpart->state&WINDOWTYPE) == EXPLORERWINDOW)
		{
			ttyputmsg(_("Cannot convert Explorer window to Quickdraw.  Export PostScript instead"));
			return(TRUE);
		}
		ttyputerr(_("No current cell to plot"));
		return(TRUE);
	}

	/* cannot write text-only cells */
	if ((np->cellview->viewstate&TEXTVIEW) != 0)
	{
		ttyputerr(_("Cannot write textual cells"));
		return(TRUE);
	}

	/* get printing control bits */
	curstate = io_getstatebits();

	/* determine area to plot */
	if (io_getareatoprint(np, &slx, &shx, &sly, &shy, TRUE)) return(TRUE);

	/* open the window (to the printer) */
	PrOpen();
	if ((err = PrError()) != noErr)
	{
		ttyputerr(_("Printer error %ld"), err);
		return(TRUE);
	}

	/* see if any printing has already been done */
	if (hPrint == 0)
	{
		/* this is the first print: create print record and do "Page Setup" dialog */
		hPrint = (THPrint)NewHandle(sizeof (TPrint));
		if (hPrint == 0)
		{
			ttyputerr(_("Cannot allocate print record"));
			return(TRUE);
		}
		PrintDefault(hPrint);
		if ((err = PrError()) != noErr)
		{
			ttyputerr(_("Printer error %ld"), err);
			return(TRUE);
		}
		if (!PrStlDialog(hPrint)) return(TRUE);
	} else
	{
		/* have already printed: validate print record */
		PrValidate(hPrint);
		if ((err = PrError()) != noErr)
		{
			ttyputerr(_("Printer error %ld"), err);
			return(TRUE);
		}
	}

	/* do the "Print..." dialog */
	if (!PrJobDialog(hPrint)) return(TRUE);

	/* open the pseudo-window to the printer */
	plotwin = PrOpenDoc(hPrint, 0, 0);
	PrOpenPage(plotwin, 0);
	if (PrError())
	{
		ttyputerr(_("Error opening print page"));
		return(TRUE);
	}
	SetPort((WindowPtr)plotwin);

	/* save background color and make it white */
	varred = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_red_key);
	vargreen = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_green_key);
	varblue = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_blue_key);
	if (varred == NOVARIABLE || vargreen == NOVARIABLE || varblue == NOVARIABLE) return(TRUE);
	saver = ((INTBIG *)varred->addr)[0];
	saveg = ((INTBIG *)vargreen->addr)[0];
	saveb = ((INTBIG *)varblue->addr)[0];
	white = 0xFF;
	colormapload(&white, &white, &white, 0, 0);

	/* see if the printer is color */
	if ((plotwin->gPort.portBits.rowBytes & 0xC000) == 0xC000)
	{
		/* special case for color printing */
		hlx = slx;   hhx = shx;
		hly = sly;   hhy = shy;
		(void)us_makescreen(&hlx, &hly, &hhx, &hhy, el_curwindowpart);
		sr.left = hlx;   sr.right = hhx;

		getwindowframesize(el_curwindowpart->frame, &swid, &shei);
		revy = shei - 1;

		sr.top = revy - hhy;    sr.bottom = revy - hly;
		sr.bottom++;
		dr = plotwin->gPort.portRect;

		/* ensure that the destination has the same aspect ratio as the source */
		sxs = hhx - hlx;   sys = hhy - hly;
		dxs = dr.right - dr.left;   dys = dr.bottom - dr.top;
		if (dxs*sys > dys*sxs)
		{
			i = dys * sxs / sys;
			dr.left += (dxs-i) / 2;
			dr.right = dr.left + i;
		} else
		{
			i = dxs * sys / sxs;
			dr.top += (dys-i) / 2;
			dr.bottom = dr.top + i;
		}

		/* now copy the offscreen bits at "sr" to the print port at "dr" */
		window = gra_getoffscreen(el_curwindowpart);
		(void)LockPixels(window->portPixMap);
		CopyBits((BitMap *)*(window->portPixMap), &plotwin->gPort.portBits,
			&sr, &dr, srcCopy, 0L);
		UnlockPixels(window->portPixMap);

		PrClosePage(plotwin);
		PrCloseDoc(plotwin);

		/* print if spooled */
		if ((*hPrint)->prJob.bJDocLoop == bSpoolLoop)
		{
			PrPicFile(hPrint, 0, 0, 0, &prstat);
			if (PrError())
				ttyputerr(_("Error queuing print page"));
		}
		PrClose();
		
		/* restore background color */
		colormapload(&saver, &saveg, &saveb, 0, 0);
		return(FALSE);
	}

	TextFont(EFONT);

	/* establish order of plotting layers */
	io_maxlayer = io_setuptechorder(el_curtech);

	/* determine scaling */
	ulx = plotwin->gPort.portRect.left;
	uly = plotwin->gPort.portRect.top;
	uhx = plotwin->gPort.portRect.right;
	uhy = plotwin->gPort.portRect.bottom;
	revy = plotwin->gPort.portRect.bottom - plotwin->gPort.portRect.top;

	prod1 = shx - slx;
	prod2 = shy - sly;
	if (prod1 > THRESH || prod2 > THRESH)
	{
		/* screen extents too large: scale them down please */
		prod1 /= 1000;   prod2 /= 1000;
	}
	prod1 *= uhy - uly;
	prod2 *= uhx - ulx;
	if (prod1 != prod2)
	{
		/* adjust the scale */
		if (prod1 > prod2)
		{
			/* screen extent is too wide for window */
			i = (shx - slx) / 20;
			if (i == 0) i++;
			shx += i;   slx -= i;
			i = muldiv(shx - slx, uhy - uly, uhx - ulx) - (shy - sly);
			sly -= i/2;
			shy += i/2;
		} else
		{
			/* screen extent is too tall for window */
			i = (shy - sly) / 20;
			if (i == 0) i++;
			shy += i;   sly -= i;
			i = muldiv(shy - sly, uhx - ulx, uhy - uly) - (shx - slx);
			slx -= i/2;
			shx += i/2;
		}
	}
	io_setbounds(slx, shx, sly, shy, ulx, uhx, uly, uhy, revy, (WindowPtr)plotwin);

	/* draw the grid if requested */
	if ((el_curwindowpart->state&(GRIDON|GRIDTOOSMALL)) == GRIDON)
	{
		lambda = lib->lambda[np->tech->techindex];
		gridx = muldiv(el_curwindowpart->gridx, lambda, WHOLE);
		gridy = muldiv(el_curwindowpart->gridy, lambda, WHOLE);
		var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_gridfloatskey);
		if (var == NOVARIABLE || var->addr == 0)
		{
			gridlx = np->lowx  / gridx * gridx;
			gridly = np->lowy  / gridy * gridy;
		} else
		{
			grabpoint(np, &gridlx, &gridly);
			gridalign(&gridlx, &gridly, 1, np);
			gridlx += gridlx / gridx * gridx;
			gridly += gridly / gridy * gridy;
		}

		/* adjust to ensure that the first point is inside the range */
		while (gridlx < slx) gridlx += gridx;
		while (gridly < sly) gridly += gridy;

		/* plot the loop to the printer */
		for(cx = gridlx; cx <= shx; cx += gridx)
			for(cy = gridly; cy <= shy; cy += gridy)
				io_plotdot(cx, cy);
	}

	/* plot layers in this technology in proper order */
	for(ix=0; ix<io_maxlayer; ix++)
	{
		/* first plot outlines around stipple layers */
		io_setplotlayer(-(io_nextplotlayer(ix) + 1));
		(void)asktool(us_tool, x_("display-to-routine"), io_plotpoly);

		/* next plot layers */
		io_setplotlayer(io_nextplotlayer(ix) + 1);
		(void)asktool(us_tool, x_("display-to-routine"), io_plotpoly);
	}

	/* finally plot layers not in the technology */
	io_setplotlayer(0);
	(void)asktool(us_tool, x_("display-to-routine"), io_plotpoly);

	/* put out dates if requested */
	if ((curstate[0]&PLOTDATES) != 0)
	{
		/* create the polygon if it doesn't exist */
		(void)needstaticpolygon(&poly, 1, io_tool->cluster);

		/* plot cell name */
		infstr = initinfstr();
		formatinfstr(infstr, _("Cell: %s"), describenodeproto(np));
		poly->string = returninfstr(infstr);
		poly->xv[0] = np->highx;
		poly->yv[0] = np->lowy;
		poly->count = 1;
		poly->style = TEXTBOTRIGHT;
		TDCLEAR(poly->textdescript);
		TDSETSIZE(poly->textdescript, TXTSETQLAMBDA(4));
		poly->tech = el_curtech;
		poly->desc = &io_black;
		(void)io_plotpoly(poly, el_curwindowpart);

		/* plot creation date */
		infstr = initinfstr();
		formatinfstr(infstr, _("Created: %s"), timetostring((time_t)np->creationdate));
		poly->string = returninfstr(infstr);
		poly->yv[0] = np->lowy + (np->highy-np->lowy) / 20;
		(void)io_plotpoly(poly, el_curwindowpart);

		/* plot revision date */
		infstr = initinfstr();
		formatinfstr(infstr, _("Revised: %s"), timetostring((time_t)np->revisiondate));
		poly->string = returninfstr(infstr);
		poly->yv[0] = np->lowy + (np->highy-np->lowy) / 10;
		(void)io_plotpoly(poly, el_curwindowpart);
	}

	/* clean up */
	PrClosePage(plotwin);
	PrCloseDoc(plotwin);

	/* print if spooled */
	if ((*hPrint)->prJob.bJDocLoop == bSpoolLoop)
	{
		PrPicFile(hPrint, 0, 0, 0, &prstat);
	}
	PrClose();
		
	/* restore background color */
	colormapload(&saver, &saveg, &saveb, 0, 0);
	return(FALSE);
}

/******************** LAYER ORDERING FOR GRAPHIC COPY/PRINT ********************/

/*
 * Routine to set the current plotting layer to "i".
 */
void io_setplotlayer(INTBIG i)
{
	io_thislayer = i;
}

/******************** POLYGON DISPLAY FOR GRAPHIC COPY/PRINT ********************/

void io_setbounds(INTBIG slx, INTBIG shx, INTBIG sly, INTBIG shy, INTBIG ulx, INTBIG uhx,
	INTBIG uly, INTBIG uhy, INTBIG revy, WindowPtr window)
{
	static WINDOWPART plotwindow;

	io_plotwindow = &plotwindow;
	io_plotrevy = revy;
	io_plotwin = window;
	plotwindow.screenlx = io_slx = slx;
	plotwindow.screenhx = io_shx = shx;
	plotwindow.screenly = io_sly = sly;
	plotwindow.screenhy = io_shy = shy;
	plotwindow.uselx = io_ulx = ulx;
	plotwindow.usehx = io_uhx = uhx;
	plotwindow.usely = io_uly = uly;
	plotwindow.usehy = io_uhy = uhy;
	plotwindow.state = DISPWINDOW;
	computewindowscale(&plotwindow);
}

/*
 * routine to plot the polygon "poly"
 */
void io_plotpoly(POLYGON *poly, WINDOWPART *win)
{
	REGISTER INTBIG k, type;
	INTBIG xl, xh, yl, yh, x, y, listx[4], listy[4], size,
		centerx, centery, x1p, y1p, x2p, y2p, xa, ya, xb, yb, radius;
	INTBIG startangle, endangle, amt, style;
	UCHAR1 pat[8];
	Rect r;
	static POLYGON *polyspline = NOPOLYGON;
	REGISTER TECHNOLOGY *tech;

	/* ignore null layers */
	if (poly->desc->bits == LAYERN || poly->desc->col == ALLOFF) return;

	/* only plot the desired layer */
	if (io_thislayer < 0)
	{
		/* first outline filled areas in proper order */
		if (poly->layer != -io_thislayer-1) return;
		if (poly->style != FILLED && poly->style != FILLEDRECT) return;
		if (isbox(poly, &xl, &xh, &yl, &yh))
		{
			if (xl == xh || yl == yh) return;
			io_xform(&xl, &yl);
			io_xform(&xh, &yh);
			xl--;   yh++;
			SetPort(io_plotwin);
			PenMode(patCopy);
			PenPat(&qd.black);
			MoveTo(xl, io_plotrevy-yl);
			LineTo(xl, io_plotrevy-yh);
			LineTo(xh, io_plotrevy-yh);
			LineTo(xh, io_plotrevy-yl);
			LineTo(xl, io_plotrevy-yl);
		} else
		{
			/* outlining stipple: expand polygon */
			if (poly->count <= 2) return;
			centerx = centery = 0;
			for(k=0; k<poly->count; k++)
			{
				centerx += poly->xv[k];   centery += poly->yv[k];
			}
			centerx /= poly->count;   centery /= poly->count;
			io_xform(&centerx, &centery);
			for(k=0; k<poly->count; k++)
			{
				if (k == 0)
				{
					x1p = poly->xv[poly->count-1];   y1p = poly->yv[poly->count-1];
				} else
				{
					x1p = poly->xv[k-1];   y1p = poly->yv[k-1];
				}
				x2p = poly->xv[k];     y2p = poly->yv[k];
				io_xform(&x1p, &y1p);
				io_xform(&x2p, &y2p);
				if (x1p < centerx) x1p--;
				if (y1p > centery) y1p++;
				if (x2p < centerx) x2p--;
				if (y2p > centery) y2p++;
				SetPort(io_plotwin);
				PenMode(patCopy);
				PenPat(&qd.black);
				MoveTo(x1p, io_plotrevy-y1p);
				LineTo(x2p, io_plotrevy-y2p);
			}
		}
		return;
	}

	if (io_thislayer > 0)
	{
		/* then draw layers in proper order */
		if (poly->layer != io_thislayer-1) return;
	} else
	{
		/* final pass: draw layers not in the technology */
		for(k=0; k<io_maxlayer; k++) if (io_nextplotlayer(k) == poly->layer) return;
	}

	/* ignore grids */
	if (poly->style == GRIDDOTS) return;
	style = poly->desc->bwstyle;

	switch (poly->style)
	{
		case FILLED:
		case FILLEDRECT:
			if (isbox(poly, &xl, &xh, &yl, &yh))
			{
				if (xl == xh)
				{
					if (yl == yh) io_plotdot(xl, yl); else
						io_plotline(xl, yl, xl, yh, 0);
					break;
				} else if (yl == yh)
				{
					io_plotline(xl, yl, xh, yl, 0);
					break;
				}
				listx[0] = xl;   listy[0] = yl;
				listx[1] = xl;   listy[1] = yh;
				listx[2] = xh;   listy[2] = yh;
				listx[3] = xh;   listy[3] = yl;
				io_plotpolygon(listx, listy, 4, style, poly->desc);
			} else
			{
				if (poly->count == 1)
				{
					io_plotdot(poly->xv[0], poly->yv[0]);
					break;
				}
				if (poly->count == 2)
				{
					io_plotline(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1], 0);
					break;
				}
				io_plotpolygon(poly->xv, poly->yv, poly->count, style, poly->desc);
			}
			break;

		case CLOSED:
		case CLOSEDRECT:
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
			if (isbox(poly, &xl, &xh, &yl, &yh))
			{
				io_plotline(xl, yl, xl, yh, type);
				io_plotline(xl, yh, xh, yh, type);
				io_plotline(xh, yh, xh, yl, type);
				if (poly->style == CLOSED || poly->style == CLOSEDRECT)
					io_plotline(xh, yl, xl, yl, type);
				break;
			}
			for (k = 1; k < poly->count; k++)
				io_plotline(poly->xv[k-1], poly->yv[k-1], poly->xv[k], poly->yv[k], type);
			if (poly->style == CLOSED || poly->style == CLOSEDRECT)
			{
				k = poly->count - 1;
				io_plotline(poly->xv[k], poly->yv[k], poly->xv[0], poly->yv[0], type);
			}
			break;

		case VECTORS:
			for(k=0; k<poly->count; k += 2)
				io_plotline(poly->xv[k], poly->yv[k], poly->xv[k+1], poly->yv[k+1], 0);
			break;

		case CROSS:
		case BIGCROSS:
			getcenter(poly, &x, &y);
			io_plotline(x-5, y, x+5, y, 0);
			io_plotline(x, y+5, x, y-5, 0);
			break;

		case CROSSED:
			getbbox(poly, &xl, &xh, &yl, &yh);
			io_plotline(xl, yl, xl, yh, 0);
			io_plotline(xl, yh, xh, yh, 0);
			io_plotline(xh, yh, xh, yl, 0);
			io_plotline(xh, yl, xl, yl, 0);
			io_plotline(xh, yh, xl, yl, 0);
			io_plotline(xh, yl, xl, yh, 0);
			break;

		case DISC:
			/* filled disc: plot it and its outline (B&W) */
			centerx = poly->xv[0];   centery = poly->yv[0];
			xa = poly->xv[1];        ya = poly->yv[1];
			io_xform(&centerx, &centery);
			io_xform(&xa, &ya);
			for(k=0; k<8; k++)
				pat[k] = (style != PATTERNED ? 0xFF : poly->desc->raster[k]);
			radius = computedistance(centerx, centery, xa, ya);
			r.left = centerx - radius;
			r.right = centerx + radius + 1;
			r.top = io_plotrevy - centery - radius;
			r.bottom = io_plotrevy - centery + radius + 1;
			SetPort(io_plotwin);
			PenMode(patCopy);
			PenPat((PatPtr)pat);
			PaintOval(&r);

		case CIRCLE: case THICKCIRCLE:
			centerx = poly->xv[0];   centery = poly->yv[0];
			xa = poly->xv[1];        ya = poly->yv[1];
			io_xform(&centerx, &centery);
			io_xform(&xa, &ya);
			radius = computedistance(centerx, centery, xa, ya);
			r.left = centerx - radius;
			r.right = centerx + radius + 1;
			r.top = io_plotrevy - centery - radius;
			r.bottom = io_plotrevy - centery + radius + 1;
			SetPort(io_plotwin);
			PenMode(patCopy);
			PenPat(&qd.black);
			FrameOval(&r);
			break;

		case CIRCLEARC: case THICKCIRCLEARC:
			centerx = poly->xv[0];   centery = poly->yv[0];
			xa = poly->xv[1];        ya = poly->yv[1];
			xb = poly->xv[2];        yb = poly->yv[2];
			io_xform(&centerx, &centery);
			io_xform(&xa, &ya);
			io_xform(&xb, &yb);

			radius = computedistance(centerx, centery, xa, ya);
			startangle = figureangle(centerx, centery, xa, ya);
			endangle = figureangle(centerx, centery, xb, yb);
			if (startangle > endangle) amt = (startangle - endangle + 5) / 10; else
				amt = (startangle - endangle + 3600 + 5) / 10;
			startangle = ((4500-startangle) % 3600 + 5) / 10;
			r.left = centerx - radius;
			r.right = centerx + radius + 1;
			centery = io_plotrevy - centery;
			r.top = centery - radius;
			r.bottom = centery + radius + 1;
			ya = io_plotrevy - ya;   yb = io_plotrevy - yb;
			if (centerx == xa)
			{
				if (abs(r.top-ya) < abs(r.bottom-ya)) r.top = ya; else r.bottom = ya+1;
			}
			if (centerx == xb)
			{
				if (abs(r.top-yb) < abs(r.bottom-yb)) r.top = yb; else r.bottom = yb+1;
			}
			if (centery == ya)
			{
				if (abs(r.left-xa) < abs(r.right-xa)) r.left = xa; else r.right = xa+1;
			}
			if (centery == yb)
			{
				if (abs(r.left-xb) < abs(r.right-xb)) r.left = xb; else r.right = xb+1;
			}
			SetPort(io_plotwin);
			PenMode(patCopy);
			PenPat(&qd.black);
			FrameArc(&r, startangle, amt);
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
			tech = win->curnodeproto->tech;
			size = truefontsize(TDGETSIZE(poly->textdescript), io_plotwindow, tech);
			getbbox(poly, &xl, &xh, &yl, &yh);
			io_plottext(poly->style, xl, xh, yl, yh, size, poly->string);
			break;
	}
}

void io_plotdot(INTBIG x, INTBIG y)
{
	Rect r;

	io_xform(&x, &y);

	r.left = x;          r.right = x+1;
	r.top = io_plotrevy-y;   r.bottom = io_plotrevy-y+1;
	SetPort(io_plotwin);
	FillRect(&r, &qd.black);
}

/* draw a line */
void io_plotline(INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, INTBIG pattern)
{
	static UCHAR1 pat0[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	static UCHAR1 pat1h[] = {0xFF, 0, 0, 0, 0xFF, 0, 0, 0};
	static UCHAR1 pat1v[] = {0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88};
	static UCHAR1 pat2h[] = {0xFF, 0xFF, 0xFF, 0, 0, 0xFF, 0xFF, 0xFF};
	static UCHAR1 pat2v[] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
	static UCHAR1 *patlisth[] = {pat0, pat1h, pat2h, pat0};
	static UCHAR1 *patlistv[] = {pat0, pat1v, pat2v, pat0};

	io_xform(&x1, &y1);
	io_xform(&x2, &y2);

	/* do it on the printer */
	SetPort(io_plotwin);
	PenMode(patCopy);
	if (abs(x1-x2) > abs(y1-y2)) PenPat((PatPtr)patlistv[pattern]); else
		PenPat((PatPtr)patlisth[pattern]);
	if (pattern == 3) PenSize(3, 3);
	MoveTo(x1, io_plotrevy-y1);
	LineTo(x2, io_plotrevy-y2);
	if (pattern == 3) PenSize(1, 1);
}

void io_plotpolygon(INTBIG *x, INTBIG *y, INTBIG count, INTBIG style, GRAPHICS *desc)
{
	INTBIG i;
	UCHAR1 pat[8];
	PolyHandle polyhandle;
	static INTBIG *myx, *myy;
	static INTBIG len = 0;

	if (count == 0) return;
	if (count > len)
	{
		if (len != 0)
		{
			DisposePtr((Ptr)myx);   DisposePtr((Ptr)myy);
		}
		len = 0;
		myx = (INTBIG *)NewPtr(count * SIZEOFINTBIG);
		myy = (INTBIG *)NewPtr(count * SIZEOFINTBIG);
		if (myx == 0 || myy == 0) return;
		len = count;
	}
	for(i=0; i<count; i++)
	{
		myx[i] = x[i];
		myy[i] = y[i];
		io_xform(&myx[i], &myy[i]);
	}

	for(i=0; i<8; i++)
		pat[i] = (style != PATTERNED ? 0xFF : desc->raster[i]);
	SetPort(io_plotwin);
	polyhandle = OpenPoly();
	MoveTo(myx[count-1], io_plotrevy-myy[count-1]);
	for(i=0; i<count; i++) LineTo(myx[i], io_plotrevy-myy[i]);
	ClosePoly();
	PenMode(patOr);
	PenPat((PatPtr)pat);
	PaintPoly(polyhandle);
	KillPoly(polyhandle);
}

void io_plottext(INTBIG type, INTBIG lx, INTBIG ux, INTBIG ly, INTBIG uy, INTBIG size,
	CHAR *text)
{
	CHAR localstring[256];
	INTBIG len, i, tsx, tsy, px, py, truesize;
	FontInfo fontinfo;

	io_xform(&lx, &ly);
	io_xform(&ux, &uy);

	len = estrlen(text);
	for(i=0; i<len; i++) localstring[i] = text[i];
	TextFont(HFONT);
	if (type == TEXTBOX)
	{
		/* scan for a font that fits */
		for(truesize = size; truesize >= 4; truesize--)
		{
			TextSize(truesize);
			GetFontInfo(&fontinfo);
			tsx = TextWidth(localstring, 0, len);
			tsy = fontinfo.ascent + fontinfo.descent + fontinfo.leading;
			if (tsx <= ux-lx && tsy <= uy-ly) break;
		}

		/* continue only if text fits in Y */
		if (tsy <= uy-ly)
		{
			/* truncate in X if necessary */
			while (len > 1 && tsx > ux-lx)
			{
				len--;
				tsx = TextWidth(localstring, 0, len);
			}

			/* continue only if text fits */
			if (tsx <= ux-lx)
			{
				/* draw the text */
				MoveTo(lx+(ux-lx-tsx)/2, io_plotrevy-(ly+(uy-ly-tsy)/2)-fontinfo.descent);
				TextMode(srcOr+64);
				DrawText(localstring, 0, len);
			}
		}
	} else
	{
		TextSize(size);
		GetFontInfo(&fontinfo);
		tsx = TextWidth(localstring, 0, len);
		tsy = fontinfo.ascent + fontinfo.descent + fontinfo.leading;
		switch (type)
		{
			case TEXTCENT:
				px = maxi(io_ulx,(lx+ux-tsx)/2);
				py = maxi(io_uly,(ly+uy-tsy)/2);
				break;
			case TEXTTOP:
				px = maxi(io_ulx,(lx+ux-tsx)/2);
				py = maxi(io_uly,uy-tsy);
				break;
			case TEXTBOT:
				px = maxi(io_ulx,(lx+ux-tsx)/2);
				py = ly;
				break;
			case TEXTLEFT:
				px = lx;
				py = maxi(io_uly,(ly+uy-tsy)/2);
				break;
			case TEXTRIGHT:
				px = maxi(io_ulx,ux-tsx);
				py = maxi(io_uly,(ly+uy-tsy)/2);
				break;
			case TEXTTOPLEFT:
				px = lx;
				py = maxi(io_uly,uy-tsy);
				break;
			case TEXTBOTLEFT:
				px = lx;
				py = ly;
				break;
			case TEXTTOPRIGHT:
				px = maxi(io_ulx,ux-tsx);
				py = maxi(io_uly,uy-tsy);
				break;
			case TEXTBOTRIGHT:
				px = maxi(io_ulx,ux-tsx);
				py = ly;
				break;
		}
		MoveTo(px, io_plotrevy-py-fontinfo.descent);
		TextMode(srcOr+64);
		DrawText(localstring, 0, len);
	}
}

/*
 * Routine to convert the coordinates (x,y) for display.  The coordinates for
 * printing are placed back into (x,y) and the PostScript coordinates are placed
 * in (psx,psy).
 */
void io_xform(INTBIG *x, INTBIG *y)
{
	*x = muldiv(*x-io_slx, io_uhx-io_ulx, io_shx-io_slx) + io_ulx;
	*y = muldiv(*y-io_sly, io_uhy-io_uly, io_shy-io_sly) + io_uly;
}

/******************** GRAPHIC COPY ********************/

/*
 * routine to copy the highlighted graphics to the clipboard.  Called from
 * "usrcomtv.c:us_text()"
 */
void io_maccopyhighlighted(void)
{
	PicHandle picture;
	INTBIG len, slx, shx, sly, shy, ulx, uhx, uly, uhy, revy;
	INTBIG ix;
	CGrafPtr realwindow;
	extern CGrafPtr gra_getwindow(WINDOWPART *win);

	/* initialize for copying */
	ZeroScrap();
	realwindow = gra_getwindow(el_curwindowpart);
	SetPort((WindowPtr)realwindow);
	ClipRect(&qd.thePort->portRect);
	picture = OpenPicture(&realwindow->portRect);

	/* establish order of plotting layers */
	io_maxlayer = io_setuptechorder(el_curtech);

	/* setup window for copy */
	(void)us_getareabounds(&slx, &shx, &sly, &shy);
	ulx = slx;   uhx = shx;
	uly = sly;   uhy = shy;
	(void)us_makescreen(&ulx, &uly, &uhx, &uhy, el_curwindowpart);

#if 0		/* this will make a color copy in the clipboard */
	{
		Rect sr;
		CGrafPtr window;
		INTBIG revy, swid, shei;

		window = gra_getoffscreen(el_curwindowpart);
		getwindowframesize(el_curwindowpart->frame, &swid, &shei);
		revy = shei - 1;

		sr.left = ulx;   sr.right = uhx;
		sr.top = revy - uhy;    sr.bottom = revy - uly;

		/* now copy the offscreen bits at "sr" to the print port at "dr" */
		(void)LockPixels(window->portPixMap);
		CopyBits((BitMap *)*(window->portPixMap), (BitMap *)*(realwindow->portPixMap),
			&sr, &sr, srcCopy, 0L);
		UnlockPixels(window->portPixMap);

		ClosePicture();
		HLock((Handle)picture);
		len = (*picture)->picSize;
		PutScrap(len, 'PICT', (Ptr)(*picture));
		HUnlock((Handle)picture);
		KillPicture(picture);
		return(0);
	}
#endif

	uhx -= ulx;   uhy -= uly;   ulx = uly = 0;
	revy = uhy;
	io_setbounds(slx, shx, sly, shy, ulx, uhx, uly, uhy, revy, (WindowPtr)realwindow);
	io_plotwindow = el_curwindowpart;

	/* plot layers in this technology in proper order */
	for(ix=0; ix<io_maxlayer; ix++)
	{
		/* first plot outlines around stipple layers */
		io_setplotlayer(-(io_nextplotlayer(ix) + 1));
		(void)asktool(us_tool, x_("display-highlighted-to-routine"), io_plotpoly);

		/* next plot layers */
		io_setplotlayer(io_nextplotlayer(ix) + 1);
		(void)asktool(us_tool, x_("display-highlighted-to-routine"), io_plotpoly);
	}

	/* finally plot layers not in the technology */
	io_setplotlayer(0);
	(void)asktool(us_tool, x_("display-highlighted-to-routine"), io_plotpoly);

	ClosePicture();
	HLock((Handle)picture);
	len = (*picture)->picSize;
	PutScrap(len, 'PICT', (Ptr)(*picture));
	HUnlock((Handle)picture);
	KillPicture(picture);
}

#endif  /* MACOS - at top */
