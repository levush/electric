/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iogdso.c
 * Input/output tool: GDS-II output
 * Written by: Sid Penstone, Queens University
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
#include "efunction.h"
#include "egraphics.h"
#include "tech.h"
#include "dbcontour.h"
#include "tecgen.h"
#include "tecart.h"
#include "edialogs.h"

/* #define OLDGDS 1 */ 

#define GDSVERSION            3
#define BYTEMASK           0xFF
#define NAMSIZE             100
#define DSIZE               512		/* data block */
#define MAXPOINTS           510		/* maximum points in a polygon */
#define EXPORTPRESENTATION    0		/* centered (was 8 for bottomleft) */

/* GDSII bit assignments in STRANS record */
#define STRANS_REFLX     0x8000
#define STRANS_ABSA         0x2

/* data type codes */
#define DTYP_NONE             0

/* header codes */
#define HDR_HEADER       0x0002
#define HDR_BGNLIB       0x0102
#define HDR_LIBNAME      0x0206
#define HDR_UNITS        0x0305
#define HDR_ENDLIB       0x0400
#define HDR_BGNSTR       0x0502
#define HDR_STRNAME      0x0606
#define HDR_ENDSTR       0x0700
#define HDR_BOUNDARY     0x0800
#define HDR_PATH         0x0900
#define HDR_SREF         0x0A00
#define HDR_AREF	     0x0B00
#define HDR_TEXT	     0x0C00
#define HDR_LAYER        0x0D02
#define HDR_DATATYPE     0x0E02
#define HDR_XY           0x1003
#define HDR_ENDEL        0x1100
#define HDR_SNAME        0x1206
#define HDR_TEXTTYPE     0x1602
#define HDR_PRESENTATION 0x1701
#define HDR_STRING	     0x1906
#define HDR_STRANS       0x1A01
#define HDR_MAG          0x1B05
#define HDR_ANGLE        0x1C05

/* Header byte counts */
#define HDR_N_BGNLIB         28
#define HDR_N_UNITS          20
#define HDR_N_ANGLE          12
#define HDR_N_MAG            12

/* Maximum string sizes  */
#define HDR_M_SNAME          32
#define HDR_M_STRNAME        32
#define HDR_M_ASCII         256

/* contour gathering thresholds for polygon accumulation */
#define BESTTHRESH           0.001			/* 1/1000 of a millimeter */
#define WORSTTHRESH          0.1			/* 1/10 of a millimeter */

typedef struct
{
	INTBIG layernumber;
	INTBIG pinnumber;
	INTBIG textnumber;
} LAYERNUMBERS;

static UCHAR1  *io_dbuffer= 0, *io_empty;	/* for buffering output data  */
static INTBIG  io_gds_curlayer;				/* Current layer for gds output */
static INTBIG  io_gds_pos;					/* Position of next byte in the buffer */
static INTBIG  io_gds_block;				/* Number data buffers output so far */
static FILE   *io_gds_outfd;				/* File descriptor for output */
static INTBIG *io_gds_outputstate;			/* current output state */
static INTBIG  io_gds_polypoints;			/* number of allowable points on a polygon */
static double  io_gds_scale;				/* constant */
static INTBIG  io_gds_layerkey;				/* key for "IO_gds_layer" */
static INTBIG  io_gds_exportlayer;			/* layer for export text */

/* working memory for "io_gds_bgnstr()" */
static INTBIG  io_gdscellnamecount;
static INTBIG  io_gdscellnametotal = 0;
static CHAR  **io_gdscellnames;

/* prototypes for local routines */
static void  io_gdswritecell(NODEPROTO*);
static void  io_outputgdspoly(TECHNOLOGY*, POLYGON*, INTBIG, INTBIG, INTBIG, XARRAY, INTBIG);
static void  io_gds_set_layer(INTBIG);
static void  io_gds_initialize(void);
static void  io_gds_closefile(void);
static void  io_gds_bgnlib(NODEPROTO*);
static void  io_gds_bgnstr(NODEPROTO*);
static void  io_gds_date(time_t*);
static void  io_gds_header(INTBIG, INTBIG);
static void  io_gds_name(INTBIG, CHAR*, INTBIG);
static void  io_gds_angle(INTBIG);
static void  io_gds_mag(double scale);
static void  io_gds_boundary(POLYGON*);
static void  io_gds_path(POLYGON*);
static void  io_gds_byte(UCHAR1);
static void  io_gds_int2(INTSML);
static void  io_gds_int4(INTBIG);
static void  io_gds_int2_arr(INTSML*, INTBIG);
static void  io_gds_int4_arr(INTBIG*, INTBIG);
static void  io_gds_string(CHAR*, INTBIG);
static void  io_gds_write_polygon(INTBIG, TECHNOLOGY*, INTBIG*, INTBIG*, INTBIG);
static CHAR *io_gds_makename(CHAR*);
static void  io_gds_cvfloat8(double*, UINTBIG*, UINTBIG*);
static void  io_gdsoptionsdlog(void);
static CHAR *io_gds_uniquename(NODEPROTO *np);
static void  io_gds_parselayerstring(CHAR *string, CHAR **val, CHAR **pinval, CHAR **textval);

/*
 * Routine to free all memory associated with this module.
 */
void io_freegdsoutmemory(void)
{
	REGISTER INTBIG i;

	if (io_dbuffer != 0) efree((CHAR *)io_dbuffer);
	if (io_empty != 0) efree((CHAR *)io_empty);

	for(i=0; i<io_gdscellnametotal; i++)
		if (io_gdscellnames[i] != 0)
			efree((CHAR *)io_gdscellnames[i]);
	if (io_gdscellnametotal > 0)
		efree((CHAR *)io_gdscellnames);
}

/*
 * Routine to initialize GDS I/O.
 */
void io_initgds(void)
{
	extern COMCOMP io_gdsp;

	DiaDeclareHook(x_("gdsopt"), &io_gdsp, io_gdsoptionsdlog);
}

