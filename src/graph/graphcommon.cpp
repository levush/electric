/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design Systems
 *
 * File: graphcommon.cpp
 * Drawing graphics primitives into off-screen buffer
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
#include "edialogs.h"
#include "egraphics.h"
#include "usr.h"
#include <errno.h>

#define USE_MEMMOVE						/* use memmove & memcpy */

#if 0		/* for checking display coordinates */
#  define CHECKCOORD(wf, x, y, where)
#else
#  define CHECKCOORD(wf, x, y, where) if (gra_badcoord(wf, x, y, where)) return
#endif
static INTBIG  gra_badcoord(WINDOWFRAME *wf, INTBIG x, INTBIG y, CHAR *where);

/****** line drawing ******/
static void    gra_drawsolidline(WINDOWFRAME *wf, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, INTBIG col, INTBIG mask);
static void    gra_drawpatline(WINDOWFRAME *wf, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, INTBIG col,
                           INTBIG mask, INTBIG pattern);
static void    gra_drawthickline(WINDOWFRAME *wf, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, INTBIG col, INTBIG mask);
static void    gra_drawthickpoint(WINDOWFRAME *wf, INTBIG x, INTBIG y, INTBIG mask, INTBIG col);

/****** rectangle saving ******/
#define NOSAVEDBOX ((SAVEDBOX *)-1)
typedef struct Isavedbox
{
	UCHAR1      *pix;
	WINDOWFRAME *wf;
	INTBIG      lx, hx, ly, hy;
} SAVEDBOX;

/****** polygon decomposition ******/
#define NOPOLYSEG ((POLYSEG *)-1)

typedef struct Isegment
{
	INTBIG fx,fy, tx,ty, direction, increment;
	struct Isegment *nextedge;
	struct Isegment *nextactive;
} POLYSEG;

static POLYSEG    *gra_polysegs;
static INTBIG     gra_polysegcount = 0;

/****** curve drawing ******/
#define MODM(x) ((x<1) ? x+8 : x)
#define MODP(x) ((x>8) ? x-8 : x)
#define ODD(x)  (x&1)

static INTBIG        gra_arcocttable[] = {0,0,0,0,0,0,0,0,0};
static INTBIG        gra_arccenterx, gra_arccentery;
static INTBIG        gra_arcradius, gra_curveminx, gra_curveminy, gra_curvemaxx, gra_curvemaxy;
static INTBIG        gra_arclx, gra_archx, gra_arcly, gra_archy;
static INTBIG        gra_curvecol, gra_curvemask, gra_curvestyle, gra_arcfirst;
static BOOLEAN       gra_arcthick;

static INTBIG  gra_arcfindoctant(INTBIG x, INTBIG y);
static void    gra_arcxformoctant(INTBIG x, INTBIG y, INTBIG oct, INTBIG *ox, INTBIG *oy);
static void    gra_arcdopixel(WINDOWFRAME *wf, INTBIG x, INTBIG y);
static void    gra_arcoutxform(WINDOWFRAME *wf, INTBIG x, INTBIG y);
static void    gra_arcbrescw(WINDOWFRAME *wf, INTBIG x, INTBIG y, INTBIG x1, INTBIG y1);
static void    gra_arcbresmidcw(WINDOWFRAME *wf, INTBIG x, INTBIG y);
static void    gra_arcbresmidccw(WINDOWFRAME *wf, INTBIG x, INTBIG y);
static void    gra_arcbresccw(WINDOWFRAME *wf, INTBIG x, INTBIG y, INTBIG x1, INTBIG y1);
static void    gra_drawdiscrow(WINDOWFRAME *wf, INTBIG thisy, INTBIG startx, INTBIG endx, GRAPHICS *desc);

void gra_termgraph(void)
{
	if (gra_polysegcount > 0) efree((CHAR *)gra_polysegs);
}

static INTBIG gra_badcoord(WINDOWFRAME *wf, INTBIG x, INTBIG y, CHAR *where )
{
	INTBIG height;

#if !defined(USEQT) && defined(ONUNIX)
	height = wf->trueheight;
#else
	height = wf->shei;
#endif

	if (x < 0 || x >= wf->swid || y < 0 || y >= height)
	{
		Q_UNUSED( where );
		return(1);
	}
	return(0);
}

/******************** GRAPHICS LINES ********************/

/*
 * Routine to draw a line on the off-screen buffer.
 */
void gra_drawline(WINDOWPART *win, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, GRAPHICS *desc,
	INTBIG texture)
{
	REGISTER INTBIG col, mask, lx, hx, ly, hy;
	REGISTER WINDOWFRAME *wf;

	/* get line type parameters */
	wf = win->frame;
	col = desc->col;      mask = ~desc->bits;
	y1 = wf->revy - y1;   y2 = wf->revy - y2;
	CHECKCOORD(wf, x1, y1, x_("line"));
	CHECKCOORD(wf, x2, y2, x_("line"));

	switch (texture)
	{
		case 0: gra_drawsolidline(wf, x1, y1, x2, y2, col, mask);       break;
		case 1: gra_drawpatline(wf, x1, y1, x2, y2, col, mask, 0x88);   break;
		case 2: gra_drawpatline(wf, x1, y1, x2, y2, col, mask, 0xE7);   break;
		case 3: gra_drawthickline(wf, x1, y1, x2, y2, col, mask);       break;
	}
	if (x1 < x2) { lx = x1;   hx = x2; } else { lx = x2;   hx = x1; }
	if (y1 < y2) { ly = y1;   hy = y2; } else { ly = y2;   hy = y1; }
	if (texture == 3)
	{
		lx--;   hx++;
		ly--;   hy++;
	}
	gra_setrect(wf, lx, hx+1, ly, hy+1);
}

/*
 * Routine to invert bits of the line on the off-screen buffer
 */
void gra_invertline(WINDOWPART *win, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2)
{
	REGISTER WINDOWFRAME *wf;
	REGISTER INTBIG lx, hx, ly, hy, dx, dy, d, incr1, incr2, x, y, xend, yend, yincr, xincr;

	/* get line type parameters */
	wf = win->frame;
	y1 = wf->revy - y1;   y2 = wf->revy - y2;
	CHECKCOORD(wf, x1, y1, x_("invline"));
	CHECKCOORD(wf, x2, y2, x_("invline"));

	/* initialize the Bresenham algorithm */
	dx = abs(x2-x1);
	dy = abs(y2-y1);
	if (dx > dy)
	{
		/* initialize for lines that increment along X */
		incr1 = 2 * dy;
		d = incr2 = 2 * (dy - dx);
		if (x1 > x2)
		{
			x = x2;   y = y2;   xend = x1;   yend = y1;
		} else
		{
			x = x1;   y = y1;   xend = x2;   yend = y2;
		}
		if (yend < y) yincr = -1; else yincr = 1;
		wf->rowstart[y][x] = ~wf->rowstart[y][x];

		/* draw line that increments along X */
		while (x < xend)
		{
			x++;
			if (d < 0) d += incr1; else
			{
				y += yincr;   d += incr2;
			}
			wf->rowstart[y][x] = ~wf->rowstart[y][x];
		}
	} else
	{
		/* initialize for lines that increment along Y */
		incr1 = 2 * dx;
		d = incr2 = 2 * (dx - dy);
		if (y1 > y2)
		{
			x = x2;   y = y2;   xend = x1;   yend = y1;
		} else
		{
			x = x1;   y = y1;   xend = x2;   yend = y2;
		}
		if (xend < x) xincr = -1; else xincr = 1;
		wf->rowstart[y][x] = ~wf->rowstart[y][x];

		/* draw line that increments along X */
		while (y < yend)
		{
			y++;
			if (d < 0) d += incr1; else
			{
				x += xincr;   d += incr2;
			}
			wf->rowstart[y][x] = ~wf->rowstart[y][x];
		}
	}
	if (x1 < x2) { lx = x1;   hx = x2; } else { lx = x2;   hx = x1; }
	if (y1 < y2) { ly = y1;   hy = y2; } else { ly = y2;   hy = y1; }
	gra_setrect(wf, lx, hx+1, ly, hy+1);
}

static void gra_drawpatline(WINDOWFRAME *wf, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, INTBIG col,
	INTBIG mask, INTBIG pattern)
{
	INTBIG dx, dy, d, incr1, incr2, x, y, xend, yend, yincr, xincr, i;

	/* initialize counter for line style */
	i = 0;

	/* initialize the Bresenham algorithm */
	dx = abs(x2-x1);
	dy = abs(y2-y1);
	if (dx > dy)
	{
		/* initialize for lines that increment along X */
		incr1 = 2 * dy;
		d = incr2 = 2 * (dy - dx);
		if (x1 > x2)
		{
			x = x2;   y = y2;   xend = x1;   yend = y1;
		} else
		{
			x = x1;   y = y1;   xend = x2;   yend = y2;
		}
		if (yend < y) yincr = -1; else yincr = 1;
		wf->rowstart[y][x] = (UCHAR1)((wf->rowstart[y][x]&mask) | col);

		/* draw line that increments along X */
		while (x < xend)
		{
			x++;
			if (d < 0) d += incr1; else
			{
				y += yincr;   d += incr2;
			}
			if (i == 7) i = 0; else i++;
			if ((pattern & (1 << i)) == 0) continue;
			wf->rowstart[y][x] = (UCHAR1)((wf->rowstart[y][x]&mask) | col);
		}
	} else
	{
		/* initialize for lines that increment along Y */
		incr1 = 2 * dx;
		d = incr2 = 2 * (dx - dy);
		if (y1 > y2)
		{
			x = x2;   y = y2;   xend = x1;   yend = y1;
		} else
		{
			x = x1;   y = y1;   xend = x2;   yend = y2;
		}
		if (xend < x) xincr = -1; else xincr = 1;
		wf->rowstart[y][x] = (UCHAR1)((wf->rowstart[y][x]&mask) | col);

		/* draw line that increments along X */
		while (y < yend)
		{
			y++;
			if (d < 0) d += incr1; else
			{
				x += xincr;   d += incr2;
			}
			if (i == 7) i = 0; else i++;
			if ((pattern & (1 << i)) == 0) continue;
			wf->rowstart[y][x] = (UCHAR1)((wf->rowstart[y][x]&mask) | col);
		}
	}
}

