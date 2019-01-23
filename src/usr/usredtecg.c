/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usredtecg.c
 * User interface technology editor: conversion from technology to library
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
#include "tech.h"
#include "tecgen.h"
#include "tecart.h"
#include "usr.h"
#include "drc.h"
#include "usredtec.h"

SPECIALTEXTDESCR us_tecedmisctexttable[] =
{
	{NONODEINST, 0, 0, 6000, TECHLAMBDA},
	{NONODEINST, 0, 0,    0, TECHDESCRIPT},
	{NONODEINST, 0, 0, 0, 0},
};

SPECIALTEXTDESCR us_tecedlayertexttable[] =
{
	{NONODEINST, 0, 28000,  18000, LAYERSTYLE},
	{NONODEINST, 0, 28000,  12000, LAYERCIF},
	{NONODEINST, 0, 28000,   6000, LAYERDXF},
	{NONODEINST, 0, 28000,      0, LAYERGDS},
	{NONODEINST, 0, 28000,  -6000, LAYERFUNCTION},
	{NONODEINST, 0, 28000, -12000, LAYERLETTERS},
	{NONODEINST, 0, 28000, -18000, LAYERSPIRES},
	{NONODEINST, 0, 28000, -24000, LAYERSPICAP},
	{NONODEINST, 0, 28000, -30000, LAYERSPIECAP},
	{NONODEINST, 0, 28000, -36000, LAYER3DHEIGHT},
	{NONODEINST, 0, 28000, -42000, LAYER3DTHICK},
	{NONODEINST, 0, 28000, -48000, LAYERPRINTCOL},
	{NONODEINST, 0, 0, 0, 0},
};

SPECIALTEXTDESCR us_tecedarctexttable[] =
{
	{NONODEINST, 0, 0, 30000, ARCFUNCTION},
	{NONODEINST, 0, 0, 24000, ARCFIXANG},
	{NONODEINST, 0, 0, 18000, ARCWIPESPINS},
	{NONODEINST, 0, 0, 12000, ARCNOEXTEND},
	{NONODEINST, 0, 0,  6000, ARCINC},
	{NONODEINST, 0, 0, 0, 0},
};

SPECIALTEXTDESCR us_tecednodetexttable[] =
{
	{NONODEINST, 0, 0, 36000, NODEFUNCTION},
	{NONODEINST, 0, 0, 30000, NODESERPENTINE},
	{NONODEINST, 0, 0, 24000, NODESQUARE},
	{NONODEINST, 0, 0, 18000, NODEWIPES},
	{NONODEINST, 0, 0, 12000, NODELOCKABLE},
	{NONODEINST, 0, 0,  6000, NODEMULTICUT},
	{NONODEINST, 0, 0, 0, 0},
};

/* prototypes for local routines */
static NODEINST *us_tecedplacegeom(POLYGON*, NODEPROTO*);
static void      us_tecedsetlist(NODEINST*, POLYGON*, INTBIG, INTBIG, INTBIG, INTBIG);
static void      us_tecedcreatespecialtext(NODEPROTO *np, SPECIALTEXTDESCR *table);

/*
 * convert technology "tech" into a library and return that library.
 * Returns NOLIBRARY on error
 */
