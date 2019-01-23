/*
 * Electric(tm) VLSI Design System
 *
 * File: logeffort.cpp
 * Logical effort timing and sizing tool
 * Written by: Steven M. Rubin
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
 *
 * This module is inspired by the book "Logical Effort"
 * by Ivan Sutherland, Bob Sproull, and David Harris
 * Morgan Kaufmann, San Francisco, 1999.
 */

#include "config.h"
#if LOGEFFTOOL

#include "global.h"
#include "efunction.h"
#include "egraphics.h"
#include "edialogs.h"
#include "logeffort.h"
#include "network.h"
#include "usr.h"
#include <math.h>

/* the LOGICAL EFFORT tool table */
static KEYWORD leopt[] =
{
	{x_("analyze-path"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("analyze-cell"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("analyze-network"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("show-loads"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("estimate-delay"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("set-options"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("set-capacitance"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("set-node-effort"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
extern "C" {
COMCOMP le_tablep = {leopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("Logical effort action"), M_("show defaults")};
}

#define MAXITERATIONS  10
#define MAXSTAGEEFFORT  3.0

/* the meaning of LENODE->type */
#define LEUNKNOWN    0
#define LETERMINAL   1
#define LEINVERTER   2
#define LENAND       3
#define LENOR        4
#define LEXOR        5
#define LEXNOR       6
#define LENMOS       7
#define LEPMOS       8
#define LEPOWER      9
#define LEGROUND    10

#define NOLENODE ((LENODE *)-1)

typedef struct Ilenode
{
	NODEINST        *ni;				/* node in database */
	PORTARCINST     *piin, *piout;		/* input and output ports on that NODEINST */
	double           cin, cout;			/* input and output capacitances */
	double           logeffort;			/* logical effort of the node (g) */
	double           parasitics;		/* parasitics on the node (p) */
	double           branching;			/* branching effort of the node (b) */
	INTBIG           type;				/* node type (see above) */
	INTBIG           inputs;			/* number of inputs */
	struct Ilenode  *nextlenode;		/* next in linked list */
	struct Ilenode  *prevlenode;		/* previous in linked list */

	struct Ilenode **inputnodes;		/* array of input nodes */
	PORTARCINST    **inputports;		/* array of input ports on nodes */
	struct Ilenode **outputnodes;		/* array of output nodes */
	PORTARCINST    **outputports;		/* array of output ports on nodes */
	INTBIG           numinputnodes;		/* size of array of input nodes */
	INTBIG           numoutputnodes;	/* size of array of output nodes */
} LENODE;

       TOOL     *le_tool;					/* this tool */
static INTBIG    le_attrcapacitance_key;	/* variable key for "ATTR_Capacitance" */
static INTBIG    le_nodeeffort_key;			/* variable key for "LE_node_effort" */
static INTBIG    le_fanout_key;				/* variable key for "LE_fanout" */
static INTBIG    le_state_key;				/* variable key for "LE_state" */
       INTBIG    le_wire_ratio_key;			/* variable key for "LE_wire_ratio" */
static INTBIG    le_maximumstageeffort_key;	/* variable key for "LE_maximum_stage_effort" */
static LENODE   *le_lastlenode;				/* for propagating capacitance values */
static double    le_lastcapacitance;		/* for propagating capacitance values */
static LENODE   *le_firstlenode;			/* first in list of LE nodes in the path */
static LENODE   *le_lenodefree;				/* list of free LE nodes */

/* prototypes for local routines */
static void      le_addlinkage(LENODE *out, PORTARCINST *outpi, LENODE *in, PORTARCINST *inpi);
static LENODE   *le_alloclenode(void);
static void      le_analyzecell(void);
static void      le_analyzenetwork(void);
static void      le_analyzepath(void);
static CHAR     *le_describenode(LENODE *le);
static void      le_estimatedelay(NODEPROTO *cell);
static void      le_figurebranching(void);
static void      le_freealllenodes(void);
static void      le_freelenode(LENODE *le);
static void      le_gathercell(INTBIG show);
static void      le_gatherpath(INTBIG show);
static double    le_getcapacitance(NETWORK *net);
static INTBIG    le_getgatetype(NODEINST *ni, INTBIG *inputs);
static double    le_getlogeffort(LENODE *le);
static double    le_getparasitics(LENODE *le);
static CHAR     *le_nextarcs(void);
static NODEINST *le_propagate(NODEPROTO *cell, INTBIG edge);
static void      le_propagatebranch(PORTARCINST *tpi, LENODE *final, double capacitance,
					double *onpath, double *offpath);
static void      le_setarccapacitance(ARCINST *ai, double c);
static void      le_setlogicaleffort(void);
static void      le_setoptions(void);
static void      le_showloads(void);
static int       le_sortbyproductsize(const void *e1, const void *e2);
static BOOLEAN   le_topofarcs(CHAR **c);
static void      le_unwind(NODEINST *start, NODEINST *prev);

/************************ CONTROL ***********************/

/*
 * tool initialization
 */
void le_init(INTBIG *argc, CHAR1 *argv[], TOOL *thistool)
{
	/* ignore pass 3 initialization */
	if (thistool == 0) return;

	/* ignore pass 2 initialization */
	if (thistool == NOTOOL)
	{
		le_attrcapacitance_key = makekey(x_("ATTR_Capacitance"));
		le_nodeeffort_key = makekey(x_("LE_node_effort"));
		le_fanout_key = makekey(x_("LE_fanout"));
		le_state_key = makekey(x_("LE_state"));
		le_wire_ratio_key = makekey(x_("LE_wire_ratio"));
		le_maximumstageeffort_key = makekey(x_("LE_maximum_stage_effort"));
		le_firstlenode = NOLENODE;
		le_lenodefree = NOLENODE;
		nextchangequiet();
		setvalkey((INTBIG)le_tool, VTOOL, le_state_key, DEFAULTSTATE, VINTEGER|VDONTSAVE);
		return;
	}

	/* copy tool pointer during pass 1 */
	le_tool = thistool;
}

void le_done(void)
{
#ifdef DEBUGMEMORY
	LENODE *le;

	le_freealllenodes();

	while (le_lenodefree != NOLENODE)
	{
		le = le_lenodefree;
		le_lenodefree = le_lenodefree->nextlenode;
		efree((CHAR *)le);
	}
#endif
}

/*
 * Handle commands to the tool from the user.
 */
void le_set(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG l;
	REGISTER VARIABLE *var;
	REGISTER NODEPROTO *np;
	REGISTER INTBIG state;
	REGISTER CHAR *pp;

	if (count == 0)
	{
		ttyputusage(x_("telltool logeffort (analyze-path | analyze-cell | analyze-network | show-loads | set-options | set-capacitance | set-node-effort)"));
		return;
	}

	l = estrlen(pp = par[0]);
	if (namesamen(pp, x_("analyze-cell"), l) == 0 && l >= 9)
	{
		var = getvalkey((INTBIG)le_tool, VTOOL, VINTEGER, le_state_key);
		if (var == NOVARIABLE) state = DEFAULTSTATE; else
			state = var->addr;
		le_gathercell(state & HIGHLIGHTCOMPONENTS);
		if (le_firstlenode == NOLENODE) return;
		le_analyzecell();
		return;
	}
	if (namesamen(pp, x_("analyze-path"), l) == 0 && l >= 9)
	{
		var = getvalkey((INTBIG)le_tool, VTOOL, VINTEGER, le_state_key);
		if (var == NOVARIABLE) state = DEFAULTSTATE; else
			state = var->addr;
		le_gatherpath(state & HIGHLIGHTCOMPONENTS);
		if (le_firstlenode == NOLENODE) return;
		le_analyzepath();
		return;
	}
	if (namesamen(pp, x_("estimate-delay"), l) == 0)
	{
		/* analyze cell */
		np = getcurcell();
		if (np == NONODEPROTO) return;
		le_estimatedelay(np);
		return;
	}
	if (namesamen(pp, x_("show-loads"), l) == 0 && l >= 2)
	{
		le_showloads();
		return;
	}
	if (namesamen(pp, x_("analyze-network"), l) == 0 && l >= 2)
	{
		le_analyzenetwork();
		return;
	}
	if (namesamen(pp, x_("set-options"), l) == 0 && l >= 5)
	{
		le_setoptions();
		return;
	}
	if (namesamen(pp, x_("set-node-effort"), l) == 0 && l >= 5)
	{
		le_setlogicaleffort();
		return;
	}

	ttyputbadusage(x_("telltool logeffort"));
}

/* Logical Effort Options dialog */
static DIALOGITEM le_leoptionsdialogitems[] =
{
 /*  1 */ {0, {196,204,220,268}, BUTTON, N_("OK")},
 /*  2 */ {0, {132,204,156,268}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,160}, MESSAGE, N_("Maximum Stage Gain")},
 /*  4 */ {0, {8,168,24,220}, EDITTEXT, x_("")},
 /*  5 */ {0, {36,8,52,244}, CHECK, N_("Display intermediate capacitances")},
 /*  6 */ {0, {64,8,80,192}, CHECK, N_("Highlight components")},
 /*  7 */ {0, {112,8,220,180}, SCROLL, x_("")},
 /*  8 */ {0, {224,8,240,92}, MESSAGE, N_("Wire ratio:")},
 /*  9 */ {0, {224,96,240,180}, EDITTEXT, x_("")},
 /* 10 */ {0, {92,8,108,180}, MESSAGE, N_("Wire ratio for each layer:")}
};
static DIALOG le_leoptionsdialog = {{75,75,324,352}, N_("Logical Effort Options"), 0, 10, le_leoptionsdialogitems, 0, 0};

/* special items for the "Logical Effort Options" dialog: */
#define DLEO_MAXGAIN    4		/* Maximum Stage Gain (edit text) */
#define DLEO_INTERCAP   5		/* Show intermediate capacitances (check) */
#define DLEO_HIGHCOMP   6		/* Highlight components (check) */
#define DLEO_ARCLIST    7		/* List of arcs (scroll) */
#define DLEO_WIRERATIO  9		/* Arc wire ratio (edit text) */

static ARCPROTO *le_posarcs;
BOOLEAN le_topofarcs(CHAR **c)
{
	le_posarcs = el_curtech->firstarcproto;
	return(TRUE);
}

CHAR *le_nextarcs(void)
{
	REGISTER ARCPROTO *ap;
	REGISTER void *infstr;

	ap = le_posarcs;
	if (ap != NOARCPROTO)
	{
		le_posarcs = ap->nextarcproto;
		infstr = initinfstr();
		formatinfstr(infstr, x_("%s (%ld)"), describearcproto(ap), ap->temp1);
		return(returninfstr(infstr));
	}
	return(0);
}

/*
 * Routine to interactively set the logical effort options.
 */
void le_setoptions(void)
{
	INTBIG itemHit;
	INTBIG state, newstate, i, lineno;
	float maxstageeffort, newmaxstageeffort;
	REGISTER VARIABLE *var;
	REGISTER ARCPROTO *ap;
	CHAR line[200], *pt;
	REGISTER void *infstr, *dia;

	dia = DiaInitDialog(&le_leoptionsdialog);
	if (dia == 0) return;
	var = getvalkey((INTBIG)le_tool, VTOOL, VINTEGER, le_state_key);
	if (var == NOVARIABLE) state = DEFAULTSTATE; else
		state = var->addr;
	if ((state&DISPLAYCAPACITANCE) != 0) DiaSetControl(dia, DLEO_INTERCAP, 1);
	if ((state&HIGHLIGHTCOMPONENTS) != 0) DiaSetControl(dia, DLEO_HIGHCOMP, 1);
	var = getvalkey((INTBIG)le_tool, VTOOL, VFLOAT, le_maximumstageeffort_key);
	if (var == NOVARIABLE) maxstageeffort = MAXSTAGEEFFORT; else
		maxstageeffort = castfloat(var->addr);

	/* handle wire ratios */
	for(ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
	{
		var = getvalkey((INTBIG)ap, VARCPROTO, VINTEGER, le_wire_ratio_key);
		if (var != NOVARIABLE) ap->temp1 = var->addr; else
			ap->temp1 = DEFWIRERATIO;
	}
	DiaInitTextDialog(dia, DLEO_ARCLIST, le_topofarcs, le_nextarcs,
		DiaNullDlogDone, 0, SCSELMOUSE|SCREPORT);

	esnprintf(line, 200, x_("%g"), maxstageeffort);
	DiaSetText(dia, -DLEO_MAXGAIN, line);
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DLEO_INTERCAP || itemHit == DLEO_HIGHCOMP)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
		if (itemHit == DLEO_ARCLIST)
		{
			lineno = DiaGetCurLine(dia, DLEO_ARCLIST);
			if (lineno < 0) continue;
			estrcpy(line, DiaGetScrollLine(dia, DLEO_ARCLIST, lineno));
			for(pt = line; *pt != 0; pt++) if (*pt == ' ') break;
			*pt = 0;
			ap = getarcproto(line);
			if (ap == NOARCPROTO) continue;
			esnprintf(line, 200, x_("%ld"), ap->temp1);
			DiaSetText(dia, DLEO_WIRERATIO, line);
			continue;
		}
		if (itemHit == DLEO_WIRERATIO)
		{
			lineno = DiaGetCurLine(dia, DLEO_ARCLIST);
			if (lineno < 0) continue;
			estrcpy(line, DiaGetScrollLine(dia, DLEO_ARCLIST, lineno));
			for(pt = line; *pt != 0; pt++) if (*pt == ' ') break;
			*pt = 0;
			ap = getarcproto(line);
			if (ap == NOARCPROTO) continue;
			i = eatoi(DiaGetText(dia, DLEO_WIRERATIO));
			if (i == ap->temp1) continue;
			ap->temp1 = i;
			infstr = initinfstr();
			formatinfstr(infstr, x_("%s (%ld)"), describearcproto(ap), ap->temp1);
			DiaSetScrollLine(dia, DLEO_ARCLIST, lineno, returninfstr(infstr));
			continue;
		}
	}
	if (itemHit == OK)
	{
		newstate = 0;
		if (DiaGetControl(dia, DLEO_INTERCAP) != 0) newstate |= DISPLAYCAPACITANCE;
		if (DiaGetControl(dia, DLEO_HIGHCOMP) != 0) newstate |= HIGHLIGHTCOMPONENTS;
		if (newstate != state)
			(void)setvalkey((INTBIG)le_tool, VTOOL, le_state_key, newstate, VINTEGER);
		newmaxstageeffort = (float)eatof(DiaGetText(dia, DLEO_MAXGAIN));
		if (newmaxstageeffort != maxstageeffort)
			(void)setvalkey((INTBIG)le_tool, VTOOL, le_maximumstageeffort_key,
				castint(newmaxstageeffort), VFLOAT);
		for(ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
		{
			var = getvalkey((INTBIG)ap, VARCPROTO, VINTEGER, le_wire_ratio_key);
			if (var != NOVARIABLE) i = var->addr; else
				i = DEFWIRERATIO;
			if (i == ap->temp1) continue;
			if (ap->temp1 == DEFWIRERATIO)
			{
				delvalkey((INTBIG)ap, VARCPROTO, le_wire_ratio_key);
			} else
			{
				setvalkey((INTBIG)ap, VARCPROTO, le_wire_ratio_key, ap->temp1, VINTEGER);
			}
		}
	}
	DiaDoneDialog(dia);
}

/* Logical Effort effort dialog */
static DIALOGITEM le_logeffortdialogitems[] =
{
 /*  1 */ {0, {40,128,64,192}, BUTTON, N_("OK")},
 /*  2 */ {0, {40,12,64,76}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,116}, MESSAGE, N_("Logical Effort:")},
 /*  4 */ {0, {8,128,24,192}, EDITTEXT, x_("")}
};
static DIALOG le_logeffortdialog = {{75,75,149,281}, N_("Logical Effort"), 0, 4, le_logeffortdialogitems, 0, 0};

/* special items for the "Logical Effort effort" dialog: */
#define DLEE_EFFVALUE    4		/* Effort (edit text) */

/*
 * Routine to interactively set the logical effort value on the selected node.
 */
void le_setlogicaleffort(void)
{
	INTBIG itemHit;
	INTBIG inputs;
	REGISTER NODEINST *ni;
	REGISTER INTBIG type;
	double e;
	CHAR line[50], *pt;
	LENODE statle;
	REGISTER void *dia;

	ni = (NODEINST *)asktool(us_tool, x_("get-node"));
	if (ni == NONODEINST) return;
	type = le_getgatetype(ni, &inputs);
	if (type == LEUNKNOWN) return;
	statle.type = type;
	statle.ni = ni;
	statle.inputs = inputs;
	e = le_getlogeffort(&statle);
	dia = DiaInitDialog(&le_logeffortdialog);
	if (dia == 0) return;

	esnprintf(line, 50, x_("%g"), e);
	DiaSetText(dia, -DLEE_EFFVALUE, line);
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
	}
	if (itemHit == OK)
	{
		pt = DiaGetText(dia, DLEE_EFFVALUE);
		startobjectchange((INTBIG)ni, VNODEINST);
		esnprintf(line, 50, x_("g=%s"), pt);
		setvalkey((INTBIG)ni, VNODEINST, le_nodeeffort_key, (INTBIG)line, VSTRING|VDISPLAY);
		endobjectchange((INTBIG)ni, VNODEINST);
	}
	DiaDoneDialog(dia);
}

/*
 * Routine to set the capacitance on arc "ai" to "c".  This is displayed on the arc.
 */
void le_setarccapacitance(ARCINST *ai, double c)
{
	REGISTER VARIABLE *var;

	startobjectchange((INTBIG)ai, VARCINST);
	var = setvalkey((INTBIG)ai, VARCINST, le_attrcapacitance_key, castint((float)c),
		VFLOAT|VDISPLAY);
	if (var != NOVARIABLE)
	{
		TDSETDISPPART(var->textdescript, VTDISPLAYNAMEVALUE);
		TDSETUNITS(var->textdescript, VTUNITSCAP);
		TDSETSIZE(var->textdescript, TXTSETQLAMBDA(3));
		if (ai->end[0].ypos == ai->end[1].ypos)
		{
			/* horizontal arc: push text to top */
			TDSETPOS(var->textdescript, VTPOSUP);
		}
		if (ai->end[0].xpos == ai->end[1].xpos)
		{
			/* vertical arc: push text to right */
			TDSETPOS(var->textdescript, VTPOSRIGHT);
		}
	}
	endobjectchange((INTBIG)ai, VARCINST);
}

/******************** ANALYSIS ********************/

/*
 * Routine to analyze a path in "le_firstlenode".
 */
void le_analyzepath(void)
{
	LENODE *le, *pathend;
	REGISTER VARIABLE *var;
	REGISTER INTBIG displaycapacitance, state;
	CHAR line[50];
	double g, G, B, h, H, F, P, N, fhat, Dhat, Cin, Cout, CinI, CoutI;

	/* find the end of the path */
	for(le = le_firstlenode; le != NOLENODE; le = le->nextlenode) pathend = le;

	/* determine the number of stages of logic in the path */
	N = 0.0;
	for(le = le_firstlenode; le != NOLENODE; le = le->nextlenode) N++;
	if (N == 0.0) return;

	/* compute the path logical effort by multiplying the individual ones */
	G = 1.0;
	for(le = le_firstlenode; le != NOLENODE; le = le->nextlenode)
	{
		g = le->logeffort;
		if (g == 0.0) return;
		G = G * g;
	}

	/* compute the branching effort along the path */
	B = 1.0;
	for(le = le_firstlenode; le != NOLENODE; le = le->nextlenode)
		B = B * le->branching;

	/* compute the electrical effort along the path */
	Cin = le_firstlenode->cin;
	Cout = pathend->cout;
	ttyputmsg(_("Capacitance starts at %g, ends at %g"), Cin, Cout);
	if (Cin == 0.0) return;
	H = Cout / Cin;

	/* compute the overall path effort */
	F = G * B * H;

	/* determine the total parasitic effect */
	P = 0.0;
	for(le = le_firstlenode; le != NOLENODE; le = le->nextlenode)
		P = P + le->parasitics;

	/* compute the stage effort */
	fhat = pow(F, 1/N);
	ttyputmsg(_("Optimum stage effort is %g"), fhat);
	if (fhat == 0.0) return;

	/* compute the minimum path delay */
	Dhat = N * fhat + P;
	ttyputmsg(_("Minimum path delay is %g"), Dhat);

	/* determine whether or not to set capacitance values */
	var = getvalkey((INTBIG)le_tool, VTOOL, VINTEGER, le_state_key);
	if (var == NOVARIABLE) state = DEFAULTSTATE; else
		state = var->addr;
	displaycapacitance = state & DISPLAYCAPACITANCE;

	/* work backwards through the path, computing electrical effort of each gate */
	CoutI = Cout;
	for(le = pathend; le != NOLENODE; le = le->prevlenode)
	{
		/* determine input capacitance to this node */
		CinI = le->logeffort * CoutI / fhat;
		if (le->prevlenode != NOLENODE && displaycapacitance != 0)
			le_setarccapacitance(le->piin->conarcinst, CinI);
		if (CinI == 0.0) break;

		/* set fanout (h) on this node */
		h = CoutI / CinI;
		esnprintf(line, 50, x_("h=%g"), h);
		startobjectchange((INTBIG)le->ni, VNODEINST);
		setvalkey((INTBIG)le->ni, VNODEINST, le_fanout_key, (INTBIG)line, VSTRING|VDISPLAY);
		endobjectchange((INTBIG)le->ni, VNODEINST);

		/* shift input capacitance to the output of the previous node */
		CoutI = CinI;
	}
}

/******************** DELAY ESTIMATION ********************/

#define NONETDELAY ((NETDELAY *)-1)

typedef struct Inetdelay
{
	float             numerator;
	float             pdenominator;
	float             ndenominator;
	NETWORK          *net;
	struct Inetdelay *nextnetdelay;
} NETDELAY;

/*
 * Routine to analyze cell "cell" and build a list of ERC errors.
 */
void le_estimatedelay(NODEPROTO *cell)
{
	REGISTER NETWORK *net;
	TRANSISTORINFO *p_gate, *n_gate, *p_active, *n_active;
	REGISTER AREAPERIM *ap, *aplist, *nextap;
	REGISTER INTBIG fun;
	REGISTER NETDELAY *nd, *firstnd;
	float numerator, coefficient, flambda;

	firstnd = NONETDELAY;
	flambda = (float)lambdaofcell(cell);
	for(net = cell->firstnetwork; net != NONETWORK; net = net->nextnetwork)
	{
		aplist = net_gathergeometry(net, &p_gate, &n_gate, &p_active, &n_active, TRUE);

		/* numerator has the area of all transistors connected by their gate */
		numerator = (float)(p_gate->area + n_gate->area);
ttyputmsg(x_("Network %s has numerator:"), describenetwork(net));
ttyputmsg(x_("      N Gates = %g, P Gates = %g"), n_gate->area/flambda/flambda, p_gate->area/flambda/flambda);

		/* numerator also sums area of each layer */
		for(ap = aplist; ap != NOAREAPERIM; ap = nextap)
		{
			nextap = ap->nextareaperim;
			fun = layerfunction(ap->tech, ap->layer);
			if ((fun&LFTYPE) != LFDIFF && !layerismetal(fun) && !layerispoly(fun)) continue;
			coefficient = 1.0f;
			if (layerismetal(fun)) coefficient = 0.1f;
			if (layerispoly(fun)) coefficient = 0.1f;
ttyputmsg(x_("      Layer %s has %g x %g = %g"), layername(ap->tech, ap->layer),
	ap->area/flambda/flambda, coefficient, ap->area*coefficient/flambda/flambda);
			numerator += (float)ap->area * coefficient;
			efree((CHAR *)ap);
		}

		/* save the results */
		nd = (NETDELAY *)emalloc(sizeof (NETDELAY), le_tool->cluster);
		if (nd == 0) break;
		nd->pdenominator = (float)p_active->width / flambda;
		nd->ndenominator = (float)n_active->width / flambda;
		nd->numerator = numerator / flambda / flambda;
if (n_active->width == 0) ttyputmsg(x_("   N denominator undefined")); else
  ttyputmsg(x_("   N denominator = %g, ratio = %g"), nd->ndenominator, nd->numerator/nd->ndenominator);
if (p_active->width == 0) ttyputmsg(x_("   P denominator undefined")); else
  ttyputmsg(x_("   P denominator = %g, ratio = %g"), nd->pdenominator, nd->numerator/nd->pdenominator);
if (n_active->width+p_active->width == 0) ttyputmsg(x_("   Denominator undefined")); else
  ttyputmsg(x_("   Denominator = %g, ratio = %g"), nd->ndenominator+nd->pdenominator,
	nd->numerator/(nd->ndenominator+nd->pdenominator));
		nd->net = net;
		nd->nextnetdelay = firstnd;
		firstnd = nd;
	}

	/* now sort by delay size */

#if 0	/* code to show the results */
	for(nd = firstnd; nd != NONETDELAY; nd = nd->nextnetdelay)
	{
		REGISTER void *infstr;

		ttyputmsg(M_("Network %s:"), describenetwork(nd->net));
		infstr = initinfstr();
		formatinfstr(infstr, M_("Network %s has N delay "), describenetwork(nd->net));
		if (nd->ndenominator == 0.0)
		{
			ttyputmsg(M_("   N delay UNDEFINED"));
		} else
		{
			ttyputmsg(M_("   N delay is %g/%g = %g"), nd->numerator, nd->ndenominator,
				nd->numerator / nd->ndenominator);
		}
		if (nd->pdenominator == 0.0)
		{
			ttyputmsg(M_("   P delay UNDEFINED"));
		} else
		{
			ttyputmsg(M_("   P delay is %g/%g = %g"), nd->numerator, nd->pdenominator,
				nd->numerator / nd->pdenominator);
		}
	}
#endif
}

/******************** LOAD CALCULATION ********************/

void le_showloads(void)
{
	REGISTER NODEPROTO *np;
	REGISTER NETWORK *net, **netlist, **selnets;
	REGISTER ARCPROTO *ap;
	REGISTER VARIABLE *var;
	REGISTER INTBIG lambda, gwidth, gtotal, atotal, wirelen, fun, load, wireratio, thisload, thiswl,
		total, i;
	float areatotal;
	REGISTER CHAR *lname;
	REGISTER AREAPERIM *arpe, *firstarpe, *nextarpe;
	TRANSISTORINFO *p_gate, *n_gate, *p_active, *n_active;
	REGISTER void *infstr;

	/* get the current cell */
	np = getcurcell();
	if (np == NONODEPROTO)
	{
		ttyputerr(_("No current cell"));
		return;
	}
	lambda = lambdaofcell(np);
	selnets = net_gethighlightednets(FALSE);

	/* gather product information for all nets in the cell */
	for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
	{
		net->temp2 = 0;

		/* if some nets were selected, ignore all others */
		if (selnets[0] != NONETWORK)
		{
			for(i=0; selnets[i] != NONETWORK; i++)
				if (selnets[i] == net) break;
			if (selnets[i] == NONETWORK) continue;
		}

		/* gather geometry on this network */
		firstarpe = net_gathergeometry(net, &p_gate, &n_gate, &p_active, &n_active, TRUE);

		/* see if there are gates on the network */
		gtotal = net_transistor_p_gate.count + net_transistor_n_gate.count;
		if (gtotal > 0)
		{
			/* sum the metal half-perimeters on the network */
			wirelen = 0;
			for(arpe = firstarpe; arpe != NOAREAPERIM; arpe = arpe->nextareaperim)
			{
				fun = layerfunction(arpe->tech, arpe->layer);
				if (!layerismetal(fun)) continue;
				wirelen += arpe->perimeter / 2;
			}
			gwidth = net_transistor_p_gate.width + net_transistor_n_gate.width;
			net->temp2 = muldiv(wirelen, gwidth, lambda);
		}

		/* free the area/perimeter information */
		for(arpe = firstarpe; arpe != NOAREAPERIM; arpe = nextarpe)
		{
			nextarpe = arpe->nextareaperim;
			efree((CHAR *)arpe);
		}
	}

	/* now sort the networks by this product number in "temp2" */
	total = 0;
	for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		if (net->temp2 != 0) total++;
	if (total == 0)
	{
		ttyputmsg(_("There are no networks with load information"));
		return;
	}
	netlist = (NETWORK **)emalloc(total * (sizeof (NETWORK *)), le_tool->cluster);
	if (netlist == 0) return;
	total = 0;
	for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		if (net->temp2 != 0) netlist[total++] = net;

	/* sort by product number */
	esort(netlist, total, sizeof (NETWORK *), le_sortbyproductsize);

	/* now report the results */
	for(i=0; i<total; i++)
	{
		/* gather geometry on this network */
		net = netlist[i];
		ttyputmsg(_("For network %s:"), describenetwork(net));
		firstarpe = net_gathergeometry(net, &p_gate, &n_gate, &p_active, &n_active, TRUE);

		/* determine the wire length */
		load = wirelen = 0;
		areatotal = 0.0;
		for(arpe = firstarpe; arpe != NOAREAPERIM; arpe = arpe->nextareaperim)
		{
			lname = layername(arpe->tech, arpe->layer);
			fun = layerfunction(arpe->tech, arpe->layer);
			if (!layerismetal(fun)) continue;
			ap = getarconlayer(arpe->layer, arpe->tech);
			wireratio = DEFWIRERATIO;
			if (ap != NOARCPROTO)
			{
				var = getvalkey((INTBIG)ap, VARCPROTO, VINTEGER, le_wire_ratio_key);
				if (var != NOVARIABLE) wireratio = var->addr;
			}
			lname = layername(arpe->tech, arpe->layer);
			thiswl = arpe->perimeter / 2;
			thisload = thiswl / wireratio;
			ttyputmsg(x_("  Layer %s wire-length (%s) / wire ratio (%ld) = load (%s)"),
				lname, latoa(thiswl, 0), wireratio, latoa(thisload, 0));
			load += thisload;
			wirelen += thiswl;
			areatotal += arpe->area;
		}

		gwidth = net_transistor_p_gate.width + net_transistor_n_gate.width;
		ttyputmsg(_("  Total wire-length (%s) x gate-widths (%s) = product (%s); average wire-width = %g"),
			latoa(wirelen, 0), latoa(gwidth, 0), latoa(muldiv(wirelen, gwidth, lambda), 0),
				areatotal / (float)wirelen / (float)lambda);

		atotal = net_transistor_p_active.count + net_transistor_n_active.count;
		if (atotal > 0)
		{
			infstr = initinfstr();
			formatinfstr(infstr, _("  Load = %s"), latoa(load, 0));
			if (net_transistor_p_active.width != 0)
				formatinfstr(infstr, x_("; load / P-active-width (%s) = %g"), latoa(net_transistor_p_active.width, 0),
					(float)load / (float)net_transistor_p_active.width);
			if (net_transistor_n_active.width != 0)
				formatinfstr(infstr, x_("; load / N-active-width (%s) = %g"), latoa(net_transistor_n_active.width, 0),
					(float)load / (float)net_transistor_n_active.width);
			ttyputmsg(x_("%s"), returninfstr(infstr));
		}

		for(arpe = firstarpe; arpe != NOAREAPERIM; arpe = nextarpe)
		{
			nextarpe = arpe->nextareaperim;
			efree((CHAR *)arpe);
		}
	}
	efree((CHAR *)netlist);
}

/*
 * Helper routine for to sort NETWORK objects by product in "temp2"
 */
int le_sortbyproductsize(const void *e1, const void *e2)
{
	REGISTER NETWORK *net1, *net2;

	net1 = *((NETWORK **)e1);
	net2 = *((NETWORK **)e2);
	return(net1->temp2 - net2->temp2);
}

void le_analyzenetwork(void)
{
	REGISTER NETWORK **netlist, *net;
	REGISTER NODEPROTO *np;
	REGISTER ARCPROTO *ap;
	REGISTER INTBIG i, j, k, widest, len, lambda, total, gtotal, atotal,
		fun, metpolhalfperim, load, wireratio, additionalload;
	REGISTER CHAR *lname, *pad;
	REGISTER VARIABLE *var;
	REGISTER AREAPERIM *arpe, *firstarpe, **arpelist;
	TRANSISTORINFO *p_gate, *n_gate, *p_active, *n_active;
	float ratio;
	REGISTER void *infstr;

	netlist = net_gethighlightednets(0);
	if (netlist[0] == NONETWORK) return;
	for(k=0; netlist[k] != NONETWORK; k++)
	{
		net = netlist[k];

		/* gather geometry on this network */
		np = net->parent;
		firstarpe = net_gathergeometry(net, &p_gate, &n_gate, &p_active, &n_active, TRUE);

		/* copy the linked list to an array for sorting */
		total = 0;
		for(arpe = firstarpe; arpe != NOAREAPERIM; arpe = arpe->nextareaperim)
			if (arpe->layer >= 0) total++;
		if (total == 0)
		{
			ttyputmsg(_("No geometry on network '%s' in cell %s"), describenetwork(net),
				describenodeproto(np));
			continue;
		}
		arpelist = (AREAPERIM **)emalloc(total * (sizeof (AREAPERIM *)), net_tool->cluster);
		if (arpelist == 0) return;
		i = 0;
		for(arpe = firstarpe; arpe != NOAREAPERIM; arpe = arpe->nextareaperim)
			if (arpe->layer >= 0) arpelist[i++] = arpe;

		/* sort the layers */
		esort(arpelist, total, sizeof (AREAPERIM *), net_areaperimdepthascending);

		ttyputmsg(_("For network '%s' in cell %s:"), describenetwork(net),
			describenodeproto(np));
		lambda = lambdaofcell(np);
		widest = 0;
		for(i=0; i<total; i++)
		{
			arpe = arpelist[i];
			lname = layername(arpe->tech, arpe->layer);
			len = estrlen(lname);
			if (len > widest) widest = len;
		}
		metpolhalfperim = 0;
		for(i=0; i<total; i++)
		{
			arpe = arpelist[i];
			lname = layername(arpe->tech, arpe->layer);
			infstr = initinfstr();
			for(j=estrlen(lname); j<widest; j++) addtoinfstr(infstr, ' ');
			pad = returninfstr(infstr);
			infstr = initinfstr();
			formatinfstr(infstr, _("Layer %s:%s area=%7g  half-perimeter=%s"), lname, pad,
				arpe->area/(float)lambda/(float)lambda, latoa(arpe->perimeter/2, 0));
			fun = layerfunction(arpe->tech, arpe->layer);
			if (layerispoly(fun) != 0 || layerismetal(fun) != 0)
			{
				ap = getarconlayer(arpe->layer, arpe->tech);
				wireratio = DEFWIRERATIO;
				if (ap != NOARCPROTO)
				{
					var = getvalkey((INTBIG)ap, VARCPROTO, VINTEGER, le_wire_ratio_key);
					if (var != NOVARIABLE) wireratio = var->addr;
				}
				additionalload = arpe->perimeter / 2 / wireratio;
				metpolhalfperim += additionalload;
				formatinfstr(infstr, _(" / wire-ratio (%ld) = %s"), wireratio,
					latoa(additionalload, 0));
			}
			if (arpe->perimeter != 0)
			{
				ratio = (arpe->area / (float)lambda) / (float)(arpe->perimeter/2);
				formatinfstr(infstr, _("; area/half-perimeter = %g"), ratio);
			}
			ttyputmsg(x_("%s"), returninfstr(infstr));
			efree((CHAR *)arpe);
		}
		efree((CHAR *)arpelist);
		gtotal = net_transistor_p_gate.count + net_transistor_n_gate.count;
		if (gtotal > 0)
		{
			infstr = initinfstr();
			formatinfstr(infstr, _("Connects to the gate of %ld %s (total width %s, average length %s)"),
				gtotal, makeplural(_("transistor"), gtotal),
					latoa(net_transistor_p_gate.width+net_transistor_n_gate.width, 0),
						latoa((net_transistor_p_gate.length+net_transistor_n_gate.length)/gtotal, 0));
			ttyputmsg(x_("%s"), returninfstr(infstr));
		}
		atotal = net_transistor_p_active.count + net_transistor_n_active.count;
		if (atotal > 0)
		{
			infstr = initinfstr();
			formatinfstr(infstr, _("Connects to the active of %ld %s (total width %s, average length %s)"),
				atotal, makeplural(_("transistor"), atotal),
					latoa(net_transistor_p_active.width+net_transistor_n_active.width, 0),
						latoa((net_transistor_p_active.length+net_transistor_n_active.length)/atotal, 0));
			ttyputmsg(x_("%s"), returninfstr(infstr));
		}
		if (metpolhalfperim > 0 && gtotal > 0 && atotal > 0)
		{
			ttyputmsg(x_("---------- Load Calculations:"));
			ttyputmsg(x_("Sum of Metal and Poly half-perimeters / wire-ratio = %s"), latoa(metpolhalfperim, 0));
			load = metpolhalfperim + net_transistor_p_gate.width+net_transistor_n_gate.width;
			ttyputmsg(x_("  Sum + gate-width (%s) = %s (Load)"),
				latoa(net_transistor_p_gate.width+net_transistor_n_gate.width, 0), latoa(load, 0));
			if (net_transistor_p_active.width != 0)
				ttyputmsg(x_("  Load / P-active-width (%s) = %g"), latoa(net_transistor_p_active.width, 0),
					(float)load / (float)net_transistor_p_active.width);
			if (net_transistor_n_active.width != 0)
				ttyputmsg(x_("  Load / N-active-width (%s) = %g"), latoa(net_transistor_n_active.width, 0),
					(float)load / (float)net_transistor_n_active.width);
		}
	}
}

/******************** CELL EXTRACTION ********************/

/*
 * Routine to gather all relevant nodes in the current cell and build the structure
 * headed by "le_firstlenode".
 */
void le_gathercell(INTBIG show)
{
	REGISTER INTBIG inport, oinport, which;
	REGISTER BOOLEAN first;
	REGISTER INTBIG arrowsize, x, y, i, type;
	INTBIG inputs;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai, *oai;
	REGISTER NODEPROTO *np;
	REGISTER NETWORK *net;
	REGISTER VARIABLE *var;
	REGISTER PORTARCINST *pi, *opi;
	LENODE *le, *ole;
	double cap;
	REGISTER void *infstr;

	/* make sure there is a current cell */
	le_freealllenodes();
	np = getcurcell();
	if (np == NONODEPROTO) return;

	/* reset to find power and ground nets */
	for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		net->temp1 = 0;

	/* gather all relevant nodes in the cell */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		type = le_getgatetype(ni, &inputs);
		switch (type)
		{
			case LEUNKNOWN:
			case LETERMINAL:
				break;
			case LEPOWER:
			case LEGROUND:
				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
					if (pi->conarcinst->network != NONETWORK)
						pi->conarcinst->network->temp1 = 1;
				break;
			default:
				le = le_alloclenode();
				if (le == NOLENODE) break;
				le->ni = ni;
				le->piin = le->piout = NOPORTARCINST;
				le->cin = 0.0;
				le->cout = 0.0;
				le->type = type;
				le->inputs = inputs;
				le->logeffort = le_getlogeffort(le);
				le->parasitics = le_getparasitics(le);
				le->numinputnodes = 0;
				le->numoutputnodes = 0;
				le->nextlenode = le_firstlenode;
				le_firstlenode = le;
				break;
		}
	}

	/* add input capacitances to the LENODEs */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		ai->temp1 = 0;
	for(le = le_firstlenode; le != NOLENODE; le = le->nextlenode)
	{
		for(pi = le->ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			ai = pi->conarcinst;
			if (ai->network == NONETWORK) continue;
			if ((pi->proto->userbits & STATEBITS) != INPORT) continue;

			/* see if there is a capacitance specification on the input to this node */
			cap = le_getcapacitance(ai->network);
			if (cap == 0.0) continue;
			le->cin = cap;
			ai->temp1 = 1;
		}
	}

	/* create additional LENODEs for those nodes with input capacitances */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* see if there is capacitance on this node */
		var = getvalkey((INTBIG)ni, VNODEINST, -1, le_attrcapacitance_key);
		if (var == NOVARIABLE) continue;

		cap = eatof(describesimplevariable(var));
		le = le_alloclenode();
		if (le == NOLENODE) continue;
		le->ni = ni;
		le->piin = le->piout = NOPORTARCINST;
		le->cin = cap;
		le->cout = 0.0;
		le->type = LETERMINAL;
		le->logeffort = 1.0;
		le->parasitics = 0.0;
		le->numinputnodes = 0;
		le->numoutputnodes = 0;
		le->nextlenode = le_firstlenode;
		le_firstlenode = le;
	}

	/* add linkage between the nodes */
	for(le = le_firstlenode; le != NOLENODE; le = le->nextlenode)
	{
		for(pi = le->ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			ai = pi->conarcinst;
			if (ai->network == NONETWORK) continue;
			if (ai->network->temp1 != 0) continue;
			switch (pi->proto->userbits & STATEBITS)
			{
				case INPORT:    inport =  1;   break;
				case BIDIRPORT:
				case OUTPORT:   inport =  0;   break;
				default:        inport = -1;   break;
			}
			if (le->type == LETERMINAL) inport = 1;
			if (inport < 0) continue;

			/* find other LENODE that connects to this */
			for(ole = le->nextlenode; ole != NOLENODE; ole = ole->nextlenode)
			{
				for(opi = ole->ni->firstportarcinst; opi != NOPORTARCINST; opi = opi->nextportarcinst)
				{
					oai = opi->conarcinst;
					if (oai->network != ai->network) continue;
					if (oai->network->temp1 != 0) continue;

					/* LENODEs connect, check directionality of ports */
					switch (opi->proto->userbits & STATEBITS)
					{
						case INPORT:    oinport =  1;   break;
						case BIDIRPORT:
						case OUTPORT:   oinport =  0;   break;
						default:        oinport = -1;   break;
					}
					if (ole->type == LETERMINAL) oinport = 1;
					if (oinport < 0) continue;

					/* figure out how to connect them */
					if (inport != 0)
					{
						/* configure input on node "le" */
						if (oinport != 0) continue;
						le_addlinkage(ole, opi, le, pi);
					} else
					{
						/* configure output on node "le" */
						if (oinport == 0)
						{
							ttyputerr(_("Node %s and %s drive the same network"),
								le_describenode(le), le_describenode(ole));
							continue;
						}
						le_addlinkage(le, pi, ole, opi);
					}
				}
			}
		}
	}

	/* stop now if display is not requested */
	if (show == 0) return;

	/* display the nodes */
	infstr = initinfstr();
	first = FALSE;
	for(le = le_firstlenode; le != NOLENODE; le = le->nextlenode)
	{
		if (first) addtoinfstr(infstr, '\n');
		first = TRUE;
		formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0"),
			describenodeproto(np), (INTBIG)le->ni->geom);
	}
	(void)asktool(us_tool, x_("show-multiple"), (INTBIG)returninfstr(infstr));

	/* display the connection sites */
	arrowsize = lambdaofcell(np) * 2;
	for(le = le_firstlenode; le != NOLENODE; le = le->nextlenode)
	{
		if (le->piin != NOPORTARCINST)
		{
			ai = le->piin->conarcinst;
			if (ai->end[0].portarcinst == le->piin) which = 0; else which = 1;
			x = ai->end[which].xpos;
			y = ai->end[which].ypos;
			(void)asktool(us_tool, x_("show-line"), x, y, x-arrowsize, y-arrowsize, ai->parent);
			(void)asktool(us_tool, x_("show-line"), x, y, x-arrowsize/2, y-arrowsize/5, ai->parent);
			(void)asktool(us_tool, x_("show-line"), x, y, x-arrowsize/5, y-arrowsize/2, ai->parent);
		}
		for(i=0; i<le->numoutputnodes; i++)
		{
			ai = le->outputports[i]->conarcinst;
			if (ai->end[0].portarcinst == le->outputports[i]) which = 0; else which = 1;
			x = ai->end[which].xpos+arrowsize;
			y = ai->end[which].ypos+arrowsize;
			(void)asktool(us_tool, x_("show-line"), x, y, x-arrowsize, y-arrowsize, ai->parent);
			(void)asktool(us_tool, x_("show-line"), x, y, x-arrowsize/2, y-arrowsize/5, ai->parent);
			(void)asktool(us_tool, x_("show-line"), x, y, x-arrowsize/5, y-arrowsize/2, ai->parent);
		}
	}
}

/*
 * Routine to add a connection that runs out of LENODE "out", port "outpi" into
 * LENODE "in", port "inpi".
 */
void le_addlinkage(LENODE *out, PORTARCINST *outpi, LENODE *in, PORTARCINST *inpi)
{
	LENODE **newinputnodelist, **newoutputnodelist;
	PORTARCINST **newinputportlist, **newoutputportlist;
	REGISTER INTBIG i;

	/* add input node to list of outputs */
	newoutputnodelist = (LENODE **)emalloc((out->numoutputnodes+1) * (sizeof (LENODE *)), le_tool->cluster);
	newoutputportlist = (PORTARCINST **)emalloc((out->numoutputnodes+1) * (sizeof (PORTARCINST *)), le_tool->cluster);
	for(i=0; i<out->numoutputnodes; i++)
	{
		newoutputnodelist[i] = out->outputnodes[i];
		newoutputportlist[i] = out->outputports[i];
	}
	newoutputnodelist[out->numoutputnodes] = in;
	newoutputportlist[out->numoutputnodes] = outpi;
	if (out->numoutputnodes > 0)
	{
		efree((CHAR *)out->outputnodes);
		efree((CHAR *)out->outputports);
	}
	out->outputnodes = newoutputnodelist;
	out->outputports = newoutputportlist;
	out->numoutputnodes++;

	/* add output node to list of inputs */
	newinputnodelist = (LENODE **)emalloc((in->numinputnodes+1) * (sizeof (LENODE *)), le_tool->cluster);
	newinputportlist = (PORTARCINST **)emalloc((in->numinputnodes+1) * (sizeof (PORTARCINST *)), le_tool->cluster);
	for(i=0; i<in->numinputnodes; i++)
	{
		newinputnodelist[i] = in->inputnodes[i];
		newinputportlist[i] = in->inputports[i];
	}
	newinputnodelist[in->numinputnodes] = out;
	newinputportlist[in->numinputnodes] = inpi;
	if (in->numinputnodes > 0)
	{
		efree((CHAR *)in->inputnodes);
		efree((CHAR *)in->inputports);
	}
	in->inputnodes = newinputnodelist;
	in->inputports = newinputportlist;
	in->numinputnodes++;
}

void le_analyzecell(void)
{
	REGISTER LENODE *le;
	REGISTER PORTARCINST *pi;
	REGISTER INTBIG iteration, changeneeded, changemade, i, displaycapacitance, state;
	REGISTER VARIABLE *var;
	CHAR line[50];
	double h, needed, maxstageeffort;

	/* determine maximum stage effort */
	var = getvalkey((INTBIG)le_tool, VTOOL, VFLOAT, le_maximumstageeffort_key);
	if (var == NOVARIABLE) maxstageeffort = MAXSTAGEEFFORT; else
		maxstageeffort = (double)castfloat(var->addr);
	ttyputmsg(_("Maximum stage effort is %g"), maxstageeffort);

	/* now iterate */
	for(iteration=0; iteration<MAXITERATIONS; iteration++)
	{
		changeneeded = changemade = 0;
		for(le = le_firstlenode; le != NOLENODE; le = le->nextlenode)
		{
			/* propagate this node */
			if (le->numoutputnodes == 0) continue;

			/* sum up all of the capacitances that this node generates */
			le->cout = 0.0;
			for(i=0; i<le->numoutputnodes; i++)
			{
				le->cout += le->outputnodes[i]->cin;
			}

			/* see if this node can generate that much capacitance */
			needed = le->cout / maxstageeffort * le->logeffort;
			if (le->cin < needed)
			{
				changeneeded = 1;
				if (le->cin != needed) changemade = 1;
				le->cin = needed;
			}
		}
		if (changemade == 0) break;
	}
	if (changemade != 0)
	{
		ttyputerr(_("WARNING: After %d iterations, analysis is still not stable"), MAXITERATIONS);
	} else if (changeneeded != 0)
	{
		ttyputerr(_("WARNING: Unable to find solution with maximum stage effort of %g"), maxstageeffort);
	}

	/* determine whether or not to set capacitance values */
	var = getvalkey((INTBIG)le_tool, VTOOL, VINTEGER, le_state_key);
	if (var == NOVARIABLE) state = DEFAULTSTATE; else
		state = var->addr;
	displaycapacitance = state & DISPLAYCAPACITANCE;

	for(le = le_firstlenode; le != NOLENODE; le = le->nextlenode)
	{
		if (le->numoutputnodes == 0) continue;
		if (displaycapacitance != 0)
		{
			for(pi = le->ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			{
				if ((pi->proto->userbits & STATEBITS) != INPORT) continue;
				le_setarccapacitance(pi->conarcinst, le->cin);
			}
		}
		h = le->cout / le->cin;
		esnprintf(line, 50, x_("h=%g"), h);
		startobjectchange((INTBIG)le->ni, VNODEINST);
		setvalkey((INTBIG)le->ni, VNODEINST, le_fanout_key, (INTBIG)line, VSTRING|VDISPLAY);
		endobjectchange((INTBIG)le->ni, VNODEINST);
	}
}

/******************** PATH EXTRACTION ********************/

/*
 * Routine to gather a path, given that two nodes at the ends of the path
 * are highlighted.  Fills the list of LENODEs pointed to by "le_firstlenode".
 */
void le_gatherpath(INTBIG show)
{
	REGISTER BOOLEAN first;
	REGISTER INTBIG edge, x, y, arrowsize, agree, disagree, which;
	REGISTER NODEINST *ret, *ni;
	REGISTER ARCINST *ai;
	REGISTER NODEPROTO *np;
	REGISTER PORTARCINST *pi;
	double c, cstart, cend;
	GEOM **list;
	LENODE *le, *newle, *nextle, *lastle;
	REGISTER void *infstr;

	/* make sure there is a current cell */
	le_freealllenodes();
	np = getcurcell();
	if (np == NONODEPROTO) return;

	/* get all selected objects (must be 2 nodes) */
	list = (GEOM **)asktool(us_tool, x_("get-all-objects"));
	if (list[0] == NOGEOM || list[1] == NOGEOM || list[2] != NOGEOM)
	{
		ttyputerr(_("Select two objects at the ends of a path"));
		return;
	}

	/* find a path from one node to the other (wavefront propagation) */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		ni->temp1 = ni->temp2 = 0;
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		ai->temp1 = ai->temp2 = 0;
	if (list[0]->entryisnode)
	{
		ni = list[0]->entryaddr.ni;
		ni->temp1 = 1;
		cstart = 0.0;
	} else
	{
		ai = list[0]->entryaddr.ai;
		ai->temp2 = 1;
		ai->end[0].nodeinst->temp1 = 1;
		ai->end[1].nodeinst->temp1 = 1;
		cstart = le_getcapacitance(ai->network);
	}
	if (list[1]->entryisnode)
	{
		ni = list[1]->entryaddr.ni;
		ni->temp1 = -1;
		cend = 0.0;
	} else
	{
		ai = list[1]->entryaddr.ai;
		ai->temp2 = 1;
		ai->end[0].nodeinst->temp1 = -1;
		ai->end[1].nodeinst->temp1 = -1;
		cend = le_getcapacitance(ai->network);
	}
	for(edge = 1; ; edge++)
	{
		ret = le_propagate(np, edge);
		if (ret != 0) break;
	}
	if (ret == NONODEINST)
	{
		ttyputerr(_("No path exists between these nodes"));
		return;
	}

	/* unwind the path and create a chain of LENODEs */
	le_lastlenode = NOLENODE;
	le_lastcapacitance = 1.0;
	le_unwind(ret, NONODEINST);
	if (le_firstlenode != NOLENODE)
	{
		if (cstart != 0.0) le_firstlenode->cin = cstart;
		if (cend != 0.0)
		{
			for(le = le_firstlenode; le != NOLENODE; le = le->nextlenode) lastle = le;
			lastle->cout = cend;
		}
	}

	/* reverse the path if the ports indicate it */
	agree = disagree = 0;
	for(le = le_firstlenode; le != NOLENODE; le = le->nextlenode)
	{
		if (le->piin != NOPORTARCINST)
		{
			switch (le->piin->proto->userbits & STATEBITS)
			{
				case INPORT:  agree++;      break;
				case OUTPORT: disagree++;   break;
			}
		}
		if (le->piout != NOPORTARCINST)
		{
			switch (le->piout->proto->userbits & STATEBITS)
			{
				case INPORT:  disagree++;   break;
				case OUTPORT: agree++;      break;
			}
		}
	}
	if (agree != 0 && disagree != 0)
		ttyputmsg(_("Directionality of path is conflicting"));
	if (agree == 0 && disagree == 0)
		ttyputmsg(_("Directionality of path is unknown"));
	if (disagree != 0)
	{
		/* reverse the path */
		newle = NOLENODE;
		for(le = le_firstlenode; le != NOLENODE; le = nextle)
		{
			nextle = le->nextlenode;
			le->nextlenode = newle;
			newle = le;
			pi = le->piin;   le->piin = le->piout;   le->piout = pi;
			c = le->cin;     le->cin = le->cout;     le->cout = c;
		}
		le_firstlenode = newle;
	}

	/* add back-pointers to path */
	lastle = NOLENODE;
	for(le = le_firstlenode; le != NOLENODE; le = le->nextlenode)
	{
		le->prevlenode = lastle;
		lastle = le;
	}

	/* determine branching */
	le_figurebranching();

	/* stop now if display is not requested */
	if (show == 0) return;

	/* describe the nodes in the path */
	for(le = le_firstlenode; le != NOLENODE; le = le->nextlenode)
	{
		ttyputmsg(_("%s: LogEffort=%g Parasitics=%g Cin=%g Cout=%g Branching=%g"),
			le_describenode(le), le->logeffort, le->parasitics, le->cin, le->cout,
				le->branching);
	}

	/* display the path between the nodes */
	(void)asktool(us_tool, x_("clear"));
	infstr = initinfstr();
	first = FALSE;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->temp2 == 0) continue;
		if (first) addtoinfstr(infstr, '\n');
		first = TRUE;
		formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0"),
			describenodeproto(np), (INTBIG)ni->geom);
	}
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->temp2 == 0) continue;
		addtoinfstr(infstr, '\n');
		formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0"),
			describenodeproto(np), (INTBIG)ai->geom);
	}
	(void)asktool(us_tool, x_("show-multiple"), (INTBIG)returninfstr(infstr));

	arrowsize = lambdaofcell(np) * 2;
	for(le = le_firstlenode; le != NOLENODE; le = le->nextlenode)
	{
		if (le->piin != NOPORTARCINST)
		{
			ai = le->piin->conarcinst;
			if (ai->end[0].portarcinst == le->piin) which = 0; else which = 1;
			x = ai->end[which].xpos;
			y = ai->end[which].ypos;
			(void)asktool(us_tool, x_("show-line"), x, y, x-arrowsize, y-arrowsize, ai->parent);
			(void)asktool(us_tool, x_("show-line"), x, y, x-arrowsize/2, y-arrowsize/5, ai->parent);
			(void)asktool(us_tool, x_("show-line"), x, y, x-arrowsize/5, y-arrowsize/2, ai->parent);
		}
		if (le->piout != NOPORTARCINST)
		{
			ai = le->piout->conarcinst;
			if (ai->end[0].portarcinst == le->piout) which = 0; else which = 1;
			x = ai->end[which].xpos+arrowsize;
			y = ai->end[which].ypos+arrowsize;
			(void)asktool(us_tool, x_("show-line"), x, y, x-arrowsize, y-arrowsize, ai->parent);
			(void)asktool(us_tool, x_("show-line"), x, y, x-arrowsize/2, y-arrowsize/5, ai->parent);
			(void)asktool(us_tool, x_("show-line"), x, y, x-arrowsize/5, y-arrowsize/2, ai->parent);
		}
	}
}

