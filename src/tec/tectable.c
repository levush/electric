/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tectable.c
 * Technology tables
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

#include "global.h"

#include "tecart.h"
extern COMCOMP art_parse;
extern GRAPHICS *art_layers[];
extern TECH_ARCS *art_arcprotos[];
extern TECH_NODES *art_nodeprotos[];
extern TECH_VARIABLES art_variables[];

extern GRAPHICS *bicmos_layers[];
extern TECH_ARCS *bicmos_arcprotos[];
extern TECH_NODES *bicmos_nodeprotos[];
extern TECH_VARIABLES bicmos_variables[];
BOOLEAN bicmos_initprocess(TECHNOLOGY*, INTBIG);

extern GRAPHICS *bipolar_layers[];
extern TECH_ARCS *bipolar_arcprotos[];
extern TECH_NODES *bipolar_nodeprotos[];
extern TECH_VARIABLES bipolar_variables[];
BOOLEAN bipolar_initprocess(TECHNOLOGY*, INTBIG);

extern GRAPHICS *cmos_layers[];
extern TECH_ARCS *cmos_arcprotos[];
extern TECH_NODES *cmos_nodeprotos[];
extern TECH_VARIABLES cmos_variables[];
BOOLEAN cmos_initprocess(TECHNOLOGY*, INTBIG);

#include "teccmosdodn.h"
extern GRAPHICS *dodcmosn_layers[];
extern TECH_ARCS *dodcmosn_arcprotos[];
extern TECH_NODES *dodcmosn_nodeprotos[];
extern TECH_VARIABLES dodcmosn_variables[];

extern GRAPHICS *efido_layers[];
extern TECH_ARCS *efido_arcprotos[];
extern TECH_NODES *efido_nodeprotos[];
extern TECH_VARIABLES efido_variables[];
BOOLEAN efido_initprocess(TECHNOLOGY*, INTBIG);

#include "tecgem.h"
extern GRAPHICS *gem_layers[];
extern TECH_ARCS *gem_arcprotos[];
extern TECH_NODES *gem_nodeprotos[];
extern TECH_VARIABLES gem_variables[];

#include "tecgen.h"
extern GRAPHICS *gen_layers[];
extern TECH_ARCS *gen_arcprotos[];
extern TECH_NODES *gen_nodeprotos[];
extern TECH_VARIABLES gen_variables[];

#include "tecmocmos.h"
extern COMCOMP mocmos_parse;
extern GRAPHICS *mocmos_layers[];
extern TECH_ARCS *mocmos_arcprotos[];
extern TECH_NODES *mocmos_nodeprotos[];
extern TECH_VARIABLES mocmos_variables[];

#include "tecmocmosold.h"
extern COMCOMP mocmosold_parse;
extern GRAPHICS *mocmosold_layers[];
extern TECH_ARCS *mocmosold_arcprotos[];
extern TECH_NODES *mocmosold_nodeprotos[];
extern TECH_VARIABLES mocmosold_variables[];

#include "tecmocmossub.h"
extern COMCOMP mocmossub_parse;
extern GRAPHICS *mocmossub_layers[];
extern TECH_ARCS *mocmossub_arcprotos[];
extern TECH_NODES *mocmossub_nodeprotos[];
extern TECH_VARIABLES mocmossub_variables[];

extern GRAPHICS *nmos_layers[];
extern TECH_ARCS *nmos_arcprotos[];
extern TECH_NODES *nmos_nodeprotos[];
extern TECH_VARIABLES nmos_variables[];
BOOLEAN nmos_initprocess(TECHNOLOGY*, INTBIG);

extern GRAPHICS *pcb_layers[];
extern TECH_ARCS *pcb_arcprotos[];
extern TECH_NODES *pcb_nodeprotos[];
extern TECH_VARIABLES pcb_variables[];
BOOLEAN pcb_initprocess(TECHNOLOGY*, INTBIG);

#include "tecrcmos.h"
extern GRAPHICS *rcmos_layers[];
extern TECH_ARCS *rcmos_arcprotos[];
extern TECH_NODES *rcmos_nodeprotos[];
extern TECH_VARIABLES rcmos_variables[];

#include "tecschem.h"
extern COMCOMP sch_parse;
extern GRAPHICS *sch_layers[];
extern TECH_ARCS *sch_arcprotos[];
extern TECH_NODES *sch_nodeprotos[];
extern TECH_VARIABLES sch_variables[];

