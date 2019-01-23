/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: contable.c
 * Constraint system tables
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
#include "conlay.h"
#include "conlin.h"

/* the null constraint solver */
static void db_nullconinit(CONSTRAINT*);
static void db_nullconterm(void);
static void db_nullconsetmode(INTBIG, CHAR*[]);
static INTBIG db_nullconrequest(CHAR*, INTBIG);
static void db_nullconsolve(NODEPROTO*);
static void db_nullconnewobject(INTBIG, INTBIG);
static void db_nullconkillobject(INTBIG, INTBIG);
static BOOLEAN db_nullconsetobject(INTBIG, INTBIG, INTBIG, INTBIG);
static void db_nullconmodifynodeinst(NODEINST*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static void db_nullconmodifynodeinsts(INTBIG, NODEINST**, INTBIG*, INTBIG*, INTBIG*, INTBIG*, INTBIG*, INTBIG*);
static void db_nullconmodifyarcinst(ARCINST*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static void db_nullconmodifyportproto(PORTPROTO*, NODEINST*, PORTPROTO*);
static void db_nullconmodifynodeproto(NODEPROTO*);
static void db_nullconmodifydescript(INTBIG, INTBIG, INTBIG, UINTBIG*);
static void db_nullconnewlib(LIBRARY*);
static void db_nullconkilllib(LIBRARY*);
static void db_nullconnewvariable(INTBIG, INTBIG, INTBIG, INTBIG);
static void db_nullconkillvariable(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, UINTBIG*);
static void db_nullconmodifyvariable(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static void db_nullconinsertvariable(INTBIG, INTBIG, INTBIG, INTBIG);
static void db_nullcondeletevariable(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static void db_nullconsetvariable(void);

CONSTRAINT el_constraints[] =
{
	{x_("null"), N_("Null constraints"), NOCOMCOMP, NOCLUSTER, NOVARIABLE, 0,
	db_nullconinit, db_nullconterm,						/* control */
	db_nullconsetmode, db_nullconrequest,				/* action */
	db_nullconsolve,									/* solution */
	db_nullconnewobject, db_nullconkillobject,			/* objects */
	db_nullconsetobject,
	db_nullconmodifynodeinst, db_nullconmodifynodeinsts,/* modification */
	db_nullconmodifyarcinst,
	db_nullconmodifyportproto, db_nullconmodifynodeproto,
	db_nullconmodifydescript,
	db_nullconnewlib, db_nullconkilllib,				/* libraries */
	db_nullconnewvariable, db_nullconkillvariable,		/* variables */
	db_nullconmodifyvariable, db_nullconinsertvariable,
	db_nullcondeletevariable, db_nullconsetvariable},

	{x_("layout"), N_("Hierarchical layout constraints"), &cla_layconp, NOCLUSTER,
		NOVARIABLE, 0,
	cla_layconinit, cla_layconterm,						/* control */
	cla_layconsetmode, cla_layconrequest,				/* action */
	cla_layconsolve,									/* solution */
	cla_layconnewobject, cla_layconkillobject,			/* objects */
	cla_layconsetobject,
	cla_layconmodifynodeinst, cla_layconmodifynodeinsts,/* modification */
	cla_layconmodifyarcinst,
	cla_layconmodifyportproto, cla_layconmodifynodeproto,
	cla_layconmodifydescript,
	cla_layconnewlib, cla_layconkilllib,				/* libraries */
	cla_layconnewvariable, cla_layconkillvariable,		/* variables */
	cla_layconmodifyvariable, cla_layconinsertvariable,
	cla_laycondeletevariable, cla_layconsetvariable},

	{x_("linear"), N_("Linear constraints"), &cli_linconp, NOCLUSTER, NOVARIABLE, 0,
	cli_linconinit, cli_linconterm,						/* control */
	cli_linconsetmode, cli_linconrequest,				/* action */
	cli_linconsolve,									/* solution */
	cli_linconnewobject, cli_linconkillobject,			/* objects */
	cli_linconsetobject,
	cli_linconmodifynodeinst, cli_linconmodifynodeinsts,/* modification */
	cli_linconmodifyarcinst,
	cli_linconmodifyportproto, cli_linconmodifynodeproto,
	cli_linconmodifydescript,
	cli_linconnewlib, cli_linconkilllib,				/* libraries */
	cli_linconnewvariable, cli_linconkillvariable,		/* variables */
	cli_linconmodifyvariable, cli_linconinsertvariable,
	cli_lincondeletevariable, cli_linconsetvariable},

	{NULL, NULL, NULL, 0, 0, 0,
	NULL, NULL,											/* control */
	NULL, NULL,											/* action */
	NULL,												/* solution */
	NULL, NULL,											/* objects */
	NULL,
	NULL, NULL,											/* modification */
	NULL,
	NULL, NULL,
	NULL,
	NULL, NULL,											/* libraries */
	NULL, NULL,											/* variables */
	NULL, NULL,
	NULL, NULL}  /* 0 */
};

/* the null constraint solver */
void db_nullconinit(CONSTRAINT *con) {}
void db_nullconterm(void) {}
void db_nullconsetmode(INTBIG count, CHAR *par[]) {}
INTBIG db_nullconrequest(CHAR *command, INTBIG arg1) { return(0); }
void db_nullconsolve(NODEPROTO *np) {}
void db_nullconnewobject(INTBIG addr, INTBIG type) {}
void db_nullconkillobject(INTBIG addr, INTBIG type) {}
BOOLEAN db_nullconsetobject(INTBIG addr, INTBIG type, INTBIG ctype, INTBIG cdata) { return(FALSE); }
void db_nullconmodifynodeinst(NODEINST *ni, INTBIG dlx, INTBIG dly, INTBIG dhx, INTBIG dhy, INTBIG drot, INTBIG dtrans) {}
void db_nullconmodifynodeinsts(INTBIG count, NODEINST **ni, INTBIG *dlx, INTBIG *dly, INTBIG *dhx, INTBIG *dhy, INTBIG *drot, INTBIG *dtrans) {}
void db_nullconmodifyarcinst(ARCINST *ai, INTBIG oldx0, INTBIG oldy0, INTBIG oldx1, INTBIG oldy1, INTBIG oldwid, INTBIG oldlen) {}
void db_nullconmodifyportproto(PORTPROTO *pp, NODEINST *oni, PORTPROTO *opp) {}
void db_nullconmodifynodeproto(NODEPROTO *np) {}
void db_nullconmodifydescript(INTBIG addr, INTBIG type, INTBIG key, UINTBIG *olddes) {}
void db_nullconnewlib(LIBRARY *lib) {}
void db_nullconkilllib(LIBRARY *lib) {}
void db_nullconnewvariable(INTBIG addr, INTBIG type, INTBIG skey, INTBIG stype) {}
void db_nullconkillvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG saddr, INTBIG stype, UINTBIG *olddes) {}
void db_nullconmodifyvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG stype, INTBIG aindex, INTBIG oldval) {}
void db_nullconinsertvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG aindex) {}
void db_nullcondeletevariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG aindex, INTBIG oldval) {}
void db_nullconsetvariable(void) {}
