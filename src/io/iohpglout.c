/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iohpglout.c
 * Input/output analysis tool: HPGL generation
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
#include "global.h"
#include "egraphics.h"
#include "eio.h"
#include <math.h>

#define MAXSTR	      132		/* maximum string length */

static FILE      *io_hpout;
static WINDOWPART io_hpwindow;
static INTBIG     io_hpcurrent_line_type;
static INTBIG     io_hpcurrent_pen_color;
static INTBIG     io_hpmerging;
static INTBIG     io_hpgl2;

typedef struct
{
	INTBIG pnum;		/* pen number */
	INTBIG ltype;		/* line type (0=solid) */
	INTBIG ftype;		/* fill type (0=none, 1=solid, */
						/*      3=parallel lines 4=crosshatched lines) */
	INTBIG fdist;		/* fill distance (ftype 3 or 4 only) */
	INTBIG fangle;		/* fill angle (ftype 3 or 4 only) */
} PENCOLOR;

static PENCOLOR pen_color_table[256];

/* prototypes for local routines */
static void io_hpwritepolygon(INTBIG, TECHNOLOGY*, INTBIG*, INTBIG*, INTBIG);
static void io_hppoly(POLYGON*, WINDOWPART*);
static void io_hpline(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static void io_hparc(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static void io_hpcircle(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static void io_hpdisc(INTBIG, INTBIG, INTBIG, INTBIG, GRAPHICS*);
static void io_hpfilledpolygon(INTBIG*, INTBIG*, INTBIG, INTBIG);
static void io_hptext(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, CHAR*, INTBIG);
static void io_hpparsepenfile(void);
static INTBIG io_hpchoosecolorentry(INTBIG);
static void io_hppenselect(INTBIG);
static void io_hpmove(INTBIG, INTBIG);
static void io_hpdraw(INTBIG, INTBIG);
static void io_hpcmd(CHAR[]);

void io_hpwritepolygon(INTBIG layer, TECHNOLOGY *tech, INTBIG *x, INTBIG *y, INTBIG count)
{
	io_hpfilledpolygon(x, y, count, layer);
}

/*
 * routine to write out a HPGL file
 */
BOOLEAN io_writehpgllibrary(LIBRARY *lib)
{
	CHAR file[100], buff[MAXSTR], *truename;
	REGISTER NODEPROTO *np;
	INTBIG lx, hx, ly, hy, i, *curstate,
		sulx, suhx, suly, suhy, sslx, sshx, ssly, sshy;
	REGISTER VARIABLE *varred, *vargreen, *varblue, *varscale;
	REGISTER float scale;

	np = lib->curnodeproto;
	if (np == NONODEPROTO)
	{
		ttyputerr(_("No current cell to plot"));
		return(TRUE);
	}

	/* cannot write text-only cells */
	if ((np->cellview->viewstate&TEXTVIEW) != 0)
	{
		ttyputerr(_("Cannot write textual cells"));
		return(TRUE);
	}

	/* determine whether this is HPGL or HPGL/2 */
	curstate = io_getstatebits();
	if ((curstate[0]&HPGL2) == 0) io_hpgl2 = 0; else
		io_hpgl2 = 1;

	/* create the output file */
	if (io_hpgl2 != 0)
	{
		(void)esnprintf(file, 100, x_("%s.hpgl2"), np->protoname);
		io_hpout = xcreate(file, io_filetypehpgl2, _("HPGL/2 File"), &truename);
	} else
	{
		(void)esnprintf(file, 100, x_("%s.hpgl"), np->protoname);
		io_hpout = xcreate(file, io_filetypehpgl, _("HPGL File"), &truename);
	}
	if (io_hpout == NULL)
	{
		if (truename != 0) ttyputerr(_("Cannot create %s"), truename);
		return(TRUE);
	}

	/* determine area to plot */
	if (io_getareatoprint(np, &lx, &hx, &ly, &hy, TRUE)) return(TRUE);

	/* build pseudowindow */
	io_hpwindow.uselx = 0;       io_hpwindow.screenlx = lx;
	io_hpwindow.usely = 0;       io_hpwindow.screenly = ly;
	io_hpwindow.usehx = 10000;   io_hpwindow.screenhx = hx;
	io_hpwindow.usehy = 10000;   io_hpwindow.screenhy = hy;
	io_hpwindow.state = DISPWINDOW;
	computewindowscale(&io_hpwindow);

	/* read pen information */
	io_hpparsepenfile();

	if (io_hpgl2 != 0)
	{
		/* HPGL/2 setup and defaults */
		io_hpcmd(x_("\033%0BBPIN"));
		io_hpcmd(x_("LA1,4,2,4QLMC0"));

		/* setup pens */
		io_hpcmd(x_("NP256"));
		varred = getval((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, x_("USER_colormap_red"));
		vargreen = getval((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, x_("USER_colormap_green"));
		varblue = getval((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, x_("USER_colormap_blue"));
		if (varred == NOVARIABLE || vargreen == NOVARIABLE || varblue == NOVARIABLE)
		{
			ttyputmsg(_("Cannot get colors!"));
			return(TRUE);
		}
		for(i=0; i<256; i++)
		{
			if ((i&(LAYERG|LAYERH|LAYEROE)) != LAYEROE) continue;
			(void)esnprintf(buff, MAXSTR, x_("PC%ld,%ld,%ld,%ld"), i&LAYERO, ((INTBIG *)varred->addr)[i],
				((INTBIG *)vargreen->addr)[i], ((INTBIG *)varblue->addr)[i]);
			io_hpcmd(buff);
		}
		(void)esnprintf(buff, MAXSTR, x_("PC%d,%ld,%ld,%ld"), LAYERT1, ((INTBIG *)varred->addr)[LAYERT1],
			((INTBIG *)vargreen->addr)[LAYERT1], ((INTBIG *)varblue->addr)[LAYERT1]);
		io_hpcmd(buff);
		(void)esnprintf(buff, MAXSTR, x_("PC%d,%ld,%ld,%ld"), LAYERT2, ((INTBIG *)varred->addr)[LAYERT2],
			((INTBIG *)vargreen->addr)[LAYERT2], ((INTBIG *)varblue->addr)[LAYERT2]);
		io_hpcmd(buff);
		(void)esnprintf(buff, MAXSTR, x_("PC%d,%ld,%ld,%ld"), LAYERT3, ((INTBIG *)varred->addr)[LAYERT3],
			((INTBIG *)vargreen->addr)[LAYERT3], ((INTBIG *)varblue->addr)[LAYERT3]);
		io_hpcmd(buff);
		(void)esnprintf(buff, MAXSTR, x_("PC%d,%ld,%ld,%ld"), LAYERT4, ((INTBIG *)varred->addr)[LAYERT4],
			((INTBIG *)vargreen->addr)[LAYERT4], ((INTBIG *)varblue->addr)[LAYERT4]);
		io_hpcmd(buff);
		(void)esnprintf(buff, MAXSTR, x_("PC%d,%ld,%ld,%ld"), LAYERT5, ((INTBIG *)varred->addr)[LAYERT5],
			((INTBIG *)vargreen->addr)[LAYERT5], ((INTBIG *)varblue->addr)[LAYERT5]);
		io_hpcmd(buff);
	} else
	{
		/* HPGL setup and defaults */
		io_hpcmd(x_("\033.(\033.@;14:\033.I80;0;17:\033.N;19:"));
		io_hpcmd(x_("DF;IN;"));
	}

	/* set default location of "P1" and "P2" points on the plotter */
	io_hpcmd(x_("IP;"));

	/* see if fixed scale specification has been made */
	varscale = getval((INTBIG)io_tool, VTOOL, VINTEGER, x_("IO_hpgl2_scale"));
	if (io_hpgl2 != 0 && varscale != NOVARIABLE)
	{
		/* in HPGL/2, can handle fixed scale specification in internal units per pixel */
		scale = 1.0f / (float)varscale->addr;
		(void)esnprintf(buff, MAXSTR, x_("SC%ld,%g,%ld,%g,2;"), io_hpwindow.screenlx, scale, io_hpwindow.screenly, scale);
		io_hpcmd(buff);
	} else
	{
		/* no fixed scale or HPGL: fill the page while maintaining isotropic scaling */
		(void)esnprintf(buff, MAXSTR, x_("SC %ld,%ld,%ld,%ld,1;"), io_hpwindow.screenlx, io_hpwindow.screenhx,
			io_hpwindow.screenly, io_hpwindow.screenhy);
		io_hpcmd(buff);
	}

	io_hpcurrent_line_type = -1;
	io_hpcurrent_pen_color = -1;
	io_hpmerging = 0;

	/* plot everything */
	sulx = el_curwindowpart->uselx;      suhx = el_curwindowpart->usehx;
	suly = el_curwindowpart->usely;      suhy = el_curwindowpart->usehy;
	sslx = el_curwindowpart->screenlx;   sshx = el_curwindowpart->screenhx;
	ssly = el_curwindowpart->screenly;   sshy = el_curwindowpart->screenhy;
	el_curwindowpart->uselx = io_hpwindow.uselx;   el_curwindowpart->usehx = io_hpwindow.usehx;
	el_curwindowpart->usely = io_hpwindow.usely;   el_curwindowpart->usehy = io_hpwindow.usehy;
	el_curwindowpart->screenlx = io_hpwindow.screenlx;
	el_curwindowpart->screenhx = io_hpwindow.screenhx;
	el_curwindowpart->screenly = io_hpwindow.screenly;
	el_curwindowpart->screenhy = io_hpwindow.screenhy;
	computewindowscale(el_curwindowpart);
	(void)asktool(us_tool, x_("display-to-routine"), io_hppoly);
	el_curwindowpart->uselx = sulx;      el_curwindowpart->usehx = suhx;
	el_curwindowpart->usely = suly;      el_curwindowpart->usehy = suhy;
	el_curwindowpart->screenlx = sslx;   el_curwindowpart->screenhx = sshx;
	el_curwindowpart->screenly = ssly;   el_curwindowpart->screenhy = sshy;
	computewindowscale(el_curwindowpart);

	/* handle merged rectangles */
	if (io_hpmerging != 0)
	{
		mrgdonecell(io_hpwritepolygon);
		mrgterm();
	}

	if (io_hpgl2 != 0)
	{
		/* HPGL/2 termination */
		io_hpcmd(x_("PUSP0PG;"));
	} else
	{
		/* HPGL termination */
		io_hpcmd(x_(";SP0;PU;DF;NR;\033.)"));
	}
	xclose(io_hpout);
	ttyputmsg(_("%s written"), truename);
	return(FALSE);
}

/*
 * routine to plot the polygon "poly"
 */
void io_hppoly(POLYGON *poly, WINDOWPART *win)
{
	REGISTER INTBIG k;
	INTBIG xl, xh, yl, yh, x, y, size;
	INTBIG color;
	REGISTER TECHNOLOGY *tech;

	/* ignore null layers */
	if (poly->desc->bits == LAYERN || poly->desc->col == ALLOFF) return;

	/* ignore grids */
	if (poly->style == GRIDDOTS) return;

	color = poly->desc->col;
	switch (poly->style)
	{
		case FILLED:
		case FILLEDRECT:
			if (isbox(poly, &xl, &xh, &yl, &yh))
			{
				if (xl == xh)
				{
					if (yl != yh) io_hpline(xl, yl, xl, yh, color);
					break;
				} else if (yl == yh)
				{
					io_hpline(xl, yl, xh, yl, color);
					break;
				}
				if (io_hpmerging == 0) mrginit();
				io_hpmerging = 1;
				mrgstorebox(color, el_curtech, xh-xl, yh-yl, (xl+xh)/2, (yl+yh)/2);
			} else
			{
				if (poly->count <= 1) break;
				if (poly->count == 2)
				{
					io_hpline(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1], color);
					break;
				}
				io_hpfilledpolygon(poly->xv, poly->yv, poly->count, color);
			}
			break;

		case CLOSED:
		case CLOSEDRECT:
		case OPENED:
		case OPENEDT1:
		case OPENEDT2:
		case OPENEDT3:
			if (isbox(poly, &xl, &xh, &yl, &yh))
			{
				io_hpline(xl, yl, xl, yh, color);
				io_hpline(xl, yh, xh, yh, color);
				io_hpline(xh, yh, xh, yl, color);
				if (poly->style == CLOSED || poly->style == CLOSEDRECT)
					io_hpline(xh, yl, xl, yl, color);
				break;
			}
			for (k = 1; k < poly->count; k++)
				io_hpline(poly->xv[k-1], poly->yv[k-1], poly->xv[k], poly->yv[k], color);
			if (poly->style == CLOSED || poly->style == CLOSEDRECT)
			{
				k = poly->count - 1;
				io_hpline(poly->xv[k], poly->yv[k], poly->xv[0], poly->yv[0], color);
			}
			break;

		case VECTORS:
			for(k=0; k<poly->count; k += 2)
				io_hpline(poly->xv[k], poly->yv[k], poly->xv[k+1], poly->yv[k+1], color);
			break;

		case CROSS:
		case BIGCROSS:
			getcenter(poly, &x, &y);
			io_hpline(x-5, y, x+5, y, color);
			io_hpline(x, y+5, x, y-5, color);
			break;

		case CROSSED:
			getbbox(poly, &xl, &xh, &yl, &yh);
			io_hpline(xl, yl, xl, yh, color);
			io_hpline(xl, yh, xh, yh, color);
			io_hpline(xh, yh, xh, yl, color);
			io_hpline(xh, yl, xl, yl, color);
			io_hpline(xh, yh, xl, yl, color);
			io_hpline(xh, yl, xl, yh, color);
			break;

		case DISC:
			/* filled disc: plot it and its outline */
			io_hpdisc(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1], poly->desc);
			/* FALLTHROUGH */ 

		case CIRCLE: case THICKCIRCLE:
			io_hpcircle(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1], color);
			break;

		case CIRCLEARC: case THICKCIRCLEARC:
			io_hparc(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1],
				poly->xv[2], poly->yv[2], color);
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
			size = truefontsize(TDGETSIZE(poly->textdescript), win, tech);
			getbbox(poly, &xl, &xh, &yl, &yh);
			io_hptext(poly->style, xl, xh, yl, yh, size, poly->string, color);
			break;
	}
}

void io_hpline(INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, INTBIG color)
{
	io_hppenselect(color);	/* color */
	io_hpmove(x1, y1);		/* move to the start */
	io_hpdraw(x2, y2);		/* draw to the end   */
}

void io_hparc(INTBIG centerx, INTBIG centery, INTBIG x1, INTBIG y1, INTBIG x2,
	INTBIG y2, INTBIG color)
{
	CHAR buff[MAXSTR];
	INTBIG startangle, endangle, amt;

	startangle = figureangle(centerx, centery, x1, y1);
	endangle = figureangle(centerx, centery, x2, y2);
	if (startangle > endangle) amt = (startangle - endangle + 5) / 10; else
		amt = (startangle - endangle + 3600 + 5) / 10;
	io_hppenselect(color);
	io_hpmove(x1, y1);
	io_hpcmd(x_("PD;"));
	(void)esnprintf(buff, MAXSTR, x_("AA %ld %ld %ld;"), centerx, centery, -amt);
	io_hpcmd(buff);
	io_hpcmd(x_("PU;"));
}

void io_hpcircle(INTBIG atx, INTBIG aty, INTBIG ex, INTBIG ey, INTBIG color)
{
	CHAR buff[MAXSTR];
	INTBIG radius;

	radius = computedistance(atx, aty, ex, ey);
	io_hppenselect(color);
	io_hpmove(atx, aty);
	io_hpcmd(x_("PD;"));
	(void)esnprintf(buff, MAXSTR, x_("CI %ld;"), radius);
	io_hpcmd(buff);
	io_hpcmd(x_("PU;"));
}

void io_hpdisc(INTBIG atx, INTBIG aty, INTBIG ex, INTBIG ey, GRAPHICS *desc)
{
	CHAR buff[MAXSTR];
	INTBIG radius;
	INTBIG cindex, ftype, fangle, fdist, color;

	color = desc->col;
	cindex = io_hpchoosecolorentry(color);
	ftype = pen_color_table[cindex].ftype;
	fangle = pen_color_table[cindex].fangle;
	fdist = pen_color_table[cindex].fdist;

	radius = computedistance(atx, aty, ex, ey);
	io_hppenselect(color);
	(void)esnprintf(buff, MAXSTR, x_("FT %ld,%ld,%ld;"), ftype, fdist, fangle);
	io_hpcmd(buff);

	io_hpmove(atx, aty);
	io_hpcmd(x_("PD;"));
	io_hpcmd(x_("PM;"));
	(void)esnprintf(buff, MAXSTR, x_("CI %ld;"), radius);
	io_hpcmd(buff);
	io_hpcmd(x_("PM2;"));
	if (ftype != 0) io_hpcmd(x_("FP;"));
	if (ftype != 1) io_hpcmd(x_("EP;"));
	io_hpcmd(x_("PU;"));
}

void io_hpfilledpolygon(INTBIG *xlist, INTBIG *ylist, INTBIG count, INTBIG color)
{
	CHAR buff[MAXSTR];
	INTBIG cindex, ftype, fangle, fdist, i;
	INTBIG firstx, firsty;

	if (count <= 1) return;
	cindex = io_hpchoosecolorentry(color);
	ftype = pen_color_table[cindex].ftype;
	fangle = pen_color_table[cindex].fangle;
	fdist = pen_color_table[cindex].fdist;

	io_hppenselect(color);	/* color */
	(void)esnprintf(buff, MAXSTR, x_("FT %ld,%ld,%ld;"), ftype, fdist, fangle);
	io_hpcmd(buff);

	firstx = xlist[0];		/* save the end point */
	firsty = ylist[0];
	io_hpmove(firstx, firsty);	/* move to the start */
	io_hpcmd(x_("PM;"));
	for(i=1; i<count; i++) io_hpdraw(xlist[i], ylist[i]);
	io_hpdraw(firstx, firsty);	/* close the polygon */

	io_hpcmd(x_("PM2;"));
	if (ftype != 0) io_hpcmd(x_("FP;"));
	if (ftype != 1) io_hpcmd(x_("EP;"));
}

void io_hptext(INTBIG type, INTBIG xl, INTBIG xh, INTBIG yl, INTBIG yh, INTBIG size,
	CHAR *text, INTBIG color)
{
	CHAR s[MAXSTR];

	(void)esnprintf(s, MAXSTR, x_("SI %f,%f;"), size*0.02f/1.3f, size*0.02f);
	io_hpcmd(s);
	io_hppenselect(color);

	switch (type)
	{
		case TEXTBOTLEFT:
			io_hpmove(xl, yl);
			io_hpcmd(x_("LO1;"));
			break;
		case TEXTLEFT:
			io_hpmove(xl, (yl+yh)/2);
			io_hpcmd(x_("LO2;"));
			break;
		case TEXTTOPLEFT:
			io_hpmove(xh, yl);
			io_hpcmd(x_("LO3;"));
			break;
		case TEXTBOT:
			io_hpmove((xl+xh)/2, yl);
			io_hpcmd(x_("LO4;"));
			break;
		case TEXTCENT:
		case TEXTBOX:
			io_hpmove((xl+xh)/2, (yl+yh)/2);
			io_hpcmd(x_("LO5;"));
			break;
		case TEXTTOP:
			io_hpmove((xl+xh)/2, yh);
			io_hpcmd(x_("LO6;"));
			break;
		case TEXTBOTRIGHT:
			io_hpmove(xh, yl);
			io_hpcmd(x_("LO7;"));
			break;
		case TEXTRIGHT:
			io_hpmove(xh, (yl+yh)/2);
			io_hpcmd(x_("LO8;"));
			break;
		case TEXTTOPRIGHT:
			io_hpmove(xh, yh);
			io_hpcmd(x_("LO9;"));
			break;
	}

	(void)esnprintf(s, MAXSTR, x_("LB %s\003"), text);
	io_hpcmd(s);
}

/*
 * ELECTRIC uses a total of 256 colors.  Some are predefined, and others can be
 * defined in the technology files.
 * Opaque colors are pre-defined.  Transparent colors, and the colors which
 * result from overlaying them can be defined in the technology file.
 * Each row in this file contains the following:
 *	color -	an integer corresponding to the entry in the color table.  These are
 *		the same colors used by Electric, as defined in "egraphics.h"
 *		The comments provided here tell what colors Electric
 *		expects (what color they correspond to on the screen).
 *
 *	pen no. - The pen number to use when drawing this color.
 *		A zero in this field means that this color will not
 *		be drawn (pen is returned to carousel).
 *
 *	line type - The line texture to use when drawing this color:
 *		0 - solid
 *		1 - dotted
 *		2 through 6 - dashed
 *
 *	fill type - The method used when filling areas with this color:
 *		0 - no fill (outline only)
 *		1 - solid fill
 *		3 - parallel lines at the specified angle
 *		4 - cross-hatched lines at the specified angle
 *
 *	fill distance - The distance between lines used in fill types 3 and 4 above,
 *		measured in half-nanometers.
 *
 *	fill angle - The angle of the lines used in fill types 3 and 4 above.
 *
 *	comments - The parser reads the first five numbers on each line.
 *		Anything else will be treated as a comment.
 *
 *   Color	Pen#	Line	Fill	Dist	Angle
 */
static CHAR *io_hppendata[] =
{
	x_("0000	0	0	0	0	0	no color "),
	x_("0004	1	0	3	300	0	transparent color 1 (blue)"),
	x_("0010	2	0	3	300	30	transparent color 2 (red)"),
	x_("0020	3	0	3	300	60	transparent color 3 (green)"),
	x_("0040	4	0	3	300	90	transparent color 4 (purple)"),
	x_("0100	5	0	3	300	120	transparent color 5 (yellow)"),
	x_("0002	0	0	0	0	0	white"),
	x_("0006	6	0	1	0	0	black "),
	x_("0012	7	0	1	0	0	red "),
	x_("0016	8	0	1	0	0	blue"),
	x_("0022	9	0	1	0	0	green"),
	x_("0026	10	2	1	0	0	cyan"),
	x_("0032	11	0	1	0	0	magenta"),
	x_("0036	12	0	1	0	0	yellow"),
	x_("0042	13	0	0	0	0	cell and port names"),
	x_("0046	1	0	0	0	0	cell outline"),
	x_("0052	0	0	0	0	0	window border color - never used"),
	x_("0056	0	0	0	0	0	highlighted window border color - never used"),
	x_("0062	0	0	0	0	0	menu border color - never used"),
	x_("0066	0	0	0	0	0	highlighted menu border color - never used"),
	x_("0072	0	0	0	0	0	menu text color - never used"),
	x_("0076	0	0	0	0	0	menu glyph color - never used"),
	x_("0102	0	0	0	0	0	cursor color - never used"),
	x_("0106	15	3	1	0	0	gray"),
	x_("0112	16	0	1	0	0	orange"),
	x_("0116	17	4	1	0	0	purple"),
	x_("0122	18	0	1	0	0	brown"),
	x_("0126	19	2	1	0	0	light gray"),
	x_("0132	20	4	1	0	0	dark gray"),
	x_("0136	21	0	1	0	0	light red"),
	x_("0142	22	0	1	0	0	dark red"),
	x_("0146	23	0	1	0	0	light green"),
	x_("0152	24	0	1	0	0	dark green"),
	x_("0156	25	4	1	0	0	light blue"),
	x_("0162	26	0	1	0	0	dark blue"),
	(CHAR *)0
};

/*
 * routine to read color/pen/line-type information from pen file, and set
 * up a color map.
 */
void io_hpparsepenfile(void)
{
	CHAR *pt;
	INTBIG in_color, in_pnum, in_ltype, in_ftype, in_fdist, in_fangle, i;

	for(i=0; i<256; i++)
	{
		pen_color_table[i].pnum = 0;
		pen_color_table[i].ltype = -1;
		pen_color_table[i].ftype = 0;
		pen_color_table[i].fdist = 0;
		pen_color_table[i].fangle = 0;
	}

	for(i=0; io_hppendata[i] != 0; i++)
	{
		/* get the color */
		pt = io_hppendata[i];
		in_color = myatoi(pt);
		in_color = io_hpchoosecolorentry(in_color);
		if (in_color <= 0 || in_color >= 256) continue;

		/* skip to pen field */
		while (*pt != 0 && *pt != ' ' && *pt != '\t') pt++;
		while (*pt == ' ' || *pt == '\t') pt++;
		if (*pt == 0) continue;
		in_pnum = myatoi(pt);
in_pnum = in_color;

		/* skip to type field */
		while (*pt != 0 && *pt != ' ' && *pt != '\t') pt++;
		while (*pt == ' ' || *pt == '\t') pt++;
		if (*pt == 0) continue;
		in_ltype = myatoi(pt);

		/* skip to fill field */
		while (*pt != 0 && *pt != ' ' && *pt != '\t') pt++;
		while (*pt == ' ' || *pt == '\t') pt++;
		if (*pt == 0) continue;
		in_ftype = myatoi(pt);

		/* skip to distance field */
		while (*pt != 0 && *pt != ' ' && *pt != '\t') pt++;
		while (*pt == ' ' || *pt == '\t') pt++;
		if (*pt == 0) continue;
		in_fdist = myatoi(pt);

		/* skip to angle field */
		while (*pt != 0 && *pt != ' ' && *pt != '\t') pt++;
		while (*pt == ' ' || *pt == '\t') pt++;
		if (*pt == 0) continue;
		in_fangle = myatoi(pt);

		/* load the table */
		pen_color_table[in_color].pnum = in_pnum;
		pen_color_table[in_color].ltype = in_ltype;
		pen_color_table[in_color].ftype = in_ftype;
		pen_color_table[in_color].fdist = in_fdist;
		pen_color_table[in_color].fangle = in_fangle;
	}
}

/*
 * this function accepts a color from 0 to 255, and returns either an opaque
 * color (refer to egraphics.h) or a transparent color.  In other words, this
 * function masks out the bits used by the grid and highlight.  In cases where
 * a color is a combination of transparent colors, only one of the transparent
 * colors is returned.  This approximation is only significant in cases where two
 * identical transparent objects exactly overlap each other.  For our
 * applications, this will be rare.
 */
INTBIG io_hpchoosecolorentry(INTBIG c)
{
	/* if the desired color is opaque, mask out the unused bit planes */
	if (c & LAYEROE) return(c & LAYERO);

	/* the desired color is transparent */
	if (c & LAYERT1) return(LAYERT1);
	if (c & LAYERT2) return(LAYERT2);
	if (c & LAYERT3) return(LAYERT3);
	if (c & LAYERT4) return(LAYERT4);
	if (c & LAYERT5) return(LAYERT5);
	return(LAYERN);
}

/*
 * based upon the current value of "color", this function will select the
 * proper entry from the pen table, and select an appropriate pen from the
 * pen_color_table.  The appropriate pen and line type is then selected.
 */
void io_hppenselect(INTBIG color)
{
	CHAR pbuff[10];
	INTBIG desired_pen, line_type, cindex;

	cindex = io_hpchoosecolorentry(color);
	desired_pen = pen_color_table[cindex].pnum;
	line_type = pen_color_table[cindex].ltype;

	/* check to see if pen is defined */
	if (desired_pen < 1) desired_pen = 1;

	/* select new pen, pen 0 returns pen to carousel and does not get new pen */
	if (desired_pen != io_hpcurrent_pen_color)
	{
		(void)esnprintf(pbuff, 10, x_("SP%ld;"), desired_pen);
		io_hpcmd(pbuff);
		io_hpcurrent_pen_color = desired_pen;
	}

	/* set line type, line type 0 defaults to solid */
	if (line_type != io_hpcurrent_line_type)
	{
		if (line_type == 0) (void)esnprintf(pbuff, 10, x_("LT;")); else
			(void)esnprintf(pbuff, 10, x_("LT%ld;"), line_type);
		io_hpcurrent_line_type = line_type;
		io_hpcmd(pbuff);
	}
}

/*
 * routine to move the pen to a new position
 * This has been changed - no longer check to see if we are already there.
 * This change adds a little to the output file length, but reduces the
 * amount of code significantly.  The hp spends most of its time moving
 * the pen, not reading commands. Therefore, this should be no problem.
 */
void io_hpmove(INTBIG x, INTBIG y)
{
	CHAR buff[MAXSTR];

	(void)esnprintf(buff, MAXSTR, x_("PU%ld,%ld;"), x, y);
	io_hpcmd(buff);
}

/*
 * routine to draw from current point to the next, assume PA already issued
 * Changed this so that a PD (pen down) instruction is issued.  Then we
 * can use this in several other functions.
 */
void io_hpdraw(INTBIG x, INTBIG y)
{
	CHAR buff[MAXSTR];

	(void)esnprintf(buff, MAXSTR, x_("PD%ld,%ld "), x, y);
	io_hpcmd(buff);
}

/* routine to send a command to the plotter */
void io_hpcmd(CHAR string[])
{
	(void)xprintf(io_hpout, x_("%s\n"), string);
}
