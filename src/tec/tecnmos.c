/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tecnmos.c
 * nMOS technology description
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

BOOLEAN nmos_initprocess(TECHNOLOGY*, INTBIG);

/******************** LAYERS ********************/

#define MAXLAYERS 15		/* total layers below */
#define LMETAL     0		/* metal             metal             */
#define LPOLY      1		/* polysilicon       polysilicon       */
#define LDIFF      2		/* diffusion         SGD               */
#define LIMPLNT    3		/* implant           hard implant      */
#define LCUT       4		/* cut               contact I         */
#define LBURIED    5		/* buried contact    buried contact    */
#define LOVERGL    6		/* overglass         vapox             */
#define LLIMP      7		/* implant 2         light implant     */
#define LOVRCON    8		/* oversize contact  contact II        */
#define LHENH      9		/* enhancement       hard enhancement  */
#define LLENH     10		/* enhancement 2     light enhancement */
#define LTRANS    11		/* transistor                          */
#define LMETALP   12		/* metal pin                           */
#define LPOLYP    13		/* polysilicon pin                     */
#define LDIFFP    14		/* diffusion pin                       */

#ifdef	CIFPLOTLAYERS
static GRAPHICS nmos_m_lay = {LAYERT1,COLORT1, SOLIDC, PATTERNED,
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
static GRAPHICS nmos_p_lay = {LAYERT2,COLORT2, SOLIDC, PATTERNED,
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
static GRAPHICS nmos_d_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
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
static GRAPHICS nmos_i_lay = {LAYERT4,COLORT4, SOLIDC, PATTERNED,
/* implant layer */		{0x0000, /*                  */
						0xCCCC,  /* XX  XX  XX  XX   */
						0x0000,  /*                  */
						0xCCCC,  /* XX  XX  XX  XX   */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0xCCCC,  /* XX  XX  XX  XX   */
						0x0000,  /*                  */
						0xCCCC,  /* XX  XX  XX  XX   */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS nmos_c_lay = {LAYERO,BLACK, SOLIDC, SOLIDC,
/* contact layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS nmos_b_lay = {LAYERT5,COLORT5, SOLIDC, PATTERNED,
/* buried layer */		{0x1000, /*    X             */
						0x0020,  /*           X      */
						0x4000,  /*  X               */
						0x0080,  /*         X        */
						0x0001,  /*                X */
						0x0200,  /*       X          */
						0x0004,  /*              X   */
						0x0800,  /*     X            */
						0x1000,  /*    X             */
						0x0020,  /*           X      */
						0x4000,  /*  X               */
						0x0080,  /*         X        */
						0x0001,  /*                X */
						0x0200,  /*       X          */
						0x0004,  /*              X   */
						0x0800}, /*     X            */
						NOVARIABLE, 0};
static GRAPHICS nmos_o_lay = {LAYERO,DGRAY, SOLIDC, PATTERNED,
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
static GRAPHICS nmos_li_lay = {LAYERO,YELLOW, SOLIDC, PATTERNED,
/* light impl. layer */	{0x0000, /*                  */
						0xCCCC,  /* XX  XX  XX  XX   */
						0x0000,  /*                  */
						0xCCCC,  /* XX  XX  XX  XX   */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0xCCCC,  /* XX  XX  XX  XX   */
						0x0000,  /*                  */
						0xCCCC,  /* XX  XX  XX  XX   */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS nmos_oc_lay = {LAYERO,LGRAY, SOLIDC, SOLIDC,
/* oversize cut layer */{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS nmos_he_lay = {LAYERO,LRED, SOLIDC, PATTERNED,
/* hard enhanc layer */	{0x1010, /*    X       X     */
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
static GRAPHICS nmos_le_lay = {LAYERO,LBLUE, SOLIDC, PATTERNED,
/* light enhanc layer */{0x4040, /*  X       X       */
						0x8080,  /* X       X        */
						0x0101,  /*        X       X */
						0x0202,  /*       X       X  */
						0x0101,  /*        X       X */
						0x8080,  /* X       X        */
						0x4040,  /*  X       X       */
						0x2020,  /*   X       X      */
						0x4040,  /*  X       X       */
						0x8080,  /* X       X        */
						0x0101,  /*        X       X */
						0x0202,  /*       X       X  */
						0x0101,  /*        X       X */
						0x8080,  /* X       X        */
						0x4040,  /*  X       X       */
						0x2020}, /*   X       X      */
						NOVARIABLE, 0};
static GRAPHICS nmos_tr_lay = {LAYERN,ALLOFF, SOLIDC, PATTERNED,
/* transistor layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS nmos_pm_lay = {LAYERT1,COLORT1, SOLIDC, SOLIDC,
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
static GRAPHICS nmos_pp_lay = {LAYERT2,COLORT2, SOLIDC, SOLIDC,
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
						0x1010}  /*    X       X     */
						NOVARIABLE, 0};
static GRAPHICS nmos_pd_lay = {LAYERT3,COLORT3, SOLIDC, SOLIDC,
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
#else
/****** Icarus layers ******/
static GRAPHICS nmos_m_lay = {LAYERT1,COLORT1, SOLIDC, PATTERNED,
/* metal layer */		{0x0000, /*                  */
						0x0000,  /*                  */
						0x2222,  /*   X   X   X   X  */
						0x8888,  /* X   X   X   X    */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x2222,  /*   X   X   X   X  */
						0x8888,  /* X   X   X   X    */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x2222,  /*   X   X   X   X  */
						0x8888,  /* X   X   X   X    */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x2222,  /*   X   X   X   X  */
						0x8888}, /* X   X   X   X    */
						NOVARIABLE, 0};
static GRAPHICS nmos_p_lay = {LAYERT2,COLORT2, SOLIDC, PATTERNED,
/* polysilicon layer */	{0x1111, /*    X   X   X   X */
						0x8888,  /* X   X   X   X    */
						0x4444,  /*  X   X   X   X   */
						0x2222,  /*   X   X   X   X  */
						0x1111,  /*    X   X   X   X */
						0x8888,  /* X   X   X   X    */
						0x4444,  /*  X   X   X   X   */
						0x2222,  /*   X   X   X   X  */
						0x1111,  /*    X   X   X   X */
						0x8888,  /* X   X   X   X    */
						0x4444,  /*  X   X   X   X   */
						0x2222,  /*   X   X   X   X  */
						0x1111,  /*    X   X   X   X */
						0x8888,  /* X   X   X   X    */
						0x4444,  /*  X   X   X   X   */
						0x2222}, /*   X   X   X   X  */
						NOVARIABLE, 0};
static GRAPHICS nmos_d_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
/* diffusion layer */	{0x4444, /*  X   X   X   X   */
						0x1111,  /*    X   X   X   X */
						0x4444,  /*  X   X   X   X   */
						0x1111,  /*    X   X   X   X */
						0x4444,  /*  X   X   X   X   */
						0x1111,  /*    X   X   X   X */
						0x4444,  /*  X   X   X   X   */
						0x1111,  /*    X   X   X   X */
						0x4444,  /*  X   X   X   X   */
						0x1111,  /*    X   X   X   X */
						0x4444,  /*  X   X   X   X   */
						0x1111,  /*    X   X   X   X */
						0x4444,  /*  X   X   X   X   */
						0x1111,  /*    X   X   X   X */
						0x4444,  /*  X   X   X   X   */
						0x1111}, /*    X   X   X   X */
						NOVARIABLE, 0};
static GRAPHICS nmos_i_lay = {LAYERT4,COLORT4, SOLIDC, PATTERNED,
/* implant layer */		{0x0000, /*                  */
						0x0000,  /*                  */
						0x1111,  /*    X   X   X   X */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x1111,  /*    X   X   X   X */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x1111,  /*    X   X   X   X */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x1111,  /*    X   X   X   X */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS nmos_c_lay = {LAYERO,BLACK, SOLIDC, SOLIDC,
/* contact layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS nmos_b_lay = {LAYERT5,COLORT5, SOLIDC, PATTERNED,
/* buried layer */		{0x0000, /*                  */
						0x2222,  /*   X   X   X   X  */
						0x4444,  /*  X   X   X   X   */
						0x8888,  /* X   X   X   X    */
						0x0000,  /*                  */
						0x2222,  /*   X   X   X   X  */
						0x4444,  /*  X   X   X   X   */
						0x8888,  /* X   X   X   X    */
						0x0000,  /*                  */
						0x2222,  /*   X   X   X   X  */
						0x4444,  /*  X   X   X   X   */
						0x8888,  /* X   X   X   X    */
						0x0000,  /*                  */
						0x2222,  /*   X   X   X   X  */
						0x4444,  /*  X   X   X   X   */
						0x8888}, /* X   X   X   X    */
						NOVARIABLE, 0};
static GRAPHICS nmos_o_lay = {LAYERO,DGRAY, SOLIDC, PATTERNED,
/* overglass layer */	{0x0000, /*                  */
						0x2222,  /*   X   X   X   X  */
						0x5555,  /*  X X X X X X X X */
						0x2222,  /*   X   X   X   X  */
						0x0000,  /*                  */
						0x2222,  /*   X   X   X   X  */
						0x5555,  /*  X X X X X X X X */
						0x2222,  /*   X   X   X   X  */
						0x0000,  /*                  */
						0x2222,  /*   X   X   X   X  */
						0x5555,  /*  X X X X X X X X */
						0x2222,  /*   X   X   X   X  */
						0x0000,  /*                  */
						0x2222,  /*   X   X   X   X  */
						0x5555,  /*  X X X X X X X X */
						0x2222}, /*   X   X   X   X  */
						NOVARIABLE, 0};
static GRAPHICS nmos_li_lay = {LAYERO,YELLOW, SOLIDC, PATTERNED,
/* light impl. layer */	{0x0000, /*                  */
						0xCCCC,  /* XX  XX  XX  XX   */
						0x0000,  /*                  */
						0xCCCC,  /* XX  XX  XX  XX   */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0xCCCC,  /* XX  XX  XX  XX   */
						0x0000,  /*                  */
						0xCCCC,  /* XX  XX  XX  XX   */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x0000}, /*                  */
						NOVARIABLE, 0};
static GRAPHICS nmos_oc_lay = {LAYERO,LGRAY, SOLIDC, SOLIDC,
/* oversize cut layer */ {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS nmos_he_lay = {LAYERO,LRED, SOLIDC, PATTERNED,
/* hard enhanc layer */ {0x1010, /*    X       X     */
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
static GRAPHICS nmos_le_lay = {LAYERO,LBLUE, SOLIDC, PATTERNED,
/* light enhanc layer */{0x4040, /*  X       X       */
						0x8080,  /* X       X        */
						0x0101,  /*        X       X */
						0x0202,  /*       X       X  */
						0x0101,  /*        X       X */
						0x8080,  /* X       X        */
						0x4040,  /*  X       X       */
						0x2020,  /*   X       X      */
						0x4040,  /*  X       X       */
						0x8080,  /* X       X        */
						0x0101,  /*        X       X */
						0x0202,  /*       X       X  */
						0x0101,  /*        X       X */
						0x8080,  /* X       X        */
						0x4040,  /*  X       X       */
						0x2020}, /*   X       X      */
						NOVARIABLE, 0};
static GRAPHICS nmos_tr_lay = {LAYERN,ALLOFF, SOLIDC, PATTERNED,
/* transistor layer */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS nmos_pm_lay = {LAYERT1,COLORT1, SOLIDC, PATTERNED,
/* pseudo-metal layer */{0x0000, /*                  */
						0x0000,  /*                  */
						0x2222,  /*   X   X   X   X  */
						0x8888,  /* X   X   X   X    */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x2222,  /*   X   X   X   X  */
						0x8888,  /* X   X   X   X    */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x2222,  /*   X   X   X   X  */
						0x8888,  /* X   X   X   X    */
						0x0000,  /*                  */
						0x0000,  /*                  */
						0x2222,  /*   X   X   X   X  */
						0x8888}, /* X   X   X   X    */
						NOVARIABLE, 0};
static GRAPHICS nmos_pp_lay = {LAYERT2,COLORT2, SOLIDC, PATTERNED,
/* pseudo-poly layer */	{0x1111, /*    X   X   X   X */
						0x8888,  /* X   X   X   X    */
						0x4444,  /*  X   X   X   X   */
						0x2222,  /*   X   X   X   X  */
						0x1111,  /*    X   X   X   X */
						0x8888,  /* X   X   X   X    */
						0x4444,  /*  X   X   X   X   */
						0x2222,  /*   X   X   X   X  */
						0x1111,  /*    X   X   X   X */
						0x8888,  /* X   X   X   X    */
						0x4444,  /*  X   X   X   X   */
						0x2222,  /*   X   X   X   X  */
						0x1111,  /*    X   X   X   X */
						0x8888,  /* X   X   X   X    */
						0x4444,  /*  X   X   X   X   */
						0x2222}, /*   X   X   X   X  */
						NOVARIABLE, 0};
static GRAPHICS nmos_pd_lay = {LAYERT3,COLORT3, SOLIDC, PATTERNED,
/* pseudo-diff layer */	{0x4444, /*  X   X   X   X   */
						0x1111,  /*    X   X   X   X */
						0x4444,  /*  X   X   X   X   */
						0x1111,  /*    X   X   X   X */
						0x4444,  /*  X   X   X   X   */
						0x1111,  /*    X   X   X   X */
						0x4444,  /*  X   X   X   X   */
						0x1111,   /*    X   X   X   X */
						0x4444,  /*  X   X   X   X   */
						0x1111,  /*    X   X   X   X */
						0x4444,  /*  X   X   X   X   */
						0x1111,  /*    X   X   X   X */
						0x4444,  /*  X   X   X   X   */
						0x1111,  /*    X   X   X   X */
						0x4444,  /*  X   X   X   X   */
						0x1111},  /*    X   X   X   X */
						NOVARIABLE, 0};
#endif

/* these tables must be updated together */
GRAPHICS *nmos_layers[MAXLAYERS+1] = {&nmos_m_lay, &nmos_p_lay,
	&nmos_d_lay, &nmos_i_lay, &nmos_c_lay, &nmos_b_lay, &nmos_o_lay,
	&nmos_li_lay, &nmos_oc_lay, &nmos_he_lay, &nmos_le_lay, &nmos_tr_lay,
	&nmos_pm_lay, &nmos_pp_lay, &nmos_pd_lay, NOGRAPHICS};
static CHAR *nmos_layer_names[MAXLAYERS] = {x_("Metal"), x_("Polysilicon"),
	x_("Diffusion"), x_("Implant"), x_("Contact-Cut"), x_("Buried-Contact"), x_("Overglass"),
	x_("Light-Implant"), x_("Oversize-Contact"), x_("Hard-Enhancement"),
	x_("Light-Enhancement"), x_("Transistor"), x_("Pseudo-Metal"), x_("Pseudo-Polysilicon"),
	x_("Pseudo-Diffusion")};
static CHAR *nmos_cif_layers[MAXLAYERS] = {x_("NM"), x_("NP"), x_("ND"), x_("NI"), x_("NC"), x_("NB"),
	x_("NG"), x_("NJ"), x_("NO"), x_("NE"), x_("NF"), x_(""), x_(""), x_(""), x_("")};
static INTBIG nmos_layer_function[MAXLAYERS] = {LFMETAL1|LFTRANS1,
	LFPOLY1|LFTRANS2, LFDIFF|LFTRANS3, LFIMPLANT|LFDEPLETION|LFHEAVY|LFTRANS4,
	LFCONTACT1, LFIMPLANT|LFTRANS5, LFOVERGLASS, LFIMPLANT|LFDEPLETION|LFLIGHT,
	LFCONTACT3, LFIMPLANT|LFENHANCEMENT|LFHEAVY, LFIMPLANT|LFENHANCEMENT|LFLIGHT,
	LFTRANSISTOR|LFPSEUDO, LFMETAL1|LFPSEUDO|LFTRANS1, LFPOLY1|LFPSEUDO|LFTRANS2,
	LFDIFF|LFPSEUDO|LFTRANS3};
static CHAR *nmos_layer_letters[MAXLAYERS] = {x_("mb"), x_("pr"), x_("dg"), x_("iy"), x_("c"),
	x_("u"), x_("v"), x_("j"), x_("o"), x_("e"), x_("f"), x_("t"), x_("M"), x_("P"), x_("D")};

/* The low 5 bits map Metal, Polysilicon, Diffusion, Implant, and Buried */
static TECH_COLORMAP nmos_colmap[32] =
{                  /*     buried implant diffusion polysilicon metal   */
	{200,200,200}, /* 0:                                               */
	{  0,  0,200}, /* 1:                                       metal   */
	{220,  0,120}, /* 2:                           polysilicon         */
	{ 80,  0,160}, /* 3:                           polysilicon+metal   */
	{ 70,250, 70}, /* 4:                 diffusion                     */
	{  0,140,140}, /* 5:                 diffusion+            metal   */
	{180,130,  0}, /* 6:                 diffusion+polysilicon         */
	{ 55, 70,140}, /* 7:                 diffusion+polysilicon+metal   */
	{250,250,  0}, /* 8:         implant                               */
	{ 85,105,160}, /* 9:         implant+                      metal   */
	{190, 80,100}, /* 10:        implant+          polysilicon         */
	{ 70, 50,150}, /* 11:        implant+          polysilicon+metal   */
	{ 80,210,  0}, /* 12:        implant+diffusion                     */
	{ 50,105,130}, /* 13:        implant+diffusion+            metal   */
	{170,110,  0}, /* 14:        implant+diffusion+polysilicon         */
	{ 60, 60,130}, /* 15:        implant+diffusion+polysilicon+metal   */
	{180,180,180}, /* 16: buried+                                      */
	{  0,  0,180}, /* 17: buried+                              metal   */
	{200,  0,100}, /* 18: buried+                  polysilicon         */
	{ 60,  0,140}, /* 19: buried+                  polysilicon+metal   */
	{ 50,230, 50}, /* 20: buried+        diffusion                     */
	{  0,120,120}, /* 21: buried+        diffusion+            metal   */
	{160,110,  0}, /* 22: buried+        diffusion+polysilicon         */
	{ 35, 50,120}, /* 23: buried+        diffusion+polysilicon+metal   */
	{230,230,  0}, /* 24: buried+implant                               */
	{ 65, 85,140}, /* 25: buried+implant+                      metal   */
	{170, 60, 80}, /* 26: buried+implant+          polysilicon         */
	{ 50, 30,130}, /* 27: buried+implant+          polysilicon+metal   */
	{ 60,190,  0}, /* 28: buried+implant+diffusion                     */
	{ 30, 85,110}, /* 29: buried+implant+diffusion+            metal   */
	{150, 90,  0}, /* 30: buried+implant+diffusion+polysilicon         */
	{ 40, 40,110}  /* 31: buried+implant+diffusion+polysilicon+metal   */
};

/******************** DESIGN RULES ********************/

/* layers that can connect to other layers when electrically disconnected */
static INTBIG nmos_unconnectedtable[] = {
/*            M  P  D  I  C  B  O  L  O  H  L  T  M  P  D */
/*            e  o  i  m  u  u  v     v        r  e  o  i */
/*            t  l  f  p  t  r  e  I  r  E  E  a  t  l  f */
/*            a  y  f  l     i  r  m  C  n  n  n  a  y  f */
/*            l        n     e  g  p  o  h  h  s  l  P  P */
/*                     t     d  l  l  n           P       */
/* Metal  */ K3,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* Poly   */    K2,K1,K1,XX,K2,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* Diff   */       K3,K2,XX,K2,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* Implnt */          XX,XX,XX,XX,XX,XX,XX,XX,K2,XX,XX,XX,
/* Cut    */             XX,XX,XX,XX,XX,XX,XX,K2,XX,XX,XX,
/* Buried */                XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* Overgl */                   XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* L Impl */                      XX,XX,XX,XX,XX,XX,XX,XX,
/* OvrCon */                         XX,XX,XX,XX,XX,XX,XX,
/* H Enh  */                            XX,XX,XX,XX,XX,XX,
/* L Enh  */                               XX,XX,XX,XX,XX,
/* Trans  */                                  XX,XX,XX,XX,
/* MetalP */                                     XX,XX,XX,
/* PolyP  */                                        XX,XX,
/* DiffP  */                                           XX
};

/* layers that can connect to other layers when electrically connected */
static INTBIG nmos_connectedtable[] = {
/*            M  P  D  I  C  B  O  L  O  H  L  T  M  P  D */
/*            e  o  i  m  u  u  v     v        r  e  o  i */
/*            t  l  f  p  t  r  e  I  r  E  E  a  t  l  f */
/*            a  y  f  l     i  r  m  C  n  n  n  a  y  f */
/*            l        n     e  g  p  o  h  h  s  l  P  P */
/*                     t     d  l  l  n           P       */
/* Metal  */ XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* Poly   */    XX,K0,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* Diff   */       XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* Implnt */          XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* Cut    */             XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* Buried */                XX,XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* Overgl */                   XX,XX,XX,XX,XX,XX,XX,XX,XX,
/* L Impl */                      XX,XX,XX,XX,XX,XX,XX,XX,
/* OvrCon */                         XX,XX,XX,XX,XX,XX,XX,
/* H Enh  */                            XX,XX,XX,XX,XX,XX,
/* L Enh  */                               XX,XX,XX,XX,XX,
/* Trans  */                                  XX,XX,XX,XX,
/* MetalP */                                     XX,XX,XX,
/* PolyP  */                                        XX,XX,
/* DiffP  */                                           XX
};

/******************** ARCS ********************/

#define ARCPROTOCOUNT   3
#define AMETAL          0	/* metal             */
#define APOLY           1	/* polysilicon       */
#define ADIFF           2	/* diffusion         */

/* metal arc */
static TECH_ARCLAY nmos_al_m[] = {{ LMETAL,0,FILLED }};
static TECH_ARCS nmos_a_m = {
	x_("Metal"),K3,AMETAL,NOARCPROTO,								/* name */
	1,nmos_al_m,													/* layers */
	(APMETAL1<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* polysilicon arc */
static TECH_ARCLAY nmos_al_p[] = {{ LPOLY,0,FILLED }};
static TECH_ARCS nmos_a_p = {
	x_("Polysilicon"),K2,APOLY,NOARCPROTO,							/* name */
	1,nmos_al_p,													/* layers */
	(APPOLY1<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

/* diffusion arc */
static TECH_ARCLAY nmos_al_d[] = {{ LDIFF,0,FILLED }};
static TECH_ARCS nmos_a_d = {
	x_("Diffusion"),K2,ADIFF,NOARCPROTO,							/* name */
	1,nmos_al_d,													/* layers */
	(APDIFF<<AFUNCTIONSH)|WANTFIXANG|CANWIPE|(90<<AANGLEINCSH)};	/* userbits */

TECH_ARCS *nmos_arcprotos[ARCPROTOCOUNT+1] = {
	&nmos_a_m, &nmos_a_p, &nmos_a_d, ((TECH_ARCS *)-1)};

/******************** PORT CONNECTIONS ********************/

/* these values are replaced with actual arcproto addresses */
static INTBIG nmos_pc_m[]    = {-1, AMETAL, ALLGEN, -1};
static INTBIG nmos_pc_p[]    = {-1, APOLY, ALLGEN, -1};
static INTBIG nmos_pc_pm[]   = {-1, APOLY, AMETAL, ALLGEN, -1};
static INTBIG nmos_pc_d[]    = {-1, ADIFF, ALLGEN, -1};
static INTBIG nmos_pc_dm[]   = {-1, ADIFF, AMETAL, ALLGEN, -1};
static INTBIG nmos_pc_dp[]   = {-1, ADIFF, APOLY, ALLGEN, -1};
static INTBIG nmos_pc_null[] = {-1, ALLGEN, -1};

/******************** RECTANGLE DESCRIPTIONS ********************/

static INTBIG nmos_fullbox[8] = {LEFTEDGE, BOTEDGE,  RIGHTEDGE,TOPEDGE};
static INTBIG nmos_cutbox[8]  = {LEFTIN1,  BOTIN1,   LEFTIN3,  BOTIN3};/*adjust*/
static INTBIG nmos_inhbox[8]  = {LEFTIN0H, BOTIN0H,  RIGHTIN0H,TOPIN0H};
static INTBIG nmos_in1box[8]  = {LEFTIN1,  BOTIN1,   RIGHTIN1, TOPIN1};
static INTBIG nmos_in2box[8]  = {LEFTIN2,  BOTIN2,   RIGHTIN2, TOPIN2};
static INTBIG nmos_rightbox[8]= {CENTER,   BOTEDGE,  RIGHTEDGE,TOPEDGE};
static INTBIG nmos_leftbox[8] = {LEFTEDGE, BOTEDGE,  CENTERR1, TOPEDGE};
static INTBIG nmos_bur1box[8] = {LEFTIN2,  BOTEDGE,  RIGHTIN2, TOPEDGE};
static INTBIG nmos_bur2box[8] = {LEFTEDGE, BOTIN1,   RIGHTEDGE,TOPIN1};
static INTBIG nmos_bur3box[8] = {LEFTIN2,  BOTEDGE,  RIGHTEDGE,TOPEDGE};
static INTBIG nmos_bur4box[8] = {LEFTIN2,  BOTEDGE,  RIGHTIN1, TOPEDGE};
static INTBIG nmos_bur5box[8] = {LEFTEDGE, BOTIN1,   RIGHTIN1, TOPIN1};
static INTBIG nmos_bur6box[8] = {LEFTIN2,  BOTIN1,   RIGHTEDGE,TOPIN1};
static INTBIG nmos_bur7box[32]= {LEFTEDGE, BOTIN1,   LEFTEDGE, TOPIN1,
								LEFTIN1,  TOPIN1,   LEFTIN1,  TOPEDGE,
								RIGHTIN1, TOPEDGE,  RIGHTIN1, BOTEDGE,
								LEFTIN1,  BOTEDGE,  LEFTIN1,  BOTIN1};
static INTBIG nmos_bur8box[8] = {LEFTIN2,  BOTIN1,   RIGHTIN1, TOPEDGE};
static INTBIG nmos_bur9box[8] = {LEFTIN2,  BOTIN1,   RIGHTIN2, TOPEDGE};
static INTBIG nmos_bur10box[24]={LEFTEDGE, BOTEDGE,  LEFTEDGE, TOPIN2,
								LEFTIN1,  TOPIN2,   LEFTIN1,  TOPIN1,
								RIGHTEDGE,TOPIN1,   RIGHTEDGE,BOTEDGE};
static INTBIG nmos_bur11box[32]={LEFTEDGE, BOTEDGE,  LEFTEDGE, TOPIN2,
								LEFTIN1,  TOPIN2,   LEFTIN1,  TOPIN1,
								RIGHTIN1, TOPIN1,   RIGHTIN1, TOPIN2,
								RIGHTEDGE,TOPIN2,   RIGHTEDGE,BOTEDGE};
static INTBIG nmos_tradbox[8] = {LEFTIN2,  BOTEDGE,  RIGHTIN2, TOPEDGE};
static INTBIG nmos_trapbox[8] = {LEFTEDGE, BOTIN2,   RIGHTEDGE,TOPIN2};
static INTBIG nmos_trd1box[8] = {LEFTIN2,  TOPIN2,   RIGHTIN2, TOPEDGE};
static INTBIG nmos_trd2box[8] = {LEFTIN2,  BOTEDGE,  RIGHTIN2, BOTIN2};

/******************** NODES ********************/

#define NODEPROTOCOUNT	26
#define NMETALP          1	/* metal pin */
#define NPOLYP           2	/* polysilicon pin */
#define NDIFFP           3	/* diffusion pin */
#define NMETPOLC         4	/* metal-polysilicon contact */
#define NMETDIFC         5	/* metal-diffusion contact */
#define NBUTC            6	/* butting contact */
#define NBURC            7	/* buried contact */
#define NTRANS           8	/* transistor */
#define NITRANS          9	/* implant transistor */
#define NBURCS          10	/* buried contact (short) */
#define NBURCT          11	/* buried contact (T) */
#define NBURCPS         12	/* buried contact (polysurr) */
#define NBURCDSI        13	/* buried contact (diffsurr inline) */
#define NBURCDST        14	/* buried contact (diffsurr T) */
#define NBURCDSL        15	/* buried contact (diffsurr L) */
#define NMETALN         16	/* metal node */
#define NPOLYN          17	/* polysilicon node */
#define NDIFFN          18	/* diffusion node */
#define NIMPN           19	/* implant node */
#define NCUTN           20	/* contact node */
#define NBURN           21	/* buried node */
#define NOVERGN         22	/* overglass node */
#define NLIMPN          23	/* light-implant node */
#define NOVRCUTN        24	/* oversize contact node */
#define NHENHN          25	/* hard enhancement node */
#define NLENHN          26	/* light enhancement node */

/* metal-pin */
static TECH_PORTS nmos_pm_p[] = {					/* ports */
	{nmos_pc_m, x_("metal"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON nmos_pm_l[] = {					/* layers */
	{LMETALP, 0, 4, CROSSED, BOX, nmos_fullbox}};
static TECH_NODES nmos_pm = {
	x_("Metal-Pin"),NMETALP,NONODEPROTO,			/* name */
	K3,K3,											/* size */
	1,nmos_pm_p,									/* ports */
	1,nmos_pm_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* polysilicon-pin */
static TECH_PORTS nmos_pp_p[] = {					/* ports */
	{nmos_pc_p, x_("polysilicon"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON nmos_pp_l[] = {					/* layers */
	{LPOLYP, 0, 4, CROSSED, BOX, nmos_fullbox}};
static TECH_NODES nmos_pp = {
	x_("Polysilicon-Pin"),NPOLYP,NONODEPROTO,		/* name */
	K2,K2,											/* size */
	1,nmos_pp_p,									/* ports */
	1,nmos_pp_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* diffusion-pin */
static TECH_PORTS nmos_pd_p[] = {					/* ports */
	{nmos_pc_d, x_("diffusion"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON nmos_pd_l[] = {					/* layers */
	{LDIFFP, 0, 4, CROSSED, BOX, nmos_fullbox}};
static TECH_NODES nmos_pd = {
	x_("Diffusion-Pin"),NDIFFP,NONODEPROTO,			/* name */
	K2,K2,											/* size */
	1,nmos_pd_p,									/* ports */
	1,nmos_pd_l,									/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE|ARCSHRINK,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* metal-polysilicon-contact */
static TECH_PORTS nmos_mp_p[] = {					/* ports */
	{nmos_pc_pm, x_("metal-poly"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON nmos_mp_l[] = {					/* layers */
	{LMETAL, 0, 4, FILLEDRECT, BOX, nmos_fullbox},
	{LPOLY,  0, 4, FILLEDRECT, BOX, nmos_fullbox},
	{LCUT,  -1, 4, CROSSED,    BOX, nmos_cutbox}};
static TECH_NODES nmos_mp = {
	x_("Metal-Polysilicon-Con"),NMETPOLC,NONODEPROTO,	/* name */
	K4,K4,											/* size */
	1,nmos_mp_p,									/* ports */
	3,nmos_mp_l,									/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,K1,K2,0,0,0,0};					/* characteristics */

/* metal-diffusion-contact */
static TECH_PORTS nmos_md_p[] = {					/* ports */
	{nmos_pc_dm, x_("metal-diff"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON nmos_md_l[] = {					/* layers */
	{LMETAL, 0, 4, FILLEDRECT, BOX, nmos_fullbox},
	{LDIFF,  0, 4, FILLEDRECT, BOX, nmos_fullbox},
	{LCUT,  -1, 4, CROSSED,    BOX, nmos_cutbox}};
static TECH_NODES nmos_md = {
	x_("Metal-Diffusion-Con"),NMETDIFC,NONODEPROTO,	/* name */
	K4,K4,											/* size */
	1,nmos_md_p,									/* ports */
	3,nmos_md_l,									/* layers */
	(NPCONTACT<<NFUNCTIONSH),						/* userbits */
	MULTICUT,K2,K2,K1,K2,0,0,0,0};					/* characteristics */

/* butting-contact */
static TECH_PORTS nmos_but_p[] = {					/* ports */
	{nmos_pc_dm, x_("but-diff"), NOPORTPROTO, (180<<PORTANGLESH)|
		(90<<PORTARANGESH), LEFTIN1, BOTIN1, LEFTIN3, RIGHTIN1},
	{nmos_pc_pm, x_("but-poly"), NOPORTPROTO, (0<<PORTANGLESH)|(90<<PORTARANGESH),
		RIGHTIN2, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON nmos_but_l[] = {				/* layers */
	{LPOLY,  1, 4, FILLEDRECT, BOX, nmos_rightbox},
	{LDIFF,  0, 4, FILLEDRECT, BOX, nmos_leftbox},
	{LMETAL, 0, 4, FILLEDRECT, BOX, nmos_fullbox},
	{LCUT,  -1, 4, CROSSED,    BOX, nmos_in1box}};
static TECH_NODES nmos_but = {
	x_("Butting-Con"),NBUTC,NONODEPROTO,			/* name */
	K6,K4,											/* size */
	2,nmos_but_p,									/* ports */
	4,nmos_but_l,									/* layers */
	(NPCONNECT<<NFUNCTIONSH),						/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* buried-con-cross */
static TECH_PORTS nmos_bu1_p[] = {					/* ports */
	{nmos_pc_d,x_("bur-diff-left"),  NOPORTPROTO, (180<<PORTANGLESH)|
		(45<<PORTARANGESH), LEFTIN1,  BOTIN2, LEFTIN1,  RIGHTIN2},
	{nmos_pc_p,x_("bur-poly-top"),   NOPORTPROTO, (90<<PORTANGLESH)|
		(45<<PORTARANGESH), LEFTIN3,  TOPIN1, RIGHTIN3, TOPIN1},
	{nmos_pc_d,x_("bur-diff-right"), NOPORTPROTO, (0<<PORTANGLESH)|
		(45<<PORTARANGESH), RIGHTIN1, BOTIN2, RIGHTIN1, TOPIN2},
	{nmos_pc_p,x_("bur-poly-bottom"),NOPORTPROTO, (270<<PORTANGLESH)|
		(45<<PORTARANGESH), LEFTIN3,  BOTIN1, RIGHTIN3, BOTIN1}};
static TECH_POLYGON nmos_bu1_l[] = {				/* layers */
	{LPOLY,   1, 4, FILLEDRECT, BOX, nmos_bur1box},
	{LDIFF,   0, 4, FILLEDRECT, BOX, nmos_bur2box},
	{LBURIED, 0, 4, FILLEDRECT, BOX, nmos_fullbox}};
static TECH_NODES nmos_bu1 = {
	x_("Buried-Con-Cross"),NBURC,NONODEPROTO,		/* name */
	K6,K4,											/* size */
	4,nmos_bu1_p,									/* ports */
	3,nmos_bu1_l,									/* layers */
	(NPCONNECT<<NFUNCTIONSH),						/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* transistor */
static TECH_PORTS nmos_tra_p[] = {					/* ports */
	{nmos_pc_p, x_("trans-poly-left"),   NOPORTPROTO, (180<<PORTANGLESH)|
		(85<<PORTARANGESH),                LEFTIN1,  BOTIN3, LEFTIN1,  TOPIN3},
	{nmos_pc_d, x_("trans-diff-top"),    NOPORTPROTO, (90<<PORTANGLESH)|
		(85<<PORTARANGESH)|(1<<PORTNETSH), LEFTIN3,  TOPIN1, RIGHTIN3, TOPIN1},
	{nmos_pc_p, x_("trans-poly-right"),  NOPORTPROTO, (0<<PORTANGLESH)|
		(85<<PORTARANGESH),                RIGHTIN1, BOTIN3, RIGHTIN1, TOPIN3},
	{nmos_pc_d, x_("trans-diff-bottom"), NOPORTPROTO, (270<<PORTANGLESH)|
		(85<<PORTARANGESH)|(2<<PORTNETSH), LEFTIN3,  BOTIN1, RIGHTIN3, BOTIN1}};
static TECH_SERPENT nmos_tra_l[] = {				/* graphical layers */
	{{LDIFF,  1, 4, FILLEDRECT, BOX, nmos_tradbox},  K3, K3,  0,  0},
	{{LPOLY,  0, 4, FILLEDRECT, BOX, nmos_trapbox},  K1, K1, K2, K2},
	{{LTRANS,-1, 4, FILLEDRECT, BOX, nmos_in2box},   K1, K1, K1, K1}};
static TECH_SERPENT nmos_traE_l[] = {				/* electric layers */
	{{LDIFF,  1, 4, FILLEDRECT, BOX, nmos_trd1box},  K3,   0,  0,  0},
	{{LDIFF,  3, 4, FILLEDRECT, BOX, nmos_trd2box},   0,  K3,  0,  0},
	{{LPOLY,  0, 4, FILLEDRECT, BOX, nmos_trapbox},  K1,  K1, K2, K2},
	{{LTRANS,-1, 4, FILLEDRECT, BOX, nmos_in2box},   K1,  K1, K1, K1}};
static TECH_NODES nmos_tra = {
	x_("Transistor"),NTRANS,NONODEPROTO,			/* name */
	K6,K6,											/* size */
	4,nmos_tra_p,									/* ports */
	3,(TECH_POLYGON *)0,							/* layers */
	NODESHRINK|(NPTRANMOS<<NFUNCTIONSH)|HOLDSTRACE,	/* userbits */
	SERPTRANS,4,K1,K1,K2,K1,K1,nmos_tra_l,nmos_traE_l};	/* characteristics */

/* implant-transistor */
static TECH_PORTS nmos_imp_p[] = {					/* ports */
	{nmos_pc_p,x_("imp-trans-poly-left"),   NOPORTPROTO, (180<<PORTANGLESH)|
		(85<<PORTARANGESH),                LEFTIN1,  BOTIN3, LEFTIN1,  TOPIN3},
	{nmos_pc_d,x_("imp-trans-diff-top"),    NOPORTPROTO, (90<<PORTANGLESH)|
		(85<<PORTARANGESH)|(1<<PORTNETSH), LEFTIN3,  TOPIN1, RIGHTIN3, TOPIN1},
	{nmos_pc_p,x_("imp-trans-poly-right"),  NOPORTPROTO, (0<<PORTANGLESH)|
		(85<<PORTARANGESH),                RIGHTIN1, BOTIN3, RIGHTIN1, RIGHTIN3},
	{nmos_pc_d,x_("imp-trans-diff-bottom"), NOPORTPROTO, (270<<PORTANGLESH)|
		(85<<PORTARANGESH)|(2<<PORTNETSH), LEFTIN3,  BOTIN1, RIGHTIN3, BOTIN1}};
static TECH_SERPENT nmos_imp_l[] = {				/* graphical layers */
	{{LDIFF,   1, 4, FILLEDRECT, BOX, nmos_tradbox},  K3, K3,  0,  0},
	{{LPOLY,   0, 4, FILLEDRECT, BOX, nmos_trapbox},  K1, K1, K2, K2},
	{{LIMPLNT,-1, 4, FILLEDRECT, BOX, nmos_inhbox},   H2, H2, H1, H1},
	{{LTRANS, -1, 4, FILLEDRECT, BOX, nmos_in2box},   K1, K1, K1, K1}};
static TECH_SERPENT nmos_impE_l[] = {				/* electric layers */
	{{LDIFF,   1, 4, FILLEDRECT, BOX, nmos_trd1box},  K3,   0,  0,  0},
	{{LDIFF,   3, 4, FILLEDRECT, BOX, nmos_trd2box},   0,  K3,  0,  0},
	{{LPOLY,   0, 4, FILLEDRECT, BOX, nmos_trapbox},  K1,  K1, K2, K2},
	{{LIMPLNT,-1, 4, FILLEDRECT, BOX, nmos_inhbox},   H2,  H2, H1, H1},
	{{LTRANS, -1, 4, FILLEDRECT, BOX, nmos_in2box},   K1,  K1, K1, K1}};
static TECH_NODES nmos_imp = {
	x_("Implant-Transistor"),NITRANS,NONODEPROTO,	/* name */
	K6,K6,											/* size */
	4,nmos_imp_p,									/* ports */
	4,(TECH_POLYGON *)0,							/* layers */
	NODESHRINK|(NPTRADMOS<<NFUNCTIONSH)|HOLDSTRACE,	/* userbits */
	SERPTRANS,5,K1,K1,K2,K1,K1,nmos_imp_l,nmos_impE_l};	/* characteristics */

/* buried-con-cross-S */
static TECH_PORTS nmos_bu2_p[] = {					/* ports */
	{nmos_pc_d,  x_("bur-diff-left"),   NOPORTPROTO,(180<<PORTANGLESH)|
		(45<<PORTARANGESH),   LEFTIN1, BOTIN2, LEFTIN1,  TOPIN2},
	{nmos_pc_p,  x_("bur-poly-top"),    NOPORTPROTO,(90<<PORTANGLESH)|
		(45<<PORTARANGESH),   LEFTIN3, TOPIN1, RIGHTIN1, TOPIN1},
	{nmos_pc_dp, x_("bur-end-right"),   NOPORTPROTO,(0<<PORTANGLESH)|
		(45<<PORTARANGESH),   LEFTIN3, BOTIN2, RIGHTIN1, TOPIN2},
	{nmos_pc_p,  x_("bur-poly-bottom"), NOPORTPROTO,(270<<PORTANGLESH)|
		(45<<PORTARANGESH),   LEFTIN3, BOTIN1, RIGHTIN1, BOTIN1}};
static TECH_POLYGON nmos_bu2_l[] = {				/* layers */
	{LPOLY,   1, 4, FILLEDRECT, BOX, nmos_bur3box},
	{LDIFF,   0, 4, FILLEDRECT, BOX, nmos_bur2box},
	{LBURIED, 0, 4, FILLEDRECT, BOX, nmos_fullbox}};
static TECH_NODES nmos_bu2 = {
	x_("Buried-Con-Cross-S"),NBURCS,NONODEPROTO,	/* name */
	K4,K4,											/* size */
	4,nmos_bu2_p,									/* ports */
	3,nmos_bu2_l,									/* layers */
	(NPCONNECT<<NFUNCTIONSH),						/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* buried-con-cross-T */
static TECH_PORTS nmos_bu3_p[] = {					/* ports */
	{nmos_pc_d, x_("bur-diff-left"),   NOPORTPROTO, (180<<PORTANGLESH)|
		(45<<PORTARANGESH),   LEFTIN1, LEFTIN2,  LEFTIN1,  RIGHTIN2},
	{nmos_pc_p, x_("bur-poly-top"),    NOPORTPROTO, (90<<PORTANGLESH)|
		(45<<PORTARANGESH),   LEFTIN3, RIGHTIN1, RIGHTIN2, TOPIN1},
	{nmos_pc_p, x_("bur-poly-bottom"), NOPORTPROTO, (270<<PORTANGLESH)|
		(45<<PORTARANGESH),   LEFTIN3, LEFTIN1,  RIGHTIN2, BOTIN1}};
static TECH_POLYGON nmos_bu3_l[] = {				/* layers */
	{LPOLY,   1, 4, FILLEDRECT, BOX, nmos_bur4box},
	{LDIFF,   0, 4, FILLEDRECT, BOX, nmos_bur2box},
	{LBURIED, 0, 4, FILLEDRECT, BOX, nmos_fullbox}};
static TECH_NODES nmos_bu3 = {
	x_("Buried-Con-Cross-T"),NBURCT,NONODEPROTO,	/* name */
	K5,K4,											/* size */
	3,nmos_bu3_p,									/* ports */
	3,nmos_bu3_l,									/* layers */
	(NPCONNECT<<NFUNCTIONSH),						/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* buried-con-polysurr */
static TECH_PORTS nmos_bu4_p[] = {					/* ports */
	{nmos_pc_d, x_("bur-diff-left"), NOPORTPROTO, (180<<PORTANGLESH)|
		(45<<PORTARANGESH),  LEFTIN1, BOTIN2, LEFTIN1,  TOPIN2},
	{nmos_pc_p, x_("bur-poly-1"),    NOPORTPROTO, (0<<PORTANGLESH)|
		(135<<PORTARANGESH), LEFTIN3, BOTIN1, RIGHTIN1, TOPIN1},
	{nmos_pc_p, x_("bur-poly-2"),    NOPORTPROTO, (0<<PORTANGLESH)|
		(135<<PORTARANGESH), LEFTIN3, BOTIN1, RIGHTIN1, TOPIN1},
	{nmos_pc_p, x_("bur-poly-3"),    NOPORTPROTO, (0<<PORTANGLESH)|
		(135<<PORTARANGESH), LEFTIN3, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON nmos_bu4_l[] = {				/* layers */
	{LPOLY,   2, 4, FILLEDRECT, BOX, nmos_bur3box},
	{LDIFF,   0, 4, FILLEDRECT, BOX, nmos_bur5box},
	{LBURIED, 0, 4, FILLEDRECT, BOX, nmos_fullbox}};
static TECH_NODES nmos_bu4 = {
	x_("Buried-Con-Polysurr"),NBURCPS,NONODEPROTO,	/* name */
	K5,K4,											/* size */
	4,nmos_bu4_p,									/* ports */
	3,nmos_bu4_l,									/* layers */
	(NPCONNECT<<NFUNCTIONSH),						/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* buried-con-diffsurr-I */
static TECH_PORTS nmos_bu5_p[] = {					/* ports */
	{nmos_pc_d, x_("bur-diff-left"), NOPORTPROTO, (180<<PORTANGLESH)|
		(45<<PORTARANGESH), LEFTIN1,  BOTIN2, LEFTIN1,  TOPIN2},
	{nmos_pc_p, x_("bur-poly-right"),NOPORTPROTO, (0<<PORTANGLESH)|
		(45<<PORTARANGESH), RIGHTIN1, BOTIN2, RIGHTIN1, TOPIN2}};
static TECH_POLYGON nmos_bu5_l[] = {				/* layers */
	{LPOLY,   1, 4, FILLEDRECT, BOX,    nmos_bur6box},
	{LDIFF,   0, 8, FILLED,     POINTS, nmos_bur7box},
	{LBURIED, 0, 4, FILLEDRECT, BOX,    nmos_fullbox}};
static TECH_NODES nmos_bu5 = {
	x_("Buried-Con-Diffsurr-I"),NBURCDSI,NONODEPROTO,	/* name */
	K5,K4,											/* size */
	2,nmos_bu5_p,									/* ports */
	3,nmos_bu5_l,									/* layers */
	(NPCONNECT<<NFUNCTIONSH),						/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* buried-con-diffsurr-T */
static TECH_PORTS nmos_bu6_p[] = {					/* ports */
	{nmos_pc_d, x_("bur-diff-left"), NOPORTPROTO, (180<<PORTANGLESH)|
		(45<<PORTARANGESH), LEFTIN1,  BOTIN1,   LEFTIN1,  TOPIN3},
	{nmos_pc_p, x_("bur-poly-top"),  NOPORTPROTO, (90<<PORTANGLESH)|
		(45<<PORTARANGESH), LEFTIN3,  RIGHTIN1, RIGHTIN3, TOPIN1},
	{nmos_pc_d, x_("bur-diff-right"),NOPORTPROTO, (0<<PORTANGLESH)|
		(45<<PORTARANGESH), RIGHTIN1, BOTIN1,   RIGHTIN1, TOPIN3}};
static TECH_POLYGON nmos_bu6_l[] = {				/* layers */
	{LPOLY,   1, 4, FILLEDRECT, BOX,    nmos_bur9box},
	{LBURIED, 0, 4, FILLEDRECT, BOX,    nmos_fullbox},
	{LDIFF,   0, 8, FILLED,     POINTS, nmos_bur11box}};
static TECH_NODES nmos_bu6 = {
	x_("Buried-Con-Diffsurr-T"),NBURCDST,NONODEPROTO,	/* name */
	K6,K4,											/* size */
	3,nmos_bu6_p,									/* ports */
	3,nmos_bu6_l,									/* layers */
	(NPCONNECT<<NFUNCTIONSH),						/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* buried-con-diffsurr-L */
static TECH_PORTS nmos_bu7_p[] = {					/* ports */
	{nmos_pc_d, x_("bur-diff-left"), NOPORTPROTO, (180<<PORTANGLESH)|
		(45<<PORTARANGESH), LEFTIN1, BOTIN1, LEFTIN1,  TOPIN3},
	{nmos_pc_p, x_("bur-poly-top"),  NOPORTPROTO, (90<<PORTANGLESH)|
		(45<<PORTARANGESH), LEFTIN3, TOPIN1, RIGHTIN2, TOPIN1}};
static TECH_POLYGON nmos_bu7_l[] = {				/* layers */
	{LPOLY,   1, 4, FILLEDRECT, BOX,    nmos_bur8box},
	{LBURIED, 0, 4, FILLEDRECT, BOX,    nmos_fullbox},
	{LDIFF,   0, 6, FILLED,     POINTS, nmos_bur10box}};
static TECH_NODES nmos_bu7 = {
	x_("Buried-Con-Diffsurr-L"),NBURCDSL,NONODEPROTO,	/* name */
	K5,K4,											/* size */
	2,nmos_bu7_p,									/* ports */
	3,nmos_bu7_l,									/* layers */
	(NPCONNECT<<NFUNCTIONSH),						/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* metal-node */
static TECH_PORTS nmos_m_p[] = {					/* ports */
	{nmos_pc_m, x_("metal"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1H, BOTIN1H, RIGHTIN1H, TOPIN1H}};
static TECH_POLYGON nmos_m_l[] = {					/* layers */
	{LMETAL, 0, 4, FILLEDRECT, BOX, nmos_fullbox}};
static TECH_NODES nmos_m = {
	x_("Metal-Node"),NMETALN,NONODEPROTO,			/* name */
	K3,K3,											/* size */
	1,nmos_m_p,										/* ports */
	1,nmos_m_l,										/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* polysilicon-node */
static TECH_PORTS nmos_p_p[] = {					/* ports */
	{nmos_pc_p, x_("polysilicon"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON nmos_p_l[] = {					/* layers */
	{LPOLY, 0, 4, FILLEDRECT, BOX, nmos_fullbox}};
static TECH_NODES nmos_p = {
	x_("Polysilicon-Node"),NPOLYN,NONODEPROTO,		/* name */
	K2,K2,											/* size */
	1,nmos_p_p,										/* ports */
	1,nmos_p_l,										/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* diffusion-node */
static TECH_PORTS nmos_d_p[] = {					/* ports */
	{nmos_pc_d, x_("diffusion"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN1, BOTIN1, RIGHTIN1, TOPIN1}};
static TECH_POLYGON nmos_d_l[] = {					/* layers */
	{LDIFF, 0, 4, FILLEDRECT, BOX, nmos_fullbox}};
static TECH_NODES nmos_d = {
	x_("Diffusion-Node"),NDIFFN,NONODEPROTO,		/* name */
	K2,K2,											/* size */
	1,nmos_d_p,										/* ports */
	1,nmos_d_l,										/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* implant-node */
static TECH_PORTS nmos_i_p[] = {					/* ports */
	{nmos_pc_null, x_("implant"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON nmos_i_l[] = {					/* layers */
	{LIMPLNT, 0, 4, FILLEDRECT, BOX, nmos_fullbox}};
static TECH_NODES nmos_i = {
	x_("Implant-Node"),NIMPN,NONODEPROTO,			/* name */
	K2,K2,											/* size */
	1,nmos_i_p,										/* ports */
	1,nmos_i_l,										/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* contact-node */
static TECH_PORTS nmos_c_p[] = {					/* ports */
	{nmos_pc_null, x_("cut"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON nmos_c_l[] = {					/* layers */
	{LCUT, 0, 4, CROSSED, BOX, nmos_fullbox}};
static TECH_NODES nmos_c = {
	x_("Cut-Node"),NCUTN,NONODEPROTO,				/* name */
	K2,K2,											/* size */
	1,nmos_c_p,										/* ports */
	1,nmos_c_l,										/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* buried-node */
static TECH_PORTS nmos_b_p[] = {					/* ports */
	{nmos_pc_null, x_("buried"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON nmos_b_l[] = {					/* layers */
	{LBURIED, 0, 4, FILLEDRECT, BOX, nmos_fullbox}};
static TECH_NODES nmos_b = {
	x_("Buried-Node"),NBURN,NONODEPROTO,			/* name */
	K2,K2,											/* size */
	1,nmos_b_p,										/* ports */
	1,nmos_b_l,										/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* overglass-node */
static TECH_PORTS nmos_o_p[] = {					/* ports */
	{nmos_pc_null, x_("overglass"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON nmos_o_l[] = {					/* layers */
	{LOVERGL, 0, 4, FILLEDRECT, BOX, nmos_fullbox}};
static TECH_NODES nmos_o = {
	x_("Overglass-Node"),NOVERGN,NONODEPROTO,		/* name */
	K2,K2,											/* size */
	1,nmos_o_p,										/* ports */
	1,nmos_o_l,										/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* light-implant-node */
static TECH_PORTS nmos_j_p[] = {					/* ports */
	{nmos_pc_null, x_("light-implant"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON nmos_j_l[] = {					/* layers */
	{LLIMP, 0, 4, FILLEDRECT, BOX, nmos_fullbox}};
static TECH_NODES nmos_j = {
	x_("Light-Implant-Node"),NLIMPN,NONODEPROTO,	/* name */
	K2,K2,											/* size */
	1,nmos_j_p,										/* ports */
	1,nmos_j_l,										/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* oversize-contact-node */
static TECH_PORTS nmos_v_p[] = {					/* ports */
	{nmos_pc_null, x_("oversize-contact"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON nmos_v_l[] = {					/* layers */
	{LOVRCON, 0, 4, CLOSEDRECT, BOX, nmos_fullbox}};
static TECH_NODES nmos_v = {
	x_("Oversize-Cut-Node"),NOVRCUTN,NONODEPROTO,	/* name */
	K2,K2,											/* size */
	1,nmos_v_p,										/* ports */
	1,nmos_v_l,										/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* hard-enhancement-node */
static TECH_PORTS nmos_h_p[] = {					/* ports */
	{nmos_pc_null, x_("hard-enhancement"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON nmos_h_l[] = {					/* layers */
	{LHENH, 0, 4, FILLEDRECT, BOX, nmos_fullbox}};
static TECH_NODES nmos_h = {
	x_("Hard-Enhancement-Node"),NHENHN,NONODEPROTO,	/* name */
	K2,K2,											/* size */
	1,nmos_h_p,										/* ports */
	1,nmos_h_l,										/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* light-enhancement-node */
static TECH_PORTS nmos_l_p[] = {					/* ports */
	{nmos_pc_null, x_("light-enhancement"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON nmos_l_l[] = {					/* layers */
	{LLENH, 0, 4, FILLEDRECT, BOX, nmos_fullbox}};
static TECH_NODES nmos_l = {
	x_("Light-Enhancement-Node"),NLENHN,NONODEPROTO,/* name */
	K2,K2,											/* size */
	1,nmos_l_p,										/* ports */
	1,nmos_l_l,										/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

TECH_NODES *nmos_nodeprotos[NODEPROTOCOUNT+1] = {
	&nmos_pm, &nmos_pp, &nmos_pd, &nmos_mp, &nmos_md, &nmos_but,
	&nmos_bu1, &nmos_tra, &nmos_imp, &nmos_bu2, &nmos_bu3, &nmos_bu4,
	&nmos_bu5, &nmos_bu6, &nmos_bu7, &nmos_m, &nmos_p, &nmos_d, &nmos_i,
	&nmos_c, &nmos_b, &nmos_o, &nmos_j, &nmos_v, &nmos_h, &nmos_l, ((TECH_NODES *)-1)};

static INTBIG nmos_node_widoff[NODEPROTOCOUNT*4] = {0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, K2,K2,K2,K2, K2,K2,K2,K2, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

/******************** SIMULATION VARIABLES ********************/

/* for SPICE simulation */
#define NMOS_MIN_RESIST	50.0f		/* minimum resistance consider */
#define NMOS_MIN_CAPAC	0.04f		/* minimum capacitance consider */
static float nmos_sim_spice_resistance[MAXLAYERS] = {   /* per square micron */
	0.03f /* metal */,    50.0f /* poly */,   10.0f /* diff */,
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
static float nmos_sim_spice_capacitance[MAXLAYERS] = {  /* per square micron */
	0.03f /* metal */,    0.04f /* poly */,   0.1f /* diff */,
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
static CHAR *nmos_sim_spice_header_level1[] = {
	x_("*NMOS 4UM PROCESS"),
	x_(".OPTIONS DEFL=4UM DEFW=4UM DEFAS=80PM^2 DEFAD=80PM^2"),
	x_(".MODEL N NMOS LEVEL=1 VTO=1.1 KP=33UA/V^2 TOX=68NM GAMMA=.41"),
	x_("+             LAMBDA=0.05 CGSO=0.18NF/M CGDO=0.18NF/M LD=0.4UM"),
	x_("+             JS=.2A/M^2 CJ=.11MF/M^2"),
	x_(".MODEL D NMOS LEVEL=1 VTO=-3.4 KP=31UA/V^2 TOX=68NM GAMMA=.44"),
	x_("+             LAMBDA=0.05 CGSO=0.18NF/M CGDO=0.18NF/M LD=0.4UM"),
	x_("+             JS=.2A/M^2 CJ=.11MF/M^2"),
	x_(".MODEL DIFFCAP D CJO=.11MF/M^2"),
	NOSTRING};
static CHAR *nmos_sim_spice_header_level3[] = {
	x_("*NMOS 4UM PROCESS"),
	x_(".OPTIONS DEFL=4UM DEFW=4UM DEFAS=80PM^2 DEFAD=80PM^2"),
	x_("* RSH SET TO ZERO (MOSIS: RSH = 12)"),
	x_(".MODEL N NMOS LEVEL=3 VTO=0.849 LD=0.17U KP=2.98E-5 GAMMA=0.552"),
	x_("+PHI=0.6 TOX=0.601E-7 NSUB=2.11E15 NSS=0 NFS=8.89E11 TPG=1 XJ=7.73E-7"),
	x_("+UO=400 UEXP=1E-3 UCRIT=1.74E5 VMAX=1E5 NEFF=1E-2 DELTA=1.19"),
	x_("+THETA=9.24E-3 ETA=0.77 KAPPA=3.25 RSH=0 CGSO=1.6E-10 CGDO=1.6E-10"),
	x_("+CGBO=1.7E-10 CJ=1.1E-4 MJ=0.5 CJSW=1E-9"),
	x_(".MODEL D NMOS LEVEL=3 VTO=-3.07 LD=0.219U KP=2.76E-5 GAMMA=0.315"),
	x_("+PHI=0.6 TOX=0.601E-7 NSUB=8.76E14 NSS=0 NFS=4.31E12 TPG=1 XJ=0.421U"),
	x_("+UO=650 UEXP=1E-3 UCRIT=8.05E5 VMAX=1.96E5 NEFF=1E-2 DELTA=2.41"),
	x_("+THETA=0 ETA=2.0 KAPPA=0.411 RSH=0 CGSO=1.6E-10 CGDO=1.6E-10"),
	x_("+CGBO=1.7E-10 CJ=1.1E-4 MJ=0.5 CJSW=1E-9"),
	x_("*MOSIS IS NOT RESPONSIBLE FOR THE FOLLOWING DIOD DATA"),
	x_(".MODEL DIFFCAP D CJO=.11MF/m^2"),
	NOSTRING};

/******************** VARIABLE AGGREGATION ********************/

TECH_VARIABLES nmos_variables[] =
{
	/* set general information about the technology */
	{x_("TECH_layer_names"), (CHAR *)nmos_layer_names, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_layer_function"), (CHAR *)nmos_layer_function, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_node_width_offset"), (CHAR *)nmos_node_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|((NODEPROTOCOUNT*4)<<VLENGTHSH)},

	/* set information for the USER tool */
	{x_("USER_layer_letters"), (CHAR *)nmos_layer_letters, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("USER_color_map"), (CHAR *)nmos_colmap, 0.0,
		VCHAR|VDONTSAVE|VISARRAY|((sizeof nmos_colmap)<<VLENGTHSH)},

	/* set information for the I/O tool */
	{x_("IO_cif_layer_names"), (CHAR *)nmos_cif_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},

	/* set information for the DRC tool */
	{x_("DRC_min_unconnected_distances"), (CHAR *)nmos_unconnectedtable, 0.0,
		 VFRACT|VDONTSAVE|VISARRAY|
			(((sizeof nmos_unconnectedtable)/SIZEOFINTBIG)<<VLENGTHSH)},
	{x_("DRC_min_connected_distances"), (CHAR *)nmos_connectedtable, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|
			(((sizeof nmos_connectedtable) / SIZEOFINTBIG)<<VLENGTHSH)},

	/* set information for the SIM tool (SPICE) */
	{x_("SIM_spice_min_resistance"), 0, NMOS_MIN_RESIST, VFLOAT|VDONTSAVE},
	{x_("SIM_spice_min_capacitance"), 0, NMOS_MIN_CAPAC, VFLOAT|VDONTSAVE},
	{x_("SIM_spice_resistance"), (CHAR *)nmos_sim_spice_resistance, 0.0,
		VFLOAT|VISARRAY|(MAXLAYERS<<VLENGTHSH)|VDONTSAVE},
	{x_("SIM_spice_capacitance"), (CHAR *)nmos_sim_spice_capacitance, 0.0,
		VFLOAT|VISARRAY|(MAXLAYERS<<VLENGTHSH)|VDONTSAVE},
	{x_("SIM_spice_header_level1"), (CHAR *)nmos_sim_spice_header_level1, 0.0,
		VSTRING|VDONTSAVE|VISARRAY},
	{x_("SIM_spice_header_level3"), (CHAR *)nmos_sim_spice_header_level3, 0.0,
		VSTRING|VDONTSAVE|VISARRAY},
	{NULL, NULL, 0.0, 0}
};

/******************** ROUTINES ********************/

BOOLEAN nmos_initprocess(TECHNOLOGY *tech, INTBIG pass)
{
	Q_UNUSED( tech );
	Q_UNUSED( pass );
	return(FALSE);
}
