/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrarc.c
 * User interface tool: simple arc routing
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
 * support@staticfreesoft.com
 */

#include "global.h"
#include "egraphics.h"
#include "usr.h"
#include "efunction.h"
#include "tecschem.h"
#include "tecgen.h"

/* prototypes for local routines */
static TECHNOLOGY *us_gettech(NODEINST*, PORTPROTO*);
static ARCINST    *us_directarcinst(NODEINST*, PORTPROTO*, ARCPROTO*, NODEINST*, PORTPROTO*, ARCPROTO*,
					NODEPROTO*, INTBIG, INTBIG, INTBIG, INTBIG, NODEINST**, INTBIG, INTBIG*, BOOLEAN);
static ARCINST    *us_onebend(NODEINST*, PORTPROTO*, ARCPROTO*, NODEINST*, PORTPROTO*, ARCPROTO*,
					NODEPROTO*, INTBIG, INTBIG, INTBIG, INTBIG, ARCINST**, NODEINST**, INTBIG*, BOOLEAN);
static ARCINST    *us_twobend(NODEINST*, PORTPROTO*, ARCPROTO*, NODEINST*, PORTPROTO*, ARCPROTO*,
					NODEPROTO*, INTBIG, INTBIG, INTBIG, INTBIG, ARCINST**, ARCINST**, NODEINST**, NODEINST**,
					INTBIG*, BOOLEAN);
static INTBIG      us_portdistance(NODEINST*, PORTPROTO*, INTBIG, INTBIG, INTBIG*, INTBIG*, INTBIG, BOOLEAN);
static BOOLEAN     us_fitportangle(NODEINST*, PORTPROTO*, INTBIG, INTBIG);
static INTBIG      us_figurearcwidth(ARCPROTO *typ, INTBIG wid, NODEINST *ni1, PORTPROTO *pp1,
					INTBIG ox1, INTBIG oy1, NODEINST *ni2, PORTPROTO *pp2, INTBIG ox2, INTBIG oy2);
static NODEPROTO  *us_findconnectingpin(ARCPROTO *ap, GEOM *geom, PORTPROTO *pp);
static void        us_getproperconsize(NODEPROTO *con, ARCPROTO *fromap, INTBIG fwid,
					ARCPROTO *toap, INTBIG twid, INTBIG *pxs, INTBIG *pys);
static BOOLEAN     us_breakarcinsertnode(ARCINST *ai, NODEINST *ni, ARCINST **ai1, ARCINST **ai2);

/*
 * routine to run an arcinst from "fromgeom" to "togeom".  The prefered
 * ports to use on these objects is "frompp" and "topp" (presuming that
 * "fromgeom" and "togeom" are nodeinsts and "frompp" and "topp" are
 * not NOPORTPROTO).  The prefered layer for the arc is "typ" but
 * this may be overridden if other layers are possible and the prefered
 * layer isn't.  The prefered location for the arcinst is closest to
 * (prefx, prefy) if there is a choice.  If "nozigzag" is true, do not
 * make 3-arc connections (only 1 or 2).  If the arcinst is run, the
 * routine returns its address (if two or three arcs are created, their
 * addresses are returned in "alt1" and "alt2", if contacts or pins are created,
 * their addresses are returned in "con1" and "con2").  Otherwise, it issues an
 * error and returns NOARCINST.  The required angle increment for the
 * arcs is "ang" tenth-degrees (900 for manhattan geometry).  The number of
 * arcs created is reported if "report" is TRUE.
 */
