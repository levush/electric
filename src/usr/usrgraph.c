/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrgraph.c
 * User interface tool: structure graphing module
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
#include "tecgen.h"
#include "tecart.h"

/****************************** LIBRARY GRAPHING *****************************/

#define NONODEDESCR ((NODEDESCR *)-1)

typedef struct Inodedescr
{
	INTBIG             x, y;
	INTBIG             yoff;
	NODEINST          *pin;
	struct Inodedescr *main;
} NODEDESCR;

static NODEPROTO *us_graphmainview(NODEPROTO *np);

void us_graphcells(NODEPROTO *top)
{
	REGISTER NODEPROTO *np, *sub, *graphnp, *truenp, *truesubnp;
	REGISTER NODEINST *ni, *nibot, *toppin;
	REGISTER ARCINST *ai;
	REGISTER LIBRARY *lib;
	REGISTER INTBIG more, maxdepth, color;
	REGISTER INTBIG *xval, *yoff, i, x, y, xe, ye, clock, maxwidth, xsc, lambda, xscale, yscale, yoffset;
	REGISTER PORTPROTO *pinpp;
	REGISTER VARIABLE *var;
	REGISTER NODEDESCR *nd, *ndsub;
	CHAR *newname;
	float spread;
	REGISTER void *infstr;

	pinpp = gen_invispinprim->firstportproto;

	/* create the graph cell */
	graphnp = newnodeproto(x_("CellStructure"), el_curlib);
	if (graphnp == NONODEPROTO) return;
	if (graphnp->prevversion != NONODEPROTO)
		ttyputverbose(M_("Creating new version of cell: CellStructure")); else
			ttyputverbose(M_("Creating cell: CellStructure"));

	/* clear flags on all of the cells */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = -1;

	/* find all top-level cells */
	if (top != NONODEPROTO) top->temp1 = 0; else
	{
		for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->firstinst == NONODEINST) np->temp1 = 0;
		}
	}

	/* now place all cells at their proper depth */
	maxdepth = 0;
	more = 1;
	while (more != 0)
	{
		more = 0;
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->temp1 == -1) continue;
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				sub = ni->proto;
				if (sub->primindex != 0) continue;

				/* ignore recursive references (showing icon in contents) */
				if (isiconof(sub, np)) continue;
				if (sub->temp1 <= np->temp1)
				{
					sub->temp1 = np->temp1 + 1;
					if (sub->temp1 > maxdepth) maxdepth = sub->temp1;
					more++;
				}
				truenp = contentsview(ni->proto);
				if (truenp == NONODEPROTO) continue;
				if (truenp->temp1 <= np->temp1)
				{
					truenp->temp1 = np->temp1 + 1;
					if (truenp->temp1 > maxdepth) maxdepth = truenp->temp1;
					more++;
				}
			}
		}

		/* add in any cells referenced from other libraries */
		if (more == 0 && top == NONODEPROTO)
		{
			for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				if (np->temp1 >= 0) continue;
				np->temp1 = 0;
				more++;
			}
		}
	}

	/* now assign X coordinates to each cell */
	maxdepth++;
	maxwidth = 0;
	xval = emalloc((SIZEOFINTBIG * maxdepth), el_tempcluster);
	if (xval == 0) return;
	yoff = emalloc((SIZEOFINTBIG * maxdepth), el_tempcluster);
	if (yoff == 0) return;
	for(i=0; i<maxdepth; i++) xval[i] = yoff[i] = 0;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		/* ignore icon cells from the graph (merge with contents) */
		if (np->temp1 == -1) continue;

		/* ignore associated cells for now */
		truenp = us_graphmainview(np);
		if (truenp != NONODEPROTO &&
			(np->firstnodeinst == NONODEINST || np->cellview == el_iconview ||
				np->cellview == el_skeletonview))
		{
			np->temp1 = -1;
			continue;
		}

		nd = (NODEDESCR *)emalloc(sizeof (NODEDESCR), us_tool->cluster);
		nd->pin = NONODEINST;
		nd->main = NONODEDESCR;

		nd->x = xval[np->temp1];
		xval[np->temp1] += estrlen(describenodeproto(np));
		if (xval[np->temp1] > maxwidth) maxwidth = xval[np->temp1];
		nd->y = np->temp1;
		nd->yoff = 0;
		np->temp1 = (INTBIG)nd;
	}

	/* now center each row */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (np->temp1 == -1) continue;
		nd = (NODEDESCR *)np->temp1;
		if (xval[nd->y] < maxwidth)
		{
			spread = (float)maxwidth / (float)xval[nd->y];
			nd->x = roundfloat(nd->x * spread);
		}
	}

	/* generate accurate X/Y coordinates */
	lambda = el_curlib->lambda[art_tech->techindex];
	xscale = lambda * 2 / 3;
	yscale = lambda * 20;
	yoffset = lambda/2;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (np->temp1 == -1) continue;
		nd = (NODEDESCR *)np->temp1;
		x = nd->x;   y = nd->y;
		x = x * xscale;
		y = -y * yscale + ((yoff[nd->y]++)%2) * yoffset;
		nd->x = x;   nd->y = y;
	}

	/* make unattached cells sit with their contents view */
	if (top == NONODEPROTO)
	{
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->temp1 != -1) continue;
			if (np->firstnodeinst != NONODEINST && np->cellview != el_iconview &&
					np->cellview != el_skeletonview) continue;
			truenp = us_graphmainview(np);
			if (truenp == NONODEPROTO) continue;
			if (truenp->temp1 == -1) continue;

			nd = (NODEDESCR *)truenp->temp1;
			ndsub = (NODEDESCR *)emalloc(sizeof (NODEDESCR), us_tool->cluster);
			ndsub->pin = NONODEINST;
			ndsub->main = nd;
			nd->yoff += yoffset*2;
			ndsub->x = nd->x;   ndsub->y = nd->y + nd->yoff;
			np->temp1 = (INTBIG)ndsub;
		}
	}

	/* write the header message */
	xsc = maxwidth * xscale / 2;
	ni = newnodeinst(gen_invispinprim, xsc, xsc, yscale, yscale, 0, 0, graphnp);
	if (ni == NONODEINST) return;
	endobjectchange((INTBIG)ni, VNODEINST);
	infstr = initinfstr();
	if (top != NONODEPROTO)
	{
		formatinfstr(infstr, _("Structure below cell %s"), describenodeproto(top));
	} else
	{
		formatinfstr(infstr, _("Structure of library %s"), el_curlib->libname);
	}
	allocstring(&newname, returninfstr(infstr), el_tempcluster);
	var = setvalkey((INTBIG)ni, VNODEINST, art_messagekey, (INTBIG)newname, VSTRING|VDISPLAY);
	efree(newname);
	if (var != NOVARIABLE)
		TDSETSIZE(var->textdescript, TXTSETQLAMBDA(24));

	/* place the components */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (np == graphnp) continue;
		if (np->temp1 == -1) continue;
		nd = (NODEDESCR *)np->temp1;

		x = nd->x;   y = nd->y;
		ni = newnodeinst(gen_invispinprim, x, x, y, y, 0, 0, graphnp);
		if (ni == NONODEINST) return;
		endobjectchange((INTBIG)ni, VNODEINST);
		nd->pin = ni;

		/* write the cell name in the node */
		var = setvalkey((INTBIG)ni, VNODEINST, art_messagekey, (INTBIG)describenodeproto(np),
			VSTRING|VDISPLAY);
		if (var != NOVARIABLE)
			TDSETSIZE(var->textdescript, TXTSETQLAMBDA(4));
	}

	/* attach related components with rigid arcs */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (np == graphnp) continue;
		if (np->temp1 == -1) continue;
		nd = (NODEDESCR *)np->temp1;
		if (nd->main == NONODEDESCR) continue;

		ai = newarcinst(art_solidarc, 0, FIXED, nd->pin, pinpp, nd->x, nd->y,
			nd->main->pin, pinpp, nd->main->x, nd->main->y, graphnp);
		if (ai == NOARCINST) return;
		endobjectchange((INTBIG)ai, VARCINST);

		/* set an invisible color on the arc */
		(void)setvalkey((INTBIG)ai, VARCINST, art_colorkey, 0, VINTEGER);
	}

	/* build wires between the hierarchical levels */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp2 = 0;
	clock = 0;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (np == graphnp) continue;

		/* always use the contents cell, not the icon */
		truenp = contentsview(np);
		if (truenp == NONODEPROTO) truenp = np;
		if (truenp->temp1 == -1) continue;

		nd = (NODEDESCR *)truenp->temp1;
		toppin = NONODEINST;
		clock++;
		for(ni = truenp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			sub = ni->proto;
			if (sub->primindex != 0) continue;

			/* ignore recursive references (showing icon in contents) */
			if (isiconof(sub, truenp)) continue;

			truesubnp = contentsview(sub);
			if (truesubnp == NONODEPROTO) truesubnp = sub;

			if (truesubnp->temp2 == clock) continue;
			truesubnp->temp2 = clock;

			/* draw a line from cell "truenp" to cell "truesubnp" */
			x = nd->x;
			y = nd->y;

			if (truesubnp->temp1 == -1) continue;
			ndsub = (NODEDESCR *)truesubnp->temp1;
			xe = ndsub->x;
			ye = ndsub->y;
			toppin = nd->pin;
			nibot = ndsub->pin;
			ai = newarcinst(art_solidarc, defaultarcwidth(art_solidarc), 0, toppin,
				pinpp, x, y, nibot, pinpp, xe, ye, graphnp);
			if (ai == NOARCINST) return;
			endobjectchange((INTBIG)ai, VARCINST);

			/* set an appropriate color on the arc (red for jumps of more than 1 level of depth) */
			color = BLUE;
			if (nd->y - ndsub->y > yscale+yoffset+yoffset) color = RED;
			(void)setvalkey((INTBIG)ai, VARCINST, art_colorkey, color, VINTEGER);
		}
	}

	/* free space */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (np->temp1 == -1) continue;
		efree((CHAR *)np->temp1);
	}
	efree((CHAR *)xval);
	efree((CHAR *)yoff);
}

