/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usreditemacs.h
 * User interface tool: EMACS-like text window headers
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

WINDOWPART *us_editemacsmakeeditor(WINDOWPART*, CHAR*, INTBIG*, INTBIG*);
void us_editemacsterminate(EDITOR*);
INTBIG us_editemacstotallines(WINDOWPART*);
CHAR *us_editemacsgetline(WINDOWPART*, INTBIG);
void us_editemacsaddline(WINDOWPART*, INTBIG, CHAR*);
void us_editemacsreplaceline(WINDOWPART*, INTBIG, CHAR*);
void us_editemacsdeleteline(WINDOWPART*, INTBIG);
void us_editemacshighlightline(WINDOWPART*, INTBIG, INTBIG);
void us_editemacssuspendgraphics(WINDOWPART*);
void us_editemacsresumegraphics(WINDOWPART*);
void us_editemacswritetextfile(WINDOWPART*, CHAR*);
void us_editemacsreadtextfile(WINDOWPART*, CHAR*);
void us_editemacseditorterm(WINDOWPART*);
void us_editemacsshipchanges(WINDOWPART*);
BOOLEAN us_editemacsgotchar(WINDOWPART*, INTSML, INTBIG);
void us_editemacscut(WINDOWPART*);
void us_editemacscopy(WINDOWPART*);
void us_editemacspaste(WINDOWPART*);
void us_editemacsundo(WINDOWPART*);
void us_editemacssearch(WINDOWPART*, CHAR*, CHAR*, INTBIG);
void us_editemacspan(WINDOWPART*, INTBIG, INTBIG);
