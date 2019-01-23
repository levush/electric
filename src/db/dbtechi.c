/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dbtechi.c
 * Database technology internal helper routines
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
#include "database.h"
#include "egraphics.h"
#include "tech.h"
#include "tecgen.h"
#include "tecmocmos.h"
#include "efunction.h"
#include "usr.h"

/*
 * when arcs are curved (with "arc curve" or "arc center") the number of
 * line segments will be between this value, and half of this value.
 */
#define MAXARCPIECES   16		/* maximum segments in curved arc */

static GRAPHICS tech_arrow = {LAYERO, CELLOUT, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
	0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};
static GRAPHICS tech_vartxt = {LAYERO, CELLTXT, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
	0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};

/* prototypes for local routines */
static void      db_adjusttree(NODEPROTO*);
static void      tech_shortenmostrans(NODEINST*, POLYGON*, TECH_POLYGON*, INTBIG, INTBIG, TECH_PORTS*);
static INTBIG    tech_displayableanyvars(WINDOWPART *win, NODEPROTO *np, POLYLOOP *pl);
static VARIABLE *tech_filldisplayableanyvar(POLYGON *poly, INTBIG lx, INTBIG hx,
					INTBIG ly, INTBIG hy, INTBIG addr, INTBIG type, TECHNOLOGY *tech,
					WINDOWPART *win, NODEPROTO *cell, VARIABLE **varnoeval, POLYLOOP *pl);

/******************** GENERAL ********************/

BOOLEAN tech_doinitprocess(TECHNOLOGY *tech)
{
	REGISTER TECH_NODES *nty;
	REGISTER TECH_ARCS *at;
	REGISTER INTBIG i;
	REGISTER INTBIG lam;

	/* calculate the true number of layers, arcprotos, and nodeprotos */
	for(tech->layercount=0; tech->layers[tech->layercount] != NOGRAPHICS;
		tech->layercount++) ;
	for(tech->arcprotocount=0; tech->arcprotos[tech->arcprotocount] != ((TECH_ARCS *)-1);
		tech->arcprotocount++) ;
	for(tech->nodeprotocount=0; tech->nodeprotos[tech->nodeprotocount] != ((TECH_NODES *)-1);
		tech->nodeprotocount++) ;

	/* initialize the nodeprotos */
	lam = tech->deflambda;
	for(i=0; i<tech->nodeprotocount; i++)
	{
		nty = tech->nodeprotos[i];
		nty->creation = db_newprimnodeproto(nty->nodename, nty->xsize*lam/WHOLE,
			nty->ysize*lam/WHOLE, nty->nodeindex, tech);
		if (nty->creation == NONODEPROTO)
		{
			ttyputerr(_("Cannot create nodeprotos"));
			return(TRUE);
		}
		nty->creation->userbits = nty->initialbits;
	}

	/* initialize the arcs */
	for(i=0; i<tech->arcprotocount; i++)
	{
		at = tech->arcprotos[i];
		at->creation = db_newarcproto(tech, at->arcname, at->arcwidth*lam/WHOLE, at->arcindex);
		if (at->creation == NOARCPROTO)
		{
			ttyputerr(_("Cannot create arcproto %s in technology %s"),
				at->arcname, tech->techname);
			return(TRUE);
		}
		at->creation->userbits = at->initialbits;
	}
	return(FALSE);
}

BOOLEAN tech_doaddportsandvars(TECHNOLOGY *tech)
{
	REGISTER TECH_NODES *nty;
	REGISTER NODEPROTO *np;
	REGISTER INTBIG i, j, k;
	REGISTER INTBIG pindex, *centerlist;
	REGISTER TECH_PORTS *portinst;
	REGISTER ARCPROTO *ap;
	REGISTER TECHNOLOGY *te;

	/* reset pointers to ports */
	for(i=0; i<tech->nodeprotocount; i++)
	{
		nty = tech->nodeprotos[i];
		portinst = nty->portlist;
		for(j=0; j<nty->portcount; j++)
			portinst[j].addr = NOPORTPROTO;
	}

	/* first add ports */
	for(i=0; i<tech->nodeprotocount; i++)
	{
		nty = tech->nodeprotos[i];
		portinst = nty->portlist;
		for(j=0; j<nty->portcount; j++)
		{
			/* create the list of arcprotos */
			if (portinst[j].portarcs[0] == -1)
			{
				/* convert the arc prototypes in the list */
				for(k=1; portinst[j].portarcs[k] != -1; k++)
				{
					pindex = portinst[j].portarcs[k];
					if ((pindex >> 16) == 0) te = tech; else
					{
						for(te = el_technologies; te != NOTECHNOLOGY; te = te->nexttechnology)
							if (te->techindex == (pindex>>16)-1) break;
						if (te == NOTECHNOLOGY) te = tech;
						pindex &= 0xFFFF;
					}
					for(ap = te->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
						if (ap->arcindex == pindex)
					{
						portinst[j].portarcs[k] = (INTBIG)ap;
						break;
					}
				}
				portinst[j].portarcs[0] = 0;
			}

			/* create the portproto */
			if (portinst[j].addr != NOPORTPROTO)
			{
				ttyputerr(_("Warning: node %s of technology %s shares port descriptions"),
					nty->creation->protoname, tech->techname);
			}
			portinst[j].addr = db_newprimportproto(nty->creation,
				(ARCPROTO **)&portinst[j].portarcs[1], portinst[j].protoname);
			if (portinst[j].addr == NOPORTPROTO)
			{
				ttyputerr(_("Error creating ports"));
				return(TRUE);
			}
			portinst[j].addr->userbits = portinst[j].initialbits;
		}
	}

	/* now add variables */
	for(i=0; tech->variables[i].name != 0; i++)
	{
		/* handle "prototype_center" variable specially */
		if (namesame(tech->variables[i].name, x_("prototype_center")) == 0)
		{
			centerlist = (INTBIG *)tech->variables[i].value;
			for(j=0; j<tech->variables[i].type; j++)
			{
				np = tech->nodeprotos[centerlist[j*3]-1]->creation;
				(void)setvalkey((INTBIG)np, VNODEPROTO, el_prototype_center_key,
					(INTBIG)&centerlist[j*3+1], VINTEGER|VISARRAY|(2<<VLENGTHSH));
			}
			continue;
		}

		/* floating point variables have data in a different place */
		if ((tech->variables[i].type&VTYPE) == VFLOAT && (tech->variables[i].type&VISARRAY) == 0)
		{
			nextchangequiet();
			if (setvalkey((INTBIG)tech, VTECHNOLOGY, makekey(tech->variables[i].name),
				castint(tech->variables[i].fvalue), tech->variables[i].type) == NOVARIABLE)
					return(TRUE);
			continue;
		}

		/* normal variable setting */
		nextchangequiet();
		if (setvalkey((INTBIG)tech, VTECHNOLOGY, makekey(tech->variables[i].name),
			(INTBIG)tech->variables[i].value, tech->variables[i].type) == NOVARIABLE)
				return(TRUE);
	}

	return(FALSE);
}

/*
 * routine to convert old format mocmos (MOSIS CMOS) library "lib" to the current
 * design style
 */
void tech_convertmocmoslib(LIBRARY *lib)
{
	REGISTER INTBIG prims;
	REGISTER INTBIG lambda;
	REGISTER NODEPROTO *np;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni;
	REGISTER TECHNOLOGY *mocmostech;

	for(mocmostech = el_technologies; mocmostech != NOTECHNOLOGY; mocmostech = mocmostech->nexttechnology)
		if (namesame(mocmostech->techname, x_("mocmos")) == 0) break;
	if (mocmostech == NOTECHNOLOGY)
	{
		ttyputmsg(M_("Cannot find 'mocmos' technology"));
		return;
	}

	lambda = lib->lambda[mocmostech->techindex];
	prims = 0;
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		{
			if (ai->proto->tech != mocmostech) continue;
			if (namesame(ai->proto->protoname, x_("S-Active")) != 0) continue;
			undogeom(ai->geom, np);
			ai->width += 4 * lambda;
			linkgeom(ai->geom, np);
			prims++;
		}
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if (ni->proto->primindex == 0) continue;
			if (ni->proto->tech != mocmostech) continue;
			if (namesame(ni->proto->protoname, x_("S-Active-Pin")) == 0 ||
				namesame(ni->proto->protoname, x_("S-Transistor")) == 0 ||
				namesame(ni->proto->protoname, x_("Metal-1-S-Active-Con")) == 0)
			{
				undogeom(ni->geom, np);
				ni->lowx -= 2 * lambda;
				ni->highx += 2 * lambda;
				ni->lowy -= 2 * lambda;
				ni->highy += 2 * lambda;
				linkgeom(ni->geom, np);
				prims++;
				continue;
			}
			if (ni->proto == mocmos_metal1pwellprim)
			{
				undogeom(ni->geom, np);
				ni->lowx -= 4 * lambda;
				ni->highx += 4 * lambda;
				ni->lowy -= 4 * lambda;
				ni->highy += 4 * lambda;
				linkgeom(ni->geom, np);
				prims++;
				continue;
			}
		}
	}

	/* if nothing was changed, quit now */
	if (prims == 0)
	{
		ttyputmsg(M_("No MOSIS CMOS objects found to alter"));
		return;
	}

	/* adjust cell sizes */
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		np->temp1 = 0;
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		if (np->firstinst == NONODEINST) db_adjusttree(np);

	ttyputmsg(M_("%ld S-active arcs, S-Active pins, Metal-1-S-Active"), prims);
	ttyputmsg(M_("  contacts, S-Transistors, and Metal-1-Substrate"));
	ttyputmsg(M_("  contacts from the MOSIS CMOS technology in this"));
	ttyputmsg(M_("  library have been adjusted."));
	ttyputmsg(M_("NOW DO A -debug check-database TO ADJUST CELL SIZES"));
}

