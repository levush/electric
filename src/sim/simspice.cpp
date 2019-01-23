/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: simspice.cpp
 * SPICE list generator: write a SPICE format file for the current cell
 * Written by: Steven M. Rubin, Static Free Software
 * Improved by: Sid Penstone, Queen's University
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

/*
 * Modified to get areas correctly by Oct.6/87 S.R.Penstone, Queen's U.
 * Pools diffusion areas (see notes below)
 * Revision Nov.12-- fixed error that applied mask scale factor twice
 * Revised Dec.2/87 SRP - separate out the diffusion and transistor types
 * Revision Dec.30/87, to leave out the poly gate on a transistor
 * Revision Mar.29/89 to declare atof() as double (SRP)
 * Revised June 6/89 to ask for cell name during parse-output operation (QU)
 * Revised June 6/89 to no longer look for X in 1st column of spice output (QU)
 * Revised Aug 31/89 to verify cell name during parse-output operation (QU)
 * Revised Aug 31/89 to look for layout view (QU)
 * Revised Nov 27/89 merged with version in Electric 4.04 (QU)
 * Revised Nov 28/89 merged with version from Chris Schneider at U. of Manitoba (QU)
 * Revised Mar 89 to support the use of node names, external model files,
 *    remote execution (UNIX), .NODESET and special sources, meters (fixed
 *    bug), varargs (UNIX), interactive use with spice2 spice3 or hspice
 *    simulators... by L. Swab (QU).  Some code by C. Schneider.
 * Revised June 7/90 to check for prototype ground net when exporting
 * subcircuit ports; if it is ground, do not export it. (SRP) This will
 * be looked at again.
 * Revised Dec.7/90 to start handling ARRAY types
 * MAJOR CHANGES Dec/90 SRP:
 * prepare to split source files into two files: simspice.c and simspicerun.c to
 * separate execution from writing
 * Create separate function to write 2 port types, so it can be called
 * Do not write  two terminal elements that are shorted out 91-1-30 SRP
 * Mar. 1991:  Include a substrate connection for bipolar transistors; use the
 * subnet defined by a substrate connection or default to ground.
 * Nov.18/91 Test the temp bits in the cell in case we
 * are doing an extraction from icons...
 * Nov. 29/91: Added option to write a trailer file, tech:~.SIM_spice_trailer_file
 * modified comment header to describe capabilities
 * Removed bug in writing substrate of bipolar transistors
 * 920113: Created default ground net in cells; output only the first
 * name of multiply-named networks; output a reference list of named arcs
 * Do not complain about posnet in subcircuits
 * SRP920603: Added option to include behavioral file for a cell, or a cell.
 * The variable "SPICE_behave_file" can be attached to the cell (in case it is
 * an icon, or to any cell in the cell on the nodeproto.
 * Changed the name of the behavior file variable to "sim_spice_behave_file"
 * SRP920604
 * We should only count diffusion connections to the network, to calculate
 * the drain and source areas correctly!
 * Removed trapdoor in sim_spice_arcarea() for well layers SRP920618:
 * Added function to look for diffusion arc function
 * Added check for isolated diffusion that will not be modelled correctly
 * Caught multiple polygons on different arc in sim_spice_evalpolygon()
 * Changed call to mrgdonecell() and evalpolygon() to use (float) area
 * Changed conditions for error messages about wells and substrates (SRP)
 * Identify ground node even if the port is not flagged SRP920623
 * SRP920712: Changed storage in nets for diffarea[] to float as well
 *     "    Changed to write cell pins in numerical order
 * Fixed bug in sim_spice_nodearea() that didi not traverse the arcs of
 * a complex cell
 * Wrote temporary local version of decsribenetwork() to get around bug
 * in infinitstr() code
 * RLW920716: modified sim_spice_writecell() to indicate the type of
 *     unconnected diffusion on a node
 *
 * SRP921116: Changed warning messages in spice file to comments so that
 *      simulator can test file
 *
 * TADR20000805 Added dependent sources CCVS CCCS VCVS VCCS
 *
 * To properly simulate a cell, it must have the following (many of these
 * commands are found in the "spice.mac" command file):
 *
 * 1) Power and ground must be exports and explicitly connected to sources.
 *    You can do this with a power source (the primitive node prototype
 *    "schematic:source") which must be connected to power at the top and to
 *    ground at the bottom.  The source should then be parameterized to
 *    indicate the amount and whether it is voltage or current.  For example,
 *    to make a 5 volt supply, create a source node and use:
 *        setsource v "DC 5"
 *
 * 2) All input ports must be exports and connected to the positive side
 *    of sources (again, the "schematic:source" node).  Time values may be
 *    placed on the source nodes.  For example, if the source is to produce
 *    values that are 0 volts from 0 to 10NS and then 5 volts, use:
 *        setsource v "PWL(0NS 0 10NS 0 11NS 5)"
 *    constant input values can use the same form as for power and ground.
 *
 *    A .NODESET source can be created by connecting the positive end of the
 *    source to the appropriate node, and typing:
 *        setsource n VALUE
 *
 *    Special sources such as VCVS's can be created with the "special" source.
 *    The letter following the "s" in the parameterization will appear as
 *    the first letter in the deck.  For example, to create a VCVS, create
 *    a source and connect it to the approprite nodes.  Then parameterize it
 *    with
 *        setsource se "NC+ NC-"
 *    Where NC+ and NC- are the names of ports which specify the control
 *    voltages.  This would produce a spice card of the form:
 *        EX N+ N- NC+ NC-
 *    Where X is a unique name or number, and N+ and N- are the nodes which
 *    are connected to the source.
 *
 * 3) All values that are being watched must be exports and have meters
 *    placed on them.  The primitive nodeproto "schematic:meter" can be placed
 *    anywhere and it should have the top and bottom ports connected
 *    appropriately.  For the meter to watch voltage from 0 to 5 volts, use:
 *        setmeter "(0 5)"
 *    To watch the current through any voltage source, parameterize it in the
 *    usual way, but add the "m" option, eg.
 *        setsource vm ...
 *        setsource vdm ...
 *
 * 4) Determine the level of simulation by saying:
 *        setlevel 2
 *    This will extract the appropriate header from the technology.
 *    Alternately, you can specify a file to read the header info from
 *    with the command:
 *        variable set tech:~.SIM_spice_model_file FILENAME
 *
 * 5) Determine the type of analysis with another source node.  For transient
 *    analysis that displays half NS intervals, runs for 20NS, starts at time
 *    0, and uses internal steps of 0.1NS, create an unconnected source node
 *    and issue this statement:
 *        setsource vt ".5NS 20NS 0NS .1NS"
 *    For DC analysis you must connect the power side of the source node to
 *    the DC point and then use:
 *        setsource vd "0V 5V .1V"
 *    For AC analysis, create a source node and parameterize it with (eg.)
 *        setsource va "DEC 10 1 10K"
 *    There must be exactly one of these source nodes.
 *
 * 6) Define the spice format to use when producing the deck and parsing
 *    the output:
 *        telltool simulation spice format [2 | 3 | hspice | pspice | gnucap ]
 *
 * 7) Run spice with the command:
 *        ontool simulation
 *    This generates a deck and runs the simulator.  The results from .PRINT
 *    commands will be converted to a cell in the current library that
 *    contains a plot of the data.
 *
 * 8) You can also run SPICE on another machine (or otherwise outside of
 *    Electric).  To do this, supress SPICE execution with:
 *        telltool simulation not execute
 *    which will cause deck generation only.  Then run SPICE externally
 *    and convert the output listing to a plot with:
 *        telltool simulation spice parse-output FILE
 *
 * 9) You can replace the internal spice header file that is part of the
 *        technology by defining the variable "SIM_spice_model_file" on the
 *        technology, which is the name of a file. You can add a trailer to the
 *    spice deck by attaching a variable "SIM_spice_trailer_file" to the
 *    technology, that is the name of a file. These variables are most
 *    easily created in the Info.variables window, by clicking on
 *    "Current Technology", then "New Attribute", defining "SIM_spice_model_file"
 *    with the name of the file. Remember to click on "Set Attribute".
 *    Include a full path if the file will not be in the current directory at
 *    run time.
 *
 * 10) You can call up special bipolar and jfet models that are defined in
 *    your header file by including an appropriate string in the variable
 *    "SIM_spice_model" that is attached to transistors when they
 *    are created. When the 'Transistor size' window appears, enter the
 *    model name, ex: LARGE_NPN
 *        The extractor will use the string in the call in the
 *    spice file, ex:   Q1 3 4 0 0 LARGE_NPN
 *    (in fact, any string that does not include the character '/' will
 *    be used to describe the transistor in the spice file; you can
 *    use this to define the attributes of individual transistors in your
 *    circuit. The character '/' is reserved as a separator for length
 *    and width values, and if present, causes the default type to be
 *    invoked.)
 *
 * 11) You can use the contents of a file to replace the extracted description
 *    of any cell, by attaching the name of the file as a variable
 *    called "SIM_spice_behave_file" to the prototype.  The extractor will always
 *    use the file, if found, instead of extracting the cell network, but it will
 *    still extract any subcells in the cell. These in turn could also be described by
 *    behavior files. If an icon or different view is used, it can have a
 *    different behavior file than the other views.
 *
 */

/*
 * Extraction notes: Layers on arcs and nodes that overlap are
 * merged into polygons, and the area and perimeter of the resulting polygon
 * is computed. Overlapping areas are thus eliminated. The resultinmg areas
 * are used to calculate the capacitance on the net, with the following special
 * treatment: If the node or arc has multiple layers, the layer that gives the
 * largest capacitance is left as the only active capacitance, and the other
 * layers have an their area equal to their area on this port of the node
 * removed.
 * BUT, if the node or arc has a diffusion layer, that layer is always assumed
 * dominant, and the area of the nondominant layers are subtracted from
 * their accumulated area. This is not quite correct, when the diffusion area
 * only partly covers the other areas.
 * The NPCAPAC nodes (series capacitors) are defined to have a dominant
 * diffusion layer, so that their nondiffusion layers are cancelled out. In
 * order to cancel out the perimeter capacity of a top-plate layer, there
 * should be an identical-sized layer with a nonzero area capacitance value,
 * and a negative edge capacitance value equal to that of the layer to be
 * cancelled out.
 * The diffusion areas are gathered up according to whether they are declared
 * as n-type, or p-type, or undefined. DMOS are assumed n-type. The number
 * of n-transistors and p-transistors on the net are counted up, and the
 * corresponding diffusion shared equally.
 * It is assumed that the technology file has correctly used the bits that
 * define layer functions.
 * MOS Transistors must have a correct labelling of the source and drain, or
 * there may be problems in the simulations if the wrong end is connected to
 * the substrate. In this extractor, the order of extraction will be gate,
 * source, gate, drain, based on the order of the ports in the technology file.
 * This will be correct for PMOS with the Vdd at the top. The extracted values
 * source and drain areas will correspond to this order.
 * pMOS-transistors as taken from the technology file prototype will have their
 * source at the top (more positive end), and nMOS-transistors taken in the
 * prototype position will have to be rotated to have their drain at the top.
 * Other device types will output collector, base, emitter, corresponding to
 * extraction of the first three ports from the prototype in the tech file.
 * Otherwise manual editing of the SPICE file is required.
 */

#include "config.h"
#if SIMTOOL

#include "global.h"
#include "sim.h"
#include "eio.h"
#include "usr.h"
#include "network.h"
#include "efunction.h"
#include "tecschem.h"
#include "tecgen.h"
#include <math.h>

#define DIFFTYPES       3	/* Types of diffusions & transistors plus 1 */
#define ISNONE	        0
#define ISNTYPE	        1
#define ISPTYPE	        2

#define SPICELEGALCHARS        x_("!#$%*+-/<>[]_")
#define CDLNOBRACKETLEGALCHARS x_("!#$%*+-/<>_")
#define SPICEMAXLENSUBCKTNAME	70						/* maximum subcircuit name length (JKG) */

#define sim_spice_puts(s,iscomment) sim_spice_xputs(s, sim_spice_file, iscomment)

static CHAR       *sim_spicelegalchars;					/* legal characters */
static float       sim_spice_min_resist;				/* spice minimum resistance */
static float       sim_spice_min_capac;					/* spice minimum capacitance */
static CHAR       *sim_spice_ac;						/* AC analysis message */
static CHAR       *sim_spice_dc;						/* DC analysis message */
static CHAR       *sim_spice_tran;						/* Transient analysis message */
static POLYGON    *sim_polygonfree = NOPOLYGON;			/* list of free simulation polygons */
static float       sim_spice_mask_scale = 1.0;			/* Mask shrink factor (default =1) */
static float      *sim_spice_extra_area = 0;			/* Duplicated area on each layer */
static INTBIG      sim_spice_diffusion_index[DIFFTYPES];/* diffusion layers indices */
static INTBIG      sim_spice_layer_count;
static INTBIG      sim_spice_unnamednum;
static INTBIG      sim_spice_netindex;
       INTBIG      sim_spice_levelkey;					/* key for "SIM_spice_level" */
       INTBIG      sim_spice_statekey;					/* key for "SIM_spice_state" */
       INTBIG      sim_spice_state;						/* value of "SIM_spice_state" */
       INTBIG      sim_spice_nameuniqueid;				/* key for "SIM_spice_nameuniqueid" */
static INTBIG      sim_spice_machine;					/* Spice type: 2, 3, H, P */
       INTBIG      sim_spice_listingfilekey;			/* key for "SIM_listingfile" */
       INTBIG      sim_spice_runargskey;				/* key for "SIM_spice_runarguments" */
static CHAR       *sim_spice_printcard;					/* the .PRINT or .PLOT card */
static TECHNOLOGY *sim_spice_tech;						/* default technology to use */
static INTBIG      sim_spicewirelisttotal = 0;
static NETWORK   **sim_spicewirelist;
static INTBIG      sim_spice_card_key = 0;				/* key for "SPICE_card" */
static INTBIG      sim_spice_template_key = 0;			/* key for "ATTR_SPICE_template" */
static INTBIG      sim_spice_template_s2_key = 0;		/* key for "ATTR_SPICE_template_spice2" */
static INTBIG      sim_spice_template_s3_key = 0;		/* key for "ATTR_SPICE_template_spice3" */
static INTBIG      sim_spice_template_hs_key = 0;		/* key for "ATTR_SPICE_template_hspice" */
static INTBIG      sim_spice_template_ps_key = 0;		/* key for "ATTR_SPICE_template_pspice" */
static INTBIG      sim_spice_template_gc_key = 0;		/* key for "ATTR_SPICE_template_gnucap" */
static INTBIG      sim_spice_template_ss_key = 0;		/* key for "ATTR_SPICE_template_smartspice" */
static INTBIG      sim_spice_preferedkey;
static INTBIG      sim_spiceglobalnetcount;				/* number of global nets */
static INTBIG      sim_spiceglobalnettotal = 0;			/* size of global net array */
static CHAR      **sim_spiceglobalnets;					/* global net names */
static INTBIG      sim_spiceNameUniqueID;				/* for short unique subckt names (JKG) */
static FILE       *sim_spice_file;                      /* output stream */
static BOOLEAN     sim_spice_cdl;                       /* If "sim_spice_cdl" is true, put handle CDL format */

/* working memory for "sim_spice_edgecapacitance()" */
static INTBIG *sim_spice_capacvalue = 0;

/******************** SPICE NET QUEUE ********************/

#define NOSPNET   ((SPNET *)-1)

typedef struct Ispnet
{
	NETWORK       *network;					/* network object associated with this */
	INTBIG         netnumber;				/* internal unique net number */
	float          diffarea[DIFFTYPES];		/* area of diffusion */
	float          diffperim[DIFFTYPES];	/* perimeter of diffusion */
	float          resistance;				/* amount of resistance */
	float          capacitance;				/* amount of capacitance */
	INTBIG         components[DIFFTYPES];	/* number of components connected to net */
	SpiceNet      *spiceNet;                /* pointer to SpiceNet class */
	struct Ispnet *nextnet;					/* next in linked list */
} SPNET;

static SPNET     *sim_spice_firstnet;				/* first in list of nets in this cell */
static SPNET     *sim_spice_netfree = NOSPNET;		/* list of free nets */
static SPNET     *sim_spice_cur_net;				/* for polygon merging */

static NETWORK   *sim_spice_gnd;					/* net of ground */
static NETWORK   *sim_spice_vdd;					/* net of power */