/*
 * routine to find the main cell that "np" is associated with in the graph.  This code is
 * essentially the same as "contentscell()" except that any original type is allowed.
 * Returns NONODEPROTO if the cell is not associated.
 */
NODEPROTO *us_graphmainview(NODEPROTO *np)
{
	REGISTER NODEPROTO *rnp;

	/* primitives have no contents view */
	if (np == NONODEPROTO) return(NONODEPROTO);
	if (np->primindex != 0) return(NONODEPROTO);

	/* first check to see if there is a schematics link */
	FOR_CELLGROUP(rnp, np)
	{
		if (rnp->cellview == el_schematicview) return(rnp);
		if ((rnp->cellview->viewstate&MULTIPAGEVIEW) != 0) return(rnp);
	}

	/* now check to see if there is any layout link */
	FOR_CELLGROUP(rnp, np)
		if (rnp->cellview == el_layoutview) return(rnp);

	/* finally check to see if there is any "unknown" link */
	FOR_CELLGROUP(rnp, np)
		if (rnp->cellview == el_unknownview) return(rnp);

	/* no contents found */
	return(NONODEPROTO);
}

/****************************** COMMAND GRAPHING ******************************/

#define MAXILLUSTRATEDEPTH  100		/* maximum depth of command graph */
#define FORCEDDEPTH           7		/* required depth of command graph */
#define MAXILLUSTRATEWIDTH  200		/* maximum depth of command graph */
#define XCOMSCALE          1000		/* horizontal distance between words */
#define YCOMSCALE       (-10000)	/* vertical distance between words */
#define YCOMOFFSET         1000		/* vertical offset between words */

