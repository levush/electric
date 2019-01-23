/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrhigh.c
 * User interface tool: object highlighting
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
#include "tech.h"
#include "tecgen.h"
#include "tecart.h"

#define DOTRADIUS 4				/* size of dots in nodes when highlighting networks */

extern GRAPHICS us_box, us_arbit, us_hbox;

/* working memory for "us_gethighlighted()" */
static GEOM   **us_gethighlist;
static INTBIG   us_gethighlistsize = 0;
static INTBIG   us_selectedtexttotal = 0;	/* size of selected text item list */
static CHAR   **us_selectedtextlist;		/* the selected text items */

/* working memory for "us_getallhighlighted()" */
static INTBIG  *us_getallhighlist;
static INTBIG   us_getallhighlistsize = 0;

/* working memory for "us_selectarea()" */
static void   *us_selectareastringarray = 0;

/* prototypes for local routines */
static void       us_makecurrentobject(void);
static void       us_highlightverbose(GEOM*, PORTPROTO*, INTBIG, NODEPROTO*, POLYGON*);
static BOOLEAN    us_geommatchestype(GEOM *geom, INTBIG type);

/*
 * Routine to free all memory associated with this module.
 */
void us_freehighmemory(void)
{
	REGISTER INTBIG i;

	if (us_gethighlistsize != 0) efree((CHAR *)us_gethighlist);
	if (us_getallhighlistsize != 0) efree((CHAR *)us_getallhighlist);
	if (us_selectareastringarray != 0) killstringarray(us_selectareastringarray);
	for(i=0; i<us_selectedtexttotal; i++)
		if (us_selectedtextlist[i] != 0) efree((CHAR *)us_selectedtextlist[i]);
	if (us_selectedtexttotal > 0) efree((CHAR *)us_selectedtextlist);
}

/******************** MENU AND WINDOW HIGHLIGHTING ********************/

/*
 * routine to highlight a menu entry
 */
void us_highlightmenu(INTBIG x, INTBIG y, INTBIG color)
{
	REGISTER INTBIG xl, yl;
	INTBIG swid, shei, pwid;
	WINDOWPART ww;
	static POLYGON *poly = NOPOLYGON;

	if ((us_tool->toolstate&MENUON) == 0) return;

	/* get polygon */
	(void)needstaticpolygon(&poly, 5, us_tool->cluster);

	/* set drawing bounds to include menu */
	if (us_menuframe == NOWINDOWFRAME)
	{
		getpaletteparameters(&swid, &shei, &pwid);
		ww.frame = getwindowframe(TRUE);
	} else
	{
		getwindowframesize(us_menuframe, &swid, &shei);
		ww.frame = us_menuframe;
	}
	ww.screenlx = ww.screenly = ww.uselx = ww.usely = 0;
	ww.screenhx = ww.usehx = swid-1;
	ww.screenhy = ww.usehy = shei-1;
	ww.state = DISPWINDOW;
	computewindowscale(&ww);

	xl = x * us_menuxsz + us_menulx;
	yl = y * us_menuysz + us_menuly;
	maketruerectpoly(xl, xl+us_menuxsz, yl, yl+us_menuysz, poly);
	poly->desc = &us_box;
	poly->style = CLOSEDRECT;
	us_box.col = color;
	us_showpoly(poly, &ww);
}

/*
 * routine to highlight window "w" and set current window to this.  If
 * "force" is true, highlight this window even if it is already the
 * current window.
 */
void us_highlightwindow(WINDOWPART *w, BOOLEAN force)
{
	if (w == el_curwindowpart && !force) return;

	if (el_curwindowpart != NOWINDOWPART) us_drawwindow(el_curwindowpart, el_colwinbor);
	if (w != NOWINDOWPART) us_drawwindow(w, el_colhwinbor);

	/* set new window */
	el_curwindowpart = w;
}

/******************** OBJECT HIGHLIGHTING ********************/

/*
 * routine to add "thishigh" to the list of objects that are highlighted.
 */
void us_addhighlight(HIGHLIGHT *thishigh)
{
	REGISTER CHAR *str, **list;
	REGISTER INTBIG i, len, rewritefirst;
	CHAR *onelist[1];
	REGISTER VARIABLE *var;
	HIGHLIGHT oldhigh;

	/* see if anything is currently highlighted */
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var == NOVARIABLE)
	{
		/* nothing highlighted: add this one line */
		onelist[0] = us_makehighlightstring(thishigh);
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_highlightedkey, (INTBIG)onelist,
			VSTRING|VISARRAY|(1<<VLENGTHSH)|VDONTSAVE);

#if SIMTOOL
		/* if current window is simulating, show equivalent elsewhere */
		if (el_curwindowpart != NOWINDOWPART && (el_curwindowpart->state&WINDOWSIMMODE) != 0 &&
			(thishigh->status&HIGHTYPE) == HIGHFROM)
		{
			onelist[0] = x_("show-equivalent");
			telltool(net_tool, 1, onelist);
		}
#endif

		/* set the current node/arc */
		us_makecurrentobject();
		return;
	}

	/* see if it is already highlighted */
	len = getlength(var);
	for(i=0; i<len; i++)
	{
		if (us_makehighlight(((CHAR **)var->addr)[i], &oldhigh)) continue;
		if ((oldhigh.status&HIGHTYPE) != (thishigh->status&HIGHTYPE)) continue;
		if (oldhigh.cell != thishigh->cell) continue;
		if ((oldhigh.status&HIGHTYPE) == HIGHLINE || (oldhigh.status&HIGHTYPE) == HIGHBBOX)
		{
			if (oldhigh.stalx != thishigh->stalx || oldhigh.stahx != thishigh->stahx ||
				oldhigh.staly != thishigh->staly || oldhigh.stahy != thishigh->stahy) continue;
		} else
		{
			if (oldhigh.fromgeom != thishigh->fromgeom) continue;
			if ((oldhigh.status&HIGHTYPE) == HIGHTEXT)
			{
				if (oldhigh.fromvar != thishigh->fromvar) continue;
				if (oldhigh.fromport != NOPORTPROTO && thishigh->fromport != NOPORTPROTO &&
					oldhigh.fromport != thishigh->fromport) continue;
			}
		}
		return;
	}

	/* process tangent snap points */
	rewritefirst = 0;
	if (len == 1 && (oldhigh.status&HIGHSNAP) != 0 && (thishigh->status&HIGHSNAP) != 0 &&
		((oldhigh.status&HIGHSNAPTAN) != 0 || (thishigh->status&HIGHSNAPTAN) != 0))
	{
		us_adjusttangentsnappoints(&oldhigh, thishigh);
		if ((oldhigh.status&HIGHSNAPTAN) != 0) rewritefirst = 1;
	} else if (len == 1 && (oldhigh.status&HIGHSNAP) != 0 && (thishigh->status&HIGHSNAP) != 0 &&
		((oldhigh.status&HIGHSNAPPERP) != 0 || (thishigh->status&HIGHSNAPPERP) != 0))
	{
		us_adjustperpendicularsnappoints(&oldhigh, thishigh);
		if ((oldhigh.status&HIGHSNAPPERP) != 0) rewritefirst = 1;
	}

	/* convert the highlight module to a string */
	str = us_makehighlightstring(thishigh);

	/* things are highlighted: build a new list */
	list = (CHAR **)emalloc(((len+1) * (sizeof (CHAR *))), el_tempcluster);
	if (list == 0) return;
	for(i=0; i<len; i++)
	{
		if (i == 0 && rewritefirst != 0)
		{
			(void)allocstring(&list[i], us_makehighlightstring(&oldhigh), el_tempcluster);
		} else
		{
			(void)allocstring(&list[i], ((CHAR **)var->addr)[i], el_tempcluster);
		}
	}
	(void)allocstring(&list[len], str, el_tempcluster);

	/* save the new list */
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_highlightedkey, (INTBIG)list,
		VSTRING|VISARRAY|((len+1)<<VLENGTHSH)|VDONTSAVE);

	/* clean up */
	for(i=0; i<=len; i++) efree(list[i]);
	efree((CHAR *)list);

	/* set the current node/arc */
	us_makecurrentobject();
}

/*
 * routine to delete "thishigh" from the list of objects that are highlighted.
 */
void us_delhighlight(HIGHLIGHT *thishigh)
{
	REGISTER CHAR **list;
	REGISTER INTBIG i, len, j, thish;
	REGISTER VARIABLE *var;
	HIGHLIGHT oldhigh;

	/* see if anything is currently highlighted */
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var == NOVARIABLE) return;

	/* see if it is highlighted */
	len = getlength(var);
	for(thish=0; thish<len; thish++)
	{
		if (us_makehighlight(((CHAR **)var->addr)[thish], &oldhigh)) continue;
		if ((oldhigh.status&HIGHTYPE) != (thishigh->status&HIGHTYPE)) continue;
		if (oldhigh.cell != thishigh->cell) continue;
		if ((oldhigh.status&HIGHTYPE) == HIGHLINE || (oldhigh.status&HIGHTYPE) == HIGHBBOX)
		{
			if (oldhigh.stalx != thishigh->stalx || oldhigh.stahx != thishigh->stahx ||
				oldhigh.staly != thishigh->staly || oldhigh.stahy != thishigh->stahy) continue;
		} else
		{
			if (oldhigh.fromgeom != thishigh->fromgeom) continue;
			if ((oldhigh.status&HIGHTYPE) == HIGHTEXT)
			{
				if (oldhigh.fromvar != thishigh->fromvar) continue;
				if (oldhigh.fromport != NOPORTPROTO && thishigh->fromport != NOPORTPROTO &&
					oldhigh.fromport != thishigh->fromport) continue;
			}
		}
		break;
	}
	if (thish >= len) return;

	/* removing highlight: build a new list */
	if (len <= 1)
	{
		(void)delvalkey((INTBIG)us_tool, VTOOL, us_highlightedkey);
		if ((us_state&NONPERSISTENTCURNODE) != 0) us_setnodeproto(NONODEPROTO);
		return;
	}
	list = (CHAR **)emalloc(((len-1) * (sizeof (CHAR *))), el_tempcluster);
	if (list == 0) return;
	for(j=i=0; i<len; i++)
	{
		if (i == thish) continue;
		(void)allocstring(&list[j], ((CHAR **)var->addr)[i], el_tempcluster);
		j++;
	}

	/* save the new list */
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_highlightedkey, (INTBIG)list,
		VSTRING|VISARRAY|((len-1)<<VLENGTHSH)|VDONTSAVE);
	us_makecurrentobject();

	/* clean up */
	for(i=0; i<len-1; i++) efree(list[i]);
	efree((CHAR *)list);
}

void us_clearhighlightcount(void)
{
	REGISTER VARIABLE *var;

	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var != NOVARIABLE)
		(void)delvalkey((INTBIG)us_tool, VTOOL, us_highlightedkey);
	if ((us_state&NONPERSISTENTCURNODE) != 0) us_setnodeproto(NONODEPROTO);
}

/*
 * routine to add highlight module "newhigh" to the display.  If "findpoint" is nonzero,
 * find the closest vertex/line to the cursor.  If "extrainfo" is nonzero, highlight
 * with extra information.  If "findmore" is nonzero, add this highlight rather
 * than replacing.  If "nobox" is nonzero, do not draw the basic box.
 */
void us_setfind(HIGHLIGHT *newhigh, INTBIG findpoint, INTBIG extrainfo, INTBIG findmore, INTBIG nobox)
{
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var;
	REGISTER INTBIG x, y, d, bestdist, bestpoint, i, total;
	INTBIG xp, yp, xc, yc;
	XARRAY trans;

	/* set to highlight only one one object */
	if (findmore == 0) us_clearhighlightcount();

	/* find the vertex/line if this is a nodeinst and it was requested */
	if (findpoint != 0 && (newhigh->status&HIGHTYPE) != HIGHTEXT &&
		newhigh->fromgeom->entryisnode)
	{
		ni = newhigh->fromgeom->entryaddr.ni;
		var = gettrace(ni);
	} else var = NOVARIABLE;
	if (var == NOVARIABLE) bestpoint = 0; else
	{
		(void)getxy(&xc, &yc);
		makerot(ni, trans);
		total = getlength(var) / 2;
		x = (ni->highx + ni->lowx) / 2;   y = (ni->highy + ni->lowy) / 2;
		for(i=0; i<total; i++)
		{
			xform(((INTBIG *)var->addr)[i*2]+x, ((INTBIG *)var->addr)[i*2+1]+y, &xp, &yp, trans);
			d = abs(xp-xc) + abs(yp-yc);
			if (i == 0) bestdist = d + 1;
			if (d > bestdist) continue;
			bestdist = d;   bestpoint = i;
		}
		bestpoint++;
	}

	/* load the highlight structure */
	newhigh->status |= extrainfo;
	newhigh->frompoint = bestpoint;
	if (nobox != 0) newhigh->status |= HIGHNOBOX;

	/* show the highlighting */
	us_addhighlight(newhigh);
}

