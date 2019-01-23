/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: simsim.cpp
 * Simulation tool: ESIM, RSIM, RNL, and COSMOS controller
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
#include "efunction.h"
#include "usr.h"
#include "tecschem.h"
#include <setjmp.h>

static INTBIG sim_cosmos_attribute_key = 0;
static jmp_buf sim_userdone;		/* for when simulator command is done */
static INTBIG sim_index, sim_vdd, sim_gnd, sim_namethresh,
		 sim_phi1h, sim_phi1l, sim_phi2h, sim_phi2l, count_unconn;
static CHAR *sim_name, *sim_prompt, *sim_inputfile = 0;

/* prototypes for local routines */
static BOOLEAN sim_simsearch(INTBIG, NODEPROTO*, INTBIG);
static void    sim_simtell(void);
static void    sim_simprint(NODEPROTO*, FILE*, INTBIG);
static void    sim_simprintcosmos(NODEPROTO*, FILE*, INTBIG);
static CHAR   *sim_makenodename(INTBIG, INTBIG);
static void    sim_prop(ARCINST*, INTBIG, INTBIG*, INTBIG*, INTBIG*, INTBIG*, INTBIG*, INTBIG*);

void sim_writesim(NODEPROTO *simnt, INTBIG format)
{
	REGISTER INTBIG filetype;
	REGISTER INTBIG i;
	REGISTER VARIABLE *var;
	REGISTER FILE *f;
	REGISTER PORTPROTO *pp, *spt, *shpt;
	CHAR *truename, *presimloc, *newname;
	REGISTER CHAR *simprog, *presimtmp, *presimprog, *netfile;
	REGISTER void *infstr;

	/* make sure network tool is on */
	if ((net_tool->toolstate&TOOLON) == 0)
	{
		ttyputerr(_("Network tool must be running...turning it on"));
		toolturnon(net_tool);
		ttyputerr(_("...now reissue the simulation command"));
		return;
	}

	if (format == COSMOS)
	{
		/* get variable names (only once) */
		if (sim_cosmos_attribute_key == 0)
			sim_cosmos_attribute_key = makekey(x_("SIM_cosmos_attribute"));
	}

	count_unconn = 0;
	sim_simnt = simnt;

	/* initialize for the particular simulator */
	var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_formatkey);
	if (var == NOVARIABLE) format = -1; else format = var->addr;

	/* construct the simulation description file name from the cell */
	infstr = initinfstr();
	addstringtoinfstr(infstr, simnt->protoname);
	addstringtoinfstr(infstr, x_(".sim"));
	allocstring(&newname, returninfstr(infstr), el_tempcluster);
	var = setvalkey((INTBIG)sim_tool, VTOOL, sim_netfilekey, (INTBIG)newname, VSTRING|VDONTSAVE);
	efree(newname);
	if (var != NOVARIABLE) netfile = (CHAR *)var->addr; else netfile = x_("");

	/* also record the actual input file to the simulator */
	if (sim_inputfile != 0) efree(sim_inputfile);
	(void)allocstring(&sim_inputfile, netfile, sim_tool->cluster);

	/* create the simulation file */
	if (format == RSIM) filetype = sim_filetypersim; else
		filetype = sim_filetypeesim;
	f = xcreate(netfile, filetype, _("Netlist File"), &truename);
	if (f == NULL)
	{
		if (truename != 0) ttyputerr(_("Cannot write %s"), truename);
		return;
	}
	xprintf(f,x_("| %s\n"), netfile);
	us_emitcopyright(f, x_("| "), x_(""));
	if ((us_useroptions&NODATEORVERSION) == 0)
	{
		if (simnt->creationdate != 0)
			xprintf(f, x_("| Cell created %s\n"), timetostring((time_t)simnt->creationdate));
		xprintf(f, x_("| Version %ld"), simnt->version);
		if (simnt->revisiondate != 0)
			xprintf(f, x_(" last revised %s\n"), timetostring((time_t)simnt->revisiondate));
	}
	if (format == COSMOS)
	{
		xprintf(f,x_("| [e | d | p | n] gate source drain length width xpos ypos {[gsd]=attrs}\n"));
		xprintf(f,x_("| N node D-area D-perim P-area P-perim M-area M-perim\n"));
		xprintf(f,x_("| A node attrs\n"));
		xprintf(f,x_("|  attrs = [Sim:[In | Out | 1 | 2 | 3 | Z | U]]\n"));
	} else
	{
		xprintf(f,x_("| [epd] gate source drain length width r xpos ypos area\n"));
		xprintf(f,x_("| N node xpos ypos M-area P-area D-area D-perim\n"));
	}

	/* assign net-list values to each port of the top cell */
	sim_index = 1;
	sim_vdd = sim_gnd = 0;
	sim_phi1h = sim_phi1l = sim_phi2h = sim_phi2l = 0;
	for(pp = simnt->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		/* see if this port is on the same net number as some other port */
		for(spt = simnt->firstportproto; spt != pp; spt = spt->nextportproto)
			if (spt->network == pp->network) break;

		/* this port is the same as another one, use same net */
		if (spt != pp)
		{
			pp->temp1 = spt->temp1;
			continue;
		}

		/* see if the port is special */
		if (portispower(pp) != 0)
		{
			if (sim_vdd == 0) sim_vdd = sim_index++;
			pp->temp1 = sim_vdd;   continue;
		}
		if (portisground(pp) != 0)
		{
			if (sim_gnd == 0) sim_gnd = sim_index++;
			pp->temp1 = sim_gnd;   continue;
		}
		if ((pp->userbits&STATEBITS) == C1PORT ||
			namesamen(pp->protoname, x_("clk1"), 4) == 0 || namesamen(pp->protoname, x_("phi1h"), 5) == 0)
		{
			if (sim_phi1h == 0) sim_phi1h = sim_index++;
			pp->temp1 = sim_phi1h;   continue;
		}
		if ((pp->userbits&STATEBITS) == C2PORT || namesamen(pp->protoname, x_("phi1l"), 5) == 0)
		{
			if (sim_phi1l == 0) sim_phi1l = sim_index++;
			pp->temp1 = sim_phi1l;   continue;
		}
		if ((pp->userbits&STATEBITS) == C3PORT ||
			namesamen(pp->protoname, x_("clk2"), 4) == 0 || namesamen(pp->protoname, x_("phi2h"), 5) == 0)
		{
			if (sim_phi2h == 0) sim_phi2h = sim_index++;
			pp->temp1 = sim_phi2h;   continue;
		}
		if ((pp->userbits&STATEBITS) == C4PORT || namesamen(pp->protoname, x_("phi2l"), 5) == 0)
		{
			if (sim_phi2l == 0) sim_phi2l = sim_index++;
			pp->temp1 = sim_phi2l;   continue;
		}

		/* assign a new nodeinst number to this portinst */
		pp->temp1 = sim_index++;
	}
	sim_namethresh = sim_index;

	if (sim_vdd == 0) ttyputmsg(_("Warning: no power export in this cell"));
	if (sim_gnd == 0) ttyputmsg(_("Warning: no ground export in this cell"));

	/* select the shortest name for electrically equivalent ports */
	for(pp = simnt->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		pp->temp2 = 0;
	for(pp = simnt->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		if (pp->temp2 != 0) continue;
		for(spt = simnt->firstportproto; spt != NOPORTPROTO; spt = spt->nextportproto)
			if (spt->temp1 == pp->temp1) break;
		if (spt == NOPORTPROTO)
		{
			pp->temp2 = 1;
			continue;
		}

		/* multiple electrically equivalent ports: find shortest of all */
		i = estrlen(pp->protoname);
		shpt = pp;
		for(spt = simnt->firstportproto; spt != NOPORTPROTO; spt = spt->nextportproto)
		{
			if (spt->temp1 != pp->temp1) continue;
			spt->temp2 = -1;
			if ((INTBIG)estrlen(spt->protoname) >= i) continue;
			shpt = spt;
			i = estrlen(spt->protoname);
		}
		shpt->temp2 = 1;
	}

	if (format == COSMOS) sim_simprintcosmos(simnt, f, format);
		else sim_simprint(simnt, f, format);

	/* describe the nonspecial ports */
	i = 0;
	for(pp = simnt->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		if (pp->temp1 == sim_vdd || pp->temp1 == sim_gnd || pp->temp1 == sim_phi1h ||
			pp->temp1 == sim_phi1l || pp->temp1 == sim_phi2h || pp->temp1 == sim_phi2l) continue;

		/* see if this port name is nonunique and unwanted */
		if (pp->temp2 < 0) continue;

		/* just a normal node: write its characteristics */
		if (i++ == 0) ttyputmsg(_("These export names are available:"));
		ttyputmsg(x_("   %s"), pp->protoname);
		if (format == COSMOS)
		{
			if ((pp->userbits & STATEBITS) == INPORT)
				xprintf(f, x_("A %s Sim:In\n"), pp->protoname);
					else if ((pp->userbits & STATEBITS) == OUTPORT ||
						(pp->userbits & STATEBITS) == BIDIRPORT)
							xprintf(f, x_("A %s Sim:Out\n"), pp->protoname);
		}
	}

	/* clean up */
	xclose(f);

	/* if execution is not desired, quit now */
	var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_dontrunkey);
	if (format == COSMOS || (var != NOVARIABLE && var->addr == SIMRUNNO))
	{
		if (count_unconn != 0)
			ttyputmsg(_("Warning: %ld unconnected nodes were found"), count_unconn);
		ttyputmsg(_("%s written"), truename);
		return;
	}

	/* with RNL, there is a pre-processing step */
	if (format == RNL || format == RSIM)
	{
		/* start up the presimulator */
		if (format == RSIM)
		{
			presimtmp = RSIMIN;
			presimprog = RSIMPRENAME;
		} else
		{
			presimtmp = RNLIN;
			presimprog = RNLPRENAME;
		}

		EProcess rnl_process;
		presimloc = egetenv(x_("ELECTRIC_PRESIMLOC"));
		if (presimloc == NULL) presimloc = PRESIMLOC;
		rnl_process.addArgument( presimloc );
		rnl_process.addArgument( netfile );
		rnl_process.addArgument( presimtmp );
		rnl_process.setCommunication( FALSE, TRUE, TRUE );
		rnl_process.start();
		rnl_process.wait();
	}

	/* start up the simulation process */
	simprog = 0;
	switch (format)
	{
		case ESIM:
			sim_name = ESIMNAME;
			simprog = egetenv(x_("ELECTRIC_ESIMLOC"));
			if (simprog == NULL) simprog = ESIMLOC;
			sim_prompt = x_("sim> ");
			break;
		case RSIM:
			sim_name = RSIMNAME;
			simprog = egetenv(x_("ELECTRIC_RSIMLOC"));
			if (simprog == NULL) simprog = RSIMLOC;
			sim_prompt = x_("rsim> ");
			break;
		case RNL:
			sim_name = RNLNAME;
			simprog = egetenv(x_("ELECTRIC_RNLLOC"));
			if (simprog == NULL) simprog = RNLLOC;
			sim_prompt = x_("");
			break;
	}

	sim_process = new EProcess();
	sim_process->addArgument( simprog );
	if (format != RNL) sim_process->addArgument( sim_inputfile );
	sim_process->setCommunication( TRUE, TRUE, TRUE );
	sim_process->start();

	/* now communicate with the simulator */
	sim_resumesim(TRUE);
}

