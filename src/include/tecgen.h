/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tecgen.h
 * Generic technology description: header file
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

extern TECHNOLOGY *gen_tech;				/* the technology */

extern NODEPROTO  *gen_univpinprim;			/* Universal Pin */
extern NODEPROTO  *gen_invispinprim;		/* Invisible Pin */
extern NODEPROTO  *gen_unroutedpinprim;		/* Unrouted Pin */
extern NODEPROTO  *gen_cellcenterprim;		/* Cell Center */
extern NODEPROTO  *gen_portprim;			/* Port */
extern NODEPROTO  *gen_drcprim;				/* DRC Node */
extern NODEPROTO  *gen_essentialprim;		/* Essential area marker */
extern NODEPROTO  *gen_simprobeprim;		/* Simulation probe */

extern ARCPROTO   *gen_universalarc;		/* universal arc */
extern ARCPROTO   *gen_invisiblearc;		/* invisible arc */
extern ARCPROTO   *gen_unroutedarc;			/* unrouted arc */

/* prototypes for interface routines */
BOOLEAN gen_initprocess(TECHNOLOGY*, INTBIG);
void    gen_termprocess(void);
INTBIG  gen_nodepolys(NODEINST*, INTBIG*, WINDOWPART*);
void    gen_shapenodepoly(NODEINST*, INTBIG, POLYGON*);
INTBIG  gen_allnodepolys(NODEINST*, POLYLIST*, WINDOWPART*, BOOLEAN);
void    gen_shapeportpoly(NODEINST*, PORTPROTO*, POLYGON*, XARRAY, BOOLEAN);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
