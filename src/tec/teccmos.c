/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: teccmos.c
 * CMOS technology description
 * Written by: Steven M. Rubin, Static Free Software
 * From Thomas Griswold's article in VLSI Design, September/October 1982
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

BOOLEAN cmos_initprocess(TECHNOLOGY*, INTBIG);

/******************** LAYERS ********************/

#define MAXLAYERS 14		/* total layers below */
#define LMETAL     0		/* metal              */
#define LPOLY      1		/* polysilicon        */
#define LDIFF      2		/* diffusion          */
#define LP         3		/* p+                 */
#define LCUT       4		/* cut                */
#define LOCUT      5		/* ohmic cut          */
#define LWELL      6		/* p-well             */
#define LOVERGL    7		/* overglass          */
#define LTRANS     8		/* transistor         */
#define LMETALP    9		/* pseudo metal       */
#define LPOLYP    10		/* pseudo polysilicon */
#define LDIFFP    11		/* pseudo diffusion   */
#define LPP       12		/* pseudo p+          */
#define LWELLP    13		/* pseudo p-well      */

static GRAPHICS cmos_m_lay = {LAYERT1,COLORT1, SOLIDC, PATTERNED,
/* metal layer */		{0x2222, /*   X   X   X   X  */
						0x0000,  /*                  */
						0x8888,  /* X   X   X   X    */
						0x0000,  /*                  */
						0x2222,  /*   X   X   X   X  */
						0x0000,  /*                  */
						0x8888,  /* X   X   X   X    */
						0x0000,  /*                  */
						0x2222,  /*   X   X   X   X  */
						0x0000,  /*                  */
						0x8888,  /* X   X   X   X    */
						0x0000,  /*                  */
						0x2222,  /*   X   X   X   X  */
						0x0000,  /*                  */
						0x8888,  /* X   X   X   X    */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS cmos_p_lay = {LAYERT2,COLORT2, SOLIDC, PATTERNED,
/* polysilicon layer */	{0x0808, /*     X       X    */
						0x0404,  /*      X       X   */
						0x0202,  /*       X       X  */
						0x0101,  /*        X       X */
						0x8080,  /* X       X        */
						0x4040,  /*  X       X       */
						0x2020,  /*   X       X      */
						0x1010,  /*    X       X     */
						0x0808,  /*     X       X    */
						0x0404,  /*      X       X   */
						0x0202,  /*       X       X  */
						0x0101,  /*        X       X */
						0x8080,  /* X       X        */
						0x4040,  /*  X       X       */
						0x2020,  /*   X       X      */
						0x1010}, /*    X       X     */
						NOVARIABLE, 0};
static GRAPHICS cmos_d_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
/* diffusion layer */	{0x0000, /*                  */
						0x0303,  /*       XX      XX */
						0x4848,  /*  X  X    X  X    */
						0x0303,  /*       XX      XX */
						0x0000,  /*                  */
						0x3030,  /*   XX      XX     */
						0x8484,  /* X    X  X    X   */
						0x3030,  /*   XX      XX     */
						0x0000,  /*                  */
						0x0303,  /*       XX      XX */
						0x4848,  /*  X  X    X  X    */
						0x0303,  /*       XX      XX */
						0x0000,  /*                  */
						0x3030,  /*   XX      XX     */
						0x8484,  /* X    X  X    X   */
						0x3030}, /*   XX      XX     */
						NOVARIABLE, 0};
static GRAPHICS cmos_pl_lay = {LAYERT4,COLORT4, SOLIDC, PATTERNED,
/* P+ layer */			{0x1000, /*    X             */
						0x0020,  /*           X      */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0001,  /*                X */
						0x0200,  /*       X          */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x1000,  /*    X             */
						0x0020,  /*           X      */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0001,  /*                X */
						0x0200,  /*       X          */
						0x0000,  /*                  */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS cmos_c_lay = {LAYERO,BLACK, SOLIDC, SOLIDC,
/* contact layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS cmos_oc_lay = {LAYERO,BLACK, SOLIDC, SOLIDC,
/* oversize-cut layer */{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS cmos_w_lay = {LAYERT5,COLORT5, SOLIDC, PATTERNED,
/* P-well layer */		{0x0000, /*                  */
						0x00C0,  /*         XX       */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x00C0,  /*         XX       */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x00C0,  /*         XX       */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x00C0,  /*         XX       */
						0x0000,  /*                  */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS cmos_o_lay = {LAYERO,DGRAY, SOLIDC, PATTERNED,
/* overglass layer */	{0x1C1C, /*    XXX     XXX   */
						0x3E3E,  /*   XXXXX   XXXXX  */
						0x3636,  /*   XX XX   XX XX  */
						0x3E3E,  /*   XXXXX   XXXXX  */
						0x1C1C,  /*    XXX     XXX   */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x1C1C,  /*    XXX     XXX   */
						0x3E3E,  /*   XXXXX   XXXXX  */
						0x3636,  /*   XX XX   XX XX  */
						0x3E3E,  /*   XXXXX   XXXXX  */
						0x1C1C,  /*    XXX     XXX   */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS cmos_tr_lay = {LAYERN,ALLOFF, SOLIDC, PATTERNED,
/* transistor layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS cmos_pm_lay = {LAYERT1,COLORT1, SOLIDC, PATTERNED,
/* pseudo-metal layer */{0x2222, /*   X   X   X   X  */
						0x0000,  /*                  */
						0x8888,  /* X   X   X   X    */
						0x0000,  /*                  */
						0x2222,  /*   X   X   X   X  */
						0x0000,  /*                  */
						0x8888,  /* X   X   X   X    */
						0x0000,  /*                  */
						0x2222,  /*   X   X   X   X  */
						0x0000,  /*                  */
						0x8888,  /* X   X   X   X    */
						0x0000,  /*                  */
						0x2222,  /*   X   X   X   X  */
						0x0000,  /*                  */
						0x8888,  /* X   X   X   X    */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS cmos_pp_lay = {LAYERT2,COLORT2, SOLIDC, PATTERNED,
/* pseudo-poly layer */	{0x0808, /*     X       X    */
						0x0404,  /*      X       X   */
						0x0202,  /*       X       X  */
						0x0101,  /*        X       X */
						0x8080,  /* X       X        */
						0x4040,  /*  X       X       */
						0x2020,  /*   X       X      */
						0x1010,  /*    X       X     */
						0x0808,  /*     X       X    */
						0x0404,  /*      X       X   */
						0x0202,  /*       X       X  */
						0x0101,  /*        X       X */
						0x8080,  /* X       X        */
						0x4040,  /*  X       X       */
						0x2020,  /*   X       X      */
						0x1010}, /*    X       X     */
						NOVARIABLE, 0};
static GRAPHICS cmos_pd_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
/* pseudo-diff layer */	{0x0000, /*                  */
						0x0303,  /*       XX      XX */
						0x4848,  /*  X  X    X  X    */
						0x0303,  /*       XX      XX */
						0x0000,  /*                  */
						0x3030,  /*   XX      XX     */
						0x8484,  /* X    X  X    X   */
						0x3030,  /*   XX      XX     */
						0x0000,  /*                  */
						0x0303,  /*       XX      XX */
						0x4848,  /*  X  X    X  X    */
						0x0303,  /*       XX      XX */
						0x0000,  /*                  */
						0x3030,  /*   XX      XX     */
						0x8484,  /* X    X  X    X   */
						0x3030}, /*   XX      XX     */
						NOVARIABLE, 0};
static GRAPHICS cmos_ppl_lay = {LAYERT4,COLORT4, SOLIDC, PATTERNED,
/* pseudo-P+ layer */	{0x1000, /*    X             */
						0x0020,  /*           X      */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0001,  /*                X */
						0x0200,  /*       X          */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x1000,  /*    X             */
						0x0020,  /*           X      */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0001,  /*                X */
						0x0200,  /*       X          */
						0x0000,  /*                  */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS cmos_pw_lay = {LAYERT5,COLORT5, SOLIDC, PATTERNED,
/* pseudo-P-well layer */{0x0000,/*                  */
						0x00C0,  /*         XX       */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x00C0,  /*         XX       */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000, /*                  */
						0x00C0,  /*         XX       */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x00C0,  /*         XX       */
						0x0000,  /*                  */
						0x0000}, /*                  */
						NOVARIABLE, 0};

/* these tables must be updated together */
GRAPHICS *cmos_layers[MAXLAYERS+1] = {&cmos_m_lay, &cmos_p_lay,
	&cmos_d_lay, &cmos_pl_lay, &cmos_c_lay, &cmos_oc_lay, &cmos_w_lay,
	&cmos_o_lay, &cmos_tr_lay, &cmos_pm_lay, &cmos_pp_lay, &cmos_pd_lay,
	&cmos_ppl_lay, &cmos_pw_lay, NOGRAPHICS};
static CHAR *cmos_layer_names[MAXLAYERS] = {x_("Metal"), x_("Polysilicon"),
	x_("Diffusion"), x_("P+"), x_("Contact-Cut"), x_("Ohmic-Cut"), x_("P-Well"),
	x_("Overglass"), x_("Transistor"), x_("Pseudo-Metal"), x_("Pseudo-Polysilicon"),
	x_("Pseudo-Diffusion"), x_("Pseudo-P+"), x_("Pseudo-P-Well")};
static CHAR *cmos_cif_layers[MAXLAYERS] = {x_("CM"), x_("CP"), x_("CD"), x_("CS"), x_("CC"),
	x_("CC"), x_("CW"), x_("CG"), x_(""), x_(""), x_(""), x_(""), x_(""), x_("")};
static INTBIG cmos_layer_function[MAXLAYERS] = {LFMETAL1|LFTRANS1, LFPOLY1|LFTRANS2,
	LFDIFF|LFTRANS3, LFIMPLANT|LFPTYPE|LFTRANS4, LFCONTACT1, LFCONTACT2,
	LFWELL|LFPTYPE|LFTRANS5, LFOVERGLASS, LFTRANSISTOR|LFPSEUDO,
	LFMETAL1|LFPSEUDO|LFTRANS1, LFPOLY1|LFPSEUDO|LFTRANS2,
	LFDIFF|LFPSEUDO|LFTRANS3, LFIMPLANT|LFPTYPE|LFPSEUDO|LFTRANS4,
	LFWELL|LFPTYPE|LFPSEUDO|LFTRANS5};
static CHAR *cmos_layer_letters[MAXLAYERS] = {x_("mb"), x_("pr"), x_("dg"), x_("s"),
	x_("c"), x_("o"), x_("w"), x_("v"), x_("t"), x_("M"), x_("P"), x_("D"), x_("S"), x_("W")};

/* The low 5 bits map Metal, Polysilicon, Diffusion, P+, and P-Well */
static TECH_COLORMAP cmos_colmap[32] =
{                  /*     well p diffusion polysilicon metal */
	{200,200,200}, /* 0:                                     */
	{  0,  0,255}, /* 1:                               metal */
	{223,  0,  0}, /* 2:                   polysilicon       */
	{150, 20,150}, /* 3:                   polysilicon+metal */
	{  0,255,  0}, /* 4:         diffusion                   */
	{  0,160,160}, /* 5:         diffusion+            metal */
	{180,130,  0}, /* 6:         diffusion+polysilicon       */
	{ 55, 70,140}, /* 7:         diffusion+polysilicon+metal */
	{255,190,  6}, /* 8:       p                             */
	{100,100,200}, /* 9:       p+                      metal */
	{255,114,  1}, /* 10:      p+          polysilicon       */
	{ 70, 50,150}, /* 11:      p+          polysilicon+metal */
	{180,255,  0}, /* 12:      p+diffusion                   */
	{ 40,160,160}, /* 13:      p+diffusion+            metal */
	{200,180, 70}, /* 14:      p+diffusion+polysilicon       */
	{ 60, 60,130}, /* 15:      p+diffusion+polysilicon+metal */
	{170,140, 30}, /* 16: well+                              */
	{  0,  0,180}, /* 17: well+                        metal */
	{200,130, 10}, /* 18: well+            polysilicon       */
	{ 60,  0,140}, /* 19: well+            polysilicon+metal */
	{156,220,  3}, /* 20: well+  diffusion                   */
	{  0,120,120}, /* 21: well+  diffusion+            metal */
	{170,170,  0}, /* 22: well+  diffusion+polysilicon       */
	{ 35, 50,120}, /* 23: well+  diffusion+polysilicon+metal */
	{200,160, 20}, /* 24: well+p                             */
	{ 65, 85,140}, /* 25: well+p+                      metal */
	{170, 60, 80}, /* 26: well+p+          polysilicon       */
	{ 50, 30,130}, /* 27: well+p+          polysilicon+metal */
	{ 60,190,  0}, /* 28: well+p+diffusion                   */
	{ 30,110,100}, /* 29: well+p+diffusion+            metal */
	{150, 90,  0}, /* 30: well+p+diffusion+polysilicon       */
	{ 40, 40,110}, /* 31: well+p+diffusion+polysilicon+metal */
};

/******************** DESIGN RULES ********************/

/* layers that can connect to other layers when electrically disconnected */
static INTBIG cmos_unconnectedtable[] = {
/*            M  P  D  P  C  O  W  O  T  M  P  D  P  W */
/*            e  o  i     u  c  e  v  r  e  o  i  P  e */
/*            t  l  f     t  u  l  e  a  t  l  f     l */
/*            a  y  f        t  l  r  n  a  y  f     l */
/*            l                    g  s  l  P  P     P */
/*                                 l     P             */
/* Metal  */ K3,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* Poly   */    K2,K1,K2,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* Diff   */       K3,K2,XX,K5,XX,XX,XX,XX,XX,XX,XX,XX,
/* P      */          K2,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* Cut    */             XX,XX,XX,XX,K2,XX,XX,XX,XX,XX,
/* Ocut   */                XX,XX,XX,K2,XX,XX,XX,XX,XX,
/* Well   */                   K2,XX,XX,XX,XX,XX,XX,XX,
/* Overgl */                      XX,XX,XX,XX,XX,XX,XX,
/* Trans  */                         XX,XX,XX,XX,XX,XX,
/* MetalP */                            XX,XX,XX,XX,XX,
/* PolyP  */                               XX,XX,XX,XX,
/* DiffP  */                                  XX,XX,XX,
/* PP     */                                     XX,XX,
/* WellP  */                                        XX,
};

/******************** ARCS ********************/

#define ARCPROTOCOUNT   4
#define AMETAL          0	/* metal              */
#define APOLY           1	/* polysilicon        */
#define ADIFP           2	/* diffusion-p+       */
#define ADIFW           3	/* diffusion-well     */

/* metal arc */
static TECH_ARCLAY cmos_al_m[] = {{ LMETAL,0,FILLED }};
static TECH_ARCS cmos_a_m = {
	x_("Metal"),K3,AMETAL,NOARCPROTO,								/* name */
	1,cmos_al_m,													/* layers */
	(APMETAL1<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* polysilicon arc */
static TECH_ARCLAY cmos_al_p[] = {{ LPOLY,0,FILLED }};
static TECH_ARCS cmos_a_p = {
	x_("Polysilicon"),K2,APOLY,NOARCPROTO,							/* name */
	1,cmos_al_p,													/* layers */
	(APPOLY1<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* diffusion-p arc */
static TECH_ARCLAY cmos_al_dp[] = {{ LDIFF,K4,FILLED}, {LP,0,FILLED }};
static TECH_ARCS cmos_a_dp = {
	x_("Diffusion-p"),K6,ADIFP,NOARCPROTO,							/* name */
	2,cmos_al_dp,													/* layers */
	(APDIFFP<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* diffusion-well arc */
static TECH_ARCLAY cmos_al_dw[] = {{ LDIFF,K6,FILLED}, {LWELL,0,FILLED }};
static TECH_ARCS cmos_a_dw = {
	x_("Diffusion-well"),K8,ADIFW,NOARCPROTO,						/* name */
	2,cmos_al_dw,													/* layers */
	(APDIFFN<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

TECH_ARCS *cmos_arcprotos[ARCPROTOCOUNT+1] = {
	&cmos_a_m, &cmos_a_p, &cmos_a_dp, &cmos_a_dw, ((TECH_ARCS *)-1)};

static INTBIG cmos_arc_widoff[ARCPROTOCOUNT] = {0,0,K4,K6};

/******************** PORTINST CONNECTIONS ********************/

/* these values are replaced with actual arcproto addresses */
static INTBIG cmos_pc_m[]    = {-1, AMETAL, ALLGEN, -1};
static INTBIG cmos_pc_p[]    = {-1, APOLY, ALLGEN, -1};
static INTBIG cmos_pc_pm[]   = {-1, APOLY, AMETAL, ALLGEN, -1};
static INTBIG cmos_pc_dP[]   = {-1, ADIFP, ALLGEN, -1};
static INTBIG cmos_pc_dW[]   = {-1, ADIFW, ALLGEN, -1};
static INTBIG cmos_pc_dPm[]  = {-1, ADIFP, AMETAL, ALLGEN, -1};
static INTBIG cmos_pc_dWm[]  = {-1, ADIFW, AMETAL, ALLGEN, -1};
static INTBIG cmos_pc_null[] = {-1, ALLGEN, -1};

/******************** NODES ********************/

#define NODEPROTOCOUNT	19
#define NMETALP          1	/* metal pin */
#define NPOLYP           2	/* polysilicon pin */
#define NDIFPP           3	/* diffusion-p+ pin */
#define NDIFWP           4	/* diffusion-well pin */
#define NMETPOLC         5	/* metal-polysilicon contact */
#define NMETDIFPC        6	/* metal-diffusion-p+ contact */
#define NMETDIFWC        7	/* metal-diffusion-well contact */
#define NTRANSP          8	/* transistor-p+ */
#define NTRANSW          9	/* transistor-well */
#define NSPLITWC        10	/* split-well contact */
#define NSPLITPC        11	/* split-p+ contact */
#define NMETALN         12	/* metal node */
#define NPOLYN          13	/* polysilicon node */
#define NDIFN           14	/* diffusion node */
#define NPN             15	/* p+ node */
#define NCUTN           16	/* cut node */
#define NOCUTN          17	/* ohmic-cut node */
#define NWELLN          18	/* p-well node */
#define NGLASSN         19	/* overglass node */

static INTBIG cmos_cutbox[8]  = {LEFTIN1,  BOTIN1,  LEFTIN3,  BOTIN3};/* adjust */
static INTBIG cmos_fullbox[8] = {LEFTEDGE, BOTEDGE, RIGHTEDGE,TOPEDGE};
static INTBIG cmos_in2box[8]  = {LEFTIN2,  BOTIN2,  RIGHTIN2, TOPIN2};
static INTBIG cmos_in3box[8]  = {LEFTIN3,  BOTIN3,  RIGHTIN3, TOPIN3};
static INTBIG cmos_tradbox[8] = {LEFTIN2,  BOTEDGE, RIGHTIN2, TOPEDGE};
static INTBIG cmos_trapbox[8] = {LEFTEDGE, BOTIN2,  RIGHTEDGE,TOPIN2};
static INTBIG cmos_trd1box[8] = {LEFTIN2,  TOPIN2,  RIGHTIN2, TOPEDGE};
static INTBIG cmos_trd2box[8] = {LEFTIN2,  BOTEDGE, RIGHTIN2, BOTIN2};
static INTBIG cmos_trwdbox[8] = {LEFTIN3,  BOTIN1,  RIGHTIN3, TOPIN1};
static INTBIG cmos_trwpbox[8] = {LEFTIN1,  BOTIN3,  RIGHTIN1, TOPIN3};
static INTBIG cmos_trd3box[8] = {LEFTIN3,  TOPIN3,  RIGHTIN3, TOPIN1};
static INTBIG cmos_trd4box[8] = {LEFTIN3,  BOTIN1,  RIGHTIN3, BOTIN3};
static INTBIG cmos_spl1box[8] = {LEFTIN1H, BOTIN1H, CENTER,   TOPIN1H};
static INTBIG cmos_spl2box[8] = {CENTER,   BOTIN4,  RIGHTIN4, TOPIN4};
static INTBIG cmos_spl3box[8] = {LEFTIN4,  BOTIN4,  CENTER,   TOPIN4};
static INTBIG cmos_spl4box[8] = {LEFTIN2,  BOTIN2,  RIGHTEDGE,TOPIN2};
static INTBIG cmos_spl5box[8] = {LEFTEDGE, BOTEDGE, CENTERR1, TOPEDGE};
static INTBIG cmos_spl6box[8] = {LEFTIN3,  BOTIN3,  CENTERR1, TOPIN3};
static INTBIG cmos_spl7box[8] = {CENTERR1, BOTIN3,  RIGHTIN1, TOPIN3};

/* metal-pin */
static TECH_PORTS cmos_pm_p[] = {					/* ports */
	{cmos_pc_m, x_("metal"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON cmos_pm_l[] = {					/* layers */
	{LMETALP, 0, 4, CROSSED, BOX, cmos_fullbox}};
static TECH_NODES cmos_pm = {
	x_("Metal-Pin"),NMETALP,NONODEPROTO,			/* name */
	K3,K3,											/* size */
	1,cmos_pm_p,									/* ports */
	1,cmos_pm_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* polysilicon-pin */
static TECH_PORTS cmos_pp_p[] = {					/* ports */
	{cmos_pc_p, x_("polysilicon"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON cmos_pp_l[] = {					/* layers */
	{LPOLYP, 0, 4, CROSSED, BOX, cmos_fullbox}};
static TECH_NODES cmos_pp = {
	x_("Polysilicon-Pin"),NPOLYP,NONODEPROTO,		/* name */
	K2,K2,											/* size */
	1,cmos_pp_p,									/* ports */
	1,cmos_pp_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* diffusion-p+-pin */
static TECH_PORTS cmos_pdp_p[] = {					/* ports */
	{cmos_pc_dP, x_("diff-p"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3}};
static TECH_POLYGON cmos_pdp_l[] = {				/* layers */
	{LDIFFP, 0, 4, CROSSED, BOX, cmos_in2box},
	{LPP,   -1, 4, CROSSED, BOX, cmos_fullbox}};
static TECH_NODES cmos_pdp = {
	x_("Diffusion-P-Pin"),NDIFPP,NONODEPROTO,		/* name */
	K6,K6,											/* size */
	1,cmos_pdp_p,									/* ports */
	2,cmos_pdp_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* diffusion-p-well-pin */
static TECH_PORTS cmos_pdw_p[] = {					/* ports */
	{cmos_pc_dW, x_("diff-w"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN4, BOTIN4, RIGHTIN4, TOPIN4}};
static TECH_POLYGON cmos_pdw_l[] = {				/* layers */
	{LDIFFP, 0, 4, CROSSED, BOX, cmos_in3box},
	{LWELLP,-1, 4, CROSSED, BOX, cmos_fullbox}};
static TECH_NODES cmos_pdw = {
	x_("Diffusion-Well-Pin"),NDIFWP,NONODEPROTO,	/* name */
	K8,K8,											/* size */
	1,cmos_pdw_p,									/* ports */
	2,cmos_pdw_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* metal-polysilicon-contact */
static TECH_PORTS cmos_mp_p[] = {					/* ports */
	{cmos_pc_pm, x_("metal-poly"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON cmos_mp_l[] = {					/* layers */
	{LMETAL, 0, 4, FILLEDRECT, BOX, cmos_fullbox},
	{LPOLY,  0, 4, FILLEDRECT, BOX, cmos_fullbox},
	{LCUT,  -1, 4, CLOSEDRECT, BOX, cmos_cutbox}};
static TECH_NODES cmos_mp = {
	x_("Metal-Polysilicon-Con"),NMETPOLC,NONODEPROTO,	/* name */
	K4,K4,											/* size */
	1,cmos_mp_p,									/* ports */
	3,cmos_mp_l,									/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,K1,K2,0,0,0,0};					/* characteristics */

/* metal-diffusion-p+-contact */
static TECH_PORTS cmos_mdp_p[] = {					/* ports */
	{cmos_pc_dPm, x_("metal-diff-p"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3}};
static TECH_POLYGON cmos_mdp_l[] = {				/* layers */
	{LMETAL, 0, 4, FILLEDRECT, BOX, cmos_in2box},
	{LDIFF,  0, 4, FILLEDRECT, BOX, cmos_in2box},
	{LP,    -1, 4, FILLEDRECT, BOX, cmos_fullbox},
	{LCUT,  -1, 4, CLOSEDRECT, BOX, cmos_cutbox}};
static TECH_NODES cmos_mdp = {
	x_("Metal-Diff-P-Con"),NMETDIFPC,NONODEPROTO,	/* name */
	K8,K8,											/* size */
	1,cmos_mdp_p,									/* ports */
	4,cmos_mdp_l,									/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,K1,K2,0,0,0,0};					/* characteristics */

/* metal-diffusion-p-well-contact */
static TECH_PORTS cmos_mdw_p[] = {					/* ports */
	{cmos_pc_dWm, x_("metal-diff-w"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN4, BOTIN4, RIGHTIN4, TOPIN4}};
static TECH_POLYGON cmos_mdw_l[] = {				/* layers */
	{LMETAL, 0, 4, FILLEDRECT, BOX, cmos_in3box},
	{LDIFF,  0, 4, FILLEDRECT, BOX, cmos_in3box},
	{LWELL, -1, 4, FILLEDRECT, BOX, cmos_fullbox},
	{LCUT,  -1, 4, CLOSEDRECT, BOX, cmos_cutbox}};
static TECH_NODES cmos_mdw = {
	x_("Metal-Diff-Well-Con"),NMETDIFWC,NONODEPROTO,	/* name */
	K10,K10,										/* size */
	1,cmos_mdw_p,									/* ports */
	4,cmos_mdw_l,									/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,K1,K2,0,0,0,0};					/* characteristics */

/* transistor-p+ */
static TECH_PORTS cmos_tra_p[] = {					/* ports */
	{cmos_pc_p, x_("trans-poly-left"),  NOPORTPROTO, (180<<PORTANGLESH)|
		(85<<PORTARANGESH),                LEFTIN1,  BOTIN3, LEFTIN1,  TOPIN3},
	{cmos_pc_dP,x_("trans-diff-top"),   NOPORTPROTO, (90<<PORTANGLESH)|
		(85<<PORTARANGESH)|(1<<PORTNETSH), LEFTIN3,  TOPIN1, RIGHTIN3, TOPIN1},
	{cmos_pc_p, x_("trans-poly-right"), NOPORTPROTO, (0<<PORTANGLESH)|
		(85<<PORTARANGESH),                RIGHTIN1, BOTIN3, RIGHTIN1, TOPIN3},
	{cmos_pc_dP,x_("trans-diff-bottom"),NOPORTPROTO, (270<<PORTANGLESH)|
		(85<<PORTARANGESH)|(2<<PORTNETSH), LEFTIN3,  BOTIN1, RIGHTIN3, BOTIN1}};
static TECH_SERPENT cmos_tra_l[] = {				/* graphical layers */
	{{LDIFF,  1, 4, FILLEDRECT, BOX, cmos_tradbox}, K3, K3,  0,  0},
	{{LPOLY,  0, 4, FILLEDRECT, BOX, cmos_trapbox}, K1, K1, K2, K2},
	{{LP,    -1, 4, FILLEDRECT, BOX, cmos_fullbox}, K3, K3, K2, K2},
	{{LTRANS,-1, 4, FILLEDRECT, BOX, cmos_in2box},  K1, K1,  0,  0}};
static TECH_SERPENT cmos_traE_l[] = {				/* electric layers */
	{{LDIFF,  1, 4, FILLEDRECT, BOX, cmos_trd1box}, K3,  0,  0,  0},
	{{LDIFF,  3, 4, FILLEDRECT, BOX, cmos_trd2box},  0, K3,  0,  0},
	{{LPOLY,  0, 4, FILLEDRECT, BOX, cmos_trapbox}, K1, K1, K2, K2},
	{{LP,    -1, 4, FILLEDRECT, BOX, cmos_fullbox}, K3, K3, K2, K2},
	{{LTRANS,-1, 4, FILLEDRECT, BOX, cmos_in2box},  K1, K1,  0,  0}};
static TECH_NODES cmos_tra = {
	x_("Transistor"),NTRANSP,NONODEPROTO,			/* name */
	K6,K6,											/* size */
	4,cmos_tra_p,									/* ports */
	4,(TECH_POLYGON *)0,							/* layers */
	NODESHRINK|(NPTRANMOS<<NFUNCTIONSH)|HOLDSTRACE,	/* userbits */
	SERPTRANS,5,K1,K1,K2,K1,K1,cmos_tra_l,cmos_traE_l};	/* characteristics */

/* transistor-p-well */
static TECH_PORTS cmos_trw_p[] = {					/* ports */
	{cmos_pc_p,   x_("transw-poly-left"),  NOPORTPROTO, (180<<PORTANGLESH)|
		(85<<PORTARANGESH),                LEFTIN2,  BOTIN4, LEFTIN2,  TOPIN4},
	{cmos_pc_dW,  x_("transw-diff-top"),   NOPORTPROTO, (90<<PORTANGLESH)|
		(85<<PORTARANGESH)|(1<<PORTNETSH), LEFTIN4,  TOPIN2, RIGHTIN4, TOPIN2},
	{cmos_pc_p,   x_("transw-poly-right"), NOPORTPROTO, (0<<PORTANGLESH)|
		(85<<PORTARANGESH),                RIGHTIN2, BOTIN4, RIGHTIN2, TOPIN4},
	{cmos_pc_dW,  x_("transw-diff-bottom"),NOPORTPROTO, (270<<PORTANGLESH)|
		(85<<PORTARANGESH)|(2<<PORTNETSH), LEFTIN4,  BOTIN2, RIGHTIN4, BOTIN2}};
static TECH_SERPENT cmos_trw_l[] = {				/* graphical layers */
	{{LDIFF,  1, 4, FILLEDRECT, BOX, cmos_trwdbox}, K3, K3,  0,  0},
	{{LPOLY,  0, 4, FILLEDRECT, BOX, cmos_trwpbox}, K1, K1, K2, K2},
	{{LWELL, -1, 4, FILLEDRECT, BOX, cmos_fullbox}, K4, K4, K3, K3},
	{{LTRANS,-1, 4, FILLEDRECT, BOX, cmos_in3box},  K1, K1,  0,  0}};
static TECH_SERPENT cmos_trwE_l[] = {				/* electric layers */
	{{LDIFF,  1, 4, FILLEDRECT, BOX, cmos_trd3box}, K3,  0,  0,  0},
	{{LDIFF,  3, 4, FILLEDRECT, BOX, cmos_trd4box},  0, K3,  0,  0},
	{{LPOLY,  0, 4, FILLEDRECT, BOX, cmos_trwpbox}, K1, K1, K2, K2},
	{{LWELL, -1, 4, FILLEDRECT, BOX, cmos_fullbox}, K4, K4, K3, K3},
	{{LTRANS,-1, 4, FILLEDRECT, BOX, cmos_in3box},  K1, K1,  0,  0}};
static TECH_NODES cmos_trw = {
	x_("Transistor-Well"),NTRANSW,NONODEPROTO,		/* name */
	K8,K8,											/* size */
	4,cmos_trw_p,									/* ports */
	4,(TECH_POLYGON *)0,							/* layers */
	NODESHRINK|(NPTRAPMOS<<NFUNCTIONSH)|HOLDSTRACE,	/* userbits */
	SERPTRANS,5,K1,K1,K2,K1,K1,cmos_trw_l,cmos_trwE_l};	/* characteristics */

/* metal-diffusion-p-well-split-cut */
static TECH_PORTS cmos_mds_p[] = {					/* ports */
	{cmos_pc_dWm, x_("metal-diff-splw-r"), NOPORTPROTO, (0<<PORTANGLESH)|
		(90<<PORTARANGESH),  CENTERR1, BOTIN4, RIGHTIN4, TOPIN4},
	{cmos_pc_m,   x_("metal-diff-splw-l"), NOPORTPROTO, (180<<PORTANGLESH)|
		(90<<PORTARANGESH),  LEFTIN4,  BOTIN4, CENTERL1, TOPIN4}};
static TECH_POLYGON cmos_mds_l[] = {				/* layers */
	{LMETAL, 1, 4, FILLEDRECT, BOX, cmos_in3box},
	{LDIFF,  0, 4, FILLEDRECT, BOX, cmos_in3box},
	{LP,    -1, 4, FILLEDRECT, BOX, cmos_spl1box},
	{LWELL, -1, 4, FILLEDRECT, BOX, cmos_fullbox},
	{LCUT,  -1, 4, CLOSEDRECT, BOX, cmos_spl2box},
	{LOCUT, -1, 4, CROSSED,    BOX, cmos_spl3box}};
static TECH_NODES cmos_mds = {
	x_("Metal-Diff-Split-Cut"),NSPLITWC,NONODEPROTO,/* name */
	K14,K10,										/* size */
	2,cmos_mds_p,									/* ports */
	6,cmos_mds_l,									/* layers */
	(NPWELL<<NFUNCTIONSH),							/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* metal-diffusion-p+-split-contact */
static TECH_PORTS cmos_mdsn_p[] = {					/* ports */
	{cmos_pc_dPm, x_("metal-diff-splp-l"), NOPORTPROTO, (0<<PORTANGLESH)|
		(90<<PORTARANGESH),  LEFTIN3,  BOTIN3, CENTER,   TOPIN3},
	{cmos_pc_m,   x_("metal-diff-splp-r"), NOPORTPROTO, (180<<PORTANGLESH)|
		(90<<PORTARANGESH),  CENTERR2, BOTIN3, RIGHTIN1, TOPIN3}};
static TECH_POLYGON cmos_mdsn_l[] = {				/* layers */
	{LMETAL, 1, 4, FILLEDRECT, BOX, cmos_spl4box},
	{LDIFF,  0, 4, FILLEDRECT, BOX, cmos_spl4box},
	{LP,    -1, 4, FILLEDRECT, BOX, cmos_spl5box},
	{LCUT,  -1, 4, CLOSEDRECT, BOX, cmos_spl6box},
	{LOCUT, -1, 4, CROSSED,    BOX, cmos_spl7box}};
static TECH_NODES cmos_mdsn = {
	x_("Metal-Diff-SplitN-Cut"),NSPLITPC,NONODEPROTO,	/* name */
	K10,K8,											/* size */
	2,cmos_mdsn_p,									/* ports */
	5,cmos_mdsn_l,									/* layers */
	(NPSUBSTRATE<<NFUNCTIONSH),						/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* metal-node */
static TECH_PORTS cmos_m_p[] = {					/* ports */
	{cmos_pc_m, x_("metal"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON cmos_m_l[] = {					/* layers */
	{LMETAL, 0,  4, FILLEDRECT, BOX, cmos_fullbox}};
static TECH_NODES cmos_m = {
	x_("Metal-Node"),NMETALN,NONODEPROTO,			/* name */
	K3,K3,											/* size */
	1,cmos_m_p,										/* ports */
	1,cmos_m_l,										/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* polysilicon-node */
static TECH_PORTS cmos_p_p[] = {					/* ports */
	{cmos_pc_p, x_("polysilicon"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON cmos_p_l[] = {					/* layers */
	{LPOLY, 0, 4, FILLEDRECT, BOX, cmos_fullbox}};
static TECH_NODES cmos_p = {
	x_("Polysilicon-Node"),NPOLYN,NONODEPROTO,		/* name */
	K2,K2,											/* size */
	1,cmos_p_p,										/* ports */
	1,cmos_p_l,										/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* diffusion-node */
static TECH_PORTS cmos_d_p[] = {					/* ports */
	{cmos_pc_null, x_("diffusion"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON cmos_d_l[] = {					/* layers */
	{LDIFF, 0, 4, FILLEDRECT, BOX, cmos_fullbox}};
static TECH_NODES cmos_d = {
	x_("Diffusion-Node"),NDIFN,NONODEPROTO,			/* name */
	K2,K2,											/* size */
	1,cmos_d_p,										/* ports */
	1,cmos_d_l,										/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* p+-node */
static TECH_PORTS cmos_i_p[] = {					/* ports */
	{cmos_pc_null, x_("p+"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON cmos_i_l[] = {					/* layers */
	{LP, 0, 4, FILLEDRECT, BOX, cmos_fullbox}};
static TECH_NODES cmos_i = {
	x_("P-Node"),NPN,NONODEPROTO,					/* name */
	K2,K2,											/* size */
	1,cmos_i_p,										/* ports */
	1,cmos_i_l,										/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* cut-node */
static TECH_PORTS cmos_c_p[] = {					/* ports */
	{cmos_pc_null, x_("cut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON cmos_c_l[] = {					/* layers */
	{LCUT, 0, 4, CLOSEDRECT, BOX, cmos_fullbox}};
static TECH_NODES cmos_c = {
	x_("Cut-Node"),NCUTN,NONODEPROTO,				/* name */
	K2,K2,											/* size */
	1,cmos_c_p,										/* ports */
	1,cmos_c_l,										/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* ohmic-cut-node */
static TECH_PORTS cmos_co_p[] = {					/* ports */
	{cmos_pc_null, x_("ohmic-cut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON cmos_co_l[] = {					/* layers */
	{LOCUT, 0, 4, CROSSED,  BOX, cmos_fullbox}};
static TECH_NODES cmos_co = {
	x_("Ohmic-Cut-Node"),NOCUTN,NONODEPROTO,		/* name */
	K2,K2,											/* size */
	1,cmos_co_p,									/* ports */
	1,cmos_co_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* p-well-node */
static TECH_PORTS cmos_w_p[] = {					/* ports */
	{cmos_pc_null, x_("well"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON cmos_w_l[] = {					/* layers */
	{LWELL, 0, 4, FILLEDRECT, BOX, cmos_fullbox}};
static TECH_NODES cmos_w = {
	x_("Well-Node"),NWELLN,NONODEPROTO,				/* name */
	K4,K4,											/* size */
	1,cmos_w_p,										/* ports */
	1,cmos_w_l,										/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* overglass-node */
static TECH_PORTS cmos_o_p[] = {					/* ports */
	{cmos_pc_null, x_("overglass"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON cmos_o_l[] = {					/* layers */
	{LOVERGL, 0, 4, FILLEDRECT, BOX, cmos_fullbox}};
static TECH_NODES cmos_o = {
	x_("Overglass-Node"),NGLASSN,NONODEPROTO,		/* name */
	K2,K2,											/* size */
	1,cmos_o_p,										/* ports */
	1,cmos_o_l,										/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

TECH_NODES *cmos_nodeprotos[NODEPROTOCOUNT+1] = {
	&cmos_pm, &cmos_pp, &cmos_pdp, &cmos_pdw, &cmos_mp, &cmos_mdp,
	&cmos_mdw, &cmos_tra, &cmos_trw, &cmos_mds, &cmos_mdsn, &cmos_m,
	&cmos_p, &cmos_d, &cmos_i, &cmos_c, &cmos_co, &cmos_w, &cmos_o, ((TECH_NODES *)-1)};

static INTBIG cmos_node_widoff[NODEPROTOCOUNT*4] = {0,0,0,0, 0,0,0,0,
	K2,K2,K2,K2, K3,K3,K3,K3, 0,0,0,0, K2,K2,K2,K2, K3,K3,K3,K3, K2,K2,K2,K2,
	K3,K3,K3,K3, K3,K3,K3,K3, K2,0,K2,K2, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

/******************** VARIABLE AGGREGATION ********************/

TECH_VARIABLES cmos_variables[] =
{
	/* set general information about the technology */
	{x_("TECH_layer_names"), (CHAR *)cmos_layer_names, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_layer_function"), (CHAR *)cmos_layer_function, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_arc_width_offset"), (CHAR *)cmos_arc_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|(ARCPROTOCOUNT<<VLENGTHSH)},
	{x_("TECH_node_width_offset"), (CHAR *)cmos_node_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|((NODEPROTOCOUNT*4)<<VLENGTHSH)},

	/* set information for the USER tool */
	{x_("USER_color_map"), (CHAR *)cmos_colmap, 0.0,
		VCHAR|VDONTSAVE|VISARRAY|((sizeof cmos_colmap)<<VLENGTHSH)},
	{x_("USER_layer_letters"), (CHAR *)cmos_layer_letters, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},

	/* set information for the I/O tool */
	{x_("IO_cif_layer_names"), (CHAR *)cmos_cif_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},

	/* set information for the DRC tool */
	{x_("DRC_min_unconnected_distances"), (CHAR *)cmos_unconnectedtable, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|
			(((sizeof cmos_unconnectedtable)/SIZEOFINTBIG)<<VLENGTHSH)},
	{NULL, NULL, 0.0, 0}
};

/******************** ROUTINES ********************/

BOOLEAN cmos_initprocess(TECHNOLOGY *tech, INTBIG pass)
{
	Q_UNUSED( tech );
	Q_UNUSED( pass );
	return(FALSE);
}
