/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: vhdl.c
 * This is the main file for the VHDL front-end compiler for generating
 * input files for the ALS Design System and QUISC (Silicon Compiler)
 * Written by: Andrew R. Kostiuk, Queen's University
 * Modified by: Steven M. Rubin, Static Free Software
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
#if VHDLTOOL

#include "global.h"
#include "efunction.h"
#include "vhdl.h"
#include "sim.h"
#include "eio.h"
#include "network.h"
#include "tecschem.h"
#include "edialogs.h"

#define MAXINPUTS              30		/* maximum inputs to logic gate */
#define IGNORE4PORTTRANSISTORS  1

/* special codes during VHDL generation */
#define BLOCKNORMAL   0		/* ordinary block */
#define BLOCKMOSTRAN  1		/* a MOS transistor */
#define BLOCKBUFFER   2		/* a buffer */
#define BLOCKPOSLOGIC 3		/* an and, or, xor */
#define BLOCKINVERTER 4		/* an inverter */
#define BLOCKNAND     5		/* a nand */
#define BLOCKNOR      6		/* a nor */
#define BLOCKXNOR     7		/* an xnor */
#define BLOCKFLOPDS   8		/* a settable D flip-flop */
#define BLOCKFLOPDR   9		/* a resettable D flip-flop */
#define BLOCKFLOPTS  10		/* a settable T flip-flop */
#define BLOCKFLOPTR  11		/* a resettable T flip-flop */
#define BLOCKFLOP    12		/* a general flip-flop */

