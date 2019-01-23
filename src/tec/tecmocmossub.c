/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tecmocmossub.c
 * MOSIS CMOS Submicron technology description
 * Written by: Steven M. Rubin, Static Free Software
 * The MOSIS 6-metal, 2-poly submicron rules,
 * interpreted by Richard "Bic" Schediwy
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

/*
 * Assumptions made from the MOSIS submicron design rules
 * (MOSIS rule is in square brackets []):
 *
 * Arc widths (minimum sizes):
 *   metal1: 3 [7.1]
 *   metal2: 3 [9.1]
 *   metal3: 5 (if 3-metal process) [15.1]
 *   metal3: 3 (if 4-metal process) [15.1]
 *   metal4: 6 [22.1]
 *   poly1:  2 [3.1]
 *   poly2:  3 [11.1]
 *   p/n active (active&select&well):
 *     active: 3 [2.1], select extends by 2 [4.2], well extends by 6 [2.3]
 *   active: 3 [2.1]
 *
 * Pin/Node sizes (minimum sizes):
 *   metal1: 3 [7.1]
 *   metal2: 3 [9.1]
 *   metal3: 5 (if 3-metal process) [15.1]
 *   metal3: 3 (if 4-metal process) [15.1]
 *   metal4: 6 [22.1]
 *   poly1:  2 [3.1]
 *   poly2:  3 [11.1]
 *   active: 3 [2.1]
 *   select: 2 [4.4]
 *   well:  12 [1.1]
 *
 * Special nodes:
 *   p/n active-to-metal1 contact:
 *     cuts 2x2 [6.1], separated 3 [6.3]
 *     metal1 extends around cut by 1 (4x4) [7.3]
 *     active extends around cut by 1.5 (5x5) [6.2]
 *     select extends around active by 1 (7x7) [4.3]  {CORRECTION: by 2 (8x8) [4.2]}
 *     well extends around active by 6 (17x17) [2.3]
 *   poly1-to-metal1 contact:
 *     cuts 2x2 [5.1], separated 3 [5.3]
 *     metal1 extends around cut by 1 (4x4) [7.3]
 *     poly1 extends around cut by 1.5 (5x5) [5.2]
 *   poly2-to-metal1 contact:
 *     cuts 2x2 [5.1], separated 3 [5.3]
 *     metal1 extends around cut by 1 (4x4) [7.3]
 *     poly2 size: 3 (3x3) [11.1]
 *   poly1-to-poly2 (capacitor) contact:
 *     cuts 2x2 [5.1], separated 3 [5.3]
 *     poly2 size: 3 (3x3) [11.1]
 *     poly1 extends around poly2 by 2 (7x7) [11.3]
 *   Transistors:
 *     active is 3 wide [2.1] and sticks out by 3 (3x8) [3.4]
 *     poly1 is 2 wide [3.1] and sticks out by 2 (7x2) [3.3]
 *     transistor area is 3x2
 *     select surrounds active by 2 (7x12) [4.2]
 *     well surrounds active by 6 (15x20) [2.3]
 *   Via1:
 *     cuts 2x2 [8.1], separated 3 [8.2]
 *     metal1 extends around cut by 1 (4x4) [8.3]
 *     metal2 extends around cut by 1 (4x4) [9.3]
 *   Via2:
 *     cuts 2x2 [14.1], separated 3 [14.2]
 *     metal2 extends around cut by 1 (4x4) [14.3]
 *     metal3 extends around cut by: 2 (6x6) (if 3-metal process) [15.3]
 *     metal3 extends around cut by: 1 (4x4) (if 4-metal process) [15.3]
 *   Via3:
 *     cuts 2x2 [21.1], separated 4 [21.2]
 *     metal3 extends around cut by: 1 (4x4) [21.3]
 *     metal4 extends around cut by: 2 (6x6) [22.3]
 *   Substrate/well contact:
 *     select extends around active by 2 [4.2]
 *     well extends around active by 6 [2.3]
 *
 * DRC:
 *   metal1-to-metal1: 3 [7.2]
 *   metal2-to-metal2: 4 [9.2]
 *   metal3-to-metal3: 3 [15.2]
 *   metal4-to-metal4: 6 [22.2]
 *   poly1-to-poly1: 3 [3.2]
 *   poly1-to-active: 1 [3.5]
 *   poly2-to-poly2: 3 [11.2]
 *   poly2-to-active: 1 [3.5]
 *   poly2-to-polyCut: 3 [11.5]
 *   active-to-active: 3 [2.2]
 *   select-to-trans: 3 [4.1]
 *   polyCut/actCut-to-polyCut/actCut: 3 [5.3]
 *   polyCut/actCut-to-via1: 2 [8.4]
 *   polyCut-to-active: 2 [5.4]
 *   actCut-to-poly: 2 [6.4]
 *   via1-to-via1: 3 [8.2]
 *   via1-to-via2: 2 [14.4]
 *   via2-to-via2: 2 [14.2]
 *   via3-to-via3: 4 [21.2]
 */

/*
 * #metals:     Metal-1       Metal-2       Metal-3       Metal-4       Metal-5       Metal-6
 *
 *  2-metals:   3 wide        3 wide
 *              3 apart       4 apart
 *              1 over via1   1 over via1
 *
 *  3-metals:   3 wide        3 wide        5 wide
 *              3 apart       3 apart       3 apart
 *              1 over via1   1 over via1/2 2 over via2
 *
 *  4-metals:   3 wide        3 wide        3 wide        3 wide
 *              3 apart       3 apart       3 apart       3 apart
 *              1 over via1   1 over via1/2 1 over via2/3 1 over via3
 *
 *  5-metals:   3 wide        3 wide        3 wide        3 wide        4 wide
 *              3 apart       3 apart       3 apart       3 apart       4 apart
 *              1 over via1   1 over via1/2 1 over via2/3 1 over via3/4 1 over via4
 *
 *  6-metals:   3 wide        3 wide        3 wide        3 wide        3 wide        4 wide
 *              3 apart       3 apart       3 apart       3 apart       3 apart       4 apart
 *              1 over via1   1 over via1/2 1 over via2/3 1 over via3/4 1 over via4/5 1 over via5
 */

#include "config.h"
#include "global.h"
#include "egraphics.h"
#include "tech.h"
#include "drc.h"
#include "tecmocmossub.h"
#include "efunction.h"

/*
 * Can switch from 4-metal rules (the default) to 2/3/5/6-metal rules
 */

