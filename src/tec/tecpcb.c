/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tecpcb.c
 * Printed Circuit Board technology description
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

#include "config.h"
#include "global.h"
#include "egraphics.h"
#include "tech.h"
#include "efunction.h"

BOOLEAN pcb_initprocess(TECHNOLOGY*, INTBIG);

/******************** LAYERS ********************/

#define MAXLAYERS 23		/* total layers below */
#define LPC1       0		/* Signal 1 */
#define LPC2       1		/* Signal 2 */
#define LPC3       2		/* Signal 3 */
#define LPC4       3		/* Signal 4 */
#define LPC5       4		/* Signal 5 */
#define LPC6       5		/* Signal 6 */
#define LPC7       6		/* Signal 7 */
#define LPC8       7		/* Signal 8 */
#define LPN1       8		/* Power 1 */
#define LPN2       9		/* Power 2 */
#define LPN3      10		/* Power 3 */
#define LPN4      11		/* Power 4 */
#define LPN5      12		/* Power 5 */
#define LPN6      13		/* Power 6 */
#define LPN7      14		/* Power 7 */
#define LPN8      15		/* Power 8 */
#define LPSSC     16		/* Top Silk Screen */
#define LPSSS     17		/* Bottom Silk Screen */
#define LPSMC     18		/* Top Solder Mask */
#define LPSMS     19		/* Bottom Solder Mask */
#define LPD       20		/* Plated Drill Hole */
#define LPDNP     21		/* NonPlated Drill Hold */
#define LPF       22		/* Engineering Drawing */

