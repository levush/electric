/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: simsilos.c
 * Generator for SILOS simulator, Version 2
 * Written by: Sid Penstone, Queen's University
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
#if SIMTOOL

#include "global.h"
#include "efunction.h"
#include "sim.h"
#include "usr.h"
#include "tecschem.h"

#define MAXSTR  79		/* split lines into 80 characters */
#define MAXNAME 12		/* Maximum macro name length */

/* Types returned by sim_silostype() */

#define SILOSCELL		-1		/* Complex node */
#define SILOSUNKNOWN	 0		/* All connected together */
#define SILOSNMOS		 1		/* Transistor */
#define SILOSPMOS		 2		/* P-transistor */
#define SILOSAND		 3		/* AND gate */
#define SILOSOR			 4		/* OR gate */
#define SILOSXOR		 5		/* XOR */
#define SILOSINV		 6		/* Inverter */
#define SILOSBUF		 7		/* Buffer */
#define SILOSSOURCE		 8		/* Source, not .CLK */
#define SILOSRES		 9		/* Resistor in SILOS */
#define SILOSCAP		10		/* Capacitor */
#define SILOSDFF		11		/* D flip flop */
#define SILOSJKFF		12		/* JK flip flop */
#define SILOSSRFF		13		/* RS flip flop */
#define SILOSTFF		14		/* Toggle flip flop */
#define SILOSCLK		15		/* .CLK (may not be used yet */
#define SILOSOUT		16		/* Output point */
#define SILOSCLOCK		17		/* Clocks, etc. declared as globals */

/********************** global variables *********************/

static FILE  *sim_silfile;
static INTBIG sim_pseudonet;		/* for internal networks */
static INTBIG sim_silos_global_name_key = 0;
static INTBIG sim_silos_model_key = 0;		/* for model on cell */

/* ************************************************************* */

/* prototypes for local routines */
static INTBIG  sim_writesilcell(NODEPROTO*, BOOLEAN);
static void    sim_writesilinstances(NODEPROTO*, BOOLEAN);
static CHAR   *sim_silportname(BOOLEAN, NODEINST*, PORTPROTO*, NODEPROTO*);
static CHAR   *sim_silname(NODEINST*);
static CHAR   *sim_silproto(NODEINST*, BOOLEAN);
static CHAR   *sim_siltext(CHAR*);
static void    sim_sendtosil(CHAR*);
static CHAR   *sim_silarcname(ARCINST*);
static BOOLEAN sim_silnegated(PORTARCINST*);
static CHAR   *sim_silspecials(CHAR*);
static INTBIG  sim_silostype(NODEINST*);
static CHAR   *sim_silrisetime(NODEINST*);
static CHAR   *sim_silfalltime(NODEINST*);
static INTBIG  sim_silgetcapacitance(NODEINST*);
static void    sim_silwriteflipflop(BOOLEAN, NODEINST*, NODEPROTO*, INTBIG);
static CHAR   *sim_sil_isaport(NODEPROTO*, CHAR*);
static CHAR   *sim_convertsub(CHAR*);

/*
 * routine to write a ".sil" file from the cell "np"
 */
