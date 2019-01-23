/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tecbicmos.c
 * bicmos technology description
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
#include "efunction.h"

BOOLEAN bicmos_initprocess(TECHNOLOGY *tech, INTBIG pass);

/******************** LAYERS ********************/

#define MAXLAYERS 27
#define LPS       0			/* P_Select */
#define LNS       1			/* N_Select */
#define LNW       2			/* N_Well */
#define LV        3			/* Via */
#define LP        4			/* Passivation */
#define LPF       5			/* Pad_Frame */
#define LT        6			/* Transistor */
#define LAC       7			/* Active_Cut */
#define LPM       8			/* Pseudo_Metal_1 */
#define LPM0      9			/* Pseudo_Metal_2 */
#define LPP       10		/* Pseudo_Polysilicon */
#define LPPS      11		/* Pseudo_P_Select */
#define LPNS      12		/* Pseudo_N_Select */
#define LPNW      13		/* Pseudo_N_Well */
#define LPP0      14		/* Pseudo_Polysilicon_2 */
#define LM        15		/* M1 */
#define LM0       16		/* M2 */
#define LP0       17		/* Poly1 */
#define LP1       18		/* Poly2 */
#define LA        19		/* Active */
#define LPC       20		/* Poly1_Cut */
#define LPC0      21		/* Poly2_Cut */
#define LPA       22		/* Pseudo_Active */
#define LPBA      23		/* P_Base_Active */
#define LB        24		/* BCCD */
#define LOS       25		/* Ohmic_Substrate */
#define LOW       26		/* Ohmic_Well */