void sim_simpointout(CHAR *nonum, INTBIG format)
{
	REGISTER INTBIG want;
	REGISTER PORTPROTO *pp, *spt;

	if (sim_simnt == NONODEPROTO)
	{
		ttyputerr(_("No simulation done"));
		return;
	}

	/* make sure user is still in the same cell */
	if (sim_simnt != getcurcell())
	{
		ttyputerr(_("Simulation was done on cell %s; please edit that cell"),
			describenodeproto(sim_simnt));
		return;
	}

	/* figure out the desired nodeinst number */
	if (*nonum >= '0' && *nonum <= '9') want = eatoi(nonum); else
	{
		pp = getportproto(sim_simnt, nonum);
		if (pp == NOPORTPROTO)
		{
			ttyputerr(_("Cannot find a port called '%s'"), nonum);
			return;
		}
		want = pp->temp1;
	}

	/* re-assign net-list values, starting at each portinst of the top cell */
	sim_index = 1;
	sim_vdd = sim_gnd = 0;
	sim_phi1h = sim_phi1l = sim_phi2h = sim_phi2l = 0;
	for(pp = sim_simnt->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		for(spt = sim_simnt->firstportproto; spt != pp; spt = spt->nextportproto)
		{
			if (spt->network != pp->network) continue;

			/* this port is the same as another one, use same net */
			pp->temp1 = spt->temp1;
			break;
		}
		if (spt != pp) continue;
		if (format == COSMOS)
		{
			/* see if the port is special */
			if (portispower(pp))
			{
				if (sim_vdd == 0) sim_vdd = sim_index++;
				pp->temp1 = sim_vdd;   continue;
			}
			if (portisground(pp))
			{
				if (sim_gnd == 0) sim_gnd = sim_index++;
				pp->temp1 = sim_gnd;   continue;
			}
			if ((pp->userbits&STATEBITS) == C1PORT ||
				namesamen(pp->protoname, x_("clk1"), 4) == 0 || namesamen(pp->protoname, x_("phi1h"), 5) == 0)
			{
				if (sim_phi1h == 0) sim_phi1h = sim_index++;
				pp->temp1 = sim_phi1h;   continue;
			}
			if ((pp->userbits&STATEBITS) == C2PORT || namesamen(pp->protoname, x_("phi1l"), 5) == 0)
			{
				if (sim_phi1l == 0) sim_phi1l = sim_index++;
				pp->temp1 = sim_phi1l;   continue;
			}
			if ((pp->userbits&STATEBITS) == C3PORT ||
				namesamen(pp->protoname, x_("clk2"), 4) == 0 || namesamen(pp->protoname, x_("phi2h"), 5) == 0)
			{
				if (sim_phi2h == 0) sim_phi2h = sim_index++;
				pp->temp1 = sim_phi2h;   continue;
			}
			if ((pp->userbits&STATEBITS) == C4PORT || namesamen(pp->protoname, x_("phi2l"), 5) == 0)
			{
				if (sim_phi2l == 0) sim_phi2l = sim_index++;
				pp->temp1 = sim_phi2l;   continue;
			}
			/* assign a new nodeinst number to this portinst */
			pp->temp1 = sim_index++;
		} else pp->temp1 = sim_index++;
	}

	/* look for node number */
	if (sim_simsearch(want, sim_simnt, format))
	{
		ttyputerr(_("Cannot find node '%s'"), nonum);
		return;
	}
}

