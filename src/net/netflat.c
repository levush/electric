/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: netflat.c
 * Network tool: module for fully instantiating a hierarchical network
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
#include "network.h"
#include "efunction.h"
#include "tecschem.h"

#define IGNOREFOURPORT 1				/* comment out to handle 4-port transistors properly */

static INTBIG    net_pseudonode;		/* net number for pseudonetworks */
static BOOLEAN   net_mfactorwarned;		/* TRUE if a warning was issued about zero m-factors */
static NODEINST *net_toplevelinst;		/* actual top-instance associated with this node */
static PCOMP    *net_seriesend1, *net_seriesend2;	/* end component of series transistor chains */
static INTBIG    net_seriescon1,  net_seriescon2;	/* end index of series transistor chains */

/* working memory for "net_comparewirelist()" */
static INTBIG     net_comparelistsize = 0;
static BOOLEAN   *net_comparelist;

/* working memory for "net_mergeseries()" */
static PCOMP    **net_serieslist;
static INTBIG     net_serieslistcount;
static INTBIG     net_serieslisttotal = 0;

#define STATIC

/* prototypes for local routines */
STATIC BOOLEAN net_addtoserieslist(PCOMP *pc);
STATIC void    net_addtraversalpath(PNET *pn, NODEPROTO *cell);
STATIC PCOMP  *net_allocpcomp(void);
STATIC PNET   *net_allocpnet(void);
STATIC PCOMP  *net_buildpseudo(NODEPROTO*, PCOMP*, INTBIG*, PNET**, PNET**, BOOLEAN, BOOLEAN, float);
STATIC void    net_crawlforseries(PCOMP *pc);
STATIC INTBIG  net_findotherexporttopology(NODEPROTO *np, CHAR *exportname);
STATIC INTBIG  net_findotherexportcharacteristic(NODEPROTO *parent, INTBIG bits);
STATIC PCOMP **net_gatherseries(PCOMP *pc, INTBIG *seriescount);
STATIC INTBIG  net_getfunction(NODEINST*);
STATIC float   net_getpartvalue(NODEINST *ni);
STATIC float   net_findunits(NODEINST *ni, INTBIG units);
STATIC BOOLEAN net_getpnetandstate(NODEINST *ni, PORTPROTO *pp, NETWORK *forcenet,
				INTBIG index, PNET **netnumber, INTSML *state, PNET **pnetlist,
				PNET **globalpnetlist, INTBIG nodewidth, INTBIG nindex, INTBIG destsignals);
STATIC INTBIG  net_mergeseries(PCOMP **pcomp, PNET *pnet, INTBIG *components);
STATIC PNET   *net_newpnet(NETWORK *net, PNET **pnetlist, PNET **globalpnetlist);
STATIC void    net_setthisexporttopology(PORTPROTO *pp, INTBIG *index);
STATIC int     net_sortpcompbyhash(const void *e1, const void *e2);
STATIC void    net_addexporttopnet(PNET *pn, PORTPROTO *pp);
STATIC void    net_gatherglobals(NODEPROTO *cell, PNET **pnetlist, PNET **globalpnetlist);
STATIC int     net_cellnameascending(const void *e1, const void *e2);
STATIC INTBIG  net_getcellfunction(NODEPROTO *np);

/*
 * Routine to free all memory associated with this module.
 */
void net_freeflatmemory(void)
{
	REGISTER PCOMP *p;
	REGISTER PNET *pn;

	while (net_pcompfree != NOPCOMP)
	{
		p = net_pcompfree;
		net_pcompfree = net_pcompfree->nextpcomp;
		efree((CHAR *)p);
	}
	while (net_pnetfree != NOPNET)
	{
		pn = net_pnetfree;
		net_pnetfree = net_pnetfree->nextpnet;
		if (pn->nodetotal > 0)
		{
			efree((CHAR *)pn->nodelist);
			efree((CHAR *)pn->nodewire);
		}
		efree((CHAR *)pn);
	}
	if (net_comparelistsize > 0) efree((CHAR *)net_comparelist);
}

/*********************** PSEUDO-NETWORK CONSTRUCTION ***********************/

/*
 * The usage of this module is:
 *   #include "network.h"
 *   PCOMP *pcomp;
 *   PNET *pnet;
 *   INTBIG components, nets, powernets, groundnets;
 *
 *   net_initnetflattening();
 *   pcomp = net_makepseudo(cell, &components, &nets, &powernets, &groundnets,
 *	    &pnet, hierarchical, mergeparallel, mergeseries, checkcelloverrides, figuresizes);
 *   .....
 *        do something with the network in "pcomp" and "pnet"
 *   .....
 *   net_freeallpnet(pnet);
 *   net_freeallpcomp(pcomp);
 *
 * "net_initnetflattening" gathers network topology for primitives and cells.  It needs to be
 * called only once for any set of libraries.
 * "net_makepseudo" builds a pseudonetwork structure that represents the
 * network in cell "cell".  If it returns NOPCOMP and sets "components" negative,
 * there is an error (if it returns NOPCOMP with "components" zero, there just are not
 * any components in the cell).
 * A linked list of PCOMP objects is returned, one for every component
 * in the pseudonetwork.  A linked list of PNET objects is also returned,
 * one for every network in the pseudonetwork.  Finally, the number of
 * components, networks, power networks, and ground networks is returned
 * in the reference parameters "components", "nets", "powernets", and
 * "groundnets".
 *
 * A number of switches controls the flattening:
 * If "hierarchical"        is true, the network will be fully instantiated.
 * If "mergeparallel"       is true, parallel components are merged into one.
 * If "mergeseries"         is true, series transistors are merged into one.
 * If "checkcelloverrides"  is true, individual cells may override "mergeparallel".
 * If "figuresizes"         is true, extract size information (takes time if programmed).
 */
PCOMP *net_makepseudo(NODEPROTO *cell, INTBIG *components, INTBIG *nets, INTBIG *powernets,
	INTBIG *groundnets, PNET **pnetlist, BOOLEAN hierarchical, BOOLEAN mergeparallel,
	BOOLEAN mergeseries, BOOLEAN checkcelloverrides, BOOLEAN figuresizes)
{
	PCOMP *pcomplist;
	PNET *globalpnetlist;
	REGISTER BOOLEAN localmergeparallel, localmergeseries, killnet;
	REGISTER INTBIG i, mergecount;
	REGISTER NETWORK *net, *subnet;
	REGISTER PNET *pn, *lastpnet, *nextpnet;
	REGISTER VARIABLE *var;
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp, *opp;

	/* cache NCC options */
	var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_ncc_optionskey);
	if (var == NOVARIABLE) net_ncc_options = 0; else
		net_ncc_options = var->addr;

	/* see if the current cell overrides the options */
	localmergeparallel = mergeparallel;
	localmergeseries = mergeseries;
	if (checkcelloverrides)
	{
		var = getvalkey((INTBIG)cell, VNODEPROTO, VINTEGER, net_ncc_optionskey);
		if (var != NOVARIABLE)
		{
			if ((var->addr&NCCNOMERGEPARALLELOVER) != 0)
			{
				if ((var->addr&NCCNOMERGEPARALLEL) == 0) localmergeparallel = TRUE; else
					localmergeparallel = FALSE;
			}
			if ((var->addr&NCCMERGESERIESOVER) != 0)
			{
				if ((var->addr&NCCMERGESERIES) != 0) localmergeseries = TRUE; else
					localmergeseries = FALSE;
			}
		}
	}

	/* first create net numbers inside of this cell */
	net_pseudonode = 0;
	*pnetlist = NOPNET;
	globalpnetlist = NOPNET;
	for(net = cell->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		net->temp1 = (INTBIG)NOPNET;
	for(pp = cell->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		net = pp->network;
		if (net->temp1 != (INTBIG)NOPNET) continue;
		pn = net_newpnet(net, pnetlist, &globalpnetlist);
		if (pn == NOPNET) { *components = -1;   return(NOPCOMP); }
		if ((pp->userbits&STATEBITS) == PWRPORT) pn->flags |= POWERNET; else
			if ((pp->userbits&STATEBITS) == GNDPORT) pn->flags |= GROUNDNET;
		pn->flags |= EXPORTEDNET;
		net->temp1 = (INTBIG)pn;
		net_addexporttopnet(pn, pp);
		for(opp = pp->nextportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
		{
			if (net != opp->network) continue;
			net_addexporttopnet(pn, opp);
		}
		if (net->buswidth > 1)
		{
			for(i=0; i<net->buswidth; i++)
			{
				subnet = net->networklist[i];
				pn = (PNET *)subnet->temp1;
				if (pn != NOPNET)
				{
					net_addexporttopnet(pn, pp);
					continue;
				}
				pn = net_newpnet(subnet, pnetlist, &globalpnetlist);
				pn->flags |= EXPORTEDNET;
				subnet->temp1 = (INTBIG)pn;
				pn->realportcount = 1;
				pn->realportlist = pp;
			}
		}
	}

	/* create a list of pseudocomponents in this cell */
	*components = 0;
	begintraversehierarchy();
	net_toplevelinst = NONODEINST;
	net_mfactorwarned = FALSE;
	pcomplist = net_buildpseudo(cell, NOPCOMP, components, pnetlist, &globalpnetlist,
		hierarchical, figuresizes, 1.0f);
	endtraversehierarchy();
	if (pcomplist == 0) { *components = -1;   return(NOPCOMP); }
	if (*components == 0)
		ttyputmsg(_("There are no components in cell %s"), describenodeproto(cell));

	/* append all global networks to the main list */
	while (globalpnetlist != NOPNET)
	{
		pn = globalpnetlist;
		globalpnetlist = pn->nextpnet;

		pn->nextpnet = *pnetlist;
		*pnetlist = pn;
	}

	/* reduce network by merging parallel components */
	if (localmergeparallel)
	{
		ttyputmsg(_("--- Merging parallel components in cell %s..."),
			describenodeproto(cell));
		mergecount = net_mergeparallel(&pcomplist, *pnetlist, components);
		if (mergecount < 0) { *components = -1;   return(NOPCOMP); }
		if (mergecount != 0)
			ttyputmsg(_("--- Merged %ld parallel components"), mergecount);
	}
	if (localmergeseries)
	{
		mergecount = net_mergeseries(&pcomplist, *pnetlist, components);
		if (mergecount != 0)
			ttyputmsg(_("--- Merged %ld series transistors in cell %s"),
				mergecount, describenodeproto(cell));
	}

	/* look to the bottom of the hierarchy and be sure all global signals are listed */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 &= ~NETGLOBALCHECKED;
	net_gatherglobals(cell, pnetlist, &globalpnetlist);

	/* count the power and ground nets */
	net_fillinnetpointers(pcomplist, *pnetlist);
	*powernets = *groundnets = *nets = 0;
	lastpnet = NOPNET;
	for(pn = *pnetlist; pn != NOPNET; pn = nextpnet)
	{
		nextpnet = pn->nextpnet;
		net = pn->network;
		killnet = FALSE;
		if (pn->nodecount <= 0 && (pn->flags&(POWERNET|GROUNDNET)) == 0)
		{
			if (net == NONETWORK) killnet = TRUE;
			if ((net_ncc_options & NCCINCLUDENOCOMPNETS) == 0 && net->buswidth <= 1)
				killnet = TRUE;
		}
		if (killnet)
		{
			if (lastpnet == NOPNET) *pnetlist = nextpnet; else
				lastpnet->nextpnet = nextpnet;
			net_freepnet(pn);
		} else
		{
			(*nets)++;
			if ((pn->flags&POWERNET) != 0) (*powernets)++;
			if ((pn->flags&GROUNDNET) != 0) (*groundnets)++;
			if ((pn->flags&(POWERNET|GROUNDNET)) == (POWERNET|GROUNDNET))
			{
				if (pn->network == NONETWORK) np = NONODEPROTO; else
					np = pn->network->parent;
				ttyputmsg(_("WARNING: cell %s, network '%s' shorts power and ground"),
					describenodeproto(np), describenetwork(pn->network));
			}
			lastpnet = pn;
		}
	}

	return(pcomplist);
}

/*
 * Routine to recursively examine the hierarchy and make sure that all globals are mentioned
 * at the top level.
 */
void net_gatherglobals(NODEPROTO *cell, PNET **pnetlist, PNET **globalpnetlist)
{
	REGISTER INTBIG i, index;
	REGISTER NETWORK *net;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *cnp, *subnp, *pnparent;
	REGISTER PNET *pn;

	if ((cell->temp1&NETGLOBALCHECKED) != 0) return;
	cell->temp1 |= NETGLOBALCHECKED;

	/* look at all globals in this cell */
	for(i=0; i<cell->globalnetcount; i++)
	{
		net = cell->globalnetworks[i];
		if (net == NONETWORK) continue;
		for(pn = *pnetlist; pn != NOPNET; pn = pn->nextpnet)
		{
			if (pn->network == NONETWORK) continue;
			index = pn->network->globalnet;
			if (index < 0) continue;
			pnparent = pn->network->parent;
			if (namesame(pnparent->globalnetnames[index], cell->globalnetnames[i]) == 0) break;
		}
		if (pn != NOPNET) continue;

		/* didn't find network "net" in the list */
		pn = net_newpnet(net, pnetlist, globalpnetlist);
		net->temp1 = (INTBIG)pn;
	}

	/* now recurse */
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		subnp = ni->proto;
		if (subnp->primindex != 0) continue;

		/* don't recurse into a schematic's own icon */
		if (isiconof(subnp, cell)) continue;

		/* switch from icon to schematic */
		cnp = contentsview(subnp);
		if (cnp != NONODEPROTO) subnp = cnp;
		net_gatherglobals(subnp, pnetlist, globalpnetlist);
	}
}