/* prototypes for local routines */
static void       sim_spice_addincludefile(CHAR *filename);
static POLYGON   *sim_spice_allocpolygon(void);
static SPNET     *sim_spice_allocspnet(void);
static void       sim_spice_arcarea(SPNET*, ARCINST*);
static INTBIG     sim_spice_arcisdiff(ARCINST*);
static float      sim_spice_capacitance(TECHNOLOGY*, INTBIG);
static CHAR      *sim_spice_cellname(NODEPROTO *np);
static CHAR      *sim_spice_describenetwork(NETWORK *net);
static void       sim_spice_dumpstringerror(void *infstr, SpiceInst *inst);
static float      sim_spice_edgecapacitance(TECHNOLOGY*, INTBIG);
static CHAR      *sim_spice_elementname(NODEINST*, CHAR, INTBIG*, CHAR*);
static void       sim_spice_evalpolygon(INTBIG, TECHNOLOGY*, INTBIG*, INTBIG*, INTBIG);
static void       sim_spice_freepolygon(POLYGON*);
static void       sim_spice_freespnet(SPNET*);
static void       sim_spice_gatherglobals(NODEPROTO *np);
static INTBIG     sim_spice_getexportednetworks(NODEPROTO *cell, NETWORK ***netlist,
					NETWORK **vdd, NETWORK **gnd, BOOLEAN cdl);
static SPNET     *sim_spice_getnet(NODEINST*, NETWORK*);
static INTBIG     sim_spice_layerisdiff(TECHNOLOGY*, INTBIG);
static CHAR      *sim_spice_legalname(CHAR *name);
static INTBIG	  sim_spice_markuniquenodeinsts(NODEPROTO*);
static CHAR      *sim_spice_netname(NETWORK *net, INTBIG bussize, INTBIG busindex);
static void       sim_spice_nodearea(SPNET*, NODEINST*, PORTPROTO*);
static CHAR      *sim_spice_nodename(SPNET*);
static int        sim_spice_sortnetworksbyname(const void *e1, const void *e2);
static void       sim_spice_storebox(TECHNOLOGY*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static INTBIG     sim_spice_writecell(NODEPROTO*, BOOLEAN, CHAR*);
static void       sim_spice_writeheader(NODEPROTO*);
static void       sim_spice_writetrailer(NODEPROTO*);
static void       sim_spice_writetwoport(NODEINST*, INTBIG, CHAR*, SpiceCell*, INTBIG*, INTBIG);
static void       sim_spice_xputs(CHAR *s, FILE *stream, BOOLEAN iscomment);

/*
 * Routine to free all memory associated with this module.
 */
void sim_freespicememory(void)
{
	REGISTER POLYGON *poly;
	REGISTER SPNET *spnet;
	REGISTER INTBIG i;

	while (sim_polygonfree != NOPOLYGON)
	{
		poly = sim_polygonfree;
		sim_polygonfree = sim_polygonfree->nextpolygon;
		freepolygon(poly);
	}

	while (sim_spice_netfree != NOSPNET)
	{
		spnet = sim_spice_netfree;
		sim_spice_netfree = sim_spice_netfree->nextnet;
		efree((CHAR *)spnet);
	}
	if (sim_spice_capacvalue != 0) efree((CHAR *)sim_spice_capacvalue);
	if (sim_spicewirelisttotal > 0) efree((CHAR *)sim_spicewirelist);
	for(i=0; i<sim_spiceglobalnettotal; i++)
		if (sim_spiceglobalnets[i] != 0)
			efree((CHAR *)sim_spiceglobalnets[i]);
	if (sim_spiceglobalnettotal > 0)
		efree((CHAR *)sim_spiceglobalnets);
}

/******************** SPICE DECK GENERATION ********************/

/*
 * Procedure to write a spice deck to describe cell "np".
 * If "cdl" is true, put handle CDL format (no parameters, no top-level).
 */
void sim_writespice(NODEPROTO *np, BOOLEAN cdl)
{
	REGISTER INTBIG retval, *curstate;
	REGISTER INTBIG analysiscards, i;
	CHAR deckfile[256], *pt;
	REGISTER VARIABLE *var;
	REGISTER LIBRARY *lib, *olib;
	REGISTER NODEPROTO *onp, *tnp, *gnp;
	REGISTER TECHNOLOGY *t;
	CHAR templatefile[256], *respar[2], *libname, *libpath, *prompt;
	extern TOOL *io_tool;
	REGISTER void *infstr;
	REGISTER BOOLEAN backannotate;

	/* make sure network tool is on */
	if ((net_tool->toolstate&TOOLON) == 0)
	{
		ttyputerr(_("Network tool must be running...turning it on"));
		toolturnon(net_tool);
		ttyputerr(_("...now reissue the simulation command"));
		return;
	}

	if (np == NONODEPROTO)
	{
		ttyputerr(_("Must have a cell to edit"));
		return;
	}
	sim_simnt = np;

	/* determine technology to use */
	sim_spice_tech = defschematictechnology(np->tech);
	sim_spice_layer_count = sim_spice_tech->layercount;

	/* get the SPICE state */
	var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_spice_statekey);
	if (var != NOVARIABLE) sim_spice_state = var->addr; else sim_spice_state = 0;
	sim_spice_machine = sim_spice_state & SPICETYPE;

	/*
	 * initialize the .PRINT or .PLOT card.
	 * As we find sources/meters, we will tack things onto this string
	 * and 'reallocstring' it.
	 * It is printed and then freed in sim_spice_writecell.
	 */
	if (allocstring(&sim_spice_printcard, x_(""), sim_tool->cluster)) return;

	/* get the overall minimum resistance and capacitance */
	var = getval((INTBIG)sim_spice_tech, VTECHNOLOGY, VFLOAT, x_("SIM_spice_min_resistance"));
	if (var != NOVARIABLE) sim_spice_min_resist = castfloat(var->addr); else
		sim_spice_min_resist = 0.0;
	var = getval((INTBIG)sim_spice_tech, VTECHNOLOGY, VFLOAT, x_("SIM_spice_min_capacitance"));
	if (var != NOVARIABLE) sim_spice_min_capac = castfloat(var->addr); else
		sim_spice_min_capac = 0.0;
	var = getval((INTBIG)sim_spice_tech, VTECHNOLOGY, VFLOAT, x_("SIM_spice_mask_scale"));
	if (var != NOVARIABLE) sim_spice_mask_scale = castfloat(var->addr); else
		sim_spice_mask_scale = 1.0;
	sim_spice_extra_area = (float *)emalloc(sizeof(float) * sim_spice_layer_count,
		sim_tool->cluster);

	/* get the layer resistance and capacitance arrays for each technology */
	for(t = el_technologies; t != NOTECHNOLOGY; t = t->nexttechnology)
	{
		var = getval((INTBIG)t, VTECHNOLOGY, VFLOAT|VISARRAY, x_("SIM_spice_resistance"));
		t->temp1 = (var == NOVARIABLE ? 0 : var->addr);
		var = getval((INTBIG)t, VTECHNOLOGY, VFLOAT|VISARRAY, x_("SIM_spice_capacitance"));
		t->temp2 = (var == NOVARIABLE ? 0 : var->addr);
	}

	/*
	 * determine whether any cells have name clashes in other libraries
	 */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
			onp->temp2 = 0;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
		for(tnp = lib->firstnodeproto; tnp != NONODEPROTO; tnp = tnp->nextnodeproto)
		{
			if (tnp->temp2 != 0) continue;
			for(olib = lib->nextlibrary; olib != NOLIBRARY; olib = olib->nextlibrary)
			{
				if ((olib->userbits&HIDDENLIBRARY) != 0) continue;
				for(onp = olib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
					if (namesame(tnp->protoname, onp->protoname) == 0) break;
				if (onp != NONODEPROTO)
				{
					FOR_CELLGROUP(gnp, onp)
						gnp->temp2 = 1;
					FOR_CELLGROUP(gnp, tnp)
						gnp->temp2 = 1;
				}
			}
		}
	}

	/* setup the legal characters */
	sim_spicelegalchars = SPICELEGALCHARS;

	/* start writing the spice deck */
	sim_spice_cdl = cdl;
	if (cdl)
	{
		/* setup bracket conversion for CDL */
		curstate = io_getstatebits();
		if ((curstate[1]&CDLNOBRACKETS) != 0)
			sim_spicelegalchars = CDLNOBRACKETLEGALCHARS;

		(void)estrcpy(deckfile, np->protoname);
		(void)estrcat(deckfile, x_(".cdl"));
		pt = deckfile;
		prompt = 0;
		if ((us_useroptions&NOPROMPTBEFOREWRITE) == 0) prompt = _("CDL File");
		sim_spice_file = xcreate(deckfile, sim_filetypecdl, prompt, &pt);
		if (pt != 0) (void)estrcpy(deckfile, pt);
		if (sim_spice_file == NULL)
		{
			ttyputerr(_("Cannot create CDL file: %s"), deckfile);
			return;
		}
		sim_spice_xprintf(sim_spice_file, TRUE, x_("* First line is ignored\n"));
	} else
	{
		(void)estrcpy(deckfile, np->protoname);
		(void)estrcat(deckfile, x_(".spi"));
		pt = deckfile;
		prompt = 0;
		if ((us_useroptions&NOPROMPTBEFOREWRITE) == 0) prompt = _("SPICE File");
		sim_spice_file = xcreate(deckfile, sim_filetypespice, prompt, &pt);
		if (pt != 0) (void)estrcpy(deckfile, pt);
		if (sim_spice_file == NULL)
		{
			ttyputerr(_("Cannot create SPICE file: %s"), deckfile);
			return;
		}
		(void)sim_spice_writeheader(np);
	}

	/* gather all global signal names (HSPICE and PSPICE only) */
	sim_spiceglobalnetcount = 0;
	if ((sim_spice_state&SPICENODENAMES) != 0 && sim_spice_machine != SPICE3)
	{
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
				onp->temp1 = 0;
		sim_spice_gatherglobals(np);
		if (sim_spiceglobalnetcount > 0)
		{
			sim_spice_xprintf(sim_spice_file, FALSE, x_("\n"));
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_(".global"));
			for(i=0; i<sim_spiceglobalnetcount; i++)
				formatinfstr(infstr, x_(" %s"), sim_spiceglobalnets[i]);
			sim_spice_xprintf(sim_spice_file, FALSE, x_("%s\n"), returninfstr(infstr));
		}
	}

	/* initialize short names for subcircuits (JKG) */
	sim_spiceNameUniqueID = 0;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
			onp->temp1 = 0;
	sim_spice_markuniquenodeinsts(np);

	/* see if there are any resistors in this circuit */
	if (hasresistors(np))
	{
		/* has resistors: make sure they aren't being ignored */
		if (asktech(sch_tech, x_("ignoring-resistor-topology")) != 0)
		{
			/* must redo network topology to make resistors real */
			respar[0] = x_("resistors");
			respar[1] = x_("include");
			(void)telltool(net_tool, 2, respar);
		}
	}

	/* make sure that all nodes have names on them */
	backannotate = FALSE;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
	{
		if (asktool(net_tool, x_("name-nodes"), (INTBIG)onp) != 0) backannotate = TRUE;
		if (asktool(net_tool, x_("name-nets"), (INTBIG)onp) != 0) backannotate = TRUE;
	}

	/* reset bits on all cells */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
	{
		onp->temp1 = 0;
//		onp->cell->temp1 = 0;
	}
	sim_spice_unnamednum = 1;

	/* initialize for parameterized cells */
	if (sim_spice_cdl || (sim_spice_state&SPICECELLPARAM) == 0) initparameterizedcells();
	begintraversehierarchy();

	/* we don't know the type of analysis yet... */
	sim_spice_ac = sim_spice_dc = sim_spice_tran = NULL;

	/* initialize the polygon merging system */
	mrginit();

	/* initialize SpiceCell structures */
	SpiceCell::clearAll();

	/* recursively write all cells below this one */
	retval = sim_spice_writecell(np, TRUE, 0);
	endtraversehierarchy();
	if (retval < 0) ttyputnomemory();
	if (backannotate)
		ttyputmsg(_("Back-annotation information has been added (library must be saved)"));

	/* handle AC, DC, and TRAN analysis cards */
	analysiscards = 0;
	if (sim_spice_dc != NULL) analysiscards++;
	if (sim_spice_tran != NULL) analysiscards++;
	if (sim_spice_ac != NULL) analysiscards++;
	if (analysiscards > 1)
		ttyputerr(_("WARNING: can only have one DC, Transient or AC source node"));
	if (sim_spice_tran != NULL)
	{
		sim_spice_xprintf(sim_spice_file, FALSE, x_("%s\n"), sim_spice_tran);
		efree(sim_spice_tran);
	} else if (sim_spice_ac != NULL)
	{
		sim_spice_xprintf(sim_spice_file, FALSE, x_("%s\n"), sim_spice_ac);
		efree(sim_spice_ac);
	} else if (sim_spice_dc != NULL)
	{
		sim_spice_xprintf(sim_spice_file, FALSE, x_("%s\n"), sim_spice_dc);
		efree(sim_spice_dc);
	}

	if (!sim_spice_cdl)
	{
		sim_spice_writetrailer(np);
		sim_spice_xprintf(sim_spice_file, FALSE, x_(".END\n"));
	}
	xclose(sim_spice_file);
	ttyputmsg(_("%s written"), deckfile);
	efree((CHAR *)sim_spice_extra_area);

	/* finish polygon merging subsystem */
	mrgterm();

	if (sim_spice_cdl)
	{
		/* write the control files */
		(void)estrcpy(templatefile, np->protoname);
		(void)estrcat(templatefile, x_(".cdltemplate"));
		sim_spice_file = xcreate(templatefile, sim_filetypectemp, _("CDL Template File"), &pt);
		if (pt != 0) (void)estrcpy(templatefile, pt);
		if (sim_spice_file == NULL)
		{
			ttyputerr(_("Cannot create CDL template file: %s"), templatefile);
			return;
		}
		for(i=estrlen(deckfile)-1; i>0; i--) if (deckfile[i] == DIRSEP) break;
		if (deckfile[i] == DIRSEP) deckfile[i++] = 0;
		var = getval((INTBIG)io_tool, VTOOL, VSTRING, x_("IO_cdl_library_name"));
		if (var == NOVARIABLE) libname = x_(""); else
			libname = (CHAR *)var->addr;
		var = getval((INTBIG)io_tool, VTOOL, VSTRING, x_("IO_cdl_library_path"));
		if (var == NOVARIABLE) libpath = x_(""); else
			libpath = (CHAR *)var->addr;
		xprintf(sim_spice_file, x_("cdlInKeys = list(nil\n"));
		xprintf(sim_spice_file, x_("    'searchPath             \"%s"), deckfile);
		if (libpath[0] != 0)
			xprintf(sim_spice_file, x_("\n                             %s"), libpath);
		xprintf(sim_spice_file, x_("\"\n"));
		xprintf(sim_spice_file, x_("    'cdlFile                \"%s\"\n"), &deckfile[i]);
		xprintf(sim_spice_file, x_("    'userSkillFile          \"\"\n"));
		xprintf(sim_spice_file, x_("    'opusLib                \"%s\"\n"), libname);
		xprintf(sim_spice_file, x_("    'primaryCell            \"%s\"\n"), sim_spice_cellname(np));
		xprintf(sim_spice_file, x_("    'caseSensitivity        \"preserve\"\n"));
		xprintf(sim_spice_file, x_("    'hierarchy              \"flatten\"\n"));
		xprintf(sim_spice_file, x_("    'cellTable              \"\"\n"));
		xprintf(sim_spice_file, x_("    'viewName               \"netlist\"\n"));
		xprintf(sim_spice_file, x_("    'viewType               \"\"\n"));
		xprintf(sim_spice_file, x_("    'pr                     nil\n"));
		xprintf(sim_spice_file, x_("    'skipDevice             nil\n"));
		xprintf(sim_spice_file, x_("    'schemaLib              \"sample\"\n"));
		xprintf(sim_spice_file, x_("    'refLib                 \"\"\n"));
		xprintf(sim_spice_file, x_("    'globalNodeExpand       \"full\"\n"));
		xprintf(sim_spice_file, x_(")\n"));
		xclose(sim_spice_file);
		ttyputmsg(_("%s written"), templatefile);
		ttyputmsg(x_("Now type: exec nino CDLIN %s &"), templatefile);
	}

	/* run spice (if requested) */
	var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_dontrunkey);
	if (var != NOVARIABLE && var->addr != SIMRUNNO)
	{
		ttyputmsg(_("Running SPICE..."));
		var = getvalkey((INTBIG)sim_tool, VTOOL, VSTRING, sim_spice_listingfilekey);
		if (var == NOVARIABLE) sim_spice_execute(deckfile, x_(""), np); else
			sim_spice_execute(deckfile, (CHAR *)var->addr, np);
	}
}

/*
 * routine to write cell "np" and its all referenced subnodeprotos in SPICE
 * format to file "f".  If "top" is true, this is the top cell.  The
 * spice level is "tool:sim.SIM_spice_level" and "sim_spice_min_resist" and
 * "sim_spice_min_capac" are the minimum required resistance and capacitance.
 * Returns negative on error, positive if back annotation was added.
 */
