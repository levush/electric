/*
 * Electric(tm) VLSI Design System
 *
 * File: tecmocmos2.c
 * mocmos2 technology description
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
#if TECMOCMOS2

#include "global.h"
#include "egraphics.h"
#include "tech.h"
#include "efunction.h"

BOOLEAN mocmos2_initprocess(TECHNOLOGY*, INTBIG);

/******************** LAYERS ********************/

#define MAXLAYERS 29
#define LM        0			/* Metal_1 */
#define LM0       1			/* Metal_2 */
#define LP        2			/* Polysilicon */
#define LP0       3			/* Polysilicon_2 */
#define LSA       4			/* S_Active */
#define LDA       5			/* D_Active */
#define LPS       6			/* P_Select */
#define LNS       7			/* N_Select */
#define LPW       8			/* P_Well */
#define LNW       9			/* N_Well */
#define LCC       10		/* Contact_Cut */
#define LV        11		/* Via */
#define LP1       12		/* Passivation */
#define LPF       13		/* Pad_Frame */
#define LT        14		/* Transistor */
#define LPC       15		/* Poly_Cut */
#define LPC0      16		/* Poly_2_Cut */
#define LAC       17		/* Active_Cut */
#define LSAW      18		/* S_Active_Well */
#define LPM       19		/* Pseudo_Metal_1 */
#define LPM0      20		/* Pseudo_Metal_2 */
#define LPP       21		/* Pseudo_Polysilicon */
#define LPSA      22		/* Pseudo_S_Active */
#define LPDA      23		/* Pseudo_D_Active */
#define LPPS      24		/* Pseudo_P_Select */
#define LPNS      25		/* Pseudo_N_Select */
#define LPPW      26		/* Pseudo_P_Well */
#define LPNW      27		/* Pseudo_N_Well */
#define LPP0      28		/* Pseudo_Poly2 */