static void gra_drawsolidline(WINDOWFRAME *wf, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, INTBIG col, INTBIG mask)
{
	INTBIG dx, dy, d, incr1, incr2, x, y, xend, yend, yincr, xincr;

	/* initialize the Bresenham algorithm */
	dx = abs(x2-x1);
	dy = abs(y2-y1);
	if (dx > dy)
	{
		/* initialize for lines that increment along X */
		incr1 = 2 * dy;
		d = incr2 = 2 * (dy - dx);
		if (x1 > x2)
		{
			x = x2;   y = y2;   xend = x1;   yend = y1;
		} else
		{
			x = x1;   y = y1;   xend = x2;   yend = y2;
		}
		if (yend < y) yincr = -1; else yincr = 1;
		wf->rowstart[y][x] = (UCHAR1)((wf->rowstart[y][x]&mask) | col);

		/* draw line that increments along X */
		while (x < xend)
		{
			x++;
			if (d < 0) d += incr1; else
			{
				y += yincr;   d += incr2;
			}
			wf->rowstart[y][x] = (UCHAR1)((wf->rowstart[y][x]&mask) | col);
		}
	} else
	{
		/* initialize for lines that increment along Y */
		incr1 = 2 * dx;
		d = incr2 = 2 * (dx - dy);
		if (y1 > y2)
		{
			x = x2;   y = y2;   xend = x1;   yend = y1;
		} else
		{
			x = x1;   y = y1;   xend = x2;   yend = y2;
		}
		if (xend < x) xincr = -1; else xincr = 1;
		wf->rowstart[y][x] = (UCHAR1)((wf->rowstart[y][x]&mask) | col);

		/* draw line that increments along X */
		while (y < yend)
		{
			y++;
			if (d < 0) d += incr1; else
			{
				x += xincr;   d += incr2;
			}
			wf->rowstart[y][x] = (UCHAR1)((wf->rowstart[y][x]&mask) | col);
		}
	}
}

static void gra_drawthickline(WINDOWFRAME *wf, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, INTBIG col, INTBIG mask)
{
	INTBIG dx, dy, d, incr1, incr2, x, y, xend, yend, yincr, xincr;

	/* initialize the Bresenham algorithm */
	dx = abs(x2-x1);
	dy = abs(y2-y1);
	if (dx > dy)
	{
		/* initialize for lines that increment along X */
		incr1 = 2 * dy;
		d = incr2 = 2 * (dy - dx);
		if (x1 > x2)
		{
			x = x2;   y = y2;   xend = x1;   yend = y1;
		} else
		{
			x = x1;   y = y1;   xend = x2;   yend = y2;
		}
		if (yend < y) yincr = -1; else yincr = 1;
		gra_drawthickpoint(wf, x, y, mask, col);

		/* draw line that increments along X */
		while (x < xend)
		{
			x++;
			if (d < 0) d += incr1; else
			{
				y += yincr;
				d += incr2;
			}
			gra_drawthickpoint(wf, x, y, mask, col);
		}
	} else
	{
		/* initialize for lines that increment along Y */
		incr1 = 2 * dx;
		d = incr2 = 2 * (dx - dy);
		if (y1 > y2)
		{
			x = x2;   y = y2;   xend = x1;   yend = y1;
		} else
		{
			x = x1;   y = y1;   xend = x2;   yend = y2;
		}
		if (xend < x) xincr = -1; else xincr = 1;
		gra_drawthickpoint(wf, x, y, mask, col);

		/* draw line that increments along X */
		while (y < yend)
		{
			y++;
			if (d < 0) d += incr1; else
			{
				x += xincr;
				d += incr2;
			}
			gra_drawthickpoint(wf, x, y, mask, col);
		}
	}
}

static void gra_drawthickpoint(WINDOWFRAME *wf, INTBIG x, INTBIG y, INTBIG mask, INTBIG col)
{
	wf->rowstart[y][x] = (UCHAR1)((wf->rowstart[y][x]&mask) | col);
	if (x > 0)
		wf->rowstart[y][x-1] = (UCHAR1)((wf->rowstart[y][x-1]&mask) | col);
	if (x < wf->swid-1)
		wf->rowstart[y][x+1] = (UCHAR1)((wf->rowstart[y][x+1]&mask) | col);
	if (y > 0)
		wf->rowstart[y-1][x] = (UCHAR1)((wf->rowstart[y-1][x]&mask) | col);
	if (y < wf->shei-1)
		wf->rowstart[y+1][x] = (UCHAR1)((wf->rowstart[y+1][x]&mask) | col);
}

/******************** GRAPHICS POLYGONS ********************/

/*
 * Routine to draw a polygon on the off-screen buffer.
 */
void gra_drawpolygon(WINDOWPART *win, INTBIG *x, INTBIG *y, INTBIG count, GRAPHICS *desc)
{
	REGISTER INTBIG i, j, k, l, ycur=0, yrev, wrap, lx=0, hx=0, ly=0, hy=0;
	REGISTER UCHAR1 *row;
	REGISTER INTBIG col, mask, style, pat;
	REGISTER POLYSEG *a, *active, *edge, *lastedge, *left, *edgelist;
	REGISTER WINDOWFRAME *wf;

	/* get parameters */
	wf = win->frame;
	col = desc->col;   mask = ~desc->bits;
	style = desc->colstyle & NATURE;

	/* set redraw area */
	for(i=0; i<count; i++)
	{
		CHECKCOORD(wf, x[i], y[i], x_("polygon"));
		if (i == 0)
		{
			lx = hx = x[i];
			ly = hy = y[i];
		} else
		{
			lx = mini(lx, x[i]);
			hx = maxi(hx, x[i]);
			ly = mini(ly, y[i]);
			hy = maxi(hy, y[i]);
		}
	}
	gra_setrect(wf, lx, hx + 1, wf->revy-hy, wf->revy-ly + 1);

	/* make sure there is room in internal structures */
	if (count > gra_polysegcount)
	{
		if (gra_polysegcount > 0) efree((CHAR *)gra_polysegs);
		gra_polysegcount = 0;
		gra_polysegs = (POLYSEG *)emalloc(count * (sizeof (POLYSEG)), us_tool->cluster);
		if (gra_polysegs == 0) return;
		gra_polysegcount = count;
	}

	/* fill in internal structures */
	edgelist = NOPOLYSEG;
	for(i=0; i<count; i++)
	{
		if (i == 0)
		{
			gra_polysegs[i].fx = x[count-1];
			gra_polysegs[i].fy = y[count-1];
		} else
		{
			gra_polysegs[i].fx = x[i-1];
			gra_polysegs[i].fy = y[i-1];
		}
		gra_polysegs[i].tx = x[i];   gra_polysegs[i].ty = y[i];

		/* draw the edge lines to make the polygon clean */
		if ((desc->colstyle&(NATURE|OUTLINEPAT)) != PATTERNED)
			gra_drawsolidline(wf, gra_polysegs[i].fx, (wf->revy - gra_polysegs[i].fy),
				gra_polysegs[i].tx, (wf->revy - gra_polysegs[i].ty), col, mask);

		/* compute the direction of this edge */
		j = gra_polysegs[i].ty - gra_polysegs[i].fy;
		if (j > 0) gra_polysegs[i].direction = 1; else
			if (j < 0) gra_polysegs[i].direction = -1; else
				gra_polysegs[i].direction = 0;

		/* compute the X increment of this edge */
		if (j == 0) gra_polysegs[i].increment = 0; else
		{
			gra_polysegs[i].increment = gra_polysegs[i].tx - gra_polysegs[i].fx;
			if (gra_polysegs[i].increment != 0) gra_polysegs[i].increment =
				(gra_polysegs[i].increment * 65536 - j + 1) / j;
		}
		gra_polysegs[i].tx <<= 16;   gra_polysegs[i].fx <<= 16;

		/* make sure "from" is above "to" */
		if (gra_polysegs[i].fy > gra_polysegs[i].ty)
		{
			j = gra_polysegs[i].tx;
			gra_polysegs[i].tx = gra_polysegs[i].fx;
			gra_polysegs[i].fx = j;
			j = gra_polysegs[i].ty;
			gra_polysegs[i].ty = gra_polysegs[i].fy;
			gra_polysegs[i].fy = j;
		}

		/* insert this edge into the edgelist, sorted by ascending "fy" */
		if (edgelist == NOPOLYSEG)
		{
			edgelist = &gra_polysegs[i];
			gra_polysegs[i].nextedge = NOPOLYSEG;
		} else
		{
			/* insert by ascending "fy" */
			if (edgelist->fy > gra_polysegs[i].fy)
			{
				gra_polysegs[i].nextedge = edgelist;
				edgelist = &gra_polysegs[i];
			} else for(a = edgelist; a != NOPOLYSEG; a = a->nextedge)
			{
				if (a->nextedge == NOPOLYSEG ||
					a->nextedge->fy > gra_polysegs[i].fy)
				{
					/* insert after this */
					gra_polysegs[i].nextedge = a->nextedge;
					a->nextedge = &gra_polysegs[i];
					break;
				}
			}
		}
	}

	/* scan polygon and render */
	active = NOPOLYSEG;
	while (active != NOPOLYSEG || edgelist != NOPOLYSEG)
	{
		if (active == NOPOLYSEG)
		{
			active = edgelist;
			active->nextactive = NOPOLYSEG;
			edgelist = edgelist->nextedge;
			ycur = active->fy;
		}

		/* introduce edges from edge list into active list */
		while (edgelist != NOPOLYSEG && edgelist->fy <= ycur)
		{
			/* insert "edgelist" into active list, sorted by "fx" coordinate */
			if (active->fx > edgelist->fx ||
				(active->fx == edgelist->fx && active->increment > edgelist->increment))
			{
				edgelist->nextactive = active;
				active = edgelist;
				edgelist = edgelist->nextedge;
			} else for(a = active; a != NOPOLYSEG; a = a->nextactive)
			{
				if (a->nextactive == NOPOLYSEG ||
					a->nextactive->fx > edgelist->fx ||
						(a->nextactive->fx == edgelist->fx &&
							a->nextactive->increment > edgelist->increment))
				{
					/* insert after this */
					edgelist->nextactive = a->nextactive;
					a->nextactive = edgelist;
					edgelist = edgelist->nextedge;
					break;
				}
			}
		}

		/* generate regions to be filled in on current scan line */
		wrap = 0;
		left = active;
		for(edge = active; edge != NOPOLYSEG; edge = edge->nextactive)
		{
			wrap = wrap + edge->direction;
			if (wrap == 0)
			{
				j = (left->fx + 32768) >> 16;
				k = (edge->fx + 32768) >> 16;
				yrev = wf->revy - ycur;
				row = wf->rowstart[yrev];
				if (style == PATTERNED)
				{
					/* patterned fill */
					pat = desc->raster[yrev&15];
					if (pat != 0)
					{
						for(l=j; l<=k; l++)
						{
							if ((pat & (1 << (15-(l&15)))) != 0)
								row[l] = (UCHAR1)((row[l] & mask) | col);
						}
					}
				} else
				{
					/* solid fill */
					for(l=j; l<=k; l++)
						row[l] = (UCHAR1)((row[l] & mask) | col);
				}
				left = edge->nextactive;
			}
		}
		ycur++;

		/* update edges in active list */
		lastedge = NOPOLYSEG;
		for(edge = active; edge != NOPOLYSEG; edge = edge->nextactive)
		{
			if (ycur >= edge->ty)
			{
				if (lastedge == NOPOLYSEG) active = edge->nextactive;
					else lastedge->nextactive = edge->nextactive;
			} else
			{
				edge->fx += edge->increment;
				lastedge = edge;
			}
		}
	}
}

