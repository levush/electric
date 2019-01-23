/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iolefo.c
 * Input/output tool: LEF output
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

/*
 * Note that this reader was built by examining LEF files and reverse-engineering them.
 * It does not claim to be compliant with the LEF specification, but it also does not
 * claim to define a new specification.  It is merely incomplete.
 */

#include "config.h"
#include "global.h"
#include "eio.h"
#include "usr.h"
#include "efunction.h"

/* prototypes for local routines */
static void io_lefoutcell(FILE *out, NODEPROTO *cell, BOOLEAN top, XARRAY trans);
static CHAR *io_lefoutlayername(TECHNOLOGY *tech, INTBIG layer);
static void io_lefoutspread(FILE *out, NODEPROTO *cell, NETWORK *net, NODEINST *ignore);
static void io_lefwritepoly(FILE *out, POLYGON *poly, XARRAY trans, TECHNOLOGY *tech);

static CHAR io_lefoutcurlayer[200];

BOOLEAN io_writeleflibrary(LIBRARY *lib)
{
	CHAR file[100], *truename;
	REGISTER CHAR *name;
	REGISTER INTBIG i, tot;
	REGISTER float xs, ys;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *rni, *ni;
	REGISTER ARCINST *ai;
	XARRAY trans, xform, temp;
	REGISTER PORTPROTO *pp, *rpp;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, io_tool->cluster);

	/* create the proper disk file for the LEF */
	np = lib->curnodeproto;
	if (np == NONODEPROTO)
	{
		ttyputerr(_("Must be editing a cell to generate LEF output"));
		return(TRUE);
	}
	(void)estrcpy(file, np->protoname);
	(void)estrcat(file, x_(".lef"));
	name = truepath(file);
	io_fileout = xcreate(name, io_filetypelef, _("LEF File"), &truename);
	if (io_fileout == NULL)
	{
		if (truename != 0) ttyputerr(_("Cannot write %s"), truename);
		return(TRUE);
	}

	/* write header information */
	if ((us_useroptions&NODATEORVERSION) == 0)
	{
		xprintf(io_fileout, _("# Electric VLSI Design System, version %s\n"), el_version);
		xprintf(io_fileout, x_("# %s\n\n"), timetostring(getcurrenttime()));
	} else
	{
		xprintf(io_fileout, _("# Electric VLSI Design System\n"));
	}
	us_emitcopyright(io_fileout, x_("# "), x_(""));
	xprintf(io_fileout, x_("NAMESCASESENSITIVE ON ;\n"));
	xprintf(io_fileout, x_("UNITS\n"));
	xprintf(io_fileout, x_("  DATABASE MICRONS 1 ;\n"));
	xprintf(io_fileout, x_("END UNITS\n\n"));

	/* write layer information */
	for(i=0; i<8; i++)
	{
		xprintf(io_fileout, x_("LAYER METAL%ld\n"), i+1);
		xprintf(io_fileout, x_("  TYPE ROUTING ;\n"));
		xprintf(io_fileout, x_("END METAL%ld\n\n"), i+1);
	}
	xprintf(io_fileout, x_("LAYER CONT\n"));
	xprintf(io_fileout, x_("  TYPE CUT ;\n"));
	xprintf(io_fileout, x_("END CONT\n\n"));
	for(i=0; i<3; i++)
	{
		xprintf(io_fileout, x_("LAYER VIA%ld%ld\n"), i+1, i+2);
		xprintf(io_fileout, x_("  TYPE CUT ;\n"));
		xprintf(io_fileout, x_("END VIA%ld%l\n\n"), i+1, i+2);
	}
	for(i=0; i<3; i++)
	{
		xprintf(io_fileout, x_("LAYER POLY%ld\n"), i+1);
		xprintf(io_fileout, x_("  TYPE MASTERSLICE ;\n"));
		xprintf(io_fileout, x_("END POLY%l\n\n"), i+1);
	}
	xprintf(io_fileout, x_("LAYER PDIFF\n"));
	xprintf(io_fileout, x_("  TYPE MASTERSLICE ;\n"));
	xprintf(io_fileout, x_("END PDIFF\n\n"));
	xprintf(io_fileout, x_("LAYER NDIFF\n"));
	xprintf(io_fileout, x_("  TYPE MASTERSLICE ;\n"));
	xprintf(io_fileout, x_("END NDIFF\n\n"));

	/* write main cell header */
	xprintf(io_fileout, x_("MACRO %s\n"), np->protoname);
	xprintf(io_fileout, x_("  FOREIGN %s ;\n"), np->protoname);
	xs = scaletodispunit(np->highx-np->lowx, DISPUNITMIC);
	ys = scaletodispunit(np->highy-np->lowy, DISPUNITMIC);
	xprintf(io_fileout, x_("  SIZE %g BY %g ;\n"), xs, ys);
	xprintf(io_fileout, x_("  SITE %s ;\n"), np->protoname);

	/* write all of the metal geometry and ports */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst) ni->temp1 = 0;
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst) ai->temp1 = 0;
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		if (pp != np->firstportproto) xprintf(io_fileout, x_("\n"));
		xprintf(io_fileout, x_("  PIN %s\n"), pp->protoname);

		transid(trans);
		rpp = pp->subportproto;
		rni = pp->subnodeinst;
		makerot(rni, xform);   transmult(xform, trans, temp);  transcpy(temp, trans);
		while (rni->proto->primindex == 0)
		{
			rni = rpp->subnodeinst;
			rpp = rpp->subportproto;
			maketrans(rni, xform);  transmult(xform, trans, temp);
			makerot(rni, xform);    transmult(xform, temp, trans);
		}
		xprintf(io_fileout, x_("    PORT\n"));
		io_lefoutcurlayer[0] = 0;
		tot = nodeEpolys(rni, 0, NOWINDOWPART);
		for(i=0; i<tot; i++)
		{
			shapeEnodepoly(rni, i, poly);
			if (poly->portproto != rpp) continue;
			io_lefwritepoly(io_fileout, poly, trans, rni->proto->tech);
		}
		io_lefoutspread(io_fileout, np, pp->network, ni);
		xprintf(io_fileout, x_("    END\n"));
		if ((pp->userbits&STATEBITS) == PWRPORT)
			xprintf(io_fileout, x_("    USE POWER ;\n"));
		if ((pp->userbits&STATEBITS) == GNDPORT)
			xprintf(io_fileout, x_("    USE GROUND ;\n"));
		xprintf(io_fileout, x_("  END %s\n"), pp->protoname);
	}

	/* write the obstructions (all of the metal) */
	xprintf(io_fileout, x_("\n  OBS\n"));
	io_lefoutcurlayer[0] = 0;
	io_lefoutcell(io_fileout, np, TRUE, el_matid);
	xprintf(io_fileout, x_("  END\n\n"));

	/* clean up */
	xprintf(io_fileout, x_("END %s\n\n"), np->protoname);
	xprintf(io_fileout, x_("END LIBRARY\n"));
	xclose(io_fileout);

	/* tell the user that the file is written */
	ttyputmsg(_("%s written"), truename);
	return(FALSE);
}

