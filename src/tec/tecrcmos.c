/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tecrcmos.c
 * Round MOSIS CMOS technology description
 * Written by: Steven M. Rubin, Static Free Software
 * Specified by: Dick Lyon, Carver Mead, and Erwin Liu
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
#include "tecrcmos.h"
#include "efunction.h"

typedef struct
{
	INTBIG list0off, list1off, list2off;
} RCMPOLYLOOP;

RCMPOLYLOOP rcmos_oneprocpolyloop;

static TECHNOLOGY *rcmos_tech;

static INTBIG rcmos_intarcpolys(ARCINST *ai, WINDOWPART *win, POLYLOOP *pl, RCMPOLYLOOP *rcmpl);
static void rcmos_intshapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly, POLYLOOP *pl, RCMPOLYLOOP *rcmpl);

/******************** LAYERS ********************/

#define MAXLAYERS 19		/* total layers below         */
#define LMETAL1    0		/* metal layer 1              */
#define LMETAL2    1		/* metal layer 2              */
#define LPOLY      2		/* polysilicon                */
#define LSACT      3		/* S active (diffusion)       */
#define LDACT      4		/* D active (diffusion)       */
#define LSELECT    5		/* Select                     */
#define LWELL      6		/* Well                       */
#define LCUT       7		/* contact cut                */
#define LVIA       8		/* metal-to-metal via         */
#define LPASS      9		/* passivation (overglass)    */
#define LPOLYCUT  10		/* poly contact cut           */
#define LACTCUT   11		/* active contact cut         */
#define LMET1P    12		/* pseudo metal 1             */
#define LMET2P    13		/* pseudo metal 2             */
#define LPOLYP    14		/* pseudo polysilicon         */
#define LSACTP    15		/* pseudo S active            */
#define LDACTP    16		/* pseudo D active            */
#define LSELECTP  17		/* pseudo Select              */
#define LWELLP    18		/* pseudo Well                */