BOOLEAN io_writegdslibrary(LIBRARY *lib)
{
	CHAR *name, *truename, *val, *pinval, *textval;
	REGISTER NODEPROTO *np;
	REGISTER VARIABLE *var;
	REGISTER INTBIG i, varlen;
	REGISTER LAYERNUMBERS *layernumbers;
	REGISTER TECHNOLOGY *tech;
	REGISTER LIBRARY *olib;
	REGISTER void *infstr;

	/* create the proper disk file for the GDS II */
	if (lib->curnodeproto == NONODEPROTO)
	{
		ttyputerr(_("Must be editing a cell to generate GDS output"));
		return(TRUE);
	}

	/* construct file name */
	infstr = initinfstr();
	addstringtoinfstr(infstr, lib->curnodeproto->protoname);
	addstringtoinfstr(infstr, x_(".gds"));
	(void)allocstring(&name, truepath(returninfstr(infstr)), el_tempcluster);

	/* get the file */
	io_gds_outfd = xcreate(name, io_filetypegds, _("GDS File"), &truename);
	if (io_gds_outfd == NULL)
	{
		if (truename != 0) ttyputerr(_("Cannot write %s"), truename);
		efree(name);
		return(TRUE);
	}

	/* get current output state */
	io_gds_outputstate = io_getstatebits();
	var = getval((INTBIG)el_curtech, VTECHNOLOGY, VINTEGER|VISARRAY,
		x_("IO_gds_export_layer"));
	if (var == NOVARIABLE) io_gds_exportlayer = 230; else
		io_gds_exportlayer = var->addr;

	/* get the number of allowable points on a polygon */
	var = getval((INTBIG)el_curtech, VTECHNOLOGY, VINTEGER, x_("IO_gds_polypoints"));
	if (var == NOVARIABLE) io_gds_polypoints = 200; else /* 200 is the MAX allowed in GDS-II */
		io_gds_polypoints = var->addr;

	/* initialize cache of layer information */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		tech->temp1 = 0;
		var = getval((INTBIG)tech, VTECHNOLOGY, -1, x_("IO_gds_layer_numbers"));
		if (var != NOVARIABLE && (var->type&VISARRAY) != 0)
			tech->temp1 = var->addr;

		/* if ignoring DRC mask layer, delete Generic technology layer names */
		if (tech == gen_tech && (io_gds_outputstate[0]&GDSOUTADDDRC) == 0) tech->temp1 = 0;

		/* create an array of layer numbers from the string names */
		if (tech->temp1 != 0)
		{
			layernumbers = (LAYERNUMBERS *)emalloc(tech->layercount * (sizeof (LAYERNUMBERS)), io_tool->cluster);
			if (layernumbers == 0) return(TRUE);
			varlen = getlength(var);
			for(i=0; i<tech->layercount; i++)
			{
				if ((var->type&VTYPE) == VSTRING && i < varlen)
				{
					io_gds_parselayerstring(((CHAR **)tech->temp1)[i], &val, &pinval, &textval);
					layernumbers[i].layernumber = myatoi(val);
					if (*pinval == 0) layernumbers[i].pinnumber = -1; else
						layernumbers[i].pinnumber = myatoi(pinval);
					if (*textval == 0) layernumbers[i].textnumber = -1; else
						layernumbers[i].textnumber = myatoi(textval);
				} else
				{
					layernumbers[i].layernumber = ((INTBIG *)tech->temp1)[i];
					layernumbers[i].pinnumber = 0;
					layernumbers[i].textnumber = 0;
				}
			}
			tech->temp1 = (INTBIG)layernumbers;
		}

		/* layer names for bloating */
		var = getval((INTBIG)tech, VTECHNOLOGY, VSTRING|VISARRAY, x_("TECH_layer_names"));
		tech->temp2 = (var == NOVARIABLE ? 0 : var->addr);
	}

	/* make sure key for individual layer overrides is set */
	io_gds_layerkey = makekey(x_("IO_gds_layer"));

	/* warn if current technology has no GDS layers */
	if (el_curtech->temp1 == 0)
		ttyputmsg(_("Warning: there are no GDS II layers defined for the %s technology"),
			el_curtech->techname);

	if ((io_gds_outputstate[0]&GDSOUTMERGE) != 0) mrginit();

	io_gds_initialize();
	io_gds_bgnlib(lib->curnodeproto);

	/* write the GDS-II */
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = 0;
	io_gdswritecell(lib->curnodeproto);

	/* clean up */
	io_gds_header(HDR_ENDLIB, 0);
	io_gds_closefile();

	if ((io_gds_outputstate[0]&GDSOUTMERGE) != 0) mrgterm();
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
	{
		for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->temp1 != 0) efree((CHAR *)np->temp1);
		}
	}
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		if (tech->temp1 != 0) efree((CHAR *)tech->temp1);
	}

	/* tell the user that the file is written */
	ttyputmsg(_("%s written"), truename);
	efree(name);
	return(FALSE);
}

void io_gdswritecell(NODEPROTO *np)
{
	REGISTER NODEINST *subno, *bottomni;
	REGISTER ARCINST *subar;
	REGISTER NODEPROTO *subnt, *onp;
	REGISTER PORTPROTO *pp, *bottompp;
	REGISTER VARIABLE *var;
	REGISTER INTBIG i, j, tot, fun, displaytotal, layeroverride, oldbits,
		offx, offy, bestthresh, worstthresh, textlayer, pinlayer,
		angle, transvalue, gatherpolygon;
	INTBIG xpos, ypos, bx, by, fx, fy, tx, ty;
	XARRAY trans, localtran, temptrans;
	REGISTER LAYERNUMBERS *laynum;
	static POLYGON *poly = NOPOLYGON;
	CHAR str[513];
	REGISTER CONTOUR *con, *nextcon, *contourlist;
	REGISTER CONTOURELEMENT *conel;
#ifdef OLDGDS
	REGISTER NODEPROTO *bodysubnt;
	INTBIG cx, cy;
#endif

	/* if there are any sub-cells that have not been written, write them */
	for(subno = np->firstnodeinst; subno != NONODEINST; subno = subno->nextnodeinst)
	{
		subnt = subno->proto;
		if (subnt->primindex != 0) continue;

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(subnt, np)) continue;

		/* convert body cells to contents cells */
		onp = contentsview(subnt);
		if (onp != NONODEPROTO) subnt = onp;

		/* don't recurse if this cell has already been written */
		if (subnt->temp1 != 0) continue;

		/* recurse to the bottom */
		io_gdswritecell(subnt);
	}

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, io_tool->cluster);

	/* write this cell */
	io_gds_set_layer(0);
	(void)allocstring((CHAR **)&np->temp1, io_gds_uniquename(np), io_tool->cluster);
#ifdef OLDGDS
	offx = (np->lowx + np->highx) / 2;
	offy = (np->lowy + np->highy) / 2;
#else
	offx = offy = 0;
