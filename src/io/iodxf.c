/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iodxf.c
 * Input/output tool: DXF input
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
#include "global.h"
#include "egraphics.h"
#include "efunction.h"
#include "database.h"
#include "tecgen.h"
#include "tecart.h"
#include "eio.h"
#include "usr.h"
#include "edialogs.h"
#include <math.h>

/* #define PRESERVEDXF     1 */		/* uncomment to save original DXF with nodes */

#define MAXCHARSPERLINE	256		/* maximum line length in DXF file */

typedef struct Idxflayer
{
	CHAR		*layerName;
	INTBIG		layerColor;
	float		layerRed, layerGreen, layerBlue;
	struct Idxflayer    *next;
} DXFLAYER;

typedef struct Iforwardref
{
	CHAR                *refname;
	NODEPROTO           *parent;
	INTBIG               x, y;
	INTBIG               rot;
	INTBIG               xrep, yrep;
	INTBIG               xspa, yspa;
	float                xsca, ysca;
	struct Iforwardref  *nextforwardref;
} FORWARDREF;

static INTBIG            io_dxfreadalllayers;
static INTBIG            io_dxfflatteninput;
static INTBIG            io_dxfcurline, io_dxflength;
static INTBIG            io_dxfentityhandle;
static INTBIG            io_dxfgroupid, io_dxfpairvalid, io_dxfignoredpoints, io_dxfignoredattribdefs,
						 io_dxfignoredattributes;
static INTBIG            io_dxfreadpolylines, io_dxfreadlines, io_dxfreadcircles, io_dxfreadsolids,
						 io_dxfread3dfaces, io_dxfreadarcs, io_dxfreadinserts, io_dxfreadtexts;
static CHAR              io_dxfline[MAXCHARSPERLINE];
static DXFLAYER         *io_dxffirstlayer = 0;
static FORWARDREF       *io_dxffirstforwardref;
static NODEPROTO        *io_dxfmaincell;
static NODEPROTO        *io_dxfcurcell;
static CHAR            **io_dxfheadertext;
static INTSML           *io_dxfheaderid;
static INTBIG            io_dxfinputmode;			/* 0: pairs not read, 1: normal pairs, 2: blank lines */
static INTBIG            io_dxfheadertotal = 0;
static INTBIG            io_dxfheadercount = 0;
static INTBIG            io_dxfvalidlayercount = 0;
static CHAR            **io_dxfvalidlayernames;
static INTBIG           *io_dxfvalidlayernumbers;
static void             *io_dxfignoredlayerstringarray = 0;
static INTBIG            io_dxflayerkey = 0;
static INTBIG            io_dxfdispunit;
static void             *io_dxfprogressdialog;

/* prototypes for local routines */
static INTBIG     io_dxfgetacceptablelayers(void);
static INTBIG     io_dxfacceptablelayer(DXFLAYER *layer);
static CHAR      *io_dxfblockname(CHAR *name);
static void       io_dxfclearacceptablelayers(void);
static void       io_dxfclearlayers(void);
static INTBIG     io_dxfextractinsert(NODEPROTO *onp, INTBIG x, INTBIG y, float xsca, float ysca, INTBIG rot, NODEPROTO *np);
static NODEPROTO *io_dxfgetscaledcell(NODEPROTO *onp, float xsca, float ysca);
static DXFLAYER  *io_dxfgetlayer(CHAR *name);
static INTBIG     io_dxfgetnextline(FILE *io, CHAR *text, INTBIG canBeBlank);
static INTBIG     io_dxfgetnextpair(FILE *io, INTBIG *groupID, CHAR **text);
static INTBIG     io_dxfignoreentity(FILE *io);
static void       io_dxfoptionsdlog(void);
static void       io_dxfpushpair(INTBIG groupID, CHAR *text);
static INTBIG     io_dxfreadcircleentity(FILE *io, INTBIG *x, INTBIG *y, INTBIG *z, INTBIG *rad, DXFLAYER **layer);
static BOOLEAN    io_dxfreadentities(FILE *io, LIBRARY *lib);
static BOOLEAN    io_dxfignoresection(FILE *io);
static INTBIG     io_dxfreadblock(FILE *io, CHAR **name);
static INTBIG     io_dxfreadarcentity(FILE *io, INTBIG *x, INTBIG *y, INTBIG *z, INTBIG *rad, double *sangle,
					double *eangle, DXFLAYER **retLayer);
static BOOLEAN    io_dxfreadheadersection(FILE *io);
static INTBIG     io_dxfreadinsertentity(FILE *io, INTBIG *x, INTBIG *y, INTBIG *z, CHAR **name, INTBIG *rot,
					float *xsca, float *ysca, INTBIG *xrep, INTBIG *yrep, INTBIG *xspa, INTBIG *yspa, DXFLAYER **retLayer);
static INTBIG     io_dxfreadlineentity(FILE *io, INTBIG *x1, INTBIG *y1, INTBIG *z1, INTBIG *x2, INTBIG *y2, INTBIG *z2,
					INTBIG *lineType, DXFLAYER **retLayer);
static INTBIG     io_dxfreadpolylineentity(FILE *io, INTBIG **xp, INTBIG **yp, INTBIG **zp, double **bulge,
					INTBIG *count, INTBIG *closed, INTBIG *lineType, DXFLAYER **layer);
static INTBIG     io_dxfreadsolidentity(FILE *io, INTBIG *x1, INTBIG *y1, INTBIG *z1, INTBIG *x2, INTBIG *y2, INTBIG *z2,
					INTBIG *x3, INTBIG *y3, INTBIG *z3, INTBIG *x4, INTBIG *y4, INTBIG *z4, DXFLAYER **retLayer);
static INTBIG     io_dxfreadtextentity(FILE *io, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy, CHAR **msg,
					UINTBIG *descript, DXFLAYER **retLayer);
static BOOLEAN    io_dxfreadtablessection(FILE *io);
static INTBIG     io_dxfread3dfaceentity(FILE *io, INTBIG *x1, INTBIG *y1, INTBIG *z1, INTBIG *x2, INTBIG *y2,
					INTBIG *z2, INTBIG *x3, INTBIG *y3, INTBIG *z3, INTBIG *x4, INTBIG *y4, INTBIG *z4, DXFLAYER **layer);
static void       io_dxfwritecell(NODEPROTO *np, INTBIG subcells);
static CHAR      *io_dxfcellname(NODEPROTO *np);
static void       io_dxfsetcurunits(void);
#ifdef PRESERVEDXF
  static void  *io_dxfpreservestringarray = 0;
  static void io_dxfstartgathering(void);
  static void io_dxfstoredxf(NODEINST *ni);
#endif

/****************************************** WRITING DXF ******************************************/

/*
 * Routine to free all memory associated with this module.
 */
void io_freedxfmemory(void)
{
	REGISTER INTBIG i;

	for(i=0; i<io_dxfheadercount; i++)
		efree((CHAR *)io_dxfheadertext[i]);
	if (io_dxfheadertotal > 0)
	{
		efree((CHAR *)io_dxfheadertext);
		efree((CHAR *)io_dxfheaderid);
	}
	io_dxfclearlayers();
	io_dxfclearacceptablelayers();
	if (io_dxfignoredlayerstringarray != 0)
		killstringarray(io_dxfignoredlayerstringarray);
#ifdef PRESERVEDXF
	if (io_dxfpreservestringarray != 0)
		killstringarray(io_dxfpreservestringarray);
#endif
}

/*
 * Routine to initialize DXF I/O.
 */
void io_initdxf(void)
{
	extern COMCOMP io_dxfp;

	DiaDeclareHook(x_("dxfopt"), &io_dxfp, io_dxfoptionsdlog);
}

BOOLEAN io_writedxflibrary(LIBRARY *lib)
{
	CHAR file[100], *truename;
	REGISTER CHAR *name, *pt;
	REGISTER INTBIG i, j, len, code;
	REGISTER NODEINST *ni;
	REGISTER LIBRARY *olib;
	REGISTER VARIABLE *varheadertext, *varheaderid;
	REGISTER NODEPROTO *np;
	static CHAR *ignorefromheader[] = {x_("$DWGCODEPAGE"), x_("$HANDSEED"), x_("$SAVEIMAGES"), 0};

	/* create the proper disk file for the DXF */
	if (io_dxflayerkey == 0) io_dxflayerkey = makekey(x_("IO_dxf_layer"));
	if (lib->curnodeproto == NONODEPROTO)
	{
		ttyputerr(_("Must be editing a cell to generate DXF output"));
		return(TRUE);
	}
	(void)estrcpy(file, lib->curnodeproto->protoname);
	if (estrcmp(&file[estrlen(file)-4],x_(".dxf")) != 0) (void)estrcat(file, x_(".dxf"));
	name = truepath(file);
	io_fileout = xcreate(name, io_filetypedxf, _("DXF File"), &truename);
	if (io_fileout == NULL)
	{
		if (truename != 0) ttyputerr(_("Cannot write %s"), truename);
		return(TRUE);
	}

	/* set the scale */
	io_dxfsetcurunits();

	/* write the header */
	varheadertext = getval((INTBIG)lib, VLIBRARY, VSTRING|VISARRAY, x_("IO_dxf_header_text"));
	varheaderid = getval((INTBIG)lib, VLIBRARY, VSHORT|VISARRAY, x_("IO_dxf_header_ID"));
	if (varheadertext != NOVARIABLE && varheaderid != NOVARIABLE)
	{
		efprintf(io_fileout, x_("  0\nSECTION\n"));
		efprintf(io_fileout, x_("  2\nHEADER\n"));
		len = mini(getlength(varheadertext), getlength(varheaderid));
		for(i=0; i<len; i++)
		{
			/* remove entries that confuse the issues */
			pt = ((CHAR **)varheadertext->addr)[i];
			code = ((INTSML *)varheaderid->addr)[i];
			if (code == 9 && i <= len-2)
			{
				for(j=0; ignorefromheader[j] != 0; j++)
					if (estrcmp(pt, ignorefromheader[j]) == 0 ||
						estrcmp(&pt[1], ignorefromheader[j]) == 0) break;
				if (ignorefromheader[j] != 0)
				{
					i++;
					continue;
				}
			}

			/* make sure Autocad version is correct */
			if (estrcmp(pt, x_("$ACADVER")) == 0 && i <= len-2)
			{
				efprintf(io_fileout, x_("%3ld\n%s\n"), code, pt);
				efprintf(io_fileout, x_("  1\nAC1009\n"));
				i++;
				continue;
			}

			efprintf(io_fileout, x_("%3ld\n%s\n"), code, pt);
		}
		efprintf(io_fileout, x_("  0\nENDSEC\n"));
	}

	/* write any subcells */
	io_dxfentityhandle = 0x100;
	efprintf(io_fileout, x_("  0\nSECTION\n"));
	efprintf(io_fileout, x_("  2\nBLOCKS\n"));
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = 0;
	for(ni = lib->curnodeproto->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		np = ni->proto;
		if (np->primindex == 0 && np->temp1 == 0) io_dxfwritecell(np, 1);
	}
	efprintf(io_fileout, x_("  0\nENDSEC\n"));

	/* write the objects in the cell */
	efprintf(io_fileout, x_("  0\nSECTION\n"));
	efprintf(io_fileout, x_("  2\nENTITIES\n"));
	io_dxfwritecell(lib->curnodeproto, 0);
	efprintf(io_fileout, x_("  0\nENDSEC\n"));

	efprintf(io_fileout, x_("  0\nEOF\n"));
	xclose(io_fileout);
	ttyputmsg(_("%s written"), truename);
	return(FALSE);
}

/*
 * routine to write the contents of cell "np".  If "subcells" is nonzero, do a recursive
 * descent through the subcells in this cell, writing out "block" definitions.
 */
