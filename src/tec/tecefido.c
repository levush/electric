/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tecefido.c
 * Digital Filter Technology
 * Written by: Wallace Kroeker, University of Calgary
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
#include "tech.h"
#include "efunction.h"

#define TH0	(WHOLE/3)		/* 0.333.. */
#define D1	(WHOLE/20)		/* 0.05 */
#define D2	(WHOLE/10)		/* 0.10 */
#define D3	(WHOLE/20 * 3)	/* 0.15 */
#define D4	(WHOLE/5)		/* 0.20 */

BOOLEAN efido_initprocess(TECHNOLOGY*, INTBIG);

/******************** LAYERS ********************/

#define MAXLAYERS 3		/* total layers below */
#define LNODE     0		/* components         */
#define LARC      1		/* connections        */
#define LOUTPAD   2

static GRAPHICS efido_n_lay = {LAYERT1, COLORT1, SOLIDC, SOLIDC,
/* node layer */    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS efido_a_lay = {LAYERT2, COLORT2, SOLIDC, SOLIDC,
/* arc layer */	    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS efido_o_lay = {LAYERT3, COLORT3, SOLIDC, SOLIDC,
/* outpad layer */  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};

/* these tables must be updated together */
GRAPHICS *efido_layers[MAXLAYERS+1] = {&efido_n_lay, &efido_a_lay,
											&efido_o_lay, NOGRAPHICS};
static CHAR *efido_layer_names[MAXLAYERS] = {x_("Node"), x_("Arc"), x_("Outpad")};
static INTBIG efido_layer_function[MAXLAYERS] = {LFART|LFNONELEC|LFTRANS1,
	LFUNKNOWN|LFTRANS2, LFOVERGLASS|LFTRANS3};
static CHAR *efido_layer_letters[MAXLAYERS] = {x_("n"),x_("a"),x_("o")};

/* The low 5 bits map LNODE and LARC */
static TECH_COLORMAP efido_colmap[32] =
{                  /*                                        */
	{200,200,200}, /* 0:                                     */
	{255,  0,  0}, /* 1:                                node */
	{  0,  0,255}, /* 2:                          arc        */
	{150, 20,150}, /* 3:                          arc + node */
	{  0,155, 80}, /* 4:                          outpad     */
	{  0,  0,  0}, /* 5:*/
	{  0,  0,  0}, /* 6:*/
	{  0,  0,  0}, /* 7:*/
	{  0,  0,  0}, /* 8:*/
	{  0,  0,  0}, /* 9:*/
	{  0,  0,  0}, /* 10:*/
	{  0,  0,  0}, /* 11:*/
	{  0,  0,  0}, /* 12:*/
	{  0,  0,  0}, /* 13:*/
	{  0,  0,  0}, /* 14:*/
	{  0,  0,  0}, /* 15:*/
	{  0,  0,  0}, /* 16:*/
	{  0,  0,  0}, /* 17:*/
	{  0,  0,  0}, /* 18:*/
	{  0,  0,  0}, /* 19:*/
	{  0,  0,  0}, /* 20:*/
	{  0,  0,  0}, /* 21:*/
	{  0,  0,  0}, /* 22:*/
	{  0,  0,  0}, /* 23:*/
	{  0,  0,  0}, /* 24:*/
	{  0,  0,  0}, /* 25:*/
	{  0,  0,  0}, /* 26:*/
	{  0,  0,  0}, /* 27:*/
	{  0,  0,  0}, /* 28:*/
	{  0,  0,  0}, /* 29:*/
	{  0,  0,  0}, /* 30:*/
	{  0,  0,  0}, /* 31:*/
};

/******************** ARCS ********************/

#define ARCPROTOCOUNT  2
#define AWIRE          0	/* wire */
#define ABUS	       1	/* bus  */

/* wire arc */
static TECH_ARCLAY efido_al_w[] = {{ LARC,0,FILLED }};
static TECH_ARCS efido_a_w = {
	x_("wire"),0,AWIRE,NOARCPROTO,									/* name */
	1,efido_al_w,													/* layers */
	(APMETAL1<<AFUNCTIONSH)|WANTFIXANG|(45<<AANGLEINCSH)};			/* userbits */

/* bus arc */
static TECH_ARCLAY efido_al_b[] = {{ LARC,0,CLOSED }};
static TECH_ARCS efido_a_b = {
	x_("bus"),K2,ABUS,NOARCPROTO,									/* name */
	1,efido_al_b,													/* layers */
	(APBUS<<AFUNCTIONSH)|WANTFIXANG|(45<<AANGLEINCSH)};				/* userbits */

TECH_ARCS *efido_arcprotos[ARCPROTOCOUNT+1] = {&efido_a_w, &efido_a_b, ((TECH_ARCS *)-1)};

/******************** PORTINST CONNECTIONS ********************/

/* these values are replaced with actual arcproto addresses */
static INTBIG efido_pc_w[]  = {-1, AWIRE, ALLGEN, -1};
static INTBIG efido_pc_b[]  = {-1, ABUS, ALLGEN, -1};

/******************** NODES ********************/

#define NODEPROTOCOUNT	10
#define NWIREPIN        1	/* wire pin */
#define NBUSPIN         2	/* bus pin */
#define NADDER          3	/* adder */
#define NMULT           4	/* multiplier */
#define NTDELAY         5	/* time delay */
#define NMUX            6	/* multiplexer */
#define NSUBTR          7	/* subtractor */
#define NDIV            8	/* divider */
#define NPADIN          9	/* pad in */
#define NPADOUT         10	/* pad out */

/******************** RECTANGLE DESCRIPTIONS ********************/

static INTBIG efido_g_fullbox[8]  = {LEFTEDGE, LEFTEDGE, RIGHTEDGE, TOPEDGE};
static INTBIG efido_g_halfbox[8]  = {-Q0,0,    -Q0,0,    Q0,0,      Q0,0};
static INTBIG efido_g_bp[8]       = {LEFTEDGE, BOTEDGE,  RIGHTEDGE, TOPEDGE};
static INTBIG efido_g_pt1[12]     = {H0-D2,0,  D2,0,     RIGHTEDGE, CENTER,
									H0-D2,0,  -D2,0};
static INTBIG efido_g_pt2[8]      = {H0-D4,0,  CENTER,   RIGHTEDGE, CENTER};
static INTBIG efido_g_adder[20]   = {Q0,0,     Q0,0,     -Q0,0,     Q0,0,
									CENTER,    CENTER,   -Q0,0,     -Q0,0,
									Q0,0,      -Q0,0};
static INTBIG efido_g_mult1[8]    = {Q0,0,     Q0,0,     -Q0,0,     -Q0,0};
static INTBIG efido_g_mult2[8]    = {-Q0,0,    Q0,0,     Q0,0,      -Q0,0};
static INTBIG efido_g_tdelay1[8]  = {CENTER,   Q0,0,     CENTER,    -Q0,0};
static INTBIG efido_g_tdelay2[8]  = {-Q0,0,    Q0,0,     Q0,0,      Q0,0};
static INTBIG efido_g_mux1[8]     = {LEFTEDGE, Q0,0,     -Q0,0,     Q0,0};
static INTBIG efido_g_mux2[16]    = {LEFTEDGE, -Q0,0,    -Q0,0,     -Q0,0,
									Q0,0,     CENTER,   RIGHTEDGE, CENTER};
static INTBIG efido_g_mux3[12]    = {-Q0+D1,0, -D1,0,    -Q0,0,     -Q0,0,
									-Q0+D3,0, -Q0-D1,0};
static INTBIG efido_g_subtr[8]    = {-Q0,0,    CENTER,   Q0,0,      CENTER};
static INTBIG efido_g_div1[8]     = {-D1,0,    Q0,0,     D1,0,      Q0,0};
static INTBIG efido_g_div2[8]     = {-D1,0,    -Q0,0,    D1,0,      -Q0,0};
static INTBIG efido_disccenter[8] = {CENTER,   CENTER,   RIGHTEDGE, CENTER};

/* wire-pin */
static TECH_PORTS efido_wp_p[] = {						/* ports */
	{efido_pc_w, x_("wire"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON efido_wp_l[] = {					/* layers */
	{LARC, 0, 2, DISC, POINTS, efido_disccenter}};
static TECH_NODES efido_wp = {
	x_("wire_pin"),NWIREPIN,NONODEPROTO,				/* name */
	K1,K1,												/* size */
	1,efido_wp_p,										/* ports */
	1,efido_wp_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|NSQUARE|ARCSWIPE|WIPEON1OR2,	/* userbits */
	0,0,0,0,0,0,0,0,0};									/* characteristics */

/* bus-pin */
static TECH_PORTS efido_bp_p[] = {						/* ports */
	{efido_pc_b, x_("bus"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON efido_bp_l[] = {					/* layers */
	{LNODE,0, 4, CROSSED, BOX, efido_g_bp}};
static TECH_NODES efido_bp = {
	x_("bus_pin"),NBUSPIN,NONODEPROTO,					/* name */
	K2,K2,												/* size */
	1,efido_bp_p,										/* ports */
	1,efido_bp_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,			/* userbits */
	0,0,0,0,0,0,0,0,0};									/* characteristics */

/* adder */
static TECH_PORTS efido_adder_p[] = {					/* ports */
	{efido_pc_w, x_("in1"), NOPORTPROTO,
		 (270<<PORTANGLESH) | (15<<PORTARANGESH) | (0<<PORTNETSH) | INPORT,
			CENTER, TOPEDGE, CENTER, TOPEDGE},
	{efido_pc_w, x_("in2"), NOPORTPROTO,
		 (225<<PORTANGLESH) | (15<<PORTARANGESH) | (1<<PORTNETSH) | INPORT,
			-TH0, 0,TH0, 0,  -TH0, 0,TH0, 0},
	{efido_pc_w, x_("in3"), NOPORTPROTO,
		 (180<<PORTANGLESH) | (15<<PORTARANGESH) | (2<<PORTNETSH) | INPORT,
			LEFTEDGE, CENTER, LEFTEDGE, CENTER},
	{efido_pc_w, x_("in4"), NOPORTPROTO,
		 (135<<PORTANGLESH) | (15<<PORTARANGESH) | (3<<PORTNETSH) | INPORT,
			-TH0, 0,-TH0, 0,  -TH0, 0,-TH0, 0},
	{efido_pc_w, x_("in5"), NOPORTPROTO,
		 (90<<PORTANGLESH) | (15<<PORTARANGESH) | (4<<PORTNETSH) | INPORT,
			CENTER, BOTEDGE, CENTER, BOTEDGE},
	{efido_pc_w, x_("out"), NOPORTPROTO,
		 (0<<PORTANGLESH) | (45<<PORTARANGESH) | (5<<PORTNETSH) | OUTPORT,
			RIGHTEDGE, CENTER, RIGHTEDGE, CENTER}};
static TECH_POLYGON efido_adder_l[] = {					/* layers */
	{LNODE,0, 2, CIRCLE, POINTS, efido_disccenter},
	{LNODE,0, 5, OPENED, POINTS, efido_g_adder},
	{LNODE,0, 2, OPENED, POINTS, efido_g_pt2},
	{LNODE,0, 3, OPENED, POINTS, efido_g_pt1}};
static TECH_NODES efido_adder = {
	x_("adder"),NADDER,NONODEPROTO,						/* name */
	K4,K4,												/* size */
	6,efido_adder_p,									/* ports */
	4,efido_adder_l,									/* layers */
	(NPUNKNOWN<<NFUNCTIONSH)|NSQUARE,					/* userbits */
	0,0,0,0,0,0,0,0,0};									/* characteristics */

/* multiplier */
static TECH_PORTS efido_mult_p[] = {					/* ports */
	{efido_pc_w, x_("in1"), NOPORTPROTO,
		 (270<<PORTANGLESH) | (15<<PORTARANGESH) | (0<<PORTNETSH) | INPORT,
			CENTER, TOPEDGE, CENTER, TOPEDGE},
	{efido_pc_w, x_("in2"), NOPORTPROTO,
		 (225<<PORTANGLESH) | (15<<PORTARANGESH) | (1<<PORTNETSH) | INPORT,
			-TH0, 0,TH0, 0,  -TH0, 0,TH0, 0},
	{efido_pc_w, x_("in3"), NOPORTPROTO,
		 (180<<PORTANGLESH) | (15<<PORTARANGESH) | (2<<PORTNETSH) | INPORT,
			LEFTEDGE, CENTER, LEFTEDGE, CENTER},
	{efido_pc_w, x_("in4"), NOPORTPROTO,
		 (135<<PORTANGLESH) | (15<<PORTARANGESH) | (3<<PORTNETSH) | INPORT,
			-TH0, 0,-TH0, 0,  -TH0, 0,-TH0, 0},
	{efido_pc_w, x_("in5"), NOPORTPROTO,
		 (90<<PORTANGLESH) | (15<<PORTARANGESH) | (4<<PORTNETSH) | INPORT,
			CENTER, BOTEDGE, CENTER, BOTEDGE},
	{efido_pc_w, x_("out"), NOPORTPROTO,
		 (0<<PORTANGLESH) | (45<<PORTARANGESH) | (5<<PORTNETSH) | OUTPORT,
			RIGHTEDGE, CENTER, RIGHTEDGE, CENTER}};
static TECH_POLYGON efido_mult_l[] = {					/* layers */
	{LNODE,0, 2, CIRCLE, POINTS, efido_disccenter},
	{LNODE,0, 2, OPENED, POINTS, efido_g_mult1},
	{LNODE,0, 2, OPENED, POINTS, efido_g_mult2},
	{LNODE,0, 2, OPENED, POINTS, efido_g_pt2},
	{LNODE,0, 3, OPENED, POINTS, efido_g_pt1}};
static TECH_NODES efido_mult = {
	x_("multiplier"),NMULT,NONODEPROTO,					/* name */
	K4,K4,												/* size */
	6,efido_mult_p,										/* ports */
	5,efido_mult_l,										/* layers */
	(NPUNKNOWN<<NFUNCTIONSH)|NSQUARE,					/* userbits */
	0,0,0,0,0,0,0,0,0};									/* characteristics */

/* time delay */
static TECH_PORTS efido_tdelay_p[] = {					/* ports */
	{efido_pc_w, x_("in"), NOPORTPROTO,
		 (180<<PORTANGLESH) | (15<<PORTARANGESH) | (0<<PORTNETSH) | INPORT,
			LEFTEDGE, CENTER, LEFTEDGE, CENTER},
	{efido_pc_w, x_("out"), NOPORTPROTO,
		 (0<<PORTANGLESH) | (45<<PORTARANGESH) | (1<<PORTNETSH) | OUTPORT,
			RIGHTEDGE, CENTER, RIGHTEDGE, CENTER}};
static TECH_POLYGON efido_tdelay_l[] = {				/* layers */
	{LNODE,0, 4, CLOSEDRECT, BOX,    efido_g_fullbox},
	{LNODE,0, 2, OPENED,     POINTS, efido_g_tdelay1},
	{LNODE,0, 2, OPENED,     POINTS, efido_g_tdelay2},
	{LNODE,0, 2, OPENED,     POINTS, efido_g_pt2},
	{LNODE,0, 3, OPENED,     POINTS, efido_g_pt1}};
static TECH_NODES efido_tdelay = {
	x_("timedelay"),NTDELAY,NONODEPROTO,				/* name */
	K4,K4,												/* size */
	2,efido_tdelay_p,									/* ports */
	5,efido_tdelay_l,									/* layers */
	(NPUNKNOWN<<NFUNCTIONSH),							/* userbits */
	0,0,0,0,0,0,0,0,0};									/* characteristics */

/* multiplexer */
static TECH_PORTS efido_mux_p[] = {						/* ports */
	{efido_pc_w, x_("in1"), NOPORTPROTO,
		 (270<<PORTANGLESH) | (15<<PORTARANGESH) | (0<<PORTNETSH) | INPORT,
			LEFTEDGE, Q0, 0, LEFTEDGE, Q0,0},
	{efido_pc_w, x_("in2"), NOPORTPROTO,
		 (135<<PORTANGLESH) | (15<<PORTARANGESH) | (1<<PORTNETSH) | INPORT,
			LEFTEDGE, -Q0, 0, LEFTEDGE, -Q0,0},
	{efido_pc_w, x_("out"), NOPORTPROTO,
		 (0<<PORTANGLESH) | (45<<PORTARANGESH) | (2<<PORTNETSH) | OUTPORT,
			RIGHTEDGE, CENTER, RIGHTEDGE, CENTER}};
static TECH_POLYGON efido_mux_l[] = {					/* layers */
	{LNODE,0, 4, CLOSEDRECT, BOX,    efido_g_fullbox},
	{LNODE,0, 2, OPENED,     POINTS, efido_g_mux1},
	{LNODE,0, 4, OPENED,     POINTS, efido_g_mux2},
	{LNODE,0, 3, OPENED,     POINTS, efido_g_mux3}};
static TECH_NODES efido_mux = {
	x_("multiplexer"),NMUX,NONODEPROTO,					/* name */
	K4,K4,												/* size */
	3,efido_mux_p,										/* ports */
	4,efido_mux_l,										/* layers */
	(NPUNKNOWN<<NFUNCTIONSH),							/* userbits */
	0,0,0,0,0,0,0,0,0};									/* characteristics */

/* subtractor */
static TECH_PORTS efido_subtr_p[] = {					/* ports */
	{efido_pc_w, x_("in1"), NOPORTPROTO,
		 (270<<PORTANGLESH) | (15<<PORTARANGESH) | (0<<PORTNETSH) | INPORT,
			CENTER, TOPEDGE, CENTER, TOPEDGE},
	{efido_pc_w, x_("in2"), NOPORTPROTO,
		 (225<<PORTANGLESH) | (15<<PORTARANGESH) | (1<<PORTNETSH) | INPORT,
			-TH0, 0,TH0, 0,  -TH0, 0,TH0, 0},
	{efido_pc_w, x_("in3"), NOPORTPROTO,
		 (180<<PORTANGLESH) | (15<<PORTARANGESH) | (2<<PORTNETSH) | INPORT,
			LEFTEDGE, CENTER, LEFTEDGE, CENTER},
	{efido_pc_w, x_("in4"), NOPORTPROTO,
		 (135<<PORTANGLESH) | (15<<PORTARANGESH) | (3<<PORTNETSH) | INPORT,
			-TH0, 0,-TH0, 0,  -TH0, 0,-TH0, 0},
	{efido_pc_w, x_("in5"), NOPORTPROTO,
		 (90<<PORTANGLESH) | (15<<PORTARANGESH) | (4<<PORTNETSH) | INPORT,
			CENTER, BOTEDGE, CENTER, BOTEDGE},
	{efido_pc_w, x_("out"), NOPORTPROTO,
		 (0<<PORTANGLESH) | (45<<PORTARANGESH) | (5<<PORTNETSH) | OUTPORT,
			RIGHTEDGE, CENTER, RIGHTEDGE, CENTER}};
static TECH_POLYGON efido_subtr_l[] = {					/* layers */
	{LNODE,0, 2, CIRCLE, POINTS, efido_disccenter},
	{LNODE,0, 2, OPENED, POINTS, efido_g_subtr},
	{LNODE,0, 2, OPENED, POINTS, efido_g_pt2},
	{LNODE,0, 3, OPENED, POINTS, efido_g_pt1}};
static TECH_NODES efido_subtr = {
	x_("subtractor"),NSUBTR,NONODEPROTO,				/* name */
	K4,K4,												/* size */
	6,efido_subtr_p,									/* ports */
	4,efido_subtr_l,									/* layers */
	(NPUNKNOWN<<NFUNCTIONSH)|NSQUARE,					/* userbits */
	0,0,0,0,0,0,0,0,0};									/* characteristics */

/* divider */
static TECH_PORTS efido_div_p[] = {						/* ports */
	{efido_pc_w, x_("in1"), NOPORTPROTO,
		 (270<<PORTANGLESH) | (15<<PORTARANGESH) | (0<<PORTNETSH) | INPORT,
			CENTER, TOPEDGE, CENTER, TOPEDGE},
	{efido_pc_w, x_("in2"), NOPORTPROTO,
		 (225<<PORTANGLESH) | (15<<PORTARANGESH) | (1<<PORTNETSH) | INPORT,
			-TH0, 0,TH0, 0,  -TH0, 0,TH0, 0},
	{efido_pc_w, x_("in3"), NOPORTPROTO,
		 (180<<PORTANGLESH) | (15<<PORTARANGESH) | (2<<PORTNETSH) | INPORT,
			LEFTEDGE, CENTER, LEFTEDGE, CENTER},
	{efido_pc_w, x_("in4"), NOPORTPROTO,
		 (135<<PORTANGLESH) | (15<<PORTARANGESH) | (3<<PORTNETSH) | INPORT,
			-TH0, 0,-TH0, 0,  -TH0, 0,-TH0, 0},
	{efido_pc_w, x_("in5"), NOPORTPROTO,
		 (90<<PORTANGLESH) | (15<<PORTARANGESH) | (4<<PORTNETSH) | INPORT,
			CENTER, BOTEDGE, CENTER, BOTEDGE},
	{efido_pc_w, x_("out"), NOPORTPROTO,
		 (0<<PORTANGLESH) | (45<<PORTARANGESH) | (5<<PORTNETSH) | OUTPORT,
			RIGHTEDGE, CENTER, RIGHTEDGE, CENTER}};
static TECH_POLYGON efido_div_l[] = {					/* layers */
	{LNODE,0, 2, CIRCLE, POINTS, efido_disccenter},
	{LNODE,0, 2, OPENED, POINTS, efido_g_subtr},
	{LNODE,0, 2, OPENED, POINTS, efido_g_div2},
	{LNODE,0, 2, OPENED, POINTS, efido_g_div1},
	{LNODE,0, 2, OPENED, POINTS, efido_g_pt2},
	{LNODE,0, 3, OPENED, POINTS, efido_g_pt1}};
static TECH_NODES efido_div = {
	x_("divider"),NDIV,NONODEPROTO,						/* name */
	K4,K4,												/* size */
	6,efido_div_p,										/* ports */
	6,efido_div_l,										/* layers */
	(NPUNKNOWN<<NFUNCTIONSH)|NSQUARE,					/* userbits */
	0,0,0,0,0,0,0,0,0};									/* characteristics */

/* pad in */
static TECH_PORTS efido_padin_p[] = {					/* ports */
	{efido_pc_w, x_("out"), NOPORTPROTO, (180<<PORTARANGESH) | INPORT,
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON efido_padin_l[] = {					/* layers */
	{LNODE,0, 4, CLOSEDRECT, BOX, efido_g_fullbox},
	{LNODE,0, 4, CROSSED,    BOX, efido_g_halfbox}};
static TECH_NODES efido_padin = {
	x_("padin"),NPADIN,NONODEPROTO,						/* name */
	K4,K4,												/* size */
	1,efido_padin_p,									/* ports */
	2,efido_padin_l,									/* layers */
	(NPCONNECT<<NFUNCTIONSH),							/* userbits */
	0,0,0,0,0,0,0,0,0};									/* characteristics */

/* pad out */
static TECH_PORTS efido_padout_p[] = {					/* ports */
	{efido_pc_w, x_("in"), NOPORTPROTO, (180<<PORTARANGESH) | OUTPORT,
			CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON efido_padout_l[] = {				/* layers */
	{LNODE,  0, 4, CLOSEDRECT, BOX, efido_g_fullbox},
	{LOUTPAD,0, 4, CROSSED,    BOX, efido_g_halfbox}};
static TECH_NODES efido_padout = {
	x_("padout"),NPADOUT,NONODEPROTO,					/* name */
	K4,K4,												/* size */
	1,efido_padout_p,									/* ports */
	2,efido_padout_l,									/* layers */
	(NPCONNECT<<NFUNCTIONSH),							/* userbits */
	0,0,0,0,0,0,0,0,0};									/* characteristics */

TECH_NODES *efido_nodeprotos[NODEPROTOCOUNT+1] =
{
	&efido_wp, &efido_bp, &efido_adder,
	&efido_mult, &efido_tdelay, &efido_mux, &efido_subtr,
	&efido_div, &efido_padin, &efido_padout, ((TECH_NODES *)-1)
};

/******************** VARIABLE AGGREGATION ********************/

TECH_VARIABLES efido_variables[] =
{
	/* set general information about the technology */
	{x_("TECH_layer_names"), (CHAR *)efido_layer_names, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_layer_function"), (CHAR *)efido_layer_function, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},

	/* set information for the USER analysis tool */
	{x_("USER_color_map"), (CHAR *)efido_colmap, 0.0,
		VCHAR|VDONTSAVE|VISARRAY|((sizeof efido_colmap)<<VLENGTHSH)},
	{x_("USER_layer_letters"), (CHAR *)efido_layer_letters, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{NULL, NULL, 0.0, 0}
};

/******************** ROUTINES ********************/

BOOLEAN efido_initprocess(TECHNOLOGY *tech, INTBIG pass)
{
	Q_UNUSED( tech );
	Q_UNUSED( pass );
	return(FALSE);
}