#define NOCOMILL ((COMILL *)-1)

typedef struct Icomill
{
	CHAR *name;
	INTBIG  x, y;
	INTBIG  realx;
	INTBIG  children;
	INTBIG  depth;
	struct Icomill *parent;
	struct Icomill *nextcomill;
	NODEINST *real;
} COMILL;

static COMILL *us_comilllist[MAXILLUSTRATEWIDTH];
static COMILL *us_allcomill;

static INTBIG us_maxillustratedepth, us_maxcommandentries;
static INTBIG us_illustrateXpos[MAXILLUSTRATEDEPTH];		/* current build-out X position */
static COMCOMP *us_illustrateparam[MAXILLUSTRATEDEPTH];	/* current parameter type */

/* prototypes for local routines */
static COMILL *us_illustratecommand(CHAR*, INTBIG, COMILL*);

void us_illustratecommandset(void)
{
	REGISTER INTBIG i;
	REGISTER INTBIG j;
	CHAR *newmessage[1];
	REGISTER NODEPROTO *graphnp;
	REGISTER NODEINST *ni;
	REGISTER COMILL *ci, *nextci;
	INTBIG xs, ys, xe, ye;
	REGISTER ARCINST *ai;
	REGISTER VARIABLE *var;

	/* create the graph cell */
	graphnp = newnodeproto(x_("CommandStructure"), el_curlib);
	if (graphnp == NONODEPROTO) return;
	if (graphnp->prevversion != NONODEPROTO)
		ttyputmsg(_("Creating new version of cell: CommandStructure")); else
			ttyputmsg(_("Creating cell: CommandStructure"));

	us_maxillustratedepth = 0;
	us_maxcommandentries = 0;
	for(i = 0; i < MAXILLUSTRATEDEPTH; i++) us_illustrateXpos[i] = 0;
	for(i = 0; i < MAXILLUSTRATEWIDTH; i++) us_comilllist[i] = NOCOMILL;
	us_allcomill = NOCOMILL;

	/* build the command graph */
	for(i = 0; us_lcommand[i].name != 0; i++)
	{
		for(j=0; j<us_lcommand[i].params; j++)
			us_illustrateparam[j] = us_lcommand[i].par[j];
		us_illustrateparam[us_lcommand[i].params] = NOCOMCOMP;
		us_comilllist[i] = us_illustratecommand(us_lcommand[i].name, 0, NOCOMILL);
	}

	ttyputmsg(_("%ld entries in command graph"), us_maxcommandentries);

	/* count the breadth information */
	j = 0;
	for(i = 0; i < us_maxillustratedepth; i++)
		if (us_illustrateXpos[i] > j) j = us_illustrateXpos[i];

	/* write the header message */
	ni = newnodeinst(gen_invispinprim, 0, j, -YCOMSCALE,
		gen_invispinprim->highx-gen_invispinprim->lowx-YCOMSCALE, 0, 0, graphnp);
	if (ni == NONODEINST) return;
	endobjectchange((INTBIG)ni, VNODEINST);
	var = setvalkey((INTBIG)ni, VNODEINST, art_messagekey, (INTBIG)_("Structure of commands"),
		VSTRING|VDISPLAY);
	if (var != NOVARIABLE)
		TDSETSIZE(var->textdescript, TXTSETPOINTS(20));

	/* make the name spacing uniform */
	for(ci = us_allcomill; ci != NOCOMILL; ci = ci->nextcomill)
		ci->realx = ci->children = 0;
	for(i = us_maxillustratedepth-1; i > 0; i--)
	{
		for(ci = us_allcomill; ci != NOCOMILL; ci = ci->nextcomill)
		{
			if (ci->depth != i) continue;
			ci->parent->realx += ci->x;
			ci->parent->children++;
		}
		for(ci = us_allcomill; ci != NOCOMILL; ci = ci->nextcomill)
			if (ci->depth == i-1 && ci->children != 0)
				ci->x = ci->realx / ci->children;
	}

	/* now place the names */
	for(ci = us_allcomill; ci != NOCOMILL; ci = ci->nextcomill)
	{
		if (ci->name == 0) continue;
		ni = newnodeinst(gen_invispinprim, ci->x,
			ci->x+gen_invispinprim->highx-gen_invispinprim->lowx, ci->y,
				ci->y+gen_invispinprim->highy-gen_invispinprim->lowy, 0, 0, graphnp);
		if (ni == NONODEINST) return;
		endobjectchange((INTBIG)ni, VNODEINST);
		ci->real = ni;

		/* set the node name, color, font */
		var = setvalkey((INTBIG)ni, VNODEINST, art_messagekey, (INTBIG)ci->name, VSTRING|VDISPLAY);
		if (var != NOVARIABLE)
			defaulttextsize(2, var->textdescript);
		(void)setvalkey((INTBIG)ni, VNODEINST, art_colorkey, RED, VINTEGER);
	}

	/* connect the names with arcs */
	for(ci = us_allcomill; ci != NOCOMILL; ci = ci->nextcomill)
	{
		if (ci->name == 0) continue;
		if (ci->parent == NOCOMILL) continue;
		portposition(ci->real, ci->real->proto->firstportproto, &xs, &ys);
		portposition(ci->parent->real, ci->parent->real->proto->firstportproto, &xe, &ye);
		ai = newarcinst(gen_universalarc, defaultarcwidth(gen_universalarc), 0, ci->real,
			ci->real->proto->firstportproto, xs, ys, ci->parent->real,
				ci->parent->real->proto->firstportproto, xe, ye, graphnp);
		if (ai == NOARCINST) break;
		endobjectchange((INTBIG)ai, VARCINST);
	}

	/* delete it all */
	for(ci = us_allcomill; ci != NOCOMILL; ci = nextci)
	{
		nextci = ci->nextcomill;
		efree((CHAR *)ci);
	}

	/* have the cell displayed on the screen */
	newmessage[0] = x_("CommandStructure");
	us_editcell(1, newmessage);
}