void io_dxfwritecell(NODEPROTO *np, INTBIG subcells)
{
	REGISTER CHAR *layername;
	REGISTER INTBIG i, j, len, xc, yc;
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var;
	INTBIG fx, fy;
	double startoffset, endangle, startangle, rot;
	XARRAY trans;
	static POLYGON *poly = NOPOLYGON;

	if (subcells != 0)
	{
		np->temp1 = 1;
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if (ni->proto->primindex == 0 && ni->proto->temp1 == 0)
				io_dxfwritecell(ni->proto, 1);
		}
		efprintf(io_fileout, x_("  0\nBLOCK\n"));
		efprintf(io_fileout, x_("  2\n%s\n"), io_dxfcellname(np));
		efprintf(io_fileout, x_(" 10\n0\n"));
		efprintf(io_fileout, x_(" 20\n0\n"));
		efprintf(io_fileout, x_(" 30\n0\n"));
		efprintf(io_fileout, x_(" 70\n0\n"));
	}

	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* handle instances */
		if (ni->proto->primindex == 0)
		{
			efprintf(io_fileout, x_("  0\nINSERT\n"));
			efprintf(io_fileout, x_("  8\nUNKNOWN\n"));
			efprintf(io_fileout, x_("  5\n%3lX\n"), io_dxfentityhandle++);
			efprintf(io_fileout, x_("  2\n%s\n"), io_dxfcellname(ni->proto));
			xc = (ni->lowx + ni->highx - (ni->proto->lowx + ni->proto->highx)) / 2;
			yc = (ni->lowy + ni->highy - (ni->proto->lowy + ni->proto->highy)) / 2;
			efprintf(io_fileout, x_(" 10\n%g\n"), scaletodispunit(xc, io_dxfdispunit));
			efprintf(io_fileout, x_(" 20\n%g\n"), scaletodispunit(yc, io_dxfdispunit));
			efprintf(io_fileout, x_(" 30\n0\n"));
			rot = (double)ni->rotation / 10.0;
			efprintf(io_fileout, x_(" 50\n%g\n"), rot);
			continue;
		}

		/* determine layer name for this node */
		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, io_dxflayerkey);
		if (var != NOVARIABLE) layername = (CHAR *)var->addr; else
		{
			/* examine technology for proper layer name */
			layername = x_("UNKNOWN");
			if (ni->proto->primindex != 0)
			{
				var = getval((INTBIG)ni->proto->tech, VTECHNOLOGY, VSTRING|VISARRAY, x_("IO_dxf_layer_names"));
				if (var != NOVARIABLE)
				{
					/* get polygon */
					(void)needstaticpolygon(&poly, 4, io_tool->cluster);
					i = nodepolys(ni, 0, NOWINDOWPART);
					for(j=0; j<i; j++)
					{
						shapenodepoly(ni, j, poly);
						layername = ((CHAR **)var->addr)[poly->layer];
						if (*layername != 0) break;
					}
				}
			}
		}

		/* get the node center */
		xc = (ni->lowx + ni->highx) / 2;
		yc = (ni->lowy + ni->highy) / 2;

		/* handle circles and arcs */
		if (ni->proto == art_circleprim || ni->proto == art_thickcircleprim)
		{
			getarcdegrees(ni, &startoffset, &endangle);
			if (startoffset != 0.0 || endangle != 0.0) efprintf(io_fileout, x_("  0\nARC\n")); else
				efprintf(io_fileout, x_("  0\nCIRCLE\n"));
			efprintf(io_fileout, x_("  8\n%s\n"), layername);
			efprintf(io_fileout, x_("  5\n%3lX\n"), io_dxfentityhandle++);
			efprintf(io_fileout, x_(" 10\n%g\n"), scaletodispunit(xc, io_dxfdispunit));
			efprintf(io_fileout, x_(" 20\n%g\n"), scaletodispunit(yc, io_dxfdispunit));
			efprintf(io_fileout, x_(" 30\n0\n"));
			efprintf(io_fileout, x_(" 40\n%g\n"), scaletodispunit((ni->highy - ni->lowy) / 2, io_dxfdispunit));
			if (startoffset != 0.0 || endangle != 0.0)
			{
				startoffset = startoffset * 180.0 / EPI;
				endangle = endangle * 180.0 / EPI;
				startangle = (double)ni->rotation / 10.0 + startoffset;
				if (ni->transpose != 0)
				{
					startangle = 270.0 - startangle - endangle;
					if (startangle < 0.0) startangle += 360.0;
				}
				endangle += startangle;
				if (endangle >= 360.0) endangle -= 360.0;
				efprintf(io_fileout, x_(" 50\n%g\n"), startangle);
				efprintf(io_fileout, x_(" 51\n%g\n"), endangle);
			}
			continue;
		}

		/* handle polygons */
		if (ni->proto == art_openedpolygonprim || ni->proto == art_openeddashedpolygonprim ||
			ni->proto == art_closedpolygonprim)
		{
			makerot(ni, trans);
			var = gettrace(ni);
			len = getlength(var);
			if (len == 4 && ni->proto != art_closedpolygonprim)
			{
				/* line */
				efprintf(io_fileout, x_("  0\nLINE\n"));
				efprintf(io_fileout, x_("  8\n%s\n"), layername);
				efprintf(io_fileout, x_("  5\n%3lX\n"), io_dxfentityhandle++);
				xform(((INTBIG *)var->addr)[0]+xc, ((INTBIG *)var->addr)[1]+yc, &fx, &fy, trans);
				efprintf(io_fileout, x_(" 10\n%g\n"), scaletodispunit(fx, io_dxfdispunit));
				efprintf(io_fileout, x_(" 20\n%g\n"), scaletodispunit(fy, io_dxfdispunit));
				efprintf(io_fileout, x_(" 30\n0\n"));
				xform(((INTBIG *)var->addr)[2]+xc, ((INTBIG *)var->addr)[3]+yc, &fx, &fy, trans);
				efprintf(io_fileout, x_(" 11\n%g\n"), scaletodispunit(fx, io_dxfdispunit));
				efprintf(io_fileout, x_(" 21\n%g\n"), scaletodispunit(fy, io_dxfdispunit));
				efprintf(io_fileout, x_(" 31\n0\n"));
			} else
			{
				/* should write a polyline here */
				for(i=0; i<len-2; i += 2)
				{
					/* line */
					efprintf(io_fileout, x_("  0\nLINE\n"));
					efprintf(io_fileout, x_("  8\n%s\n"), layername);
					efprintf(io_fileout, x_("  5\n%3lX\n"), io_dxfentityhandle++);
					xform(((INTBIG *)var->addr)[i+0]+xc, ((INTBIG *)var->addr)[i+1]+yc, &fx, &fy, trans);
					efprintf(io_fileout, x_(" 10\n%g\n"), scaletodispunit(fx, io_dxfdispunit));
					efprintf(io_fileout, x_(" 20\n%g\n"), scaletodispunit(fy, io_dxfdispunit));
					efprintf(io_fileout, x_(" 30\n0\n"));
					xform(((INTBIG *)var->addr)[i+2]+xc, ((INTBIG *)var->addr)[i+3]+yc, &fx, &fy, trans);
					efprintf(io_fileout, x_(" 11\n%g\n"), scaletodispunit(fx, io_dxfdispunit));
					efprintf(io_fileout, x_(" 21\n%g\n"), scaletodispunit(fy, io_dxfdispunit));
					efprintf(io_fileout, x_(" 31\n0\n"));
				}
				if (ni->proto == art_closedpolygonprim)
				{
					efprintf(io_fileout, x_("  0\nLINE\n"));
					efprintf(io_fileout, x_("  8\n%s\n"), layername);
					efprintf(io_fileout, x_("  5\n%3lX\n"), io_dxfentityhandle++);
					xform(((INTBIG *)var->addr)[len-2]+xc, ((INTBIG *)var->addr)[len-1]+yc, &fx, &fy, trans);
					efprintf(io_fileout, x_(" 10\n%g\n"), scaletodispunit(fx, io_dxfdispunit));
					efprintf(io_fileout, x_(" 20\n%g\n"), scaletodispunit(fy, io_dxfdispunit));
					efprintf(io_fileout, x_(" 30\n0\n"));
					xform(((INTBIG *)var->addr)[0]+xc, ((INTBIG *)var->addr)[1]+yc, &fx, &fy, trans);
					efprintf(io_fileout, x_(" 11\n%g\n"), scaletodispunit(fx, io_dxfdispunit));
					efprintf(io_fileout, x_(" 21\n%g\n"), scaletodispunit(fy, io_dxfdispunit));
					efprintf(io_fileout, x_(" 31\n0\n"));
				}
			}
		}
	}
	if (subcells != 0)
		efprintf(io_fileout, x_("  0\nENDBLK\n"));
}

