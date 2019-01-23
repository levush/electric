/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrwindow.c
 * User interface tool: public graphics routines
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
#include "efunction.h"
#include "usr.h"
#include "usrtrack.h"
#include "tecgen.h"
#include <math.h>

/***** REDRAW MODULES *****/

#define NOREDRAW ((REDRAW *)-1)

typedef struct Iredraw
{
	BOOLEAN          entryisnode;		/* true of object being re-drawn is node */
	union us_entry
	{
		NODEINST   *ni;
		ARCINST    *ai;
		PORTPROTO  *pp;
		void       *blind;
	} entryaddr;						/* object being re-drawn */
	struct Iredraw *nextredraw;			/* next in list */
} REDRAW;

static REDRAW   *us_redrawfree = NOREDRAW;	/* free list of re-draw modules */
static REDRAW   *us_firstredraw = NOREDRAW;	/* first in list of active re-draw modules */
static REDRAW   *us_firstopaque = NOREDRAW;	/* first in list of active opaque re-draws */
static REDRAW   *us_firstexport = NOREDRAW;	/* first in list of export re-draws */
static REDRAW   *us_lastopaque = NOREDRAW;	/* last in list of active opaque re-draws */
static REDRAW   *us_usedredraw = NOREDRAW;	/* first in list of used re-draw modules */

static REDRAW *us_allocredraw(void);
static void    us_freeredraw(REDRAW*);

/***** 3D DISPLAY *****/

#define FOVMAX 170.0f
#define FOVMIN   2.0f

enum {ROTATEVIEW, ZOOMVIEW, PANVIEW, TWISTVIEW};

typedef struct
{
	INTBIG    count;
	INTBIG    total;
	GRAPHICS *desc;
	float     depth;
	float    *x, *y, *z;
} POLY3D;

static BOOLEAN     us_3dgatheringpolys = 0;
static INTBIG      us_3dpolycount;
static INTBIG      us_3dpolytotal = 0;
static POLY3D    **us_3dpolylist;
static WINDOWPART *us_3dwindowpart;

/* interaction information */
static INTBIG      us_3dinteraction = ROTATEVIEW;
static INTBIG      us_3dinitialbuty;
static INTBIG      us_3dlastbutx, us_3dlastbuty;
static float       us_3dinitialfov;
static float       us_3dcenterx, us_3dcentery, us_3dcenterz;
static float       us_3dlowx, us_3dhighx, us_3dlowy, us_3dhighy, us_3dlowz, us_3dhighz;

static BOOLEAN us_3deachdown(INTBIG x, INTBIG y);
static void    us_3dbuildtransform(XFORM3D *xf3);
static void    us_3drender(WINDOWPART *w);
static POLY3D *us_3dgetnextpoly(INTBIG count);
static int     us_3dpolydepthascending(const void *e1, const void *e2);
static void    us_3drotatepointaboutaxis(float *p, float theta, float *r, float *q);

/***** MISCELLANEOUS *****/

#define ZSHIFT       4
#define ZUP(x)       ((x) << ZSHIFT)
#define ZDN(x)       (((x) + 1) >> ZSHIFT)

#define CROSSSIZE      3				/* size in pixels of half a cross */
#define BIGCROSSSIZE   5				/* size in pixels of half a big cross */

extern GRAPHICS us_ebox;

/* prototypes for local routines */
static INTBIG us_showin(WINDOWPART*, GEOM*, NODEPROTO*, XARRAY, INTBIG, INTBIG);
static void us_queuevicinity(WINDOWPART*, GEOM*, NODEPROTO*, XARRAY, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static void us_wanttodrawo(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, WINDOWPART*, GRAPHICS*, INTBIG);
static void us_drawextendedpolyline(POLYGON*, WINDOWPART*);

/*
 * Routine to free all memory associated with this module.
 */
void us_freewindowmemory(void)
{
	REGISTER REDRAW *r;
	REGISTER POLY3D *p3;
	REGISTER INTBIG i;

	while (us_redrawfree != NOREDRAW)
	{
		r = us_redrawfree;
		us_redrawfree = us_redrawfree->nextredraw;
		efree((CHAR *)r);
	}

	for(i=0; i<us_3dpolytotal; i++)
	{
		p3 = us_3dpolylist[i];
		if (p3->total > 0)
		{
			efree((CHAR *)p3->x);
			efree((CHAR *)p3->y);
			efree((CHAR *)p3->z);
		}
		efree((CHAR *)p3);
	}
	if (us_3dpolytotal > 0)
		efree((CHAR *)us_3dpolylist);
}

/******************** WINDOW DISPLAY ********************/

/* routine to erase the cell in window "w" */
void us_clearwindow(WINDOWPART *w)
{
	REGISTER INTBIG newstate;

	startobjectchange((INTBIG)w, VWINDOWPART);
	(void)setval((INTBIG)w, VWINDOWPART, x_("curnodeproto"), (INTBIG)NONODEPROTO, VNODEPROTO);
	newstate = (w->state & ~(GRIDON|WINDOWTYPE|WINDOWMODE)) | DISPWINDOW;
	(void)setval((INTBIG)w, VWINDOWPART, x_("state"), newstate, VINTEGER);
	(void)setval((INTBIG)w, VWINDOWPART, x_("buttonhandler"), (INTBIG)DEFAULTBUTTONHANDLER, VADDRESS);
	(void)setval((INTBIG)w, VWINDOWPART, x_("charhandler"), (INTBIG)DEFAULTCHARHANDLER, VADDRESS);
	(void)setval((INTBIG)w, VWINDOWPART, x_("changehandler"), (INTBIG)DEFAULTCHANGEHANDLER, VADDRESS);
	(void)setval((INTBIG)w, VWINDOWPART, x_("termhandler"), (INTBIG)DEFAULTTERMHANDLER, VADDRESS);
	(void)setval((INTBIG)w, VWINDOWPART, x_("redisphandler"), (INTBIG)DEFAULTREDISPHANDLER, VADDRESS);
	endobjectchange((INTBIG)w, VWINDOWPART);
}

/* routine to erase the display of window "w" */
void us_erasewindow(WINDOWPART *w)
{
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	maketruerectpoly(w->screenlx, w->screenhx, w->screenly, w->screenhy, poly);
	if ((w->state&INPLACEEDIT) != 0)
		xformpoly(poly, w->intocell);
	poly->desc = &us_ebox;
	poly->style = FILLEDRECT;
	(*us_displayroutine)(poly, w);
}

/*
 * routine to begin editing cell "cur" in the current window.  The screen
 * extents of the cell are "lx", "hx", "ly", and "hy".  If "focusinst"
 * is not NONODEINST, then highlight that node and port "cellinstport"
 * (if it is not NOPORTPROTO) in the new window.  If "newframe" is true,
 * create a window for this cell, otherwise reuse the current one.
 */
void us_switchtocell(NODEPROTO *cur, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy,
	NODEINST *focusinst, PORTPROTO *cellinstport, BOOLEAN newframe, BOOLEAN pushpop, BOOLEAN exact)
{
	HIGHLIGHT newhigh;
	REGISTER INTBIG i, l, resethandlers, oldmode;
	INTBIG dummy;
	REGISTER VARIABLE *var;
	CHAR *one[1];
	REGISTER INTBIG oldstate;
	REGISTER CHAR *thisline;
	REGISTER EDITOR *ed;
	WINDOWPART *neww, *oldw;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	extern GRAPHICS us_hbox;
	REGISTER void *infstr;

	us_clearhighlightcount();
	oldw = el_curwindowpart;
	if (oldw == NOWINDOWPART)
	{
		/* no former window: force a new one to be created */
		newframe = TRUE;
	} else
	{
		/* if this is an explorer window, use the other partition (if there is one) */
		if ((oldw->state&WINDOWTYPE) == EXPLORERWINDOW)
		{
			for(neww = el_topwindowpart; neww != NOWINDOWPART; neww = neww->nextwindowpart)
				if (neww != oldw && neww->frame == oldw->frame) break;
			if (neww != NOWINDOWPART) oldw = neww;
		}
	}

	/* start changes to the tool */
	startobjectchange((INTBIG)us_tool, VTOOL);

	if (newframe)
	{
		/* just create a new window for this cell */
		neww = us_wantnewwindow(0);
		if (neww == NOWINDOWPART)
		{
			us_abortcommand(_("Cannot create new window"));
			return;
		}
		oldmode = oldstate = DISPWINDOW;
	} else
	{
		/* reuse the window */
		oldmode = oldw->state;
		if (pushpop == 0 && (oldw->state&WINDOWMODE) != 0)
		{
			/* was in a mode: turn that off */
			us_setwindowmode(oldw, oldw->state, oldw->state & ~WINDOWMODE);
		}
		oldstate = oldw->state;
		neww = oldw;
	}
	el_curwindowpart = neww;

	if ((cur->cellview->viewstate&TEXTVIEW) != 0)
	{
		/* text window: make an editor */
		if (us_makeeditor(neww, describenodeproto(cur), &dummy, &dummy) == NOWINDOWPART)
			return;

		/* get the text that belongs here */
		var = getvalkey((INTBIG)cur, VNODEPROTO, VSTRING|VISARRAY, el_cell_message_key);
		if (var == NOVARIABLE)
		{
			one[0] = x_("");
			var = setvalkey((INTBIG)cur, VNODEPROTO, el_cell_message_key, (INTBIG)one,
				VSTRING|VISARRAY|(1<<VLENGTHSH));
			if (var == NOVARIABLE) return;
		}

		ed = neww->editor;
		ed->editobjaddr = (CHAR *)cur;
		ed->editobjtype = VNODEPROTO;
		ed->editobjqual = x_("FACET_message");
		ed->editobjvar = var;

		/* load the text into the window */
		us_suspendgraphics(neww);
		l = getlength(var);
		for(i=0; i<l; i++)
		{
			thisline = ((CHAR **)var->addr)[i];
			if (i == l-1 && *thisline == 0) continue;
			us_addline(neww, i, thisline);
		}
		us_resumegraphics(neww);

		/* setup for editing */
		neww->curnodeproto = cur;
		neww->changehandler = us_textcellchanges;
	} else
	{
		neww->curnodeproto = cur;
		neww->editor = NOEDITOR;

		/* change the window type to be appropriate for editing */
		neww->state = (oldstate & ~WINDOWTYPE) | DISPWINDOW;
		resethandlers = 1;
		if (pushpop != 0)
		{
			if ((neww->state&WINDOWMODE) != 0 &&
				((oldstate&WINDOWTYPE) == DISPWINDOW))
			{
				resethandlers = 0;
			}
		}

		/* if editing a technology-edit cell, indicate so */
		if ((cur->userbits&TECEDITCELL) != 0)
		{
			us_setwindowmode(neww, neww->state, neww->state | WINDOWTECEDMODE);
		}

		/* adjust window if it changed from display window to something else */
		if ((neww->state&WINDOWTYPE) == DISPWINDOW && (oldstate&WINDOWTYPE) != DISPWINDOW)
		{
			/* became a display window: shrink the bottom and right edge */
			neww->usehx -= DISPLAYSLIDERSIZE;
			neww->usely += DISPLAYSLIDERSIZE;
		}
		if ((neww->state&WINDOWTYPE) != DISPWINDOW && (oldstate&WINDOWTYPE) == DISPWINDOW)
		{
			/* no longer a display window: shrink the bottom and right edge */
			neww->usehx += DISPLAYSLIDERSIZE;
			neww->usely -= DISPLAYSLIDERSIZE;
		}

		/* adjust window if it changed from waveform window to something else */
		if ((neww->state&WINDOWTYPE) == WAVEFORMWINDOW && (oldstate&WINDOWTYPE) != WAVEFORMWINDOW)
		{
			/* became a waveform window: shrink the bottom and right edge */
			neww->uselx += DISPLAYSLIDERSIZE;
			neww->usely += DISPLAYSLIDERSIZE;
		}
		if ((neww->state&WINDOWTYPE) != WAVEFORMWINDOW && (oldstate&WINDOWTYPE) == WAVEFORMWINDOW)
		{
			/* no longer a waveform window: shrink the bottom and right edge */
			neww->uselx -= DISPLAYSLIDERSIZE;
			neww->usely -= DISPLAYSLIDERSIZE;
		}

		if (resethandlers != 0)
		{
			neww->buttonhandler = DEFAULTBUTTONHANDLER;
			neww->changehandler = DEFAULTCHANGEHANDLER;
			neww->charhandler = DEFAULTCHARHANDLER;
			neww->termhandler = DEFAULTTERMHANDLER;
			neww->redisphandler = DEFAULTREDISPHANDLER;
		}

		us_squarescreen(neww, NOWINDOWPART, FALSE, &lx, &hx, &ly, &hy, exact);
		neww->screenlx = lx;
		neww->screenhx = hx;
		neww->screenly = ly;
		neww->screenhy = hy;
		computewindowscale(neww);

		/* window gets bigger: see if grid can be drawn */
		us_gridset(neww, neww->state);

		if (focusinst != NONODEINST)
		{
			newhigh.status = HIGHFROM;
			newhigh.cell = focusinst->parent;
			newhigh.fromgeom = focusinst->geom;
			newhigh.fromport = cellinstport;
			newhigh.frompoint = 0;
			newhigh.fromvar = NOVARIABLE;
			newhigh.fromvarnoeval = NOVARIABLE;
			us_addhighlight(&newhigh);
		}
	}

	(void)setval((INTBIG)el_curlib, VLIBRARY, x_("curnodeproto"), (INTBIG)cur, VNODEPROTO);
	endobjectchange((INTBIG)us_tool, VTOOL);
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)neww,
		VWINDOWPART|VDONTSAVE);
	us_setcellname(neww);
	us_setcellsize(neww);
	us_setgridsize(neww);
	if (neww->redisphandler != 0) (*neww->redisphandler)(neww);
	if ((neww->state&WINDOWTECEDMODE) != 0 && (oldmode&WINDOWTECEDMODE) == 0)
	{
		TDCLEAR(descript);
		TDSETSIZE(descript, TXTSETPOINTS(18));
		us_hbox.col = HIGHLIT;
		us_writetext(neww->uselx, neww->usehx, neww->usehy-30, neww->usehy, &us_hbox,
			_("TECHNOLOGY EDIT MODE:"), descript, neww, NOTECHNOLOGY);
		var = getval((INTBIG)us_tool, VTOOL, VSTRING, "USER_local_capg");
		if (var != NOVARIABLE)
		{
			infstr = initinfstr();
			formatinfstr(infstr, _("Use %s to make changes"), (CHAR *)var->addr);
			us_writetext(neww->uselx, neww->usehx, neww->usehy-60, neww->usehy-30, &us_hbox,
				returninfstr(infstr), descript, neww, NOTECHNOLOGY);
		}
	}
}