/* the options table */
static KEYWORD mocmossubopt[] =
{
	{x_("2-metal-rules"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("3-metal-rules"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("4-metal-rules"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("5-metal-rules"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("6-metal-rules"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP mocmossub_parse = {mocmossubopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("MOSIS CMOS Submicron option"), M_("show current options")};

static INTBIG      mocmossub_state;			/* settings */
       TECHNOLOGY *mocmossub_tech;

/* prototypes for local routines */
static void mocmossub_setstate(INTBIG newstate);
static void mocmossub_setlayerrules(CHAR *layername, INTBIG spacing, INTBIG minwidth);
static void mocmossub_setarcwidth(CHAR *arcname, INTBIG width);
static void mocmossub_setnodesize(CHAR *nodename, INTBIG size, INTBIG portoffset);
static void mocmossub_setmetalonvia(CHAR *nodename, INTBIG layer, INTBIG *boxdesc, INTBIG nodeoffset);
static CHAR *mocmossub_describestate(INTBIG state);

/******************** LAYERS ********************/

#define MAXLAYERS   40		/* total layers below         */
#define LMETAL1      0		/* metal layer 1              */
#define LMETAL2      1		/* metal layer 2              */
#define LMETAL3      2		/* metal layer 3              */
#define LMETAL4      3		/* metal layer 4              */
#define LMETAL5      4		/* metal layer 5              */
#define LMETAL6      5		/* metal layer 6              */
#define LPOLY1       6		/* polysilicon                */
#define LPOLY2       7		/* polysilicon 2 (electrode)  */
#define LSACT        8		/* P (or N) active            */
#define LDACT        9		/* N (or P) active            */
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
#define LTRANS      22		/* transistor                 */
#define LPOLYCAP    23		/* polysilicon capacitor      */
#define LSACTWELL   24		/* P active in well           */
#define LMET1P      25		/* pseudo metal 1             */
#define LMET2P      26		/* pseudo metal 2             */
#define LMET3P      27		/* pseudo metal 3             */
#define LMET4P      28		/* pseudo metal 4             */
#define LMET5P      29		/* pseudo metal 5             */
#define LMET6P      30		/* pseudo metal 6             */
#define LPOLY1P     31		/* pseudo polysilicon 1       */
#define LPOLY2P     32		/* pseudo polysilicon 2       */
#define LSACTP      33		/* pseudo P (or N) active     */
#define LDACTP      34		/* pseudo N (or P) active     */
#define LSELECTPP   35		/* pseudo P-type select       */
#define LSELECTNP   36		/* pseudo N-type select       */
#define LWELLPP     37		/* pseudo P-type well         */
#define LWELLNP     38		/* pseudo N-type well         */
#define LFRAME      39		/* pad frame boundary         */

static GRAPHICS mocmossub_m1_lay = {LAYERT1,COLORT1, SOLIDC, PATTERNED,
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
static GRAPHICS mocmossub_m2_lay = {LAYERT4,COLORT4, SOLIDC, PATTERNED,
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
static GRAPHICS mocmossub_m3_lay = {LAYERT5,COLORT5, SOLIDC, PATTERNED,
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
static GRAPHICS mocmossub_m4_lay = {LAYERO,LBLUE, PATTERNED, PATTERNED,
/* metal-4 layer */		{0x0808, /*     X       X    */
						0x1818,  /*    XX      XX    */
						0x2828,  /*   X X     X X    */
						0x4848,  /*  X  X    X  X    */
						0xFCFC,  /* XXXXXX  XXXXXXX  */
						0x0808,  /*     X       X    */
						0x0808,  /*     X       X    */
						0x0000,  /*                  */
						0x0808,  /*     X       X    */
						0x1818,  /*    XX      XX    */
						0x2828,  /*   X X     X X    */
						0x4848,  /*  X  X    X  X    */
						0xFCFC,  /* XXXXXX  XXXXXXX  */
						0x0808,  /*     X       X    */
						0x0808,  /*     X       X    */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS mocmossub_m5_lay = {LAYERO,LRED, PATTERNED, PATTERNED,
/* metal-5 layer */		{0xFCFC, /* XXXXXX  XXXXXX   */
						0x8080,  /* X       X        */
						0x8080,  /* X       X        */
						0xF8F8,  /* XXXXX   XXXXX    */
						0x0404,  /*      X       X   */
						0x0404,  /*      X       X   */
						0xF8F8,  /* XXXXX   XXXXX    */
						0x0000,  /*                  */
						0xFCFC,  /* XXXXXX  XXXXXX   */
						0x8080,  /* X       X        */
						0x8080,  /* X       X        */
						0xF8F8,  /* XXXXX   XXXXX    */
						0x0404,  /*      X       X   */
						0x0404,  /*      X       X   */
						0xF8F8,  /* XXXXX   XXXXX    */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS mocmossub_m6_lay = {LAYERO,CYAN, PATTERNED, PATTERNED,
/* metal-6 layer */		{0x1818, /*    XX      XX    */
						0x6060,  /*  XX      XX      */
						0x8080,  /* X       X        */
						0xF8F8,  /* XXXXX   XXXXX    */
						0x8484,  /* X    X  X    X   */
						0x8484,  /* X    X  X    X   */
						0x7878,  /*  XXXX    XXXX    */
						0x0000,  /*                  */
						0x1818,  /*    XX      XX    */
						0x6060,  /*  XX      XX      */
						0x8080,  /* X       X        */
						0xF8F8,  /* XXXXX   XXXXX    */
						0x8484,  /* X    X  X    X   */
						0x8484,  /* X    X  X    X   */
						0x7878,  /*  XXXX    XXXX    */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS mocmossub_p1_lay = {LAYERT2,COLORT2, SOLIDC, PATTERNED,
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
static GRAPHICS mocmossub_p2_lay = {LAYERO,ORANGE, PATTERNED, PATTERNED,
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
static GRAPHICS mocmossub_sa_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
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
static GRAPHICS mocmossub_da_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
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
static GRAPHICS mocmossub_ssp_lay = {LAYERO,YELLOW, PATTERNED, PATTERNED,
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
static GRAPHICS mocmossub_ssn_lay = {LAYERO,YELLOW, PATTERNED, PATTERNED,
/* N Select layer */	{0x0100, /*        X         */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0001,  /*                X */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0100,  /*        X         */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0001,  /*                X */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS mocmossub_wp_lay = {LAYERO,BROWN, PATTERNED, PATTERNED,
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
static GRAPHICS mocmossub_wn_lay = {LAYERO,BROWN, PATTERNED, PATTERNED,
/* N Well implant */	{0x0002, /*               X  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0200,  /*       X          */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0002,  /*               X  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0200,  /*       X          */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS mocmossub_pc_lay = {LAYERO,BLACK, SOLIDC, SOLIDC,
/* poly cut layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmossub_ac_lay = {LAYERO,BLACK, SOLIDC, SOLIDC,
/* active cut layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmossub_v1_lay = {LAYERO,LGRAY, SOLIDC, SOLIDC,
/* via1 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmossub_v2_lay = {LAYERO,LGRAY, SOLIDC, SOLIDC,
/* via2 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmossub_v3_lay = {LAYERO,LGRAY, SOLIDC, SOLIDC,
/* via3 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmossub_v4_lay = {LAYERO,LGRAY, SOLIDC, SOLIDC,
/* via4 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmossub_v5_lay = {LAYERO,LGRAY, SOLIDC, SOLIDC,
/* via5 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmossub_ovs_lay = {LAYERO,DGRAY, PATTERNED, PATTERNED,
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
static GRAPHICS mocmossub_tr_lay = {LAYERN,ALLOFF, SOLIDC, SOLIDC,
/* transistor layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmossub_cp_lay = {LAYERO,BLACK, SOLIDC, SOLIDC,
/* poly cap layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmossub_saw_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
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
static GRAPHICS mocmossub_pm1_lay ={LAYERT1,COLORT1, SOLIDC, PATTERNED,
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
static GRAPHICS mocmossub_pm2_lay = {LAYERT4,COLORT4, SOLIDC, PATTERNED,
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
static GRAPHICS mocmossub_pm3_lay = {LAYERT5,COLORT5, SOLIDC, PATTERNED,
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
static GRAPHICS mocmossub_pm4_lay = {LAYERO,LBLUE, PATTERNED, PATTERNED,
/* pseudo metal-4 */	{0x0808, /*     X       X    */
						0x1818,  /*    XX      XX    */
						0x2828,  /*   X X     X X    */
						0x4848,  /*  X  X    X  X    */
						0xFCFC,  /* XXXXXX  XXXXXXX  */
						0x0808,  /*     X       X    */
						0x0808,  /*     X       X    */
						0x0000,  /*                  */
						0x0808,  /*     X       X    */
						0x1818,  /*    XX      XX    */
						0x2828,  /*   X X     X X    */
						0x4848,  /*  X  X    X  X    */
						0xFCFC,  /* XXXXXX  XXXXXXX  */
						0x0808,  /*     X       X    */
						0x0808,  /*     X       X    */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS mocmossub_pm5_lay = {LAYERO,LRED, PATTERNED, PATTERNED,
/* pseudo metal-5 */	{0xFCFC, /* XXXXXX  XXXXXX   */
						0x8080,  /* X       X        */
						0x8080,  /* X       X        */
						0xF8F8,  /* XXXXX   XXXXX    */
						0x0404,  /*      X       X   */
						0x0404,  /*      X       X   */
						0xF8F8,  /* XXXXX   XXXXX    */
						0x0000,  /*                  */
						0xFCFC,  /* XXXXXX  XXXXXX   */
						0x8080,  /* X       X        */
						0x8080,  /* X       X        */
						0xF8F8,  /* XXXXX   XXXXX    */
						0x0404,  /*      X       X   */
						0x0404,  /*      X       X   */
						0xF8F8,  /* XXXXX   XXXXX    */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS mocmossub_pm6_lay = {LAYERO,CYAN, PATTERNED, PATTERNED,
/* pseudo metal-6 */	{0x1818, /*    XX      XX    */
						0x6060,  /*  XX      XX      */
						0x8080,  /* X       X        */
						0xF8F8,  /* XXXXX   XXXXX    */
						0x8484,  /* X    X  X    X   */
						0x8484,  /* X    X  X    X   */
						0x7878,  /*  XXXX    XXXX    */
						0x0000,  /*                  */
						0x1818,  /*    XX      XX    */
						0x6060,  /*  XX      XX      */
						0x8080,  /* X       X        */
						0xF8F8,  /* XXXXX   XXXXX    */
						0x8484,  /* X    X  X    X   */
						0x8484,  /* X    X  X    X   */
						0x7878,  /*  XXXX    XXXX    */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS mocmossub_pp1_lay = {LAYERT2,COLORT2, SOLIDC, PATTERNED,
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
static GRAPHICS mocmossub_pp2_lay = {LAYERO,ORANGE, PATTERNED, PATTERNED,
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
static GRAPHICS mocmossub_psa_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
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
static GRAPHICS mocmossub_pda_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
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
static GRAPHICS mocmossub_pssp_lay = {LAYERO,YELLOW,PATTERNED, PATTERNED,
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
static GRAPHICS mocmossub_pssn_lay = {LAYERO,YELLOW,PATTERNED, PATTERNED,
/* pseudo N Select */	{0x0100, /*        X         */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0001,  /*                X */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0100,  /*        X         */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0001,  /*                X */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS mocmossub_pwp_lay = {LAYERO,BROWN, PATTERNED, PATTERNED,
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
static GRAPHICS mocmossub_pwn_lay = {LAYERO,BROWN, PATTERNED, PATTERNED,
/* pseudo N Well */		{0x0002, /*               X  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0200,  /*       X          */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0002,  /*               X  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0200,  /*       X          */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS mocmossub_pf_lay = {LAYERO, RED, SOLIDC, PATTERNED,
/* pad frame */		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};

/* these tables must be updated together */
GRAPHICS *mocmossub_layers[MAXLAYERS+1] = {
	&mocmossub_m1_lay, &mocmossub_m2_lay, &mocmossub_m3_lay,	/* metal 1/2/3 */
	&mocmossub_m4_lay, &mocmossub_m5_lay, &mocmossub_m6_lay,	/* metal 4/5/6 */
	&mocmossub_p1_lay, &mocmossub_p2_lay,						/* poly 1/2 */
	&mocmossub_sa_lay, &mocmossub_da_lay,						/* P/N active */
	&mocmossub_ssp_lay, &mocmossub_ssn_lay,						/* P/N select */
	&mocmossub_wp_lay, &mocmossub_wn_lay,						/* P/N well */
	&mocmossub_pc_lay, &mocmossub_ac_lay,						/* poly/act cut */
	&mocmossub_v1_lay, &mocmossub_v2_lay, &mocmossub_v3_lay,	/* via 1/2/3 */
	&mocmossub_v4_lay, &mocmossub_v5_lay,						/* via 4/5 */
	&mocmossub_ovs_lay,											/* overglass */
	&mocmossub_tr_lay,											/* transistor */
	&mocmossub_cp_lay,											/* poly cap */
	&mocmossub_saw_lay,											/* P active well */
	&mocmossub_pm1_lay, &mocmossub_pm2_lay,						/* pseudo metal 1/2 */
	&mocmossub_pm3_lay, &mocmossub_pm4_lay,						/* pseudo metal 3/4 */
	&mocmossub_pm5_lay, &mocmossub_pm6_lay,						/* pseudo metal 5/6 */
	&mocmossub_pp1_lay, &mocmossub_pp2_lay,						/* pseudo poly 1/2 */
	&mocmossub_psa_lay, &mocmossub_pda_lay,						/* pseudo P/N active */
	&mocmossub_pssp_lay, &mocmossub_pssn_lay,					/* pseudo P/N select */
	&mocmossub_pwp_lay, &mocmossub_pwn_lay,						/* pseudo P/N well */
	&mocmossub_pf_lay, NOGRAPHICS};								/* pad frame */
static CHAR *mocmossub_layer_names[MAXLAYERS] = {
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
	x_("Transistor"),											/* transistor */
	x_("Poly-Cap"),												/* poly cap */
	x_("P-Active-Well"),										/* P active well */
	x_("Pseudo-Metal-1"), x_("Pseudo-Metal-2"),					/* pseudo metal 1/2 */
	x_("Pseudo-Metal-3"), x_("Pseudo-Metal-4"),					/* pseudo metal 3/4 */
	x_("Pseudo-Metal-5"), x_("Pseudo-Metal-6"),					/* pseudo metal 5/6 */
	x_("Pseudo-Polysilicon"), x_("Pseudo-Electrode"),			/* pseudo poly 1/2 */
	x_("Pseudo-P-Active"), x_("Pseudo-N-Active"),				/* pseudo P/N active */
	x_("Pseudo-P-Select"), x_("Pseudo-N-Select"),				/* pseudo P/N select */
	x_("Pseudo-P-Well"), x_("Pseudo-N-Well"),					/* pseudo P/N well */
	x_("Pad-Frame")};											/* pad frame */
static INTBIG mocmossub_layer_function[MAXLAYERS] = {
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
	LFTRANSISTOR|LFPSEUDO,										/* transistor */
	LFCAP,														/* poly cap */
	LFDIFF|LFPTYPE|LFTRANS3,									/* P active well */
	LFMETAL1|LFPSEUDO|LFTRANS1,LFMETAL2|LFPSEUDO|LFTRANS4,		/* pseudo metal 1/2 */
	LFMETAL3|LFPSEUDO|LFTRANS5, LFMETAL4|LFPSEUDO,				/* pseudo metal 3/4 */
	LFMETAL5|LFPSEUDO, LFMETAL6|LFPSEUDO,						/* pseudo metal 5/6 */
	LFPOLY1|LFPSEUDO|LFTRANS2, LFPOLY2|LFPSEUDO,				/* pseudo poly 1/2 */
	LFDIFF|LFPTYPE|LFPSEUDO|LFTRANS3,							/* pseudo P/N active */
		LFDIFF|LFNTYPE|LFPSEUDO|LFTRANS3,
	LFIMPLANT|LFPTYPE|LFPSEUDO, LFIMPLANT|LFNTYPE|LFPSEUDO,		/* pseudo P/N select */
	LFWELL|LFPTYPE|LFPSEUDO, LFWELL|LFNTYPE|LFPSEUDO,			/* pseudo P/N well */
	LFART};														/* pad frame */
static CHAR *mocmossub_cif_layers[MAXLAYERS] = {
	x_("CMF"), x_("CMS"), x_("CMT"), x_("CMQ"), x_("CMP"), x_("CM6"),	/* metal 1/2/3/4/5/6 */
	x_("CPG"), x_("CEL"),										/* poly 1/2 */
	x_("CAA"), x_("CAA"),										/* P/N active */
	x_("CSP"), x_("CSN"),										/* P/N select */
	x_("CWP"), x_("CWN"),										/* P/N well */
	x_("CCG"), x_("CCG"),										/* poly/act cut */
	x_("CVA"), x_("CVS"), x_("CVT"), x_("CVQ"), x_("CV5"),		/* via 1/2/3/4/5 */
	x_("COG"),													/* overglass */
	x_(""),														/* transistor */
	x_("CPC"),													/* poly cap */
	x_("CAA"),													/* P active well */
	x_(""), x_(""), x_(""), x_(""), x_(""), x_(""),				/* pseudo metal 1/2/3/4/5/6 */
	x_(""), x_(""),												/* pseudo poly 1/2 */
	x_(""), x_(""),												/* pseudo P/N active */
	x_("CSP"), x_("CSN"),										/* pseudo P/N select */
	x_("CWP"), x_("CWN"),										/* pseudo P/N well */
	x_("CX")};													/* pad frame */
static CHAR *mocmossub_gds_layers[MAXLAYERS] = {
	x_("49"), x_("51"), x_("62"), x_("31"), x_("33"), x_("38"),	/* metal 1/2/3/4/5/6 */
	x_("46"), x_("56"),											/* poly 1/2 */
	x_("43"), x_("43"),											/* P/N active */
	x_("44"), x_("45"),											/* P/N select */
	x_("41"), x_("42"),											/* P/N well */
	x_("25"), x_("25"),											/* poly/act cut */
	x_("50"), x_("61"), x_("30"), x_("32"), x_("39"),			/* via 1/2/3/4/5 */
	x_("52"),													/* overglass */
	x_(""),														/* transistor */
	x_("28"),													/* poly cap */
	x_("43"),													/* P active well */
	x_(""), x_(""), x_(""), x_(""), x_(""), x_(""),				/* pseudo metal 1/2/3/4/5/6 */
	x_(""), x_(""),												/* pseudo poly 1/2 */
	x_(""), x_(""),												/* pseudo P/N active */
	x_(""), x_(""),												/* pseudo P/N select */
	x_(""), x_(""),												/* pseudo P/N well */
	x_("19")};													/* pad frame */
static INTBIG mocmossub_minimum_width[MAXLAYERS] = {
	K3, K3, K3, K3, K3, K3,										/* metal 1/2/3/4/5/6 */
	K2, K3,														/* poly 1/2 */
	K3, K3,														/* P/N active */
	K2, K2,														/* P/N select */
	K12, K12,													/* P/N well */
	K2, K2,														/* poly/act cut */
	K2, K2, K2, K2, K2,											/* via 1/2/3/4/5 */
	XX,															/* overglass */
	XX,															/* transistor */
	XX,															/* poly cap */
	XX,															/* P active well */
	XX, XX, XX, XX, XX, XX,										/* pseudo metal 1/2/3/4/5/6 */
	XX, XX,														/* pseudo poly 1/2 */
	XX, XX,														/* pseudo P/N active */
	K2, K2,														/* pseudo P/N select */
	K12, K12,													/* pseudo P/N well */
	XX};														/* pad frame */
static CHAR *mocmossub_skill_layers[MAXLAYERS] = {
	x_("metal1"), x_("metal2"), x_("metal3"),					/* metal 1/2/3 */
	x_("metal4"), x_("metal5"), x_("metal6"),					/* metal 4/5/6 */
	x_("poly"), x_(""),											/* poly 1/2 */
	x_("aa"), x_("aa"),											/* P/N active */
	x_("pplus"), x_("nplus"),									/* P/N select */
	x_("pwell"), x_("nwell"),									/* P/N well */
	x_("pcont"), x_("acont"),									/* poly/act cut */
	x_("via"), x_("via2"), x_("via3"), x_("via4"), x_("via5"),	/* via 1/2/3/4/5 */
	x_("glasscut"),												/* overglass */
	x_(""),														/* transistor */
	x_(""),														/* poly cap */
	x_("aa"),													/* P active well */
	x_(""), x_(""), x_(""), x_(""), x_(""), x_(""),				/* pseudo metal 1/2/3/4/5/6 */
	x_(""), x_(""),												/* pseudo poly 1/2 */
	x_(""), x_(""),												/* pseudo P/N active */
	x_("pplus"), x_("nplus"),									/* pseudo P/N select */
	x_("pwell"), x_("nwell"),									/* pseudo P/N well */
	x_("")};													/* pad frame */
static INTBIG mocmossub_3dthick_layers[MAXLAYERS] = {
	0, 0, 0, 0, 0, 0,											/* metal 1/2/3/4/5/6 */
	0, 0,														/* poly 1/2 */
	0, 0,														/* P/N active */
	0, 0,														/* P/N select */
	0, 0,														/* P/N well */
	2, 4,														/* poly/act cut */
	2, 2, 2, 2, 2,												/* via 1/2/3/4/5 */
	0,															/* overglass */
	0,															/* transistor */
	0,															/* poly cap */
	0,															/* P active well */
	0, 0, 0, 0, 0, 0,											/* pseudo metal 1/2/3/4/5/6 */
	0, 0,														/* pseudo poly 1/2 */
	0, 0,														/* pseudo P/N active */
	0, 0,														/* pseudo P/N select */
	0, 0,														/* pseudo P/N well */
	0};															/* pad frame */
static INTBIG mocmossub_3dheight_layers[MAXLAYERS] = {
	17, 19, 21, 23, 25, 27,										/* metal 1/2/3/4/5/6 */
	15, 16,														/* poly 1/2 */
	13, 13,														/* P/N active */
	12, 12,														/* P/N select */
	11, 11,														/* P/N well */
	16, 15,														/* poly/act cut */
	18, 20, 22, 24, 26,											/* via 1/2/3/4/5 */
	30,															/* overglass */
	31,															/* transistor */
	28,															/* poly cap */
	29,															/* P active well */
	17, 19, 21, 23, 25, 27,										/* pseudo metal 1/2/3/4/5/6 */
	12, 13,														/* pseudo poly 1/2 */
	11, 11,														/* pseudo P/N active */
	2, 2,														/* pseudo P/N select */
	0, 0,														/* pseudo P/N well */
	33};														/* pad frame */
/* there are no available letters */
static CHAR *mocmossub_layer_letters[MAXLAYERS] = {
	x_("m"), x_("h"), x_("r"), x_("q"), x_("a"), x_("c"),		/* metal 1/2/3/4/5/6 */
	x_("p"), x_("l"),											/* poly 1/2 */
	x_("s"), x_("d"),											/* P/N active */
	x_("e"), x_("f"),											/* P/N select */
	x_("w"), x_("n"),											/* P/N well */
	x_("j"), x_("k"),											/* poly/act cut */
	x_("v"), x_("u"), x_("z"), x_("i"), x_("y"),				/* via 1/2/3/4/5 */
	x_("o"),													/* overglass */
	x_("t"),													/* transistor */
	x_("g"),													/* poly cap */
	x_("x"),													/* P active well */
	x_("M"), x_("H"), x_("R"), x_("Q"), x_("A"), x_("C"),		/* pseudo metal 1/2/3/4/5/6 */
	x_("P"), x_("L"),											/* pseudo poly 1/2 */
	x_("S"), x_("D"),											/* pseudo P/N active */
	x_("E"), x_("F"),											/* pseudo P/N select */
	x_("W"), x_("N"),											/* pseudo P/N well */
	x_("b")};													/* pad frame */
/* The low 5 bits map Metal-1, Poly-1, Active, Metal-2, and Metal-3 */
static TECH_COLORMAP mocmossub_colmap[32] =
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

#define X	XX
#define A	K1
#define B	K2
#define C	K3
#define D	K4
#define E	K5
#define F	K6
#define R	K18

/* layers that can connect to other layers when electrically disconnected */
static INTBIG mocmossub_unconnectedtable[] = {
/*          M M M M M M P P P N S S W W P A V V V V V P T P P M M M M M M P P P N S S W W P */
/*          e e e e e e o o A A e e e e o c i i i i i a r C a e e e e e e o o A A e e e e a */
/*          t t t t t t l l c c l l l l l t a a a a a s a a c t t t t t t l l c c l l l l d */
/*          1 2 3 4 5 6 y y t t P N l l y C 1 2 3 4 5 s n p t 1 2 3 4 5 6 1 2 t t P N P N F */
/*                      1 2         P N C               s   W P P P P P P P P P P P P P P r */
/* Met1  */ C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Met2  */   C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Met3  */     C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Met4  */       C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Met5  */         C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Met6  */           C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Poly1 */             C,X,A,A,X,X,X,X,X,B,X,X,X,X,X,X,X,X,A,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Poly2 */               C,A,A,X,X,X,X,C,X,X,X,X,X,X,X,X,X,A,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* PAct  */                 C,C,X,X,X,X,B,X,X,X,X,X,X,X,X,X,C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* NAct  */                   C,X,X,X,X,B,X,X,X,X,X,X,X,X,X,C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* SelP  */                     B,0,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* SelN  */                       B,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* WellP */                         R,0,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* WellN */                           R,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* PolyC */                             C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* ActC  */                               C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Via1  */                                 C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Via2  */                                   C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Via3  */                                     D,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Via4  */                                       C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Via5  */                                         C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Pass  */                                           X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Trans */                                             X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* PCap  */                                               X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* PactW */                                                 C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Met1P */                                                   X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Met2P */                                                     X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Met3P */                                                       X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Met4P */                                                         X,X,X,X,X,X,X,X,X,X,X,X,
/* Met5P */                                                           X,X,X,X,X,X,X,X,X,X,X,
/* Met6P */                                                             X,X,X,X,X,X,X,X,X,X,
/* Poly1P */                                                              X,X,X,X,X,X,X,X,X,
/* Poly2P */                                                                X,X,X,X,X,X,X,X,
/* PActP */                                                                   X,X,X,X,X,X,X,
/* NActP */                                                                     X,X,X,X,X,X,
/* SelPP */                                                                       X,X,X,X,X,
/* SelNP */                                                                         X,X,X,X,
/* WelPP */                                                                           X,X,X,
/* WelNP */                                                                             X,X,
/* PadFr */                                                                               X,
};

/* layers that can connect to other layers when electrically connected */
static INTBIG mocmossub_connectedtable[] = {
/*          M M M M M M P P P N S S W W P A V V V V V P T P P M M M M M M P P P N S S W W P */
/*          e e e e e e o o A A e e e e o c i i i i i a r C a e e e e e e o o A A e e e e a */
/*          t t t t t t l l c c l l l l l t a a a a a s a a c t t t t t t l l c c l l l l d */
/*          1 2 3 4 5 6 y y t t P N l l y C 1 2 3 4 5 s n p t 1 2 3 4 5 6 1 2 t t P N P N F */
/*                      1 2         P N C               s   W P P P P P P P P P P P P P P r */
/* Met1  */ C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Met2  */   C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Met3  */     C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Met4  */       C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Met5  */         C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Met6  */           C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Poly1 */             C,X,A,A,X,X,X,X,X,X,X,X,X,X,X,X,X,X,A,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Poly2 */               C,A,A,X,X,X,X,X,X,X,X,X,X,X,X,X,X,A,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* PAct  */                 C,C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* NAct  */                   C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* SelP  */                     X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* SelN  */                       X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* WellP */                         F,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* WellN */                           F,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* PolyC */                             C,C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* ActC  */                               C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Via1  */                                 C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Via2  */                                   C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Via3  */                                     D,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Via4  */                                       C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Via5  */                                         C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Pass  */                                           X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Trans */                                             X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* PCap  */                                               X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* PactW */                                                 C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Met1P */                                                   X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Met2P */                                                     X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Met3P */                                                       X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Met4P */                                                         X,X,X,X,X,X,X,X,X,X,X,X,
/* Met5P */                                                           X,X,X,X,X,X,X,X,X,X,X,
/* Met6P */                                                             X,X,X,X,X,X,X,X,X,X,
/* Poly1P */                                                              X,X,X,X,X,X,X,X,X,
/* Poly2P */                                                                X,X,X,X,X,X,X,X,
/* PActP */                                                                   X,X,X,X,X,X,X,
/* NActP */                                                                     X,X,X,X,X,X,
/* SelPP */                                                                       X,X,X,X,X,
/* SelNP */                                                                         X,X,X,X,
/* WelPP */                                                                           X,X,X,
/* WelNP */                                                                             X,X,
/* PadFr */                                                                               X,
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
#define ASACT          8	/* P-active (or N)           */
#define ADACT          9	/* N-active (or P)           */
#define AACT          10	/* General active            */

/* metal 1 arc */
static TECH_ARCLAY mocmossub_al_m1[] = {{LMETAL1,0,FILLED }};
static TECH_ARCS mocmossub_a_m1 = {
	x_("Metal-1"),K3,AMETAL1,NOARCPROTO,							/* name */
	1,mocmossub_al_m1,												/* layers */
	(APMETAL1<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* metal 2 arc */
static TECH_ARCLAY mocmossub_al_m2[] = {{LMETAL2,0,FILLED }};
static TECH_ARCS mocmossub_a_m2 = {
	x_("Metal-2"),K3,AMETAL2,NOARCPROTO,							/* name */
	1,mocmossub_al_m2,												/* layers */
	(APMETAL2<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* metal 3 arc */
static TECH_ARCLAY mocmossub_al_m3[] = {{LMETAL3,0,FILLED }};
static TECH_ARCS mocmossub_a_m3 = {
	x_("Metal-3"),K3,AMETAL3,NOARCPROTO,							/* name */
	1,mocmossub_al_m3,												/* layers */
	(APMETAL3<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* metal 4 arc */
static TECH_ARCLAY mocmossub_al_m4[] = {{LMETAL4,0,FILLED }};
static TECH_ARCS mocmossub_a_m4 = {
	x_("Metal-4"),K3,AMETAL4,NOARCPROTO,							/* name */
	1,mocmossub_al_m4,												/* layers */
	(APMETAL4<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* metal 5 arc */
static TECH_ARCLAY mocmossub_al_m5[] = {{LMETAL5,0,FILLED }};
static TECH_ARCS mocmossub_a_m5 = {
	x_("Metal-5"),K3,AMETAL5,NOARCPROTO,							/* name */
	1,mocmossub_al_m5,												/* layers */
	(APMETAL5<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* metal 6 arc */
static TECH_ARCLAY mocmossub_al_m6[] = {{LMETAL6,0,FILLED }};
static TECH_ARCS mocmossub_a_m6 = {
	x_("Metal-6"),K3,AMETAL6,NOARCPROTO,							/* name */
	1,mocmossub_al_m6,												/* layers */
	(APMETAL6<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* polysilicon 1 arc */
static TECH_ARCLAY mocmossub_al_p1[] = {{LPOLY1,0,FILLED }};
static TECH_ARCS mocmossub_a_po1 = {
	x_("Polysilicon-1"),K2,APOLY1,NOARCPROTO,						/* name */
	1,mocmossub_al_p1,												/* layers */
	(APPOLY1<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* polysilicon 2 arc */
static TECH_ARCLAY mocmossub_al_p2[] = {{LPOLY2,0,FILLED }};
static TECH_ARCS mocmossub_a_po2 = {
	x_("Polysilicon-2"),K3,APOLY2,NOARCPROTO,						/* name */
	1,mocmossub_al_p2,												/* layers */
	(APPOLY2<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* P-active arc */
static TECH_ARCLAY mocmossub_al_pa[] = {{LSACT,K12,FILLED}, {LWELLN,0,FILLED},
	{LSELECTP,K8,FILLED}};
static TECH_ARCS mocmossub_a_pa = {
	x_("P-Active"),K15,ASACT,NOARCPROTO,							/* name */
	3,mocmossub_al_pa,												/* layers */
	(APDIFFP<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* N-active arc */
static TECH_ARCLAY mocmossub_al_na[] = {{LDACT,K12,FILLED}, {LWELLP,0,FILLED},
	{LSELECTN,K8,FILLED}};
static TECH_ARCS mocmossub_a_na = {
	x_("N-Active"),K15,ADACT,NOARCPROTO,							/* name */
	3,mocmossub_al_na,												/* layers */
	(APDIFFN<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* General active arc */
static TECH_ARCLAY mocmossub_al_a[] = {{LDACT,0,FILLED}, {LSACT,0,FILLED}};
static TECH_ARCS mocmossub_a_a = {
	x_("Active"),K3,AACT,NOARCPROTO,								/* name */
	2,mocmossub_al_a,												/* layers */
	(APDIFF<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

TECH_ARCS *mocmossub_arcprotos[ARCPROTOCOUNT+1] = {
	&mocmossub_a_m1, &mocmossub_a_m2, &mocmossub_a_m3,
	&mocmossub_a_m4, &mocmossub_a_m5, &mocmossub_a_m6,
	&mocmossub_a_po1, &mocmossub_a_po2,
	&mocmossub_a_pa, &mocmossub_a_na,
	&mocmossub_a_a, ((TECH_ARCS *)-1)};

static INTBIG mocmossub_arc_widoff[ARCPROTOCOUNT] = {0,0,0,0,0,0, 0,0, K12,K12, 0};

/******************** PORTINST CONNECTIONS ********************/

static INTBIG mocmossub_pc_m1[]   = {-1, AMETAL1, ALLGEN, -1};
static INTBIG mocmossub_pc_m1a[]  = {-1, AMETAL1, AACT, ALLGEN, -1};
static INTBIG mocmossub_pc_m2[]   = {-1, AMETAL2, ALLGEN, -1};
static INTBIG mocmossub_pc_m3[]   = {-1, AMETAL3, ALLGEN, -1};
static INTBIG mocmossub_pc_m4[]   = {-1, AMETAL4, ALLGEN, -1};
static INTBIG mocmossub_pc_m5[]   = {-1, AMETAL5, ALLGEN, -1};
static INTBIG mocmossub_pc_m6[]   = {-1, AMETAL6, ALLGEN, -1};
static INTBIG mocmossub_pc_p1[]   = {-1, APOLY1, ALLGEN, -1};
static INTBIG mocmossub_pc_p2[]   = {-1, APOLY2, ALLGEN, -1};
static INTBIG mocmossub_pc_pa[]   = {-1, ASACT, ALLGEN, -1};
static INTBIG mocmossub_pc_a[]    = {-1, AACT, ASACT, ADACT, ALLGEN,-1};
static INTBIG mocmossub_pc_na[]   = {-1, ADACT, ALLGEN, -1};
static INTBIG mocmossub_pc_pam1[] = {-1, ASACT, AMETAL1, ALLGEN, -1};
static INTBIG mocmossub_pc_nam1[] = {-1, ADACT, AMETAL1, ALLGEN, -1};
static INTBIG mocmossub_pc_pm1[]  = {-1, APOLY1, AMETAL1, ALLGEN, -1};
static INTBIG mocmossub_pc_pm2[]  = {-1, APOLY2, AMETAL1, ALLGEN, -1};
static INTBIG mocmossub_pc_pm12[] = {-1, APOLY1, APOLY2, AMETAL1, ALLGEN, -1};
static INTBIG mocmossub_pc_m1m2[] = {-1, AMETAL1, AMETAL2, ALLGEN, -1};
static INTBIG mocmossub_pc_m2m3[] = {-1, AMETAL2, AMETAL3, ALLGEN, -1};
static INTBIG mocmossub_pc_m3m4[] = {-1, AMETAL3, AMETAL4, ALLGEN, -1};
static INTBIG mocmossub_pc_m4m5[] = {-1, AMETAL4, AMETAL5, ALLGEN, -1};
static INTBIG mocmossub_pc_m5m6[] = {-1, AMETAL5, AMETAL6, ALLGEN, -1};
static INTBIG mocmossub_pc_null[] = {-1, ALLGEN, -1};

/******************** NODES ********************/

#define NODEPROTOCOUNT 50
#define NMETAL1P        1	/* metal-1 pin */
#define NMETAL2P        2	/* metal-2 pin */
#define NMETAL3P        3	/* metal-3 pin */
#define NMETAL4P        4	/* metal-4 pin */
#define NMETAL5P        5	/* metal-5 pin */
#define NMETAL6P        6	/* metal-6 pin */
#define NPOLY1P         7	/* polysilicon-1 pin */
#define NPOLY2P         8	/* polysilicon-2 pin */
#define NSACTP          9	/* P-active (or N) pin */
#define NDACTP         10	/* N-active (or P) pin */
#define NACTP          11	/* General active pin */
#define NMETSACTC      12	/* metal-1-P-active (or N) contact */
#define NMETDACTC      13	/* metal-1-N-active (or P) contact */
#define NMETPOLY1C     14	/* metal-1-polysilicon-1 contact */
#define NMETPOLY2C     15	/* metal-1-polysilicon-2 contact */
#define NMETPOLY12C    16	/* metal-1-polysilicon-1-2 capacitor/contact */
#define NSTRANS        17	/* P-transistor (or N) */
#define NDTRANS        18	/* N-transistor (or P) */
#define NVIA1          19	/* metal-1-metal-2 contact */
#define NVIA2          20	/* metal-2-metal-3 contact */
#define NVIA3          21	/* metal-3-metal-4 contact */
#define NVIA4          22	/* metal-4-metal-5 contact */
#define NVIA5          23	/* metal-5-metal-6 contact */
#define NWBUT          24	/* metal-1-Well contact */
#define NSBUT          25	/* metal-1-Substrate contact */
#define NMETAL1N       26	/* metal-1 node */
#define NMETAL2N       27	/* metal-2 node */
#define NMETAL3N       28	/* metal-3 node */
#define NMETAL4N       29	/* metal-4 node */
#define NMETAL5N       30	/* metal-5 node */
#define NMETAL6N       31	/* metal-6 node */
#define NPOLY1N        32	/* polysilicon-1 node */
#define NPOLY2N        33	/* polysilicon-2 node */
#define NACTIVEN       34	/* active(P) node */
#define NDACTIVEN      35	/* N-active node */
#define NSELECTPN      36	/* P-select node */
#define NSELECTNN      37	/* N-select node */
#define NPCUTN         38	/* poly cut node */
#define NACUTN         39	/* active cut node */
#define NVIA1N         40	/* via-1 node */
#define NVIA2N         41	/* via-2 node */
#define NVIA3N         42	/* via-3 node */
#define NVIA4N         43	/* via-4 node */
#define NVIA5N         44	/* via-5 node */
#define NWELLPN        45	/* P-well node */
#define NWELLNN        46	/* N-well node */
#define NPASSN         47	/* passivation node */
#define NPADFRN        48	/* pad frame node */
#define NPOLYCAPN      49	/* polysilicon-capacitor node */
#define NPACTWELLN     50	/* P-active-Well node */

/* for geometry */
static INTBIG mocmossub_cutbox[8]  = {LEFTIN1,  BOTIN1,   LEFTIN3,   BOTIN3};/*adjust*/
static INTBIG mocmossub_fullbox[8] = {LEFTEDGE, BOTEDGE,  RIGHTEDGE, TOPEDGE};
static INTBIG mocmossub_in0hbox[8] = {LEFTIN0H, BOTIN0H,  RIGHTIN0H, TOPIN0H};
static INTBIG mocmossub_in1box[8]  = {LEFTIN1,  BOTIN1,   RIGHTIN1,  TOPIN1};
static INTBIG mocmossub_in2box[8]  = {LEFTIN2,  BOTIN2,   RIGHTIN2,  TOPIN2};
static INTBIG mocmossub_in4box[8]  = {LEFTIN4,  BOTIN4,   RIGHTIN4,  TOPIN4};
static INTBIG mocmossub_in6box[8]  = {LEFTIN6,  BOTIN6,   RIGHTIN6,  TOPIN6};
static INTBIG mocmossub_in6hbox[8] = {LEFTIN6H, BOTIN6H,  RIGHTIN6H, TOPIN6H};
static INTBIG mocmossub_min5box[16]= {LEFTIN5,  BOTIN5,   RIGHTIN5,  TOPIN5,
									 CENTERL2, CENTERD2, CENTERR2,  CENTERU2};
static INTBIG mocmossub_trnpbox[8] = {LEFTIN3H, BOTIN9,   RIGHTIN3H, TOPIN9};
static INTBIG mocmossub_trd1box[8] = {LEFTIN6,  BOTIN9,   RIGHTIN6,  TOPIN9};
static INTBIG mocmossub_trna2box[8]= {LEFTIN6,  TOPIN9,   RIGHTIN6,  TOPIN6};
static INTBIG mocmossub_trna3box[8]= {LEFTIN6,  BOTIN6,   RIGHTIN6,  BOTIN9};

/* metal-1-pin */
static TECH_PORTS mocmossub_pm1_p[] = {				/* ports */
	{mocmossub_pc_m1, x_("metal-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmossub_pm1_l[] = {			/* layers */
	{LMET1P, 0, 4, CROSSED, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_pm1 = {
	x_("Metal-1-Pin"),NMETAL1P,NONODEPROTO,			/* name */
	K3,K3,											/* size */
	1,mocmossub_pm1_p,								/* ports */
	1,mocmossub_pm1_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* metal-2-pin */
static TECH_PORTS mocmossub_pm2_p[] = {				/* ports */
	{mocmossub_pc_m2, x_("metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmossub_pm2_l[] = {			/* layers */
	{LMET2P, 0, 4, CROSSED, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_pm2 = {
	x_("Metal-2-Pin"),NMETAL2P,NONODEPROTO,			/* name */
	K3,K3,											/* size */
	1,mocmossub_pm2_p,								/* ports */
	1,mocmossub_pm2_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* metal-3-pin */
static TECH_PORTS mocmossub_pm3_p[] = {				/* ports */
	{mocmossub_pc_m3, x_("metal-3"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmossub_pm3_l[] = {			/* layers */
	{LMET3P, 0, 4, CROSSED, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_pm3 = {
	x_("Metal-3-Pin"),NMETAL3P,NONODEPROTO,			/* name */
	K3,K3,											/* size */
	1,mocmossub_pm3_p,								/* ports */
	1,mocmossub_pm3_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* metal-4-pin */
static TECH_PORTS mocmossub_pm4_p[] = {				/* ports */
	{mocmossub_pc_m4, x_("metal-4"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmossub_pm4_l[] = {			/* layers */
	{LMET4P, 0, 4, CROSSED, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_pm4 = {
	x_("Metal-4-Pin"),NMETAL4P,NONODEPROTO,			/* name */
	K3,K3,											/* size */
	1,mocmossub_pm4_p,								/* ports */
	1,mocmossub_pm4_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* metal-5-pin */
static TECH_PORTS mocmossub_pm5_p[] = {				/* ports */
	{mocmossub_pc_m5, x_("metal-5"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmossub_pm5_l[] = {			/* layers */
	{LMET5P, 0, 4, CROSSED, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_pm5 = {
	x_("Metal-5-Pin"),NMETAL5P,NONODEPROTO,			/* name */
	K3,K3,											/* size */
	1,mocmossub_pm5_p,								/* ports */
	1,mocmossub_pm5_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK|NNOTUSED,	/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* metal-6-pin */
static TECH_PORTS mocmossub_pm6_p[] = {				/* ports */
	{mocmossub_pc_m6, x_("metal-6"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmossub_pm6_l[] = {			/* layers */
	{LMET6P, 0, 4, CROSSED, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_pm6 = {
	x_("Metal-6-Pin"),NMETAL6P,NONODEPROTO,			/* name */
	K3,K3,											/* size */
	1,mocmossub_pm6_p,								/* ports */
	1,mocmossub_pm6_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK|NNOTUSED,	/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* polysilicon-1-pin */
static TECH_PORTS mocmossub_pp1_p[] = {				/* ports */
	{mocmossub_pc_p1, x_("polysilicon-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON mocmossub_pp1_l[] = {			/* layers */
	{LPOLY1P, 0, 4, CROSSED, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_pp1 = {
	x_("Polysilicon-1-Pin"),NPOLY1P,NONODEPROTO,	/* name */
	K2,K2,											/* size */
	1,mocmossub_pp1_p,								/* ports */
	1,mocmossub_pp1_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* polysilicon-2-pin */
static TECH_PORTS mocmossub_pp2_p[] = {				/* ports */
	{mocmossub_pc_p2, x_("polysilicon-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmossub_pp2_l[] = {			/* layers */
	{LPOLY2P, 0, 4, CROSSED, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_pp2 = {
	x_("Polysilicon-2-Pin"),NPOLY2P,NONODEPROTO,	/* name */
	K3,K3,											/* size */
	1,mocmossub_pp2_p,								/* ports */
	1,mocmossub_pp2_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* P-active-pin (or N) */
static TECH_PORTS mocmossub_psa_p[] = {				/* ports */
	{mocmossub_pc_pa, x_("p-active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN7H, BOTIN7H, RIGHTIN7H, TOPIN7H}};
static TECH_POLYGON mocmossub_psa_l[] = {			/* layers */
	{LSACTP,    0, 4, CROSSED, BOX, mocmossub_in6box},
	{LWELLNP,  -1, 4, CROSSED, BOX, mocmossub_fullbox},
	{LSELECTPP,-1, 4, CROSSED, BOX, mocmossub_in4box}};
static TECH_NODES mocmossub_psa = {
	x_("P-Active-Pin"),NSACTP,NONODEPROTO,			/* name */
	K15,K15,										/* size */
	1,mocmossub_psa_p,								/* ports */
	3,mocmossub_psa_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* N-active-pin (or P) */
static TECH_PORTS mocmossub_pda_p[] = {				/* ports */
	{mocmossub_pc_na, x_("n-active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN7H, BOTIN7H, RIGHTIN7H, TOPIN7H}};
static TECH_POLYGON mocmossub_pda_l[] = {			/* layers */
	{LDACTP,    0, 4, CROSSED, BOX, mocmossub_in6box},
	{LWELLPP,  -1, 4, CROSSED, BOX, mocmossub_fullbox},
	{LSELECTNP,-1, 4, CROSSED, BOX, mocmossub_in4box}};
static TECH_NODES mocmossub_pda = {
	x_("N-Active-Pin"),NDACTP,NONODEPROTO,			/* name */
	K15,K15,										/* size */
	1,mocmossub_pda_p,								/* ports */
	3,mocmossub_pda_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* General active-pin */
static TECH_PORTS mocmossub_pa_p[] = {				/* ports */
	{mocmossub_pc_a, x_("active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}}	;
static TECH_POLYGON mocmossub_pa_l[] = {			/* layers */
	{LDACTP, 0, 4, CROSSED, BOX, mocmossub_fullbox},
	{LSACTP, 0, 4, CROSSED, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_pa = {
	x_("Active-Pin"),NACTP,NONODEPROTO,				/* name */
	K3,K3,											/* size */
	1,mocmossub_pa_p,								/* ports */
	2,mocmossub_pa_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* metal-1-P-active-contact (or N) */
static TECH_PORTS mocmossub_mpa_p[] = {				/* ports */
	{mocmossub_pc_pam1, x_("metal-1-p-act"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN8, BOTIN8, RIGHTIN8, TOPIN8}};
static TECH_POLYGON mocmossub_mpa_l[] = {			/* layers */
	{LMETAL1,  0, 4, FILLEDRECT, BOX, mocmossub_in6hbox},
	{LSACT,    0, 4, FILLEDRECT, BOX, mocmossub_in6box},
	{LWELLN,  -1, 4, FILLEDRECT, BOX, mocmossub_fullbox},
	{LSELECTP,-1, 4, FILLEDRECT, BOX, mocmossub_in4box},
	{LACTCUT,  0, 4, FILLEDRECT, BOX, mocmossub_cutbox}};
static TECH_NODES mocmossub_mpa = {
	x_("Metal-1-P-Active-Con"),NMETSACTC,NONODEPROTO,	/* name */
	K17,K17,										/* size */
	1,mocmossub_mpa_p,								/* ports */
	5,mocmossub_mpa_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,H1,K4,0,0,0,0};					/* characteristics */

/* metal-1-N-active-contact (or P) */
static TECH_PORTS mocmossub_mna_p[] = {				/* ports */
	{mocmossub_pc_nam1, x_("metal-1-n-act"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN8, BOTIN8, RIGHTIN8, TOPIN8}};
static TECH_POLYGON mocmossub_mna_l[] = {			/* layers */
	{LMETAL1,   0, 4, FILLEDRECT, BOX, mocmossub_in6hbox},
	{LDACT,     0, 4, FILLEDRECT, BOX, mocmossub_in6box},
	{LWELLP,   -1, 4, FILLEDRECT, BOX, mocmossub_fullbox},
	{LSELECTN, -1, 4, FILLEDRECT, BOX, mocmossub_in4box},
	{LACTCUT,   0, 4, FILLEDRECT, BOX, mocmossub_cutbox}};
static TECH_NODES mocmossub_mna = {
	x_("Metal-1-N-Active-Con"),NMETDACTC,NONODEPROTO,	/* name */
	K17,K17,										/* size */
	1,mocmossub_mna_p,								/* ports */
	5,mocmossub_mna_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,H1,K4,0,0,0,0};					/* characteristics */

/* metal-1-polysilicon-1-contact */
static TECH_PORTS mocmossub_mp1_p[] = {				/* ports */
	{mocmossub_pc_pm1, x_("metal-1-polysilicon-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN2, BOTIN2, RIGHTIN2, TOPIN2}};
static TECH_POLYGON mocmossub_mp1_l[] = {			/* layers */
	{LMETAL1,  0, 4, FILLEDRECT, BOX, mocmossub_in0hbox},
	{LPOLY1,   0, 4, FILLEDRECT, BOX, mocmossub_fullbox},
	{LPOLYCUT, 0, 4, FILLEDRECT, BOX, mocmossub_cutbox}};
static TECH_NODES mocmossub_mp1 = {
	x_("Metal-1-Polysilicon-1-Con"),NMETPOLY1C,NONODEPROTO,/* name */
	K5,K5,											/* size */
	1,mocmossub_mp1_p,								/* ports */
	3,mocmossub_mp1_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,H1,K4,0,0,0,0};					/* characteristics */

/* metal-1-polysilicon-2-contact */
static TECH_PORTS mocmossub_mp2_p[] = {				/* ports */
	{mocmossub_pc_pm2, x_("metal-1-polysilicon-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmossub_mp2_l[] = {			/* layers */
	{LMETAL1,  0, 4, FILLEDRECT, BOX, mocmossub_fullbox},
	{LPOLY2,   0, 4, FILLEDRECT, BOX, mocmossub_in0hbox},
	{LPOLYCUT, 0, 4, FILLEDRECT, BOX, mocmossub_cutbox}};
static TECH_NODES mocmossub_mp2 = {
	x_("Metal-1-Polysilicon-2-Con"),NMETPOLY2C,NONODEPROTO,/* name */
	K4,K4,											/* size */
	1,mocmossub_mp2_p,								/* ports */
	3,mocmossub_mp2_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,K1,K4,0,0,0,0};					/* characteristics */

/* metal-1-polysilicon-1-2-contact */
static TECH_PORTS mocmossub_mp12_p[] = {			/* ports */
	{mocmossub_pc_pm12, x_("metal-1-polysilicon-1-2"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON mocmossub_mp12_l[] = {			/* layers */
	{LPOLY1,   0, 4, FILLEDRECT, BOX, mocmossub_fullbox},
	{LPOLY2,   0, 4, FILLEDRECT, BOX, mocmossub_in2box},
	{LPOLYCUT, 0, 4, FILLEDRECT, BOX, mocmossub_cutbox}};
static TECH_NODES mocmossub_mp12 = {
	x_("Metal-1-Polysilicon-1-2-Con"),NMETPOLY12C,NONODEPROTO,/* name */
	K7,K7,											/* size */
	1,mocmossub_mp12_p,								/* ports */
	3,mocmossub_mp12_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,H2,K4,0,0,0,0};					/* characteristics */

/* P-transistor (or N) */
static TECH_PORTS mocmossub_tpa_p[] = {				/* ports */
	{mocmossub_pc_p1, x_("p-trans-poly-left"),  NOPORTPROTO, (180<<PORTANGLESH)|
		(90<<PORTARANGESH),                LEFTIN4,  BOTIN10, LEFTIN4,   TOPIN10},
	{mocmossub_pc_pa, x_("p-trans-diff-top"),   NOPORTPROTO, (90<<PORTANGLESH)|
		(90<<PORTARANGESH)|(1<<PORTNETSH), LEFTIN7H,  TOPIN6H, RIGHTIN7H,  TOPIN6},
	{mocmossub_pc_p1, x_("p-trans-poly-right"), NOPORTPROTO, (0<<PORTANGLESH)|
		(90<<PORTARANGESH),                RIGHTIN4, BOTIN10, RIGHTIN4,  TOPIN10},
	{mocmossub_pc_pa, x_("p-trans-diff-bottom"),NOPORTPROTO, (270<<PORTANGLESH)|
		(90<<PORTARANGESH)|(2<<PORTNETSH), LEFTIN7H,  BOTIN6, RIGHTIN7H,  BOTIN6H}};
static TECH_SERPENT mocmossub_tpa_l[] = {			/* graphical layers */
	{{LSACT,    1, 4, FILLEDRECT, BOX,    mocmossub_in6box},  K4, K4,  0,  0},
	{{LPOLY1,   0, 4, FILLEDRECT, BOX,    mocmossub_trnpbox}, K1, K1, H2, H2},
	{{LWELLN,  -1, 4, FILLEDRECT, BOX,    mocmossub_fullbox}, K10,K10,K6, K6},
	{{LSELECTP,-1, 4, FILLEDRECT, BOX,    mocmossub_in4box},  K6, K6, K2, K2},
	{{LTRANS,  -1, 4, FILLEDRECT, BOX,    mocmossub_trd1box}, K1, K1,  0,  0}};
static TECH_SERPENT mocmossub_tpaE_l[] = {			/* electric layers */
	{{LSACT,    1, 4, FILLEDRECT, BOX,    mocmossub_trna2box}, K4,-K1,  0,  0},
	{{LSACT,    3, 4, FILLEDRECT, BOX,    mocmossub_trna3box},-K1, K4,  0,  0},
	{{LPOLY1,   0, 4, FILLEDRECT, BOX,    mocmossub_trnpbox},  K1, K1, H2, H2},
	{{LWELLN,  -1, 4, FILLEDRECT, BOX,    mocmossub_fullbox},  K10,K10,K6, K6},
	{{LSELECTP,-1, 4, FILLEDRECT, BOX,    mocmossub_in4box},   K6, K6, K2, K2},
	{{LTRANS,  -1, 4, FILLEDRECT, BOX,    mocmossub_trd1box},  K1, K1,  0,  0}};
static TECH_NODES mocmossub_tpa = {
	x_("P-Transistor"),NSTRANS,NONODEPROTO,			/* name */
	K15,K20,										/* size */
	4,mocmossub_tpa_p,								/* ports */
	5,(TECH_POLYGON *)0,							/* layers */
	NODESHRINK|(NPTRAPMOS<<NFUNCTIONSH)|HOLDSTRACE,	/* userbits */
	SERPTRANS,6,H1,H2,K2,K1,K2,mocmossub_tpa_l,mocmossub_tpaE_l};/* characteristics */

/* N-transistor (or P) */
static TECH_PORTS mocmossub_tna_p[] = {				/* ports */
	{mocmossub_pc_p1, x_("n-trans-poly-left"),  NOPORTPROTO, (180<<PORTANGLESH)|
		(90<<PORTARANGESH),                 LEFTIN4,  BOTIN10, LEFTIN4,   TOPIN10},
	{mocmossub_pc_na, x_("n-trans-diff-top"),   NOPORTPROTO, (90<<PORTANGLESH)|
		(90<<PORTARANGESH)|(1<<PORTNETSH),  LEFTIN7H,  TOPIN6H, RIGHTIN7H,  TOPIN6},
	{mocmossub_pc_p1, x_("n-trans-poly-right"), NOPORTPROTO, (0<<PORTANGLESH)|
		(90<<PORTARANGESH),                 RIGHTIN4, BOTIN10, RIGHTIN4,  TOPIN10},
	{mocmossub_pc_na, x_("n-trans-diff-bottom"),NOPORTPROTO, (270<<PORTANGLESH)|
		(90<<PORTARANGESH)|(2<<PORTNETSH), LEFTIN7H,  BOTIN6, RIGHTIN7H,  BOTIN6H}};
static TECH_SERPENT mocmossub_tna_l[] = {			/* graphical layers */
	{{LDACT,     1, 4, FILLEDRECT, BOX,    mocmossub_in6box},  K4, K4,  0,  0},
	{{LPOLY1,    0, 4, FILLEDRECT, BOX,    mocmossub_trnpbox}, K1, K1, H2, H2},
	{{LWELLP,   -1, 4, FILLEDRECT, BOX,    mocmossub_fullbox}, K10,K10,K6, K6},
	{{LSELECTN, -1, 4, FILLEDRECT, BOX,    mocmossub_in4box},  K6, K6, K2, K2},
	{{LTRANS,   -1, 4, FILLEDRECT, BOX,    mocmossub_trd1box}, K1, K1,  0,  0}};
static TECH_SERPENT mocmossub_tnaE_l[] = {			/* electric layers */
	{{LDACT,     1, 4, FILLEDRECT, BOX,    mocmossub_trna2box}, K4,-K1,  0,  0},
	{{LDACT,     3, 4, FILLEDRECT, BOX,    mocmossub_trna3box},-K1, K4,  0,  0},
	{{LPOLY1,    0, 4, FILLEDRECT, BOX,    mocmossub_trnpbox},  K1, K1, H2, H2},
	{{LWELLP,   -1, 4, FILLEDRECT, BOX,    mocmossub_fullbox},  K10,K10,K6, K6},
	{{LSELECTN, -1, 4, FILLEDRECT, BOX,    mocmossub_in4box},   K6, K6, K2, K2},
	{{LTRANS,   -1, 4, FILLEDRECT, BOX,    mocmossub_trd1box},  K1, K1,  0,  0}};
static TECH_NODES mocmossub_tna = {
	x_("N-Transistor"),NDTRANS,NONODEPROTO,			/* name */
	K15,K20,										/* size */
	4,mocmossub_tna_p,								/* ports */
	5,(TECH_POLYGON *)0,							/* layers */
	NODESHRINK|(NPTRANMOS<<NFUNCTIONSH)|HOLDSTRACE,	/* userbits */
	SERPTRANS,6,H1,H2,K2,K1,K2,mocmossub_tna_l,mocmossub_tnaE_l};/* characteristics */

/* metal-1-metal-2-contact */
static TECH_PORTS mocmossub_m1m2_p[] = {			/* ports */
	{mocmossub_pc_m1m2, x_("metal-1-metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmossub_m1m2_l[] = {			/* layers */
	{LMETAL1, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox},
	{LMETAL2, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox},
	{LVIA1,   0, 4, FILLEDRECT, BOX, mocmossub_cutbox}};
static TECH_NODES mocmossub_m1m2 = {
	x_("Metal-1-Metal-2-Con"),NVIA1,NONODEPROTO,	/* name */
	K4,K4,											/* size */
	1,mocmossub_m1m2_p,								/* ports */
	3,mocmossub_m1m2_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,K1,K4,0,0,0,0};					/* characteristics */

/* metal-2-metal-3-contact */
static TECH_PORTS mocmossub_m2m3_p[] = {			/* ports */
	{mocmossub_pc_m2m3, x_("metal-2-metal-3"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN2H, BOTIN2H, RIGHTIN2H, TOPIN2H}};
static TECH_POLYGON mocmossub_m2m3_l[] = {			/* layers */
	{LMETAL2, 0, 4, FILLEDRECT, BOX, mocmossub_in1box},
	{LMETAL3, 0, 4, FILLEDRECT, BOX, mocmossub_in1box},
	{LVIA2,   0, 4, FILLEDRECT, BOX, mocmossub_cutbox}};
static TECH_NODES mocmossub_m2m3 = {
	x_("Metal-2-Metal-3-Con"),NVIA2,NONODEPROTO,	/* name */
	K6,K6,											/* size */
	1,mocmossub_m2m3_p,								/* ports */
	3,mocmossub_m2m3_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,K1,K4,0,0,0,0};					/* characteristics */

/* metal-3-metal-4-contact */
static TECH_PORTS mocmossub_m3m4_p[] = {			/* ports */
	{mocmossub_pc_m3m4, x_("metal-3-metal-4"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN2H, BOTIN2H, RIGHTIN2H, TOPIN2H}};
static TECH_POLYGON mocmossub_m3m4_l[] = {			/* layers */
	{LMETAL3, 0, 4, FILLEDRECT, BOX, mocmossub_in1box},
	{LMETAL4, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox},
	{LVIA3,   0, 4, FILLEDRECT, BOX, mocmossub_cutbox}};
static TECH_NODES mocmossub_m3m4 = {
	x_("Metal-3-Metal-4-Con"),NVIA3,NONODEPROTO,	/* name */
	K6,K6,											/* size */
	1,mocmossub_m3m4_p,								/* ports */
	3,mocmossub_m3m4_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,K2,K4,0,0,0,0};					/* characteristics */

/* metal-4-metal-5-contact */
static TECH_PORTS mocmossub_m4m5_p[] = {			/* ports */
	{mocmossub_pc_m4m5, x_("metal-4-metal-5"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN2H, BOTIN2H, RIGHTIN2H, TOPIN2H}};
static TECH_POLYGON mocmossub_m4m5_l[] = {			/* layers */
	{LMETAL4, 0, 4, FILLEDRECT, BOX, mocmossub_in1box},
	{LMETAL5, 0, 4, FILLEDRECT, BOX, mocmossub_in1box},
	{LVIA4,   0, 4, FILLEDRECT, BOX, mocmossub_cutbox}};
static TECH_NODES mocmossub_m4m5 = {
	x_("Metal-4-Metal-5-Con"),NVIA4,NONODEPROTO,	/* name */
	K6,K6,											/* size */
	1,mocmossub_m4m5_p,								/* ports */
	3,mocmossub_m4m5_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH)|NNOTUSED,				/* userbits */
	MULTICUT,K2,K2,K1,K4,0,0,0,0};					/* characteristics */

/* metal-5-metal-6-contact */
static TECH_PORTS mocmossub_m5m6_p[] = {			/* ports */
	{mocmossub_pc_m5m6, x_("metal-5-metal-6"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN2H, BOTIN2H, RIGHTIN2H, TOPIN2H}};
static TECH_POLYGON mocmossub_m5m6_l[] = {			/* layers */
	{LMETAL5, 0, 4, FILLEDRECT, BOX, mocmossub_in1box},
	{LMETAL6, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox},
	{LVIA5,   0, 4, FILLEDRECT, BOX, mocmossub_cutbox}};
static TECH_NODES mocmossub_m5m6 = {
	x_("Metal-5-Metal-6-Con"),NVIA5,NONODEPROTO,	/* name */
	K6,K6,											/* size */
	1,mocmossub_m5m6_p,								/* ports */
	3,mocmossub_m5m6_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH)|NNOTUSED,				/* userbits */
	MULTICUT,K2,K2,K1,K4,0,0,0,0};					/* characteristics */

/* Metal-1-Well Contact */
static TECH_PORTS mocmossub_psub_p[] = {			/* ports */
	{mocmossub_pc_m1a, x_("metal-1-well"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN6H, BOTIN6H, RIGHTIN6H, TOPIN6H}};
static TECH_POLYGON mocmossub_psub_l[] = {			/* layers */
	{LMETAL1,   0, 4, FILLEDRECT, MINBOX, mocmossub_min5box},
	{LSACTWELL, 0, 4, FILLEDRECT, BOX,    mocmossub_in4box},
	{LWELLP,   -1, 4, FILLEDRECT, BOX,    mocmossub_fullbox},
	{LSELECTP, -1, 4, FILLEDRECT, BOX,    mocmossub_in2box},
	{LACTCUT,   0, 4, FILLEDRECT, BOX,    mocmossub_cutbox}};
static TECH_NODES mocmossub_psub = {
	x_("Metal-1-Well-Con"),NWBUT,NONODEPROTO,		/* name */
	K14,K14,										/* size */
	1,mocmossub_psub_p,								/* ports */
	5,mocmossub_psub_l,								/* layers */
	(NPWELL<<NFUNCTIONSH),							/* userbits */
	MULTICUT,K2,K2,K2,K4,0,0,0,0};					/* characteristics */

/* Metal-1-Substrate Contact */
static TECH_PORTS mocmossub_nsub_p[] = {			/* ports */
	{mocmossub_pc_m1a, x_("metal-1-substrate"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN6H, BOTIN6H, RIGHTIN6H, TOPIN6H}};
static TECH_POLYGON mocmossub_nsub_l[] = {			/* layers */
	{LMETAL1,   0, 4, FILLEDRECT, MINBOX, mocmossub_min5box},
	{LDACT,     0, 4, FILLEDRECT, BOX,    mocmossub_in4box},
	{LWELLN,   -1, 4, FILLEDRECT, BOX,    mocmossub_fullbox},
	{LSELECTN, -1, 4, FILLEDRECT, BOX,    mocmossub_in2box},
	{LACTCUT,   0, 4, FILLEDRECT, BOX,    mocmossub_cutbox}};
static TECH_NODES mocmossub_nsub = {
	x_("Metal-1-Substrate-Con"),NSBUT,NONODEPROTO,	/* name */
	K14,K14,										/* size */
	1,mocmossub_nsub_p,								/* ports */
	5,mocmossub_nsub_l,								/* layers */
	(NPSUBSTRATE<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,K2,K4,0,0,0,0};					/* characteristics */

/* Metal-1-Node */
static TECH_PORTS mocmossub_m1_p[] = {				/* ports */
	{mocmossub_pc_m1, x_("metal-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmossub_m1_l[] = {			/* layers */
	{LMETAL1, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_m1 = {
	x_("Metal-1-Node"),NMETAL1N,NONODEPROTO,		/* name */
	K3,K3,											/* size */
	1,mocmossub_m1_p,								/* ports */
	1,mocmossub_m1_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Metal-2-Node */
static TECH_PORTS mocmossub_m2_p[] = {				/* ports */
	{mocmossub_pc_m2, x_("metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmossub_m2_l[] = {			/* layers */
	{LMETAL2, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_m2 = {
	x_("Metal-2-Node"),NMETAL2N,NONODEPROTO,		/* name */
	K3,K3,											/* size */
	1,mocmossub_m2_p,								/* ports */
	1,mocmossub_m2_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Metal-3-Node */
static TECH_PORTS mocmossub_m3_p[] = {				/* ports */
	{mocmossub_pc_m3, x_("metal-3"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmossub_m3_l[] = {			/* layers */
	{LMETAL3, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_m3 = {
	x_("Metal-3-Node"),NMETAL3N,NONODEPROTO,		/* name */
	K3,K3,											/* size */
	1,mocmossub_m3_p,								/* ports */
	1,mocmossub_m3_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Metal-4-Node */
static TECH_PORTS mocmossub_m4_p[] = {				/* ports */
	{mocmossub_pc_m4, x_("metal-4"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmossub_m4_l[] = {			/* layers */
	{LMETAL4, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_m4 = {
	x_("Metal-4-Node"),NMETAL4N,NONODEPROTO,		/* name */
	K3,K3,											/* size */
	1,mocmossub_m4_p,								/* ports */
	1,mocmossub_m4_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Metal-5-Node */
static TECH_PORTS mocmossub_m5_p[] = {				/* ports */
	{mocmossub_pc_m5, x_("metal-5"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmossub_m5_l[] = {			/* layers */
	{LMETAL5, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_m5 = {
	x_("Metal-5-Node"),NMETAL5N,NONODEPROTO,		/* name */
	K3,K3,											/* size */
	1,mocmossub_m5_p,								/* ports */
	1,mocmossub_m5_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE|NNOTUSED,		/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Metal-6-Node */
static TECH_PORTS mocmossub_m6_p[] = {				/* ports */
	{mocmossub_pc_m6, x_("metal-6"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmossub_m6_l[] = {			/* layers */
	{LMETAL6, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_m6 = {
	x_("Metal-6-Node"),NMETAL6N,NONODEPROTO,		/* name */
	K3,K3,											/* size */
	1,mocmossub_m6_p,								/* ports */
	1,mocmossub_m6_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE|NNOTUSED,		/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Polysilicon-1-Node */
static TECH_PORTS mocmossub_p1_p[] = {				/* ports */
	{mocmossub_pc_p1, x_("polysilicon-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON mocmossub_p1_l[] = {			/* layers */
	{LPOLY1, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_p1 = {
	x_("Polysilicon-1-Node"),NPOLY1N,NONODEPROTO,	/* name */
	K2,K2,											/* size */
	1,mocmossub_p1_p,								/* ports */
	1,mocmossub_p1_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Polysilicon-2-Node */
static TECH_PORTS mocmossub_p2_p[] = {				/* ports */
	{mocmossub_pc_p2, x_("polysilicon-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmossub_p2_l[] = {			/* layers */
	{LPOLY2, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_p2 = {
	x_("Polysilicon-2-Node"),NPOLY2N,NONODEPROTO,	/* name */
	K3,K3,											/* size */
	1,mocmossub_p2_p,								/* ports */
	1,mocmossub_p2_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Active-Node */
static TECH_PORTS mocmossub_a_p[] = {				/* ports */
	{mocmossub_pc_a, x_("active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmossub_a_l[] = {				/* layers */
	{LSACT, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_a = {
	x_("Active-Node"),NACTIVEN,NONODEPROTO,			/* name */
	K3,K3,											/* size */
	1,mocmossub_a_p,								/* ports */
	1,mocmossub_a_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* N-Active-Node (or P) */
static TECH_PORTS mocmossub_da_p[] = {				/* ports */
	{mocmossub_pc_a, x_("active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmossub_da_l[] = {			/* layers */
	{LDACT, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_da = {
	x_("N-Active-Node"),NDACTIVEN,NONODEPROTO,		/* name */
	K3,K3,											/* size */
	1,mocmossub_da_p,								/* ports */
	1,mocmossub_da_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* P-Select-Node */
static TECH_PORTS mocmossub_sp_p[] = {				/* ports */
	{mocmossub_pc_null, x_("select"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmossub_sp_l[] = {			/* layers */
	{LSELECTP, -1, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_sp = {
	x_("P-Select-Node"),NSELECTPN,NONODEPROTO,		/* name */
	K4,K4,											/* size */
	1,mocmossub_sp_p,								/* ports */
	1,mocmossub_sp_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* N-Select-Node */
static TECH_PORTS mocmossub_sn_p[] = {				/* ports */
	{mocmossub_pc_null, x_("select"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmossub_sn_l[] = {			/* layers */
	{LSELECTN,  -1, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_sn = {
	x_("N-Select-Node"),NSELECTNN,NONODEPROTO,		/* name */
	K4,K4,											/* size */
	1,mocmossub_sn_p,								/* ports */
	1,mocmossub_sn_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* PolyCut-Node */
static TECH_PORTS mocmossub_gc_p[] = {				/* ports */
	{mocmossub_pc_null, x_("polycut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmossub_gc_l[] = {			/* layers */
	{LPOLYCUT, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_gc = {
	x_("Poly-Cut-Node"),NPCUTN,NONODEPROTO,			/* name */
	K2,K2,											/* size */
	1,mocmossub_gc_p,								/* ports */
	1,mocmossub_gc_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* ActiveCut-Node */
static TECH_PORTS mocmossub_ac_p[] = {				/* ports */
	{mocmossub_pc_null, x_("activecut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmossub_ac_l[] = {			/* layers */
	{LACTCUT, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_ac = {
	x_("Active-Cut-Node"),NACUTN,NONODEPROTO,		/* name */
	K2,K2,											/* size */
	1,mocmossub_ac_p,								/* ports */
	1,mocmossub_ac_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Via-1-Node */
static TECH_PORTS mocmossub_v1_p[] = {				/* ports */
	{mocmossub_pc_null, x_("via-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmossub_v1_l[] = {			/* layers */
	{LVIA1, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_v1 = {
	x_("Via-1-Node"),NVIA1N,NONODEPROTO,			/* name */
	K2,K2,											/* size */
	1,mocmossub_v1_p,								/* ports */
	1,mocmossub_v1_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Via-2-Node */
static TECH_PORTS mocmossub_v2_p[] = {				/* ports */
	{mocmossub_pc_null, x_("via-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmossub_v2_l[] = {			/* layers */
	{LVIA2, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_v2 = {
	x_("Via-2-Node"),NVIA2N,NONODEPROTO,			/* name */
	K2,K2,											/* size */
	1,mocmossub_v2_p,								/* ports */
	1,mocmossub_v2_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Via-3-Node */
static TECH_PORTS mocmossub_v3_p[] = {				/* ports */
	{mocmossub_pc_null, x_("via-3"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmossub_v3_l[] = {			/* layers */
	{LVIA3, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_v3 = {
	x_("Via-3-Node"),NVIA3N,NONODEPROTO,			/* name */
	K2,K2,											/* size */
	1,mocmossub_v3_p,								/* ports */
	1,mocmossub_v3_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Via-4-Node */
static TECH_PORTS mocmossub_v4_p[] = {				/* ports */
	{mocmossub_pc_null, x_("via-4"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmossub_v4_l[] = {			/* layers */
	{LVIA4, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_v4 = {
	x_("Via-4-Node"),NVIA4N,NONODEPROTO,			/* name */
	K2,K2,											/* size */
	1,mocmossub_v4_p,								/* ports */
	1,mocmossub_v4_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE|NNOTUSED,		/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Via-5-Node */
static TECH_PORTS mocmossub_v5_p[] = {				/* ports */
	{mocmossub_pc_null, x_("via-5"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmossub_v5_l[] = {			/* layers */
	{LVIA5, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_v5 = {
	x_("Via-5-Node"),NVIA5N,NONODEPROTO,			/* name */
	K2,K2,											/* size */
	1,mocmossub_v5_p,								/* ports */
	1,mocmossub_v5_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE|NNOTUSED,		/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* P-Well-Node */
static TECH_PORTS mocmossub_wp_p[] = {				/* ports */
	{mocmossub_pc_pa, x_("well"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3}};
static TECH_POLYGON mocmossub_wp_l[] = {			/* layers */
	{LWELLP, -1, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_wp = {
	x_("P-Well-Node"),NWELLPN,NONODEPROTO,			/* name */
	K12,K12,										/* size */
	1,mocmossub_wp_p,								/* ports */
	1,mocmossub_wp_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* N-Well-Node */
static TECH_PORTS mocmossub_wn_p[] = {				/* ports */
	{mocmossub_pc_pa, x_("well"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3}};
static TECH_POLYGON mocmossub_wn_l[] = {			/* layers */
	{LWELLN,  -1, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_wn = {
	x_("N-Well-Node"),NWELLNN,NONODEPROTO,			/* name */
	K12,K12,										/* size */
	1,mocmossub_wn_p,								/* ports */
	1,mocmossub_wn_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Passivation-node */
static TECH_PORTS mocmossub_o_p[] = {				/* ports */
	{mocmossub_pc_null, x_("passivation"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmossub_o_l[] = {				/* layers */
	{LPASS, 0, 4, FILLEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_o = {
	x_("Passivation-Node"),NPASSN,NONODEPROTO,		/* name */
	K8,K8,											/* size */
	1,mocmossub_o_p,								/* ports */
	1,mocmossub_o_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Pad-Frame-node */
static TECH_PORTS mocmossub_pf_p[] = {				/* ports */
	{mocmossub_pc_null, x_("pad-frame"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmossub_pf_l[] = {			/* layers */
	{LFRAME, 0, 4, CLOSEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_pf = {
	x_("Pad-Frame-Node"),NPADFRN,NONODEPROTO,		/* name */
	K8,K8,											/* size */
	1,mocmossub_pf_p,								/* ports */
	1,mocmossub_pf_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Polysilicon-Capacitor-node */
static TECH_PORTS mocmossub_pc_p[] = {				/* ports */
	{mocmossub_pc_null, x_("poly-cap"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmossub_pc_l[] = {			/* layers */
	{LPOLYCAP, 0, 4, CLOSEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_pc = {
	x_("Poly-Cap-Node"),NPOLYCAPN,NONODEPROTO,		/* name */
	K8,K8,											/* size */
	1,mocmossub_pc_p,								/* ports */
	1,mocmossub_pc_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* P-Active-Well-node */
static TECH_PORTS mocmossub_paw_p[] = {				/* ports */
	{mocmossub_pc_null, x_("p-active-well"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmossub_paw_l[] = {			/* layers */
	{LSACTWELL, 0, 4, CLOSEDRECT, BOX, mocmossub_fullbox}};
static TECH_NODES mocmossub_paw = {
	x_("P-Active-Well-Node"),NPACTWELLN,NONODEPROTO,/* name */
	K8,K8,											/* size */
	1,mocmossub_paw_p,								/* ports */
	1,mocmossub_paw_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

TECH_NODES *mocmossub_nodeprotos[NODEPROTOCOUNT+1] = {
	&mocmossub_pm1, &mocmossub_pm2, &mocmossub_pm3,	/* metal 1/2/3 pin */
	&mocmossub_pm4, &mocmossub_pm5, &mocmossub_pm6,	/* metal 4/5/6 pin */
	&mocmossub_pp1, &mocmossub_pp2,					/* polysilicon 1/2 pin */
	&mocmossub_psa, &mocmossub_pda,					/* P/N active pin */
	&mocmossub_pa,									/* active pin */
	&mocmossub_mpa, &mocmossub_mna,					/* metal 1 to P/N active contact */
	&mocmossub_mp1, &mocmossub_mp2,					/* metal 1 to polysilicon 1/2 contact */
	&mocmossub_mp12,								/* poly capacitor */
	&mocmossub_tpa, &mocmossub_tna,					/* P/N transistor */
	&mocmossub_m1m2, &mocmossub_m2m3, &mocmossub_m3m4,	/* via 1/2/3 */
	&mocmossub_m4m5, &mocmossub_m5m6,				/* via 4/5 */
	&mocmossub_psub, &mocmossub_nsub,				/* well / substrate contact */
	&mocmossub_m1, &mocmossub_m2, &mocmossub_m3,	/* metal 1/2/3 node */
	&mocmossub_m4, &mocmossub_m5, &mocmossub_m6,	/* metal 4/5/6 node */
	&mocmossub_p1, &mocmossub_p2,					/* polysilicon 1/2 node */
	&mocmossub_a, &mocmossub_da,					/* active N-Active node */
	&mocmossub_sp, &mocmossub_sn,					/* P/N select node */
	&mocmossub_gc, &mocmossub_ac,					/* poly cut / active cut */
	&mocmossub_v1, &mocmossub_v2, &mocmossub_v3,	/* via 1/2/3 node */
	&mocmossub_v4, &mocmossub_v5,					/* via 4/5 node */
	&mocmossub_wp, &mocmossub_wn,					/* P/N well node */
	&mocmossub_o,									/* overglass node */
	&mocmossub_pf,									/* pad frame node */
	&mocmossub_pc,									/* poly-cap node */
	&mocmossub_paw, ((TECH_NODES *)-1)};			/* p-active-well node */

/* this table must correspond with the above table */
static INTBIG mocmossub_node_widoff[NODEPROTOCOUNT*4] = {
	0,0,0,0, 0,0,0,0, 0,0,0,0,						/* metal 1/2/3 pin */
	0,0,0,0, 0,0,0,0, 0,0,0,0,						/* metal 4/5/6 pin */
	0,0,0,0, 0,0,0,0,								/* polysilicon 1/2 pin */
	K6,K6,K6,K6, K6,K6,K6,K6,						/* P/N active pin */
	0,0,0,0,										/* active pin */
	K6,K6,K6,K6, K6,K6,K6,K6,						/* metal 1 to P/N active contact */
	0,0,0,0, 0,0,0,0,								/* metal 1 to polysilicon 1/2 contact */
	0,0,0,0,										/* poly capacitor */
	K6,K6,K9,K9, K6,K6,K9,K9,						/* P/N transistor */
	0,0,0,0, K1,K1,K1,K1, 0,0,0,0,					/* via 1/2/3 */
	K1,K1,K1,K1, 0,0,0,0,							/* via 4/5 */
	K4,K4,K4,K4, K4,K4,K4,K4,						/* well / substrate contact */
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
	0,0,0,0};										/* p-active-well node */

/******************** SIMULATION VARIABLES ********************/

/* for SPICE simulation */
#define MOCMOSSUB_MIN_RESIST	50.0f	/* minimum resistance consider */
#define MOCMOSSUB_MIN_CAPAC	     0.04f	/* minimum capacitance consider */
static float mocmossub_sim_spice_resistance[MAXLAYERS] = {  /* per square micron */
	0.06f, 0.06f, 0.06f,				/* metal 1/2/3 */
	0.03f, 0.03f, 0.03f,				/* metal 4/5/6 */
	2.5f, 50.0f,						/* poly 1/2 */
	2.5f, 3.0f,							/* P/N active */
	0.0, 0.0,							/* P/N select */
	0.0, 0.0,							/* P/N well */
	2.2f, 2.5f,							/* poly/act cut */
	1.0f, 0.9f, 0.8f, 0.8f, 0.8f,		/* via 1/2/3/4/5 */
	0.0,								/* overglass */
	0.0,								/* transistor */
	0.0,								/* poly cap */
	0.0,								/* P active well */
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0,		/* pseudo metal 1/2/3/4/5/6 */
	0.0, 0.0,							/* pseudo poly 1/2 */
	0.0, 0.0,							/* pseudo P/N active */
	0.0, 0.0,							/* pseudo P/N select */
	0.0, 0.0,							/* pseudo P/N well */
	0.0};								/* pad frame */
static float mocmossub_sim_spice_capacitance[MAXLAYERS] = { /* per square micron */
	0.07f, 0.04f, 0.04f,				/* metal 1/2/3 */
	0.04f, 0.04f, 0.04f,				/* metal 4/5/6 */
	0.09f, 1.0f,						/* poly 1/2 */
	0.9f, 0.9f,							/* P/N active */
	0.0, 0.0,							/* P/N select */
	0.0, 0.0,							/* P/N well */
	0.0, 0.0,							/* poly/act cut */
	0.0, 0.0, 0.0, 0.0, 0.0,			/* via 1/2/3/4/5 */
	0.0,								/* overglass */
	0.0,								/* transistor */
	0.0,								/* poly cap */
	0.0,								/* P active well */
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0,		/* pseudo metal 1/2/3/4/5/6 */
	0.0, 0.0,							/* pseudo poly 1/2 */
	0.0, 0.0,							/* pseudo P/N active */
	0.0, 0.0,							/* pseudo P/N select */
	0.0, 0.0,							/* pseudo P/N well */
	0.0};								/* pad frame */
static CHAR *mocmossub_sim_spice_header_level1[] = {
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
static CHAR *mocmossub_sim_spice_header_level2[] = {
	x_("* MOSIS 3u CMOS PARAMS"),
	x_(".OPTIONS NOMOD DEFL=2UM DEFW=6UM DEFAD=100P DEFAS=100P"),
	x_("+LIMPTS=1000 ITL4=1000 ITL5=0 ABSTOL=500PA VNTOL=500UV"),
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

TECH_VARIABLES mocmossub_variables[] =
{
	/* set general information about the technology */
	{x_("TECH_layer_names"), (CHAR *)mocmossub_layer_names, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_layer_function"), (CHAR *)mocmossub_layer_function, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_arc_width_offset"), (CHAR *)mocmossub_arc_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|(ARCPROTOCOUNT<<VLENGTHSH)},
	{x_("TECH_node_width_offset"), (CHAR *)mocmossub_node_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|((NODEPROTOCOUNT*4)<<VLENGTHSH)},
	{x_("TECH_layer_3dthickness"), (CHAR *)mocmossub_3dthick_layers, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_layer_3dheight"), (CHAR *)mocmossub_3dheight_layers, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},

	/* set information for the USER tool */
	{x_("USER_color_map"), (CHAR *)mocmossub_colmap, 0.0,
		VCHAR|VDONTSAVE|VISARRAY|((sizeof mocmossub_colmap)<<VLENGTHSH)},
	{x_("USER_layer_letters"), (CHAR *)mocmossub_layer_letters, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},

	/* set information for the I/O tool */
	{x_("IO_cif_layer_names"), (CHAR *)mocmossub_cif_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("IO_gds_layer_numbers"), (CHAR *)mocmossub_gds_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("IO_skill_layer_names"), (CHAR *)mocmossub_skill_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},

	/* set information for the DRC tool */
	{x_("DRC_min_unconnected_distances"), (CHAR *)mocmossub_unconnectedtable, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|
			(((sizeof mocmossub_unconnectedtable)/SIZEOFINTBIG)<<VLENGTHSH)},
	{x_("DRC_min_connected_distances"), (CHAR *)mocmossub_connectedtable, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|
			(((sizeof mocmossub_connectedtable)/SIZEOFINTBIG)<<VLENGTHSH)},
	{x_("DRC_min_width"), (CHAR *)mocmossub_minimum_width, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},

	/* set information for the SIM tool (SPICE) */
	{x_("SIM_spice_min_resistance"), 0, MOCMOSSUB_MIN_RESIST, VFLOAT|VDONTSAVE},
	{x_("SIM_spice_min_capacitance"), 0, MOCMOSSUB_MIN_CAPAC, VFLOAT|VDONTSAVE},
	{x_("SIM_spice_resistance"), (CHAR *)mocmossub_sim_spice_resistance, 0.0,
		VFLOAT|VISARRAY|(MAXLAYERS<<VLENGTHSH)|VDONTSAVE},
	{x_("SIM_spice_capacitance"), (CHAR *)mocmossub_sim_spice_capacitance, 0.0,
		VFLOAT|VISARRAY|(MAXLAYERS<<VLENGTHSH)|VDONTSAVE},
	{x_("SIM_spice_header_level1"), (CHAR *)mocmossub_sim_spice_header_level1, 0.0,
		VSTRING|VDONTSAVE|VISARRAY},
	{x_("SIM_spice_header_level2"), (CHAR *)mocmossub_sim_spice_header_level2, 0.0,
		VSTRING|VDONTSAVE|VISARRAY},
	{NULL, NULL, 0.0, 0}
};

/******************** ROUTINES ********************/

BOOLEAN mocmossub_initprocess(TECHNOLOGY *tech, INTBIG pass)
{
	/* initialize the technology variable */
	if (pass == 0) mocmossub_tech = tech;
	mocmossub_state = MOCMOSSUB4METAL;
	nextchangequiet();
	setvalkey((INTBIG)mocmossub_tech, VTECHNOLOGY, el_techstate_key, mocmossub_state,
		VINTEGER|VDONTSAVE);
	return(FALSE);
}

INTBIG mocmossub_request(CHAR *command, va_list ap)
{
	if (namesame(command, x_("has-state")) == 0) return(1);
	if (namesame(command, x_("get-state")) == 0)
	{
		return(mocmossub_state);
	}
	if (namesame(command, x_("set-state")) == 0)
	{
		mocmossub_setstate(va_arg(ap, INTBIG));
		return(0);
	}
	if (namesame(command, x_("describe-state")) == 0)
	{
		return((INTBIG)mocmossub_describestate(va_arg(ap, INTBIG)));
	}
	return(0);
}

void mocmossub_setmode(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG l;
	REGISTER CHAR *pp;
	Q_UNUSED( count );

	l = estrlen(pp = par[0]);

	if (namesamen(pp, x_("2-metal-rules"), l) == 0)
	{
		mocmossub_setstate((mocmossub_state & ~MOCMOSSUBMETALS) | MOCMOSSUB2METAL);
		ttyputverbose(M_("MOSIS CMOS Submicron technology uses 2-metal rules"));
		return;
	}
	if (namesamen(pp, x_("3-metal-rules"), l) == 0)
	{
		mocmossub_setstate((mocmossub_state & ~MOCMOSSUBMETALS) | MOCMOSSUB3METAL);
		ttyputverbose(M_("MOSIS CMOS Submicron technology uses 3-metal rules"));
		return;
	}
	if (namesamen(pp, x_("4-metal-rules"), l) == 0)
	{
		mocmossub_setstate((mocmossub_state & ~MOCMOSSUBMETALS) | MOCMOSSUB4METAL);
		ttyputverbose(M_("MOSIS CMOS Submicron technology uses 4-metal rules"));
		return;
	}
	if (namesamen(pp, x_("5-metal-rules"), l) == 0)
	{
		mocmossub_setstate((mocmossub_state & ~MOCMOSSUBMETALS) | MOCMOSSUB5METAL);
		ttyputverbose(M_("MOSIS CMOS Submicron technology uses 5-metal rules"));
		return;
	}
	if (namesamen(pp, x_("6-metal-rules"), l) == 0)
	{
		mocmossub_setstate((mocmossub_state & ~MOCMOSSUBMETALS) | MOCMOSSUB6METAL);
		ttyputverbose(M_("MOSIS CMOS Submicron technology uses 6-metal rules"));
		return;
	}

	ttyputbadusage(x_("technology tell mocmossub"));
}

void mocmossub_setstate(INTBIG newstate)
{
	if (mocmossub_state == newstate) return;
	mocmossub_state = newstate;

	/* adjust metal rules according to the number of metal layers */
	switch (mocmossub_state&MOCMOSSUBMETALS)
	{
		case MOCMOSSUB2METAL:							/* 2-metal process: */
			/* metal-2 is 4 apart, 3 wide */
			mocmossub_setlayerrules(x_("Metal-2"), K4, K3);
			break;

		case MOCMOSSUB3METAL:							/* 3-metal process: */
			/* metal-2 is 3 apart, 3 wide */
			mocmossub_setlayerrules(x_("Metal-2"), K3, K3);

			/* metal-3 is 5 wide, 3 apart, overlaps vias by 2 */
			mocmossub_setlayerrules(x_("Metal-3"), K3, K5);
			mocmossub_setarcwidth(x_("Metal-3"), K5);
			mocmossub_setnodesize(x_("Metal-3-Pin"), K5, H2);
			mocmossub_setmetalonvia(x_("Metal-2-Metal-3-Con"), LMETAL3, mocmossub_fullbox, 0);
			mocmossub_m2m3.f3 = K2;
			break;

		case MOCMOSSUB4METAL:							/* 4-metal process: */
			/* metal-2 is 3 apart, 3 wide */
			mocmossub_setlayerrules(x_("Metal-2"), K3, K3);

			/* metal-3 is 3 wide, 3 apart, overlaps vias by 1 */
			mocmossub_setlayerrules(x_("Metal-3"), K3, K3);
			mocmossub_setarcwidth(x_("Metal-3"), K3);
			mocmossub_setnodesize(x_("Metal-3-Pin"), K3, H1);
			mocmossub_setmetalonvia(x_("Metal-2-Metal-3-Con"), LMETAL3, mocmossub_in1box, K1);
			mocmossub_m2m3.f3 = K1;

			/* metal-4 is 5 wide, 3 apart, overlaps vias by 2 */
			mocmossub_setmetalonvia(x_("Metal-3-Metal-4-Con"), LMETAL4, mocmossub_fullbox, 0);
			mocmossub_m3m4.f3 = K2;
			break;

		case MOCMOSSUB5METAL:							/* 5-metal process: */
			/* metal-2 is 3 apart, 3 wide */
			mocmossub_setlayerrules(x_("Metal-2"), K3, K3);

			/* metal-3 is 3 wide, 3 apart, overlaps vias by 1 */
			mocmossub_setlayerrules(x_("Metal-3"), K3, K3);
			mocmossub_setarcwidth(x_("Metal-3"), K3);
			mocmossub_setnodesize(x_("Metal-3-Pin"), K3, H1);
			mocmossub_setmetalonvia(x_("Metal-2-Metal-3-Con"), LMETAL3, mocmossub_in1box, K1);
			mocmossub_m2m3.f3 = K1;

			/* metal-4 is 3 wide, 3 apart, overlaps vias by 1 */
			mocmossub_setmetalonvia(x_("Metal-3-Metal-4-Con"), LMETAL4, mocmossub_in1box, K1);
			mocmossub_m3m4.f3 = K1;

			/* metal-5 is 4 apart, 4 wide */
			mocmossub_setlayerrules(x_("Metal-5"), K4, K4);
			mocmossub_setarcwidth(x_("Metal-5"), K4);
			mocmossub_setnodesize(x_("Metal-5-Pin"), K4, 0);
			break;

		case MOCMOSSUB6METAL:							/* 6-metal process: */
			/* metal-2 is 3 apart, 3 wide */
			mocmossub_setlayerrules(x_("Metal-2"), K3, K3);

			/* metal-3 is 3 wide, 3 apart, overlaps vias by 1 */
			mocmossub_setlayerrules(x_("Metal-3"), K3, K3);
			mocmossub_setarcwidth(x_("Metal-3"), K3);
			mocmossub_setnodesize(x_("Metal-3-Pin"), K3, H1);
			mocmossub_setmetalonvia(x_("Metal-2-Metal-3-Con"), LMETAL3, mocmossub_in1box, K1);
			mocmossub_m2m3.f3 = K1;

			/* metal-4 is 3 wide, 3 apart, overlaps vias by 1 */
			mocmossub_setmetalonvia(x_("Metal-3-Metal-4-Con"), LMETAL4, mocmossub_in1box, K1);
			mocmossub_m3m4.f3 = K1;

			/* metal-5 is 3 apart, 3 wide */
			mocmossub_setlayerrules(x_("Metal-5"), K3, K3);
			mocmossub_setarcwidth(x_("Metal-5"), K3);
			mocmossub_setnodesize(x_("Metal-5-Pin"), K3, 0);
			break;
	}

	/* disable Metal-3/4/5/6-Pin, Metal-2/3/4/5-Metal-3/4/5/6-Con, Metal-3/4/5/6-Node, Via-2/3/4/5-Node */
	mocmossub_pm3.creation->userbits |= NNOTUSED;
	mocmossub_pm4.creation->userbits |= NNOTUSED;
	mocmossub_pm5.creation->userbits |= NNOTUSED;
	mocmossub_pm6.creation->userbits |= NNOTUSED;
	mocmossub_m2m3.creation->userbits |= NNOTUSED;
	mocmossub_m3m4.creation->userbits |= NNOTUSED;
	mocmossub_m4m5.creation->userbits |= NNOTUSED;
	mocmossub_m5m6.creation->userbits |= NNOTUSED;
	mocmossub_m3.creation->userbits |= NNOTUSED;
	mocmossub_m4.creation->userbits |= NNOTUSED;
	mocmossub_m5.creation->userbits |= NNOTUSED;
	mocmossub_m6.creation->userbits |= NNOTUSED;
	mocmossub_v2.creation->userbits |= NNOTUSED;
	mocmossub_v3.creation->userbits |= NNOTUSED;
	mocmossub_v4.creation->userbits |= NNOTUSED;
	mocmossub_v5.creation->userbits |= NNOTUSED;

	/* enable the desired nodes */
	switch (mocmossub_state&MOCMOSSUBMETALS)
	{
		case MOCMOSSUB6METAL:
			mocmossub_pm6.creation->userbits &= ~NNOTUSED;
			mocmossub_m5m6.creation->userbits &= ~NNOTUSED;
			mocmossub_m6.creation->userbits &= ~NNOTUSED;
			mocmossub_v5.creation->userbits &= ~NNOTUSED;
			/* FALLTHROUGH */ 
		case MOCMOSSUB5METAL:
			mocmossub_pm5.creation->userbits &= ~NNOTUSED;
			mocmossub_m4m5.creation->userbits &= ~NNOTUSED;
			mocmossub_m5.creation->userbits &= ~NNOTUSED;
			mocmossub_v4.creation->userbits &= ~NNOTUSED;
			/* FALLTHROUGH */ 
		case MOCMOSSUB4METAL:
			mocmossub_pm4.creation->userbits &= ~NNOTUSED;
			mocmossub_m3m4.creation->userbits &= ~NNOTUSED;
			mocmossub_m4.creation->userbits &= ~NNOTUSED;
			mocmossub_v3.creation->userbits &= ~NNOTUSED;
			/* FALLTHROUGH */ 
		case MOCMOSSUB3METAL:
			mocmossub_pm3.creation->userbits &= ~NNOTUSED;
			mocmossub_m2m3.creation->userbits &= ~NNOTUSED;
			mocmossub_m3.creation->userbits &= ~NNOTUSED;
			mocmossub_v2.creation->userbits &= ~NNOTUSED;
			break;
	}

	/* now rewrite the description */
	(void)reallocstring(&mocmossub_tech->techdescript, mocmossub_describestate(mocmossub_state),
		mocmossub_tech->cluster);
}

CHAR *mocmossub_describestate(INTBIG state)
{
	REGISTER INTBIG nummetals;
	REGISTER CHAR *conversion;
	REGISTER void *infstr;

	switch (state&MOCMOSSUBMETALS)
	{
		case MOCMOSSUB2METAL: nummetals = 2;   break;
		case MOCMOSSUB3METAL: nummetals = 3;   break;
		case MOCMOSSUB4METAL: nummetals = 4;   break;
		case MOCMOSSUB5METAL: nummetals = 5;   break;
		case MOCMOSSUB6METAL: nummetals = 6;   break;
		default: nummetals = 2;
	}
	infstr = initinfstr();
	if ((state&MOCMOSSUBNOCONV) == 0) conversion = N_("converts to newer MOCMOS"); else
		conversion = N_("no conversion to newer MOCMOS");
	formatinfstr(infstr, _("Complementary MOS (old, from MOSIS, Submicron, 2-6 metals [now %ld], double poly, %s)"),
		nummetals, conversion);
	return(returninfstr(infstr));
}

/*
 * Routine to change the design rules for layer "layername" layers so that
 * the layers remain "spacing" apart and are at least "minwidth" wide.
 */
void mocmossub_setlayerrules(CHAR *layername, INTBIG spacing, INTBIG minwidth)
{
	REGISTER INTBIG layer, rindex;

	for(layer=0; layer<mocmossub_tech->layercount; layer++)
		if (namesame(mocmossub_layer_names[layer], layername) == 0) break;
	if (layer >= mocmossub_tech->layercount) return;

	rindex = (layer+1) * (layer/2) + (layer&1) * ((layer+1)/2);
	rindex = layer + mocmossub_tech->layercount * layer - rindex;
	changesquiet(TRUE);
	setindkey((INTBIG)mocmossub_tech, VTECHNOLOGY, dr_unconnected_distanceskey, rindex, spacing);
	setindkey((INTBIG)mocmossub_tech, VTECHNOLOGY, dr_connected_distanceskey, rindex, spacing);
	setindkey((INTBIG)mocmossub_tech, VTECHNOLOGY, dr_min_widthkey, layer, minwidth);
	changesquiet(FALSE);
}

/*
 * Routine to change the default width of arc "arcname" to "width".
 */
void mocmossub_setarcwidth(CHAR *arcname, INTBIG width)
{
	REGISTER ARCPROTO *ap;
	REGISTER INTBIG lambda;

	for(ap = mocmossub_tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
		if (namesame(ap->protoname, arcname) == 0) break;
	if (ap == NOARCPROTO) return;
	lambda = el_curlib->lambda[mocmossub_tech->techindex];
	ap->nominalwidth = width * lambda / WHOLE;
}

/*
 * Routine to change the default size of node "nodename" to "size" squared with a
 * port offset of "portoffset" (this is the distance from the edge in to the port).
 */
void mocmossub_setnodesize(CHAR *nodename, INTBIG size, INTBIG portoffset)
{
	REGISTER NODEPROTO *np;
	REGISTER INTBIG i, lambda;
	REGISTER TECH_NODES *nty;
	REGISTER TECH_PORTS *npp;

	np = NONODEPROTO;
	nty = 0;
	for(i=0; i<mocmossub_tech->nodeprotocount; i++)
	{
		nty = mocmossub_tech->nodeprotos[i];
		np = nty->creation;
		if (namesame(np->protoname, nodename) == 0) break;
	}
	if (np == NONODEPROTO) return;

	lambda = el_curlib->lambda[mocmossub_tech->techindex];
	np->lowx = -size * lambda / WHOLE / 2;
	np->highx = size * lambda / WHOLE / 2;
	np->lowy = -size * lambda / WHOLE / 2;
	np->highy = size * lambda / WHOLE / 2;
	npp = &nty->portlist[0];
	npp->lowxsum = (INTSML)portoffset;
	npp->lowysum = (INTSML)portoffset;
	npp->highxsum = (INTSML)(-portoffset);
	npp->highysum = (INTSML)(-portoffset);
}

/*
 * Routine to set the size of metal layer "layer" on via "nodename" so that it is described
 * with "boxdesc".  Also sets the node inset for this node to "nodeoffset".
 */
void mocmossub_setmetalonvia(CHAR *nodename, INTBIG layer, INTBIG *boxdesc, INTBIG nodeoffset)
{
	REGISTER NODEPROTO *np;
	REGISTER INTBIG i, j;
	REGISTER TECH_NODES *nty;
	REGISTER VARIABLE *var;

	np = NONODEPROTO;
	nty = 0;
	for(i=0; i<mocmossub_tech->nodeprotocount; i++)
	{
		nty = mocmossub_tech->nodeprotos[i];
		np = nty->creation;
		if (namesame(np->protoname, nodename) == 0) break;
	}
	if (np == NONODEPROTO) return;

	for(j=0; j<nty->layercount; j++)
		if (nty->layerlist[j].layernum == layer) break;
	if (j >= nty->layercount) return;
	nty->layerlist[j].points = boxdesc;

	var = getval((INTBIG)mocmossub_tech, VTECHNOLOGY, VFRACT|VISARRAY, x_("TECH_node_width_offset"));
	if (var != NOVARIABLE)
	{
		((INTBIG *)var->addr)[i*4] = nodeoffset;
		((INTBIG *)var->addr)[i*4+1] = nodeoffset;
		((INTBIG *)var->addr)[i*4+2] = nodeoffset;
		((INTBIG *)var->addr)[i*4+3] = nodeoffset;
	}
}