CHAR *io_dxfcellname(NODEPROTO *np)
{
	REGISTER CHAR *buf;
	REGISTER INTBIG i, leng;
	REGISTER NODEPROTO *onp;
	REGISTER void *infstr;

	if (namesame(np->protoname, np->lib->libname) == 0)
	{
		/* use another name */
		leng = estrlen(np->protoname) + 4;
		buf = (CHAR *)emalloc(leng * SIZEOFCHAR, io_tool->cluster);
		if (buf == 0) return(np->protoname);
		for(i=1; i<1000; i++)
		{
			esnprintf(buf, leng, x_("%s%ld"), np->protoname, i);
			for(onp = np->lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
				if (namesame(onp->protoname, buf) == 0) break;
			if (onp == NONODEPROTO) break;
		}
		infstr = initinfstr();
		addstringtoinfstr(infstr, buf);
		efree(buf);
		return(returninfstr(infstr));
	}

	/* just return the cell name */
	return(np->protoname);
}

/****************************************** READING DXF ******************************************/

/*
 * Routine to read the DXF file into library "lib".  Returns true on error.
 */
BOOLEAN io_readdxflibrary(LIBRARY *lib)
{
	FILE *io;
	CHAR *filename, *text, warning[256], countStr[50], **list;
	INTBIG groupID;
	BOOLEAN err;
	INTBIG len, i, *curstate;
	NODEPROTO *np;
	FORWARDREF *fr, *nextfr;
	NODEINST *ni;
	REGISTER void *infstr;

	/* parameters for reading files */
	if (io_dxflayerkey == 0) io_dxflayerkey = makekey(x_("IO_dxf_layer"));
	curstate = io_getstatebits();
	io_dxfreadalllayers = curstate[0] & DXFALLLAYERS;
	io_dxfflatteninput = curstate[0] & DXFFLATTENINPUT;

	/* set the scale */
	io_dxfsetcurunits();

	/* examine technology for acceptable DXF layer names */
	if (io_dxfgetacceptablelayers() != 0) return(TRUE);

	/* get the DXF file */
	if ((io = xopen(lib->libfile, io_filetypedxf, x_(""), &filename)) == NULL)
	{
		ttyputerr(_("File %s not found"), lib->libfile);
		return(TRUE);
	}

	/* determine file length */
	io_dxflength = filesize(io);
	if (io_verbose < 0 && io_dxflength > 0)
	{
		io_dxfprogressdialog = DiaInitProgress(0, 0);
		if (io_dxfprogressdialog == 0)
		{
			xclose(io);
			return(TRUE);
		}
		DiaSetProgress(io_dxfprogressdialog, 0,  io_dxflength);
	}

	/* make the only cell in this library */
	io_dxfmaincell = us_newnodeproto(lib->libname, lib);
	if (io_dxfmaincell == NONODEPROTO) return(TRUE);
	lib->curnodeproto = io_dxfmaincell;
	io_dxfcurcell = io_dxfmaincell;
	io_dxfheadercount = 0;
	if (io_dxfignoredlayerstringarray == 0)
	{
		io_dxfignoredlayerstringarray = newstringarray(io_tool->cluster);
		if (io_dxfignoredlayerstringarray == 0) return(TRUE);
	}
	clearstrings(io_dxfignoredlayerstringarray);

	/* read the file */
	io_dxfpairvalid = 0;
	io_dxfcurline = 0;
	err = FALSE;
	io_dxfclearlayers();
	io_dxffirstforwardref = 0;
	io_dxfignoredpoints = io_dxfignoredattributes = io_dxfignoredattribdefs = 0;
	io_dxfreadpolylines = io_dxfreadlines = io_dxfreadcircles = io_dxfreadsolids = 0;
	io_dxfread3dfaces = io_dxfreadarcs = io_dxfreadinserts = io_dxfreadtexts = 0;
	io_dxfinputmode = 0;
	for(;;)
	{
		if (stopping(STOPREASONDXF)) break;
		if (io_dxfgetnextpair(io, &groupID, &text) != 0) break;

		/* must have section change here */
		if (groupID != 0)
		{
			ttyputerr(_("Expected group 0 (start section) at line %ld"), io_dxfcurline);
			break;
		}

		if (estrcmp(text, x_("EOF")) == 0) break;

		if (estrcmp(text, x_("SECTION")) == 0)
		{
			/* see what section is coming */
			if (io_dxfgetnextpair(io, &groupID, &text) != 0) break;
			if (groupID != 2)
			{
				ttyputerr(_("Expected group 2 (name) at line %ld"), io_dxfcurline);
				err = TRUE;
				break;
			}
			if (estrcmp(text, x_("HEADER")) == 0)
			{
				if ((err = io_dxfreadheadersection(io)) != 0) break;
				continue;
			}
			if (estrcmp(text, x_("TABLES")) == 0)
			{
				if ((err = io_dxfreadtablessection(io)) != 0) break;
				continue;
			}
			if (estrcmp(text, x_("BLOCKS")) == 0)
			{
				if ((err = io_dxfreadentities(io, lib)) != 0) break;
				continue;
			}
			if (estrcmp(text, x_("ENTITIES")) == 0)
			{
				if ((err = io_dxfreadentities(io, lib)) != 0) break;
				continue;
			}
			if (estrcmp(text, x_("CLASSES")) == 0)
			{
				if ((err = io_dxfignoresection(io)) != 0) break;
				continue;
			}
			if (estrcmp(text, x_("OBJECTS")) == 0)
			{
				if ((err = io_dxfignoresection(io)) != 0) break;
				continue;
			}
		}
		ttyputerr(_("Unknown section name (%s) at line %ld"), text, io_dxfcurline);
		err = TRUE;
		break;
	}
	xclose(io);
	if (io_verbose < 0 && io_dxflength > 0)
	{
		DiaSetProgress(io_dxfprogressdialog, io_dxflength, io_dxflength);
		DiaDoneProgress(io_dxfprogressdialog);
	}

	/* insert forward references */
	for(fr = io_dxffirstforwardref; fr != 0; fr = nextfr)
	{
		nextfr = fr->nextforwardref;

		/* have to search by hand because of weird prototype names */
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			if (estrcmp(np->protoname, fr->refname) == 0) break;
		if (np == NONODEPROTO)
		{
			ttyputmsg(_("Cannot find block '%s'"), fr->refname);
		} else
		{
			if (io_dxfflatteninput != 0)
			{
				if (io_dxfextractinsert(np, fr->x, fr->y, fr->xsca, fr->ysca, fr->rot, fr->parent) != 0) return(TRUE);
			} else
			{
				if (fr->xsca != 1.0 || fr->ysca != 1.0)
				{
					np = io_dxfgetscaledcell(np, fr->xsca, fr->ysca);
					if (np == NONODEPROTO) return(TRUE);
				}
				ni = newnodeinst(np, fr->x+np->lowx, fr->x+np->highx, fr->y+np->lowy, fr->y+np->highy,
					0, fr->rot*10, fr->parent);
				if (ni == NONODEINST) return(TRUE);
				ni->userbits |= NEXPAND;
			}
		}
		efree(fr->refname);
		efree((CHAR *)fr);
	}

	/* save header with library */
	if (io_dxfheadercount > 0)
	{
		setval((INTBIG)lib, VLIBRARY, x_("IO_dxf_header_text"), (INTBIG)io_dxfheadertext,
			VSTRING|VISARRAY|(io_dxfheadercount<<VLENGTHSH));
		setval((INTBIG)lib, VLIBRARY, x_("IO_dxf_header_ID"), (INTBIG)io_dxfheaderid,
			VSHORT|VISARRAY|(io_dxfheadercount<<VLENGTHSH));
	}

	/* recompute bounds */
	(*el_curconstraint->solve)(NONODEPROTO);

	if (io_dxfreadpolylines > 0 || io_dxfreadlines > 0 || io_dxfreadcircles > 0 ||
		io_dxfreadsolids > 0 || io_dxfread3dfaces > 0 || io_dxfreadarcs > 0 ||
		io_dxfreadtexts > 0 || io_dxfreadinserts > 0)
	{
		estrcpy(warning, _("Read"));
		i = 0;
		if (io_dxfreadpolylines > 0)
		{
			if (i > 0) estrcat(warning, x_(","));	i++;
			esnprintf(countStr, 50, _(" %ld polylines"), io_dxfreadpolylines);
			estrcat(warning, countStr);
		}
		if (io_dxfreadlines > 0)
		{
			if (i > 0) estrcat(warning, x_(","));	i++;
			esnprintf(countStr, 50, _(" %ld lines"), io_dxfreadlines);
			estrcat(warning, countStr);
		}
		if (io_dxfreadcircles > 0)
		{
			if (i > 0) estrcat(warning, x_(","));	i++;
			esnprintf(countStr, 50, _(" %ld circles"), io_dxfreadcircles);
			estrcat(warning, countStr);
		}
		if (io_dxfreadsolids > 0)
		{
			if (i > 0) estrcat(warning, x_(","));	i++;
			esnprintf(countStr, 50, _(" %ld solids"), io_dxfreadsolids);
			estrcat(warning, countStr);
		}
		if (io_dxfread3dfaces > 0)
		{
			if (i > 0) estrcat(warning, x_(","));	i++;
			esnprintf(countStr, 50, _(" %ld 3d faces"), io_dxfread3dfaces);
			estrcat(warning, countStr);
		}
		if (io_dxfreadarcs > 0)
		{
			if (i > 0) estrcat(warning, x_(","));	i++;
			esnprintf(countStr, 50, _(" %ld arcs"), io_dxfreadarcs);
			estrcat(warning, countStr);
		}
		if (io_dxfreadtexts > 0)
		{
			if (i > 0) estrcat(warning, x_(","));	i++;
			esnprintf(countStr, 50, _(" %ld texts"), io_dxfreadtexts);
			estrcat(warning, countStr);
		}
		if (io_dxfreadinserts > 0)
		{
			if (i > 0) estrcat(warning, x_(","));	i++;
			esnprintf(countStr, 50, _(" %ld inserts"), io_dxfreadinserts);
			estrcat(warning, countStr);
		}
		ttyputmsg(warning);
	}

	if (io_dxfignoredpoints > 0 || io_dxfignoredattributes > 0 || io_dxfignoredattribdefs > 0)
	{
		estrcpy(warning, _("Ignored"));
		i = 0;
		if (io_dxfignoredpoints > 0)
		{
			if (i > 0) estrcat(warning, x_(","));	i++;
			esnprintf(countStr, 50, _(" %ld points"), io_dxfignoredpoints);
			estrcat(warning, countStr);
		}
		if (io_dxfignoredattributes > 0)
		{
			if (i > 0) estrcat(warning, x_(","));	i++;
			esnprintf(countStr, 50, _(" %ld attributes"), io_dxfignoredattributes);
			estrcat(warning, countStr);
		}
		if (io_dxfignoredattribdefs > 0)
		{
			if (i > 0) estrcat(warning, x_(","));	i++;
			esnprintf(countStr, 50, _(" %ld attribute definitions"), io_dxfignoredattribdefs);
			estrcat(warning, countStr);
		}
		ttyputmsg(warning);
	}

	/* say which layers were ignored */
	list = getstringarray(io_dxfignoredlayerstringarray, &len);
	if (len > 0)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, _("Ignored layers "));
		for(i=0; i<len; i++)
		{
			if (i > 0) addstringtoinfstr(infstr, x_(", "));
			formatinfstr(infstr, x_("'%s'"), list[i]);
		}
		ttyputmsg(x_("%s"), returninfstr(infstr));
	}

	return(err);
}

/****************************************** READING SECTIONS ******************************************/

BOOLEAN io_dxfreadheadersection(FILE *io)
{
	CHAR *text, **newtext;
	INTBIG groupID, line, newtotal, i;
	INTSML *newid;

	/* just save everything until the end-of-section */
	for(line=0; ; line++)
	{
		if (io_dxfgetnextpair(io, &groupID, &text) != 0) return(1);
		if (groupID == 0 && estrcmp(text, x_("ENDSEC")) == 0) break;

		/* save it */
		if (io_dxfheadercount >= io_dxfheadertotal)
		{
			newtotal = io_dxfheadercount + 25;
			newtext = (CHAR **)emalloc(newtotal * (sizeof (CHAR *)), io_tool->cluster);
			if (newtext == 0) return(1);
			newid = (INTSML *)emalloc(newtotal * SIZEOFINTSML, io_tool->cluster);
			if (newid == 0) return(1);
			for(i=0; i<io_dxfheadercount; i++)
			{
				newtext[i] = io_dxfheadertext[i];
				newid[i] = io_dxfheaderid[i];
			}
			for(i=io_dxfheadercount; i<newtotal; i++) newtext[i] = 0;
			if (io_dxfheadertotal > 0)
			{
				efree((CHAR *)io_dxfheadertext);
				efree((CHAR *)io_dxfheaderid);
			}
			io_dxfheadertext = newtext;
			io_dxfheaderid = newid;
			io_dxfheadertotal = newtotal;
		}
		if (io_dxfheadertext[io_dxfheadercount] == 0)
			(void)allocstring(&io_dxfheadertext[io_dxfheadercount], text, io_tool->cluster);
		io_dxfheaderid[io_dxfheadercount] = (INTSML)groupID;
		io_dxfheadercount++;
	}
	return(0);
}

BOOLEAN io_dxfreadtablessection(FILE *io)
{
	CHAR *text;
	INTBIG groupID;
	DXFLAYER *layer, *l;

	/* just ignore everything until the end-of-section */
	for(;;)
	{
		if (stopping(STOPREASONDXF)) return(1);
		if (io_dxfgetnextpair(io, &groupID, &text) != 0) return(1);

		/* quit now if at the end of the table section */
		if (groupID == 0 && estrcmp(text, x_("ENDSEC")) == 0) break;

		/* must be a 'TABLE' declaration */
		if (groupID != 0 || estrcmp(text, x_("TABLE")) != 0) continue;

		/* a table: see what kind it is */
		if (io_dxfgetnextpair(io, &groupID, &text) != 0) return(1);
		if (groupID != 2 || estrcmp(text, x_("LAYER")) != 0) continue;

		/* a layer table: ignore the size information */
		if (io_dxfgetnextpair(io, &groupID, &text) != 0) return(1);
		if (groupID != 70) continue;

		/* read the layers */
		layer = 0;
		for(;;)
		{
			if (stopping(STOPREASONDXF)) return(1);
			if (io_dxfgetnextpair(io, &groupID, &text) != 0) return(1);
			if (groupID == 0 && estrcmp(text, x_("ENDTAB")) == 0) break;
			if (groupID == 0 && estrcmp(text, x_("LAYER")) == 0)
			{
				/* make a new layer */
				layer = (DXFLAYER *)emalloc(sizeof (DXFLAYER), io_tool->cluster);
				if (layer == 0) return(1);
				layer->layerName = 0;
				layer->layerColor = -1;
				layer->layerRed = 1.0;
				layer->layerGreen = 1.0;
				layer->layerBlue = 1.0;
				layer->next = io_dxffirstlayer;
				io_dxffirstlayer = layer;
			}
			if (groupID == 2 && layer != 0)
			{
				if (allocstring(&layer->layerName, text, io_tool->cluster)) return(1);
			}
			if (groupID == 62 && layer != 0)
			{
				layer->layerColor = eatoi(text);
				for(l = io_dxffirstlayer; l != 0; l = l->next)
				{
					if (l == layer) continue;
					if (l->layerColor == layer->layerColor) break;
				}
				if (l != 0)
				{
					layer->layerRed = l->layerRed;
					layer->layerGreen = l->layerGreen;
					layer->layerBlue = l->layerBlue;
				} else
				{
					switch (layer->layerColor)
					{
						case 1:			    /* red */
							layer->layerRed = 1.0;    layer->layerGreen = 0.0;    layer->layerBlue = 0.0;
							break;
						case 2:			    /* yellow */
							layer->layerRed = 1.0;    layer->layerGreen = 1.0;    layer->layerBlue = 0.0;
							break;
						case 3:			    /* green */
							layer->layerRed = 0.0;    layer->layerGreen = 1.0;    layer->layerBlue = 0.0;
							break;
						case 4:			    /* cyan */
							layer->layerRed = 0.0;    layer->layerGreen = 1.0;    layer->layerBlue = 1.0;
							break;
						case 5:			    /* blue */
							layer->layerRed = 0.0;    layer->layerGreen = 0.0;    layer->layerBlue = 1.0;
							break;
						case 6:			    /* magenta */
							layer->layerRed = 1.0;    layer->layerGreen = 0.0;    layer->layerBlue = 1.0;
							break;
						case 7:			    /* white (well, gray) */
							layer->layerRed = 0.75;    layer->layerGreen = 0.75;    layer->layerBlue = 0.75;
							break;
						default:			/* unknown layer */
							layer->layerRed = (float)rand() / 65535.0f;
							layer->layerGreen = (float)rand() / 65535.0f;
							layer->layerBlue = (float)rand() / 65535.0f;
							break;
					}
				}
			}
		}
	}
	return(0);
}

