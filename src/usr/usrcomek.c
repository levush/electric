/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrcomek.c
 * User interface tool: command handler for E through K
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
#include "usr.h"
#include "usrtrack.h"
#include "usrdiacom.h"
#include "efunction.h"
#include "edialogs.h"
#include "tecart.h"
#include "tecgen.h"
#include "tecschem.h"
#include "sim.h"
#include "network.h"
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif

#define MAXLINE 500		/* maximum length of news/help file input lines */

/* working memory for "us_iterate()" */
static INTBIG *us_iterateaddr, *us_iteratetype;
static INTBIG  us_iteratelimit=0;

/*
 * Routine to free all memory associated with this module.
 */
void us_freecomekmemory(void)
{
	if (us_iteratelimit > 0)
	{
		efree((CHAR *)us_iterateaddr);
		efree((CHAR *)us_iteratetype);
	}
}

void us_echo(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG lastquiet, j;
	REGISTER void *infstr;

	lastquiet = ttyquiet(0);
	infstr = initinfstr();
	for(j=0; j<count; j++)
	{
		addstringtoinfstr(infstr, par[j]);
		addtoinfstr(infstr, ' ');
	}
	ttyputmsg(x_("%s"), returninfstr(infstr));
	(void)ttyquiet(lastquiet);
}

/* Simulation: Select Node Index */
static DIALOGITEM us_selinddialogitems[] =
{
 /*  1 */ {0, {76,120,100,200}, BUTTON, N_("OK")},
 /*  2 */ {0, {76,16,100,96}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,4,20,216}, MESSAGE, N_("This node is arrayed")},
 /*  4 */ {0, {24,4,40,216}, MESSAGE, N_("Which entry should be entered?")},
 /*  5 */ {0, {48,4,64,216}, POPUP, x_("")}
};
static DIALOG us_selinddialog = {{75,75,184,301}, N_("Select Node Index"), 0, 5, us_selinddialogitems, 0, 0};

/* special items for the "Select Node Index" dialog: */
#define DSNI_CHOICES       5		/* List of indices (popup) */

void us_editcell(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG implicit, i, len, newwindow, nonredundant, intoicon, lambda, inplace, index,
		sigcount, itemHit;
	BOOLEAN newframe, push, exact;
	INTBIG lx, hx, ly, hy;
	INTBIG viewinfo[22];
	REGISTER NODEPROTO *np, *onp, *np1, *curcell;
	REGISTER NODEINST *ni, *stacknodeinst;
	REGISTER LIBRARY *lib, *olib;
	REGISTER VARIABLE *var;
	REGISTER void *dia;
	REGISTER WINDOWPART *win;
	REGISTER CHAR *pt;
	CHAR **nodenames;
	NODEINST *hini;
	XARRAY rotarray, transarray, xfarray, newarray;
	PORTPROTO *hipp;

	/* get proper highlighting in subcell if port is selected */
	us_findlowerport(&hini, &hipp);

	/* find the nodeinst in this window to go "down into" (if any) */
	newwindow = 0;
	nonredundant = 0;
	intoicon = 0;
	inplace = 0;
	index = 0;
	if (count == 1)
	{
		len = estrlen(par[0]);
		if (len == 0) count--; else
		{
			if (namesamen(par[0], x_("in-place"), len) == 0)
			{
				if (el_curwindowpart->inplacedepth >= MAXINPLACEDEPTH)
				{
					ttyputerr(_("Can only go down %ld levels in-place"), MAXINPLACEDEPTH);
					return;
				}
				inplace = 1;
				count--;
			}
		}
	}
	if (count == 0)
	{
		implicit = 1;
		ni = (NODEINST *)us_getobject(VNODEINST, FALSE);
		if (ni == NONODEINST) return;
		stacknodeinst = ni;
		np = ni->proto;

		/* translate this reference if this is an icon cell */
		if (np->cellview == el_iconview)
		{
			implicit = 0;
			if (!isiconof(np, ni->parent))
			{
				onp = contentsview(np);
				if (onp != NONODEPROTO) np = onp;
			} else
			{
				intoicon = 1;
			}
		}
	} else
	{
		/* check for options */
		for(i=1; i<count; i++)
		{
			len = estrlen(par[i]);
			if (namesamen(par[i], x_("new-window"), len) == 0 && len > 1) newwindow++; else
			if (namesamen(par[i], x_("non-redundant"), len) == 0 && len > 1) nonredundant++; else
			{
				ttyputbadusage(x_("editcell"));
				return;
			}
		}

		/* see if specified cell exists */
		implicit = 0;
		np = getnodeproto(par[0]);

		/* if it is a new cell, create it */
		if (np == NONODEPROTO)
		{
			lib = el_curlib;
			for(pt = par[0]; *pt != 0; pt++) if (*pt == ':') break;
			if (*pt != ':') pt = par[0]; else
			{
				i = *pt;
				*pt = 0;
				lib = getlibrary(par[0]);
				*pt = (CHAR)i;
				if (lib == NOLIBRARY)
				{
					us_abortcommand(_("Cannot find library for new cell %s"), par[0]);
					return;
				}
				pt++;
			}
			np = us_newnodeproto(pt, lib);
			if (np == NONODEPROTO)
			{
				us_abortcommand(_("Cannot create cell %s"), par[0]);
				return;
			}
			ttyputverbose(M_("Editing new cell: %s"), par[0]);
		}

		/* look through window for instances of this cell */
		stacknodeinst = NONODEINST;
		curcell = getcurcell();
		if (curcell != NONODEPROTO)
		{
			for(ni = curcell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				if (ni->proto == np)
			{
				stacknodeinst = ni;
				break;
			}
		}
	}

	/* make sure nodeinst is not primitive and in proper technology */
	if (np->primindex != 0)
	{
		us_abortcommand(_("Cannot edit primitive nodes"));
		return;
	}

	push = FALSE;
	exact = FALSE;
	if (stacknodeinst != NONODEINST)
	{
		/* if in waveform window, switch to associated circuit window */
		if ((el_curwindowpart->state&WINDOWTYPE) == WAVEFORMWINDOW)
		{
			for(win = el_topwindowpart; win != NOWINDOWPART; win = win->nextwindowpart)
				if (win != el_curwindowpart && win->curnodeproto == stacknodeinst->parent) break;
			if (win == NOWINDOWPART)
			{
				us_abortcommand(_("Cannot go down the hierarchy from this waveform window"));
				return;
			}
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)win,
				VWINDOWPART|VDONTSAVE);
		}

		/* if simulating, and node is arrayed, find out the index */
		if (stacknodeinst->arraysize > 1 && el_curwindowpart != NOWINDOWPART &&
			(el_curwindowpart->state&WINDOWMODE) == WINDOWSIMMODE)
		{
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
			if (var != NOVARIABLE)
			{
				/* must find out which index is being traversed */
				sigcount = net_evalbusname(APBUS, (CHAR *)var->addr, &nodenames,
					NOARCINST, NONODEPROTO, 0);
				dia = DiaInitDialog(&us_selinddialog);
				DiaSetPopup(dia, DSNI_CHOICES, sigcount, nodenames);
				for(;;)
				{
					itemHit = DiaNextHit(dia);
					if (itemHit == CANCEL || itemHit == OK) break;
				}
				if (itemHit != CANCEL)
					index = DiaGetPopupEntry(dia, DSNI_CHOICES);
				DiaDoneDialog(dia);
			}
		}

		viewinfo[0] = el_curwindowpart->screenlx;
		viewinfo[1] = el_curwindowpart->screenhx;
		viewinfo[2] = el_curwindowpart->screenly;
		viewinfo[3] = el_curwindowpart->screenhy;
		viewinfo[4] = el_curwindowpart->intocell[0][0];
		viewinfo[5] = el_curwindowpart->intocell[0][1];
		viewinfo[6] = el_curwindowpart->intocell[0][2];
		viewinfo[7] = el_curwindowpart->intocell[1][0];
		viewinfo[8] = el_curwindowpart->intocell[1][1];
		viewinfo[9] = el_curwindowpart->intocell[1][2];
		viewinfo[10] = el_curwindowpart->intocell[2][0];
		viewinfo[11] = el_curwindowpart->intocell[2][1];
		viewinfo[12] = el_curwindowpart->intocell[2][2];
		viewinfo[13] = el_curwindowpart->outofcell[0][0];
		viewinfo[14] = el_curwindowpart->outofcell[0][1];
		viewinfo[15] = el_curwindowpart->outofcell[0][2];
		viewinfo[16] = el_curwindowpart->outofcell[1][0];
		viewinfo[17] = el_curwindowpart->outofcell[1][1];
		viewinfo[18] = el_curwindowpart->outofcell[1][2];
		viewinfo[19] = el_curwindowpart->outofcell[2][0];
		viewinfo[20] = el_curwindowpart->outofcell[2][1];
		viewinfo[21] = el_curwindowpart->outofcell[2][2];
		push = TRUE;
		exact = TRUE;
		onp = contentsview(np);
		if (onp == NONODEPROTO || intoicon != 0) onp = np;
		sethierarchicalparent(onp, stacknodeinst, el_curwindowpart, index, viewinfo);
	}

	/* check dates of subcells */
	if ((us_useroptions&CHECKDATE) != 0)
	{
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			for(np1 = olib->firstnodeproto; np1 != NONODEPROTO; np1 = np1->nextnodeproto)
				np1->temp1 = 0;
		us_check_cell_date(np, np->revisiondate);
	}

	/* if a nonredundant display is needed, see if it already exists */
	if (nonredundant != 0)
	{
		for(win = el_topwindowpart; win != NOWINDOWPART; win = win->nextwindowpart)
			if (win->curnodeproto == np) break;
		if (win != NOWINDOWPART)
		{
			/* switch to window "win" */
			bringwindowtofront(win->frame);
			us_highlightwindow(win, FALSE);
			return;
		}
	}

	/* determine window area */
	if (el_curwindowpart == NOWINDOWPART)
	{
		lx = np->lowx;   hx = np->lowx;
		ly = np->lowy;   hy = np->lowy;
	} else
	{
		lx = el_curwindowpart->screenlx;   hx = el_curwindowpart->screenhx;
		ly = el_curwindowpart->screenly;   hy = el_curwindowpart->screenhy;
		if (el_curwindowpart->curnodeproto == NONODEPROTO)
		{
			lambda = el_curlib->lambda[el_curtech->techindex];
			lx = -lambda * 25;
			hx =  lambda * 25;
			ly = -lambda * 25;
			hy =  lambda * 25;
		}
	}
	if (implicit == 0)
	{
		/* make the new cell fill the window */
		us_fullview(np, &lx, &hx, &ly, &hy);
		exact = FALSE;
	} else
	{
		/* make the current cell be in the same place in the window */
		if (inplace != 0)
		{
			lx = el_curwindowpart->screenlx;   hx = el_curwindowpart->screenhx;
			ly = el_curwindowpart->screenly;   hy = el_curwindowpart->screenhy;
		} else
		{
			lx += np->lowx - stacknodeinst->lowx;
			hx += np->lowx - stacknodeinst->lowx;
			ly += np->lowy - stacknodeinst->lowy;
			hy += np->lowy - stacknodeinst->lowy;
		}
	}

	if (stacknodeinst != NONODEINST)
	{
		/* if simulating, coordinate this hierarchy traversal with the waveform */
		if (el_curwindowpart != NOWINDOWPART &&
			(el_curwindowpart->state&WINDOWMODE) == WINDOWSIMMODE)
		{
			asktool(sim_tool, "traverse-down", (INTBIG)stacknodeinst, np);
		}
	}

	/* edit the cell (creating a new frame/partition if requested) */
	if (newwindow != 0 || el_curwindowpart == NOWINDOWPART)
	{
		newframe = TRUE;
		push = FALSE;
	} else
	{
		newframe = FALSE;
	}
	if (implicit != 0 && inplace != 0)
	{
		makerotI(stacknodeinst, rotarray);
		maketransI(stacknodeinst, transarray);
		transmult(rotarray, transarray, xfarray);
		transmult(el_curwindowpart->intocell, xfarray, newarray);
		us_setxarray((INTBIG)el_curwindowpart, VWINDOWPART, x_("intocell"), newarray);

		makerot(stacknodeinst, rotarray);
		maketrans(stacknodeinst, transarray);
		transmult(transarray, rotarray, xfarray);
		transmult(xfarray, el_curwindowpart->outofcell, newarray);
		us_setxarray((INTBIG)el_curwindowpart, VWINDOWPART, x_("outofcell"), newarray);
		if ((el_curwindowpart->state&INPLACEEDIT) == 0)
		{
			(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("inplacedepth"), 0, VINTEGER);
			(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("topnodeproto"),
				(INTBIG)el_curwindowpart->curnodeproto, VNODEPROTO);
		}
		(void)setind((INTBIG)el_curwindowpart, VWINDOWPART, x_("inplacestack"),
			el_curwindowpart->inplacedepth, (INTBIG)stacknodeinst);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("inplacedepth"),
			el_curwindowpart->inplacedepth+1, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("state"),
			el_curwindowpart->state | INPLACEEDIT, VINTEGER);
	} else
	{
		if (el_curwindowpart != NOWINDOWPART)
		{
			us_setxarray((INTBIG)el_curwindowpart, VWINDOWPART, x_("intocell"), el_matid);
			us_setxarray((INTBIG)el_curwindowpart, VWINDOWPART, x_("outofcell"), el_matid);
			(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("state"),
				el_curwindowpart->state & ~INPLACEEDIT, VINTEGER);
		}
	}
	us_switchtocell(np, lx, hx, ly, hy, hini, hipp, newframe, push, exact);
}

