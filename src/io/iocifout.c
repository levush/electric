/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iocifout.c
 * Input/output tool: CIF output
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
#include "eio.h"
#include "egraphics.h"
#include "usr.h"
#include "tecgen.h"
#include "edialogs.h"
#include "efunction.h"

#define FULLPOLYMERGE 1		/* uncomment to use older manhattan merging */

#define NMCHR	80

static INTBIG     io_prev_chr_sep, io_nchar;	/* for CRC checksum */
static UINTBIG    io_check_sum;					/* for CRC checksum */
static UINTBIG    io_cifcrctab[256];			/* for CRC checksum */
static INTBIG     io_cif_resolution;			/* for resolution tests */
static INTBIG     io_cif_polypoints;			/* for points on a polygon tests */
static INTBIG    *io_cifoutputstate;			/* current output state */
static INTBIG     io_cifoutoffx;				/* offset for current cell - for highlight */
static INTBIG     io_cifoutoffy;				/* offset for current cell - for highlight */
static NODEPROTO *io_cifoutcurnp;				/* node proto of cell being written */
static BOOLEAN    io_zero_offset;				/* Set if there is no centering */
static BOOLEAN    io_cif_highlightreserr;		/* Set to highlight resolution errors */
static INTBIG     io_reserr_count;				/* count the resolution errors */
static CHAR       io_cifoutcurlayer[200];		/* current layer name written out */
static INTBIG     io_cifoutcellnumber;			/* count of cells being written out */
static INTBIG     io_cifoutjobsize;				/* number of cells to write out */

/* layer requests for grouping output layers */
#define NOLAYERREQ ((LAYERREQ *)-1)

typedef struct Ilayerreq
{
	TECHNOLOGY *tech;
	INTBIG      layer;
	CHAR       *layername;
	struct Ilayerreq *nextlayerreq;
} LAYERREQ;

static LAYERREQ  *io_ciflayerreq = NOLAYERREQ;
static LAYERREQ  *io_ciflayerreqfree = NOLAYERREQ;
static LAYERREQ **io_ciflayerreqlist;
static INTBIG     io_ciflayerreqlisttotal = 0;
static void      *io_cifmerge;			/* for merging geometry */

/* prototypes for local routines */
static void io_cifwritecell(NODEPROTO*, BOOLEAN, void*);
static void io_outputcifpoly(TECHNOLOGY*, POLYGON*, INTBIG, INTBIG, INTBIG, INTBIG, XARRAY, GEOM*);
static void io_cifprintf(CHAR *msg, ...);
static void io_cifreserror(GEOM*, INTBIG, INTBIG, INTBIG, INTBIG, XARRAY);
static void io_cif_write_polygon(INTBIG, TECHNOLOGY*, INTBIG*, INTBIG*, INTBIG);
static void io_cif_show_reserror(INTBIG, INTBIG, INTBIG, INTBIG);
static INTBIG io_cif_outscale(INTBIG value);
static LAYERREQ *io_alloclayerreq(void);
static void io_freelayerreq(LAYERREQ *req);
static void io_includeciflayer(TECHNOLOGY *tech, INTBIG layer);
static void io_writeciflayer(CHAR *layername);
static void io_cifoptionsdlog(void);
static INTBIG io_ciffindhighestlayer(NODEPROTO *np, INTBIG x, INTBIG y);
static int io_ciflayerreqascending(const void *e1, const void *e2);
static CHAR *io_cif_cellname(NODEPROTO *cell);

/*
 * Routine to free all memory associated with this module.
 */
void io_freecifoutmemory(void)
{
	REGISTER LAYERREQ *req;

	while (io_ciflayerreq != NOLAYERREQ)
	{
		req = io_ciflayerreq;
		io_ciflayerreq = req->nextlayerreq;
		io_freelayerreq(req);
	}
	while (io_ciflayerreqfree != NOLAYERREQ)
	{
		req = io_ciflayerreqfree;
		io_ciflayerreqfree = req->nextlayerreq;
		efree((CHAR *)req);
	}
	if (io_ciflayerreqlisttotal > 0) efree((CHAR *)io_ciflayerreqlist);
}

/*
 * Routine to initialize CIF I/O.
 */
void io_initcif(void)
{
	extern COMCOMP io_cifp;

	DiaDeclareHook(x_("cifopt"), &io_cifp, io_cifoptionsdlog);
}