LIBRARY *us_tecedmakelibfromtech(TECHNOLOGY *tech)
{
	REGISTER CHAR *lay, *dxf, **sequence, **varnames, *fname, *gds;
	REGISTER INTBIG i, j, k, e, xs, ys, oldlam, *newmap, *mapptr, *minnodesize,
		tcon, func, nodexpos, bits, layertotal, arctotal, nodetotal, multicutsep,
		height3d, thick3d, lambda, min2x, min2y, nlx, nhx, nly, nhy, wid, xoff,
		*printcolors, *colors;
	INTBIG lx, hx, ly, hy, xpos[4], ypos[4], xsc[4], ysc[4], lxo, hxo, lyo, hyo,
		lxp, hxp, lyp, hyp, blx, bhx, bly, bhy;
	REGISTER BOOLEAN serp, square, wipes, lockable, first;
	float spires, spicap, spiecap;
	CHAR gdsbuf[50];
	REGISTER void *infstr;
	REGISTER NODEPROTO *np, **nplist, *pnp, **aplist;
	REGISTER VARIABLE *var, *var2, *var3, *var5, *var6, *var7, *var8,
		*var10, *var11, *varred, *vargreen, *varblue;
	REGISTER LIBRARY *lib;
	REGISTER NODEINST *ni, *oni, *nni;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp, *opp;
	REGISTER ARCPROTO *ap;
	REGISTER GRAPHICS *desc;
	static POLYGON *poly = NOPOLYGON;
	REGISTER DRCRULES *rules;
	REGISTER TECH_POLYGON *ll;
	REGISTER TECH_NODES *techn;
	REGISTER TECH_COLORMAP *colmap;
	NODEINST node;
	ARCINST arc;

	/* make sure network tool is on */
	if ((net_tool->toolstate&TOOLON) == 0)
	{
		ttyputerr(_("Network tool must be running...turning it on"));
		toolturnon(net_tool);
		ttyputerr(_("...now reissue the technology editing command"));
		return(NOLIBRARY);
	}

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	lib = newlibrary(tech->techname, tech->techname);
	if (lib == NOLIBRARY)
	{
		ttyputerr(_("Cannot create library %s"), tech->techname);
		return(NOLIBRARY);
	}
	ttyputmsg(_("Created library %s..."), tech->techname);

	/* create the information node */
	np = newnodeproto(x_("factors"), lib);
	if (np == NONODEPROTO) return(NOLIBRARY);
	np->userbits |= TECEDITCELL;

	/* modify this technology's lambda value to match the current one */
	oldlam = lib->lambda[tech->techindex];
	lambda = lib->lambda[art_tech->techindex];
	lib->lambda[tech->techindex] = lambda;

	/* create the miscellaneous info cell (called "factors") */
	us_tecedmakeinfo(np, oldlam, tech->techdescript);

	/* copy any miscellaneous variables and make a list of their names */
	j = 0;
	for(i=0; us_knownvars[i].varname != 0; i++)
	{
		us_knownvars[i].ival = 0;
		var = getval((INTBIG)tech, VTECHNOLOGY, -1, us_knownvars[i].varname);
		if (var == NOVARIABLE) continue;
		us_knownvars[i].ival = 1;
		j++;
		(void)setval((INTBIG)lib, VLIBRARY, us_knownvars[i].varname, var->addr, var->type);
	}
	if (j > 0)
	{
		varnames = (CHAR **)emalloc(j * (sizeof (CHAR *)), el_tempcluster);
		if (varnames == 0) return(NOLIBRARY);
		j = 0;
		for(i=0; us_knownvars[i].varname != 0; i++)
			if (us_knownvars[i].ival != 0) varnames[j++] = us_knownvars[i].varname;
		(void)setval((INTBIG)lib, VLIBRARY, x_("EDTEC_variable_list"), (INTBIG)varnames,
			VSTRING|VISARRAY|(j<<VLENGTHSH));
		efree((CHAR *)varnames);
	}

	/* create the layer node names */
	layertotal = tech->layercount;
	nplist = (NODEPROTO **)emalloc((layertotal * (sizeof (NODEPROTO *))), el_tempcluster);
	if (nplist == 0) return(NOLIBRARY);

	/* create the layer nodes */
	ttyputmsg(_("Creating the layers..."));
	var2 = getval((INTBIG)tech, VTECHNOLOGY, VSTRING|VISARRAY, x_("IO_cif_layer_names"));
	var3 = getval((INTBIG)tech, VTECHNOLOGY, -1, x_("IO_gds_layer_numbers"));
	var8 = getval((INTBIG)tech, VTECHNOLOGY, VSTRING|VISARRAY, x_("IO_dxf_layer_names"));
	var5 = getval((INTBIG)tech, VTECHNOLOGY, VFLOAT|VISARRAY, x_("SIM_spice_resistance"));
	var6 = getval((INTBIG)tech, VTECHNOLOGY, VFLOAT|VISARRAY, x_("SIM_spice_capacitance"));
	var7 = getval((INTBIG)tech, VTECHNOLOGY, VFLOAT|VISARRAY, x_("SIM_spice_edge_capacitance"));
	var10 = getval((INTBIG)tech, VTECHNOLOGY, VINTEGER|VISARRAY, x_("TECH_layer_3dheight"));
	var11 = getval((INTBIG)tech, VTECHNOLOGY, VINTEGER|VISARRAY, x_("TECH_layer_3dthickness"));
	printcolors = us_getprintcolors(tech);
	for(i=0; i<layertotal; i++)
	{
		desc = tech->layers[i];
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("layer-"));
		addstringtoinfstr(infstr, layername(tech, i));
		fname = returninfstr(infstr);

		/* make sure the layer doesn't exist */
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			if (namesame(fname, np->protoname) == 0)
		{
			ttyputerr(_("Warning: multiple layers named '%s'"), fname);
			break;
		}

		np = newnodeproto(fname, lib);
		if (np == NONODEPROTO) return(NOLIBRARY);
		np->userbits |= TECEDITCELL;
		nplist[i] = np;

		/* compute foreign file formats */
		if (var2 == NOVARIABLE) lay = x_(""); else
			lay = ((CHAR **)var2->addr)[i];
		gds = x_("");
		if (var3 != NOVARIABLE && (var3->type&VISARRAY) != 0)
		{
			if ((var3->type&VTYPE) == VSTRING) gds = ((CHAR **)var3->addr)[i]; else
			{
				j = ((INTBIG *)var3->addr)[i];
				if (j >= 0)
				{
					esnprintf(gdsbuf, 50, x_("%ld"), j);
					gds = gdsbuf;
				}
			}
		}
		if (var8 == NOVARIABLE) dxf = x_(""); else
			dxf = ((CHAR **)var8->addr)[i];

		/* compute the SPICE information */
		if (var5 == NOVARIABLE) spires = 0.0; else
			spires = ((float *)var5->addr)[i];
		if (var6 == NOVARIABLE) spicap = 0.0; else
			spicap = ((float *)var6->addr)[i];
		if (var7 == NOVARIABLE) spiecap = 0.0; else
			spiecap = ((float *)var7->addr)[i];

		/* compute the 3D information */
		if (var10 == NOVARIABLE) height3d = 0; else
			height3d = ((INTBIG *)var10->addr)[i];
		if (var11 == NOVARIABLE) thick3d = 0; else
			thick3d = ((INTBIG *)var11->addr)[i];

		/* get the print colors */
		if (printcolors == 0) colors = 0; else
			colors = &printcolors[i*5];

		/* build the layer cell */
		us_tecedmakelayer(np, desc->col, desc->raster,
			desc->colstyle&(NATURE|OUTLINEPAT), lay, layerfunction(tech, i),
				us_layerletters(tech, i), dxf, gds, spires, spicap, spiecap,
					height3d, thick3d, colors);
	}

	/* save the layer sequence */
	sequence = (CHAR **)emalloc(layertotal * (sizeof (CHAR *)), el_tempcluster);
	if (sequence == 0) return(NOLIBRARY);
	i = 0;
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		if (namesamen(np->protoname, x_("layer-"), 6) == 0)
			sequence[i++] = &np->protoname[6];
	(void)setval((INTBIG)lib, VLIBRARY, x_("EDTEC_layersequence"), (INTBIG)sequence,
		VSTRING|VISARRAY|(layertotal<<VLENGTHSH));
	efree((CHAR *)sequence);

	/* create the arc cells */
	ttyputmsg(_("Creating the arcs..."));
	arctotal = 0;
	for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
	{
		ap->temp1 = (INTBIG)NONODEPROTO;
		if ((ap->userbits&ANOTUSED) != 0) continue;
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("arc-"));
		addstringtoinfstr(infstr, ap->protoname);
		fname = returninfstr(infstr);

		/* make sure the arc doesn't exist */
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			if (namesame(fname, np->protoname) == 0)
		{
			ttyputerr(_("Warning: multiple arcs named '%s'"), fname);
			break;
		}

		np = newnodeproto(fname, lib);
		if (np == NONODEPROTO) return(NOLIBRARY);
		np->userbits |= TECEDITCELL;
		ap->temp1 = (INTBIG)np;
		var = getvalkey((INTBIG)ap, VARCPROTO, VINTEGER, us_arcstylekey);
		if (var != NOVARIABLE) bits = var->addr; else
			bits = ap->userbits;
		us_tecedmakearc(np, (ap->userbits&AFUNCTION)>>AFUNCTIONSH,
			bits&WANTFIXANG, ap->userbits&CANWIPE, bits&WANTNOEXTEND,
				(ap->userbits&AANGLEINC)>>AANGLEINCSH);

		/* now create the arc layers */
		ai = &arc;   initdummyarc(ai);
		ai->proto = ap;
		ai->width = defaultarcwidth(ap);
		wid = ai->width - arcwidthoffset(ai);
		ai->end[0].xpos = wid*2;
		ai->end[0].ypos = 0;
		ai->end[1].xpos = -wid*2;
		ai->end[1].ypos = 0;
		ai->length = computedistance(ai->end[0].xpos, ai->end[0].ypos, ai->end[1].xpos, ai->end[1].ypos);
		j = arcpolys(ai, NOWINDOWPART);
		xoff = wid*2 + wid/2 + arcwidthoffset(ai)/2;
		for(i=0; i<j; i++)
		{
			shapearcpoly(ai, i, poly);
			if (poly->layer < 0) continue;
			desc = tech->layers[poly->layer];
			if (desc->bits == LAYERN) continue;

			/* scale the arc geometry appropriately */
			for(k=0; k<poly->count; k++)
			{
				poly->xv[k] = muldiv(poly->xv[k] - xoff, lambda, oldlam) - 40000;
				poly->yv[k] = muldiv(poly->yv[k], lambda, oldlam) - 10000;
			}

			/* create the node to describe this layer */
			ni = us_tecedplacegeom(poly, np);
			if (ni == NONODEINST) continue;

			/* get graphics for this layer */
			us_teceditsetpatch(ni, desc);
			(void)setval((INTBIG)ni, VNODEINST, x_("EDTEC_layer"), (INTBIG)nplist[poly->layer], VNODEPROTO);
			(void)setvalkey((INTBIG)ni, VNODEINST, us_edtec_option_key, LAYERPATCH, VINTEGER);
		}
		i = muldiv(arcwidthoffset(ai) / 2, lambda, oldlam);
		wid = muldiv(wid, lambda, oldlam);
		ni = newnodeinst(art_boxprim, -40000 - wid*5 - i, -40000-i,
			-10000-wid/2, -10000+wid/2, 0, 0, np);
		if (ni == NONODEINST) return(NOLIBRARY);
		(void)setvalkey((INTBIG)ni, VNODEINST, art_colorkey, HIGHLIT, VINTEGER);
		(void)setval((INTBIG)ni, VNODEINST, x_("EDTEC_layer"), (INTBIG)NONODEPROTO, VNODEPROTO);
		(void)setvalkey((INTBIG)ni, VNODEINST, us_edtec_option_key, LAYERPATCH, VINTEGER);
		endobjectchange((INTBIG)ni, VNODEINST);
		arctotal++;

		/* compact it accordingly */
		us_tecedcompact(np);
	}

	/* save the arc sequence */
	sequence = (CHAR **)emalloc(arctotal * (sizeof (CHAR *)), el_tempcluster);
	if (sequence == 0) return(NOLIBRARY);
	i = 0;
	for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
	{
		if ((ap->userbits&ANOTUSED) != 0) continue;
		sequence[i++] = &((NODEPROTO *)ap->temp1)->protoname[4];
	}
	(void)setval((INTBIG)lib, VLIBRARY, x_("EDTEC_arcsequence"), (INTBIG)sequence,
		VSTRING|VISARRAY|(arctotal<<VLENGTHSH));
	efree((CHAR *)sequence);

	/* create the node cells */
	ttyputmsg(_("Creating the nodes..."));
	nodetotal = 0;
	for(pnp = tech->firstnodeproto; pnp != NONODEPROTO; pnp = pnp->nextnodeproto)
		if ((pnp->userbits&NNOTUSED) == 0) nodetotal++;
	minnodesize = (INTBIG *)emalloc(nodetotal*2*SIZEOFINTBIG, el_tempcluster);
	if (minnodesize == 0) return(NOLIBRARY);
	nodetotal = 0;
	for(pnp = tech->firstnodeproto; pnp != NONODEPROTO; pnp = pnp->nextnodeproto)
	{
		pnp->temp1 = 0;
		if ((pnp->userbits&NNOTUSED) != 0) continue;
		first = TRUE;

		/* create the node layers */
		oni = &node;   initdummynode(oni);
		oni->proto = pnp;
		xs = (pnp->highx - pnp->lowx) * 2;
		ys = (pnp->highy - pnp->lowy) * 2;
		if (xs < 5000) xs = 5000;
		if (ys < 5000) ys = 5000;
		nodexpos = -xs*2;
		xpos[0] = nodexpos - xs;
		xpos[1] = nodexpos + xs;
		xpos[2] = nodexpos - xs;
		xpos[3] = nodexpos + xs;
		ypos[0] = -10000 + ys;
		ypos[1] = -10000 + ys;
		ypos[2] = -10000 - ys;
		ypos[3] = -10000 - ys;
		nodesizeoffset(oni, &lxp, &lyp, &hxp, &hyp);
		xs = (pnp->highx - pnp->lowx) - lxp - hxp;
		ys = (pnp->highy - pnp->lowy) - lyp - hyp;
		xsc[0] = xs*1;   ysc[0] = ys*1;
		xsc[1] = xs*2;   ysc[1] = ys*1;
		xsc[2] = xs*1;   ysc[2] = ys*2;
		xsc[3] = xs*2;   ysc[3] = ys*2;

		/* for multicut contacts, make large size be just right for 2 cuts */
		techn = tech->nodeprotos[pnp->primindex-1];
		if (techn->special == MULTICUT)
		{
			min2x = (techn->f1*2 + techn->f3*2 + techn->f4) * oldlam / WHOLE;
			min2y = (techn->f2*2 + techn->f3*2 + techn->f4) * oldlam / WHOLE;
			xsc[1] = min2x;
			xsc[3] = min2x;
			ysc[2] = min2y;
			ysc[3] = min2y;
		}
		for(e=0; e<4; e++)
		{
			/* do not create node if main example had no polygons */
			if (e != 0 && first) continue;

			/* square nodes have only two examples */
			if ((pnp->userbits&NSQUARE) != 0 && (e == 1 || e == 2)) continue;
			oni->lowx  = xpos[e] - xsc[e]/2 - lxp;
			oni->lowy  = ypos[e] - ysc[e]/2 - lyp;
			oni->highx = xpos[e] + xsc[e]/2 + hxp;
			oni->highy = ypos[e] + ysc[e]/2 + hyp;

			/* place the layers */
			j = nodepolys(oni, 0, NOWINDOWPART);
			for(i=0; i<j; i++)
			{
				shapenodepoly(oni, i, poly);
				if (poly->layer < 0) continue;
				desc = tech->layers[poly->layer];
				if (desc->bits == LAYERN) continue;

				for(k=0; k<poly->count; k++)
				{
					poly->xv[k] = poly->xv[k] * lambda / oldlam;
					poly->yv[k] = poly->yv[k] * lambda / oldlam;
				}
				/* accumulate total size of main example */
				if (e == 0)
				{
					getbbox(poly, &blx, &bhx, &bly, &bhy);
					if (i == 0)
					{
						nlx = blx;   nhx = bhx;
						nly = bly;   nhy = bhy;
					} else
					{
						if (blx < nlx) nlx = blx;
						if (bhx > nhx) nhx = bhx;
						if (bly < nly) nly = bly;
						if (bhy > nhy) nhy = bhy;
					}
				}

				/* create the node cell on the first valid layer */
				if (first)
				{
					first = FALSE;
					infstr = initinfstr();
					addstringtoinfstr(infstr, x_("node-"));
					addstringtoinfstr(infstr, pnp->protoname);
					fname = returninfstr(infstr);

					/* make sure the node doesn't exist */
					for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
						if (namesame(fname, np->protoname) == 0)
					{
						ttyputerr(_("Warning: multiple nodes named '%s'"), fname);
						break;
					}

					np = newnodeproto(fname, lib);
					if (np == NONODEPROTO) return(NOLIBRARY);
					np->userbits |= TECEDITCELL;
					func = (pnp->userbits&NFUNCTION)>>NFUNCTIONSH;
					serp = FALSE;
					if ((func == NPTRANMOS || func == NPTRAPMOS || func == NPTRADMOS) &&
						(pnp->userbits&HOLDSTRACE) != 0)
							serp = TRUE;
					if ((pnp->userbits&NSQUARE) != 0) square = TRUE; else
						square = FALSE;
					if ((pnp->userbits&WIPEON1OR2) != 0) wipes = TRUE; else
						wipes = FALSE;
					if ((pnp->userbits&LOCKEDPRIM) != 0) lockable = TRUE; else
						lockable = FALSE;
					if (techn->special == MULTICUT) multicutsep = techn->f4; else
						multicutsep = 0;
					us_tecedmakenode(np, func, serp, square, wipes, lockable, multicutsep);
					pnp->temp1 = (INTBIG)np;
				}

				/* create the node to describe this layer */
				ni = us_tecedplacegeom(poly, np);
				if (ni == NONODEINST) return(NOLIBRARY);

				/* get graphics for this layer */
				us_teceditsetpatch(ni, desc);
				(void)setval((INTBIG)ni, VNODEINST, x_("EDTEC_layer"),
					(INTBIG)nplist[poly->layer], VNODEPROTO);
				(void)setvalkey((INTBIG)ni, VNODEINST, us_edtec_option_key, LAYERPATCH, VINTEGER);

				/* set minimum polygon factor on smallest example */
				if (e != 0) continue;
				if (i >= tech->nodeprotos[pnp->primindex-1]->layercount) continue;
				ll = tech->nodeprotos[pnp->primindex-1]->layerlist;
				if (ll == 0) continue;
				if (ll[i].representation != MINBOX) continue;
				var = setval((INTBIG)ni, VNODEINST, x_("EDTEC_minbox"), (INTBIG)x_("MIN"), VSTRING|VDISPLAY);
				if (var != NOVARIABLE)
					defaulttextsize(3, var->textdescript);
			}
			if (first) continue;

			/* create the highlight node */
			lx = (xpos[e]-xsc[e]/2) * lambda / oldlam;
			hx = (xpos[e]+xsc[e]/2) * lambda / oldlam;
			ly = (ypos[e]-ysc[e]/2) * lambda / oldlam;
			hy = (ypos[e]+ysc[e]/2) * lambda / oldlam;
			ni = newnodeinst(art_boxprim, lx, hx, ly, hy, 0, 0, np);
			if (ni == NONODEINST) return(NOLIBRARY);
			(void)setvalkey((INTBIG)ni, VNODEINST, art_colorkey, HIGHLIT, VINTEGER);
			(void)setval((INTBIG)ni, VNODEINST, x_("EDTEC_layer"), (INTBIG)NONODEPROTO, VNODEPROTO);
			(void)setvalkey((INTBIG)ni, VNODEINST, us_edtec_option_key, LAYERPATCH, VINTEGER);
			endobjectchange((INTBIG)ni, VNODEINST);

			/* create a grab node (only in main example) */
			if (e == 0)
			{
				var = getvalkey((INTBIG)pnp, VNODEPROTO, VINTEGER|VISARRAY, el_prototype_center_key);
				if (var != NOVARIABLE)
				{
					lx = hx = xpos[0] + ((INTBIG *)var->addr)[0];
					ly = hy = ypos[0] + ((INTBIG *)var->addr)[1];
					lx = lx * lambda / oldlam;
					hx = hx * lambda / oldlam;
					ly = ly * lambda / oldlam;
					hy = hy * lambda / oldlam;
					nodeprotosizeoffset(gen_cellcenterprim, &lxo, &lyo, &hxo, &hyo, np);
					ni = newnodeinst(gen_cellcenterprim, lx-lxo, hx+hxo, ly-lyo, hy+hyo, 0, 0, np);
					if (ni == NONODEINST) return(NOLIBRARY);
					endobjectchange((INTBIG)ni, VNODEINST);
				}
			}

			/* also draw ports */
			for(pp = pnp->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			{
				shapeportpoly(oni, pp, poly, FALSE);
				getbbox(poly, &lx, &hx, &ly, &hy);
				lx = lx * lambda / oldlam;
				hx = hx * lambda / oldlam;
				ly = ly * lambda / oldlam;
				hy = hy * lambda / oldlam;
				nodeprotosizeoffset(gen_portprim, &lxo, &lyo, &hxo, &hyo, np);
				ni = newnodeinst(gen_portprim, lx-lxo, hx+hxo, ly-lyo, hy+hyo, 0, 0, np);
				if (ni == NONODEINST) return(NOLIBRARY);
				pp->temp1 = (INTBIG)ni;
				(void)setvalkey((INTBIG)ni, VNODEINST, us_edtec_option_key, LAYERPATCH, VINTEGER);
				var = setval((INTBIG)ni, VNODEINST, x_("EDTEC_portname"), (INTBIG)pp->protoname,
					VSTRING|VDISPLAY);
				if (var != NOVARIABLE)
					defaulttextsize(3, var->textdescript);
				endobjectchange((INTBIG)ni, VNODEINST);

				/* on the first sample, also show angle and connection */
				if (e != 0) continue;
				if (((pp->userbits&PORTANGLE)>>PORTANGLESH) != 0 ||
					((pp->userbits&PORTARANGE)>>PORTARANGESH) != 180)
				{
					(void)setval((INTBIG)ni, VNODEINST, x_("EDTEC_portangle"),
						(pp->userbits&PORTANGLE)>>PORTANGLESH, VINTEGER);
					(void)setval((INTBIG)ni, VNODEINST, x_("EDTEC_portrange"),
						(pp->userbits&PORTARANGE)>>PORTARANGESH, VINTEGER);
				}

				/* add in the "local" port connections (from this tech) */
				for(tcon=i=0; pp->connects[i] != NOARCPROTO; i++)
					if (pp->connects[i]->tech == tech) tcon++;
				if (tcon != 0)
				{
					aplist = (NODEPROTO **)emalloc((tcon * (sizeof (NODEPROTO *))), el_tempcluster);
					if (aplist == 0) return(NOLIBRARY);
					for(j=i=0; pp->connects[i] != NOARCPROTO; i++)
					{
						if (pp->connects[i]->tech != tech) continue;
						aplist[j] = (NODEPROTO *)pp->connects[i]->temp1;
						if (aplist[j] != NONODEPROTO) j++;
					}
					(void)setval((INTBIG)ni, VNODEINST, x_("EDTEC_connects"),
						(INTBIG)aplist, VNODEPROTO|VISARRAY|(j<<VLENGTHSH));
					efree((CHAR *)aplist);
				}

				/* connect the connected ports */
				for(opp = pnp->firstportproto; opp != pp; opp = opp->nextportproto)
				{
					if (opp->network != pp->network) continue;
					nni = (NODEINST *)opp->temp1;
					if (nni == NONODEINST) continue;
					if (newarcinst(gen_universalarc, 0, 0, ni, ni->proto->firstportproto,
						(ni->highx+ni->lowx)/2, (ni->highy+ni->lowy)/2, nni,
							nni->proto->firstportproto, (nni->highx+nni->lowx)/2,
								(nni->highy+nni->lowy)/2, np) == NOARCINST) return(NOLIBRARY);
					break;
				}
			}
		}
		minnodesize[nodetotal*2] = (nhx - nlx) * WHOLE / lambda;
		minnodesize[nodetotal*2+1] = (nhy - nly) * WHOLE / lambda;
		nodetotal++;

		/* compact it accordingly */
		us_tecedcompact(np);
	}

	/* save the node sequence */
	sequence = (CHAR **)emalloc(nodetotal * (sizeof (CHAR *)), el_tempcluster);
	if (sequence == 0) return(NOLIBRARY);
	i = 0;
	for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		if (np->temp1 != 0)
			sequence[i++] = &((NODEPROTO *)np->temp1)->protoname[5];
	(void)setval((INTBIG)lib, VLIBRARY, x_("EDTEC_nodesequence"), (INTBIG)sequence,
		VSTRING|VISARRAY|(i<<VLENGTHSH));
	efree((CHAR *)sequence);

	/* create the color map information */
	ttyputmsg(_("Adding color map and design rules..."));
	var2 = getval((INTBIG)tech, VTECHNOLOGY, VCHAR|VISARRAY, x_("USER_color_map"));
	varred = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_red_key);
	vargreen = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_green_key);
	varblue = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_blue_key);
	if (varred != NOVARIABLE && vargreen != NOVARIABLE && varblue != NOVARIABLE &&
		var2 != NOVARIABLE)
	{
		newmap = emalloc((256*3*SIZEOFINTBIG), el_tempcluster);
		if (newmap == 0) return(NOLIBRARY);
		mapptr = newmap;
		colmap = (TECH_COLORMAP *)var2->addr;
		for(i=0; i<256; i++)
		{
			*mapptr++ = ((INTBIG *)varred->addr)[i];
			*mapptr++ = ((INTBIG *)vargreen->addr)[i];
			*mapptr++ = ((INTBIG *)varblue->addr)[i];
		}
		for(i=0; i<32; i++)
		{
			newmap[(i<<2)*3]   = colmap[i].red;
			newmap[(i<<2)*3+1] = colmap[i].green;
			newmap[(i<<2)*3+2] = colmap[i].blue;
		}
		(void)setval((INTBIG)lib, VLIBRARY, x_("EDTEC_colormap"), (INTBIG)newmap,
			VINTEGER|VISARRAY|((256*3)<<VLENGTHSH));
		efree((CHAR *)newmap);
	}

	/* create the design rule information */
	rules = dr_allocaterules(layertotal, nodetotal, tech->techname);
	if (rules == NODRCRULES) return(NOLIBRARY);
	for(i=0; i<layertotal; i++)
		(void)allocstring(&rules->layernames[i], layername(tech, i), el_tempcluster);
	i = 0;
	for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		if (np->temp1 != 0)
			(void)allocstring(&rules->nodenames[i++],  &((NODEPROTO *)np->temp1)->protoname[5],
				el_tempcluster);
	var = getvalkey((INTBIG)tech, VTECHNOLOGY, VFRACT|VISARRAY, dr_min_widthkey);
	if (var != NOVARIABLE)
		for(i=0; i<rules->numlayers; i++) rules->minwidth[i] = ((INTBIG *)var->addr)[i];
	var = getvalkey((INTBIG)tech, VTECHNOLOGY, VSTRING|VISARRAY, dr_min_width_rulekey);
	if (var != NOVARIABLE)
		for(i=0; i<rules->numlayers; i++)
			(void)reallocstring(&rules->minwidthR[i], ((CHAR **)var->addr)[i], el_tempcluster);
	var = getvalkey((INTBIG)tech, VTECHNOLOGY, VFRACT|VISARRAY, dr_connected_distanceskey);
	if (var != NOVARIABLE)
		for(i=0; i<rules->utsize; i++) rules->conlist[i] = ((INTBIG *)var->addr)[i];
	var = getvalkey((INTBIG)tech, VTECHNOLOGY, VSTRING|VISARRAY, dr_connected_distances_rulekey);
	if (var != NOVARIABLE)
		for(i=0; i<rules->utsize; i++)
			(void)reallocstring(&rules->conlistR[i], ((CHAR **)var->addr)[i], el_tempcluster);
	var = getvalkey((INTBIG)tech, VTECHNOLOGY, VFRACT|VISARRAY, dr_unconnected_distanceskey);
	if (var != NOVARIABLE)
		for(i=0; i<rules->utsize; i++) rules->unconlist[i] = ((INTBIG *)var->addr)[i];
	var = getvalkey((INTBIG)tech, VTECHNOLOGY, VSTRING|VISARRAY, dr_unconnected_distances_rulekey);
	if (var != NOVARIABLE)
		for(i=0; i<rules->utsize; i++)
			(void)reallocstring(&rules->unconlistR[i], ((CHAR **)var->addr)[i], el_tempcluster);
	var = getvalkey((INTBIG)tech, VTECHNOLOGY, VFRACT|VISARRAY, dr_connected_distancesWkey);
	if (var != NOVARIABLE)
		for(i=0; i<rules->utsize; i++) rules->conlistW[i] = ((INTBIG *)var->addr)[i];
	var = getvalkey((INTBIG)tech, VTECHNOLOGY, VSTRING|VISARRAY, dr_connected_distancesW_rulekey);
	if (var != NOVARIABLE)
		for(i=0; i<rules->utsize; i++)
			(void)reallocstring(&rules->conlistWR[i], ((CHAR **)var->addr)[i], el_tempcluster);
	var = getvalkey((INTBIG)tech, VTECHNOLOGY, VFRACT|VISARRAY, dr_unconnected_distancesWkey);
	if (var != NOVARIABLE)
		for(i=0; i<rules->utsize; i++) rules->unconlistW[i] = ((INTBIG *)var->addr)[i];
	var = getvalkey((INTBIG)tech, VTECHNOLOGY, VSTRING|VISARRAY, dr_unconnected_distancesW_rulekey);
	if (var != NOVARIABLE)
		for(i=0; i<rules->utsize; i++)
			(void)reallocstring(&rules->unconlistWR[i], ((CHAR **)var->addr)[i], el_tempcluster);
	var = getvalkey((INTBIG)tech, VTECHNOLOGY, VFRACT|VISARRAY, dr_connected_distancesMkey);
	if (var != NOVARIABLE)
		for(i=0; i<rules->utsize; i++) rules->conlistM[i] = ((INTBIG *)var->addr)[i];
	var = getvalkey((INTBIG)tech, VTECHNOLOGY, VSTRING|VISARRAY, dr_connected_distancesM_rulekey);
	if (var != NOVARIABLE)
		for(i=0; i<rules->utsize; i++)
			(void)reallocstring(&rules->conlistMR[i], ((CHAR **)var->addr)[i], el_tempcluster);
	var = getvalkey((INTBIG)tech, VTECHNOLOGY, VFRACT|VISARRAY, dr_unconnected_distancesMkey);
	if (var != NOVARIABLE)
		for(i=0; i<rules->utsize; i++) rules->unconlistM[i] = ((INTBIG *)var->addr)[i];
	var = getvalkey((INTBIG)tech, VTECHNOLOGY, VSTRING|VISARRAY, dr_unconnected_distancesM_rulekey);
	if (var != NOVARIABLE)
		for(i=0; i<rules->utsize; i++)
			(void)reallocstring(&rules->unconlistMR[i], ((CHAR **)var->addr)[i], el_tempcluster);
	var = getvalkey((INTBIG)tech, VTECHNOLOGY, VFRACT|VISARRAY, dr_edge_distanceskey);
	if (var != NOVARIABLE)
		for(i=0; i<rules->utsize; i++) rules->edgelist[i] = ((INTBIG *)var->addr)[i];
	var = getvalkey((INTBIG)tech, VTECHNOLOGY, VSTRING|VISARRAY, dr_edge_distances_rulekey);
	if (var != NOVARIABLE)
		for(i=0; i<rules->utsize; i++)
			(void)reallocstring(&rules->edgelistR[i], ((CHAR **)var->addr)[i], el_tempcluster);
	var = getvalkey((INTBIG)tech, VTECHNOLOGY, VFRACT, dr_wide_limitkey);
	if (var != NOVARIABLE) rules->widelimit = var->addr;
	var = getvalkey((INTBIG)tech, VTECHNOLOGY, VFRACT|VISARRAY, dr_min_node_sizekey);
	if (var != NOVARIABLE)
	{
		i = j = 0;
		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->temp1 != 0)
			{
				rules->minnodesize[i*2] = ((INTBIG *)var->addr)[j*2];
				rules->minnodesize[i*2+1] = ((INTBIG *)var->addr)[j*2+1];

				/* if rule is valid, make sure it is no larger than actual size */
				if (rules->minnodesize[i*2] > 0 && rules->minnodesize[i*2+1] > 0)
				{
					if (rules->minnodesize[i*2] > minnodesize[i*2])
						rules->minnodesize[i*2] = minnodesize[i*2];
					if (rules->minnodesize[i*2+1] > minnodesize[i*2+1])
						rules->minnodesize[i*2+1] = minnodesize[i*2+1];
				}
				i++;
			}
			j++;
		}
	}
	var = getvalkey((INTBIG)tech, VTECHNOLOGY, VSTRING|VISARRAY, dr_min_node_size_rulekey);
	if (var != NOVARIABLE)
	{
		i = j = 0;
		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->temp1 != 0)
			{
				reallocstring(&rules->minnodesizeR[i], ((CHAR **)var->addr)[j], el_tempcluster);
				i++;
			}
			j++;
		}
	}

	us_tecedloaddrcmessage(rules, lib);
	dr_freerules(rules);

	/* clean up */
	ttyputmsg(_("Done."));
	efree((CHAR *)nplist);
	efree((CHAR *)minnodesize);
	lib->lambda[tech->techindex] = oldlam;
	return(lib);
}