void us_erase(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG i, j, l;
	INTBIG textcount;
	CHAR **textinfo;
	BOOLEAN allvisible, foundnode, cleaned;
	REGISTER NODEINST *ni;
	ARCINST *ai;
	REGISTER NODEPROTO *np;
	HIGHLIGHT high;
	REGISTER CHAR *pt;
	REGISTER GEOM **list;
	REGISTER LIBRARY *lib;
	REGISTER WINDOWPART *w;
	HIGHLIGHT newhigh;
	REGISTER void *infstr;

	if (count > 0)
	{
		l = estrlen(pt = par[0]);
		if (namesamen(pt, x_("clean-up-all"), l) == 0 && l > 8)
		{
			us_clearhighlightcount();
			cleaned = FALSE;
			for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				if (us_cleanupcell(np, FALSE)) cleaned = TRUE;
			if (!cleaned) ttyputmsg(_("Nothing to clean"));
			return;
		}
		if (namesamen(pt, x_("clean-up"), l) == 0)
		{
			np = us_needcell();
			if (np == NONODEPROTO) return;
			us_clearhighlightcount();
			(void)us_cleanupcell(np, TRUE);
			return;
		}
		if (namesamen(pt, x_("geometry"), l) == 0)
		{
			np = us_needcell();
			if (np == NONODEPROTO) return;
			us_erasegeometry(np);
			return;
		}
	}

	/* get list of highlighted objects to be erased */
	np = us_needcell();
	if (np == NONODEPROTO)
	{
		us_abortcommand(_("No current cell"));
		return;
	}
	list = us_gethighlighted(WANTARCINST|WANTNODEINST, &textcount, &textinfo);
	if (list[0] == NOGEOM && textcount == 0)
	{
		us_abortcommand(_("Find an object to erase"));
		return;
	}

	/* if in outline-edit mode, delete the selected point */
	if ((el_curwindowpart->state&WINDOWOUTLINEEDMODE) != 0)
	{
		par[0] = x_("trace");
		par[1] = x_("delete-point");
		us_node(2, par);
		return;
	}

	/* make sure that all requested objects are displayed */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = 0;
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		if (w->curnodeproto != NONODEPROTO)
			w->curnodeproto->temp1 = 1;
	allvisible = TRUE;
	for(i=0; list[i] != NOGEOM; i++)
	{
		np = geomparent(list[i]);
		if (np->temp1 == 0) allvisible = FALSE;
	}
	for(i=0; i<textcount; i++)
	{
		(void)us_makehighlight(textinfo[i], &high);
		np = high.cell;
		if (np->temp1 == 0) allvisible = FALSE;
	}
	if (!allvisible)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, _("Some of the objects to be deleted are not displayed.  Delete anyway?"));
		addstringtoinfstr(infstr, _(" (click 'no' to delete only visible objects, 'yes' to delete all selected)"));
		j = us_noyescanceldlog(returninfstr(infstr), par);
		if (j == 0) return;
		if (namesame(par[0], x_("cancel")) == 0) return;
		if (namesame(par[0], x_("no")) == 0)
		{
			j = 0;
			for(i=0; list[i] != NOGEOM; i++)
			{
				np = geomparent(list[i]);
				if (np->temp1 == 0) continue;
				list[j++] = list[i];
			}
			if (j == 0)
			{
				us_abortcommand(_("No displayed objects to delete"));
				return;
			}
			list[j] = NOGEOM;
		}
	}

	/* remove from list if a node is locked */
	j = 0;
	foundnode = FALSE;
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (list[i]->entryisnode)
		{
			ni = list[i]->entryaddr.ni;
			if (us_cantedit(np, ni, TRUE)) continue;
			foundnode = TRUE;
		}
		list[j++] = list[i];
	}
	list[j] = NOGEOM;
	if (list[0] == NOGEOM && textcount == 0)
	{
		us_abortcommand(_("All selected objects are locked"));
		return;
	}

	if (!foundnode)
	{
		/* disallow erasing if lock is on */
		if (us_cantedit(np, NONODEINST, TRUE)) return;
	}

	/* unhighlight */
	us_clearhighlightcount();

	/* if one node is selected, see if it can be handled with reconnection */
	if (list[0] != NOGEOM && list[0]->entryisnode && list[1] == NOGEOM && textcount == 0)
	{
		j = us_erasepassthru(ni, FALSE, &ai);
		if (j == 2)
		{
			/* worked: highlight the arc */
			newhigh.status = HIGHFROM;
			newhigh.fromgeom = ai->geom;
			newhigh.cell = np;
			us_setfind(&newhigh, 0, 1, 0, 0);
			return;
		}
	}

	/* delete the text */
	for(i=0; i<textcount; i++)
	{
		(void)us_makehighlight(textinfo[i], &high);

		/* disallow erasing if lock is on */
		np = high.cell;
		if (np != NONODEPROTO)
		{
			if (us_cantedit(np, NONODEINST, TRUE)) continue;
		}

		/* do not deal with text on an object if the object is already in the list */
		if (high.fromgeom != NOGEOM)
		{
			for(j=0; list[j] != NOGEOM; j++)
				if (list[j] == high.fromgeom) break;
			if (list[j] != NOGEOM) continue;
		}

		/* deleting variable on object */
		if (high.fromvar != NOVARIABLE)
		{
			if (high.fromgeom == NOGEOM)
			{
				us_undrawcellvariable(high.fromvar, np);
				(void)delval((INTBIG)np, VNODEPROTO, makename(high.fromvar->key));
			} else if (high.fromgeom->entryisnode)
			{
				ni = high.fromgeom->entryaddr.ni;
				startobjectchange((INTBIG)ni, VNODEINST);

				/* if deleting port variables, do that */
				if (high.fromport != NOPORTPROTO)
				{
					(void)delval((INTBIG)high.fromport, VPORTPROTO, makename(high.fromvar->key));
				} else
				{
					/* if deleting text on invisible pin, delete pin too */
					if (ni->proto == gen_invispinprim) us_erasenodeinst(ni); else
						(void)delval((INTBIG)ni, VNODEINST, makename(high.fromvar->key));
				}
				endobjectchange((INTBIG)ni, VNODEINST);
			} else
			{
				ai = high.fromgeom->entryaddr.ai;
				startobjectchange((INTBIG)ai, VARCINST);
				(void)delval((INTBIG)ai, VARCINST, makename(high.fromvar->key));
				endobjectchange((INTBIG)ai, VARCINST);
			}
		} else if (high.fromport != NOPORTPROTO)
		{
			ni = high.fromgeom->entryaddr.ni;
			startobjectchange((INTBIG)ni, VNODEINST);
			us_undoportproto((NODEINST *)ni, high.fromport);
			endobjectchange((INTBIG)ni, VNODEINST);
		} else if (high.fromgeom->entryisnode)
			us_abortcommand(_("Cannot delete cell name"));
	}

	/* look for option to re-connect arcs into an erased node */
	if (count > 0 && namesamen(par[0], x_("pass-through"), estrlen(par[0])) == 0)
	{
		for(i=0; list[i] != NOGEOM; i++)
		{
			if (!list[i]->entryisnode) continue;
			ni = list[i]->entryaddr.ni;
			j = us_erasepassthru(ni, TRUE, &ai);
			switch (j)
			{
				case 2:
					break;
				case -1:
					us_abortcommand(_("Arcs to node %s are of different type"),
						describenodeinst(ni));
					break;
				case -5:
					us_abortcommand(_("Cannot create connecting arc"));
					break;
				default:
					us_abortcommand(_("Must be 2 arcs on node %s (it has %ld)"),
						describenodeinst(ni), j);
					break;
			}
		}
		return;
	}

	/* handle simple erasing */
	us_eraseobjectsinlist(np, list);
}