void sim_writesilnetlist(NODEPROTO *np)
{
	CHAR name[100], numberstring[100], *truename;
	REGISTER NODEPROTO *lnp;
	REGISTER LIBRARY *lib;

	/* make sure network tool is on */
	if ((net_tool->toolstate&TOOLON) == 0)
	{
		ttyputerr(_("Network tool must be running...turning it on"));
		toolturnon(net_tool);
		ttyputerr(_("...now reissue the simulation command"));
		return;
	}

	/* first write the "sil" file */
	(void)estrcpy(name, np->protoname);
	(void)estrcat(name, x_(".sil"));
	sim_silfile = xcreate(name, sim_filetypesilos, _("SILOS File"), &truename);
	if (sim_silfile == NULL)
	{
		if (truename != 0) ttyputerr(_("Cannot write %s"), truename);
		return;
	}
	sim_sendtosil(x_("\n"));
	sim_sendtosil(x_("$ CELL "));
	sim_sendtosil(describenodeproto(np));
	sim_sendtosil(x_(" FROM LIBRARY:"));
	sim_sendtosil(np->lib->libname);
	sim_sendtosil(x_("\n"));
	if ((us_useroptions&NODATEORVERSION) == 0)
	{
		if (np->creationdate)
		{
			(void)esnprintf(numberstring, 100, x_("$ CELL CREATED ON %s"),
				timetostring((time_t)np->creationdate));
			sim_sendtosil(numberstring);
			sim_sendtosil(x_("\n"));
		}
		if (np->version)
		{
			(void)esnprintf(numberstring, 100, x_("$ VERSION %ld"), np->version);
			sim_sendtosil(numberstring);
			sim_sendtosil(x_("\n"));
		}
		if (np->revisiondate)
		{
			(void)esnprintf(numberstring, 100, x_("$ LAST REVISED %s"),
				timetostring((time_t)np->revisiondate));
			sim_sendtosil(numberstring);
			sim_sendtosil(x_("\n"));
		}
		(void)esnprintf(numberstring, 100,
			x_("$ SILOS netlist written by Electric Design System; Version %s"), el_version);
		sim_sendtosil(numberstring);
		sim_sendtosil(x_("\n"));

		(void)esnprintf(numberstring, 100, x_("%s"), timetostring(getcurrenttime()));
		sim_sendtosil(x_("$ WRITTEN ON "));
		sim_sendtosil(numberstring);
		sim_sendtosil(x_("\n"));
	} else
	{
		sim_sendtosil(x_("$ SILOS netlist written by Electric Design System\n"));
	}

	if (sim_silos_global_name_key == 0)
		sim_silos_global_name_key = makekey(x_("SIM_silos_global_name"));
	if (sim_silos_model_key == 0)
		sim_silos_model_key = makekey(x_("SC_silos"));

	/* reset flags for cells that have been written */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(lnp = lib->firstnodeproto; lnp != NONODEPROTO; lnp = lnp->nextnodeproto)
			lnp->temp1 = 0;
	if (sim_writesilcell(np, TRUE) != 0)
		ttyputmsg(_("Back-annotation information has been added (library must be saved)"));
	sim_sendtosil(x_("\n"));

	/* clean up */
	xclose(sim_silfile);
	ttyputmsg(_("%s written"), truename);

}

/*
 * recursively called routine to print the SILOS description of cell "np".
 * The description is treated as the top-level cell if "top" is true
 * np is the current nodeproto
 */