#endif
	io_gds_bgnstr(np);

	/* see if there are any outline nodes (opened polygon or circle arcs) */
	gatherpolygon = 0;
	for(subno = np->firstnodeinst; subno != NONODEINST; subno = subno->nextnodeinst)
	{
		if (subno->proto == art_openedpolygonprim || subno->proto == art_circleprim ||
			subno->proto == art_thickcircleprim)
		{
			gatherpolygon = 1;
			break;
		}
	}
	if (gatherpolygon != 0)
	{
		bestthresh = scalefromdispunit((float)BESTTHRESH, DISPUNITMM);
		worstthresh = scalefromdispunit((float)WORSTTHRESH, DISPUNITMM);
		contourlist = gathercontours(np, 0, bestthresh, worstthresh);

		/* mark all nodes not in contours as not-written */
		for(subno = np->firstnodeinst; subno != NONODEINST; subno = subno->nextnodeinst)
			subno->temp1 = 0;

		/* write the contour polygons */
		for(con = contourlist; con != NOCONTOUR; con = con->nextcontour)
		{
			poly->count = 0;
			poly->style = OPENED;
			poly->layer = 0;

			/* add in first point of contour */
			if (poly->count+1 >= poly->limit)
				(void)extendpolygon(poly, poly->count+1);
			poly->xv[poly->count] = con->firstcontourelement->sx;
			poly->yv[poly->count] = con->firstcontourelement->sy;
			poly->count++;
			layeroverride = -1;
			for(conel = con->firstcontourelement; conel != NOCONTOURELEMENT; conel = conel->nextcontourelement)
			{
				conel->ni->temp1 = 1;
				switch (conel->elementtype)
				{
					case ARCSEGMENTTYPE:
					case REVARCSEGMENTTYPE:
					case CIRCLESEGMENTTYPE:
						initcontoursegmentgeneration(conel);
						for(;;)
						{
							if (nextcontoursegmentgeneration(&fx, &fy, &tx, &ty)) break;
							if (poly->count+1 >= poly->limit)
								(void)extendpolygon(poly, poly->count+1);
							poly->xv[poly->count] = tx;
							poly->yv[poly->count] = ty;
							poly->count++;
						}
						break;
					case LINESEGMENTTYPE:
						if (poly->count+1 >= poly->limit)
							(void)extendpolygon(poly, poly->count+1);
						poly->xv[poly->count] = conel->ex;
						poly->yv[poly->count] = conel->ey;
						poly->count++;
						break;
					default:
						break;
				}
				var = getvalkey((INTBIG)conel->ni, VNODEINST, VINTEGER, io_gds_layerkey);
				if (var != NOVARIABLE) layeroverride = var->addr;
			}
			io_outputgdspoly(art_tech, poly, 0, offx, offy, el_matid, layeroverride);
		}

		/* dispose the data */
		for(con = contourlist; con != NOCONTOUR; con = nextcon)
		{
			nextcon = con->nextcontour;
			killcontour(con);
		}
	} else
	{
		/* mark all nodes as not-written */
		for(subno = np->firstnodeinst; subno != NONODEINST; subno = subno->nextnodeinst)
			subno->temp1 = 0;
	}

	/* write all nodes in the cell */
	for(subno = np->firstnodeinst; subno != NONODEINST; subno = subno->nextnodeinst)
	{
		if (subno->temp1 != 0) continue;
		subno->temp1 = 1;

		layeroverride = -1;
		var = getvalkey((INTBIG)subno, VNODEINST, VINTEGER, io_gds_layerkey);
		if (var != NOVARIABLE) layeroverride = var->addr;
		subnt = subno->proto;
		angle = subno->rotation;
		if (subno->transpose != 0) angle = (angle + 900) % 3600;
		makerot(subno, trans);
		if (subnt->primindex != 0)
		{
			/* don't draw anything if the node is wiped out */
			if ((subno->userbits&WIPED) != 0) continue;

			/* check for displayable variables */
			displaytotal = tech_displayablenvars(subno, NOWINDOWPART, &tech_oneprocpolyloop);
			if (displaytotal != 0)
			{
				/* get the first polygon off the node, this is the text layer */
				shapenodepoly(subno, 0, poly);

				/* determine the layer name for this polygon */
				if (poly->layer < 0 || subnt->tech->temp1 == 0) continue;
				laynum = &((LAYERNUMBERS *)subnt->tech->temp1)[poly->layer];
				io_gds_set_layer(laynum->layernumber);

				for (i=0; i<displaytotal; i++)
				{
					(void)tech_filldisplayablenvar(subno, poly, NOWINDOWPART, 0, &tech_oneprocpolyloop);

					/* dump this text field */
					io_gds_header(HDR_TEXT, 0);
					io_gds_header(HDR_LAYER, io_gds_curlayer);
					io_gds_header(HDR_TEXTTYPE, 0);
					io_gds_header(HDR_PRESENTATION, EXPORTPRESENTATION);

					/* now the orientation */
					transvalue = 0;
					if (subno->transpose != 0)
					{
						/* Angles are reversed  */
						transvalue |= STRANS_REFLX;
						angle = (3600 - angle)%3600;
					}
					io_gds_header(HDR_STRANS, transvalue);
					io_gds_angle(angle);
					io_gds_int2(12);
					io_gds_int2(HDR_XY);
					io_gds_int4(rounddouble(io_gds_scale*(double)poly->xv[0]));
					io_gds_int4(rounddouble(io_gds_scale*(double)poly->yv[0]));

					/* now the string */
					j = estrlen (poly->string);
					if (j > 512) j = 512;
					estrncpy (str, poly->string, j);

					/* pad with a blank */
					if (j&1) str[j++] = ' ';
					str[j] = '\0';
					io_gds_int2((INTSML)(4+j));
					io_gds_int2(HDR_STRING);
					io_gds_string(str, j);
					io_gds_header(HDR_ENDEL, 0);

					/* write out the node the text is attached to */
					/* RLW - Queen's */
					i = nodepolys(subno, 0, NOWINDOWPART);
					for (j=0; j<i; j++)
					{
						shapenodepoly(subno, j, poly);
						io_outputgdspoly(subnt->tech, poly, angle, offx, offy, trans, layeroverride);
					}
				}
			} else
			{
				/* write a primitive nodeinst */
				i = nodepolys(subno, 0, NOWINDOWPART);
				for(j=0; j<i; j++)
				{
					shapenodepoly(subno, j, poly);
					io_outputgdspoly(subnt->tech, poly, angle, offx, offy, trans, layeroverride);
				}
			}
		} else
		{
			/* ignore recursive references (showing icon in contents) */
			if (isiconof(subno->proto, np)) continue;

			/* write a call to a cell */
#ifdef OLDGDS
			xpos = (subno->lowx + subno->highx) / 2 - offx;
			ypos = (subno->lowy + subno->highy) / 2 - offy;

			/* convert body cells to contents cells */
			onp = contentsview(subnt);
			if (onp != NONODEPROTO)
			{
				/* look for grab points in contents and body cells */
				bodysubnt = subnt;
				subnt = onp;
				corneroffset(subno, bodysubnt, subno->rotation, subno->transpose, &bx, &by, FALSE);
				corneroffset(NONODEINST, subnt, subno->rotation, subno->transpose, &cx, &cy, FALSE);
				xpos = subno->lowx + bx - cx + (subnt->highx-subnt->lowx)/2 - offx;
				ypos = subno->lowy + by - cy + (subnt->highy-subnt->lowy)/2 - offy;
			}
#else
			/* now origin, normal placement */
			bx = (subno->lowx - subno->proto->lowx);
			by = (subno->lowy - subno->proto->lowy);
			xform(bx, by, &xpos, &ypos, trans);
#endif

			/* Generate a symbol reference  */
			io_gds_header(HDR_SREF, 0);
			io_gds_name(HDR_SNAME, (CHAR *)subnt->temp1, HDR_M_SNAME);
			transvalue = 0;
			if (subno->transpose != 0)
			{
				/* Angles are reversed */
				transvalue |= STRANS_REFLX;
				angle = (3600 - angle)%3600;
			}
#ifdef OLDGDS
			if (angle != 0) transvalue |= STRANS_ABSA;
			if (transvalue != 0) io_gds_header(HDR_STRANS, transvalue);
			if (angle != 0) io_gds_angle(angle);
#else
			/* always output the angle and transvalue */
			io_gds_header(HDR_STRANS, transvalue);
			io_gds_angle(angle);
#endif
			io_gds_int2(12);
			io_gds_int2(HDR_XY);
			io_gds_int4(rounddouble(io_gds_scale*(double)xpos));
			io_gds_int4(rounddouble(io_gds_scale*(double)ypos));
			io_gds_header(HDR_ENDEL, 0);
		}
	}

	/* now do the arcs in the cell */
	for(subar = np->firstarcinst; subar != NOARCINST; subar = subar->nextarcinst)
	{
		/* arcinst: ask the technology how to draw it */
		i = arcpolys(subar, NOWINDOWPART);
		var = getvalkey((INTBIG)subar, VARCINST, VINTEGER, io_gds_layerkey);
		if (var != NOVARIABLE) layeroverride = var->addr;

		/* plot each layer of the arcinst */
		for(j=0; j<i; j++)
		{
			/* write the box describing the layer */
			shapearcpoly(subar, j, poly);
			io_outputgdspoly(subar->proto->tech, poly, 0, offx, offy, el_matid, layeroverride);
		}
	}

	/* now write exports */
	if (io_gds_exportlayer >= 0)
	{
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			/* find the node at the bottom of this export */
			bottomni = pp->subnodeinst;
			bottompp = pp->subportproto;
			makerot(bottomni, trans);
			while (bottomni->proto->primindex == 0)
			{
				maketrans(bottomni, localtran);
				transmult(localtran, trans, temptrans);
				bottomni = bottompp->subnodeinst;
				bottompp = bottompp->subportproto;
				makerot(bottomni, localtran);
				transmult(localtran, temptrans, trans);
			}

			/* find the layer associated with this node */
			oldbits = bottomni->userbits;
			bottomni->userbits &= ~WIPED;
			tot = nodepolys(bottomni, 0, NOWINDOWPART);
			shapenodepoly(bottomni, 0, poly);
			bottomni->userbits = oldbits;
			fun = layerfunction(poly->tech, poly->layer);
			if ((fun&LFPSEUDO) != 0)
				poly->layer = nonpseudolayer(poly->layer, poly->tech);

			textlayer = pinlayer = io_gds_exportlayer;
			if (poly->layer >= 0 && poly->tech->temp1 != 0)
			{
				laynum = &((LAYERNUMBERS *)poly->tech->temp1)[poly->layer];
				if (laynum->pinnumber >= 0) pinlayer = laynum->pinnumber;
				if (laynum->textnumber >= 0) textlayer = laynum->textnumber;
			}

			/* put out a pin if requested */
			if ((io_gds_outputstate[1]&GDSOUTPINS) != 0)
			{
				io_outputgdspoly(poly->tech, poly, 0, offx, offy, trans, pinlayer);
			}

			io_gds_header(HDR_TEXT, 0);
			io_gds_header(HDR_LAYER, textlayer);
			io_gds_header(HDR_TEXTTYPE, 0);
			io_gds_header(HDR_PRESENTATION, EXPORTPRESENTATION);

			/* now the orientation */
			subno = pp->subnodeinst;
			transvalue = 0;
			if (subno->transpose != 0)
			{
				/* Angles are reversed  */
				transvalue |= STRANS_REFLX;
				angle = (3600 - angle)%3600;
			}
			io_gds_header(HDR_STRANS, transvalue);
			io_gds_mag(0.5);
			io_gds_angle(angle);
			io_gds_int2(12);
			io_gds_int2(HDR_XY);
			portposition(subno, pp->subportproto, &bx, &by);
			io_gds_int4(rounddouble(io_gds_scale*(double)bx));
			io_gds_int4(rounddouble(io_gds_scale*(double)by));

			/* now the string */
			j = estrlen(pp->protoname);
			if (j > 512) j = 512;
			estrncpy(str, pp->protoname, j);

			/* pad with a blank */
			if (j&1) str[j++] = ' ';
			str[j] = '\0';
			io_gds_int2((INTSML)(4+j));
			io_gds_int2(HDR_STRING);
			io_gds_string(str, j);
			io_gds_header(HDR_ENDEL, 0);
		}
	}

	if ((io_gds_outputstate[0]&GDSOUTMERGE) != 0)
		mrgdonecell(io_gds_write_polygon);

	io_gds_header(HDR_ENDSTR, 0);
}