/*
 * routine to build a linked list of pseudocomponents in cell "cell".  The
 * list is appended to "initiallist" and returned.  The values of "power" and
 * "ground" are the PNETs of such components.  Routine increments the
 * integer at "components" for every component created.  If
 * "compare_hierarchically" is nonzero, net is flattened.  Returns zero on error.
 */
PCOMP *net_buildpseudo(NODEPROTO *cell, PCOMP *initiallist, INTBIG *components,
	PNET **pnetlist, PNET **globalpnetlist, BOOLEAN compare_hierarchically,
	BOOLEAN figuresizes, float mfactor)
{
	REGISTER PCOMP *pcomp;
	REGISTER PORTPROTO *pp, *opp, *realpp, *temprealpp;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER NETWORK *net, *subnet, *foundnet;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	NODEINST **nilist;
	float submfactor, thismfactor;
	REGISTER INTBIG fun, i, j, k, l, toplevel, flattenit, lambda,
		nodewidth, nindex, sigcount, stopcheck;
	BOOLEAN localcompare_hierarchically;
	REGISTER CHAR *pt;
	INTBIG width, length, pathcount, *indexlist;
	REGISTER NODEPROTO *realnp, *anp, *cnp;
	REGISTER PNET *pn;
	REGISTER VARIABLE *var;

	if (net_toplevelinst == NONODEINST) toplevel = 1; else
		toplevel = 0;

	/* make simple checks that port characteristics match the name */
	for(pp = cell->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		/* check busses for consistent component characteristics */
		if (pp->network->buswidth > 1)
		{
			for(i=0; i<pp->network->buswidth; i++)
			{
				subnet = pp->network->networklist[i];
				for(opp = cell->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
				{
					if (opp->network != subnet) continue;
					if ((opp->userbits&STATEBITS) != (pp->userbits&STATEBITS))
					{
						ttyputerr(_("Warning: bus export %s is %s but export %s is %s"),
							pp->protoname, describeportbits(pp->userbits), opp->protoname,
								describeportbits(opp->userbits));
					}
					break;
				}
			}
		}
	}

	/* spread power and ground information from appropriate nodes */
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* see if power or ground comes from this node */
		for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
			if ((pe->exportproto->userbits&STATEBITS) == PWRPORT ||
				(pe->exportproto->userbits&STATEBITS) == GNDPORT) break;
		if (pe == NOPORTEXPINST) continue;

		/* they do: get the network */
		pp = pe->proto;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			if (pi->proto == pp) break;
		if (pi == NOPORTARCINST) continue;
		net = pi->conarcinst->network;

		/* propagate power and ground */
		if ((pe->exportproto->userbits&STATEBITS) == PWRPORT)
		{
			pn = (PNET *)net->temp1;
			if (pn == NOPNET)
				pn = net_newpnet(net, pnetlist, globalpnetlist);
			pn->flags |= POWERNET;
		}
		if ((pe->exportproto->userbits&STATEBITS) == GNDPORT)
		{
			pn = (PNET *)net->temp1;
			if (pn == NOPNET)
				pn = net_newpnet(net, pnetlist, globalpnetlist);
			pn->flags |= GROUNDNET;
		}
	}

	/* generate new pseudo-netnumbers for networks not connected to ports */
	for(net = cell->firstnetwork; net != NONETWORK; net = net->nextnetwork)
	{
		if (net->temp1 != (INTBIG)NOPNET) continue;
		pn = net_newpnet(net, pnetlist, globalpnetlist);
		if (pn == NOPNET) return(0);
		net_addtraversalpath(pn, cell);
		net->temp1 = (INTBIG)pn;
		if (net->buswidth > 1)
		{
			for(i=0; i<net->buswidth; i++)
			{
				subnet = net->networklist[i];
				if (subnet->temp1 != (INTBIG)NOPNET) continue;
				pn = net_newpnet(subnet, pnetlist, globalpnetlist);
				net_addtraversalpath(pn, cell);
				subnet->temp1 = (INTBIG)pn;
			}
		}
	}

	/* add in forced matches on arcs */
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		var = getvalkey((INTBIG)ai, VARCINST, VSTRING, net_ncc_forcedassociationkey);
		if (var == NOVARIABLE) continue;
		pn = (PNET *)ai->network->temp1;
		for(pt = (CHAR *)var->addr; *pt != 0; pt++)
			pn->forcedassociation += (INTBIG)*pt;
	}

	/* search every component in the cell */
	stopcheck = 0;
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (((stopcheck++)%5) == 0)
		{
			if (stopping(STOPREASONNCC)) return(0);
		}

		anp = ni->proto;
		if (toplevel != 0) net_toplevelinst = ni;

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(anp, cell)) continue;

		/* if there is an alternate contents cell, use it */
		realnp = contentsview(anp);
		if (realnp == NONODEPROTO) realnp = anp;

		/* if flattening the circuit, explore contents of cell instances */
		flattenit = 0;
		localcompare_hierarchically = compare_hierarchically;
		var = getvalkey((INTBIG)realnp, VNODEPROTO, VINTEGER, net_ncc_optionskey);
		if (var != NOVARIABLE)
		{
			if ((var->addr&NCCHIERARCHICALOVER) != 0)
			{
				if ((var->addr&NCCHIERARCHICAL) != 0) localcompare_hierarchically = TRUE; else
					localcompare_hierarchically = FALSE;
			}
		}
		if (anp->primindex == 0 && localcompare_hierarchically) flattenit = 1;

		/* determine whether the node is arrayed */
		nodewidth = ni->arraysize;
		if (nodewidth < 1) nodewidth = 1;

		/* accumulate m factors */
		submfactor = mfactor;
		var = getvalkey((INTBIG)ni, VNODEINST, -1, el_attrkey_M);
		if (var != NOVARIABLE)
		{
			CHAR *pt = describevariable(var, -1, -1);
			if (isanumber(pt))
			{
				thismfactor = (float)atof(pt);
				if (thismfactor == 0.0)
				{
					if (!net_mfactorwarned)
						ttyputmsg(_("WARNING: Cell %s, node %s has a zero M-factor"),
							describenodeproto(ni->parent), describenodeinst(ni));
					net_mfactorwarned = TRUE;
				}
				submfactor *= thismfactor;
			}
		}

		/* run through each instantiation of the node */
		for(nindex = 0; nindex < nodewidth; nindex++)
		{
			if (flattenit != 0)		
			{
				/* put pseudo-netnumbers on the networks of this cell */
				for(net = realnp->firstnetwork; net != NONETWORK; net = net->nextnetwork)
					net->temp1 = (INTBIG)NOPNET;
				for(realpp = realnp->firstportproto; realpp != NOPORTPROTO; realpp = realpp->nextportproto)
				{
					if (realpp->network->temp1 != (INTBIG)NOPNET) continue;

					/* if there is an alternate contents cell, compute the port */
					if (realnp == anp) pp = realpp; else
					{
						pp = equivalentport(realnp, realpp, anp);
						if (pp == NOPORTPROTO) pp = anp->firstportproto;
					}

					/* see if an arc connects to the port */
					foundnet = NONETWORK;
					for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
					{
						temprealpp = equivalentport(anp, pi->proto, realnp);
						if (temprealpp == NOPORTPROTO)
						{
							if (pi->proto->network == pp->network) break;
						} else
						{
							if (temprealpp->network == realpp->network) break;
						}
					}
					if (pi != NOPORTARCINST && pi->conarcinst->network != NONETWORK)
					{
						foundnet = pi->conarcinst->network;
					} else
					{
						/* see if the port is an export */
						for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
						{
							temprealpp = equivalentport(anp, pe->proto, realnp);
							if (temprealpp == NOPORTPROTO)
							{
								if (pe->proto->network == pp->network) break;
							} else
							{
								if (temprealpp->network == realpp->network) break;
							}
						}
						if (pe != NOPORTEXPINST) foundnet = pe->exportproto->network; else
						{
							pn = net_newpnet(realpp->network, pnetlist, globalpnetlist);
							if (pn == NOPNET) return(0);
							net_addtraversalpath(pn, cell);
							if ((realpp->userbits&STATEBITS) == PWRPORT)
							{
								pn->flags = POWERNET;
							} else if ((realpp->userbits&STATEBITS) == GNDPORT)
							{
								pn->flags = GROUNDNET;
							}
							realpp->network->temp1 = (INTBIG)pn;
						}
					}
					if (foundnet != NONETWORK)
					{
						/* propagate export networks to nets inside cell */
						if (foundnet->buswidth > 1)
						{
							sigcount = foundnet->buswidth;
							if (nodewidth > 1 && realpp->network->buswidth * nodewidth == foundnet->buswidth)
							{
								/* map wide bus to a particular instantiation of an arrayed node */
								if (realpp->network->buswidth == 1)
								{
									realpp->network->temp1 = foundnet->networklist[nindex]->temp1;
								} else
								{
									for(i=0; i<realpp->network->buswidth; i++)
										realpp->network->networklist[i]->temp1 =
											foundnet->networklist[i + nindex*realpp->network->buswidth]->temp1;
								}
							} else
							{
								if (realpp->network->buswidth != foundnet->buswidth)
								{
									ttyputerr(_("***ERROR: port %s on node %s in cell %s is %d wide, but is connected/exported with width %d"),
										realpp->protoname, describenodeinst(ni), describenodeproto(cell),
											realpp->network->buswidth, foundnet->buswidth);
									sigcount = mini(sigcount, realpp->network->buswidth);
									if (sigcount == 1) sigcount = 0;
								}
								realpp->network->temp1 = (INTBIG)foundnet->temp1;
								for(i=0; i<sigcount; i++)
									realpp->network->networklist[i]->temp1 = foundnet->networklist[i]->temp1;
							}
						} else
						{
							realpp->network->temp1 = (INTBIG)foundnet->temp1;
						}
					}
				}

				/* recurse into the cell */
				downhierarchy(ni, realnp, nindex);
				initiallist = net_buildpseudo(realnp, initiallist, components, pnetlist,
					globalpnetlist, compare_hierarchically, figuresizes, submfactor);
				uphierarchy();
				if (initiallist == 0) return(0);
				continue;
			}

			/* nonflattenable component: add it to the pseudocomponent list */
			if (anp->primindex == 0)
			{
				fun = net_getcellfunction(anp);
			} else
			{
				fun = net_getfunction(ni);
				if (fun == NPCONNECT || fun == NPART || fun == NPUNKNOWN ||
					fun == NPCONPOWER || fun == NPCONGROUND) continue;

				/* special case: ignore resistors and capacitors if they are being ignored electrically */
				if (fun == NPRESIST || fun == NPCAPAC || fun == NPECAPAC)
				{
					if (asktech(sch_tech, x_("ignoring-resistor-topology")) != 0)
						continue;
				}
			}

			/* create a pseudo-component */
			pcomp = net_allocpcomp();
			if (pcomp == NOPCOMP) return(0);
			pcomp->nextpcomp = initiallist;
			initiallist = pcomp;
			pcomp->function = (INTSML)fun;
			pcomp->hashreason = 0;
			pcomp->forcedassociation = 0;
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, net_ncc_forcedassociationkey);
			if (var != NOVARIABLE)
			{
				pt = (CHAR *)var->addr;
				for(pt = (CHAR *)var->addr; *pt != 0; pt++)
					pcomp->forcedassociation += (INTBIG)*pt;
			}
			gettraversalpath(ni->parent, NOWINDOWPART, &nilist, &indexlist, &pathcount, 0);
			pcomp->hierpathcount = pathcount;
			if (pcomp->hierpathcount > 0)
			{
				pcomp->hierpath = (NODEINST **)emalloc(pcomp->hierpathcount *
					(sizeof (NODEINST *)), net_tool->cluster);
				if (pcomp->hierpath == 0) return(0);
				pcomp->hierindex = (INTBIG *)emalloc(pcomp->hierpathcount *
					SIZEOFINTBIG, net_tool->cluster);
				if (pcomp->hierindex == 0) return(0);
				for(i=0; i<pcomp->hierpathcount; i++)
				{
					pcomp->hierpath[i] = nilist[i];
					pcomp->hierindex[i] = indexlist[i];
				}
			}
			pcomp->actuallist = (void *)ni;
			pcomp->numactual = 1;
			pcomp->topactual = net_toplevelinst;
			(*components)++;

			/* count the number of electrically distinct nets on the component */
			pcomp->wirecount = 0;
			cnp = contentsview(anp);
			for(pp = anp->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			{
				/* get real export on contents */
				if (cnp == NONODEPROTO) realpp = pp; else
				{
					realpp = equivalentport(anp, pp, cnp);
					if (realpp == NOPORTPROTO) continue;
				}

				/* special case for isolated ports */
				if ((realpp->userbits&PORTISOLATED) != 0)
				{
					/* add one wire for each arc on the port */
					for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
						if (pi->proto == pp) pcomp->wirecount++;
					continue;
				}

				/* new port, add in the number of signals */
				if (pp->network->buswidth <= 1) pcomp->wirecount++; else
					pcomp->wirecount += pp->network->buswidth;
			}

			/* get parameters for the node */
			switch (fun)
			{
				case NPTRANMOS:  case NPTRA4NMOS:
				case NPTRADMOS:  case NPTRA4DMOS:
				case NPTRAPMOS:  case NPTRA4PMOS:
				case NPTRANJFET: case NPTRA4NJFET:
				case NPTRAPJFET: case NPTRA4PJFET:
				case NPTRADMES:  case NPTRA4DMES:
				case NPTRAEMES:  case NPTRA4EMES:
					/* transistors that have a length and a width */
					if (figuresizes)
					{
						lambda = lambdaofnode(ni);
						if (ni->proto->primindex == 0)
						{
							var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, el_attrkey_width);
							if (var != NOVARIABLE) width = var->addr * lambda; else
								width = 0;
							var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, el_attrkey_length);
							if (var != NOVARIABLE) length = var->addr * lambda; else
								length = 0;
						} else
						{
							transistorsize(ni, &length, &width);
						}
						pcomp->length = (float)muldiv(length, WHOLE, lambda);
						pcomp->width = (float)muldiv(width, WHOLE, lambda) * submfactor;
					} else
					{
						/* just set fake sizes */
						pcomp->length = pcomp->width = 2.0f * submfactor;
					}
					pcomp->flags |= COMPHASWIDLEN;