INTBIG sim_writesilcell(NODEPROTO *np, BOOLEAN top)
{
	UCHAR1 buf[256];
	REGISTER INTBIG i, j, nodetype, clktype, count, backannotate;
	REGISTER NODEINST *ni;
	REGISTER PORTPROTO *pp;
	REGISTER NODEPROTO *onp;
	REGISTER NETWORK *net;
	REGISTER VARIABLE *var;
	REGISTER FILE *f;
	CHAR line[100], *filename, *extra;
	CHAR **model, *name;

	/* stop if requested */
	if (el_pleasestop != 0)
	{
		(void)stopping(STOPREASONDECK);
		return(0);
	}
	backannotate = 0;

	/* First look for any global sources */
	if (top)
	{
		for (ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if (ni->proto->primindex == 0) continue;	/* only real sources */

			nodetype = nodefunction(ni);
			if (nodetype != NPSOURCE) continue;

			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, sim_silos_global_name_key);
			if (var != NOVARIABLE)	/* is this a global source ? */
			{
				name = (CHAR *)var->addr;
				(void)esnprintf(line, 100, x_(".GLOBAL %s"), sim_silspecials(name));
				sim_sendtosil(line);
				sim_sendtosil(x_("\n"));
			}

			/* Get the source type */
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, sch_spicemodelkey);
			if (var == NOVARIABLE)
			{
				ttyputerr(_("Unspecified source:"));
				esnprintf(line, 100, x_("$$$$$ Unspecified source: \n"));
			} else	/* There is more */
			{
				clktype = 0;	/* Extra data required if variable there */
				for(extra = (CHAR *)var->addr; *extra != 0 && *extra != '/'; extra++)
				{
					switch (*extra)
					{
						case 'g':	/* a global clock (THIS IS WRONG!!!) */
							(void)esnprintf(line, 100, x_("%s .CLK "), name);
							clktype = 1;
							break;
						case 'h':	/* a fixed high source (THIS IS WRONG!!!) */
							(void)esnprintf(line, 100, x_("%s .CLK 0 S1 $ HIGH LEVEL"), name);
							break;
						case 'l':	/* a fixed low source (THIS IS WRONG!!!) */
							(void)esnprintf(line, 100, x_("%s .CLK 0 S0 $ LOW LEVEL"), name);
							break;
					}
				}
				if (*extra == '/') extra++;
				if (clktype == 1) estrcat(line, extra);
			}
			sim_sendtosil(line);
			sim_sendtosil(x_("\n"));
		}
	}

	/* Read a behavior file if it is available */
	FOR_CELLGROUP(onp, np)
	{
		var = getval((INTBIG)onp, VNODEPROTO, VSTRING, x_("SIM_silos_behavior_file"));
		if (var != NOVARIABLE)
		{
			f = xopen(truepath((CHAR *)var->addr), sim_filetypesilos, x_(""), &filename);
			if (f == NULL)
				ttyputerr(_("Cannot find SILOS behavior file %s on cell %s"),
					(CHAR *)var->addr, describenodeproto(onp)); else
			{
				/* copy the file */
				for(;;)
				{
					count = xfread(buf, 1, 256, f);
					if (count <= 0) break;
					if (xfwrite(buf, 1, count, sim_silfile) != count)
						ttyputerr(_("Error copying file"));
				}
				xclose(f);
				sim_sendtosil(x_("\n"));

				/* mark this cell as written */
				np->temp1++;
				return(backannotate);
			}
		}
	}

	/*
	 * There was no behavior file...
	 * Get the SILOS model from the library if it exists
	 */
	FOR_CELLGROUP(onp, np)
	{
		var = getvalkey((INTBIG)onp, VNODEPROTO, VSTRING|VISARRAY, sim_silos_model_key);
		if (var != NOVARIABLE)
		{
			model = (CHAR **)var->addr;
			j = getlength(var);
			for(i = 0; i < j; i++)
				xprintf(sim_silfile, x_("%s\n"), model[i]);
			sim_sendtosil(x_("\n"));

			/* mark this cell as written */
			np->temp1++;
			return(backannotate);
		}
	}

	/*
	 * No database model either...
	 * must write netlist for this cell
	 * ...recurse on sub-cells first
	 */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex != 0) continue;

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(ni->proto, np)) continue;

		/* get actual subcell (including contents/body distinction) */
		/* NOTE: this gets the schematic before the layout */
		onp = contentsview(ni->proto);
		if (onp == NONODEPROTO) onp = ni->proto;

		/* write the subcell */
		if (onp->temp1 == 0)
		{
			if (sim_writesilcell(onp, FALSE) != 0) backannotate = 1;
		}
	}

	/* make sure that all nodes have names on them */
	if (asktool(net_tool, x_("name-nodes"), (INTBIG)np) != 0) backannotate++;

	/* mark this cell as written */
	np->temp1++;

	/* write the header if this not the top level */
	if (!top)
	{
		sim_sendtosil(x_("\n"));
		sim_sendtosil(x_(".MACRO"));
		(void)estrcpy(line, np->protoname);
		for (i=0; line[i] && line[i] != '{'; i++)
			;
		line[i] = 0;			/* explicit termination of line */
		if(estrlen(line) > MAXNAME)
		{
			ttyputerr(_(".MACRO name %s is too long;"), line);
			line[MAXNAME] = 0;
			ttyputerr(_("truncated to %s"), line);
		}
		sim_sendtosil(x_(" "));
		sim_sendtosil(sim_siltext(line));

		/*
		 * This is different from sim_silportname() because it finds ports on
		 * the np, not the ni - No shortcuts here!
		 */
		if (np->firstportproto != NOPORTPROTO)
		{
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			{
				if (portispower(pp) || portisground(pp)) continue;
				(void)estrcpy(line, sim_convertsub(pp->protoname));
				sim_sendtosil(x_(" "));
				sim_sendtosil(sim_siltext(line));
			}
		}
		sim_sendtosil(x_("\n"));
	}

	/* initialize ports that get used */
	for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		net->temp1 = 0;
	if (np->firstnodeinst != NONODEINST)	/* any valid nodes ? */
		sim_writesilinstances(np, top);
	if (top == 0) sim_sendtosil(x_(".EOM\n"));
	sim_sendtosil(x_("\n"));
	return(backannotate);
}						/* end sim_writesilcell() */


/* Function to write the instances contained in 'np'
 * This was part of sim_writesilcell before...
 */