/* the VHDL compiler tool table */
static KEYWORD vhdloutputopt[] =
{
	{x_("netlisp"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("als"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("quisc"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("silos"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("rsim"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP vhdloutputp = {vhdloutputopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("VHDL compiler output format"), 0};
static COMCOMP vhdllibraryp = {NOKEYWORD, topoflibs, nextlibs, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Behavioral library to use in VHDL compilation"), 0};
static KEYWORD vhdlnopt[] =
{
	{x_("vhdl-on-disk"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("netlist-on-disk"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("external"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("warn"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP vhdlnp = {vhdlnopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Negating action"), 0};
static KEYWORD vhdlsetopt[] =
{
	{x_("output-format"),      1,{&vhdloutputp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("library"),            1,{&vhdllibraryp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("external"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("warn"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("vhdl-on-disk"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("netlist-on-disk"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("not"),                1,{&vhdlnp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("make-vhdl"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("compile-now"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP vhdl_comp = {vhdlsetopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("VHDL compiler action"), 0};

CHAR	vhdl_delimiterstr[] = {x_("&'()*+,-./:;<=>|")};
CHAR	vhdl_doubledelimiterstr[] = {x_("=>..**:=/=>=<=<>")};

static VKEYWORD vhdl_keywords[] =
{
	{x_("abs"),				KEY_ABS},
	{x_("after"),			KEY_AFTER},
	{x_("alias"),			KEY_ALIAS},
#ifndef VHDL50
	{x_("all"),				KEY_ALL},
#endif
	{x_("and"),				KEY_AND},
#ifdef VHDL50
	{x_("architectural"),	KEY_ARCHITECTURAL},
#else
	{x_("architecture"),	KEY_ARCHITECTURE},
#endif
	{x_("array"),			KEY_ARRAY},
	{x_("assertion"),		KEY_ASSERTION},
	{x_("attribute"),		KEY_ATTRIBUTE},
	{x_("begin"),			KEY_BEGIN},
	{x_("behavioral"),		KEY_BEHAVIORAL},
	{x_("body"),			KEY_BODY},
	{x_("case"),			KEY_CASE},
	{x_("component"),		KEY_COMPONENT},
	{x_("connect"),			KEY_CONNECT},
	{x_("constant"),		KEY_CONSTANT},
	{x_("convert"),			KEY_CONVERT},
	{x_("dot"),				KEY_DOT},
	{x_("downto"),			KEY_DOWNTO},
	{x_("else"),			KEY_ELSE},
	{x_("elsif"),			KEY_ELSIF},
	{x_("end"),				KEY_END},
	{x_("entity"),			KEY_ENTITY},
	{x_("exit"),			KEY_EXIT},
	{x_("for"),				KEY_FOR},
	{x_("function"),		KEY_FUNCTION},
	{x_("generate"),		KEY_GENERATE},
	{x_("generic"),			KEY_GENERIC},
	{x_("if"),				KEY_IF},
	{x_("in"),				KEY_IN},
	{x_("inout"),			KEY_INOUT},
	{x_("is"),				KEY_IS},
	{x_("library"),			KEY_LIBRARY},
	{x_("linkage"),			KEY_LINKAGE},
	{x_("loop"),			KEY_LOOP},
#ifndef VHDL50
	{x_("map"),				KEY_MAP},
#endif
	{x_("mod"),				KEY_MOD},
	{x_("nand"),			KEY_NAND},
	{x_("next"),			KEY_NEXT},
	{x_("nor"),				KEY_NOR},
	{x_("not"),				KEY_NOT},
	{x_("null"),			KEY_NULL},
	{x_("of"),				KEY_OF},
#ifndef VHDL50
	{x_("open"),			KEY_OPEN},
#endif
	{x_("or"),				KEY_OR},
	{x_("others"),			KEY_OTHERS},
	{x_("out"),				KEY_OUT},
	{x_("package"),			KEY_PACKAGE},
	{x_("port"),			KEY_PORT},
	{x_("range"),			KEY_RANGE},
	{x_("record"),			KEY_RECORD},
	{x_("rem"),				KEY_REM},
	{x_("report"),			KEY_REPORT},
	{x_("resolve"),			KEY_RESOLVE},
	{x_("return"),			KEY_RETURN},
	{x_("severity"),		KEY_SEVERITY},
	{x_("signal"),			KEY_SIGNAL},
	{x_("standard"),		KEY_STANDARD},
	{x_("static"),			KEY_STATIC},
	{x_("subtype"),			KEY_SUBTYPE},
	{x_("then"),			KEY_THEN},
	{x_("to"),				KEY_TO},
	{x_("type"),			KEY_TYPE},
	{x_("units"),			KEY_UNITS},
	{x_("use"),				KEY_USE},
	{x_("variable"),		KEY_VARIABLE},
	{x_("when"),			KEY_WHEN},
	{x_("while"),			KEY_WHILE},
	{x_("with"),			KEY_WITH},
	{x_("xor"),				KEY_XOR}
};

static IDENTTABLE *vhdl_identtable = 0;
static TOKENLIST  *vhdl_tliststart;
static TOKENLIST  *vhdl_tlistend;
       INTBIG      vhdl_externentities;		/* enternal entities flag */
       INTBIG      vhdl_warnflag;			/* warning flag, TRUE warn */
static INTBIG      vhdl_vhdlondiskkey;		/* variable key for "VHDL_vhdl_on_disk" */
static INTBIG      vhdl_netlistondiskkey;	/* variable key for "VHDL_netlist_on_disk" */
       INTBIG      vhdl_target;				/* Current simulator */
static LIBRARY    *vhdl_lib;				/* behavioral library */
static FILE       *vhdl_fp;					/* current file */
static VARIABLE   *vhdl_var;				/* current variable */
static INTBIG      vhdl_linecount;			/* current line number */
static NODEPROTO  *vhdl_cell;				/* current output cell */
extern INTBIG      vhdl_errorcount;
extern SYMBOLLIST *vhdl_gsymbols;
       TOOL       *vhdl_tool;
static void       *vhdl_componentarray = 0;
static void       *vhdl_stringarray;
       UNRESLIST  *vhdl_unresolved_list = 0;

/* prototypes for local routines */
static void      vhdl_addportlist(void*, NODEINST*, NODEPROTO*, INTBIG);
static void      vhdl_addrealports(void *infstr, NODEINST *ni, INTBIG special);
static void      vhdl_addstring(void*, CHAR*, NODEPROTO*);
static void      vhdl_addtheseports(void*, NODEINST*, NODEPROTO*, UINTBIG, CHAR*);
static BOOLEAN   vhdl_compile(NODEPROTO*);
static void      vhdl_convertcell(NODEPROTO*, BOOLEAN);
static INTBIG    vhdl_endinput(void);
static INTBIG    vhdl_endoutput(void);
static void      vhdl_freescannermemory(void);
static BOOLEAN   vhdl_generatevhdl(NODEPROTO *np);
static time_t    vhdl_getcelldate(NODEPROTO *np);
static INTBIG    vhdl_getnextline(CHAR*);
static INTBIG    vhdl_identfirsthash(CHAR*);
static INTBIG    vhdl_identsecondhash(CHAR*, INTBIG);
static VKEYWORD *vhdl_iskeyword(CHAR*, INTBIG);
static void      vhdl_makechartoken(CHAR, INTBIG, INTBIG);
static void      vhdl_makedecimaltoken(CHAR*, INTBIG, INTBIG, INTBIG);
static void      vhdl_makeidenttoken(CHAR*, INTBIG, INTBIG, INTBIG);
static void      vhdl_makekeytoken(VKEYWORD*, INTBIG, INTBIG);
static BOOLEAN   vhdl_morerecentcontents(NODEPROTO*, UINTBIG);
static void      vhdl_makestrtoken(CHAR*, INTBIG, INTBIG, INTBIG);
static void      vhdl_maketoken(INTBIG, INTBIG, INTBIG);
static void      vhdl_optionsdlog(void);
static CHAR     *vhdl_primname(NODEINST*, INTBIG*);
static void      vhdl_scanner(void);
static INTBIG    vhdl_startinput(NODEPROTO*, INTBIG, BOOLEAN, CHAR**, UINTBIG*);
static INTBIG    vhdl_startoutput(NODEPROTO*, INTBIG, BOOLEAN, CHAR**);
static CHAR      vhdl_toupper(CHAR);

void vhdl_init(INTBIG *argc, CHAR1 *argv[], TOOL *thistool)
{
	Q_UNUSED( argc );
	Q_UNUSED( argv );

	/* only initialize during pass 1 */
	if (thistool == NOTOOL || thistool == 0) return;

	vhdl_tool = thistool;
	vhdl_target = TARGET_QUISC;
	vhdl_externentities = TRUE;
	vhdl_warnflag = FALSE;
	vhdl_lib = NOLIBRARY;
	vhdl_vhdlondiskkey = makekey(x_("VHDL_vhdl_on_disk"));
	vhdl_netlistondiskkey = makekey(x_("VHDL_netlist_on_disk"));
	DiaDeclareHook(x_("vhdlopt"), &vhdl_comp, vhdl_optionsdlog);
}

void vhdl_done(void)
{
#ifdef DEBUGMEMORY
	if (vhdl_componentarray != 0) killstringarray(vhdl_componentarray);
	vhdl_freescannermemory();
	if (vhdl_identtable != 0) efree((CHAR *)vhdl_identtable);
	vhdl_freeparsermemory();
	vhdl_freesemantic();
	vhdl_freeunresolvedlist(&vhdl_unresolved_list);
#endif
}

void vhdl_set(INTBIG count, CHAR *par[])
{
	INTBIG l;
	CHAR *pp;
	REGISTER NODEPROTO *np;

	if (count == 0) return;
	l = estrlen(pp = par[0]);

	if (namesamen(pp, x_("make-vhdl"), l) == 0)
	{
		if (count >= 2)
		{
			np = getnodeproto(par[1]);
			if (np == NONODEPROTO)
			{
				ttyputerr(_("No cell named %s"), par[1]);
				return;
			}
			if (np->primindex != 0)
			{
				ttyputerr(M_("Can only convert cells to VHDL, not primitives"));
				return;
			}
		} else
		{
			np = getcurcell();
			if (np == NONODEPROTO)
			{
				ttyputerr(_("No current cell"));
				return;
			}
		}
		begintraversehierarchy();
		vhdl_convertcell(np, TRUE);
		endtraversehierarchy();

		return;
	}

	if (namesamen(pp, x_("compile-now"), l) == 0)
	{
		np = getcurcell();
		if (np == NONODEPROTO)
		{
			ttyputerr(_("Must be editing a cell"));
			return;
		}

		(void)vhdl_compile(np);
		return;
	}

	if (namesamen(pp, x_("library"), l) == 0)
	{
		if (count <= 1)
		{
			ttyputusage(x_("telltool vhdl-compiler library LIBRARY"));
			return;
		}
		vhdl_lib = getlibrary(par[1]);
		if (vhdl_lib == NOLIBRARY)
		{
			ttyputerr(_("Cannot find library %s"), par[1]);
			return;
		}
		ttyputverbose(M_("Library %s will be used for behavioral descriptions"), par[1]);
		return;
	}

	if (namesamen(pp, x_("vhdl-on-disk"), l) == 0)
	{
		setvalkey((INTBIG)vhdl_tool, VTOOL, vhdl_vhdlondiskkey, 1, VINTEGER);
		ttyputverbose(M_("VHDL will be kept in separate disk files"));
		return;
	}

	if (namesamen(pp, x_("netlist-on-disk"), l) == 0)
	{
		setvalkey((INTBIG)vhdl_tool, VTOOL, vhdl_netlistondiskkey, 1, VINTEGER);
		ttyputverbose(M_("Netlists will be kept in separate disk files"));
		return;
	}

	if (namesamen(pp, x_("warn"), l) == 0)
	{
		vhdl_warnflag = TRUE;
		ttyputverbose(M_("VHDL compiler will display warnings"));
		return;
	}

	if (namesamen(pp, x_("external"), l) == 0)
	{
		vhdl_externentities = TRUE;
		ttyputverbose(M_("VHDL compiler will allow external references"));
		return;
	}

	if (namesamen(pp, x_("not"), l) == 0)
	{
		if (count <= 1)
		{
			ttyputusage(x_("telltool vhdl-compiler not OPTION"));
			return;
		}
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("warn"), l) == 0)
		{
			vhdl_warnflag = FALSE;
			ttyputverbose(M_("VHDL compiler will not display warnings"));
			return;
		}
		if (namesamen(pp, x_("external"), l) == 0)
		{
			vhdl_externentities = FALSE;
			ttyputverbose(M_("VHDL compiler will not allow external references"));
			return;
		}
		if (namesamen(pp, x_("vhdl-on-disk"), l) == 0)
		{
			setvalkey((INTBIG)vhdl_tool, VTOOL, vhdl_vhdlondiskkey, 0, VINTEGER);
			ttyputverbose(M_("VHDL will be kept in cells"));
			return;
		}
		if (namesamen(pp, x_("netlist-on-disk"), l) == 0)
		{
			setvalkey((INTBIG)vhdl_tool, VTOOL, vhdl_netlistondiskkey, 0, VINTEGER);
			ttyputverbose(M_("Netlists will be kept in cells"));
			return;
		}
		ttyputbadusage(x_("telltool vhdl-compiler not"));
		return;
	}

	if (namesamen(pp, x_("output-format"), l) == 0)
	{
		if (count <= 1)
		{
			ttyputusage(x_("telltool vhdl-compiler output-format FORMAT"));
			return;
		}
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("netlisp"), l) == 0)
		{
			vhdl_target = TARGET_NETLISP;
			ttyputverbose(M_("VHDL compiles to net lisp"));
			return;
		}
		if (namesamen(pp, x_("als"), l) == 0 && l >= 1)
		{
			vhdl_target = TARGET_ALS;
			ttyputverbose(M_("VHDL compiles to ALS simulation"));
			return;
		}
		if (namesamen(pp, x_("quisc"), l) == 0 && l >= 3)
		{
			vhdl_target = TARGET_QUISC;
			ttyputverbose(M_("VHDL compiles to QUISC place-and-route"));
			return;
		}
		if (namesamen(pp, x_("silos"), l) == 0)
		{
			vhdl_target = TARGET_SILOS;
			ttyputverbose(M_("VHDL compiles to SILOS simulation"));
			return;
		}
		if (namesamen(pp, x_("rsim"), l) == 0)
		{
			vhdl_target = TARGET_RSIM;
			ttyputverbose(M_("VHDL compiles to RSIM simulation"));
			return;
		}
		ttyputbadusage(x_("telltool vhdl-compiler output-format"));
		return;
	}

	ttyputbadusage(x_("telltool vhdl-compiler"));
}

void vhdl_slice(void)
{
	REGISTER NODEPROTO *np;

	np = getcurcell();
	if (np == NONODEPROTO)
	{
		ttyputerr(_("Must be editing a cell"));
		toolturnoff(vhdl_tool, FALSE);
		return;
	}

	(void)vhdl_compile(np);

	toolturnoff(vhdl_tool, FALSE);
	ttyputmsg(_("VHDL compiler turned off"));
}

/*
 * make requests of the VHDL tool:
 *  "begin-vhdl-input" TAKES: cell, string for source  RETURNS: nonzero on error
 *  "begin-netlist-input" TAKES: cell, filetype, string for source
 *                        RETURNS: nonzero on error
 *  "want-netlist-input" TAKES: cell, filetype
 *                       RETURNS: nonzero on error
 *  "get-line" TAKES: buffer address  RETURNS: line number, zero on EOF
 *  "end-input"
 *
 *  "begin-vhdl-output" TAKES: cell, string for source  RETURNS: output name (zero on error)
 *  "begin-netlist-output" TAKES: cell  RETURNS: output name (zero on error)
 *  "put-line" TAKES: buffer address
 *  "end-output"
 */
INTBIG vhdl_request(CHAR *command, va_list ap)
{
	INTBIG i, len;
	BOOLEAN vhdlondisk, netlistondisk;
	UINTBIG netlistdate, vhdldate;
	CHAR *intended, *desired, buffer[MAXVHDLLINE];
	REGISTER VARIABLE *var;
	REGISTER INTBIG arg1, arg2, arg3;
	REGISTER NODEPROTO *np, *orignp;
	REGISTER VIEW *view;

	var = getvalkey((INTBIG)vhdl_tool, VTOOL, VINTEGER, vhdl_vhdlondiskkey);
	vhdlondisk = FALSE;
	if (var != NOVARIABLE && var->addr != 0) vhdlondisk = TRUE;
	netlistondisk = FALSE;
	var = getvalkey((INTBIG)vhdl_tool, VTOOL, VINTEGER, vhdl_netlistondiskkey);
	if (var != NOVARIABLE && var->addr != 0) netlistondisk = TRUE;

	if (namesame(command, x_("begin-vhdl-input")) == 0)
	{
		/* get the arguments (1=cell, 2=source) */
		arg1 = va_arg(ap, INTBIG);
		arg2 = va_arg(ap, INTBIG);

		return(vhdl_startinput((NODEPROTO *)arg1, io_filetypevhdl, vhdlondisk,
			(CHAR **)arg2, &vhdldate));
	}

	if (namesame(command, x_("begin-netlist-input")) == 0)
	{
		/* get the arguments (1=cell, 2=filetype, 3=source) */
		arg1 = va_arg(ap, INTBIG);
		arg2 = va_arg(ap, INTBIG);
		arg3 = va_arg(ap, INTBIG);

		return(vhdl_startinput((NODEPROTO *)arg1, arg2, netlistondisk,
			(CHAR **)arg3, &netlistdate));
	}

	if (namesame(command, x_("get-line")) == 0)
	{
		/* get the arguments (1=buffer) */
		arg1 = va_arg(ap, INTBIG);

		return(vhdl_getnextline((CHAR *)arg1));
	}

	if (namesame(command, x_("end-input")) == 0)
	{
		return(vhdl_endinput());
	}

	if (namesame(command, x_("want-netlist-input")) == 0)
	{
		/* get the arguments (1=cell, 2=filetype) */
		arg1 = va_arg(ap, INTBIG);
		arg2 = va_arg(ap, INTBIG);

		if (arg2 == sim_filetypequisc)
		{
			view = el_netlistquiscview;   vhdl_target = TARGET_QUISC;
		} else if (arg2 == sim_filetypeals)
		{
			view = el_netlistalsview;     vhdl_target = TARGET_ALS;
		} else if (arg2 == sim_filetypenetlisp)
		{
			view = el_netlistnetlispview; vhdl_target = TARGET_NETLISP;
		} else if (arg2 == sim_filetypesilos)
		{
			view = el_netlistsilosview;   vhdl_target = TARGET_SILOS;
		} else if (arg2 == sim_filetypersim)
		{
			view = el_netlistrsimview;    vhdl_target = TARGET_RSIM;
		} else view = NOVIEW;

		/* must have a valid netlist */
		orignp = (NODEPROTO *)arg1;
		i = vhdl_startinput(orignp, arg2, netlistondisk, &intended, &netlistdate);
		vhdl_endinput();
		if (i == 0)
		{
			/* got netlist: make sure it is more recent than any VHDL or layout */
			if (orignp->cellview != view)
			{
				/* determine date of associated VHDL */
				if (vhdl_startinput(orignp, io_filetypevhdl, vhdlondisk, &intended,
					&vhdldate) == 0)
				{
					if (netlistdate < vhdldate) i = 1;

					/* if original is not VHDL, ensure that VHDL comes from original */
					if (orignp->cellview != el_vhdlview)
					{
						if (vhdl_getnextline(buffer) != 0)
						{
							desired = x_("-- VHDL automatically generated from cell ");
							len = estrlen(desired);
							if (estrncmp(buffer, desired, len) == 0)
							{
								if (estrcmp(&buffer[len], describenodeproto(orignp)) != 0) i = 1;
							}
						}
					}
				}
				vhdl_endinput();

				/* if original is not VHDL, check all cells for recency */
				if (orignp->cellview != el_vhdlview && i == 0)
				{
					/* check all in cell for recency */
					FOR_CELLGROUP(np, orignp)
						if (netlistdate < np->revisiondate)
					{
						i = 1;
						break;
					}
					if (i == 0)
					{
						if (vhdl_morerecentcontents(orignp, netlistdate)) i = 1;
					}
				}
			}
		}

		if (i != 0)
		{
			/* no valid netlist: look for VHDL */
			i = vhdl_startinput(orignp, io_filetypevhdl, vhdlondisk, &intended, &vhdldate);

			/* if original is not VHDL, ensure that VHDL comes from original */
			if (i == 0 && orignp->cellview != el_vhdlview)
			{
				if (vhdl_getnextline(buffer) != 0)
				{
					desired = x_("-- VHDL automatically generated from cell ");
					len = estrlen(desired);
					if (estrncmp(buffer, desired, len) == 0)
					{
						if (estrcmp(&buffer[len], describenodeproto(orignp)) != 0) i = 1;
					}
				}
			}
			vhdl_endinput();
			if (i == 0)
			{
				/* got VHDL: make sure it is more recent than any layout */
				if (orignp->cellview != el_vhdlview)
				{
					/* check all in cell for recency */
					FOR_CELLGROUP(np, orignp)
						if (vhdldate < np->revisiondate)
					{
						i = 1;
						break;
					}
					if (i == 0)
					{
						if (vhdl_morerecentcontents(orignp, vhdldate)) i = 1;
					}
				}
			}
			if (i != 0)
			{
				/* no valid VHDL: convert cell */
				np = (NODEPROTO *)arg1;
				begintraversehierarchy();
				vhdl_convertcell(np, FALSE);
				endtraversehierarchy();
			}

			/* compile VHDL to netlist */
			if (vhdl_compile(getcurcell())) return(1);
		}
		return(0);
	}

	if (namesame(command, x_("begin-vhdl-output")) == 0)
	{
		/* get the arguments (1=cell, 2=source) */
		arg1 = va_arg(ap, INTBIG);
		arg2 = va_arg(ap, INTBIG);

		return(vhdl_startoutput((NODEPROTO *)arg1, io_filetypevhdl, vhdlondisk,
			(CHAR **)arg2));
	}

	if (namesame(command, x_("put-line")) == 0)
	{
		/* get the arguments */
		arg1 = va_arg(ap, INTBIG);
		arg2 = va_arg(ap, INTBIG);
		arg3 = va_arg(ap, INTBIG);

		vhdl_print((CHAR *)arg1, (CHAR *)arg2, (CHAR *)arg3);
		return(0);
	}

	if (namesame(command, x_("end-output")) == 0)
	{
		return(vhdl_endoutput());
	}
	return(1);
}

/*
 * routine returns false if all cells in the hierarchy below cell "np" are less recent
 * than "dateoffile".  Returns true if any more recent cell is found.
 */
BOOLEAN vhdl_morerecentcontents(NODEPROTO *np, UINTBIG dateoffile)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *subnp;

	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		subnp = ni->proto;
		if (subnp->primindex != 0) continue;

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(subnp, np)) continue;
		if (subnp->cellview == el_iconview)
		{
			subnp = contentsview(subnp);
			if (subnp == NONODEPROTO) continue;
		}
		if (dateoffile < subnp->revisiondate) return(TRUE);
		if (vhdl_morerecentcontents(subnp, dateoffile)) return(TRUE);
	}
	return(FALSE);
}

/****************************** GENERALIZED VHDL/NET I/O ******************************/

/*
 * Routine to begin input of a file or cell of type "filetype".  If "diskflag" is true,
 * use a disk file whose name is the cell name of cell "np".  If "diskflag" is false, use
 * the cell with the appropriate view of "np".  Returns the name of the input source in
 * "intended" and its last modification date in "revisiondate".  Returns nonzero on error.
 */
INTBIG vhdl_startinput(NODEPROTO *np, INTBIG filetype, BOOLEAN diskflag, CHAR **intended,
	UINTBIG *revisiondate)
{
	REGISTER NODEPROTO *onp;
	CHAR *filename;
	REGISTER VIEW *view;
	REGISTER void *infstr;

	if (np == NONODEPROTO || np->primindex != 0)
	{
		*intended = x_("NO CURRENT CELL");
		return(1);
	}

	if (filetype == io_filetypevhdl)
	{
		view = el_vhdlview;
	} else if (filetype == sim_filetypequisc)
	{
		view = el_netlistquiscview;
	} else if (filetype == sim_filetypeals)
	{
		view = el_netlistalsview;
	} else if (filetype == sim_filetypenetlisp)
	{
		view = el_netlistnetlispview;
	} else if (filetype == sim_filetypesilos)
	{
		view = el_netlistsilosview;
	} else if (filetype == sim_filetypersim)
	{
		view = el_netlistrsimview;
	} else view = NOVIEW;

	vhdl_fp = 0;
	vhdl_var = NOVARIABLE;
	if (diskflag)
	{
		/* attempt to open file */
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("file "));
		addstringtoinfstr(infstr, np->protoname);
		*intended = returninfstr(infstr);
		if ((vhdl_fp = xopen(np->protoname, filetype, x_(""), &filename)) == 0) return(1);
		*revisiondate = (UINTBIG)filedate(filename);
	} else
	{
		/* find proper view of this cell */
		if (np->cellview == view) onp = np; else
			FOR_CELLGROUP(onp, np)
				if (onp->cellview == view) break;
		if (onp == NONODEPROTO)
		{
			*intended = x_("CANNOT FIND CELL VIEW");
			return(1);
		}

		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("cell "));
		addstringtoinfstr(infstr, describenodeproto(onp));
		*intended = returninfstr(infstr);
		vhdl_var = getvalkey((INTBIG)onp, VNODEPROTO, VSTRING|VISARRAY, el_cell_message_key);
		if (vhdl_var == NOVARIABLE) return(1);
		*revisiondate = onp->revisiondate;
	}
	vhdl_linecount = 0;
	return(0);
}

INTBIG vhdl_endinput(void)
{
	if (vhdl_fp != 0)
	{
		xclose(vhdl_fp);
		vhdl_fp = 0;
	}
	if (vhdl_var != NOVARIABLE) vhdl_var = NOVARIABLE;
	return(0);
}

/*
 * Routine to get the next line from the current input file/cell into the buffer
 * at "addr".  Returns the line number (1-based).  Returns zero on EOF.
 */
INTBIG vhdl_getnextline(CHAR *addr)
{
	BOOLEAN ret;
	REGISTER CHAR *line;

	if (vhdl_fp != 0)
	{
		ret = xfgets(addr, MAXVHDLLINE-1, vhdl_fp);
		if (ret) return(0);
	} else if (vhdl_var != NOVARIABLE)
	{
		if (vhdl_linecount >= getlength(vhdl_var)) return(0);
		line = ((CHAR **)vhdl_var->addr)[vhdl_linecount];
		if (estrlen(line) >= MAXVHDLLINE)
		{
			ttyputerr(_("Error: line %ld longer than limit of %d characters"),
				vhdl_linecount+1, MAXVHDLLINE);
			estrncpy(addr, line, MAXVHDLLINE-1);
			addr[MAXVHDLLINE] = 0;
		} else
		{
			(void)estrcpy(addr, line);
		}
	} else return(0);
	vhdl_linecount++;
	return(vhdl_linecount);
}

/*
 * Routine to begin output of a file of type "filetype" (either VHDL or Netlist) that is either
 * to disk ("diskflag" true) or to a cell ("diskflag" false).  If it is to disk,
 * the file name is the cell name of cell "np".  If it is to a cell, that cell is the
 * appropriate view of "np".  Returns the intended destination in "indended".
 * Returns 1 on error, -1 if aborted.
 */
INTBIG vhdl_startoutput(NODEPROTO *np, INTBIG filetype, BOOLEAN diskflag, CHAR **intended)
{
	REGISTER CHAR *prompt;
	CHAR *truefile, *ext;
	REGISTER VIEW *view;
	REGISTER void *infstr;

	if (filetype == io_filetypevhdl)
	{
		view = el_vhdlview;            ext = x_("vhdl");
	} else if (filetype == sim_filetypequisc)
	{
		view = el_netlistquiscview;    ext = x_("sci");
	} else if (filetype == sim_filetypeals)
	{
		view = el_netlistalsview;      ext = x_("net");
	} else if (filetype == sim_filetypenetlisp)
	{
		view = el_netlistnetlispview;  ext = x_("net");
	} else if (filetype == sim_filetypesilos)
	{
		view = el_netlistsilosview;    ext = x_("sil");
	} else if (filetype == sim_filetypersim)
	{
		view = el_netlistrsimview;     ext = x_("net");
	} else view = NOVIEW;

	vhdl_fp = 0;
	vhdl_cell = NONODEPROTO;
	if (diskflag)
	{
		/* attempt to open output file */
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("file "));
		addstringtoinfstr(infstr, np->protoname);
		addstringtoinfstr(infstr, x_("."));
		addstringtoinfstr(infstr, ext);
		*intended = returninfstr(infstr);
		if (namesame(ext, x_("vhdl")) == 0) prompt = _("VHDL File"); else
			prompt = _("Netlist File");
		vhdl_fp = xcreate(&(*intended)[5], filetype, prompt, &truefile);
		if (vhdl_fp == 0)
		{
			if (truefile == 0) return(-1);
			return(1);
		}
	} else
	{
		/* make proper view of this cell */
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("cell "));
		addstringtoinfstr(infstr, np->protoname);
		addstringtoinfstr(infstr, x_("{"));
		addstringtoinfstr(infstr, view->sviewname);
		addstringtoinfstr(infstr, x_("}"));
		*intended = returninfstr(infstr);
		vhdl_cell = newnodeproto(&(*intended)[5], np->lib);
		if (vhdl_cell == NONODEPROTO) return(1);
		vhdl_stringarray = newstringarray(vhdl_tool->cluster);
	}
	return(0);
}

INTBIG vhdl_endoutput(void)
{
	if (vhdl_fp != 0)
	{
		xclose(vhdl_fp);
		vhdl_fp = 0;
	}
	if (vhdl_cell != NONODEPROTO)
	{
		stringarraytotextcell(vhdl_stringarray, vhdl_cell, FALSE);
		killstringarray(vhdl_stringarray);
		vhdl_cell = NONODEPROTO;
	}
	return(0);
}

/*
 * Routine to write a formatted string to the current output file.
 */
void vhdl_printoneline(CHAR *fstring, ...)
{
	CHAR buff[4000];
	va_list ap;

	var_start(ap, fstring);
	evsnprintf(buff, 4000, fstring, ap);
	va_end(ap);
	if (vhdl_fp == 0) addtostringarray(vhdl_stringarray, buff); else
		xprintf(vhdl_fp, x_("%s\n"), buff);
}

/*
 * Routine to write a formatted string to the current output file.
 */
void vhdl_print(CHAR *fstring, ...)
{
	INTBIG i;
	CHAR buff[4000], *sptr, save;
	va_list ap;

	var_start(ap, fstring);
	evsnprintf(buff, 4000, fstring, ap);
	va_end(ap);
	sptr = buff;
	while (estrlen(sptr) > 70)
	{
		for(i=70; i>0; i--) if (isspace(sptr[i])) break;
		if (i <= 0) break;
		save = sptr[i];   sptr[i] = 0;
		if (vhdl_fp == 0) addtostringarray(vhdl_stringarray, sptr); else
			xprintf(vhdl_fp, x_("%s\n"), sptr);
		sptr[i] = save;
		if (vhdl_target == TARGET_SILOS)
		{
			i--;
			sptr[i] = '+';
		}
		sptr = &sptr[i];
	}
	if (vhdl_fp == 0) addtostringarray(vhdl_stringarray, sptr); else
		xprintf(vhdl_fp, x_("%s\n"), sptr);
}

/*
 * Routine to determine the revision date of cell "np", including any subcells.
 */
time_t vhdl_getcelldate(NODEPROTO *np)
{
	REGISTER time_t mostrecent, subcelldate;
	REGISTER NODEINST *ni;

	mostrecent = (time_t)np->revisiondate;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex != 0) continue;

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(ni->proto, np)) continue;
		subcelldate = vhdl_getcelldate(ni->proto);
		if (subcelldate > mostrecent) mostrecent = subcelldate;
	}
	return(mostrecent);
}

/****************************** GENERATION ******************************/

/*
 * Routine to ensure that cell "np" and all subcells have VHDL on them.
 * If "force" is true, do conversion even if VHDL cell is newer.  Otherwise
 * convert only if necessary.
 */
void vhdl_convertcell(NODEPROTO *np, BOOLEAN force)
{
	REGISTER NODEPROTO *npvhdl, *onp;
	REGISTER LIBRARY *lib;
	REGISTER INTBIG i, len;
	REGISTER BOOLEAN backannotate;
	BOOLEAN vhdlondisk;
	REGISTER time_t dateoflayout;
	REGISTER VARIABLE *var;
	FILE *f;
	CHAR *intended, *filename, *desired, *firstline;

	/* cannot make VHDL for cell with no ports */
	if (np->firstportproto == NOPORTPROTO)
	{
		ttyputerr(_("Cannot convert cell %s to VHDL: it has no ports"), describenodeproto(np));
		return;
	}

	vhdlondisk = FALSE;
	var = getvalkey((INTBIG)vhdl_tool, VTOOL, VINTEGER, vhdl_vhdlondiskkey);
	if (var != NOVARIABLE && var->addr != 0) vhdlondisk = TRUE;

	if (!force)
	{
		/* determine most recent change to this or any subcell */
		dateoflayout = vhdl_getcelldate(np);

		/* if there is already VHDL that is newer, stop now */
		if (vhdlondisk)
		{
			/* look for the disk file */
			f = xopen(np->protoname, io_filetypevhdl, x_(""), &filename);
			if (f != NULL)
			{
				xclose(f);
				if (filedate(filename) >= dateoflayout) return;
			}
		} else
		{
			/* look for the cell */
			npvhdl = anyview(np, el_vhdlview);
			if (npvhdl != NONODEPROTO)
			{
				/* examine the VHDL to see if it really came from this cell */
				var = getvalkey((INTBIG)npvhdl, VNODEPROTO, VSTRING|VISARRAY, el_cell_message_key);
				if (var != NOVARIABLE)
				{
					firstline = ((CHAR **)var->addr)[0];
					desired = x_("-- VHDL automatically generated from cell ");
					len = estrlen(desired);
					if (estrncmp(firstline, desired, len) == 0)
					{
						if (estrcmp(&desired[len], describenodeproto(np)) != 0)
							dateoflayout = (time_t)(npvhdl->revisiondate + 1);
					}
				}

				if ((time_t)npvhdl->revisiondate >= dateoflayout) return;
			}
		}
	}

	/* begin output of VHDL */
	i = vhdl_startoutput(np, io_filetypevhdl, vhdlondisk, &intended);
	if (i != 0)
	{
		if (i > 0) ttyputerr(_("Cannot write %s"), intended);
		return;
	}
	ttyputmsg(_("Converting layout in cell %s, writing VHDL to %s"), describenodeproto(np),
		intended);

	/* recursively generate the VHDL */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
			onp->temp1 = 0;
	backannotate = vhdl_generatevhdl(np);
	if (backannotate)
		ttyputmsg(_("Back-annotation information has been added (library must be saved)"));

	/* that's it */
	vhdl_endoutput();
}

BOOLEAN vhdl_generatevhdl(NODEPROTO *np)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *onp, *cnp;
	REGISTER INTBIG first, i, j, instnum, linelen, thisend, gotinverters;
	REGISTER BOOLEAN backannotate;
	INTBIG componentcount;
	INTBIG special;
	REGISTER NETWORK *net;
	REGISTER PORTPROTO *pp;
	REGISTER ARCINST *ai;
	REGISTER VARIABLE *var;
	CHAR line[30], *pt, *instname, gotnand[MAXINPUTS+1], gotxnor[MAXINPUTS+1],
		gotnor[MAXINPUTS+1], **componentlist;
	REGISTER void *infstr;

	/* make sure that all nodes have names on them */
	backannotate = FALSE;
	if (asktool(net_tool, x_("name-nodes"), (INTBIG)np) != 0) backannotate = TRUE;
	if (asktool(net_tool, x_("name-nets"), (INTBIG)np) != 0) backannotate = TRUE;

	/* indicate the source of this VHDL */
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("-- VHDL automatically generated from cell "));
	addstringtoinfstr(infstr, describenodeproto(np));
	vhdl_printoneline(returninfstr(infstr));

	/* build the "entity" line */
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("entity "));
	vhdl_addstring(infstr, np->protoname, NONODEPROTO);
	addstringtoinfstr(infstr, x_(" is port("));
	vhdl_addportlist(infstr, NONODEINST, np, 0);
	addstringtoinfstr(infstr, x_(");"));
	vhdl_print(returninfstr(infstr));

	/* add the "end" line */
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("  end "));
	vhdl_addstring(infstr, np->protoname, NONODEPROTO);
	addstringtoinfstr(infstr, x_(";"));
	vhdl_print(returninfstr(infstr));

	/* now write the "architecture" line */
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("architecture "));
	vhdl_addstring(infstr, np->protoname, NONODEPROTO);
	addstringtoinfstr(infstr, x_("_BODY of "));
	vhdl_addstring(infstr, np->protoname, NONODEPROTO);
	addstringtoinfstr(infstr, x_(" is"));
	vhdl_print(returninfstr(infstr));

	/* enumerate negated arcs */
	instnum = 1;
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if ((ai->userbits&ISNEGATED) == 0) continue;
		ai->temp1 = instnum;
		instnum++;
	}

	/* write prototypes for each node */
	for(i=0; i<=MAXINPUTS; i++) gotnand[i] = gotnor[i] = gotxnor[i] = 0;
	if (vhdl_componentarray == 0)
	{
		vhdl_componentarray = newstringarray(vhdl_tool->cluster);
		if (vhdl_componentarray == 0) return(backannotate);
	}
	clearstrings(vhdl_componentarray);
	componentlist = getstringarray(vhdl_componentarray, &componentcount);
	instnum = 1;
	gotinverters = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex == 0)
		{
			/* ignore recursive references (showing icon in contents) */
			if (isiconof(ni->proto, np)) continue;
		}
		pt = vhdl_primname(ni, &special);
		if (pt[0] == 0) continue;

		/* see if the node has a name, number it if not */
		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
		if (var == NOVARIABLE) ni->temp1 = instnum++;

		/* write only once per prototype */
		if (special == BLOCKINVERTER)
		{
			gotinverters = 1;
			continue;
		}
		if (special == BLOCKNAND)
		{
			i = eatoi(&pt[4]);
			if (i <= MAXINPUTS) gotnand[i]++; else
				ttyputerr(x_("Cannot handle %ld-input NAND, limit is %d"), i, MAXINPUTS);
			continue;
		}
		if (special == BLOCKNOR)
		{
			i = eatoi(&pt[3]);
			if (i <= MAXINPUTS) gotnor[i]++; else
				ttyputerr(x_("Cannot handle %ld-input NOR, limit is %d"), i, MAXINPUTS);
			continue;
		}
		if (special == BLOCKXNOR)
		{
			i = eatoi(&pt[4]);
			if (i <= MAXINPUTS) gotxnor[i]++; else
				ttyputerr(x_("Cannot handle %ld-input XNOR, limit is %d"), i, MAXINPUTS);
			continue;
		}

		/* ignore component with no ports */
		if (ni->proto->firstportproto == NOPORTPROTO) continue;

		/* see if this component is already written to the header */
		for(i=0; i<componentcount; i++)
			if (namesame(pt, componentlist[i]) == 0) break;
		if (i < componentcount) continue;

		/* new component: add to the list */
		addtostringarray(vhdl_componentarray, pt);
		componentlist = getstringarray(vhdl_componentarray, &componentcount);

		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("  component "));
		vhdl_addstring(infstr, pt, NONODEPROTO);
		addstringtoinfstr(infstr, x_(" port("));
		vhdl_addportlist(infstr, ni, ni->proto, special);
		addstringtoinfstr(infstr, x_(");"));
		vhdl_print(returninfstr(infstr));
		vhdl_print(x_("    end component;"));
	}

	/* write pseudo-prototype if there are any negated arcs */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		if ((ai->userbits&ISNEGATED) != 0 && ai->temp1 != 0)
	{
		gotinverters = 1;
		break;
	}
	if (gotinverters != 0)
	{
		vhdl_print(x_("  component inverter port(a: in BIT; y: out BIT);"));
		vhdl_print(x_("    end component;"));
	}
	for(i=0; i<MAXINPUTS; i++)
	{
		if (gotnand[i] != 0)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("  component nand"));
			(void)esnprintf(line, 30, x_("%ld"), i);
			addstringtoinfstr(infstr, line);
			addstringtoinfstr(infstr, x_(" port("));
			for(j=1; j<=i; j++)
			{
				if (j > 1) addstringtoinfstr(infstr, x_(", "));
				(void)esnprintf(line, 30, x_("a%ld"), j);
				addstringtoinfstr(infstr, line);
			}
			addstringtoinfstr(infstr, x_(": in BIT; y: out BIT);"));
			vhdl_print(returninfstr(infstr));
			vhdl_print(x_("    end component;"));
		}
		if (gotnor[i] != 0)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("  component nor"));
			(void)esnprintf(line, 30, x_("%ld"), i);
			addstringtoinfstr(infstr, line);
			addstringtoinfstr(infstr, x_(" port("));
			for(j=1; j<=i; j++)
			{
				if (j > 1) addstringtoinfstr(infstr, x_(", "));
				(void)esnprintf(line, 30, x_("a%ld"), j);
				addstringtoinfstr(infstr, line);
			}
			addstringtoinfstr(infstr, x_(": in BIT; y: out BIT);"));
			vhdl_print(returninfstr(infstr));
			vhdl_print(x_("    end component;"));
		}
		if (gotxnor[i] != 0)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("  component xnor"));
			(void)esnprintf(line, 30, x_("%ld"), i);
			addstringtoinfstr(infstr, line);
			addstringtoinfstr(infstr, x_(" port("));
			for(j=1; j<=i; j++)
			{
				if (j > 1) addstringtoinfstr(infstr, x_(", "));
				(void)esnprintf(line, 30, x_("a%ld"), j);
				addstringtoinfstr(infstr, line);
			}
			addstringtoinfstr(infstr, x_(": in BIT; y: out BIT);"));
			vhdl_print(returninfstr(infstr));
			vhdl_print(x_("    end component;"));
		}
	}

	/* write internal nodes */
	for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		net->temp1 = 0;
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		pp->network->temp1 = 1;
		if (pp->network->buswidth > 1)
		{
			for(i=0; i<pp->network->buswidth; i++)
				pp->network->networklist[i]->temp1 = 1;
		}
	}
	first = 0;
	linelen = 0;
	for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
	{
		if (net->portcount != 0 || net->buslinkcount != 0) continue;

		/* disallow if part of a bus export or already written */
		if (net->temp1 != 0) continue;
		for(i=0; i<net->buswidth; i++)
		{
			if (net->buswidth <= 1)
			{
				if (net->namecount > 0) pt = networkname(net, 0); else
					pt = describenetwork(net);
				net->temp1 = 1;
			} else
			{
				if (net->networklist[i]->temp1 != 0) continue;
				pt = networkname(net->networklist[i], 0);
				net->networklist[i]->temp1 = 1;
				pt = describenetwork(net->networklist[i]);
			}
			if (first == 0)
			{
				infstr = initinfstr();
				addstringtoinfstr(infstr, x_("  signal "));
				linelen = 9;
			} else
			{
				addstringtoinfstr(infstr, x_(", "));
				linelen += 2;
			}
			first = 1;
			if (linelen + estrlen(pt) > 80)
			{
				vhdl_print(returninfstr(infstr));
				infstr = initinfstr();
				addstringtoinfstr(infstr, x_("    "));
				linelen = 4;
			}
			vhdl_addstring(infstr, pt, np);
			linelen += estrlen(pt);
		}
	}
	if (first != 0)
	{
		addstringtoinfstr(infstr, x_(": BIT;"));
		vhdl_print(returninfstr(infstr));
	}

	/* write pseudo-internal nodes for all negated arcs */
	first = 0;
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if ((ai->userbits&ISNEGATED) == 0 || ai->temp1 == 0) continue;
		if (first == 0)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("  signal "));
		} else addstringtoinfstr(infstr, x_(", "));
		(void)esnprintf(line, 30, x_("PINV%ld"), ai->temp1);
		addstringtoinfstr(infstr, line);
		first++;
	}
	if (first != 0)
	{
		addstringtoinfstr(infstr, x_(": BIT;"));
		vhdl_print(returninfstr(infstr));
	}

	/* write the instances */
	vhdl_print(x_("begin"));
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* ignore component with no ports */
		if (ni->proto->firstportproto == NOPORTPROTO) continue;

		if (ni->proto->primindex == 0)
		{
			/* ignore recursive references (showing icon in contents) */
			if (isiconof(ni->proto, np)) continue;
		}

		pt = vhdl_primname(ni, &special);
		if (pt[0] == 0) continue;

		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("  "));
		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
		if (var != NOVARIABLE)
		{
			instname = (CHAR *)var->addr;
			vhdl_addstring(infstr, instname, NONODEPROTO);

			/* make sure the instance name doesn't conflict with a prototype name */
			for(i=0; i<componentcount; i++)
				if (namesame(instname, componentlist[i]) == 0) break;
			if (i < componentcount) addstringtoinfstr(infstr, x_("NV"));
		} else
		{
			ttyputerr(x_("VHDL conversion warning: no name on node %s"), describenodeinst(ni));
			(void)esnprintf(line, 30, x_("NODE%ld"), (INTBIG)ni);
			addstringtoinfstr(infstr, line);
		}
		addstringtoinfstr(infstr, x_(": "));
		vhdl_addstring(infstr, pt, NONODEPROTO);

		addstringtoinfstr(infstr, x_(" port map("));
		vhdl_addrealports(infstr, ni, special);
		addstringtoinfstr(infstr, x_(");"));
		vhdl_print(returninfstr(infstr));
	}

	/* write pseudo-nodes for all negated arcs */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if ((ai->userbits&ISNEGATED) == 0 || ai->temp1 == 0) continue;
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("  PSEUDO_INVERT"));
		(void)esnprintf(line, 30, x_("%ld"), ai->temp1);
		addstringtoinfstr(infstr, line);
		addstringtoinfstr(infstr, x_(": inverter port map("));
		if ((ai->userbits&REVERSEEND) == 0) thisend = 0; else thisend = 1;
		if ((ai->end[thisend].portarcinst->proto->userbits&STATEBITS) == OUTPORT)
		{
			(void)esnprintf(line, 30, x_("PINV%ld"), ai->temp1);
			addstringtoinfstr(infstr, line);
			addstringtoinfstr(infstr, x_(", "));
			net = ai->network;
			if (net->namecount > 0) vhdl_addstring(infstr, networkname(net, 0), np); else
				addstringtoinfstr(infstr, describenetwork(net));
		} else
		{
			net = ai->network;
			if (net->namecount > 0) vhdl_addstring(infstr, networkname(net, 0), np); else
				addstringtoinfstr(infstr, describenetwork(net));
			addstringtoinfstr(infstr, x_(", "));
			(void)esnprintf(line, 30, x_("PINV%ld"), ai->temp1);
			addstringtoinfstr(infstr, line);
		}
		addstringtoinfstr(infstr, x_(");"));
		vhdl_print(returninfstr(infstr));
	}

	/* write the end of the body */
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("end "));
	vhdl_addstring(infstr, np->protoname, NONODEPROTO);
	addstringtoinfstr(infstr, x_("_BODY;"));
	vhdl_print(returninfstr(infstr));
	vhdl_print(x_(""));

	/* finally, generate VHDL for all subcells */
	np->temp1 = 1;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex != 0) continue;

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(ni->proto, np)) continue;
		if (ni->proto->temp1 != 0) continue;
		cnp = contentsview(ni->proto);
		if (cnp == NONODEPROTO) cnp = ni->proto;
		if (cnp->temp1 != 0) continue;

		/* ignore component with no ports */
		if (cnp->firstportproto == NOPORTPROTO) continue;

		/* if ALS, see if this cell is in the reference library */
		if (vhdl_lib != NOLIBRARY && vhdl_target == TARGET_ALS)
		{
			for(onp = vhdl_lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
				if (onp->cellview == el_netlistalsview &&
					namesame(onp->protoname, cnp->protoname) == 0) break;
			if (onp != NONODEPROTO) continue;
		}

		downhierarchy(ni, cnp, 0);
		if (vhdl_generatevhdl(cnp)) backannotate = TRUE;
		uphierarchy();
	}
	return(backannotate);
}