BOOLEAN io_dxfignoresection(FILE *io)
{
	CHAR *text;
	INTBIG groupID;

	/* just ignore everything until the end-of-section */
	for(;;)
	{
		if (io_dxfgetnextpair(io, &groupID, &text) != 0) return(1);
		if (groupID == 0 && estrcmp(text, x_("ENDSEC")) == 0) break;
	}
	return(0);
}

/****************************************** READING ENTITIES ******************************************/

BOOLEAN io_dxfreadentities(FILE *io, LIBRARY *lib)
{
	CHAR *text, *msg, *pt;
	float xsca, ysca;
	INTBIG groupID, count, i, closed, xrep, yrep, rot, lineType, start, last;
	INTBIG x1,y1,z1, x2,y2,z2, x3,y3,z3, x4,y4,z4, rad, *xp,*yp,*zp, coord[8], *coords, lx, hx, ly, hy,
		xc, yc, xspa, yspa, iangle;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	double sangle, eangle, startoffset, bulgesign;
	double *bulge;
	DXFLAYER *layer;
	NODEINST *ni;
	NODEPROTO *np;
	VARIABLE *var;
	FORWARDREF *fr;

	/* read the blocks/entities section */
	for(;;)
	{
		if (stopping(STOPREASONDXF)) return(1);
#ifdef PRESERVEDXF
		io_dxfstartgathering();
#endif
		if (io_dxfgetnextpair(io, &groupID, &text) != 0) return(1);
		if (groupID != 0)
		{
			ttyputerr(_("Unknown group code (%ld) at line %ld"), groupID, io_dxfcurline);
			return(1);
		}

		if (estrcmp(text, x_("ARC")) == 0)
		{
			if (io_dxfreadarcentity(io, &x1, &y1, &z1, &rad, &sangle, &eangle, &layer) != 0) return(1);
			if (io_dxfacceptablelayer(layer) == 0) continue;
			if (sangle >= 360.0) sangle -= 360.0;
			iangle = rounddouble(sangle * 10.0);
			ni = newnodeinst(art_circleprim, x1-rad, x1+rad, y1-rad, y1+rad, 0, iangle%3600,
				io_dxfcurcell);
			if (ni == NONODEINST) return(1);
#ifdef PRESERVEDXF
			io_dxfstoredxf(ni);
#endif
			if (sangle > eangle) eangle += 360.0;
			startoffset = sangle;
			startoffset -= (double)iangle / 10.0;
			setarcdegrees(ni, startoffset * EPI / 180.0, (eangle-sangle) * EPI / 180.0);
			setvalkey((INTBIG)ni, VNODEINST, io_dxflayerkey, (INTBIG)layer->layerName, VSTRING);
			io_dxfreadarcs++;
			continue;
		}
		if (estrcmp(text, x_("ATTDEF")) == 0)
		{
			if (io_dxfignoreentity(io) != 0) return(1);
			io_dxfignoredattribdefs++;
			continue;
		}
		if (estrcmp(text, x_("ATTRIB")) == 0)
		{
			if (io_dxfignoreentity(io) != 0) return(1);
			io_dxfignoredattributes++;
			continue;
		}
		if (estrcmp(text, x_("BLOCK")) == 0)
		{
			if (io_dxfreadblock(io, &msg) != 0) return(1);
			if (msg != 0)
			{
				io_dxfcurcell = us_newnodeproto(io_dxfblockname(msg), lib);
				if (io_dxfcurcell == NONODEPROTO) return(1);
				efree(msg);
			}
			continue;
		}
		if (estrcmp(text, x_("CIRCLE")) == 0)
		{
			if (io_dxfreadcircleentity(io, &x1,&y1,&z1, &rad, &layer) != 0) return(1);
			if (io_dxfacceptablelayer(layer) == 0) continue;
			ni = newnodeinst(art_circleprim, x1-rad, x1+rad, y1-rad, y1+rad, 0, 0, io_dxfcurcell);
			if (ni == NONODEINST) return(1);
#ifdef PRESERVEDXF
			io_dxfstoredxf(ni);
#endif
			setvalkey((INTBIG)ni, VNODEINST, io_dxflayerkey, (INTBIG)layer->layerName, VSTRING);
			io_dxfreadcircles++;
			continue;
		}
		if (estrcmp(text, x_("ENDBLK")) == 0)
		{
			if (io_dxfignoreentity(io) != 0) return(1);
			io_dxfcurcell = io_dxfmaincell;
			continue;
		}
		if (estrcmp(text, x_("ENDSEC")) == 0)
		{
			break;
		}
		if (estrcmp(text, x_("INSERT")) == 0)
		{
			if (io_dxfreadinsertentity(io, &x1,&y1,&z1, &msg, &rot, &xsca, &ysca, &xrep, &yrep,
				&xspa, &yspa, &layer) != 0) return(1);
			pt = io_dxfblockname(msg);
			efree(msg);
			if (pt != 0)
			{
				if (xrep != 1 || yrep != 1)
				{
					ttyputmsg(_("Cannot insert block '%s' repeated %ldx%ld times"), pt, xrep, yrep);
					continue;
				}

				/* have to search by hand because of weird prototype names */
				for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
					if (estrcmp(np->protoname, pt) == 0) break;
				if (np == NONODEPROTO)
				{
					fr = (FORWARDREF *)emalloc(sizeof (FORWARDREF), io_tool->cluster);
					if (fr == 0) return(1);
					(void)allocstring(&fr->refname, pt, io_tool->cluster);
					fr->parent = io_dxfcurcell;
					fr->x = x1;		fr->y = y1;
					fr->rot = rot;
					fr->xrep = xrep;	fr->yrep = yrep;
					fr->xspa = xspa;	fr->yspa = yspa;
					fr->xsca = xsca;	fr->ysca = ysca;
					fr->nextforwardref = io_dxffirstforwardref;
					io_dxffirstforwardref = fr;
					continue;
				}

				if (io_dxfflatteninput != 0)
				{
					if (io_dxfextractinsert(np, x1, y1, xsca, ysca, rot, io_dxfcurcell) != 0) return(1);
				} else
				{
					if (xsca != 1.0 || ysca != 1.0)
					{
						np = io_dxfgetscaledcell(np, xsca, ysca);
						if (np == NONODEPROTO) return(1);
					}
					ni = newnodeinst(np, x1+np->lowx, x1+np->highx, y1+np->lowy, y1+np->highy, 0, rot*10, io_dxfcurcell);
					if (ni == NONODEINST) return(1);
					ni->userbits |= NEXPAND;
				}
			}
			io_dxfreadinserts++;
			continue;
		}
		if (estrcmp(text, x_("LINE")) == 0)
		{
			if (io_dxfreadlineentity(io, &x1,&y1,&z1, &x2,&y2,&z2, &lineType, &layer) != 0) return(1);
			if (io_dxfacceptablelayer(layer) == 0) continue;
			lx = mini(x1, x2);		hx = maxi(x1, x2);
			ly = mini(y1, y2);		hy = maxi(y1, y2);
			xc  = (lx + hx) / 2;	yc = (ly + hy) / 2;
			if (lineType == 0) np = art_openedpolygonprim; else
				np = art_openeddashedpolygonprim;
			ni = newnodeinst(np, lx, hx, ly, hy, 0, 0, io_dxfcurcell);
			if (ni == NONODEINST) return(1);
#ifdef PRESERVEDXF
			io_dxfstoredxf(ni);
#endif
			coord[0] = x1 - xc;		coord[1] = y1 - yc;
			coord[2] = x2 - xc;		coord[3] = y2 - yc;
			(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)coord,
				VINTEGER|VISARRAY|(4<<VLENGTHSH));

			setvalkey((INTBIG)ni, VNODEINST, io_dxflayerkey, (INTBIG)layer->layerName, VSTRING);
			io_dxfreadlines++;
			continue;
		}
		if (estrcmp(text, x_("POINT")) == 0)
		{
			if (io_dxfignoreentity(io) != 0) return(1);
			io_dxfignoredpoints++;
			continue;
		}
		if (estrcmp(text, x_("POLYLINE")) == 0)
		{
			if (io_dxfreadpolylineentity(io, &xp, &yp, &zp, &bulge, &count, &closed, &lineType, &layer) != 0) return(1);
			if (io_dxfacceptablelayer(layer) != 0 && count >= 3)
			{
				/* see if there is bulge information */
				for(i=0; i<count; i++) if (bulge[i] != 0.0) break;
				if (i < count)
				{
					/* handle bulges */
					if (closed != 0) start = 0; else start = 1;
					for(i=start; i<count; i++)
					{
						if (i == 0) last = count-1; else last = i-1;
						x1 = xp[last];   y1 = yp[last];
						x2 = xp[i];      y2 = yp[i];
						if (bulge[last] != 0.0)
						{
							/* this segment has a bulge */
							double dist, dx, dy, arcrad, x01, y01, x02, y02, r2, delta_1, delta_12, delta_2,
								xcf, ycf, sa, ea, incangle;

							/* special case the semicircle bulges */
							if (fabs(bulge[last]) == 1.0)
							{
								xc = (x1 + x2) / 2;
								yc = (y1 + y2) / 2;
								if ((y1 == yc && x1 == xc) || (y2 == yc && x2 == xc))
								{
									ttyputerr(_("Domain error in polyline bulge computation"));
									continue;
								}
								sa = atan2((double)(y1-yc), (double)(x1-xc));
								ea = atan2((double)(y2-yc), (double)(x2-xc));
								if (bulge[last] < 0.0)
								{
									r2 = sa;   sa = ea;   ea = r2;
								}
								if (sa < 0.0) sa += 2.0 * EPI;
								sa = sa * 1800.0 / EPI;
								iangle = rounddouble(sa);
								rad = computedistance(xc, yc, x1, y1);
								ni = newnodeinst(art_circleprim, xc-rad, xc+rad, yc-rad, yc+rad, 0,
									iangle, io_dxfcurcell);
								if (ni == NONODEINST) return(1);
								startoffset = sa;
								startoffset -= (double)iangle;
								setarcdegrees(ni, startoffset * EPI / 1800.0, EPI);
								setvalkey((INTBIG)ni, VNODEINST, io_dxflayerkey, (INTBIG)layer->layerName, VSTRING);
								continue;
							}

							/* compute half distance between the points */
							x01 = x1;   y01 = y1;
							x02 = x2;   y02 = y2;
							dx = x02 - x01;   dy = y02 - y01;
							dist = sqrt(dx*dx + dy*dy);

							/* compute radius of arc (bulge is tangent of 1/4 of included arc angle) */
							incangle = atan(bulge[last]) * 4.0;
							arcrad = fabs((dist / 2.0) / sin(incangle / 2.0));
							rad = rounddouble(arcrad);

							/* prepare to compute the two circle centers */
							r2 = arcrad*arcrad;
							delta_1 = -dist / 2.0;
							delta_12 = delta_1 * delta_1;
							delta_2 = sqrt(r2 - delta_12);

							/* pick one center, according to bulge sign */
							bulgesign = bulge[last];
							if (fabs(bulgesign) > 1.0) bulgesign = -bulgesign;
							if (bulgesign > 0.0)
							{
								xcf = x02 + ((delta_1 * (x02-x01)) + (delta_2 * (y01-y02))) / dist;
								ycf = y02 + ((delta_1 * (y02-y01)) + (delta_2 * (x02-x01))) / dist;
							} else
							{
								xcf = x02 + ((delta_1 * (x02-x01)) + (delta_2 * (y02-y01))) / dist;
								ycf = y02 + ((delta_1 * (y02-y01)) + (delta_2 * (x01-x02))) / dist;
							}
							x1 = rounddouble(xcf);   y1 = rounddouble(ycf);

							/* compute angles to the arc endpoints */
							if ((y01 == ycf && x01 == xcf) || (y02 == ycf && x02 == xcf))
							{
								ttyputerr(_("Domain error in polyline computation"));
								continue;
							}
							sa = atan2(y01-ycf, x01-xcf);
							ea = atan2(y02-ycf, x02-xcf);
							if (bulge[last] < 0.0)
							{
								r2 = sa;   sa = ea;   ea = r2;
							}
							if (sa < 0.0) sa += 2.0 * EPI;
							if (ea < 0.0) ea += 2.0 * EPI;
							sa = sa * 1800.0 / EPI;
							ea = ea * 1800.0 / EPI;

							/* create the arc node */
							iangle = rounddouble(sa);
							ni = newnodeinst(art_circleprim, x1-rad, x1+rad, y1-rad, y1+rad, 0,
								iangle%3600, io_dxfcurcell);
							if (ni == NONODEINST) return(1);
							if (sa > ea) ea += 3600.0;
							startoffset = sa;
							startoffset -= (double)iangle;
							setarcdegrees(ni, startoffset * EPI / 1800.0, (ea-sa) * EPI / 1800.0);
							setvalkey((INTBIG)ni, VNODEINST, io_dxflayerkey, (INTBIG)layer->layerName, VSTRING);
							continue;
						}

						/* this segment has no bulge */
						lx = mini(x1, x2);		hx = maxi(x1, x2);
						ly = mini(y1, y2);		hy = maxi(y1, y2);
						xc  = (lx + hx) / 2;	yc = (ly + hy) / 2;
						if (lineType == 0) np = art_openedpolygonprim; else
							np = art_openeddashedpolygonprim;
						ni = newnodeinst(np, lx, hx, ly, hy, 0, 0, io_dxfcurcell);
						if (ni == NONODEINST) return(1);
#ifdef PRESERVEDXF
						io_dxfstoredxf(ni);
#endif
						coord[0] = x1 - xc;		coord[1] = y1 - yc;
						coord[2] = x2 - xc;		coord[3] = y2 - yc;
						(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)coord,
							VINTEGER|VISARRAY|(4<<VLENGTHSH));
						setvalkey((INTBIG)ni, VNODEINST, io_dxflayerkey, (INTBIG)layer->layerName,
							VSTRING);
					}
				} else
				{
					/* no bulges: do simple polygon */
					lx = hx = xp[0];
					ly = hy = yp[0];
					for(i=1; i<count; i++)
					{
						if (xp[i] < lx) lx = xp[i];
						if (xp[i] > hx) hx = xp[i];
						if (yp[i] < ly) ly = yp[i];
						if (yp[i] > hy) hy = yp[i];
					}
					xc  = (lx + hx) / 2;	yc = (ly + hy) / 2;
					if (closed != 0)
					{
						np = art_closedpolygonprim;
					} else
					{
						if (lineType == 0) np = art_openedpolygonprim; else
							np = art_openeddashedpolygonprim;
					}
					ni = newnodeinst(np, lx, hx, ly, hy, 0, 0, io_dxfcurcell);
					if (ni == NONODEINST) return(1);
#ifdef PRESERVEDXF
					io_dxfstoredxf(ni);
#endif
					coords = (INTBIG *)emalloc(count*2 * SIZEOFINTBIG, el_tempcluster);
					if (coords == 0) return(1);
					for(i=0; i<count; i++)
					{
						coords[i*2] = xp[i] - xc;
						coords[i*2+1] = yp[i] - yc;
					}
					(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)coords,
						VINTEGER|VISARRAY|((count*2)<<VLENGTHSH));
					efree((CHAR *)coords);
					setvalkey((INTBIG)ni, VNODEINST, io_dxflayerkey, (INTBIG)layer->layerName, VSTRING);
				}
			}
			efree((CHAR *)xp);    efree((CHAR *)yp);    efree((CHAR *)zp);    efree((CHAR *)bulge);
			io_dxfreadpolylines++;
			continue;
		}
		if (estrcmp(text, x_("SEQEND")) == 0)
		{
			if (io_dxfignoreentity(io) != 0) return(1);
			continue;
		}
		if (estrcmp(text, x_("SOLID")) == 0)
		{
			if (io_dxfreadsolidentity(io, &x1,&y1,&z1, &x2,&y2,&z2, &x3,&y3,&z3, &x4,&y4,&z4,
				&layer) != 0) return(1);
			if (io_dxfacceptablelayer(layer) == 0) continue;
			lx = mini(mini(x1, x2), mini(x3, x4));	hx = maxi(maxi(x1, x2), maxi(x3, x4));
			ly = mini(mini(y1, y2), mini(y3, y4));	hy = maxi(maxi(y1, y2), maxi(y3, y4));
			xc  = (lx + hx) / 2;	yc = (ly + hy) / 2;
			ni = newnodeinst(art_filledpolygonprim, lx, hx, ly, hy, 0, 0, io_dxfcurcell);
			if (ni == NONODEINST) return(1);
#ifdef PRESERVEDXF
			io_dxfstoredxf(ni);
#endif
			coord[0] = x1 - xc;		coord[1] = y1 - yc;
			coord[2] = x2 - xc;		coord[3] = y2 - yc;
			coord[4] = x3 - xc;		coord[5] = y3 - yc;
			coord[6] = x4 - xc;		coord[7] = y4 - yc;
			(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)coord,
				VINTEGER|VISARRAY|(8<<VLENGTHSH));
			setvalkey((INTBIG)ni, VNODEINST, io_dxflayerkey, (INTBIG)layer->layerName, VSTRING);
			io_dxfreadsolids++;
			continue;
		}
		if (estrcmp(text, x_("TEXT")) == 0)
		{
			if (io_dxfreadtextentity(io, &lx, &hx, &ly, &hy, &msg, descript, &layer) != 0) return(1);
			/* if (io_dxfreadalllayers == 0) continue; */
			if (io_dxfacceptablelayer(layer) == 0)
			{
				if (msg != 0) efree(msg);
				continue;
			}
			if (msg != 0)
			{
				for(pt = msg; *pt != 0; pt++) if (*pt != ' ' && *pt != '\t') break;
				if (*pt != 0)
				{
					ni = newnodeinst(gen_invispinprim, lx, hx, ly, hy, 0, 0, io_dxfcurcell);
					if (ni == NONODEINST) return(1);
#ifdef PRESERVEDXF
					io_dxfstoredxf(ni);
#endif
					var = setvalkey((INTBIG)ni, VNODEINST, art_messagekey, (INTBIG)msg,
						VSTRING|VDISPLAY);
					if (var != NOVARIABLE) TDCOPY(var->textdescript, descript);
					setvalkey((INTBIG)ni, VNODEINST, io_dxflayerkey, (INTBIG)layer->layerName, VSTRING);
				}
				efree(msg);
			}
			io_dxfreadtexts++;
			continue;
		}
		if (estrcmp(text, x_("VIEWPORT")) == 0)
		{
			if (io_dxfignoreentity(io) != 0) return(1);
			continue;
		}
		if (estrcmp(text, x_("3DFACE")) == 0)
		{
			if (io_dxfread3dfaceentity(io, &x1,&y1,&z1, &x2,&y2,&z2, &x3,&y3,&z3, &x4,&y4,&z4,
				&layer) != 0) return(1);
			if (io_dxfacceptablelayer(layer) == 0) continue;
			lx = mini(mini(x1, x2), mini(x3, x4));	hx = maxi(maxi(x1, x2), maxi(x3, x4));
			ly = mini(mini(y1, y2), mini(y3, y4));	hy = maxi(maxi(y1, y2), maxi(y3, y4));
			xc  = (lx + hx) / 2;	yc = (ly + hy) / 2;
			ni = newnodeinst(art_closedpolygonprim, lx, hx, ly, hy, 0, 0, io_dxfcurcell);
			if (ni == NONODEINST) return(1);
#ifdef PRESERVEDXF
			io_dxfstoredxf(ni);
#endif
			coord[0] = x1 - xc;		coord[1] = y1 - yc;
			coord[2] = x2 - xc;		coord[3] = y2 - yc;
			coord[4] = x3 - xc;		coord[5] = y3 - yc;
			coord[6] = x4 - xc;		coord[7] = y4 - yc;
			(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)coord,
				VINTEGER|VISARRAY|(8<<VLENGTHSH));
			setvalkey((INTBIG)ni, VNODEINST, io_dxflayerkey, (INTBIG)layer->layerName, VSTRING);
			io_dxfread3dfaces++;
			continue;
		}
		ttyputerr(_("Unknown entity type (%s) at line %ld"), text, io_dxfcurline);
		return(1);
	}
	return(0);
}