/*
 * helper function for "le_gatherpath" which propagates a wave of connectivity
 * through the circuit to find a path between the end nodes.
 * Returns NONODEINST if the propagation has failed to advance.
 * Returns zero if the propagation must run more.
 * Returns a NODEINST if it has found that node at then end of the path
 */
NODEINST *le_propagate(NODEPROTO *cell, INTBIG edge)
{
	REGISTER NODEINST *ni, *oni;
	REGISTER PORTARCINST *pi;
	REGISTER ARCINST *ai;
	REGISTER INTBIG otherend, moved;

	moved = 0;
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->temp1 != edge) continue;

		/* propagate from here */
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			ai = pi->conarcinst;
			if (ai->end[0].portarcinst == pi) otherend = 1; else otherend = 0;
			oni = ai->end[otherend].nodeinst;
			if (oni->temp1 == -1)
			{
				ai->temp1 = edge;
				oni->temp1 = edge+1;
				return(oni);
			}
			if (oni->temp1 == 0)
			{
				ai->temp1 = edge;
				oni->temp1 = edge+1;
				moved++;
			}
		}
	}
	if (moved == 0) return(NONODEINST);
	return(0);
}

/*
 * helper function for "le_gatherpath" which retraces the wave information
 * to construct the minimum path between two nodes.
 */
