/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrcomoq.c
 * User interface tool: command handler for O through Q
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
#include "egraphics.h"
#include "sim.h"

void us_offtool(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG permanently;

	if (count == 2 && namesamen(par[1], x_("permanently"), estrlen(par[1])) == 0)
	{
		count--;
		permanently = 1;
	} else permanently = 0;
	us_settool(count, par, permanently);
}

void us_ontool(INTBIG count, CHAR *par[])
{
	us_settool(count, par, 2);
}

void us_outhier(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG len, total, i, j, haveview, vlx, vhx, vly, vhy;
	INTBIG lx, hx, ly, hy, index, viewinfo[22];
	REGISTER NODEPROTO *curnp, *hinp, *np;
	REGISTER BOOLEAN newwindow, found;
	REGISTER NODEINST *ni, *hini, *iconinst;
	REGISTER PORTPROTO *pp, *curpp, *hipp;
	REGISTER PORTEXPINST *pe;
	REGISTER WINDOWPART *w;
	REGISTER VARIABLE *var;
	XARRAY into, outof;
	HIGHLIGHT high;

	/* make sure repeat count is valid */
	if (count == 0) total = 1; else
	{
		total = eatoi(par[0]);
		if (total <= 0)
		{
			us_abortcommand(_("Specify a positive number"));
			return;
		}
	}

	/* must be editing a cell */
	curnp = us_needcell();
	if (curnp == NONODEPROTO) return;

	/* if in waveform window, switch to associated circuit window */
	if ((el_curwindowpart->state&WINDOWTYPE) == WAVEFORMWINDOW)
	{
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
			if (w != el_curwindowpart && w->curnodeproto == curnp) break;
		if (w == NOWINDOWPART)
		{
			us_abortcommand(_("Cannot go up the hierarchy from this waveform window"));
			return;
		}
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)w,
			VWINDOWPART|VDONTSAVE);
	}

	/* initialize desired window extent */
	lx = el_curwindowpart->screenlx;   hx = el_curwindowpart->screenhx;
	ly = el_curwindowpart->screenly;   hy = el_curwindowpart->screenhy;

	/* see if a node with an export is highlighted */
	curpp = NOPORTPROTO;
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var != NOVARIABLE)
	{
		len = getlength(var);
		if (len == 1)
		{
			/* get the highlighted object */
			if (!us_makehighlight(((CHAR **)var->addr)[0], &high))
			{
				pp = NOPORTPROTO;
				if ((high.status&HIGHTYPE) == HIGHTEXT)
				{
					if (high.fromvar == NOVARIABLE && high.fromport != NOPORTPROTO)
						pp = high.fromport->subportproto;
				} else
				{
					if ((high.status&HIGHTYPE) == HIGHFROM &&
						high.fromgeom->entryisnode && high.fromport != NOPORTPROTO)
							pp = high.fromport;
				}
				if (pp != NOPORTPROTO)
				{
					ni = high.fromgeom->entryaddr.ni;
					np = ni->proto;

					/* see if port is an export */
					for (pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
					{
						if (pe->proto == pp)
						{
							curpp = pe->exportproto;
							break;
						}
					}
				}
			}
		}
	}

	us_clearhighlightcount();
	haveview = 0;
	newwindow = FALSE;
	for(i=0; i<total; i++)
	{
		hinp = NONODEPROTO;

		hini = descentparent(curnp, &index, el_curwindowpart, viewinfo);
		if (hini != NONODEINST)
		{
			hinp = hini->parent;
			for(j=0; j<22; j++) if (viewinfo[j] != 0) break;
			if (j >= 22) haveview = 0; else
			{
				haveview = 1;
				vlx         = viewinfo[0];
				vhx         = viewinfo[1];
				vly         = viewinfo[2];
				vhy         = viewinfo[3];
				into[0][0]  = viewinfo[4];
				into[0][1]  = viewinfo[5];
				into[0][2]  = viewinfo[6];
				into[1][0]  = viewinfo[7];
				into[1][1]  = viewinfo[8];
				into[1][2]  = viewinfo[9];
				into[2][0]  = viewinfo[10];
				into[2][1]  = viewinfo[11];
				into[2][2]  = viewinfo[12];
				outof[0][0] = viewinfo[13];
				outof[0][1] = viewinfo[14];
				outof[0][2] = viewinfo[15];
				outof[1][0] = viewinfo[16];
				outof[1][1] = viewinfo[17];
				outof[1][2] = viewinfo[18];
				outof[2][0] = viewinfo[19];
				outof[2][1] = viewinfo[20];
				outof[2][2] = viewinfo[21];
			}
		}

		/* if stack didn't tell where to go, see if upper cell is obvious */
		if (hinp == NONODEPROTO)
		{
			/* see if cell "curnp" has an icon cell */
			np = iconview(curnp);
			if (np == NONODEPROTO) np = curnp;

			/* see if there is any instance of that cell */
			found = FALSE;
			hinp = NONODEPROTO;
			iconinst = NONODEINST;
			for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
			{
				if (isiconof(np, ni->parent))
				{
					iconinst = ni;
					continue;
				}
				if ((ni->parent->lib->userbits&HIDDENLIBRARY) != 0) continue;
				found = TRUE;
				if (hinp != NONODEPROTO)
				{
					if (ni->parent == hinp) continue;
					hinp = NONODEPROTO;
					break;
				}
				hinp = ni->parent;
				hini = ni;
			}
			if (!found && iconinst != NONODEINST)
			{
				hini = iconinst;
				hinp = iconinst->parent;
				found = TRUE;
			}
			if (!found)
			{
				if (i == 0)
				{
					us_abortcommand(_("Not in any cells"));
					return;
				}
				ttyputerr(_("Only in %ld deep"), i);
				hinp = curnp;
				break;
			}
			if (hinp == NONODEPROTO)
			{
				hini = us_pickhigherinstance(np);
				if (hini == NONODEINST) return;
				newwindow = TRUE;
				hinp = hini->parent;
			}
		}

		/* determine which port to highlight */
		hipp = NOPORTPROTO;
		if (hini != NONODEINST && curpp != NOPORTPROTO)
			hipp = equivalentport(curnp, curpp, hini->proto);

		/* adjust view of the cell */
		if (haveview != 0)
		{
			lx = vlx;   hx = vhx;
			ly = vly;   hy = vhy;
		} else
		{
			if (hini != NONODEINST && hini->proto == curnp)
			{
				lx += hini->geom->lowx - curnp->lowx;
				hx += hini->geom->lowx - curnp->lowx;
				ly += hini->geom->lowy - curnp->lowy;
				hy += hini->geom->lowy - curnp->lowy;
			} else
			{
				/* make a full view of the cell */
				us_fullview(hinp, &lx, &hx, &ly, &hy);
			}
		}

		/* keep rolling up the hierarchy */
		curnp = hinp;
		curpp = hipp;
	}

	/* display the final location */
	if (haveview != 0)
	{
		us_setxarray((INTBIG)el_curwindowpart, VWINDOWPART, x_("intocell"), into);
		us_setxarray((INTBIG)el_curwindowpart, VWINDOWPART, x_("outofcell"), outof);
		if (outof[2][0] == 0 && outof[2][1] == 0 && outof[2][2] == 0)
		{
			(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("state"),
				el_curwindowpart->state & ~INPLACEEDIT, VINTEGER);
		} else
		{
			(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("state"),
				el_curwindowpart->state | INPLACEEDIT, VINTEGER);
			(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("inplacedepth"),
				el_curwindowpart->inplacedepth-1, VINTEGER);
		}
	}
	us_switchtocell(hinp, lx, hx, ly, hy, hini, hipp, newwindow, TRUE, TRUE);

	/* if simulating, coordinate this hierarchy traversal with the waveform */
	if (el_curwindowpart != NOWINDOWPART &&
		(el_curwindowpart->state&WINDOWMODE) == WINDOWSIMMODE)
	{
		asktool(sim_tool, "traverse-up", (INTBIG)hinp);
	}
}

