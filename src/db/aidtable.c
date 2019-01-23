/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: aidtable.c
 * Tool dispatch table
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

/*
 * sample routines and their arguments:
 *
 * aa_init(INTBIG *argc, CHAR1 *argv[], TOOL *tool) {}
 * BOOLEAN aa_set(INTBIG count, CHAR *par[]) {}
 * INTBIG aa_request(CHAR *command,...) {}
 * aa_examinenodeproto(NODEPROTO *np) {}
 * aa_slice() {}
 * aa_done() {}
 * aa_startbatch(TOOL *source, BOOLEAN undoredo) {}
 * aa_endbatch() {}
 * aa_startobjectchange(INTBIG addr, INTBIG type) {}
 * aa_endobjectchange(INTBIG addr, INTBIG type) {}
 * aa_modifynodeinst(NODEINST *ni, INTBIG olx, INTBIG oly, INTBIG ohx, INTBIG ohy,
 *    INTBIG orot, INTBIG otran) {}
 * aa_modifynodeinsts(INTBIG count, NODEINST **ni, INTBIG *olx, INTBIG *oly, INTBIG *ohx,
 *    INTBIG *ohy, INTBIG *orot, INTBIG *otran) {}
 * aa_modifyarcinst(ARCINST *ai, INTBIG oxA, INTBIG oyA, INTBIG oxB, INTBIG oyB,
 *    INTBIG owid, INTBIG olen) {}
 * aa_modifyportproto(PORTPROTO *pp) {}
 * aa_modifynodeproto(NODEPROTO *np) {}
 * aa_modifydescript(INTBIG addr, INTBIG type, INTBIG key, UINTBIG *old) {}
 * aa_newobject(INTBIG addr, INTBIG type) {}
 * aa_killobject(INTBIG addr, INTBIG type) {}
 * aa_newvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG newtype) {}
 * aa_killvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG oldaddr,
 *    INTBIG oldtype, UINTBIG *olddescript) {}
 * aa_modifyvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG vartype,
 *    INTBIG aindex, INTBIG oldvalue) {}
 * aa_insertvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG vartype, INTBIG aindex) {}
 * aa_deletevariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG vartype, INTBIG aindex,
 *    INTBIG ovalue) {}
 * aa_readlibrary(LIBRARY *lib) {}
 * aa_eraselibrary(LIBRARY *lib) {}
 * aa_writelibrary(LIBRARY *lib, BOOLEAN pass2) {}
 */

/*
 * first come the interactive tools:
 *    us     user interface tool
 *    io     input/output in many forms
 * now come the synthesis tools:
 *    com    compaction
 *    pla    PLA generator
 *    ro     wire router (stitching and river-routing only)
 *    sc     Quisc silicon compiler (Queen's University)
 *    map    FPGA mapper
 *    vhdl   VHDL compiler (Queen's University)
 *    compen compensation
 *    le     logical effort analyzer/sizer
 * now the tools preceeded by the network tool:
 *    net    network maintenance and comparison
 *    dr     incremental design rule checking
 *    erc    electrical rule checking
 *    sim    simulation (esim,rsim,rnl,mossim,spice,als,verilog,silos,etc.)
 *    proj   project management (should be last)
 * the rest is dreaming:
 *    rat    ratio check
 *    pow    power estimation
 *    vec    test vector generation
 *    nov    novice user help
 *    fra    fracturing for fabrication
 *    sea    function search
 *    tv     timing verification
 *    pr     placement and routing for TTL
 *    ga     gate array layout
 *    pad    pad frame support
 *    flr    floor planning
 */

#if COMTOOL
# include "compact.h"
  extern COMCOMP com_compactp;
#endif

#if COMPENTOOL
# include "compensate.h"
  extern COMCOMP compen_compensatep;
#endif

#if DBMIRRORTOOL
# include "dbmirrortool.h"
#endif

#if DRCTOOL
# include "drc.h"
  extern COMCOMP dr_drcp;
#endif

#if ERCTOOL
# include "erc.h"
  extern COMCOMP erc_tablep;
#endif

# include "eio.h"
extern COMCOMP io_iop;

#if LOGEFFTOOL
# include "logeffort.h"
  extern COMCOMP le_tablep;