void vhdl_addrealports(void *infstr, NODEINST *ni, INTBIG special)
{
	REGISTER INTBIG first, pass, i;
	REGISTER PORTPROTO *pp, *opp, *cpp;
	REGISTER NODEPROTO *np, *cnp;
	REGISTER ARCINST *ai;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER NETWORK *net, *subnet;
	CHAR line[50];

	np = ni->proto;
	cnp = contentsview(np);
	if (cnp == NONODEPROTO) cnp = np;
	first = 0;
	for(pass = 0; pass < 5; pass++)
	{
		for(cpp = cnp->firstportproto; cpp != NOPORTPROTO; cpp = cpp->nextportproto)
		{
#ifdef IGNORE4PORTTRANSISTORS
			/* ignore the bias port of 4-port transistors */
			if (ni->proto == sch_transistor4prim)
			{
				if (namesame(cpp->protoname, x_("b")) == 0) continue;
			}
#endif
			if (cnp == np) pp = cpp; else
				pp = equivalentport(cnp, cpp, np);
			if (pass == 0)
			{
				/* must be an input port */
				if ((pp->userbits&STATEBITS) != INPORT) continue;
			}
			if (pass == 1)
			{
				/* must be an output port */
				if ((pp->userbits&STATEBITS) != OUTPORT) continue;
			}
			if (pass == 2)
			{
				/* must be an output port */
				if ((pp->userbits&STATEBITS) != PWRPORT) continue;
			}
			if (pass == 3)
			{
				/* must be an output port */
				if ((pp->userbits&STATEBITS) != GNDPORT) continue;
			}
			if (pass == 4)
			{
				/* any other port type */
				if ((pp->userbits&STATEBITS) == INPORT || (pp->userbits&STATEBITS) == OUTPORT ||
					(pp->userbits&STATEBITS) == PWRPORT || (pp->userbits&STATEBITS) == GNDPORT)
						continue;
			}

			if (special == BLOCKMOSTRAN)
			{
				/* ignore electrically connected ports */
				for(opp = np->firstportproto; opp != pp; opp = opp->nextportproto)
					if (opp->network == pp->network) break;
				if (opp != pp) continue;
			}
			if (special == BLOCKPOSLOGIC || special == BLOCKBUFFER || special == BLOCKINVERTER ||
				special == BLOCKNAND || special == BLOCKNOR || special == BLOCKXNOR)
			{
				/* ignore ports not named "a" or "y" */
				if (estrcmp(pp->protoname, x_("a")) != 0 && estrcmp(pp->protoname, x_("y")) != 0)
				{
					pp->temp1 = 1;
					continue;
				}
			}
			if (special == BLOCKFLOPTS || special == BLOCKFLOPDS)
			{
				/* ignore ports not named "i1", "ck", "preset", or "q" */
				if (estrcmp(pp->protoname, x_("i1")) != 0 && estrcmp(pp->protoname, x_("ck")) != 0 &&
					estrcmp(pp->protoname, x_("preset")) != 0 && estrcmp(pp->protoname, x_("q")) != 0)
				{
					pp->temp1 = 1;
					continue;
				}
			}
			if (special == BLOCKFLOPTR || special == BLOCKFLOPDR)
			{
				/* ignore ports not named "i1", "ck", "clear", or "q" */
				if (estrcmp(pp->protoname, x_("i1")) != 0 && estrcmp(pp->protoname, x_("ck")) != 0 &&
					estrcmp(pp->protoname, x_("clear")) != 0 && estrcmp(pp->protoname, x_("q")) != 0)
				{
					pp->temp1 = 1;
					continue;
				}
			}

			/* if multiple connections, get them all */
			if ((pp->userbits&PORTISOLATED) != 0)
			{
				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				{
					if (pi->proto != pp) continue;
					ai = pi->conarcinst;
					if (((ai->proto->userbits&AFUNCTION)>>AFUNCTIONSH) == APNONELEC) continue;
					if (first != 0) addstringtoinfstr(infstr, x_(", "));   first = 1;
					if ((ai->userbits&ISNEGATED) != 0)
					{
						if ((ai->end[0].portarcinst == pi && (ai->userbits&REVERSEEND) == 0) ||
							(ai->end[1].portarcinst == pi && (ai->userbits&REVERSEEND) != 0))
						{
							(void)esnprintf(line, 50, x_("PINV%ld"), ai->temp1);
							addstringtoinfstr(infstr, line);
							continue;
						}
					}
					net = ai->network;
					if (net->namecount > 0) vhdl_addstring(infstr, networkname(net, 0), ni->parent); else
						addstringtoinfstr(infstr, describenetwork(net));
				}
				continue;
			}

			/* get connection */
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				if (pi->proto->network == pp->network) break;
			if (pi != NOPORTARCINST)
			{
				ai = pi->conarcinst;
				if (((ai->proto->userbits&AFUNCTION)>>AFUNCTIONSH) != APNONELEC)
				{
					if (first != 0) addstringtoinfstr(infstr, x_(", "));   first = 1;
					if ((ai->userbits&ISNEGATED) != 0 && ai->temp1 != 0)
					{
						if ((ai->end[0].portarcinst == pi && (ai->userbits&REVERSEEND) == 0) ||
							(ai->end[1].portarcinst == pi && (ai->userbits&REVERSEEND) != 0))
						{
							(void)esnprintf(line, 50, x_("PINV%ld"), ai->temp1);
							addstringtoinfstr(infstr, line);
							continue;
						}
					}

					net = ai->network;
					if (net->buswidth > 1)
					{
						for(i=0; i<net->buswidth; i++)
						{
							if (i != 0) addstringtoinfstr(infstr, x_(", "));
							subnet = net->networklist[i];
							if (subnet->namecount > 0)
								vhdl_addstring(infstr, networkname(subnet, 0), ni->parent);
							else
								addstringtoinfstr(infstr, describenetwork(subnet));
						}
					} else
					{
						if (net->namecount > 0)
							vhdl_addstring(infstr, networkname(net, 0), ni->parent);
						else
							addstringtoinfstr(infstr, describenetwork(net));
					}
					continue;
				}
			}

			/* see if this is an export */
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
				if (pe->proto->network == pp->network) break;
			if (pe != NOPORTEXPINST)
			{
				if (first != 0) addstringtoinfstr(infstr, x_(", "));   first = 1;
				vhdl_addstring(infstr, pe->exportproto->protoname, ni->parent);
				continue;
			}

			/* port is not connected or an export */
			if (first != 0) addstringtoinfstr(infstr, x_(", "));   first = 1;
			addstringtoinfstr(infstr, x_("open"));
			ttyputmsg(_("Warning: port %s of node %s is not connected"),
				cpp->protoname, describenodeinst(ni));
		}
	}
}

