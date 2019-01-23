/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrcomab.c
 * User interface tool: command handler for A through B
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
#include "drc.h"
#include "usrtrack.h"

void us_arc(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG negate, toggle, r, j, i, l, interactive, lambda, wid;
	INTBIG ix, iy, xcur, ycur, dx[2], dy[2];
	REGISTER ARCINST *ai;
	REGISTER NODEPROTO *np;
	REGISTER CHAR *pp;
	REGISTER NETWORK *net;
	extern COMCOMP us_arcp, us_arcnotp, us_arcnamep, us_arctogp;
	REGISTER VARIABLE *var;
	REGISTER GEOM **list;
	static POLYGON *poly = NOPOLYGON;

	if (count == 0)
	{
		count = ttygetparam(M_("ARC command: "), &us_arcp, MAXPARS, par);
		if (count == 0)
		{
			us_abortedmsg();
			return;
		}
	}
	l = estrlen(pp = par[0]);
	negate = toggle = 0;

	if (namesamen(pp, x_("not"), l) == 0 && l >= 2)
	{
		if (count == 1)
		{
			count = ttygetparam(M_("Negation option: "), &us_arcnotp, MAXPARS-1, &par[1]) + 1;
			if (count == 1)
			{
				us_abortedmsg();
				return;
			}
		}
		count--;
		par++;
		l = estrlen(pp = par[0]);
		negate = 1;
	} else if (namesamen(pp, x_("toggle"), l) == 0 && l >= 2)
	{
		if (count == 1)
		{
			count = ttygetparam(M_("Toggle option: "), &us_arctogp, MAXPARS-1, &par[1]) + 1;
			if (count == 1)
			{
				us_abortedmsg();
				return;
			}
		}
		count--;
		par++;
		l = estrlen(pp = par[0]);
		toggle = 1;
	}

	/* get the list of all highlighted arcs */
	list = us_gethighlighted(WANTARCINST, 0, 0);
	if (list[0] == NOGEOM)
	{
		us_abortcommand(_("No arcs are selected"));
		return;
	}

	/* disallow arraying if lock is on */
	np = geomparent(list[0]);
	if (us_cantedit(np, NONODEINST, TRUE)) return;

	/* save and erase highlighting */
	us_pushhighlight();
	us_clearhighlightcount();

	if (namesamen(pp, x_("rigid"), l) == 0 && l >= 2)
	{
		if (namesame(el_curconstraint->conname, x_("layout")) != 0)
		{
			us_abortcommand(_("Use the layout constraint system for rigid arcs"));
			us_pophighlight(FALSE);
			return;
		}
		i = us_modarcbits((toggle != 0 ? 23 : (negate != 0 ? 1 : 0)), FALSE, x_(""), list);
		if (i >= 0) ttyputverbose(x_("%ld %s %s"), i, makeplural(_("arc"), i),
			(toggle != 0 ? _("toggle rigidity") : (negate != 0 ? _("made un-rigid") : _("made rigid"))));
	} else if ((namesamen(pp, x_("manhattan"), l) == 0 ||
		namesamen(pp, x_("fixed-angle"), l) == 0) && l >= 1)
	{
		if (namesame(el_curconstraint->conname, x_("layout")) != 0)
		{
			us_abortcommand(_("Use the layout constraint system for fixed-angle arcs"));
			us_pophighlight(FALSE);
			return;
		}
		if (namesamen(pp, x_("manhattan"), l) == 0)
			ttyputmsg(_("It is now proper to use 'arc fixed-angle' instead of 'arc manhattan'"));
		i = us_modarcbits((toggle != 0 ? 24 : (negate != 0 ? 3 : 2)), FALSE, x_(""), list);
		if (i >= 0)
			ttyputverbose(x_("%ld %s %s"), i, makeplural(_("arc"), i), (toggle != 0 ? _("toggle fixed-angle") :
				(negate != 0 ? _("made not fixed-angle") : _("made fixed-angle"))));
	} else if (namesamen(pp, x_("slide"), l) == 0 && l >= 2)
	{
		if (namesame(el_curconstraint->conname, x_("layout")) != 0)
		{
			us_abortcommand(_("Use the layout constraint system for slidable arcs"));
			us_pophighlight(FALSE);
			return;
		}
		i = us_modarcbits((toggle != 0 ? 25 : (negate != 0 ? 5 : 4)), FALSE, x_(""), list);
		if (i >= 0)
			ttyputverbose(x_("%ld %s %s"), i, makeplural(_("arc"), i), (toggle != 0 ? _("toggle slidability") :
				(negate != 0 ? _("can not slide in ports") : _("can slide in ports"))));
	} else if (namesamen(pp, x_("temp-rigid"), l) == 0 && l >= 1)
	{
		if (namesame(el_curconstraint->conname, x_("layout")) != 0)
		{
			us_abortcommand(_("Use the layout constraint system for rigid arcs"));
			us_pophighlight(FALSE);
			return;
		}
		i = us_modarcbits((negate != 0 ? 7 : 6), FALSE, x_(""), list);
		if (i >= 0) ttyputverbose(M_("%ld %s made temporarily %srigid"), i, makeplural(_("arc"), i),
			(negate!=0 ? _("un-") : x_("")));
	} else if (namesamen(pp, x_("ends-extend"), l) == 0 && l >= 1)
	{
		i = us_modarcbits((toggle != 0 ? 26 : (negate != 0 ? 9 : 8)), TRUE, x_(""), list);
		if (i >= 0) ttyputverbose(x_("%ld %s %s"), i, makeplural(_("arc"), i),
			(toggle != 0 ? _("toggle end extension") :
				(negate != 0 ? _("given no end extension") : _("given full end extension"))));
	} else if (namesamen(pp, x_("directional"), l) == 0 && l >= 1)
	{
		i = us_modarcbits((toggle != 0 ? 27 : (negate != 0 ? 11: 10)), TRUE, x_(""), list);
		if (i >= 0) ttyputverbose(x_("%ld %s %s"), i, makeplural(_("arc"), i),
			(toggle != 0 ? _("toggle directionality") :
				(negate != 0 ? _("given no direction") : _("given direction"))));
	} else if (namesamen(pp, x_("negated"), l) == 0 && l >= 2)
	{
		i = us_modarcbits((toggle != 0 ? 28 : (negate != 0 ? 13 : 12)), TRUE, x_(""), list);
		if (i >= 0) ttyputverbose(x_("%ld %s %s"), i, makeplural(_("arc"), i),
			(toggle != 0 ? _("toggle negation") : (negate != 0 ? _("not negated") : _("negated"))));
	} else if (namesamen(pp, x_("skip-tail"), l) == 0 && l >= 6)
	{
		i = us_modarcbits((toggle != 0 ? 29 : (negate != 0 ? 15 : 14)), TRUE, x_(""), list);
		if (i >= 0) ttyputverbose(x_("%ld %s %s"), i, makeplural(_("arc"), i),
			(toggle != 0 ? _("toggle tail use") : (negate!=0 ? _("use tail") : _("do not use tail"))));
	} else if (namesamen(pp, x_("skip-head"), l) == 0 && l >= 6)
	{
		i = us_modarcbits((toggle != 0 ? 30 : (negate != 0 ? 17 : 16)), TRUE, x_(""), list);
		if (i >= 0) ttyputverbose(x_("%ld %s %s"), i, makeplural(_("arc"), i),
			(toggle != 0 ? _("toggle head use") : (negate!=0 ? _("use head") : _("do not use head"))));
	} else if (namesamen(pp, x_("reverse"), l) == 0 && l >= 2)
	{
		if (negate != 0) ttyputerr(_("'arc reverse' is a relative notion, 'not' ignored"));
		i = us_modarcbits(18, TRUE, x_(""), list);
		if (i >= 0) ttyputverbose(M_("%ld %s direction reversed"), i, makeplural(_("arc"), i));
	} else if (namesamen(pp, x_("constraint"), l) == 0 && l >= 2)
	{
		if (namesame(el_curconstraint->conname, x_("linear")) != 0)
		{
			us_abortcommand(_("First switch to the linear inequalities constraint system"));
			us_pophighlight(FALSE);
			return;
		}
		if (count <= 1)
		{
			if (negate != 0) (void)us_modarcbits(19, TRUE, x_(""), list); else
				(void)us_modarcbits(20, FALSE, x_(""), list);
		} else
		{
			if (count > 2 && namesamen(par[2], x_("add"), estrlen(par[2])) == 0)
				i = us_modarcbits(22, TRUE, par[1], list); else
					i = us_modarcbits(21, TRUE, par[1], list);
		}
	} else if (namesamen(pp, x_("name"), l) == 0 && l >= 2)
	{
		if (list[0]->entryisnode || list[1] != NOGEOM)
		{
			us_abortcommand(_("Select only one arc when naming"));
			us_pophighlight(FALSE);
			return;
		}
		ai = list[0]->entryaddr.ai;

		if (negate == 0)
		{
			if (count < 2)
			{
				count = ttygetparam(M_("ARC name: "), &us_arcnamep, MAXPARS-1, &par[1]) + 1;
				if (count == 1)
				{
					us_abortedmsg();
					us_pophighlight(FALSE);
					return;
				}
			}
			pp = par[1];

			/* get the former name on this arc */
			net = getnetwork(pp, ai->parent);
			if (net != NONETWORK)
			{
				if (ai->network == net)
					ttyputmsg(_("Network %s is already on this arc"), describenetwork(net));
			} else ttyputverbose(M_("Network %s defined"), pp);
			us_setarcname(ai, pp);
		} else
		{
			var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
			if (var == NOVARIABLE)
			{
				us_abortcommand(_("This arc has no name"));
				us_pophighlight(FALSE);
				return;
			}
			if (var != NOVARIABLE) us_setarcname(ai, (CHAR *)0);
			ttyputverbose(M_("Arc name removed"));
		}
	} else if (namesamen(pp, x_("center"), l) == 0 && l >= 2)
	{
		if (list[0]->entryisnode || list[1] != NOGEOM)
		{
			us_abortcommand(_("Select only one arc when changing curvature"));
			us_pophighlight(FALSE);
			return;
		}
		ai = list[0]->entryaddr.ai;

		/* make sure the arc can handle it */
		if ((ai->proto->userbits&CANCURVE) == 0 && negate == 0)
		{
			us_abortcommand(_("This arc cannot curve"));
			us_pophighlight(FALSE);
			return;
		}

		/* see if curvature should be set interactively */
		interactive = 0;
		if (count >= 2 && namesamen(par[1], x_("interactive"), estrlen(par[1])) == 0)
			interactive = 1;

		/* change the arc center information */
		if (negate == 0)
		{
			if (interactive != 0)
			{
				us_arccurveinit(ai);
				trackcursor(TRUE, us_ignoreup, us_nullvoid, us_arccenterdown, us_stoponchar,
					us_nullvoid, TRACKDRAGGING);
			} else
			{
				if (us_demandxy(&xcur, &ycur))
				{
					us_pophighlight(FALSE);
					return;
				}

				/* get radius that lets arc curve about this point */
				r = us_curvearcaboutpoint(ai, xcur, ycur);

				startobjectchange((INTBIG)ai, VARCINST);
				(void)setvalkey((INTBIG)ai, VARCINST, el_arc_radius_key, r, VINTEGER);
				(void)modifyarcinst(ai, 0, 0, 0, 0, 0);
				endobjectchange((INTBIG)ai, VARCINST);
			}
		} else
		{
			startobjectchange((INTBIG)ai, VARCINST);
			(void)delvalkey((INTBIG)ai, VARCINST, el_arc_radius_key);
			(void)modifyarcinst(ai, 0, 0, 0, 0, 0);
			endobjectchange((INTBIG)ai, VARCINST);
		}
	} else if (namesamen(pp, x_("shorten"), l) == 0 && l >= 2)
	{
		/* get the necessary polygon */
		(void)needstaticpolygon(&poly, 4, us_tool->cluster);

		l = 0;
		for(i=0; list[i] != NOGEOM; i++)
		{
			ai = list[i]->entryaddr.ai;
			for(j=0; j<2; j++)
			{
				shapeportpoly(ai->end[j].nodeinst, ai->end[j].portarcinst->proto,
					poly, FALSE);
				wid = ai->width - arcwidthoffset(ai);
				reduceportpoly(poly, ai->end[j].nodeinst, ai->end[j].portarcinst->proto, wid,
					(ai->userbits&AANGLE)>>AANGLESH);
				ix = ai->end[1-j].xpos;   iy = ai->end[1-j].ypos;
				closestpoint(poly, &ix, &iy);
				dx[j] = ix - ai->end[j].xpos;
				dy[j] = iy - ai->end[j].ypos;
			}
			if (dx[0] != 0 || dy[0] != 0 || dx[1] != 0 || dy[1] != 0)
			{
				startobjectchange((INTBIG)ai, VARCINST);
				if (modifyarcinst(ai, 0, dx[0], dy[0], dx[1], dy[1]))
				{
					lambda = lambdaofarc(ai);
					ttyputerr(_("Problem shortening arc %s: cannot move tail by (%s,%s) and head by (%s,%s)"),
						describearcinst(ai), latoa(dx[0], lambda), latoa(dy[0], lambda),
							latoa(dx[1], lambda), latoa(dy[1], lambda));
				}
				endobjectchange((INTBIG)ai, VARCINST);
				l++;
			}
		}
		ttyputmsg(x_("Shortened %ld %s"), l, makeplural(_("arc"), l));
	} else if (namesamen(pp, x_("curve"), l) == 0 && l >= 2)
	{
		if (list[0]->entryisnode || list[1] != NOGEOM)
		{
			us_abortcommand(_("Select only one arc when changing curvature"));
			us_pophighlight(FALSE);
			return;
		}
		ai = list[0]->entryaddr.ai;

		/* make sure the arc can handle it */
		if ((ai->proto->userbits&CANCURVE) == 0 && negate == 0)
		{
			us_abortcommand(_("This arc cannot curve"));
			us_pophighlight(FALSE);
			return;
		}

		/* see if curvature should be set interactively */
		interactive = 0;
		if (count >= 2 && namesamen(par[1], x_("interactive"), estrlen(par[1])) == 0)
			interactive = 1;

		/* change the arc center information */
		if (negate == 0)
		{
			if (interactive != 0)
			{
				us_arccurveinit(ai);
				trackcursor(TRUE, us_ignoreup, us_nullvoid, us_arccurvedown, us_stoponchar,
					us_nullvoid, TRACKDRAGGING);
			} else
			{
				if (us_demandxy(&xcur, &ycur))
				{
					us_pophighlight(FALSE);
					return;
				}

				/* get radius that lets arc curve through this point */
				r = us_curvearcthroughpoint(ai, xcur, ycur);

				startobjectchange((INTBIG)ai, VARCINST);
				(void)setvalkey((INTBIG)ai, VARCINST, el_arc_radius_key, r, VINTEGER);
				(void)modifyarcinst(ai, 0, 0, 0, 0, 0);
				endobjectchange((INTBIG)ai, VARCINST);
			}
		} else
		{
			/* remove curvature */
			startobjectchange((INTBIG)ai, VARCINST);
			(void)delvalkey((INTBIG)ai, VARCINST, el_arc_radius_key);
			(void)modifyarcinst(ai, 0, 0, 0, 0, 0);
			endobjectchange((INTBIG)ai, VARCINST);
		}
	} else ttyputbadusage(x_("arc"));

	/* clean-up */
	us_pophighlight(TRUE);
}