/******************** GRAPHICS BOXES ********************/

/*
 * Routine to draw a box on the off-screen buffer.
 */
void gra_drawbox(WINDOWPART *win, INTBIG lowx, INTBIG highx, INTBIG lowy, INTBIG highy,
	GRAPHICS *desc)
{
	REGISTER WINDOWFRAME *wf;
	REGISTER INTBIG style, x, y, left, right, bottom, top, pat;
	REGISTER UCHAR1 *thisrow, mask, col;

	/* get graphics parameters */
	wf = win->frame;
	col = (UCHAR1)desc->col;   mask = (UCHAR1)(~desc->bits);
	style = desc->colstyle & NATURE;
	left = lowx;                  right = highx + 1;
	bottom = wf->revy-lowy + 1;   top = wf->revy-highy;
	CHECKCOORD(wf, left, top, x_("box"));
	CHECKCOORD(wf, right-1, bottom-1, x_("box"));

	/* handle color drawing */
	if (style == PATTERNED)
	{
		/* special case the patterned fill */
		for(y=top; y<bottom; y++)
		{
			pat = desc->raster[y&15];
			if (pat == 0) continue;
			thisrow = &wf->rowstart[y][left];
			for(x=left; x<right; x++)
			{
				if ((pat & (1 << (15-(x&15)))) != 0)
					*thisrow = (*thisrow & mask) | col;
				thisrow++;
			}
		}
	} else
	{
		for(y=top; y<bottom; y++)
		{
			thisrow = &wf->rowstart[y][left];
			for(x=left; x<right; x++)
			{
				*thisrow = (*thisrow & mask) | col;
				thisrow++;
			}
		}
	}
	gra_setrect(wf, left, right, top, bottom);
}

/*
 * Routine to invert the bits in the box from (lowx, lowy) to (highx, highy) on the off-screen buffer
 */
void gra_invertbox(WINDOWPART *win, INTBIG lowx, INTBIG highx, INTBIG lowy, INTBIG highy)
{
	REGISTER WINDOWFRAME *wf;
	REGISTER INTBIG top, bottom, left, right, x, y;
	REGISTER UCHAR1 *thisrow;

	wf = win->frame;
	left = lowx;
	right = highx + 1;
	bottom = wf->revy - lowy + 1;
	top = wf->revy - highy;
	if (top < 0) top = 0;
	CHECKCOORD(wf, left, top, x_("invbox"));
	CHECKCOORD(wf, right-1, bottom-1, x_("invbox"));

	for(y=top; y<bottom; y++)
	{
		thisrow = wf->rowstart[y];
		for(x=left; x<right; x++) thisrow[x] = ~thisrow[x];
	}

	gra_setrect(wf, left, right, top, bottom);
}

/*
 * Routine to move bits on the off-screen buffer starting with the area at
 * (sx,sy) and ending at (dx,dy).  The size of the area to be
 * moved is "wid" by "hei".
 */
void gra_movebox(WINDOWPART *win, INTBIG sx, INTBIG sy, INTBIG wid, INTBIG hei,
	INTBIG dx, INTBIG dy)
{
	REGISTER WINDOWFRAME *wf;
	INTBIG fleft, fright, ftop, fbottom, tleft, tright, ttop, tbottom;
#ifdef USE_MEMMOVE
	INTBIG i;
#else
	INTBIG xsize, ysize, x, y, dir, fromstart, frominc, tostart, toinc;
	REGISTER UCHAR1 *frombase, *tobase;
#endif

	/* setup source rectangle */
	wf = win->frame;
	fleft = sx;
	fright = fleft + wid;
	ftop = wf->revy + 1 - sy - hei;
	fbottom = ftop + hei;
	CHECKCOORD(wf, fleft, ftop, x_("movebox"));
	CHECKCOORD(wf, fright-1, fbottom-1, x_("movebox"));

	/* setup destination rectangle */
	tleft = dx;
	tright = tleft + wid;
	ttop = wf->revy + 1 - dy - hei;
	tbottom = ttop + hei;
	CHECKCOORD(wf, tleft, ttop, x_("movebox"));
	CHECKCOORD(wf, tright-1, tbottom-1, x_("movebox"));

#ifdef USE_MEMMOVE
	/* move the rows */
	if (ttop < ftop)
	{
		for (i = 0; i < hei; i++)
			memcpy(wf->rowstart[ttop + i] + tleft, wf->rowstart[ftop + i] + fleft, wid);
	} else if (ttop > ftop)
	{
		for (i = hei - 1; i >= 0; i--)
			memcpy(wf->rowstart[ttop + i] + tleft, wf->rowstart[ftop + i] + fleft, wid);
	} else
	{
		for (i = 0; i < hei; i++)
			memmove(wf->rowstart[ttop + i] + tleft, wf->rowstart[ftop + i] + fleft, wid);
	}
#else
	/* determine size of bits to move */
	xsize = wid;   ysize = hei;

	/* determine direction of bit copy */
	if (fleft < tleft) dir = 1; else dir = 0;
	if (ftop < ttop)
	{
		fromstart = fbottom-1;   frominc = -1;
		tostart = tbottom-1;       toinc = -1;
	} else
	{
		fromstart = ftop;   frominc = 1;
		tostart = ttop;       toinc = 1;
	}

	/* move the bits */
	if (dir == 0)
	{
		/* normal forward copy in X */
		for(y = 0; y < ysize; y++)
		{
			frombase = wf->rowstart[fromstart] + fleft;
			fromstart += frominc;
			tobase = wf->rowstart[tostart] + tleft;
			tostart += toinc;
			for(x = 0; x < xsize; x++) *tobase++ = *frombase++;
		}
	} else
	{
		/* reverse copy in X */
		for(y = 0; y < ysize; y++)
		{
			frombase = wf->rowstart[fromstart] + fright;
			fromstart += frominc;
			tobase = wf->rowstart[tostart] + tright;
			tostart += toinc;
			for(x = 0; x < xsize; x++) *tobase-- = *frombase--;
		}
	}
#endif
	gra_setrect(wf, tleft, tright, ttop, tbottom);
}

/*
 * routine to save the contents of the box from "lx" to "hx" in X and from
 * "ly" to "hy" in Y.  A code is returned that identifies this box for
 * overwriting and restoring.  The routine returns negative if there is a error.
 */
INTBIG gra_savebox(WINDOWPART *win, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy)
{
	REGISTER WINDOWFRAME *wf;
	SAVEDBOX *box;
#ifdef USE_MEMMOVE
	REGISTER INTBIG i, y;
#else
	REGISTER INTBIG i, x, y;
#endif
	REGISTER INTBIG xsize, ysize, toindex;

	wf = win->frame;
	i = ly;   ly = wf->revy-hy;   hy = wf->revy-i;
	xsize = hx-lx+1;
	ysize = hy-ly+1;

	box = (SAVEDBOX *)emalloc((sizeof (SAVEDBOX)), us_tool->cluster);
	if (box == 0) return(-1);
	box->wf = win->frame;
	box->pix = (UCHAR1 *)emalloc(xsize * ysize, us_tool->cluster);
	if (box->pix == 0) return(-1);
	box->lx = lx;           box->hx = hx;
	box->ly = ly;           box->hy = hy;

	/* move the bits */
	toindex = 0;
	for(y = ly; y <= hy; y++)
	{
#ifdef USE_MEMMOVE
		memcpy(box->pix + toindex, wf->rowstart[y] + lx, xsize);
		toindex += xsize;
#else
		for(x = lx; x <= hx; x++)
			box->pix[toindex++] = wf->rowstart[y][x];
#endif
	}

	return((INTBIG)box);
}

/*
 * routine to shift the saved box "code" so that it is restored in a different
 * lcoation, offset by (dx,dy)
 */
void gra_movesavedbox(INTBIG code, INTBIG dx, INTBIG dy)
{
	REGISTER SAVEDBOX *box;

	if (code == -1) return;
	box = (SAVEDBOX *)code;
	box->lx += dx;       box->hx += dx;
	box->ly -= dy;       box->hy -= dy;
}

/*
 * routine to restore saved box "code" to the screen.  "destroy" is:
 *  0   restore box, do not free memory
 *  1   restore box, free memory
 * -1   free memory
 * Returns true if there is an error.
 */
BOOLEAN gra_restorebox(INTBIG code, INTBIG destroy)
{
	REGISTER WINDOWFRAME *wf;
	REGISTER SAVEDBOX *box;
	REGISTER INTBIG fromindex;
#ifdef USE_MEMMOVE
	REGISTER INTBIG xsize, y;
#else
	REGISTER INTBIG x, y;
#endif

	/* get the box */
	if (code == -1) return(TRUE);
	box = (SAVEDBOX *)code;
	wf = box->wf;

	/* move the bits */
	if (destroy >= 0)
	{
		fromindex = 0;
#ifdef USE_MEMMOVE
		xsize = box->hx - box->lx + 1;
		for(y = box->ly; y <= box->hy; y++) {
			memcpy(wf->rowstart[y] + box->lx, box->pix + fromindex, xsize);
			fromindex += xsize;
		}
#else
		for(y = box->ly; y <= box->hy; y++)
			for(x = box->lx; x <= box->hx; x++)
				wf->rowstart[y][x] = box->pix[fromindex++];
#endif
		gra_setrect(wf, box->lx, box->hx+1, box->ly, box->hy+1);
	}

	/* destroy this box's memory if requested */
	if (destroy != 0)
	{
		efree((CHAR *)box->pix);
		efree((CHAR *)box);
	}
	return(FALSE);
}