ARCINST *aconnect(GEOM *fromgeom, PORTPROTO *frompp, GEOM *togeom, PORTPROTO *topp,
	ARCPROTO *typ, INTBIG prefx, INTBIG prefy,
	ARCINST **alt1, ARCINST **alt2, NODEINST **con1, NODEINST **con2,
	INTBIG ang, BOOLEAN nozigzag, BOOLEAN report)
{
	REGISTER NODEINST *fromnodeinst, *tonodeinst, *ni;
	REGISTER NODEPROTO *con;
	REGISTER ARCINST *ai, *newai, *ai1, *ai2;
	REGISTER PORTPROTO *fpt, *tpt, *pp;
	REGISTER ARCPROTO *ap, *fap, *tap;
	REGISTER TECHNOLOGY *tech, *tech1, *tech2;
	REGISTER INTBIG j, t, ans, gent;
	REGISTER INTBIG i, bestdist, bestx, besty, usecontact;
	INTBIG x, y;
	CHAR *result[2];
	extern COMCOMP us_noyesp;
	REGISTER void *infstr;

	/* error check */
	if (fromgeom == togeom && !fromgeom->entryisnode)
	{
		us_abortcommand(_("Cannot run from arc to itself"));
		return(NOARCINST);
	}

	/* handle special case of an arcinst connecting to a cell */
	if (!fromgeom->entryisnode || !togeom->entryisnode)
	{
		if (!fromgeom->entryisnode) ai = fromgeom->entryaddr.ai; else
			ai = togeom->entryaddr.ai;
		ni = NONODEINST;
		if (fromgeom->entryisnode) { ni = fromgeom->entryaddr.ni;   pp = frompp; }
		if (togeom->entryisnode) { ni = togeom->entryaddr.ni;   pp = topp; }

		if (ni != NONODEINST && ni->proto->primindex == 0)
		{
			/* found cell connected to arcinst: search for closest portinst */
			if (pp == NOPORTPROTO)
			{
				bestdist = MAXINTBIG;
				for(fpt = ni->proto->firstportproto; fpt != NOPORTPROTO; fpt = fpt->nextportproto)
				{
					/* see if the arcinst can connect at this portinst */
					for(i=0; fpt->connects[i] != NOARCPROTO; i++)
						if (fpt->connects[i] == ai->proto) break;
					if (fpt->connects[i] == NOARCPROTO) continue;

					/* compute position for closest applicable portinst */
					i = us_portdistance(ni, fpt, prefx, prefy, &x, &y,
						defaultarcwidth(typ) - arcprotowidthoffset(typ), FALSE);
					if (i > bestdist) continue;
					bestdist = i;   bestx = x;   besty = y;
				}

				/* adjust prefered position to closest port (manhattan only!!!) */
				if (bestdist < MAXINTBIG)
				{
					if (ai->end[0].xpos == ai->end[1].xpos) prefy = besty;
					if (ai->end[0].ypos == ai->end[1].ypos) prefx = bestx;
				}
			}
		}
	}

	/* if two arcs are selected and they cross, make only 1 contact */
	fromnodeinst = NONODEINST;
	if (!fromgeom->entryisnode && !togeom->entryisnode)
	{
		ai1 = fromgeom->entryaddr.ai;
		ai2 = togeom->entryaddr.ai;
		if (segintersect(ai1->end[0].xpos, ai1->end[0].ypos, ai1->end[1].xpos, ai1->end[1].ypos,
			ai2->end[0].xpos, ai2->end[0].ypos, ai2->end[1].xpos, ai2->end[1].ypos, &x, &y))
		{
			/* they intersect */
			fromnodeinst = us_getnodeonarcinst(&fromgeom, &frompp, togeom, topp, x, y, 0);
			if (fromnodeinst == NONODEINST)
			{
				us_abortcommand(_("Cannot create splitting pin"));
				return(NOARCINST);
			}
			if (us_breakarcinsertnode(ai2, fromnodeinst, alt1, alt2)) return(NOARCINST);
			ai = *alt2;
			*alt2 = NOARCINST;
			*con1 = fromnodeinst;
			return(ai);
		}
	}

	/* get actual nodes for each end of connection */
	if (fromnodeinst == NONODEINST)
	{
		fromnodeinst = us_getnodeonarcinst(&fromgeom, &frompp, togeom, topp, prefx, prefy, 0);
		if (fromnodeinst == NONODEINST)
		{
			us_abortcommand(_("Cannot create splitting pin"));
			return(NOARCINST);
		}

		tonodeinst = us_getnodeonarcinst(&togeom, &topp, fromgeom, frompp, prefx, prefy, 0);
		if (tonodeinst == NONODEINST)
		{
			us_abortcommand(_("Cannot create splitting pin"));
			return(NOARCINST);
		}
	}

	/* default to single port on one-port nodeinsts */
	if (frompp == NOPORTPROTO)
		if ((fpt = fromnodeinst->proto->firstportproto) != NOPORTPROTO)
			if (fpt->nextportproto == NOPORTPROTO) frompp = fpt;
	if (topp == NOPORTPROTO)
		if ((tpt = tonodeinst->proto->firstportproto) != NOPORTPROTO)
			if (tpt->nextportproto == NOPORTPROTO) topp = tpt;

	/* sillyness check */
	if (fromnodeinst == tonodeinst && frompp == topp)
	{
		infstr = initinfstr();
		formatinfstr(infstr, _("Are you sure you want to run an arc from node %s to itself?"),
			describenodeinst(fromnodeinst));
		ans = ttygetparam(returninfstr(infstr), &us_noyesp, 1, result);
		if (ans <= 0 || (result[0][0] != 'y' && result[0][0] != 'Y')) return(NOARCINST);
	}

	/* reset all arcproto connection bits */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			ap->userbits &= ~CANCONNECT;

	/* set bits in arc prototypes that can make the connection */
	t = gent = 0;
	for(fpt = fromnodeinst->proto->firstportproto; fpt !=NOPORTPROTO; fpt = fpt->nextportproto)
	{
		if (frompp != NOPORTPROTO && frompp != fpt) continue;
		for (tpt = tonodeinst->proto->firstportproto; tpt != NOPORTPROTO; tpt = tpt->nextportproto)
		{
			if (topp != NOPORTPROTO && topp != tpt) continue;

			/* set those bits common to both "fpt" and "tpt" */
			for(i=0; fpt->connects[i] != NOARCPROTO; i++)
			{
				for(j=0; tpt->connects[j] != NOARCPROTO; j++)
				{
					if (tpt->connects[j] != fpt->connects[i]) continue;
					fpt->connects[i]->userbits |= CANCONNECT;
					if (fpt->connects[i]->tech == gen_tech) gent++; else t++;
					break;
				}
			}
		}
	}

	/* if no common ports, look for a contact */
	con = NONODEPROTO;
	usecontact = 0;
	if (t == 0)
	{
		tech = fromnodeinst->proto->tech;
		if (tech != gen_tech)
		{
			for(con = tech->firstnodeproto; con != NONODEPROTO; con = con->nextnodeproto)
			{
				if (((con->userbits&NFUNCTION) >> NFUNCTIONSH) != NPCONTACT) continue;
				pp = con->firstportproto;
				for(fpt = fromnodeinst->proto->firstportproto; fpt != NOPORTPROTO; fpt = fpt->nextportproto)
				{
					if (frompp != NOPORTPROTO && frompp != fpt) continue;
					for(i=0; pp->connects[i] != NOARCPROTO; i++)
					{
						if (pp->connects[i]->tech == gen_tech) continue;
						for(j=0; fpt->connects[j] != NOARCPROTO; j++)
							if (fpt->connects[j] == pp->connects[i]) break;
						if (fpt->connects[j] != NOARCPROTO) break;
					}
					if (pp->connects[i] != NOARCPROTO) break;
				}
				if (fpt == NOPORTPROTO) continue;
				fap = fpt->connects[j];

				for (tpt = tonodeinst->proto->firstportproto; tpt != NOPORTPROTO; tpt = tpt->nextportproto)
				{
					if (topp != NOPORTPROTO && topp != tpt) continue;
					for(i=0; pp->connects[i] != NOARCPROTO; i++)
					{
						if (pp->connects[i]->tech == gen_tech) continue;
						for(j=0; tpt->connects[j] != NOARCPROTO; j++)
							if (tpt->connects[j] == pp->connects[i]) break;
						if (tpt->connects[j] != NOARCPROTO) break;
					}
					if (pp->connects[i] != NOARCPROTO) break;
				}
				if (tpt == NOPORTPROTO) continue;
				tap = tpt->connects[j];

				/* this contact connects */
				t = 1;
				usecontact = 1;
				break;
			}
		}
	}

	/* if no common ports, don't run the arcinst */
	if (t+gent == 0)
	{
		us_abortcommand(_("Cannot find arc that connects %s to %s"), geomname(fromgeom),
			geomname(togeom));
		return(NOARCINST);
	}

	/* if a contact must be used, try it */
	if (usecontact != 0)
	{
		newai = us_makeconnection(fromnodeinst, frompp, fap, tonodeinst, topp, tap,
			con, prefx, prefy, alt1, alt2, con1, con2, ang, nozigzag, 0, report);
		if (newai != NOARCINST) return(newai);
	}

	/* see if the default arc prototype can be used */
	if ((typ->userbits&CANCONNECT) != 0)
	{
		newai = us_makeconnection(fromnodeinst, frompp, typ, tonodeinst, topp, typ,
			getpinproto(typ), prefx, prefy, alt1, alt2, con1, con2, ang, nozigzag, 0, report);
		if (newai != NOARCINST) return(newai);
	}

	/* default arc prototype cannot be used: try others in this technology */
	for(ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
	{
		if ((ap->userbits&CANCONNECT) == 0) continue;
		if (ap == typ) continue;
		newai = us_makeconnection(fromnodeinst, frompp, ap, tonodeinst, topp, ap,
			getpinproto(ap), prefx, prefy, alt1, alt2, con1, con2, ang, nozigzag, 0, report);
		if (newai != NOARCINST) return(newai);
	}

	/* current technology bad: see if this is a cross-technology connect */
	tech1 = us_gettech(fromnodeinst, frompp);
	tech2 = us_gettech(tonodeinst, topp);
	if (tech1 == tech2)
	{
		/* if current technology not that of the two nodes, check it */
		if (tech1 != el_curtech)
			for(ap = tech1->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
		{
			if ((ap->userbits&CANCONNECT) == 0) continue;
			if (ap == typ) continue;
			newai = us_makeconnection(fromnodeinst, frompp, ap, tonodeinst, topp, ap,
				getpinproto(ap), prefx, prefy, alt1, alt2, con1, con2, ang, nozigzag, 0, report);
			if (newai != NOARCINST) return(newai);
		}
	} else
	{
		/* current technology bad: try other technologies */
		for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
			if (tech != el_curtech)
				for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
		{
			if ((ap->userbits&CANCONNECT) == 0) continue;
			newai = us_makeconnection(fromnodeinst, frompp, ap, tonodeinst, topp, ap,
				getpinproto(ap), prefx, prefy, alt1, alt2, con1, con2, ang, nozigzag, 0, report);
			if (newai != NOARCINST) return(newai);
		}
	}
	us_abortcommand(_("There is no way to connect these objects"));
	return(NOARCINST);
}

/*
 * routine to return the technology associated with node "ni", port "pp".
 */
TECHNOLOGY *us_gettech(NODEINST *ni, PORTPROTO *pp)
{
	for(;;)
	{
		if (ni->proto->primindex != 0) break;
		if (pp == NOPORTPROTO) break;
		ni = pp->subnodeinst;
		pp = pp->subportproto;
	}
	return(ni->proto->tech);
}

/*
 * routine to try to run an arcinst from nodeinst "fromnodeinst" to nodeinst
 * "tonodeinst".  The default port prototypes to use are in "fromportproto" and
 * "toportproto".  The arcs to use are "fromarcproto" and "toarcproto".  The
 * connecting pin (if necessary) is "con".  If it can be done, the arcinst is
 * created and the routine returns its address, otherwise it returns
 * NOARCINST.  If two or three arcs are created, their addresses are returned
 * in "alt1" and "alt2". If contacts or pins are created, their addresses are
 * returned in "con1" and "con2". The preferred arcinst runs closest to (prefx, prefy).
 * The most direct route will be traveled, but an additional nodeinst or two
 * may have to be created.  The angle increment for the arcs is "ang".
 * If "nozigzag" is nonzero, disallow 3-arc connections (only 1 or 2).  The number of
 * arcs created is reported if "report" is TRUE.
 */
ARCINST *us_makeconnection(NODEINST *fromnodeinst, PORTPROTO *fromportproto, ARCPROTO *fromarcproto,
	NODEINST *tonodeinst, PORTPROTO *toportproto, ARCPROTO *toarcproto, NODEPROTO *con, INTBIG prefx,
	INTBIG prefy, ARCINST **alt1, ARCINST **alt2, NODEINST **con1, NODEINST **con2,
	INTBIG ang, BOOLEAN nozigzag, INTBIG *fakecoords, BOOLEAN report)
{
	REGISTER PORTPROTO *fpt, *tpt;
	REGISTER INTBIG bestdist, dist, fwid, twid, w;
	REGISTER INTBIG i;
	REGISTER ARCINST *newai;
	INTBIG x, y, hx, hy, lx, ly;

	/* see if the cursor is near a port on a cell */
	bestdist = MAXINTBIG;
	if (fromportproto == NOPORTPROTO && fromnodeinst->proto->primindex == 0)
		for(fpt = fromnodeinst->proto->firstportproto; fpt != NOPORTPROTO; fpt = fpt->nextportproto)
	{
		for(i=0; fpt->connects[i] != NOARCPROTO; i++)
			if (fromarcproto == fpt->connects[i]) break;
		if (fpt->connects[i] == NOARCPROTO) continue;

		/* portproto may be valid: compute distance to it */
		us_portposition(fromnodeinst, fpt, &x, &y);
		dist = abs(x-prefx) + abs(y-prefy);
		if (dist > bestdist) continue;
		bestdist = dist;   fromportproto = fpt;
	}
	if (toportproto == NOPORTPROTO && tonodeinst->proto->primindex == 0)
		for(tpt = tonodeinst->proto->firstportproto; tpt != NOPORTPROTO; tpt = tpt->nextportproto)
	{
		for(i=0; tpt->connects[i] != NOARCPROTO; i++)
			if (toarcproto == tpt->connects[i]) break;
		if (tpt->connects[i] == NOARCPROTO) continue;

		/* portproto may be valid: compute distance to it */
		us_portposition(tonodeinst, tpt, &x, &y);
		dist = abs(x-prefx) + abs(y-prefy);
		if (dist > bestdist) continue;
		bestdist = dist;   toportproto = tpt;
	}
	if (fromportproto == NOPORTPROTO || toportproto == NOPORTPROTO) return(NOARCINST);

	/* determine the width of this arc from others on the nodes */
	fwid = defaultarcwidth(fromarcproto);
	w = us_widestarcinst(fromarcproto, fromnodeinst, fromportproto);
	if (w > fwid) fwid = w;
	twid = defaultarcwidth(toarcproto);
	w = us_widestarcinst(toarcproto, tonodeinst, toportproto);
	if (w > twid) twid = w;

	/* now first check for direct connection */
	*con1 = *con2 = NONODEINST;
	newai = us_directarcinst(fromnodeinst, fromportproto, fromarcproto,
		tonodeinst, toportproto, toarcproto, con,
			prefx, prefy, fwid, twid, con1, ang, fakecoords, report);
	if (newai != NOARCINST)
	{
		*alt1 = *alt2 = NOARCINST;
		return(newai);
	}

	/* now try a zig-zag if Manhattan or diagonal */
	if ((ang == 450 || ang == 900) && !nozigzag)
	{
		/*
		 * check location of cursor with respect to fromnode and tonode.
		 * There are nine possibilities.  Each implies a specific routing
		 * request (cases where cursor is lined up horizontally or vertically
		 * with either fromnode or tonode are not included as they default to
		 * L-connections)
		 *       1   2   3
		 *         *             [fromnode]
		 *       4   5   6
		 *             *         [tonode]
		 *       7   8   9
		 */
		us_portposition(fromnodeinst, fromportproto, &lx, &ly);
		us_portposition(tonodeinst, toportproto, &hx, &hy);
		if (hx < lx)
		{
			x = lx;   lx = hx;   hx = x;
		}
		if (hy < ly)
		{
			y = ly;   ly = hy;   hy = y;
		}

		/* if cursor location is in 2, 4, 5, 6, or 8, try two-bend connection */
		if ((prefx > lx && prefx < hx) || (prefy > ly && prefy < hy))
		{
			newai = us_twobend(fromnodeinst, fromportproto, fromarcproto, tonodeinst,
				toportproto, toarcproto, con, prefx, prefy, fwid, twid,
				alt1, alt2, con1, con2, fakecoords, report);
			if (newai != NOARCINST) return(newai);
		}
	}

	/* next check for a one-bend connection */
	newai = us_onebend(fromnodeinst, fromportproto, fromarcproto, tonodeinst, toportproto,
		toarcproto, con, prefx, prefy, fwid, twid, alt1, con1, fakecoords, report);
	if (newai != NOARCINST)
	{
		*alt2 = NOARCINST;
		return(newai);
	}

	/* finally check for any zig-zag connection */
	newai = us_twobend(fromnodeinst, fromportproto, fromarcproto, tonodeinst, toportproto,
		toarcproto, con, prefx, prefy, fwid, twid, alt1, alt2, con1, con2, fakecoords, report);
	if (newai != NOARCINST) return(newai);

	/* give up */
	return(NOARCINST);
}

/*
 * routine to check for direct connection from nodeinst "fromnodeinst", port "fromppt",
 * arc "fromarcproto" to nodeinst "tonodeinst",  port "toppt", arc "toarcproto" (with
 * a possible via of type "con"). and create an arc that is "wid" wide if possible.
 * It is prefered that the arcinst be close to (prefx, prefy).  If "fakecoords" is
 * zero, create the arc if possible.  If "fakecoords" is nonzero, do not create an arc,
 * but just store the coordinates of the two endpoints in the four integers there (and
 * return any non-NOARCINST value).  If the arcinst is created, the routine
 * returns its address, otherwise it returns NOARCINST.  The angle increment
 * for the arc is "ang" tenth-degrees (900 for manhattan geometry).
 */
ARCINST *us_directarcinst(NODEINST *fromnodeinst, PORTPROTO *fromppt, ARCPROTO *fromarcproto,
	NODEINST *tonodeinst, PORTPROTO *toppt, ARCPROTO *toarcproto, NODEPROTO *con,
	INTBIG prefx, INTBIG prefy, INTBIG fwid, INTBIG twid, NODEINST **con1, INTBIG ang,
	INTBIG *fakecoords, BOOLEAN report)
{
	REGISTER PORTPROTO *fpt, *tpt, *fstart, *tstart;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni;
	PORTPROTO *fromportproto, *toportproto;
	REGISTER INTBIG i, j, trange, frange, bad, gotpath;
	REGISTER INTBIG edgealignment, alignment;
	INTBIG bestdist, dist, bestpdist, pdist, bestfx,bestfy, besttx,bestty, otheralign, fpx, fpy,
		tpx, tpy, flx, fly, fhx, fhy, tlx, tly, thx, thy, wid, frwid, trwid, x, y, bestxi, bestyi,
		pxs, pys, lx, hx, ly, hy;
	static POLYGON *fpoly = NOPOLYGON, *tpoly = NOPOLYGON;

	/* get polygons */
	(void)needstaticpolygon(&fpoly, 4, us_tool->cluster);
	(void)needstaticpolygon(&tpoly, 4, us_tool->cluster);

	/* determine true width */
	fwid = us_figurearcwidth(fromarcproto, fwid, fromnodeinst, fromppt, prefx, prefy, tonodeinst, toppt, prefx, prefy);
	frwid = fwid - arcprotowidthoffset(fromarcproto);
	twid = us_figurearcwidth(toarcproto, twid, fromnodeinst, fromppt, prefx, prefy, tonodeinst, toppt, prefx, prefy);
	trwid = twid - arcprotowidthoffset(toarcproto);
	wid = maxi(fwid, twid);

	/* assume default prefered positions for port locations */
	tpx = (tonodeinst->highx + tonodeinst->lowx) / 2;
	tpy = (tonodeinst->highy + tonodeinst->lowy) / 2;
	fpx = (fromnodeinst->highx + fromnodeinst->lowx) / 2;
	fpy = (fromnodeinst->highy + fromnodeinst->lowy) / 2;

	/* precompute better positions if ports were specified */
	if (toppt != NOPORTPROTO)
	{
		us_portposition(tonodeinst, toppt, &tpx, &tpy);
		tpoly->xv[0] = prefx;   tpoly->yv[0] = prefy;   tpoly->count = 1;
		shapeportpoly(tonodeinst, toppt, tpoly, TRUE);
		reduceportpoly(tpoly, tonodeinst, toppt, trwid, -1);
		if ((toppt->userbits&PORTISOLATED) != 0)
		{
			/* use prefered location on isolated ports */
			tpx = prefx;   tpy = prefy;
			closestpoint(tpoly, &tpx, &tpy);
		}
		tstart = toppt;
	} else tstart = tonodeinst->proto->firstportproto;
	if (fromppt != NOPORTPROTO)
	{
		us_portposition(fromnodeinst, fromppt, &fpx, &fpy);
		fpoly->xv[0] = prefx;   fpoly->yv[0] = prefy;   fpoly->count = 1;
		shapeportpoly(fromnodeinst, fromppt, fpoly, TRUE);
		reduceportpoly(fpoly, fromnodeinst, fromppt, frwid, -1);
		if ((fromppt->userbits&PORTISOLATED) != 0)
		{
			/* use prefered location on isolated ports */
			fpx = prefx;   fpy = prefy;
			closestpoint(fpoly, &fpx, &fpy);
		}
		fstart = fromppt;
	} else fstart = fromnodeinst->proto->firstportproto;

	/* check again to make sure the ports align */
	if (toppt != NOPORTPROTO)
	{
		tpx = fpx;   tpy = fpy;
		closestpoint(tpoly, &tpx, &tpy);
	}
	if (fromppt != NOPORTPROTO)
	{
		fpx = tpx;   fpy = tpy;
		closestpoint(fpoly, &fpx, &fpy);
	}
	if (toppt != NOPORTPROTO)
	{
		tpx = fpx;   tpy = fpy;
		closestpoint(tpoly, &tpx, &tpy);
	}

	/* search every possible port on the "from" node */
	bestdist = bestpdist = MAXINTBIG;
	for(fpt = fstart; fpt != NOPORTPROTO; fpt = fpt->nextportproto)
	{
		if (fromppt != NOPORTPROTO && fromppt != fpt) break;

		/* see if the port has an opening for this type of arcinst */
		for(i=0; fpt->connects[i] != NOARCPROTO; i++)
			if (fromarcproto == fpt->connects[i]) break;
		if (fpt->connects[i] == NOARCPROTO) continue;

		/* potential port: get information about its position if not already known */
		if (fromppt == NOPORTPROTO)
		{
			fpoly->xv[0] = prefx;   fpoly->yv[0] = prefy;   fpoly->count = 1;
			shapeportpoly(fromnodeinst, fpt, fpoly, TRUE);

			/* handle arc width offset from node edge */
			reduceportpoly(fpoly, fromnodeinst, fpt, frwid, -1);

			/* find single closest point */
			fpx = tpx;   fpy = tpy;
			closestpoint(fpoly, &fpx, &fpy);
		}

		/* correlate with every possible port on the "to" node */
		for(tpt = tstart; tpt != NOPORTPROTO; tpt = tpt->nextportproto)
		{
			if (toppt != NOPORTPROTO && toppt != tpt) break;

			/* silly to run from port to itself when ports are unspecified */
			if (fromnodeinst == tonodeinst && fpt == tpt &&
				(fromppt == NOPORTPROTO || toppt == NOPORTPROTO)) continue;

			/* see if the port has an opening for this type of arcinst */
			for(i=0; tpt->connects[i] != NOARCPROTO; i++)
				if (toarcproto == tpt->connects[i]) break;
			if (tpt->connects[i] == NOARCPROTO) continue;

			/* get the shape of the "to" port if not already done */
			if (toppt == NOPORTPROTO)
			{
				tpoly->xv[0] = prefx;   tpoly->yv[0] = prefy;   tpoly->count = 1;
				shapeportpoly(tonodeinst, tpt, tpoly, TRUE);

				/* handle arc width offset from node edge */
				reduceportpoly(tpoly, tonodeinst, tpt, trwid, -1);

				/* find single closest point */
				tpx = fpx;   tpy = fpy;
				closestpoint(tpoly, &tpx, &tpy);
			}

			/* check directionality of ports */
			trange = ((tpt->userbits&PORTARANGE) >> PORTARANGESH) * 10;
			frange = ((fpt->userbits&PORTARANGE) >> PORTARANGESH) * 10;

			/* ignore range calculations for serpentine transistors */
			if ((tonodeinst->proto->userbits&HOLDSTRACE) != 0 &&
				gettrace(tonodeinst) != NOVARIABLE)
					trange = 1800;
			if ((fromnodeinst->proto->userbits&HOLDSTRACE) != 0 &&
				gettrace(fromnodeinst) != NOVARIABLE)
					frange = 1800;

			/* make sure ranges are acceptable */
			if (trange != 1800 || frange != 1800)
			{
				/* determine angle between port centers */
				bad = 0;
				if (fpx != tpx || fpy != tpy)
				{
					/* see if the angle is permitted */
					i = figureangle(fpx, fpy, tpx, tpy);
					if (us_fitportangle(fromnodeinst, fpt, i, frange)) bad = 1; else
						if (us_fitportangle(tonodeinst, tpt, i+1800, trange))
							bad = 1;
				}

				/* special case for ports that overlap */
				if (bad != 0)
				{
					flx = (fromnodeinst->lowx+fromnodeinst->highx)/2;
					fly = (fromnodeinst->lowy+fromnodeinst->highy)/2;
					fhx = (tonodeinst->lowx+tonodeinst->highx)/2;
					fhy = (tonodeinst->lowy+tonodeinst->highy)/2;
					if (flx != fhx || fly != fhy)
					{
						j = figureangle(flx, fly, fhx, fhy);
						if ((j+1800)%3600 == i &&
							!us_fitportangle(fromnodeinst, fpt, j, frange) &&
								!us_fitportangle(tonodeinst, tpt, j+1800, trange))
									bad = 0;
					}
				}
				if (bad != 0) continue;
			}

			/* see if an arc can connect at this angle */
			if (ang == 0)
			{
				/* no angle restrictions: simply use the chosen locations */
				gotpath = 1;
			} else
			{
				/* if manhattan is possible, test it first */
				gotpath = 0;
				if (900 / ang * ang == 900)
				{
					/* manhattan angle restriction: check directly */
					if (tpx == fpx || tpy == fpy) gotpath = 1;
				}
				if (gotpath == 0 && ang != 900)
				{
					flx = fhx = fpx;   fly = fhy = fpy;
					tlx = thx = tpx;   tly = thy = tpy;

					/* arbitrary angle restrictions: try all angle possibilities */
					for(i=0; i<3600; i += ang)
						if (arcconnects(i, flx,fhx,fly,fhy, tlx,thx,tly,thy, &fpx,&fpy, &tpx,&tpy))
					{
						gotpath = 1;
						break;
					}
				}
			}
			if (gotpath == 0) continue;

			/* for manhattan arcs, adjust if edge alignment requested */
			if (fpx == tpx)
			{
				/* arcinst runs vertically */
				getbbox(fpoly, &flx, &fhx, &fly, &fhy);
				getbbox(tpoly, &tlx, &thx, &tly, &thy);
				if (us_edgealignment_ratio != 0 && flx != fhx && tlx != thx)
				{
					/* make the arc edge align */
					edgealignment = muldiv(us_edgealignment_ratio, WHOLE, el_curlib->lambda[el_curtech->techindex]);
					x = us_alignvalue(fpx - frwid/2, edgealignment, &otheralign) + frwid/2;
					otheralign += frwid/2;
					if (x <= mini(fhx,thx) && x >= maxi(flx,tlx)) tpx = fpx = x; else
						if (otheralign <= mini(fhx,thx) && otheralign >= maxi(flx,tlx))
							fpx = tpx = otheralign;

					/* try to align the ends */
					y = us_alignvalue(tpy+trwid/2, edgealignment, &otheralign) - trwid/2;
					otheralign -= trwid/2;
					if (isinside(tpx, y, tpoly)) tpy = y; else
						if (isinside(tpx, otheralign, tpoly)) tpy = otheralign;
					y = us_alignvalue(fpy+frwid/2, edgealignment, &otheralign) - frwid/2;
					otheralign -= frwid/2;
					if (isinside(fpx, y, fpoly)) fpy = y; else
						if (isinside(fpx, otheralign, fpoly)) fpy = otheralign;
				}
			} else if (fpy == tpy)
			{
				/* arcinst runs horizontally */
				getbbox(fpoly, &flx, &fhx, &fly, &fhy);
				getbbox(tpoly, &tlx, &thx, &tly, &thy);
				if (us_edgealignment_ratio != 0 && fly != fhy && tly != thy)
				{
					/* make the arc edge align */
					edgealignment = muldiv(us_edgealignment_ratio, WHOLE, el_curlib->lambda[el_curtech->techindex]);
					y = us_alignvalue(tpy - trwid/2, edgealignment, &otheralign) + trwid/2;
					otheralign += trwid/2;
					if (y <= mini(fhy,thy) && y >= maxi(fly,tly)) tpy = fpy = y; else
						if (otheralign <= mini(fhy,thy) && otheralign >= maxi(fly,tly))
							tpy = fpy = otheralign;

					/* try to align the ends */
					x = us_alignvalue(tpx+trwid/2, edgealignment, &otheralign) - trwid/2;
					otheralign -= trwid/2;
					if (isinside(x, tpy, tpoly)) tpx = x; else
						if (isinside(otheralign, tpy, tpoly)) tpx = otheralign;
					x = us_alignvalue(fpx+frwid/2, edgealignment, &otheralign) - frwid/2;
					otheralign -= frwid/2;
					if (isinside(fpx, x, fpoly)) fpx = x; else
						if (isinside(otheralign, fpy, fpoly)) fpx = otheralign;
				}
			}

			/* if this path is longer than another, forget it */
			dist = abs(fpx-tpx) + abs(fpy-tpy);
			if (dist > bestdist) continue;

			/* if this path is same as another check prefered position */
			pdist = (fpx==tpx) ? abs(prefx-fpx) : abs(prefy-fpy);
			if (dist == bestdist && pdist > bestpdist) continue;

			/* this is best: remember it */
			bestdist = dist;       bestpdist = pdist;
			fromportproto = fpt;   toportproto = tpt;
			bestfx = fpx;          bestfy = fpy;
			besttx = tpx;          bestty = tpy;
		}
	}

	if (bestdist < MAXINTBIG)
	{
		if (fakecoords == 0)
		{
			if (fromarcproto != toarcproto)
			{
				/* create the intermediate nodeinst */
				alignment = muldiv(us_alignment_ratio, el_curlib->lambda[el_curtech->techindex], WHOLE);
				if (bestfx == besttx)
				{
					bestxi = bestfx;
					bestyi = us_alignvalue(prefy, alignment, &otheralign);
				} else if (bestfy == bestty)
				{
					bestxi = us_alignvalue(prefx, alignment, &otheralign);
					bestyi = bestfy;
				} else
				{
					bestxi = prefx;
					bestyi = prefy;
				}
				us_getproperconsize(con, fromarcproto, fwid, toarcproto, twid, &pxs, &pys);
				lx = bestxi - pxs/2;   hx = lx + pxs;
				ly = bestyi - pys/2;   hy = ly + pys;
				*con1 = ni = newnodeinst(con, lx, hx, ly, hy, 0, 0, fromnodeinst->parent);
				if (ni == NONODEINST)
				{
					us_abortcommand(_("Cannot create connecting contact"));
					return(NOARCINST);
				}
				endobjectchange((INTBIG)ni, VNODEINST);

				ai = us_runarcinst(fromnodeinst, fromportproto, bestfx, bestfy, 
					ni, con->firstportproto, bestxi, bestyi, fromarcproto, fwid);
				if (ai == NOARCINST) return(NOARCINST);

				ai = us_runarcinst(ni, con->firstportproto, bestxi, bestyi, 
					tonodeinst, toportproto, besttx, bestty, toarcproto, twid);
				if (ai == NOARCINST) return(NOARCINST);
				if (report)
					us_reportarcscreated(0, 1, fromarcproto->protoname, 1, toarcproto->protoname);
			} else
			{
				ai = us_runarcinst(fromnodeinst, fromportproto, bestfx, bestfy, 
					tonodeinst, toportproto, besttx, bestty, fromarcproto, wid);
				if (ai == NOARCINST) return(NOARCINST);
				if (report)
					us_reportarcscreated(0, 1, fromarcproto->protoname, 0, x_(""));
			}
			return(ai);
		} else
		{
			fakecoords[0] = bestfx;
			fakecoords[1] = bestfy;
			fakecoords[2] = besttx;
			fakecoords[3] = bestty;
			return((ARCINST *)1);
		}
	}

	/* give up */
	return(NOARCINST);
}

/*
 * Routine to report that activity "who" created "numarcs1" arcs of type "type1" and "numarcs2"
 * arcs of type "type2".  If "who" is zero, ignore that part of the message.  If "numarcs2" is zero,
 * ignore that part of the message.  If "type1" is zero, ignore that part of the message.
 * Also plays a sound appropriate to the number of arcs created.
 */
void us_reportarcscreated(CHAR *who, INTBIG numarcs1, CHAR *type1, INTBIG numarcs2, CHAR *type2)
{
	REGISTER void *infstr;
	REGISTER INTBIG totalcreated;

	infstr = initinfstr();
	if (who != 0) formatinfstr(infstr, x_("%s: "), who);
	if (numarcs2 > 0)
	{
		formatinfstr(infstr, _("Created %ld %s and %ld %s %s"), numarcs1, type1, numarcs2, type2,
			makeplural(x_("arc"), numarcs2));
	} else
	{
		if (type1 == 0)
		{
			formatinfstr(infstr, _("Created %ld %s"), numarcs1, makeplural(x_("arc"), numarcs1));
		} else
		{
			formatinfstr(infstr, _("Created %ld %s %s"), numarcs1, type1, makeplural(x_("arc"), numarcs1));
		}
	}
	ttyputmsg(x_("%s"), returninfstr(infstr));
	totalcreated = numarcs1 + numarcs2;
	if (totalcreated == 1)
	{
		ttybeep(SOUNDCLICK, TRUE);
	} else if (totalcreated == 2)
	{
		ttybeep(SOUNDCLICK, TRUE);
		ttybeep(SOUNDCLICK, TRUE);
	} else if (totalcreated >= 3)
	{
		ttybeep(SOUNDCLICK, TRUE);
		ttybeep(SOUNDCLICK, TRUE);
		ttybeep(SOUNDCLICK, TRUE);
	}
}

/*
 * routine to check for a one-bend connection running from nodeinst
 * "fromnodeinst" to nodeinst "tonodeinst" on arcproto "typ" and create two
 * arcs that are "wid" wide if possible.  It is prefered that the bend pass
 * close to (prefx, prefy) and is on portproto "fromppt" (if "fromppt" is not
 * NOPORTPROTO) on the FROM nodeinst and on portproto "toppt" (if "toppt" is
 * not NOPORTPROTO) on the TO nodeinst.  If "fakecoords" is zero, create the arc
 * if possible.  If "fakecoords" is nonzero, do not create an arc, but just store
 * the coordinates of the two endpoints in the four integers there (and return any
 * non-NOARCINST value).  If the arcinst is created, the
 * routine returns its address and the second arc's address is returned in
 * "alt".  If no arc can be created, the routine returns NOARCINST.
 */
ARCINST *us_onebend(NODEINST *fromnodeinst, PORTPROTO *fromppt, ARCPROTO *fromarcproto,
	NODEINST *tonodeinst, PORTPROTO *toppt, ARCPROTO *toarcproto, NODEPROTO *con,
	INTBIG prefx, INTBIG prefy, INTBIG fwid, INTBIG twid, ARCINST **alt, NODEINST **con1,
	INTBIG *fakecoords, BOOLEAN report)
{
	REGISTER NODEINST *ni;
	REGISTER PORTPROTO *fpt, *tpt;
	PORTPROTO *fromportproto, *toportproto;
	REGISTER INTBIG i, bad, frange, trange;
	REGISTER INTBIG lx, hx, ly, hy, frwid, trwid, edgealignment;
	REGISTER ARCINST *ai1, *ai2;
	static POLYGON *poly = NOPOLYGON;
	INTBIG fx, fy, tx, ty, xi, yi, bestxi, bestyi, bestdist, dist,
		bestfx, bestfy, besttx, bestty, altxi, altyi, otheralign, pxs, pys;
	BOOLEAN found;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* determine true width */
	fwid = us_figurearcwidth(fromarcproto, fwid, fromnodeinst, fromppt, prefx, prefy, tonodeinst, toppt, prefx, prefy);
	frwid = fwid - arcprotowidthoffset(fromarcproto);
	twid = us_figurearcwidth(toarcproto, twid, fromnodeinst, fromppt, prefx, prefy, tonodeinst, toppt, prefx, prefy);
	trwid = twid - arcprotowidthoffset(toarcproto);

	found = FALSE;    bestdist = MAXINTBIG;
	for(fpt = fromnodeinst->proto->firstportproto; fpt != NOPORTPROTO; fpt = fpt->nextportproto)
	{
		if (fromppt != NOPORTPROTO && fromppt != fpt) continue;

		/* see if the port has an opening for this type of arcinst */
		for(i=0; fpt->connects[i] != NOARCPROTO; i++)
			if (fromarcproto == fpt->connects[i]) break;
		if (fpt->connects[i] == NOARCPROTO) continue;

		/* potential port: get information about its position */
		if ((fpt->userbits&PORTISOLATED) != 0)
		{
			(void)us_portdistance(fromnodeinst, fpt, prefx, prefy, &fx, &fy, frwid, TRUE);
		} else
		{
			us_portposition(fromnodeinst, fpt, &fx, &fy);
		}

		for(tpt = tonodeinst->proto->firstportproto; tpt != NOPORTPROTO; tpt = tpt->nextportproto)
		{
			if (toppt != NOPORTPROTO && toppt != tpt) continue;

			/* should not run arcinst from port to itself */
			if (fromnodeinst == tonodeinst && fpt == tpt) continue;

			/* see if the port has an opening for this type of arcinst */
			for(i=0; tpt->connects[i] != NOARCPROTO; i++)
				if (toarcproto == tpt->connects[i]) break;
			if (tpt->connects[i] == NOARCPROTO) continue;

			/* potential portinst: get information about its position */
			if ((tpt->userbits&PORTISOLATED) != 0)
			{
				(void)us_portdistance(tonodeinst, tpt, prefx, prefy, &tx, &ty, trwid, TRUE);
			} else
			{
				us_portposition(tonodeinst, tpt, &tx, &ty);
			}

			if (abs(tx-prefx) + abs(fy-prefy) < abs(fx-prefx) + abs(ty-prefy))
			{
				xi = tx;  yi = fy;   altxi = fx;   altyi = ty;
			} else
			{
				xi = fx;  yi = ty;   altxi = tx;   altyi = fy;
			}

			/* see if port angles are correct */
			bad = 0;
			trange = ((tpt->userbits&PORTARANGE) >> PORTARANGESH) * 10;
			if (trange != 1800)
			{
				if (tx == xi && ty == yi) bad++; else
				{
					i = figureangle(tx, ty, xi, yi);
					if (us_fitportangle(tonodeinst, tpt, i, trange)) bad++;
				}
			}
			frange = ((fpt->userbits&PORTARANGE) >> PORTARANGESH) * 10;
			if (frange != 1800)
			{
				if (fx == xi && fy == yi) bad++; else
				{
					i = figureangle(fx, fy, xi, yi);
					if (us_fitportangle(fromnodeinst, fpt, i, frange)) bad++;
				}
			}

			/* if port angles are wrong, try the other inflexion point */
			if (bad != 0)
			{
				if (tx == altxi && ty == altyi) continue;
				i = figureangle(tx, ty, altxi, altyi);
				if (us_fitportangle(tonodeinst, tpt, i, trange)) continue;
				if (fx == altxi && fy == altyi) continue;
				i = figureangle(fx, fy, altxi, altyi);
				if (us_fitportangle(fromnodeinst, fpt, i, frange)) continue;
				xi = altxi;   yi = altyi;
			}

			/* see if this path is better than any previous ones */
			dist = abs(fx-tx) + abs(fy-ty);
			if (dist > bestdist) continue;

			/* select this path */
			found = TRUE;          bestdist = dist;
			fromportproto = fpt;   toportproto = tpt;
			bestxi = xi;           bestyi = yi;
			bestfx = fx;           bestfy = fy;
			besttx = tx;           bestty = ty;
		}
	}

	/* make one-bend arcinst */
	if (found)
	{
		/* handle edge alignment */
		if (us_edgealignment_ratio != 0)
		{
			edgealignment = muldiv(us_edgealignment_ratio, WHOLE, el_curlib->lambda[el_curtech->techindex]);
			if (bestfx == bestxi)
			{
				/* see if "bestxi" and "bestfx" can be aligned */
				i = us_alignvalue(bestfx - frwid/2, edgealignment, &otheralign) + frwid/2;
				otheralign += frwid/2;
				shapeportpoly(fromnodeinst, fromportproto, poly, FALSE);
				if (isinside(i, bestfy, poly)) bestfx = bestxi = i; else
					if (isinside(otheralign, bestfy, poly)) bestfx = bestxi = otheralign;

				/* see if "bestyi" and "bestty" can be aligned */
				i = us_alignvalue(bestty - trwid/2, edgealignment, &otheralign) + trwid/2;
				otheralign += trwid/2;
				shapeportpoly(tonodeinst, toportproto, poly, FALSE);
				if (isinside(besttx, i, poly)) bestty = bestyi = i; else
					if (isinside(besttx, otheralign, poly)) bestty = bestyi = otheralign;
			} else if (bestfy == bestyi)
			{
				/* see if "bestyi" and "bestfy" can be aligned */
				i = us_alignvalue(bestfy - frwid/2, edgealignment, &otheralign) + frwid/2;
				otheralign += frwid/2;
				shapeportpoly(fromnodeinst, fromportproto, poly, FALSE);
				if (isinside(bestfx, i, poly)) bestfy = bestyi = i; else
					if (isinside(bestfx, otheralign, poly)) bestfy = bestyi = otheralign;

				/* see if "bestxi" and "besttx" can be aligned */
				i = us_alignvalue(besttx - trwid/2, edgealignment, &otheralign) + trwid/2;
				otheralign += trwid/2;
				shapeportpoly(tonodeinst, toportproto, poly, FALSE);
				if (isinside(i, bestty, poly)) besttx = bestxi = i; else
					if (isinside(otheralign, bestty, poly)) besttx = bestxi = otheralign;
			}
		}

		/* run the connecting arcs */
		if (fakecoords == 0)
		{
			/* create the intermediate nodeinst */
			us_getproperconsize(con, fromarcproto, fwid, toarcproto, twid, &pxs, &pys);
			lx = bestxi - pxs/2;   hx = lx + pxs;
			ly = bestyi - pys/2;   hy = ly + pys;
			*con1 = ni = newnodeinst(con, lx, hx, ly, hy, 0, 0, fromnodeinst->parent);
			if (ni == NONODEINST)
			{
				us_abortcommand(_("Cannot create connecting pin"));
				return(NOARCINST);
			}
			endobjectchange((INTBIG)ni, VNODEINST);

			fpt = con->firstportproto;
			ai1 = us_runarcinst(fromnodeinst, fromportproto, bestfx, bestfy, ni, fpt,
				bestxi, bestyi, fromarcproto, fwid);
			if (ai1 == NOARCINST) return(NOARCINST);
			ai2 = us_runarcinst(ni, fpt, bestxi, bestyi, tonodeinst, toportproto,
				besttx, bestty, toarcproto, twid);
			if (ai2 == NOARCINST) return(NOARCINST);
			if (report)
			{
				if (fromarcproto == toarcproto)
				{
					us_reportarcscreated(0, 2, fromarcproto->protoname, 0, x_(""));
				} else
				{
					us_reportarcscreated(0, 1, fromarcproto->protoname, 1, toarcproto->protoname);
				}
			}
			if (abs(bestfx-bestxi) + abs(bestfy-bestyi) >
				abs(bestxi-besttx) + abs(bestyi-bestty))
			{
				*alt = ai2;
				return(ai1);
			} else
			{
				*alt = ai1;
				return(ai2);
			}
		} else
		{
			fakecoords[0] = bestfx;
			fakecoords[1] = bestfy;
			fakecoords[2] = bestxi;
			fakecoords[3] = bestyi;
			fakecoords[4] = besttx;
			fakecoords[5] = bestty;
			*alt = (ARCINST *)1;
			return((ARCINST *)1);
		}
	}

	/* give up */
	return(NOARCINST);
}

/*
 * routine to check for a two-bend connection running from nodeinst
 * "fromnodeinst" to nodeinst "tonodeinst" on arcproto "typ" and create two
 * arcs that are "wid" wide if possible.  It is prefered that the jog pass
 * close to (prefx, prefy) and is on portproto "fromppt" (if "fromppt" is not
 * NOPORTPROTO) on the FROM nodeinst and on portproto "toppt" (if "toppt" is
 * not NOPORTPROTO) on the TO nodeinst.  If "fakecoords" is zero, create the arc
 * if possible.  If "fakecoords" is nonzero, do not create an arc, but just store
 * the coordinates of the two endpoints in the four integers there (and return any
 * non-NOARCINST value).  If the arcinst is created, the
 * routine returns its address and the other two arcs are returned in "alt1"
 * and "alt2" and contacts or pins are returned in "con1" and "con2".  If no arc
 * can be created, the routine returns NOARCINST.
 */
ARCINST *us_twobend(NODEINST *fromnodeinst, PORTPROTO *fromppt, ARCPROTO *fromarcproto,
	NODEINST *tonodeinst, PORTPROTO *toppt, ARCPROTO *toarcproto, NODEPROTO *con,
	INTBIG prefx, INTBIG prefy, INTBIG fwid, INTBIG twid,
	ARCINST **alt1, ARCINST **alt2, NODEINST **con1, NODEINST **con2,
	INTBIG *fakecoords, BOOLEAN report)
{
	REGISTER NODEINST *ni1, *ni2;
	REGISTER NODEPROTO *fpin;
	REGISTER PORTPROTO *fpt, *tpt;
	PORTPROTO *fromportproto, *toportproto;
	REGISTER INTBIG i, lx, hx, ly, hy, frwid, trwid;
	REGISTER ARCINST *ai;
	REGISTER BOOLEAN horizontaljog;
	INTBIG fx, fy, tx, ty, xi1, yi1, xi2, yi2, bestdist, dist,
		bestfx, bestfy, besttx, bestty, fpxs, fpys, tpxs, tpys;

	/* determine true width */
	fwid = us_figurearcwidth(fromarcproto, fwid, fromnodeinst, fromppt, prefx, prefy, tonodeinst, toppt, prefx, prefy);
	frwid = fwid - arcprotowidthoffset(fromarcproto);
	twid = us_figurearcwidth(toarcproto, twid, fromnodeinst, fromppt, prefx, prefy, tonodeinst, toppt, prefx, prefy);
	trwid = twid - arcprotowidthoffset(toarcproto);

	/* find the "from" port */
	bestdist = MAXINTBIG;   fromportproto = NOPORTPROTO;
	for(fpt = fromnodeinst->proto->firstportproto; fpt != NOPORTPROTO; fpt = fpt->nextportproto)
	{
		if (fromppt != NOPORTPROTO && fromppt != fpt) continue;

		/* see if the port has an opening for this type of arcinst */
		for(i=0; fpt->connects[i] != NOARCPROTO; i++)
			if (fromarcproto == fpt->connects[i]) break;
		if (fpt->connects[i] == NOARCPROTO) continue;

		/* potential portinst: get information about its position */
		(void)us_portdistance(fromnodeinst, fpt, prefx, prefy, &fx, &fy, frwid, TRUE);

		dist = abs(fx-prefx) + abs(fy-prefy);
		if (dist > bestdist) continue;
		fromportproto = fpt;   bestdist = dist;
		bestfx = fx;           bestfy = fy;
	}
	if (fromportproto == NOPORTPROTO) return(NOARCINST);

	/* find the "to" port */
	bestdist = MAXINTBIG;   toportproto = NOPORTPROTO;
	for(tpt = tonodeinst->proto->firstportproto; tpt != NOPORTPROTO; tpt = tpt->nextportproto)
	{
		if (toppt != NOPORTPROTO && toppt != tpt) continue;

		/* see if the port has an opening for this type of arcinst */
		for(i=0; tpt->connects[i] != NOARCPROTO; i++)
			if (toarcproto == tpt->connects[i]) break;
		if (tpt->connects[i] == NOARCPROTO) continue;

		/* potential portinst: get information about its position */
		(void)us_portdistance(tonodeinst, tpt, prefx, prefy, &tx, &ty, trwid, TRUE);

		dist = abs(tx-prefx) + abs(ty-prefy);
		if (dist > bestdist) continue;
		toportproto = tpt;   bestdist = dist;
		besttx = tx;         bestty = ty;
	}
	if (toportproto == NOPORTPROTO) return(NOARCINST);

	/*
	 * figure out whether the jog will run horizontally or vertically.
	 * Use directionality constraints if they exist
	 */
	if (((fromportproto->userbits&PORTARANGE) >> PORTARANGESH) != 180)
	{
		i = us_bottomrecurse(fromnodeinst, fromportproto);
	} else if (((toportproto->userbits&PORTARANGE) >> PORTARANGESH) != 180)
	{
		i = us_bottomrecurse(tonodeinst, toportproto);
	} else if ((prefy > bestfy && prefy > bestty) || (prefy < bestfy && prefy < bestty))
	{
		/* jog is horizontal if prefy is above or below both ports */
		i = 900;
	} else if ((prefx > bestfx && prefx > besttx) || (prefx < bestfx && prefx < besttx))
	{
		/* jog is vertical if prefx is to right or left of both ports */
		i = 0;
	} else
	{
		/* if area between nodes is wider than tall, jog is vertical */
		if (abs(bestfx-besttx) > abs(bestfy-bestty)) i = 0; else i = 900;
	}
	i = (i+450) % 1800;   if (i < 0) i += 1800;
	gridalign(&prefx, &prefx, 1, fromnodeinst->parent);
	if (i < 900)
	{
		if (bestty == bestfy) horizontaljog = TRUE; else
			horizontaljog = FALSE;
	} else
	{
		if (besttx == bestfx) horizontaljog = FALSE; else
			horizontaljog = TRUE;
	}
	if (horizontaljog)
	{
		xi1 = bestfx;   xi2 = besttx;
		yi1 = yi2 = prefy;
	} else
	{
		xi1 = xi2 = prefx;
		yi1 = bestfy;   yi2 = bestty;
	}

	/* figure out what primitive nodeproto connects these arcs */
	fpin = getpinproto(fromarcproto);
	if (fpin == NONODEPROTO)
	{
		us_abortcommand(_("No pin for %s arcs"), describearcproto(fromarcproto));
		return(NOARCINST);
	}
	defaultnodesize(fpin, &fpxs, &fpys);
	us_getproperconsize(con, fromarcproto, fwid, toarcproto, twid, &tpxs, &tpys);

	/* run the arcs */
	if (fakecoords == 0)
	{
		/* create the intermediate nodeinsts */
		lx = xi1 - fpxs/2;   hx = lx + fpxs;
		ly = yi1 - fpys/2;   hy = ly + fpys;
		*con1 = ni1 = newnodeinst(fpin, lx,hx, ly,hy, 0, 0, fromnodeinst->parent);
		if (ni1 == NONODEINST)
		{
			us_abortcommand(_("Cannot create first connecting pin"));
			return(NOARCINST);
		}
		endobjectchange((INTBIG)ni1, VNODEINST);
		lx = xi2 - tpxs/2;   hx = lx + tpxs;
		ly = yi2 - tpys/2;   hy = ly + tpys;
		*con2 = ni2 = newnodeinst(con, lx,hx, ly,hy, 0, 0, fromnodeinst->parent);
		if (ni2 == NONODEINST)
		{
			us_abortcommand(_("Cannot create second connecting pin"));
			return(NOARCINST);
		}
		endobjectchange((INTBIG)ni2, VNODEINST);

		*alt1 = us_runarcinst(fromnodeinst, fromportproto, bestfx, bestfy, ni1,
			fpin->firstportproto, xi1,yi1, fromarcproto, fwid);
		if (*alt1 == NOARCINST) return(NOARCINST);
		ai = us_runarcinst(ni1,fpin->firstportproto, xi1,yi1, ni2,con->firstportproto,
			xi2,yi2, fromarcproto, fwid);
		if (ai == NOARCINST) return(NOARCINST);
		*alt2 = us_runarcinst(ni2,con->firstportproto, xi2,yi2, tonodeinst,
			toportproto, besttx, bestty, toarcproto, twid);
		if (*alt2 == NOARCINST) return(NOARCINST);
		if (report)
		{
			if (fromarcproto == toarcproto)
			{
				us_reportarcscreated(0, 3, fromarcproto->protoname, 0, x_(""));
			} else
			{
				us_reportarcscreated(0, 2, fromarcproto->protoname, 1, toarcproto->protoname);
			}
		}
		return(ai);
	}

	/* record the fake */
	fakecoords[0] = bestfx;
	fakecoords[1] = bestfy;
	fakecoords[2] = xi1;
	fakecoords[3] = yi1;
	fakecoords[4] = xi2;
	fakecoords[5] = yi2;
	fakecoords[6] = besttx;
	fakecoords[7] = bestty;
	*alt1 = (ARCINST *)1;
	*alt2 = (ARCINST *)1;
	return((ARCINST *)1);
}

/*
 * run an arcinst from portproto "fromportproto" of nodeinst "fromnodeinst" at
 * (fromx,fromy) to portproto "toportproto" of nodeinst "tonodeinst" at
 * (tox,toy).  The type of the arcinst is "ap" and the width is "wid".  The
 * routine returns the address of the newly created arcinst (NOARCINST on
 * error).
 */
ARCINST *us_runarcinst(NODEINST *fromnodeinst, PORTPROTO *fromportproto, INTBIG fromx, INTBIG fromy,
	NODEINST *tonodeinst, PORTPROTO *toportproto, INTBIG tox, INTBIG toy, ARCPROTO *ap, INTBIG wid)
{
	REGISTER ARCINST *ai;
	REGISTER INTBIG bits;

	/* see if nodes need to be undrawn to account for "Steiner Point" changes */
	if ((fromnodeinst->proto->userbits&WIPEON1OR2) != 0)
		startobjectchange((INTBIG)fromnodeinst, VNODEINST);
	if ((tonodeinst->proto->userbits&WIPEON1OR2) != 0)
		startobjectchange((INTBIG)tonodeinst, VNODEINST);

	/* create the arcinst */
	bits = us_makearcuserbits(ap);
	ai = newarcinst(ap, wid, bits, fromnodeinst, fromportproto, fromx,fromy,
		tonodeinst, toportproto, tox, toy, fromnodeinst->parent);
	if (ai == NOARCINST)
	{
		us_abortcommand(_("Problem creating the arc"));
		return(NOARCINST);
	}
	ai->changed = 0;
	endobjectchange((INTBIG)ai, VARCINST);
	us_setarcproto(ap, FALSE);

	/* see if nodes need to be redrawn to account for "Steiner Point" changes */
	if ((fromnodeinst->proto->userbits&WIPEON1OR2) != 0)
		endobjectchange((INTBIG)fromnodeinst, VNODEINST);
	if ((tonodeinst->proto->userbits&WIPEON1OR2) != 0)
		endobjectchange((INTBIG)tonodeinst, VNODEINST);
	return(ai);
}

/*
 * routine to find the width of the widest arcinst of type "ap" connected
 * to any port of nodeinst "ni" (if "ni" is primitive) or to port "por" of
 * nodeinst "ni" (if "ni" is complex).
 */
INTBIG us_widestarcinst(ARCPROTO *ap, NODEINST *ni, PORTPROTO *por)
{
	REGISTER INTBIG wid, pindex;
	REGISTER PORTARCINST *pi;

	/* look at all arcs on the nodeinst */
	wid = 0;
	pindex = ni->proto->primindex;
	if (por == NOPORTPROTO) pindex = 1;
	for(;;)
	{
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			if (pindex == 0 && pi->proto != por) continue;
			if (ap != NOARCPROTO && pi->conarcinst->proto != ap) continue;
			if (pi->conarcinst->width > wid) wid = pi->conarcinst->width;
		}

		/* descend to the next level in the hierarchy */
		if (pindex != 0) break;
		ni = por->subnodeinst;
		pindex = ni->proto->primindex;
		por = por->subportproto;
	}
	return(wid);
}

/*
 * Routine to determine the proper width of arc of type "typ" with width "wid" and running from node
 * "ni1", port "pp1" to node "ni2", port "pp2".  Oversize nodes are considered when sizing the arc.
 */
INTBIG us_stretchtonodes(ARCPROTO *typ, INTBIG wid, NODEINST *ni1, PORTPROTO *pp1, INTBIG otherx, INTBIG othery)
{
#if 0		/* this turns out to be a bad idea.  So it is turned off. */
	REGISTER INTBIG xstretch, ystretch, i, rot, cx, cy;
	INTBIG ni1x, ni1y;

	/* see if node 1 is stretched */
	xstretch = (ni1->highx - ni1->lowx) - (ni1->proto->highx - ni1->proto->lowx);
	ystretch = (ni1->highy - ni1->lowy) - (ni1->proto->highy - ni1->proto->lowy);
	if (xstretch > 0 || ystretch > 0)
	{
		rot = (ni1->rotation + 900 * ni1->transpose) % 3600;
		switch (rot)
		{
			case 0:
			case 1800:
				break;
			case 900:
			case 2700:
				i = xstretch;   xstretch = ystretch;   ystretch = i;
				break;
			default:
				return(wid);
		}
		us_portposition(ni1, pp1, &ni1x, &ni1y);
		cx = (ni1->lowx + ni1->highx) / 2;
		cy = (ni1->lowy + ni1->highy) / 2;
		if (ni1x == cx && ni1y == cy)
		{
			if (abs(ni1x-otherx) > abs(ni1y-othery))
			{
				/* horizontal wire: see if Y stretch allows growth */
				if (ystretch > wid - typ->nominalwidth) wid = ystretch + typ->nominalwidth;
			} else
			{
				/* vertical wire: see if X stretch allows growth */
				if (xstretch > wid - typ->nominalwidth) wid = xstretch + typ->nominalwidth;
			}
		} else
		{
			if (abs(ni1x-cx) > abs(ni1y-cy))
			{
				/* horizontal wire: see if Y stretch allows growth */
				if (ystretch > wid - typ->nominalwidth) wid = ystretch + typ->nominalwidth;
			} else
			{
				/* vertical wire: see if X stretch allows growth */
				if (xstretch > wid - typ->nominalwidth) wid = xstretch + typ->nominalwidth;
			}
		}
	}
#endif
	return(wid);
}

/*
 * Routine to determine the width to use for arc "typ", given a default width of "wid", that it
 * runs from "ni1/pp1" towards (ox1,oy1) and to "ni2/pp2" towards (ox2,oy2).  If either end is
 * a pin, the width calculation is not performed for it.
 */
INTBIG us_figurearcwidth(ARCPROTO *typ, INTBIG wid, NODEINST *ni1, PORTPROTO *pp1, INTBIG ox1, INTBIG oy1,
	NODEINST *ni2, PORTPROTO *pp2, INTBIG ox2, INTBIG oy2)
{
	INTBIG wid1, wid2, stretchwid, stretch1, stretch2, fun;
	REGISTER NODEINST *rni;
	REGISTER PORTPROTO *rpp;

	stretch1 = stretch2 = 1;

	/* see if node 1 is a pin */
	rni = ni1;   rpp = pp1;
	while (rni->proto->primindex == 0)
	{
		rni = rpp->subnodeinst;
		rpp = rpp->subportproto;
	}
	fun = nodefunction(rni);
	if (fun == NPPIN) stretch1 = 0;

	/* see if node 2 is a pin */
	rni = ni2;   rpp = pp2;
	while (rni->proto->primindex == 0)
	{
		rni = rpp->subnodeinst;
		rpp = rpp->subportproto;
	}
	fun = nodefunction(rni);
	if (fun == NPPIN) stretch2 = 0;

	if (stretch1 == 0 && stretch2 == 0) return(wid);

	wid1 = us_stretchtonodes(typ, wid, ni1, pp1, ox1, oy1);
	wid2 = us_stretchtonodes(typ, wid, ni2, pp2, ox2, oy2);
	if (stretch1 == 0) wid1 = wid2;
	if (stretch2 == 0) wid2 = wid1;
	stretchwid = mini(wid1, wid2);
	if (stretchwid > wid) wid = stretchwid;
	return(wid);
}

/*
 * routine to determine the nodeinst to be used when connecting the geometry
 * module pointed to by "ipos" and the geometry module in "othergeom".
 * The port prototype on the other object ("othergeom") is in "otherport" and
 * the port prototype on the object in "ipos" is in "ipp".  The prefered site
 * of connection is in (prefx, prefy).  If the first module (ipos) is an
 * arcinst, it may be split into two arcs and a nodeinst in which case that
 * nodeinst will be returned and the address of the geometry module will be
 * changed to point to that nodeinst.  If "fake" is nonzero, the newly created
 * node will be a fake one (not really created) for the purposes of determining
 * an intended connection only.
 */
NODEINST *us_getnodeonarcinst(GEOM **ipos, PORTPROTO **ipp, GEOM *othergeom,
	PORTPROTO *otherport, INTBIG prefx, INTBIG prefy, INTBIG fake)
{
	REGISTER ARCINST *ai, *oar;
	ARCINST *ai1, *ai2;
	REGISTER ARCPROTO *ap;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER GEOM *geom;
	static NODEINST node;
	REGISTER INTBIG w, wid, owid, lx, hx, ly, hy, delta, fun;
	INTBIG pxs, pys;

	/* get the actual geometry modules */
	geom = *ipos;

	/* if the module is a nodeinst, return it */
	if (geom->entryisnode) return(geom->entryaddr.ni);

	/* if the other module is a node use it to determine the break point */
	if (othergeom->entryisnode)
	{
		ni = othergeom->entryaddr.ni;

		/* if the other module has a port specified, use port center as break point */
		if (othergeom->entryisnode && otherport != NOPORTPROTO)
		{
			portposition(ni, otherport, &prefx, &prefy);
		} else
		{
			/* if the other module is a primitive node, use center as break point */
			if (ni->proto->primindex != 0)
			{
				prefx = (ni->lowx + ni->highx) / 2;
				prefy = (ni->lowy + ni->highy) / 2;
			}
		}
	}

	/* find point on this arcinst closest to break point */
	ai = geom->entryaddr.ai;
	if (ai->end[0].xpos == ai->end[1].xpos)
	{
		/* vertical arcinst */
		if (!othergeom->entryisnode)
		{
			/* if two arcs are perpendicular, find point of intersection */
			oar = othergeom->entryaddr.ai;
			if (oar->end[0].ypos == oar->end[1].ypos) prefy = oar->end[0].ypos;
		}
	} else if (ai->end[0].ypos == ai->end[1].ypos)
	{
		/* horizontal arcinst */
		if (!othergeom->entryisnode)
		{
			/* if two arcs are perpendicular, find point of intersection */
			oar = othergeom->entryaddr.ai;
			if (oar->end[0].xpos == oar->end[1].xpos) prefx = oar->end[0].xpos;
		}
	}

	/* adjust to closest point */
	(void)closestpointtosegment(ai->end[0].xpos, ai->end[0].ypos,
		ai->end[1].xpos, ai->end[1].ypos, &prefx, &prefy);
	if (prefx == ai->end[0].xpos && prefy == ai->end[0].ypos)
	{
		*ipp = ai->end[0].portarcinst->proto;
		*ipos = ai->end[0].nodeinst->geom;
		return(ai->end[0].nodeinst);
	}
	if (prefx == ai->end[1].xpos && prefy == ai->end[1].ypos)
	{
		*ipp = ai->end[1].portarcinst->proto;
		*ipos = ai->end[1].nodeinst->geom;
		return(ai->end[1].nodeinst);
	}

	/* create the splitting pin */
	ap = ai->proto;
	np = us_findconnectingpin(ap, othergeom, otherport);
	if (np == NONODEPROTO) return(NONODEINST);

	/* determine width of other arc */
	wid = ai->width;
	us_getproperconsize(np, ap, wid, ap, wid, &pxs, &pys);

	/* make contact asymmetric if other arc is wider */
	fun = (np->userbits&NFUNCTION) >> NFUNCTIONSH;
	if (fun != NPPIN)
	{
		owid = wid;
		if (othergeom->entryisnode)
		{
			w = us_widestarcinst(ap, othergeom->entryaddr.ni, otherport);
			if (w > owid) owid = w;
		} else
		{
			if (othergeom->entryaddr.ai->width > owid)
				owid = othergeom->entryaddr.ai->width;
		}
		if (owid > ai->width)
		{
			delta = owid - ai->width;
			if (ai->end[0].xpos == ai->end[1].xpos)
			{
				/* vertical arc, make contact larger in Y */
				pys += delta;
			} else if (ai->end[0].ypos == ai->end[1].ypos)
			{
				/* horizontal arc, make contact larger in x */
				pxs += delta;
			}
		}
	}

	/* create the contact */
	lx = prefx - pxs/2;   hx = lx + pxs;
	ly = prefy - pys/2;   hy = ly + pys;
	if (fake != 0)
	{
		ni = &node;   initdummynode(ni);
		ni->lowx = lx;   ni->highx = hx;
		ni->lowy = ly;   ni->highy = hy;
		ni->proto = np;
	} else
	{
		ni = newnodeinst(np, lx, hx, ly, hy, 0, 0, ai->parent);
		if (ni == NONODEINST)
		{
			ttyputerr(_("Cannot create splitting pin"));
			return(NONODEINST);
		}
		endobjectchange((INTBIG)ni, VNODEINST);

		/* create the two new arcinsts */
		(void)us_breakarcinsertnode(ai, ni, &ai1, &ai2);
	}

	/* return pointers to the splitting pin */
	*ipos = ni->geom;
	*ipp = ni->proto->firstportproto;
	return(ni);
}

/*
 * Routine to break arc "ai" at node "ni".  Sets the two new halves in "ai1" and "ai2".
 * Returns TRUE on error.
 */
BOOLEAN us_breakarcinsertnode(ARCINST *ai, NODEINST *ni, ARCINST **ai1, ARCINST **ai2)
{
	REGISTER NODEINST *fno, *tno;
	REGISTER ARCINST *mainarc;
	REGISTER PORTPROTO *fpt, *tpt, *pp;
	REGISTER NODEPROTO *parent;
	REGISTER INTBIG fendx, fendy, tendx, tendy, bits1, bits2, wid;
	INTBIG x, y;
	REGISTER ARCPROTO *ap;

	fno = ai->end[0].nodeinst;    fpt = ai->end[0].portarcinst->proto;
	tno = ai->end[1].nodeinst;    tpt = ai->end[1].portarcinst->proto;
	fendx = ai->end[0].xpos;      fendy = ai->end[0].ypos;
	tendx = ai->end[1].xpos;      tendy = ai->end[1].ypos;
	ap = ai->proto;   wid = ai->width;  parent = ai->parent;
	pp = ni->proto->firstportproto;
	portposition(ni, pp, &x, &y);
	bits1 = bits2 = ai->userbits;
	if ((bits1&ISNEGATED) != 0)
	{
		if ((bits1&REVERSEEND) == 0) bits2 &= ~ISNEGATED; else
			bits1 &= ~ISNEGATED;
	}
	if (figureangle(fendx, fendy, x, y) != figureangle(x, y, tendx, tendy))
	{
		bits1 &= ~FIXANG;
		bits2 &= ~FIXANG;
	}

	/* create the two new arcinsts */
	*ai1 = newarcinst(ap, wid, bits1, fno, fpt, fendx, fendy, ni, pp, x, y, parent);
	*ai2 = newarcinst(ap, wid, bits2, ni, pp, x, y, tno, tpt, tendx, tendy, parent);
	if (*ai1 == NOARCINST || *ai2 == NOARCINST)
	{
		ttyputerr(_("Error creating the split arc parts"));
		return(TRUE);
	}

	/* figure out on which half of the arc to place former arc information */
	mainarc = *ai1;
	if (fno->proto->primindex == 0 && tno->proto->primindex != 0)
		mainarc = *ai2; else
	{
		if (computedistance(fendx, fendy, x, y) <
			computedistance(tendx, tendy, x, y)) mainarc = *ai2;
	}

	(void)copyvars((INTBIG)ai, VARCINST, (INTBIG)mainarc, VARCINST, FALSE);
	endobjectchange((INTBIG)*ai1, VARCINST);
	endobjectchange((INTBIG)*ai2, VARCINST);

	/* delete the old arcinst */
	startobjectchange((INTBIG)ai, VARCINST);
	if (killarcinst(ai))
	{
		ttyputerr(_("Error deleting original arc"));
		return(TRUE);
	}
	return(FALSE);
}

/*
 * Routine to find the connecting node that can join arcs of type "ap" with
 * an object "geom" (which may be a node with port "pp").  Returns NONODEPROTO
 * if no connecting node can be found.
 */
NODEPROTO *us_findconnectingpin(ARCPROTO *ap, GEOM *geom, PORTPROTO *pp)
{
	REGISTER NODEPROTO *np, *niproto;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *firstpp;
	REGISTER TECHNOLOGY *tech;
	REGISTER INTBIG fun, i, j;

	/* first presume the pin that connects this type of arc */
	np = getpinproto(ap);

	/* if there is no other object, use this pin */
	if (geom == NOGEOM) return(np);

	/* ensure that it connects to this */
	if (geom->entryisnode)
	{
		if (pp == NOPORTPROTO)
		{
			niproto = geom->entryaddr.ni->proto;
			pp = niproto->firstportproto;
			if (pp == NOPORTPROTO || pp->nextportproto != NOPORTPROTO)
				return(np);
		}
		for(i=0; pp->connects[i] != NOARCPROTO; i++)
		{
			if (pp->connects[i] == ap) break;

			/* special case: bus arc can connect to a node with a wire-pin port */
			if (ap == sch_busarc && pp->connects[i] == sch_wirearc) break;
		}
		if (pp->connects[i] == NOARCPROTO) np = NONODEPROTO;
	} else
	{
		ai = geom->entryaddr.ai;
		if (ai->proto != ap) np = NONODEPROTO;
	}
	if (np == NONODEPROTO)
	{
		/* doesn't connect: look for a contact */
		tech = ap->tech;
		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			fun = (np->userbits&NFUNCTION) >> NFUNCTIONSH;
			if (fun != NPCONTACT) continue;
			firstpp = np->firstportproto;

			/* make sure the original arc connects to this contact */
			for(i=0; firstpp->connects[i] != NOARCPROTO; i++)
				if (firstpp->connects[i] == ap) break;
			if (firstpp->connects[i] == NOARCPROTO) continue;

			/* make sure the other object connects to this contact */
			for(i=0; firstpp->connects[i] != NOARCPROTO; i++)
			{
				if (firstpp->connects[i]->tech != tech) continue;
				if (geom->entryisnode)
				{
					for(j=0; pp->connects[j] != NOARCPROTO; j++)
						if (pp->connects[j] == firstpp->connects[i]) break;
					if (pp->connects[j] != NOARCPROTO) break;
				} else
				{
					ai = geom->entryaddr.ai;
					if (ai->proto == firstpp->connects[i]) break;
				}
			}
			if (firstpp->connects[i] != NOARCPROTO) break;
		}
	}
	return(np);
}