#ifdef IGNOREFOURPORT
					pcomp->wirecount = 3;
#endif
					break;

				case NPTRANPN:    case NPTRA4NPN:
				case NPTRAPNP:    case NPTRA4PNP:
					/* transistors that have an area */
					if (figuresizes)
					{
						lambda = lambdaofnode(ni);
						if (ni->proto->primindex == 0)
						{
							var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, el_attrkey_area);
							if (var != NOVARIABLE) length = var->addr * lambda; else
								length = 0;
						} else
						{
							transistorsize(ni, &length, &width);
						}
						if (length < 0) pcomp->length = 0.0; else
							pcomp->length = (float)muldiv(length, WHOLE, lambda) * submfactor;
					} else pcomp->length = 4.0f * submfactor;
					pcomp->width = 0.0;
					pcomp->flags |= COMPHASAREA;
					break;

				case NPRESIST:
				case NPCAPAC:   case NPECAPAC:
				case NPINDUCT:
					pcomp->length = net_getpartvalue(ni) * submfactor;
					pcomp->width = 0.0;
					pcomp->flags |= COMPHASAREA;
					break;
			}

			/* no further information if there are no wires */
			if (pcomp->wirecount == 0) continue;

			/* allocate the port and connection lists */
			pcomp->portlist = (PORTPROTO **)emalloc(((sizeof (PORTPROTO *)) * pcomp->wirecount),
				net_tool->cluster);
			if (pcomp->portlist == 0) return(0);
			pcomp->state = (INTSML *)emalloc((SIZEOFINTSML * pcomp->wirecount),
				net_tool->cluster);
			if (pcomp->state == 0) return(0);
			pcomp->portindices = (INTSML *)emalloc((SIZEOFINTSML * pcomp->wirecount),
				net_tool->cluster);
			if (pcomp->portindices == 0) return(0);
			pcomp->netnumbers = (PNET **)emalloc(((sizeof (PNET *)) * pcomp->wirecount),
				net_tool->cluster);
			if (pcomp->netnumbers == 0) return(0);
			for(i=0; i<pcomp->wirecount; i++)
				pcomp->state[i] = 0;

			switch (pcomp->function)
			{
				case NPTRANMOS:
				case NPTRADMOS:
				case NPTRAPMOS:
				case NPTRADMES:
				case NPTRAEMES:
				case NPTRANPN:
				case NPTRAPNP:
				case NPTRANJFET:
				case NPTRAPJFET:
					/* transistors make the active ports equivalent */
					if (anp->primindex == 0)
					{
						pcomp->portlist[0] = getportproto(anp, "g");
						pcomp->portlist[1] = getportproto(anp, "s");
						pcomp->portlist[2] = getportproto(anp, "d");
						if (pcomp->portlist[0] == NOPORTPROTO || pcomp->portlist[0] == NOPORTPROTO ||
							pcomp->portlist[0] == NOPORTPROTO)
						{
                            ttyputmsg("Transistor cell %s must have exports 'g', 's', and 'd'",
								describenodeproto(anp));
						}
					} else
					{
						pcomp->portlist[0] = anp->firstportproto;
						pcomp->portlist[1] = pcomp->portlist[0]->nextportproto;
						pcomp->portlist[2] = pcomp->portlist[1]->nextportproto;
						if (anp != sch_transistorprim && anp != sch_transistor4prim)
							pcomp->portlist[2] = pcomp->portlist[2]->nextportproto;
					}
					for(j=0; j<pcomp->wirecount; j++)
					{
						pp = pcomp->portlist[j];
						if (pp == NOPORTPROTO) continue;
						pcomp->portindices[j] = (INTSML)pp->network->temp2;
						if (net_getpnetandstate(ni, pp, NONETWORK, -1, &pcomp->netnumbers[j],
							&pcomp->state[j], pnetlist, globalpnetlist, nodewidth, nindex,
								pp->network->buswidth)) return(0);
					}
					break;

				case NPTRA4NMOS:
				case NPTRA4DMOS:
				case NPTRA4PMOS:
				case NPTRA4DMES:
				case NPTRA4EMES:
				case NPTRA4NPN:
				case NPTRA4PNP:
				case NPTRA4NJFET:
				case NPTRA4PJFET:
					/* 4-port transistors make the active two equivalent */
					if (anp->primindex == 0)
					{
						ttyputmsg("Cannot handle transistor functions yet");
					} else
					{
						pcomp->portlist[0] = anp->firstportproto;
						pcomp->portlist[1] = pcomp->portlist[0]->nextportproto;
						pcomp->portlist[2] = pcomp->portlist[1]->nextportproto;
						pcomp->portlist[3] = pcomp->portlist[2]->nextportproto;
						for(j=0; j<pcomp->wirecount; j++)
						{
							pp = pcomp->portlist[j];
							pcomp->portindices[j] = (INTSML)pp->network->temp2;
							if (net_getpnetandstate(ni, pp, NONETWORK, -1, &pcomp->netnumbers[j],
								&pcomp->state[j], pnetlist, globalpnetlist, nodewidth, nindex,
									pp->network->buswidth)) return(0);
						}
					}
					break;

				case NPRESIST:
				case NPCAPAC:   case NPECAPAC:
				case NPINDUCT:
					/* resistors, capacitors, and inductors have equivalent ports */
					pcomp->portlist[0] = anp->firstportproto;
					pcomp->portlist[1] = pcomp->portlist[0]->nextportproto;
					for(j=0; j<pcomp->wirecount; j++)
					{
						pp = pcomp->portlist[j];
						pcomp->portindices[j] = (INTSML)getprime(0);
						if (net_getpnetandstate(ni, pp, NONETWORK, -1, &pcomp->netnumbers[j],
							&pcomp->state[j], pnetlist, globalpnetlist, nodewidth, nindex,
								pp->network->buswidth)) return(0);
					}
					break;

				default:
					j = 0;
					for(pp = anp->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
					{
						/* get real export on contents */
						if (cnp == NONODEPROTO) realpp = pp; else
						{
							realpp = equivalentport(anp, pp, cnp);
							if (realpp == NOPORTPROTO) continue;
						}
						sigcount = realpp->network->buswidth;

						/* special case for isolated ports */
						if ((realpp->userbits&PORTISOLATED) != 0)
						{
							/* add one wire for each arc on the port */
							for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
							{
								if (pi->proto != pp) continue;
								pcomp->portlist[j] = pp;
								pcomp->portindices[j] = (INTSML)pp->network->temp2;
								if (net_getpnetandstate(ni, pp, pi->conarcinst->network, -1,
									&pcomp->netnumbers[j], &pcomp->state[j], pnetlist, globalpnetlist,
										nodewidth, nindex, sigcount)) return(0);
								j++;
							}
							continue;
						}

						if (sigcount <= 1)
						{
							/* single-wire port */
							pcomp->portlist[j] = pp;
							pcomp->portindices[j] = (INTSML)realpp->network->temp2;
							for(l=0; l<j; l++) if (pcomp->portindices[l] == pcomp->portindices[j]) break;
							if (l >= j)
							{
								if (net_getpnetandstate(ni, pp, NONETWORK, -1, &pcomp->netnumbers[j],
									&pcomp->state[j], pnetlist, globalpnetlist, nodewidth, nindex,
										sigcount)) return(0);
								j++;
							}
						} else
						{
							/* bus port */
							for(k=0; k<sigcount; k++)
							{
								pcomp->portlist[j] = pp;
								pcomp->portindices[j] = (INTSML)realpp->network->networklist[k]->temp2;
								for(l=0; l<j; l++) if (pcomp->portindices[l] == pcomp->portindices[j]) break;
								if (l >= j)
								{
									if (net_getpnetandstate(ni, pp, NONETWORK, k,
										&pcomp->netnumbers[j], &pcomp->state[j], pnetlist, globalpnetlist,
											nodewidth, nindex, sigcount)) return(0);
									j++;
								}
							}
						}
					}
					pcomp->wirecount = (INTSML)j;
					break;
			}
		}
	}
	return(initiallist);
}

