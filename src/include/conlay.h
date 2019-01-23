/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: conlay.h
 * Hierarchical layout constraint system
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

#define CHANGETYPERIGID         0		/* make arc rigid */
#define CHANGETYPEUNRIGID       1		/* make arc un-rigid */
#define CHANGETYPEFIXEDANGLE    2		/* make arc fixed-angle */
#define CHANGETYPENOTFIXEDANGLE 3		/* make arc not fixed-angle */
#define CHANGETYPESLIDABLE      4		/* make arc slidable */
#define CHANGETYPENOTSLIDABLE   5		/* make arc nonslidable */
#define CHANGETYPETEMPRIGID     6		/* make arc temporarily rigid */
#define CHANGETYPETEMPUNRIGID   7		/* make arc temporarily un-rigid */
#define CHANGETYPEREMOVETEMP    8		/* remove temporarily state of arc */

extern CONSTRAINT *cla_constraint;		/* the constraint object for this solver */
extern INTBIG      cla_changeclock;		/* timestamp for changes */

void cla_layconinit(CONSTRAINT*);
void cla_layconterm(void);
void cla_layconsetmode(INTBIG, CHAR*[]);
INTBIG cla_layconrequest(CHAR*, INTBIG);
void cla_layconsolve(NODEPROTO*);
void cla_layconnewobject(INTBIG, INTBIG);
void cla_layconkillobject(INTBIG, INTBIG);
BOOLEAN cla_layconsetobject(INTBIG, INTBIG, INTBIG, INTBIG);
void cla_layconmodifynodeinst(NODEINST*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void cla_layconmodifynodeinsts(INTBIG, NODEINST**, INTBIG*, INTBIG*, INTBIG*, INTBIG*, INTBIG*, INTBIG*);
void cla_layconmodifyarcinst(ARCINST*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void cla_layconmodifyportproto(PORTPROTO*, NODEINST*, PORTPROTO*);
void cla_layconmodifynodeproto(NODEPROTO*);
void cla_layconmodifydescript(INTBIG, INTBIG, INTBIG, UINTBIG*);
void cla_layconnewlib(LIBRARY*);
void cla_layconkilllib(LIBRARY*);
void cla_layconnewvariable(INTBIG, INTBIG, INTBIG, INTBIG);
void cla_layconkillvariable(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, UINTBIG*);
void cla_layconmodifyvariable(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void cla_layconinsertvariable(INTBIG, INTBIG, INTBIG, INTBIG);
void cla_laycondeletevariable(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void cla_layconsetvariable(void);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif

extern COMCOMP cla_layconp;