/*
 * routine to write polygon "poly" to the GDS-II file.  The polygon is from
 * technology "tech", is rotated by the angle "angle", and is offset
 * by "offx" and "offy" in its parent cell.  If "layeroverride" is not -1,
 * it is the layer number to use (an override).
 */
void io_outputgdspoly(TECHNOLOGY *tech, POLYGON *poly, INTBIG angle, INTBIG offx,
	INTBIG offy, XARRAY trans, INTBIG layeroverride)
{
	REGISTER INTBIG r, bloat;
	INTBIG i, layernumber, xl, xh, yl, yh, xpos, ypos;
	REGISTER LAYERNUMBERS *laynum;
	static POLYGON *poly2 = NOPOLYGON;
	REGISTER void *infstr;

	(void)needstaticpolygon(&poly2, 4, io_tool->cluster);

	/* determine the layer name for this polygon */
	if (layeroverride != -1) layernumber = layeroverride; else
	{
		if (poly->layer < 0 || tech->temp1 == 0) return;
		laynum = &((LAYERNUMBERS *)tech->temp1)[poly->layer];
		layernumber = laynum->layernumber;
		if (layernumber < 0) return;
	}
	io_gds_set_layer(layernumber);

	/* determine bloat factor */
	if (poly->layer < 0 || tech->temp2 == 0) bloat = 0; else
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, tech->techname);
		addtoinfstr(infstr, ':');
		addstringtoinfstr(infstr, ((CHAR **)tech->temp2)[poly->layer]);
		bloat = io_getoutputbloat(returninfstr(infstr));
	}

	switch (poly->style)
	{
		case DISC:      /* Make a square of the size of the diameter  */
			xformpoly(poly, trans);
			r = computedistance(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1]);
			if (r <= 0) break;
			xform(poly->xv[0], poly->yv[0], &xpos, &ypos, trans);
			xl = xpos - offx - (r + bloat/2);
			xh = xl + 2*r + bloat;
			yl = ypos - offy - (r + bloat/2);
			yh = yl + 2*r + bloat;
			makerectpoly(xl, xh, yl, yh, poly2);
			io_gds_boundary(poly2);
			return;

		default:
			if (isbox(poly, &xl,&xh, &yl,&yh))
			{
				/* rectangular manhattan shape */
				if (xh == xl || yh == yl) return;

				/* find the center of this polygon, inside the cell */
				xform((xl+xh)/2, (yl+yh)/2, &xpos, &ypos, trans);

				if ((io_gds_outputstate[0]&GDSOUTMERGE) != 0 && angle%900 == 0)
				{
					/* do polygon format output */
					if (angle%1800 == 0)
						mrgstorebox(layernumber, tech, xh-xl+bloat, yh-yl+bloat, xpos-offx, ypos-offy);
					else	/* rotated by 90 degrees */
						mrgstorebox(layernumber, tech, yh-yl+bloat, xh-xl+bloat, xpos-offx, ypos-offy);
				} else    /* Output as a single box */
				{
					makerectpoly(xl-bloat/2, xh+bloat/2, yl-bloat/2, yh+bloat/2, poly2);
					switch(poly->style)
					{
						case FILLEDRECT: poly2->style = FILLED;    break;
						case CLOSEDRECT: poly2->style = CLOSED;    break;
						default:         poly2->style = poly->style;
					}
					xformpoly(poly2, trans); /* in 4.05, did this only for angle = 0 */

					/* Now output the box as a set of points  */
					for (i = 0; i < poly2->count; i++)
					{
						poly2->xv[i] = poly2->xv[i] - offx;
						poly2->yv[i] = poly2->yv[i] - offy;
					}
					io_gds_boundary(poly2);
				}
				break;
			}

			/* non-manhattan or worse .. direct output */
			if (bloat != 0 && poly->count > 4)
				ttyputmsg(_("Warning: complex polygon cannot be bloated"));
			xformpoly(poly, trans); /* was missing in 4.05 (SRP) */
			if (poly->count == 1)
			{
				ttyputerr(_("Single point cannot be written in GDS-II"));
				break;
			}

			/* polygonally defined shape or a line */
			if (poly2->limit < poly->count)	 /* SRP */
				(void)extendpolygon(poly2, poly->count);

			for (i = 0; i < poly->count; i++)
			{
				poly2->xv[i] = poly->xv[i] - offx;
				poly2->yv[i] = poly->yv[i] - offy;
			}
			poly2->count = poly->count;
			if (poly->count == 2) io_gds_path(poly2); else
				io_gds_boundary(poly2);
			break;
	}
}

/*
 * routine to set the correct layer in "layer" for later output.
 */
void io_gds_set_layer(INTBIG layer)
{
	io_gds_curlayer = layer;
}

/*************************** GDS OUTPUT ROUTINES ***************************/

/* Initialize various fields, get some standard values */
void io_gds_initialize(void)
{
	INTBIG i;

	io_gds_block = 0;
	io_gds_pos = 0;
	io_gds_set_layer(0);
	if (io_dbuffer == 0)
	{
		io_dbuffer = (UCHAR1 *)emalloc(DSIZE, io_tool->cluster);
		io_empty = (UCHAR1 *)emalloc(DSIZE, io_tool->cluster);

		/* all zeroes, even if malloc() did it  */
		for (i=0; i<DSIZE; i++) io_empty[i] = 0;
	}
	io_gds_scale = 1000.0 / (double)scalefromdispunit(1.0, DISPUNITMIC);
}