/*
 * Routine to write all geometry in cell "cell" that is on network "net"
 * to file "out".  Does not write node "ignore".
 */
void io_lefoutspread(FILE *out, NODEPROTO *cell, NETWORK *net, NODEINST *ignore)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER PORTARCINST *pi;
	REGISTER INTBIG i, tot, fun;
	XARRAY trans;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, io_tool->cluster);
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex == 0) continue;
		if (ni == ignore) continue;
		fun = nodefunction(ni);
		if (fun != NPPIN && fun != NPCONTACT && fun != NPNODE && fun != NPCONNECT)
			continue;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			if (pi->conarcinst->network != net) break;
		if (pi != NOPORTARCINST) continue;

		/* write all layers on this node */
		ni->temp1 = 1;
		makerot(ni, trans);
		tot = nodepolys(ni, 0, NOWINDOWPART);
		for(i=0; i<tot; i++)
		{
			shapenodepoly(ni, i, poly);
			io_lefwritepoly(out, poly, trans, ni->proto->tech);
		}
	}

	/* write metal layers for all arcs */
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->network != net) continue;
		ai->temp1 = 1;
		tot = arcpolys(ai, NOWINDOWPART);
		for(i=0; i<tot; i++)
		{
			shapearcpoly(ai, i, poly);
			io_lefwritepoly(out, poly, trans, ai->proto->tech);
		}
	}
}

/*
 * Routine to write all geometry in cell "cell" to file "out".  The
 * hierarchy is flattened.  The current cell is transformed by "trans".
 */