void us_find(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG i, j, l, findport, findpoint, findexclusively, findangle,
		findwithin, findstill, findspecial, size, findeasy, findhard, total, type, addr,
		len, first, areasizex, areasizey, x, y, *newlist, extrainfo, findmore, findnobox;
	INTBIG xcur, ycur;
	REGISTER BOOLEAN waitforpush;
	XARRAY trans;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np, *onp;
	REGISTER LIBRARY *lib;
	REGISTER GEOM *geom, **glist;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp;
	REGISTER NETWORK *net;
	REGISTER VARIABLE *var, *highvar;
	REGISTER CHAR *pt;
	CHAR **list;
	HIGHLIGHT newhigh, newhightext;
	REGISTER void *infstr;

	if (count >= 1)
	{
		l = estrlen(pt = par[0]);
		if (namesamen(pt, x_("constraint-angle"), l) == 0 && l >= 12)
		{
			if (count >= 2)
			{
				(void)setvalkey((INTBIG)us_tool, VTOOL, us_interactiveanglekey,
					atofr(par[1])*10/WHOLE, VINTEGER|VDONTSAVE);
			}
			var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_interactiveanglekey);
			if (var == NOVARIABLE) findangle = 0; else
				findangle = var->addr;
			if (findangle == 0)
			{
				ttyputverbose(M_("Interactive dragging done with no constraints"));
			} else
			{
				ttyputverbose(M_("Interactive dragging constrained to %ld degree angles"),
					findangle);
			}
			return;
		}
	}

	/* make sure there is a cell being edited */
	np = us_needcell();
	if (np == NONODEPROTO) return;
	if ((np->cellview->viewstate&TEXTVIEW) != 0)
	{
		l = estrlen(pt = par[0]);
		if (namesamen(pt, x_("all"), l) == 0 && l >= 2)
		{
			/* special case: "find all" selects all text */
			i = us_totallines(el_curwindowpart);
			us_highlightline(el_curwindowpart, 0, i-1);
			return;
		}
		us_abortcommand(M_("There are no components to select in a text-only cell"));
		return;
	}
	if ((el_curwindowpart->state&WINDOWTYPE) == DISP3DWINDOW)
	{
		us_abortcommand(M_("Cannot select objects in a 3D window"));
		return;
	}

	/* establish the default highlight environment */
	highvar = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (highvar != NOVARIABLE)
		(void)us_makehighlight(((CHAR **)highvar->addr)[0], &newhigh); else
	{
		newhigh.status = 0;
		newhigh.fromgeom = NOGEOM;
		newhigh.fromport = NOPORTPROTO;
		newhigh.fromvar = NOVARIABLE;
		newhigh.fromvarnoeval = NOVARIABLE;
		newhigh.frompoint = 0;
	}

	/* look for qualifiers */
	findport = findpoint = findexclusively = findwithin = findmore = 0;
	findstill = findspecial = findnobox = 0;
	extrainfo = 0;
	while (count > 0)
	{
		l = estrlen(pt = par[0]);
		if (namesamen(pt, x_("extra-info"), l) == 0 && l >= 3)
		{
			extrainfo = HIGHEXTRA;
			count--;   par++;   continue;
		}
		if (namesamen(pt, x_("more"), l) == 0 && l >= 1)
		{
			findmore++;
			count--;   par++;   continue;
		}
		if (namesamen(pt, x_("no-box"), l) == 0 && l >= 3)
		{
			findnobox++;
			count--;   par++;   continue;
		}
		if (namesamen(pt, x_("special"), l) == 0 && l >= 2)
		{
			findspecial++;
			count--;   par++;   continue;
		}
		if (namesamen(pt, x_("still"), l) == 0 && l >= 2)
		{
			findstill++;
			count--;   par++;   continue;
		}
		if (namesamen(pt, x_("exclusively"), l) == 0 && l >= 3)
		{
			findexclusively++;
			count--;   par++;   continue;
		}
		if (namesamen(pt, x_("within"), l) == 0 && l >= 1)
		{
			if (newhigh.status == 0)
			{
				us_abortcommand(M_("Find an object before working 'within' it"));
				return;
			}
			if ((newhigh.status&HIGHTYPE) != HIGHFROM || !newhigh.fromgeom->entryisnode)
			{
				us_abortcommand(M_("Must find a node before working 'within'"));
				return;
			}

			findwithin++;
			count--;   par++;   continue;
		}
		if (namesamen(pt, x_("port"), l) == 0 && l >= 1)
		{
			findport = 1;
			count--;   par++;   continue;
		}
		if (namesamen(pt, x_("vertex"), l) == 0 && l >= 2)
		{
			findpoint = 1;
			count--;   par++;   continue;
		}
		break;
	}

	if (count >= 1)
	{
		l = estrlen(pt = par[0]);

		if (namesamen(pt, x_("all"), l) == 0 && l >= 2)
		{
			if (findpoint != 0 || findwithin != 0 || findmore != 0 ||
				findexclusively != 0 || findstill != 0 || findspecial != 0)
			{
				us_abortcommand(M_("Illegal options given to 'find all'"));
				return;
			}
			findeasy = findhard = 1;
			if (count >= 2)
			{
				l = estrlen(pt = par[1]);
				if (namesamen(pt, x_("easy"), l) == 0) findhard = 0; else
				if (namesamen(pt, x_("hard"), l) == 0) findeasy = 0; else
				{
					ttyputusage(x_("find all [easy|hard]"));
					return;
				}
			}
			total = 0;
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst) total++;
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst) total++;
			for(i=0; i<np->numvar; i++)
			{
				var = &np->firstvar[i];
				if ((var->type&VDISPLAY) != 0) total++;
			}
			if (total == 0) return;
			list = (CHAR **)emalloc((total * (sizeof (CHAR *))), el_tempcluster);
			if (list == 0) return;

			newhigh.status = HIGHFROM | extrainfo;
			if (findnobox != 0) newhigh.status |= HIGHNOBOX;
			newhigh.cell = np;
			newhigh.frompoint = 0;
			newhigh.fromvar = NOVARIABLE;
			newhigh.fromvarnoeval = NOVARIABLE;

			newhightext.status = HIGHTEXT;
			newhightext.cell = np;
			newhightext.fromport = NOPORTPROTO;
			newhightext.frompoint = 0;
			newhightext.fromvarnoeval = NOVARIABLE;

			total = 0;
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto->primindex == 0 && (us_useroptions&NOINSTANCESELECT) != 0)
				{
					/* cell instance that is hard to select */
					if (findhard == 0) continue;
				} else
				{
					/* regular node: see if it should be selected */
					if (findeasy == 0 && (ni->userbits&HARDSELECTN) == 0) continue;
					if (findhard == 0 && (ni->userbits&HARDSELECTN) != 0) continue;
				}
				if (ni->proto->primindex != 0 && (ni->proto->userbits&NINVISIBLE) != 0)
					continue;

				/* if this is an invisible primitive with text, select the text */
				if (ni->proto == gen_invispinprim)
				{
					j = 0;
					for(i=0; i<ni->numvar; i++)
					{
						var = &ni->firstvar[i];
						if ((var->type&VDISPLAY) == 0) continue;
						newhightext.fromgeom = ni->geom;
						newhightext.fromvar = var;
						pt = us_makehighlightstring(&newhightext);
						(void)allocstring(&list[total], pt, el_tempcluster);
						total++;
						j = 1;
					}
					if (j != 0) continue;
				}

				newhigh.fromgeom = ni->geom;
				newhigh.fromport = NOPORTPROTO;
				if (findport != 0)
				{
					pp = ni->proto->firstportproto;
					if (pp != NOPORTPROTO && pp->nextportproto == NOPORTPROTO)
						newhigh.fromport = pp;
				}
				pt = us_makehighlightstring(&newhigh);
				(void)allocstring(&list[total], pt, el_tempcluster);
				total++;
			}
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			{
				if (findeasy == 0 && (ai->userbits&HARDSELECTA) == 0) continue;
				if (findhard == 0 && (ai->userbits&HARDSELECTA) != 0) continue;
				if ((ai->proto->userbits&AINVISIBLE) != 0) continue;
				newhigh.fromgeom = ai->geom;
				newhigh.fromport = NOPORTPROTO;
				pt = us_makehighlightstring(&newhigh);
				(void)allocstring(&list[total], pt, el_tempcluster);
				total++;
			}
			if (findeasy != 0)
			{
				for(i=0; i<np->numvar; i++)
				{
					var = &np->firstvar[i];
					if ((var->type&VDISPLAY) == 0) continue;
					newhightext.fromgeom = NOGEOM;
					newhightext.fromvar = var;
					pt = us_makehighlightstring(&newhightext);
					(void)allocstring(&list[total], pt, el_tempcluster);
					total++;
				}
			}
			if (total == 0) us_clearhighlightcount(); else
			{
				(void)setvalkey((INTBIG)us_tool, VTOOL, us_highlightedkey, (INTBIG)list,
					VSTRING|VISARRAY|(total<<VLENGTHSH)|VDONTSAVE);
			}
			for(i=0; i<total; i++) efree(list[i]);
			efree((CHAR *)list);
			us_showallhighlight();
			if ((us_state&NONPERSISTENTCURNODE) != 0) us_setnodeproto(NONODEPROTO);
			return;
		}

		if (namesamen(pt, x_("arc"), l) == 0 && l >= 3)
		{
			if (count <= 1)
			{
				ttyputusage(x_("find arc ARCNAME"));
				return;
			}
			if (findport != 0 || findpoint != 0 || findwithin != 0 || findspecial != 0)
			{
				us_abortcommand(M_("Illegal options given to 'find arc'"));
				return;
			}
			if (findexclusively != 0 && us_curarcproto == NOARCPROTO)
			{
				us_abortcommand(M_("Must select an arc for exclusive finding"));
				return;
			}
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			{
				if (findexclusively != 0 && ai->proto != us_curarcproto) continue;
				var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
				if (var == NOVARIABLE) continue;
				if (namesame((CHAR *)var->addr, par[1]) == 0) break;
			}
			if (ai == NOARCINST)
			{
				us_abortcommand(_("Sorry, no %s arc named '%s' in this cell"),
					(findexclusively==0 ? x_("") : describearcproto(us_curarcproto)), par[1]);
				return;
			}
			newhigh.status = HIGHFROM;
			newhigh.fromgeom = ai->geom;
			newhigh.cell = np;
			us_setfind(&newhigh, 0, extrainfo, findmore, findnobox);
			return;
		}

		if (namesamen(pt, x_("area-define"), l) == 0 && l >= 6)
		{
			if (findport != 0 || findpoint != 0 || findexclusively != 0 ||
				extrainfo != 0 || findwithin != 0 || findmore != 0 || findnobox != 0 ||
				findspecial != 0)
			{
				us_abortcommand(M_("Illegal options given to 'find area-define'"));
				return;
			}
			if ((el_curwindowpart->state&WINDOWOUTLINEEDMODE) != 0)
			{
				/* nonsensical in outline-edit mode */
				return;
			}

			us_clearhighlightcount();
			waitforpush = FALSE;
			if (count >= 2)
			{
				l = estrlen(par[1]);
				if (namesamen(par[1], x_("wait"), l) == 0 && l >= 1)
					waitforpush = TRUE;
			}
			trackcursor(waitforpush, us_ignoreup, us_finddbegin, us_stretchdown, us_stoponchar,
				us_invertdragup, TRACKDRAGGING);
			if (el_pleasestop != 0) return;
			np = getcurcell();
			if (np == NONODEPROTO) return;
			us_finddterm(&newhigh.stalx, &newhigh.staly);
			if (us_demandxy(&xcur, &ycur)) return;
			if (xcur >= newhigh.stalx) newhigh.stahx = xcur; else
			{
				newhigh.stahx = newhigh.stalx;   newhigh.stalx = xcur;
			}
			if (ycur >= newhigh.staly) newhigh.stahy = ycur; else
			{
				newhigh.stahy = newhigh.staly;   newhigh.staly = ycur;
			}
			newhigh.status = HIGHBBOX;
			newhigh.cell = np;
			us_addhighlight(&newhigh);
			return;
		}

		if (namesamen(pt, x_("area-move"), l) == 0 && l >= 6)
		{
			if (findport != 0 || findpoint != 0 || findexclusively != 0 ||
				extrainfo != 0 || findwithin != 0 || findmore != 0 || findnobox != 0 ||
				findspecial != 0)
			{
				us_abortcommand(M_("Illegal options given to 'find area-move'"));
				return;
			}
			if (us_demandxy(&xcur, &ycur)) return;
			gridalign(&xcur, &ycur, 1, np);

			/* set the highlight */
			if ((newhigh.status&HIGHTYPE) == HIGHBBOX)
			{
				areasizex = newhigh.stahx - newhigh.stalx;
				areasizey = newhigh.stahy - newhigh.staly;
			} else
			{
				areasizex = (el_curwindowpart->screenhx-el_curwindowpart->screenlx) / 5;
				areasizey = (el_curwindowpart->screenhy-el_curwindowpart->screenly) / 5;
			}

			us_clearhighlightcount();

			/* adjust the cursor position if selecting interactively */
			if ((us_tool->toolstate&INTERACTIVE) != 0)
			{
				us_findinit(areasizex, areasizey);
				trackcursor(FALSE, us_ignoreup, us_findmbegin, us_dragdown, us_stoponchar,
					us_dragup, TRACKDRAGGING);
				if (el_pleasestop != 0) return;
				if (us_demandxy(&xcur, &ycur)) return;
				gridalign(&xcur, &ycur, 1, np);
			}
			newhigh.status = HIGHBBOX;
			newhigh.cell = np;
			newhigh.stalx = xcur;   newhigh.stahx = xcur + areasizex;
			newhigh.staly = ycur;   newhigh.stahy = ycur + areasizey;
			us_addhighlight(&newhigh);
			return;
		}

		if (namesamen(pt, x_("area-size"), l) == 0 && l >= 6)
		{
			if (findport != 0 || findpoint != 0 || findexclusively != 0 ||
				extrainfo != 0 || findwithin != 0 || findmore != 0 || findnobox != 0 ||
				findspecial != 0)
			{
				us_abortcommand(M_("Illegal options given to 'find area-size'"));
				return;
			}
			if (us_demandxy(&xcur, &ycur)) return;
			gridalign(&xcur, &ycur, 1, np);

			if (newhigh.status != HIGHBBOX)
			{
				us_abortcommand(M_("Use 'find area-move' first, then this"));
				return;
			}
			if (np != newhigh.cell)
			{
				us_abortcommand(M_("Not in same cell as highlight area"));
				return;
			}

			us_clearhighlightcount();

			/* adjust the cursor position if selecting interactively */
			if ((us_tool->toolstate&INTERACTIVE) != 0)
			{
				us_findinit(newhigh.stalx, newhigh.staly);
				trackcursor(FALSE, us_ignoreup, us_findsbegin, us_stretchdown,
					us_stoponchar, us_invertdragup, TRACKDRAGGING);
				if (el_pleasestop != 0) return;
				if (us_demandxy(&xcur, &ycur)) return;
				gridalign(&xcur, &ycur, 1, np);
			}
			if (xcur >= newhigh.stalx) newhigh.stahx = xcur; else
			{
				newhigh.stahx = newhigh.stalx;   newhigh.stalx = xcur;
			}
			if (ycur >= newhigh.staly) newhigh.stahy = ycur; else
			{
				newhigh.stahy = newhigh.staly;   newhigh.staly = ycur;
			}
			us_addhighlight(&newhigh);
			return;
		}

		if (namesamen(pt, x_("clear"), l) == 0 && l >= 2)
		{
			us_clearhighlightcount();
			return;
		}

		if (namesamen(pt, x_("comp-interactive"), l) == 0 && l >= 3)
		{
			if (findpoint != 0 || findexclusively != 0 || extrainfo != 0)
			{
				us_abortcommand(M_("Illegal options given to 'find comp-interactive'"));
				return;
			}
			if ((el_curwindowpart->state&WINDOWOUTLINEEDMODE) != 0)
			{
				/* nonsensical in outline-edit mode */
				return;
			}
			var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_interactiveanglekey);
			if (var == NOVARIABLE) findangle = 0; else
				findangle = var->addr;
			us_findiinit(findport, extrainfo, findangle, 1-findmore, findstill,
				findnobox, findspecial);
			trackcursor(FALSE, us_ignoreup, us_findcibegin, us_findidown, us_stoponchar,
				us_findiup, TRACKDRAGGING);
			return;
		}

		if (namesamen(pt, x_("deselect-arcs"), l) == 0 && l >= 2)
		{
			var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
			if (var == NOVARIABLE) return;
			len = getlength(var);
			list = (CHAR **)emalloc(len * (sizeof (CHAR *)), el_tempcluster);
			if (list == 0) return;
			j = 0;
			for(i=0; i<len; i++)
			{
				if (us_makehighlight(((CHAR **)var->addr)[i], &newhigh)) break;
				if ((newhigh.status&HIGHTYPE) == HIGHFROM &&
					!newhigh.fromgeom->entryisnode) continue;
				(void)allocstring(&list[j], ((CHAR **)var->addr)[i], el_tempcluster);
				j++;
			}
			if (j == 0)
			{
				(void)delvalkey((INTBIG)us_tool, VTOOL, us_highlightedkey);
			} else
			{
				(void)setvalkey((INTBIG)us_tool, VTOOL, us_highlightedkey, (INTBIG)list,
					VSTRING|VISARRAY|(j<<VLENGTHSH)|VDONTSAVE);
			}
			for(i=0; i<j; i++)
				efree((CHAR *)list[i]);
			efree((CHAR *)list);
			return;
		}

		if (namesamen(pt, x_("down-stack"), l) == 0 && l >= 2)
		{
			if (findport != 0 || findpoint != 0 || findexclusively != 0 ||
				extrainfo != 0 || findwithin != 0 || findmore != 0 || findspecial != 0)
			{
				us_abortcommand(M_("Illegal options given to 'find down-stack'"));
				return;
			}
			us_pushhighlight();
			ttyputverbose(M_("Pushed"));
			return;
		}

		if (namesamen(pt, x_("dragging-selects"), l) == 0 && l >= 2)
		{
			if (findport != 0 || findpoint != 0 || findexclusively != 0 ||
				extrainfo != 0 || findwithin != 0 || findmore != 0 || findspecial != 0)
			{
				us_abortcommand(M_("Illegal options given to 'find dragging-selects'"));
				return;
			}
			if (count <= 1)
			{
				ttyputusage(x_("find dragging-selects WHAT"));
				return;
			}
			len = estrlen(pt = par[1]);
			if (namesamen(pt, x_("any-touching"), len) == 0)
			{
				(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey,
					us_useroptions & ~MUSTENCLOSEALL, VINTEGER);
				ttyputverbose(M_("Dragging selects anything touching the area"));
				return;
			}
			if (namesamen(pt, x_("only-enclosed"), len) == 0)
			{
				(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey,
					us_useroptions | MUSTENCLOSEALL, VINTEGER);
				ttyputverbose(M_("Dragging selects only objects inside the area"));
				return;
			}
			ttyputbadusage(x_("find dragging-selects"));
			return;
		}

		if (namesamen(pt, x_("export"), l) == 0 && l >= 3)
		{
			if (count <= 1)
			{
				ttyputusage(x_("find export PORTNAME"));
				return;
			}
			if (findport != 0 || findpoint != 0 || findexclusively != 0 || findwithin != 0 ||
				findspecial != 0)
			{
				us_abortcommand(M_("'find export' cannot accept other control"));
				return;
			}
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				if (namesame(par[1], pp->protoname) == 0) break;
			if (pp == NOPORTPROTO)
			{
				us_abortcommand(_("Sorry, no export named '%s' in this cell"), par[1]);
				return;
			}
			newhigh.status = HIGHFROM;
			newhigh.fromgeom = pp->subnodeinst->geom;
			newhigh.fromport = pp->subportproto;
			newhigh.cell = np;
			us_setfind(&newhigh, 0, extrainfo, findmore, findnobox);
			return;
		}

		if (namesamen(pt, x_("interactive"), l) == 0)
		{
			if ((el_curwindowpart->state&WINDOWOUTLINEEDMODE) != 0)
			{
				/* do outline select and move if in that mode */
				findwithin = 1;
				findpoint = 1;
			}
			if (findexclusively != 0)
			{
				us_abortcommand(M_("Illegal options given to 'find interactively'"));
				return;
			}

			/* special case: "find within vertex interactive" for polygon-editing */
			if (findpoint != 0 && findwithin != 0)
			{
				if (newhigh.fromgeom == NOGEOM)
				{
					us_abortcommand(M_("Cannot edit this object"));
					return;
				}
				ni = newhigh.fromgeom->entryaddr.ni;
				us_pointinit(ni, 0);
				trackcursor(FALSE, us_ignoreup, us_findpointbegin, us_movepdown,
					us_stoponchar, us_dragup, TRACKDRAGGING);
				if (el_pleasestop != 0) return;

				highvar = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
				if (highvar == NOVARIABLE) return;
				(void)us_makehighlight(((CHAR **)highvar->addr)[0], &newhigh);
				if (us_demandxy(&xcur, &ycur)) return;
				gridalign(&xcur, &ycur, 1, np);
				var = gettrace(ni);
				if (var == NOVARIABLE) return;
				size = getlength(var) / 2;
				newlist = (INTBIG *)emalloc((size*2*SIZEOFINTBIG), el_tempcluster);
				if (newlist == 0) return;
				makerot(ni, trans);
				x = (ni->highx + ni->lowx) / 2;
				y = (ni->highy + ni->lowy) / 2;
				for(i=0; i<size; i++)
				{
					if (i+1 == newhigh.frompoint)
					{
						newlist[i*2] = xcur;
						newlist[i*2+1] = ycur;
					} else xform(((INTBIG *)var->addr)[i*2]+x, ((INTBIG *)var->addr)[i*2+1]+y,
						&newlist[i*2], &newlist[i*2+1], trans);
				}

				/* now re-draw this trace */
				us_pushhighlight();
				us_clearhighlightcount();
				us_settrace(ni, newlist, size);
				us_pophighlight(FALSE);
				efree((CHAR *)newlist);
				return;
			}

			/* traditional interactive selection */
			var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_interactiveanglekey);
			if (var == NOVARIABLE) findangle = 0; else
				findangle = var->addr;
			us_findiinit(findport, extrainfo, findangle, 1-findmore, findstill,
				findnobox, findspecial);
			trackcursor(FALSE, us_ignoreup, us_findibegin, us_findidown, us_stoponchar,
				us_findiup, TRACKDRAGGING);
			return;
		}

		if (namesamen(pt, x_("just-objects"), l) == 0)
		{
			/* must be a single "area select" */
			var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
			if (var == NOVARIABLE) return;
			len = getlength(var);
			if (len != 1) return;
			(void)us_makehighlight(((CHAR **)var->addr)[0], &newhigh);
			if ((newhigh.status&HIGHTYPE) != HIGHBBOX) return;

			/* remove it and select everything in that area */
			np = newhigh.cell;
			us_clearhighlightcount();
			total = us_selectarea(np, newhigh.stalx, newhigh.stahx, newhigh.staly,
				newhigh.stahy, 0, 0, 0, &list);
			if (total > 0)
				(void)setvalkey((INTBIG)us_tool, VTOOL, us_highlightedkey, (INTBIG)list,
					VSTRING|VISARRAY|(total<<VLENGTHSH)|VDONTSAVE);
			return;
		}

		if (namesamen(pt, x_("name"), l) == 0 && l >= 2)
		{
			if (count <= 1)
			{
				ttyputusage(x_("find name HIGHLIGHTNAME"));
				return;
			}
			if (findport != 0 || findpoint != 0 || findexclusively != 0 ||
				findwithin != 0 || findmore != 0 || findspecial != 0)
			{
				us_abortcommand(M_("Illegal options given to 'find name'"));
				return;
			}
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("USER_highlight_"));
			addstringtoinfstr(infstr, par[1]);
			var = getval((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, returninfstr(infstr));
			if (var == NOVARIABLE)
			{
				us_abortcommand(_("Cannot find saved highlight '%s'"), par[1]);
				return;
			}

			(void)setvalkey((INTBIG)us_tool, VTOOL, us_highlightedkey, var->addr, var->type|VDONTSAVE);
			if ((us_state&NONPERSISTENTCURNODE) != 0) us_setnodeproto(NONODEPROTO);
			return;
		}

		if (namesamen(pt, x_("node"), l) == 0 && l >= 3)
		{
			if (count <= 1)
			{
				ttyputusage(x_("find node NODENAME"));
				return;
			}
			if (findport != 0 || findwithin != 0 || findspecial != 0)
			{
				us_abortcommand(M_("Illegal options given to 'find node'"));
				return;
			}
			if (findexclusively != 0 && us_curnodeproto == NONODEPROTO)
			{
				us_abortcommand(M_("Must select a node for exclusive finding"));
				return;
			}
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (findexclusively != 0 && ni->proto != us_curnodeproto) continue;
				var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
				if (var == NOVARIABLE) continue;
				if (namesame((CHAR *)var->addr, par[1]) == 0) break;
			}
			if (ni == NONODEINST)
			{
				us_abortcommand(_("Sorry, no %s node named '%s' in this cell"),
					(findexclusively==0 ? x_("") : describenodeproto(us_curnodeproto)), par[1]);
				return;
			}
			newhigh.status = HIGHFROM;
			newhigh.fromgeom = ni->geom;
			newhigh.fromport = NOPORTPROTO;
			newhigh.cell = np;
			us_setfind(&newhigh, findpoint, extrainfo, findmore, findnobox);
			return;
		}

		if (namesamen(pt, x_("nonmanhattan"), l) == 0 && l >= 3)
		{
			i = 0;
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
				for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
					onp->temp1 = 0;
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			{
				for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
				{
					for(ai = onp->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
					{
						if (ai->proto->tech == gen_tech || ai->proto->tech == art_tech ||
							ai->proto->tech == sch_tech)
								continue;
						var = getvalkey((INTBIG)ai, VARCINST, VINTEGER, el_arc_radius_key);
						if (var != NOVARIABLE || (ai->end[0].xpos != ai->end[1].xpos &&
							ai->end[0].ypos != ai->end[1].ypos)) onp->temp1++;
					}
					for(ni = onp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
					{
						if ((ni->rotation % 900) != 0) onp->temp1++;
					}
				}
			}
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			{
				if (ai->proto->tech == gen_tech || ai->proto->tech == art_tech)
					continue;
				var = getvalkey((INTBIG)ai, VARCINST, VINTEGER, el_arc_radius_key);
				if (var == NOVARIABLE && (ai->end[0].xpos == ai->end[1].xpos ||
					ai->end[0].ypos == ai->end[1].ypos)) continue;
				if (i == 0) us_clearhighlightcount();
				newhigh.status = HIGHFROM;
				newhigh.cell = np;
				newhigh.fromgeom = ai->geom;
				newhigh.fromport = NOPORTPROTO;
				newhigh.frompoint = 0;
				us_addhighlight(&newhigh);
				i++;
			}
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if ((ni->rotation % 900) == 0) continue;
				if (i == 0) us_clearhighlightcount();
				newhigh.status = HIGHFROM;
				newhigh.cell = np;
				newhigh.fromgeom = ni->geom;
				newhigh.fromport = NOPORTPROTO;
				newhigh.frompoint = 0;
				us_addhighlight(&newhigh);
				i++;
			}
			if (i == 0) ttyputmsg(_("No nonmanhattan objects in this cell")); else
				ttyputmsg(_("%ld objects are not manhattan in this cell"), i);
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			{
				if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
				i = 0;
				for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
					if (onp != np) i += onp->temp1;
				if (i == 0) continue;
				if (lib == el_curlib)
				{
					l = 0;
					infstr = initinfstr();
					for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
					{
						if (onp == np || onp->temp1 == 0) continue;
						if (l != 0) addtoinfstr(infstr, ' ');
						addstringtoinfstr(infstr, describenodeproto(onp));
						l++;
					}
					if (l == 1)
					{
						ttyputmsg(_("Found nonmanhattan geometry in cell %s"), returninfstr(infstr));
					} else
					{
						ttyputmsg(_("Found nonmanhattan geometry in these cells: %s"),
							returninfstr(infstr));
					}
				} else
				{
					ttyputmsg(_("Found nonmanhattan geometry in library %s"), lib->libname);
				}
			}
			return;
		}

		if (namesamen(pt, x_("object"), l) == 0)
		{
			if (count < 2)
			{
				ttyputusage(x_("find object (TYPE ADDRESS | TYPEADDRESS)"));
				return;
			}
			if (findport != 0 || findpoint != 0 || findexclusively != 0 ||
				findwithin != 0 || findspecial != 0)
			{
				us_abortcommand(M_("Illegal options given to 'find object'"));
				return;
			}

			/* determine type and address to highlight */
			if (count == 3)
			{
				type = us_variabletypevalue(par[1]);
				addr = myatoi(par[2]);
			} else
			{
				type = VUNKNOWN;
				if (namesamen(par[1], x_("node"), 4) == 0)
				{
					type = VNODEINST;
					addr = myatoi(&par[1][4]);
				}
				if (namesamen(par[1], x_("arc"), 3) == 0)
				{
					type = VARCINST;
					addr = myatoi(&par[1][3]);
				}
				if (namesamen(par[1], x_("port"), 4) == 0)
				{
					type = VPORTPROTO;
					addr = myatoi(&par[1][4]);
				}
				if (namesamen(par[1], x_("network"), 7) == 0)
				{
					type = VNETWORK;
					addr = myatoi(&par[1][7]);
				}
			}
			if (type == VUNKNOWN)
			{
				us_abortcommand(_("Unknown object type in 'find object' command"));
				return;
			}
			switch (type)
			{
				case VNODEINST:
					ni = (NODEINST *)addr;
					if (ni == 0 || ni == NONODEINST) return;
					if (ni->parent != np)
					{
						us_abortcommand(_("Cannot find node %ld in this cell"), addr);
						return;
					}
					newhigh.status = HIGHFROM;
					newhigh.fromgeom = ni->geom;
					newhigh.fromport = NOPORTPROTO;
					newhigh.cell = np;
					us_setfind(&newhigh, findpoint, extrainfo, findmore, findnobox);
					break;
				case VARCINST:
					ai = (ARCINST *)addr;
					if (ai == 0 || ai == NOARCINST) return;
					if (ai->parent != np)
					{
						us_abortcommand(_("Cannot find arc %ld in this cell"), addr);
						return;
					}
					newhigh.status = HIGHFROM;
					newhigh.fromgeom = ai->geom;
					newhigh.fromport = NOPORTPROTO;
					newhigh.cell = np;
					us_setfind(&newhigh, findpoint, extrainfo, findmore, findnobox);
					break;
				case VNETWORK:
					net = (NETWORK *)addr;
					if (net == 0 || net == NONETWORK) return;
					if (net->parent != np)
					{
						us_abortcommand(_("Cannot find network %ld in this cell"), addr);
						return;
					}
					for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
					{
						if (ai->network != net) continue;
						newhigh.status = HIGHFROM;
						newhigh.fromgeom = ai->geom;
						newhigh.fromport = NOPORTPROTO;
						newhigh.cell = np;
						us_setfind(&newhigh, findpoint, extrainfo, findmore, findnobox);
					}
					for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
					{
						if (pp->network != net) continue;
						newhigh.status = HIGHFROM;
						newhigh.fromgeom = pp->subnodeinst->geom;
						newhigh.fromport = pp->subportproto;
						newhigh.cell = np;
						us_setfind(&newhigh, 0, extrainfo, findmore, findnobox);
					}
					break;
				case VPORTPROTO:
					pp = (PORTPROTO *)addr;
					if (pp == 0 || pp == NOPORTPROTO) return;
					if (pp->parent != np)
					{
						us_abortcommand(_("Cannot find port %ld in this cell"), addr);
						return;
					}
					newhigh.status = HIGHFROM;
					newhigh.fromgeom = pp->subnodeinst->geom;
					newhigh.fromport = pp->subportproto;
					newhigh.cell = np;
					us_setfind(&newhigh, 0, extrainfo, findmore, findnobox);
					break;
				default:
					us_abortcommand(_("Cannot highlight objects of type %s"),
						us_variabletypename(type));
					return;
			}
			return;
		}

		if (namesamen(pt, x_("save"), l) == 0 && l >= 2)
		{
			if (count <= 1)
			{
				ttyputusage(x_("find save HIGHLIGHTNAME"));
				return;
			}
			if (findport != 0 || findpoint != 0 || findexclusively != 0 ||
				extrainfo != 0 || findwithin != 0 || findmore != 0 || findspecial != 0)
			{
				us_abortcommand(M_("Illegal options given to 'find save'"));
				return;
			}
			var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
			if (var == NOVARIABLE)
			{
				us_abortcommand(_("Highlight something before saving highlight"));
				return;
			}
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("USER_highlight_"));
			addstringtoinfstr(infstr, par[1]);
			(void)setval((INTBIG)us_tool, VTOOL, returninfstr(infstr), var->addr, var->type|VDONTSAVE);
			ttyputverbose(M_("%s saved"), par[1]);
			return;
		}

		if (namesamen(pt, x_("set-easy-selection"), l) == 0 && l >= 5)
		{
			glist = us_gethighlighted(WANTARCINST|WANTNODEINST, 0, 0);
			if (glist[0] == NOGEOM)
			{
				us_abortcommand(_("Select something before making it easy-to-select"));
				return;
			}
			for(i=0; glist[i] != NOGEOM; i++)
			{
				geom = glist[i];
				if (!geom->entryisnode)
				{
					ai = geom->entryaddr.ai;
					ai->userbits &= ~HARDSELECTA;
				} else
				{
					ni = geom->entryaddr.ni;
					ni->userbits &= ~HARDSELECTN;
				}
			}
			return;
		}

		if (namesamen(pt, x_("set-hard-selection"), l) == 0 && l >= 5)
		{
			glist = us_gethighlighted(WANTARCINST|WANTNODEINST, 0, 0);
			if (glist[0] == NOGEOM)
			{
				us_abortcommand(_("Select something before making it easy-to-select"));
				return;
			}
			for(i=0; glist[i] != NOGEOM; i++)
			{
				geom = glist[i];
				if (!geom->entryisnode)
				{
					ai = geom->entryaddr.ai;
					ai->userbits |= HARDSELECTA;
				} else
				{
					ni = geom->entryaddr.ni;
					ni->userbits |= HARDSELECTN;
				}
			}
			return;
		}

		if (namesamen(pt, x_("similar"), l) == 0 && l >= 2)
		{
			if (findpoint != 0 || findwithin != 0 || findmore != 0 ||
				findexclusively != 0 || findstill != 0 || findspecial != 0)
			{
				us_abortcommand(M_("Illegal options given to 'find similar'"));
				return;
			}

			glist = us_gethighlighted(WANTNODEINST|WANTARCINST, 0, 0);
			if (glist[0] == NOGEOM)
			{
				us_abortcommand(_("Must select objects before selecting all like them"));
				return;
			}
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				ni->temp1 = 0;
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
				ai->temp1 = 0;
			for(i=0; glist[i] != NOGEOM; i++)
			{
				if (glist[i]->entryisnode)
				{
					for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
						if (ni->proto == glist[i]->entryaddr.ni->proto) ni->temp1 = 1;
				} else
				{
					for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
						if (ai->proto == glist[i]->entryaddr.ai->proto) ai->temp1 = 1;
				}
			}
			infstr = initinfstr();
			first = 0;
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->temp1 == 0) continue;
				if (first != 0) addtoinfstr(infstr, '\n');
				first++;
				formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0"),
					describenodeproto(np), (INTBIG)ni->geom);
			}
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			{
				if (ai->temp1 == 0) continue;
				if (first != 0) addtoinfstr(infstr, '\n');
				first++;
				formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0"),
					describenodeproto(np), (INTBIG)ai->geom);
			}
			us_setmultiplehighlight(returninfstr(infstr), FALSE);
			return;
		}

		if (namesamen(pt, x_("snap-mode"), l) == 0 && l >= 2)
		{
			if (findport != 0 || findpoint != 0 || findexclusively != 0 ||
				extrainfo != 0 || findwithin != 0 || findmore != 0 || findspecial != 0)
			{
				us_abortcommand(M_("Illegal options given to 'find snap-mode'"));
				return;
			}

			if (count < 2)
			{
				switch (us_state&SNAPMODE)
				{
					case SNAPMODENONE:     ttyputmsg(M_("Snapping mode: none"));              break;
					case SNAPMODECENTER:   ttyputmsg(M_("Snapping mode: center"));            break;
					case SNAPMODEMIDPOINT: ttyputmsg(M_("Snapping mode: midpoint"));          break;
					case SNAPMODEENDPOINT: ttyputmsg(M_("Snapping mode: end point"));         break;
					case SNAPMODETANGENT:  ttyputmsg(M_("Snapping mode: tangent"));           break;
					case SNAPMODEPERP:     ttyputmsg(M_("Snapping mode: perpendicular"));     break;
					case SNAPMODEQUAD:     ttyputmsg(M_("Snapping mode: quadrant"));          break;
					case SNAPMODEINTER:    ttyputmsg(M_("Snapping mode: any intersection"));  break;
				}
				return;
			}
			l = estrlen(pt = par[1]);
			if (namesamen(pt, x_("none"), l) == 0)
			{
				us_state = (us_state & ~SNAPMODE) | SNAPMODENONE;
				ttyputverbose(M_("Snapping mode: none"));
				return;
			}
			if (namesamen(pt, x_("center"), l) == 0)
			{
				us_state = (us_state & ~SNAPMODE) | SNAPMODECENTER;
				ttyputverbose(M_("Snapping mode: center"));
				return;
			}
			if (namesamen(pt, x_("midpoint"), l) == 0)
			{
				us_state = (us_state & ~SNAPMODE) | SNAPMODEMIDPOINT;
				ttyputverbose(M_("Snapping mode: midpoint"));
				return;
			}
			if (namesamen(pt, x_("endpoint"), l) == 0)
			{
				us_state = (us_state & ~SNAPMODE) | SNAPMODEENDPOINT;
				ttyputverbose(M_("Snapping mode: endpoint"));
				return;
			}
			if (namesamen(pt, x_("tangent"), l) == 0)
			{
				us_state = (us_state & ~SNAPMODE) | SNAPMODETANGENT;
				ttyputverbose(M_("Snapping mode: tangent"));
				return;
			}
			if (namesamen(pt, x_("perpendicular"), l) == 0)
			{
				us_state = (us_state & ~SNAPMODE) | SNAPMODEPERP;
				ttyputverbose(M_("Snapping mode: perpendicular"));
				return;
			}
			if (namesamen(pt, x_("quadrant"), l) == 0)
			{
				us_state = (us_state & ~SNAPMODE) | SNAPMODEQUAD;
				ttyputverbose(M_("Snapping mode: quadrant"));
				return;
			}
			if (namesamen(pt, x_("intersection"), l) == 0)
			{
				us_state = (us_state & ~SNAPMODE) | SNAPMODEINTER;
				ttyputverbose(M_("Snapping mode: any intersection"));
				return;
			}
			us_abortcommand(M_("Unknown snapping mode: %s"), pt);
			return;
		}

		if (namesamen(pt, x_("up-stack"), l) == 0 && l >= 1)
		{
			if (findport != 0 || findpoint != 0 || findexclusively != 0 ||
				extrainfo != 0 || findwithin != 0 || findmore != 0 || findspecial != 0)
			{
				us_abortcommand(M_("Illegal options given to 'find up-stack'"));
				return;
			}
			us_pophighlight(FALSE);
			return;
		}

		if (namesamen(pt, x_("variable"), l) == 0 && l >= 2)
		{
			if (count <= 1)
			{
				ttyputusage(x_("find variable VARNAME"));
				return;
			}
			if (findport != 0 || findpoint != 0 || findexclusively != 0 || findwithin != 0 ||
				findspecial != 0)
			{
				us_abortcommand(M_("'find variable' cannot accept other control"));
				return;
			}
			ni = (NODEINST *)us_getobject(VNODEINST, TRUE);
			if (ni == NONODEINST) return;
			for(i=0; i<ni->numvar; i++)
			{
				var = &ni->firstvar[i];
				if ((var->type&VDISPLAY) == 0) continue;
				if (namesame(par[1], makename(var->key)) == 0) break;
			}
			if (i >= ni->numvar)
			{
				us_abortcommand(_("Sorry, no variable named '%s' on the current node"),
					par[1]);
				return;
			}
			newhigh.status = HIGHTEXT;
			newhigh.fromgeom = ni->geom;
			newhigh.fromport = NOPORTPROTO;
			newhigh.fromvar = var;
			newhigh.cell = np;
			us_setfind(&newhigh, 0, extrainfo, findmore, findnobox);
			return;
		}
		ttyputbadusage(x_("find"));
		return;
	}

	/* get the cursor co-ordinates */
	if (us_demandxy(&xcur, &ycur)) return;

	/* find the closest object to the cursor */
	if (findwithin == 0)
		us_findobject(xcur, ycur, el_curwindowpart, &newhigh, findexclusively, 0, findport, 0, findspecial);
	if (newhigh.status == 0) return;
	if (findport == 0) newhigh.fromport = NOPORTPROTO;
	newhigh.cell = np;
	us_setfind(&newhigh, findpoint, extrainfo, findmore, findnobox);
}

