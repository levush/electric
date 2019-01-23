/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: simmossim.c
 * MOSSIM net list generator
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

/*
 * A circuit can be augmented in a number of ways for MOSSIM output:
 * The variable "SIM_mossim_strength" may be placed on any node or arc.
 * On transistor nodes, it specifies the strength field to use for that
 * transistor declaration.  On arcs, it specifies the strength field to
 * use for that network node (only works for internal nodes of a cell).
 *
 * The use of "icon" views of cells as instances works correctly and will
 * replace them with the contents view.
 */

#include "config.h"
#if SIMTOOL

#include "global.h"
#include "efunction.h"
#include "sim.h"
#include "tecschem.h"

static FILE *sim_mossimfile;
static INTBIG sim_mossiminternal;
static INTBIG sim_mossim_strength_key = 0;

/* prototypes for local routines */
static BOOLEAN sim_writemossimcell(NODEPROTO*, BOOLEAN);
static void    sim_addmossimnode(void*, NODEINST*, PORTPROTO*);
static CHAR   *sim_mossim_netname(ARCINST*);

/*
 * routine to write a ".ntk" file from the cell "np"
 */
void sim_writemossim(NODEPROTO *np)
{
	CHAR name[100], *truename;
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

	(void)estrcpy(name, np->protoname);
	(void)estrcat(name, x_(".ntk"));
	sim_mossimfile = xcreate(name, sim_filetypemossim, _("MOSSIM File"), &truename);
	if (sim_mossimfile == NULL)
	{
		if (truename != 0) ttyputerr(_("Cannot write %s"), truename);
		return;
	}

	/* get variable names (only once) */
	if (sim_mossim_strength_key == 0)
		sim_mossim_strength_key = makekey(x_("SIM_mossim_strength"));

	/* reset flags for cells that have been written */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(lnp = lib->firstnodeproto; lnp != NONODEPROTO; lnp = lnp->nextnodeproto)
			lnp->temp1 = 0;
	if (sim_writemossimcell(np, TRUE))
		ttyputmsg(_("Back-annotation information has been added (library must be saved)"));

	xclose(sim_mossimfile);
	ttyputmsg(_("%s written"), truename);
}

/*
 * recursively called routine to print MOSSIM description of cell "np"
 * The description is treated as the top-level cell if "top" is true.
 */