/*
 * Routine to add the traversal path to cell "cell" to the PNET "pn".
 */
void net_addtraversalpath(PNET *pn, NODEPROTO *cell)
{
#ifdef PATHTOPNET
	NODEINST **nilist;
	INTBIG pathcount, *indexlist;
	REGISTER INTBIG i;

	gettraversalpath(cell, NOWINDOWPART, &nilist, &indexlist, &pathcount, 0);
	pn->hierpathcount = pathcount;
	if (pn->hierpathcount > 0)
	{
		pn->hierpath = (NODEINST **)emalloc(pn->hierpathcount *
			(sizeof (NODEINST *)), net_tool->cluster);
		if (pn->hierpath == 0) return;
		pn->hierindex = (INTBIG *)emalloc(pn->hierpathcount *
			SIZEOFINTBIG, net_tool->cluster);
		if (pn->hierindex == 0) return;
		for(i=0; i<pn->hierpathcount; i++)
		{
			pn->hierpath[i] = nilist[i];
			pn->hierindex[i] = indexlist[i];
		}
	}
#endif
}

/*
 * Routine to examine node "ni", port "pp", and find the PNET that is connected to it.
 * If "forcenet" is not NONETWORK, use it as the arc site.
 * If "index" is not negative, look for that entry in a bus.
 * The PNET and its state are stored in "netnumber" and "state".  If nothing is connected,
 * a new PNET is allocated and saved in the list "pnetlist".
 * Returns true on error.
 */
BOOLEAN net_getpnetandstate(NODEINST *ni, PORTPROTO *pp, NETWORK *forcenet,
	INTBIG index, PNET **netnumber, INTSML *state, PNET **pnetlist, PNET **globalpnetlist,
	INTBIG nodewidth, INTBIG nindex, INTBIG destsignals)
{
	REGISTER ARCINST *ai;
	REGISTER INTBIG entry;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER NETWORK *net;
	REGISTER PNET *pn;

	/* first look for an arc that connects */
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		if (forcenet != NONETWORK)
		{
			if (forcenet != pi->conarcinst->network) continue;
		} else
		{
			if (pi->proto->network != pp->network) continue;
		}

		/* pickup the network number of this connection */
		ai = pi->conarcinst;
		net = ai->network;
		if (net == NONETWORK) continue;
		if (index < 0)
		{
			if (nodewidth > 1 && net->buswidth == nodewidth)
				*netnumber = (PNET *)net->networklist[nindex]->temp1; else
					*netnumber = (PNET *)net->temp1;
		} else
		{
			entry = index;
			if (nodewidth > 1 && net->buswidth == nodewidth * destsignals)
				entry = index + nindex*destsignals;
			if (entry < net->buswidth)
				*netnumber = (PNET *)net->networklist[entry]->temp1;
		}
		if ((ai->userbits&ISNEGATED) != 0)
		{
			if ((ai->end[0].portarcinst == pi && (ai->userbits&REVERSEEND) == 0) ||
				(ai->end[1].portarcinst == pi && (ai->userbits&REVERSEEND) != 0))
					*state = NEGATEDPORT;
		}
		if (*netnumber != NOPNET)
		{
			if (((*netnumber)->flags&EXPORTEDNET) != 0)
				*state |= EXPORTEDPORT;
			if (forcenet == NONETWORK)
			{
				if ((pp->userbits&STATEBITS) == GNDPORT) (*netnumber)->flags |= GROUNDNET; else
					if ((pp->userbits&STATEBITS) == PWRPORT) (*netnumber)->flags |= POWERNET;
			}
			return(FALSE);
		}
	}

	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
	{
		if (forcenet != NONETWORK)
		{
			if (forcenet != pe->proto->network) continue;
		} else
		{
			if (pe->proto->network != pp->network) continue;
		}
		net = pe->exportproto->network;
		if (index < 0)
		{
			if (nodewidth > 1 && net->buswidth == nodewidth)
				*netnumber = (PNET *)net->networklist[nindex]->temp1; else
					*netnumber = (PNET *)net->temp1;
		} else
		{
			entry = index;
			if (nodewidth > 1 && net->buswidth == nodewidth * destsignals)
				entry = index + nindex*destsignals;
			if (entry < net->buswidth)
				*netnumber = (PNET *)net->networklist[entry]->temp1;
		}
		*state |= EXPORTEDPORT;
		if (*netnumber != NOPNET) return(FALSE);
	}

	/* not found on arc or export: create a new entry */
	if (forcenet == NONETWORK)
		ttyputmsg(_("Warning: cell %s: no connection to node %s, port %s"),
			describenodeproto(ni->parent), describenodeinst(ni), pp->protoname);
	pn = net_newpnet(forcenet, pnetlist, globalpnetlist);
	if (pn == NOPNET) return(TRUE);
	net_addtraversalpath(pn, ni->parent);
	pn->network = forcenet;
	*netnumber = pn;
	return(FALSE);
}

/*
 * Routine to fill in the network pointers to components.
 */
