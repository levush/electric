/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: drcflat.cpp
 * Flat box and net extraction for short circuit detection
 * Written by: David Lewis, University of Toronto
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
#if DRCTOOL

#include "global.h"
#include "drc.h"
#include "efunction.h"
#include "egraphics.h"

#define MAX_DRC_FLATIGNORE 100				/* max cells to ignore */
#define FLATDRCNAME        x_("ffindshort")		/* fast short program */
#define SFLATDRCNAME       x_("findshort")		/* slow short program */
#define SORTNAME           x_("sort")			/* sort program */
#define MAXSHORTLAYER	   9				/* Max. layer index  */

static NODEPROTO  *dr_flatignored[MAX_DRC_FLATIGNORE];
static INTBIG      dr_index;
static INTBIG      dr_nflat_ignored = 0;
static INTBIG      dr_flat_boxcount;
static FILE       *dr_file;
static TECHNOLOGY *dr_maintech;
static CHAR        dr_flatdrcfile[132];
static CHAR        dr_yminfile[132];
static CHAR        dr_ymaxfile[132];

/* prototypes for local routines */
static BOOLEAN dr_readshorterrors(EProcess &process);
static void    dr_flatprint(NODEPROTO*, XARRAY);
static void    dr_transistor_hack(POLYGON*, NODEINST*);
static void    dr_flatdesc_poly(POLYGON*, INTBIG, XARRAY, TECHNOLOGY*, NODEPROTO*);
static void    dr_flatprop(ARCINST*, INTBIG);

/*
 * routine to flag cell "cell" as ignorable for flat DRC
 */
void dr_flatignore(CHAR *cell)
{
	INTBIG i;
	REGISTER NODEPROTO *np;

	np = getnodeproto(cell);
	if (np == NONODEPROTO)
	{
		ttyputerr(_("No cell called %s"), cell);
		return;
	}
	if (np->primindex != 0)
	{
		ttyputerr(_("Can only ignore cells, not primitives"));
		return;
	}
	for(i=0; i<dr_nflat_ignored; i++) if (dr_flatignored[i] == np)
		return;
	if (dr_nflat_ignored == MAX_DRC_FLATIGNORE)
	{
		ttyputerr(_("Too many cells"));
		return;
	}
	dr_flatignored[dr_nflat_ignored++] = np;
	ttyputmsg(_("Cell %s will be ignored from flat DRC"), describenodeproto(np));
}

/*
 * routine to flag cell "cell" as not ignorable for flat DRC
 */
void dr_flatunignore(CHAR *cell)
{
	INTBIG i;
	REGISTER NODEPROTO *np;

	np = getnodeproto(cell);
	if (np == NONODEPROTO)
	{
		ttyputerr(_("No cell called %s"), cell);
		return;
	}
	if (np->primindex != 0)
	{
		ttyputerr(_("Can only unignore cells, not primitives"));
		return;
	}
	for(i=0; i<dr_nflat_ignored && dr_flatignored[i] != np; i++) ;
	if (i == dr_nflat_ignored)
	{
		ttyputerr(_("Not being ignored"));
		return;
	}
	for(; i<dr_nflat_ignored; i++)
		dr_flatignored[i-1] = dr_flatignored[i];
	dr_nflat_ignored--;
	ttyputmsg(_("Cell %s will be included in flat DRC"), describenodeproto(np));
}