void le_unwind(NODEINST *start, NODEINST *prev)
{
	REGISTER NODEINST *oni;
	REGISTER PORTARCINST *pi;
	REGISTER ARCINST *ai;
	REGISTER INTBIG otherend;
	double capacitance;
	REGISTER INTBIG func;
	LENODE *le;

	/* add this node to the chain, if it can be analyzed */
	le = NOLENODE;
	func = nodefunction(start);
	switch (func)
	{
		case NPPIN:
		case NPCONTACT:
		case NPNODE:
		case NPCONNECT:
		case NPCONPOWER:
		case NPCONGROUND:
			break;
		case NPBUFFER:
		case NPGATEAND:
		case NPGATEOR:
		case NPGATEXOR:
			le = le_alloclenode();
			if (le == NOLENODE) break;
			le->ni = start;
			le->piin = le->piout = NOPORTARCINST;
			le->cin = 0.0;
			le->cout = le_lastcapacitance;
			le->type = le_getgatetype(start, &le->inputs);
			le->logeffort = le_getlogeffort(le);
			le->parasitics = le_getparasitics(le);
			le->numinputnodes = 0;
			le->numoutputnodes = 0;
			le_lastcapacitance = 1.0;
			le->nextlenode = le_firstlenode;
			le_firstlenode = le;
			le_lastlenode = le;
			break;
		default:
			ttyputmsg(_("Sorry, cannot analyze %s nodes"), nodefunctionname(func, start));
			break;
	}
	if (le != NOLENODE)
	{
		for(pi = start->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			ai = pi->conarcinst;
			if (ai->temp1 == 0) continue;
			if (ai->end[0].portarcinst == pi) otherend = 1; else otherend = 0;
			oni = ai->end[otherend].nodeinst;
			if (oni == prev) le->piout = pi;
			if (ai->temp1 == start->temp1 - 1 && oni->temp1 == start->temp1 - 1)
				le->piin = pi;
		}
	}

	/* mark and trace the path */
	start->temp2 = 1;
	if (start->temp1 == 1) return;
	for(pi = start->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		ai = pi->conarcinst;
		if (ai->temp1 == 0) continue;
		if (ai->end[0].portarcinst == pi) otherend = 1; else otherend = 0;
		oni = ai->end[otherend].nodeinst;
		if (ai->temp1 == start->temp1 - 1 && oni->temp1 == start->temp1 - 1)
		{
			ai->temp2 = 1;
			capacitance = le_getcapacitance(ai->network);
			if (capacitance != 0.0)
			{
				if (le_lastlenode != NOLENODE) le_lastlenode->cin = capacitance;
				le_lastcapacitance = capacitance;
			}
			le_unwind(oni, start);
			break;
		}
	}
}

