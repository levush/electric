/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: simmaxwell.c
 * Generator for MAXWELL simulator
 *
 * Copyright (c) 2003 Static Free Software.
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
#if SIMTOOL

#include "global.h"
#include "efunction.h"
#include "sim.h"
#include "usr.h"

#define NOMAXNET ((MAXNET *)-1)

typedef struct Imaxnet
{
	INTBIG globalnetnumber;
	INTBIG *layerlist;
	INTBIG  layercount;
	INTBIG  layertotal;
	struct Imaxnet *nextmaxnet;
} MAXNET;

static MAXNET   *sim_maxwell_firstmaxnet;
static MAXNET   *sim_maxwell_maxnetfree = NOMAXNET;
static VARIABLE *sim_maxwell_varred;
static VARIABLE *sim_maxwell_vargreen;
static VARIABLE *sim_maxwell_varblue;
static INTBIG    sim_maxwell_boxnumber;
static INTBIG    sim_maxwell_netnumber;

/* prototypes for local routines */
static void    sim_writemaxcell(NODEPROTO *np, FILE *io, XARRAY trans);
static void    sim_maxwellwritepoly(POLYGON *poly, FILE *io, INTBIG globalnet);
static MAXNET *sim_maxwell_allocmaxnet(void);
static void    sim_maxwell_freemaxnet(MAXNET *mn);

void sim_freemaxwellmemory(void)
{
	REGISTER MAXNET *mn;

	while (sim_maxwell_maxnetfree != NOMAXNET)
	{
		mn = sim_maxwell_maxnetfree;
		sim_maxwell_maxnetfree = mn->nextmaxnet;
		if (mn->layertotal > 0) efree((CHAR *)mn->layerlist);
		efree((CHAR *)mn);
	}
}

/*
 * routine to write a Maxwell file from the cell "np"
 */
void sim_writemaxwell(NODEPROTO *np)
{
	CHAR *truename, name[400];
	REGISTER NETWORK *net;
	REGISTER INTBIG i;
	REGISTER MAXNET *mn, *nextmn;
	FILE *io;

	/* make sure network tool is on */
	if ((net_tool->toolstate&TOOLON) == 0)
	{
		ttyputerr(_("Network tool must be running...turning it on"));
		toolturnon(net_tool);
		ttyputerr(_("...now reissue the simulation command"));
		return;
	}

	/* first write the "max" file */
	(void)estrcpy(name, np->protoname);
	(void)estrcat(name, x_(".mac"));
	io = xcreate(name, sim_filetypemaxwell, _("Maxwell File"), &truename);
	if (io == NULL)
	{
		if (truename != 0) ttyputerr(_("Cannot write %s"), truename);
		return;
	}
	xprintf(io, x_("# Maxwell for Cell %s from Library %s\n"), describenodeproto(np),
		np->lib->libname);
	if ((us_useroptions&NODATEORVERSION) == 0)
	{
		if (np->version) xprintf(io, x_("# CELL VERSION %ld\n"), np->version);
		if (np->creationdate)
			xprintf(io, x_("# CELL CREATED ON %s\n"), timetostring((time_t)np->creationdate));
		if (np->revisiondate)
			xprintf(io, x_("# LAST REVISED ON %s\n"), timetostring((time_t)np->revisiondate));
		xprintf(io, x_("# Maxwell netlist written by Electric Design System; Version %s\n"), el_version);
		xprintf(io, x_("# WRITTEN ON %s\n"), timetostring(getcurrenttime()));
	} else
	{
		xprintf(io, x_("# Maxwell netlist written by Electric Design System\n"));
	}
	xprintf(io, x_("\n"));

	/* get colormap for emitting display information */
	sim_maxwell_varred = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_red_key);
	sim_maxwell_vargreen = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_green_key);
	sim_maxwell_varblue = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_blue_key);

	/* recursively write the maxwell information */
	sim_maxwell_boxnumber = 1;
	sim_maxwell_netnumber = 0;
	for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		net->temp1 = sim_maxwell_netnumber++;
	sim_maxwell_firstmaxnet = NOMAXNET;
	sim_writemaxcell(np, io, el_matid);

	/* write the unite statements */
	for(mn = sim_maxwell_firstmaxnet; mn != NOMAXNET; mn = nextmn)
	{
		nextmn = mn->nextmaxnet;
		if (mn->layercount > 1)
		{
			xprintf(io, x_("Unite {"));
			for(i=0; i<mn->layercount; i++)
			{
				if (i != 0) xprintf(io, x_(" "));
				xprintf(io, x_("\"Box-%ld\""), mn->layerlist[i]);
			}
			xprintf(io, x_("}\n"));
		}
		sim_maxwell_freemaxnet(mn);
	}

	/* clean up */
	xclose(io);
	ttyputmsg(_("%s written"), truename);
}

/*
 * recursively called routine to print the Maxwell description of cell "np".
 */