BOOLEAN sim_simsearch(INTBIG want, NODEPROTO *cell, INTBIG format)
{
	CHAR *newpar[2];
	INTBIG darea, dperim, parea, pperim, marea, mperim;
	REGISTER BOOLEAN i;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp, *spt;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np, *snp;
	REGISTER INTBIG type;

	/* reset the arcinst node values */
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		ai->temp1 = 0;

	/* set every arcinst to a global node number (from inside or outside) */
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->temp1 != 0) continue;

		/* see if this arcinst is connected to a port */
		for(pp = cell->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			if (pp->network != ai->network) continue;
			if (pp->temp1 == want)
			{
				(void)asktool(us_tool, x_("clear"));
				(void)asktool(us_tool, x_("show-object"), (INTBIG)ai->geom);
				newpar[0] = x_("highlight");
				telltool(net_tool, 1, newpar);
				return(FALSE);
			}
			darea = dperim = parea = pperim = marea = mperim = 0;
			sim_prop(ai, (INTBIG)pp->temp1, &darea, &dperim, &parea, &pperim, &marea, &mperim);
			break;
		}

		/* if not on a port, this is an internal node */
		if (pp == NOPORTPROTO)
		{
			if (sim_index == want)
			{
				(void)asktool(us_tool, x_("clear"));
				(void)asktool(us_tool, x_("show-object"), (INTBIG)ai->geom);
				newpar[0] = x_("highlight");
				telltool(net_tool, 1, newpar);
				return(FALSE);
			}
			darea = dperim = parea = pperim = marea = mperim = 0;
			sim_prop(ai, sim_index, &darea, &dperim, &parea, &pperim, &marea, &mperim);
			sim_index++;
		}
	}

	/* see if the port has the right node but is unconnected */
	for(pp = cell->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		if (pp->temp1 == want)
	{
		(void)asktool(us_tool, x_("clear"));
		(void)asktool(us_tool, x_("show-port"), (INTBIG)pp->subnodeinst->geom, (INTBIG)pp->subportproto);
		return(FALSE);
	}

	/* look at every nodeinst in the cell */
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		np = ni->proto;
		if (format == COSMOS)
		{
			type = (np->userbits&NFUNCTION) >> NFUNCTIONSH;
			snp = np;
			if (np->primindex == 0)
			{
				type = NPUNKNOWN;

				/* ignore recursive references (showing icon in contents) */
				if (isiconof(np, cell)) continue;
				np = contentsview(snp);
				if (np == NONODEPROTO) np = snp;
			}

			/* ignore artwork */
			if (type == NPART) continue;
		} else snp = NONODEPROTO;

		/* initialize network numbers on every portinst on this nodeinst */
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto) pp->temp1 = 0;

		/* set network numbers from arcs and exports */
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			pp = pi->proto;
			if (format == COSMOS && snp != np)
			{
				pp = equivalentport(snp, pp, np);
				if (pp == NOPORTPROTO) continue;
			}
			if (pp->temp1 != 0) continue;
			pp->temp1 = pi->conarcinst->temp1;
			if (pp->temp1 == 0) continue;
			for(spt = np->firstportproto; spt != NOPORTPROTO; spt = spt->nextportproto)
				if (spt->network == pp->network) spt->temp1 = pp->temp1;
		}
		for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		{
			pp = pe->proto;
			if (format == COSMOS && snp != np)
			{
				pp = equivalentport(snp, pp, np);
				if (pp == NOPORTPROTO) continue;
			}
			if (pp->temp1 != 0) continue;
			pp->temp1 = pe->exportproto->temp1;
			if (pp->temp1 == 0) continue;
			for(spt = np->firstportproto; spt != NOPORTPROTO; spt = spt->nextportproto)
				if (spt->network == pp->network) spt->temp1 = pp->temp1;
		}

		/* look for unconnected ports and assign new network numbers */
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			if (pp->temp1 != 0) continue;

			/* Hack!!! ignore unconnected well and select ports, flag the others */
			if (format == COSMOS)
			{
				if (namesame(pp->protoname, x_("well")) != 0 && namesame(pp->protoname, x_("select")) != 0)
					pp->temp1 = sim_index++;
			} else pp->temp1 = sim_index++;
		}

		/* if it is a cell, recurse */
		if (np->primindex == 0)
		{
			i = sim_simsearch(want, ni->proto, format);
			if (!i) return(FALSE);
		}
	}
	return(TRUE);
}