BOOLEAN io_writeciflibrary(LIBRARY *lib)
{
	CHAR file[100], *truename;
	REGISTER CHAR *name;
	REGISTER NODEPROTO *np;
	REGISTER VARIABLE *var;
	REGISTER TECHNOLOGY *tech;
	REGISTER LIBRARY *olib;
	REGISTER UINTBIG bytesread, i, j;
	static UINTBIG crcrow[8] = {0x04C11DB7, 0x09823B6E, 0x130476DC, 0x2608EDB8,
		0x4C11DB70, 0x9823B6E0, 0x34867077, 0x690CE0EE};
	REGISTER void *dia;

	/* create the proper disk file for the CIF */
	if (lib->curnodeproto == NONODEPROTO)
	{
		ttyputerr(_("Must be editing a cell to generate CIF output"));
		return(TRUE);
	}
	(void)estrcpy(file, lib->curnodeproto->protoname);
	if (estrcmp(&file[estrlen(file)-4],x_(".cif")) != 0) (void)estrcat(file, x_(".cif"));
	name = truepath(file);
	io_fileout = xcreate(name, io_filetypecif, _("CIF File"), &truename);
	if (io_fileout == NULL)
	{
		if (truename != 0) ttyputerr(_("Cannot write %s"), truename);
		return(TRUE);
	}

	/* write header information */
	if ((us_useroptions&NODATEORVERSION) == 0)
	{
		xprintf(io_fileout, _("( Electric VLSI Design System, version %s );\n"), el_version);
		xprintf(io_fileout, x_("( %s );\n\n"), timetostring(getcurrenttime()));
	} else
	{
		xprintf(io_fileout, _("( Electric VLSI Design System );\n"));
	}
	us_emitcopyright(io_fileout, x_("( "), x_(" );"));

	/* get current output state */
	io_cifoutputstate = io_getstatebits();

	/* get form of output */
	if ((io_cifoutputstate[0]&CIFOUTNORMALIZE) != 0) io_zero_offset = FALSE; else
		io_zero_offset = TRUE;
	if ((io_cifoutputstate[1]&CIFRESHIGH) != 0) io_cif_highlightreserr = TRUE; else
		io_cif_highlightreserr = FALSE;

	/* get cif resolution */
	var = getval((INTBIG)el_curtech, VTECHNOLOGY, VINTEGER, x_("IO_cif_resolution"));
	if (var == NOVARIABLE) io_cif_resolution = 0; else
	{
		io_cif_resolution = var->addr;
		if (io_cif_resolution != 0 && io_cif_highlightreserr)
			initerrorlogging(x_("CIF Resolution"));
	}

	/* get the number of allowable points on a polygon */
	var = getval((INTBIG)el_curtech, VTECHNOLOGY, VINTEGER, x_("IO_cif_polypoints"));
	if (var == NOVARIABLE) io_cif_polypoints = MAXINTBIG; else
		io_cif_polypoints = var->addr;

	/* initialize cif merging facility if needed */
#ifndef FULLPOLYMERGE
	if ((io_cifoutputstate[0]&CIFOUTMERGE) != 0) mrginit();
#endif

	/* initialize the CRC checksum accumulation */
	for(i=0; i<256; i++)
	{
		io_cifcrctab[i] = 0;
		for(j=0; j<8; j++)
			if (((1 << j) & i) != 0)
				io_cifcrctab[i] = io_cifcrctab[i] ^ crcrow[j];
		io_cifcrctab[i] &= 0xFFFFFFFF;
	}
	io_nchar = 1;
	io_prev_chr_sep = 1;
	io_check_sum = io_cifcrctab[' '];

	/* initialize cache of CIF layer information */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		var = getval((INTBIG)tech, VTECHNOLOGY, VSTRING|VISARRAY, x_("IO_cif_layer_names"));
		if (var == NOVARIABLE) tech->temp1 = 0; else
		{
			tech->temp1 = var->addr;
			if (getlength(var) != tech->layercount)
			{
				ttyputerr(_("Warning: CIF layer information is bad for technology %s.  Use 'CIF Options' to fix it"),
					tech->techname);
				tech->temp1 = 0;
			}
		}

		var = getval((INTBIG)tech, VTECHNOLOGY, VSTRING|VISARRAY, x_("TECH_layer_names"));
		tech->temp2 = (var == NOVARIABLE ? 0 : var->addr);

		/* if ignoring DRC mask layer, delete Generic technology layer names */
		if (tech == gen_tech && (io_cifoutputstate[0]&CIFOUTADDDRC) == 0) tech->temp1 = 0;
	}

	/* figure out how many cells will be written */
	io_cifoutcellnumber = 0;
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = 0;
	io_cifwritecell(lib->curnodeproto, TRUE, 0);
	io_cifoutjobsize = io_cifoutcellnumber;
	dia = DiaInitProgress(_("Writing CIF..."), 0);
	if (dia == 0) return(TRUE);
	DiaSetProgress(dia, 0, io_cifoutjobsize);

	/* write the CIF */
	io_cifoutcellnumber = 0;
	io_reserr_count = 0;
	io_cifbase = 100;
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = 0;
	io_cifwritecell(lib->curnodeproto, FALSE, dia);

	/* clean up */
	if ((io_cifoutputstate[0]&CIFOUTNOTOPCALL) == 0)
		io_cifprintf(x_("C %ld;\n"), io_cifbase);
	io_cifprintf(x_("E\n"));
	xclose(io_fileout);
	DiaDoneProgress(dia);

#ifndef FULLPOLYMERGE
	/* if doing polygon output */
	if ((io_cifoutputstate[0]&CIFOUTMERGE) != 0) mrgterm();
#endif

	/* tell the user that the file is written */
	ttyputmsg(_("%s written"), truename);

	/* complete the checksum accumulation */
	if (!io_prev_chr_sep)
	{
		io_check_sum = (io_check_sum << 8) ^
			io_cifcrctab[((io_check_sum >> 24) ^ (UCHAR1)' ') & 0xFF];
		io_nchar++;
	}
	bytesread = io_nchar;
	while (bytesread > 0)
	{
		io_check_sum = (io_check_sum << 8) ^
			io_cifcrctab[((io_check_sum >> 24) ^ bytesread) & 0xFF];
		bytesread >>= 8;
	}
	io_check_sum = ~io_check_sum & 0xFFFFFFFF;
	ttyputmsg(_("(MOSIS CRC: %lu %ld)"), io_check_sum, io_nchar);

	/* complete error accumulation */
	if (io_cif_resolution != 0)
	{
		if (io_cif_highlightreserr)
		{
			io_reserr_count = numerrors();
			termerrorlogging(TRUE);
		}
		if (io_reserr_count != 0)
			ttyputerr(_("WARNING: Found %ld resolution %s"), io_reserr_count,
				makeplural(_("error"), io_reserr_count));
	}

	return(FALSE);
}

