/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: routmimic.c
 * Arc mimic code for the wire routing tool
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

#include "config.h"
#if ROUTTOOL

#include "global.h"
#include "rout.h"
#include "usr.h"
#include "edialogs.h"
#include <math.h>

#define NOPOSSIBLEARC ((POSSIBLEARC *)-1)
#define LIKELYDIFFPORT      1
#define LIKELYDIFFARCCOUNT  2
#define LIKELYDIFFNODETYPE  4
#define LIKELYDIFFNODESIZE  8
#define LIKELYARCSSAMEDIR  16

typedef struct Ipossiblearc
{
	NODEINST            *ni1, *ni2;
	PORTPROTO           *pp1, *pp2;
	INTBIG               x1, y1, x2, y2;
	INTBIG               situation;
	struct Ipossiblearc *nextpossiblearc;
} POSSIBLEARC;

static POSSIBLEARC *ro_mimicpossiblearcfree = NOPOSSIBLEARC;

static INTBIG       ro_totalmimicports = 0;
static POLYGON    **ro_mimicportpolys;

/* prototypes for local routines */
static void         ro_mimicdelete(ARCPROTO *typ, NODEINST **nodes, PORTPROTO **ports);
static INTBIG       ro_mimiccreatedarc(ARCINST *arc);
static INTBIG       ro_mimiccreatedany(NODEINST *ni1, PORTPROTO *pp1, INTBIG x1, INTBIG y1,
						NODEINST *ni2, PORTPROTO *pp2, INTBIG x2, INTBIG y2,
						INTBIG width, ARCPROTO *proto, INTBIG prefx, INTBIG prefy);
static INTBIG       ro_mimicthis(NODEINST *ni1, PORTPROTO *pp1, INTBIG x1, INTBIG y1,
						NODEINST *ni2, PORTPROTO *pp2, INTBIG x2, INTBIG y2,
						INTBIG width, ARCPROTO *proto, INTBIG prefx, INTBIG prefy);
static POSSIBLEARC *ro_allocpossiblearc(void);
static void         ro_freepossiblearc(POSSIBLEARC *pa);

/*
 * Routine to free all memory associated with this module.
 */
void ro_freemimicmemory(void)
{
	REGISTER POSSIBLEARC *pa;
	REGISTER INTBIG i;

	while (ro_mimicpossiblearcfree != NOPOSSIBLEARC)
	{
		pa = ro_mimicpossiblearcfree;
		ro_mimicpossiblearcfree = pa->nextpossiblearc;
		efree((CHAR *)pa);
	}
	for(i=0; i<ro_totalmimicports; i++)
		freepolygon(ro_mimicportpolys[i]);
	if (ro_totalmimicports > 0) efree((CHAR *)ro_mimicportpolys);
}

/*
 * Entry point for mimic router.  Called each "slice".  If "forced" is true,
 * this mimic operation was explicitly requested.
 */
