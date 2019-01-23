/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: simfasthenry.cpp
 * Simulation tool: FastHenry output
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
#if SIMTOOL

#include "global.h"
#include "sim.h"
#include "usr.h"
#include "edialogs.h"


/* keys to cached variables */
INTBIG sim_fasthenrystatekey = 0;			/* variable key for "SIM_fasthenry_state" */
INTBIG sim_fasthenryfreqstartkey = 0;		/* variable key for "SIM_fasthenry_freqstart" */
INTBIG sim_fasthenryfreqendkey = 0;			/* variable key for "SIM_fasthenry_freqend" */
INTBIG sim_fasthenryrunsperdecadekey = 0;	/* variable key for "SIM_fasthenry_runsperdecade" */
INTBIG sim_fasthenrynumpoleskey = 0;		/* variable key for "SIM_fasthenry_numpoles" */
INTBIG sim_fasthenryseglimitkey = 0;		/* variable key for "SIM_fasthenry_seglimit" */
INTBIG sim_fasthenrythicknesskey = 0;		/* variable key for "SIM_fasthenry_thickness" */
INTBIG sim_fasthenrywidthsubdivkey = 0;		/* variable key for "SIM_fasthenry_width_subdivs" */
INTBIG sim_fasthenryheightsubdivkey = 0;	/* variable key for "SIM_fasthenry_height_subdivs" */

INTBIG sim_fasthenryzheadkey = 0;			/* variable key for "SIM_fasthenry_z_head" */
INTBIG sim_fasthenryztailkey = 0;			/* variable key for "SIM_fasthenry_z_tail" */
INTBIG sim_fasthenrygroupnamekey = 0;		/* variable key for "SIM_fasthenry_group_name" */

/* prototypes for local routines */
static BOOLEAN sim_writefhcell(NODEPROTO *np, FILE *io);
static void    sim_fasthenryarcdlog(void);
static void    sim_fasthenrydlog(void);
static void    sim_fasthenrygetoptions(INTBIG *options, float *startfreq, float *endfreq,
				INTBIG *runsperdecade, INTBIG *numpoles, INTBIG *seglenlimit, INTBIG *thickness,
				INTBIG *widsubdiv, INTBIG *heisubdiv);
static CHAR   *sim_fasthenrygetarcoptions(ARCINST *ai, INTBIG *thickness, INTBIG *widsubdiv,
				INTBIG *heisubdiv, INTBIG *z_head, INTBIG *z_tail, BOOLEAN *zhover, BOOLEAN *ztover,
				INTBIG *defz);
static PORTPROTO *sim_fasthenryfindotherport(ARCINST *ai, INTBIG end);

extern "C" { extern COMCOMP sim_fhp, sim_fhap; }

/*
 * Routine called once to initialize this module
 */
void sim_fasthenryinit(void)
{
	DiaDeclareHook(x_("fasthenry"), &sim_fhp, sim_fasthenrydlog);
	DiaDeclareHook(x_("fasthenryarc"), &sim_fhap, sim_fasthenryarcdlog);
}

/*
 * routine to write a ".sil" file from the cell "np"
 */
void sim_writefasthenrynetlist(NODEPROTO *np)
{
	CHAR name[100], *truename, txtpoles[20], *path;
	FILE *io;
	float startfreq, endfreq;
	INTBIG options, runsperdecade, numpoles, seglenlimit, thickness, widsubdiv, heisubdiv,
		lambda;

	/* make sure network tool is on */
	if ((net_tool->toolstate&TOOLON) == 0)
	{
		ttyputerr(_("Network tool must be running...turning it on"));
		toolturnon(net_tool);
		ttyputerr(_("...now reissue the simulation command"));
		return;
	}

	/* get all parameters */
	sim_fasthenrygetoptions(&options, &startfreq, &endfreq, &runsperdecade, &numpoles,
		&seglenlimit, &thickness, &widsubdiv, &heisubdiv);

	/* first write the "inp" file */
	(void)estrcpy(name, np->protoname);
	(void)estrcat(name, x_(".inp"));
	io = xcreate(name, sim_filetypefasthenry, _("FastHenry File"), &truename);
	if (io == NULL)
	{
		if (truename != 0) ttyputerr(_("Cannot write %s"), truename);
		return;
	}
	xprintf(io, x_("* FastHenry for cell %s\n"), describenodeproto(np));
	us_emitcopyright(io, x_("* "), x_(""));

	if ((us_useroptions&NODATEORVERSION) == 0)
	{
		if (np->creationdate != 0)
			xprintf(io, x_("* Cell created on %s\n"), timetostring((time_t)np->creationdate));
		if (np->revisiondate != 0)
			xprintf(io, x_("* Cell last modified on %s\n"), timetostring((time_t)np->revisiondate));
		xprintf(io, x_("* Netlist written on %s\n"), timetostring(getcurrenttime()));
		xprintf(io, x_("* Written by Electric VLSI Design System, version %s\n"), el_version);
	} else
	{
		xprintf(io, x_("* Written by Electric VLSI Design System\n"));
	}

	xprintf(io, x_("\n* Units are microns\n"));
	xprintf(io, x_(".units um\n"));

	/* write default width and height subdivisions */
	lambda = lambdaofcell(np);
	xprintf(io, x_("\n* Default number of subdivisions\n"));
	xprintf(io, x_(".Default nwinc=%ld nhinc=%ld h=%s\n"), widsubdiv, heisubdiv, latoa(thickness, lambda));

	/* reset flags for cells that have been written */
	if (sim_writefhcell(np, io))
		ttyputmsg(x_("Back-annotation information has been added (library must be saved)"));

	/* write frequency range */
	if ((options&FHUSESINGLEFREQ) == 0)
	{
		xprintf(io, x_("\n.freq fmin=%g fmax=%g ndec=%ld\n"), startfreq, endfreq, runsperdecade);
	} else
	{
		xprintf(io, x_("\n.freq fmin=%g fmax=%g ndec=1\n"), startfreq, startfreq);
	}

	/* clean up */
	xprintf(io, x_("\n.end\n"));
	xclose(io);
	ttyputmsg(_("%s written"), truename);

	/* generate invocation for fasthenry */
	EProcess fh_process;
	path = egetenv(x_("ELECTRIC_FASTHENRYLOC"));
	if (path == NULL) path = FASTHENRYLOC;
	fh_process.addArgument( path );
	if ((options&FHMAKEMULTIPOLECKT) != 0)
	{
		fh_process.addArgument( x_("-r") );
		esnprintf(txtpoles, 20, x_("%ld"), numpoles);
		fh_process.addArgument( txtpoles );
		fh_process.addArgument( x_("-M") );
	}
	fh_process.addArgument( truename );
	fh_process.setCommunication( FALSE, FALSE, FALSE );
	fh_process.start();
	/* No wait */
}