/*
 * routine to report the distance of point (prefx,prefy) to port "pt" of node
 * instance "ni".  The closest point on the polygon is returned in (x,y),
 * given that the port will connect to an arc with width "wid".  If "purpose"
 * is true, a new sub-port location within the port is desired from
 * the "shapeportpoly" routine.  Euclidean distance is not always used, but
 * at least the metric is consistent with itself.
 */
INTBIG us_portdistance(NODEINST *ni, PORTPROTO *pt, INTBIG prefx, INTBIG prefy, INTBIG *x,
	INTBIG *y, INTBIG wid, BOOLEAN purpose)
{
	static POLYGON *poly = NOPOLYGON;
	REGISTER INTBIG i, j;
	REGISTER INTBIG bestdist, px, py, nx, ny;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* specify prefered location of new port */
	poly->xv[0] = prefx;   poly->yv[0] = prefy;   poly->count = 1;
	shapeportpoly(ni, pt, poly, purpose);
	switch (poly->style)
	{
		case FILLED:
			/* reduce the port area to the proper amount for this arc */
			reduceportpoly(poly, ni, pt, wid, -1);

			/* for filled polygon, see if point is inside */
			if (isinside(prefx, prefy, poly))
			{
				*x = prefx;   *y = prefy;
				return(0);
			}

			*x = prefx;   *y = prefy;
			closestpoint(poly, x, y);
			return(0);

		case OPENED:
		case CLOSED:
			/* for OPENED/CLOSED polygons look for proximity to a vertex */
			bestdist = abs(poly->xv[0] - prefx) + abs(poly->yv[0] - prefy);
			*x = poly->xv[0];   *y = poly->yv[0];
			for(j=1; j<poly->count; j++)
			{
				i = abs(poly->xv[j] - prefx) + abs(poly->yv[j] - prefy);
				if (i < bestdist)
				{
					bestdist = i;
					*x = poly->xv[j];   *y = poly->yv[j];
				}
			}

			/* additionally, look for proximity to an edge */
			for(j=0; j<poly->count; j++)
			{
				if (j == 0)
				{
					if (poly->style == OPENED) continue;
					px = poly->xv[poly->count-1];
					py = poly->yv[poly->count-1];
				} else
				{
					px = poly->xv[j-1];
					py = poly->yv[j-1];
				}
				nx = poly->xv[j];
				ny = poly->yv[j];

				/* handle vertical line that is perpendicular to point */
				if (px == nx && maxi(py, ny) >= prefy && mini(py, ny) <= prefy &&
					abs(px - prefx) < bestdist)
				{
					bestdist = i;
					*x = px;
					*y = prefy;
				}

				/* handle horizontal line that is perpendicular to point */
				if (py == ny && maxi(px, nx) >= prefx && mini(px, nx) <= prefx &&
					abs(py - prefy) < bestdist)
				{
					bestdist = i;
					*x = prefx;
					*y = py;
				}
			}
			return(bestdist);
	}

	/* bogus answer for unusual shapes!!! */
	return(0);
}