void sim_resumesim(BOOLEAN firsttime)
{
	REGISTER INTBIG i, j, format;
	CHAR line[200], *ptr;
	REGISTER VARIABLE *var;

	if (sim_process == 0)
	{
		ttyputerr(_("Cannot communicate with the simulator"));
		return;
	}
	ttyputmsg(_("YOU ARE TALKING TO THE SIMULATOR '%s'"), sim_name);

	if (firsttime)
	{
		ttyputmsg(_("Type 'help' if you need it"));
		var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_formatkey);
		if (var == NOVARIABLE) format = -1; else format = var->addr;
		if (format == RNL)
		{
			/* get the simulator to load the default commands */
			(void)esnprintf(line, 200, x_("(load \"%s\") (read-network \"%s%s\")"), el_libdir, RNLCOMM,
				sim_inputfile);
			ttyputmsg(line);
			for (j = 0; line[j]; j++) sim_process->putChar( line[j] );
			sim_process->putChar( '\n' );
		}
	}
	ptr = line;
	for(;;)
	{
		i = setjmp(sim_userdone);
		switch (i)
		{
			case 0: break;		/* first return from "setjmp" */
			case 1: continue;		/* normal return from sim_simtell */
			case 2:			/* stop the simulation */
				sim_process->kill();
				delete sim_process;
				sim_process = 0;
				var = getvalkey((INTBIG)sim_tool, VTOOL, VSTRING, sim_netfilekey);
				if (var != NOVARIABLE)
					ttyputmsg(_("Net list saved in '%s'"), (CHAR *)var->addr);
				return;
			case 3:			/* pause the simulation */
				if (firsttime)
					ttyputmsg(_("Type '-telltool simulation resume' to return"));
				return;
		}

		/* read from the simulator and list on the status terminal */
		for(;;)
		{
			INTSML ch = sim_process->getChar();
			*ptr = (CHAR)ch;
			if (ch == EOF) break;

			if (*ptr == '\n')
			{
				*ptr = 0;
				ttyputmsg(x_("%s"), line);
				ptr = line;
				continue;
			}
			*++ptr = 0;
			if (*sim_prompt != 0 && estrcmp(line, sim_prompt) == 0)
			{
				ptr = line;
				sim_simtell();
			}
		}
#ifndef USEQT
		if (ttydataready()) sim_simtell();
#endif

		ttyputmsg(_("Abnormal simulator termination"));
		longjmp(sim_userdone, 2);
	}
}