void net_fillinnetpointers(PCOMP *pcomplist, PNET *pnetlist)
{
	REGISTER PCOMP *pc;
	REGISTER PNET *pn;
	REGISTER INTBIG i, index;

	for(pn = pnetlist; pn != NOPNET; pn = pn->nextpnet)
		pn->nodecount = 0;
	for(pc = pcomplist; pc != NOPCOMP; pc = pc->nextpcomp)
	{
		for(i=0; i<pc->wirecount; i++)
		{
			pn = pc->netnumbers[i];
			pn->nodecount++;
		}
	}

	for(pn = pnetlist; pn != NOPNET; pn = pn->nextpnet)
	{
		if (pn->nodecount <= pn->nodetotal) continue;
		if (pn->nodetotal > 0)
		{
			efree((CHAR *)pn->nodelist);
			efree((CHAR *)pn->nodewire);
			pn->nodetotal = 0;
		}
		pn->nodelist = (PCOMP **)emalloc(pn->nodecount * (sizeof (PCOMP *)), net_tool->cluster);
		if (pn->nodelist == 0) return;
		pn->nodewire = (INTBIG *)emalloc(pn->nodecount * SIZEOFINTBIG, net_tool->cluster);
		if (pn->nodelist == 0) return;
		pn->nodetotal = pn->nodecount;
	}
	for(pn = pnetlist; pn != NOPNET; pn = pn->nextpnet)
		pn->nodecount = 0;
	for(pc = pcomplist; pc != NOPCOMP; pc = pc->nextpcomp)
	{
		for(i=0; i<pc->wirecount; i++)
		{
			pn = pc->netnumbers[i];
			if (pn->nodelist == 0)
				continue;
			index = pn->nodecount++;
			if (index >= pn->nodetotal) continue;
			pn->nodelist[index] = pc;
			pn->nodewire[index] = i;
		}
	}
}	

/*
 * Routine to reduce the network in "pcomp" to merge parallel components.
 */
INTBIG net_mergeparallel(PCOMP **pcomp, PNET *pnet, INTBIG *components)
{
	REGISTER PCOMP *pc, *opc, *nextpc, *lastpc, **complist;
	REGISTER PNET *pn;
	REGISTER INTBIG i, j, k, m, newnum, mergecount, compcount, counter;
	REGISTER NODEINST *ni, **newlist, *newsingle;

	net_fillinnetpointers(*pcomp, pnet);

	/* assign values to each net */
	counter = 0;
	for(pn = pnet; pn != NOPNET; pn = pn->nextpnet)
	{
		counter += 6;
		pn->timestamp = counter;
	}

	/* compute hash value for each component */
	compcount = 0;
	for(pc = *pcomp; pc != NOPCOMP; pc = pc->nextpcomp)
	{
		pc->flags &= ~COMPDELETED;
		if (pc->function == NPUNKNOWN) continue;
		counter = pc->function;
		for(i=0; i<pc->wirecount; i++)
		{
			pn = pc->netnumbers[i];
			counter += pn->timestamp * pc->portindices[i];
		}

		pc->timestamp = counter;
		compcount++;
	}

	if (compcount == 0) return(0);
	complist = (PCOMP **)emalloc(compcount * (sizeof (PCOMP *)), net_tool->cluster);
	if (complist == 0) return(0);
	compcount = 0;
	for(pc = *pcomp; pc != NOPCOMP; pc = pc->nextpcomp)
	{
		if (pc->function == NPUNKNOWN) continue;
		complist[compcount++] = pc;
	}
	esort(complist, compcount, sizeof (PCOMP *), net_sortpcompbyhash);

	mergecount = 0;
	for(i=0; i<compcount; i++)
	{
		if ((i%5) == 0)
		{
			if (stopping(STOPREASONNCC)) break;
		}
		opc = complist[i];
		if ((opc->flags & COMPDELETED) != 0) continue;

		for(m=i+1; m<compcount; m++)
		{
			pc = complist[m];
			if (pc->timestamp != opc->timestamp) break;
			if ((pc->flags & COMPDELETED) != 0) continue;

			/* both components must have the same function */
			if (pc->function != opc->function) continue;

			/* compare the wire lists */
			if (net_comparewirelist(pc, opc, FALSE)) continue;

			/* components are equivalent: delete "pc" */
			mergecount++;

			/* add "pc"s node pointer to "opc" */
			newnum = pc->numactual + opc->numactual;
			newsingle = NONODEINST;
			if (newnum > 1)
			{
				newlist = (NODEINST **)emalloc(newnum * (sizeof (NODEINST *)),
					net_tool->cluster);
				if (newlist == 0) return(0);
			} else newlist = 0;
			k = 0;
			for(j=0; j<pc->numactual; j++)
			{
				if (pc->numactual == 1) ni = (NODEINST *)pc->actuallist; else
					ni = ((NODEINST **)pc->actuallist)[j];
				if (newnum == 1) newsingle = ni; else
					newlist[k++] = ni;
			}
			for(j=0; j<opc->numactual; j++)
			{
				if (opc->numactual == 1) ni = (NODEINST *)opc->actuallist; else
					ni = ((NODEINST **)opc->actuallist)[j];
				if (newnum == 1) newsingle = ni; else
					newlist[k++] = ni;
			}
			if (opc->numactual > 1) efree((CHAR *)opc->actuallist);
			if (newnum == 1) opc->actuallist = (void *)newsingle; else
				opc->actuallist = (void *)newlist;
			opc->numactual = newnum;

			/* combine sizes (as specified by Robert Bosnyak) */
			if ((pc->flags&(COMPHASWIDLEN|COMPHASAREA)) != 0 &&
				(opc->flags&(COMPHASWIDLEN|COMPHASAREA)) != 0)
			{
				switch (pc->function)
				{
					case NPTRANMOS:  case NPTRA4NMOS:
					case NPTRADMOS:  case NPTRA4DMOS:
					case NPTRAPMOS:  case NPTRA4PMOS:
						/* FET transistors in parallel depend on whether the length is the same */
						if (opc->length == pc->length)
						{
							/* same-length transistors: sum the width */
							opc->width += pc->width;
						} else
						{
							/* different-length transistors: more complex formula */
							if (pc->width + opc->width != 0.0)
							{
								opc->length = (pc->width * pc->length + opc->width * opc->length) /
									(pc->width + opc->width);
							}
							opc->width += pc->width;
						}
						break;
					case NPTRANPN:   case NPTRA4NPN:
					case NPTRAPNP:   case NPTRA4PNP:
					case NPTRANJFET: case NPTRA4NJFET:
					case NPTRAPJFET: case NPTRA4PJFET:
					case NPTRADMES:  case NPTRA4DMES:
					case NPTRAEMES:  case NPTRA4EMES:
						/* nonFET transistors in parallel sum the area */
						opc->length += pc->length;
						break;
					case NPRESIST:
					case NPINDUCT:
						/* resistance and capacitance in parallel take product over sum */
						if (pc->length + opc->length != 0.0)
							opc->length = (pc->length * opc->length) / (pc->length + opc->length);
						break;
					case NPCAPAC:  case NPECAPAC:
					case NPDIODE:  case NPDIODEZ:
						/* capacitance and diode in parallel sum the farads/area */
						opc->length += pc->length;
						break;
				}
			}

			pc->flags |= COMPDELETED;
		}
	}
	efree((CHAR *)complist);

	/* remove deleted components */
	lastpc = NOPCOMP;
	for(pc = *pcomp; pc != NOPCOMP; pc = nextpc)
	{
		nextpc = pc->nextpcomp;
		if ((pc->flags&COMPDELETED) != 0)
		{
			if (lastpc == NOPCOMP) *pcomp = pc->nextpcomp; else
				lastpc->nextpcomp = pc->nextpcomp;
			net_freepcomp(pc);
			(*components)--;
			continue;
		}
		lastpc = pc;
	}
	return(mergecount);
}

int net_sortpcompbyhash(const void *e1, const void *e2)
{
	REGISTER PCOMP *pc1, *pc2;

	pc1 = *((PCOMP **)e1);
	pc2 = *((PCOMP **)e2);
	return(pc1->timestamp - pc2->timestamp);
}

/*
 * Routine to add PCOMP "pc" to the list of series transistors.
 */
BOOLEAN net_addtoserieslist(PCOMP *pc)
{
	REGISTER INTBIG newtotal, i;
	REGISTER PCOMP **newlist;

	if (net_serieslistcount >= net_serieslisttotal)
	{
		newtotal = net_serieslisttotal * 2;
		if (newtotal <= net_serieslistcount)
			newtotal = net_serieslistcount+1;
		newlist = (PCOMP **)emalloc(newtotal * (sizeof (PCOMP *)), net_tool->cluster);
		if (newlist == 0) return(TRUE);
		for(i=0; i<net_serieslistcount; i++)
			newlist[i] = net_serieslist[i];
		if (net_serieslisttotal > 0) efree((CHAR *)net_serieslist);
		net_serieslist = newlist;
		net_serieslisttotal = newtotal;
	}
	net_serieslist[net_serieslistcount] = pc;
	net_serieslistcount++;
	return(FALSE);
}

/*
 * Routine to gather a list of series transistors that include "pc".  Returns
 * the list, and its length in "seriescount".  If "seriescount" is less than 2,
 * no chain of series transistors has been found.
 */
PCOMP **net_gatherseries(PCOMP *pc, INTBIG *seriescount)
{
	net_serieslistcount = 0;
	net_seriescon1 = net_seriescon2 = -1;
	(void)net_addtoserieslist(pc);
	net_crawlforseries(pc);
	*seriescount = net_serieslistcount;
	return(net_serieslist);
}

/*
 * Recursive helper routine for "net_gatherseries" to find transistors adjacent
 * to "pc" and add them to the series chain if they are series transistors.
 */