void sim_writesilinstances(NODEPROTO *np, BOOLEAN top)
{
	REGISTER INTBIG j, schem_ref;
	REGISTER INTBIG nodetype;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *cnp;
	REGISTER PORTPROTO *pp, *lastpp, *outpp, *ipp;
	REGISTER PORTARCINST *pi;
	CHAR line[100];

	sim_pseudonet = 1;
	schem_ref = 1;

	/* look at every node in this cell */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		ni->temp1 = 0;

		/* not interested in passive nodes (ports electrically shorted) */
		j = 0;
		lastpp = ni->proto->firstportproto;
		if (lastpp == NOPORTPROTO) continue;
		for(pp = lastpp->nextportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			if (pp->network != lastpp->network) j++;
		if (j == 0) continue;

		/* all remaining nodes have at least two distinct ports */
		ni->temp1 = schem_ref++;

		/* reset flag for output port */
		outpp = NOPORTPROTO;

		nodetype = sim_silostype(ni);
		switch (nodetype)
		{
			case SILOSCELL:		/* Complex cell uses instance name */
				sim_sendtosil(x_("("));
				sim_sendtosil(sim_silname(ni));
				sim_sendtosil(x_(" "));
				sim_sendtosil(sim_silproto(ni, FALSE));	/* write the type */

				/* write the rest of the port(s), connected or not */
				cnp = contentsview(ni->proto);
				if (cnp == NONODEPROTO) cnp = ni->proto;
				for(pp = cnp->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				{
					ipp = equivalentport(cnp, pp, ni->proto);
					if (ipp == NOPORTPROTO) continue;
					sim_sendtosil(sim_silportname(top, ni, ipp, np));
				}
				break;

			case SILOSNMOS:
			case SILOSPMOS:
			case SILOSTFF:
			case SILOSDFF:
			case SILOSJKFF:
			case SILOSSRFF:
				/* Transistors and flip-flops need a part number */
				sim_sendtosil(sim_silname(ni));
				sim_sendtosil(x_(" "));
				sim_sendtosil(sim_silproto(ni, FALSE));

				/* write the names of the port(s) */
				if (nodetype == SILOSTFF || nodetype == SILOSDFF ||
					nodetype == SILOSJKFF || nodetype == SILOSSRFF)
				{
					(void)sim_silwriteflipflop(top, ni, np, nodetype);
				} else
				{
					for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
						sim_sendtosil(sim_silportname(top, ni, pp, np));
				}
				break;

			case SILOSCLOCK:
			case SILOSSOURCE:
				if (!top) ttyputerr(_("WARNING: Global Clock in a sub-cell"));
				break;

			case SILOSAND:	/* only need positive logic here */
			case SILOSOR:
			case SILOSXOR:
			case SILOSBUF:
				/* Gates use their output port as a name */
				for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				{
					/* write the output signal as the name */
					if ((pp->userbits&STATEBITS) == OUTPORT)
					{
						/* Find the name of the output port */
						sim_sendtosil(sim_silportname(top, ni, pp, np));

						/* Record that we used it */
						outpp = pp;
						sim_sendtosil(x_(" "));

						/*
						 * determine if this proto is negated...
						 * find a pi that is connected to this pp
						 */
						for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
							if (pi->proto == pp) break;

						/* sim_silnegated handles NOPORTARCINST */
						sim_sendtosil(sim_silproto(ni, sim_silnegated(pi)));
					}
					if (outpp != NOPORTPROTO)	/* found the output name */
						break;
				}
				if (pp == NOPORTPROTO)
					ttyputerr(_("Could not find an output connection on %s"), ni->proto->protoname);

				/* get the fall and rise times */
				sim_sendtosil(sim_silrisetime(ni));
				sim_sendtosil(sim_silfalltime(ni));

				/* write the rest of the port(s) iff they're connected */
				for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				{
					if (pp == outpp) continue;	/* already used this port */

					/* search for a connection */
					for (pi = ni->firstportarcinst; pi != NOPORTARCINST && pi->proto != pp;
						pi = pi->nextportarcinst)
							;

					if (pi != NOPORTARCINST)	/* ... port is connected */
						sim_sendtosil(sim_silportname(top, ni, pp, np));
				}
				break;

			case SILOSCAP:
				/* find a connected port for the node name */
				for (pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				{
					/* search for a connection */
					for (pi = ni->firstportarcinst; pi != NOPORTARCINST && pi->proto != pp;
						pi = pi->nextportarcinst)
							;

					if (pi != NOPORTARCINST)	/* ...port is connected */
						break;			/* out of "for (pp" */
				}

				if (pp != NOPORTPROTO)	/* ...there is a connection */
				{
					/* write port name as output */
					sim_sendtosil(sim_silportname(top, ni, pp, np));

					sim_sendtosil(x_(" "));
					sim_sendtosil(sim_silproto(ni, FALSE));

					j = sim_silgetcapacitance(ni);
					if (j >= 0)
					{
						(void)esnprintf(line, 100, x_(" %ld"), j);
						sim_sendtosil(line);
					} else
						ttyputerr(_("Warning: capacitor with no value"));
				}
				break;

			case SILOSRES:	/* sorry! can't handle the resistive gate yet */
			default:	/* some compilers may not like this empty default */
				break;
		}
		sim_sendtosil(x_("\n"));
	}
}

/*
 * Find a name to write for the port prototype, pp, on the node instance, ni
 * The node instance is located within the prototype, np
 * If there is an arc connected to the port, use the net name, or NETn.
 * If there are more than one arc (that are not electrically connected)
 * on the port, concatenate the names (with spaces between them).
 * If there is no arc, but the port is an export, use the exported name.
 * If the port is a power or ground port, ignore it
 * If this is not the top level cell (ie. a .macro) remove [] notation.
 */
CHAR *sim_silportname(BOOLEAN top, NODEINST *ni, PORTPROTO *pp, NODEPROTO *np)
{
	REGISTER PORTPROTO *epp;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER ARCINST *ai = NOARCINST;
	CHAR *portname;
	BOOLEAN noarcs;
	REGISTER void *infstr;

	if (portispower(pp) || portisground(pp)) return(x_(""));

	infstr = initinfstr();

	noarcs = TRUE;

	/* find all portarcinsts that are connected to this pp */
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		if (pi->proto == pp)	/* if this pi is connected */
		{
			/* found an arc */
			noarcs = FALSE;

			/* if an input connection has a bubble, negate the signal */
			if (sim_silnegated(pi) && ((pp->userbits&STATEBITS) == INPORT))
				addstringtoinfstr(infstr, x_(" -")); else
					addtoinfstr(infstr, ' ');

			/* find the arc connected to this pi */
			ai = pi->conarcinst;
			if (ai != NOARCINST)
			{
				/*
				 * get the name...
				 * this is either the network name, or of the form NETn
				 */
				if (top)
					addstringtoinfstr(infstr, sim_silspecials(sim_silarcname(ai))); else
						addstringtoinfstr(infstr, sim_sil_isaport(np, sim_silspecials(sim_silarcname(ai))));
			} else /* this is a database error, and won't ever occur */
			{
				ttyputerr(_("Error: there is a PORTARCINST without a CONARCINST"));
				return(x_(""));
			}

			/* if PORTISOLATED, then there may be more unconnected arcs */
			/* otherwise, break out of for loop */
			if (pp->userbits&PORTISOLATED) continue;
			break;		/* out of "for (pi..." */
		}
	}

	/* if this is the top cell and we didn't find an arc */
	if (top && noarcs)
	{
		/* search for an export */
		for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		{
			if (pe->proto == pp)	/* this port is an export */
			{
				addtoinfstr(infstr, ' ');

				/* write the export name */
				epp = pe->exportproto;
				if (epp->network->namecount > 0)
				{
					if (top) addstringtoinfstr(infstr, sim_silspecials(networkname(epp->network, 0))); else
						addstringtoinfstr(infstr, sim_sil_isaport(np, sim_silspecials(networkname(epp->network, 0))));
				} else	/* this would be a database error - won't happen */
				{
					ttyputerr(_("Error: there is a net on an export, but no net name"));
					return(x_(""));
				}
				break;	/* out of "for (pe..." */
			}
		}
	}

	/* get the name that we've just built */
	portname = returninfstr(infstr);

	/* nothing connected to this port...leave a position */
	if (portname[0] == '\0') return(x_(" .SKIP"));
	return(portname);
}

/*
 * routine to return a string describing the SILOS part name of nodeinst
 * "ni"
 */
CHAR *sim_silname(NODEINST *ni)
{
	static CHAR name[100];
	REGISTER VARIABLE *var;
	static INTBIG SIM_silos_node_name_key = 0;

	if (SIM_silos_node_name_key == 0)
		SIM_silos_node_name_key = makekey(x_("SIM_silos_node_name"));
	var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, SIM_silos_node_name_key);
	if (var != NOVARIABLE) return((CHAR *)var->addr);
	var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
	if (var != NOVARIABLE)
	{
		estrcpy(name, (CHAR *)var->addr);
		if (isalpha(name[0])) return(name);
		if (name[0] == '[') return(sim_convertsub(name));
	}
	(void)esnprintf(name, 100, x_("U%ld"), ni->temp1);
	return(name);
}