void sim_simtell(void)
{
	CHAR *newpar[2];
	static CHAR quit[2];
	REGISTER CHAR *pp;
	REGISTER INTBIG len, i;
	REGISTER VARIABLE *var;

	for(;;)
	{
		pp = ttygetline(sim_prompt);
		if (pp == 0)
		{
			estrcpy(quit, x_("q"));
			pp = quit;
			break;
		}

		/* handle nonsimulator commands */
		if (*pp == 'p')
		{
			sim_process->putChar( '\n' );
			longjmp(sim_userdone, 3);
		}
		len = estrlen(pp);
		if (len > 1 && namesamen(pp, x_("help"), len) == 0)
		{
			newpar[0] = x_("help");   newpar[1] = sim_name;
			telltool(us_tool, 2, newpar);
		} else break;
	}

	/* send user command to simulator */
	if (*pp == 'q')
	{
		var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_formatkey);
		if (var != NOVARIABLE && var->addr == RNL) *pp = CTRLDKEY;
	}
	len = estrlen(pp);
	for (i = 0; pp[i]; i++) sim_process->putChar( pp[i] );
	sim_process->putChar( '\n' );
	if (*pp == 'q' || *pp == CTRLDKEY) longjmp(sim_userdone, 2);
	longjmp(sim_userdone, 1);
}

void sim_simprint(NODEPROTO *cell, FILE *f, INTBIG format)
{
	REGISTER INTBIG gate, source, drain, type;
	INTBIG darea, dperim, parea, pperim, marea, mperim, length, width;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *p, *pp, *spt;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np, *snp;

	/* stop if requested */
	if (stopping(STOPREASONDECK)) return;

	/* reset the arcinst node values */
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		ai->temp1 = 0;

	/* set every arcinst to a global node number (from inside or outside) */
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->temp1 != 0) continue;

		/* see if this arcinst is connected to a port */
		for(pp = cell->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			if (pp->network != ai->network) continue;
			darea = dperim = parea = pperim = marea = mperim = 0;
			sim_prop(ai, (INTBIG)pp->temp1, &darea, &dperim, &parea, &pperim, &marea, &mperim);
			if (pp->temp1 != sim_vdd && pp->temp1 != sim_gnd &&
				(marea != 0 || parea != 0 || darea != 0))
					xprintf(f, x_("N %s 0 0 %g %g %g %g\n"), sim_makenodename(pp->temp1, format),
						scaletodispunitsq(marea, DISPUNITMIC), scaletodispunitsq(parea, DISPUNITMIC),
							scaletodispunitsq(darea, DISPUNITMIC), scaletodispunit(dperim, DISPUNITMIC));
			break;
		}

		/* if not on a portinst, this is an internal node */
		if (pp == NOPORTPROTO)
		{
			darea = dperim = parea = pperim = marea = mperim = 0;
			sim_prop(ai, sim_index, &darea, &dperim, &parea, &pperim, &marea, &mperim);
			if (marea != 0 || parea != 0 || darea != 0)
				xprintf(f, x_("N %s 0 0 %g %g %g %g\n"), sim_makenodename(sim_index, format),
					scaletodispunitsq(marea, DISPUNITMIC), scaletodispunitsq(parea, DISPUNITMIC),
						scaletodispunitsq(darea, DISPUNITMIC), scaletodispunit(dperim, DISPUNITMIC));
			sim_index++;
		}
	}

	/* look at every nodeinst in the cell */
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* initialize network numbers on every portinst on this nodeinst */
		np = ni->proto;
		type = (np->userbits&NFUNCTION) >> NFUNCTIONSH;
		snp = np;
		if (np->primindex == 0)
		{
			type = NPUNKNOWN;

			/* ignore recursive references (showing icon in contents) */
			if (isiconof(np, cell)) continue;
			np = contentsview(snp);
			if (np == NONODEPROTO) np = snp;
		}

		/* ignore artwork */
		if (type == NPART) continue;

		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			pp->temp1 = 0;

		/* set network numbers from arcs and exports */
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			pp = pi->proto;
			if (snp != np)
			{
				pp = equivalentport(snp, pp, np);
				if (pp == NOPORTPROTO) continue;
			}
			if (pp->temp1 != 0) continue;
			pp->temp1 = pi->conarcinst->temp1;
			if (pp->temp1 == 0) continue;
			for(spt = np->firstportproto; spt != NOPORTPROTO; spt = spt->nextportproto)
				if (spt->network == pp->network) spt->temp1 = pp->temp1;
		}
		for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		{
			pp = pe->proto;
			if (snp != np)
			{
				pp = equivalentport(snp, pp, np);
				if (pp == NOPORTPROTO) continue;
			}
			if (pp->temp1 != 0) continue;
			pp->temp1 = pe->exportproto->temp1;
			if (pp->temp1 == 0) continue;
			for(spt = np->firstportproto; spt != NOPORTPROTO; spt = spt->nextportproto)
				if (spt->network == pp->network) spt->temp1 = pp->temp1;
		}

		/* look for unconnected ports and assign new network numbers */
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			if (pp->temp1 != 0) continue;

			/* Hack!!! ignore unconnected p-well ports, flag the others */
			if (namesame(pp->protoname, x_("P-well")) != 0 &&
				namesame(pp->protoname, x_("Ntrans-P-well")) != 0 &&
				namesame(pp->protoname, x_("P-welln")) != 0)
			{
				count_unconn++;
				xprintf(f, x_("| Warning: no connection to node %s port %s at (%ld,%ld)\n"),
					describenodeinst(ni), pp->protoname, ni->lowx, ni->lowy);
			}
			marea = parea = darea = dperim = 0;
			xprintf(f, x_("N %s 0 0 %g %g %g %g\n"), sim_makenodename(sim_index, format),
				scaletodispunitsq(marea, DISPUNITMIC), scaletodispunitsq(parea, DISPUNITMIC),
					scaletodispunitsq(darea, DISPUNITMIC), scaletodispunit(dperim, DISPUNITMIC));
			pp->temp1 = sim_index++;
		}

		/* some static checking while we are at it */
		if ((type == NPSUBSTRATE && np->firstportproto->temp1 != sim_vdd) ||
			(type == NPWELL && np->firstportproto->temp1 != sim_gnd))
		{
			ttyputmsg(_("WARNING: node %s in cell %s not connected to proper supply"),
				describenodeinst(ni), describenodeproto(cell));
		}

		/* if it is a transistor, write the information */
		if (type == NPTRANMOS || type == NPTRADMOS || type == NPTRAPMOS || type == NPTRANS)
		{
			if (type == NPTRANS)
			{
				/* undefined transistor: look at its transistor type */
				type = nodefunction(ni);

				/* gate is port 0, source is port 1, drain is port 2 */
				p = np->firstportproto;    gate = p->temp1;
				p = p->nextportproto;      source = p->temp1;
				p = p->nextportproto;      drain = p->temp1;
			} else
			{
				/* gate is port 0 or 2, source is port 1, drain is port 3 */
				p = np->firstportproto;                gate = p->temp1;
				p = p->nextportproto;                  source = p->temp1;
				p = p->nextportproto->nextportproto;   drain = p->temp1;
			}

			if (type == NPTRANMOS)
			{
				xprintf(f, x_("e"));
			} else if (type == NPTRADMOS)
			{
				xprintf(f, x_("d"));
			} else if (type == NPTRAPMOS)
			{
				xprintf(f, x_("p"));
			} else
			{
				xprintf(f, x_("U"));
			}
			xprintf(f, x_(" %s"), sim_makenodename(gate, format));
			xprintf(f, x_(" %s"), sim_makenodename(source, format));
			xprintf(f, x_(" %s"), sim_makenodename(drain, format));

			/* determine size of transistor */
			transistorsize(ni, &length, &width);
			if (length < 0) length = 0;
			if (width < 0) width = 0;
			xprintf(f, x_(" %g %g r 0 0 %g\n"), scaletodispunit(abs(length), DISPUNITMIC),
				scaletodispunit(abs(width), DISPUNITMIC),
					scaletodispunitsq(abs(length*width), DISPUNITMIC));

			/* approximate source and drain diffusion capacitances */
			xprintf(f, x_("N %s"), sim_makenodename(source, format));
			xprintf(f, x_(" 0 0 0 0 %g %g\n"), scaletodispunitsq(abs(length*width), DISPUNITMIC),
				scaletodispunit(abs(width), DISPUNITMIC));
			xprintf(f, x_("N %s"), sim_makenodename(drain, format));
			xprintf(f, x_(" 0 0 0 0 %g %g\n"),scaletodispunitsq(abs(length*width), DISPUNITMIC),
				scaletodispunit(abs(width), DISPUNITMIC));
			continue;
		}

		/* recurse on nonprimitive nodeinst */
		if (np->primindex == 0)
		{
			sim_simprint(np, f, format);
		}
	}
}