void us_array(INTBIG count, CHAR *par[])
{
	REGISTER BOOLEAN flipx, flipy, first, staggerx, staggery, centerx, centery,
		noname, diagonal, onlydrcvalid, *validity;
	REGISTER INTSML ro, tr;
	REGISTER INTBIG x, y, xpos, ypos, nodecount, arccount, i, l, lx, hx, ly, hy, cx, cy,
		cx0, cy0, cx1, cy1, xoff0, xoff1, yoff0, yoff1, xoff, yoff, keepbits, xindex,
		xsize, ysize, yindex, originalx, originaly, lambda, checknodecount;
	REGISTER GEOM **list, *geom;
	INTBIG xoverlap, yoverlap, plx, ply, phx, phy;
	REGISTER CHAR *pt1, *pt2;
	CHAR totalname[200], *objname;
	REGISTER NODEINST *ni, *newno, **nodelist, *ni0, *ni1, **nodestocheck;
	REGISTER ARCINST *ai, **arclist, *newai;
	REGISTER NODEPROTO *np;
	REGISTER PORTEXPINST *pe;
	REGISTER VARIABLE *var;

	/* special case for the "array file" command */
	if (count > 0 && namesamen(par[0], x_("file"), estrlen(par[0])) == 0)
	{
		us_arrayfromfile(par[1]);
		return;
	}

	/* get the objects to be arrayed */
	list = us_gethighlighted(WANTNODEINST|WANTARCINST, 0, 0);
	if (list[0] == NOGEOM)
	{
		us_abortcommand(_("Must select circuitry before arraying it"));
		return;
	}
	np = geomparent(list[0]);
	lambda = lambdaofcell(np);

	/* disallow arraying if lock is on */
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (!list[i]->entryisnode)
		{
			if (us_cantedit(np, NONODEINST, TRUE)) return;
		} else
		{
			if (us_cantedit(np, list[i]->entryaddr.ni, TRUE)) return;
		}
	}

	/* see if the array is to be unnamed (last argument is "no-names") */
	noname = diagonal = onlydrcvalid = FALSE;
	while (count > 0)
	{
		l = estrlen(pt1 = par[count-1]);
		if (namesamen(pt1, x_("no-names"), l) == 0)
		{
			noname = TRUE;
			count--;
		} else if (namesamen(pt1, x_("diagonal"), l) == 0)
		{
			diagonal = TRUE;
			count--;
		} else if (namesamen(pt1, x_("only-drc-valid"), l) == 0)
		{
			onlydrcvalid = TRUE;
			count--;
		} else break;
	}

	/* get the X extent of the array */
	if (count == 0)
	{
		ttyputusage(x_("array XREP[f][s][c] [YREP[f][s][c] [XOVRLP [YOVRLP [no-names][diagonal][only-drc-valid]]]]"));
		return;
	}
	pt1 = par[0];
	xsize = myatoi(pt1);
	flipx = staggerx = centerx = FALSE;
	for(i = estrlen(pt1)-1; i > 0; i--)
	{
		if (pt1[i] == 'f') flipx = TRUE; else
			if (pt1[i] == 's') staggerx = TRUE; else
				if (pt1[i] == 'c') centerx = TRUE; else
					break;
	}

	/* get the Y extent of the array */
	if (count == 1) pt2 = x_("1"); else pt2 = par[1];
	ysize = eatoi(pt2);
	flipy = staggery = centery = FALSE;
	for(i = estrlen(pt2)-1; i > 0; i--)
	{
		if (pt2[i] == 'f') flipy = TRUE; else
			if (pt2[i] == 's') staggery = TRUE; else
				if (pt2[i] == 'c') centery = TRUE; else
					break;
	}

	/* check for nonsense */
	if (xsize <= 1 && ysize <= 1)
	{
		us_abortcommand(_("One dimension of the array must be greater than 1"));
		return;
	}
	if (diagonal && xsize != 1 && ysize != 1)
	{
		us_abortcommand(_("Diagonal arrays need one dimension to be 1"));
		return;
	}

	/* mark the list of nodes and arcs in the cell that will be arrayed */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		ni->temp1 = 0;
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		ai->temp1 = 0;
	for(i=0; list[i] != NOGEOM; i++)
	{
		geom = list[i];
		if (geom->entryisnode)
		{
			ni = geom->entryaddr.ni;
			ni->temp1 = 1;
		} else
		{
			ai = geom->entryaddr.ai;
			ai->temp1 = 1;
			ai->end[0].nodeinst->temp1 = ai->end[1].nodeinst->temp1 = 1;
		}
	}

	/* count the number of nodes and arcs that will be arrayed */
	nodecount = arccount = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		if (ni->temp1 != 0) nodecount++;
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		if (ai->temp1 != 0) arccount++;

	/* build arrays of nodes and arcs that will be arrayed */
	if (nodecount > 0)
	{
		nodelist = (NODEINST **)emalloc(nodecount * (sizeof (NODEINST *)), el_tempcluster);
		if (nodelist == 0) return;
		for(ni = np->firstnodeinst, i=nodecount-1; ni != NONODEINST; ni = ni->nextnodeinst)
			if (ni->temp1 != 0) nodelist[i--] = ni;
		esort(nodelist, nodecount, sizeof (NODEINST *), us_sortnodesbyname);
	}
	if (arccount > 0)
	{
		arclist = (ARCINST **)emalloc(arccount * (sizeof (ARCINST *)), el_tempcluster);
		if (arclist == 0)
		{
			if (nodecount > 0) efree((CHAR *)nodelist);
			return;
		}
		for(ai = np->firstarcinst, i=0; ai != NOARCINST; ai = ai->nextarcinst)
			if (ai->temp1 != 0) arclist[i++] = ai;
		esort(arclist, arccount, sizeof (ARCINST *), us_sortarcsbyname);
	}

	/* determine spacing between arrayed objects */
	first = TRUE;
	for(i=0; i<nodecount; i++)
	{
		ni = nodelist[i];
		if (first)
		{
			lx = ni->geom->lowx;   hx = ni->geom->highx;
			ly = ni->geom->lowy;   hy = ni->geom->highy;
			first = FALSE;
		} else
		{
			if (ni->geom->lowx < lx) lx = ni->geom->lowx;
			if (ni->geom->highx > hx) hx = ni->geom->highx;
			if (ni->geom->lowy < ly) ly = ni->geom->lowy;
			if (ni->geom->highy > hy) hy = ni->geom->highy;
		}
	}
	for(i=0; i<arccount; i++)
	{
		ai = arclist[i];
		if (first)
		{
			lx = ai->geom->lowx;   hx = ai->geom->highx;
			ly = ai->geom->lowy;   hy = ai->geom->highy;
			first = FALSE;
		} else
		{
			if (ai->geom->lowx < lx) lx = ai->geom->lowx;
			if (ai->geom->highx > hx) hx = ai->geom->highx;
			if (ai->geom->lowy < ly) ly = ai->geom->lowy;
			if (ai->geom->highy > hy) hy = ai->geom->highy;
		}
	}
	xoverlap = hx - lx;   yoverlap = hy - ly;
	cx = (lx + hx) / 2;   cy = (ly + hy) / 2;

	/* get the overlap (if any) */
	if (count >= 3)
	{
		xoverlap -= atola(par[2], lambda);
		if (count >= 4) yoverlap -= atola(par[3], lambda);
	}

	/* check to ensure the array doesn't overflow the address space */
	if (!centerx)
	{
		for(x=0; x<xsize; x++)
		{
			plx = lx + xoverlap * x;
			phx = hx + xoverlap * x;
			if (staggery) { plx += xoverlap/2;   phx += xoverlap/2; }
			if (xoverlap > 0)
			{
				if (phx < lx) break;
			} else
			{
				if (plx > hx) break;
			}
		}
		if (x < xsize)
		{
			us_abortcommand(_("Array is too wide...place elements closer or use fewer"));
			if (nodecount > 0) efree((CHAR *)nodelist);
			if (arccount > 0) efree((CHAR *)arclist);
			return;
		}
	}
	if (!centery)
	{
		for(y=0; y<ysize; y++)
		{
			ply = ly + yoverlap * y;
			phy = hy + yoverlap * y;
			if (staggerx) { ply += yoverlap/2;   phy += yoverlap/2; }
			if (yoverlap > 0)
			{
				if (phy < ly) break;
			} else
			{
				if (ply > hy) break;
			}
		}
		if (y < ysize)
		{
			us_abortcommand(_("Array is too tall...place elements closer or use fewer"));
			if (nodecount > 0) efree((CHAR *)nodelist);
			if (arccount > 0) efree((CHAR *)arclist);
			return;
		}
	}

	/* if only arraying where DRC clean, make an array of newly created nodes */
	if (onlydrcvalid)
	{
		nodestocheck = (NODEINST **)emalloc(xsize * ysize * (sizeof (NODEINST *)), us_tool->cluster);
		if (nodestocheck == 0) return;
		validity = (BOOLEAN *)emalloc(xsize * ysize * (sizeof (BOOLEAN)), us_tool->cluster);
		if (validity == 0) return;
		checknodecount = 0;
		if (nodecount == 1)
			nodestocheck[checknodecount++] = nodelist[0];
	}

	/* save highlighting */
	us_pushhighlight();
	us_clearhighlightcount();

	/* create the array */
	for(y=0; y<ysize; y++) for(x=0; x<xsize; x++)
	{
		if (stopping(STOPREASONARRAY)) break;
		if (!centerx) xindex = x; else
			xindex = x - (xsize-1)/2;
		if (!centery) yindex = y; else
			yindex = y - (ysize-1)/2;
		if (xindex == 0 && yindex == 0)
		{
			originalx = x;
			originaly = y;
			continue;
		}

		/* initialize for queueing creation of new exports */
		us_initqueuedexports();

		/* first replicate the nodes */
		for(i=0; i<nodecount; i++)
		{
			ni = nodelist[i];
			if (diagonal && xsize == 1) xpos = cx + xoverlap * yindex; else
				xpos = cx + xoverlap * xindex;
			if (diagonal && ysize == 1) ypos = cy + yoverlap * xindex; else
				ypos = cy + yoverlap * yindex;
			xoff = (ni->lowx + ni->highx) / 2 - cx;
			yoff = (ni->lowy + ni->highy) / 2 - cy;
			if ((xindex&1) != 0 && staggerx) ypos += yoverlap/2;
			if ((yindex&1) != 0 && staggery) xpos += xoverlap/2;
			ro = ni->rotation;  tr = ni->transpose;
			if ((xindex&1) != 0 && flipx)
			{
				if (tr) ro += 2700; else ro += 900;
				ro %= 3600;
				tr = 1 - tr;
				xoff = -xoff;
			}
			if ((yindex&1) != 0 && flipy)
			{
				if (tr) ro += 900; else ro += 2700;
				ro %= 3600;
				tr = 1 - tr;
				yoff = -yoff;
			}
			xpos += xoff;   ypos += yoff;
			plx = xpos - (ni->highx - ni->lowx) / 2;   phx = plx + (ni->highx - ni->lowx);
			ply = ypos - (ni->highy - ni->lowy) / 2;   phy = ply + (ni->highy - ni->lowy);
			newno = newnodeinst(ni->proto, plx, phx, ply, phy, tr, ro, np);
			if (newno == NONODEINST)
			{
				ttyputerr(_("Problem creating array nodes"));
				if (nodecount > 0) efree((CHAR *)nodelist);
				if (arccount > 0) efree((CHAR *)arclist);
				us_pophighlight(FALSE);
				return;
			}
			keepbits = NEXPAND | HARDSELECTN | NTECHBITS;
			newno->userbits = (newno->userbits & ~keepbits) | (ni->userbits & keepbits);
			(void)copyvars((INTBIG)ni, VNODEINST, (INTBIG)newno, VNODEINST, TRUE);
			if (!noname)
			{
				objname = x_("");
				var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
				if (var != NOVARIABLE)
				{
					if (estrcmp((CHAR *)var->addr, x_("0")) != 0 && estrcmp((CHAR *)var->addr, x_("0-0")) != 0)
						objname = (CHAR *)var->addr;
				}
				if (xsize <= 1 || ysize <= 1)
					(void)esnprintf(totalname, 200, x_("%s%ld"), objname, x+y); else
						(void)esnprintf(totalname, 200, x_("%s%ld-%ld"), objname, x, y);
				var = setvalkey((INTBIG)newno, VNODEINST, el_node_name_key,
					(INTBIG)totalname, VSTRING|VDISPLAY);
				if (var != NOVARIABLE)
				{
					defaulttextsize(3, var->textdescript);

					/* shift text down if on a cell instance */
					if (ni->proto->primindex == 0)
					{
						us_setdescriptoffset(var->textdescript,
							0, (ni->highy-ni->lowy) / el_curlib->lambda[el_curtech->techindex]);
					}
				}
			}

			/* copy the ports, too */
			if ((us_useroptions&DUPCOPIESPORTS) != 0)
			{
				for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
				{
					if (us_queuenewexport(newno, pe->proto, pe->exportproto)) return;
				}
			}

			endobjectchange((INTBIG)newno, VNODEINST);
			ni->temp1 = (UINTBIG)newno;
			if (onlydrcvalid && i == 0)
				nodestocheck[checknodecount++] = newno;
		}

		/* create any queued exports */
		us_createqueuedexports();

		/* next replicate the arcs */
		for(i=0; i<arccount; i++)
		{
			ai = arclist[i];
			cx0 = (ai->end[0].nodeinst->lowx + ai->end[0].nodeinst->highx) / 2;
			cy0 = (ai->end[0].nodeinst->lowy + ai->end[0].nodeinst->highy) / 2;
			xoff0 = ai->end[0].xpos - cx0;
			yoff0 = ai->end[0].ypos - cy0;

			cx1 = (ai->end[1].nodeinst->lowx + ai->end[1].nodeinst->highx) / 2;
			cy1 = (ai->end[1].nodeinst->lowy + ai->end[1].nodeinst->highy) / 2;
			xoff1 = ai->end[1].xpos - cx1;
			yoff1 = ai->end[1].ypos - cy1;
			if ((xindex&1) != 0 && flipx)
			{
				xoff0 = -xoff0;
				xoff1 = -xoff1;
			}
			if ((yindex&1) != 0 && flipy)
			{
				yoff0 = -yoff0;
				yoff1 = -yoff1;
			}

			ni0 = (NODEINST *)ai->end[0].nodeinst->temp1;
			ni1 = (NODEINST *)ai->end[1].nodeinst->temp1;
			cx0 = (ni0->lowx + ni0->highx) / 2;
			cy0 = (ni0->lowy + ni0->highy) / 2;
			cx1 = (ni1->lowx + ni1->highx) / 2;
			cy1 = (ni1->lowy + ni1->highy) / 2;
			newai = newarcinst(ai->proto, ai->width, ai->userbits, ni0, ai->end[0].portarcinst->proto,
				cx0+xoff0, cy0+yoff0, ni1, ai->end[1].portarcinst->proto, cx1+xoff1, cy1+yoff1, np);
			if (newai == NOARCINST)
			{
				ttyputerr(_("Problem creating array arcs"));
				if (nodecount > 0) efree((CHAR *)nodelist);
				if (arccount > 0) efree((CHAR *)arclist);
				us_pophighlight(FALSE);
				return;
			}
			(void)copyvars((INTBIG)ai, VARCINST, (INTBIG)newai, VARCINST, TRUE);
			if (!noname)
			{
				objname = x_("");
				var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
				if (var != NOVARIABLE)
				{
					if (estrcmp((CHAR *)var->addr, x_("0")) != 0 && estrcmp((CHAR *)var->addr, x_("0-0")) != 0)
						objname = (CHAR *)var->addr;
				}
				if (xsize <= 1 || ysize <= 1)
					(void)esnprintf(totalname, 200, x_("%s%ld"), objname, x+y); else
						(void)esnprintf(totalname, 200, x_("%s%ld-%ld"), objname, x, y);
				var = setvalkey((INTBIG)newai, VARCINST, el_arc_name_key,
					(INTBIG)totalname, VSTRING|VDISPLAY);
				if (var != NOVARIABLE)
					defaulttextsize(4, var->textdescript);
			}
			endobjectchange((INTBIG)newai, VARCINST);
		}
	}

	/* rename the replicated objects */
	if (!noname)
	{
		for(i=0; i<nodecount; i++)
		{
			ni = nodelist[i];
			startobjectchange((INTBIG)ni, VNODEINST);

			/* get former name of node */
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
			objname = x_("");
			if (var != NOVARIABLE)
			{
				if (estrcmp((CHAR *)var->addr, x_("0")) != 0 && estrcmp((CHAR *)var->addr, x_("0-0")) != 0)
					objname = (CHAR *)var->addr;
			}
			if (xsize <= 1 || ysize <= 1)
			{
				(void)esnprintf(totalname, 200, x_("%s%ld"), objname, originalx+originaly);
			} else
			{
				(void)esnprintf(totalname, 200, x_("%s%ld-%ld"), objname, originalx, originaly);
			}
			var = setvalkey((INTBIG)ni, VNODEINST, el_node_name_key, (INTBIG)totalname, VSTRING|VDISPLAY);
			if (var != NOVARIABLE)
			{
				defaulttextsize(3, var->textdescript);

				/* shift text down if on a cell instance */
				if (ni->proto->primindex == 0)
				{
					us_setdescriptoffset(var->textdescript,
						0, (ni->highy-ni->lowy) / el_curlib->lambda[el_curtech->techindex]);
				}
			}
			endobjectchange((INTBIG)ni, VNODEINST);
		}
		for(i=0; i<arccount; i++)
		{
			ai = arclist[i];
			startobjectchange((INTBIG)ai, VARCINST);

			/* get former name of arc */
			var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
			objname = x_("");
			if (var != NOVARIABLE)
			{
				if (estrcmp((CHAR *)var->addr, x_("0")) != 0 && estrcmp((CHAR *)var->addr, x_("0-0")) != 0)
					objname = (CHAR *)var->addr;
			}
			if (xsize <= 1 || ysize <= 1)
			{
				(void)esnprintf(totalname, 200, x_("%s%ld"), objname, originalx+originaly);
			} else
			{
				(void)esnprintf(totalname, 200, x_("%s%ld-%ld"), objname, originalx, originaly);
			}
			var = setvalkey((INTBIG)ai, VARCINST, el_arc_name_key, (INTBIG)totalname, VSTRING|VDISPLAY);
			if (var != NOVARIABLE)
				defaulttextsize(4, var->textdescript);
			endobjectchange((INTBIG)ai, VARCINST);
		}
	}

	/* if only arraying where DRC valid, check them now and delete what is not valid */
	if (onlydrcvalid)
	{
		(void)asktool(dr_tool, x_("check-instances"), (INTBIG)np, checknodecount, (INTBIG)nodestocheck,
			(INTBIG)validity);
		for(i=1; i<checknodecount; i++)
		{
			if (!validity[i])
			{
				/* delete the node */
				killnodeinst(nodestocheck[i]);
			}
		}
		efree((CHAR *)nodestocheck);
		efree((CHAR *)validity);
	}

	/* restore highlighting */
	us_pophighlight(FALSE);
	if (nodecount > 0) efree((CHAR *)nodelist);
	if (arccount > 0) efree((CHAR *)arclist);
}