/*
 * routine to return a string describing the SILOS type of nodeinst "ni"
 * if 'neg' is true, then the negated version is needed
 */
CHAR *sim_silproto(NODEINST *ni, BOOLEAN neg)
{
	REGISTER INTBIG f;
	static CHAR name[100];

	/* cells use their real name */
	if (ni->proto->primindex == 0)
	{
		esnprintf(name, 100, x_("%s"), sim_siltext(ni->proto->protoname));
		if (estrlen(name) > MAXNAME) name[MAXNAME] = 0;
		return(name);
	}

	f = sim_silostype(ni);
	switch (f)
	{
		case SILOSNMOS:
			return(x_(".NMOS"));
		case SILOSPMOS:
			return(x_(".PMOS"));
		case SILOSBUF:
		case SILOSINV:
			if (neg) return(x_(".INV"));
			return(x_(".BUF"));
		case SILOSXOR:
			if (neg) return(x_(".XNOR"));
			return(x_(".XOR"));
		case SILOSAND:
			if (neg) return(x_(".NAND"));
			return(x_(".AND"));
		case SILOSOR:
			if (neg) return(x_(".NOR"));
			return(x_(".OR"));
		case SILOSRES:
			return(x_(".RES"));
		case SILOSCAP:
			return(x_(".CAP"));
		case SILOSJKFF:
			if ((ni->userbits&FFCLOCK) == FFCLOCKN) return(x_(".JKNEFF"));
			return(x_(".JKPEFF"));
		case SILOSDFF:
			if ((ni->userbits&FFCLOCK) == FFCLOCKN) return(x_(".DNEFF"));
			return(x_(".DPEFF"));
		case SILOSSRFF:
			if ((ni->userbits&FFCLOCK) == FFCLOCKN) return(x_(".SRNEFF"));
			return(x_(".SRPEFF"));
		case SILOSTFF:
			if ((ni->userbits&FFCLOCK) == FFCLOCKN) return(x_(".TNEFF"));
			return(x_(".TPEFF"));
	}
	return(sim_siltext(ni->proto->protoname));
}