/*
 * Routine to select the area (slx <= X <= shx, sly <= Y <= shy) in cell "np".
 * If "special" is nonzero, select "hard-to-select" items.
 * If "selport" is nonzero, select ports.
 * If "nobbox" is nonzero, do not show bounding box.
 * Returns the number of things selected, and loads the string array "highlist"
 * with the highlight information.
 */
INTBIG us_selectarea(NODEPROTO *np, INTBIG slx, INTBIG shx, INTBIG sly, INTBIG shy,
	INTBIG special, INTBIG selport, INTBIG nobbox, CHAR ***highlist)
{
	REGISTER INTBIG i, j, k, search, slop, foundtext;
	INTBIG xcur, ycur, xc, yc, xw, yw, style, count;
	BOOLEAN checktext;
	REGISTER GEOM *geom;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER BOOLEAN accept;
	PORTPROTO *port, *lastport;
	VARIABLE *var, *varnoeval, *lastvar;
	XARRAY trans;
	static POLYGON *poly = NOPOLYGON;
	HIGHLIGHT newhigh, newhightext;

	(void)needstaticpolygon(&poly, 4, us_tool->cluster);
	if (us_selectareastringarray == 0) us_selectareastringarray = newstringarray(us_tool->cluster);
	clearstrings(us_selectareastringarray);

	/* build a list of objects to highlight */
	newhigh.cell = np;
	newhigh.fromport = NOPORTPROTO;
	newhigh.fromvar = NOVARIABLE;
	newhigh.fromvarnoeval = NOVARIABLE;
	newhigh.frompoint = 0;

	newhightext.status = HIGHTEXT;
	newhightext.cell = np;
	newhightext.fromvarnoeval = NOVARIABLE;
	newhightext.frompoint = 0;

	if ((us_useroptions&HIDETXTCELL) == 0)
	{
		for(i=0; i<np->numvar; i++)
		{
			var = &np->firstvar[i];
			if ((var->type&VDISPLAY) == 0) continue;
			newhightext.fromgeom = NOGEOM;
			newhightext.fromport = NOPORTPROTO;
			newhightext.fromvarnoeval = var;
			newhightext.fromvar = evalvar(newhightext.fromvarnoeval, (INTBIG)np, VNODEPROTO);
			us_gethightextcenter(&newhightext, &xc, &yc, &style);
			us_gethightextsize(&newhightext, &xw, &yw, el_curwindowpart);
			if ((us_useroptions&MUSTENCLOSEALL) != 0)
			{
				/* accept if it is inside the area */
				if (xc-xw/2 < slx || xc+xw/2 > shx ||
					yc-yw/2 < sly || yc+yw/2 > shy) continue;
			} else
			{
				/* accept if it touches the area */
				if (xc+xw/2 < slx || xc-xw/2 > shx ||
					yc+yw/2 < sly || yc-yw/2 > shy) continue;
			}
			addtostringarray(us_selectareastringarray, us_makehighlightstring(&newhightext));
		}
	}
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if ((ni->userbits&NHASFARTEXT) == 0) continue;
		if (ni->proto == gen_invispinprim)
		{
			if ((us_useroptions&HIDETXTNONLAY) != 0) continue;
		} else
		{
			if ((us_useroptions&HIDETXTNODE) != 0) continue;
		}
		for(i=0; i<ni->numvar; i++)
		{
			var = &ni->firstvar[i];
			if ((var->type&VDISPLAY) == 0) continue;
			newhightext.fromgeom = ni->geom;
			newhightext.fromport = NOPORTPROTO;
			newhightext.fromvarnoeval = var;
			newhightext.fromvar = evalvar(newhightext.fromvarnoeval, (INTBIG)ni, VNODEINST);
			us_gethightextcenter(&newhightext, &xc, &yc, &style);
			us_gethightextsize(&newhightext, &xw, &yw, el_curwindowpart);
			if ((us_useroptions&MUSTENCLOSEALL) != 0)
			{
				/* accept if it is inside the area */
				if (xc-xw/2 < slx || xc+xw/2 > shx ||
					yc-yw/2 < sly || yc+yw/2 > shy) continue;
			} else
			{
				/* accept if it touches the area */
				if (xc+xw/2 < slx || xc-xw/2 > shx ||
					yc+yw/2 < sly || yc-yw/2 > shy) continue;
			}
			addtostringarray(us_selectareastringarray, us_makehighlightstring(&newhightext));
		}
	}
	if ((us_useroptions&HIDETXTARC) == 0)
	{
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		{
			if ((ai->userbits&AHASFARTEXT) == 0) continue;
			for(i=0; i<ai->numvar; i++)
			{
				var = &ai->firstvar[i];
				if ((var->type&VDISPLAY) == 0) continue;
				newhightext.fromgeom = ai->geom;
				newhightext.fromport = NOPORTPROTO;
				newhightext.fromvarnoeval = var;
				newhightext.fromvar = evalvar(newhightext.fromvarnoeval, (INTBIG)ai, VARCINST);
				us_gethightextcenter(&newhightext, &xc, &yc, &style);
				us_gethightextsize(&newhightext, &xw, &yw, el_curwindowpart);
				if ((us_useroptions&MUSTENCLOSEALL) != 0)
				{
					/* accept if it is inside the area */
					if (xc-xw/2 < slx || xc+xw/2 > shx ||
						yc-yw/2 < sly || yc+yw/2 > shy) continue;
				} else
				{
					/* accept if it touches the area */
					if (xc+xw/2 < slx || xc-xw/2 > shx ||
						yc+yw/2 < sly || yc-yw/2 > shy) continue;
				}
				addtostringarray(us_selectareastringarray, us_makehighlightstring(&newhightext));
			}
		}
	}
	slop = FARTEXTLIMIT * lambdaofcell(np);
	search = initsearch(slx-slop, shx+slop, sly-slop, shy+slop, np);
	while ((geom = nextobject(search)) != NOGEOM)
	{
		if (geom->entryisnode)
		{
			ni = geom->entryaddr.ni;

			/* don't find cells if not requested */
			if (special == 0 && (us_useroptions&NOINSTANCESELECT) != 0 &&
				ni->proto->primindex == 0) continue;

			/* do not include hard-to-find nodes if not selecting specially */
			if (special == 0 && (ni->userbits&HARDSELECTN) != 0) continue;

			/* do not include primitives that have all layers invisible */
			if (ni->proto->primindex != 0 && (ni->proto->userbits&NINVISIBLE) != 0)
				continue;

			/* include all displayable variables */
			foundtext = 0;

			/* ignore text if invisible */
			checktext = TRUE;
			if (ni->proto == gen_invispinprim)
			{
				if ((us_useroptions&HIDETXTNONLAY) != 0) checktext = FALSE;
			} else
			{
				if ((us_useroptions&HIDETXTNODE) != 0) checktext = FALSE;
			}
			if (checktext)
			{
				/* compute threshold for direct hits */
				lastvar = NOVARIABLE;
				lastport = NOPORTPROTO;
				if ((ni->userbits&NHASFARTEXT) == 0)
				{
					us_initnodetext(ni, special, el_curwindowpart);
					for(;;)
					{
						if (us_getnodetext(ni, el_curwindowpart, poly, &var, &varnoeval, &port)) break;
						accept = FALSE;
						if ((us_useroptions&MUSTENCLOSEALL) != 0)
						{
							/* accept if it is inside the area */
							for(k=0; k<poly->count; k++)
							{
								if (poly->xv[k] < slx || poly->xv[k] > shx ||
									poly->yv[k] < sly || poly->yv[k] > shy) break;
							}
							if (k >= poly->count) accept = TRUE;
						} else
						{
							/* accept if it touches the area */
							if (polyinrect(poly, slx, shx, sly, shy)) accept = TRUE;
						}
						if (accept)
						{
							if (varnoeval == lastvar && port == lastport) continue;
							lastvar = varnoeval;
							lastport = port;
							newhightext.fromgeom = geom;
							newhightext.fromport = port;
							newhightext.fromvarnoeval = varnoeval;
							newhightext.fromvar = var;
							addtostringarray(us_selectareastringarray, us_makehighlightstring(&newhightext));
						}
						foundtext++;
					}
				}
			}

			/* ignore invisible primitive if text on it was selected */
			if (foundtext != 0 && ni->proto == gen_invispinprim) continue;

			/* do not include "edge select" primitives that are outside the area */
			if (ni->proto->primindex != 0 &&
				(ni->proto->userbits&NEDGESELECT) != 0)
			{
				i = nodepolys(ni, 0, NOWINDOWPART);
				makerot(ni, trans);
				for(j=0; j<i; j++)
				{
					shapenodepoly(ni, j, poly);
					if ((poly->desc->colstyle&INVISIBLE) == 0)
					{
						xformpoly(poly, trans);
						accept = FALSE;
						if ((us_useroptions&MUSTENCLOSEALL) != 0)
						{
							/* accept if it is inside the area */
							for(k=0; k<poly->count; k++)
							{
								if (poly->xv[k] < slx || poly->xv[k] > shx ||
									poly->yv[k] < sly || poly->yv[k] > shy) break;
							}
							if (k >= poly->count) accept = TRUE;
						} else
						{
							/* accept if it touches the area */
							if (polyinrect(poly, slx, shx, sly, shy)) accept = TRUE;
						}
						if (!accept) break;
					}
				}
				if (j < i) continue;
			}
		} else
		{
			ai = geom->entryaddr.ai;

			/* do not include hard-to-find arcs if not selecting specially */
			if (special == 0 && (ai->userbits&HARDSELECTA) != 0) continue;

			/* do not include arcs that have all layers invisible */
			if ((ai->proto->userbits&AINVISIBLE) != 0) continue;

			/* include all displayable variables */
			if ((us_useroptions&HIDETXTARC) == 0)
			{
				if ((ai->userbits&AHASFARTEXT) == 0)
				{
					for(i=0; i<ai->numvar; i++)
					{
						var = &ai->firstvar[i];
						if ((var->type&VDISPLAY) == 0) continue;
						newhightext.fromgeom = geom;
						newhightext.fromport = NOPORTPROTO;
						newhightext.fromvarnoeval = var;
						newhightext.fromvar = evalvar(newhightext.fromvarnoeval, (INTBIG)ai, VARCINST);
						us_gethightextcenter(&newhightext, &xc, &yc, &style);
						us_gethightextsize(&newhightext, &xw, &yw, el_curwindowpart);
						if ((us_useroptions&MUSTENCLOSEALL) != 0)
						{
							/* accept if it is inside the area */
							if (xc-xw/2 < slx || xc+xw/2 > shx ||
								yc-yw/2 < sly || yc+yw/2 > shy) continue;
						} else
						{
							/* accept if it touches the area */
							if (xc+xw/2 < slx || xc-xw/2 > shx ||
								yc+yw/2 < sly || yc-yw/2 > shy) continue;
						}
						addtostringarray(us_selectareastringarray, us_makehighlightstring(&newhightext));
					}
				}
			}

			/* do not include "edge select" arcs that are outside the area */
			if ((ai->proto->userbits&AEDGESELECT) != 0)
			{
				ai = geom->entryaddr.ai;
				i = arcpolys(ai, NOWINDOWPART);
				for(j=0; j<i; j++)
				{
					shapearcpoly(ai, j, poly);
					if ((poly->desc->colstyle&INVISIBLE) == 0)
					{
						accept = FALSE;
						if ((us_useroptions&MUSTENCLOSEALL) != 0)
						{
							/* accept if it is inside the area */
							for(k=0; k<poly->count; k++)
							{
								if (poly->xv[k] < slx || poly->xv[k] > shx ||
									poly->yv[k] < sly || poly->yv[k] > shy) break;
							}
							if (k >= poly->count) accept = TRUE;
						} else
						{
							/* accept if it touches the area */
							if (polyinrect(poly, slx, shx, sly, shy)) accept = TRUE;
						}
						if (!accept) break;
					}
				}
				if (j < i) continue;
			}
		}

		/* eliminate objects that aren't really in the selected area */
		if (!us_geominrect(geom, slx, shx, sly, shy)) continue;

		newhigh.status = HIGHFROM;
		if (nobbox != 0) newhigh.status |= HIGHNOBOX;
		newhigh.fromgeom = geom;

		/* add snapping information if requested */
		if ((us_state&SNAPMODE) != SNAPMODENONE)
		{
			(void)getxy(&xcur, &ycur);
			us_selectsnap(&newhigh, xcur, ycur);
		}

		/* select the first port if ports are requested and node is artwork */
		newhigh.fromport = NOPORTPROTO;
		if (selport != 0 && geom->entryisnode &&
			geom->entryaddr.ni->proto->primindex != 0 &&
				geom->entryaddr.ni->proto->tech == art_tech)
					newhigh.fromport = geom->entryaddr.ni->proto->firstportproto;
		addtostringarray(us_selectareastringarray, us_makehighlightstring(&newhigh));
	}
	*highlist = getstringarray(us_selectareastringarray, &count);
	return(count);
}

/*
 * routine to demand a specified type of highlighted object (VNODEINST or
 * VARCINST, according to "type") and return its address.  If "canbetext"
 * is true, the highlighted object can be a text object on the requested
 * type.  If the specified object is not currently highlighted, an error is
 * printed and the routine returns -1.
 */