static GRAPHICS pcb_c1_lay = {LAYERT1, COLORT1, SOLIDC, SOLIDC,
/* Signal 1 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_c2_lay = {LAYERT2, COLORT2, SOLIDC, SOLIDC,
/* Signal 2 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_c3_lay = {LAYERT3, COLORT3, SOLIDC, SOLIDC,
/* Signal 3 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_c4_lay = {LAYERT4, COLORT4, SOLIDC, SOLIDC,
/* Signal 4 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_c5_lay = {LAYERT5, COLORT5, SOLIDC, SOLIDC,
/* Signal 5 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_c6_lay = {LAYERO, YELLOW, SOLIDC, SOLIDC,
/* Signal 6 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_c7_lay = {LAYERO, ORANGE, SOLIDC, SOLIDC,
/* Signal 7 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_c8_lay = {LAYERO, CYAN, SOLIDC, SOLIDC,
/* Signal 8 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_n1_lay = {LAYERT1, COLORT1, SOLIDC, SOLIDC,
/* Power 1 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_n2_lay = {LAYERT2, COLORT2, SOLIDC, SOLIDC,
/* Power 2 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_n3_lay = {LAYERT3, COLORT3, SOLIDC, SOLIDC,
/* Power 3 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_n4_lay = {LAYERT4, COLORT4, SOLIDC, SOLIDC,
/* Power 4 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_n5_lay = {LAYERT5, COLORT5, SOLIDC, SOLIDC,
/* Power 5 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_n6_lay = {LAYERO, YELLOW, SOLIDC, SOLIDC,
/* Power 6 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_n7_lay = {LAYERO, ORANGE, SOLIDC, SOLIDC,
/* Power 7 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_n8_lay = {LAYERO, CYAN, SOLIDC, SOLIDC,
/* Power 8 layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_ssc_lay = {LAYERO, LGRAY, SOLIDC, SOLIDC,
/* TopSilk layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_sss_lay = {LAYERO, DGRAY, SOLIDC, SOLIDC,
/* BotSilk layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_smc_lay = {LAYERO, LGREEN, SOLIDC, SOLIDC,
/* TopSolder layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_sms_lay = {LAYERO, DGREEN, SOLIDC, SOLIDC,
/* BotSolder layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_d_lay = {LAYERO, DBLUE, SOLIDC, SOLIDC,
/* Drill layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_dnp_lay = {LAYERO, LBLUE, SOLIDC, SOLIDC,
/* NonPDrill layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS pcb_f_lay = {LAYERO, LRED, SOLIDC, SOLIDC,
/* Engineering layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};

/* these tables must be updated together */
GRAPHICS *pcb_layers[MAXLAYERS+1] = {&pcb_c1_lay, &pcb_c2_lay, &pcb_c3_lay,
	&pcb_c4_lay, &pcb_c5_lay, &pcb_c6_lay, &pcb_c7_lay, &pcb_c8_lay,
	&pcb_n1_lay, &pcb_n2_lay, &pcb_n3_lay, &pcb_n4_lay, &pcb_n5_lay,
	&pcb_n6_lay, &pcb_n7_lay, &pcb_n8_lay, &pcb_ssc_lay, &pcb_sss_lay,
	&pcb_smc_lay, &pcb_sms_lay, &pcb_d_lay, &pcb_dnp_lay, &pcb_f_lay, NOGRAPHICS};
static CHAR *pcb_layer_names[MAXLAYERS] = {x_("Signal1"), x_("Signal2"), x_("Signal3"),
	x_("Signal4"), x_("Signal5"), x_("Signal6"), x_("Signal7"), x_("Signal8"), x_("Power1"), x_("Power2"),
	x_("Power3"), x_("Power4"), x_("Power5"), x_("Power6"), x_("Power7"), x_("Power8"), x_("TopSilk"),
	x_("BottomSilk"), x_("TopSolder"), x_("BottomSolder"), x_("Drill"), x_("DrillNonPlated"),
	x_("Drawing")};
static CHAR *pcb_cif_layers[MAXLAYERS] = {x_("PC1"), x_("PC2"), x_("PC3"), x_("PC4"), x_("PC5"),
	x_("PC6"), x_("PC7"), x_("PC8"), x_("PN1"), x_("PN2"), x_("PN3"), x_("PN4"), x_("PN5"), x_("PN6"), x_("PN7"),
	x_("PN8"), x_("PSSC"), x_("PSSS"), x_("PSMC"), x_("PSMS"), x_("PD"), x_("PDNP"), x_("PF")};
static INTBIG pcb_layer_function[MAXLAYERS] = {LFMETAL1|LFTRANS1, LFMETAL2|LFTRANS2,
	LFMETAL3|LFTRANS3, LFMETAL4|LFTRANS4, LFMETAL5|LFTRANS5, LFMETAL6, LFMETAL7,
	LFMETAL8, LFMETAL1|LFTRANS1, LFMETAL2|LFTRANS2, LFMETAL3|LFTRANS3,
	LFMETAL4|LFTRANS4, LFMETAL5|LFTRANS5, LFMETAL6, LFMETAL7, LFMETAL8,
	LFART, LFART, LFMETAL1, LFMETAL8, LFCONTACT1|LFCONMETAL, LFART, LFART};
static CHAR *pcb_layer_letters[MAXLAYERS] = {x_("1"), x_("2"), x_("3"), x_("4"), x_("5"), x_("6"), x_("7"),
	x_("8"), x_("a"), x_("b"), x_("c"), x_("d"), x_("e"), x_("f"), x_("g"), x_("h"), x_("K"), x_("k"), x_("S"), x_("s"), x_("l"), x_("n"),
	x_("w")};

/* The low 5 bits map Signal1, Signal2, Signal3, Signal4, and Signal5 */
static TECH_COLORMAP pcb_colmap[32] =
{                  /*     signal5 signal4 signal3 signal2 signal1 */
	{200,200,200}, /* 0:                                          */
	{  0,  0,  0}, /* 1:                                  signal1 */
	{255,  0,  0}, /* 2:                          signal2         */
	{  0,  0,  0}, /* 3:                          signal2+signal1 */
	{  0,255,  0}, /* 4:                  signal3                 */
	{  0,  0,  0}, /* 5:                  signal3+        signal1 */
	{  0,255,  0}, /* 6:                  signal3+signal2         */
	{  0,  0,  0}, /* 7:                  signal3+signal2+signal1 */
	{  0,  0,255}, /* 8:          signal4                         */
	{  0,  0,  0}, /* 9:          signal4+                signal1 */
	{  0,  0,255}, /* 10:         signal4+        signal2         */
	{  0,  0,  0}, /* 11:         signal4+        signal2+signal1 */
	{  0,255,  0}, /* 12:         signal4+signal3                 */
	{  0,  0,  0}, /* 13:         signal4+signal3+        signal1 */
	{  0,255,  0}, /* 14:         signal4+signal3+signal2         */
	{  0,  0,  0}, /* 15:         signal4+signal3+signal2+signal1 */
	{255,255,  0}, /* 16: signal5+                                */
	{  0,  0,  0}, /* 17: signal5+                        signal1 */
	{255,255,  0}, /* 18: signal5+                signal2         */
	{  0,  0,  0}, /* 19: signal5+                signal2+signal1 */
	{  0,255,  0}, /* 20: signal5+        signal3                 */
	{  0,  0,  0}, /* 21: signal5+        signal3+        signal1 */
	{  0,255,  0}, /* 22: signal5+        signal3+signal2         */
	{  0,  0,  0}, /* 23: signal5+        signal3+signal2+signal1 */
	{  0,  0,255}, /* 24: signal5+signal4                         */
	{  0,  0,  0}, /* 25: signal5+signal4+                signal1 */
	{  0,  0,255}, /* 26: signal5+signal4+        signal2         */
	{  0,  0,  0}, /* 27: signal5+signal4+        signal2+signal1 */
	{  0,255,  0}, /* 28: signal5+signal4+signal3                 */
	{  0,  0,  0}, /* 29: signal5+signal4+signal3+        signal1 */
	{  0,255,  0}, /* 30: signal5+signal4+signal3+signal2         */
	{  0,  0,  0}  /* 31: signal5+signal4+signal3+signal2+signal1 */
};

/******************** ARCS ********************/

#define ARCPROTOCOUNT  21
#define APC1            0		/* Signal 1 */
#define APC2            1		/* Signal 2 */
#define APC3            2		/* Signal 3 */
#define APC4            3		/* Signal 4 */
#define APC5            4		/* Signal 5 */
#define APC6            5		/* Signal 6 */
#define APC7            6		/* Signal 7 */
#define APC8            7		/* Signal 8 */
#define APN1            8		/* Power 1 */
#define APN2            9		/* Power 2 */
#define APN3           10		/* Power 3 */
#define APN4           11		/* Power 4 */
#define APN5           12		/* Power 5 */
#define APN6           13		/* Power 6 */
#define APN7           14		/* Power 7 */
#define APN8           15		/* Power 8 */
#define APSSC          16		/* Top Silk Screen */
#define APSSS          17		/* Bottom Silk Screen */
#define APSMC          18		/* Top Solder Mask */
#define APSMS          19		/* Bottom Solder Mask */
#define APF            20		/* Engineering Drawing */

/* Signal 1 arc */
static TECH_ARCLAY pcb_al_c1[] = {{ LPC1,0,FILLED }};
static TECH_ARCS pcb_a_c1 = {
	x_("Signal-1"),0,APC1,NOARCPROTO,					/* name */
	1,pcb_al_c1,										/* layers */
	(APMETAL1<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};	/* userbits */

/* Signal 2 arc */
static TECH_ARCLAY pcb_al_c2[] = {{ LPC2,0,FILLED }};
static TECH_ARCS pcb_a_c2 = {
	x_("Signal-2"),0,APC2,NOARCPROTO,					/* name */
	1,pcb_al_c2,										/* layers */
	(APMETAL2<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};	/* userbits */

/* Signal 3 arc */
static TECH_ARCLAY pcb_al_c3[] = {{ LPC3,0,FILLED }};
static TECH_ARCS pcb_a_c3 = {
	x_("Signal-3"),0,APC3,NOARCPROTO,					/* name */
	1,pcb_al_c3,										/* layers */
	(APMETAL3<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};	/* userbits */

/* Signal 4 arc */
static TECH_ARCLAY pcb_al_c4[] = {{ LPC4,0,FILLED }};
static TECH_ARCS pcb_a_c4 = {
	x_("Signal-4"),0,APC4,NOARCPROTO,					/* name */
	1,pcb_al_c4,										/* layers */
	(APMETAL4<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};	/* userbits */

/* Signal 5 arc */
static TECH_ARCLAY pcb_al_c5[] = {{ LPC5,0,FILLED }};
static TECH_ARCS pcb_a_c5 = {
	x_("Signal-5"),0,APC5,NOARCPROTO,					/* name */
	1,pcb_al_c5,										/* layers */
	(APMETAL5<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};	/* userbits */

/* Signal 6 arc */
static TECH_ARCLAY pcb_al_c6[] = {{ LPC6,0,FILLED }};
static TECH_ARCS pcb_a_c6 = {
	x_("Signal-6"),0,APC6,NOARCPROTO,					/* name */
	1,pcb_al_c6,										/* layers */
	(APMETAL6<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};	/* userbits */

/* Signal 7 arc */
static TECH_ARCLAY pcb_al_c7[] = {{ LPC7,0,FILLED }};
static TECH_ARCS pcb_a_c7 = {
	x_("Signal-7"),0,APC7,NOARCPROTO,					/* name */
	1,pcb_al_c7,										/* layers */
	(APMETAL7<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};	/* userbits */

/* Signal 8 arc */
static TECH_ARCLAY pcb_al_c8[] = {{ LPC8,0,FILLED }};
static TECH_ARCS pcb_a_c8 = {
	x_("Signal-8"),0,APC8,NOARCPROTO,					/* name */
	1,pcb_al_c8,										/* layers */
	(APMETAL8<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};	/* userbits */

/* Power 1 arc */
static TECH_ARCLAY pcb_al_n1[] = {{ LPN1,0,FILLED }};
static TECH_ARCS pcb_a_n1 = {
	x_("Power-1"),0,APN1,NOARCPROTO,					/* name */
	1,pcb_al_n1,										/* layers */
	(APMETAL1<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};	/* userbits */

/* Power 2 arc */
static TECH_ARCLAY pcb_al_n2[] = {{ LPN2,0,FILLED }};
static TECH_ARCS pcb_a_n2 = {
	x_("Power-2"),0,APN2,NOARCPROTO,					/* name */
	1,pcb_al_n2,										/* layers */
	(APMETAL2<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};	/* userbits */

/* Power 3 arc */
static TECH_ARCLAY pcb_al_n3[] = {{ LPN3,0,FILLED }};
static TECH_ARCS pcb_a_n3 = {
	x_("Power-3"),0,APN3,NOARCPROTO,					/* name */
	1,pcb_al_n3,										/* layers */
	(APMETAL3<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};	/* userbits */

/* Power 4 arc */
static TECH_ARCLAY pcb_al_n4[] = {{ LPN4,0,FILLED }};
static TECH_ARCS pcb_a_n4 = {
	x_("Power-4"),0,APN4,NOARCPROTO,					/* name */
	1,pcb_al_n4,										/* layers */
	(APMETAL4<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};	/* userbits */

/* Power 5 arc */
static TECH_ARCLAY pcb_al_n5[] = {{ LPN5,0,FILLED }};
static TECH_ARCS pcb_a_n5 = {
	x_("Power-5"),0,APN5,NOARCPROTO,					/* name */
	1,pcb_al_n5,										/* layers */
	(APMETAL5<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};	/* userbits */

/* Power 6 arc */
static TECH_ARCLAY pcb_al_n6[] = {{ LPN6,0,FILLED }};
static TECH_ARCS pcb_a_n6 = {
	x_("Power-6"),0,APN6,NOARCPROTO,					/* name */
	1,pcb_al_n6,										/* layers */
	(APMETAL6<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};	/* userbits */

/* Power 7 arc */
static TECH_ARCLAY pcb_al_n7[] = {{ LPN7,0,FILLED }};
static TECH_ARCS pcb_a_n7 = {
	x_("Power-7"),0,APN7,NOARCPROTO,					/* name */
	1,pcb_al_n7,										/* layers */
	(APMETAL7<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};	/* userbits */

/* Power 8 arc */
static TECH_ARCLAY pcb_al_n8[] = {{ LPN8,0,FILLED }};
static TECH_ARCS pcb_a_n8 = {
	x_("Power-8"),0,APN8,NOARCPROTO,					/* name */
	1,pcb_al_n8,										/* layers */
	(APMETAL8<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};	/* userbits */

/* Top Silk Screen arc */
static TECH_ARCLAY pcb_al_ssc[] = {{ LPSSC,0,FILLED }};
static TECH_ARCS pcb_a_ssc = {
	x_("Top-Silk"),0,APSSC,NOARCPROTO,					/* name */
	1,pcb_al_ssc,										/* layers */
	(APNONELEC<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};/* userbits */

/* Bottom Silk Screen arc */
static TECH_ARCLAY pcb_al_sss[] = {{ LPSSS,0,FILLED }};
static TECH_ARCS pcb_a_sss = {
	x_("Bottom-Silk"),0,APSSS,NOARCPROTO,				/* name */
	1,pcb_al_sss,										/* layers */
	(APNONELEC<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};/* userbits */

/* Top Solder Mask arc */
static TECH_ARCLAY pcb_al_smc[] = {{ LPSMC,0,FILLED }};
static TECH_ARCS pcb_a_smc = {
	x_("Top-Solder"),0,APSMC,NOARCPROTO,				/* name */
	1,pcb_al_smc,										/* layers */
	(APNONELEC<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};/* userbits */

/* Bottom Solder Mask arc */
static TECH_ARCLAY pcb_al_sms[] = {{ LPSMS,0,FILLED }};
static TECH_ARCS pcb_a_sms = {
	x_("Bottom-Solder"),0,APSMS,NOARCPROTO,				/* name */
	1,pcb_al_sms,										/* layers */
	(APNONELEC<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};/* userbits */

/* Engineering Drawing arc */
static TECH_ARCLAY pcb_al_f[] = {{ LPF,0,FILLED }};
static TECH_ARCS pcb_a_f = {
	x_("Drawing"),0,APF,NOARCPROTO,						/* name */
	1,pcb_al_f,											/* layers */
	(APNONELEC<<AFUNCTIONSH)|CANWIPE|(45<<AANGLEINCSH)};/* userbits */

TECH_ARCS *pcb_arcprotos[ARCPROTOCOUNT+1] = {
	&pcb_a_c1, &pcb_a_c2, &pcb_a_c3, &pcb_a_c4, &pcb_a_c5, &pcb_a_c6,
	&pcb_a_c7, &pcb_a_c8, &pcb_a_n1, &pcb_a_n2, &pcb_a_n3, &pcb_a_n4,
	&pcb_a_n5, &pcb_a_n6, &pcb_a_n7, &pcb_a_n8, &pcb_a_ssc, &pcb_a_sss,
	&pcb_a_smc, &pcb_a_sms, &pcb_a_f, ((TECH_ARCS *)-1)};

/******************** PORTINST CONNECTIONS ********************/

/* these values are replaced with actual arcproto addresses */
static INTBIG pcb_pc_1[]  = {-1, APC1, APN1, ALLGEN, -1};
static INTBIG pcb_pc_2[]  = {-1, APC2, APN2, ALLGEN, -1};
static INTBIG pcb_pc_3[]  = {-1, APC3, APN3, ALLGEN, -1};
static INTBIG pcb_pc_4[]  = {-1, APC4, APN4, ALLGEN, -1};
static INTBIG pcb_pc_5[]  = {-1, APC5, APN5, ALLGEN, -1};
static INTBIG pcb_pc_6[]  = {-1, APC6, APN6, ALLGEN, -1};
static INTBIG pcb_pc_7[]  = {-1, APC7, APN7, ALLGEN, -1};
static INTBIG pcb_pc_8[]  = {-1, APC8, APN8, ALLGEN, -1};
static INTBIG pcb_pc_all[]  = {-1, APC1, APC2, APC3, APC4, APC5, APC6, APC7, APC8, ALLGEN, -1};
static INTBIG pcb_pc_ssc[]= {-1, APSSC, ALLGEN, -1};
static INTBIG pcb_pc_sss[]= {-1, APSSS, ALLGEN, -1};
static INTBIG pcb_pc_smc[]= {-1, APSMC, ALLGEN, -1};
static INTBIG pcb_pc_sms[]= {-1, APSMS, ALLGEN, -1};
static INTBIG pcb_pc_f[]  = {-1, APF, ALLGEN, -1};
static INTBIG pcb_pc_null[]={-1, ALLGEN, -1};

/******************** NODES ********************/

#define NODEPROTOCOUNT	44
#define NPC1P       1		/* Signal 1 Pin */
#define NPC2P       2		/* Signal 2 Pin */
#define NPC3P       3		/* Signal 3 Pin */
#define NPC4P       4		/* Signal 4 Pin */
#define NPC5P       5		/* Signal 5 Pin */
#define NPC6P       6		/* Signal 6 Pin */
#define NPC7P       7		/* Signal 7 Pin */
#define NPC8P       8		/* Signal 8 Pin */
#define NPN1P       9		/* Power 1 Pin */
#define NPN2P      10		/* Power 2 Pin */
#define NPN3P      11		/* Power 3 Pin */
#define NPN4P      12		/* Power 4 Pin */
#define NPN5P      13		/* Power 5 Pin */
#define NPN6P      14		/* Power 6 Pin */
#define NPN7P      15		/* Power 7 Pin */
#define NPN8P      16		/* Power 8 Pin */
#define NPSSCP     17		/* Top Silk Pin */
#define NPSSSP     18		/* Bottom Silk Pin */
#define NPSMCP     19		/* Top Solder Pin */
#define NPSMSP     20		/* Bottom Solder Pin */
#define NPDP       21		/* Drill Pin */
#define NPDNPP     22		/* NonPlated Drill Pin */
#define NPFP       23		/* Engineering Drawing Pin */
#define NPC1N      24		/* Signal 1 Node */
#define NPC2N      25		/* Signal 2 Node */
#define NPC3N      26		/* Signal 3 Node */
#define NPC4N      27		/* Signal 4 Node */
#define NPC5N      28		/* Signal 5 Node */
#define NPC6N      29		/* Signal 6 Node */
#define NPC7N      30		/* Signal 7 Node */
#define NPC8N      31		/* Signal 8 Node */
#define NPN1N      32		/* Power 1 Node */
#define NPN2N      33		/* Power 2 Node */
#define NPN3N      34		/* Power 3 Node */
#define NPN4N      35		/* Power 4 Node */
#define NPN5N      36		/* Power 5 Node */
#define NPN6N      37		/* Power 6 Node */
#define NPN7N      38		/* Power 7 Node */
#define NPN8N      39		/* Power 8 Node */
#define NPSSCN     40		/* Top Silk Node */
#define NPSSSN     41		/* Bottom Silk Node */
#define NPSMCN     42		/* Top Solder Node */
#define NPSMSN     43		/* Bottom Solder Node */
#define NPFN       44		/* Engineering Drawing Node */

static INTBIG pcb_disccenter[8] = {CENTER, CENTER, RIGHTEDGE, CENTER};
static INTBIG pcb_fullbox[8]    = {LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE};

/* Signal-1-pin */
static TECH_PORTS pcb_c1_p[] = {					/* ports */
	{pcb_pc_1, x_("signal-1"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_c1_l[] = {					/* layers */
	{LPC1, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_c1 = {
	x_("Signal-1-Pin"),NPC1P,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_c1_p,										/* ports */
	1,pcb_c1_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Signal-2-pin */
static TECH_PORTS pcb_c2_p[] = {					/* ports */
	{pcb_pc_2, x_("signal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_c2_l[] = {					/* layers */
	{LPC2, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_c2 = {
	x_("Signal-2-Pin"),NPC2P,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_c2_p,										/* ports */
	1,pcb_c2_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Signal-3-pin */
static TECH_PORTS pcb_c3_p[] = {					/* ports */
	{pcb_pc_3, x_("signal-3"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_c3_l[] = {					/* layers */
	{LPC3, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_c3 = {
	x_("Signal-3-Pin"),NPC3P,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_c3_p,										/* ports */
	1,pcb_c3_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Signal-4-pin */
static TECH_PORTS pcb_c4_p[] = {					/* ports */
	{pcb_pc_4, x_("signal-4"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_c4_l[] = {					/* layers */
	{LPC4, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_c4 = {
	x_("Signal-4-Pin"),NPC4P,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_c4_p,										/* ports */
	1,pcb_c4_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Signal-5-pin */
static TECH_PORTS pcb_c5_p[] = {					/* ports */
	{pcb_pc_5, x_("signal-5"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_c5_l[] = {					/* layers */
	{LPC5, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_c5 = {
	x_("Signal-5-Pin"),NPC5P,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_c5_p,										/* ports */
	1,pcb_c5_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Signal-6-pin */
static TECH_PORTS pcb_c6_p[] = {					/* ports */
	{pcb_pc_6, x_("signal-6"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_c6_l[] = {					/* layers */
	{LPC6, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_c6 = {
	x_("Signal-6-Pin"),NPC6P,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_c6_p,										/* ports */
	1,pcb_c6_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Signal-7-pin */
static TECH_PORTS pcb_c7_p[] = {					/* ports */
	{pcb_pc_7, x_("signal-7"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_c7_l[] = {					/* layers */
	{LPC7, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_c7 = {
	x_("Signal-7-Pin"),NPC7P,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_c7_p,										/* ports */
	1,pcb_c7_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Signal-8-pin */
static TECH_PORTS pcb_c8_p[] = {					/* ports */
	{pcb_pc_8, x_("signal-8"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_c8_l[] = {					/* layers */
	{LPC8, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_c8 = {
	x_("Signal-8-Pin"),NPC8P,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_c8_p,										/* ports */
	1,pcb_c8_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Power-1-pin */
static TECH_PORTS pcb_n1_p[] = {					/* ports */
	{pcb_pc_1, x_("power-1"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_n1_l[] = {					/* layers */
	{LPN1, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_n1 = {
	x_("Power-1-Pin"),NPN1P,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_n1_p,										/* ports */
	1,pcb_n1_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Power-2-pin */
static TECH_PORTS pcb_n2_p[] = {					/* ports */
	{pcb_pc_2, x_("power-2"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_n2_l[] = {					/* layers */
	{LPN2, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_n2 = {
	x_("Power-2-Pin"),NPN2P,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_n2_p,										/* ports */
	1,pcb_n2_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Power-3-pin */
static TECH_PORTS pcb_n3_p[] = {					/* ports */
	{pcb_pc_3, x_("power-3"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_n3_l[] = {					/* layers */
	{LPN3, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_n3 = {
	x_("Power-3-Pin"),NPN3P,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_n3_p,										/* ports */
	1,pcb_n3_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Power-4-pin */
static TECH_PORTS pcb_n4_p[] = {					/* ports */
	{pcb_pc_4, x_("power-4"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_n4_l[] = {					/* layers */
	{LPN4, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_n4 = {
	x_("Power-4-Pin"),NPN4P,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_n4_p,										/* ports */
	1,pcb_n4_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Power-5-pin */
static TECH_PORTS pcb_n5_p[] = {					/* ports */
	{pcb_pc_5, x_("power-5"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_n5_l[] = {					/* layers */
	{LPN5, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_n5 = {
	x_("Power-5-Pin"),NPN5P,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_n5_p,										/* ports */
	1,pcb_n5_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Power-6-pin */
static TECH_PORTS pcb_n6_p[] = {					/* ports */
	{pcb_pc_6, x_("power-6"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_n6_l[] = {					/* layers */
	{LPN6, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_n6 = {
	x_("Power-6-Pin"),NPN6P,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_n6_p,										/* ports */
	1,pcb_n6_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Power-7-pin */
static TECH_PORTS pcb_n7_p[] = {					/* ports */
	{pcb_pc_7, x_("power-7"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_n7_l[] = {					/* layers */
	{LPN7, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_n7 = {
	x_("Power-7-Pin"),NPN7P,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_n7_p,										/* ports */
	1,pcb_n7_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Power-8-pin */
static TECH_PORTS pcb_n8_p[] = {					/* ports */
	{pcb_pc_8, x_("power-8"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_n8_l[] = {					/* layers */
	{LPN8, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_n8 = {
	x_("Power-8-Pin"),NPN8P,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_n8_p,										/* ports */
	1,pcb_n8_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Top-Silk-pin */
static TECH_PORTS pcb_ssc_p[] = {					/* ports */
	{pcb_pc_ssc, x_("top-silk"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_ssc_l[] = {					/* layers */
	{LPSSC, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_ssc = {
	x_("Top-Silk-Pin"),NPSSCP,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_ssc_p,									/* ports */
	1,pcb_ssc_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Bottom-Silk-pin */
static TECH_PORTS pcb_sss_p[] = {					/* ports */
	{pcb_pc_sss, x_("bottom-silk"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_sss_l[] = {					/* layers */
	{LPSSS, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_sss = {
	x_("Bottom-Silk-Pin"),NPSSSP,NONODEPROTO,		/* name */
	Q1,Q1,											/* size */
	1,pcb_sss_p,									/* ports */
	1,pcb_sss_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Top-Solder-pin */
static TECH_PORTS pcb_smc_p[] = {					/* ports */
	{pcb_pc_smc, x_("top-solder"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_smc_l[] = {					/* layers */
	{LPSMC, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_smc = {
	x_("Top-Solder-Pin"),NPSMCP,NONODEPROTO,		/* name */
	Q1,Q1,											/* size */
	1,pcb_smc_p,									/* ports */
	1,pcb_smc_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Bottom-Solder-pin */
static TECH_PORTS pcb_sms_p[] = {					/* ports */
	{pcb_pc_sms, x_("bottom-solder"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_sms_l[] = {					/* layers */
	{LPSMS, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_sms = {
	x_("Bottom-Solder-Pin"),NPSMSP,NONODEPROTO,		/* name */
	Q1,Q1,											/* size */
	1,pcb_sms_p,									/* ports */
	1,pcb_sms_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Drill-pin */
static TECH_PORTS pcb_d_p[] = {						/* ports */
	{pcb_pc_all, x_("drill"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_d_l[] = {					/* layers */
	{LPD, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_d = {
	x_("Drill-Pin"),NPDP,NONODEPROTO,				/* name */
	Q1,Q1,											/* size */
	1,pcb_d_p,										/* ports */
	1,pcb_d_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* NonPlated-Drill-pin */
static TECH_PORTS pcb_dnp_p[] = {					/* ports */
	{pcb_pc_null, x_("nondrill"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_dnp_l[] = {					/* layers */
	{LPDNP, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_dnp = {
	x_("NonPlated-Drill-Pin"),NPDNPP,NONODEPROTO,	/* name */
	Q1,Q1,											/* size */
	1,pcb_dnp_p,									/* ports */
	1,pcb_dnp_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Engineering-Drawing-pin */
static TECH_PORTS pcb_f_p[] = {						/* ports */
	{pcb_pc_f, x_("engineering"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON pcb_f_l[] = {					/* layers */
	{LPF, 0, 2, DISC, POINTS, pcb_disccenter}};
static TECH_NODES pcb_f = {
	x_("Engineering-Drawing-Pin"),NPFP,NONODEPROTO,	/* name */
	Q1,Q1,											/* size */
	1,pcb_f_p,										/* ports */
	1,pcb_f_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2|NSQUARE,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Signal-1-node */
static TECH_PORTS pcb_c1n_p[] = {					/* ports */
	{pcb_pc_1, x_("signal-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_c1n_l[] = {					/* layers */
	{LPC1, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_c1n = {
	x_("Signal-1-Node"),NPC1N,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_c1n_p,									/* ports */
	1,pcb_c1n_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Signal-2-node */
static TECH_PORTS pcb_c2n_p[] = {					/* ports */
	{pcb_pc_2, x_("signal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_c2n_l[] = {					/* layers */
	{LPC2, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_c2n = {
	x_("Signal-2-Node"),NPC2N,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_c2n_p,									/* ports */
	1,pcb_c2n_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Signal-3-node */
static TECH_PORTS pcb_c3n_p[] = {					/* ports */
	{pcb_pc_3, x_("signal-3"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_c3n_l[] = {					/* layers */
	{LPC3, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_c3n = {
	x_("Signal-3-Node"),NPC3N,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_c3n_p,									/* ports */
	1,pcb_c3n_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Signal-4-node */
static TECH_PORTS pcb_c4n_p[] = {					/* ports */
	{pcb_pc_4, x_("signal-4"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_c4n_l[] = {					/* layers */
	{LPC4, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_c4n = {
	x_("Signal-4-Node"),NPC4N,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_c4n_p,									/* ports */
	1,pcb_c4n_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Signal-5-node */
static TECH_PORTS pcb_c5n_p[] = {					/* ports */
	{pcb_pc_5, x_("signal-5"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_c5n_l[] = {					/* layers */
	{LPC5, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_c5n = {
	x_("Signal-5-Node"),NPC5N,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_c5n_p,									/* ports */
	1,pcb_c5n_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Signal-6-node */
static TECH_PORTS pcb_c6n_p[] = {					/* ports */
	{pcb_pc_6, x_("signal-6"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_c6n_l[] = {					/* layers */
	{LPC6, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_c6n = {
	x_("Signal-6-Node"),NPC6N,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_c6n_p,									/* ports */
	1,pcb_c6n_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Signal-7-node */
static TECH_PORTS pcb_c7n_p[] = {					/* ports */
	{pcb_pc_7, x_("signal-7"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_c7n_l[] = {					/* layers */
	{LPC7, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_c7n = {
	x_("Signal-7-Node"),NPC7N,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_c7n_p,									/* ports */
	1,pcb_c7n_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Signal-8-node */
static TECH_PORTS pcb_c8n_p[] = {					/* ports */
	{pcb_pc_8, x_("signal-8"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_c8n_l[] = {					/* layers */
	{LPC8, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_c8n = {
	x_("Signal-8-Node"),NPC8N,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_c8n_p,									/* ports */
	1,pcb_c8n_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Power-1-node */
static TECH_PORTS pcb_n1n_p[] = {					/* ports */
	{pcb_pc_1, x_("power-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_n1n_l[] = {					/* layers */
	{LPN1, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_n1n = {
	x_("Power-1-Node"),NPN1N,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_n1n_p,									/* ports */
	1,pcb_n1n_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Power-2-node */
static TECH_PORTS pcb_n2n_p[] = {					/* ports */
	{pcb_pc_2, x_("power-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_n2n_l[] = {					/* layers */
	{LPN2, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_n2n = {
	x_("Power-2-Node"),NPN2N,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_n2n_p,									/* ports */
	1,pcb_n2n_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Power-3-node */
static TECH_PORTS pcb_n3n_p[] = {					/* ports */
	{pcb_pc_3, x_("power-3"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_n3n_l[] = {					/* layers */
	{LPN3, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_n3n = {
	x_("Power-3-Node"),NPN3N,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_n3n_p,									/* ports */
	1,pcb_n3n_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Power-4-node */
static TECH_PORTS pcb_n4n_p[] = {					/* ports */
	{pcb_pc_4, x_("power-4"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_n4n_l[] = {					/* layers */
	{LPN4, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_n4n = {
	x_("Power-4-Node"),NPN4N,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_n4n_p,									/* ports */
	1,pcb_n4n_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Power-5-node */
static TECH_PORTS pcb_n5n_p[] = {					/* ports */
	{pcb_pc_5, x_("power-5"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_n5n_l[] = {					/* layers */
	{LPN5, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_n5n = {
	x_("Power-5-Node"),NPN5N,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_n5n_p,									/* ports */
	1,pcb_n5n_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Power-6-node */
static TECH_PORTS pcb_n6n_p[] = {					/* ports */
	{pcb_pc_6, x_("power-6"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_n6n_l[] = {					/* layers */
	{LPN6, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_n6n = {
	x_("Power-6-Node"),NPN6N,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_n6n_p,									/* ports */
	1,pcb_n6n_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Power-7-node */
static TECH_PORTS pcb_n7n_p[] = {					/* ports */
	{pcb_pc_7, x_("power-7"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_n7n_l[] = {					/* layers */
	{LPN7, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_n7n = {
	x_("Power-7-Node"),NPN7N,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_n7n_p,									/* ports */
	1,pcb_n7n_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Power-8-node */
static TECH_PORTS pcb_n8n_p[] = {					/* ports */
	{pcb_pc_8, x_("power-8"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_n8n_l[] = {					/* layers */
	{LPN8, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_n8n = {
	x_("Power-8-Node"),NPN8N,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_n8n_p,									/* ports */
	1,pcb_n8n_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Top-Silk-node */
static TECH_PORTS pcb_sscn_p[] = {					/* ports */
	{pcb_pc_ssc, x_("top-silk"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_sscn_l[] = {				/* layers */
	{LPSSC, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_sscn = {
	x_("Top-Silk-Node"),NPSSCN,NONODEPROTO,			/* name */
	Q1,Q1,											/* size */
	1,pcb_sscn_p,									/* ports */
	1,pcb_sscn_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Bottom-Silk-node */
static TECH_PORTS pcb_sssn_p[] = {					/* ports */
	{pcb_pc_sss, x_("bottom-silk"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_sssn_l[] = {				/* layers */
	{LPSSS, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_sssn = {
	x_("Bottom-Silk-Node"),NPSSSN,NONODEPROTO,		/* name */
	Q1,Q1,											/* size */
	1,pcb_sssn_p,									/* ports */
	1,pcb_sssn_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Top-Solder-node */
static TECH_PORTS pcb_smcn_p[] = {					/* ports */
	{pcb_pc_smc, x_("top-solder"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_smcn_l[] = {				/* layers */
	{LPSMC, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_smcn = {
	x_("Top-Solder-Node"),NPSMCN,NONODEPROTO,		/* name */
	Q1,Q1,											/* size */
	1,pcb_smcn_p,									/* ports */
	1,pcb_smcn_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Bottom-Solder-node */
static TECH_PORTS pcb_smsn_p[] = {					/* ports */
	{pcb_pc_sms, x_("bottom-solder"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_smsn_l[] = {				/* layers */
	{LPSMS, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_smsn = {
	x_("Bottom-Solder-Node"),NPSMSN,NONODEPROTO,	/* name */
	Q1,Q1,											/* size */
	1,pcb_smsn_p,									/* ports */
	1,pcb_smsn_l,									/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Engineering-Drawing-node */
static TECH_PORTS pcb_fn_p[] = {					/* ports */
	{pcb_pc_f, x_("engineering"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON pcb_fn_l[] = {					/* layers */
	{LPF, 0, 4, FILLEDRECT, BOX, pcb_fullbox}};
static TECH_NODES pcb_fn = {
	x_("Engineering-Drawing-Node"),NPFN,NONODEPROTO,/* name */
	Q1,Q1,											/* size */
	1,pcb_fn_p,										/* ports */
	1,pcb_fn_l,										/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

TECH_NODES *pcb_nodeprotos[NODEPROTOCOUNT+1] = {
	&pcb_c1, &pcb_c2, &pcb_c3, &pcb_c4, &pcb_c5, &pcb_c6, &pcb_c7, &pcb_c8,
	&pcb_n1, &pcb_n2, &pcb_n3, &pcb_n4, &pcb_n5, &pcb_n6, &pcb_n7, &pcb_n8,
	&pcb_ssc, &pcb_sss, &pcb_smc, &pcb_sms, &pcb_d, &pcb_dnp, &pcb_f,
	&pcb_c1n, &pcb_c2n, &pcb_c3n, &pcb_c4n, &pcb_c5n, &pcb_c6n, &pcb_c7n,
	&pcb_c8n, &pcb_n1n, &pcb_n2n, &pcb_n3n, &pcb_n4n, &pcb_n5n, &pcb_n6n,
	&pcb_n7n, &pcb_n8n, &pcb_sscn, &pcb_sssn, &pcb_smcn, &pcb_smsn, &pcb_fn,
	((TECH_NODES *)-1)};

/******************** VARIABLE AGGREGATION ********************/

TECH_VARIABLES pcb_variables[] =
{
	/* set general information about the technology */
	{x_("TECH_layer_names"), (CHAR *)pcb_layer_names, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_layer_function"), (CHAR *)pcb_layer_function, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},

	/* set information for the I/O tool */
	{x_("IO_cif_layer_names"), (CHAR *)pcb_cif_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},

	/* set information for the USER tool */
	{x_("USER_color_map"), (CHAR *)pcb_colmap, 0.0,
		VCHAR|VDONTSAVE|VISARRAY|((sizeof pcb_colmap)<<VLENGTHSH)},
	{x_("USER_layer_letters"), (CHAR *)pcb_layer_letters, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{NULL, NULL, 0.0, 0}
};

/******************** ROUTINES ********************/

BOOLEAN pcb_initprocess(TECHNOLOGY *tech, INTBIG pass)
{
	Q_UNUSED( tech );
	Q_UNUSED( pass );
	return(FALSE);
}