void io_cifwritecell(NODEPROTO *np, BOOLEAN fake, void *dia)
{
	REGISTER NODEINST *subni;
	REGISTER ARCINST *ai;
	REGISTER NODEPROTO *subnt, *onp;
	REGISTER PORTPROTO *pp, *subpp;
	REGISTER PORTARCINST *pi;
	REGISTER INTBIG i, j, total, validport, toplayer, toplayerfound, offx, offy, fun;
	REGISTER CHAR *type;
	REGISTER LAYERREQ *req;
	REGISTER VARIABLE *var;
	XARRAY trans, localtran, temp2, subrot, subt, subr, submat;
	INTBIG rx, ry, xpos, ypos, bx, by, subcornerx, subcornery;
	static POLYGON *poly = NOPOLYGON;
	REGISTER void *infstr;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, io_tool->cluster);

	io_cifoutcellnumber++;
	if (!fake)
	{
		/* stop if requested */
		if (stopping(STOPREASONCIF)) return;

		/* show progress */
		infstr = initinfstr();
		formatinfstr(infstr, _("Writing %s"), describenodeproto(np));
		DiaSetTextProgress(dia, returninfstr(infstr));
		DiaSetProgress(dia, io_cifoutcellnumber, io_cifoutjobsize);
	}

	/* if there are any sub-cells that have not been written, write them */
	for(subni = np->firstnodeinst; subni != NONODEINST; subni = subni->nextnodeinst)
	{
		subnt = subni->proto;
		if (subnt->primindex != 0) continue;

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(subnt, np)) continue;

		if ((subni->userbits & NEXPAND) == 0 && (io_cifoutputstate[0]&CIFOUTEXACT) != 0) continue;

		/* convert body cells to contents cells */
		onp = contentsview(subnt);
		if (onp != NONODEPROTO) subnt = onp;

		/* don't recurse if this cell has already been written */
		if (subnt->temp1 != 0) continue;

		/* recurse to the bottom */
		io_cifwritecell(subnt, fake, dia);
	}

	np->temp1 = ++io_cifbase;
	if (fake) return;

	io_cifoutcurnp = np;			/* set global for highlighting */

	/* prepare to write the cell */
	if (io_zero_offset)
	{
		grabpoint(np, &io_cifoutoffx, &io_cifoutoffy);
		offx = io_cifoutoffx;
		offy = io_cifoutoffy;
	} else
	{
		offx = io_cifoutoffx = (np->lowx + np->highx) / 2;
		offy = io_cifoutoffy = (np->lowy + np->highy) / 2;
	}
	io_cifprintf(x_("DS %ld 1 1;\n"), io_cifbase);
	io_cifprintf(x_("9 %s; (from library %s);\n"), io_cif_cellname(np), np->lib->libname);

	io_cifoutcurlayer[0] = 0;
	if ((io_cifoutputstate[0]&CIFOUTMERGE) == 0)
	{
		/* no polygon merging: sort by layer */

		/* determine which layers exist in this cell */
		while (io_ciflayerreq != NOLAYERREQ)
		{
			req = io_ciflayerreq;
			io_ciflayerreq = req->nextlayerreq;
			io_freelayerreq(req);
		}
		for(subni = np->firstnodeinst; subni != NONODEINST; subni = subni->nextnodeinst)
		{
			if ((subni->userbits&WIPED) != 0) continue;
			subnt = subni->proto;
			if (subnt->primindex == 0) continue;
			i = nodepolys(subni, 0, NOWINDOWPART);
			for(j=0; j<i; j++)
			{
				shapenodepoly(subni, j, poly);
				io_includeciflayer(subnt->tech, poly->layer);
			}
		}
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		{
			i = arcpolys(ai, NOWINDOWPART);
			for(j=0; j<i; j++)
			{
				shapearcpoly(ai, j, poly);
				io_includeciflayer(ai->proto->tech, poly->layer);
			}
		}

		/* sort the requests by layer name */
		total = 0;
		for(req = io_ciflayerreq; req != NOLAYERREQ; req = req->nextlayerreq)
			total++;
		if (total > io_ciflayerreqlisttotal)
		{
			if (io_ciflayerreqlisttotal > 0)
				efree((CHAR *)io_ciflayerreqlist);
			io_ciflayerreqlisttotal = 0;
			io_ciflayerreqlist = (LAYERREQ **)emalloc(total * (sizeof (LAYERREQ *)),
				io_tool->cluster);
			if (io_ciflayerreqlist == 0) return;
			io_ciflayerreqlisttotal = total;
		}
		total = 0;
		for(req = io_ciflayerreq; req != NOLAYERREQ; req = req->nextlayerreq)
			io_ciflayerreqlist[total++] = req;
		esort(io_ciflayerreqlist, total, sizeof (LAYERREQ *), io_ciflayerreqascending);
		for(i=0; i<total-1; i++)
			io_ciflayerreqlist[i]->nextlayerreq = io_ciflayerreqlist[i+1];
		if (total == 0) io_ciflayerreq = NOLAYERREQ; else
		{
			io_ciflayerreq = io_ciflayerreqlist[0];
			io_ciflayerreqlist[total-1]->nextlayerreq = NOLAYERREQ;
		}

		/* now write geometry by layer */
		for(req = io_ciflayerreq; req != NOLAYERREQ; req = req->nextlayerreq)
		{
			if (req->layername == 0 || *req->layername == 0) continue;

			/* write all primitive nodes in the cell */
			for(subni = np->firstnodeinst; subni != NONODEINST; subni = subni->nextnodeinst)
			{
				/* don't draw anything if the node is wiped out */
				if ((subni->userbits&WIPED) != 0) continue;

				subnt = subni->proto;
				if (subnt->primindex == 0) continue;
				if (subnt->tech != req->tech) continue;
				i = subni->rotation;
				if (subni->transpose != 0) i = (i + 900) % 3600;
				rx = ((cosine(i)>>14) * 100) >> 16;
				ry = ((sine(i)>>14) * 100) >> 16;
				makerot(subni, trans);

				/* write a primitive nodeinst */
				i = nodepolys(subni, 0, NOWINDOWPART);
				for(j=0; j<i; j++)
				{
					shapenodepoly(subni, j, poly);
					if (poly->layer != req->layer) continue;
					io_outputcifpoly(subnt->tech, poly, rx,
						(subni->transpose ? -ry : ry), offx, offy, trans, subni->geom);
				}
			}

			/* write all of the arcs in the cell */
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			{
				if (ai->proto->tech != req->tech) continue;
				i = arcpolys(ai, NOWINDOWPART);
				for(j=0; j<i; j++)
				{
					/* write the box describing the layer */
					shapearcpoly(ai, j, poly);
					if (poly->layer != req->layer) continue;
					io_outputcifpoly(ai->proto->tech, poly, 1, 0, offx, offy, el_matid, ai->geom);
				}
			}
		}
	} else
	{
#ifdef FULLPOLYMERGE
		io_cifmerge = mergenew(io_tool->cluster);
#endif

		/* doing polygon merging: just dump the geometry into the merging system */
		for(subni = np->firstnodeinst; subni != NONODEINST; subni = subni->nextnodeinst)
		{
			/* don't draw anything if the node is wiped out */
			if ((subni->userbits&WIPED) != 0) continue;

			subnt = subni->proto;
			if (subnt->primindex == 0) continue;
			i = subni->rotation;
			if (subni->transpose != 0) i = (i + 900) % 3600;
			rx = ((cosine(i)>>14) * 100) >> 16;
			ry = ((sine(i)>>14) * 100) >> 16;
			makerot(subni, trans);

			/* write a primitive nodeinst */
			i = nodepolys(subni, 0, NOWINDOWPART);
			for(j=0; j<i; j++)
			{
				shapenodepoly(subni, j, poly);
				io_outputcifpoly(subnt->tech, poly, rx,
					(subni->transpose ? -ry : ry), offx, offy, trans, subni->geom);
			}
		}

		/* write all of the arcs in the cell */
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		{
			i = arcpolys(ai, NOWINDOWPART);
			for(j=0; j<i; j++)
			{
				/* write the box describing the layer */
				shapearcpoly(ai, j, poly);
				io_outputcifpoly(ai->proto->tech, poly, 1, 0, offx, offy, el_matid, ai->geom);
			}
		}

#ifdef FULLPOLYMERGE
		mergeextract(io_cifmerge, io_cif_write_polygon);
		mergedelete(io_cifmerge);
#else
		mrgdonecell(io_cif_write_polygon);
#endif
	}

	/* write all cell instances */
	for(subni = np->firstnodeinst; subni != NONODEINST; subni = subni->nextnodeinst)
	{
		subnt = subni->proto;
		if (subnt->primindex != 0) continue;

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(subnt, np)) continue;

		i = subni->rotation;
		if (subni->transpose != 0) i = (i + 900) % 3600;
		rx = ((cosine(i)>>14) * 100) >> 16;
		ry = ((sine(i)>>14) * 100) >> 16;
		makerot(subni, trans);

		/* determine offset of cell */
		if (io_zero_offset)
		{
			grabpoint(subni->proto, &bx, &by);
			maketrans(subni, subt);
			makerot(subni, subr);
			transmult(subt, subr, submat);
			xform(bx, by, &subcornerx, &subcornery, submat);
			xpos = (subcornerx - offx);
			ypos = (subcornery - offy);
		} else
		{
			xpos = (subni->lowx + subni->highx)/2 - offx;
			ypos = (subni->lowy + subni->highy)/2 - offy;
		}
		if ((subni->userbits&NEXPAND) != 0 || (io_cifoutputstate[0]&CIFOUTEXACT) == 0)
		{
			/* check resolution of call */
			if (io_cif_resolution != 0)
			{
				if ((xpos%io_cif_resolution) != 0 || (ypos%io_cif_resolution) != 0)
				{
					io_cifprintf(_("(Call to symbol %ld does not resolve to grid);\n"),
						subnt->temp1);
					io_reserr_count++;
				}
			}

			/* write a call to a cell */
			io_cifprintf(x_("C %ld R %ld %ld"), subnt->temp1, rx, ry);
			if (subni->transpose != 0) io_cifprintf(x_(" M Y"));
			io_cifprintf(x_(" T %ld %ld;\n"), io_cif_outscale(xpos), io_cif_outscale(ypos));
		} else
		{
			/* write the vectors that describe an unexpanded cell */
			io_cifprintf(x_("0V %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld;\n"),
				io_cif_outscale(subni->lowx-offx), io_cif_outscale(subni->lowy-offy),
					io_cif_outscale(subni->lowx-offx), io_cif_outscale(subni->highy-offy),
						io_cif_outscale(subni->highx-offx), io_cif_outscale(subni->highy-offy),
							io_cif_outscale(subni->highx-offx), io_cif_outscale(subni->lowy-offy),
								io_cif_outscale(subni->lowx-offx), io_cif_outscale(subni->lowy-offy));
			io_cifprintf(x_("2C \"%s\" T %ld %ld;\n"), describenodeproto(subnt),
				io_cif_outscale(xpos), io_cif_outscale(ypos));
		}
	}

	/* write the ports as labels */
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		/* get the coordinate of the port label */
		subni = pp->subnodeinst;   subpp = pp->subportproto;
		portposition(subni, subpp, &xpos, &ypos);

		/* find the primitive node on this port */
		makerot(subni, subrot);
		while (subni->proto->primindex == 0)
		{
			maketrans(subni, localtran);
			transmult(localtran, subrot, temp2);
			subni = subpp->subnodeinst;
			subpp = subpp->subportproto;
			makerot(subni, localtran);
			transmult(localtran, temp2, subrot);
		}

		/* get highest layer at this point */
		toplayer = io_ciffindhighestlayer(np, xpos, ypos);
		toplayerfound = 0;

		/* find valid layers on this node that surround the port */
		validport = 0;
		total = nodepolys(subni, 0, NOWINDOWPART);
		for(i=0; i<total; i++)
		{
			shapenodepoly(subni, i, poly);
			if (poly->layer < 0) continue;
			fun = layerfunction(poly->tech, poly->layer);
			if ((fun&LFPSEUDO) != 0) continue;
			xformpoly(poly, subrot);
			if (!isinside(xpos-1, ypos, poly)) continue;
			if (!isinside(xpos+1, ypos, poly)) continue;
			if (!isinside(xpos, ypos-1, poly)) continue;
			if (!isinside(xpos, ypos+1, poly)) continue;
			if (poly->layer == toplayer) toplayerfound = 1;
			validport = 1;
			break;
		}
		if (validport == 0)
		{
			/* look for connected arcs */
			subpp = pp;
			transid(subrot);
			for(;;)
			{
				/* look for layers at this level of hierarchy */
				for(pi = subpp->subnodeinst->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				{
					ai = pi->conarcinst;
					total = arcpolys(ai, NOWINDOWPART);
					for(i=0; i<total; i++)
					{
						shapearcpoly(ai, i, poly);
						if (poly->layer < 0) continue;
						fun = layerfunction(poly->tech, poly->layer);
						if ((fun&LFPSEUDO) != 0) continue;
						xformpoly(poly, subrot);
						if (!isinside(xpos-1, ypos, poly)) continue;
						if (!isinside(xpos+1, ypos, poly)) continue;
						if (!isinside(xpos, ypos-1, poly)) continue;
						if (!isinside(xpos, ypos+1, poly)) continue;
						if (poly->layer == toplayer) toplayerfound = 1;
						validport = 1;
						break;
					}
					if (validport != 0) break;
				}
				if (validport != 0) break;

				/* not found on this level: descend and look again */
				if (subpp->subnodeinst->proto->primindex != 0) break;
				maketrans(subpp->subnodeinst, localtran);
				transmult(localtran, subrot, temp2);
				makerot(subpp->subnodeinst, localtran);
				transmult(localtran, temp2, subrot);
				subpp = subpp->subportproto;
			}
		}
		if (validport != 0)
		{
			if (toplayerfound == 0)
				ttyputmsg(_("Warning: cell %s, export '%s' obscured by higher layer"),
					describenodeproto(np), pp->protoname);
			var = getval((INTBIG)pp, VPORTPROTO, VSTRING, x_("EXPORT_reference_name"));
			type = describeportbits(pp->userbits);
			if (((pp->userbits&STATEBITS) == REFOUTPORT || (pp->userbits&STATEBITS) == REFINPORT ||
				(pp->userbits&STATEBITS) == REFBASEPORT) && var != NOVARIABLE)
			{
				io_cifprintf("94 %s %ld %ld type=%s reference=%s;\n", pp->protoname,
					io_cif_outscale(xpos-offx), io_cif_outscale(ypos-offy), type, (CHAR *)var->addr);
			} else
			{
				io_cifprintf("94 %s %ld %ld type=%s;\n", pp->protoname, io_cif_outscale(xpos-offx),
					io_cif_outscale(ypos-offy), type);
			}
		}
	}

	io_cifprintf(x_("DF;\n"));
}