INTBIG sim_spice_writecell(NODEPROTO *np, BOOLEAN top, CHAR *paramname)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *subnt, *cnp;
	REGISTER PORTPROTO *pp, *cpp;
	PORTPROTO *biasport = NOPORTPROTO;
	PORTPROTO *gate, *source, *drain, *gatedummy;
	NETWORK **netlist, *subvdd, *subgnd;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER ARCINST *ai;
	REGISTER SPNET *spnet, *nspnet, *gaten, *sourcen, *drainn, *posnet, *negnet,
		*subnet, *biasn;
	float a, b, purevalue;
	REGISTER BOOLEAN backannotate;
	REGISTER INTBIG state, j, first, i, nodetype, retval, len, count,
		reallambda, nodelambda, netcount, err, sigcount, dumpcell, nodewidth, nindex;
	INTBIG subcellindex, nodeindex, resistnum, capacnum, inductnum, diodenum;
	REGISTER VARIABLE *var, *vartemplate, *nivar, *varl, *varw;
	REGISTER NETWORK *net, *rnet;
	CHAR *extra, *info, line[100], *shortname, *pname, **strings;
	REGISTER CHAR *pt, *start, save;
	INTBIG lx, ly, *indexlist, depth;
	INTBIG bipolars, nmostrans, pmostrans;
	NODEINST **hier;
	CHAR *uncon_diff_type, **nodenames;
	REGISTER void *infstr;
	SpiceCell *spCell;
	SpiceInst *spInst;

	/* stop if requested */
	if (stopping(STOPREASONDECK)) return(-1);
	
	/* make sure key is cached */
	if (sim_spice_template_key == 0)
		sim_spice_template_key = makekey(x_("ATTR_SPICE_template"));
	if (sim_spice_template_s2_key == 0)
		sim_spice_template_s2_key = makekey(x_("ATTR_SPICE_template_spice2"));
	if (sim_spice_template_s3_key == 0)
		sim_spice_template_s3_key = makekey(x_("ATTR_SPICE_template_spice3"));
	if (sim_spice_template_hs_key == 0)
		sim_spice_template_hs_key = makekey(x_("ATTR_SPICE_template_hspice"));
	if (sim_spice_template_ps_key == 0)
		sim_spice_template_ps_key = makekey(x_("ATTR_SPICE_template_pspice"));
	if (sim_spice_template_gc_key == 0)
		sim_spice_template_gc_key = makekey(x_("ATTR_SPICE_template_gnucap"));
	if (sim_spice_template_ss_key == 0)
		sim_spice_template_ss_key = makekey(x_("ATTR_SPICE_template_smartspice"));
	switch (sim_spice_state&SPICETYPE)
	{
		case SPICE2:          sim_spice_preferedkey = sim_spice_template_s2_key;   break;
		case SPICE3:          sim_spice_preferedkey = sim_spice_template_s3_key;   break;
		case SPICEHSPICE:     sim_spice_preferedkey = sim_spice_template_hs_key;   break;
		case SPICEPSPICE:     sim_spice_preferedkey = sim_spice_template_ps_key;   break;
		case SPICEGNUCAP:     sim_spice_preferedkey = sim_spice_template_gc_key;   break;
		case SPICESMARTSPICE: sim_spice_preferedkey = sim_spice_template_ss_key;   break;
	}

	/* look for a model file on the current cell */
	var = getval((INTBIG)np, VNODEPROTO, VSTRING, x_("SIM_spice_behave_file"));

	/* first pass through the node list */
	/* if there are any sub-cells that have not been written, write them */
	backannotate = FALSE;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		subnt = ni->proto;
		if (subnt->primindex != 0) continue;

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(subnt, np)) continue;

		/* see if this cell has a template */
		
		var = getvalkey((INTBIG)subnt, VNODEPROTO, VSTRING, sim_spice_preferedkey);
		if (var == NOVARIABLE)
			var = getvalkey((INTBIG)subnt, VNODEPROTO, VSTRING, sim_spice_template_key);
		if (var != NOVARIABLE) continue;

		/* use the contents view if this is an icon */
		if (subnt->cellview == el_iconview)
		{
			cnp = contentsview(subnt);
			if (cnp != NONODEPROTO)
			{
				subnt = cnp;

				/* see if this contents cell has a template */
				var = getvalkey((INTBIG)subnt, VNODEPROTO, VSTRING, sim_spice_preferedkey);
				if (var == NOVARIABLE)
					var = getvalkey((INTBIG)subnt, VNODEPROTO, VSTRING, sim_spice_template_key);
				if (var != NOVARIABLE) continue;
			}
		}

		/* make sure the subcell hasn't already been written */
		dumpcell = 1;
		if (subnt->temp1 != 0) dumpcell = 0;