void io_lefoutcell(FILE *out, NODEPROTO *cell, BOOLEAN top, XARRAY trans)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER INTBIG i, tot;
	XARRAY localtran, subrot, localrot, thisrot;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, io_tool->cluster);
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (top && ni->temp1 != 0) continue;
		makerot(ni, localrot);   transmult(localrot, trans, thisrot);
		if (ni->proto->primindex == 0)
		{
			/* subcell: recurse */
			maketrans(ni, localtran);
			transmult(localtran, thisrot, subrot);
			io_lefoutcell(out, ni->proto, FALSE, subrot);
		} else
		{
			/* primitive: find the metal layers */
			tot = nodepolys(ni, 0, NOWINDOWPART);
			for(i=0; i<tot; i++)
			{
				shapenodepoly(ni, i, poly);
				io_lefwritepoly(out, poly, trans, ni->proto->tech);
			}
		}
	}

	/* write metal layers for all arcs */
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (top && ai->temp1 != 0) continue;
		tot = arcpolys(ai, NOWINDOWPART);
		for(i=0; i<tot; i++)
		{
			shapearcpoly(ai, i, poly);
			io_lefwritepoly(out, poly, trans, ai->proto->tech);
		}
	}
}

/*
 * Routine to write polygon "poly" from technology "tech", transformed by "trans",
 * to "out".
 */
void io_lefwritepoly(FILE *out, POLYGON *poly, XARRAY trans, TECHNOLOGY *tech)
{
	INTBIG lx, hx, ly, hy;
	REGISTER float flx, fhx, fly, fhy;
	REGISTER CHAR *layername;

	layername = io_lefoutlayername(tech, poly->layer);
	if (*layername == 0) return;
	xformpoly(poly, trans);
	if (!isbox(poly, &lx, &hx, &ly, &hy)) return;
	flx = scaletodispunit(lx, DISPUNITMIC);
	fhx = scaletodispunit(hx, DISPUNITMIC);
	fly = scaletodispunit(ly, DISPUNITMIC);
	fhy = scaletodispunit(hy, DISPUNITMIC);
	if (estrcmp(layername, io_lefoutcurlayer) != 0)
	{
		xprintf(out, x_("    LAYER %s ;\n"), layername);
		estrcpy(io_lefoutcurlayer, layername);
	}
	xprintf(out, x_("    RECT %g %g %g %g ;\n"), flx, fly, fhx, fhy);
}

CHAR *io_lefoutlayername(TECHNOLOGY *tech, INTBIG layer)
{
	REGISTER INTBIG fun;

	fun = layerfunction(tech, layer);
	switch (fun&LFTYPE)
	{
		case LFMETAL1:    return(x_("METAL1"));
		case LFMETAL2:    return(x_("METAL2"));
		case LFMETAL3:    return(x_("METAL3"));
		case LFMETAL4:    return(x_("METAL4"));
		case LFMETAL5:    return(x_("METAL5"));
		case LFMETAL6:    return(x_("METAL6"));
		case LFMETAL7:    return(x_("METAL7"));
		case LFMETAL8:    return(x_("METAL8"));
		case LFMETAL9:    return(x_("METAL9"));
		case LFMETAL10:   return(x_("METAL10"));
		case LFMETAL11:   return(x_("METAL11"));
		case LFMETAL12:   return(x_("METAL12"));
		case LFCONTACT1:  return(x_("CONT"));
		case LFCONTACT2:  return(x_("VIA12"));
		case LFCONTACT3:  return(x_("VIA23"));
		case LFCONTACT4:  return(x_("VIA34"));
		case LFCONTACT5:  return(x_("VIA45"));
		case LFCONTACT6:  return(x_("VIA56"));
		case LFCONTACT7:  return(x_("VIA67"));
		case LFCONTACT8:  return(x_("VIA78"));
		case LFCONTACT9:  return(x_("VIA89"));
		case LFCONTACT10: return(x_("VIA9"));
		case LFCONTACT11: return(x_("VIA10"));
		case LFCONTACT12: return(x_("VIA11"));
		case LFPOLY1:     return(x_("POLY1"));
		case LFPOLY2:     return(x_("POLY2"));
		case LFPOLY3:     return(x_("POLY3"));
		case LFDIFF:
			if ((fun&LFPTYPE) != 0) return(x_("PDIFF"));
			if ((fun&LFNTYPE) != 0) return(x_("NDIFF"));
			return(x_("DIFF"));
	}
	return(x_(""));
}