int io_ciflayerreqascending(const void *e1, const void *e2)
{
	REGISTER LAYERREQ *lr1, *lr2;
	REGISTER CHAR *lrname1, *lrname2;

	lr1 = *((LAYERREQ **)e1);
	lr2 = *((LAYERREQ **)e2);
	lrname1 = lr1->layername;
	if (lrname1 == 0) lrname1 = x_("");
	lrname2 = lr2->layername;
	if (lrname2 == 0) lrname2 = x_("");
	return(namesame(lrname1, lrname2));
}

INTBIG io_ciffindhighestlayer(NODEPROTO *np, INTBIG x, INTBIG y)
{
	REGISTER INTBIG sea, i, tot, first, bestlayer;
	float height, thickness, bestheight;
	REGISTER GEOM *geom;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	static POLYGON *poly = NOPOLYGON;

	(void)needstaticpolygon(&poly, 4, io_tool->cluster);
	sea = initsearch(x, x, y, y, np);
	first = 1;
	for(;;)
	{
		geom = nextobject(sea);
		if (geom == NOGEOM) break;
		if (geom->entryisnode)
		{
			ni = geom->entryaddr.ni;
			if (ni->proto->primindex == 0) continue;
			tot = nodepolys(ni, 0, NOWINDOWPART);
			for(i=0; i<tot; i++)
			{
				shapenodepoly(ni, i, poly);
				if (!isinside(x, y, poly)) continue;
				if (get3dfactors(poly->tech, poly->layer, &height, &thickness))
					continue;

				/* LINTED "bestheight" used in proper order */
				if (first != 0 || height > bestheight)
				{
					bestheight = height;
					bestlayer = poly->layer;
					first = 0;
				}
			}
		} else
		{
			ai = geom->entryaddr.ai;
			tot = arcpolys(ai, NOWINDOWPART);
			for(i=0; i<tot; i++)
			{
				shapearcpoly(ai, i, poly);
				if (!isinside(x, y, poly)) continue;
				if (get3dfactors(poly->tech, poly->layer, &height, &thickness))
					continue;
				if (first != 0 || height > bestheight)
				{
					bestheight = height;
					bestlayer = poly->layer;
					first = 0;
				}
			}
		}
	}
	if (first != 0) return(0);
	return(bestlayer);
}