/*
 * Routine to figure out the branching for the chain of LENODEs pointed to by
 * "le_firstlenode".  Loads the "branching" field of each LENODE.
 */
void le_figurebranching(void)
{
	REGISTER LENODE *le, *nextle;
	REGISTER ARCINST *ai;
	REGISTER PORTARCINST *pi;
	double onpath, offpath;

	for(le = le_firstlenode; le != NOLENODE; le = le->nextlenode)
	{
		nextle = le->nextlenode;
		le->branching = 1.0;
		if (nextle == NOLENODE) continue;
		if (le->piout == NOPORTARCINST) continue;

		/* reset indication of arcs that have been examined */
		for(ai = le->ni->parent->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			ai->temp1 = 0;

		/* find useful and stray capacitance in network coming from this LE node */
		onpath = 0.0;
		offpath = 0.0;
		for(pi = le->ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			if (pi->proto != le->piout->proto) continue;
			le_propagatebranch(pi, nextle, 1.0, &onpath, &offpath);
		}

		/* determine branching factor */
		if (onpath == 0.0) le->branching = 1.0; else
			le->branching = (onpath + offpath) / onpath;
	}
}

/*
 * Helper routine for "le_figurebranching" to propagate through the circuit, starting at
 * port "tpi" and gather capacitances into either "onpath" (if the path ends at the
 * desired "final" LENODE) or "offpath".  "capacitance" is the capacitance value seen
 * so far while following the path.
 */
void le_propagatebranch(PORTARCINST *tpi, LENODE *final, double capacitance,
	double *onpath, double *offpath)
{
	REGISTER ARCINST *ai;
	REGISTER PORTARCINST *pi;
	REGISTER INTBIG otherend;
	REGISTER INTBIG func;
	double cap;
	REGISTER NODEINST *oni;

	/* see if this arc has been examined */
	ai = tpi->conarcinst;
	if (ai->temp1 != 0) return;
	ai->temp1 = 1;

	/* gather any capacitance specification on this arc */
	cap = le_getcapacitance(ai->network);
	if (cap != 0.0) capacitance = cap;

	/* see if the other end of the arc is a terminal point */
	if (ai->end[0].portarcinst == tpi) otherend = 1; else otherend = 0;
	oni = ai->end[otherend].nodeinst;
	func = nodefunction(oni);
	if (func != NPPIN && func != NPCONTACT && func != NPNODE && func != NPCONNECT)
	{
		/* end of this chain: add capacitance into one of the path accumulators */
		if (oni == final->ni) *onpath += capacitance; else
			*offpath += capacitance;
		return;
	}

	/* propagate further */
	for(pi = oni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		le_propagatebranch(pi, final, capacitance, onpath, offpath);
	}
}

/******************** SUPPORT ********************/

/*
 * Routine to determine the type of object at "ni", returning the type and the number
 * of inputs.
 */
INTBIG le_getgatetype(NODEINST *ni, INTBIG *inputs)
{
	REGISTER INTBIG inputnegates, outputnegates, thisend, isinput, gatetype, func;
	CHAR *inputport, *outputport1, *outputport2;
	REGISTER ARCINST *ai;
	REGISTER PORTARCINST *pi;
	REGISTER VARIABLE *var;

	/* determine the number of inputs to the gate */
	*inputs = 0;
	inputnegates = outputnegates = 0;
	func = nodefunction(ni);
	if (func == NPUNKNOWN)
	{
		/* ignore if it is a capacitance element */
		var = getvalkey((INTBIG)ni, VNODEINST, -1, le_attrcapacitance_key);
		if (var == NOVARIABLE)
			ttyputmsg(_("Sorry, Logical Effort cannot handle cells"));
		return(NPUNKNOWN);
	}

	inputport = x_("a");   outputport1 = outputport2 = x_("y");
	if (func == NPTRANMOS || func == NPTRAPMOS)
	{
		inputport = x_("g");   outputport1 = x_("s");   outputport2 = x_("d");
	}
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		isinput = 0;
		if (estrcmp(pi->proto->protoname, inputport) == 0)
		{
			isinput = 1;
			(*inputs)++;
		}
		if (estrcmp(pi->proto->protoname, inputport) == 0 ||
			estrcmp(pi->proto->protoname, outputport1) == 0 ||
			estrcmp(pi->proto->protoname, outputport2) == 0)
		{
			if ((pi->conarcinst->userbits&ISNEGATED) != 0)
			{
				ai = pi->conarcinst;
				if (ai->end[0].portarcinst == pi) thisend = 0; else thisend = 1;
				if ((thisend == 0 && (ai->userbits&REVERSEEND) == 0) ||
					(thisend == 1 && (ai->userbits&REVERSEEND) != 0))
				{
					if (isinput != 0) inputnegates++; else outputnegates++;
				}
			}
		}
	}

	/* determine the type of the gate */
	gatetype = LEUNKNOWN;
	switch (func)
	{
		case NPBUFFER:
			if (inputnegates+outputnegates == 0)
			{
				ttyputmsg(_("Cannot handle BUFFER gates, only INVERTER"));
			} else if (inputnegates+outputnegates != 1)
			{
				ttyputmsg(_("Cannot handle INVERTER gates that invert more than once"));
			} else gatetype = LEINVERTER;
			break;

		case NPGATEAND:
			if (inputnegates != 0)
			{
				ttyputmsg(_("Cannot handle AND gates with inverted inputs"));
			} else if (outputnegates == 0)
			{
				ttyputmsg(_("Cannot handle AND gates, only NAND"));
			} else gatetype = LENAND;
			break;

		case NPGATEOR:
			if (inputnegates != 0)
			{
				ttyputmsg(_("Cannot handle OR gates with inverted inputs"));
			} else if (outputnegates == 0)
			{
				ttyputmsg(_("Cannot handle OR gates, only NOR"));
			} else gatetype = LENOR;
			break;

		case NPGATEXOR:
			if (inputnegates != 0)
			{
				ttyputmsg(_("Cannot handle XOR gates with inverted inputs"));
			} else
			{
				if (outputnegates != 0) gatetype = LEXNOR; else
					gatetype = LEXOR;
			}
			break;

		case NPTRANMOS:
			if (inputnegates != 0 || outputnegates != 0)
			{
				ttyputmsg(_("Cannot negate inputs to transistors"));
			} else gatetype = LENMOS;
			break;

		case NPTRAPMOS:
			if (inputnegates != 0 || outputnegates != 0)
			{
				ttyputmsg(_("Cannot negate inputs to transistors"));
			} else gatetype = LEPMOS;
			break;

		case NPCONPOWER:
			if (inputnegates != 0 || outputnegates != 0)
			{
				ttyputmsg(_("Cannot negate inputs to power"));
			} else gatetype = LEPOWER;
			break;

		case NPCONGROUND:
			if (inputnegates != 0 || outputnegates != 0)
			{
				ttyputmsg(_("Cannot negate inputs to ground"));
			} else gatetype = LEGROUND;
			break;

		case NPPIN:
		case NPCONTACT:
		case NPNODE:
		case NPCONNECT:
			break;

		default:
			ttyputmsg(_("Sorry, Logical Effort cannot handle %s nodes"), nodefunctionname(func, ni));
			break;
	}

	return(gatetype);
}

