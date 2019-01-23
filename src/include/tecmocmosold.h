/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tecmocmosold.h
 * Old MOSIS CMOS technology header
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

/* prototypes for local routines */
BOOLEAN mocmosold_initprocess(TECHNOLOGY*, INTBIG);
void    mocmosold_setmode(INTBIG, CHAR*[]);
INTBIG  mocmosold_arcpolys(ARCINST*, WINDOWPART*);
void    mocmosold_shapearcpoly(ARCINST*, INTBIG, POLYGON*);
INTBIG  mocmosold_allarcpolys(ARCINST*, POLYLIST*, WINDOWPART*);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