/*************************** CELL CREATION HELPERS ***************************/

/*
 * routine to build the appropriate descriptive information for the information
 * cell "np".
 */
void us_tecedmakeinfo(NODEPROTO *np, INTBIG lambda, CHAR *description)
{
	REGISTER INTBIG i;

	/* load up the structure with the current values */
	for(i=0; us_tecedmisctexttable[i].funct != 0; i++)
	{
		switch (us_tecedmisctexttable[i].funct)
		{
			case TECHLAMBDA:
				us_tecedmisctexttable[i].value = (void *)lambda;
				break;
			case TECHDESCRIPT:
				us_tecedmisctexttable[i].value = (void *)description;
				break;
		}
	}

	/* now create those text objects */
	us_tecedcreatespecialtext(np, us_tecedmisctexttable);
}

/*
 * routine to build the appropriate descriptive information for a layer into
 * cell "np".  The color is "colorindex"; the stipple array is in "stip"; the
 * layer style is in "style", the CIF layer is in "ciflayer"; the function is
 * in "functionindex"; the layer letters are in "layerletters"; the DXF layer name(s)
 * are "dxf"; the Calma GDS-II layer is in "gds"; the SPICE resistance is in "spires",
 * the SPICE capacitance is in "spicap", the SPICE edge capacitance is in "spiecap",
 * the 3D height is in "height3d", and the 3D thickness is in "thick3d".
 */