void dr_flatwrite(NODEPROTO *simnp)
{
	REGISTER BOOLEAN ret = TRUE;
	REGISTER PORTPROTO *pp, *spp;
	REGISTER TECHNOLOGY *tech;
	REGISTER VARIABLE *var;
	CHAR p1[20], p2[20];
	CHAR *flatdrcloc, *sflatdrcloc;
	EProcess dr_process;

	if (simnp == NONODEPROTO)
	{
		ttyputerr(_("No cell to flatten for DRC"));
		return;
	}
	dr_maintech = simnp->tech;

	/* create output file names */
	(void)esnprintf(dr_flatdrcfile, 132, x_("%s"), simnp->protoname);
	(void)estrcat(dr_flatdrcfile, x_(".drc"));

	(void)esnprintf(dr_yminfile, 132, x_("%s"), simnp->protoname);
	(void)estrcat(dr_yminfile, x_("_miny.drc"));

	(void)esnprintf(dr_ymaxfile, 132, x_("%s"), simnp->protoname);
	(void)estrcat(dr_ymaxfile, x_("_maxy.drc"));

	/* create the file */
	dr_file = xcreate(dr_flatdrcfile, el_filetypetext, 0, 0);
	if (dr_file == NULL)
	{
		ttyputerr(_("Cannot write %s"), dr_flatdrcfile);
		return;
	}
	ttyputmsg(_("Writing flattened %s circuit into '%s'..."),
		dr_maintech->techname, dr_flatdrcfile);

	/* re-assign net-list values, starting at each port of the top cell */
	dr_index = 1;
	for(pp = simnp->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		for(spp = simnp->firstportproto; spp != pp; spp = spp->nextportproto)
		{
			if (spp->network != pp->network) continue;

			/* this port is the same as another one, use same net */
			pp->temp2 = spp->temp2;
			break;
		}

		/* assign a new net number if the loop terminated normally */
		if (spp == pp) pp->temp2 = dr_index++;
	}

	/* initialize cache of CIF layer information */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		var = getval((INTBIG)tech, VTECHNOLOGY, VSTRING|VISARRAY, x_("IO_cif_layer_names"));
		tech->temp1 = (var == NOVARIABLE ? 0 : var->addr);
	}

	/* write the flattened geometry */
	dr_flat_boxcount = 0;
	begintraversehierarchy();
	dr_flatprint(simnp, el_matid);
	endtraversehierarchy();

	/* clean up the file */
	xclose(dr_file);

	if (stopping(STOPREASONDRC)) return;

	/* no point continuing if nothing to do */
	if (dr_flat_boxcount == 0)
	{
		ttyputmsg(_("No boxes written, DRC short detection complete"));
		return;
	}

	/* setup communication with fast short finding program */
	ttyputmsg(_("%ld boxes written, now attempting fast short detection..."), dr_flat_boxcount);
	(void)esnprintf(p1, 20, x_("%ld"), el_curlib->lambda[el_curtech->techindex]);
	(void)esnprintf(p2, 20, x_("%ld"), dr_flat_boxcount);
	if ((flatdrcloc = egetenv(x_("FLATDRCLOC"))) == NULL)
		flatdrcloc = FLATDRCLOC;
	dr_process.clearArguments();
	dr_process.addArgument( flatdrcloc );
	dr_process.addArgument( dr_flatdrcfile );
	dr_process.addArgument( p1 );
	dr_process.addArgument( p2 );
	dr_process.setCommunication( FALSE, TRUE, TRUE );
	dr_process.start();
	ret = dr_readshorterrors(dr_process);
	dr_process.kill();

	/* if this program ran correctly, stop now */
	if (!ret)
	{
		ttyputmsg(_("DRC short detection complete"));
		return;
	}

	/* setup for slow short detection */
	ttyputmsg(_("Too many boxes: disk version must be run"));
	ttyputmsg(_("  Interim step: sorting by minimum Y into '%s'..."), dr_yminfile);
	dr_process.clearArguments();
	dr_process.addArgument( SORTLOC );
	dr_process.addArgument( x_("+0n") );
	dr_process.addArgument( x_("-1") );
	dr_process.addArgument( x_("-o") );
	dr_process.addArgument( dr_yminfile );
	dr_process.addArgument( dr_flatdrcfile );
	dr_process.setCommunication( FALSE, FALSE, FALSE );
	dr_process.start();
	dr_process.wait();
	ttyputmsg(_("  Interim step: sorting by maximum Y into '%s'..."), dr_ymaxfile);
	dr_process.clearArguments();
	dr_process.addArgument( SORTLOC );
	dr_process.addArgument( x_("+1n") );
	dr_process.addArgument(	x_("-2") );
	dr_process.addArgument(	x_("-o") );
	dr_process.addArgument(	dr_ymaxfile );
	dr_process.addArgument(	dr_flatdrcfile );
	dr_process.setCommunication( FALSE, FALSE, FALSE );
	dr_process.start();
	dr_process.wait();

	ttyputmsg(_("Now invoking slow short detection..."));
	(void)esnprintf(p1, 20, x_("%ld"), el_curlib->lambda[el_curtech->techindex]);
	if ((sflatdrcloc = egetenv(x_("SFLATDRCLOC"))) == NULL)
		sflatdrcloc = SFLATDRCLOC;
	dr_process.clearArguments();
	dr_process.addArgument( sflatdrcloc );
	dr_process.addArgument( dr_yminfile );
	dr_process.addArgument( dr_ymaxfile );
	dr_process.addArgument( p1 );
	dr_process.setCommunication( FALSE, TRUE, TRUE );
	dr_process.start();
	/* run the short detection program */
	(void)dr_readshorterrors(dr_process);
	dr_process.kill();
	ttyputmsg(_("DRC short detection complete"));
}