/*
 * routine to replace all non-printing characters
 * in the string "p" with the letter "X" and return the string
 * We will not permit a digit in the first location; replace it
 * with '_'
 */
CHAR *sim_siltext(CHAR *p)
{
	REGISTER CHAR *t;
	REGISTER void *infstr;

	for(t = p; *t != 0; t++) if (!isprint(*t)) break;
	if (*t == 0 && (!isdigit(*p))) return(p);

	infstr = initinfstr();
	if (isdigit(*p)) addtoinfstr(infstr, '_');
	for(t = p; *t != 0; t++)
		if (isprint(*t)) addtoinfstr(infstr, *t); else
			addtoinfstr(infstr, '_');
	return(returninfstr(infstr));
}

/* Write to the .sil file, but break into printable lines */

void sim_sendtosil(CHAR *str)
{
	static INTBIG count = 0;
	INTBIG i;

	if (*str == '\n') count = 0;
	if ((count + (i = estrlen(str))) > MAXSTR)
	{
		if (*str == '$')
		{
			xprintf(sim_silfile, x_("\n"));
			count = 0;
		} else
		{
			xprintf(sim_silfile, x_("\n+"));
			count = 1;
		}
	}
	xprintf(sim_silfile, x_("%s"), str);
	count += i;
	return;
}

/*
 * routine to return the name of arc "ai" (either its network name or
 * a generated name
 */
CHAR *sim_silarcname(ARCINST *ai)
{
	static CHAR line[50];

	if (ai->network->namecount > 0) return(networkname(ai->network, 0));
	if (ai->network->temp1 == 0) ai->network->temp1 = sim_pseudonet++;
	(void)esnprintf(line, 50, x_("NET%ld"), ai->network->temp1);
	return(line);
}

