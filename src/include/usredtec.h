/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usredtec.h
 * User interface technology editor: header file
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

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
extern "C"
{
#endif

/* the meaning of "EDTEC_option" on nodes */
#define LAYERCOLOR      1					/* color (layer cell) */
#define LAYERSTYLE      2					/* style (layer cell) */
#define LAYERCIF        3					/* CIF name (layer cell) */
#define LAYERFUNCTION   4					/* function (layer cell) */
#define LAYERLETTERS    5					/* letters (layer cell) */
#define LAYERPATTERN    6					/* pattern (layer cell) */
#define LAYERPATCONT    7					/* pattern control (layer cell) */
#define LAYERPATCH      8					/* patch of layer (node/arc cell) */
#define ARCFUNCTION     9					/* function (arc cell) */
#define NODEFUNCTION   10					/* function (node cell) */
#define ARCFIXANG      11					/* fixed-angle (arc cell) */
#define ARCWIPESPINS   12					/* wipes pins (arc cell) */
#define ARCNOEXTEND    13					/* end extension (arc cell) */
#define TECHLAMBDA     14					/* lambda (info cell) */
#define TECHDESCRIPT   15					/* description (info cell) */
#define NODESERPENTINE 16					/* serpentine MOS trans (node cell) */
#define LAYERDRCMINWID 17					/* DRC minimum width (layer cell, OBSOLETE) */
#define PORTOBJ        18					/* port object (node cell) */
#define HIGHLIGHTOBJ   19					/* highlight object (node/arc cell) */
#define LAYERGDS       20					/* Calma GDS-II layer (layer cell) */
#define NODESQUARE     21					/* square node (node cell) */
#define NODEWIPES      22					/* pin node can disappear (node cell) */
#define ARCINC         23					/* increment for arc angles (arc cell) */
#define NODEMULTICUT   24					/* separation of multiple contact cuts (node cell) */
#define NODELOCKABLE   25					/* lockable primitive (node cell) */
#define CENTEROBJ      26					/* grab point object (node cell) */
#define LAYERSPIRES    27					/* SPICE resistance (layer cell) */
#define LAYERSPICAP    28					/* SPICE capacitance (layer cell) */
#define LAYERSPIECAP   29					/* SPICE edge capacitance (layer cell) */
#define LAYERDXF       30					/* DXF layer (layer cell) */
#define LAYER3DHEIGHT  31					/* 3D height (layer cell) */
#define LAYER3DTHICK   32					/* 3D thickness (layer cell) */
#define LAYERPRINTCOL  33					/* print colors (layer cell) */

/* the strings that appear in the technology editor */
#define TECEDNODETEXTCOLOR      x_("Color: ")
#define TECEDNODETEXTSTYLE      x_("Style: ")
#define TECEDNODETEXTFUNCTION   x_("Function: ")
#define TECEDNODETEXTLETTERS    x_("Layer letters: ")
#define TECEDNODETEXTGDS        x_("GDS-II Layer: ")
#define TECEDNODETEXTCIF        x_("CIF Layer: ")
#define TECEDNODETEXTDXF        x_("DXF Layer(s): ")
#define TECEDNODETEXTSPICERES   x_("SPICE Resistance: ")
#define TECEDNODETEXTSPICECAP   x_("SPICE Capacitance: ")
#define TECEDNODETEXTSPICEECAP  x_("SPICE Edge Capacitance: ")
#define TECEDNODETEXTDRCMINWID  x_("DRC Minimum Width: ")
#define TECEDNODETEXT3DHEIGHT   x_("3D Height: ")
#define TECEDNODETEXT3DTHICK    x_("3D Thickness: ")
#define TECEDNODETEXTPRINTCOL   x_("Print colors: ")

typedef struct Ilist
{
	CHAR  *name;
	CHAR  *constant;
	INTBIG value;
} LIST;


#define NOSAMPLE ((SAMPLE *)-1)

typedef struct Isample
{
	NODEINST        *node;					/* true node used for sample */
	NODEPROTO       *layer;					/* type of node used for sample */
	INTBIG           xpos, ypos;			/* center of sample */
	struct Isample  *assoc;					/* associated sample in first example */
	struct Irule    *rule;					/* rule associated with this sample */
	struct Iexample *parent;				/* example containing this sample */
	struct Isample  *nextsample;			/* next sample in list */
} SAMPLE;


#define NOEXAMPLE ((EXAMPLE *)-1)

typedef struct Iexample
{
	SAMPLE          *firstsample;			/* head of list of samples in example */
	SAMPLE          *studysample;			/* sample under analysis */
	INTBIG           lx, hx, ly, hy;		/* bounding box of example */
	struct Iexample *nextexample;			/* next example in list */
} EXAMPLE;


/* port connections */
#define NOPCON ((PCON *)-1)

typedef struct Ipcon
{
	INTBIG       *connects;
	INTBIG       *assoc;
	INTBIG        total;
	INTBIG        pcindex;
	struct Ipcon *nextpcon;
} PCON;


/* rectangle rules */
#define NORULE ((RULE *)-1)

typedef struct Irule
{
	INTBIG       *value;					/* data points for rule */
	INTBIG        count;					/* number of points in rule */
	INTBIG        istext;					/* nonzero if text at end of rule */
	INTBIG        rindex;					/* identifier for this rule */
	BOOLEAN       used;						/* nonzero if actually used */
	BOOLEAN       multicut;					/* nonzero if this is multiple cut */
	INTBIG        multixs, multiys;			/* size of multicut */
	INTBIG        multiindent, multisep;	/* indent and separation of multicuts */
	struct Irule *nextrule;
} RULE;


/* the meaning of "us_tecflags" */
#define HASDRCMINWID         01				/* has DRC minimum width information */
#define HASDRCMINWIDR        02				/* has DRC minimum width information */
#define HASCOLORMAP          04				/* has color map */
#define HASARCWID           010				/* has arc width offset factors */
#define HASCIF              020				/* has CIF layers */
#define HASDXF              040				/* has DXF layers */
#define HASGDS             0100				/* has Calma GDS-II layers */
#define HASGRAB            0200				/* has grab point information */
#define HASSPIRES          0400				/* has SPICE resistance information */
#define HASSPICAP         01000				/* has SPICE capacitance information */
#define HASSPIECAP        02000				/* has SPICE edge capacitance information */
#define HAS3DINFO         04000				/* has 3D height/thickness information */
#define HASCONDRC        010000				/* has connected design rules */
#define HASCONDRCR       020000				/* has connected design rules reasons */
#define HASUNCONDRC      040000				/* has unconnected design rules */
#define HASUNCONDRCR    0100000				/* has unconnected design rules reasons */
#define HASCONDRCW      0200000				/* has connected wide design rules */
#define HASCONDRCWR     0400000				/* has connected wide design rules reasons */
#define HASUNCONDRCW   01000000				/* has unconnected wide design rules */
#define HASUNCONDRCWR  02000000				/* has unconnected wide design rules reasons */
#define HASCONDRCM     04000000				/* has connected multicut design rules */
#define HASCONDRCMR   010000000				/* has connected multicut design rules reasons */
#define HASUNCONDRCM  020000000				/* has unconnected multicut design rules */
#define HASUNCONDRCMR 040000000				/* has unconnected multicut design rules reasons */
#define HASEDGEDRC   0100000000				/* has edge design rules */
#define HASEDGEDRCR  0200000000				/* has edge design rules reasons */
#define HASMINNODE   0400000000				/* has minimum node size */
#define HASMINNODER 01000000000				/* has minimum node size reasons */
#define HASPRINTCOL 02000000000				/* has print colors */

/* additional technology variables */
#define NOTECHVAR ((TECHVAR *)-1)

typedef struct Itechvar
{
	CHAR            *varname;
	struct Itechvar *nexttechvar;
	BOOLEAN          changed;
	INTBIG           ival;
	float            fval;
	CHAR            *sval;
	INTBIG           vartype;
	CHAR            *description;
} TECHVAR;

extern TECHVAR us_knownvars[];

/* for describing special text in a cell */
typedef struct
{
	NODEINST *ni;
	void     *value;
	INTBIG    x, y;
	INTBIG    funct;
} SPECIALTEXTDESCR;
extern SPECIALTEXTDESCR us_tecednodetexttable[];
extern SPECIALTEXTDESCR us_tecedarctexttable[];
extern SPECIALTEXTDESCR us_tecedmisctexttable[];

/* the globals that define a technology */
extern INTBIG           us_teceddrclayers;
extern CHAR           **us_teceddrclayernames;

extern LIST             us_teclayer_functions[];
extern LIST             us_tecarc_functions[];

/* prototypes for intramodule routines */
void       us_tecedcompact(NODEPROTO *cell);
void       us_tecedgetbbox(NODEINST *ni, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy);
EXAMPLE   *us_tecedgetexamples(NODEPROTO *np, BOOLEAN isnode);
INTBIG     us_teceditfindsequence(LIBRARY **dependentlibs, INTBIG dependentlibcount,
			CHAR *match, CHAR *seqname, NODEPROTO ***sequence);
INTBIG     us_teceditgetdependents(LIBRARY *lib, LIBRARY ***liblist);
BOOLEAN    us_teceditgetlayerinfo(NODEPROTO *np, GRAPHICS *desc, CHAR **cif, INTBIG *func,
			CHAR **layerletters, CHAR **dxf, CHAR **gds, float *spires, float *spicap,
			float *spiecap, INTBIG *drcminwid, INTBIG *height3d, INTBIG *thick3d, INTBIG *printcol);
void       us_teceditsetpatch(NODEINST *ni, GRAPHICS *desc);
void       us_tecedmakearc(NODEPROTO *np, INTBIG func, INTBIG fixang, INTBIG wipes,
			INTBIG noextend, INTBIG anginc);
void       us_tecedmakeinfo(NODEPROTO *np, INTBIG lambda, CHAR *description);
void       us_tecedmakelayer(NODEPROTO *np, INTBIG colorindex, UINTSML stip[16], INTBIG style,
			CHAR *ciflayer, INTBIG functionindex, CHAR *layerletters, CHAR *dxf,
			CHAR *gds, float spires, float spicap, float spiecap,
			INTBIG height3d, INTBIG thick3d, INTBIG *printcolors);
LIBRARY   *us_tecedmakelibfromtech(TECHNOLOGY *tech);
void       us_tecedmakenode(NODEPROTO *np, INTBIG func, BOOLEAN serp, BOOLEAN square, BOOLEAN wipes,
			BOOLEAN lockable, INTBIG multicutsep);
void       us_tecedpointout(NODEINST *ni, NODEPROTO *np);
CHAR      *us_tecedsamplename(NODEPROTO *layernp);
void       us_tecedswapports(INTBIG *p1, INTBIG *p2, TECH_NODES *tlist);
void       us_tecfromlibinit(LIBRARY *lib, CHAR *techname, INTBIG dumpformat);
void       us_tecedfreeexamples(EXAMPLE *ne);
void       us_tecedgetlayernamelist(void);
void       us_tecedloaddrcmessage(DRCRULES *rules, LIBRARY *lib);
void       us_teceditgetdrcarrays(VARIABLE *var, DRCRULES *rules);
NODEPROTO *us_tecedgetlayer(NODEINST *ni);
CHAR      *us_tecedgetportname(NODEINST *ni);
void       us_teceditgetprintcol(VARIABLE *var, INTBIG *r, INTBIG *g, INTBIG *b, INTBIG *o, INTBIG *f);
void       us_tecedfindspecialtext(NODEPROTO *np, SPECIALTEXTDESCR *table);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