/*
 * Routine to return a string describing LENODE "le".
 */
CHAR *le_describenode(LENODE *le)
{
	REGISTER VARIABLE *var;
	REGISTER void *infstr;

	infstr = initinfstr();
	switch (le->type)
	{
		case LEINVERTER:
			addstringtoinfstr(infstr, _("inverter"));
			break;
		case LENAND:
			addstringtoinfstr(infstr, _("nand"));
			break;
		case LENOR:
			addstringtoinfstr(infstr, _("nor"));
			break;
		case LEXOR:
			addstringtoinfstr(infstr, _("xor"));
			break;
		case LEXNOR:
			addstringtoinfstr(infstr, _("xnor"));
			break;
		case LENMOS:
			addstringtoinfstr(infstr, _("nMOS"));
			break;
		case LEPMOS:
			addstringtoinfstr(infstr, _("pMOS"));
			break;
		default:
			addstringtoinfstr(infstr, describenodeproto(le->ni->proto));
			break;
	}
	var = getvalkey((INTBIG)le->ni, VNODEINST, VSTRING, el_node_name_key);
	if (var != NOVARIABLE)
	{
		addstringtoinfstr(infstr, x_("["));
		addstringtoinfstr(infstr, (CHAR *)var->addr);
		addstringtoinfstr(infstr, x_("]"));
	}
	return(returninfstr(infstr));
}