void ro_mimicstitch(BOOLEAN forced)
{
	REGISTER ARCPROTO *ap;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni;
	NODEINST *endnode[2];
	PORTPROTO *endport[2];
	INTBIG endx[2], endy[2], x0, y0, x1, y1;
	ARCPROTO *proto;
	REGISTER NODEPROTO *parent;
	REGISTER INTBIG options, i, width, foundends, e, prefx, prefy;
	float elapsed;

	options = ro_getoptions();

	/* if a single arc was deleted, and that is being mimiced, do it */
	if (ro_lastactivity.numdeletedarcs == 1 && (options&MIMICUNROUTES) != 0)
	{
		for(i=0; i<2; i++)
		{
			endnode[i] = ro_deletednodes[i];
			endport[i] = ro_deletedports[i];
		}
		ap = ro_lastactivity.deletedarcs[0]->proto;
		ro_mimicdelete(ap, endnode, endport);
		ro_lastactivity.numdeletedarcs = 0;
		return;
	}

	/* if a single arc was just created, mimic that */
	if (ro_lastactivity.numcreatedarcs == 1)
	{
		ai = ro_lastactivity.createdarcs[0];
		starttimer();
		if (ro_mimiccreatedarc(ai) == 0)
		{
			/* nothing was wired */
			if (forced) ttyputmsg(_("No wires mimiced"));
		}
		elapsed = endtimer();
		if (elapsed > 60.0 && (us_useroptions&BEEPAFTERLONGJOB) != 0)
			ttybeep(SOUNDBEEP, TRUE);
		if ((options&MIMICINTERACTIVE) == 0 && elapsed >= 2.0)
			ttyputmsg(_("Mimicing took %s"), explainduration(elapsed));
		ro_lastactivity.numcreatedarcs = 0;
		return;
	}

	/* if multiple arcs were just created, find the true end and mimic that */
	if (ro_lastactivity.numcreatedarcs > 1 && ro_lastactivity.numcreatednodes > 0)
	{
		/* find the ends of arcs that do not attach to the intermediate pins */
		parent = ro_lastactivity.createdarcs[0]->parent;
		for(ni = parent->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			ni->temp1 = 0;
		for(i=0; i<ro_lastactivity.numcreatednodes; i++)
			ro_lastactivity.creatednodes[i]->temp1 = 0;
		for(i=0; i<ro_lastactivity.numcreatedarcs; i++)
		{
			ro_lastactivity.createdarcs[i]->end[0].nodeinst->temp1++;
			ro_lastactivity.createdarcs[i]->end[1].nodeinst->temp1++;
		}
		foundends = 0;
		width = 0;
		for(i=0; i<ro_lastactivity.numcreatedarcs; i++)
		{
			for(e=0; e<2; e++)
			{
				if (ro_lastactivity.createdarcs[i]->end[e].nodeinst->temp1 != 1) continue;
				if (foundends < 2)
				{
					endnode[foundends] = ro_lastactivity.createdarcs[i]->end[e].nodeinst;
					endport[foundends] = ro_lastactivity.createdarcs[i]->end[e].portarcinst->proto;
					endx[foundends] = ro_lastactivity.createdarcs[i]->end[e].xpos;
					endy[foundends] = ro_lastactivity.createdarcs[i]->end[e].ypos;
					if (ro_lastactivity.createdarcs[i]->width > width)
						width = ro_lastactivity.createdarcs[i]->width;
					proto = ro_lastactivity.createdarcs[i]->proto;
				}
				foundends++;
			}
		}

		/* if exactly two ends are found, mimic that connection */
		if (foundends == 2)
		{
			starttimer();
			prefx = prefy = 0;
			if (ro_lastactivity.numcreatednodes == 1)
			{
				portposition(endnode[0], endport[0], &x0, &y0);
				portposition(endnode[1], endport[1], &x1, &y1);
				prefx = (ro_lastactivity.creatednodes[0]->lowx + ro_lastactivity.creatednodes[0]->highx) / 2 -
					(x0+x1) / 2;
				prefy = (ro_lastactivity.creatednodes[0]->lowy + ro_lastactivity.creatednodes[0]->highy) / 2 -
					(y0+y1) / 2;
			} else if (ro_lastactivity.numcreatednodes == 2)
			{
				portposition(endnode[0], endport[0], &x0, &y0);
				portposition(endnode[1], endport[1], &x1, &y1);
				prefx = (ro_lastactivity.creatednodes[0]->lowx + ro_lastactivity.creatednodes[0]->highx +
					ro_lastactivity.creatednodes[1]->lowx + ro_lastactivity.creatednodes[1]->highx) / 4 -
						(x0+x1) / 2;
				prefy = (ro_lastactivity.creatednodes[0]->lowy + ro_lastactivity.creatednodes[0]->highy +
					ro_lastactivity.creatednodes[1]->lowy + ro_lastactivity.creatednodes[1]->highy) / 4 -
						(y0+y1) / 2;
			}
			if (ro_mimiccreatedany(endnode[0], endport[0], endx[0], endy[0],
				endnode[1], endport[1], endx[1], endy[1],
				width, proto, prefx, prefy) == 0)
			{
				/* nothing was wired */
				if (forced) ttyputmsg(_("No wires mimiced"));
			}
			elapsed = endtimer();
			if (elapsed > 60.0 && (us_useroptions&BEEPAFTERLONGJOB) != 0)
				ttybeep(SOUNDBEEP, TRUE);
			if ((options&MIMICINTERACTIVE) == 0 && elapsed >= 2.0)
				ttyputmsg(_("Mimicing took %s"), explainduration(elapsed));
		}
		ro_lastactivity.numcreatedarcs = 0;
		return;
	}
}

/*
 * Routine to mimic the unrouting of an arc that ran from nodes[0]/ports[0] to
 * nodes[1]/ports[1] with type "typ".
 */
void ro_mimicdelete(ARCPROTO *typ, NODEINST **nodes, PORTPROTO **ports)
{
	REGISTER NODEPROTO *cell;
	REGISTER ARCINST *ai, *nextai;
	REGISTER INTBIG match, dist, thisdist, deleted, angle, thisangle;
	INTBIG x0, y0, x1, y1;

	/* determine length of deleted arc */
	portposition(nodes[0], ports[0], &x0, &y0);
	portposition(nodes[1], ports[1], &x1, &y1);
	dist = computedistance(x0, y0, x1, y1);
	angle = (dist != 0 ? figureangle(x0, y0, x1, y1) : 0);

	/* look for a similar situation to delete */
	deleted = 0;
	cell = nodes[0]->parent;
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = nextai)
	{
		nextai = ai->nextarcinst;

		/* arc must be of the same type */
		if (ai->proto != typ) continue;

		/* arc must connect to the same type of node/port */
		match = 0;
		if (ai->end[0].nodeinst->proto == nodes[0]->proto &&
			ai->end[1].nodeinst->proto == nodes[1]->proto &&
			ai->end[0].portarcinst->proto == ports[0] &&
			ai->end[1].portarcinst->proto == ports[1]) match = 1;
		if (ai->end[0].nodeinst->proto == nodes[1]->proto &&
			ai->end[1].nodeinst->proto == nodes[0]->proto &&
			ai->end[0].portarcinst->proto == ports[1] &&
			ai->end[1].portarcinst->proto == ports[0]) match = -1;
		if (match == 0) continue;

		/* must be the same length and angle */
		portposition(ai->end[0].nodeinst, ai->end[0].portarcinst->proto, &x0, &y0);
		portposition(ai->end[1].nodeinst, ai->end[1].portarcinst->proto, &x1, &y1);
		thisdist = computedistance(x0, y0, x1, y1);
		if (dist != thisdist) continue;
		if (dist != 0)
		{
			thisangle = figureangle(x0, y0, x1, y1);
			if ((angle%1800) != (thisangle%1800)) continue;
		}

		/* the same! delete it */
		startobjectchange((INTBIG)ai, VARCINST);
		(void)killarcinst(ai);
		deleted++;
	}
	if (deleted != 0)
		ttyputmsg(_("MIMIC ROUTING: deleted %ld %s"), deleted, makeplural(_("wire"), deleted));
}

