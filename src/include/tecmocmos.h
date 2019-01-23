/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tecmocmos.h
 * MOSIS CMOS technology header
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

#define MOCMOSNOSTACKEDVIAS    01		/* set if no stacked vias allowed */
#define MOCMOSSTICKFIGURE      02		/* set for stick-figure display */
#define MOCMOSMETALS          034		/* number of metal layers */
#define MOCMOS2METAL            0		/* 2-metal rules */
#define MOCMOS3METAL           04		/* 3-metal rules */
#define MOCMOS4METAL          010		/* 4-metal rules */
#define MOCMOS5METAL          014		/* 5-metal rules */
#define MOCMOS6METAL          020		/* 6-metal rules */
#define MOCMOSRULESET        0140		/* type of rules */
#define MOCMOSSUBMRULES         0		/*   set if submicron rules in use */
#define MOCMOSDEEPRULES       040		/*   set if deep rules in use */
#define MOCMOSSCMOSRULES     0100		/*   set if standard SCMOS rules in use */
#define MOCMOSALTAPRULES     0200		/* set to use alternate active/poly rules */
#define MOCMOSTWOPOLY        0400		/* set to use second polysilicon layer */
#define MOCMOSSPECIALTRAN   01000		/* set to show special transistors */

extern TECHNOLOGY *mocmos_tech;			/* the technology */
extern NODEPROTO  *mocmos_metal1poly2prim;
extern NODEPROTO  *mocmos_metal1poly12prim;
extern NODEPROTO  *mocmos_metal1metal2prim;
extern NODEPROTO  *mocmos_metal4metal5prim;
extern NODEPROTO  *mocmos_metal5metal6prim;
extern NODEPROTO  *mocmos_ptransistorprim;
extern NODEPROTO  *mocmos_ntransistorprim;
extern NODEPROTO  *mocmos_metal1pwellprim;
extern NODEPROTO  *mocmos_metal1nwellprim;
extern NODEPROTO  *mocmos_scalablentransprim;
extern NODEPROTO  *mocmos_scalableptransprim;
extern INTBIG      mocmos_transcontactkey;

/* prototypes for local routines */
BOOLEAN mocmos_initprocess(TECHNOLOGY*, INTBIG);
void    mocmos_setmode(INTBIG, CHAR*[]);
INTBIG  mocmos_request(CHAR*, va_list);
INTBIG  mocmos_nodepolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win);
void    mocmos_shapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly);
INTBIG  mocmos_allnodepolys(NODEINST *ni, POLYLIST *plist, WINDOWPART *win, BOOLEAN onlyreasonable);
INTBIG  mocmos_nodeEpolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win);
void    mocmos_shapeEnodepoly(NODEINST *ni, INTBIG box, POLYGON *poly);
INTBIG  mocmos_allnodeEpolys(NODEINST *ni, POLYLIST *plist, WINDOWPART *win, BOOLEAN onlyreasonable);
void    mocmos_shapeportpoly(NODEINST *ni, PORTPROTO *pp, POLYGON *poly, XARRAY trans, BOOLEAN purpose);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