INTBIG io_dxfreadarcentity(FILE *io, INTBIG *x, INTBIG *y, INTBIG *z, INTBIG *rad, double *sangle,
	double *eangle, DXFLAYER **retLayer)
{
	CHAR *text;
	INTBIG groupID;

	*retLayer = 0;
	for(;;)
	{
		if (stopping(STOPREASONDXF)) return(1);
		if (io_dxfgetnextpair(io, &groupID, &text) != 0) return(1);
		switch (groupID)
		{
			case 8:  *retLayer = io_dxfgetlayer(text);                        break;
			case 10: *x = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 20: *y = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 30: *z = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 40: *rad = scalefromdispunit((float)eatof(text), io_dxfdispunit); break;
			case 50: *sangle = eatof(text);                                    break;
			case 51: *eangle = eatof(text);                                    break;
		}
		if (groupID == 0)
		{
			io_dxfpushpair(groupID, text);
			break;
		}
	}
	return(0);
}

INTBIG io_dxfreadblock(FILE *io, CHAR **name)
{
	CHAR *text, *saveMsg;
	INTBIG groupID;

	saveMsg = 0;
	for(;;)
	{
		if (stopping(STOPREASONDXF)) return(1);
		if (io_dxfgetnextpair(io, &groupID, &text) != 0) return(1);
		switch (groupID)
		{
			case 2:
				if (saveMsg != 0) (void)reallocstring(&saveMsg, text, el_tempcluster); else
					(void)allocstring(&saveMsg, text, el_tempcluster);
				break;
		}
		if (groupID == 0)
		{
			io_dxfpushpair(groupID, text);
			break;
		}
	}
	*name = saveMsg;
	return(0);
}