/******************** GRAPHICS TEXT ********************/

/*
 * Routine to draw a text on the off-screen buffer
 */
void gra_drawtext(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG rotation, CHAR *s, GRAPHICS *desc)
{
	REGISTER INTBIG len, col, top, bottom, left, right, mask, pos,
		lx, hx, ly, hy, sx, ex, i, j, dpos, desti;
	REGISTER UCHAR1 *dest, *ptr;
	UCHAR1 **rowstart;
	INTBIG wid, hei;
	REGISTER WINDOWFRAME *wf;

	/* get parameters */
	/* quit if string is null */
	len = estrlen(s);
	if (len == 0) return;

	/* get parameters */
	wf = win->frame;
	col = desc->col;   mask = ~desc->bits;
	aty = wf->revy - aty;
	lx = win->uselx;   hx = win->usehx;
	ly = wf->revy - win->usehy;
	hy = wf->revy - win->usely;

	/* clip to window frame area */
	if (lx < 0) lx = 0;
	if (hx >= wf->swid) hx = wf->swid - 1;
	if (ly < 0) ly = 0;
#if !defined(USEQT) && defined(ONUNIX)
	if (hy >= wf->trueheight) hy = wf->trueheight - 1;
#else
	if (hy >= wf->shei) hy = wf->shei - 1;
#endif

	/* get text description */
	if (gettextbits(win, s, &wid, &hei, &rowstart)) return;

	switch (rotation)
	{
		case 0:			/* no rotation */
			/* copy text buffer to the main offscreen buffer */
			if (atx < lx) sx = lx-atx; else sx = 0;
			if (atx+wid >= hx) ex = hx - atx; else ex = wid;
			pos = aty - hei;
			for(i=0; i<hei; i++)
			{
				desti = pos + i;
				if (desti < ly || desti > hy) continue;
				ptr = rowstart[i];
				dest = wf->rowstart[desti];
				for(j=sx; j<ex; j++)
				{
					if (ptr[j] == 0) continue;
					dpos = atx + j;
					dest[dpos] = (UCHAR1)((dest[dpos] & mask) | col);
				}
			}
			left = atx;     right = atx + wid;
			bottom = aty;   top = aty - hei;
			break;
		case 1:			/* 90 degrees counterclockwise */
			if (atx-hei < lx) sx = lx+hei-atx; else sx = 0;
			if (atx >= hx) ex = hx - atx; else ex = hei;
			pos = aty - wid;
			for(i=0; i<wid; i++)
			{
				desti = pos + i;
				if (desti < ly || desti > hy) continue;
				dest = wf->rowstart[desti];
				for(j=sx; j<ex; j++)
				{
					ptr = rowstart[hei-1-j];
					if (ptr[wid-1-i] == 0) continue;
					dpos = atx - j;
					dest[dpos] = (UCHAR1)((dest[dpos] & mask) | col);
				}
			}
			left = atx - hei;   right = atx;
			bottom = aty;       top = aty - wid;
			break;
		case 2:			/* 180 degrees */
			if (atx-wid < lx) sx = lx+wid-atx; else sx = 0;
			if (atx >= hx) ex = hx - atx; else ex = wid;
			pos = aty;
			for(i=0; i<hei; i++)
			{
				desti = pos + hei - i;
				if (desti < ly || desti > hy) continue;
				ptr = rowstart[i];
				dest = wf->rowstart[desti];
				for(j=sx; j<ex; j++)
				{
					if (ptr[j] == 0) continue;
					dpos = atx - j;
					dest[dpos] = (UCHAR1)((dest[dpos] & mask) | col);
				}
			}
			left = atx - wid;     right = atx;
			bottom = aty + hei;   top = aty;
			break;
		case 3:			/* 90 degrees clockwise */
			if (atx < lx) sx = lx-atx; else sx = 0;
			if (atx+hei >= hx) ex = hx - atx; else ex = hei;
			pos = aty;
			for(i=0; i<wid; i++)
			{
				desti = pos + i;
				if (desti < ly || desti > hy) continue;
				dest = wf->rowstart[desti];
				for(j=sx; j<ex; j++)
				{
					ptr = rowstart[hei-1-j];
					if (ptr[i] == 0) continue;
					dpos = atx + j;
					dest[dpos] = (UCHAR1)((dest[dpos] & mask) | col);
				}
			}
			left = atx;           right = atx + hei;
			bottom = aty - wid;   top = aty;
			break;
		default:
			return;
	}

	/* set redraw area */
	gra_setrect(wf, left, right, top, bottom);
}

/******************** CIRCLE DRAWING ********************/

/*
 * Routine to draw a circle on the off-screen buffer
 */
void gra_drawcircle(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius, GRAPHICS *desc)
{
	REGISTER WINDOWFRAME *wf;
	REGISTER INTBIG col, mask, top, bottom, left, right;
	REGISTER INTBIG x, y, d, maxx, maxy, thisx, thisy;
	REGISTER UCHAR1 *thisrow;

	/* get parameters */
	wf = win->frame;
	col = desc->col;   mask = ~desc->bits;
	aty = wf->revy - aty;

	/* set redraw area */
	left = atx - radius;
	right = atx + radius + 1;
	top = aty - radius;
	bottom = aty + radius + 1;
	CHECKCOORD(wf, left, top, x_("circle"));
	CHECKCOORD(wf, right, bottom, x_("circle"));
	gra_setrect(wf, left, right, top, bottom);

	maxx = wf->swid;
	maxy = wf->shei;
	x = 0;   y = radius;
	d = 3 - 2 * radius;
	if (left >= 0 && right < maxx && top >= 0 && bottom < maxy)
	{
		/* no clip version is faster */
		while (x <= y)
		{
			thisrow = wf->rowstart[aty + y];
			thisrow[atx + x] = (UCHAR1)((thisrow[atx + x] & mask) | col);
			thisrow[atx - x] = (UCHAR1)((thisrow[atx - x] & mask) | col);

			thisrow = wf->rowstart[aty - y];
			thisrow[atx + x] = (UCHAR1)((thisrow[atx + x] & mask) | col);
			thisrow[atx - x] = (UCHAR1)((thisrow[atx - x] & mask) | col);

			thisrow = wf->rowstart[aty + x];
			thisrow[atx + y] = (UCHAR1)((thisrow[atx + y] & mask) | col);
			thisrow[atx - y] = (UCHAR1)((thisrow[atx - y] & mask) | col);

			thisrow = wf->rowstart[aty - x];
			thisrow[atx + y] = (UCHAR1)((thisrow[atx + y] & mask) | col);
			thisrow[atx - y] = (UCHAR1)((thisrow[atx - y] & mask) | col);

			if (d < 0) d += 4*x + 6; else
			{
				d += 4 * (x-y) + 10;
				y--;
			}
			x++;
		}
	} else
	{
		/* clip version */
		while (x <= y)
		{
			thisy = aty + y;
			if (thisy >= 0 && thisy < maxy)
			{
				thisrow = wf->rowstart[thisy];
				thisx = atx + x;
				if (thisx >= 0 && thisx < maxx)
					thisrow[thisx] = (UCHAR1)((thisrow[thisx] & mask) | col);
				thisx = atx - x;
				if (thisx >= 0 && thisx < maxx)
					thisrow[thisx] = (UCHAR1)((thisrow[thisx] & mask) | col);
			}

			thisy = aty - y;
			if (thisy >= 0 && thisy < maxy)
			{
				thisrow = wf->rowstart[thisy];
				thisx = atx + x;
				if (thisx >= 0 && thisx < maxx)
					thisrow[thisx] = (UCHAR1)((thisrow[thisx] & mask) | col);
				thisx = atx - x;
				if (thisx >= 0 && thisx < maxx)
					thisrow[thisx] = (UCHAR1)((thisrow[thisx] & mask) | col);
			}

			thisy = aty + x;
			if (thisy >= 0 && thisy < maxy)
			{
				thisrow = wf->rowstart[thisy];
				thisx = atx + y;
				if (thisx >= 0 && thisx < maxx)
					thisrow[thisx] = (UCHAR1)((thisrow[thisx] & mask) | col);
				thisx = atx - y;
				if (thisx >= 0 && thisx < maxx)
					thisrow[thisx] = (UCHAR1)((thisrow[thisx] & mask) | col);
			}

			thisy = aty - x;
			if (thisy >= 0 && thisy < maxy)
			{
				thisrow = wf->rowstart[thisy];
				thisx = atx + y;
				if (thisx >= 0 && thisx < maxx)
					thisrow[thisx] = (UCHAR1)((thisrow[thisx] & mask) | col);
				thisx = atx - y;
				if (thisx >= 0 && thisx < maxx)
					thisrow[thisx] = (UCHAR1)((thisrow[thisx] & mask) | col);
			}

			if (d < 0) d += 4*x + 6; else
			{
				d += 4 * (x-y) + 10;
				y--;
			}
			x++;
		}
	}
}

/*
 * Routine to draw a thick circle on the off-screen buffer
 */
