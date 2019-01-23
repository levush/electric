/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tecmocmossub.h
 * MOSIS CMOS Submicron technology header
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

#define MOCMOSSUBMETALS       034	/* number of metal layers */
#define MOCMOSSUB2METAL         0	/* 2-metal rules */
#define MOCMOSSUB3METAL        04	/* 3-metal rules */
#define MOCMOSSUB4METAL       010	/* 4-metal rules */
#define MOCMOSSUB5METAL       014	/* 5-metal rules */
#define MOCMOSSUB6METAL       020	/* 6-metal rules */
#define MOCMOSSUBNOCONV       040	/* don't convert to new "mocmos" technology */

extern TECHNOLOGY *mocmossub_tech;			/* the technology */

/* prototypes for local routines */
BOOLEAN mocmossub_initprocess(TECHNOLOGY*, INTBIG);
void    mocmossub_setmode(INTBIG, CHAR*[]);
INTBIG  mocmossub_request(CHAR*, va_list);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