/*
 * Close the file, pad to make the file match the tape format
 */
void io_gds_closefile(void)
{
	INTBIG i;

	/* Write out the current buffer */
	if (io_gds_pos > 0)
	{
		/* Pack with zeroes */
		for (i = io_gds_pos; i < DSIZE; i++) io_dbuffer[i] = 0;
		(void)xfwrite(io_dbuffer, DSIZE, 1, io_gds_outfd);
		io_gds_block++;
	}

	/*  Pad to 2048  */
	while (io_gds_block%4 != 0)
	{
		(void)xfwrite(io_empty, DSIZE, 1, io_gds_outfd);
		io_gds_block++;
	}
	xclose(io_gds_outfd);
}

/* Write a library header, get the date information */
void io_gds_bgnlib(NODEPROTO *np)
{
	CHAR name[NAMSIZE];
	double xnum;
	static INTBIG units[4];
	/* GDS floating point values - -
	 * 0x3E418937,0x4BC6A7EF = 0.001
	 * 0x3944B82F,0xA09B5A53 = 1e-9
	 * 0x3F28F5C2,0x8F5C28F6 = 0.01
	 * 0x3A2AF31D,0xC4611874 = 1e-8
	 */

	/* set units */
	/* modified 9 april 92 - RLW */
	xnum = 1e-3;
	io_gds_cvfloat8(&xnum, (UINTBIG *)&units[0], (UINTBIG *)&units[1]);
	xnum = 1.0e-9;
	io_gds_cvfloat8(&xnum, (UINTBIG *)&units[2], (UINTBIG *)&units[3]);

	io_gds_header(HDR_HEADER, GDSVERSION);
	io_gds_header(HDR_BGNLIB, 0);
	io_gds_date((time_t *)&np->creationdate);
	io_gds_date((time_t *)&np->revisiondate);
	(void)estrcpy(name, np->protoname);
	io_gds_name(HDR_LIBNAME, io_gds_makename(name), HDR_M_ASCII);
	io_gds_int2(HDR_N_UNITS);
	io_gds_int2(HDR_UNITS);
	io_gds_int4_arr(units, 4);
}

CHAR *io_gds_uniquename(NODEPROTO *np)
{
	static CHAR name[NAMSIZE];
	REGISTER INTBIG len, index, i, newtotal;
	REGISTER CHAR **newlist;

	(void)estrcpy(name, io_gds_makename(np->protoname));
	if (np->newestversion != np)
	{
		len = strlen(name);
		sprintf(&name[len], x_("_%ld"), np->version);
	}

	/* see if the name is unique */
	len = strlen(name);
	for(index = 1; ; index++)
	{
		for(i=0; i<io_gdscellnamecount; i++)
			if (namesame(name, io_gdscellnames[i]) == 0) break;
		if (i >= io_gdscellnamecount) break;
		sprintf(&name[len], x_("_%ld"), index);
	}

	/* add this name to the list */
	if (io_gdscellnamecount >= io_gdscellnametotal)
	{
		newtotal = io_gdscellnametotal * 2;
		if (io_gdscellnamecount >= newtotal) newtotal = io_gdscellnamecount + 10;
		newlist = (CHAR **)emalloc(newtotal * (sizeof (CHAR *)), io_tool->cluster);
		if (newlist == 0) return("");
		for(i=0; i<io_gdscellnametotal; i++)
			newlist[i] = io_gdscellnames[i];
		for(i=io_gdscellnametotal; i<newtotal; i++) newlist[i] = 0;
		if (io_gdscellnametotal > 0) efree((CHAR *)io_gdscellnames);
		io_gdscellnames = newlist;
		io_gdscellnametotal = newtotal;
	}
	if (io_gdscellnames[io_gdscellnamecount] != 0)
		efree((CHAR *)io_gdscellnames[io_gdscellnamecount]);
	allocstring(&io_gdscellnames[io_gdscellnamecount], name, io_tool->cluster);
	io_gdscellnamecount++;
	return(name);
}

void io_gds_bgnstr(NODEPROTO *np)
{
	time_t cdate, rdate;

	io_gds_header(HDR_BGNSTR, 0);
	cdate = (time_t)np->creationdate;
	io_gds_date(&cdate);
	rdate = (time_t)np->revisiondate;
	io_gds_date(&rdate);

	io_gds_name(HDR_STRNAME, (CHAR *)np->temp1, HDR_M_STRNAME);
}

/* Utilities to put the data into the output file */

/* Output date array, converted from a date integer */
void io_gds_date(time_t *val)
{
	CHAR buffer[28], save;
	INTSML date[6];

	(void)estrcpy(buffer, timetostring(*val));
	date[0] = (INTSML)myatoi(&buffer[20]);      /* year */
	save = buffer[7];   buffer[7] = 0;
	date[1] = (INTSML)parsemonth(&buffer[4]);	/* month */
	buffer[7] = save;
	date[2] = (INTSML)myatoi(&buffer[8]);       /* day */
	date[3] = (INTSML)myatoi(&buffer[11]);      /* hours */
	date[4] = (INTSML)myatoi(&buffer[14]);      /* minutes */
	date[5] = (INTSML)myatoi(&buffer[17]);      /* seconds */
	io_gds_int2_arr(date, 6);
}

/*
 * Write a simple header, with a fixed length
 * Enter with the header as argument, the routine will output
 * the count, the header, and the argument (if present) in p1.
 */
void io_gds_header(INTBIG header, INTBIG p1)
{
	INTBIG type, count;

	/* func = (header>>8) & BYTEMASK; */
	type = header & BYTEMASK;
	if (type == DTYP_NONE) count = 4; else
		switch(header)
	{
		case HDR_HEADER:
		case HDR_LAYER:
		case HDR_DATATYPE:
		case HDR_TEXTTYPE:
		case HDR_STRANS:
		case HDR_PRESENTATION:
			count = 6;
			break;
		case HDR_BGNSTR:
		case HDR_BGNLIB:
			count = HDR_N_BGNLIB;
			break;
		case HDR_UNITS:
			count = HDR_N_UNITS;
			break;
		default:
			ttyputerr(_("No entry for header 0x%x"), header);
			return;
	}
	io_gds_int2((INTSML)count);
	io_gds_int2((INTSML)header);
	if (type == DTYP_NONE) return;
	if (count == 6) io_gds_int2((INTSML)p1);
	if (count == 8) io_gds_int4(p1);
}

/*
 * Add a name (STRNAME, LIBNAME, etc.) to the file. The header
 * to be used is in header; the string starts at p1
 * if there is an odd number of bytes, then output the 0 at
 * the end of the string as a pad. The maximum length of string is "max"
 */
void io_gds_name(INTBIG header, CHAR *p1, INTBIG max)
{
	INTBIG count;

	count = mini(estrlen(p1), max);
	if ((count&1) != 0) count++;
	io_gds_int2((INTSML)(count+4));
	io_gds_int2((INTSML)header);
	io_gds_string(p1, count);
}

/* Output an angle as part of a STRANS */
void io_gds_angle(INTBIG ang)
{
	double gdfloat;
	static INTBIG units[2];

	gdfloat = (double)ang / 10.0;
	io_gds_int2(HDR_N_ANGLE);
	io_gds_int2(HDR_ANGLE);
	io_gds_cvfloat8(&gdfloat, (UINTBIG *)&units[0], (UINTBIG *)&units[1]);
	io_gds_int4_arr(units, 2);
}

/* Output a magnification as part of a STRANS */
void io_gds_mag(double scale)
{
	static INTBIG units[2];

	io_gds_int2(HDR_N_MAG);
	io_gds_int2(HDR_MAG);
	io_gds_cvfloat8(&scale, (UINTBIG *)&units[0], (UINTBIG *)&units[1]);
	io_gds_int4_arr(units, 2);
}

