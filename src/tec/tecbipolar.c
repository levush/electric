/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tecbipolar.c
 * bipolar technology description
 * Written by: Burnie West, Schlumberger ATE
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

BOOLEAN bipolar_initprocess(TECHNOLOGY *tech, INTBIG pass);

/******************** LAYERS ********************/

#define MAXLAYERS 16
#define LM        0			/* Metal1 */
#define LM0       1			/* Metal2 */
#define LN        2			/* NPImplant */
#define LP        3			/* PPImplant */
#define LPD       4			/* Poly_Definition */
#define LFI       5			/* Field_Implant */
#define LI        6			/* Isolation */
#define LSI       7			/* Sink_Implant */
#define LNI       8			/* N_Implant */
#define LSE       9			/* Silicide_Exclusion */
#define LC        10		/* Contact */
#define LV        11		/* Via */
#define LSP       12		/* Scratch_Protection */
#define LB        13		/* Buried */
#define LPM       14		/* Pseudo_Metal1 */
#define LPM0      15		/* Pseudo_Metal2 */

static GRAPHICS bipolar_M_lay = {LAYERT1, COLORT1, SOLIDC, PATTERNED,
	{0x0808, /*     X       X    */
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
static GRAPHICS bipolar_M0_lay = {LAYERT4, COLORT4, SOLIDC, PATTERNED,
	{0x2222, /*   X   X   X   X  */
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
static GRAPHICS bipolar_N_lay = {LAYERO, DGREEN, PATTERNED, PATTERNED,
	{0xcccc, /* XX  XX  XX  XX   */
	0xc0c0,  /* XX      XX       */
	0xcccc,  /* XX  XX  XX  XX   */
	0xc0c0,  /* XX      XX       */
	0xcccc,  /* XX  XX  XX  XX   */
	0xc0c0,  /* XX      XX       */
	0xcccc,  /* XX  XX  XX  XX   */
	0xc0c0,  /* XX      XX       */
	0xcccc,  /* XX  XX  XX  XX   */
	0xc0c0,  /* XX      XX       */
	0xcccc,  /* XX  XX  XX  XX   */
	0xc0c0,  /* XX      XX       */
	0xcccc,  /* XX  XX  XX  XX   */
	0xc0c0,  /* XX      XX       */
	0xcccc,  /* XX  XX  XX  XX   */
	0xc0c0}, /* XX      XX       */
	NOVARIABLE, 0};
static GRAPHICS bipolar_P_lay = {LAYERO, DBLUE, PATTERNED, PATTERNED,
	{0x0000, /*                  */
	0xcccc,  /* XX  XX  XX  XX   */
	0x0000,  /*                  */
	0xcccc,  /* XX  XX  XX  XX   */
	0x0000,  /*                  */
	0xcccc,  /* XX  XX  XX  XX   */
	0x0000,  /*                  */
	0xcccc,  /* XX  XX  XX  XX   */
	0x0000,  /*                  */
	0xcccc,  /* XX  XX  XX  XX   */
	0x0000,  /*                  */
	0xcccc,  /* XX  XX  XX  XX   */
	0x0000,  /*                  */
	0xcccc,  /* XX  XX  XX  XX   */
	0x0000,  /*                  */
	0xcccc}, /* XX  XX  XX  XX   */
	NOVARIABLE, 0};
static GRAPHICS bipolar_PD_lay = {LAYERT2, COLORT2, SOLIDC, PATTERNED,
	{0x1111, /*    X   X   X   X */
	0x3030,  /*   XX      XX     */
	0x7171,  /*  XXX   X XXX   X */
	0x3030,  /*   XX      XX     */
	0x1111,  /*    X   X   X   X */
	0x0303,  /*       XX      XX */
	0x1717,  /*    X XXX   X XXX */
	0x0303,  /*       XX      XX */
	0x1111,  /*    X   X   X   X */
	0x3030,  /*   XX      XX     */
	0x7171,  /*  XXX   X XXX   X */
	0x3030,  /*   XX      XX     */
	0x1111,  /*    X   X   X   X */
	0x0303,  /*       XX      XX */
	0x1717,  /*    X XXX   X XXX */
	0x0303}, /*       XX      XX */
	NOVARIABLE, 0};
static GRAPHICS bipolar_FI_lay = {LAYERO, MAGENTA, PATTERNED, PATTERNED,
	{0x0000, /*                  */
	0x4141,  /*  X     X X     X */
	0x2222,  /*   X   X   X   X  */
	0x1414,  /*    X X     X X   */
	0x0000,  /*                  */
	0x1414,  /*    X X     X X   */
	0x2222,  /*   X   X   X   X  */
	0x4141,  /*  X     X X     X */
	0x0000,  /*                  */
	0x4141,  /*  X     X X     X */
	0x2222,  /*   X   X   X   X  */
	0x1414,  /*    X X     X X   */
	0x0000,  /*                  */
	0x1414,  /*    X X     X X   */
	0x2222,  /*   X   X   X   X  */
	0x4141}, /*  X     X X     X */
	NOVARIABLE, 0};
static GRAPHICS bipolar_I_lay = {LAYERT3, COLORT3, SOLIDC, PATTERNED,
	{0x5555, /*  X X X X X X X X */
	0xaaaa,  /* X X X X X X X X  */
	0x5555,  /*  X X X X X X X X */
	0xaaaa,  /* X X X X X X X X  */
	0x5555,  /*  X X X X X X X X */
	0xaaaa,  /* X X X X X X X X  */
	0x5555,  /*  X X X X X X X X */
	0xaaaa,  /* X X X X X X X X  */
	0x5555,  /*  X X X X X X X X */
	0xaaaa,  /* X X X X X X X X  */
	0x5555,  /*  X X X X X X X X */
	0xaaaa,  /* X X X X X X X X  */
	0x5555,  /*  X X X X X X X X */
	0xaaaa,  /* X X X X X X X X  */
	0x5555,  /*  X X X X X X X X */
	0xaaaa}, /* X X X X X X X X  */
	NOVARIABLE, 0};
static GRAPHICS bipolar_SI_lay = {LAYERO, PURPLE, PATTERNED, PATTERNED,
	{0x1111, /*    X   X   X   X */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0x1111,  /*    X   X   X   X */
	0x5555,  /*  X X X X X X X X */
	0x1111,  /*    X   X   X   X */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0x1111,  /*    X   X   X   X */
	0x5555,  /*  X X X X X X X X */
	0x1111,  /*    X   X   X   X */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0x1111,  /*    X   X   X   X */
	0x5555,  /*  X X X X X X X X */
	0x1111,  /*    X   X   X   X */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0x1111,  /*    X   X   X   X */
	0x5555}, /*  X X X X X X X X */
	NOVARIABLE, 0};
static GRAPHICS bipolar_NI_lay = {LAYERO, BROWN, PATTERNED, PATTERNED,
	{0x1c1c, /*    XXX     XXX   */
	0x0e0e,  /*     XXX     XXX  */
	0x0707,  /*      XXX     XXX */
	0x8383,  /* X     XXX     XX */
	0xc1c1,  /* XX     XXX     X */
	0xe0e0,  /* XXX     XXX      */
	0x7070,  /*  XXX     XXX     */
	0x3838,  /*   XXX     XXX    */
	0x1c1c,  /*    XXX     XXX   */
	0x0e0e,  /*     XXX     XXX  */
	0x0707,  /*      XXX     XXX */
	0x8383,  /* X     XXX     XX */
	0xc1c1,  /* XX     XXX     X */
	0xe0e0,  /* XXX     XXX      */
	0x7070,  /*  XXX     XXX     */
	0x3838}, /*   XXX     XXX    */
	NOVARIABLE, 0};
static GRAPHICS bipolar_SE_lay = {LAYERT5, COLORT5, SOLIDC, PATTERNED,
	{0xafaf, /* X X XXXXX X XXXX */
	0x8888,  /* X   X   X   X    */
	0xfafa,  /* XXXXX X XXXXX X  */
	0x8888,  /* X   X   X   X    */
	0xafaf,  /* X X XXXXX X XXXX */
	0x8888,  /* X   X   X   X    */
	0xfafa,  /* XXXXX X XXXXX X  */
	0x8888,  /* X   X   X   X    */
	0xafaf,  /* X X XXXXX X XXXX */
	0x8888,  /* X   X   X   X    */
	0xfafa,  /* XXXXX X XXXXX X  */
	0x8888,  /* X   X   X   X    */
	0xafaf,  /* X X XXXXX X XXXX */
	0x8888,  /* X   X   X   X    */
	0xfafa,  /* XXXXX X XXXXX X  */
	0x8888}, /* X   X   X   X    */
	NOVARIABLE, 0};
static GRAPHICS bipolar_C_lay = {LAYERO, YELLOW, SOLIDC, PATTERNED,
	{0xffff, /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff}, /* XXXXXXXXXXXXXXXX */
	NOVARIABLE, 0};
static GRAPHICS bipolar_V_lay = {LAYERO, BLACK, SOLIDC, PATTERNED,
	{0xffff, /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff}, /* XXXXXXXXXXXXXXXX */
	NOVARIABLE, 0};
static GRAPHICS bipolar_SP_lay = {LAYERO, DGRAY, SOLIDC, PATTERNED,
	{0x1c1c, /*    XXX     XXX   */
	0x3e3e,  /*   XXXXX   XXXXX  */
	0x3636,  /*   XX XX   XX XX  */
	0x3e3e,  /*   XXXXX   XXXXX  */
	0x1c1c,  /*    XXX     XXX   */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x1c1c,  /*    XXX     XXX   */
	0x3e3e,  /*   XXXXX   XXXXX  */
	0x3636,  /*   XX XX   XX XX  */
	0x3e3e,  /*   XXXXX   XXXXX  */
	0x1c1c,  /*    XXX     XXX   */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000}, /*                  */
	NOVARIABLE, 0};
static GRAPHICS bipolar_B_lay = {LAYERO, YELLOW, SOLIDC, PATTERNED,
	{0xffff, /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff,  /* XXXXXXXXXXXXXXXX */
	0xffff}, /* XXXXXXXXXXXXXXXX */
	NOVARIABLE, 0};
static GRAPHICS bipolar_PM_lay = {LAYERT1, COLORT1, SOLIDC, PATTERNED,
	{0x2222, /*   X   X   X   X  */
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
static GRAPHICS bipolar_PM0_lay = {LAYERT4, COLORT4, SOLIDC, PATTERNED,
	{0x0808, /*     X       X    */
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

GRAPHICS *bipolar_layers[MAXLAYERS+1] = {&bipolar_M_lay, &bipolar_M0_lay,
	&bipolar_N_lay, &bipolar_P_lay, &bipolar_PD_lay, &bipolar_FI_lay,
	&bipolar_I_lay, &bipolar_SI_lay, &bipolar_NI_lay, &bipolar_SE_lay,
	&bipolar_C_lay, &bipolar_V_lay, &bipolar_SP_lay, &bipolar_B_lay,
	&bipolar_PM_lay, &bipolar_PM0_lay, NOGRAPHICS};
static CHAR *bipolar_layer_names[MAXLAYERS] = {x_("Metal1"), x_("Metal2"), x_("NPImplant"),
	x_("PPImplant"), x_("Poly_Definition"), x_("Field_Implant"), x_("Isolation"), x_("Sink_Implant"),
	x_("N_Implant"), x_("Silicide_Exclusion"), x_("Contact"), x_("Via"), x_("Scratch_Protection"),
	x_("Buried"), x_("Pseudo_Metal1"), x_("Pseudo_Metal2")};
static CHAR *bipolar_cif_layers[MAXLAYERS] = {x_("IM1"), x_("IM2"), x_("INP"), x_("IPP"), x_("IP"), x_("IF"), x_("II"),
	x_("IS"), x_("INM"), x_("ISE"), x_("IC"), x_("IV"), x_("ISP"), x_("IB"), x_(""), x_("")};
static CHAR *bipolar_gds_layers[MAXLAYERS] = {x_("8"), x_("9"), x_("52"), x_("53"), x_("4"), x_("2"), x_("3"), x_("6"), x_("51"), x_("45"), x_("7"), x_("81"), x_("10"),
	x_("1"), x_("18"), x_("19")};
static INTBIG bipolar_layer_function[MAXLAYERS] = {LFMETAL1|LFTRANS1,
	LFMETAL2|LFTRANS4, LFIMPLANT|LFNTYPE, LFIMPLANT|LFPTYPE, LFPOLY1|LFTRANS2,
	LFIMPLANT|LFLIGHT, LFISOLATION|LFTRANS3, LFDIFF|LFHEAVY, LFIMPLANT|LFNTYPE,
	LFGUARD|LFTRANS5, LFCONTACT1, LFCONTACT2, LFOVERGLASS, LFDIFF,
	LFMETAL1|LFPSEUDO|LFTRANS1, LFMETAL2|LFPSEUDO|LFTRANS4};
static CHAR *bipolar_layer_letters[MAXLAYERS] = {x_("1"), x_("2"), x_("n"), x_("p"), x_("P"), x_("f"), x_("i"), x_("s"),
	x_("N"), x_("x"), x_("c"), x_("v"), x_("o"), x_("b"), x_("A"), x_("B")};

static TECH_COLORMAP bipolar_colmap[32] =
{
	{200,200,200}, /*  0:       +               +         +      +                   */
	{255,  0,  0}, /*  1: Metal1+               +         +      +                   */
	{ 50, 50,200}, /*  2:       +Poly_Definition+         +      +                   */
	{161, 23,117}, /*  3: Metal1+Poly_Definition+         +      +                   */
	{115,255, 82}, /*  4:       +               +Isolation+      +                   */
	{255, 89, 24}, /*  5: Metal1+               +Isolation+      +                   */
	{ 83,153,141}, /*  6:       +Poly_Definition+Isolation+      +                   */
	{134, 33,140}, /*  7: Metal1+Poly_Definition+Isolation+      +                   */
	{ 96,213,255}, /*  8:       +               +         +Metal2+                   */
	{189, 74,247}, /*  9: Metal1+               +         +Metal2+                   */
	{100,142,195}, /* 10:       +Poly_Definition+         +Metal2+                   */
	{255, 21,236}, /* 11: Metal1+Poly_Definition+         +Metal2+                   */
	{ 80,210,  0}, /* 12:       +               +Isolation+Metal2+                   */
	{255,175, 76}, /* 13: Metal1+               +Isolation+Metal2+                   */
	{ 69,149,177}, /* 14:       +Poly_Definition+Isolation+Metal2+                   */
	{208,148,208}, /* 15: Metal1+Poly_Definition+Isolation+Metal2+                   */
	{205,205,205}, /* 16:       +               +         +      +Silicide_Exclusion */
	{205,  0,  0}, /* 17: Metal1+               +         +      +Silicide_Exclusion */
	{  0,  0,150}, /* 18:       +Poly_Definition+         +      +Silicide_Exclusion */
	{111,  0, 67}, /* 19: Metal1+Poly_Definition+         +      +Silicide_Exclusion */
	{ 65,205, 32}, /* 20:       +               +Isolation+      +Silicide_Exclusion */
	{205, 39,  0}, /* 21: Metal1+               +Isolation+      +Silicide_Exclusion */
	{ 33,103, 91}, /* 22:       +Poly_Definition+Isolation+      +Silicide_Exclusion */
	{ 84,  0, 90}, /* 23: Metal1+Poly_Definition+Isolation+      +Silicide_Exclusion */
	{ 46,163,205}, /* 24:       +               +         +Metal2+Silicide_Exclusion */
	{139, 24,197}, /* 25: Metal1+               +         +Metal2+Silicide_Exclusion */
	{ 50, 92,145}, /* 26:       +Poly_Definition+         +Metal2+Silicide_Exclusion */
	{255, 21,236}, /* 27: Metal1+Poly_Definition+         +Metal2+Silicide_Exclusion */
	{ 30,160,  0}, /* 28:       +               +Isolation+Metal2+Silicide_Exclusion */
	{205,125, 26}, /* 29: Metal1+               +Isolation+Metal2+Silicide_Exclusion */
	{ 19, 99,127}, /* 30:       +Poly_Definition+Isolation+Metal2+Silicide_Exclusion */
	{158, 98,158}, /* 31: Metal1+Poly_Definition+Isolation+Metal2+Silicide_Exclusion */
};

/******************** DESIGN RULES ********************/

static INTBIG bipolar_unconnectedtable[] = {
/*            M  M  N  P  P  F  I  S  N  S  C  V  S  B  P  P   */
/*               0        D  I     I  I  E        P     M  M   */
/*                                                         0   */
/*                                                             */
/*                                                             */
/*                                                             */
/* M      */ K2,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* M0     */    K2,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* N      */       XX,K0,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* P      */          XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PD     */             K2,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* FI     */                K2,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* I      */                   XX,XX,XX,XX,H0,XX,XX,XX,XX,XX,
/* SI     */                      XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* NI     */                         XX,XX,XX,XX,XX,XX,XX,XX,
/* SE     */                            XX,XX,XX,XX,XX,XX,XX,
/* C      */                               K2,K1,XX,XX,XX,XX,
/* V      */                                  K2,XX,XX,XX,XX,
/* SP     */                                     XX,XX,XX,XX,
/* B      */                                        XX,XX,XX,
/* PM     */                                           XX,XX,
/* PM0    */                                              XX
};

/******************** ARCS ********************/

#define ARCPROTOCOUNT 4
#define AMETAL_1      0		/* Metal_1 */
#define AMETAL_2      1		/* Metal_2 */
#define ANPPOLY       2		/* NPPoly */
#define APPPOLY       3		/* PPPoly */

static TECH_ARCLAY bipolar_al_0[] = {{LM,0,FILLED}};
static TECH_ARCS bipolar_a_0 = {
	x_("Metal_1"), K3, AMETAL_1, NOARCPROTO,
	1, bipolar_al_0,
	(APMETAL1<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};

static TECH_ARCLAY bipolar_al_1[] = {{LM0,0,FILLED}};
static TECH_ARCS bipolar_a_1 = {
	x_("Metal_2"), K4, AMETAL_2, NOARCPROTO,
	1, bipolar_al_1,
	(APMETAL2<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};

static TECH_ARCLAY bipolar_al_2[] = {{LPD,K2,FILLED}, {LN,0,FILLED}};
static TECH_ARCS bipolar_a_2 = {
	x_("NPPoly"), K4, ANPPOLY, NOARCPROTO,
	2, bipolar_al_2,
	(APPOLY1<<AFUNCTIONSH)|WANTFIXANG|WANTNOEXTEND|(90<<AANGLEINCSH)};

static TECH_ARCLAY bipolar_al_3[] = {{LPD,K2,FILLED}, {LP,0,FILLED}};
static TECH_ARCS bipolar_a_3 = {
	x_("PPPoly"), K4, APPPOLY, NOARCPROTO,
	2, bipolar_al_3,
	(APPOLY2<<AFUNCTIONSH)|WANTFIXANG|WANTNOEXTEND|(90<<AANGLEINCSH)};

TECH_ARCS *bipolar_arcprotos[ARCPROTOCOUNT+1] = {
	&bipolar_a_0, &bipolar_a_1, &bipolar_a_2, &bipolar_a_3, ((TECH_ARCS *)-1)};

static INTBIG bipolar_arc_widoff[ARCPROTOCOUNT] = {0, 0, K2, K2};

/******************** PORT CONNECTIONS ********************/

static INTBIG bipolar_pc_1[] = {-1, ALLGEN, -1};
static INTBIG bipolar_pc_2[] = {-1, AMETAL_1, ANPPOLY, ALLGEN, -1};
static INTBIG bipolar_pc_3[] = {-1, AMETAL_1, APPPOLY, ALLGEN, -1};
static INTBIG bipolar_pc_4[] = {-1, AMETAL_1, AMETAL_2, ALLGEN, -1};
static INTBIG bipolar_pc_5[] = {-1, APPPOLY, ALLGEN, -1};
static INTBIG bipolar_pc_6[] = {-1, ANPPOLY, ALLGEN, -1};
static INTBIG bipolar_pc_7[] = {-1, AMETAL_2, ALLGEN, -1};
static INTBIG bipolar_pc_8[] = {-1, AMETAL_1, ALLGEN, -1};

/******************** RECTANGLE DESCRIPTIONS ********************/

static INTBIG bipolar_box1[8] = {CENTER, BOTEDGE, RIGHTEDGE, TOPEDGE};
static INTBIG bipolar_box2[8] = {CENTER, BOTIN1, RIGHTIN1, TOPIN1};
static INTBIG bipolar_box3[8] = {LEFTEDGE, BOTEDGE, CENTER, TOPEDGE};
static INTBIG bipolar_box4[8] = {LEFTIN1, BOTIN1, CENTER, TOPIN1};
static INTBIG bipolar_box5[8] = {LEFTIN9, BOTEDGE, RIGHTIN7, TOPEDGE};
static INTBIG bipolar_box6[8] = {RIGHTIN7, BOTEDGE, RIGHTEDGE, TOPEDGE};
static INTBIG bipolar_box7[8] = {RIGHTIN7, BOTIN3H, RIGHTIN3H, TOPIN3H};
static INTBIG bipolar_box8[8] = {RIGHTIN7, BOTIN2, RIGHTIN2, TOPIN2};
static INTBIG bipolar_box9[8] = {LEFTIN9, BOTIN2, RIGHTIN7, TOPIN2};
static INTBIG bipolar_box10[8] = {RIGHTIN6, BOTIN3, RIGHTIN3, TOPIN3};
static INTBIG bipolar_box11[8] = {LEFTIN9, BOTIN3H, RIGHTIN7, TOPIN3H};
static INTBIG bipolar_box12[8] = {LEFTEDGE, BOTEDGE, LEFTIN9, TOPEDGE};
static INTBIG bipolar_box13[8] = {LEFTIN10, BOTIN3, RIGHTIN8, TOPIN3};
static INTBIG bipolar_box14[8] = {LEFTIN2, BOTIN2, LEFTIN9, TOPIN2};
static INTBIG bipolar_box15[8] = {LEFTIN3H, BOTIN3H, LEFTIN7H, TOPIN3H};
static INTBIG bipolar_box16[8] = {LEFTIN2H, BOTIN2H, LEFTIN8H, TOPIN2H};
static INTBIG bipolar_box17[8] = {LEFTIN3, BOTIN3, LEFTIN8, TOPIN3};
static INTBIG bipolar_box18[8] = {LEFTEDGE, BOTIN1H, RIGHTEDGE, CENTER};
static INTBIG bipolar_box19[8] = {LEFTEDGE, CENTER, RIGHTEDGE, TOPIN1H};
static INTBIG bipolar_box20[8] = {LEFTEDGE, BOTEDGE, RIGHTEDGE, BOTIN2};
static INTBIG bipolar_box21[8] = {LEFTEDGE, TOPIN2, RIGHTEDGE, TOPEDGE};
static INTBIG bipolar_box22[8] = {LEFTEDGE, BOTIN2, RIGHTEDGE, CENTER};
static INTBIG bipolar_box23[8] = {LEFTEDGE, BOTEDGE, RIGHTEDGE, CENTER};
static INTBIG bipolar_box24[8] = {LEFTIN1, BOTIN1, RIGHTIN1, CENTER};
static INTBIG bipolar_box25[8] = {LEFTEDGE, CENTER, RIGHTEDGE, TOPEDGE};
static INTBIG bipolar_box26[8] = {LEFTEDGE, CENTER, RIGHTEDGE, TOPIN2};
static INTBIG bipolar_box27[8] = {LEFTIN1, CENTER, RIGHTIN1, TOPIN1};
static INTBIG bipolar_box28[8] = {LEFTIN1, BOTIN1, LEFTIN3, BOTIN3};
static INTBIG bipolar_box29[8] = {LEFTIN0H, BOTIN0H, RIGHTIN0H, TOPIN0H};
static INTBIG bipolar_box30[8] = {LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1};
static INTBIG bipolar_box31[8] = {LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE};

/******************** NODES ********************/

#define NODEPROTOCOUNT 25
#define NMP            1		/* Metal1_Pin */
#define NMP0           2		/* Metal2_Pin */
#define NNP            3		/* NPPoly_pin */
#define NPP            4		/* PPPoly_pin */
#define NV             5		/* Via */
#define NMPC           6		/* M1_PP_Contact */
#define NMNC           7		/* M1_NP_Contact */
#define NN             8		/* NPResistor */
#define NN0            9		/* NMResistor */
#define NN1            10		/* npn111 */
#define NP             11		/* PNJunction */
#define NMN            12		/* Metal1_Node */
#define NMN0           13		/* Metal2_Node */
#define NNN            14		/* NPImplant_Node */
#define NPN            15		/* PPImplant_Node */
#define NPDN           16		/* Poly_Def_Node */
#define NFIN           17		/* Field_Implant_Node */
#define NIIN           18		/* Isolation_Implant_Node */
#define NSIN           19		/* Sink_Implant_Node */
#define NNIN           20		/* N_Implant_Node */
#define NSEN           21		/* Silicode_Exclusion_Node */
#define NCN            22		/* Contact_Node */
#define NVN            23		/* Via_Node */
#define NSPN           24		/* Scratch_Protection_Node */
#define NBN            25		/* Buried_Node */

/* Metal1_Pin */
static TECH_PORTS bipolar_mp_p[] = {
	{bipolar_pc_8, x_("metal1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON bipolar_mp_l[] = {
	{LPM, 0, 4, CROSSED, BOX, bipolar_box31}};
static TECH_NODES bipolar_mp = {
	x_("Metal1_Pin"), NMP, NONODEPROTO,
	K3, K3,
	1, bipolar_mp_p,
	1, bipolar_mp_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* Metal2_Pin */
static TECH_PORTS bipolar_mp0_p[] = {
	{bipolar_pc_7, x_("metal2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON bipolar_mp0_l[] = {
	{LPM0, 0, 4, CROSSED, BOX, bipolar_box31}};
static TECH_NODES bipolar_mp0 = {
	x_("Metal2_Pin"), NMP0, NONODEPROTO,
	K4, K4,
	1, bipolar_mp0_p,
	1, bipolar_mp0_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* NPPoly_pin */
static TECH_PORTS bipolar_np_p[] = {
	{bipolar_pc_6, x_("p"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN2, BOTIN2, RIGHTIN2, TOPIN2}};
static TECH_POLYGON bipolar_np_l[] = {
	{LPD, 0, 4, FILLEDRECT, BOX, bipolar_box30},
	{LN, 0, 4, FILLEDRECT, BOX, bipolar_box31}};
static TECH_NODES bipolar_np = {
	x_("NPPoly_pin"), NNP, NONODEPROTO,
	K4, K4,
	1, bipolar_np_p,
	2, bipolar_np_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* PPPoly_pin */
static TECH_PORTS bipolar_pp_p[] = {
	{bipolar_pc_5, x_("p"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN2, BOTIN2, RIGHTIN2, TOPIN2}};
static TECH_POLYGON bipolar_pp_l[] = {
	{LPD, 0, 4, FILLEDRECT, BOX, bipolar_box30},
	{LP, 0, 4, FILLEDRECT, BOX, bipolar_box31}};
static TECH_NODES bipolar_pp = {
	x_("PPPoly_pin"), NPP, NONODEPROTO,
	K4, K4,
	1, bipolar_pp_p,
	2, bipolar_pp_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* Via */
static TECH_PORTS bipolar_v_p[] = {
	{bipolar_pc_4, x_("via"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN2, BOTIN2, RIGHTIN2, TOPIN2}};
static TECH_POLYGON bipolar_v_l[] = {
	{LM, 0, 4, FILLEDRECT, BOX, bipolar_box29},
	{LM0, 0, 4, FILLEDRECT, BOX, bipolar_box31},
	{LV, 0, 4, FILLEDRECT, BOX, bipolar_box28}};
static TECH_NODES bipolar_v = {
	x_("Via"), NV, NONODEPROTO,
	K4, K4,
	1, bipolar_v_p,
	3, bipolar_v_l,
	(NPCONNECT<<NFUNCTIONSH),
	MULTICUT,K2,K2,K1,K2,0,0,0,0};

/* M1_PP_Contact */
static TECH_PORTS bipolar_mpc_p[] = {
	{bipolar_pc_3, x_("m"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN2H, BOTIN2H, RIGHTIN2H, TOPIN2H}};
static TECH_POLYGON bipolar_mpc_l[] = {
	{LM, 0, 4, FILLEDRECT, BOX, bipolar_box30},
	{LPD, 0, 4, FILLEDRECT, BOX, bipolar_box30},
	{LP, 0, 4, FILLEDRECT, BOX, bipolar_box31},
	{LC, 0, 4, FILLEDRECT, BOX, bipolar_box28}};
static TECH_NODES bipolar_mpc = {
	x_("M1_PP_Contact"), NMPC, NONODEPROTO,
	K6, K6,
	1, bipolar_mpc_p,
	4, bipolar_mpc_l,
	(NPCONNECT<<NFUNCTIONSH),
	MULTICUT,K2,K2,K0,K2,0,0,0,0};

/* M1_NP_Contact */
static TECH_PORTS bipolar_mnc_p[] = {
	{bipolar_pc_2, x_("m"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN2H, BOTIN2H, RIGHTIN2H, TOPIN2H}};
static TECH_POLYGON bipolar_mnc_l[] = {
	{LPD, 0, 4, FILLEDRECT, BOX, bipolar_box30},
	{LM, 0, 4, FILLEDRECT, BOX, bipolar_box30},
	{LN, 0, 4, FILLEDRECT, BOX, bipolar_box31},
	{LC, 0, 4, FILLEDRECT, BOX, bipolar_box28}};
static TECH_NODES bipolar_mnc = {
	x_("M1_NP_Contact"), NMNC, NONODEPROTO,
	K6, K6,
	1, bipolar_mnc_p,
	4, bipolar_mnc_l,
	(NPCONNECT<<NFUNCTIONSH),
	MULTICUT,K2,K2,K0,K2,0,0,0,0};

/* NPResistor */
static TECH_PORTS bipolar_n_p[] = {
	{bipolar_pc_6, x_("p1"), NOPORTPROTO, (0<<PORTARANGESH)|(90<<PORTANGLESH),
		LEFTIN2, TOPIN1, RIGHTIN2, TOPIN1},
	{bipolar_pc_6, x_("p2"), NOPORTPROTO, (0<<PORTARANGESH)|(270<<PORTANGLESH)|(1<<PORTNETSH),
		LEFTIN2, BOTIN1, RIGHTIN2, BOTIN1}};
static TECH_POLYGON bipolar_n_l[] = {
	{LPD, 0, 4, FILLEDRECT, BOX, bipolar_box27},
	{LSE, -1, 4, FILLEDRECT, BOX, bipolar_box26},
	{LPD, 1, 4, FILLEDRECT, BOX, bipolar_box24},
	{LSE, -1, 4, FILLEDRECT, BOX, bipolar_box22},
	{LN, 0, 4, FILLEDRECT, BOX, bipolar_box25},
	{LN, 1, 4, FILLEDRECT, BOX, bipolar_box23}};
static TECH_NODES bipolar_n = {
	x_("NPResistor"), NN, NONODEPROTO,
	K5, K7,
	2, bipolar_n_p,
	6, bipolar_n_l,
	(NPRESIST<<NFUNCTIONSH),
	0,0,0,0,0,0,0,0,0};

/* NMResistor */
static TECH_PORTS bipolar_n0_p[] = {
	{bipolar_pc_6, x_("p1"), NOPORTPROTO, (0<<PORTARANGESH)|(90<<PORTANGLESH),
		LEFTIN2, TOPIN1, RIGHTIN2, TOPIN1},
	{bipolar_pc_6, x_("p2"), NOPORTPROTO, (0<<PORTARANGESH)|(270<<PORTANGLESH)|(1<<PORTNETSH),
		LEFTIN2, BOTIN1, RIGHTIN2, BOTIN1}};
static TECH_POLYGON bipolar_n0_l[] = {
	{LPD, 0, 4, FILLEDRECT, BOX, bipolar_box27},
	{LPD, 1, 4, FILLEDRECT, BOX, bipolar_box24},
	{LSE, -1, 4, FILLEDRECT, BOX, bipolar_box26},
	{LSE, -1, 4, FILLEDRECT, BOX, bipolar_box22},
	{LN, 0, 4, FILLEDRECT, BOX, bipolar_box21},
	{LN, 1, 4, FILLEDRECT, BOX, bipolar_box20},
	{LNI, -1, 4, FILLEDRECT, BOX, bipolar_box19},
	{LNI, -1, 4, FILLEDRECT, BOX, bipolar_box18}};
static TECH_NODES bipolar_n0 = {
	x_("NMResistor"), NN0, NONODEPROTO,
	K5, K7,
	2, bipolar_n0_p,
	8, bipolar_n0_l,
	(NPUNKNOWN<<NFUNCTIONSH),
	0,0,0,0,0,0,0,0,0};

/* npn111 */
static TECH_PORTS bipolar_n1_p[] = {
	{bipolar_pc_6, x_("c"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN4, BOTIN4, LEFTIN7, TOPIN4},
	{bipolar_pc_6, x_("e"), NOPORTPROTO, (180<<PORTARANGESH)|(1<<PORTNETSH),
		LEFTIN11, BOTIN4, RIGHTIN9, TOPIN4},
	{bipolar_pc_5, x_("b"), NOPORTPROTO, (180<<PORTARANGESH)|(2<<PORTNETSH),
		RIGHTIN5, BOTIN4, RIGHTIN4, TOPIN4}};
static TECH_POLYGON bipolar_n1_l[] = {
	{LPD, 0, 4, FILLEDRECT, BOX, bipolar_box17},
	{LI, 0, 4, FILLEDRECT, BOX, bipolar_box15},
	{LPD, 1, 4, FILLEDRECT, BOX, bipolar_box13},
	{LI, 1, 4, FILLEDRECT, BOX, bipolar_box11},
	{LPD, 2, 4, FILLEDRECT, BOX, bipolar_box10},
	{LI, 2, 4, FILLEDRECT, BOX, bipolar_box7},
	{LSI, 0, 4, FILLEDRECT, BOX, bipolar_box16},
	{LN, 0, 4, FILLEDRECT, BOX, bipolar_box14},
	{LFI, 0, 4, FILLEDRECT, BOX, bipolar_box12},
	{LN, 1, 4, FILLEDRECT, BOX, bipolar_box9},
	{LP, 2, 4, FILLEDRECT, BOX, bipolar_box8},
	{LFI, 2, 4, FILLEDRECT, BOX, bipolar_box6},
	{LFI, 1, 4, FILLEDRECT, BOX, bipolar_box5}};
static TECH_NODES bipolar_n1 = {
	x_("npn111"), NN1, NONODEPROTO,
	K20, K11,
	3, bipolar_n1_p,
	13, bipolar_n1_l,
	(NPTRANPN<<NFUNCTIONSH),
	0,0,0,0,0,0,0,0,0};

/* PNJunction */
static TECH_PORTS bipolar_p_p[] = {
	{bipolar_pc_5, x_("p"), NOPORTPROTO, (0<<PORTARANGESH),
		RIGHTIN1, BOTIN2, RIGHTIN1, TOPIN2},
	{bipolar_pc_6, x_("n"), NOPORTPROTO, (0<<PORTARANGESH)|(180<<PORTANGLESH),
		LEFTIN1, BOTIN2, LEFTIN1, TOPIN2}};
static TECH_POLYGON bipolar_p_l[] = {
	{LPD, 1, 4, FILLEDRECT, BOX, bipolar_box4},
	{LPD, 0, 4, FILLEDRECT, BOX, bipolar_box2},
	{LN, 1, 4, FILLEDRECT, BOX, bipolar_box3},
	{LP, 0, 4, FILLEDRECT, BOX, bipolar_box1}};
static TECH_NODES bipolar_p = {
	x_("PNJunction"), NP, NONODEPROTO,
	K4, K4,
	2, bipolar_p_p,
	4, bipolar_p_l,
	(NPCONNECT<<NFUNCTIONSH),
	0,0,0,0,0,0,0,0,0};

/* Metal1_Node */
static TECH_PORTS bipolar_mn_p[] = {
	{bipolar_pc_8, x_("metal1"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON bipolar_mn_l[] = {
	{LM, 0, 4, FILLEDRECT, BOX, bipolar_box31}};
static TECH_NODES bipolar_mn = {
	x_("Metal1_Node"), NMN, NONODEPROTO,
	K2, K2,
	1, bipolar_mn_p,
	1, bipolar_mn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Metal2_Node */
static TECH_PORTS bipolar_mn0_p[] = {
	{bipolar_pc_7, x_("metal2"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON bipolar_mn0_l[] = {
	{LM0, 0, 4, FILLEDRECT, BOX, bipolar_box31}};
static TECH_NODES bipolar_mn0 = {
	x_("Metal2_Node"), NMN0, NONODEPROTO,
	K2, K2,
	1, bipolar_mn0_p,
	1, bipolar_mn0_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* NPImplant_Node */
static TECH_PORTS bipolar_nn_p[] = {
	{bipolar_pc_1, x_("N+implant"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON bipolar_nn_l[] = {
	{LN, 0, 4, FILLEDRECT, BOX, bipolar_box31}};
static TECH_NODES bipolar_nn = {
	x_("NPImplant_Node"), NNN, NONODEPROTO,
	K2, K2,
	1, bipolar_nn_p,
	1, bipolar_nn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* PPImplant_Node */
static TECH_PORTS bipolar_pn_p[] = {
	{bipolar_pc_1, x_("P+implant"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON bipolar_pn_l[] = {
	{LP, 0, 4, FILLEDRECT, BOX, bipolar_box31}};
static TECH_NODES bipolar_pn = {
	x_("PPImplant_Node"), NPN, NONODEPROTO,
	K2, K2,
	1, bipolar_pn_p,
	1, bipolar_pn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Poly_Def_Node */
static TECH_PORTS bipolar_pdn_p[] = {
	{bipolar_pc_1, x_("poly-def"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON bipolar_pdn_l[] = {
	{LPD, 0, 4, FILLEDRECT, BOX, bipolar_box31}};
static TECH_NODES bipolar_pdn = {
	x_("Poly_Def_Node"), NPDN, NONODEPROTO,
	K2, K2,
	1, bipolar_pdn_p,
	1, bipolar_pdn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Field_Implant_Node */
static TECH_PORTS bipolar_fin_p[] = {
	{bipolar_pc_1, x_("field"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON bipolar_fin_l[] = {
	{LFI, 0, 4, CLOSEDRECT, BOX, bipolar_box31}};
static TECH_NODES bipolar_fin = {
	x_("Field_Implant_Node"), NFIN, NONODEPROTO,
	K4, K4,
	1, bipolar_fin_p,
	1, bipolar_fin_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Isolation_Implant_Node */
static TECH_PORTS bipolar_iin_p[] = {
	{bipolar_pc_1, x_("isolation"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON bipolar_iin_l[] = {
	{LI, 0, 4, CLOSEDRECT, BOX, bipolar_box31}};
static TECH_NODES bipolar_iin = {
	x_("Isolation_Implant_Node"), NIIN, NONODEPROTO,
	K4, K4,
	1, bipolar_iin_p,
	1, bipolar_iin_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Sink_Implant_Node */
static TECH_PORTS bipolar_sin_p[] = {
	{bipolar_pc_1, x_("sink-implant"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON bipolar_sin_l[] = {
	{LSI, 0, 4, CLOSEDRECT, BOX, bipolar_box31}};
static TECH_NODES bipolar_sin = {
	x_("Sink_Implant_Node"), NSIN, NONODEPROTO,
	K4, K4,
	1, bipolar_sin_p,
	1, bipolar_sin_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* N_Implant_Node */
static TECH_PORTS bipolar_nin_p[] = {
	{bipolar_pc_1, x_("N-implant"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON bipolar_nin_l[] = {
	{LNI, 0, 4, CLOSEDRECT, BOX, bipolar_box31}};
static TECH_NODES bipolar_nin = {
	x_("N_Implant_Node"), NNIN, NONODEPROTO,
	K4, K4,
	1, bipolar_nin_p,
	1, bipolar_nin_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Silicode_Exclusion_Node */
static TECH_PORTS bipolar_sen_p[] = {
	{bipolar_pc_1, x_("silicide-exclusion"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON bipolar_sen_l[] = {
	{LSE, 0, 4, CLOSEDRECT, BOX, bipolar_box31}};
static TECH_NODES bipolar_sen = {
	x_("Silicode_Exclusion_Node"), NSEN, NONODEPROTO,
	K4, K4,
	1, bipolar_sen_p,
	1, bipolar_sen_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Contact_Node */
static TECH_PORTS bipolar_cn_p[] = {
	{bipolar_pc_1, x_("contact"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON bipolar_cn_l[] = {
	{LC, 0, 4, CROSSED, BOX, bipolar_box31}};
static TECH_NODES bipolar_cn = {
	x_("Contact_Node"), NCN, NONODEPROTO,
	K4, K4,
	1, bipolar_cn_p,
	1, bipolar_cn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Via_Node */
static TECH_PORTS bipolar_vn_p[] = {
	{bipolar_pc_1, x_("via"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON bipolar_vn_l[] = {
	{LV, 0, 4, FILLEDRECT, BOX, bipolar_box31}};
static TECH_NODES bipolar_vn = {
	x_("Via_Node"), NVN, NONODEPROTO,
	K4, K4,
	1, bipolar_vn_p,
	1, bipolar_vn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Scratch_Protection_Node */
static TECH_PORTS bipolar_spn_p[] = {
	{bipolar_pc_1, x_("scratch-protection"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON bipolar_spn_l[] = {
	{LSP, 0, 4, FILLEDRECT, BOX, bipolar_box31}};
static TECH_NODES bipolar_spn = {
	x_("Scratch_Protection_Node"), NSPN, NONODEPROTO,
	K4, K4,
	1, bipolar_spn_p,
	1, bipolar_spn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Buried_Node */
static TECH_PORTS bipolar_bn_p[] = {
	{bipolar_pc_1, x_("buried"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON bipolar_bn_l[] = {
	{LB, 0, 4, CLOSEDRECT, BOX, bipolar_box31}};
static TECH_NODES bipolar_bn = {
	x_("Buried_Node"), NBN, NONODEPROTO,
	K4, K4,
	1, bipolar_bn_p,
	1, bipolar_bn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

TECH_NODES *bipolar_nodeprotos[NODEPROTOCOUNT+1] = {
	&bipolar_mp, &bipolar_mp0, &bipolar_np, &bipolar_pp, &bipolar_v,
	&bipolar_mpc, &bipolar_mnc, &bipolar_n, &bipolar_n0, &bipolar_n1, &bipolar_p,
	&bipolar_mn, &bipolar_mn0, &bipolar_nn, &bipolar_pn, &bipolar_pdn,
	&bipolar_fin, &bipolar_iin, &bipolar_sin, &bipolar_nin, &bipolar_sen,
	&bipolar_cn, &bipolar_vn, &bipolar_spn, &bipolar_bn, ((TECH_NODES *)-1)};

static INTBIG bipolar_node_widoff[NODEPROTOCOUNT*4] = {
	0,0,0,0, 0,0,0,0, K1,K1,K1,K1, K1,K1,K1,K1, 0,0,0,0, K2,K2,K2,K2, K2,K2,K2,K2, K1,K1,K2,K2,
	K1,K1,K2,K2, K10,K8,H3,H3, K1,K1,K1,K1, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0};

/******************** VARIABLE AGGREGATION ********************/

TECH_VARIABLES bipolar_variables[] =
{
	{x_("TECH_layer_names"), (CHAR *)bipolar_layer_names, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_layer_function"), (CHAR *)bipolar_layer_function, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_node_width_offset"), (CHAR *)bipolar_node_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|((NODEPROTOCOUNT*4)<<VLENGTHSH)},
	{x_("TECH_arc_width_offset"), (CHAR *)bipolar_arc_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|(ARCPROTOCOUNT<<VLENGTHSH)},
	{x_("USER_layer_letters"), (CHAR *)bipolar_layer_letters, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("USER_color_map"), (CHAR *)bipolar_colmap, 0.0,
		VCHAR|VDONTSAVE|VISARRAY|((sizeof bipolar_colmap)<<VLENGTHSH)},
	{x_("IO_cif_layer_names"), (CHAR *)bipolar_cif_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("IO_gds_layer_numbers"), (CHAR *)bipolar_gds_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("DRC_min_unconnected_distances"), (CHAR *)bipolar_unconnectedtable, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|
		   (((sizeof bipolar_unconnectedtable)/SIZEOFINTBIG)<<VLENGTHSH)},
	{NULL, NULL, 0.0, 0}
};

/******************** ROUTINES ********************/

BOOLEAN bipolar_initprocess(TECHNOLOGY *tech, INTBIG pass)
{
	Q_UNUSED( tech );
	Q_UNUSED( pass );
	return(FALSE);
}