void us_tecedmakelayer(NODEPROTO *np, INTBIG colorindex, UINTSML stip[16], INTBIG style,
	CHAR *ciflayer, INTBIG functionindex, CHAR *layerletters, CHAR *dxf,
	CHAR *gds, float spires, float spicap, float spiecap,
	INTBIG height3d, INTBIG thick3d, INTBIG *printcolors)
{
	REGISTER NODEINST *nicolor, *ni, *laypatcontrol, *laycolor, *laystipple;
	REGISTER INTBIG i, x, y;
	REGISTER VARIABLE *var;
	CHAR *colorname, *colorsymbol, *newname;
	UINTBIG pattern[16];
	UINTSML spattern[16];
	REGISTER void *infstr;

	laycolor = laystipple = laypatcontrol = NONODEINST;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, us_edtec_option_key);
		if (var == NOVARIABLE) continue;
		switch (var->addr)
		{
			case LAYERCOLOR:     laycolor = ni;      break;
			case LAYERPATTERN:   laystipple = ni;    break;
			case LAYERPATCONT:   laypatcontrol = ni; break;
		}
	}

	/* create the color information if it is not there */
	if (laycolor == NONODEINST)
	{
		/* create the graphic color object */
		nicolor = newnodeinst(art_filledboxprim, -20000, -10000, 10000, 20000, 0, 0, np);
		if (nicolor == NONODEINST) return;
		(void)setvalkey((INTBIG)nicolor, VNODEINST, art_colorkey, colorindex, VINTEGER);
		(void)setval((INTBIG)np, VNODEPROTO, x_("EDTEC_colornode"), (INTBIG)nicolor, VNODEINST);
		if ((style&NATURE) == PATTERNED)
		{
			if ((style&OUTLINEPAT) == 0)
			{
				for(i=0; i<16; i++) pattern[i] = stip[i];
				(void)setvalkey((INTBIG)nicolor, VNODEINST, art_patternkey, (INTBIG)pattern,
					VINTEGER|VISARRAY|(16<<VLENGTHSH));
			} else
			{
				for(i=0; i<16; i++) spattern[i] = stip[i];
				(void)setvalkey((INTBIG)nicolor, VNODEINST, art_patternkey, (INTBIG)spattern,
					VSHORT|VISARRAY|(16<<VLENGTHSH));
			}
		}
		endobjectchange((INTBIG)nicolor, VNODEINST);

		/* create the text color object */
		ni = newnodeinst(gen_invispinprim, 20000, 20000, 24000, 24000, 0, 0, np);
		if (ni == NONODEINST) return;
		infstr = initinfstr();
		addstringtoinfstr(infstr, TECEDNODETEXTCOLOR);
		if (ecolorname(colorindex, &colorname, &colorsymbol)) colorname = x_("unknown");
		addstringtoinfstr(infstr, colorname);
		allocstring(&newname, returninfstr(infstr), el_tempcluster);
		var = setvalkey((INTBIG)ni, VNODEINST, art_messagekey, (INTBIG)newname,
			VSTRING|VDISPLAY);
		efree(newname);
		if (var != NOVARIABLE)
			defaulttextsize(2, var->textdescript);
		(void)setvalkey((INTBIG)ni, VNODEINST, us_edtec_option_key, LAYERCOLOR, VINTEGER);
		endobjectchange((INTBIG)ni, VNODEINST);
	}

	/* create the stipple pattern objects if none are there */
	if (laystipple == NONODEINST)
	{
		for(x=0; x<16; x++) for(y=0; y<16; y++)
		{
			if ((stip[y] & (1 << (15-x))) != 0)
			{
				ni = newnodeinst(art_filledboxprim, x*2000-40000, x*2000-38000,
					4000-y*2000, 6000-y*2000, 0, 0, np);
			} else
			{
				ni = newnodeinst(art_filledboxprim, x*2000-40000, x*2000-38000, 4000-y*2000,
					6000-y*2000, 0, 0, np);
				for(i=0; i<16; i++) spattern[i] = 0;
				(void)setvalkey((INTBIG)ni, VNODEINST, art_patternkey, (INTBIG)spattern,
					VSHORT|VISARRAY|(16<<VLENGTHSH));
			}
			if (ni == NONODEINST) return;
			(void)setvalkey((INTBIG)ni, VNODEINST, us_edtec_option_key, LAYERPATTERN, VINTEGER);
			endobjectchange((INTBIG)ni, VNODEINST);
		}
		ni = newnodeinst(gen_invispinprim, -24000, -24000, 7000, 7000, 0, 0, np);
		if (ni == NONODEINST) return;
		var = setvalkey((INTBIG)ni, VNODEINST, art_messagekey, (INTBIG)x_("Stipple Pattern"),
			VSTRING|VDISPLAY);
		if (var != NOVARIABLE)
			TDSETSIZE(var->textdescript, TXTSETQLAMBDA(2));
		endobjectchange((INTBIG)ni, VNODEINST);
	}

	/* create the patch control object */
	if (laypatcontrol == NONODEINST)
	{
		ni = newnodeinst(gen_invispinprim, 16000-40000, 16000-40000, 4000-16*2000, 4000-16*2000, 0, 0, np);
		if (ni == NONODEINST) return;
		infstr = initinfstr();
		addstringtoinfstr(infstr, _("Stipple Pattern Operations"));
		allocstring(&newname, returninfstr(infstr), el_tempcluster);
		var = setvalkey((INTBIG)ni, VNODEINST, art_messagekey, (INTBIG)newname, VSTRING|VDISPLAY);
		efree(newname);
		if (var != NOVARIABLE)
			defaulttextsize(2, var->textdescript);
		(void)setvalkey((INTBIG)ni, VNODEINST, us_edtec_option_key, LAYERPATCONT, VINTEGER);
		endobjectchange((INTBIG)ni, VNODEINST);
	}

	/* load up the structure with the current values */
	for(i=0; us_tecedlayertexttable[i].funct != 0; i++)
	{
		switch (us_tecedlayertexttable[i].funct)
		{
			case LAYERSTYLE:
				us_tecedlayertexttable[i].value = (void *)style;
				break;
			case LAYERCIF:
				us_tecedlayertexttable[i].value = (void *)ciflayer;
				break;
			case LAYERDXF:
				us_tecedlayertexttable[i].value = (void *)dxf;
				break;
			case LAYERGDS:
				us_tecedlayertexttable[i].value = (void *)gds;
				break;
			case LAYERFUNCTION:
				us_tecedlayertexttable[i].value = (void *)functionindex;
				break;
			case LAYERLETTERS:
				us_tecedlayertexttable[i].value = (void *)layerletters;
				break;
			case LAYERSPIRES:
				us_tecedlayertexttable[i].value = (void *)castint(spires);
				break;
			case LAYERSPICAP:
				us_tecedlayertexttable[i].value = (void *)castint(spicap);
				break;
			case LAYERSPIECAP:
				us_tecedlayertexttable[i].value = (void *)castint(spiecap);
				break;
			case LAYER3DHEIGHT:
				us_tecedlayertexttable[i].value = (void *)height3d;
				break;
			case LAYER3DTHICK:
				us_tecedlayertexttable[i].value = (void *)thick3d;
				break;
			case LAYERPRINTCOL:
				us_tecedlayertexttable[i].value = (void *)printcolors;
				break;
		}
	}

	/* now create those text objects */
	us_tecedcreatespecialtext(np, us_tecedlayertexttable);
}