BOOLEAN sim_writemossimcell(NODEPROTO *np, BOOLEAN top)
{
	REGISTER INTBIG type, inst, i, strength;
	REGISTER BOOLEAN backannotate;
	REGISTER VARIABLE *var;
	REGISTER NODEPROTO *snp, *subnp;
	CHAR temp[20], *str;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai, *nai;
	REGISTER PORTPROTO *pp, *opp, *gate, *source, *drain, *spp;
	REGISTER void *infstr;

	/* stop if requested */
	if (stopping(STOPREASONDECK)) return(FALSE);

	/* mark this cell as written */
	np->temp1 = 1;

	/* make sure all sub-cells have been written */
	backannotate = FALSE;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		subnp = ni->proto;
		if (subnp->primindex != 0) continue;

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(subnp, np)) continue;
		snp = contentsview(subnp);
		if (snp == NONODEPROTO) snp = subnp;
		if (snp->temp1 != 0) continue;
		if (sim_writemossimcell(snp, FALSE)) backannotate = TRUE;
	}

	/* make sure that all nodes have names on them */
	if (asktool(net_tool, x_("name-nodes"), (INTBIG)np) != 0) backannotate = TRUE;

	/* initialize instance counter */
	inst = 1;

	if (top)
	{
		/* declare power and ground nodes if this is top cell */
		xprintf(sim_mossimfile, x_("| Top-level cell %s ;\n"), describenodeproto(np));
		xprintf(sim_mossimfile, x_("i VDD ;\ni GND ;\n"));
	} else
	{
		/* write ports if this cell is sub-cell */
		xprintf(sim_mossimfile, x_("| Cell %s ;\nc %s"), describenodeproto(np), np->protoname);
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			xprintf(sim_mossimfile, x_(" %s"), pp->protoname);
		xprintf(sim_mossimfile, x_(" ;\n"));
	}

	/* mark all ports that are equivalent */
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto) pp->temp1 = 0;
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		if (pp->temp1 != 0) continue;
		pp->temp1 = 1;
		if (top && portispower(pp))
		{
			if (namesame(pp->protoname, x_("vdd")) != 0)
				xprintf(sim_mossimfile, x_("e VDD %s ;\n"), pp->protoname);
			continue;
		}
		if (top && portisground(pp))
		{
			if (namesame(pp->protoname, x_("gnd")) != 0)
				xprintf(sim_mossimfile, x_("e GND %s ;\n"), pp->protoname);
			continue;
		}
		if (top)
		{
			if ((pp->userbits&STATEBITS) == INPORT)
				xprintf(sim_mossimfile, x_("i %s ;\n"), pp->protoname); else
					xprintf(sim_mossimfile, x_("s 1 %s ;\n"), pp->protoname);
		}
		i = 0;
		for(opp = pp->nextportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
			if (opp->network == pp->network)
		{
			if (i == 0) xprintf(sim_mossimfile, x_("e %s"), pp->protoname);
			xprintf(sim_mossimfile, x_(" %s"), opp->protoname);
			i++;
			opp->temp1 = 1;
		}
		if (i != 0) xprintf(sim_mossimfile, x_(" ;\n"));
	}

	/* determine internal networks and give them a name */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst) ai->temp1 = 0;
	sim_mossiminternal = 0;
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->temp1 != 0) continue;

		/* see if arc is connected to a port */
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			if (pp->network == ai->network) break;

		/* if connected to port, save that port location */
		if (pp != NOPORTPROTO)
		{
			for(nai = np->firstarcinst; nai != NOARCINST; nai = nai->nextarcinst)
			{
				if (nai->network != ai->network) continue;

				/* save the port (see comments on "sim_mossim_netname") */
				nai->temp1 = -1;
				nai->temp2 = (INTBIG)pp;
			}
			continue;
		}

		/* not connected to a port: generate a local node name */
		sim_mossiminternal++;
		for(nai = np->firstarcinst; nai != NOARCINST; nai = nai->nextarcinst)
		{
			if (nai->network != ai->network) continue;
			for(i=0; i<2; i++)
			{
				if (nai->end[i].nodeinst->proto->primindex != 0) continue;
				infstr = initinfstr();
				addstringtoinfstr(infstr, nai->end[i].portarcinst->proto->protoname);
				(void)esnprintf(temp, 20, x_("-%ld"), sim_mossiminternal);
				addstringtoinfstr(infstr, temp);
				(void)allocstring(&str, returninfstr(infstr), el_tempcluster);
				break;
			}
		}
		if (nai == NOARCINST)
		{
			if (ai->network == NONETWORK || ai->network->namecount == 0)
			{
				(void)esnprintf(temp, 20, x_("node%ld"), sim_mossiminternal);
				(void)allocstring(&str, temp, el_tempcluster);
			} else (void)allocstring(&str, networkname(ai->network, 0), el_tempcluster);
		}

		/* store this name on every connected internal arc */
		strength = -1;
		for(nai = np->firstarcinst; nai != NOARCINST; nai = nai->nextarcinst)
		{
			if (nai->network != ai->network) continue;

			/* see if there is a strength mentioned on this node */
			var = getvalkey((INTBIG)nai, VARCINST, VINTEGER, sim_mossim_strength_key);
			if (var != NOVARIABLE)
			{
				if (var->addr < 0)
					ttyputerr(_("Warning: arc %s has negative strength"), describearcinst(nai)); else
				{
					if (strength >= 0 && strength != var->addr)
						ttyputerr(_("Warning: net has multiple strengths: %ld and %ld"), strength,
							var->addr);
					strength = var->addr;
				}
			}

			/* save the name (see comments on "sim_mossim_netname") */
			nai->temp1 = sim_mossiminternal;
			nai->temp2 = (INTBIG)str;
		}

		/* print the internal node name */
		if (strength < 0) strength = 1;
		xprintf(sim_mossimfile, x_("s %ld %s ;\n"), strength, str);
	}

	/* now write the transistors */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		subnp = ni->proto;
		if (subnp->primindex != 0)
		{
			/* handle primitives */
			type = (subnp->userbits&NFUNCTION) >> NFUNCTIONSH;

			/* if it is a transistor, write the information */
			if (type != NPTRANMOS && type != NPTRADMOS && type != NPTRAPMOS && type != NPTRANS)
				continue;

			/* get exact type and ports for general transistors */
			if (type == NPTRANS)
			{
				/* undefined transistor: look at its transistor type */
				type = nodefunction(ni);

				/* gate is port 0, source is port 1, drain is port 2 */
				gate = subnp->firstportproto;
				source = gate->nextportproto;
				drain = source->nextportproto;
			} else
			{
				/* gate is port 0 or 2, source is port 1, drain is port 3 */
				gate = subnp->firstportproto;
				source = gate->nextportproto;
				drain = source->nextportproto->nextportproto;
			}

			/* write the transistor */
			infstr = initinfstr();
			addtoinfstr(infstr, (CHAR)(type == NPTRANMOS ? 'n' : (type == NPTRADMOS ? 'd' : 'p')));

			/* write the strength of the transistor */
			var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, sim_mossim_strength_key);
			if (var != NOVARIABLE)
			{
				formatinfstr(infstr, x_(" %ld"), var->addr);
			} else addstringtoinfstr(infstr, x_(" 2"));

			/* write the gate/source/drain nodes */
			sim_addmossimnode(infstr, ni, gate);
			sim_addmossimnode(infstr, ni, source);
			sim_addmossimnode(infstr, ni, drain);
			addstringtoinfstr(infstr, x_(" ;"));
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
			if (var != NOVARIABLE)
			{
				addstringtoinfstr(infstr, x_("   | "));
				addstringtoinfstr(infstr, (CHAR *)var->addr);
				addstringtoinfstr(infstr, x_(";"));
			}
			addstringtoinfstr(infstr, x_("\n"));
			xprintf(sim_mossimfile, x_("%s"), returninfstr(infstr));
		} else
		{
			/* ignore recursive references (showing icon in contents) */
			if (isiconof(subnp, np)) continue;

			/* complex node: make instance call */
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("h "));

			/* see if this cell is an icon for the real thing */
			snp = contentsview(subnp);
			if (snp != NONODEPROTO)
				addstringtoinfstr(infstr, snp->protoname); else
					addstringtoinfstr(infstr, subnp->protoname);
			addtoinfstr(infstr, ' ');

			/* get appropriate name of node */
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
			if (var == NOVARIABLE)
			{
				ttyputerr(_("MOSSIM generation warning: no name on node %s"),
					describenodeinst(ni));
				addstringtoinfstr(infstr, x_("INST"));
				formatinfstr(infstr, x_("%ld"), inst++);
			} else addstringtoinfstr(infstr, (CHAR *)var->addr);

			if (snp != NONODEPROTO)
			{
				/* icon cell: report ports according to order on contents */
				for(pp = snp->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				{
					spp = equivalentport(snp, pp, subnp);
					if (spp == NOPORTPROTO) continue;
					sim_addmossimnode(infstr, ni, spp);
				}
			} else
			{
				/* normal cell: simply report ports in order */
				for(pp = subnp->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
					sim_addmossimnode(infstr, ni, pp);
			}
			addstringtoinfstr(infstr, x_(" ;\n"));
			xprintf(sim_mossimfile, x_("%s"), returninfstr(infstr));
		}
	}

	/* finish up */
	xprintf(sim_mossimfile, x_(".\n"));

	/* free space from internal node names */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->temp1 <= 0) continue;
		efree((CHAR *)ai->temp2);
		for(nai = np->firstarcinst; nai != NOARCINST; nai = nai->nextarcinst)
			if (nai->network == ai->network) nai->temp1 = 0;
	}
	return(backannotate);
}