/* Output the pairs of XY points to the file  */
void io_gds_boundary(POLYGON *poly)
{
	INTBIG count, i, j, sofar;
	REGISTER INTBIG *xv, *yv;
	INTBIG lx, hx, ly, hy;
	POLYGON *side1, *side2;

	if (poly->count > MAXPOINTS)
	{
		getbbox(poly, &lx, &hx, &ly, &hy);
		if (hx-lx > hy-ly)
		{
			if (polysplitvert((lx+hx)/2, poly, &side1, &side2)) return;
		} else
		{
			if (polysplithoriz((ly+hy)/2, poly, &side1, &side2)) return;
		}
		io_gds_boundary(side1);
		io_gds_boundary(side2);
		freepolygon(side1);
		freepolygon(side2);
		return;
	}

	xv = poly->xv;   yv = poly->yv;
	count = poly->count;
	for(;;)
	{
		/* look for a closed section */
		for(sofar=1; sofar<count; sofar++)
			if (xv[sofar] == xv[0] && yv[sofar] == yv[0]) break;
		if (sofar < count) sofar++;

		io_gds_header(HDR_BOUNDARY, 0);
		io_gds_header(HDR_LAYER, io_gds_curlayer);
		io_gds_header(HDR_DATATYPE, 0);
		io_gds_int2((INTSML)(8 * (sofar+1) + 4));
		io_gds_int2(HDR_XY);
		for (i = 0; i <= sofar; i++)
		{
			if (i == sofar) j = 0; else j = i;
			io_gds_int4(rounddouble(io_gds_scale*(double)xv[j]));
			io_gds_int4(rounddouble(io_gds_scale*(double)yv[j]));
		}
		io_gds_header(HDR_ENDEL, 0);
		if (sofar >= count) break;
		count -= sofar;
		xv = &xv[sofar];
		yv = &yv[sofar];
	}
}

void io_gds_path(POLYGON *poly)
{
	INTBIG count, i;

	io_gds_header(HDR_PATH, 0);
	io_gds_header(HDR_LAYER, io_gds_curlayer);
	io_gds_header(HDR_DATATYPE, 0);
	count = 8 * poly->count + 4;
	io_gds_int2((INTSML)count);
	io_gds_int2(HDR_XY);
	for (i = 0; i < poly->count; i ++)
	{
		io_gds_int4(rounddouble(io_gds_scale*(double)poly->xv[i]));
		io_gds_int4(rounddouble(io_gds_scale*(double)poly->yv[i]));
	}
	io_gds_header(HDR_ENDEL, 0);
}

/*  Primitive output routines : */

/* Add one byte to the file io_gds_outfd */
void io_gds_byte(UCHAR1 val)
{
	io_dbuffer[io_gds_pos++] = val;
	if (io_gds_pos >= DSIZE)
	{
		(void)xfwrite(io_dbuffer, DSIZE, 1, io_gds_outfd);
		io_gds_block++;
		io_gds_pos = 0;
	}
}

/* Add a 2-byte integer */
void io_gds_int2(INTSML val)
{
	io_gds_byte((UCHAR1)((val>>8)&BYTEMASK));
	io_gds_byte((UCHAR1)(val&BYTEMASK));
}

/* Four byte integer */
void io_gds_int4(INTBIG val)
{
	io_gds_int2((INTSML)(val>>16));
	io_gds_int2((INTSML)val);
}

/* Array of 2 byte integers in array ptr, count n */
void io_gds_int2_arr(INTSML *ptr, INTBIG n)
{
	INTBIG i;

	for (i = 0; i < n; i++) io_gds_int2(ptr[i]);
}

/* Array of 4-byte integers or floating numbers in array  ptr, count n */
void io_gds_int4_arr(INTBIG *ptr, INTBIG n)
{
	INTBIG i;

	for (i = 0; i < n; i++) io_gds_int4(ptr[i]);
}

/*
 * String of n bytes, starting at ptr
 * Revised 90-11-23 to convert to upper case (SRP)
 */
void io_gds_string(CHAR *ptr, INTBIG n)
{
	INTBIG i;

	if ((io_gds_outputstate[1]&GDSOUTUC) != 0)
	{
		/* convert to upper case */
		for (i = 0; i < n; i++)
			io_gds_byte((UCHAR1)toupper(ptr[i]));
	} else
	{
		for (i = 0; i < n; i++)
			io_gds_byte((UCHAR1)ptr[i]);
	}
}

/*
 * Write out a merged polygon created by the merging algorithm
 * we have to scan it to make sure that it is closed
 * We assume that it does not exceed 200 points.
 */
void io_gds_write_polygon(INTBIG layer, TECHNOLOGY *tech, INTBIG *xbuf, INTBIG *ybuf,
	INTBIG count)
{
	static POLYGON *gds_poly = NOPOLYGON;
	INTBIG i;

	/* check the number of points on the polygon */
	if (count > io_gds_polypoints)
		ttyputerr(_("WARNING: Polygon has too many points (%ld)"), count);

	(void)needstaticpolygon(&gds_poly, 4, io_tool->cluster);
	if (count > gds_poly->limit) (void)extendpolygon(gds_poly, count);
	gds_poly->count = count;
	for (i = 0; i < count; i++)
	{
		gds_poly->xv[i] = xbuf[i];
		gds_poly->yv[i] = ybuf[i];
	}
	io_gds_set_layer(layer);
	io_gds_boundary(gds_poly);       /* Now write it out */
}

/*
 * function to create proper GDSII names with restricted character set
 * from input string str.
 * Uses only 'A'-'Z', '_', $, ?, and '0'-'9'
 */
CHAR *io_gds_makename(CHAR *str)
{
	CHAR *k, ch;
	REGISTER void *infstr;

	/* filter the name string for the GDS output cell */
	infstr = initinfstr();
	for (k = str; *k != 0; k++)
	{
		ch = *k;
		if ((io_gds_outputstate[1]&GDSOUTUC) != 0)
			ch = toupper(ch);
		if (ch != '$' && !isdigit(ch) && ch != '?' && !isalpha(ch))
			ch = '_';
		addtoinfstr(infstr, ch);
	}
	return(returninfstr(infstr));
}

/*
 * Create 8-byte GDS representation of a floating point number
 */
void io_gds_cvfloat8(double *a, UINTBIG *m, UINTBIG *n)
{
	double temp, top, frac;
	BOOLEAN negsign;
	INTBIG exponent, i;
	UINTBIG highmantissa;

	/* handle default */
	if (*a == 0)
	{
		*m = 0x40000000;
		*n = 0;
		return;
	}

	/* identify sign */
	temp = *a;
	if (temp < 0)
	{
		negsign = TRUE;
		temp = -temp;
	} else negsign = FALSE;

	/* establish the excess-64 exponent value */
	exponent = 64;

	/* scale the exponent and mantissa */
	for (; temp < 0.0625 && exponent > 0; exponent--) temp *= 16.0;

	if (exponent == 0) ttyputerr(_("Exponent underflow"));

	for (; temp >= 1 && exponent < 128; exponent++) temp /= 16.0;

	if (exponent > 127) ttyputerr(_("Exponent overflow"));

	/* set the sign */
	if (negsign) exponent |= 0x80;

	/* convert temp to 7-byte binary integer */
	top = temp;
	for (i = 0; i < 24; i++) top *= 2;
	highmantissa = (UINTBIG)top;
	frac = top - highmantissa;
	for (i = 0; i < 32; i++) frac *= 2;
	*n = (UINTBIG)frac;   *m = highmantissa | (exponent<<24);
}