void us_package(INTBIG count, CHAR *par[])
{
	INTBIG lx, hx, ly, hy;
	REGISTER INTBIG search, len;
	REGISTER void *infstr;
	REGISTER GEOM *look;
	REGISTER NODEPROTO *np, *parnt;
	REGISTER PORTPROTO *ppt, *npt;
	REGISTER NODEINST *ni, *newni;
	REGISTER ARCINST *ai, *newar;
	REGISTER LIBRARY *lib;
	REGISTER CHAR *pt;
	extern COMCOMP us_packagep;

	/* get the specified area */
	if (us_getareabounds(&lx, &hx, &ly, &hy) == NONODEPROTO)
	{
		us_abortcommand(_("Enclose an area to be packaged"));
		return;
	}

	if (count == 0)
	{
		count = ttygetparam(M_("New cell name: "), &us_packagep, MAXPARS, par);
		if (count == 0)
		{
			us_abortedmsg();
			return;
		}
	}

	/* figure out which library to use */
	lib = el_curlib;
	for(pt = par[0]; *pt != 0; pt++) if (*pt == ':') break;
	if (*pt == ':')
	{
		*pt++ = 0;
		lib = getlibrary(par[0]);
		if (lib == NOLIBRARY)
		{
			us_abortcommand(_("Cannot find library '%s'"), par[0]);
			return;
		}
		par[0] = pt;
	}

	/* make sure there is a proper view type on this new cell */
	parnt = getcurcell();
	len = strlen(par[0]);
	if (par[0][len-1] != '}')
	{
		infstr = initinfstr();
		formatinfstr(infstr, "%s{%s}", par[0], parnt->cellview->sviewname);
		par[0] = returninfstr(infstr);
	}

	/* create the new cell */
	np = us_newnodeproto(par[0], lib);
	if (np == NONODEPROTO)
	{
		us_abortcommand(_("Cannot create cell %s"), par[0]);
		return;
	}

	/* first zap all nodes touching all arcs in the region */
	search = initsearch(lx, hx, ly, hy, parnt);
	for(;;)
	{
		look = nextobject(search);
		if (look == NOGEOM) break;
		if (look->entryisnode) continue;
		ai = look->entryaddr.ai;
		ni = NONODEINST;
		ai->end[0].nodeinst->temp1 = (UINTBIG)ni;
		ai->end[1].nodeinst->temp1 = (UINTBIG)ni;
	}

	/* copy the nodes into the new cell */
	search = initsearch(lx, hx, ly, hy, parnt);
	for(;;)
	{
		look = nextobject(search);
		if (look == NOGEOM) break;
		if (!look->entryisnode) continue;
		ni = look->entryaddr.ni;
		newni = newnodeinst(ni->proto, ni->lowx, ni->highx, ni->lowy,
			ni->highy, ni->transpose, ni->rotation, np);
		if (newni == NONODEINST)
		{
			us_abortcommand(_("Cannot create node in new cell"));
			return;
		}
		ni->temp1 = (INTBIG)newni;
		newni->userbits = ni->userbits;
		TDCOPY(newni->textdescript, ni->textdescript);
		if (copyvars((INTBIG)ni, VNODEINST, (INTBIG)newni, VNODEINST, FALSE))
		{
			ttyputnomemory();
			return;
		}

		/* make ports where this nodeinst has them */
		if (ni->firstportexpinst != NOPORTEXPINST)
			for(ppt = parnt->firstportproto; ppt != NOPORTPROTO; ppt = ppt->nextportproto)
		{
			if (ppt->subnodeinst != ni) continue;
			npt = newportproto(np, newni, ppt->subportproto, ppt->protoname);
			if (npt != NOPORTPROTO)
			{
				npt->userbits = (npt->userbits & ~STATEBITS) | (ppt->userbits & STATEBITS);
				TDCOPY(npt->textdescript, ppt->textdescript);
				if (copyvars((INTBIG)ppt, VPORTPROTO, (INTBIG)npt, VPORTPROTO, FALSE))
				{
					ttyputnomemory();
					return;
				}
			}
		}
	}

	/* copy the arcs into the new cell */
	search = initsearch(lx, hx, ly, hy, parnt);
	for(;;)
	{
		look = nextobject(search);
		if (look == NOGEOM) break;
		if (look->entryisnode) continue;
		ai = look->entryaddr.ai;
		if (ai->end[0].nodeinst->temp1 == (UINTBIG)-1 ||
			ai->end[1].nodeinst->temp1 == (UINTBIG)-1) continue;
		newar = newarcinst(ai->proto, ai->width, ai->userbits, (NODEINST *)ai->end[0].nodeinst->temp1,
			ai->end[0].portarcinst->proto, ai->end[0].xpos,ai->end[0].ypos,
				(NODEINST *)ai->end[1].nodeinst->temp1, ai->end[1].portarcinst->proto,
					ai->end[1].xpos,ai->end[1].ypos, np);
		if (newar == NOARCINST)
		{
			us_abortcommand(_("Cannot create arc in new cell"));
			return;
		}
		if (copyvars((INTBIG)ai, VARCINST, (INTBIG)newar, VARCINST, FALSE) ||
			copyvars((INTBIG)ai->end[0].portarcinst, VPORTARCINST,
				(INTBIG)newar->end[0].portarcinst, VPORTARCINST, FALSE) ||
			copyvars((INTBIG)ai->end[1].portarcinst, VPORTARCINST,
				(INTBIG)newar->end[1].portarcinst, VPORTARCINST, FALSE))
		{
			ttyputnomemory();
			return;
		}
	}

	/* recompute bounds of new cell */
	(*el_curconstraint->solve)(np);

	ttyputmsg(_("Cell %s created"), describenodeproto(np));
}