static GRAPHICS mocmos2_M_lay = {LAYERT1, COLORT1, SOLIDC, PATTERNED,
	{0x2222, /*   X   X   X   X  */
	0x0000,  /*                  */
	0x8888,  /* X   X   X   X    */
	0x0000,  /*                  */
	0x2222,  /*   X   X   X   X  */
	0x0000,  /*                  */
	0x8888,  /* X   X   X   X    */
	0x0000}, /*                  */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_M0_lay = {LAYERT4, COLORT4, SOLIDC, PATTERNED,
	{0x1010, /*    X       X     */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0404,  /*      X       X   */
	0x0808}, /*     X       X    */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_P_lay = {LAYERT2, COLORT2, SOLIDC, PATTERNED,
	{0x0808, /*     X       X    */
	0x0404,  /*      X       X   */
	0x0202,  /*       X       X  */
	0x0101,  /*        X       X */
	0x8080,  /* X       X        */
	0x4040,  /*  X       X       */
	0x2020,  /*   X       X      */
	0x1010}, /*    X       X     */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_P0_lay = {LAYERO, MAGENTA, PATTERNED, PATTERNED,
	{0xe0e0, /* XXX     XXX      */
	0x7070,  /*  XXX     XXX     */
	0x3838,  /*   XXX     XXX    */
	0x1c1c,  /*    XXX     XXX   */
	0x0e0e,  /*     XXX     XXX  */
	0x0707,  /*      XXX     XXX */
	0x8383,  /* X     XXX     XX */
	0xc1c1}, /* XX     XXX     X */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_SA_lay = {LAYERT3, COLORT3, SOLIDC, PATTERNED,
	{0x0000, /*                  */
	0x0303,  /*       XX      XX */
	0x4848,  /*  X  X    X  X    */
	0x0303,  /*       XX      XX */
	0x0000,  /*                  */
	0x3030,  /*   XX      XX     */
	0x8484,  /* X    X  X    X   */
	0x3030}, /*   XX      XX     */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_DA_lay = {LAYERT3, COLORT3, SOLIDC, PATTERNED,
	{0x0000, /*                  */
	0x0303,  /*       XX      XX */
	0x4848,  /*  X  X    X  X    */
	0x0303,  /*       XX      XX */
	0x0000,  /*                  */
	0x3030,  /*   XX      XX     */
	0x8484,  /* X    X  X    X   */
	0x3030}, /*   XX      XX     */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_PS_lay = {LAYERO, YELLOW, SOLIDC, PATTERNED,
	{0x1010, /*    X       X     */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0404,  /*      X       X   */
	0x0808}, /*     X       X    */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_NS_lay = {LAYERO, YELLOW, SOLIDC, PATTERNED,
	{0x1010, /*    X       X     */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0404,  /*      X       X   */
	0x0808}, /*     X       X    */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_PW_lay = {LAYERT5, COLORT5, SOLIDC, PATTERNED,
	{0x1000, /*    X             */
	0x0020,  /*           X      */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0001,  /*                X */
	0x0200,  /*       X          */
	0x0000,  /*                  */
	0x0000}, /*                  */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_NW_lay = {LAYERT5, COLORT5, SOLIDC, PATTERNED,
	{0x1000, /*    X             */
	0x0020,  /*           X      */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0001,  /*                X */
	0x0200,  /*       X          */
	0x0000,  /*                  */
	0x0000}, /*                  */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_CC_lay = {LAYERO, BLACK, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmos2_V_lay = {LAYERO, BLACK, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmos2_P1_lay = {LAYERO, DGRAY, SOLIDC, PATTERNED,
	{0x1c1c, /*    XXX     XXX   */
	0x3e3e,  /*   XXXXX   XXXXX  */
	0x3636,  /*   XX XX   XX XX  */
	0x3e3e,  /*   XXXXX   XXXXX  */
	0x1c1c,  /*    XXX     XXX   */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000}, /*                  */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_PF_lay = {LAYERO, RED, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmos2_T_lay = {LAYERO, ALLOFF, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmos2_PC_lay = {LAYERO, BLACK, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmos2_PC0_lay = {LAYERO, BLACK, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmos2_AC_lay = {LAYERO, BLACK, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmos2_SAW_lay = {LAYERT3, COLORT3, SOLIDC, PATTERNED,
	{0x0000, /*                  */
	0x0303,  /*       XX      XX */
	0x4848,  /*  X  X    X  X    */
	0x0303,  /*       XX      XX */
	0x0000,  /*                  */
	0x3030,  /*   XX      XX     */
	0x8484,  /* X    X  X    X   */
	0x3030}, /*   XX      XX     */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_PM_lay = {LAYERT1, COLORT1, SOLIDC, PATTERNED,
	{0x2222, /*   X   X   X   X  */
	0x0000,  /*                  */
	0x8888,  /* X   X   X   X    */
	0x0000,  /*                  */
	0x2222,  /*   X   X   X   X  */
	0x0000,  /*                  */
	0x8888,  /* X   X   X   X    */
	0x0000}, /*                  */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_PM0_lay = {LAYERT4, COLORT4, SOLIDC, PATTERNED,
	{0x1010, /*    X       X     */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0404,  /*      X       X   */
	0x0808}, /*     X       X    */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_PP_lay = {LAYERT2, COLORT2, SOLIDC, PATTERNED,
	{0x0808, /*     X       X    */
	0x0404,  /*      X       X   */
	0x0202,  /*       X       X  */
	0x0101,  /*        X       X */
	0x8080,  /* X       X        */
	0x4040,  /*  X       X       */
	0x2020,  /*   X       X      */
	0x1010}, /*    X       X     */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_PSA_lay = {LAYERT3, COLORT3, SOLIDC, PATTERNED,
	{0x0000, /*                  */
	0x0303,  /*       XX      XX */
	0x4848,  /*  X  X    X  X    */
	0x0303,  /*       XX      XX */
	0x0000,  /*                  */
	0x3030,  /*   XX      XX     */
	0x8484,  /* X    X  X    X   */
	0x3030}, /*   XX      XX     */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_PDA_lay = {LAYERT3, COLORT3, SOLIDC, PATTERNED,
	{0x0000, /*                  */
	0x0303,  /*       XX      XX */
	0x4848,  /*  X  X    X  X    */
	0x0303,  /*       XX      XX */
	0x0000,  /*                  */
	0x3030,  /*   XX      XX     */
	0x8484,  /* X    X  X    X   */
	0x3030}, /*   XX      XX     */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_PPS_lay = {LAYERO, YELLOW, SOLIDC, PATTERNED,
	{0x1010, /*    X       X     */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0404,  /*      X       X   */
	0x0808}, /*     X       X    */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_PNS_lay = {LAYERO, YELLOW, SOLIDC, PATTERNED,
	{0x1010, /*    X       X     */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0404,  /*      X       X   */
	0x0808}, /*     X       X    */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_PPW_lay = {LAYERT5, COLORT5, SOLIDC, PATTERNED,
	{0x1000, /*    X             */
	0x0020,  /*           X      */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0001,  /*                X */
	0x0200,  /*       X          */
	0x0000,  /*                  */
	0x0000}, /*                  */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_PNW_lay = {LAYERT5, COLORT5, SOLIDC, PATTERNED,
	{0x1000, /*    X             */
	0x0020,  /*           X      */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0001,  /*                X */
	0x0200,  /*       X          */
	0x0000,  /*                  */
	0x0000}, /*                  */
	NOVARIABLE, 0};
static GRAPHICS mocmos2_PP0_lay = {LAYERO, MAGENTA, SOLIDC, PATTERNED,
	{0,0,0,0,0,0,0,0}, NOVARIABLE, 0};

GRAPHICS *mocmos2_layers[MAXLAYERS+1] = {&mocmos2_M_lay, &mocmos2_M0_lay,
	&mocmos2_P_lay, &mocmos2_P0_lay, &mocmos2_SA_lay, &mocmos2_DA_lay,
	&mocmos2_PS_lay, &mocmos2_NS_lay, &mocmos2_PW_lay, &mocmos2_NW_lay,
	&mocmos2_CC_lay, &mocmos2_V_lay, &mocmos2_P1_lay, &mocmos2_PF_lay,
	&mocmos2_T_lay, &mocmos2_PC_lay, &mocmos2_PC0_lay, &mocmos2_AC_lay,
	&mocmos2_SAW_lay, &mocmos2_PM_lay, &mocmos2_PM0_lay, &mocmos2_PP_lay,
	&mocmos2_PSA_lay, &mocmos2_PDA_lay, &mocmos2_PPS_lay, &mocmos2_PNS_lay,
	&mocmos2_PPW_lay, &mocmos2_PNW_lay, &mocmos2_PP0_lay, NOGRAPHICS};
static CHAR *mocmos2_layer_names[MAXLAYERS] = {x_("Metal_1"), x_("Metal_2"),
	x_("Polysilicon"), x_("Polysilicon_2"), x_("S_Active"), x_("D_Active"), x_("P_Select"),
	x_("N_Select"), x_("P_Well"), x_("N_Well"), x_("Contact_Cut"), x_("Via"), x_("Passivation"),
	x_("Pad_Frame"), x_("Transistor"), x_("Poly_Cut"), x_("Poly_2_Cut"), x_("Active_Cut"),
	x_("S_Active_Well"), x_("Pseudo_Metal_1"), x_("Pseudo_Metal_2"), x_("Pseudo_Polysilicon"),
	x_("Pseudo_S_Active"), x_("Pseudo_D_Active"), x_("Pseudo_P_Select"), x_("Pseudo_N_Select"),
	x_("Pseudo_P_Well"), x_("Pseudo_N_Well"), x_("Pseudo_Poly2")};
static CHAR *mocmos2_cif_layers[MAXLAYERS] = {x_("CMF"), x_("CMS"), x_("CPG"), x_("CEL"), x_("CAA"), x_("CAA"), x_("CSG"), x_("CSG"), x_("CWG"), x_("CWG"), x_("CC"), x_("CVA"), x_("COG"), x_("CX"), x_(""), x_("CCP"), x_("CCE"), x_("CCA"), x_("CAA"), x_(""), x_(""), x_(""), x_(""), x_(""), x_(""), x_(""), x_(""), x_(""), x_("XX")};
static INTBIG mocmos2_gds_layers[MAXLAYERS] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static INTBIG mocmos2_layer_function[MAXLAYERS] = {LFMETAL1|LFTRANS1,
	LFMETAL2|LFTRANS4, LFPOLY1|LFTRANS2, LFPOLY2, LFDIFF|LFPTYPE|LFTRANS3,
	LFDIFF|LFNTYPE|LFTRANS3, LFIMPLANT|LFPTYPE, LFIMPLANT|LFNTYPE,
	LFWELL|LFPTYPE|LFTRANS5, LFWELL|LFNTYPE|LFTRANS5, LFCONTACT1, LFCONTACT2|LFCONMETAL,
	LFOVERGLASS, LFART, LFTRANSISTOR|LFPSEUDO, LFCONTACT1|LFCONPOLY, LFCONTACT3|LFCONPOLY,
	LFCONTACT1|LFCONDIFF, LFDIFF|LFPTYPE|LFTRANS3, LFMETAL1|LFPSEUDO|LFTRANS1,
	LFMETAL2|LFPSEUDO|LFTRANS4, LFPOLY1|LFPSEUDO|LFTRANS2,
	LFDIFF|LFPTYPE|LFPSEUDO|LFTRANS3, LFDIFF|LFNTYPE|LFPSEUDO|LFTRANS3,
	LFIMPLANT|LFPTYPE|LFPSEUDO, LFIMPLANT|LFNTYPE|LFPSEUDO,
	LFWELL|LFPTYPE|LFPSEUDO|LFTRANS5, LFWELL|LFNTYPE|LFPSEUDO|LFTRANS5,
	LFUNKNOWN|LFPSEUDO};
static CHAR *mocmos2_layer_letters[MAXLAYERS] = {x_("m"), x_("h"), x_("p"), x_("g"), x_("s"), x_("d"), x_("e"), x_("f"), x_("w"), x_("n"), x_("c"), x_("v"), x_("o"), x_("b"), x_("t"), x_("a"), x_("i"), x_("A"), x_("x"), x_("M"), x_("H"), x_("P"), x_("S"), x_("D"), x_("E"), x_("F"), x_("W"), x_("N"), x_("x")};

static TECH_COLORMAP mocmos2_colmap[32] =
{
	{200,200,200}, /*  0:        +           +        +       +       */
	{ 96,209,255}, /*  1: Metal_1+           +        +       +       */
	{255,155,192}, /*  2:        +Polysilicon+        +       +       */
	{ 96,127,192}, /*  3: Metal_1+Polysilicon+        +       +       */
	{107,226, 96}, /*  4:        +           +S_Active+       +       */
	{ 40,186, 96}, /*  5: Metal_1+           +S_Active+       +       */
	{107,137, 72}, /*  6:        +Polysilicon+S_Active+       +       */
	{ 40,113, 72}, /*  7: Metal_1+Polysilicon+S_Active+       +       */
	{224, 95,255}, /*  8:        +           +        +Metal_2+       */
	{ 85, 78,255}, /*  9: Metal_1+           +        +Metal_2+       */
	{224, 57,192}, /* 10:        +Polysilicon+        +Metal_2+       */
	{ 85, 47,192}, /* 11: Metal_1+Polysilicon+        +Metal_2+       */
	{ 94, 84, 96}, /* 12:        +           +S_Active+Metal_2+       */
	{ 36, 69, 96}, /* 13: Metal_1+           +S_Active+Metal_2+       */
	{ 94, 51, 72}, /* 14:        +Polysilicon+S_Active+Metal_2+       */
	{ 36, 42, 72}, /* 15: Metal_1+Polysilicon+S_Active+Metal_2+       */
	{240,221,181}, /* 16:        +           +        +       +P_Well */
	{ 91,182,181}, /* 17: Metal_1+           +        +       +P_Well */
	{240,134,136}, /* 18:        +Polysilicon+        +       +P_Well */
	{ 91,111,136}, /* 19: Metal_1+Polysilicon+        +       +P_Well */
	{101,196, 68}, /* 20:        +           +S_Active+       +P_Well */
	{ 38,161, 68}, /* 21: Metal_1+           +S_Active+       +P_Well */
	{101,119, 51}, /* 22:        +Polysilicon+S_Active+       +P_Well */
	{ 38, 98, 51}, /* 23: Metal_1+Polysilicon+S_Active+       +P_Well */
	{211, 82,181}, /* 24:        +           +        +Metal_2+P_Well */
	{ 80, 68,181}, /* 25: Metal_1+           +        +Metal_2+P_Well */
	{211, 50,136}, /* 26:        +Polysilicon+        +Metal_2+P_Well */
	{ 80, 41,136}, /* 27: Metal_1+Polysilicon+        +Metal_2+P_Well */
	{ 89, 73, 68}, /* 28:        +           +S_Active+Metal_2+P_Well */
	{ 33, 60, 68}, /* 29: Metal_1+           +S_Active+Metal_2+P_Well */
	{ 89, 44, 51}, /* 30:        +Polysilicon+S_Active+Metal_2+P_Well */
	{ 33, 36, 51}, /* 31: Metal_1+Polysilicon+S_Active+Metal_2+P_Well */
};

/******************** DESIGN RULES ********************/

static INTBIG mocmos2_unconnectedtable[] = {
/*            M  M  P  P  S  D  P  N  P  N  C  V  P  P  T  P  P  A  S  P  P  P  P  P  P  P  P  P  P   */
/*               0     0  A  A  S  S  W  W  C     1  F     C  C  C  A  M  M  P  S  D  P  N  P  N  P   */
/*                                                            0     W     0     A  A  S  S  W  W  0   */
/*                                                                                                    */
/*                                                                                                    */
/*                                                                                                    */
/* M      */ K3,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* M0     */    K4,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* P      */       K2,XX,K1,K1,XX,XX,XX,XX,XX,K2,XX,XX,XX,K4,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* P0     */          K3,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* SA     */             K3,K3,XX,XX,K4,XX,XX,K2,XX,XX,XX,XX,XX,K5,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* DA     */                K3,XX,XX,XX,XX,XX,K2,XX,XX,XX,XX,XX,K5,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PS     */                   XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* NS     */                      XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PW     */                         XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* NW     */                            XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* CC     */                               K2,K2,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* V      */                                  K2,XX,XX,K2,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* P1     */                                     XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PF     */                                        XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* T      */                                           XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PC     */                                              XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PC0    */                                                 XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* AC     */                                                    XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* SAW    */                                                       XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PM     */                                                          XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PM0    */                                                             XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PP     */                                                                XX,XX,XX,XX,XX,XX,XX,XX,
/* PSA    */                                                                   XX,XX,XX,XX,XX,XX,XX,
/* PDA    */                                                                      XX,XX,XX,XX,XX,XX,
/* PPS    */                                                                         XX,XX,XX,XX,XX,
/* PNS    */                                                                            XX,XX,XX,XX,
/* PPW    */                                                                               XX,XX,XX,
/* PNW    */                                                                                  XX,XX,
/* PP0    */                                                                                     XX
};

static INTBIG mocmos2_connectedtable[] = {
/*            M  M  P  P  S  D  P  N  P  N  C  V  P  P  T  P  P  A  S  P  P  P  P  P  P  P  P  P  P   */
/*               0     0  A  A  S  S  W  W  C     1  F     C  C  C  A  M  M  P  S  D  P  N  P  N  P   */
/*                                                            0     W     0     A  A  S  S  W  W  0   */
/*                                                                                                    */
/*                                                                                                    */
/*                                                                                                    */
/* M      */ XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* M0     */    XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* P      */       XX,XX,K1,K1,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* P0     */          XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* SA     */             XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* DA     */                XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PS     */                   XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* NS     */                      XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PW     */                         XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* NW     */                            XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* CC     */                               XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* V      */                                  K2,XX,XX,XX,K2,XX,K2,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* P1     */                                     XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PF     */                                        XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* T      */                                           XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PC     */                                              K2,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PC0    */                                                 XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* AC     */                                                    K2,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* SAW    */                                                       XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PM     */                                                          XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PM0    */                                                             XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* PP     */                                                                XX,XX,XX,XX,XX,XX,XX,XX,
/* PSA    */                                                                   XX,XX,XX,XX,XX,XX,XX,
/* PDA    */                                                                      XX,XX,XX,XX,XX,XX,
/* PPS    */                                                                         XX,XX,XX,XX,XX,
/* PNS    */                                                                            XX,XX,XX,XX,
/* PPW    */                                                                               XX,XX,XX,
/* PNW    */                                                                                  XX,XX,
/* PP0    */                                                                                     XX
};

/******************** ARCS ********************/

#define ARCPROTOCOUNT  7
#define AMETAL_1       0		/* Metal_1 */
#define AMETAL_2       1		/* Metal_2 */
#define APOLYSILICON   2		/* Polysilicon */
#define APOLYSILICON_2 3		/* Polysilicon_2 */
#define AS_ACTIVE      4		/* S_Active */
#define AD_ACTIVE      5		/* D_Active */
#define AACTIVE        6		/* Active */

static TECH_ARCLAY mocmos2_al_0[] = {{LM,0,FILLED}};
static TECH_ARCS mocmos2_a_0 = {
	x_("Metal_1"), K3, AMETAL_1, NOARCPROTO,
	1, mocmos2_al_0,
	(APMETAL1<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};

static TECH_ARCLAY mocmos2_al_1[] = {{LM0,0,FILLED}};
static TECH_ARCS mocmos2_a_1 = {
	x_("Metal_2"), K3, AMETAL_2, NOARCPROTO,
	1, mocmos2_al_1,
	(APMETAL2<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};

static TECH_ARCLAY mocmos2_al_2[] = {{LP,0,FILLED}};
static TECH_ARCS mocmos2_a_2 = {
	x_("Polysilicon"), K2, APOLYSILICON, NOARCPROTO,
	1, mocmos2_al_2,
	(APPOLY1<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};

static TECH_ARCLAY mocmos2_al_3[] = {{LP0,0,FILLED}};
static TECH_ARCS mocmos2_a_3 = {
	x_("Polysilicon_2"), K2, APOLYSILICON_2, NOARCPROTO,
	1, mocmos2_al_3,
	(APPOLY2<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};

static TECH_ARCLAY mocmos2_al_4[] = {{LSA,K4,FILLED}, {LPS,0,CLOSED}};
static TECH_ARCS mocmos2_a_4 = {
	x_("S_Active"), K6, AS_ACTIVE, NOARCPROTO,
	2, mocmos2_al_4,
	(APDIFFP<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};

static TECH_ARCLAY mocmos2_al_5[] = {{LDA,K8,FILLED}, {LPW,0,FILLED}};
static TECH_ARCS mocmos2_a_5 = {
	x_("D_Active"), K10, AD_ACTIVE, NOARCPROTO,
	2, mocmos2_al_5,
	(APDIFFN<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};

static TECH_ARCLAY mocmos2_al_6[] = {{LDA,0,FILLED}, {LSA,0,FILLED}};
static TECH_ARCS mocmos2_a_6 = {
	x_("Active"), K2, AACTIVE, NOARCPROTO,
	2, mocmos2_al_6,
	(APDIFF<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};

TECH_ARCS *mocmos2_arcprotos[ARCPROTOCOUNT+1] = {
	&mocmos2_a_0, &mocmos2_a_1, &mocmos2_a_2, &mocmos2_a_3, &mocmos2_a_4, &mocmos2_a_5, &mocmos2_a_6, ((TECH_ARCS *)-1)};

static INTBIG mocmos2_arc_widoff[ARCPROTOCOUNT] = {0, 0, 0, 0, K4, K8, 0};

/******************** PORT CONNECTIONS ********************/

static INTBIG mocmos2_pc_1[] = {-1, ALLGEN, -1};
static INTBIG mocmos2_pc_2[] = {-1, AMETAL_1, AACTIVE, ALLGEN, -1};
static INTBIG mocmos2_pc_3[] = {-1, AMETAL_1, AMETAL_2, ALLGEN, -1};
static INTBIG mocmos2_pc_4[] = {-1, AMETAL_1, APOLYSILICON_2, ALLGEN, -1};
static INTBIG mocmos2_pc_5[] = {-1, APOLYSILICON, AMETAL_1, ALLGEN, -1};
static INTBIG mocmos2_pc_6[] = {-1, AD_ACTIVE, AMETAL_1, ALLGEN, -1};
static INTBIG mocmos2_pc_7[] = {-1, AS_ACTIVE, AMETAL_1, ALLGEN, -1};
static INTBIG mocmos2_pc_8[] = {-1, AACTIVE, AS_ACTIVE, AD_ACTIVE, ALLGEN, -1};
static INTBIG mocmos2_pc_9[] = {-1, AD_ACTIVE, ALLGEN, -1};
static INTBIG mocmos2_pc_10[] = {-1, AS_ACTIVE, ALLGEN, -1};
static INTBIG mocmos2_pc_11[] = {-1, APOLYSILICON_2, ALLGEN, -1};
static INTBIG mocmos2_pc_12[] = {-1, APOLYSILICON, ALLGEN, -1};
static INTBIG mocmos2_pc_13[] = {-1, AMETAL_2, ALLGEN, -1};
static INTBIG mocmos2_pc_14[] = {-1, AMETAL_1, ALLGEN, -1};

/******************** RECTANGLE DESCRIPTIONS ********************/

static INTBIG mocmos2_box1[8] = {CENTERL1, CENTERD1, CENTERR1, CENTERU1};
static INTBIG mocmos2_box2[8] = {LEFTIN4, BOTIN4, RIGHTIN4, BOTIN6};
static INTBIG mocmos2_box3[8] = {LEFTIN4, TOPIN6, RIGHTIN4, TOPIN4};
static INTBIG mocmos2_box4[8] = {LEFTIN2, BOTIN6, RIGHTIN2, TOPIN6};
static INTBIG mocmos2_box5[8] = {LEFTIN4, BOTIN6, RIGHTIN4, TOPIN6};
static INTBIG mocmos2_box6[8] = {LEFTIN2, BOTIN2, RIGHTIN2, BOTIN4};
static INTBIG mocmos2_box7[8] = {LEFTIN2, TOPIN4, RIGHTIN2, TOPIN2};
static INTBIG mocmos2_box8[8] = {LEFTIN2, BOTIN4, RIGHTIN2, TOPIN4};
static INTBIG mocmos2_box9[8] = {LEFTEDGE, BOTIN4, RIGHTEDGE, TOPIN4};
static INTBIG mocmos2_box10[16] = {LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1, CENTERL2, CENTERD2, CENTERR2, CENTERU2};
static INTBIG mocmos2_box11[16] = {LEFTIN5, BOTIN5, RIGHTIN5, TOPIN5, CENTERL2, CENTERD2, CENTERR2, CENTERU2};
static INTBIG mocmos2_box12[8] = {LEFTIN1, BOTIN1, LEFTIN3, BOTIN3};
static INTBIG mocmos2_box13[16] = {LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3, CENTERL2, CENTERD2, CENTERR2, CENTERU2};
static INTBIG mocmos2_box14[8] = {LEFTIN4, BOTIN4, RIGHTIN4, TOPIN4};
static INTBIG mocmos2_box15[8] = {LEFTIN2, BOTIN2, RIGHTIN2, TOPIN2};
static INTBIG mocmos2_box16[8] = {LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1};
static INTBIG mocmos2_box17[8] = {LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE};

/******************** NODES ********************/

#define NODEPROTOCOUNT 32
#define NMP            1		/* Metal_1_Pin */
#define NMP0           2		/* Metal_2_Pin */
#define NPP            3		/* Polysilicon_Pin */
#define NPP0           4		/* Polysilicon_2_Pin */
#define NSAP           5		/* S_Active_Pin */
#define NDAP           6		/* D_Active_Pin */
#define NAP            7		/* Active_Pin */
#define NMSAC          8		/* Metal_1_S_Active_Con */
#define NMDAC          9		/* Metal_1_D_Active_Con */
#define NMPC           10		/* Metal_1_Polysilicon_Con */
#define NMPC0          11		/* Metal_1_Polysilicon_2_Con */
#define NST            12		/* S_Transistor */
#define NDT            13		/* D_Transistor */
#define NMMC           14		/* Metal_1_Metal_2_Con */
#define NMWC           15		/* Metal_1_Well_Con */
#define NMSC           16		/* Metal_1_Substrate_Con */
#define NPPC           17		/* P1_P2_Capacitor */
#define NMN            18		/* Metal_1_Node */
#define NMN0           19		/* Metal_2_Node */
#define NPN            20		/* Polysilicon_Node */
#define NPN0           21		/* Polysilicon_2_Node */
#define NAN            22		/* Active_Node */
#define NDAN           23		/* D_Active_Node */
#define NPSN           24		/* P_Select_Node */
#define NCN            25		/* Cut_Node */
#define NPCN           26		/* Poly_Cut_Node */
#define NPCN0          27		/* Poly_2_Cut_Node */
#define NACN           28		/* Active_Cut_Node */
#define NVN            29		/* Via_Node */
#define NPWN           30		/* P_Well_Node */
#define NPN1           31		/* Passivation_Node */
#define NPFN           32		/* Pad_Frame_Node */

/* Metal_1_Pin */
static TECH_PORTS mocmos2_mp_p[] = {
	{mocmos2_pc_14, x_("metal-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos2_mp_l[] = {
	{LPM, 0, 4, CROSSED, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_mp = {
	x_("Metal_1_Pin"), NMP, NONODEPROTO,
	K4, K4,
	1, mocmos2_mp_p,
	1, mocmos2_mp_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* Metal_2_Pin */
static TECH_PORTS mocmos2_mp0_p[] = {
	{mocmos2_pc_13, x_("metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos2_mp0_l[] = {
	{LPM0, 0, 4, CROSSED, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_mp0 = {
	x_("Metal_2_Pin"), NMP0, NONODEPROTO,
	K4, K4,
	1, mocmos2_mp0_p,
	1, mocmos2_mp0_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* Polysilicon_Pin */
static TECH_PORTS mocmos2_pp_p[] = {
	{mocmos2_pc_12, x_("polysilicon"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON mocmos2_pp_l[] = {
	{LPP, 0, 4, CROSSED, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_pp = {
	x_("Polysilicon_Pin"), NPP, NONODEPROTO,
	K2, K2,
	1, mocmos2_pp_p,
	1, mocmos2_pp_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* Polysilicon_2_Pin */
static TECH_PORTS mocmos2_pp0_p[] = {
	{mocmos2_pc_11, x_("p2-pin"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON mocmos2_pp0_l[] = {
	{LPP0, 0, 4, CROSSED, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_pp0 = {
	x_("Polysilicon_2_Pin"), NPP0, NONODEPROTO,
	K2, K2,
	1, mocmos2_pp0_p,
	1, mocmos2_pp0_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* S_Active_Pin */
static TECH_PORTS mocmos2_sap_p[] = {
	{mocmos2_pc_10, x_("s-active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3}};
static TECH_POLYGON mocmos2_sap_l[] = {
	{LPSA, 0, 4, CROSSED, BOX, mocmos2_box15},
	{LPPS, 0, 4, CROSSED, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_sap = {
	x_("S_Active_Pin"), NSAP, NONODEPROTO,
	K6, K6,
	1, mocmos2_sap_p,
	2, mocmos2_sap_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* D_Active_Pin */
static TECH_PORTS mocmos2_dap_p[] = {
	{mocmos2_pc_9, x_("d-active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN5, BOTIN5, RIGHTIN5, TOPIN5}};
static TECH_POLYGON mocmos2_dap_l[] = {
	{LPPW, 0, 4, CROSSED, BOX, mocmos2_box17},
	{LPDA, 0, 4, CROSSED, BOX, mocmos2_box14}};
static TECH_NODES mocmos2_dap = {
	x_("D_Active_Pin"), NDAP, NONODEPROTO,
	K10, K10,
	1, mocmos2_dap_p,
	2, mocmos2_dap_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* Active_Pin */
static TECH_PORTS mocmos2_ap_p[] = {
	{mocmos2_pc_8, x_("active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON mocmos2_ap_l[] = {
	{LPSA, 0, 4, CROSSED, BOX, mocmos2_box17},
	{LPDA, 0, 4, CROSSED, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_ap = {
	x_("Active_Pin"), NAP, NONODEPROTO,
	K2, K2,
	1, mocmos2_ap_p,
	2, mocmos2_ap_l,
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,
	0,0,0,0,0,0,0,0,0};

/* Metal_1_S_Active_Con */
static TECH_PORTS mocmos2_msac_p[] = {
	{mocmos2_pc_7, x_("metal-1-s-act"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3}};
static TECH_POLYGON mocmos2_msac_l[] = {
	{LM, 0, 4, FILLEDRECT, MINBOX, mocmos2_box13},
	{LSA, 0, 4, FILLEDRECT, BOX, mocmos2_box15},
	{LPS, 0, 4, CLOSEDRECT, BOX, mocmos2_box17},
	{LAC, 0, 4, FILLEDRECT, BOX, mocmos2_box12}};
static TECH_NODES mocmos2_msac = {
	x_("Metal_1_S_Active_Con"), NMSAC, NONODEPROTO,
	K10, K10,
	1, mocmos2_msac_p,
	4, mocmos2_msac_l,
	(NPCONTACT<<NFUNCTIONSH),
	MULTICUT,K2,K2,K2,K2,0,0,0,0};

/* Metal_1_D_Active_Con */
static TECH_PORTS mocmos2_mdac_p[] = {
	{mocmos2_pc_6, x_("metal-1-d-act"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN5, BOTIN5, RIGHTIN5, TOPIN5}};
static TECH_POLYGON mocmos2_mdac_l[] = {
	{LDA, 0, 4, FILLEDRECT, BOX, mocmos2_box14},
	{LM, 0, 4, FILLEDRECT, MINBOX, mocmos2_box11},
	{LPW, 0, 4, FILLEDRECT, BOX, mocmos2_box17},
	{LAC, 0, 4, FILLEDRECT, BOX, mocmos2_box12}};
static TECH_NODES mocmos2_mdac = {
	x_("Metal_1_D_Active_Con"), NMDAC, NONODEPROTO,
	K14, K14,
	1, mocmos2_mdac_p,
	4, mocmos2_mdac_l,
	(NPCONTACT<<NFUNCTIONSH),
	MULTICUT,K2,K2,K2,K2,0,0,0,0};

/* Metal_1_Polysilicon_Con */
static TECH_PORTS mocmos2_mpc_p[] = {
	{mocmos2_pc_5, x_("metal-1-polysilicon"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON mocmos2_mpc_l[] = {
	{LM, 0, 4, FILLEDRECT, MINBOX, mocmos2_box10},
	{LP, 0, 4, FILLEDRECT, BOX, mocmos2_box17},
	{LPC, 0, 4, FILLEDRECT, BOX, mocmos2_box12}};
static TECH_NODES mocmos2_mpc = {
	x_("Metal_1_Polysilicon_Con"), NMPC, NONODEPROTO,
	K6, K6,
	1, mocmos2_mpc_p,
	3, mocmos2_mpc_l,
	(NPCONTACT<<NFUNCTIONSH),
	MULTICUT,K2,K2,K2,K2,0,0,0,0};

/* Metal_1_Polysilicon_2_Con */
static TECH_PORTS mocmos2_mpc0_p[] = {
	{mocmos2_pc_4, x_("M1P2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON mocmos2_mpc0_l[] = {
	{LM, 0, 4, FILLEDRECT, BOX, mocmos2_box16},
	{LP0, 0, 4, FILLEDRECT, BOX, mocmos2_box17},
	{LPC0, 0, 4, FILLEDRECT, BOX, mocmos2_box12}};
static TECH_NODES mocmos2_mpc0 = {
	x_("Metal_1_Polysilicon_2_Con"), NMPC0, NONODEPROTO,
	K6, K6,
	1, mocmos2_mpc0_p,
	3, mocmos2_mpc0_l,
	(NPCONTACT<<NFUNCTIONSH),
	MULTICUT,K2,K2,K1,K2,0,0,0,0};

/* S_Transistor */
static TECH_PORTS mocmos2_st_p[] = {
	{mocmos2_pc_12, x_("s-trans-poly-left"), NOPORTPROTO, (90<<PORTARANGESH)|(180<<PORTANGLESH)|(1<<PORTNETSH),
		LEFTEDGE, BOTIN5, LEFTIN1, TOPIN5},
	{mocmos2_pc_10, x_("s-trans-diff-top"), NOPORTPROTO, (90<<PORTARANGESH)|(90<<PORTANGLESH),
		LEFTIN3, TOPIN3, RIGHTIN3, TOPIN2},
	{mocmos2_pc_12, x_("s-trans-poly-right"), NOPORTPROTO, (90<<PORTARANGESH)|(1<<PORTNETSH),
		RIGHTIN1, BOTIN5, RIGHTEDGE, TOPIN5},
	{mocmos2_pc_10, x_("s-trans-diff-bottom"), NOPORTPROTO, (90<<PORTARANGESH)|(270<<PORTANGLESH)|(3<<PORTNETSH),
		LEFTIN3, BOTIN2, RIGHTIN3, BOTIN3}};
static TECH_SERPENT mocmos2_st_l[] = {
	{{LP,  0, 4, FILLEDRECT, BOX, mocmos2_box9},  K1, K1, K2, K2},
	{{LSA, 0, 4, FILLEDRECT, BOX, mocmos2_box15}, K3, K3, K0, K0},
	{{LPS,-1, 4, CLOSEDRECT, BOX, mocmos2_box17}, K5, K5, K2, K2}};
static TECH_SERPENT mocmos2_stE_l[] = {
	{{LP,  0, 4, FILLEDRECT, BOX, mocmos2_box9},  K1, K1, K2, K2},
	{{LSA, 1, 4, FILLEDRECT, BOX, mocmos2_box7},  K3,  0, K0, K0},
	{{LSA, 3, 4, FILLEDRECT, BOX, mocmos2_box6},   0, K3, K0, K0},
	{{LPS,-1, 4, CLOSEDRECT, BOX, mocmos2_box17}, K5, K5, K2, K2}};
static TECH_NODES mocmos2_st = {
	x_("S_Transistor"), NST, NONODEPROTO,
	K6, K10,
	4, mocmos2_st_p,
	3, (TECH_POLYGON *)0,
	(NPTRAPMOS<<NFUNCTIONSH)|HOLDSTRACE|NODESHRINK,
	SERPTRANS,4,K1,K1,K2,K1,K1,mocmos2_st_l,mocmos2_stE_l};

/* D_Transistor */
static TECH_PORTS mocmos2_dt_p[] = {
	{mocmos2_pc_12, x_("d-trans-poly-left"), NOPORTPROTO, (90<<PORTARANGESH)|(180<<PORTANGLESH)|(2<<PORTNETSH),
		LEFTIN2, BOTIN7, LEFTIN3, TOPIN7},
	{mocmos2_pc_9, x_("d-trans-diff-top"), NOPORTPROTO, (90<<PORTARANGESH)|(90<<PORTANGLESH)|(1<<PORTNETSH),
		LEFTIN5, TOPIN5, RIGHTIN5, TOPIN4},
	{mocmos2_pc_12, x_("d-trans-poly-right"), NOPORTPROTO, (90<<PORTARANGESH)|(2<<PORTNETSH),
		RIGHTIN3, BOTIN7, RIGHTIN2, TOPIN7},
	{mocmos2_pc_9, x_("d-trans-diff-bottom"), NOPORTPROTO, (90<<PORTARANGESH)|(270<<PORTANGLESH),
		LEFTIN5, BOTIN4, RIGHTIN5, BOTIN5}};
static TECH_SERPENT mocmos2_dt_l[] = {
	{{LP,  0, 4, FILLEDRECT, BOX, mocmos2_box4},  K1, K1, K2, K2},
	{{LDA, 0, 4, FILLEDRECT, BOX, mocmos2_box14}, K3, K3, K0, K0},
	{{LPW,-1, 4, FILLEDRECT, BOX, mocmos2_box17}, K7, K7, K4, K4}};
static TECH_SERPENT mocmos2_dtE_l[] = {
	{{LP,  0, 4, FILLEDRECT, BOX, mocmos2_box4},  K1, K1, K2, K2},
	{{LDA, 1, 4, FILLEDRECT, BOX, mocmos2_box3},  K3,  0, K0, K0},
	{{LDA, 3, 4, FILLEDRECT, BOX, mocmos2_box2},   0, K3, K0, K0},
	{{LPW,-1, 4, FILLEDRECT, BOX, mocmos2_box17}, K7, K7, K4, K4}};
static TECH_NODES mocmos2_dt = {
	x_("D_Transistor"), NDT, NONODEPROTO,
	K10, K14,
	4, mocmos2_dt_p,
	3, (TECH_POLYGON *)0,
	(NPTRANMOS<<NFUNCTIONSH)|HOLDSTRACE|NODESHRINK,
	SERPTRANS,4,K1,K1,K2,K1,K1,mocmos2_dt_l,mocmos2_dtE_l};

/* Metal_1_Metal_2_Con */
static TECH_PORTS mocmos2_mmc_p[] = {
	{mocmos2_pc_3, x_("metal-1-metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos2_mmc_l[] = {
	{LM0, 0, 4, FILLEDRECT, BOX, mocmos2_box17},
	{LM, 0, 4, FILLEDRECT, BOX, mocmos2_box17},
	{LV, 0, 4, CLOSEDRECT, BOX, mocmos2_box1}};
static TECH_NODES mocmos2_mmc = {
	x_("Metal_1_Metal_2_Con"), NMMC, NONODEPROTO,
	K4, K4,
	1, mocmos2_mmc_p,
	3, mocmos2_mmc_l,
	(NPCONTACT<<NFUNCTIONSH),
	0,0,0,0,0,0,0,0,0};

/* Metal_1_Well_Con */
static TECH_PORTS mocmos2_mwc_p[] = {
	{mocmos2_pc_2, x_("metal-1-well"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN5H, BOTIN5H, RIGHTIN5H, TOPIN5H}};
static TECH_POLYGON mocmos2_mwc_l[] = {
	{LM, 0, 4, FILLEDRECT, MINBOX, mocmos2_box11},
	{LSAW, 0, 4, FILLEDRECT, BOX, mocmos2_box14},
	{LPW, 0, 4, FILLEDRECT, BOX, mocmos2_box17},
	{LPS, 0, 4, CLOSEDRECT, MINBOX, mocmos2_box11},
	{LAC, 0, 4, FILLEDRECT, BOX, mocmos2_box12}};
static TECH_NODES mocmos2_mwc = {
	x_("Metal_1_Well_Con"), NMWC, NONODEPROTO,
	K14, K14,
	1, mocmos2_mwc_p,
	5, mocmos2_mwc_l,
	(NPWELL<<NFUNCTIONSH),
	MULTICUT,K2,K2,K2,K2,0,0,0,0};

/* Metal_1_Substrate_Con */
static TECH_PORTS mocmos2_msc_p[] = {
	{mocmos2_pc_2, x_("metal-1-substrate"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos2_msc_l[] = {
	{LM, 0, 4, FILLEDRECT, MINBOX, mocmos2_box10},
	{LDA, 0, 4, FILLEDRECT, BOX, mocmos2_box17},
	{LAC, 0, 4, FILLEDRECT, BOX, mocmos2_box12}};
static TECH_NODES mocmos2_msc = {
	x_("Metal_1_Substrate_Con"), NMSC, NONODEPROTO,
	K6, K6,
	1, mocmos2_msc_p,
	3, mocmos2_msc_l,
	(NPSUBSTRATE<<NFUNCTIONSH),
	MULTICUT,K2,K2,K2,K2,0,0,0,0};

/* P1_P2_Capacitor */
static TECH_PORTS mocmos2_ppc_p[] = {
	{mocmos2_pc_12, x_("Poly1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, CENTER, TOPEDGE},
	{mocmos2_pc_11, x_("Poly2"), NOPORTPROTO, (180<<PORTARANGESH)|(1<<PORTNETSH),
		CENTER, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos2_ppc_l[] = {
	{LP, -1, 4, FILLEDRECT, BOX, mocmos2_box17},
	{LP0, -1, 4, FILLEDRECT, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_ppc = {
	x_("P1_P2_Capacitor"), NPPC, NONODEPROTO,
	K2, K2,
	2, mocmos2_ppc_p,
	2, mocmos2_ppc_l,
	(NPCAPAC<<NFUNCTIONSH),
	0,0,0,0,0,0,0,0,0};

/* Metal_1_Node */
static TECH_PORTS mocmos2_mn_p[] = {
	{mocmos2_pc_14, x_("metal-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos2_mn_l[] = {
	{LM, 0, 4, FILLEDRECT, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_mn = {
	x_("Metal_1_Node"), NMN, NONODEPROTO,
	K4, K4,
	1, mocmos2_mn_p,
	1, mocmos2_mn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Metal_2_Node */
static TECH_PORTS mocmos2_mn0_p[] = {
	{mocmos2_pc_13, x_("metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos2_mn0_l[] = {
	{LM0, 0, 4, FILLEDRECT, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_mn0 = {
	x_("Metal_2_Node"), NMN0, NONODEPROTO,
	K4, K4,
	1, mocmos2_mn0_p,
	1, mocmos2_mn0_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Polysilicon_Node */
static TECH_PORTS mocmos2_pn_p[] = {
	{mocmos2_pc_12, x_("polysilicon"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON mocmos2_pn_l[] = {
	{LP, 0, 4, FILLEDRECT, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_pn = {
	x_("Polysilicon_Node"), NPN, NONODEPROTO,
	K4, K4,
	1, mocmos2_pn_p,
	1, mocmos2_pn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Polysilicon_2_Node */
static TECH_PORTS mocmos2_pn0_p[] = {
	{mocmos2_pc_11, x_("P2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON mocmos2_pn0_l[] = {
	{LP0, 0, 4, FILLEDRECT, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_pn0 = {
	x_("Polysilicon_2_Node"), NPN0, NONODEPROTO,
	K2, K2,
	1, mocmos2_pn0_p,
	1, mocmos2_pn0_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Active_Node */
static TECH_PORTS mocmos2_an_p[] = {
	{mocmos2_pc_8, x_("active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON mocmos2_an_l[] = {
	{LSA, 0, 4, FILLEDRECT, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_an = {
	x_("Active_Node"), NAN, NONODEPROTO,
	K4, K4,
	1, mocmos2_an_p,
	1, mocmos2_an_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* D_Active_Node */
static TECH_PORTS mocmos2_dan_p[] = {
	{mocmos2_pc_8, x_("active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON mocmos2_dan_l[] = {
	{LDA, 0, 4, FILLEDRECT, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_dan = {
	x_("D_Active_Node"), NDAN, NONODEPROTO,
	K4, K4,
	1, mocmos2_dan_p,
	1, mocmos2_dan_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* P_Select_Node */
static TECH_PORTS mocmos2_psn_p[] = {
	{mocmos2_pc_1, x_("select"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos2_psn_l[] = {
	{LPS, 0, 4, CLOSEDRECT, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_psn = {
	x_("P_Select_Node"), NPSN, NONODEPROTO,
	K6, K6,
	1, mocmos2_psn_p,
	1, mocmos2_psn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Cut_Node */
static TECH_PORTS mocmos2_cn_p[] = {
	{mocmos2_pc_1, x_("cut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos2_cn_l[] = {
	{LCC, 0, 4, FILLEDRECT, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_cn = {
	x_("Cut_Node"), NCN, NONODEPROTO,
	K2, K2,
	1, mocmos2_cn_p,
	1, mocmos2_cn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Poly_Cut_Node */
static TECH_PORTS mocmos2_pcn_p[] = {
	{mocmos2_pc_1, x_("polycut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos2_pcn_l[] = {
	{LPC, 0, 4, FILLEDRECT, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_pcn = {
	x_("Poly_Cut_Node"), NPCN, NONODEPROTO,
	K2, K2,
	1, mocmos2_pcn_p,
	1, mocmos2_pcn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Poly_2_Cut_Node */
static TECH_PORTS mocmos2_pcn0_p[] = {
	{mocmos2_pc_11, x_("Poly_2_Cut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos2_pcn0_l[] = {
	{LPC0, 0, 4, FILLEDRECT, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_pcn0 = {
	x_("Poly_2_Cut_Node"), NPCN0, NONODEPROTO,
	K4, K4,
	1, mocmos2_pcn0_p,
	1, mocmos2_pcn0_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0};

/* Active_Cut_Node */
static TECH_PORTS mocmos2_acn_p[] = {
	{mocmos2_pc_1, x_("activecut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos2_acn_l[] = {
	{LAC, 0, 4, FILLEDRECT, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_acn = {
	x_("Active_Cut_Node"), NACN, NONODEPROTO,
	K2, K2,
	1, mocmos2_acn_p,
	1, mocmos2_acn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Via_Node */
static TECH_PORTS mocmos2_vn_p[] = {
	{mocmos2_pc_1, x_("via"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos2_vn_l[] = {
	{LV, 0, 4, CLOSEDRECT, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_vn = {
	x_("Via_Node"), NVN, NONODEPROTO,
	K2, K2,
	1, mocmos2_vn_p,
	1, mocmos2_vn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* P_Well_Node */
static TECH_PORTS mocmos2_pwn_p[] = {
	{mocmos2_pc_10, x_("well"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3}};
static TECH_POLYGON mocmos2_pwn_l[] = {
	{LPW, 0, 4, FILLEDRECT, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_pwn = {
	x_("P_Well_Node"), NPWN, NONODEPROTO,
	K6, K6,
	1, mocmos2_pwn_p,
	1, mocmos2_pwn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Passivation_Node */
static TECH_PORTS mocmos2_pn1_p[] = {
	{mocmos2_pc_1, x_("passivation"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos2_pn1_l[] = {
	{LP1, 0, 4, CLOSEDRECT, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_pn1 = {
	x_("Passivation_Node"), NPN1, NONODEPROTO,
	K8, K8,
	1, mocmos2_pn1_p,
	1, mocmos2_pn1_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

/* Pad_Frame_Node */
static TECH_PORTS mocmos2_pfn_p[] = {
	{mocmos2_pc_1, x_("pad-frame"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos2_pfn_l[] = {
	{LPF, 0, 4, CLOSEDRECT, BOX, mocmos2_box17}};
static TECH_NODES mocmos2_pfn = {
	x_("Pad_Frame_Node"), NPFN, NONODEPROTO,
	K8, K8,
	1, mocmos2_pfn_p,
	1, mocmos2_pfn_l,
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,
	POLYGONAL,0,0,0,0,0,0,0,0};

TECH_NODES *mocmos2_nodeprotos[NODEPROTOCOUNT+1] = {
	&mocmos2_mp, &mocmos2_mp0, &mocmos2_pp, &mocmos2_pp0, &mocmos2_sap,
	&mocmos2_dap, &mocmos2_ap, &mocmos2_msac, &mocmos2_mdac, &mocmos2_mpc,
	&mocmos2_mpc0, &mocmos2_st, &mocmos2_dt, &mocmos2_mmc, &mocmos2_mwc,
	&mocmos2_msc, &mocmos2_ppc, &mocmos2_mn, &mocmos2_mn0, &mocmos2_pn,
	&mocmos2_pn0, &mocmos2_an, &mocmos2_dan, &mocmos2_psn, &mocmos2_cn,
	&mocmos2_pcn, &mocmos2_pcn0, &mocmos2_acn, &mocmos2_vn, &mocmos2_pwn,
	&mocmos2_pn1, &mocmos2_pfn, ((TECH_NODES *)-1)};

static INTBIG mocmos2_node_widoff[NODEPROTOCOUNT*4] = {
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, K2,K2,K2,K2, K4,K4,K4,K4, 0,0,0,0,
	K2,K2,K2,K2, K4,K4,K4,K4, 0,0,0,0, K1,K1,K1,K1, K2,K2,K4,K4, K4,K4,K6,K6, 0,0,0,0,
	K4,K4,K4,K4, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

/******************** VARIABLE AGGREGATION ********************/

TECH_VARIABLES mocmos2_variables[] =
{
	{x_("TECH_layer_names"), (CHAR *)mocmos2_layer_names, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_layer_function"), (CHAR *)mocmos2_layer_function, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_node_width_offset"), (CHAR *)mocmos2_node_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|((NODEPROTOCOUNT*4)<<VLENGTHSH)},
	{x_("TECH_arc_width_offset"), (CHAR *)mocmos2_arc_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|(ARCPROTOCOUNT<<VLENGTHSH)},
	{x_("USER_layer_letters"), (CHAR *)mocmos2_layer_letters, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("USER_color_map"), (CHAR *)mocmos2_colmap, 0.0,
		VCHAR|VDONTSAVE|VISARRAY|((sizeof mocmos2_colmap)<<VLENGTHSH)},
	{x_("IO_cif_layer_names"), (CHAR *)mocmos2_cif_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("IO_gds_layer_numbers"), (CHAR *)mocmos2_gds_layers, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("DRC_min_unconnected_distances"), (CHAR *)mocmos2_unconnectedtable, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|
		   (((sizeof mocmos2_unconnectedtable)/SIZEOFINTBIG)<<VLENGTHSH)},
	{x_("DRC_min_connected_distances"), (CHAR *)mocmos2_connectedtable, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|
			(((sizeof mocmos2_connectedtable)/SIZEOFINTBIG)<<VLENGTHSH)},
	{NULL, NULL, 0.0, 0}
};

/******************** ROUTINES ********************/

BOOLEAN mocmos2_initprocess(TECHNOLOGY *tech, INTBIG pass)
{
	return(FALSE);
}

#endif  /* TECMOCMOS2 - at top */