/* GDS Options */
static DIALOGITEM io_gdsoptionsdialogitems[] =
{
 /*  1 */ {0, {308,140,332,212}, BUTTON, N_("OK")},
 /*  2 */ {0, {308,32,332,104}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,296,232}, SCROLL, x_("")},
 /*  4 */ {0, {8,240,24,336}, MESSAGE, N_("GDS Layer(s):")},
 /*  5 */ {0, {8,340,24,444}, EDITTEXT, x_("")},
 /*  6 */ {0, {80,240,96,464}, CHECK, N_("Input Includes Text")},
 /*  7 */ {0, {104,240,120,464}, CHECK, N_("Input Expands Cells")},
 /*  8 */ {0, {128,240,144,464}, CHECK, N_("Input Instantiates Arrays")},
 /*  9 */ {0, {176,240,192,464}, CHECK, N_("Output Merges Boxes")},
 /* 10 */ {0, {272,240,288,428}, MESSAGE, N_("Output Arc Conversion:")},
 /* 11 */ {0, {296,248,312,424}, MESSAGE, N_("Maximum arc angle:")},
 /* 12 */ {0, {296,428,312,496}, EDITTEXT, x_("")},
 /* 13 */ {0, {320,248,336,424}, MESSAGE, N_("Maximum arc sag:")},
 /* 14 */ {0, {320,428,336,496}, EDITTEXT, x_("")},
 /* 15 */ {0, {152,240,168,464}, CHECK, N_("Input Ignores Unknown Layers")},
 /* 16 */ {0, {248,240,264,424}, MESSAGE, N_("Output default text layer:")},
 /* 17 */ {0, {248,428,264,496}, EDITTEXT, x_("")},
 /* 18 */ {0, {32,256,48,336}, MESSAGE, N_("Pin layer:")},
 /* 19 */ {0, {32,340,48,444}, EDITTEXT, x_("")},
 /* 20 */ {0, {56,256,72,336}, MESSAGE, N_("Text layer:")},
 /* 21 */ {0, {56,340,72,444}, EDITTEXT, x_("")},
 /* 22 */ {0, {200,240,216,464}, CHECK, N_("Output Writes Export Pins")},
 /* 23 */ {0, {224,240,240,464}, CHECK, N_("Output All Upper Case")}
};
static DIALOG io_gdsoptionsdialog = {{50,75,395,581}, N_("GDS Options"), 0, 23, io_gdsoptionsdialogitems, 0, 0};

/* special items for the "GDS Options" dialog: */
#define DGDO_LAYERLIST       3		/* Layer list (scroll list) */
#define DGDO_NEWLAYER        5		/* New layer (edit text) */
#define DGDO_IINCLUDETEXT    6		/* Input Includes Text (check) */
#define DGDO_IEXPANDCELL     7		/* Input Expands Cells (check) */
#define DGDO_IINSTARRAY      8		/* Input Instantiates Arrays (check) */
#define DGDO_OMERGEBOX       9		/* Output Merges Boxes (check) */
#define DGDO_OARCANGLE      12		/* Output Arc Angle (edit text) */
#define DGDO_OARCSAG        14		/* Output Arc Sag (edit text) */
#define DGDO_IIGNOREUNKNOWN 15		/* Input Ignores unknown layers (check) */
#define DGDO_EXPORTLAYER    17		/* Output default text layer (edit text) */
#define DGDO_PINLAYER       19		/* Pin layer (edit text) */
#define DGDO_TEXTLAYER      21		/* Text layer (edit text) */
#define DGDO_WRITEEXPORTPIN 22		/* Write Export Pins (check) */
#define DGDO_OUPPERCASE     23		/* Output Upper Case (check) */