BOOLEAN dr_readshorterrors(EProcess &process)
{
	REGISTER CHAR *ptr, *layname;
	REGISTER INTBIG errors, lx, hx, ly, hy;
	CHAR line[200], *err, layer[50];
	INTBIG sx, sy, cx, cy;

	/* read from the DRC and list on the status terminal */
	errors = 0;
	ptr = line;
	for(;;)
	{
		if (stopping(STOPREASONDRC)) break;
		INTSML ch = process.getChar();
		*ptr = (CHAR)ch;
		if (ch == EOF) break;
		if (*ptr == '\n')
		{
			*ptr = 0;

			/* special case to detect failure of fast short program */
			if (estrcmp(line, _("Out of space")) == 0) return(TRUE);

			if (line[0] == 'L')
			{
				if (errors == 0)
				{
					/* start by removing any highlighting */
					(void)asktool(us_tool, x_("clear"));
					flushscreen(); /* empty graphics buffer */
				}

				/* convert error to highlight */
				(void)esscanf(line, x_("L %s B %ld %ld %ld %ld"), layer, &sx, &sy, &cx, &cy);
				lx = cx-sx/2;   hx = cx+sx/2;
				ly = cy-sy/2;   hy = cy+sy/2;
				layname = layername(dr_maintech, eatoi(layer));
				ttyputmsg(_("Error on %s layer from X(%s...%s) Y(%s...%s)"),
					layname, latoa(lx, 0), latoa(hx, 0), latoa(ly, 0), latoa(hy, 0));
				(void)asktool(us_tool, x_("show-area"), lx, hx, ly, hy, el_curwindowpart->curnodeproto);
				flushscreen(); /* empty graphics buffer */
				errors++;
			} else if (line[0] == '*')
			{
				/* found number of errors, pass to user */
				err = &line[2];
				ttyputerr(x_("%s"), err);
			} else ttyputmsg(x_("%s"), line);
			ptr = line;
			continue;
		}
		ptr++;
	}
	return(FALSE);
}

/*
 * routine to flatten hierarchy from cell "cell" down, printing boxes and
 * net numbers.  The transformation into the cell is "mytrans".
 */
void dr_flatprint(NODEPROTO *cell, XARRAY mytrans)
{
	REGISTER INTBIG i;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp, *spp;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER NODEINST *ni;
	static POLYGON *poly = NOPOLYGON;
	XARRAY subtrans, temptrans1, temptrans2;
	INTBIG n;

	if (stopping(STOPREASONDRC)) return;

	/* see if this cell is to be ignored */
	for(i = 0; i < dr_nflat_ignored; i++) if (cell == dr_flatignored[i]) return;

	/* make sure there is a polygon */
	(void)needstaticpolygon(&poly, 4, dr_tool->cluster);

	/* reset the arcinst node values */
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		ai->temp2 = 0;

	/* propagate port numbers to arcs in the cell */
	for(pp = cell->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		/* find a connecting arc on the node that this port comes from */
		for(pi = pp->subnodeinst->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			ai = pi->conarcinst;
			if (ai->network != pp->network) continue;
			if (ai->temp2 != 0) continue;
			dr_flatprop(ai, pp->temp2);
			break;
		}
	}

	/* set node numbers on arcs that are local */
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		if (ai->temp2 == 0) dr_flatprop(ai, dr_index++);

	/* write every arc in the cell */
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->proto->tech != dr_maintech) continue;
		n = arcpolys(ai, NOWINDOWPART);
		for(i = 0; i < n; i++)
		{
			shapearcpoly(ai, i, poly);
			dr_flatdesc_poly(poly, ai->temp2, mytrans, ai->proto->tech, cell);
		}
	}

	/* write every node in the cell */
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* initialize network numbers on every port on this node */
		for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			pp->temp2 = 0;

		/* set network numbers from arcs and exports */
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			pp = pi->proto;
			if (pp->temp2 != 0) continue;
			pp->temp2 = pi->conarcinst->temp2;
		}
		for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		{
			pp = pe->proto;
			if (pp->temp2 != 0) continue;
			pp->temp2 = pe->exportproto->temp2;
		}

		/* look for unassigned ports and give new or copied network numbers */
		for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			if (pp->temp2 != 0) continue;
			for(spp = ni->proto->firstportproto; spp != NOPORTPROTO; spp = spp->nextportproto)
			{
				if (spp->temp2 == 0) continue;
				if (spp->network != pp->network) continue;
				pp->temp2 = spp->temp2;
				break;
			}
			if (pp->temp2 == 0) pp->temp2 = dr_index++;
		}

		/* if it is primitive, write the information */
		if (ni->proto->primindex != 0)
		{
			if (ni->proto->tech == dr_maintech)
			{
				makerot(ni, subtrans);
				n = nodeEpolys(ni, 0, NOWINDOWPART);
				for(i = 0; i < n; i ++)
				{
					shapeEnodepoly(ni, i, poly);
					if (poly->portproto == NOPORTPROTO) continue;
					dr_transistor_hack(poly, ni);
					xformpoly(poly, subtrans);
					dr_flatdesc_poly(poly, poly->portproto->temp2, mytrans, ni->proto->tech, cell);
				}
			}
		} else
		{
			/* recurse on nonprimitive node */
			makerot(ni, temptrans1);
			transmult(temptrans1, mytrans, temptrans2);
			maketrans(ni, temptrans1);
			transmult(temptrans1, temptrans2, subtrans);
			downhierarchy(ni, ni->proto, 0);
			dr_flatprint(ni->proto, subtrans);
			uphierarchy();
		}
	}
}