/*
 * Routine to determine the size of contact "con", used when connecting arcs "fromap" that
 * are "fwid" wide and "toap" that are "twid" wide.  The size is put in (pxs,pys).
 */
void us_getproperconsize(NODEPROTO *con, ARCPROTO *fromap, INTBIG fwid,
	ARCPROTO *toap, INTBIG twid, INTBIG *pxs, INTBIG *pys)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER INTBIG truefwid, truetwid, fromlayer, tolayer, tot, i, fromdiff, todiff,
		size, growth;
	INTBIG lx, hx, ly, hy;
	NODEINST node;
	ARCINST arc;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* determine width and layer of each arc */
	ai = &arc;   initdummyarc(ai);
	ai->proto = fromap;
	ai->width = fromap->nominalwidth;
	ai->end[0].xpos = -1000000;   ai->end[0].ypos = 0;
	ai->end[1].xpos =  1000000;   ai->end[1].ypos = 0;
	tot = arcpolys(ai, NOWINDOWPART);
	shapearcpoly(ai, 0, poly);
	fromlayer = poly->layer;
	truefwid = fwid - arcprotowidthoffset(fromap);

	ai->proto = toap;
	ai->width = toap->nominalwidth;
	ai->end[0].xpos = -1000000;   ai->end[0].ypos = 0;
	ai->end[1].xpos =  1000000;   ai->end[1].ypos = 0;
	tot = arcpolys(ai, NOWINDOWPART);
	shapearcpoly(ai, 0, poly);
	tolayer = poly->layer;
	truetwid = twid - arcprotowidthoffset(toap);

	ni = &node;   initdummynode(ni);
	ni->proto = con;
	ni->lowx = con->lowx;   ni->highx = con->highx;
	ni->lowy = con->lowy;   ni->highy = con->highy;
	tot = nodepolys(ni, 0, NOWINDOWPART);
	fromdiff = todiff = 0;
	for(i=0; i<tot; i++)
	{
		shapenodepoly(ni, i, poly);
		if (poly->layer == fromlayer)
		{
			getbbox(poly, &lx, &hx, &ly, &hy);
			size = maxi(hx-lx, hy-ly);
			fromdiff = truefwid - size;
		}
		if (poly->layer == tolayer)
		{
			getbbox(poly, &lx, &hx, &ly, &hy);
			size = maxi(hx-lx, hy-ly);
			todiff = truetwid - size;
		}
	}
	defaultnodesize(con, pxs, pys);
	growth = maxi(fromdiff, todiff);
	if (growth > 0)
	{
		*pxs += growth;
		*pys += growth;
	}
}

