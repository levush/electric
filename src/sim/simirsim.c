/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: simirsim.c
 * Simulation tool: IRSIM deck generator
 * Written by: Steven M. Rubin, Static Free Software
 *
 * Copyright (c) 2001 Static Free Software.
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
#include "simirsim.h"
#include "efunction.h"
#include "egraphics.h"
#include "network.h"
#include "usr.h"
#include "tecschem.h"
#include <math.h>

       IRSIMTRANSISTOR *sim_firstirsimtransistor = NOIRSIMTRANSISTOR;
static IRSIMTRANSISTOR *sim_irsimtransistorfree = NOIRSIMTRANSISTOR;
static IRSIMTRANSISTOR *sim_irsimcelllisthead;
	   IRSIMNETWORK    *sim_irsimnets;
       INTBIG           sim_irsimnetnumber = 0;
static INTBIG           sim_irsimnettotal = 0;
#if SIMFSDBWRITE != 0
static INTBIG           sim_irsimtopnetnumber;
	   IRSIMALIAS      *sim_irsimaliases;
       INTBIG           sim_irsimaliasnumber = 0;
static INTBIG           sim_irsimaliastotal = 0;
#endif
       INTBIG           sim_irsim_statekey;		/* variable key for "SIM_irsim_state" */
static INTBIG           sim_irsim_state;		/* cached value of "SIM_irsim_state" variable */
static INTBIG           sim_irsimlambda;
static INTBIG           sim_irsiminternal;
static INTBIG           sim_irsimglobalnetcount;	/* number of global nets */
static INTBIG           sim_irsimglobalnettotal = 0;/* size of global net array */
static CHAR           **sim_irsimglobalnets;		/* global net names */
static INTBIG          *sim_irsimglobalnetsnumbers;	/* global net numbers */
static void            *sim_irsimmerge;				/* for polygon merging */

/* prototypes for local routines */
static void             sim_irsimaddarctomerge(ARCINST *ai, XARRAY trans);
static void             sim_irsimaddnodetomerge(NODEINST *ni, XARRAY localtrans);
static IRSIMTRANSISTOR *sim_irsimalloctransistor(void);
static CHAR            *sim_irsimbuildnetname(CHAR *name);
static BOOLEAN          sim_irsimclipagainstgates(POLYGON *poly, ARCINST *ai, XARRAY trans);
static BOOLEAN          sim_irsimcropbox(INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy, INTBIG bx,
							INTBIG ux, INTBIG by, INTBIG uy);
static void             sim_irsimfreenetworkinfo(void);
static void             sim_irsimfreetransistor(IRSIMTRANSISTOR *it);
static void             sim_irsimgatherglobals(NODEPROTO *np);
static void             sim_irsimgetallgeometry(INTBIG layer, TECHNOLOGY *tech, INTBIG *x, INTBIG *y,
							INTBIG count);
static void             sim_irsimgetareaperimeter(INTBIG *x, INTBIG *y, INTBIG count,
							float *a, INTBIG *p);
static INTBIG           sim_irsimgetglobal(NETWORK *net);
static INTBIG           sim_irsimnewnetnumber(CHAR *name);
#if SIMFSDBWRITE != 0
static void             sim_irsimnewalias(INTBIG netnum, CHAR *alias);
#endif
static INTBIG           sim_irsimrecurse(NODEPROTO *cell, XARRAY trans, float m);

/*
 * Routine to free all memory associated with this module.
 */
void sim_freeirsimmemory(void)
{
	REGISTER IRSIMTRANSISTOR *it;
	REGISTER INTBIG i;

	sim_irsimfreenetworkinfo();
	while (sim_irsimtransistorfree != NOIRSIMTRANSISTOR)
	{
		it = sim_irsimtransistorfree;
		sim_irsimtransistorfree = it->nextirsimtransistor;
		efree((CHAR *)it);
	}
	if (sim_irsimnettotal > 0)
		efree((CHAR *)sim_irsimnets);

	for(i=0; i<sim_irsimglobalnettotal; i++)
		if (sim_irsimglobalnets[i] != 0)
			efree((CHAR *)sim_irsimglobalnets[i]);
	if (sim_irsimglobalnettotal > 0)
	{
		efree((CHAR *)sim_irsimglobalnets);
		efree((CHAR *)sim_irsimglobalnetsnumbers);
	}
}

/*
 * Routine to write a simulation deck for cell "cell".  This is called when the user
 * just requests a deck, so the file is not temporary.
 */
void sim_writeirsim(NODEPROTO *cell)
{
	REGISTER FILE *f;
	CHAR *truename;
	REGISTER CHAR *netfile;
	REGISTER void *infstr;

	/* make sure network tool is on */
	if ((net_tool->toolstate&TOOLON) == 0)
	{
		ttyputerr(_("Network tool must be running...turning it on"));
		toolturnon(net_tool);
		ttyputerr(_("...now reissue the simulation command"));
		return;
	}

	/* construct the simulation description file name from the cell */
	infstr = initinfstr();
	addstringtoinfstr(infstr, cell->protoname);
	addstringtoinfstr(infstr, x_(".sim"));
	netfile = returninfstr(infstr);

	/* create the simulation file */
	f = xcreate(netfile, sim_filetypeirsim, _("Netlist File"), &truename);
	if (f == NULL)
	{
		if (truename != 0) ttyputerr(_("Cannot write %s"), truename);
		return;
	}

	ttyputmsg(_("Writing IRSIM deck..."));
	sim_irsiminternal = 1;
	sim_irsimgeneratedeck(cell, f);

	xclose(f);
	ttyputmsg(_("%s written"), truename);
}