/*
 * recursively called routine to print the SILOS description of cell "np".
 * The description is treated as the top-level cell if "top" is nonzero
 * np is the current nodeproto
 */
BOOLEAN sim_writefhcell(NODEPROTO *np, FILE *io)
{
	REGISTER BOOLEAN backannotate, found;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER PORTARCINST *pi;
	REGISTER PORTPROTO *pp, *opp;
	REGISTER INTBIG nodezval, zval, wid, thatend;
	float xf, yf, zf, wf, hf;
	CHAR *nname, *n1name, *n2name, *groupname;
	static POLYGON *poly = NOPOLYGON;
	REGISTER VARIABLE *var;
	float startfreq, endfreq;
	INTBIG options, runsperdecade, numpoles, seglenlimit, thickness, defthickness,
		widsubdiv, defwidsubdiv, heisubdiv, defheisubdiv, z_head, z_tail, defz;
	BOOLEAN zhover, ztover;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, sim_tool->cluster);

	/* stop if requested */
	if (el_pleasestop != 0)
	{
		stopping(STOPREASONDECK);
		return(FALSE);
	}

	/* get overriding defaults */
	sim_fasthenrygetoptions(&options, &startfreq, &endfreq, &runsperdecade, &numpoles,
		&seglenlimit, &defthickness, &defwidsubdiv, &defheisubdiv);

	/* make sure that all nodes have names on them */
	backannotate = FALSE;
	if (asktool(net_tool, x_("name-all-nodes"), (INTBIG)np) != 0) backannotate = TRUE;

	/* look at every node in the cell */
	xprintf(io, x_("\n* Traces\n"));
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* see if this node has a FastHenry arc on it */
		found = FALSE;
		nodezval = 0;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			ai = pi->conarcinst;
			groupname = sim_fasthenrygetarcoptions(ai, &thickness, &widsubdiv, &heisubdiv,
				&z_head, &z_tail, &zhover, &ztover, &defz);
			if (groupname == 0) continue;
			zval = defz;
			if (ai->end[0].portarcinst == pi && zhover) zval = z_head;
			if (ai->end[1].portarcinst == pi && ztover) zval = z_tail;
			if (found)
			{
				/* LINTED "nodezval" used in proper order */
				if (zval != nodezval)
					ttyputerr(_("Warning: inconsistent z value at node %s"),
						describenodeinst(ni));
			}
			nodezval = zval;
			found = TRUE;
		}
		if (!found) continue;

		/* node is an end point: get its name */
		if (ni->firstportexpinst != NOPORTEXPINST)
		{
			nname = ni->firstportexpinst->exportproto->protoname;
		} else
		{
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
			if (var == NOVARIABLE) nname = x_(""); else nname = (CHAR *)var->addr;
		}

		/* write the "N" line */
		xf = scaletodispunit((ni->lowx+ni->highx)/2, DISPUNITMIC);
		yf = scaletodispunit((ni->lowy+ni->highy)/2, DISPUNITMIC);
		zf = scaletodispunit(nodezval, DISPUNITMIC);
		xprintf(io, x_("N_%s x=%g y=%g z=%g\n"), nname, xf, yf, zf);
	}

	/* look at every arc in the cell */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		/* get info about this arc, stop if not part of the FastHenry output */
		groupname = sim_fasthenrygetarcoptions(ai, &thickness, &widsubdiv, &heisubdiv,
			&z_head, &z_tail, &zhover, &ztover, &defz);
		if (groupname == 0) continue;

		/* get size */
		wid = ai->width;

		/* get the name of the nodes on each end */
		if (ai->end[0].nodeinst->firstportexpinst != NOPORTEXPINST)
		{
			n1name = ai->end[0].nodeinst->firstportexpinst->exportproto->protoname;
		} else
		{
			var = getvalkey((INTBIG)ai->end[0].nodeinst, VNODEINST, VSTRING, el_node_name_key);
			if (var == NOVARIABLE) n1name = x_(""); else n1name = (CHAR *)var->addr;
		}
		if (ai->end[1].nodeinst->firstportexpinst != NOPORTEXPINST)
		{
			n2name = ai->end[1].nodeinst->firstportexpinst->exportproto->protoname;
		} else
		{
			var = getvalkey((INTBIG)ai->end[1].nodeinst, VNODEINST, VSTRING, el_node_name_key);
			if (var == NOVARIABLE) n2name = x_(""); else n2name = (CHAR *)var->addr;
		}

		/* write the "E" line */
		wf = scaletodispunit(wid, DISPUNITMIC);
		xprintf(io, x_("E_%s_%s N_%s N_%s w=%g"), n1name, n2name, n1name, n2name, wf);
		if (thickness > 0)
		{
			hf = scaletodispunit(thickness, DISPUNITMIC);
			xprintf(io, x_(" h=%g"), hf);
		}
		if (widsubdiv > 0) xprintf(io, x_(" nwinc=%ld"), widsubdiv);
		if (heisubdiv > 0) xprintf(io, x_(" nhinc=%ld"), heisubdiv);
		xprintf(io, x_("\n"));
	}

	/* find external connections */
	xprintf(io, x_("\n* External connections\n"));
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		ai->temp1 = 0;
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		pp->temp1 = 0;

	/* look at every export in the cell */
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		if (pp->temp1 != 0) continue;
		pp->temp1 = 1;
		ni = pp->subnodeinst;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			ai = pi->conarcinst;
			var = getvalkey((INTBIG)ai, VARCINST, VSTRING, sim_fasthenrygroupnamekey);
			if (var != NOVARIABLE) break;
		}
		if (pi == NOPORTARCINST) continue;

		/* port "pp" is one end, now find the other */
		if (ai->end[0].portarcinst == pi) thatend = 1; else thatend = 0;
		opp = sim_fasthenryfindotherport(ai, thatend);
		if (opp == NOPORTPROTO)
		{
			ttyputerr(_("Warning: trace on port %s has no other end that is an export"),
				pp->protoname);
			continue;
		}

		/* found two ports: write the ".external" line */
		opp->temp1 = 1;
		xprintf(io, x_(".external N_%s N_%s\n"), pp->protoname, opp->protoname);
	}

	/* warn about arcs that aren't connected to ".external" lines */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->temp1 != 0) continue;
		var = getvalkey((INTBIG)ai, VARCINST, VSTRING, sim_fasthenrygroupnamekey);
		if (var == NOVARIABLE) continue;
		ttyputerr(_("Warning: arc %s is not on a measured trace"), describearcinst(ai));
	}
	return(backannotate);
}