INTBIG io_dxfreadcircleentity(FILE *io, INTBIG *x, INTBIG *y, INTBIG *z, INTBIG *rad, DXFLAYER **retLayer)
{
	CHAR *text;
	INTBIG groupID;

	*retLayer = 0;
	for(;;)
	{
		if (stopping(STOPREASONDXF)) return(1);
		if (io_dxfgetnextpair(io, &groupID, &text) != 0) return(1);
		switch (groupID)
		{
			case 8:  *retLayer = io_dxfgetlayer(text);                         break;
			case 10: *x = scalefromdispunit((float)eatof(text), io_dxfdispunit);    break;
			case 20: *y = scalefromdispunit((float)eatof(text), io_dxfdispunit);    break;
			case 30: *z = scalefromdispunit((float)eatof(text), io_dxfdispunit);    break;
			case 40: *rad = scalefromdispunit((float)eatof(text), io_dxfdispunit);  break;
		}
		if (groupID == 0)
		{
			io_dxfpushpair(groupID, text);
			break;
		}
	}
	return(0);
}

INTBIG io_dxfreadinsertentity(FILE *io, INTBIG *x, INTBIG *y, INTBIG *z, CHAR **name, INTBIG *rot,
	float *xsca, float *ysca, INTBIG *xrep, INTBIG *yrep, INTBIG *xspa, INTBIG *yspa, DXFLAYER **retLayer)
{
	CHAR *text, *saveMsg;
	INTBIG groupID;

	*retLayer = 0;
	*rot = 0;
	saveMsg = 0;
	*xrep = *yrep = 1;
	*xspa = *yspa = 0;
	*xsca = *ysca = 1.0;
	for(;;)
	{
		if (stopping(STOPREASONDXF)) return(1);
		if (io_dxfgetnextpair(io, &groupID, &text) != 0) return(1);
		switch (groupID)
		{
			case 8:  *retLayer = io_dxfgetlayer(text);                         break;
			case 10: *x = scalefromdispunit((float)eatof(text), io_dxfdispunit);    break;
			case 20: *y = scalefromdispunit((float)eatof(text), io_dxfdispunit);    break;
			case 30: *z = scalefromdispunit((float)eatof(text), io_dxfdispunit);    break;
			case 50: *rot = eatoi(text);                                        break;
			case 41: *xsca = (float)eatof(text);                                break;
			case 42: *ysca = (float)eatof(text);                                break;
			case 70: *xrep = eatoi(text);                                       break;
			case 71: *yrep = eatoi(text);                                       break;
			case 44: *xspa = scalefromdispunit((float)eatof(text), io_dxfdispunit); break;
			case 45: *yspa = scalefromdispunit((float)eatof(text), io_dxfdispunit); break;
			case 2:
				if (saveMsg != 0) (void)reallocstring(&saveMsg, text, el_tempcluster); else
					(void)allocstring(&saveMsg, text, el_tempcluster);
				break;
		}
		if (groupID == 0)
		{
			io_dxfpushpair(groupID, text);
			break;
		}
	}
	*name = saveMsg;
	return(0);
}

INTBIG io_dxfreadlineentity(FILE *io, INTBIG *x1, INTBIG *y1, INTBIG *z1, INTBIG *x2, INTBIG *y2, INTBIG *z2,
	INTBIG *lineType, DXFLAYER **retLayer)
{
	CHAR *text;
	INTBIG groupID;

	*retLayer = 0;
	*lineType = 0;
	for(;;)
	{
		if (stopping(STOPREASONDXF)) return(1);
		if (io_dxfgetnextpair(io, &groupID, &text) != 0) return(1);
		switch (groupID)
		{
			case 8:  *retLayer = io_dxfgetlayer(text);                         break;
			case 10: *x1 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 20: *y1 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 30: *z1 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 11: *x2 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 21: *y2 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 31: *z2 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
		}
		if (groupID == 0)
		{
			io_dxfpushpair(groupID, text);
			break;
		}
	}
	return(0);
}

INTBIG io_dxfreadpolylineentity(FILE *io, INTBIG **xp, INTBIG **yp, INTBIG **zp, double **bulgep,
	INTBIG *count, INTBIG *closed, INTBIG *lineType, DXFLAYER **retLayer)
{
	CHAR *text;
	INTBIG groupID, vertCount, inEnd, vertLimit, newVertLimit, i;
	INTBIG *x, *y, *z, *newx, *newy, *newz;
	double *bulge, *newbulge;

	*closed = 0;
	*retLayer = 0;
	*lineType = 0;
	inEnd = 0;
	vertCount = vertLimit = 0;
	for(;;)
	{
		if (stopping(STOPREASONDXF)) return(1);
		if (io_dxfgetnextpair(io, &groupID, &text) != 0) break;
		switch(groupID)
		{
			case 8:
				*retLayer = io_dxfgetlayer(text);
				break;

			case 0:
				if (inEnd != 0)
				{
					io_dxfpushpair(groupID, text);

					/* LINTED "x" used in proper order */
					*xp = x;

					/* LINTED "y" used in proper order */
					*yp = y;

					/* LINTED "z" used in proper order */
					*zp = z;

					/* LINTED "bulge" used in proper order */
					*bulgep = bulge;
					*count = vertCount;
					return(0);
				}
				if (estrcmp(text, x_("SEQEND")) == 0)
				{
					inEnd = 1;
					continue;
				}
				if (estrcmp(text, x_("VERTEX")) == 0)
				{
					if (vertCount >= vertLimit)
					{
						newVertLimit = vertCount + 10;
						newx = (INTBIG *)emalloc(newVertLimit * SIZEOFINTBIG, el_tempcluster);
						if (newx == 0) return(1);
						newy = (INTBIG *)emalloc(newVertLimit * SIZEOFINTBIG, el_tempcluster);
						if (newy == 0) return(1);
						newz = (INTBIG *)emalloc(newVertLimit * SIZEOFINTBIG, el_tempcluster);
						if (newz == 0) return(1);
						newbulge = (double *)emalloc(newVertLimit * (sizeof (double)), el_tempcluster);
						if (newbulge == 0) return(1);
						for(i=0; i<vertCount; i++)
						{
							newx[i] = x[i];
							newy[i] = y[i];
							newz[i] = z[i];
							newbulge[i] = bulge[i];
						}
						if (vertLimit > 0)
						{
							efree((CHAR *)x);
							efree((CHAR *)y);
							efree((CHAR *)z);
							efree((CHAR *)bulge);
						}
						x = newx;    y = newy;    z = newz;    bulge = newbulge;
						vertLimit = newVertLimit;
					}
					bulge[vertCount] = 0.0;
					vertCount++;
				}
				break;

			case 10:
				if (vertCount > 0) x[vertCount-1] = scalefromdispunit((float)eatof(text), io_dxfdispunit);
				break;
			case 20:
				if (vertCount > 0) y[vertCount-1] = scalefromdispunit((float)eatof(text), io_dxfdispunit);
				break;
			case 30:
				if (vertCount > 0) z[vertCount-1] = scalefromdispunit((float)eatof(text), io_dxfdispunit);
				break;
			case 42:
				if (vertCount > 0) bulge[vertCount-1] = eatof(text);
				break;
			case 70:
				i = eatoi(text);
				if ((i&1) != 0) *closed = 1;
				break;
		}
	}
	return(1);
}

INTBIG io_dxfreadsolidentity(FILE *io, INTBIG *x1, INTBIG *y1, INTBIG *z1, INTBIG *x2, INTBIG *y2, INTBIG *z2,
	INTBIG *x3, INTBIG *y3, INTBIG *z3, INTBIG *x4, INTBIG *y4, INTBIG *z4, DXFLAYER **retLayer)
{
	CHAR *text;
	INTBIG groupID;
	double factor;

	*retLayer = 0;
	factor = 1.0;
	for(;;)
	{
		if (stopping(STOPREASONDXF)) return(1);
		if (io_dxfgetnextpair(io, &groupID, &text) != 0) return(1);
		switch (groupID)
		{
			case 8:  *retLayer = io_dxfgetlayer(text);                         break;
			case 10: *x1 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 20: *y1 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 30: *z1 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 11: *x2 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 21: *y2 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 31: *z2 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 12: *x3 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 22: *y3 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 32: *z3 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 13: *x4 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 23: *y4 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 33: *z4 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 230:
				factor = eatof(text);
				break;
		}
		if (groupID == 0)
		{
			io_dxfpushpair(groupID, text);
			break;
		}
	}
	*x1 = rounddouble((double)(*x1) * factor);
	*x2 = rounddouble((double)(*x2) * factor);
	*x3 = rounddouble((double)(*x3) * factor);
	*x4 = rounddouble((double)(*x4) * factor);
	return(0);
}

INTBIG io_dxfreadtextentity(FILE *io, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy,
	CHAR **msg, UINTBIG *descript, DXFLAYER **retLayer)
{
	CHAR *text, *saveMsg;
	INTBIG groupID, gotxa, px, py;
	INTBIG x, y, xalign, height;

	*retLayer = 0;
	saveMsg = 0;
	*descript = 0;
	gotxa = 0;
	for(;;)
	{
		if (stopping(STOPREASONDXF)) return(1);
		if (io_dxfgetnextpair(io, &groupID, &text) != 0) return(1);
		switch (groupID)
		{
			case 8:  *retLayer = io_dxfgetlayer(text);                                         break;
			case 10: x = scalefromdispunit((float)eatof(text), io_dxfdispunit);                     break;
			case 20: y = scalefromdispunit((float)eatof(text), io_dxfdispunit);                     break;
			case 40: height = scalefromdispunit((float)eatof(text), io_dxfdispunit);                break;
			case 11: xalign = scalefromdispunit((float)eatof(text), io_dxfdispunit);   gotxa = 1;   break;
			case 1:
				if (saveMsg != 0) (void)reallocstring(&saveMsg, text, el_tempcluster); else
					(void)allocstring(&saveMsg, text, el_tempcluster);
				break;
		}
		if (groupID == 0)
		{
			io_dxfpushpair(groupID, text);
			break;
		}
	}
	*lx = *hx = x;
	*ly = *hy = y;
	if (gotxa != 0)
	{
		*lx = mini(x, xalign);	*hx = *lx + abs(xalign-x) * 2;
		*ly = y;                *hy = y + height;
	} else
	{
		if (*saveMsg != 0)
		{
			screengettextsize(el_curwindowpart, saveMsg, &px, &py);
			*lx = x;	*hx = x + height*px/py;
			*ly = y;	*hy = y + height;
		}
	}
	defaulttextsize(2, descript);
	TDSETPOS(descript, VTPOSBOXED);
	TDSETSIZE(descript, TXTSETPOINTS(TXTMAXPOINTS));
	*msg = saveMsg;
	return(0);
}

INTBIG io_dxfread3dfaceentity(FILE *io, INTBIG *x1, INTBIG *y1, INTBIG *z1, INTBIG *x2, INTBIG *y2,
	INTBIG *z2, INTBIG *x3, INTBIG *y3, INTBIG *z3, INTBIG *x4, INTBIG *y4, INTBIG *z4, DXFLAYER **retLayer)
{
	CHAR *text;
	INTBIG groupID;

	*retLayer = 0;
	for(;;)
	{
		if (stopping(STOPREASONDXF)) return(1);
		if (io_dxfgetnextpair(io, &groupID, &text) != 0) return(1);
		switch (groupID)
		{
			case 8:  *retLayer = io_dxfgetlayer(text);                         break;
			case 10: *x1 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 20: *y1 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 30: *z1 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;

			case 11: *x2 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 21: *y2 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 31: *z2 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;

			case 12: *x3 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 22: *y3 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 32: *z3 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;

			case 13: *x4 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 23: *y4 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
			case 33: *z4 = scalefromdispunit((float)eatof(text), io_dxfdispunit);   break;
		}
		if (groupID == 0)
		{
			io_dxfpushpair(groupID, text);
			break;
		}
	}
	return(0);
}

