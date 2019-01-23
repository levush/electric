/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: teccmosdodn.c
 * dodcmosn technology description
 * Generated automatically from a library
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
#include "teccmosdodn.h"
#include "efunction.h"

static void dodcmosn_intshapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl);

/******************** LAYERS ********************/

#define MAXLAYERS 22
#define LM        0			/* Metal_1 */
#define LM0       1			/* Metal_2 */
#define LV        2			/* Via */
#define LP        3			/* Passivation */
#define LT        4			/* Transistor */
#define LPM       5			/* Pseudo_Metal_1 */
#define LPM0      6			/* Pseudo_Metal_2 */
#define LPSA      7			/* Pseudo_S_Active */
#define LPF       8			/* Pad_Frame */
#define LP0       9			/* Poly1 */
#define LPI       10		/* P+Implant */
#define LNI       11		/* N+Implant */
#define LP1       12		/* PWell */
#define LN        13		/* NWell */
#define LC        14		/* Contact */
#define LPP       15		/* Pseudo_Poly1 */
#define LPA       16		/* Pseudo_Active */
#define LPPI      17		/* Pseudo_P+Implant */
#define LPNI      18		/* Pseudo_N+Implant */
#define LPP0      19		/* Pseudo_PWell */
#define LPN       20		/* Pseudo_NWell */
#define LA        21		/* Active */