/*
 * In the MOCMOS technology (and possibly among others) there is a problem
 * with the shortchecker.  Here is a diagram of what happens where the || is
 * part of the gate and polysilicon, and = is the active.
 * Represented here are three transistors at minimum spacing (each ascii
 * character is 1 lambda).  If the shortchecker is run on this, it gives
 * errors on the active spacing between the transistors 1 & 3.  This is
 * because the actives of these transistors actually does touch underneath
 * transistor 2 (the overlap of active is 3 lambda).
 *
 *          1    2   3
 *          ||  ||  ||
 *        ==||==||==||==
 *        ==||==||==||==
 *        ==||==||==||==
 *          ||  ||  ||
 *
 * This routine checks for this situation and attempts to nullify it.
 * It sees if the active portion of a transistor connects to another transistor,
 * and if so, shortens the distance.  It works great on regular transistors, but only
 * marginally well on the serpentine transistors.  Written by Ken Stevens.
 */
void dr_transistor_hack(POLYGON *poly, NODEINST *ni)
{
	REGISTER PORTPROTO *pp, *spp;
	REGISTER PORTARCINST *pi, *spi;
	REGISTER INTBIG drain, portnum, type;
	INTBIG edge;
	REGISTER TECH_NODES *thistn;

	type = (ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH;
	if (!(type == NPTRANMOS || type == NPTRADMOS || type == NPTRAPMOS ||
		type == NPTRANS)) return;

	/* Make sure this is an active port */
	if (type == NPTRANS)
		/* gate is port 0, source is port 1, drain is port 2 */
		drain = 2;
	else
		/* gate is port 0 or 2, source is port 1, drain is port 3 */
		drain = 3;

	/* Get current port and what it is connected to. */
	pp = poly->portproto;  portnum = 0;
	for (spp = ni->proto->firstportproto; spp != NOPORTPROTO; spp = spp->nextportproto)
	{
		if (spp == pp) break;
		portnum++;
	}
	if (spp == NOPORTPROTO) return;
	thistn = pp->parent->tech->nodeprotos[ni->proto->primindex-1];
	if (portnum != 1 && portnum != drain) return;
	for (pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		if (pi->proto == pp) break;
	if (pi == NOPORTARCINST) return;
	if (pi->conarcinst->end[0].portarcinst == pi) spi = pi->conarcinst->end[1].portarcinst;
		else spi = pi->conarcinst->end[0].portarcinst;

	/* spi contains other port arc inst.  See if it is a transistor. */
	type = (spi->proto->parent->userbits&NFUNCTION) >> NFUNCTIONSH;
	if (type == NPTRANMOS || type == NPTRADMOS || type == NPTRAPMOS || type == NPTRANS)
	{
		/* Spacing needs adjusting when active width is greater than gate distance. */
		REGISTER INTBIG i, gate_min_distance, active_overlap;
		REGISTER INTBIG lambda, correction;
		REGISTER TECH_SERPENT *st;
		REGISTER TECH_POLYGON *lay;

		lambda = lambdaofnode(ni);
		i = 0;
		for (lay = &thistn->ele[i].basics; lay->portnum != portnum; lay = &thistn->ele[i].basics)
			i++;
		st = &thistn->ele[i];
		active_overlap = st->lwidth + st->rwidth;
		i=0; do lay = &thistn->ele[i++].basics; while (lay->portnum != 0);
		gate_min_distance = drcmindistance(ni->proto->tech, ni->parent->lib,
			lay->layernum, 0, lay->layernum, 0, FALSE, FALSE, &edge, 0);
		if (active_overlap > gate_min_distance)
		{
			correction = ((active_overlap - gate_min_distance) / WHOLE) * lambda;
			if (poly->style == FILLEDRECT)
			{
				/* Decrease high or low coordinates of polygon by lambda
				 * since we have a transistor to transistor contact
				 */
				if (portnum == 1) poly->yv[1] -= correction; else
					poly->yv[0] += correction;
			} else
			{
				INTBIG lx, hx, ly, hy, centerx, centery, xwid, ywid;
				REGISTER INTBIG default_width = (active_overlap/WHOLE) * lambda;

				/* I don't really know which polygons of the serpentine transistor
				 * to adjust, so this is a lame heuristic to figure it out using the
				 * center of the cell.  It shouldn't screw anything up too bad, anyway.
				 * Not too good for obtuse port, better on ports of angle > 180 degrees.
				 */
				centerx = (ni->lowx + ni->highx) / 2;
				centery = (ni->lowy + ni->highy) / 2;
				getbbox(poly, &lx, &hx, &ly, &hy);
				xwid = hx - lx;
				ywid = hy - ly;
				for(i = 0; i < poly->count; i++)
				{
					if (xwid == default_width)
					{
						if (hx < centerx && poly->xv[i] == lx) poly->xv[i] += correction; else
							if (lx > centerx && poly->xv[i] == hx) poly->xv[i] -= correction;
					} else if (ywid == default_width)
					{
						if (hy < centery && poly->yv[i] == ly) poly->yv[i] += correction; else
							if (ly > centery && poly->yv[i] == hy) poly->yv[i] -= correction;
					}
				}
			}
		}
	}
}

/*
 * routine to print the box and net number for polygon "poly" whose net
 * number is "net".  The polygon is transformed by "trans" and is from
 * technology "tech".
 */
void dr_flatdesc_poly(POLYGON *poly, INTBIG net, XARRAY trans, TECHNOLOGY *tech, NODEPROTO *cell)
{
	INTBIG lx, hx, ly, hy, lxb, hxb, lyb, hyb;
	REGISTER INTBIG xv, yv;
	REGISTER INTBIG i;

	/* ignore layers that have no valid CIF */
	if (poly->layer < 0 || tech->temp1 == 0) return;
	if (*(((CHAR **)tech->temp1)[poly->layer]) == 0) return;

	/* ignore layers that have no design rules */
	if (maxdrcsurround(tech, cell->lib, poly->layer) < 0) return;

	xformpoly(poly, trans);
	getbbox(poly, &lxb, &hxb, &lyb, &hyb);
	if (!isbox(poly, &lx, &hx, &ly, &hy))
	{
		/* not a rectangle, but are all coordinates either high or low? */
		xv = yv = 0;
		for(i=0; i<poly->count; i++)
		{
			if (poly->xv[i] != lxb && poly->xv[i] != hxb) xv++;
			if (poly->yv[i] != lyb && poly->yv[i] != hyb) yv++;
		}

		/* if not an orthogonal parallelpiped, don't write it */
		if (xv != 0 && yv != 0) return;
		lx = lxb;   hx = hxb;   ly = lyb;   hy = hyb;
	}

	/* ignore zero or negative sizes */
	if ((hy - ly) > 0 && (hx - lx) > 0)
	{
		xprintf(dr_file, x_("%ld %ld %ld %ld %ld %ld\n"), ly, hy, lx, hx, poly->layer, net);
		dr_flat_boxcount++;
	}
}

/*
 * routine to propagate the node number "nindex" to the "temp2" of arcs
 * connected to arc "ai".
 */
void dr_flatprop(ARCINST *ai, INTBIG nindex)
{
	REGISTER ARCINST *oai;
	REGISTER NODEINST *ni;
	REGISTER PORTARCINST *pi;
	REGISTER INTBIG i;

	/* set this arcinst to the current node number */
	ai->temp2 = nindex;

	/* if signals do not flow through this arc, do not propagate */
	if (((ai->proto->userbits&AFUNCTION) >> AFUNCTIONSH) == APNONELEC) return;

	/* recursively set all arcs and nodes touching this */
	for(i=0; i<2; i++)
	{
		if ((ai->end[i].portarcinst->proto->userbits&PORTISOLATED) != 0) continue;
		ni = ai->end[i].nodeinst;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			/* select an arc that has not been examined */
			oai = pi->conarcinst;
			if (oai->temp2 != 0) continue;

			/* see if the two ports connect electrically */
			if (ai->end[i].portarcinst->proto->network != pi->proto->network)
				continue;

			/* recurse on the nodes of this arc */
			dr_flatprop(oai, nindex);
		}
	}
}

#endif  /* DRCTOOL - at top */