PORTPROTO *sim_fasthenryfindotherport(ARCINST *ai, INTBIG end)
{
	REGISTER NODEINST *ni;
	REGISTER PORTARCINST *pi;
	REGISTER ARCINST *oai;
	REGISTER INTBIG thatend;
	REGISTER PORTPROTO *opp;
	REGISTER VARIABLE *var;

	ai->temp1 = 1;
	ni = ai->end[end].nodeinst;
	if (ni->firstportexpinst != NOPORTEXPINST) return(ni->firstportexpinst->exportproto);
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		oai = pi->conarcinst;
		if (oai == ai) continue;
		var = getvalkey((INTBIG)ai, VARCINST, VSTRING, sim_fasthenrygroupnamekey);
		if (var == NOVARIABLE) continue;
		if (oai->end[0].portarcinst == pi) thatend = 1; else thatend = 0;
		opp = sim_fasthenryfindotherport(oai, thatend);
		if (opp != NOPORTPROTO) return(opp);
	}
	return(NOPORTPROTO);
}

void sim_fasthenrygetoptions(INTBIG *options, float *startfreq, float *endfreq,
	INTBIG *runsperdecade, INTBIG *numpoles, INTBIG *seglenlimit, INTBIG *thickness,
	INTBIG *widsubdiv, INTBIG *heisubdiv)
{
	REGISTER VARIABLE *var;

	var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_fasthenrystatekey);
	if (var != NOVARIABLE) *options = var->addr; else *options = 0;
	var = getvalkey((INTBIG)sim_tool, VTOOL, VFLOAT, sim_fasthenryfreqstartkey);
	if (var != NOVARIABLE) *startfreq = castfloat(var->addr); else *startfreq = 0.0;
	var = getvalkey((INTBIG)sim_tool, VTOOL, VFLOAT, sim_fasthenryfreqendkey);
	if (var != NOVARIABLE) *endfreq = castfloat(var->addr); else *endfreq = 0.0;
	var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_fasthenryrunsperdecadekey);
	if (var != NOVARIABLE) *runsperdecade = var->addr; else *runsperdecade = 1;
	var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_fasthenrynumpoleskey);
	if (var != NOVARIABLE) *numpoles = var->addr; else *numpoles = 20;
	var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_fasthenryseglimitkey);
	if (var != NOVARIABLE) *seglenlimit = var->addr; else *seglenlimit = 0;
	var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_fasthenrythicknesskey);
	if (var != NOVARIABLE) *thickness = var->addr; else *thickness = 2*el_curlib->lambda[el_curtech->techindex];
	var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_fasthenrywidthsubdivkey);
	if (var != NOVARIABLE) *widsubdiv = var->addr; else *widsubdiv = 1;
	var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_fasthenryheightsubdivkey);
	if (var != NOVARIABLE) *heisubdiv = var->addr; else *heisubdiv = 1;
}

CHAR *sim_fasthenrygetarcoptions(ARCINST *ai, INTBIG *thickness, INTBIG *widsubdiv,
	INTBIG *heisubdiv, INTBIG *z_head, INTBIG *z_tail, BOOLEAN *zhover, BOOLEAN *ztover,
	INTBIG *defz)
{
	REGISTER VARIABLE *var;
	REGISTER INTBIG total, i;
	float lheight, lthickness;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, sim_tool->cluster);

	/* get miscellaneous parameters */
	var = getvalkey((INTBIG)ai, VARCINST, VINTEGER, sim_fasthenrythicknesskey);
	if (var != NOVARIABLE) *thickness = var->addr; else *thickness = -1;
	var = getvalkey((INTBIG)ai, VARCINST, VINTEGER, sim_fasthenrywidthsubdivkey);
	if (var != NOVARIABLE) *widsubdiv = var->addr; else *widsubdiv = -1;
	var = getvalkey((INTBIG)ai, VARCINST, VINTEGER, sim_fasthenryheightsubdivkey);
	if (var != NOVARIABLE) *heisubdiv = var->addr; else *heisubdiv = -1;

	/* get z depth and any overrides */
	total = arcpolys(ai, NOWINDOWPART);
	for(i=0; i<total; i++)
	{
		shapearcpoly(ai, i, poly);
		if (get3dfactors(ai->proto->tech, poly->layer, &lheight, &lthickness))
			continue;
		*defz = (INTBIG)(lheight * lambdaofarc(ai));
		break;
	}
	var = getvalkey((INTBIG)ai, VARCINST, VINTEGER, sim_fasthenryzheadkey);
	if (var == NOVARIABLE) *zhover = FALSE; else
	{
		*z_head = var->addr;
		*zhover = TRUE;
	}
	var = getvalkey((INTBIG)ai, VARCINST, VINTEGER, sim_fasthenryztailkey);
	if (var == NOVARIABLE) *ztover = FALSE; else
	{
		*z_tail = var->addr;
		*ztover = TRUE;
	}		

	/* get the group name */
	var = getvalkey((INTBIG)ai, VARCINST, VSTRING, sim_fasthenrygroupnamekey);
	if (var == NOVARIABLE) return(0);
	return((CHAR *)var->addr);
}

/***************************************** DIALOGS *****************************************/