COMILL *us_illustratecommand(CHAR *name, INTBIG depth, COMILL *thisci)
{
	static COMCOMP us_recursioncomcomp = {
		NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS, 0, x_(""), x_("")};
	REGISTER COMCOMP *cc;
	REGISTER COMILL *ci, *subci;
	REGISTER INTBIG i, j, k;
	REGISTER CHAR *arg;

	/* put this name at the appropriate depth */
	if (depth >= MAXILLUSTRATEDEPTH) return(NOCOMILL);
	if (depth > us_maxillustratedepth) us_maxillustratedepth = depth;

	ci = (COMILL *)emalloc(sizeof (COMILL), el_tempcluster);
	if (ci == 0) return(NOCOMILL);
	ci->name = name;
	ci->depth = depth;
	ci->parent = thisci;
	ci->nextcomill = us_allcomill;
	us_allcomill = ci;
	ci->x = us_illustrateXpos[depth];
	ci->y = ci->depth * YCOMSCALE + ((us_illustrateXpos[depth]/XCOMSCALE)%5-2) * YCOMOFFSET;
	us_illustrateXpos[depth] += XCOMSCALE;
	if (name != 0) us_maxcommandentries++;

	/* if there is nothing below this command, return now */
	if (name == 0 || us_illustrateparam[depth] == NOCOMCOMP)
	{
		/* force extension of the tree to a specified depth */
		if (depth < FORCEDDEPTH)
			(void)us_illustratecommand((CHAR *)0, depth+1, ci);
		return(ci);
	}

	cc = us_illustrateparam[depth];
	if (cc->ifmatch == NOKEYWORD)
	{
		if (cc->toplist == topoffile) arg = x_("FILE"); else
		if (cc->toplist == topoflibfile) arg = x_("LIBRARY"); else
		if (cc->toplist == topoftechs) arg = x_("TECH"); else
		if (cc->toplist == topoflibs) arg = x_("LIB"); else
		if (cc->toplist == topoftools) arg = x_("TOOL"); else
		if (cc->toplist == topofviews) arg = x_("VIEW"); else
		if (cc->toplist == topofnets) arg = x_("NET"); else
		if (cc->toplist == topofarcs) arg = x_("ARC"); else
		if (cc->toplist == topofcells) arg = x_("CELL"); else
		if (cc->toplist == us_topofcommands) arg = x_("COM"); else
		if (cc->toplist == us_topofmacros) arg = x_("MACRO"); else
		if (cc->toplist == us_topofpopupmenu) arg = x_("POPUP"); else
		if (cc->toplist == us_topofports) arg = x_("PORT"); else
		if (cc->toplist == us_topofcports) arg = x_("CELLPORT"); else
		if (cc->toplist == us_topofexpports) arg = x_("EXPORT"); else
		if (cc->toplist == us_topofwindows) arg = x_("WINDOWPART"); else
		if (cc->toplist == us_topoflayers) arg = x_("LAYER"); else
		if (cc->toplist == us_topofhighlight) arg = x_("HIGH"); else
		if (cc->toplist == us_topofarcnodes) arg = x_("ARC/NODE"); else
		if (cc->toplist == us_topofnodes) arg = x_("NODE"); else
		if (cc->toplist == us_topofcells) arg = x_("CELL"); else
		if (cc->toplist == us_topofprims) arg = x_("PRIM"); else
		if (cc->toplist == us_topofconstraints) arg = x_("CONSTR"); else
		if (cc->toplist == us_topofmbuttons) arg = x_("BUTTON"); else
		if (cc->toplist == us_topofedteclay) arg = x_("LAYER"); else
		if (cc->toplist == us_topofedtecarc) arg = x_("ARC"); else
		if (cc->toplist == us_topofedtecnode) arg = x_("NODE"); else
		if (cc->toplist == us_topofallthings) arg = x_("ANY"); else
		if (cc->toplist == us_topofvars) arg = x_("VAR"); else
		if (cc == &us_recursioncomcomp) arg = x_("***"); else arg = x_("ARG");
		subci = us_illustratecommand(arg, depth+1, ci);
		if (subci == NOCOMILL) return(NOCOMILL);
		return(ci);
	}

	for(i=0; cc->ifmatch[i].name != 0; i++)
	{
		/* spread open the list and insert these options */
		k = cc->ifmatch[i].params;
		for(j = MAXILLUSTRATEDEPTH-k-2; j >= depth; j--)
			us_illustrateparam[j+k+1] = us_illustrateparam[j+1];
		for(j = 0; j < k; j++)
		{
			us_illustrateparam[depth+j+1] = cc->ifmatch[i].par[j];
			if (us_illustrateparam[depth+j+1] != us_illustrateparam[depth]) continue;
			us_illustrateparam[depth+j+1] = &us_recursioncomcomp;
		}

		subci = us_illustratecommand(cc->ifmatch[i].name, depth+1, ci);
		if (subci == NOCOMILL) return(NOCOMILL);

		/* remove the inserted options */
		for(j=depth+1; j<MAXILLUSTRATEDEPTH-k; j++)
			us_illustrateparam[j] = us_illustrateparam[j+k];
	}
	return(ci);
}