#endif

#if MAPPERTOOL
# include "mapper.h"
  extern COMCOMP map_mapperp;
#endif

#include "network.h"
extern COMCOMP net_networkp;

#if PLATOOL
# include "pla.h"
  extern COMCOMP pla_plap;
#endif

#if PROJECTTOOL
# include "projecttool.h"
  extern COMCOMP proj_projp;
#endif

#if ROUTTOOL
# include "rout.h"
  extern COMCOMP ro_routerp;
#endif

#if SCTOOL
# include "sc1.h"
  extern COMCOMP sc_silcomp;
#endif

#if SIMTOOL
# include "sim.h"
  extern COMCOMP sim_simulatorp;
#endif

# include "usr.h"
extern COMCOMP us_userp;

#if VHDLTOOL
# include "vhdl.h"
  extern COMCOMP vhdl_comp;
#endif

TOOL el_tools[] =
{
	/********** the interactive tools **********/

	/* user interface */
	{x_("user"), TOOLON | TOOLINCREMENTAL | USEDIALOGS, 0, &us_userp, NOCLUSTER,
	us_init,               us_done,              us_set,				/* init/done/set */
	us_request,            us_examinenodeproto,  us_slice,				/* request/examine/slice */
	us_startbatch,         us_endbatch,									/* startbatch/endbatch */
	us_startobjectchange,  us_endobjectchange,							/* startobject/endobject */
	us_modifynodeinst,     0,                    0,						/* modnode/modnodes/modarc */
	0,                     us_modifynodeproto,   us_modifydescript,		/* modport/modnodeproto/moddescript */
	us_newobject,          us_killobject,								/* newobject/killobject */
	us_newvariable,        us_killvariable,								/* newvariable/killvariable */
	us_modifyvariable,     0,                    0,						/* modvar/insvar/delvar */
	us_readlibrary,        us_eraselibrary,      us_writelibrary,		/* readlib/eraselib/writelib */
	NOVARIABLE, 0},

	/* I/O module */
	{x_("io"), 0, 0, &io_iop, NOCLUSTER,
	io_init,               io_done,              io_set,				/* init/done/set */
	io_request,            0,                    io_slice,				/* request/examine/slice */
	0,                     0,											/* startbatch/endbatch */
	0,                     0,											/* startobject/endobject */
	0,                     0,                    0,						/* modnode/modnodes/modarc */
	0,                     0,                    0,						/* modport/modnodeproto/moddescript */
	0,                     0,											/* newobject/killobject */
	0,                     0,											/* newvariable/killvariable */
	0,                     0,                    0,						/* modvar/insvar/delvar */
	0,                     0,                    0,						/* readlib/eraselib/writelib */
	NOVARIABLE, 0},

#if DBMIRRORTOOL
	/* database mirror module */
	{x_("dbmirror"), TOOLSYNTHESIS,	0,	NOCOMCOMP,	NOCLUSTER,
	dbm_init,              dbm_done,             dbm_set,				/* init/done/set */
	dbm_request,           dbm_examinenodeproto, dbm_slice,				/* request/examine/slice */
	dbm_startbatch,        dbm_endbatch,								/* startbatch/endbatch */
	dbm_startobjectchange, dbm_endobjectchange,							/* startobject/endobject */
	dbm_modifynodeinst,    dbm_modifynodeinsts,  dbm_modifyarcinst,		/* modnode/modnodes/modarc */
	dbm_modifyportproto,   dbm_modifynodeproto,  dbm_modifydescript,	/* modport/modnodeproto/moddescript */
	dbm_newobject,         dbm_killobject,								/* newobject/killobject */
	dbm_newvariable,       dbm_killvariable,							/* newvariable/killvariable */
	dbm_modifyvariable,    dbm_insertvariable,   dbm_deletevariable,	/* modvar/insvar/delvar */
	dbm_readlibrary,       dbm_eraselibrary,     dbm_writelibrary,		/* readlib/eraselib/writelib */
	NOVARIABLE, 0},
#endif

#if COMTOOL
	/* compaction module */
	{x_("compaction"), TOOLSYNTHESIS,              0, &com_compactp, NOCLUSTER,
	com_init,              com_done,             com_set,				/* init/done/set */
	0,                     0,                    com_slice,				/* request/examine/slice */
	0,                     0,											/* startbatch/endbatch */
	0,                     0,											/* startobject/endobject */
	0,                     0,                    0,						/* modnode/modnodes/modarc */
	0,                     0,                    0,						/* modport/modnodeproto/moddescript */
	0,                     0,											/* newobject/killobject */
	0,                     0,											/* newvariable/killvariable */
	0,                     0,                    0,						/* modvar/insvar/delvar */
	0,                     0,                    0,						/* readlib/eraselib/writelib */
	NOVARIABLE, 0},
#endif

#if PLATOOL
	/* PLA generation module */
	{x_("pla"), TOOLSYNTHESIS,                     0, &pla_plap, NOCLUSTER,
	pla_init,              pla_done,             pla_set,				/* init/done/set */
	0,                     0,                    pla_slice,				/* request/examine/slice */
	0,                     0,											/* startbatch/endbatch */
	0,                     0,											/* startobject/endobject */
	0,                     0,                    0,						/* modnode/modnodes/modarc */
	0,                     0,                    0,						/* modport/modnodeproto/moddescript */
	0,                     0,											/* newobject/killobject */
	0,                     0,											/* newvariable/killvariable */
	0,                     0,                    0,						/* modvar/insvar/delvar */
	0,                     0,                    0,						/* readlib/eraselib/writelib */
	NOVARIABLE, 0},
#endif

#if ROUTTOOL
	/* wire routing module */
	{x_("routing"), TOOLINCREMENTAL | TOOLSYNTHESIS, 0, &ro_routerp, NOCLUSTER,
	ro_init,               ro_done,              ro_set,				/* init/done/set */
	0,                     0,                    ro_slice,				/* request/examine/slice */
	ro_startbatch,         ro_endbatch,									/* startbatch/endbatch */
	0,                     0,											/* startobject/endobject */
	ro_modifynodeinst,     0,                    ro_modifyarcinst,		/* modnode/modnodes/modarc */
	ro_modifyportproto,    0,                    0,						/* modport/modnodeproto/moddescript */
	ro_newobject,          ro_killobject,								/* newobject/killobject */
	ro_newvariable,        0,											/* newvariable/killvariable */
	0,                     0,                    0,						/* modvar/insvar/delvar */
	0,                     0,                    0,						/* readlib/eraselib/writelib */
	NOVARIABLE, 0},
#endif

#if SCTOOL
	/* Quisc "Silicon" Compiler module */
	{x_("silicon-compiler"), TOOLSYNTHESIS,	0,	&sc_silcomp,	NOCLUSTER,
	sc_init,               sc_done,              sc_set,				/* init/done/set */
	sc_request,            0,                    sc_slice,				/* request/examine/slice */
	0,                     0,											/* startbatch/endbatch */
	0,                     0,											/* startobject/endobject */
	0,                     0,                    0,						/* modnode/modnodes/modarc */
	0,                     0,                    0,						/* modport/modnodeproto/moddescript */
	0,                     0,											/* newobject/killobject */
	0,                     0,											/* newvariable/killvariable */
	0,                     0,                    0,						/* modvar/insvar/delvar */
	0,                     0,                    0,						/* readlib/eraselib/writelib */
	NOVARIABLE, 0},
#endif

#if MAPPERTOOL
	/* mapper module */
	{x_("mapper"), TOOLSYNTHESIS,	0,	&map_mapperp,	NOCLUSTER,
	map_init,              map_done,             map_set,				/* init/done/set */
	map_request,           map_examinenodeproto, map_slice,				/* request/examine/slice */
	map_startbatch,        map_endbatch,								/* startbatch/endbatch */
	map_startobjectchange, map_endobjectchange,							/* startobject/endobject */
	map_modifynodeinst,    0,                    map_modifyarcinst,		/* modnode/modnodes/modarc */
	map_modifyportproto,   map_modifynodeproto,  map_modifydescript,	/* modport/modnodeproto/moddescript */
	map_newobject,         map_killobject,								/* newobject/killobject */
	map_newvariable,       map_killvariable,							/* newvariable/killvariable */
	map_modifyvariable,    map_insertvariable,   map_deletevariable,	/* modvar/insvar/delvar */
	map_readlibrary,       map_eraselibrary,     map_writelibrary,		/* readlib/eraselib/writelib */
	NOVARIABLE, 0},
#endif

#if VHDLTOOL
	/* VHDL compiler module */
	{x_("vhdl-compiler"), TOOLSYNTHESIS,	0,	&vhdl_comp,	NOCLUSTER,
	vhdl_init,             vhdl_done,            vhdl_set,				/* init/done/set */
	vhdl_request,          0,                    vhdl_slice,			/* request/examine/slice */
	0,                     0,											/* startbatch/endbatch */
	0,                     0,											/* startobject/endobject */
	0,                     0,                    0,						/* modnode/modnodes/modarc */
	0,                     0,                    0,						/* modport/modnodeproto/moddescript */
	0,                     0,											/* newobject/killobject */
	0,                     0,											/* newvariable/killvariable */
	0,                     0,                    0,						/* modvar/insvar/delvar */
	0,                     0,                    0,						/* readlib/eraselib/writelib */
	NOVARIABLE, 0},
#endif

#if COMPENTOOL
	/* compensation module */
	{x_("compensation"), TOOLSYNTHESIS,	0,	&compen_compensatep,	NOCLUSTER,
	compen_init,           compen_done,          compen_set,			/* init/done/set */
	0,                     0,                    compen_slice,			/* request/examine/slice */
	0,                     0,											/* startbatch/endbatch */
	0,                     0,											/* startobject/endobject */
	compen_modifynodeinst, 0,                    0,						/* modnode/modnodes/modarc */
	0,                     0,                    0,						/* modport/modnodeproto/moddescript */
	0,                     compen_killobject,							/* newobject/killobject */
	0,                     0,											/* newvariable/killvariable */
	0,                     0,                    0,						/* modvar/insvar/delvar */
	0,                     0,                    0,						/* readlib/eraselib/writelib */
	NOVARIABLE, 0},
#endif

#if LOGEFFTOOL
	/* logical effort module */
	{x_("logeffort"), TOOLSYNTHESIS,	0,	&le_tablep,	NOCLUSTER,
	le_init,               le_done,              le_set,				/* init/done/set */
	0,                     0,                    0,						/* request/examine/slice */
	0,                     0,											/* startbatch/endbatch */
	0,                     0,											/* startobject/endobject */
	0,                     0,                    0,						/* modnode/modnodes/modarc */
	0,                     0,                    0,						/* modport/modnodeproto/moddescript */
	0,                     0,											/* newobject/killobject */
	0,                     0,											/* newvariable/killvariable */
	0,                     0,                    0,						/* modvar/insvar/delvar */
	0,                     0,                    0,						/* readlib/eraselib/writelib */
	NOVARIABLE, 0},
#endif

	/* network maintenance and comparison */
	{x_("network"), TOOLON | TOOLINCREMENTAL, 0, &net_networkp, NOCLUSTER,
	net_init,              net_done,             net_set,				/* init/done/set */
	net_request,           net_examinenodeproto, net_slice,				/* request/examine/slice */
	net_startbatch,        net_endbatch,								/* startbatch/endbatch */
	0,                     0,											/* startobject/endobject */
	0,                     0,                    0,						/* modnode/modnodes/modarc */
	net_modifyportproto,   0,                    0,						/* modport/modnodeproto/moddescript */
	net_newobject,         net_killobject,								/* newobject/killobject */
	net_newvariable,       net_killvariable,							/* newvariable/killvariable */
	0,                     0,                    0,						/* modvar/insvar/delvar */
	net_readlibrary,       net_eraselibrary,     0,						/* readlib/eraselib/writelib */
	NOVARIABLE, 0},

#if DRCTOOL
	/* design-rule checker */
	{x_("drc"), TOOLON | TOOLINCREMENTAL | TOOLANALYSIS, 0, &dr_drcp, NOCLUSTER,
	dr_init,               dr_done,              dr_set,				/* init/done/set */
	dr_request,            dr_examinenodeproto,  dr_slice,				/* request/examine/slice */
	0,                     0,											/* startbatch/endbatch */
	0,                     0,											/* startobject/endobject */
	dr_modifynodeinst,     0,                    dr_modifyarcinst,		/* modnode/modnodes/modarc */
	0,                     0,                    0,						/* modport/modnodeproto/moddescript */
	dr_newobject,          dr_killobject,								/* newobject/killobject */
	dr_newvariable,        0,											/* newvariable/killvariable */
	0,                     0,                    0,						/* modvar/insvar/delvar */
	0,                     dr_eraselibrary,      0,						/* readlib/eraselib/writelib */
	NOVARIABLE,0},
#endif

#if ERCTOOL
	/* electrical-rule checker */
	{x_("erc"), TOOLANALYSIS, 0, &erc_tablep, NOCLUSTER,
	erc_init,              erc_done,             erc_set,				/* init/done/set */
	0,                     0,                    0,						/* request/examine/slice */
	0,                     0,											/* startbatch/endbatch */
	0,                     0,											/* startobject/endobject */
	0,                     0,                    0,						/* modnode/modnodes/modarc */
	0,                     0,                    0,						/* modport/modnodeproto/moddescript */
	0,                     0,											/* newobject/killobject */
	0,                     0,											/* newvariable/killvariable */
	0,                     0,                    0,						/* modvar/insvar/delvar */
	0,                     0,                    0,						/* readlib/eraselib/writelib */
	NOVARIABLE,0},
#endif

#if SIMTOOL
	/* simulation module */
	{x_("simulation"), TOOLON|TOOLANALYSIS, 0, &sim_simulatorp, NOCLUSTER,
	sim_init,              sim_done,             sim_set,				/* init/done/set */
	sim_request,           0,                    sim_slice,				/* request/examine/slice */
	sim_startbatch,        0,											/* startbatch/endbatch */
	0,                     0,											/* startobject/endobject */
	sim_modifynodeinst,    0,                    sim_modifyarcinst,		/* modnode/modnodes/modarc */
	sim_modifyportproto,   0,                    0,						/* modport/modnodeproto/moddescript */
	sim_newobject,         sim_killobject,								/* newobject/killobject */
	sim_newvariable,       sim_killvariable,							/* newvariable/killvariable */
	sim_modifyvariable,    sim_insertvariable,   sim_deletevariable,	/* modvar/insvar/delvar */
	sim_readlibrary,       0,                    0,						/* readlib/eraselib/writelib */
	NOVARIABLE, 0},
#endif

#if PROJECTTOOL
	/* project management module */
	{x_("project"), TOOLON | TOOLINCREMENTAL, 0, &proj_projp, NOCLUSTER,
	proj_init,             proj_done,            proj_set,				/* init/done/set */
	0,                     0,                    proj_slice,			/* request/examine/slice */
	proj_startbatch,       0,											/* startbatch/endbatch */
	0,                     0,											/* startobject/endobject */
	proj_modifynodeinst,   0,                    proj_modifyarcinst,	/* modnode/modnodes/modarc */
	proj_modifyportproto,  0,                    proj_modifydescript,	/* modport/modnodeproto/moddescript */
	proj_newobject,        proj_killobject,								/* newobject/killobject */
	proj_newvariable,      proj_killvariable,							/* newvariable/killvariable */
	proj_modifyvariable,   proj_insertvariable,  proj_deletevariable,	/* modvar/insvar/delvar */
	proj_readlibrary,      0,                    0,						/* readlib/eraselib/writelib */
	NOVARIABLE, 0},
#endif

	/* termination */
	{NULL, 0, 0, NULL, NULL,
	 NULL, NULL, NULL,													/* init/done/set */
	 NULL, NULL, NULL,													/* request/examine/slice */
	 NULL, NULL,														/* startbatch/endbatch */
	 NULL, NULL,														/* startobject/endobject */
	 NULL, NULL, NULL,													/* modnode/modnodes/modarc */
	 NULL, NULL, NULL,													/* modport/modnodeproto/moddescript */
	 NULL, NULL,														/* newobject/killobject */
	 NULL, NULL,														/* newvariable/killvariable */
	 NULL, NULL, NULL,													/* modvar/insvar/delvar */
	 NULL, NULL, NULL,													/* readlib/eraselib/writelib */
	 NULL, 0}
};