void us_getproto(INTBIG count, CHAR *par[])
{
	REGISTER NODEPROTO *np, *onp;
	REGISTER ARCPROTO *ap, *lat;
	REGISTER INTBIG cellgroupcount, cellcount, doarc, i, l, firstinpass;
	BOOLEAN butstate;
	REGISTER CHAR *pp;
	HIGHLIGHT high;
	extern COMCOMP us_getproto1p;
	REGISTER VARIABLE *highvar;
	GEOM *fromgeom, *togeom;
	PORTPROTO *fromport, *toport;
	REGISTER USERCOM *uc, *ucsub;
	POPUPMENU *pm, *cpm, *pmsub;
	REGISTER POPUPMENUITEM *mi, *miret, *misub;
	extern COMCOMP us_userp;
	REGISTER void *infstr;

	if (count == 0)
	{
		count = ttygetparam(M_("Getproto option: "), &us_getproto1p, MAXPARS, par);
		if (count == 0)
		{
			us_abortedmsg();
			return;
		}
	}
	l = estrlen(pp = par[0]);

	if (namesamen(pp, x_("instance"), l) == 0)
	{
		/* show a popup menu with instances */

		/* number each cellgroup */
		for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = 0;
		cellgroupcount = 0;
		for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->temp1 != 0) continue;
			cellgroupcount++;
			FOR_CELLGROUP(onp, np)
				onp->temp1 = cellgroupcount;
		}
		pm = (POPUPMENU *)emalloc(sizeof(POPUPMENU), us_tool->cluster);
		if (pm == 0) return;
		mi = (POPUPMENUITEM *)emalloc(cellgroupcount * sizeof(POPUPMENUITEM), us_tool->cluster);
		if (mi == 0) return;
		pm->name = x_("x");
		infstr = initinfstr();
		if (cellgroupcount != 0) formatinfstr(infstr, _("Cells in %s"), el_curlib->libname); else
			formatinfstr(infstr, _("No cells in %s"), el_curlib->libname);
		(void)allocstring(&pm->header, returninfstr(infstr), us_tool->cluster);
		pm->list = mi;
		pm->total = cellgroupcount;

		/* fill the menu */
		for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = 0;
		cellgroupcount = 0;
		for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->temp1 != 0) continue;
			cellcount = 0;
			FOR_CELLGROUP(onp, np)
			{
				onp->temp1 = 1;
				cellcount++;
			}

			uc = us_allocusercom();
			mi[cellgroupcount].response = uc;
			mi[cellgroupcount].response->active = -1;
			mi[cellgroupcount].value = 0;

			if (cellcount == 1)
			{
				/* only one cell of this cell: create it directly */
				uc->active = parse(x_("getproto"), &us_userp, TRUE);
				(void)allocstring(&uc->comname, x_("getproto"), us_tool->cluster);
				uc->count = 2;
				(void)allocstring(&uc->word[0], x_("node"), us_tool->cluster);
				(void)allocstring(&uc->word[1], describenodeproto(np), us_tool->cluster);
				mi[cellgroupcount].attribute = uc->word[1];
			} else
			{
				/* multiple cells of this cell: make a submenu */
				mi[cellgroupcount].attribute = np->protoname;
				mi[cellgroupcount].value = x_(">>");
				mi[cellgroupcount].maxlen = -1;
				pmsub = (POPUPMENU *)emalloc(sizeof(POPUPMENU), us_tool->cluster);
				if (pmsub == 0) return;
				misub = (POPUPMENUITEM *)emalloc(cellcount * sizeof(POPUPMENUITEM), us_tool->cluster);
				if (misub == 0) return;
				pmsub->name = x_("x");
				pmsub->header = 0;
				pmsub->list = misub;
				pmsub->total = cellcount;
				cellcount = 0;
				for(i=0; i<2; i++)
				{
					firstinpass = cellcount;
					FOR_CELLGROUP(onp, np)
					{
						/* see if this cell should be included on this pass */
						if (i == 0)
						{
							/* first pass: only include cells that match the current view */
							if (!us_cellfromtech(onp, el_curtech)) continue;
						} else
						{
							/* first pass: only include cells that don't match the current view */
							if (us_cellfromtech(onp, el_curtech)) continue;
						}

						ucsub = us_allocusercom();
						misub[cellcount].response = ucsub;
						misub[cellcount].response->active = -1;
						(void)allocstring(&misub[cellcount].attribute, describenodeproto(onp),
							us_tool->cluster);
						misub[cellcount].value = 0;

						/* only one cell of this cell: create it directly */
						ucsub->active = parse(x_("getproto"), &us_userp, TRUE);
						(void)allocstring(&ucsub->comname, x_("getproto"), us_tool->cluster);
						ucsub->count = 2;
						(void)allocstring(&ucsub->word[0], x_("node"), us_tool->cluster);
						(void)allocstring(&ucsub->word[1], describenodeproto(onp),
							us_tool->cluster);
						cellcount++;
					}
					esort(&misub[firstinpass], cellcount-firstinpass,
						sizeof (POPUPMENUITEM), us_sortpopupmenuascending);
				}
				uc->menu = pmsub;
			}
			cellgroupcount++;
		}

		/* invoke the popup menu */
		esort(mi, cellgroupcount, sizeof (POPUPMENUITEM), us_sortpopupmenuascending);
		butstate = TRUE;
		cpm = pm;
		miret = us_popupmenu(&cpm, &butstate, TRUE, -1, -1, 4);
		if (miret == 0)
		{
			us_abortcommand(_("Sorry, popup menus are not available"));
		} else
		{
			if (miret != NOPOPUPMENUITEM)
				us_execute(miret->response, FALSE, FALSE, FALSE);
		}
		for(i=0; i<cellgroupcount; i++)
		{
			uc = mi[i].response;
			if (uc->menu != NOPOPUPMENU)
			{
				for(l=0; l<uc->menu->total; l++)
				{
					efree((CHAR *)uc->menu->list[l].attribute);
					us_freeusercom(uc->menu->list[l].response);
				}
				efree((CHAR *)uc->menu->list);
				efree((CHAR *)uc->menu);
			}
			us_freeusercom(uc);
		}
		efree((CHAR *)mi);
		efree((CHAR *)pm->header);
		efree((CHAR *)pm);
		return;
	}

	if (namesamen(pp, x_("node"), l) == 0 && l >= 2 && count > 1)
	{
		np = getnodeproto(par[1]);
		if (np == NONODEPROTO)
		{
			us_abortcommand(_("Cannot find node '%s'"), par[1]);
			return;
		}
		us_setnodeproto(np);
		return;
	}
	if (namesamen(pp, x_("arc"), l) == 0 && l >= 1 && count > 1)
	{
		ap = getarcproto(par[1]);
		if (ap == NOARCPROTO) us_abortcommand(_("Cannot find arc '%s'"), par[1]); else
			us_setarcproto(ap, TRUE);
		return;
	}

	if (namesamen(pp, x_("this-proto"), l) == 0 && l >= 1)
	{
		np = us_needcell();
		if (np == NONODEPROTO) return;
		us_setnodeproto(np);
		return;
	}

	/* decide whether arcs are the default */
	doarc = 0;
	if (!us_gettwoobjects(&fromgeom, &fromport, &togeom, &toport)) doarc = 1; else
	{
		highvar = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
		if (highvar != NOVARIABLE && getlength(highvar) == 1)
		{
			(void)us_makehighlight(((CHAR **)highvar->addr)[0], &high);
			if ((high.status&HIGHFROM) != 0 && !high.fromgeom->entryisnode) doarc++;
		}
	}

	if (namesamen(pp, x_("next-proto"), l) == 0 && l >= 2)
	{
		if (doarc)
		{
			/* advance to the next arcproto */
			ap = us_curarcproto->nextarcproto;
			if (ap == NOARCPROTO) ap = el_curtech->firstarcproto;
			us_setarcproto(ap, TRUE);
		} else
		{
			/* advance to the next nodeproto */
			np = us_curnodeproto;
			if (np->primindex == 0) np = el_curtech->firstnodeproto; else
			{
				/* advance to next after "np" */
				np = np->nextnodeproto;
				if (np == NONODEPROTO) np = el_curtech->firstnodeproto;
			}
			us_setnodeproto(np);
		}
		return;
	}

	if (namesamen(pp, x_("prev-proto"), l) == 0 && l >= 1)
	{
		if (doarc)
		{
			/* backup to the previous arcproto */
			for(ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
				if (ap->nextarcproto == us_curarcproto) break;
			if (ap == NOARCPROTO)
				for(lat = el_curtech->firstarcproto; lat != NOARCPROTO; lat = lat->nextarcproto)
					ap = lat;
			us_setarcproto(ap, TRUE);
		} else
		{
			/* backup to the previous nodeproto */
			np = us_curnodeproto;
			if (np->primindex == 0) np = el_curtech->firstnodeproto; else
			{
				/* back up to previous of "np" */
				np = np->prevnodeproto;
				if (np == NONODEPROTO)
					for(np = el_curtech->firstnodeproto; np->nextnodeproto != NONODEPROTO;
						np = np->nextnodeproto) ;
			}
			us_setnodeproto(np);
		}
		return;
	}

	/* must be a prototype name */
	if (doarc != 0)
	{
		ap = getarcproto(pp);
		if (ap != NOARCPROTO)
		{
			us_setarcproto(ap, TRUE);
			return;
		}
	}

	np = getnodeproto(pp);
	if (np == NONODEPROTO)
	{
		if (doarc != 0) us_abortcommand(_("Cannot find node or arc '%s'"), pp); else
			us_abortcommand(_("Cannot find node '%s'"), pp);
		return;
	}
	us_setnodeproto(np);
}