void us_port(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG i, l, labels, total, reducehighlighting;
	INTBIG lx, hx, ly, hy, mlx, mly, mhx, mhy, bits, mask, newbits, x, y, deleted;
	REGISTER BOOLEAN nodegood;
	REGISTER NODEINST *ni, *oni;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp, *ppt;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe, *nextpe;
	REGISTER LIBRARY *olib;
	GEOM *fromgeom, *togeom;
	PORTPROTO *fromport, *toport;
	REGISTER CHAR *pt;
	CHAR *portname, *refname;
	NODEINST **nilist;
	REGISTER HIGHLIGHT *high;
	HIGHLIGHT newhigh;
	extern COMCOMP us_portp, us_portep;
	REGISTER GEOM **g;
	static POLYGON *poly = NOPOLYGON;
	REGISTER void *infstr;

	/* get options */
	if (count == 0)
	{
		count = ttygetparam(M_("Port option: "), &us_portp, MAXPARS, par);
		if (count == 0)
		{
			us_abortedmsg();
			return;
		}
	}
	l = estrlen(pt = par[0]);

	if (namesamen(pt, x_("labels"), l) == 0 && l >= 1)
	{
		if (count > 1)
		{
			l = estrlen(pt = par[1]);
			if (namesamen(pt, x_("crosses"), l) == 0 && l >= 1) labels = PORTSCROSS; else
			if (namesamen(pt, x_("short"), l) == 0 && l >= 2) labels = PORTSSHORT; else
			if (namesamen(pt, x_("long"), l) == 0 && l >= 1) labels = PORTSFULL; else
			{
				ttyputusage(x_("port labels short|long|crosses"));
				return;
			}
			startobjectchange((INTBIG)us_tool, VTOOL);
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey,
				(us_useroptions & ~PORTLABELS) | labels, VINTEGER);
			endobjectchange((INTBIG)us_tool, VTOOL);
		}
		switch (us_useroptions&PORTLABELS)
		{
			case PORTSCROSS: ttyputverbose(M_("Ports shown as crosses"));     break;
			case PORTSFULL:  ttyputverbose(M_("Long port names displayed"));  break;
			case PORTSSHORT: ttyputverbose(M_("Short port names displayed")); break;
		}
		return;
	}

	if (namesamen(pt, x_("export-labels"), l) == 0 && l >= 8)
	{
		if (count > 1)
		{
			l = estrlen(pt = par[1]);
			if (namesamen(pt, x_("crosses"), l) == 0 && l >= 1) labels = EXPORTSCROSS; else
			if (namesamen(pt, x_("short"), l) == 0 && l >= 2) labels = EXPORTSSHORT; else
			if (namesamen(pt, x_("long"), l) == 0 && l >= 1) labels = EXPORTSFULL; else
			{
				ttyputusage(x_("port export-labels short|long|crosses"));
				return;
			}
			startobjectchange((INTBIG)us_tool, VTOOL);
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey,
				(us_useroptions & ~EXPORTLABELS) | labels, VINTEGER);
			endobjectchange((INTBIG)us_tool, VTOOL);
		}
		switch (us_useroptions&EXPORTLABELS)
		{
			case EXPORTSCROSS: ttyputverbose(M_("Exports shown as crosses"));     break;
			case EXPORTSFULL:  ttyputverbose(M_("Long export names displayed"));  break;
			case EXPORTSSHORT: ttyputverbose(M_("Short export names displayed")); break;
		}
		return;
	}

	if (namesamen(pt, x_("synchronize-library"), l) == 0 && l >= 1)
	{
		if (count <= 1)
		{
			ttyputusage(x_("port synchronize-library LIBRARY"));
			return;
		}
		olib = getlibrary(par[1]);
		if (olib == NOLIBRARY)
		{
			us_abortcommand(_("No library called %s is read in"), par[1]);
			return;
		}
		if (olib == el_curlib)
		{
			us_abortcommand(_("Must synchronize with a different library"));
			return;
		}
		us_portsynchronize(olib);
		return;
	}

	if (namesamen(pt, x_("identify-cell"), l) == 0 && l >= 10)
	{
		/* make sure there is a current cell */
		np = us_needcell();
		if (np == NONODEPROTO) return;
		if ((el_curwindowpart->state&WINDOWTYPE) == DISP3DWINDOW)
		{
			us_abortcommand(_("Cannot show exports in a 3D window"));
			return;
		}
		us_identifyports(0, 0, np, LAYERA, FALSE);
		return;
	}

	if (namesamen(pt, x_("identify-node"), l) == 0 && l >= 10)
	{
		/* make sure there is a current node */
		if ((el_curwindowpart->state&WINDOWTYPE) == DISP3DWINDOW)
		{
			us_abortcommand(_("Cannot show ports in a 3D window"));
			return;
		}
		g = us_gethighlighted(WANTNODEINST, 0, 0);
		for(i=0; g[i] != NOGEOM; i++) ;
		if (i <= 0)
		{
			us_abortcommand(_("Must select one or more nodes"));
			return;
		}
		nilist = (NODEINST **)emalloc(i * (sizeof (NODEINST *)), el_tempcluster);
		if (nilist == 0) return;
		for(i=0; g[i] != NOGEOM; i++)
			nilist[i] = g[i]->entryaddr.ni;
		us_identifyports(i, nilist, nilist[0]->proto, LAYERA, FALSE);
		efree((CHAR *)nilist);
		return;
	}

	if (namesamen(pt, x_("move"), l) == 0 && l >= 1)
	{
		if (us_gettwoobjects(&fromgeom, &fromport, &togeom, &toport))
		{
			us_abortcommand(_("Must select two nodes"));
			return;
		}
		if (fromport == NOPORTPROTO || toport == NOPORTPROTO)
		{
			us_abortcommand(_("Must select two nodes AND their ports"));
			return;
		}

		/* make sure ports are in the same cell */
		np = geomparent(fromgeom);
		if (np != geomparent(togeom))
		{
			us_abortcommand(_("Port motion must be within a single cell"));
			return;
		}

		/* disallow port action if lock is on */
		if (us_cantedit(np, NONODEINST, TRUE)) return;

		reducehighlighting = 1;
		if (count > 1)
		{
			l = estrlen(pt = par[1]);
			if (namesamen(pt, x_("remain-highlighted"), l) == 0) reducehighlighting = 0;
		}

		/* get the "source" node and export */
		oni = fromgeom->entryaddr.ni;
		for(pe = oni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
			if (pe->proto == fromport) break;
		if (pe == NOPORTEXPINST)
		{
			us_abortcommand(_("Port %s of node %s is not an export"), fromport->protoname,
				describenodeinst(oni));
			return;
		}

		/* move the port */
		ni = togeom->entryaddr.ni;
		if (ni == oni && fromport == toport)
		{
			us_abortcommand(_("This does not move the port"));
			return;
		}

		/* clear highlighting */
		if (reducehighlighting == 0) us_pushhighlight();
		us_clearhighlightcount();

		np = ni->parent;
		pt = pe->exportproto->protoname;
		startobjectchange((INTBIG)ni, VNODEINST);
		startobjectchange((INTBIG)oni, VNODEINST);
		if (moveportproto(np, pe->exportproto, ni, toport)) ttyputerr(_("Port not moved")); else
		{
			endobjectchange((INTBIG)ni, VNODEINST);
			endobjectchange((INTBIG)oni, VNODEINST);
			ttyputverbose(M_("Port %s moved from node %s to node %s"), pt, describenodeinst(oni),
				describenodeinst(ni));
		}

		/* restore highlighting */
		if (reducehighlighting == 0) us_pophighlight(FALSE); else
		{
			newhigh.status = HIGHFROM;
			newhigh.cell = np;
			newhigh.fromgeom = fromgeom;
			newhigh.fromport = fromport;
			us_addhighlight(&newhigh);
		}
		return;
	}

	if (namesamen(pt, x_("re-export-all"), l) == 0 && l >= 1)
	{
		/* make sure there is a current cell */
		np = us_needcell();
		if (np == NONODEPROTO) return;

		/* disallow port action if lock is on */
		if (us_cantedit(np, NONODEINST, TRUE)) return;

		/* save highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* look at every node in this cell */
		total = 0;
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			/* only look for cells, not primitives */
			if (ni->proto->primindex != 0) continue;

			/* ignore recursive references (showing icon in contents) */
			if (isiconof(ni->proto, np)) continue;

			/* clear marks on the ports of this node */
			for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				pp->temp1 = 0;

			/* mark the connected and exports */
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				pi->proto->temp1++;
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
				pe->proto->temp1++;

			/* initialize for queueing creation of new exports */
			us_initqueuedexports();

			/* now export the remaining ports */
			nodegood = FALSE;
			for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				if (pp->temp1 == 0)
			{
				if (!nodegood)
				{
					startobjectchange((INTBIG)ni, VNODEINST);
					nodegood = TRUE;
				}
				if (us_queuenewexport(ni, pp, pp))
				{
					us_pophighlight(FALSE);
					return;
				}
				total++;
			}
			if (nodegood)
			{
				/* create any queued exports */
				us_createqueuedexports();
				endobjectchange((INTBIG)ni, VNODEINST);
			}
		}
		if (total == 0) ttyputmsg(_("No ports to export")); else
			ttyputmsg(_("%ld ports exported"), total);

		/* restore highlighting */
		us_pophighlight(FALSE);
		return;
	}

	if (namesamen(pt, x_("highlighted-re-export"), l) == 0 && l >= 1)
	{
		/* get the necessary polygon */
		(void)needstaticpolygon(&poly, 4, us_tool->cluster);

		/* get the bounds of the selected area */
		np = us_getareabounds(&lx, &hx, &ly, &hy);
		if (np == NONODEPROTO)
		{
			us_abortcommand(_("Must select an area enclosing ports to export"));
			return;
		}

		/* disallow port action if lock is on */
		if (us_cantedit(np, NONODEINST, TRUE)) return;

		/* get list of nodes in the selected area */
		g = us_gethighlighted(WANTNODEINST, 0, 0);
		if (g[0] == NOGEOM)
		{
			us_abortcommand(_("Must select some nodes for making exports"));
			return;
		}

		/* save highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* initialize for queueing creation of new exports */
		us_initqueuedexports();

		/* re-export the requested ports */
		total = 0;
		for(i=0; g[i] != NOGEOM; i++)
		{
			ni = g[i]->entryaddr.ni;

			/* only look for cells, not primitives */
			if (ni->proto->primindex != 0) continue;

			/* clear marks on the ports of this node */
			for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				pp->temp1 = 0;

			/* mark the connected and make exports */
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				pi->proto->temp1++;
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
				pe->proto->temp1++;

			/* now export the remaining ports that are in the area */
			nodegood = FALSE;
			for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				if (pp->temp1 == 0)
			{
				shapeportpoly(ni, pp, poly, FALSE);
				getbbox(poly, &mlx, &mhx, &mly, &mhy);
				if (mlx > hx || mhx < lx || mly > hy || mhy < ly) continue;
				if (!nodegood)
				{
					startobjectchange((INTBIG)ni, VNODEINST);
					nodegood = TRUE;
				}
				if (us_queuenewexport(ni, pp, pp))
				{
					us_pophighlight(FALSE);
					return;
				}
				total++;
			}
		}

		/* create any queued exports */
		us_createqueuedexports();
		for(i=0; g[i] != NOGEOM; i++)
		{
			ni = g[i]->entryaddr.ni;
			if (ni->proto->primindex != 0) continue;
			for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			{
				if (pp->temp1 != 0) continue;
				endobjectchange((INTBIG)ni, VNODEINST);
				break;
			}
		}
		if (total == 0) ttyputmsg(_("No ports to export")); else
			ttyputmsg(_("%ld ports exported"), total);

		/* restore highlighting */
		us_pophighlight(FALSE);
		return;
	}

	if (namesamen(pt, x_("power-ground-re-export"), l) == 0 && l >= 1)
	{
		/* make sure there is a current cell */
		np = us_needcell();
		if (np == NONODEPROTO) return;

		/* disallow port action if lock is on */
		if (us_cantedit(np, NONODEINST, TRUE)) return;

		/* save highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* look at every node in this cell */
		total = 0;
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			/* only look for cells, not primitives */
			if (ni->proto->primindex != 0) continue;

			/* ignore recursive references (showing icon in contents) */
			if (isiconof(ni->proto, np)) continue;

			/* clear marks on the ports of this node */
			for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				pp->temp1 = 0;

			/* mark the connected and exports */
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				pi->proto->temp1++;
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
				pe->proto->temp1++;

			/* now export the remaining power and ground ports */
			for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			{
				if (pp->temp1 != 0) continue;
				if (!portispower(pp) && !portisground(pp)) continue;

				if (us_reexportport(pp, ni))
				{
					us_pophighlight(FALSE);
					return;
				}
				total++;
			}
		}
		if (total == 0) ttyputmsg(_("No power and ground ports to export")); else
			ttyputmsg(_("%ld power and ground ports exported"), total);

		/* restore highlighting */
		us_pophighlight(FALSE);
		return;
	}

	if (namesamen(pt, x_("not"), l) == 0 && l >= 1)
	{
		if (count <= 1 || namesamen(par[1], x_("export"), estrlen(par[1])) != 0)
		{
			ttyputusage(x_("port not export [OPTIONS]"));
			return;
		}
		if (count == 3 && namesame(par[2], x_("geometry")) == 0)
		{
			/* delete ports in selected area */
			np = us_getareabounds(&lx, &hx, &ly, &hy);
			if (np == NONODEPROTO)
			{
				us_abortcommand(_("Outline an area first"));
				return;
			}

			/* get the necessary polygon */
			(void)needstaticpolygon(&poly, 4, us_tool->cluster);

			/* remove highlighting */
			us_pushhighlight();
			us_clearhighlightcount();

			deleted = 0;
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				i = 0;
				for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = nextpe)
				{
					nextpe = pe->nextportexpinst;
					shapeportpoly(ni, pe->proto, poly, FALSE);
					getcenter(poly, &x, &y);
					if (x < lx || x > hx || y < ly || y > hy) continue;
					ttyputmsg(_("Deleting port %s"), pe->exportproto->protoname);
					if (i == 0) startobjectchange((INTBIG)ni, VNODEINST);
					if (killportproto(np, pe->exportproto))
						ttyputerr(_("killportproto error"));
					i = 1;
					deleted++;
				}
				if (i != 0) endobjectchange((INTBIG)ni, VNODEINST);
			}

			/* restore highlighting */
			us_pophighlight(FALSE);
			if (deleted == 0) ttyputmsg(_("No exports are in the selected area"));
			return;
		}
		if (count == 3 && namesame(par[2], x_("all")) == 0)
		{
			g = us_gethighlighted(WANTNODEINST, 0, 0);
			if (g[0] == NOGEOM)
			{
				us_abortcommand(_("Must first select nodes with exports"));
				return;
			}

			/* remove highlighting */
			us_pushhighlight();
			us_clearhighlightcount();

			/* delete ports from all selected node */
			deleted = 0;
			for(i=0; g[i] != NOGEOM; i++)
			{
				ni = g[i]->entryaddr.ni;
				if (ni->firstportexpinst == NOPORTEXPINST) continue;

				/* remove the port(s) */
				startobjectchange((INTBIG)ni, VNODEINST);
				for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = nextpe)
				{
					nextpe = pe->nextportexpinst;
					ttyputmsg(_("Deleting export %s"), pe->exportproto->protoname);
					if (killportproto(ni->parent, pe->exportproto))
						ttyputerr(_("killportproto error"));
					deleted++;
				}
				endobjectchange((INTBIG)ni, VNODEINST);
			}

			/* restore highlighting */
			us_pophighlight(FALSE);
			if (deleted == 0) ttyputmsg(_("No exports are on the selected nodes"));
			return;
		}

		/* delete only the selected port */
		high = us_getonehighlight();
		if (high == NOHIGHLIGHT) return;
		if ((high->status&HIGHTYPE) == HIGHTEXT)
		{
			if (high->fromvar != NOVARIABLE || high->fromport == NOPORTPROTO)
			{
				us_abortcommand(_("Selected text must be a port name"));
				return;
			}
			ni = high->fromgeom->entryaddr.ni;
			ppt = high->fromport;
		} else
		{
			ni = (NODEINST *)us_getobject(VNODEINST, FALSE);
			if (ni == NONODEINST) return;
			ppt = NOPORTPROTO;
		}

		/* first see if there is a port on this nodeinst */
		if (ni->firstportexpinst == NOPORTEXPINST)
		{
			us_abortcommand(_("No exports on this node"));
			return;
		}

		/* disallow port action if lock is on */
		if (us_cantedit(ni->parent, NONODEINST, TRUE)) return;

		/* get the specification details */
		pp = us_portdetails(ppt, count-2, &par[2], ni, &bits, &mask, TRUE, x_(""), &refname);
		if (pp == NOPORTPROTO) return;

		/* remove highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* remove the port(s) */
		startobjectchange((INTBIG)ni, VNODEINST);
		us_undoportproto(ni, pp);
		endobjectchange((INTBIG)ni, VNODEINST);

		/* restore highlighting */
		us_pophighlight(FALSE);

		ttyputverbose(M_("Port %s deleted"), pp->protoname);
		return;
	}

	if (namesamen(pt, x_("export"), l) == 0 && l >= 1)
	{
		ni = (NODEINST *)us_getobject(VNODEINST, FALSE);
		if (ni == NONODEINST) return;

		/* disallow port action if lock is on */
		if (us_cantedit(ni->parent, NONODEINST, TRUE)) return;

		if (count <= 1)
		{
			count = ttygetparam(M_("Port name: "), &us_portep, MAXPARS-1, &par[1])+1;
			if (count <= 1)
			{
				us_abortedmsg();
				return;
			}
		}

		/* check name for validity */
		pt = par[1];
		while (*pt == ' ') pt++;
		if (*pt == 0)
		{
			us_abortcommand(_("Null name"));
			return;
		}

		/* find a unique port name */
		portname = us_uniqueportname(pt, ni->parent);
		if (namesame(portname, pt) != 0)
			ttyputmsg(_("Already a port called %s, calling this %s"), pt, portname);

		/* get the specification details */
		pp = us_portdetails(NOPORTPROTO, count-2, &par[2], ni, &bits, &mask, FALSE, pt, &refname);
		if (pp == NOPORTPROTO) return;

		/* save highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* make the port */
		pp = us_makenewportproto(ni->parent, ni, pp, portname, mask, bits, pp->textdescript);
		if (*refname != 0)
			setval((INTBIG)pp, VPORTPROTO, x_("EXPORT_reference_name"), (INTBIG)refname, VSTRING);

		/* restore highlighting */
		us_pophighlight(FALSE);

		ttyputverbose(M_("Port %s created"), pp->protoname);
		return;
	}

	if (namesamen(pt, x_("change"), l) == 0 && l >= 1)
	{
		high = us_getonehighlight();
		if (high == NOHIGHLIGHT) return;
		if ((high->status&HIGHTYPE) == HIGHTEXT)
		{
			if (high->fromvar != NOVARIABLE || high->fromport == NOPORTPROTO)
			{
				us_abortcommand(_("Selected text must be a port name"));
				return;
			}
			ni = high->fromgeom->entryaddr.ni;
			ppt = high->fromport;
		} else
		{
			ni = (NODEINST *)us_getobject(VNODEINST, FALSE);
			if (ni == NONODEINST) return;
			ppt = NOPORTPROTO;
		}

		/* disallow port action if lock is on */
		if (us_cantedit(ni->parent, NONODEINST, TRUE)) return;

		/* get the specification details */
		pp = us_portdetails(ppt, count-1, &par[1], ni, &bits, &mask, TRUE, x_(""), &refname);
		if (pp == NOPORTPROTO) return;

		/* change the port characteristics */
		newbits = pp->userbits;
		if ((mask&STATEBITS) != 0)
			newbits = (newbits & ~STATEBITS) | (bits & STATEBITS);
		if ((mask&PORTDRAWN) != 0)
			newbits = (newbits & ~PORTDRAWN) | (bits & PORTDRAWN);
		if ((mask&BODYONLY) != 0)
			newbits = (newbits & ~BODYONLY) | (bits & BODYONLY);
		setval((INTBIG)pp, VPORTPROTO, x_("userbits"), newbits, VINTEGER);

		/* propagate the change information up the hierarchy */
		changeallports(pp);

		infstr = initinfstr();
		formatinfstr(infstr, _("Port '%s'"), pp->protoname);
		if ((pp->userbits&STATEBITS) == 0)
			addstringtoinfstr(infstr, _(" has no characteristics")); else
		{
			formatinfstr(infstr, _(" is %s"), describeportbits(pp->userbits));
		}
		formatinfstr(infstr, _(" (label %s"), us_describetextdescript(pp->textdescript));
		if ((pp->userbits&PORTDRAWN) != 0) addstringtoinfstr(infstr, _(",always-drawn"));
		if ((pp->userbits&BODYONLY) != 0) addstringtoinfstr(infstr, _(",only-on-body"));
		addstringtoinfstr(infstr, x_(") "));
		ttyputmsg(x_("%s"), returninfstr(infstr));
		return;
	}

	ttyputbadusage(x_("port"));
}

void us_quit(INTBIG count, CHAR *par[])
{
	if (us_preventloss(NOLIBRARY, 0, TRUE)) return;
	bringdown();
}
