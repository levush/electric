/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tecart.c
 * Artwork technology description
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
#include "tech.h"
#include "tecart.h"
#include "efunction.h"
#include <math.h>

#define SPLINEGRAIN   20		/* number of line segments in each spline section */

/* the options table */
static KEYWORD artopt[] =
{
	{x_("arrows-filled"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("arrows-outline"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP art_parse = {artopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("ARTWORK option"), M_("show current option")};

typedef struct
{
	INTBIG col, bits;
	VARIABLE *colorvar, *patternvar;
} ARTPOLYLOOP;

ARTPOLYLOOP art_oneprocpolyloop;

       TECHNOLOGY *art_tech;
static INTBIG      art_state;			/* state bits */
       INTBIG      art_messagekey;		/* key for "ART_message" */
       INTBIG      art_colorkey;		/* key for "ART_color" */
       INTBIG      art_degreeskey;		/* key for "ART_degrees" */
       INTBIG      art_patternkey;		/* key for "ART_pattern" */
       NODEPROTO  *art_pinprim;
       NODEPROTO  *art_openedpolygonprim;
       NODEPROTO  *art_boxprim;
       NODEPROTO  *art_crossedboxprim;
       NODEPROTO  *art_filledboxprim;
       NODEPROTO  *art_circleprim;
       NODEPROTO  *art_thickcircleprim;
       NODEPROTO  *art_filledcircleprim;
       NODEPROTO  *art_splineprim;
       NODEPROTO  *art_triangleprim;
       NODEPROTO  *art_filledtriangleprim;
       NODEPROTO  *art_arrowprim;
       NODEPROTO  *art_openeddottedpolygonprim;
       NODEPROTO  *art_openeddashedpolygonprim;
       NODEPROTO  *art_openedthickerpolygonprim;
       NODEPROTO  *art_closedpolygonprim;
       NODEPROTO  *art_filledpolygonprim;
       ARCPROTO   *art_solidarc;
       ARCPROTO   *art_dottedarc;
       ARCPROTO   *art_dashedarc;
       ARCPROTO   *art_thickerarc;

/* prototypes for local routines */
static void   art_getgraphics(INTBIG, INTBIG, ARTPOLYLOOP*);
static void   art_fillellipse(INTBIG, INTBIG, INTBIG, INTBIG, double, double, INTBIG, POLYGON*);
static void   art_fillspline(INTBIG *points, INTBIG count, INTBIG cx, INTBIG cy, POLYGON *poly, INTBIG steps);
static void   art_setstate(INTBIG newstate);
static void   art_fillnodebox(NODEINST *ni, INTBIG box, POLYGON *poly);
static INTBIG art_intnodepolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win, POLYLOOP *pl,
				ARTPOLYLOOP *artpl);
static void   art_intshapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl, ARTPOLYLOOP *artpl);
static INTBIG art_intarcpolys(ARCINST *ai, WINDOWPART *win, POLYLOOP *pl, ARTPOLYLOOP *artpl);
static void   art_intshapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly, POLYLOOP *pl, ARTPOLYLOOP *artpl);

/******************** LAYERS ********************/

#define MAXLAYERS  1		/* total layers below */
#define LOPAQUE    0		/* solid graphics     */

static GRAPHICS art_o_lay = {LAYERO, MENGLY, SOLIDC, SOLIDC,
/* graphics layer */		{0xFFFF,0xFFFF,0xFFFF,0xFFFF,
							0xFFFF,0xFFFF,0xFFFF,0xFFFF,
							0xFFFF,0xFFFF,0xFFFF,0xFFFF,
							0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};
static GRAPHICS art_st_lay= {LAYERO, MENGLY, PATTERNED, PATTERNED,
							{0xFFFF,0xFFFF,0xFFFF,0xFFFF,
							0xFFFF,0xFFFF,0xFFFF,0xFFFF,
							0xFFFF,0xFFFF,0xFFFF,0xFFFF,
							0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};

/* these tables must be updated together */
GRAPHICS *art_layers[MAXLAYERS+1] = {&art_o_lay, NOGRAPHICS};
static CHAR *art_layer_names[MAXLAYERS] = {x_("Graphics")};
static CHAR *art_dxf_layers[MAXLAYERS] = {x_("OBJECT")};
static INTBIG art_layer_function[MAXLAYERS] = {LFART};
static CHAR *art_layer_letters[MAXLAYERS] = {x_("g")};
static CHAR *art_gds_layers[MAXLAYERS] = {x_("1")};

/******************** ARCS ********************/

#define ARCPROTOCOUNT  4
#define ASOLID         0
#define ADOTTED        1
#define ADASHED        2
#define ATHICKER       3

/* solid arc */
static TECH_ARCLAY art_al_s[] = {{LOPAQUE, 0, FILLED}};
static TECH_ARCS art_a_s = {
	x_("Solid"),0,ASOLID,NOARCPROTO,								/* name */
	1,art_al_s,														/* layers */
	(APNONELEC<<AFUNCTIONSH)|CANWIPE|CANCURVE|(0<<AANGLEINCSH)};	/* userbits */

/* dotted arc */
static TECH_ARCLAY art_al_d1[] = {{LOPAQUE, 0, FILLED}};
static TECH_ARCS art_a_d1 = {
	x_("Dotted"),0,ADOTTED,NOARCPROTO,								/* name */
	1,art_al_d1,													/* layers */
	(APNONELEC<<AFUNCTIONSH)|CANWIPE|CANCURVE|(0<<AANGLEINCSH)};	/* userbits */

/* dashed arc */
static TECH_ARCLAY art_al_d2[] = {{LOPAQUE, 0, FILLED}};
static TECH_ARCS art_a_d2 = {
	x_("Dashed"),0,ADASHED,NOARCPROTO,								/* name */
	1,art_al_d2,													/* layers */
	(APNONELEC<<AFUNCTIONSH)|CANWIPE|CANCURVE|(0<<AANGLEINCSH)};	/* userbits */

/* thicker arc */
static TECH_ARCLAY art_al_d3[] = {{LOPAQUE, 0, FILLED}};
static TECH_ARCS art_a_d3 = {
	x_("Thicker"),0,ATHICKER,NOARCPROTO,							/* name */
	1,art_al_d3,													/* layers */
	(APNONELEC<<AFUNCTIONSH)|CANWIPE|CANCURVE|(0<<AANGLEINCSH)};	/* userbits */

TECH_ARCS *art_arcprotos[ARCPROTOCOUNT+1] = {&art_a_s, &art_a_d1,
	&art_a_d2, &art_a_d3, ((TECH_ARCS *)-1)};

/******************** PORTINST CONNECTIONS ********************/

/* these values are replaced with actual arcproto addresses */
static INTBIG art_pc_d[] = {-1, ASOLID, ADOTTED, ADASHED, ATHICKER, ALLGEN, -1};

/******************** NODES ********************/

#define NODEPROTOCOUNT 17
#define NPIN            1		/* point */
#define NBOX            2		/* rectangular box */
#define NCBOX           3		/* crossed rectangular box */
#define NFBOX           4		/* filled rectangular box */
#define NCIRCLE         5		/* circle */
#define NFCIRCLE        6		/* filled circle */
#define NSPLINE         7		/* spline curve */
#define NTRIANGLE       8		/* triangle */
#define NFTRIANGLE      9		/* filled triangle */
#define NARROW         10		/* arrow head */
#define NOPENED        11		/* opened solid polygon */
#define NOPENEDT1      12		/* opened dotted polygon */
#define NOPENEDT2      13		/* opened dashed polygon */
#define NOPENEDT3      14		/* opened thicker polygon */
#define NCLOSED        15		/* closed polygon */
#define NFILLED        16		/* filled polygon */
#define NTCIRCLE       17		/* thick circle */

#define EI   (WHOLE/8)
static INTBIG art_fullbox[8]  = {LEFTEDGE, LEFTEDGE, RIGHTEDGE, TOPEDGE};
static INTBIG art_circ[8]     = {CENTER, CENTER,     RIGHTEDGE, CENTER};
static INTBIG art_opnd[16]    = {LEFTEDGE, BOTEDGE,  -EI,0,TOPEDGE,
								EI,0,BOTEDGE,       RIGHTEDGE,TOPEDGE};
static INTBIG art_clsd[16]    = {LEFTEDGE, CENTER,   CENTER,TOPEDGE,
								RIGHTEDGE, BOTEDGE, CENTER, BOTEDGE};
static INTBIG art_tri[12]     = {LEFTEDGE, BOTEDGE,  RIGHTEDGE, BOTEDGE,
								CENTER, TOPEDGE};
static INTBIG art_arr[12]     = {LEFTEDGE, TOPEDGE,  RIGHTEDGE, CENTER,
								LEFTEDGE, BOTEDGE};
static INTBIG art_arr1[12]    = {LEFTEDGE, TOPEDGE,  RIGHTEDGE, CENTER,
								CENTER, CENTER};
static INTBIG art_arr2[12]    = {LEFTEDGE, BOTEDGE,  RIGHTEDGE, CENTER,
								CENTER, CENTER};

/* Pin */
static TECH_PORTS art_pin_p[] = {					/* ports */
	{art_pc_d, x_("site"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON art_pin_l[] = {					/* layers */
	{LOPAQUE, -1, 2, DISC, POINTS, art_circ}};
static TECH_NODES art_pin = {
	x_("Pin"),NPIN,NONODEPROTO,						/* name */
	K1,K1,											/* size */
	1,art_pin_p,									/* ports */
	1,art_pin_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE,					/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Box */
static TECH_PORTS art_box_p[] = {					/* ports */
	{art_pc_d, x_("box"),    NOPORTPROTO, (180<<PORTANGLESH),
		LEFTEDGE, TOPEDGE, RIGHTEDGE, BOTEDGE}};
static TECH_POLYGON art_box_l[] = {					/* layers */
	{LOPAQUE, -1, 4, CLOSEDRECT, BOX, art_fullbox}};
static TECH_NODES art_box = {
	x_("Box"),NBOX,NONODEPROTO,						/* name */
	K6,K6,											/* size */
	1,art_box_p,									/* ports */
	1,art_box_l,									/* layers */
	(NPART<<NFUNCTIONSH)|NEDGESELECT,				/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Crossed Box */
static TECH_PORTS art_cbox_p[] = {					/* ports */
	{art_pc_d, x_("fbox"),    NOPORTPROTO, (180<<PORTANGLESH),
		LEFTEDGE, TOPEDGE, RIGHTEDGE, BOTEDGE}};
static TECH_POLYGON art_cbox_l[] = {				/* layers */
	{LOPAQUE, -1, 4, CROSSED, BOX, art_fullbox}};
static TECH_NODES art_cbox = {
	x_("Crossed-Box"),NCBOX,NONODEPROTO,			/* name */
	K6,K6,											/* size */
	1,art_cbox_p,									/* ports */
	1,art_cbox_l,									/* layers */
	(NPART<<NFUNCTIONSH),							/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Filled Box */
static TECH_PORTS art_fbox_p[] = {					/* ports */
	{art_pc_d, x_("fbox"),    NOPORTPROTO, (180<<PORTANGLESH),
		LEFTEDGE, TOPEDGE, RIGHTEDGE, BOTEDGE}};
static TECH_POLYGON art_fbox_l[] = {				/* layers */
	{LOPAQUE, -1, 4, FILLEDRECT, BOX, art_fullbox}};
static TECH_NODES art_fbox = {
	x_("Filled-Box"),NFBOX,NONODEPROTO,				/* name */
	K6,K6,											/* size */
	1,art_fbox_p,									/* ports */
	1,art_fbox_l,									/* layers */
	(NPART<<NFUNCTIONSH)|NEDGESELECT,				/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Circle (or Arc or Ellipse) */
static TECH_PORTS art_circle_p[] = {				/* ports */
	{art_pc_d, x_("site"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON art_circle_l[] = {				/* layers */
	{LOPAQUE, -1, 2, CIRCLE, POINTS, art_circ}};
static TECH_NODES art_circle = {
	x_("Circle"),NCIRCLE,NONODEPROTO,				/* name */
	K6,K6,											/* size */
	1,art_circle_p,									/* ports */
	1,art_circle_l,									/* layers */
	(NPART<<NFUNCTIONSH)|NEDGESELECT,				/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Filled Circle */
static TECH_PORTS art_fcircle_p[] = {				/* ports */
	{art_pc_d, x_("site"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON art_fcircle_l[] = {				/* layers */
	{LOPAQUE, -1, 2, DISC, POINTS, art_circ}};
static TECH_NODES art_fcircle = {
	x_("Filled-Circle"),NFCIRCLE,NONODEPROTO,		/* name */
	K6,K6,											/* size */
	1,art_fcircle_p,								/* ports */
	1,art_fcircle_l,								/* layers */
	(NPART<<NFUNCTIONSH)|NSQUARE|NEDGESELECT,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Spline Curve */
static TECH_PORTS art_spline_p[] = {				/* ports */
	{art_pc_d, x_("site"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON art_spline_l[] = {				/* layers */
	{LOPAQUE, -1, 4, OPENED, POINTS, art_opnd}};
static TECH_NODES art_spline = {
	x_("Spline"),NSPLINE,NONODEPROTO,				/* name */
	K6,K6,											/* size */
	1,art_spline_p,									/* ports */
	1,art_spline_l,									/* layers */
	(NPART<<NFUNCTIONSH)|HOLDSTRACE|NEDGESELECT,	/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Triangle */
static TECH_PORTS art_triangle_p[] = {				/* ports */
	{art_pc_d, x_("triangle"),    NOPORTPROTO, (180<<PORTANGLESH),
		LEFTEDGE, TOPEDGE, RIGHTEDGE, BOTEDGE}};
static TECH_POLYGON art_triangle_l[] = {			/* layers */
	{LOPAQUE, -1, 3, CLOSED, POINTS, art_tri}};
static TECH_NODES art_triangle = {
	x_("Triangle"),NTRIANGLE,NONODEPROTO,			/* name */
	K6,K6,											/* size */
	1,art_triangle_p,								/* ports */
	1,art_triangle_l,								/* layers */
	(NPART<<NFUNCTIONSH)|NEDGESELECT,				/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Filled Triangle */
static TECH_PORTS art_ftriangle_p[] = {				/* ports */
	{art_pc_d, x_("ftriangle"),    NOPORTPROTO, (180<<PORTANGLESH),
		LEFTEDGE, TOPEDGE, RIGHTEDGE, BOTEDGE}};
static TECH_POLYGON art_ftriangle_l[] = {			/* layers */
	{LOPAQUE, -1, 3, FILLED, POINTS, art_tri}};
static TECH_NODES art_ftriangle = {
	x_("Filled-Triangle"),NFTRIANGLE,NONODEPROTO,	/* name */
	K6,K6,											/* size */
	1,art_ftriangle_p,								/* ports */
	1,art_ftriangle_l,								/* layers */
	(NPART<<NFUNCTIONSH)|NEDGESELECT,				/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Arrow Head */
static TECH_PORTS art_arrow_p[] = {					/* ports */
	{art_pc_d, x_("arrow"), NOPORTPROTO, (180<<PORTARANGESH),
		 RIGHTEDGE, CENTER, RIGHTEDGE, CENTER}};
static TECH_POLYGON art_arrow_l[] = {				/* layers */
	{LOPAQUE, -1, 3, OPENED, POINTS, art_arr},
	{LOPAQUE, -1, 3, FILLED, POINTS, art_arr2}};
static TECH_NODES art_arrow = {
	x_("Arrow"),NARROW,NONODEPROTO,					/* name */
	K2,K2,											/* size */
	1,art_arrow_p,									/* ports */
	1,art_arrow_l,									/* layers */
	(NPART<<NFUNCTIONSH)|NEDGESELECT,				/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Opened Solid Polygon */
static TECH_PORTS art_freeform_p[] = {				/* ports */
	{art_pc_d, x_("site"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON art_freeform_l[] = {			/* layers */
	{LOPAQUE, -1, 4, OPENED, POINTS, art_opnd}};
static TECH_NODES art_freeform = {
	x_("Opened-Polygon"),NOPENED,NONODEPROTO,		/* name */
	K6,K6,											/* size */
	1,art_freeform_p,								/* ports */
	1,art_freeform_l,								/* layers */
	(NPART<<NFUNCTIONSH)|HOLDSTRACE|NEDGESELECT,	/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Opened Dotted Polygon */
static TECH_PORTS art_freeformdot_p[] = {			/* ports */
	{art_pc_d, x_("site"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON art_freeformdot_l[] = {			/* layers */
	{LOPAQUE, -1, 4, OPENEDT1, POINTS, art_opnd}};
static TECH_NODES art_freeformdot = {
	x_("Opened-Dotted-Polygon"),NOPENEDT1,NONODEPROTO,	/* name */
	K6,K6,											/* size */
	1,art_freeformdot_p,							/* ports */
	1,art_freeformdot_l,							/* layers */
	(NPART<<NFUNCTIONSH)|HOLDSTRACE|NEDGESELECT,	/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Opened Dashed Polygon */
static TECH_PORTS art_freeformdash_p[] = {			/* ports */
	{art_pc_d, x_("site"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON art_freeformdash_l[] = {		/* layers */
	{LOPAQUE, -1, 4, OPENEDT2, POINTS, art_opnd}};
static TECH_NODES art_freeformdash = {
	x_("Opened-Dashed-Polygon"),NOPENEDT2,NONODEPROTO,	/* name */
	K6,K6,											/* size */
	1,art_freeformdash_p,							/* ports */
	1,art_freeformdash_l,							/* layers */
	(NPART<<NFUNCTIONSH)|HOLDSTRACE|NEDGESELECT,	/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Opened Thicker Polygon */
static TECH_PORTS art_freeformfdot_p[] = {			/* ports */
	{art_pc_d, x_("site"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON art_freeformfdot_l[] = {		/* layers */
	{LOPAQUE, -1, 4, OPENEDT3, POINTS, art_opnd}};
static TECH_NODES art_freeformfdot = {
	x_("Opened-Thicker-Polygon"),NOPENEDT3,NONODEPROTO,	/* name */
	K6,K6,											/* size */
	1,art_freeformfdot_p,							/* ports */
	1,art_freeformfdot_l,							/* layers */
	(NPART<<NFUNCTIONSH)|HOLDSTRACE|NEDGESELECT,	/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Closed Polygon */
static TECH_PORTS art_closed_p[] = {				/* ports */
	{art_pc_d, x_("site"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON art_closed_l[] = {				/* layers */
	{LOPAQUE, -1, 4, CLOSED, POINTS, art_clsd}};
static TECH_NODES art_closed = {
	x_("Closed-Polygon"),NCLOSED,NONODEPROTO,		/* name */
	K6,K6,											/* size */
	1,art_closed_p,									/* ports */
	1,art_closed_l,									/* layers */
	(NPART<<NFUNCTIONSH)|HOLDSTRACE|NEDGESELECT,	/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Filled Polygon */
static TECH_PORTS art_polygon_p[] = {				/* ports */
	{art_pc_d, x_("site"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON art_polygon_l[] = {				/* layers */
	{LOPAQUE, -1, 4, FILLED, POINTS, art_clsd}};
static TECH_NODES art_polygon = {
	x_("Filled-Polygon"),NFILLED,NONODEPROTO,		/* name */
	K6,K6,											/* size */
	1,art_polygon_p,								/* ports */
	1,art_polygon_l,								/* layers */
	(NPART<<NFUNCTIONSH)|HOLDSTRACE|NEDGESELECT,	/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Thick Circle (or Arc or Ellipse) */
static TECH_PORTS art_tcircle_p[] = {				/* ports */
	{art_pc_d, x_("site"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON art_tcircle_l[] = {				/* layers */
	{LOPAQUE, -1, 2, THICKCIRCLE, POINTS, art_circ}};
static TECH_NODES art_tcircle = {
	x_("Thick-Circle"),NTCIRCLE,NONODEPROTO,		/* name */
	K6,K6,											/* size */
	1,art_tcircle_p,								/* ports */
	1,art_tcircle_l,								/* layers */
	(NPART<<NFUNCTIONSH)|NEDGESELECT,				/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

TECH_NODES *art_nodeprotos[NODEPROTOCOUNT+1] = {&art_pin,
	&art_box, &art_cbox, &art_fbox, &art_circle, &art_fcircle, &art_spline,
	&art_triangle, &art_ftriangle, &art_arrow, &art_freeform, &art_freeformdot,
	&art_freeformdash, &art_freeformfdot, &art_closed, &art_polygon, &art_tcircle,
	((TECH_NODES *)-1)};

/******************** VARIABLE AGGREGATION ********************/

TECH_VARIABLES art_variables[] =
{
	/* set general information about the technology */
	{x_("TECH_layer_names"), (CHAR *)art_layer_names, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_layer_function"), (CHAR *)art_layer_function, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},

	/* set information for the USER tool */
	{x_("USER_layer_letters"), (CHAR *)art_layer_letters, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},

	/* set information for the IO tool */
	{x_("IO_dxf_layer_names"), (CHAR *)art_dxf_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("IO_gds_layer_numbers"), (CHAR *)art_gds_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{NULL, NULL, 0.0, 0}
};

/******************** ROUTINES ********************/

BOOLEAN art_initprocess(TECHNOLOGY *tech, INTBIG pass)
{
	if (pass == 0)
	{
		/* initialize the technology variable */
		art_tech = tech;
	} else if (pass == 1)
	{
		art_pinprim = getnodeproto(x_("artwork:Pin"));
		art_boxprim = getnodeproto(x_("artwork:Box"));
		art_crossedboxprim = getnodeproto(x_("artwork:Crossed-Box"));
		art_filledboxprim = getnodeproto(x_("artwork:Filled-Box"));
		art_circleprim = getnodeproto(x_("artwork:Circle"));
		art_thickcircleprim = getnodeproto(x_("artwork:Thick-Circle"));
		art_filledcircleprim = getnodeproto(x_("artwork:Filled-Circle"));
		art_splineprim = getnodeproto(x_("artwork:Spline"));
		art_triangleprim = getnodeproto(x_("artwork:Triangle"));
		art_filledtriangleprim = getnodeproto(x_("artwork:Filled-Triangle"));
		art_arrowprim = getnodeproto(x_("artwork:Arrow"));
		art_openedpolygonprim = getnodeproto(x_("artwork:Opened-Polygon"));
		art_openeddottedpolygonprim = getnodeproto(x_("artwork:Opened-Dotted-Polygon"));
		art_openeddashedpolygonprim = getnodeproto(x_("artwork:Opened-Dashed-Polygon"));
		art_openedthickerpolygonprim = getnodeproto(x_("artwork:Opened-Thicker-Polygon"));
		art_closedpolygonprim = getnodeproto(x_("artwork:Closed-Polygon"));
		art_filledpolygonprim = getnodeproto(x_("artwork:Filled-Polygon"));

		art_solidarc = getarcproto(x_("Artwork:Solid"));
		art_dottedarc = getarcproto(x_("Artwork:Dotted"));
		art_dashedarc = getarcproto(x_("Artwork:Dashed"));
		art_thickerarc = getarcproto(x_("Artwork:Thicker"));

		art_messagekey = makekey(x_("ART_message"));
		art_colorkey = makekey(x_("ART_color"));
		art_degreeskey = makekey(x_("ART_degrees"));
		art_patternkey = makekey(x_("ART_pattern"));
		art_state = 0;
		nextchangequiet();
		setvalkey((INTBIG)art_tech, VTECHNOLOGY, el_techstate_key, art_state,
			VINTEGER|VDONTSAVE);
	}
	return(FALSE);
}

void art_setmode(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG l;
	REGISTER CHAR *pp;

	if (count == 0)
	{
		if (art_arrow.layercount == 2)
			ttyputmsg(M_("Artwork arrow heads are filled")); else
				ttyputmsg(M_("Artwork arrow heads are outlines"));
		return;
	}

	l = estrlen(pp = par[0]);
	if (namesamen(pp, x_("arrows-filled"), l) == 0)
	{
		/* set the arrow heads to be fancy */
		art_setstate(art_state | ARTWORKFILLARROWHEADS);
		ttyputverbose(M_("Artwork arrow heads will be filled"));
		return;
	}
	if (namesamen(pp, x_("arrows-outline"), l) == 0)
	{
		/* set the arrow heads to be simple */
		art_setstate(art_state & ~ARTWORKFILLARROWHEADS);
		ttyputverbose(M_("Artwork arrow heads will be outline"));
		return;
	}
	ttyputbadusage(x_("technology tell artwork"));
}

INTBIG art_request(CHAR *command, va_list ap)
{
	if (namesame(command, x_("has-state")) == 0) return(1);
	if (namesame(command, x_("get-state")) == 0)
	{
		return(art_state);
	}
	if (namesame(command, x_("set-state")) == 0)
	{
		art_setstate(va_arg(ap, INTBIG));
		return(0);
	}
	return(0);
}

void art_setstate(INTBIG newstate)
{
	art_state = newstate;

	if ((art_state&ARTWORKFILLARROWHEADS) != 0)
	{
		/* set filled arrow heads */
		art_arrow.layercount = 2;
		art_arrow_l[0].style = FILLED;
		art_arrow_l[0].points = art_arr1;
	} else
	{
		/* set outline arrow heads */
		art_arrow.layercount = 1;
		art_arrow_l[0].style = OPENED;
		art_arrow_l[0].points = art_arr;
	}
}

INTBIG art_nodepolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win)
{
	return(art_intnodepolys(ni, reasonable, win, &tech_oneprocpolyloop, &art_oneprocpolyloop));
}

INTBIG art_intnodepolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win, POLYLOOP *pl,
	ARTPOLYLOOP *artpl)
{
	REGISTER INTBIG pindex, count;
	Q_UNUSED( win );

	pindex = ni->proto->primindex;

	art_getgraphics((INTBIG)ni, VNODEINST, artpl);

	/* default count of polygons in the node */
	count = art_nodeprotos[pindex-1]->layercount;

	/* zero the count if this node is not to be displayed */
	if ((ni->userbits&WIPED) != 0) count = 0;

	/* add in displayable variables */
	pl->realpolys = count;
	count += tech_displayablenvars(ni, pl->curwindowpart, pl);
	if (reasonable != 0) *reasonable = count;
	return(count);
}

void art_shapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly)
{
	art_intshapenodepoly(ni, box, poly, &tech_oneprocpolyloop, &art_oneprocpolyloop);
}

void art_intshapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl, ARTPOLYLOOP *artpl)
{
	/* handle displayable variables */
	if (box >= pl->realpolys)
	{
		(void)tech_filldisplayablenvar(ni, poly, pl->curwindowpart, 0, pl);
		return;
	}

	art_fillnodebox(ni, box, poly);

	/* use stipple pattern if specified */
	if (artpl->patternvar == NOVARIABLE) poly->desc = &art_o_lay; else
		poly->desc = &art_st_lay;

	poly->desc->col = artpl->col;
	poly->desc->bits = artpl->bits;
}

INTBIG art_allnodepolys(NODEINST *ni, POLYLIST *plist, WINDOWPART *win, BOOLEAN onlyreasonable)
{
	REGISTER INTBIG tot, j;
	INTBIG reasonable;
	REGISTER NODEPROTO *np;
	REGISTER POLYGON *poly;
	POLYLOOP mypl;
	ARTPOLYLOOP myartpl;

	np = ni->proto;
	mypl.curwindowpart = win;
	tot = art_intnodepolys(ni, &reasonable, win, &mypl, &myartpl);
	if (onlyreasonable) tot = reasonable;
	if (mypl.realpolys < tot) tot = mypl.realpolys;
	if (ensurepolylist(plist, tot, db_cluster)) return(-1);
	for(j = 0; j < tot; j++)
	{
		poly = plist->polygons[j];
		poly->tech = art_tech;
		art_intshapenodepoly(ni, j, poly, &mypl, &myartpl);
	}
	return(tot);
}

INTBIG art_nodeEpolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win)
{
	Q_UNUSED( ni );
	Q_UNUSED( win );

	if (reasonable != 0) *reasonable = 0;
	return(0);
}

void art_shapeEnodepoly(NODEINST *ni, INTBIG box, POLYGON *poly)
{
	Q_UNUSED( ni );
	Q_UNUSED( box );
	Q_UNUSED( poly );
}

INTBIG art_allnodeEpolys(NODEINST *ni, POLYLIST *plist, WINDOWPART *win, BOOLEAN onlyreasonable)
{
	Q_UNUSED( ni );
	Q_UNUSED( plist );
	Q_UNUSED( win );
	Q_UNUSED( onlyreasonable );
	return(0);
}

void art_shapeportpoly(NODEINST *ni, PORTPROTO *pp, POLYGON *poly, XARRAY trans,
	BOOLEAN purpose)
{
	REGISTER INTBIG pindex;
	Q_UNUSED( purpose );

	pindex = ni->proto->primindex;
	if (pindex == NPIN || pindex == NARROW)
	{
		tech_fillportpoly(ni, pp, poly, trans, art_nodeprotos[pindex-1], CLOSED, lambdaofnode(ni));
		return;
	}

	/* just use first graphic polygon as the port */
	art_fillnodebox(ni, 0, poly);
	xformpoly(poly, trans);
}

INTBIG art_arcpolys(ARCINST *ai, WINDOWPART *win)
{
	return(art_intarcpolys(ai, win, &tech_oneprocpolyloop, &art_oneprocpolyloop));
}

INTBIG art_intarcpolys(ARCINST *ai, WINDOWPART *win, POLYLOOP *pl, ARTPOLYLOOP *artpl)
{
	REGISTER INTBIG def;

	art_getgraphics((INTBIG)ai, VARCINST, artpl);

	def = tech_initcurvedarc(ai, art_arcprotos[ai->proto->arcindex]->laycount, pl);

	/* add in displayable variables */
	pl->realpolys = def;
	def += tech_displayableavars(ai, win, pl);
	return(def);
}

void art_shapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly)
{
	art_intshapearcpoly(ai, box, poly, &tech_oneprocpolyloop, &art_oneprocpolyloop);
}

void art_intshapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly, POLYLOOP *pl, ARTPOLYLOOP *artpl)
{
	REGISTER INTBIG aindex;
	REGISTER INTBIG realwid;
	REGISTER TECH_ARCLAY *thista;

	/* handle displayable variables */
	if (box >= pl->realpolys)
	{
		(void)tech_filldisplayableavar(ai, poly, pl->curwindowpart, 0, pl);
		return;
	}

	/* handle curved arcs */
	aindex = ai->proto->arcindex;
	thista = &art_arcprotos[aindex]->list[0];
	realwid = ai->width - thista->off*lambdaofarc(ai)/WHOLE;
	if (tech_curvedarcpiece(ai, box, poly, art_arcprotos, pl))
	{
		/* standard arc: compute polygon in normal way */
		makearcpoly(ai->length, realwid, ai, poly, thista->style);
		poly->layer = thista->lay;
	}

	/* use stipple pattern if specified */
	if (artpl->patternvar == NOVARIABLE)
	{
		poly->desc = &art_o_lay;
		if (realwid == 0 && poly->style != CIRCLEARC) switch (ai->proto->arcindex)
		{
			case ADOTTED:  poly->style = OPENEDT1;   break;
			case ADASHED:  poly->style = OPENEDT2;   break;
			case ATHICKER: poly->style = OPENEDT3;   break;
		}
	} else poly->desc = &art_st_lay;

	poly->desc->col = artpl->col;
	poly->desc->bits = artpl->bits;
}

INTBIG art_allarcpolys(ARCINST *ai, POLYLIST *plist, WINDOWPART *win)
{
	REGISTER INTBIG tot, j;
	POLYLOOP mypl;
	ARTPOLYLOOP myartpl;

	mypl.curwindowpart = win;
	tot = art_intarcpolys(ai, win, &mypl, &myartpl);
	if (ensurepolylist(plist, tot, db_cluster)) return(-1);
	for(j = 0; j < tot; j++)
	{
		art_intshapearcpoly(ai, j, plist->polygons[j], &mypl, &myartpl);
	}
	return(tot);
}

/*
 * Helper routine to fill in polygon "box" of node "ni" into polygon "poly".
 */
void art_fillnodebox(NODEINST *ni, INTBIG box, POLYGON *poly)
{
	REGISTER INTBIG sty;
	REGISTER INTBIG pindex, dist, cx, cy, sx, sy, count;
	REGISTER VARIABLE *var;
	double startoffset, endangle;
	INTBIG dummypoints[8];

	pindex = ni->proto->primindex;
	if (pindex == NCIRCLE || pindex == NTCIRCLE)
	{
		/* handle ellipses */
		getarcdegrees(ni, &startoffset, &endangle);
		if (ni->highx - ni->lowx != ni->highy - ni->lowy)
		{
			art_fillellipse(ni->lowx, ni->lowy, ni->highx, ni->highy, startoffset, endangle, pindex, poly);
			poly->layer = art_nodeprotos[pindex-1]->layerlist[box].layernum;
		} else
		{
			tech_fillpoly(poly, &art_nodeprotos[pindex-1]->layerlist[box], ni,
				lambdaofnode(ni), FILLED);

			/* if there is arc information here, make it an arc of a circle */
			if (startoffset != 0.0 || endangle != 0.0)
			{
				/* fill an arc of a circle here */
				if (poly->limit < 3) (void)extendpolygon(poly, 3);
				dist = poly->xv[1] - poly->xv[0];
				poly->xv[2] = poly->xv[0] + rounddouble(cos(startoffset) * (double)dist);
				poly->yv[2] = poly->yv[0] + rounddouble(sin(startoffset) * (double)dist);
				poly->xv[1] = poly->xv[0] + rounddouble(cos(startoffset+endangle) * (double)dist);
				poly->yv[1] = poly->yv[0] + rounddouble(sin(startoffset+endangle) * (double)dist);
				poly->count = 3;
				if (pindex == NCIRCLE) poly->style = CIRCLEARC; else
					poly->style = THICKCIRCLEARC;
			}
		}
	} else if (pindex == NSPLINE)
	{
		poly->layer = art_nodeprotos[pindex-1]->layerlist[box].layernum;
		cx = (ni->highx + ni->lowx) / 2;   cy = (ni->highy + ni->lowy) / 2;
		var = gettrace(ni);
		if (var != NOVARIABLE)
		{
			count = getlength(var) / 2;
			art_fillspline((INTBIG *)var->addr, count, cx, cy, poly, SPLINEGRAIN);
		} else
		{
			sx = ni->highx - ni->lowx;   sy = ni->highy - ni->lowy;
			dummypoints[0] = -sx / 2;   dummypoints[1] = -sy / 2;
			dummypoints[2] = -sx / 8;   dummypoints[3] =  sy / 2;
			dummypoints[4] =  sx / 8;   dummypoints[5] = -sy / 2;
			dummypoints[6] =  sx / 2;   dummypoints[7] =  sy / 2;
			art_fillspline(dummypoints, 4, cx, cy, poly, SPLINEGRAIN);
		}
	} else
	{
		switch (pindex)
		{
			case NOPENED:   sty = OPENED;    break;
			case NOPENEDT1: sty = OPENEDT1;  break;
			case NOPENEDT2: sty = OPENEDT2;  break;
			case NOPENEDT3: sty = OPENEDT3;  break;
			case NCLOSED:   sty = CLOSED;    break;
			default:        sty = FILLED;    break;
		}

		tech_fillpoly(poly, &art_nodeprotos[pindex-1]->layerlist[box], ni,
			lambdaofnode(ni), sty);
	}
}

void art_getgraphics(INTBIG addr, INTBIG type, ARTPOLYLOOP *artpl)
{
	REGISTER INTBIG i, sty, len;

	/* get the color information */
	artpl->colorvar = getvalkey(addr, type, VINTEGER, art_colorkey);
	if (artpl->colorvar == NOVARIABLE)
	{
		artpl->col = BLACK;
		artpl->bits = LAYERO;
	} else
	{
		switch (artpl->colorvar->addr)
		{
			case LAYERT1: artpl->col = COLORT1;  artpl->bits = LAYERT1;  break;
			case LAYERT2: artpl->col = COLORT2;  artpl->bits = LAYERT2;  break;
			case LAYERT3: artpl->col = COLORT3;  artpl->bits = LAYERT3;  break;
			case LAYERT4: artpl->col = COLORT4;  artpl->bits = LAYERT4;  break;
			case LAYERT5: artpl->col = COLORT5;  artpl->bits = LAYERT5;  break;
			default:
				if ((artpl->colorvar->addr&(LAYERG|LAYERH|LAYEROE)) == LAYEROE) artpl->bits = LAYERO; else
					artpl->bits = LAYERA;
				artpl->col = artpl->colorvar->addr;
				break;
		}
	}

	/* get the stipple pattern information */
	artpl->patternvar = getvalkey(addr, type, -1, art_patternkey);
	if (artpl->patternvar != NOVARIABLE)
	{
		len = getlength(artpl->patternvar);
		if ((len != 8 && len != 16) ||
			((artpl->patternvar->type&VTYPE) != VINTEGER && (artpl->patternvar->type&VTYPE) != VSHORT))
		{
			ttyputerr(_("'ART_pattern' must be a 16-member INTEGER or SHORT array"));
			artpl->patternvar = NOVARIABLE;
			return;
		}

		sty = PATTERNED;
		if ((artpl->patternvar->type&VTYPE) == VINTEGER)
		{
			for(i=0; i<len; i++)
				art_st_lay.raster[i] = (UINTSML)(((INTBIG *)artpl->patternvar->addr)[i]);
		} else
		{
			for(i=0; i<len; i++)
				art_st_lay.raster[i] = ((INTSML *)artpl->patternvar->addr)[i];
			sty |= OUTLINEPAT;
		}
		if (len == 8)
		{
			for(i=0; i<8; i++) art_st_lay.raster[i+8] = art_st_lay.raster[i];
		}

		/* set the outline style (outlined if SHORT used) */
		art_st_lay.colstyle = art_st_lay.bwstyle = (INTSML)sty;
	}
}

/******************** CURVE DRAWING ********************/

/*
 * routine to fill polygon "poly" with the vectors for the ellipse whose
 * bounding box is given by the rectangle (lx-hx) and (ly-hy).
 */
void art_fillellipse(INTBIG lx, INTBIG ly, INTBIG hx, INTBIG hy, double startoffset,
	double endangle, INTBIG pindex, POLYGON *poly)
{
	double cx, cy, a, b, p, s2, s3, c2, c3, t1;
	REGISTER INTBIG m, pts;
	REGISTER BOOLEAN closed;

	/* ensure that the polygon can hold the vectors */
	if (startoffset == 0.0 && endangle == 0.0)
	{
		/* full ellipse */
		closed = TRUE;
		endangle = EPI * 2.0;
		if (pindex == NCIRCLE) poly->style = CLOSED; else
			poly->style = OPENEDT3;
	} else
	{
		/* partial ellipse */
		closed = FALSE;
		if (pindex == NCIRCLE) poly->style = OPENED; else
			poly->style = OPENEDT3;
	}
	pts = (INTBIG)(endangle * ELLIPSEPOINTS / (EPI * 2.0));
	if (closed && pindex != NCIRCLE) pts++;
	if (poly->limit < pts) (void)extendpolygon(poly, pts);
	poly->count = pts;

	/* get center of ellipse */
	cx = (lx + hx) / 2;
	cy = (ly + hy) / 2;

	/* compute the length of the semi-major and semi-minor axes */
	a = (hx - lx) / 2;
	b = (hy - ly) / 2;

	if (closed)
	{
		/* more efficient algorithm used for full ellipse drawing */
		p = 2.0 * EPI / (ELLIPSEPOINTS-1);
		c2 = cos(p);    s2 = sin(p);
		c3 = 1.0;       s3 = 0.0;
		for(m=0; m<ELLIPSEPOINTS; m++)
		{
			poly->xv[m] = rounddouble(cx + a * c3);
			poly->yv[m] = rounddouble(cy + b * s3);
			t1 = c3*c2 - s3*s2;
			s3 = s3*c2 + c3*s2;
			c3 = t1;
		}
		if (pindex != NCIRCLE)
		{
			poly->xv[m] = poly->xv[0];
			poly->yv[m] = poly->yv[0];
		}
	} else
	{
		/* less efficient algorithm for partial ellipse drawing */
		for(m=0; m<pts; m++)
		{
			p = startoffset + m * endangle / (pts-1);
			c2 = cos(p);   s2 = sin(p);
			poly->xv[m] = rounddouble(cx + a * c2);
			poly->yv[m] = rounddouble(cy + b * s2);
		}
	}
}

#define SPLINEPOLYX(i) (points[(i)*2] + cx)
#define SPLINEPOLYY(i) (points[(i)*2+1] + cy)

/*
 * Routine to convert the "count" spline control points in "points" that are centered at (cx,cy)
 * to a line approximation in "poly".  Uses "steps" lines per spline segment.
 */
void art_fillspline(INTBIG *points, INTBIG count, INTBIG cx, INTBIG cy, POLYGON *poly, INTBIG steps)
{
	REGISTER INTBIG i, k, out, outpoints;
	double t, t1, t2, t3, t4, x1, y1, x2, y2, x3, y3, x4, y4, x, y, tsq, splinestep;

	outpoints = (count - 1) * steps + 1;
	if (poly->limit < outpoints) (void)extendpolygon(poly, outpoints);
	out = 0;

	splinestep = 1.0 / (double)steps;
	x2 = SPLINEPOLYX(0)*2 - SPLINEPOLYX(1);
	y2 = SPLINEPOLYY(0)*2 - SPLINEPOLYY(1);
	x3 = SPLINEPOLYX(0);
	y3 = SPLINEPOLYY(0);
	x4 = SPLINEPOLYX(1);
	y4 = SPLINEPOLYY(1);
	for(k = 2; k <= count; k++)
	{
		x1 = x2;   x2 = x3;   x3 = x4;
		y1 = y2;   y2 = y3;   y3 = y4;
		if (k == count)
		{
		   x4 = SPLINEPOLYX(k-1)*2 - SPLINEPOLYX(k-2);
		   y4 = SPLINEPOLYY(k-1)*2 - SPLINEPOLYY(k-2);
		} else
		{
		   x4 = SPLINEPOLYX(k);
		   y4 = SPLINEPOLYY(k);
		}

		for(t=0.0, i=0; i<steps; i++, t+= splinestep)
		{
			tsq = t * t;
			t4 = tsq * t;
			t3 = -3.0*t4 + 3.0*tsq + 3.0*t + 1.0;
			t2 = 3.0*t4 - 6.0*tsq + 4.0;
			t1 = -t4 + 3.0*tsq - 3.0*t + 1.0;

			x = (x1*t1 + x2*t2 + x3*t3 + x4*t4) / 6.0;
			y = (y1*t1 + y2*t2 + y3*t3 + y4*t4) / 6.0;
			poly->xv[out] = rounddouble(x);
			poly->yv[out++] = rounddouble(y);
		}
	}

	/* close the spline */
	poly->xv[out] = SPLINEPOLYX(count-1);
	poly->yv[out++] = SPLINEPOLYY(count-1);
	poly->style = OPENED;
	poly->count = out;
}