void net_crawlforseries(PCOMP *pc)
{
	REGISTER INTBIG i, j, badend;
	REGISTER PCOMP *opc;
	REGISTER PNET *pn;

	/* check source and drain connections */
	for(i=1; i<3; i++)
	{
		badend = 0;
		opc = NOPCOMP;

		/* connection must be to exactly 1 other component */
		pn = pc->netnumbers[i];
		if (pn->nodecount != 2) badend = 1; else
		{
			opc = pn->nodelist[0];
			if (opc == pc) opc = pn->nodelist[1];
		}

		/* other component must be the same as this */
		if (badend == 0)
		{
			if (opc->function != pc->function) badend = 1;
		}

		/* both components must be in the same cell */
		if (badend == 0)
		{
			if (pc->hierpathcount != opc->hierpathcount) continue;
			for(j=0; j<pc->hierpathcount; j++)
				if (pc->hierpath[j] != opc->hierpath[j] ||
					pc->hierindex[j] != opc->hierindex[j]) break;
			if (j < pc->hierpathcount) badend = 1;
		}

		/* other component must point just to this on its source or drain */
		if (badend == 0)
		{
			pn = pc->netnumbers[i];
			if ((pn->flags&(EXPORTEDNET|POWERNET|GROUNDNET)) != 0) badend = 1;
		}

		/* other component must not already be in the list */
		if (badend == 0)
		{
			for(j=0; j<net_serieslistcount; j++)
				if (net_serieslist[j] == opc) break;
			if (j < net_serieslistcount) badend = 2;
		}

		switch (badend)
		{
			case 0:		/* good end */
				/* another series transistor found */
				(void)net_addtoserieslist(opc);

				/* recursively search for others */
				net_crawlforseries(opc);
				break;
			case 1:		/* bad end: end of chain */
				/* add to the end list */
				if (net_seriescon1 < 0)
				{
					net_seriesend1 = pc;
					net_seriescon1 = i;
				} else if (net_seriescon2 < 0)
				{
					net_seriesend2 = pc;
					net_seriescon2 = i;
				}
				break;
		}
	}
}

/*
 * Routine to reduce the network in "pcomp" to merge series transistors into more complex
 * single components.
 */
INTBIG net_mergeseries(PCOMP **pcomp, PNET *pnet, INTBIG *components)
{
	REGISTER PCOMP *pc, *opc, *nextpc, *lastpc, **serieslist, *newpc, *npc;
	REGISTER INTBIG i, j, k, t, mergecount;
	INTBIG seriescount;
	REGISTER NODEINST *ni;

	/* clear flags on every component */
	for(pc = *pcomp; pc != NOPCOMP; pc = pc->nextpcomp) pc->timestamp = -1;
	net_fillinnetpointers(*pcomp, pnet);

	/* scan for series transistors */
	mergecount = 0;
	for(pc = *pcomp; pc != NOPCOMP; pc = nextpc)
	{
		nextpc = pc->nextpcomp;
		if (pc->function != NPTRANMOS && pc->function != NPTRADMOS &&
			pc->function != NPTRAPMOS) continue;

		/* look for adjacent single transistor */
		serieslist = net_gatherseries(pc, &seriescount);
		if (seriescount <= 1) continue;

		mergecount++;

		/* mark transistors */
		for(t=0; t<seriescount; t++) serieslist[t]->timestamp = t;

		/* create a new component with all the features of the gates and ends */
		newpc = net_allocpcomp();
		if (newpc == NOPCOMP) return(0);
		newpc->nextpcomp = *pcomp;
		*pcomp = newpc;
		newpc->topactual = pc->topactual;
		newpc->hierpathcount = pc->hierpathcount;
		if (newpc->hierpathcount > 0)
		{
			newpc->hierpath = (NODEINST **)emalloc(newpc->hierpathcount *
				(sizeof (NODEINST *)), net_tool->cluster);
			if (newpc->hierpath == 0) return(0);
			newpc->hierindex = (INTBIG *)emalloc(newpc->hierpathcount *
				SIZEOFINTBIG, net_tool->cluster);
			if (newpc->hierindex == 0) return(0);
			for(i=0; i<newpc->hierpathcount; i++)
			{
				newpc->hierpath[i] = pc->hierpath[i];
				newpc->hierindex[i] = pc->hierindex[i];
			}
		}
		newpc->flags = pc->flags;
		newpc->function = (INTSML)(pc->function * 1000 + seriescount);
		newpc->wirecount = (INTSML)(seriescount + 2);
		newpc->timestamp = -1;
		newpc->hashreason = 0;
		newpc->forcedassociation = 0;
		for(t=0; t<seriescount; t++) newpc->forcedassociation += serieslist[t]->forcedassociation;

		/* length is the sum of all lengths, width is the average width */
		newpc->length = 0.0;
		newpc->width = 0.0;
		for(t=0; t<seriescount; t++)
		{
			newpc->length += serieslist[t]->length;
			newpc->width += serieslist[t]->width;
		}
		newpc->width /= seriescount;

		/* build "actual" list from all of the individual transistors */
		newpc->numactual = 0;
		for(t=0; t<seriescount; t++) newpc->numactual += serieslist[t]->numactual;
		newpc->actuallist = (NODEINST **)emalloc(newpc->numactual * (sizeof (NODEINST *)),
			net_tool->cluster);
		if (newpc->actuallist == 0) return(0);
		j = 0;
		for(t=0; t<seriescount; t++)
		{
			opc = serieslist[t];
			for(k=0; k < opc->numactual; k++)
			{
				if (opc->numactual == 1) ni = (NODEINST *)opc->actuallist; else
					ni = ((NODEINST **)opc->actuallist)[k];
				((NODEINST **)newpc->actuallist)[j++] = ni;
			}
		}

		/* allocate the pointer arrays */
		newpc->portlist = (PORTPROTO **)emalloc(((sizeof (PORTPROTO *)) * newpc->wirecount),
			net_tool->cluster);
		if (newpc->portlist == 0) return(0);
		newpc->state = (INTSML *)emalloc((SIZEOFINTSML * newpc->wirecount),
			net_tool->cluster);
		if (newpc->state == 0) return(0);
		newpc->portindices = (INTSML *)emalloc((SIZEOFINTSML * newpc->wirecount),
			net_tool->cluster);
		if (newpc->portindices == 0) return(0);
		newpc->netnumbers = (PNET **)emalloc(((sizeof (PNET *)) * newpc->wirecount),
			net_tool->cluster);
		if (newpc->netnumbers == 0) return(0);

		/* pick up gates from all series transistors */
		for(t=0; t<seriescount; t++)
		{
			newpc->portindices[t] = (INTSML)getprime(0);
			newpc->portlist[t] = serieslist[t]->portlist[0];
			newpc->netnumbers[t] = serieslist[t]->netnumbers[0];
			newpc->state[t] = serieslist[t]->state[0];
		}

		/* add in new source and drain */
		newpc->portindices[t] = (INTSML)getprime(1);
		newpc->portlist[t] = net_seriesend1->portlist[net_seriescon1];
		newpc->netnumbers[t] = net_seriesend1->netnumbers[net_seriescon1];
		newpc->state[t] = net_seriesend1->state[net_seriescon1];
		t++;
		newpc->portindices[t] = (INTSML)getprime(1);
		newpc->portlist[t] = net_seriesend2->portlist[net_seriescon2];
		newpc->netnumbers[t] = net_seriesend2->netnumbers[net_seriescon2];
		newpc->state[t] = net_seriesend2->state[net_seriescon2];
		(*components)++;

		/* fix pointer to the next transistor */
		while (nextpc != NOPCOMP && nextpc->timestamp != -1)
			nextpc = nextpc->nextpcomp;

		/* remove the series transistors */
		lastpc = NOPCOMP;
		for(opc = *pcomp; opc != NOPCOMP; opc = npc)
		{
			npc = opc->nextpcomp;
			if (opc->timestamp != -1)
			{
				/* remove this */
				if (lastpc == NOPCOMP) *pcomp = npc; else
					lastpc->nextpcomp = npc;
				net_freepcomp(opc);
				(*components)--;
				continue;
			}
			lastpc = opc;
		}
	}
	return(mergecount);
}

/*
 * routine to compare pseudocomponent "p1" and "p2", returning true if they
 * are different.
 */
BOOLEAN net_comparewirelist(PCOMP *p1, PCOMP *p2, BOOLEAN useportnames)
{
	REGISTER INTBIG i, j;
	Q_UNUSED( useportnames );

	/* simple test: number of wire lists must be equal */
	if (p1->wirecount != p2->wirecount) return(TRUE);

	/* if ports must match in sequence, check is simpler */
	if (p1->function == NPTRANPN || p1->function == NPTRAPNP ||
		p1->function == NPDIODE || p1->function == NPDIODEZ ||
		p1->function == NPBUFFER || p1->function == NPFLIPFLOP ||
		p1->function == NPUNKNOWN)
	{
		for(i=0; i<p1->wirecount; i++)
		{
			if (p1->netnumbers[i] != p2->netnumbers[i]) return(TRUE);
		}
		return(FALSE);
	}

	/* make sure there is memory for flags corresponding to component 2 */
	if (p2->wirecount > net_comparelistsize)
	{
		if (net_comparelistsize != 0) efree((CHAR *)net_comparelist);
		net_comparelist = (BOOLEAN *)emalloc(((sizeof (BOOLEAN)) * p2->wirecount),
			net_tool->cluster);
		net_comparelistsize = p2->wirecount;
	}

	/* reset flags in list for component 2 */
	for(j=0; j<p2->wirecount; j++) net_comparelist[j] = FALSE;

	for(i=0; i<p1->wirecount; i++)
	{
		if (p1->netnumbers[i]->nodecount == 0) return(TRUE);
		for(j=0; j<p2->wirecount; j++)
		{
			if (net_comparelist[j]) continue;
			if (p1->portindices[i] != p2->portindices[j]) continue;
			if (p1->netnumbers[i] != p2->netnumbers[j]) continue;
			net_comparelist[j] = TRUE;
			break;
		}
		if (j >= p2->wirecount) return(TRUE);
	}
	return(FALSE);
}

float net_findunits(NODEINST *ni, INTBIG units)
{
	REGISTER INTBIG i;
	REGISTER VARIABLE *var;

	for(i=0; i<ni->numvar; i++)
	{
		var = &ni->firstvar[i];
		if (TDGETUNITS(var->textdescript) == units)
		{
			return((float)eatof(describesimplevariable(var)));
		}
	}
	return(0.0);
}