typedef struct
{
	CHAR *signalname;
	INTBIG signalindex;
} UNIQUESIGNALSORT;

int sim_irsimsortsignalnames(const void *e1, const void *e2);

int sim_irsimsortsignalnames(const void *e1, const void *e2)
{
	UNIQUESIGNALSORT *uss1, *uss2;
	INTBIG diff;

	uss1 = (UNIQUESIGNALSORT *)e1;
	uss2 = (UNIQUESIGNALSORT *)e2;
	diff = namesame(uss1->signalname, uss2->signalname);
	if (diff != 0) return(diff);
	return(uss1->signalindex - uss2->signalindex);
}

/*
 * Routine to generate an IRSIM deck for cell "cell" and write it to stream "f".
 */
void sim_irsimgeneratedeck(NODEPROTO *cell, FILE *f)
{
	REGISTER INTBIG backannotate;
	REGISTER TECHNOLOGY *tech;
	CHAR numberstring[80];
	REGISTER IRSIMTRANSISTOR *it;
	REGISTER PORTPROTO *pp;
	REGISTER NETWORK *net;
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var;
	REGISTER CHAR *name;
	REGISTER INTBIG sa, sp, da, dp, i, j;
	UNIQUESIGNALSORT *uss;
	INTBIG index, nextindex;

	/* get value of lambda */
	tech = cell->tech;
	sim_irsimlambda = cell->lib->lambda[tech->techindex];

	/* get IRSIM state */
	var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_irsim_statekey);
	if (var == NOVARIABLE) sim_irsim_state = DEFIRSIMSTATE; else
		sim_irsim_state = var->addr;

	/* initialize the list of networks */
	sim_irsimfreenetworkinfo();
	sim_irsimnetnumber = 0;
#if SIMFSDBWRITE != 0
	sim_irsimaliasnumber = 0;