static GRAPHICS rcmos_m1_lay = {LAYERT1,COLORT1, SOLIDC, PATTERNED,
/* metal-1 layer */		{0x2222, /*   X   X   X   X  */
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
static GRAPHICS rcmos_m2_lay = {LAYERT5,COLORT5, SOLIDC, PATTERNED,
/* metal-2 layer */		{0x1010, /*    X       X     */
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
static GRAPHICS rcmos_p_lay = {LAYERT2,COLORT2, SOLIDC, PATTERNED,
/* poly layer */		{0x0808, /*     X       X    */
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
static GRAPHICS rcmos_sa_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
/* S active layer */	{0x0000, /*                  */
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
static GRAPHICS rcmos_da_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
/* D active layer */	{0x0000, /*                  */
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
static GRAPHICS rcmos_s_lay = {LAYERO,YELLOW, PATTERNED, PATTERNED,
/* Select layer */		{0x1010, /*    X       X     */
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
static GRAPHICS rcmos_w_lay = {LAYERT4,COLORT4, SOLIDC, PATTERNED,
/* Well implant */		{0x1000, /*    X             */
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
static GRAPHICS rcmos_c_lay = {LAYERO,BLACK, SOLIDC, SOLIDC,
/* cut layer */		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS rcmos_v_lay = {LAYERO,BLACK, SOLIDC, SOLIDC,
/* via layer */		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS rcmos_ov_lay = {LAYERO,DGRAY, PATTERNED, PATTERNED,
/* passivation layer */	{0x1C1C, /*    XXX     XXX   */
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
static GRAPHICS rcmos_pc_lay = {LAYERO,BLACK, SOLIDC, SOLIDC,
/* poly cut layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS rcmos_ac_lay = {LAYERO,BLACK, SOLIDC, SOLIDC,
/* active cut layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS rcmos_pm1_lay ={LAYERT1,COLORT1, SOLIDC, PATTERNED,
/* pseudo metal 1 */	{0x2222, /*   X   X   X   X  */
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
static GRAPHICS rcmos_pm2_lay = {LAYERT5,COLORT5, SOLIDC, PATTERNED,
/* pseudo metal-2 */	{0x1010, /*    X       X     */
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
static GRAPHICS rcmos_pp_lay = {LAYERT2,COLORT2, SOLIDC, PATTERNED,
/* pseudo poly layer */	{0x0808, /*     X       X    */
						0x0404,  /*      X       X   */
						0x0202,  /*       X       X  */
						0x0101,  /*        X       X */
						0x8080,  /* X       X        */
						0x4040,  /*  X       X       */
						0x2020,  /*   X       X      */
						0x1010,   /*    X       X     */
						0x0808,  /*     X       X    */
						0x0404,  /*      X       X   */
						0x0202,  /*       X       X  */
						0x0101,  /*        X       X */
						0x8080,  /* X       X        */
						0x4040,  /*  X       X       */
						0x2020,  /*   X       X      */
						0x1010},  /*    X       X     */
						NOVARIABLE, 0};
static GRAPHICS rcmos_psa_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
/* pseudo S active */	{0x0000, /*                  */
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
static GRAPHICS rcmos_pda_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
/* pseudo D active */	{0x0000, /*                  */
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
static GRAPHICS rcmos_ps_lay = {LAYERO,YELLOW, PATTERNED, PATTERNED,
/* pseudo Select */		{0x1010, /*    X       X     */
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
static GRAPHICS rcmos_pw_lay = {LAYERT4,COLORT4, SOLIDC, PATTERNED,
/* pseudo Well */		{0x1000, /*    X             */
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

/* these tables must be updated together */
GRAPHICS *rcmos_layers[MAXLAYERS+1] = {&rcmos_m1_lay, &rcmos_m2_lay,
	&rcmos_p_lay, &rcmos_sa_lay, &rcmos_da_lay, &rcmos_s_lay,
	&rcmos_w_lay, &rcmos_c_lay, &rcmos_v_lay, &rcmos_ov_lay,
	&rcmos_pc_lay, &rcmos_ac_lay, &rcmos_pm1_lay, &rcmos_pm2_lay, &rcmos_pp_lay,
	&rcmos_psa_lay, &rcmos_pda_lay, &rcmos_ps_lay, &rcmos_pw_lay, NOGRAPHICS};
static CHAR *rcmos_layer_names[MAXLAYERS] = {x_("Metal-1"), x_("Metal-2"),
	x_("Polysilicon"), x_("S-Active"), x_("D-Active"), x_("Select"),
	x_("Well"), x_("Contact-Cut"), x_("Via"), x_("Passivation"), x_("Poly-Cut"),
	x_("Active-Cut"), x_("Pseudo-Metal-1"), x_("Pseudo-Metal-2"), x_("Pseudo-Polysilicon"),
	x_("Pseudo-S-Active"), x_("Pseudo-D-Active"), x_("Pseudo-Select"), x_("Pseudo-Well")};
static INTBIG rcmos_layer_function[MAXLAYERS] = {LFMETAL1|LFTRANS1,
	LFMETAL2|LFTRANS5, LFPOLY1|LFTRANS2, LFDIFF|LFPTYPE|LFTRANS3,
	LFDIFF|LFNTYPE|LFTRANS3, LFIMPLANT|LFPTYPE, LFWELL|LFTRANS4, LFCONTACT1,
	LFCONTACT2|LFCONMETAL, LFOVERGLASS, LFCONTACT1|LFCONPOLY, LFCONTACT1|LFCONDIFF,
	LFMETAL1|LFPSEUDO|LFTRANS1, LFMETAL2|LFPSEUDO|LFTRANS5, LFPOLY1|LFPSEUDO|LFTRANS2,
	LFDIFF|LFPTYPE|LFPSEUDO|LFTRANS3, LFDIFF|LFNTYPE|LFPSEUDO|LFTRANS3,
	LFIMPLANT|LFPTYPE|LFPSEUDO, LFWELL|LFPSEUDO|LFTRANS4};
static CHAR *rcmos_cif_layers[MAXLAYERS] = {x_("CMF"), x_("CMS"), x_("CPG"), x_("CAA"),
	x_("CAA"), x_("CSG"), x_("CWG"), x_("CC"), x_("CVA"), x_("COG"), x_("CCP"), x_("CCA"),
	x_(""), x_(""), x_(""), x_(""), x_(""), x_(""), x_("")};
static CHAR *rcmos_layer_letters[MAXLAYERS] = {x_("m"), x_("h"), x_("p"), x_("s"), x_("d"),
	x_("e"), x_("w"), x_("c"), x_("v"), x_("o"), x_("a"), x_("A"), x_("M"), x_("H"), x_("P"), x_("S"), x_("D"), x_("E"), x_("W")};

/* The low 5 bits map Metal-1, Poly, Active, Well, and Metal-2 */
static TECH_COLORMAP rcmos_colmap[32] =
{                  /*     metal2 select active poly metal1 */
	{200,200,200}, /* 0:                                 */
	{ 96,209,255}, /* 1:                          metal1 */
	{255,155,192}, /* 2:                     poly        */
	{ 96,127,192}, /* 3:                     poly+metal1 */
	{107,226, 96}, /* 4:              active             */
	{ 40,186, 96}, /* 5:              active+     metal1 */
	{107,137, 72}, /* 6:              active+poly        */
	{ 40,113, 72}, /* 7:              active+poly+metal1 */
	{240,221,181}, /* 8:         well                    */
	{ 91,182,181}, /* 9:         well+            metal1 */
	{240,134,136}, /* 10:        well+       poly        */
	{ 91,111,136}, /* 11:        well+       poly+metal1 */
	{101,196, 68}, /* 12:        well+active             */
	{ 38,161, 68}, /* 13:        well+active+     metal1 */
	{101,119, 51}, /* 14:        well+active+poly        */
	{ 38, 98, 51}, /* 15:        well+active+poly+metal1 */
	{224, 95,255}, /* 16: metal2+                        */
	{ 85, 78,255}, /* 17: metal2+                 metal1 */
	{224, 57,192}, /* 18: metal2+            poly        */
	{ 85, 47,192}, /* 19: metal2+            poly+metal1 */
	{ 94, 84, 96}, /* 20: metal2+     active             */
	{ 36, 69, 96}, /* 21: metal2+     active+     metal1 */
	{ 94, 51, 72}, /* 22: metal2+     active+poly        */
	{ 36, 42, 72}, /* 23: metal2+     active+poly+metal1 */
	{211, 82,181}, /* 24: metal2+well                    */
	{ 80, 68,181}, /* 25: metal2+well+            metal1 */
	{211, 50,136}, /* 26: metal2+well+       poly        */
	{ 80, 41,136}, /* 27: metal2+well+       poly+metal1 */
	{ 89, 73, 68}, /* 28: metal2+well+active             */
	{ 33, 60, 68}, /* 29: metal2+well+active+     metal1 */
	{ 89, 44, 51}, /* 30: metal2+well+active+poly        */
	{ 33, 36, 51}  /* 31: metal2+well+active+poly+metal1 */
};

/******************** ARCS ********************/

#define ARCPROTOCOUNT  9
#define AMETAL1        0	/* metal-1                   */
#define AMETAL2        1	/* metal-2                   */
#define APOLY          2	/* polysilicon               */
#define ASACT          3	/* S-active                  */
#define ADACT          4	/* D-active                  */
#define ASUACT         5	/* Substrate active          */
#define AWEACT         6	/* Well active               */
#define ASTRANS        7	/* S-transistor              */
#define ADTRANS        8	/* D-transistor              */

/* metal 1 arc */
static TECH_ARCLAY rcmos_al_m1[] = {{LMETAL1,0,FILLED }};
static TECH_ARCS rcmos_a_m1 = {
	x_("Metal-1"),K3,AMETAL1,NOARCPROTO,								/* name */
	1,rcmos_al_m1,														/* layers */
	(APMETAL1<<AFUNCTIONSH)|WANTNOEXTEND|CANCURVE|(0<<AANGLEINCSH)};	/* userbits */

/* metal 2 arc */
static TECH_ARCLAY rcmos_al_m2[] = {{LMETAL2,0,FILLED }};
static TECH_ARCS rcmos_a_m2 = {
	x_("Metal-2"),K3,AMETAL2,NOARCPROTO,								/* name */
	1,rcmos_al_m2,														/* layers */
	(APMETAL2<<AFUNCTIONSH)|WANTNOEXTEND|CANCURVE|(0<<AANGLEINCSH)};	/* userbits */

/* polysilicon arc */
static TECH_ARCLAY rcmos_al_p[] = {{LPOLY,0,FILLED }};
static TECH_ARCS rcmos_a_po = {
	x_("Polysilicon"),K2,APOLY,NOARCPROTO,								/* name */
	1,rcmos_al_p,														/* layers */
	(APPOLY1<<AFUNCTIONSH)|WANTNOEXTEND|CANCURVE|(0<<AANGLEINCSH)};		/* userbits */

/* S-active arc */
static TECH_ARCLAY rcmos_al_sa[] = {{LSACT,K4,FILLED}, {LSELECT,0,FILLED}};
static TECH_ARCS rcmos_a_sa = {
	x_("S-Active"),K6,ASACT,NOARCPROTO,									/* name */
	2,rcmos_al_sa,														/* layers */
	(APDIFFN<<AFUNCTIONSH)|WANTNOEXTEND|CANCURVE|(0<<AANGLEINCSH)};		/* userbits */

/* D-active arc */
static TECH_ARCLAY rcmos_al_da[] = {{LDACT,K8,FILLED}, {LWELL,0,FILLED}};
static TECH_ARCS rcmos_a_da = {
	x_("D-Active"),K10,ADACT,NOARCPROTO,								/* name */
	2,rcmos_al_da,														/* layers */
	(APDIFFP<<AFUNCTIONSH)|WANTNOEXTEND|CANCURVE|(0<<AANGLEINCSH)};		/* userbits */

/* Substrate active arc */
static TECH_ARCLAY rcmos_al_sua[] = {{LDACT,0,FILLED}, {LSACT,0,FILLED}};
static TECH_ARCS rcmos_a_sua = {
	x_("Substrate-Active"),K2,ASUACT,NOARCPROTO,						/* name */
	2,rcmos_al_sua,														/* layers */
	(APDIFFS<<AFUNCTIONSH)|WANTNOEXTEND|CANCURVE|(0<<AANGLEINCSH)};		/* userbits */

/* Well active arc */
static TECH_ARCLAY rcmos_al_wea[] = {{LDACT,K4,FILLED}, {LSACT,K4,FILLED},
	{LWELL,0,FILLED}, {LSELECT,K2,FILLED}};
static TECH_ARCS rcmos_a_wea = {
	x_("Well-Active"),K6,AWEACT,NOARCPROTO,								/* name */
	4,rcmos_al_wea,														/* layers */
	(APDIFFW<<AFUNCTIONSH)|WANTNOEXTEND|CANCURVE|(0<<AANGLEINCSH)};		/* userbits */

/* S-transistor arc */
static TECH_ARCLAY rcmos_al_st[] = {{LDACT,K4,FILLED}, {LPOLY,K4,FILLED},
	{LSELECT,0,FILLED}};
static TECH_ARCS rcmos_a_st = {
	x_("S-Transistor"),K6,ASTRANS,NOARCPROTO,							/* name */
	3,rcmos_al_st,														/* layers */
	(APDIFFN<<AFUNCTIONSH)|WANTNOEXTEND|CANCURVE|(0<<AANGLEINCSH)};		/* userbits */

/* D-transistor arc */
static TECH_ARCLAY rcmos_al_dt[] = {{LDACT,K8,FILLED}, {LPOLY,K8,FILLED},
	{LWELL,0,FILLED}};
static TECH_ARCS rcmos_a_dt = {
	x_("D-Transistor"),K10,ADTRANS,NOARCPROTO,							/* name */
	3,rcmos_al_dt,														/* layers */
	(APDIFFP<<AFUNCTIONSH)|WANTNOEXTEND|CANCURVE|(0<<AANGLEINCSH)};		/* userbits */

TECH_ARCS *rcmos_arcprotos[ARCPROTOCOUNT+1] = {
	&rcmos_a_m1, &rcmos_a_m2, &rcmos_a_po, &rcmos_a_sa, &rcmos_a_da,
	&rcmos_a_sua, &rcmos_a_wea, &rcmos_a_st, &rcmos_a_dt, ((TECH_ARCS *)-1)};

static INTBIG rcmos_arc_widoff[ARCPROTOCOUNT] = {0,0,0,K4,K8,0,K4,K4,K8};

/******************** PORTINST CONNECTIONS ********************/

/* these values are replaced with actual arcproto addresses */
static INTBIG rcmos_pc_m1[]   = {-1, AMETAL1, ALLGEN, -1};
static INTBIG rcmos_pc_m2[]   = {-1, AMETAL2, ALLGEN, -1};
static INTBIG rcmos_pc_p[]    = {-1, APOLY, ALLGEN, -1};
static INTBIG rcmos_pc_st[]   = {-1, ASTRANS, ASACT, APOLY, ALLGEN, -1};
static INTBIG rcmos_pc_dt[]   = {-1, ADTRANS, ADACT, APOLY, ALLGEN, -1};
static INTBIG rcmos_pc_sa[]   = {-1, ASACT, ALLGEN, -1};
static INTBIG rcmos_pc_da[]   = {-1, ADACT, ALLGEN, -1};
static INTBIG rcmos_pc_sua[]  = {-1, ASUACT, ALLGEN, -1};
static INTBIG rcmos_pc_wea[]  = {-1, AWEACT, ALLGEN, -1};
static INTBIG rcmos_pc_suam1[]= {-1, AMETAL1, ASUACT, ALLGEN, -1};
static INTBIG rcmos_pc_weam1[]= {-1, AMETAL1, AWEACT, ALLGEN, -1};
static INTBIG rcmos_pc_sam1[] = {-1, ASACT, AMETAL1, ALLGEN, -1};
static INTBIG rcmos_pc_dam1[] = {-1, ADACT, AMETAL1, ALLGEN, -1};
static INTBIG rcmos_pc_pm1[]  = {-1, APOLY, AMETAL1, ALLGEN, -1};
static INTBIG rcmos_pc_mm[]   = {-1, AMETAL1, AMETAL2, ALLGEN, -1};
static INTBIG rcmos_pc_null[] = {-1, ALLGEN, -1};

/******************** NODES ********************/

#define NODEPROTOCOUNT 26
#define NMETAL1P        1	/* metal-1 pin */
#define NMETAL2P        2	/* metal-2 pin */
#define NPOLYP          3	/* polysilicon pin */
#define NSACTP          4	/* S-active pin */
#define NDACTP          5	/* D-active pin */
#define NSUACTP         6	/* Substrate active pin */
#define NWEACTP         7	/* Well active pin */
#define NSTRANSP        8	/* S-transistor pin */
#define NDTRANSP        9	/* D-transistor pin */
#define NMETSACTC      10	/* metal-1-S-active contact */
#define NMETDACTC      11	/* metal-1-D-active contact */
#define NMETPOLYC      12	/* metal-1-polysilicon contact */
#define NVIA           13	/* metal-1-metal-2 contact */
#define NWBUT          14	/* metal-1-Well contact */
#define NSBUT          15	/* metal-1-Substrate contact */
#define NMETAL1N       16	/* metal-1 node */
#define NMETAL2N       17	/* metal-2 node */
#define NPOLYN         18	/* polysilicon node */
#define NACTIVEN       19	/* active node */
#define NSELECTN       20	/* select node */
#define NCUTN          21	/* cut node */
#define NPCUTN         22	/* poly cut node */
#define NACUTN         23	/* active cut node */
#define NVIAN          24	/* via node */
#define NWELLN         25	/* well node */
#define NPASSN         26	/* passivation node */

/* for geometry */
static INTBIG rcmos_fullbox[8] = {LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE};
static INTBIG rcmos_2round[8]  = {CENTER,   CENTER,  CENTERR1,  CENTER};
static INTBIG rcmos_4round[8]  = {CENTER,   CENTER,  CENTERR2,  CENTER};
static INTBIG rcmos_fullround[8]={CENTER,   CENTER,  RIGHTEDGE, CENTER};
static INTBIG rcmos_in1round[8]= {CENTER,   CENTER,  RIGHTIN1,  CENTER};
static INTBIG rcmos_in2round[8]= {CENTER,   CENTER,  RIGHTIN2,  CENTER};
static INTBIG rcmos_in4round[8]= {CENTER,   CENTER,  RIGHTIN4,  CENTER};
static INTBIG rcmos_tras1[8]   = {CENTER,   CENTER,  CENTER,    TOPIN2};
static INTBIG rcmos_tras2[8]   = {CENTER,   CENTER,  RIGHTIN2,  CENTER};
static INTBIG rcmos_tras3[8]   = {CENTER,   CENTER,  CENTER,    TOPEDGE};
static INTBIG rcmos_tras4[8]   = {CENTER,   CENTER,  CENTER,    TOPIN4};
static INTBIG rcmos_tras5[8]   = {CENTER,   CENTER,  RIGHTIN4,  CENTER};

/* metal-1-pin */
static TECH_PORTS rcmos_pm1_p[] = {				/* ports */
	{rcmos_pc_m1, x_("metal-1"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON rcmos_pm1_l[] = {			/* layers */
	{LMETAL1, 0, 2, DISC, POINTS, rcmos_fullround}};
static TECH_NODES rcmos_pm1 = {
	x_("Metal-1-Pin"),NMETAL1P,NONODEPROTO,		/* name */
	K3,K3,										/* size */
	1,rcmos_pm1_p,								/* ports */
	1,rcmos_pm1_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|NSQUARE,				/* userbits */
	0,0,0,0,0,0,0,0,0};							/* characteristics */

/* metal-2-pin */
static TECH_PORTS rcmos_pm2_p[] = {				/* ports */
	{rcmos_pc_m2, x_("metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON rcmos_pm2_l[] = {			/* layers */
	{LMETAL2, 0, 2, DISC, POINTS, rcmos_fullround}};
static TECH_NODES rcmos_pm2 = {
	x_("Metal-2-Pin"),NMETAL2P,NONODEPROTO,		/* name */
	K3,K3,										/* size */
	1,rcmos_pm2_p,								/* ports */
	1,rcmos_pm2_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|NSQUARE,				/* userbits */
	0,0,0,0,0,0,0,0,0};							/* characteristics */

/* polysilicon-pin */
static TECH_PORTS rcmos_pp_p[] = {				/* ports */
	{rcmos_pc_p, x_("polysilicon"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON rcmos_pp_l[] = {			/* layers */
	{LPOLY,  0, 2, DISC, POINTS, rcmos_fullround}};
static TECH_NODES rcmos_pp = {
	x_("Polysilicon-Pin"),NPOLYP,NONODEPROTO,	/* name */
	K2,K2,										/* size */
	1,rcmos_pp_p,								/* ports */
	1,rcmos_pp_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|NSQUARE,				/* userbits */
	0,0,0,0,0,0,0,0,0};							/* characteristics */

/* S-active-pin */
static TECH_PORTS rcmos_psa_p[] = {				/* ports */
	{rcmos_pc_sa, x_("s-active"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON rcmos_psa_l[] = {			/* layers */
	{LSACT,    0, 2, DISC, POINTS, rcmos_in2round},
	{LSELECT, -1, 2, DISC, POINTS, rcmos_fullround}};
static TECH_NODES rcmos_psa = {
	x_("S-Active-Pin"),NSACTP,NONODEPROTO,		/* name */
	K6,K6,										/* size */
	1,rcmos_psa_p,								/* ports */
	2,rcmos_psa_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|NSQUARE,				/* userbits */
	0,0,0,0,0,0,0,0,0};							/* characteristics */

/* D-active-pin */
static TECH_PORTS rcmos_pda_p[] = {				/* ports */
	{rcmos_pc_da, x_("d-active"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON rcmos_pda_l[] = {			/* layers */
	{LDACT,  0, 2, DISC, POINTS, rcmos_in4round},
	{LWELL, -1, 2, DISC, POINTS, rcmos_fullround}};
static TECH_NODES rcmos_pda = {
	x_("D-Active-Pin"),NDACTP,NONODEPROTO,		/* name */
	K10,K10,									/* size */
	1,rcmos_pda_p,								/* ports */
	2,rcmos_pda_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|NSQUARE,				/* userbits */
	0,0,0,0,0,0,0,0,0};							/* characteristics */

/* Substrate active-pin */
static TECH_PORTS rcmos_psu_p[] = {				/* ports */
	{rcmos_pc_sua, x_("active"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON rcmos_psu_l[] = {			/* layers */
	{LDACT,  0, 2, DISC, POINTS, rcmos_fullround},
	{LSACT,  0, 2, DISC, POINTS, rcmos_fullround}};
static TECH_NODES rcmos_psu = {
	x_("Substrate-Active-Pin"),NSUACTP,NONODEPROTO,	/* name */
	K2,K2,										/* size */
	1,rcmos_psu_p,								/* ports */
	2,rcmos_psu_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|NSQUARE,				/* userbits */
	0,0,0,0,0,0,0,0,0};							/* characteristics */

/* Well active-pin */
static TECH_PORTS rcmos_pwe_p[] = {				/* ports */
	{rcmos_pc_wea, x_("active"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON rcmos_pwe_l[] = {			/* layers */
	{LDACT,  0, 2, DISC, POINTS, rcmos_in2round},
	{LSACT,  0, 2, DISC, POINTS, rcmos_in2round},
	{LWELL,  0, 2, DISC, POINTS, rcmos_fullround},
	{LSELECT,0, 2, DISC, POINTS, rcmos_in1round}};
static TECH_NODES rcmos_pwe = {
	x_("Well-Active-Pin"),NWEACTP,NONODEPROTO,	/* name */
	K6,K6,										/* size */
	1,rcmos_pwe_p,								/* ports */
	4,rcmos_pwe_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|NSQUARE,				/* userbits */
	0,0,0,0,0,0,0,0,0};							/* characteristics */

/* S-transistor pin */
static TECH_PORTS rcmos_tsa_p[] = {				/* ports */
	{rcmos_pc_st,  x_("s-trans"),  NOPORTPROTO, (180<<PORTARANGESH)|PORTISOLATED,
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON rcmos_tsa_l[] = {			/* layers */
	{LSACT,   0, 2, DISC,   POINTS, rcmos_tras1},
	{LPOLY,   0, 2, DISC,   POINTS, rcmos_tras2},
	{LSELECT,-1, 2, DISC,   POINTS, rcmos_tras3}};
static TECH_NODES rcmos_tsa = {
	x_("S-Transistor"),NSTRANSP,NONODEPROTO,	/* name */
	K6,K6,										/* size */
	1,rcmos_tsa_p,								/* ports */
	3,rcmos_tsa_l,								/* layers */
	(NPPIN<<NFUNCTIONSH),						/* userbits */
	0,0,0,0,0,0,0,0,0};							/* characteristics */

/* D-transistor */
static TECH_PORTS rcmos_tda_p[] = {				/* ports */
	{rcmos_pc_dt,  x_("d-trans"),  NOPORTPROTO, (180<<PORTARANGESH)|PORTISOLATED,
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON rcmos_tda_l[] = {			/* layers */
	{LDACT,   0, 2, DISC,   POINTS, rcmos_tras4},
	{LPOLY,   0, 2, DISC,   POINTS, rcmos_tras5},
	{LWELL,  -1, 2, DISC,   POINTS, rcmos_tras3}};
static TECH_NODES rcmos_tda = {
	x_("D-Transistor"),NDTRANSP,NONODEPROTO,	/* name */
	K10,K10,									/* size */
	1,rcmos_tda_p,								/* ports */
	3,rcmos_tda_l,								/* layers */
	(NPPIN<<NFUNCTIONSH),						/* userbits */
	0,0,0,0,0,0,0,0,0};							/* characteristics */

/* metal-1-S-active-contact */
static TECH_PORTS rcmos_msa_p[] = {				/* ports */
	{rcmos_pc_sam1, x_("metal-1-s-act"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON rcmos_msa_l[] = {			/* layers */
	{LMETAL1, 0, 2, DISC, POINTS, rcmos_4round},
	{LSACT,   0, 2, DISC, POINTS, rcmos_in2round},
	{LSELECT, 0, 2, DISC, POINTS, rcmos_fullround},
	{LACTCUT, 0, 2, DISC, POINTS, rcmos_2round}};
static TECH_NODES rcmos_msa = {
	x_("Metal-1-S-Active-Con"),NMETSACTC,NONODEPROTO,/* name */
	K10,K10,									/* size */
	1,rcmos_msa_p,								/* ports */
	4,rcmos_msa_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH)|NSQUARE,			/* userbits */
	0,0,0,0,0,0,0,0,0};							/* characteristics */

/* metal-1-D-active-contact */
static TECH_PORTS rcmos_mda_p[] = {				/* ports */
	{rcmos_pc_dam1, x_("metal-1-d-act"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON rcmos_mda_l[] = {			/* layers */
	{LMETAL1, 0, 2, DISC, POINTS, rcmos_4round},
	{LDACT,   0, 2, DISC, POINTS, rcmos_in4round},
	{LWELL,  -1, 2, DISC, POINTS, rcmos_fullround},
	{LACTCUT, 0, 2, DISC, POINTS, rcmos_2round}};
static TECH_NODES rcmos_mda = {
	x_("Metal-1-D-Active-Con"),NMETDACTC,NONODEPROTO,/* name */
	K14,K14,									/* size */
	1,rcmos_mda_p,								/* ports */
	4,rcmos_mda_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH)|NSQUARE,			/* userbits */
	0,0,0,0,0,0,0,0,0};							/* characteristics */

/* metal-1-polysilicon-contact */
static TECH_PORTS rcmos_mp_p[] = {				/* ports */
	{rcmos_pc_pm1, x_("metal-1-polysilicon"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON rcmos_mp_l[] = {			/* layers */
	{LMETAL1,  0, 2, DISC, POINTS, rcmos_4round},
	{LPOLY,    0, 2, DISC, POINTS, rcmos_fullround},
	{LPOLYCUT, 0, 2, DISC, POINTS, rcmos_2round}};
static TECH_NODES rcmos_mp = {
	x_("Metal-1-Polysilicon-Con"),NMETPOLYC,NONODEPROTO,/* name */
	K6,K6,										/* size */
	1,rcmos_mp_p,								/* ports */
	3,rcmos_mp_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH)|NSQUARE,			/* userbits */
	0,0,0,0,0,0,0,0,0};							/* characteristics */

/* metal-1-metal-2-contact */
static TECH_PORTS rcmos_mm_p[] = {				/* ports */
	{rcmos_pc_mm, x_("metal-1-metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON rcmos_mm_l[] = {			/* layers */
	{LMETAL1, 0, 2, DISC,   POINTS, rcmos_fullround},
	{LMETAL2, 0, 2, DISC,   POINTS, rcmos_fullround},
	{LVIA,    0, 2, CIRCLE, POINTS, rcmos_2round}};
static TECH_NODES rcmos_mm = {
	x_("Metal-1-Metal-2-Con"),NVIA,NONODEPROTO,	/* name */
	K4,K4,										/* size */
	1,rcmos_mm_p,								/* ports */
	3,rcmos_mm_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH)|NSQUARE,			/* userbits */
	0,0,0,0,0,0,0,0,0};							/* characteristics */

/* Metal-1-Well Contact */
static TECH_PORTS rcmos_psub_p[] = {			/* ports */
	{rcmos_pc_weam1, x_("metal-1-well"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON rcmos_psub_l[] = {			/* layers */
	{LMETAL1, 0, 2, DISC, POINTS, rcmos_4round},
	{LSACT,   0, 2, DISC, POINTS, rcmos_in2round},
	{LWELL,  -1, 2, DISC, POINTS, rcmos_fullround},
	{LSELECT,-1, 2, DISC, POINTS, rcmos_in2round},
	{LACTCUT, 0, 2, DISC, POINTS, rcmos_2round}};
static TECH_NODES rcmos_psub = {
	x_("Metal-1-Well-Con"),NWBUT,NONODEPROTO,	/* name */
	K10,K10,									/* size */
	1,rcmos_psub_p,								/* ports */
	5,rcmos_psub_l,								/* layers */
	(NPWELL<<NFUNCTIONSH)|NSQUARE,				/* userbits */
	0,0,0,0,0,0,0,0,0};							/* characteristics */

/* Metal-1-Substrate Contact */
static TECH_PORTS rcmos_nsub_p[] = {			/* ports */
	{rcmos_pc_suam1, x_("metal-1-substrate"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON rcmos_nsub_l[] = {			/* layers */
	{LMETAL1, 0, 2, DISC, POINTS, rcmos_4round},
	{LDACT,   0, 2, DISC, POINTS, rcmos_fullround},
	{LACTCUT, 0, 2, DISC, POINTS, rcmos_2round}};
static TECH_NODES rcmos_nsub = {
	x_("Metal-1-Substrate-Con"),NSBUT,NONODEPROTO,	/* name */
	K6,K6,										/* size */
	1,rcmos_nsub_p,								/* ports */
	3,rcmos_nsub_l,								/* layers */
	(NPSUBSTRATE<<NFUNCTIONSH)|NSQUARE,			/* userbits */
	0,0,0,0,0,0,0,0,0};							/* characteristics */

/* Metal-1-Node */
static TECH_PORTS rcmos_m1_p[] = {				/* ports */
	{rcmos_pc_m1, x_("metal-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON rcmos_m1_l[] = {			/* layers */
	{LMETAL1, 0, 4, FILLEDRECT, BOX, rcmos_fullbox}};
static TECH_NODES rcmos_m1 = {
	x_("Metal-1-Node"),NMETAL1N,NONODEPROTO,	/* name */
	K4,K4,										/* size */
	1,rcmos_m1_p,								/* ports */
	1,rcmos_m1_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,			/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};					/* characteristics */

/* Metal-2-Node */
static TECH_PORTS rcmos_m2_p[] = {				/* ports */
	{rcmos_pc_m2, x_("metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON rcmos_m2_l[] = {			/* layers */
	{LMETAL2, 0, 4, FILLEDRECT, BOX, rcmos_fullbox}};
static TECH_NODES rcmos_m2 = {
	x_("Metal-2-Node"),NMETAL2N,NONODEPROTO,	/* name */
	K4,K4,										/* size */
	1,rcmos_m2_p,								/* ports */
	1,rcmos_m2_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,			/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};					/* characteristics */

/* Polysilicon-Node */
static TECH_PORTS rcmos_p_p[] = {				/* ports */
	{rcmos_pc_p, x_("polysilicon"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON rcmos_p_l[] = {				/* layers */
	{LPOLY, 0, 4, FILLEDRECT, BOX, rcmos_fullbox}};
static TECH_NODES rcmos_p = {
	x_("Polysilicon-Node"),NPOLYN,NONODEPROTO,	/* name */
	K4,K4,										/* size */
	1,rcmos_p_p,								/* ports */
	1,rcmos_p_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,			/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};					/* characteristics */

/* Active-Node */
static TECH_PORTS rcmos_a_p[] = {				/* ports */
	{rcmos_pc_null, x_("active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON rcmos_a_l[] = {				/* layers */
	{LSACT, 0, 4, FILLEDRECT, BOX, rcmos_fullbox}};
static TECH_NODES rcmos_a = {
	x_("Active-Node"),NACTIVEN,NONODEPROTO,		/* name */
	K4,K4,										/* size */
	1,rcmos_a_p,								/* ports */
	1,rcmos_a_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,			/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};					/* characteristics */

/* Select-Node */
static TECH_PORTS rcmos_s_p[] = {				/* ports */
	{rcmos_pc_null, x_("select"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON rcmos_s_l[] = {				/* layers */
	{LSELECT, 0, 4, FILLEDRECT, BOX, rcmos_fullbox}};
static TECH_NODES rcmos_s = {
	x_("Select-Node"),NSELECTN,NONODEPROTO,		/* name */
	K6,K6,										/* size */
	1,rcmos_s_p,								/* ports */
	1,rcmos_s_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,			/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};					/* characteristics */

/* Cut-Node */
static TECH_PORTS rcmos_c_p[] = {				/* ports */
	{rcmos_pc_null, x_("cut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON rcmos_c_l[] = {				/* layers */
	{LCUT, 0, 4, FILLEDRECT, BOX, rcmos_fullbox}};
static TECH_NODES rcmos_c = {
	x_("Cut-Node"),NCUTN,NONODEPROTO,			/* name */
	K2,K2,										/* size */
	1,rcmos_c_p,								/* ports */
	1,rcmos_c_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,			/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};					/* characteristics */

/* PolyCut-Node */
static TECH_PORTS rcmos_gc_p[] = {				/* ports */
	{rcmos_pc_null, x_("polycut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON rcmos_gc_l[] = {			/* layers */
	{LPOLYCUT, 0, 4, FILLEDRECT, BOX, rcmos_fullbox}};
static TECH_NODES rcmos_gc = {
	x_("Poly-Cut-Node"),NPCUTN,NONODEPROTO,		/* name */
	K2,K2,										/* size */
	1,rcmos_gc_p,								/* ports */
	1,rcmos_gc_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,			/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};					/* characteristics */

/* ActiveCut-Node */
static TECH_PORTS rcmos_ac_p[] = {				/* ports */
	{rcmos_pc_null, x_("activecut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON rcmos_ac_l[] = {			/* layers */
	{LACTCUT, 0, 4, FILLEDRECT, BOX, rcmos_fullbox}};
static TECH_NODES rcmos_ac = {
	x_("Active-Cut-Node"),NACUTN,NONODEPROTO,	/* name */
	K2,K2,										/* size */
	1,rcmos_ac_p,								/* ports */
	1,rcmos_ac_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,			/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};					/* characteristics */

/* Via-Node */
static TECH_PORTS rcmos_v_p[] = {				/* ports */
	{rcmos_pc_null, x_("via"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON rcmos_v_l[] = {				/* layers */
	{LVIA, 0, 4, CLOSEDRECT, BOX, rcmos_fullbox}};
static TECH_NODES rcmos_v = {
	x_("Via-Node"),NVIAN,NONODEPROTO,			/* name */
	K2,K2,										/* size */
	1,rcmos_v_p,								/* ports */
	1,rcmos_v_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,			/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};					/* characteristics */

/* Well-Node */
static TECH_PORTS rcmos_w_p[] = {				/* ports */
	{rcmos_pc_sa, x_("well"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3}};
static TECH_POLYGON rcmos_w_l[] = {				/* layers */
	{LWELL, 0, 4, FILLEDRECT, BOX, rcmos_fullbox}};
static TECH_NODES rcmos_w = {
	x_("Well-Node"),NWELLN,NONODEPROTO,			/* name */
	K6,K6,										/* size */
	1,rcmos_w_p,								/* ports */
	1,rcmos_w_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,			/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};					/* characteristics */

/* Passivation-node */
static TECH_PORTS rcmos_o_p[] = {				/* ports */
	{rcmos_pc_null, x_("passivation"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON rcmos_o_l[] = {				/* layers */
	{LPASS, 0, 4, FILLEDRECT, BOX, rcmos_fullbox}};
static TECH_NODES rcmos_o = {
	x_("Passivation-Node"),NPASSN,NONODEPROTO,	/* name */
	K8,K8,										/* size */
	1,rcmos_o_p,								/* ports */
	1,rcmos_o_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,			/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};					/* characteristics */

TECH_NODES *rcmos_nodeprotos[NODEPROTOCOUNT+1] = {
	&rcmos_pm1, &rcmos_pm2, &rcmos_pp, &rcmos_psa, &rcmos_pda,
	&rcmos_psu, &rcmos_pwe,
	&rcmos_tsa, &rcmos_tda,
	&rcmos_msa, &rcmos_mda, &rcmos_mp,
	&rcmos_mm,
	&rcmos_psub, &rcmos_nsub,
	&rcmos_m1, &rcmos_m2, &rcmos_p, &rcmos_a, &rcmos_s,
	&rcmos_c, &rcmos_gc, &rcmos_ac, &rcmos_v, &rcmos_w, &rcmos_o, ((TECH_NODES *)-1)};

/* this table must correspond with the above table */
static INTBIG rcmos_node_widoff[NODEPROTOCOUNT*4] = {
	0,0,0,0, 0,0,0,0, 0,0,0,0, K2,K2,K2,K2, K4,K4,K4,K4,	/* pins */
	0,0,0,0, K2,K2,K2,K2,
	K2,K2,K2,K2, K4,K4,K4,K4,								/* tran pin */
	K2,K2,K2,K2, K4,K4,K4,K4, 0,0,0,0,						/* contacts */
	0,0,0,0,												/* vias */
	K2,K2,K2,K2, 0,0,0,0,									/* buttons */
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,			/* nodes */
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};


/******************** SIMULATION VARIABLES ********************/

/* for SPICE simulation */
#define RCMOS_MIN_RESIST	50.0f	/* minimum resistance consider */
#define RCMOS_MIN_CAPAC	     0.04f	/* minimum capacitance consider */
static float rcmos_sim_spice_resistance[MAXLAYERS] = {  /* per square micron */
	0.03f /* METAL1 */,    0.03f /* METAL2 */,    50.0f /* POLY */,
	10.0f /* SACT */,      10.0f /* DACT */,
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
static float rcmos_sim_spice_capacitance[MAXLAYERS] = { /* per square micron */
	0.03f /* METAL1 */,    0.03f /* METAL2 */,    0.04f /* POLY */,
	0.1f /* SACT */,       0.1f /* DACT */,
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
static CHAR *rcmos_sim_spice_header_level1[] = {
	x_("*CMOS/BULK-NWELL (PRELIMINARY PARAMETERS)"),
	x_(".OPTIONS NOMOD DEFL=3UM DEFW=3UM DEFAD=70P DEFAS=70P LIMPTS=1000"),
	x_("+ITL4=1000 ITL5=0 RELTOL=0.01 ABSTOL=500PA VNTOL=500UV LVLTIM=2"),
	x_("+LVLCOD=1"),
	x_(".MODEL N NMOS LEVEL=1"),
	x_("+KP=60E-6 VTO=0.7 GAMMA=0.3 LAMBDA=0.05 PHI=0.6"),
	x_("+LD=0.4E-6 TOX=40E-9 CGSO=2.0E-10 CGDO=2.0E-10 CJ=.2MF/M^2"),
	x_(".MODEL P PMOS LEVEL=1"),
	x_("+KP=20E-6 VTO=0.7 GAMMA=0.4 LAMBDA=0.05 PHI=0.6"),
	x_("+LD=0.6E-6 TOX=40E-9 CGSO=3.0E-10 CGDO=3.0E-10 CJ=.2MF/M^2"),
	x_(".MODEL DIFFCAP D CJO=.2MF/M^2"),
	NOSTRING};
static CHAR *rcmos_sim_spice_header_level2[] = {
	x_("* CMOS 3um process parameters from MOSIS run M46M"),
	x_("* CBPEM2 Telmos/Sierracin"),
	x_(".OPTIONS NOMOD DEFL=3UM DEFW=3UM DEFAD=70P DEFAS=70P LIMPTS=1000"),
	x_("+ITL4=1000 ITL5=0 RELTOL=0.01 ABSTOL=500PA VNTOL=500UV LVLTIM=2"),
	x_("+LVLCOD=1"),
	x_(".MODEL P PMOS LEVEL=2 LD=0.51286U TOX=500E-10"),
	x_("+NSUB=2.971614E+14 VTO=-0.844293 KP=1.048805E-5 GAMMA=0.723071"),
	x_("+PHI=0.6 UO=100.0 UEXP=0.145531 UCRIT=18543.6"),
	x_("+DELTA=2.19030 VMAX=100000. XJ=2.583588E-2U LAMBDA=5.274834E-2"),
	x_("+NFS=1.615644E+12 NEFF=1.001E-2 NSS=0. TPG=-1."),
	x_("+RSH=95 CGSO=4E-10 CGDO=4E-10 CJ=2E-4 MJ=0.5 CJSW=4.5E-10 MJSW=0.33"),
	x_(".MODEL N NMOS LEVEL=2 LD=0.245423U TOX=500E-10"),
	x_("+NSUB=1E+16 VTO=0.932797 KP=2.696667E-5 GAMMA=1.28047"),
	x_("+PHI=0.6 UO=381.905 UEXP=1.001E-3 UCRIT=999000"),
	x_("+DELTA=1.47242 VMAX=55346.3 XJ=0.145596U LAMBDA=2.491255E-2"),
	x_("+NFS=3.727796E+11 NEFF=1.001E-2 NSS=0 TPG=1"),
	x_("+RSH=25 CGSO=5.2E-10 CGDO=5.2E-10 CJ=3.2E-4 MJ=0.5 CJSW=9E-10"),
	x_("+MJSW=0.33"),
	NOSTRING};

/******************** VARIABLE AGGREGATION ********************/

TECH_VARIABLES rcmos_variables[] =
{
	/* set general information about the technology */
	{x_("TECH_layer_names"), (CHAR *)rcmos_layer_names, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_layer_function"), (CHAR *)rcmos_layer_function, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_arc_width_offset"), (CHAR *)rcmos_arc_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|(ARCPROTOCOUNT<<VLENGTHSH)},
	{x_("TECH_node_width_offset"), (CHAR *)rcmos_node_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|((NODEPROTOCOUNT*4)<<VLENGTHSH)},

	/* set information for the USER tool */
	{x_("USER_color_map"), (CHAR *)rcmos_colmap, 0.0,
		VCHAR|VDONTSAVE|VISARRAY|((sizeof rcmos_colmap)<<VLENGTHSH)},
	{x_("USER_layer_letters"), (CHAR *)rcmos_layer_letters, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},

			/* set information for the I/O tool */
	{x_("IO_cif_layer_names"), (CHAR *)rcmos_cif_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},

	/* set information for the SIM tool (SPICE) */
	{x_("SIM_spice_min_resistance"), 0, RCMOS_MIN_RESIST, VFLOAT|VDONTSAVE},
	{x_("SIM_spice_min_capacitance"), 0, RCMOS_MIN_CAPAC, VFLOAT|VDONTSAVE},
	{x_("SIM_spice_resistance"), (CHAR *)rcmos_sim_spice_resistance, 0.0,
		VFLOAT|VISARRAY|(MAXLAYERS<<VLENGTHSH)|VDONTSAVE},
	{x_("SIM_spice_capacitance"), (CHAR *)rcmos_sim_spice_capacitance, 0.0,
		VFLOAT|VISARRAY|(MAXLAYERS<<VLENGTHSH)|VDONTSAVE},
	{x_("SIM_spice_header_level1"), (CHAR *)rcmos_sim_spice_header_level1, 0.0,
		VSTRING|VDONTSAVE|VISARRAY},
	{x_("SIM_spice_header_level2"), (CHAR *)rcmos_sim_spice_header_level2, 0.0,
		VSTRING|VDONTSAVE|VISARRAY},
	{NULL, NULL, 0.0, 0}
};

/******************** ROUTINES ********************/

BOOLEAN rcmos_initprocess(TECHNOLOGY *tech, INTBIG pass)
{
	if (pass == 0) rcmos_tech = tech;
	return(FALSE);
}

INTBIG rcmos_arcpolys(ARCINST *ai, WINDOWPART *win)
{
	return(rcmos_intarcpolys(ai, win, &tech_oneprocpolyloop, &rcmos_oneprocpolyloop));
}

INTBIG rcmos_intarcpolys(ARCINST *ai, WINDOWPART *win, POLYLOOP *pl, RCMPOLYLOOP *rcmpl)
{
	REGISTER INTBIG widestp, widestd, i, lambda, pwid, dwid, orip, orid, amt;
	REGISTER INTBIG total, aindex;
	INTBIG lx, ly, hx, hy;
	REGISTER NODEINST *ni;

	if ((ai->userbits&ISNEGATED) != 0) tech_resetnegated(ai);
	aindex = ai->proto->arcindex;
	total = rcmos_arcprotos[aindex]->laycount;
	if (aindex == ASTRANS || aindex == ADTRANS)
	{
		lambda = lambdaofarc(ai);

		/* set defaults for the arc */
		if (aindex == ASTRANS) amt = K4; else amt = K8;
		rcmpl->list0off = amt;	/* diffusion */
		rcmpl->list1off = amt;	/* polysilicon */
		rcmpl->list2off = 0;	/* well/select */

		/* determine the maximum and initial size of the arc */
		widestp = widestd = ai->width - amt * lambda / WHOLE;
		orip = widestp;   orid = widestd;

		/* include the size of the nodes on the ends of the arc */
		for(i=0; i<2; i++)
		{
			ni = ai->end[i].nodeinst;
			if (ni->proto->tech != rcmos_tech) continue;
			nodesizeoffset(ni, &lx, &ly, &hx, &hy);
			lx = ni->lowx+lx;   hx = ni->highx-hx;
			ly = ni->lowy+ly;   hy = ni->highy-hy;
			pwid = hx - lx;
			dwid = hy - ly;
			if (pwid > orip || dwid > orid)
				ttyputmsg(_("Warning: arc %s is too narrow for its %s node"),
					describearcinst(ai), (i==0 ? _("tail") : _("head")));
			if (pwid < widestp) widestp = pwid;
			if (dwid < widestd) widestd = dwid;
		}

		/* set the true size of the arc */
		rcmpl->list0off = (ai->width - widestd) * WHOLE / lambda;
		rcmpl->list1off = (ai->width - widestp) * WHOLE / lambda;
		rcmpl->list2off = rcmos_arcprotos[aindex]->list[0].off - amt;
	}
	total = tech_initcurvedarc(ai, total, pl);

	/* add in displayable variables */
	pl->realpolys = total;
	total += tech_displayableavars(ai, win, pl);
	return(total);
}

void rcmos_shapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly)
{
	rcmos_intshapearcpoly(ai, box, poly, &tech_oneprocpolyloop, &rcmos_oneprocpolyloop);
}

void rcmos_intshapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly, POLYLOOP *pl, RCMPOLYLOOP *rcmpl)
{
	REGISTER INTBIG aindex;
	REGISTER TECH_ARCLAY *thista;

	/* handle displayable variables */
	if (box >= pl->realpolys)
	{
		(void)tech_filldisplayableavar(ai, poly, pl->curwindowpart, 0, pl);
		return;
	}

	/* now handle curved arcs */
	if (!tech_curvedarcpiece(ai, box, poly, rcmos_arcprotos, pl))
	{
		poly->desc = rcmos_layers[poly->layer];
		return;
	}

	/* finally handle straight arcs */
	aindex = ai->proto->arcindex;
	thista = &rcmos_arcprotos[aindex]->list[box];
	if (aindex == ASTRANS || aindex == ADTRANS)
	{
		rcmos_arcprotos[aindex]->list[0].off = rcmpl->list0off;	/* diffusion */
		rcmos_arcprotos[aindex]->list[1].off = rcmpl->list1off;	/* polysilicon */
		rcmos_arcprotos[aindex]->list[2].off = rcmpl->list2off;	/* well/select */
	}
	makearcpoly(ai->length, ai->width-thista->off*lambdaofarc(ai)/WHOLE, ai, poly, thista->style);
	poly->layer = thista->lay;
	poly->desc = rcmos_layers[poly->layer];
}

INTBIG rcmos_allarcpolys(ARCINST *ai, POLYLIST *plist, WINDOWPART *win)
{
	REGISTER INTBIG tot, j;
	POLYLOOP mypl;
	RCMPOLYLOOP myrcmpl;

	mypl.curwindowpart = win;
	tot = rcmos_intarcpolys(ai, win, &mypl, &myrcmpl);
	tot = mypl.realpolys;
	if (ensurepolylist(plist, tot, db_cluster)) return(-1);
	for(j = 0; j < tot; j++)
	{
		rcmos_intshapearcpoly(ai, j, plist->polygons[j], &mypl, &myrcmpl);
	}
	return(tot);
}