//		if (subnt->cell->temp1 != 0) dumpcell = 0;
		
		pname = 0;
		if (sim_spice_cdl || (sim_spice_state&SPICECELLPARAM) == 0)
		{
			pname = parameterizedname(ni, sim_spice_cellname(ni->proto));
			
			/* logical effort requires uniqueness for so marked insts, even if parameterized inst */
			var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, sim_spice_nameuniqueid);
			if (var != NOVARIABLE) 
			{
				/* make unique name out of hierarchy */
				gettraversalpath(ni->parent, el_curwindowpart, &hier, &indexlist, &depth, 0);
				infstr = initinfstr();
				for(i=0; i<depth; i++)
					addstringtoinfstr(infstr, describenodeinst(hier[i]));
				addstringtoinfstr(infstr, describenodeinst(ni));
				(void)allocstring(&pname, returninfstr(infstr), el_tempcluster);
				
				/* this ni has been marked to dump regardless of parameter state */
				dumpcell = 1;
			}
			if (pname != 0)
			{
				if (inparameterizedcells(subnt, pname) == 0)
				{
					dumpcell = 1;

					/* JKG CHANGE - use shorter subckt names if subckt name too long */
					shortname = 0;
					if (estrlen(pname) > SPICEMAXLENSUBCKTNAME)
					{
						infstr = initinfstr();
						formatinfstr(infstr, x_("%s_ID%ld"), sim_spice_cellname(ni->proto), sim_spiceNameUniqueID);
						shortname = returninfstr(infstr);
						sim_spiceNameUniqueID++;
					}
					addtoparameterizedcells(subnt, pname, shortname);
					if (shortname != 0)
					{
						efree((CHAR *)pname);
						(void)allocstring(&pname, shortname, el_tempcluster);
					}
				} else
					dumpcell = 0;
			}
		}

		if (dumpcell != 0)
		{
			downhierarchy(ni, subnt, 0);
			retval = sim_spice_writecell(subnt, FALSE, pname);
			uphierarchy();
			if (retval < 0) return(-1);
			if (retval > 0) backannotate = TRUE;
		}
		if (pname != 0) efree((CHAR *)pname);
	}

	/* mark this cell as written */
	if (paramname == 0)
	{
		np->temp1 = 1;
//		np->cell->temp1 = 1;
	}

	/* see if this cell has a disk file of SPICE associated with it */
	var = getval((INTBIG)np, VNODEPROTO, VSTRING, x_("SIM_spice_behave_file"));
	if (var != NOVARIABLE)
	{
		xprintf(sim_spice_file, x_("* Cell %s is described in this file:\n"), describenodeproto(np));
		sim_spice_addincludefile((CHAR *)var->addr);
		return(0);
	}

	/* find all networks in this cell, and find power and ground */
	netcount = sim_spice_getexportednetworks(np, &netlist, &sim_spice_vdd, &sim_spice_gnd, sim_spice_cdl);

	/* make sure power and ground appear at the top level */
	if (top && (sim_spice_state&SPICEGLOBALPG) == 0)
	{
		if (sim_spice_vdd == NONETWORK)
			ttyputerr(_("WARNING: cannot find power at top level of circuit"));
		if (sim_spice_gnd == NONETWORK)
			ttyputerr(_("WARNING: cannot find ground at top level of circuit"));
	}

	/* zero the temp1 for arcs and nodes to compute area */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst) ai->temp1 = 0;
	for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork) net->temp1 = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		ni->temp1 = ni->temp2 = 0;
	bipolars = nmostrans = pmostrans = 0;

	/* create linked list of electrical nets in this cell */
	sim_spice_firstnet = NOSPNET;

	/* must have a default node 0 in subcells */
	nodeindex = subcellindex = 1;
	sim_spice_netindex = 2;	   /* save 1 for the substrate */

	/* look at all arcs in the cell */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->temp1 != 0) continue;

		/* don't count non-electrical arcs */
		if (((ai->proto->userbits&AFUNCTION)>>AFUNCTIONSH) == APNONELEC) continue;

		/* ignore busses */
		if (ai->network->buswidth > 1) continue;

		/* add this net to the list */
		if (ai->network->temp1 != 0)
		{
			spnet = (SPNET*)ai->network->temp1;
		} else
		{
			spnet = sim_spice_allocspnet();
			if (spnet == NOSPNET) return(-1);
			spnet->network = ai->network;
			ai->network->temp1 = (INTBIG)spnet;
			spnet->netnumber = sim_spice_netindex++;
			spnet->nextnet = sim_spice_firstnet;
			sim_spice_firstnet = spnet;
		}

		/* reset */
		for (j = 0; j < sim_spice_layer_count; j++) sim_spice_extra_area[j] = 0.0;
		for (j = 0; j < DIFFTYPES; j++) sim_spice_diffusion_index[j] = -1;

		sim_spice_arcarea(spnet, ai); /* builds areas, etc */

		/* get merged polygons so far */
		sim_spice_cur_net = spnet;
		mrgdonecell(sim_spice_evalpolygon);
	}

	/* now add any unwired but exported ports to the list of nets */
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		/* determine the net number of the export */
		if (pp->network->temp1 != 0) continue;

		/* add this net to the list */
		spnet = sim_spice_allocspnet();
		if (spnet == NOSPNET) return(-1);
		spnet->network = pp->network;
		pp->network->temp1 = (INTBIG)spnet;
		spnet->netnumber = sim_spice_netindex++;

		/* reset */
		ni = pp->subnodeinst;
		for (j = 0; j < sim_spice_layer_count; j++) sim_spice_extra_area[j] = 0.0;
		for (j = 0; j < DIFFTYPES; j++) sim_spice_diffusion_index[j] = -1;
		sim_spice_nodearea(spnet, ni, pp->subportproto);

		/* get merged polygons so far */
		sim_spice_cur_net = spnet;
		mrgdonecell(sim_spice_evalpolygon);

		/* count the number of components on the net */
		state = nodefunction(ni);
		switch (state)
		{
			case NPTRAEMES:
			case NPTRA4EMES:
			case NPTRADMES:
			case NPTRA4DMES:
			case NPTRADMOS:
			case NPTRA4DMOS:
			case NPTRANMOS:
			case NPTRA4NMOS: nodetype = ISNTYPE; break;
			case NPTRAPMOS:
			case NPTRA4PMOS: nodetype = ISPTYPE; break;
			default:         nodetype = ISNONE;  break;
		}

		/* only count drains and source connections in counting transistors */
		if (nodetype == ISNONE)
			spnet->components[nodetype]++; /* We don't use this, anyhow */
		else
		{
			transistorports(ni, &gate, &gatedummy, &source, &drain);
			if (pp->subportproto == source || pp->subportproto == drain)
			{
				spnet->components[nodetype]++;
			}
		}
		spnet->nextnet = sim_spice_firstnet;
		sim_spice_firstnet = spnet;
	}

	/* create Spice net information for all remaining nets in the cell */
	for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
	{
		if (net->temp1 != 0) continue;

		/* add this net to the list */
		spnet = sim_spice_allocspnet();
		if (spnet == NOSPNET) return(-1);
		net->temp1 = (INTBIG)spnet;
		spnet->network = net;
		spnet->netnumber = sim_spice_netindex++;
		spnet->nextnet = sim_spice_firstnet;
		sim_spice_firstnet = spnet;
	}

	/* make sure the ground net is number zero */
	if (top && sim_spice_gnd != NONETWORK)
		((SPNET *)sim_spice_gnd->temp1)->netnumber = 0;

	posnet = negnet = subnet = NOSPNET;

	/* second pass through the node list */ 
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		state = nodefunction(ni);
		switch (state)
		{
			case NPTRANPN:
			case NPTRA4NPN:
			case NPTRAPNP:
			case NPTRA4PNP:
			case NPTRANS:
				nodetype = ISNONE;
				bipolars++;
				break;
			case NPTRAEMES:
			case NPTRA4EMES:
			case NPTRADMES:
			case NPTRA4DMES:
			case NPTRADMOS:
			case NPTRA4DMOS:
			case NPTRANMOS:
			case NPTRA4NMOS:
				nodetype = ISNTYPE;
				nmostrans++;
				break;
			case NPTRAPMOS:
			case NPTRA4PMOS:
				nodetype = ISPTYPE;
				pmostrans++;
				break;
			case NPSUBSTRATE:
				if (subnet == NOSPNET)
					subnet = sim_spice_getnet(ni, ni->proto->firstportproto->network);
				continue;
			case NPTRANSREF:
			case NPTRANJFET:
			case NPTRA4NJFET:
			case NPTRAPJFET:
			case NPTRA4PJFET:
			case NPRESIST:
			case NPCAPAC:
			case NPECAPAC:
			case NPINDUCT:
			case NPDIODE:
			case NPDIODEZ:
			case NPCONGROUND:
			case NPCONPOWER:
				nodetype = ISNONE;
				break;
			default:
		        continue;
		}

		/*
		 * find all wired ports on component and increment their count,
		 * but only if they are a drain or source
		 */
		if (nodetype != ISNONE)
		{
			transistorports(ni, &gate, &gatedummy, &source, &drain);
			for(spnet = sim_spice_firstnet; spnet != NOSPNET; spnet = spnet->nextnet)
			{
				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				{
					if (pi->proto != source) continue;
					if (spnet->network == pi->conarcinst->network) break;
				}
				if (pi != NOPORTARCINST) spnet->components[nodetype]++;
				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				{
					if (pi->proto != drain) continue;
					if (spnet->network == pi->conarcinst->network) break;
				}
				if (pi != NOPORTARCINST) spnet->components[nodetype]++;
			}
		}
	}

	/* use ground net for substrate */
	if (subnet == NOSPNET && sim_spice_gnd != NONETWORK)
		subnet = (SPNET *)sim_spice_gnd->temp1;

	if (pmostrans != 0 && posnet == NOSPNET)
	{
		if (sim_spice_vdd == NONETWORK)
		{
			infstr = initinfstr();
			formatinfstr(infstr, _("WARNING: no power connection for P-transistor wells in cell %s"),
				describenodeproto(np));
			sim_spice_dumpstringerror(infstr, 0);
		} else posnet = (SPNET *)sim_spice_vdd->temp1;
	}
	if (nmostrans != 0 && negnet == NOSPNET)
	{
		if (sim_spice_gnd == NONETWORK)
		{
			infstr = initinfstr();
			formatinfstr(infstr, _("WARNING: no connection for N-transistor wells in cell %s"),
				describenodeproto(np));
			sim_spice_dumpstringerror(infstr, 0);
		} else negnet = (SPNET *)sim_spice_gnd->temp1;
	}

	if (bipolars != 0 && subnet == NOSPNET)
	{
		infstr = initinfstr();
		formatinfstr(infstr, _("WARNING: no explicit connection to the substrate in cell %s"),
			describenodeproto(np));
		sim_spice_dumpstringerror(infstr, 0);
		if (sim_spice_gnd != NONETWORK)
		{
			ttyputmsg(_("     A connection to ground will be used if necessary."));
			subnet = (SPNET *)sim_spice_gnd->temp1;
		}
	}

	/* generate header for subckt or top-level cell */
	if (top && !sim_spice_cdl)
	{
		sim_spice_xprintf(sim_spice_file, TRUE, x_("\n*** TOP LEVEL CELL: %s\n"), describenodeproto(np));
		spCell = new SpiceCell();
	} else
	{
		CHAR *cellName = (paramname != 0 ? paramname : sim_spice_cellname(np));
		cellName = sim_spice_legalname( cellName );
		sim_spice_xprintf(sim_spice_file, FALSE, x_("\n*** CELL: %s\n.SUBCKT %s"), describenodeproto(np), cellName );
		spCell = new SpiceCell( cellName );
		for(i=0; i<netcount; i++)
		{
			net = netlist[i];
			if (net->globalnet >= 0) continue;
			if (sim_spice_cdl)
			{
				if (i > 0 && netlist[i-1]->temp2 != net->temp2)
					sim_spice_xprintf(sim_spice_file, FALSE, x_(" /"));
			}
			sim_spice_xprintf(sim_spice_file, FALSE, x_(" %s"), sim_spice_netname(net, 0, 0));
		}
		if ((sim_spice_state&SPICENODENAMES) == 0 || sim_spice_machine == SPICE3)
		{
			for (i = 0; i < np->globalnetcount; i++)
			{
				net = np->globalnetworks[i];
				if (net == NONETWORK) continue;
				sim_spice_xprintf(sim_spice_file, FALSE, x_(" %s"), sim_spice_netname(net, 0, 0));
			}
		}
		if (!sim_spice_cdl && (sim_spice_state&SPICECELLPARAM) != 0)
		{
			/* add in parameters to this cell */
			for(i=0; i<np->numvar; i++)
			{
				var = &np->firstvar[i];
				if (TDGETISPARAM(var->textdescript) == 0) continue;
				sim_spice_xprintf(sim_spice_file, FALSE, x_(" %s=%s"), truevariablename(var),
					describesimplevariable(var));
			}
		}
		sim_spice_xprintf(sim_spice_file, FALSE, x_("\n"));

		/* generate pin descriptions for reference (when not using node names) */
		for (i = 0; i < np->globalnetcount; i++)
		{
			net = np->globalnetworks[i];
			if (net == NONETWORK) continue;
			sim_spice_xprintf(sim_spice_file, TRUE, x_("** GLOBAL %s (network: %s)\n"),
				sim_spice_netname(net, 0, 0),
				sim_spice_describenetwork(net));
		}

		/* write exports to this cell */
		for(i=0; i<netcount; i++)
		{
			net = netlist[i];
			if (net == sim_spice_gnd || net == sim_spice_vdd) continue;
			sim_spice_xprintf(sim_spice_file, TRUE, x_("** PORT %s"), sim_spice_netname(net, 0, 0));
			if (net->namecount > 0)
				sim_spice_xprintf(sim_spice_file, TRUE, x_(" (network: %s)"), sim_spice_describenetwork(net));
			sim_spice_xprintf(sim_spice_file, TRUE, x_("\n"));
		}
	}

	/* add nodes to SpiceCell */
	for(spnet = sim_spice_firstnet; spnet != NOSPNET; spnet = spnet->nextnet)
	{
		spnet->spiceNet = new SpiceNet( spCell, spnet->netnumber, spnet->network );
	}

	/* now run through all components in the cell */
	resistnum = capacnum = inductnum = diodenum = 1;

	/* third pass through the node list, print it this time */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* handle sub-cell calls */
		if (ni->proto->primindex == 0)
		{
			/* ignore recursive references (showing icon in contents) */
			if (isiconof(ni->proto, np)) continue;

			/* see if the node is arrayed */
			nodewidth = ni->arraysize;
			if (nodewidth < 1) nodewidth = 1;

			/* look for a SPICE template on the prototype */
			vartemplate = getvalkey((INTBIG)ni->proto, VNODEPROTO, VSTRING, sim_spice_preferedkey);
			if (vartemplate == NOVARIABLE)
				vartemplate = getvalkey((INTBIG)ni->proto, VNODEPROTO, VSTRING, sim_spice_template_key);

			/* get the ports on this node (in proper order) */
			cnp = contentsview(ni->proto);
			if (cnp == NONODEPROTO) cnp = ni->proto; else
			{
				/* if there is a contents view, look for a SPICE template there, too */
				if (vartemplate == NOVARIABLE)
				{
					vartemplate = getvalkey((INTBIG)cnp, VNODEPROTO, VSTRING, sim_spice_preferedkey);
					if (vartemplate == NOVARIABLE)
						vartemplate = getvalkey((INTBIG)cnp, VNODEPROTO, VSTRING, sim_spice_template_key);
				}
			}
			netcount = sim_spice_getexportednetworks(cnp, &netlist, &subvdd, &subgnd, sim_spice_cdl);
			err = 0;

			/* if the node is arrayed, get the names of each instantiation */
			if (nodewidth > 1)
			{
				var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
				if (var == NOVARIABLE) nodewidth = 1; else
				{
					sigcount = net_evalbusname(APBUS, (CHAR *)var->addr, &nodenames,
						NOARCINST, NONODEPROTO, 0);
					if (sigcount != nodewidth) nodewidth = 1;
				}
			}

			/* loop through each arrayed instantiation of this node */
			for(nindex=0; nindex<nodewidth; nindex++)
			{
				for(net = cnp->firstnetwork; net != NONETWORK; net = net->nextnetwork)
					net->temp1 = (INTBIG)NONETWORK;
				for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				{
					cpp = equivalentport(ni->proto, pp, cnp);
					if (cpp == NOPORTPROTO) continue;
					net = NONETWORK;
					for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
						if (pi->proto == pp) break;
					if (pi != NOPORTARCINST) net = pi->conarcinst->network; else
					{
						for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
							if (pe->proto == pp) break;
						if (pe != NOPORTEXPINST) net = pe->exportproto->network;
					}
					if (net == NONETWORK) continue;
					if (net->buswidth > 1)
					{
						sigcount = net->buswidth;
						if (nodewidth > 1 && cpp->network->buswidth * nodewidth == net->buswidth)
						{
							/* map wide bus to a particular instantiation of an arrayed node */
							if (cpp->network->buswidth == 1)
							{
								cpp->network->temp1 = (INTBIG)net->networklist[nindex];
							} else
							{
								for(i=0; i<cpp->network->buswidth; i++)
									cpp->network->networklist[i]->temp1 =
										(INTBIG)net->networklist[i + nindex*cpp->network->buswidth];
							}
						} else
						{
							if (cpp->network->buswidth != net->buswidth)
							{
								ttyputerr(_("***ERROR: port %s on node %s in cell %s is %d wide, but is connected/exported with width %d"),
									pp->protoname, describenodeinst(ni), describenodeproto(np),
										cpp->network->buswidth, net->buswidth);
								sigcount = mini(sigcount, cpp->network->buswidth);
								if (sigcount == 1) sigcount = 0;
							}
							cpp->network->temp1 = (INTBIG)net;
							for(i=0; i<sigcount; i++)
								cpp->network->networklist[i]->temp1 = (INTBIG)net->networklist[i];
						}
					} else
					{
						cpp->network->temp1 = (INTBIG)net;
					}
				}

				/* handle self-defined models */
				if (vartemplate != NOVARIABLE)
				{
					infstr = initinfstr();
					for(pt = (CHAR *)vartemplate->addr; *pt != 0; pt++)
					{
						if (pt[0] != '$' || pt[1] != '(')
						{
							addtoinfstr(infstr, *pt);
							continue;
						}
						start = pt + 2;
						for(pt = start; *pt != 0; pt++)
							if (*pt == ')') break;
						save = *pt;
						*pt = 0;
						pp = getportproto(ni->proto, start);
						if (pp != NOPORTPROTO)
						{
							/* port name found: use its spice node */
							spnet = sim_spice_getnet(ni, pp->network);
							if (spnet == NOSPNET)
							{
								if ((sim_spice_state&SPICENODENAMES) != 0)
									formatinfstr(infstr, x_("UNNAMED%ld"), sim_spice_unnamednum++); else
										formatinfstr(infstr, x_("%ld"), sim_spice_netindex++);
								err++;
							} else
								addstringtoinfstr(infstr, sim_spice_netname(spnet->network, nodewidth, nindex));
						} else
						{
							/* no port name found, look for variable name */
							esnprintf(line, 100, x_("ATTR_%s"), start);
							var = getval((INTBIG)ni, VNODEINST, -1, line);
							if (var == NOVARIABLE)
								var = getval((INTBIG)ni, VNODEINST, -1, start);
							if (var == NOVARIABLE)
							{
								addstringtoinfstr(infstr, x_("??"));
								err++;
							} else
							{
								if (nodewidth > 1)
								{
									/* see if this name is arrayed, and pick a single entry from it if so */
									count = net_evalbusname(APBUS, describesimplevariable(var),
										&strings, NOARCINST, NONODEPROTO, 0);
									if (count == nodewidth)
										addstringtoinfstr(infstr, strings[nindex]); else
											addstringtoinfstr(infstr, describesimplevariable(var));
								} else
								{
									addstringtoinfstr(infstr, describesimplevariable(var));
								}
							}
						}
						*pt = save;
						if (save == 0) break;
					}
					spInst = new SpiceInst( spCell, returninfstr(infstr) );
					spInst->addParamM( ni );
					continue;
				}

				if (nodewidth > 1)
				{
					pt = sim_spice_elementname(ni, 'X', &subcellindex, nodenames[nindex]);
				} else
				{
					pt = sim_spice_elementname(ni, 'X', &subcellindex, 0);
				}
				spInst = new SpiceInst( spCell, pt );

				for(i=0; i<netcount; i++)
				{
					net = netlist[i];
					if (net->globalnet >= 0) continue;

					rnet = (NETWORK *)net->temp1;
					if (rnet == NONETWORK)
					{
						new SpiceBind( spInst );

						/* do not report errors on power or ground net */
						if (net == subvdd || net == subgnd) continue;
						for(pp = cnp->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
						{
							if ((pp->userbits&STATEBITS) != OUTPORT &&
								(pp->userbits&STATEBITS) != BIDIRPORT) continue;
							if (pp->network == net) break;
							if (pp->network->buswidth <= 1) continue;
							for(j=0; j<pp->network->buswidth; j++)
								if (pp->network->networklist[j] == net) break;
							if (j < pp->network->buswidth) break;
						}
						if (pp != NOPORTPROTO) continue;
						err++;
						continue;
					}
					spnet = (SPNET *)rnet->temp1;
					new SpiceBind( spInst, spnet->spiceNet );
					/*if (spnet->netnumber == 0 && (sim_spice_state&SPICENODENAMES) == 0) continue;*/
				}
				if ((sim_spice_state&SPICENODENAMES) == 0 || sim_spice_machine == SPICE3)
				{
					for (i = 0; i < cnp->globalnetcount; i++)
					{
						net = cnp->globalnetworks[i];
						if (net == NONETWORK) continue;
						if (i >= 2)
							i = net_findglobalnet(np, cnp->globalnetnames[i]);
						rnet = np->globalnetworks[i];
						spnet = (SPNET *)rnet->temp1;
						new SpiceBind( spInst, spnet->spiceNet );
					}
				}
				pname = 0;
				if (sim_spice_cdl || (sim_spice_state&SPICECELLPARAM) == 0)
				{
					pname = parameterizedname(ni, sim_spice_cellname(ni->proto));

					/* logical effort requires uniqueness for so marked insts, even if they are parameterized insts */
					var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, sim_spice_nameuniqueid);
					if (var != NOVARIABLE)
					{
						gettraversalpath(ni->parent, el_curwindowpart, &hier, &indexlist, &depth, 0);
						infstr = initinfstr();
						for(i=0; i<depth; i++)
							addstringtoinfstr(infstr, describenodeinst(hier[i]));
						addstringtoinfstr(infstr, describenodeinst(ni));
						(void)allocstring(&pname, returninfstr(infstr), el_tempcluster);
					}
				}
				CHAR *instCellName;
				if (pname == 0)
				{
					instCellName = sim_spice_cellname(ni->proto);
				} else
				{
					shortname = inparameterizedcells(ni->proto, pname);
					instCellName = (shortname == 0 ? pname : shortname);
				}
				instCellName = sim_spice_legalname( instCellName );
				SpiceCell *spinst = SpiceCell::findByName( instCellName );
				if (spinst != 0) spInst->setInstCell( spinst ); else
				{
					ttyputerr(_("Warning: cannot determine name of instance %s in cell %s"),
						instCellName, describenodeproto(np));
				}
				if (pname != 0) efree((CHAR *)pname);

				if (!sim_spice_cdl && (sim_spice_state&SPICECELLPARAM) != 0)
				{
					/* add in parameters to this instance */
					for(i=0; i<cnp->numvar; i++)
					{
						var = &cnp->firstvar[i];
						if (TDGETISPARAM(var->textdescript) == 0) continue;
						nivar = getvalkey((INTBIG)ni, VNODEINST, -1, var->key);
						CHAR *paramStr = (nivar == NOVARIABLE ? (CHAR*)x_("??") : describesimplevariable(nivar));
						spInst->addParam( new SpiceParam( paramStr ) );
					}
				}
				spInst->addParamM( ni );
			}
			if (err != 0)
			{
				infstr = initinfstr();
				formatinfstr(infstr, _("WARNING: subcell %s is not fully connected in cell %s"),
					describenodeinst(ni), describenodeproto(np));
				sim_spice_dumpstringerror(infstr, spInst);
			}
			continue;
		}

		/* get the type of this node */
		state = nodefunction(ni);

		/* handle resistors, inductors, capacitors, and diodes */
		if (state == NPRESIST || state == NPCAPAC || state == NPECAPAC || state == NPINDUCT ||
			state == NPDIODE || state == NPDIODEZ)
		{
			switch (state)
			{
				case NPRESIST:		/* resistor */
					var = getvalkey((INTBIG)ni, VNODEINST, -1, sch_resistancekey);
					if (var == NOVARIABLE) extra = x_(""); else
					{
						extra = describesimplevariable(var);
						if (isanumber(extra))
						{
							purevalue = (float)eatof(extra);
							extra = displayedunits(purevalue, VTUNITSRES, INTRESUNITOHM);
						}
					}
					sim_spice_writetwoport(ni, state, extra, spCell, &resistnum, 1);
					break;
				case NPCAPAC:
				case NPECAPAC:	/* capacitor */
					var = getvalkey((INTBIG)ni, VNODEINST, -1, sch_capacitancekey);
					if (var == NOVARIABLE) extra = x_(""); else
					{
						extra = describesimplevariable(var);
						if (isanumber(extra))
						{
							purevalue = (float)eatof(extra);
							extra = displayedunits(purevalue, VTUNITSCAP, INTCAPUNITFARAD);
						}
					}
					sim_spice_writetwoport(ni, state, extra, spCell, &capacnum, 1);
					break;
				case NPINDUCT:		/* inductor */
					var = getvalkey((INTBIG)ni, VNODEINST, -1, sch_inductancekey);
					if (var == NOVARIABLE) extra = x_(""); else
					{
						extra = describesimplevariable(var);
						if (isanumber(extra))
						{
							purevalue = (float)eatof(extra);
							extra = displayedunits(purevalue, VTUNITSIND, INTINDUNITHENRY);
						}
					}
					sim_spice_writetwoport(ni, state, extra, spCell, &inductnum, 1);
					break;
				case NPDIODE:		/* diode */
				case NPDIODEZ:		/* Zener diode */
					var = getvalkey((INTBIG)ni, VNODEINST, -1, sch_diodekey);
					if (var == NOVARIABLE) extra = x_(""); else
						extra = describesimplevariable(var);
					sim_spice_writetwoport(ni, state, extra, spCell, &diodenum, 1);
					break;
			}
			spInst->addParamM( ni );
			continue;
		}

		/* the default is to handle everything else as a transistor */
		if (state < NPTRANMOS || state > NPTRANS4)
			continue;

		/* get the source, gates, and drain for the transistor */
		transistorports(ni, &gate, &gatedummy, &source, &drain);

		/* determine the net numbers for these parts of the transistor */
		gaten = sim_spice_getnet(ni, gate->network);     /* base */
		sourcen = sim_spice_getnet(ni, source->network); /* emitter */
		drainn = sim_spice_getnet(ni, drain->network);   /* collector */

		/* make sure transistor is connected to nets */
		if (sourcen == NOSPNET || gaten == NOSPNET || drainn == NOSPNET)
		{
			infstr = initinfstr();
			formatinfstr(infstr, _("WARNING: %s not fully connected in cell %s"),
				describenodeinst(ni), describenodeproto(np));
			sim_spice_dumpstringerror(infstr, 0);
		}

		/* print source, gate, drain, and substrate */

		/* get any special model information */
		info = NULL;
		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, sch_spicemodelkey);
		if (var != NOVARIABLE) info = (CHAR *)var->addr;

		for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			biasport = pp;
		biasn = sim_spice_getnet(ni, biasport->network);
		CHAR tranChar = 0;
		switch (state)
		{
			case NPTRANSREF:			/* self-referential transistor */
				tranChar = 'X'; 
				biasn = negnet;
				info = sim_spice_cellname(ni->proto);
				break;
			case NPTRANMOS:			/* NMOS (Enhancement) transistor */
				tranChar = 'M';
				biasn = negnet;
				if (info == NULL) info = x_("N");
				break;
			case NPTRA4NMOS:		/* NMOS (Complementary) 4-port transistor */
				tranChar = 'M';
				if (info == NULL) info = x_("N");
				break;
			case NPTRADMOS:			/* DMOS (Depletion) transistor */
				tranChar = 'M';
				biasn = negnet;
				if (info == NULL) info = x_("D");
				break;
			case NPTRA4DMOS:		/* DMOS (Depletion) 4-port transistor */
				tranChar = 'M';
				if (info == NULL) info = x_("D");
				break;
			case NPTRAPMOS:			/* PMOS (Complementary) transistor */
				tranChar = 'M';
				biasn = posnet;
				if (info == NULL) info = x_("P");
				break;
			case NPTRA4PMOS:		/* PMOS (Complementary) 4-port transistor */
				tranChar = 'M';
				if (info == NULL) info = x_("P");
				break;
			case NPTRANPN:			/* NPN (Junction) transistor */
				tranChar = 'Q';
				biasn = subnet != NOSPNET ? subnet : 0;
				if (info == NULL) info = x_("NBJT");
				break;
			case NPTRA4NPN:			/* NPN (Junction) 4-port transistor */
				tranChar = 'Q';
				if (info == NULL) info = x_("NBJT");
				break;
			case NPTRAPNP:			/* PNP (Junction) transistor */
				tranChar = 'Q';
				biasn = subnet != NOSPNET ? subnet : 0;
				if (info == NULL) info = x_("PBJT");
				break;
			case NPTRA4PNP:			/* PNP (Junction) 4-port transistor */
				tranChar = 'Q';
				if (info == NULL) info = x_("PBJT");
				break;
			case NPTRANJFET:		/* NJFET (N Channel) transistor */
				tranChar = 'J';
				biasn = 0;
				if (info == NULL) info = x_("NJFET");
				break;
			case NPTRA4NJFET:		/* NJFET (N Channel) 4-port transistor */
				tranChar = 'J';
				if (info == NULL) info = x_("NJFET");
				break;
			case NPTRAPJFET:		/* PJFET (P Channel) transistor */
				tranChar = 'J';
				biasn = 0;
				if (info == NULL) info = x_("PJFET");
				break;
			case NPTRA4PJFET:		/* PJFET (P Channel) 4-port transistor */
				tranChar = 'J';
				if (info == NULL) info = x_("PJFET");
				break;
			case NPTRADMES:			/* DMES (Depletion) transistor */
			case NPTRA4DMES:		/* DMES (Depletion) 4-port transistor */
				tranChar = 'Z';
				biasn = 0;
				info = x_("DMES");
				break;
			case NPTRAEMES:			/* EMES (Enhancement) transistor */
			case NPTRA4EMES:		/* EMES (Enhancement) 4-port transistor */
				tranChar = 'Z';
				biasn = 0;
				info = x_("EMES");
				break;
			case NPTRANS:		/* special transistor */
				tranChar = 'Q';
				biasn = subnet != NOSPNET ? subnet : 0;
				break;
		}
		CHAR *tranName = sim_spice_elementname(ni, tranChar, (tranChar == 'X' ? &subcellindex : &nodeindex), 0);
		spInst = new SpiceInst( spCell, tranName );
		new SpiceBind( spInst, (drainn != NOSPNET ? drainn->spiceNet : 0) );
		new SpiceBind( spInst, (gaten != NOSPNET ? gaten->spiceNet : 0) );
		new SpiceBind( spInst, (sourcen != NOSPNET ? sourcen->spiceNet : 0) );
		if (biasn != 0)
			new SpiceBind( spInst, (biasn != NOSPNET ? biasn->spiceNet : 0) );
		if (info != NULL) spInst->addParam( new SpiceParam( info ) );

		/* do not print area for self-referential transistors */
		if (state == NPTRANSREF)
		{
			spInst->addParamM( ni );
			continue;
		}

		/* compute length and width (or area for nonMOS transistors) */
		nodelambda = lambdaofnode(ni);
		reallambda = ni->parent->lib->lambda[sim_spice_tech->techindex];
		transistorsize(ni, &lx, &ly);

		if (lx >= 0 && ly >= 0)
		{
			if( (sim_spice_state&SPICEUSELAMBDAS) == 0)
			{
				/* write sizes in microns */
				if (nodelambda != reallambda && nodelambda != 0)
				{
					lx = muldiv(lx, reallambda, nodelambda);
					ly = muldiv(ly, reallambda, nodelambda);
				}
				
				a = sim_spice_mask_scale * lx;
				b = sim_spice_mask_scale * ly;
				if (state == NPTRANMOS  || state == NPTRADMOS  || state == NPTRAPMOS ||
					state == NPTRA4NMOS || state == NPTRA4DMOS || state == NPTRA4PMOS ||
					((state == NPTRANJFET || state == NPTRAPJFET || state == NPTRADMES ||
					  state == NPTRAEMES) && (sim_spice_machine == SPICEHSPICE)))
				{
					esnprintf(line, 100, x_("L=%3.2fU"), scaletodispunit((INTBIG)a, DISPUNITMIC));
					spInst->addParam( new SpiceParam( line ) );
					esnprintf(line, 100, x_("W=%3.2fU"), scaletodispunit((INTBIG)b, DISPUNITMIC));
					spInst->addParam( new SpiceParam( line ) );
				}
				if (state != NPTRANMOS && state != NPTRADMOS  && state != NPTRAPMOS &&
					state != NPTRA4NMOS && state != NPTRA4DMOS && state != NPTRA4PMOS)
				{
					esnprintf(line, 100, x_("AREA=%3.2fP"), scaletodispunitsq((INTBIG)(a*b), DISPUNITMIC));
					spInst->addParam( new SpiceParam( line ) );
				}
			} else
				/* write sizes in lambda */
			{
				if (state == NPTRANMOS  || state == NPTRADMOS  || state == NPTRAPMOS ||
					state == NPTRA4NMOS || state == NPTRA4DMOS || state == NPTRA4PMOS ||
					((state == NPTRANJFET || state == NPTRAPJFET || state == NPTRADMES ||
					  state == NPTRAEMES) && (sim_spice_machine == SPICEHSPICE)))
				{
					esnprintf(line, 100, x_("L=%4.2f"), scaletodispunit((INTBIG)lx, DISPUNITLAMBDA));
					spInst->addParam( new SpiceParam( line ) );
					esnprintf(line, 100, x_("W=%4.2f"), scaletodispunit((INTBIG)ly, DISPUNITLAMBDA));
					spInst->addParam( new SpiceParam( line ) );
				}
				if (state != NPTRANMOS && state != NPTRADMOS  && state != NPTRAPMOS &&
					state != NPTRA4NMOS && state != NPTRA4DMOS && state != NPTRA4PMOS)
				{
					esnprintf(line, 100, x_("AREA=%4.2f"), scaletodispunitsq((INTBIG)(lx*ly), DISPUNITLAMBDA));
					spInst->addParam( new SpiceParam( line ) );
				}
			}				
		} else
		{
			/* if there is nonnumeric size on a schematic transistor, get it */
			varl = getvalkey((INTBIG)ni, VNODEINST, -1, el_attrkey_length);
			varw = getvalkey((INTBIG)ni, VNODEINST, -1, el_attrkey_width);
			if (varl != NOVARIABLE && varw != NOVARIABLE)
			{
				if( (sim_spice_state&SPICEUSELAMBDAS) == 0)
				{
					/* write sizes in microns */
					pt = describevariable(varl, -1, -1);
					if (isanumber(pt))
					{
						lx = muldiv(atofr(pt), nodelambda, WHOLE);
						if (nodelambda != reallambda && nodelambda != 0)
							lx = muldiv(lx, reallambda, nodelambda);
						a = sim_spice_mask_scale * lx;
						esnprintf(line, 100, x_("L=%3.2fU"), scaletodispunit((INTBIG)a, DISPUNITMIC));
					} else esnprintf(line, 100, x_("L=%s"), pt);
					spInst->addParam( new SpiceParam( line ) );
					pt = describevariable(varw, -1, -1);
					if (isanumber(pt))
					{
						lx = muldiv(atofr(pt), nodelambda, WHOLE);
						if (nodelambda != reallambda && nodelambda != 0)
							lx = muldiv(lx, reallambda, nodelambda);
						a = sim_spice_mask_scale * lx;
						esnprintf(line, 100, x_("W=%3.2fU"), scaletodispunit((INTBIG)a, DISPUNITMIC));
					} else esnprintf(line, 100, x_("W=%s"), pt);
					spInst->addParam( new SpiceParam( line ) );
				} else
				{
					/* write sizes in lambda */
					pt = describevariable(varl, -1, -1);
					if (isanumber(pt))
					{
						lx = atofr(pt);
						esnprintf(line, 100, x_("L=%4.2f"), scaletodispunit((INTBIG)lx, DISPUNITLAMBDA));
					} else esnprintf(line, 100, x_("L=%s"), pt);
					spInst->addParam( new SpiceParam( line ) );
					pt = describevariable(varw, -1, -1);
					if (isanumber(pt))
					{
						lx = atofr(pt);
						esnprintf(line, 100, x_("W=%4.2f"), scaletodispunit((INTBIG)lx, DISPUNITLAMBDA));
					} else esnprintf(line, 100, x_("W=%s"), pt);
					spInst->addParam( new SpiceParam( line ) );
				}
			}
		}
		spInst->addParamM( ni );

		/* make sure transistor is connected to nets */
		if (sourcen == NOSPNET || gaten == NOSPNET || drainn == NOSPNET) continue;

		/* compute area of source and drain */
		if (!sim_spice_cdl)
		{
			if (state == NPTRANMOS  || state == NPTRADMOS  || state == NPTRAPMOS ||
				state == NPTRA4NMOS || state == NPTRA4DMOS || state == NPTRA4PMOS)
			{
				switch (state)
				{
					case NPTRADMOS:
					case NPTRA4DMOS:
					case NPTRANMOS:
					case NPTRA4NMOS: i = ISNTYPE; break;
					case NPTRAPMOS:
					case NPTRA4PMOS: i = ISPTYPE; break;
					default:         i = ISNONE;  break;
				}

				/* we should not look at the ISNONE entry of components[],
				 * but the diffareas will be zero anyhow,
				 */
				if (sourcen->components[i] != 0)
				{
					a = scaletodispunitsq((INTBIG)(sourcen->diffarea[i] / sourcen->components[i]),
						DISPUNITMIC);
					if (a > 0.0)
					{
						esnprintf(line, 100, x_("AS=%5.2fP"), a);
						spInst->addParam( new SpiceParam( line ) );
					}
				}
				if (drainn->components[i] != 0)
				{
					b = scaletodispunitsq((INTBIG)(drainn->diffarea[i] / drainn->components[i]),
						DISPUNITMIC);
					if (b > 0.0)
					{
						esnprintf(line, 100, x_("AD=%5.2fP"), b);
						spInst->addParam( new SpiceParam( line ) );
					}
				}

				/* compute perimeters of source and drain */
				if (sourcen->components[i] != 0)
				{
					a = scaletodispunit((INTBIG)(sourcen->diffperim[i] / sourcen->components[i]),
						DISPUNITMIC);
					if (a > 0.0)
					{
						esnprintf(line, 100, x_("PS=%5.2fU"), a);
						spInst->addParam( new SpiceParam( line ) );
					}
				}
				if (drainn->components[i] != 0)
				{
					b = scaletodispunit((INTBIG)(drainn->diffperim[i] / drainn->components[i]),
						DISPUNITMIC);
					if (b > 0.0)
					{
						esnprintf(line, 100, x_("PD=%5.2fU"), b);
						spInst->addParam( new SpiceParam( line ) );
					}
				}
			}
		}
	}
	spCell->printSpice();

	/* print resistances and capacitances */
	if (!sim_spice_cdl)
	{
		if ((sim_spice_state&SPICERESISTANCE) != 0)
		{
			/* print parasitic capacitances */
			first = 1;
			for(spnet = sim_spice_firstnet; spnet != NOSPNET; spnet = spnet->nextnet)
			{
				spnet->resistance = scaletodispunitsq((INTBIG)spnet->resistance, DISPUNITMIC);
				if (spnet->resistance > sim_spice_min_resist)
				{
					if (first != 0)
					{
						first = 0;
						sim_spice_xprintf(sim_spice_file, TRUE, x_("** Extracted Parasitic Elements:\n"));
					}
					sim_spice_xprintf(sim_spice_file, FALSE, x_("R%ld ? ? %9.2f\n"), resistnum++, spnet->resistance);
				}

				if (spnet->network == sim_spice_gnd) continue;
				if (spnet->capacitance > sim_spice_min_capac)
				{
					if (first != 0)
					{
						first = 0;
						sim_spice_xprintf(sim_spice_file, TRUE, x_("** Extracted Parasitic Elements:\n"));
					}
					sim_spice_xprintf(sim_spice_file, FALSE, x_("C%ld%s 0 %9.2fF\n"), capacnum++,
						sim_spice_nodename(spnet), spnet->capacitance);
				}
			}
		}
	}

	/* write out any directly-typed SPICE cards */
	if (sim_spice_card_key == 0)
		sim_spice_card_key = makekey(x_("SIM_spice_card"));
	spCell->setup();
	if (top && !sim_spice_cdl && sim_spice_machine == SPICEGNUCAP)
		SpiceCell::traverseAll();
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto != gen_invispinprim) continue;
		var = getvalkey((INTBIG)ni, VNODEINST, -1, sim_spice_card_key);
		if (var == NOVARIABLE) continue;
		if ((var->type&VTYPE) != VSTRING) continue;
		if ((var->type&VDISPLAY) == 0) continue;
		if ((var->type&VISARRAY) == 0)
		{
			sim_spice_xprintf(sim_spice_file, FALSE, x_("%s\n"), (CHAR *)var->addr);
		} else
		{
			len = getlength(var);
			for(i=0; i<len; i++)
				sim_spice_xprintf(sim_spice_file, FALSE, x_("%s\n"), ((CHAR **)var->addr)[i]);
		}
	}

	/*
	 * Now we're finished writing the subcircuit.
	 * Only the top-level cell can contain meters and connections.
	 */
	if (top && !sim_spice_cdl)
	{
		/* if no voltmeters, print metered sources anyway */
		if (*sim_spice_printcard != '\0')
		{
			sim_spice_puts((CHAR *)(sim_spice_state&SPICEPLOT ? x_(".PLOT") : x_(".PRINT")),
				FALSE);

			if (sim_spice_tran) sim_spice_puts(x_(" TRAN"), FALSE); else
				if (sim_spice_dc) sim_spice_puts(x_(" DC"), FALSE); else
					if (sim_spice_ac) sim_spice_puts(x_(" AC"), FALSE);

			/* write what we have already (from metered sources) */
			sim_spice_puts(sim_spice_printcard, FALSE);
			sim_spice_xprintf(sim_spice_file, FALSE, x_("\n"));
		}
		efree(sim_spice_printcard);

		/* miscellaneous checks */
		for(spnet = sim_spice_firstnet; spnet != NOSPNET; spnet = spnet->nextnet)
		{
			for (i = 0; i < DIFFTYPES; i++)
			{
				if (spnet->diffarea[i] == 0.0 || spnet->components[i] != 0) continue;

				/* do not issue errors for active area on supply rails (probably well contacts) */
				if (spnet->network == sim_spice_vdd || spnet->network == sim_spice_gnd) continue;
				switch (i)
				{
					case ISNTYPE:
						uncon_diff_type = x_(" N-type");
						break;
					case ISPTYPE:
						uncon_diff_type = x_(" P-type");
						break;
					case ISNONE:
					default:
						uncon_diff_type = x_("");
						break;
				}
				infstr = initinfstr();
				formatinfstr(infstr, _("WARNING: SPICE node%s has unconnected%s device diffusion in cell %s"),
					sim_spice_nodename(spnet), uncon_diff_type, describenodeproto(np));
				sim_spice_dumpstringerror(infstr, 0);
			}
		}
	} else
	{
		if (paramname != 0)
		{
			/* parameterized subcircuit name */
			sim_spice_xprintf(sim_spice_file, FALSE, x_(".ENDS %s\n"), sim_spice_legalname(paramname));
		} else
		{
			sim_spice_xprintf(sim_spice_file, FALSE, x_(".ENDS %s\n"), sim_spice_legalname(sim_spice_cellname(np)));
		}
	}

	/* free the net modules */
	for(spnet = sim_spice_firstnet; spnet != NOSPNET; spnet = nspnet)
	{
		nspnet = spnet->nextnet;
		sim_spice_freespnet(spnet);
	}
	if (backannotate) return(1);
	return(0);
}