/* Simulation: FastHenry Options */
static DIALOGITEM sim_fasthenrydialogitems[] =
{
 /*  1 */ {0, {164,392,188,472}, BUTTON, N_("OK")},
 /*  2 */ {0, {164,12,188,92}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,20,48,140}, MESSAGE, N_("Frequency start:")},
 /*  4 */ {0, {32,144,48,200}, EDITTEXT, x_("")},
 /*  5 */ {0, {56,20,72,140}, MESSAGE, N_("Frequency end:")},
 /*  6 */ {0, {56,144,72,200}, EDITTEXT, x_("")},
 /*  7 */ {0, {80,20,96,140}, MESSAGE, N_("Runs per decade:")},
 /*  8 */ {0, {80,144,96,200}, EDITTEXT, x_("")},
 /*  9 */ {0, {8,8,24,200}, CHECK, N_("Use single frequency")},
 /* 10 */ {0, {128,20,144,140}, MESSAGE, N_("Number of poles:")},
 /* 11 */ {0, {128,144,144,200}, EDITTEXT, x_("")},
 /* 12 */ {0, {104,8,120,200}, CHECK, N_("Make multipole subcircuit")},
 /* 13 */ {0, {104,224,120,420}, CHECK, N_("Make PostScript view")},
 /* 14 */ {0, {80,224,96,420}, MESSAGE, N_("Maximum segment length:")},
 /* 15 */ {0, {80,424,96,480}, EDITTEXT, x_("")},
 /* 16 */ {0, {32,224,48,420}, MESSAGE, N_("Default width subdivisions:")},
 /* 17 */ {0, {32,424,48,480}, EDITTEXT, x_("")},
 /* 18 */ {0, {56,224,72,420}, MESSAGE, N_("Default height subdivisions:")},
 /* 19 */ {0, {56,424,72,480}, EDITTEXT, x_("")},
 /* 20 */ {0, {128,224,144,420}, CHECK, N_("Make SPICE subcircuit")},
 /* 21 */ {0, {8,224,24,420}, MESSAGE, N_("Default thickness:")},
 /* 22 */ {0, {8,424,24,480}, EDITTEXT, x_("")},
 /* 23 */ {0, {176,140,192,344}, POPUP, x_("")},
 /* 24 */ {0, {156,176,172,320}, MESSAGE, N_("After writing deck:")}
};
static DIALOG sim_fasthenrydialog = {{75,75,276,565}, N_("FastHenry Options"), 0, 24, sim_fasthenrydialogitems, 0, 0};

/* special items for the "FastHenry options" dialog: */
#define DFHO_FREQSTART      4		/* frequency start (edit text) */
#define DFHO_FREQEND_L      5		/* frequency end label (stat text) */
#define DFHO_FREQEND        6		/* frequency end (edit text) */
#define DFHO_RUNSPERDEC_L   7		/* runs per decade label (stat text) */
#define DFHO_RUNSPERDEC     8		/* runs per decade (edit text) */
#define DFHO_USESINGLEFREQ  9		/* use single frequency (check) */
#define DFHO_NUMPOLES_L    10		/* number of poles label (stat text) */
#define DFHO_NUMPOLES      11		/* number of poles (edit text) */
#define DFHO_MULTIPOLE     12		/* make multipole subckt (check) */
#define DFHO_POSTSCRIPT    13		/* make PostScript view (check) */
#define DFHO_MAXSEGLEN     15		/* max seg length (edit text) */
#define DFHO_DEFWIDSUB     17		/* default width subdiv (edit text) */
#define DFHO_DEFHEISUB     19		/* default height subdiv (edit text) */
#define DFHO_MAKESPICE     20		/* make SPICE subckt (check) */
#define DFHO_DEFTHICK      22		/* default thickness (edit text) */
#define DFHO_AFTERWRITE    23		/* after deck writing (popup) */