/*
 * Routine to switch technologies.  If "forcedtechnology" is nonzero, switch to that
 * technology, otherwise determine technology from cell "cell".  If "always" is false,
 * only switch if the user has selected "auto-switching".
 */
void us_ensurepropertechnology(NODEPROTO *cell, CHAR *forcedtechnology, BOOLEAN always)
{
	REGISTER USERCOM *uc;
	REGISTER TECHNOLOGY *tech;
	REGISTER WINDOWFRAME *curframe, *lastframe;
	REGISTER void *infstr;

	/* if not auto-switching technologies, quit */
	if ((us_useroptions&AUTOSWITCHTECHNOLOGY) == 0) return;

	/* see if cell has a technology */
	if (cell == NONODEPROTO) return;
	if ((cell->cellview->viewstate&TEXTVIEW) != 0) return;
	tech = cell->tech;
	if (tech == gen_tech) return;
	if ((tech->userbits&NOPRIMTECHNOLOGY) != 0) return;

	/* switch technologies */
	lastframe = getwindowframe(TRUE);
	if (us_getmacro(x_("pmtesetup")) == NOVARIABLE) return;
	infstr = initinfstr();
	formatinfstr(infstr, x_("pmtesetup \"%s\""), us_techname(cell));
	uc = us_makecommand(returninfstr(infstr));
	if (uc != NOUSERCOM)
	{
		us_execute(uc, FALSE, FALSE, FALSE);
		us_freeusercom(uc);
	}
	curframe = getwindowframe(TRUE);
	if (curframe != lastframe && lastframe != NOWINDOWFRAME)
		bringwindowtofront(lastframe);
}

/*
 * routine to initialize for changes to the screen
 */
void us_beginchanges(void)
{
}

/*
 * routine to finish making changes to the screen.  If "which" is NOWINDOWPART,
 * changes are made to all windows.  Otherwise, "which" is the specific
 * window to update.
 */