void us_grid(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG i, j;
	REGISTER INTBIG l;
	REGISTER CHAR *pp;

	/* no arguments: toggle the grid state */
	if (count == 0)
	{
		if (us_needwindow()) return;
		if ((el_curwindowpart->state&WINDOWTYPE) != DISPWINDOW &&
		    (el_curwindowpart->state&WINDOWTYPE) != WAVEFORMWINDOW)
		{
			us_abortcommand(_("Cannot show grid in this type of window"));
			return;
		}

		/* save highlight */
		us_pushhighlight();
		us_clearhighlightcount();

		startobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);
		us_gridset(el_curwindowpart, ~el_curwindowpart->state);
		endobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);

		/* restore highlighting */
		us_pophighlight(FALSE);
		return;
	}

	l = estrlen(pp = par[0]);
	if (namesamen(pp, x_("alignment"), l) == 0 && l >= 1)
	{
		if (count >= 2)
		{
			i = atofr(par[1]);
			if (i < 0)
			{
				us_abortcommand(_("Alignment must be positive"));
				return;
			}
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_alignment_ratio_key, i, VINTEGER);
		}
		ttyputverbose(M_("Cursor alignment is %s lambda"), frtoa(us_alignment_ratio));
		return;
	}

	if (namesamen(pp, x_("edges"), l) == 0 && l >= 1)
	{
		if (count >= 2)
		{
			i = atofr(par[1]);
			if (i < 0)
			{
				us_abortcommand(_("Alignment must be positive"));
				return;
			}
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_alignment_edge_ratio_key, i, VINTEGER);
		}
		if (us_edgealignment_ratio == 0) ttyputverbose(M_("No edge alignment done")); else
			ttyputverbose(M_("Edge alignment is %s lambda"), frtoa(us_edgealignment_ratio));
		return;
	}

	if (namesamen(pp, x_("size"), l) == 0 && l >= 1)
	{
		if (count < 2)
		{
			ttyputusage(x_("grid size X [Y]"));
			return;
		}
		i = atola(par[1], 0);
		if (i&1) i++;
		if (count >= 3)
		{
			j = atola(par[2], 0);
			if (j&1) j++;
		} else j = i;
		if (i <= 0 || j <= 0)
		{
			us_abortcommand(_("Invalid grid spacing"));
			return;
		}

		if (us_needwindow()) return;

		/* save highlight */
		us_pushhighlight();
		us_clearhighlightcount();
		startobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);

		/* turn grid off if on */
		if ((el_curwindowpart->state&GRIDON) != 0)
			(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("state"),
				el_curwindowpart->state & ~GRIDON, VINTEGER);

		/* adjust grid */
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("gridx"), i, VINTEGER);
		(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("gridy"), j, VINTEGER);

		/* show new grid */
		us_gridset(el_curwindowpart, GRIDON);

		/* restore highlighting */
		endobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);
		us_pophighlight(FALSE);
		return;
	}
	ttyputbadusage(x_("grid"));
}