void db_adjusttree(NODEPROTO *np)
{
	REGISTER NODEINST *ni;
	REGISTER INTBIG dlx, dhx, dly, dhy, nlx, nhx, nly, nhy;
	INTBIG lx, hx, ly, hy, offx, offy;
	XARRAY trans;

	/* descend the hierarchy to get the bottom cells first */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex != 0) continue;
		if (ni->proto->temp1 != 0) continue;
		db_adjusttree(ni->proto);
	}

	/* now adjust the cell size */
	np->temp1++;
	db_boundcell(np, &lx, &hx, &ly, &hy);
	if (lx == np->lowx && hx == np->highx && ly == np->lowy && hy == np->highy) return;

	/* adjust all instances of the cell */
	for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
	{
		undogeom(ni->geom, ni->parent);
		makeangle(ni->rotation, ni->transpose, trans);
		dlx = lx - np->lowx;   dhx = hx - np->highx;
		dly = ly - np->lowy;   dhy = hy - np->highy;
		xform(dhx+dlx, dhy+dly, &offx, &offy, trans);
		nlx = (dlx-dhx+offx) / 2;   nhx = offx - nlx;
		nly = (dly-dhy+offy) / 2;   nhy = offy - nly;
		ni->lowx += nlx;   ni->highx += nhx;
		ni->lowy += nly;   ni->highy += nhy;
		linkgeom(ni->geom, ni->parent);
	}
	np->lowx = lx;   np->highx = hx;
	np->lowy = ly;   np->highy = hy;
}

/******************** NODEINST DESCRIPTION ********************/

/*
 * routine to fill polygon "poly" with a description of box "box" of MOS
 * transistor node "ni".  The assumption is that this box is shortened
 * because of a nonmanhattan arc attaching to the transistor.  The graphical
 * layer information is in "lay", the value of lambda is in "lambda", and
 * the port information is in "portstruct".  The assumption of this port
 * structure and the transistor in general is that ports 0 and 2 are the
 * polysilicon and ports 1 and 3 are the diffusion.  Also, box 0 is the
 * diffusion and box 1 is the polysilicon.
 */
void tech_shortenmostrans(NODEINST *ni, POLYGON *poly, TECH_POLYGON *lay, INTBIG lambda,
	INTBIG box, TECH_PORTS *portstruct)
{
	INTBIG lx, hx, ly, hy;
	REGISTER INTBIG wid, swid, len, x1, y1, x2, y2, shrink1, shrink2, ang, end1, end2, dist,
		halflength;
	REGISTER ARCINST *ai;
	REGISTER PORTARCINST *pi, *pi1, *pi2;

	/* compute the box geometry */
	subrange(ni->lowx, ni->highx, lay->points[0], lay->points[1], lay->points[4], lay->points[5],
		&lx, &hx, lambda);
	subrange(ni->lowy, ni->highy, lay->points[2], lay->points[3], lay->points[6], lay->points[7],
		&ly, &hy, lambda);
	if (box == 0)
	{
		/* find the arcs on this, the vertical diffusion box */
		pi1 = pi2 = NOPORTARCINST;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			if (pi->proto == portstruct[1].addr) pi1 = pi;
			if (pi->proto == portstruct[3].addr) pi2 = pi;
		}

		/* specify the box */
		x1 = x2 = (hx+lx) / 2;
		swid = wid = hx-lx;
		halflength = abs(hy-ly) / 2;
		if (pi1 != NOPORTARCINST)
		{
			if (pi1->conarcinst->width < wid) pi1 = NOPORTARCINST; else
				swid = mini(swid, pi1->conarcinst->width);
		}
		if (pi2 != NOPORTARCINST)
		{
			if (pi2->conarcinst->width < wid) pi2 = NOPORTARCINST; else
				swid = mini(swid, pi2->conarcinst->width);
		}
		y1 = hy - swid / 2;   y2 = ly + swid / 2;
		len = y1-y2;
	} else
	{
		/* find the arcs on this, the horizontal polysilicon box */
		pi1 = pi2 = NOPORTARCINST;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			if (pi->proto == portstruct[0].addr) pi1 = pi;
			if (pi->proto == portstruct[2].addr) pi2 = pi;
		}

		/* specify the box */
		y1 = y2 = (hy+ly) / 2;
		swid = wid = hy-ly;
		halflength = abs(hx-lx) / 2;
		if (pi1 != NOPORTARCINST)
		{
			if (pi1->conarcinst->width < wid) pi1 = NOPORTARCINST; else
				swid = mini(swid, pi1->conarcinst->width);
		}
		if (pi2 != NOPORTARCINST)
		{
			if (pi2->conarcinst->width < wid) pi2 = NOPORTARCINST; else
				swid = mini(swid, pi2->conarcinst->width);
		}
		x1 = lx + swid / 2;   x2 = hx - swid / 2;
		len = x2-x1;
	}
	shrink1 = shrink2 = 0;
	if (pi1 != NOPORTARCINST)
	{
		ai = pi1->conarcinst;
		if (ai->end[0].nodeinst == ni && ai->end[0].portarcinst->proto == pi1->proto)
		{
			end1 = 0;
			shrink1 = ai->endshrink & 0xFFFF;
		} else
		{
			end1 = 1;
			shrink1 = (ai->endshrink >> 16) & 0xFFFF;
		}
	}
	if (pi2 != NOPORTARCINST)
	{
		ai = pi2->conarcinst;
		if (ai->end[0].nodeinst == ni && ai->end[0].portarcinst->proto == pi2->proto)
		{
			end2 = 0;
			shrink2 = ai->endshrink & 0xFFFF;
		} else
		{
			end2 = 1;
			shrink2 = (ai->endshrink >> 16) & 0xFFFF;
		}
	}
	if (shrink1 == 0 && shrink2 == 0)
	{
		if (poly->limit < 4) (void)extendpolygon(poly, 4);
		poly->xv[1] = poly->xv[0] = lx;   poly->xv[3] = poly->xv[2] = hx;
		poly->yv[3] = poly->yv[0] = ly;   poly->yv[2] = poly->yv[1] = hy;
		poly->count = 4;
	} else
	{
		if (shrink1 == 0) shrink1 = wid / 2; else
		{
			dist = computedistance(ai->end[end1].xpos, ai->end[end1].ypos,
				(ni->geom->lowx+ni->geom->highx)/2, (ni->geom->lowy+ni->geom->highy)/2);
			shrink1 = wid / 2 + tech_getextendfactor(wid, shrink1);
			shrink1 -= halflength - dist;
		}
		if (shrink2 == 0) shrink2 = wid / 2; else
		{
			dist = computedistance(ai->end[end2].xpos, ai->end[end2].ypos,
				(ni->geom->lowx+ni->geom->highx)/2, (ni->geom->lowy+ni->geom->highy)/2);
			shrink2 = wid / 2 + tech_getextendfactor(wid, shrink2);
			shrink2 -= halflength - dist;
		}
		ang = figureangle(x1, y1, x2, y2);
		tech_makeendpointpoly(len, wid, ang, x1,y1, shrink1, x2,y2, shrink2, poly);
	}
	poly->layer = lay->layernum;
	poly->style = lay->style;
	if (poly->style == FILLEDRECT) poly->style = FILLED; else
		if (poly->style == CLOSEDRECT) poly->style = CLOSED;
}

/*
 * routine to convert extension factors into distances.  For an arc of
 * width "wid", the routine returns the proper extension distance for a
 * factor of "extend".
 *
 * Arcs are typically drawn with an extension of half their width.  When arcs
 * are nonmanhattan, their ends are extended by a variable amount so that
 * little tabs don't appear at the places where they overlap.  The
 * "endshrink" factor on an arcinst determines the angle at which the ends
 * meet other arcinsts and therefore, the amount of extension that must be
 * applied.  Since angles greater than 90 are reflective cases of the angles
 * less than 90, the extension factor is a number from 0 to 90 where a small
 * value indicates that the ends should not extend much and an extension of
 * 90 indicates full extension.  The exception is the value 0 which also
 * indicates full extension.  The formula is that the end of the arcinst
 * extends beyond the actual terminus by:
 *    (half arcinst width) / tan(extension/2)
 * This works out correctly for a extension of 90 because the tangent of 45
 * is 1 and a manhattan connection should extend beyond its terminus by half
 * its width.  The "extendfactor" table has the values of:
 *     100 * tan(extension/2)
 * for the values of extension from 0 to 90.
 */
INTBIG tech_getextendfactor(INTBIG wid, INTBIG extend)
{
	static INTBIG extendfactor[] = {0,
		11459, 5729, 3819, 2864, 2290, 1908, 1635, 1430, 1271, 1143,
		 1039,  951,  878,  814,  760,  712,  669,  631,  598,  567,
		  540,  514,  492,  470,  451,  433,  417,  401,  387,  373,
		  361,  349,  338,  327,  317,  308,  299,  290,  282,  275,
		  267,  261,  254,  248,  241,  236,  230,  225,  219,  214,
		  210,  205,  201,  196,  192,  188,  184,  180,  177,  173,
		  170,  166,  163,  160,  157,  154,  151,  148,  146,  143,
		  140,  138,  135,  133,  130,  128,  126,  123,  121,  119,
		  117,  115,  113,  111,  109,  107,  105,  104,  102,  100};

	/* compute the amount of extension (from 0 to wid/2) */
	if (extend <= 0) return(wid/2);

	/* values should be from 0 to 90, but check anyway */
	if (extend > 90) return(wid/2);

	/* return correct extension */
	return(wid * 50 / extendfactor[extend]);
}

/*
 * routine to determine whether pin display should be supressed by counting
 * the number of arcs and seeing if there are one or two and also by seeing if
 * the node has exports (then draw it if there are three or more).
 * Returns true if the pin should be supressed.  If "win" is NOWINDOWPART, then
 * see if this node is visible in that window and include arcs connected at
 * higher levels of hierarchy.
 */