/* Function to determine if this end of the connecting arc is negated */
BOOLEAN sim_silnegated(PORTARCINST *pi)
{
	REGISTER ARCINST *ai;
	INTBIG thisend;

	if (pi == NOPORTARCINST) return(FALSE);

	ai = pi->conarcinst;
	if (ai == NOARCINST) return(FALSE);
	if ((ai->userbits&ISNEGATED) == 0) return(FALSE);

	if (ai->end[0].portarcinst == pi) thisend = 0; else
		thisend = 1;
	if ((thisend == 0 && (ai->userbits&REVERSEEND) == 0) ||
		(thisend == 1 && (ai->userbits&REVERSEEND) != 0)) return(TRUE);
	return(FALSE);
}

/*
 * function to convert special names to SILOS format
 */

CHAR *sim_silspecials(CHAR *str)
{
	if (namesamen(str, x_("vdd"), 3) == 0) return(x_(".VDD"));
	if (namesamen(str, x_("vss"), 3) == 0) return(x_(".VSS"));
	if (namesamen(str, x_("vcc"), 3) == 0) return(x_(".VCC"));
	if (namesamen(str, x_("gnd"), 3) == 0) return(x_(".GND"));
	if (namesamen(str, x_("low"), 3) == 0) return(x_(".GND"));
	if (namesamen(str, x_("hig"), 3) == 0) return(x_(".VDD"));
	return(str);
}

/*
 * Function to return the SILOS type of a node
 * Read the contents of the added string, make it available to
 * the caller
 */
INTBIG sim_silostype(NODEINST *ni)
{
	INTBIG func;

	if (ni->proto->primindex == 0) return(SILOSCELL);

	func = nodefunction(ni);
	switch(func)
	{
		case NPTRAPMOS:
		case NPTRA4PMOS:
			return(SILOSPMOS);
		case NPTRANMOS:
		case NPTRA4NMOS:
			return(SILOSNMOS);
		case NPGATEAND:
			return(SILOSAND);
		case NPGATEOR:
			return(SILOSOR);
		case NPGATEXOR:
			return(SILOSXOR);
		case NPBUFFER:
			return(SILOSBUF);
		case NPRESIST:
			return(SILOSRES);
		case NPCAPAC:
			return(SILOSCAP);
		case NPFLIPFLOP:	/* We will decode further here */
			switch (ni->userbits&FFTYPE)
			{
				case FFTYPERS: return(SILOSSRFF);
				case FFTYPEJK: return(SILOSJKFF);
				case FFTYPED:  return(SILOSDFF);
				case FFTYPET:  return(SILOSTFF);
			}
			return(SILOSDFF);
		case NPSOURCE:
			return(SILOSCLOCK);
		case NPMETER:
			return(SILOSOUT);
		default:
			return(SILOSUNKNOWN);
		}
}

/*
 * This function returns a string containing the rise time, as stored in
 * the variable SIM_rise_delay on node instance ni.
 * SIM_rise_delay can be multiple numbers (e.g. "rise_time,fanout")
 * This function returns a string.
 * A space is inserted as the first character in the string.
 * Returns an empty string if no variable found.
 */
CHAR *sim_silrisetime(NODEINST *ni)
{
	REGISTER VARIABLE *var;
	static INTBIG SIM_sil_risetime_key = 0;
	static CHAR s[80];

	if (SIM_sil_risetime_key == 0)
		SIM_sil_risetime_key = makekey(x_("SIM_rise_delay"));
	var = getvalkey((INTBIG)ni, VNODEINST, -1, SIM_sil_risetime_key);
	if (var != NOVARIABLE)
	{
		if ((var->type&VTYPE) == VINTEGER)
		{
			(void)esnprintf(s, 80, x_(" %ld"), var->addr);
			return(s);
		}
		if ((var->type&VTYPE) == VSTRING)
		{
			(void)esnprintf(s, 80, x_(" %s"), (CHAR *)var->addr);
			return(s);
		}
	}
	return(x_(""));
}

/*
 * This function returns a string containing the fall time, as stored in
 * the variable SIM_fall_delay on node instance ni.
 * SIM_fall_delay can be either an integer or a string
 * (e.g. "fall_time,fanout")
 * This function returns a string.
 * A space is inserted as the first character in the string.
 * Returns an empty string if no variable found.
 */
