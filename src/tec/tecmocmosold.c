/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tecmocmos.c
 * Old MOSIS CMOS technology description
 * Written by: Steven M. Rubin, Static Free Software
 * Specified by: Dick Lyon and Carver Mead
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
#include "tecmocmosold.h"
#include "efunction.h"

typedef struct
{
	VARIABLE *shrinkvar;
} MOCPOLYLOOP;

static MOCPOLYLOOP mocmosold_oneprocpolyloop;

/* prototypes for local routines */
static void   mocmosold_setpseudo(void);
static INTBIG mocmosold_intarcpolys(ARCINST *ai, WINDOWPART *win, POLYLOOP *pl, MOCPOLYLOOP *mocpl);
static void   mocmosold_intshapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly, POLYLOOP *pl, MOCPOLYLOOP *mocpl);

/*
 * this technology has six important layers to draw:
 *     metal-1, metal-2, poly, active, select, and well
 * Since three bitplanes are used by the system, not all of these layers
 * can have their own bitplanes.  For example, with 8-bit color maps, only
 * five of these can have their own bitplanes.
 * Therefore, the layers are ordered in the following importance:
 *    metal-1, poly, active	always in their own bitplane (6-bits or more)
 *    metal-2                   for 7-bits
 *    well                      for 8-bits
 *    select                    for 9-bits
 * When a layer cannot have its own bitplane, it is drawn with stipple patterns.
 * On certain displays (notably VAX frame buffers) this cannot be done.
 * Therefore, use "technology tell mocmos outline-layers"
 *
 * By default, this technology assumes the General CMOS process, where CIF
 * layers CSG and CWG are used for select and well.  It can also handle the
 * Either CMOS process, where CIF layers CSP and CWP are used for P-type
 * select/well and CSN and CWN are used for N-type select/well.  The Either
 * mode, set with "technology tell mocmos either-process", causes both P and
 * N layers to be written for all components so that either process can be
 * selected after design is complete.
 *
 * This technology can also switch from P-well (the default) to N-well with
 * "technology tell mocmos n-well".  This switching works properly regardless
 * of the General/Either process selection.
 */