/*
 * routine to build the appropriate descriptive information for an arc into
 * cell "np".  The function is in "func"; the arc is fixed-angle if "fixang"
 * is nonzero; the arc wipes pins if "wipes" is nonzero; and the arc does
 * not extend its ends if "noextend" is nonzero.  The angle increment is
 * in "anginc".
 */
void us_tecedmakearc(NODEPROTO *np, INTBIG func, INTBIG fixang, INTBIG wipes, INTBIG noextend,
	INTBIG anginc)
{
	REGISTER INTBIG i;

	/* load up the structure with the current values */
	for(i=0; us_tecedarctexttable[i].funct != 0; i++)
	{
		switch (us_tecedarctexttable[i].funct)
		{
			case ARCFUNCTION:
				us_tecedarctexttable[i].value = (void *)func;
				break;
			case ARCFIXANG:
				us_tecedarctexttable[i].value = (void *)fixang;
				break;
			case ARCWIPESPINS:
				us_tecedarctexttable[i].value = (void *)wipes;
				break;
			case ARCNOEXTEND:
				us_tecedarctexttable[i].value = (void *)noextend;
				break;
			case ARCINC:
				us_tecedarctexttable[i].value = (void *)anginc;
				break;
		}
	}

	/* now create those text objects */
	us_tecedcreatespecialtext(np, us_tecedarctexttable);
}

