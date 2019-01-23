/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tecart.h
 * Artwork technology header
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

#define ELLIPSEPOINTS         30		/* number of lines in an ellipse */

/* the state bits */
#define ARTWORKFILLARROWHEADS  1		/* set to fill arrow heads */

/* all of the artwork primitives */
extern TECHNOLOGY *art_tech;						/* the technology */

extern NODEPROTO  *art_pinprim;						/* Pin */
extern NODEPROTO  *art_boxprim;						/* Box */
extern NODEPROTO  *art_crossedboxprim;				/* Crossed-Box */
extern NODEPROTO  *art_filledboxprim;				/* Filled-Box */
extern NODEPROTO  *art_circleprim;					/* Circle */
extern NODEPROTO  *art_thickcircleprim;				/* Thick-Circle */
extern NODEPROTO  *art_filledcircleprim;			/* Filled-Circle */
extern NODEPROTO  *art_splineprim;					/* Spline */
extern NODEPROTO  *art_triangleprim;				/* Triangle */
extern NODEPROTO  *art_filledtriangleprim;			/* Filled-Triangle */
extern NODEPROTO  *art_arrowprim;					/* Arrow */
extern NODEPROTO  *art_openedpolygonprim;			/* Opened-Polygon */
extern NODEPROTO  *art_openeddottedpolygonprim;		/* Opened-Dotted-Polygon */
extern NODEPROTO  *art_openeddashedpolygonprim;		/* Opened-Dashed-Polygon */
extern NODEPROTO  *art_openedthickerpolygonprim;	/* Opened-Thicker-Polygon */
extern NODEPROTO  *art_closedpolygonprim;			/* Closed-Polygon */
extern NODEPROTO  *art_filledpolygonprim;			/* Filled-Polygon */

extern ARCPROTO   *art_solidarc;					/* solid arc */
extern ARCPROTO   *art_dottedarc;					/* dotted arc */
extern ARCPROTO   *art_dashedarc;					/* dashed arc */
extern ARCPROTO   *art_thickerarc;					/* thicker arc */

extern INTBIG      art_messagekey;					/* key for "ART_message" */
extern INTBIG      art_colorkey;					/* key for "ART_color" */
extern INTBIG      art_degreeskey;					/* key for "ART_degrees" */
extern INTBIG      art_patternkey;					/* key for "ART_pattern" */

/* prototypes for technology routines */
BOOLEAN art_initprocess(TECHNOLOGY*, INTBIG);
void    art_setmode(INTBIG, CHAR*[]);
INTBIG  art_request(CHAR*, va_list);
INTBIG  art_nodepolys(NODEINST*, INTBIG*, WINDOWPART*);
void    art_shapenodepoly(NODEINST*, INTBIG, POLYGON*);
INTBIG  art_allnodepolys(NODEINST*, POLYLIST*, WINDOWPART*, BOOLEAN);
INTBIG  art_nodeEpolys(NODEINST*, INTBIG*, WINDOWPART*);
void    art_shapeEnodepoly(NODEINST*, INTBIG, POLYGON*);
INTBIG  art_allnodeEpolys(NODEINST *ni, POLYLIST *plist, WINDOWPART *win, BOOLEAN onlyreasonable);
void    art_shapeportpoly(NODEINST*, PORTPROTO*, POLYGON*, XARRAY, BOOLEAN);
INTBIG  art_arcpolys(ARCINST*, WINDOWPART*);
void    art_shapearcpoly(ARCINST*, INTBIG, POLYGON*);
INTBIG  art_allarcpolys(ARCINST*, POLYLIST*, WINDOWPART*);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