#include "tecfpga.h"
extern COMCOMP fpga_parse;
extern GRAPHICS *fpga_layers[];
extern TECH_ARCS *fpga_arcprotos[];
extern TECH_NODES *fpga_nodeprotos[];
extern TECH_VARIABLES fpga_variables[];

#ifdef FORCESUNTOOLS
extern GRAPHICS *ziptronics_layers[];
extern TECH_ARCS *ziptronics_arcprotos[];
extern TECH_NODES *ziptronics_nodeprotos[];
extern TECH_VARIABLES ziptronics_variables[];

extern GRAPHICS *epic7s_layers[];
extern TECH_ARCS *epic7s_arcprotos[];
extern TECH_NODES *epic7s_nodeprotos[];
extern TECH_VARIABLES epic7s_variables[];
#endif

/*
 * the first entry in this table MUST BE THE GENERIC TECHNOLOGY!
 */
TECHNOLOGY el_technologylist[] =
{
	/* Generic */
	{x_("generic"), 0, 2000, NONODEPROTO,NOARCPROTO,NOVARIABLE,0,NOCOMCOMP,NOCLUSTER,	/* info */
	N_("Miscellaneous interconnect, constraint, and glyph"),						/* description */
	0, gen_layers, 0, gen_arcprotos, 0, gen_nodeprotos, gen_variables,				/* tables */
	gen_initprocess, gen_termprocess, 0, 0,											/* control routines */
	gen_nodepolys, gen_nodepolys, gen_shapenodepoly, gen_shapenodepoly, gen_allnodepolys, gen_allnodepolys, 0,	/* node routines */
	gen_shapeportpoly,																/* port routine */
	0, 0, 0, 0,																		/* arc routines */
	NOTECHNOLOGY, NONSTANDARD|STATICTECHNOLOGY, 0, 0},								/* miscellaneous */

	/* NMOS */
	{x_("nmos"), 0, 4000, NONODEPROTO,NOARCPROTO,NOVARIABLE,0,NOCOMCOMP,NOCLUSTER,		/* info */
	N_("n-channel MOS (from Mead & Conway)"),										/* description */
	0, nmos_layers, 0, nmos_arcprotos, 0, nmos_nodeprotos, nmos_variables,			/* tables */
	nmos_initprocess, 0, 0, 0,														/* control routines */
	0, 0, 0, 0, 0, 0, 0,															/* node routines */
	0,																				/* port routine */
	0, 0, 0, 0,																		/* arc routines */
	NOTECHNOLOGY, NONEGATEDARCS|STATICTECHNOLOGY, 0, 0},							/* miscellaneous */

	/* CMOS */
	{x_("cmos"), 0, 4000, NONODEPROTO,NOARCPROTO,NOVARIABLE,0,NOCOMCOMP,NOCLUSTER,		/* info */
	N_("Complementary MOS (old, N-Well, from Griswold)"),							/* description */
	0, cmos_layers, 0, cmos_arcprotos, 0, cmos_nodeprotos, cmos_variables,			/* tables */
	cmos_initprocess, 0, 0, 0,														/* control routines */
	0, 0, 0, 0, 0, 0, 0,															/* node routines */
	0,																				/* port routine */
	0, 0, 0, 0,																		/* arc routines */
	NOTECHNOLOGY, NONEGATEDARCS|STATICTECHNOLOGY, 0, 0},							/* miscellaneous */

	/* MOSIS CMOS */
	{x_("mocmos"), 0, 400,NONODEPROTO,NOARCPROTO,NOVARIABLE,0,&mocmos_parse,NOCLUSTER,	/* info */
	N_("Complementary MOS (from MOSIS, 2-6 metals [4], 1-2 polys [2], flex rules [submicron])"),	/* description */
	0, mocmos_layers, 0, mocmos_arcprotos, 0, mocmos_nodeprotos, mocmos_variables,	/* tables */
	mocmos_initprocess, 0, mocmos_setmode, mocmos_request,							/* control routines */
	mocmos_nodepolys, mocmos_nodeEpolys, mocmos_shapenodepoly, mocmos_shapeEnodepoly, mocmos_allnodepolys, mocmos_allnodeEpolys, 0,	/* node routines */
	mocmos_shapeportpoly,															/* port routine */
	0, 0, 0, 0,																		/* arc routines */
	NOTECHNOLOGY, NONEGATEDARCS|STATICTECHNOLOGY, 0, 0},							/* miscellaneous */

	/* Old MOSIS CMOS */
	{x_("mocmosold"), 0, 2000,NONODEPROTO,NOARCPROTO,NOVARIABLE,0,&mocmosold_parse,NOCLUSTER,	/* info */
	N_("Complementary MOS (old, from MOSIS, P-Well, double metal)"),				/* description */
	0, mocmosold_layers, 0, mocmosold_arcprotos, 0, mocmosold_nodeprotos, mocmosold_variables,	/* tables */
	mocmosold_initprocess, 0, mocmosold_setmode, 0,									/* control routines */
	0, 0, 0, 0, 0, 0, 0,															/* node routines */
	0,																				/* port routine */
	mocmosold_arcpolys, mocmosold_shapearcpoly, mocmosold_allarcpolys, 0,			/* arc routines */
	NOTECHNOLOGY, NONEGATEDARCS|STATICTECHNOLOGY, 0, 0},							/* miscellaneous */

	/* MOSIS CMOS Submicron */
	{x_("mocmossub"), 0, 400,NONODEPROTO,NOARCPROTO,NOVARIABLE,0,&mocmossub_parse,NOCLUSTER,/* info */
	N_("Complementary MOS (old, from MOSIS, Submicron, 2-6 metals [4], double poly)"),	/* description */
	0, mocmossub_layers, 0, mocmossub_arcprotos, 0, mocmossub_nodeprotos, mocmossub_variables,	/* tables */
	mocmossub_initprocess, 0, mocmossub_setmode, mocmossub_request,					/* control routines */
	0, 0, 0, 0, 0, 0, 0,															/* node routines */
	0,																				/* port routine */
	0, 0, 0, 0,																		/* arc routines */
	NOTECHNOLOGY, NONEGATEDARCS|STATICTECHNOLOGY, 0, 0},							/* miscellaneous */

	/* MOSIS BiCMOS */
	{x_("bicmos"), 0, 2000, NONODEPROTO,NOARCPROTO,NOVARIABLE,0,NOCOMCOMP,NOCLUSTER,	/* info */
	N_("Bipolar/CMOS (from MOSIS, N-Well, SCE Rules)"),								/* description */
	0, bicmos_layers, 0, bicmos_arcprotos, 0, bicmos_nodeprotos, bicmos_variables,	/* tables */
	bicmos_initprocess, 0, 0, 0,													/* control routines */
	0, 0, 0, 0, 0, 0, 0,															/* node routines */
	0,																				/* port routine */
	0, 0, 0, 0,																		/* arc routines */
	NOTECHNOLOGY, NONEGATEDARCS|STATICTECHNOLOGY, 0, 0},							/* miscellaneous */

	/* Round MOSIS CMOS */
	{x_("rcmos"), 0, 2000, NONODEPROTO,NOARCPROTO,NOVARIABLE,0,NOCOMCOMP,NOCLUSTER,		/* info */
	N_("Complementary MOS (round, from MOSIS, P-Well, double metal)"),				/* description */
	0, rcmos_layers, 0, rcmos_arcprotos, 0, rcmos_nodeprotos, rcmos_variables,		/* tables */
	rcmos_initprocess, 0, 0, 0,														/* control routines */
	0, 0, 0, 0, 0, 0, 0,															/* node routines */
	0,																				/* port routine */
	rcmos_arcpolys, rcmos_shapearcpoly, rcmos_allarcpolys, 0,						/* arc routines */
	NOTECHNOLOGY, NONSTANDARD|NONEGATEDARCS|STATICTECHNOLOGY, 0, 0},				/* miscellaneous */

	/* DOD CMOS (from Gary Lambert) */
	{x_("cmosdodn"), 0, 2000, NONODEPROTO,NOARCPROTO,NOVARIABLE,0,NOCOMCOMP,NOCLUSTER,	/* info */
	N_("Complementary MOS (from DOD, P-Well, double metal"),						/* description */
	0, dodcmosn_layers, 0, dodcmosn_arcprotos, 0, dodcmosn_nodeprotos, dodcmosn_variables,	/* tables */
 	dodcmosn_initprocess, 0, 0, 0,													/* control routines */
	0, 0, dodcmosn_shapenodepoly, 0, dodcmosn_allnodepolys, 0, 0,					/* node routines */
	0,																				/* port routine */
	0, 0, 0, 0,																		/* arc routines */
	NOTECHNOLOGY, NONEGATEDARCS|STATICTECHNOLOGY, 0, 0},							/* miscellaneous */

	/* bipolar */
	{x_("bipolar"), 0, 4000, NONODEPROTO,NOARCPROTO,NOVARIABLE,0,NOCOMCOMP,NOCLUSTER,	/* info */
	N_("Bipolar (self-aligned, single poly)"),										/* description */
	0, bipolar_layers, 0, bipolar_arcprotos, 0, bipolar_nodeprotos, bipolar_variables,	/* tables */
	bipolar_initprocess, 0, 0, 0,													/* control routines */
	0, 0, 0, 0, 0, 0, 0,															/* node routines */
	0,																				/* port routine */
	0, 0, 0, 0,																		/* arc routines */
	NOTECHNOLOGY, NONEGATEDARCS|STATICTECHNOLOGY, 0, 0},							/* miscellaneous */

	/* Schematic capture */
	{x_("schematic"), 0, 4000, NONODEPROTO,NOARCPROTO,NOVARIABLE,0,&sch_parse,NOCLUSTER,/* info */
	N_("Schematic Capture"),														/* description */
	0, sch_layers, 0, sch_arcprotos, 0, sch_nodeprotos, sch_variables,				/* tables */
	sch_initprocess, sch_termprocess, sch_setmode, sch_request,						/* control routines */
	sch_nodepolys, sch_nodeEpolys, sch_shapenodepoly, sch_shapeEnodepoly, sch_allnodepolys, sch_allnodeEpolys, sch_nodesizeoffset,	/* node routines */
	sch_shapeportpoly,																/* port routine */
	sch_arcpolys, sch_shapearcpoly, sch_allarcpolys, 0,								/* arc routines */
	NOTECHNOLOGY, NONSTANDARD|STATICTECHNOLOGY, 0, 0},								/* miscellaneous */

	/* FPGA */
	{x_("fpga"), 0, 2000, NONODEPROTO,NOARCPROTO,NOVARIABLE,0,&fpga_parse,NOCLUSTER,	/* info */
	N_("FPGA Building-Blocks"),														/* description */
	0, fpga_layers, 0, fpga_arcprotos, 0, fpga_nodeprotos, fpga_variables,			/* tables */
	fpga_initprocess, fpga_termprocess, fpga_setmode, 0,							/* control routines */
	fpga_nodepolys, fpga_nodeEpolys, fpga_shapenodepoly, fpga_shapeEnodepoly, fpga_allnodepolys, fpga_allnodeEpolys, 0,	/* node routines */
	fpga_shapeportpoly,																/* port routine */
	fpga_arcpolys, fpga_shapearcpoly, fpga_allarcpolys, 0,							/* arc routines */
	NOTECHNOLOGY, NONSTANDARD|STATICTECHNOLOGY|NOPRIMTECHNOLOGY, 0, 0},				/* miscellaneous */

	/* Printed Circuit Board */
	{x_("pcb"), 0, 2540000, NONODEPROTO,NOARCPROTO,NOVARIABLE,0,NOCOMCOMP,NOCLUSTER,	/* info */
	N_("Printed Circuit Board (eight-layer)"),										/* description */
	0, pcb_layers, 0, pcb_arcprotos, 0, pcb_nodeprotos, pcb_variables,				/* tables */
	pcb_initprocess, 0, 0, 0,														/* control routines */
	0, 0, 0, 0, 0, 0, 0,															/* node routines */
	0,																				/* port routine */
	0, 0, 0, 0,																		/* arc routines */
	NOTECHNOLOGY, NONEGATEDARCS|STATICTECHNOLOGY, 0, 0},							/* miscellaneous */

	/* Artwork */
	{x_("artwork"), 0, 4000, NONODEPROTO,NOARCPROTO,NOVARIABLE,0,&art_parse,NOCLUSTER,	/* info */
	N_("General Purpose Sketchpad Facility"),										/* description */
	0, art_layers, 0, art_arcprotos, 0, art_nodeprotos, art_variables,				/* tables */
	art_initprocess, 0, art_setmode, art_request,									/* control routines */
	art_nodepolys, art_nodeEpolys, art_shapenodepoly, art_shapeEnodepoly, art_allnodepolys, art_allnodeEpolys, 0,	/* node routines */
	art_shapeportpoly,																/* port routine */
	art_arcpolys, art_shapearcpoly, art_allarcpolys, 0,								/* arc routines */
	NOTECHNOLOGY, NONSTANDARD|NONELECTRICAL|NONEGATEDARCS|STATICTECHNOLOGY, 0, 0},	/* miscellaneous */

	/* Gem Planning */
	{x_("gem"), 0, 2000, NONODEPROTO,NOARCPROTO,NOVARIABLE,0,NOCOMCOMP,NOCLUSTER,		/* info */
	N_("Temporal Specification Facility (from Lansky)"),							/* description */
	0, gem_layers, 0, gem_arcprotos, 0, gem_nodeprotos, gem_variables,				/* tables */
	gem_initprocess, 0, 0, 0,														/* control routines */
	0, 0, gem_shapenodepoly, 0, gem_allnodepolys, 0, 0,								/* node routines */
	0,																				/* port routine */
	0, gem_shapearcpoly, gem_allarcpolys, 0,										/* arc routines */
	NOTECHNOLOGY, NONSTANDARD|NONELECTRICAL|NODIRECTIONALARCS|NONEGATEDARCS|STATICTECHNOLOGY, 0, 0},/* miscellaneous */

	/* Digital Filter */
	{x_("efido"), 0, 20000, NONODEPROTO,NOARCPROTO,NOVARIABLE,0,NOCOMCOMP,NOCLUSTER,	/* info */
	N_("Digital Filter Facility (from Kroeker)"),									/* description */
	0, efido_layers, 0, efido_arcprotos, 0, efido_nodeprotos, efido_variables,		/* tables */
	efido_initprocess, 0, 0, 0,														/* control routines */
	0, 0, 0, 0, 0, 0, 0,															/* node routines */
	0,																				/* port routine */
	0, 0, 0, 0,																		/* arc routines */
	NOTECHNOLOGY, NONEGATEDARCS|STATICTECHNOLOGY, 0, 0},							/* miscellaneous */

#ifdef FORCESUNTOOLS
	{x_("ziptronics"), 0, 2000, NONODEPROTO,NOARCPROTO,NOVARIABLE,0,NOCOMCOMP,NOCLUSTER,	/* info */
	N_("Ziptronics Metal Layers with Micron SRAM Wafer"),							/* description */
	0, ziptronics_layers, 0, ziptronics_arcprotos, 0, ziptronics_nodeprotos, ziptronics_variables,	/* tables */
	0, 0, 0, 0,																		/* control routines */
	0, 0, 0, 0, 0, 0, 0,															/* node routines */
	0,																				/* port routine */
	0, 0, 0, 0,																		/* arc routines */
	NOTECHNOLOGY, NONEGATEDARCS|STATICTECHNOLOGY, 0, 0},							/* miscellaneous */

	{x_("epic7s"), 0, 40, NONODEPROTO,NOARCPROTO,NOVARIABLE,0,NOCOMCOMP,NOCLUSTER,	/* info */
	N_("Epic7S from Texas Instruments (90nm, 9 metal, 1 poly)"),					/* description */
	0, epic7s_layers, 0, epic7s_arcprotos, 0, epic7s_nodeprotos, epic7s_variables,	/* tables */
	0, 0, 0, 0,																		/* control routines */
	0, 0, 0, 0, 0, 0, 0,															/* node routines */
	0,																				/* port routine */
	0, 0, 0, 0,																		/* arc routines */
	NOTECHNOLOGY, NONEGATEDARCS|STATICTECHNOLOGY, 0, 0},							/* miscellaneous */
#endif

	/* termination */
	{NULL, 0, 0, NULL, NULL, NULL, 0, NULL, NULL,									/* info */
	NULL,																			/* description */
	0, NULL, 0, NULL, 0, NULL, NULL,												/* tables */
	0, 0, 0, 0,																		/* control routines */
	0, 0, 0, 0, 0, 0, 0,															/* node routines */
	0,																				/* port routine */
	0, 0, 0, 0,																		/* arc routines */
	NOTECHNOLOGY, 0, 0, 0}															/* miscellaneous */
};
