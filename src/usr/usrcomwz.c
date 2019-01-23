/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrcomtz.c
 * User interface tool: command handler for W through Z
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

#include "global.h"
#include "egraphics.h"
#include "usr.h"
#include "usrtrack.h"
#include "efunction.h"

static struct
{
	CHAR         *keyword;
	short         unique;
	STATUSFIELD **fieldaddr;
} us_statusfields[] =
{
	{x_("align"),      2,  &us_statusalign},
	{x_("angle"),      2,  &us_statusangle},
	{x_("arc"),        2,  &us_statusarc},
	{x_("cell"),       1,  &us_statuscell},
	{x_("grid"),       1,  &us_statusgridsize},
	{x_("lambda"),     1,  &us_statuslambda},
	{x_("numselected"),2,  &us_statusselectcount},
	{x_("node"),       2,  &us_statusnode},
	{x_("package"),    3,  &us_statuspackage},
	{x_("part"),       3,  &us_statuspart},
	{x_("project"),    2,  &us_statusproject},
	{x_("root"),       1,  &us_statusroot},
	{x_("selection"),  2,  &us_statusselection},
	{x_("size"),       2,  &us_statuscellsize},
	{x_("technology"), 1,  &us_statustechnology},
	{x_("x"),          1,  &us_statusxpos},
	{x_("y"),          1,  &us_statusypos},
	{0,0,0}
};