/*
 * Routine to dump the layer name (if it has changed).
 */
void io_writeciflayer(CHAR *layername)
{
	if (estrcmp(layername, io_cifoutcurlayer) == 0) return;
	io_cifprintf(x_("L %s;\n"), layername);
	estrcpy(io_cifoutcurlayer, layername);
}

/*
 * routine to write polygon "poly" to the CIF file.  The polygon is from
 * technology "tech", is rotated by the factor "rx" and "ry", and is offset
 * by "offx" and "offy"
 */
void io_outputcifpoly(TECHNOLOGY *tech, POLYGON *poly, INTBIG rx, INTBIG ry,
	INTBIG offx, INTBIG offy, XARRAY trans, GEOM *geom)
{
	REGISTER INTBIG r, k, bloat;
	INTBIG xl, xh, yl, yh, xpos, ypos;
	REGISTER CHAR *layername;
	REGISTER void *infstr;

	/* get bloating for this layer */
	if (poly->layer < 0 || tech->temp1 == 0) return;
	infstr = initinfstr();
	addstringtoinfstr(infstr, tech->techname);
	addtoinfstr(infstr, ':');
	addstringtoinfstr(infstr, ((CHAR **)tech->temp2)[poly->layer]);
	bloat = io_getoutputbloat(returninfstr(infstr));

	/* get the CIF layer, stop if none */
	layername = ((CHAR **)tech->temp1)[poly->layer];
	if (*layername == 0) return;

	switch (poly->style)
	{
		case DISC:
			xformpoly(poly, trans);
			r = computedistance(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1]);
			if (r <= 0) break;

			/* write the layer name */
			io_writeciflayer(layername);

			/* write the round-flash */
			io_cifprintf(x_(" R %ld %ld %ld;\n"), r+bloat, io_cif_outscale(poly->xv[0]-offx),
				io_cif_outscale(poly->yv[0]-offy));
			break;

		default:
#ifdef FULLPOLYMERGE
			if ((io_cifoutputstate[0]&CIFOUTMERGE) != 0)
			{
				xformpoly(poly, trans);
				for(k=0; k<poly->count; k++)
				{
					poly->xv[k] -= offx;
					poly->yv[k] -= offy;
				}
				mergeaddpolygon(io_cifmerge, poly->layer, tech, poly);
				break;
			}
#endif

			if (isbox(poly, &xl,&xh, &yl,&yh))
			{
				/* ignore zero-size polygons */
				if (xh == xl || yh == yl) return;

#ifndef FULLPOLYMERGE
				if ((io_cifoutputstate[0]&CIFOUTMERGE) != 0)
				{
					/* do polygon format output */
					xform((xl+xh)/2, (yl+yh)/2, &xpos, &ypos, trans);
					if (rx != 0 && ry == 0)
					{
						/* no rotation needed */
						mrgstorebox(poly->layer, tech, xh-xl+bloat, yh-yl+bloat,
							xpos-offx, ypos-offy);
						return;
					}
					if (rx == 0 && ry != 0)
					{
						/* rotate through 90 degrees */
						mrgstorebox(poly->layer, tech, yh-yl+bloat, xh-xl+bloat,
							xpos-offx, ypos-offy);
						return;
					}
					/* nonmanhattan or worse .. fall into direct output case */
				}
#endif

				/* write the layer name */
				io_writeciflayer(layername);

				/* non-merged box: highlight resolution errors */
				io_cifreserror(geom, xh-xl+bloat, yh-yl+bloat, (xl+xh)/2, (yl+yh)/2, trans);

				/* want individual box output */
				xform((xl+xh)/2, (yl+yh)/2, &xpos, &ypos, trans);
				io_cifprintf(x_(" B %ld %ld %ld %ld"), io_cif_outscale(xh-xl+bloat),
					io_cif_outscale(yh-yl+bloat), io_cif_outscale(xpos-offx),
						io_cif_outscale(ypos-offy));
				if (rx <= 0 || ry != 0) io_cifprintf(x_(" %ld %ld"), rx, ry);
				io_cifprintf(x_(";\n"));
			} else
			{
				/* write the layer name */
				io_writeciflayer(layername);

				xformpoly(poly, trans);
				if (bloat != 0)
				{
					ttyputmsg(_("Warning: complex CIF polygon cannot be bloated"));
					io_cifprintf(_("(NEXT POLYGON CANNOT BE BLOATED);\n"));
				}
				if (poly->count == 1)
					io_cifprintf(x_(" 0V %ld %ld %ld %ld;\n"), io_cif_outscale(poly->xv[0]-offx),
						io_cif_outscale(poly->yv[0]-offy), io_cif_outscale(poly->xv[0]-offx),
							io_cif_outscale(poly->yv[0]-offy)); else
				if (poly->count == 2)
					io_cifprintf(x_(" 0V %ld %ld %ld %ld;\n"), io_cif_outscale(poly->xv[0]-offx),
						io_cif_outscale(poly->yv[0]-offy), io_cif_outscale(poly->xv[1]-offx),
							io_cif_outscale(poly->yv[1]-offy)); else
				{
					/*
					 * Now call routine to write the polygon:
					 *    - break long lines
					 *    - check for resolution errors
					 */
					for (k = 0; k < poly->count; k++)
					{
						/*
						 * WARNING: this changes poly!
						 */
						poly->xv[k] -= offx;
						poly->yv[k] -= offy;
					}
					io_cif_write_polygon(poly->layer, tech, poly->xv, poly->yv, poly->count);
				}
			}
			return;
	}
}

