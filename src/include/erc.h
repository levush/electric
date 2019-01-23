/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: erc.h
 * Tool for electrical rules checking
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

/* the meaning of "ERC_options" */
#define PWELLCONOPTIONS   03				/* PWell contact options */
#define PWELLCONPERAREA    0				/*   must be 1 contact in each PWell area */
#define PWELLCONONCE       1				/*   must be 1 contact in any PWell area */
#define PWELLCONIGNORE     2				/*   do not check for contacts in PWell areas */
#define NWELLCONOPTIONS  014				/* NWell contact options */
#define NWELLCONPERAREA    0				/*   must be 1 contact in each NWell area */
#define NWELLCONONCE      04				/*   must be 1 contact in any NWell area */
#define NWELLCONIGNORE   010				/*   do not check for contacts in NWell areas */
#define PWELLONGROUND    020				/* ensure that PWell is connected to ground */
#define NWELLONPOWER     040				/* ensure that NWell is connected to power */
#define FINDEDGEDIST    0100				/* find farthest distance from contact to edge */

extern TOOL       *erc_tool;				/* this tool */
extern INTBIG      erc_antennaarcratiokey;	/* key for "ERC_antenna_arc_ratio" */

void erc_init(INTBIG*, CHAR1*[], TOOL*);
void erc_done(void);
void erc_set(INTBIG, CHAR*[]);

/* the antenna rules interface */
void erc_antcheckcell(NODEPROTO *cell);
void erc_antterm(void);
void erc_antoptionsdlog(void);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