#define NEWSFILE   x_("newsfile")			/* file with news */

#define NEWSDATE   x_(".electricnews")		/* file with date of last news */

void us_help(INTBIG count, CHAR *par[])
{
	REGISTER CHAR *pp;
	REGISTER INTBIG len, lastquiet, nday, nmonth, nyear, on, filestatus;
	INTBIG day, month, year, hour, minute, second;
	FILE *in;
	CHAR line[256], *filename, *hd, *dummy;
	time_t clock;
	REGISTER void *infstr;

	if (count > 0) len = estrlen(pp = par[0]);

	/* show the user manual */
	if (count >= 1 && namesamen(pp, x_("manual"), len) == 0)
	{
#ifdef DOCDIR
		estrcpy(line, DOCDIR);
		estrcat(line, x_("index.html"));
		filestatus = fileexistence(line);
		if (filestatus == 1 || filestatus == 3)
		{
			if (browsefile(line))
			{
				us_abortcommand(_("Cannot bring up the user's manual on this system"));
			}
			return;
		}
#endif
		estrcpy(line, el_libdir);
		len = estrlen(line);
		if (line[len-1] == DIRSEP) line[len-1] = 0;
		len = estrlen(line);
		if (namesame(&line[len-3], x_("lib")) == 0) line[len-3] = 0;
		estrcat(line, x_("html"));
		estrcat(line, DIRSEPSTR);
		estrcat(line, x_("manual"));
		estrcat(line, DIRSEPSTR);
		estrcat(line, x_("index.html"));
		filestatus = fileexistence(line);
		if (filestatus != 1 && filestatus != 3)
		{
			us_abortcommand(_("Sorry, cannot locate the user's manual"));
			return;
		}

		if (browsefile(line))
		{
			us_abortcommand(_("Cannot bring up the user's manual on this system"));
		}
		return;
	}

	/* print news */
	if (count >= 1 && namesamen(pp, x_("news"), len) == 0)
	{
		/* determine last date of news reading */
		infstr = initinfstr();
		hd = hashomedir();
		if (hd == 0) addstringtoinfstr(infstr, el_libdir); else
			addstringtoinfstr(infstr, hd);
		addstringtoinfstr(infstr, NEWSDATE);
		pp = truepath(returninfstr(infstr));
		in = xopen(pp, el_filetypetext, 0, &dummy);
		if (in == 0) year = month = day = 0; else
		{
			fclose(in);
			clock = filedate(pp);
			parsetime(clock, &year, &month, &day, &hour, &minute, &second);
			month++;
		}

		/* get the news file */
		infstr = initinfstr();
		addstringtoinfstr(infstr, el_libdir);
		addstringtoinfstr(infstr, NEWSFILE);
		pp = truepath(returninfstr(infstr));
		in = xopen(pp, us_filetypenews, x_(""), &filename);
		if (in == NULL)
		{
			ttyputerr(_("Sorry, cannot find the news file: %s"), pp);
			return;
		}

		/* read the file */
		on = 0;

		/* enable messages (if they were off) */
		lastquiet = ttyquiet(0);
		for(;;)
		{
			if (xfgets(line, MAXLINE, in)) break;
			if (on != 0)
			{
				ttyputmsg(x_("%s"), line);
				continue;
			}
			if (line[0] == ' ' || line[0] == 0) continue;

			/* line with date, see if it is current */
			pp = line;
			nmonth = eatoi(pp);
			while (*pp != '/' && *pp != 0) pp++;
			if (*pp == '/') pp++;
			nday = eatoi(pp);
			while (*pp != '/' && *pp != 0) pp++;
			if (*pp == '/') pp++;
			nyear = eatoi(pp);
			if (nyear < year) continue; else if (nyear > year) on = 1;
			if (nmonth < month) continue; else if (nmonth > month) on = 1;
			if (nday >= day) on = 1;
			if (on != 0) ttyputmsg(x_("%s"), line);
		}
		xclose(in);

		/* restore message output state */
		(void)ttyquiet(lastquiet);

		if (on == 0) ttyputmsg(_("No news"));

		/* now mark the current date */
		infstr = initinfstr();
		hd = hashomedir();
		if (hd == 0) addstringtoinfstr(infstr, el_libdir); else
			addstringtoinfstr(infstr, hd);
		addstringtoinfstr(infstr, NEWSDATE);
		xclose(xcreate(truepath(returninfstr(infstr)), us_filetypenews, 0, 0));
		return;
	}

	/* illustrate commands */
	if (count >= 1 && namesamen(pp, x_("illustrate"), len) == 0)
	{
		us_illustratecommandset();
		return;
	}

	/* dump pulldown menus */
	if (count >= 1 && namesamen(pp, x_("pulldowns"), len) == 0)
	{
		us_dumppulldownmenus();
		return;
	}

	/* general dialog-based help on command-line */
	(void)us_helpdlog(x_("CL"));
}