#endif

	/* gather global signal names */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = 0;
	sim_irsimglobalnetcount = 0;
	sim_irsimgatherglobals(cell);
	for(i=0; i<sim_irsimglobalnetcount; i++)
		sim_irsimglobalnetsnumbers[i] = sim_irsimnewnetnumber(sim_irsimglobalnets[i]);

	/* clear pointers on nodes to IRSIM transistors */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				ni->temp1 = 0;

	/*
	 * extract the IRSIM information from cell "cell".  Creates a linked
	 * list of transistors in "sim_firstirsimtransistor" and creates
	 * "sim_irsimnetnumber" networks in "sim_irsimnetnames".
	 */
	sim_firstirsimtransistor = NOIRSIMTRANSISTOR;
	backannotate = 0;
	if (asktool(net_tool, x_("name-nets"), (INTBIG)cell) != 0) backannotate++;
	for(net = cell->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		net->temp1 = -1;
	for(pp = cell->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		if (pp->network->temp1 != -1) continue;
		if (pp->network->buswidth > 1)
		{
			for(i=0; i<pp->network->buswidth; i++)
			{
				net = pp->network->networklist[i];
				if (net->globalnet >= 0) net->temp1 = sim_irsimgetglobal(net); else
				{
					if (net->namecount <= 0) continue;
					net->temp1 = sim_irsimnewnetnumber(networkname(net, 0));
				}
			}
		} else
		{
			net = pp->network;
			if (net->globalnet >= 0) net->temp1 = sim_irsimgetglobal(net); else
			{
				name = pp->protoname;
				if (net->namecount > 0) name = networkname(net, 0);
				if (portispower(pp)) name = x_("vdd"); else
					if (portisground(pp)) name = x_("gnd");
				net->temp1 = sim_irsimnewnetnumber(name);
			}
		}
	}
#if SIMFSDBWRITE != 0
	sim_irsimtopnetnumber = sim_irsimnetnumber;
#endif
	begintraversehierarchy();
	if ((sim_irsim_state&IRSIMPARASITICS) == IRSIMPARAFULL)
		sim_irsimmerge = mergenew(sim_tool->cluster);
	if (sim_irsimrecurse(cell, el_matid, 1.0) != 0) backannotate++;
	if ((sim_irsim_state&IRSIMPARASITICS) == IRSIMPARAFULL)
	{
		sim_irsimcelllisthead = NOIRSIMTRANSISTOR;
		mergeextract(sim_irsimmerge, sim_irsimgetallgeometry);
		mergedelete(sim_irsimmerge);
	}
	endtraversehierarchy();

	/* write deck to "f" */	
	var = getval((INTBIG)cell, VNODEPROTO, VSTRING, x_("SIM_irsim_behave_file"));
	if (var != NOVARIABLE)
	{
		/* this cell has a disk file of IRSIM associated with it */
		xprintf(f, x_("|\n")); /* First comment line is treated by IRSIM in a special way */
		xprintf(f, x_("| Cell %s is described in this file:\n"), describenodeproto(cell));
		xprintf(f, x_("@ %s\n"), (CHAR *)var->addr);
	} else
	{
		xprintf(f,x_("| units: %g tech: %s format: SU\n"),
			scaletodispunit(sim_irsimlambda, DISPUNITCMIC), tech->techname);
		xprintf(f, x_("| IRSIM file for cell %s from Library %s\n"),
			describenodeproto(cell), cell->lib->libname);
		us_emitcopyright(f, x_("| "), x_(""));
		if ((us_useroptions&NODATEORVERSION) == 0)
		{
			if (cell->creationdate)
				xprintf(f, x_("| Created on %s\n"), timetostring((time_t)cell->creationdate));
			if (cell->revisiondate)
				xprintf(f, x_("| Last revised on %s\n"), timetostring((time_t)cell->revisiondate));
			(void)esnprintf(numberstring, 80, x_("%s"), timetostring(getcurrenttime()));
			xprintf(f, x_("| Written on %s by Electric VLSI Design System, version %s\n"),
				numberstring, el_version);
		} else
		{
			xprintf(f, x_("| Written by Electric VLSI Design System\n"));
		}

		for(it = sim_firstirsimtransistor; it != NOIRSIMTRANSISTOR; it = it->nextirsimtransistor)
		{
			if (it->m != (INTBIG)it->m)
				ttyputmsg("Noninteger m factor in Irsim deck: %f\n", it->m);
			if (it->transistortype == NPTRANMOS) xprintf(f, x_("n")); else
				xprintf(f, x_("p"));
			xprintf(f, x_(" %s"), sim_irsimnets[it->gate].name);
			xprintf(f, x_(" %s"), sim_irsimnets[it->source].name);
			xprintf(f, x_(" %s"), sim_irsimnets[it->drain].name);
			xprintf(f, x_(" %s %s"), latoa(it->length, sim_irsimlambda), latoa((INTBIG)(it->width*it->m), sim_irsimlambda));
			xprintf(f, x_(" %s %s"), latoa(it->xpos, sim_irsimlambda), latoa(it->ypos, sim_irsimlambda));
			if (it->transistortype == NPTRANMOS) xprintf(f, x_(" g=S_gnd")); else
				xprintf(f, x_(" g=S_vdd"));
			sa = (INTBIG)(it->sarea / sim_irsimlambda);
			if (sa == 0.0) sa = it->width * 6;

			sa = (sa + sim_irsimlambda/2) / sim_irsimlambda * sim_irsimlambda;
			sp = (it->sperimeter + sim_irsimlambda/2) / sim_irsimlambda * sim_irsimlambda;
			if (sp == 0) sp = it->width + 12*sim_irsimlambda;
			da = (INTBIG)(it->darea / sim_irsimlambda);
			if (da == 0.0) da = it->width * 6;
			da = (da + sim_irsimlambda/2) / sim_irsimlambda * sim_irsimlambda;
			dp = (it->dperimeter + sim_irsimlambda/2) / sim_irsimlambda * sim_irsimlambda;
			if (dp == 0) dp = it->width + 12*sim_irsimlambda;
			xprintf(f, x_(" s=A_%s,P_%s"), latoa((INTBIG)(sa*it->m), sim_irsimlambda), latoa((INTBIG)(sp*it->m), sim_irsimlambda));
			xprintf(f, x_(" d=A_%s,P_%s\n"), latoa((INTBIG)(da*it->m), sim_irsimlambda), latoa((INTBIG)(dp*it->m), sim_irsimlambda));
		}
	}

	if (backannotate != 0)
		ttyputmsg(_("Back-annotation information has been added (library must be saved)"));

	/* mark redundant signals */
	uss = (UNIQUESIGNALSORT *)emalloc(sim_irsimnetnumber * (sizeof (UNIQUESIGNALSORT)), sim_tool->cluster);
	if (uss == 0) return;
	for(i=0; i<sim_irsimnetnumber; i++)
	{
		uss[i].signalindex = i;
		uss[i].signalname = sim_irsimnets[i].name;
	}
	esort(uss, sim_irsimnetnumber, sizeof (UNIQUESIGNALSORT), sim_irsimsortsignalnames);
	for(i=0; i<sim_irsimnetnumber; i++)
	{
		index = uss[i].signalindex;
		name = uss[i].signalname;
		if (namesame(name, x_("vdd")) == 0 || namesame(name, x_("gnd")) == 0)
			sim_irsimnets[index].flags = 0; else
				sim_irsimnets[index].flags = IRSIMNETVALID;
		for(j=i+1; j<sim_irsimnetnumber; j++)
		{
			nextindex = uss[j].signalindex;
			if (namesame(sim_irsimnets[index].name, sim_irsimnets[nextindex].name) != 0) break;
			sim_irsimnets[nextindex].flags = 0;
		}
		i = j - 1;
	}
	efree((CHAR *)uss);
}

/*
 * Routine to recursively examine cell "cell" with transformation matrix "trans"
 * and gather transistor information.
 */