/*
 * routine to build the appropriate descriptive information for a node into
 * cell "np".  The function is in "func", the serpentine transistor factor
 * is in "serp", the node is square if "square" is true, the node
 * is invisible on 1 or 2 arcs if "wipes" is true, and the node is lockable
 * if "lockable" is true.
 */
void us_tecedmakenode(NODEPROTO *np, INTBIG func, BOOLEAN serp, BOOLEAN square, BOOLEAN wipes,
	BOOLEAN lockable, INTBIG multicutsep)
{
	REGISTER INTBIG i;

	/* load up the structure with the current values */
	for(i=0; us_tecednodetexttable[i].funct != 0; i++)
	{
		switch (us_tecednodetexttable[i].funct)
		{
			case NODEFUNCTION:
				us_tecednodetexttable[i].value = (void *)func;
				break;
			case NODESERPENTINE:
				us_tecednodetexttable[i].value = (void *)((INTBIG)serp);
				break;
			case NODESQUARE:
				us_tecednodetexttable[i].value = (void *)((INTBIG)square);
				break;
			case NODEWIPES:
				us_tecednodetexttable[i].value = (void *)((INTBIG)wipes);
				break;
			case NODELOCKABLE:
				us_tecednodetexttable[i].value = (void *)((INTBIG)lockable);
				break;
			case NODEMULTICUT:
				us_tecednodetexttable[i].value = (void *)multicutsep;
				break;
		}
	}

	/* now create those text objects */
	us_tecedcreatespecialtext(np, us_tecednodetexttable);
}

/*
 * routine to add the text corresponding to the layer function in "func"
 * to the current infinite string
 */
void us_tecedaddfunstring(void *infstr, INTBIG func)
{
	REGISTER INTBIG i;

	addstringtoinfstr(infstr, us_teclayer_functions[func&LFTYPE].name);
	for(i=0; us_teclayer_functions[i].name != 0; i++)
	{
		if (us_teclayer_functions[i].value <= LFTYPE) continue;
		if ((func&us_teclayer_functions[i].value) == 0) continue;
		func &= ~us_teclayer_functions[i].value;
		addtoinfstr(infstr, ',');
		addstringtoinfstr(infstr, us_teclayer_functions[i].name);
	}
}

