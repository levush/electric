/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: logeffort.h
 * Logical effort timing and sizing tool header file
 * Written by: Steven M. Rubin
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

#define DISPLAYCAPACITANCE    01
#define HIGHLIGHTCOMPONENTS   02
#define LEUSELESETTINGS		  04

#define DEFAULTSTATE        (DISPLAYCAPACITANCE | LEUSELESETTINGS)

#define DEFWIRERATIO   10				/* default wire ratio */

extern TOOL    *le_tool;				/* the Logical Effort tool object */
extern INTBIG   le_wire_ratio_key;		/* variable key for "LE_wire_ratio" */

void le_init(INTBIG*, CHAR1*[], TOOL*);
void le_done(void);
void le_set(INTBIG, CHAR*[]);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