void sim_writemaxcell(NODEPROTO *np, FILE *io, XARRAY trans)
{
	REGISTER INTBIG i, tot;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER NETWORK *net;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	XARRAY transr, transt, temptrans, subrot;
	static POLYGON *poly = NOPOLYGON;

	/* stop if requested */
	if (el_pleasestop != 0)
	{
		(void)stopping(STOPREASONDECK);
		return;
	}

	(void)needstaticpolygon(&poly, 4, sim_tool->cluster);
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		makerot(ni, transr);
		transmult(transr, trans, temptrans);
		if (ni->proto->primindex == 0)
		{
			/* recurse */
			maketrans(ni, transt);
			transmult(transt, temptrans, subrot);

			/* propagate networks to the subcell */
			for(net = ni->proto->firstnetwork; net != NONETWORK; net = net->nextnetwork)
				net->temp1 = -1;
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				pi->proto->network->temp1 = pi->conarcinst->network->temp1;
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
				pe->proto->network->temp1 = pe->exportproto->network->temp1;
			for(net = ni->proto->firstnetwork; net != NONETWORK; net = net->nextnetwork)
				if (net->temp1 == -1) net->temp1 = sim_maxwell_netnumber++;
			sim_writemaxcell(ni->proto, io, subrot);
		} else
		{
			tot = nodeEpolys(ni, 0, NOWINDOWPART);
			for(i=0; i<tot; i++)
			{
				shapeEnodepoly(ni, i, poly);
				if (poly->portproto == NOPORTPROTO) continue;
				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
					if (pi->proto == poly->portproto) break;
				if (pi == NOPORTARCINST) continue;
				xformpoly(poly, temptrans);
				sim_maxwellwritepoly(poly, io, pi->conarcinst->network->temp1);
			}
		}
	}
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		tot = arcpolys(ai, NOWINDOWPART);
		for(i=0; i<tot; i++)
		{
			shapearcpoly(ai, i, poly);
			xformpoly(poly, trans);
			sim_maxwellwritepoly(poly, io, ai->network->temp1);
		}
	}
}

void sim_maxwellwritepoly(POLYGON *poly, FILE *io, INTBIG globalnet)
{
	INTBIG lx, hx, ly, hy;
	REGISTER INTBIG r, g, b, i, *newlist, newtotal, fun;
	float x, y, dx, dy;
	REGISTER MAXNET *mn;
	REGISTER CHAR *layname;
	CHAR laynamebot[200], laynamehei[200], thisname[100];

	if (poly->tech != el_curtech) return;
	fun = layerfunction(poly->tech, poly->layer);
	if ((fun&LFPSEUDO) != 0) return;
	if (!isbox(poly, &lx, &hx, &ly, &hy)) return;
	if (sim_maxwell_varred != NOVARIABLE && sim_maxwell_vargreen != NOVARIABLE &&
		sim_maxwell_varblue != NOVARIABLE)
	{
		/* emit the color information */
		r = ((INTBIG *)sim_maxwell_varred->addr)[poly->desc->col];
		g = ((INTBIG *)sim_maxwell_vargreen->addr)[poly->desc->col];
		b = ((INTBIG *)sim_maxwell_varblue->addr)[poly->desc->col];
		xprintf(io, x_("NewObjColor %ld %ld %ld\n"), r, g, b);
	}

	/* emit the box */
	x = scaletodispunit(lx, DISPUNITMIC);
	y = scaletodispunit(ly, DISPUNITMIC);
	dx = scaletodispunit(hx-lx, DISPUNITMIC);
	dy = scaletodispunit(hy-ly, DISPUNITMIC);
	layname = layername(poly->tech, poly->layer);
	esnprintf(laynamebot, 200, x_("%s-Bot"), layname);
	esnprintf(laynamehei, 200, x_("%s-Hei"), layname);

	/* find this layer and add maxwell box */
	for(mn = sim_maxwell_firstmaxnet; mn != NOMAXNET; mn = mn->nextmaxnet)
		if (mn->globalnetnumber == globalnet) break;
	if (mn == NOMAXNET)
	{
		mn = sim_maxwell_allocmaxnet();
		if (mn == NOMAXNET) return;
		mn->nextmaxnet = sim_maxwell_firstmaxnet;
		sim_maxwell_firstmaxnet = mn;
		mn->globalnetnumber = globalnet;
	}

	/* now add this box to the maxnet */
	if (mn->layercount >= mn->layertotal)
	{
		newtotal = mn->layertotal * 2;
		if (mn->layercount >= newtotal) newtotal = mn->layercount + 20;
		newlist = (INTBIG *)emalloc(newtotal * SIZEOFINTBIG, sim_tool->cluster);
		if (newlist == 0) return;
		for(i=0; i<mn->layercount; i++)
			newlist[i] = mn->layerlist[i];
		if (mn->layertotal > 0) efree((CHAR *)mn->layerlist);
		mn->layerlist = newlist;
		mn->layertotal = newtotal;
	}
	mn->layerlist[mn->layercount++] = sim_maxwell_boxnumber;

	esnprintf(thisname, 100, x_("Box-%ld"), sim_maxwell_boxnumber);
	sim_maxwell_boxnumber++;

	xprintf(io, x_("Box pos3 %g %g %s   %g %g %s \"%s\"\n"), x, y, laynamebot,
		dx, dy, laynamehei, thisname);
}

MAXNET *sim_maxwell_allocmaxnet(void)
{
	REGISTER MAXNET *mn;

	if (sim_maxwell_maxnetfree != NOMAXNET)
	{
		mn = sim_maxwell_maxnetfree;
		sim_maxwell_maxnetfree = mn->nextmaxnet;
	} else
	{
		mn = (MAXNET *)emalloc(sizeof (MAXNET), sim_tool->cluster);
		if (mn == 0) return(NOMAXNET);
		mn->layertotal = 0;
	}
	mn->layercount = 0;
	return(mn);
}

void sim_maxwell_freemaxnet(MAXNET *mn)
{
	mn->nextmaxnet = sim_maxwell_maxnetfree;
	sim_maxwell_maxnetfree = mn;
}

#endif	/* SIMTOOL - at top */