static GRAPHICS bicmos_PS_lay = {LAYERO, YELLOW, PATTERNED|OUTLINEPAT,PATTERNED,
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
static GRAPHICS bicmos_NS_lay = {LAYERO, LGREEN, PATTERNED|OUTLINEPAT,PATTERNED,
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
static GRAPHICS bicmos_NW_lay = {LAYERT5, COLORT5, SOLIDC, PATTERNED,
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
static GRAPHICS bicmos_V_lay = {LAYERO, BLACK, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS bicmos_P_lay = {LAYERO, DGRAY, PATTERNED, PATTERNED,
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
static GRAPHICS bicmos_PF_lay = {LAYERO, RED, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS bicmos_T_lay = {LAYERO, ALLOFF, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS bicmos_AC_lay = {LAYERO, BLACK, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS bicmos_PM_lay = {LAYERT1, COLORT1, SOLIDC, PATTERNED,
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
static GRAPHICS bicmos_PM0_lay = {LAYERT4, COLORT4, SOLIDC, PATTERNED,
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
static GRAPHICS bicmos_PP_lay = {LAYERT2, COLORT2, SOLIDC, PATTERNED,
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
static GRAPHICS bicmos_PPS_lay = {LAYERO, YELLOW, PATTERNED|OUTLINEPAT, PATTERNED,
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
static GRAPHICS bicmos_PNS_lay = {LAYERO, LGREEN, PATTERNED|OUTLINEPAT, PATTERNED,
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
static GRAPHICS bicmos_PNW_lay = {LAYERT5, COLORT5, SOLIDC, PATTERNED,
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
static GRAPHICS bicmos_PP0_lay = {LAYERO, MAGENTA, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS bicmos_M_lay = {LAYERT1, COLORT1, SOLIDC, PATTERNED,
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
static GRAPHICS bicmos_M0_lay = {LAYERT4, COLORT4, SOLIDC, PATTERNED,
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
static GRAPHICS bicmos_P0_lay = {LAYERT2, COLORT2, SOLIDC, PATTERNED,
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
static GRAPHICS bicmos_P1_lay = {LAYERO, MAGENTA, PATTERNED|OUTLINEPAT, PATTERNED,
	{0xe0e0, /* XXX     XXX      */
	0x7070,  /*  XXX     XXX     */
	0x3838,  /*   XXX     XXX    */
	0x1c1c,  /*    XXX     XXX   */
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
	0xc1c1}, /* XX     XXX     X */
	NOVARIABLE, 0};
static GRAPHICS bicmos_A_lay = {LAYERT3, COLORT3, SOLIDC, PATTERNED,
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
static GRAPHICS bicmos_PC_lay = {LAYERO, BLACK, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS bicmos_PC0_lay = {LAYERO, BLACK, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS bicmos_PA_lay = {LAYERT3, COLORT3, SOLIDC, PATTERNED,
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
static GRAPHICS bicmos_PBA_lay = {LAYERT3, COLORT3, PATTERNED|OUTLINEPAT, PATTERNED,
	{0x4444, /*  X   X   X   X   */
	0x2222,  /*   X   X   X   X  */
	0x1111,  /*    X   X   X   X */
	0x8888,  /* X   X   X   X    */
	0x4444,  /*  X   X   X   X   */
	0x2222,  /*   X   X   X   X  */
	0x1111,  /*    X   X   X   X */
	0x0888,  /*     X   X   X    */
	0x4444,  /*  X   X   X   X   */
	0x2222,  /*   X   X   X   X  */
	0x1111,  /*    X   X   X   X */
	0x8888,  /* X   X   X   X    */
	0x4444,  /*  X   X   X   X   */
	0x2222,  /*   X   X   X   X  */
	0x1111,  /*    X   X   X   X */
	0x0888}, /*     X   X   X    */
	NOVARIABLE, 0};
static GRAPHICS bicmos_B_lay = {LAYERO, LRED, PATTERNED|OUTLINEPAT, PATTERNED,
	{0x8888, /* X   X   X   X    */
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
	0x0000,  /*                  */
	0x2222,  /*   X   X   X   X  */
	0x0000}, /*                  */
	NOVARIABLE, 0};
static GRAPHICS bicmos_OS_lay = {LAYERT3, COLORT3, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS bicmos_OW_lay = {LAYERT3, COLORT3, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};

GRAPHICS *bicmos_layers[MAXLAYERS+1] = {&bicmos_PS_lay, &bicmos_NS_lay,
	&bicmos_NW_lay, &bicmos_V_lay, &bicmos_P_lay, &bicmos_PF_lay, &bicmos_T_lay,
	&bicmos_AC_lay, &bicmos_PM_lay, &bicmos_PM0_lay, &bicmos_PP_lay,
	&bicmos_PPS_lay, &bicmos_PNS_lay, &bicmos_PNW_lay, &bicmos_PP0_lay,
	&bicmos_M_lay, &bicmos_M0_lay, &bicmos_P0_lay, &bicmos_P1_lay, &bicmos_A_lay,
	&bicmos_PC_lay, &bicmos_PC0_lay, &bicmos_PA_lay, &bicmos_PBA_lay,
	&bicmos_B_lay, &bicmos_OS_lay, &bicmos_OW_lay, NOGRAPHICS};
static CHAR *bicmos_layer_names[MAXLAYERS] = {x_("P_Select"), x_("N_Select"), x_("N_Well"),
	x_("Via"), x_("Passivation"), x_("Pad_Frame"), x_("Transistor"), x_("Active_Cut"),
	x_("Pseudo_Metal_1"), x_("Pseudo_Metal_2"), x_("Pseudo_Polysilicon"), x_("Pseudo_P_Select"),
	x_("Pseudo_N_Select"), x_("Pseudo_N_Well"), x_("Pseudo_Polysilicon_2"), x_("M1"), x_("M2"),
	x_("Poly1"), x_("Poly2"), x_("Active"), x_("Poly1_Cut"), x_("Poly2_Cut"), x_("Pseudo_Active"),
	x_("P_Base_Active"), x_("BCCD"), x_("Ohmic_Substrate"), x_("Ohmic_Well")};
static CHAR *bicmos_cif_layers[MAXLAYERS] = {x_("CSP"), x_("CSN"), x_("CWN"), x_("CVA"), x_("COG"), x_("XP"), x_(""), x_("CCA"), x_(""), x_(""), x_(""), x_(""), x_(""), x_(""), x_(""), x_("CMF"), x_("CMS"), x_("CPG"), x_("CEL"), x_("CAA"), x_("CCP"), x_("CCE"), x_(""), x_("CBA"), x_("CCD"), x_("CAA"), x_("CAA")};
static CHAR *bicmos_gds_layers[MAXLAYERS] = {x_("8"), x_("7"), x_("1"), x_(""), x_("13"), x_("9"), x_(""), x_("35"), x_(""), x_(""), x_(""), x_(""), x_(""), x_(""), x_(""), x_("10"), x_("12"), x_("4"), x_("19"), x_("31"), x_("45"), x_("55"), x_(""), x_("33"), x_("17"), x_("3"), x_("3")};
static INTBIG bicmos_layer_function[MAXLAYERS] = {LFIMPLANT|LFPTYPE,
	LFIMPLANT|LFNTYPE, LFWELL|LFNTYPE|LFTRANS5, LFCONTACT2, LFOVERGLASS, LFART,
	LFTRANSISTOR|LFPSEUDO, LFCONTACT1, LFMETAL1|LFPSEUDO|LFTRANS1,
	LFMETAL2|LFPSEUDO|LFTRANS4, LFPOLY1|LFPSEUDO|LFTRANS2,
	LFIMPLANT|LFPTYPE|LFPSEUDO, LFIMPLANT|LFNTYPE|LFPSEUDO,
	LFWELL|LFNTYPE|LFPSEUDO|LFTRANS5, LFPOLY2|LFPSEUDO, LFMETAL1|LFTRANS1,
	LFMETAL2|LFTRANS4, LFPOLY1|LFTRANS2, LFPOLY2, LFDIFF|LFTRANS3, LFCONTACT1,
	LFCONTACT3, LFDIFF|LFPSEUDO|LFTRANS3, LFDIFF|LFPTYPE|LFTRANS3,
	LFIMPLANT|LFNTYPE, LFSUBSTRATE|LFTRANS3, LFWELL|LFTRANS3};
static CHAR *bicmos_layer_letters[MAXLAYERS] = {x_("e"), x_("f"), x_("n"), x_("v"), x_("o"), x_("b"), x_("t"), x_("A"), x_("M"), x_("H"), x_("P"), x_("E"), x_("F"), x_("N"), x_("x"), x_("m"), x_("h"), x_("p"), x_("g"), x_("d"), x_("a"), x_("i"), x_("S"), x_("x"), x_("c"), x_("l"), x_("r")};

static TECH_COLORMAP bicmos_colmap[32] =
{
	{200,200,200}, /*  0:               +                  +      +              +       */
	{ 96,209,255}, /*  1: Pseudo_Metal_1+                  +      +              +       */
	{255,155,192}, /*  2:               +Pseudo_Polysilicon+      +              +       */
	{ 96,127,192}, /*  3: Pseudo_Metal_1+Pseudo_Polysilicon+      +              +       */
	{107,226, 96}, /*  4:               +                  +Active+              +       */
	{ 40,186, 96}, /*  5: Pseudo_Metal_1+                  +Active+              +       */
	{107,137, 72}, /*  6:               +Pseudo_Polysilicon+Active+              +       */
	{ 40,113, 72}, /*  7: Pseudo_Metal_1+Pseudo_Polysilicon+Active+              +       */
	{224, 95,255}, /*  8:               +                  +      +Pseudo_Metal_2+       */
	{ 85, 78,255}, /*  9: Pseudo_Metal_1+                  +      +Pseudo_Metal_2+       */
	{224, 57,192}, /* 10:               +Pseudo_Polysilicon+      +Pseudo_Metal_2+       */
	{ 85, 47,192}, /* 11: Pseudo_Metal_1+Pseudo_Polysilicon+      +Pseudo_Metal_2+       */
	{ 94, 84, 96}, /* 12:               +                  +Active+Pseudo_Metal_2+       */
	{ 36, 69, 96}, /* 13: Pseudo_Metal_1+                  +Active+Pseudo_Metal_2+       */
	{ 94, 51, 72}, /* 14:               +Pseudo_Polysilicon+Active+Pseudo_Metal_2+       */
	{ 36, 42, 72}, /* 15: Pseudo_Metal_1+Pseudo_Polysilicon+Active+Pseudo_Metal_2+       */
	{240,221,181}, /* 16:               +                  +      +              +N_Well */
	{ 91,182,181}, /* 17: Pseudo_Metal_1+                  +      +              +N_Well */
	{240,134,136}, /* 18:               +Pseudo_Polysilicon+      +              +N_Well */
	{ 91,111,136}, /* 19: Pseudo_Metal_1+Pseudo_Polysilicon+      +              +N_Well */
	{101,196, 68}, /* 20:               +                  +Active+              +N_Well */
	{ 38,161, 68}, /* 21: Pseudo_Metal_1+                  +Active+              +N_Well */
	{101,119, 51}, /* 22:               +Pseudo_Polysilicon+Active+              +N_Well */
	{ 38, 98, 51}, /* 23: Pseudo_Metal_1+Pseudo_Polysilicon+Active+              +N_Well */
	{211, 82,181}, /* 24:               +                  +      +Pseudo_Metal_2+N_Well */
	{ 80, 68,181}, /* 25: Pseudo_Metal_1+                  +      +Pseudo_Metal_2+N_Well */
	{211, 50,136}, /* 26:               +Pseudo_Polysilicon+      +Pseudo_Metal_2+N_Well */
	{ 80, 41,136}, /* 27: Pseudo_Metal_1+Pseudo_Polysilicon+      +Pseudo_Metal_2+N_Well */
	{ 89, 73, 68}, /* 28:               +                  +Active+Pseudo_Metal_2+N_Well */
	{ 33, 60, 68}, /* 29: Pseudo_Metal_1+                  +Active+Pseudo_Metal_2+N_Well */
	{ 89, 44, 51}, /* 30:               +Pseudo_Polysilicon+Active+Pseudo_Metal_2+N_Well */
	{ 33, 36, 51}, /* 31: Pseudo_Metal_1+Pseudo_Polysilicon+Active+Pseudo_Metal_2+N_Well */
};

/******************** DESIGN RULES ********************/

static INTBIG bicmos_unconnectedtable[] = {
/*            P  N  N  V  P  P  T  A  P  P  P  P  P  P  P  M  M  P  P  A  P  P  P  P  B  O  O   */
/*            S  S  W        F     C  M  M  P  P  N  N  P     0  0  1     C  C  A  B     S  W   */
/*                                       0     S  S  W  0                    0     A            */
/*                                                                                              */
/*                                                                                              */
/*                                                                                              */
/* PS     */ XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* NS     */    XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* NW     */       XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* V      */          K3,XX,XX,K2,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,K2,XX,K2,XX,XX,XX,XX,XX,XX,XX,
/* P      */             XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PF     */                XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* T      */                   XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* AC     */                      XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,K5,XX,XX,XX,XX,XX,XX,XX,
/* PM     */                         XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PM0    */                            XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PP     */                               XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PPS    */                                  XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PNS    */                                     XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PNW    */                                        XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PP0    */                                           XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* M      */                                              K3,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* M0     */                                                 K4,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* P0     */                                                    K2,XX,K1,K4,XX,XX,XX,XX,XX,XX,
/* P1     */                                                       K3,XX,XX,XX,XX,XX,XX,XX,XX,
/* A      */                                                          K3,XX,XX,XX,XX,XX,XX,XX,
/* PC     */                                                             XX,XX,XX,XX,XX,XX,XX,
/* PC0    */                                                                XX,XX,XX,XX,XX,XX,
/* PA     */                                                                   XX,XX,XX,XX,XX,
/* PBA    */                                                                      XX,XX,XX,XX,
/* B      */                                                                         XX,XX,XX,
/* OS     */                                                                            XX,XX,
/* OW     */                                                                               XX
};

static INTBIG bicmos_connectedtable[] = {
/*            P  N  N  V  P  P  T  A  P  P  P  P  P  P  P  M  M  P  P  A  P  P  P  P  B  O  O   */
/*            S  S  W        F     C  M  M  P  P  N  N  P     0  0  1     C  C  A  B     S  W   */
/*                                       0     S  S  W  0                    0     A            */
/*                                                                                              */
/*                                                                                              */
/*                                                                                              */
/* PS     */ XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* NS     */    XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* NW     */       XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* V      */          K2,XX,XX,XX,K2,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,K2,XX,XX,XX,XX,XX,XX,
/* P      */             XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PF     */                XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* T      */                   XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* AC     */                      K2,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PM     */                         XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PM0    */                            XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PP     */                               XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PPS    */                                  XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PNS    */                                     XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PNW    */                                        XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PP0    */                                           XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* M      */                                              XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* M0     */                                                 XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* P0     */                                                    XX,XX,K1,XX,XX,XX,XX,XX,XX,XX,
/* P1     */                                                       XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* A      */                                                          XX,XX,XX,XX,XX,XX,XX,XX,
/* PC     */                                                             K2,XX,XX,XX,XX,XX,XX,
/* PC0    */                                                                XX,XX,XX,XX,XX,XX,
/* PA     */                                                                   XX,XX,XX,XX,XX,
/* PBA    */                                                                      XX,XX,XX,XX,
/* B      */                                                                         XX,XX,XX,
/* OS     */                                                                            XX,XX,
/* OW     */                                                                               XX
};

/******************** ARCS ********************/

#define ARCPROTOCOUNT  7
#define AMETAL_1       0		/* Metal_1 */
#define AMETAL_2       1		/* Metal_2 */
#define APOLYSILICON   2		/* Polysilicon */
#define APOLYSILICON_2 3		/* Polysilicon_2 */
#define AACTIVE        4		/* Active */
#define APDIFFA        5		/* Pdiff */
#define ANDIFF         6		/* Ndiff */

static TECH_ARCLAY bicmos_al_0[] = {{LM,0,FILLED}};
static TECH_ARCS bicmos_a_0 = {
	x_("Metal_1"), K3, AMETAL_1, NOARCPROTO,
	1, bicmos_al_0,
	(APMETAL1<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};

static TECH_ARCLAY bicmos_al_1[] = {{LM0,0,FILLED}};
static TECH_ARCS bicmos_a_1 = {
	x_("Metal_2"), K3, AMETAL_2, NOARCPROTO,
	1, bicmos_al_1,
	(APMETAL2<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};

static TECH_ARCLAY bicmos_al_2[] = {{LP0,0,FILLED}};
static TECH_ARCS bicmos_a_2 = {
	x_("Polysilicon"), K2, APOLYSILICON, NOARCPROTO,
	1, bicmos_al_2,
	(APPOLY1<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};

static TECH_ARCLAY bicmos_al_3[] = {{LP1,0,FILLED}};
static TECH_ARCS bicmos_a_3 = {
	x_("Polysilicon_2"), K2, APOLYSILICON_2, NOARCPROTO,
	1, bicmos_al_3,
	(APPOLY2<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};

static TECH_ARCLAY bicmos_al_4[] = {{LA,0,FILLED}};
static TECH_ARCS bicmos_a_4 = {
	x_("Active"), K2, AACTIVE, NOARCPROTO,
	1, bicmos_al_4,
	(APDIFFA<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};

static TECH_ARCLAY bicmos_al_5[] = {{LNW,0,FILLED}, {LA,K10,FILLED}, {LPS,K6,CLOSED}};
static TECH_ARCS bicmos_a_5 = {
	x_("Pdiff"), K12, APDIFFA, NOARCPROTO,
	3, bicmos_al_5,
	(APDIFFP<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};

static TECH_ARCLAY bicmos_al_6[] = {{LA,K4,FILLED}, {LNS,0,FILLED}};
static TECH_ARCS bicmos_a_6 = {
	x_("Ndiff"), K6, ANDIFF, NOARCPROTO,
	2, bicmos_al_6,
	(APDIFFN<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};

TECH_ARCS *bicmos_arcprotos[ARCPROTOCOUNT+1] = {
	&bicmos_a_0, &bicmos_a_1, &bicmos_a_2, &bicmos_a_3, &bicmos_a_4, &bicmos_a_5,
	&bicmos_a_6, ((TECH_ARCS *)-1)};

static INTBIG bicmos_arc_widoff[ARCPROTOCOUNT] = {0, 0, 0, 0, 0, K10, K4};

/******************** PORT CONNECTIONS ********************/

static INTBIG bicmos_pc_1[] = {-1, ANDIFF, ALLGEN, -1};
static INTBIG bicmos_pc_2[] = {-1, APOLYSILICON, APOLYSILICON_2, ALLGEN, -1};
static INTBIG bicmos_pc_3[] = {-1, AMETAL_1, AACTIVE, ALLGEN, -1};
static INTBIG bicmos_pc_4[] = {-1, AMETAL_1, AMETAL_2, ALLGEN, -1};
static INTBIG bicmos_pc_5[] = {-1, APDIFFA, ALLGEN, -1};
static INTBIG bicmos_pc_6[] = {-1, AMETAL_1, APOLYSILICON_2, ALLGEN, -1};
static INTBIG bicmos_pc_7[] = {-1, APOLYSILICON, AMETAL_1, ALLGEN, -1};
static INTBIG bicmos_pc_8[] = {-1, AMETAL_1, ANDIFF, ALLGEN, -1};
static INTBIG bicmos_pc_9[] = {-1, AMETAL_1, APDIFFA, ALLGEN, -1};
static INTBIG bicmos_pc_10[] = {-1, ALLGEN, -1};
static INTBIG bicmos_pc_11[] = {-1, APOLYSILICON_2, ALLGEN, -1};
static INTBIG bicmos_pc_12[] = {-1, APOLYSILICON, ALLGEN, -1};
static INTBIG bicmos_pc_13[] = {-1, AMETAL_2, ALLGEN, -1};
static INTBIG bicmos_pc_14[] = {-1, AMETAL_1, ALLGEN, -1};
static INTBIG bicmos_pc_15[] = {-1, AACTIVE, APDIFFA, ANDIFF, ALLGEN, -1};

/******************** RECTANGLE DESCRIPTIONS ********************/

static INTBIG bicmos_box1[8] = {LEFTIN2, BOTIN2, RIGHTIN2, BOTIN4};
static INTBIG bicmos_box2[8] = {LEFTIN2, TOPIN4, RIGHTIN2, TOPIN2};
static INTBIG bicmos_box3[8] = {LEFTIN2, BOTIN4, RIGHTIN2, TOPIN4};
static INTBIG bicmos_box4[8] = {LEFTEDGE, BOTIN4, RIGHTEDGE, TOPIN4};
static INTBIG bicmos_box5[16] = {LEFTIN4, BOTIN4, RIGHTIN4, TOPIN4, CENTERL2, CENTERD2, CENTERR2, CENTERU2};
static INTBIG bicmos_box6[8] = {CENTERL1, CENTERD1, CENTERR1, CENTERU1};
static INTBIG bicmos_box7[8] = {LEFTIN5, BOTIN5, RIGHTIN5, BOTIN7};
static INTBIG bicmos_box8[8] = {LEFTIN5, TOPIN7, RIGHTIN5, TOPIN5};
static INTBIG bicmos_box9[8] = {LEFTIN3, BOTIN7, RIGHTIN3, TOPIN7};
static INTBIG bicmos_box10[8] = {LEFTIN5, BOTIN7, RIGHTIN5, TOPIN7};
static INTBIG bicmos_box11[16] = {LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1, CENTERL2, CENTERD2, CENTERR2, CENTERU2};
static INTBIG bicmos_box12[16] = {LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3, CENTERL2, CENTERD2, CENTERR2, CENTERU2};
static INTBIG bicmos_box13[8] = {LEFTIN1, BOTIN1, LEFTIN3, BOTIN3};
static INTBIG bicmos_box14[16] = {LEFTIN6, BOTIN6, RIGHTIN6, TOPIN6, CENTERL2, CENTERD2, CENTERR2, CENTERU2};
static INTBIG bicmos_box15[8] = {LEFTIN5, BOTIN5, RIGHTIN5, TOPIN5};
static INTBIG bicmos_box16[8] = {-H0,4200, BOTIN10, RIGHTIN7, H0,-K22};
static INTBIG bicmos_box17[8] = {-H0,4080, BOTIN9, RIGHTIN6, H0,-K21};
static INTBIG bicmos_box18[8] = {-H0,4320, BOTIN11, RIGHTIN8, H0,-K23};
static INTBIG bicmos_box19[8] = {LEFTIN2, -H0,K21, H0,-4560, TOPIN9};
static INTBIG bicmos_box20[8] = {LEFTEDGE, BOTIN19, H0,-4320, TOPIN7};
static INTBIG bicmos_box21[8] = {LEFTEDGE, BOTIN7, H0,-4320, TOPIN19};
static INTBIG bicmos_box22[8] = {LEFTIN2, BOTIN9, H0,-4560, H0,-K21};
static INTBIG bicmos_box23[8] = {-H0,K24, BOTIN10, RIGHTIN18, H0,-K22};
static INTBIG bicmos_box24[8] = {-H0,4200, -H0,K22, RIGHTIN7, TOPIN10};
static INTBIG bicmos_box25[8] = {-H0,K24, -H0,K22, RIGHTIN18, TOPIN10};
static INTBIG bicmos_box26[8] = {-H0,K22, -H0,K20, RIGHTIN16, TOPIN8};
static INTBIG bicmos_box27[8] = {-H0,4080, -H0,K21, RIGHTIN6, TOPIN9};
static INTBIG bicmos_box28[8] = {LEFTIN13, BOTIN10, H0,-K29, H0,-K22};
static INTBIG bicmos_box29[8] = {LEFTIN4, BOTIN11, H0,-4800, H0,-K23};
static INTBIG bicmos_box30[8] = {LEFTIN3, BOTIN10, H0,-4680, H0,-K22};
static INTBIG bicmos_box31[8] = {LEFTIN12, BOTIN9, H0,-K28, H0,-K21};
static INTBIG bicmos_box32[8] = {LEFTIN3, -H0,K22, H0,-4680, TOPIN10};
static INTBIG bicmos_box33[8] = {LEFTIN4, -H0,K23, H0,-4800, TOPIN11};
static INTBIG bicmos_box34[8] = {LEFTIN14, BOTIN11, H0,-K30, H0,-K23};
static INTBIG bicmos_box35[8] = {LEFTIN13, -H0,K22, H0,-K29, TOPIN10};
static INTBIG bicmos_box36[8] = {LEFTIN12, -H0,K21, H0,-K28, TOPIN9};
static INTBIG bicmos_box37[8] = {-H0,K22, BOTIN8, RIGHTIN16, H0,-K20};
static INTBIG bicmos_box38[8] = {-H0,4320, -H0,K23, RIGHTIN8, TOPIN11};
static INTBIG bicmos_box39[8] = {LEFTIN14, -H0,K23, H0,-K30, TOPIN11};
static INTBIG bicmos_box40[8] = {-H0,K25, -H0,K23, RIGHTIN19, TOPIN11};
static INTBIG bicmos_box41[8] = {-H0,K25, BOTIN11, RIGHTIN19, H0,-K23};
static INTBIG bicmos_box42[8] = {LEFTIN3, BOTIN10, H0,-4680, TOPIN10};
static INTBIG bicmos_box43[8] = {LEFTIN2, BOTIN9, H0,-4560, TOPIN9};
static INTBIG bicmos_box44[8] = {LEFTIN2, BOTEDGE, RIGHTEDGE, TOPEDGE};
static INTBIG bicmos_box45[8] = {LEFTEDGE, BOTIN7, H0,-4320, TOPIN7};
static INTBIG bicmos_box46[8] = {LEFTIN4, BOTIN11, H0,-4800, TOPIN11};
static INTBIG bicmos_box47[8] = {-H0,4200, BOTIN10, RIGHTIN7, TOPIN10};
static INTBIG bicmos_box48[8] = {-H0,4080, BOTIN9, RIGHTIN6, TOPIN9};
static INTBIG bicmos_box49[8] = {LEFTIN12, BOTIN6, RIGHTIN6, TOPIN6};
static INTBIG bicmos_box50[8] = {LEFTIN13, BOTIN10, H0,-K29, TOPIN10};
static INTBIG bicmos_box51[8] = {-H0,K24, BOTIN10, RIGHTIN18, TOPIN10};
static INTBIG bicmos_box52[8] = {-H0,K22, BOTIN8, RIGHTIN16, TOPIN8};
static INTBIG bicmos_box53[8] = {-H0,4320, BOTIN11, RIGHTIN8, TOPIN11};
static INTBIG bicmos_box54[8] = {-H0,K25, BOTIN11, RIGHTIN19, TOPIN11};
static INTBIG bicmos_box55[8] = {LEFTIN14, BOTIN11, H0,-K30, TOPIN11};
static INTBIG bicmos_box56[8] = {LEFTIN12, BOTIN9, H0,-K28, TOPIN9};
static INTBIG bicmos_box57[8] = {LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3};
static INTBIG bicmos_box58[8] = {LEFTIN2, BOTIN2, RIGHTIN2, TOPIN2};
static INTBIG bicmos_box59[8] = {LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1};
static INTBIG bicmos_box60[8] = {LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE};

/******************** NODES ********************/

#define NODEPROTOCOUNT 39
#define NAP            1		/* Active_Pin */
#define NMP            2		/* M1_Pin */
#define NMP0           3		/* M2_Pin */
#define NPP            4		/* Poly1_Pin */
#define NPP0           5		/* Poly2_Pin */
#define NNP            6		/* Ndiff_Pin */
#define NPP1           7		/* Pdiff_Pin */
#define NNT            8		/* NPN1_transistor */
#define NNT0           9		/* NPN2_Transistor */
#define NMPC           10		/* M1_Pdiff_Con */
#define NMNC           11		/* M1_Ndiff_Con */
#define NMPC0          12		/* M1_Poly1_Con */
#define NMPC1          13		/* M1_Poly2_Con */
#define NP             14		/* PMOSFET */
#define NMMC           15		/* M1_M2_Con */
#define NMNWC          16		/* M1_N_Well_Con */
#define NPPC           17		/* Poly1_Poly2_Cap */
#define NN             18		/* NMOSFET */
#define NMSC           19		/* M1_Substrate_Con */
#define NATN           20		/* Active_Node */
#define NPSN           21		/* P_Select_Node */
#define NPCN           22		/* Poly_2_Cut_Node */
#define NACN           23		/* Active_Cut_Node */
#define NVN            24		/* Via_Node */
#define NPN            25		/* Passivation_Node */
#define NPFN           26		/* Pad_Frame_Node */
#define NMN            27		/* M1_Node */
#define NMN0           28		/* M2_Node */
#define NPN0           29		/* Poly1_Node */
#define NPN1           30		/* Poly2_Node */
#define NNN            31		/* Ndiff_Node */
#define NPCN0          32		/* Poly1_Cut_Node */
#define NNWN           33		/* N_Well_Node */
#define NNSN           34		/* N_Select_Node */
#define NPBAN          35		/* P_Base_Active_Node */
#define NBN            36		/* BCCD_Node */
#define NPN2           37		/* Pdiff_Node */
#define NOW            38		/* Ohmic_Well */
#define NOS            39		/* Ohmic_Substrate */

/* Active_Pin */
static TECH_PORTS bicmos_ap_p[] = {
	{bicmos_pc_15, x_("active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON bicmos_ap_l[] = {
	{LPA, 0, 4, CROSSED, BOX, bicmos_box60}};
static TECH_NODES bicmos_ap = {
	x_("Active_Pin"), NAP, NONODEPROTO,
	K2, K2,
	1, bicmos_ap_p,
	1, bicmos_ap_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* M1_Pin */
static TECH_PORTS bicmos_mp_p[] = {
	{bicmos_pc_14, x_("metal-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON bicmos_mp_l[] = {
	{LPM, 0, 4, CROSSED, BOX, bicmos_box60}};
static TECH_NODES bicmos_mp = {
	x_("M1_Pin"), NMP, NONODEPROTO,
	K4, K4,
	1, bicmos_mp_p,
	1, bicmos_mp_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* M2_Pin */
static TECH_PORTS bicmos_mp0_p[] = {
	{bicmos_pc_13, x_("metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON bicmos_mp0_l[] = {
	{LPM0, 0, 4, CROSSED, BOX, bicmos_box60}};
static TECH_NODES bicmos_mp0 = {
	x_("M2_Pin"), NMP0, NONODEPROTO,
	K4, K4,
	1, bicmos_mp0_p,
	1, bicmos_mp0_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* Poly1_Pin */
static TECH_PORTS bicmos_pp_p[] = {
	{bicmos_pc_12, x_("polysilicon"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON bicmos_pp_l[] = {
	{LPP, 0, 4, CROSSED, BOX, bicmos_box60}};
static TECH_NODES bicmos_pp = {
	x_("Poly1_Pin"), NPP, NONODEPROTO,
	K2, K2,
	1, bicmos_pp_p,
	1, bicmos_pp_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* Poly2_Pin */
static TECH_PORTS bicmos_pp0_p[] = {
	{bicmos_pc_11, x_("p2-pin"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON bicmos_pp0_l[] = {
	{LPP0, 0, 4, CROSSED, BOX, bicmos_box60}};
static TECH_NODES bicmos_pp0 = {
	x_("Poly2_Pin"), NPP0, NONODEPROTO,
	K2, K2,
	1, bicmos_pp0_p,
	1, bicmos_pp0_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* Ndiff_Pin */
static TECH_PORTS bicmos_np_p[] = {
	{bicmos_pc_10, x_("Ndiff_Pin"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3}};
static TECH_POLYGON bicmos_np_l[] = {
	{LPA, 0, 4, CLOSEDRECT, BOX, bicmos_box58},
	{LPNS, 0, 4, CROSSED, BOX, bicmos_box60}};
static TECH_NODES bicmos_np = {
	x_("Ndiff_Pin"), NNP, NONODEPROTO,
	K8, K8,
	1, bicmos_np_p,
	2, bicmos_np_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* Pdiff_Pin */
static TECH_PORTS bicmos_pp1_p[] = {
	{bicmos_pc_10, x_("Pdiff_Pin"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3}};
static TECH_POLYGON bicmos_pp1_l[] = {
	{LPA, 0, 4, CLOSEDRECT, BOX, bicmos_box58},
	{LPPS, 0, 4, CROSSED, BOX, bicmos_box60}};
static TECH_NODES bicmos_pp1 = {
	x_("Pdiff_Pin"), NPP1, NONODEPROTO,
	K8, K8,
	1, bicmos_pp1_p,
	2, bicmos_pp1_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* NPN1_transistor */
static TECH_PORTS bicmos_nt_p[] = {
	{bicmos_pc_14, x_("E1"), NOPORTPROTO, (180<<PORTARANGESH),
		-H0,K25, BOTIN11, RIGHTIN19, TOPIN11},
	{bicmos_pc_14, x_("B1"), NOPORTPROTO, (180<<PORTARANGESH)|(1<<PORTNETSH),
		LEFTIN14, BOTIN11, H0,-K30, TOPIN11},
	{bicmos_pc_14, x_("B2"), NOPORTPROTO, (180<<PORTARANGESH)|(2<<PORTNETSH),
		-H0,4320, BOTIN11, RIGHTIN8, TOPIN11},
	{bicmos_pc_14, x_("C1"), NOPORTPROTO, (180<<PORTARANGESH)|(3<<PORTNETSH),
		LEFTIN4, BOTIN11, H0,-4800, TOPIN11}};
static TECH_POLYGON bicmos_nt_l[] = {
	{LM, 0, 4, FILLEDRECT, BOX, bicmos_box51},
	{LA, 1, 4, FILLEDRECT, BOX, bicmos_box56},
	{LM, 1, 4, FILLEDRECT, BOX, bicmos_box50},
	{LPBA, -1, 4, FILLEDRECT, BOX, bicmos_box49},
	{LA, 0, 4, FILLEDRECT, BOX, bicmos_box52},
	{LA, 2, 4, FILLEDRECT, BOX, bicmos_box48},
	{LM, 2, 4, FILLEDRECT, BOX, bicmos_box47},
	{LNW, -1, 4, FILLEDRECT, BOX, bicmos_box44},
	{LA, 3, 4, FILLEDRECT, BOX, bicmos_box43},
	{LM, 3, 4, FILLEDRECT, BOX, bicmos_box42},
	{LPS, 1, 4, FILLEDRECT, BOX, bicmos_box56},
	{LAC, 1, 4, FILLEDRECT, BOX, bicmos_box55},
	{LNS, 0, 4, FILLEDRECT, BOX, bicmos_box52},
	{LAC, 0, 4, FILLEDRECT, BOX, bicmos_box54},
	{LPS, 2, 4, FILLEDRECT, BOX, bicmos_box48},
	{LAC, 2, 4, FILLEDRECT, BOX, bicmos_box53},
	{LAC, 3, 4, FILLEDRECT, BOX, bicmos_box46},
	{LNS, 3, 4, CLOSEDRECT, BOX, bicmos_box45}};
static TECH_NODES bicmos_nt = {
	x_("NPN1_transistor"), NNT, NONODEPROTO,
	5520, K24,
	4, bicmos_nt_p,
	18, bicmos_nt_l,
	(NPTRANPN<<NFUNCTIONSH),
	0,0,0,0,0,0,0,0,0};

/* NPN2_Transistor */
static TECH_PORTS bicmos_nt0_p[] = {
	{bicmos_pc_14, x_("E1"), NOPORTPROTO, (180<<PORTARANGESH),
		-H0,K25, BOTIN11, RIGHTIN19, H0,-K23},
	{bicmos_pc_14, x_("E2"), NOPORTPROTO, (180<<PORTARANGESH)|(1<<PORTNETSH),
		-H0,K25, -H0,K23, RIGHTIN19, TOPIN11},
	{bicmos_pc_14, x_("B3"), NOPORTPROTO, (180<<PORTARANGESH)|(2<<PORTNETSH),
		LEFTIN14, -H0,K23, H0,-K30, TOPIN11},
	{bicmos_pc_14, x_("B4"), NOPORTPROTO, (180<<PORTARANGESH)|(3<<PORTNETSH),
		-H0,4320, -H0,K23, RIGHTIN8, TOPIN11},
	{bicmos_pc_14, x_("B1"), NOPORTPROTO, (180<<PORTARANGESH)|(4<<PORTNETSH),
		LEFTIN14, BOTIN11, H0,-K30, H0,-K23},
	{bicmos_pc_14, x_("C2"), NOPORTPROTO, (180<<PORTARANGESH)|(5<<PORTNETSH),
		LEFTIN4, -H0,K23, H0,-4800, TOPIN11},
	{bicmos_pc_14, x_("C1"), NOPORTPROTO, (180<<PORTARANGESH)|(6<<PORTNETSH),
		LEFTIN4, BOTIN11, H0,-4800, H0,-K23},
	{bicmos_pc_14, x_("B2"), NOPORTPROTO, (180<<PORTARANGESH)|(7<<PORTNETSH),
		-H0,4320, BOTIN11, RIGHTIN8, H0,-K23}};
static TECH_POLYGON bicmos_nt0_l[] = {
	{LA, 0, 4, FILLEDRECT, BOX, bicmos_box37},
	{LA, 2, 4, FILLEDRECT, BOX, bicmos_box36},
	{LM, 2, 4, FILLEDRECT, BOX, bicmos_box35},
	{LM, 5, 4, FILLEDRECT, BOX, bicmos_box32},
	{LM, 6, 4, FILLEDRECT, BOX, bicmos_box30},
	{LM, 4, 4, FILLEDRECT, BOX, bicmos_box28},
	{LA, 4, 4, FILLEDRECT, BOX, bicmos_box31},
	{LA, 1, 4, FILLEDRECT, BOX, bicmos_box26},
	{LM, 1, 4, FILLEDRECT, BOX, bicmos_box25},
	{LA, 3, 4, FILLEDRECT, BOX, bicmos_box27},
	{LM, 3, 4, FILLEDRECT, BOX, bicmos_box24},
	{LM, 0, 4, FILLEDRECT, BOX, bicmos_box23},
	{LA, 6, 4, FILLEDRECT, BOX, bicmos_box22},
	{LA, 5, 4, FILLEDRECT, BOX, bicmos_box19},
	{LPBA, -1, 4, FILLEDRECT, BOX, bicmos_box49},
	{LA, 7, 4, FILLEDRECT, BOX, bicmos_box17},
	{LM, 7, 4, FILLEDRECT, BOX, bicmos_box16},
	{LNW, -1, 4, FILLEDRECT, BOX, bicmos_box44},
	{LNS, 0, 4, FILLEDRECT, BOX, bicmos_box37},
	{LAC, 2, 4, FILLEDRECT, BOX, bicmos_box39},
	{LAC, 5, 4, FILLEDRECT, BOX, bicmos_box33},
	{LPS, 4, 4, FILLEDRECT, BOX, bicmos_box31},
	{LAC, 6, 4, FILLEDRECT, BOX, bicmos_box29},
	{LAC, 0, 4, FILLEDRECT, BOX, bicmos_box41},
	{LAC, 4, 4, FILLEDRECT, BOX, bicmos_box34},
	{LAC, 3, 4, FILLEDRECT, BOX, bicmos_box38},
	{LPS, 3, 4, FILLEDRECT, BOX, bicmos_box27},
	{LAC, 1, 4, FILLEDRECT, BOX, bicmos_box40},
	{LNS, 1, 4, FILLEDRECT, BOX, bicmos_box26},
	{LNS, 6, 4, FILLEDRECT, BOX, bicmos_box21},
	{LNS, 5, 4, FILLEDRECT, BOX, bicmos_box20},
	{LPS, 2, 4, FILLEDRECT, BOX, bicmos_box36},
	{LAC, 7, 4, FILLEDRECT, BOX, bicmos_box18},
	{LPS, 7, 4, FILLEDRECT, BOX, bicmos_box17}};
static TECH_NODES bicmos_nt0 = {
	x_("NPN2_Transistor"), NNT0, NONODEPROTO,
	5520, 4320,
	8, bicmos_nt0_p,
	34, bicmos_nt0_l,
	(NPTRANPN<<NFUNCTIONSH),
	0,0,0,0,0,0,0,0,0};

/* M1_Pdiff_Con */
static TECH_PORTS bicmos_mpc_p[] = {
	{bicmos_pc_9, x_("m1_pdiff"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN6, BOTIN6, RIGHTIN6, TOPIN6}};
static TECH_POLYGON bicmos_mpc_l[] = {
	{LA, 0, 4, FILLEDRECT, BOX, bicmos_box15},
	{LM, 0, 4, FILLEDRECT, MINBOX, bicmos_box14},
	{LNW, 0, 4, FILLEDRECT, BOX, bicmos_box60},
	{LPS, 0, 4, CLOSEDRECT, BOX, bicmos_box57},
	{LAC, 0, 4, FILLEDRECT, BOX, bicmos_box13}};
static TECH_NODES bicmos_mpc = {
	x_("M1_Pdiff_Con"), NMPC, NONODEPROTO,
	K16, K16,
	1, bicmos_mpc_p,
	5, bicmos_mpc_l,
	(NPCONTACT<<NFUNCTIONSH),
	MULTICUT,K2,K2,K2,K2,0,0,0,0};

/* M1_Ndiff_Con */
static TECH_PORTS bicmos_mnc_p[] = {
	{bicmos_pc_8, x_("M1_Ndiff"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3}};
static TECH_POLYGON bicmos_mnc_l[] = {
	{LA, 0, 4, FILLEDRECT, BOX, bicmos_box58},
	{LM, 0, 4, FILLEDRECT, MINBOX, bicmos_box12},
	{LNS, 0, 4, FILLEDRECT, BOX, bicmos_box60},
	{LAC, 0, 4, FILLEDRECT, BOX, bicmos_box13}};
static TECH_NODES bicmos_mnc = {
	x_("M1_Ndiff_Con"), NMNC, NONODEPROTO,
	K10, K10,
	1, bicmos_mnc_p,
	4, bicmos_mnc_l,
	(NPCONTACT<<NFUNCTIONSH),
	MULTICUT,K2,K2,K2,K2,0,0,0,0};

/* M1_Poly1_Con */
static TECH_PORTS bicmos_mpc0_p[] = {
	{bicmos_pc_7, x_("metal-1-polysilicon"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON bicmos_mpc0_l[] = {
	{LM, 0, 4, FILLEDRECT, MINBOX, bicmos_box11},
	{LP0, 0, 4, FILLEDRECT, BOX, bicmos_box60},
	{LPC, 0, 4, FILLEDRECT, BOX, bicmos_box13}};
static TECH_NODES bicmos_mpc0 = {
	x_("M1_Poly1_Con"), NMPC0, NONODEPROTO,
	K6, K6,
	1, bicmos_mpc0_p,
	3, bicmos_mpc0_l,
	(NPCONTACT<<NFUNCTIONSH),
	MULTICUT,K2,K2,K2,K2,0,0,0,0};

/* M1_Poly2_Con */
static TECH_PORTS bicmos_mpc1_p[] = {
	{bicmos_pc_6, x_("M1P2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON bicmos_mpc1_l[] = {
	{LM, 0, 4, FILLEDRECT, BOX, bicmos_box59},
	{LP1, 0, 4, FILLEDRECT, BOX, bicmos_box60},
	{LPC0, 0, 4, FILLEDRECT, BOX, bicmos_box13}};
static TECH_NODES bicmos_mpc1 = {
	x_("M1_Poly2_Con"), NMPC1, NONODEPROTO,
	K6, K6,
	1, bicmos_mpc1_p,
	3, bicmos_mpc1_l,
	(NPCONTACT<<NFUNCTIONSH),
	MULTICUT,K2,K2,K1,K2,0,0,0,0};

/* PMOSFET */
static TECH_PORTS bicmos_p_p[] = {
	{bicmos_pc_12, x_("pmos_poly_lt"), NOPORTPROTO, (90<<PORTARANGESH)|(180<<PORTANGLESH)|(2<<PORTNETSH),
		LEFTIN3, CENTERD1, LEFTIN4, CENTERU1},
	{bicmos_pc_5, x_("pmos_diff_bot"), NOPORTPROTO, (90<<PORTARANGESH)|(270<<PORTANGLESH)|(1<<PORTNETSH),
		CENTERL1, BOTIN5, CENTERR1, BOTIN6},
	{bicmos_pc_12, x_("pmos_poly_rt"), NOPORTPROTO, (90<<PORTARANGESH)|(2<<PORTNETSH),
		RIGHTIN4, CENTERD1, RIGHTIN3, CENTERU1},
	{bicmos_pc_5, x_("pmos_diff_top"), NOPORTPROTO, (90<<PORTARANGESH)|(90<<PORTANGLESH),
		CENTERL1, TOPIN6, CENTERR1, TOPIN5}};
static TECH_SERPENT bicmos_p_l[] = {
	{{LP0, 2, 4, FILLEDRECT, BOX, bicmos_box9}, K1, K1, K2, K2},
	{{LNW, -1, 4, FILLEDRECT, BOX, bicmos_box60}, K8, K8, K5, K5},
	{{LA, 0, 4, FILLEDRECT, BOX, bicmos_box15}, K3, K3, K0, K0},
	{{LPS, -1, 4, FILLEDRECT, BOX, bicmos_box57}, K5, K5, K2, K2}};
static TECH_SERPENT bicmos_pE_l[] = {
	{{LP0, 2, 4, FILLEDRECT, BOX, bicmos_box9}, K1, K1, K2, K2},
	{{LNW, -1, 4, FILLEDRECT, BOX, bicmos_box60}, K8, K8, K5, K5},
	{{LA, 3, 4, FILLEDRECT, BOX, bicmos_box8}, K3, -K1, K0, K0},
	{{LA, -1, 4, FILLEDRECT, BOX, bicmos_box10}, K1, K1, K0, K0},
	{{LA, 1, 4, FILLEDRECT, BOX, bicmos_box7}, -K1, K3, K0, K0},
	{{LPS, -1, 4, FILLEDRECT, BOX, bicmos_box57}, K5, K5, K2, K2}};
static TECH_NODES bicmos_p = {
	x_("PMOSFET"), NP, NONODEPROTO,
	K12, K16,
	4, bicmos_p_p,
	4, (TECH_POLYGON *)0,
	(NPTRAPMOS<<NFUNCTIONSH)|HOLDSTRACE|NODESHRINK,
	SERPTRANS,6,0,K1,K2,0,K1,bicmos_p_l,bicmos_pE_l};

/* M1_M2_Con */
static TECH_PORTS bicmos_mmc_p[] = {
	{bicmos_pc_4, x_("metal-1-metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON bicmos_mmc_l[] = {
	{LM, 0, 4, FILLEDRECT, BOX, bicmos_box60},
	{LM0, 0, 4, FILLEDRECT, BOX, bicmos_box60},
	{LV, 0, 4, CLOSEDRECT, BOX, bicmos_box6}};
static TECH_NODES bicmos_mmc = {
	x_("M1_M2_Con"), NMMC, NONODEPROTO,
	K4, K4,
	1, bicmos_mmc_p,
	3, bicmos_mmc_l,
	(NPCONTACT<<NFUNCTIONSH),
	0,0,0,0,0,0,0,0,0};

/* M1_N_Well_Con */
static TECH_PORTS bicmos_mnwc_p[] = {
	{bicmos_pc_3, x_("metal-1-well"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN4, BOTIN4, RIGHTIN4, TOPIN4}};
static TECH_POLYGON bicmos_mnwc_l[] = {
	{LOW, 0, 4, FILLEDRECT, BOX, bicmos_box57},
	{LM, 0, 4, FILLEDRECT, MINBOX, bicmos_box5},
	{LNW, 0, 4, FILLEDRECT, BOX, bicmos_box60},
	{LNS, 0, 4, FILLEDRECT, BOX, bicmos_box59},
	{LAC, 0, 4, FILLEDRECT, BOX, bicmos_box13}};
static TECH_NODES bicmos_mnwc = {
	x_("M1_N_Well_Con"), NMNWC, NONODEPROTO,
	K12, K12,
	1, bicmos_mnwc_p,
	5, bicmos_mnwc_l,
	(NPWELL<<NFUNCTIONSH),
	MULTICUT,K2,K2,K2,K2,0,0,0,0};

/* Poly1_Poly2_Cap */
static TECH_PORTS bicmos_ppc_p[] = {
	{bicmos_pc_2, x_("P1P2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3}};
static TECH_POLYGON bicmos_ppc_l[] = {
	{LP0, 0, 4, FILLEDRECT, BOX, bicmos_box60},
	{LP1, 0, 4, FILLEDRECT, BOX, bicmos_box58}};
static TECH_NODES bicmos_ppc = {
	x_("Poly1_Poly2_Cap"), NPPC, NONODEPROTO,
	K12, K12,
	1, bicmos_ppc_p,
	2, bicmos_ppc_l,
	(NPCAPAC<<NFUNCTIONSH),
	0,0,0,0,0,0,0,0,0};

/* NMOSFET */
static TECH_PORTS bicmos_n_p[] = {
	{bicmos_pc_12, x_("nmos_poly_rt"), NOPORTPROTO, (90<<PORTARANGESH)|(1<<PORTNETSH),
		RIGHTIN1, CENTERD1, RIGHTEDGE, CENTERU1},
	{bicmos_pc_1, x_("nmos_diff_top"), NOPORTPROTO, (90<<PORTARANGESH)|(90<<PORTANGLESH),
		CENTERL1, TOPIN3, CENTERR1, TOPIN2},
	{bicmos_pc_12, x_("nmos_poly_lt"), NOPORTPROTO, (90<<PORTARANGESH)|(180<<PORTANGLESH)|(1<<PORTNETSH),
		LEFTEDGE, CENTERD1, LEFTIN1, CENTERU1},
	{bicmos_pc_1, x_("nmos_diff_bot"), NOPORTPROTO, (90<<PORTARANGESH)|(270<<PORTANGLESH)|(3<<PORTNETSH),
		CENTERL1, BOTIN2, CENTERR1, BOTIN3}};
static TECH_SERPENT bicmos_n_l[] = {
	{{LP0, 0, 4, FILLEDRECT, BOX, bicmos_box4}, K1, K1, K2, K2},
	{{LA, 0, 4, FILLEDRECT, BOX, bicmos_box58}, K3, K3, K0, K0},
	{{LNS, -1, 4, FILLEDRECT, BOX, bicmos_box60}, K5, K5, K2, K2}};
static TECH_SERPENT bicmos_nE_l[] = {
	{{LP0, 0, 4, FILLEDRECT, BOX, bicmos_box4}, K1, K1, K2, K2},
	{{LA, 1, 4, FILLEDRECT, BOX, bicmos_box2}, K3, -K1, K0, K0},
	{{LA, -1, 4, FILLEDRECT, BOX, bicmos_box3}, K1, K1, K0, K0},
	{{LA, 3, 4, FILLEDRECT, BOX, bicmos_box1}, -K1, K3, K0, K0},
	{{LNS, -1, 4, FILLEDRECT, BOX, bicmos_box60}, K5, K5, K2, K2}};
static TECH_NODES bicmos_n = {
	x_("NMOSFET"), NN, NONODEPROTO,
	K6, K10,
	4, bicmos_n_p,
	3, (TECH_POLYGON *)0,
	(NPTRANMOS<<NFUNCTIONSH)|HOLDSTRACE|NODESHRINK,
	SERPTRANS,5,0,K1,K2,0,K1,bicmos_n_l,bicmos_nE_l};

/* M1_Substrate_Con */
static TECH_PORTS bicmos_msc_p[] = {
	{bicmos_pc_14, x_("M1_Substrate"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN4, BOTIN4, RIGHTIN4, TOPIN4}};
static TECH_POLYGON bicmos_msc_l[] = {
	{LM, 0, 4, FILLEDRECT, BOX, bicmos_box57},
	{LOS, 0, 4, FILLEDRECT, BOX, bicmos_box58},
	{LPS, 0, 4, FILLEDRECT, BOX, bicmos_box60},
	{LAC, 0, 4, FILLEDRECT, BOX, bicmos_box13}};
static TECH_NODES bicmos_msc = {
	x_("M1_Substrate_Con"), NMSC, NONODEPROTO,
	K10, K10,
	1, bicmos_msc_p,
	4, bicmos_msc_l,
	(NPUNKNOWN<<NFUNCTIONSH),
	MULTICUT,K2,K2,K1,K2,0,0,0,0};

/* Active_Node */
static TECH_PORTS bicmos_an_p[] = {
	{bicmos_pc_15, x_("active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON bicmos_an_l[] = {
	{LA, 0, 4, FILLEDRECT, BOX, bicmos_box60}};
static TECH_NODES bicmos_an = {
	x_("Active_Node"), NATN, NONODEPROTO,
	K4, K4,
	1, bicmos_an_p,
	1, bicmos_an_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* P_Select_Node */
static TECH_PORTS bicmos_psn_p[] = {
	{bicmos_pc_10, x_("select"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON bicmos_psn_l[] = {
	{LPS, 0, 4, CLOSEDRECT, BOX, bicmos_box60}};
static TECH_NODES bicmos_psn = {
	x_("P_Select_Node"), NPSN, NONODEPROTO,
	K6, K6,
	1, bicmos_psn_p,
	1, bicmos_psn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Poly_2_Cut_Node */
static TECH_PORTS bicmos_pcn_p[] = {
	{bicmos_pc_11, x_("Poly_2_Cut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON bicmos_pcn_l[] = {
	{LPC0, 0, 4, FILLEDRECT, BOX, bicmos_box60}};
static TECH_NODES bicmos_pcn = {
	x_("Poly_2_Cut_Node"), NPCN, NONODEPROTO,
	K4, K4,
	1, bicmos_pcn_p,
	1, bicmos_pcn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Active_Cut_Node */
static TECH_PORTS bicmos_acn_p[] = {
	{bicmos_pc_10, x_("activecut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON bicmos_acn_l[] = {
	{LAC, 0, 4, FILLEDRECT, BOX, bicmos_box60}};
static TECH_NODES bicmos_acn = {
	x_("Active_Cut_Node"), NACN, NONODEPROTO,
	K2, K2,
	1, bicmos_acn_p,
	1, bicmos_acn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Via_Node */
static TECH_PORTS bicmos_vn_p[] = {
	{bicmos_pc_10, x_("via"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON bicmos_vn_l[] = {
	{LV, 0, 4, CLOSEDRECT, BOX, bicmos_box60}};
static TECH_NODES bicmos_vn = {
	x_("Via_Node"), NVN, NONODEPROTO,
	K2, K2,
	1, bicmos_vn_p,
	1, bicmos_vn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Passivation_Node */
static TECH_PORTS bicmos_pn_p[] = {
	{bicmos_pc_10, x_("passivation"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON bicmos_pn_l[] = {
	{LP, 0, 4, CLOSEDRECT, BOX, bicmos_box60}};
static TECH_NODES bicmos_pn = {
	x_("Passivation_Node"), NPN, NONODEPROTO,
	K8, K8,
	1, bicmos_pn_p,
	1, bicmos_pn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Pad_Frame_Node */
static TECH_PORTS bicmos_pfn_p[] = {
	{bicmos_pc_10, x_("pad-frame"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON bicmos_pfn_l[] = {
	{LPF, 0, 4, CLOSEDRECT, BOX, bicmos_box60}};
static TECH_NODES bicmos_pfn = {
	x_("Pad_Frame_Node"), NPFN, NONODEPROTO,
	K8, K8,
	1, bicmos_pfn_p,
	1, bicmos_pfn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* M1_Node */
static TECH_PORTS bicmos_mn_p[] = {
	{bicmos_pc_14, x_("metal-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON bicmos_mn_l[] = {
	{LM, 0, 4, FILLEDRECT, BOX, bicmos_box60}};
static TECH_NODES bicmos_mn = {
	x_("M1_Node"), NMN, NONODEPROTO,
	K4, K4,
	1, bicmos_mn_p,
	1, bicmos_mn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* M2_Node */
static TECH_PORTS bicmos_mn0_p[] = {
	{bicmos_pc_13, x_("metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON bicmos_mn0_l[] = {
	{LM0, 0, 4, FILLEDRECT, BOX, bicmos_box60}};
static TECH_NODES bicmos_mn0 = {
	x_("M2_Node"), NMN0, NONODEPROTO,
	K4, K4,
	1, bicmos_mn0_p,
	1, bicmos_mn0_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Poly1_Node */
static TECH_PORTS bicmos_pn0_p[] = {
	{bicmos_pc_12, x_("polysilicon"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON bicmos_pn0_l[] = {
	{LP0, 0, 4, FILLEDRECT, BOX, bicmos_box60}};
static TECH_NODES bicmos_pn0 = {
	x_("Poly1_Node"), NPN0, NONODEPROTO,
	K4, K4,
	1, bicmos_pn0_p,
	1, bicmos_pn0_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Poly2_Node */
static TECH_PORTS bicmos_pn1_p[] = {
	{bicmos_pc_11, x_("P2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON bicmos_pn1_l[] = {
	{LP1, 0, 4, FILLEDRECT, BOX, bicmos_box60}};
static TECH_NODES bicmos_pn1 = {
	x_("Poly2_Node"), NPN1, NONODEPROTO,
	K2, K2,
	1, bicmos_pn1_p,
	1, bicmos_pn1_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Ndiff_Node */
static TECH_PORTS bicmos_nn_p[] = {
	{bicmos_pc_15, x_("active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3}};
static TECH_POLYGON bicmos_nn_l[] = {
	{LA, 0, 4, FILLEDRECT, BOX, bicmos_box58},
	{LNS, 0, 4, FILLEDRECT, BOX, bicmos_box60}};
static TECH_NODES bicmos_nn = {
	x_("Ndiff_Node"), NNN, NONODEPROTO,
	K8, K8,
	1, bicmos_nn_p,
	2, bicmos_nn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Poly1_Cut_Node */
static TECH_PORTS bicmos_pcn0_p[] = {
	{bicmos_pc_10, x_("polycut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON bicmos_pcn0_l[] = {
	{LPC, 0, 4, FILLEDRECT, BOX, bicmos_box60}};
static TECH_NODES bicmos_pcn0 = {
	x_("Poly1_Cut_Node"), NPCN0, NONODEPROTO,
	K2, K2,
	1, bicmos_pcn0_p,
	1, bicmos_pcn0_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* N_Well_Node */
static TECH_PORTS bicmos_nwn_p[] = {
	{bicmos_pc_5, x_("well"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3}};
static TECH_POLYGON bicmos_nwn_l[] = {
	{LNW, 0, 4, FILLEDRECT, BOX, bicmos_box60}};
static TECH_NODES bicmos_nwn = {
	x_("N_Well_Node"), NNWN, NONODEPROTO,
	K6, K6,
	1, bicmos_nwn_p,
	1, bicmos_nwn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* N_Select_Node */
static TECH_PORTS bicmos_nsn_p[] = {
	{bicmos_pc_10, x_("N_Select"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON bicmos_nsn_l[] = {
	{LNS, 0, 4, CLOSEDRECT, BOX, bicmos_box60}};
static TECH_NODES bicmos_nsn = {
	x_("N_Select_Node"), NNSN, NONODEPROTO,
	K4, K4,
	1, bicmos_nsn_p,
	1, bicmos_nsn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* P_Base_Active_Node */
static TECH_PORTS bicmos_pban_p[] = {
	{bicmos_pc_10, x_("P_Base"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON bicmos_pban_l[] = {
	{LPBA, 0, 4, FILLEDRECT, BOX, bicmos_box60}};
static TECH_NODES bicmos_pban = {
	x_("P_Base_Active_Node"), NPBAN, NONODEPROTO,
	K4, K4,
	1, bicmos_pban_p,
	1, bicmos_pban_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* BCCD_Node */
static TECH_PORTS bicmos_bn_p[] = {
	{bicmos_pc_10, x_("BCCD"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON bicmos_bn_l[] = {
	{LB, 0, 4, FILLEDRECT, BOX, bicmos_box60}};
static TECH_NODES bicmos_bn = {
	x_("BCCD_Node"), NBN, NONODEPROTO,
	K4, K4,
	1, bicmos_bn_p,
	1, bicmos_bn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Pdiff_Node */
static TECH_PORTS bicmos_pn2_p[] = {
	{bicmos_pc_10, x_("Pdiff"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3}};
static TECH_POLYGON bicmos_pn2_l[] = {
	{LA, 0, 4, FILLEDRECT, BOX, bicmos_box58},
	{LPS, 0, 4, FILLEDRECT, BOX, bicmos_box60}};
static TECH_NODES bicmos_pn2 = {
	x_("Pdiff_Node"), NPN2, NONODEPROTO,
	K8, K8,
	1, bicmos_pn2_p,
	2, bicmos_pn2_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Ohmic_Well */
static TECH_PORTS bicmos_ow_p[] = {
	{bicmos_pc_10, x_("Ohmic_Well"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON bicmos_ow_l[] = {
	{LOW, 0, 4, FILLEDRECT, BOX, bicmos_box60}};
static TECH_NODES bicmos_ow = {
	x_("Ohmic_Well"), NOW, NONODEPROTO,
	K4, K4,
	1, bicmos_ow_p,
	1, bicmos_ow_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Ohmic_Substrate */
static TECH_PORTS bicmos_os_p[] = {
	{bicmos_pc_10, x_("Ohmic_Substrate"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON bicmos_os_l[] = {
	{LOS, 0, 4, FILLEDRECT, BOX, bicmos_box60}};
static TECH_NODES bicmos_os = {
	x_("Ohmic_Substrate"), NOS, NONODEPROTO,
	K4, K4,
	1, bicmos_os_p,
	1, bicmos_os_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

TECH_NODES *bicmos_nodeprotos[NODEPROTOCOUNT+1] = {
	&bicmos_ap, &bicmos_mp, &bicmos_mp0, &bicmos_pp, &bicmos_pp0, &bicmos_np,
	&bicmos_pp1, &bicmos_nt, &bicmos_nt0, &bicmos_mpc, &bicmos_mnc, &bicmos_mpc0,
	&bicmos_mpc1, &bicmos_p, &bicmos_mmc, &bicmos_mnwc, &bicmos_ppc, &bicmos_n,
	&bicmos_msc, &bicmos_an, &bicmos_psn, &bicmos_pcn, &bicmos_acn, &bicmos_vn,
	&bicmos_pn, &bicmos_pfn, &bicmos_mn, &bicmos_mn0, &bicmos_pn0, &bicmos_pn1,
	&bicmos_nn, &bicmos_pcn0, &bicmos_nwn, &bicmos_nsn, &bicmos_pban, &bicmos_bn,
	&bicmos_pn2, &bicmos_ow, &bicmos_os, ((TECH_NODES *)-1)};

static INTBIG bicmos_node_widoff[NODEPROTOCOUNT*4] = {
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, K2,K2,K2,K2, K2,K2,K2,K2,
	K22,K16,K8,K8, 0,0,0,0, K5,K5,K5,K5, K2,K2,K2,K2, 0,0,0,0, K1,K1,K1,K1, K5,K5,K7,K7,
	0,0,0,0, K3,K3,K3,K3, 0,0,0,0, K2,K2,K4,K4, K3,K3,K3,K3, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, K2,K2,K2,K2,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, K2,K2,K2,K2, 0,0,0,0, 0,0,0,0};

/******************** VARIABLE AGGREGATION ********************/

TECH_VARIABLES bicmos_variables[] =
{
	{x_("TECH_layer_names"), (CHAR *)bicmos_layer_names, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_layer_function"), (CHAR *)bicmos_layer_function, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_node_width_offset"), (CHAR *)bicmos_node_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|((NODEPROTOCOUNT*4)<<VLENGTHSH)},
	{x_("TECH_arc_width_offset"), (CHAR *)bicmos_arc_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|(ARCPROTOCOUNT<<VLENGTHSH)},
	{x_("USER_layer_letters"), (CHAR *)bicmos_layer_letters, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("USER_color_map"), (CHAR *)bicmos_colmap, 0.0,
		VCHAR|VDONTSAVE|VISARRAY|((sizeof bicmos_colmap)<<VLENGTHSH)},
	{x_("IO_cif_layer_names"), (CHAR *)bicmos_cif_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("IO_gds_layer_numbers"), (CHAR *)bicmos_gds_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("DRC_min_unconnected_distances"), (CHAR *)bicmos_unconnectedtable, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|
		   (((sizeof bicmos_unconnectedtable)/SIZEOFINTBIG)<<VLENGTHSH)},
	{x_("DRC_min_connected_distances"), (CHAR *)bicmos_connectedtable, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|
			(((sizeof bicmos_connectedtable)/SIZEOFINTBIG)<<VLENGTHSH)},
	{NULL, NULL, 0.0, 0}
};

/******************** ROUTINES ********************/

BOOLEAN bicmos_initprocess(TECHNOLOGY *tech, INTBIG pass)
{
	Q_UNUSED( tech );
	Q_UNUSED( pass );
	return(FALSE);
}