INTBIG sim_irsimrecurse(NODEPROTO *cell, XARRAY trans, float m)
{
	REGISTER NETWORK *net, *gate, *source, *drain;
	REGISTER PORTPROTO *pp, *s, *g1, *g2, *d, *cpp;
	REGISTER NODEPROTO *np, *cnp;
	REGISTER NODEINST *ni;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER ARCINST *ai;
	REGISTER VARIABLE *var, *varm;
	REGISTER INTBIG i, backannotate, type, realtype, firsttransistor, nodewidth, nindex, sigcount;
	INTBIG x, y;
	REGISTER IRSIMTRANSISTOR *it, *previt;
	XARRAY singletrans, localtrans, subrot;
	float mi;
	CHAR internalname[50], **nodenames, *onenodename[1];
	REGISTER void *infstr;

	backannotate = 0;
	if (asktool(net_tool, x_("name-nodes"), (INTBIG)cell) != 0) backannotate++;

	/* create unique numbers for internal nets */
	for(net = cell->firstnetwork; net != NONETWORK; net = net->nextnetwork)
	{
		if (net->buswidth > 1)  continue;
		if (net->temp1 != -1)
		{
#if SIMFSDBWRITE != 0
			if (net->namecount > 0 && net->globalnet < 0 && net->temp1 >= sim_irsimtopnetnumber)
				sim_irsimnewalias(net->temp1, sim_irsimbuildnetname(networkname(net, 0)));
#endif
			continue;
		}
		if (net->globalnet >= 0) net->temp1 = sim_irsimgetglobal(net); else
		{
			if (net->namecount > 0)
			{
				net->temp1 = sim_irsimnewnetnumber(sim_irsimbuildnetname(networkname(net, 0)));
			} else
			{
				esnprintf(internalname, 50, x_("INTERNAL%ld"), sim_irsiminternal++);
				net->temp1 = sim_irsimnewnetnumber(internalname);
			}
		}
	}

	/* now examine all geometry in the cell */
	sim_irsimcelllisthead = NOIRSIMTRANSISTOR;
	firsttransistor = 0;
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		np = ni->proto;
		makerot(ni, singletrans);
		transmult(singletrans, trans, localtrans);
		mi = m;
		varm = getvalkey((INTBIG)ni, VNODEINST, -1, el_attrkey_M);
		if (varm != NOVARIABLE)
		{
			CHAR *pt = describevariable(varm, -1, -1);
			if (isanumber(pt))
				mi = m * (float)atof(pt);
			else ttyputmsg("Attribute \"m\" is not a number: %s\n", pt);
		}
		if (np->primindex == 0)
		{
			/* ignore recursive references (showing icon in contents) */
			if (isiconof(np, cell)) continue;

			/* cell instance: recurse */
			cnp = contentsview(np);
			if (cnp == NONODEPROTO) cnp = np;

			if (asktool(net_tool, x_("name-nets"), (INTBIG)cnp) != 0) backannotate++;

			/* see if the node is arrayed */
			nodewidth = ni->arraysize;
			if (nodewidth < 1) nodewidth = 1;

			/* if the node is arrayed, get the names of each instantiation */
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
			if (nodewidth > 1)
			{
				if (var == NOVARIABLE) nodewidth = 1; else
				{
					sigcount = net_evalbusname(APBUS, (CHAR *)var->addr, &nodenames,
						NOARCINST, NONODEPROTO, 0);
					if (sigcount != nodewidth) nodewidth = 1;
				}
			}
			if (nodewidth == 1)
			{
				if (var == NOVARIABLE) onenodename[0] = x_(""); else
					onenodename[0] = (CHAR *)var->addr;
				nodenames = onenodename;
			}
			for(nindex=0; nindex<nodewidth; nindex++)
			{
				for(net = cnp->firstnetwork; net != NONETWORK; net = net->nextnetwork)
					net->temp1 = -1;
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
								cpp->network->temp1 = net->networklist[nindex]->temp1;
							} else
							{
								for(i=0; i<cpp->network->buswidth; i++)
									cpp->network->networklist[i]->temp1 =
										net->networklist[i + nindex*cpp->network->buswidth]->temp1;
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
							cpp->network->temp1 = net->temp1;
							for(i=0; i<sigcount; i++)
								cpp->network->networklist[i]->temp1 = net->networklist[i]->temp1;
						}
					} else
					{
						cpp->network->temp1 = net->temp1;
					}
				}

				for(pp = cnp->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				{
					if (pp->network->temp1 != -1) continue;
					if (pp->network->buswidth > 1)
					{
						for(i=0; i<pp->network->buswidth; i++)
						{
							net = pp->network->networklist[i];
							if (net->temp1 != -1) continue;
							if (net->globalnet >= 0) net->temp1 = sim_irsimgetglobal(net); else
							{
								if (net->namecount <= 0) continue;
								infstr = initinfstr();
								formatinfstr(infstr, x_("%s/%s"), nodenames[nindex], networkname(net, 0));
								net->temp1 = sim_irsimnewnetnumber(sim_irsimbuildnetname(returninfstr(infstr)));
							}
						}
					} else
					{
						net = pp->network;
						if (net->globalnet >= 0) net->temp1 = sim_irsimgetglobal(net); else
						{
							infstr = initinfstr();
							formatinfstr(infstr, x_("%s/"), nodenames[nindex]);
							if (net->namecount > 0) addstringtoinfstr(infstr, networkname(net, 0)); else
								addstringtoinfstr(infstr, pp->protoname);
							net->temp1 = sim_irsimnewnetnumber(sim_irsimbuildnetname(returninfstr(infstr)));
						}
					}
				}
				maketrans(ni, singletrans);
				transmult(singletrans, localtrans, subrot);
				downhierarchy(ni, cnp, nindex);
				if (sim_irsimrecurse(cnp, subrot, mi) != 0) backannotate = 1;
				uphierarchy();
			}
		} else
		{
			/* primitive: see if it is a transistor */
			it = NOIRSIMTRANSISTOR;
			type = (np->userbits&NFUNCTION) >> NFUNCTIONSH;
			realtype = nodefunction(ni);
			if (realtype == NPTRANMOS || realtype == NPTRA4NMOS ||
				realtype == NPTRAPMOS || realtype == NPTRA4PMOS)
			{
				if (type == NPTRANS || type == NPTRANS4)
				{
					/* gate is port 0, source is port 1, drain is port 2 */
					g1 = g2 = np->firstportproto;
					s = g1->nextportproto;
					d = s->nextportproto;
				} else
				{
					/* gate is port 0 or 2, source is port 1, drain is port 3 */
					g1 = np->firstportproto;
					s = g1->nextportproto;
					g2 = s->nextportproto;
					d = g2->nextportproto;
					if (realtype == NPTRA4NMOS || realtype == NPTRA4PMOS) g2 = g1;
				}
				gate = source = drain = NONETWORK;
				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				{
					if (pi->proto == g1 || pi->proto == g2) gate = pi->conarcinst->network;
					if (pi->proto == s) source = pi->conarcinst->network;
					if (pi->proto == d) drain = pi->conarcinst->network;
				}
				for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
				{
					if (pe->proto == g1 || pe->proto == g2) gate = pe->exportproto->network;
					if (pe->proto == s) source = pe->exportproto->network;
					if (pe->proto == d) drain = pe->exportproto->network;
				}
				if (gate == NONETWORK || source == NONETWORK || drain == NONETWORK)
				{
					ttyputerr(_("Warning: ignoring transistor %s in cell %s (not fully connected)"),
						describenodeinst(ni), describenodeproto(cell));
				} else
				{
					it = sim_irsimalloctransistor();
					if (it == NOIRSIMTRANSISTOR) continue;
					previt = (IRSIMTRANSISTOR *)ni->temp1;
					it->transistortype = realtype;
					it->source = source->temp1;
					it->drain = drain->temp1;
					it->gate = gate->temp1;
					xform((ni->lowx+ni->highx)/2, (ni->lowy+ni->highy)/2,
						&it->xpos, &it->ypos, localtrans);
					if (previt != 0 && (sim_irsim_state&IRSIMPARASITICS) == IRSIMPARALOCAL)
					{
						it->length = previt->length;
						it->width = previt->width;
						it->sarea = previt->sarea;
						it->darea = previt->darea;
						it->sperimeter = previt->sperimeter;
						it->dperimeter = previt->dperimeter;
						it->m = mi;
					} else
					{
						portposition(ni, s, &x, &y);
						xform(x, y, &it->sourcex, &it->sourcey, trans);
						portposition(ni, d, &x, &y);
						xform(x, y, &it->drainx, &it->drainy, trans);
						transistorsize(ni, &it->length, &it->width);
						if (it->length < 0) it->length = 0;
						if (it->width < 0) it->width = 0;
						it->sarea = it->darea = 0.0;
						it->sperimeter = it->dperimeter = 0;
						it->m = mi;
						firsttransistor++;
						ni->temp1 = (INTBIG)it;
					}

					/* insert in list of all transistors */
					it->nextirsimtransistor = sim_firstirsimtransistor;
					sim_firstirsimtransistor = it;

					/* insert in list of transistors in this cell */
					it->nextcellirsimtransistor = sim_irsimcelllisthead;
					sim_irsimcelllisthead = it;
				}
			}

			if ((sim_irsim_state&IRSIMPARASITICS) == IRSIMPARAFULL)
				sim_irsimaddnodetomerge(ni, localtrans);
		}
	}
	if ((sim_irsim_state&IRSIMPARASITICS) == IRSIMPARAFULL)
	{
		for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			sim_irsimaddarctomerge(ai, trans);
	}

	/* do quick (cell-level) parasitics if requested */
	if ((sim_irsim_state&IRSIMPARASITICS) == IRSIMPARALOCAL && firsttransistor != 0)
	{
		sim_irsimmerge = mergenew(sim_tool->cluster);
		for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			makerot(ni, singletrans);
			transmult(singletrans, trans, localtrans);
			sim_irsimaddnodetomerge(ni, localtrans);
		}
		for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			sim_irsimaddarctomerge(ai, trans);
		mergeextract(sim_irsimmerge, sim_irsimgetallgeometry);
		mergedelete(sim_irsimmerge);
	}
	return(backannotate);
}