/*
 * Routine to find the center of node "ni", port "pp" and place it in (x,y).
 * Adjusts the coordinate to account for grid alignment.
 */
void us_portposition(NODEINST *ni, PORTPROTO *pp, INTBIG *x, INTBIG *y)
{
	INTBIG ax, ay;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);
	portposition(ni, pp, x, y);
	ax = *x;   ay = *y;
	gridalign(&ax, &ay, 1, ni->parent);
	if (ax != *x || ay != *y)
	{
		shapeportpoly(ni, pp, poly, FALSE);
		if (isinside(ax, ay, poly))
		{
			*x = ax;
			*y = ay;
		}
	}
}

/*
 * routine to determine whether port "pp" of node "ni" can connect to an
 * arc at angle "angle" within range "range".  Returns true if the
 * connection cannot be made.
 */
BOOLEAN us_fitportangle(NODEINST *ni, PORTPROTO *pp, INTBIG angle, INTBIG range)
{
	REGISTER INTBIG j;

	j = us_bottomrecurse(ni, pp);
	j = (j - angle) % 3600;   if (j < 0) j += 3600;
	if (j > 1800) j = 3600 - j;
	if (j > range) return(TRUE);
	return(FALSE);
}

/*
 * routine to recurse to the bottom (most primitive node) of a port and
 * compute the port orientation from the bottom up.
 */