void us_if(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG term1, term1type, term2, term2type;
	REGISTER INTBIG relation;
	REGISTER USERCOM *com;

	/* make sure the proper number of parameters is given */
	if (count < 4)
	{
		ttyputusage(x_("if TERM1 RELATION TERM2 COMMAND"));
		return;
	}

	/* get term 1 */
	if (isanumber(par[0]))
	{
		term1 = myatoi(par[0]);
		term1type = VINTEGER;
	} else
	{
		term1 = (INTBIG)par[0];
		term1type = VSTRING;
	}

	/* get term 2 */
	if (isanumber(par[2]))
	{
		term2 = myatoi(par[2]);
		term2type = VINTEGER;
	} else
	{
		term2 = (INTBIG)par[2];
		term2type = VSTRING;
	}

	/* make sure the two terms are comparable */
	if (term1type != term2type)
	{
		if (term1 == VINTEGER)
		{
			term1 = (INTBIG)par[0];
			term1type = VSTRING;
		} else
		{
			term2 = (INTBIG)par[1];
			term2type = VSTRING;
		}
	}

	/* determine the relation being tested */
	relation = -1;
	if (estrcmp(par[1], x_("==")) == 0) relation = 0;
	if (estrcmp(par[1], x_("!=")) == 0) relation = 1;
	if (estrcmp(par[1], x_("<"))  == 0) relation = 2;
	if (estrcmp(par[1], x_("<=")) == 0) relation = 3;
	if (estrcmp(par[1], x_(">"))  == 0) relation = 4;
	if (estrcmp(par[1], x_(">=")) == 0) relation = 5;
	if (relation < 0)
	{
		us_abortcommand(_("Unknown relation: %s"), par[1]);
		return;
	}

	/* make sure that qualitative comparison is done on numbers */
	if (relation > 1 && term1type != VINTEGER)
	{
		us_abortcommand(_("Inequality comparisons must be done on numbers"));
		return;
	}

	/* see if the command should be executed */
	switch (relation)
	{
		case 0:		/* == */
			if (term1type == VINTEGER)
			{
				if (term1 != term2) return;
			} else
			{
				if (namesame((CHAR *)term1, (CHAR *)term2) != 0) return;
			}
			break;
		case 1:		/* != */
			if (term1type == VINTEGER)
			{
				if (term1 == term2) return;
			} else
			{
				if (namesame((CHAR *)term1, (CHAR *)term2) == 0) return;
			}
			break;
		case 2:		/* < */
			if (term1 >= term2) return;
			break;
		case 3:		/* <= */
			if (term1 > term2) return;
			break;
		case 4:		/* > */
			if (term1 <= term2) return;
			break;
		case 5:		/* >= */
			if (term1 < term2) return;
			break;
	}

	/* condition is true: create the command to execute */
	com = us_buildcommand(count-3, &par[3]);
	if (com == NOUSERCOM)
	{
		us_abortcommand(_("Condition true but there is no command to execute"));
		return;
	}
	us_execute(com, FALSE, FALSE, FALSE);
	us_freeusercom(com);
}