void us_endchanges(WINDOWPART *which)
{
	REGISTER WINDOWPART *w;
	XARRAY rottrans;
	REGISTER REDRAW *r, *nextr;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp;
	REGISTER INTBIG stopped;
	REGISTER INTBIG ret;

	stopped = el_pleasestop;
	for(r = us_firstredraw; r != NOREDRAW; r = nextr)
	{
		/* remember the next redraw module and queue this one for deletion */
		nextr = r->nextredraw;
		r->nextredraw = us_usedredraw;
		us_usedredraw = r;
		if (stopped != 0) continue;

		if (r->entryisnode)
		{
			ni = r->entryaddr.ni;
			if ((ni->userbits & (REWANTN|RETDONN|DEADN)) == REWANTN)
			{
				if (which != NOWINDOWPART)
				{
					if (ni->rotation == 0 && ni->transpose == 0)
					{
						ret = us_showin(which, ni->geom, ni->parent, el_matid, LAYERA, 1);
					} else
					{
						makerot(ni, rottrans);
						ret = us_showin(which, ni->geom, ni->parent, rottrans, LAYERA, 1);
					}
					if (ret < 0) stopped = 1; else
						if (ret & 2) us_queueopaque(ni->geom, FALSE);
				} else
				{
					for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
					{
						if ((w->state&WINDOWTYPE) != DISPWINDOW &&
							(w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
						if (ni->rotation == 0 && ni->transpose == 0)
						{
							ret = us_showin(w, ni->geom, ni->parent, el_matid, LAYERA, 1);
						} else
						{
							makerot(ni, rottrans);
							ret = us_showin(w, ni->geom, ni->parent, rottrans, LAYERA, 1);
						}
						if (ret < 0) { stopped = 1;   break; }
						if (ret & 2) us_queueopaque(ni->geom, FALSE);
					}
				}
				ni->userbits |= RETDONN;
			}
		} else
		{
			ai = r->entryaddr.ai;
			if ((ai->userbits & (RETDONA|DEADA)) == 0)
			{
				if (which != NOWINDOWPART)
				{
					ret = us_showin(which, ai->geom, ai->parent, el_matid, LAYERA, 1);
					if (ret < 0) stopped = 1; else
						if (ret & 2) us_queueopaque(ai->geom, FALSE);
				} else
				{
					for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
					{
						if ((w->state&WINDOWTYPE) != DISPWINDOW &&
							(w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
						ret = us_showin(w, ai->geom, ai->parent, el_matid, LAYERA, 1);
						if (ret < 0) { stopped = 1;   break; }
						if (ret & 2) us_queueopaque(ai->geom, FALSE);
					}
				}
				ai->userbits |= RETDONA;
			}
		}
	}
	us_firstredraw = NOREDRAW;

	/* now re-draw the opaque objects */
	for(r = us_firstopaque; r != NOREDRAW; r = nextr)
	{
		/* remember the next redraw module and queue this one for deletion */
		nextr = r->nextredraw;
		r->nextredraw = us_usedredraw;
		us_usedredraw = r;

		if (stopped != 0) continue;
		if (r->entryisnode)
		{
			ni = r->entryaddr.ni;
			if ((ni->userbits & (REWANTN|REODONN|DEADN)) == REWANTN)
			{
				if (which != NOWINDOWPART)
				{
					if (ni->rotation == 0 && ni->transpose == 0)
					{
						ret = us_showin(which, ni->geom, ni->parent, el_matid, LAYERA, 2);
					} else
					{
						makerot(ni, rottrans);
						ret = us_showin(which, ni->geom, ni->parent, rottrans, LAYERA, 2);
					}
					if (ret < 0) stopped = 1;
				} else
				{
					for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
					{
						if ((w->state&WINDOWTYPE) != DISPWINDOW &&
							(w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
						if (ni->rotation == 0 && ni->transpose == 0)
						{
							ret = us_showin(w, ni->geom, ni->parent, el_matid, LAYERA, 2);
						} else
						{
							makerot(ni, rottrans);
							ret = us_showin(w, ni->geom, ni->parent, rottrans, LAYERA, 2);
						}
						if (ret < 0) { stopped = 1;   break; }
					}
				}
				ni->userbits |= REODONN;
			}
		} else
		{
			ai = r->entryaddr.ai;
			if ((ai->userbits & (REODONA|DEADA)) == 0)
			{
				if (which != NOWINDOWPART)
				{
					ret = us_showin(which, ai->geom, ai->parent, el_matid, LAYERA, 2);
					if (ret < 0) stopped = 1;
				} else
				{
					for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
					{
						if ((w->state&WINDOWTYPE) != DISPWINDOW &&
							(w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
						ret = us_showin(w, ai->geom, ai->parent, el_matid, LAYERA, 2);
						if (ret < 0) { stopped = 1;   break; }
					}
				}
				ai->userbits |= REODONA;
			}
		}
	}

	us_firstopaque = NOREDRAW;
	us_lastopaque = NOREDRAW;

	/* now free up all of the redraw modules */
	for(r = us_usedredraw; r != NOREDRAW; r = nextr)
	{
		nextr = r->nextredraw;
		if (r->entryisnode)
		{
			ni = r->entryaddr.ni;
			ni->userbits &= ~(REWANTN|RELOCLN);
		} else
		{
			ai = r->entryaddr.ai;
			ai->userbits &= ~(REWANTA|RELOCLA);
		}
		us_freeredraw(r);
	}
	us_usedredraw = NOREDRAW;

	/* now re-draw the exports */
	for(r = us_firstexport; r != NOREDRAW; r = nextr)
	{
		/* remember the next redraw module and queue this one for deletion */
		nextr = r->nextredraw;
		pp = r->entryaddr.pp;
		us_freeredraw(r);
		if (stopped != 0) continue;
		makerot(pp->subnodeinst, rottrans);

		if (which != NOWINDOWPART)
		{
			us_writeprotoname(pp, LAYERA, rottrans, LAYERO, el_colcelltxt, which, 0, 0, 0, 0,
				(us_useroptions&EXPORTLABELS)>>EXPORTLABELSSH);
			us_drawportprotovariables(pp, LAYERA, rottrans, which, FALSE);
		} else
		{
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
			{
				if ((w->state&WINDOWTYPE) != DISPWINDOW &&
					(w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
				if (w->curnodeproto != pp->parent) continue;
				us_writeprotoname(pp, LAYERA, rottrans, LAYERO, el_colcelltxt, w, 0, 0, 0, 0,
					(us_useroptions&EXPORTLABELS)>>EXPORTLABELSSH);
				us_drawportprotovariables(pp, LAYERA, rottrans, w, FALSE);
			}
		}
	}
	us_firstexport = NOREDRAW;

	/* force all changes out */
	flushscreen();
}

/*
 * routine to make a change to the screen (sandwiched between a "us_beginchanges"
 * and an "us_endchanges" call.  Returns an indicator of what else needs to be
 * drawn (negative to stop display).
 */
INTBIG us_showin(WINDOWPART *w, GEOM *object, NODEPROTO *parnt, XARRAY trans, INTBIG on,
	INTBIG layers)
{
	REGISTER NODEINST *ni;
	XARRAY localtran, subrot, subtran;
	REGISTER WINDOWPART *oldwin;
	REGISTER INTBIG moretodo, res;
	REGISTER INTBIG objlocal;

	/* if the parent is the current cell in the window, draw the instance */
	moretodo = 0;
	oldwin = setvariablewindow(w);
	if (parnt == w->curnodeproto)
	{
		if (object->entryisnode)
		{
			ni = object->entryaddr.ni;
			begintraversehierarchy();
			res = us_drawnodeinst(ni, on, trans, layers, w);
			endtraversehierarchy();
			if (res == -2)
			{
				(void)setvariablewindow(oldwin);
				return(-1);
			}
			if (res < 0) res = 0;
			moretodo |= res;
		} else
		{
			res = us_drawarcinst(object->entryaddr.ai, on, trans, layers, w);
			if (res < 0)
			{
				(void)setvariablewindow(oldwin);
				return(-1);
			}
			moretodo |= res;
		}

		/* if drawing the opaque layer, don't look for other things to draw */
		if (on && layers == 2)
		{
			(void)setvariablewindow(oldwin);
			return(moretodo);
		}

		/* queue re-drawing of objects in vicinity of this one */
		if (object->entryisnode)
			us_queuevicinity(w, object, parnt, trans, 0, 0, 0, 0, on); else
				us_queuevicinity(w, object, parnt, trans, 0, 0, 0, 0, on);
		(void)setvariablewindow(oldwin);
		return(moretodo);
	}

	/* look at all instances of the parent cell that are expanded */
	if (object->entryisnode)
		objlocal = object->entryaddr.ni->userbits & RELOCLN; else
			objlocal = object->entryaddr.ai->userbits & RELOCLA;

	/* Steve Holmlund of Factron suggested this next statement */
	if (on == 0) objlocal = 0;

	for(ni = parnt->firstinst; ni != NONODEINST; ni = ni->nextinst)
	{
		if ((ni->userbits & NEXPAND) == 0) continue;
		if (objlocal != 0 && (ni->userbits&(RELOCLN|REWANTN)) == 0) continue;

		/* transform nodeinst to outer instance */
		maketrans(ni, localtran);
		transmult(trans, localtran, subrot);
		if (ni->rotation == 0 && ni->transpose == 0)
		{
			res = us_showin(w, object, ni->parent, subrot, on, layers);
			if (res < 0)
			{
				(void)setvariablewindow(oldwin);
				return(-1);
			}
			moretodo |= res;
		} else
		{
			makerot(ni, localtran);
			transmult(subrot, localtran, subtran);
			res = us_showin(w, object, ni->parent, subtran, on, layers);
			if (res < 0)
			{
				(void)setvariablewindow(oldwin);
				return(-1);
			}
			moretodo |= res;
		}
	}
	(void)setvariablewindow(oldwin);
	return(moretodo);
}

/*
 * routine to queue object "p" to be re-drawn.  If "local" is true, only
 * draw the local instance of this object, not every instance.
 */
void us_queueredraw(GEOM *p, BOOLEAN local)
{
	REGISTER REDRAW *r;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;

	if (p->entryisnode)
	{
		ni = p->entryaddr.ni;
		ni->userbits = (ni->userbits & ~(RETDONN|REODONN|RELOCLN)) | REWANTN;
		if (local) ni->userbits |= RELOCLN;
	} else
	{
		ai = p->entryaddr.ai;
		ai->userbits = (ai->userbits & ~(RETDONA|REODONA|RELOCLA)) | REWANTA;
		if (local) ai->userbits |= RELOCLA;
	}

	/* queue this object for being re-drawn */
	r = us_allocredraw();
	if (r == NOREDRAW)
	{
		ttyputnomemory();
		return;
	}
	r->entryisnode = p->entryisnode;
	r->entryaddr.blind = p->entryaddr.blind;
	r->nextredraw = us_firstredraw;
	us_firstredraw = r;
}

/*
 * Routine to remove all redraw objects that are in library "lib"
 * (because the library has been deleted)
 */
void us_unqueueredraw(LIBRARY *lib)
{
	REGISTER REDRAW *r, *nextr, *lastr;
	REGISTER LIBRARY *rlib;

	lastr = NOREDRAW;
	for(r = us_firstredraw; r != NOREDRAW; r = nextr)
	{
		nextr = r->nextredraw;
		if (r->entryisnode) rlib = r->entryaddr.ni->parent->lib; else
			rlib = r->entryaddr.ai->parent->lib;
		if (rlib == lib)
		{
			if (lastr == NOREDRAW) us_firstredraw = nextr; else
				lastr->nextredraw = nextr;
			us_freeredraw(r);
			continue;
		}
		lastr = r;
	}
}

/*
 * routine to erase the display of "geom" in all windows (sandwiched between
 * "us_beginchanges" and "us_endchanges" calls)
 */
void us_undisplayobject(GEOM *geom)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER WINDOWPART *w;
	XARRAY rottrans;

	if (geom->entryisnode)
	{
		ni = geom->entryaddr.ni;
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if ((w->state&WINDOWTYPE) != DISPWINDOW &&
				(w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
			if (ni->rotation == 0 && ni->transpose == 0)
			{
				(void)us_showin(w, geom, ni->parent, el_matid, LAYERN, 3);
			} else
			{
				makerot(ni, rottrans);
				(void)us_showin(w, geom, ni->parent, rottrans, LAYERN, 3);
			}
		}
	} else
	{
		ai = geom->entryaddr.ai;
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if ((w->state&WINDOWTYPE) != DISPWINDOW &&
				(w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
			(void)us_showin(w, geom, ai->parent, el_matid, LAYERN, 3);
		}
	}
}

/*
 * routine to queue object "p" to be re-drawn.
 */
void us_queueexport(PORTPROTO *pp)
{
	REGISTER REDRAW *r;

	/* queue this object for being re-drawn */
	r = us_allocredraw();
	if (r == NOREDRAW)
	{
		ttyputnomemory();
		return;
	}
	r->entryaddr.pp = pp;
	r->nextredraw = us_firstexport;
	us_firstexport = r;
}

/******************** HELPER ROUTINES ******************/

/*
 * routine to queue the opaque layers of object "p" to be re-drawn.
 * If "local" is true, only draw the local instance of this object, not
 * every instance.
 */
void us_queueopaque(GEOM *p, BOOLEAN local)
{
	REGISTER REDRAW *r;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER ARCINST *ai;
	REGISTER TECHNOLOGY *tech;
	REGISTER INTBIG i;

	if (p->entryisnode)
	{
		ni = p->entryaddr.ni;

		/* count displayable variables on this node */
		for(i = 0; i < ni->numvar; i++)
			if ((ni->firstvar[i].type&VDISPLAY) != 0) break;

		/* if this nodeinst has nothing on the opaque layer, don't queue it */
		np = ni->proto;
		if (np->primindex != 0 && (np->userbits&NHASOPA) == 0 &&
			i >= ni->numvar && ni->firstportexpinst == NOPORTEXPINST) return;

		/* set the nodeinst bits for opaque redraw */
		ni->userbits = (ni->userbits & ~REODONN) | REWANTN;
		if (local) ni->userbits |= RELOCLN;
		tech = np->tech;
	} else
	{
		ai = p->entryaddr.ai;

		/* count displayable variables on this arc */
		for(i = 0; i < ai->numvar; i++)
			if ((ai->firstvar[i].type&VDISPLAY) != 0) break;

		/* if this arcinst has nothing on the opaque layer, don't queue it */
		if ((ai->proto->userbits&AHASOPA) == 0 && i >= ai->numvar) return;

		/* set the arcinst bits for opaque redraw */
		ai->userbits = (ai->userbits & ~REODONA) | REWANTA;
		if (local) ai->userbits |= RELOCLA;
		tech = ai->proto->tech;
	}

	/* queue the object for opaque redraw */
	r = us_allocredraw();
	if (r == NOREDRAW)
	{
		ttyputnomemory();
		return;
	}
	r->entryisnode = p->entryisnode;
	r->entryaddr.blind = p->entryaddr.blind;
	if (tech == el_curtech)
	{
		/* in the current technology: queue redraw of this stuff first */
		r->nextredraw = us_firstopaque;
		us_firstopaque = r;
		if (us_lastopaque == NOREDRAW) us_lastopaque = r;
	} else
	{
		/* in some other technology: queue redraw of this stuff last */
		if (us_lastopaque != NOREDRAW) us_lastopaque->nextredraw = r;
		r->nextredraw = NOREDRAW;
		us_lastopaque = r;
		if (us_firstopaque == NOREDRAW) us_firstopaque = r;
	}
}

/*
 * routine to allocate a new redraw from the pool (if any) or memory
 * routine returns NOREDRAW upon error
 */
REDRAW *us_allocredraw(void)
{
	REGISTER REDRAW *r;

	if (us_redrawfree == NOREDRAW)
	{
		r = (REDRAW *)emalloc(sizeof (REDRAW), us_tool->cluster);
		if (r == 0) return(NOREDRAW);
	} else
	{
		/* take module from free list */
		r = us_redrawfree;
		us_redrawfree = r->nextredraw;
	}
	return(r);
}

/*
 * routine to return redraw module "r" to the pool of free modules
 */
void us_freeredraw(REDRAW *r)
{
	r->nextredraw = us_redrawfree;
	us_redrawfree = r;
}

/*
 * routine to queue for local re-draw anything in cell "parnt" that is
 * in the vicinity of "object" with bounding box (lx-hx, ly-hy).  The objects
 * are to be drawn on if "on" is nonzero.  The transformation matrix
 * between the bounding box and the cell is in "trans".
 */
void us_queuevicinity(WINDOWPART *w, GEOM *object, NODEPROTO *parnt, XARRAY trans, INTBIG lx,
	INTBIG hx, INTBIG ly, INTBIG hy, INTBIG on)
{
	REGISTER GEOM *look;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	INTBIG bx, ux, by, uy, olx, ohx, oly, ohy;
	REGISTER BOOLEAN local;
	REGISTER INTBIG i, search, slop;

	/* compute full extent of the object, including text */
	if (object != NOGEOM)
	{
		if (object->entryisnode)
		{
			us_getnodebounds(object->entryaddr.ni, &lx, &hx, &ly, &hy);
		} else
		{
			us_getarcbounds(object->entryaddr.ai, &lx, &hx, &ly, &hy);
		}
	}

	xform(lx, ly, &bx, &by, trans);
	xform(hx, hy, &ux, &uy, trans);
	if (bx > ux) { i = bx;  bx = ux;  ux = i; }
	if (by > uy) { i = by;  by = uy;  uy = i; }

	/* extend by 1 screen pixel */
	i = roundfloat(3.0f / w->scalex);
	if (i <= 0) i = 1;
	bx -= i;   hx += i;
	by -= i;   hy += i;

	/* clip search to visible area of window */
	if (bx > w->screenhx || ux < w->screenlx ||
		by > w->screenhy || uy < w->screenly) return;
	if (bx < w->screenlx) bx = w->screenlx;
	if (ux > w->screenhx) ux = w->screenhx;
	if (by < w->screenly) by = w->screenly;
	if (uy > w->screenhy) uy = w->screenhy;

	slop = FARTEXTLIMIT * lambdaofcell(parnt);
	search = initsearch(bx-slop, ux+slop, by-slop, uy+slop, parnt);
	if (object == NOGEOM) local = TRUE; else local = FALSE;
	for(;;)
	{
		if ((look = nextobject(search)) == NOGEOM) break;

		/* don't re-draw the current object whose vicinity is being checked */
		if (look == object) continue;

		/* see if the object really intersects the redraw area */
		if (look->entryisnode)
			us_getnodebounds(look->entryaddr.ni, &olx, &ohx, &oly, &ohy); else
				us_getarcbounds(look->entryaddr.ai, &olx, &ohx, &oly, &ohy);
		if (olx > ux || ohx < bx || oly > uy || ohy < by) continue;

		/* redraw all if object is going off, redraw opaque if going on */
		if (on == LAYERN) us_queueredraw(look, local); else
			us_queueopaque(look, local);
	}

	/* now queue things with "far text" */
	for(ni = parnt->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if ((ni->userbits&NHASFARTEXT) == 0) continue;
		us_getnodebounds(ni, &olx, &ohx, &oly, &ohy);
		if (olx >= ux || ohx <= bx || oly >= uy || ohy <= by) continue;
		if (on == LAYERN) us_queueredraw(ni->geom, local); else
			us_queueopaque(ni->geom, local);
	}
	for(ai = parnt->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if ((ai->userbits&AHASFARTEXT) == 0) continue;
		us_getarcbounds(ai, &olx, &ohx, &oly, &ohy);
		if (olx >= ux || ohx <= bx || oly >= uy || ohy <= by) continue;
		if (on == LAYERN) us_queueredraw(ai->geom, local); else
			us_queueopaque(ai->geom, local);
	}
}

/*
 * null routine for polygon display
 */
void us_nulldisplayroutine(POLYGON *obj, WINDOWPART *w)
{
}

/*
 * routine to write polygon "obj" in window "w".
 */
void us_showpoly(POLYGON *obj, WINDOWPART *w)
{
	REGISTER INTBIG i, cx, cy, rad;
	REGISTER INTBIG pre, tx, ty;
	INTBIG lx, ux, ly, uy, six, siy;
	WINDOWPART wsc;
	REGISTER GRAPHICS *gra;
	static POLYGON *objc = NOPOLYGON;
	INTBIG tsx, tsy;

	if (us_3dgatheringpolys)
	{
		us_3dshowpoly(obj, w);
		return;
	}

	/* quit if no bits to be written */
	if (obj->desc->bits == LAYERN) return;

	/* special case for grid display */
	if (obj->style == GRIDDOTS)
	{
		screendrawgrid(w, obj);
		return;
	}

	/* transform to space of this window */
	if ((w->state&INPLACEEDIT) != 0) xformpoly(obj, w->outofcell);

	/* now draw the polygon */
	gra = obj->desc;
	switch (obj->style)
	{
		case FILLED:		/* filled polygon */
		case FILLEDRECT:	/* filled rectangle */
			if (isbox(obj, &lx, &ux, &ly, &uy))
			{
				/* simple rectangular box: transform, clip, and draw */
				if (us_makescreen(&lx, &ly, &ux, &uy, w)) break;
				screendrawbox(w, lx, ux, ly, uy, gra);

				/* for patterned and outlined rectangles, draw the box too */
				if ((gra->colstyle&(NATURE|OUTLINEPAT)) == (PATTERNED|OUTLINEPAT))
				{
					screendrawline(w, lx, ly, lx, uy, gra, 0);
					screendrawline(w, lx, uy, ux, uy, gra, 0);
					screendrawline(w, ux, uy, ux, ly, gra, 0);
					screendrawline(w, ux, ly, lx, ly, gra, 0);
				}
				break;
			}

			/* code cannot be called by multiple procesors: uses globals */
			NOT_REENTRANT;

			/* copy the polygon since it will be mangled when clipped */
			(void)needstaticpolygon(&objc, obj->count, us_tool->cluster);
			objc->count = obj->count;
			objc->style = obj->style;
			objc->desc = gra;
			for(i=0; i<obj->count; i++)
			{
				objc->xv[i] = applyxscale(w, obj->xv[i]-w->screenlx) + w->uselx;
				objc->yv[i] = applyyscale(w, obj->yv[i]-w->screenly) + w->usely;
			}

			/* clip and draw the polygon */
			clippoly(objc, w->uselx, w->usehx, w->usely, w->usehy);
			if (objc->count <= 1) break;
			if (objc->count > 2)
			{
				/* always clockwise */
				if (areapoly(objc) < 0.0) reversepoly(objc);
				screendrawpolygon(w, objc->xv, objc->yv, objc->count, objc->desc);

				/* for patterned and outlined polygons, draw the outline too */
				if ((gra->colstyle&(NATURE|OUTLINEPAT)) == (PATTERNED|OUTLINEPAT))
				{
					for(i=0; i<objc->count; i++)
					{
						if (i == 0) pre = objc->count-1; else pre = i-1;
						screendrawline(w, objc->xv[pre], objc->yv[pre], objc->xv[i], objc->yv[i],
							objc->desc, 0);
					}
				}
			} else screendrawline(w, objc->xv[0], objc->yv[0], objc->xv[1], objc->yv[1],
				gra, 0);
			break;

		case CLOSEDRECT:		/* closed rectangle outline */
			us_wanttodraw(obj->xv[0], obj->yv[0], obj->xv[0], obj->yv[1], w, gra, 0);
			us_wanttodraw(obj->xv[0], obj->yv[1], obj->xv[1], obj->yv[1], w, gra, 0);
			us_wanttodraw(obj->xv[1], obj->yv[1], obj->xv[1], obj->yv[0], w, gra, 0);
			us_wanttodraw(obj->xv[1], obj->yv[0], obj->xv[0], obj->yv[0], w, gra, 0);
			break;

		case CROSSED:		/* polygon outline with cross */
			us_wanttodraw(obj->xv[0], obj->yv[0], obj->xv[2], obj->yv[2], w, gra, 0);
			us_wanttodraw(obj->xv[1], obj->yv[1], obj->xv[3], obj->yv[3], w, gra, 0);
			/* FALLTHROUGH */ 

		case CLOSED:		/* closed polygon outline */
			for(i=0; i<obj->count; i++)
			{
				if (i == 0) pre = obj->count-1; else pre = i-1;
				us_wanttodraw(obj->xv[pre], obj->yv[pre], obj->xv[i], obj->yv[i], w, gra, 0);
			}
			break;

		case OPENED:		/* opened polygon outline */
			for(i=1; i<obj->count; i++)
				us_wanttodraw(obj->xv[i-1], obj->yv[i-1], obj->xv[i], obj->yv[i], w, gra, 0);
			break;

		case OPENEDT1:		/* opened polygon outline, dotted */
			for(i=1; i<obj->count; i++)
				us_wanttodraw(obj->xv[i-1], obj->yv[i-1], obj->xv[i], obj->yv[i], w, gra, 1);
			break;

		case OPENEDT2:		/* opened polygon outline, dashed */
			for(i=1; i<obj->count; i++)
				us_wanttodraw(obj->xv[i-1], obj->yv[i-1], obj->xv[i], obj->yv[i], w, gra, 2);
			break;

		case OPENEDT3:		/* opened polygon outline, thicker */
			for(i=1; i<obj->count; i++)
				us_wanttodraw(obj->xv[i-1], obj->yv[i-1], obj->xv[i], obj->yv[i], w, gra, 3);
			break;

		case OPENEDO1:		/* extended opened polygon outline */
			us_drawextendedpolyline(obj, w);
			break;

		case VECTORS:		/* many lines */
			if (obj->count % 2 != 0)
				ttyputmsg(_("Cannot display vector with %ld vertices (must be even)"),
					obj->count);
			for(i=0; i<obj->count; i += 2)
				us_wanttodraw(obj->xv[i], obj->yv[i], obj->xv[i+1], obj->yv[i+1], w, gra, 0);
			break;

		case CROSS:		/* crosses (always have one point) */
		case BIGCROSS:
			getcenter(obj, &six, &siy);
			if (six < w->screenlx || six > w->screenhx || siy < w->screenly || siy > w->screenhy)
				break;
			if (obj->style == CROSS) i = CROSSSIZE; else i = BIGCROSSSIZE;
			us_wanttodrawo(six, -i, siy, 0, six, i, siy, 0, w, gra, 0);
			us_wanttodrawo(six, 0, siy, -i, six, 0, siy, i, w, gra, 0);
			break;

		case TEXTCENT:		/* text centered in box */
		case TEXTTOP:		/* text below top of box */
		case TEXTBOT:		/* text above bottom of box */
		case TEXTLEFT:		/* text right of left edge of box */
		case TEXTRIGHT:		/* text left of right edge of box */
		case TEXTTOPLEFT:	/* text to lower-right of upper-left corner */
		case TEXTBOTLEFT:	/* text to upper-right of lower-left corner */
		case TEXTTOPRIGHT:	/* text to lower-left of upper-right corner */
		case TEXTBOTRIGHT:	/* text to upper-left of lower-right corner */
			getbbox(obj, &lx, &ux, &ly, &uy);
			lx = applyxscale(w, lx-w->screenlx) + w->uselx;
			ly = applyyscale(w, ly-w->screenly) + w->usely;
			ux = applyxscale(w, ux-w->screenlx) + w->uselx;
			uy = applyyscale(w, uy-w->screenly) + w->usely;
			screensettextinfo(w, obj->tech, obj->textdescript);
			screengettextsize(w, obj->string, &tsx, &tsy);
			switch (obj->style)
			{
				case TEXTCENT:
					tx = (lx+ux-tsx) / 2;
					ty = (ly+uy-tsy) / 2;
					break;
				case TEXTTOP:
					tx = (lx+ux-tsx) / 2;
					ty = uy-tsy;
					break;
				case TEXTBOT:
					tx = (lx+ux-tsx) / 2;
					ty = ly;
					break;
				case TEXTLEFT:
					tx = lx;
					ty = (ly+uy-tsy) / 2;
					break;
				case TEXTRIGHT:
					tx = ux-tsx;
					ty = (ly+uy-tsy) / 2;
					break;
				case TEXTTOPLEFT:
					tx = lx;
					ty = uy-tsy;
					break;
				case TEXTBOTLEFT:
					tx = lx;
					ty = ly;
					break;
				case TEXTTOPRIGHT:
					tx = ux-tsx;
					ty = uy-tsy;
					break;
				case TEXTBOTRIGHT:
					tx = ux-tsx;
					ty = ly;
					break;
			}
			if (tx > w->usehx || tx+tsx < w->uselx ||
				ty > w->usehy || ty+tsy < w->usely) break;
			screendrawtext(w, tx, ty, obj->string, gra);
			break;

		case TEXTBOX:		/* text centered and contained in box */
			getbbox(obj, &lx, &ux, &ly, &uy);
			if (us_makescreen(&lx, &ly, &ux, &uy, w)) break;
			us_writetext(lx, ux, ly, uy, gra, obj->string, obj->textdescript, w, obj->tech);
			break;

		case CIRCLE:    case THICKCIRCLE:
		case CIRCLEARC: case THICKCIRCLEARC:
			/* must scale the window for best precision when drawing curves */
			wsc.screenlx = w->screenlx;
			wsc.screenly = w->screenly;
			wsc.screenhx = w->screenhx;
			wsc.screenhy = w->screenhy;
			wsc.uselx = ZUP(w->uselx);
			wsc.usely = ZUP(w->usely);
			wsc.usehx = ZUP(w->usehx);
			wsc.usehy = ZUP(w->usehy);
			computewindowscale(&wsc);

			/* code cannot be called by multiple procesors: uses globals */
			NOT_REENTRANT;

			/* get copy polygon */
			(void)needstaticpolygon(&objc, obj->count, us_tool->cluster);

			/* transform and copy the polygon */
			objc->count = obj->count;
			objc->style = obj->style;
			for(i=0; i<obj->count; i++)
			{
				objc->xv[i] = applyxscale(&wsc, obj->xv[i]-wsc.screenlx) + wsc.uselx;
				objc->yv[i] = applyyscale(&wsc, obj->yv[i]-wsc.screenly) + wsc.usely;
			}

			/* clip the circle */
			cliparc(objc, wsc.uselx, wsc.usehx, wsc.usely, wsc.usehy);

			/* circle outline at [0] radius to [1] */
			switch (objc->style)
			{
				case CIRCLE:
					if (objc->count != 2) break;
					six = applyxscale(w, obj->xv[1]-obj->xv[0]);
					siy = applyxscale(w, obj->yv[1]-obj->yv[0]);
					rad = computedistance(0, 0, six, siy);
					screendrawcircle(w, ZDN(objc->xv[0]), ZDN(objc->yv[0]), rad, gra);
					break;
				case THICKCIRCLE:
					if (objc->count != 2) break;
					six = applyxscale(w, obj->xv[1]-obj->xv[0]);
					siy = applyxscale(w, obj->yv[1]-obj->yv[0]);
					rad = computedistance(0, 0, six, siy);
					screendrawthickcircle(w, ZDN(objc->xv[0]), ZDN(objc->yv[0]), rad, gra);
					break;
				case CIRCLEARC:
					/* thin arcs at [i] points [1+i] [2+i] clockwise */
					if (objc->count == 0) break;
					if ((objc->count%3) != 0) break;
					for (i=0; i<objc->count; i += 3)
						screendrawcirclearc(w, ZDN(objc->xv[i]), ZDN(objc->yv[i]), ZDN(objc->xv[i+1]),
							ZDN(objc->yv[1+i]), ZDN(objc->xv[i+2]), ZDN(objc->yv[i+2]), gra);
					break;
				case THICKCIRCLEARC:
					/* thick arcs at [i] points [1+i] [2+i] clockwise */
					if (objc->count == 0) break;
					if ((objc->count%3) != 0) break;
					for (i=0; i<objc->count; i += 3)
						screendrawthickcirclearc(w, ZDN(objc->xv[i]), ZDN(objc->yv[i]), ZDN(objc->xv[i+1]),
							ZDN(objc->yv[1+i]), ZDN(objc->xv[i+2]), ZDN(objc->yv[i+2]), gra);
			}
			break;

		case DISC:
			/* filled circle at [0] radius to [1] */
			if (obj->count != 2) break;
			cx = applyxscale(w, obj->xv[0]-w->screenlx) + w->uselx;
			cy = applyyscale(w, obj->yv[0]-w->screenly) + w->usely;
			six = applyxscale(w, obj->xv[1]-obj->xv[0]);
			siy = applyxscale(w, obj->yv[1]-obj->yv[0]);
			rad = computedistance(0, 0, six, siy);
			if (rad == 0) break;

			/* clip if completely off screen */
			if (cx + rad < w->uselx || cx - rad > w->usehx ||
				cy + rad < w->usely || cy - rad > w->usehy)
					break;
			screendrawdisc(w, cx, cy, rad, gra);
			break;
	}

	/* transform from space of this window */
	if ((w->state&INPLACEEDIT) != 0) xformpoly(obj, w->intocell);
}

/*
 * routine to clip and possibly draw a line from (fx,fy) to (tx,ty) in
 * window "w" with description "desc", texture "texture"
 */
void us_wanttodraw(INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty, WINDOWPART *w, GRAPHICS *desc,
	INTBIG texture)
{
	fx = applyxscale(w, fx-w->screenlx) + w->uselx;
	fy = applyyscale(w, fy-w->screenly) + w->usely;
	tx = applyxscale(w, tx-w->screenlx) + w->uselx;
	ty = applyyscale(w, ty-w->screenly) + w->usely;
	if (clipline(&fx, &fy, &tx, &ty, w->uselx, w->usehx, w->usely, w->usehy)) return;
	screendrawline(w, fx, fy, tx, ty, desc, texture);
}

/*
 * routine to clip and possibly draw a line from (fx,fy) to (tx,ty) in
 * window "w" with description "desc", texture "texture"
 */
void us_wanttodrawo(INTBIG fx, INTBIG fxo, INTBIG fy, INTBIG fyo, INTBIG tx, INTBIG txo,
	INTBIG ty, INTBIG tyo, WINDOWPART *w, GRAPHICS *desc, INTBIG texture)
{
	fx = applyxscale(w, fx-w->screenlx) + w->uselx + fxo;
	fy = applyyscale(w, fy-w->screenly) + w->usely + fyo;
	tx = applyxscale(w, tx-w->screenlx) + w->uselx + txo;
	ty = applyyscale(w, ty-w->screenly) + w->usely + tyo;
	if (clipline(&fx, &fy, &tx, &ty, w->uselx, w->usehx, w->usely, w->usehy)) return;
	screendrawline(w, fx, fy, tx, ty, desc, texture);
}

/*
 * routine to draw an opened polygon full of lines, set out by 1 pixel.  The polygon is in "obj".
 */
void us_drawextendedpolyline(POLYGON *obj, WINDOWPART *w)
{
	REGISTER INTBIG i;
	REGISTER INTBIG x1, y1, x2, y2, x1o, y1o, x2o, y2o, centerx, centery, diff;
	INTBIG lx, hx, ly, hy;

	/* if polygon is a line, extension is easy */
	if (isbox(obj, &lx, &hx, &ly, &hy))
	{
		if (lx == hx)
		{
			us_wanttodrawo(lx, -1, ly, 0, lx, -1, hy, 0, w, obj->desc, 0);
			us_wanttodrawo(lx, 1, ly, 0, lx, 1, hy, 0, w, obj->desc, 0);
			return;
		}
		if (ly == hy)
		{
			us_wanttodrawo(lx, -1, ly, 0, hx, -1, ly, 0, w, obj->desc, 0);
			us_wanttodrawo(lx, 1, ly, 0, hx, 1, ly, 0, w, obj->desc, 0);
			return;
		}
	}

	if (obj->count == 3 && obj->xv[0] == obj->xv[2] && obj->yv[0] == obj->yv[2])
	{
		x1 = obj->xv[0];   y1 = obj->yv[0];
		x2 = obj->xv[1];   y2 = obj->yv[1];
		if (x1 == x2)
		{
			us_wanttodrawo(x1,-1, y1, 0, x2,-1, y2, 0, w, obj->desc, 0);
			us_wanttodrawo(x1, 1, y1, 0, x2, 1, y2, 0, w, obj->desc, 0);
			return;
		}
		if (y1 == y2)
		{
			us_wanttodrawo(x1, 0, y1,-1, x2, 0, y2,-1, w, obj->desc, 0);
			us_wanttodrawo(x1, 0, y1,1, x2, 0, y2, 1, w, obj->desc, 0);
			return;
		}
		if ((x1-x2) * (y1-y2) > 0)
		{
			us_wanttodrawo(x1,1, y1,-1, x2,1, y2,-1, w, obj->desc, 0);
			us_wanttodrawo(x1,-1, y1,1, x2,-1, y2,1, w, obj->desc, 0);
		} else
		{
			us_wanttodrawo(x1,1, y1,1, x2,1, y2,1, w, obj->desc, 0);
			us_wanttodrawo(x1,-1, y1,-1, x2,-1, y2,-1, w, obj->desc, 0);
		}
		return;
	}

	/* do extension about polygon (and see if the polygon is a single point) */
	centerx = centery = diff = 0;
	for(i=0; i<obj->count; i++)
	{
		centerx += obj->xv[i];   centery += obj->yv[i];
		if (obj->xv[i] != obj->xv[0]) diff++;
		if (obj->yv[i] != obj->yv[0]) diff++;
	}
	centerx /= obj->count;   centery /= obj->count;

	/* special case if a single point */
	if (diff == 0)
	{
		us_wanttodrawo(centerx, -1, centery, -1, centerx, 1, centery, -1, w, obj->desc, 0);
		us_wanttodrawo(centerx, 1, centery, -1, centerx, 1, centery, 1, w, obj->desc, 0);
		us_wanttodrawo(centerx, 1, centery, 1, centerx, -1, centery, 1, w, obj->desc, 0);
		us_wanttodrawo(centerx, -1, centery, 1, centerx, -1, centery, -1, w, obj->desc, 0);
		return;
	}

	for(i=1; i<obj->count; i++)
	{
		x1 = obj->xv[i-1];   y1 = obj->yv[i-1];
		x2 = obj->xv[i];     y2 = obj->yv[i];
		if (x1 < centerx) x1o = -1; else
			if (x1 > centerx) x1o = 1; else x1o = 0;
		if (y1 < centery) y1o = -1; else
			if (y1 > centery) y1o = 1; else y1o = 0;
		if (x2 < centerx) x2o = -1; else
			if (x2 > centerx) x2o = 1; else x2o = 0;
		if (y2 < centery) y2o = -1; else
			if (y2 > centery) y2o = 1; else y2o = 0;
		us_wanttodrawo(x1, x1o, y1, y1o, x2, x2o, y2, y2o, w, obj->desc,
			0);
	}
}

/*
 * Write text in a box.  The box ranges from "lx" to "ux" in X and
 * from "ly" to "uy" in Y.  Draw in bit planes "desc->bits" with color
 * "desc->color".  Put "txt" there (or as much as will fit).  The value of
 * "initialfont" is the default size of text which will be reduced until it
 * can fit.
 */
void us_writetext(INTBIG lx, INTBIG ux, INTBIG ly, INTBIG uy, GRAPHICS *desc, CHAR *txt,
	UINTBIG *initdescript, WINDOWPART *win, TECHNOLOGY *tech)
{
	REGISTER INTBIG stop, save, xabssize, yabssize, abssize, newsize, oldsize;
	INTBIG six, siy;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	/* scan for a font that fits */
	TDCOPY(descript, initdescript);
	for(;;)
	{
		screensettextinfo(win, tech, descript);
		screengettextsize(win, txt, &six, &siy);
		if (six <= ux-lx && siy <= uy-ly) break;

		oldsize = TDGETSIZE(descript);
		abssize = TXTGETPOINTS(oldsize);
		if (abssize != 0)
		{
			/* jump quickly to the proper font size */
			if (six <= ux-lx) xabssize = abssize; else
				xabssize = abssize * (ux-lx) / six;
			if (siy <= uy-ly) yabssize = abssize; else
				yabssize = abssize * (uy-ly) / siy;
			newsize = mini(xabssize, yabssize);
			if (newsize < 4) return;
			TDSETSIZE(descript, TXTSETPOINTS(newsize));
		} else
		{
			newsize = TXTGETQLAMBDA(TDGETSIZE(descript)) - 1;
			if (newsize <= 0) return;
			TDSETSIZE(descript, TXTSETQLAMBDA(newsize));
		}
	}

	/* if the text doesn't fit in Y, quit */
	if (siy > uy-ly) return;

	/* truncate in X if possible */
	if (six > ux-lx)
	{
		stop = (ux-lx) * estrlen(txt);
		stop /= six;
		if (stop == 0) return;
		save = txt[stop];   txt[stop] = 0;
		screengettextsize(win, txt, &six, &siy);
	} else stop = -1;

	/* draw the text */
	screendrawtext(win, lx+(ux-lx-six)/2, ly+(uy-ly-siy)/2, txt, desc);
	if (stop >= 0) txt[stop] = (CHAR)save;
}

/******************** WINDOW PANNING ********************/

/*
 * routine to slide the contents of the current window up by "dist" lambda
 * units (slides down if "dist" is negative)
 */
void us_slideup(INTBIG dist)
{
	/* save and erase highlighting */
	us_pushhighlight();
	us_clearhighlightcount();

	/* set the new window data */
	startobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);
	(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenly"), el_curwindowpart->screenly - dist, VINTEGER);
	(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenhy"), el_curwindowpart->screenhy - dist, VINTEGER);
	endobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);

	/* restore highlighting */
	us_pophighlight(FALSE);
}

/*
 * routine to slide the current window left by "dist" lambda units
 * (slides right if "dist" is negative)
 */
void us_slideleft(INTBIG dist)
{
	/* save and erase highlighting */
	us_pushhighlight();
	us_clearhighlightcount();

	startobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);
	(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenlx"), el_curwindowpart->screenlx + dist, VINTEGER);
	(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("screenhx"), el_curwindowpart->screenhx + dist, VINTEGER);
	endobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);

	/* restore highlighting */
	us_pophighlight(FALSE);
}

/******************** TRANSFORMATION TO SCREEN ********************/

/*
 * routine to convert the reference parameters (lx,ux, ly,uy)
 * which define a box to screen co-ordinates ready to plot.
 * The values are scaled to screen space of window "w" and clipped.
 * If the routine returns true, the box is all off the screen.
 */
BOOLEAN us_makescreen(INTBIG *lx, INTBIG *ly, INTBIG *ux, INTBIG *uy, WINDOWPART *w)
{
	/* transform to screen space */
	if (*ux < w->screenlx || *lx > w->screenhx) return(TRUE);
	if (*uy < w->screenly || *ly > w->screenhy) return(TRUE);
	*lx = applyxscale(w, *lx-w->screenlx) + w->uselx;
	*ly = applyyscale(w, *ly-w->screenly) + w->usely;
	*ux = applyxscale(w, *ux-w->screenlx) + w->uselx;
	*uy = applyyscale(w, *uy-w->screenly) + w->usely;

	/* now clip to screen bounds */
	if (*lx < w->uselx) *lx = w->uselx;
	if (*ly < w->usely) *ly = w->usely;
	if (*ux > w->usehx) *ux = w->usehx;
	if (*uy > w->usehy) *uy = w->usehy;
	return(FALSE);
}

/******************** 3D DISPLAY ********************/

/*
 * Routine to setup the transformation matrix for 3D viewing.
 * Called once when the display is initially converted to 3D.
 */
void us_3dsetupviewing(WINDOWPART *w)
{
	REGISTER TECHNOLOGY *tech;
	REGISTER INTBIG i, lambda;
	float thickness, height, lowheight, highheight, scale, cx, cy, cz, sx, sy, sz;
	REGISTER NODEPROTO *np;
	REGISTER XFORM3D *xf3;

	/* determine height range */
	lowheight = 0.0;   highheight = -1.0;
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		for(i=0; i<tech->layercount; i++)
		{
			if (get3dfactors(tech, i, &height, &thickness)) continue;
			if (highheight < lowheight)
			{
				highheight = lowheight = height;
			} else
			{
				if (height < lowheight) lowheight = height;
				if (height > highheight) highheight = height;
			}
		}
	}

	/* setup initial camera */
	np = w->curnodeproto;
	if (np == NONODEPROTO) return;
	cx = (np->lowx + np->highx) / 2.0f;
	cy = (np->lowy + np->highy) / 2.0f;
	lambda = el_curlib->lambda[el_curtech->techindex];
	cz = (highheight + lowheight) / 2.0f * lambda;
	sx = (float)(np->highx - np->lowx);
	sy = (float)(np->highy - np->lowy);
	sz = (highheight - lowheight) * lambda;
	scale = (sx > sy) ? sx : sy;
	scale = (sz > scale) ? sz : scale;

	/* setup viewing parameters */
	xf3 = &w->xf3;
	xf3->fieldofview = 45.0f;
	xf3->eye[0] = -0.2f;   xf3->eye[1] = 0.2f;   xf3->eye[2] = 1.0f;
	vectornormalize3d(xf3->eye);
	vectormultiply3d(xf3->eye, scale, xf3->eye);
	xf3->eye[0] += cx;   xf3->eye[1] += cy;   xf3->eye[2] += cz;
	xf3->view[0] = cx;   xf3->view[1] = cy;   xf3->view[2] = cz;
	xf3->up[0] = 0.0;   xf3->up[1] = 1.0;   xf3->up[2] = 0.0;
	xf3->nearplane = 0.1f;   xf3->farplane = scale * 60.0f;
	xf3->screenx = (float)(w->usehx - w->uselx) / 2.0f;
	xf3->screeny = (float)(w->usehy - w->usely) / 2.0f;
	xf3->aspect = xf3->screenx / xf3->screeny;
	us_3dbuildtransform(xf3);
}

/*
 * Routine to fill window "win".
 */
void us_3dfillview(WINDOWPART *win)
{
	XFORM3D *xf3;
	float sx, sy, sz, maxs, toeye[3];

	xf3 = &win->xf3;

	sx = us_3dhighx - us_3dlowx;
	sy = us_3dhighy - us_3dlowy;
	sz = us_3dhighz - us_3dlowz;
	maxs = (sx > sy) ? sx : sy;
	maxs = (sz > maxs) ? sz : maxs;

	xf3->fieldofview = 45.0f;
	vectorsubtract3d(xf3->eye, xf3->view, toeye);
	xf3->view[0] = us_3dcenterx;  xf3->view[1] = us_3dcentery;  xf3->view[2] = us_3dcenterz;
	vectoradd3d(xf3->view, toeye, xf3->eye);
	xf3->nearplane = 0.1f;
	xf3->farplane = maxs * 60.0f;

	us_3dbuildtransform(xf3);
	us_3drender(win);
}

/*
 * Routine to zoom window "win" by a factor of "z".
 */
void us_3dzoomview(WINDOWPART *win, float z)
{
	XFORM3D *xf3;

	xf3 = &win->xf3;
	xf3->fieldofview = xf3->fieldofview * z;
	if (xf3->fieldofview > FOVMAX) xf3->fieldofview = FOVMAX;
	if (xf3->fieldofview < FOVMIN) xf3->fieldofview = FOVMIN;
	us_3dbuildtransform(xf3);
	us_3drender(win);
}

/*
 * Routine to pan window "win" by a factor of "x,y".
 */
void us_3dpanview(WINDOWPART *win, INTBIG x, INTBIG y)
{
	XFORM3D *xf3;
	float d[3], side[3], up[3], view[3], scale, e[3], offset[3];

	xf3 = &win->xf3;
	vectorsubtract3d(xf3->view, xf3->eye, d);
	vectorcross3d(d, xf3->up, side);
	vectorcross3d(d, side, up);
	vectornormalize3d(side);
	vectornormalize3d(up);
	vectorsubtract3d(xf3->view, xf3->eye, view);
	scale = vectormagnitude3d(view);
	vectormultiply3d(side, scale * 0.1f * (float)x, d);
	vectormultiply3d(up, scale * 0.1f * (float)y, e);
	vectoradd3d(d, e, offset);
	vectoradd3d(xf3->eye, offset, xf3->eye);
	vectoradd3d(xf3->view, offset, xf3->view);

	us_3dbuildtransform(xf3);
	us_3drender(win);
}

/*
 * Routine to build the 4x4 transformation matrix in "xf3" from
 * the viewing parameters there.
 */
void us_3dbuildtransform(XFORM3D *xf3)
{
	float f[3], s[3], u[3], persp[4][4], xform[4][4], trans[4][4], rot[4][4], ff;

	/* build the perspective transform */
	matrixid3d(persp);
	if ((us_useroptions&NO3DPERSPECTIVE) == 0)
	{
		ff = 1.0f / (float)tan((xf3->fieldofview * EPI / 180.0f) / 2.0f);
		persp[0][0] = ff / xf3->aspect;
		persp[1][1] = ff;
		persp[2][2] = (xf3->farplane + xf3->nearplane) / (xf3->nearplane - xf3->farplane);
		persp[2][3] = 2.0f * xf3->farplane * xf3->nearplane / (xf3->nearplane - xf3->farplane);
		persp[3][2] = -1.0f;
		persp[3][3] = 0.0f;
	} else
	{
		persp[0][0] = 45.0f / xf3->fieldofview;
		persp[1][1] = 45.0f / xf3->fieldofview;
		persp[2][2] = -1.0;
	}

	/* build the viewing transform */
	matrixid3d(trans);
	trans[0][3] = -xf3->eye[0];
	trans[1][3] = -xf3->eye[1];
	trans[2][3] = -xf3->eye[2];

	vectorsubtract3d(xf3->view, xf3->eye, f);
	vectornormalize3d(f);
	vectornormalize3d(xf3->up);
	vectorcross3d(f, xf3->up, s);
	vectorcross3d(s, f, u);
	matrixid3d(rot);
	rot[0][0] = s[0];   rot[0][1] = s[1];   rot[0][2] = s[2];
	rot[1][0] = u[0];   rot[1][1] = u[1];   rot[1][2] = u[2];
	rot[2][0] = -f[0];  rot[2][1] = -f[1];  rot[2][2] = -f[2];
	matrixmult3d(trans, rot, xform);

	/* build the transformation matrix */
	matrixmult3d(xform, persp, xf3->xform);
}

/*
 * Routine called at the start of drawing.
 */
void us_3dstartdrawing(WINDOWPART *win)
{
	us_3dgatheringpolys = TRUE;
	us_3dpolycount = 0;
	us_3dwindowpart = win;
}

/*
 * Helper routine for "us_3denddrawing()" that makes polygon depth go in ascending order
 */
int us_3dpolydepthascending(const void *e1, const void *e2)
{
	REGISTER POLY3D *c1, *c2;
	REGISTER float diff;

	c1 = *((POLY3D **)e1);
	c2 = *((POLY3D **)e2);
	diff = c1->depth - c2->depth;
	if (diff < 0.0) return(-1);
	if (diff > 0.0) return(1);
	return(0);
}

/*
 * Routine called at the end of drawing.
 */
void us_3denddrawing(void)
{
	REGISTER POLY3D *poly1;
	REGISTER INTBIG i, j;

	/* flush the opaque graphics out */
	us_endchanges(NOWINDOWPART);
	us_3dgatheringpolys = FALSE;

	/* sort the polygons */
	esort(us_3dpolylist, us_3dpolycount, sizeof (POLY3D *), us_3dpolydepthascending);

	/* determine bounding volume */
	for(i=0; i<us_3dpolycount; i++)
	{
		poly1 = us_3dpolylist[i];
		for(j=0; j<poly1->count; j++)
		{
			if (i == 0 && j == 0)
			{
				us_3dlowx = us_3dhighx = poly1->x[j];
				us_3dlowy = us_3dhighy = poly1->y[j];
				us_3dlowz = us_3dhighz = poly1->z[j];
			} else
			{
				if (poly1->x[j] < us_3dlowx) us_3dlowx = poly1->x[j];
				if (poly1->x[j] > us_3dhighx) us_3dhighx = poly1->x[j];
				if (poly1->y[j] < us_3dlowy) us_3dlowy = poly1->y[j];
				if (poly1->y[j] > us_3dhighy) us_3dhighy = poly1->y[j];
				if (poly1->z[j] < us_3dlowz) us_3dlowz = poly1->z[j];
				if (poly1->z[j] > us_3dhighz) us_3dhighz = poly1->z[j];
			}
		}
	}
	us_3dcenterx = (us_3dlowx + us_3dhighx) / 2.0f;
	us_3dcentery = (us_3dlowy + us_3dhighy) / 2.0f;
	us_3dcenterz = (us_3dlowz + us_3dhighz) / 2.0f;

	/* render it */
	us_3dbuildtransform(&us_3dwindowpart->xf3);
	us_3drender(us_3dwindowpart);
}

/*
 * Routine to re-render the polygons to window "w".
 */
void us_3drender(WINDOWPART *w)
{
	REGISTER INTBIG i, j, k, passes, lambda;
	INTBIG start[2], finish[2], incr[2];
	REGISTER INTBIG save, isneg;
	POLY3D *poly3d;
	float vec[4], res[4], res2[4];
	float zplane, res3[4];
	static POLYGON *poly = NOPOLYGON;
	REGISTER XFORM3D *xf3;

	xf3 = &w->xf3;
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* clear the screen */
	us_erasewindow(w);

	/* see if the view is from the back */
	lambda = el_curlib->lambda[el_curtech->techindex];
	for(zplane = us_3dhighz; zplane >= us_3dlowz; zplane -= lambda)
	{
		vec[0] = us_3dcenterx; vec[1] = us_3dcentery; vec[2] = zplane; vec[3] = 1.0;
		matrixxform3d(vec, xf3->xform, res);
		vec[0] = us_3dcenterx+1000.0f; vec[1] = us_3dcentery; vec[2] = zplane; vec[3] = 1.0;
		matrixxform3d(vec, xf3->xform, res2);
		vec[0] = us_3dcenterx; vec[1] = us_3dcentery+1000.0f; vec[2] = zplane; vec[3] = 1.0;
		matrixxform3d(vec, xf3->xform, res3);
		res[0] /= res[3];   res[1] /= res[3];   res[2] /= res[3];
		res2[0] /= res2[3];   res2[1] /= res2[3];   res2[2] /= res2[3];
		res3[0] /= res3[3];   res3[1] /= res3[3];   res3[2] /= res3[3];
		vectorsubtract3d(res2, res, res2);
		vectorsubtract3d(res3, res, res3);
		vectorcross3d(res2, res3, res);
		if (res[2] > 0) break;
	}
	if (zplane == us_3dhighz)
	{
		start[0] = 0;   finish[0] = us_3dpolycount;   incr[0] = 1;
		passes = 1;
	} else if (zplane < us_3dlowz)
	{
		start[0] = us_3dpolycount-1;   finish[0] = -1;   incr[0] = -1;
		passes = 1;
	} else
	{
		for(i=0; i<us_3dpolycount; i++)
			if (us_3dpolylist[i]->depth > zplane) break;
		start[0] = 0;                  finish[0] = i;	  incr[0] = 1;
		start[1] = us_3dpolycount-1;   finish[1] = i-1;   incr[1] = -1;
		passes = 2;
	}

	/* now draw it all */
	for(k=0; k<passes; k++)
	{
		for(i=start[k]; i != finish[k]; i = i + incr[k])
		{
			poly3d = us_3dpolylist[i];
			if (poly->limit < poly3d->count) (void)extendpolygon(poly, poly3d->count);
			isneg = 0;
			for(j=0; j<poly3d->count; j++)
			{
				vec[0] = poly3d->x[j];
				vec[1] = poly3d->y[j];
				vec[2] = poly3d->z[j];
				vec[3] = 1.0;
				matrixxform3d(vec, xf3->xform, res);
				if (res[2] < 0.0)
				{
					isneg = 1;
					break;
				}
				poly->xv[j] = (INTBIG)(res[0] / res[3] / 2.0 * xf3->screenx + xf3->screenx);
				poly->yv[j] = (INTBIG)(res[1] / res[3] / 2.0 * xf3->screeny + xf3->screeny);
				poly->xv[j] = roundfloat((poly->xv[j] - w->uselx) / w->scalex) + w->screenlx;
				poly->yv[j] = roundfloat((poly->yv[j] - w->usely) / w->scaley) + w->screenly;
			}
			if (isneg != 0) continue;
			poly->desc = poly3d->desc;
			poly->count = poly3d->count;
			poly->style = FILLED;
			save = poly3d->desc->bits;
			poly3d->desc->bits = LAYERA;
			(*us_displayroutine)(poly, w);
			poly3d->desc->bits = save;
		}
	}
}

void us_3dsetinteraction(INTBIG interaction)
{
	switch (interaction)
	{
		case 0: us_3dinteraction = ROTATEVIEW;  break;
		case 1: us_3dinteraction = ZOOMVIEW;    break;
		case 2: us_3dinteraction = PANVIEW;     break;
		case 3: us_3dinteraction = TWISTVIEW;   break;
	}
}

/*
 * button handler for 3D windows
 */
void us_3dbuttonhandler(WINDOWPART *w, INTBIG but, INTBIG x, INTBIG y)
{
	REGISTER XFORM3D *xf3;

	/* changes to the mouse-wheel are handled by the user interface */
	if (wheelbutton(but))
	{
		us_buttonhandler(w, but, x, y);
		return;
	}
	xf3 = &w->xf3;
	us_3dlastbutx = x;
	us_3dlastbuty = us_3dinitialbuty = y;
	switch (us_3dinteraction)
	{
		case ROTATEVIEW:
			break;
		case ZOOMVIEW:
			us_3dinitialfov = xf3->fieldofview;
			break;
		case PANVIEW:
			break;
		case TWISTVIEW:
			break;
	}
	trackcursor(FALSE, us_nullup, us_nullvoid, us_3deachdown, us_nullchar, us_nullvoid, TRACKNORMAL);
}

BOOLEAN us_3deachdown(INTBIG x, INTBIG y)
{
	float d[3], e[3], offset[3], side[3], up[3], view[3], scale;
	float angle, sinTheta, cosTheta, toDirection[3], tempVector1[3], tempVector2[3],
		tempVector3[3], rotateVector[3], toLength, newFrom[3], sinPhi, cosPhi,
		newToDirection[3], rightDirection[3], dot, lastangle, cx, cy;
	REGISTER XFORM3D *xf3;

	xf3 = &us_3dwindowpart->xf3;
	switch (us_3dinteraction)
	{
		case ROTATEVIEW:
			if (x == us_3dlastbutx && y == us_3dlastbuty) break;

			vectorsubtract3d(xf3->eye, xf3->view, toDirection);
			toLength = vectormagnitude3d(toDirection);
			vectornormalize3d(toDirection);

			/* Calculate orthonormal up direction by Gram-Schmidt orthogonalization */
			dot = vectordot3d(xf3->up, toDirection);
			up[0] = xf3->up[0] - dot * toDirection[0];
			up[1] = xf3->up[1] - dot * toDirection[1];
			up[2] = xf3->up[2] - dot * toDirection[2];

			/* orthonormal up vector vector */
			vectornormalize3d(up);	

			/* Calculate orthonormal right vector to make up an orthonormal view frame */
			vectorcross3d(up, toDirection, rightDirection);

			angle = (float)(180.0f * ((float)(us_3dlastbuty - y) /
				(float)(us_3dwindowpart->usehy-us_3dwindowpart->usely)/2.0f) * EPI / 180.0f);
			sinTheta = (float)sin(angle);
			cosTheta = (float)cos(angle);
			vectormultiply3d(toDirection, cosTheta, tempVector1);
			vectormultiply3d(xf3->up, sinTheta, tempVector2);
			vectoradd3d(tempVector1, tempVector2, rotateVector);
			vectormultiply3d(rotateVector, toLength, rotateVector);
			vectoradd3d(xf3->view, rotateVector, newFrom);

			/* rotate using RIGHT and new TO directions and X position of mouse */
			angle = (float)(180.0f * ((float)(us_3dlastbutx - x) /
				(float)(us_3dwindowpart->usehx-us_3dwindowpart->uselx)/2.0f) * EPI/180.0f);
			sinPhi = (float)sin(angle);
			cosPhi = (float)cos(angle);
			vectorsubtract3d(newFrom, xf3->view, newToDirection);
			vectornormalize3d(newToDirection);
			vectormultiply3d(newToDirection, cosPhi, tempVector1);
			vectormultiply3d(rightDirection, sinPhi, tempVector2);
			vectoradd3d(tempVector1, tempVector2, rotateVector);
			vectormultiply3d(rotateVector, toLength, rotateVector);
			vectoradd3d(xf3->view, rotateVector, xf3->eye);

			/* calculate new UP vector */
			vectormultiply3d(xf3->up, cosTheta, tempVector2);
			vectormultiply3d(toDirection, -sinTheta*cosPhi, tempVector1);
			vectormultiply3d(rightDirection, sinTheta*sinPhi, tempVector3);
			vectoradd3d(tempVector1, tempVector2, xf3->up);
			vectoradd3d(xf3->up, tempVector3, xf3->up);

			us_3dlastbutx = x;
			us_3dlastbuty = y;
			us_3dbuildtransform(xf3);
			us_3drender(us_3dwindowpart);
			break;

		case ZOOMVIEW:
			xf3->fieldofview = us_3dinitialfov + 0.1f * (float)(us_3dinitialbuty - y);
			if (xf3->fieldofview > FOVMAX) xf3->fieldofview = FOVMAX;
			if (xf3->fieldofview < FOVMIN) xf3->fieldofview = FOVMIN;
			us_3dbuildtransform(xf3);
			us_3drender(us_3dwindowpart);
			break;

		case PANVIEW:
			if (x == us_3dlastbutx && y == us_3dlastbuty) break;
			vectorsubtract3d(xf3->view, xf3->eye, d);
			vectorcross3d(d, xf3->up, side);
			vectorcross3d(d, side, up);
			vectornormalize3d(side);
			vectornormalize3d(up);
			vectorsubtract3d(xf3->view, xf3->eye, view);
			scale = vectormagnitude3d(view);
			vectormultiply3d(side, scale * 0.01f * (float)(us_3dlastbutx - x), d);
			vectormultiply3d(up, scale * 0.01f * (float)(y - us_3dlastbuty), e);
			vectoradd3d(d, e, offset);
			vectoradd3d(xf3->eye, offset, xf3->eye);
			vectoradd3d(xf3->view, offset, xf3->view);

			us_3dbuildtransform(xf3);
			us_3drender(us_3dwindowpart);
			us_3dlastbutx = x;
			us_3dlastbuty = y;
			break;

		case TWISTVIEW:
			if (x == us_3dlastbutx && y == us_3dlastbuty) break;

			/* compute angle of twist */
			cx = (us_3dwindowpart->usehx+us_3dwindowpart->uselx) / 2.0f;
			cy = (us_3dwindowpart->usehy+us_3dwindowpart->usely) / 2.0f;
			lastangle = (float)atan2(us_3dlastbuty-cy, us_3dlastbutx-cx);
			angle = (float)atan2(y-cy, x-cx);
			vectorsubtract3d(xf3->eye, xf3->view, toDirection);
			us_3drotatepointaboutaxis(xf3->up, lastangle-angle, toDirection, tempVector1);
			xf3->up[0] = tempVector1[0];
			xf3->up[1] = tempVector1[1];
			xf3->up[2] = tempVector1[2];
			vectornormalize3d(xf3->up);

			us_3dlastbutx = x;
			us_3dlastbuty = y;
			us_3dbuildtransform(xf3);
			us_3drender(us_3dwindowpart);
			break;
	}
	return(FALSE);
}

/*
 * Rotate a point "p" by angle "theta" around an arbitrary axis "r", returning point "q".
 */
void us_3drotatepointaboutaxis(float *p, float theta, float *r, float *q)
{
	float costheta, sintheta;

	vectornormalize3d(r);	
	costheta = (float)cos(theta);
	sintheta = (float)sin(theta);

	q[0] = (costheta + (1.0f - costheta) * r[0] * r[0]) * p[0];
	q[0] += ((1.0f - costheta) * r[0] * r[1] - r[2] * sintheta) * p[1];
	q[0] += ((1.0f - costheta) * r[0] * r[2] + r[1] * sintheta) * p[2];

	q[1] = ((1.0f - costheta) * r[0] * r[1] + r[2] * sintheta) * p[0];
	q[1] += (costheta + (1.0f - costheta) * r[1] * r[1]) * p[1];
	q[1] += ((1.0f - costheta) * r[1] * r[2] - r[0] * sintheta) * p[2];

	q[2] = ((1.0f - costheta) * r[0] * r[2] - r[1] * sintheta) * p[0];
	q[2] += ((1.0f - costheta) * r[1] * r[2] + r[0] * sintheta) * p[1];
	q[2] += (costheta + (1.0f - costheta) * r[2] * r[2]) * p[2];
}

/*
 * Routine to draw polygon "poly" in window "w".
 */
void us_3dshowpoly(POLYGON *poly, WINDOWPART *w)
{
	REGISTER INTBIG i, previ, lambda;
	float topheight, botheight;
	INTBIG lx, hx, ly, hy;
	float thickness, depth, topdepth, botdepth, centerdepth;
	REGISTER POLY3D *poly3d;

	/* ignore polygons with no color */
	if (poly->desc->col == 0) return;

	/* special case when drawing instance boundaries */
	if (poly->desc->col == el_colcell)
	{
		(void)get3dfactors(el_curtech, 0, &depth, &thickness);
		topdepth = depth;
		topheight = topdepth;
		if (isbox(poly, &lx, &hx, &ly, &hy))
		{
			poly3d = us_3dgetnextpoly(2);
			if (poly3d == 0) return;
			poly3d->x[0] = (float)lx;   poly3d->y[0] = (float)ly;   poly3d->z[0] = topheight;
			poly3d->x[1] = (float)lx;   poly3d->y[1] = (float)hy;   poly3d->z[1] = topheight;
			poly3d->depth = topdepth;
			poly3d->desc = poly->desc;

			poly3d = us_3dgetnextpoly(2);
			if (poly3d == 0) return;
			poly3d->x[0] = (float)lx;   poly3d->y[0] = (float)hy;   poly3d->z[0] = topheight;
			poly3d->x[1] = (float)hx;   poly3d->y[1] = (float)hy;   poly3d->z[1] = topheight;
			poly3d->depth = topdepth;
			poly3d->desc = poly->desc;

			poly3d = us_3dgetnextpoly(2);
			if (poly3d == 0) return;
			poly3d->x[0] = (float)hx;   poly3d->y[0] = (float)hy;   poly3d->z[0] = topheight;
			poly3d->x[1] = (float)hx;   poly3d->y[1] = (float)ly;   poly3d->z[1] = topheight;
			poly3d->depth = topdepth;
			poly3d->desc = poly->desc;

			poly3d = us_3dgetnextpoly(2);
			if (poly3d == 0) return;
			poly3d->x[0] = (float)hx;   poly3d->y[0] = (float)ly;   poly3d->z[0] = topheight;
			poly3d->x[1] = (float)lx;   poly3d->y[1] = (float)ly;   poly3d->z[1] = topheight;
			poly3d->depth = topdepth;
			poly3d->desc = poly->desc;
		}
		return;
	}

	/* make sure there is a technology and a layer for this polygon */
	if (poly->tech == NOTECHNOLOGY) return;
	if (get3dfactors(poly->tech, poly->layer, &depth, &thickness)) return;

	/* setup a 3D polygon */
	lambda = el_curlib->lambda[el_curtech->techindex];
	topdepth = (depth + thickness/2.0f) * lambda;
	topheight = topdepth;
	if (thickness != 0)
	{
		topdepth++;
		centerdepth = depth * lambda;
		botdepth = (depth - thickness/2.0f) * lambda - 1;
		botheight = (depth - thickness/2.0f) * lambda;
	}

	/* fill the 3D polygon points */
	switch (poly->style)
	{
		case FILLED:		/* filled polygon */
		case FILLEDRECT:	/* filled rectangle */
		case CLOSEDRECT:	/* closed rectangle outline */
		case CLOSED:		/* closed polygon outline */
		case CROSSED:		/* polygon outline with cross */
			if (isbox(poly, &lx, &hx, &ly, &hy))
			{
				poly3d = us_3dgetnextpoly(4);
				if (poly3d == 0) return;
				poly3d->x[0] = (float)lx;   poly3d->y[0] = (float)ly;   poly3d->z[0] = topheight;
				poly3d->x[1] = (float)lx;   poly3d->y[1] = (float)hy;   poly3d->z[1] = topheight;
				poly3d->x[2] = (float)hx;   poly3d->y[2] = (float)hy;   poly3d->z[2] = topheight;
				poly3d->x[3] = (float)hx;   poly3d->y[3] = (float)ly;   poly3d->z[3] = topheight;
				poly3d->depth = topdepth;
				poly3d->desc = poly->desc;
				if (thickness != 0)
				{
					poly3d = us_3dgetnextpoly(4);
					if (poly3d == 0) return;
					poly3d->x[0] = (float)lx;   poly3d->y[0] = (float)ly;   poly3d->z[0] = botheight;
					poly3d->x[1] = (float)lx;   poly3d->y[1] = (float)hy;   poly3d->z[1] = botheight;
					poly3d->x[2] = (float)hx;   poly3d->y[2] = (float)hy;   poly3d->z[2] = botheight;
					poly3d->x[3] = (float)hx;   poly3d->y[3] = (float)ly;   poly3d->z[3] = botheight;
					poly3d->depth = botdepth;
					poly3d->desc = poly->desc;

					poly3d = us_3dgetnextpoly(4);
					if (poly3d == 0) return;
					poly3d->x[0] = (float)lx;   poly3d->y[0] = (float)ly;   poly3d->z[0] = topheight;
					poly3d->x[1] = (float)lx;   poly3d->y[1] = (float)hy;   poly3d->z[1] = topheight;
					poly3d->x[2] = (float)lx;   poly3d->y[2] = (float)hy;   poly3d->z[2] = botheight;
					poly3d->x[3] = (float)lx;   poly3d->y[3] = (float)ly;   poly3d->z[3] = botheight;
					poly3d->depth = centerdepth;
					poly3d->desc = poly->desc;

					poly3d = us_3dgetnextpoly(4);
					if (poly3d == 0) return;
					poly3d->x[0] = (float)lx;   poly3d->y[0] = (float)hy;   poly3d->z[0] = topheight;
					poly3d->x[1] = (float)hx;   poly3d->y[1] = (float)hy;   poly3d->z[1] = topheight;
					poly3d->x[2] = (float)hx;   poly3d->y[2] = (float)hy;   poly3d->z[2] = botheight;
					poly3d->x[3] = (float)lx;   poly3d->y[3] = (float)hy;   poly3d->z[3] = botheight;
					poly3d->depth = centerdepth;
					poly3d->desc = poly->desc;

					poly3d = us_3dgetnextpoly(4);
					if (poly3d == 0) return;
					poly3d->x[0] = (float)hx;   poly3d->y[0] = (float)hy;   poly3d->z[0] = topheight;
					poly3d->x[1] = (float)hx;   poly3d->y[1] = (float)ly;   poly3d->z[1] = topheight;
					poly3d->x[2] = (float)hx;   poly3d->y[2] = (float)ly;   poly3d->z[2] = botheight;
					poly3d->x[3] = (float)hx;   poly3d->y[3] = (float)hy;   poly3d->z[3] = botheight;
					poly3d->depth = centerdepth;
					poly3d->desc = poly->desc;

					poly3d = us_3dgetnextpoly(4);
					if (poly3d == 0) return;
					poly3d->x[0] = (float)hx;   poly3d->y[0] = (float)ly;   poly3d->z[0] = topheight;
					poly3d->x[1] = (float)lx;   poly3d->y[1] = (float)ly;   poly3d->z[1] = topheight;
					poly3d->x[2] = (float)lx;   poly3d->y[2] = (float)ly;   poly3d->z[2] = botheight;
					poly3d->x[3] = (float)hx;   poly3d->y[3] = (float)ly;   poly3d->z[3] = botheight;
					poly3d->depth = centerdepth;
					poly3d->desc = poly->desc;
				}
				break;
			}

			/* nonmanhattan polygon: handle it as points */
			poly3d = us_3dgetnextpoly(poly->count);
			if (poly3d == 0) return;
			for(i=0; i<poly->count; i++)
			{
				poly3d->x[i] = (float)poly->xv[i];
				poly3d->y[i] = (float)poly->yv[i];
				poly3d->z[i] = topheight;
			}
			poly3d->depth = topdepth;
			poly3d->desc = poly->desc;
			if (thickness != 0)
			{
				poly3d = us_3dgetnextpoly(poly->count);
				if (poly3d == 0) return;
				for(i=0; i<poly->count; i++)
				{
					poly3d->x[i] = (float)poly->xv[i];
					poly3d->y[i] = (float)poly->yv[i];
					poly3d->z[i] = botheight;
				}
				poly3d->depth = botdepth;
				poly3d->desc = poly->desc;

				for(i=0; i<poly->count; i++)
				{
					if (i == 0) previ = poly->count-1; else previ = i-1;
					poly3d = us_3dgetnextpoly(4);
					if (poly3d == 0) return;
					poly3d->x[0] = (float)poly->xv[previ];   poly3d->y[0] = (float)poly->yv[previ];   poly3d->z[0] = topheight;
					poly3d->x[1] = (float)poly->xv[i];       poly3d->y[1] = (float)poly->yv[i];       poly3d->z[1] = topheight;
					poly3d->x[2] = (float)poly->xv[i];       poly3d->y[2] = (float)poly->yv[i];       poly3d->z[2] = botheight;
					poly3d->x[3] = (float)poly->xv[previ];   poly3d->y[3] = (float)poly->yv[previ];   poly3d->z[3] = botheight;
					poly3d->depth = centerdepth;
					poly3d->desc = poly->desc;
				}
			}
			break;
	}
}

POLY3D *us_3dgetnextpoly(INTBIG count)
{
	REGISTER INTBIG newtotal, i;
	REGISTER POLY3D **newlist, *poly3d;

	/* make sure there is room for another 3D polygon */
	if (us_3dpolycount >= us_3dpolytotal)
	{
		newtotal = us_3dpolytotal * 2;
		if (newtotal <= 0) newtotal = 50;
		if (us_3dpolycount > newtotal) newtotal = us_3dpolycount;
		newlist = (POLY3D **)emalloc(newtotal * (sizeof (POLY3D *)), us_tool->cluster);
		if (newlist == 0) return(0);
		for(i=0; i<us_3dpolytotal; i++)
			newlist[i] = us_3dpolylist[i];
		for(i=us_3dpolytotal; i<newtotal; i++)
		{
			newlist[i] = (POLY3D *)emalloc(sizeof (POLY3D), us_tool->cluster);
			if (newlist[i] == 0) return(0);
			newlist[i]->total = 0;
		}
		if (us_3dpolytotal > 0) efree((CHAR *)us_3dpolylist);
		us_3dpolylist = newlist;
		us_3dpolytotal = newtotal;
	}
	poly3d = us_3dpolylist[us_3dpolycount++];
	if (poly3d->total < count)
	{
		if (poly3d->total > 0)
		{
			efree((CHAR *)poly3d->x);
			efree((CHAR *)poly3d->y);
			efree((CHAR *)poly3d->z);
		}
		poly3d->total = 0;
		poly3d->x = (float *)emalloc(count * (sizeof (float)), us_tool->cluster);
		if (poly3d->x == 0) return(0);
		poly3d->y = (float *)emalloc(count * (sizeof (float)), us_tool->cluster);
		if (poly3d->y == 0) return(0);
		poly3d->z = (float *)emalloc(count * (sizeof (float)), us_tool->cluster);
		if (poly3d->z == 0) return(0);
		poly3d->total = count;
	}
	poly3d->count = count;
	return(poly3d);
}