void sim_fasthenrydlog(void)
{
	REGISTER INTBIG itemHit, value, i;
	REGISTER BOOLEAN canexecute;
	float fvalue;
	CHAR line[30], *newlang[5];
	static CHAR *exechoices[] = {N_("Nothing run"), N_("Run FastHenry"),
		N_("Run FastHenry Multiprocessing")};
	float startfreq, endfreq;
	INTBIG options, runsperdecade, numpoles, seglenlimit, thickness,
		widsubdiv, heisubdiv;
	REGISTER void *dia;

	/* get all parameters */
	sim_fasthenrygetoptions(&options, &startfreq, &endfreq, &runsperdecade, &numpoles,
		&seglenlimit, &thickness, &widsubdiv, &heisubdiv);

	/* Display the FastHenry options dialog box */
	dia = DiaInitDialog(&sim_fasthenrydialog);
	if (dia == 0) return;

	/* set popup */
	for(i=0; i<3; i++) newlang[i] = TRANSLATE(exechoices[i]);
	DiaSetPopup(dia, DFHO_AFTERWRITE, 3, newlang);
	canexecute = graphicshas(CANRUNPROCESS);
	if (canexecute)
	{
		DiaUnDimItem(dia, DFHO_AFTERWRITE);
		if ((options&FHEXECUTETYPE) == FHEXECUTERUNFH)
		{
			DiaSetPopupEntry(dia, DFHO_AFTERWRITE, 1);
		} else if ((options&FHEXECUTETYPE) == FHEXECUTERUNFHMUL)
		{
			DiaSetPopupEntry(dia, DFHO_AFTERWRITE, 2);
		}
	} else
	{
		DiaDimItem(dia, DFHO_AFTERWRITE);
	}

	/* set checkboxes */
	if ((options&FHUSESINGLEFREQ) != 0) DiaSetControl(dia, DFHO_USESINGLEFREQ, 1);
	if ((options&FHMAKEMULTIPOLECKT) != 0) DiaSetControl(dia, DFHO_MULTIPOLE, 1);
	if ((options&FHMAKEPOSTSCRIPTVIEW) != 0) DiaSetControl(dia, DFHO_POSTSCRIPT, 1);
	if ((options&FHMAKESPICESUBCKT) != 0) DiaSetControl(dia, DFHO_MAKESPICE, 1);

	/* load default frequency range */
	esnprintf(line, 30, x_("%g"), startfreq);   DiaSetText(dia, DFHO_FREQSTART, line);
	esnprintf(line, 30, x_("%g"), endfreq);   DiaSetText(dia, DFHO_FREQEND, line);
	esnprintf(line, 30, x_("%ld"), runsperdecade);   DiaSetText(dia, DFHO_RUNSPERDEC, line);

	/* load segment limits */
	DiaSetText(dia, DFHO_MAXSEGLEN, latoa(seglenlimit, 0));
	DiaSetText(dia, DFHO_DEFTHICK, latoa(thickness, 0));
	esnprintf(line, 30, x_("%ld"), widsubdiv);   DiaSetText(dia, DFHO_DEFWIDSUB, line);
	esnprintf(line, 30, x_("%ld"), heisubdiv);   DiaSetText(dia, DFHO_DEFHEISUB, line);

	/* load other numeric options */
	esnprintf(line, 30, x_("%ld"), numpoles);   DiaSetText(dia, DFHO_NUMPOLES, line);

	if ((options&FHMAKEMULTIPOLECKT) != 0)
	{
		DiaUnDimItem(dia, DFHO_NUMPOLES_L);
		DiaEditControl(dia, DFHO_NUMPOLES);
	} else
	{
		DiaDimItem(dia, DFHO_NUMPOLES_L);
		DiaNoEditControl(dia, DFHO_NUMPOLES);
	}
	if ((options&FHUSESINGLEFREQ) != 0)
	{
		DiaDimItem(dia, DFHO_FREQEND_L);
		DiaNoEditControl(dia, DFHO_FREQEND);
		DiaDimItem(dia, DFHO_RUNSPERDEC_L);
		DiaNoEditControl(dia, DFHO_RUNSPERDEC);
	} else
	{
		DiaUnDimItem(dia, DFHO_FREQEND_L);
		DiaEditControl(dia, DFHO_FREQEND);
		DiaUnDimItem(dia, DFHO_RUNSPERDEC_L);
		DiaEditControl(dia, DFHO_RUNSPERDEC);
	}
	DiaDimItem(dia, DFHO_POSTSCRIPT);
	DiaNoEditControl(dia, DFHO_MAXSEGLEN);
	DiaDimItem(dia, DFHO_MAKESPICE);

	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DFHO_USESINGLEFREQ || itemHit == DFHO_MULTIPOLE ||
			itemHit == DFHO_POSTSCRIPT || itemHit == DFHO_MAKESPICE)
		{
			value = 1 - DiaGetControl(dia, itemHit);
			DiaSetControl(dia, itemHit, value);
			if (itemHit == DFHO_USESINGLEFREQ)
			{
				if (value != 0)
				{
					DiaDimItem(dia, DFHO_FREQEND_L);
					DiaNoEditControl(dia, DFHO_FREQEND);
					DiaDimItem(dia, DFHO_RUNSPERDEC_L);
					DiaNoEditControl(dia, DFHO_RUNSPERDEC);
				} else
				{
					DiaUnDimItem(dia, DFHO_FREQEND_L);
					DiaEditControl(dia, DFHO_FREQEND);
					DiaUnDimItem(dia, DFHO_RUNSPERDEC_L);
					DiaEditControl(dia, DFHO_RUNSPERDEC);
				}
			}
			if (itemHit == DFHO_MULTIPOLE)
			{
				if (value != 0)
				{
					DiaUnDimItem(dia, DFHO_NUMPOLES_L);
					DiaEditControl(dia, DFHO_NUMPOLES);
				} else
				{
					DiaDimItem(dia, DFHO_NUMPOLES_L);
					DiaNoEditControl(dia, DFHO_NUMPOLES);
				}
			}
			continue;
		}
	}

	if (itemHit == OK)
	{
		/* save options */
		value = 0;
		if (DiaGetControl(dia, DFHO_USESINGLEFREQ) != 0) value |= FHUSESINGLEFREQ;
		if (DiaGetControl(dia, DFHO_MULTIPOLE) != 0) value |= FHMAKEMULTIPOLECKT;
		if (DiaGetControl(dia, DFHO_POSTSCRIPT) != 0) value |= FHMAKEPOSTSCRIPTVIEW;
		if (DiaGetControl(dia, DFHO_MAKESPICE) != 0) value |= FHMAKESPICESUBCKT;
		i = DiaGetPopupEntry(dia, DFHO_AFTERWRITE);
		if (i == 1) value |= FHEXECUTERUNFH; else
			if (i == 2) value |= FHEXECUTERUNFHMUL;
		if (value != options)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenrystatekey, value, VINTEGER);

		/* save other limits */
		fvalue = (float)eatof(DiaGetText(dia, DFHO_FREQSTART));
		if (fvalue != startfreq)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenryfreqstartkey, castint(fvalue), VFLOAT);
		fvalue = (float)eatof(DiaGetText(dia, DFHO_FREQEND));
		if (fvalue != endfreq)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenryfreqendkey, castint(fvalue), VFLOAT);
		value = eatoi(DiaGetText(dia, DFHO_RUNSPERDEC));
		if (value != runsperdecade)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenryrunsperdecadekey, value, VINTEGER);
		value = atola(DiaGetText(dia, DFHO_MAXSEGLEN), 0);
		if (value != seglenlimit)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenryseglimitkey, value, VINTEGER);
		value = atola(DiaGetText(dia, DFHO_DEFTHICK), 0);
		if (value != thickness)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenrythicknesskey, value, VINTEGER);
		value = eatoi(DiaGetText(dia, DFHO_DEFWIDSUB));
		if (value != widsubdiv)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenrywidthsubdivkey, value, VINTEGER);
		value = eatoi(DiaGetText(dia, DFHO_DEFHEISUB));
		if (value != heisubdiv)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenryheightsubdivkey, value, VINTEGER);
		value = eatoi(DiaGetText(dia, DFHO_NUMPOLES));
		if (value != numpoles)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenrynumpoleskey, value, VINTEGER);
	}
	DiaDoneDialog(dia);
}