UINTBIG us_getobject(INTBIG type, BOOLEAN canbetext)
{
	HIGHLIGHT high;
	REGISTER VARIABLE *var;
	REGISTER INTBIG len, i;
	REGISTER BOOLEAN notone;
	REGISTER GEOM *onegeom;

	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var == NOVARIABLE) len = 0; else
		len = getlength(var);
	notone = FALSE;
	onegeom = NOGEOM;
	for(i=0; i<len; i++)
	{
		if (us_makehighlight(((CHAR **)var->addr)[i], &high))
		{
			us_abortcommand(_("Highlight unintelligible"));
			return((UINTBIG)-1);
		}
		if ((high.status&HIGHTYPE) == HIGHTEXT && !canbetext) continue;
		if (high.fromgeom == NOGEOM)
		{
			notone = TRUE;
			break;
		}
		if (onegeom == NOGEOM) onegeom = high.fromgeom;
		if (high.fromgeom != onegeom)
		{
			notone = TRUE;
			break;
		}
	}
	if (!notone && onegeom != NOGEOM)
	{
		if (onegeom->entryisnode)
		{
			if (type != VNODEINST) notone = TRUE;
		} else
		{
			if (type != VARCINST) notone = TRUE;
		}
	}
	if (notone || onegeom == NOGEOM)
	{
		if (type == VARCINST) us_abortcommand(_("Select an arc")); else
			us_abortcommand(_("Select a node"));
		return((UINTBIG)-1);
	}

	/* set the current position */
	return((UINTBIG)onegeom->entryaddr.blind);
}

/*
 * routine to return the one object currently highlighted.  If no objects
 * are highlighted or many are, an error is printed and the routine returns
 * NOHIGHLIGHT.
 */
HIGHLIGHT *us_getonehighlight(void)
{
	static HIGHLIGHT high;
	REGISTER VARIABLE *var;
	REGISTER GEOM *onegeom;
	REGISTER INTBIG len, i;
	REGISTER BOOLEAN notone;

	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var == NOVARIABLE) len = 0; else
		len = getlength(var);
	notone = FALSE;
	onegeom = NOGEOM;
	for(i=0; i<len; i++)
	{
		if (us_makehighlight(((CHAR **)var->addr)[i], &high))
		{
			us_abortcommand(_("Highlight unintelligible"));
			return(NOHIGHLIGHT);
		}
		if (high.fromgeom == NOGEOM)
		{
			notone = TRUE;
			break;
		}
		if (onegeom == NOGEOM) onegeom = high.fromgeom;
		if (high.fromgeom != onegeom)
		{
			notone = TRUE;
			break;
		}
	}
	if (notone || onegeom == NOGEOM)
	{
		us_abortcommand(_("Must select one node or arc"));
		return(NOHIGHLIGHT);
	}
	return(&high);
}

/*
 * routine to get the two nodes that are selected and their ports.
 * If there are not two nodes selected, the routine returns true.
 */
BOOLEAN us_gettwoobjects(GEOM **firstgeom, PORTPROTO **firstport, GEOM **secondgeom,
	PORTPROTO **secondport)
{
	REGISTER VARIABLE *var;
	HIGHLIGHT newhigh;
	REGISTER INTBIG len, i;

	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var == NOVARIABLE) return(TRUE);
	len = getlength(var);
	*firstgeom = *secondgeom = NOGEOM;
	for(i=0; i<len; i++)
	{
		(void)us_makehighlight(((CHAR **)var->addr)[i], &newhigh);
		if ((newhigh.status&HIGHFROM) != 0)
		{
			if (*firstgeom == NOGEOM)
			{
				*firstgeom = newhigh.fromgeom;
				*firstport = newhigh.fromport;
			} else if (*secondgeom == NOGEOM)
			{
				*secondgeom = newhigh.fromgeom;
				*secondport = newhigh.fromport;
			} else return(TRUE);
		}
	}
	if (*firstgeom != NOGEOM && *secondgeom != NOGEOM) return(FALSE);
	return(TRUE);
}

/*
 * routine to get a list of all objects that are highlighted, build an array
 * of them, and return the array.
 * If "type" is WANTNODEINST, only the highlighted nodes are returned.
 * If "type" is WANTARCINST, only the highlighted arcs are returned.
 * if "type" is WANTNODEINST|WANTARCINST, all highlighted objects are returned.
 * The list is terminated with the value NOGEOM.
 * If "textcount" and "textinfo" are nonzero, they are set to the number of
 * text objects that are highlighted (and the highlight descriptors for each).
 * When text objects are requested, those on selected nodes or arcs are ignored.
 *
 * As an indication of the nodes that are selected, each node's "temp1" field is
 * set to nonzero if it is selected (but not if text on it is selected).
 * This is used by "usrtrack.c:us_finddoibegin()" and by "usrcomln.c:us_move()".
 */
GEOM **us_gethighlighted(INTBIG type, INTBIG *textcount, CHAR ***textinfo)
{
	HIGHLIGHT high;
	REGISTER VARIABLE *var;
	REGISTER INTBIG total, len, i, j, k, newcount, numtext, lx, hx, ly, hy,
		textonnode, search;
	REGISTER GEOM *geom;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER CHAR **newtext;
	static GEOM *nulllist[1];
	static POLYGON *poly = NOPOLYGON;
	XARRAY trans;

	if (textcount != 0) *textcount = 0;
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var == NOVARIABLE)
	{
		nulllist[0] = NOGEOM;
		return(nulllist);
	}

	/* first search to see how many objects there are */
	len = getlength(var);
	numtext = 0;
	total = 0;
	for(i=0; i<len; i++)
	{
		(void)us_makehighlight(((CHAR **)var->addr)[i], &high);
		if ((high.status&HIGHTYPE) == HIGHFROM)
			if (us_geommatchestype(high.fromgeom, type)) total++;
		if ((high.status&HIGHTYPE) == HIGHBBOX)
		{
			lx = high.stalx;   hx = high.stahx;
			ly = high.staly;   hy = high.stahy;
			search = initsearch(lx, hx, ly, hy, high.cell);
			while ((geom = nextobject(search)) != NOGEOM)
			{
				if (!us_geommatchestype(geom, type)) continue;
				if (!us_geominrect(geom, lx, hx, ly, hy)) continue;
				total++;
			}
		}

		/* special case: include in node list if text tied to node and no text requested */
		if ((high.status&HIGHTYPE) == HIGHTEXT)
		{
			if (us_nodemoveswithtext(&high) && textcount == 0)
				total++;
		}
	}

	/* get memory for array of these objects */
	if (total >= us_gethighlistsize)
	{
		if (us_gethighlistsize != 0) efree((CHAR *)us_gethighlist);
		us_gethighlist = (GEOM **)emalloc(((total+1) * (sizeof (GEOM *))),
			us_tool->cluster);
		if (us_gethighlist == 0)
		{
			ttyputnomemory();
			nulllist[0] = NOGEOM;
			return(nulllist);
		}
		us_gethighlistsize = total + 1;
	}

	/* now place the objects in the list */
	total = 0;
	for(i=0; i<len; i++)
	{
		(void)us_makehighlight(((CHAR **)var->addr)[i], &high);
		if ((high.status&HIGHTYPE) == HIGHFROM)
			if (us_geommatchestype(high.fromgeom, type))
		{
			for(j=0; j<total; j++) if (us_gethighlist[j] == high.fromgeom) break;
			if (j >= total) us_gethighlist[total++] = high.fromgeom;
		}
		if ((high.status&HIGHTYPE) == HIGHBBOX)
		{
			lx = high.stalx;   hx = high.stahx;
			ly = high.staly;   hy = high.stahy;
			search = initsearch(lx, hx, ly, hy, high.cell);
			for(;;)
			{
				geom = nextobject(search);
				if (geom == NOGEOM) break;

				/* make sure the object has the desired type */
				if (!us_geommatchestype(geom, type)) continue;

				/* make sure the object is really in the selection area */
				if (!us_geominrect(geom, lx, hx, ly, hy)) continue;

				/* make sure the object isn't already in the list */
				for(j=0; j<total; j++) if (us_gethighlist[j] == geom) break;
				if (j < total) continue;

				/* check carefully for "edge select" primitives */
				if (geom->entryisnode)
				{
					ni = geom->entryaddr.ni;
					if (ni->proto->primindex != 0)
					{
						if ((ni->proto->userbits&NINVISIBLE) != 0) continue;
						if ((ni->proto->userbits&NEDGESELECT) != 0)
						{
							ni = geom->entryaddr.ni;
							k = nodepolys(ni, 0, NOWINDOWPART);
							(void)needstaticpolygon(&poly, 4, us_tool->cluster);
							makerot(ni, trans);
							for(j=0; j<k; j++)
							{
								shapenodepoly(ni, j, poly);
								if ((poly->desc->colstyle&INVISIBLE) == 0)
								{
									xformpoly(poly, trans);
									if (!polyinrect(poly, high.stalx, high.stahx,
										high.staly, high.stahy)) break;
								}
							}
							if (j < k) continue;
						}
					}
				} else
				{
					ai = geom->entryaddr.ai;
					if ((ai->proto->userbits&AINVISIBLE) != 0) continue;
					if ((ai->proto->userbits&AEDGESELECT) != 0)
					{
						k = arcpolys(ai, NOWINDOWPART);
						(void)needstaticpolygon(&poly, 4, us_tool->cluster);
						for(j=0; j<k; j++)
						{
							shapearcpoly(ai, j, poly);
							if ((poly->desc->colstyle&INVISIBLE) == 0)
							{
								if (!polyinrect(poly, high.stalx, high.stahx,
									high.staly, high.stahy)) break;
							}
						}
						if (j < k) continue;
					}
				}

				/* add this object to the list */
				us_gethighlist[total++] = geom;
			}
		}
		if ((high.status&HIGHTYPE) == HIGHTEXT)
		{
			if (us_nodemoveswithtext(&high) && textcount == 0)
			{
				/* include in node list if text tied to node and no text requested */
				us_gethighlist[total++] = high.fromgeom;
			} else
			{
				if (textcount != 0)
				{
					if (numtext >= us_selectedtexttotal)
					{
						newcount = numtext + 5;
						newtext = (CHAR **)emalloc(newcount * (sizeof (CHAR *)),
							us_tool->cluster);
						if (newtext == 0)
						{
							nulllist[0] = NOGEOM;
							return(nulllist);
						}
						for(j=0; j<us_selectedtexttotal; j++)
							newtext[j] = us_selectedtextlist[j];
						for( ; j<newcount; j++) newtext[j] = 0;
						if (us_selectedtexttotal > 0) efree((CHAR *)us_selectedtextlist);
						us_selectedtextlist = newtext;
						us_selectedtexttotal = newcount;
					}
					if (us_selectedtextlist[numtext] == 0)
					{
						(void)allocstring(&us_selectedtextlist[numtext],
							((CHAR **)var->addr)[i], us_tool->cluster);
					} else
					{
						(void)reallocstring(&us_selectedtextlist[numtext],
							((CHAR **)var->addr)[i], us_tool->cluster);
					}
					numtext++;
				}
			}
		}
	}
	us_gethighlist[total] = NOGEOM;
	if (textcount != 0 && textinfo != 0)
	{
		/* mark all nodes (including those touched by highlighted arcs) */
		if (us_gethighlist[0] != NOGEOM) np = geomparent(us_gethighlist[0]); else
			np = el_curwindowpart->curnodeproto;
		if (np != NONODEPROTO)
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				ni->temp1 = 0;
		if (us_gethighlist[0] != NOGEOM)
		{
			for(i=0; us_gethighlist[i] != NOGEOM; i++)
			{
				if (!us_gethighlist[i]->entryisnode)
				{
					ai = us_gethighlist[i]->entryaddr.ai;
					ai->end[0].nodeinst->temp1 = ai->end[1].nodeinst->temp1 = 1;
				} else
					us_gethighlist[i]->entryaddr.ni->temp1 = 1;
			}
		}

		/* remove nodes listed as text objects */
		for(i=0; i<numtext; i++)
		{
			(void)us_makehighlight(us_selectedtextlist[i], &high);

			/* make sure we aren't moving text and object it resides on */
			textonnode = 0;
			if (high.fromgeom != NOGEOM)
			{
				if (high.fromgeom->entryisnode)
				{
					ni = high.fromgeom->entryaddr.ni;
					if (ni->temp1 != 0) textonnode = 1;
				} else
				{
					ai = high.fromgeom->entryaddr.ai;
					for(j=0; us_gethighlist[j] != NOGEOM; j++)
						if (us_gethighlist[j] == ai->geom) break;
					if (us_gethighlist[j] != NOGEOM) textonnode = 1;
				}
			}
			if (textonnode == 0) continue;
			efree(us_selectedtextlist[i]);
			for(j=i; j<numtext-1; j++)
				us_selectedtextlist[j] = us_selectedtextlist[j+1];
			us_selectedtextlist[numtext-1] = 0;
			numtext--;
			i--;
		}

		*textcount = numtext;
		*textinfo = us_selectedtextlist;
	}
	return(us_gethighlist);
}