/*
 * Routine to determine the logical effort of the gate in "le".  Uses the table
 * from page 7 of Sutherland's book.
 */
double le_getlogeffort(LENODE *le)
{
	REGISTER VARIABLE *var;
	REGISTER CHAR *pt;

	/* see if there is an override on the node */
	var = getvalkey((INTBIG)le->ni, VNODEINST, VSTRING, le_nodeeffort_key);
	if (var != NOVARIABLE)
	{
		pt = (CHAR *)var->addr;
		while (*pt == ' ' || *pt == '\t') pt++;
		if (*pt == 'g' || *pt == 'G')
		{
			pt++;
			while (*pt == ' ' || *pt == '\t') pt++;
		}
		if (*pt == '=')
		{
			pt++;
			while (*pt == ' ' || *pt == '\t') pt++;
		}
		return(eatof(pt));
	}

	/* calculate the logical effort */
	switch (le->type)
	{
		case LEINVERTER: return(1.0);
		case LENAND:     return((le->inputs+2.0) / 3.0);
		case LENOR:      return((2.0*le->inputs+1.0) / 3.0);
		case LEXOR:
		case LEXNOR:
			switch (le->inputs)
			{
				case 2: return(4.0);
				case 3: return(12.0);
				case 4: return(32.0);
			}
			ttyputmsg(_("Cannot handle %ld-input XOR gates"), le->inputs);
			return(0.0);
		case LENMOS:     return(1.0/3.0);
		case LEPMOS:     return(2.0/3.0);
	}
	ttyputmsg(_("Sorry, node %s has unknown type"), describenodeinst(le->ni));
	return(0.0);
}