/******************** DECK GENERATION SUPPORT ********************/

/*
 * write a header for "cell" to spice deck "sim_spice_file"
 * The model cards come from a file specified by tech:~.SIM_spice_model_file
 * or else tech:~.SIM_spice_header_level%ld
 * The spice model file can be located in el_libdir
 */
void sim_spice_writeheader(NODEPROTO *cell)
{
	REGISTER INTBIG i, j, level, pathlen, liblen, lambda;
	time_t cdate, rdate;
	REGISTER VARIABLE *var;
	CHAR hvar[80], *truefile;
	REGISTER CHAR **name, *pt;
	REGISTER void *infstr;
	CHAR headerpath[500];
	FILE *io;

	/* Print the header line for SPICE  */
	sim_spice_xprintf(sim_spice_file, TRUE, x_("*** SPICE deck for cell %s from library %s\n"),
		describenodeproto(cell), cell->lib->libname);
	us_emitcopyright(sim_spice_file, x_("*** "), x_(""));
	if ((us_useroptions&NODATEORVERSION) == 0)
	{
		cdate = (time_t)cell->creationdate;
		if (cdate != 0)
			sim_spice_xprintf(sim_spice_file, TRUE, x_("*** Created on %s\n"), timetostring(cdate));
		rdate = (time_t)cell->revisiondate;
		if (rdate != 0)
			sim_spice_xprintf(sim_spice_file, TRUE, x_("*** Last revised on %s\n"), timetostring(rdate));
		(void)esnprintf(hvar, 80, x_("%s"), timetostring(getcurrenttime()));
		sim_spice_xprintf(sim_spice_file, TRUE, x_("*** Written on %s by Electric VLSI Design System, %s\n"),
			hvar, el_version);
	} else
	{
		sim_spice_xprintf(sim_spice_file, TRUE, x_("*** Written by Electric VLSI Design System\n"));
	}

	sim_spice_xprintf(sim_spice_file, TRUE, x_("*** UC SPICE *** , MIN_RESIST %f, MIN_CAPAC %fFF\n"),
		sim_spice_min_resist, sim_spice_min_capac);
	sim_spice_puts(x_(".OPTIONS NOMOD NOPAGE\n"), FALSE);

	/* if sizes to be written in lambda, tell spice conversion factor */
	if( (sim_spice_state&SPICEUSELAMBDAS) != 0)
	{
		lambda = cell->lib->lambda[sim_spice_tech->techindex];
		sim_spice_xprintf(sim_spice_file, FALSE, x_("*** Lambda Conversion ***\n"));
		sim_spice_xprintf(sim_spice_file, FALSE, x_(".opt scale=%3.3fU\n\n"), scaletodispunit((INTBIG)lambda, DISPUNITMIC));
	}

	/* see if spice model/option cards from file if specified */
	var = getval((INTBIG)sim_spice_tech, VTECHNOLOGY, VSTRING, x_("SIM_spice_model_file"));
	if (var != NOVARIABLE)
	{
		pt = (CHAR *)var->addr;
		if (estrncmp(pt, x_(":::::"), 5) == 0)
		{
			/* extension specified: look for a file with the cell name and that extension */
			estrcpy(headerpath, truepath(cell->lib->libfile));
			pathlen = estrlen(headerpath);
			liblen = estrlen(skippath(headerpath));
			if (liblen < pathlen) headerpath[pathlen-liblen-1] = 0; else
				headerpath[0] = 0;
			infstr = initinfstr();
			formatinfstr(infstr, x_("%s%c%s.%s"), headerpath, DIRSEP, sim_spice_cellname(cell), &pt[5]);
			pt = returninfstr(infstr);
			if (fileexistence(pt) == 1)
			{
				xprintf(sim_spice_file, x_("* Model cards are described in this file:\n"));
				sim_spice_addincludefile(pt);
				return;
			}
		} else
		{
			/* normal header file specified */
			xprintf(sim_spice_file, x_("* Model cards are described in this file:\n"));
			io = xopen(pt, el_filetypetext, el_libdir, &truefile);
			if (io == 0)
			{
				ttyputmsg(_("Warning: cannot find model file '%s'"), pt);
			} else
			{
				pt = truefile;
			}
			sim_spice_addincludefile(pt);
			return;
		}
	}

	/* no header files: write predefined header for this level and technology */
	var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_spice_levelkey);
	if (var == NOVARIABLE) level = 1; else
		level = var->addr;
	(void)esnprintf(hvar, 80, x_("SIM_spice_header_level%ld"), level);
	var = getval((INTBIG)sim_spice_tech, VTECHNOLOGY, VSTRING|VISARRAY, hvar);
	if (var != NOVARIABLE)
	{
		name = (CHAR **)var->addr;
		j = getlength(var);
		for(i=0; i<j; i++) sim_spice_xprintf(sim_spice_file, FALSE, x_("%s\n"), name[i]);
	} else ttyputerr(_("WARNING: no model cards for SPICE level %ld in %s technology"),
		level, sim_spice_tech->techname);
}