void io_gdsoptionsdlog(void)
{
	REGISTER INTBIG i, v, itemHit, *curstate, newres, newsag, numberschanged,
		explayer, varlen;
	INTBIG arcres, arcsag, newstate[NUMIOSTATEBITWORDS];
	CHAR buf[50], **layernumbers, **pinnumbers, **textnumbers, *val, *pinval, *textval;
	REGISTER VARIABLE *gdsvar, *expvar;
	REGISTER void *infstr, *dia;

	dia = DiaInitDialog(&io_gdsoptionsdialog);
	if (dia == 0) return;
	DiaInitTextDialog(dia, DGDO_LAYERLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0,
		SCSELMOUSE|SCREPORT);
	layernumbers = (CHAR **)emalloc(el_curtech->layercount * (sizeof (CHAR *)), el_tempcluster);
	if (layernumbers == 0) return;
	pinnumbers = (CHAR **)emalloc(el_curtech->layercount * (sizeof (CHAR *)), el_tempcluster);
	if (pinnumbers == 0) return;
	textnumbers = (CHAR **)emalloc(el_curtech->layercount * (sizeof (CHAR *)), el_tempcluster);
	if (textnumbers == 0) return;
	gdsvar = getval((INTBIG)el_curtech, VTECHNOLOGY, -1, x_("IO_gds_layer_numbers"));
	if (gdsvar != NOVARIABLE && (gdsvar->type&VISARRAY) == 0) gdsvar = NOVARIABLE;
	if (gdsvar != NOVARIABLE) varlen = getlength(gdsvar);
	for(i=0; i<el_curtech->layercount; i++)
	{
		val = pinval = textval = x_("");
		if (gdsvar != NOVARIABLE && i < varlen)
		{
			if ((gdsvar->type&VTYPE) == VSTRING)
			{
				io_gds_parselayerstring(((CHAR **)gdsvar->addr)[i], &val, &pinval, &textval);
			} else
			{
				v = ((INTBIG *)gdsvar->addr)[i];
				if (v >= 0)
				{
					esnprintf(buf, 50, x_("%ld"), v);
					val = buf;
				}
			}
		}
		allocstring(&layernumbers[i], val, io_tool->cluster);
		allocstring(&pinnumbers[i], pinval, io_tool->cluster);
		allocstring(&textnumbers[i], textval, io_tool->cluster);
		infstr = initinfstr();
		formatinfstr(infstr, x_("%s (%s"), layername(el_curtech, i), layernumbers[i]);
		if (*pinnumbers[i] != 0) formatinfstr(infstr, x_(",%sP"), pinnumbers[i]);
		if (*textnumbers[i] != 0) formatinfstr(infstr, x_(",%sT"), textnumbers[i]);
		addstringtoinfstr(infstr, x_(")"));
		DiaStuffLine(dia, DGDO_LAYERLIST, returninfstr(infstr));
	}
	DiaSelectLine(dia, DGDO_LAYERLIST, 0);
	DiaSetText(dia, DGDO_NEWLAYER, layernumbers[0]);
	DiaSetText(dia, DGDO_PINLAYER, pinnumbers[0]);
	DiaSetText(dia, DGDO_TEXTLAYER, textnumbers[0]);

	expvar = getval((INTBIG)el_curtech, VTECHNOLOGY, VINTEGER, x_("IO_gds_export_layer"));
	if (expvar == NOVARIABLE) explayer = 230; else
		explayer = expvar->addr;
	esnprintf(buf, 50, x_("%ld"), explayer);
	DiaSetText(dia, DGDO_EXPORTLAYER, buf);

	curstate = io_getstatebits();
	for(i=0; i<NUMIOSTATEBITWORDS; i++) newstate[i] = curstate[i];
	if ((curstate[0]&GDSINTEXT) != 0) DiaSetControl(dia, DGDO_IINCLUDETEXT, 1);
	if ((curstate[0]&GDSINEXPAND) != 0) DiaSetControl(dia, DGDO_IEXPANDCELL, 1);
	if ((curstate[0]&GDSINARRAYS) != 0) DiaSetControl(dia, DGDO_IINSTARRAY, 1);
	if ((curstate[0]&GDSOUTMERGE) != 0) DiaSetControl(dia, DGDO_OMERGEBOX, 1);
	if ((curstate[0]&GDSINIGNOREUKN) != 0) DiaSetControl(dia, DGDO_IIGNOREUNKNOWN, 1);
	if ((curstate[1]&GDSOUTPINS) != 0) DiaSetControl(dia, DGDO_WRITEEXPORTPIN, 1);
	if ((curstate[1]&GDSOUTUC) != 0) DiaSetControl(dia, DGDO_OUPPERCASE, 1);
	getcontoursegmentparameters(&arcres, &arcsag);
	DiaSetText(dia, DGDO_OARCANGLE, frtoa(arcres*WHOLE/10));
	DiaSetText(dia, DGDO_OARCSAG, latoa(arcsag, 0));

	/* loop until done */
	numberschanged = 0;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DGDO_IINCLUDETEXT || itemHit == DGDO_IEXPANDCELL ||
			itemHit == DGDO_IINSTARRAY || itemHit == DGDO_OMERGEBOX ||
			itemHit == DGDO_IIGNOREUNKNOWN || itemHit == DGDO_WRITEEXPORTPIN ||
			itemHit == DGDO_OUPPERCASE)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
		if (itemHit == DGDO_LAYERLIST)
		{
			i = DiaGetCurLine(dia, DGDO_LAYERLIST);
			DiaSetText(dia, DGDO_NEWLAYER, layernumbers[i]);
			DiaSetText(dia, DGDO_PINLAYER, pinnumbers[i]);
			DiaSetText(dia, DGDO_TEXTLAYER, textnumbers[i]);
			continue;
		}
		if (itemHit == DGDO_NEWLAYER || itemHit == DGDO_PINLAYER ||
			itemHit == DGDO_TEXTLAYER)
		{
			i = DiaGetCurLine(dia, DGDO_LAYERLIST);
			if (itemHit == DGDO_NEWLAYER)
			{
				val = DiaGetText(dia, DGDO_NEWLAYER);
				if (namesame(val, layernumbers[i]) == 0) continue;
				reallocstring(&layernumbers[i], val, io_tool->cluster);
			} else if (itemHit == DGDO_PINLAYER)
			{
				val = DiaGetText(dia, DGDO_PINLAYER);
				if (namesame(val, pinnumbers[i]) == 0) continue;
				reallocstring(&pinnumbers[i], val, io_tool->cluster);
			} else
			{
				val = DiaGetText(dia, DGDO_TEXTLAYER);
				if (namesame(val, textnumbers[i]) == 0) continue;
				reallocstring(&textnumbers[i], val, io_tool->cluster);
			} 
			numberschanged++;
			infstr = initinfstr();
			formatinfstr(infstr, x_("%s (%s"), layername(el_curtech, i), layernumbers[i]);
			if (*pinnumbers[i] != 0) formatinfstr(infstr, x_(",%sP"), pinnumbers[i]);
			if (*textnumbers[i] != 0) formatinfstr(infstr, x_(",%sT"), textnumbers[i]);
			addstringtoinfstr(infstr, x_(")"));
			DiaSetScrollLine(dia, DGDO_LAYERLIST, i, returninfstr(infstr));
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		if (DiaGetControl(dia, DGDO_IINCLUDETEXT) != 0) newstate[0] |= GDSINTEXT; else
			newstate[0] &= ~GDSINTEXT;
		if (DiaGetControl(dia, DGDO_IEXPANDCELL) != 0) newstate[0] |= GDSINEXPAND; else
			newstate[0] &= ~GDSINEXPAND;
		if (DiaGetControl(dia, DGDO_IINSTARRAY) != 0) newstate[0] |= GDSINARRAYS; else
			newstate[0] &= ~GDSINARRAYS;
		if (DiaGetControl(dia, DGDO_OMERGEBOX) != 0) newstate[0] |= GDSOUTMERGE; else
			newstate[0] &= ~GDSOUTMERGE;
		if (DiaGetControl(dia, DGDO_IIGNOREUNKNOWN) != 0) newstate[0] |= GDSINIGNOREUKN; else
			newstate[0] &= ~GDSINIGNOREUKN;
		if (DiaGetControl(dia, DGDO_WRITEEXPORTPIN) != 0) newstate[1] |= GDSOUTPINS; else
			newstate[1] &= ~GDSOUTPINS;
		if (DiaGetControl(dia, DGDO_OUPPERCASE) != 0) newstate[1] |= GDSOUTUC; else
			newstate[1] &= ~GDSOUTUC;
		for(i=0; i<NUMIOSTATEBITWORDS; i++) if (curstate[i] != newstate[i]) break;
		if (i < NUMIOSTATEBITWORDS) io_setstatebits(newstate);
		if (numberschanged != 0)
		{
			for(i=0; i<el_curtech->layercount; i++)
			{
				if (*pinnumbers[i] != 0 || *textnumbers[i] != 0)
				{
					infstr = initinfstr();
					addstringtoinfstr(infstr, layernumbers[i]);
					if (*pinnumbers[i] != 0)
						formatinfstr(infstr, x_(",%sP"), pinnumbers[i]);
					if (*textnumbers[i] != 0)
						formatinfstr(infstr, x_(",%sT"), textnumbers[i]);
					reallocstring(&layernumbers[i], returninfstr(infstr), io_tool->cluster);
				}
			}
			setval((INTBIG)el_curtech, VTECHNOLOGY, x_("IO_gds_layer_numbers"),
				(INTBIG)layernumbers, VSTRING|VISARRAY|
					(el_curtech->layercount<<VLENGTHSH));
		}
		newres = atofr(DiaGetText(dia, DGDO_OARCANGLE)) * 10 / WHOLE;
		newsag = atola(DiaGetText(dia, DGDO_OARCSAG), 0);
		if (newres != arcres || newsag != arcsag)
			setcontoursegmentparameters(newres, newsag);
		i = myatoi(DiaGetText(dia, DGDO_EXPORTLAYER));
		if (i != explayer)
			setval((INTBIG)el_curtech, VTECHNOLOGY, x_("IO_gds_export_layer"), i, VINTEGER);
	}
	for(i=0; i<el_curtech->layercount; i++)
	{
		efree((CHAR *)pinnumbers[i]);
		efree((CHAR *)textnumbers[i]);
	}
	efree((CHAR *)layernumbers);
	efree((CHAR *)pinnumbers);
	efree((CHAR *)textnumbers);
	DiaDoneDialog(dia);
}

void io_gds_parselayerstring(CHAR *string, CHAR **val, CHAR **pinval, CHAR **textval)
{
	REGISTER void *infstrv, *infstrp, *infstrt;
	REGISTER BOOLEAN havev, havep, havet;
	REGISTER INTBIG number;

	infstrv = initinfstr();
	infstrp = initinfstr();
	infstrt = initinfstr();
	havev = havep = havet = FALSE;
	for(;;)
	{
		while (*string == ' ') string++;
		if (*string == 0) break;
		number = myatoi(string);
		while (isdigit(*string)) string++;
		if (tolower(*string) == 't')
		{
			if (havet) addtoinfstr(infstrt, ',');
			formatinfstr(infstrt, x_("%ld"), number);
			havet = TRUE;
			string++;
		} else if (tolower(*string) == 'p')
		{
			if (havep) addtoinfstr(infstrp, ',');
			formatinfstr(infstrp, x_("%ld"), number);
			havep = TRUE;
			string++;
		} else
		{
			if (havev) addtoinfstr(infstrv, ',');
			formatinfstr(infstrv, x_("%ld"), number);
			havev = TRUE;
		}
		while (*string == ' ') string++;
		if (*string == 0) break;
		if (*string == ',') string++;
	}
	*val = returninfstr(infstrv);
	*pinval = returninfstr(infstrp);
	*textval = returninfstr(infstrt);
}