/*
 * routine to return the VHDL name to use for node "ni".  Returns a "special" value
 * that indicates the nature of the node.
 *  BLOCKNORMAL: no special port arrangements necessary
 *  BLOCKMOSTRAN: only output ports that are not electrically connected
 *  BLOCKBUFFER: only include input port "a" and output port "y"
 *  BLOCKPOSLOGIC: only include input port "a" and output port "y"
 *  BLOCKINVERTER: only include input port "a" and output port "y"
 *  BLOCKNAND: only include input port "a" and output port "y"
 *  BLOCKNOR: only include input port "a" and output port "y"
 *  BLOCKXNOR: only include input port "a" and output port "y"
 *  BLOCKFLOPTS: only include input ports "i1", "ck", "preset" and output port "q"
 *  BLOCKFLOPTR: only include input ports "i1", "ck", "clear" and output port "q"
 *  BLOCKFLOPDS: only include input ports "i1", "ck", "preset" and output port "q"
 *  BLOCKFLOPDR: only include input ports "i1", "ck", "clear" and output port "q"
 *  BLOCKFLOP: include input ports "i1", "i2", "ck", "preset", "clear", and output ports "q" and "qb"
 */
CHAR *vhdl_primname(NODEINST *ni, INTBIG *special)
{
	REGISTER INTBIG k, inport, isneg;
	static INTBIG tech_vhdl_names_key = 0;
	REGISTER CHAR *str, *ptr;
	REGISTER VARIABLE *var;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER PORTPROTO *pp;
	REGISTER ARCINST *ai;
	static CHAR pt[30];

	/* initialize */
	if (tech_vhdl_names_key == 0) tech_vhdl_names_key = makekey(x_("TECH_vhdl_names"));

	/* cell instances are easy */
	*special = BLOCKNORMAL;
	if (ni->proto->primindex == 0) return(ni->proto->protoname);

	/* get the primitive function */
	k = nodefunction(ni);
	pt[0] = 0;
	ai = NOARCINST;
	switch (k)
	{
		case NPTRANMOS:   case NPTRA4NMOS:
			var = getvalkey((INTBIG)ni, VNODEINST, -1, sim_weaknodekey);
			if (var == NOVARIABLE) (void)estrcpy(pt, x_("nMOStran")); else
				(void)estrcpy(pt, x_("nMOStranWeak"));
			*special = BLOCKMOSTRAN;
			break;
		case NPTRADMOS:   case NPTRA4DMOS:
			(void)estrcpy(pt, x_("DMOStran"));
			*special = BLOCKMOSTRAN;
			break;
		case NPTRAPMOS:   case NPTRA4PMOS:
			var = getvalkey((INTBIG)ni, VNODEINST, -1, sim_weaknodekey);
			if (var == NOVARIABLE) (void)estrcpy(pt, x_("PMOStran")); else
				(void)estrcpy(pt, x_("pMOStranWeak"));
			*special = BLOCKMOSTRAN;
			break;
		case NPTRANPN:    case NPTRA4NPN:
			(void)estrcpy(pt, x_("NPNtran"));
			break;
		case NPTRAPNP:    case NPTRA4PNP:
			(void)estrcpy(pt, x_("PNPtran"));
			break;
		case NPTRANJFET:  case NPTRA4NJFET:
			(void)estrcpy(pt, x_("NJFET"));
			break;
		case NPTRAPJFET:  case NPTRA4PJFET:
			(void)estrcpy(pt, x_("PJFET"));
			break;
		case NPTRADMES:   case NPTRA4DMES:
			(void)estrcpy(pt, x_("DMEStran"));
			break;
		case NPTRAEMES:   case NPTRA4EMES:
			(void)estrcpy(pt, x_("EMEStran"));
			break;

		case NPFLIPFLOP:
			switch (ni->userbits&FFTYPE)
			{
				case FFTYPERS:
					(void)estrcpy(pt, x_("rsff"));
					*special = BLOCKFLOP;
					break;
				case FFTYPEJK:
					(void)estrcpy(pt, x_("jkff"));
					*special = BLOCKFLOP;
					break;
				case FFTYPED:
					(void)estrcpy(pt, x_("dsff"));
					*special = BLOCKFLOPDS;
					for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
					{
						if (namesame(pi->proto->protoname, x_("clear")) == 0)
						{
							(void)estrcpy(pt, x_("drff"));
							*special = BLOCKFLOPDR;
							break;
						}
					}
					break;
				case FFTYPET:
					(void)estrcpy(pt, x_("tsff"));
					*special = BLOCKFLOPTS;
					for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
					{
						if (namesame(pi->proto->protoname, x_("clear")) == 0)
						{
							(void)estrcpy(pt, x_("trff"));
							*special = BLOCKFLOPTR;
							break;
						}
					}
					break;
				default:
					(void)estrcpy(pt, x_("fflop"));
					*special = BLOCKFLOP;
					break;
			}
			break;

		case NPBUFFER:
			var = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VSTRING|VISARRAY, tech_vhdl_names_key);
			if (var == NOVARIABLE) str = x_("buffer/inverter"); else
				str = ((CHAR **)var->addr)[ni->proto->primindex-1];
			*special = BLOCKBUFFER;
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			{
				if (estrcmp(pi->proto->protoname, x_("y")) != 0) continue;
				ai = pi->conarcinst;
				if ((ai->userbits&ISNEGATED) == 0) continue;
				if (ai->end[0].portarcinst == pi && (ai->userbits&REVERSEEND) == 0) break;
				if (ai->end[1].portarcinst == pi && (ai->userbits&REVERSEEND) != 0) break;
			}
			if (pi != NOPORTARCINST)
			{
				for(ptr = str; *ptr != 0; ptr++) if (*ptr == '/') break;
				if (*ptr == '/') estrcpy(pt, &ptr[1]); else
					estrcpy(pt, str);
				*special = BLOCKINVERTER;
				ai->temp1 = 0;
			} else
			{
				estrcpy(pt, str);
				for(ptr = pt; *ptr != 0; ptr++) if (*ptr == '/') break;
				if (*ptr == '/') *ptr = 0;					
			}
			break;
		case NPGATEAND:
			var = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VSTRING|VISARRAY, tech_vhdl_names_key);
			if (var == NOVARIABLE) str = x_("and%ld/nand%ld"); else
				str = ((CHAR **)var->addr)[ni->proto->primindex-1];
			inport = isneg = 0;
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			{
				if (estrcmp(pi->proto->protoname, x_("a")) == 0) inport++;
				if (estrcmp(pi->proto->protoname, x_("y")) != 0) continue;
				ai = pi->conarcinst;
				if ((ai->userbits&ISNEGATED) == 0) continue;
				if (ai->end[0].portarcinst == pi && (ai->userbits&REVERSEEND) == 0) isneg++;
				if (ai->end[1].portarcinst == pi && (ai->userbits&REVERSEEND) != 0) isneg++;
			}
			if (isneg != 0)
			{
				for(ptr = str; *ptr != 0; ptr++) if (*ptr == '/') break;
				if (*ptr == '/') (void)esnprintf(pt, 30, &ptr[1], inport); else
					(void)esnprintf(pt, 30, str, inport);
				*special = BLOCKNAND;
				ai->temp1 = 0;
			} else
			{
				(void)esnprintf(pt, 30, str, inport);
				for(ptr = pt; *ptr != 0; ptr++) if (*ptr == '/') break;
				if (*ptr == '/') *ptr = 0;					
				*special = BLOCKPOSLOGIC;
			}
			break;
		case NPGATEOR:
			var = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VSTRING|VISARRAY, tech_vhdl_names_key);
			if (var == NOVARIABLE) str = x_("or%ld/nor%ld"); else
				str = ((CHAR **)var->addr)[ni->proto->primindex-1];
			inport = isneg = 0;
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			{
				if (estrcmp(pi->proto->protoname, x_("a")) == 0) inport++;
				if (estrcmp(pi->proto->protoname, x_("y")) != 0) continue;
				ai = pi->conarcinst;
				if ((ai->userbits&ISNEGATED) == 0) continue;
				if (ai->end[0].portarcinst == pi && (ai->userbits&REVERSEEND) == 0) isneg++;
				if (ai->end[1].portarcinst == pi && (ai->userbits&REVERSEEND) != 0) isneg++;
			}
			if (isneg != 0)
			{
				for(ptr = str; *ptr != 0; ptr++) if (*ptr == '/') break;
				if (*ptr == '/') (void)esnprintf(pt, 30, &ptr[1], inport); else
					(void)esnprintf(pt, 30, str, inport);
				*special = BLOCKNOR;
				ai->temp1 = 0;
			} else
			{
				(void)esnprintf(pt, 30, str, inport);
				for(ptr = pt; *ptr != 0; ptr++) if (*ptr == '/') break;
				if (*ptr == '/') *ptr = 0;					
				*special = BLOCKPOSLOGIC;
			}
			break;
		case NPGATEXOR:
			var = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VSTRING|VISARRAY, tech_vhdl_names_key);
			if (var == NOVARIABLE) str = x_("xor%ld/xnor%ld"); else
				str = ((CHAR **)var->addr)[ni->proto->primindex-1];
			inport = isneg = 0;
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			{
				if (estrcmp(pi->proto->protoname, x_("a")) == 0) inport++;
				if (estrcmp(pi->proto->protoname, x_("y")) != 0) continue;
				ai = pi->conarcinst;
				if ((ai->userbits&ISNEGATED) == 0) continue;
				if (ai->end[0].portarcinst == pi && (ai->userbits&REVERSEEND) == 0) isneg++;
				if (ai->end[1].portarcinst == pi && (ai->userbits&REVERSEEND) != 0) isneg++;
			}
			if (isneg != 0)
			{
				for(ptr = str; *ptr != 0; ptr++) if (*ptr == '/') break;
				if (*ptr == '/') (void)esnprintf(pt, 30, &ptr[1], inport); else
					(void)esnprintf(pt, 30, str, inport);
				*special = BLOCKXNOR;
				ai->temp1 = 0;
			} else
			{
				(void)esnprintf(pt, 30, str, inport);
				for(ptr = pt; *ptr != 0; ptr++) if (*ptr == '/') break;
				if (*ptr == '/') *ptr = 0;					
				*special = BLOCKPOSLOGIC;
			}
			break;
		case NPMUX:
			var = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VSTRING|VISARRAY, tech_vhdl_names_key);
			if (var == NOVARIABLE) str = x_("mux%ld"); else
				str = ((CHAR **)var->addr)[ni->proto->primindex-1];
			inport = isneg = 0;
			inport = 0;
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				if (estrcmp(pi->proto->protoname, x_("a")) == 0) inport++;
			(void)esnprintf(pt, 30, str, inport);
			break;
		case NPCONPOWER:
			estrcpy(pt, x_("power"));
			break;
		case NPCONGROUND:
			estrcpy(pt, x_("ground"));
			break;
	}
	if (*pt == 0)
	{
		/* if the node has an export with power/ground, make it that */
		for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		{
			pp = pe->exportproto;
			if (portispower(pp))
			{
				estrcpy(pt, x_("power"));
				break;
			}
			if (portisground(pp))
			{
				estrcpy(pt, x_("ground"));
				break;
			}
		}
	}
	return(pt);
}