BOOLEAN us_geommatchestype(GEOM *geom, INTBIG type)
{
	if (geom->entryisnode)
	{
		if ((type&WANTNODEINST) != 0) return(TRUE);
		return(FALSE);
	}
	if ((type&WANTARCINST) != 0) return(TRUE);
	return(FALSE);
}

/*
 * routine to get a list of all objects that are highlighted (including nodes, arcs,
 * and ports), build an array that alternates address and type, and return the array.
 * The list is terminated with the type 0.
 */
INTBIG *us_getallhighlighted(void)
{
	HIGHLIGHT high;
	REGISTER VARIABLE *var;
	REGISTER INTBIG total, len, i, j, k, lx, hx, ly, hy, search;
	REGISTER GEOM *geom;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	static INTBIG nulllist[2];
	static POLYGON *poly = NOPOLYGON;
	XARRAY trans;

	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var == NOVARIABLE)
	{
		nulllist[0] = nulllist[1] = 0;
		return(nulllist);
	}

	/* first search to see how many objects there are */
	len = getlength(var);
	total = 0;
	for(i=0; i<len; i++)
	{
		(void)us_makehighlight(((CHAR **)var->addr)[i], &high);
		if ((high.status&HIGHTYPE) == HIGHFROM) total++;
		if ((high.status&HIGHTYPE) == HIGHBBOX)
		{
			search = initsearch(high.stalx, high.stahx, high.staly, high.stahy, high.cell);
			while ((geom = nextobject(search)) != NOGEOM) total++;
		}

		/* special case: accept ports and invisible node if it holds highlighted text */
		if ((high.status&HIGHTYPE) == HIGHTEXT)
		{
			if (high.fromvar != NOVARIABLE && high.fromgeom != NOGEOM)
			{
				if (high.fromgeom->entryisnode &&
					high.fromgeom->entryaddr.ni->proto == gen_invispinprim) total++;
			}
			if (high.fromvar == NOVARIABLE && high.fromport != NOPORTPROTO) total++;
		}
	}

	/* get memory for array of these objects */
	if (total*2 >= us_getallhighlistsize)
	{
		if (us_getallhighlistsize != 0) efree((CHAR *)us_getallhighlist);
		us_getallhighlist = (INTBIG *)emalloc((total+1)*2 * SIZEOFINTBIG, us_tool->cluster);
		if (us_getallhighlist == 0)
		{
			ttyputnomemory();
			nulllist[0] = nulllist[1] = 0;
			return(nulllist);
		}
		us_getallhighlistsize = (total+1)*2;
	}

	/* now place the objects in the list */
	total = 0;
	for(i=0; i<len; i++)
	{
		(void)us_makehighlight(((CHAR **)var->addr)[i], &high);
		if ((high.status&HIGHTYPE) == HIGHFROM)
		{
			for(j=0; j<total; j += 2)
				if (us_getallhighlist[j] == (INTBIG)high.fromgeom->entryaddr.blind) break;
			if (j >= total)
			{
				us_getallhighlist[total++] = (INTBIG)high.fromgeom->entryaddr.blind;
				if (high.fromgeom->entryisnode) us_getallhighlist[total++] = VNODEINST; else
					us_getallhighlist[total++] = VARCINST;
			}
		}
		if ((high.status&HIGHTYPE) == HIGHBBOX)
		{
			lx = high.stalx;   hx = high.stahx;
			ly = high.staly;   hy = high.stahy;
			search = initsearch(lx, hx, ly, hy, high.cell);
			for(;;)
			{
				geom = nextobject(search);
				if (geom == NOGEOM) break;
				if (!us_geominrect(geom, lx, hx, ly, hy)) continue;

				/* make sure it isn't already in the list */
				for(j=0; j<total; j++)
					if (us_getallhighlist[j] == (INTBIG)geom->entryaddr.blind) break;
				if (j < total) continue;

				/* check carefully for "edge select" primitives */
				if (geom->entryisnode)
				{
					ni = geom->entryaddr.ni;
					if (ni->proto->primindex != 0)
					{
						if ((ni->proto->userbits&NINVISIBLE) != 0) continue;
						if ((ni->proto->userbits&NEDGESELECT) != 0)
						{
							ni = geom->entryaddr.ni;
							k = nodepolys(ni, 0, NOWINDOWPART);
							(void)needstaticpolygon(&poly, 4, us_tool->cluster);
							makerot(ni, trans);
							for(j=0; j<k; j++)
							{
								shapenodepoly(ni, j, poly);
								if ((poly->desc->colstyle&INVISIBLE) == 0)
								{
									xformpoly(poly, trans);
									if (!polyinrect(poly, high.stalx, high.stahx, high.staly, high.stahy))
										break;
								}
							}
							if (j < k) continue;
						}
					}
				} else
				{
					ai = geom->entryaddr.ai;
					if ((ai->proto->userbits&AINVISIBLE) != 0) continue;
					if ((ai->proto->userbits&AEDGESELECT) != 0)
					{
						k = arcpolys(ai, NOWINDOWPART);
						(void)needstaticpolygon(&poly, 4, us_tool->cluster);
						for(j=0; j<k; j++)
						{
							shapearcpoly(ai, j, poly);
							if ((poly->desc->colstyle&INVISIBLE) == 0)
							{
								if (!polyinrect(poly, high.stalx, high.stahx, high.staly, high.stahy))
									break;
							}
						}
						if (j < k) continue;
					}
				}

				/* add this object to the list */
				us_getallhighlist[total++] = (INTBIG)geom->entryaddr.blind;
				if (geom->entryisnode) us_getallhighlist[total++] = VNODEINST; else
					us_getallhighlist[total++] = VARCINST;
			}
		}
		if ((high.status&HIGHTYPE) == HIGHTEXT)
		{
			if (high.fromvar != NOVARIABLE && high.fromgeom != NOGEOM)
			{
				if (high.fromgeom->entryisnode &&
					high.fromgeom->entryaddr.ni->proto == gen_invispinprim)
				{
					us_getallhighlist[total++] = (INTBIG)high.fromgeom->entryaddr.blind;
					us_getallhighlist[total++] = VNODEINST;
				}
			}
			if (high.fromvar == NOVARIABLE && high.fromport != NOPORTPROTO)
			{
				us_getallhighlist[total++] = (INTBIG)high.fromport;
				us_getallhighlist[total++] = VPORTPROTO;
			}
		}
	}
	us_getallhighlist[total++] = 0;
	us_getallhighlist[total] = 0;
	return(us_getallhighlist);
}

/*
 * routine to get the boundary of an area delimited by the highlight.
 * The bounds are put in the reference variables "lx", "hx", "ly", and "hy".
 * If there is no highlighted area, the routine returns with NONODEPROTO
 * Otherwise the routine returns the cell that contains the area.
 */
NODEPROTO *us_getareabounds(INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy)
{
	HIGHLIGHT high;
	REGISTER NODEINST *ni;
	REGISTER BOOLEAN first;
	REGISTER INTBIG len, pointlen, i, good, bad;
	INTBIG tlx, thx, tly, thy, xc, yc;
	XARRAY trans;
	static POLYGON *poly = NOPOLYGON;
	REGISTER NODEPROTO *thiscell;
	REGISTER TECHNOLOGY *tech;
	REGISTER VARIABLE *var;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var == NOVARIABLE) return(NONODEPROTO);

	thiscell = getcurcell();
	good = bad = 0;
	(void)us_makehighlight(((CHAR **)var->addr)[0], &high);
	len = getlength(var);
	first = TRUE;
	for(i=0; i<len; i++)
	{
		(void)us_makehighlight(((CHAR **)var->addr)[i], &high);
		if (thiscell == NONODEPROTO) thiscell = high.cell;
		if (high.cell == thiscell) good++; else
		{
			bad++;
			continue;
		}
		if ((high.status&HIGHTYPE) == HIGHTEXT)
		{
			if (high.fromvar != NOVARIABLE)
			{
				if (high.fromgeom == NOGEOM) continue;
				pointlen = 1;
				ni = NONODEINST;
				if (high.fromport != NOPORTPROTO) ni = high.fromport->subnodeinst; else
					if (high.fromgeom != NOGEOM && high.fromgeom->entryisnode)
				{
					ni = high.fromgeom->entryaddr.ni;
					if ((high.fromvar->type&VISARRAY) != 0)
						pointlen = getlength(high.fromvar);
				}
				if (pointlen > 1)
				{
					makedisparrayvarpoly(high.fromgeom, el_curwindowpart, high.fromvar, poly);
				} else
				{
					if (ni != NONODEINST) tech = ni->proto->tech; else
					{
						if (high.fromgeom != NOGEOM) tech = high.fromgeom->entryaddr.ai->proto->tech; else
							tech = high.cell->tech;
					}
					us_maketextpoly(describevariable(high.fromvar, -1, -1), el_curwindowpart,
						(high.fromgeom->lowx + high.fromgeom->highx) / 2,
							(high.fromgeom->lowy + high.fromgeom->highy) / 2,
								ni, tech, high.fromvar->textdescript, poly);
				}
				if (ni != NONODEINST)
				{
					makerot(ni, trans);
					xformpoly(poly, trans);
				}
			} else if (high.fromport != NOPORTPROTO)
			{
				portposition(high.fromport->subnodeinst, high.fromport->subportproto, &xc, &yc);
				us_maketextpoly(us_displayedportname(high.fromport, (us_useroptions&EXPORTLABELS) >> EXPORTLABELSSH),
					el_curwindowpart, xc, yc, high.fromport->subnodeinst,
						high.fromport->subnodeinst->proto->tech, high.fromport->textdescript, poly);
			} else if (high.fromgeom->entryisnode)
			{
				ni = high.fromgeom->entryaddr.ni;
				us_maketextpoly(describenodeproto(ni->proto), el_curwindowpart,
					(ni->lowx + ni->highx) / 2, (ni->lowy + ni->highy) / 2,
						ni, ni->proto->tech, ni->textdescript, poly);
			}
			getbbox(poly, &tlx, &thx, &tly, &thy);
		}
		if ((high.status&HIGHTYPE) == HIGHBBOX || (high.status&HIGHTYPE) == HIGHLINE)
		{
			tlx = high.stalx;   thx = high.stahx;
			tly = high.staly;   thy = high.stahy;
		}
		if ((high.status&HIGHTYPE) == HIGHFROM)
		{
			if (high.fromgeom->entryisnode)
			{
				us_getnodebounds(high.fromgeom->entryaddr.ni, &tlx, &thx, &tly, &thy);
			} else
			{
				us_getarcbounds(high.fromgeom->entryaddr.ai, &tlx, &thx, &tly, &thy);
			}
		}
		if (first)
		{
			*lx = tlx;   *hx = thx;   *ly = tly;   *hy = thy;
			first = FALSE;
		} else
		{
			*lx = mini(*lx, tlx);   *hx = maxi(*hx, thx);
			*ly = mini(*ly, tly);   *hy = maxi(*hy, thy);
		}
	}
	if (good == 0 && bad != 0)
	{
		us_abortcommand(_("Highlighted areas must be in the current cell"));
		return(NONODEPROTO);
	}
	return(thiscell);
}

/*
 * Routine to move the "numtexts" text objects described (as highlight strings)
 * in the array "textlist", by "odx" and "ody".  Geometry objects in "list" (NOGEOM-terminated)
 * and the "total" nodes in "nodelist" have already been moved, so don't move any text that
 * is on these objects.
 */