void us_interpret(INTBIG count, CHAR *par[])
{
	REGISTER CHAR *pp;
	REGISTER INTBIG language, len;
	REGISTER VARIABLE *var;
	REGISTER BOOLEAN fromfile;

	language = VLISP;
	fromfile = FALSE;
	if (namesamen(par[0], x_("file"), estrlen(par[0])) == 0)
	{
		count--;
		par++;
		fromfile = TRUE;
	}

	if (count > 0)
	{
		len = estrlen(pp = par[0]);
		if (namesamen(pp, x_("lisp"), len) == 0)
		{
			language = VLISP;
			count--;
			par++;
		} else if (namesamen(pp, x_("tcl"), len) == 0)
		{
			language = VTCL;
			count--;
			par++;
		} else if (namesamen(pp, x_("java"), len) == 0)
		{
			language = VJAVA;
			count--;
			par++;
		}
	}

	/* handle "file" option */
	if (fromfile)
	{
		if (count <= 0)
		{
			ttyputusage(x_("interpret file LANGUAGE FILENAME"));
			return;
		}
		if (loadcode(par[0], language))
			ttyputerr(_("Error loading code"));
		return;
	}

	/* with no parameters, simply drop into the interpreter loop */
	if (count == 0)
	{
		/* "languageconverse" returns false if it wants to continue conversation */
		if (!languageconverse(language)) us_state |= LANGLOOP; else
			ttyputmsg(_("Back to Electric"));
		return;
	}

	if (count > 1)
	{
		us_abortcommand(_("Please provide only one parameter to be interpreted"));
		return;
	}

	var = doquerry(par[0], language, VSTRING);
	if (var == NOVARIABLE) ttyputmsg(x_("%s"), par[0]); else
	{
		ttyputmsg(x_("%s => %s"), par[0], describesimplevariable(var));
	}
}

void us_iterate(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG i, j, l, times;
	REGISTER BOOLEAN repeatcommand;
	REGISTER USERCOM *uc;
	REGISTER INTBIG len, total;
	INTBIG cindex, objaddr, objtype;
	VARIABLE *var, fvar;
	CHAR *qual, parnames[30];
	BOOLEAN comvar;
	REGISTER USERCOM *com;
	REGISTER void *infstr;

	/* see if this is the "repeat last command" form */
	repeatcommand = FALSE;
	if (count == 0)
	{
		repeatcommand = TRUE;
		times = 1;
	} else
	{
		l = estrlen(par[0]);
		if (namesamen(par[0], x_("remembered"), l) == 0 && l >= 1)
		{
			if (us_lastcommandcount == 0)
			{
				us_abortcommand(_("No previous command to repeat"));
				return;
			}
			/* repeat the command issued by the "remember" statement */
			uc = us_buildcommand(us_lastcommandcount, us_lastcommandpar);
			if (uc == NOUSERCOM) return;
			us_execute(uc, FALSE, FALSE, FALSE);
			us_freeusercom(uc);
			return;
		}
		if (isanumber(par[0]))
		{
			repeatcommand = TRUE;
			times = eatoi(par[0]);
		}
	}

	if (repeatcommand)
	{
		/* make sure there was a valid previous command */
		if (us_lastcom == NOUSERCOM || us_lastcom->active < 0)
		{
			us_abortcommand(_("No last command to repeat"));
			return;
		}

		/* copy last command into new one */
		uc = us_allocusercom();
		if (uc == NOUSERCOM)
		{
			ttyputnomemory();
			return;
		}
		for(i=0; i<us_lastcom->count; i++)
			if (allocstring(&uc->word[i], us_lastcom->word[i], us_tool->cluster))
		{
			ttyputnomemory();
			return;
		}
		uc->active = us_lastcom->active;
		uc->count = us_lastcom->count;
		(void)allocstring(&uc->comname, us_lastcom->comname, us_tool->cluster);
		uc->menu = us_lastcom->menu;

		/* execute this command */
		for(j=0; j<times; j++) us_execute(uc, TRUE, FALSE, TRUE);
		us_freeusercom(uc);
	} else
	{
		/* implement the iterate over array-variable form */
		if (count < 2)
		{
			ttyputusage(x_("iterate ARRAY-VARIABLE MACRO"));
			return;
		}
		if (us_getvar(par[0], &objaddr, &objtype, &qual, &comvar, &cindex))
		{
			us_abortcommand(_("Incorrect iterator variable name: %s"), par[0]);
			return;
		}
		if (*qual != 0)
		{
			var = getval(objaddr, objtype, -1, qual);
			if (var == NOVARIABLE)
			{
				us_abortcommand(_("Cannot find iterator variable: %s"), par[0]);
				return;
			}
		} else
		{
			fvar.addr = objaddr;
			fvar.type = objtype;
			var = &fvar;
		}
		len = getlength(var);
		if (len < 0) len = 1;
		if (us_expandaddrtypearray(&us_iteratelimit, &us_iterateaddr,
			&us_iteratetype, len)) return;
		if ((var->type&VISARRAY) == 0)
		{
			us_iterateaddr[0] = var->addr;
			us_iteratetype[0] = var->type;
			total = 1;
		} else
		{
			if ((var->type&VTYPE) == VGENERAL)
			{
				for(i=0; i<len; i += 2)
				{
					us_iterateaddr[i/2] = ((INTBIG *)var->addr)[i];
					us_iteratetype[i/2] = ((INTBIG *)var->addr)[i+1];
				}
				total = len / 2;
			} else
			{
				for(i=0; i<len; i++)
				{
					us_iterateaddr[i] = ((INTBIG *)var->addr)[i];
					us_iteratetype[i] = var->type;
				}
				total = len;
			}
		}

		/* now iterate with this value */
		for(i=0; i<total; i++)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, par[1]);
			(void)esnprintf(parnames, 30, x_(" %s 0%lo"), us_variabletypename(us_iteratetype[i]),
				us_iterateaddr[i]);
			addstringtoinfstr(infstr, parnames);
			com = us_makecommand(returninfstr(infstr));
			if (com->active < 0) { us_freeusercom(com);   break; }
			us_execute(com, FALSE, FALSE, FALSE);
			us_freeusercom(com);
			if (stopping(STOPREASONITERATE)) break;
		}
	}
}

void us_killcell(INTBIG count, CHAR *par[])
{
	REGISTER NODEPROTO *np, *onp;
	REGISTER NODEINST *ni;
	REGISTER LIBRARY *lib;
	extern COMCOMP us_showdp;

	if (count == 0)
	{
		count = ttygetparam(M_("Cell name: "), &us_showdp, MAXPARS, par);
		if (count == 0)
		{
			us_abortedmsg();
			return;
		}
	}

	np = getnodeproto(par[0]);
	if (np == NONODEPROTO)
	{
		us_abortcommand(_("No cell called %s"), par[0]);
		return;
	}
	if (np->primindex != 0)
	{
		us_abortcommand(_("Can only kill cells"));
		return;
	}

	/* disallow killing if lock is on */
	if (us_cantedit(np, NONODEINST, TRUE)) return;

	/* if there are still instances of the cell, mention them */
	if (np->firstinst != NONODEINST)
	{
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
				onp->temp1 = 0;
		for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
			ni->parent->temp1++;
		ttyputerr(_("Erase all of the following instances of this cell first:"));
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
				if (onp->temp1 != 0)
					ttyputmsg(_("  %ld instance(s) in cell %s"), onp->temp1, describenodeproto(onp));
		return;
	}

	/* kill the cell */
	us_dokillcell(np);
	ttyputmsg(_("Cell %s:%s deleted"), np->lib->libname, nldescribenodeproto(np));
}
