/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dbcontour.h
 * Header file for gathering contours
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

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
extern "C"
{
#endif

typedef enum {
	LINESEGMENTTYPE,
	ARCSEGMENTTYPE, REVARCSEGMENTTYPE,
	CIRCLESEGMENTTYPE,
	BRIDGESEGMENTTYPE
} CONTOURELEMENTTYPE;

#define NOCONTOURELEMENT ((CONTOURELEMENT *)-1)

typedef struct Icontourelement
{
	CONTOURELEMENTTYPE      elementtype;		/* type of segment */
	INTBIG                  sx, sy;				/* starting coordinate (line, arc, circle) */
	INTBIG                  ex, ey;				/* ending coordinate (line, arc) */
	INTBIG                  cx, cy;				/* coordinate of arc center (arc, circle) */
	NODEINST               *ni;					/* node on which this geometry resides */
	INTBIG                  userdata;			/* place for additional information */
	struct Icontourelement *nextcontourelement;	/* next in linked list */
} CONTOURELEMENT;

#define NOCONTOUR ((CONTOUR *)-1)

typedef struct Icontour
{
	CONTOURELEMENT      *firstcontourelement;
	CONTOURELEMENT      *lastcontourelement;
	INTBIG               lx, hx, ly, hy;
	INTSML               valid;					/* zero if not closed */
	INTSML               depth;
	INTSML               childtotal;
	INTSML               childcount;
	INTBIG               userdata;
	struct Icontour    **children;
	struct Icontour     *nextcontour;
} CONTOUR;

CONTOUR *gathercontours(NODEPROTO *np, NODEINST **usearray, INTBIG bestthresh, INTBIG worstthresh);
void getcontourbbox(CONTOUR *con, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy);
void getcontourelementbbox(CONTOURELEMENT *conel, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy);
void killcontour(CONTOUR *con);
void getcontoursegmentparameters(INTBIG *arcres, INTBIG *arcsag);
void setcontoursegmentparameters(INTBIG arcres, INTBIG arcsag);
void initcontoursegmentgeneration(CONTOURELEMENT *conel);
BOOLEAN nextcontoursegmentgeneration(INTBIG *x1, INTBIG *y1, INTBIG *x2, INTBIG *y2);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