/****************************** PULLDOWN MENU DUMPING ******************************/

/* prototypes for local routines */
static void us_dumppulldownmenu(FILE *io, POPUPMENU *pm, CHAR *name, CHAR *prefix);

/*
 * Routine to dump the pulldown menus to an indented text file.
 */
void us_dumppulldownmenus(void)
{
	FILE *io;
	CHAR *truename;
	REGISTER INTBIG i;
	REGISTER POPUPMENU *pm;

	io = xcreate(x_("pulldowns.txt"), el_filetypetext, M_("Menu dump file"), &truename);
	if (io == NULL) return;

	xprintf(io, M_("Pulldown menus in Electric as of %s\n"),
		timetostring(getcurrenttime()));
	for(i=0; i<us_pulldownmenucount; i++)
	{
		pm = us_pulldowns[i];
		xprintf(io, x_("\n"));
		us_dumppulldownmenu(io, pm, pm->header, x_(""));
	}

	xclose(io);
	ttyputmsg(M_("Pulldown menus dumped to %s"), truename);
}

void us_dumppulldownmenu(FILE *io, POPUPMENU *pm, CHAR *name, CHAR *prefix)
{
	REGISTER POPUPMENUITEM *mi;
	REGISTER USERCOM *uc;
	REGISTER INTBIG i, j, k;
	CHAR comname[300], subprefix[50];

	for(k=j=0; name[k] != 0; k++)
		if (name[k] != '&') comname[j++] = name[k];
	comname[j] = 0;

	xprintf(io, x_("%s%s:\n"), prefix, comname);
	for(i=0; i<pm->total; i++)
	{
		mi = &pm->list[i];
		uc = mi->response;
		if (uc->active < 0)
		{
			xprintf(io, x_("%s    ----------\n"), prefix);
			continue;
		}

		for(k=j=0; mi->attribute[k] != 0; k++)
		{
			if (mi->attribute[k] == '&') continue;
			if (mi->attribute[k] == '/') break;
			comname[j++] = mi->attribute[k];
		}
		comname[j] = 0;

		if (uc->menu != NOPOPUPMENU)
		{
			estrcpy(subprefix, prefix);
			estrcat(subprefix, x_("    "));
			us_dumppulldownmenu(io, uc->menu, comname, subprefix);
			continue;
		}
		xprintf(io, x_("%s    %s\n"), prefix, comname);
	}
}