/*
 * Routine to add the diffusion on arc "ai" (with transformation "trans") to the
 * merged geometry information.
 */
void sim_irsimaddarctomerge(ARCINST *ai, XARRAY trans)
{
	REGISTER INTBIG i, tot, fun;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, sim_tool->cluster);

	tot = arcpolys(ai, NOWINDOWPART);
	for(i=0; i<tot; i++)
	{
		shapearcpoly(ai, i, poly);
		fun = layerfunction(poly->tech, poly->layer);
		if ((fun&LFTYPE) != LFDIFF) continue;
		xformpoly(poly, trans);
		if (sim_irsimclipagainstgates(poly, ai, trans)) continue;
		mergeaddpolygon(sim_irsimmerge, ai->network->temp1, poly->tech, poly);
	}
}

/*
 * Routine to add the diffusion on node "ni" (with transformation "trans") to the
 * merged geometry information.
 */
void sim_irsimaddnodetomerge(NODEINST *ni, XARRAY trans)
{
	REGISTER INTBIG i, tot, fun, netnumber;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, sim_tool->cluster);

	tot = nodeEpolys(ni, 0, NOWINDOWPART);
	for(i=0; i<tot; i++)
	{
		shapeEnodepoly(ni, i, poly);
		if (poly->portproto == NOPORTPROTO) continue;
		fun = layerfunction(poly->tech, poly->layer);
		if ((fun&LFTYPE) != LFDIFF) continue;
		netnumber = -1;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			if (pi->proto != poly->portproto) continue;
			netnumber = pi->conarcinst->network->temp1;
			break;
		}
		if (netnumber < 0)
		{
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
			{
				if (pe->proto != poly->portproto) continue;
				netnumber = pe->exportproto->network->temp1;
				break;
			}
		}
		if (netnumber < 0)
			netnumber = sim_irsimnewnetnumber(sim_irsimbuildnetname(x_("unattached")));
		xformpoly(poly, trans);
		mergeaddpolygon(sim_irsimmerge, netnumber, poly->tech, poly);
	}
}