void gra_drawthickcircle(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius,
	GRAPHICS *desc)
{
	REGISTER INTBIG col, mask, top, bottom, left, right;
	REGISTER INTBIG x, y, d, maxx, maxy, thisx, thisy;
	REGISTER WINDOWFRAME *wf;
	REGISTER UCHAR1 *thisrow1, *thisrow2, *thisrow3, *thisrow4,
		*thisrow1m, *thisrow2m, *thisrow3m, *thisrow4m,
		*thisrow1p, *thisrow2p, *thisrow3p, *thisrow4p;

	/* get parameters */
	wf = win->frame;
	col = desc->col;   mask = ~desc->bits;
	aty = wf->revy - aty;

	/* set redraw area */
	left = atx - radius - 1;
	right = atx + radius + 2;
	top = aty - radius - 1;
	bottom = aty + radius + 2;
	gra_setrect(wf, left, right, top, bottom);

	maxx = wf->swid;
	maxy = wf->shei;
	x = 0;   y = radius;
	d = 3 - 2 * radius;
	if (left >= 0 && right < maxx && top >= 0 && bottom < maxy)
	{
		/* no clip version is faster */
		while (x <= y)
		{
			/* draw basic circle */
			thisrow1 = wf->rowstart[aty + y];
			thisrow2 = wf->rowstart[aty - y];
			thisrow3 = wf->rowstart[aty + x];
			thisrow4 = wf->rowstart[aty - x];
			thisrow1[atx + x] = (UCHAR1)((thisrow1[atx + x] & mask) | col);
			thisrow1[atx - x] = (UCHAR1)((thisrow1[atx - x] & mask) | col);

			thisrow2[atx + x] = (UCHAR1)((thisrow2[atx + x] & mask) | col);
			thisrow2[atx - x] = (UCHAR1)((thisrow2[atx - x] & mask) | col);

			thisrow3[atx + y] = (UCHAR1)((thisrow3[atx + y] & mask) | col);
			thisrow3[atx - y] = (UCHAR1)((thisrow3[atx - y] & mask) | col);

			thisrow4[atx + y] = (UCHAR1)((thisrow4[atx + y] & mask) | col);
			thisrow4[atx - y] = (UCHAR1)((thisrow4[atx - y] & mask) | col);

			/* draw 1 pixel around it to make it thick */
			thisrow1m = wf->rowstart[aty + y - 1];
			thisrow2m = wf->rowstart[aty - y - 1];
			thisrow3m = wf->rowstart[aty + x - 1];
			thisrow4m = wf->rowstart[aty - x - 1];
			thisrow1p = wf->rowstart[aty + y + 1];
			thisrow2p = wf->rowstart[aty - y + 1];
			thisrow3p = wf->rowstart[aty + x + 1];
			thisrow4p = wf->rowstart[aty - x + 1];

			thisrow1[atx + x + 1] = (UCHAR1)((thisrow1[atx + x + 1] & mask) | col);
			thisrow1[atx + x - 1] = (UCHAR1)((thisrow1[atx + x - 1] & mask) | col);
			thisrow1[atx - x + 1] = (UCHAR1)((thisrow1[atx - x + 1] & mask) | col);
			thisrow1[atx - x - 1] = (UCHAR1)((thisrow1[atx - x - 1] & mask) | col);
			thisrow1m[atx + x] = (UCHAR1)((thisrow1m[atx + x] & mask) | col);
			thisrow1m[atx - x] = (UCHAR1)((thisrow1m[atx - x] & mask) | col);
			thisrow1p[atx + x] = (UCHAR1)((thisrow1p[atx + x] & mask) | col);
			thisrow1p[atx - x] = (UCHAR1)((thisrow1p[atx - x] & mask) | col);

			thisrow2[atx + x + 1] = (UCHAR1)((thisrow2[atx + x + 1] & mask) | col);
			thisrow2[atx + x - 1] = (UCHAR1)((thisrow2[atx + x - 1] & mask) | col);
			thisrow2[atx - x + 1] = (UCHAR1)((thisrow2[atx - x + 1] & mask) | col);
			thisrow2[atx - x - 1] = (UCHAR1)((thisrow2[atx - x - 1] & mask) | col);
			thisrow2m[atx + x] = (UCHAR1)((thisrow2m[atx + x] & mask) | col);
			thisrow2m[atx - x] = (UCHAR1)((thisrow2m[atx - x] & mask) | col);
			thisrow2p[atx + x] = (UCHAR1)((thisrow2p[atx + x] & mask) | col);
			thisrow2p[atx - x] = (UCHAR1)((thisrow2p[atx - x] & mask) | col);

			thisrow3[atx + y + 1] = (UCHAR1)((thisrow3[atx + y + 1] & mask) | col);
			thisrow3[atx + y - 1] = (UCHAR1)((thisrow3[atx + y - 1] & mask) | col);
			thisrow3[atx - y + 1] = (UCHAR1)((thisrow3[atx - y + 1] & mask) | col);
			thisrow3[atx - y - 1] = (UCHAR1)((thisrow3[atx - y - 1] & mask) | col);
			thisrow3m[atx + y] = (UCHAR1)((thisrow3m[atx + y] & mask) | col);
			thisrow3m[atx - y] = (UCHAR1)((thisrow3m[atx - y] & mask) | col);
			thisrow3p[atx + y] = (UCHAR1)((thisrow3p[atx + y] & mask) | col);
			thisrow3p[atx - y] = (UCHAR1)((thisrow3p[atx - y] & mask) | col);

			thisrow4[atx + y + 1] = (UCHAR1)((thisrow4[atx + y + 1] & mask) | col);
			thisrow4[atx + y - 1] = (UCHAR1)((thisrow4[atx + y - 1] & mask) | col);
			thisrow4[atx - y + 1] = (UCHAR1)((thisrow4[atx - y + 1] & mask) | col);
			thisrow4[atx - y - 1] = (UCHAR1)((thisrow4[atx - y - 1] & mask) | col);
			thisrow4m[atx + y] = (UCHAR1)((thisrow4m[atx + y] & mask) | col);
			thisrow4m[atx - y] = (UCHAR1)((thisrow4m[atx - y] & mask) | col);
			thisrow4p[atx + y] = (UCHAR1)((thisrow4p[atx + y] & mask) | col);
			thisrow4p[atx - y] = (UCHAR1)((thisrow4p[atx - y] & mask) | col);

			if (d < 0) d += 4*x + 6; else
			{
				d += 4 * (x-y) + 10;
				y--;
			}
			x++;
		}
	} else
	{
		/* clip version */
		while (x <= y)
		{
			thisy = aty + y;
			if (thisy >= 0 && thisy < maxy)
			{
				thisx = atx + x;
				if (thisx >= 0 && thisx < maxx)
					gra_drawthickpoint(wf, thisx, thisy, mask, col);
				thisx = atx - x;
				if (thisx >= 0 && thisx < maxx)
					gra_drawthickpoint(wf, thisx, thisy, mask, col);
			}

			thisy = aty - y;
			if (thisy >= 0 && thisy < maxy)
			{
				thisx = atx + x;
				if (thisx >= 0 && thisx < maxx)
					gra_drawthickpoint(wf, thisx, thisy, mask, col);
				thisx = atx - x;
				if (thisx >= 0 && thisx < maxx)
					gra_drawthickpoint(wf, thisx, thisy, mask, col);
			}

			thisy = aty + x;
			if (thisy >= 0 && thisy < maxy)
			{
				thisx = atx + y;
				if (thisx >= 0 && thisx < maxx)
					gra_drawthickpoint(wf, thisx, thisy, mask, col);
				thisx = atx - y;
				if (thisx >= 0 && thisx < maxx)
					gra_drawthickpoint(wf, thisx, thisy, mask, col);
			}

			thisy = aty - x;
			if (thisy >= 0 && thisy < maxy)
			{
				thisx = atx + y;
				if (thisx >= 0 && thisx < maxx)
					gra_drawthickpoint(wf, thisx, thisy, mask, col);
				thisx = atx - y;
				if (thisx >= 0 && thisx < maxx)
					gra_drawthickpoint(wf, thisx, thisy, mask, col);
			}

			if (d < 0) d += 4*x + 6; else
			{
				d += 4 * (x-y) + 10;
				y--;
			}
			x++;
		}
	}
}

/******************** DISC DRAWING ********************/

/*
 * routine to draw a scan line of the filled-in circle of radius "radius"
 */
static void gra_drawdiscrow(WINDOWFRAME *wf, INTBIG thisy, INTBIG startx, INTBIG endx, GRAPHICS *desc)
{
	REGISTER UCHAR1 *thisrow;
	REGISTER INTBIG x;
	REGISTER INTBIG pat;

	if (thisy < gra_curveminy || thisy >= gra_curvemaxy) return;
	thisrow = wf->rowstart[thisy];
	if (startx < gra_curveminx) startx = gra_curveminx;
	if (endx >= gra_curvemaxx) endx = gra_curvemaxx - 1;
	if (gra_curvestyle == PATTERNED)
	{
		pat = desc->raster[thisy&15];
		if (pat != 0)
		{
			for(x=startx; x<=endx; x++)
				if ((pat & (1 << (15-(x&15)))) != 0)
					thisrow[x] = (UCHAR1)((thisrow[x] & gra_curvemask) | gra_curvecol);
		}
	} else
	{
		for(x=startx; x<=endx; x++)
			thisrow[x] = (UCHAR1)((thisrow[x] & gra_curvemask) | gra_curvecol);
	}
}

/*
 * routine to draw a filled-in circle of radius "radius" on the off-screen buffer
 */
void gra_drawdisc(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius, GRAPHICS *desc)
{
	REGISTER WINDOWFRAME *wf;
	REGISTER INTBIG x, y, d;
	REGISTER INTBIG top, bottom, left, right;
	REGISTER UCHAR1 *thisrow;

	/* get parameters */
	wf = win->frame;
	gra_curvestyle = desc->colstyle & NATURE;
	gra_curvecol = desc->col;   gra_curvemask = ~desc->bits;

	/* set redraw area */
	aty = wf->revy - aty;
	left = atx - radius;
	right = atx + radius + 1;
	top = aty - radius;
	bottom = aty + radius + 1;
	gra_setrect(wf, left, right, top, bottom);

	/* draw a more general disc */
	gra_curveminx = maxi(win->uselx, 0);
	gra_curvemaxx = mini(win->usehx, wf->swid);
	gra_curveminy = maxi(wf->revy - win->usehy, 0);
	gra_curvemaxy = mini(wf->revy - win->usely, wf->shei);

	if (radius == 1)
	{
		/* just fill the area for discs this small */
		if (left < gra_curveminx) left = gra_curveminx;
		if (right >= gra_curvemaxx) right = gra_curvemaxx - 1;
		for(y=top; y<bottom; y++)
		{
			if (y < gra_curveminy || y >= gra_curvemaxy) continue;
			thisrow = wf->rowstart[y];
			for(x=left; x<right; x++)
				thisrow[x] = (UCHAR1)((thisrow[x] & gra_curvemask) | gra_curvecol);
		}
		return;
	}

	x = 0;   y = radius;
	d = 3 - 2 * radius;
	while (x <= y)
	{
		gra_drawdiscrow(wf, aty+y, atx-x, atx+x, desc);
		gra_drawdiscrow(wf, aty-y, atx-x, atx+x, desc);
		gra_drawdiscrow(wf, aty+x, atx-y, atx+y, desc);
		gra_drawdiscrow(wf, aty-x, atx-y, atx+y, desc);

		if (d < 0) d += 4*x + 6; else
		{
			d += 4 * (x-y) + 10;
			y--;
		}
		x++;
	}
}