/*
 * Routine to mimic the creation of arc "arc".  Returns the number of mimics made.
 */
INTBIG ro_mimiccreatedany(NODEINST *ni1, PORTPROTO *pp1, INTBIG x1, INTBIG y1,
	NODEINST *ni2, PORTPROTO *pp2, INTBIG x2, INTBIG y2,
	INTBIG width, ARCPROTO *proto, INTBIG prefx, INTBIG prefy)
{
	return(ro_mimicthis(ni1, pp1, x1, y1, ni2, pp2, x2, y2, width, proto, prefx, prefy));
}

/*
 * Routine to mimic the creation of arc "arc".  Returns the number of mimics made.
 */
INTBIG ro_mimiccreatedarc(ARCINST *ai)
{
	REGISTER INTBIG x1, y1, x2, y2;

	x1 = ai->end[0].xpos;   y1 = ai->end[0].ypos;
	x2 = ai->end[1].xpos;   y2 = ai->end[1].ypos;
	return(ro_mimicthis(ai->end[0].nodeinst, ai->end[0].portarcinst->proto, x1, y1,
		ai->end[1].nodeinst, ai->end[1].portarcinst->proto, x2, y2,
		ai->width, ai->proto, 0, 0));
}

/* Prompt: Yes/No with "stop asking" */
static DIALOGITEM ro_yesnostopdialogitems[] =
{
 /*  1 */ {0, {64,156,88,228}, BUTTON, N_("Yes")},
 /*  2 */ {0, {64,68,88,140}, BUTTON, N_("No")},
 /*  3 */ {0, {6,15,54,279}, MESSAGE, x_("")},
 /*  4 */ {0, {96,156,120,276}, BUTTON, N_("Yes, then stop")},
 /*  5 */ {0, {96,20,120,140}, BUTTON, N_("No, and stop")}
};
static DIALOG ro_yesnostopdialog = {{50,75,179,363}, N_("Warning"), 0, 5, ro_yesnostopdialogitems, 0, 0};

#define DYNS_YES     1		/* "Yes" (button) */
#define DYNS_NO      2		/* "No" (button) */
#define DYNS_MESSAGE 3		/* routing message (stat text) */
#define DYNS_YESSTOP 4		/* "Yes, then stop" (button) */
#define DYNS_NOSTOP  5		/* "No, and stop" (button) */