void us_moveselectedtext(INTBIG numtexts, CHAR **textlist, GEOM **list, INTBIG odx, INTBIG ody)
{
	REGISTER INTBIG i, j, lambda;
	INTBIG dx, dy, addr, type;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	REGISTER NODEINST *ni;
	XARRAY trans;
	HIGHLIGHT high;

	for(i=0; i<numtexts; i++)
	{
		dx = odx;   dy = ody;
		(void)us_makehighlight(textlist[i], &high);
		if ((high.status&HIGHTYPE) != HIGHTEXT) continue;

		/* get object and movement amount */
		us_gethighaddrtype(&high, &addr, &type);

		/* undraw the text */
		if (type == VPORTPROTO)
		{
			ni = ((PORTPROTO *)addr)->subnodeinst;
			for(j=0; list[j] != NOGEOM; j++)
				if (list[j]->entryisnode && list[j]->entryaddr.ni == ni) break;
			if (list[j] != NOGEOM) continue;

			startobjectchange((INTBIG)ni, VNODEINST);
			if (us_nodemoveswithtext(&high))
			{
				modifynodeinst(ni, dx, dy, dx, dy, 0, 0);
				endobjectchange((INTBIG)ni, VNODEINST);
				continue;
			}
			if (ni->transpose != 0)
				makeangle(ni->rotation, ni->transpose, trans); else
					makeangle((3600-ni->rotation)%3600, 0, trans);
			xform(dx, dy, &dx, &dy, trans);
		} else
		{
			startobjectchange(addr, type);
			if (type == VNODEINST)
			{
				ni = (NODEINST *)addr;
				for(j=0; list[j] != NOGEOM; j++)
					if (list[j]->entryisnode && list[j]->entryaddr.ni == ni) break;
				if (list[j] != NOGEOM) continue;

				if (us_nodemoveswithtext(&high))
				{
					modifynodeinst(ni, dx, dy, dx, dy, 0, 0);
					endobjectchange((INTBIG)ni, VNODEINST);
					continue;
				}
				if (ni->transpose != 0)
					makeangle(ni->rotation, ni->transpose, trans); else
						makeangle((3600-ni->rotation)%3600, 0, trans);
				xform(dx, dy, &dx, &dy, trans);
			}
		}
		if (type == VNODEPROTO && high.fromvar != NOVARIABLE)
			us_undrawcellvariable(high.fromvar, (NODEPROTO *)addr);

		/* set the new descriptor on the text */
		lambda = el_curlib->lambda[us_hightech(&high)->techindex];
		dx = dx*4/lambda;   dy = dy*4/lambda;
		us_gethighdescript(&high, descript);
		dx += TDGETXOFF(descript);
		dy += TDGETYOFF(descript);
		us_setdescriptoffset(descript, dx, dy);
		us_modifytextdescript(&high, descript);

		/* redisplay the text */
		if (type == VNODEPROTO && high.fromvar != NOVARIABLE)
			us_drawcellvariable(high.fromvar, (NODEPROTO *)addr);
		if (type == VPORTPROTO)
		{
			endobjectchange((INTBIG)ni, VNODEINST);
		} else
		{
			endobjectchange(addr, type);
		}
		us_addhighlight(&high);

		/* modify all higher-level nodes if port moved */
		if (high.fromvar == NOVARIABLE && high.fromport != NOPORTPROTO)
		{
			for(ni = high.fromport->parent->firstinst; ni != NONODEINST; ni = ni->nextinst)
			{
				if ((ni->userbits&NEXPAND) != 0 &&
					(high.fromport->userbits&PORTDRAWN) == 0) continue;
				startobjectchange((INTBIG)ni, VNODEINST);
				endobjectchange((INTBIG)ni, VNODEINST);
			}
		}
	}
}

/*
 * Routine to find a single selected snap point and return it in (x, y).
 * Returns false if there is not a single snap point.
 */
BOOLEAN us_getonesnappoint(INTBIG *x, INTBIG *y)
{
	REGISTER INTBIG snapcount;
	REGISTER VARIABLE *var;
	REGISTER INTBIG len, i;
	HIGHLIGHT thishigh;

	snapcount = 0;
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var != NOVARIABLE)
	{
		len = getlength(var);
		for(i=0; i<len; i++)
		{
			if (!us_makehighlight(((CHAR **)var->addr)[i], &thishigh))
			{
				if ((thishigh.status&HIGHSNAP) != 0)
				{
					us_getsnappoint(&thishigh, x, y);
					snapcount++;
				}
			}
		}
	}
	if (snapcount == 1) return(TRUE);
	return(FALSE);
}

/*
 * routine to draw the highlighting for module "high".  The highlighting is
 * drawn on if "on" is HIGHLIT, off if "on" is ALLOFF.
 */