/* the options table */
static KEYWORD mocmosopt[] =
{
	{x_("p-well"),                     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("n-well"),                     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("general-process"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("either-process"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("stipple-layers"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("outline-layers"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("convert-old-format-library"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP mocmosold_parse = {mocmosopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("MOSIS CMOS option"), M_("show current options")};

#define EITHER    1		/* set for either, cleared for general */
#define OUTLINE   2		/* set to outline layers, clear for stipple */
#define NWELL     4		/* set for n-well, clear for p-well */
static INTBIG mocmosold_state = 0;
static TECHNOLOGY *mocmosold_tech;
static CHAR *mocmosold_stipplelayers = x_("Select and Passivation");

/******************** LAYERS ********************/

#define MAXLAYERS 26		/* total layers below         */
#define LMETAL1    0		/* metal layer 1              */
#define LMETAL2    1		/* metal layer 2              */
#define LPOLY      2		/* polysilicon                */
#define LSACT      3		/* S active (diffusion)       */
#define LDACT      4		/* D active (diffusion)       */
#define LSELECTP   5		/* P-type select              */
#define LSELECTN   6		/* N-type select              */
#define LWELLP     7		/* P-type well                */
#define LWELLN     8		/* N-type well                */
#define LCUT       9		/* contact cut                */
#define LVIA      10		/* metal-to-metal via         */
#define LPASS     11		/* passivation (overglass)    */
#define LTRANS    12		/* transistor                 */
#define LPOLYCUT  13		/* poly contact cut           */
#define LACTCUT   14		/* active contact cut         */
#define LSACTWELL 15		/* S active in well           */
#define LMET1P    16		/* pseudo metal 1             */
#define LMET2P    17		/* pseudo metal 2             */
#define LPOLYP    18		/* pseudo polysilicon         */
#define LSACTP    19		/* pseudo S active            */
#define LDACTP    20		/* pseudo D active            */
#define LSELECTPP 21		/* pseudo P-type select       */
#define LSELECTNP 22		/* pseudo N-type select       */
#define LWELLPP   23		/* pseudo P-type well         */
#define LWELLNP   24		/* pseudo N-type well         */
#define LFRAME    25		/* pad frame boundary         */

static GRAPHICS mocmosold_m1_lay = {LAYERT1,COLORT1, SOLIDC, PATTERNED,
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
static GRAPHICS mocmosold_m2_lay = {LAYERT4,COLORT4, SOLIDC, PATTERNED,
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
static GRAPHICS mocmosold_p_lay = {LAYERT2,COLORT2, SOLIDC, PATTERNED,
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
static GRAPHICS mocmosold_sa_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
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
static GRAPHICS mocmosold_da_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
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
static GRAPHICS mocmosold_ssp_lay = {LAYERO,YELLOW, PATTERNED, PATTERNED,
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
static GRAPHICS mocmosold_ssn_lay = {LAYERN,YELLOW, PATTERNED, PATTERNED,
/* N Select layer */	{0x1010, /*    X       X     */
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
static GRAPHICS mocmosold_wp_lay = {LAYERT5,COLORT5, SOLIDC, PATTERNED,
/* P Well implant */	{0x1000, /*    X             */
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
static GRAPHICS mocmosold_wn_lay = {LAYERN,COLORT5, SOLIDC, PATTERNED,
/* N Well implant */	{0x1000, /*    X             */
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
static GRAPHICS mocmosold_c_lay = {LAYERO,BLACK, SOLIDC, SOLIDC,
/* cut layer */		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmosold_v_lay = {LAYERO,BLACK, SOLIDC, SOLIDC,
/* via layer */		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmosold_ovs_lay = {LAYERO,DGRAY, PATTERNED, PATTERNED,
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
static GRAPHICS mocmosold_tr_lay = {LAYERN,ALLOFF, SOLIDC, SOLIDC,
/* transistor layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmosold_pc_lay = {LAYERO,BLACK, SOLIDC, SOLIDC,
/* poly cut layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmosold_ac_lay = {LAYERO,BLACK, SOLIDC, SOLIDC,
/* active cut layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS mocmosold_saw_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
/* S act well layer */	{0x0000, /*                  */
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
static GRAPHICS mocmosold_pm1_lay ={LAYERT1,COLORT1, SOLIDC, PATTERNED,
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
static GRAPHICS mocmosold_pm2_lay = {LAYERT4,COLORT4, SOLIDC, PATTERNED,
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
static GRAPHICS mocmosold_pp_lay = {LAYERT2,COLORT2, SOLIDC, PATTERNED,
/* pseudo poly layer */	{0x0808, /*     X       X    */
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
static GRAPHICS mocmosold_psa_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
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
static GRAPHICS mocmosold_pda_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
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
static GRAPHICS mocmosold_pssp_lay = {LAYERO,YELLOW,PATTERNED, PATTERNED,
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
static GRAPHICS mocmosold_pssn_lay = {LAYERN,YELLOW,PATTERNED, PATTERNED,
/* pseudo N Select */	{0x1010, /*    X       X     */
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
static GRAPHICS mocmosold_pwp_lay = {LAYERT5,COLORT5, SOLIDC, PATTERNED,
/* pseudo P Well */		{0x1000, /*    X             */
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
static GRAPHICS mocmosold_pwn_lay = {LAYERN,COLORT5, SOLIDC, PATTERNED,
/* pseudo N Well */		{0x1000, /*    X             */
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
static GRAPHICS mocmosold_pf_lay = {LAYERO, RED, SOLIDC, PATTERNED,
/* pad frame */		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};

/* these tables must be updated together */
GRAPHICS *mocmosold_layers[MAXLAYERS+1] = {&mocmosold_m1_lay, &mocmosold_m2_lay,
	&mocmosold_p_lay, &mocmosold_sa_lay, &mocmosold_da_lay, &mocmosold_ssp_lay,
	&mocmosold_ssn_lay, &mocmosold_wp_lay, &mocmosold_wn_lay, &mocmosold_c_lay,
	&mocmosold_v_lay, &mocmosold_ovs_lay, &mocmosold_tr_lay, &mocmosold_pc_lay,
	&mocmosold_ac_lay, &mocmosold_saw_lay, &mocmosold_pm1_lay, &mocmosold_pm2_lay,
	&mocmosold_pp_lay, &mocmosold_psa_lay, &mocmosold_pda_lay, &mocmosold_pssp_lay,
	&mocmosold_pssn_lay, &mocmosold_pwp_lay, &mocmosold_pwn_lay, &mocmosold_pf_lay, NOGRAPHICS};
static CHAR *mocmosold_layer_names[MAXLAYERS] = {x_("Metal-1"), x_("Metal-2"),
	x_("Polysilicon"), x_("S-Active"), x_("D-Active"), x_("P-Select"), x_("N-Select"), x_("P-Well"),
	x_("N-Well"), x_("Contact-Cut"), x_("Via"), x_("Passivation"), x_("Transistor"), x_("Poly-Cut"),
	x_("Active-Cut"), x_("S-Active-Well"), x_("Pseudo-Metal-1"), x_("Pseudo-Metal-2"),
	x_("Pseudo-Polysilicon"), x_("Pseudo-S-Active"), x_("Pseudo-D-Active"),
	x_("Pseudo-P-Select"), x_("Pseudo-N-Select"), x_("Pseudo-P-Well"), x_("Pseudo-N-Well"),
	x_("Pad-Frame")};
static INTBIG mocmosold_layer_function[MAXLAYERS] = {LFMETAL1|LFTRANS1,
	LFMETAL2|LFTRANS4, LFPOLY1|LFTRANS2, LFDIFF|LFPTYPE|LFTRANS3,
	LFDIFF|LFNTYPE|LFTRANS3, LFIMPLANT|LFPTYPE, LFIMPLANT|LFNTYPE,
	LFWELL|LFPTYPE|LFTRANS5, LFWELL|LFNTYPE|LFTRANS5, LFCONTACT1,
	LFCONTACT2|LFCONMETAL, LFOVERGLASS, LFTRANSISTOR|LFPSEUDO,
	LFCONTACT1|LFCONPOLY, LFCONTACT1|LFCONDIFF,
	LFDIFF|LFPTYPE|LFTRANS3, LFMETAL1|LFPSEUDO|LFTRANS1,
	LFMETAL2|LFPSEUDO|LFTRANS4, LFPOLY1|LFPSEUDO|LFTRANS2,
	LFDIFF|LFPTYPE|LFPSEUDO|LFTRANS3, LFDIFF|LFNTYPE|LFPSEUDO|LFTRANS3,
	LFIMPLANT|LFPTYPE|LFPSEUDO, LFIMPLANT|LFNTYPE|LFPSEUDO,
	LFWELL|LFPTYPE|LFPSEUDO|LFTRANS5, LFWELL|LFNTYPE|LFPSEUDO|LFTRANS5,
	LFART};
static CHAR *mocmosold_cif_layers[MAXLAYERS] = {x_("CMF"), x_("CMS"), x_("CPG"), x_("CAA"), x_("CAA"),
	x_("CSG"), x_("CSG"), x_("CWG"), x_("CWG"), x_("CC"), x_("CVA"), x_("COG"), x_(""), x_("CCP"), x_("CCA"), x_("CAA"), x_(""),
	x_(""), x_(""), x_(""), x_(""), x_(""), x_(""), x_(""), x_(""), x_("CX")};
static CHAR *mocmosold_gds_layers[MAXLAYERS] = {x_("10"), x_("19"), x_("12"), x_("2"), x_("2"), x_("8"), x_("7"), x_("1"), x_("1"), x_("9"),
	x_("18"), x_("11"), x_(""), x_("9"), x_("9"), x_("2"), x_(""), x_(""), x_(""), x_(""), x_(""), x_(""), x_(""), x_(""), x_(""), x_("")};
static CHAR *mocmosold_layer_letters[MAXLAYERS] = {x_("m"), x_("h"), x_("p"), x_("s"), x_("d"), x_("e"),
	x_("f"), x_("w"), x_("n"), x_("c"), x_("v"), x_("o"), x_("t"), x_("a"), x_("A"), x_("x"), x_("M"), x_("H"), x_("P"), x_("S"), x_("D"),
	x_("E"), x_("F"), x_("W"), x_("N"), x_("b")};

/*
 * tables for converting between stipples and outlines
 */
static GRAPHICS *mocmosold_layerstipple[] =
{
	&mocmosold_ssn_lay,
	&mocmosold_ssp_lay,
	&mocmosold_ovs_lay,
	&mocmosold_pssn_lay,
	&mocmosold_pssp_lay,
	&mocmosold_wp_lay,
	&mocmosold_wn_lay,
	&mocmosold_pwp_lay,
	&mocmosold_pwn_lay,
	&mocmosold_m2_lay,
	&mocmosold_pm2_lay
};
static INTBIG mocmosold_layerstipplesize = 11;

/* The low 5 bits map Metal-1, Poly, Active, Metal-2}, and well */
static TECH_COLORMAP mocmosold_colmap[32] =
{                  /*     well metal2 active poly metal1 */
	{200,200,200}, /* 0:                                 */
	{ 96,209,255}, /* 1:                          metal1 */
	{255,155,192}, /* 2:                     poly        */
	{ 96,127,192}, /* 3:                     poly+metal1 */
	{107,226, 96}, /* 4:              active             */
	{ 40,186, 96}, /* 5:              active+     metal1 */
	{107,137, 72}, /* 6:              active+poly        */
	{ 40,113, 72}, /* 7:              active+poly+metal1 */
	{224, 95,255}, /* 8:       metal2                    */
	{ 85, 78,255}, /* 9:       metal2+            metal1 */
	{224, 57,192}, /* 10:      metal2+       poly        */
	{ 85, 47,192}, /* 11:      metal2+       poly+metal1 */
	{ 94, 84, 96}, /* 12:      metal2+active             */
	{ 36, 69, 96}, /* 13:      metal2+active+     metal1 */
	{ 94, 51, 72}, /* 14:      metal2+active+poly        */
	{ 36, 42, 72}, /* 15:      metal2+active+poly+metal1 */
	{240,221,181}, /* 16: well                           */
	{ 91,182,181}, /* 17: well+                   metal1 */
	{240,134,136}, /* 18: well+              poly        */
	{ 91,111,136}, /* 19: well+              poly+metal1 */
	{101,196, 68}, /* 20: well+       active             */
	{ 38,161, 68}, /* 21: well+       active+     metal1 */
	{101,119, 51}, /* 22: well+       active+poly        */
	{ 38, 98, 51}, /* 23: well+       active+poly+metal1 */
	{211, 82,181}, /* 24: well+metal2                    */
	{ 80, 68,181}, /* 25: well+metal2+            metal1 */
	{211, 50,136}, /* 26: well+metal2+       poly        */
	{ 80, 41,136}, /* 27: well+metal2+       poly+metal1 */
	{ 89, 73, 68}, /* 28: well+metal2+active             */
	{ 33, 60, 68}, /* 29: well+metal2+active+     metal1 */
	{ 89, 44, 51}, /* 30: well+metal2+active+poly        */
	{ 33, 36, 51}  /* 31: well+metal2+active+poly+metal1 */
};

/******************** DESIGN RULES ********************/

#define X	XX
#define A	K1
#define B	K2
#define C	K3
#define D	K4
#define E	K5

/* layers that can connect to other layers when electrically disconnected */
static INTBIG mocmosold_unconnectedtable[] = {
/*          M M P S D S S W W C V P T P A S M M P S D S S W W P */
/*          e e o A A e e e e u i a r o c a e e o A A e e e e a */
/*          t t l c c l l l l t a s a l t c t t l c c l l l l d */
/*          1 2 y t t P N l l     s n y C t 1 2 y t t P N P N F */
/*                        P N       s C   W P P P P P P P P P r */
/* Met1  */ C,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Met2  */   D,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Poly  */     B,A,A,X,X,X,X,X,B,X,X,D,X,X,X,X,X,X,X,X,X,X,X,X,
/* SAct  */       C,C,X,X,D,X,X,B,X,X,X,E,X,X,X,X,X,X,X,X,X,X,X,
/* DAct  */         C,X,X,X,X,X,B,X,X,X,E,X,X,X,X,X,X,X,X,X,X,X,
/* SelP  */           X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* SelN  */             X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* WellP */               X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* WellN */                 X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Cut   */                   B,B,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Via   */                     B,X,B,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Pass  */                       X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Trans */                         X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* PolyC */                           X,X,X,X,X,X,X,X,X,X,X,X,X,
/* ActC  */                             X,X,X,X,X,X,X,X,X,X,X,X,
/* SactW */                               X,X,X,X,X,X,X,X,X,X,X,
/* Met1P */                                 X,X,X,X,X,X,X,X,X,X,
/* Met2P */                                   X,X,X,X,X,X,X,X,X,
/* PolyP */                                     X,X,X,X,X,X,X,X,
/* SActP */                                       X,X,X,X,X,X,X,
/* DActP */                                         X,X,X,X,X,X,
/* SelPP */                                           X,X,X,X,X,
/* SelNP */                                             X,X,X,X,
/* WelPP */                                               X,X,X,
/* WelNP */                                                 X,X,
/* PadFr */                                                   X,
};

/* layers that can connect to other layers when electrically connected */
static INTBIG mocmosold_connectedtable[] = {
/*          M M P S D S S W W C V P T P A S M M P S D S S W W P */
/*          e e o A A e e e e u i a r o c a e e o A A e e e e a */
/*          t t l c c l l l l t a s a l t c t t l c c l l l l d */
/*          1 2 y t t P N l l     s n y C t 1 2 y t t P N P N F */
/*                        P N       s C   W P P P P P P P P P r */
/* Met1  */ X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Met2  */   X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Poly  */     X,A,A,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* SAct  */       X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* DAct  */         X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* SelP  */           X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* SelN  */             X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* WellP */               X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* WellN */                 X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Cut   */                   X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Via   */                     B,X,X,B,B,X,X,X,X,X,X,X,X,X,X,X,
/* Pass  */                       X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* Trans */                         X,X,X,X,X,X,X,X,X,X,X,X,X,X,
/* PolyC */                           B,X,X,X,X,X,X,X,X,X,X,X,X,
/* ActC  */                             B,X,X,X,X,X,X,X,X,X,X,X,
/* SactW */                               X,X,X,X,X,X,X,X,X,X,X,
/* Met1P */                                 X,X,X,X,X,X,X,X,X,X,
/* Met2P */                                   X,X,X,X,X,X,X,X,X,
/* PolyP */                                     X,X,X,X,X,X,X,X,
/* SActP */                                       X,X,X,X,X,X,X,
/* DActP */                                         X,X,X,X,X,X,
/* SelPP */                                           X,X,X,X,X,
/* SelNP */                                             X,X,X,X,
/* WelPP */                                               X,X,X,
/* WelNP */                                                 X,X,
/* PadFr */                                                   X,
};

/******************** ARCS ********************/

#define ARCPROTOCOUNT  6
#define AMETAL1        0	/* metal-1                   */
#define AMETAL2        1	/* metal-2                   */
#define APOLY          2	/* polysilicon               */
#define ASACT          3	/* S-active                  */
#define ADACT          4	/* D-active                  */
#define AACT           5	/* General active            */

/* metal 1 arc */
static TECH_ARCLAY mocmosold_al_m1[] = {{LMETAL1,0,FILLED }};
static TECH_ARCS mocmosold_a_m1 = {
	x_("Metal-1"),K3,AMETAL1,NOARCPROTO,							/* name */
	1,mocmosold_al_m1,												/* layers */
	(APMETAL1<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* metal 2 arc */
static TECH_ARCLAY mocmosold_al_m2[] = {{LMETAL2,0,FILLED }};
static TECH_ARCS mocmosold_a_m2 = {
	x_("Metal-2"),K3,AMETAL2,NOARCPROTO,							/* name */
	1,mocmosold_al_m2,												/* layers */
	(APMETAL2<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* polysilicon arc */
static TECH_ARCLAY mocmosold_al_p[] = {{LPOLY,0,FILLED }};
static TECH_ARCS mocmosold_a_po = {
	x_("Polysilicon"),K2,APOLY,NOARCPROTO,							/* name */
	1,mocmosold_al_p,												/* layers */
	(APPOLY1<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* S-active arc */
static TECH_ARCLAY mocmosold_al_pa[] = {{LSACT,K8,FILLED}, {LWELLNP,0,FILLED},
	{LSELECTP,K4,FILLED}};
static TECH_ARCS mocmosold_a_pa = {
	x_("S-Active"),K10,ASACT,NOARCPROTO,							/* name */
	3,mocmosold_al_pa,												/* layers */
	(APDIFFP<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* D-active arc */
static TECH_ARCLAY mocmosold_al_na[] = {{LDACT,K8,FILLED}, {LWELLP,0,FILLED},
	{LSELECTNP,K4,FILLED}};
static TECH_ARCS mocmosold_a_na = {
	x_("D-Active"),K10,ADACT,NOARCPROTO,							/* name */
	3,mocmosold_al_na,												/* layers */
	(APDIFFN<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* General active arc */
static TECH_ARCLAY mocmosold_al_a[] = {{LDACT,0,FILLED}, {LSACT,0,FILLED}};
static TECH_ARCS mocmosold_a_a = {
	x_("Active"),K2,AACT,NOARCPROTO,								/* name */
	2,mocmosold_al_a,												/* layers */
	(APDIFF<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

TECH_ARCS *mocmosold_arcprotos[ARCPROTOCOUNT+1] = {
	&mocmosold_a_m1, &mocmosold_a_m2, &mocmosold_a_po, &mocmosold_a_pa, &mocmosold_a_na,
	&mocmosold_a_a, ((TECH_ARCS *)-1)};

static INTBIG mocmosold_arc_widoff[ARCPROTOCOUNT] = {0,0,0,K8,K8,0};

/*
 * tables for converting between stipples and outlines
 */
typedef struct
{
	TECH_ARCLAY *str;
	INTBIG        layer;
} STIPARC;
static STIPARC mocmosold_arcstipple[] =
{
	{mocmosold_al_pa, 2},		/* select-p */
	{mocmosold_al_na, 2},		/* select-n */
	{mocmosold_al_pa, 1},		/* well-n */
	{mocmosold_al_na, 1},		/* well-p */
	{mocmosold_al_m2, 0}		/* metal-2 */
};
static INTBIG mocmosold_arcstipplesize = 5;

/******************** PORTINST CONNECTIONS ********************/

static INTBIG mocmosold_pc_m1[]   = {-1, AMETAL1, ALLGEN, -1};
static INTBIG mocmosold_pc_m1a[]  = {-1, AMETAL1, AACT, ALLGEN, -1};
static INTBIG mocmosold_pc_m2[]   = {-1, AMETAL2, ALLGEN, -1};
static INTBIG mocmosold_pc_p[]    = {-1, APOLY, ALLGEN, -1};
static INTBIG mocmosold_pc_pa[]   = {-1, ASACT, ALLGEN, -1};
static INTBIG mocmosold_pc_a[]    = {-1, AACT, ASACT, ADACT, ALLGEN,-1};
static INTBIG mocmosold_pc_na[]   = {-1, ADACT, ALLGEN, -1};
static INTBIG mocmosold_pc_pam1[] = {-1, ASACT, AMETAL1, ALLGEN, -1};
static INTBIG mocmosold_pc_nam1[] = {-1, ADACT, AMETAL1, ALLGEN, -1};
static INTBIG mocmosold_pc_pm1[]  = {-1, APOLY, AMETAL1, ALLGEN, -1};
static INTBIG mocmosold_pc_mm[]   = {-1, AMETAL1, AMETAL2, ALLGEN, -1};
static INTBIG mocmosold_pc_null[] = {-1, ALLGEN, -1};

/******************** NODES ********************/

#define NODEPROTOCOUNT 29
#define NMETAL1P        1	/* metal-1 pin */
#define NMETAL2P        2	/* metal-2 pin */
#define NPOLYP          3	/* polysilicon pin */
#define NSACTP          4	/* S-active pin */
#define NDACTP          5	/* D-active pin */
#define NACTP           6	/* General active pin */
#define NMETSACTC       7	/* metal-1-S-active contact */
#define NMETDACTC       8	/* metal-1-D-active contact */
#define NMETPOLYC       9	/* metal-1-polysilicon contact */
#define NSTRANS        10	/* S-transistor */
#define NDTRANS        11	/* D-transistor */
#define NVIA           12	/* metal-1-metal-2 contact */
#define NWBUT          13	/* metal-1-Well contact */
#define NSBUT          14	/* metal-1-Substrate contact */
#define NMETAL1N       15	/* metal-1 node */
#define NMETAL2N       16	/* metal-2 node */
#define NPOLYN         17	/* polysilicon node */
#define NACTIVEN       18	/* active node */
#define NDACTIVEN      19	/* D-active node */
/* Why no S-Active Node??? */
#define NSELECTPN      20	/* P-select node */
#define NSELECTNN      21	/* N-select node */
#define NCUTN          22	/* cut node */
#define NPCUTN         23	/* poly cut node */
#define NACUTN         24	/* active cut node */
#define NVIAN          25	/* via node */
#define NWELLPN        26	/* P-well node */
#define NWELLNN        27	/* N-well node */
#define NPASSN         28	/* passivation node */
#define NPADFRN        29	/* pad frame node */

/* for geometry */
static INTBIG mocmosold_cutbox[8]  = {LEFTIN1,  BOTIN1,   LEFTIN3,   BOTIN3};/*adjust*/
static INTBIG mocmosold_fullbox[8] = {LEFTEDGE, BOTEDGE,  RIGHTEDGE, TOPEDGE};
static INTBIG mocmosold_min1box[16]= {LEFTIN1,  BOTIN1,   RIGHTIN1,  TOPIN1,
								  CENTERL2, CENTERD2, CENTERR2,  CENTERU2};
static INTBIG mocmosold_in2box[8]  = {LEFTIN2,  BOTIN2,   RIGHTIN2,  TOPIN2};
static INTBIG mocmosold_in4box[8]  = {LEFTIN4,  BOTIN4,   RIGHTIN4,  TOPIN4};
static INTBIG mocmosold_in5box[8]  = {LEFTIN5,  BOTIN5,   RIGHTIN5,  TOPIN5};
static INTBIG mocmosold_min5box[16]= {LEFTIN5,  BOTIN5,   RIGHTIN5,  TOPIN5,
								  CENTERL2, CENTERD2, CENTERR2,  CENTERU2};
static INTBIG mocmosold_trdabox[8] = {LEFTIN4,  BOTIN4,   RIGHTIN4,  TOPIN4};
static INTBIG mocmosold_trdpbox[8] = {LEFTIN2,  BOTIN6,   RIGHTIN2,  TOPIN6};
static INTBIG mocmosold_trd1box[8] = {LEFTIN4,  BOTIN6,   RIGHTIN4,  TOPIN6};
static INTBIG mocmosold_trd2box[8] = {LEFTIN4,  TOPIN5,   RIGHTIN4,  TOPIN4};
static INTBIG mocmosold_trd3box[8] = {LEFTIN4,  BOTIN4,   RIGHTIN4,  BOTIN5};

/* metal-1-pin */
static TECH_PORTS mocmosold_pm1_p[] = {				/* ports */
	{mocmosold_pc_m1, x_("metal-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmosold_pm1_l[] = {			/* layers */
	{LMET1P, 0, 4, CROSSED, BOX, mocmosold_fullbox}};
static TECH_NODES mocmosold_pm1 = {
	x_("Metal-1-Pin"),NMETAL1P,NONODEPROTO,			/* name */
	K4,K4,											/* size */
	1,mocmosold_pm1_p,								/* ports */
	1,mocmosold_pm1_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* metal-2-pin */
static TECH_PORTS mocmosold_pm2_p[] = {				/* ports */
	{mocmosold_pc_m2, x_("metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmosold_pm2_l[] = {			/* layers */
	{LMET2P, 0, 4, CROSSED, BOX, mocmosold_fullbox}};
static TECH_NODES mocmosold_pm2 = {
	x_("Metal-2-Pin"),NMETAL2P,NONODEPROTO,			/* name */
	K4,K4,											/* size */
	1,mocmosold_pm2_p,								/* ports */
	1,mocmosold_pm2_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* polysilicon-pin */
static TECH_PORTS mocmosold_pp_p[] = {				/* ports */
	{mocmosold_pc_p, x_("polysilicon"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON mocmosold_pp_l[] = {			/* layers */
	{LPOLYP, 0, 4, CROSSED, BOX, mocmosold_fullbox}};
static TECH_NODES mocmosold_pp = {
	x_("Polysilicon-Pin"),NPOLYP,NONODEPROTO,		/* name */
	K2,K2,											/* size */
	1,mocmosold_pp_p,								/* ports */
	1,mocmosold_pp_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* S-active-pin */
static TECH_PORTS mocmosold_psa_p[] = {				/* ports */
	{mocmosold_pc_pa, x_("s-active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN5, BOTIN5, RIGHTIN5, TOPIN5}};
static TECH_POLYGON mocmosold_psa_l[] = {			/* layers */
	{LWELLNP,  -1, 4, CROSSED, BOX, mocmosold_fullbox},
	{LSACTP,    0, 4, CROSSED, BOX, mocmosold_in4box},
	{LSELECTPP,-1, 4, CROSSED, BOX, mocmosold_in2box}};
static TECH_NODES mocmosold_psa = {
	x_("S-Active-Pin"),NSACTP,NONODEPROTO,			/* name */
	K10,K10,										/* size */
	1,mocmosold_psa_p,								/* ports */
	3,mocmosold_psa_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* D-active-pin */
static TECH_PORTS mocmosold_pda_p[] = {				/* ports */
	{mocmosold_pc_na, x_("d-active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN5, BOTIN5, RIGHTIN5, TOPIN5}};
static TECH_POLYGON mocmosold_pda_l[] = {			/* layers */
	{LWELLPP,  -1, 4, CROSSED, BOX, mocmosold_fullbox},
	{LDACTP,    0, 4, CROSSED, BOX, mocmosold_in4box},
	{LSELECTNP,-1, 4, CROSSED, BOX, mocmosold_in2box}};
static TECH_NODES mocmosold_pda = {
	x_("D-Active-Pin"),NDACTP,NONODEPROTO,			/* name */
	K10,K10,										/* size */
	1,mocmosold_pda_p,								/* ports */
	3,mocmosold_pda_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* General active-pin */
static TECH_PORTS mocmosold_pa_p[] = {				/* ports */
	{mocmosold_pc_a, x_("active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}}	;
static TECH_POLYGON mocmosold_pa_l[] = {			/* layers */
	{LDACTP, 0, 4, CROSSED, BOX, mocmosold_fullbox},
	{LSACTP, 0, 4, CROSSED, BOX, mocmosold_fullbox}};
static TECH_NODES mocmosold_pa = {
	x_("Active-Pin"),NACTP,NONODEPROTO,				/* name */
	K2,K2,											/* size */
	1,mocmosold_pa_p,								/* ports */
	2,mocmosold_pa_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* metal-1-S-active-contact */
static TECH_PORTS mocmosold_mpa_p[] = {				/* ports */
	{mocmosold_pc_pam1, x_("metal-1-s-act"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN5, BOTIN5, RIGHTIN5, TOPIN5}};
static TECH_POLYGON mocmosold_mpa_l[] = {			/* layers */
	{LMETAL1,  0, 4, FILLEDRECT, MINBOX, mocmosold_min5box},
	{LSACT,    0, 4, FILLEDRECT, BOX,    mocmosold_in4box},
	{LWELLNP, -1, 4, FILLEDRECT, BOX,    mocmosold_fullbox},
	{LSELECTP,-1, 4, FILLEDRECT, BOX,    mocmosold_in2box},
	{LACTCUT,  0, 4, FILLEDRECT, BOX,    mocmosold_cutbox}};
static TECH_NODES mocmosold_mpa = {
	x_("Metal-1-S-Active-Con"),NMETSACTC,NONODEPROTO,	/* name */
	K14,K14,										/* size */
	1,mocmosold_mpa_p,								/* ports */
	5,mocmosold_mpa_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,K2,K2,0,0,0,0};					/* characteristics */

/* metal-1-D-active-contact */
static TECH_PORTS mocmosold_mna_p[] = {				/* ports */
	{mocmosold_pc_nam1, x_("metal-1-d-act"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN5, BOTIN5, RIGHTIN5, TOPIN5}};
static TECH_POLYGON mocmosold_mna_l[] = {			/* layers */
	{LMETAL1,   0, 4, FILLEDRECT, MINBOX, mocmosold_min5box},
	{LDACT,     0, 4, FILLEDRECT, BOX,    mocmosold_in4box},
	{LWELLP,   -1, 4, FILLEDRECT, BOX,    mocmosold_fullbox},
	{LSELECTNP,-1, 4, FILLEDRECT, BOX,    mocmosold_in2box},
	{LACTCUT,   0, 4, FILLEDRECT, BOX,    mocmosold_cutbox}};
static TECH_NODES mocmosold_mna = {
	x_("Metal-1-D-Active-Con"),NMETDACTC,NONODEPROTO,	/* name */
	K14,K14,										/* size */
	1,mocmosold_mna_p,								/* ports */
	5,mocmosold_mna_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,K2,K2,0,0,0,0};					/* characteristics */

/* metal-1-polysilicon-contact */
static TECH_PORTS mocmosold_mp_p[] = {				/* ports */
	{mocmosold_pc_pm1, x_("metal-1-polysilicon"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON mocmosold_mp_l[] = {			/* layers */
	{LMETAL1,  0, 4, FILLEDRECT, MINBOX, mocmosold_min1box},
	{LPOLY,    0, 4, FILLEDRECT, BOX,    mocmosold_fullbox},
	{LPOLYCUT, 0, 4, FILLEDRECT, BOX,    mocmosold_cutbox}};
static TECH_NODES mocmosold_mp = {
	x_("Metal-1-Polysilicon-Con"),NMETPOLYC,NONODEPROTO,/* name */
	K6,K6,											/* size */
	1,mocmosold_mp_p,								/* ports */
	3,mocmosold_mp_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,K2,K2,0,0,0,0};					/* characteristics */

/* S-transistor */
static TECH_PORTS mocmosold_tpa_p[] = {				/* ports */
	{mocmosold_pc_p,  x_("s-trans-poly-left"),  NOPORTPROTO, (180<<PORTANGLESH)|
		(90<<PORTARANGESH),                LEFTIN2,  BOTIN7, LEFTIN3,   TOPIN7},
	{mocmosold_pc_pa, x_("s-trans-diff-top"),   NOPORTPROTO, (90<<PORTANGLESH)|
		(90<<PORTARANGESH)|(1<<PORTNETSH), LEFTIN5,  TOPIN5, RIGHTIN5,  TOPIN4},
	{mocmosold_pc_p,  x_("s-trans-poly-right"), NOPORTPROTO, (0<<PORTANGLESH)|
		(90<<PORTARANGESH),                RIGHTIN3, BOTIN7, RIGHTIN2,  TOPIN7},
	{mocmosold_pc_pa, x_("s-trans-diff-bottom"),NOPORTPROTO, (270<<PORTANGLESH)|
		(90<<PORTARANGESH)|(2<<PORTNETSH), LEFTIN5,  BOTIN4, RIGHTIN5,  BOTIN5}};
static TECH_SERPENT mocmosold_tpa_l[] = {			/* graphical layers */
	{{LSACT,    1, 4, FILLEDRECT, BOX,    mocmosold_trdabox}, K3, K3,  0,  0},
	{{LPOLY,    0, 4, FILLEDRECT, BOX,    mocmosold_trdpbox}, K1, K1, K2, K2},
	{{LWELLNP, -1, 4, FILLEDRECT, BOX,    mocmosold_fullbox}, K7, K7, K4, K4},
	{{LSELECTP,-1, 4, FILLEDRECT, BOX,    mocmosold_in2box},  K5, K5, K2, K2},
	{{LTRANS,  -1, 4, FILLEDRECT, BOX,    mocmosold_trd1box}, K1, K1,  0,  0}};
static TECH_SERPENT mocmosold_tpaE_l[] = {			/* electric layers */
	{{LSACT,    1, 4, FILLEDRECT, BOX,    mocmosold_trd2box}, K3,  0,  0,  0},
	{{LSACT,    3, 4, FILLEDRECT, BOX,    mocmosold_trd3box},  0, K3,  0,  0},
	{{LPOLY,    0, 4, FILLEDRECT, BOX,    mocmosold_trdpbox}, K1, K1, K2, K2},
	{{LWELLNP, -1, 4, FILLEDRECT, BOX,    mocmosold_fullbox}, K7, K7, K4, K4},
	{{LSELECTP,-1, 4, FILLEDRECT, BOX,    mocmosold_in2box},  K5, K5, K2, K2},
	{{LTRANS,  -1, 4, FILLEDRECT, BOX,    mocmosold_trd1box}, K1, K1,  0,  0}};
static TECH_NODES mocmosold_tpa = {
	x_("S-Transistor"),NSTRANS,NONODEPROTO,			/* name */
	K10,K14,										/* size */
	4,mocmosold_tpa_p,								/* ports */
	5,(TECH_POLYGON *)0,							/* layers */
	NODESHRINK|(NPTRAPMOS<<NFUNCTIONSH)|HOLDSTRACE,	/* userbits */
	SERPTRANS,6,K1,K1,K2,K1,K1,mocmosold_tpa_l,mocmosold_tpaE_l};/* characteristics */

/* D-transistor */
static TECH_PORTS mocmosold_tna_p[] = {				/* ports */
	{mocmosold_pc_p,  x_("d-trans-poly-left"),  NOPORTPROTO, (180<<PORTANGLESH)|
		(90<<PORTARANGESH),                 LEFTIN2,  BOTIN7, LEFTIN3,  TOPIN7},
	{mocmosold_pc_na, x_("d-trans-diff-top"),   NOPORTPROTO, (90<<PORTANGLESH)|
		(90<<PORTARANGESH)|(1<<PORTNETSH),  LEFTIN5,  TOPIN5, RIGHTIN5, TOPIN4},
	{mocmosold_pc_p,  x_("d-trans-poly-right"), NOPORTPROTO, (0<<PORTANGLESH)|
		(90<<PORTARANGESH),                 RIGHTIN3, BOTIN7, RIGHTIN2, TOPIN7},
	{mocmosold_pc_na, x_("d-trans-diff-bottom"),NOPORTPROTO, (270<<PORTANGLESH)|
		(90<<PORTARANGESH)|(2<<PORTNETSH), LEFTIN5,  BOTIN4, RIGHTIN5, BOTIN5}};
static TECH_SERPENT mocmosold_tna_l[] = {			/* graphical layers */
	{{LDACT,     1, 4, FILLEDRECT, BOX,    mocmosold_trdabox}, K3, K3,  0,  0},
	{{LPOLY,     0, 4, FILLEDRECT, BOX,    mocmosold_trdpbox}, K1, K1, K2, K2},
	{{LWELLP,   -1, 4, FILLEDRECT, BOX,    mocmosold_fullbox}, K7, K7, K4, K4},
	{{LSELECTNP,-1, 4, FILLEDRECT, BOX,    mocmosold_in2box},  K5, K5, K2, K2},
	{{LTRANS,   -1, 4, FILLEDRECT, BOX,    mocmosold_trd1box}, K1, K1,  0,  0}};
static TECH_SERPENT mocmosold_tnaE_l[] = {			/* electric layers */
	{{LDACT,     1, 4, FILLEDRECT, BOX,    mocmosold_trd2box}, K3,  0,  0,  0},
	{{LDACT,     3, 4, FILLEDRECT, BOX,    mocmosold_trd3box},  0, K3,  0,  0},
	{{LPOLY,     0, 4, FILLEDRECT, BOX,    mocmosold_trdpbox}, K1, K1, K2, K2},
	{{LWELLP,   -1, 4, FILLEDRECT, BOX,    mocmosold_fullbox}, K7, K7, K4, K4},
	{{LSELECTNP,-1, 4, FILLEDRECT, BOX,    mocmosold_in2box},  K5, K5, K2, K2},
	{{LTRANS,   -1, 4, FILLEDRECT, BOX,    mocmosold_trd1box}, K1, K1,  0,  0}};
static TECH_NODES mocmosold_tna = {
	x_("D-Transistor"),NDTRANS,NONODEPROTO,			/* name */
	K10,K14,										/* size */
	4,mocmosold_tna_p,								/* ports */
	5,(TECH_POLYGON *)0,							/* layers */
	NODESHRINK|(NPTRANMOS<<NFUNCTIONSH)|HOLDSTRACE,	/* userbits */
	SERPTRANS,6,K1,K1,K2,K1,K1,mocmosold_tna_l,mocmosold_tnaE_l};/* characteristics */

/* metal-1-metal-2-contact */
static TECH_PORTS mocmosold_mm_p[] = {				/* ports */
	{mocmosold_pc_mm, x_("metal-1-metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmosold_mm_l[] = {			/* layers */
	{LMETAL1, 0, 4, FILLEDRECT, BOX, mocmosold_fullbox},
	{LMETAL2, 0, 4, FILLEDRECT, BOX, mocmosold_fullbox},
	{LVIA,    0, 4, CLOSEDRECT, BOX, mocmosold_cutbox}};
static TECH_NODES mocmosold_mm = {
	x_("Metal-1-Metal-2-Con"),NVIA,NONODEPROTO,		/* name */
	K4,K4,											/* size */
	1,mocmosold_mm_p,								/* ports */
	3,mocmosold_mm_l,								/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,K1,K3,0,0,0,0};					/* characteristics */

/* Metal-1-Well Contact */
static TECH_PORTS mocmosold_psub_p[] = {			/* ports */
	{mocmosold_pc_m1a, x_("metal-1-well"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN5H, BOTIN5H, RIGHTIN5H, TOPIN5H}};
static TECH_POLYGON mocmosold_psub_l[] = {			/* layers */
	{LMETAL1,   0, 4, FILLEDRECT, MINBOX, mocmosold_min5box},
	{LSACTWELL,-1, 4, FILLEDRECT, BOX,    mocmosold_in4box},
	{LWELLP,   -1, 4, FILLEDRECT, BOX,    mocmosold_fullbox},
	{LSELECTP,  0, 4, FILLEDRECT, MINBOX, mocmosold_min5box},
	{LACTCUT,   0, 4, FILLEDRECT, BOX,    mocmosold_cutbox}};
static TECH_NODES mocmosold_psub = {
	x_("Metal-1-Well-Con"),NWBUT,NONODEPROTO,		/* name */
	K14,K14,										/* size */
	1,mocmosold_psub_p,								/* ports */
	5,mocmosold_psub_l,								/* layers */
	(NPWELL<<NFUNCTIONSH),							/* userbits */
	MULTICUT,K2,K2,K2,K2,0,0,0,0};					/* characteristics */

/* Metal-1-Substrate Contact */
static TECH_PORTS mocmosold_nsub_p[] = {			/* ports */
	{mocmosold_pc_m1a, x_("metal-1-substrate"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN5H, BOTIN5H, RIGHTIN5H, TOPIN5H}};
static TECH_POLYGON mocmosold_nsub_l[] = {			/* layers */
	{LMETAL1,   0, 4, FILLEDRECT, MINBOX, mocmosold_min5box},
	{LDACT,     0, 4, FILLEDRECT, BOX,    mocmosold_in4box},
	{LWELLNP,  -1, 4, FILLEDRECT, BOX,    mocmosold_fullbox},
	{LSELECTNP, 0, 4, FILLEDRECT, BOX,    mocmosold_in5box},
	{LACTCUT,   0, 4, FILLEDRECT, BOX,    mocmosold_cutbox}};
static TECH_NODES mocmosold_nsub = {
	x_("Metal-1-Substrate-Con"),NSBUT,NONODEPROTO,	/* name */
	K14,K14,										/* size */
	1,mocmosold_nsub_p,								/* ports */
	5,mocmosold_nsub_l,								/* layers */
	(NPSUBSTRATE<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,K2,K2,0,0,0,0};					/* characteristics */

/* Metal-1-Node */
static TECH_PORTS mocmosold_m1_p[] = {				/* ports */
	{mocmosold_pc_m1, x_("metal-1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmosold_m1_l[] = {			/* layers */
	{LMETAL1, 0, 4, FILLEDRECT, BOX, mocmosold_fullbox}};
static TECH_NODES mocmosold_m1 = {
	x_("Metal-1-Node"),NMETAL1N,NONODEPROTO,		/* name */
	K4,K4,											/* size */
	1,mocmosold_m1_p,								/* ports */
	1,mocmosold_m1_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Metal-2-Node */
static TECH_PORTS mocmosold_m2_p[] = {				/* ports */
	{mocmosold_pc_m2, x_("metal-2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON mocmosold_m2_l[] = {			/* layers */
	{LMETAL2, 0, 4, FILLEDRECT, BOX, mocmosold_fullbox}};
static TECH_NODES mocmosold_m2 = {
	x_("Metal-2-Node"),NMETAL2N,NONODEPROTO,		/* name */
	K4,K4,											/* size */
	1,mocmosold_m2_p,								/* ports */
	1,mocmosold_m2_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Polysilicon-Node */
static TECH_PORTS mocmosold_p_p[] = {				/* ports */
	{mocmosold_pc_p, x_("polysilicon"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON mocmosold_p_l[] = {				/* layers */
	{LPOLY, 0, 4, FILLEDRECT, BOX, mocmosold_fullbox}};
static TECH_NODES mocmosold_p = {
	x_("Polysilicon-Node"),NPOLYN,NONODEPROTO,		/* name */
	K4,K4,											/* size */
	1,mocmosold_p_p,								/* ports */
	1,mocmosold_p_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Active-Node */
static TECH_PORTS mocmosold_a_p[] = {				/* ports */
	{mocmosold_pc_a, x_("active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON mocmosold_a_l[] = {				/* layers */
	{LSACT, 0, 4, FILLEDRECT, BOX, mocmosold_fullbox}};
static TECH_NODES mocmosold_a = {
	x_("Active-Node"),NACTIVEN,NONODEPROTO,			/* name */
	K4,K4,											/* size */
	1,mocmosold_a_p,								/* ports */
	1,mocmosold_a_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* D-Active-Node */
static TECH_PORTS mocmosold_da_p[] = {				/* ports */
	{mocmosold_pc_a, x_("active"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON mocmosold_da_l[] = {			/* layers */
	{LDACT, 0, 4, FILLEDRECT, BOX, mocmosold_fullbox}};
static TECH_NODES mocmosold_da = {
	x_("D-Active-Node"),NDACTIVEN,NONODEPROTO,		/* name */
	K4,K4,											/* size */
	1,mocmosold_da_p,								/* ports */
	1,mocmosold_da_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* P-Select-Node */
static TECH_PORTS mocmosold_sp_p[] = {				/* ports */
	{mocmosold_pc_null, x_("select"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmosold_sp_l[] = {			/* layers */
	{LSELECTP, 0, 4, FILLEDRECT, BOX, mocmosold_fullbox}};
static TECH_NODES mocmosold_sp = {
	x_("P-Select-Node"),NSELECTPN,NONODEPROTO,		/* name */
	K6,K6,											/* size */
	1,mocmosold_sp_p,								/* ports */
	1,mocmosold_sp_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* N-Select-Node */
static TECH_PORTS mocmosold_sn_p[] = {				/* ports */
	{mocmosold_pc_null, x_("select"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmosold_sn_l[] = {			/* layers */
	{LSELECTNP, 0, 4, FILLEDRECT, BOX, mocmosold_fullbox}};
static TECH_NODES mocmosold_sn = {
	x_("N-Select-Node"),NSELECTNN,NONODEPROTO,		/* name */
	K6,K6,											/* size */
	1,mocmosold_sn_p,								/* ports */
	1,mocmosold_sn_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Cut-Node */
static TECH_PORTS mocmosold_c_p[] = {				/* ports */
	{mocmosold_pc_null, x_("cut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmosold_c_l[] = {				/* layers */
	{LCUT, 0, 4, FILLEDRECT, BOX, mocmosold_fullbox}};
static TECH_NODES mocmosold_c = {
	x_("Cut-Node"),NCUTN,NONODEPROTO,				/* name */
	K2,K2,											/* size */
	1,mocmosold_c_p,								/* ports */
	1,mocmosold_c_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* PolyCut-Node */
static TECH_PORTS mocmosold_gc_p[] = {				/* ports */
	{mocmosold_pc_null, x_("polycut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmosold_gc_l[] = {			/* layers */
	{LPOLYCUT, 0, 4, FILLEDRECT, BOX, mocmosold_fullbox}};
static TECH_NODES mocmosold_gc = {
	x_("Poly-Cut-Node"),NPCUTN,NONODEPROTO,			/* name */
	K2,K2,											/* size */
	1,mocmosold_gc_p,								/* ports */
	1,mocmosold_gc_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* ActiveCut-Node */
static TECH_PORTS mocmosold_ac_p[] = {				/* ports */
	{mocmosold_pc_null, x_("activecut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmosold_ac_l[] = {			/* layers */
	{LACTCUT, 0, 4, FILLEDRECT, BOX, mocmosold_fullbox}};
static TECH_NODES mocmosold_ac = {
	x_("Active-Cut-Node"),NACUTN,NONODEPROTO,		/* name */
	K2,K2,											/* size */
	1,mocmosold_ac_p,								/* ports */
	1,mocmosold_ac_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Via-Node */
static TECH_PORTS mocmosold_v_p[] = {				/* ports */
	{mocmosold_pc_null, x_("via"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmosold_v_l[] = {				/* layers */
	{LVIA, 0, 4, CLOSEDRECT, BOX, mocmosold_fullbox}};
static TECH_NODES mocmosold_v = {
	x_("Via-Node"),NVIAN,NONODEPROTO,				/* name */
	K2,K2,											/* size */
	1,mocmosold_v_p,								/* ports */
	1,mocmosold_v_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* P-Well-Node */
static TECH_PORTS mocmosold_wp_p[] = {				/* ports */
	{mocmosold_pc_pa, x_("well"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3}};
static TECH_POLYGON mocmosold_wp_l[] = {			/* layers */
	{LWELLP, 0, 4, FILLEDRECT, BOX, mocmosold_fullbox}};
static TECH_NODES mocmosold_wp = {
	x_("P-Well-Node"),NWELLPN,NONODEPROTO,			/* name */
	K6,K6,											/* size */
	1,mocmosold_wp_p,								/* ports */
	1,mocmosold_wp_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* N-Well-Node */
static TECH_PORTS mocmosold_wn_p[] = {				/* ports */
	{mocmosold_pc_pa, x_("well"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN3, BOTIN3, RIGHTIN3, TOPIN3}};
static TECH_POLYGON mocmosold_wn_l[] = {			/* layers */
	{LWELLNP, 0, 4, FILLEDRECT, BOX, mocmosold_fullbox}};
static TECH_NODES mocmosold_wn = {
	x_("N-Well-Node"),NWELLNN,NONODEPROTO,			/* name */
	K6,K6,											/* size */
	1,mocmosold_wn_p,								/* ports */
	1,mocmosold_wn_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Passivation-node */
static TECH_PORTS mocmosold_o_p[] = {				/* ports */
	{mocmosold_pc_null, x_("passivation"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmosold_o_l[] = {				/* layers */
	{LPASS, 0, 4, FILLEDRECT, BOX, mocmosold_fullbox}};
static TECH_NODES mocmosold_o = {
	x_("Passivation-Node"),NPASSN,NONODEPROTO,		/* name */
	K8,K8,											/* size */
	1,mocmosold_o_p,								/* ports */
	1,mocmosold_o_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Pad-Frame-node */
static TECH_PORTS mocmosold_pf_p[] = {				/* ports */
	{mocmosold_pc_null, x_("pad-frame"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON mocmosold_pf_l[] = {			/* layers */
	{LFRAME, 0, 4, CLOSEDRECT, BOX, mocmosold_fullbox}};
static TECH_NODES mocmosold_pf = {
	x_("Pad-Frame-Node"),NPADFRN,NONODEPROTO,		/* name */
	K8,K8,											/* size */
	1,mocmosold_pf_p,								/* ports */
	1,mocmosold_pf_l,								/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

TECH_NODES *mocmosold_nodeprotos[NODEPROTOCOUNT+1] = {
	&mocmosold_pm1, &mocmosold_pm2, &mocmosold_pp, &mocmosold_psa, &mocmosold_pda,
	&mocmosold_pa,
	&mocmosold_mpa, &mocmosold_mna, &mocmosold_mp,
	&mocmosold_tpa, &mocmosold_tna,
	&mocmosold_mm,
	&mocmosold_psub, &mocmosold_nsub,
	&mocmosold_m1, &mocmosold_m2, &mocmosold_p, &mocmosold_a, &mocmosold_da, &mocmosold_sp,
	&mocmosold_sn, &mocmosold_c, &mocmosold_gc, &mocmosold_ac,
	&mocmosold_v, &mocmosold_wp, &mocmosold_wn, &mocmosold_o, &mocmosold_pf, ((TECH_NODES *)-1)};

/* this table must correspond with the above table */
static INTBIG mocmosold_node_widoff[NODEPROTOCOUNT*4] = {
	0,0,0,0, 0,0,0,0, 0,0,0,0, K4,K4,K4,K4, K4,K4,K4,K4,		/* pins */
	0,0,0,0,
	K4,K4,K4,K4, K4,K4,K4,K4, 0,0,0,0,							/* contacts */
	K4,K4,K6,K6, K4,K4,K6,K6,									/* trans. */
	0,0,0,0,													/* vias */
	K4,K4,K4,K4, K4,K4,K4,K4,									/* buttons */
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,		/* nodes */
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

/*
 * tables for converting between stipples and outlines
 */
typedef struct
{
	TECH_POLYGON *str;
	INTBIG        layer;
} STIPPCONV;

static STIPPCONV mocmosold_nodepstipple[] =
{
	{mocmosold_mpa_l, 3},	/* select-p */
	{mocmosold_mna_l, 3},	/* select-n */
	{mocmosold_psub_l, 3},	/* select-p */
	{mocmosold_nsub_l, 3},	/* select-n */
	{mocmosold_sp_l, 0},	/* select-p */
	{mocmosold_sn_l, 0},	/* select-n */
	{mocmosold_o_l, 0},		/* passivation */
	{mocmosold_mpa_l, 2},	/* well-n */
	{mocmosold_mna_l, 2},	/* well-p */
	{mocmosold_psub_l, 2},	/* well-p */
	{mocmosold_nsub_l, 2},	/* well-n */
	{mocmosold_wp_l, 2},	/* well-p */
	{mocmosold_wn_l, 2},	/* well-n */
	{mocmosold_mm_l, 1},	/* metal-2 */
	{mocmosold_m2_l, 0}		/* metal-2 */
};
static INTBIG mocmosold_nodepstipplesize = 7;

typedef struct
{
	TECH_SERPENT *str;
	INTBIG        layer;
} STIPSCONV;

static STIPSCONV mocmosold_nodesstipple[] =
{
	{mocmosold_tpa_l, 3},	/* select-p */
	{mocmosold_tna_l, 3},	/* select-n */
	{mocmosold_tpa_l, 2},	/* well-n */
	{mocmosold_tna_l, 2},	/* well-p */
};
static INTBIG mocmosold_nodesstipplesize = 2;

/******************** SIMULATION VARIABLES ********************/

/* for SPICE simulation */
#define mocmosold_MIN_RESIST	50.0f	/* minimum resistance consider */
#define mocmosold_MIN_CAPAC	0.04f	/* minimum capacitance consider */
static float mocmosold_sim_spice_resistance[MAXLAYERS] = {  /* per square micron */
	0.03f /* METAL1 */,    0.03f /* METAL2 */,    50.0f /* POLY */,
	10.0f /* SACT */,      10.0f /* DACT */,
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0};
static float mocmosold_sim_spice_capacitance[MAXLAYERS] = { /* per square micron */
	0.03f /* METAL1 */,    0.03f /* METAL2 */,    0.04f /* POLY */,
	0.1f /* SACT */,       0.1f /* DACT */,
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0};
static CHAR *mocmosold_sim_spice_header_level1[] = {
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
static CHAR *mocmosold_sim_spice_header_level2[] = {
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

/******************** ECAD DRC VARIABLES ********************/

/*
 * this ECAD design-rule file was written by Rick Schediwy at SPAR
 * the two lines that read " PRIMARY =" and " INDISK =" are modified
 * to include the file name with the CIF.  This design-rule set assumes
 * that the CIF layer called "DRC" is a cloak (generated by the DRC-Node
 * in the Generic technology) which causes layout under it to be ignored.
 * Errors are returned on layer "CX".  In order to do this, there must be
 * a "CIF-REF-FILE" in the specified location.  This file contains the
 * single line:
 *     .CIF 20 CX
 */
static CHAR *mocmosold_drc_ecad_deck[] = {
	x_("; MOSIS REV 6 'EITHER' CMOS DESIGN RULE FILE"),
	x_("; THIS COMMAND FILE CONSISTS OF COMMANDS FOR BOTH PWELL PROCESS AND NWELL PROCESS."),
	x_("; Last modified:  7-Jul-90  KSS"),
	x_("*DESCRIPTION"),
	x_(" MODE = EXEC NOW"),
	x_(" SYSTEM = CIF"),
	x_(" SCALE = 0.01 MIC"),
	x_(" RESOLUTION = 0.25 MIC     ;ECADs internal resolution"),
	x_(" FLAGNON45 = NO"),
	x_(" PRINTFILE = ECADTST"),
	x_(" PRIMARY ="),			/* this has a name added to it */
	x_(" INDISK ="),			/* this has a name added to it */
	x_(" OUTDISK = errors.cif      ;name of cif errors file to be created"),
	x_(" CIF-REF-FILE = CIFREF"),
	x_(" KEEPDATA = NO             ;YES if you want .DAT files etc for LAYDE"),
	x_(" CNAMES-CSEN = YES         ;ECAD is now case sensitive"),
	x_(" CHECK-MODE = FLAT         ;HIER or FLAT (must be FLAT if layout flat)"),
	x_(" STATUS-COMMAND = \"time\"   ; comment this out in VM/CS"),
	x_("*END"),
	x_("*INPUT-LAYER"),
	x_(" ACTIVA  =  CAA     ;CAA"),
	x_(" POLYA   =  CPG     ;CPG"),
	x_(" PLYCNT  =  CCP     ;CCP"),
	x_(" ACTCNT  =  CCA     ;CCA"),
	x_(" GENCNT  =  CC      ;CC"),
	x_(" MT1     =  CMF     ;CMF"),
	x_(" VIA     =  CVA     ;CVA"),
	x_(" MT2     =  CMS     ;CMS"),
	x_(" VAP     =  COG     ;COG"),
	x_(" CLOAK   =  DRC     ;ERROR CLOAKING LAYER"),
	x_(";   PWELL PROCESS"),
	x_(" PWELL    =  CWP     ;CWP"),
	x_(" PSELECT  =  CSP     ;CSP"),
	x_(";   NWELL PROCESS"),
	x_(" NWELL   =  CWN     ;CWN"),
	x_(" NSELECT =  CSN     ;CSN"),
	x_(" SUBSTRATE = BULK 60"),
	x_("*END"),
	x_("*OPERATION"),
	x_("; ** SORT OUT CONTACTS **"),
	x_(" AND  ACTIVA GENCNT  TMPACT     ;FIND ALL CONTACTS TO ACTIVE"),
	x_(" OR   TMPACT ACTCNT  ACTCT"),
	x_(" AND  POLYA  GENCNT  TMPPLY     ;FIND ALL CONTACTS TO POLY"),
	x_(" OR   TMPPLY PLYCNT PLYCT"),
	x_(" OR   PLYCNT ACTCNT  TMPCNT     ;FIND ALL CONTACTS"),
	x_(" OR   TMPCNT GENCNT  CNTACT"),
	x_("; ** MAKE SURE CONTACTS ARE 2X2 **"),
	x_("  WIDTH CNTACT  LT 2.0                OUT G000 20 ;CONTACT SIZE EXACTLY"),
	x_("  AREA  CNTACT  RANGE 4 40000   BGCNT OUT G010 20"),
	x_("; ** SOME LAYER DEFINITIONS **"),
	x_(";   (DETERMINATION OF ACTIVE AND WELL TYPES DEPENDS ON PROCESS)"),
	x_(";    SAME FOR EITHER PROCESS"),
	x_(" AND  POLY   ACTIVE  GATE      ;GATE REGIONS"),
	x_(" NOT  POLYA  GATE    POLYNG    ;NON GATE POLY"),
	x_(" OR   POLYNG ACTIVA  CONLYR    ;VALID LAYERS TO CONTACTS"),
	x_("; ** BLOAT 4X4 CONTACT STRUCTURES **"),
	x_(" SIZE  CNTACT  BY 1.0  SZCNT   ;POLY STRUCTURES"),
	x_(" AND   SZCNT   POLYA   POLYB"),
	x_(" SIZE  POLYB   BY 0.5  POLYC"),
	x_(" OR    POLYA   POLYC   POLY    ; Bloated Poly"),
	x_(" AND   SZCNT   ACTIVA  ACTIVB  ;ACTIVE STRUCTURES"),
	x_(" SIZE  ACTIVB  BY 0.5  ACTIVC"),
	x_(" OR    ACTIVA  ACTIVC  ACTIVE  ; Bloated active"),
	x_(";    P-WELL PROCESS"),
	x_(" AND  ACTIVA PSELECT PPLUSP    ;P+ ACTIVE"),
	x_(" NOT  ACTIVA PSELECT NPLUSP    ;N+ ACTIVE"),
	x_(" AND  BULK   PWELL   PWEL      ;P-WELL"),
	x_(" NOT  BULK   PWELL   NSUB      ;N-SUBSTRATE"),
	x_(" NOT  PPLUSP GATE    PTMPP     "),
	x_(" AND  PTMPP  NSUB    PSDP      ;PTRANS SOURCES & DRAINS"),
	x_(" AND  PPLUSP PWEL    PCLMPP    ;WELL CLAMPS"),
	x_(" NOT  NPLUSP GATE    NTMPP"),
	x_(" AND  NTMPP  PWEL    NSDP      ;NTRANS SOURCES & DRAINS"),
	x_(" AND  NPLUSP NSUB    NCLMPP    ;SUBSTRATE CLAMPS"),
	x_(";    N-WELL PROCESS"),
	x_(" AND  ACTIVA NSELECT NPLUSN    ;N+ ACTIVE"),
	x_(" NOT  ACTIVA NSELECT PPLUSN    ;P+ ACTIVE"),
	x_(" AND  BULK   NWELL   NWEL      ;N-WELL"),
	x_(" NOT  BULK   NWELL   PSUB      ;P-SUBSTRATE"),
	x_(" NOT  PPLUSN GATE    PTMPN     "),
	x_(" AND  PTMPN  NWEL    PSDN      ;PTRANS SOURCES & DRAINS"),
	x_(" AND  PPLUSN PSUB    PCLMPN    ;SUBSTRATE CLAMPS"),
	x_(" NOT  NPLUSN GATE    NTMPN"),
	x_(" AND  NTMPN  PSUB    NSDN      ;NTRANS SOURCES & DRAINS"),
	x_(" AND  NPLUSN NWEL    NCLMPN    ;WELL CLAMPS"),
	x_("*BREAK BRK1"),
	x_("; ** GENERIC CONTACT CHECK **"),
	x_(" ENC[TO] CNTACT    CONLYR LT 1.0   OUT G020 20 ;POLY OR ACT OVERLAP OF CONT"),
	x_(" EXT     CNTACT    LT 3.0          OUT G030 20 ;SPACING TO OTHER CNTACT"),
	x_(" EXT[TO] ACTCT     PLYCT  LT 4.0   OUT G035 20 ;POLY CON TO ACT CON SPACE"),
	x_(" EXT[TO] CNTACT    GATE   LT 2.0   OUT G040 20 ;SPACING TO GATE"),
	x_(" NOT     CNTACT    CONLYR  ERRCN1  OUT G050 20 ;MAKE SURE THERE IS POLY OR ACT SURROUND"),
	x_(";    N-WELL PROCESS"),
	x_(" EXT[TO]  ACTCT    NSELECT LT 1.0  OUT N055 20 ;SPACING TO NSELECT"),
	x_(" ENC      ACTCT    NSELECT LT 1.0  OUT N056 20 ;NSELECT SURROUND OF CONTACT"),
	x_(";    P-WELL PROCESS"),
	x_(" EXT[TO]  ACTCT   PSELECT LT 1.0  OUT P055 20 ;SPACING TO PSELECT"),
	x_(" ENC      ACTCT   PSELECT LT 1.0  OUT P056 20 ;PSELECT SURROUND OF CONTACT"),
	x_("; ** WELL CHECK **"),
	x_(";    P-WELL PROCESS"),
	x_(" WIDTH   PWELL  LT 10.0 OUT P057 20"),
	x_(" EXT     PWELL  LT 6.0  OUT P058 20     ;ASSUMES WELLS ALWAYS AT SAME POTENTIAL"),
	x_(";    N-WELL PROCESS"),
	x_(" WIDTH   NWELL  LT 10.0 OUT N057 20"),
	x_(" EXT     NWELL  LT 6.0  OUT N058 20     ;ASSUMES WELLS ALWAYS AT SAME POTENTIAL"),
	x_("; ** ACTIVE CHECK  **"),
	x_(";    SAME FOR EITHER PROCESS"),
	x_(" WIDTH    ACTIVE LT 3.0       OUT G060 20    ;ACTIVE WIDTH"),
	x_(" EXT      ACTIVE LT 3.0       OUT G070 20    ;ACTIVE SPACING"),
	x_(" EXT[R]   GATE   LT 3.0       SMGSP          ;GENERATE GATE-GATE SPACING LAYER FOR LATER USE IN SRC/DRAIN CHECKS"),
	x_(";    P-WELL PROCESS"),
	x_(" ENC[T]  NPLUSP PWEL   LT 5.0   OUT P080 20    ;SOURCE/DRAIN ACTIVE TO WELL"),
	x_(" ENC[T]  PCLMPP PWEL   LT 3.0   OUT P085 20    ;WELL CLAMP TO WELL"),
	x_(" ENC[T]  PPLUSP NSUB   LT 5.0   OUT P090 20    ;SOURCE/DRAIN ACTIVE TO WELL"),
	x_(" ENC[T]  NCLMPP NSUB   LT 3.0   OUT P095 20    ;SUBSTRATE CLAMP TO WELL"),
	x_(" WIDTH[R] PSDP      LT 3.0      SMPSDP         ;ALL P SRC/DRAINS WIDTH < 3"),
	x_(" NOT      SMPSDP    SMGSP       POSPERP        ;WHICH ARE NOT BETWEEN GATES "),
	x_(" WIDTH    POSPERP   LT 3.0      OUT P100 20    ;ARE ERRORS"),
	x_(" WIDTH[R] NSDP      LT 3.0      SMNSDP         ;ALL N SRC/DRAINS WIDTH < 3"),
	x_(" NOT      SMNSDP    SMGSP       POSNERP        ;WHICH ARE NOT BETWEEN GATES"),
	x_(" WIDTH    POSNERP   LT 3.0      OUT P110 20    ;ARE ERRORS"),
	x_(";    N-WELL PROCESS"),
	x_(" ENC[T]  PPLUSN NWEL   LT 5.0   OUT N080 20    ;SOURCE/DRAIN ACTIVE TO WELL"),
	x_(" ENC[T]  NCLMPN NWEL   LT 3.0   OUT N085 20    ;WELL CLAMP TO WELL"),
	x_(" ENC[T]  NPLUSN PSUB   LT 5.0   OUT N090 20    ;SOURCE/DRAIN ACTIVE TO WELL"),
	x_(" ENC[T]  PCLMPN PSUB   LT 3.0   OUT N095 20    ;SUBSTRATE CLAMP TO WELL"),
	x_(" WIDTH[R] PSDN      LT 3.0      SMPSDN         ;ALL P SRC/DRAINS WIDTH < 3"),
	x_(" NOT      SMPSDN    SMGSP       POSPERN        ;WHICH ARE NOT BETWEEN GATES "),
	x_(" WIDTH    POSPERN   LT 3.0      OUT N100 20    ;ARE ERRORS"),
	x_(" WIDTH[R] NSDN      LT 3.0      SMNSDN         ;ALL N SRC/DRAINS WIDTH < 3"),
	x_(" NOT      SMNSDN    SMGSP       POSNERN        ;WHICH ARE NOT BETWEEN GATES"),
	x_(" WIDTH    POSNERN   LT 3.0      OUT N110 20    ;ARE ERRORS"),
	x_("; ** GATE CHECK **"),
	x_(";    SAME FOR EITHER PROCESS"),
	x_(" WIDTH[P]  GATE  LT 2.0           OUT G120 20 ;MIN XTOR WIDTH"),
	x_(" ENC[T]    GATE  ACTIVE   LT 0.001 &"),
	x_(" ENC[TO]   GATE  POLY     LT 2.0  OUT G160 20 ;GATE OVERLAP OF ACTIVE"),
	x_(" EXT[TOC]  GATE  PSELECT  LT 3.0  OUT P150 20 ;PSELECT AWAY FROM GATES"),
	x_(" EXT[TOC]  GATE  NSELECT  LT 3.0  OUT N150 20 ;NSELECT AWAY FROM GATES"),
	x_(";    P-WELL PROCESS"),
	x_(" EXT[TP]   GATE  PSDP     LT 0.001 &    ;CHECK PSELECT SURROUND OF PTRANS GATES"),
	x_(" ENC[TOCP] GATE  PSELECT  LT 3.0  OUT P130 20"),
	x_(";    N-WELL PROCESS"),
	x_(" EXT[TP]   GATE  NSDN     LT 0.001 &    ;CHECK NSELECT SURROUND OF PTRANS GATES"),
	x_(" ENC[TOCP] GATE  NSELECT  LT 3.0  OUT N130 20"),
	x_("; **  SELECT  CHECKS"),
	x_(";     P-WELL PROCESS"),
	x_(" ENC     PSDP    PSELECT  LT 2.0  OUT P200 20 ;PSELECT OVERLAP OF ACTIVE "),
	x_(" EXT     NSDP    PSELECT  LT 2.0  OUT P210 20 ;PSELECT SPACE TO ACTIVE"),
	x_(";     N-WELL PROCESS"),
	x_(" ENC     NSDN    NSELECT  LT 2.0  OUT N200 20 ;NSELECT OVERLAP OF ACTIVE"),
	x_(" EXT     PSDN    NSELECT  LT 2.0  OUT N210 20 ;NSELECT SPACE TO ACTIVE"),
	x_("; **  POLY CHECK  **"),
	x_(" WIDTH   POLY   LT 2.0          OUT G170 20   ;WIDTH"),
	x_(" EXT     POLY   LT 2.0          OUT G180 20   ;SPACING"),
	x_(" EXT[T]  POLYA  ACTIVA  LT  1.0 OUT G190 20   ;POLY - ACTIVE SPACING"),
	x_(" NOT     POLY   CLOAK      CLPOLY             ;FIELD POLY TO ACTIVE"),
	x_(" NOT     ACTIVE CLOAK      CLACT              ;USER CAN CLOAK ERRORS"),
	x_(" EXT[T]  CLPOLY CLACT   LT  1.0 OUT G195 20"),
	x_("; ** METAL1 CHECK **"),
	x_(" WIDTH   MT1     LT 3.0         OUT G230 20 ;WIDTH"),
	x_(" EXT     MT1     LT 3.0         OUT G240 20 ;SPACING"),
	x_(" NOT     CNTACT  MT1  ERRCN2    OUT G250 20 ;METAL OVER CONTACT ?"),
	x_(" ENC[TO] CNTACT  MT1    LT 1.0  OUT G260 20 ;OVERLAP OF CONTACT"),
	x_("; ** METAL2 CHECK **"),
	x_(" WIDTH   MT2     LT 3.0         OUT G350 20  ;WIDTH"),
	x_(" EXT     MT2     LT 4.0         OUT G360 20  ;SPACE"),
	x_(" ENC[TO] VIA     MT2    LT 1.0  OUT G370 20  ;OVERLAP OF VIA"),
	x_("; **  VIA CHECK **"),
	x_(" WIDTH   VIA     LT 2.0                 OUT G270 20  ;VIA SIZE EXACTLY"),
	x_(" AREA    VIA     RANGE 4 40000   BGVIA  OUT G280 20"),
	x_("; WIDTH   VIA     SELGT  2.0 BGCNTV          ;THIS FAILS SINCE WILL PASS"),
	x_("; AND     BGCNTV  BULK   ERRCV  OUT D280 20  ;ONLY IF ALL DIMENSIONS GT 2.0"),
	x_(" EXT     VIA     LT 3.0         OUT G290 20  ;SEPARATION TO VIA"),
	x_(" ENC[TO] VIA     MT1    LT 1.0  OUT G300 20  ;OVERLAP BY METAL1"),
	x_(" EXT[T]  VIA     POLY   LT 2.0  OUT G310 20  ;SPACE TO POLY"),
	x_(" ENC     VIA     POLY   LT 2.0  OUT G315 20  ;POLY OVERLAP"),
	x_(" EXT[T]  VIA     ACTIVE LT 2.0  OUT G320 20  ;SPACE TO ACTIVE"),
	x_(" ENC     VIA     ACTIVE LT 2.0  OUT G335 20  ;ACTIVE OVERLAP"),
	x_(" EXT[T]  VIA     CNTACT LT 2.0  OUT G330 20  ;SPACING TO CONTACT"),
	x_(" AND     VIA     CNTACT ERRVP   OUT G340 20  ;VIA STACKED OVER CONTACT"),
	x_("; **  OVERGLASS CHECK **"),
	x_("; FLATTEN VAP     FVAP"),
	x_("; WIDTH   FVAP    LT 75.0        OUT G380 20  ;BONDING PAD WIDTH"),
	x_("; SIZE    VAP  BY 6.0    SVAP"),
	x_("; AND     SVAP    MT2    MT2VAP"),
	x_("; FLATTEN MT2VAP  FMT2V"),
	x_("; ENC[T]  FVAP    FMT2V  LT 6.0  OUT G390 20  ;PAD OVERLAP OF GLASS"),
	x_("; NOT     VAP     MT2    VAPERR  OUT G400 20  ;METAL2 UNDER GLASS"),
	x_("*END"),
	NOSTRING};

/******************** VARIABLE AGGREGATION ********************/

TECH_VARIABLES mocmosold_variables[] =
{
	/* set general information about the technology */
	{x_("TECH_layer_names"), (CHAR *)mocmosold_layer_names, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_layer_function"), (CHAR *)mocmosold_layer_function, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_arc_width_offset"), (CHAR *)mocmosold_arc_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|(ARCPROTOCOUNT<<VLENGTHSH)},
	{x_("TECH_node_width_offset"), (CHAR *)mocmosold_node_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|((NODEPROTOCOUNT*4)<<VLENGTHSH)},

	/* set information for the USER tool */
	{x_("USER_color_map"), (CHAR *)mocmosold_colmap, 0.0,
		VCHAR|VDONTSAVE|VISARRAY|((sizeof mocmosold_colmap)<<VLENGTHSH)},
	{x_("USER_layer_letters"), (CHAR *)mocmosold_layer_letters, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},

	/* set information for the I/O tool */
	{x_("IO_cif_layer_names"), (CHAR *)mocmosold_cif_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("IO_gds_layer_numbers"), (CHAR *)mocmosold_gds_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},

	/* set information for the DRC tool */
	{x_("DRC_min_unconnected_distances"), (CHAR *)mocmosold_unconnectedtable, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|
			(((sizeof mocmosold_unconnectedtable)/SIZEOFINTBIG)<<VLENGTHSH)},
	{x_("DRC_min_connected_distances"), (CHAR *)mocmosold_connectedtable, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|
			(((sizeof mocmosold_connectedtable)/SIZEOFINTBIG)<<VLENGTHSH)},
	{x_("DRC_ecad_deck"), (CHAR *)mocmosold_drc_ecad_deck, 0.0,
		VSTRING|VDONTSAVE|VISARRAY},

	/* set information for the SIM tool (SPICE) */
	{x_("SIM_spice_min_resistance"), 0, mocmosold_MIN_RESIST, VFLOAT|VDONTSAVE},
	{x_("SIM_spice_min_capacitance"), 0, mocmosold_MIN_CAPAC, VFLOAT|VDONTSAVE},
	{x_("SIM_spice_resistance"), (CHAR *)mocmosold_sim_spice_resistance, 0.0,
		VFLOAT|VISARRAY|(MAXLAYERS<<VLENGTHSH)|VDONTSAVE},
	{x_("SIM_spice_capacitance"), (CHAR *)mocmosold_sim_spice_capacitance, 0.0,
		VFLOAT|VISARRAY|(MAXLAYERS<<VLENGTHSH)|VDONTSAVE},
	{x_("SIM_spice_header_level1"), (CHAR *)mocmosold_sim_spice_header_level1, 0.0,
		VSTRING|VDONTSAVE|VISARRAY},
	{x_("SIM_spice_header_level2"), (CHAR *)mocmosold_sim_spice_header_level2, 0.0,
		VSTRING|VDONTSAVE|VISARRAY},
	{NULL, NULL, 0.0, 0}
};

/******************** ROUTINES ********************/

BOOLEAN mocmosold_initprocess(TECHNOLOGY *tech, INTBIG pass)
{
	/* initialize the technology variable */
	if (pass == 0) mocmosold_tech = tech;
	return(FALSE);
}

void mocmosold_setmode(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG l, i, aindex;
	REGISTER CHAR *pp;
	REGISTER ARCPROTO *ap;
	REGISTER VARIABLE *var;
	extern COMCOMP us_noyesp;

	if (count == 0)
	{
		if ((mocmosold_state&NWELL) == 0) ttyputmsg(M_("Technology currently set for p-well")); else
			ttyputmsg(M_("Technology currently set for n-well"));
		if ((mocmosold_state&EITHER) == 0)
			ttyputmsg(M_("CIF output uses general (p/n-well switchable) layers")); else
				ttyputmsg(M_("CIF output runs in either p-well or n-well"));
		if ((mocmosold_state&OUTLINE) == 0)
			ttyputmsg(M_("%s layers use stipple patterns"), mocmosold_stipplelayers); else
				ttyputmsg(M_("%s layers use outlines"), mocmosold_stipplelayers);
		return;
	}

	l = estrlen(pp = par[0]);

	if (namesamen(pp, x_("convert-old-format-library"), l) == 0)
	{
		ttyputmsg(M_("This operation will irreversibly alter the current library"));
		count = ttygetparam(M_("Proceed? [n] "), &us_noyesp, MAXPARS, par);
		if (count <= 0 || namesamen(par[0], x_("yes"), estrlen(par[0])) != 0)
			ttyputmsg(M_("No changes made")); else
				tech_convertmocmoslib(el_curlib);
		return;
	}

	if (namesamen(pp, x_("stipple-layers"), l) == 0)
	{
		/* change the descriptions of the layers */
		for(i=0; i<mocmosold_layerstipplesize; i++)
			mocmosold_layerstipple[i]->colstyle = PATTERNED;

		/* change the style of arcs that use these layers */
		for(i=0; i<mocmosold_arcstipplesize; i++)
		{
			aindex = mocmosold_arcstipple[i].layer;
			mocmosold_arcstipple[i].str[aindex].style = FILLED;
		}

		/* change the style of nodes that use these layers */
		for(i=0; i<mocmosold_nodepstipplesize; i++)
		{
			aindex = mocmosold_nodepstipple[i].layer;
			mocmosold_nodepstipple[i].str[aindex].style = FILLEDRECT;
		}
		for(i=0; i<mocmosold_nodesstipplesize; i++)
		{
			aindex = mocmosold_nodesstipple[i].layer;
			mocmosold_nodesstipple[i].str[aindex].basics.style = FILLEDRECT;
		}

		mocmosold_state &= ~OUTLINE;
		ttyputmsg(M_("%s layers will use stipple patterns"), mocmosold_stipplelayers);
		return;
	}

	if (namesamen(pp, x_("outline-layers"), l) == 0)
	{
		/* change the descriptions of the layers */
		for(i=0; i<mocmosold_layerstipplesize; i++)
			mocmosold_layerstipple[i]->colstyle = SOLIDC;

		/* change the style of arcs that use these layers */
		for(i=0; i<mocmosold_arcstipplesize; i++)
		{
			aindex = mocmosold_arcstipple[i].layer;
			mocmosold_arcstipple[i].str[aindex].style = CLOSED;
		}

		/* change the style of nodes that use these layers */
		for(i=0; i<mocmosold_nodepstipplesize; i++)
		{
			aindex = mocmosold_nodepstipple[i].layer;
			mocmosold_nodepstipple[i].str[aindex].style = CLOSEDRECT;
		}
		for(i=0; i<mocmosold_nodesstipplesize; i++)
		{
			aindex = mocmosold_nodesstipple[i].layer;
			mocmosold_nodesstipple[i].str[aindex].basics.style = CLOSEDRECT;
		}

		mocmosold_state |= OUTLINE;
		ttyputmsg(M_("%s layers will use outlines"), mocmosold_stipplelayers);
		return;
	}

	if (namesamen(pp, x_("general-process"), l) == 0)
	{
		mocmosold_cif_layers[LSELECTP] = x_("CSG");
		mocmosold_cif_layers[LSELECTN] = x_("CSG");
		mocmosold_cif_layers[LWELLP] = x_("CWG");
		mocmosold_cif_layers[LWELLN] = x_("CWG");
		mocmosold_cif_layers[LSELECTPP] = x_("");
		mocmosold_cif_layers[LSELECTNP] = x_("");
		mocmosold_cif_layers[LWELLPP] = x_("");
		mocmosold_cif_layers[LWELLNP] = x_("");
		mocmosold_setpseudo();
		(void)setval((INTBIG)mocmosold_tech, VTECHNOLOGY, x_("IO_cif_layer_names"),
			(INTBIG)mocmosold_cif_layers, VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH));
		mocmosold_state &= ~EITHER;
		ttyputverbose(M_("CIF output will use general (p/n-well switchable) layers"));
		return;
	}

	if (namesamen(pp, x_("either-process"), l) == 0)
	{
		mocmosold_cif_layers[LSELECTP] = x_("CSP");
		mocmosold_cif_layers[LSELECTN] = x_("CSN");
		mocmosold_cif_layers[LWELLP] = x_("CWP");
		mocmosold_cif_layers[LWELLN] = x_("CWN");
		mocmosold_cif_layers[LSELECTPP] = x_("CSP");
		mocmosold_cif_layers[LSELECTNP] = x_("CSN");
		mocmosold_cif_layers[LWELLPP] = x_("CWP");
		mocmosold_cif_layers[LWELLNP] = x_("CWN");
		mocmosold_setpseudo();
		(void)setval((INTBIG)mocmosold_tech, VTECHNOLOGY, x_("IO_cif_layer_names"),
			(INTBIG)mocmosold_cif_layers, VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH));
		mocmosold_state |= EITHER;
		ttyputverbose(M_("CIF output will be able to run in p-well or n-well"));
		return;
	}

	if (namesamen(pp, x_("p-well"), l) == 0)
	{
		/* set the D-transistor to be NMOS */
		mocmosold_tna.creation->userbits &= ~NFUNCTION;
		mocmosold_tna.creation->userbits |= (NPTRANMOS<<NFUNCTIONSH);

		/* set the S-transistor to be PMOS */
		mocmosold_tpa.creation->userbits &= ~NFUNCTION;
		mocmosold_tpa.creation->userbits |= (NPTRAPMOS<<NFUNCTIONSH);

		/* set the Metal-1-Well-Contact to be of type WELL */
		mocmosold_psub.creation->userbits &= ~NFUNCTION;
		mocmosold_psub.creation->userbits |= (NPWELL<<NFUNCTIONSH);

		/* set the Metal-1-Substrate-Contact to be of type SUBSTRATE */
		mocmosold_nsub.creation->userbits &= ~NFUNCTION;
		mocmosold_nsub.creation->userbits |= (NPSUBSTRATE<<NFUNCTIONSH);

		/* set the D-active arc to N-well and S-active arc to P-well */
		ap = mocmosold_tech->firstarcproto;	/* metal-1 */
		ap = ap->nextarcproto;			/* metal-2 */
		ap = ap->nextarcproto;			/* polysilicon */
		ap = ap->nextarcproto;			/* S-active */
		ap->userbits &= ~AFUNCTION;
		ap->userbits |= (APDIFFP << AFUNCTIONSH);
		ap = ap->nextarcproto;			/* D-active */
		ap->userbits &= ~AFUNCTION;
		ap->userbits |= (APDIFFN << AFUNCTIONSH);

		/* change the layer functions */
		var = getval((INTBIG)mocmosold_tech, VTECHNOLOGY, VINTEGER|VISARRAY, x_("TECH_layer_function"));
		if (var != NOVARIABLE)
		{
			/* change layers */
			((INTBIG *)var->addr)[3] = LFDIFF|LFPTYPE|LFTRANS3;	/* S-active */
			((INTBIG *)var->addr)[4] = LFDIFF|LFNTYPE|LFTRANS3;	/* D-active */
			((INTBIG *)var->addr)[15] = LFDIFF|LFPTYPE|LFTRANS3; /*S-active-well*/

			/* change pseudo-layers */
			((INTBIG *)var->addr)[19] = LFDIFF|LFPTYPE|LFPSEUDO|LFTRANS3; /*sact*/
			((INTBIG *)var->addr)[20] = LFDIFF|LFNTYPE|LFPSEUDO|LFTRANS3; /*dact*/
		}

		/* change the layer visibility */
		mocmosold_wp_lay.bits = LAYERT5;
		mocmosold_pwp_lay.bits = LAYERT5;
		mocmosold_ssp_lay.bits = LAYERO;
		mocmosold_pssp_lay.bits = LAYERO;
		mocmosold_wn_lay.bits = LAYERN;
		mocmosold_pwn_lay.bits = LAYERN;
		mocmosold_ssn_lay.bits = LAYERN;
		mocmosold_pssn_lay.bits = LAYERN;

		mocmosold_setpseudo();

		/* change the technology description */
		mocmosold_tech->techdescript = _("Complementary MOS (from MOSIS, P-Well, double metal)");

		mocmosold_state &= ~NWELL;
		ttyputverbose(M_("MOSIS CMOS technology is P-well"));
		return;
	}

	if (namesamen(pp, x_("n-well"), l) == 0)
	{
		/* set the D-transistor to be PMOS */
		mocmosold_tna.creation->userbits &= ~NFUNCTION;
		mocmosold_tna.creation->userbits |= (NPTRAPMOS<<NFUNCTIONSH);

		/* set the S-transistor to be NMOS */
		mocmosold_tpa.creation->userbits &= ~NFUNCTION;
		mocmosold_tpa.creation->userbits |= (NPTRANMOS<<NFUNCTIONSH);

		/* set the Metal-1-Well-Contact to be of type SUBSTRATE */
		mocmosold_psub.creation->userbits &= ~NFUNCTION;
		mocmosold_psub.creation->userbits |= (NPSUBSTRATE<<NFUNCTIONSH);

		/* set the Metal-1-Substrate-Contact to be of type WELL */
		mocmosold_nsub.creation->userbits &= ~NFUNCTION;
		mocmosold_nsub.creation->userbits |= (NPWELL<<NFUNCTIONSH);

		/* set the D-active arc to P-well and S-active arc to N-well */
		ap = mocmosold_tech->firstarcproto;	/* metal-1 */
		ap = ap->nextarcproto;			/* metal-2 */
		ap = ap->nextarcproto;			/* polysilicon */
		ap = ap->nextarcproto;			/* S-active */
		ap->userbits &= ~AFUNCTION;
		ap->userbits |= (APDIFFN << AFUNCTIONSH);
		ap = ap->nextarcproto;			/* D-active */
		ap->userbits &= ~AFUNCTION;
		ap->userbits |= (APDIFFP << AFUNCTIONSH);

		/* change the layer functions */
		var = getval((INTBIG)mocmosold_tech, VTECHNOLOGY, VINTEGER|VISARRAY, x_("TECH_layer_function"));
		if (var != NOVARIABLE)
		{
			/* change layers */
			((INTBIG *)var->addr)[3] = LFDIFF|LFNTYPE|LFTRANS3;	/* S-active */
			((INTBIG *)var->addr)[4] = LFDIFF|LFPTYPE|LFTRANS3;	/* D-active */
			((INTBIG *)var->addr)[15] = LFSUBSTRATE|LFNTYPE|LFTRANS3;/*Sact-well*/

			/* change pseudo-layers */
			((INTBIG *)var->addr)[19] = LFDIFF|LFNTYPE|LFPSEUDO|LFTRANS3; /*sact*/
			((INTBIG *)var->addr)[20] = LFDIFF|LFPTYPE|LFPSEUDO|LFTRANS3; /*dact*/
		}

		/* change the layer visibility */
		mocmosold_wp_lay.bits = LAYERN;
		mocmosold_pwp_lay.bits = LAYERN;
		mocmosold_ssp_lay.bits = LAYERN;
		mocmosold_pssp_lay.bits = LAYERN;
		mocmosold_wn_lay.bits = LAYERT5;
		mocmosold_pwn_lay.bits = LAYERT5;
		mocmosold_ssn_lay.bits = LAYERO;
		mocmosold_pssn_lay.bits = LAYERO;

		mocmosold_setpseudo();

		/* change the technology description */
		mocmosold_tech->techdescript = _("Complementary MOS (from MOSIS, N-Well, double metal)");

		mocmosold_state |= NWELL;
		ttyputverbose(M_("MOSIS CMOS technology is N-well"));
		return;
	}
	ttyputbadusage(x_("technology tell mocmos"));
}

void mocmosold_setpseudo(void)
{
	/* set normal use of N-select */
	mocmosold_al_na[2].lay = LSELECTN;
	mocmosold_mna_l[3].layernum = LSELECTN;
	mocmosold_tna_l[3].basics.layernum = LSELECTN;
	mocmosold_tnaE_l[5].basics.layernum = LSELECTN;
	mocmosold_nsub_l[3].layernum = LSELECTN;
	mocmosold_sn_l[0].layernum = LSELECTN;

	/* set normal use of P-select */
	mocmosold_al_pa[2].lay = LSELECTP;
	mocmosold_mpa_l[3].layernum = LSELECTP;
	mocmosold_tpa_l[3].basics.layernum = LSELECTP;
	mocmosold_tpaE_l[5].basics.layernum = LSELECTP;
	mocmosold_psub_l[3].layernum = LSELECTP;
	mocmosold_sp_l[0].layernum = LSELECTP;

	/* set normal use of N-Well */
	mocmosold_al_pa[1].lay = LWELLN;
	mocmosold_mpa_l[2].layernum = LWELLN;
	mocmosold_tpa_l[2].basics.layernum = LWELLN;
	mocmosold_tpaE_l[4].basics.layernum = LWELLN;
	mocmosold_nsub_l[2].layernum = LWELLN;
	mocmosold_wn_l[0].layernum = LWELLN;

	/* set normal use of P-Well */
	mocmosold_al_na[1].lay = LWELLP;
	mocmosold_mna_l[2].layernum = LWELLP;
	mocmosold_tna_l[2].basics.layernum = LWELLP;
	mocmosold_tnaE_l[4].basics.layernum = LWELLP;
	mocmosold_psub_l[2].layernum = LWELLP;
	mocmosold_wp_l[0].layernum = LWELLP;

	/* in general processes, "pseudo" the layer not being used */
	if ((mocmosold_state&EITHER) == 0)
	{
		/* switch all uses of N-select */
		if (mocmosold_ssn_lay.bits == LAYERN)
		{
			mocmosold_al_na[2].lay = LSELECTNP;
			mocmosold_mna_l[3].layernum = LSELECTNP;
			mocmosold_tna_l[3].basics.layernum = LSELECTNP;
			mocmosold_tnaE_l[5].basics.layernum = LSELECTNP;
			mocmosold_nsub_l[3].layernum = LSELECTNP;
			mocmosold_sn_l[0].layernum = LSELECTNP;
		}

		/* switch all uses of P-select */
		if (mocmosold_ssp_lay.bits == LAYERN)
		{
			mocmosold_al_pa[2].lay = LSELECTPP;
			mocmosold_mpa_l[3].layernum = LSELECTPP;
			mocmosold_tpa_l[3].basics.layernum = LSELECTPP;
			mocmosold_tpaE_l[5].basics.layernum = LSELECTPP;
			mocmosold_psub_l[3].layernum = LSELECTPP;
			mocmosold_sp_l[0].layernum = LSELECTPP;
		}

		/* switch all uses of N-well */
		if (mocmosold_wn_lay.bits == LAYERN)
		{
			mocmosold_al_pa[1].lay = LWELLNP;
			mocmosold_mpa_l[2].layernum = LWELLNP;
			mocmosold_tpa_l[2].basics.layernum = LWELLNP;
			mocmosold_tpaE_l[4].basics.layernum = LWELLNP;
			mocmosold_nsub_l[2].layernum = LWELLNP;
			mocmosold_wn_l[0].layernum = LWELLNP;
		}

		/* switch all uses of P-well */
		if (mocmosold_wp_lay.bits == LAYERN)
		{
			mocmosold_al_na[1].lay = LWELLPP;
			mocmosold_mna_l[2].layernum = LWELLPP;
			mocmosold_tna_l[2].basics.layernum = LWELLPP;
			mocmosold_tnaE_l[4].basics.layernum = LWELLPP;
			mocmosold_psub_l[2].layernum = LWELLPP;
			mocmosold_wp_l[0].layernum = LWELLPP;
		}
	}
}

INTBIG mocmosold_arcpolys(ARCINST *ai, WINDOWPART *win)
{
	return(mocmosold_intarcpolys(ai, win, &tech_oneprocpolyloop, &mocmosold_oneprocpolyloop));
}

INTBIG mocmosold_intarcpolys(ARCINST *ai, WINDOWPART *win, POLYLOOP *pl, MOCPOLYLOOP *mocpl)
{
	REGISTER INTBIG total;
	static INTBIG arc_shrinkback_key = 0;

	if (arc_shrinkback_key == 0) arc_shrinkback_key = makekey(x_("ARC_shrinkback"));
	mocpl->shrinkvar = getvalkey((INTBIG)ai, VARCINST, VINTEGER, arc_shrinkback_key);

	if ((ai->userbits&ISNEGATED) != 0) tech_resetnegated(ai);
	total = mocmosold_arcprotos[ai->proto->arcindex]->laycount;
	if ((ai->userbits&ISDIRECTIONAL) != 0) total++;

	/* add in displayable variables */
	pl->realpolys = total;
	total += tech_displayableavars(ai, win, pl);
	return(total);
}

void mocmosold_shapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly)
{
	mocmosold_intshapearcpoly(ai, box, poly, &tech_oneprocpolyloop, &mocmosold_oneprocpolyloop);
}

void mocmosold_intshapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly, POLYLOOP *pl, MOCPOLYLOOP *mocpl)
{
	REGISTER INTBIG aindex, i;
	REGISTER INTBIG shrinkback, len, oldbits, thisend, lambda;
	REGISTER TECH_ARCLAY *thista;
	INTBIG oldx[2], oldy[2];

	/* handle displayable variables */
	if (box >= pl->realpolys)
	{
		(void)tech_filldisplayableavar(ai, poly, pl->curwindowpart, 0, pl);
		return;
	}

	aindex = ai->proto->arcindex;
	if (box >= mocmosold_arcprotos[aindex]->laycount) tech_makearrow(ai, poly); else
	{
		thista = &mocmosold_arcprotos[aindex]->list[box];
		lambda = lambdaofarc(ai);

		if (mocpl->shrinkvar != NOVARIABLE)
		{
			/* shrink arc if connected to export */
			shrinkback = mocpl->shrinkvar->addr * lambda;
			len = ai->length;
			oldbits = ai->userbits;
			ai->userbits |= (NOEXTEND|NOTEND0|NOTEND1);
			for(i=0; i<2; i++)
			{
				if (i == 0) thisend = NOTEND0; else thisend = NOTEND1;

				oldx[i] = ai->end[i].xpos;   oldy[i] = ai->end[i].ypos;
				if (ai->end[i].nodeinst->proto->primindex != 0)
				{
					if ((oldbits&(NOEXTEND|thisend)) == NOEXTEND) ai->userbits &= ~thisend;
					continue;
				}

				/* quit if this end cannot be shrunk any more */
				if (len <= shrinkback) continue;

				/* shrink this end */
				len -= shrinkback;
				ai->userbits &= ~thisend;
				if (ai->end[0].xpos == ai->end[1].xpos)
				{
					/* vertical arc */
					if (ai->end[i].ypos < ai->end[1-i].ypos)
						ai->end[i].ypos += shrinkback; else
							ai->end[i].ypos -= shrinkback;
				} else
				{
					/* horizontal arc */
					if (ai->end[i].xpos < ai->end[1-i].xpos)
						ai->end[i].xpos += shrinkback; else
							ai->end[i].xpos -= shrinkback;
				}
			}
			makearcpoly(ai->length, ai->width-thista->off*lambda/WHOLE, ai, poly,
				thista->style);

			/* restore end co-ordinates */
			for(i=0; i<2; i++)
			{
				ai->end[i].xpos = oldx[i];   ai->end[i].ypos = oldy[i];
			}
			ai->userbits = oldbits;
		} else
		{
			makearcpoly(ai->length, ai->width-thista->off*lambda/WHOLE, ai, poly,
				thista->style);
		}
		poly->layer = thista->lay;
		poly->desc = mocmosold_layers[poly->layer];
	}
}

INTBIG mocmosold_allarcpolys(ARCINST *ai, POLYLIST *plist, WINDOWPART *win)
{
	REGISTER INTBIG tot, j;
	POLYLOOP mypl;
	MOCPOLYLOOP mymocpl;

	mypl.curwindowpart = win;
	tot = mocmosold_intarcpolys(ai, win, &mypl, &mymocpl);
	tot = mypl.realpolys;
	if (ensurepolylist(plist, tot, db_cluster)) return(-1);
	for(j = 0; j < tot; j++)
	{
		mocmosold_intshapearcpoly(ai, j, plist->polygons[j], &mypl, &mymocpl);
	}
	return(tot);
}