/*
 * Write a trailer from an external file, defined as a variable on
 * the current technology in this library: tech:~.SIM_spice_trailer_file
 * if it is available.
 */
void sim_spice_writetrailer(NODEPROTO *cell)
{
	REGISTER INTBIG pathlen, liblen;
	VARIABLE *var;
	REGISTER void *infstr;
	REGISTER CHAR *pt;
	CHAR trailerpath[500];

	/* get spice trailer cards from file if specified */
	var = getval((INTBIG)sim_spice_tech, VTECHNOLOGY, VSTRING, x_("SIM_spice_trailer_file"));
	if (var != NOVARIABLE)
	{
		pt = (CHAR *)var->addr;
		if (estrncmp(pt, x_(":::::"), 5) == 0)
		{
			/* extension specified: look for a file with the cell name and that extension */
			estrcpy(trailerpath, truepath(cell->lib->libfile));
			pathlen = estrlen(trailerpath);
			liblen = estrlen(skippath(trailerpath));
			if (liblen < pathlen) trailerpath[pathlen-liblen-1] = 0; else
				trailerpath[0] = 0;
			infstr = initinfstr();
			formatinfstr(infstr, x_("%s%c%s.%s"), trailerpath, DIRSEP, sim_spice_cellname(cell), &pt[5]);
			pt = returninfstr(infstr);
			if (fileexistence(pt) == 1)
			{
				xprintf(sim_spice_file, x_("* Trailer cards are described in this file:\n"));
				sim_spice_addincludefile(pt);
			}
		} else
		{
			/* normal trailer file specified */
			xprintf(sim_spice_file, x_("* Trailer cards are described in this file:\n"));
			sim_spice_addincludefile((CHAR *)var->addr);
		}
	}
}

/*
 * Function to write a two port device to the file. If the flag 'report'
 * is set, then complain about the missing connections.
 * Determine the port connections from the portprotos in the instance
 * prototype. Get the part number from the 'part' number value;
 * increment it. The type of device is declared in type; extra is the string
 * data acquired before calling here.
 * If the device is connected to the same net at both ends, do not
 * write it. Is this OK?
 */
void sim_spice_writetwoport(NODEINST *ni, INTBIG type, CHAR *extra,
	SpiceCell *cell, INTBIG *part, INTBIG report)
{
	REGISTER PORTPROTO *pp1, *pp2;
	REGISTER SPNET *end1, *end2;
	REGISTER void *infstr;
	CHAR partChar;

	pp1 = ni->proto->firstportproto;
	pp2 = pp1->nextportproto;
	end1 = sim_spice_getnet(ni, pp1->network);
	end2 = sim_spice_getnet(ni, pp2->network);

	/* make sure the component is connected to nets */
	if (end1 == NOSPNET || end2 == NOSPNET)
	{
		infstr = initinfstr();
		formatinfstr(infstr, _("WARNING: %s component not fully connected in cell %s"),
			describenodeinst(ni), describenodeproto(ni->parent));
		sim_spice_dumpstringerror(infstr, 0);
	}
	if (end1 != NOSPNET && end2 != NOSPNET)
		if (end1->netnumber == end2->netnumber)
	{
		if (report)
		{
			infstr = initinfstr();
			formatinfstr(infstr, _("WARNING: %s component appears to be shorted on net %s in cell %s"),
				describenodeinst(ni), sim_spice_nodename(end1), describenodeproto(ni->parent));
			sim_spice_dumpstringerror(infstr, 0);
		}
		return;
	}

	/* next line is not really necessary any more */
	switch (type)
	{
		case NPRESIST:		/* resistor */
			partChar = 'R';
			break;
		case NPCAPAC:	/* capacitor */
		case NPECAPAC:
			partChar = 'C';
			break;
		case NPINDUCT:		/* inductor */
			partChar = 'L';
			break;
		case NPDIODE:		/* diode */
		case NPDIODEZ:		/* Zener diode */
			partChar = 'D';
			break;
		default:
			return;
	}
	SpiceInst *spInst = new SpiceInst( cell, sim_spice_elementname(ni, partChar, part, 0));

	new SpiceBind( spInst, (end2 != NOSPNET ? end2->spiceNet : 0) );
	new SpiceBind( spInst, (end1 != NOSPNET ? end1->spiceNet : 0) );  /* note order */
	if ((type == NPDIODE || type == NPDIODEZ) && extra[0] == 0) extra = x_("DIODE");
	spInst->addParam( new SpiceParam( extra ) );
}

/******************** PARASITIC CALCULATIONS ********************/

/*
 * routine to recursively determine the area of diffusion and capacitance
 * associated with port "pp" of nodeinst "ni".  If the node is mult_layer, then
 * determine the dominant capacitance layer, and add its area; all other
 * layers will be added as well to the extra_area total.
 * Continue out of the ports on a complex cell
 */
void sim_spice_nodearea(SPNET *spnet, NODEINST *ni, PORTPROTO *pp)
{
	REGISTER INTBIG tot, i, function;
	REGISTER INTBIG fun;
	BOOLEAN isabox;
	XARRAY trans;
	INTBIG lx, hx, ly, hy;
	INTBIG dominant;
	REGISTER POLYGON *poly, *lastpoly, *firstpoly;
	REGISTER PORTARCINST *pi;
	REGISTER TECHNOLOGY *tech;
	float worst, cap;

	/* make sure this node hasn't been completely examined */
	if (ni->temp1 == 2) return;
	if (ni->temp2 == (INTBIG)pp->network) return;	/* Is this recursive? */
	ni->temp2 = (INTBIG)pp->network;

	/* cells have no area or capacitance (for now) */
	if (ni->proto->primindex != 0)  /* No area for complex nodes */
	{
		/* assign new state of this node */
		function = nodefunction(ni);
		if (function == NPCONGROUND || function == NPCONPOWER)
			ni->temp1 = 2; else
				ni->temp1 = 1;

		/* initialize to examine the polygons on this node */
		tech = ni->proto->tech;
		makerot(ni, trans);

		/*
		 * NOW!  A fudge to make sure that well capacitors mask out the capacity
		 * to substrate of their top plate polysilicon  or metal
		 */
		if (function == NPCAPAC || function == NPECAPAC) dominant = -1; else dominant = -2;
		if (function == NPTRANMOS || function == NPTRA4NMOS ||
			function == NPTRAPMOS || function == NPTRA4PMOS ||
			function == NPTRADMOS || function == NPTRA4DMOS ||
			function == NPTRAEMES || function == NPTRA4EMES ||
			function == NPTRADMES || function == NPTRA4DMES)
				function = NPTRANMOS;   /* One will do */

		/* make linked list of polygons */
		lastpoly = firstpoly = NOPOLYGON;
		tot = nodeEpolys(ni, 0, NOWINDOWPART);
		for(i=0; i<tot; i++)
		{
			poly = sim_spice_allocpolygon();
			if (poly == NOPOLYGON) break;
			shapeEnodepoly(ni, i, poly);

			/* make sure this layer connects electrically to the desired port */
			if (poly->portproto == NOPORTPROTO)
			{
				sim_spice_freepolygon(poly);   continue;
			}
			if (poly->portproto->network != pp->network)
			{
				sim_spice_freepolygon(poly);   continue;
			}

			/* don't bother with layers without capacity */
			if ((sim_spice_layerisdiff(tech, poly->layer) == ISNONE) &&
				(sim_spice_capacitance(tech, poly->layer) == 0.0))
			{
				sim_spice_freepolygon(poly);   continue;
			}

			/* leave out the gate capacitance of transistors */
			if (function == NPTRANMOS)
			{
				fun = layerfunction(tech, poly->layer);
				if ((fun & LFPSEUDO) == 0 && layerispoly(fun))
				{
					sim_spice_freepolygon(poly);   continue;
				}
			}
			if (lastpoly != NOPOLYGON) lastpoly->nextpolygon = poly; else
				firstpoly = poly;
			lastpoly = poly;
		}

		/* do we need to test the layers? */
		if (dominant != -1)
		{
			if (tot != 0 && firstpoly != NOPOLYGON) dominant = firstpoly->layer;

			/* find the layer that will contribute the maximum capacitance */
			if (tot > 1 && tech == el_curtech)
			{
				worst = 0.0;
				for(poly = firstpoly; poly != NOPOLYGON; poly = poly->nextpolygon)
				{
					if (sim_spice_layerisdiff(tech, poly->layer) != ISNONE)
					{
						dominant = -1;      /* flag for diffusion on this port */
						break;
					} else
					{
						cap = (float)fabs(areapoly(poly));
						if (cap * sim_spice_capacitance(tech, poly->layer) > worst)
						{
							worst = cap;
							dominant = poly->layer;
						}
					}
				}
			}
		}

		for(poly = firstpoly; poly != NOPOLYGON; poly = poly->nextpolygon)
		{
			/* get the area of this polygon */
			xformpoly(poly, trans);
			isabox = isbox(poly, &lx, &hx, &ly, &hy);
			if (tech != el_curtech || !isabox) continue;
			sim_spice_storebox(tech, poly->layer, hx-lx, hy-ly, (lx+hx)/2, (ly+hy)/2);
			if (sim_spice_layerisdiff(tech, poly->layer) == ISNONE &&
				poly->layer != dominant)
					sim_spice_extra_area[poly->layer] += (float)fabs(areapoly(poly));
		}

		/* free the polygons */
		while (firstpoly != NOPOLYGON)
		{
			poly = firstpoly;
			firstpoly = firstpoly->nextpolygon;
			sim_spice_freepolygon(poly);
		}
	}
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		if (pi->proto->network == pp->network)
			sim_spice_arcarea(spnet, pi->conarcinst);
	ni->temp2 = 0;      /* reset for next net pass */
}

/*
 * routine to recursively determine the area of diffusion, capacitance, (NOT
 * resistance) on arc "ai". If the arc contains active device diffusion, then
 * it will contribute to the area of sources and drains, and the other layers
 * will be ignored. This is not quite the same as the rule used for
 * contact (node) structures. Note: the earlier version of this
 * function assumed that diffusion arcs would always have zero capacitance
 * values for the other layers; this produces an error if any of these layers
 * have non-zero values assigned for other reasons. So we will check for the
 * function of the arc, and if it contains active device, we will ignore any
 * other layers
 */
void sim_spice_arcarea(SPNET *spnet, ARCINST *ai)
{
	REGISTER INTBIG i, tot;
	REGISTER TECHNOLOGY *tech;
	static POLYGON *poly = NOPOLYGON;
	INTBIG lx, hx, ly, hy;
	BOOLEAN j;
	INTBIG isdiffarc;

	if (ai->temp1 != 0) return;
	ai->temp1++;

	(void)needstaticpolygon(&poly, 4, sim_tool->cluster);

	tot = arcpolys(ai, NOWINDOWPART);
	tech = ai->proto->tech;
	isdiffarc = sim_spice_arcisdiff(ai);    /* check arc function */
	for(i=0; i<tot; i++)
	{
		shapearcpoly(ai, i, poly);
		j = isbox(poly, &lx, &hx, &ly, &hy);
		if (tech != el_curtech || !j) continue;
		if ((layerfunction(tech, poly->layer)&LFPSEUDO) != 0) continue;
		if (sim_spice_layerisdiff(tech, poly->layer) != ISNONE ||
			(isdiffarc == 0 && sim_spice_capacitance(tech, poly->layer) > 0.0))
				sim_spice_storebox(tech, poly->layer, hx-lx, hy-ly, (lx+hx)/2, (ly+hy)/2);
	}

	/* propagate to all of the nodes on this arc */
	for(i=0; i<2; i++)
		sim_spice_nodearea(spnet, ai->end[i].nodeinst, ai->end[i].portarcinst->proto);
}

/*
 * Routine to store a box on layer "layer" into the box merging system
 */
void sim_spice_storebox(TECHNOLOGY *tech, INTBIG layer, INTBIG xs, INTBIG ys, INTBIG xc, INTBIG yc)
{
	INTBIG type;

	if ((type = sim_spice_layerisdiff(tech, layer)) != ISNONE)
	{
		if (sim_spice_diffusion_index[type] < 0)
			sim_spice_diffusion_index[type] = layer; else
				layer = sim_spice_diffusion_index[type];
	}
	mrgstorebox(layer, tech, xs, ys, xc, yc);
}

/*
 * routine to obtain a polygon from the box merging system
 */