CHAR *sim_silfalltime(NODEINST *ni)
{
	REGISTER VARIABLE *var;
	static INTBIG SIM_sil_falltime_key = 0;
	static CHAR s[80];

	if (SIM_sil_falltime_key == 0)
		SIM_sil_falltime_key = makekey(x_("SIM_fall_delay"));
	var = getvalkey((INTBIG)ni, VNODEINST, -1, SIM_sil_falltime_key);
	if (var != NOVARIABLE)
	{
		if ((var->type&VTYPE) == VINTEGER)
		{
			(void)esnprintf(s, 80, x_(" %ld"), var->addr);
			return(s);
		}
		if ((var->type&VTYPE) == VSTRING)
		{
			(void)esnprintf(s, 80, x_(" %s"), (CHAR *)var->addr);
			return(s);
		}
	}
	return(x_(""));
}

/*
 * Function to return an integer as the capacitance defined
 * by "SCHEM_capacitance" variable on an instance of a capacitor.
 * The returned units are in microfarads (is this right?).
 * Return -1 if nothing found.
 */
INTBIG sim_silgetcapacitance(NODEINST *ni)
{
	REGISTER VARIABLE *var;
	float farads;
	INTBIG microfarads;

	var = getvalkey((INTBIG)ni, VNODEINST, -1, sch_capacitancekey);
	if (var != NOVARIABLE)
	{
		farads = (float)eatof(describesimplevariable(var));
		microfarads = eatoi(displayedunits(farads, VTUNITSCAP, INTCAPUNITUFARAD));
		return(microfarads);
	}
	return(-1);
}

/*
 * Function to write the ports of a flip-flop;
 * get them in the Electric order, then rewrite them
 * 'ni' is the current NODEINST, found in 'np' cell
 */
#define JORD 0
#define K	 1
#define Q	 2
#define QB   3
#define CK	 4
#define PRE  5
#define CLR  6
void sim_silwriteflipflop(BOOLEAN top, NODEINST *ni, NODEPROTO *np, INTBIG type)
{
	REGISTER PORTPROTO *pp;
	CHAR line[8][50], out[100];
	INTBIG i;

	for(pp = ni->proto->firstportproto, i=0; pp != NOPORTPROTO && i < 7; pp = pp->nextportproto, i++)
	{
		/* find arcs on this port */
		(void)esnprintf(line[i], 50, x_(" %s"), sim_silportname(top, ni, pp, np));
	}
	if (namesamen(line[PRE], x_(" .SKIP"), 4) == 0 && namesamen(line[CLR], x_(" .SKIP"), 4) == 0)
	{
		*line[CLR] = 0;		/* If neither on, don't print */
		*line[PRE] = 0;
	}
	if (type == SILOSDFF) (void)esnprintf(out, 100, x_("%s%s%s%s /%s%s"),
		line[CK], line[JORD], line[PRE], line[CLR], line[Q], line[QB]); else
			(void)esnprintf(out, 100, x_("%s%s%s%s%s /%s%s"), line[CK], line[JORD],
				line[K], line[PRE], line[CLR], line[Q], line[QB]);
	sim_sendtosil(out);
}

/* Function to check if a port of an instance is connected to one of
the ports of the containing instance. If so, get rid of the '[]' format;
replace '[' with '__', ignore ']'.
*/
CHAR *sim_sil_isaport(NODEPROTO *np, CHAR *portname)
{
	REGISTER PORTPROTO *pp;

	if (estrchr(portname, '[') == NULL) return(portname);		/* no references anyhow */
	pp = getportproto(np, portname);
	if (pp != NOPORTPROTO) return(sim_convertsub(portname));
	return(portname);
}

/* replace subscripted name with __ format */
static CHAR *sim_convertsub(CHAR *string)
{
	CHAR *ptr;
	static CHAR newname[100];

	for (ptr = newname; *string; string++)
	{
		if (*string == '[')
		{
			estrcpy(ptr, x_("__"));
			ptr+=2;
		} else if (*string == ']')
		{
			continue;
		} else
		{
			*ptr = *string;
			ptr++;
		}
	}
	*ptr = 0;
	return(newname);
}

#endif	/* SIMTOOL - at top */