/*
 * routine to send a line to the CIF output file and to accumulate
 * checksum information.
 */
void io_cifprintf(CHAR *msg, ...)
{
	va_list ap;
	CHAR temp[200], *cp;
	INTBIG c;

	var_start(ap, msg);
	evsnprintf(temp, 200, msg, ap);
	va_end(ap);

	/* accumulate CRC */
	for(cp = temp; *cp != 0; cp++)
	{
		c = *cp;
		if (c > ' ')
		{
			io_check_sum = (io_check_sum << 8) ^
				io_cifcrctab[((io_check_sum >> 24) ^ c) & 0xFF];
			io_prev_chr_sep = 0;
			io_nchar++;
		} else if (io_prev_chr_sep == 0)
		{
			io_check_sum = (io_check_sum << 8) ^
				io_cifcrctab[((io_check_sum >> 24) ^ (UCHAR1)' ') & 0xFF];
			io_prev_chr_sep = 1;
			io_nchar++;
		}
	}

	xprintf(io_fileout, x_("%s"), temp);
}

/* routine which highlights box which doesn't resolve */
void io_cifreserror(GEOM *pos, INTBIG length, INTBIG width, INTBIG xc, INTBIG yc, XARRAY trans)
{
	INTBIG ptx[4], pty[4], x, y;
	REGISTER INTBIG i;
	void *err;

	/* stop if no resolution check or no geometry module */
	if (io_cif_resolution == 0 || pos == NOGEOM) return;

	ptx[0] = ptx[1] = xc - (length/2);
	ptx[2] = ptx[3] = xc + (length/2);
	pty[0] = pty[3] = yc + (width/2);
	pty[1] = pty[2] = yc - (width/2);
	for(i=0; i<4; i++)
	{
		xform(ptx[i], pty[i], &x, &y, trans);

		/* see if it resolves to grid */
		if ((x % io_cif_resolution) != 0 || (y % io_cif_resolution) != 0)
		{
			/* highlight the object */
			if (io_cif_highlightreserr)
			{
				err = logerror(x_("Resolution error"), geomparent(pos), 0);
				addgeomtoerror(err, pos, TRUE, 0, 0);
			}
			io_reserr_count++;
			return;
		}
	}
}

void io_cif_write_polygon(INTBIG layer, TECHNOLOGY *tech, INTBIG *xbuf, INTBIG *ybuf,
	INTBIG count)
{
	INTBIG buflen, prlen, i;
	REGISTER INTBIG tmpx, tmpy, lasttmpx, lasttmpy;
	CHAR outbuf[NMCHR+2], prbuf[40], *layername; /* declare buffers */

	/* write the layer name */
	if (layer < 0 || tech->temp1 == 0) return;
	layername = ((CHAR **)tech->temp1)[layer];
	if (*layername == 0) return;
	io_writeciflayer(layername);

	/* set up first line */
	(void)esnprintf(outbuf, NMCHR+2, x_("P "));
	buflen = estrlen(outbuf);

	/* check the number of points on the polygon */
	if (count > io_cif_polypoints)
		ttyputerr(_("WARNING: Polygon has too many points (%ld)"), count);

	/* search for any resolution errors on this polygon */
	/* highlight any edges that don't resolve */
	if (io_cif_resolution != 0)
	{
		lasttmpx = xbuf[count-1] + io_cifoutoffx;
		lasttmpy = ybuf[count-1] + io_cifoutoffy;
		for (i = 0; i < count; i++)
		{
			tmpx = xbuf[i] + io_cifoutoffx;
			tmpy = ybuf[i] + io_cifoutoffy;
			if ((lasttmpx%io_cif_resolution) != 0 && lasttmpx == tmpx)
			{
				io_cif_show_reserror(lasttmpx, lasttmpy, tmpx, tmpy);
			} else if ((lasttmpy%io_cif_resolution) != 0 && lasttmpy == tmpy)
			{
				io_cif_show_reserror(lasttmpx, lasttmpy, tmpx, tmpy);
			}
			lasttmpx = tmpx;
			lasttmpy = tmpy;
		}
	}

	for(i=0; i<count; i++)
	{
		(void)esnprintf(prbuf, 40, x_(" %ld %ld"), io_cif_outscale(xbuf[i]),
			io_cif_outscale(ybuf[i]));
		prlen = estrlen(prbuf); /* get length of coord. pair */

		/* if we have done the last point */
		if (i == count-1)
		{
			if (prlen + buflen < NMCHR-1) /* if space in buffer */
			{
				(void)estrcat(outbuf, prbuf); /* concatenate strings */
				io_cifprintf(x_("%s;\n"), outbuf); /* write the buffer */
			} else
			{
				/* write as two lines */
				io_cifprintf(x_("%s\n"), outbuf);
				io_cifprintf(x_("  %s;\n"), prbuf);
			}
		} else
		{
			/* not yet done */
			if (prlen + buflen < NMCHR) /* if small enough */
			{
				/* append to buffer */
				(void)estrcat(outbuf, prbuf);
				buflen = estrlen(outbuf);
			} else
			{
				/* we must write the buffer out, and re-initialize it */
				io_cifprintf(x_("%s\n"), outbuf);
				(void)esnprintf(outbuf, NMCHR+2, x_("  %s"), prbuf);
				buflen = estrlen(outbuf);
			}
		}
	}
}

CHAR *io_cif_cellname(NODEPROTO *cell)
{
	CHAR *name;

	name = describenodeproto(cell);
	return(name);
}