INTBIG io_dxfignoreentity(FILE *io)
{
	CHAR *text;
	INTBIG groupID;

	for(;;)
	{
		if (stopping(STOPREASONDXF)) return(1);
		if (io_dxfgetnextpair(io, &groupID, &text) != 0) break;
		if (groupID == 0) break;
	}
	io_dxfpushpair(groupID, text);
	return(0);
}

/****************************************** READING SUPPORT ******************************************/

INTBIG io_dxfacceptablelayer(DXFLAYER *layer)
{
	REGISTER INTBIG i;
	INTBIG len;
	REGISTER CHAR **list;

	if (io_dxfreadalllayers != 0) return(1);
	if (layer == 0) return(0);
	for(i=0; i<io_dxfvalidlayercount; i++)
		if (estrcmp(layer->layerName, io_dxfvalidlayernames[i]) == 0) return(1);

	/* add this to the list of layer names that were ignored */
	list = getstringarray(io_dxfignoredlayerstringarray, &len);
	for(i=0; i<len; i++)
		if (estrcmp(layer->layerName, list[i]) == 0) break;
	if (i >= len)
		addtostringarray(io_dxfignoredlayerstringarray, layer->layerName);
	return(0);
}

INTBIG io_dxfextractinsert(NODEPROTO *onp, INTBIG x, INTBIG y, float xsca, float ysca, INTBIG rot, NODEPROTO *np)
{
	INTBIG *newtrace, tx, ty;
	REGISTER INTBIG sx, sy, cx, cy;
	INTBIG i, len;
	NODEINST *ni, *nni;
	double startoffset, endangle;
	REGISTER VARIABLE *var;
	XARRAY trans;

	/* rotate "rot*10" about point [(onp->lowx+onp->highx)/2+x, (onp->lowy+onp->highy)/2+y] */
	makeangle(rot*10, 0, trans);
	trans[2][0] = (onp->lowx+onp->highx)/2+x;
	trans[2][1] = (onp->lowy+onp->highy)/2+y;
	xform(-trans[2][0], -trans[2][1], &trans[2][0], &trans[2][1], trans);

	for(ni = onp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex == 0)
		{
			ttyputmsg(_("Cannot insert block '%s'...it has inserts in it"),
				onp->protoname);
			return(1);
		}
		if (ni->proto == gen_cellcenterprim) continue;
		sx = roundfloat((float)(ni->highx-ni->lowx) * xsca);
		sy = roundfloat((float)(ni->highy-ni->lowy) * ysca);
		cx = x + roundfloat((float)(ni->highx+ni->lowx) * xsca / 2.0f);
		cy = y + roundfloat((float)(ni->highy+ni->lowy) * ysca / 2.0f);
		xform(cx, cy, &tx, &ty, trans);
		tx -= sx/2;   ty -= sy/2;
		nni = newnodeinst(ni->proto, tx, tx+sx, ty, ty+sy, ni->transpose, (ni->rotation+rot*10)%3600, np);
		if (nni == NONODEINST) return(1);
		if (ni->proto == art_closedpolygonprim || ni->proto == art_filledpolygonprim ||
			ni->proto == art_openedpolygonprim || ni->proto == art_openeddashedpolygonprim)
		{
			/* copy trace information */
			var = gettrace(ni);
			if (var != NOVARIABLE)
			{
				len = (var->type&VLENGTH) >> VLENGTHSH;
				newtrace = (INTBIG *)emalloc(len * SIZEOFINTBIG, el_tempcluster);
				if (newtrace == 0) return(1);
				for(i=0; i<len; i++)
				{
					newtrace[i] = (INTBIG)(((INTBIG *)var->addr)[i] * ((i&1) == 0 ? xsca : ysca));
				}
				(void)setvalkey((INTBIG)nni, VNODEINST, el_trace_key, (INTBIG)newtrace, var->type);
				efree((CHAR *)newtrace);
			}
		} else if (ni->proto == gen_invispinprim)
		{
			/* copy text information */
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, art_messagekey);
			if (var != NOVARIABLE)
				var = setvalkey((INTBIG)nni, VNODEINST, art_messagekey, var->addr, var->type);
		} else if (ni->proto == art_circleprim || ni->proto == art_thickcircleprim)
		{
			/* copy arc information */
			getarcdegrees(ni, &startoffset, &endangle);
			setarcdegrees(nni, startoffset, endangle);
		}

		/* copy other information */
		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, io_dxflayerkey);
		if (var != NOVARIABLE)
			setvalkey((INTBIG)nni, VNODEINST, io_dxflayerkey, var->addr, var->type);
		var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, art_colorkey);
		if (var != NOVARIABLE)
			setvalkey((INTBIG)nni, VNODEINST, art_colorkey, var->addr, var->type);
	}
	return(0);
}

NODEPROTO *io_dxfgetscaledcell(NODEPROTO *onp, float xsca, float ysca)
{
	CHAR sviewname[100], fviewname[100], cellname[200];
	INTBIG *newtrace;
	INTBIG i, len;
	VIEW *view;
	NODEINST *ni, *nni;
	NODEPROTO *np;
	double startoffset, endangle;
	REGISTER VARIABLE *var;

	esnprintf(fviewname, 100, x_("scaled%gx%g"), xsca, ysca);
	esnprintf(sviewname, 100, x_("s%gx%g"), xsca, ysca);
	view = getview(fviewname);
	if (view == NOVIEW)
	{
		view = newview(fviewname, sviewname);
		if (view == NOVIEW) return(NONODEPROTO);
	}

	/* find the view of this cell */
	FOR_CELLGROUP(np, onp)
		if (np->cellview == view) return(np);

	/* not found: create it */
	esnprintf(cellname, 200, x_("%s{%s}"), onp->protoname, sviewname);
	np = us_newnodeproto(cellname, onp->lib);
	if (np == NONODEPROTO) return(NONODEPROTO);

	for(ni = onp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex == 0)
		{
			ttyputmsg(_("Cannot scale insert of block '%s'...it has inserts in it"),
				onp->protoname);
			return(NONODEPROTO);
		}
		nni = newnodeinst(ni->proto, (INTBIG)(ni->lowx*xsca), (INTBIG)(ni->highx*xsca),
			(INTBIG)(ni->lowy*ysca), (INTBIG)(ni->highy*ysca), ni->transpose, ni->rotation, np);
		if (nni == NONODEINST) return(NONODEPROTO);
		if (ni->proto == art_closedpolygonprim || ni->proto == art_filledpolygonprim ||
			ni->proto == art_openedpolygonprim || ni->proto == art_openeddashedpolygonprim)
		{
			/* copy trace information */
			var = gettrace(ni);
			if (var != NOVARIABLE)
			{
				len = (var->type&VLENGTH) >> VLENGTHSH;
				newtrace = (INTBIG *)emalloc(len * SIZEOFINTBIG, el_tempcluster);
				if (newtrace == 0) return(NONODEPROTO);
				for(i=0; i<len; i++)
					newtrace[i] = (INTBIG)(((INTBIG *)var->addr)[i] * ((i&1) == 0 ? xsca : ysca));
				(void)setvalkey((INTBIG)nni, VNODEINST, el_trace_key, (INTBIG)newtrace, var->type);
				efree((CHAR *)newtrace);
			}
		} else if (ni->proto == gen_invispinprim)
		{
			/* copy text information */
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, art_messagekey);
			if (var != NOVARIABLE)
				var = setvalkey((INTBIG)nni, VNODEINST, art_messagekey, var->addr, var->type);
		} else if (ni->proto == art_circleprim || ni->proto == art_thickcircleprim)
		{
			/* copy arc information */
			getarcdegrees(ni, &startoffset, &endangle);
			setarcdegrees(nni, startoffset, endangle);
		}

		/* copy layer information */
		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, io_dxflayerkey);
		if (var != NOVARIABLE)
			setvalkey((INTBIG)nni, VNODEINST, io_dxflayerkey, var->addr, var->type);
	}
	return(np);
}

DXFLAYER *io_dxfgetlayer(CHAR *name)
{
	DXFLAYER *layer;

	for(layer = io_dxffirstlayer; layer != 0; layer = layer->next)
		if (estrcmp(name, layer->layerName) == 0) return(layer);

	/* create a new one */
	layer = (DXFLAYER *)emalloc(sizeof (DXFLAYER), io_tool->cluster);
	if (layer == 0) return(0);
	if (allocstring(&layer->layerName, name, io_tool->cluster)) return(0);
	layer->layerColor = -1;
	layer->layerRed = 1.0;
	layer->layerGreen = 1.0;
	layer->layerBlue = 1.0;
	layer->next = io_dxffirstlayer;
	io_dxffirstlayer = layer;
	return(layer);
}

void io_dxfclearlayers(void)
{
	REGISTER DXFLAYER *layer;

	while (io_dxffirstlayer != 0)
	{
		layer = io_dxffirstlayer;
		io_dxffirstlayer = io_dxffirstlayer->next;
		efree((CHAR *)layer->layerName);
		efree((CHAR *)layer);
	}
}

/*
 * Routine to read the next group ID and content pair from the file.
 * Returns nonzero on end-of-file.
 */
INTBIG io_dxfgetnextpair(FILE *io, INTBIG *groupID, CHAR **text)
{
	CHAR *pt, dummyline[MAXCHARSPERLINE];

	if (io_dxfpairvalid != 0)
	{
		*text = io_dxfline;
		*groupID = io_dxfgroupid;
		io_dxfpairvalid = 0;
		return(0);
	}

	for(;;)
	{
		/* read a line and get the group ID */
		if (io_dxfgetnextline(io, io_dxfline, 0) != 0)
		{
			ttyputerr(_("Unexpected end-of-file at line %ld"), io_dxfcurline);
			return(1);
		}
		for(pt = io_dxfline; *pt != 0; pt++)
			if (*pt != ' ' && !isdigit(*pt))
		{
			ttyputerr(_("Invalid group ID on line %ld (%s)"), io_dxfcurline, io_dxfline);
			return(1);
		}
		*groupID = eatoi(io_dxfline);

		/* ignore blank line if file is double-spaced */
		if (io_dxfinputmode == 2) (void)io_dxfgetnextline(io, dummyline, 1);

		/* read a line and get the text */
		if (io_dxfgetnextline(io, io_dxfline, 1) != 0)
		{
			ttyputerr(_("Unexpected end-of-file at line %ld"), io_dxfcurline);
			return(1);
		}
		*text = io_dxfline;

		/* ignore blank line if file is double-spaced */
		if (io_dxfinputmode == 2) (void)io_dxfgetnextline(io, dummyline, 1);

		if (io_dxfinputmode == 0)
		{
			/* see if file is single or double spaced */
			if (io_dxfline[0] != 0) io_dxfinputmode = 1; else
			{
				io_dxfinputmode = 2;
				if (io_dxfgetnextline(io, io_dxfline, 1) != 0)
				{
					ttyputerr(_("Unexpected end-of-file at line %ld"), io_dxfcurline);
					return(1);
				}
				*text = io_dxfline;
				(void)io_dxfgetnextline(io, dummyline, 1);
			}
		}

		/* continue reading if a comment, otherwise quit */
		if (*groupID != 999) break;
	}

	return(0);
}

void io_dxfpushpair(INTBIG groupID, CHAR *text)
{
	io_dxfgroupid = groupID;
	estrcpy(io_dxfline, text);
	io_dxfpairvalid = 1;
}

#ifdef PRESERVEDXF

void io_dxfstartgathering(void)
{
	if (io_dxfpreservestringarray == 0)
	{
		io_dxfpreservestringarray = newstringarray(io_tool->cluster);
		if (io_dxfpreservestringarray == 0) return;
	}
	clearstrings(io_dxfpreservestringarray);
}

void io_dxfstoredxf(NODEINST *ni)
{
	REGISTER CHAR **list;
	INTBIG len;

	if (io_dxfpreservestringarray == 0) return;
	list = getstringarray(io_dxfpreservestringarray, &len);
	if (len == 0) return;
	(void)setval((INTBIG)ni, VNODEINST, x_("IO_dxf_original"), (INTBIG)list,
		VSTRING | VISARRAY | (len << VLENGTHSH));
	clearstrings(io_dxfpreservestringarray);
}
#endif