/*
 * Routine to create special text geometry described by "table" in cell "np".
 */
void us_tecedcreatespecialtext(NODEPROTO *np, SPECIALTEXTDESCR *table)
{
	REGISTER NODEINST *ni;
	REGISTER INTBIG i, *pc;
	CHAR line[50], *allocatedname, *str;
	REGISTER VARIABLE *var;
	REGISTER void *infstr;

	us_tecedfindspecialtext(np, table);
	for(i=0; table[i].funct != 0; i++)
	{
		ni = table[i].ni;
		if (ni == NONODEINST)
		{
			ni = newnodeinst(gen_invispinprim, table[i].x, table[i].x,
				table[i].y, table[i].y, 0, 0, np);
			if (ni == NONODEINST) return;
			allocatedname = 0;
			switch (table[i].funct)
			{
				case TECHLAMBDA:
					(void)esnprintf(line, 50, x_("Lambda: %ld"), (INTBIG)table[i].value);
					str = line;
					break;
				case TECHDESCRIPT:
					infstr = initinfstr();
					formatinfstr(infstr, x_("Description: %s"), (CHAR *)table[i].value);
					allocstring(&allocatedname, returninfstr(infstr), el_tempcluster);
					str = allocatedname;
					break;
				case LAYERSTYLE:
					infstr = initinfstr();
					addstringtoinfstr(infstr, TECEDNODETEXTSTYLE);
					if (((INTBIG)table[i].value&NATURE) == SOLIDC) addstringtoinfstr(infstr, x_("solid")); else
					{
						if (((INTBIG)table[i].value&OUTLINEPAT) != 0)
							addstringtoinfstr(infstr, x_("patterned/outlined")); else
								addstringtoinfstr(infstr, x_("patterned"));
					}
					allocstring(&allocatedname, returninfstr(infstr), el_tempcluster);
					str = allocatedname;
					break;
				case LAYERCIF:
					infstr = initinfstr();
					formatinfstr(infstr, x_("%s%s"), TECEDNODETEXTCIF, (CHAR *)table[i].value);
					allocstring(&allocatedname, returninfstr(infstr), el_tempcluster);
					str = allocatedname;
					break;
				case LAYERDXF:
					infstr = initinfstr();
					formatinfstr(infstr, x_("%s%s"), TECEDNODETEXTDXF, (CHAR *)table[i].value);
					allocstring(&allocatedname, returninfstr(infstr), el_tempcluster);
					str = allocatedname;
					break;
				case LAYERGDS:
					infstr = initinfstr();
					formatinfstr(infstr, x_("%s%s"), TECEDNODETEXTGDS, (CHAR *)table[i].value);
					allocstring(&allocatedname, returninfstr(infstr), el_tempcluster);
					str = allocatedname;
					break;
				case LAYERFUNCTION:
					infstr = initinfstr();
					addstringtoinfstr(infstr, TECEDNODETEXTFUNCTION);
					us_tecedaddfunstring(infstr, (INTBIG)table[i].value);
					allocstring(&allocatedname, returninfstr(infstr), el_tempcluster);
					str = allocatedname;
					break;
				case LAYERLETTERS:
					infstr = initinfstr();
					formatinfstr(infstr, x_("%s%s"), TECEDNODETEXTLETTERS, (CHAR *)table[i].value);
					allocstring(&allocatedname, returninfstr(infstr), el_tempcluster);
					str = allocatedname;
					break;
				case LAYERSPIRES:
					infstr = initinfstr();
					formatinfstr(infstr, x_("%s%g"), TECEDNODETEXTSPICERES, castfloat((INTBIG)table[i].value));
					allocstring(&allocatedname, returninfstr(infstr), el_tempcluster);
					str = allocatedname;
					break;
				case LAYERSPICAP:
					infstr = initinfstr();
					formatinfstr(infstr, x_("%s%g"), TECEDNODETEXTSPICECAP, castfloat((INTBIG)table[i].value));
					allocstring(&allocatedname, returninfstr(infstr), el_tempcluster);
					str = allocatedname;
					break;
				case LAYERSPIECAP:
					infstr = initinfstr();
					formatinfstr(infstr, x_("%s%g"), TECEDNODETEXTSPICEECAP, castfloat((INTBIG)table[i].value));
					allocstring(&allocatedname, returninfstr(infstr), el_tempcluster);
					str = allocatedname;
					break;
				case LAYER3DHEIGHT:
					infstr = initinfstr();
					formatinfstr(infstr, x_("%s%ld"), TECEDNODETEXT3DHEIGHT, (INTBIG)table[i].value);
					allocstring(&allocatedname, returninfstr(infstr), el_tempcluster);
					str = allocatedname;
					break;
				case LAYER3DTHICK:
					infstr = initinfstr();
					formatinfstr(infstr, x_("%s%ld"), TECEDNODETEXT3DTHICK, (INTBIG)table[i].value);
					allocstring(&allocatedname, returninfstr(infstr), el_tempcluster);
					str = allocatedname;
					break;
				case LAYERPRINTCOL:
					infstr = initinfstr();
					pc = (INTBIG *)table[i].value;
					formatinfstr(infstr, x_("%s%ld,%ld,%ld, %ld,%s"), TECEDNODETEXTPRINTCOL,
						pc[0], pc[1], pc[2], pc[3], (pc[4]==0 ? x_("off") : x_("on")));
					allocstring(&allocatedname, returninfstr(infstr), el_tempcluster);
					str = allocatedname;
					break;
				case ARCFUNCTION:
					infstr = initinfstr();
					formatinfstr(infstr, x_("%s%s"), TECEDNODETEXTFUNCTION,
						us_tecarc_functions[(INTBIG)table[i].value].name);
					allocstring(&allocatedname, returninfstr(infstr), el_tempcluster);
					str = allocatedname;
					break;
				case ARCFIXANG:
					if ((INTBIG)table[i].value != 0) str = x_("Fixed-angle: Yes"); else
						str = x_("Fixed-angle: No");
					break;
				case ARCWIPESPINS:
					if ((INTBIG)table[i].value != 0) str = x_("Wipes pins: Yes"); else
						str = x_("Wipes pins: No");
					break;
				case ARCNOEXTEND:
					if ((INTBIG)table[i].value == 0) str = x_("Extend arcs: Yes"); else
						str = x_("Extend arcs: No");
					break;
				case ARCINC:
					(void)esnprintf(line, 50, x_("Angle increment: %ld"), (INTBIG)table[i].value);
					str = line;
					break;
				case NODEFUNCTION:
					infstr = initinfstr();
					formatinfstr(infstr, x_("%s%s"), TECEDNODETEXTFUNCTION,
						nodefunctionname((INTBIG)table[i].value, NONODEINST));
					allocstring(&allocatedname, returninfstr(infstr), el_tempcluster);
					str = allocatedname;
					break;
				case NODESERPENTINE:
					if (table[i].value) str = x_("Serpentine transistor: yes"); else
						str = x_("Serpentine transistor: no");
					break;
				case NODESQUARE:
					if (table[i].value) str = x_("Square node: yes"); else
						str = x_("Square node: no");
					break;
				case NODEWIPES:
					if (table[i].value) str = x_("Invisible with 1 or 2 arcs: yes"); else
						str = x_("Invisible with 1 or 2 arcs: no");
					break;
				case NODELOCKABLE:
					if (table[i].value) str = x_("Lockable: yes"); else
						str = x_("Lockable: no");
					break;
				case NODEMULTICUT:
					esnprintf(line, 100, _("Multicut separation: %s"), frtoa((INTBIG)table[i].value));
					str = line;
					break;
			}
			var = setvalkey((INTBIG)ni, VNODEINST, art_messagekey, (INTBIG)str, VSTRING|VDISPLAY);
			if (var != NOVARIABLE)
				defaulttextsize(2, var->textdescript);
			(void)setvalkey((INTBIG)ni, VNODEINST, us_edtec_option_key, table[i].funct, VINTEGER);
			endobjectchange((INTBIG)ni, VNODEINST);
			if (allocatedname != 0) efree(allocatedname);
		}
	}
}

/*
 * Routine to locate the nodes with the special node-cell text.  In cell "np", finds
 * the relevant text nodes in "table" and loads them into the structure.
 */