/* Simulation: FastHenry Arc */
static DIALOGITEM sim_fasthenryarcdialogitems[] =
{
 /*  1 */ {0, {88,236,112,316}, BUTTON, N_("OK")},
 /*  2 */ {0, {40,236,64,316}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,8,48,132}, MESSAGE, N_("Thickness:")},
 /*  4 */ {0, {32,136,48,216}, EDITTEXT, x_("")},
 /*  5 */ {0, {56,8,72,132}, MESSAGE, N_("Width:")},
 /*  6 */ {0, {56,136,72,216}, MESSAGE, x_("")},
 /*  7 */ {0, {80,8,96,180}, MESSAGE, N_("Width subdivisions:")},
 /*  8 */ {0, {80,184,96,216}, EDITTEXT, x_("")},
 /*  9 */ {0, {104,8,120,180}, MESSAGE, N_("Height subdivisions:")},
 /* 10 */ {0, {104,184,120,216}, EDITTEXT, x_("")},
 /* 11 */ {0, {232,8,248,36}, MESSAGE, N_("X:")},
 /* 12 */ {0, {232,40,248,136}, MESSAGE, x_("")},
 /* 13 */ {0, {204,8,220,144}, MESSAGE, N_("Head of arc is at:")},
 /* 14 */ {0, {256,8,272,36}, MESSAGE, N_("Y:")},
 /* 15 */ {0, {256,40,272,136}, MESSAGE, x_("")},
 /* 16 */ {0, {280,8,296,36}, MESSAGE, N_("Z:")},
 /* 17 */ {0, {280,40,296,132}, EDITTEXT, x_("")},
 /* 18 */ {0, {232,180,248,208}, MESSAGE, N_("X:")},
 /* 19 */ {0, {232,212,248,308}, MESSAGE, x_("")},
 /* 20 */ {0, {204,180,220,316}, MESSAGE, N_("Tail of arc is at:")},
 /* 21 */ {0, {256,180,272,208}, MESSAGE, N_("Y:")},
 /* 22 */ {0, {256,212,272,308}, MESSAGE, x_("")},
 /* 23 */ {0, {280,180,296,208}, MESSAGE, N_("Z:")},
 /* 24 */ {0, {280,212,296,304}, EDITTEXT, x_("")},
 /* 25 */ {0, {144,8,160,108}, MESSAGE, N_("Group name:")},
 /* 26 */ {0, {144,112,160,316}, POPUP, x_("")},
 /* 27 */ {0, {168,80,184,176}, BUTTON, N_("New Group")},
 /* 28 */ {0, {132,8,133,316}, DIVIDELINE, x_("")},
 /* 29 */ {0, {192,8,193,316}, DIVIDELINE, x_("")},
 /* 30 */ {0, {8,8,24,316}, CHECK, N_("Include this arc in FastHenry analysis")},
 /* 31 */ {0, {304,88,320,168}, MESSAGE, N_("Default Z:")},
 /* 32 */ {0, {304,172,320,268}, MESSAGE, x_("")}
};
static DIALOG sim_fasthenryarcdialog = {{75,75,404,401}, N_("FastHenry Arc Properties"), 0, 32, sim_fasthenryarcdialogitems, 0, 0};

/* special items for the "FastHenry arc" dialog: */
#define DFHA_ARCTHICK_L      3		/* arc thickness label (stat text) */
#define DFHA_ARCTHICK        4		/* arc thickness (edit text) */
#define DFHA_ARCWIDTH        6		/* arc width (stat text) */
#define DFHA_WIDTHSUBDIV_L   7		/* width subdiv label (edit text) */
#define DFHA_WIDTHSUBDIV     8		/* width subdivisions (edit text) */
#define DFHA_HEIGHTSUBDIV_L  9		/* height subdiv label (edit text) */
#define DFHA_HEIGHTSUBDIV   10		/* height subdivisions (edit text) */
#define DFHA_ARCHEADX       12		/* arc head X (stat text) */
#define DFHA_ARCHEADY       15		/* arc head Y (stat text) */
#define DFHA_ARCHEADZ_L     16		/* arc head Z label (stat text) */
#define DFHA_ARCHEADZ       17		/* arc head Z (edit text) */
#define DFHA_ARCTAILX       19		/* arc tail X (stat text) */
#define DFHA_ARCTAILY       22		/* arc tail Y (stat text) */
#define DFHA_ARCTAILZ_L     23		/* arc tail Z label (stat text) */
#define DFHA_ARCTAILZ       24		/* arc tail Z (edit text) */
#define DFHA_GROUPNAME_L    25		/* group name label (stat text) */
#define DFHA_GROUPNAME      26		/* group name (popup) */
#define DFHA_NEWGROUP       27		/* make new group (button) */
#define DFHA_INCLUDE        30		/* include in FastHenry (check) */
#define DFHA_DEFZ           32		/* default z (stat text) */

