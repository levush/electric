/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrtrack.h
 * User interface tool: cursor tracking headers
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

void us_arccurveinit(ARCINST *ai);
BOOLEAN us_arccurvedown(INTBIG x, INTBIG y);
BOOLEAN us_arccenterdown(INTBIG x, INTBIG y);
void us_oncommand(INTSML cmd, INTBIG special);
void us_ontablet(INTBIG x, INTBIG y, INTBIG but);
void us_distanceinit(void);
BOOLEAN us_distancedown(INTBIG, INTBIG);
void us_distanceup(void);
void us_textgrabinit(UINTBIG*, INTBIG, INTBIG, INTBIG, INTBIG, GEOM*);
void us_textgrabbegin(void);
BOOLEAN us_textgrabdown(INTBIG, INTBIG);
void us_sizeainit(ARCINST*);
void us_sizeabegin(void);
BOOLEAN us_sizeadown(INTBIG, INTBIG);
void us_sizeinit(NODEINST*);
INTBIG us_sizeterm(void);
BOOLEAN us_sizecdown(INTBIG, INTBIG);
BOOLEAN us_sizedown(INTBIG, INTBIG);
void us_rotateinit(NODEINST*);
void us_rotatebegin(void);
BOOLEAN us_rotatedown(INTBIG, INTBIG);
void us_tracebegin(void);
BOOLEAN us_tracedown(INTBIG, INTBIG);
void us_traceup(void);
void us_pointbegin(void);
BOOLEAN us_addpdown(INTBIG, INTBIG);
void us_createinit(INTBIG, INTBIG, NODEPROTO*, INTBIG, BOOLEAN, GEOM*, PORTPROTO*);
BOOLEAN us_ignoreup(INTBIG, INTBIG);
void us_createbegin(void);
BOOLEAN us_dragdown(INTBIG, INTBIG);
BOOLEAN us_stoponchar(INTBIG, INTBIG, INTSML);
BOOLEAN us_stopandpoponchar(INTBIG, INTBIG, INTSML);
void us_dragup(void);
void us_invertdragup(void);
void us_createabegin(void);
BOOLEAN us_createadown(INTBIG, INTBIG);
void us_createaup(void);
void us_createajoinedobject(GEOM**, PORTPROTO**);
void us_createinsinit(ARCINST*, NODEPROTO*);
void us_createinsbegin(void);
BOOLEAN us_createinsdown(INTBIG, INTBIG);
void us_multidraginit(INTBIG, INTBIG, GEOM**, NODEINST**, INTBIG, INTBIG, BOOLEAN);
void us_multidragbegin(void);
BOOLEAN us_multidragdown(INTBIG, INTBIG);
void us_multidragup(void);
void us_pointinit(NODEINST*, INTBIG);
void us_findpointbegin(void);
BOOLEAN us_movepdown(INTBIG, INTBIG);
void us_findiinit(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void us_findibegin(void);
BOOLEAN us_findidown(INTBIG, INTBIG);
void us_findiup(void);
void us_findcibegin(void);
void us_findinit(INTBIG, INTBIG);
void us_findmbegin(void);
void us_findsbegin(void);
BOOLEAN us_stretchdown(INTBIG, INTBIG);
void us_finddbegin(void);
void us_finddterm(INTBIG*, INTBIG*);
void us_hthumbbegin(INTBIG x, WINDOWPART *w, INTBIG ly, INTBIG lx, INTBIG hx, void (*callback)(INTBIG));
void us_hthumbtrackingcallback(INTBIG delta);
BOOLEAN us_hthumbdown(INTBIG x, INTBIG y);
void us_hthumbdone(void);
void us_vthumbbegin(INTBIG y, WINDOWPART *w, INTBIG hx, INTBIG ly, INTBIG hy,
		BOOLEAN onleft, void (*callback)(INTBIG));
void us_vthumbtrackingcallback(INTBIG delta);
BOOLEAN us_vthumbdown(INTBIG x, INTBIG y);
void us_vthumbdone(void);
void us_hpartdividerbegin(INTBIG y, INTBIG lx, INTBIG hx, WINDOWFRAME *wf);
BOOLEAN us_hpartdividerdown(INTBIG x, INTBIG y);
void us_hpartdividerdone(void);
void us_vpartdividerbegin(INTBIG y, INTBIG lx, INTBIG hx, WINDOWFRAME *wf);
BOOLEAN us_vpartdividerdown(INTBIG x, INTBIG y);
void us_vpartdividerdone(void);
void us_arrowclickbegin(WINDOWPART *w, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy, INTBIG amount);
BOOLEAN us_varrowdown(INTBIG x, INTBIG y);
BOOLEAN us_harrowdown(INTBIG x, INTBIG y);
BOOLEAN us_nullup(INTBIG, INTBIG);
void us_nullvoid(void);
BOOLEAN us_nullchar(INTBIG x, INTBIG y, INTSML ch);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
