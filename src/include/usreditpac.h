/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usreditpac.h
 * Point-and-click editor headers
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

WINDOWPART *us_editpacmakeeditor(WINDOWPART*, CHAR*, INTBIG*, INTBIG*);
void us_editpacterminate(EDITOR*);
INTBIG us_editpactotallines(WINDOWPART*);
CHAR *us_editpacgetline(WINDOWPART*, INTBIG);
void us_editpacaddline(WINDOWPART*, INTBIG, CHAR*);
void us_editpacreplaceline(WINDOWPART*, INTBIG, CHAR*);
void us_editpacdeleteline(WINDOWPART*, INTBIG);
void us_editpachighlightline(WINDOWPART*, INTBIG, INTBIG);
void us_editpacsuspendgraphics(WINDOWPART*);
void us_editpacresumegraphics(WINDOWPART*);
void us_editpacwritetextfile(WINDOWPART*, CHAR*);
void us_editpacreadtextfile(WINDOWPART*, CHAR*);
void us_editpaceditorterm(WINDOWPART*);
void us_editpacshipchanges(WINDOWPART*);
BOOLEAN us_editpacgotchar(WINDOWPART*, INTSML, INTBIG);
void us_editpaccut(WINDOWPART*);
void us_editpaccopy(WINDOWPART*);
void us_editpacpaste(WINDOWPART*);
void us_editpacundo(WINDOWPART*);
void us_editpacsearch(WINDOWPART*, CHAR*, CHAR*, INTBIG);
void us_editpacpan(WINDOWPART*, INTBIG, INTBIG);