void sim_fasthenryarcdlog(void)
{
	REGISTER INTBIG itemHit, value, groupnamesize, i, val, lambda;
	REGISTER ARCINST *ai, *oai;
	REGISTER VARIABLE *var;
	CHAR line[60], *groupname, *pt, **groupnames, **newgroupnames, *newname;
	float startfreq, endfreq;
	INTBIG options, runsperdecade, numpoles, seglenlimit, thickness, defthickness,
		widsubdiv, defwidsubdiv, heisubdiv, defheisubdiv, z_head, z_tail, defz;
	BOOLEAN zhover, ztover;
	REGISTER void *dia;

	/* get currently selected arc */
	ai = (ARCINST *)asktool(us_tool, x_("get-arc"));
	if (ai == NOARCINST)
	{
		ttyputerr(_("Select an arc first"));
		return;
	}
	lambda = lambdaofarc(ai);

	/* get all parameters */
	sim_fasthenrygetoptions(&options, &startfreq, &endfreq, &runsperdecade, &numpoles,
		&seglenlimit, &defthickness, &defwidsubdiv, &defheisubdiv);
	groupname = sim_fasthenrygetarcoptions(ai, &thickness, &widsubdiv, &heisubdiv,
		&z_head, &z_tail, &zhover, &ztover, &defz);

	/* display the FastHenry arc dialog box */
	dia = DiaInitDialog(&sim_fasthenryarcdialog);
	if (dia == 0) return;

	/* make a list of all group names */
	groupnamesize = 0;
	for(oai = ai->parent->firstarcinst; oai != NOARCINST; oai = oai->nextarcinst)
	{
		var = getvalkey((INTBIG)oai, VARCINST, VSTRING, sim_fasthenrygroupnamekey);
		if (var != NOVARIABLE)
		{
			for(i=0; i<groupnamesize; i++)
			{
				/* LINTED "groupnames" used in proper order */
				if (namesame(groupnames[i], (CHAR *)var->addr) == 0) break;
			}
			if (i < groupnamesize) continue;

			/* add to the list */
			newgroupnames = (CHAR **)emalloc((groupnamesize+1) * (sizeof (CHAR *)),
				sim_tool->cluster);
			for(i=0; i<groupnamesize; i++)
				newgroupnames[i] = groupnames[i];
			(void)allocstring(&newgroupnames[groupnamesize], (CHAR *)var->addr,
				sim_tool->cluster);
			if (groupnamesize > 0) efree((CHAR *)groupnames);
			groupnames = newgroupnames;
			groupnamesize++;
		}
	}
	if (groupnamesize == 0)
	{
		groupnames = (CHAR **)emalloc((sizeof (CHAR *)), sim_tool->cluster);
		(void)allocstring(&groupnames[0], _("Group 1"), sim_tool->cluster);
		groupnamesize++;
	}
	DiaSetPopup(dia, DFHA_GROUPNAME, groupnamesize, groupnames);

	/* select group name */
	if (groupname != 0)
	{
		for(i=0; i<groupnamesize; i++)
			if (namesame(groupname, groupnames[i]) == 0) break;
		if (i < groupnamesize) DiaSetPopupEntry(dia, DFHA_GROUPNAME, i);
	}

	/* show arc parameters */
	esnprintf(line, 60, _("Thickness (%s):"), latoa(defthickness, lambda));
	DiaSetText(dia, DFHA_ARCTHICK_L, line);
	if (thickness >= 0)
		DiaSetText(dia, DFHA_ARCTHICK, latoa(thickness, lambda));
	DiaSetText(dia, DFHA_ARCWIDTH, latoa(ai->width, lambda));

	/* show subdivision overrides */
	esnprintf(line, 60, _("Width subdivisions (%ld):"), defwidsubdiv);
	DiaSetText(dia, DFHA_WIDTHSUBDIV_L, line);
	if (widsubdiv > 0)
	{
		esnprintf(line, 60, x_("%ld"), widsubdiv);
		DiaSetText(dia, DFHA_WIDTHSUBDIV, line);
	}
	esnprintf(line, 60, _("Height subdivisions (%ld):"), defheisubdiv);
	DiaSetText(dia, DFHA_HEIGHTSUBDIV_L, line);
	if (heisubdiv > 0)
	{
		esnprintf(line, 60, x_("%ld"), heisubdiv);
		DiaSetText(dia, DFHA_HEIGHTSUBDIV, line);
	}

	/* show end coordinates */
	DiaSetText(dia, DFHA_DEFZ, latoa(defz, lambda));
	DiaSetText(dia, DFHA_ARCHEADX, latoa(ai->end[0].xpos, lambda));
	DiaSetText(dia, DFHA_ARCHEADY, latoa(ai->end[0].ypos, lambda));
	if (zhover) DiaSetText(dia, DFHA_ARCHEADZ, latoa(z_head, lambda));
	DiaSetText(dia, DFHA_ARCTAILX, latoa(ai->end[1].xpos, lambda));
	DiaSetText(dia, DFHA_ARCTAILY, latoa(ai->end[1].ypos, lambda));
	if (ztover) DiaSetText(dia, DFHA_ARCTAILZ, latoa(z_tail, lambda));

	if (groupname != 0)
	{
		DiaUnDimItem(dia, DFHA_ARCTHICK_L);
		DiaEditControl(dia, DFHA_ARCTHICK);
		DiaUnDimItem(dia, DFHA_WIDTHSUBDIV_L);
		DiaEditControl(dia, DFHA_WIDTHSUBDIV);
		DiaUnDimItem(dia, DFHA_HEIGHTSUBDIV_L);
		DiaEditControl(dia, DFHA_HEIGHTSUBDIV);
		DiaUnDimItem(dia, DFHA_ARCHEADZ_L);
		DiaEditControl(dia, DFHA_ARCHEADZ);
		DiaUnDimItem(dia, DFHA_ARCTAILZ_L);
		DiaEditControl(dia, DFHA_ARCTAILZ);
		DiaUnDimItem(dia, DFHA_GROUPNAME_L);
		DiaUnDimItem(dia, DFHA_GROUPNAME);
		DiaUnDimItem(dia, DFHA_NEWGROUP);
		DiaSetControl(dia, DFHA_INCLUDE, 1);
	} else
	{
		DiaDimItem(dia, DFHA_ARCTHICK_L);
		DiaNoEditControl(dia, DFHA_ARCTHICK);
		DiaDimItem(dia, DFHA_WIDTHSUBDIV_L);
		DiaNoEditControl(dia, DFHA_WIDTHSUBDIV);
		DiaDimItem(dia, DFHA_HEIGHTSUBDIV_L);
		DiaNoEditControl(dia, DFHA_HEIGHTSUBDIV);
		DiaDimItem(dia, DFHA_ARCHEADZ_L);
		DiaNoEditControl(dia, DFHA_ARCHEADZ);
		DiaDimItem(dia, DFHA_ARCTAILZ_L);
		DiaNoEditControl(dia, DFHA_ARCTAILZ);
		DiaDimItem(dia, DFHA_GROUPNAME_L);
		DiaDimItem(dia, DFHA_GROUPNAME);
		DiaDimItem(dia, DFHA_NEWGROUP);
	}

	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DFHA_INCLUDE)
		{
			value = 1 - DiaGetControl(dia, itemHit);
			DiaSetControl(dia, itemHit, value);
			if (value != 0)
			{
				DiaUnDimItem(dia, DFHA_ARCTHICK_L);
				DiaEditControl(dia, DFHA_ARCTHICK);
				DiaUnDimItem(dia, DFHA_WIDTHSUBDIV_L);
				DiaEditControl(dia, DFHA_WIDTHSUBDIV);
				DiaUnDimItem(dia, DFHA_HEIGHTSUBDIV_L);
				DiaEditControl(dia, DFHA_HEIGHTSUBDIV);
				DiaUnDimItem(dia, DFHA_ARCHEADZ_L);
				DiaEditControl(dia, DFHA_ARCHEADZ);
				DiaUnDimItem(dia, DFHA_ARCTAILZ_L);
				DiaEditControl(dia, DFHA_ARCTAILZ);
				DiaUnDimItem(dia, DFHA_GROUPNAME_L);
				DiaUnDimItem(dia, DFHA_GROUPNAME);
				DiaUnDimItem(dia, DFHA_NEWGROUP);
				DiaSetControl(dia, DFHA_INCLUDE, 1);
			} else
			{
				DiaDimItem(dia, DFHA_ARCTHICK_L);
				DiaNoEditControl(dia, DFHA_ARCTHICK);
				DiaDimItem(dia, DFHA_WIDTHSUBDIV_L);
				DiaNoEditControl(dia, DFHA_WIDTHSUBDIV);
				DiaDimItem(dia, DFHA_HEIGHTSUBDIV_L);
				DiaNoEditControl(dia, DFHA_HEIGHTSUBDIV);
				DiaDimItem(dia, DFHA_ARCHEADZ_L);
				DiaNoEditControl(dia, DFHA_ARCHEADZ);
				DiaDimItem(dia, DFHA_ARCTAILZ_L);
				DiaNoEditControl(dia, DFHA_ARCTAILZ);
				DiaDimItem(dia, DFHA_GROUPNAME_L);
				DiaDimItem(dia, DFHA_GROUPNAME);
				DiaDimItem(dia, DFHA_NEWGROUP);
			}
			continue;
		}
		if (itemHit == DFHA_NEWGROUP)		/* new group name */
		{
			newname = ttygetline(_("New group name:"));
			if (newname == 0 || *newname == 0) continue;
			for(i=0; i<groupnamesize; i++)
				if (namesame(groupnames[i], newname) == 0) break;
			if (i < groupnamesize) continue;

			/* add to the list */
			newgroupnames = (CHAR **)emalloc((groupnamesize+1) * (sizeof (CHAR *)), sim_tool->cluster);
			for(i=0; i<groupnamesize; i++)
				newgroupnames[i] = groupnames[i];
			(void)allocstring(&newgroupnames[groupnamesize], newname, sim_tool->cluster);
			if (groupnamesize > 0) efree((CHAR *)groupnames);
			groupnames = newgroupnames;
			groupnamesize++;
			DiaSetPopup(dia, DFHA_GROUPNAME, groupnamesize, groupnames);
			DiaSetPopupEntry(dia, DFHA_GROUPNAME, groupnamesize-1);
			continue;
		}
	}

	if (itemHit == OK)
	{
		startobjectchange((INTBIG)ai, VARCINST);
		var = getvalkey((INTBIG)ai, VARCINST, VSTRING, sim_fasthenrygroupnamekey);
		if (DiaGetControl(dia, DFHA_INCLUDE) != 0)
		{
			/* active in FastHenry */
			i = DiaGetPopupEntry(dia, DFHA_GROUPNAME);
			groupname = groupnames[i];
			if (var == NOVARIABLE || estrcmp(groupname, (CHAR *)var->addr) != 0)
				setvalkey((INTBIG)ai, VARCINST, sim_fasthenrygroupnamekey,
					(INTBIG)groupname, VSTRING|VDISPLAY);
		} else
		{
			if (var != NOVARIABLE)
				(void)delvalkey((INTBIG)ai, VARCINST, sim_fasthenrygroupnamekey);
		}

		/* save thickness */
		pt = DiaGetText(dia, DFHA_ARCTHICK);
		var = getvalkey((INTBIG)ai, VARCINST, VINTEGER, sim_fasthenrythicknesskey);
		if (*pt != 0)
		{
			val = atola(pt, 0);
			if (var == NOVARIABLE || val != var->addr)
				setvalkey((INTBIG)ai, VARCINST, sim_fasthenrythicknesskey, val, VINTEGER);
		} else
		{
			if (var != NOVARIABLE)
				(void)delvalkey((INTBIG)ai, VARCINST, sim_fasthenrythicknesskey);
		}

		/* save width and height subdivisions */
		pt = DiaGetText(dia, DFHA_WIDTHSUBDIV);
		var = getvalkey((INTBIG)ai, VARCINST, VINTEGER, sim_fasthenrywidthsubdivkey);
		if (*pt != 0)
		{
			val = eatoi(pt);
			if (var == NOVARIABLE || val != var->addr)
				setvalkey((INTBIG)ai, VARCINST, sim_fasthenrywidthsubdivkey, val, VINTEGER);
		} else
		{
			if (var != NOVARIABLE)
				(void)delvalkey((INTBIG)ai, VARCINST, sim_fasthenrywidthsubdivkey);
		}
		pt = DiaGetText(dia, DFHA_HEIGHTSUBDIV);
		var = getvalkey((INTBIG)ai, VARCINST, VINTEGER, sim_fasthenryheightsubdivkey);
		if (*pt != 0)
		{
			val = eatoi(pt);
			if (var == NOVARIABLE || val != var->addr)
				setvalkey((INTBIG)ai, VARCINST, sim_fasthenryheightsubdivkey, val, VINTEGER);
		} else
		{
			if (var != NOVARIABLE)
				(void)delvalkey((INTBIG)ai, VARCINST, sim_fasthenryheightsubdivkey);
		}

		/* save z overrides */
		pt = DiaGetText(dia, DFHA_ARCHEADZ);
		var = getvalkey((INTBIG)ai, VARCINST, VINTEGER, sim_fasthenryzheadkey);
		if (*pt != 0)
		{
			val = atola(pt, 0);
			if (var == NOVARIABLE || val != var->addr)
				setvalkey((INTBIG)ai, VARCINST, sim_fasthenryzheadkey, val, VINTEGER);
		} else
		{
			if (var != NOVARIABLE)
				(void)delvalkey((INTBIG)ai, VARCINST, sim_fasthenryzheadkey);
		}
		pt = DiaGetText(dia, DFHA_ARCTAILZ);
		var = getvalkey((INTBIG)ai, VARCINST, VINTEGER, sim_fasthenryztailkey);
		if (*pt != 0)
		{
			val = atola(pt, 0);
			if (var == NOVARIABLE || val != var->addr)
				setvalkey((INTBIG)ai, VARCINST, sim_fasthenryztailkey, val, VINTEGER);
		} else
		{
			if (var != NOVARIABLE)
				(void)delvalkey((INTBIG)ai, VARCINST, sim_fasthenryztailkey);
		}
		endobjectchange((INTBIG)ai, VARCINST);
	}
	DiaDoneDialog(dia);

	/* free group list memory */
	for(i=0; i<groupnamesize; i++)
		efree(groupnames[i]);
	efree((CHAR *)groupnames);
}

#endif  /* SIMTOOL - at top */