void sim_spice_evalpolygon(INTBIG layer, TECHNOLOGY *tech, INTBIG *xbuf, INTBIG *ybuf, INTBIG count)
{
	REGISTER INTBIG perim;
	float area;
	REGISTER INTBIG i, j;

	/* compute perimeter */
	perim = 0;
	for(i=0; i<count; i++)
	{
		if (i == 0) j = count-1; else j = i-1;
		perim += computedistance(xbuf[j], ybuf[j], xbuf[i], ybuf[i]);
	}

	/* get area */
	area = areapoints(count, xbuf, ybuf);
	if (sim_spice_extra_area[layer] != 0.0)
	{
		area -= sim_spice_extra_area[layer];
		sim_spice_extra_area[layer] = 0.0; /* but only once */
	}

	i = sim_spice_layerisdiff(tech, layer);
	if (i != ISNONE)
	{
		sim_spice_cur_net->diffarea[i] += area * sim_spice_mask_scale * sim_spice_mask_scale;
		sim_spice_cur_net->diffperim[i] += perim * sim_spice_mask_scale;
	} else
	{
		sim_spice_cur_net->capacitance += scaletodispunitsq((INTBIG)(sim_spice_capacitance(
			tech, layer) * area), DISPUNITMIC) *
				sim_spice_mask_scale * sim_spice_mask_scale;
		sim_spice_cur_net->capacitance += scaletodispunit((INTBIG)(sim_spice_edgecapacitance(
			tech, layer) * perim), DISPUNITMIC) *
				sim_spice_mask_scale;
	}
}

/*
 * routine to find the capacitance for the arc on layer "layer"
 */
float sim_spice_capacitance(TECHNOLOGY *tech, INTBIG layer)
{
	if (layer < 0 || tech->temp2 == 0) return(0.0);
	return(castfloat(((INTBIG *)tech->temp2)[layer]));
}

/*
 * Routine to return the fringing capacitance of layer "layer" in tech "tech"
 */
float sim_spice_edgecapacitance(TECHNOLOGY *tech, INTBIG layer)
{
	REGISTER INTBIG addr;
	REGISTER VARIABLE *var;
	REGISTER TECHNOLOGY *t;

	if (layer < 0) return(0.0);
	if (sim_spice_capacvalue == 0)
	{
		sim_spice_capacvalue = emalloc(el_maxtech * SIZEOFINTBIG, sim_tool->cluster);
		if (sim_spice_capacvalue == 0) return(0.0);
		for(t = el_technologies; t != NOTECHNOLOGY; t = t->nexttechnology)
		{
			var = getval((INTBIG)t, VTECHNOLOGY, VFLOAT|VISARRAY, x_("sim_spice_edgecapacitance"));
			sim_spice_capacvalue[t->techindex] = (var == NOVARIABLE ? 0 : var->addr);
		}
	}
	addr = sim_spice_capacvalue[tech->techindex];
	if (addr == 0) return(0.0);
	return(castfloat(((INTBIG *)(addr))[layer]));
}

/******************** TEXT ROUTINES ********************/

/*
 * Routine to insert an "include" of file "filename" into the stream "io".
 */
void sim_spice_addincludefile(CHAR *filename)
{
	switch (sim_spice_machine)
	{
		case SPICE2:
		case SPICE3:
		case SPICEGNUCAP:
		case SPICESMARTSPICE:
			xprintf(sim_spice_file, x_(".include %s\n"), filename);
			break;
		case SPICEHSPICE:
			xprintf(sim_spice_file, x_(".include '%s'\n"), filename);
			break;
		case SPICEPSPICE:
			xprintf(sim_spice_file, x_(".INC %s\n"), filename);
			break;
	}
}

/*
 * Routine to convert "name" to a legal SPICE name (converting illegal characters
 * to "_") and return that name.
 */
CHAR *sim_spice_legalname(CHAR *name)
{
	REGISTER CHAR *pt;
	REGISTER INTBIG i;
	REGISTER void *infstr;

	for(pt = name; *pt != 0; pt++)
	{
		if (isalnum(*pt) != 0) continue;
		for(i=0; sim_spicelegalchars[i] != 0; i++)
			if (*pt == sim_spicelegalchars[i]) break;
		if (sim_spicelegalchars[i] == 0) break;
	}
	if (*pt == 0) return(name);

	/* convert the name */
	infstr = initinfstr();
	for(pt = name; *pt != 0; pt++)
	{
		if (isalnum(*pt) != 0)
		{
			addtoinfstr(infstr, *pt);
			continue;
		}
		for(i=0; sim_spicelegalchars[i] != 0; i++)
			if (*pt == sim_spicelegalchars[i]) break;
		if (sim_spicelegalchars[i] != 0)
		{
			addtoinfstr(infstr, *pt);
			continue;
		}
		addtoinfstr(infstr, '_');
	}
	return(returninfstr(infstr));
}

/*
 * Function to return a spice "element" name.
 * The first character (eg. R,L,C,D,X,...) is specified by "first".
 * the rest of the name comes from the name on inst "ni".
 * If there is no node name, a unique number specified by "counter" is used
 * and counter is incremented.
 *
 * Warning: This routine is not re-entrant.  You must use the returned string
 * before calling the routine again.
 */
CHAR *sim_spice_elementname(NODEINST *ni, CHAR first, INTBIG *counter, CHAR *overridename)
{
	VARIABLE *varname;
	static CHAR s[200];

	if (overridename != 0)
	{
		(void)esnprintf(s, 200, x_("%c%s"), first, sim_spice_legalname(overridename));
	} else
	{
		varname = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
		if (varname == NOVARIABLE)
		{
			ttyputerr(_("WARNING: no name on node %s"), describenodeinst(ni));
			(void)esnprintf(s, 200, x_("%c%ld"), first, (*counter)++);
		} else
		{
			(void)esnprintf(s, 200, x_("%c%s"), first, sim_spice_legalname((CHAR *)varname->addr));
		}
	}

	return(s);
}

/*
 * Routine to return the net name of SPICE net "spnet".
 * Unknown nets are assigned as node '*'.
 *
 * Warning: This routine is not re-entrant.  You must use the returned string
 * before calling the routine again.
 */
CHAR *sim_spice_nodename(SPNET *spnet)
{
	REGISTER NETWORK *net;
	REGISTER void *infstr;

	if (spnet == NOSPNET) net = NONETWORK; else
		net = spnet->network;
	infstr = initinfstr();
	formatinfstr(infstr, x_(" %s"), sim_spice_netname(net, 0, 0));
	return(returninfstr(infstr));
}

/*
 * Routine to return the net name of net "net".
 */
CHAR *sim_spice_netname(NETWORK *net, INTBIG bussize, INTBIG busindex)
{
	static CHAR s[80];
	REGISTER SPNET *spnet;
	REGISTER INTBIG count;
	CHAR *pt, **strings;

	if (net == NONETWORK)
	{
		if ((sim_spice_state&SPICENODENAMES) != 0)
			esnprintf(s, 80, x_("UNNAMED%ld"), sim_spice_unnamednum++); else
				esnprintf(s, 80, x_("%ld"), sim_spice_netindex++);
		return(s);
	}

	if ((sim_spice_state&SPICENODENAMES) != 0)
	{
		/* SPICE3, HSPICE, or PSPICE only */
		if (sim_spice_machine == SPICE3)
		{
			/* SPICE3 treats Ground as "0" in top-level cell */
			if (net == sim_spice_gnd && net->parent == sim_simnt) return(x_("0"));
		}
		if (net->globalnet >= 0) return(sim_spice_describenetwork(net));
		if (net->namecount > 0)
		{
			pt = networkname(net, 0);
			if (isdigit(pt[0]) == 0)
			{
				if (bussize > 1)
				{
					/* see if this name is arrayed, and pick a single entry from it if so */
					count = net_evalbusname(APBUS, networkname(net, 0), &strings, NOARCINST, NONODEPROTO, 0);
					if (count == bussize)
						return(strings[busindex]);
				}
				return(sim_spice_legalname(pt));
			}
		}
	}
	spnet = (SPNET *)net->temp1;
	(void)esnprintf(s, 80, x_("%ld"), spnet->netnumber);
	return(s);
}

CHAR *sim_spice_describenetwork(NETWORK *net)
{
	static CHAR gennetname[50];
	REGISTER NODEPROTO *np;

	/* write global net name if present (HSPICE and PSPICE only) */
	if (net->globalnet >= 0)
	{
		if (net->globalnet == GLOBALNETGROUND) return(x_("gnd"));
		if (net->globalnet == GLOBALNETPOWER) return(x_("vdd"));
		np = net->parent;
		if (net->globalnet >= np->globalnetcount)
			return(_("UNKNOWN"));
		return(np->globalnetnames[net->globalnet]);
	}
	if (net->namecount == 0)
	{
		esnprintf(gennetname, 50, _("UNNAMED%ld"), (INTBIG)net);
		return(gennetname);
	}
	return(sim_spice_legalname(networkname(net, 0)));
}

/*
 * Routine to mark nodeinst temps if nodeinst->proto contains LEGATEs,
 * or if any nodeinsts within nodeinst->proto contain LEGATEs, and so forth,
 * to be uniquified when spice netlisting.  This is because LEGATEs use the 
 * LE.getdrive() function to find their attribute values, and thus unique values
 * do not show up as parameters when the spice netlister decides if a cell needs
 * to be written again as a unique cell.
 * This function is bottom-up recursive because if a subcell must be uniquified
 * so must the current cell.
 * returns 1 if cell marked to uniquify.
 */
INTBIG sim_spice_markuniquenodeinsts(NODEPROTO *np)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *cnp;
	REGISTER INTBIG marked;
	static INTBIG attrLEGATEkey = 0;
	static INTBIG attrLEKEEPERkey = 0;

	if (attrLEGATEkey == 0) attrLEGATEkey = makekey(x_("ATTR_LEGATE"));
	if (attrLEKEEPERkey == 0) attrLEKEEPERkey = makekey(x_("ATTR_LEKEEPER"));

	/* temp1 == 1 means marked unique
	   temp1 == 2 means checked, not marked unique
	   temp1 == 0 means not check yet
	   */
	if (np->temp1 == 1) return(TRUE);
	if (np->temp2 == 2) return(FALSE);
	marked = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* ignore primitives and ignore self */
		if (ni->proto->primindex != 0) continue;	   
		if (isiconof(ni->proto, np)) continue;

		/* if ni is LEGATE or LEKEEPER, mark current cell and no recurse into */
		if (getvalkeynoeval((INTBIG)ni, VNODEINST, -1, attrLEGATEkey) != NOVARIABLE) { marked = 1; continue; }
		if (getvalkeynoeval((INTBIG)ni, VNODEINST, -1, attrLEKEEPERkey) != NOVARIABLE) { marked = 1; continue; }

		/* remove old var (marker) if present */
		if (getvalkeynoeval((INTBIG)ni, VNODEINST, -1, sim_spice_nameuniqueid) != NOVARIABLE)
			delvalkey((INTBIG)ni, VNODEINST, sim_spice_nameuniqueid);

		/* recurse, bottom-up into ni */
		cnp = contentsview(ni->proto);
		if (cnp == NONODEPROTO) cnp = ni->proto;
		if (sim_spice_markuniquenodeinsts(cnp) != 0)
		{
			/* mark nodeinst, mark current cell */
			setvalkey((INTBIG)ni, VNODEINST, sim_spice_nameuniqueid, 1, VINTEGER|VDONTSAVE);
			marked = 1;
		}
	}
	if (marked) np->temp1 = 1; else np->temp1 = 2;

	return(marked);
}

/******************** SUPPORT ********************/

/*
 * routine to return nonzero if layer "layer" is on diffusion
 * Return the type of the diffusion
 */
INTBIG sim_spice_layerisdiff(TECHNOLOGY *tech, INTBIG layer)
{
	REGISTER INTBIG i;

	i = layerfunction(tech, layer);
	if ((i&LFPSEUDO) != 0) return(ISNONE);
	if ((i&LFTYPE) != LFDIFF) return(ISNONE);
	if ((i&LFPTYPE) != 0) return(ISPTYPE);
	if ((i&LFNTYPE) != 0) return(ISNTYPE);
	return(ISNTYPE);		/* Default to N-type  */
}

/*
 * routine to return value if arc contains device active diffusion
 * Return the type of the diffusion, else ISNONE
 */
INTBIG sim_spice_arcisdiff(ARCINST *ai)
{
	REGISTER INTBIG i;

	i = (ai->proto->userbits&AFUNCTION)>>AFUNCTIONSH;
	switch (i)
	{
		case APDIFFP:
			return(ISPTYPE);
		case APDIFFN:
			return(ISNTYPE);
		case APDIFF:
			return(ISNTYPE);	/* Default device is n-type */
		default:
			return(ISNONE);	/* Default to Unknown  */
	}
}

/*
 * routine to search the net list for this cell and return the net number
 * associated with nodeinst "ni", network "net"
 */
SPNET *sim_spice_getnet(NODEINST *ni, NETWORK *net)
{
	REGISTER SPNET *spnet;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;

	/* search for arcs electrically connected to this port */
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		if (pi->proto->network == net)
	{
		for(spnet = sim_spice_firstnet; spnet != NOSPNET; spnet = spnet->nextnet)
			if (pi->conarcinst->network == spnet->network) return(spnet);
	}

	/* search for exports on the node, connected to this port */
	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		if (pe->proto->network == net)
	{
		for(spnet = sim_spice_firstnet; spnet != NOSPNET; spnet = spnet->nextnet)
			if (pe->exportproto->network == spnet->network) return(spnet);
	}
	return(NOSPNET);
}

/******************** LOW-LEVEL OUTPUT ROUTINES ********************/

/*
 * Routine to report an error that is built in the infinite string.
 * The error is sent to the messages window and also to the SPICE deck "f".
 */
void sim_spice_dumpstringerror(void *infstr, SpiceInst *inst)
{
	REGISTER CHAR *error;

	error = returninfstr(infstr);
	if (inst)
		inst->addError( error );
	else
		sim_spice_xprintf(sim_spice_file, TRUE, x_("*** %s\n"), error);
	ttyputmsg(x_("%s"), error);
}

/*
 * Formatted output to file "stream".  All spice output is in upper case.
 * The buffer can contain no more than 1024 chars including the newline
 * and null characters.
 * Doesn't return anything.
 */
void sim_spice_xprintf(FILE *stream, BOOLEAN iscomment, CHAR *format, ...)
{
	va_list ap;
	CHAR s[1024];

	var_start(ap, format);
	evsnprintf(s, 1024, format, ap);
	va_end(ap);
	sim_spice_xputs(s, stream, iscomment);
}

/*
 * Routine to write string "s" onto stream "stream", breaking
 * it into lines of the proper width, and converting to upper case
 * if SPICE2.
 */
void sim_spice_xputs(CHAR *s, FILE *stream, BOOLEAN iscomment)
{
	CHAR *pt, *lastspace, contchar;
	BOOLEAN insidequotes = FALSE;
	static INTBIG i=0;

	/* put in line continuations, if over 78 chars long */
	if (iscomment) contchar = '*'; else
		contchar = '+';
	lastspace = NULL;
	for (pt = s; *pt; pt++)
	{
		if (sim_spice_machine == SPICE2)
		{
			if (islower(*pt)) *pt = toupper(*pt);
		}
		if (*pt == '\n')
		{
			i = 0;
			lastspace = NULL;
		} else
		{
			/* removed '/' from check here, not sure why it's there */
			if (*pt == ' ' || *pt == '\'') lastspace = pt;
			if (*pt == '\'') insidequotes = !insidequotes;
			++i;
			if (i >= 78 && !insidequotes)
			{
				if (lastspace != NULL)
				{
					if( *lastspace == '\'')
					{
						*lastspace = '\0';
						xputs(s, stream);
						xprintf(stream, x_("'\n%c  "), contchar);
					} else
					{
						*lastspace = '\0';
						xputs(s, stream);
						xprintf(stream, x_("\n%c  "), contchar);
					}
					s = lastspace + 1;
					i = 9 + pt-s+1;
					lastspace = NULL;
				} else
				{
					xprintf(stream, x_("\n%c  "), contchar);
					i = 9 + 1;
				}
			}
		}
	}
	xputs(s, stream);
}

/******************** MEMORY ALLOCATION ********************/

POLYGON *sim_spice_allocpolygon(void)
{
	REGISTER POLYGON *poly;

	if (sim_polygonfree != NOPOLYGON)
	{
		poly = sim_polygonfree;
		sim_polygonfree = poly->nextpolygon;
	} else
	{
		poly = allocpolygon(4, sim_tool->cluster);
		if (poly == NOPOLYGON) return(NOPOLYGON);
	}
	poly->nextpolygon = NOPOLYGON;
	return(poly);
}

void sim_spice_freepolygon(POLYGON *poly)
{
	poly->nextpolygon = sim_polygonfree;
	sim_polygonfree = poly;
}

/*
 * routine to allocate and initialize a new net module from the pool
 * (if any) or memory
 */