void io_cif_show_reserror(INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2)
{
	REGISTER void *err, *infstr;

	if (!io_cif_highlightreserr) return;

	/* show resolution error if it is in the current window */
	infstr = initinfstr();
	formatinfstr(infstr, _("Resolution error: edge is (%s,%s) to (%s,%s)"),
		latoa(x1, 0), latoa(y1, 0), latoa(x2, 0), latoa(y2, 0));
	err = logerror(returninfstr(infstr), io_cifoutcurnp, 0);
	addlinetoerror(err, x1, y1, x2, y2);
	io_reserr_count++;
}

INTBIG io_cif_outscale(INTBIG value)
{
	float v;

	v = scaletodispunit(value, DISPUNITCMIC);
	return(roundfloat(v));
}

void io_includeciflayer(TECHNOLOGY *tech, INTBIG layer)
{
	REGISTER LAYERREQ *req;

	/* stop now if the layer is already in the list */
	for(req = io_ciflayerreq; req != NOLAYERREQ; req = req->nextlayerreq)
		if (req->tech == tech && req->layer == layer) return;

	/* add to the list */
	req = io_alloclayerreq();
	if (req == NOLAYERREQ) return;
	req->tech = tech;
	req->layer = layer;
	if (layer < 0 || tech->temp1 == 0) req->layername = 0; else
		req->layername = ((CHAR **)tech->temp1)[layer];
	req->nextlayerreq = io_ciflayerreq;
	io_ciflayerreq = req;
}

LAYERREQ *io_alloclayerreq(void)
{
	REGISTER LAYERREQ *req;

	if (io_ciflayerreqfree == NOLAYERREQ)
	{
		req = (LAYERREQ *)emalloc(sizeof (LAYERREQ), io_tool->cluster);
		if (req == 0) return(NOLAYERREQ);
	} else
	{
		req = io_ciflayerreqfree;
		io_ciflayerreqfree = req->nextlayerreq;
	}
	return(req);
}

void io_freelayerreq(LAYERREQ *req)
{
	req->nextlayerreq = io_ciflayerreqfree;
	io_ciflayerreqfree = req;
}

/* CIF Options */
static DIALOGITEM io_cifoptionsdialogitems[] =
{
 /*  1 */ {0, {224,380,248,452}, BUTTON, N_("OK")},
 /*  2 */ {0, {224,240,248,312}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,248,223}, SCROLL, x_("")},
 /*  4 */ {0, {8,232,24,312}, MESSAGE, N_("CIF Layer:")},
 /*  5 */ {0, {8,316,24,454}, EDITTEXT, x_("")},
 /*  6 */ {0, {32,232,48,454}, CHECK, N_("Output Mimics Display")},
 /*  7 */ {0, {56,232,72,454}, CHECK, N_("Output Merges Boxes")},
 /*  8 */ {0, {148,232,164,454}, CHECK, N_("Input Squares Wires")},
 /*  9 */ {0, {100,232,116,454}, CHECK, N_("Output Instantiates Top Level")},
 /* 10 */ {0, {196,240,212,384}, MESSAGE, N_("Output resolution:")},
 /* 11 */ {0, {196,388,212,454}, EDITTEXT, x_("")},
 /* 12 */ {0, {172,232,188,436}, POPUP, x_("")},
 /* 13 */ {0, {124,232,140,454}, CHECK, N_("Normalize Coordinates")},
 /* 14 */ {0, {76,248,92,454}, MESSAGE, N_("(time consuming)")}
};
static DIALOG io_cifoptionsdialog = {{50,75,307,538}, N_("CIF Options"), 0, 14, io_cifoptionsdialogitems, 0, 0};

/* special items for the "CIF Options" dialog: */
#define DCFO_LAYERLIST         3		/* Layer list (scroll list) */
#define DCFO_NEWLAYER          5		/* New layer (edit text) */
#define DCFO_OMIMICDISPLAY     6		/* Output Mimics Display (check) */
#define DCFO_OMERGEBOXES       7		/* Output Merges Boxes (check) */
#define DCFO_ISQUAREWIRES      8		/* Input Squares Wires (check) */
#define DCFO_OINSTANTIATETOP   9		/* Output Instantiates top (check) */
#define DCFO_ORESOLUTION_L    10		/* Output Resolution label (stat text) */
#define DCFO_ORESOLUTION      11		/* Output resolution (edit text) */
#define DCFO_ORESOLUTIONMODE  12		/* Output resolution mode (popup) */
#define DCFO_ONORMALIZECOORD  13		/* Output normalizes coordinates (check) */

