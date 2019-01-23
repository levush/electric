/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tecmocmos.c
 * MOSIS CMOS technology description
 * Written by: Steven M. Rubin, Static Free Software
 * The MOSIS 6-metal, 2-poly submicron rules, with SCMOS and DEEP options
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
#include "tecmocmos.h"
#include "efunction.h"
#include "drc.h"

/* #define CENTERACTIVE 1 */		/* uncomment to extend DRC active to center of transistor */

/* the options table */
static KEYWORD mocmosopt[] =
{
	{x_("2-metal-rules"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("3-metal-rules"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("4-metal-rules"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("5-metal-rules"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("6-metal-rules"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("one-polysilicon"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("two-polysilicon"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("scmos-rules"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("submicron-rules"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("deep-rules"),                 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("full-graphics"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("alternate-active-poly"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("standard-active-poly"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("allow-stacked-vias"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("disallow-stacked-vias"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("stick-display"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("switch-n-and-p"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("show-special-transistors"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("hide-special-transistors"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP mocmos_parse = {mocmosopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("MOSIS CMOS Submicron option"), 0};

typedef struct
{
	NODEPROTO *np[2];
	PORTPROTO *pp[2][4];
	INTBIG     portcount;
} PRIMSWAP;

static PRIMSWAP    mocmos_primswap[7];

typedef struct
{
	INTBIG      arrowbox;

	/* for the vertical PNP transistors */
	INTBIG      numlayers;
	INTBIG      outerhorizcuts;
	INTBIG      outervertcuts;
	INTBIG      innerhorizcuts;
	INTBIG      innervertcuts;
	INTBIG      innergap;
	INTBIG      outergap;

	/* for scalable transistors */
	INTBIG      actcontinset;
	INTBIG      actinset;
	INTBIG      polyinset;
	INTBIG      metcontinset;

	INTBIG      numcuts;
	INTBIG      moscutsize;
	INTBIG      moscutsep;
	INTBIG      moscutbase;

	INTBIG      boxoffset;
	INTBIG      numcontacts;
	BOOLEAN     insetcontacts;
} MOCPOLYLOOP;

static INTBIG      mocmos_state;			/* settings */
       TECHNOLOGY *mocmos_tech;
static MOCPOLYLOOP mocmos_oneprocpolyloop;
       NODEPROTO  *mocmos_metal1poly2prim;
       NODEPROTO  *mocmos_metal1poly12prim;
       NODEPROTO  *mocmos_metal1metal2prim;
       NODEPROTO  *mocmos_metal4metal5prim;
       NODEPROTO  *mocmos_metal5metal6prim;
       NODEPROTO  *mocmos_ptransistorprim;
       NODEPROTO  *mocmos_ntransistorprim;
       NODEPROTO  *mocmos_metal1pwellprim;
       NODEPROTO  *mocmos_metal1nwellprim;
       NODEPROTO  *mocmos_scalablentransprim;
       NODEPROTO  *mocmos_scalableptransprim;
	   INTBIG      mocmos_transcontactkey;

/* prototypes for local routines */
static void    mocmos_setstate(INTBIG newstate);
static void    mocmos_switchnp(void);
static void    mocmos_nodesizeoffset(NODEINST *ni, INTBIG *lx, INTBIG *ly, INTBIG *hx, INTBIG *hy);
static INTBIG  mocmos_arcwidthoffset(ARCINST *ai);
static void    mocmos_setupprimswap(INTBIG index1, INTBIG index2, PRIMSWAP *swap);
static void    mocmos_setlayerminwidth(CHAR *layername, INTBIG minwidth);
static void    mocmos_setdefnodesize(TECH_NODES *nty, INTBIG index, INTBIG wid, INTBIG hei);
static void    mocmos_setlayersurroundvia(TECH_NODES *nty, INTBIG layer, INTBIG surround);
static void    mocmos_setarclayersurroundlayer(TECH_ARCS *aty, INTBIG outerlayer, INTBIG innerlayer, INTBIG surround);
static void    mocmos_setlayersurroundlayer(TECH_NODES *nty, INTBIG outerlayer, INTBIG innerlayer, INTBIG surround, INTBIG *minsize);
static void    mocmos_settransistoractiveoverhang(INTBIG overhang);
static void    mocmos_settransistorpolyoverhang(INTBIG overhang);
static void    mocmos_settransistorwellsurround(INTBIG overhang);
static INTBIG  mocmos_arcpolys(ARCINST *ai, WINDOWPART *win);
static void    mocmos_shapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly);
static INTBIG  mocmos_allarcpolys(ARCINST *ai, POLYLIST *plist, WINDOWPART *win);
static BOOLEAN mocmos_loadDRCtables(void);
static CHAR   *mocmos_describestate(INTBIG state);
static INTBIG  mocmos_intnodepolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win, POLYLOOP *pl, MOCPOLYLOOP *mocpl);
static void    mocmos_intshapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl, MOCPOLYLOOP *mocpl);
static INTBIG  mocmos_intarcpolys(ARCINST *ai, WINDOWPART *win, POLYLOOP *pl, MOCPOLYLOOP *mocpl);
static void    mocmos_intshapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly, POLYLOOP *pl, MOCPOLYLOOP *mocpl);
static INTBIG  mocmos_intEnodepolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win, POLYLOOP *pl, MOCPOLYLOOP *mocpl);
static void    mocmos_intshapeEnodepoly(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl, MOCPOLYLOOP *mocpl);
static INTBIG  mocmos_initializescalabletransistor(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win, POLYLOOP *pl, MOCPOLYLOOP *mocpl);
static void    mocmos_iteratescalabletransistor(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl, MOCPOLYLOOP *mocpl);
static INTBIG  mocmos_initializevpnptransistor(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win, POLYLOOP *pl, MOCPOLYLOOP *mocpl);
static void    mocmos_iteratevpnptransistor(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl, MOCPOLYLOOP *mocpl);
static void    mocmos_fillring(NODEINST *ni, INTBIG box, POLYGON *poly, INTBIG lambda, INTBIG x, INTBIG y, INTBIG xs, INTBIG ys, INTBIG inner, INTBIG outer, INTBIG gap);
static void    mocmos_fillcut(NODEINST *ni, INTBIG boxnum, INTBIG boxesonside, POLYGON *poly, INTBIG lambda,
					INTBIG x, INTBIG y, INTBIG xside, INTBIG yside);

/******************** LAYERS ********************/

#define MAXLAYERS   41		/* total layers below         */
#define LMETAL1      0		/* metal layer 1              */
#define LMETAL2      1		/* metal layer 2              */
#define LMETAL3      2		/* metal layer 3              */
#define LMETAL4      3		/* metal layer 4              */
#define LMETAL5      4		/* metal layer 5              */
#define LMETAL6      5		/* metal layer 6              */
#define LPOLY1       6		/* polysilicon                */
#define LPOLY2       7		/* polysilicon 2 (electrode)  */
#define LPACT        8		/* P active                   */
#define LNACT        9		/* N active                   */
#define LSELECTP    10		/* P-type select              */
#define LSELECTN    11		/* N-type select              */
#define LWELLP      12		/* P-type well                */
#define LWELLN      13		/* N-type well                */
#define LPOLYCUT    14		/* poly contact cut           */
#define LACTCUT     15		/* active contact cut         */
#define LVIA1       16		/* metal1-to-metal2 via       */
#define LVIA2       17		/* metal2-to-metal3 via       */
#define LVIA3       18		/* metal3-to-metal4 via       */
#define LVIA4       19		/* metal4-to-metal5 via       */
#define LVIA5       20		/* metal5-to-metal6 via       */
#define LPASS       21		/* passivation (overglass)    */
#define LTRANS      22		/* transistor polysilicon     */
#define LPOLYCAP    23		/* polysilicon capacitor      */
#define LPACTWELL   24		/* P active in well           */
#define LSILBLOCK   25		/* Silicide block             */
#define LMET1P      26		/* pseudo metal 1             */
#define LMET2P      27		/* pseudo metal 2             */
#define LMET3P      28		/* pseudo metal 3             */
#define LMET4P      29		/* pseudo metal 4             */
#define LMET5P      30		/* pseudo metal 5             */
#define LMET6P      31		/* pseudo metal 6             */
#define LPOLY1P     32		/* pseudo polysilicon 1       */
#define LPOLY2P     33		/* pseudo polysilicon 2       */
#define LPACTP      34		/* pseudo P active            */
#define LNACTP      35		/* pseudo N active            */
#define LSELECTPP   36		/* pseudo P-type select       */
#define LSELECTNP   37		/* pseudo N-type select       */
#define LWELLPP     38		/* pseudo P-type well         */
#define LWELLNP     39		/* pseudo N-type well         */
#define LFRAME      40		/* pad frame boundary         */

static GRAPHICS mocmos_m1_lay = {LAYERT1,COLORT1, SOLIDC, PATTERNED,
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
static GRAPHICS mocmos_m2_lay = {LAYERT4,COLORT4, SOLIDC, PATTERNED,
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
static GRAPHICS mocmos_m3_lay = {LAYERT5,COLORT5, SOLIDC, PATTERNED,
/* metal-3 layer */		{0x2222, /*   X   X   X   X  */
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
static GRAPHICS mocmos_m4_lay = {LAYERO,LBLUE, PATTERNED, PATTERNED,
/* metal-4 layer */		{0xFFFF, /* XXXXXXXXXXXXXXXX */
						0x0000,  /*                  */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x0000,  /*                  */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x0000,  /*                  */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x0000,  /*                  */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x0000,  /*                  */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x0000,  /*                  */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x0000,  /*                  */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS mocmos_m5_lay = {LAYERO, ORANGE, PATTERNED|OUTLINEPAT, PATTERNED|OUTLINEPAT,
/* metal-5 layer */		{0x8888,  /* X   X   X   X    */
						0x1111,   /*    X   X   X   X */
						0x2222,   /*   X   X   X   X  */
						0x4444,   /*  X   X   X   X   */
						0x8888,   /* X   X   X   X    */
						0x1111,   /*    X   X   X   X */
						0x2222,   /*   X   X   X   X  */
						0x4444,   /*  X   X   X   X   */
						0x8888,   /* X   X   X   X    */
						0x1111,   /*    X   X   X   X */
						0x2222,   /*   X   X   X   X  */
						0x4444,   /*  X   X   X   X   */
						0x8888,   /* X   X   X   X    */
						0x1111,   /*    X   X   X   X */
						0x2222,   /*   X   X   X   X  */
						0x4444},  /*  X   X   X   X   */
						NOVARIABLE, 0};
static GRAPHICS mocmos_m6_lay = {LAYERO, CYAN, PATTERNED, PATTERNED,
/* metal-6 layer */		{0x8888,  /* X   X   X   X    */
						0x4444,   /*  X   X   X   X   */
						0x2222,   /*   X   X   X   X  */
						0x1111,   /*    X   X   X   X */
						0x8888,   /* X   X   X   X    */
						0x4444,   /*  X   X   X   X   */
						0x2222,   /*   X   X   X   X  */
						0x1111,   /*    X   X   X   X */
						0x8888,   /* X   X   X   X    */
						0x4444,   /*  X   X   X   X   */
						0x2222,   /*   X   X   X   X  */
						0x1111,   /*    X   X   X   X */
						0x8888,   /* X   X   X   X    */
						0x4444,   /*  X   X   X   X   */
						0x2222,   /*   X   X   X   X  */
						0x1111},  /*    X   X   X   X */
						NOVARIABLE, 0};
static GRAPHICS mocmos_p1_lay = {LAYERT2,COLORT2, SOLIDC, PATTERNED,
/* poly layer */		{0x1111, /*    X   X   X   X */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x1111,  /*    X   X   X   X */
						0x5555,  /*  X X X X X X X X */
						0x1111,  /*    X   X   X   X */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x1111,  /*    X   X   X   X */
						0x5555,  /*  X X X X X X X X */
						0x1111,  /*    X   X   X   X */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x1111,  /*    X   X   X   X */
						0x5555,  /*  X X X X X X X X */
						0x1111,  /*    X   X   X   X */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x1111,  /*    X   X   X   X */
						0x5555}, /*  X X X X X X X X */
						NOVARIABLE, 0};
static GRAPHICS mocmos_p2_lay = {LAYERO,ORANGE, PATTERNED, PATTERNED,
/* poly2 layer */		{0xAFAF, /* X X XXXXX X XXXX */
						0x8888,  /* X   X   X   X    */
						0xFAFA,  /* XXXXX X XXXXX X  */
						0x8888,  /* X   X   X   X    */
						0xAFAF,  /* X X XXXXX X XXXX */
						0x8888,  /* X   X   X   X    */
						0xFAFA,  /* XXXXX X XXXXX X  */
						0x8888,  /* X   X   X   X    */
						0xAFAF,  /* X X XXXXX X XXXX */
						0x8888,  /* X   X   X   X    */
						0xFAFA,  /* XXXXX X XXXXX X  */
						0x8888,  /* X   X   X   X    */
						0xAFAF,  /* X X XXXXX X XXXX */
						0x8888,  /* X   X   X   X    */
						0xFAFA,  /* XXXXX X XXXXX X  */
						0x8888}, /* X   X   X   X    */
						NOVARIABLE, 0};
static GRAPHICS mocmos_pa_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
/* P active layer */	{0x0000, /*                  */
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
static GRAPHICS mocmos_na_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
/* N active layer */	{0x0000, /*                  */
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
static GRAPHICS mocmos_ssp_lay = {LAYERO,YELLOW, PATTERNED, PATTERNED,
/* P Select layer */	{0x1010, /*    X       X     */
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
static GRAPHICS mocmos_ssn_lay = {LAYERO,YELLOW, PATTERNED, PATTERNED,
/* N Select layer */	{0x0101, /*        X       X */
						0x0000,  /*                  */
						0x1010,  /*    X       X     */
						0x0000,  /*                  */
						0x0101,  /*        X       X */
						0x0000,  /*                  */
						0x1010,  /*    X       X     */
						0x0000,  /*                  */
						0x0101,  /*        X       X */
						0x0000,  /*                  */
						0x1010,  /*    X       X     */
						0x0000,  /*                  */
						0x0101,  /*        X       X */
						0x0000,  /*                  */
						0x1010,  /*    X       X     */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS mocmos_wp_lay = {LAYERO,BROWN, PATTERNED, PATTERNED,
/* P Well layer */		{0x0202, /*       X       X  */
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
						0x1010,  /*    X       X     */
						0x0808,  /*     X       X    */
						0x0404}, /*      X       X   */
						NOVARIABLE, 0};
static GRAPHICS mocmos_wn_lay = {LAYERO,BROWN, PATTERNED, PATTERNED,
/* N Well implant */	{0x0202, /*       X       X  */
						0x0000,  /*                  */
						0x2020,  /*   X       X      */
						0x0000,  /*                  */
						0x0202,  /*       X       X  */
						0x0000,  /*                  */
						0x2020,  /*   X       X      */
						0x0000,  /*                  */
						0x0202,  /*       X       X  */
						0x0000,  /*                  */
						0x2020,  /*   X       X      */
						0x0000,  /*                  */
						0x0202,  /*       X       X  */
						0x0000,  /*                  */
						0x2020,  /*   X       X      */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS mocmos_pc_lay = {LAYERO,DGRAY, SOLIDC, SOLIDC,
/* poly cut layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmos_ac_lay = {LAYERO,DGRAY, SOLIDC, SOLIDC,
/* active cut layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmos_v1_lay = {LAYERO,GRAY, SOLIDC, SOLIDC,
/* via1 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmos_v2_lay = {LAYERO,GRAY, SOLIDC, SOLIDC,
/* via2 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmos_v3_lay = {LAYERO,GRAY, SOLIDC, SOLIDC,
/* via3 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmos_v4_lay = {LAYERO,GRAY, SOLIDC, SOLIDC,
/* via4 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmos_v5_lay = {LAYERO,GRAY, SOLIDC, SOLIDC,
/* via5 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmos_ovs_lay = {LAYERO,DGRAY, PATTERNED, PATTERNED,
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
static GRAPHICS mocmos_tr_lay = {LAYERT2,COLORT2, SOLIDC, PATTERNED,
/* poly/trans layer */	{0x1111, /*    X   X   X   X */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x1111,  /*    X   X   X   X */
						0x5555,  /*  X X X X X X X X */
						0x1111,  /*    X   X   X   X */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x1111,  /*    X   X   X   X */
						0x5555,  /*  X X X X X X X X */
						0x1111,  /*    X   X   X   X */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x1111,  /*    X   X   X   X */
						0x5555,  /*  X X X X X X X X */
						0x1111,  /*    X   X   X   X */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x1111,  /*    X   X   X   X */
						0x5555}, /*  X X X X X X X X */
						NOVARIABLE, 0};
static GRAPHICS mocmos_cp_lay = {LAYERO,BLACK, SOLIDC, SOLIDC,
/* poly cap layer */	{0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmos_paw_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
/* P act well layer */	{0x0000, /*                  */
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
static GRAPHICS mocmos_sb_lay = {LAYERO,LGRAY, PATTERNED, PATTERNED,
/* Silicide block */	{0x2222, /*   X   X   X   X  */
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
static GRAPHICS mocmos_pm1_lay ={LAYERT1,COLORT1, SOLIDC, PATTERNED,
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
static GRAPHICS mocmos_pm2_lay = {LAYERT4,COLORT4, SOLIDC, PATTERNED,
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
static GRAPHICS mocmos_pm3_lay = {LAYERT5,COLORT5, SOLIDC, PATTERNED,
/* pseudo metal-3 */	{0x1010, /*    X       X     */
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
static GRAPHICS mocmos_pm4_lay = {LAYERO,LBLUE, PATTERNED, PATTERNED,
/* pseudo metal-4 */	{0xFFFF, /* XXXXXXXXXXXXXXXX */
						0x0000,  /*                  */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x0000,  /*                  */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x0000,  /*                  */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x0000,  /*                  */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x0000,  /*                  */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x0000,  /*                  */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x0000,  /*                  */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS mocmos_pm5_lay = {LAYERO, ORANGE, PATTERNED|OUTLINEPAT, PATTERNED|OUTLINEPAT,
/* pseudo metal-5 */	{0x8888,  /* X   X   X   X    */
						0x1111,   /*    X   X   X   X */
						0x2222,   /*   X   X   X   X  */
						0x4444,   /*  X   X   X   X   */
						0x8888,   /* X   X   X   X    */
						0x1111,   /*    X   X   X   X */
						0x2222,   /*   X   X   X   X  */
						0x4444,   /*  X   X   X   X   */
						0x8888,   /* X   X   X   X    */
						0x1111,   /*    X   X   X   X */
						0x2222,   /*   X   X   X   X  */
						0x4444,   /*  X   X   X   X   */
						0x8888,   /* X   X   X   X    */
						0x1111,   /*    X   X   X   X */
						0x2222,   /*   X   X   X   X  */
						0x4444},  /*  X   X   X   X   */
						NOVARIABLE, 0};
static GRAPHICS mocmos_pm6_lay = {LAYERO, CYAN, PATTERNED, PATTERNED,
/* pseudo metal-6 */	{0x8888,  /* X   X   X   X    */
						0x4444,   /*  X   X   X   X   */
						0x2222,   /*   X   X   X   X  */
						0x1111,   /*    X   X   X   X */
						0x8888,   /* X   X   X   X    */
						0x4444,   /*  X   X   X   X   */
						0x2222,   /*   X   X   X   X  */
						0x1111,   /*    X   X   X   X */
						0x8888,   /* X   X   X   X    */
						0x4444,   /*  X   X   X   X   */
						0x2222,   /*   X   X   X   X  */
						0x1111,   /*    X   X   X   X */
						0x8888,   /* X   X   X   X    */
						0x4444,   /*  X   X   X   X   */
						0x2222,   /*   X   X   X   X  */
						0x1111},  /*    X   X   X   X */
						NOVARIABLE, 0};
static GRAPHICS mocmos_pp1_lay = {LAYERT2,COLORT2, SOLIDC, PATTERNED,
/* pseudo poly layer */	{0x1111, /*    X   X   X   X */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x1111,  /*    X   X   X   X */
						0x5555,  /*  X X X X X X X X */
						0x1111,  /*    X   X   X   X */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x1111,  /*    X   X   X   X */
						0x5555,  /*  X X X X X X X X */
						0x1111,  /*    X   X   X   X */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x1111,  /*    X   X   X   X */
						0x5555,  /*  X X X X X X X X */
						0x1111,  /*    X   X   X   X */
						0xFFFF,  /* XXXXXXXXXXXXXXXX */
						0x1111,  /*    X   X   X   X */
						0x5555}, /*  X X X X X X X X */
						NOVARIABLE, 0};
static GRAPHICS mocmos_pp2_lay = {LAYERO,ORANGE, PATTERNED, PATTERNED,
/* pseudo poly2 layer */{0xAFAF, /* X X XXXXX X XXXX */
						0x8888,  /* X   X   X   X    */
						0xFAFA,  /* XXXXX X XXXXX X  */
						0x8888,  /* X   X   X   X    */
						0xAFAF,  /* X X XXXXX X XXXX */
						0x8888,  /* X   X   X   X    */
						0xFAFA,  /* XXXXX X XXXXX X  */
						0x8888,  /* X   X   X   X    */
						0xAFAF,  /* X X XXXXX X XXXX */
						0x8888,  /* X   X   X   X    */
						0xFAFA,  /* XXXXX X XXXXX X  */
						0x8888,  /* X   X   X   X    */
						0xAFAF,  /* X X XXXXX X XXXX */
						0x8888,  /* X   X   X   X    */
						0xFAFA,  /* XXXXX X XXXXX X  */
						0x8888}, /* X   X   X   X    */
						NOVARIABLE, 0};
static GRAPHICS mocmos_ppa_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
/* pseudo P active */	{0x0000, /*                  */
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
static GRAPHICS mocmos_pna_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
/* pseudo N active */	{0x0000, /*                  */
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
static GRAPHICS mocmos_pssp_lay = {LAYERO,YELLOW,PATTERNED, PATTERNED,
/* pseudo P Select */	{0x1010, /*    X       X     */
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
static GRAPHICS mocmos_pssn_lay = {LAYERO,YELLOW,PATTERNED, PATTERNED,
/* pseudo N Select */	{0x0101, /*        X       X */
						0x0000,  /*                  */
						0x1010,  /*    X       X     */
						0x0000,  /*                  */
						0x0101,  /*        X       X */
						0x0000,  /*                  */
						0x1010,  /*    X       X     */
						0x0000,  /*                  */
						0x0101,  /*        X       X */
						0x0000,  /*                  */
						0x1010,  /*    X       X     */
						0x0000,  /*                  */
						0x0101,  /*        X       X */
						0x0000,  /*                  */
						0x1010,  /*    X       X     */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS mocmos_pwp_lay = {LAYERO,BROWN, PATTERNED, PATTERNED,
/* pseudo P Well */		{0x0202, /*       X       X  */
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
						0x1010,  /*    X       X     */
						0x0808,  /*     X       X    */
						0x0404}, /*      X       X   */
						NOVARIABLE, 0};
static GRAPHICS mocmos_pwn_lay = {LAYERO,BROWN, PATTERNED, PATTERNED,
/* pseudo N Well */		{0x0202, /*       X       X  */
						0x0000,  /*                  */
						0x2020,  /*   X       X      */
						0x0000,  /*                  */
						0x0202,  /*       X       X  */
						0x0000,  /*                  */
						0x2020,  /*   X       X      */
						0x0000,  /*                  */
						0x0202,  /*       X       X  */
						0x0000,  /*                  */
						0x2020,  /*   X       X      */
						0x0000,  /*                  */
						0x0202,  /*       X       X  */
						0x0000,  /*                  */
						0x2020,  /*   X       X      */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS mocmos_pf_lay = {LAYERO, RED, SOLIDC, PATTERNED,
/* pad frame */		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};

/* these tables must be updated together */
GRAPHICS *mocmos_layers[MAXLAYERS+1] = {
	&mocmos_m1_lay, &mocmos_m2_lay, &mocmos_m3_lay,				/* metal 1/2/3 */
	&mocmos_m4_lay, &mocmos_m5_lay, &mocmos_m6_lay,				/* metal 4/5/6 */
	&mocmos_p1_lay, &mocmos_p2_lay,								/* poly 1/2 */
	&mocmos_pa_lay, &mocmos_na_lay,								/* P/N active */
	&mocmos_ssp_lay, &mocmos_ssn_lay,							/* P/N select */
	&mocmos_wp_lay, &mocmos_wn_lay,								/* P/N well */
	&mocmos_pc_lay, &mocmos_ac_lay,								/* poly/act cut */
	&mocmos_v1_lay, &mocmos_v2_lay, &mocmos_v3_lay,				/* via 1/2/3 */
	&mocmos_v4_lay, &mocmos_v5_lay,								/* via 4/5 */
	&mocmos_ovs_lay,											/* overglass */
	&mocmos_tr_lay,												/* transistor poly */
	&mocmos_cp_lay,												/* poly cap */
	&mocmos_paw_lay,											/* P active well */
	&mocmos_sb_lay,												/* Silicide block */
	&mocmos_pm1_lay, &mocmos_pm2_lay,							/* pseudo metal 1/2 */
	&mocmos_pm3_lay, &mocmos_pm4_lay,							/* pseudo metal 3/4 */
	&mocmos_pm5_lay, &mocmos_pm6_lay,							/* pseudo metal 5/6 */
	&mocmos_pp1_lay, &mocmos_pp2_lay,							/* pseudo poly 1/2 */
	&mocmos_ppa_lay, &mocmos_pna_lay,							/* pseudo P/N active */
	&mocmos_pssp_lay, &mocmos_pssn_lay,							/* pseudo P/N select */
	&mocmos_pwp_lay, &mocmos_pwn_lay,							/* pseudo P/N well */
	&mocmos_pf_lay, NOGRAPHICS};								/* pad frame */
static CHAR *mocmos_layer_names[MAXLAYERS] = {
	x_("Metal-1"), x_("Metal-2"), x_("Metal-3"),				/* metal 1/2/3 */
	x_("Metal-4"), x_("Metal-5"), x_("Metal-6"),				/* metal 4/5/6 */
	x_("Polysilicon-1"), x_("Polysilicon-2"),					/* poly 1/2 */
	x_("P-Active"), x_("N-Active"),								/* P/N active */
	x_("P-Select"), x_("N-Select"),								/* P/N select */
	x_("P-Well"), x_("N-Well"),									/* P/N well */
	x_("Poly-Cut"), x_("Active-Cut"),							/* poly/act cut */
	x_("Via1"), x_("Via2"), x_("Via3"),							/* via 1/2/3 */
	x_("Via4"), x_("Via5"),										/* via 4/5 */
	x_("Passivation"),											/* overglass */
	x_("Transistor-Poly"),										/* transistor poly */
	x_("Poly-Cap"),												/* poly cap */
	x_("P-Active-Well"),										/* P active well */
	x_("Silicide-Block"),										/* Silicide block */
	x_("Pseudo-Metal-1"), x_("Pseudo-Metal-2"),					/* pseudo metal 1/2 */
	x_("Pseudo-Metal-3"), x_("Pseudo-Metal-4"),					/* pseudo metal 3/4 */
	x_("Pseudo-Metal-5"), x_("Pseudo-Metal-6"),					/* pseudo metal 5/6 */
	x_("Pseudo-Polysilicon"), x_("Pseudo-Electrode"),			/* pseudo poly 1/2 */
	x_("Pseudo-P-Active"), x_("Pseudo-N-Active"),				/* pseudo P/N active */
	x_("Pseudo-P-Select"), x_("Pseudo-N-Select"),				/* pseudo P/N select */
	x_("Pseudo-P-Well"), x_("Pseudo-N-Well"),					/* pseudo P/N well */
	x_("Pad-Frame")};											/* pad frame */
static INTBIG mocmos_layer_function[MAXLAYERS] = {
	LFMETAL1|LFTRANS1, LFMETAL2|LFTRANS4, LFMETAL3|LFTRANS5,	/* metal 1/2/3 */
	LFMETAL4, LFMETAL5, LFMETAL6,								/* metal 4/5/6 */
	LFPOLY1|LFTRANS2, LFPOLY2,									/* poly 1/2 */
	LFDIFF|LFPTYPE|LFTRANS3, LFDIFF|LFNTYPE|LFTRANS3,			/* P/N active */
	LFIMPLANT|LFPTYPE, LFIMPLANT|LFNTYPE,						/* P/N select */
	LFWELL|LFPTYPE, LFWELL|LFNTYPE,								/* P/N well */
	LFCONTACT1|LFCONPOLY, LFCONTACT1|LFCONDIFF,					/* poly/act cut */
	LFCONTACT2|LFCONMETAL, LFCONTACT3|LFCONMETAL, LFCONTACT4|LFCONMETAL,	/* via 1/2/3 */
	LFCONTACT5|LFCONMETAL, LFCONTACT6|LFCONMETAL,				/* via 4/5 */
	LFOVERGLASS,												/* overglass */
	LFPOLY1|LFINTRANS|LFTRANS2,									/* transistor poly */
	LFCAP,														/* poly cap */
	LFDIFF|LFPTYPE|LFTRANS3,									/* P active well */
	LFART,														/* Silicide block */
	LFMETAL1|LFPSEUDO|LFTRANS1,LFMETAL2|LFPSEUDO|LFTRANS4,		/* pseudo metal 1/2 */
	LFMETAL3|LFPSEUDO|LFTRANS5, LFMETAL4|LFPSEUDO,				/* pseudo metal 3/4 */
	LFMETAL5|LFPSEUDO, LFMETAL6|LFPSEUDO,						/* pseudo metal 5/6 */
	LFPOLY1|LFPSEUDO|LFTRANS2, LFPOLY2|LFPSEUDO,				/* pseudo poly 1/2 */
	LFDIFF|LFPTYPE|LFPSEUDO|LFTRANS3,							/* pseudo P/N active */
		LFDIFF|LFNTYPE|LFPSEUDO|LFTRANS3,
	LFIMPLANT|LFPTYPE|LFPSEUDO, LFIMPLANT|LFNTYPE|LFPSEUDO,		/* pseudo P/N select */
	LFWELL|LFPTYPE|LFPSEUDO, LFWELL|LFNTYPE|LFPSEUDO,			/* pseudo P/N well */
	LFART};														/* pad frame */
static CHAR *mocmos_cif_layers[MAXLAYERS] = {
	x_("CMF"), x_("CMS"), x_("CMT"), x_("CMQ"), x_("CMP"), x_("CM6"),	/* metal 1/2/3/4/5/6 */
	x_("CPG"), x_("CEL"),										/* poly 1/2 */
	x_("CAA"), x_("CAA"),										/* P/N active */
	x_("CSP"), x_("CSN"),										/* P/N select */
	x_("CWP"), x_("CWN"),										/* P/N well */
	x_("CCC"), x_("CCC"),										/* poly/act cut */
	x_("CVA"), x_("CVS"), x_("CVT"), x_("CVQ"), x_("CV5"),		/* via 1/2/3/4/5 */
	x_("COG"),													/* overglass */
	x_("CPG"),													/* transistor poly */
	x_("CPC"),													/* poly cap */
	x_("CAA"),													/* P active well */
	x_("CSB"),													/* Silicide block */
	x_(""), x_(""), x_(""), x_(""), x_(""), x_(""),				/* pseudo metal 1/2/3/4/5/6 */
	x_(""), x_(""),												/* pseudo poly 1/2 */
	x_(""), x_(""),												/* pseudo P/N active */
	x_("CSP"), x_("CSN"),										/* pseudo P/N select */
	x_("CWP"), x_("CWN"),										/* pseudo P/N well */
	x_("XP")};													/* pad frame */
static CHAR *mocmos_gds_layers[MAXLAYERS] = {
	x_("49"), x_("51"), x_("62"), x_("31"), x_("33"), x_("37"),	/* metal 1/2/3/4/5/6 */
	x_("46"), x_("56"),											/* poly 1/2 */
	x_("43"), x_("43"),											/* P/N active */
	x_("44"), x_("45"),											/* P/N select */
	x_("41"), x_("42"),											/* P/N well */
	x_("47"), x_("48"),											/* poly/act cut */
	x_("50"), x_("61"), x_("30"), x_("32"), x_("36"),			/* via 1/2/3/4/5 */
	x_("52"),													/* overglass */
	x_("46"),													/* transistor poly */
	x_("28"),													/* poly cap */
	x_("43"),													/* P active well */
	x_("29"),													/* Silicide block */
	x_(""), x_(""), x_(""), x_(""), x_(""), x_(""),				/* pseudo metal 1/2/3/4/5/6 */
	x_(""), x_(""),												/* pseudo poly 1/2 */
	x_(""), x_(""),												/* pseudo P/N active */
	x_(""), x_(""),												/* pseudo P/N select */
	x_(""), x_(""),												/* pseudo P/N well */
	x_("26")};													/* pad frame */
static CHAR *mocmos_skill_layers[MAXLAYERS] = {
	x_("metal1"), x_("metal2"), x_("metal3"),					/* metal 1/2/3 */
	x_("metal4"), x_("metal5"), x_("metal6"),					/* metal 4/5/6 */
	x_("poly"), x_(""),											/* poly 1/2 */
	x_("aa"), x_("aa"),											/* P/N active */
	x_("pplus"), x_("nplus"),									/* P/N select */
	x_("pwell"), x_("nwell"),									/* P/N well */
	x_("pcont"), x_("acont"),									/* poly/act cut */
	x_("via"), x_("via2"), x_("via3"), x_("via4"), x_("via5"),	/* via 1/2/3/4/5 */
	x_("glasscut"),												/* overglass */
	x_("poly"),													/* transistor poly */
	x_(""),														/* poly cap */
	x_("aa"),													/* P active well */
	x_(""),														/* Silicide block */
	x_(""), x_(""), x_(""), x_(""), x_(""), x_(""),				/* pseudo metal 1/2/3/4/5/6 */
	x_(""), x_(""),												/* pseudo poly 1/2 */
	x_(""), x_(""),												/* pseudo P/N active */
	x_("pplus"), x_("nplus"),									/* pseudo P/N select */
	x_("pwell"), x_("nwell"),									/* pseudo P/N well */
	x_("")};													/* pad frame */
static INTBIG mocmos_3dthick_layers[MAXLAYERS] = {
	0, 0, 0, 0, 0, 0,											/* metal 1/2/3/4/5/6 */
	0, 0,														/* poly 1/2 */
	0, 0,														/* P/N active */
	0, 0,														/* P/N select */
	0, 0,														/* P/N well */
	2, 4,														/* poly/act cut */
	2, 2, 2, 2, 2,												/* via 1/2/3/4/5 */
	0,															/* overglass */
	0,															/* transistor poly */
	0,															/* poly cap */
	0,															/* P active well */
	0,															/* Silicide block */
	0, 0, 0, 0, 0, 0,											/* pseudo metal 1/2/3/4/5/6 */
	0, 0,														/* pseudo poly 1/2 */
	0, 0,														/* pseudo P/N active */
	0, 0,														/* pseudo P/N select */
	0, 0,														/* pseudo P/N well */
	0};															/* pad frame */
static INTBIG mocmos_3dheight_layers[MAXLAYERS] = {
	17, 19, 21, 23, 25, 27,										/* metal 1/2/3/4/5/6 */
	15, 16,														/* poly 1/2 */
	13, 13,														/* P/N active */
	12, 12,														/* P/N select */
	11, 11,														/* P/N well */
	16, 15,														/* poly/act cut */
	18, 20, 22, 24, 26,											/* via 1/2/3/4/5 */
	30,															/* overglass */
	15,															/* transistor poly */
	28,															/* poly cap */
	13,															/* P active well */
	10,															/* Silicide block */
	17, 19, 21, 23, 25, 27,										/* pseudo metal 1/2/3/4/5/6 */
	12, 13,														/* pseudo poly 1/2 */
	11, 11,														/* pseudo P/N active */
	2, 2,														/* pseudo P/N select */
	0, 0,														/* pseudo P/N well */
	33};														/* pad frame */
static INTBIG mocmos_printcolors_layers[MAXLAYERS*5] = {
	0,0,255,     WHOLE*8/10,1,									/* metal 1 */
	184,84,212,  WHOLE*7/10,1,									/* metal 2 */
	255,255,51,  WHOLE*6/10,1,									/* metal 3 */
	150,150,255, WHOLE*5/10,1,									/* metal 4 */
	2, 15,159,   WHOLE*4/10,1,									/* metal 5 */
	2, 15,159,   WHOLE*3/10,1,									/* metal 6 */
	255,0,0,     WHOLE,1,										/* poly 1 */
	255,190,6,   WHOLE,1,										/* poly 2 */
	204,204,0,   WHOLE,1,										/* P active */
	0,255,0,     WHOLE,1,										/* N active */
	255,255,255, WHOLE,0,										/* P select */
	255,255,255, WHOLE,0,										/* N select */
	255,255,255, WHOLE,0,										/* P well */
	204,204,204, WHOLE,0,										/* N well */
	0,0,0,       WHOLE,1,										/* poly cut */
	0,0,0,       WHOLE,1,										/* active cut */
	0,0,0,       WHOLE,1,										/* via 1 */
	0,0,0,       WHOLE,1,										/* via 2 */
	0,0,0,       WHOLE,1,										/* via 3 */
	0,0,0,       WHOLE,1,										/* via 4 */
	0,0,0,       WHOLE,1,										/* via 5 */
	179,179,179, WHOLE,1,										/* overglass */
	255,0,0,     WHOLE,1,										/* transistor poly */
	0,0,255,     WHOLE,1,										/* poly cap */
	255,255,255, WHOLE,0,										/* P active well */
	255,255,255, WHOLE,0,										/* Silicide block */
	255,255,255, WHOLE,1,										/* pseudo metal 1 */
	255,255,255, WHOLE,1,										/* pseudo metal 2 */
	255,255,255, WHOLE,1,										/* pseudo metal 3 */
	255,255,255, WHOLE,1,										/* pseudo metal 4 */
	255,255,255, WHOLE,1,										/* pseudo metal 5 */
	255,255,255, WHOLE,1,										/* pseudo metal 6 */
	255,255,255, WHOLE,1,										/* pseudo poly 1 */
	255,255,255, WHOLE,1,										/* pseudo poly 2 */
	255,255,255, WHOLE,1,										/* pseudo P active */
	255,255,255, WHOLE,1,										/* pseudo N active */
	255,255,255, WHOLE,1,										/* pseudo P select */
	255,255,255, WHOLE,1,										/* pseudo N select */
	255,255,255, WHOLE,1,										/* pseudo P well */
	255,255,255, WHOLE,1,										/* pseudo N well */
	255,255,255, WHOLE,1};										/* pad frame */
/* there are no available letters */
static CHAR *mocmos_layer_letters[MAXLAYERS] = {
	x_("m"), x_("h"), x_("r"), x_("q"), x_("a"), x_("c"),		/* metal 1/2/3/4/5/6 */
	x_("p"), x_("l"),											/* poly 1/2 */
	x_("s"), x_("d"),											/* P/N active */
	x_("e"), x_("f"),											/* P/N select */
	x_("w"), x_("n"),											/* P/N well */
	x_("j"), x_("k"),											/* poly/act cut */
	x_("v"), x_("u"), x_("z"), x_("i"), x_("y"),				/* via 1/2/3/4/5 */
	x_("o"),													/* overglass */
	x_("t"),													/* transistor poly */
	x_("g"),													/* poly cap */
	x_("x"),													/* P active well */
	x_("9"),													/* Silicide block */
	x_("M"), x_("H"), x_("R"), x_("Q"), x_("A"), x_("C"),		/* pseudo metal 1/2/3/4/5/6 */
	x_("P"), x_("L"),											/* pseudo poly 1/2 */
	x_("S"), x_("D"),											/* pseudo P/N active */
	x_("E"), x_("F"),											/* pseudo P/N select */
	x_("W"), x_("N"),											/* pseudo P/N well */
	x_("b")};													/* pad frame */

/* The low 5 bits map Metal-1, Poly-1, Active, Metal-2, and Metal-3 */
static TECH_COLORMAP mocmos_colmap[32] =
{                  /*     Metal-3 Metal-2 Active Polysilicon-1 Metal-1 */
	{200,200,200}, /*  0:                                              */
	{ 96,209,255}, /*  1:                                      Metal-1 */
	{255,155,192}, /*  2:                        Polysilicon-1         */
	{111,144,177}, /*  3:                        Polysilicon-1 Metal-1 */
	{107,226, 96}, /*  4:                 Active                       */
	{ 83,179,160}, /*  5:                 Active               Metal-1 */
	{161,151,126}, /*  6:                 Active Polysilicon-1         */
	{110,171,152}, /*  7:                 Active Polysilicon-1 Metal-1 */
	{224, 95,255}, /*  8:         Metal-2                              */
	{135,100,191}, /*  9:         Metal-2                      Metal-1 */
	{170, 83,170}, /* 10:         Metal-2        Polysilicon-1         */
	{152,104,175}, /* 11:         Metal-2        Polysilicon-1 Metal-1 */
	{150,124,163}, /* 12:         Metal-2 Active                       */
	{129,144,165}, /* 13:         Metal-2 Active               Metal-1 */
	{155,133,151}, /* 14:         Metal-2 Active Polysilicon-1         */
	{141,146,153}, /* 15:         Metal-2 Active Polysilicon-1 Metal-1 */
	{247,251, 20}, /* 16: Metal-3                                      */
	{154,186, 78}, /* 17: Metal-3                              Metal-1 */
	{186,163, 57}, /* 18: Metal-3                Polysilicon-1         */
	{167,164, 99}, /* 19: Metal-3                Polysilicon-1 Metal-1 */
	{156,197, 41}, /* 20: Metal-3         Active                       */
	{138,197, 83}, /* 21: Metal-3         Active               Metal-1 */
	{161,184, 69}, /* 22: Metal-3         Active Polysilicon-1         */
	{147,183, 97}, /* 23: Metal-3         Active Polysilicon-1 Metal-1 */
	{186,155, 76}, /* 24: Metal-3 Metal-2                              */
	{155,163,119}, /* 25: Metal-3 Metal-2                      Metal-1 */
	{187,142, 97}, /* 26: Metal-3 Metal-2        Polysilicon-1         */
	{165,146,126}, /* 27: Metal-3 Metal-2        Polysilicon-1 Metal-1 */
	{161,178, 82}, /* 28: Metal-3 Metal-2 Active                       */
	{139,182,111}, /* 29: Metal-3 Metal-2 Active               Metal-1 */
	{162,170, 97}, /* 30: Metal-3 Metal-2 Active Polysilicon-1         */
	{147,172,116}  /* 31: Metal-3 Metal-2 Active Polysilicon-1 Metal-1 */
};

/******************** DESIGN RULES ********************/

#define WIDELIMIT         K10					/* wide rules apply to geometry larger than this */


/* the meaning of "when" in the DRC table */
#define M2      01		/* only applies if there are 2 metal layers in process */
#define M3      02		/* only applies if there are 3 metal layers in process */
#define M4      04		/* only applies if there are 4 metal layers in process */
#define M5     010		/* only applies if there are 5 metal layers in process */
#define M6     020		/* only applies if there are 6 metal layers in process */
#define M23     03		/* only applies if there are 2-3 metal layers in process */
#define M234    07		/* only applies if there are 2-4 metal layers in process */
#define M2345  017		/* only applies if there are 2-5 metal layers in process */
#define M456   034		/* only applies if there are 4-6 metal layers in process */
#define M56    030		/* only applies if there are 5-6 metal layers in process */
#define M3456  036		/* only applies if there are 3-6 metal layers in process */

#define AC     040		/* only applies if alternate contact rules are in effect */
#define NAC   0100		/* only applies if alternate contact rules are not in effect */
#define SV    0200		/* only applies if stacked vias are allowed */
#define NSV   0400		/* only applies if stacked vias are not allowed */
#define DE   01000		/* only applies if deep rules are in effect */
#define SU   02000		/* only applies if submicron rules are in effect */
#define SC   04000		/* only applies if scmos rules are in effect */

/* the meaning of "ruletype" in the DRC table */
#define MINWID     1		/* a minimum-width rule */
#define NODSIZ     2		/* a node size rule */
#define SURROUND   3		/* a general surround rule */
#define VIASUR     4		/* a via surround rule */
#define TRAWELL    5		/* a transistor well rule */
#define TRAPOLY    6		/* a transistor poly rule */
#define TRAACTIVE  7		/* a transistor active rule */
#define SPACING    8		/* a spacing rule */
#define SPACINGM   9		/* a multi-cut spacing rule */
#define SPACINGW  10		/* a wide spacing rule */
#define SPACINGE  11		/* an edge spacing rule */
#define CONSPA    12		/* a connected spacing rule */
#define UCONSPA   13		/* an unconnected spacing rule */
#define CUTSPA    14		/* a contact cut spacing rule */
#define CUTSIZE   15		/* a contact cut size rule */
#define CUTSUR    16		/* a contact cut surround rule */
#define ASURROUND 17		/* arc surround rule */

struct
{
	CHAR *rule;				/* the name of the rule */
	INTBIG when;			/* when the rule is used */
	INTBIG ruletype;		/* the type of the rule */
	CHAR *layer1, *layer2;	/* two layers that are used by the rule */
	INTBIG distance;		/* the spacing of the rule */
	CHAR *nodename;			/* the node that is used by the rule */
} mocmos_drcrules[] =
{
	{x_("1.1"),  DE|SU,           MINWID,   x_("P-Well"),          0,                   K12, 0},
	{x_("1.1"),  DE|SU,           MINWID,   x_("N-Well"),          0,                   K12, 0},
	{x_("1.1"),  DE|SU,           MINWID,   x_("Pseudo-P-Well"),   0,                   K12, 0},
	{x_("1.1"),  DE|SU,           MINWID,   x_("Pseudo-N-Well"),   0,                   K12, 0},
	{x_("1.1"),  SC,              MINWID,   x_("P-Well"),          0,                   K10, 0},
	{x_("1.1"),  SC,              MINWID,   x_("N-Well"),          0,                   K10, 0},
	{x_("1.1"),  SC,              MINWID,   x_("Pseudo-P-Well"),   0,                   K10, 0},
	{x_("1.1"),  SC,              MINWID,   x_("Pseudo-N-Well"),   0,                   K10, 0},

	{x_("1.2"),  DE|SU,           UCONSPA,  x_("P-Well"),         x_("P-Well"),         K18, 0},
	{x_("1.2"),  DE|SU,           UCONSPA,  x_("N-Well"),         x_("N-Well"),         K18, 0},
	{x_("1.2"),  SC,              UCONSPA,  x_("P-Well"),         x_("P-Well"),         K9,  0},
	{x_("1.2"),  SC,              UCONSPA,  x_("N-Well"),         x_("N-Well"),         K9,  0},

	{x_("1.3"),  0,               CONSPA,   x_("P-Well"),         x_("P-Well"),         K6,  0},
	{x_("1.3"),  0,               CONSPA,   x_("N-Well"),         x_("N-Well"),         K6,  0},

	{x_("1.4"),  0,               UCONSPA,  x_("P-Well"),         x_("N-Well"),         0,   0},

	{x_("2.1"),  0,               MINWID,   x_("P-Active"),        0,                   K3,  0},
	{x_("2.1"),  0,               MINWID,   x_("N-Active"),        0,                   K3,  0},

	{x_("2.2"),  0,               SPACING,  x_("P-Active"),       x_("P-Active"),       K3,  0},
	{x_("2.2"),  0,               SPACING,  x_("N-Active"),       x_("N-Active"),       K3,  0},
	{x_("2.2"),  0,               SPACING,  x_("P-Active-Well"),  x_("P-Active-Well"),  K3,  0},
	{x_("2.2"),  0,               SPACING,  x_("P-Active"),       x_("N-Active"),       K3,  0},
	{x_("2.2"),  0,               SPACING,  x_("P-Active"),       x_("P-Active-Well"),  K3,  0},
	{x_("2.2"),  0,               SPACING,  x_("N-Active"),       x_("P-Active-Well"),  K3,  0},

	{x_("2.3"),  DE|SU,           SURROUND, x_("N-Well"),         x_("P-Active"),       K6, x_("Metal-1-P-Active-Con")},
	{x_("2.3"),  DE|SU,           ASURROUND,x_("N-Well"),         x_("P-Active"),       K6, x_("P-Active")},
	{x_("2.3"),  DE|SU,           SURROUND, x_("P-Well"),         x_("N-Active"),       K6, x_("Metal-1-N-Active-Con")},
	{x_("2.3"),  DE|SU,           ASURROUND,x_("P-Well"),         x_("N-Active"),       K6, x_("N-Active")},
	{x_("2.3"),  DE|SU,           TRAWELL,   0,                    0,                   K6,  0},
	{x_("2.3"),  SC,              SURROUND, x_("N-Well"),         x_("P-Active"),       K5, x_("Metal-1-P-Active-Con")},
	{x_("2.3"),  SC,              ASURROUND,x_("N-Well"),         x_("P-Active"),       K5, x_("P-Active")},
	{x_("2.3"),  SC,              SURROUND, x_("P-Well"),         x_("N-Active"),       K5, x_("Metal-1-N-Active-Con")},
	{x_("2.3"),  SC,              ASURROUND,x_("P-Well"),         x_("N-Active"),       K5, x_("N-Active")},
	{x_("2.3"),  SC,              TRAWELL,   0,                    0,                   K5,  0},

	{x_("3.1"),  0,               MINWID,   x_("Polysilicon-1"),   0,                   K2,  0},
	{x_("3.1"),  0,               MINWID,   x_("Transistor-Poly"), 0,                   K2,  0},

	{x_("3.2"),  DE|SU,           SPACING,  x_("Polysilicon-1"),  x_("Polysilicon-1"),  K3,  0},
	{x_("3.2"),  DE|SU,           SPACING,  x_("Polysilicon-1"),  x_("Transistor-Poly"),K3,  0},
	{x_("3.2"),  SC,              SPACING,  x_("Polysilicon-1"),  x_("Polysilicon-1"),  K2,  0},
	{x_("3.2"),  SC,              SPACING,  x_("Polysilicon-1"),  x_("Transistor-Poly"),K2,  0},

	{x_("3.2a"), DE,              SPACING,  x_("Transistor-Poly"),x_("Transistor-Poly"),K4,  0},
	{x_("3.2a"), SU,              SPACING,  x_("Transistor-Poly"),x_("Transistor-Poly"),K3,  0},
	{x_("3.2a"), SC,              SPACING,  x_("Transistor-Poly"),x_("Transistor-Poly"),K2,  0},

	{x_("3.3"),  DE,              TRAPOLY,   0,                    0,                   H2,  0},
	{x_("3.3"),  SU|SC,           TRAPOLY,   0,                    0,                   K2,  0},

	{x_("3.4"),  DE,              TRAACTIVE, 0,                    0,                   K4,  0},
	{x_("3.4"),  SU|SC,           TRAACTIVE, 0,                    0,                   K3,  0},

	{x_("3.5"),  0,               SPACING,  x_("Polysilicon-1"),  x_("P-Active"),       K1,  0},
	{x_("3.5"),  0,               SPACING,  x_("Transistor-Poly"),x_("P-Active"),       K1,  0},
	{x_("3.5"),  0,               SPACING,  x_("Polysilicon-1"),  x_("N-Active"),       K1,  0},
	{x_("3.5"),  0,               SPACING,  x_("Transistor-Poly"),x_("N-Active"),       K1,  0},
	{x_("3.5"),  0,               SPACING,  x_("Polysilicon-1"),  x_("P-Active-Well"),  K1,  0},
	{x_("3.5"),  0,               SPACING,  x_("Transistor-Poly"),x_("P-Active-Well"),  K1,  0},

	{x_("4.4"),  DE,              MINWID,   x_("P-Select"),        0,                   K4,  0},
	{x_("4.4"),  DE,              MINWID,   x_("N-Select"),        0,                   K4,  0},
	{x_("4.4"),  DE,              MINWID,   x_("Pseudo-P-Select"), 0,                   K4,  0},
	{x_("4.4"),  DE,              MINWID,   x_("Pseudo-N-Select"), 0,                   K4,  0},
	{x_("4.4"),  DE,              SPACING,  x_("P-Select"),       x_("P-Select"),       K4,  0},
	{x_("4.4"),  DE,              SPACING,  x_("N-Select"),       x_("N-Select"),       K4,  0},
	{x_("4.4"),  SU|SC,           MINWID,   x_("P-Select"),        0,                   K2,  0},
	{x_("4.4"),  SU|SC,           MINWID,   x_("N-Select"),        0,                   K2,  0},
	{x_("4.4"),  SU|SC,           MINWID,   x_("Pseudo-P-Select"), 0,                   K2,  0},
	{x_("4.4"),  SU|SC,           MINWID,   x_("Pseudo-N-Select"), 0,                   K2,  0},
	{x_("4.4"),  SU|SC,           SPACING,  x_("P-Select"),       x_("P-Select"),       K2,  0},
	{x_("4.4"),  SU|SC,           SPACING,  x_("N-Select"),       x_("N-Select"),       K2,  0},
	{x_("4.4"),  0,               SPACING,  x_("P-Select"),       x_("N-Select"),       0,   0},

	{x_("5.1"),  0,               MINWID,   x_("Poly-Cut"),        0,                   K2,  0},

	{x_("5.2"),        NAC,       NODSIZ,    0,                    0,                   K5, x_("Metal-1-Polysilicon-1-Con")},
	{x_("5.2"),        NAC,       SURROUND, x_("Polysilicon-1"),  x_("Metal-1"),        H0, x_("Metal-1-Polysilicon-1-Con")},
	{x_("5.2"),        NAC,       CUTSUR,    0,                    0,                   H1, x_("Metal-1-Polysilicon-1-Con")},
	{x_("5.2b"),       AC,        NODSIZ,    0,                    0,                   K4, x_("Metal-1-Polysilicon-1-Con")},
	{x_("5.2b"),       AC,        SURROUND, x_("Polysilicon-1"),  x_("Metal-1"),        0,  x_("Metal-1-Polysilicon-1-Con")},
	{x_("5.2b"),       AC,        CUTSUR,    0,                    0,                   K1, x_("Metal-1-Polysilicon-1-Con")},

	{x_("5.3"),     DE,           CUTSPA,    0,                    0,                   K4, x_("Metal-1-Polysilicon-1-Con")},
	{x_("5.3"),     DE,           SPACING,  x_("Poly-Cut"),       x_("Poly-Cut"),       K4,  0},
	{x_("5.3,6.3"), DE|NAC,       SPACING,  x_("Active-Cut"),     x_("Poly-Cut"),       K4,  0},
	{x_("5.3"),     SU,           CUTSPA,    0,                    0,                   K3, x_("Metal-1-Polysilicon-1-Con")},
	{x_("5.3"),     SU,           SPACING,  x_("Poly-Cut"),       x_("Poly-Cut"),       K3,  0},
	{x_("5.3,6.3"), SU|NAC,       SPACING,  x_("Active-Cut"),     x_("Poly-Cut"),       K3,  0},
	{x_("5.3"),     SC,           CUTSPA,    0,                    0,                   K2, x_("Metal-1-Polysilicon-1-Con")},
	{x_("5.3"),     SC,           SPACING,  x_("Poly-Cut"),       x_("Poly-Cut"),       K2,  0},
	{x_("5.3,6.3"), SC|NAC,       SPACING,  x_("Active-Cut"),     x_("Poly-Cut"),       K2,  0},

#ifdef CENTERACTIVE
	{x_("5.4"),  0,               SPACING,  x_("Poly-Cut"),       x_("P-Active"),       K2,  0},
	{x_("5.4"),  0,               SPACING,  x_("Poly-Cut"),       x_("N-Active"),       K2,  0},
#else
	{x_("5.4"),  0,               SPACING,  x_("Poly-Cut"),       x_("Transistor-Poly"),K2,  0},
#endif

	{x_("5.5b"), DE|SU|AC,        UCONSPA,  x_("Poly-Cut"),       x_("Polysilicon-1"),  K5,  0},
	{x_("5.5b"), DE|SU|AC,        UCONSPA,  x_("Poly-Cut"),       x_("Transistor-Poly"),K5,  0},
	{x_("5.5b"), SC|   AC,        UCONSPA,  x_("Poly-Cut"),       x_("Polysilicon-1"),  K4,  0},
	{x_("5.5b"), SC|   AC,        UCONSPA,  x_("Poly-Cut"),       x_("Transistor-Poly"),K4,  0},

	{x_("5.6b"),       AC,        SPACING,  x_("Poly-Cut"),       x_("P-Active"),       K2,  0},
	{x_("5.6b"),       AC,        SPACING,  x_("Poly-Cut"),       x_("N-Active"),       K2,  0},

	{x_("5.7b"),       AC,        SPACINGM, x_("Poly-Cut"),       x_("P-Active"),       K3,  0},
	{x_("5.7b"),       AC,        SPACINGM, x_("Poly-Cut"),       x_("N-Active"),       K3,  0},

	{x_("6.1"),  0,               MINWID,   x_("Active-Cut"),      0,                   K2,  0},

	{x_("6.2"),        NAC,       NODSIZ,    0,                    0,                   K5, x_("Metal-1-P-Active-Con")},
	{x_("6.2"),        NAC,       SURROUND, x_("P-Active"),       x_("Metal-1"),        H0, x_("Metal-1-P-Active-Con")},
	{x_("6.2"),        NAC,       SURROUND, x_("P-Select"),       x_("P-Active"),       K2, x_("Metal-1-P-Active-Con")},
	{x_("6.2"),  DE|SU|NAC,       SURROUND, x_("N-Well"),         x_("P-Active"),       K6, x_("Metal-1-P-Active-Con")},
	{x_("6.2"),  SC|   NAC,       SURROUND, x_("N-Well"),         x_("P-Active"),       K5, x_("Metal-1-P-Active-Con")},
	{x_("6.2"),        NAC,       CUTSUR,    0,                    0,                   H1, x_("Metal-1-P-Active-Con")},
	{x_("6.2b"),       AC,        NODSIZ,    0,                    0,                   K4, x_("Metal-1-P-Active-Con")},
	{x_("6.2b"),       AC,        SURROUND, x_("P-Active"),       x_("Metal-1"),        0,  x_("Metal-1-P-Active-Con")},
	{x_("6.2b"),       AC,        SURROUND, x_("P-Select"),       x_("P-Active"),       K2, x_("Metal-1-P-Active-Con")},
	{x_("6.2b"), DE|SU|AC,        SURROUND, x_("N-Well"),         x_("P-Active"),       K6, x_("Metal-1-P-Active-Con")},
	{x_("6.2b"), SC|   AC,        SURROUND, x_("N-Well"),         x_("P-Active"),       K5, x_("Metal-1-P-Active-Con")},
	{x_("6.2b"),       AC,        CUTSUR,    0,                    0,                   K1, x_("Metal-1-P-Active-Con")},

	{x_("6.2"),        NAC,       NODSIZ,    0,                    0,                   K5, x_("Metal-1-N-Active-Con")},
	{x_("6.2"),        NAC,       SURROUND, x_("N-Active"),       x_("Metal-1"),        H0, x_("Metal-1-N-Active-Con")},
	{x_("6.2"),        NAC,       SURROUND, x_("N-Select"),       x_("N-Active"),       K2, x_("Metal-1-N-Active-Con")},
	{x_("6.2"),  DE|SU|NAC,       SURROUND, x_("P-Well"),         x_("N-Active"),       K6, x_("Metal-1-N-Active-Con")},
	{x_("6.2"),  SC|   NAC,       SURROUND, x_("P-Well"),         x_("N-Active"),       K5, x_("Metal-1-N-Active-Con")},
	{x_("6.2"),        NAC,       CUTSUR,    0,                    0,                   H1, x_("Metal-1-N-Active-Con")},
	{x_("6.2b"),       AC,        NODSIZ,    0,                    0,                   K4, x_("Metal-1-N-Active-Con")},
	{x_("6.2b"),       AC,        SURROUND, x_("N-Active"),       x_("Metal-1"),        0,  x_("Metal-1-N-Active-Con")},
	{x_("6.2b"),       AC,        SURROUND, x_("N-Select"),       x_("N-Active"),       K2, x_("Metal-1-N-Active-Con")},
	{x_("6.2b"), DE|SU|AC,        SURROUND, x_("P-Well"),         x_("N-Active"),       K6, x_("Metal-1-N-Active-Con")},
	{x_("6.2b"), SC|   AC,        SURROUND, x_("P-Well"),         x_("N-Active"),       K5, x_("Metal-1-N-Active-Con")},
	{x_("6.2b"),       AC,        CUTSUR,    0,                    0,                   K1, x_("Metal-1-N-Active-Con")},

	{x_("6.2"),        NAC,       NODSIZ,    0,                    0,                   K5, x_("Metal-1-P-Well-Con")},
	{x_("6.2"),        NAC,       SURROUND, x_("P-Active-Well"),  x_("Metal-1"),        H0, x_("Metal-1-P-Well-Con")},
	{x_("6.2"),        NAC,       SURROUND, x_("P-Select"),       x_("P-Active-Well"),  K2, x_("Metal-1-P-Well-Con")},
/* #define FIXWELLCONTACTS 1 */
#ifdef FIXWELLCONTACTS
	{x_("6.2"),        NAC,       SURROUND, x_("P-Well"),         x_("P-Active-Well"),  H3, x_("Metal-1-P-Well-Con")},
#else
	{x_("6.2"),        NAC,       SURROUND, x_("P-Well"),         x_("P-Active-Well"),  K3, x_("Metal-1-P-Well-Con")},
#endif
	{x_("6.2"),        NAC,       CUTSUR,    0,                    0,                   H1, x_("Metal-1-P-Well-Con")},
	{x_("6.2b"),       AC,        NODSIZ,    0,                    0,                   K4, x_("Metal-1-P-Well-Con")},
	{x_("6.2b"),       AC,        SURROUND, x_("P-Active-Well"),  x_("Metal-1"),        0,  x_("Metal-1-P-Well-Con")},
	{x_("6.2b"),       AC,        SURROUND, x_("P-Select"),       x_("P-Active-Well"),  K2, x_("Metal-1-P-Well-Con")},
#ifdef FIXWELLCONTACTS
	{x_("6.2b"),       AC,        SURROUND, x_("P-Well"),         x_("P-Active-Well"),  K4, x_("Metal-1-P-Well-Con")},
#else
	{x_("6.2b"),       AC,        SURROUND, x_("P-Well"),         x_("P-Active-Well"),  K3, x_("Metal-1-P-Well-Con")},
#endif
	{x_("6.2b"),       AC,        CUTSUR,    0,                    0,                   K1, x_("Metal-1-P-Well-Con")},

	{x_("6.2"),        NAC,       NODSIZ,    0,                    0,                   K5, x_("Metal-1-N-Well-Con")},
	{x_("6.2"),        NAC,       SURROUND, x_("N-Active"),       x_("Metal-1"),        H0, x_("Metal-1-N-Well-Con")},
	{x_("6.2"),        NAC,       SURROUND, x_("N-Select"),       x_("N-Active"),       K2, x_("Metal-1-N-Well-Con")},
#ifdef FIXWELLCONTACTS
	{x_("6.2"),        NAC,       SURROUND, x_("N-Well"),         x_("N-Active"),       H3, x_("Metal-1-N-Well-Con")},
#else
	{x_("6.2"),        NAC,       SURROUND, x_("N-Well"),         x_("N-Active"),       K3, x_("Metal-1-N-Well-Con")},
#endif
	{x_("6.2"),        NAC,       CUTSUR,    0,                    0,                   H1, x_("Metal-1-N-Well-Con")},
	{x_("6.2b"),       AC,        NODSIZ,    0,                    0,                   K4, x_("Metal-1-N-Well-Con")},
	{x_("6.2b"),       AC,        SURROUND, x_("N-Active"),       x_("Metal-1"),        0,  x_("Metal-1-N-Well-Con")},
	{x_("6.2b"),       AC,        SURROUND, x_("N-Select"),       x_("N-Active"),       K2, x_("Metal-1-N-Well-Con")},
#ifdef FIXWELLCONTACTS
	{x_("6.2b"),       AC,        SURROUND, x_("N-Well"),         x_("N-Active"),       K4, x_("Metal-1-N-Well-Con")},
#else
	{x_("6.2b"),       AC,        SURROUND, x_("N-Well"),         x_("N-Active"),       K3, x_("Metal-1-N-Well-Con")},
#endif
	{x_("6.2b"),       AC,        CUTSUR,    0,                    0,                   K1, x_("Metal-1-N-Well-Con")},

	{x_("6.3"),  DE,              CUTSPA,    0,                    0,                   K4, x_("Metal-1-P-Active-Con")},
	{x_("6.3"),  DE,              CUTSPA,    0,                    0,                   K4, x_("Metal-1-N-Active-Con")},
	{x_("6.3"),  DE,              SPACING,  x_("Active-Cut"),     x_("Active-Cut"),     K4,  0},
	{x_("6.3"),  SU,              CUTSPA,    0,                    0,                   K3, x_("Metal-1-P-Active-Con")},
	{x_("6.3"),  SU,              CUTSPA,    0,                    0,                   K3, x_("Metal-1-N-Active-Con")},
	{x_("6.3"),  SU,              SPACING,  x_("Active-Cut"),     x_("Active-Cut"),     K3,  0},
	{x_("6.3"),  SC,              CUTSPA,    0,                    0,                   K2, x_("Metal-1-P-Active-Con")},
	{x_("6.3"),  SC,              CUTSPA,    0,                    0,                   K2, x_("Metal-1-N-Active-Con")},
	{x_("6.3"),  SC,              SPACING,  x_("Active-Cut"),     x_("Active-Cut"),     K2,  0},

	{x_("6.4"),  0,               SPACING,  x_("Active-Cut"),     x_("Transistor-Poly"),K2,  0},

	{x_("6.5b"),       AC,        UCONSPA,  x_("Active-Cut"),     x_("P-Active"),       K5,  0},
	{x_("6.5b"),       AC,        UCONSPA,  x_("Active-Cut"),     x_("N-Active"),       K5,  0},

	{x_("6.6b"),       AC,        SPACING,  x_("Active-Cut"),     x_("Polysilicon-1"),  K2,  0},
#if 0		/* MOSIS agrees that this rule need not be checked */
	{x_("6.7b"),       AC,        SPACINGM, x_("Active-Cut"),     x_("Polysilicon-1"),  K3,  0},
#endif
	{x_("6.8b"),       AC,        SPACING,  x_("Active-Cut"),     x_("Poly-Cut"),       K4,  0},

	{x_("7.1"),  0,               MINWID,   x_("Metal-1"),         0,                   K3,  0},

	{x_("7.2"),  DE|SU,           SPACING,  x_("Metal-1"),        x_("Metal-1"),        K3,  0},
	{x_("7.2"),  SC,              SPACING,  x_("Metal-1"),        x_("Metal-1"),        K2,  0},

	{x_("7.4"),  DE|SU,           SPACINGW, x_("Metal-1"),        x_("Metal-1"),        K6,  0},
	{x_("7.4"),  SC,              SPACINGW, x_("Metal-1"),        x_("Metal-1"),        K4,  0},

	{x_("8.1"),  DE,              CUTSIZE,   0,                    0,                   K3, x_("Metal-1-Metal-2-Con")},
	{x_("8.1"),  DE,              NODSIZ,    0,                    0,                   K5, x_("Metal-1-Metal-2-Con")},
	{x_("8.1"),  SU|SC,           CUTSIZE,   0,                    0,                   K2, x_("Metal-1-Metal-2-Con")},
	{x_("8.1"),  SU|SC,           NODSIZ,    0,                    0,                   K4, x_("Metal-1-Metal-2-Con")},

	{x_("8.2"),  0,               SPACING,  x_("Via1"),           x_("Via1"),           K3,  0},

	{x_("8.3"),  0,               VIASUR,   x_("Metal-1"),         0,                   K1, x_("Metal-1-Metal-2-Con")},

	{x_("8.4"),        NSV,       SPACING,  x_("Poly-Cut"),       x_("Via1"),           K2,  0},
	{x_("8.4"),        NSV,       SPACING,  x_("Active-Cut"),     x_("Via1"),           K2,  0},

	{x_("8.5"),        NSV,       SPACINGE, x_("Via1"),           x_("Polysilicon-1"),  K2,  0},
	{x_("8.5"),        NSV,       SPACINGE, x_("Via1"),           x_("Transistor-Poly"),K2,  0},
	{x_("8.5"),        NSV,       SPACINGE, x_("Via1"),           x_("Polysilicon-2"),  K2,  0},
	{x_("8.5"),        NSV,       SPACINGE, x_("Via1"),           x_("P-Active"),       K2,  0},
	{x_("8.5"),        NSV,       SPACINGE, x_("Via1"),           x_("N-Active"),       K2,  0},

	{x_("9.1"),  0,               MINWID,   x_("Metal-2"),         0,                   K3,  0},

	{x_("9.2"),  DE,              SPACING,  x_("Metal-2"),        x_("Metal-2"),        K4,  0},
	{x_("9.2"),  SU|SC,           SPACING,  x_("Metal-2"),        x_("Metal-2"),        K3,  0},

	{x_("9.3"),  0,               VIASUR,   x_("Metal-2"),         0,                   K1, x_("Metal-1-Metal-2-Con")},

	{x_("9.4"),  DE,              SPACINGW, x_("Metal-2"),        x_("Metal-2"),        K8,  0},
	{x_("9.4"),  SU|SC,           SPACINGW, x_("Metal-2"),        x_("Metal-2"),        K6,  0},

	{x_("11.1"), SU,              MINWID,   x_("Polysilicon-2"),   0,                   K7,  0},
	{x_("11.1"), SC,              MINWID,   x_("Polysilicon-2"),   0,                   K3,  0},

	{x_("11.2"), 0,               SPACING,  x_("Polysilicon-2"),  x_("Polysilicon-2"),  K3,  0},

	{x_("11.3"), SU,              SURROUND, x_("Polysilicon-2"),  x_("Polysilicon-1"),  K5, x_("Metal-1-Polysilicon-1-2-Con")},
	{x_("11.3"), SU,              NODSIZ,    0,                    0,                   K15,x_("Metal-1-Polysilicon-1-2-Con")},
	{x_("11.3"), SU,              CUTSUR,    0,                    0,                   H6, x_("Metal-1-Polysilicon-1-2-Con")},
	{x_("11.3"), SC,              SURROUND, x_("Polysilicon-2"),  x_("Polysilicon-1"),  K2, x_("Metal-1-Polysilicon-1-2-Con")},
	{x_("11.3"), SC,              NODSIZ,    0,                    0,                   K9, x_("Metal-1-Polysilicon-1-2-Con")},
	{x_("11.3"), SC,              CUTSUR,    0,                    0,                   H3, x_("Metal-1-Polysilicon-1-2-Con")},

	{x_("14.1"), DE,              CUTSIZE,   0,                    0,                   K3, x_("Metal-2-Metal-3-Con")},
	{x_("14.1"), DE,              MINWID,   x_("Via2"),            0,                   K3,  0},
	{x_("14.1"), DE,              NODSIZ,    0,                    0,                   K5, x_("Metal-2-Metal-3-Con")},
	{x_("14.1"), SU|SC,           CUTSIZE,   0,                    0,                   K2, x_("Metal-2-Metal-3-Con")},
	{x_("14.1"), SU|SC,           MINWID,   x_("Via2"),            0,                   K2,  0},
	{x_("14.1"), SU|SC|    M23,   NODSIZ,    0,                    0,                   K6, x_("Metal-2-Metal-3-Con")},
	{x_("14.1"), SU|SC|    M456,  NODSIZ,    0,                    0,                   K4, x_("Metal-2-Metal-3-Con")},

	{x_("14.2"), 0,               SPACING,  x_("Via2"),           x_("Via2"),           K3,  0},

	{x_("14.3"), 0,               VIASUR,   x_("Metal-2"),         0,                   K1, x_("Metal-2-Metal-3-Con")},

	{x_("14.4"), SU|SC|NSV,       SPACING,  x_("Via1"),           x_("Via2"),           K2,  0},

	{x_("15.1"), SC|       M3,    MINWID,   x_("Metal-3"),         0,                   K6,  0},
	{x_("15.1"), SU|       M3,    MINWID,   x_("Metal-3"),         0,                   K5,  0},
	{x_("15.1"), SC|       M456,  MINWID,   x_("Metal-3"),         0,                   K3,  0},
	{x_("15.1"), SU|       M456,  MINWID,   x_("Metal-3"),         0,                   K3,  0},
	{x_("15.1"), DE,              MINWID,   x_("Metal-3"),         0,                   K3,  0},

	{x_("15.2"), DE,              SPACING,  x_("Metal-3"),        x_("Metal-3"),        K4,  0},
	{x_("15.2"), SU,              SPACING,  x_("Metal-3"),        x_("Metal-3"),        K3,  0},
	{x_("15.2"), SC|       M3,    SPACING,  x_("Metal-3"),        x_("Metal-3"),        K4,  0},
	{x_("15.2"), SC|       M456,  SPACING,  x_("Metal-3"),        x_("Metal-3"),        K3,  0},

	{x_("15.3"), DE,              VIASUR,   x_("Metal-3"),         0,                   K1, x_("Metal-2-Metal-3-Con")},
	{x_("15.3"), SU|SC|    M3,    VIASUR,   x_("Metal-3"),         0,                   K2, x_("Metal-2-Metal-3-Con")},
	{x_("15.3"), SU|SC|    M456,  VIASUR,   x_("Metal-3"),         0,                   K1, x_("Metal-2-Metal-3-Con")},

	{x_("15.4"), DE,              SPACINGW, x_("Metal-3"),        x_("Metal-3"),        K8,  0},
	{x_("15.4"), SU,              SPACINGW, x_("Metal-3"),        x_("Metal-3"),        K6,  0},
	{x_("15.4"), SC|       M3,    SPACINGW, x_("Metal-3"),        x_("Metal-3"),        K8,  0},
	{x_("15.4"), SC|       M456,  SPACINGW, x_("Metal-3"),        x_("Metal-3"),        K6,  0},

	{x_("21.1"), DE,              CUTSIZE,   0,                    0,                   K3, x_("Metal-3-Metal-4-Con")},
	{x_("21.1"), DE,              MINWID,   x_("Via3"),            0,                   K3,  0},
	{x_("21.1"), DE,              NODSIZ,    0,                    0,                   K5, x_("Metal-3-Metal-4-Con")},
	{x_("21.1"), SU|SC,           CUTSIZE,   0,                    0,                   K2, x_("Metal-3-Metal-4-Con")},
	{x_("21.1"), SU|SC,           MINWID,   x_("Via3"),            0,                   K2,  0},
	{x_("21.1"), SU|       M4,    NODSIZ,    0,                    0,                   K6, x_("Metal-3-Metal-4-Con")},
	{x_("21.1"), SU|       M56,   NODSIZ,    0,                    0,                   K4, x_("Metal-3-Metal-4-Con")},
	{x_("21.1"), SC,              NODSIZ,    0,                    0,                   K6, x_("Metal-3-Metal-4-Con")},

	{x_("21.2"), 0,               SPACING,  x_("Via3"),           x_("Via3"),           K3,  0},

	{x_("21.3"), 0,               VIASUR,   x_("Metal-3"),         0,                   K1, x_("Metal-3-Metal-4-Con")},

	{x_("22.1"),           M4,    MINWID,   x_("Metal-4"),         0,                   K6,  0},
	{x_("22.1"),           M56,   MINWID,   x_("Metal-4"),         0,                   K3,  0},

	{x_("22.2"),           M4,    SPACING,  x_("Metal-4"),        x_("Metal-4"),        K6,  0},
	{x_("22.2"), DE|       M56,   SPACING,  x_("Metal-4"),        x_("Metal-4"),        K4,  0},
	{x_("22.2"), SU|       M56,   SPACING,  x_("Metal-4"),        x_("Metal-4"),        K3,  0},

	{x_("22.3"),           M4,    VIASUR,   x_("Metal-4"),         0,                   K2, x_("Metal-3-Metal-4-Con")},
	{x_("22.3"),           M56,   VIASUR,   x_("Metal-4"),         0,                   K1, x_("Metal-3-Metal-4-Con")},

	{x_("22.4"),           M4,    SPACINGW, x_("Metal-4"),        x_("Metal-4"),        K12, 0},
	{x_("22.4"), DE|       M56,   SPACINGW, x_("Metal-4"),        x_("Metal-4"),        K8,  0},
	{x_("22.4"), SU|       M56,   SPACINGW, x_("Metal-4"),        x_("Metal-4"),        K6,  0},

	{x_("25.1"), DE,              CUTSIZE,   0,                    0,                   K3, x_("Metal-4-Metal-5-Con")},
	{x_("25.1"), DE,              MINWID,   x_("Via4"),            0,                   K3,  0},
	{x_("25.1"), SU,              CUTSIZE,   0,                    0,                   K2, x_("Metal-4-Metal-5-Con")},
	{x_("25.1"), SU,              MINWID,   x_("Via4"),            0,                   K2,  0},
	{x_("25.1"), SU,              NODSIZ,    0,                    0,                   K4, x_("Metal-4-Metal-5-Con")},
	{x_("25.1"), DE|       M5,    NODSIZ,    0,                    0,                   K7, x_("Metal-4-Metal-5-Con")},
	{x_("25.1"), DE|       M6,    NODSIZ,    0,                    0,                   K5, x_("Metal-4-Metal-5-Con")},

	{x_("25.2"), 0,               SPACINGW, x_("Via4"),           x_("Via4"),           K3,  0},

	{x_("25.3"), 0,               VIASUR,   x_("Metal-4"),         0,                   K1, x_("Metal-4-Metal-5-Con")},

	{x_("26.1"),           M5,    MINWID,   x_("Metal-5"),         0,                   K4,  0},
	{x_("26.1"),           M6,    MINWID,   x_("Metal-5"),         0,                   K3,  0},

	{x_("26.2"),           M5,    SPACING,  x_("Metal-5"),        x_("Metal-5"),        K4,  0},
	{x_("26.2"), DE|       M6,    SPACING,  x_("Metal-5"),        x_("Metal-5"),        K4,  0},
	{x_("26.2"), SU|       M6,    SPACING,  x_("Metal-5"),        x_("Metal-5"),        K3,  0},

	{x_("26.3"), DE|       M5,    VIASUR,   x_("Metal-5"),         0,                   K2, x_("Metal-4-Metal-5-Con")},
	{x_("26.3"), SU|       M5,    VIASUR,   x_("Metal-5"),         0,                   K1, x_("Metal-4-Metal-5-Con")},
	{x_("26.3"),           M6,    VIASUR,   x_("Metal-5"),         0,                   K1, x_("Metal-4-Metal-5-Con")},

	{x_("26.4"),           M5,    SPACINGW, x_("Metal-5"),        x_("Metal-5"),        K8,  0},
	{x_("26.4"), DE|       M6,    SPACINGW, x_("Metal-5"),        x_("Metal-5"),        K8,  0},
	{x_("26.4"), SU|       M6,    SPACINGW, x_("Metal-5"),        x_("Metal-5"),        K6,  0},

	{x_("29.1"), DE,              CUTSIZE,   0,                    0,                   K4, x_("Metal-5-Metal-6-Con")},
	{x_("29.1"), DE,              MINWID,   x_("Via5"),            0,                   K4,  0},
	{x_("29.1"), DE,              NODSIZ,    0,                    0,                   K8, x_("Metal-5-Metal-6-Con")},
	{x_("29.1"), SU,              CUTSIZE,   0,                    0,                   K3, x_("Metal-5-Metal-6-Con")},
	{x_("29.1"), SU,              MINWID,   x_("Via5"),            0,                   K3,  0},
	{x_("29.1"), SU,              NODSIZ,    0,                    0,                   K5, x_("Metal-5-Metal-6-Con")},

	{x_("29.2"), 0,               SPACING,  x_("Via5"),           x_("Via5"),           K4,  0},

	{x_("29.3"), 0,               VIASUR,   x_("Metal-5"),         0,                   K1, x_("Metal-5-Metal-6-Con")},

	{x_("30.1"), 0,               MINWID,   x_("Metal-6"),         0,                   K4,  0},

	{x_("30.2"), 0,               SPACING,  x_("Metal-6"),        x_("Metal-6"),        K4,  0},

	{x_("30.3"), DE,              VIASUR,   x_("Metal-6"),         0,                   K2, x_("Metal-5-Metal-6-Con")},
	{x_("30.3"), SU,              VIASUR,   x_("Metal-6"),         0,                   K1, x_("Metal-5-Metal-6-Con")},

	{x_("30.4"), 0,               SPACINGW, x_("Metal-6"),        x_("Metal-6"),        K8,  0},

	{0,      0,               0,             0,                    0,                   0,   0}
};

/******************** ARCS ********************/

#define ARCPROTOCOUNT 11
#define AMETAL1        0	/* metal-1                   */
#define AMETAL2        1	/* metal-2                   */
#define AMETAL3        2	/* metal-3                   */
#define AMETAL4        3	/* metal-4                   */
#define AMETAL5        4	/* metal-5                   */
#define AMETAL6        5	/* metal-6                   */
#define APOLY1         6	/* polysilicon-1             */
#define APOLY2         7	/* polysilicon-2             */
#define APACT          8	/* P-active                  */
#define ANACT          9	/* N-active                  */
#define AACT          10	/* General active            */

/* metal 1 arc */
static TECH_ARCLAY mocmos_al_m1[] = {{LMETAL1,0,FILLED }};
static TECH_ARCS mocmos_a_m1 = {
	x_("Metal-1"),K3,AMETAL1,NOARCPROTO,							/* name */
	1,mocmos_al_m1,													/* layers */
	(APMETAL1<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* metal 2 arc */
static TECH_ARCLAY mocmos_al_m2[] = {{LMETAL2,0,FILLED }};
static TECH_ARCS mocmos_a_m2 = {
	x_("Metal-2"),K3,AMETAL2,NOARCPROTO,							/* name */
	1,mocmos_al_m2,													/* layers */
	(APMETAL2<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* metal 3 arc */
static TECH_ARCLAY mocmos_al_m3[] = {{LMETAL3,0,FILLED }};
static TECH_ARCS mocmos_a_m3 = {
	x_("Metal-3"),K3,AMETAL3,NOARCPROTO,							/* name */
	1,mocmos_al_m3,													/* layers */
	(APMETAL3<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* metal 4 arc */
static TECH_ARCLAY mocmos_al_m4[] = {{LMETAL4,0,FILLED }};
static TECH_ARCS mocmos_a_m4 = {
	x_("Metal-4"),K3,AMETAL4,NOARCPROTO,							/* name */
	1,mocmos_al_m4,													/* layers */
	(APMETAL4<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* metal 5 arc */
static TECH_ARCLAY mocmos_al_m5[] = {{LMETAL5,0,FILLED }};
static TECH_ARCS mocmos_a_m5 = {
	x_("Metal-5"),K3,AMETAL5,NOARCPROTO,							/* name */
	1,mocmos_al_m5,													/* layers */
	(APMETAL5<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)|ANOTUSED};	/* userbits */

/* metal 6 arc */
static TECH_ARCLAY mocmos_al_m6[] = {{LMETAL6,0,FILLED }};
static TECH_ARCS mocmos_a_m6 = {
	x_("Metal-6"),K4,AMETAL6,NOARCPROTO,							/* name */
	1,mocmos_al_m6,													/* layers */
	(APMETAL6<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)|ANOTUSED};	/* userbits */

/* polysilicon 1 arc */
static TECH_ARCLAY mocmos_al_p1[] = {{LPOLY1,0,FILLED }};
static TECH_ARCS mocmos_a_po1 = {
	x_("Polysilicon-1"),K2,APOLY1,NOARCPROTO,						/* name */
	1,mocmos_al_p1,													/* layers */
	(APPOLY1<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* polysilicon 2 arc */
static TECH_ARCLAY mocmos_al_p2[] = {{LPOLY2,0,FILLED }};
static TECH_ARCS mocmos_a_po2 = {
	x_("Polysilicon-2"),K7,APOLY2,NOARCPROTO,						/* name */
	1,mocmos_al_p2,													/* layers */
	(APPOLY2<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* P-active arc */
static TECH_ARCLAY mocmos_al_pa[] = {{LPACT,K12,FILLED}, {LWELLN,0,FILLED},
	{LSELECTP,K8,FILLED}};
static TECH_ARCS mocmos_a_pa = {
	x_("P-Active"),K15,APACT,NOARCPROTO,							/* name */
	3,mocmos_al_pa,													/* layers */
	(APDIFFP<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* N-active arc */
static TECH_ARCLAY mocmos_al_na[] = {{LNACT,K12,FILLED}, {LWELLP,0,FILLED},
	{LSELECTN,K8,FILLED}};
static TECH_ARCS mocmos_a_na = {
	x_("N-Active"),K15,ANACT,NOARCPROTO,							/* name */
	3,mocmos_al_na,													/* layers */
	(APDIFFN<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* General active arc */
static TECH_ARCLAY mocmos_al_a[] = {{LNACT,0,FILLED}, {LPACT,0,FILLED}};
static TECH_ARCS mocmos_a_a = {
	x_("Active"),K3,AACT,NOARCPROTO,								/* name */
	2,mocmos_al_a,													/* layers */
	(APDIFF<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|ANOTUSED|(90<<AANGLEINCSH)};	/* userbits */

TECH_ARCS *mocmos_arcprotos[ARCPROTOCOUNT+1] = {
	&mocmos_a_m1, &mocmos_a_m2, &mocmos_a_m3,
	&mocmos_a_m4, &mocmos_a_m5, &mocmos_a_m6,
	&mocmos_a_po1, &mocmos_a_po2,
	&mocmos_a_pa, &mocmos_a_na,
	&mocmos_a_a, ((TECH_ARCS *)-1)};

static INTBIG mocmos_arc_widoff[ARCPROTOCOUNT] = {0,0,0,0,0,0, 0,0, K12,K12, 0};

/******************** PORTINST CONNECTIONS ********************/

static INTBIG mocmos_pc_m1[]   = {-1, AMETAL1, ALLGEN, -1};
static INTBIG mocmos_pc_m1a[]  = {-1, AMETAL1, AACT, ALLGEN, -1};
static INTBIG mocmos_pc_m2[]   = {-1, AMETAL2, ALLGEN, -1};
static INTBIG mocmos_pc_m3[]   = {-1, AMETAL3, ALLGEN, -1};
static INTBIG mocmos_pc_m4[]   = {-1, AMETAL4, ALLGEN, -1};
static INTBIG mocmos_pc_m5[]   = {-1, AMETAL5, ALLGEN, -1};
static INTBIG mocmos_pc_m6[]   = {-1, AMETAL6, ALLGEN, -1};
static INTBIG mocmos_pc_p1[]   = {-1, APOLY1, ALLGEN, -1};
static INTBIG mocmos_pc_p2[]   = {-1, APOLY2, ALLGEN, -1};
static INTBIG mocmos_pc_pa[]   = {-1, APACT, ALLGEN, -1};
static INTBIG mocmos_pc_a[]    = {-1, AACT, APACT, ANACT, ALLGEN,-1};
static INTBIG mocmos_pc_na[]   = {-1, ANACT, ALLGEN, -1};
static INTBIG mocmos_pc_pam1[] = {-1, APACT, AMETAL1, ALLGEN, -1};
static INTBIG mocmos_pc_nam1[] = {-1, ANACT, AMETAL1, ALLGEN, -1};
static INTBIG mocmos_pc_pm1[]  = {-1, APOLY1, AMETAL1, ALLGEN, -1};
static INTBIG mocmos_pc_pm2[]  = {-1, APOLY2, AMETAL1, ALLGEN, -1};
static INTBIG mocmos_pc_pm12[] = {-1, APOLY1, APOLY2, AMETAL1, ALLGEN, -1};
static INTBIG mocmos_pc_m1m2[] = {-1, AMETAL1, AMETAL2, ALLGEN, -1};
static INTBIG mocmos_pc_m2m3[] = {-1, AMETAL2, AMETAL3, ALLGEN, -1};
static INTBIG mocmos_pc_m3m4[] = {-1, AMETAL3, AMETAL4, ALLGEN, -1};
static INTBIG mocmos_pc_m4m5[] = {-1, AMETAL4, AMETAL5, ALLGEN, -1};
static INTBIG mocmos_pc_m5m6[] = {-1, AMETAL5, AMETAL6, ALLGEN, -1};
static INTBIG mocmos_pc_null[] = {-1, ALLGEN, -1};

/******************** NODES ********************/

#define NODEPROTOCOUNT 55
#define NMETAL1P        1	/* metal-1 pin */
#define NMETAL2P        2	/* metal-2 pin */
#define NMETAL3P        3	/* metal-3 pin */
#define NMETAL4P        4	/* metal-4 pin */
#define NMETAL5P        5	/* metal-5 pin */
#define NMETAL6P        6	/* metal-6 pin */
#define NPOLY1P         7	/* polysilicon-1 pin */
#define NPOLY2P         8	/* polysilicon-2 pin */
#define NPACTP          9	/* P-active pin */
#define NNACTP         10	/* N-active pin */
#define NACTP          11	/* General active pin */
#define NMETPACTC      12	/* metal-1-P-active contact */
#define NMETNACTC      13	/* metal-1-N-active contact */
#define NMETPOLY1C     14	/* metal-1-polysilicon-1 contact */
#define NMETPOLY2C     15	/* metal-1-polysilicon-2 contact */
#define NMETPOLY12C    16	/* metal-1-polysilicon-1-2 capacitor/contact */
#define NTRANSP        17	/* P-transistor */
#define NTRANSN        18	/* N-transistor */
#define NSTRANSP       19	/* Scalable P-transistor */
#define NSTRANSN       20	/* Scalable N-transistor */
#define NVPNPN         21	/* Vertical PNP-transistor */
#define NVIA1          22	/* metal-1-metal-2 contact */
#define NVIA2          23	/* metal-2-metal-3 contact */
#define NVIA3          24	/* metal-3-metal-4 contact */
#define NVIA4          25	/* metal-4-metal-5 contact */
#define NVIA5          26	/* metal-5-metal-6 contact */
#define NPWBUT         27	/* metal-1-P-Well contact */
#define NNWBUT         28	/* metal-1-N-Well contact */
#define NMETAL1N       29	/* metal-1 node */
#define NMETAL2N       30	/* metal-2 node */
#define NMETAL3N       31	/* metal-3 node */
#define NMETAL4N       32	/* metal-4 node */
#define NMETAL5N       33	/* metal-5 node */
#define NMETAL6N       34	/* metal-6 node */
#define NPOLY1N        35	/* polysilicon-1 node */
#define NPOLY2N        36	/* polysilicon-2 node */
#define NPACTIVEN      37	/* P-active node */
#define NNACTIVEN      38	/* N-active node */
#define NSELECTPN      39	/* P-select node */
#define NSELECTNN      40	/* N-select node */
#define NPCUTN         41	/* poly cut node */
#define NACUTN         42	/* active cut node */
#define NVIA1N         43	/* via-1 node */
#define NVIA2N         44	/* via-2 node */
#define NVIA3N         45	/* via-3 node */
#define NVIA4N         46	/* via-4 node */
#define NVIA5N         47	/* via-5 node */
#define NWELLPN        48	/* P-well node */
#define NWELLNN        49	/* N-well node */
#define NPASSN         50	/* passivation node */
#define NPADFRN        51	/* pad frame node */
#define NPOLYCAPN      52	/* polysilicon-capacitor node */
#define NPACTWELLN     53	/* P-active-Well node */
#define NTRANSPN       54	/* transistor-poly node */
#define NSILBLOCKN     55	/* silicide-block node */

#define LEFTIN24   -H0, K24	/* 24.0  */
#define LEFTIN24H  -H0, H24	/* 24.5  */
#define RIGHTIN24   H0,-K24	/* 24.0  */
#define RIGHTIN24H  H0,-H24	/* 24.5  */
#define BOTIN24    -H0, K24	/* 24.0  */
#define BOTIN24H   -H0, H24	/* 24.5  */
#define TOPIN24     H0,-K24	/* 24.0  */
#define TOPIN24H    H0,-H24	/* 24.5  */

#define LEFTIN26   -H0, K26	/* 26.0  */
#define LEFTIN26H  -H0, H26	/* 26.5  */
#define RIGHTIN26   H0,-K26	/* 26.0  */
#define RIGHTIN26H  H0,-H26	/* 26.5  */
#define BOTIN26    -H0, K26	/* 26.0  */
#define BOTIN26H   -H0, H26	/* 26.5  */
#define TOPIN26     H0,-K26	/* 26.0  */
#define TOPIN26H    H0,-H26	/* 26.5  */

/* for geometry */
static INTBIG mocmos_cutbox[8]   = {LEFTIN1,  BOTIN1,   LEFTIN3,   BOTIN3};/*adjust*/
static INTBIG mocmos_fullbox[8]  = {LEFTEDGE, BOTEDGE,  RIGHTEDGE, TOPEDGE};
static INTBIG mocmos_in0hbox[8]  = {LEFTIN0H, BOTIN0H,  RIGHTIN0H, TOPIN0H};
static INTBIG mocmos_in1box[8]   = {LEFTIN1,  BOTIN1,   RIGHTIN1,  TOPIN1};
static INTBIG mocmos_in1hbox[8]  = {LEFTIN1H, BOTIN1H,  RIGHTIN1H, TOPIN1H};
static INTBIG mocmos_in2box[8]   = {LEFTIN2,  BOTIN2,   RIGHTIN2,  TOPIN2};
static INTBIG mocmos_in2hbox[8]  = {LEFTIN2H, BOTIN2H,  RIGHTIN2H, TOPIN2H};
static INTBIG mocmos_in3box[8]   = {LEFTIN3,  BOTIN3,   RIGHTIN3,  TOPIN3};
static INTBIG mocmos_in3hbox[8]  = {LEFTIN3H, BOTIN3H,  RIGHTIN3H, TOPIN3H};
static INTBIG mocmos_in4box[8]   = {LEFTIN4,  BOTIN4,   RIGHTIN4,  TOPIN4};
static INTBIG mocmos_in4hbox[8]  = {LEFTIN4H, BOTIN4H,  RIGHTIN4H, TOPIN4H};
static INTBIG mocmos_in5box[8]   = {LEFTIN5,  BOTIN5,   RIGHTIN5,  TOPIN5};
static INTBIG mocmos_in5hbox[8]  = {LEFTIN5H, BOTIN5H,  RIGHTIN5H, TOPIN5H};
static INTBIG mocmos_in6box[8]   = {LEFTIN6,  BOTIN6,   RIGHTIN6,  TOPIN6};
static INTBIG mocmos_in6hbox[8]  = {LEFTIN6H, BOTIN6H,  RIGHTIN6H, TOPIN6H};

static INTBIG mocmos_in13box[8]  = {LEFTIN13, BOTIN13,  RIGHTIN13, TOPIN13};
static INTBIG mocmos_in24box[8]  = {LEFTIN24, BOTIN24,  RIGHTIN24, TOPIN24};
static INTBIG mocmos_in26box[8]  = {LEFTIN26, BOTIN26,  RIGHTIN26, TOPIN26};
static INTBIG mocmos_in26hbox[8] = {LEFTIN26H,BOTIN26H, RIGHTIN26H,TOPIN26H};

static INTBIG mocmos_tinybox[8]  = {-H0, K27,-H0, K27, H0,-K27,H0,-K27};

static INTBIG mocmos_in3_2box[8] = {LEFTIN3,  BOTIN3,   RIGHTIN2,  TOPIN2};
static INTBIG mocmos_in3h_1hbox[8]={LEFTIN3H, BOTIN3H,  RIGHTIN1H, TOPIN1H};
static INTBIG mocmos_trabox[8]   = {LEFTIN6,  BOTIN7,   RIGHTIN6,  TOPIN7};
#ifdef CENTERACTIVE
static INTBIG mocmos_tra1box[8]  = {LEFTIN6,  CENTER,   RIGHTIN6,  TOPIN7};
static INTBIG mocmos_tra2box[8]  = {LEFTIN6,  BOTIN7,   RIGHTIN6,  CENTER};
#else
static INTBIG mocmos_tra1box[8]  = {LEFTIN6,  TOPIN10,  RIGHTIN6,  TOPIN7};
static INTBIG mocmos_tra2box[8]  = {LEFTIN6,  BOTIN7,   RIGHTIN6,  BOTIN10};
static INTBIG mocmos_trpobox[8]  = {LEFTIN6,  BOTIN10,  RIGHTIN6,  TOPIN10};
static INTBIG mocmos_trp1box[8]  = {LEFTIN4,  BOTIN10,  LEFTIN6,   TOPIN10};
static INTBIG mocmos_trp2box[8]  = {RIGHTIN6, BOTIN10,  RIGHTIN4,  TOPIN10};
#endif
static INTBIG mocmos_trwbox[8]   = {LEFTEDGE, BOTIN1,   RIGHTEDGE, TOPIN1};
static INTBIG mocmos_trsbox[8]   = {LEFTIN4,  BOTIN5,   RIGHTIN4,  TOPIN5};
static INTBIG mocmos_trpbox[8]   = {LEFTIN4,  BOTIN10,  RIGHTIN4,  TOPIN10};

static INTBIG mocmos_trsabox0[8] = {LEFTIN7,  BOTIN9,   RIGHTIN7,  TOPIN9};
static INTBIG mocmos_trspbox[8]  = {LEFTIN5,  BOTIN12,  RIGHTIN5,  TOPIN12};
static INTBIG mocmos_trssbox[8]  = {LEFTIN4,  BOTIN4,   RIGHTIN4,  TOPIN4};
static INTBIG mocmos_trsabox1[8] = {LEFTIN6,  BOTIN11,  RIGHTIN6,  BOTIN6};
static INTBIG mocmos_trsmbox1[8] = {LEFTIN6H, BOTIN10H, RIGHTIN6H, BOTIN6H};
static INTBIG mocmos_trscbox1[8] = {LEFTIN7H, BOTIN9H,  LEFTIN9H,  BOTIN7H};
static INTBIG mocmos_trsabox2[8] = {LEFTIN6,  TOPIN6,   RIGHTIN6,  TOPIN11};
static INTBIG mocmos_trsmbox2[8] = {LEFTIN6H, TOPIN6H,  RIGHTIN6H, TOPIN10H};
static INTBIG mocmos_trscbox2[8] = {LEFTIN7H, TOPIN7H,  LEFTIN9H,  TOPIN9H};

/*
 * for MULTICUT:
 *   cut size is f1 x f2
 *   cut indented f3 from highlighting
 *   cuts spaced f4 apart
 *
 * for SERPTRANS:
 *   layer count is f1
 *   active port inset f2 from end of serpentine path
 *   active port is f3 from poly edge
 *   poly width is f4
 *   poly port inset f5 from poly edge
 *   poly port is f6 from active edge
 * also for SERPTRANS, the 4 extra fields of the TECH_SERPENT are:
 *   lwidth:  the extension of the polygon to the left of the centerline
 *   rwidth:  the extension of the polygon to the right
 *   extendt: the extension of the polygon on the top end of the serpentine path.
 *   extendb: the extension of the polygon on the bottom end of the serpentine path.
 */
#define WC 0
/* metal-1-pin */
static TECH_PORTS mocmos_pm1_p[] = {				/* ports */
	{mocmos_pc_m1, x_("metal-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos_pm1_l[] = {				/* layers */
	{LMET1P, 0, 4, CROSSED, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_pm1 = {
	x_("Metal-1-Pin"),NMETAL1P,NONODEPROTO,			/* name */
	K3,K3,											/* size */
	1,mocmos_pm1_p,									/* ports */
	1,mocmos_pm1_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* metal-2-pin */
static TECH_PORTS mocmos_pm2_p[] = {				/* ports */
	{mocmos_pc_m2, x_("metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos_pm2_l[] = {				/* layers */
	{LMET2P, 0, 4, CROSSED, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_pm2 = {
	x_("Metal-2-Pin"),NMETAL2P,NONODEPROTO,			/* name */
	K3,K3,											/* size */
	1,mocmos_pm2_p,									/* ports */
	1,mocmos_pm2_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* metal-3-pin */
static TECH_PORTS mocmos_pm3_p[] = {				/* ports */
	{mocmos_pc_m3, x_("metal-3"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos_pm3_l[] = {				/* layers */
	{LMET3P, 0, 4, CROSSED, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_pm3 = {
	x_("Metal-3-Pin"),NMETAL3P,NONODEPROTO,			/* name */
	K3,K3,											/* size */
	1,mocmos_pm3_p,									/* ports */
	1,mocmos_pm3_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* metal-4-pin */
static TECH_PORTS mocmos_pm4_p[] = {				/* ports */
	{mocmos_pc_m4, x_("metal-4"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos_pm4_l[] = {				/* layers */
	{LMET4P, 0, 4, CROSSED, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_pm4 = {
	x_("Metal-4-Pin"),NMETAL4P,NONODEPROTO,			/* name */
	K3,K3,											/* size */
	1,mocmos_pm4_p,									/* ports */
	1,mocmos_pm4_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* metal-5-pin */
static TECH_PORTS mocmos_pm5_p[] = {				/* ports */
	{mocmos_pc_m5, x_("metal-5"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos_pm5_l[] = {				/* layers */
	{LMET5P, 0, 4, CROSSED, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_pm5 = {
	x_("Metal-5-Pin"),NMETAL5P,NONODEPROTO,			/* name */
	K3,K3,											/* size */
	1,mocmos_pm5_p,									/* ports */
	1,mocmos_pm5_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK|NNOTUSED,	/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* metal-6-pin */
static TECH_PORTS mocmos_pm6_p[] = {				/* ports */
	{mocmos_pc_m6, x_("metal-6"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos_pm6_l[] = {				/* layers */
	{LMET6P, 0, 4, CROSSED, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_pm6 = {
	x_("Metal-6-Pin"),NMETAL6P,NONODEPROTO,			/* name */
	K3,K3,											/* size */
	1,mocmos_pm6_p,									/* ports */
	1,mocmos_pm6_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK|NNOTUSED,	/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* polysilicon-1-pin */
static TECH_PORTS mocmos_pp1_p[] = {				/* ports */
	{mocmos_pc_p1, x_("polysilicon-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON mocmos_pp1_l[] = {				/* layers */
	{LPOLY1P, 0, 4, CROSSED, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_pp1 = {
	x_("Polysilicon-1-Pin"),NPOLY1P,NONODEPROTO,	/* name */
	K2,K2,											/* size */
	1,mocmos_pp1_p,									/* ports */
	1,mocmos_pp1_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* polysilicon-2-pin */
static TECH_PORTS mocmos_pp2_p[] = {				/* ports */
	{mocmos_pc_p2, x_("polysilicon-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos_pp2_l[] = {				/* layers */
	{LPOLY2P, 0, 4, CROSSED, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_pp2 = {
	x_("Polysilicon-2-Pin"),NPOLY2P,NONODEPROTO,	/* name */
	K3,K3,											/* size */
	1,mocmos_pp2_p,									/* ports */
	1,mocmos_pp2_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* P-active-pin */
static TECH_PORTS mocmos_psa_p[] = {				/* ports */
	{mocmos_pc_pa, x_("p-active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN7H, BOTIN7H, RIGHTIN7H, TOPIN7H}};
static TECH_POLYGON mocmos_psa_l[] = {				/* layers */
	{LPACTP,    0, 4, CROSSED, BOX, mocmos_in6box},
	{LWELLNP,  WC, 4, CROSSED, BOX, mocmos_fullbox},
	{LSELECTPP,WC, 4, CROSSED, BOX, mocmos_in4box}};
static TECH_NODES mocmos_psa = {
	x_("P-Active-Pin"),NPACTP,NONODEPROTO,			/* name */
	K15,K15,										/* size */
	1,mocmos_psa_p,									/* ports */
	3,mocmos_psa_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* N-active-pin */
static TECH_PORTS mocmos_pda_p[] = {				/* ports */
	{mocmos_pc_na, x_("n-active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN7H, BOTIN7H, RIGHTIN7H, TOPIN7H}};
static TECH_POLYGON mocmos_pda_l[] = {				/* layers */
	{LNACTP,    0, 4, CROSSED, BOX, mocmos_in6box},
	{LWELLPP,  WC, 4, CROSSED, BOX, mocmos_fullbox},
	{LSELECTNP,WC, 4, CROSSED, BOX, mocmos_in4box}};
static TECH_NODES mocmos_pda = {
	x_("N-Active-Pin"),NNACTP,NONODEPROTO,			/* name */
	K15,K15,										/* size */
	1,mocmos_pda_p,									/* ports */
	3,mocmos_pda_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* General active-pin */
static TECH_PORTS mocmos_pa_p[] = {					/* ports */
	{mocmos_pc_a, x_("active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}}	;
static TECH_POLYGON mocmos_pa_l[] = {				/* layers */
	{LNACTP, 0, 4, CROSSED, BOX, mocmos_fullbox},
	{LPACTP, 0, 4, CROSSED, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_pa = {
	x_("Active-Pin"),NACTP,NONODEPROTO,				/* name */
	K3,K3,											/* size */
	1,mocmos_pa_p,									/* ports */
	2,mocmos_pa_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* metal-1-P-active-contact */
static TECH_PORTS mocmos_mpa_p[] = {				/* ports */
	{mocmos_pc_pam1, x_("metal-1-p-act"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN8, BOTIN8, RIGHTIN8, TOPIN8}};
static TECH_POLYGON mocmos_mpa_l[] = {				/* layers */
	{LMETAL1,  0, 4, FILLEDRECT, BOX, mocmos_in6hbox},
	{LPACT,    0, 4, FILLEDRECT, BOX, mocmos_in6box},
	{LWELLN,  WC, 4, FILLEDRECT, BOX, mocmos_fullbox},
	{LSELECTP,WC, 4, FILLEDRECT, BOX, mocmos_in4box},
	{LACTCUT,  0, 4, FILLEDRECT, BOX, mocmos_cutbox}};
static TECH_NODES mocmos_mpa = {
	x_("Metal-1-P-Active-Con"),NMETPACTC,NONODEPROTO,	/* name */
	K17,K17,										/* size */
	1,mocmos_mpa_p,									/* ports */
	5,mocmos_mpa_l,									/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,H1,K3,0,0,0,0};					/* characteristics */

/* metal-1-N-active-contact */
static TECH_PORTS mocmos_mna_p[] = {				/* ports */
	{mocmos_pc_nam1, x_("metal-1-n-act"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN8, BOTIN8, RIGHTIN8, TOPIN8}};
static TECH_POLYGON mocmos_mna_l[] = {				/* layers */
	{LMETAL1,   0, 4, FILLEDRECT, BOX, mocmos_in6hbox},
	{LNACT,     0, 4, FILLEDRECT, BOX, mocmos_in6box},
	{LWELLP,   WC, 4, FILLEDRECT, BOX, mocmos_fullbox},
	{LSELECTN, WC, 4, FILLEDRECT, BOX, mocmos_in4box},
	{LACTCUT,   0, 4, FILLEDRECT, BOX, mocmos_cutbox}};
static TECH_NODES mocmos_mna = {
	x_("Metal-1-N-Active-Con"),NMETNACTC,NONODEPROTO,	/* name */
	K17,K17,										/* size */
	1,mocmos_mna_p,									/* ports */
	5,mocmos_mna_l,									/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,H1,K3,0,0,0,0};					/* characteristics */

/* metal-1-polysilicon-1-contact */
static TECH_PORTS mocmos_mp1_p[] = {				/* ports */
	{mocmos_pc_pm1, x_("metal-1-polysilicon-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN2, BOTIN2, RIGHTIN2, TOPIN2}};
static TECH_POLYGON mocmos_mp1_l[] = {				/* layers */
	{LMETAL1,  0, 4, FILLEDRECT, BOX, mocmos_in0hbox},
	{LPOLY1,   0, 4, FILLEDRECT, BOX, mocmos_fullbox},
	{LPOLYCUT, 0, 4, FILLEDRECT, BOX, mocmos_cutbox}};
static TECH_NODES mocmos_mp1 = {
	x_("Metal-1-Polysilicon-1-Con"),NMETPOLY1C,NONODEPROTO,/* name */
	K5,K5,											/* size */
	1,mocmos_mp1_p,									/* ports */
	3,mocmos_mp1_l,									/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,H1,K3,0,0,0,0};					/* characteristics */

/* metal-1-polysilicon-2-contact */
static TECH_PORTS mocmos_mp2_p[] = {				/* ports */
	{mocmos_pc_pm2, x_("metal-1-polysilicon-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN4H, BOTIN4H, RIGHTIN4H, TOPIN4H}};
static TECH_POLYGON mocmos_mp2_l[] = {				/* layers */
	{LMETAL1,  0, 4, FILLEDRECT, BOX, mocmos_in3box},
	{LPOLY2,   0, 4, FILLEDRECT, BOX, mocmos_fullbox},
	{LPOLYCUT, 0, 4, FILLEDRECT, BOX, mocmos_cutbox}};
static TECH_NODES mocmos_mp2 = {
	x_("Metal-1-Polysilicon-2-Con"),NMETPOLY2C,NONODEPROTO,/* name */
	K10,K10,										/* size */
	1,mocmos_mp2_p,									/* ports */
	3,mocmos_mp2_l,									/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,K4,K3,0,0,0,0};					/* characteristics */

/* metal-1-polysilicon-1-2-contact */
static TECH_PORTS mocmos_mp12_p[] = {				/* ports */
	{mocmos_pc_pm12, x_("metal-1-polysilicon-1-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN7, BOTIN7, RIGHTIN7, TOPIN7}};
static TECH_POLYGON mocmos_mp12_l[] = {				/* layers */
	{LMETAL1,  0, 4, FILLEDRECT, BOX, mocmos_in5hbox},
	{LPOLY1,   0, 4, FILLEDRECT, BOX, mocmos_in5box},
	{LPOLY2,   0, 4, FILLEDRECT, BOX, mocmos_fullbox},
	{LPOLYCUT, 0, 4, FILLEDRECT, BOX, mocmos_cutbox}};
static TECH_NODES mocmos_mp12 = {
	x_("Metal-1-Polysilicon-1-2-Con"),NMETPOLY12C,NONODEPROTO,/* name */
	K15,K15,										/* size */
	1,mocmos_mp12_p,								/* ports */
	4,mocmos_mp12_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,H6,K3,0,0,0,0};					/* characteristics */

/* index numbers for the transistor layers in "mocmos_tpa_l" and "mocmos_tna_l" */
#define TRANSACTLAYER      0		/* layer for active */
#define TRANSPOLYLAYER     1		/* layer for polysilicon */
#define TRANSWELLLAYER     2		/* layer for well */
#define TRANSSELECTLAYER   3		/* layer for select */

/* index numbers for the transistor electrical layers in "mocmos_tpaE_l" and "mocmos_tnaE_l" */
#define TRANSEACT1LAYER    0		/* electrical layer for part 1 of active */
#define TRANSEACT2LAYER    1		/* electrical layer for part 2 of active */
#define TRANSEPOLY1LAYER   2		/* electrical layer for part 1 of polysilicon */
#define TRANSEPOLY2LAYER   3		/* electrical layer for part 2 of polysilicon */
#define TRANSEGPOLYLAYER   4		/* electrical layer for gate polysilicon */
#define TRANSEWELLLAYER    5		/* electrical layer for well */
#define TRANSESELECTLAYER  6		/* electrical layer for select */

/* P-transistor */
static TECH_PORTS mocmos_tpa_p[] = {				/* ports */
	{mocmos_pc_p1, x_("p-trans-poly-left"),  NOPORTPROTO, (180<<PORTANGLESH)|
		(90<<PORTARANGESH),                LEFTIN4,  BOTIN11, LEFTIN4,   TOPIN11},
	{mocmos_pc_pa, x_("p-trans-diff-top"),   NOPORTPROTO, (90<<PORTANGLESH)|
		(90<<PORTARANGESH)|(1<<PORTNETSH), LEFTIN7H,  TOPIN7H, RIGHTIN7H,  TOPIN7},
	{mocmos_pc_p1, x_("p-trans-poly-right"), NOPORTPROTO, (0<<PORTANGLESH)|
		(90<<PORTARANGESH),                RIGHTIN4, BOTIN11, RIGHTIN4,  TOPIN11},
	{mocmos_pc_pa, x_("p-trans-diff-bottom"),NOPORTPROTO, (270<<PORTANGLESH)|
		(90<<PORTARANGESH)|(2<<PORTNETSH), LEFTIN7H,  BOTIN7, RIGHTIN7H,  BOTIN7H}};
static TECH_SERPENT mocmos_tpa_l[] = {				/* graphical layers */
	{{LPACT,     1, 4, FILLEDRECT, BOX,    mocmos_trabox},  K4, K4,  0,  0},
	{{LTRANS,    0, 4, FILLEDRECT, BOX,    mocmos_trpbox},  K1, K1, K2, K2},
	{{LWELLN,   -1, 4, FILLEDRECT, BOX,    mocmos_trwbox}, K10,K10, K6, K6},
	{{LSELECTP, -1, 4, FILLEDRECT, BOX,    mocmos_trsbox},  K6, K6, K2, K2}};
static TECH_SERPENT mocmos_tpaE_l[] = {				/* electric layers */
#ifdef CENTERACTIVE
	{{LPACT,     1, 4, FILLEDRECT, BOX,    mocmos_tra1box}, K4,  0,  0,  0},
	{{LPACT,     3, 4, FILLEDRECT, BOX,    mocmos_tra2box},  0, K4,  0,  0},
	{{LTRANS,    0, 4, FILLEDRECT, BOX,    mocmos_trpbox},  K1, K1, K2, K2},
#else
	{{LPACT,     1, 4, FILLEDRECT, BOX,    mocmos_tra1box}, K4,-K1,  0,  0},
	{{LPACT,     3, 4, FILLEDRECT, BOX,    mocmos_tra2box},-K1, K4,  0,  0},
	{{LPOLY1,    0, 4, FILLEDRECT, BOX,    mocmos_trp1box}, K1, K1,  0, K2},
	{{LPOLY1,    0, 4, FILLEDRECT, BOX,    mocmos_trp2box}, K1, K1, K2,  0},
	{{LTRANS,    0, 4, FILLEDRECT, BOX,    mocmos_trpobox}, K1, K1,  0,  0},
#endif
	{{LWELLN,   -1, 4, FILLEDRECT, BOX,    mocmos_trwbox}, K10,K10, K6, K6},
	{{LSELECTP, -1, 4, FILLEDRECT, BOX,    mocmos_trsbox},  K6, K6, K2, K2}};
static TECH_NODES mocmos_tpa = {
	x_("P-Transistor"),NTRANSP,NONODEPROTO,			/* name */
	K15,K22,										/* size */
	4,mocmos_tpa_p,									/* ports */
	4,(TECH_POLYGON *)0,							/* layers */
	NODESHRINK|(NPTRAPMOS<<NFUNCTIONSH)|HOLDSTRACE,	/* userbits */
	SERPTRANS,7,H1,H2,K2,K1,K2,mocmos_tpa_l,mocmos_tpaE_l};/* characteristics */

/* N-transistor */
static TECH_PORTS mocmos_tna_p[] = {				/* ports */
	{mocmos_pc_p1, x_("n-trans-poly-left"),  NOPORTPROTO, (180<<PORTANGLESH)|
		(90<<PORTARANGESH),                LEFTIN4,  BOTIN11, LEFTIN4,   TOPIN11},
	{mocmos_pc_na, x_("n-trans-diff-top"),   NOPORTPROTO, (90<<PORTANGLESH)|
		(90<<PORTARANGESH)|(1<<PORTNETSH), LEFTIN7H,  TOPIN7H, RIGHTIN7H,  TOPIN7},
	{mocmos_pc_p1, x_("n-trans-poly-right"), NOPORTPROTO, (0<<PORTANGLESH)|
		(90<<PORTARANGESH),                RIGHTIN4, BOTIN11, RIGHTIN4,  TOPIN11},
	{mocmos_pc_na, x_("n-trans-diff-bottom"),NOPORTPROTO, (270<<PORTANGLESH)|
		(90<<PORTARANGESH)|(2<<PORTNETSH), LEFTIN7H,  BOTIN7, RIGHTIN7H,  BOTIN7H}};
static TECH_SERPENT mocmos_tna_l[] = {				/* graphical layers */
	{{LNACT,     1, 4, FILLEDRECT, BOX,    mocmos_trabox},  K4, K4,  0,  0},
	{{LTRANS,    0, 4, FILLEDRECT, BOX,    mocmos_trpbox},  K1, K1, K2, K2},
	{{LWELLP,   -1, 4, FILLEDRECT, BOX,    mocmos_trwbox}, K10,K10, K6, K6},
	{{LSELECTN, -1, 4, FILLEDRECT, BOX,    mocmos_trsbox},  K6, K6, K2, K2}};
static TECH_SERPENT mocmos_tnaE_l[] = {				/* electric layers */
#ifdef CENTERACTIVE
	{{LNACT,     1, 4, FILLEDRECT, BOX,    mocmos_tra1box}, K4,  0,  0,  0},
	{{LNACT,     3, 4, FILLEDRECT, BOX,    mocmos_tra2box},  0, K4,  0,  0},
	{{LTRANS,    0, 4, FILLEDRECT, BOX,    mocmos_trpbox},  K1, K1, K2, K2},
#else
	{{LNACT,     1, 4, FILLEDRECT, BOX,    mocmos_tra1box}, K4,-K1,  0,  0},
	{{LNACT,     3, 4, FILLEDRECT, BOX,    mocmos_tra2box},-K1, K4,  0,  0},
	{{LPOLY1,    0, 4, FILLEDRECT, BOX,    mocmos_trp1box}, K1, K1,  0, K2},
	{{LPOLY1,    0, 4, FILLEDRECT, BOX,    mocmos_trp2box}, K1, K1, K2,  0},
	{{LTRANS,    0, 4, FILLEDRECT, BOX,    mocmos_trpobox}, K1, K1,  0,  0},
#endif
	{{LWELLP,   -1, 4, FILLEDRECT, BOX,    mocmos_trwbox}, K10,K10, K6, K6},
	{{LSELECTN, -1, 4, FILLEDRECT, BOX,    mocmos_trsbox},  K6, K6, K2, K2}};
static TECH_NODES mocmos_tna = {
	x_("N-Transistor"),NTRANSN,NONODEPROTO,			/* name */
	K15,K22,										/* size */
	4,mocmos_tna_p,									/* ports */
	4,(TECH_POLYGON *)0,							/* layers */
	NODESHRINK|(NPTRANMOS<<NFUNCTIONSH)|HOLDSTRACE,	/* userbits */
	SERPTRANS,7,H1,H2,K2,K1,K2,mocmos_tna_l,mocmos_tnaE_l};/* characteristics */

/* Scalable P-transistor */
static TECH_PORTS mocmos_tpas_p[] = {				/* ports */
	{mocmos_pc_p1,   x_("p-trans-sca-poly-left"),  NOPORTPROTO, (180<<PORTANGLESH)|
		(90<<PORTARANGESH),                CENTERL3H, CENTER,    CENTERL3H, CENTER},
	{mocmos_pc_pam1, x_("p-trans-sca-diff-top"),   NOPORTPROTO, (90<<PORTANGLESH)|
		(90<<PORTARANGESH)|(1<<PORTNETSH), CENTER,    CENTERU4H, CENTER,    CENTERU4H},
	{mocmos_pc_p1,   x_("p-trans-sca-poly-right"), NOPORTPROTO, (0<<PORTANGLESH)|
		(90<<PORTARANGESH),                CENTERR3H, CENTER,    CENTERR3H, CENTER},
	{mocmos_pc_pam1, x_("p-trans-sca-diff-bottom"),NOPORTPROTO, (270<<PORTANGLESH)|
		(90<<PORTARANGESH)|(2<<PORTNETSH), CENTER,    CENTERD4H, CENTER,    CENTERD4H}};
static TECH_POLYGON mocmos_tpas_l[] = {				/* graphical layers */
	{LPACT,     1, 4, FILLEDRECT, BOX,    mocmos_trsabox2},
	{LMETAL1,   1, 4, FILLEDRECT, BOX,    mocmos_trsmbox2},
	{LPACT,     3, 4, FILLEDRECT, BOX,    mocmos_trsabox1},
	{LMETAL1,   3, 4, FILLEDRECT, BOX,    mocmos_trsmbox1},
	{LPACT,    -1, 4, FILLEDRECT, BOX,    mocmos_trsabox0},
	{LTRANS,    0, 4, FILLEDRECT, BOX,    mocmos_trspbox},
	{LWELLN,   -1, 4, FILLEDRECT, BOX,    mocmos_fullbox},
	{LSELECTP, -1, 4, FILLEDRECT, BOX,    mocmos_trssbox},
	{LACTCUT,  -1, 4, FILLEDRECT, BOX,    mocmos_trscbox1},
	{LACTCUT,  -1, 4, FILLEDRECT, BOX,    mocmos_trscbox2}};
static TECH_NODES mocmos_tpas = {
	x_("P-Transistor-Scalable"),NSTRANSP,NONODEPROTO,	/* name */
	K17,K26,										/* size */
	4,mocmos_tpas_p,								/* ports */
	10,mocmos_tpas_l,								/* layers */
	NODESHRINK|(NPTRAPMOS<<NFUNCTIONSH),			/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Scalable N-transistor */
static TECH_PORTS mocmos_tnas_p[] = {				/* ports */
	{mocmos_pc_p1,   x_("n-trans-sca-poly-left"),  NOPORTPROTO, (180<<PORTANGLESH)|
		(90<<PORTARANGESH),                CENTERL3H, CENTER,    CENTERL3H, CENTER},
	{mocmos_pc_nam1, x_("n-trans-sca-diff-top"),   NOPORTPROTO, (90<<PORTANGLESH)|
		(90<<PORTARANGESH)|(1<<PORTNETSH), CENTER,    CENTERU4H, CENTER,    CENTERU4H},
	{mocmos_pc_p1,   x_("n-trans-sca-poly-right"), NOPORTPROTO, (0<<PORTANGLESH)|
		(90<<PORTARANGESH),                CENTERR3H, CENTER,    CENTERR3H, CENTER},
	{mocmos_pc_nam1, x_("n-trans-sca-diff-bottom"),NOPORTPROTO, (270<<PORTANGLESH)|
		(90<<PORTARANGESH)|(2<<PORTNETSH), CENTER,    CENTERD4H, CENTER,    CENTERD4H}};
static TECH_POLYGON mocmos_tnas_l[] = {				/* graphical layers */
	{LNACT,     1, 4, FILLEDRECT, BOX,    mocmos_trsabox2},
	{LMETAL1,   1, 4, FILLEDRECT, BOX,    mocmos_trsmbox2},
	{LNACT,     3, 4, FILLEDRECT, BOX,    mocmos_trsabox1},
	{LMETAL1,   3, 4, FILLEDRECT, BOX,    mocmos_trsmbox1},
	{LNACT,    -1, 4, FILLEDRECT, BOX,    mocmos_trsabox0},
	{LTRANS,    0, 4, FILLEDRECT, BOX,    mocmos_trspbox},
	{LWELLP,   -1, 4, FILLEDRECT, BOX,    mocmos_fullbox},
	{LSELECTN, -1, 4, FILLEDRECT, BOX,    mocmos_trssbox},
	{LACTCUT,  -1, 4, FILLEDRECT, BOX,    mocmos_trscbox1},
	{LACTCUT,  -1, 4, FILLEDRECT, BOX,    mocmos_trscbox2}};
static TECH_NODES mocmos_tnas = {
	x_("N-Transistor-Scalable"),NSTRANSN,NONODEPROTO,	/* name */
	K17,K26,										/* size */
	4,mocmos_tnas_p,								/* ports */
	10,mocmos_tnas_l,								/* layers */
	NODESHRINK|(NPTRANMOS<<NFUNCTIONSH),			/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Vertical PNP-transistor */
static TECH_PORTS mocmos_vpnp_p[] = {				/* ports */
	{mocmos_pc_pam1, x_("emitter"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER},
	{mocmos_pc_pam1, x_("base"), NOPORTPROTO, (180<<PORTARANGESH)|(1<<PORTNETSH),
		CENTER, CENTER, CENTER, CENTER},
	{mocmos_pc_pam1, x_("collector"), NOPORTPROTO, (180<<PORTARANGESH)|(2<<PORTNETSH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON mocmos_vpnp_l[] = {				/* graphical layers */
	{LMETAL1,  0, 4, FILLEDRECT, BOX, mocmos_in26hbox},
	{LMETAL1,  0, 4, FILLEDRECT, BOX, mocmos_tinybox},	/* inner ring */
	{LMETAL1,  0, 4, FILLEDRECT, BOX, mocmos_tinybox},	/* outer ring */
	{LPACT,    0, 4, FILLEDRECT, BOX, mocmos_in26box},
	{LPACT,    0, 4, FILLEDRECT, BOX, mocmos_tinybox},	/* inner ring */
	{LPACT,    0, 4, FILLEDRECT, BOX, mocmos_tinybox},	/* outer ring */
	{LWELLN,  WC, 4, FILLEDRECT, BOX, mocmos_in13box},
	{LWELLP,   0, 4, FILLEDRECT, BOX, mocmos_tinybox},	/* ring */
	{LSELECTP,WC, 4, FILLEDRECT, BOX, mocmos_in24box},
	{LSELECTN, 0, 4, FILLEDRECT, BOX, mocmos_tinybox},	/* ring */
	{LSELECTP, 0, 4, FILLEDRECT, BOX, mocmos_tinybox},	/* ring */
	{LACTCUT,  0, 4, FILLEDRECT, BOX, mocmos_cutbox}};
static TECH_NODES mocmos_vpnp = {
	x_("Vertical-PNP-Transistor"),NVPNPN,NONODEPROTO,	/* name */
	K57,K57,										/* size */
	3,mocmos_vpnp_p,								/* ports */
	12,mocmos_vpnp_l,								/* layers */
	(NPTRAPNP<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,H23,K3,0,0,0,0};					/* characteristics */
/* for MULTICUT:
 *   cut size is f1 x f2
 *   cut indented f3 from highlighting
 *   cuts spaced f4 apart
 */

/* metal-1-metal-2-contact */
static TECH_PORTS mocmos_m1m2_p[] = {				/* ports */
	{mocmos_pc_m1m2, x_("metal-1-metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos_m1m2_l[] = {				/* layers */
	{LMETAL1, 0, 4, FILLEDRECT, BOX, mocmos_in0hbox},
	{LMETAL2, 0, 4, FILLEDRECT, BOX, mocmos_in0hbox},
	{LVIA1,   0, 4, FILLEDRECT, BOX, mocmos_cutbox}};
static TECH_NODES mocmos_m1m2 = {
	x_("Metal-1-Metal-2-Con"),NVIA1,NONODEPROTO,	/* name */
	K5,K5,											/* size */
	1,mocmos_m1m2_p,								/* ports */
	3,mocmos_m1m2_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,K1,K3,0,0,0,0};					/* characteristics */

/* metal-2-metal-3-contact */
static TECH_PORTS mocmos_m2m3_p[] = {				/* ports */
	{mocmos_pc_m2m3, x_("metal-2-metal-3"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN2H, BOTIN2H, RIGHTIN2H, TOPIN2H}};
static TECH_POLYGON mocmos_m2m3_l[] = {				/* layers */
	{LMETAL2, 0, 4, FILLEDRECT, BOX, mocmos_in1box},
	{LMETAL3, 0, 4, FILLEDRECT, BOX, mocmos_in1box},
	{LVIA2,   0, 4, FILLEDRECT, BOX, mocmos_cutbox}};
static TECH_NODES mocmos_m2m3 = {
	x_("Metal-2-Metal-3-Con"),NVIA2,NONODEPROTO,	/* name */
	K6,K6,											/* size */
	1,mocmos_m2m3_p,								/* ports */
	3,mocmos_m2m3_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,K1,K3,0,0,0,0};					/* characteristics */

/* metal-3-metal-4-contact */
static TECH_PORTS mocmos_m3m4_p[] = {				/* ports */
	{mocmos_pc_m3m4, x_("metal-3-metal-4"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN2H, BOTIN2H, RIGHTIN2H, TOPIN2H}};
static TECH_POLYGON mocmos_m3m4_l[] = {				/* layers */
	{LMETAL3, 0, 4, FILLEDRECT, BOX, mocmos_in1box},
	{LMETAL4, 0, 4, FILLEDRECT, BOX, mocmos_fullbox},
	{LVIA3,   0, 4, FILLEDRECT, BOX, mocmos_cutbox}};
static TECH_NODES mocmos_m3m4 = {
	x_("Metal-3-Metal-4-Con"),NVIA3,NONODEPROTO,	/* name */
	K6,K6,											/* size */
	1,mocmos_m3m4_p,								/* ports */
	3,mocmos_m3m4_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,K2,K3,0,0,0,0};					/* characteristics */

/* metal-4-metal-5-contact */
static TECH_PORTS mocmos_m4m5_p[] = {				/* ports */
	{mocmos_pc_m4m5, x_("metal-4-metal-5"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN2H, BOTIN2H, RIGHTIN2H, TOPIN2H}};
static TECH_POLYGON mocmos_m4m5_l[] = {				/* layers */
	{LMETAL4, 0, 4, FILLEDRECT, BOX, mocmos_in1hbox},
	{LMETAL5, 0, 4, FILLEDRECT, BOX, mocmos_in1hbox},
	{LVIA4,   0, 4, FILLEDRECT, BOX, mocmos_cutbox}};
static TECH_NODES mocmos_m4m5 = {
	x_("Metal-4-Metal-5-Con"),NVIA4,NONODEPROTO,	/* name */
	K7,K7,											/* size */
	1,mocmos_m4m5_p,								/* ports */
	3,mocmos_m4m5_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH)|NNOTUSED,				/* userbits */
	MULTICUT,K2,K2,K1,K3,0,0,0,0};					/* characteristics */

/* metal-5-metal-6-contact */
static TECH_PORTS mocmos_m5m6_p[] = {				/* ports */
	{mocmos_pc_m5m6, x_("metal-5-metal-6"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN2H, BOTIN2H, RIGHTIN2H, TOPIN2H}};
static TECH_POLYGON mocmos_m5m6_l[] = {				/* layers */
	{LMETAL5, 0, 4, FILLEDRECT, BOX, mocmos_in1box},
	{LMETAL6, 0, 4, FILLEDRECT, BOX, mocmos_in1box},
	{LVIA5,   0, 4, FILLEDRECT, BOX, mocmos_cutbox}};
static TECH_NODES mocmos_m5m6 = {
	x_("Metal-5-Metal-6-Con"),NVIA5,NONODEPROTO,	/* name */
	K8,K8,											/* size */
	1,mocmos_m5m6_p,								/* ports */
	3,mocmos_m5m6_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH)|NNOTUSED,				/* userbits */
	MULTICUT,K3,K3,K2,K4,0,0,0,0};					/* characteristics */

/* Metal-1-P-Well Contact */
static TECH_PORTS mocmos_psub_p[] = {				/* ports */
	{mocmos_pc_m1a, x_("metal-1-well"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN8, BOTIN8, RIGHTIN8, TOPIN8}};
static TECH_POLYGON mocmos_psub_l[] = {				/* layers */
	{LMETAL1,   0, 4, FILLEDRECT, BOX,    mocmos_in6hbox},
	{LPACTWELL, 0, 4, FILLEDRECT, BOX,    mocmos_in6box},
	{LWELLP,   WC, 4, FILLEDRECT, BOX,    mocmos_fullbox},
	{LSELECTP, WC, 4, FILLEDRECT, BOX,    mocmos_in4box},
	{LACTCUT,   0, 4, FILLEDRECT, BOX,    mocmos_cutbox}};
static TECH_NODES mocmos_psub = {
	x_("Metal-1-P-Well-Con"),NPWBUT,NONODEPROTO,	/* name */
	K17,K17,										/* size */
	1,mocmos_psub_p,								/* ports */
	5,mocmos_psub_l,								/* layers */
	(NPWELL<<NFUNCTIONSH),							/* userbits */
	MULTICUT,K2,K2,H1,K3,0,0,0,0};					/* characteristics */

/* Metal-1-N-Well Contact */
static TECH_PORTS mocmos_nsub_p[] = {				/* ports */
	{mocmos_pc_m1a, x_("metal-1-substrate"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN8, BOTIN8, RIGHTIN8, TOPIN8}};
static TECH_POLYGON mocmos_nsub_l[] = {				/* layers */
	{LMETAL1,   0, 4, FILLEDRECT, BOX,    mocmos_in6hbox},
	{LNACT,     0, 4, FILLEDRECT, BOX,    mocmos_in6box},
	{LWELLN,   WC, 4, FILLEDRECT, BOX,    mocmos_fullbox},
	{LSELECTN, WC, 4, FILLEDRECT, BOX,    mocmos_in4box},
	{LACTCUT,   0, 4, FILLEDRECT, BOX,    mocmos_cutbox}};
static TECH_NODES mocmos_nsub = {
	x_("Metal-1-N-Well-Con"),NNWBUT,NONODEPROTO,	/* name */
	K17,K17,										/* size */
	1,mocmos_nsub_p,								/* ports */
	5,mocmos_nsub_l,								/* layers */
	(NPSUBSTRATE<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,H1,K3,0,0,0,0};					/* characteristics */

/* Metal-1-Node */
static TECH_PORTS mocmos_m1_p[] = {					/* ports */
	{mocmos_pc_m1, x_("metal-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos_m1_l[] = {				/* layers */
	{LMETAL1, 0, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_m1 = {
	x_("Metal-1-Node"),NMETAL1N,NONODEPROTO,		/* name */
	K3,K3,											/* size */
	1,mocmos_m1_p,									/* ports */
	1,mocmos_m1_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Metal-2-Node */
static TECH_PORTS mocmos_m2_p[] = {					/* ports */
	{mocmos_pc_m2, x_("metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos_m2_l[] = {				/* layers */
	{LMETAL2, 0, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_m2 = {
	x_("Metal-2-Node"),NMETAL2N,NONODEPROTO,		/* name */
	K3,K3,											/* size */
	1,mocmos_m2_p,									/* ports */
	1,mocmos_m2_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Metal-3-Node */
static TECH_PORTS mocmos_m3_p[] = {					/* ports */
	{mocmos_pc_m3, x_("metal-3"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos_m3_l[] = {				/* layers */
	{LMETAL3, 0, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_m3 = {
	x_("Metal-3-Node"),NMETAL3N,NONODEPROTO,		/* name */
	K3,K3,											/* size */
	1,mocmos_m3_p,									/* ports */
	1,mocmos_m3_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Metal-4-Node */
static TECH_PORTS mocmos_m4_p[] = {					/* ports */
	{mocmos_pc_m4, x_("metal-4"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos_m4_l[] = {				/* layers */
	{LMETAL4, 0, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_m4 = {
	x_("Metal-4-Node"),NMETAL4N,NONODEPROTO,		/* name */
	K3,K3,											/* size */
	1,mocmos_m4_p,									/* ports */
	1,mocmos_m4_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Metal-5-Node */
static TECH_PORTS mocmos_m5_p[] = {					/* ports */
	{mocmos_pc_m5, x_("metal-5"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos_m5_l[] = {				/* layers */
	{LMETAL5, 0, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_m5 = {
	x_("Metal-5-Node"),NMETAL5N,NONODEPROTO,		/* name */
	K3,K3,											/* size */
	1,mocmos_m5_p,									/* ports */
	1,mocmos_m5_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE|NNOTUSED,		/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Metal-6-Node */
static TECH_PORTS mocmos_m6_p[] = {					/* ports */
	{mocmos_pc_m6, x_("metal-6"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos_m6_l[] = {				/* layers */
	{LMETAL6, 0, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_m6 = {
	x_("Metal-6-Node"),NMETAL6N,NONODEPROTO,		/* name */
	K3,K3,											/* size */
	1,mocmos_m6_p,									/* ports */
	1,mocmos_m6_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE|NNOTUSED,		/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Polysilicon-1-Node */
static TECH_PORTS mocmos_p1_p[] = {					/* ports */
	{mocmos_pc_p1, x_("polysilicon-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON mocmos_p1_l[] = {				/* layers */
	{LPOLY1, 0, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_p1 = {
	x_("Polysilicon-1-Node"),NPOLY1N,NONODEPROTO,	/* name */
	K2,K2,											/* size */
	1,mocmos_p1_p,									/* ports */
	1,mocmos_p1_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Polysilicon-2-Node */
static TECH_PORTS mocmos_p2_p[] = {					/* ports */
	{mocmos_pc_p2, x_("polysilicon-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos_p2_l[] = {				/* layers */
	{LPOLY2, 0, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_p2 = {
	x_("Polysilicon-2-Node"),NPOLY2N,NONODEPROTO,	/* name */
	K3,K3,											/* size */
	1,mocmos_p2_p,									/* ports */
	1,mocmos_p2_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* P-Active-Node */
static TECH_PORTS mocmos_a_p[] = {					/* ports */
	{mocmos_pc_a, x_("active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos_a_l[] = {				/* layers */
	{LPACT, 0, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_a = {
	x_("P-Active-Node"),NPACTIVEN,NONODEPROTO,		/* name */
	K3,K3,											/* size */
	1,mocmos_a_p,									/* ports */
	1,mocmos_a_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* N-Active-Node */
static TECH_PORTS mocmos_da_p[] = {					/* ports */
	{mocmos_pc_a, x_("active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmos_da_l[] = {				/* layers */
	{LNACT, 0, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_da = {
	x_("N-Active-Node"),NNACTIVEN,NONODEPROTO,		/* name */
	K3,K3,											/* size */
	1,mocmos_da_p,									/* ports */
	1,mocmos_da_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* P-Select-Node */
static TECH_PORTS mocmos_sp_p[] = {					/* ports */
	{mocmos_pc_null, x_("select"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos_sp_l[] = {				/* layers */
	{LSELECTP, -1, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_sp = {
	x_("P-Select-Node"),NSELECTPN,NONODEPROTO,		/* name */
	K4,K4,											/* size */
	1,mocmos_sp_p,									/* ports */
	1,mocmos_sp_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* N-Select-Node */
static TECH_PORTS mocmos_sn_p[] = {					/* ports */
	{mocmos_pc_null, x_("select"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos_sn_l[] = {				/* layers */
	{LSELECTN,  -1, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_sn = {
	x_("N-Select-Node"),NSELECTNN,NONODEPROTO,		/* name */
	K4,K4,											/* size */
	1,mocmos_sn_p,									/* ports */
	1,mocmos_sn_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* PolyCut-Node */
static TECH_PORTS mocmos_gc_p[] = {					/* ports */
	{mocmos_pc_null, x_("polycut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos_gc_l[] = {				/* layers */
	{LPOLYCUT, 0, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_gc = {
	x_("Poly-Cut-Node"),NPCUTN,NONODEPROTO,			/* name */
	K2,K2,											/* size */
	1,mocmos_gc_p,									/* ports */
	1,mocmos_gc_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* ActiveCut-Node */
static TECH_PORTS mocmos_ac_p[] = {					/* ports */
	{mocmos_pc_null, x_("activecut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos_ac_l[] = {				/* layers */
	{LACTCUT, 0, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_ac = {
	x_("Active-Cut-Node"),NACUTN,NONODEPROTO,		/* name */
	K2,K2,											/* size */
	1,mocmos_ac_p,									/* ports */
	1,mocmos_ac_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Via-1-Node */
static TECH_PORTS mocmos_v1_p[] = {					/* ports */
	{mocmos_pc_null, x_("via-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos_v1_l[] = {				/* layers */
	{LVIA1, 0, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_v1 = {
	x_("Via-1-Node"),NVIA1N,NONODEPROTO,			/* name */
	K2,K2,											/* size */
	1,mocmos_v1_p,									/* ports */
	1,mocmos_v1_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Via-2-Node */
static TECH_PORTS mocmos_v2_p[] = {					/* ports */
	{mocmos_pc_null, x_("via-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos_v2_l[] = {				/* layers */
	{LVIA2, 0, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_v2 = {
	x_("Via-2-Node"),NVIA2N,NONODEPROTO,			/* name */
	K2,K2,											/* size */
	1,mocmos_v2_p,									/* ports */
	1,mocmos_v2_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Via-3-Node */
static TECH_PORTS mocmos_v3_p[] = {					/* ports */
	{mocmos_pc_null, x_("via-3"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos_v3_l[] = {				/* layers */
	{LVIA3, 0, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_v3 = {
	x_("Via-3-Node"),NVIA3N,NONODEPROTO,			/* name */
	K2,K2,											/* size */
	1,mocmos_v3_p,									/* ports */
	1,mocmos_v3_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Via-4-Node */
static TECH_PORTS mocmos_v4_p[] = {					/* ports */
	{mocmos_pc_null, x_("via-4"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos_v4_l[] = {				/* layers */
	{LVIA4, 0, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_v4 = {
	x_("Via-4-Node"),NVIA4N,NONODEPROTO,			/* name */
	K2,K2,											/* size */
	1,mocmos_v4_p,									/* ports */
	1,mocmos_v4_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE|NNOTUSED,		/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Via-5-Node */
static TECH_PORTS mocmos_v5_p[] = {					/* ports */
	{mocmos_pc_null, x_("via-5"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos_v5_l[] = {				/* layers */
	{LVIA5, 0, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_v5 = {
	x_("Via-5-Node"),NVIA5N,NONODEPROTO,			/* name */
	K2,K2,											/* size */
	1,mocmos_v5_p,									/* ports */
	1,mocmos_v5_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE|NNOTUSED,		/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* P-Well-Node */
static TECH_PORTS mocmos_wp_p[] = {					/* ports */
	{mocmos_pc_pa, x_("well"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3}};
static TECH_POLYGON mocmos_wp_l[] = {				/* layers */
	{LWELLP, -1, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_wp = {
	x_("P-Well-Node"),NWELLPN,NONODEPROTO,			/* name */
	K12,K12,										/* size */
	1,mocmos_wp_p,									/* ports */
	1,mocmos_wp_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* N-Well-Node */
static TECH_PORTS mocmos_wn_p[] = {					/* ports */
	{mocmos_pc_pa, x_("well"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3}};
static TECH_POLYGON mocmos_wn_l[] = {				/* layers */
	{LWELLN,  -1, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_wn = {
	x_("N-Well-Node"),NWELLNN,NONODEPROTO,			/* name */
	K12,K12,										/* size */
	1,mocmos_wn_p,									/* ports */
	1,mocmos_wn_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Passivation-node */
static TECH_PORTS mocmos_o_p[] = {					/* ports */
	{mocmos_pc_null, x_("passivation"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos_o_l[] = {				/* layers */
	{LPASS, 0, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_o = {
	x_("Passivation-Node"),NPASSN,NONODEPROTO,		/* name */
	K8,K8,											/* size */
	1,mocmos_o_p,									/* ports */
	1,mocmos_o_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Pad-Frame-node */
static TECH_PORTS mocmos_pf_p[] = {					/* ports */
	{mocmos_pc_null, x_("pad-frame"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos_pf_l[] = {				/* layers */
	{LFRAME, 0, 4, CLOSEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_pf = {
	x_("Pad-Frame-Node"),NPADFRN,NONODEPROTO,		/* name */
	K8,K8,											/* size */
	1,mocmos_pf_p,									/* ports */
	1,mocmos_pf_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Polysilicon-Capacitor-node */
static TECH_PORTS mocmos_pc_p[] = {					/* ports */
	{mocmos_pc_null, x_("poly-cap"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos_pc_l[] = {				/* layers */
	{LPOLYCAP, 0, 4, CLOSEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_pc = {
	x_("Poly-Cap-Node"),NPOLYCAPN,NONODEPROTO,		/* name */
	K8,K8,											/* size */
	1,mocmos_pc_p,									/* ports */
	1,mocmos_pc_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* P-Active-Well-node */
static TECH_PORTS mocmos_paw_p[] = {				/* ports */
	{mocmos_pc_null, x_("p-active-well"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos_paw_l[] = {				/* layers */
	{LPACTWELL, 0, 4, CLOSEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_paw = {
	x_("P-Active-Well-Node"),NPACTWELLN,NONODEPROTO,/* name */
	K8,K8,											/* size */
	1,mocmos_paw_p,									/* ports */
	1,mocmos_paw_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Polysilicon-1-Transistor-Node */
static TECH_PORTS mocmos_tp1_p[] = {				/* ports */
	{mocmos_pc_p1, x_("trans-poly-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON mocmos_tp1_l[] = {				/* layers */
	{LTRANS, 0, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_tp1 = {
	x_("Transistor-Poly-Node"),NTRANSPN,NONODEPROTO,	/* name */
	K2,K2,											/* size */
	1,mocmos_tp1_p,									/* ports */
	1,mocmos_tp1_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Silicide-Block-Node */
static TECH_PORTS mocmos_sb_p[] = {				/* ports */
	{mocmos_pc_p1, x_("silicide-block"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmos_sb_l[] = {				/* layers */
	{LSILBLOCK, 0, 4, FILLEDRECT, BOX, mocmos_fullbox}};
static TECH_NODES mocmos_sb = {
	x_("Silicide-Block-Node"),NSILBLOCKN,NONODEPROTO,	/* name */
	K2,K2,											/* size */
	1,mocmos_sb_p,									/* ports */
	1,mocmos_sb_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

TECH_NODES *mocmos_nodeprotos[NODEPROTOCOUNT+1] = {
	&mocmos_pm1, &mocmos_pm2, &mocmos_pm3,			/* metal 1/2/3 pin */
	&mocmos_pm4, &mocmos_pm5, &mocmos_pm6,			/* metal 4/5/6 pin */
	&mocmos_pp1, &mocmos_pp2,						/* polysilicon 1/2 pin */
	&mocmos_psa, &mocmos_pda,						/* P/N active pin */
	&mocmos_pa,										/* active pin */
	&mocmos_mpa, &mocmos_mna,						/* metal 1 to P/N active contact */
	&mocmos_mp1, &mocmos_mp2,						/* metal 1 to polysilicon 1/2 contact */
	&mocmos_mp12,									/* poly capacitor */
	&mocmos_tpa, &mocmos_tna,						/* P/N transistor */
	&mocmos_tpas, &mocmos_tnas,						/* scalable P/N transistor */
	&mocmos_vpnp,									/* vertical PNP transistor */
	&mocmos_m1m2, &mocmos_m2m3, &mocmos_m3m4,		/* via 1/2/3 */
	&mocmos_m4m5, &mocmos_m5m6,						/* via 4/5 */
	&mocmos_psub, &mocmos_nsub,						/* p-well / n-well contact */
	&mocmos_m1, &mocmos_m2, &mocmos_m3,				/* metal 1/2/3 node */
	&mocmos_m4, &mocmos_m5, &mocmos_m6,				/* metal 4/5/6 node */
	&mocmos_p1, &mocmos_p2,							/* polysilicon 1/2 node */
	&mocmos_a, &mocmos_da,							/* active N-Active node */
	&mocmos_sp, &mocmos_sn,							/* P/N select node */
	&mocmos_gc, &mocmos_ac,							/* poly cut / active cut */
	&mocmos_v1, &mocmos_v2, &mocmos_v3,				/* via 1/2/3 node */
	&mocmos_v4, &mocmos_v5,							/* via 4/5 node */
	&mocmos_wp, &mocmos_wn,							/* P/N well node */
	&mocmos_o,										/* overglass node */
	&mocmos_pf,										/* pad frame node */
	&mocmos_pc,										/* poly-cap node */
	&mocmos_paw,									/* p-active-well node */
	&mocmos_tp1,									/* transistor poly node */
	&mocmos_sb, ((TECH_NODES *)-1)};				/* silicide-block node */

/* this tables must correspond with the above table (mocmos_nodeprotos) */
static INTBIG mocmos_node_widoff[NODEPROTOCOUNT*4] = {
	0,0,0,0, 0,0,0,0, 0,0,0,0,						/* metal 1/2/3 pin */
	0,0,0,0, 0,0,0,0, 0,0,0,0,						/* metal 4/5/6 pin */
	0,0,0,0, 0,0,0,0,								/* polysilicon 1/2 pin */
	K6,K6,K6,K6, K6,K6,K6,K6,						/* P/N active pin */
	0,0,0,0,										/* active pin */
	K6,K6,K6,K6, K6,K6,K6,K6,						/* metal 1 to P/N active contact */
	0,0,0,0, 0,0,0,0,								/* metal 1 to polysilicon 1/2 contact */
	0,0,0,0,										/* poly capacitor */
	K6,K6,K10,K10, K6,K6,K10,K10,					/* P/N transistor */
	K7,K7,K12,K12, K7,K7,K12,K12,					/* scalable P/N transistor */
	K4,K4,K4,K4,									/* vertical PNP transistor */
	H0,H0,H0,H0, K1,K1,K1,K1, 0,0,0,0,				/* via 1/2/3 */
	H1,H1,H1,H1, K1,K1,K1,K1,						/* via 4/5 */
	K6,K6,K6,K6, K6,K6,K6,K6,						/* p-well / n-well contact */
	0,0,0,0, 0,0,0,0, 0,0,0,0,						/* metal 1/2/3 node */
	0,0,0,0, 0,0,0,0, 0,0,0,0,						/* metal 4/5/6 node */
	0,0,0,0, 0,0,0,0,								/* polysilicon 1/2 node */
	0,0,0,0, 0,0,0,0,								/* active N-Active node */
	0,0,0,0, 0,0,0,0,								/* P/N select node */
	0,0,0,0, 0,0,0,0,								/* poly cut / active cut */
	0,0,0,0, 0,0,0,0, 0,0,0,0,						/* via 1/2/3 node */
	0,0,0,0, 0,0,0,0,								/* via 4/5 node */
	0,0,0,0, 0,0,0,0,								/* P/N well node */
	0,0,0,0,										/* overglass node */
	0,0,0,0,										/* pad frame node */
	0,0,0,0,										/* poly-cap node */
	0,0,0,0,										/* p-active-well node */
	0,0,0,0,										/* transistor poly node */
	0,0,0,0};										/* silicide-block node */

/* this tables must correspond with the above table (mocmos_nodeprotos) */
static INTBIG mocmos_node_minsize[NODEPROTOCOUNT*2] = {
	XX,XX, XX,XX, XX,XX,							/* metal 1/2/3 pin */
	XX,XX, XX,XX, XX,XX,							/* metal 4/5/6 pin */
	XX,XX, XX,XX,									/* polysilicon 1/2 pin */
	XX,XX, XX,XX,									/* P/N active pin */
	XX,XX,											/* active pin */
	K17,K17, K17,K17,								/* metal 1 to P/N active contact */
	K5,K5, K10,K10,									/* metal 1 to polysilicon 1/2 contact */
	K15,K15,										/* poly capacitor */
	K15,K22, K15,K22,								/* P/N transistor */
	K17,K26, K17,K26,								/* scalable P/N transistor */
	K57,K57,										/* vertical PNP transistor */
	K5,K5, K6,K6, K6,K6,							/* via 1/2/3 */
	K7,K7, K8,K8,									/* via 4/5 */
	K17,K17, K17,K17,								/* p-well / n-well contact */
	XX,XX, XX,XX, XX,XX,							/* metal 1/2/3 node */
	XX,XX, XX,XX, XX,XX,							/* metal 4/5/6 node */
	XX,XX, XX,XX,									/* polysilicon 1/2 node */
	XX,XX, XX,XX,									/* active N-Active node */
	XX,XX, XX,XX,									/* P/N select node */
	XX,XX, XX,XX,									/* poly cut / active cut */
	XX,XX, XX,XX, XX,XX,							/* via 1/2/3 node */
	XX,XX, XX,XX,									/* via 4/5 node */
	XX,XX, XX,XX,									/* P/N well node */
	XX,XX,											/* overglass node */
	XX,XX,											/* pad frame node */
	XX,XX,											/* poly-cap node */
	XX,XX,											/* p-active-well node */
	XX,XX,											/* transistor poly node */
	XX,XX};											/* silicide-block node */

/* this tables must correspond with the above table (mocmos_nodeprotos) */
static CHAR *mocmos_node_minsize_rule[NODEPROTOCOUNT] = {
	x_(""), x_(""), x_(""),							/* metal 1/2/3 pin */
	x_(""), x_(""), x_(""),							/* metal 4/5/6 pin */
	x_(""), x_(""),									/* polysilicon 1/2 pin */
	x_(""), x_(""),									/* P/N active pin */
	x_(""),											/* active pin */
	x_("6.2, 7.3"), x_("6.2, 7.3"),					/* metal 1 to P/N active contact */
	x_("5.2, 7.3"), x_("???"),						/* metal 1 to polysilicon 1/2 contact */
	x_("???"),										/* poly capacitor */
	x_("2.1, 3.1"), x_("2.1, 3.1"),					/* P/N transistor */
	x_("2.1, 3.1"), x_("2.1, 3.1"),					/* scalable P/N transistor */
	x_(""),											/* vertical PNP transistor */
	x_("8.3, 9.3"), x_("14.3, 15.3"), x_("21.3, 22.3"),	/* via 1/2/3 */
	x_("25.3, 26.3"), x_("29.3, 30.3"),				/* via 4/5 */
	x_("4.2, 6.2, 7.3"), x_("4.2, 6.2, 7.3"),		/* p-well / n-well contact */
	x_(""), x_(""), x_(""),							/* metal 1/2/3 node */
	x_(""), x_(""), x_(""),							/* metal 4/5/6 node */
	x_(""), x_(""),									/* polysilicon 1/2 node */
	x_(""), x_(""),									/* active N-Active node */
	x_(""), x_(""),									/* P/N select node */
	x_(""), x_(""),									/* poly cut / active cut */
	x_(""), x_(""), x_(""),							/* via 1/2/3 node */
	x_(""), x_(""),									/* via 4/5 node */
	x_(""), x_(""),									/* P/N well node */
	x_(""),											/* overglass node */
	x_(""),											/* pad frame node */
	x_(""),											/* poly-cap node */
	x_(""),											/* p-active-well node */
	x_(""),											/* transistor poly node */
	x_("")};										/* silicide-block node */

/******************** SIMULATION VARIABLES ********************/

/* for SPICE simulation */
#define mocmos_MIN_RESIST	50.0f		/* minimum resistance consider */
#define mocmos_MIN_CAPAC	 0.04f		/* minimum capacitance consider */
static float mocmos_sim_spice_resistance[MAXLAYERS] = {  /* per square micron */
	0.06f, 0.06f, 0.06f,				/* metal 1/2/3 */
	0.03f, 0.03f, 0.03f,				/* metal 4/5/6 */
	2.5f, 50.0f,						/* poly 1/2 */
	2.5f, 3.0f,							/* P/N active */
	0.0, 0.0,							/* P/N select */
	0.0, 0.0,							/* P/N well */
	2.2f, 2.5f,							/* poly/act cut */
	1.0f, 0.9f, 0.8f, 0.8f, 0.8f,		/* via 1/2/3/4/5 */
	0.0,								/* overglass */
	2.5f,								/* transistor poly */
	0.0,								/* poly cap */
	0.0,								/* P active well */
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0,		/* pseudo metal 1/2/3/4/5/6 */
	0.0, 0.0,							/* pseudo poly 1/2 */
	0.0, 0.0,							/* pseudo P/N active */
	0.0, 0.0,							/* pseudo P/N select */
	0.0, 0.0,							/* pseudo P/N well */
	0.0};								/* pad frame */
static float mocmos_sim_spice_capacitance[MAXLAYERS] = { /* per square micron */
	0.07f, 0.04f, 0.04f,				/* metal 1/2/3 */
	0.04f, 0.04f, 0.04f,				/* metal 4/5/6 */
	0.09f, 1.0f,						/* poly 1/2 */
	0.9f, 0.9f,							/* P/N active */
	0.0, 0.0,							/* P/N select */
	0.0, 0.0,							/* P/N well */
	0.0, 0.0,							/* poly/act cut */
	0.0, 0.0, 0.0, 0.0, 0.0,			/* via 1/2/3/4/5 */
	0.0,								/* overglass */
	0.09f,								/* transistor poly */
	0.0,								/* poly cap */
	0.0,								/* P active well */
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0,		/* pseudo metal 1/2/3/4/5/6 */
	0.0, 0.0,							/* pseudo poly 1/2 */
	0.0, 0.0,							/* pseudo P/N active */
	0.0, 0.0,							/* pseudo P/N select */
	0.0, 0.0,							/* pseudo P/N well */
	0.0};								/* pad frame */
static CHAR *mocmos_sim_spice_header_level1[] = {
	x_("*CMOS/BULK-NWELL (PRELIMINARY PARAMETERS)"),
	x_(".OPTIONS NOMOD DEFL=3UM DEFW=3UM DEFAD=70P DEFAS=70P LIMPTS=1000"),
	x_("+ITL5=0 RELTOL=0.01 ABSTOL=500PA VNTOL=500UV LVLTIM=2"),
	x_("+LVLCOD=1"),
	x_(".MODEL N NMOS LEVEL=1"),
	x_("+KP=60E-6 VTO=0.7 GAMMA=0.3 LAMBDA=0.05 PHI=0.6"),
	x_("+LD=0.4E-6 TOX=40E-9 CGSO=2.0E-10 CGDO=2.0E-10 CJ=.2MF/M^2"),
	x_(".MODEL P PMOS LEVEL=1"),
	x_("+KP=20E-6 VTO=0.7 GAMMA=0.4 LAMBDA=0.05 PHI=0.6"),
	x_("+LD=0.6E-6 TOX=40E-9 CGSO=3.0E-10 CGDO=3.0E-10 CJ=.2MF/M^2"),
	x_(".MODEL DIFFCAP D CJO=.2MF/M^2"),
	NOSTRING};
static CHAR *mocmos_sim_spice_header_level2[] = {
	x_("* MOSIS 3u CMOS PARAMS"),
	x_(".OPTIONS NOMOD DEFL=2UM DEFW=6UM DEFAD=100P DEFAS=100P"),
	x_("+LIMPTS=1000 ITL5=0 ABSTOL=500PA VNTOL=500UV"),
	x_("* Note that ITL5=0 sets ITL5 to infinity"),
	x_(".MODEL N NMOS LEVEL=2 LD=0.3943U TOX=502E-10"),
	x_("+NSUB=1.22416E+16 VTO=0.756 KP=4.224E-05 GAMMA=0.9241"),
	x_("+PHI=0.6 UO=623.661 UEXP=8.328627E-02 UCRIT=54015.0"),
	x_("+DELTA=5.218409E-03 VMAX=50072.2 XJ=0.4U LAMBDA=2.975321E-02"),
	x_("+NFS=4.909947E+12 NEFF=1.001E-02 NSS=0.0 TPG=1.0"),
	x_("+RSH=20.37 CGDO=3.1E-10 CGSO=3.1E-10"),
	x_("+CJ=3.205E-04 MJ=0.4579 CJSW=4.62E-10 MJSW=0.2955 PB=0.7"),
	x_(".MODEL P PMOS LEVEL=2 LD=0.2875U TOX=502E-10"),
	x_("+NSUB=1.715148E+15 VTO=-0.7045 KP=1.686E-05 GAMMA=0.3459"),
	x_("+PHI=0.6 UO=248.933 UEXP=1.02652 UCRIT=182055.0"),
	x_("+DELTA=1.0E-06 VMAX=100000.0 XJ=0.4U LAMBDA=1.25919E-02"),
	x_("+NFS=1.0E+12 NEFF=1.001E-02 NSS=0.0 TPG=-1.0"),
	x_("+RSH=79.10 CGDO=2.89E-10 CGSO=2.89E-10"),
	x_("+CJ=1.319E-04 MJ=0.4125 CJSW=3.421E-10 MJSW=0.198 PB=0.66"),
	x_(".TEMP 25.0"),
	NOSTRING};


/******************** VARIABLE AGGREGATION ********************/

TECH_VARIABLES mocmos_variables[] =
{
	/* set general information about the technology */
	{x_("TECH_layer_names"), (CHAR *)mocmos_layer_names, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_layer_function"), (CHAR *)mocmos_layer_function, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_arc_width_offset"), (CHAR *)mocmos_arc_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|(ARCPROTOCOUNT<<VLENGTHSH)},
	{x_("TECH_node_width_offset"), (CHAR *)mocmos_node_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|((NODEPROTOCOUNT*4)<<VLENGTHSH)},
	{x_("TECH_layer_3dthickness"), (CHAR *)mocmos_3dthick_layers, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_layer_3dheight"), (CHAR *)mocmos_3dheight_layers, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},

	/* set information for the USER tool */
	{x_("USER_color_map"), (CHAR *)mocmos_colmap, 0.0,
		VCHAR|VDONTSAVE|VISARRAY|((sizeof mocmos_colmap)<<VLENGTHSH)},
	{x_("USER_layer_letters"), (CHAR *)mocmos_layer_letters, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("USER_print_colors"), (CHAR *)mocmos_printcolors_layers, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|((MAXLAYERS*5)<<VLENGTHSH)},

	/* set information for the DRC tool */
	{x_("DRC_min_node_size"), (CHAR *)mocmos_node_minsize, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|((NODEPROTOCOUNT*2)<<VLENGTHSH)},
	{x_("DRC_min_node_size_rule"), (CHAR *)mocmos_node_minsize_rule, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(NODEPROTOCOUNT<<VLENGTHSH)},

	/* set information for the I/O tool */
	{x_("IO_cif_layer_names"), (CHAR *)mocmos_cif_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("IO_gds_layer_numbers"), (CHAR *)mocmos_gds_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("IO_skill_layer_names"), (CHAR *)mocmos_skill_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},

	/* set information for the SIM tool (SPICE) */
	{x_("SIM_spice_min_resistance"), 0, mocmos_MIN_RESIST, VFLOAT|VDONTSAVE},
	{x_("SIM_spice_min_capacitance"), 0, mocmos_MIN_CAPAC, VFLOAT|VDONTSAVE},
	{x_("SIM_spice_resistance"), (CHAR *)mocmos_sim_spice_resistance, 0.0,
		VFLOAT|VISARRAY|(MAXLAYERS<<VLENGTHSH)|VDONTSAVE},
	{x_("SIM_spice_capacitance"), (CHAR *)mocmos_sim_spice_capacitance, 0.0,
		VFLOAT|VISARRAY|(MAXLAYERS<<VLENGTHSH)|VDONTSAVE},
	{x_("SIM_spice_header_level1"), (CHAR *)mocmos_sim_spice_header_level1, 0.0,
		VSTRING|VDONTSAVE|VISARRAY},
	{x_("SIM_spice_header_level2"), (CHAR *)mocmos_sim_spice_header_level2, 0.0,
		VSTRING|VDONTSAVE|VISARRAY},
	{NULL, NULL, 0.0, 0}
};

/******************** TECHNOLOGY INTERFACE ROUTINES ********************/

BOOLEAN mocmos_initprocess(TECHNOLOGY *tech, INTBIG pass)
{
	/* initialize the technology variable */
	switch (pass)
	{
		case 0:
			mocmos_tech = tech;
			break;
		case 1:
			mocmos_metal1poly2prim = getnodeproto(x_("mocmos:Metal-1-Polysilicon-2-Con"));
			mocmos_metal1poly12prim = getnodeproto(x_("mocmos:Metal-1-Polysilicon-1-2-Con"));
			mocmos_metal1metal2prim = getnodeproto(x_("mocmos:Metal-1-Metal-2-Con"));
			mocmos_metal4metal5prim = getnodeproto(x_("mocmos:Metal-4-Metal-5-Con"));
			mocmos_metal5metal6prim = getnodeproto(x_("mocmos:Metal-5-Metal-6-Con"));
			mocmos_ptransistorprim = getnodeproto(x_("mocmos:P-Transistor"));
			mocmos_ntransistorprim = getnodeproto(x_("mocmos:N-Transistor"));
			mocmos_metal1pwellprim = getnodeproto(x_("mocmos:Metal-1-P-Well-Con"));
			mocmos_metal1nwellprim = getnodeproto(x_("mocmos:Metal-1-N-Well-Con"));
			mocmos_scalablentransprim = getnodeproto(x_("mocmos:N-Transistor-Scalable"));
			mocmos_scalableptransprim = getnodeproto(x_("mocmos:P-Transistor-Scalable"));
			mocmos_transcontactkey = makekey(x_("MOCMOS_transcontacts"));
			break;
		case 2:
			/* load these DRC tables */
			nextchangequiet();
			if (setvalkey((INTBIG)mocmos_tech, VTECHNOLOGY, dr_wide_limitkey,
				WIDELIMIT, VFRACT|VDONTSAVE) == NOVARIABLE) return(TRUE);
			mocmos_state = 0;
			mocmos_setstate(MOCMOSSUBMRULES|MOCMOS4METAL);
			nextchangequiet();
			setvalkey((INTBIG)mocmos_tech, VTECHNOLOGY, el_techstate_key, mocmos_state,
				VINTEGER|VDONTSAVE);
			break;
	}
	return(FALSE);
}

INTBIG mocmos_request(CHAR *command, va_list ap)
{
	REGISTER INTBIG realstate;
	static INTBIG equivtable[3] = {LPOLY1,LTRANS, -1};

	if (namesame(command, x_("has-state")) == 0) return(1);
	if (namesame(command, x_("get-state")) == 0)
	{
		return(mocmos_state);
	}
	if (namesame(command, x_("get-layer-equivalences")) == 0)
	{
		return((INTBIG)equivtable);
	}
	if (namesame(command, x_("set-state")) == 0)
	{
		mocmos_setstate(va_arg(ap, INTBIG));
		return(0);
	}
	if (namesame(command, x_("describe-state")) == 0)
	{
		return((INTBIG)mocmos_describestate(va_arg(ap, INTBIG)));
	}
	if (namesame(command, x_("switch-n-and-p")) == 0)
	{
		mocmos_switchnp();
		return(0);
	}
	if (namesame(command, x_("factory-reset")) == 0)
	{
		realstate = mocmos_state;
		mocmos_state++;
		mocmos_setstate(realstate);
		return(0);
	}
	return(0);
}

void mocmos_setmode(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG l;
	REGISTER CHAR *pp;
	Q_UNUSED( count );

	l = estrlen(pp = par[0]);

	if (namesamen(pp, x_("full-graphics"), l) == 0)
	{
		mocmos_setstate(mocmos_state & ~MOCMOSSTICKFIGURE);
		ttyputverbose(M_("MOSIS CMOS technology displays full graphics"));
		return;
	}
	if (namesamen(pp, x_("stick-display"), l) == 0 && l >= 3)
	{
		mocmos_setstate(mocmos_state | MOCMOSSTICKFIGURE);
		ttyputverbose(M_("MOSIS CMOS technology displays stick figures"));
		return;
	}

	if (namesamen(pp, x_("alternate-active-poly"), l) == 0 && l >= 3)
	{
		mocmos_setstate(mocmos_state | MOCMOSALTAPRULES);
		ttyputverbose(M_("MOSIS CMOS technology uses alternate active/poly rules"));
		return;
	}
	if (namesamen(pp, x_("standard-active-poly"), l) == 0 && l >= 3)
	{
		mocmos_setstate(mocmos_state & ~MOCMOSALTAPRULES);
		ttyputverbose(M_("MOSIS CMOS technology uses standard active/poly rules"));
		return;
	}

	if (namesamen(pp, x_("disallow-stacked-vias"), l) == 0 && l >= 2)
	{
		mocmos_setstate(mocmos_state | MOCMOSNOSTACKEDVIAS);
		ttyputverbose(M_("MOSIS CMOS technology disallows stacked vias"));
		return;
	}
	if (namesamen(pp, x_("allow-stacked-vias"), l) == 0 && l >= 3)
	{
		mocmos_setstate(mocmos_state & ~MOCMOSNOSTACKEDVIAS);
		ttyputverbose(M_("MOSIS CMOS technology allows stacked vias"));
		return;
	}

	if (namesamen(pp, x_("scmos-rules"), l) == 0 && l >= 2)
	{
		mocmos_setstate((mocmos_state & ~MOCMOSRULESET) | MOCMOSSCMOSRULES);
		ttyputverbose(M_("MOSIS CMOS technology uses standard SCMOS rules"));
		return;
	}
	if (namesamen(pp, x_("submicron-rules"), l) == 0 && l >= 2)
	{
		mocmos_setstate((mocmos_state & ~MOCMOSRULESET) | MOCMOSSUBMRULES);
		ttyputverbose(M_("MOSIS CMOS technology uses submicron rules"));
		return;
	}
	if (namesamen(pp, x_("deep-rules"), l) == 0 && l >= 2)
	{
		mocmos_setstate((mocmos_state & ~MOCMOSRULESET) | MOCMOSDEEPRULES);
		ttyputverbose(M_("MOSIS CMOS technology uses deep submicron rules"));
		return;
	}

	if (namesamen(pp, x_("one-polysilicon"), l) == 0)
	{
		mocmos_setstate(mocmos_state & ~MOCMOSTWOPOLY);
		ttyputverbose(M_("MOSIS CMOS technology uses 1-polysilicon rules"));
		return;
	}
	if (namesamen(pp, x_("two-polysilicon"), l) == 0)
	{
		mocmos_setstate(mocmos_state | MOCMOSTWOPOLY);
		ttyputverbose(M_("MOSIS CMOS technology uses 2-polysilicon rules"));
		return;
	}

	if (namesamen(pp, x_("hide-special-transistors"), l) == 0)
	{
		mocmos_setstate(mocmos_state & ~MOCMOSSPECIALTRAN);
		ttyputverbose(M_("MOSIS CMOS technology excludes scalable transistors"));
		return;
	}
	if (namesamen(pp, x_("show-special-transistors"), l) == 0)
	{
		mocmos_setstate(mocmos_state | MOCMOSSPECIALTRAN);
		ttyputverbose(M_("MOSIS CMOS technology includes scalable transistors"));
		return;
	}

	if (namesamen(pp, x_("2-metal-rules"), l) == 0)
	{
		mocmos_setstate((mocmos_state & ~MOCMOSMETALS) | MOCMOS2METAL);
		ttyputverbose(M_("MOSIS CMOS technology uses 2-metal rules"));
		return;
	}
	if (namesamen(pp, x_("3-metal-rules"), l) == 0)
	{
		mocmos_setstate((mocmos_state & ~MOCMOSMETALS) | MOCMOS3METAL);
		ttyputverbose(M_("MOSIS CMOS technology uses 3-metal rules"));
		return;
	}
	if (namesamen(pp, x_("4-metal-rules"), l) == 0)
	{
		mocmos_setstate((mocmos_state & ~MOCMOSMETALS) | MOCMOS4METAL);
		ttyputverbose(M_("MOSIS CMOS technology uses 4-metal rules"));
		return;
	}
	if (namesamen(pp, x_("5-metal-rules"), l) == 0)
	{
		mocmos_setstate((mocmos_state & ~MOCMOSMETALS) | MOCMOS5METAL);
		ttyputverbose(M_("MOSIS CMOS technology uses 5-metal rules"));
		return;
	}
	if (namesamen(pp, x_("6-metal-rules"), l) == 0)
	{
		mocmos_setstate((mocmos_state & ~MOCMOSMETALS) | MOCMOS6METAL);
		ttyputverbose(M_("MOSIS CMOS technology uses 6-metal rules"));
		return;
	}

	if (namesamen(pp, x_("switch-n-and-p"), l) == 0 && l >= 2)
	{
		mocmos_switchnp();
	}

	ttyputbadusage(x_("technology tell mocmos"));
}

/******************** NODE DESCRIPTION (GRAPHICAL) ********************/

INTBIG mocmos_nodepolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win)
{
	return(mocmos_intnodepolys(ni, reasonable, win, &tech_oneprocpolyloop, &mocmos_oneprocpolyloop));
}

INTBIG mocmos_intnodepolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win, POLYLOOP *pl, MOCPOLYLOOP *mocpl)
{
	REGISTER INTBIG pindex, count;
	TECH_NODES *thistn;
	REGISTER NODEPROTO *np;

	np = ni->proto;
	pindex = np->primindex;

	/* non-stick-figures: standard components */
	if ((mocmos_state&MOCMOSSTICKFIGURE) == 0)
	{
		if (pindex == NSTRANSP || pindex == NSTRANSN)
			return(mocmos_initializescalabletransistor(ni, reasonable, win, pl, mocpl));
		if (pindex == NVPNPN)
			return(mocmos_initializevpnptransistor(ni, reasonable, win, pl, mocpl));
		return(tech_nodepolys(ni, reasonable, win, pl));
	}

	/* stick figures: special cases for special primitives */
	thistn = np->tech->nodeprotos[pindex-1];
	count = thistn->layercount;
	switch (pindex)
	{
		case NMETAL1P:
		case NMETAL2P:
		case NMETAL3P:
		case NMETAL4P:
		case NMETAL5P:
		case NMETAL6P:
		case NPOLY1P:
		case NPOLY2P:
		case NPACTP:
		case NNACTP:
		case NACTP:
			/* pins disappear with one or two wires */
			if (tech_pinusecount(ni, NOWINDOWPART)) count = 0;
			break;
		case NMETPACTC:
		case NMETNACTC:
		case NMETPOLY1C:
		case NMETPOLY2C:
		case NMETPOLY12C:
		case NVIA1:
		case NVIA2:
		case NVIA3:
		case NVIA4:
		case NVIA5:
		case NPWBUT:
		case NNWBUT:
			/* contacts draw a box the size of the port */
			count = 1;
			break;
		case NTRANSP:
		case NTRANSN:
			/* prepare for possible serpentine transistor */
			count = tech_inittrans(2, ni, pl);
			break;
	}

	/* add in displayable variables */
	pl->realpolys = count;
	count += tech_displayablenvars(ni, pl->curwindowpart, pl);
	if (reasonable != 0) *reasonable = count;
	return(count);
}

void mocmos_shapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly)
{
	mocmos_intshapenodepoly(ni, box, poly, &tech_oneprocpolyloop, &mocmos_oneprocpolyloop);
}

void mocmos_intshapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl, MOCPOLYLOOP *mocpl)
{
	TECH_POLYGON *lay;
	REGISTER INTBIG pindex, lambda, cx, cy;
	REGISTER NODEPROTO *np;
	REGISTER TECH_NODES *thistn;
	REGISTER TECH_PORTS *portdata;
	static GRAPHICS contactdesc = {LAYERO, BLACK, SOLIDC, SOLIDC,
		{0,0,0,0,0,0,0,0}, NOVARIABLE, 0};

	lay = 0;
	if ((mocmos_state&MOCMOSSTICKFIGURE) == 0)
	{
		/* non-stick-figures: standard components */
		np = ni->proto;
		pindex = np->primindex;
		if (pindex == NSTRANSP || pindex == NSTRANSN)
		{
			mocmos_iteratescalabletransistor(ni, box, poly, pl, mocpl);
			return;
		}
		if (pindex == NVPNPN)
		{
			mocmos_iteratevpnptransistor(ni, box, poly, pl, mocpl);
			return;
		}

		tech_shapenodepoly(ni, box, poly, pl);
		return;
	}

	/* handle displayable variables */
	if (box >= pl->realpolys)
	{
		(void)tech_filldisplayablenvar(ni, poly, pl->curwindowpart, 0, pl);
		return;
	}

	np = ni->proto;
	pindex = np->primindex;
	thistn = mocmos_tech->nodeprotos[pindex-1];
	lambda = lambdaofnode(ni);
	switch (pindex)
	{
		case NMETAL1P:
		case NMETAL2P:
		case NMETAL3P:
		case NMETAL4P:
		case NMETAL5P:
		case NMETAL6P:
		case NPOLY1P:
		case NPOLY2P:
		case NPACTP:
		case NNACTP:
		case NACTP:
			/* pins disappear with one or two wires */
			lay = &thistn->layerlist[box];
			poly->layer = LPOLYCUT;
			if (poly->limit < 2) (void)extendpolygon(poly, 2);
			cx = (ni->lowx + ni->highx) / 2;
			cy = (ni->lowy + ni->highy) / 2;
			poly->xv[0] = cx;   poly->yv[0] = cy;
			poly->xv[1] = cx;   poly->yv[1] = cy + lambda/2;
			poly->count = 2;
			poly->style = DISC;
			poly->desc = mocmos_tech->layers[poly->layer];
			break;
		case NMETPACTC:
		case NMETNACTC:
		case NMETPOLY1C:
		case NMETPOLY2C:
		case NMETPOLY12C:
		case NVIA1:
		case NVIA2:
		case NVIA3:
		case NVIA4:
		case NVIA5:
		case NPWBUT:
		case NNWBUT:
			/* contacts draw a box the size of the port */
			lay = &thistn->layerlist[box];
			poly->layer = LPOLYCUT;
			if (poly->limit < 2) (void)extendpolygon(poly, 2);
			portdata = &thistn->portlist[0];
			subrange(ni->lowx, ni->highx, portdata->lowxmul, portdata->lowxsum,
				portdata->highxmul, portdata->highxsum, &poly->xv[0], &poly->xv[1], lambda);
			subrange(ni->lowy, ni->highy, portdata->lowymul, portdata->lowysum,
				portdata->highymul, portdata->highysum, &poly->yv[0], &poly->yv[1], lambda);
			poly->count = 2;
			poly->style = CLOSEDRECT;
			poly->desc = &contactdesc;

			/* code cannot be called by multiple procesors: uses globals */
			NOT_REENTRANT;

			switch (pindex)
			{
				case NMETPACTC:   contactdesc.bits = LAYERT1|LAYERT3;  contactdesc.col = COLORT1|COLORT3;   break;
				case NMETNACTC:   contactdesc.bits = LAYERT1|LAYERT3;  contactdesc.col = COLORT1|COLORT3;   break;
				case NMETPOLY1C:  contactdesc.bits = LAYERT1|LAYERT2;  contactdesc.col = COLORT1|COLORT2;   break;
				case NMETPOLY2C:  contactdesc.bits = LAYERO;           contactdesc.col = ORANGE;            break;
				case NMETPOLY12C: contactdesc.bits = LAYERO;           contactdesc.col = ORANGE;            break;
				case NVIA1:       contactdesc.bits = LAYERT1|LAYERT4;  contactdesc.col = COLORT1|COLORT4;   break;
				case NVIA2:       contactdesc.bits = LAYERT4|LAYERT5;  contactdesc.col = COLORT4|COLORT5;   break;
				case NVIA3:       contactdesc.bits = LAYERO;           contactdesc.col = LBLUE;             break;
				case NVIA4:       contactdesc.bits = LAYERO;           contactdesc.col = LRED;              break;
				case NVIA5:       contactdesc.bits = LAYERO;           contactdesc.col = CYAN;              break;
				case NPWBUT:      contactdesc.bits = LAYERO;           contactdesc.col = BROWN;             break;
				case NNWBUT:      contactdesc.bits = LAYERO;           contactdesc.col = YELLOW;            break;
			}
			break;
		case NTRANSP:
		case NTRANSN:
			/* prepare for possible serpentine transistor */
			lay = &thistn->gra[box].basics;
			poly->layer = lay->layernum;
			if (poly->layer == LTRANS)
			{
				ni->lowy += lambda;
				ni->highy -= lambda;
			} else
			{
				ni->lowx += lambda + lambda/2;
				ni->highx -= lambda + lambda/2;
			}
			tech_filltrans(poly, &lay, thistn->gra, ni, lambda, box, (TECH_PORTS *)0, pl);
			if (poly->layer == LTRANS)
			{
				ni->lowy -= lambda;
				ni->highy += lambda;
			} else
			{
				ni->lowx -= lambda + lambda/2;
				ni->highx += lambda + lambda/2;
			}
			poly->desc = mocmos_tech->layers[poly->layer];
			break;
		default:
			lay = &thistn->layerlist[box];
			tech_fillpoly(poly, lay, ni, lambda, FILLED);
			poly->desc = mocmos_tech->layers[poly->layer];
			break;
	}
}

INTBIG mocmos_allnodepolys(NODEINST *ni, POLYLIST *plist, WINDOWPART *win, BOOLEAN onlyreasonable)
{
	REGISTER INTBIG tot, j;
	INTBIG reasonable;
	REGISTER NODEPROTO *np;
	REGISTER POLYGON *poly;
	POLYLOOP mypl;
	MOCPOLYLOOP mymocpl;

	np = ni->proto;
	mypl.curwindowpart = win;
	tot = mocmos_intnodepolys(ni, &reasonable, win, &mypl, &mymocpl);
	if (onlyreasonable) tot = reasonable;
	if (mypl.realpolys < tot) tot = mypl.realpolys;
	if (ensurepolylist(plist, tot, db_cluster)) return(-1);
	for(j = 0; j < tot; j++)
	{
		poly = plist->polygons[j];
		poly->tech = mocmos_tech;
		mocmos_intshapenodepoly(ni, j, poly, &mypl, &mymocpl);
	}
	return(tot);
}

void mocmos_nodesizeoffset(NODEINST *ni, INTBIG *lx, INTBIG *ly, INTBIG *hx, INTBIG *hy)
{
	REGISTER INTBIG pindex;
	REGISTER NODEPROTO *np;
	REGISTER INTBIG lambda, cx, cy;
	INTBIG bx, by, ux, uy;
	REGISTER TECH_NODES *thistn;
	REGISTER TECH_PORTS *portdata;

	np = ni->proto;
	pindex = np->primindex;
	lambda = lambdaofnode(ni);
	switch (pindex)
	{
		case NMETAL1P:
		case NMETAL2P:
		case NMETAL3P:
		case NMETAL4P:
		case NMETAL5P:
		case NMETAL6P:
		case NPOLY1P:
		case NPOLY2P:
		case NPACTP:
		case NNACTP:
		case NACTP:
			cx = (ni->lowx + ni->highx) / 2;
			cy = (ni->lowy + ni->highy) / 2;
			*lx = (cx - lambda) - ni->lowx;
			*hx = ni->highx - (cx + lambda);
			*ly = (cy - lambda) - ni->lowy;
			*hy = ni->highy - (cy + lambda);
			break;
		case NMETPACTC:
		case NMETNACTC:
		case NMETPOLY1C:
		case NMETPOLY2C:
		case NMETPOLY12C:
		case NVIA1:
		case NVIA2:
		case NVIA3:
		case NVIA4:
		case NVIA5:
		case NPWBUT:
		case NNWBUT:
			/* contacts draw a box the size of the port */
			thistn = mocmos_tech->nodeprotos[pindex-1];
			portdata = &thistn->portlist[0];
			subrange(ni->lowx, ni->highx, portdata->lowxmul, portdata->lowxsum,
				portdata->highxmul, portdata->highxsum, &bx, &ux, lambda);
			subrange(ni->lowy, ni->highy, portdata->lowymul, portdata->lowysum,
				portdata->highymul, portdata->highysum, &by, &uy, lambda);
			*lx = bx - ni->lowx;
			*hx = ni->highx - ux;
			*ly = by - ni->lowy;
			*hy = ni->highy - uy;
			break;
		default:
			nodeprotosizeoffset(np, lx, ly, hx, hy, ni->parent);
			if (pindex == NTRANSP || pindex == NTRANSN)
			{
				*lx += lambda + lambda/2;
				*hx += lambda + lambda/2;
				*ly += lambda;
				*hy += lambda;
			}
			break;
	}
}

/******************** NODE DESCRIPTION (ELECTRICAL) ********************/

INTBIG mocmos_nodeEpolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win)
{
	return(mocmos_intEnodepolys(ni, reasonable, win, &tech_oneprocpolyloop, &mocmos_oneprocpolyloop));
}

INTBIG mocmos_intEnodepolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win, POLYLOOP *pl, MOCPOLYLOOP *mocpl)
{
	REGISTER INTBIG pindex;

	if ((mocmos_state&MOCMOSSTICKFIGURE) != 0) return(0);

	/* non-stick-figures: standard components */
	pindex = ni->proto->primindex;
	if (pindex == NSTRANSP || pindex == NSTRANSN)
		return(mocmos_initializescalabletransistor(ni, reasonable, win, pl, mocpl));
	if (pindex == NVPNPN)
		return(mocmos_initializevpnptransistor(ni, reasonable, win, pl, mocpl));
	return(tech_nodeEpolys(ni, reasonable, win, pl));
}

void mocmos_shapeEnodepoly(NODEINST *ni, INTBIG box, POLYGON *poly)
{
	mocmos_intshapeEnodepoly(ni, box, poly, &tech_oneprocpolyloop, &mocmos_oneprocpolyloop);
}

void mocmos_intshapeEnodepoly(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl, MOCPOLYLOOP *mocpl)
{
	REGISTER INTBIG pindex;

	pindex = ni->proto->primindex;
	if (pindex == NSTRANSP || pindex == NSTRANSN)
	{
		mocmos_iteratescalabletransistor(ni, box, poly, pl, mocpl);
		return;
	}
	if (pindex == NVPNPN)
	{
		mocmos_iteratevpnptransistor(ni, box, poly, pl, mocpl);
		return;
	}

	tech_shapeEnodepoly(ni, box, poly, pl);
}

INTBIG mocmos_allnodeEpolys(NODEINST *ni, POLYLIST *plist, WINDOWPART *win, BOOLEAN onlyreasonable)
{
	REGISTER INTBIG tot, j;
	INTBIG reasonable;
	REGISTER POLYGON *poly;
	POLYLOOP mypl;
	MOCPOLYLOOP mymocpl;

	mypl.curwindowpart = win;
	tot = mocmos_intEnodepolys(ni, &reasonable, win, &mypl, &mymocpl);
	if (onlyreasonable) tot = reasonable;
	if (ensurepolylist(plist, tot, db_cluster)) return(-1);
	for(j = 0; j < tot; j++)
	{
		poly = plist->polygons[j];
		poly->tech = mocmos_tech;
		mocmos_intshapeEnodepoly(ni, j, poly, &mypl, &mymocpl);
	}
	return(tot);
}

/******************** PORT DESCRIPTION ********************/

void mocmos_shapeportpoly(NODEINST *ni, PORTPROTO *pp, POLYGON *poly, XARRAY trans,
	BOOLEAN purpose)
{
	REGISTER INTBIG pindex, portnum, inset, x, y, xs, ys, lambda;
	REGISTER TECH_NODES *thistn;
	Q_UNUSED( purpose );

	pindex = ni->proto->primindex;
	thistn = ni->proto->tech->nodeprotos[pindex-1];

	/* look down to the bottom level node/port */
	switch (thistn->special)
	{
		case SERPTRANS:
			tech_filltransport(ni, pp, poly, trans, thistn, thistn->f2, thistn->f3,
				thistn->f4, thistn->f5, thistn->f6);
			break;

		default:
			if (pindex == NVPNPN)
			{
				portnum = (pp->userbits&PORTNET) >> PORTNETSH;
				if (portnum > 0)
				{
					if (poly->limit < 4) (void)extendpolygon(poly, 4);
					if (portnum == 1) inset = 39; else inset = 13;
					lambda = lambdaofnode(ni);
					xs = ((ni->highx - ni->lowx) - inset*lambda) / 2;
					ys = ((ni->highy - ni->lowy) - inset*lambda) / 2;
					x = (ni->highx + ni->lowx) / 2;   y = (ni->highy + ni->lowy) / 2;
					poly->xv[0] = x + xs;   poly->yv[0] = y + ys;
					poly->xv[1] = x + xs;   poly->yv[1] = y - ys;
					poly->xv[2] = x - xs;   poly->yv[2] = y - ys;
					poly->xv[3] = x - xs;   poly->yv[3] = y + ys;
					xformpoly(poly, trans);
					poly->count = 4;
					poly->style = CLOSED;
					return;
				}
			}
			tech_fillportpoly(ni, pp, poly, trans, thistn, CLOSED, lambdaofnode(ni));
			break;
	}
}

/******************** VERTICAL PNP TRANSISTOR DESCRIPTION ********************/

INTBIG mocmos_initializevpnptransistor(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win, POLYLOOP *pl, MOCPOLYLOOP *mocpl)
{
	REGISTER INTBIG lambda, numoutercuts, numinnercuts;
	static INTBIG innergapkey = 0, outergapkey = 0;
	REGISTER VARIABLE *innergapvar, *outergapvar;

	lambda = lambdaofnode(ni);

	/* get the opening variables */
	if (ni->parent == NONODEPROTO) mocpl->innergap = mocpl->outergap = 0; else
	{
		if (innergapkey == 0) innergapkey = makekey("ATTR_inner_gap");
		innergapvar = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, innergapkey);
		if (innergapvar == NOVARIABLE)
		{
			nextchangequiet();
			innergapvar = setvalkey((INTBIG)ni, VNODEINST, innergapkey, 0, VINTEGER);
		}
		mocpl->innergap = innergapvar->addr * lambda;

		if (outergapkey == 0) outergapkey = makekey("ATTR_outer_gap");
		outergapvar = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, outergapkey);
		if (outergapvar == NOVARIABLE)
		{
			nextchangequiet();
			outergapvar = setvalkey((INTBIG)ni, VNODEINST, outergapkey, 0, VINTEGER);
		}
		mocpl->outergap = outergapvar->addr * lambda;
	}

	mocpl->numlayers = tech_nodepolys(ni, reasonable, win, pl);
	mocpl->outerhorizcuts = ((ni->highx - ni->lowx) / lambda - 8) / 5;
	mocpl->outervertcuts = ((ni->highy - ni->lowy) / lambda - 8) / 5 - 2;
	numoutercuts = (mocpl->outerhorizcuts + mocpl->outervertcuts) * 2;
	if (mocpl->outergap != 0) numoutercuts -= mocpl->outervertcuts;

	mocpl->innerhorizcuts = ((ni->highx - ni->lowx) / lambda - 34) / 5;
	mocpl->innervertcuts = ((ni->highy - ni->lowy) / lambda - 34) / 5 - 2;
	numinnercuts = (mocpl->innerhorizcuts + mocpl->innervertcuts) * 2;
	if (mocpl->innergap != 0) numinnercuts -= mocpl->innervertcuts;

	return(mocpl->numlayers + numoutercuts + numinnercuts);
}

void mocmos_iteratevpnptransistor(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl, MOCPOLYLOOP *mocpl)
{
	REGISTER INTBIG x, y, xs, ys, lambda, boxnum, gap;

	x = (ni->highx + ni->lowx) / 2;   y = (ni->highy + ni->lowy) / 2;
	lambda = lambdaofnode(ni);
	xs = ((ni->highx - ni->lowx) - muldiv(K57, lambda, WHOLE)) / 2;
	ys = ((ni->highy - ni->lowy) - muldiv(K57, lambda, WHOLE)) / 2;

	if (box >= mocpl->numlayers)
	{
		/* do the cuts along the outer rings */
		boxnum = box - mocpl->numlayers;
		if (boxnum < mocpl->outerhorizcuts)
		{
			/* do cut along top of outer ring */
			mocmos_fillcut(ni, boxnum, mocpl->outerhorizcuts, poly, lambda, x, y, 0, ys+22*lambda);   return;
		}
		boxnum -= mocpl->outerhorizcuts;
		if (boxnum < mocpl->outerhorizcuts)
		{
			/* do cut along bottom of outer ring */
			mocmos_fillcut(ni, boxnum, mocpl->outerhorizcuts, poly, lambda, x, y, 0, -ys-22*lambda);   return;
		}
		boxnum -= mocpl->outerhorizcuts;
		if (boxnum < mocpl->outervertcuts)
		{
			/* do cut along left of outer ring */
			mocmos_fillcut(ni, boxnum, mocpl->outervertcuts, poly, lambda, x, y, -xs-22*lambda, 0);   return;
		}
		boxnum -= mocpl->outervertcuts;
		if (mocpl->outergap == 0)
		{
			if (boxnum < mocpl->outervertcuts)
			{
				/* do cut along right of outer ring */
				mocmos_fillcut(ni, boxnum, mocpl->outervertcuts, poly, lambda, x, y, xs+22*lambda, 0);   return;
			}
			boxnum -= mocpl->outervertcuts;
		}

		/* do the cuts along the inner rings */
		if (boxnum < mocpl->innerhorizcuts)
		{
			/* do cut along top of inner ring */
			mocmos_fillcut(ni, boxnum, mocpl->innerhorizcuts, poly, lambda, x, y, 0, ys+9*lambda);   return;
		}
		boxnum -= mocpl->innerhorizcuts;
		if (boxnum < mocpl->innerhorizcuts)
		{
			/* do cut along bottom of inner ring */
			mocmos_fillcut(ni, boxnum, mocpl->innerhorizcuts, poly, lambda, x, y, 0, -ys-9*lambda);   return;
		}
		boxnum -= mocpl->innerhorizcuts;
		if (boxnum < mocpl->innervertcuts)
		{
			/* do cut along left of inner ring */
			mocmos_fillcut(ni, boxnum, mocpl->innervertcuts, poly, lambda, x, y, -xs-9*lambda, 0);   return;
		}
		boxnum -= mocpl->innervertcuts;
		if (boxnum < mocpl->innervertcuts)
		{
			/* do cut along right of inner ring */
			mocmos_fillcut(ni, boxnum, mocpl->innervertcuts, poly, lambda, x, y, xs+9*lambda, 0);   return;
		}
		ttyputmsg("Huh? extra layers!");
	}
	switch (box)
	{
		case 1:		/* the inner ring of Metal-1 */
			if (mocpl->innergap == 0) gap = 0; else gap = mocpl->innergap + lambda;
			mocmos_fillring(ni, box, poly, lambda, x, y, xs, ys, K7, K11, gap);   return;
		case 2:		/* the outer ring of Metal-1 */
			if (mocpl->outergap == 0) gap = 0; else gap = mocpl->outergap + lambda;
			mocmos_fillring(ni, box, poly, lambda, x, y, xs, ys, K20, K24, gap);  return;
		case 4:		/* the inner ring of P-Active */
			mocmos_fillring(ni, box, poly, lambda, x, y, xs, ys, H6, H11, mocpl->innergap);   return;
		case 5:		/* the outer ring of P-Active */
			mocmos_fillring(ni, box, poly, lambda, x, y, xs, ys, H19, H24, mocpl->outergap);  return;
		case 7:		/* the ring of P-Well */
			mocmos_fillring(ni, box, poly, lambda, x, y, xs, ys, H15, H28, 0);  return;
		case 9:		/* the ring of N-Select */
			mocmos_fillring(ni, box, poly, lambda, x, y, xs, ys, H5, H11, 0);   return;
		case 10:	/* the ring of P-Select */
			mocmos_fillring(ni, box, poly, lambda, x, y, xs, ys, H17, H26, 0);  return;
	}
	tech_shapenodepoly(ni, box, poly, pl);
}

void mocmos_fillcut(NODEINST *ni, INTBIG boxnum, INTBIG boxesonside, POLYGON *poly, INTBIG lambda,
	INTBIG x, INTBIG y, INTBIG xside, INTBIG yside)
{
	REGISTER INTBIG cutextent;
	REGISTER NODEPROTO *np;

	np = ni->proto;
	poly->layer = LACTCUT;
	poly->desc = &mocmos_ac_lay;
	poly->tech = np->tech;

	cutextent = (boxesonside-1) * 5 * lambda;
	if (poly->limit < 4) (void)extendpolygon(poly, 4);
	if (xside == 0)
	{
		/* top/bottom cuts */
		poly->xv[0] = x - cutextent/2 + (boxnum * lambda * 5) - lambda;
		poly->yv[0] = y + yside-lambda;
		poly->xv[1] = x - cutextent/2 + (boxnum * lambda * 5) + lambda;
		poly->yv[1] = y + yside-lambda;
		poly->xv[2] = x - cutextent/2 + (boxnum * lambda * 5) + lambda;
		poly->yv[2] = y + yside+lambda;
		poly->xv[3] = x - cutextent/2 + (boxnum * lambda * 5) - lambda;
		poly->yv[3] = y + yside+lambda;
	} else
	{
		/* left/right cuts */
		poly->xv[0] = x + xside-lambda;
		poly->yv[0] = y - cutextent/2 + (boxnum * lambda * 5) - lambda;
		poly->xv[1] = x + xside-lambda;
		poly->yv[1] = y - cutextent/2 + (boxnum * lambda * 5) + lambda;
		poly->xv[2] = x + xside+lambda;
		poly->yv[2] = y - cutextent/2 + (boxnum * lambda * 5) + lambda;
		poly->xv[3] = x + xside+lambda;
		poly->yv[3] = y - cutextent/2 + (boxnum * lambda * 5) - lambda;
	}
	poly->count = 4;
	poly->style = FILLED;
}

void mocmos_fillring(NODEINST *ni, INTBIG box, POLYGON *poly, INTBIG lambda,
	INTBIG x, INTBIG y, INTBIG xs, INTBIG ys, INTBIG inner, INTBIG outer, INTBIG gap)
{
	REGISTER INTBIG pindex, innerdist, outerdist;
	REGISTER TECH_POLYGON *lay;
	REGISTER TECH_NODES *thistn;
	REGISTER NODEPROTO *np;

	np = ni->proto;
	pindex = np->primindex;
	thistn = np->tech->nodeprotos[pindex-1];
	lay = &thistn->layerlist[box];
	poly->layer = lay->layernum;
	poly->desc = np->tech->layers[poly->layer];
	poly->tech = np->tech;
	innerdist = muldiv(lambda, inner, WHOLE);
	outerdist = muldiv(lambda, outer, WHOLE);

	if (poly->limit < 12) (void)extendpolygon(poly, 12);
	poly->xv[0] = x+xs + outerdist;   poly->yv[0] = y+ys + outerdist;
	poly->xv[1] = x-xs - outerdist;   poly->yv[1] = y+ys + outerdist;
	poly->xv[2] = x-xs - outerdist;   poly->yv[2] = y-ys - outerdist;
	poly->xv[3] = x+xs + outerdist;   poly->yv[3] = y-ys - outerdist;
	poly->xv[4] = x+xs + outerdist;   poly->yv[4] = y - gap/2;
	poly->xv[5] = x+xs + innerdist;   poly->yv[5] = y - gap/2;
	poly->xv[6] = x+xs + innerdist;   poly->yv[6] = y-ys - innerdist;
	poly->xv[7] = x-xs - innerdist;   poly->yv[7] = y-ys - innerdist;
	poly->xv[8] = x-xs - innerdist;   poly->yv[8] = y+ys + innerdist;
	poly->xv[9] = x+xs + innerdist;   poly->yv[9] = y+ys + innerdist;
	poly->xv[10] = x+xs + innerdist;  poly->yv[10] = y + gap/2;
	poly->xv[11] = x+xs + outerdist;  poly->yv[11] = y + gap/2;
	poly->count = 12;
	poly->style = FILLED;
}

/******************** SCALABLE TRANSISTOR DESCRIPTION ********************/

INTBIG mocmos_initializescalabletransistor(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win, POLYLOOP *pl, MOCPOLYLOOP *mocpl)
{
	REGISTER INTBIG pindex, count, lambda, activewid, requestedwid, extrainset, nodewid, extracuts;
	REGISTER INTBIG cutsize, cutindent, cutsep;
	REGISTER VARIABLE *var;
	REGISTER CHAR *pt;
	TECH_NODES *thistn;
	REGISTER NODEPROTO *np;

	/* determine the width */
	np = ni->proto;
	pindex = np->primindex;
	lambda = lambdaofnode(ni);
	nodewid = (ni->highx - ni->lowx) * WHOLE / lambda;
	activewid = nodewid - K14;
	extrainset = 0;

	/* determine special configurations (number of active contacts, inset of active contacts) */
	mocpl->numcontacts = 2;
	mocpl->insetcontacts = FALSE;
	var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, mocmos_transcontactkey);
	if (var != NOVARIABLE)
	{
		pt = (CHAR *)var->addr;
		if (*pt == '0' || *pt == '1' || *pt == '2')
		{
			mocpl->numcontacts = *pt - '0';
			pt++;
		}
		if (*pt == 'i' || *pt == 'I') mocpl->insetcontacts = TRUE;
	}
	mocpl->boxoffset = 4 - mocpl->numcontacts * 2;

	/* determine width */
	var = getvalkey((INTBIG)ni, VNODEINST, -1, el_attrkey_width);
	if (var != NOVARIABLE)
	{
		pt = describevariable(var, -1, -1);
		if (*pt == '-' || *pt == '+' || isdigit(*pt))
		{
			requestedwid = atofr(pt);
			if (requestedwid > activewid)
			{
				ttyputmsg(_("Warning: cell %s, node %s requests width of %s but is only %s wide"),
					describenodeproto(ni->parent), describenodeinst(ni), frtoa(requestedwid),
						frtoa(activewid));
			}
			if (requestedwid < activewid && requestedwid > 0)
			{
				extrainset = (activewid - requestedwid) / 2;
				activewid = requestedwid;
			}
		}
	}
	mocpl->actinset = (nodewid-activewid) / 2;
	mocpl->polyinset = mocpl->actinset - K2;
	mocpl->actcontinset = K7 + extrainset;

	/* contacts must be 5 wide at a minimum */
	if (activewid < K5) mocpl->actcontinset -= (K5-activewid)/2;
	mocpl->metcontinset = mocpl->actcontinset + H0;

	/* determine the multicut information */
	mocpl->moscutsize = cutsize = mocmos_mpa.f1;
	cutindent = mocmos_mpa.f3;
	mocpl->moscutsep = cutsep = mocmos_mpa.f4;
	mocpl->numcuts = (activewid-cutindent*2+cutsep) / (cutsize+cutsep);
	if (mocpl->numcuts <= 0) mocpl->numcuts = 1;
	if (mocpl->numcuts != 1)
		mocpl->moscutbase = (activewid-cutindent*2 - cutsize*mocpl->numcuts -
			cutsep*(mocpl->numcuts-1)) / 2 + (nodewid-activewid)/2 + cutindent;

	/* now compute the number of polygons */
	extracuts = (mocpl->numcuts-1)*2 - (2-mocpl->numcontacts) * mocpl->numcuts;
	count = tech_nodepolys(ni, reasonable, win, pl) + extracuts - mocpl->boxoffset;
	thistn = np->tech->nodeprotos[pindex-1];
	pl->realpolys = thistn->layercount + extracuts;
	return(count);
}

void mocmos_iteratescalabletransistor(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl, MOCPOLYLOOP *mocpl)
{
	TECH_POLYGON *lay;
	REGISTER INTBIG i, lambda, count, cut, pindex, shift;
	REGISTER TECH_NODES *thistn;
	REGISTER NODEPROTO *np;
	TECH_POLYGON localtp;
	INTBIG mypoints[8];

	np = ni->proto;
	pindex = np->primindex;
	thistn = mocmos_tech->nodeprotos[pindex-1];
	box += mocpl->boxoffset;
	if (box <= 7)
	{
		lay = &thistn->layerlist[box];
		lambda = lambdaofnode(ni);
		localtp.layernum = lay->layernum;
		localtp.portnum = lay->portnum;
		localtp.count = lay->count;
		localtp.style = lay->style;
		localtp.representation = lay->representation;
		localtp.points = mypoints;
		for(i=0; i<8; i++) mypoints[i] = lay->points[i];
		switch (box)
		{
			case 4:		/* active that passes through gate */
				mypoints[1] = mocpl->actinset;
				mypoints[5] = -mocpl->actinset;
				break;
			case 0:		/* active surrounding contacts */
			case 2:
				mypoints[1] = mocpl->actcontinset;
				mypoints[5] = -mocpl->actcontinset;
				if (mocpl->insetcontacts)
				{
					if (mypoints[3] < 0) shift = -H0; else shift = H0;
					mypoints[3] += shift;
					mypoints[7] += shift;
				}
				break;
			case 5:		/* poly */
				mypoints[1] = mocpl->polyinset;
				mypoints[5] = -mocpl->polyinset;
				break;
			case 1:		/* metal surrounding contacts */
			case 3:
				mypoints[1] = mocpl->metcontinset;
				mypoints[5] = -mocpl->metcontinset;
				if (mocpl->insetcontacts)
				{
					if (mypoints[3] < 0) shift = -H0; else shift = H0;
					mypoints[3] += shift;
					mypoints[7] += shift;
				}
				break;
			case 6:		/* well and select */
			case 7:
				if (mocpl->insetcontacts)
				{
					mypoints[3] += H0;
					mypoints[7] -= H0;
				}
				break;
		}
		tech_fillpoly(poly, &localtp, ni, lambda, FILLED);
		poly->desc = mocmos_tech->layers[poly->layer];
		if (lay->portnum < 0) poly->portproto = NOPORTPROTO; else
			poly->portproto = thistn->portlist[lay->portnum].addr;
		return;
	}
	if (box >= pl->realpolys)
	{
		/* displayable variables */
		(void)tech_filldisplayablenvar(ni, poly, pl->curwindowpart, 0, pl);
		return;
	}

	/* multiple contact cuts */
	count = thistn->layercount - 2;
	if (box >= count)
	{
		lambda = lambdaofnode(ni);
		lay = &thistn->layerlist[count+(box-count) / mocpl->numcuts];
		cut = (box-count) % mocpl->numcuts;
		localtp.layernum = lay->layernum;
		localtp.portnum = lay->portnum;
		localtp.count = lay->count;
		localtp.style = lay->style;
		localtp.representation = lay->representation;
		localtp.points = mypoints;
		for(i=0; i<8; i++) mypoints[i] = lay->points[i];

		if (mocpl->numcuts == 1)
		{
			mypoints[1] = (ni->highx-ni->lowx)/2 * WHOLE/lambda - mocpl->moscutsize/2;
			mypoints[5] = (ni->highx-ni->lowx)/2 * WHOLE/lambda + mocpl->moscutsize/2;
		} else
		{
			mypoints[1] = mocpl->moscutbase + cut * (mocpl->moscutsize + mocpl->moscutsep);
			mypoints[5] = mypoints[1] + mocpl->moscutsize;
		}
		if (mocpl->insetcontacts)
		{
			if (mypoints[3] < 0) shift = -H0; else shift = H0;
			mypoints[3] += shift;
			mypoints[7] += shift;
		}

		tech_fillpoly(poly, &localtp, ni, lambda, FILLED);
		poly->desc = mocmos_tech->layers[poly->layer];
		poly->portproto = NOPORTPROTO;
		return;
	}
	tech_shapenodepoly(ni, box, poly, pl);
}

/******************** ARC DESCRIPTION ********************/

INTBIG mocmos_arcpolys(ARCINST *ai, WINDOWPART *win)
{
	return(mocmos_intarcpolys(ai, win, &tech_oneprocpolyloop, &mocmos_oneprocpolyloop));
}

INTBIG mocmos_intarcpolys(ARCINST *ai, WINDOWPART *win, POLYLOOP *pl, MOCPOLYLOOP *mocpl)
{
	REGISTER INTBIG i;

	i = 1;
	mocpl->arrowbox = -1;
	if ((ai->userbits&ISDIRECTIONAL) != 0) mocpl->arrowbox = i++;

	/* add in displayable variables */
	pl->realpolys = i;
	i += tech_displayableavars(ai, win, pl);
	return(i);
}

void mocmos_shapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly)
{
	mocmos_intshapearcpoly(ai, box, poly, &tech_oneprocpolyloop, &mocmos_oneprocpolyloop);
}

void mocmos_intshapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly, POLYLOOP *pl, MOCPOLYLOOP *mocpl)
{
	REGISTER INTBIG aindex;
	REGISTER INTBIG angle;
	REGISTER INTBIG x1,y1, x2,y2, i;
	REGISTER TECH_ARCLAY *thista;
	static GRAPHICS intense = {LAYERO, RED, SOLIDC, SOLIDC,
		{0,0,0,0,0,0,0,0}, NOVARIABLE, 0};

	/* handle displayable variables */
	if (box >= pl->realpolys)
	{
		(void)tech_filldisplayableavar(ai, poly, pl->curwindowpart, 0, pl);
		return;
	}

	/* initialize for the arc */
	aindex = ai->proto->arcindex;
	thista = &mocmos_arcprotos[aindex]->list[box];
	poly->layer = thista->lay;
	switch (ai->proto->arcindex)
	{
		case AMETAL1:
		case AMETAL2:
		case AMETAL3:
		case AMETAL4:
		case AMETAL5:
		case AMETAL6:
			intense.col = BLUE;
			break;
		case APOLY1:
		case APOLY2:
			intense.col = RED;
			break;
		case APACT:
		case ANACT:
		case AACT:
			intense.col = DGREEN;
			break;
	}
	if (mocpl->arrowbox < 0 || box == 0)
	{
		/* simple arc */
		poly->desc = mocmos_tech->layers[poly->layer];
		makearcpoly(ai->length, ai->width-ai->proto->nominalwidth, ai, poly, thista->style);
		return;
	}

	/* prepare special information for directional arcs */
	poly->desc = &intense;
	x1 = ai->end[0].xpos;   y1 = ai->end[0].ypos;
	x2 = ai->end[1].xpos;   y2 = ai->end[1].ypos;
	angle = ((ai->userbits&AANGLE) >> AANGLESH) * 10;
	if ((ai->userbits&REVERSEEND) != 0)
	{
		i = x1;   x1 = x2;   x2 = i;
		i = y1;   y1 = y2;   y2 = i;
		angle = (angle+1800) % 3600;
	}

	/* draw the directional arrow */
	poly->style = VECTORS;
	poly->layer = -1;
	if (poly->limit < 2) (void)extendpolygon(poly, 2);
	poly->count = 0;
	if ((ai->userbits&NOTEND1) == 0)
		tech_addheadarrow(poly, angle, x2, y2, lambdaofarc(ai));
}

INTBIG mocmos_allarcpolys(ARCINST *ai, POLYLIST *plist, WINDOWPART *win)
{
	REGISTER INTBIG tot, j;
	POLYLOOP mypl;
	MOCPOLYLOOP mymocpl;

	mypl.curwindowpart = win;
	tot = mocmos_intarcpolys(ai, win, &mypl, &mymocpl);
	if (ensurepolylist(plist, tot, db_cluster)) return(-1);
	for(j = 0; j < tot; j++)
	{
		mocmos_intshapearcpoly(ai, j, plist->polygons[j], &mypl, &mymocpl);
	}
	return(tot);
}

INTBIG mocmos_arcwidthoffset(ARCINST *ai)
{
	return(ai->proto->nominalwidth);
}

/******************** SUPPORT ROUTINES ********************/

/*
 * Routine to switch N and P layers (not terribly useful)
 */
void mocmos_switchnp(void)
{
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni, *rni;
	REGISTER ARCINST *ai;
	REGISTER ARCPROTO *ap, *app, *apn;
	REGISTER PORTPROTO *pp, *rpp;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER INTBIG i, j, k;

	/* find the important node and arc prototypes */
	mocmos_setupprimswap(NPACTP, NNACTP, &mocmos_primswap[0]);
	mocmos_setupprimswap(NMETPACTC, NMETNACTC, &mocmos_primswap[1]);
	mocmos_setupprimswap(NTRANSP, NTRANSN, &mocmos_primswap[2]);
	mocmos_setupprimswap(NPWBUT, NNWBUT, &mocmos_primswap[3]);
	mocmos_setupprimswap(NPACTIVEN, NNACTIVEN, &mocmos_primswap[4]);
	mocmos_setupprimswap(NSELECTPN, NSELECTNN, &mocmos_primswap[5]);
	mocmos_setupprimswap(NWELLPN, NWELLNN, &mocmos_primswap[6]);
	app = apn = NOARCPROTO;
	for(ap = mocmos_tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
	{
		if (namesame(ap->protoname, x_("P-Active")) == 0) app = ap;
		if (namesame(ap->protoname, x_("N-Active")) == 0) apn = ap;
	}

	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto->primindex == 0) continue;
				if (ni->proto->tech != mocmos_tech) continue;
				for(i=0; i<7; i++)
				{
					for(k=0; k<2; k++)
					{
						if (ni->proto == mocmos_primswap[i].np[k])
						{
							ni->proto = mocmos_primswap[i].np[1-k];
							for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
							{
								for(j=0; j<mocmos_primswap[i].portcount; j++)
								{
									if (pi->proto == mocmos_primswap[i].pp[k][j])
									{
										pi->proto = mocmos_primswap[i].pp[1-k][j];
										break;
									}
								}
							}
							for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
							{
								for(j=0; j<mocmos_primswap[i].portcount; j++)
								{
									if (pe->proto == mocmos_primswap[i].pp[k][j])
									{
										pe->proto = mocmos_primswap[i].pp[1-k][j];
										pe->exportproto->subportproto = pe->proto;
										break;
									}
								}
							}
							break;
						}
					}
				}
			}
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			{
				if (ai->proto->tech != mocmos_tech) continue;
				if (ai->proto == app)
				{
					ai->proto = apn;
				} else if (ai->proto == apn)
				{
					ai->proto = app;
				}
			}
		}
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			{
				/* find the primitive at the bottom */
				rpp = pp->subportproto;
				rni = pp->subnodeinst;
				while (rni->proto->primindex == 0)
				{
					rni = rpp->subnodeinst;
					rpp = rpp->subportproto;
				}
				pp->connects = rpp->connects;
			}
		}
	}
	for(i=0; i<7; i++)
	{
		ni = mocmos_primswap[i].np[0]->firstinst;
		mocmos_primswap[i].np[0]->firstinst = mocmos_primswap[i].np[1]->firstinst;
		mocmos_primswap[i].np[1]->firstinst = ni;
	}
}

/*
 * Helper routine for "mocmos_switchnp()".
 */
void mocmos_setupprimswap(INTBIG index1, INTBIG index2, PRIMSWAP *swap)
{
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;

	swap->np[0] = swap->np[1] = NONODEPROTO;
	for(np = mocmos_tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (np->primindex == index1) swap->np[0] = np;
		if (np->primindex == index2) swap->np[1] = np;
	}
	if (swap->np[0] == NONODEPROTO || swap->np[1] == NONODEPROTO) return;
	swap->portcount = 0;
	for(pp = swap->np[0]->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		swap->pp[0][swap->portcount++] = pp;
	swap->portcount = 0;
	for(pp = swap->np[1]->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		swap->pp[1][swap->portcount++] = pp;
}

/*
 * Routine to set the technology to state "newstate", which encodes the number of metal
 * layers, whether it is a deep process, and other rules.
 */
void mocmos_setstate(INTBIG newstate)
{
	extern void tech_initmaxdrcsurround(void);

	switch (newstate&MOCMOSMETALS)
	{
		/* cannot use deep rules if less than 5 layers of metal */
		case MOCMOS2METAL:
		case MOCMOS3METAL:
		case MOCMOS4METAL:
			if ((newstate&MOCMOSRULESET) == MOCMOSDEEPRULES)
				newstate = (newstate & ~MOCMOSRULESET) | MOCMOSSUBMRULES;
			break;

		/* cannot use scmos rules if more than 4 layers of metal */
		case MOCMOS5METAL:
		case MOCMOS6METAL:
			if ((newstate&MOCMOSRULESET) == MOCMOSSCMOSRULES)
				newstate = (newstate & ~MOCMOSRULESET) | MOCMOSSUBMRULES;
			break;
	}

	if (mocmos_state == newstate) return;
	mocmos_state = newstate;

	/* set stick-figure state */
	if ((mocmos_state&MOCMOSSTICKFIGURE) != 0)
	{
		/* stick figure drawing */
		mocmos_tech->nodesizeoffset = mocmos_nodesizeoffset;
		mocmos_tech->arcpolys = mocmos_arcpolys;
		mocmos_tech->shapearcpoly = mocmos_shapearcpoly;
		mocmos_tech->allarcpolys = mocmos_allarcpolys;
		mocmos_tech->arcwidthoffset = mocmos_arcwidthoffset;
	} else
	{
		/* full figure drawing */
		mocmos_tech->nodesizeoffset = 0;
		mocmos_tech->arcpolys = 0;
		mocmos_tech->shapearcpoly = 0;
		mocmos_tech->allarcpolys = 0;
		mocmos_tech->arcwidthoffset = 0;
	}

	/* set rules */
	if (mocmos_loadDRCtables()) return;

	/* handle scalable transistors */
	if ((mocmos_state&MOCMOSSPECIALTRAN) == 0)
	{
		/* hide scalable transistors */
		mocmos_tpas.creation->userbits |= NNOTUSED;
		mocmos_tnas.creation->userbits |= NNOTUSED;
		mocmos_vpnp.creation->userbits |= NNOTUSED;
	} else
	{
		/* show scalable transistors */
		mocmos_tpas.creation->userbits &= ~NNOTUSED;
		mocmos_tnas.creation->userbits &= ~NNOTUSED;
		mocmos_vpnp.creation->userbits &= ~NNOTUSED;
	}

	/* disable Metal-3/4/5/6-Pin, Metal-2/3/4/5-Metal-3/4/5/6-Con, Metal-3/4/5/6-Node, Via-2/3/4/5-Node */
	mocmos_pm3.creation->userbits |= NNOTUSED;
	mocmos_pm4.creation->userbits |= NNOTUSED;
	mocmos_pm5.creation->userbits |= NNOTUSED;
	mocmos_pm6.creation->userbits |= NNOTUSED;
	mocmos_m2m3.creation->userbits |= NNOTUSED;
	mocmos_m3m4.creation->userbits |= NNOTUSED;
	mocmos_m4m5.creation->userbits |= NNOTUSED;
	mocmos_m5m6.creation->userbits |= NNOTUSED;
	mocmos_m3.creation->userbits |= NNOTUSED;
	mocmos_m4.creation->userbits |= NNOTUSED;
	mocmos_m5.creation->userbits |= NNOTUSED;
	mocmos_m6.creation->userbits |= NNOTUSED;
	mocmos_v2.creation->userbits |= NNOTUSED;
	mocmos_v3.creation->userbits |= NNOTUSED;
	mocmos_v4.creation->userbits |= NNOTUSED;
	mocmos_v5.creation->userbits |= NNOTUSED;

	/* disable Polysilicon-2 */
	mocmos_a_po2.creation->userbits |= ANOTUSED;
	mocmos_pp2.creation->userbits |= NNOTUSED;
	mocmos_mp2.creation->userbits |= NNOTUSED;
	mocmos_mp12.creation->userbits |= NNOTUSED;
	mocmos_p2.creation->userbits |= NNOTUSED;

	/* disable metal 3-6 arcs */
	mocmos_a_m3.creation->userbits |= ANOTUSED;
	mocmos_a_m4.creation->userbits |= ANOTUSED;
	mocmos_a_m5.creation->userbits |= ANOTUSED;
	mocmos_a_m6.creation->userbits |= ANOTUSED;

	/* enable the desired nodes */
	switch (mocmos_state&MOCMOSMETALS)
	{
		case MOCMOS6METAL:
			mocmos_pm6.creation->userbits &= ~NNOTUSED;
			mocmos_m5m6.creation->userbits &= ~NNOTUSED;
			mocmos_m6.creation->userbits &= ~NNOTUSED;
			mocmos_v5.creation->userbits &= ~NNOTUSED;
			mocmos_a_m6.creation->userbits &= ~ANOTUSED;
			/* FALLTHROUGH */ 
		case MOCMOS5METAL:
			mocmos_pm5.creation->userbits &= ~NNOTUSED;
			mocmos_m4m5.creation->userbits &= ~NNOTUSED;
			mocmos_m5.creation->userbits &= ~NNOTUSED;
			mocmos_v4.creation->userbits &= ~NNOTUSED;
			mocmos_a_m5.creation->userbits &= ~ANOTUSED;
			/* FALLTHROUGH */ 
		case MOCMOS4METAL:
			mocmos_pm4.creation->userbits &= ~NNOTUSED;
			mocmos_m3m4.creation->userbits &= ~NNOTUSED;
			mocmos_m4.creation->userbits &= ~NNOTUSED;
			mocmos_v3.creation->userbits &= ~NNOTUSED;
			mocmos_a_m4.creation->userbits &= ~ANOTUSED;
			/* FALLTHROUGH */ 
		case MOCMOS3METAL:
			mocmos_pm3.creation->userbits &= ~NNOTUSED;
			mocmos_m2m3.creation->userbits &= ~NNOTUSED;
			mocmos_m3.creation->userbits &= ~NNOTUSED;
			mocmos_v2.creation->userbits &= ~NNOTUSED;
			mocmos_a_m3.creation->userbits &= ~ANOTUSED;
			break;
	}
	if ((mocmos_state&MOCMOSRULESET) != MOCMOSDEEPRULES)
	{
		if ((mocmos_state&MOCMOSTWOPOLY) != 0)
		{
			/* non-DEEP: enable Polysilicon-2 */
			mocmos_a_po2.creation->userbits &= ~ANOTUSED;
			mocmos_pp2.creation->userbits &= ~NNOTUSED;
			mocmos_mp2.creation->userbits &= ~NNOTUSED;
			mocmos_mp12.creation->userbits &= ~NNOTUSED;
			mocmos_p2.creation->userbits &= ~NNOTUSED;
		}
	}

	/* now rewrite the description */
	(void)reallocstring(&mocmos_tech->techdescript, mocmos_describestate(mocmos_state), mocmos_tech->cluster);

	/* recache design rules */
	tech_initmaxdrcsurround();
}

/*
 * Routine to remove all information in the design rule tables.
 * Returns true on error.
 */
BOOLEAN mocmos_loadDRCtables(void)
{
	REGISTER INTBIG i, tot, totarraybits, layer1, layer2, layert1, layert2, temp, index,
		distance, goodrule, when, node, pass, arc;
	INTBIG *condist, *uncondist, *condistW, *uncondistW,
		*condistM, *uncondistM, *minsize, *edgedist;
	CHAR **condistrules, **uncondistrules, **condistWrules, **uncondistWrules,
		**condistMrules, **uncondistMrules, **minsizerules, **edgedistrules,
		proc[20], metal[20], rule[100];
	REGISTER BOOLEAN errorfound;
	REGISTER TECH_NODES *nty;
	REGISTER TECH_ARCS *aty;
	REGISTER NODEPROTO *np;
	REGISTER ARCPROTO *ap;

	/* allocate local copy of DRC tables */
	tot = (MAXLAYERS * MAXLAYERS + MAXLAYERS) / 2;
	condist = (INTBIG *)emalloc(tot * SIZEOFINTBIG, el_tempcluster);
	uncondist = (INTBIG *)emalloc(tot * SIZEOFINTBIG, el_tempcluster);
	condistW = (INTBIG *)emalloc(tot * SIZEOFINTBIG, el_tempcluster);
	uncondistW = (INTBIG *)emalloc(tot * SIZEOFINTBIG, el_tempcluster);
	condistM = (INTBIG *)emalloc(tot * SIZEOFINTBIG, el_tempcluster);
	uncondistM = (INTBIG *)emalloc(tot * SIZEOFINTBIG, el_tempcluster);
	edgedist = (INTBIG *)emalloc(tot * SIZEOFINTBIG, el_tempcluster);
	condistrules = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	uncondistrules = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	condistWrules = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	uncondistWrules = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	condistMrules = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	uncondistMrules = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	edgedistrules = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	minsize = (INTBIG *)emalloc(MAXLAYERS * SIZEOFINTBIG, el_tempcluster);
	minsizerules = (CHAR **)emalloc(MAXLAYERS * (sizeof (CHAR *)), el_tempcluster);

	/* clear all rules */
	for(i=0; i<tot; i++)
	{
		condist[i] = uncondist[i] = XX;
		condistW[i] = uncondistW[i] = XX;
		condistM[i] = uncondistM[i] = XX;
		edgedist[i] = XX;
		(void)allocstring(&condistrules[i], x_(""), el_tempcluster);
		(void)allocstring(&uncondistrules[i], x_(""), el_tempcluster);
		(void)allocstring(&condistWrules[i], x_(""), el_tempcluster);
		(void)allocstring(&uncondistWrules[i], x_(""), el_tempcluster);
		(void)allocstring(&condistMrules[i], x_(""), el_tempcluster);
		(void)allocstring(&uncondistMrules[i], x_(""), el_tempcluster);
		(void)allocstring(&edgedistrules[i], x_(""), el_tempcluster);
	}
	for(i=0; i<MAXLAYERS; i++)
	{
		minsize[i] = XX;
		(void)allocstring(&minsizerules[i], x_(""), el_tempcluster);
	}

	/* load the DRC tables from the explanation table */
	errorfound = FALSE;
	for(pass=0; pass<2; pass++)
	{
		for(i=0; mocmos_drcrules[i].rule != 0; i++)
		{
			/* see if the rule applies */
			if (pass == 0)
			{
				if (mocmos_drcrules[i].ruletype == NODSIZ) continue;
			} else
			{
				if (mocmos_drcrules[i].ruletype != NODSIZ) continue;
			}

			when = mocmos_drcrules[i].when;
			goodrule = 1;
			if ((when&(DE|SU|SC)) != 0)
			{
				switch (mocmos_state&MOCMOSRULESET)
				{
					case MOCMOSDEEPRULES:  if ((when&DE) == 0) goodrule = 0;   break;
					case MOCMOSSUBMRULES:  if ((when&SU) == 0) goodrule = 0;   break;
					case MOCMOSSCMOSRULES: if ((when&SC) == 0) goodrule = 0;   break;
				}
				if (goodrule == 0) continue;
			}
			if ((when&(M2|M3|M4|M5|M6)) != 0)
			{
				switch (mocmos_state&MOCMOSMETALS)
				{
					case MOCMOS2METAL:  if ((when&M2) == 0) goodrule = 0;   break;
					case MOCMOS3METAL:  if ((when&M3) == 0) goodrule = 0;   break;
					case MOCMOS4METAL:  if ((when&M4) == 0) goodrule = 0;   break;
					case MOCMOS5METAL:  if ((when&M5) == 0) goodrule = 0;   break;
					case MOCMOS6METAL:  if ((when&M6) == 0) goodrule = 0;   break;
				}
				if (goodrule == 0) continue;
			}
			if ((when&AC) != 0)
			{
				if ((mocmos_state&MOCMOSALTAPRULES) == 0) continue;
			}
			if ((when&NAC) != 0)
			{
				if ((mocmos_state&MOCMOSALTAPRULES) != 0) continue;
			}
			if ((when&SV) != 0)
			{
				if ((mocmos_state&MOCMOSNOSTACKEDVIAS) != 0) continue;
			}
			if ((when&NSV) != 0)
			{
				if ((mocmos_state&MOCMOSNOSTACKEDVIAS) == 0) continue;
			}

			/* find the layer names */
			if (mocmos_drcrules[i].layer1 == 0) layer1 = -1; else
			{
				for(layer1=0; layer1<mocmos_tech->layercount; layer1++)
					if (namesame(mocmos_layer_names[layer1], mocmos_drcrules[i].layer1) == 0) break;
				if (layer1 >= mocmos_tech->layercount)
				{
					ttyputerr(x_("Warning: no layer '%s' in mocmos technology"), mocmos_drcrules[i].layer1);
					errorfound = TRUE;
					break;
				}
			}
			if (mocmos_drcrules[i].layer2 == 0) layer2 = -1; else
			{
				for(layer2=0; layer2<mocmos_tech->layercount; layer2++)
					if (namesame(mocmos_layer_names[layer2], mocmos_drcrules[i].layer2) == 0) break;
				if (layer2 >= mocmos_tech->layercount)
				{
					ttyputerr(x_("Warning: no layer '%s' in mocmos technology"), mocmos_drcrules[i].layer2);
					errorfound = TRUE;
					break;
				}
			}
			node = -1;
			nty = 0;
			aty = 0;
			if (mocmos_drcrules[i].nodename != 0)
			{
				if (mocmos_drcrules[i].ruletype == ASURROUND)
				{
					for(arc=0; arc<mocmos_tech->arcprotocount; arc++)
					{
						aty = mocmos_tech->arcprotos[arc];
						ap = aty->creation;
						if (namesame(ap->protoname, mocmos_drcrules[i].nodename) == 0) break;
					}
					if (arc >= mocmos_tech->arcprotocount)
					{
						ttyputerr(x_("Warning: no arc '%s' in mocmos technology"), mocmos_drcrules[i].nodename);
						errorfound = TRUE;
						break;
					}
				} else
				{
					for(node=0; node<mocmos_tech->nodeprotocount; node++)
					{
						nty = mocmos_tech->nodeprotos[node];
						np = nty->creation;
						if (namesame(np->protoname, mocmos_drcrules[i].nodename) == 0) break;
					}
					if (node >= mocmos_tech->nodeprotocount)
					{
						ttyputerr(x_("Warning: no node '%s' in mocmos technology"), mocmos_drcrules[i].nodename);
						errorfound = TRUE;
						break;
					}
				}
			}

			/* get more information about the rule */
			distance = mocmos_drcrules[i].distance;
			proc[0] = 0;
			if ((when&(DE|SU|SC)) != 0)
			{
				switch (mocmos_state&MOCMOSRULESET)
				{
					case MOCMOSDEEPRULES:  estrcpy(proc, x_("DEEP"));   break;
					case MOCMOSSUBMRULES:  estrcpy(proc, x_("SUBM"));   break;
					case MOCMOSSCMOSRULES: estrcpy(proc, x_("SCMOS"));  break;
				}
			}
			metal[0] = 0;
			if ((when&(M2|M3|M4|M5|M6)) != 0)
			{
				switch (mocmos_state&MOCMOSMETALS)
				{
					case MOCMOS2METAL:  estrcpy(metal, x_("2m"));   break;
					case MOCMOS3METAL:  estrcpy(metal, x_("3m"));   break;
					case MOCMOS4METAL:  estrcpy(metal, x_("4m"));   break;
					case MOCMOS5METAL:  estrcpy(metal, x_("5m"));   break;
					case MOCMOS6METAL:  estrcpy(metal, x_("6m"));   break;
				}
				if (goodrule == 0) continue;
			}
			estrcpy(rule, mocmos_drcrules[i].rule);
			if (proc[0] != 0 || metal[0] != 0)
			{
				estrcat(rule, x_(", "));
				estrcat(rule, metal);
				estrcat(rule, proc);
			}
			layert1 = layer1;   layert2 = layer2;
			if (layert1 > layert2) { temp = layert1; layert1 = layert2;  layert2 = temp; }
			index = (layert1+1) * (layert1/2) + (layert1&1) * ((layert1+1)/2);
			index = layert2 + mocmos_tech->layercount * layert1 - index;

			/* set the rule */
			switch (mocmos_drcrules[i].ruletype)
			{
				case MINWID:
					minsize[layer1] = distance;
					(void)reallocstring(&minsizerules[layer1], rule, el_tempcluster);
					mocmos_setlayerminwidth(mocmos_drcrules[i].layer1, distance);
					break;
				case NODSIZ:
					mocmos_setdefnodesize(nty, node, distance, distance);
					break;
				case SURROUND:
					mocmos_setlayersurroundlayer(nty, layer1, layer2, distance, minsize);
					break;
				case ASURROUND:
					mocmos_setarclayersurroundlayer(aty, layer1, layer2, distance);
					break;
				case VIASUR:
					mocmos_setlayersurroundvia(nty, layer1, distance);
					nty->f3 = (INTSML)distance;
					break;
				case TRAWELL:
					mocmos_settransistorwellsurround(distance);
					break;
				case TRAPOLY:
					mocmos_settransistorpolyoverhang(distance);
					break;
				case TRAACTIVE:
					mocmos_settransistoractiveoverhang(distance);
					break;
				case SPACING:
					condist[index] = uncondist[index] = distance;
					(void)reallocstring(&condistrules[index], rule, el_tempcluster);
					(void)reallocstring(&uncondistrules[index], rule, el_tempcluster);
					break;
				case SPACINGM:
					condistM[index] = uncondistM[index] = distance;
					(void)reallocstring(&condistMrules[index], rule, el_tempcluster);
					(void)reallocstring(&uncondistMrules[index], rule, el_tempcluster);
					break;
				case SPACINGW:
					condistW[index] = uncondistW[index] = distance;
					(void)reallocstring(&condistWrules[index], rule, el_tempcluster);
					(void)reallocstring(&uncondistWrules[index], rule, el_tempcluster);
					break;
				case SPACINGE:
					edgedist[index] = distance;
					(void)reallocstring(&edgedistrules[index], rule, el_tempcluster);
					break;
				case CONSPA:
					condist[index] = distance;
					(void)reallocstring(&condistrules[index], rule, el_tempcluster);
					break;
				case UCONSPA:
					uncondist[index] = distance;
					(void)reallocstring(&uncondistrules[index], rule, el_tempcluster);
					break;
				case CUTSPA:
					nty->f4 = (INTSML)distance;
					break;
				case CUTSIZE:
					nty->f1 = nty->f2 = (INTSML)distance;
					break;
				case CUTSUR:
					nty->f3 = (INTSML)distance;
					break;
			}
		}
	}
	if (!errorfound)
	{
		/* clear the rules on the technology */
		changesquiet(TRUE);
		totarraybits = VDONTSAVE|VISARRAY|(tot << VLENGTHSH);
		if (setvalkey((INTBIG)mocmos_tech, VTECHNOLOGY, dr_connected_distanceskey,
			(INTBIG)condist, VFRACT|totarraybits) == NOVARIABLE) return(TRUE);
		if (setvalkey((INTBIG)mocmos_tech, VTECHNOLOGY, dr_connected_distances_rulekey,
			(INTBIG)condistrules, VSTRING|totarraybits) == NOVARIABLE) return(TRUE);
		if (setvalkey((INTBIG)mocmos_tech, VTECHNOLOGY, dr_unconnected_distanceskey,
			(INTBIG)uncondist, VFRACT|totarraybits) == NOVARIABLE) return(TRUE);
		if (setvalkey((INTBIG)mocmos_tech, VTECHNOLOGY, dr_unconnected_distances_rulekey,
			(INTBIG)uncondistrules, VSTRING|totarraybits) == NOVARIABLE) return(TRUE);
		if (setvalkey((INTBIG)mocmos_tech, VTECHNOLOGY, dr_connected_distancesWkey,
			(INTBIG)condistW, VFRACT|totarraybits) == NOVARIABLE) return(TRUE);
		if (setvalkey((INTBIG)mocmos_tech, VTECHNOLOGY, dr_connected_distancesW_rulekey,
			(INTBIG)condistWrules, VSTRING|totarraybits) == NOVARIABLE) return(TRUE);
		if (setvalkey((INTBIG)mocmos_tech, VTECHNOLOGY, dr_unconnected_distancesWkey,
			(INTBIG)uncondistW, VFRACT|totarraybits) == NOVARIABLE) return(TRUE);
		if (setvalkey((INTBIG)mocmos_tech, VTECHNOLOGY, dr_unconnected_distancesW_rulekey,
			(INTBIG)uncondistWrules, VSTRING|totarraybits) == NOVARIABLE) return(TRUE);
		if (setvalkey((INTBIG)mocmos_tech, VTECHNOLOGY, dr_connected_distancesMkey,
			(INTBIG)condistM, VFRACT|totarraybits) == NOVARIABLE) return(TRUE);
		if (setvalkey((INTBIG)mocmos_tech, VTECHNOLOGY, dr_connected_distancesM_rulekey,
			(INTBIG)condistMrules, VSTRING|totarraybits) == NOVARIABLE) return(TRUE);
		if (setvalkey((INTBIG)mocmos_tech, VTECHNOLOGY, dr_unconnected_distancesMkey,
			(INTBIG)uncondistM, VFRACT|totarraybits) == NOVARIABLE) return(TRUE);
		if (setvalkey((INTBIG)mocmos_tech, VTECHNOLOGY, dr_unconnected_distancesM_rulekey,
			(INTBIG)uncondistMrules, VSTRING|totarraybits) == NOVARIABLE) return(TRUE);
		if (setvalkey((INTBIG)mocmos_tech, VTECHNOLOGY, dr_edge_distanceskey,
			(INTBIG)edgedist, VFRACT|totarraybits) == NOVARIABLE) return(TRUE);
		if (setvalkey((INTBIG)mocmos_tech, VTECHNOLOGY, dr_edge_distances_rulekey,
			(INTBIG)edgedistrules, VSTRING|totarraybits) == NOVARIABLE) return(TRUE);
		if (setvalkey((INTBIG)mocmos_tech, VTECHNOLOGY, dr_wide_limitkey,
			WIDELIMIT, VFRACT|VDONTSAVE) == NOVARIABLE) return(TRUE);

		/* clear minimum size rules */
		if (setvalkey((INTBIG)mocmos_tech, VTECHNOLOGY, dr_min_widthkey,
			(INTBIG)minsize, VFRACT|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)) == NOVARIABLE) return(TRUE);
		if (setvalkey((INTBIG)mocmos_tech, VTECHNOLOGY, dr_min_width_rulekey,
			(INTBIG)minsizerules, VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)) == NOVARIABLE) return(TRUE);
		changesquiet(FALSE);

		/* reset valid DRC dates */
		dr_reset_dates();
	}

	/* free rule arrays */
	/* clear all rules */
	for(i=0; i<tot; i++)
	{
		efree((CHAR *)condistrules[i]);
		efree((CHAR *)uncondistrules[i]);
		efree((CHAR *)condistWrules[i]);
		efree((CHAR *)uncondistWrules[i]);
		efree((CHAR *)condistMrules[i]);
		efree((CHAR *)uncondistMrules[i]);
		efree((CHAR *)edgedistrules[i]);
	}
	for(i=0; i<MAXLAYERS; i++)
	{
		efree((CHAR *)minsizerules[i]);
	}
	efree((CHAR *)condist);
	efree((CHAR *)uncondist);
	efree((CHAR *)condistW);
	efree((CHAR *)uncondistW);
	efree((CHAR *)condistM);
	efree((CHAR *)uncondistM);
	efree((CHAR *)edgedist);
	efree((CHAR *)condistrules);
	efree((CHAR *)uncondistrules);
	efree((CHAR *)condistWrules);
	efree((CHAR *)uncondistWrules);
	efree((CHAR *)condistMrules);
	efree((CHAR *)uncondistMrules);
	efree((CHAR *)edgedistrules);
	efree((CHAR *)minsize);
	efree((CHAR *)minsizerules);
	return(errorfound);
}

/*
 * Routine to describe the technology when it is in state "state".
 */
CHAR *mocmos_describestate(INTBIG state)
{
	REGISTER INTBIG nummetals, numpolys;
	REGISTER CHAR *rules;
	REGISTER void *infstr;

	infstr = initinfstr();
	switch (state&MOCMOSMETALS)
	{
		case MOCMOS2METAL: nummetals = 2;   break;
		case MOCMOS3METAL: nummetals = 3;   break;
		case MOCMOS4METAL: nummetals = 4;   break;
		case MOCMOS5METAL: nummetals = 5;   break;
		case MOCMOS6METAL: nummetals = 6;   break;
		default: nummetals = 2;
	}
	switch (state&MOCMOSRULESET)
	{
		case MOCMOSSCMOSRULES:
			rules = _("now standard");
			break;
		case MOCMOSDEEPRULES:
			rules = _("now deep");
			break;
		case MOCMOSSUBMRULES:
			rules = _("now submicron");
			break;
		default:
			rules = 0;
	}
	if ((state&MOCMOSTWOPOLY) != 0) numpolys = 2; else
		numpolys = 1;
	formatinfstr(infstr, _("Complementary MOS (from MOSIS, 2-6 metals [now %ld], 1-2 polys [now %ld], flex rules [%s]"),
		nummetals, numpolys, rules);
	if ((state&MOCMOSSTICKFIGURE) != 0) addstringtoinfstr(infstr, _(", stick-figures"));
	if ((state&MOCMOSNOSTACKEDVIAS) != 0) addstringtoinfstr(infstr, _(", stacked vias disallowed"));
	if ((state&MOCMOSALTAPRULES) != 0) addstringtoinfstr(infstr, _(", alternate contact rules"));
	if ((state&MOCMOSSPECIALTRAN) != 0) addstringtoinfstr(infstr, _(", shows special transistors"));
	addstringtoinfstr(infstr, x_(")"));
	return(returninfstr(infstr));
}

/*
 * Routine to implement rule 3.3 which specifies the amount of poly overhang
 * on a transistor.
 */
void mocmos_settransistorpolyoverhang(INTBIG overhang)
{
	/* define the poly box in terms of the central transistor box */
	mocmos_trpbox[1] = mocmos_trp1box[1] = mocmos_trpobox[1] - overhang;
	mocmos_trpbox[5] = mocmos_trp2box[5] = mocmos_trpobox[5] + overhang;

	/* the serpentine poly overhang */
	mocmos_tpa_l[TRANSPOLYLAYER].extendt = mocmos_tpa_l[TRANSPOLYLAYER].extendb = (INTSML)overhang;
	mocmos_tpaE_l[TRANSEPOLY1LAYER].extendb = (INTSML)overhang;
	mocmos_tpaE_l[TRANSEPOLY2LAYER].extendt = (INTSML)overhang;
	mocmos_tna_l[TRANSPOLYLAYER].extendt = mocmos_tna_l[TRANSPOLYLAYER].extendb = (INTSML)overhang;
	mocmos_tnaE_l[TRANSEPOLY1LAYER].extendb = (INTSML)overhang;
	mocmos_tnaE_l[TRANSEPOLY2LAYER].extendt = (INTSML)overhang;
}

/*
 * Routine to implement rule 3.4 which specifies the amount of active overhang
 * on a transistor.
 */
void mocmos_settransistoractiveoverhang(INTBIG overhang)
{
	INTBIG polywidth, welloverhang;

	/* pickup extension of well about active (2.3) */
	welloverhang = mocmos_trabox[1] - mocmos_trwbox[1];

	/* define the active box in terms of the central transistor box */
	mocmos_trabox[3] = mocmos_trpobox[3] - overhang;
	mocmos_trabox[7] = mocmos_trpobox[7] + overhang;
	mocmos_tra1box[7] = mocmos_trpobox[7] + overhang;
	mocmos_tra2box[3] = mocmos_trpobox[3] - overhang;

	/* extension of well about active (2.3) */
	mocmos_trwbox[3] = mocmos_trabox[3] - welloverhang;
	mocmos_trwbox[7] = mocmos_trabox[7] + welloverhang;

	/* extension of select about active = 2 (4.2) */
	mocmos_trsbox[3] = mocmos_trabox[3] - K2;
	mocmos_trsbox[7] = mocmos_trabox[7] + K2;

	/* the serpentine active overhang */
	polywidth = mocmos_tpa.ysize/2 - mocmos_trpobox[3];
	mocmos_tpa_l[TRANSACTLAYER].lwidth = (INTSML)(polywidth + overhang);
	mocmos_tpa_l[TRANSACTLAYER].rwidth = (INTSML)(polywidth + overhang);
	mocmos_tpaE_l[TRANSEACT1LAYER].lwidth = (INTSML)(polywidth + overhang);
	mocmos_tpaE_l[TRANSEACT2LAYER].rwidth = (INTSML)(polywidth + overhang);
	mocmos_tna_l[TRANSACTLAYER].lwidth = (INTSML)(polywidth + overhang);
	mocmos_tna_l[TRANSACTLAYER].rwidth = (INTSML)(polywidth + overhang);
	mocmos_tnaE_l[TRANSEACT1LAYER].lwidth = (INTSML)(polywidth + overhang);
	mocmos_tnaE_l[TRANSEACT2LAYER].rwidth = (INTSML)(polywidth + overhang);

	/* serpentine: extension of well about active (2.3) */
	mocmos_tpa_l[TRANSWELLLAYER].lwidth = (INTSML)(polywidth + overhang + welloverhang);
	mocmos_tpa_l[TRANSWELLLAYER].rwidth = (INTSML)(polywidth + overhang + welloverhang);
	mocmos_tpaE_l[TRANSEWELLLAYER].lwidth = (INTSML)(polywidth + overhang + welloverhang);
	mocmos_tpaE_l[TRANSEWELLLAYER].rwidth = (INTSML)(polywidth + overhang + welloverhang);
	mocmos_tna_l[TRANSWELLLAYER].lwidth = (INTSML)(polywidth + overhang + welloverhang);
	mocmos_tna_l[TRANSWELLLAYER].rwidth = (INTSML)(polywidth + overhang + welloverhang);
	mocmos_tnaE_l[TRANSEWELLLAYER].lwidth = (INTSML)(polywidth + overhang + welloverhang);
	mocmos_tnaE_l[TRANSEWELLLAYER].rwidth = (INTSML)(polywidth + overhang + welloverhang);

	/* serpentine: extension of select about active = 2 (4.2) */
	mocmos_tpa_l[TRANSSELECTLAYER].lwidth = (INTSML)(polywidth + overhang + K2);
	mocmos_tpa_l[TRANSSELECTLAYER].rwidth = (INTSML)(polywidth + overhang + K2);
	mocmos_tpaE_l[TRANSESELECTLAYER].lwidth = (INTSML)(polywidth + overhang + K2);
	mocmos_tpaE_l[TRANSESELECTLAYER].rwidth = (INTSML)(polywidth + overhang + K2);
	mocmos_tna_l[TRANSSELECTLAYER].lwidth = (INTSML)(polywidth + overhang + K2);
	mocmos_tna_l[TRANSSELECTLAYER].rwidth = (INTSML)(polywidth + overhang + K2);
	mocmos_tnaE_l[TRANSESELECTLAYER].lwidth = (INTSML)(polywidth + overhang + K2);
	mocmos_tnaE_l[TRANSESELECTLAYER].rwidth = (INTSML)(polywidth + overhang + K2);
}

/*
 * Routine to implement rule 2.3 which specifies the amount of well surround
 * about active on a transistor.
 */
void mocmos_settransistorwellsurround(INTBIG overhang)
{
	/* define the well box in terms of the active box */
	mocmos_trwbox[1] = mocmos_trabox[1] - overhang;
	mocmos_trwbox[3] = mocmos_trabox[3] - overhang;
	mocmos_trwbox[5] = mocmos_trabox[5] + overhang;
	mocmos_trwbox[7] = mocmos_trabox[7] + overhang;

	/* the serpentine poly overhang */
	mocmos_tpa_l[TRANSWELLLAYER].extendt = mocmos_tpa_l[TRANSWELLLAYER].extendb = (INTSML)overhang;
	mocmos_tpa_l[TRANSWELLLAYER].lwidth = mocmos_tpa_l[TRANSWELLLAYER].rwidth = (INTSML)(overhang+K4);
	mocmos_tpaE_l[TRANSEWELLLAYER].extendt = mocmos_tpaE_l[TRANSEWELLLAYER].extendb = (INTSML)overhang;
	mocmos_tpaE_l[TRANSEWELLLAYER].lwidth = mocmos_tpaE_l[TRANSEWELLLAYER].rwidth = (INTSML)(overhang+K4);
	mocmos_tna_l[TRANSWELLLAYER].extendt = mocmos_tna_l[TRANSWELLLAYER].extendb = (INTSML)overhang;
	mocmos_tna_l[TRANSWELLLAYER].lwidth = mocmos_tna_l[TRANSWELLLAYER].rwidth = (INTSML)(overhang+K4);
	mocmos_tnaE_l[TRANSEWELLLAYER].extendt = mocmos_tnaE_l[TRANSEWELLLAYER].extendb = (INTSML)overhang;
	mocmos_tnaE_l[TRANSEWELLLAYER].lwidth = mocmos_tnaE_l[TRANSEWELLLAYER].rwidth = (INTSML)(overhang+K4);
}

/*
 * Routine to change the design rules for layer "layername" layers so that
 * the layers are at least "width" wide.  Affects the default arc width
 * and the default pin size.
 */
void mocmos_setlayerminwidth(CHAR *layername, INTBIG width)
{
	REGISTER ARCPROTO *ap;
	REGISTER INTBIG lambda, i;
	INTBIG lx, hx, ly, hy;
	REGISTER NODEPROTO *np;
	REGISTER TECH_NODES *nty;
	REGISTER TECH_PORTS *npp;
	REGISTER CHAR *pt;
	REGISTER void *infstr;

	/* next find that arc and set its default width */
	for(ap = mocmos_tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
		if (namesame(ap->protoname, layername) == 0) break;
	if (ap == NOARCPROTO) return;
	lambda = el_curlib->lambda[mocmos_tech->techindex];
	ap->nominalwidth = (width + mocmos_arc_widoff[ap->arcindex]) * lambda / WHOLE;

	/* finally, find that arc's pin and set its size and port offset */
	infstr = initinfstr();
	formatinfstr(infstr, x_("%s-Pin"), layername);
	pt = returninfstr(infstr);
	np = NONODEPROTO;
	nty = 0;
	for(i=0; i<mocmos_tech->nodeprotocount; i++)
	{
		nty = mocmos_tech->nodeprotos[i];
		np = nty->creation;
		if (namesame(np->protoname, pt) == 0) break;
	}
	if (np != NONODEPROTO)
	{
		nodeprotosizeoffset(np, &lx, &ly, &hx, &hy, NONODEPROTO);
		lambda = el_curlib->lambda[mocmos_tech->techindex];
		np->lowx = -width * lambda / WHOLE / 2 - lx;
		np->highx = width * lambda / WHOLE / 2 + hx;
		np->lowy = -width * lambda / WHOLE / 2 - ly;
		np->highy = width * lambda / WHOLE / 2 + hy;
		npp = &nty->portlist[0];
		npp->lowxsum = (INTSML)(width/2 + lx * WHOLE / lambda);
		npp->lowysum = (INTSML)(width/2 + ly * WHOLE / lambda);
		npp->highxsum = (INTSML)(-width/2 - hx * WHOLE / lambda);
		npp->highysum = (INTSML)(-width/2 - hy * WHOLE / lambda);
	}
}

INTBIG *mocmos_offsetboxes[] =
{
	mocmos_fullbox,
	mocmos_in0hbox,
	mocmos_in1box,
	mocmos_in1hbox,
	mocmos_in2box,
	mocmos_in2hbox,
	mocmos_in3_2box,
	mocmos_in3h_1hbox,
	mocmos_in3box,
	mocmos_in3hbox,
	mocmos_in4box,
	mocmos_in4hbox,
	mocmos_in5box,
	mocmos_in5hbox,
	mocmos_in6box,
	mocmos_in6hbox,
	0
};

/*
 * Routine to set the surround distance of layer "layer" from the via in node "nodename" to "surround".
 */
void mocmos_setlayersurroundvia(TECH_NODES *nty, INTBIG layer, INTBIG surround)
{
	REGISTER INTBIG i, j, viasize, layersize, indent;

	/* find the via size */
	viasize = nty->f1;
	layersize = viasize + surround*2;
	indent = (nty->xsize - layersize) / 2;
	for(i=0; mocmos_offsetboxes[i] != 0; i++)
		if (mocmos_offsetboxes[i][1] == indent) break;
	if (mocmos_offsetboxes[i] == 0)
		ttyputerr(x_("MOSIS CMOS Submicron technology has no box that offsets by %s"), frtoa(indent)); else
	{
		for(j=0; j<nty->layercount; j++)
			if (nty->layerlist[j].layernum == layer) break;
		if (j >= nty->layercount) return;
		nty->layerlist[j].points = mocmos_offsetboxes[i];
	}
}

/*
 * Routine to set the surround distance of layer "outerlayer" from layer "innerlayer"
 * in node "nty" to "surround".  The array "minsize" is the minimum size of each layer.
 */
void mocmos_setlayersurroundlayer(TECH_NODES *nty, INTBIG outerlayer, INTBIG innerlayer, INTBIG surround, INTBIG *minsize)
{
	REGISTER INTBIG i, j, lxindent, hxindent, lyindent, hyindent, xsize, ysize;

	/* find the inner layer */
	for(j=0; j<nty->layercount; j++)
		if (nty->layerlist[j].layernum == innerlayer) break;
	if (j >= nty->layercount)
	{
		ttyputerr(x_("Internal error in MOCMOS surround computation"));
		return;
	}

	/* find that layer in the specified node */
	for(i=0; mocmos_offsetboxes[i] != 0; i++)
		if (mocmos_offsetboxes[i] == nty->layerlist[j].points) break;
	if (mocmos_offsetboxes[i] == 0)
	{
		ttyputerr(x_("MOSIS CMOS Submicron technology cannot determine indentation of layer %ld on %s"),
			innerlayer, nty->creation->protoname);
		return;
	}

	/* determine if minimum size design rules are met */
	lxindent = mocmos_offsetboxes[i][1] - surround;
	hxindent = -mocmos_offsetboxes[i][5] - surround;
	lyindent = mocmos_offsetboxes[i][3] - surround;
	hyindent = -mocmos_offsetboxes[i][7] - surround;
	xsize = nty->xsize - lxindent - hxindent;
	ysize = nty->ysize - lyindent - hyindent;
	if (xsize < minsize[outerlayer] || ysize < minsize[outerlayer])
	{
		/* make it irregular to force the proper minimum size */
		if (xsize < minsize[outerlayer]) hxindent -= minsize[outerlayer] - xsize;
		if (ysize < minsize[outerlayer]) hyindent -= minsize[outerlayer] - ysize;
	}

	/* find an appropriate descriptor for this surround amount */
	for(i=0; mocmos_offsetboxes[i] != 0; i++)
		if (mocmos_offsetboxes[i][1] == lxindent &&
			mocmos_offsetboxes[i][3] == lyindent &&
			mocmos_offsetboxes[i][5] == -hxindent &&
			mocmos_offsetboxes[i][7] == -hyindent) break;
	if (mocmos_offsetboxes[i] == 0)
	{
		ttyputerr(x_("MOSIS CMOS Submicron technology has no box that offsets lx=%s hx=%s ly=%s hy=%s"),
			frtoa(lxindent), frtoa(hxindent), frtoa(lyindent), frtoa(hyindent));
		return;
	}

	/* find the outer layer and set that size */
	for(j=0; j<nty->layercount; j++)
		if (nty->layerlist[j].layernum == outerlayer) break;
	if (j >= nty->layercount)
	{
		ttyputerr(x_("Internal error in MOCMOS surround computation"));
		return;
	}
	nty->layerlist[j].points = mocmos_offsetboxes[i];
}

/*
 * Routine to set the surround distance of layer "outerlayer" from layer "innerlayer"
 * in arc "aty" to "surround".
 */
void mocmos_setarclayersurroundlayer(TECH_ARCS *aty, INTBIG outerlayer, INTBIG innerlayer, INTBIG surround)
{
	REGISTER INTBIG j, indent;

	/* find the inner layer */
	for(j=0; j<aty->laycount; j++)
		if (aty->list[j].lay == innerlayer) break;
	if (j >= aty->laycount)
	{
		ttyputerr(x_("Internal error in MOCMOS surround computation"));
		return;
	}

	indent = aty->list[j].off - surround*2;

	for(j=0; j<aty->laycount; j++)
		if (aty->list[j].lay == outerlayer) break;
	if (j >= aty->laycount)
	{
		ttyputerr(x_("Internal error in MOCMOS surround computation"));
		return;
	}
	aty->list[j].off = indent;
}

/*
 * Routine to set the true node size (the highlighted area) of node "nodename" to "wid" x "hei".
 */
void mocmos_setdefnodesize(TECH_NODES *nty, INTBIG index, INTBIG wid, INTBIG hei)
{
	REGISTER INTBIG xindent, yindent;
	REGISTER VARIABLE *var;

	xindent = (nty->xsize - wid) / 2;
	yindent = (nty->ysize - hei) / 2;

	var = getval((INTBIG)mocmos_tech, VTECHNOLOGY, VFRACT|VISARRAY, x_("TECH_node_width_offset"));
	if (var != NOVARIABLE)
	{
		((INTBIG *)var->addr)[index*4] = xindent;
		((INTBIG *)var->addr)[index*4+1] = xindent;
		((INTBIG *)var->addr)[index*4+2] = yindent;
		((INTBIG *)var->addr)[index*4+3] = yindent;
	}
}