INTBIG ro_mimicthis(NODEINST *oni1, PORTPROTO *opp1, INTBIG ox1, INTBIG oy1,
	NODEINST *oni2, PORTPROTO *opp2, INTBIG ox2, INTBIG oy2,
	INTBIG owidth, ARCPROTO *oproto, INTBIG prefx, INTBIG prefy)
{
	REGISTER NODEPROTO *cell, *proto0, *proto1;
	REGISTER NODEINST *ni, *oni, *node0, *node1;
	NODEINST *endni[2], *nicon1, *nicon2;
	REGISTER PORTPROTO *port0, *port1, *pp, *opp;
	REGISTER NETWORK *net1, *net0;
	PORTPROTO *endpp[2];
	REGISTER PORTARCINST *pi;
	REGISTER BOOLEAN usefangle;
	double fangle;
	REGISTER ARCINST *newai, *oai;
	ARCINST *alt1, *alt2;
	REGISTER INTBIG dist, angle, options, count, i, wantx1, wanty1, end, end0offx, end0offy,
		situation, total, node0wid, node0hei, node1wid, node1hei, wid, hei, con1, con2,
		portpos, oportpos, stopcheck, ifignoreports, ifignorenodetype, ifignorenodesize,
		ifignorearccount, ifignoreothersamedir, itemHit=0;
	INTBIG x0, y0, x1, y1, x0c, y0c, desiredangle, existingangle, thisend, lx, hx, ly, hy,
		endx[2], endy[2];
	static POLYGON *poly = NOPOLYGON;
	REGISTER POLYGON *thispoly, **newportpolys;
	REGISTER POSSIBLEARC *firstpossiblearc, *pa;
	CHAR question[200];
#define NUMSITUATIONS 32
	static INTBIG situations[NUMSITUATIONS] = {
		0,
		LIKELYARCSSAMEDIR,
		                  LIKELYDIFFNODESIZE,
		                                     LIKELYDIFFARCCOUNT,
		                                                        LIKELYDIFFPORT,
		                                                                       LIKELYDIFFNODETYPE,

		LIKELYARCSSAMEDIR|LIKELYDIFFNODESIZE,
		LIKELYARCSSAMEDIR|                   LIKELYDIFFARCCOUNT,
		LIKELYARCSSAMEDIR|                                      LIKELYDIFFPORT,
		LIKELYARCSSAMEDIR|                                                     LIKELYDIFFNODETYPE,
		                  LIKELYDIFFNODESIZE|LIKELYDIFFARCCOUNT,
		                  LIKELYDIFFNODESIZE|                   LIKELYDIFFPORT,
		                  LIKELYDIFFNODESIZE|                                  LIKELYDIFFNODETYPE,
		                                     LIKELYDIFFARCCOUNT|LIKELYDIFFPORT,
		                                     LIKELYDIFFARCCOUNT|               LIKELYDIFFNODETYPE,
		                                                        LIKELYDIFFPORT|LIKELYDIFFNODETYPE,

		LIKELYARCSSAMEDIR|LIKELYDIFFNODESIZE|LIKELYDIFFARCCOUNT,
		LIKELYARCSSAMEDIR|LIKELYDIFFNODESIZE|                   LIKELYDIFFPORT,
		LIKELYARCSSAMEDIR|LIKELYDIFFNODESIZE|                                  LIKELYDIFFNODETYPE,
		LIKELYARCSSAMEDIR|                   LIKELYDIFFARCCOUNT|LIKELYDIFFPORT,
		LIKELYARCSSAMEDIR|                   LIKELYDIFFARCCOUNT|               LIKELYDIFFNODETYPE,
		LIKELYARCSSAMEDIR|                                      LIKELYDIFFPORT|LIKELYDIFFNODETYPE,
		                  LIKELYDIFFNODESIZE|LIKELYDIFFARCCOUNT|LIKELYDIFFPORT,
		                  LIKELYDIFFNODESIZE|LIKELYDIFFARCCOUNT|               LIKELYDIFFNODETYPE,
		                  LIKELYDIFFNODESIZE|                   LIKELYDIFFPORT|LIKELYDIFFNODETYPE,
		                                     LIKELYDIFFARCCOUNT|LIKELYDIFFPORT|LIKELYDIFFNODETYPE,

		LIKELYARCSSAMEDIR|LIKELYDIFFNODESIZE|LIKELYDIFFARCCOUNT|LIKELYDIFFPORT,
		LIKELYARCSSAMEDIR|LIKELYDIFFNODESIZE|LIKELYDIFFARCCOUNT|               LIKELYDIFFNODETYPE,
		LIKELYARCSSAMEDIR|LIKELYDIFFNODESIZE|                   LIKELYDIFFPORT|LIKELYDIFFNODETYPE,
		LIKELYARCSSAMEDIR|                   LIKELYDIFFARCCOUNT|LIKELYDIFFPORT|LIKELYDIFFNODETYPE,
		                  LIKELYDIFFNODESIZE|LIKELYDIFFARCCOUNT|LIKELYDIFFPORT|LIKELYDIFFNODETYPE,

		LIKELYARCSSAMEDIR|LIKELYDIFFNODESIZE|LIKELYDIFFARCCOUNT|LIKELYDIFFPORT|LIKELYDIFFNODETYPE
	};
	REGISTER void *infstr, *dia;
	Q_UNUSED( owidth );

	ttyputmsg(_("Mimicing last arc..."));

	/* get polygon */
	(void)needstaticpolygon(&poly, 5, ro_tool->cluster);

	cell = oni1->parent;
	endni[0] = oni1;   endni[1] = oni2;
	endpp[0] = opp1;   endpp[1] = opp2;
	endx[0] = ox1;     endx[1] = ox2;
	endy[0] = oy1;     endy[1] = oy2;
	options = ro_getoptions();
	count = 0;
	stopcheck = 0;
	firstpossiblearc = NOPOSSIBLEARC;

	/* count the number of other arcs on the ends */
	con1 = 0;
	for(pi = oni1->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst) con1++;
	for(pi = oni2->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst) con1++;
	con1 -= 2;

	/* precompute information about every port in the cell */
	total = 0;
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			pp->temp1 = 0;
			for(i=0; pp->connects[i] != NOARCPROTO; i++)
				if (pp->connects[i] == oproto) break;
			if (pp->connects[i] == NOARCPROTO) continue;
			pp->temp1 = 1;
			total++;
		}
	}
	if (total == 0) return(count);
	if (total > ro_totalmimicports)
	{
		newportpolys = (POLYGON **)emalloc(total * (sizeof (POLYGON *)), ro_tool->cluster);
		for(i=0; i<ro_totalmimicports; i++)
			newportpolys[i] = ro_mimicportpolys[i];
		for(i=ro_totalmimicports; i<total; i++)
			newportpolys[i] = allocpolygon(4, ro_tool->cluster);
		if (ro_totalmimicports > 0) efree((CHAR *)ro_mimicportpolys);
		ro_mimicportpolys = newportpolys;
		ro_totalmimicports = total;
	}
	i = 0;
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		ni->temp1 = i;
		for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			if (pp->temp1 == 0) continue;
			shapeportpoly(ni, pp, ro_mimicportpolys[i], FALSE);
			i++;
		}
	}
	if (stopping(STOPREASONROUTING)) return(count);

	/* search from both ends */
	for(end=0; end<2; end++)
	{
		node0 = endni[end];
		node1 = endni[1-end];
		proto0 = node0->proto;
		proto1 = node1->proto;
		port0 = endpp[end];
		port1 = endpp[1-end];
		x0 = endx[end];     y0 = endy[end];
		x1 = endx[1-end];   y1 = endy[1-end];
		dist = computedistance(x0, y0, x1, y1);
		angle = (dist != 0 ? figureangle(x0, y0, x1, y1) : 0);
		if ((angle%900) == 0) usefangle = FALSE; else
		{
			fangle = ffigureangle(x0, y1, x1, y1);
			usefangle = TRUE;
		}
		portposition(node0, port0, &x0c, &y0c);
		end0offx = x0 - x0c;   end0offy = y0 - y0c;
		nodesizeoffset(node0, &lx, &ly, &hx, &hy);
		node0wid = node0->highx-hx - (node0->lowx+lx);
		node0hei = node0->highy-hy - (node0->lowy+ly);
		nodesizeoffset(node1, &lx, &ly, &hx, &hy);
		node1wid = node1->highx-hx - (node1->lowx+lx);
		node1hei = node1->highy-hy - (node1->lowy+ly);

		/* now search every node in the cell */
		for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			/* now look for another node that matches the situation */
			for(oni = cell->firstnodeinst; oni != NONODEINST; oni = oni->nextnodeinst)
			{
				/* see if stopping is requested */
				stopcheck++;
				if ((stopcheck%50) == 0)
				{
					if (stopping(STOPREASONROUTING))
					{
						/* free memory for mimicing */
						while (firstpossiblearc != NOPOSSIBLEARC)
						{
							pa = firstpossiblearc;
							firstpossiblearc = pa->nextpossiblearc;
							ro_freepossiblearc(pa);
						}
						return(count);
					}
				}

				/* ensure that intra-node wirings stay that way */
				if (node0 == node1)
				{
					if (ni != oni) continue;
				} else
				{
					if (ni == oni) continue;
				}

				/* make sure the distances are sensible */
				if (ni->geom->lowx - oni->geom->highx > dist) continue;
				if (oni->geom->lowx - ni->geom->highx > dist) continue;
				if (ni->geom->lowy - oni->geom->highy > dist) continue;
				if (oni->geom->lowy - ni->geom->highy > dist) continue;

				/* compare each port */
				portpos = ni->temp1;
				for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				{
					/* make sure the arc can connect */
					if (pp->temp1 == 0) continue;

					getcenter(ro_mimicportpolys[portpos], &x0, &y0);
					portpos++;
					x0 += end0offx;   y0 += end0offy;
					if (dist == 0)
					{
						wantx1 = x0;   wanty1 = y0;
					} else
					{
						if (usefangle)
						{
							wantx1 = x0 + rounddouble(dist * cos(fangle));
							wanty1 = y0 + rounddouble(dist * sin(fangle));
						} else
						{
							wantx1 = x0 + mult(dist, cosine(angle));
							wanty1 = y0 + mult(dist, sine(angle));
						}
					}

					oportpos = oni->temp1;
					for(opp = oni->proto->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
					{
						/* make sure the arc can connect */
						if (opp->temp1 == 0) continue;
						thispoly = ro_mimicportpolys[oportpos];
						oportpos++;

						/* don't replicate what is already done */
						if (ni == node0 && pp == port0 && oni == node1 && opp == port1) continue;

						/* see if they are the same distance apart */
						if (!isinside(wantx1, wanty1, thispoly)) continue;

						/* figure out the wiring situation here */
						situation = 0;

						/* see if there are already wires going in this direction */
						if (x0 == wantx1 && y0 == wanty1) desiredangle = -1; else
							desiredangle = figureangle(x0, y0, wantx1, wanty1);
						net0 = NONETWORK;
						for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
						{
							if (pi->proto != pp) continue;
							oai = pi->conarcinst;
							net0 = oai->network;
							if (desiredangle < 0)
							{
								if (oai->end[0].xpos == oai->end[1].xpos &&
									oai->end[0].ypos == oai->end[1].ypos) break;
							} else
							{
								if (oai->end[0].xpos == oai->end[1].xpos &&
									oai->end[0].ypos == oai->end[1].ypos) continue;
								if (oai->end[0].portarcinst == pi) thisend = 0; else thisend = 1;
								existingangle = figureangle(oai->end[thisend].xpos, oai->end[thisend].ypos,
									oai->end[1-thisend].xpos, oai->end[1-thisend].ypos);
								if (existingangle == desiredangle) break;
							}
						}
						if (pi != NOPORTARCINST) situation |= LIKELYARCSSAMEDIR;

						if (x0 == wantx1 && y0 == wanty1) desiredangle = -1; else
							desiredangle = figureangle(wantx1, wanty1, x0, y0);
						net1 = NONETWORK;
						for(pi = oni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
						{
							if (pi->proto != opp) continue;
							oai = pi->conarcinst;
							net1 = oai->network;
							if (desiredangle < 0)
							{
								if (oai->end[0].xpos == oai->end[1].xpos &&
									oai->end[0].ypos == oai->end[1].ypos) break;
							} else
							{
								if (oai->end[0].xpos == oai->end[1].xpos &&
									oai->end[0].ypos == oai->end[1].ypos) continue;
								if (oai->end[0].portarcinst == pi) thisend = 0; else thisend = 1;
								existingangle = figureangle(oai->end[thisend].xpos, oai->end[thisend].ypos,
									oai->end[1-thisend].xpos, oai->end[1-thisend].ypos);
								if (existingangle == desiredangle) break;
							}
						}
						if (pi != NOPORTARCINST) situation |= LIKELYARCSSAMEDIR;

						/* if there is a network that already connects these, ignore */
						if (net1 == net0 && net0 != NONETWORK) continue;

						if (pp != port0 || opp != port1)
							situation |= LIKELYDIFFPORT;
						con2 = 0;
						for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst) con2++;
						for(pi = oni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst) con2++;
						if (con1 != con2) situation |= LIKELYDIFFARCCOUNT;
						if (ni->proto != proto0 || oni->proto != proto1)
							situation |= LIKELYDIFFNODETYPE;
						nodesizeoffset(ni, &lx, &ly, &hx, &hy);
						wid = ni->highx-hx - (ni->lowx+lx);
						hei = ni->highy-hy - (ni->lowy+ly);
						if (wid != node0wid || hei != node0hei) situation |= LIKELYDIFFNODESIZE;
						nodesizeoffset(oni, &lx, &ly, &hx, &hy);
						wid = oni->highx-hx - (oni->lowx+lx);
						hei = oni->highy-hy - (oni->lowy+ly);
						if (wid != node1wid || hei != node1hei) situation |= LIKELYDIFFNODESIZE;

						/* see if this combination has already been considered */
						for(pa = firstpossiblearc; pa != NOPOSSIBLEARC; pa = pa->nextpossiblearc)
						{
							if (pa->ni1 == ni && pa->pp1 == pp && pa->ni2 == oni && pa->pp2 == opp)
								break;
							if (pa->ni2 == ni && pa->pp2 == pp && pa->ni1 == oni && pa->pp1 == opp)
								break;
						}
						if (pa != NOPOSSIBLEARC)
						{
							if (pa->situation == situation) continue;
							for(i=0; i<NUMSITUATIONS; i++)
							{
								if (pa->situation == situations[i]) break;
								if (situation == situations[i]) break;
							}
							if (pa->situation == situations[i]) continue;
						}
						if (pa == NOPOSSIBLEARC)
						{
							pa = ro_allocpossiblearc();
							pa->nextpossiblearc = firstpossiblearc;
							firstpossiblearc = pa;
						}
						pa->ni1 = ni;      pa->pp1 = pp;
						pa->x1 = x0;       pa->y1 = y0;
						pa->ni2 = oni;     pa->pp2 = opp;
						pa->x2 = wantx1;   pa->y2 = wanty1;
						pa->situation = situation;
					}
				}
			}
		}
	}

	/* turn off network tool before mimicing to speed up reevaluation */
	toolturnoff(net_tool, FALSE);

	/* now create the mimiced arcs */
	ifignoreports = ifignorearccount = ifignorenodetype = ifignorenodesize = ifignoreothersamedir = 0;
	for(i=0; i<NUMSITUATIONS; i++)
	{
		/* see if this situation is possible */
		total = 0;
		for(pa = firstpossiblearc; pa != NOPOSSIBLEARC; pa = pa->nextpossiblearc)
			if (pa->situation == situations[i]) total++;
		if (total == 0) continue;

		/* see if this situation is desired */
		if ((options&MIMICINTERACTIVE) == 0)
		{
			/* make sure this situation is the desired one */
			if ((options&MIMICIGNOREPORTS) == 0 && (situations[i]&LIKELYDIFFPORT) != 0)
			{
				ifignoreports += total;
				continue;
			}
			if ((options&MIMICONSIDERARCCOUNT) != 0 && (situations[i]&LIKELYDIFFARCCOUNT) != 0)
			{
				ifignorearccount += total;
				continue;
			}
			if ((options&MIMICIGNORENODETYPE) == 0 && (situations[i]&LIKELYDIFFNODETYPE) != 0)
			{
				ifignorenodetype += total;
				continue;
			}
			if ((options&MIMICIGNORENODESIZE) == 0 && (situations[i]&LIKELYDIFFNODESIZE) != 0)
			{
				ifignorenodesize += total;
				continue;
			}
			if ((options&MIMICOTHARCTHISDIR) == 0 && (situations[i]&LIKELYARCSSAMEDIR) != 0)
			{
				ifignoreothersamedir += total;
				continue;
			}
		} else
		{
			/* show the wires to be created */
			(void)asktool(us_tool, x_("down-stack"));
			(void)asktool(us_tool, x_("clear"));
			for(pa = firstpossiblearc; pa != NOPOSSIBLEARC; pa = pa->nextpossiblearc)
			{
				if (pa->situation != situations[i]) continue;
				if (pa->x1 == pa->x2 && pa->y1 == pa->y2)
				{
					dist = el_curlib->lambda[el_curtech->techindex];
					(void)asktool(us_tool, x_("show-area"), pa->x1-dist, pa->x1+dist, pa->y1-dist, pa->y1+dist,
						(INTBIG)cell);
				} else
				{
					(void)asktool(us_tool, x_("show-line"), pa->x1, pa->y1, pa->x2, pa->y2, (INTBIG)cell);
				}
			}
			esnprintf(question, 200, _("Create %ld %s shown here?"),
				total, makeplural(_("wire"), total));
			dia = DiaInitDialog(&ro_yesnostopdialog);
			if (dia == 0)
			{
				toolturnon(net_tool);
				return(0);
			}
			DiaSetText(dia, DYNS_MESSAGE, question);
			for(;;)
			{
				itemHit = DiaNextHit(dia);
				if (itemHit == DYNS_YES || itemHit == DYNS_YESSTOP ||
					itemHit == DYNS_NO || itemHit == DYNS_NOSTOP) break;
			}
			DiaDoneDialog(dia);
			(void)asktool(us_tool, x_("up-stack"));
			if (itemHit == DYNS_NO) continue;
			if (itemHit == DYNS_NOSTOP) break;
		}

		/* make the wires */
		for(pa = firstpossiblearc; pa != NOPOSSIBLEARC; pa = pa->nextpossiblearc)
		{
			if (pa->situation != situations[i]) continue;
			portposition(pa->ni1, pa->pp1, &x0, &y0);
			portposition(pa->ni2, pa->pp2, &x1, &y1);
			newai = aconnect(pa->ni1->geom, pa->pp1, pa->ni2->geom, pa->pp2,
				oproto, (x0+x1)/2+prefx, (y0+y1)/2+prefy, &alt1, &alt2, &nicon1, &nicon2, 900, FALSE, FALSE);
			if (newai == NOARCINST)
			{
				ttyputerr(_("Problem creating arc"));
				toolturnon(net_tool);
				return(count);
			}
			count++;
		}
		(void)asktool(us_tool, x_("flush-changes"));

		/* stop now if requested */
		if ((options&MIMICINTERACTIVE) != 0 && itemHit == DYNS_YESSTOP) break;
	}

	/* turn network tool back on */
	toolturnon(net_tool);

	/* free memory for mimicing */
	while (firstpossiblearc != NOPOSSIBLEARC)
	{
		pa = firstpossiblearc;
		firstpossiblearc = pa->nextpossiblearc;
		ro_freepossiblearc(pa);
	}
	if (count != 0)
	{
		us_reportarcscreated(_("MIMIC ROUTING"), count, 0, 0, 0);
	} else
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, _("No wires added"));
		if (ifignoreports != 0)
			formatinfstr(infstr, _(", would add %ld wires if 'ports must match' were off"), ifignoreports);
		if (ifignorearccount != 0)
			formatinfstr(infstr, _(", would add %ld wires if 'number of existing arcs must match' were off"), ifignorearccount);
		if (ifignorenodetype != 0)
			formatinfstr(infstr, _(", would add %ld wires if 'node types must match' were off"), ifignorenodetype);
		if (ifignorenodesize != 0)
			formatinfstr(infstr, _(", would add %ld wires if 'nodes sizes must match' were off"), ifignorenodesize);
		if (ifignoreothersamedir != 0)
			formatinfstr(infstr, _(", would add %ld wires if 'cannot have other arcs in the same direction' were off"), ifignoreothersamedir);
		ttyputmsg(x_("%s"), returninfstr(infstr));
	}
	return(count);
}

POSSIBLEARC *ro_allocpossiblearc(void)
{
	REGISTER POSSIBLEARC *pa;

	if (ro_mimicpossiblearcfree == NOPOSSIBLEARC)
	{
		pa = (POSSIBLEARC *)emalloc(sizeof (POSSIBLEARC), ro_tool->cluster);
		if (pa == 0) return(NOPOSSIBLEARC);
	} else
	{
		pa = ro_mimicpossiblearcfree;
		ro_mimicpossiblearcfree = pa->nextpossiblearc;
	}
	return(pa);
}

void ro_freepossiblearc(POSSIBLEARC *pa)
{
	pa->nextpossiblearc = ro_mimicpossiblearcfree;
	ro_mimicpossiblearcfree = pa;
}

#endif  /* ROUTTOOL - at top */