void us_window(INTBIG count, CHAR *par[])
{
	REGISTER WINDOWPART *w, *oldw, *nextw, *neww;
	REGISTER INTBIG i, dist, curwx, curwy, size, x, y, diffx, diffy, lambda, needwx, needwy;
	REGISTER INTBIG l, nogood, splitkey, lineno, startper, endper;
	INTBIG lx, hx, ly, hy, xcur, ycur, windowView[8];
	UINTBIG descript[TEXTDESCRIPTSIZE];
	static POLYGON *poly = NOPOLYGON;
	REGISTER STATUSFIELD *sf, **whichstatus;
	CHAR *newpar[4], *fieldname;
	WINDOWFRAME *wf;
#if SIMTOOL
	extern TOOL *sim_tool;
#endif
	REGISTER CHAR *pp, *win;
	REGISTER VARIABLE *var;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	extern GRAPHICS us_arbit;
	extern COMCOMP us_windowp, us_windowup, us_windowmp;
	REGISTER void *infstr;
	extern GRAPHICS us_hbox;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	if (count == 0)
	{
		count = ttygetparam(M_("Window configuration: "), &us_windowp, MAXPARS, par);
		if (count == 0)
		{
			us_abortedmsg();
			return;
		}
	}
	l = estrlen(pp = par[0]);

	if (namesamen(pp, x_("adjust"), l) == 0 && l >= 2)
	{
		if (count == 1)
		{
			ttyputusage(x_("window adjust STYLE"));
			return;
		}
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("horizontal-tile"), l) == 0)
		{
			adjustwindowframe(0);
			return;
		}
		if (namesamen(pp, x_("vertical-tile"), l) == 0)
		{
			adjustwindowframe(1);
			return;
		}
		if (namesamen(pp, x_("cascade"), l) == 0)
		{
			adjustwindowframe(2);
			return;
		}
		ttyputusage(x_("window adjust (horizontal-tile | vertical-tile | cascade)"));
		return;
	}

	if (namesamen(pp, x_("all-displayed"), l) == 0 && l >= 2)
	{
		if (us_needwindow()) return;

#if SIMTOOL
		/* special case for waveform window */
		if ((el_curwindowpart->state&WINDOWTYPE) == WAVEFORMWINDOW)
		{
			newpar[0] = x_("window");
			newpar[1] = x_("zoom");
			newpar[2] = x_("all-displayed");
			telltool(sim_tool, 3, newpar);
			return;
		}
#endif

		/* fill a 3D window */
		if ((el_curwindowpart->state&WINDOWTYPE) == DISP3DWINDOW)
		{
			us_3dfillview(el_curwindowpart);
			return;
		}

		/* use direct methods on nonstandard windows */
		if ((el_curwindowpart->state&WINDOWTYPE) != DISPWINDOW)
		{
			if (el_curwindowpart->redisphandler != 0)
				(*el_curwindowpart->redisphandler)(el_curwindowpart);
			return;
		}

		np = us_needcell();
		if (np == NONODEPROTO) return;

		/* save and erase highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* make the cell fill the window */
		us_fullview(np, &lx, &hx, &ly, &hy);
		if ((el_curwindowpart->state&INPLACEEDIT) != 0)
			xformbox(&lx, &hx, &ly, &hy, el_curwindowpart->outofcell);
		us_squarescreen(el_curwindowpart, NOWINDOWPART, FALSE, &lx, &hx, &ly, &hy, 0);

		startobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenlx"), lx, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenhx"), hx, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenly"), ly, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenhy"), hy, VINTEGER);
		us_gridset(el_curwindowpart, el_curwindowpart->state);
		endobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);

		/* restore highlighting */
		us_pophighlight(FALSE);
		return;
	}

	if (namesamen(pp, x_("center-highlight"), l) == 0 && l >= 2)
	{
		if (us_needwindow()) return;

		np = us_getareabounds(&lx, &hx, &ly, &hy);
		if (np == NONODEPROTO)
		{
			us_abortcommand(_("Outline an area"));
			return;
		}

		if (el_curwindowpart->curnodeproto == np) w = el_curwindowpart; else
		{
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
				if (w->curnodeproto == np && (w->state&WINDOWTYPE) == DISPWINDOW)
					break;
			if (w == NOWINDOWPART)
			{
				us_abortcommand(_("Cannot find an editing window with highlighted objects"));
				return;
			}
		}

		/* cannot manipulate a nonstandard window */
		if ((w->state&WINDOWTYPE) != DISPWINDOW)
		{
			us_abortcommand(_("Can only pan circuit editing windows"));
			return;
		}

		/* pre-compute current window size */
		curwx = w->screenhx - w->screenlx;
		curwy = w->screenhy - w->screenly;

		/* center about this area without re-scaling */
		x = (hx + lx) / 2;     y = (hy + ly) / 2;
		lx = x - curwx/2;      ly = y - curwy/2;
		hx = lx + curwx;       hy = ly + curwy;
		if ((el_curwindowpart->state&INPLACEEDIT) != 0)
			xformbox(&lx, &hx, &ly, &hy, el_curwindowpart->outofcell);
		us_squarescreen(el_curwindowpart, NOWINDOWPART, FALSE, &lx, &hx, &ly, &hy, 1);

		/* save and erase highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		startobjectchange((INTBIG)w, VWINDOWPART);
		(void)setval((INTBIG)w, VWINDOWPART, x_("screenlx"), lx, VINTEGER);
		(void)setval((INTBIG)w, VWINDOWPART, x_("screenhx"), hx, VINTEGER);
		(void)setval((INTBIG)w, VWINDOWPART, x_("screenly"), ly, VINTEGER);
		(void)setval((INTBIG)w, VWINDOWPART, x_("screenhy"), hy, VINTEGER);
		us_gridset(w, el_curwindowpart->state);
		endobjectchange((INTBIG)w, VWINDOWPART);

		/* restore highlighting */
		us_pophighlight(FALSE);
		return;
	}

	if (namesamen(pp, x_("cursor-centered"), l) == 0 && l >= 2)
	{
#if SIMTOOL
		if ((el_curwindowpart->state&WINDOWTYPE) == WAVEFORMWINDOW)
		{
			if (us_needwindow()) return;
			newpar[0] = x_("window");
			newpar[1] = x_("cursor");
			newpar[2] = x_("center");
			telltool(sim_tool, 3, newpar);
			return;
		}
#endif

		np = us_needcell();
		if (np == NONODEPROTO) return;

		/* cannot manipulate a nonstandard window */
		if ((el_curwindowpart->state&WINDOWTYPE) != DISPWINDOW)
		{
			us_abortcommand(_("Can only pan circuit editing windows"));
			return;
		}

		if (us_demandxy(&xcur, &ycur)) return;

		/* pre-compute current window size */
		curwx = el_curwindowpart->screenhx - el_curwindowpart->screenlx;
		curwy = el_curwindowpart->screenhy - el_curwindowpart->screenly;
		lx = xcur - curwx/2;   ly = ycur - curwy/2;
		hx = lx + curwx;       hy = ly + curwy;
		if ((el_curwindowpart->state&INPLACEEDIT) != 0)
			xformbox(&lx, &hx, &ly, &hy, el_curwindowpart->outofcell);
		us_squarescreen(el_curwindowpart, NOWINDOWPART, FALSE, &lx, &hx, &ly, &hy, 1);

		/* save and erase highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		startobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenlx"), lx, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenhx"), hx, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenly"), ly, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenhy"), hy, VINTEGER);
		us_gridset(el_curwindowpart, el_curwindowpart->state);
		endobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);

		/* restore highlighting */
		us_pophighlight(FALSE);
		return;
	}

	/* handle deletion of separate windows */
	if (namesamen(pp, x_("delete"), l) == 0 && l >= 2)
	{
		/* close the messages window if in front and can be closed */
		if (closefrontmostmessages()) return;

		/* delete split if there are no multiple window frames */
		if (!graphicshas(CANUSEFRAMES))
		{
			if (us_needwindow()) return;
			us_killcurrentwindow(TRUE);
			return;
		}

		/* disallow if this is the last window and it must remain */
		if (!graphicshas(CANHAVENOWINDOWS))
		{
			/* disallow deletion if this is the last window */
			i = 0;
			for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
				if (wf->floating == 0) i++;
			if (i <= 1)
			{
				ttyputerr(_("Sorry, cannot delete the last window"));
				return;
			}
		}

		/* get the current frame */
		wf = getwindowframe(FALSE);
		if (wf == NOWINDOWFRAME)
		{
			us_abortcommand(_("No current window to delete"));
			return;
		}

		/* save highlighting and turn it off */
		us_pushhighlight();
		us_clearhighlightcount();

		startobjectchange((INTBIG)us_tool, VTOOL);

		/* kill all editor windows on this frame */
		neww = NOWINDOWPART;
		for(w = el_topwindowpart; w != NOWINDOWPART; w = nextw)
		{
			nextw = w->nextwindowpart;
			if (w->frame != wf)
			{
				neww = w;
				continue;
			}

			/* kill this window */
			killwindowpart(w);
		}
		endobjectchange((INTBIG)us_tool, VTOOL);

		/* if (neww == NOWINDOWPART) */
		el_curwindowpart = NOWINDOWPART;
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)neww, VWINDOWPART|VDONTSAVE);
		if (neww != NOWINDOWPART) np = neww->curnodeproto; else np = NONODEPROTO;
		(void)setval((INTBIG)el_curlib, VLIBRARY, x_("curnodeproto"), (INTBIG)np, VNODEPROTO);

		/* restore highlighting */
		us_pophighlight(FALSE);
		return;
	}

	if (namesamen(pp, x_("down"), l) == 0 && l >= 2)
	{
		if (us_needwindow()) return;
		if (count < 2) pp = x_("0.5"); else pp = par[1];
		switch (el_curwindowpart->state&WINDOWTYPE)
		{
			case DISPWINDOW:
				if (us_needcell() == NONODEPROTO) return;
				if (pp[estrlen(pp)-1] == 'l') dist = atola(pp, 0); else
					dist = muldiv(atofr(pp), (el_curwindowpart->screenhy - el_curwindowpart->screenly), WHOLE);
				us_slideup(-dist);
				return;
			case DISP3DWINDOW:
				us_3dpanview(el_curwindowpart, 0, -1);
				return;
			case TEXTWINDOW:
				us_pantext(el_curwindowpart, 0, -1);
				return;
#if SIMTOOL
			case WAVEFORMWINDOW:
				newpar[0] = x_("window");
				newpar[1] = x_("move");
				newpar[2] = x_("down");
				telltool(sim_tool, 3, newpar);
				return;
#endif
			case EXPLORERWINDOW:
				us_explorevpan(el_curwindowpart, -1);
				return;
		}
		us_abortcommand(_("Cannot pan this kind of windows"));
		return;
	}

	if (namesamen(pp, x_("dragging"), l) == 0 && l >= 3)
	{
		if (count >= 2)
		{
			pp = par[1];
			if (namesame(pp, x_("on")) == 0)
				(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate | INTERACTIVE, VINTEGER);
			else if (namesamen(pp, x_("of"), 2) == 0)
				(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate & ~INTERACTIVE, VINTEGER);
			else
			{
				ttyputusage(x_("window dragging on|off"));
				return;
			}
		}
		if ((us_tool->toolstate&INTERACTIVE) == 0)
			ttyputverbose(M_("Cursor-based commands will act immediately")); else
				ttyputverbose(M_("Cursor-based commands will drag their objects"));
		return;
	}

	if (namesamen(pp, x_("explore"), l) == 0)
	{
		/* see if this frame already has an explorer */
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if (w->frame != el_curwindowpart->frame) continue;
			if ((w->state&WINDOWTYPE) == EXPLORERWINDOW) break;
		}
		if (w != NOWINDOWPART)
		{
			/* the explorer is up, delete it */
			el_curwindowpart = w;
			us_killcurrentwindow(TRUE);
			return;
		}

		/* make an explorer window */
		w = el_curwindowpart;
		if (w == NOWINDOWPART)
		{
			w = us_wantnewwindow(0);
			if (w == NOWINDOWPART)
			{
				us_abortcommand(_("Cannot create new window frame"));
				return;
			}
		}
		if (estrcmp(w->location, x_("entire")) == 0)
		{
			w = us_splitcurrentwindow(2, FALSE, 0, us_explorerratio);
			if (w == NOWINDOWPART) return;
		}

		/* find the other window */
		for(oldw = el_topwindowpart; oldw != NOWINDOWPART; oldw = oldw->nextwindowpart)
			if (oldw != w && oldw->frame == w->frame) break;

		/* turn this window into an Explorer */
		us_createexplorerstruct(w);

		if (oldw != NOWINDOWPART)
		{
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)oldw,
				VWINDOWPART|VDONTSAVE);
			us_setcellname(oldw);
		}
		return;
	}

	if (namesamen(pp, x_("grid-zoom"), l) == 0 && l >= 1)
	{
		if (us_needwindow()) return;

		/* cannot zoom a nonstandard window */
		if ((el_curwindowpart->state&WINDOWTYPE) != DISPWINDOW)
		{
			us_abortcommand(_("Can only grid-zoom circuit editing windows"));
			return;
		}

		np = us_needcell();
		if (np == NONODEPROTO) return;
		lambda = lambdaofcell(np);
		x = muldiv(el_curwindowpart->gridx, lambda, WHOLE);
		y = muldiv(el_curwindowpart->gridy, lambda, WHOLE);
		curwx = el_curwindowpart->screenhx-el_curwindowpart->screenlx;
		curwy = el_curwindowpart->screenhy-el_curwindowpart->screenly;
		needwx = muldiv(x, el_curwindowpart->usehx-el_curwindowpart->uselx, 5);
		needwy = muldiv(y, el_curwindowpart->usehy-el_curwindowpart->usely, 5);

		lx = el_curwindowpart->screenlx;   hx = el_curwindowpart->screenhx;
		ly = el_curwindowpart->screenly;   hy = el_curwindowpart->screenhy;
		lx = (hx+lx-needwx)/2;   hx = lx + needwx;
		ly = (hy+ly-needwy)/2;   hy = ly + needwy;
		us_squarescreen(el_curwindowpart, NOWINDOWPART, FALSE, &lx, &hx, &ly, &hy, 1);

		startobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenlx"), lx, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenhx"), hx, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenly"), ly, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenhy"), hy, VINTEGER);
		us_gridset(el_curwindowpart, el_curwindowpart->state|GRIDON);
		endobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);

		/* redisplay */
		us_endchanges(NOWINDOWPART);
		us_state |= HIGHLIGHTSET;
		us_showallhighlight();
		return;
	}

	if (namesamen(pp, x_("highlight-displayed"), l) == 0 && l >= 3)
	{
		if (us_needwindow()) return;

#if SIMTOOL
		/* special case for waveform window */
		if ((el_curwindowpart->state&WINDOWTYPE) == WAVEFORMWINDOW)
		{
			newpar[0] = x_("window");
			newpar[1] = x_("zoom");
			newpar[2] = x_("cursor");
			telltool(sim_tool, 3, newpar);
			return;
		}
#endif

		np = us_getareabounds(&lx, &hx, &ly, &hy);
		if (np == NONODEPROTO)
		{
			us_abortcommand(_("Outline an area"));
			return;
		}

		if (el_curwindowpart->curnodeproto == np) w = el_curwindowpart; else
		{
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
				if (w->curnodeproto == np && (w->state&WINDOWTYPE) == DISPWINDOW)
					break;
			if (w == NOWINDOWPART)
			{
				us_abortcommand(_("Cannot find an editing window with highlighted objects"));
				return;
			}
		}

		/* cannot manipulate a nonstandard window */
		if ((w->state&WINDOWTYPE) != DISPWINDOW)
		{
			us_abortcommand(_("Can only adjust circuit editing windows"));
			return;
		}

		if ((el_curwindowpart->state&INPLACEEDIT) != 0)
			xformbox(&lx, &hx, &ly, &hy, el_curwindowpart->outofcell);
		if (lx == hx && ly == hy)
		{
			lambda = lambdaofcell(np);
			lx -= lambda;
			hx += lambda;
			ly -= lambda;
			hy += lambda;
		}

		/* save and erase highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* make sure the new window has square pixels */
		us_squarescreen(w, NOWINDOWPART, FALSE, &lx, &hx, &ly, &hy, 0);

		startobjectchange((INTBIG)w, VWINDOWPART);
		(void)setval((INTBIG)w, VWINDOWPART, x_("screenlx"), lx, VINTEGER);
		(void)setval((INTBIG)w, VWINDOWPART, x_("screenhx"), hx, VINTEGER);
		(void)setval((INTBIG)w, VWINDOWPART, x_("screenly"), ly, VINTEGER);
		(void)setval((INTBIG)w, VWINDOWPART, x_("screenhy"), hy, VINTEGER);
		us_gridset(w, el_curwindowpart->state);
		endobjectchange((INTBIG)w, VWINDOWPART);

		/* restore highlighting */
		us_pophighlight(FALSE);
		return;
	}

	if (namesamen(pp, x_("in-zoom"), l) == 0 && l >= 1)
	{
		if (us_needwindow()) return;

#if SIMTOOL
		/* special case for waveform window */
		if ((el_curwindowpart->state&WINDOWTYPE) == WAVEFORMWINDOW)
		{
			newpar[0] = x_("window");
			newpar[1] = x_("zoom");
			newpar[2] = x_("in");
			telltool(sim_tool, 3, newpar);
			return;
		}
#endif

		/* zoom a 3D window */
		if ((el_curwindowpart->state&WINDOWTYPE) == DISP3DWINDOW)
		{
			us_3dzoomview(el_curwindowpart, 0.75f);
			return;
		}

		/* cannot zoom a nonstandard window */
		if ((el_curwindowpart->state&WINDOWTYPE) != DISPWINDOW)
		{
			us_abortcommand(_("Can only zoom circuit editing windows"));
			return;
		}

		np = us_needcell();
		if (np == NONODEPROTO) return;
		lambda = lambdaofcell(np);
		if (count >= 2) dist = atola(par[1], 0); else
			dist = 2 * lambda;
		if (dist == 0)
		{
			us_abortcommand(_("Must zoom by a nonzero amount"));
			return;
		}

		diffx = muldiv(el_curwindowpart->screenhx - el_curwindowpart->screenlx,
			lambda, dist);
		diffy = muldiv(el_curwindowpart->screenhy - el_curwindowpart->screenly,
			lambda, dist);
		lx = el_curwindowpart->screenlx;   hx = el_curwindowpart->screenhx;
		ly = el_curwindowpart->screenly;   hy = el_curwindowpart->screenhy;
		lx = (hx+lx-diffx)/2;   hx = lx + diffx;
		ly = (hy+ly-diffy)/2;   hy = ly + diffy;
		us_squarescreen(el_curwindowpart, NOWINDOWPART, FALSE, &lx, &hx, &ly, &hy, 1);

		startobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenlx"), lx, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenhx"), hx, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenly"), ly, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenhy"), hy, VINTEGER);
		us_gridset(el_curwindowpart, el_curwindowpart->state);
		endobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);

		/* restore highlighting */
		us_endchanges(NOWINDOWPART);
		us_state |= HIGHLIGHTSET;
		us_showallhighlight();
		return;
	}

	/* handle killing of the other window specially */
	if (namesamen(pp, x_("join"), l) == 0 && l >= 1)
	{
		if (us_needwindow()) return;
		us_killcurrentwindow(FALSE);
		return;
	}

	/* handle killing of this window specially */
	if (namesamen(pp, x_("kill"), l) == 0 && l >= 1)
	{
		if (us_needwindow()) return;
		us_killcurrentwindow(TRUE);
		return;
	}

	if (namesamen(pp, x_("left"), l) == 0 && l >= 1)
	{
		if (us_needwindow()) return;
		if (count < 2) pp = x_("0.5"); else pp = par[1];
		switch (el_curwindowpart->state&WINDOWTYPE)
		{
			case DISPWINDOW:
				if (us_needcell() == NONODEPROTO) return;
				if (pp[estrlen(pp)-1] == 'l') dist = atola(pp, 0); else
					dist = muldiv(atofr(pp), (el_curwindowpart->screenhx - el_curwindowpart->screenlx), WHOLE);
				us_slideleft(dist);
				return;
			case DISP3DWINDOW:
				us_3dpanview(el_curwindowpart, 1, 0);
				return;
			case TEXTWINDOW:
				us_pantext(el_curwindowpart, 1, 0);
				return;
#if SIMTOOL
			case WAVEFORMWINDOW:
				newpar[0] = x_("window");
				newpar[1] = x_("move");
				newpar[2] = x_("left");
				telltool(sim_tool, 3, newpar);
				return;
#endif
		}
		us_abortcommand(_("Cannot pan this kind of windows"));
		return;
	}

	if (namesamen(pp, x_("match"), l) == 0 && l >= 3)
	{
		/* count the number of windows */
		for(i = 0, w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart) i++;
		if (i <= 1)
		{
			us_abortcommand(_("Must be multiple windows to match them"));
			return;
		}

		/* if there are two windows, the other to match is obvious */
		if (i == 2)
		{
			if (el_curwindowpart == el_topwindowpart) w = el_topwindowpart->nextwindowpart; else
				w = el_topwindowpart;
		} else
		{
			if (count < 2)
			{
				count = ttygetparam(_("Other window to match: "), &us_windowmp, MAXPARS-1, &par[1]) + 1;
				if (count == 1)
				{
					us_abortedmsg();
					return;
				}
			}
			win = par[1];
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
			{
				infstr = initinfstr();
				addstringtoinfstr(infstr, describenodeproto(w->curnodeproto));
				addtoinfstr(infstr, '(');
				addstringtoinfstr(infstr, w->location);
				addtoinfstr(infstr, ')');
				if (namesame(win, returninfstr(infstr)) == 0) break;
			}
			if (w == NOWINDOWPART)
			{
				us_abortcommand(_("No window named '%s'"), win);
				return;
			}
		}

		if (w == el_curwindowpart)
		{
			us_abortcommand(_("Choose a window other than the current one to match"));
			return;
		}

		/* cannot match if they are editing the same thing */
		if ((w->state&WINDOWTYPE) != (el_curwindowpart->state&WINDOWTYPE))
		{
			us_abortcommand(_("Can only match windows that edit the same thing"));
			return;
		}

		/* cannot match if they are not normal display windows */
		if ((w->state&WINDOWTYPE) != DISPWINDOW)
		{
			us_abortcommand(_("Can only match normal editing windows"));
			return;
		}

		/* save and erase highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* make window "el_curwindowpart" match the scale of "w" */
		diffx = muldiv(w->screenhx - w->screenlx, el_curwindowpart->usehx -
			el_curwindowpart->uselx, w->usehx - w->uselx);
		diffx = diffx - (el_curwindowpart->screenhx - el_curwindowpart->screenlx);
		diffy = muldiv(w->screenhy - w->screenly, el_curwindowpart->usehy -
			el_curwindowpart->usely, w->usehy - w->usely);
		diffy = diffy - (el_curwindowpart->screenhy - el_curwindowpart->screenly);

		startobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenlx"),
			el_curwindowpart->screenlx - diffx/2, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenhx"),
			el_curwindowpart->screenhx + diffx/2, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenly"),
			el_curwindowpart->screenly - diffy/2, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenhy"),
			el_curwindowpart->screenhy + diffy/2, VINTEGER);
		us_gridset(el_curwindowpart, el_curwindowpart->state);
		endobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);

		/* restore highlighting */
		us_pophighlight(FALSE);
		return;
	}

	if (namesamen(pp, x_("measure"), l) == 0 && l >= 2)
	{
		if ((el_curwindowpart->state&WINDOWTYPE) != DISPWINDOW &&
			(el_curwindowpart->state&WINDOWTYPE) != WAVEFORMWINDOW)
		{
			us_abortcommand(_("Can only measure distance in an edit and waveform windows"));
			return;
		}
		if (el_curwindowpart->curnodeproto == NONODEPROTO)
		{
			us_abortcommand(_("No cell in this window to measure"));
			return;
		}
		if ((us_state&MEASURINGDISTANCE) != 0)
		{
			us_state &= ~(MEASURINGDISTANCE | MEASURINGDISTANCEINI);
			ttyputmsg(_("Exiting distance measurement mode"));
		} else
		{
			us_clearhighlightcount();
			us_state |= MEASURINGDISTANCE | MEASURINGDISTANCEINI;
			ttyputmsg(_("Entering distance measurement mode"));
		}
		return;
	}

	if (namesamen(pp, x_("move-display"), l) == 0 && l >= 2)
	{
		if (us_needwindow()) return;
#ifdef USEQT
		movedisplay();
#else
		ttyputmsg(_("(Display move works only on Qt"));
#endif
		return;
	}

	if (namesamen(pp, x_("name"), l) == 0 && l >= 2)
	{
		if (us_needwindow()) return;
		if (count <= 1)
		{
			ttyputusage(x_("window name VIEWNAME"));
			return;
		}
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("USER_windowview_"));
		addstringtoinfstr(infstr, par[1]);
		var = getval((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, returninfstr(infstr));
		if (var == NOVARIABLE)
		{
			us_abortcommand(_("Cannot find saved window view '%s'"), par[1]);
			return;
		}

		lx = ((INTBIG *)var->addr)[0];
		hx = ((INTBIG *)var->addr)[1];
		ly = ((INTBIG *)var->addr)[2];
		hy = ((INTBIG *)var->addr)[3];

		/* if the window extent changed, make sure the pixels are square */
		if (((INTBIG *)var->addr)[5] - ((INTBIG *)var->addr)[4] != el_curwindowpart->usehx - el_curwindowpart->uselx ||
			((INTBIG *)var->addr)[7] - ((INTBIG *)var->addr)[6] != el_curwindowpart->usehy - el_curwindowpart->usely)
		{
			us_squarescreen(el_curwindowpart, NOWINDOWPART, FALSE, &lx, &hx, &ly, &hy, 0);
		}

		/* save and erase highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		startobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenlx"), lx, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenhx"), hx, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenly"), ly, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenhy"), hy, VINTEGER);
		us_gridset(el_curwindowpart, el_curwindowpart->state);
		endobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);

		/* restore highlighting */
		us_pophighlight(FALSE);
		return;
	}

	/* handle creating of separate windows */
	if (namesamen(pp, x_("new"), l) == 0 && l >= 2)
	{
		/* create a new frame */
		w = us_wantnewwindow(0);
		if (w == NOWINDOWPART)
		{
			us_abortcommand(_("Cannot create new window frame"));
			return;
		}
		return;
	}

	if (namesamen(pp, x_("normal-cursor"), l) == 0 && l >= 2)
	{
		if (count < 2)
		{
			ttyputusage(x_("window normal-cursor CURSORNAME"));
			return;
		}
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("standard"), l) == 0) setnormalcursor(NORMALCURSOR); else
			if (namesamen(pp, x_("pen"), l) == 0) setnormalcursor(PENCURSOR); else
				if (namesamen(pp, x_("tee"), l) == 0) setnormalcursor(TECHCURSOR); else
					ttyputbadusage(x_("window cursor"));
		return;
	}

	if (namesamen(pp, x_("out-zoom"), l) == 0 && l >= 4)
	{
		if (us_needwindow()) return;

#if SIMTOOL
		/* special case for waveform window */
		if ((el_curwindowpart->state&WINDOWTYPE) == WAVEFORMWINDOW)
		{
			newpar[0] = x_("window");
			newpar[1] = x_("zoom");
			newpar[2] = x_("out");
			telltool(sim_tool, 3, newpar);
			return;
		}
#endif

		/* zoom a 3D window */
		if ((el_curwindowpart->state&WINDOWTYPE) == DISP3DWINDOW)
		{
			us_3dzoomview(el_curwindowpart, 1.5f);
			return;
		}

		/* cannot zoom a nonstandard window */
		if ((el_curwindowpart->state&WINDOWTYPE) != DISPWINDOW)
		{
			us_abortcommand(_("Can only zoom circuit editing windows"));
			return;
		}

		np = us_needcell();
		if (np == NONODEPROTO) return;
		lambda = lambdaofcell(np);
		if (count >= 2) dist = atola(par[1], 0); else
			dist = 2 * lambda;
		if (dist == 0)
		{
			us_abortcommand(_("Must zoom by a nonzero amount"));
			return;
		}

		lx = el_curwindowpart->screenlx;   hx = el_curwindowpart->screenhx;
		ly = el_curwindowpart->screenly;   hy = el_curwindowpart->screenhy;
		i = muldiv(el_curwindowpart->screenhx - el_curwindowpart->screenlx, dist, lambda);
		lx = (lx+hx-i)/2;   hx = lx + i;
		i = muldiv(el_curwindowpart->screenhy - el_curwindowpart->screenly, dist, lambda);
		ly = (ly+hy-i)/2;   hy = ly + i;
		us_squarescreen(el_curwindowpart, NOWINDOWPART, FALSE, &lx, &hx, &ly, &hy, 1);

		startobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenlx"), lx, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenhx"), hx, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenly"), ly, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenhy"), hy, VINTEGER);
		us_gridset(el_curwindowpart, el_curwindowpart->state);
		endobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);

		/* restore highlighting */
		us_endchanges(NOWINDOWPART);
		us_state |= HIGHLIGHTSET;
		us_showallhighlight();
		return;
	}

	if (namesamen(pp, x_("outline-edit-toggle"), l) == 0 && l >= 4)
	{
		/* in outline edit mode:
		 * cursor is a pen
		 * "Erase" deletes selected point
		 * "Rotate" rotates about point
		 * "Mirror" mirrors about point
		 * "Get Info" gives polygon info
		 * left arrow goes to previous point
		 * right arrow goes to next point
		 * selection button selects a point
		 * creation button creates a point
		 */
		if (us_needwindow()) return;

		/* find the rotate menu entry */
		if ((el_curwindowpart->state&WINDOWOUTLINEEDMODE) == 0)
		{
			/* enter outline-edit mode: must have a current node */
			ni = (NODEINST *)us_getobject(VNODEINST, FALSE);
			if (ni == NONODEINST) return;
			if ((ni->proto->userbits&HOLDSTRACE) == 0)
			{
				us_abortcommand(_("Sorry, %s nodes cannot hold outline information"),
					describenodeproto(ni->proto));
				return;
			}
			us_pushhighlight();
			us_clearhighlightcount();
			startobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);
			(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("state"),
				el_curwindowpart->state | WINDOWOUTLINEEDMODE, VINTEGER);
			endobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);
			us_pophighlight(FALSE);

			TDCLEAR(descript);
			TDSETSIZE(descript, TXTSETPOINTS(18));
			us_hbox.col = HIGHLIT;
			us_writetext(el_curwindowpart->uselx, el_curwindowpart->usehx,
				el_curwindowpart->usehy-30, el_curwindowpart->usehy, &us_hbox, _("OUTLINE EDIT MODE:"),
					descript, el_curwindowpart, NOTECHNOLOGY);
			infstr = initinfstr();
			var = getval((INTBIG)us_tool, VTOOL, VSTRING, "USER_local_capf");
			if (var != NOVARIABLE) formatinfstr(infstr, _("Use %s to select/move point; "), (CHAR *)var->addr);
			var = getval((INTBIG)us_tool, VTOOL, VSTRING, "USER_local_capg");
			if (var != NOVARIABLE) formatinfstr(infstr, _(" Use %s to create point; "), (CHAR *)var->addr);
			us_writetext(el_curwindowpart->uselx, el_curwindowpart->usehx,
				el_curwindowpart->usehy-60, el_curwindowpart->usehy-30, &us_hbox, returninfstr(infstr),
					descript, el_curwindowpart, NOTECHNOLOGY);
		} else
		{
			/* leave outline edit mode */
			us_pushhighlight();
			us_clearhighlightcount();
			startobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);
			(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("state"),
				el_curwindowpart->state & ~WINDOWOUTLINEEDMODE, VINTEGER);
			endobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);
			us_pophighlight(FALSE);
		}
		noundoallowed();
		return;
	}

	if (namesamen(pp, x_("overlappable-display"), l) == 0 && l >= 5)
	{
		if (count >= 2)
		{
			pp = par[1];
			l = estrlen(pp);
			if (namesamen(pp, x_("on"), l) == 0)
			{
				us_state &= ~NONOVERLAPPABLEDISPLAY;
			} else if (namesamen(pp, x_("off"), l) == 0)
			{
				us_state |= NONOVERLAPPABLEDISPLAY;
			} else
			{
				ttyputusage(x_("window overlappable-display [on|off]"));
				return;
			}
		}
		if ((us_state&NONOVERLAPPABLEDISPLAY) != 0)
			ttyputverbose(M_("Transparent layers will not be handled")); else
				ttyputverbose(M_("Transparent layers will be drawn properly"));
		return;
	}

	if (namesamen(pp, x_("overview"), l) == 0 && l >= 5)
	{
		ttyputerr(_("Cannot make an overview window yet"));
		return;
	}

	if (namesamen(pp, x_("peek"), l) == 0 && l >= 2)
	{
		np = us_getareabounds(&lx, &hx, &ly, &hy);
		if (np == NONODEPROTO)
		{
			us_abortcommand(_("Enclose an area to be peeked"));
			return;
		}

		if (el_curwindowpart->curnodeproto == np) w = el_curwindowpart; else
		{
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
				if (w->curnodeproto == np && (w->state&WINDOWTYPE) == DISPWINDOW)
					break;
			if (w == NOWINDOWPART)
			{
				us_abortcommand(_("Cannot find an editing window with highlighted objects"));
				return;
			}
		}

		/* clip this bounding box to the window extent */
		lx = maxi(lx, w->screenlx);
		hx = mini(hx, w->screenhx);
		ly = maxi(ly, w->screenly);
		hy = mini(hy, w->screenhy);

		/* save and erase highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* un-draw the peek area */
		maketruerectpoly(lx, hx, ly, hy, poly);
		poly->desc = &us_arbit;
		us_arbit.col = 0;
		us_arbit.bits = LAYERO;
		poly->style = FILLEDRECT;
		us_showpoly(poly, w);

		/* get new window to describe sub-area */
		w = us_subwindow(lx, hx, ly, hy, w);

		/* do the peek operation */
		us_dopeek(lx, hx, ly, hy, np, w);

		/* restore highlighting */
		us_pophighlight(FALSE);
		return;
	}

	if (namesamen(pp, x_("right"), l) == 0 && l >= 1)
	{
		if (us_needwindow()) return;
		if (count < 2) pp = x_("0.5"); else pp = par[1];
		switch (el_curwindowpart->state&WINDOWTYPE)
		{
			case DISPWINDOW:
				if (us_needcell() == NONODEPROTO) return;
				if (pp[estrlen(pp)-1] == 'l') dist = atola(pp, 0); else
					dist = muldiv(atofr(pp), (el_curwindowpart->screenhx - el_curwindowpart->screenlx), WHOLE);
				us_slideleft(-dist);
				return;
			case DISP3DWINDOW:
				us_3dpanview(el_curwindowpart, -1, 0);
				return;
			case TEXTWINDOW:
				us_pantext(el_curwindowpart, -1, 0);
				return;
#if SIMTOOL
			case WAVEFORMWINDOW:
				newpar[0] = x_("window");
				newpar[1] = x_("move");
				newpar[2] = x_("right");
				telltool(sim_tool, 3, newpar);
				return;
#endif
		}
		us_abortcommand(_("Cannot pan this kind of windows"));
		return;
	}

	if (namesamen(pp, x_("save"), l) == 0 && l >= 2)
	{
		if (us_needwindow()) return;
		if (count <= 1)
		{
			ttyputusage(x_("window save VIEWNAME"));
			return;
		}
		windowView[0] = el_curwindowpart->screenlx;
		windowView[1] = el_curwindowpart->screenhx;
		windowView[2] = el_curwindowpart->screenly;
		windowView[3] = el_curwindowpart->screenhy;
		windowView[4] = el_curwindowpart->uselx;
		windowView[5] = el_curwindowpart->usehx;
		windowView[6] = el_curwindowpart->usely;
		windowView[7] = el_curwindowpart->usehy;
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("USER_windowview_"));
		addstringtoinfstr(infstr, par[1]);
		(void)setval((INTBIG)us_tool, VTOOL, returninfstr(infstr), (INTBIG)windowView,
			VINTEGER|VISARRAY|(8<<VLENGTHSH)|VDONTSAVE);
		ttyputverbose(M_("Window view %s saved"), par[1]);
		return;
	}

	if (namesamen(pp, x_("split"), l) == 0 && l >= 2)
	{
		if (us_needwindow()) return;
		splitkey = 0;
		if (count > 1)
		{
			l = estrlen(pp = par[1]);
			if (namesamen(pp, x_("horizontal"), l) == 0 && l >= 1) splitkey = 1; else
				if (namesamen(pp, x_("vertical"), l) == 0 && l >= 1) splitkey = 2; else
			{
				ttyputusage(x_("window split horizontal|vertical"));
				return;
			}
		}

		/* split the window */
		(void)us_splitcurrentwindow(splitkey, TRUE, 0, 50);
		return;
	}

	if (namesamen(pp, x_("status-bar"), l) == 0 && l >= 2)
	{
		if (count < 2)
		{
			/* report all status bar locations */
			for(i=0; us_statusfields[i].fieldaddr != 0; i++)
			{
				sf = *us_statusfields[i].fieldaddr;
				if (sf == 0) continue;
				if (sf->line == 0)
					ttyputmsg(M_("Window title has %s"), us_statusfields[i].fieldaddr); else
						ttyputmsg(M_("Line %ld from %3ld%% to %3ld%% is %s"), sf->line, sf->startper,
							sf->endper, us_statusfields[i].fieldaddr);
			}
			return;
		}

		if (namesamen(par[1], x_("current-node"), estrlen(par[1])) == 0)
		{
			if (count < 3)
			{
				if ((us_state&NONPERSISTENTCURNODE) != 0)
					ttyputmsg(M_("Current node displayed temporarily")); else
						ttyputmsg(M_("Current node display is persistent"));
				return;
			}
			l = estrlen(pp = par[2]);
			if (namesamen(pp, x_("persistent"), l) == 0)
			{
				us_state &= ~NONPERSISTENTCURNODE;
				ttyputverbose(M_("Current node display is persistent"));
				return;
			}
			if (namesamen(pp, x_("temporary"), l) == 0)
			{
				us_state |= NONPERSISTENTCURNODE;
				ttyputverbose(M_("Current node displayed temporarily"));
				return;
			}
			ttyputusage(x_("window status-bar current-node [persistent|temporary]"));
			return;
		}

		if (count < 3)
		{
			ttyputusage(x_("window status-bar COMMAND FIELD..."));
			return;
		}

		/* determine area being controlled */
		l = estrlen(pp = par[2]);
		for(i=0; us_statusfields[i].keyword != 0; i++)
			if (namesamen(pp, us_statusfields[i].keyword, l) == 0 &&
				l >= us_statusfields[i].unique)
		{
			whichstatus = us_statusfields[i].fieldaddr;
			break;
		}
		if (us_statusfields[i].keyword == 0)
		{
			us_abortcommand(_("Unknown status-bar location: %s"), pp);
			return;
		}

		/* get option */
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("delete"), l) == 0)
		{
			if (*whichstatus != 0) ttyfreestatusfield(*whichstatus);
			*whichstatus = 0;
			us_redostatus(NOWINDOWFRAME);
			return;
		}
		if (namesamen(pp, x_("add"), l) == 0)
		{
			if (count < 6)
			{
				ttyputusage(x_("window status-bar add FIELD LINE STARTPER ENDPER [TITLE]"));
				return;
			}
			lineno = eatoi(par[3]);
			if (lineno < 0 || lineno > ttynumstatuslines())
			{
				us_abortcommand(_("Line number must range from 0 to %ld"), ttynumstatuslines());
				return;
			}
			startper = eatoi(par[4]);
			if (startper < 0 || startper > 100)
			{
				us_abortcommand(_("Starting percentage must range from 0 to 100"));
				return;
			}
			endper = eatoi(par[5]);
			if (endper <= startper || endper > 100)
			{
				us_abortcommand(_("Ending percentage must range from %ld to 100"), startper+1);
				return;
			}
			if (count == 7) fieldname = par[6]; else fieldname = x_("");
			if (*whichstatus != 0) ttyfreestatusfield(*whichstatus);
			*whichstatus = ttydeclarestatusfield(lineno, startper, endper, fieldname);
			us_redostatus(NOWINDOWFRAME);
			return;
		}
		ttyputusage(x_("window status-bar [add | delete]"));
		return;
	}

	if (namesamen(pp, x_("tiny-cells"), l) == 0 && l >= 2)
	{
		if (us_needwindow()) return;
		if (count >= 2)
		{
			l = estrlen(pp = par[1]);
			if (namesamen(pp, x_("draw"), l) == 0)
			{
				startobjectchange((INTBIG)us_tool, VTOOL);
				(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey,
					us_useroptions | DRAWTINYCELLS, VINTEGER);
				if (count >= 3)
				{
					i = atofr(par[2]);
					(void)setvalkey((INTBIG)us_tool, VTOOL, us_tinylambdaperpixelkey,
						i, VFRACT);
				}
				endobjectchange((INTBIG)us_tool, VTOOL);
			} else if (namesamen(pp, x_("hash-out"), l) == 0)
			{
				startobjectchange((INTBIG)us_tool, VTOOL);
				(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey,
					us_useroptions & ~DRAWTINYCELLS, VINTEGER);
				endobjectchange((INTBIG)us_tool, VTOOL);
			} else
			{
				ttyputusage(x_("window tiny-cells draw|(hash-out [LAMBDAPERPIXEL])"));
				return;
			}
		}
		if ((us_useroptions&DRAWTINYCELLS) != 0)
		{
			ttyputverbose(M_("Tiny cells will be drawn"));
		} else
		{
			ttyputverbose(M_("Tiny cells will be hashed-out after %s lambda per pixel"),
				frtoa(us_tinyratio));
		}
		return;
	}

	if (namesamen(pp, x_("trace-displayed"), l) == 0 && l >= 2)
	{
		np = us_needcell();
		if (np == NONODEPROTO) return;

		/* cannot manipulate a nonstandard window */
		if ((el_curwindowpart->state&WINDOWTYPE) != DISPWINDOW)
		{
			us_abortcommand(_("Can only adjust circuit editing windows"));
			return;
		}

		/* pre-compute current window size */
		curwx = el_curwindowpart->screenhx - el_curwindowpart->screenlx;
		curwy = el_curwindowpart->screenhy - el_curwindowpart->screenly;

		var = getval((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_commandvarname('T'));
		if (var == NOVARIABLE)
		{
			us_abortcommand(_("Issue an outline before zooming into that area"));
			return;
		}
		size = getlength(var) / 2;
		nogood = 0;
		for(i=0; i<size; i++)
		{
			x = ((INTBIG *)var->addr)[i*2];
			y = ((INTBIG *)var->addr)[i*2+1];
			if (us_setxy(x, y)) nogood++;
			(void)getxy(&xcur, &ycur);
			if (i == 0)
			{
				lx = hx = xcur;   ly = hy = ycur;
			} else
			{
				lx = mini(lx, xcur);   hx = maxi(hx, xcur);
				ly = mini(ly, ycur);   hy = maxi(hy, ycur);
			}
		}
		if (nogood != 0)
		{
			us_abortcommand(_("Outline not inside window"));
			return;
		}

		/* save and erase highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* set the new window size */
		us_squarescreen(el_curwindowpart, NOWINDOWPART, FALSE, &lx, &hx, &ly, &hy, 0);

		startobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenlx"), lx, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenhx"), hx, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenly"), ly, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenhy"), hy, VINTEGER);
		us_gridset(el_curwindowpart, el_curwindowpart->state);
		endobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);

		/* restore highlighting */
		us_pophighlight(FALSE);
		return;
	}

	if (namesamen(pp, x_("up"), l) == 0 && l >= 2)
	{
		if (us_needwindow()) return;
		if (count < 2) pp = x_("0.5"); else pp = par[1];
		switch (el_curwindowpart->state&WINDOWTYPE)
		{
			case DISPWINDOW:
				if (us_needcell() == NONODEPROTO) return;
				if (pp[estrlen(pp)-1] == 'l') dist = atola(pp, 0); else
					dist = muldiv(atofr(pp), (el_curwindowpart->screenhy - el_curwindowpart->screenly), WHOLE);
				us_slideup(dist);
				return;
			case DISP3DWINDOW:
				us_3dpanview(el_curwindowpart, 0, 1);
				return;
			case TEXTWINDOW:
				us_pantext(el_curwindowpart, 0, 1);
				return;
#if SIMTOOL
			case WAVEFORMWINDOW:
				newpar[0] = x_("window");
				newpar[1] = x_("move");
				newpar[2] = x_("up");
				telltool(sim_tool, 3, newpar);
				return;
#endif
			case EXPLORERWINDOW:
				us_explorevpan(el_curwindowpart, 1);
				return;
		}
		us_abortcommand(_("Cannot pan this kind of windows"));
		return;
	}

	if (namesamen(pp, x_("use"), l) == 0 && l >= 2)
	{
		if (count <= 1)
		{
			count = ttygetparam(M_("Window to use: "), &us_windowup, MAXPARS-1, &par[1]) + 1;
			if (count == 1)
			{
				us_abortedmsg();
				return;
			}
		}
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, describenodeproto(w->curnodeproto));
			addtoinfstr(infstr, '(');
			addstringtoinfstr(infstr, w->location);
			addtoinfstr(infstr, ')');
			if (namesame(par[1], returninfstr(infstr)) == 0) break;
		}
		if (w == NOWINDOWPART)
		{
			us_abortcommand(_("No window named '%s'"), par[1]);
			return;
		}
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)w, VWINDOWPART|VDONTSAVE);
		(void)setval((INTBIG)el_curlib, VLIBRARY, x_("curnodeproto"), (INTBIG)w->curnodeproto, VNODEPROTO);
		return;
	}

	if (namesamen(pp, x_("zoom-scale"), l) == 0 && l >= 1)
	{
		if (count >= 2)
		{
			l = estrlen(pp = par[1]);
			if (namesamen(pp, x_("integral"), l) == 0 && l >= 1)
				(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate | INTEGRAL, VINTEGER);
			else if (namesamen(pp, x_("nonintegral"), l) == 0 && l >= 1)
				(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate & ~INTEGRAL, VINTEGER);
			else
			{
				ttyputusage(x_("window zoom-scale integral|nonintegral"));
				return;
			}
		}
		if ((us_tool->toolstate&INTEGRAL) == 0)
			ttyputverbose(M_("Window scaling will be continuous")); else
				ttyputverbose(M_("Window scaling will force integral pixel alignment"));
		return;
	}

	if (namesamen(pp, x_("1-window"), l) == 0 && l >= 1)
	{
		if (us_needwindow()) return;

		if (estrcmp(el_curwindowpart->location, x_("entire")) == 0)
		{
			ttyputmsg(_("Already displaying only one window"));
			return;
		}

		/* remember the current window */
		oldw = el_curwindowpart;

		/* turn off highlighting */
		us_pushhighlight();
		us_clearhighlightcount();
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)NOWINDOWPART,
			VWINDOWPART|VDONTSAVE);

		startobjectchange((INTBIG)us_tool, VTOOL);

		/* create a new window */
		neww = newwindowpart(x_("entire"), oldw);
		if (neww == NOWINDOWPART)
		{
			ttyputnomemory();
			return;
		}

		/* if reducing to an editor window, move the editor structure */
		if ((oldw->state&WINDOWTYPE) == TEXTWINDOW ||
			(oldw->state&WINDOWTYPE) == POPTEXTWINDOW)
		{
			(void)setval((INTBIG)neww, VWINDOWPART, x_("editor"), (INTBIG)oldw->editor, VADDRESS);
			(void)setval((INTBIG)oldw, VWINDOWPART, x_("editor"), -1, VADDRESS);
		}

		/* now delete all other windows */
		for(w = el_topwindowpart; w != NOWINDOWPART; w = nextw)
		{
			nextw = w->nextwindowpart;
			if (w->frame != neww->frame) continue;
			if (w != neww) killwindowpart(w);
		}

		/* set the window extents */
		us_windowfit(NOWINDOWFRAME, FALSE, 1);
		endobjectchange((INTBIG)us_tool, VTOOL);

		/* restore highlighting */
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)neww,
			VWINDOWPART|VDONTSAVE);
		us_pophighlight(FALSE);
		return;
	}

	if (namesamen(pp, x_("3-dimensional"), l) == 0 && l >= 1)
	{
		if (us_needwindow()) return;
		w = el_curwindowpart;
		if (count < 2)
		{
			ttyputusage(x_("window 3-dimensional OPTION"));
			return;
		}

		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("begin"), l) == 0 && l >= 1)
		{
			if ((w->state&WINDOWTYPE) == DISP3DWINDOW)
			{
				ttyputmsg(_("Window already shown in 3 dimensions"));
				return;
			}
			if ((w->state&WINDOWTYPE) != DISPWINDOW)
			{
				us_abortcommand(_("Cannot view this window in 3 dimensions"));
				return;
			}
			np = us_needcell();
			if (np == NONODEPROTO) return;

			us_3dsetupviewing(w);

			/* clear highlighting */
			us_clearhighlightcount();

			/* make the cell fill the window */
			us_fullview(np, &lx, &hx, &ly, &hy);
			us_squarescreen(w, NOWINDOWPART, FALSE, &lx, &hx, &ly, &hy, 0);

			/* set the window extents */
			startobjectchange((INTBIG)w, VWINDOWPART);
			(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenlx"), lx, VINTEGER);
			(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenhx"), hx, VINTEGER);
			(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenly"), ly, VINTEGER);
			(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenhy"), hy, VINTEGER);
			(void)setval((INTBIG)w, VWINDOWPART, x_("buttonhandler"), (INTBIG)us_3dbuttonhandler,
				VADDRESS);
			(void)setval((INTBIG)w, VWINDOWPART, x_("state"), (w->state & ~WINDOWTYPE) | DISP3DWINDOW,
				VINTEGER);
			endobjectchange((INTBIG)w, VWINDOWPART);
			return;
		}

		if (namesamen(pp, x_("end"), l) == 0 && l >= 1)
		{
			if ((w->state&WINDOWTYPE) == DISPWINDOW)
			{
				ttyputmsg(_("Window already shown in 2 dimensions"));
				return;
			}
			if ((w->state&WINDOWTYPE) != DISP3DWINDOW)
			{
				us_abortcommand(_("Cannot view this window in 2 dimensions"));
				return;
			}

			/* set the window extents */
			lx = w->screenlx;   hx = w->screenhx;
			ly = w->screenly;   hy = w->screenhy;
			us_squarescreen(w, NOWINDOWPART, FALSE, &lx, &hx, &ly, &hy, 0);
			startobjectchange((INTBIG)w, VWINDOWPART);
			(void)setval((INTBIG)w, VWINDOWPART, x_("state"), (w->state & ~WINDOWTYPE) | DISPWINDOW,
				VINTEGER);
			(void)setval((INTBIG)w, VWINDOWPART, x_("buttonhandler"), (INTBIG)DEFAULTBUTTONHANDLER,
				VADDRESS);
			(void)setval((INTBIG)w, VWINDOWPART, x_("screenlx"), lx, VINTEGER);
			(void)setval((INTBIG)w, VWINDOWPART, x_("screenhx"), hx, VINTEGER);
			(void)setval((INTBIG)w, VWINDOWPART, x_("screenly"), ly, VINTEGER);
			(void)setval((INTBIG)w, VWINDOWPART, x_("screenhy"), hy, VINTEGER);
			us_gridset(w, el_curwindowpart->state);
			endobjectchange((INTBIG)w, VWINDOWPART);
			return;
		}
		if (namesamen(pp, x_("rotate"), l) == 0 && l >= 1)
		{
			us_3dsetinteraction(0);
			return;
		}
		if (namesamen(pp, x_("zoom"), l) == 0 && l >= 1)
		{
			us_3dsetinteraction(1);
			return;
		}
		if (namesamen(pp, x_("pan"), l) == 0 && l >= 1)
		{
			us_3dsetinteraction(2);
			return;
		}
		if (namesamen(pp, x_("twist"), l) == 0 && l >= 1)
		{
			us_3dsetinteraction(3);
			return;
		}
	}

	ttyputbadusage(x_("window"));
}

void us_yanknode(INTBIG count, CHAR *par[])
{
	REGISTER BOOLEAN found;
	REGISTER INTBIG total;
	REGISTER NODEINST *topno;
	REGISTER NODEPROTO *np;
	REGISTER GEOM **list;

	list = us_gethighlighted(WANTNODEINST, 0, 0);
	if (list[0] == NOGEOM)
	{
		us_abortcommand(_("Must highlight cell(s) to be yanked"));
		return;
	}

	np = geomparent(list[0]);
	found = FALSE;
	for(total=0; list[total] != NOGEOM; total++)
	{
		topno = list[total]->entryaddr.ni;
		if (topno->proto->primindex != 0) continue;

		/* disallow yanking if lock is on */
		if (us_cantedit(np, topno, TRUE)) return;

		/* turn off highlighting for the first cell */
		if (!found) us_clearhighlightcount();

		/* yank this cell */
		us_yankonenode(topno);
		found = TRUE;
	}

	if (!found) us_abortcommand(_("Can only yank cells"));
}
