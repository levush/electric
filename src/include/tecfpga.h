/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tecfpga.h
 * FPGA technology header file
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

/* prototypes for technology routines */
BOOLEAN fpga_initprocess(TECHNOLOGY*, INTBIG);
void    fpga_termprocess(void);
void    fpga_setmode(INTBIG, CHAR*[]);
INTBIG  fpga_nodepolys(NODEINST*, INTBIG*, WINDOWPART*);
void    fpga_shapenodepoly(NODEINST*, INTBIG, POLYGON*);
INTBIG  fpga_allnodepolys(NODEINST*, POLYLIST*, WINDOWPART*, BOOLEAN);
INTBIG  fpga_nodeEpolys(NODEINST*, INTBIG*, WINDOWPART*);
void    fpga_shapeEnodepoly(NODEINST*, INTBIG, POLYGON*);
INTBIG  fpga_allnodeEpolys(NODEINST *ni, POLYLIST *plist, WINDOWPART *win, BOOLEAN onlyreasonable);
void    fpga_shapeportpoly(NODEINST*, PORTPROTO*, POLYGON*, XARRAY, BOOLEAN);
INTBIG  fpga_arcpolys(ARCINST*, WINDOWPART*);
void    fpga_shapearcpoly(ARCINST*, INTBIG, POLYGON*);
INTBIG  fpga_allarcpolys(ARCINST*, POLYLIST*, WINDOWPART*);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
};
#endif