/*
 * Routine to add the string "orig" to the infinite string.
 * If "environment" is not NONODEPROTO, it is the cell in which this signal is
 * to reside, and if that cell has nodes with this name, the signal must be renamed.
 */
void vhdl_addstring(void *infstr, CHAR *orig, NODEPROTO *environment)
{
	REGISTER CHAR *pt;
	REGISTER INTBIG len, nonalnum;
	REGISTER NODEINST *ni;

	/* remove all nonVHDL characters while adding to current string */
	nonalnum = 0;
	for(pt = orig; *pt != 0; pt++)
		if (isalnum(*pt)) addtoinfstr(infstr, *pt); else
	{
		addtoinfstr(infstr, '_');
		nonalnum++;
	}

	/* if there were nonalphanumeric characters, this cannot be a VHDL keyword */
	if (nonalnum == 0)
	{
		/* check for VHDL keyword clashes */
		len = estrlen(orig);
		if (vhdl_iskeyword(orig, len) != NOVKEYWORD)
		{
			addstringtoinfstr(infstr, x_("NV"));
			return;
		}

		/* "bit" isn't a keyword, but the compiler can't handle it */
		if (namesame(orig, x_("bit")) == 0)
		{
			addstringtoinfstr(infstr, x_("NV"));
			return;
		}
	}

	/* see if there is a name clash */
	if (environment != NONODEPROTO)
	{
		for(ni = environment->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if (ni->proto->primindex != 0) continue;
			if (namesame(orig, ni->proto->protoname) == 0) break;
		}
		if (ni != NONODEINST)
		{
			addstringtoinfstr(infstr, x_("NV"));
			return;
		}
	}
}

/*
 * routine to add, to the infinite string, VHDL for the ports on instance "ni"
 * (which is of prototype "np").  If "ni" is NONODEINST, use only prototype information,
 * otherwise, treat each connection on an isolated port as a separate port.
 * If "special" is BLOCKMOSTRAN, only list ports that are not electrically connected.
 * If "special" is BLOCKPOSLOGIC, BLOCKBUFFER or BLOCKINVERTER, only include input
 *    port "a" and output port "y".
 * If "special" is BLOCKFLOPTS or BLOCKFLOPDS, only include input ports "i1", "ck", "preset"
 *    and output port "q".
 * If "special" is BLOCKFLOPTR or BLOCKFLOPDR, only include input ports "i1", "ck", "clear"
 *    and output port "q".
 */