void us_sethighlight(HIGHLIGHT *high, INTBIG on)
{
	INTBIG xw, yw, xc, yc, lx, hx, ly, hy, addr, type;
	INTBIG style;
	BOOLEAN verbose, nobbox;
	REGISTER WINDOWPART *w;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 5, us_tool->cluster);

	/* handle drawing bounding boxes */
	if ((high->status&HIGHTYPE) == HIGHBBOX)
	{
		maketruerectpoly(high->stalx, high->stahx, high->staly, high->stahy, poly);
		poly->desc = &us_hbox;
		poly->style = CLOSEDRECT;
		us_hbox.col = on;
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if ((w->state&WINDOWTYPE) != DISPWINDOW &&
				(w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
			if (w->curnodeproto != high->cell) continue;
			us_showpoly(poly, w);
		}
		return;
	}

	/* handle drawing lines */
	if ((high->status&HIGHTYPE) == HIGHLINE)
	{
		poly->xv[0] = high->stalx;   poly->yv[0] = high->staly;
		poly->xv[1] = high->stahx;   poly->yv[1] = high->stahy;
		poly->count = 2;
		poly->desc = &us_hbox;
		poly->style = VECTORS;
		us_hbox.col = on;
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if ((w->state&WINDOWTYPE) != DISPWINDOW &&
				(w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
			if (w->curnodeproto != high->cell) continue;
			us_showpoly(poly, w);
		}
		return;
	}

	/* handle text outlines */
	if ((high->status&HIGHTYPE) == HIGHTEXT)
	{
		/* determine center of text */
		us_gethightextcenter(high, &xc, &yc, &style);

		/* initialize polygon */
		poly->layer = -1;
		poly->desc = &us_hbox;
		us_hbox.col = on;

		/* display in every appropriate window */
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if ((w->state&WINDOWTYPE) != DISPWINDOW &&
				(w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
			if (w->curnodeproto != high->cell) continue;

			us_gethightextsize(high, &xw, &yw, w);

			/* construct polygon */
			us_gethighaddrtype(high, &addr, &type);
			(void)us_getobjectinfo(addr, type, &lx, &hx, &ly, &hy);
			us_buildtexthighpoly(lx, hx, ly, hy, xc, yc, xw, yw, style, poly);
			us_showpoly(poly, w);
		}
		return;
	}

	/* quit if nothing to draw */
	if ((high->status&HIGHFROM) == 0 || high->fromgeom == NOGEOM) return;

	/* highlight "from" object */
	if ((high->status&HIGHEXTRA) != 0) verbose = TRUE; else verbose = FALSE;
	if ((high->status&HIGHNOBOX) != 0) nobbox = TRUE; else nobbox = FALSE;
	us_highlighteverywhere(high->fromgeom, high->fromport, high->frompoint,
		verbose, on, nobbox);

	/* draw the snap point if set */
	if ((high->status&HIGHSNAP) != 0)
	{
		us_getsnappoint(high, &poly->xv[0], &poly->yv[0]);
		poly->count = 1;
		poly->desc = &us_hbox;
		poly->style = BIGCROSS;
		us_hbox.col = on;
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if ((w->state&WINDOWTYPE) != DISPWINDOW &&
				(w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
			if (w->curnodeproto != high->cell) continue;
			us_showpoly(poly, w);
		}
	}
}

/*
 * routine to build a polygon in "poly" that describes the highlighting of
 * a text object whose grab point is (xc,yc), size is (xw,yw), graphics
 * style is in "style" (either TEXTCENT, TEXTBOT, ...) and whose attached
 * object is "geom".
 */
void us_buildtexthighpoly(INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy,
	INTBIG xc, INTBIG yc, INTBIG xw, INTBIG yw, INTBIG style, POLYGON *poly)
{
	REGISTER INTBIG xsh, ysh;

	/* compute appropriate highlight */
	switch (style)
	{
		case TEXTCENT:
		case TEXTBOX:
			poly->xv[0] = xc - xw/2;   poly->yv[0] = yc - yw/2;
			poly->xv[1] = xc + xw/2;   poly->yv[1] = yc + yw/2;
			poly->xv[2] = xc - xw/2;   poly->yv[2] = yc + yw/2;
			poly->xv[3] = xc + xw/2;   poly->yv[3] = yc - yw/2;
			poly->style = VECTORS;     poly->count = 4;
			if (style == TEXTCENT) break;
			if (poly->limit < 12) (void)extendpolygon(poly, 12);
			xsh = (hx - lx) / 6;
			ysh = (hy - ly) / 6;
			poly->xv[4] = lx + xsh;    poly->yv[4] = ly;
			poly->xv[5] = hx - xsh;    poly->yv[5] = ly;
			poly->xv[6] = lx + xsh;    poly->yv[6] = hy;
			poly->xv[7] = hx - xsh;    poly->yv[7] = hy;
			poly->xv[8] = lx;          poly->yv[8] = ly + ysh;
			poly->xv[9] = lx;          poly->yv[9] = hy - ysh;
			poly->xv[10] = hx;         poly->yv[10] = ly + ysh;
			poly->xv[11] = hx;         poly->yv[11] = hy - ysh;
			poly->count = 12;
			break;
		case TEXTBOT:
			poly->xv[0] = xc - xw/2;   poly->yv[0] = yc + yw;
			poly->xv[1] = xc - xw/2;   poly->yv[1] = yc;
			poly->xv[2] = xc + xw/2;   poly->yv[2] = yc;
			poly->xv[3] = xc + xw/2;   poly->yv[3] = yc + yw;
			poly->style = OPENED;      poly->count = 4;
			break;
		case TEXTTOP:
			poly->xv[0] = xc - xw/2;   poly->yv[0] = yc - yw;
			poly->xv[1] = xc - xw/2;   poly->yv[1] = yc;
			poly->xv[2] = xc + xw/2;   poly->yv[2] = yc;
			poly->xv[3] = xc + xw/2;   poly->yv[3] = yc - yw;
			poly->style = OPENED;      poly->count = 4;
			break;
		case TEXTLEFT:
			poly->xv[0] = xc + xw;     poly->yv[0] = yc + yw/2;
			poly->xv[1] = xc;          poly->yv[1] = yc + yw/2;
			poly->xv[2] = xc;          poly->yv[2] = yc - yw/2;
			poly->xv[3] = xc + xw;     poly->yv[3] = yc - yw/2;
			poly->style = OPENED;      poly->count = 4;
			break;
		case TEXTRIGHT:
			poly->xv[0] = xc - xw;     poly->yv[0] = yc + yw/2;
			poly->xv[1] = xc;          poly->yv[1] = yc + yw/2;
			poly->xv[2] = xc;          poly->yv[2] = yc - yw/2;
			poly->xv[3] = xc - xw;     poly->yv[3] = yc - yw/2;
			poly->style = OPENED;      poly->count = 4;
			break;
		case TEXTTOPLEFT:
			poly->xv[0] = xc + xw;     poly->yv[0] = yc;
			poly->xv[1] = xc;          poly->yv[1] = yc;
			poly->xv[2] = xc;          poly->yv[2] = yc - yw;
			poly->style = OPENED;      poly->count = 3;
			break;
		case TEXTBOTLEFT:
			poly->xv[0] = xc;          poly->yv[0] = yc + yw;
			poly->xv[1] = xc;          poly->yv[1] = yc;
			poly->xv[2] = xc + xw;     poly->yv[2] = yc;
			poly->style = OPENED;      poly->count = 3;
			break;
		case TEXTTOPRIGHT:
			poly->xv[0] = xc - xw;     poly->yv[0] = yc;
			poly->xv[1] = xc;          poly->yv[1] = yc;
			poly->xv[2] = xc;          poly->yv[2] = yc - yw;
			poly->style = OPENED;      poly->count = 3;
			break;
		case TEXTBOTRIGHT:
			poly->xv[0] = xc - xw;     poly->yv[0] = yc;
			poly->xv[1] = xc;          poly->yv[1] = yc;
			poly->xv[2] = xc;          poly->yv[2] = yc + yw;
			poly->style = OPENED;      poly->count = 3;
			break;
	}
}

/*
 * general routine to change the highlighting of an object.  Every displayed
 * occurrence of geometry module "look" will have its highlighting drawn.
 * The highlighting will be in color "color".
 * If "port" is not NOPORTPROTO and the geometry module points to
 * a nodeinst, that port will also be highlighted.  If "point" is nonzero,
 * then that point/line of the trace will be highlighted.  If "verbose" is
 * true, the object will be highlighted with extra information.  If
 * "nobbox" is true, don't draw the basic box around the object.
 */
void us_highlighteverywhere(GEOM *look, PORTPROTO *port, INTBIG point, BOOLEAN verbose, INTBIG color,
	BOOLEAN nobbox)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER NODEPROTO *par;
	REGISTER INTBIG i, x, y;
	REGISTER WINDOWPART *w;
	REGISTER VARIABLE *var;
	static POLYGON *poly = NOPOLYGON;
	XARRAY trans;
	INTBIG lx, hx, ly, hy;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* sensibility check to prevent null highlighting */
	if (look->entryisnode)
	{
		if (port == NOPORTPROTO && !verbose && point == 0) nobbox = FALSE;
	} else
	{
		point = 0;
		if (!verbose) nobbox = FALSE;
	}

	/* determine parent, compute polygon */
	par = geomparent(look);
	if (look->entryisnode)
	{
		ni = look->entryaddr.ni;
		makerot(ni, trans);
		nodesizeoffset(ni, &lx, &ly, &hx, &hy);
		makerectpoly(ni->lowx+lx, ni->highx-hx, ni->lowy+ly, ni->highy-hy, poly);
		poly->style = FILLED;
		xformpoly(poly, trans);
	} else
	{
		ai = look->entryaddr.ai;
		i = ai->width - arcwidthoffset(ai);
		if (curvedarcoutline(ai, poly, CLOSED, i))
			makearcpoly(ai->length, i, ai, poly, FILLED);
	}

	/* figure out the line type for this highlighting */
	poly->desc = &us_hbox;
	poly->desc->col = color;
	poly->style = OPENEDO1;

	/* if extra information is being un-drawn, show it now */
	if (verbose && color == ALLOFF)
		us_highlightverbose(look, port, color, par, poly);

	/* add the final point to close the polygon */
	if (poly->limit < poly->count+1) (void)extendpolygon(poly, poly->count+1);
	poly->xv[poly->count] = poly->xv[0];
	poly->yv[poly->count] = poly->yv[0];
	poly->count++;

	/* if all points are the same, draw as a cross */
	for(i=1; i<poly->count; i++)
		if (poly->xv[i-1] != poly->xv[i] || poly->yv[i-1] != poly->yv[i]) break;
	if (i >= poly->count) poly->style = CROSS;

	/* loop through windows and draw the highlighting */
	if (!nobbox)
	{
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if ((w->state&WINDOWTYPE) != DISPWINDOW &&
				(w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
			if (w->curnodeproto != par) continue;
			us_showpoly(poly, w);
		}
	}

	/* remove that point to make the polygon centered */
	poly->count--;

	/* if extra information is being drawn, show it now */
	if (verbose && color != ALLOFF)
		us_highlightverbose(look, port, color, par, poly);

	/* draw the port prototype if requested */
	if (look->entryisnode && port != NOPORTPROTO)
	{
		/* compute the port bounds */
		shapeportpoly(ni, port, poly, FALSE);

		/* compute graphics for the port outline */
		poly->desc = &us_arbit;
		us_arbit.bits = LAYERH;   us_arbit.col = color;

		/* see if the polygon is a single point */
		for(i=1; i<poly->count; i++)
			if (poly->xv[i] != poly->xv[i-1] || poly->yv[i] != poly->yv[i-1]) break;
		if (i < poly->count)
		{
			/* not a single point, draw its outline */
			if (poly->style == FILLEDRECT) poly->style = CLOSEDRECT; else
				if (poly->style == FILLED) poly->style = CLOSED; else
					if (poly->style == DISC) poly->style = CIRCLE;
		} else
		{
			/* single point port: make it a cross */
			poly->count = 1;
			poly->style = CROSS;
		}

		/* draw the port */
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if ((w->state&WINDOWTYPE) != DISPWINDOW &&
				(w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
			if (w->curnodeproto != par) continue;
			us_showpoly(poly, w);
		}
	}

	/* draw the point or line on the trace if requested */
	if (point != 0)
	{
		var = gettrace(ni);
		if (var == NOVARIABLE) return;
		x = (ni->highx + ni->lowx) / 2;   y = (ni->highy + ni->lowy) / 2;

		xform(((INTBIG *)var->addr)[(point-1)*2]+x, ((INTBIG *)var->addr)[(point-1)*2+1]+y,
			&poly->xv[0], &poly->yv[0], trans);
		poly->desc = &us_arbit;
		us_arbit.bits = LAYERH;   us_arbit.col = color;
		poly->count = 1;
		poly->style = CROSS;
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if ((w->state&WINDOWTYPE) != DISPWINDOW &&
				(w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
			if (w->curnodeproto != par) continue;
			us_showpoly(poly, w);
		}

		if (point*2 < getlength(var))
		{
			xform(((INTBIG *)var->addr)[point*2]+x, ((INTBIG *)var->addr)[point*2+1]+y,
				&poly->xv[1], &poly->yv[1], trans);
			poly->desc = &us_arbit;
			us_arbit.bits = LAYERH;   us_arbit.col = color;
			poly->count = 2;
			poly->style = OPENED;
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
			{
				if ((w->state&WINDOWTYPE) != DISPWINDOW &&
					(w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
				if (w->curnodeproto != par) continue;
				us_showpoly(poly, w);
			}
		}
	}
}

/*
 * Routine to draw extra verbose highlighting on object "look" (if it is a node, then
 * "pp" is the port on that node).  The color is "color", the parent cell is "par", and
 * if it is an arc, the polygon for that arc is "arcpoly".
 */
void us_highlightverbose(GEOM *look, PORTPROTO *pp, INTBIG color, NODEPROTO *par, POLYGON *arcpoly)
{
	static POLYGON *poly = NOPOLYGON;
	REGISTER NODEINST *ni, *hni;
	REGISTER ARCINST *ai, *oai;
	REGISTER WINDOWPART *w;
	REGISTER PORTARCINST *pi;
	REGISTER INTBIG i, thisend;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);
	poly->desc = &us_hbox;
	poly->desc->col = color;

	if (look->entryisnode)
	{
		/* handle verbose highlighting of nodes */
		if (pp == NOPORTPROTO) return;
		if (pp->network == NONETWORK) return;

		hni = look->entryaddr.ni;
		for(ni = par->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			ni->userbits &= ~NODEFLAGBIT;
		for(ai = par->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			ai->userbits &= ~ARCFLAGBIT;

		/* determine which arcs should be highlighted */
		for(pi = hni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			if (pi->proto->network != pp->network) continue;
			ai = pi->conarcinst;
			ai->userbits |= ARCFLAGBIT;
			if (ai->network != NONETWORK)
			{
				for(oai = par->firstarcinst; oai != NOARCINST; oai = oai->nextarcinst)
				{
					if (oai->network != ai->network) continue;
					oai->userbits |= ARCFLAGBIT;
					oai->end[0].nodeinst->userbits |= NODEFLAGBIT;
					oai->end[1].nodeinst->userbits |= NODEFLAGBIT;
				}
			}
		}

		/* draw lines along all of the arcs on the network */
		for(ai = par->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		{
			if ((ai->userbits&ARCFLAGBIT) == 0) continue;
			poly->xv[0] = ai->end[0].xpos;
			poly->yv[0] = ai->end[0].ypos;
			poly->xv[1] = ai->end[1].xpos;
			poly->yv[1] = ai->end[1].ypos;
			poly->count = 2;
			poly->style = OPENEDT2;
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
			{
				if ((w->state&WINDOWTYPE) != DISPWINDOW &&
					(w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
				if (w->curnodeproto != par) continue;
				us_showpoly(poly, w);
			}
		}

		/* draw dots in all connected nodes */
		for(ni = par->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if (ni == hni) continue;
			if ((ni->userbits&NODEFLAGBIT) == 0) continue;
			poly->xv[0] = (ni->lowx + ni->highx) / 2;
			poly->yv[0] = (ni->lowy + ni->highy) / 2;
			poly->xv[1] = poly->xv[0] + el_curlib->lambda[el_curtech->techindex]/4;
			poly->yv[1] = poly->yv[0];
			poly->count = 2;
			poly->style = DISC;
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
			{
				if ((w->state&WINDOWTYPE) != DISPWINDOW &&
					(w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
				if (w->curnodeproto != par) continue;
				poly->xv[1] = poly->xv[0] + (INTBIG)(DOTRADIUS / w->scalex);
				us_showpoly(poly, w);
			}

			/* connect the center dots to the input arcs */
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			{
				ai = pi->conarcinst;
				if ((ai->userbits&ARCFLAGBIT) == 0) continue;
				if (ai->end[0].portarcinst == pi) thisend = 0; else thisend = 1;
				if (ai->end[thisend].xpos != poly->xv[0] ||
					ai->end[thisend].ypos != poly->yv[0])
				{
					poly->xv[1] = ai->end[thisend].xpos;
					poly->yv[1] = ai->end[thisend].ypos;
					poly->count = 2;
					poly->style = OPENEDT1;
					for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
					{
						if ((w->state&WINDOWTYPE) != DISPWINDOW &&
							(w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
						if (w->curnodeproto != par) continue;
						us_showpoly(poly, w);
					}
				}
			}
		}
		return;
	}

	/* handle verbose highlighting of arcs */
	ai = look->entryaddr.ai;

	/* get a description of the constraints on the arc */
	poly->string = (CHAR *)(*(el_curconstraint->request))(x_("describearc"), (INTBIG)ai);

	/* quit now if there is no verbose text to print */
	if (poly->string == 0 || *poly->string == 0) return;

	/* determine the location of this text */
	if (arcpoly->count != 4)
	{
		/* presume curved arc outline and determine center four points */
		i = arcpoly->count/2;
		poly->xv[0] = (arcpoly->xv[i/2] + arcpoly->xv[arcpoly->count-i/2-1] +
			arcpoly->xv[(i-1)/2] + arcpoly->xv[arcpoly->count-(i-1)/2-1]) / 4;
		poly->yv[0] = (arcpoly->yv[i/2] + arcpoly->yv[arcpoly->count-i/2-1] +
			arcpoly->yv[(i-1)/2] + arcpoly->yv[arcpoly->count-(i-1)/2-1]) / 4;
	} else
	{
		/* use straight arc: simply get center */
		poly->xv[0] = (ai->end[0].xpos + ai->end[1].xpos) / 2;
		poly->yv[0] = (ai->end[0].ypos + ai->end[1].ypos) / 2;
	}
	poly->style = TEXTCENT;
	if (ai->width == 0)
	{
		if (ai->end[0].ypos == ai->end[1].ypos) poly->style = TEXTBOT; else
			if (ai->end[0].xpos == ai->end[1].xpos) poly->style = TEXTLEFT; else
				if ((ai->end[0].xpos-ai->end[1].xpos) * (ai->end[0].ypos-ai->end[1].ypos) > 0)
					poly->style = TEXTBOTRIGHT; else
						poly->style = TEXTBOTLEFT;
	}
	poly->count = 1;
	TDCLEAR(poly->textdescript);
	TDSETSIZE(poly->textdescript, TXTSETQLAMBDA(4));
	poly->tech = ai->parent->tech;
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		if ((w->state&WINDOWTYPE) != DISPWINDOW &&
			(w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
		if (w->curnodeproto != par) continue;
		(void)us_showpoly(poly, w);
	}
}

/*
 * routine to push the currently highlighted configuration onto a stack
 */
void us_pushhighlight(void)
{
	REGISTER VARIABLE *var;
	REGISTER INTBIG len, i;
	REGISTER CHAR **list;
	CHAR *one[1];
	REGISTER void *infstr;

	/* get what is highlighted */
	infstr = initinfstr();
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var != NOVARIABLE)
	{
		len = getlength(var);
		for(i=0; i<len; i++)
		{
			if (i != 0) addtoinfstr(infstr, '\n');
			addstringtoinfstr(infstr, ((CHAR **)var->addr)[i]);
		}
	}

	/* now add this to the stack variable */
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightstackkey);
	if (var == NOVARIABLE)
	{
		one[0] = returninfstr(infstr);
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_highlightstackkey, (INTBIG)one,
			VSTRING|VISARRAY|(1<<VLENGTHSH)|VDONTSAVE);
		return;
	}

	len = getlength(var);
	list = (CHAR **)emalloc(((len+1) * (sizeof (CHAR *))), el_tempcluster);
	if (list == 0) return;
	for(i=0; i<len; i++)
		(void)allocstring(&list[i], ((CHAR **)var->addr)[i], el_tempcluster);
	list[len] = returninfstr(infstr);
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_highlightstackkey, (INTBIG)list,
		VSTRING|VISARRAY|((len+1)<<VLENGTHSH)|VDONTSAVE);
	for(i=0; i<len; i++) efree(list[i]);
	efree((CHAR *)list);
}

/*
 * routine to pop the stack onto the currently highlighted configuration.
 */
void us_pophighlight(BOOLEAN clearsnap)
{
	REGISTER VARIABLE *var;
	REGISTER CHAR **vaddr;
	REGISTER INTBIG len, i;
	REGISTER CHAR **list;

	/* get the stack variable */
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightstackkey);
	if (var == NOVARIABLE) return;
	len = getlength(var);
	vaddr = (CHAR **)var->addr;

	/* set the configuration */
	us_setmultiplehighlight(vaddr[len-1], clearsnap);

	/* shorten the stack */
	if (len == 1) (void)delvalkey((INTBIG)us_tool, VTOOL, us_highlightstackkey); else
	{
		len--;
		list = (CHAR **)emalloc((len * (sizeof (CHAR *))), el_tempcluster);
		if (list == 0) return;
		for(i=0; i<len; i++)
			(void)allocstring(&list[i], vaddr[i], el_tempcluster);
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_highlightstackkey, (INTBIG)list,
			VSTRING|VISARRAY|(len<<VLENGTHSH)|VDONTSAVE);
		for(i=0; i<len; i++) efree(list[i]);
		efree((CHAR *)list);
	}
}

/*
 * routine to highlight the multiple highlight information in "str".
 */
void us_setmultiplehighlight(CHAR *str, BOOLEAN clearsnap)
{
	CHAR *pp;
	REGISTER CHAR *line, **list;
	REGISTER INTBIG total, i;
	HIGHLIGHT high;
	REGISTER void *infstr;

	/* count the number of highlights */
	pp = str;
	for(total=0; ; total++)
	{
		line = getkeyword(&pp, x_("\n"));
		if (line == NOSTRING) break;
		if (*line == 0) break;
		(void)tonextchar(&pp);
	}
	if (total == 0) return;

	/* make space for all of the highlight strings */
	list = (CHAR **)emalloc((total * (sizeof (CHAR *))), el_tempcluster);
	if (list == 0) return;

	/* fill the list */
	pp = str;
	for(total=0; ; )
	{
		line = getkeyword(&pp, x_("\n"));
		if (line == NOSTRING) break;
		if (*line == 0) break;

		if (clearsnap)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, line);
			if (!us_makehighlight(returninfstr(infstr), &high))
			{
				high.status &= ~(HIGHSNAP | HIGHSNAPTAN | HIGHSNAPPERP);
				line = us_makehighlightstring(&high);
				(void)allocstring(&list[total++], line, el_tempcluster);
			}
		} else
		{
			(void)allocstring(&list[total++], line, el_tempcluster);
		}
		if (tonextchar(&pp) != '\n') break;
	}

	/* set the multiple list */
	if (total == 0) return;
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_highlightedkey, (INTBIG)list,
		VSTRING|VISARRAY|(total<<VLENGTHSH)|VDONTSAVE);

	for(i=0; i<total; i++) efree(list[i]);
	efree((CHAR *)list);
	us_makecurrentobject();
}

/*
 * routine to set the current prototype from the current selection
 */
void us_makecurrentobject(void)
{
	REGISTER NODEPROTO *np;
	REGISTER GEOM *from;
	HIGHLIGHT high;
	REGISTER VARIABLE *var;

	/* see what is highlighted */
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);

	/* there must be exactly 1 object highlighted */
	if (var == NOVARIABLE || getlength(var) != 1)
	{
		if ((us_state&NONPERSISTENTCURNODE) != 0) us_setnodeproto(NONODEPROTO);
		return;
	}

	/* get information about the highlighted object */
	if (us_makehighlight(((CHAR **)var->addr)[0], &high)) return;
	if ((high.status&HIGHTYPE) != HIGHFROM && (high.status&HIGHTYPE) != HIGHTEXT)
	{
		if ((us_state&NONPERSISTENTCURNODE) != 0) us_setnodeproto(NONODEPROTO);
		return;
	}
	from = high.fromgeom;
	if (from == NOGEOM)
	{
		us_setnodeproto(NONODEPROTO);
		return;
	}

	/* if an arc is selected, show it */
	if (!from->entryisnode)
	{
		us_setarcproto(from->entryaddr.ai->proto, TRUE);
		if ((us_state&NONPERSISTENTCURNODE) != 0) us_setnodeproto(NONODEPROTO);
		return;
	}

	/* a node is selected */
	np = from->entryaddr.ni->proto;
	us_setnodeproto(np);
}

/*
 * routine to convert the string in "str" into the highlight module "high".
 * returns true on error
 */
BOOLEAN us_makehighlight(CHAR *str, HIGHLIGHT *high)
{
	CHAR *pt;
	REGISTER CHAR *keyword;
	REGISTER INTBIG addr, type;

	pt = str;

	/* look for the "CELL=" keyword */
	keyword = getkeyword(&pt, x_("="));
	if (keyword == NOSTRING) return(TRUE);
	if (namesame(keyword, x_("CELL")) != 0) return(TRUE);
	if (tonextchar(&pt) != '=') return(TRUE);

	/* get the cell name */
	keyword = getkeyword(&pt, x_(" \t"));
	if (keyword == NOSTRING) return(TRUE);
	high->cell = getnodeproto(keyword);
	if (high->cell == NONODEPROTO) return(TRUE);

	/* get the type of highlight */
	keyword = getkeyword(&pt, x_("="));
	if (keyword == NOSTRING) return(TRUE);
	if (tonextchar(&pt) != '=') return(TRUE);
	if (namesame(keyword, x_("FROM")) == 0)
	{
		/* parse the three values: geom/port/point */
		high->status = HIGHFROM;
		high->fromvar = NOVARIABLE;
		high->fromvarnoeval = NOVARIABLE;
		keyword = getkeyword(&pt, x_(";"));
		if (keyword == NOSTRING) return(TRUE);
		high->fromgeom = (GEOM *)myatoi(keyword);
		if (tonextchar(&pt) != ';') return(TRUE);
		keyword = getkeyword(&pt, x_(";"));
		if (keyword == NOSTRING) return(TRUE);
		high->fromport = (PORTPROTO *)myatoi(keyword);
		if (tonextchar(&pt) != ';') return(TRUE);
		keyword = getkeyword(&pt, x_(";"));
		if (keyword == NOSTRING) return(TRUE);
		high->frompoint = myatoi(keyword);
	} else if (namesame(keyword, x_("AREA")) == 0 || namesame(keyword, x_("LINE")) == 0)
	{
		if (namesame(keyword, x_("AREA")) == 0) high->status = HIGHBBOX; else
			high->status = HIGHLINE;
		keyword = getkeyword(&pt, x_(","));
		if (keyword == NOSTRING) return(TRUE);
		high->stalx = myatoi(keyword);
		if (tonextchar(&pt) != ',') return(TRUE);
		keyword = getkeyword(&pt, x_(","));
		if (keyword == NOSTRING) return(TRUE);
		high->stahx = myatoi(keyword);
		if (tonextchar(&pt) != ',') return(TRUE);
		keyword = getkeyword(&pt, x_(","));
		if (keyword == NOSTRING) return(TRUE);
		high->staly = myatoi(keyword);
		if (tonextchar(&pt) != ',') return(TRUE);
		keyword = getkeyword(&pt, x_(","));
		if (keyword == NOSTRING) return(TRUE);
		high->stahy = myatoi(keyword);
	} else if (namesame(keyword, x_("TEXT")) == 0)
	{
		/* parse the three values: geom/port/var */
		high->status = HIGHTEXT;
		high->frompoint = 0;
		keyword = getkeyword(&pt, x_(";"));
		if (keyword == NOSTRING) return(TRUE);
		high->fromgeom = (GEOM *)myatoi(keyword);
		if (tonextchar(&pt) != ';') return(TRUE);
		keyword = getkeyword(&pt, x_(";"));
		if (keyword == NOSTRING) return(TRUE);
		high->fromport = (PORTPROTO *)myatoi(keyword);
		if (tonextchar(&pt) != ';') return(TRUE);
		keyword = getkeyword(&pt, x_(";"));
		if (keyword == NOSTRING) return(TRUE);
		if (*keyword == '-') high->fromvarnoeval = high->fromvar = NOVARIABLE; else
		{
			if (high->fromport != NOPORTPROTO)
			{
				addr = (INTBIG)high->fromport;
				type = VPORTPROTO;
			} else if (high->fromgeom != NOGEOM)
			{
				if (high->fromgeom->entryisnode)
				{
					addr = (INTBIG)high->fromgeom->entryaddr.ni;
					type = VNODEINST;
				} else
				{
					addr = (INTBIG)high->fromgeom->entryaddr.ai;
					type = VARCINST;
				}
			} else
			{
				addr = (INTBIG)high->cell;
				type = VNODEPROTO;
			}
			high->fromvarnoeval = getvalnoeval(addr, type, -1, keyword);
			high->fromvar = evalvar(high->fromvarnoeval, addr, type);
		}
	} else return(TRUE);

	/* parse any additional keywords */
	for(;;)
	{
		if (tonextchar(&pt) == 0) break;
		keyword = getkeyword(&pt, x_(";,="));
		if (keyword == NOSTRING) return(TRUE);
		if ((namesame(keyword, x_("SNAP")) == 0 || namesame(keyword, x_("SNAPTAN")) == 0 ||
			namesame(keyword, x_("SNAPPERP")) == 0) && tonextchar(&pt) == '=')
		{
			high->status |= HIGHSNAP;
			if (namesame(keyword, x_("SNAPTAN")) == 0) high->status |= HIGHSNAPTAN;
			if (namesame(keyword, x_("SNAPPERP")) == 0) high->status |= HIGHSNAPPERP;
			keyword = getkeyword(&pt, x_(","));
			if (keyword == NOSTRING) return(TRUE);
			high->snapx = myatoi(keyword);
			if (tonextchar(&pt) != ',') return(TRUE);
			keyword = getkeyword(&pt, x_(";,"));
			if (keyword == NOSTRING) return(TRUE);
			high->snapy = myatoi(keyword);
			continue;
		}
		if (namesame(keyword, x_("EXTRA")) == 0)
		{
			high->status |= HIGHEXTRA;
			continue;
		}
		if (namesame(keyword, x_("NOBBOX")) == 0)
		{
			high->status |= HIGHNOBOX;
			continue;
		}
		return(TRUE);
	}
	return(FALSE);
}

/*
 * routine to convert the highlight in "high" into a string which is returned.
 */
CHAR *us_makehighlightstring(HIGHLIGHT *high)
{
	REGISTER void *infstr;

	infstr = initinfstr();
	formatinfstr(infstr, x_("CELL=%s"), describenodeproto(high->cell));
	switch (high->status&HIGHTYPE)
	{
		case HIGHFROM:
			formatinfstr(infstr, x_(" FROM=0%lo;"), (UINTBIG)high->fromgeom);
			if (high->fromport == NOPORTPROTO) addstringtoinfstr(infstr, x_("-1;")); else
				formatinfstr(infstr, x_("0%lo;"), (UINTBIG)high->fromport);
			formatinfstr(infstr, x_("%ld"), high->frompoint);
			if ((high->status&HIGHEXTRA) != 0) addstringtoinfstr(infstr, x_(";EXTRA"));
			if ((high->status&HIGHNOBOX) != 0) addstringtoinfstr(infstr, x_(";NOBBOX"));
			break;
		case HIGHTEXT:
			formatinfstr(infstr, x_(" TEXT=0%lo;"), (UINTBIG)high->fromgeom);
			if (high->fromport == NOPORTPROTO) addstringtoinfstr(infstr, x_("-1;")); else
				formatinfstr(infstr, x_("0%lo;"), (UINTBIG)high->fromport);
			if (high->fromvar == NOVARIABLE) addstringtoinfstr(infstr, x_("-")); else
				addstringtoinfstr(infstr, makename(high->fromvar->key));
			break;
		case HIGHBBOX:
			formatinfstr(infstr, x_(" AREA=%ld,%ld,%ld,%ld"), high->stalx, high->stahx, high->staly, high->stahy);
			break;
		case HIGHLINE:
			formatinfstr(infstr, x_(" LINE=%ld,%ld,%ld,%ld"), high->stalx, high->stahx, high->staly, high->stahy);
			break;
	}
	if ((high->status&HIGHSNAP) != 0)
	{
		if ((high->status&HIGHSNAPTAN) != 0) addstringtoinfstr(infstr, x_(";SNAPTAN=")); else
			if ((high->status&HIGHSNAPPERP) != 0) addstringtoinfstr(infstr, x_(";SNAPPERP=")); else
				addstringtoinfstr(infstr, x_(";SNAP="));
		formatinfstr(infstr, x_("%ld,%ld"), high->snapx, high->snapy);
	}
	return(returninfstr(infstr));
}

/*
 * routine to determine the center of the highlighted text described by
 * "high".  The coordinates are placed in (xc, yc), and the appropriate
 * polygon style is placed in "style".
 */
void us_gethightextcenter(HIGHLIGHT *high, INTBIG *xc, INTBIG *yc, INTBIG *style)
{
	static POLYGON *poly = NOPOLYGON;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER NODEPROTO *cell;
	XARRAY trans;
	INTBIG newxc, newyc;
	REGISTER INTSML saverot, savetrn;
	REGISTER INTBIG lambda;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	/* get polygon */
	(void)needstaticpolygon(&poly, 1, us_tool->cluster);

	poly->count = 1;
	poly->style = FILLED;
	if (high->fromvar != NOVARIABLE)
	{
		TDCOPY(descript, high->fromvar->textdescript);
		if (high->fromport != NOPORTPROTO)
		{
			ni = high->fromport->subnodeinst;
			saverot = ni->rotation;   savetrn = ni->transpose;
			ni->rotation = ni->transpose = 0;
			portposition(ni, high->fromport->subportproto,
				&poly->xv[0], &poly->yv[0]);
			ni->rotation = saverot;   ni->transpose = savetrn;
			adjustdisoffset((INTBIG)high->fromport, VPORTPROTO, high->cell->tech,
				poly, descript);
			makerot(high->fromport->subnodeinst, trans);
			xformpoly(poly, trans);
		} else if (high->fromgeom != NOGEOM)
		{
			if (high->fromgeom->entryisnode)
			{
				ni = high->fromgeom->entryaddr.ni;
				poly->xv[0] = (ni->lowx + ni->highx) / 2;
				poly->yv[0] = (ni->lowy + ni->highy) / 2;
				adjustdisoffset((INTBIG)ni, VNODEINST, ni->proto->tech, poly, descript);
				makerot(ni, trans);
				xformpoly(poly, trans);
			} else
			{
				ai = high->fromgeom->entryaddr.ai;
				poly->xv[0] = (ai->end[0].xpos + ai->end[1].xpos) / 2;
				poly->yv[0] = (ai->end[0].ypos + ai->end[1].ypos) / 2;
				adjustdisoffset((INTBIG)ai, VARCINST, ai->proto->tech, poly, descript);
			}
		} else
		{
			/* cell variables are offset from (0,0) */
			poly->xv[0] = poly->yv[0] = 0;
			adjustdisoffset((INTBIG)high->cell, VNODEPROTO, high->cell->tech,
				poly, descript);
		}
		*style = poly->style;
	} else if (high->fromport != NOPORTPROTO)
	{
		TDCOPY(descript, high->fromport->textdescript);
		ni = high->fromgeom->entryaddr.ni;
		portposition(high->fromport->subnodeinst, high->fromport->subportproto,
			&poly->xv[0], &poly->yv[0]);
		makeangle(ni->rotation, ni->transpose, trans);
		cell = ni->parent;
		lambda = cell->lib->lambda[cell->tech->techindex];
		newxc = TDGETXOFF(descript);
		newxc = newxc * lambda / 4;
		newyc = TDGETYOFF(descript);
		newyc = newyc * lambda / 4;
		xform(newxc, newyc, &newxc, &newyc, trans);
		poly->xv[0] += newxc;   poly->yv[0] += newyc;
		switch (TDGETPOS(descript))
		{
			case VTPOSCENT:      *style = TEXTCENT;      break;
			case VTPOSBOXED:     *style = TEXTBOX;       break;
			case VTPOSUP:        *style = TEXTBOT;       break;
			case VTPOSDOWN:      *style = TEXTTOP;       break;
			case VTPOSLEFT:      *style = TEXTRIGHT;     break;
			case VTPOSRIGHT:     *style = TEXTLEFT;      break;
			case VTPOSUPLEFT:    *style = TEXTBOTRIGHT;  break;
			case VTPOSUPRIGHT:   *style = TEXTBOTLEFT;   break;
			case VTPOSDOWNLEFT:  *style = TEXTTOPRIGHT;  break;
			case VTPOSDOWNRIGHT: *style = TEXTTOPLEFT;   break;
		}
		*style = rotatelabel(*style, TDGETROTATION(descript), trans);
	} else if (high->fromgeom != NOGEOM && high->fromgeom->entryisnode)
	{
		/* instance name */
		ni = high->fromgeom->entryaddr.ni;
		TDCOPY(descript, ni->textdescript);
		poly->xv[0] = (ni->lowx + ni->highx) / 2;
		poly->yv[0] = (ni->lowy + ni->highy) / 2;
		adjustdisoffset((INTBIG)ni, VNODEINST, ni->proto->tech, poly, descript);
		makerot(ni, trans);
		xformpoly(poly, trans);
		*style = poly->style;
	}
	getcenter(poly, xc, yc);
}

void us_gethightextsize(HIGHLIGHT *high, INTBIG *xw, INTBIG *yw, WINDOWPART *w)
{
	REGISTER INTBIG len;
	REGISTER CHAR *str;
	REGISTER TECHNOLOGY *tech;
	REGISTER WINDOWPART *oldwin;
	INTBIG lx, hx, ly, hy;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 5, us_tool->cluster);

	/* determine number of lines of text */
	len = 1;
	if (high->fromvar != NOVARIABLE && (high->fromvar->type&VISARRAY) != 0)
		len = getlength(high->fromvar);
	if (len > 1)
	{
		/* get size of array of text */
		makedisparrayvarpoly(high->fromgeom, w, high->fromvar, poly);
		getbbox(poly, &lx, &hx, &ly, &hy);
		*xw = hx - lx;
		*yw = hy - ly;
		return;
	}

	/* determine size of single line of text */
	tech = us_hightech(high);
	oldwin = setvariablewindow(w);
	str = us_gethighstring(high);
	us_gethighdescript(high, descript);
	(void)setvariablewindow(oldwin);

	us_gettextscreensize(str, descript, w, tech, high->fromgeom, xw, yw);
}

/*
 * Routine to return the text descriptor on highlight "high" in "descript".
 */
void us_gethighdescript(HIGHLIGHT *high, UINTBIG *descript)
{
	if (high->fromvar != NOVARIABLE)
	{
		TDCOPY(descript, high->fromvar->textdescript);
		return;
	}
	if (high->fromport != NOPORTPROTO)
	{
		TDCOPY(descript, high->fromport->textdescript);
		return;
	}
	if (high->fromgeom->entryisnode)
	{
		TDCOPY(descript, high->fromgeom->entryaddr.ni->textdescript);
		return;
	}
	TDCLEAR(descript);
}

/*
 * Routine to return the text on highlight "high".
 */
CHAR *us_gethighstring(HIGHLIGHT *high)
{
	REGISTER CHAR *str;
	REGISTER VARIABLE *var;

	var = high->fromvar;
	if (var != NOVARIABLE)
	{
		if (high->fromvarnoeval != NOVARIABLE) var = evalvar(high->fromvarnoeval, 0, 0);
		str = describedisplayedvariable(var, -1, -1);
		return(str);
	}
	if (high->fromport != NOPORTPROTO)
		return(us_displayedportname(high->fromport, (us_useroptions&EXPORTLABELS) >> EXPORTLABELSSH));
	if (high->fromgeom->entryisnode)
		return(describenodeproto(high->fromgeom->entryaddr.ni->proto));
	return(x_(""));
}

/*
 * Routine to examine highlight "high" and return the address and type
 * of the object to which it refers.
 */
void us_gethighaddrtype(HIGHLIGHT *high, INTBIG *addr, INTBIG *type)
{
	if ((high->status&HIGHTYPE) == HIGHTEXT)
	{
		if (high->fromvar != NOVARIABLE)
		{
			if (high->fromport != NOPORTPROTO)
			{
				*addr = (INTBIG)high->fromport;
				*type = VPORTPROTO;
				return;
			}
			if (high->fromgeom == NOGEOM)
			{
				*addr = (INTBIG)high->cell;
				*type = VNODEPROTO;
				return;
			}

			*addr = (INTBIG)high->fromgeom->entryaddr.blind;
			if (high->fromgeom->entryisnode)
			{
				*type = VNODEINST;
			} else
			{
				*type = VARCINST;
			}
			return;
		}
		if (high->fromport != NOPORTPROTO)
		{
			*addr = (INTBIG)high->fromport;
			*type = VPORTPROTO;
			return;
		}
		*addr = (INTBIG)high->fromgeom->entryaddr.blind;
		if (high->fromgeom->entryisnode) *type = VNODEINST; else
			*type = VARCINST;
		return;
	}
}

/*
 * Routine to return true if highlight "high" is text on a node that must move together.
 * This only happens for "nonlayout text" (a variable on an invisible node), for exports
 * on an invisible pin, or for any export when it is requested that the node move too.
 */
BOOLEAN us_nodemoveswithtext(HIGHLIGHT *high)
{
	REGISTER NODEINST *ni;

	if (high->status != HIGHTEXT) return(FALSE);
	if (high->fromgeom == NOGEOM) return(FALSE);
	if (!high->fromgeom->entryisnode) return(FALSE);
	ni = high->fromgeom->entryaddr.ni;
	if (ni == NONODEINST) return(FALSE);
	if (high->fromvar != NOVARIABLE)
	{
		/* moving variable text */
		if (ni->proto != gen_invispinprim) return(FALSE);
		if (high->fromport == NOPORTPROTO) return(TRUE);
	} else
	{
		/* moving export text */
		if ((us_useroptions&MOVENODEWITHEXPORT) == 0 && ni->proto != gen_invispinprim)
			return(FALSE);
		if (high->fromport != NOPORTPROTO) return(TRUE);
	}
	return(FALSE);
}

/*
 * Routine to figure out the technology to use for highlight "high".
 */
TECHNOLOGY *us_hightech(HIGHLIGHT *high)
{
	REGISTER NODEPROTO *cell;

	cell = us_highcell(high);
	if (cell == NONODEPROTO) return(el_curtech);
	if (cell->tech == NOTECHNOLOGY)
		cell->tech = whattech(cell);
	return(cell->tech);
}

/*
 * Routine to figure out the cell to use for highlight "high".
 */
NODEPROTO *us_highcell(HIGHLIGHT *high)
{
	if (high->fromvar != NOVARIABLE)
	{
		if (high->fromport != NOPORTPROTO)
			return(high->fromport->subnodeinst->parent);
		if (high->fromgeom == NOGEOM) return(high->cell);
		if (high->fromgeom->entryisnode)
			return(high->fromgeom->entryaddr.ni->parent);
		return(high->fromgeom->entryaddr.ai->parent);
	}
	if (high->fromport != NOPORTPROTO)
		return(high->fromgeom->entryaddr.ni->parent);
	if (high->fromgeom->entryisnode)
		return(high->fromgeom->entryaddr.ni->parent);
	return(high->fromgeom->entryaddr.ai->parent);
}

TECHNOLOGY *us_getobjectinfo(INTBIG addr, INTBIG type, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp;
	REGISTER NODEPROTO *np;
	INTBIG olx, ohx, oly, ohy;

	switch (type)
	{
		case VNODEINST:
			ni = (NODEINST *)addr;
			nodesizeoffset(ni, &olx, &oly, &ohx, &ohy);
			*lx = ni->geom->lowx+olx;   *hx = ni->geom->highx-ohx;
			*ly = ni->geom->lowy+oly;   *hy = ni->geom->highy-ohy;
			return(ni->parent->tech);
		case VARCINST:
			ai = (ARCINST *)addr;
			*lx = ai->geom->lowx;   *hx = ai->geom->highx;
			*ly = ai->geom->lowy;   *hy = ai->geom->highy;
			return(ai->parent->tech);
		case VPORTPROTO:
			pp = (PORTPROTO *)addr;
			ni = pp->subnodeinst;
			*lx = ni->geom->lowx;   *hx = ni->geom->highx;
			*ly = ni->geom->lowy;   *hy = ni->geom->highy;
			return(pp->parent->tech);
		case VNODEPROTO:
			np = (NODEPROTO *)addr;
			*lx = np->lowx;   *hx = np->highx;
			*ly = np->lowy;   *hy = np->highy;
			return(np->tech);
	}
	return(NOTECHNOLOGY);
}

void us_drawcellvariable(VARIABLE *var, NODEPROTO *np)
{
	REGISTER INTBIG i, displaytotal;
	REGISTER WINDOWPART *w;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	displaytotal = tech_displayablecellvars(np, NOWINDOWPART, &tech_oneprocpolyloop);
	for(i = 0; i < displaytotal; i++)
	{
		(void)tech_filldisplayablecellvar(np, poly, NOWINDOWPART, 0, &tech_oneprocpolyloop);
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if (w->curnodeproto != np) continue;
			(*us_displayroutine)(poly, w);
		}
	}
}

void us_undrawcellvariable(VARIABLE *var, NODEPROTO *np)
{
	INTBIG x, y;
	INTBIG tsx, tsy;
	REGISTER INTBIG objwid, objhei, sea, slx, shx, sly, shy, nudge;
	INTBIG lx, hx, ly, hy;
	REGISTER TECHNOLOGY *tech;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER GEOM *geom;
	REGISTER CHAR *str;
	REGISTER WINDOWPART *w;
	extern GRAPHICS us_ebox;

	if ((var->type&VDISPLAY) == 0) return;
	tech = np->tech;
	str = describedisplayedvariable(var, -1, -1);
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		if ((w->state&WINDOWTYPE) != DISPWINDOW) continue;
		if (w->curnodeproto != np) continue;
		screensettextinfo(w, tech, var->textdescript);

		getdisparrayvarlinepos((INTBIG)np, VNODEPROTO, tech,
			w, var, 0, &x, &y, TRUE);
		screengettextsize(w, str, &tsx, &tsy);
		objwid = roundfloat((float)tsx / w->scalex);
		objhei = roundfloat((float)tsy / w->scaley);
		slx = x;   shx = slx + objwid;
		sly = y;   shy = sly + objhei;

		/* erase that area */
		lx = slx;   hx = shx;
		ly = sly;   hy = shy;
		if ((w->state&INPLACEEDIT) != 0) 
			xformbox(&lx, &hx, &ly, &hy, w->outofcell);
		if (us_makescreen(&lx, &ly, &hx, &hy, w)) continue;
		screendrawbox(w, lx, hx, ly, hy, &us_ebox);

		/* now redraw everything that touches this box */
		nudge = (INTBIG)(1.0f/w->scalex);
		slx -= nudge;
		shx += nudge;
		nudge = (INTBIG)(1.0f/w->scaley);
		sly -= nudge;
		shy += nudge;
		sea = initsearch(slx, shx, sly, shy, np);
		for(;;)
		{
			geom = nextobject(sea);
			if (geom == NOGEOM) break;
			if (geom->entryisnode)
			{
				ni = geom->entryaddr.ni;
				(void)us_drawcell(ni, LAYERA, el_matid, 3, w);
			} else
			{
				ai = geom->entryaddr.ai;
				(void)us_drawarcinst(ai, LAYERA, el_matid, 3, w);
			}
		}
	}
}