/*
 * Routine to clip polygon "poly" on arc "ai" given a transformation for this cell
 * in "trans".
 * Returns true if the polygon is clipped to oblivion.
 */
BOOLEAN sim_irsimclipagainstgates(POLYGON *poly, ARCINST *ai, XARRAY trans)
{
	REGISTER INTBIG e, i, tot, lfun;
	REGISTER BOOLEAN ret;
	INTBIG plx, phx, ply, phy, lx, hx, ly, hy;
	XARRAY singletrans, localtrans;
	REGISTER NODEINST *ni;
	static POLYGON *gatepoly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&gatepoly, 4, sim_tool->cluster);

	if (!isbox(poly, &plx, &phx, &ply, &phy)) return(FALSE);

	/* clip this active against gate of connecting transistors */
	for(e=0; e<2; e++)
	{
		ni = ai->end[e].nodeinst;
		if (!isfet(ni->geom)) continue;
		makerot(ni, singletrans);
		transmult(singletrans, trans, localtrans);
		tot = nodepolys(ni, 0, 0);
		for(i=0; i<tot; i++)
		{
			shapenodepoly(ni, i, gatepoly);
			lfun = layerfunction(gatepoly->tech, gatepoly->layer);
			if (!layerispoly(lfun)) continue;

			/* limitation: connecting transistors must be manhattan */
			xformpoly(gatepoly, localtrans);
			if (isbox(gatepoly, &lx, &hx, &ly, &hy))
			{
				ret = sim_irsimcropbox(&plx, &phx, &ply, &phy,
					lx, hx, ly, hy);
				if (ret) return(TRUE);
			}
		}
	}
	makerectpoly(plx, phx, ply, phy, poly);
	return(FALSE);
}

/*
 * routine to crop the box in the reference parameters (lx-hx, ly-hy)
 * against the box in (bx-ux, by-uy).  If the box is cropped into oblivion,
 * the routine returns true.  Otherwise the box is cropped and false is returned
 */
BOOLEAN sim_irsimcropbox(INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy, INTBIG bx, INTBIG ux, INTBIG by,
	INTBIG uy)
{
	/* if the two boxes don't touch, just return */
	if (bx >= *hx || by >= *hy || ux <= *lx || uy <= *ly) return(FALSE);

	/* if the box to be cropped is within the other, mark it all covered */
	if (bx <= *lx && ux >= *hx && by <= *ly && uy >= *hy) return(TRUE);

	/* reduce (lx-hx,ly-hy) by (bx-ux,by-uy) */
	if (bx <= *lx && ux >= *hx)
	{
		/* it covers in X...crop in Y */
		if (*hy < uy && *hy > by) *hy = by;
		if (*ly < uy && *ly > by) *ly = uy;
	}
	if (by <= *ly && uy >= *hy)
	{
		/* it covers in Y...crop in X */
		if (*hx < ux && *hx > bx) *hx = bx;
		if (*lx < ux && *lx > bx) *lx = ux;
	}
	return(FALSE);
}

/*
 * Coroutine for polygon merging that is called for every complex polygon of Diffusion found
 * in the circuit (either in the cell or the entire flattened circuit, depending on the user
 * preferences).  "layer" is always the Diffusion layer and "tech" is the current technology.
 * The polygon has "count" points in "(x, y)".
 */
void sim_irsimgetallgeometry(INTBIG layer, TECHNOLOGY *tech, INTBIG *x, INTBIG *y, INTBIG count)
{
	REGISTER IRSIMTRANSISTOR *it, *firstsource, *firstdrain, *firstit, *nextit;
	POLYGON fakepoly;
	REGISTER INTBIG found, lx, hx, ly, hy, i;
	INTBIG perimeter;
	float area;

	Q_UNUSED( layer );
	Q_UNUSED( tech );
	/* get bounds of this geometry */
	lx = hx = x[0];
	ly = hy = y[0];
	for(i=1; i<count; i++)
	{
		if (x[i] < lx) lx = x[i];
		if (x[i] > hx) hx = x[i];
		if (y[i] < ly) ly = y[i];
		if (y[i] > hy) hy = y[i];
	}
	found = 0;
	firstsource = firstdrain = NOIRSIMTRANSISTOR;

	/* determine the first transistor in the list */
	firstit = sim_irsimcelllisthead;
	if (firstit == NOIRSIMTRANSISTOR) firstit = sim_firstirsimtransistor;

	/* loop through the list */
	for(it = firstit; it != NOIRSIMTRANSISTOR; it = nextit)
	{
		/* determine the next transistor in the list */
		if (sim_irsimcelllisthead == NOIRSIMTRANSISTOR)
			nextit = it->nextirsimtransistor; else
				nextit = it->nextcellirsimtransistor;

		/* see if the source or drain of this transistor is in the polygon */
		if (it->sarea == 0.0)
		{
			if (it->sourcex >= lx && it->sourcex <= hx &&
				it->sourcey >= ly && it->sourcey <= hy)
			{
				fakepoly.count = count;
				fakepoly.xv = x;
				fakepoly.yv = y;
				fakepoly.style = FILLED;
				if (isinside(it->sourcex, it->sourcey, &fakepoly))
				{
					it->nextsourcelist = firstsource;
					firstsource = it;
					found++;
				}
			}
		}
		if (it->darea == 0.0)
		{
			if (it->drainx >= lx && it->drainx <= hx &&
				it->drainy >= ly && it->drainy <= hy)
			{
				fakepoly.count = count;
				fakepoly.xv = x;
				fakepoly.yv = y;
				fakepoly.style = FILLED;
				if (isinside(it->drainx, it->drainy, &fakepoly))
				{
					it->nextdrainlist = firstdrain;
					firstdrain = it;
					found++;
				}
			}
		}
	}
	if (found != 0)
	{
		sim_irsimgetareaperimeter(x, y, count, &area, &perimeter);
		area /= found;   perimeter /= found;
		for(it = firstdrain; it != NOIRSIMTRANSISTOR; it = it->nextdrainlist)
		{
			it->darea = area;
			it->dperimeter = perimeter;
		}
		for(it = firstsource; it != NOIRSIMTRANSISTOR; it = it->nextsourcelist)
		{
			it->sarea = area;
			it->sperimeter = perimeter;
		}
	}
}