/*
 * Routine to determine the parasitics of the gate in "le".  Uses the table
 * from page 10 of Sutherland's book.
 */
double le_getparasitics(LENODE *le)
{
	switch (le->type)
	{
		case LEINVERTER: return(1.0);
		case LENAND:     return((double)le->inputs);
		case LENOR:      return((double)le->inputs);
		case LEXOR:      return(4.0);
		case LEXNOR:     return(4.0);
		case LENMOS:     return(1.0);		/* probably not right!!! */
		case LEPMOS:     return(1.0);		/* probably not right!!! */
	}
	ttyputmsg(_("Sorry, node %s has unknown type"), describenodeinst(le->ni));
	return(0.0);
}

/*
 * Routine to extract the capacitance value that is stored on network "net".
 * Returns zero if no value is stored there.
 */
double le_getcapacitance(NETWORK *net)
{
	REGISTER VARIABLE *var;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER PORTARCINST *pi;

	np = net->parent;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* see if the node is connected to this arc */
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			if (pi->conarcinst->network == net) break;
		if (pi == NOPORTARCINST) continue;

		/* see if there is capacitance on this node */
		var = getvalkey((INTBIG)ni, VNODEINST, -1, le_attrcapacitance_key);
		if (var != NOVARIABLE)
		{
			return(eatof(describesimplevariable(var)));
		}
	}
	return(0.0);
}