/******************** ARC DRAWING ********************/

static INTBIG gra_arcfindoctant(INTBIG x, INTBIG y)
{
	if (x > 0)
		if (y >= 0)
			if (y >= x)	 return 7;
			else         return 8;
		else
			if (x >= -y) return 1;
			else         return 2;
	else
		if (y > 0)
			if (y > -x)  return 6;
			else         return 5;
		else
			if (y > x)   return 4;
			else         return 3;
}

static void gra_arcxformoctant(INTBIG x, INTBIG y, INTBIG oct, INTBIG *ox, INTBIG *oy)
{
	switch (oct)
	{
		case 1 : *ox = -y;   *oy = x;   break;
		case 2 : *ox = x;    *oy = -y;  break;
		case 3 : *ox = -x;   *oy = -y;  break;
		case 4 : *ox = -y;   *oy = -x;  break;
		case 5 : *ox = y;    *oy = -x;  break;
		case 6 : *ox = -x;   *oy = y;   break;
		case 7 : *ox = x;    *oy = y;   break;
		case 8 : *ox = y;    *oy = x;   break;
	}
}

static void gra_arcdopixel(WINDOWFRAME *wf, INTBIG x, INTBIG y)
{
	if (x < 0 || x >= gra_curvemaxx || y < 0 || y >= gra_curvemaxy) return;
	if (gra_arcfirst != 0)
	{
		gra_arcfirst = 0;
		gra_arclx = gra_archx = x;
		gra_arcly = gra_archy = y;
	} else
	{
		if (x < gra_arclx) gra_arclx = x;
		if (x > gra_archx) gra_archx = x;
		if (y < gra_arcly) gra_arcly = y;
		if (y > gra_archy) gra_archy = y;
	}
	CHECKCOORD(wf, x, y, x_("arc"));
	if (gra_arcthick)
	{
		gra_drawthickpoint(wf, x, y, gra_curvemask, gra_curvecol);
	} else
	{
		wf->rowstart[y][x] = (UCHAR1)((wf->rowstart[y][x] & gra_curvemask) | gra_curvecol);
	}
}

static void gra_arcoutxform(WINDOWFRAME *wf, INTBIG x, INTBIG y)
{
	if (gra_arcocttable[1]) gra_arcdopixel(wf,  y + gra_arccenterx, -x + gra_arccentery);
	if (gra_arcocttable[2]) gra_arcdopixel(wf,  x + gra_arccenterx, -y + gra_arccentery);
	if (gra_arcocttable[3]) gra_arcdopixel(wf, -x + gra_arccenterx, -y + gra_arccentery);
	if (gra_arcocttable[4]) gra_arcdopixel(wf, -y + gra_arccenterx, -x + gra_arccentery);
	if (gra_arcocttable[5]) gra_arcdopixel(wf, -y + gra_arccenterx,  x + gra_arccentery);
	if (gra_arcocttable[6]) gra_arcdopixel(wf, -x + gra_arccenterx,  y + gra_arccentery);
	if (gra_arcocttable[7]) gra_arcdopixel(wf,  x + gra_arccenterx,  y + gra_arccentery);
	if (gra_arcocttable[8]) gra_arcdopixel(wf,  y + gra_arccenterx,  x + gra_arccentery);
}

static void gra_arcbrescw(WINDOWFRAME *wf, INTBIG x, INTBIG y, INTBIG x1, INTBIG y1)
{
	REGISTER INTBIG d;

	d = 3 - 2 * y + 4 * x;
	while (x < x1 && y > y1)
	{
		gra_arcoutxform(wf, x, y);
		if (d < 0) d += 4*x+6; else
		{
			d += 4*(x-y)+10;
			y--;
		}
		x++;
	}

	/* get to the end */
	for ( ; x < x1; x++) gra_arcoutxform(wf, x, y);
	for ( ; y > y1; y--) gra_arcoutxform(wf, x, y);
   gra_arcoutxform(wf, x1, y1);
}

static void gra_arcbresmidcw(WINDOWFRAME *wf, INTBIG x, INTBIG y)
{
	REGISTER INTBIG d;

	d = 3 - 2 * y + 4 * x;
	while (x < y)
	{
		gra_arcoutxform(wf, x, y);
		if (d < 0) d += 4*x+6; else
		{
			d += 4*(x-y)+10;
			y--;
		}
		x++;
   }
   if (x == y) gra_arcoutxform(wf, x, y);
}

static void gra_arcbresmidccw(WINDOWFRAME *wf, INTBIG x, INTBIG y)
{
	REGISTER INTBIG d;

	d = 3 + 2 * y - 4 * x;
	while (x > 0)
	{
		gra_arcoutxform(wf, x, y);
		if (d > 0) d += 6-4*x; else
		{
			d += 4*(y-x)+10;
			y++;
		}
		x--;
   }
   gra_arcoutxform(wf, 0, gra_arcradius);
}

static void gra_arcbresccw(WINDOWFRAME *wf, INTBIG x, INTBIG y, INTBIG x1, INTBIG y1)
{
	REGISTER INTBIG d;

	d = 3 + 2 * y + 4 * x;
	while(x > x1 && y < y1)
	{
		/* not always correct */
		gra_arcoutxform(wf, x, y);
		if (d > 0) d += 6 - 4*x; else
		{
			d += 4*(y-x)+10;
			y++;
		}
		x--;
	}

	/* get to the end */
	for ( ; x > x1; x--) gra_arcoutxform(wf, x, y);
	for ( ; y < y1; y++) gra_arcoutxform(wf, x, y);
	gra_arcoutxform(wf, x1, y1);
}

/*
 * draws an arc centered at (centerx, centery), clockwise,
 * passing by (x1,y1) and (x2,y2)
 */
void gra_drawcirclearc(WINDOWPART *win, INTBIG centerx, INTBIG centery, INTBIG x1, INTBIG y1,
	INTBIG x2, INTBIG y2, BOOLEAN thick, GRAPHICS *desc)
{
	REGISTER WINDOWFRAME *wf;
	REGISTER INTBIG alternate, pa_x, pa_y, pb_x, pb_y, i, diff;
	INTBIG x, y;
	REGISTER INTBIG start_oct, end_oct;

	/* ignore tiny arcs */
	if (x1 == x2 && y1 == y2) return;

	/* get parameters */
	wf = win->frame;
	gra_curvecol = desc->col;   gra_curvemask = ~desc->bits;
	gra_curvemaxx = wf->swid;
	gra_curvemaxy = wf->shei;
	y1 = wf->revy - y1;
	y2 = wf->revy - y2;
	gra_arccentery = wf->revy - centery;
	gra_arccenterx = centerx;
	pa_x = x2 - gra_arccenterx;
	pa_y = y2 - gra_arccentery;
	pb_x = x1 - gra_arccenterx;
	pb_y = y1 - gra_arccentery;
	gra_arcradius = computedistance(gra_arccenterx, gra_arccentery, x2, y2);
	alternate = computedistance(gra_arccenterx, gra_arccentery, x1, y1);
	start_oct = gra_arcfindoctant(pa_x, pa_y);
	end_oct   = gra_arcfindoctant(pb_x, pb_y);
	gra_arcfirst = 1;
	gra_arcthick = thick;

	/* move the point */
	if (gra_arcradius != alternate)
	{
		diff = gra_arcradius-alternate;
		switch (end_oct)
		{
			case 6:
			case 7: /*  y >  x */ pb_y += diff;  break;
			case 8: /*  x >  y */
			case 1: /*  x > -y */ pb_x += diff;  break;
			case 2: /* -y >  x */
			case 3: /* -y > -x */ pb_y -= diff;  break;
			case 4: /* -y < -x */
			case 5: /*  y < -x */ pb_x -= diff;  break;
		}
	}

	for(i=1; i<9; i++) gra_arcocttable[i] = 0;

	if (start_oct == end_oct)
	{
		INTBIG x1, y1, x2, y2;

		gra_arcocttable[start_oct] = 1;
		gra_arcxformoctant(pa_x, pa_y, start_oct, &x1, &y1);
		gra_arcxformoctant(pb_x, pb_y, start_oct, &x2 ,&y2);

		if (ODD(start_oct)) gra_arcbrescw(wf, x1, y1, x2, y2);
		else				gra_arcbresccw(wf, x1 ,y1, x2, y2);
		gra_arcocttable[start_oct] = 0;
	} else
	{
		gra_arcocttable[start_oct] = 1;
		gra_arcxformoctant(pa_x, pa_y, start_oct, &x, &y);
		if (ODD(start_oct)) gra_arcbresmidcw(wf, x, y);
		else				gra_arcbresmidccw(wf, x, y);
		gra_arcocttable[start_oct] = 0;

		gra_arcocttable[end_oct] = 1;
		gra_arcxformoctant(pb_x, pb_y, end_oct, &x, &y);
		if (ODD(end_oct)) gra_arcbresmidccw(wf, x, y);
		else			  gra_arcbresmidcw(wf, x, y);
		gra_arcocttable[end_oct] = 0;

		if (MODP(start_oct+1) != end_oct)
		{
			if (MODP(start_oct+1) == MODM(end_oct-1))
				gra_arcocttable[MODP(start_oct+1)] = 1; else
					for(i = MODP(start_oct+1); i != end_oct; i = MODP(i+1))
						gra_arcocttable[i] = 1;
			gra_arcbresmidcw(wf, 0, gra_arcradius);
		}
	}

	/* set redraw area */
	if (gra_arcfirst == 0)
		gra_setrect(wf, gra_arclx, gra_archx+1, gra_arcly, gra_archy+1);
}

/******************** GRID CONTROL ********************/

/*
 * fast grid drawing routine
 */