/*
 * Routine to determine the area and perimeter of "count" points in (x, y).  The area is
 * put in "a" and the perimeter in "p".
 */
void sim_irsimgetareaperimeter(INTBIG *x, INTBIG *y, INTBIG count, float *a, INTBIG *p)
{
	REGISTER INTBIG per, seglen, i, lastx, lasty;
	float area;

	/* compute the perimeter */
	per = 0;
	for(i=0; i<count; i++)
	{
		if (i == 0)
		{
			lastx = x[count-1];   lasty = y[count-1];
		} else
		{
			lastx = x[i-1];   lasty = y[i-1];
		}
		seglen = computedistance(lastx, lasty, x[i], y[i]);
		per += seglen;
	}
	*p = per;

	/* compute the area */
	area = areapoints(count, x, y);
	*a = (float)fabs(area);
}

/*
 * Routine to construct the IRSIM name of a node, given that its name is "name" and that
 * it is down in the hierarchy (use "gettraversalpath" to determine its location).
 */
CHAR *sim_irsimbuildnetname(CHAR *name)
{
	REGISTER INTBIG i, nodewidth, sigcount;
	NODEINST **nilist, *ni;
	CHAR **nodenames;
	INTBIG depth, *indexlist;
	REGISTER VARIABLE *var;
	REGISTER void *infstr;

	gettraversalpath(el_curlib->curnodeproto, el_curwindowpart, &nilist, &indexlist, &depth, 0);
	infstr = initinfstr();
	for(i=0; i<depth; i++)
	{
		ni = nilist[i];
		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
		nodewidth = ni->arraysize;
		if (nodewidth < 1) nodewidth = 1;
		if (nodewidth > 1)
		{
			if (var == NOVARIABLE) nodewidth = 1; else
			{
				sigcount = net_evalbusname(APBUS, (CHAR *)var->addr, &nodenames,
					NOARCINST, NONODEPROTO, 0);
				if (sigcount != nodewidth) nodewidth = 1;
			}
		}

		if (var != NOVARIABLE)
		{
			if (nodewidth > 1) addstringtoinfstr(infstr, nodenames[indexlist[i]]); else
				addstringtoinfstr(infstr, (CHAR *)var->addr);
		} else
		{
			addstringtoinfstr(infstr, ni->proto->protoname);
		}
		addtoinfstr(infstr, '/');
	}
	addstringtoinfstr(infstr, name);
	return(returninfstr(infstr));
}

/*
 * Routine to allocate (or reuse) an IRSIM transistor structure and return it.
 */
IRSIMTRANSISTOR *sim_irsimalloctransistor(void)
{
	REGISTER IRSIMTRANSISTOR *it;

	if (sim_irsimtransistorfree != NOIRSIMTRANSISTOR)
	{
		it = sim_irsimtransistorfree;
		sim_irsimtransistorfree = it->nextirsimtransistor;
	} else
	{
		it = (IRSIMTRANSISTOR *)emalloc(sizeof (IRSIMTRANSISTOR), sim_tool->cluster);
		if (it == 0) return(NOIRSIMTRANSISTOR);
	}
	return(it);
}

/*
 * Routine to free IRSIM transistor structure "it".
 */
void sim_irsimfreetransistor(IRSIMTRANSISTOR *it)
{
	it->nextirsimtransistor = sim_irsimtransistorfree;
	sim_irsimtransistorfree = it;
}

/*
 * Routine to clear out the IRSIM network in memory (from the last cell that was
 * simulated).
 */
void sim_irsimfreenetworkinfo(void)
{
	REGISTER IRSIMTRANSISTOR *it, *nextit;
	REGISTER INTBIG i;

	for(it = sim_firstirsimtransistor; it != NOIRSIMTRANSISTOR; it = nextit)
	{
		nextit = it->nextirsimtransistor;
		sim_irsimfreetransistor(it);
	}
	for(i=0; i<sim_irsimnetnumber; i++) efree(sim_irsimnets[i].name);
}

/*
 * Routine to create a new IRSIM network object with name "name" and return its number.
 */
INTBIG sim_irsimnewnetnumber(CHAR *name)
{
	REGISTER INTBIG newtotal, i;
	IRSIMNETWORK *newnets;

	if (sim_irsimnetnumber >= sim_irsimnettotal)
	{
		newtotal = sim_irsimnetnumber * 2;
		if (newtotal <= 0) newtotal = 10;
		newnets = (IRSIMNETWORK *)emalloc(newtotal * (sizeof (IRSIMNETWORK)), sim_tool->cluster);
		if (newnets == 0) return(-1);
		for(i=0; i<sim_irsimnetnumber; i++)
		{
			newnets[i].name = sim_irsimnets[i].name;
			newnets[i].signal = sim_irsimnets[i].signal;
			newnets[i].flags = sim_irsimnets[i].flags;
		}
		if (sim_irsimnettotal > 0) efree((CHAR *)sim_irsimnets);
		sim_irsimnets = newnets;
		sim_irsimnettotal = newtotal;
	}
	i = sim_irsimnetnumber++;
	(void)allocstring(&sim_irsimnets[i].name, name, sim_tool->cluster);
#if SIMFSDBWRITE != 0
	sim_irsimnewalias(i, name);
#endif
	return(i);
}