/*
 * Routine to free the global array of LE nodes (headed by "le_firstlenode").
 */
void le_freealllenodes(void)
{
	LENODE *nextle, *le;

	for(le = le_firstlenode; le != NOLENODE; le = nextle)
	{
		nextle = le->nextlenode;
		le_freelenode(le);
	}
	le_firstlenode = NOLENODE;
}

/*
 * Routine to allocate a new LENODE (either from the pool of unused ones
 * or from memory).
 */
LENODE *le_alloclenode(void)
{
	LENODE *le;

	if (le_lenodefree != NOLENODE)
	{
		le = le_lenodefree;
		le_lenodefree = le->nextlenode;
	} else
	{
		le = (LENODE *)emalloc(sizeof (LENODE), le_tool->cluster);
		if (le == 0) return(NOLENODE);
	}
	return(le);
}

/*
 * Routine to return LENODE "le" to the pool of unused ones.
 */
void le_freelenode(LENODE *le)
{
	if (le->numinputnodes > 0)
	{
		efree((CHAR *)le->inputnodes);
		efree((CHAR *)le->inputports);
	}
	if (le->numoutputnodes > 0)
	{
		efree((CHAR *)le->outputnodes);
		efree((CHAR *)le->outputports);
	}
	le->nextlenode = le_lenodefree;
	le_lenodefree = le;
}

#endif  /* LOGEFFTOOL - at top */