void sim_simprintcosmos(NODEPROTO *cell, FILE *f, INTBIG format)
{
	REGISTER PORTPROTO *gate, *source, *drain;
	REGISTER INTBIG type;
	INTBIG darea, dperim, parea, pperim, marea, mperim, length, width;
	REGISTER INTBIG lambda, lambda_2;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp, *spt;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np, *snp;
	REGISTER VARIABLE *var;

	/* stop if requested */
	if (stopping(STOPREASONDECK)) return;

	xprintf(f, x_("| cell %s\n"), cell->protoname);

	lambda = lambdaofcell(cell); /* setup lambda for scaling */
	lambda_2 = lambda*lambda;

	/* reset the arcinst node values */
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		ai->temp1 = 0;

	/* set every arcinst to a global node number (from inside or outside) */
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->temp1 != 0) continue;

		/* see if this arcinst is connected to a port */
		for(pp = cell->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			if (pp->network != ai->network) continue;
			darea = dperim = parea = pperim = marea = mperim = 0;
			sim_prop(ai, (INTBIG)pp->temp1, &darea, &dperim, &parea, &pperim, &marea, &mperim);
			if (pp->temp1 != sim_vdd && pp->temp1 != sim_gnd &&
				(mperim != 0 || pperim != 0 || dperim != 0))
					xprintf(f, x_("N %s %ld %ld %ld %ld %ld %ld\n"), sim_makenodename(pp->temp1, format),
						darea/lambda_2, dperim/lambda, parea/lambda_2, pperim/lambda,
							marea/lambda_2, mperim/lambda);
			break;
		}

		/* if not on a portinst, this is an internal node */
		if (pp == NOPORTPROTO)
		{
			darea = dperim = parea = pperim = marea = mperim = 0;
			sim_prop(ai, sim_index, &darea, &dperim, &parea, &pperim, &marea, &mperim);
			if (mperim != 0 || pperim != 0 || dperim != 0)
				xprintf(f, x_("N %s %ld %ld %ld %ld %ld %ld\n"), sim_makenodename(sim_index, format),
					darea/lambda_2, dperim/lambda, parea/lambda_2, pperim/lambda, marea/lambda_2,
						mperim/lambda);
			sim_index++;
		}
	}

	/* Test each arc for attributes */
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		var = getvalkey((INTBIG)ai, VARCINST, -1, sim_cosmos_attribute_key);
		if (var != NOVARIABLE)
		{
			if (var->type == VINTEGER)
				xprintf(f, x_("A %s Sim:%ld\n"), sim_makenodename(ai->temp1, format), var->addr); else
					xprintf(f, x_("A %s Sim:%s\n"), sim_makenodename(ai->temp1, format), (CHAR *)var->addr);
		}
	}

	/* look at every nodeinst in the cell */
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* initialize network numbers on every portinst on this nodeinst */
		np = ni->proto;
		type = (np->userbits&NFUNCTION) >> NFUNCTIONSH;
		snp = np;
		if (np->primindex == 0)
		{
			type = NPUNKNOWN;

			/* ignore recursive references (showing icon in contents) */
			if (isiconof(np, cell)) continue;
			np = contentsview(snp);
			if (np == NONODEPROTO) np = snp;
		}

		/* ignore artwork */
		if (type == NPART) continue;

		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			pp->temp1 = 0;

		/* set network numbers from arcs and exports */
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			pp = pi->proto;
			if (snp != np)
			{
				pp = equivalentport(snp, pp, np);
				if (pp == NOPORTPROTO) continue;
			}
			if (pp->temp1 != 0) continue;
			pp->temp1 = pi->conarcinst->temp1;
			if (pp->temp1 == 0) continue;
			for(spt = np->firstportproto; spt != NOPORTPROTO; spt = spt->nextportproto)
				if (spt->network == pp->network) spt->temp1 = pp->temp1;
		}
		for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		{
			pp = pe->proto;
			if (snp != np)
			{
				pp = equivalentport(snp, pp, np);
				if (pp == NOPORTPROTO) continue;
			}
			if (pp->temp1 != 0) continue;
			pp->temp1 = pe->exportproto->temp1;
			if (pp->temp1 == 0) continue;
			for(spt = np->firstportproto; spt != NOPORTPROTO; spt = spt->nextportproto)
				if (spt->network == pp->network) spt->temp1 = pp->temp1;
		}

		/* look for unconnected ports and assign new network numbers */
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			if (pp->temp1 != 0) continue;

			/* Hack!!! ignore unconnected well and select ports, flag the others */
			if (namesame(pp->protoname, x_("well")) != 0 &&
				namesame(pp->protoname, x_("select")) != 0)
			{
				count_unconn++;
				xprintf(f, x_("| Warning: no connection to node %s port %s at (%ld,%ld)\n"),
					describenodeinst(ni), pp->protoname, ni->lowx, ni->lowy);

				/* Calculate area and perimiter of nodes! */
				darea = dperim = parea = pperim = marea = mperim = 0;
				xprintf(f, x_("N %s %ld %ld %ld %ld %ld %ld\n"),
					sim_makenodename(sim_index, format), darea/lambda_2, dperim/lambda,
						parea/lambda_2, pperim/lambda, marea/lambda_2, mperim/lambda);
				pp->temp1 = sim_index++;
			}
		}

		/* some static checking while we are at it */
		if ((type == NPSUBSTRATE && np->firstportproto->temp1 != sim_vdd) ||
			(type == NPWELL && np->firstportproto->temp1 != sim_gnd))
		{
			ttyputmsg(_("WARNING: node %s in cell %s not connected to proper supply"),
				describenodeinst(ni), describenodeproto(cell));
		}

		/* if it is a transistor, write the information */
		if (type == NPTRANMOS || type == NPTRADMOS || type == NPTRAPMOS || type == NPTRANS)
		{
			if (type == NPTRANS)
			{
				/* undefined transistor: look at its transistor type */
				type = nodefunction(ni);

				/* gate is port 0, source is port 1, drain is port 2 */
				gate = np->firstportproto;
				source = gate->nextportproto;
				drain = source->nextportproto;
			} else
			{
				/* gate is port 0 or 2, source is port 1, drain is port 3 */
				gate = np->firstportproto;
				source = gate->nextportproto;
				drain = source->nextportproto->nextportproto;
			}

			if (type == NPTRANMOS)
			{			/* nmos processes don't have a 'c' */
				if (estrchr(el_curtech->techname, 'c') != 0) xprintf(f, x_("n"));
				else xprintf(f, x_("e"));
			} else
				if (type == NPTRADMOS) xprintf(f, x_("d")); else
					if (type == NPTRAPMOS) xprintf(f, x_("p")); else
						xprintf(f, x_("U"));
			xprintf(f, x_(" %s"), sim_makenodename(gate->temp1, format));
			xprintf(f, x_(" %s"), sim_makenodename(source->temp1, format));
			xprintf(f, x_(" %s"), sim_makenodename(drain->temp1, format));

			/* determine size of transistor */
			transistorsize(ni, &length, &width);
			if (length < 0) length = 0;
			if (width < 0) width = 0;
			xprintf(f, x_(" %ld %ld %ld %ld"), abs(length)/lambda, abs(width)/lambda, ni->lowx, ni->lowy);

			/* see if there is a strength mentioned on this node */
			for (pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			{
				var = getvalkey((INTBIG)pi, VPORTARCINST, -1, sim_cosmos_attribute_key);
				if (var != NOVARIABLE)
				{
					if (pi->proto->network == source->network) xprintf(f, x_(" s=")); else
						if (pi->proto->network == drain->network) xprintf(f, x_(" d=")); else
							if (pi->proto->network == gate->network) xprintf(f, x_(" g=")); else
								ttyputerr(_("Bad cosmos gate/source/drain transistor variable %s"),
									(CHAR *)var->addr);
					if (var->type == VINTEGER) xprintf(f, x_("Sim:%ld"), var->addr); else
						if (var->type == VSTRING) xprintf(f, x_("Sim:%s"), (CHAR *)var->addr); else
							ttyputerr(_("Bad cosmos attribute type"));
				}
			}
			var = getvalkey((INTBIG)ni, VNODEINST, -1, sim_cosmos_attribute_key);
			if (var != NOVARIABLE)
			{
				if (var->type == VINTEGER) xprintf(f, x_(" g=Sim:%ld"), var->addr);
					else xprintf(f, x_(" g=Sim:%s"), (CHAR *)var->addr);
			}
			xprintf(f, x_("\n"));

			/* approximate source and drain diffusion capacitances */
			xprintf(f, x_("N %s"), sim_makenodename(source->temp1, format));
			xprintf(f, x_(" %ld %ld 0 0 0 0\n"), abs(length*width)/lambda_2, abs(width)/lambda);
			xprintf(f, x_("N %s"), sim_makenodename(drain->temp1, format));
			xprintf(f, x_(" %ld %ld 0 0 0 0\n"), abs(length*width)/lambda_2, abs(width)/lambda);
			continue;
		}

		/* recurse on nonprimitive nodeinst */
		if (np->primindex == 0)
		{
			sim_simprintcosmos(np, f, format);
		}
	}
}