void vhdl_addportlist(void *infstr, NODEINST *ni, NODEPROTO *np, INTBIG special)
{
	REGISTER PORTPROTO *pp, *opp;
	REGISTER NODEPROTO *cnp;
	CHAR before[10];

	if (special == BLOCKFLOPTS || special == BLOCKFLOPDS)
	{
		addstringtoinfstr(infstr, x_("i1, ck, preset: in BIT; q: out BIT"));
		return;
	}
	if (special == BLOCKFLOPTR || special == BLOCKFLOPDR)
	{
		addstringtoinfstr(infstr, x_("i1, ck, clear: in BIT; q: out BIT"));
		return;
	}

	/* if this is an icon, use the contents */
	if (np->primindex == 0 && np->cellview == el_iconview)
	{
		cnp = contentsview(np);
		if (cnp != NONODEPROTO)
		{
			/* make sure that the ports correspond */
			for(pp = cnp->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				(void)equivalentport(cnp, pp, np);
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				(void)equivalentport(np, pp, cnp);
			np = cnp;
		}
	}

	/* flag important ports */
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		pp->temp1 = 0;
		if (special == BLOCKMOSTRAN)
		{
			/* ignore ports that are electrically connected to previous ones */
			for(opp = np->firstportproto; opp != pp; opp = opp->nextportproto)
				if (opp->network == pp->network) break;
			if (opp != pp) { pp->temp1 = 1;   continue; }
		}
		if (special == BLOCKPOSLOGIC || special == BLOCKBUFFER || special == BLOCKINVERTER)
		{
			/* ignore ports not named "a" or "y" */
			if (estrcmp(pp->protoname, x_("a")) != 0 && estrcmp(pp->protoname, x_("y")) != 0)
			{
				pp->temp1 = 1;
				continue;
			}
		}
	}

	(void)estrcpy(before, x_(""));
	vhdl_addtheseports(infstr, ni, np, INPORT, before);
	vhdl_addtheseports(infstr, ni, np, OUTPORT, before);
	vhdl_addtheseports(infstr, ni, np, PWRPORT, before);
	vhdl_addtheseports(infstr, ni, np, GNDPORT, before);
	vhdl_addtheseports(infstr, ni, np, 0, before);
}

void vhdl_addtheseports(void *infstr, NODEINST *ni, NODEPROTO *np, UINTBIG bits, CHAR *before)
{
	REGISTER PORTPROTO *pp;
	REGISTER PORTARCINST *pi;
	REGISTER BOOLEAN didsome;
	REGISTER INTBIG i, count, inst;
	CHAR instname[10], **strings;

	didsome = FALSE;
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
#ifdef IGNORE4PORTTRANSISTORS
		if (np == sch_transistor4prim)
		{
			if (namesame(pp->protoname, x_("b")) == 0) continue;
		}
#endif
		if (pp->temp1 != 0) continue;
		if (bits == 0)
		{
			if ((pp->userbits&STATEBITS) == INPORT || (pp->userbits&STATEBITS) == OUTPORT ||
				(pp->userbits&STATEBITS) == PWRPORT || (pp->userbits&STATEBITS) == GNDPORT)
					continue;
		} else
		{
			if ((pp->userbits&STATEBITS) != bits) continue;
		}
		if (ni != NONODEINST && (pp->userbits&PORTISOLATED) != 0)
		{
			inst = 1;
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			{
				if (pi->proto != pp) continue;
				count = net_evalbusname(APBUS, pp->protoname, &strings, NOARCINST, np, 0);
				for(i=0; i<count; i++)
				{
					addstringtoinfstr(infstr, before);
					(void)estrcpy(before, x_(", "));
					vhdl_addstring(infstr, strings[i], np);
					(void)esnprintf(instname, 10, x_("%ld"), inst);
					addstringtoinfstr(infstr, instname);
					didsome = TRUE;
				}
				inst++;
			}
		} else
		{
			count = net_evalbusname(APBUS, pp->protoname, &strings, NOARCINST, np, 0);
			for(i=0; i<count; i++)
			{
				addstringtoinfstr(infstr, before);
				(void)estrcpy(before, x_(", "));
				vhdl_addstring(infstr, strings[i], np);
				didsome = TRUE;
			}
		}
	}
	if (didsome)
	{
		if (bits == INPORT)
		{
			addstringtoinfstr(infstr, x_(": in BIT"));
		} else if (bits == OUTPORT || bits == PWRPORT || bits == GNDPORT)
		{
			addstringtoinfstr(infstr, x_(": out BIT"));
		} else
		{
			addstringtoinfstr(infstr, x_(": inout BIT"));
		}
		(void)estrcpy(before, x_("; "));
	}
}

/****************************** COMPILATION ******************************/

/*
 * routine to compile the VHDL in "basecell" (or wherever the related VHDL
 * view is).  If "toplevel" is nonzero, this network is the final output, so
 * it MUST be recompiled if any subcells have changed.  Returns true
 * on error.
 */
BOOLEAN vhdl_compile(NODEPROTO *basecell)
{
	CHAR *intended, *fromintended;
	INTBIG	i, filetype;
	BOOLEAN err;
	REGISTER VARIABLE *var;
	UINTBIG	netlistdate, vhdldate;
	INTBIG retval;
	BOOLEAN vhdlondisk, netlistondisk;

	/* determine netlist type */
	switch (vhdl_target)
	{
		case TARGET_ALS:     filetype = sim_filetypeals;     break;
		case TARGET_NETLISP: filetype = sim_filetypenetlisp; break;
		case TARGET_RSIM:    filetype = sim_filetypersim;    break;
		case TARGET_SILOS:   filetype = sim_filetypesilos;   break;
		case TARGET_QUISC:   filetype = sim_filetypequisc;   break;
	}

	var = getvalkey((INTBIG)vhdl_tool, VTOOL, VINTEGER, vhdl_vhdlondiskkey);
	vhdlondisk = FALSE;
	if (var != NOVARIABLE && var->addr != 0) vhdlondisk = TRUE;
	var = getvalkey((INTBIG)vhdl_tool, VTOOL, VINTEGER, vhdl_netlistondiskkey);
	netlistondisk = FALSE;
	if (var != NOVARIABLE && var->addr != 0) netlistondisk = TRUE;

	/* see if there is netlist associated with this cell */
	if (vhdl_startinput(basecell, filetype, netlistondisk, &intended, &netlistdate) == 0)
		err = FALSE; else
			err = TRUE;
	if (!err)
	{
		/* got netlist, don't need to read it, just wanted the date and existence */
		vhdl_endinput();
	}

	/* get VHDL */
	if (vhdl_startinput(basecell, io_filetypevhdl, vhdlondisk, &intended, &vhdldate) != 0)
	{
		/* issue error if this isn't an icon */
		if (basecell->cellview != el_iconview)
			ttyputerr(_("Cannot find VHDL for %s"), basecell->protoname);
		return(TRUE);
	}

	/* if there is a newer netlist, stop now */
	if (!err && netlistdate >= vhdldate)
	{
		vhdl_endinput();
		return(FALSE);
	}

	/* build and clear vhdl_identtable */
	if (vhdl_identtable == 0)
	{
		vhdl_identtable = (IDENTTABLE *)emalloc((INTBIG)IDENT_TABLE_SIZE *
			(INTBIG)(sizeof (IDENTTABLE)), vhdl_tool->cluster);
		if (vhdl_identtable == 0) return(TRUE);
		for (i = 0; i < IDENT_TABLE_SIZE; i++)
			vhdl_identtable[i].string = (CHAR *)0;
	}

	if (allocstring(&fromintended, intended, el_tempcluster)) return(TRUE);
	vhdl_errorcount = 0;
	vhdl_scanner();
	(void)vhdl_endinput();
	err = vhdl_parser(vhdl_tliststart);
	if (!err) err = vhdl_semantic();
	if (err)
	{
		ttyputmsg(_("ERRORS during compilation, no output produced"));
		efree(fromintended);
		return(TRUE);
	}

	/* prepare to create netlist */
	retval = vhdl_startoutput(basecell, filetype, netlistondisk, &intended);
	if (retval != 0)
	{
		if (retval > 0) ttyputerr(_("Cannot write %s"), intended);
		return(TRUE);
	}
	ttyputmsg(_("Compiling VHDL in %s, writing netlist to %s"), fromintended, intended);
	efree(fromintended);

	/* write output */
	switch (vhdl_target)
	{
		case TARGET_ALS:     vhdl_genals(vhdl_lib, basecell);  break;
		case TARGET_NETLISP: vhdl_gennet(vhdl_target);          break;
		case TARGET_RSIM:    vhdl_gennet(vhdl_target);          break;
		case TARGET_SILOS:   vhdl_gensilos();                   break;
		case TARGET_QUISC:   vhdl_genquisc();                   break;
	}

	/* finish up */
	vhdl_endoutput();
	return(FALSE);
}

/*
Module:  vhdl_scanner
------------------------------------------------------------------------
Description:
	Lexical scanner of input file creating token list.
------------------------------------------------------------------------
Calling Sequence:  vhdl_scanner();
------------------------------------------------------------------------
*/
void vhdl_scanner(void)
{
	CHAR *c, buffer[MAXVHDLLINE], *end, tchar;
	INTBIG line_num, space;
	VKEYWORD *key;

	/* start by clearing previous scanner memory */
	vhdl_freescannermemory();

	c = x_("");
	for(;;)
	{
		if (*c == 0)
		{
			line_num = vhdl_getnextline(buffer);
			if (line_num == 0) return;
			space = TRUE;
			c = buffer;
		} else if (isspace(*c)) space = TRUE; else
			space = FALSE;
		while (isspace(*c)) c++;
		if (*c == 0) continue;
		if (isalpha(*c))
		{
			/* could be identifier (keyword) or bit string literal */
			if (*(c + 1) == '"')
			{
				if ((tchar = vhdl_toupper(*c)) == 'B')
				{
					/* EMPTY */ 
				} else if (tchar == '0')
				{
					/* EMPTY */ 
				} else if (tchar == 'X')
				{
					/* EMPTY */ 
				}
			}
			end = c + 1;
			while (isalnum(*end) || *end == '_') end++;

			/* got alphanumeric from c to end - 1 */
			if ((key = vhdl_iskeyword(c, (INTBIG)(end - c))) != NOVKEYWORD)
			{
				vhdl_makekeytoken(key, line_num, (INTBIG)space);
			} else
			{
				vhdl_makeidenttoken(c, (INTBIG)(end - c), line_num, (INTBIG)space);
			}
			c = end;
		} else if (isdigit(*c))
		{
			/* could be decimal or based literal */
			end = c + 1;
			while (isdigit(*end) || *end == '_') end++;

			/* got numeric from c to end - 1 */
			vhdl_makedecimaltoken(c, (INTBIG)(end - c), line_num, (INTBIG)space);
			c = end;
		} else
		{
			switch (*c)
			{
				case '"':
					/* got a start of a string */
					end = c + 1;
					while (*end != '\n')
					{
						if (*end == '"')
						{
							if (*(end + 1) == '"') end++; else
								break;
						}
						end++;
					}
					/* string from c + 1 to end - 1 */
					vhdl_makestrtoken(c + 1, (INTBIG)(end - c - 1), line_num, (INTBIG)space);
					if (*end == '"') end++;
					c = end;
					break;
				case '&':
					vhdl_maketoken((INTBIG)TOKEN_AMPERSAND, line_num, (INTBIG)space);
					c++;
					break;
				case '\'':
					/* character literal */
					if (isgraph(*(c + 1)) && *(c + 2) == '\'')
					{
						vhdl_makechartoken(*(c + 1), line_num, (INTBIG)space);
						c += 3;
					} else c++;
					break;
				case '(':
					vhdl_maketoken((INTBIG)TOKEN_LEFTBRACKET, line_num, (INTBIG)space);
					c++;
					break;
				case ')':
					vhdl_maketoken((INTBIG)TOKEN_RIGHTBRACKET, line_num, (INTBIG)space);
					c++;
					break;
				case '*':
					/* could be STAR or DOUBLESTAR */
					if (*(c + 1) == '*')
					{
						vhdl_maketoken((INTBIG)TOKEN_DOUBLESTAR, line_num, (INTBIG)space);
						c += 2;
					} else
					{
						vhdl_maketoken((INTBIG)TOKEN_STAR, line_num, (INTBIG)space);
						c++;
					}
					break;
				case '+':
					vhdl_maketoken((INTBIG)TOKEN_PLUS, line_num, (INTBIG)space);
					c++;
					break;
				case ',':
					vhdl_maketoken((INTBIG)TOKEN_COMMA, line_num, (INTBIG)space);
					c++;
					break;
				case '-':
					if (*(c + 1) == '-')
					{
						/* got a comment, throw away rest of line */
						c = buffer + estrlen(buffer);
					} else
					{
						/* got a minus sign */
						vhdl_maketoken((INTBIG)TOKEN_MINUS, line_num, (INTBIG)space);
						c++;
					}
					break;
				case '.':
					/* could be PERIOD or DOUBLEDOT */
					if (*(c + 1) == '.')
					{
						vhdl_maketoken((INTBIG)TOKEN_DOUBLEDOT, line_num, (INTBIG)space);
						c += 2;
					} else
					{
						vhdl_maketoken((INTBIG)TOKEN_PERIOD, line_num, (INTBIG)space);
						c++;
					}
					break;
				case '/':
					/* could be SLASH or NE */
					if (*(c + 1) == '=')
					{
						vhdl_maketoken((INTBIG)TOKEN_NE, line_num, (INTBIG)space);
						c += 2;
					} else
					{
						vhdl_maketoken((INTBIG)TOKEN_SLASH, line_num, (INTBIG)space);
						c++;
					}
					break;
				case ':':
					/* could be COLON or VARASSIGN */
					if (*(c + 1) == '=')
					{
						vhdl_maketoken((INTBIG)TOKEN_VARASSIGN, line_num, (INTBIG)space);
						c += 2;
					} else
					{
						vhdl_maketoken((INTBIG)TOKEN_COLON, line_num, (INTBIG)space);
						c++;
					}
					break;
				case ';':
					vhdl_maketoken((INTBIG)TOKEN_SEMICOLON, line_num, (INTBIG)space);
					c++;
					break;
				case '<':
					/* could be LT or LE or BOX */
					switch (*(c + 1))
					{
						case '=':
							vhdl_maketoken((INTBIG)TOKEN_LE, line_num, (INTBIG)space);
							c += 2;
							break;
						case '>':
							vhdl_maketoken((INTBIG)TOKEN_BOX, line_num, (INTBIG)space);
							c += 2;
							break;
						default:
							vhdl_maketoken((INTBIG)TOKEN_LT, line_num, (INTBIG)space);
							c++;
							break;
					}
					break;
				case '=':
					/* could be EQUAL or double delimiter ARROW */
					if (*(c + 1) == '>')
					{
						vhdl_maketoken((INTBIG)TOKEN_ARROW, line_num, (INTBIG)space);
						c += 2;
					} else
					{
						vhdl_maketoken((INTBIG)TOKEN_EQ, line_num, (INTBIG)space);
						c++;
					}
					break;
				case '>':
					/* could be GT or GE */
					if (*(c + 1) == '=')
					{
						vhdl_maketoken((INTBIG)TOKEN_GE, line_num, (INTBIG)space);
						c += 2;
					} else
					{
						vhdl_maketoken((INTBIG)TOKEN_GT, line_num, (INTBIG)space);
						c++;
					}
					break;
				case '|':
					vhdl_maketoken((INTBIG)TOKEN_VERTICALBAR, line_num, (INTBIG)space);
					c++;
					break;
				default:
	/*	AJ	vhdl_makestrtoken(TOKEN_UNKNOWN, c, 1, line_num, space); */
					vhdl_maketoken((INTBIG)TOKEN_UNKNOWN, line_num, (INTBIG)space);
					c++;
					break;
			}
		}
	}
}