float net_getpartvalue(NODEINST *ni)
{
	REGISTER NODEPROTO *np;
	REGISTER VARIABLE *var;
	REGISTER CHAR *pt;
	REGISTER INTBIG fun;

	np = ni->proto;
	if (np->primindex == 0)
	{
		fun = net_getcellfunction(np);
		if (fun == NPUNKNOWN) return(0.0);

		/* cell has function override */
		if (fun == NPRESIST)
			return(net_findunits(ni, VTUNITSRES));
		if (fun == NPCAPAC || fun == NPECAPAC)
			return(net_findunits(ni, VTUNITSCAP));
		if (fun == NPINDUCT)
			return(net_findunits(ni, VTUNITSIND));
	}

	/* diodes have area on them */
	if (np == sch_diodeprim)
	{
		var = getvalkey((INTBIG)ni, VNODEINST, -1, sch_diodekey);
		pt = describesimplevariable(var);
		return((float)eatof(pt));
	}

	/* capacitors have Farads on them */
	if (np == sch_capacitorprim)
	{
		var = getvalkey((INTBIG)ni, VNODEINST, -1, sch_capacitancekey);
		if (var == NOVARIABLE) return(0.0);
		return((float)eatof(describesimplevariable(var)));
	}

	/* resistors have Ohms on them */
	if (np == sch_resistorprim)
	{
		var = getvalkey((INTBIG)ni, VNODEINST, -1, sch_resistancekey);
		if (var == NOVARIABLE) return(0.0);
		return((float)eatof(describesimplevariable(var)));
	}

	/* inductors have Henrys on them */
	if (np == sch_inductorprim)
	{
		var = getvalkey((INTBIG)ni, VNODEINST, -1, sch_inductancekey);
		if (var == NOVARIABLE) return(0.0);
		return((float)eatof(describesimplevariable(var)));
	}
	return(0.0);
}

INTBIG net_getcellfunction(NODEPROTO *np)
{
	REGISTER VARIABLE *var;
	REGISTER INTBIG fun;

	var = getvalkey((INTBIG)np, VNODEPROTO, VSTRING, net_ncc_function_key);
	if (var != NOVARIABLE)
	{
		for(fun=0; fun<MAXNODEFUNCTION; fun++)
			if (namesame((CHAR *)var->addr, nodefunctionname(fun, NONODEINST)) == 0)
				return(fun);
	}
	return(NPUNKNOWN);
}

/*********************** ALLOCATION ***********************/

/*
 * routine to create a new PNET module for network "net" (if this is a global net,
 * merge it with others of the same name).  Increases the net number count in the
 * global "net_pseudonode" and adds the module to the list in "pnetlist".
 * Return the module (NOPNET on error).
 */
PNET *net_newpnet(NETWORK *net, PNET **localpnetlist, PNET **globalpnetlist)
{
	REGISTER PNET *pn, **pnetlist;
	REGISTER NETWORK *onet;
	REGISTER CHAR *globalname;
	REGISTER UINTBIG characteristics;
	REGISTER BOOLEAN isglobal;

	/* see if this is a global network */
	if (net != NONETWORK && net->globalnet >= 0 &&
		net->globalnet < net->parent->globalnetcount) isglobal = TRUE; else
			isglobal = FALSE;

	/* if the net is global, look for an equivalent */
	if (isglobal)
	{
		pnetlist = globalpnetlist;
		if (net->globalnet <= 1)
		{
			/* power and ground: search by index */
			for(pn = *pnetlist; pn != NOPNET; pn = pn->nextpnet)
			{
				onet = pn->network;
				if (onet == NONETWORK) continue;
				if (net->globalnet == onet->globalnet) return(pn);
			}
		} else
		{
			/* not power-and-ground: search by name */
			globalname = net->parent->globalnetnames[net->globalnet];
			for(pn = *pnetlist; pn != NOPNET; pn = pn->nextpnet)
			{
				if ((pn->flags&GLOBALNET) == 0) continue;
				onet = pn->network;
				if (onet == NONETWORK) continue;
				if (onet->globalnet < 0) continue;
				if (onet->globalnet >= onet->parent->globalnetcount) continue;
				if (namesame(onet->parent->globalnetnames[onet->globalnet], globalname) == 0)
					return(pn);
			}
		}
	} else
	{
		pnetlist = localpnetlist;
	}

	/* global net not found */
	pn = net_allocpnet();
	if (pn == 0) return(NOPNET);
	net_pseudonode++;
	pn->nextpnet = *pnetlist;
	*pnetlist = pn;
	pn->network = net;
	pn->forcedassociation = 0;
	if (isglobal)
	{
		pn->flags |= GLOBALNET;
		characteristics = net->parent->globalnetchar[net->globalnet];
		if (characteristics == PWRPORT) pn->flags |= POWERNET; else
			if (characteristics == GNDPORT) pn->flags |= GROUNDNET;
	}
	return(pn);
}

/*
 * Routine to add portproto "pp" to the list of ports on PNET "pn".
 */
void net_addexporttopnet(PNET *pn, PORTPROTO *pp)
{
	REGISTER PORTPROTO **newportlist, *npp;
	REGISTER INTBIG i;

	if (pn->realportcount <= 0)
	{
		pn->realportcount = 1;
		pn->realportlist = pp;
		return;
	}
	newportlist = (PORTPROTO **)emalloc((pn->realportcount+1) *
		(sizeof (PORTPROTO *)), net_tool->cluster);
	if (newportlist == 0) return;
	for(i=0; i<pn->realportcount; i++)
	{
		if (pn->realportcount == 1) npp = (PORTPROTO *)pn->realportlist; else
			npp = ((PORTPROTO **)pn->realportlist)[i];
		newportlist[i] = npp;
	}
	newportlist[pn->realportcount] = pp;
	if (pn->realportcount > 1) efree((CHAR *)pn->realportlist);
	pn->realportlist = newportlist;
	pn->realportcount++;
}

/*
 * routine to allocate a new pcomp module from the pool (if any) or memory
 */
PCOMP *net_allocpcomp(void)
{
	REGISTER PCOMP *pc;

	if (net_pcompfree == NOPCOMP)
	{
		pc = (PCOMP *)emalloc(sizeof (PCOMP), net_tool->cluster);
		if (pc == 0) return(NOPCOMP);
	} else
	{
		pc = net_pcompfree;
		net_pcompfree = (PCOMP *)pc->nextpcomp;
	}
	pc->flags = 0;
	return(pc);
}

/*
 * routine to return pcomp module "pc" to the pool of free modules
 */
void net_freepcomp(PCOMP *pc)
{
	if (pc->wirecount != 0)
	{
		efree((CHAR *)pc->portlist);
		efree((CHAR *)pc->state);
		efree((CHAR *)pc->netnumbers);
		efree((CHAR *)pc->portindices);
	}
	if (pc->numactual > 1) efree((CHAR *)pc->actuallist);
	if (pc->hashreason != 0) efree((CHAR *)pc->hashreason);
	if (pc->hierpathcount != 0)
	{
		efree((CHAR *)pc->hierpath);
		efree((CHAR *)pc->hierindex);
	}

	pc->nextpcomp = net_pcompfree;
	net_pcompfree = pc;
}


/*
 * routine to allocate a new pnet module from the pool (if any) or memory
 */
PNET *net_allocpnet(void)
{
	REGISTER PNET *pn;

	if (net_pnetfree == NOPNET)
	{
		pn = (PNET *)emalloc(sizeof (PNET), net_tool->cluster);
		if (pn == 0) return(NOPNET);
		pn->nodetotal = 0;
	} else
	{
		pn = net_pnetfree;
		net_pnetfree = (PNET *)pn->nextpnet;
	}

	pn->flags = 0;
	pn->nodecount = 0;
	pn->realportcount = 0;
#ifdef PATHTOPNET
	pn->hierpathcount = 0;
#endif
	return(pn);
}

/*
 * routine to return pnet module "pn" to the pool of free modules
 */
void net_freepnet(PNET *pn)
{
	if (pn->realportcount > 1)
		efree((CHAR *)pn->realportlist);
#ifdef PATHTOPNET
	if (pn->hierpathcount > 0)
	{
		efree((CHAR *)pn->hierpath);
		efree((CHAR *)pn->hierindex);
	}
#endif

	pn->nextpnet = net_pnetfree;
	net_pnetfree = pn;
}

/*
 * routine to free all allocated structures in the list of pseudonets
 * headed by "pnlist"
 */
void net_freeallpnet(PNET *pnlist)
{
	REGISTER PNET *pn, *nextpn;

	for(pn = pnlist; pn != NOPNET; pn = nextpn)
	{
		nextpn = pn->nextpnet;
		net_freepnet(pn);
	}
}

/*
 * routine to free all allocated structures in the list of pseudocomponents
 * headed by "pclist"
 */
void net_freeallpcomp(PCOMP *pclist)
{
	REGISTER PCOMP *pc, *nextpc;

	for(pc = pclist; pc != NOPCOMP; pc = nextpc)
	{
		nextpc = pc->nextpcomp;
		net_freepcomp(pc);
	}
}

/*********************** COMPONENT FUNCTION ***********************/

#define NOTRANMODEL ((TRANMODEL *)-1)

typedef struct Itranmodel
{
	CHAR *modelname;
	INTBIG tmindex;
	struct Itranmodel *nexttranmodel;
} TRANMODEL;

static TRANMODEL *net_firsttranmodel = NOTRANMODEL;

/* must be larger than largest node function entry in "efunction.h" */
static INTBIG net_tranmodelindex = 100;

/*
 * routine to return the function of node "ni"
 */
INTBIG net_getfunction(NODEINST *ni)
{
	REGISTER INTBIG fun;
	REGISTER TRANMODEL *tm;

	fun = nodefunction(ni);
	switch (fun)
	{
		case NPTRANS:
			fun = NPTRANMOS;
			break;

		case NPTRANS4:
#ifdef IGNOREFOURPORT
			fun = NPTRANMOS;
#else
			fun = NPTRA4NMOS;
#endif
			break;

#ifdef IGNOREFOURPORT
		case NPTRA4NMOS:  fun = NPTRANMOS;   break;
		case NPTRA4DMOS:  fun = NPTRADMOS;   break;
		case NPTRA4PMOS:  fun = NPTRAPMOS;   break;
		case NPTRA4NJFET: fun = NPTRANJFET;  break;
		case NPTRA4PJFET: fun = NPTRAPJFET;  break;
		case NPTRA4DMES:  fun = NPTRADMES;   break;
		case NPTRA4EMES:  fun = NPTRAEMES;   break;
		case NPTRA4NPN:   fun = NPTRANPN;    break;
		case NPTRA4PNP:   fun = NPTRAPNP;    break;
#endif

		case NPTRANSREF:
			/* self-referential transistor: lookup the string in the table */
			for(tm = net_firsttranmodel; tm != NOTRANMODEL; tm = tm->nexttranmodel)
				if (namesame(tm->modelname, ni->proto->protoname) == 0) break;
			if (tm == NOTRANMODEL)
			{
				/* new table entry */
				tm = (TRANMODEL *)emalloc(sizeof (TRANMODEL), net_tool->cluster);
				if (tm == 0) break;
				(void)allocstring(&tm->modelname, ni->proto->protoname, net_tool->cluster);
				tm->tmindex = net_tranmodelindex++;
				tm->nexttranmodel = net_firsttranmodel;
				net_firsttranmodel = tm;
			}
			fun = tm->tmindex;
			break;

		case NPPIN:
		case NPNODE:
		case NPCONTACT:
		case NPWELL:
		case NPSUBSTRATE:
			fun = NPCONNECT;
			break;
	}
	return(fun);
}