void gra_drawgrid(WINDOWPART *win, POLYGON *obj)
{
	REGISTER WINDOWFRAME *wf;
	REGISTER INTBIG i, j, xnum, xden, ynum, yden, x0,y0, x1,y1, x2,y2, x3,y3,
		x4,y4, x5,y5, x10, y10, y10mod, xspacing, yspacing, y1base, x1base;
	REGISTER INTBIG x, y, fatdots;
	REGISTER VARIABLE *var;

	wf = win->frame;
	x0 = obj->xv[0];   y0 = obj->yv[0];		/* screen space grid spacing */
	x1 = obj->xv[1];   y1 = obj->yv[1];		/* screen space grid start */
	x2 = obj->xv[2];   y2 = obj->yv[2];		/* display space low */
	x3 = obj->xv[3];   y3 = obj->yv[3];		/* display space high */
	x4 = obj->xv[4];   y4 = obj->yv[4];		/* screen space low */
	x5 = obj->xv[5];   y5 = obj->yv[5];		/* screen space high */
	CHECKCOORD(wf, x2, y2, x_("grid"));
	CHECKCOORD(wf, x3, y3, x_("grid"));

	var = getvalkey((INTBIG)us_tool, VTOOL, -1, us_gridboldspacingkey);
	if (var == NOVARIABLE) xspacing = yspacing = 10; else
	{
		if ((var->type&VISARRAY) == 0)
			xspacing = yspacing = var->addr; else
		{
			xspacing = ((INTBIG *)var->addr)[0];
			yspacing = ((INTBIG *)var->addr)[1];
		}
	}

	xnum = x3 - x2;
	xden = x5 - x4;
	ynum = y3 - y2;
	yden = y5 - y4;
	x10 = x0*xspacing;       y10 = y0*yspacing;
	y1base = y1 - (y1 / y0 * y0);
	x1base = x1 - (x1 / x0 * x0);

	/* adjust grid placement according to scale */
	fatdots = 0;
	if (muldiv(x0, xnum, xden) < 5 || muldiv(y0, ynum, yden) < 5)
	{
		x1 = x1base - (x1base - x1) / x10 * x10;   x0 = x10;
		y1 = y1base - (y1base - y1) / y10 * y10;   y0 = y10;
	} else if (muldiv(x0, xnum, xden) > 75 && muldiv(y0, ynum, yden) > 75)
	{
		fatdots = 1;
	}

	/* draw the grid to the offscreen buffer */
	for(i = y1; i < y5; i += y0)
	{
		y = muldiv(i-y4, ynum, yden) + y2;
		if (y < y2 || y > y3) continue;
		y = wf->revy - y;
		y10mod = (i-y1base) % y10;
		for(j = x1; j < x5; j += x0)
		{
			x = muldiv(j-x4, xnum, xden) + x2;
			if (x >= x2 && x <= x3) wf->rowstart[y][x] |= GRID;

			/* special case every 10 grid points in each direction */
			if (fatdots != 0 || ((j-x1base)%x10) == 0 && y10mod == 0)
			{
				if (x > x2) wf->rowstart[y][x-1] |= GRID;
				if (x < x3) wf->rowstart[y][x+1] |= GRID;
				if (y > y2) wf->rowstart[y-1][x] |= GRID;
				if (y < y3) wf->rowstart[y+1][x] |= GRID;
				if (fatdots != 0 && ((j-x1base)%x10) == 0 && y10mod == 0)
				{
					if (x-1 > x2) wf->rowstart[y][x-2] |= GRID;
					if (x+1 < x3) wf->rowstart[y][x+2] |= GRID;
					if (y-1 > y2) wf->rowstart[y-2][x] |= GRID;
					if (y+1 < y3) wf->rowstart[y+2][x] |= GRID;
					if (x > x2 && y > y2) wf->rowstart[y-1][x-1] |= GRID;
					if (x > x2 && y < y3) wf->rowstart[y+1][x-1] |= GRID;
					if (x < x3 && y > y2) wf->rowstart[y-1][x+1] |= GRID;
					if (x < x3 && y < y3) wf->rowstart[y+1][x+1] |= GRID;
				}
			}
		}
	}

	/* copy it back to the screen */
	gra_setrect(wf, x2, x3, wf->revy-y3, wf->revy-y2);
}

BOOLEAN DiaNullDlogList(CHAR **c) { Q_UNUSED( c ); return(FALSE); }

CHAR *DiaNullDlogItem(void) { return(0); }

void DiaNullDlogDone(void) {}

#ifndef USEQT

/******* C++ Dialog API: EDialog class on non-QT platforms *************/

class EDialogPrivate
{
public:
	EDialogPrivate( EDialog *dia );
	~EDialogPrivate();
	static EDialogPrivate *find( void *vdia );
	static void itemHitActionCaller(void *vdia, INTBIG item);
	static void redispCaller(RECTAREA *rect, void *vdia);

	EDialog *dia;
    BOOLEAN modal;
	void *vdia;
	void (*redisp)(RECTAREA *rect, void *dia);
	EDialogPrivate *nextActive;
	static EDialogPrivate *firstActive;
};

EDialogPrivate *EDialogPrivate::firstActive = 0;

EDialogPrivate::EDialogPrivate(EDialog *dia )
	: dia(dia), vdia(0), redisp(0)
{
	nextActive = firstActive;
	firstActive = this;
}

EDialogPrivate::~EDialogPrivate()
{
	EDialogPrivate **p = &firstActive;

	/* remove from active list */
	for (p = &firstActive; *p && *p != this; p = &(*p)->nextActive);
	if (*p) *p = nextActive;
	if (vdia) DiaDoneDialog(vdia);
}

EDialogPrivate *EDialogPrivate::find(void *vdia)
{
	EDialogPrivate *p;
	for (p = firstActive; p != 0 && p->vdia != vdia; p = p->nextActive);
	return p;
}

void DiaCloseAllModeless()
{
    EDialogPrivate *p;

    for (p = EDialogPrivate::firstActive; p != 0; p = p->nextActive)
    {
        if (p->modal) continue;
        EDialogModeless *dia = (EDialogModeless*)p->dia;
        dia->hide();
        dia->reset();
    }
}

void EDialogPrivate::itemHitActionCaller(void *vdia, INTBIG item)
{
	EDialogPrivate *p = find(vdia);
	if (!p) return;

	EDialog *dia = p->dia;
	INTBIG itemtypeext = dia->itemDesc()->list[item - 1].type&ITEMTYPEEXT;
	switch(itemtypeext&ITEMTYPE)
	{
		case CHECK:
			if (itemtypeext == AUTOCHECK)
				dia->setControl( item, !dia->getControl( item ) );
			break;
		case RADIO:
			if (itemtypeext != RADIO)
			{
				DIALOG *dialog = dia->itemDesc();
				for (int i = 0; i < dialog->items; i++)
					if ((dialog->list[i].type&ITEMTYPEEXT) == itemtypeext)
				{
					dia->setControl( i + 1, (i + 1) == item );
				}
			}
			break;
	}
	dia->itemHitAction( item );
}

void EDialogPrivate::redispCaller(RECTAREA *rect, void *vdia)
{
	EDialogPrivate *p = find(vdia);
	if (!p) return;
	(*p->redisp)( rect, p->dia );
}

EDialog::EDialog( DIALOG *dialog )
	: itemdesc( dialog ), isExtended( FALSE )
{
	d = new EDialogPrivate( this );
	if (!d) ttyputnomemory();
}

EDialog::~EDialog()
{
    /* if (in_loop) ttyputerr(_("Dialog shuuld be closed by accept() or reject()")); */
    delete d;
}

void EDialog::reset()
{
}

INTBIG EDialog::nextHit(void)
{
    ttyputerr(_("nextHit() called in class dialog"));
    return 0;
}

void EDialog::itemHitAction(INTBIG itemHit)
{
    ttyputerr(_("EDialog::itemHisAction should be redefined"));
}

void EDialog::showExtension( BOOLEAN showIt )
{
    if (itemdesc->briefHeight == 0) return;
    if (isExtended == showIt) return;
    isExtended = showIt;
    RECTAREA& r = itemdesc->windowRect;
    if (d->vdia)
        DiaResizeDialog( d->vdia, r.right - r.left, (isExtended ? r.bottom  - r.top : itemdesc->briefHeight ) );
}

BOOLEAN EDialog::extension()
{
    return isExtended;
}

void EDialog::initTextDialog(INTBIG item, BOOLEAN (*toplist)(CHAR **), CHAR *(*nextinlist)(void),
			void (*donelist)(void), INTBIG sortpos, INTBIG flags)
{
    DiaInitTextDialog( d->vdia, item, toplist, nextinlist, donelist, sortpos, flags);
}

void EDialog::loadTextDialog(INTBIG item, BOOLEAN (*toplist)(CHAR **), CHAR *(*nextinlist)(void),
			void (*donelist)(void), INTBIG sortpos)
{
    DiaLoadTextDialog( d->vdia, item, toplist, nextinlist, donelist, sortpos );
}

void EDialog::redispRoutine(INTBIG item, void (*routine)(RECTAREA *rect, void *dia))
{
    d->redisp = routine;
    DiaRedispRoutine ( d->vdia, item, EDialogPrivate::redispCaller );
}

void EDialog::setControl(INTBIG item, INTBIG value)
{
    INTBIG itemtypeext = itemdesc->list[item - 1].type&ITEMTYPEEXT;
    /* emulate grouped radio-buttones */
    if ((itemtypeext&ITEMTYPE) == RADIO && itemtypeext != RADIO && value != 0)
    {
        for (int i = 0; i < itemdesc->items; i++)
        {
            if ((itemdesc->list[i].type&ITEMTYPEEXT) == itemtypeext && i != item - 1)
                DiaSetControl( d->vdia, item, 0 );
        }
    }
    DiaSetControl( d->vdia, item, value );
}

void EDialog::setText(INTBIG item, CHAR *msg)
{
    if (d->vdia)
        DiaSetText( d->vdia, item, msg );
}