SPNET *sim_spice_allocspnet(void)
{
	REGISTER SPNET *spnet;
	REGISTER INTBIG j;

	if (sim_spice_netfree == NOSPNET)
	{
		spnet = (SPNET *)emalloc(sizeof (SPNET), sim_tool->cluster);
		if (spnet == 0) return(NOSPNET);
	} else
	{
		/* take module from free list */
		spnet = sim_spice_netfree;
		sim_spice_netfree = spnet->nextnet;
	}

	/* Initialize it to empty values */
	spnet->resistance = 0.0;
	spnet->capacitance = 0.0;
	spnet->network = NONETWORK;
	for (j = 0; j < DIFFTYPES; j++)
	{
		spnet->diffperim[j] = 0.0;
		spnet->diffarea[j] = 0.0;
		spnet->components[j] = 0;
	}
	return(spnet);
}

/*
 * routine to return net module "spnet" to the pool of free modules
 */
void sim_spice_freespnet(SPNET *spnet)
{
	spnet->nextnet = sim_spice_netfree;
	sim_spice_netfree = spnet;
}

/*
 * Routine to scan networks in cell "cell".  All networks are sorted by name.
 * The number of networks is returned and the list of them is put in "netlist".
 * Each network's "temp2" field is 2 for output, 1 for all other export types
 * (this is reversed for CDL).  Also, the power and ground networks are put into
 * "vdd" and "gnd".
 */
INTBIG sim_spice_getexportednetworks(NODEPROTO *cell, NETWORK ***netlist, NETWORK **vdd, NETWORK **gnd,
	BOOLEAN cdl)
{
	REGISTER NETWORK *net;
	REGISTER PORTPROTO *pp;
	REGISTER INTBIG wirecount, i, termtype;
	REGISTER UINTBIG characteristics;

	/* initialize to describe all nets */
	for(net = cell->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		net->temp2 = 0;

	/* mark exported networks */
	*vdd = *gnd = NONETWORK;
	for(i = 0; i < cell->globalnetcount; i++)
	{
		net = cell->globalnetworks[i];
		if(net == NONETWORK) continue;
		characteristics = cell->globalnetchar[i];
		if (i == 0 || (characteristics == GNDPORT && *gnd == NONETWORK))
			*gnd = net;
		if (i == 1 || (characteristics == PWRPORT && *vdd == NONETWORK))
			*vdd = net;
	}
	for(pp = cell->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		net = pp->network;
		if (portispower(pp) && *vdd == NONETWORK) *vdd = net;
		if (portisground(pp) && *gnd == NONETWORK) *gnd = net;
		if (cdl)
		{
			if ((pp->userbits&STATEBITS) == OUTPORT) termtype = 1; else
				termtype = 2;
		} else
		{
			if ((pp->userbits&STATEBITS) == OUTPORT) termtype = 2; else
				termtype = 1;
		}
		if (net->buswidth > 1)
		{
			/* bus export: mark individual network entries */
			for(i=0; i<net->buswidth; i++)
				net->networklist[i]->temp2 = termtype;
		} else
		{
			/* single wire export: mark the network */
			net->temp2 = termtype;
		}
	}
	if (*vdd != NONETWORK && *vdd == *gnd)
		ttyputerr(_("*** WARNING: Power and Ground are shorted together in cell %s"),
			describenodeproto(cell));

	wirecount = 0;
	for(net = cell->firstnetwork; net != NONETWORK; net = net->nextnetwork)
	{
		if (net->temp2 != 0) wirecount++;
	}

	if (wirecount > 0)
	{
		/* make sure there is room in the array of networks */
		if (wirecount > sim_spicewirelisttotal)
		{
			if (sim_spicewirelisttotal > 0) efree((CHAR *)sim_spicewirelist);
			sim_spicewirelisttotal = 0;
			sim_spicewirelist = (NETWORK **)emalloc(wirecount * (sizeof (NETWORK *)),
				sim_tool->cluster);
			if (sim_spicewirelist == 0) return(0);
			sim_spicewirelisttotal = wirecount;
		}

		/* load the array */
		i = 0;
		for(net = cell->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		{
			if (net->temp2 != 0) sim_spicewirelist[i++] = net;
		}

		/* sort the networks by name */
		esort(sim_spicewirelist, wirecount, sizeof (NETWORK *), sim_spice_sortnetworksbyname);
		*netlist = sim_spicewirelist;
	}

	return(wirecount);
}

/*
 * Routine to recursively examine cells and gather global network names.
 */
void sim_spice_gatherglobals(NODEPROTO *np)
{
	for(INTBIG k = 0; k < np->globalnetcount; k++)
	{
		if (np->globalnetworks[k] == NONETWORK) continue;
		CHAR *gname = sim_spice_describenetwork(np->globalnetworks[k]);
		/* add the global net name */
		if (sim_spiceglobalnetcount >= sim_spiceglobalnettotal)
		{
			INTBIG newtotal = sim_spiceglobalnettotal * 2;
			INTBIG i;

			if (sim_spiceglobalnetcount >= newtotal)
				newtotal = sim_spiceglobalnetcount + 5;
			CHAR **newlist = (CHAR **)emalloc(newtotal * (sizeof (CHAR *)), sim_tool->cluster);
			if (newlist == 0) return;
			for(i=0; i<sim_spiceglobalnettotal; i++)
				newlist[i] = sim_spiceglobalnets[i];
			for(i=sim_spiceglobalnettotal; i<newtotal; i++)
				newlist[i] = 0;
			if (sim_spiceglobalnettotal > 0)
				efree((CHAR *)sim_spiceglobalnets);
			sim_spiceglobalnets = newlist;
			sim_spiceglobalnettotal = newtotal;
		}
		if (sim_spiceglobalnets[sim_spiceglobalnetcount] != 0)
			efree((CHAR *)sim_spiceglobalnets[sim_spiceglobalnetcount]);
		(void)allocstring(&sim_spiceglobalnets[sim_spiceglobalnetcount], gname,
			sim_tool->cluster);
		sim_spiceglobalnetcount++;
	}
}

/*
 * Routine to determine the proper name to use for cell "np".
 */
CHAR *sim_spice_cellname(NODEPROTO *np)
{
	REGISTER void *infstr;

	if (np->temp2 == 0) return(np->protoname);
	infstr = initinfstr();
	formatinfstr(infstr, "%s-%s", np->lib->libname, np->protoname);
	return(returninfstr(infstr));
}

/*
 * Helper routine for "esort" that makes networks with names go in ascending name order.
 */
int sim_spice_sortnetworksbyname(const void *e1, const void *e2)
{
	REGISTER NETWORK *net1, *net2;
	REGISTER CHAR *pt1, *pt2;
	CHAR empty[1];

	net1 = *((NETWORK **)e1);
	net2 = *((NETWORK **)e2);
	if (net1->temp2 != net2->temp2)
		return(net1->temp2 - net2->temp2);
	empty[0] = 0;
	if (net1->namecount == 0) pt1 = empty; else pt1 = networkname(net1, 0);
	if (net2->namecount == 0) pt2 = empty; else pt2 = networkname(net2, 0);
	return(namesamenumeric(pt1, pt2));
}

/******************** Spice Classes ********************/

SpiceCell *SpiceCell::_firstCell = 0;
CHAR *SpiceCell::_path = 0;
CHAR *SpiceCell::_pathend;
CHAR *SpiceCell::_rpath = 0;
CHAR *SpiceCell::_rpathbeg;
CHAR **sim_spice_printlist = 0;
INTBIG sim_spice_printlistlen = 0;


SpiceCell::SpiceCell( CHAR *name )
	: _firstInst( 0 ), _lastInst( 0 ), _firstNet( 0 )
{
	_name = new CHAR[estrlen(name) + 1];
	estrcpy( _name, name );
	_nextCell = _firstCell;
	_firstCell = this;
}

SpiceCell::~SpiceCell()
{
	delete[] _name;
	while (_firstInst)
	{
		SpiceInst *inst = _firstInst;
		_firstInst = _firstInst->_nextInst;
		delete inst;
	}
	while (_firstNet)
	{
		SpiceNet *net = _firstNet;
		_firstNet = _firstNet->_nextNet;
		delete net;
	}
}

SpiceCell *SpiceCell::findByName( CHAR *name )
{
	SpiceCell *cell;
	for (cell = _firstCell; cell && estrcmp(name, cell->_name) != 0; cell = cell->_nextCell);
	return cell;
}

void SpiceCell::setup()
{
	_pathlen = 15;
	_totalnets = 0;
	for (SpiceInst *inst = _firstInst; inst; inst = inst->_nextInst)
	{
		if (!inst->_instCell) continue;
		if ((INTBIG)estrlen(inst->_name) + inst->_instCell->_pathlen >= _pathlen)
			_pathlen = estrlen(inst->_name) + 1 + inst->_instCell->_pathlen;
		_totalnets += inst->_instCell->_totalnets;
	}
	for (SpiceNet *net = _firstNet; net; net = net->_nextNet)
	{
		if (net->_netw && net->_netw->namecount > 0)
		{
			INTBIG len = estrlen(networkname(net->_netw, 0));
			if (len > _pathlen)
				_pathlen = len;
		}
		_totalnets++;
	}
}

void SpiceCell::printSpice()
{
	for (SpiceInst *inst = _firstInst; inst; inst = inst->_nextInst)
		inst->printSpice();
}

void SpiceCell::traverse()
{
	for (SpiceNet *net = _firstNet; net; net = net->_nextNet)
	{
		if (net->_netnumber == 0) continue;
		xprintf(sim_spice_file, x_(" V(%ld%s)"), net->_netnumber, _rpathbeg);
		if (net->_netw && net->_netw->namecount > 0)
			estrcpy(_pathend, networkname(net->_netw, 0));
		else
			esnprintf(_pathend, 10, x_("%ld"), net->_netnumber);
		sim_spice_printlist[sim_spice_printlistlen] = new CHAR[estrlen(_path) + 1];
		estrcpy(sim_spice_printlist[sim_spice_printlistlen], _path);
		sim_spice_printlistlen++;
	}
	for (SpiceInst *inst = _firstInst; inst; inst = inst->_nextInst)
	{
		if (!inst->_instCell) continue;
		inst->traverse();
	}
}

void SpiceCell::traverseAll()
{
	_pathend = _path = new CHAR[_firstCell->_pathlen + 1];
	_rpath = new CHAR[_firstCell->_pathlen + 1];
	_rpathbeg = _rpath + _firstCell->_pathlen;
	_path[0] = _rpathbeg[0] = 0;

	if (sim_spice_printlist)
	{
		for (int i = 0; i < sim_spice_printlistlen; i++) delete[] sim_spice_printlist[i];
		delete[] sim_spice_printlist;
	}
	sim_spice_printlistlen = 0;
	sim_spice_printlist = new (CHAR*[_firstCell->_totalnets]);
	xprintf(sim_spice_file, x_(".print tran") );
	_firstCell->traverse();
	if (sim_spice_printlistlen > _firstCell->_totalnets)
		ttyputmsg(x_("Spice cell: sim_spice_printlistlen=%ld totalnets=%ld"),
			sim_spice_printlistlen, _firstCell->_totalnets);
	xprintf(sim_spice_file, x_("\n"));
	delete _path;
	delete _rpath;
}

void SpiceCell::clearAll()
{
	while (_firstCell)
	{
		SpiceCell *cell = _firstCell;
		_firstCell = _firstCell->_nextCell;
		delete cell;
	}
	if (sim_spice_printlist)
	{
		for (int i = 0; i < sim_spice_printlistlen; i++) delete[] sim_spice_printlist[i];
		delete[] sim_spice_printlist;
	}
	sim_spice_printlist = 0;
	sim_spice_printlistlen = 0;
}

SpiceInst::SpiceInst( SpiceCell *parentCell, CHAR *name )
	: _parentCell( parentCell ), _instCell( 0 ), _firstBind( 0 ), _lastBind( 0 ), _firstParam( 0 ), _lastParam( 0 ),
	  _error( 0 ), _nextInst( 0 )
{
	_name = new CHAR[estrlen(name) + 1];
	estrcpy( _name, name );
	if ( _parentCell->_firstInst )
		_parentCell->_lastInst->_nextInst = this;
	else
		_parentCell->_firstInst = this;
	_parentCell->_lastInst = this;
}

SpiceInst::~SpiceInst()
{
	while (_firstBind)
	{
		SpiceBind *bind = _firstBind;
		_firstBind = bind->_nextBind;
		delete bind;
	}
	while (_firstParam)
	{
		SpiceParam *param = _firstParam;
		_firstParam = param->_nextParam;
		delete param;
	}
	if (_error) delete[] _error;
	delete[] _name;
}

void SpiceInst::setInstCell( SpiceCell *instCell )
{
	_instCell = instCell;
}

void SpiceInst::addBind( SpiceBind *bind )
{
	if (_lastBind)
		_lastBind->_nextBind = bind;
	else
		_firstBind = bind;
	_lastBind = bind;
	bind->_nextBind = 0;
	bind->_inst = this;
}

void SpiceInst::addParam( SpiceParam *param )
{
	if (_lastParam)
		_lastParam->_nextParam = param;
	else
		_firstParam = param;
	_lastParam = param;
	param->_nextParam = 0;
}

void SpiceInst::addParamM( NODEINST *ni )
{
	CHAR line[100];

	VARIABLE *varm = getvalkey((INTBIG)ni, VNODEINST, -1, el_attrkey_M);
	if (varm == NOVARIABLE) return;
	if (ni->proto->primindex == 0 && TDGETISPARAM(varm->textdescript) != 0) return;
	CHAR *pt = describevariable(varm, -1, -1);
	esnprintf(line, 100, x_("M=%s"), pt);
	addParam( new SpiceParam( line ) );
}

void SpiceInst::addError( CHAR *error )
{
	void *infstr = initinfstr();
	if (_error) addstringtoinfstr( infstr, _error );
	formatinfstr(infstr, x_("*** %s\n"), error);
	CHAR *newStr = returninfstr(infstr);
	if (_error) delete[] _error;
	_error = new CHAR[estrlen(newStr) + 1];
	estrcpy( _error, newStr );
}

void SpiceInst::printSpice()
{
	sim_spice_puts( _name, FALSE );
	for (SpiceBind *bind = _firstBind; bind; bind = bind->_nextBind)
		bind->printSpice();
	if (sim_spice_cdl)
		sim_spice_puts( x_(" /"), FALSE );
	if (_instCell)
		sim_spice_xprintf( sim_spice_file, FALSE, x_(" %s"), _instCell->_name );
	for (SpiceParam *param = _firstParam; param; param = param->_nextParam)
		sim_spice_xprintf(sim_spice_file, FALSE, x_(" %s"), param->str());
	sim_spice_puts( x_("\n"), FALSE );
	if (_error)
		sim_spice_puts( _error, TRUE );
}

void SpiceInst::traverse()
{
	estrcpy(SpiceCell::_pathend, _name);
	estrcat(SpiceCell::_pathend, x_("/"));
	SpiceCell::_pathend += estrlen(_name) + 1;
	SpiceCell::_rpathbeg -= estrlen(_name) + 1;
	estrncpy(SpiceCell::_rpathbeg + 1, _name, estrlen(_name));
	*SpiceCell::_rpathbeg = '.';
	_instCell->traverse();
	SpiceCell::_pathend -= estrlen(_name) + 1;
	SpiceCell::_rpathbeg += estrlen(_name) + 1;
}

SpiceBind::SpiceBind( SpiceInst *inst, SpiceNet *net )
{
	if (!net)
	{
		INTBIG unnamed = ((sim_spice_state&SPICENODENAMES) != 0 ? sim_spice_unnamednum++ : sim_spice_netindex++);
		net = new SpiceNet( inst->_parentCell, unnamed );
	}
	_net = net;
	inst->addBind( this );
}

SpiceBind::~SpiceBind()
{
}

void SpiceBind::printSpice()
{
	_net->printSpice();
}

SpiceParam::SpiceParam( CHAR *str )
	: _nextParam( 0 )
{
	_str = new CHAR[estrlen(str) + 1];
	estrcpy( _str, str );
}

SpiceParam::~SpiceParam()
{
	delete[] _str;
}

SpiceNet::SpiceNet( SpiceCell *parentCell, INTBIG netnumber, NETWORK *netw )
	: _netnumber( netnumber ), _netw( netw ), _parentCell( parentCell )
{
	_nextNet = _parentCell->_firstNet;
	_parentCell->_firstNet = this;
}

SpiceNet::~SpiceNet()
{
}

void SpiceNet::printSpice()
{
	CHAR line[100];

	if ((sim_spice_state&SPICENODENAMES) != 0)
	{
		if (_netw)
		{
			void *infstr = initinfstr();
			formatinfstr(infstr, x_(" %s"), sim_spice_netname(_netw, 0, 0));
			sim_spice_puts(returninfstr(infstr), FALSE);
			return;
		}
		esnprintf(line, 100, x_(" UNNAMED%ld"), _netnumber);
	} else
	{
		esnprintf(line, 100, x_(" %ld"), _netnumber);
	}
	sim_spice_puts(line, FALSE);
}

#endif  /* SIMTOOL - at top */