void us_bind(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG i, j, l, negated, bindex, newcount, menu, popup,
		x, y, backgroundcolor, swap, lastchar, varkey, formerquickkey, len;
	INTBIG special;
	INTSML key;
	REGISTER INTBIG but;
	INTBIG important, menumessagesize;
	REGISTER USERCOM *rb;
	REGISTER CHAR *pp, *letter, *popname, *str;
	NODEPROTO *nodeglyph;
	ARCPROTO *arcglyph;
	POPUPMENU *pm;
	REGISTER VARIABLE *var;
	COMMANDBINDING commandbinding, oldcommandbinding;
	CHAR partial[80], *a[MAXPARS+1], *menumessage, *pt;
	extern COMCOMP us_bindp, us_bindsetp, us_bindbuttonp, us_bindkeyp,
		us_bindmenuryp, us_bindmenuxp, us_userp, us_bindpopnp, us_bindpoprep;
	REGISTER void *infstr;

	if (count == 0)
	{
		count = ttygetparam(M_("BIND option: "), &us_bindp, MAXPARS, par);
		if (count == 0)
		{
			us_abortedmsg();
			return;
		}
	}
	l = estrlen(pp = par[0]);
	negated = 0;

	if (namesamen(pp, x_("not"), l) == 0 && l >= 1)
	{
		negated = 1;
		if (count <= 1)
		{
			ttyputusage(x_("bind not verbose"));
			return;
		}
		l = estrlen(pp = par[1]);
	}

	if (namesamen(pp, x_("verbose"), l) == 0 && l >= 1)
	{
		if (negated == 0)
		{
			us_tool->toolstate |= ECHOBIND;
			ttyputverbose(M_("Key/menu/button commands echoed"));
		} else
		{
			us_tool->toolstate &= ~ECHOBIND;
			ttyputverbose(M_("Key/menu/button commands not echoed"));
		}
		return;
	}

	if (namesamen(pp, x_("get"), l) == 0 && l >= 1)
	{
		if (count < 2)
		{
			ttyputusage(x_("bind get LETTER OBJECT"));
			return;
		}
		letter = par[1];
		if (!isalpha(*letter) || letter[1] != 0)
		{
			us_abortcommand(_("A single letter must be used, not '%s'"), letter);
			return;
		}

		l = estrlen(pp = par[2]);
		if (namesamen(pp, x_("button"), l) == 0 && l >= 1)
		{
			if (count < 4)
			{
				ttyputusage(x_("bind get LETTER button BUTTON"));
				return;
			}
			pp = par[3];
			for(but=0; but<buttoncount(); but++)
			{
				(void)estrcpy(partial, buttonname(but, &important));
				for(j=0; j<important; j++)
				{
					if (isupper(pp[j])) pp[j] = tolower(pp[j]);
					if (isupper(partial[j])) partial[j] = tolower(partial[j]);
					if (pp[j] != partial[j]) break;
				}
				if (j >= important) break;
			}
			if (j < important)
			{
				us_abortcommand(_("Button %s unknown"), pp);
				return;
			}
			var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_buttons_key);
			if (var == NOVARIABLE) return;
			us_parsebinding(((CHAR **)var->addr)[but], &commandbinding);
			if (*commandbinding.command == 0) commandbinding.command = x_("-");
			(void)setval((INTBIG)us_tool, VTOOL, us_commandvarname(*letter),
				(INTBIG)commandbinding.command, VSTRING|VDONTSAVE);
			us_freebindingparse(&commandbinding);
			return;
		}
		if (namesamen(pp, x_("key"), l) == 0 && l >= 1)
		{
			if (count < 4)
			{
				ttyputusage(x_("bind get LETTER key KEY"));
				return;
			}
			pp = par[3];
			key = *pp & 255;
			i = us_findboundkey(key, 0, &pt);
			if (i < 0) return;
			us_parsebinding(pt, &commandbinding);
			if (*commandbinding.command == 0) commandbinding.command = x_("-");
			(void)setval((INTBIG)us_tool, VTOOL, us_commandvarname(*letter),
				(INTBIG)commandbinding.command, VSTRING|VDONTSAVE);
			us_freebindingparse(&commandbinding);
			return;
		}
		if (namesamen(pp, x_("menu"), l) == 0 && l >= 1)
		{
			if (count < 5)
			{
				ttyputusage(x_("bind get LETTER menu ROW COLUMN"));
				return;
			}
			y = myatoi(pp = par[3]);
			x = myatoi(pp = par[4]);
			if (x < 0 || y < 0 || x >= us_menux || y >= us_menuy)
			{
				us_abortcommand(_("Index of menu entry out of range"));
				return;
			}
			var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_menu_key);
			if (var == NOVARIABLE) return;
			if (us_menupos <= 1)
				str = ((CHAR **)var->addr)[y * us_menux + x]; else
					str = ((CHAR **)var->addr)[x * us_menuy + y];
			us_parsebinding(str, &commandbinding);
			if (*commandbinding.command == 0) commandbinding.command = x_("-");
			(void)setval((INTBIG)us_tool, VTOOL, us_commandvarname(*letter),
				(INTBIG)commandbinding.command, VSTRING|VDONTSAVE);
			us_freebindingparse(&commandbinding);
			return;
		}
		if (namesamen(pp, x_("popup"), l) == 0 && l >= 1)
		{
			if (count < 5)
			{
				ttyputusage(x_("bind get LETTER popup MENU ENTRY"));
				return;
			}
			pm = us_getpopupmenu(par[3]);
			if (pm == NOPOPUPMENU)
			{
				us_abortcommand(_("No popup menu called %s"), par[3]);
				return;
			}
			popup = myatoi(par[4]) + 1;
			if (popup <= 0 || popup > pm->total)
			{
				us_abortcommand(_("Popup menu entry must be from 0 to %ld"), pm->total-1);
				return;
			}
			rb = pm->list[popup-1].response;

			/* make sure there is a command there */
			if (rb == NOUSERCOM || rb->active < 0) pp = x_("-"); else
			{
				infstr = initinfstr();
				addstringtoinfstr(infstr, rb->comname);
				us_appendargs(infstr, rb);
				pp = returninfstr(infstr);
			}
			(void)setval((INTBIG)us_tool, VTOOL, us_commandvarname(*letter),
				(INTBIG)pp, VSTRING|VDONTSAVE);
			return;
		}
		ttyputusage(x_("bind get LETTER key|button|menu|popup ENTRY"));
		return;
	}

	if (namesamen(pp, x_("set"), l) == 0 && l >= 1)
	{
		if (count < 2)
		{
			count = ttygetparam(M_("Key, Menu, Button, or Popup: "), &us_bindsetp, MAXPARS-1, &par[1])+1;
			if (count == 1)
			{
				us_abortedmsg();
				return;
			}
		}
		l = estrlen(pp = par[1]);
		important = 0;
		popup = menu = key = 0;
		special = 0;
		nodeglyph = NONODEPROTO;
		arcglyph = NOARCPROTO;
		backgroundcolor = 0;
		menumessage = 0;
		menumessagesize = TXTSETPOINTS(20);

		if (namesamen(pp, x_("button"), l) == 0 && l >= 1)
		{
			if (count < 3)
			{
				count = ttygetparam(M_("Button name: "), &us_bindbuttonp, MAXPARS-2, &par[2])+2;
				if (count == 2)
				{
					us_abortedmsg();
					return;
				}
			}
			pp = par[2];
			for(but=0; but<buttoncount(); but++)
			{
				(void)estrcpy(partial, buttonname(but, &important));
				for(j=0; j<important; j++)
				{
					if (isupper(pp[j])) pp[j] = tolower(pp[j]);
					if (isupper(partial[j])) partial[j] = tolower(partial[j]);
					if (pp[j] != partial[j]) break;
				}
				if (j >= important) break;
			}
			if (j < important)
			{
				us_abortcommand(_("Button %s unknown"), pp);
				return;
			}
			important = 3;
			varkey = us_binding_buttons_key;
			bindex = but;
		} else if (namesamen(pp, x_("key"), l) == 0 && l >= 1)
		{
			if (count < 3)
			{
				count = ttygetparam(M_("Key name: "), &us_bindkeyp, MAXPARS-2, &par[2])+2;
				if (count == 2)
				{
					us_abortedmsg();
					return;
				}
			}
			pp = par[2];
			(void)us_getboundkey(pp, &key, &special);
			varkey = us_binding_keys_key;
			important = 3;
		} else if ((namesamen(pp, x_("popup"), l) == 0 ||
			namesamen(pp, x_("input-popup"), l) == 0) && l >= 1)
		{
			if (count < 3)
			{
				count = ttygetparam(M_("Popup menu name: "), &us_bindpopnp, MAXPARS-2, &par[2])+2;
				if (count == 2)
				{
					us_abortedmsg();
					return;
				}
			}
			popname = par[2];
			pm = us_getpopupmenu(popname);
			if (pm == NOPOPUPMENU)
			{
				us_abortcommand(_("No popup menu called %s"), par[2]);
				return;
			}
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("USER_binding_popup_"));
			addstringtoinfstr(infstr, popname);
			varkey = makekey(returninfstr(infstr));

			/* look for possible message indication */
			if (count >= 5 && namesamen(par[3], x_("message"), estrlen(par[3])) == 0)
			{
				menumessage = par[4];
				count -= 2;
				par += 2;
			}

			if (count < 4)
			{
				count = ttygetparam(M_("Popup menu entry: "), &us_bindpoprep, MAXPARS-3, &par[3])+3;
				if (count == 3)
				{
					us_abortedmsg();
					return;
				}
			}
			popup = myatoi(par[3]) + 1;
			if (popup <= 0 || popup > pm->total)
			{
				us_abortcommand(_("Popup menu entry must be from 0 to %ld"), pm->total-1);
				return;
			}
			if (pp[0] == 'i') popup = -popup;
			important = 4;
			bindex = abs(popup);
		} else if (namesamen(pp, x_("menu"), l) == 0 && l >= 1)
		{
			/* look for possible glyph/message/background indication */
			while (count >= 4)
			{
				l = estrlen(pp = par[2]);
				if (namesamen(pp, x_("glyph"), l) == 0)
				{
					nodeglyph = getnodeproto(par[3]);
					if (nodeglyph == NONODEPROTO)
					{
						us_abortcommand(_("Cannot find glyph '%s' for component menu"), par[3]);
						return;
					}
					count -= 2;
					par += 2;
					continue;
				}
				if (namesamen(pp, x_("message"), l) == 0)
				{
					menumessage = par[3];
					count -= 2;
					par += 2;
					continue;
				}
				if (namesamen(pp, x_("textsize"), l) == 0)
				{
					menumessagesize = us_gettextsize(par[3], TXTSETQLAMBDA(12));
					count -= 2;
					par += 2;
					continue;
				}
				if (namesamen(pp, x_("background"), l) == 0)
				{
					backgroundcolor = getecolor(par[3]);
					count -= 2;
					par += 2;
					continue;
				}
				break;
			}

			/* get the menu row number */
			if (count < 3)
			{
				count = ttygetparam(M_("Menu row: "), &us_bindmenuryp, MAXPARS-2, &par[2]) + 2;
				if (count == 2)
				{
					us_abortedmsg();
					return;
				}
			}
			y = myatoi(pp = par[2]);
			if (count < 4)
			{
				count = ttygetparam(M_("Menu column: "), &us_bindmenuxp, MAXPARS+3, &par[3])+3;
				if (count == 3)
				{
					us_abortedmsg();
					return;
				}
			}
			x = myatoi(pp = par[3]);

			/* reorder the meaning if the optional "w" character is appended */
			lastchar = par[2][ estrlen(par[2])-1 ];
			if (lastchar == 'w' && us_menuy > us_menux)
			{
				swap = x;   x = y;   y = swap;
			} else
			{
				lastchar = par[3][ estrlen(par[3])-1 ];
				if (lastchar == 'w' && us_menux > us_menuy)
				{
					swap = x;   x = y;   y = swap;
				}
			}

			/* check validity */
			if (x < 0 || y < 0 || x >= us_menux || y >= us_menuy)
			{
				us_abortcommand(_("Index of menu entry out of range"));
				return;
			}
			varkey = us_binding_menu_key;
			if (us_menupos <= 1) bindex = y * us_menux + x; else
				bindex = x * us_menuy + y;
			important = 4;
			menu++;
		}

		if (important == 0)
		{
			ttyputusage(x_("bind set key|button|menu|[input-]popup COMMAND"));
			return;
		}

		if (count <= important)
			count = ttygetparam(M_("Command: "), &us_userp, MAXPARS-important,
				&par[important]) + important;
		infstr = initinfstr();
		for(i=important; i<count; i++)
		{
			addstringtoinfstr(infstr, par[i]);
			addtoinfstr(infstr, ' ');
		}

		newcount = us_parsecommand(returninfstr(infstr), a);
		if (newcount <= 0)
		{
			us_abortedmsg();
			return;
		}

		/* handle un-binding */
		if (estrcmp(par[important], x_("-")) == 0)
		{
			if (key != 0 && us_islasteval(key, special))
			{
				us_abortcommand(_("Cannot un-bind the last 'telltool user' key"));
				return;
			}
			infstr = initinfstr();
			if (key != 0)
				formatinfstr(infstr, x_("%c%c"), key, (special == 0 ? '\\' : '/'));
			if (popup != 0)
			{
				if (popup < 0) addstringtoinfstr(infstr, x_("inputpopup=")); else
					addstringtoinfstr(infstr, x_("popup="));
				addstringtoinfstr(infstr, popname);
				addtoinfstr(infstr, ' ');
			}
			if (menumessage != 0)
			{
				addstringtoinfstr(infstr, x_("message=\""));
				for(pt = menumessage; *pt != 0; pt++)
				{
					if (*pt == '"') addtoinfstr(infstr, '^');
					addtoinfstr(infstr, *pt);
				}
				addstringtoinfstr(infstr, x_("\" "));
			}
			addstringtoinfstr(infstr, x_("command="));
			if (key != 0)
			{
				bindex = us_findboundkey(key, special, &pt);
				if (bindex < 0) return;
			}

			/* clear the entry in the array */
			(void)setindkey((INTBIG)us_tool, VTOOL, varkey, bindex, (INTBIG)returninfstr(infstr));
			return;
		}

		i = parse(a[0], &us_userp, TRUE);
		if (i < 0)
		{
			us_unknowncommand();
			return;
		}

		/* special case: cannot rebind the last "telltool user" */
		if (i >= us_longcount || estrcmp(us_lcommand[i].name, x_("telltool")) != 0 ||
			newcount != 1 || estrcmp(a[1], x_("user")) != 0)
		{
			if (key != 0 && us_islasteval(key, special))
			{
				us_abortcommand(_("Cannot re-bind the last 'telltool user' key"));
				return;
			}
		}

		/* special case for menu binds of "getproto" command */
		if (menu != 0 && namesame(par[important], x_("getproto")) == 0)
		{
			if (newcount > 2 && namesame(a[1], x_("node")) == 0) nodeglyph = getnodeproto(a[2]); else
				if (newcount > 2 && namesame(a[1], x_("arc")) == 0) arcglyph = getarcproto(a[2]); else
					if (newcount >= 2) nodeglyph = getnodeproto(a[1]);
		}

		/* set the variable that describes this new binding */
		infstr = initinfstr();
		if (key != 0 || special != 0)
			addstringtoinfstr(infstr, us_describeboundkey(key, special, 0));
		if (nodeglyph != NONODEPROTO)
		{
			addstringtoinfstr(infstr, x_("node="));
			if (nodeglyph->primindex != 0)
			{
				addstringtoinfstr(infstr, nodeglyph->tech->techname);
				addtoinfstr(infstr, ':');
				addstringtoinfstr(infstr, nodeglyph->protoname);
			} else addstringtoinfstr(infstr, describenodeproto(nodeglyph));
			addtoinfstr(infstr, ' ');
		}
		if (arcglyph != NOARCPROTO)
		{
			addstringtoinfstr(infstr, x_("arc="));
			addstringtoinfstr(infstr, arcglyph->tech->techname);
			addtoinfstr(infstr, ':');
			addstringtoinfstr(infstr, arcglyph->protoname);
			addtoinfstr(infstr, ' ');
		}
		if (backgroundcolor != 0)
		{
			(void)esnprintf(partial, 80, x_("background=%ld "), backgroundcolor);
			addstringtoinfstr(infstr, partial);
		}
		if (menumessage != 0)
		{
			addstringtoinfstr(infstr, x_("message=\""));
			formerquickkey = 0;
			if (popup != 0)
			{
				/* check for defaulted quick-key in popup assignment */
				len = estrlen(menumessage) - 1;
				if (menumessage[len] == '<') len--;
				if (menumessage[len] == '/' || menumessage[len] == '\\')
				{
					/* defaulted: find the previous quick-key value */
					var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, varkey);
					if (var != NOVARIABLE)
					{
						us_parsebinding(((CHAR **)var->addr)[bindex], &oldcommandbinding);
						len = estrlen(oldcommandbinding.menumessage) - 1;
						if (oldcommandbinding.menumessage[len] == '<') len--;
						if (oldcommandbinding.menumessage[len-1] == '/' ||
							oldcommandbinding.menumessage[len-1] == '\\')
								formerquickkey = oldcommandbinding.menumessage[len];
						us_freebindingparse(&oldcommandbinding);
					}
				}
			}
			for(pt = menumessage; *pt != 0; pt++)
			{
				if (*pt == '"') addtoinfstr(infstr, '^');
				addtoinfstr(infstr, *pt);
				if (formerquickkey != 0 && (*pt == '/' || *pt == '\\'))
				{
					addtoinfstr(infstr, (CHAR)formerquickkey);
					formerquickkey = 0;
				}
			}
			addstringtoinfstr(infstr, x_("\" "));
			if (menumessagesize != 0)
			{
				(void)esnprintf(partial, 80, x_("messagesize=%ld "), menumessagesize);
				addstringtoinfstr(infstr, partial);
			}
		}
		if (popup != 0)
		{
			if (popup < 0) addstringtoinfstr(infstr, x_("inputpopup=")); else
				addstringtoinfstr(infstr, x_("popup="));
			addstringtoinfstr(infstr, popname);
			addtoinfstr(infstr, ' ');
		}
		addstringtoinfstr(infstr, x_("command="));
		for(j=important; j<count; j++)
		{
			if (j != important) addtoinfstr(infstr, ' ');
			addstringtoinfstr(infstr, par[j]);
		}
		if (key != 0 || special != 0)
		{
			us_setkeybinding(returninfstr(infstr), key, special, FALSE);
		} else
		{
			(void)setindkey((INTBIG)us_tool, VTOOL, varkey, bindex, (INTBIG)returninfstr(infstr));
		}
		if (popup > 0) us_setkeyequiv(pm, bindex);
		return;
	}

	ttyputbadusage(x_("bind"));
}