INTBIG io_dxfgetnextline(FILE *io, CHAR *text, INTBIG canBeBlank)
{
	INTBIG c, len;
	INTBIG curLength;

	for(;;)
	{
		for(len=0; len<MAXCHARSPERLINE; )
		{
			c = xgetc(io);
			if (c == EOF) return(1);
			if (c == '\r') continue;
			if (c == '\n') break;
			text[len++] = (CHAR)c;
		}
		if (len >= MAXCHARSPERLINE)
		{
			ttyputerr(_("Warning: line %ld too long (%d character limit)"),
				io_dxfcurline, MAXCHARSPERLINE);
			return(1);
		}
		text[len] = 0;
		io_dxfcurline++;
		if ((io_dxfcurline % 100) == 0)
		{
			curLength = xtell(io);
			DiaSetProgress(io_dxfprogressdialog, curLength, io_dxflength);
		}
		if (canBeBlank != 0 || *text != 0) break;
	}
#ifdef PRESERVEDXF
	addtostringarray(io_dxfpreservestringarray, text);
#endif
	return(0);
}

/*
 * routine to examine the variable "IO_dxf_layer_names" on the artwork technology and obtain
 * a list of acceptable layer names and numbers (in "io_dxfvalidlayernames" and "io_dxfvalidlayernumbers"
 * that is "io_dxfvalidlayercount" long.  Returns nonzero on error.
 */
INTBIG io_dxfgetacceptablelayers(void)
{
	REGISTER VARIABLE *var;
	INTBIG i, j, k, save;
	CHAR **list, *pt, *start;

	/* get the acceptable DXF layer names */
	var = getval((INTBIG)art_tech, VTECHNOLOGY, VSTRING|VISARRAY, x_("IO_dxf_layer_names"));
	if (var == NOVARIABLE)
	{
		ttyputerr(_("There are no DXF layer names in the %s technology"), art_tech->techname);
		return(1);
	}
	list = (CHAR **)var->addr;

	/* determine number of DXF layers */
	j = getlength(var);
	io_dxfclearacceptablelayers();
	for(i=0; i<j; i++) if (list[i] != 0)
	{
		for(pt = list[i]; *pt != 0; pt++)
			if (*pt == ',') io_dxfvalidlayercount++;
		io_dxfvalidlayercount++;
	}

	/* make space for these values */
	io_dxfvalidlayernames = (CHAR **)emalloc((io_dxfvalidlayercount * sizeof(CHAR *)),
		io_tool->cluster);
	if (io_dxfvalidlayernames == 0) return(1);
	io_dxfvalidlayernumbers = (INTBIG *)emalloc((io_dxfvalidlayercount * SIZEOFINTBIG),
		io_tool->cluster);
	if (io_dxfvalidlayernumbers == 0) return(1);

	/* set DXF layer names */
	k = 0;
	for(i=0; i<j; i++) if (list[i] != 0)
	{
		pt = list[i];
		for(;;)
		{
			start = pt;
			while(*pt != 0 && *pt != ',') pt++;
			save = *pt;
			*pt = 0;
			if (allocstring(&io_dxfvalidlayernames[k], start, io_tool->cluster))
				return(1);
			io_dxfvalidlayernumbers[k] = i;
			k++;
			if (save == 0) break;
			*pt++ = (CHAR)save;
		}
	}
	return(0);
}

void io_dxfclearacceptablelayers(void)
{
	REGISTER INTBIG i;

	for(i=0; i<io_dxfvalidlayercount; i++)
		efree((CHAR *)io_dxfvalidlayernames[i]);
	if (io_dxfvalidlayercount > 0)
	{
		efree((CHAR *)io_dxfvalidlayernames);
		efree((CHAR *)io_dxfvalidlayernumbers);
	}
	io_dxfvalidlayercount = 0;
}

/*
 * Routine to convert a block name "name" into a valid Electric cell name (converts
 * bad characters).
 */
CHAR *io_dxfblockname(CHAR *name)
{
	REGISTER CHAR *pt, chr;
	REGISTER void *infstr;

	infstr = initinfstr();
	for(pt=name; *pt != 0; pt++)
	{
		chr = *pt;
		if (chr == '$' || chr == '{' || chr == '}' || chr == ':') chr = '_';
		addtoinfstr(infstr, chr);
	}
	return(returninfstr(infstr));
}

/*
 * Routine to set the conversion units between DXF files and real distance.
 * The value is stored in the global "io_dxfdispunit".
 */
void io_dxfsetcurunits(void)
{
	REGISTER VARIABLE *var;
	REGISTER INTBIG units;

	var = getval((INTBIG)io_tool, VTOOL, VINTEGER, x_("IO_dxf_units"));
	if (var == NOVARIABLE) units = 2; else
		units = var->addr;
	if (units < 0) units = 0;
	if (units > 6) units = 6;
	switch (units)
	{
		case 0:   io_dxfdispunit = DISPUNITINCH;   break;
		case 1:   io_dxfdispunit = DISPUNITCM;     break;
		case 2:   io_dxfdispunit = DISPUNITMM;     break;
		case 3:   io_dxfdispunit = DISPUNITMIL;    break;
		case 4:   io_dxfdispunit = DISPUNITMIC;    break;
		case 5:   io_dxfdispunit = DISPUNITCMIC;   break;
		case 6:   io_dxfdispunit = DISPUNITNM;     break;
	}
}

/* DXF Options */
static DIALOGITEM io_dxfoptionsdialogitems[] =
{
 /*  1 */ {0, {144,344,168,416}, BUTTON, N_("OK")},
 /*  2 */ {0, {144,248,168,320}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,16,168,231}, SCROLL, x_("")},
 /*  4 */ {0, {8,240,24,320}, MESSAGE, N_("DXF Layer:")},
 /*  5 */ {0, {8,324,24,430}, EDITTEXT, x_("")},
 /*  6 */ {0, {32,240,48,430}, CHECK, N_("Input flattens hierarchy")},
 /*  7 */ {0, {56,240,72,430}, CHECK, N_("Input reads all layers")},
 /*  8 */ {0, {80,240,96,320}, MESSAGE, N_("DXF Scale:")},
 /*  9 */ {0, {80,324,96,430}, POPUP, x_("")}
};
static DIALOG io_dxfoptionsdialog = {{50,75,228,514}, N_("DXF Options"), 0, 9, io_dxfoptionsdialogitems, 0, 0};

/* special items for the "DXF Options" dialog: */
#define DDXO_LAYERLIST      3		/* Layer list (scroll) */
#define DDXO_NEWLAYER       5		/* New layer (edit text) */
#define DDXO_FLATTENHIER    6		/* Input flattens hierarchy (check) */
#define DDXO_READALLLAYERS  7		/* Input reads all layers (check) */
#define DDXO_DXFUNITS       9		/* DXF units (popup) */

void io_dxfoptionsdlog(void)
{
	REGISTER INTBIG i, itemHit, *curstate, nameschanged, units;
	INTBIG newstate[NUMIOSTATEBITWORDS];
	REGISTER CHAR **layernames, *pt;
	CHAR *translated[7];
	REGISTER VARIABLE *dxfvar, *var;
	CHAR *dispunits[] = {N_("Inch"), N_("Centimeter"), N_("Millimeter"), N_("Mil"),
		N_("Micron"), N_("Centimicron"), N_("Nanometer")};
	REGISTER void *infstr, *dia;

	dia = DiaInitDialog(&io_dxfoptionsdialog);
	if (dia == 0) return;
	DiaInitTextDialog(dia, DDXO_LAYERLIST, DiaNullDlogList, DiaNullDlogItem,
		DiaNullDlogDone, 0, SCSELMOUSE|SCREPORT);
	layernames = (CHAR **)emalloc(el_curtech->layercount * (sizeof (CHAR *)),
		el_tempcluster);
	dxfvar = getval((INTBIG)el_curtech, VTECHNOLOGY, VSTRING|VISARRAY,
		x_("IO_dxf_layer_names"));
	for(i=0; i<el_curtech->layercount; i++)
	{
		if (dxfvar == NOVARIABLE) pt = x_(""); else
			pt = ((CHAR **)dxfvar->addr)[i];
		(void)allocstring(&layernames[i], pt, el_tempcluster);
		infstr = initinfstr();
		addstringtoinfstr(infstr, layername(el_curtech, i));
		addstringtoinfstr(infstr, x_(" ("));
		addstringtoinfstr(infstr, pt);
		addstringtoinfstr(infstr, x_(")"));
		DiaStuffLine(dia, DDXO_LAYERLIST, returninfstr(infstr));
	}
	DiaSelectLine(dia, DDXO_LAYERLIST, 0);
	DiaSetText(dia, DDXO_NEWLAYER, layernames[0]);
	curstate = io_getstatebits();
	for(i=0; i<NUMIOSTATEBITWORDS; i++) newstate[i] = curstate[i];
	if ((curstate[0]&DXFFLATTENINPUT) != 0) DiaSetControl(dia, DDXO_FLATTENHIER, 1);
	if ((curstate[0]&DXFALLLAYERS) != 0) DiaSetControl(dia, DDXO_READALLLAYERS, 1);
	for(i=0; i<7; i++) translated[i] = TRANSLATE(dispunits[i]);
	DiaSetPopup(dia, DDXO_DXFUNITS, 7, translated); 
	var = getval((INTBIG)io_tool, VTOOL, VINTEGER, x_("IO_dxf_units"));
	if (var == NOVARIABLE) units = 2; else
		units = var->addr;
	DiaSetPopupEntry(dia, DDXO_DXFUNITS, units);

	/* loop until done */
	nameschanged = 0;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DDXO_LAYERLIST)
		{
			i = DiaGetCurLine(dia, DDXO_LAYERLIST);
			DiaSetText(dia, DDXO_NEWLAYER, layernames[i]);
			continue;
		}
		if (itemHit == DDXO_NEWLAYER)
		{
			i = DiaGetCurLine(dia, DDXO_LAYERLIST);
			pt = DiaGetText(dia, DDXO_NEWLAYER);
			if (estrcmp(pt, layernames[i]) == 0) continue;
			nameschanged++;
			(void)reallocstring(&layernames[i], pt, el_tempcluster);
			infstr = initinfstr();
			addstringtoinfstr(infstr, layername(el_curtech, i));
			addstringtoinfstr(infstr, x_(" ("));
			addstringtoinfstr(infstr, layernames[i]);
			addstringtoinfstr(infstr, x_(")"));
			DiaSetScrollLine(dia, DDXO_LAYERLIST, i, returninfstr(infstr));
			continue;
		}
		if (itemHit == DDXO_FLATTENHIER || itemHit == DDXO_READALLLAYERS)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		if (nameschanged != 0)
			setval((INTBIG)el_curtech, VTECHNOLOGY, x_("IO_dxf_layer_names"),
				(INTBIG)layernames, VSTRING|VISARRAY|
					(el_curtech->layercount<<VLENGTHSH));
		if (DiaGetControl(dia, DDXO_FLATTENHIER) != 0) newstate[0] |= DXFFLATTENINPUT; else
			newstate[0] &= ~DXFFLATTENINPUT;
		if (DiaGetControl(dia, DDXO_READALLLAYERS) != 0) newstate[0] |= DXFALLLAYERS; else
			newstate[0] &= ~DXFALLLAYERS;
		for(i=0; i<NUMIOSTATEBITWORDS; i++) if (curstate[i] != newstate[i]) break;
		if (i < NUMIOSTATEBITWORDS) io_setstatebits(newstate);
		i = DiaGetPopupEntry(dia, DDXO_DXFUNITS);
		if (i != units)
			setval((INTBIG)io_tool, VTOOL, x_("IO_dxf_units"), i, VINTEGER);
	}
	for(i=0; i<el_curtech->layercount; i++) efree(layernames[i]);
	efree((CHAR *)layernames);
	DiaDoneDialog(dia);
}