/*
 * routine to add the proper node name for port "pp" of node "ni" to the
 * infinite string
 */
void sim_addmossimnode(void *infstr, NODEINST *ni, PORTPROTO *pp)
{
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER PORTPROTO *opp;
	REGISTER ARCINST *ai;
	REGISTER INTBIG i;
	CHAR temp[20];

	/* initialize for internal node memory (see comment below) */
	pp->temp2 = 0;

	/* see if this node is connected to an arc */
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		if (pi->proto->network != pp->network) continue;
		ai = pi->conarcinst;
		addtoinfstr(infstr, ' ');
		addstringtoinfstr(infstr, sim_mossim_netname(ai));
		return;
	}

	/* see if this node is connected directly to a port */
	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
	{
		if (pe->proto->network != pp->network) continue;
		addtoinfstr(infstr, ' ');
		addstringtoinfstr(infstr, pe->exportproto->protoname);
		return;
	}

	/*
	 * a strange situation may occur here: there may be a node with two
	 * unconnected ports that do connect inside the cell.  Rather than write
	 * two internal node names, there should only be one.  Therefore, when
	 * internal node names are assigned, they are stored on the port prototype
	 * so that subsequent unconnected equivalent ports can use the same name.
	 */
	for(opp = ni->proto->firstportproto; opp != pp; opp = opp->nextportproto)
	{
		if (opp->network != pp->network) continue;
		if (opp->temp2 == 0)
		{
			sim_addmossimnode(infstr, ni, opp);
			return;
		}
		break;
	}

	/* determine the internal net number to use */
	if (opp == pp)
	{
		sim_mossiminternal++;
		i = sim_mossiminternal;
		(void)esnprintf(temp, 20, x_("node%ld"), i);
		xprintf(sim_mossimfile, x_("s 1 %s ;\n"), temp);
	} else
	{
		i = opp->temp2;
		(void)esnprintf(temp, 20, x_("node%ld"), i);
	}
	pp->temp2 = i;

	addtoinfstr(infstr, ' ');
	addstringtoinfstr(infstr, temp);
}

/*
 * routine to write the net name of arc "ai".  There are two types of arcs:
 * those connected to exports, and those that are internal.  When
 * the "temp1" field is -1, the arc is connected to an export which
 * is saved in the "temp2" field.  In other cases, the actual string name
 * of the net is in the "temp2" field.
 */
CHAR *sim_mossim_netname(ARCINST *ai)
{
	if (ai->temp1 == -1) return(((PORTPROTO *)ai->temp2)->protoname);
	return((CHAR *)ai->temp2);
}

#endif  /* SIMTOOL - at top */