/*
 * routine to generate the name of simulation node "node" and return the
 * string name (either a numeric node number if internal, an export
 * name if at the top level, or "power", "ground", etc. if special).  The
 * "format" is the particular simuator being used
 */
CHAR *sim_makenodename(INTBIG node, INTBIG format)
{
	static CHAR line[50];
	REGISTER PORTPROTO *pp;

	/* values above a threshold are internal and printed as numbers */
	if (node >= sim_namethresh)
	{
		(void)esnprintf(line, 50, x_("%ld"), node);
		return(line);
	}

	/* test for special names */
	if (node == sim_vdd) return(x_("vdd"));
	if (node == sim_gnd) return(x_("gnd"));
	if (node == sim_phi1h)
	{
		if (format == RSIM) return(x_("phi1h")); else return(x_("clk1"));
	}
	if (node == sim_phi1l) return(x_("phi1l"));
	if (node == sim_phi2h)
	{
		if (format == RSIM) return(x_("phi2h")); else return(x_("clk2"));
	}
	if (node == sim_phi2l) return(x_("phi2l"));

	/* see if it is an export name */
	for(pp = sim_simnt->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		if (pp->temp1 == node && pp->temp2 >= 0) break;
	if (pp != NOPORTPROTO) return(pp->protoname);

	/* this is an error, but can be handled anyway */
	ttyputmsg(_("simsim: should have real name for node %ld"), node);
	(void)esnprintf(line, 50, x_("%ld"), node);
	return(line);
}

void sim_prop(ARCINST *ai, INTBIG indx, INTBIG *darea, INTBIG *dperim, INTBIG *parea,
	INTBIG *pperim, INTBIG *marea, INTBIG *mperim)
{
	REGISTER ARCINST *oar;
	REGISTER NODEINST *ni;
	REGISTER PORTARCINST *pi;
	REGISTER PORTPROTO *pp;
	REGISTER INTBIG i;
	INTBIG length, width;

	/* see if this arcinst is already on a path */
	if (ai->temp1 != 0) return;
	ai->temp1 = indx;
	if (((ai->proto->userbits&AFUNCTION) >> AFUNCTIONSH) == APNONELEC)
		return;

	/* calculate true length and width of arc */
	width = ai->width - arcwidthoffset(ai);
	length = ai->length;
	if ((ai->userbits&NOEXTEND) != 0)
	{
		if ((ai->userbits&NOTEND0) == 0) length += width/2;
		if ((ai->userbits&NOTEND1) == 0) length += width/2;
	} else length += width;

	/* sum up area and perimeter to total */
	switch ((ai->proto->userbits&AFUNCTION) >> AFUNCTIONSH)
	{
		case APMETAL1:   case APMETAL2:   case APMETAL3:
		case APMETAL4:   case APMETAL5:   case APMETAL6:
		case APMETAL7:   case APMETAL8:   case APMETAL9:
		case APMETAL10:  case APMETAL11:  case APMETAL12:
			*marea += length*width;
			*mperim += 2 * (length + width);
			break;
		case APPOLY1:    case APPOLY2:    case APPOLY3:
			*parea += length*width;
			*pperim += 2 * (length + width);
			break;
		case APDIFF:     case APDIFFP:    case APDIFFN:
		case APDIFFS:    case APDIFFW:
			*darea += length*width;
			*dperim += 2 * (length + width);
			break;
	}

	/* recursively set all arcs and nodes touching this */
	for(i=0; i<2; i++)
	{
		if ((ai->end[i].portarcinst->proto->userbits&PORTISOLATED) != 0) continue;
		ni = ai->end[i].nodeinst;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			/* select an arcinst that has not been examined */
			pp = pi->proto;
			oar = pi->conarcinst;

			/* see if the two ports connect electrically */
			if (ai->end[i].portarcinst->proto->network != pp->network) continue;

			/* recurse on the nodes of this arcinst */
			sim_prop(oar, indx, darea, dperim, parea, pperim, marea, mperim);
		}
	}
}

#endif  /* SIMTOOL - at top */