#if SIMFSDBWRITE != 0
/*
 * Routine to create an alias to IRSIM network object netnum
 */
void sim_irsimnewalias(INTBIG netnum, CHAR *alias)
{
	REGISTER INTBIG newtotal, i;
	IRSIMALIAS *newaliases;

	if (sim_irsimaliasnumber >= sim_irsimaliastotal)
	{
		newtotal = sim_irsimaliasnumber * 2;
		if (newtotal <= 0) newtotal = 10;
		newaliases = (IRSIMALIAS *)emalloc(newtotal * (sizeof (IRSIMALIAS)), sim_tool->cluster);
		if (newaliases == 0) return;
		for(i=0; i<sim_irsimaliasnumber; i++)
		{
			newaliases[i].name = sim_irsimaliases[i].name;
			newaliases[i].alias = sim_irsimaliases[i].alias;
		}
		if (sim_irsimaliastotal > 0) efree((CHAR *)sim_irsimaliases);
		sim_irsimaliases = newaliases;
		sim_irsimaliastotal = newtotal;
	}
	i = sim_irsimaliasnumber++;
	sim_irsimaliases[i].name = sim_irsimnets[netnum].name;
	(void)allocstring(&sim_irsimaliases[i].alias, alias, sim_tool->cluster);
}
#endif

INTBIG sim_irsimgetglobal(NETWORK *net)
{
	REGISTER INTBIG i;
	REGISTER CHAR *name;

	if (net->globalnet < 0 || net->globalnet >= net->parent->globalnetcount) return(0);
	if (net->globalnet == GLOBALNETPOWER) name = x_("vdd"); else
		if (net->globalnet == GLOBALNETGROUND) name = x_("gnd"); else
			name = describenetwork(net);
	for(i=0; i<sim_irsimglobalnetcount; i++)
		if (namesame(name, sim_irsimglobalnets[i]) == 0) break;
	if (i >= sim_irsimglobalnetcount) return(0);
	return(sim_irsimglobalnetsnumbers[i]);
}

/*
 * Routine to recursively examine cells and gather global network names.
 */
void sim_irsimgatherglobals(NODEPROTO *np)
{
	REGISTER INTBIG i, newtotal, *newnumbers;
	NETWORK *net;
	REGISTER CHAR *gname, **newlist;
	REGISTER NODEPROTO *onp, *cnp;
	REGISTER NODEINST *ni;

	if (np->temp1 != 0) return;
	np->temp1 = 1;
	for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
	{
		if (net->globalnet < 0) continue;

		/* global net found: see if it is already in the list */
		if (net->globalnet == GLOBALNETPOWER) gname = x_("vdd"); else
			if (net->globalnet == GLOBALNETGROUND) gname = x_("gnd"); else
				gname = describenetwork(net);
		for(i=0; i<sim_irsimglobalnetcount; i++)
			if (namesame(gname, sim_irsimglobalnets[i]) == 0) break;
		if (i < sim_irsimglobalnetcount) continue;

		/* add the global net name */
		if (sim_irsimglobalnetcount >= sim_irsimglobalnettotal)
		{
			newtotal = sim_irsimglobalnettotal * 2;
			if (sim_irsimglobalnetcount >= newtotal)
				newtotal = sim_irsimglobalnetcount + 5;
			newlist = (CHAR **)emalloc(newtotal * (sizeof (CHAR *)), sim_tool->cluster);
			if (newlist == 0) return;
			newnumbers = (INTBIG *)emalloc(newtotal * SIZEOFINTBIG, sim_tool->cluster);
			if (newnumbers == 0) return;

			for(i=0; i<sim_irsimglobalnettotal; i++)
			{
				newlist[i] = sim_irsimglobalnets[i];
				newnumbers[i] = sim_irsimglobalnetsnumbers[i];
			}
			for(i=sim_irsimglobalnettotal; i<newtotal; i++)
				newlist[i] = 0;
			if (sim_irsimglobalnettotal > 0)
			{
				efree((CHAR *)sim_irsimglobalnets);
				efree((CHAR *)sim_irsimglobalnetsnumbers);
			}
			sim_irsimglobalnets = newlist;
			sim_irsimglobalnetsnumbers = newnumbers;
			sim_irsimglobalnettotal = newtotal;
		}
		if (sim_irsimglobalnets[sim_irsimglobalnetcount] != 0)
			efree((CHAR *)sim_irsimglobalnets[sim_irsimglobalnetcount]);
		(void)allocstring(&sim_irsimglobalnets[sim_irsimglobalnetcount], gname,
			sim_tool->cluster);
		sim_irsimglobalnetsnumbers[sim_irsimglobalnetcount] = 0;
		sim_irsimglobalnetcount++;
	}

	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		onp = ni->proto;
		if (onp->primindex != 0) continue;

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(onp, np)) continue;

		if (onp->cellview == el_iconview)
		{
			cnp = contentsview(onp);
			if (cnp != NONODEPROTO) onp = cnp;
		}
		sim_irsimgatherglobals(onp);
	}
}

#endif  /* SIMTOOL - at top */