void us_tecedfindspecialtext(NODEPROTO *np, SPECIALTEXTDESCR *table)
{
	REGISTER NODEINST *ni;
	REGISTER INTBIG i;
	REGISTER VARIABLE *var;

	/* clear the node assignments */
	for(i=0; table[i].funct != 0; i++)
		table[i].ni = NONODEINST;

	/* determine the number of special texts here */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, us_edtec_option_key);
		if (var == NOVARIABLE) continue;
		for(i=0; table[i].funct != 0; i++)
		{
			if (var->addr != table[i].funct) continue;
			table[i].ni = ni;
			break;
		}
	}
}

NODEINST *us_tecedplacegeom(POLYGON *poly, NODEPROTO *np)
{
	INTBIG lx, hx, ly, hy, dummy;
	REGISTER NODEINST *nni;
	REGISTER VARIABLE *var;

	getbbox(poly, &lx, &hx, &ly, &hy);
	switch (poly->style)
	{
		case FILLED:
			if (!isbox(poly, &dummy, &dummy, &dummy, &dummy))
			{
				nni = newnodeinst(art_filledpolygonprim, lx, hx, ly, hy, 0, 0, np);
				if (nni == NONODEINST) return(NONODEINST);
				us_tecedsetlist(nni, poly, lx, hx, ly, hy);
				endobjectchange((INTBIG)nni, VNODEINST);
				return(nni);
			}
			/* FALLTHROUGH */ 
		case FILLEDRECT:
			nni = newnodeinst(art_filledboxprim, lx, hx, ly, hy, 0, 0, np);
			if (nni == NONODEINST) return(NONODEINST);
			endobjectchange((INTBIG)nni, VNODEINST);
			return(nni);
		case CLOSED:
			if (!isbox(poly, &dummy, &dummy, &dummy, &dummy))
			{
				nni = newnodeinst(art_closedpolygonprim, lx, hx, ly, hy, 0, 0, np);
				if (nni == NONODEINST) return(NONODEINST);
				us_tecedsetlist(nni, poly, lx, hx, ly, hy);
				endobjectchange((INTBIG)nni, VNODEINST);
				return(nni);
			}
			/* FALLTHROUGH */ 
		case CLOSEDRECT:
			nni = newnodeinst(art_boxprim, lx, hx, ly, hy, 0, 0, np);
			if (nni == NONODEINST) return(NONODEINST);
			endobjectchange((INTBIG)nni, VNODEINST);
			return(nni);
		case CROSSED:
			nni = newnodeinst(art_crossedboxprim, lx, hx, ly, hy, 0,0, np);
			if (nni == NONODEINST) return(NONODEINST);
			endobjectchange((INTBIG)nni, VNODEINST);
			return(nni);
		case OPENED:
			nni = newnodeinst(art_openedpolygonprim, lx, hx, ly, hy, 0, 0, np);
			if (nni == NONODEINST) return(NONODEINST);
			us_tecedsetlist(nni, poly, lx, hx, ly, hy);
			endobjectchange((INTBIG)nni, VNODEINST);
			return(nni);
		case OPENEDT1:
			nni = newnodeinst(art_openeddottedpolygonprim, lx, hx, ly, hy, 0, 0, np);
			if (nni == NONODEINST) return(NONODEINST);
			us_tecedsetlist(nni, poly, lx, hx, ly, hy);
			endobjectchange((INTBIG)nni, VNODEINST);
			return(nni);
		case OPENEDT2:
			nni = newnodeinst(art_openeddashedpolygonprim, lx, hx, ly, hy, 0, 0, np);
			if (nni == NONODEINST) return(NONODEINST);
			us_tecedsetlist(nni, poly, lx, hx, ly, hy);
			endobjectchange((INTBIG)nni, VNODEINST);
			return(nni);
		case OPENEDT3:
			nni = newnodeinst(art_openedthickerpolygonprim, lx, hx, ly, hy, 0, 0, np);
			if (nni == NONODEINST) return(NONODEINST);
			us_tecedsetlist(nni, poly, lx, hx, ly, hy);
			endobjectchange((INTBIG)nni, VNODEINST);
			return(nni);
		case CIRCLE:
			nni = newnodeinst(art_circleprim, lx, hx, ly, hy, 0, 0, np);
			if (nni == NONODEINST) return(NONODEINST);
			endobjectchange((INTBIG)nni, VNODEINST);
			return(nni);
		case THICKCIRCLE:
			nni = newnodeinst(art_thickcircleprim, lx, hx, ly, hy, 0, 0, np);
			if (nni == NONODEINST) return(NONODEINST);
			endobjectchange((INTBIG)nni, VNODEINST);
			return(nni);
		case DISC:
			nni = newnodeinst(art_filledcircleprim, lx, hx, ly, hy, 0, 0, np);
			if (nni == NONODEINST) return(NONODEINST);
			endobjectchange((INTBIG)nni, VNODEINST);
			return(nni);
		case CIRCLEARC:
			nni = newnodeinst(art_circleprim, lx, hx, ly, hy, 0, 0, np);
			if (nni == NONODEINST) return(NONODEINST);
			setarcdegrees(nni, 0.0, 45.0*EPI/180.0);
			endobjectchange((INTBIG)nni, VNODEINST);
			return(nni);
		case THICKCIRCLEARC:
			nni = newnodeinst(art_thickcircleprim, lx, hx, ly, hy, 0, 0, np);
			if (nni == NONODEINST) return(NONODEINST);
			setarcdegrees(nni, 0.0, 45.0*EPI/180.0);
			endobjectchange((INTBIG)nni, VNODEINST);
			return(nni);
		case TEXTCENT:
			nni = newnodeinst(gen_invispinprim, lx, hx, ly, hy, 0, 0, np);
			if (nni == NONODEINST) return(NONODEINST);
			var = setvalkey((INTBIG)nni, VNODEINST, art_messagekey,
				(INTBIG)poly->string, VSTRING|VDISPLAY);
			if (var != NOVARIABLE) TDSETPOS(var->textdescript, VTPOSCENT);
			endobjectchange((INTBIG)nni, VNODEINST);
			return(nni);
		case TEXTBOTLEFT:
			nni = newnodeinst(gen_invispinprim, lx, hx, ly, hy, 0, 0, np);
			if (nni == NONODEINST) return(NONODEINST);
			var = setvalkey((INTBIG)nni, VNODEINST, art_messagekey,
				(INTBIG)poly->string, VSTRING|VDISPLAY);
			if (var != NOVARIABLE) TDSETPOS(var->textdescript, VTPOSUPRIGHT);
			endobjectchange((INTBIG)nni, VNODEINST);
			return(nni);
		case TEXTBOTRIGHT:
			nni = newnodeinst(gen_invispinprim, lx, hx, ly, hy, 0, 0, np);
			if (nni == NONODEINST) return(NONODEINST);
			var = setvalkey((INTBIG)nni, VNODEINST, art_messagekey,
				(INTBIG)poly->string, VSTRING|VDISPLAY);
			if (var != NOVARIABLE) TDSETPOS(var->textdescript, VTPOSUPLEFT);
			endobjectchange((INTBIG)nni, VNODEINST);
			return(nni);
		case TEXTBOX:
			nni = newnodeinst(gen_invispinprim, lx, hx, ly, hy, 0, 0, np);
			if (nni == NONODEINST) return(NONODEINST);
			var = setvalkey((INTBIG)nni, VNODEINST, art_messagekey,
				(INTBIG)poly->string, VSTRING|VDISPLAY);
			if (var != NOVARIABLE) TDSETPOS(var->textdescript, VTPOSBOXED);
			endobjectchange((INTBIG)nni, VNODEINST);
			return(nni);
	}
	return(NONODEINST);
}

void us_tecedsetlist(NODEINST *ni, POLYGON *poly, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy)
{
	REGISTER INTBIG *list;
	REGISTER INTBIG i;

	list = emalloc((poly->count*2*SIZEOFINTBIG), el_tempcluster);
	if (list == 0) return;
	for(i=0; i<poly->count; i++)
	{
		list[i*2] = poly->xv[i] - (hx+lx)/2;
		list[i*2+1] = poly->yv[i] - (hy+ly)/2;
	}
	(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)list,
		VINTEGER|VISARRAY|((poly->count*2)<<VLENGTHSH));
	efree((CHAR *)list);
}