/*
Module:  vhdl_makekeytoken
------------------------------------------------------------------------
Description:
	Add a token to the token list which has a key reference.
------------------------------------------------------------------------
Calling Sequence:  vhdl_makekeytoken(key, line_num, space);

Name		Type		Description
----		----		-----------
key			*VKEYWORD	Pointer to keyword in table.
line_num	INTBIG		Line number of occurence.
space		INTBIG		Previous space flag.
------------------------------------------------------------------------
*/
void vhdl_makekeytoken(VKEYWORD *key, INTBIG line_num, INTBIG space)
{
	TOKENLIST *newtoken;

	newtoken = (TOKENLIST *)emalloc((INTBIG)sizeof(TOKENLIST), vhdl_tool->cluster);
	newtoken->token = TOKEN_KEYWORD;
	newtoken->pointer = (CHAR *)key;
	newtoken->line_num = line_num;
	newtoken->space = TRUE;
	newtoken->next = NOTOKENLIST;
	newtoken->last = vhdl_tlistend;
	if (vhdl_tlistend == NOTOKENLIST)
	{
		vhdl_tliststart = vhdl_tlistend = newtoken;
	} else
	{
		vhdl_tlistend->space = space;
		vhdl_tlistend->next = newtoken;
		vhdl_tlistend = newtoken;
	}
}

/*
Module:  vhdl_makeidenttoken
------------------------------------------------------------------------
Description:
	Add a identity token to the token list which has a string reference.
------------------------------------------------------------------------
Calling Sequence:  vhdl_makeidenttoken(str, length, line_num, space);

Name		Type		Description
----		----		-----------
str			*char		Pointer to start of string (not 0 term).
length		INTBIG		Length of string.
line_num	INTBIG		Line number of occurence.
space		INTBIG		Previous space flag.
------------------------------------------------------------------------
*/
void vhdl_makeidenttoken(CHAR *str, INTBIG length, INTBIG line_num, INTBIG space)
{
	TOKENLIST *newtoken;
	CHAR *newstring;
	IDENTTABLE *ikey;

	newtoken = (TOKENLIST *)emalloc((INTBIG)sizeof(TOKENLIST), vhdl_tool->cluster);
	newtoken->token = TOKEN_IDENTIFIER;
	newstring = (CHAR *)emalloc((length + 1) * SIZEOFCHAR, vhdl_tool->cluster);
	estrncpy(newstring, str, length);
	newstring[length] = 0;

	/* check if ident exits in the global name space */
	if ((ikey = vhdl_findidentkey(newstring)) == (IDENTTABLE *)0)
	{
		ikey = vhdl_makeidentkey(newstring);
		if (ikey == 0) return;
	} else efree(newstring);
	newtoken->pointer = (CHAR *)ikey;
	newtoken->line_num = line_num;
	newtoken->space = TRUE;
	newtoken->next = NOTOKENLIST;
	newtoken->last = vhdl_tlistend;
	if (vhdl_tlistend == NOTOKENLIST)
	{
		vhdl_tliststart = vhdl_tlistend = newtoken;
	} else
	{
		vhdl_tlistend->space = space;
		vhdl_tlistend->next = newtoken;
		vhdl_tlistend = newtoken;
	}
}

/*
Module:  vhdl_makestrtoken
------------------------------------------------------------------------
Description:
	Add a string token to the token list.  Note that two adjacent double
	quotes should be mergered into one.
------------------------------------------------------------------------
Calling Sequence:  vhdl_makestrtoken(string, length, line_num, space);

Name		Type		Description
----		----		-----------
string		*char		Pointer to start of string (not 0 term).
length		INTBIG		Length of string.
line_num	INTBIG		Line number of occurence.
space		INTBIG		Previous space flag.
------------------------------------------------------------------------
*/
void vhdl_makestrtoken(CHAR *string, INTBIG length, INTBIG line_num, INTBIG space)
{
	TOKENLIST *newtoken;
	CHAR *newstring, *str, *str2;

	newtoken = (TOKENLIST *)emalloc((INTBIG)sizeof(TOKENLIST), vhdl_tool->cluster);
	newtoken->token = TOKEN_STRING;
	newstring = (CHAR *)emalloc((length + 1) * SIZEOFCHAR, vhdl_tool->cluster);
	estrncpy(newstring, string, length);
	newstring[length] = 0;

	/* merge two adjacent double quotes */
	str = newstring;
	while (*str)
	{
		if (*str == '"')
		{
			if (*(str + 1) == '"')
			{
				str2 = str + 1;
				do
				{
					*str2 = *(str2 + 1);
					str2++;
				} while (*str2 != 0);
			}
		}
		str++;
	}
	newtoken->pointer = newstring;
	newtoken->line_num = line_num;
	newtoken->space = TRUE;
	newtoken->next = NOTOKENLIST;
	newtoken->last = vhdl_tlistend;
	if (vhdl_tlistend == NOTOKENLIST)
	{
		vhdl_tliststart = vhdl_tlistend = newtoken;
	} else
	{
		vhdl_tlistend->space = space;
		vhdl_tlistend->next = newtoken;
		vhdl_tlistend = newtoken;
	}
}

/*
Module:  vhdl_makedecimaltoken
------------------------------------------------------------------------
Description:
	Add a numeric token to the token list which has a string reference.
------------------------------------------------------------------------
Calling Sequence:  vhdl_makedecimaltoken(string, length, line_num, space);

Name		Type		Description
----		----		-----------
string		*char		Pointer to start of string (not 0 term).
length		INTBIG		Length of string.
line_num	INTBIG		Line number of occurence.
space		INTBIG		Previous space flag.
------------------------------------------------------------------------
*/
void vhdl_makedecimaltoken(CHAR *string, INTBIG length, INTBIG line_num,
	INTBIG space)
{
	TOKENLIST *newtoken;
	CHAR *newstring;

	newtoken = (TOKENLIST *)emalloc((INTBIG)sizeof(TOKENLIST), vhdl_tool->cluster);
	newtoken->token = TOKEN_DECIMAL;
	newstring = (CHAR *)emalloc((length + 1) * SIZEOFCHAR, vhdl_tool->cluster);
	estrncpy(newstring, string, length);
	newstring[length] = 0;
	newtoken->pointer = newstring;
	newtoken->line_num = line_num;
	newtoken->space = TRUE;
	newtoken->next = NOTOKENLIST;
	newtoken->last = vhdl_tlistend;
	if (vhdl_tlistend == NOTOKENLIST)
	{
		vhdl_tliststart = vhdl_tlistend = newtoken;
	} else
	{
		vhdl_tlistend->space = space;
		vhdl_tlistend->next = newtoken;
		vhdl_tlistend = newtoken;
	}
}

/*
Module:  vhdl_maketoken
------------------------------------------------------------------------
Description:
	Add a token to the token list which has no string reference.
------------------------------------------------------------------------
Calling Sequence:  vhdl_maketoken(token, line_num, space);

Name		Type		Description
----		----		-----------
token		INTBIG		Token number.
line_num	INTBIG		Line number of occurence.
space		INTBIG		Previous space flag.
------------------------------------------------------------------------
*/
void vhdl_maketoken(INTBIG token, INTBIG line_num, INTBIG space)
{
	TOKENLIST *newtoken;

	newtoken = (TOKENLIST *)emalloc((INTBIG)sizeof(TOKENLIST), vhdl_tool->cluster);
	newtoken->token = token;
	newtoken->pointer = (CHAR *)0;
	newtoken->line_num = line_num;
	newtoken->space = TRUE;
	newtoken->next = NOTOKENLIST;
	newtoken->last = vhdl_tlistend;
	if (vhdl_tlistend == NOTOKENLIST)
	{
		vhdl_tliststart = vhdl_tlistend = newtoken;
	} else
	{
		vhdl_tlistend->space = space;
		vhdl_tlistend->next = newtoken;
		vhdl_tlistend = newtoken;
	}
}

/*
Module:  vhdl_makechartoken
------------------------------------------------------------------------
Description:
	Make a character literal token.
------------------------------------------------------------------------
Calling Sequence:  vhdl_makechartoken(c, line_num, space);

Name		Type		Description
----		----		-----------
c			char		Character literal.
line_num	INTBIG		Number of source line.
space		INTBIG		Previous space flag.
------------------------------------------------------------------------
*/
void vhdl_makechartoken(CHAR c, INTBIG line_num, INTBIG space)
{
	TOKENLIST *newtoken;

	newtoken = (TOKENLIST *)emalloc((INTBIG)sizeof(TOKENLIST), vhdl_tool->cluster);
	newtoken->token = TOKEN_CHAR;
	newtoken->pointer = (CHAR *)((INTBIG)c);
	newtoken->line_num = line_num;
	newtoken->space = TRUE;
	newtoken->next = NOTOKENLIST;
	newtoken->last = vhdl_tlistend;
	if (vhdl_tlistend == NOTOKENLIST)
	{
		vhdl_tliststart = vhdl_tlistend = newtoken;
	} else
	{
		vhdl_tlistend->space = space;
		vhdl_tlistend->next = newtoken;
		vhdl_tlistend = newtoken;
	}
}

void vhdl_freescannermemory(void)
{
	TOKENLIST *tok, *nexttok;
	REGISTER INTBIG i;

	/* free parsed tokens */
	for(tok = vhdl_tliststart; tok != NOTOKENLIST; tok = nexttok)
	{
		nexttok = tok->next;
		if (tok->token == TOKEN_STRING || tok->token == TOKEN_DECIMAL)
			efree(tok->pointer);
		efree((CHAR *)tok);
	}
	vhdl_tliststart = vhdl_tlistend = NOTOKENLIST;

	/* clear identifier table */
	if (vhdl_identtable != 0)
	{
		for (i = 0; i < IDENT_TABLE_SIZE; i++)
			if (vhdl_identtable[i].string != 0)
		{
			efree(vhdl_identtable[i].string);
			vhdl_identtable[i].string = 0;
		}
	}
}

/*
Module:  vhdl_iskeyword
------------------------------------------------------------------------
Description:
	If the passed string is a keyword, return its address in the
	keyword table, else return NOVKEYWORD.
------------------------------------------------------------------------
Calling Sequence:  key = vhdl_iskeyword(string, length);

Name		Type		Description
----		----		-----------
string		*char		Pointer to start of string (not 0 term).
length		INTBIG		Length of string.
key			*VKEYWORD	Pointer to entry in keywords table if
								keyword, else NOVKEYWORD.
------------------------------------------------------------------------
*/
VKEYWORD *vhdl_iskeyword(CHAR *string, INTBIG length)
{
	INTBIG aindex, num, check, base;
	CHAR tstring[MAXVHDLLINE];

	if (length >= MAXVHDLLINE) return(NOVKEYWORD);
	for (aindex = 0; aindex < length; aindex++)
		tstring[aindex] = string[aindex];
	tstring[length] = 0;
	base = 0;
	num = sizeof(vhdl_keywords) / sizeof(VKEYWORD);
	aindex = num >> 1;
	while (num)
	{
		check = namesame(tstring, vhdl_keywords[base + aindex].name);
		if (check == 0) return(&vhdl_keywords[base + aindex]);
		if (check < 0)
		{
			num = aindex;
			aindex = num >> 1;
		} else
		{
			base += aindex + 1;
			num -= aindex + 1;
			aindex = num >> 1;
		}
	}
	return(NOVKEYWORD);
}

/*
Module:  vhdl_findidentkey
------------------------------------------------------------------------
Description:
	Search for the passed string in the global name space.  Return
	a pointer to the ident table entry if found or 0 if not
	found.
------------------------------------------------------------------------
Calling Sequence:  ikey = vhdl_findidentkey(string);

Name		Type		Description
----		----		-----------
string		*char		Pointer to string of name.
ikey		*IDENTTABLE	Pointer to entry in table if found,
								or 0 if not found.
------------------------------------------------------------------------
*/
IDENTTABLE *vhdl_findidentkey(CHAR *string)
{
	INTBIG attempt;
	CHAR *ident;

	attempt = vhdl_identfirsthash(string);
	for(;;)
	{
		ident = vhdl_identtable[attempt].string;
		if (ident == 0) break;
		if (namesame(string, ident) == 0) return(&(vhdl_identtable[attempt]));
		attempt = vhdl_identsecondhash(string, attempt);
	}
	return((IDENTTABLE *)0);
}