BOOLEAN tech_pinusecount(NODEINST *ni, WINDOWPART *win)
{
	REGISTER INTBIG i;
	INTBIG depth;
	REGISTER NODEINST *upni;
	NODEINST **nilist;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;

	/* count the number of arcs on this node */
	i = 0;
	for (pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		i++;

	/* if the next level up the hierarchy is visible, consider arcs connected there */
	if (win != NOWINDOWPART && ni->firstportexpinst != NOPORTEXPINST)
	{
		db_gettraversalpath(ni->parent, win, &nilist, &depth);
		if (depth == 1)
		{
			upni = nilist[0];
			if (upni->proto == ni->parent && upni->parent == win->curnodeproto)
			{
				/* make sure there is at least one connection for the icon */
				if (i == 0) i++;
				for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
				{
					for (pi = upni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
						if (pi->proto == pe->exportproto) i++;
				}
			}
		}
	}

	/* now decide whether or not to supress the pin */
	if (i > 2) return(FALSE);
	if (ni->firstportexpinst != NOPORTEXPINST) return(TRUE);
	if (i == 0) return(FALSE);
	return(TRUE);
}

/*
 * routine to return the number of contact cuts needed for node "ni" and to
 * setup the globals for the subsequent calls to "tech_moscutpoly".  The size
 * of a single cut is "cutsizex" by "cutsizey", the indentation from the edge
 * of the node is in "cutindent", and the separation between cuts is "cutsep",
 * all given in WHOLE fractions of lambda.  "reasonable" is loaded with a
 * smaller number of polygons to use when too many become prohibitive.
 */
INTBIG tech_moscutcount(NODEINST *ni, INTBIG cutsizex, INTBIG cutsizey, INTBIG cutindent,
	INTBIG cutsep, INTBIG *reasonable, POLYLOOP *pl)
{
	REGISTER INTBIG lambda, lx, hx, ly, hy;

	/* get the lambda value */
	lambda = lambdaofnode(ni);
	pl->moscutsizex = cutsizex;
	pl->moscutsizey = cutsizey;
	pl->moscutsep = cutsep;
	cutsizex = cutsizex * lambda / WHOLE;
	cutsizey = cutsizey * lambda / WHOLE;
	cutindent = cutindent * lambda / WHOLE;
	cutsep = cutsep * lambda / WHOLE;

	/* determine the actual node size */
	nodesizeoffset(ni, &pl->moscutlx, &pl->moscutly, &pl->moscuthx, &pl->moscuthy);
	lx = ni->lowx + pl->moscutlx;   hx = ni->highx - pl->moscuthx;
	ly = ni->lowy + pl->moscutly;   hy = ni->highy - pl->moscuthy;

	/* number of cuts depends on the size */
	pl->moscutsx = (hx-lx-cutindent*2+cutsep) / (cutsizex+cutsep);
	pl->moscutsy = (hy-ly-cutindent*2+cutsep) / (cutsizey+cutsep);
	if (pl->moscutsx <= 0) pl->moscutsx = 1;
	if (pl->moscutsy <= 0) pl->moscutsy = 1;
	pl->moscuttotal = pl->moscutsx * pl->moscutsy;
	*reasonable = pl->moscuttotal;
	if (pl->moscuttotal != 1)
	{
		/* prepare for the multiple contact cut locations */
		pl->moscutbasex = (hx-lx-cutindent*2 - cutsizex*pl->moscutsx -
			cutsep*(pl->moscutsx-1)) * WHOLE / lambda / 2 +
				(pl->moscutlx + cutindent) * WHOLE / lambda;
		pl->moscutbasey = (hy-ly-cutindent*2 - cutsizey*pl->moscutsy -
			cutsep*(pl->moscutsy-1)) * WHOLE / lambda / 2 +
				(pl->moscutly + cutindent) * WHOLE / lambda;
		if (pl->moscutsx > 2 && pl->moscutsy > 2)
		{
			*reasonable = pl->moscutsx * 2 + (pl->moscutsy-2) * 2;
			pl->moscuttopedge = pl->moscutsx*2;
			pl->moscutleftedge = pl->moscutsx*2 + pl->moscutsy-2;
			pl->moscutrightedge = pl->moscutsx*2 + (pl->moscutsy-2)*2;
		}
	}
	return(pl->moscuttotal);
}

/*
 * routine to fill in the contact cuts of a MOS contact when there are
 * multiple cuts.  Node is in "ni" and the contact cut number (0 based) is
 * in "cut".  The array describing the cut is filled into "descrip" which is:
 *     INTBIG descrip[8] = {-H0, K1,-H0, K1, -H0, K3,-H0, K3};
 * This routine presumes that "tech_moscutcount" has already been called
 * so that the globals "tech_moscutlx", "tech_moscuthx", "tech_moscutly",
 * "tech_moscuthy", "tech_moscutbasex", "tech_moscutbasey", "tech_moscuttotal",
 * "tech_moscutsizex", "tech_moscutsizey", "tech_moscuttopedge",
 * "tech_moscutleftedge", and "tech_moscutrightedge" are set.
 */
void tech_moscutpoly(NODEINST *ni, INTBIG cut, INTBIG descrip[], POLYLOOP *pl)
{
	REGISTER INTBIG lambda, cutx, cuty;

	lambda = lambdaofnode(ni);

	if (pl->moscutsx > 2 && pl->moscutsy > 2)
	{
		/* rearrange cuts so that the initial ones go around the outside */
		if (cut < pl->moscutsx)
		{
			/* bottom edge: it's ok as is */
			/* EMPTY */ 
		} else if (cut < pl->moscuttopedge)
		{
			/* top edge: shift up */
			cut += pl->moscutsx * (pl->moscutsy-2);
		} else if (cut < pl->moscutleftedge)
		{
			/* left edge: rearrange */
			cut = (cut - pl->moscuttopedge) * pl->moscutsx + pl->moscutsx;
		} else if (cut < pl->moscutrightedge)
		{
			/* right edge: rearrange */
			cut = (cut - pl->moscutleftedge) * pl->moscutsx + pl->moscutsx*2-1;
		} else
		{
			/* center: rearrange and scale down */
			cut = cut - pl->moscutrightedge;
			cutx = cut % (pl->moscutsx-2);
			cuty = cut / (pl->moscutsx-2);
			cut = cuty * pl->moscutsx + cutx+pl->moscutsx+1;
		}
	}

	if (pl->moscutsx == 1)
	{
		descrip[1] = (ni->highx-ni->lowx)/2 * WHOLE/lambda - pl->moscutsizex/2;
		descrip[5] = (ni->highx-ni->lowx)/2 * WHOLE/lambda + pl->moscutsizex/2;
	} else
	{
		descrip[1] = pl->moscutbasex + (cut % pl->moscutsx) * (pl->moscutsizex + pl->moscutsep);
		descrip[5] = descrip[1] + pl->moscutsizex;
	}

	if (pl->moscutsy == 1)
	{
		descrip[3] = (ni->highy-ni->lowy)/2 * WHOLE/lambda - pl->moscutsizey/2;
		descrip[7] = (ni->highy-ni->lowy)/2 * WHOLE/lambda + pl->moscutsizey/2;
	} else
	{
		descrip[3] = pl->moscutbasey + (cut / pl->moscutsx) * (pl->moscutsizey + pl->moscutsep);
		descrip[7] = descrip[3] + pl->moscutsizey;
	}
}

/*
 * helper routine to fill polygon "poly" from the "tech_polygon" structure
 * in "lay" which is on node "ni".  The value of lambda is "lambda" and the
 * style of the polygon (if trace information is used) is "sty"
 */
void tech_fillpoly(POLYGON *poly, TECH_POLYGON *lay, NODEINST *ni, INTBIG lambda,
	INTBIG sty)
{
	REGISTER INTBIG i, lastpoint, count;
	REGISTER INTBIG x, y, *pt, xm, xs, ym, ys;
	INTBIG minlx, minhx, minly, minhy;
	REGISTER VARIABLE *var;

	/* one thing is constant */
	poly->layer = lay->layernum;

	/* see if trace information is present */
	if ((ni->proto->userbits&HOLDSTRACE) != 0)
	{
		var = gettrace(ni);
		if (var != NOVARIABLE)
		{
			count = getlength(var) / 2;
			x = (ni->highx + ni->lowx) / 2;   y = (ni->highy + ni->lowy) / 2;
			if (poly->limit < count) (void)extendpolygon(poly, count);
			for(i=0; i<count; i++)
			{
				poly->xv[i] = ((INTBIG *)var->addr)[i*2] + x;
				poly->yv[i] = ((INTBIG *)var->addr)[i*2+1] + y;
			}
			poly->count = count;
			poly->style = sty;
			return;
		}
	}

	/* normal description from the technology tables */
	switch (lay->representation)
	{
		case BOX:
		case MINBOX:
			lastpoint = 8;
			if (lay->style == FILLEDRECT || lay->style == CLOSEDRECT)
			{
				if (poly->limit < 2) (void)extendpolygon(poly, 2);
				subrange(ni->lowx, ni->highx, lay->points[0], lay->points[1],
					lay->points[4], lay->points[5], &poly->xv[0], &poly->xv[1], lambda);
				subrange(ni->lowy, ni->highy, lay->points[2], lay->points[3],
					lay->points[6], lay->points[7], &poly->yv[0], &poly->yv[1], lambda);
				if (lay->representation == MINBOX)
				{
					/* make sure the box is large enough */
					lastpoint = 16;
					subrange(ni->lowx, ni->highx, lay->points[8], lay->points[9],
						lay->points[12], lay->points[13], &minlx, &minhx, lambda);
					subrange(ni->lowy, ni->highy, lay->points[10], lay->points[11],
						lay->points[14], lay->points[15], &minly, &minhy, lambda);
					if (poly->xv[0] > minlx) poly->xv[0] = minlx;
					if (poly->xv[1] < minhx) poly->xv[1] = minhx;
					if (poly->yv[0] > minly) poly->yv[0] = minly;
					if (poly->yv[1] < minhy) poly->yv[1] = minhy;
				}
			} else
			{
				if (poly->limit < 4) (void)extendpolygon(poly, 4);
				subrange(ni->lowx, ni->highx, lay->points[0], lay->points[1],
					lay->points[4], lay->points[5], &poly->xv[0], &poly->xv[2], lambda);
				subrange(ni->lowy, ni->highy, lay->points[2], lay->points[3],
					lay->points[6], lay->points[7], &poly->yv[0], &poly->yv[1], lambda);
				if (lay->representation == MINBOX)
				{
					/* make sure the box is large enough */
					lastpoint = 16;
					subrange(ni->lowx, ni->highx, lay->points[8], lay->points[9],
						lay->points[10], lay->points[13], &minlx, &minhx, lambda);
					subrange(ni->lowy, ni->highy, lay->points[10], lay->points[11],
						lay->points[14], lay->points[15], &minly, &minhy, lambda);
					if (poly->xv[0] > minlx) poly->xv[0] = minlx;
					if (poly->xv[2] < minhx) poly->xv[2] = minhx;
					if (poly->yv[0] > minly) poly->yv[0] = minly;
					if (poly->yv[1] < minhy) poly->yv[1] = minhy;
				}
				poly->xv[1] = poly->xv[0];   poly->xv[3] = poly->xv[2];
				poly->yv[3] = poly->yv[0];   poly->yv[2] = poly->yv[1];
			}
			break;

		case POINTS:
			pt = &lay->points[0];
			if (poly->limit < lay->count) (void)extendpolygon(poly, lay->count);
			for(i=0; i<lay->count; i++)
			{
				xm = *pt++;   xs = *pt++;   ym = *pt++;   ys = *pt++;
				poly->xv[i] = getrange(ni->lowx,ni->highx, xm,xs, lambda);
				poly->yv[i] = getrange(ni->lowy,ni->highy, ym,ys, lambda);
			}
			lastpoint = lay->count*4;
			break;

		case ABSPOINTS:
			pt = &lay->points[0];
			if (poly->limit < lay->count) (void)extendpolygon(poly, lay->count);
			for(i=0; i<lay->count; i++)
			{
				xs = *pt++;   ys = *pt++;
				poly->xv[i] = getrange(ni->lowx,ni->highx, 0,xs, lambda);
				poly->yv[i] = getrange(ni->lowy,ni->highy, 0,ys, lambda);
			}
			lastpoint = lay->count*2;
			break;
	}
	poly->count = lay->count;
	poly->style = lay->style;
	if (lay->style >= TEXTCENT && lay->style <= TEXTBOX)
		poly->string = (CHAR *)lay->points[lastpoint];
}

/*
 * routine to determine the number of polygons that will compose transistor
 * "ni".  If the transistor is not serpentine, the value supplied in "count"
 * is returned.  Otherwise, one polygon will be drawn for every segment of
 * the serpent times every one of the "count" layers.
 */
INTBIG tech_inittrans(INTBIG count, NODEINST *ni, POLYLOOP *pl)
{
	REGISTER INTBIG total;

	/* see if the transistor has serpentine information */
	pl->serpentvar = gettrace(ni);
	if (pl->serpentvar == NOVARIABLE) return(count);

	/* trace data is there: make sure there are enough points */
	total = getlength(pl->serpentvar);
	if (total < 4)
	{
		pl->serpentvar = NOVARIABLE;
		return(count);
	}

	/* return the number of polygons */
	return(count * (total/2 - 1));
}

/*
 * Version Jan.11/88 of tech_filltrans() and tech_filltransport() that
 * draws the diffusions and ports as they are described in the technology
 * file.
 */

#define LEFTANGLE   900
#define RIGHTANGLE 2700
/*
 * routine to describe box "box" of transistor "ni" that may be part of a
 * serpentine path.  If the variable "trace" exists on the node, get that
 * x/y/x/y information as the centerline of the serpentine path.  The "laylist"
 * structure describes the transistor: "lwidth" and "rwidth" are the extension
 * of the active layer to the left and right; "extendt/extendb" are the extension
 * of the poly layer on the top and bottom ends.  The outline is
 * placed in the polygon "poly".  Layer information for this polygon is in
 * "lay", port information is in "portstruct", and the value of lambda is in
 * "lambda".  In all manhattan cases, nonoverlapping polygons are
 * constructed for consecutive segments; the current segment extends to
 * the furthest boundary of the next, and the next is truncated where it
 * overlaps the previous polygon.
 * NOTE: For each trace segment, the left hand side of the trace
 * will contain the polygons that appear ABOVE the gate in the node
 * definition. That is, the "top" port and diffusion will be above a
 * gate segment that extends from left to right, and on the left of a
 * segment that goes from bottom to top.
 */
void tech_filltrans(POLYGON *poly, TECH_POLYGON **lay, TECH_SERPENT *laylist,
	NODEINST *ni, INTBIG lambda, INTBIG box, TECH_PORTS *portstruct, POLYLOOP *pl)
{
	REGISTER INTBIG angle, ang;
	REGISTER INTBIG thissg, next, total, segment, element, otherang;
	REGISTER INTBIG sin, cos, xoff, yoff, thisxl, thisyl, thisxr, thisyr, lwid,
		rwid, extendt, extendb, scale, thisx, thisy, nextx, nexty, *list,
		nextxl, nextyl, nextxr, nextyr, otherx, othery;
	INTBIG x, y, ly, hy, xl, xr, yl, yr;
	REGISTER VARIABLE *varw;

	/* nonserpentine transtors fill in the normal way */
	if (pl->serpentvar == NOVARIABLE)
	{
		*lay = &laylist[box].basics;
		if (portstruct == 0) tech_fillpoly(poly, *lay, ni, lambda, -1); else
			tech_shortenmostrans(ni, poly, *lay, lambda, box, portstruct);
		return;
	}

	/* compute the segment (along the serpent) and element (of transistor) */
	total = getlength(pl->serpentvar) / 2 - 1;
	segment = box % total;
	element = box / total;

	/* see if nonstandard width is specified */
	lwid = lambda * laylist[element].lwidth / WHOLE;
	rwid = lambda * laylist[element].rwidth / WHOLE;
	extendt = lambda * laylist[element].extendt / WHOLE;
	extendb = lambda * laylist[element].extendb / WHOLE;
	varw = getvalkey((INTBIG)ni, VNODEINST, VFRACT, el_transistor_width_key);
	if (varw != NOVARIABLE)
	{
		nodesizeoffset(ni, &x, &ly, &x, &hy);
		scale = varw->addr * lambda / WHOLE - ni->proto->highy+ni->proto->lowy+hy+ly;
		lwid += scale / 2;
		rwid += scale / 2;
	}

	/* prepare to fill the serpentine transistor */
	list = (INTBIG *)pl->serpentvar->addr;
	xoff = (ni->highx+ni->lowx)/2;
	yoff = (ni->highy+ni->lowy)/2;
	thissg = segment;   next = segment+1;
	thisx = list[thissg*2];   thisy = list[thissg*2+1];
	nextx = list[next*2];   nexty = list[next*2+1];
	angle = figureangle(thisx, thisy, nextx, nexty);

	/* push the points at the ends of the transistor */
	if (thissg == 0)
	{
		/* extend "thissg" 180 degrees back */
		ang = (angle+1800) % 3600;
		thisx += mult(cosine(ang), extendt);
		thisy += mult(sine(ang), extendt);
	}
	if (next == total)
	{
		/* extend "next" 0 degrees forward */
		nextx += mult(cosine(angle), extendb);
		nexty += mult(sine(angle), extendb);
	}

	/* compute endpoints of line parallel to and left of center line */
	ang = (angle+LEFTANGLE) % 3600;
	sin = mult(sine(ang), lwid);   cos = mult(cosine(ang), lwid);
	thisxl = thisx + cos;   thisyl = thisy + sin;
	nextxl = nextx + cos;   nextyl = nexty + sin;

	/* compute endpoints of line parallel to and right of center line */
	ang = (angle+RIGHTANGLE) % 3600;
	sin = mult(sine(ang), rwid);   cos = mult(cosine(ang), rwid);
	thisxr = thisx + cos;   thisyr = thisy + sin;
	nextxr = nextx + cos;   nextyr = nexty + sin;

	/* determine proper intersection of this and the previous segment */
	if (thissg != 0)
	{
		otherx = list[thissg*2-2];   othery = list[thissg*2-1];
		otherang = figureangle(otherx,othery, thisx,thisy);
		if (otherang != angle)
		{
			/* special case for completely orthogonal wires */
			if (angle%900 == 0 && otherang%900 == 0)
			{
				/* Do nonoverlapping extensions */
				ang = (otherang+LEFTANGLE) % 3600;
				xl = otherx + mult(cosine(ang), lwid);
				yl = othery + mult(sine(ang), lwid);
				ang = (otherang+RIGHTANGLE) % 3600;
				xr = otherx + mult(cosine(ang), rwid);
				yr = othery + mult(sine(ang), rwid);
				switch (angle)
				{
					case 0:    thisxr = thisxl = mini(xl,xr);   break;
					case 1800: thisxr = thisxl = maxi(xl,xr);   break;
					case 900:  thisyr = thisyl = mini(yl,yr);   break;
					case 2700: thisyr = thisyl = maxi(yl,yr);   break;
				}
			} else	      /* NonManhattan */
			{
				ang = (otherang+LEFTANGLE) % 3600;
				(void)intersect(thisx+mult(cosine(ang),lwid), thisy+mult(sine(ang),lwid),
					otherang, thisxl,thisyl,angle, &x, &y);
				thisxl = x;   thisyl = y;
				ang = (otherang+RIGHTANGLE) % 3600;
				(void)intersect(thisx+mult(cosine(ang),rwid), thisy+mult(sine(ang),rwid),
					otherang, thisxr,thisyr,angle, &x, &y);
				thisxr = x;   thisyr = y;
			}
		}
	}

	/* determine proper intersection of this and the next segment */
	if (next != total)
	{
		otherx = list[next*2+2];   othery = list[next*2+3];
		otherang = figureangle(nextx, nexty, otherx,othery);
		if (otherang != angle)
		{
			/* special case for completely orthogonal wires */
			if (angle%900 == 0 && otherang%900 == 0)
			{
				/* Do nonoverlapping extensions */
				ang = (otherang+LEFTANGLE) % 3600;
				xl = nextx + mult(cosine(ang), lwid);
				yl = nexty + mult(sine(ang), lwid);
				ang = (otherang+RIGHTANGLE) % 3600;
				xr = nextx + mult(cosine(ang), rwid);
				yr = nexty + mult(sine(ang), rwid);
				switch (angle)
				{
					case 0:    nextxr = nextxl = maxi(xl,xr);   break;
					case 1800: nextxr = nextxl = mini(xl,xr);   break;
					case 900:  nextyr = nextyl = maxi(yl,yr);   break;
					case 2700: nextyr = nextyl = mini(yl,yr);   break;
				}
			} else
			{
				ang = (otherang+LEFTANGLE) % 3600;
				(void)intersect(nextx+mult(cosine(ang),lwid), nexty+mult(sine(ang),lwid),
					otherang, nextxl,nextyl,angle, &x, &y);
				nextxl = x;   nextyl = y;
				ang = (otherang+RIGHTANGLE) % 3600;
				(void)intersect(nextx+mult(cosine(ang),rwid), nexty+mult(sine(ang),rwid),
					otherang, nextxr,nextyr,angle, &x, &y);
				nextxr = x;   nextyr = y;
			}
		}
	}

	/* fill the polygon */
	if (poly->limit < 4) (void)extendpolygon(poly, 4);
	poly->xv[0] = thisxl+xoff;   poly->yv[0] = thisyl+yoff;
	poly->xv[1] = thisxr+xoff;   poly->yv[1] = thisyr+yoff;
	poly->xv[2] = nextxr+xoff;   poly->yv[2] = nextyr+yoff;
	poly->xv[3] = nextxl+xoff;   poly->yv[3] = nextyl+yoff;
	poly->count = 4;

	/* see if the sides of the polygon intersect */
	ang = figureangle(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1]);
	angle = figureangle(poly->xv[2], poly->yv[2], poly->xv[3], poly->yv[3]);
	if (intersect(poly->xv[0], poly->yv[0], ang, poly->xv[2], poly->yv[2], angle, &x, &y) >= 0)
	{
		/* lines intersect, see if the point is on one of the lines */
		if (x >= mini(poly->xv[0], poly->xv[1]) && x <= maxi(poly->xv[0], poly->xv[1]) &&
			y >= mini(poly->yv[0], poly->yv[1]) && y <= maxi(poly->yv[0], poly->yv[1]))
		{
			if (abs(x-poly->xv[0])+abs(y-poly->yv[0]) > abs(x-poly->xv[1])+abs(y-poly->yv[1]))
			{
				poly->xv[1] = x;   poly->yv[1] = y;
				poly->xv[2] = poly->xv[3];   poly->yv[2] = poly->yv[3];
			} else
			{
				poly->xv[0] = x;   poly->yv[0] = y;
			}
			poly->count = 3;
		}
	}

	*lay = &laylist[element].basics;
	poly->style = (*lay)->style;
	if (poly->style == FILLEDRECT) poly->style = FILLED; else
		if (poly->style == CLOSEDRECT) poly->style = CLOSED;
	poly->layer = (*lay)->layernum;
}

/*
 * routine to describe a port in a transistor that may be part of a serpentine
 * path.  If the variable "trace" exists on the node, get that x/y/x/y
 * information as the centerline of the serpentine path.  The port path
 * is shrunk by "diffinset" in the length and is pushed "diffextend" from the centerline.
 * The default width of the transistor is "defwid".  The outline is placed
 * in the polygon "poly".
 * The assumptions about directions are:
 * Segments have port 1 to the left, and port 3 to the right of the gate
 * trace. Port 0, the "left-hand" end of the gate, appears at the starting
 * end of the first trace segment; port 2, the "right-hand" end of the gate,
 * appears at the end of the last trace segment.  Port 3 is drawn as a
 * reflection of port 1 around the trace.
 * The values "diffinset", "diffextend", "defwid", "polyinset", and "polyextend"
 * are used to determine the offsets of the ports:
 * The poly ports are extended "polyextend" beyond the appropriate end of the trace
 * and are inset by "polyinset" from the polysilicon edge.
 * The diffusion ports are extended "diffextend" from the polysilicon edge
 * and set in "diffinset" from the ends of the trace segment.
 */
void tech_filltransport(NODEINST *ni, PORTPROTO *pp, POLYGON *poly, XARRAY trans,
	TECH_NODES *nodedata, INTBIG diffinset, INTBIG diffextend, INTBIG defwid,
	INTBIG polyinset, INTBIG polyextend)
{
	REGISTER INTBIG thissg, next, angle, pangle, ang;
	REGISTER INTBIG sin, cos, thisx, thisy, nextx, nexty, pthisx, pthisy;
	INTBIG *list, xoff, yoff, lambda, x, y;
	INTBIG total, which;
	REGISTER VARIABLE *var, *varw;
	REGISTER PORTPROTO *lpp;

	/* see if the transistor has serpentine information */
	var = gettrace(ni);
	if (var != NOVARIABLE)
	{
		/* trace data is there: make sure there are enough points */
		total = getlength(var);
		if (total <= 2) var = NOVARIABLE;
	}

	/* nonserpentine transtors fill in the normal way */
	lambda = lambdaofnode(ni);
	if (var == NOVARIABLE)
	{
		tech_fillportpoly(ni, pp, poly, trans, nodedata, -1, lambda);
		return;
	}

	/* prepare to fill the serpentine transistor port */
	list = (INTBIG *)var->addr;
	poly->style = OPENED;
	xoff = (ni->highx+ni->lowx)/2;
	yoff = (ni->highy+ni->lowy)/2;
	total /= 2;

	/* see if nonstandard width is specified */
	defwid = lambda * defwid / WHOLE;
	diffinset = lambda * diffinset / WHOLE;   diffextend = lambda * diffextend / WHOLE;
	polyinset = lambda * polyinset / WHOLE;   polyextend = lambda * polyextend / WHOLE;
	varw = getvalkey((INTBIG)ni, VNODEINST, VFRACT, el_transistor_width_key);
	if (varw != NOVARIABLE) defwid = lambda * varw->addr / WHOLE;

	/* determine which port is being described */
	for(lpp = ni->proto->firstportproto, which=0; lpp != NOPORTPROTO;
		lpp = lpp->nextportproto, which++) if (lpp == pp) break;

	/* ports 0 and 2 are poly (simple) */
	if (which == 0)
	{
		if (poly->limit < 2) (void)extendpolygon(poly, 2);
		thisx = list[0];   thisy = list[1];
		nextx = list[2];   nexty = list[3];
		angle = figureangle(thisx, thisy, nextx, nexty);
		ang = (angle+1800) % 3600;
		thisx += mult(cosine(ang), polyextend) + xoff;
		thisy += mult(sine(ang), polyextend) + yoff;
		ang = (angle+LEFTANGLE) % 3600;
		nextx = thisx + mult(cosine(ang), defwid/2-polyinset);
		nexty = thisy + mult(sine(ang), defwid/2-polyinset);
		xform(nextx, nexty, &poly->xv[0], &poly->yv[0], trans);
		ang = (angle+RIGHTANGLE) % 3600;
		nextx = thisx + mult(cosine(ang), defwid/2-polyinset);
		nexty = thisy + mult(sine(ang), defwid/2-polyinset);
		xform(nextx, nexty, &poly->xv[1], &poly->yv[1], trans);
		poly->count = 2;
		return;
	}
	if (which == 2)
	{
		if (poly->limit < 2) (void)extendpolygon(poly, 2);
		thisx = list[(total-1)*2];   thisy = list[(total-1)*2+1];
		nextx = list[(total-2)*2];   nexty = list[(total-2)*2+1];
		angle = figureangle(thisx, thisy, nextx, nexty);
		ang = (angle+1800) % 3600;
		thisx += mult(cosine(ang), polyextend) + xoff;
		thisy += mult(sine(ang), polyextend) + yoff;
		ang = (angle+LEFTANGLE) % 3600;
		nextx = thisx + mult(cosine(ang), defwid/2-polyinset);
		nexty = thisy + mult(sine(ang), defwid/2-polyinset);
		xform(nextx, nexty, &poly->xv[0], &poly->yv[0], trans);
		ang = (angle+RIGHTANGLE) % 3600;
		nextx = thisx + mult(cosine(ang), defwid/2-polyinset);
		nexty = thisy + mult(sine(ang), defwid/2-polyinset);
		xform(nextx, nexty, &poly->xv[1], &poly->yv[1], trans);
		poly->count = 2;
		return;
	}

	/* THE ORIGINAL CODE TREATED PORT 1 AS THE NEGATED PORT ... SRP */
	/* port 3 is the negated path side of port 1 */
	if (which == 3)
	{
		diffextend = -diffextend;
		defwid = -defwid;
	}

	/* extra port on some n-transistors */
	if (which == 4) diffextend = defwid = 0;

	/* polygon will need total points */
	if (poly->limit < total) (void)extendpolygon(poly, total);

	for(next=1; next<total; next++)
	{
		thissg = next-1;
		thisx = list[thissg*2];   thisy = list[thissg*2+1];
		nextx = list[next*2];   nexty = list[next*2+1];
		angle = figureangle(thisx, thisy, nextx, nexty);

		/* determine the points */
		if (thissg == 0)
		{
			/* extend "thissg" 0 degrees forward */
			thisx += mult(cosine(angle), diffinset);
			thisy += mult(sine(angle), diffinset);
		}
		if (next == total-1)
		{
			/* extend "next" 180 degrees back */
			ang = (angle+1800) % 3600;
			nextx += mult(cosine(ang), diffinset);
			nexty += mult(sine(ang), diffinset);
		}

		/* compute endpoints of line parallel to center line */
		ang = (angle+LEFTANGLE) % 3600;   sin = sine(ang);   cos = cosine(ang);
		thisx += mult(cos, defwid/2+diffextend);   thisy += mult(sin, defwid/2+diffextend);
		nextx += mult(cos, defwid/2+diffextend);   nexty += mult(sin, defwid/2+diffextend);

		if (thissg != 0)
		{
			/* compute intersection of this and previous line */

			/* LINTED "pthisx", "pthisy", and "pangle" used in proper order */
			(void)intersect(pthisx, pthisy, pangle, thisx, thisy, angle, &x, &y);
			thisx = x;   thisy = y;
			xform(thisx+xoff, thisy+yoff, &poly->xv[thissg], &poly->yv[thissg], trans);
		} else
			xform(thisx+xoff, thisy+yoff, &poly->xv[0], &poly->yv[0], trans);
		pthisx = thisx;   pthisy = thisy;
		pangle = angle;
	}

	xform(nextx+xoff, nexty+yoff, &poly->xv[total-1], &poly->yv[total-1], trans);
	poly->count = total;
}

/*
 * routine to compute the number of displayable polygons on nodeinst "ni"
 */
INTBIG tech_displayablenvars(NODEINST *ni, WINDOWPART *win, POLYLOOP *pl)
{
	pl->numvar = ni->numvar;
	pl->firstvar = ni->firstvar;
	if (ni->proto == gen_invispinprim)
	{
		if ((us_useroptions&HIDETXTNONLAY) != 0) return(0);
	} else
	{
		if ((us_useroptions&HIDETXTNODE) != 0) return(0);
	}
	return(tech_displayableanyvars(win, ni->parent, pl));
}

/*
 * routine to fill polygon "poly" with the next displayable variable
 * on nodeinst "ni".  Returns the address of the variable just filled.
 */
VARIABLE *tech_filldisplayablenvar(NODEINST *ni, POLYGON *poly, WINDOWPART *win,
	VARIABLE **varnoeval, POLYLOOP *pl)
{
	REGISTER NODEPROTO *cell;
	REGISTER VARIABLE *var;
	REGISTER INTBIG lx, hx, ly, hy;
	INTBIG olx, ohx, oly, ohy;
	REGISTER BOOLEAN oldonobjectstate;

	/* track which object we are on */
	oldonobjectstate = db_onanobject;
	if (!db_onanobject)
	{
		db_onanobject = TRUE;
		db_onobjectaddr = (INTBIG)ni;
		db_onobjecttype = VNODEINST;
	}
	db_lastonobjectaddr = (INTBIG)ni;
	db_lastonobjecttype = VNODEINST;

	cell = ni->parent;
	pl->numvar = ni->numvar;
	pl->firstvar = ni->firstvar;
	if (cell->tech == NOTECHNOLOGY)
		cell->tech = whattech(cell);
	lx = ni->lowx;   hx = ni->highx;
	ly = ni->lowy;   hy = ni->highy;
	nodesizeoffset(ni, &olx, &oly, &ohx, &ohy);
	lx += olx;   hx -= ohx;
	ly += oly;   hy -= ohy;
	var = tech_filldisplayableanyvar(poly, lx, hx, ly, hy, (INTBIG)ni, VNODEINST,
		cell->tech, win, cell, varnoeval, pl);
	db_onanobject = oldonobjectstate;
	return(var);
}

/*
 * routine to compute the number of displayable polygons on nodeproto "np"
 */
INTBIG tech_displayablecellvars(NODEPROTO *np, WINDOWPART *win, POLYLOOP *pl)
{
	pl->numvar = np->numvar;
	pl->firstvar = np->firstvar;
	if ((us_useroptions&HIDETXTCELL) != 0) return(0);
	return(tech_displayableanyvars(win, np, pl));
}

/*
 * routine to fill polygon "poly" with the next displayable variable
 * on nodeproto "np".  Returns the address of the variable just filled.
 */
VARIABLE *tech_filldisplayablecellvar(NODEPROTO *np, POLYGON *poly, WINDOWPART *win,
	VARIABLE **varnoeval, POLYLOOP *pl)
{
	pl->numvar = np->numvar;
	pl->firstvar = np->firstvar;
	if (np->tech == NOTECHNOLOGY) np->tech = whattech(np);

	/* cell variables are offset from (0,0) */
	return(tech_filldisplayableanyvar(poly, 0, 0, 0, 0, (INTBIG)np,
		VNODEPROTO, np->tech, win, np, varnoeval, pl));
}

/******************** PORTINST DESCRIPTION ********************/

/*
 * routine to fill polygon "poly" with the bounding box of port "pp" on
 * nodeinst "ni".  The port description should be transformed by "trans"
 * and the node is further described by the internal data in "nodedata".
 * If "sty" is not -1, the outline is described in trace data on the node.
 */
void tech_fillportpoly(NODEINST *ni, PORTPROTO *pp, POLYGON *poly, XARRAY trans,
	TECH_NODES *nodedata, INTBIG sty, INTBIG lambda)
{
	REGISTER INTBIG i, count;
	REGISTER INTBIG x, y;
	INTBIG lx, ly, hx, hy;
	REGISTER TECH_PORTS *portdata;
	REGISTER VARIABLE *var;

	/* if the node has trace data, use it */
	if ((ni->proto->userbits&HOLDSTRACE) != 0)
	{
		var = gettrace(ni);
		if (var != NOVARIABLE)
		{
			count = getlength(var) / 2;
			if (poly->limit < count) (void)extendpolygon(poly, count);
			x = (ni->highx + ni->lowx) / 2;   y = (ni->highy + ni->lowy) / 2;
			for(i=0; i<count; i++)
				xform(((INTBIG *)var->addr)[i*2]+x, ((INTBIG *)var->addr)[i*2+1]+y,
					&poly->xv[i], &poly->yv[i], trans);
			poly->count = count;
			poly->style = sty;
			return;
		}
	}

	/* find the port to get its boundary information */
	for(i=0; i<nodedata->portcount; i++)
	{
		portdata = &nodedata->portlist[i];
		if (portdata->addr == pp) break;
	}

	/* get the high/low X/Y of the port area */
	subrange(ni->lowx, ni->highx, portdata->lowxmul, portdata->lowxsum,
		portdata->highxmul, portdata->highxsum, &lx, &hx, lambda);
	subrange(ni->lowy, ni->highy, portdata->lowymul, portdata->lowysum,
		portdata->highymul, portdata->highysum, &ly, &hy, lambda);

	/* clip to the size of the node */
	if (hx < lx) lx = hx = (lx + hx) / 2;
	if (hy < ly) ly = hy = (ly + hy) / 2;

	/* transform this into the port polygon description */
	if (poly->limit < 4) (void)extendpolygon(poly, 4);
	xform(lx, ly, &poly->xv[0], &poly->yv[0], trans);
	xform(lx, hy, &poly->xv[1], &poly->yv[1], trans);
	xform(hx, hy, &poly->xv[2], &poly->yv[2], trans);
	xform(hx, ly, &poly->xv[3], &poly->yv[3], trans);
	poly->count = 4;
	poly->style = FILLED;
}

/*
 * routine to tell whether end "e" of arcinst "ai" would be properly connected
 * to its nodeinst if it were located at (x, y), but considering the width of
 * the arc as a limiting factor.
 */
BOOLEAN db_stillinport(ARCINST *ai, INTBIG e, INTBIG x, INTBIG y)
{
	REGISTER INTBIG wid;
	REGISTER BOOLEAN isin;
	REGISTER POLYGON *poly;

	/* make sure there is a polygon */
	poly = allocpolygon(4, db_cluster);

	/* determine the area of the nodeinst */
	shapeportpoly(ai->end[e].nodeinst, ai->end[e].portarcinst->proto, poly, FALSE);
	wid = ai->width - arcwidthoffset(ai);
	reduceportpoly(poly, ai->end[e].nodeinst, ai->end[e].portarcinst->proto, wid,
		(ai->userbits&AANGLE)>>AANGLESH);
	isin = isinside(x, y, poly);
	freepolygon(poly);
	if (isin) return(TRUE);

	/* no good */
	return(FALSE);
}

/*
 * routine to compute the number of displayable polygons on portproto "pp"
 */
INTBIG tech_displayableportvars(PORTPROTO *pp, WINDOWPART *win, POLYLOOP *pl)
{
	pl->numvar = pp->numvar;
	pl->firstvar = pp->firstvar;
	if ((us_useroptions&HIDETXTEXPORT) != 0) return(0);
	return(tech_displayableanyvars(win, pp->parent, pl));
}

/*
 * routine to fill polygon "poly" with the next displayable variable
 * on nodeproto "np".  Returns the address of the variable just filled.
 */
VARIABLE *tech_filldisplayableportvar(PORTPROTO *pp, POLYGON *poly, WINDOWPART *win,
	VARIABLE **varnoeval, POLYLOOP *pl)
{
	INTBIG x, y;
	REGISTER INTSML saverot, savetrn;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *cell;

	cell = pp->parent;
	ni = pp->subnodeinst;
	saverot = ni->rotation;   savetrn = ni->transpose;
	ni->rotation = ni->transpose = 0;
	portposition(ni, pp->subportproto, &x, &y);
	ni->rotation = saverot;   ni->transpose = savetrn;
	pl->numvar = pp->numvar;
	pl->firstvar = pp->firstvar;
	if (cell->tech == NOTECHNOLOGY)
		cell->tech = whattech(cell);
	return(tech_filldisplayableanyvar(poly, x, x, y, y,
		(INTBIG)pp, VPORTPROTO, cell->tech, win, cell, varnoeval, pl));
}

/******************** ARCINST DESCRIPTION ********************/

/*
 * routine to handle initialization of arcs that are curved.  Decomposition
 * into segments is necessary when arcs have a nonzero width.  The routine
 * returns the number of polygons that make up arc "ai".  For straight
 * cases, there are "total" polygons and for curved cases, the true value is
 * computed.
 */
INTBIG tech_initcurvedarc(ARCINST *ai, INTBIG total, POLYLOOP *pl)
{
	REGISTER INTBIG i;
	INTBIG x1, y1, x2, y2;
	REGISTER VARIABLE *var;

	/* by default, set flag for straight arc */
	pl->arcpieces = 0;

	/* see if there is radius information on the arc */
	var = getvalkey((INTBIG)ai, VARCINST, VINTEGER, el_arc_radius_key);
	if (var == NOVARIABLE) return(total);

	/* get the radius of the circle, check for validity */
	pl->radius = var->addr;
	if (abs(pl->radius)*2 < ai->length) return(total);

	/* determine the center of the circle */
	if (findcenters(abs(pl->radius), ai->end[0].xpos, ai->end[0].ypos,
		ai->end[1].xpos, ai->end[1].ypos, ai->length, &x1,&y1, &x2,&y2))
			return(total);

	if (pl->radius < 0)
	{
		pl->radius = -pl->radius;
		pl->centerx = x1;   pl->centery = y1;
	} else
	{
		pl->centerx = x2;   pl->centery = y2;
	}

	/* special case for zero-width arcs */
	if (ai->width == 0)
	{
		pl->arcpieces = -1;
		return(total);
	}

	/* determine the base and range of angles */
	pl->anglebase = figureangle(pl->centerx, pl->centery, ai->end[0].xpos, ai->end[0].ypos);
	pl->anglerange = figureangle(pl->centerx, pl->centery, ai->end[1].xpos, ai->end[1].ypos);
	if ((ai->userbits&REVERSEEND) != 0)
	{
		i = pl->anglebase;
		pl->anglebase = pl->anglerange;
		pl->anglerange = i;
	}
	pl->anglerange -= pl->anglebase;
	if (pl->anglerange < 0) pl->anglerange += 3600;

	/* determine the number of intervals to use for the arc */
	pl->arcpieces = pl->anglerange;
	while (pl->arcpieces > MAXARCPIECES) pl->arcpieces /= 2;
	return(pl->arcpieces*total);
}

/*
 * routine to generate piece "box" of curved arc "ai" and put it in "poly".
 * The arc prototype structure is in "arcprotos".  The routine returns false
 * if successful, true if the arc is not curved.
 */
BOOLEAN tech_curvedarcpiece(ARCINST *ai, INTBIG box, POLYGON *poly, TECH_ARCS **arcprotos,
	POLYLOOP *pl)
{
	REGISTER INTBIG a;
	REGISTER INTBIG aindex, sin, cos, innerradius, outerradius, wid;
	REGISTER TECH_ARCLAY *thista;

	if (pl->arcpieces == 0) return(TRUE);

	aindex = ai->proto->arcindex;

	/* handle zero-width arcs as a true arc polygon */
	if (pl->arcpieces < 0)
	{
		/* initialize the polygon */
		if (poly->limit < 3) (void)extendpolygon(poly, 3);
		poly->count = 3;
		poly->style = CIRCLEARC;
		thista = &arcprotos[aindex]->list[box];
		poly->layer = thista->lay;
		poly->xv[0] = pl->centerx;
		poly->yv[0] = pl->centery;
		if ((ai->userbits&REVERSEEND) == 0)
		{
			poly->xv[1] = ai->end[1].xpos;
			poly->yv[1] = ai->end[1].ypos;
			poly->xv[2] = ai->end[0].xpos;
			poly->yv[2] = ai->end[0].ypos;
		} else
		{
			poly->xv[1] = ai->end[0].xpos;
			poly->yv[1] = ai->end[0].ypos;
			poly->xv[2] = ai->end[1].xpos;
			poly->yv[2] = ai->end[1].ypos;
		}
		return(FALSE);
	}

	/* nonzero width arcs are described in segments */
	thista = &arcprotos[aindex]->list[box/pl->arcpieces];
	box = box % pl->arcpieces;

	/* initialize the polygon */
	if (poly->limit < 4) (void)extendpolygon(poly, 4);
	poly->count = 4;
	poly->style = thista->style;
	poly->layer = thista->lay;

	/* get the inner and outer radii of the arc */
	wid = ai->width - thista->off * lambdaofarc(ai) / WHOLE;
	outerradius = pl->radius + wid / 2;
	innerradius = outerradius - wid;

	/* fill the polygon */
	a = (pl->anglebase + box * pl->anglerange / pl->arcpieces) % 3600;
	sin = sine(a);   cos = cosine(a);
	poly->xv[0] = mult(cos, innerradius) + pl->centerx;
	poly->yv[0] = mult(sin, innerradius) + pl->centery;
	poly->xv[1] = mult(cos, outerradius) + pl->centerx;
	poly->yv[1] = mult(sin, outerradius) + pl->centery;
	a = (pl->anglebase + (box+1) * pl->anglerange / pl->arcpieces) % 3600;
	sin = sine(a);   cos = cosine(a);
	poly->xv[2] = mult(cos, outerradius) + pl->centerx;
	poly->yv[2] = mult(sin, outerradius) + pl->centery;
	poly->xv[3] = mult(cos, innerradius) + pl->centerx;
	poly->yv[3] = mult(sin, innerradius) + pl->centery;
	return(FALSE);
}

/*
 * routine to draw a directional arrow in "poly" for arc "ai"
 */
void tech_makearrow(ARCINST *ai, POLYGON *poly)
{
	REGISTER INTBIG x1, y1, x2, y2, swap;
	REGISTER INTBIG angle;

	x1 = ai->end[0].xpos;   y1 = ai->end[0].ypos;
	x2 = ai->end[1].xpos;   y2 = ai->end[1].ypos;
	angle = ((ai->userbits&AANGLE) >> AANGLESH) * 10;
	if ((ai->userbits&REVERSEEND) != 0)
	{
		swap = x1;   x1 = x2;   x2 = swap;
		swap = y1;   y1 = y2;   y2 = swap;
		angle = (angle+1800) % 3600;
	}
	if (poly->limit < 2) (void)extendpolygon(poly, 2);
	poly->style = VECTORS;
	poly->layer = -1;
	poly->desc = &tech_arrow;
	tech_arrow.col = el_colcell;
	poly->count = 2;
	poly->xv[0] = x1;   poly->yv[0] = y1;
	poly->xv[1] = x2;   poly->yv[1] = y2;
	if ((ai->userbits&NOTEND1) == 0) tech_addheadarrow(poly, angle, x2, y2,
		lambdaofarc(ai));
}

/*
 * helper routine to add an arrow head to the arc in "poly", given that
 * the arc runs from (x,y) and is at an angle of "angle" tenth-degrees
 */
void tech_addheadarrow(POLYGON *poly, INTBIG angle, INTBIG x, INTBIG y, INTBIG lambda)
{
	REGISTER INTBIG c;
	REGISTER INTBIG dist;

	c = poly->count;
	if (poly->limit < c+4) (void)extendpolygon(poly, c+4);
	dist = K1 * lambda / WHOLE;
	poly->xv[c] = x;    poly->yv[c] = y;   c++;
	poly->xv[c] = x + mult(cosine((angle+1500) % 3600), dist);
	poly->yv[c] = y + mult(sine((angle+1500) % 3600), dist);   c++;
	poly->xv[c] = x;    poly->yv[c] = y;   c++;
	poly->xv[c] = x + mult(cosine((angle+2100) % 3600), dist);
	poly->yv[c] = y + mult(sine((angle+2100) % 3600), dist);   c++;
	poly->count = c;
}

/*
 * helper routine to add an arrow head to the arc in "poly", given that
 * the arc runs from (x,y), is "width" wide, and is at an angle of "angle"
 * tenth-degrees.  The body of the arc is a double-line so the end must be shortened
 * too.
 */
void tech_adddoubleheadarrow(POLYGON *poly, INTBIG angle, INTBIG *x,INTBIG *y, INTBIG width)
{
	REGISTER INTBIG c, a;

	c = poly->count;
	if (poly->limit < c+4) (void)extendpolygon(poly, c+4);
	poly->xv[c] = *x;   poly->yv[c] = *y;   c++;
	a = (angle + 1350) % 3600;
	poly->xv[c] = *x + mult(cosine(a), width*2);
	poly->yv[c] = *y + mult(sine(a), width*2);   c++;

	poly->xv[c] = *x;   poly->yv[c] = *y;   c++;
	a = (angle + 2250) % 3600;
	poly->xv[c] = *x + mult(cosine(a), width*2);
	poly->yv[c] = *y + mult(sine(a), width*2);   c++;

	a = (angle + 1800) % 3600;
	*x += mult(cosine(a), width);
	*y += mult(sine(a), width);
	poly->count = c;
}

/*
 * helper routine to add a double-line body to the arc in "poly", given that
 * the arc runs from (x1,y1) to (x2,y2), is "width" wide, and is at an angle
 * of "angle" tenth-degrees.
 */
void tech_add2linebody(POLYGON *poly, INTBIG angle, INTBIG x1, INTBIG y1, INTBIG x2,
	INTBIG y2, INTBIG width)
{
	REGISTER INTBIG c;
	REGISTER INTBIG sin, cos;

	c = poly->count;
	if (poly->limit < c+4) (void)extendpolygon(poly, c+4);
	cos = cosine((angle+900) % 3600);
	sin = sine((angle+900) % 3600);
	poly->xv[c] = x1 + mult(cos, width);
	poly->yv[c] = y1 + mult(sin, width);   c++;
	poly->xv[c] = x2 + mult(cos, width);
	poly->yv[c] = y2 + mult(sin, width);   c++;
	cos = cosine((angle+2700) % 3600);
	sin = sine((angle+2700) % 3600);
	poly->xv[c] = x1 + mult(cos, width);
	poly->yv[c] = y1 + mult(sin, width);   c++;
	poly->xv[c] = x2 + mult(cos, width);
	poly->yv[c] = y2 + mult(sin, width);   c++;
	poly->count = c;
}

/*
 * routine to build a polygon "poly" that describes an arc.  The arc is "len"
 * long and "wid" wide and runs at "angle" tenths of a degree.  The two points
 * (x1,y1) and (x2,y2) are at the ends of this arc.  There is an extension of
 * "e1" on end 1 and "e2" on end 2 (a value typically ranging from 0 to half of
 * the width).
 */
void tech_makeendpointpoly(INTBIG len, INTBIG wid, INTBIG angle, INTBIG x1, INTBIG y1, INTBIG e1,
	INTBIG x2, INTBIG y2, INTBIG e2, POLYGON *poly)
{
	REGISTER INTBIG temp, xextra, yextra, xe1, ye1, xe2, ye2, w2, sa, ca;

	if (poly->limit < 4) (void)extendpolygon(poly, 4);
	poly->count = 4;
	w2 = wid / 2;

	/* somewhat simpler if rectangle is manhattan */
	if (angle == 900 || angle == 2700)
	{
		if (y1 > y2)
		{
			temp = y1;   y1 = y2;   y2 = temp;
			temp = e1;   e1 = e2;   e2 = temp;
		}
		poly->xv[0] = x1 - w2;   poly->yv[0] = y1 - e1;
		poly->xv[1] = x1 + w2;   poly->yv[1] = y1 - e1;
		poly->xv[2] = x2 + w2;   poly->yv[2] = y2 + e2;
		poly->xv[3] = x2 - w2;   poly->yv[3] = y2 + e2;
		return;
	}
	if (angle == 0 || angle == 1800)
	{
		if (x1 > x2)
		{
			temp = x1;   x1 = x2;   x2 = temp;
			temp = e1;   e1 = e2;   e2 = temp;
		}
		poly->xv[0] = x1 - e1;   poly->yv[0] = y1 - w2;
		poly->xv[1] = x1 - e1;   poly->yv[1] = y1 + w2;
		poly->xv[2] = x2 + e2;   poly->yv[2] = y2 + w2;
		poly->xv[3] = x2 + e2;   poly->yv[3] = y2 - w2;
		return;
	}

	/* nonmanhattan arcs cannot have zero length so re-compute it */
	if (len == 0) len = computedistance(x1,y1, x2,y2);
	if (len == 0)
	{
		sa = sine(angle);
		ca = cosine(angle);
		xe1 = x1 - mult(ca, e1);
		ye1 = y1 - mult(sa, e1);
		xe2 = x2 + mult(ca, e2);
		ye2 = y2 + mult(sa, e2);
		xextra = mult(ca, w2);
		yextra = mult(sa, w2);
	} else
	{
		/* work out all the math for nonmanhattan arcs */
		xe1 = x1 - muldiv(e1, (x2-x1), len);
		ye1 = y1 - muldiv(e1, (y2-y1), len);
		xe2 = x2 + muldiv(e2, (x2-x1), len);
		ye2 = y2 + muldiv(e2, (y2-y1), len);

		/* now compute the corners */
		xextra = muldiv(w2, (x2-x1), len);
		yextra = muldiv(w2, (y2-y1), len);
	}

	poly->xv[0] = yextra + xe1;   poly->yv[0] = ye1 - xextra;
	poly->xv[1] = xe1 - yextra;   poly->yv[1] = xextra + ye1;
	poly->xv[2] = xe2 - yextra;   poly->yv[2] = xextra + ye2;
	poly->xv[3] = yextra + xe2;   poly->yv[3] = ye2 - xextra;
}

/*
 * routine to reset the ISNEGATED bit on arc "ai" because it is in a
 * layout technology and shouldn't ought to be set
 */
void tech_resetnegated(ARCINST *ai)
{
	ttyputmsg(_("Warning: arc %s cannot be negated: state reset"), describearcinst(ai));
	ai->userbits &= ~ISNEGATED;
}

/*
 * routine to compute the number of displayable polygons on arcinst "ai"
 */
INTBIG tech_displayableavars(ARCINST *ai, WINDOWPART *win, POLYLOOP *pl)
{
	pl->numvar = ai->numvar;
	pl->firstvar = ai->firstvar;
	if ((us_useroptions&HIDETXTARC) != 0) return(0);
	return(tech_displayableanyvars(win, ai->parent, pl));
}

/*
 * routine to fill polygon "poly" with the next displayable variable
 * on arcinst "ai".  Returns the address of the variable just filled.
 */
VARIABLE *tech_filldisplayableavar(ARCINST *ai, POLYGON *poly, WINDOWPART *win,
	VARIABLE **varnoeval, POLYLOOP *pl)
{
	REGISTER NODEPROTO *cell;
	REGISTER VARIABLE *var;
	REGISTER BOOLEAN oldonobjectstate;

	/* track which object we are on */
	oldonobjectstate = db_onanobject;
	if (!db_onanobject)
	{
		db_onanobject = TRUE;
		db_onobjectaddr = (INTBIG)ai;
		db_onobjecttype = VARCINST;
	}
	db_lastonobjectaddr = (INTBIG)ai;
	db_lastonobjecttype = VARCINST;

	cell = ai->parent;
	pl->numvar = ai->numvar;
	pl->firstvar = ai->firstvar;
	if (cell->tech == NOTECHNOLOGY)
		cell->tech = whattech(cell);
	var = tech_filldisplayableanyvar(poly, ai->geom->lowx, ai->geom->highx,
		ai->geom->lowy, ai->geom->highy, (INTBIG)ai, VARCINST, cell->tech,
			win, cell, varnoeval, pl);
	db_onanobject = oldonobjectstate;
	return(var);
}

/******************** SUPPORT ********************/

/*
 * routine to compute the number of displayable polygons in the pair "tech_numvar/
 * tech_firstvar"
 */
INTBIG tech_displayableanyvars(WINDOWPART *win, NODEPROTO *cell, POLYLOOP *pl)
{
	REGISTER INTBIG i, total;
	REGISTER VARIABLE *var;

	pl->ndisplayindex = pl->ndisplaysubindex = total = 0;
	if (win == NOWINDOWPART) win = el_curwindowpart;
	for(i = 0; i < pl->numvar; i++)
	{
		var = &pl->firstvar[i];
		if ((var->type&VDISPLAY) == 0) continue;
		if (TDGETINTERIOR(var->textdescript) != 0 ||
			TDGETINHERIT(var->textdescript) != 0)
		{
			if (win != NOWINDOWPART && cell != win->curnodeproto) continue;
		}
		if ((var->type&VISARRAY) == 0) total++; else
			total += getlength(var);
	}
	return(total);
}

/*
 * routine to fill polygon "poly" with the next displayable variable
 * on nodeinst "ni".  Returns the address of the variable just filled.
 */
VARIABLE *tech_filldisplayableanyvar(POLYGON *poly, INTBIG lx, INTBIG hx,
	INTBIG ly, INTBIG hy, INTBIG addr, INTBIG type, TECHNOLOGY *tech,
		WINDOWPART *win, NODEPROTO *cell, VARIABLE **varnoeval, POLYLOOP *pl)
{
	REGISTER INTBIG len, language, whichindex;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	REGISTER CHAR *query;
	REGISTER VARIABLE *var, *retvar;
	REGISTER NODEINST *ni;
	REGISTER WINDOWPART *oldwin;

	/* get next displayable variable */
	if (varnoeval != 0)
		(*varnoeval) = NOVARIABLE;
	if (win == NOWINDOWPART) win = el_curwindowpart;
	oldwin = setvariablewindow(win);
	poly->count = 1;
	poly->xv[0] = (lx+hx)/2;
	poly->yv[0] = (ly+hy)/2;
	poly->style = CROSS;
	for(;;)
	{
		if (pl->ndisplayindex >= pl->numvar)
		{
			TDCLEAR(descript);
			poly->string = x_("ERROR");
			var = NOVARIABLE;
			break;
		}
		var = &pl->firstvar[pl->ndisplayindex];
		if (varnoeval != 0)
			(*varnoeval) = var;
		if ((var->type&VDISPLAY) != 0 &&
			((TDGETINTERIOR(var->textdescript) == 0 && TDGETINHERIT(var->textdescript) == 0) ||
				win == NOWINDOWPART ||
					cell == win->curnodeproto))
		{
			TDCOPY(descript, var->textdescript);
			language = var->type & (VCODE1|VCODE2);
			if (language != 0)
			{
				if ((var->type&VISARRAY) == 0) query = (CHAR *)var->addr; else
					query = ((CHAR **)var->addr)[0];
				retvar = doquerry(query, language, var->type & ~(VCODE1|VCODE2|VISARRAY|VLENGTH));
				if (retvar != NOVARIABLE)
				{
					TDCOPY(retvar->textdescript, descript);
					retvar->key = var->key;
					var = retvar;
				}
			}
			if ((var->type&VISARRAY) == 0)
			{
				pl->ndisplayindex++;
				pl->ndisplaysubindex = 0;
				if (TDGETPOS(descript) == VTPOSBOXED)
				{
					makerectpoly(lx, hx, ly, hy, poly);
					poly->style = CLOSED;
				}
				poly->string = describedisplayedvariable(var, -1, -1);
				break;
			}
			len = getlength(var);
			if (pl->ndisplaysubindex < len)
			{
				whichindex = pl->ndisplaysubindex++;
				poly->string = describedisplayedvariable(var, whichindex, -1);
				if (TDGETPOS(descript) == VTPOSBOXED)
				{
					makerectpoly(lx, hx, ly, hy, poly);
					poly->style = CLOSED;
				} else
				{
					if (type == VNODEINST)
					{
						/* multiline variables only work on nodes! */
						ni = (NODEINST *)addr;
						if (win == NOWINDOWPART) win = el_curwindowpart;
						getdisparrayvarlinepos((INTBIG)ni, VNODEINST, tech, win, var,
							whichindex, &poly->xv[0], &poly->yv[0], FALSE);
					}
				}
				break;
			}
		}
		pl->ndisplayindex++;
		pl->ndisplaysubindex = 0;
	}

	adjustdisoffset(addr, type, tech, poly, descript);
	poly->desc = &tech_vartxt;
	tech_vartxt.col = el_colcelltxt;
	poly->layer = -1;
	(void)setvariablewindow(oldwin);
	return(var);
}