INTBIG us_bottomrecurse(NODEINST *ni, PORTPROTO *pp)
{
	REGISTER INTBIG k;

	if (ni->proto->primindex == 0)
		k = us_bottomrecurse(pp->subnodeinst, pp->subportproto); else
			k = ((pp->userbits&PORTANGLE) >> PORTANGLESH) * 10;
	k += ni->rotation;
	if (ni->transpose != 0) k = 2700 - k;
	return(k);
}

/*
 * Routine to figure out the proper "cursor location" given that arcs are to be
 * drawn from "fromgeom/fromport" to "togeom/toport" and that the cursor is at
 * (xcur,ycur).  Changes (xcur,ycur) to be the proper location.
 */
BOOLEAN us_figuredrawpath(GEOM *fromgeom, PORTPROTO *fromport, GEOM *togeom, PORTPROTO *toport,
	INTBIG *xcur, INTBIG *ycur)
{
	static POLYGON *poly = NOPOLYGON, *poly2 = NOPOLYGON;
	REGISTER NODEINST *tonode, *fromnode;
	INTBIG flx, fhx, fly, fhy, tlx, tly, thx, thy, lx, hx, ly, hy, overx[2], overy[2];
	REGISTER INTBIG k, overcount, fx, fy, tx, ty, c1x, c1y, c2x, c2y,
		c1dist, c2dist, pxs, pys, dist, ox, oy, x, y;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);
	(void)needstaticpolygon(&poly2, 4, us_tool->cluster);

	if (!fromgeom->entryisnode || fromport == NOPORTPROTO)
	{
		us_abortcommand(_("Cannot connect to %s: it has no ports"),
			geomname(fromgeom));
		return(TRUE);
	}
	fromnode = fromgeom->entryaddr.ni;

	ox = *xcur;   oy = *ycur;
	gridalign(xcur, ycur, 1, fromnode->parent);

	/* user dragged over another object: connect them */
	tonode = us_getnodeonarcinst(&togeom, &toport, fromgeom,
		fromport, *xcur, *ycur, 1);
	if (tonode == NONODEINST)
	{
		us_abortcommand(_("Cannot find a way to connect these objects"));
		return(TRUE);
	}
	if (toport == NOPORTPROTO)
	{
		us_abortcommand(_("Cannot connect to %s: it has no ports"),
			geomname(togeom));
		return(TRUE);
	}

	/* change cursor according to port angle */
	shapeportpoly(fromnode, fromport, poly2, FALSE);
	getbbox(poly2, &flx, &fhx, &fly, &fhy);
	shapeportpoly(tonode, toport, poly2, FALSE);
	getbbox(poly2, &tlx, &thx, &tly, &thy);
	lx = mini(flx, tlx);
	hx = maxi(fhx, thx);
	ly = mini(fly, tly);
	hy = maxi(fhy, thy);
	dist = maxi(hx-lx, hy-ly);
	overcount = 0;
	if (((fromport->userbits&PORTARANGE) >> PORTARANGESH) != 180)
	{
		k = us_bottomrecurse(fromnode, fromport) % 3600;
		if (k < 0) k += 3600;
		overx[overcount] = (flx+fhx)/2 + mult(cosine(k), dist);
		overy[overcount] = (fly+fhy)/2 + mult(sine(k), dist);
		overcount++;
	}
	if (((toport->userbits&PORTARANGE) >> PORTARANGESH) != 180)
	{
		k = us_bottomrecurse(tonode, toport) % 3600;
		if (k < 0) k += 3600;
		overx[overcount] = (tlx+thx)/2 + mult(cosine(k), dist);
		overy[overcount] = (tly+thy)/2 + mult(sine(k), dist);
		overcount++;
	}
	if (overcount == 2)
	{
		pxs = (overx[0] + overx[1]) / 2;
		pys = (overy[0] + overy[1]) / 2;
	} else
	{
		pxs = (flx + fhx) / 2;
		pys = (tly + thy) / 2;
		if (overcount > 0)
		{
			pxs = overx[0];   pys = overy[0];
		} else
		{
			/* use slight cursor differences to pick a corner */
			fx = (flx+fhx)/2;
			fy = (fly+fhy)/2;
			tx = (tlx+thx)/2;
			ty = (tly+thy)/2;
			c1x = fx;   c1y = ty;
			c2x = tx;   c2y = fy;
			c1dist = computedistance(ox, oy, c1x, c1y) +
				computedistance(c1x, c1y, fx, fy);
			c2dist = computedistance(ox, oy, c2x, c2y) +
				computedistance(c2x, c2y, fx, fy);
			if (c1dist < c2dist)
			{
				pxs = c1x;   pys = c1y;
			} else
			{
				pxs = c2x;   pys = c2y;
			}
		}

		/* set coordinate in the center if the ports overlap in either axis */
		if (fromport != NOPORTPROTO)
		{
			shapeportpoly(fromnode, fromport, poly, FALSE);
			getbbox(poly, &flx, &fhx, &fly, &fhy);
			shapeportpoly(tonode, toport, poly, FALSE);
			getbbox(poly, &tlx, &thx, &tly, &thy);
			lx = maxi(flx, tlx);   hx = mini(fhx, thx);
			ly = maxi(fly, tly);   hy = mini(fhy, thy);
			if (lx <= hx || ly <= hy)
			{
				pxs = (lx + hx) / 2;
				pys = (ly + hy) / 2;
			}
		}
	}

	/* isolated ports simply use cursor location */
	if ((fromport != NOPORTPROTO &&
		(fromport->userbits&PORTISOLATED) != 0) ||
			(toport->userbits&PORTISOLATED) != 0)
	{
		us_portposition(fromnode, fromport, &lx, &ly);
		us_portposition(tonode, toport, &hx, &hy);
		if (hx < lx)
		{
			x = lx;   lx = hx;   hx = x;
		}
		if (hy < ly)
		{
			y = ly;   ly = hy;   hy = y;
		}

		/* if cursor location is within range of ports, use it */
		if (*xcur >= lx && *xcur <= hx) pxs = *xcur;
		if (*ycur >= ly && *ycur <= hy) pys = *ycur;
	}
	*xcur = pxs;
	*ycur = pys;
	return(FALSE);
}
