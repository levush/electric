/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: projecttool.h
 * Project management tool
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

void proj_init(INTBIG*, CHAR1*[], TOOL*);
void proj_done(void);
void proj_set(INTBIG, CHAR*[]);
void proj_slice(void);
void proj_startbatch(TOOL*, BOOLEAN);
void proj_modifynodeinst(NODEINST*,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG);
void proj_modifyarcinst(ARCINST*,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG);
void proj_modifyportproto(PORTPROTO*, NODEINST*, PORTPROTO*);
void proj_modifydescript(INTBIG, INTBIG, INTBIG, UINTBIG*);
void proj_newobject(INTBIG, INTBIG);
void proj_killobject(INTBIG, INTBIG);
void proj_newvariable(INTBIG, INTBIG, INTBIG, INTBIG);
void proj_killvariable(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, UINTBIG*);
void proj_modifyvariable(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void proj_insertvariable(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void proj_deletevariable(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void proj_readlibrary(LIBRARY*);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
};
#endif