/*
Module:  vhdl_makeidentkey
------------------------------------------------------------------------
Description:
	Make an entry for the passed string in the global name space.
	Return a pointer to the ident table entry created.
	Returns zero on error.
------------------------------------------------------------------------
Calling Sequence:  ikey = vhdl_makeidentkey(string);

Name		Type		Description
----		----		-----------
string		*char		Pointer to string of name.
ikey		*IDENTTABLE	Pointer to entry in table created.
------------------------------------------------------------------------
*/
IDENTTABLE *vhdl_makeidentkey(CHAR *string)
{
	INTBIG attempt, count;

	count = 0;
	attempt = vhdl_identfirsthash(string);
	while (vhdl_identtable[attempt].string)
	{
		if (++count >= MAX_HASH_TRYS)
		{
			ttyputmsg(_("ERROR hashing identifier - tried %ld times"), count);
			return(0);
		}
		attempt = vhdl_identsecondhash(string, attempt);
	}
	vhdl_identtable[attempt].string = string;
	return(&vhdl_identtable[attempt]);
}

/*
Module:  vhdl_identfirsthash
------------------------------------------------------------------------
Description:
	Return the hash value for the passed string.  The first hash
	function is:

		value = ((firstchar << 7) + (lastchar << 4) + length) mod
				IDENT_TABLE_SIZE;

	Note:  lowercase letters are converted to uppercase.
------------------------------------------------------------------------
Calling Sequence:  value = vhdl_identfirsthash(string);

Name		Type		Description
----		----		-----------
string		*char		Pointer to string to hash.
value		INTBIG		Returned hash value.
------------------------------------------------------------------------
*/
INTBIG vhdl_identfirsthash(CHAR *string)
{
	REGISTER INTBIG length, value;

	length = (INTBIG)estrlen(string);
	value = (vhdl_toupper(*string) & 0xFF) << 7;
	value += (vhdl_toupper(string[length - 1]) & 0xFF) << 4;
	value = (value + length) % IDENT_TABLE_SIZE;
	return(value);
}

/*
Module:  vhdl_identsecondhash
------------------------------------------------------------------------
Description:
	Return the hash value for the passed string.  The second hash
	function is:

		value = (previous_hash +  ((summation of characters) << 4) +
				(length of string)) mod IDENT_TABLE_SIZE;
------------------------------------------------------------------------
Calling Sequence:  value = vhdl_identsecondhash(string, first_hash);

Name			Type		Description
----			----		-----------
string			*char		Pointer to string to hash.
previous_hash	INTBIG		Value of previous hash.
value			INTBIG		Returned hash value.
------------------------------------------------------------------------
*/
INTBIG vhdl_identsecondhash(CHAR *string, INTBIG previous_hash)
{
	INTBIG length, value;
	CHAR *sptr;

	value = 0;
	for (sptr = string; *sptr; sptr++)
		value += (vhdl_toupper(*sptr) & 0xFF);
	value <<= 4;
	length = estrlen(string);
	value = (previous_hash + value + length) % IDENT_TABLE_SIZE;
	return(value);
}

/*
Module:  vhdl_toupper
------------------------------------------------------------------------
Description:
	Convert a character to uppercase if it is a lowercase letter only.
------------------------------------------------------------------------
Calling Sequence:  c = vhdl_toupper(c);

Name		Type		Description
----		----		-----------
c			char		Character to be converted.
------------------------------------------------------------------------
*/
CHAR vhdl_toupper(CHAR c)
{
	if (islower(c))
		c = toupper(c);
	return(c);
}

/*
Module:  vhdl_findtopinterface
------------------------------------------------------------------------
Description:
	Find the top interface in the database.  The top interface is defined
	as the interface is called by no other architectural bodies.
------------------------------------------------------------------------
Calling Sequence:  top_interface = vhdl_findtopinterface(units);

Name			Type		Description
----			----		-----------
units			*DBUNITS	Pointer to database design units.
top_interface	*DBINTERFACE	Pointer to top interface.
------------------------------------------------------------------------
*/
DBINTERFACE *vhdl_findtopinterface(DBUNITS *units)
{
	DBINTERFACE *interfacef;
	DBBODY *body;
	DBCOMPONENTS *compo;
	SYMBOLTREE *symbol;

	/* clear flags of all interfaces in database */
	for (interfacef = units->interfaces; interfacef != NULL; interfacef = interfacef->next)
		interfacef->flags &= ~TOP_ENTITY_FLAG;

	/* go through the list of bodies and flag any interfaces */
	for (body = units->bodies; body != NULL; body = body->next)
	{
		/* go through component list */
		if (body->declare == 0) continue;
		for (compo = body->declare->components; compo != NULL; compo = compo->next)
		{
			symbol = vhdl_searchsymbol(compo->name, vhdl_gsymbols);
			if (symbol != NULL && symbol->pointer != NULL)
			{
				((DBINTERFACE *)(symbol->pointer))->flags |= TOP_ENTITY_FLAG;
			}
		}
	}

	/* find interface with the flag bit not set */
	for (interfacef = units->interfaces; interfacef != NULL; interfacef = interfacef->next)
	{
		if (!(interfacef->flags & TOP_ENTITY_FLAG)) break;
	}
	return(interfacef);
}

void vhdl_freeunresolvedlist(UNRESLIST **lstart)
{
	UNRESLIST *unres;

	while (*lstart != 0)
	{
		unres = *lstart;
		*lstart = (*lstart)->next;
		efree((CHAR *)unres);
	}
}

/*
Module:  vhdl_unresolved
------------------------------------------------------------------------
Description:
	Maintain a list of unresolved interfaces for later reporting.
------------------------------------------------------------------------
Calling Sequence:  vhdl_unresolved(iname, lstart);

Name		Type		Description
----		----		-----------
iname		*IDENTTABLE	Pointer to global name space.
lstart		**UNRESLIST	Address of start of list pointer;
------------------------------------------------------------------------
*/
void vhdl_unresolved(IDENTTABLE *iname, UNRESLIST **lstart)
{
	UNRESLIST *ulist;

	for (ulist = *lstart; ulist != NULL; ulist = ulist->next)
	{
		if (ulist->interfacef == iname) break;
	}
	if (ulist) ulist->numref++; else
	{
		ulist = (UNRESLIST *)emalloc((INTBIG)sizeof(UNRESLIST), vhdl_tool->cluster);
		ulist->interfacef = iname;
		ulist->numref = 1;
		ulist->next = *lstart;
		*lstart = ulist;
	}
}

/****************************** VHDL OPTIONS DIALOG ******************************/

/* VHDL Options */
static DIALOGITEM vhdl_optionsdialogitems[] =
{
 /*  1 */ {0, {336,244,360,308}, BUTTON, N_("OK")},
 /*  2 */ {0, {300,244,324,308}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {308,4,324,166}, CHECK, N_("VHDL stored in cell")},
 /*  4 */ {0, {332,4,348,179}, CHECK, N_("Netlist stored in cell")},
 /*  5 */ {0, {28,8,224,319}, SCROLL, x_("")},
 /*  6 */ {0, {236,4,252,203}, MESSAGE, N_("VHDL for primitive:")},
 /*  7 */ {0, {260,4,276,203}, MESSAGE, N_("VHDL for negated primitive:")},
 /*  8 */ {0, {236,208,252,319}, EDITTEXT, x_("")},
 /*  9 */ {0, {260,208,276,319}, EDITTEXT, x_("")},
 /* 10 */ {0, {8,8,24,183}, MESSAGE, N_("Schematics primitives:")},
 /* 11 */ {0, {292,8,293,319}, DIVIDELINE, x_("")}
};
static DIALOG vhdl_optionsdialog = {{50,75,419,403}, N_("VHDL Options"), 0, 11, vhdl_optionsdialogitems, 0, 0};

/* special items for the "VHDL Options" dialog: */
#define DVHO_VHDLINCELL    3		/* VHDL stored in cell (check) */
#define DVHO_NETINCELL     4		/* Netlist stored in cell (check) */
#define DVHO_PRIMLIST      5		/* List of schematics prims (scroll) */
#define DVHO_UNNEGATEDNAME 8		/* Unnegated name (edit text) */
#define DVHO_NEGATEDNAME   9		/* Negated name (edit text) */

void vhdl_optionsdlog(void)
{
	INTBIG itemHit;
	REGISTER CHAR **varnames, *pt, *opt;
	REGISTER VARIABLE *var;
	REGISTER TECHNOLOGY *tech;
	REGISTER NODEPROTO *np, **nplist;
	REGISTER INTBIG i, len, technameschanged;
	BOOLEAN vhdlondisk, netlistondisk, ondisk;
	REGISTER void *infstr, *dia;

	dia = DiaInitDialog(&vhdl_optionsdialog);
	if (dia == 0) return;
	var = getval((INTBIG)vhdl_tool, VTOOL, VINTEGER, x_("VHDL_vhdl_on_disk"));
	vhdlondisk = FALSE;
	if (var != NOVARIABLE && var->addr != 0) vhdlondisk = TRUE;
	var = getval((INTBIG)vhdl_tool, VTOOL, VINTEGER, x_("VHDL_netlist_on_disk"));
	netlistondisk = FALSE;
	if (var != NOVARIABLE && var->addr != 0) netlistondisk = TRUE;

	/* setup for generated names in the technology */
	tech = sch_tech;
	DiaInitTextDialog(dia, DVHO_PRIMLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0,
		SCSELMOUSE|SCREPORT);
	var = getval((INTBIG)tech, VTECHNOLOGY, VSTRING|VISARRAY, x_("TECH_vhdl_names"));
	if (var == NOVARIABLE)
	{
		for(len=0, np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto, len++) ;
		varnames = (CHAR **)emalloc(len * (sizeof (CHAR *)), el_tempcluster);
		for(i=0, np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto, i++)
		{
			varnames[i] = (CHAR *)emalloc(1 * SIZEOFCHAR, el_tempcluster);
			varnames[i][0] = 0;
		}
	} else
	{
		len = getlength(var);
		varnames = (CHAR **)emalloc(len * (sizeof (CHAR *)), el_tempcluster);
		for(i=0; i<len; i++)
		{
			pt = ((CHAR **)var->addr)[i];
			allocstring(&varnames[i], pt, el_tempcluster);
		}
	}
	nplist = (NODEPROTO **)emalloc(i * (sizeof (NODEPROTO *)), el_tempcluster);
	for(i=0, np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto, i++)
		nplist[i] = np;
	for(i=0; i<len; i++)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, nplist[i]->protoname);
		if (*varnames[i] != 0)
		{
			addstringtoinfstr(infstr, x_(" ("));
			addstringtoinfstr(infstr, varnames[i]);
			addstringtoinfstr(infstr, x_(")"));
		}
		DiaStuffLine(dia, DVHO_PRIMLIST, returninfstr(infstr));
	}
	DiaSelectLine(dia, DVHO_PRIMLIST, 0);

	if (!vhdlondisk) DiaSetControl(dia, DVHO_VHDLINCELL, 1);
	if (!netlistondisk) DiaSetControl(dia, DVHO_NETINCELL, 1);

	/* loop until done */
	technameschanged = 0;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DVHO_VHDLINCELL || itemHit == DVHO_NETINCELL)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
		if (itemHit == DVHO_PRIMLIST)
		{
			DiaSetText(dia, DVHO_UNNEGATEDNAME, x_(""));
			DiaSetText(dia, DVHO_NEGATEDNAME, x_(""));
			i = DiaGetCurLine(dia, DVHO_PRIMLIST);
			pt = varnames[i];
			for(opt = pt; *opt != 0; opt++) if (*opt == '/') break;
			if (*opt == '/')
			{
				*opt = 0;
				DiaSetText(dia, DVHO_UNNEGATEDNAME, pt);
				DiaSetText(dia, DVHO_NEGATEDNAME, &opt[1]);
				*opt = '/';
			} else
			{
				DiaSetText(dia, DVHO_UNNEGATEDNAME, pt);
				DiaSetText(dia, DVHO_NEGATEDNAME, pt);
			}
			continue;
		}
		if (itemHit == DVHO_UNNEGATEDNAME || itemHit == DVHO_NEGATEDNAME)
		{
			i = DiaGetCurLine(dia, DVHO_PRIMLIST);
			infstr = initinfstr();
			addstringtoinfstr(infstr, DiaGetText(dia, DVHO_UNNEGATEDNAME));
			addtoinfstr(infstr, '/');
			addstringtoinfstr(infstr, DiaGetText(dia, DVHO_NEGATEDNAME));
			pt = returninfstr(infstr);
			if (estrcmp(varnames[i], pt) == 0) continue;

			technameschanged++;
			(void)reallocstring(&varnames[i], pt, el_tempcluster);

			infstr = initinfstr();
			addstringtoinfstr(infstr, nplist[i]->protoname);
			if (*varnames[i] != 0)
			{
				addstringtoinfstr(infstr, x_(" ("));
				addstringtoinfstr(infstr, varnames[i]);
				addstringtoinfstr(infstr, x_(")"));
			}
			DiaSetScrollLine(dia, DVHO_PRIMLIST, i, returninfstr(infstr));
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		if (DiaGetControl(dia, DVHO_VHDLINCELL) != 0) ondisk = FALSE; else ondisk = TRUE;
		if (ondisk != vhdlondisk)
			setval((INTBIG)vhdl_tool, VTOOL, x_("VHDL_vhdl_on_disk"), ondisk, VINTEGER);
		if (DiaGetControl(dia, DVHO_NETINCELL) != 0) ondisk = FALSE; else ondisk = TRUE;
		if (ondisk != netlistondisk)
			setval((INTBIG)vhdl_tool, VTOOL, x_("VHDL_netlist_on_disk"), ondisk, VINTEGER);
		if (technameschanged != 0)
			setval((INTBIG)tech, VTECHNOLOGY, x_("TECH_vhdl_names"), (INTBIG)varnames,
				VSTRING|VISARRAY|(len<<VLENGTHSH));
	}
	DiaDoneDialog(dia);
	for(i=0; i<len; i++) efree(varnames[i]);
	efree((CHAR *)varnames);
	efree((CHAR *)nplist);
}

#endif  /* VHDLTOOL - at top */