void EDialog::resizeDialog(INTBIG wid, INTBIG hei) { DiaResizeDialog( d->vdia, wid, hei ); }
void EDialog::bringToTop() { DiaBringToTop(d->vdia); };
CHAR *EDialog::getText(INTBIG item) { return DiaGetText( d->vdia, item ); }
INTBIG EDialog::getControl(INTBIG item) { return DiaGetControl( d->vdia, item ); }
BOOLEAN EDialog::validEntry(INTBIG item) { return DiaValidEntry( d->vdia, item ); }
void EDialog::dimItem(INTBIG item) { DiaDimItem( d->vdia, item ); }
void EDialog::unDimItem(INTBIG item) { DiaUnDimItem( d->vdia, item ); }
void EDialog::noEditControl(INTBIG item) { DiaNoEditControl( d->vdia, item ); }
void EDialog::editControl(INTBIG item) { DiaEditControl( d->vdia, item ); }
void EDialog::opaqueEdit(INTBIG item) { DiaOpaqueEdit( d->vdia, item ); }
void EDialog::defaultButton(INTBIG item) { DiaDefaultButton( d->vdia, item ); }
void EDialog::changeIcon(INTBIG item, UCHAR1 *icon) { DiaChangeIcon( d->vdia, item, icon ); }
void EDialog::setPopup(INTBIG item, INTBIG count, CHAR **names) { DiaSetPopup( d->vdia, item, count, names ); }
void EDialog::setPopupEntry(INTBIG item, INTBIG entry) { DiaSetPopupEntry( d->vdia, item, entry ); }
INTBIG EDialog::getPopupEntry(INTBIG item) { return DiaGetPopupEntry( d->vdia, item ); }
void EDialog::stuffLine(INTBIG item, CHAR *line) { DiaStuffLine( d->vdia, item, line ); }
void EDialog::selectLine(INTBIG item, INTBIG line) { DiaSelectLine( d->vdia, item, line); }
void EDialog::selectLines(INTBIG item, INTBIG count, INTBIG *lines) { DiaSelectLines( d->vdia, item, count, lines );}
INTBIG EDialog::getCurLine(INTBIG item) { return DiaGetCurLine( d->vdia, item ); }
INTBIG *EDialog::getCurLines(INTBIG item) { return DiaGetCurLines( d->vdia, item ); }
INTBIG EDialog::getNumScrollLines(INTBIG item) { return DiaGetNumScrollLines( d->vdia, item ); }
CHAR *EDialog::getScrollLine(INTBIG item, INTBIG line) { return DiaGetScrollLine( d->vdia, item, line ); }
void EDialog::setScrollLine(INTBIG item, INTBIG line, CHAR *msg) { DiaSetScrollLine( d->vdia, item, line, msg ); }
void EDialog::synchVScrolls(INTBIG item1, INTBIG item2, INTBIG item3) { DiaSynchVScrolls( d->vdia, item1, item2, item3 ); }
void EDialog::unSynchVScrolls() { DiaUnSynchVScrolls( d->vdia ); }
void EDialog::itemRect(INTBIG item, RECTAREA *rect) { DiaItemRect( d->vdia, item, rect ); }
void EDialog::percent(INTBIG item, INTBIG percent) { DiaPercent( d->vdia, item, percent ); }
void EDialog::allowUserDoubleClick(void) { DiaAllowUserDoubleClick( d->vdia ); }
void EDialog::fillPoly(INTBIG item, INTBIG *xv, INTBIG *yv, INTBIG count, INTBIG r, INTBIG g, INTBIG b) { DiaFillPoly( d->vdia, item, xv, yv, count, r, g, b ); }
void EDialog::drawRect(INTBIG item, RECTAREA *rect, INTBIG r, INTBIG g, INTBIG b) { DiaDrawRect( d->vdia, item, rect, r, g, b ); }
void EDialog::frameRect(INTBIG item, RECTAREA *r) { DiaFrameRect( d->vdia, item, r ); }
void EDialog::invertRect(INTBIG item, RECTAREA *r) { DiaInvertRect( d->vdia, item, r); }
void EDialog::drawLine(INTBIG item, INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty, INTBIG mode) { DiaDrawLine( d->vdia, item, fx, fy, tx, ty, mode ); }
void EDialog::setTextSize(INTBIG size) { DiaSetTextSize( d->vdia, size ); }
void EDialog::getTextInfo(CHAR *msg, INTBIG *wid, INTBIG *hei) { DiaGetTextInfo( d->vdia, msg, wid, hei ); }
void EDialog::putText(INTBIG item, CHAR *msg, INTBIG x, INTBIG y) { DiaPutText( d->vdia, item, msg, x, y ); }
void EDialog::trackCursor(void (*eachdown)(INTBIG x, INTBIG y)) { DiaTrackCursor( d->vdia, eachdown ); }
void EDialog::getMouse(INTBIG *x, INTBIG *y) { DiaGetMouse( d->vdia, x, y ); }

EDialogModal::EDialogModal( DIALOG *dialog )
    : EDialog( dialog ), in_loop(FALSE), result(Rejected)
{
    d->modal = TRUE;
    d->vdia = DiaInitDialog( dialog );
    if ( !d->vdia ) ttyputerr(_("Creation of dialog %s failed"), dialog->movable );
}

EDialogModal::DialogCode EDialogModal::exec()
{
    result = Rejected;
    for (in_loop = TRUE; in_loop;)
    {
		INTBIG itemHit = DiaNextHit( d->vdia );
        EDialogPrivate::itemHitActionCaller( d->vdia, itemHit );
    }
    return(result);
}

EDialogModeless::EDialogModeless( DIALOG *dialog )
    : EDialog( dialog )
{
    d->modal = FALSE;
}

void EDialogModeless::show()
{
    if (d->vdia)
    {
        DiaBringToTop( d->vdia );
        return;
    }
    if (itemDesc()->briefHeight != 0 && !extension())
    {
        INTBIG saveBottom = itemDesc()->windowRect.bottom;
        itemDesc()->windowRect.bottom = (INTSML)(itemDesc()->windowRect.top + itemDesc()->briefHeight);
        d->vdia = DiaInitDialogModeless( itemDesc(), EDialogPrivate::itemHitActionCaller );
        itemDesc()->windowRect.bottom = (INTSML)saveBottom;
    } else d->vdia = DiaInitDialogModeless( itemDesc(), EDialogPrivate::itemHitActionCaller );
    if ( !d->vdia ) ttyputerr(_("Creation of dialog %s failed"), itemDesc()->movable );
}

void EDialogModeless::hide()
{
    if (!d->vdia) return;
    DiaDoneDialog( d->vdia );
    d->vdia = 0;
}

BOOLEAN EDialogModeless::isHidden()
{
    return(d->vdia == 0);
}

/***************** EProcess class on non-QT platforms ***********************/

class EProcessPrivate
{
	friend class EProcess;
public:
	EProcessPrivate();
	INTBIG process;
	CHAR **arguments;
	BOOLEAN _cStdin, _cStdout, _cStderr;
	int ipipe[2];
	int opipe[2];
	UCHAR1 buf[1024];
	UCHAR1 *bufp;
	UINTBIG bufn;
};

EProcessPrivate::EProcessPrivate()
	: _cStdin( TRUE ), _cStdout( TRUE ), _cStderr( TRUE ), bufn( 0 )
{
	arguments = new (CHAR*[1]);
	arguments[0] = 0;
}

EProcess::EProcess()
{
	d = new EProcessPrivate();
}

EProcess::~EProcess()
{
	clearArguments();
	delete[] d->arguments;
	delete d;
}

void EProcess::clearArguments()
{
	for (int i = 0; d->arguments[i] != 0; i++)
		delete[] d->arguments[i];
	delete[] d->arguments;
	d->arguments = new (CHAR*[1]);
	d->arguments[0] = 0;
}

void EProcess::addArgument( CHAR *arg )
{
	int i, n;
	CHAR **newArguments;

	n = 0;
	while (d->arguments[n] != 0) n++;
	newArguments = new (CHAR*[n + 2]);

	for (i = 0; i < n; i++)
		newArguments[i] = d->arguments[i];
	newArguments[n] = new CHAR[estrlen(arg) + 1];
	estrcpy(newArguments[n], arg);
	newArguments[n + 1] = 0;
	delete[] d->arguments;
	d->arguments = newArguments;
}

void EProcess::setCommunication( BOOLEAN cStdin, BOOLEAN cStdout, BOOLEAN cStderr )
{
	d->_cStdin = cStdin;
	d->_cStdout = cStdout;
	d->_cStderr = cStderr;
}

BOOLEAN EProcess::start( CHAR *infile )
{
	d->bufn = 0;
	if ( d->_cStdin ) (void)epipe(d->ipipe);
	if ( d->_cStdout || d->_cStderr ) (void)epipe(d->opipe);

	d->process = efork();
	if (d->process == 1)
	{
		ttyputmsg(_("Run process directly please"));
		return FALSE;
	}
	if (d->process == 0)
	{
		REGISTER INTBIG save0, save1, save2;

		if (d->_cStdin ) save0 = channelreplacewithchannel(0, d->ipipe[0]);
		else if (infile != 0) save0 = channelreplacewithfile(0, infile);
		if ( d->_cStdout ) save1 = channelreplacewithchannel(1, d->opipe[1]);
		if ( d->_cStderr ) save2 = channelreplacewithchannel(2, d->opipe[1]);
		eexec( d->arguments[0], d->arguments );

		if ( d->_cStdin || infile != 0) channelrestore(0, save0);
		if ( d->_cStdout ) channelrestore(1, save1);
		if ( d->_cStderr ) channelrestore(2, save2);
		ttyputerr(_("Cannot run %s"), d->arguments[0]);
		exit(1);
	}

	/* close the file descriptor so the pipe will terminate when done */
	if ( d->_cStdin) eclose(d->ipipe[0]);
	if ( d->_cStdout || d->_cStderr) eclose(d->opipe[1]);

	return TRUE;
}

void EProcess::wait()
{
	ewait( d->process );
}

void EProcess::kill()
{
	if ( d->_cStdout || d->_cStderr) eclose(d->opipe[0]);
	if (ekill(d->process) == 0)
	{
		ewait(d->process);
		if (errno > 0)
			ttyputmsg(_("%s execution reports error %d"), d->arguments[0], errno);
		else ttyputmsg(_("%s execution terminated"), d->arguments[0]);
	}
}

INTSML EProcess::getChar()
{
	if (d->bufn <= 0)
	{
		d->bufn = eread(d->opipe[0], d->buf, sizeof(d->buf)/sizeof(d->buf[0]));
		d->bufp=d->buf;
	}
	if (d->bufn <= 0) return EOF;
	d->bufn--;
	return *d->bufp++;
}

void EProcess::putChar( UCHAR1 ch )
{
	ewrite(d->ipipe[1], &ch, 1 );
}

#endif /* USEQT */