static GRAPHICS dodcmosn_M_lay = {LAYERT1, COLORT1, SOLIDC, PATTERNED,
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
static GRAPHICS dodcmosn_M0_lay = {LAYERT4, COLORT4, SOLIDC, PATTERNED,
	{0x1010, /*    X       X     */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0404,  /*      X       X   */
	0x0808,  /*     X       X    */
	0x1010,  /*    X       X     */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0404,  /*      X       X   */
	0x0808}, /*     X       X    */
	NOVARIABLE, 0};
static GRAPHICS dodcmosn_V_lay = {LAYERO, WHITE, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS dodcmosn_P_lay = {LAYERO, DGRAY, PATTERNED, PATTERNED,
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
static GRAPHICS dodcmosn_T_lay = {LAYERO, ALLOFF, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS dodcmosn_PM_lay = {LAYERT1, COLORT1, SOLIDC, PATTERNED,
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
static GRAPHICS dodcmosn_PM0_lay = {LAYERT4, COLORT4, SOLIDC, PATTERNED,
	{0x1010, /*    X       X     */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0404,  /*      X       X   */
	0x0808,  /*     X       X    */
	0x1010,  /*    X       X     */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0404,  /*      X       X   */
	0x0808}, /*     X       X    */
	NOVARIABLE, 0};
static GRAPHICS dodcmosn_PSA_lay = {LAYERT3, COLORT3, SOLIDC, PATTERNED,
	{0x0000, /*                  */
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
static GRAPHICS dodcmosn_PF_lay = {LAYERO, RED, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS dodcmosn_P0_lay = {LAYERT2, COLORT2, SOLIDC, PATTERNED,
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
static GRAPHICS dodcmosn_PI_lay = {LAYERO, YELLOW, PATTERNED, PATTERNED,
	{0x1010, /*    X       X     */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0404,  /*      X       X   */
	0x0808,  /*     X       X    */
	0x1010,  /*    X       X     */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0404,  /*      X       X   */
	0x0808}, /*     X       X    */
	NOVARIABLE, 0};
static GRAPHICS dodcmosn_NI_lay = {LAYERO, PURPLE, PATTERNED, PATTERNED,
	{0x1010, /*    X       X     */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0404,  /*      X       X   */
	0x0808,  /*     X       X    */
	0x1010,  /*    X       X     */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0404,  /*      X       X   */
	0x0808}, /*     X       X    */
	NOVARIABLE, 0};
static GRAPHICS dodcmosn_P1_lay = {LAYERO, YELLOW, SOLIDC, PATTERNED,
	{0x1000, /*    X             */
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
static GRAPHICS dodcmosn_N_lay = {LAYERO, PURPLE, SOLIDC, PATTERNED,
	{0x1000, /*    X             */
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
static GRAPHICS dodcmosn_C_lay = {LAYERO, BLACK, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS dodcmosn_PP_lay = {LAYERT2, COLORT2, SOLIDC, PATTERNED,
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
static GRAPHICS dodcmosn_PA_lay = {LAYERT3, COLORT3, SOLIDC, PATTERNED,
	{0x0000, /*                  */
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
static GRAPHICS dodcmosn_PPI_lay = {LAYERO, YELLOW, PATTERNED, PATTERNED,
	{0x1010, /*    X       X     */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0404,  /*      X       X   */
	0x0808,  /*     X       X    */
	0x1010,  /*    X       X     */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0404,  /*      X       X   */
	0x0808}, /*     X       X    */
	NOVARIABLE, 0};
static GRAPHICS dodcmosn_PNI_lay = {LAYERO, YELLOW, PATTERNED, PATTERNED,
	{0x1010, /*    X       X     */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0404,  /*      X       X   */
	0x0808,  /*     X       X    */
	0x1010,  /*    X       X     */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0404,  /*      X       X   */
	0x0808}, /*     X       X    */
	NOVARIABLE, 0};
static GRAPHICS dodcmosn_PP0_lay = {LAYERO, YELLOW, SOLIDC, PATTERNED,
	{0x1000, /*    X             */
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
static GRAPHICS dodcmosn_PN_lay = {LAYERT5, COLORT5, SOLIDC, PATTERNED,
	{0x1000, /*    X             */
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
static GRAPHICS dodcmosn_A_lay = {LAYERT3, COLORT3, SOLIDC, PATTERNED,
	{0x9249, /* X  X  X  X  X  X */
	0x0000,  /*                  */
	0x4924,  /*  X  X  X  X  X   */
	0x0000,  /*                  */
	0x2492,  /*   X  X  X  X  X  */
	0x0000,  /*                  */
	0x9249,  /* X  X  X  X  X  X */
	0x0000,  /*                  */
	0x9249,  /* X  X  X  X  X  X */
	0x0000,  /*                  */
	0x4924,  /*  X  X  X  X  X   */
	0x0000,  /*                  */
	0x2492,  /*   X  X  X  X  X  */
	0x0000,  /*                  */
	0x9249,  /* X  X  X  X  X  X */
	0x0000}, /*                  */
	NOVARIABLE, 0};

GRAPHICS *dodcmosn_layers[MAXLAYERS+1] = {&dodcmosn_M_lay,
	&dodcmosn_M0_lay, &dodcmosn_V_lay, &dodcmosn_P_lay,
	&dodcmosn_T_lay, &dodcmosn_PM_lay, &dodcmosn_PM0_lay,
	&dodcmosn_PSA_lay, &dodcmosn_PF_lay, &dodcmosn_P0_lay,
	&dodcmosn_PI_lay, &dodcmosn_NI_lay, &dodcmosn_P1_lay,
	&dodcmosn_N_lay, &dodcmosn_C_lay, &dodcmosn_PP_lay,
	&dodcmosn_PA_lay, &dodcmosn_PPI_lay, &dodcmosn_PNI_lay,
	&dodcmosn_PP0_lay, &dodcmosn_PN_lay, &dodcmosn_A_lay, NOGRAPHICS};
static CHAR *dodcmosn_layer_names[MAXLAYERS] = {x_("Metal_1"), x_("Metal_2"), x_("Via"),
	x_("Passivation"), x_("Transistor"), x_("Pseudo_Metal_1"), x_("Pseudo_Metal_2"),
	x_("Pseudo_S_Active"), x_("Pad_Frame"), x_("Poly1"), x_("P+Implant"), x_("N+Implant"), x_("PWell"),
	x_("NWell"), x_("Contact"), x_("Pseudo_Poly1"), x_("Pseudo_Active"), x_("Pseudo_P+Implant"),
	x_("Pseudo_N+Implant"), x_("Pseudo_PWell"), x_("Pseudo_NWell"), x_("Active")};
static CHAR *dodcmosn_cif_layers[MAXLAYERS] = {x_("NMF"), x_("NMS"), x_("NVA"), x_("NOG"), x_(""), x_(""), x_(""), x_(""), x_("CX"), x_("NFP"), x_("NPI"), x_("NNI"), x_("NPW"), x_("NNW"), x_("NCC"), x_(""), x_(""), x_(""), x_(""), x_(""), x_(""), x_("NAA")};
static CHAR *dodcmosn_gds_layers[MAXLAYERS] = {x_("10"), x_("12"), x_("11"), x_("13"), x_("0"), x_("0"), x_("0"), x_("0"), x_("0"), x_("4"), x_("8"), x_("7"), x_("2"), x_("1"), x_("9"), x_("0"), x_("0"), x_("0"), x_("0"), x_("0"), x_("0"), x_("3")};
static INTBIG dodcmosn_layer_function[MAXLAYERS] = {LFMETAL1|LFTRANS1,
	LFMETAL2|LFTRANS4, LFCONTACT2, LFOVERGLASS, LFTRANSISTOR|LFPSEUDO,
	LFMETAL1|LFPSEUDO|LFTRANS1, LFMETAL2|LFPSEUDO|LFTRANS4,
	LFDIFF|LFPTYPE|LFPSEUDO|LFTRANS3, LFART, LFPOLY1|LFTRANS2, LFIMPLANT|LFPTYPE,
	LFIMPLANT|LFNTYPE, LFWELL|LFPTYPE, LFWELL|LFNTYPE, LFCONTACT1,
	LFPOLY1|LFPSEUDO|LFTRANS2, LFDIFF|LFNTYPE|LFPSEUDO|LFTRANS3,
	LFIMPLANT|LFPTYPE|LFPSEUDO, LFIMPLANT|LFNTYPE|LFPSEUDO,
	LFWELL|LFPTYPE|LFPSEUDO, LFWELL|LFNTYPE|LFPSEUDO|LFTRANS5, LFDIFF|LFTRANS3};
static CHAR *dodcmosn_layer_letters[MAXLAYERS] = {x_("m"), x_("h"), x_("v"), x_("o"), x_("t"), x_("M"), x_("H"), x_("S"), x_("b"), x_("p"), x_("e"), x_("f"), x_("w"), x_("n"), x_("c"), x_("P"), x_("D"), x_("E"), x_("F"), x_("W"), x_("N"), x_("x")};

static TECH_COLORMAP dodcmosn_colmap[32] =
{
	{200,200,200}, /*  0:        +     +               +       +             */
	{ 96,209,255}, /*  1: Metal_1+     +               +       +             */
	{255,155,192}, /*  2:        +Poly1+               +       +             */
	{ 96,127,192}, /*  3: Metal_1+Poly1+               +       +             */
	{107,226, 96}, /*  4:        +     +Pseudo_S_Active+       +             */
	{ 40,186, 96}, /*  5: Metal_1+     +Pseudo_S_Active+       +             */
	{107,137, 72}, /*  6:        +Poly1+Pseudo_S_Active+       +             */
	{ 40,113, 72}, /*  7: Metal_1+Poly1+Pseudo_S_Active+       +             */
	{224, 95,255}, /*  8:        +     +               +Metal_2+             */
	{ 85, 78,255}, /*  9: Metal_1+     +               +Metal_2+             */
	{224, 57,192}, /* 10:        +Poly1+               +Metal_2+             */
	{ 85, 47,192}, /* 11: Metal_1+Poly1+               +Metal_2+             */
	{ 94, 84, 96}, /* 12:        +     +Pseudo_S_Active+Metal_2+             */
	{ 36, 69, 96}, /* 13: Metal_1+     +Pseudo_S_Active+Metal_2+             */
	{ 94, 51, 72}, /* 14:        +Poly1+Pseudo_S_Active+Metal_2+             */
	{ 36, 42, 72}, /* 15: Metal_1+Poly1+Pseudo_S_Active+Metal_2+             */
	{240,221,181}, /* 16:        +     +               +       +Pseudo_NWell */
	{ 91,182,181}, /* 17: Metal_1+     +               +       +Pseudo_NWell */
	{240,134,136}, /* 18:        +Poly1+               +       +Pseudo_NWell */
	{ 91,111,136}, /* 19: Metal_1+Poly1+               +       +Pseudo_NWell */
	{101,196, 68}, /* 20:        +     +Pseudo_S_Active+       +Pseudo_NWell */
	{ 38,161, 68}, /* 21: Metal_1+     +Pseudo_S_Active+       +Pseudo_NWell */
	{101,119, 51}, /* 22:        +Poly1+Pseudo_S_Active+       +Pseudo_NWell */
	{ 38, 98, 51}, /* 23: Metal_1+Poly1+Pseudo_S_Active+       +Pseudo_NWell */
	{211, 82,181}, /* 24:        +     +               +Metal_2+Pseudo_NWell */
	{ 80, 68,181}, /* 25: Metal_1+     +               +Metal_2+Pseudo_NWell */
	{211, 50,136}, /* 26:        +Poly1+               +Metal_2+Pseudo_NWell */
	{ 80, 41,136}, /* 27: Metal_1+Poly1+               +Metal_2+Pseudo_NWell */
	{ 89, 73, 68}, /* 28:        +     +Pseudo_S_Active+Metal_2+Pseudo_NWell */
	{ 33, 60, 68}, /* 29: Metal_1+     +Pseudo_S_Active+Metal_2+Pseudo_NWell */
	{ 89, 44, 51}, /* 30:        +Poly1+Pseudo_S_Active+Metal_2+Pseudo_NWell */
	{ 33, 36, 51}, /* 31: Metal_1+Poly1+Pseudo_S_Active+Metal_2+Pseudo_NWell */
};

/******************** DESIGN RULES ********************/

static INTBIG dodcmosn_unconnectedtable[] = {
/*            M  M  V  P  T  P  P  P  P  P  P  N  P  N  C  P  P  P  P  P  P  A   */
/*               0           M  M  S  F  0  I  I  1        P  A  P  N  P  N      */
/*                              0  A                             I  I  0         */
/*                                                                               */
/*                                                                               */
/*                                                                               */
/* M      */ K3,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* M0     */    K3,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* V      */       K4,XX,XX,XX,XX,XX,XX,K2,XX,XX,XX,XX,K3,XX,XX,XX,XX,XX,XX,K2,
/* P      */          XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* T      */             XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PM     */                XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PM0    */                   XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PSA    */                      XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PF     */                         XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* P0     */                            K3,XX,XX,XX,XX,K3,XX,XX,XX,XX,XX,XX,K2,
/* PI     */                               K4,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* NI     */                                  K4,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* P1     */                                     K14,K1,XX,XX,XX,XX,XX,XX,XX,XX,
/* N      */                                        K14,XX,XX,XX,XX,XX,XX,XX,XX,
/* C      */                                           K3,XX,XX,XX,XX,XX,XX,XX,
/* PP     */                                              XX,XX,XX,XX,XX,XX,XX,
/* PA     */                                                 XX,XX,XX,XX,XX,XX,
/* PPI    */                                                    XX,XX,XX,XX,XX,
/* PNI    */                                                       XX,XX,XX,XX,
/* PP0    */                                                          XX,XX,XX,
/* PN     */                                                             XX,XX,
/* A      */                                                                K4
};

static INTBIG dodcmosn_connectedtable[] = {
/*            M  M  V  P  T  P  P  P  P  P  P  N  P  N  C  P  P  P  P  P  P  A   */
/*               0           M  M  S  F  0  I  I  1        P  A  P  N  P  N      */
/*                              0  A                             I  I  0         */
/*                                                                               */
/*                                                                               */
/*                                                                               */
/* M      */ XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* M0     */    XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* V      */       XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,K3,XX,XX,XX,XX,XX,XX,XX,
/* P      */          XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* T      */             XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PM     */                XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PM0    */                   XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PSA    */                      XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PF     */                         XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* P0     */                            XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PI     */                               XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* NI     */                                  XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* P1     */                                     K6,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* N      */                                        K6,XX,XX,XX,XX,XX,XX,XX,XX,
/* C      */                                           XX,XX,XX,XX,XX,XX,XX,XX,
/* PP     */                                              XX,XX,XX,XX,XX,XX,XX,
/* PA     */                                                 XX,XX,XX,XX,XX,XX,
/* PPI    */                                                    XX,XX,XX,XX,XX,
/* PNI    */                                                       XX,XX,XX,XX,
/* PP0    */                                                          XX,XX,XX,
/* PN     */                                                             XX,XX,
/* A      */                                                                XX
};

/******************** ARCS ********************/

#define ARCPROTOCOUNT 5
#define AMETAL_1      0		/* Metal_1 */
#define AMETAL_2      1		/* Metal_2 */
#define APOLY1        2		/* Poly1 */
#define AP_ACTIVE     3		/* P+Active */
#define AN_ACTIVE     4		/* N+Active */

static TECH_ARCLAY dodcmosn_al_0[] = {{LM,0,FILLED}};
static TECH_ARCS dodcmosn_a_0 = {
	x_("Metal_1"), K3, AMETAL_1, NOARCPROTO,
	1, dodcmosn_al_0,
	(APMETAL1<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};

static TECH_ARCLAY dodcmosn_al_1[] = {{LM0,0,FILLED}};
static TECH_ARCS dodcmosn_a_1 = {
	x_("Metal_2"), K4, AMETAL_2, NOARCPROTO,
	1, dodcmosn_al_1,
	(APMETAL2<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};

static TECH_ARCLAY dodcmosn_al_2[] = {{LP0,0,FILLED}};
static TECH_ARCS dodcmosn_a_2 = {
	x_("Poly1"), K2, APOLY1, NOARCPROTO,
	1, dodcmosn_al_2,
	(APPOLY1<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};

static TECH_ARCLAY dodcmosn_al_3[] = {{LA,K12,FILLED}, {LPI,K8,FILLED}, {LN,0,CLOSED}};
static TECH_ARCS dodcmosn_a_3 = {
	x_("P+Active"), K18, AP_ACTIVE, NOARCPROTO,
	3, dodcmosn_al_3,
	(APDIFFP<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|WANTNOEXTEND|(90<<AANGLEINCSH)};

static TECH_ARCLAY dodcmosn_al_4[] = {{LA,K12,FILLED}, {LNI,K8,FILLED}, {LP1,0,CLOSED}};
static TECH_ARCS dodcmosn_a_4 = {
	x_("N+Active"), K18, AN_ACTIVE, NOARCPROTO,
	3, dodcmosn_al_4,
	(APDIFFN<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|WANTNOEXTEND|(90<<AANGLEINCSH)};

TECH_ARCS *dodcmosn_arcprotos[ARCPROTOCOUNT+1] = {
	&dodcmosn_a_0, &dodcmosn_a_1, &dodcmosn_a_2, &dodcmosn_a_3, &dodcmosn_a_4,
	((TECH_ARCS *)-1)};

static INTBIG dodcmosn_arc_widoff[ARCPROTOCOUNT] = {0, 0, 0, K12, K12};

/******************** PORT CONNECTIONS ********************/

static INTBIG dodcmosn_pc_1[] = {-1, ALLGEN, -1};
static INTBIG dodcmosn_pc_2[] = {-1, AN_ACTIVE, ALLGEN, -1};
static INTBIG dodcmosn_pc_3[] = {-1, AMETAL_1, AMETAL_2, ALLGEN, -1};
static INTBIG dodcmosn_pc_4[] = {-1, AP_ACTIVE, ALLGEN, -1};
static INTBIG dodcmosn_pc_5[] = {-1, APOLY1, AMETAL_1, ALLGEN, -1};
static INTBIG dodcmosn_pc_6[] = {-1, AN_ACTIVE, AMETAL_1, ALLGEN, -1};
static INTBIG dodcmosn_pc_7[] = {-1, AMETAL_1, AP_ACTIVE, ALLGEN, -1};
static INTBIG dodcmosn_pc_8[] = {-1, AP_ACTIVE, AN_ACTIVE, ALLGEN, -1};
static INTBIG dodcmosn_pc_9[] = {-1, APOLY1, ALLGEN, -1};
static INTBIG dodcmosn_pc_10[] = {-1, AMETAL_2, ALLGEN, -1};
static INTBIG dodcmosn_pc_11[] = {-1, AMETAL_1, ALLGEN, -1};

/******************** RECTANGLE DESCRIPTIONS ********************/

static INTBIG dodcmosn_box1[16] = {LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE, CENTERL1, CENTERD1, CENTERR1, CENTERU1};
static INTBIG dodcmosn_box2[16] = {LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE, CENTERL2, CENTERD2, CENTERR2, CENTERU2};
static INTBIG dodcmosn_box3[16] = {LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE, CENTERL1H, CENTERD1H, CENTERR1H, CENTERU1H};
static INTBIG dodcmosn_box4[8] = {LEFTIN4, BOTIN6, RIGHTIN4, TOPIN6};
static INTBIG dodcmosn_box5[16] = {LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE, CENTERL3H, CENTERD3H, CENTERR3H, CENTERU3H};
static INTBIG dodcmosn_box6[8] = {LEFTIN6, BOTIN2, RIGHTIN6, BOTIN6};
static INTBIG dodcmosn_box7[8] = {LEFTIN6, TOPIN6, RIGHTIN6, TOPIN2};
static INTBIG dodcmosn_box8[8] = {LEFTIN6, BOTIN2, RIGHTIN6, TOPIN2};
static INTBIG dodcmosn_box9[8] = {LEFTEDGE, BOTIN3, RIGHTEDGE, TOPIN3};
static INTBIG dodcmosn_box10[8] = {LEFTIN4, BOTEDGE, RIGHTIN4, TOPEDGE};
static INTBIG dodcmosn_box11[16] = {LEFTIN4, BOTIN6, RIGHTIN4, TOPIN6, CENTERL5, CENTERD1, CENTERR5, CENTERU1};
static INTBIG dodcmosn_box12[16] = {LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE, CENTERL3, CENTERD3, CENTERR3, CENTERU3};
static INTBIG dodcmosn_box13[8] = {LEFTIN4, BOTIN4, RIGHTIN4, TOPIN4};
static INTBIG dodcmosn_box14[8] = {LEFTIN6, BOTIN6, RIGHTIN6, TOPIN6};
static INTBIG dodcmosn_box15[8] = {LEFTIN1, BOTIN1, LEFTIN3, BOTIN3};
static INTBIG dodcmosn_box16[16] = {LEFTIN6, BOTIN6, RIGHTIN6, TOPIN6, CENTERL3, CENTERD3, CENTERR3, CENTERU3};
static INTBIG dodcmosn_box17[16] = {LEFTIN4, BOTIN4, RIGHTIN4, TOPIN4, CENTERL5, CENTERD5, CENTERR5, CENTERU5};
static INTBIG dodcmosn_box18[8] = {LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE};

/******************** NODES ********************/

#define NODEPROTOCOUNT 24
#define NPM            1		/* PIN_Metal1 */
#define NPM0           2		/* PIN_Metal2 */
#define NPP            3		/* PIN_Poly1 */
#define NPA            4		/* PIN_Active */
#define NCMTP          5		/* Contact_M1_to_P+ */
#define NCMTN          6		/* Contact_M1_to_N+ */
#define NCMTP0         7		/* Contact_M1_to_Poly1 */
#define NTP            8		/* Tran_P+ */
#define NCMTM          9		/* Contact_M1_to_M2 */
#define NCMTN0         10		/* Contact_M1_to_NWell */
#define NCMTP1         11		/* Contact_M1_to_PWell */
#define NTN            12		/* Tran_N+ */
#define NPM1           13		/* PLN_Metal1 */
#define NPM2           14		/* PLN_Metal2 */
#define NPP0           15		/* PLN_Poly1 */
#define NPA0           16		/* PLN_Active */
#define NPPI           17		/* PLN_P+Implant */
#define NPCC           18		/* PLN_Contact_Cut */
#define NPV            19		/* PLN_Via */
#define NPP1           20		/* PLN_Pwell */
#define NPP2           21		/* PLN_Passivation */
#define NPPF           22		/* PLN_Pad_Frame */
#define NPN            23		/* PLN_NWell */
#define NPNI           24		/* PLN_N+Implant */

/* PIN_Metal1 */
static TECH_PORTS dodcmosn_pm_p[] = {
	{dodcmosn_pc_11, x_("metal-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON dodcmosn_pm_l[] = {
	{LPM, 0, 4, CROSSED, BOX, dodcmosn_box18}};
static TECH_NODES dodcmosn_pm = {
	x_("PIN_Metal1"), NPM, NONODEPROTO,
	K3, K3,
	1, dodcmosn_pm_p,
	1, dodcmosn_pm_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* PIN_Metal2 */
static TECH_PORTS dodcmosn_pm0_p[] = {
	{dodcmosn_pc_10, x_("metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON dodcmosn_pm0_l[] = {
	{LPM0, 0, 4, CROSSED, BOX, dodcmosn_box18}};
static TECH_NODES dodcmosn_pm0 = {
	x_("PIN_Metal2"), NPM0, NONODEPROTO,
	K4, K4,
	1, dodcmosn_pm0_p,
	1, dodcmosn_pm0_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* PIN_Poly1 */
static TECH_PORTS dodcmosn_pp_p[] = {
	{dodcmosn_pc_9, x_("poly1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON dodcmosn_pp_l[] = {
	{LPP, 0, 4, CROSSED, BOX, dodcmosn_box18}};
static TECH_NODES dodcmosn_pp = {
	x_("PIN_Poly1"), NPP, NONODEPROTO,
	K2, K2,
	1, dodcmosn_pp_p,
	1, dodcmosn_pp_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* PIN_Active */
static TECH_PORTS dodcmosn_pa_p[] = {
	{dodcmosn_pc_8, x_("active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON dodcmosn_pa_l[] = {
	{LPA, 0, 4, CROSSED, BOX, dodcmosn_box18}};
static TECH_NODES dodcmosn_pa = {
	x_("PIN_Active"), NPA, NONODEPROTO,
	K2, K2,
	1, dodcmosn_pa_p,
	1, dodcmosn_pa_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* Contact_M1_to_P+ */
static TECH_PORTS dodcmosn_cmtp_p[] = {
	{dodcmosn_pc_7, x_("metal-1-s-act"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN7, BOTIN7, RIGHTIN7, TOPIN7}};
static TECH_POLYGON dodcmosn_cmtp_l[] = {
	{LM, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box16},
	{LA, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box16},
	{LPI, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box17},
	{LN, 0, 4, CLOSEDRECT, BOX, dodcmosn_box18},
	{LC, 0, 4, FILLEDRECT, BOX, dodcmosn_box15}};
static TECH_NODES dodcmosn_cmtp = {
	x_("Contact_M1_to_P+"), NCMTP, NONODEPROTO,
	K18, K18,
	1, dodcmosn_cmtp_p,
	5, dodcmosn_cmtp_l,
	(NPCONTACT<<NFUNCTIONSH),
	MULTICUT,K2,K2,K2,K3,0,0,0,0};

/* Contact_M1_to_N+ */
static TECH_PORTS dodcmosn_cmtn_p[] = {
	{dodcmosn_pc_6, x_("metal-1-d-act"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN7, BOTIN7, RIGHTIN7, TOPIN7}};
static TECH_POLYGON dodcmosn_cmtn_l[] = {
	{LM, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box16},
	{LA, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box16},
	{LNI, 0, 4, FILLEDRECT, BOX, dodcmosn_box13},
	{LP1, 0, 4, CLOSEDRECT, BOX, dodcmosn_box18},
	{LC, 0, 4, FILLEDRECT, BOX, dodcmosn_box15}};
static TECH_NODES dodcmosn_cmtn = {
	x_("Contact_M1_to_N+"), NCMTN, NONODEPROTO,
	K18, K18,
	1, dodcmosn_cmtn_p,
	5, dodcmosn_cmtn_l,
	(NPCONTACT<<NFUNCTIONSH),
	MULTICUT,K2,K2,K2,K3,0,0,0,0};

/* Contact_M1_to_Poly1 */
static TECH_PORTS dodcmosn_cmtp0_p[] = {
	{dodcmosn_pc_5, x_("metal-1-polysilicon"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON dodcmosn_cmtp0_l[] = {
	{LM, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box12},
	{LP0, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box12},
	{LC, 0, 4, FILLEDRECT, BOX, dodcmosn_box15}};
static TECH_NODES dodcmosn_cmtp0 = {
	x_("Contact_M1_to_Poly1"), NCMTP0, NONODEPROTO,
	K6, K6,
	1, dodcmosn_cmtp0_p,
	3, dodcmosn_cmtp0_l,
	(NPCONTACT<<NFUNCTIONSH),
	MULTICUT,K2,K2,K2,K3,0,0,0,0};

/* Tran_P+ */
static TECH_PORTS dodcmosn_tp_p[] = {
	{dodcmosn_pc_9, x_("s-trans-poly-left"), NOPORTPROTO, (45<<PORTARANGESH)|(180<<PORTANGLESH)|(1<<PORTNETSH),
		LEFTIN4, BOTIN7, LEFTIN4, TOPIN7},
	{dodcmosn_pc_4, x_("s-trans-diff-top"), NOPORTPROTO, (45<<PORTARANGESH)|(90<<PORTANGLESH),
		LEFTIN9, TOPIN3, RIGHTIN9, TOPIN3},
	{dodcmosn_pc_9, x_("s-trans-poly-right"), NOPORTPROTO, (45<<PORTARANGESH)|(1<<PORTNETSH),
		RIGHTIN4, BOTIN7, RIGHTIN4, TOPIN7},
	{dodcmosn_pc_4, x_("s-trans-diff-bottom"), NOPORTPROTO, (45<<PORTARANGESH)|(270<<PORTANGLESH)|(3<<PORTNETSH),
		LEFTIN9, BOTIN3, RIGHTIN9, BOTIN3}};
static TECH_SERPENT dodcmosn_tp_l[] = {
	{{LP0, 0, 4, FILLEDRECT, POINTS, dodcmosn_box11}, K1, K1, K2, K2},
	{{LA,  0, 4, FILLEDRECT, BOX,    dodcmosn_box8},  K5, K5, K0, K0},
	{{LPI,-1, 4, FILLEDRECT, BOX,    dodcmosn_box10}, K7, K7, K2, K2},
	{{LN, -1, 4, CLOSEDRECT, BOX,    dodcmosn_box9},  K4, K4, K6, K6}};
static TECH_SERPENT dodcmosn_tpE_l[] = {
	{{LP0, 0, 4, FILLEDRECT, POINTS, dodcmosn_box11}, K1, K1, K2, K2},
	{{LA,  1, 4, FILLEDRECT, BOX,    dodcmosn_box7},  K5,  0, K0, K0},
	{{LA,  3, 4, FILLEDRECT, BOX,    dodcmosn_box6},   0, K5, K0, K0},
	{{LPI,-1, 4, FILLEDRECT, BOX,    dodcmosn_box10}, K7, K7, K2, K2},
	{{LN, -1, 4, CLOSEDRECT, BOX,    dodcmosn_box9},  K4, K4, K6, K6}};
static TECH_NODES dodcmosn_tp = {
	x_("Tran_P+"), NTP, NONODEPROTO,
	K18, K14,
	4, dodcmosn_tp_p,
	4, (TECH_POLYGON *)0,
	(NPTRAPMOS<<NFUNCTIONSH)|HOLDSTRACE|NODESHRINK,
	SERPTRANS,5,K3,K3,K2,K1,K2,dodcmosn_tp_l,dodcmosn_tpE_l};

/* Contact_M1_to_M2 */
static TECH_PORTS dodcmosn_cmtm_p[] = {
	{dodcmosn_pc_3, x_("metal-1-metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON dodcmosn_cmtm_l[] = {
	{LM0, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box5},
	{LM, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box5},
	{LV, 0, 4, FILLEDRECT, BOX, dodcmosn_box15}};
static TECH_NODES dodcmosn_cmtm = {
	x_("Contact_M1_to_M2"), NCMTM, NONODEPROTO,
	K7, K7,
	1, dodcmosn_cmtm_p,
	3, dodcmosn_cmtm_l,
	(NPCONTACT<<NFUNCTIONSH),
	MULTICUT,K3,K3,K2,K4,0,0,0,0};

/* Contact_M1_to_NWell */
static TECH_PORTS dodcmosn_cmtn0_p[] = {
	{dodcmosn_pc_6, x_("M1NWell"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN6, BOTIN6, RIGHTIN6, TOPIN6}};
static TECH_POLYGON dodcmosn_cmtn0_l[] = {
	{LA, 0, 4, FILLEDRECT, BOX, dodcmosn_box14},
	{LM, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box16},
	{LNI, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box17},
	{LN, 0, 4, CLOSEDRECT, BOX, dodcmosn_box18},
	{LC, 0, 4, FILLEDRECT, BOX, dodcmosn_box15}};
static TECH_NODES dodcmosn_cmtn0 = {
	x_("Contact_M1_to_NWell"), NCMTN0, NONODEPROTO,
	K18, K18,
	1, dodcmosn_cmtn0_p,
	5, dodcmosn_cmtn0_l,
	(NPWELL<<NFUNCTIONSH),
	MULTICUT,K2,K2,K2,K3,0,0,0,0};

/* Contact_M1_to_PWell */
static TECH_PORTS dodcmosn_cmtp1_p[] = {
	{dodcmosn_pc_7, x_("metal-1-substrate"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN6, BOTIN6, RIGHTIN6, TOPIN6}};
static TECH_POLYGON dodcmosn_cmtp1_l[] = {
	{LM, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box16},
	{LA, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box16},
	{LPI, 0, 4, FILLEDRECT, BOX, dodcmosn_box13},
	{LP1, 0, 4, CLOSEDRECT, BOX, dodcmosn_box18},
	{LC, 0, 4, FILLEDRECT, BOX, dodcmosn_box15}};
static TECH_NODES dodcmosn_cmtp1 = {
	x_("Contact_M1_to_PWell"), NCMTP1, NONODEPROTO,
	K18, K18,
	1, dodcmosn_cmtp1_p,
	5, dodcmosn_cmtp1_l,
	(NPSUBSTRATE<<NFUNCTIONSH),
	MULTICUT,K2,K2,K2,K3,0,0,0,0};

/* Tran_N+ */
static TECH_PORTS dodcmosn_tn_p[] = {
	{dodcmosn_pc_9, x_("d-trans-poly-right"), NOPORTPROTO, (45<<PORTARANGESH),
		RIGHTIN4, BOTIN7, RIGHTIN4, TOPIN7},
	{dodcmosn_pc_2, x_("d-trans-diff-bottom"), NOPORTPROTO, (45<<PORTARANGESH)|(270<<PORTANGLESH)|(1<<PORTNETSH),
		LEFTIN9, BOTIN3, RIGHTIN9, BOTIN3},
	{dodcmosn_pc_9, x_("d-trans-poly-left"), NOPORTPROTO, (45<<PORTARANGESH)|(180<<PORTANGLESH),
		LEFTIN4, BOTIN7, LEFTIN4, TOPIN7},
	{dodcmosn_pc_2, x_("d-trans-diff-top"), NOPORTPROTO, (45<<PORTARANGESH)|(90<<PORTANGLESH)|(3<<PORTNETSH),
		LEFTIN9, TOPIN3, RIGHTIN9, TOPIN3}};
static TECH_SERPENT dodcmosn_tn_l[] = {
	{{LA,  0, 4, FILLEDRECT, BOX, dodcmosn_box8},  K5, K5, K0, K0},
	{{LP0, 0, 4, FILLEDRECT, BOX, dodcmosn_box4},  K1, K1, K2, K2},
	{{LNI,-1, 4, FILLEDRECT, BOX, dodcmosn_box10}, K7, K7, K2, K2},
	{{LP1,-1, 4, CLOSEDRECT, BOX, dodcmosn_box9},  K4, K4, K6, K6}};
static TECH_SERPENT dodcmosn_tnE_l[] = {
	{{LA,  3, 4, FILLEDRECT, BOX, dodcmosn_box7},  K5,  0, K0, K0},
	{{LA,  1, 4, FILLEDRECT, BOX, dodcmosn_box6},   0, K5, K0, K0},
	{{LP0, 0, 4, FILLEDRECT, BOX, dodcmosn_box4},  K1, K1, K2, K2},
	{{LNI,-1, 4, FILLEDRECT, BOX, dodcmosn_box10}, K7, K7, K2, K2},
	{{LP1,-1, 4, CLOSEDRECT, BOX, dodcmosn_box9},  K4, K4, K6, K6}};
static TECH_NODES dodcmosn_tn = {
	x_("Tran_N+"), NTN, NONODEPROTO,
	K18, K14,
	4, dodcmosn_tn_p,
	4, (TECH_POLYGON *)0,
	(NPTRANMOS<<NFUNCTIONSH)|HOLDSTRACE|NODESHRINK,
	SERPTRANS,5,K3,K3,K2,K1,K2,dodcmosn_tn_l,dodcmosn_tnE_l};

/* PLN_Metal1 */
static TECH_PORTS dodcmosn_pm1_p[] = {
	{dodcmosn_pc_11, x_("metal-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON dodcmosn_pm1_l[] = {
	{LM, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box3}};
static TECH_NODES dodcmosn_pm1 = {
	x_("PLN_Metal1"), NPM1, NONODEPROTO,
	K3, K3,
	1, dodcmosn_pm1_p,
	1, dodcmosn_pm1_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* PLN_Metal2 */
static TECH_PORTS dodcmosn_pm2_p[] = {
	{dodcmosn_pc_10, x_("metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON dodcmosn_pm2_l[] = {
	{LM0, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box2}};
static TECH_NODES dodcmosn_pm2 = {
	x_("PLN_Metal2"), NPM2, NONODEPROTO,
	K4, K4,
	1, dodcmosn_pm2_p,
	1, dodcmosn_pm2_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* PLN_Poly1 */
static TECH_PORTS dodcmosn_pp0_p[] = {
	{dodcmosn_pc_9, x_("polysilicon"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON dodcmosn_pp0_l[] = {
	{LP0, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box1}};
static TECH_NODES dodcmosn_pp0 = {
	x_("PLN_Poly1"), NPP0, NONODEPROTO,
	K2, K2,
	1, dodcmosn_pp0_p,
	1, dodcmosn_pp0_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* PLN_Active */
static TECH_PORTS dodcmosn_pa0_p[] = {
	{dodcmosn_pc_8, x_("active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON dodcmosn_pa0_l[] = {
	{LA, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box12}};
static TECH_NODES dodcmosn_pa0 = {
	x_("PLN_Active"), NPA0, NONODEPROTO,
	K6, K6,
	1, dodcmosn_pa0_p,
	1, dodcmosn_pa0_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* PLN_P+Implant */
static TECH_PORTS dodcmosn_ppi_p[] = {
	{dodcmosn_pc_1, x_("select"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON dodcmosn_ppi_l[] = {
	{LPI, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box2}};
static TECH_NODES dodcmosn_ppi = {
	x_("PLN_P+Implant"), NPPI, NONODEPROTO,
	K4, K4,
	1, dodcmosn_ppi_p,
	1, dodcmosn_ppi_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* PLN_Contact_Cut */
static TECH_PORTS dodcmosn_pcc_p[] = {
	{dodcmosn_pc_1, x_("cut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON dodcmosn_pcc_l[] = {
	{LC, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box1}};
static TECH_NODES dodcmosn_pcc = {
	x_("PLN_Contact_Cut"), NPCC, NONODEPROTO,
	K2, K2,
	1, dodcmosn_pcc_p,
	1, dodcmosn_pcc_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* PLN_Via */
static TECH_PORTS dodcmosn_pv_p[] = {
	{dodcmosn_pc_1, x_("via"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON dodcmosn_pv_l[] = {
	{LV, 0, 4, CLOSEDRECT, MINBOX, dodcmosn_box3}};
static TECH_NODES dodcmosn_pv = {
	x_("PLN_Via"), NPV, NONODEPROTO,
	K3, K3,
	1, dodcmosn_pv_p,
	1, dodcmosn_pv_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* PLN_Pwell */
static TECH_PORTS dodcmosn_pp1_p[] = {
	{dodcmosn_pc_4, x_("well"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON dodcmosn_pp1_l[] = {
	{LP1, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box12}};
static TECH_NODES dodcmosn_pp1 = {
	x_("PLN_Pwell"), NPP1, NONODEPROTO,
	K6, K6,
	1, dodcmosn_pp1_p,
	1, dodcmosn_pp1_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* PLN_Passivation */
static TECH_PORTS dodcmosn_pp2_p[] = {
	{dodcmosn_pc_1, x_("passivation"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON dodcmosn_pp2_l[] = {
	{LP, 0, 4, FILLEDRECT, BOX, dodcmosn_box18}};
static TECH_NODES dodcmosn_pp2 = {
	x_("PLN_Passivation"), NPP2, NONODEPROTO,
	K8, K8,
	1, dodcmosn_pp2_p,
	1, dodcmosn_pp2_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* PLN_Pad_Frame */
static TECH_PORTS dodcmosn_ppf_p[] = {
	{dodcmosn_pc_1, x_("pad-frame"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON dodcmosn_ppf_l[] = {
	{LPF, 0, 4, CLOSEDRECT, BOX, dodcmosn_box18}};
static TECH_NODES dodcmosn_ppf = {
	x_("PLN_Pad_Frame"), NPPF, NONODEPROTO,
	K8, K8,
	1, dodcmosn_ppf_p,
	1, dodcmosn_ppf_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* PLN_NWell */
static TECH_PORTS dodcmosn_pn_p[] = {
	{dodcmosn_pc_1, x_("NWell"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON dodcmosn_pn_l[] = {
	{LN, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box12}};
static TECH_NODES dodcmosn_pn = {
	x_("PLN_NWell"), NPN, NONODEPROTO,
	K6, K6,
	1, dodcmosn_pn_p,
	1, dodcmosn_pn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* PLN_N+Implant */
static TECH_PORTS dodcmosn_pni_p[] = {
	{dodcmosn_pc_1, x_("N+Imp"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON dodcmosn_pni_l[] = {
	{LNI, 0, 4, FILLEDRECT, MINBOX, dodcmosn_box2}};
static TECH_NODES dodcmosn_pni = {
	x_("PLN_N+Implant"), NPNI, NONODEPROTO,
	K4, K4,
	1, dodcmosn_pni_p,
	1, dodcmosn_pni_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

TECH_NODES *dodcmosn_nodeprotos[NODEPROTOCOUNT+1] = {
	&dodcmosn_pm, &dodcmosn_pm0, &dodcmosn_pp, &dodcmosn_pa,
	&dodcmosn_cmtp, &dodcmosn_cmtn, &dodcmosn_cmtp0,
	&dodcmosn_tp, &dodcmosn_cmtm, &dodcmosn_cmtn0,
	&dodcmosn_cmtp1, &dodcmosn_tn, &dodcmosn_pm1, &dodcmosn_pm2,
	&dodcmosn_pp0, &dodcmosn_pa0, &dodcmosn_ppi, &dodcmosn_pcc,
	&dodcmosn_pv, &dodcmosn_pp1, &dodcmosn_pp2, &dodcmosn_ppf,
	&dodcmosn_pn, &dodcmosn_pni, ((TECH_NODES *)-1)};

static INTBIG dodcmosn_node_widoff[NODEPROTOCOUNT*4] = {
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, K6,K6,K6,K6, K6,K6,K6,K6, 0,0,0,0,
	K6,K6,K6,K6, 0,0,0,0, K6,K6,K6,K6, K6,K6,K6,K6, K6,K6,K6,K6, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

/******************** VARIABLE AGGREGATION ********************/

TECH_VARIABLES dodcmosn_variables[] =
{
	{x_("TECH_layer_names"), (CHAR *)dodcmosn_layer_names, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_layer_function"), (CHAR *)dodcmosn_layer_function, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_node_width_offset"), (CHAR *)dodcmosn_node_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|((NODEPROTOCOUNT*4)<<VLENGTHSH)},
	{x_("TECH_arc_width_offset"), (CHAR *)dodcmosn_arc_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|(ARCPROTOCOUNT<<VLENGTHSH)},
	{x_("USER_layer_letters"), (CHAR *)dodcmosn_layer_letters, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("USER_color_map"), (CHAR *)dodcmosn_colmap, 0.0,
		VCHAR|VDONTSAVE|VISARRAY|((sizeof dodcmosn_colmap)<<VLENGTHSH)},
	{x_("IO_cif_layer_names"), (CHAR *)dodcmosn_cif_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("IO_gds_layer_numbers"), (CHAR *)dodcmosn_gds_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("DRC_min_unconnected_distances"), (CHAR *)dodcmosn_unconnectedtable, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|
		   (((sizeof dodcmosn_unconnectedtable)/SIZEOFINTBIG)<<VLENGTHSH)},
	{x_("DRC_min_connected_distances"), (CHAR *)dodcmosn_connectedtable, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|
			(((sizeof dodcmosn_connectedtable)/SIZEOFINTBIG)<<VLENGTHSH)},
	{NULL, NULL, 0.0, 0}
};

/******************** ROUTINES ********************/

BOOLEAN dodcmosn_initprocess(TECHNOLOGY *tech, INTBIG pass)
{
	Q_UNUSED( tech );
	Q_UNUSED( pass );
	return(FALSE);
}

void dodcmosn_shapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly)
{
	dodcmosn_intshapenodepoly(ni, box, poly, &tech_oneprocpolyloop);
}

void dodcmosn_intshapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl)
{
	REGISTER TECH_NODES *thistn, *othistn;
	REGISTER INTBIG lambda, sea;
	INTBIG lx, hx, ly, hy, olx, ohx, oly, ohy, clx, chx, cly, chy, cex, cey, ocex, ocey;
	REGISTER TECH_PORTS *p;
	REGISTER GEOM *geom;
	REGISTER NODEINST *oni;
	TECH_POLYGON *lay, *olay, *clay;
	REGISTER INTBIG pindex, count, i, olayer;
	static POLYGON *opoly = NOPOLYGON, *cpoly = NOPOLYGON;

	/* handle displayable variables */
	if (box >= pl->realpolys)
	{
		(void)tech_filldisplayablenvar(ni, poly, pl->curwindowpart, 0, pl);
		return;
	}

	pindex = ni->proto->primindex;
	thistn = dodcmosn_nodeprotos[pindex-1];
	lambda = lambdaofnode(ni);
	switch (thistn->special)
	{
		case SERPTRANS:
			if (box > 1 || (ni->userbits&NSHORT) == 0) p = (TECH_PORTS *)0; else
				p = thistn->portlist;
			tech_filltrans(poly, &lay, thistn->gra, ni, lambda, box, p, pl);
			break;

		case MULTICUT:
			count = thistn->layercount - 1;
			if (box >= count)
			{
				/* code cannot be called by multiple procesors: uses globals */
				NOT_REENTRANT;

				lay = &thistn->layerlist[count];
				tech_moscutpoly(ni, box-count, lay->points, pl);
				tech_fillpoly(poly, lay, ni, lambda, FILLED);
				break;
			}

		default:
			lay = &thistn->layerlist[box];
			tech_fillpoly(poly, lay, ni, lambda, FILLED);
			break;
	}
	poly->desc = dodcmosn_layers[poly->layer];

	/* now the special trimming code */
	if ((poly->layer == LPI || poly->layer == LNI) && ni->parent != NONODEPROTO)
	{
		/* get bounding box of this node, it must be manhattan */
		if (!isbox(poly, &lx, &hx, &ly, &hy)) return;
		if (poly->layer == LPI) olayer = LNI; else olayer = LPI;
		sea = initsearch(lx, hx, ly, hy, ni->parent);
		if (sea == -1) return;
		for(;;)
		{
			geom = nextobject(sea);
			if (geom == NOGEOM) break;
			if (geom->entryisnode)
			{
				oni = geom->entryaddr.ni;
				if (oni->proto->primindex == 0) continue;
				if (oni->proto->tech != ni->proto->tech) continue;
				if (poly->layer == LPI)
				{
					/* layer LPI interacts with only some nodes */
					if (oni->proto->primindex != NCMTN && oni->proto->primindex != NCMTN0 &&
						oni->proto->primindex != NTN) continue;
				} else
				{
					/* layer LNI interacts with only some nodes */
					if (oni->proto->primindex != NCMTP && oni->proto->primindex != NCMTP1 &&
						oni->proto->primindex != NTP) continue;
				}

				/* find polygon for opposite layer on node "oni" */
				othistn = dodcmosn_nodeprotos[oni->proto->primindex-1];
				if (othistn->special == SERPTRANS)
				{
					for(i=0; i<othistn->layercount; i++)
						if (othistn->gra[i].basics.layernum == olayer) break;
					if (i >= othistn->layercount) continue;
					olay = &othistn->gra[i].basics;
				} else
				{
					for(i=0; i<othistn->layercount; i++)
						if (othistn->layerlist[i].layernum == olayer) break;
					if (i >= othistn->layercount) continue;
					olay = &othistn->layerlist[i];
				}
				(void)needstaticpolygon(&opoly, 4, db_cluster);
				tech_fillpoly(opoly, olay, oni, lambda, FILLED);
				if (!isbox(opoly, &olx, &ohx, &oly, &ohy)) continue;

				/* if the boxes don't intersect, all is fine */
				if (ohx < lx || olx > hx || ohy < ly || oly > hy) continue;

				/* find the polygon for the Active layer on this node */
				if (thistn->special == SERPTRANS)
				{
					for(i=0; i<thistn->layercount; i++)
						if (thistn->gra[i].basics.layernum == LA) break;
					if (i >= thistn->layercount) continue;
					clay = &thistn->gra[i].basics;
				} else
				{
					for(i=0; i<thistn->layercount; i++)
						if (thistn->layerlist[i].layernum == LA) break;
					if (i >= thistn->layercount) continue;
					clay = &thistn->layerlist[i];
				}
				(void)needstaticpolygon(&cpoly, 4, db_cluster);
				tech_fillpoly(cpoly, clay, ni, lambda, FILLED);
				if (!isbox(cpoly, &clx, &chx, &cly, &chy)) continue;

				/* get center of the two nodes */
				cex = (lx + hx) / 2;      cey = (ly + hy) / 2;
				ocex = (olx + ohx) / 2;   ocey = (oly + ohy) / 2;

				/* now see if clipping to the Active layer makes the implants not intersect */
				if (hy > oly && ly < ohy)
				{
					if (cex > ocex && lx < ohx) lx = clx;
					if (ocex > cex && hx > olx) hx = chx;
				}
				if (hx > olx && lx < ohx)
				{
					if (cey > ocey && ly < ohy) ly = cly;
					if (ocey > cey && hy > oly) hy = chy;
				}
				if (poly->style == FILLEDRECT || poly->style == CLOSEDRECT)
					maketruerectpoly(lx, hx, ly, hy, poly); else
						makerectpoly(lx, hx, ly, hy, poly);
			}
		}
	}
}

INTBIG dodcmosn_allnodepolys(NODEINST *ni, POLYLIST *plist, WINDOWPART *win, BOOLEAN onlyreasonable)
{
	REGISTER INTBIG tot, j;
	INTBIG reasonable;
	REGISTER NODEPROTO *np;
	REGISTER POLYGON *poly;
	POLYLOOP mypl;

	np = ni->proto;
	mypl.curwindowpart = win;
	tot = tech_nodepolys(ni, &reasonable, win, &mypl);
	if (onlyreasonable) tot = reasonable;
	if (mypl.realpolys < tot) tot = mypl.realpolys;
	if (ensurepolylist(plist, tot, db_cluster)) return(-1);
	for(j = 0; j < tot; j++)
	{
		poly = plist->polygons[j];
		poly->tech = np->tech;
		dodcmosn_intshapenodepoly(ni, j, poly, &mypl);
	}
	return(tot);
}