/*
 * Routine to initialize the database for network flattening and comparison.
 * Assigns unique function numbers to the "temp1&NETCELLCODE" field of each cell.
 */
void net_initnetflattening(void)
{
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np, **celllist;
	REGISTER INTBIG i, primeindex, cellcount, cellcode;
	REGISTER PORTPROTO *pp;
	REGISTER TECHNOLOGY *tech;
	REGISTER NETWORK *net;

	/* assign unique factors to the "temp1&NETCELLCODE" field of each cell, matching across libraries */
	cellcount = 0;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			cellcount++;
	}
	if (cellcount > 0)
	{
		celllist = (NODEPROTO **)emalloc(cellcount * (sizeof (NODEPROTO *)), net_tool->cluster);
		if (celllist == 0) return;
		cellcount = 0;
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		{
			if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				celllist[cellcount++] = np;
		}
		esort(celllist, cellcount, sizeof (NODEPROTO *), net_cellnameascending);
		primeindex = 1;
		for(i=0; i<cellcount; i++)
		{
			np = celllist[i];
			cellcode = getprime(primeindex++);
			np->temp1 = (np->temp1 & ~NETCELLCODE) | (cellcode << NETCELLCODESH);
			while (i+1 < cellcount && namesame(np->protoname, celllist[i+1]->protoname) == 0)
			{
				i++;
				np = celllist[i];
				np->temp1 = (np->temp1 & ~NETCELLCODE) | (cellcode << NETCELLCODESH);
			}
		}
		efree((CHAR *)celllist);
	}

	/* clear prime number indices in the cells */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp2 = 0;

	/* clear network topology information */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
				net->temp2 = 0;
		}
	}

	/* assign network topology based on export names */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		/* assign topology information uniformly in each cell */
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			/* first pass: ignore icon and skeleton views */
			if (np->cellview == el_iconview || np->cellview == el_skeletonview) continue;
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			{
				if (pp->network->temp2 != 0) continue;
				net_setthisexporttopology(pp, &np->temp2);
			}
			for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
			{
				if (net->temp2 == 0) net->temp2 = getprime(np->temp2++);
			}
		}
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			/* second pass: do icon and skeleton views */
			if (np->cellview != el_iconview && np->cellview != el_skeletonview) continue;
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			{
				if (pp->network->temp2 != 0) continue;
				net_setthisexporttopology(pp, &np->temp2);
			}
			for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
			{
				if (net->temp2 == 0) net->temp2 = getprime(np->temp2++);
			}
		}
	}
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			switch ((np->userbits&NFUNCTION) >> NFUNCTIONSH)
			{
				case NPTRANMOS:  case NPTRADMOS: case NPTRAPMOS:
				case NPTRANJFET: case NPTRAPJFET:
				case NPTRADMES:  case NPTRAEMES:
					pp = np->firstportproto;   pp->network->temp2 = getprime(0);	/* poly */
					pp = pp->nextportproto;    pp->network->temp2 = getprime(1);	/* active */
					pp = pp->nextportproto;    pp->network->temp2 = getprime(0);	/* poly */
					pp = pp->nextportproto;    pp->network->temp2 = getprime(1);	/* active */
					break;
				case NPTRANS:
					pp = np->firstportproto;   pp->network->temp2 = getprime(0);	/* poly */
					pp = pp->nextportproto;    pp->network->temp2 = getprime(1);	/* active */
					pp = pp->nextportproto;    pp->network->temp2 = getprime(1);	/* active */
					break;
				case NPTRANS4:
					pp = np->firstportproto;   pp->network->temp2 = getprime(0);	/* poly */
					pp = pp->nextportproto;    pp->network->temp2 = getprime(1);	/* active */
					pp = pp->nextportproto;    pp->network->temp2 = getprime(1);	/* active */
					pp = pp->nextportproto;    pp->network->temp2 = getprime(2);	/* bias */
					break;
				case NPTRANPN:   case NPTRAPNP:
					pp = np->firstportproto;   pp->network->temp2 = getprime(0);	/* collector */
					pp = pp->nextportproto;    pp->network->temp2 = getprime(1);	/* emitter */
					pp = pp->nextportproto;    pp->network->temp2 = getprime(2);	/* base */
					break;
				case NPRESIST:
				case NPCAPAC:   case NPECAPAC:
				case NPINDUCT:
					pp = np->firstportproto;   pp->network->temp2 = getprime(0);	/* side 1 */
					pp = pp->nextportproto;
					if (pp != NOPORTPROTO)     pp->network->temp2 = getprime(0);	/* side 2 */
					break;
				default:
					i = 0;
					for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
					{
						pp->network->temp2 = getprime(i);
						i++;
					}
					break;
			}
		}
	}
}

/*
 * Helper routine for "esort" that makes cells go in ascending name order.
 */
int net_cellnameascending(const void *e1, const void *e2)
{
	REGISTER NODEPROTO *c1, *c2;

	c1 = *((NODEPROTO **)e1);
	c2 = *((NODEPROTO **)e2);
	return(namesame(c1->protoname, c2->protoname));
}

void net_setthisexporttopology(PORTPROTO *pp, INTBIG *index)
{
	REGISTER NETWORK *net, *subnet;
	REGISTER PORTPROTO *opp;
	REGISTER NODEPROTO *np;
	REGISTER INTBIG i;
	REGISTER UINTBIG bits;
	REGISTER CHAR *name;

	/* assign the unique prime numbers to the export's network */
	net = pp->network;
	np = pp->parent;
	if (net->temp2 == 0)
	{
		net->temp2 = net_findotherexporttopology(np, pp->protoname);
		if (net->temp2 == 0)
		{
			/* this name not found: see if another name in this cell is connected */
			for(opp = np->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
			{
				if (opp == pp) continue;
				if (opp->network != pp->network) continue;
				net->temp2 = net_findotherexporttopology(np, opp->protoname);
				if (net->temp2 != 0) break;
			}

			/* if no name found, see if power/ground association can be made */
			if (net->temp2 == 0)
			{
				bits = pp->userbits & STATEBITS;
				if (bits == PWRPORT || bits == GNDPORT)
					net->temp2 = net_findotherexportcharacteristic(np, bits);
			}
			if (net->temp2 == 0)
				net->temp2 = getprime((*index)++);
		}
	}
	if (net->buswidth > 1)
	{
		for(i=0; i<net->buswidth; i++)
		{
			subnet = net->networklist[i];
			if (subnet->temp2 == 0)
			{
				if (subnet->namecount > 0) name = networkname(subnet, 0); else
					name = pp->protoname;
				subnet->temp2 = net_findotherexporttopology(np, name);
				if (subnet->temp2 == 0) subnet->temp2 = getprime((*index)++);
			}
		}
	}
}

/*
 * Helper routine to look at all cells associated with "parent" (i.e. in the same cell
 * but with a different view) and find an export with the name "exportname".  Returns
 * the "temp2" field (unique ID) of that network.  Returns zero if no association can be found.
 */
INTBIG net_findotherexporttopology(NODEPROTO *parent, CHAR *exportname)
{
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *opp;
	REGISTER NETWORK *net;
	REGISTER LIBRARY *lib;

	/* first look for an equivalent export in another cell */
	FOR_CELLGROUP(np, parent)
	{
		if (np == parent) continue;

		/* cell from the same cell: look for an equivalent port */
		opp = getportproto(np, exportname);
		if (opp != NOPORTPROTO && opp->network->temp2 != 0) return(opp->network->temp2);
	}

	/* next look for an equivalent network name in another cell */
	FOR_CELLGROUP(np, parent)
	{
		if (np == parent) continue;

		/* cell from the same cell: look for an equivalent network name */
		net = getnetwork(exportname, np);
		if (net == NONETWORK) continue;
		if (net->temp2 != 0) return(net->temp2);
	}

	/* now look for an equivalent in another library */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if (lib == parent->lib) continue;
		if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if ((np->temp1&NETCELLCODE) != (parent->temp1&NETCELLCODE)) continue;

			/* cell from the same cell in another library: look for an equivalent export or network name */
			opp = getportproto(np, exportname);
			if (opp != NOPORTPROTO && opp->network->temp2 != 0) return(opp->network->temp2);
			net = getnetwork(exportname, np);
			if (net == NONETWORK) continue;
			if (net->temp2 != 0) return(net->temp2);
		}
	}
	return(0);
}

/*
 * Helper routine to look at all cells associated with "parent" (i.e. in the same cell
 * but with a different view) and find a network with the characteristics "bits".  Returns
 * the "temp2" field (unique ID) of that network.  Returns zero if no association can be found.
 */
INTBIG net_findotherexportcharacteristic(NODEPROTO *parent, INTBIG bits)
{
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *opp;
	REGISTER NETWORK *net;
	REGISTER INTBIG obits;

	/* first look for an equivalent export in another cell */
	FOR_CELLGROUP(np, parent)
	{
		if (np == parent) continue;

		/* cell from the same cell: look for an equivalent port */
		for(opp = np->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
		{
			obits = opp->userbits & STATEBITS;
			if (bits != obits) continue;
			if (opp->network->temp2 != 0) return(opp->network->temp2);
		}
	}

	/* next look for an equivalent network name in another cell */
	FOR_CELLGROUP(np, parent)
	{
		if (np == parent) continue;

		/* cell from the same cell: look for an equivalent network name */
		for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		{
			if (net->globalnet < 0) continue;
			if (net->globalnet >= np->globalnetcount) continue;
			if (np->globalnetchar[net->globalnet] != bits) continue;
			if (net->temp2 != 0) return(net->temp2);
		}
	}
	return(0);
}