void io_cifoptionsdlog(void)
{
	INTBIG newstate[NUMIOSTATEBITWORDS];
	REGISTER INTBIG *curstate, oldresolution, i, itemHit, nameschanged;
	REGISTER CHAR **layernames, *pt;
	CHAR *newlang[3];
	REGISTER VARIABLE *var, *cifvar;
	static CHAR *whattodisplay[] = {N_("No Resolution Check"),
		N_("Report Resolution Errors"), N_("Show Resolution Errors")};
	REGISTER void *infstr, *dia;

	dia = DiaInitDialog(&io_cifoptionsdialog);
	if (dia == 0) return;
	for(i=0; i<3; i++) newlang[i] = TRANSLATE(whattodisplay[i]);
	DiaSetPopup(dia, DCFO_ORESOLUTIONMODE, 3, newlang);
	DiaInitTextDialog(dia, DCFO_LAYERLIST, DiaNullDlogList, DiaNullDlogItem,
		DiaNullDlogDone, 0, SCSELMOUSE|SCREPORT);
	layernames = (CHAR **)emalloc(el_curtech->layercount * (sizeof (CHAR *)),
		el_tempcluster);
	cifvar = getval((INTBIG)el_curtech, VTECHNOLOGY, VSTRING|VISARRAY,
		x_("IO_cif_layer_names"));
	for(i=0; i<el_curtech->layercount; i++)
	{
		if (cifvar == NOVARIABLE || i >= getlength(cifvar)) pt = x_(""); else
			pt = ((CHAR **)cifvar->addr)[i];
		(void)allocstring(&layernames[i], pt, el_tempcluster);
		infstr = initinfstr();
		addstringtoinfstr(infstr, layername(el_curtech, i));
		addstringtoinfstr(infstr, x_(" ("));
		addstringtoinfstr(infstr, pt);
		addstringtoinfstr(infstr, x_(")"));
		DiaStuffLine(dia, DCFO_LAYERLIST, returninfstr(infstr));
	}
	DiaSelectLine(dia, DCFO_LAYERLIST, 0);
	DiaSetText(dia, DCFO_NEWLAYER, layernames[0]);
	curstate = io_getstatebits();
	for(i=0; i<NUMIOSTATEBITWORDS; i++) newstate[i] = curstate[i];
	if ((curstate[0]&CIFOUTEXACT) != 0) DiaSetControl(dia, DCFO_OMIMICDISPLAY, 1);
	if ((curstate[0]&CIFOUTMERGE) != 0) DiaSetControl(dia, DCFO_OMERGEBOXES, 1);
	if ((curstate[0]&CIFINSQUARE) != 0) DiaSetControl(dia, DCFO_ISQUAREWIRES, 1);
	if ((curstate[0]&CIFOUTNOTOPCALL) == 0) DiaSetControl(dia, DCFO_OINSTANTIATETOP, 1);
	if ((curstate[0]&CIFOUTNORMALIZE) != 0) DiaSetControl(dia, DCFO_ONORMALIZECOORD, 1);
	var = getval((INTBIG)el_curtech, VTECHNOLOGY, VINTEGER, x_("IO_cif_resolution"));
	if (var == NOVARIABLE) oldresolution = 0; else oldresolution = var->addr;
	if (oldresolution == 0)
	{
		DiaSetText(dia, DCFO_ORESOLUTION, x_(""));
		DiaSetPopupEntry(dia, DCFO_ORESOLUTIONMODE, 0);
		DiaDimItem(dia, DCFO_ORESOLUTION_L);
		DiaDimItem(dia, DCFO_ORESOLUTION);
	} else
	{
		DiaUnDimItem(dia, DCFO_ORESOLUTION_L);
		DiaUnDimItem(dia, DCFO_ORESOLUTION);
		DiaSetText(dia, DCFO_ORESOLUTION, latoa(oldresolution, 0));
		if ((curstate[1]&CIFRESHIGH) != 0)
			DiaSetPopupEntry(dia, DCFO_ORESOLUTIONMODE, 2); else
				DiaSetPopupEntry(dia, DCFO_ORESOLUTIONMODE, 1);
	}

	/* loop until done */
	nameschanged = 0;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL) break;
		if (itemHit == OK)
		{
			i = DiaGetPopupEntry(dia, DCFO_ORESOLUTIONMODE);
			if (i != 0 && atola(DiaGetText(dia, DCFO_ORESOLUTION), 0) == 0)
			{
				DiaMessageInDialog(_("Must set nonzero output resolution"));
				continue;
			}
			break;
		}
		if (itemHit == DCFO_OMIMICDISPLAY || itemHit == DCFO_OMERGEBOXES ||
			itemHit == DCFO_ISQUAREWIRES || itemHit == DCFO_OINSTANTIATETOP ||
			itemHit == DCFO_ONORMALIZECOORD)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
		if (itemHit == DCFO_LAYERLIST)
		{
			i = DiaGetCurLine(dia, DCFO_LAYERLIST);
			DiaSetText(dia, DCFO_NEWLAYER, layernames[i]);
			continue;
		}
		if (itemHit == DCFO_ORESOLUTIONMODE)
		{
			i = DiaGetPopupEntry(dia, DCFO_ORESOLUTIONMODE);
			if (i == 0)
			{
				DiaDimItem(dia, DCFO_ORESOLUTION_L);
				DiaDimItem(dia, DCFO_ORESOLUTION);
				DiaSetText(dia, DCFO_ORESOLUTION, x_(""));
			} else
			{
				DiaUnDimItem(dia, DCFO_ORESOLUTION_L);
				DiaUnDimItem(dia, DCFO_ORESOLUTION);
				if (atola(DiaGetText(dia, DCFO_ORESOLUTION), 0) == 0)
					DiaSetText(dia, DCFO_ORESOLUTION, x_("1"));
			}
			continue;
		}
		if (itemHit == DCFO_NEWLAYER)
		{
			i = DiaGetCurLine(dia, DCFO_LAYERLIST);
			pt = DiaGetText(dia, DCFO_NEWLAYER);
			if (estrcmp(pt, layernames[i]) == 0) continue;
			nameschanged++;
			(void)reallocstring(&layernames[i], pt, el_tempcluster);
			infstr = initinfstr();
			addstringtoinfstr(infstr, layername(el_curtech, i));
			addstringtoinfstr(infstr, x_(" ("));
			addstringtoinfstr(infstr, layernames[i]);
			addstringtoinfstr(infstr, x_(")"));
			DiaSetScrollLine(dia, DCFO_LAYERLIST, i, returninfstr(infstr));
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		if (DiaGetControl(dia, DCFO_OMIMICDISPLAY) != 0) newstate[0] |= CIFOUTEXACT; else
			newstate[0] &= ~CIFOUTEXACT;
		if (DiaGetControl(dia, DCFO_OMERGEBOXES) != 0) newstate[0] |= CIFOUTMERGE; else
			newstate[0] &= ~CIFOUTMERGE;
		if (DiaGetControl(dia, DCFO_ISQUAREWIRES) != 0) newstate[0] |= CIFINSQUARE; else
			newstate[0] &= ~CIFINSQUARE;
		if (DiaGetControl(dia, DCFO_OINSTANTIATETOP) == 0) newstate[0] |= CIFOUTNOTOPCALL; else
			newstate[0] &= ~CIFOUTNOTOPCALL;
		if (DiaGetControl(dia, DCFO_ONORMALIZECOORD) != 0) newstate[0] |= CIFOUTNORMALIZE; else
			newstate[0] &= ~CIFOUTNORMALIZE;
		if (DiaGetPopupEntry(dia, DCFO_ORESOLUTIONMODE) == 2) newstate[1] |= CIFRESHIGH; else
			newstate[1] &= ~CIFRESHIGH;
		for(i=0; i<NUMIOSTATEBITWORDS; i++) if (curstate[i] != newstate[i]) break;
		if (i < NUMIOSTATEBITWORDS) io_setstatebits(newstate);
		if (nameschanged != 0)
			setval((INTBIG)el_curtech, VTECHNOLOGY, x_("IO_cif_layer_names"),
				(INTBIG)layernames, VSTRING|VISARRAY|
					(el_curtech->layercount<<VLENGTHSH));
		i = atola(DiaGetText(dia, DCFO_ORESOLUTION), 0);
		if (i != oldresolution)
			setval((INTBIG)el_curtech, VTECHNOLOGY, x_("IO_cif_resolution"), i, VINTEGER);
	}
	for(i=0; i<el_curtech->layercount; i++) efree(layernames[i]);
	efree((CHAR *)layernames);
	DiaDoneDialog(dia);
}
