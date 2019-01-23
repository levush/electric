/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: netdiff.cpp
 * Network tool: module for network comparison
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
 *
 * This module is inspired by the work of Carl Ebeling:
 *   Ebeling, Carl, "GeminiII: A Second Generation Layout Validation Program",
 *   Proceedings of ICCAD 1988, p322-325.
 */

#include "global.h"
#include "network.h"
#include "efunction.h"
#include "egraphics.h"
#include "edialogs.h"
#include "tecschem.h"
#include "usr.h"
#include <math.h>

#define NEWNCC	1		/* comment out to remove last-minute changes */
#define STATIC

#define MAXITERATIONS 10
#define SYMGROUPCOMP   0
#define SYMGROUPNET    1

/* the meaning of errors returned by "net_analyzesymmetrygroups()" */
#define SIZEERRORS       1
#define EXPORTERRORS     2
#define STRUCTUREERRORS  4

static GRAPHICS net_cleardesc = {LAYERH, ALLOFF, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
	0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};
static GRAPHICS net_msgdesc = {LAYERH, HIGHLIT, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
	0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};

#define NOSYMGROUP ((SYMGROUP *)-1)

/* the meaning of "SYMGROUP->groupflags" */
#define GROUPACTIVENOW		1					/* set if this group is active now */
#define GROUPFRESH	        2					/* set if this group is "fresh" (just split) */
#define GROUPPROMISING		4					/* set if this group is "promising" (refed from split group) */

typedef struct Isymgroup
{
	HASHTYPE          hashvalue;			/* hash value of this symmetry group */
	INTBIG            grouptype;			/* SYMGROUPCOMP (components) or SYMGROUPNET (nets) */
	INTBIG            groupflags;			/* state of this group */
	INTBIG            groupindex;			/* ordinal value of group */
	INTBIG            checksum;				/* additional value for group to ensure hash codes don't clash */
	INTBIG            cellcount[2];			/* number of objects from cells */
	INTBIG            celltotal[2];			/* size of object list from cells */
	void            **celllist[2];			/* list of objects from cells */
	struct Isymgroup *nextsymgroup;			/* next in list */
	struct Isymgroup *nexterrsymgroup;		/* next in list of errors */
} SYMGROUP;

static SYMGROUP    *net_firstsymgroup = NOSYMGROUP;
static SYMGROUP    *net_firstmatchedsymgroup = NOSYMGROUP;
static SYMGROUP    *net_symgroupfree = NOSYMGROUP;

static SYMGROUP   **net_symgrouphashcomp;			/* hash table for components */
static SYMGROUP   **net_symgrouphashnet;			/* hash table for nets */
static INTBIG      *net_symgrouphashckcomp;			/* hash table checksums for components */
static INTBIG      *net_symgrouphashcknet;			/* hash table checksums for nets */
static INTBIG       net_symgrouphashcompsize = 0;	/* size of component hash table */
static INTBIG       net_symgrouphashnetsize = 0;	/* size of net hash table */
static INTBIG       net_symgrouplisttotal = 0;
static INTBIG       net_symgroupnumber;
static SYMGROUP   **net_symgrouplist;
static INTBIG       net_nodeCountMultiplier = 0;
static INTBIG       net_portFactorMultiplier;
static INTBIG       net_portNetFactorMultiplier;
static INTBIG       net_portHashFactorMultiplier;
static INTBIG       net_functionMultiplier;
static PNET        *net_nodelist1 = NOPNET, *net_nodelist2 = NOPNET;
static INTBIG       net_ncc_tolerance;				/* component value tolerance (%) */
static INTBIG       net_ncc_tolerance_amt;			/* component value tolerance (amt) */
static HASHTYPE     net_uniquehashvalue;
static BOOLEAN      net_nethashclashtold;
static BOOLEAN      net_comphashclashtold;

static INTBIG       net_debuggeminipasscount;		/* number of passes through gemini renumbering */
static INTBIG       net_debuggeminiexpandglobal;	/* number of times entire set of groups considered */
static INTBIG       net_debuggeminiexpandglobalworked;	/* number of times entire group considereation worked */
static INTBIG       net_debuggeminigroupsrenumbered;/* number of symmetry groups renumbered */
static INTBIG       net_debuggeminigroupssplit;		/* number of symmetry groups split */
static INTBIG       net_debuggeminitotalgroups;		/* total number of groups examined */
static INTBIG       net_debuggeminitotalgroupsact;	/* number of active groups examined */

/* structures for name matching */
typedef struct
{
	CHAR     *name;
	INTBIG    number;
	NODEINST *original;
} NAMEMATCH;

static NAMEMATCH   *net_namematch[2];
static INTBIG       net_namematchtotal[2] = {0, 0};
static INTBIG      *net_compmatch0list;
static INTBIG      *net_compmatch1list;
static INTBIG       net_compmatch0total = 0;
static INTBIG       net_compmatch1total = 0;

static INTBIG       net_foundsymgrouptotal = 0;
static INTBIG       net_foundsymgroupcount;
static SYMGROUP   **net_foundsymgroups;

/* structures for size matching */
typedef struct
{
	float length, width;
} NODESIZE;

static INTBIG    net_sizearraytotal[2] = {0, 0};
static NODESIZE *net_sizearray[2];

/* used by "netanalyze.c" */
#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
extern "C" {
#endif
       PCOMP       *net_pcomp1 = NOPCOMP, *net_pcomp2 = NOPCOMP;
       INTBIG       net_timestamp;
       NODEPROTO   *net_cell[2];
       INTBIG       net_ncc_options;				/* options to use in NCC */
extern GRAPHICS     us_hbox;
#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif

/* prototypes for local routines */
STATIC void       net_addcomptoerror(void *err, PCOMP *pc);
STATIC void       net_addnettoerror(void *err, PNET *pn);
STATIC void       net_addsymgrouptoerror(void *err, SYMGROUP *sg);
STATIC void       net_addtonamematch(NAMEMATCH **match, INTBIG *total, INTBIG *count, CHAR *name,
					INTBIG number, NODEINST *orig);
STATIC BOOLEAN    net_addtosymgroup(SYMGROUP *sg, INTBIG cellno, void *obj);
STATIC INTBIG     net_analyzesymmetrygroups(BOOLEAN reporterrors, BOOLEAN checksize,
					BOOLEAN checkexportnames, BOOLEAN ignorepwrgnd, INTBIG *errorcount);
STATIC INTBIG     net_assignnewgrouphashvalues(SYMGROUP *sg, INTBIG verbose);
STATIC INTBIG     net_assignnewhashvalues(INTBIG grouptype);
STATIC CHAR      *net_describesizefactor(float sizew, float sizel);
STATIC INTBIG     net_dogemini(PCOMP *pcomp1, PNET *nodelist1, PCOMP *pcomp2, PNET *nodelist2,
					BOOLEAN checksize, BOOLEAN checkexportnames, BOOLEAN ignorepwrgnd);
STATIC INTBIG     net_findamatch(INTBIG verbose, BOOLEAN ignorepwrgnd);
STATIC BOOLEAN    net_findcommonsizefactor(SYMGROUP *sg, float *sizew, float *sizel);
STATIC BOOLEAN    net_findcomponentnamematch(SYMGROUP *sg, BOOLEAN usenccmatches,
					INTBIG verbose, BOOLEAN ignorepwrgnd, INTBIG total, INTBIG unmatchednets, INTBIG unmatchedcomps);
STATIC BOOLEAN    net_findexportnamematch(SYMGROUP *sg, INTBIG verbose, BOOLEAN ignorepwrgnd,
					INTBIG total, INTBIG unmatchednets, INTBIG unmatchedcomps);
STATIC SYMGROUP **net_findgeomsymmetrygroup(GEOM *obj);
STATIC SYMGROUP **net_findnetsymmetrygroup(NETWORK *net);
STATIC BOOLEAN    net_findnetworknamematch(SYMGROUP *sg, BOOLEAN usenccmatches,
					INTBIG verbose, BOOLEAN ignorepwrgnd, INTBIG total, INTBIG unmatchednets, INTBIG unmatchedcomps);
STATIC BOOLEAN    net_findpowerandgroundmatch(SYMGROUP *sg, INTBIG verbose, INTBIG total, INTBIG unmatchednets, INTBIG unmatchedcomps);
STATIC SYMGROUP  *net_findsymmetrygroup(INTBIG grouptype, HASHTYPE hashvalue, INTBIG checksum);
STATIC void       net_forceamatch(SYMGROUP *sg, INTBIG c1, INTBIG *i1, INTBIG c2, INTBIG *i2,
					float sizew, float sizel, INTBIG verbose, BOOLEAN ignorepwrgnd);
STATIC void       net_freesymgroup(SYMGROUP *sg);
STATIC void       net_initializeverbose(PCOMP *pcomplist, PNET *pnetlist);
STATIC BOOLEAN    net_insertinhashtable(SYMGROUP *sg);
STATIC BOOLEAN    net_isspice(PCOMP *pc);
STATIC INTBIG     net_ncconelevel(NODEPROTO *cell1, NODEPROTO *cell2, BOOLEAN preanalyze, 
					BOOLEAN interactive);
STATIC SYMGROUP  *net_newsymgroup(INTBIG type, HASHTYPE hashvalue, INTBIG checksum);
STATIC void       net_preserveresults(NODEPROTO *np1, NODEPROTO *np2);
STATIC UINTBIG    net_recursiverevisiondate(NODEPROTO *cell);
STATIC void       net_rebuildhashtable(void);
STATIC void       net_redeemzerogroups(SYMGROUP *sgnewc, SYMGROUP *sgnewn, INTBIG verbose, BOOLEAN ignorepwrgnd);
STATIC void       net_removefromsymgroup(SYMGROUP *sg, INTBIG f, INTBIG index);
STATIC INTBIG     net_reporterror(SYMGROUP *sg, CHAR *errmsg, BOOLEAN ignorepwrgnd);
STATIC void       net_reportsizeerror(PCOMP *pc1, CHAR *size1, PCOMP *pc2, CHAR *size2, INTBIG pctdiff, SYMGROUP *sg);
STATIC BOOLEAN    net_sameexportnames(PNET *pn1, PNET *pn2);
STATIC void       net_showpreanalysis(NODEPROTO *cell1, PCOMP *pcomp1, PNET *nodelist1,
					NODEPROTO *cell2, PCOMP *pcomp2, PNET *nodelist2, BOOLEAN ignorepwrgnd);
STATIC void       net_showsymmetrygroups(INTBIG verbose, INTBIG type);
STATIC HASHTYPE   net_uniquesymmetrygrouphash(INTBIG grouptype);
STATIC void       net_unmatchedstatus(INTBIG *unmatchednets, INTBIG *unmatchedcomps, INTBIG *symgroupcount);
STATIC void       net_showmatchedgroup(SYMGROUP *sg);
STATIC void       net_addtofoundsymgroups(SYMGROUP *sg);
STATIC int        net_sortbycelltype(const void *n1, const void *n2);
STATIC int        net_sortbypnet(const void *n1, const void *n2);
STATIC int        net_sortexportcodes(const void *e1, const void *e2);
STATIC void       net_checkcomponenttypes(void *errorsa, BOOLEAN ignorepwrgnd, PCOMP *pcomp1, PCOMP *pcomp2,
					PNET *pnetlist1, PNET *pnetlist2, NODEPROTO *cell1, NODEPROTO *cell2);
STATIC NETWORK  **net_makeexternalnetlist(NODEINST *ni, INTBIG *size, BOOLEAN ignorepwrgnd);
STATIC void       net_randomizehashcodes(INTBIG verbose);
STATIC void       net_randomizesymgroup(SYMGROUP *sg, INTBIG verbose, INTBIG factor);
STATIC void       net_cleanupsymmetrygroups(void);
STATIC void       net_foundmismatch(NODEPROTO *cell, NETWORK *net, NODEPROTO *cellwithout, PCOMP **celllist,
					INTBIG start, INTBIG end, PNET *pnetlist, BOOLEAN ignorepwrgnd, void *errorsa);
#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
extern "C"
{
#endif
	STATIC int    net_sortnamematches(const void *e1, const void *e2);
	STATIC int    net_sortpcomp(const void *e1, const void *e2);
	STATIC int    net_sortpnet(const void *e1, const void *e2);
	STATIC int    net_sortsizearray(const void *e1, const void *e2);
	STATIC int    net_sortsymgroups(const void *e1, const void *e2);
#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif

/*
 * Routine to free all memory associated with this module.
 */
void net_freediffmemory(void)
{
	REGISTER SYMGROUP *sg;
	REGISTER INTBIG f;

	net_removeassociations();
	while (net_firstsymgroup != NOSYMGROUP)
	{
		sg = net_firstsymgroup;
		net_firstsymgroup = sg->nextsymgroup;
		net_freesymgroup(sg);
	}
	while (net_firstmatchedsymgroup != NOSYMGROUP)
	{
		sg = net_firstmatchedsymgroup;
		net_firstmatchedsymgroup = sg->nextsymgroup;
		net_freesymgroup(sg);
	}
	if (net_symgrouphashcompsize != 0)
	{
		efree((CHAR *)net_symgrouphashcomp);
		efree((CHAR *)net_symgrouphashckcomp);
	}
	if (net_symgrouphashnetsize != 0)
	{
		efree((CHAR *)net_symgrouphashnet);
		efree((CHAR *)net_symgrouphashcknet);
	}
	while (net_symgroupfree != NOSYMGROUP)
	{
		sg = net_symgroupfree;
		net_symgroupfree = sg->nextsymgroup;
		for(f=0; f<2; f++)
			if (sg->celltotal[f] > 0) efree((CHAR *)sg->celllist[f]);
		efree((CHAR *)sg);
	}
	if (net_symgrouplisttotal > 0) efree((CHAR *)net_symgrouplist);
	if (net_foundsymgrouptotal > 0) efree((CHAR *)net_foundsymgroups);

	for(f=0; f<2; f++)
	{
		if (net_namematchtotal[f] > 0) efree((CHAR *)net_namematch[f]);
		if (net_sizearraytotal[f] > 0) efree((CHAR *)net_sizearray[f]);
	}
	if (net_compmatch0total > 0) efree((CHAR *)net_compmatch0list);
	if (net_compmatch1total > 0) efree((CHAR *)net_compmatch1list);
#ifdef FORCESUNTOOLS
	net_freeexpdiffmemory();
#endif
}

void net_removeassociations(void)
{
	if (net_pcomp1 != NOPCOMP)
	{
		net_freeallpcomp(net_pcomp1);
		net_pcomp1 = NOPCOMP;
	}
	if (net_pcomp2 != NOPCOMP)
	{
		net_freeallpcomp(net_pcomp2);
		net_pcomp2 = NOPCOMP;
	}
	if (net_nodelist1 != NOPNET)
	{
		net_freeallpnet(net_nodelist1);
		net_nodelist1 = NOPNET;
	}
	if (net_nodelist2 != NOPNET)
	{
		net_freeallpnet(net_nodelist2);
		net_nodelist2 = NOPNET;
	}
}

/******************************** EQUATING COMPARED OBJECTS ********************************/

/*
 * routine to identify the equivalent object associated with the currently
 * highlighted one (comparison must have been done).  If "noise" is true,
 * report errors.  Returns false if an equate was shown.
 */
BOOLEAN net_equate(BOOLEAN noise)
{
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni;
	REGISTER NETWORK *net, *anet;
	REGISTER PCOMP *pc;
	REGISTER PNET *pn;
	REGISTER GEOM *obj;
	REGISTER SYMGROUP *sg, **sglist;
	REGISTER INTBIG i, j, f, k, fun;
	REGISTER BOOLEAN first;
	REGISTER void *infstr;

	/* make sure an association has been done */
#ifdef NEWNCC
	if (net_pcomp1 == NOPCOMP && net_pcomp2 == NOPCOMP && net_nodelist1 == NOPNET && net_nodelist2 == NOPNET)
#else
	if (net_pcomp1 == NOPCOMP || net_pcomp2 == NOPCOMP)
#endif
	{
		if (noise) ttyputerr(_("First associate with '-telltool network compare'"));
		return(TRUE);
	}

	/* get the highlighted object */
	obj = (GEOM *)asktool(us_tool, x_("get-object"));
	if (obj == NOGEOM)
	{
		if (noise) ttyputerr(_("Must select something to be equated"));
		return(TRUE);
	}

	/* make sure this object is in one of the associated cells */
	np = geomparent(obj);
	if (np != net_cell[0] && np != net_cell[1])
	{
		if (!isachildof(np, net_cell[0]) && !isachildof(np, net_cell[1]))
		{
			if (noise)
				ttyputerr(_("This object is not in one of the two associated cells"));
			return(TRUE);
		}
	}

	/* highlight the associated object */
	sglist = net_findgeomsymmetrygroup(obj);
	if (sglist[0] == NOSYMGROUP)
	{
#ifdef FORCESUNTOOLS
		return(net_equateexp(noise));
#endif
		ttyputmsg(_("This object is not associated with anything else"));
		return(TRUE);
	}
	if (sglist[1] == NOSYMGROUP && sglist[0]->hashvalue == 0)
	{
		ttyputmsg(_("This object was not matched successfully"));
		return(TRUE);
	}

	(void)asktool(us_tool, x_("clear"));
	infstr = initinfstr();
	first = FALSE;
	for(k=0; sglist[k] != NOSYMGROUP; k++)
	{
		sg = sglist[k];
		switch (sg->grouptype)
		{
			case SYMGROUPCOMP:
				for(f=0; f<2; f++)
				{
					for(i=0; i<sg->cellcount[f]; i++)
					{
						pc = (PCOMP *)sg->celllist[f][i];
						for(j=0; j<pc->numactual; j++)
						{
							if (pc->numactual == 1) ni = (NODEINST *)pc->actuallist; else
								ni = ((NODEINST **)pc->actuallist)[j];
							if (first) addtoinfstr(infstr, '\n');
							first = TRUE;
							formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0"),
								describenodeproto(geomparent(ni->geom)), (INTBIG)ni->geom);
						}
					}
				}
				break;
			case SYMGROUPNET:
				for(f=0; f<2; f++)
				{
					for(i=0; i<sg->cellcount[f]; i++)
					{
						pn = (PNET *)sg->celllist[f][i];
						net = pn->network;
						if (net == NONETWORK) continue;
						np = net->parent;
						for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
						{
							anet = ai->network;
							if (ai->proto == sch_busarc)
							{
								if (anet->buswidth > 1)
								{
#ifdef NEWNCC  
									for(j=0; j<anet->buswidth; j++)
										if (anet->networklist[j] == net) break;
									if (j >= anet->buswidth) continue;
#else
									for(i=0; i<anet->buswidth; i++)
										if (anet->networklist[i] == net) break;
									if (i >= anet->buswidth) continue;
#endif
								} else
								{
									if (anet != net) continue;
								}
							} else
							{
								if (anet != net) continue;
							}
							if (first) addtoinfstr(infstr, '\n');
							first = TRUE;
							formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0"),
								describenodeproto(np), (INTBIG)ai->geom);
						}
						for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
						{
							if (ni->proto->primindex == 0) continue;
							fun = nodefunction(ni);
							if (fun != NPPIN && fun != NPCONTACT && fun != NPNODE && fun != NPCONNECT)
								continue;
							if (ni->firstportarcinst == NOPORTARCINST) continue;
							if (ni->firstportarcinst->conarcinst->network != net) continue;
							if (first) addtoinfstr(infstr, '\n');
							first = TRUE;
							formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0"),
								describenodeproto(np), (INTBIG)ni->geom);
						}
						for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
						{
							if (pp->network != net) continue;
							if (first) addtoinfstr(infstr, '\n');
							first = TRUE;
							formatinfstr(infstr, x_("CELL=%s TEXT=0%lo;0%lo;-"),
								describenodeproto(np), (INTBIG)pp->subnodeinst->geom, (INTBIG)pp);
						}
					}
				}
		}
	}
	(void)asktool(us_tool, x_("show-multiple"), (INTBIG)returninfstr(infstr));
	return(FALSE);
}

/*
 * Routine to return equivalent network(s) associated with passed net
 * (comparison must have been done).  Returns 0 if no equivalent net
 * could be found.  Any equivalent networks found are put in **equiv, with array
 * size *numequiv, and returns 1.  Returns -1 if no NCC data.
 */
INTBIG net_getequivalentnet(NETWORK *net, NETWORK ***equiv, INTBIG *numequiv, BOOLEAN allowchild)
{
	REGISTER SYMGROUP *sg, **sglist;
	REGISTER PNET *pn;
	REGISTER NETWORK **newequiv;
	REGISTER INTBIG f, i, k;
	REGISTER BOOLEAN done;

	if (net == NONETWORK) return(0);

	/* make sure an association has been done */
#ifdef NEWNCC
	if (net_pcomp1 == NOPCOMP && net_pcomp2 == NOPCOMP && net_nodelist1 == NOPNET && net_nodelist2 == NOPNET)
		return(-1);
#else
	if (net_pcomp1 == NOPCOMP || net_pcomp2 == NOPCOMP) return(-1);
#endif

	/* make sure ni is in one of the associated cells */
	if (net->parent != net_cell[0] && net->parent != net_cell[1]) 
	{
		if (!allowchild) return(-1);
		if (!isachildof(net->parent, net_cell[0]) && !isachildof(net->parent, net_cell[1]))
			return(-1);
	}

	sglist = net_findnetsymmetrygroup(net);
	sg = sglist[0];
	if (sg == NOSYMGROUP) return(0);
	if (sglist[1] != NOSYMGROUP) return(0);
	if (sg->hashvalue == 0) return(0);
	if (sg->grouptype != SYMGROUPNET) return(0);

	/* find group for net */
	done = FALSE;
	for (f=0; f<2; f++)
	{
		for (i=0; i<sg->cellcount[f]; i++)
		{
			pn = (PNET *)sg->celllist[f][i];
			if( net == pn->network) done = TRUE;
			if(done) break;
		}
		if(done) break;
	}
	if (f==2) return(0);
	if (f==1) f = 0; else f = 1;

	/* build equivalent nodeinst list */
	*numequiv = 0;
	for (i=0; i<sg->cellcount[f]; i++)
	{
		pn = (PNET *)sg->celllist[f][i];
		newequiv = (NETWORK **)emalloc((*numequiv + 1)*(sizeof(NETWORK *)), el_tempcluster);
		for (k=0; k<(*numequiv); k++)
			newequiv[k] = (*equiv)[k];
		newequiv[*numequiv] = pn->network;
		if( *numequiv > 0) efree((CHAR *)(*equiv));
		*equiv = newequiv;
		(*numequiv)++;
	}
	return(TRUE);
}

/*
 * Routine to find the symmetry groups associated with object "obj".
 * Returns an array of symmetry groups.  The first element is NOSYMGROUP
 * if there are no associated objects.
 */
SYMGROUP **net_findgeomsymmetrygroup(GEOM *obj)
{
	REGISTER SYMGROUP *sg;
	REGISTER INTBIG f, i, j, fun;
	REGISTER PCOMP *pc;
	REGISTER NODEINST *ni, *wantni;
	REGISTER ARCINST *ai;

	if (obj->entryisnode)
	{
		/* look for a node */
		wantni = obj->entryaddr.ni;
		for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
		{
			if (sg->grouptype != SYMGROUPCOMP) continue;
			for(f=0; f<2; f++)
			{
				for(i=0; i<sg->cellcount[f]; i++)
				{
					pc = (PCOMP *)sg->celllist[f][i];
					for(j=0; j<pc->numactual; j++)
					{
						if (pc->numactual == 1) ni = (NODEINST *)pc->actuallist; else
							ni = ((NODEINST **)pc->actuallist)[j];
						if (ni == wantni)
						{
							net_foundsymgroupcount = 0;
							net_addtofoundsymgroups(sg);
							net_addtofoundsymgroups(NOSYMGROUP);
							return(net_foundsymgroups);
						}
					}
				}
			}
		}

		/* node not found, try network coming out of it */
		fun = nodefunction(wantni);
		if (fun == NPPIN || fun == NPCONTACT || fun == NPCONNECT)
		{
			if (wantni->firstportarcinst != NOPORTARCINST)
				obj = wantni->firstportarcinst->conarcinst->geom;
		}
		if (obj->entryisnode)
		{
			/* return a null list */
			net_foundsymgroupcount = 0;
			net_addtofoundsymgroups(NOSYMGROUP);
			return(net_foundsymgroups);
		}
	}

	/* look for an arc */
	ai = obj->entryaddr.ai;
	return(net_findnetsymmetrygroup(ai->network));
}

/*
 * Routine to find the symmetry groups associated with network "net".
 * Returns an array of symmetry groups.  The first element is NOSYMGROUP
 * if there are no associated objects.
 */
SYMGROUP **net_findnetsymmetrygroup(NETWORK *net)
{
	REGISTER SYMGROUP *sg;
	REGISTER INTBIG f, i, k;
	REGISTER PNET *pn;
	REGISTER NETWORK *subnet;

	net_foundsymgroupcount = 0;
	for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
	{
		if (sg->grouptype != SYMGROUPNET) continue;
		for(f=0; f<2; f++)
		{
			for(i=0; i<sg->cellcount[f]; i++)
			{
				pn = (PNET *)sg->celllist[f][i];
				if (pn->network == net)
				{
					net_addtofoundsymgroups(sg);
					net_addtofoundsymgroups(NOSYMGROUP);
					return(net_foundsymgroups);
				}
			}
		}
	}

	/* if this is a bus, load up all signals on it */
	if (net->buswidth > 1)
	{
		for(k=0; k<net->buswidth; k++)
		{
			subnet = net->networklist[k];
			for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
			{
				if (sg->grouptype != SYMGROUPNET) continue;
				for(f=0; f<2; f++)
				{
					for(i=0; i<sg->cellcount[f]; i++)
					{
						pn = (PNET *)sg->celllist[f][i];
						if (pn->network == subnet)
						{
							net_addtofoundsymgroups(sg);
							break;
						}
					}
					if (i < sg->cellcount[f]) break;
				}
				if (f < 2) break;
			}
		}
	}
	net_addtofoundsymgroups(NOSYMGROUP);
	return(net_foundsymgroups);
}

/*
 * Routine to add symmetry group "sg" to the global array "net_foundsymgroups".
 */
void net_addtofoundsymgroups(SYMGROUP *sg)
{
	REGISTER INTBIG newtotal, i;
	REGISTER SYMGROUP **newlist;

	if (net_foundsymgroupcount >= net_foundsymgrouptotal)
	{
		newtotal = net_foundsymgrouptotal * 2;
		if (net_foundsymgroupcount >= newtotal) newtotal = net_foundsymgroupcount + 5;
		newlist = (SYMGROUP **)emalloc(newtotal * (sizeof (SYMGROUP *)), net_tool->cluster);
		if (newlist == 0) return;
		for(i=0; i<net_foundsymgroupcount; i++)
			newlist[i] = net_foundsymgroups[i];
		if (net_foundsymgrouptotal > 0)
			efree((CHAR *)net_foundsymgroups);
		net_foundsymgroups = newlist;
		net_foundsymgrouptotal = newtotal;
	}
	net_foundsymgroups[net_foundsymgroupcount++] = sg;
}

/******************************** COMPARISON ********************************/

/*
 * routine to compare the two networks in "cell1" and "cell2" (if they are NONODEPROTO,
 * use the two cells on the screen).  If "preanalyze" is
 * true, only do preanalysis and display results.
 * Returns FALSE if the cells match.
 */
BOOLEAN net_compare(BOOLEAN preanalyze, BOOLEAN interactive, NODEPROTO *cell1, NODEPROTO *cell2)
{
	REGISTER INTBIG ret, resignore;
	REGISTER VARIABLE *var;
	REGISTER NODEPROTO *np;
	REGISTER LIBRARY *lib;
	REGISTER BOOLEAN backannotate;
	CHAR *respar[2];

	backannotate = FALSE;

	/* make sure network tool is on */
	if ((net_tool->toolstate&TOOLON) == 0)
	{
		ttyputerr(_("Network tool must be running...turning it on for you"));
		toolturnon(net_tool);
		return(TRUE);
	}

	if (cell1 == NONODEPROTO || cell2 == NONODEPROTO)
	{
		if (net_getcells(&cell1, &cell2))
		{
			ttyputerr(_("Must have two windows with two different cells"));
			return(TRUE);
		}
	}

	/* if the top cells are already checked, stop now */
	if (net_nccalreadydone(cell1, cell2))
	{
		ttyputmsg(_("Cells are already checked"));
		return(FALSE);
	}

	/* get options to use during comparison */
	var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_ncc_optionskey);
	if (var == NOVARIABLE) net_ncc_options = 0; else
		net_ncc_options = var->addr;

	/* see if there are any resistors in this circuit */
	if ((net_ncc_options&NCCRESISTINCLUSION) != NCCRESISTLEAVE)
	{
		resignore = asktech(sch_tech, x_("ignoring-resistor-topology"));
		if (resignore != 0 && (net_ncc_options&NCCRESISTINCLUSION) == NCCRESISTINCLUDE)
		{
			/* must redo network topology to include resistors */
			respar[0] = x_("resistors");
			respar[1] = x_("include");
			(void)telltool(net_tool, 2, respar);
		} else if (resignore == 0 && (net_ncc_options&NCCRESISTINCLUSION) == NCCRESISTEXCLUDE)
		{
			/* must redo network topology to exclude resistors */
			respar[0] = x_("resistors");
			respar[1] = x_("ignore");
			(void)telltool(net_tool, 2, respar);
		}
	}

	starttimer();
	if (preanalyze) ttyputmsg(_("Analyzing..."));

	/* reset the random number generator so that the results are repeatable */
	srand(1);
	net_nethashclashtold = FALSE;
	net_comphashclashtold = FALSE;

	var = getvalkey((INTBIG)net_tool, VTOOL, -1, net_ncc_comptolerancekey);
	if (var == NOVARIABLE) net_ncc_tolerance = 0; else
	{
		if ((var->type&VTYPE) == VINTEGER) net_ncc_tolerance = var->addr * WHOLE; else
			if ((var->type&VTYPE) == VFRACT) net_ncc_tolerance = var->addr;
	}
	var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_ncc_comptoleranceamtkey);
	if (var == NOVARIABLE) net_ncc_tolerance_amt = 0; else net_ncc_tolerance_amt = var->addr;

	/* mark all cells as not-checked and name all nets */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			np->temp1 = (np->temp1 & ~NETNCCCHECKSTATE) | NETNCCNOTCHECKED;
			if (asktool(net_tool, x_("name-nets"), (INTBIG)np) != 0) backannotate = TRUE;
		}
	}
	
	ret = net_ncconelevel(cell1, cell2, preanalyze, interactive);

	if (backannotate)
		ttyputmsg(_("Back-annotation information has been added (library must be saved)"));

	return(ret!=0 ? TRUE : FALSE);
}

/* NCC warning */
static DIALOGITEM net_nccwarndialogitems[] =
{
 /*  1 */ {0, {276,340,300,480}, BUTTON, N_("Show Preanalysis")},
 /*  2 */ {0, {244,340,268,480}, BUTTON, N_("Stop Now")},
 /*  3 */ {0, {308,340,332,480}, BUTTON, N_("Do Full NCC")},
 /*  4 */ {0, {8,8,24,512}, MESSAGE, x_("")},
 /*  5 */ {0, {28,8,236,512}, SCROLL, x_("")},
 /*  6 */ {0, {248,56,264,332}, MESSAGE, N_("You may stop the NCC now:")},
 /*  7 */ {0, {280,56,296,332}, MESSAGE, N_("You may request additional detail:")},
 /*  8 */ {0, {312,56,328,332}, MESSAGE, N_("You may continue with NCC:")}
};
static DIALOG net_nccwarndialog = {{75,75,416,597}, N_("NCC Differences Have Been Found"), 0, 8, net_nccwarndialogitems, 0, 0};

/* special items for the "NCC warning" dialog: */
#define DNCW_DOPREANALYSIS       1		/* Do preanalysis (button) */
#define DNCW_STOPNOW             2		/* Stop now (button) */
#define DNCW_DONCC               3		/* Do NCC (button) */
#define DNCW_TITLE               4		/* Title line (stat text) */
#define DNCW_DIFFLIST            5		/* List of differences (scroll) */

/*
 * Routine to compare "cell1" and "cell2".
 * If "preanalyze" is TRUE, only do preanalysis.
 * If "interactive" is TRUE, do interactive comparison.
 * Returns 0 if they compare, 1 if they do not compare, -1 on error.
 */
INTBIG net_ncconelevel(NODEPROTO *cell1, NODEPROTO *cell2, BOOLEAN preanalyze, BOOLEAN interactive)
{
	INTBIG comp1, comp2, power1, power2, ground1, ground2, netcount1, netcount2,
		unmatchednets, unmatchedcomps, prevunmatchednets, prevunmatchedcomps, errors,
		symgroupcount, net1remove, net2remove, errorcount;
	REGISTER INTBIG i, f, ocomp1, ocomp2, verbose, buscount1, buscount2, ret, itemHit;
	BOOLEAN hierarchical, ignorepwrgnd, mergeparallel, mergeseries, recurse,
		checkexportnames, checksize, localmergeseries1, localmergeseries2,
		localmergeparallel1, localmergeparallel2, subcellsbad,
		localhierarchical1, localhierarchical2, figuresizes;
	float elapsed;
	REGISTER CHAR *errortype, **errorstrings;
	REGISTER WINDOWPART *w, *savecurw;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *subnp1, *subnp2;
	REGISTER NETWORK *net;
	REGISTER VARIABLE *var;
	REGISTER PCOMP *pc, *opc;
	REGISTER PNET *pn;
	REGISTER SYMGROUP *sg;
	REGISTER void *infstr, *dia, *errorsa;

	/* make sure prime multipliers are computed */
	net_initdiff();

	/* stop if already checked */
	if ((cell1->temp1&NETNCCCHECKSTATE) == NETNCCCHECKEDGOOD &&
		(cell2->temp1&NETNCCCHECKSTATE) == NETNCCCHECKEDGOOD) return(0);
	if ((cell1->temp1&NETNCCCHECKSTATE) != NETNCCNOTCHECKED ||
		(cell2->temp1&NETNCCCHECKSTATE) != NETNCCNOTCHECKED) return(1);
	if (net_nccalreadydone(cell1, cell2))
	{
		cell1->temp1 = (cell1->temp1 & ~NETNCCCHECKSTATE) | NETNCCCHECKEDGOOD;
		cell2->temp1 = (cell2->temp1 & ~NETNCCCHECKSTATE) | NETNCCCHECKEDGOOD;
		return(0);
	}

	verbose = net_ncc_options & (NCCVERBOSETEXT | NCCVERBOSEGRAPHICS);
	if ((net_ncc_options&NCCRECURSE) != 0) recurse = TRUE; else
		recurse = FALSE;
	if ((net_ncc_options&NCCHIERARCHICAL) != 0) hierarchical = TRUE; else
		hierarchical = FALSE;
	if ((net_ncc_options&NCCIGNOREPWRGND) != 0) ignorepwrgnd = TRUE; else
		ignorepwrgnd = FALSE;
	if ((net_ncc_options&NCCNOMERGEPARALLEL) == 0) mergeparallel = TRUE; else
		mergeparallel = FALSE;
	if ((net_ncc_options&NCCMERGESERIES) != 0) mergeseries = TRUE; else
		mergeseries = FALSE;
	if ((net_ncc_options&NCCCHECKEXPORTNAMES) != 0) checkexportnames = TRUE; else
		checkexportnames = FALSE;
	if ((net_ncc_options&NCCCHECKSIZE) != 0) checksize = TRUE; else
		checksize = FALSE;
	figuresizes = TRUE;
	if (preanalyze) figuresizes = FALSE;
	if (!checksize) figuresizes = FALSE;

	/* check for cell overrides */
	localhierarchical1 = localhierarchical2 = hierarchical;
	localmergeparallel1 = localmergeparallel2 = mergeparallel;
	localmergeseries1 = localmergeseries2 = mergeseries;
	var = getvalkey((INTBIG)cell1, VNODEPROTO, VINTEGER, net_ncc_optionskey);
	if (var != NOVARIABLE)
	{
		if ((var->addr&NCCHIERARCHICALOVER) != 0)
		{
			if ((var->addr&NCCHIERARCHICAL) != 0) localhierarchical1 = TRUE; else
				localhierarchical1 = FALSE;
		}
		if ((var->addr&NCCNOMERGEPARALLELOVER) != 0)
		{
			if ((var->addr&NCCNOMERGEPARALLEL) == 0) localmergeparallel1 = TRUE; else
				localmergeparallel1 = FALSE;
		}
		if ((var->addr&NCCMERGESERIESOVER) != 0)
		{
			if ((var->addr&NCCMERGESERIES) != 0) localmergeseries1 = TRUE; else
				localmergeseries1 = FALSE;
		}
	}
	var = getvalkey((INTBIG)cell2, VNODEPROTO, VINTEGER, net_ncc_optionskey);
	if (var != NOVARIABLE)
	{
		if ((var->addr&NCCHIERARCHICALOVER) != 0)
		{
			if ((var->addr&NCCHIERARCHICAL) != 0) localhierarchical2 = TRUE; else
				localhierarchical2 = FALSE;
		}
		if ((var->addr&NCCNOMERGEPARALLELOVER) != 0)
		{
			if ((var->addr&NCCNOMERGEPARALLEL) == 0) localmergeparallel2 = TRUE; else
				localmergeparallel2 = FALSE;
		}
		if ((var->addr&NCCMERGESERIESOVER) != 0)
		{
			if ((var->addr&NCCMERGESERIES) != 0) localmergeseries2 = TRUE; else
				localmergeseries2 = FALSE;
		}
	}
	if (localhierarchical1 || localhierarchical2) hierarchical = TRUE; else
		hierarchical = FALSE;
	if (localmergeparallel1 || localmergeparallel2) mergeparallel = TRUE; else
		mergeparallel = FALSE;
	if (localmergeseries1 || localmergeseries2) mergeseries = TRUE; else
		mergeseries = FALSE;
	if (hierarchical) recurse = FALSE;

	/* if recursing, look at subcells first */
	subcellsbad = FALSE;
	if (recurse)
	{
		for(ni = cell1->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			subnp1 = ni->proto;
			if (subnp1->primindex != 0) continue;

			/* ignore recursive references (showing icon in contents) */
			if (isiconof(subnp1, cell1)) continue;
			if (subnp1->cellview == el_iconview)
			{
				subnp1 = anyview(subnp1, cell1->cellview);
				if (subnp1 == NONODEPROTO) continue;
			}

			/* find equivalent to this in the other view */
			subnp2 = anyview(subnp1, cell2->cellview);
			if (subnp2 == NONODEPROTO)
			{
				ttyputerr(_("Cannot find %s view of cell %s"), cell2->cellview->viewname,
					describenodeproto(subnp1));
				continue;
			}
			ret = net_ncconelevel(subnp1, subnp2, preanalyze, interactive);
			if (ret < 0)
			{
				cell1->temp1 = (cell1->temp1 & ~NETNCCCHECKSTATE) | NETNCCCHECKEDBAD;
				cell2->temp1 = (cell2->temp1 & ~NETNCCCHECKSTATE) | NETNCCCHECKEDBAD;
				return(ret);
			}
			if (ret > 0) subcellsbad = TRUE;
		}
		for(ni = cell2->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			subnp2 = ni->proto;
			if (subnp2->primindex != 0) continue;

			/* ignore recursive references (showing icon in contents) */
			if (isiconof(subnp2, cell2)) continue;
			if (subnp2->cellview == el_iconview)
			{
				subnp2 = anyview(subnp2, cell2->cellview);
				if (subnp2 == NONODEPROTO) continue;
			}

			/* find equivalent to this in the other view */
			subnp1 = anyview(subnp2, cell1->cellview);
			if (subnp1 == NONODEPROTO)
			{
				ttyputerr(_("Cannot find %s view of cell %s"), cell1->cellview->viewname,
					describenodeproto(subnp2));
				continue;
			}
			ret = net_ncconelevel(subnp1, subnp2, preanalyze, interactive);
			if (ret < 0)
			{
				cell1->temp1 = (cell1->temp1 & ~NETNCCCHECKSTATE) | NETNCCCHECKEDBAD;
				cell2->temp1 = (cell2->temp1 & ~NETNCCCHECKSTATE) | NETNCCCHECKEDBAD;
				return(ret);
			}
			if (ret > 0) subcellsbad = TRUE;
		}
	}

	/* free any previous data structures */
	net_removeassociations();

	/* announce what is happening */
	ttyputmsg(_("Comparing cell %s with cell %s"),
		describenodeproto(cell1), describenodeproto(cell2));

	infstr = initinfstr();
	if ((net_ncc_options&NCCHIERARCHICAL) != 0) addstringtoinfstr(infstr, _("Flattening hierarchy")); else
	{
		if (recurse) addstringtoinfstr(infstr, _("Checking cells recursively")); else
			addstringtoinfstr(infstr, _("Checking this cell only"));
	}
	if (ignorepwrgnd) addstringtoinfstr(infstr, _("; Ignoring Power and Ground nets")); else
		addstringtoinfstr(infstr, _("; Considering Power and Ground nets"));
	ttyputmsg(x_("- %s"), returninfstr(infstr));

	infstr = initinfstr();
	if (!mergeparallel) addstringtoinfstr(infstr, _("Parallel components not merged")); else
		 addstringtoinfstr(infstr, _("Parallel components merged"));
	if (!mergeseries) addstringtoinfstr(infstr, _("; Series transistors not merged")); else
		 addstringtoinfstr(infstr, _("; Series transistors merged"));
	ttyputmsg(x_("- %s"), returninfstr(infstr));

	if (checkexportnames && checksize)
	{
		ttyputmsg(_("- Checking export names and component sizes"));
	} else if (!checkexportnames && !checksize)
	{
		ttyputmsg(_("- Ignoring export names and component sizes"));
	} else
	{
		if (checkexportnames)
			ttyputmsg(_("- Checking export names; Ignoring component sizes")); else
				ttyputmsg(_("- Ignoring export names; Checking component sizes"));
	}
	net_listnccoverrides(FALSE);

	/* precompute network topology */
	ttyputmsg(_("Preparing circuit for extraction..."));
	net_initnetflattening();
	ttyputmsg(_("--- Done preparing (%s so far)"), explainduration(endtimer()));

	/* build network of pseudocomponents */
	ttyputmsg(_("Extracting networks from %s..."), describenodeproto(cell1));
	savecurw = el_curwindowpart;
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		if (w->curnodeproto == cell1) break;
	if (w != NOWINDOWPART) el_curwindowpart = w;
	net_pcomp1 = net_makepseudo(cell1, &comp1, &netcount1, &power1, &ground1,
		&net_nodelist1, hierarchical, mergeparallel, mergeseries, TRUE, figuresizes);
	el_curwindowpart = savecurw;
	ttyputmsg(_("--- Done extracting %s (%s so far)"), describenodeproto(cell1),
		explainduration(endtimer()));
	if (net_pcomp1 == NOPCOMP && comp1 < 0)
	{
		cell1->temp1 = (cell1->temp1 & ~NETNCCCHECKSTATE) | NETNCCCHECKEDBAD;
		cell2->temp1 = (cell2->temp1 & ~NETNCCCHECKSTATE) | NETNCCCHECKEDBAD;
		return(-1);
	}
	ttyputmsg(_("Extracting networks from %s..."), describenodeproto(cell2));
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		if (w->curnodeproto == cell2) break;
	if (w != NOWINDOWPART) el_curwindowpart = w;
	net_pcomp2 = net_makepseudo(cell2, &comp2, &netcount2, &power2, &ground2,
		&net_nodelist2, hierarchical, mergeparallel, mergeseries, TRUE, figuresizes);
	el_curwindowpart = savecurw;
	ttyputmsg(_("--- Done extracting %s (%s so far)"), describenodeproto(cell2),
		explainduration(endtimer()));
	if (net_pcomp2 == NOPCOMP && comp2 < 0)
	{
		cell1->temp1 = (cell1->temp1 & ~NETNCCCHECKSTATE) | NETNCCCHECKEDBAD;
		cell2->temp1 = (cell2->temp1 & ~NETNCCCHECKSTATE) | NETNCCCHECKEDBAD;
		return(-1);
	}
	net_cell[0] = cell1;   net_cell[1] = cell2;
	net1remove = net2remove = 0;
	if (ignorepwrgnd)
	{
		net1remove = power1 + ground1;
		net2remove = power2 + ground2;
	}

	/* separate nets into plain and busses */
	buscount1 = 0;
	for(pn = net_nodelist1; pn != NOPNET; pn = pn->nextpnet)
	{
		net = pn->network;
		if (net == NONETWORK) continue;
		if (net->buswidth > 1) buscount1++;
	}
	netcount1 -= buscount1;
	buscount2 = 0;
	for(pn = net_nodelist2; pn != NOPNET; pn = pn->nextpnet)
	{
		net = pn->network;
		if (net == NONETWORK) continue;
		if (net->buswidth > 1) buscount2++;
	}
	netcount2 -= buscount2;

	/* remove extraneous information */
	net_removeextraneous(&net_pcomp1, &net_nodelist1, &comp1);
	net_removeextraneous(&net_pcomp2, &net_nodelist2, &comp2);

	if (!mergeparallel && comp1 != comp2)
	{
		/* see if merging parallel components makes them match */
		ocomp1 = comp1;   ocomp2 = comp2;
		if (comp1 < comp2)
		{
			/* try merging parallel components in cell 2 */
			ttyputmsg(_("--- Cell %s has %ld components and cell %s has %ld: merging parallel components in cell %s..."),
				describenodeproto(cell1), comp1, describenodeproto(cell2), comp2,
					describenodeproto(cell2));
			(void)net_mergeparallel(&net_pcomp2, net_nodelist2, &comp2);
			if (comp1 > comp2)
			{
				ttyputmsg(_("--- Merging parallel components in cell %s..."),
					describenodeproto(cell1));
				(void)net_mergeparallel(&net_pcomp1, net_nodelist1, &comp1);
			}
		} else
		{
			/* try merging parallel components in cell 1 */
			ttyputmsg(_("--- Cell %s has %ld components and cell %s has %ld: merging parallel components in cell %s..."),
				describenodeproto(cell1), comp1, describenodeproto(cell2), comp2,
					describenodeproto(cell1));
			(void)net_mergeparallel(&net_pcomp1, net_nodelist1, &comp1);
			if (comp2 > comp1)
			{
				ttyputmsg(_("--- Merging parallel components in cell %s..."),
					describenodeproto(cell2));
				(void)net_mergeparallel(&net_pcomp2, net_nodelist2, &comp2);
			}
		}
	}

	/* make sure network pointers are correct */
	net_fillinnetpointers(net_pcomp1, net_nodelist1);
	net_fillinnetpointers(net_pcomp2, net_nodelist2);

	/* announce results of extraction */
	errorsa = newstringarray(net_tool->cluster);
	if (comp1 == comp2)
		ttyputmsg(_("Both cells have %ld components"), comp1); else
	{
		infstr = initinfstr();
		formatinfstr(infstr, _("Cell %s has %ld components but cell %s has %ld"),
			describenodeproto(cell1), comp1, describenodeproto(cell2), comp2);
		addtostringarray(errorsa, returninfstr(infstr));
	}
	if (netcount1-net1remove == netcount2-net2remove)
		ttyputmsg(_("Both cells have %ld nets"), netcount1-net1remove); else
	{
		infstr = initinfstr();
		formatinfstr(infstr, _("Cell %s has %ld nets but cell %s has %ld"),
			describenodeproto(cell1), netcount1-net1remove, describenodeproto(cell2), netcount2-net2remove);
		addtostringarray(errorsa, returninfstr(infstr));
	}
	if (buscount1 == buscount2)
	{
		if (buscount1 != 0)
			ttyputmsg(_("Both cells have %ld busses"), netcount1-net1remove);
	} else
	{
		ttyputmsg(_("Note: cell %s has %ld busses but cell %s has %ld"),
			describenodeproto(cell1), buscount1, describenodeproto(cell2), buscount2);
	}
	if (!ignorepwrgnd)
	{
		if (power1 != power2)
		{
			infstr = initinfstr();
			formatinfstr(infstr, _("Cell %s has %ld power nets but cell %s has %ld"),
				describenodeproto(cell1), power1, describenodeproto(cell2), power2);
			addtostringarray(errorsa, returninfstr(infstr));
			infstr = initinfstr();
			formatinfstr(infstr, _("  Number of components on power nets in cell %s:"),
				describenodeproto(cell1));
			for(pn = net_nodelist1; pn != NOPNET; pn = pn->nextpnet)
				if ((pn->flags&POWERNET) != 0)
					formatinfstr(infstr, x_(" %ld"), pn->nodecount);
			addtostringarray(errorsa, returninfstr(infstr));
			infstr = initinfstr();
			formatinfstr(infstr, _("  Number of components on power nets in cell %s:"),
				describenodeproto(cell2));
			for(pn = net_nodelist2; pn != NOPNET; pn = pn->nextpnet)
				if ((pn->flags&POWERNET) != 0)
					formatinfstr(infstr, x_(" %ld"), pn->nodecount);
			addtostringarray(errorsa, returninfstr(infstr));
		}
		if (ground1 != ground2)
		{
			infstr = initinfstr();
			formatinfstr(infstr, _("Cell %s has %ld ground nets but cell %s has %ld"),
				describenodeproto(cell1), ground1, describenodeproto(cell2), ground2);
			addtostringarray(errorsa, returninfstr(infstr));
			infstr = initinfstr();
			formatinfstr(infstr, _("  Number of components on ground nets in cell %s:"),
				describenodeproto(cell1));
			for(pn = net_nodelist1; pn != NOPNET; pn = pn->nextpnet)
				if ((pn->flags&GROUNDNET) != 0)
					formatinfstr(infstr, x_(" %ld"), pn->nodecount);
			addtostringarray(errorsa, returninfstr(infstr));
			infstr = initinfstr();
			formatinfstr(infstr, _("  Number of components on ground nets in cell %s:"),
				describenodeproto(cell2));
			for(pn = net_nodelist2; pn != NOPNET; pn = pn->nextpnet)
				if ((pn->flags&GROUNDNET) != 0)
					formatinfstr(infstr, x_(" %ld"), pn->nodecount);
			addtostringarray(errorsa, returninfstr(infstr));
		}
	}

	/* if there are no problems found, look deeper */
	(void)getstringarray(errorsa, &errorcount);
	if (errorcount == 0)
	{
		/* check to be sure the component types match */
		net_checkcomponenttypes(errorsa, ignorepwrgnd, net_pcomp1, net_pcomp2,
			net_nodelist1, net_nodelist2, cell1, cell2);
	}

	/* check for duplicate names */
#if 0		/* why does this crash? */
	net_checkforduplicatenames(net_nodelist1);
	net_checkforduplicatenames(net_nodelist2);
#endif

	/* if there are possible problems, report them now */
	errorstrings = getstringarray(errorsa, &errorcount);
	for(i=0; i<errorcount; i++)
		ttyputmsg(_("Note: %s"), errorstrings[i]);
	if (!preanalyze && errorcount > 0 && interactive)
	{
		dia = DiaInitDialog(&net_nccwarndialog);
		if (dia == 0) return(-1);
		DiaInitTextDialog(dia, DNCW_DIFFLIST, DiaNullDlogList, DiaNullDlogItem,
			DiaNullDlogDone, -1, SCHORIZBAR);
		infstr = initinfstr();
		formatinfstr(infstr, _("Differences between cells %s and %s:"),
			describenodeproto(cell1), describenodeproto(cell2));
		DiaSetText(dia, DNCW_TITLE, returninfstr(infstr));
		for(i=0; i<errorcount; i++)
			DiaStuffLine(dia, DNCW_DIFFLIST, errorstrings[i]);
		DiaSelectLine(dia, DNCW_DIFFLIST, -1);

		for(;;)
		{
			itemHit = DiaNextHit(dia);
			if (itemHit == DNCW_DOPREANALYSIS || itemHit == DNCW_STOPNOW ||
				itemHit == DNCW_DONCC) break;
		}
		DiaDoneDialog(dia);
		if (itemHit == DNCW_STOPNOW)
		{
			killstringarray(errorsa);
			return(-1);
		}
		if (itemHit == DNCW_DOPREANALYSIS) preanalyze = TRUE;
	}
	killstringarray(errorsa);

	/* build list of PNODEs and wires on each net */
	if (preanalyze) verbose = 0;
	net_timestamp = 0;
	if (verbose != 0)
	{
		net_initializeverbose(net_pcomp1, net_nodelist1);
		net_initializeverbose(net_pcomp2, net_nodelist2);
	}

	if (preanalyze)
	{
		/* dump the networks and stop */
		net_showpreanalysis(cell1, net_pcomp1, net_nodelist1,
			cell2, net_pcomp2, net_nodelist2, ignorepwrgnd);
		cell1->temp1 = (cell1->temp1 & ~NETNCCCHECKSTATE) | NETNCCCHECKEDGOOD;
		cell2->temp1 = (cell2->temp1 & ~NETNCCCHECKSTATE) | NETNCCCHECKEDGOOD;
		return(0);
	}

	/* try to find network symmetry with existing switches */
	net_debuggeminipasscount = 0;
	net_debuggeminiexpandglobal = net_debuggeminiexpandglobalworked = 0;
	net_debuggeminigroupsrenumbered = net_debuggeminigroupssplit = 0;
	net_debuggeminitotalgroups = net_debuggeminitotalgroupsact = 0;
	ret = net_dogemini(net_pcomp1, net_nodelist1, net_pcomp2, net_nodelist2,
		checksize, checkexportnames, ignorepwrgnd);
	if (ret < 0) return(-1);

	/* if match failed, see if unmerged parallel components are ambiguous */
	if (ret != 0 && !mergeparallel)
	{
		for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
		{
			if (sg->grouptype != SYMGROUPCOMP) continue;
			if (sg->cellcount[0] <= 1 || sg->cellcount[1] <= 1) continue;
			for(f=0; f<2; f++)
			{
				for(i=1; i<sg->cellcount[f]; i++)
				{
					opc = (PCOMP *)sg->celllist[f][i-1];
					pc = (PCOMP *)sg->celllist[f][i];
					if (net_comparewirelist(pc, opc, FALSE)) continue;
					mergeparallel = TRUE;
					break;
				}
				if (mergeparallel) break;
			}
			if (mergeparallel) break;
		}
		if (mergeparallel)
		{
			/* might work if parallel components are merged */
			ttyputmsg(_("--- No match: trying again with parallel components merged"));
			net_unmatchedstatus(&prevunmatchednets, &prevunmatchedcomps, &symgroupcount);
			(void)net_mergeparallel(&net_pcomp1, net_nodelist1, &comp1);
			(void)net_mergeparallel(&net_pcomp2, net_nodelist2, &comp2);
			net_debuggeminipasscount = 0;
			net_debuggeminiexpandglobal = net_debuggeminiexpandglobalworked = 0;
			net_debuggeminigroupsrenumbered = net_debuggeminigroupssplit = 0;
			net_debuggeminitotalgroups = net_debuggeminitotalgroupsact = 0;
			ret = net_dogemini(net_pcomp1, net_nodelist1, net_pcomp2, net_nodelist2,
				checksize, checkexportnames, ignorepwrgnd);
			if (ret < 0) return(-1);
			if (ret != 0)
			{
				net_unmatchedstatus(&unmatchednets, &unmatchedcomps, &symgroupcount);
				if (unmatchednets + unmatchedcomps < prevunmatchednets + prevunmatchedcomps)
				{
					/* this improved things but didn't solve them, use it */
					ttyputmsg(_("------ Merge of parallel components improved match"));
				} else if (unmatchednets + unmatchedcomps == prevunmatchednets + prevunmatchedcomps)
					ttyputmsg(_("------ Merge of parallel components make no change")); else
						ttyputmsg(_("------ Merge of parallel components make things worse"));
			}
		}
	}

	/* free reason information */
	if (verbose != 0)
	{
		for(pn = net_nodelist1; pn != NOPNET; pn = pn->nextpnet)
		{
			efree((CHAR *)pn->hashreason);
			pn->hashreason = 0;
		}
		for(pn = net_nodelist2; pn != NOPNET; pn = pn->nextpnet)
		{
			efree((CHAR *)pn->hashreason);
			pn->hashreason = 0;
		}
		for(pc = net_pcomp1; pc != NOPCOMP; pc = pc->nextpcomp)
		{
			efree((CHAR *)pc->hashreason);
			pc->hashreason = 0;
		}
		for(pc = net_pcomp2; pc != NOPCOMP; pc = pc->nextpcomp)
		{
			efree((CHAR *)pc->hashreason);
			pc->hashreason = 0;
		}
	}
	if ((net_ncc_options&NCCENASTATISTICS) != 0)
	{
		ttyputmsg(x_("--- Made %ld iterations of Gemini"), net_debuggeminipasscount);
		if (net_debuggeminiexpandglobal != 0)
		{
			ttyputmsg(x_("---    Of those %ld iterations considered all groups"), net_debuggeminiexpandglobal);
			ttyputmsg(x_("---    And %ld of the all-group examinations found new splits"), net_debuggeminiexpandglobalworked);
		}
		ttyputmsg(x_("--- Renumbered %ld symmetry groups"), net_debuggeminigroupsrenumbered);
		if (net_debuggeminigroupssplit != net_debuggeminigroupsrenumbered)
			ttyputmsg(x_("---    Of these, %ld split"), net_debuggeminigroupssplit);
		if (net_debuggeminitotalgroups != net_debuggeminitotalgroupsact)
		{
			if (net_debuggeminitotalgroups == 0) net_debuggeminitotalgroups= 1;
			ttyputmsg(x_("---    On average, %ld%% of groups are active"),
				net_debuggeminitotalgroupsact*100/net_debuggeminitotalgroups);
		}
	}

	/* see if errors were found */
	initerrorlogging(_("NCC"));

#ifdef FORCESUNTOOLS
	if ((net_ncc_options&NCCEXPERIMENTAL) != 0)
	{
		errors = net_expanalyzesymmetrygroups(TRUE, checksize, checkexportnames, ignorepwrgnd, &errorcount);
	} else
#endif
	errors = net_analyzesymmetrygroups(TRUE, checksize, checkexportnames, ignorepwrgnd, &errorcount);

	/* write summary of NCC */
	elapsed = endtimer();
	if (elapsed > 60.0 && (us_useroptions&BEEPAFTERLONGJOB) != 0)
		ttybeep(SOUNDBEEP, TRUE);
	if (errors != 0)
	{
		switch (errors)
		{
			case SIZEERRORS:
				errortype = N_("Size");                          break;
			case EXPORTERRORS:
				errortype = N_("Export");                        break;
			case STRUCTUREERRORS:
				errortype = N_("Structural");                    break;
			case SIZEERRORS|EXPORTERRORS:
				errortype = N_("Size and Export");               break;
			case SIZEERRORS|STRUCTUREERRORS:
				errortype = N_("Size and Structural");           break;
			case EXPORTERRORS|STRUCTUREERRORS:
				errortype = N_("Export and Structural");         break;
			case SIZEERRORS|EXPORTERRORS|STRUCTUREERRORS:
				errortype = N_("Size, Export and Structural");   break;
			default:
				errortype = 0;
		}
		ttyputmsg(_("******* Found %ld %s differences! (%s)"), errorcount,
			errortype, explainduration(elapsed));
		ret = 1;
	} else
	{
		ttyputmsg(_("Cells %s and %s are equivalent (%s)"), describenodeproto(net_cell[0]),
			describenodeproto(net_cell[1]), explainduration(elapsed));
		if (subcellsbad)
		{
			ttyputmsg(_("******* But some subcells are not equivalent"));
		} else
		{
#ifndef NEWNCC
			net_preserveresults(cell1, cell2);
			net_preserveresults(cell2, cell1);
#endif
		}
		ret = subcellsbad ? 1 : 0;
	}
	termerrorlogging(TRUE);
#ifdef NEWNCC
	net_preserveresults(cell1, cell2);
	net_preserveresults(cell2, cell1);
#endif
	if (ret == 0)
	{
		cell1->temp1 = (cell1->temp1 & ~NETNCCCHECKSTATE) | NETNCCCHECKEDGOOD;
		cell2->temp1 = (cell2->temp1 & ~NETNCCCHECKSTATE) | NETNCCCHECKEDGOOD;
	} else
	{
		cell1->temp1 = (cell1->temp1 & ~NETNCCCHECKSTATE) | NETNCCCHECKEDBAD;
		cell2->temp1 = (cell2->temp1 & ~NETNCCCHECKSTATE) | NETNCCCHECKEDBAD;
	}
	return(ret);
}

/*
 * Routine to run the Gemini algorithm to match components/nets "pcomp1/pnet1" with
 * components/nets "pcomp2/pnet2".  Use "checksize" to check sizes,
 * "checkexportnames" to check port names, and "ignorepwrgnd" to ignore power and ground.
 * The value of "mergeparallel" indicates whether parallel components are merged.
 * The routine returns:
 *   -1  Hard error (memory allocation, etc.)
 *    0  Networks match
 *    1  No match, but association quiesced
 *    2  No match, and association did not quiesce
 */
INTBIG net_dogemini(PCOMP *pcomp1, PNET *pnet1, PCOMP *pcomp2, PNET *pnet2,
	BOOLEAN checksize, BOOLEAN checkexportnames, BOOLEAN ignorepwrgnd)
{
	REGISTER SYMGROUP *sgc, *sgn, *sgnz, *sg, *lastsg;
	REGISTER PCOMP *pc;
	REGISTER PNET *pn;
	REGISTER INTBIG i, changesc, changesn, verbose, redeemcount, unmatched,
		prevunmatched, prevsymgroupcount, splittype, assigncompfirst, renumber,
		changes, reassignnow, pass;
	INTBIG unmatchednets, unmatchedcomps, symgroupcount, errors, errorcount;
	CHAR prompt[100];
	static BOOLEAN toldtohitreturn = FALSE;

#ifdef FORCESUNTOOLS
	if ((net_ncc_options&NCCEXPERIMENTAL) != 0)
	{
		return(net_doexpgemini(pcomp1, pnet1, pcomp2, pnet2,
			checksize, checkexportnames, ignorepwrgnd));
	}
#endif

	verbose = net_ncc_options & (NCCVERBOSETEXT | NCCVERBOSEGRAPHICS);

	/* clear old symmetry group list */
	while (net_firstsymgroup != NOSYMGROUP)
	{
		sg = net_firstsymgroup;
		net_firstsymgroup = sg->nextsymgroup;
		net_freesymgroup(sg);
	}
	while (net_firstmatchedsymgroup != NOSYMGROUP)
	{
		sg = net_firstmatchedsymgroup;
		net_firstmatchedsymgroup = sg->nextsymgroup;
		net_freesymgroup(sg);
	}
	if (net_symgrouphashcompsize != 0)
	{
		efree((CHAR *)net_symgrouphashcomp);
		efree((CHAR *)net_symgrouphashckcomp);
	}
	if (net_symgrouphashnetsize != 0)
	{
		efree((CHAR *)net_symgrouphashnet);
		efree((CHAR *)net_symgrouphashcknet);
	}
	net_uniquehashvalue = -1;
	net_symgroupnumber = 1;

	/* determine size of hash tables */
	net_symgrouphashnetsize = net_symgrouphashcompsize = 0;
	for(pc = pcomp1; pc != NOPCOMP; pc = pc->nextpcomp) net_symgrouphashcompsize++;
	for(pc = pcomp2; pc != NOPCOMP; pc = pc->nextpcomp) net_symgrouphashcompsize++;
	if (net_symgrouphashcompsize <= 0) net_symgrouphashcompsize = 2;
	for(pn = pnet1; pn != NOPNET; pn = pn->nextpnet) net_symgrouphashnetsize++;
	for(pn = pnet2; pn != NOPNET; pn = pn->nextpnet) net_symgrouphashnetsize++;
	if (net_symgrouphashnetsize <= 0) net_symgrouphashnetsize = 2;
	net_symgrouphashcompsize = pickprime(net_symgrouphashcompsize * 2);
	net_symgrouphashnetsize = pickprime(net_symgrouphashnetsize * 2);
	net_symgrouphashcomp = (SYMGROUP **)emalloc(net_symgrouphashcompsize * (sizeof (SYMGROUP *)),
		net_tool->cluster);
	if (net_symgrouphashcomp == 0) return(-1);
	net_symgrouphashckcomp = (INTBIG *)emalloc(net_symgrouphashcompsize * SIZEOFINTBIG,
		net_tool->cluster);
	if (net_symgrouphashckcomp == 0) return(-1);
	net_symgrouphashnet = (SYMGROUP **)emalloc(net_symgrouphashnetsize * (sizeof (SYMGROUP *)),
		net_tool->cluster);
	if (net_symgrouphashnet == 0) return(-1);
	net_symgrouphashcknet = (INTBIG *)emalloc(net_symgrouphashnetsize * SIZEOFINTBIG,
		net_tool->cluster);
	if (net_symgrouphashcknet == 0) return(-1);
	for(i=0; i<net_symgrouphashcompsize; i++) net_symgrouphashcomp[i] = NOSYMGROUP;
	for(i=0; i<net_symgrouphashnetsize; i++) net_symgrouphashnet[i] = NOSYMGROUP;

	/* reset hash explanations */
	if (verbose != 0)
	{
		for(pc = pcomp1; pc != NOPCOMP; pc = pc->nextpcomp)
			(void)reallocstring(&pc->hashreason, x_("initial"), net_tool->cluster);
		for(pc = pcomp2; pc != NOPCOMP; pc = pc->nextpcomp)
			(void)reallocstring(&pc->hashreason, x_("initial"), net_tool->cluster);
		for(pn = pnet1; pn != NOPNET; pn = pn->nextpnet)
			(void)reallocstring(&pn->hashreason, x_("initial"), net_tool->cluster);
		for(pn = pnet2; pn != NOPNET; pn = pn->nextpnet)
			(void)reallocstring(&pn->hashreason, x_("initial"), net_tool->cluster);
	}

	/* new time stamp for initial entry into symmetry groups */
	net_timestamp++;

	/* initially assign all components to the same symmetry group (ignore SPICE) */
	sgc = net_newsymgroup(SYMGROUPCOMP, 1, 0);
	if (sgc == NOSYMGROUP) return(-1);
	for(pc = pcomp1; pc != NOPCOMP; pc = pc->nextpcomp)
	{
		pc->hashvalue = sgc->hashvalue;
		if (net_addtosymgroup(sgc, 0, (void *)pc)) return(-1);
		pc->symgroup = sgc;
	}
	for(pc = pcomp2; pc != NOPCOMP; pc = pc->nextpcomp)
	{
		pc->hashvalue = sgc->hashvalue;
		if (net_addtosymgroup(sgc, 1, (void *)pc)) return(-1);
		pc->symgroup = sgc;
	}

	/* initially assign all nets to the same symmetry group (with ignored pwr/gnd in zero group) */
	sgn = net_newsymgroup(SYMGROUPNET, 1, 0);
	if (sgn == NOSYMGROUP) return(-1);
	sgnz = net_newsymgroup(SYMGROUPNET, 0, 0);
	if (sgnz == NOSYMGROUP) return(-1);
	for(pn = pnet1; pn != NOPNET; pn = pn->nextpnet)
	{
		if (ignorepwrgnd && (pn->flags&(POWERNET|GROUNDNET)) != 0)
		{
			pn->hashvalue = sgnz->hashvalue;
			if (net_addtosymgroup(sgnz, 0, (void *)pn)) return(-1);
			pn->symgroup = sgnz;
		} else
		{
			pn->hashvalue = sgn->hashvalue;
			if (net_addtosymgroup(sgn, 0, (void *)pn)) return(-1);
			pn->symgroup = sgn;
		}
	}
	for(pn = pnet2; pn != NOPNET; pn = pn->nextpnet)
	{
		if (ignorepwrgnd && (pn->flags&(POWERNET|GROUNDNET)) != 0)
		{
			pn->hashvalue = sgnz->hashvalue;
			if (net_addtosymgroup(sgnz, 0, (void *)pn)) return(-1);
			pn->symgroup = sgnz;
		} else
		{
			pn->hashvalue = sgn->hashvalue;
			if (net_addtosymgroup(sgn, 1, (void *)pn)) return(-1);
			pn->symgroup = sgn;
		}
	}

	/* determine the true wirecount (considering ignored power and ground) */
	for(pc = pcomp1; pc != NOPCOMP; pc = pc->nextpcomp)
	{
		pc->truewirecount = pc->wirecount;
		for(i=0; i<pc->wirecount; i++)
			if (pc->netnumbers[i]->hashvalue == 0) pc->truewirecount--;
	}
	for(pc = pcomp2; pc != NOPCOMP; pc = pc->nextpcomp)
	{
		pc->truewirecount = pc->wirecount;
		for(i=0; i<pc->wirecount; i++)
			if (pc->netnumbers[i]->hashvalue == 0) pc->truewirecount--;
	}

	/* now iteratively refine the symmetry groups */
	net_unmatchedstatus(&unmatchednets, &unmatchedcomps, &symgroupcount);
	prevsymgroupcount = symgroupcount;
	prevunmatched = unmatchednets + unmatchedcomps;
	redeemcount = 1;
	assigncompfirst = 1;
	changesc = changesn = 0;
	pass = 0;
	for(i=0; i<MAXITERATIONS; i++)
	{
		if (stopping(STOPREASONNCC)) break;

		/* new time stamp for entry into symmetry groups */
		net_timestamp++;
		changes = 0;
		for(renumber=0; renumber<2; renumber++)
		{
			if ((renumber == 0 && assigncompfirst != 0) ||
				(renumber == 1 && assigncompfirst == 0))
			{
				/* assign new hash values to components */
				if ((verbose&NCCVERBOSETEXT) != 0)
					ttyputmsg(x_("***Computing component codes from networks:"));
				changes = changesc = net_assignnewhashvalues(reassignnow = SYMGROUPCOMP);
				if (changesc < 0) break;
			} else
			{
				/* assign new hash values to nets */
				if ((verbose&NCCVERBOSETEXT) != 0)
					ttyputmsg(x_("***Computing network codes from components:"));
				changes = changesn = net_assignnewhashvalues(reassignnow = SYMGROUPNET);
				if (changesn < 0) break;
			}

			/* show the state of the world if requested */
			if (verbose != 0)
			{
				net_showsymmetrygroups(verbose, reassignnow);
				esnprintf(prompt, 100, x_("%ld changed, %ld symmetry groups:"), changes, symgroupcount);
				if (!toldtohitreturn)
				{
					estrcat(prompt, x_(" (hit return to continue) "));
					toldtohitreturn = TRUE;
				}
				(void)asktool(us_tool, x_("flush-changes"));
				if (*ttygetlinemessages(prompt) != 0) break;
			}

			/* pull out matched objects */
			net_cleanupsymmetrygroups();

			/* every 3rd time, randomize hash codes */
			if (((pass++) % 3) == 0)
				net_randomizehashcodes(verbose);
		}
		if (changes < 0) break;

		/* if things are still improving, keep on */
		net_unmatchedstatus(&unmatchednets, &unmatchedcomps, &symgroupcount);
		unmatched = unmatchednets + unmatchedcomps;
		if (unmatched == 0) break;
		if (unmatched < prevunmatched)
		{
			prevunmatched = unmatched;
			i--;
			prevsymgroupcount = symgroupcount;
			continue;
		}

		/* if nothing changed or about to stop the loop, look for ambiguity */
		if (changesc + changesn == 0 || symgroupcount == prevsymgroupcount || i == MAXITERATIONS-1)
		{
			/* new time stamp for entry into symmetry groups */
			net_timestamp++;

			/* see if some incremental match can be applied */
			splittype = net_findamatch(verbose, ignorepwrgnd);
			if (splittype != 0)
			{
				if (splittype > 0)
				{
					/* if just split a component group, reassign networks first */
					assigncompfirst = 0;
				} else
				{
					/* if just split a network group, reassign components first */
					assigncompfirst = 1;
				}
				i = 0;
				prevsymgroupcount = symgroupcount;
				continue;
			}
			break;
		}
		prevsymgroupcount = symgroupcount;
	}

	/* put matched symmetry groups back into the main list (place it at the end) */
	lastsg = NOSYMGROUP;
	for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
		lastsg = sg;
	if (lastsg == NOSYMGROUP) net_firstsymgroup = net_firstmatchedsymgroup; else
		lastsg->nextsymgroup = net_firstmatchedsymgroup;
	net_firstmatchedsymgroup = NOSYMGROUP;

	/* see if errors were found */
	errors = net_analyzesymmetrygroups(FALSE, checksize, checkexportnames, ignorepwrgnd, &errorcount);
	if (errors == 0) return(0);
	if (changesc + changesn != 0) return(2);
	return(1);
}

/*
 * Routine to clean up the symmetry groups by deleting those with nothing in them
 * and pulling matched ones out of consideration.
 */
void net_cleanupsymmetrygroups(void)
{
	REGISTER SYMGROUP *sg, *lastsg, *nextsg;

	lastsg = NOSYMGROUP;
	for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = nextsg)
	{
		nextsg = sg->nextsymgroup;

		/* delete empty groups */
		if (sg->cellcount[0] == 0 && sg->cellcount[1] == 0)
		{
			if (lastsg == NOSYMGROUP) net_firstsymgroup = sg->nextsymgroup; else
				lastsg->nextsymgroup = sg->nextsymgroup;
			net_freesymgroup(sg);
			continue;
		}

		/* pull matched groups into separate list */
		if (sg->cellcount[0] == 1 && sg->cellcount[1] == 1)
		{
			if (lastsg == NOSYMGROUP) net_firstsymgroup = sg->nextsymgroup; else
				lastsg->nextsymgroup = sg->nextsymgroup;
			sg->nextsymgroup = net_firstmatchedsymgroup;
			net_firstmatchedsymgroup = sg;
			if ((net_ncc_options&NCCGRAPHICPROGRESS) != 0)
				net_showmatchedgroup(sg);
			continue;
		}
		lastsg = sg;
	}
}

#define RANDOMIZEBYMULTIPLYING 1

void net_randomizehashcodes(INTBIG verbose)
{
#ifdef RANDOMIZEBYMULTIPLYING
	REGISTER SYMGROUP *sg;
	REGISTER INTBIG index, factor;

	if ((verbose&NCCVERBOSETEXT) != 0)
		ttyputmsg(x_("***Renumbering hash codes:"));
	index = 0;
	for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
	{
		factor = getprime(index++);
		net_randomizesymgroup(sg, verbose, factor);
	}
	for(sg = net_firstmatchedsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
	{
		factor = getprime(index++);
		net_randomizesymgroup(sg, verbose, factor);
	}
#else
	REGISTER SYMGROUP *sg;

	if ((verbose&NCCVERBOSETEXT) != 0)
		ttyputmsg(x_("***Renumbering hash codes:"));
	for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
		net_randomizesymgroup(sg, verbose, 0);
	for(sg = net_firstmatchedsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
		net_randomizesymgroup(sg, verbose, 0);
#endif
	net_rebuildhashtable();
}

void net_randomizesymgroup(SYMGROUP *sg, INTBIG verbose, INTBIG factor)
{
	REGISTER INTBIG f, j;
	REGISTER PCOMP *pc;
	REGISTER PNET *pn;
	REGISTER NODEINST *ni;
	REGISTER NETWORK *net;

	if (sg->hashvalue != 0)
	{
#ifdef RANDOMIZEBYMULTIPLYING
		sg->hashvalue *= factor;
#else
		sg->hashvalue = (HASHTYPE)rand();
		sg->hashvalue = (sg->hashvalue << 16) | (HASHTYPE)rand();
		sg->hashvalue = (sg->hashvalue << 16) | (HASHTYPE)rand();
		sg->hashvalue = (sg->hashvalue << 16) | (HASHTYPE)rand();
#endif
		if (sg->grouptype == SYMGROUPCOMP)
		{
			for(f=0; f<2; f++)
			{
				for(j=0; j<sg->cellcount[f]; j++)
				{
					pc = (PCOMP *)sg->celllist[f][j];
					pc->hashvalue = sg->hashvalue;
					if ((verbose&NCCVERBOSETEXT) != 0)
					{
						if (pc->numactual == 1) ni = (NODEINST *)pc->actuallist; else
							ni = ((NODEINST **)pc->actuallist)[0];
						if (sg->groupindex != 0)
						{
							ttyputmsg(x_(" COMPONENT %s: now %s (hash #%ld)"), describenodeinst(ni),
								hugeinttoa(sg->hashvalue), sg->groupindex);
						} else
						{
							ttyputmsg(x_(" COMPONENT %s: now %s (hash)"), describenodeinst(ni),
								hugeinttoa(sg->hashvalue));
						}
					}
				}
			}
		} else
		{
			for(f=0; f<2; f++)
			{
				for(j=0; j<sg->cellcount[f]; j++)
				{
					pn = (PNET *)sg->celllist[f][j];
					pn->hashvalue = sg->hashvalue;
					if ((verbose&NCCVERBOSETEXT) != 0)
					{
						net = pn->network;
						if (net != NONETWORK)
						{
							if (sg->groupindex != 0)
							{
								ttyputmsg(x_(" NETWORK %s:%s: now %s (hash #%ld)"), describenodeproto(net->parent),
									net_describepnet(pn), hugeinttoa(sg->hashvalue), sg->groupindex);
							} else
							{
								ttyputmsg(x_(" NETWORK %s:%s: now %s (hash)"), describenodeproto(net->parent),
									net_describepnet(pn), hugeinttoa(sg->hashvalue));
							}
						}
					}
				}
			}
		}
	}
}

/*
 * Routine to assign new hash values to the components or nets (depending on the
 * value of "grouptype") in all symmetry groups.  Returns the number of changes
 * that were made (negative on error).
 */
INTBIG net_assignnewhashvalues(INTBIG grouptype)
{
	REGISTER SYMGROUP *sg;
	REGISTER INTBIG changes, verbose;
	REGISTER INTBIG f, i, j, change, activemask;
	REGISTER BOOLEAN matched, focusonactivegroups;
	REGISTER SYMGROUP *lastsg, *nextsg;
	REGISTER PCOMP *pc;
	REGISTER PNET *pn;

	/* pickup options */
	net_debuggeminipasscount++;
	verbose = net_ncc_options & (NCCVERBOSETEXT | NCCVERBOSEGRAPHICS);
	activemask = 0;
	if ((net_ncc_options & (NCCENAFOCSYMGRPFRE|NCCENAFOCSYMGRPPRO)) != 0)
	{
		if ((net_ncc_options&NCCENAFOCSYMGRPFRE) != 0) activemask |= GROUPFRESH;
		if ((net_ncc_options&NCCENAFOCSYMGRPPRO) != 0) activemask |= GROUPPROMISING;
		focusonactivegroups = TRUE;
	} else focusonactivegroups = FALSE;

	/* setup for spreading active groups */
	for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
		sg->groupflags &= ~activemask;

	changes = 0;
	for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
	{
		if (sg->hashvalue == 0) continue;
		if (sg->grouptype != grouptype) continue;
		net_debuggeminitotalgroups++;
		if (focusonactivegroups)
		{
			if ((sg->groupflags&GROUPACTIVENOW) == 0) continue;
		}
		net_debuggeminitotalgroupsact++;
		net_debuggeminigroupsrenumbered++;
		change = net_assignnewgrouphashvalues(sg, verbose);
		if (change != 0) net_debuggeminigroupssplit++;
		changes += change;
	}

	if (changes == 0 && focusonactivegroups)
	{
		/* no changes found while examining the focus group: expand to the entire list */
		net_debuggeminiexpandglobal++;
		for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
		{
			if (sg->hashvalue == 0) continue;
			if (sg->grouptype != grouptype) continue;
			if ((sg->groupflags&GROUPACTIVENOW) != 0) continue;
			net_debuggeminigroupsrenumbered++;
			change = net_assignnewgrouphashvalues(sg, verbose);
			if (change != 0) net_debuggeminigroupssplit++;
			changes += change;
		}
		if (changes != 0) net_debuggeminiexpandglobalworked++;
	}

	/* propagate active groups */
	for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
	{
		if (sg->grouptype != grouptype)
		{
			if ((sg->groupflags&activemask) != 0)
				sg->groupflags |= GROUPACTIVENOW;
		} else
		{
			if ((sg->groupflags&activemask) != 0)
				sg->groupflags |= GROUPACTIVENOW; else
					sg->groupflags &= ~GROUPACTIVENOW;
		}
	}

	/* see if local processing after a match is requested */
	if ((net_ncc_options&NCCDISLOCAFTERMATCH) != 0) return(changes);

	/* now look for newly created matches and keep working from there */
	for(;;)
	{
		/* clear all flags of locality */
		for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
		{
			switch (sg->grouptype)
			{
				case SYMGROUPCOMP:
					for(f=0; f<2; f++)
					{
						for(i=0; i<sg->cellcount[f]; i++)
						{
							pc = (PCOMP *)sg->celllist[f][i];
							pc->flags &= ~COMPLOCALFLAG;
						}
					}
					break;
				case SYMGROUPNET:
					for(f=0; f<2; f++)
					{
						for(i=0; i<sg->cellcount[f]; i++)
						{
							pn = (PNET *)sg->celllist[f][i];
							pn->flags &= ~NETLOCALFLAG;
						}
					}
					break;
			}
		}

		/* mark local flags and remove match groups */
		matched = FALSE;
		lastsg = NOSYMGROUP;
		for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = nextsg)
		{
			nextsg = sg->nextsymgroup;
			if (sg->hashvalue != 0)
			{
				if (sg->cellcount[0] == 1 && sg->cellcount[1] == 1)
				{
					switch (sg->grouptype)
					{
						case SYMGROUPCOMP:
							/* mark all related nets for local reevaluation */
							for(f=0; f<2; f++)
							{
								pc = (PCOMP *)sg->celllist[f][0];
								for(j=0; j<pc->wirecount; j++)
								{
									pn = pc->netnumbers[j];
									pn->flags |= NETLOCALFLAG;
								}
							}
							break;
						case SYMGROUPNET:
							/* mark all related components for local reevaluation */
							for(f=0; f<2; f++)
							{
								pn = (PNET *)sg->celllist[f][0];
								for(j=0; j<pn->nodecount; j++)
								{
									pc = pn->nodelist[j];
									pc->flags |= COMPLOCALFLAG;
								}
							}
							break;
					}

					/* pull the matched groups into separate list */
					if (lastsg == NOSYMGROUP) net_firstsymgroup = sg->nextsymgroup; else
						lastsg->nextsymgroup = sg->nextsymgroup;
					sg->nextsymgroup = net_firstmatchedsymgroup;
					net_firstmatchedsymgroup = sg;
					matched = TRUE;
					continue;
				}
			}
			lastsg = sg;
		}

		/* if there are no new matches, stop now */
		if (!matched) break;

		/* search for groups that need to be reevaluated */
		for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
		{
			if (sg->hashvalue == 0) continue;
			switch (sg->grouptype)
			{
				case SYMGROUPCOMP:
					/* see if this group is marked as local */
					for(f=0; f<2; f++)
					{
						for(i=0; i<sg->cellcount[f]; i++)
						{
							pc = (PCOMP *)sg->celllist[f][i];
							if ((pc->flags&COMPLOCALFLAG) != 0) break;
						}
						if (i < sg->cellcount[f]) break;
					}
					if (f >= 2) break;

					/* reevaluate this group */
					change = net_assignnewgrouphashvalues(sg, verbose);
					changes += change;
					break;
				case SYMGROUPNET:
					/* mark all related components for local reevaluation */
					for(f=0; f<2; f++)
					{
						for(i=0; i<sg->cellcount[f]; i++)
						{
							pn = (PNET *)sg->celllist[f][i];
							if ((pn->flags&NETLOCALFLAG) != 0) break;
						}
						if (i < sg->cellcount[f]) break;
					}
					if (f >= 2) break;

					/* reevaluate this group */
					change = net_assignnewgrouphashvalues(sg, verbose);
					changes += change;
					break;
			}
		}
	}
	return(changes);
}

INTBIG net_assignnewgrouphashvalues(SYMGROUP *sg, INTBIG verbose)
{
	REGISTER INTBIG f, i, j;
	REGISTER PCOMP *pc;
	REGISTER PNET *pn;
	REGISTER SYMGROUP *osg, *newgroup;
	REGISTER BOOLEAN groupsplits;

	switch (sg->grouptype)
	{
		case SYMGROUPCOMP:
			for(f=0; f<2; f++)
			{
				for(i=0; i<sg->cellcount[f]; i++)
				{
					pc = (PCOMP *)sg->celllist[f][i];

					/* if the group is properly matched, don't change its hash value */
					if (sg->cellcount[0] == 1 && sg->cellcount[1] == 1)
					{
						if (verbose != 0)
							(void)reallocstring(&pc->hashreason, x_("matched"), net_tool->cluster);
						continue;
					}

					/* if the group is a singleton, set a zero hash value */
					if (sg->cellcount[0] <= 0 || sg->cellcount[1] <= 0)
					{
						if (verbose != 0)
							(void)reallocstring(&pc->hashreason, x_("unmatched"), net_tool->cluster);
						pc->hashvalue = 0;
					} else
					{
						/* compute a new hash value for the component */
						pc->hashvalue = net_getcomphash(pc, verbose);
					}
				}
			}
			newgroup = NOSYMGROUP;
			groupsplits = FALSE;
			for(f=0; f<2; f++)
			{
				for(i=0; i<sg->cellcount[f]; i++)
				{
					pc = (PCOMP *)sg->celllist[f][i];
					if (pc->hashvalue != sg->hashvalue)
					{
						/* reassign this component to a different symmetry group */
						osg = net_findsymmetrygroup(SYMGROUPCOMP, pc->hashvalue, pc->truewirecount);
						if (osg == NOSYMGROUP)
						{
							osg = net_newsymgroup(SYMGROUPCOMP, pc->hashvalue, pc->truewirecount);
							if (osg == NOSYMGROUP) return(-1);
						}
						net_removefromsymgroup(sg, f, i);
						i--;
						if (net_addtosymgroup(osg, f, (void *)pc)) return(-1);
						pc->symgroup = osg;
						if (groupsplits) osg->groupflags |= GROUPFRESH; else
						{
							if (newgroup == NOSYMGROUP) newgroup = osg; else
							{
								if (newgroup != osg)
								{
									groupsplits = TRUE;
									newgroup->groupflags |= GROUPFRESH;
									osg->groupflags |= GROUPFRESH;
								}
							}
						}
					} else
					{
						if (groupsplits) sg->groupflags |= GROUPFRESH; else
						{
							if (newgroup == NOSYMGROUP) newgroup = sg; else
							{
								if (newgroup != sg)
								{
									groupsplits = TRUE;
									newgroup->groupflags |= GROUPFRESH;
									sg->groupflags |= GROUPFRESH;
								}
							}
						}
					}
				}
			}

			/* propagate promising groups */
			if (groupsplits && (net_ncc_options&NCCENAFOCSYMGRPPRO) != 0)
			{
				/* this group split into others: propagate this information for localism */
				for(f=0; f<2; f++)
				{
					for(i=0; i<sg->cellcount[f]; i++)
					{
						pc = (PCOMP *)sg->celllist[f][i];
						for(j=0; j<pc->wirecount; j++)
						{
							pn = pc->netnumbers[j];
							((SYMGROUP *)pn->symgroup)->groupflags |= GROUPPROMISING;
						}
					}
				}
			}
			break;

		case SYMGROUPNET:
			for(f=0; f<2; f++)
			{
				for(i=0; i<sg->cellcount[f]; i++)
				{
					pn = (PNET *)sg->celllist[f][i];

					/* if the group is properly matched, don't change its hash value */
					if (sg->cellcount[0] == 1 && sg->cellcount[1] == 1)
					{
						if (verbose != 0)
							(void)reallocstring(&pn->hashreason, x_("matched"), net_tool->cluster);
						continue;
					}

					/* if the group is a singleton, set a zero hash value */
					if (sg->cellcount[0] <= 0 || sg->cellcount[1] <= 0)
					{
						pn->hashvalue = 0;
						if (verbose != 0)
							(void)reallocstring(&pn->hashreason, x_("unmatched"), net_tool->cluster);
					} else
					{
						/* compute a new hash value for the net */
						pn->hashvalue = net_getnethash(pn, verbose);
					}
				}
			}
			newgroup = NOSYMGROUP;
			groupsplits = FALSE;
			for(f=0; f<2; f++)
			{
				for(i=0; i<sg->cellcount[f]; i++)
				{
					pn = (PNET *)sg->celllist[f][i];
					if (pn->hashvalue != sg->hashvalue)
					{
						/* reassign this component to a different symmetry group */
						osg = net_findsymmetrygroup(SYMGROUPNET, pn->hashvalue, pn->nodecount);
						if (osg == NOSYMGROUP)
						{
							osg = net_newsymgroup(SYMGROUPNET, pn->hashvalue, pn->nodecount);
							if (osg == NOSYMGROUP) return(-1);
						}
						net_removefromsymgroup(sg, f, i);
						i--;
						if (net_addtosymgroup(osg, f, (void *)pn)) return(-1);
						pn->symgroup = osg;
						if (groupsplits) osg->groupflags |= GROUPFRESH; else
						{
							if (newgroup == NOSYMGROUP) newgroup = osg; else
							{
								if (newgroup != osg)
								{
									groupsplits = TRUE;
									newgroup->groupflags |= GROUPFRESH;
									osg->groupflags |= GROUPFRESH;
								}
							}
						}
					} else
					{
						if (groupsplits) sg->groupflags |= GROUPFRESH; else
						{
							if (newgroup == NOSYMGROUP) newgroup = sg; else
							{
								if (newgroup != sg)
								{
									groupsplits = TRUE;
									newgroup->groupflags |= GROUPFRESH;
									sg->groupflags |= GROUPFRESH;
								}
							}
						}
					}
				}
			}

			if (groupsplits && (net_ncc_options&NCCENAFOCSYMGRPPRO) != 0)
			{
				/* this group split into others: propagate this information for localism */
				for(f=0; f<2; f++)
				{
					for(i=0; i<sg->cellcount[f]; i++)
					{
						pn = (PNET *)sg->celllist[f][i];
						for(j=0; j<pn->nodecount; j++)
						{
							pc = pn->nodelist[j];
							((SYMGROUP *)pc->symgroup)->groupflags |= GROUPPROMISING;
						}
					}
				}
			}
			break;
	}
	if (groupsplits) return(1);
	return(0);
}

/*
 * Routine to fill out the "nodecount/nodelist/nodewire" fields of the PNET
 * list in "pnetlist", given that it points to "pcomplist".  Returns true on error.
 */
void net_initializeverbose(PCOMP *pcomplist, PNET *pnetlist)
{
	REGISTER PNET *pn;
	REGISTER PCOMP *pc;

	for(pn = pnetlist; pn != NOPNET; pn = pn->nextpnet)
	{
		(void)allocstring(&pn->hashreason, x_("initial"), net_tool->cluster);
	}

	for(pc = pcomplist; pc != NOPCOMP; pc = pc->nextpcomp)
	{
		(void)allocstring(&pc->hashreason, x_("initial"), net_tool->cluster);
	}
}

/*
 * Routine to examine the unexpanded cell instances in lists "pcomp1" and "pcomp2"
 * and make sure there are matching numbers of each type.
 * Adds error messages to the string array "errorsa" if any problems are found.
 */
void net_checkcomponenttypes(void *errorsa, BOOLEAN ignorepwrgnd, PCOMP *pcomp1, PCOMP *pcomp2,
	PNET *pnetlist1, PNET *pnetlist2, NODEPROTO *cell1, NODEPROTO *cell2)
{
	REGISTER INTBIG cells1, cells2, cellnum, i, j, l, portcount1, portcount2, start;
	REGISTER NODEPROTO *thecell;
	REGISTER BOOLEAN found;
	REGISTER PCOMP **celllist1, **celllist2, *pc1, *pc2;
	REGISTER PNET *pn;
	REGISTER NODEINST *ni1, *ni2;
	REGISTER CHAR *pt;
	REGISTER void *infstr;

	/* first count the number of cell instances that are primitives in each cell */
	cells1 = 0;
	for(pc1 = pcomp1; pc1 != NOPCOMP; pc1 = pc1->nextpcomp)
	{
		if (pc1->numactual == 1) ni1 = (NODEINST *)pc1->actuallist; else
			ni1 = ((NODEINST **)pc1->actuallist)[0];
		if (ni1->proto->primindex == 0 && pc1->function == NPUNKNOWN) cells1++;
	}
	cells2 = 0;
	for(pc2 = pcomp2; pc2 != NOPCOMP; pc2 = pc2->nextpcomp)
	{
		if (pc2->numactual == 1) ni2 = (NODEINST *)pc2->actuallist; else
			ni2 = ((NODEINST **)pc2->actuallist)[0];
		if (ni2->proto->primindex == 0 && pc2->function == NPUNKNOWN) cells2++;
	}

	/* if they are different, report that error */
	if (cells1 != cells2)
	{
		infstr = initinfstr();
		formatinfstr(infstr, _("Cell %s has %ld instances but cell %s has %ld instances"),
			describenodeproto(cell1), cells1, describenodeproto(cell2), cells2);
		addtostringarray(errorsa, returninfstr(infstr));
		return;
	}

	/* if there are no cells on either side, stop now */
	if (cells1 == 0) return;

	/* make a list of each cell's "function" number */
	celllist1 = (PCOMP **)emalloc(cells1 * (sizeof (PCOMP *)), net_tool->cluster);
	if (celllist1 == 0) return;
	celllist2 = (PCOMP **)emalloc(cells2 * (sizeof (PCOMP *)), net_tool->cluster);
	if (celllist2 == 0) return;
	cells1 = 0;
	for(pc1 = pcomp1; pc1 != NOPCOMP; pc1 = pc1->nextpcomp)
	{
		if (pc1->numactual == 1) ni1 = (NODEINST *)pc1->actuallist; else
			ni1 = ((NODEINST **)pc1->actuallist)[0];
		if (ni1->proto->primindex != 0) continue;
		celllist1[cells1++] = pc1;
		portcount1 = 0;
		for(l=0; l<pc1->wirecount; l++)
		{
			pn = pc1->netnumbers[l];
			if (ignorepwrgnd && (pn->flags&(POWERNET|GROUNDNET)) != 0) continue;
			portcount1++;
		}
		pc1->truewirecount = (INTSML)portcount1;
	}
	cells2 = 0;
	for(pc2 = pcomp2; pc2 != NOPCOMP; pc2 = pc2->nextpcomp)
	{
		if (pc2->numactual == 1) ni2 = (NODEINST *)pc2->actuallist; else
			ni2 = ((NODEINST **)pc2->actuallist)[0];
		if (ni2->proto->primindex != 0) continue;
		celllist2[cells2++] = pc2;
		portcount2 = 0;
		for(l=0; l<pc2->wirecount; l++)
		{
			pn = pc2->netnumbers[l];
			if (ignorepwrgnd && (pn->flags&(POWERNET|GROUNDNET)) != 0) continue;
			portcount2++;
		}
		pc2->truewirecount = (INTSML)portcount2;
	}
	esort(celllist1, cells1, sizeof (PCOMP *), net_sortbycelltype);
	esort(celllist2, cells2, sizeof (PCOMP *), net_sortbycelltype);

	/* see if the functions are the same */
	for(i=0; i<cells1; i++)
	{
		pc1 = celllist1[i];
		pc2 = celllist2[i];
		if (pc1->numactual == 1) ni1 = (NODEINST *)pc1->actuallist; else
			ni1 = ((NODEINST **)pc1->actuallist)[0];
		if (pc2->numactual == 1) ni2 = (NODEINST *)pc2->actuallist; else
			ni2 = ((NODEINST **)pc2->actuallist)[0];
		if ((ni1->proto->temp1&NETCELLCODE) != (ni2->proto->temp1&NETCELLCODE)) break;
	}
	if (i >= cells1)
	{
		/* cell functions are the same: make sure the exports match */
		for(i=0; i<cells1; i++)
		{
			start = i;
			pc1 = celllist1[i];
			if (pc1->numactual == 1) ni1 = (NODEINST *)pc1->actuallist; else
				ni1 = ((NODEINST **)pc1->actuallist)[0];
			thecell = ni1->proto;
			cellnum = thecell->temp1 & NETCELLCODE;

			/* determine the end of the block of cells with the same instance number */
			while (i < cells1-1)
			{
				pc1 = celllist1[i+1];
				if (pc1->numactual == 1) ni1 = (NODEINST *)pc1->actuallist; else
					ni1 = ((NODEINST **)pc1->actuallist)[0];
				if (cellnum != (ni1->proto->temp1 & NETCELLCODE)) break;
				i++;
			}

			/* make sure the prototype exports match */
			{
				NETWORK **list1, **list2;
				INTBIG size1, size2, pos1, pos2;

				pc1 = celllist1[start];
				if (pc1->numactual == 1) ni1 = (NODEINST *)pc1->actuallist; else
					ni1 = ((NODEINST **)pc1->actuallist)[0];
				pc2 = celllist2[start];
				if (pc2->numactual == 1) ni2 = (NODEINST *)pc2->actuallist; else
					ni2 = ((NODEINST **)pc2->actuallist)[0];
				list1 = net_makeexternalnetlist(ni1, &size1, ignorepwrgnd);
				list2 = net_makeexternalnetlist(ni2, &size2, ignorepwrgnd);
				if (size1 > 0) esort(list1, size1, sizeof (NETWORK *), net_sortexportcodes);
				if (size2 > 0) esort(list2, size2, sizeof (NETWORK *), net_sortexportcodes);
				pos1 = pos2 = 0;
				while (pos1 < size1 || pos2 < size2)
				{
					if (pos1 < size1 && pos2 < size2 && list1[pos1]->temp2 == list2[pos2]->temp2)
					{
						pos1++;   pos2++;
						while (pos1 < size1 && list1[pos1]->temp2 == list1[pos1-1]->temp2)
							pos1++;
						while (pos2 < size2 && list2[pos2]->temp2 == list2[pos2-1]->temp2)
							pos2++;
						continue;
					}
					if (pos2 >= size2 ||
						(pos1 < size1 && list1[pos1]->temp2 < list2[pos2]->temp2))
					{
						/* report or fix missing connection */
						net_foundmismatch(list1[pos1]->parent, list1[pos1], ni2->proto, celllist2, start, i,
							pnetlist2, ignorepwrgnd, errorsa);
						pos1++;
						while (pos1 < size1 && list1[pos1]->temp2 == list1[pos1-1]->temp2)
							pos1++;
					} else if (pos1 >= size1 ||
						(pos2 < size2 && list2[pos2]->temp2 < list1[pos1]->temp2))
					{
						/* report or fix missing connection */
						net_foundmismatch(list2[pos2]->parent, list2[pos2], ni1->proto, celllist1, start, i,
							pnetlist1, ignorepwrgnd, errorsa);
						pos2++;
						while (pos2 < size2 && list2[pos2]->temp2 == list2[pos2-1]->temp2)
							pos2++;
					}
				}
				if (size1 > 0) efree((CHAR *)list1);
				if (size2 > 0) efree((CHAR *)list2);
			}

			/* check all cells for export similarity */
			for(j=start; j<=i; j++)
			{
				pc1 = celllist1[j];
				if (pc1->numactual == 1) ni1 = (NODEINST *)pc1->actuallist; else
					ni1 = ((NODEINST **)pc1->actuallist)[0];

				pc2 = celllist2[j];
				if (pc2->numactual == 1) ni2 = (NODEINST *)pc2->actuallist; else
					ni2 = ((NODEINST **)pc2->actuallist)[0];
				if (ni1->proto == ni2->proto) continue;

				/* make sure the export count matches */
				if (pc1->truewirecount != pc2->truewirecount)
				{
					infstr = initinfstr();
					formatinfstr(infstr, _("Instance %s has %ld wires in cell %s but has %ld wires in cell %s"),
						ni1->proto->protoname, pc1->truewirecount, describenodeproto(cell1),
							pc2->truewirecount, describenodeproto(cell2));
					addtostringarray(errorsa, returninfstr(infstr));
				}
			}
		}
	} else
	{
		/* cell functions are different: report the differences */
		infstr = initinfstr();
		formatinfstr(infstr, _("These instances exist only in cell %s:"),
			describenodeproto(cell1));
		found = FALSE;
		for(i=0; i<cells1; i++)
		{
			/* get the index of the next cell instance in the list */
			pc1 = celllist1[i];
			if (pc1->numactual == 1) ni1 = (NODEINST *)pc1->actuallist; else
				ni1 = ((NODEINST **)pc1->actuallist)[0];
			thecell = ni1->proto;
			cellnum = thecell->temp1 & NETCELLCODE;

			/* advance to the end of the block of cells with the same instance number */
			while (i < cells1-1)
			{
				pc1 = celllist1[i+1];
				if (pc1->numactual == 1) ni1 = (NODEINST *)pc1->actuallist; else
					ni1 = ((NODEINST **)pc1->actuallist)[0];
				if (cellnum != (ni1->proto->temp1 & NETCELLCODE)) break;
				i++;
			}

			/* make sure it exists in the other list */
			for(j=0; j<cells2; j++)
			{
				pc2 = celllist2[j];
				if (pc2->numactual == 1) ni2 = (NODEINST *)pc2->actuallist; else
					ni2 = ((NODEINST **)pc2->actuallist)[0];
				if (cellnum == (ni2->proto->temp1 & NETCELLCODE)) break;
			}
			if (j < cells2) continue;

			formatinfstr(infstr, x_(" %s"), thecell->protoname);
			found = TRUE;
		}
		pt = returninfstr(infstr);
		if (found) addtostringarray(errorsa, pt);

		infstr = initinfstr();
		formatinfstr(infstr, _("These instances exist only in cell %s:"),
			describenodeproto(cell2));
		found = FALSE;
		for(i=0; i<cells2; i++)
		{
			/* get the index of the next cell instance in the list */
			pc2 = celllist2[i];
			if (pc2->numactual == 1) ni2 = (NODEINST *)pc2->actuallist; else
				ni2 = ((NODEINST **)pc2->actuallist)[0];
			thecell = ni2->proto;
			cellnum = thecell->temp1 & NETCELLCODE;

			/* advance to the end of the block of cells with the same instance number */
			while (i < cells2-1)
			{
				pc2 = celllist2[i+1];
				if (pc2->numactual == 1) ni2 = (NODEINST *)pc2->actuallist; else
					ni2 = ((NODEINST **)pc2->actuallist)[0];
				if (cellnum != (ni2->proto->temp1 & NETCELLCODE)) break;
				i++;
			}

			/* make sure it exists in the other list */
			for(j=0; j<cells1; j++)
			{
				pc1 = celllist1[j];
				if (pc1->numactual == 1) ni1 = (NODEINST *)pc1->actuallist; else
					ni1 = ((NODEINST **)pc1->actuallist)[0];
				if (cellnum == (ni1->proto->temp1 & NETCELLCODE)) break;
			}
			if (j < cells1) continue;

			formatinfstr(infstr, x_(" %s"), thecell->protoname);
			found = TRUE;
		}
		pt = returninfstr(infstr);
		if (found) addtostringarray(errorsa, pt);
	}
	efree((CHAR *)celllist1);
	efree((CHAR *)celllist2);
}

/*
 * Routine to handle a missing network on an unexpanded icon in NCC.
 * Cell "cell" has network "net", but "cellwithout" has no corresponding network.
 * The routine reports the problem in the string array "errorsa".
 *
 * A special case is handled.  If:
 *     "cellwithout" is an icon
 *     the network is of type power or ground
 *     there is a global network of that type in the flattened circuit
 * then:
 *     create a port on "cell" of that type
 *     connect it to that global net.
 *     To do this, modify the PCOMPs in entries "start" to "end" of "celllist".
 * If power and ground are being ignored, "ignorepwrgnd" is TRUE.
 * The list of PNETs in cell "cell" is in "pnetlist".
 */
void net_foundmismatch(NODEPROTO *cell, NETWORK *net, NODEPROTO *cellwithout, PCOMP **celllist,
	INTBIG start, INTBIG end, PNET *pnetlist, BOOLEAN ignorepwrgnd, void *errorsa)
{
	REGISTER void *infstr;
	REGISTER PNET *pn, **newnetnumbers;
	REGISTER PCOMP *pc, **newnodelist;
	REGISTER PORTPROTO **newportlist, *pp;
	REGISTER INTBIG i, j, wantedflag, newnodecount, *newnodewire;
	REGISTER INTSML *newportindices, *newstate, newwirecount;
	REGISTER BOOLEAN fixed;

	fixed = FALSE;

	/* special case for icons with missing connections (and not ignoring power and ground) */
	if (cellwithout->cellview == el_iconview && !ignorepwrgnd)
	{
		/* see if the missing connection is power or ground */
		wantedflag = 0;
		if (net->globalnet == GLOBALNETPOWER) wantedflag = POWERNET; else
			if (net->globalnet == GLOBALNETGROUND) wantedflag = GROUNDNET; else
		{
			pc = celllist[start];
			for(j=0; j<pc->wirecount; j++)
			{
				pn = pc->netnumbers[j];
				if (pn->network == net) break;
			}
			if (j < pc->wirecount && (pn->flags&(POWERNET|GROUNDNET)) != 0)
				wantedflag = pn->flags&(POWERNET|GROUNDNET);
			if (wantedflag == 0 && net->portcount > 0)
			{
				for(pp = cell->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
					if (pp->network == net) break;
				if (pp != NOPORTPROTO)
				{
					if ((pp->userbits&STATEBITS) == PWRPORT) wantedflag = POWERNET; else
						if ((pp->userbits&STATEBITS) == GNDPORT) wantedflag = GROUNDNET;
				}
			}
		}
		if (wantedflag != 0)
		{
			/* missing power or ground on an icon: see if such a signal is in the list */
			for(pn = pnetlist; pn != NOPNET; pn = pn->nextpnet)
				if ((pn->flags&(POWERNET|GROUNDNET)) == wantedflag) break;
			if (pn != NOPNET)
			{
				/* signal found: add it to the components */
				ttyputmsg(_("Note: added %s port to Cell %s"),
					net_describepnet(pn), describenodeproto(cellwithout));
				for(i=start; i<=end; i++)
				{
					/* add the PNET to the PCOMP */
					pc = celllist[i];
					newwirecount = pc->wirecount + 1;
					newportindices = (INTSML *)emalloc(newwirecount * SIZEOFINTSML, net_tool->cluster);
					newportlist = (PORTPROTO **)emalloc(newwirecount * sizeof (PORTPROTO *), net_tool->cluster);
					newnetnumbers = (PNET **)emalloc(newwirecount * sizeof (PNET *), net_tool->cluster);
					newstate = (INTSML *)emalloc(newwirecount * SIZEOFINTSML, net_tool->cluster);
					if (newportindices == 0 || newportlist == 0 || newnetnumbers == 0 ||
						newstate == 0) return;
					for(j=0; j<pc->wirecount; j++)
					{
						newportindices[j] = pc->portindices[j];
						newportlist[j] = pc->portlist[j];
						newnetnumbers[j] = pc->netnumbers[j];
						newstate[j] = pc->state[j];
					}
					newportindices[pc->wirecount] = (INTSML)net->temp2;
					newportlist[pc->wirecount] = NOPORTPROTO;
					newnetnumbers[pc->wirecount] = pn;
					newstate[pc->wirecount] = 0;
					if (pc->wirecount > 0)
					{
						efree((CHAR *)pc->portindices);
						efree((CHAR *)pc->portlist);
						efree((CHAR *)pc->netnumbers);
						efree((CHAR *)pc->state);
					}
					pc->portindices = newportindices;
					pc->portlist = newportlist;
					pc->netnumbers = newnetnumbers;
					pc->state = newstate;
					pc->wirecount = newwirecount;
					pc->truewirecount++;

					/* add the PCOMP to the PNET */
					newnodecount = pn->nodecount + 1;
					newnodelist = (PCOMP **)emalloc(newnodecount * sizeof (PCOMP *), net_tool->cluster);
					newnodewire = (INTBIG *)emalloc(newnodecount * SIZEOFINTBIG, net_tool->cluster);
					if (newnodelist == 0 || newnodewire == 0) return;
					for(j=0; j<pn->nodecount; j++)
					{
						newnodelist[j] = pn->nodelist[j];
						newnodewire[j] = pn->nodewire[j];
					}
					newnodelist[pn->nodecount] = pc;
					newnodewire[pn->nodecount] = pc->wirecount-1;
					if (pn->nodecount > 0)
					{
						efree((CHAR *)pn->nodelist);
						efree((CHAR *)pn->nodewire);
					}
					pn->nodelist = newnodelist;
					pn->nodewire = newnodewire;
					pn->nodecount = newnodecount;
				}
				fixed = TRUE;
			}
		}
	}
	if (!fixed)
	{
		/* can't fix it: report the error */
		infstr = initinfstr();
		formatinfstr(infstr, _("Cell %s has %s, which does not exist in cell %s"),
			describenodeproto(cell), describenetwork(net), describenodeproto(cellwithout));
		addtostringarray(errorsa, returninfstr(infstr));
	}
}

NETWORK **net_makeexternalnetlist(NODEINST *ni, INTBIG *size, BOOLEAN ignorepwrgnd)
{
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
	REGISTER NETWORK *net, *subnet, **list;
	REGISTER INTBIG i, total;

	np = contentsview(ni->proto);
	if (np == NONODEPROTO) np = ni->proto;
	total = 0;
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		if (ignorepwrgnd)
		{
			if (portispower(pp) || portisground(pp)) continue;
		}
		net = pp->network;
		if (net->buswidth > 1)
		{
			for(i=0; i<net->buswidth; i++) total++;
		} else total++;
	}
	*size = total;
	if (total > 0)
	{
		list = (NETWORK **)emalloc(total * (sizeof (NETWORK *)), el_tempcluster);
		total = 0;
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			if (ignorepwrgnd)
			{
				if (portispower(pp) || portisground(pp)) continue;
			}
			net = pp->network;
			if (net->buswidth > 1)
			{
				for(i=0; i<net->buswidth; i++)
				{
					subnet = net->networklist[i];
					list[total++] = subnet;
				}
			} else
			{
				list[total++] = net;
			}
		}
	}
	return(list);
}

int net_sortexportcodes(const void *e1, const void *e2)
{
	REGISTER NETWORK *pp1, *pp2;

	pp1 = *((NETWORK **)e1);
	pp2 = *((NETWORK **)e2);
	return(pp1->temp2 - pp2->temp2);
}

int net_sortbycelltype(const void *n1, const void *n2)
{
	REGISTER PCOMP *pc1, *pc2;
	REGISTER NODEINST *ni1, *ni2;
	REGISTER INTBIG diff;

	pc1 = *((PCOMP **)n1);
	pc2 = *((PCOMP **)n2);
	if (pc1->numactual == 1) ni1 = (NODEINST *)pc1->actuallist; else
		ni1 = ((NODEINST **)pc1->actuallist)[0];
	if (pc2->numactual == 1) ni2 = (NODEINST *)pc2->actuallist; else
		ni2 = ((NODEINST **)pc2->actuallist)[0];
	diff = (ni1->proto->temp1 & NETCELLCODE) - (ni2->proto->temp1 & NETCELLCODE);
	if (diff != 0) return(diff);
	return(pc1->truewirecount - pc2->truewirecount);
}

int net_sortbypnet(const void *n1, const void *n2)
{
	REGISTER PNET *pn1, *pn2;

	pn1 = *((PNET **)n1);
	pn2 = *((PNET **)n2);
	return(namesame(net_describepnet(pn1), net_describepnet(pn2)));
}

/*
 * Routine to remove extraneous information.  It removes busses from the
 * networks and it removes SPICE parts from the components.
 */
void net_removeextraneous(PCOMP **pcomplist, PNET **pnetlist, INTBIG *comp)
{
	REGISTER PNET *pn, *lastpn, *nextpn;
	REGISTER PCOMP *pc, *lastpc, *nextpc;
	REGISTER NETWORK *net;

	/* initialize all networks */
	lastpn = NOPNET;
	for(pn = *pnetlist; pn != NOPNET; pn = nextpn)
	{
		nextpn = pn->nextpnet;
		net = pn->network;

		/* remove networks that refer to busses (individual signals are compared) */
		if (net != NONETWORK && net->buswidth > 1)
		{
			if (lastpn == NOPNET)
			{
				*pnetlist = pn->nextpnet;
			} else
			{
				lastpn->nextpnet = pn->nextpnet;
			}
			net_freepnet(pn);
			continue;
		}
		lastpn = pn;
	}

	lastpc = NOPCOMP;
	for(pc = *pcomplist; pc != NOPCOMP; pc = nextpc)
	{
		nextpc = pc->nextpcomp;

		/* remove components that relate to SPICE simulation */
		if (net_isspice(pc))
		{
			if (lastpc == NOPCOMP)
			{
				*pcomplist = pc->nextpcomp;
			} else
			{
				lastpc->nextpcomp = pc->nextpcomp;
			}
			net_freepcomp(pc);
			(*comp)--;
			continue;
		}
		lastpc = pc;
	}
}

#if 0
/*
 * Routine to check for duplicate names in the netlist, which may cause problems later.
 */
void net_checkforduplicatenames(PNET *pnetlist)
{
	REGISTER PNET *pn, **pnlist, *lastpn;
	REGISTER INTBIG total, i;
	REGISTER NETWORK *net, *lastnet;

	/* see how many PNETs there are */
	total = 0;
	for(pn = pnetlist; pn != NOPNET; pn = pn->nextpnet)
	{
		net = pn->network;
		if (net == NONETWORK || net->namecount == 0) continue;
		total++;
	}
	if (total == 0) return;

	/* make a list of them */
	pnlist = (PNET **)emalloc(total * (sizeof (PNET *)), net_tool->cluster);
	if (pnlist == 0) return;
	i = 0;
	for(pn = pnetlist; pn != NOPNET; pn = pn->nextpnet)
	{
		net = pn->network;
		if (net == NONETWORK || net->namecount == 0) continue;
		pnlist[i++] = pn;
	}

	/* sort by name within parent */
	esort(pnlist, total, sizeof (PNET *), net_sortpnetlist);

	/* now look for duplicates */
	for(i=1; i<total; i++)
	{
		lastpn = pnlist[i-1];
		lastnet = lastpn->network;
		pn = pnlist[i];
		net = pn->network;
		if (lastnet == net) continue;
		if (net->parent != lastnet->parent) continue;
		if (namesame(networkname(net, 0), networkname(lastnet, 0)) != 0) continue;
		ttyputmsg(_("Warning: cell %s has multiple networks named %s"),
			describenodeproto(net->parent), networkname(net, 0));
	}
	efree((CHAR *)pnlist);
}

int net_sortpnetlist(const void *n1, const void *n2)
{
	REGISTER PNET *pn1, *pn2;
	REGISTER NETWORK *net1, *net2;

	pn1 = *((PNET **)n1);
	pn2 = *((PNET **)n2);
	net1 = pn1->network;
	net2 = pn2->network;
	if (net1->parent != net2->parent) return((int)(net1->parent - net2->parent));
	return(namesame(networkname(net1, 0), networkname(net2, 0)));
}
#endif

/******************************** RESULTS ANALYSIS ********************************/

#ifdef NEWNCC
typedef struct
{
	PORTPROTO *pp;
	SYMGROUP *sg;
} EXPORTSYMGROUP;

int net_sortexportsymgroup(const void *e1, const void *e2);

int net_sortexportsymgroup(const void *e1, const void *e2)
{
	REGISTER EXPORTSYMGROUP *es1, *es2;

	es1 = (EXPORTSYMGROUP *)e1;
	es2 = (EXPORTSYMGROUP *)e2;
	return(namesame(es1->pp->protoname, es2->pp->protoname));
}

#endif

/*
 * Routine to look at the symmetry groups and return the number of hard errors and the number of
 * soft errors in "harderrors", and "softerrors".  If "reporterrors" is nonzero, these errors are
 * logged for perusal by the user.  If "checksize" is true, check sizes.  If "checkexportnames" is
 * true, check export names.  If "ignorepwrgnd" is true, ignore power and ground nets.  The total
 * number of errors is put in "errorcount".
 */
INTBIG net_analyzesymmetrygroups(BOOLEAN reporterrors, BOOLEAN checksize, BOOLEAN checkexportnames,
	BOOLEAN ignorepwrgnd, INTBIG *errorcount)
{
	REGISTER INTBIG f, i, j, errors, pctdiff1, pctdiff2, pctdiff, worstpctdiff, i1, i2;
	float diff, largest;
	BOOLEAN valid, exportcomparison;
	REGISTER SYMGROUP *sg, *osg, *amblist, *unasslist;
	REGISTER PCOMP *pc1, *pc2;
	REGISTER void *err;
	REGISTER CHAR *net1name, *pt1, *pt2;
	CHAR size1[200], size2[200];
	REGISTER PNET *pn, *pn1, *pn2, *opn;
	PNET **pnlist[2];
	REGISTER PORTPROTO *pp1, *pp2;
	REGISTER NODEPROTO *par1, *par2;
	REGISTER ARCINST *ai;
	REGISTER NETWORK *net;
	INTBIG pnlisttotal[2];
	REGISTER void *infstr;

	errors = 0;
	*errorcount = 0;
	worstpctdiff = 0;
	amblist = unasslist = NOSYMGROUP;

	/* now evaluate the differences */
	for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
	{
		/* if there is nothing in the group, ignore it */
		if (sg->cellcount[0] == 0 && sg->cellcount[1] == 0) continue;

		/* if the group is a good match, make optional checks */
		if (sg->cellcount[0] == 1 && sg->cellcount[1] == 1)
		{
			if (checksize && sg->grouptype == SYMGROUPCOMP)
			{
				/* see if sizes match */
				pc1 = (PCOMP *)sg->celllist[0][0];
				pc2 = (PCOMP *)sg->celllist[1][0];
				if ((pc1->flags&COMPHASWIDLEN) != 0)
				{
					if (!net_componentequalvalue(pc1->width, pc2->width) ||
						!net_componentequalvalue(pc1->length, pc2->length))
					{
						if (reporterrors)
						{
							diff = (float)fabs(pc1->width - pc2->width);
							if (pc1->width > pc2->width) largest = pc1->width; else largest = pc2->width;
							pctdiff1 = roundfloat(diff * 100.0f / largest);
							diff = (float)fabs(pc1->length - pc2->length);
							if (pc1->length > pc2->length) largest = pc1->length; else largest = pc2->length;
							pctdiff2 = roundfloat(diff * 100.0f / largest);
							pctdiff = maxi(pctdiff1, pctdiff2);
							if (pctdiff > worstpctdiff) worstpctdiff = pctdiff;
							esnprintf(size1, 200, x_("%s/%s"), frtoa(roundfloat(pc1->width)), frtoa(roundfloat(pc1->length)));
							esnprintf(size2, 200, x_("%s/%s"), frtoa(roundfloat(pc2->width)), frtoa(roundfloat(pc2->length)));
							net_reportsizeerror(pc1, size1, pc2, size2, pctdiff, sg);
							(*errorcount)++;
						}
						errors |= SIZEERRORS;
					}
				} else if ((pc1->flags&COMPHASAREA) != 0)
				{
					if (!net_componentequalvalue(pc1->length, pc2->length))
					{
						if (reporterrors)
						{
							diff = (float)fabs(pc1->length - pc2->length);
							if (pc1->length > pc2->length) largest = pc1->length; else largest = pc2->length;
							pctdiff = roundfloat(diff * 100.0f / largest);
							if (pctdiff > worstpctdiff) worstpctdiff = pctdiff;
							if (pc1->function == NPRESIST)
							{
								/* not area: show the values as they are */
								esnprintf(size1, 200, x_("%g"), pc1->length);
								esnprintf(size2, 200, x_("%g"), pc2->length);
								net_reportsizeerror(pc1, size1, pc2, size2, pctdiff, sg);
							} else
							{
								/* area: make the values sensible */
								net_reportsizeerror(pc1, frtoa(roundfloat(pc1->length)), pc2,
									frtoa(roundfloat(pc2->length)), pctdiff, sg);
							}
							(*errorcount)++;
						}
						errors |= SIZEERRORS;
					}
				}
			}
			if (sg->grouptype == SYMGROUPNET)
			{
				/* see if names match */
				pn1 = (PNET *)sg->celllist[0][0];
				pn2 = (PNET *)sg->celllist[1][0];

				/* ignore name match for power and ground nets */
				if ((pn1->flags&(POWERNET|GROUNDNET)) != 0 || (pn2->flags&(POWERNET|GROUNDNET)) != 0)
				{
					if ((pn1->flags&(POWERNET|GROUNDNET)) != (pn2->flags&(POWERNET|GROUNDNET)))
					{
						if (reporterrors)
						{
							infstr = initinfstr();
							formatinfstr(infstr, _("Network '%s' in cell %s is on different power/ground than network '%s' in cell %s"),
								net_describepnet(pn1), describenodeproto(net_cell[0]),
								net_describepnet(pn2), describenodeproto(net_cell[1]));
							err = logerror(returninfstr(infstr), NONODEPROTO, 0);
							net_addsymgrouptoerror(err, sg);
							(*errorcount)++;
						}
						errors |= EXPORTERRORS;
					}
					continue;
				}

				if ((pn1->flags&EXPORTEDNET) != 0 &&
					(pn1->realportcount > 0 || (pn1->network != NONETWORK && pn1->network->namecount > 0)))
				{
					if ((pn2->flags&EXPORTEDNET) == 0)
					{
						if (checkexportnames)
						{
							/* net in cell 1 is exported, but net in cell 2 isn't */
							if (reporterrors)
							{
								infstr = initinfstr();
								formatinfstr(infstr, _("Network in cell %s is '%s' but network in cell %s is not exported"),
									describenodeproto(net_cell[0]), net_describepnet(pn1), describenodeproto(net_cell[1]));
								err = logerror(returninfstr(infstr), NONODEPROTO, 0);
								net_addsymgrouptoerror(err, sg);
								(*errorcount)++;
							}
							errors |= EXPORTERRORS;
						}
					} else
					{
						/* both networks exported: check names */
						if (checkexportnames)
						{
							exportcomparison = net_sameexportnames(pn1, pn2);
							if (!exportcomparison)
							{
								if (reporterrors)
								{
									infstr = initinfstr();
									addstringtoinfstr(infstr, net_describepnet(pn1));
									net1name = returninfstr(infstr);
									infstr = initinfstr();
									par1 = pn1->network->parent;
									par2 = pn2->network->parent;
									if ((par1 == net_cell[0] && par2 == net_cell[1]) ||
										(par1 == net_cell[1] && par2 == net_cell[0]) ||
										insamecellgrp(par1, par2))
									{
										formatinfstr(infstr, _("Export names '%s:%s' and '%s:%s' do not match"),
											describenodeproto(par1), net1name,
												describenodeproto(par2), net_describepnet(pn2));
									} else
									{
										formatinfstr(infstr, _("Export names '%s:%s' and '%s:%s' are not at the same level of hierarchy"),
											describenodeproto(par1), net1name,
												describenodeproto(par2), net_describepnet(pn2));
									}
									err = logerror(returninfstr(infstr), NONODEPROTO, 0);
									net_addsymgrouptoerror(err, sg);
									(*errorcount)++;
								}
								errors |= EXPORTERRORS;
							}
						}

						/* check that the export characteristics match */
						if (pn2->realportcount > 0)
						{
							pp1 = NOPORTPROTO;
							for(i=0; i<pn1->realportcount; i++)
							{
								if (pn1->realportcount == 1) pp1 = (PORTPROTO *)pn1->realportlist; else
									pp1 = ((PORTPROTO **)pn1->realportlist)[i];
								for(j=0; j<pn2->realportcount; j++)
								{
									if (pn2->realportcount == 1) pp2 = (PORTPROTO *)pn2->realportlist; else
										pp2 = ((PORTPROTO **)pn2->realportlist)[j];
									if ((pp1->userbits&STATEBITS) == (pp2->userbits&STATEBITS)) break;
								}
								if (j < pn2->realportcount) break;
							}
							if (i >= pn1->realportcount)
							{
								if (reporterrors)
								{
									if (pn2->realportcount == 1) pp2 = (PORTPROTO *)pn2->realportlist; else
										pp2 = ((PORTPROTO **)pn2->realportlist)[0];
									infstr = initinfstr();
									formatinfstr(infstr, _("Exports have different characteristics (%s:%s is %s and %s:%s is %s)"),
										describenodeproto(net_cell[0]), pp1->protoname, describeportbits(pp1->userbits),
										describenodeproto(net_cell[1]), pp2->protoname, describeportbits(pp2->userbits));
									err = logerror(returninfstr(infstr), NONODEPROTO, 0);
									net_addsymgrouptoerror(err, sg);
									(*errorcount)++;
								}
								errors |= EXPORTERRORS;
							}
						}
					}
				} else
				{
					if ((pn2->flags&EXPORTEDNET) != 0 &&
						(pn2->realportcount > 0 || (pn2->network != NONETWORK && pn2->network->namecount > 0)))
					{
						if (checkexportnames)
						{
							/* net in cell 2 is exported, but net in cell 1 isn't */
							if (reporterrors)
							{
								infstr = initinfstr();
								formatinfstr(infstr, _("Network in cell %s is '%s' but network in cell %s is not exported"),
									describenodeproto(net_cell[1]), net_describepnet(pn2), describenodeproto(net_cell[0]));
								err = logerror(returninfstr(infstr), NONODEPROTO, 0);
								net_addsymgrouptoerror(err, sg);
								(*errorcount)++;
							}
							errors |= EXPORTERRORS;
						}
					}
				}
			}
			continue;
		}

		if (sg->cellcount[0] <= 0 || sg->cellcount[1] <= 0 || sg->hashvalue == 0)
		{
			if (sg->grouptype == SYMGROUPNET)
			{
				/* network group: ignore if no real associated networks */
				valid = FALSE;
				pn = NOPNET;
				for(f=0; f<2; f++)
				{
					for(i=0; i<sg->cellcount[f]; i++)
					{
						pn = (PNET *)sg->celllist[f][i];
						if (pn->network != NONETWORK) valid = TRUE;
					}
				}
				if (!valid) continue;

				/* network group: ignore if a bus */
				for(f=0; f<2; f++)
				{
					for(i=0; i<sg->cellcount[f]; i++)
					{
						pn = (PNET *)sg->celllist[f][i];
						net = pn->network;
						if (net == NONETWORK) continue;
						if (net->buswidth > 1) break;
					}
					if (i < sg->cellcount[f]) break;
				}
				if (f < 2) continue;

				/* network group: ignore if all power and ground that is being ignored */
				if (ignorepwrgnd)
				{
					valid = FALSE;
					for(f=0; f<2; f++)
					{
						for(i=0; i<sg->cellcount[f]; i++)
						{
							pn = (PNET *)sg->celllist[f][i];
							if ((pn->flags&(POWERNET|GROUNDNET)) == 0)
								valid = TRUE;
						}
					}
					if (!valid) continue;
				}

				/* network group: ignore if no components are on it (but warn) */
				if (sg->cellcount[0] == 0 || sg->cellcount[1] == 0)
				{
					for(f=0; f<2; f++)
					{
						if (sg->cellcount[f] == 0) continue;
						for(i=0; i<sg->cellcount[f]; i++)
						{
							pn = (PNET *)sg->celllist[f][i];
							if (pn->nodecount != 0) break;
						}
						if (i < sg->cellcount[f]) continue;
						if (reporterrors)
						{
							infstr = initinfstr();
							formatinfstr(infstr, _("Network %s in cell %s is unused"),
								net_describepnet(pn), describenodeproto(net_cell[f]));
							err = logerror(returninfstr(infstr), NONODEPROTO, 0);
							net_addsymgrouptoerror(err, sg);
							(*errorcount)++;
						}
						errors |= EXPORTERRORS;
						break;
					}
					if (f < 2) continue;
				}
			}

			/* add to the list of unassociated groups */
			sg->nexterrsymgroup = unasslist;
			unasslist = sg;
		} else
		{
			/* add to the list of ambiguous groups */
			sg->nexterrsymgroup = amblist;
			amblist = sg;
		}
	}

#ifdef NEWNCC
	/* make more thorough check if checking export names */
	if (checkexportnames)
	{
		REGISTER INTBIG c1, c2, i1, i2, chdiff;
		REGISTER PORTPROTO *pp;
		EXPORTSYMGROUP *es1, *es2;

		c1 = c2 = 0;
		for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
		{
			/* only interested in matched network groups */
			if (sg->grouptype != SYMGROUPNET) continue;
			if (sg->cellcount[0] != 1 || sg->cellcount[1] != 1) continue;
			pn1 = (PNET *)sg->celllist[0][0];
			if ((pn1->flags&EXPORTEDNET) != 0)
			{
				for(i=0; i<pn1->realportcount; i++)
				{
					if (pn1->realportcount == 1) pp = (PORTPROTO *)pn1->realportlist; else
						pp = ((PORTPROTO **)pn1->realportlist)[i];
					if (pp->network->buswidth <= 1) c1++;
				}
			}
			pn2 = (PNET *)sg->celllist[1][0];
			if ((pn2->flags&EXPORTEDNET) != 0)
			{
				for(i=0; i<pn2->realportcount; i++)
				{
					if (pn2->realportcount == 1) pp = (PORTPROTO *)pn2->realportlist; else
						pp = ((PORTPROTO **)pn2->realportlist)[i];
					if (pp->network->buswidth <= 1) c2++;
				}
			}
		}

		if (c1 != 0 && c2 != 0)
		{
			/* build array of export names */
			es1 = (EXPORTSYMGROUP *)emalloc(c1 * (sizeof (EXPORTSYMGROUP)), net_tool->cluster);
			if (es1 == 0) return(0);
			es2 = (EXPORTSYMGROUP *)emalloc(c2 * (sizeof (EXPORTSYMGROUP)), net_tool->cluster);
			if (es1 == 0) return(0);

			/* load the export lists */
			c1 = c2 = 0;
			for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
			{
				/* only interested in matched network groups */
				if (sg->grouptype != SYMGROUPNET) continue;
				if (sg->cellcount[0] != 1 || sg->cellcount[1] != 1) continue;
				pn1 = (PNET *)sg->celllist[0][0];
				if ((pn1->flags&EXPORTEDNET) != 0)
				{
					for(i=0; i<pn1->realportcount; i++)
					{
						if (pn1->realportcount == 1) pp = (PORTPROTO *)pn1->realportlist; else
							pp = ((PORTPROTO **)pn1->realportlist)[i];
						if (pp->network->buswidth > 1) continue;
						es1[c1].pp = pp;
						es1[c1].sg = sg;
						c1++;
					}
				}
				pn2 = (PNET *)sg->celllist[1][0];
				if ((pn2->flags&EXPORTEDNET) != 0)
				{
					for(i=0; i<pn2->realportcount; i++)
					{
						if (pn2->realportcount == 1) pp = (PORTPROTO *)pn2->realportlist; else
							pp = ((PORTPROTO **)pn2->realportlist)[i];
						if (pp->network->buswidth > 1) continue;
						es2[c2].pp = pp;
						es2[c2].sg = sg;
						c2++;
					}
				}
			}

			/* analyze */
			esort(es1, c1, sizeof (EXPORTSYMGROUP), net_sortexportsymgroup);
			esort(es2, c2, sizeof (EXPORTSYMGROUP), net_sortexportsymgroup);

			i1 = i2 = 0;
			for(;;)
			{
				if (i1 >= c1 || i2 >= c2) break;
				chdiff = namesame(es1[i1].pp->protoname, es2[i2].pp->protoname);
				if (chdiff == 0)
				{
					/* same name: make sure symgroups are the same */
					if (es1[i1].sg != es2[i2].sg)
					{
						if (reporterrors)
						{
							infstr = initinfstr();
							formatinfstr(infstr, _("Export %s not wired consistently"),
								es1[i1].pp->protoname);
							err = logerror(returninfstr(infstr), NONODEPROTO, 0);
							addexporttoerror(err, es1[i1].pp, TRUE);
							addexporttoerror(err, es2[i2].pp, TRUE);
							(*errorcount)++;
						}
						errors |= EXPORTERRORS;
					}
					i1++;
					i2++;
					continue;
				}
				if (chdiff < 0) i1++; else
					i2++;
			}
			efree((CHAR *)es1);
			efree((CHAR *)es2);
		}
	}
#endif

	if (unasslist != NOSYMGROUP || amblist != NOSYMGROUP)
	{
		if (reporterrors)
		{
			if (unasslist != NOSYMGROUP)
				*errorcount += net_reporterror(unasslist, _("Unassociated"), ignorepwrgnd);
			if (amblist != NOSYMGROUP)
				*errorcount += net_reporterror(amblist, _("Ambiguous"), ignorepwrgnd);
		}
		errors |= STRUCTUREERRORS;
	}

	/* if reporting errors, look for groups with missing parts */
	if (reporterrors)
	{
		pnlisttotal[0] = pnlisttotal[1] = 0;
		for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
		{
			if (sg->cellcount[0] <= 0 || sg->cellcount[1] <= 0) continue;
			if (sg->grouptype != SYMGROUPNET) continue;
			if (sg->cellcount[0] == 1 && sg->cellcount[1] == 1) continue;

			/* first make a list of the networks in both cells */
			for(f=0; f<2; f++)
			{
				if (sg->cellcount[f] > pnlisttotal[f])
				{
					if (pnlisttotal[f] > 0) efree((CHAR *)pnlist[f]);
					pnlist[f] = (PNET **)emalloc(sg->cellcount[f] * (sizeof (PNET *)), el_tempcluster);
					if (pnlist[f] == 0) return(errors);
					pnlisttotal[f] = sg->cellcount[f];
				}
				for(i=0; i<sg->cellcount[f]; i++)
					pnlist[f][i] = (PNET *)sg->celllist[f][i];
			}

			/* sort the names and look for matches */
			esort(pnlist[0], sg->cellcount[0], sizeof (PNET *), net_sortbypnet);
			esort(pnlist[1], sg->cellcount[1], sizeof (PNET *), net_sortbypnet);
			i1 = i2 = 0;
			for(;;)
			{
				if (i1 >= sg->cellcount[0] || i2 >= sg->cellcount[1]) break;
				pn1 = pnlist[0][i1];
				pn2 = pnlist[1][i2];
				if (pn1 == NOPNET || pn2 == NOPNET) break;
				pt1 = net_describepnet(pn1);
				pt2 = net_describepnet(pn2);
				i = namesame(pt1, pt2);
				if (i < 0)
				{
					i1++;
					continue;
				}
				if (i > 0)
				{
					i2++;
					continue;
				}
				pnlist[0][i1] = NOPNET;
				while (i1+1 < sg->cellcount[0])
				{
					pn = pnlist[0][i1+1];
					if (pn == NOPNET) break;
					if (namesame(net_describepnet(pn1), net_describepnet(pn)) != 0) break;
					i1++;
					pnlist[0][i1] = NOPNET;
				}
				pnlist[1][i2] = NOPNET;
				while (i2+1 < sg->cellcount[1])
				{
					pn = pnlist[1][i2+1];
					if (pn == NOPNET) break;
					if (namesame(net_describepnet(pn2), net_describepnet(pn)) != 0) break;
					i2++;
					pnlist[1][i2] = NOPNET;
				}
			}

			/* see if one entire side is eliminated */
			for(f=0; f<2; f++)
			{
				for(i=0; i<sg->cellcount[f]; i++)
					if (pnlist[f][i] != NOPNET) break;
				if (i >= sg->cellcount[f]) break;
			}
			if (f < 2)
			{
				/* side "f" is eliminated: find name matches to those on side "1-f" */
				for(j=0; j<sg->cellcount[1-f]; j++)
				{
					opn = (PNET *)sg->celllist[1-f][j];
					if (opn == NOPNET) continue;
					if (opn->network == NONETWORK) continue;
					for(osg = net_firstsymgroup; osg != NOSYMGROUP; osg = osg->nextsymgroup)
					{
						if (osg->grouptype != SYMGROUPNET) continue;
						if (osg == sg) continue;
						for(i=0; i<osg->cellcount[f]; i++)
						{
							pn = (PNET *)osg->celllist[f][i];
							if (opn->nodecount == pn->nodecount) continue;
							if (pn->network == NONETWORK) continue;
							pt1 = net_describepnet(opn);
							pt2 = net_describepnet(pn);
							if (namesame(pt1, pt2) == 0)
							{
								/* found it! */
								infstr = initinfstr();
								formatinfstr(infstr, _("Networks %s may be wired differently"),
									net_describepnet(pn));
								err = logerror(returninfstr(infstr), NONODEPROTO, 0);
								for(ai = pn->network->parent->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
									if (ai->network == pn->network)
										addgeomtoerror(err, ai->geom, TRUE, 0, 0);
								for(ai = opn->network->parent->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
									if (ai->network == opn->network)
										addgeomtoerror(err, ai->geom, TRUE, 0, 0);
								(*errorcount)++;
								break;
							}
						}
						if (i < osg->cellcount[f]) break;
					}
				}
			}
		}
		for(f=0; f<2; f++) if (pnlisttotal[f] > 0)
			efree((CHAR *)pnlist[f]);
	}
	if (reporterrors && worstpctdiff != 0)
		ttyputmsg(_("Worst size difference is %ld%%"), worstpctdiff);
	return(errors);
}

/*
 * Routine to report a size error between component "pc1" with size "size1" and component "pc2" with
 * size "size2".  The error is "pctdiff" percent, and it comes from symmetry group "sg".
 */
void net_reportsizeerror(PCOMP *pc1, CHAR *size1, PCOMP *pc2, CHAR *size2, INTBIG pctdiff, SYMGROUP *sg)
{
	REGISTER void *infstr, *err;

	infstr = initinfstr();
	formatinfstr(infstr, _("Node sizes differ by %ld%% ("), pctdiff);
	if (pc1->numactual > 1)
	{
		formatinfstr(infstr, _("cell %s has %s from %ld transistors"), describenodeproto(net_cell[0]),
			size1, pc1->numactual);
	} else
	{
		formatinfstr(infstr, _("cell %s has %s"), describenodeproto(net_cell[0]), size1);
	}
	addstringtoinfstr(infstr, _(", but "));
	if (pc2->numactual > 1)
	{
		formatinfstr(infstr, _("cell %s has %s from %ld transistors"), describenodeproto(net_cell[1]),
			size2, pc2->numactual);
	} else
	{
		formatinfstr(infstr, _("cell %s has %s"), describenodeproto(net_cell[1]), size2);
	}
	addstringtoinfstr(infstr, x_(")"));
	err = logerror(returninfstr(infstr), NONODEPROTO, 0);
	net_addsymgrouptoerror(err, sg);
}

/*
 * Routine to report the list of groups starting at "sg" as errors of type "errmsg".
 * Returns the number of errors reported.
 */
INTBIG net_reporterror(SYMGROUP *sg, CHAR *errmsg, BOOLEAN ignorepwrgnd)
{
	REGISTER INTBIG i, f, oi, of, errorsfound;
	REGISTER PCOMP *pc, *opc;
	REGISTER PNET *pn, *opn;
	REGISTER void *infstr, *err;
	REGISTER CHAR *segue;
	CHAR errormessage[200];
	REGISTER NODEPROTO *lastcell;
	REGISTER NETWORK *net;

	errorsfound = 0;
	for( ; sg != NOSYMGROUP; sg = sg->nexterrsymgroup)
	{
		switch (sg->grouptype)
		{
			case SYMGROUPCOMP:
				for(f=0; f<2; f++)
				{
					for(i=0; i<sg->cellcount[f]; i++)
					{
						pc = (PCOMP *)sg->celllist[f][i];
						if (pc->timestamp <= 0) continue;
						esnprintf(errormessage, 200, _("%s nodes"), errmsg);
						err = logerror(errormessage, NONODEPROTO, 0);
						net_addcomptoerror(err, pc);
						for(of=0; of<2; of++)
						{
							for(oi=0; oi<sg->cellcount[of]; oi++)
							{
								opc = (PCOMP *)sg->celllist[of][oi];
								if (opc == pc) continue;
								if (opc->timestamp != pc->timestamp) continue;
								net_addcomptoerror(err, opc);
								opc->timestamp = -opc->timestamp;
							}
						}
						pc->timestamp = -pc->timestamp;
						errorsfound++;
					}
				}
				for(f=0; f<2; f++)
				{
					for(i=0; i<sg->cellcount[f]; i++)
					{
						pc = (PCOMP *)sg->celllist[f][i];
						pc->timestamp = -pc->timestamp;
					}
				}
				break;
			case SYMGROUPNET:
				for(f=0; f<2; f++)
				{
					for(i=0; i<sg->cellcount[f]; i++)
					{
						pn = (PNET *)sg->celllist[f][i];
						if (pn->timestamp <= 0) continue;
						if (ignorepwrgnd && (pn->flags&(POWERNET|GROUNDNET)) != 0)
							continue;

						/* build the error message */
						infstr = initinfstr();
						formatinfstr(infstr, _("%s networks"), errmsg);
						segue = x_(": ");
						for(of=0; of<2; of++)
						{
							lastcell = NONODEPROTO;
							for(oi=0; oi<sg->cellcount[of]; oi++)
							{
								opn = (PNET *)sg->celllist[of][oi];
								if (opn->timestamp != pn->timestamp) continue;
								if (ignorepwrgnd && (opn->flags&(POWERNET|GROUNDNET)) != 0)
									continue;
								net = opn->network;
								if (net == NONETWORK) continue;
								if (lastcell != net->parent)
								{
									addstringtoinfstr(infstr, segue);
									segue = x_("; ");
									formatinfstr(infstr, _("from cell %s:"),
										describenodeproto(net->parent));
								} else addtoinfstr(infstr, ',');
								lastcell = net->parent;
								formatinfstr(infstr, x_(" %s (%ld connections)"), describenetwork(net), opn->nodecount);
							}
						}

						/* report the error */
						err = logerror(returninfstr(infstr), NONODEPROTO, 0);
						net_addnettoerror(err, pn);
						for(of=0; of<2; of++)
						{
							for(oi=0; oi<sg->cellcount[of]; oi++)
							{
								opn = (PNET *)sg->celllist[of][oi];
								if (opn == pn) continue;
								if (opn->timestamp != pn->timestamp) continue;
								if (ignorepwrgnd && (opn->flags&(POWERNET|GROUNDNET)) != 0)
									continue;
								net_addnettoerror(err, opn);
								opn->timestamp = -opn->timestamp;
							}
						}
						pn->timestamp = -pn->timestamp;
						errorsfound++;
					}
				}
				for(f=0; f<2; f++)
				{
					for(i=0; i<sg->cellcount[f]; i++)
					{
						pn = (PNET *)sg->celllist[f][i];
						pn->timestamp = -pn->timestamp;
					}
				}
				break;
		}
	}
	return(errorsfound);
}

#define PRECISEBUSMATCH 1

/*
 * Routine to return true if the exported ports on "pn1" and "pn2" match.
 */
BOOLEAN net_sameexportnames(PNET *pn1, PNET *pn2)
{
	REGISTER INTBIG i, j, c1, c2, nc1, nc2, c1ind, c1sig, c2ind, c2sig;
	REGISTER CHAR *name1, *name2;
	REGISTER PORTPROTO *pp1, *pp2;

	/* determine the number of signal names on net 1 */
	c1 = 0;
	for(i=0; i<pn1->realportcount; i++)
	{
		if (pn1->realportcount == 1) pp1 = (PORTPROTO *)pn1->realportlist; else
			pp1 = ((PORTPROTO **)pn1->realportlist)[i];
		if (pp1->network->buswidth <= 1) c1++;
#ifndef PRECISEBUSMATCH
			else c1 += pp1->network->buswidth;
#endif
	}
	if (pn1->network == NONETWORK) nc1 = 0; else
		nc1 = pn1->network->namecount;

	/* determine the number of signal names on net 2 */
	c2 = 0;
	for(i=0; i<pn2->realportcount; i++)
	{
		if (pn2->realportcount == 1) pp2 = (PORTPROTO *)pn2->realportlist; else
			pp2 = ((PORTPROTO **)pn2->realportlist)[i];
		if (pp2->network->buswidth <= 1) c2++;
#ifndef PRECISEBUSMATCH
			else c2 += pp2->network->buswidth;
#endif
	}
	if (pn2->network == NONETWORK) nc2 = 0; else
		nc2 = pn2->network->namecount;

	c1ind = c1sig = 0;
	for(i=0; i<nc1+c1; i++)
	{
		if (i < nc1)
		{
			name1 = networkname(pn1->network, i);
		} else
		{
			if (pn1->realportcount == 1) pp1 = (PORTPROTO *)pn1->realportlist; else
				pp1 = ((PORTPROTO **)pn1->realportlist)[c1ind];
			if (pp1->network->buswidth <= 1)
			{
				name1 = pp1->protoname;
				c1ind++;
				c1sig = 0;
#ifndef PRECISEBUSMATCH
			} else
			{
				name1 = networkname(pp1->network->networklist[c1sig++], 0);
				if (c1sig >= pp1->network->buswidth)
				{
					c1ind++;
					c1sig = 0;
				}
#endif
			}
		}

		c2ind = c2sig = 0;
		for(j=0; j<nc2+c2; j++)
		{
			if (j < nc2)
			{
				name2 = networkname(pn2->network, j);
			} else
			{
				if (pn2->realportcount == 1) pp2 = (PORTPROTO *)pn2->realportlist; else
					pp2 = ((PORTPROTO **)pn2->realportlist)[c2ind];
				if (pp2->network->buswidth <= 1)
				{
					name2 = pp2->protoname;
					c2ind++;
					c2sig = 0;
#ifndef PRECISEBUSMATCH
				} else
				{
					name2 = networkname(pp2->network->networklist[c2sig++], 0);
					if (c2sig >= pp2->network->buswidth)
					{
						c2ind++;
						c2sig = 0;
					}
#endif
				}
			}
			if (namesame(name1, name2) == 0) return(TRUE);
		}
	}
	return(FALSE);
}

/*
 * Routine to report the number of unmatched networks and components.
 */
void net_unmatchedstatus(INTBIG *unmatchednets, INTBIG *unmatchedcomps, INTBIG *symgroupcount)
{
	REGISTER INTBIG f;
	REGISTER SYMGROUP *sg;

	*unmatchednets = *unmatchedcomps = *symgroupcount = 0;
	for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
	{
		if (sg->cellcount[0] == 0 && sg->cellcount[1] == 0) continue;
		(*symgroupcount)++;
		if (sg->cellcount[0] == 1 && sg->cellcount[1] == 1) continue;
		for(f=0; f<2; f++)
		{
			if (sg->grouptype == SYMGROUPCOMP)
			{
				*unmatchedcomps += sg->cellcount[f];
			} else
			{
				*unmatchednets += sg->cellcount[f];
			}
		}
	}
	for(sg = net_firstmatchedsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
		(*symgroupcount)++;
}

void net_addcomptoerror(void *err, PCOMP *pc)
{
	REGISTER NODEINST *ni;
	REGISTER INTBIG i;

	for(i=0; i<pc->numactual; i++)
	{
		if (pc->numactual == 1) ni = (NODEINST *)pc->actuallist; else
			ni = ((NODEINST **)pc->actuallist)[i];
		addgeomtoerror(err, ni->geom, TRUE, pc->hierpathcount, pc->hierpath);
	}
}

void net_addnettoerror(void *err, PNET *pn)
{
	REGISTER ARCINST *ai;
	REGISTER NETWORK *net, *anet;
	REGISTER PORTPROTO *pp;
	REGISTER NODEPROTO *np;
	REGISTER BOOLEAN found;
	REGISTER INTBIG i;

	found = FALSE;
	net = pn->network;
	if (net == NONETWORK) return;
	np = net->parent;
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		anet = ai->network;
		if (ai->proto == sch_busarc)
		{
			if (anet->buswidth > 1)
			{
				for(i=0; i<anet->buswidth; i++)
					if (anet->networklist[i] == net) break;
				if (i >= anet->buswidth) continue;
			} else
			{
				if (anet != net) continue;
			}
		} else
		{
			if (anet != net) continue;
		}
		addgeomtoerror(err, ai->geom, TRUE, 0, 0);
		found = TRUE;
	}
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		if (pp->network != net) continue;
		addexporttoerror(err, pp, TRUE);
		found = TRUE;
	}
	if (!found && net->namecount > 0)
	{
		if (np == net_cell[0]) np = net_cell[1]; else
			np = net_cell[0];
		net = getnetwork(networkname(net, 0), np);
		if (net == NONETWORK) return;
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		{
			if (ai->network != net) continue;
			addgeomtoerror(err, ai->geom, TRUE, 0, 0);
		}
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			if (pp->network != net) continue;
			addexporttoerror(err, pp, TRUE);
		}
	}
}

/*
 * Routine to add all objects in symmetry group "sg" to the error report "err".
 */
void net_addsymgrouptoerror(void *err, SYMGROUP *sg)
{
	REGISTER INTBIG i, f;
	REGISTER PCOMP *pc;
	REGISTER PNET *pn;

	switch (sg->grouptype)
	{
		case SYMGROUPCOMP:
			for(f=0; f<2; f++)
			{
				for(i=0; i<sg->cellcount[f]; i++)
				{
					pc = (PCOMP *)sg->celllist[f][i];
					net_addcomptoerror(err, pc);
				}
			}
			break;
		case SYMGROUPNET:
			for(f=0; f<2; f++)
			{
				for(i=0; i<sg->cellcount[f]; i++)
				{
					pn = (PNET *)sg->celllist[f][i];
					net_addnettoerror(err, pn);
				}
			}
			break;
	}
}

/******************************** RESOLVING AMBIGUITY ********************************/

/*
 * Routine to look for matches in ambiguous symmetry groups.
 * Returns 1 if a component group is split; -1 if a network group is split;
 * zero if no split was found.
 */
INTBIG net_findamatch(INTBIG verbose, BOOLEAN ignorepwrgnd)
{
	REGISTER SYMGROUP *sg, *osg;
	REGISTER PNET *pn;
	REGISTER PCOMP *pc;
	NETWORK *nets[2];
	REGISTER INTBIG i, f, u, any, total, newtype;
	INTBIG is[2], unmatchednets, unmatchedcomps, symgroupcount;
	CHAR uniquename[30];
	float sizew, sizel;
	REGISTER NODEINST *ni;
	NODEPROTO *localcell[2];
	REGISTER ARCINST *ai;
	NODEINST *nis[2];
	REGISTER VARIABLE *var, *var0, *var1;

	/* determine the number of unmatched nets and components */
	net_unmatchedstatus(&unmatchednets, &unmatchedcomps, &symgroupcount);

	/* prepare a list of ambiguous symmetry groups */
	total = 0;
	for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
	{
		if (sg->hashvalue == 0) continue;
#ifdef NEWNCC
		if (sg->cellcount[0] < 2 && sg->cellcount[1] < 2) continue;
#else
		if (sg->cellcount[0] < 2 || sg->cellcount[1] < 2) continue;
#endif
		total++;
	}
	if (total > net_symgrouplisttotal)
	{
		if (net_symgrouplisttotal > 0) efree((CHAR *)net_symgrouplist);
		net_symgrouplisttotal = 0;
		net_symgrouplist = (SYMGROUP **)emalloc(total * (sizeof (SYMGROUP *)),
			net_tool->cluster);
		if (net_symgrouplist == 0) return(0);
		net_symgrouplisttotal = total;
	}
	total = 0;
	for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
	{
		if (sg->hashvalue == 0) continue;
#ifdef NEWNCC
		if (sg->cellcount[0] < 2 && sg->cellcount[1] < 2) continue;
#else
		if (sg->cellcount[0] < 2 || sg->cellcount[1] < 2) continue;
#endif
		net_symgrouplist[total++] = sg;
	}

	/* sort by size of ambiguity */
	esort(net_symgrouplist, total, sizeof (SYMGROUP *), net_sortsymgroups);

	/* now look through the groups, starting with the smallest */
	for(i=0; i<total; i++)
	{
		sg = net_symgrouplist[i];

		if (sg->grouptype == SYMGROUPNET)
		{
			/* look for export names that are the same */
			if (net_findexportnamematch(sg, verbose, ignorepwrgnd,
				symgroupcount, unmatchednets, unmatchedcomps)) return(-1);

			/* look for network names that are the same */
			if (net_findnetworknamematch(sg, FALSE, verbose, ignorepwrgnd,
				symgroupcount, unmatchednets, unmatchedcomps)) return(-1);
		} else
		{
			/* look for nodes that are uniquely the same size */
			if (net_findcommonsizefactor(sg, &sizew, &sizel))
			{
				ttyputmsg(_("--- Forcing a match based on size %s nodes (%ld symmetry groups with %ld nets and %ld nodes unmatched)"),
					net_describesizefactor(sizew, sizel), symgroupcount, unmatchednets, unmatchedcomps);
				net_forceamatch(sg, 0, 0, 0, 0, sizew, sizel, verbose, ignorepwrgnd);
				return(1);
			}

			/* look for node names that are the same */
			if (net_findcomponentnamematch(sg, FALSE, verbose, ignorepwrgnd,
				symgroupcount, unmatchednets, unmatchedcomps)) return(1);
		}
	}

	/* now look for pseudo-matches with the "NCCMatch" tags */
	for(i=0; i<total; i++)
	{
		sg = net_symgrouplist[i];

		if (sg->grouptype == SYMGROUPNET)
		{
			/* look for network names that are the same */
			if (net_findnetworknamematch(sg, TRUE, verbose, ignorepwrgnd,
				symgroupcount, unmatchednets, unmatchedcomps)) return(-1);
		} else
		{
			/* look for node names that are the same */
			if (net_findcomponentnamematch(sg, TRUE, verbose, ignorepwrgnd,
				symgroupcount, unmatchednets, unmatchedcomps)) return(1);
		}
	}

	/* random match: look again through the groups, starting with the smallest */
	for(i=0; i<total; i++)
	{
		sg = net_symgrouplist[i];
		if (sg->cellcount[0] <= 0 || sg->cellcount[1] <= 0) continue;

		if (sg->grouptype == SYMGROUPCOMP)
		{
			for(f=0; f<2; f++)
			{
				for(is[f]=0; is[f] < sg->cellcount[f]; is[f]++)
				{
					pc = (PCOMP *)sg->celllist[f][is[f]];
					if (pc->numactual == 1) nis[f] = (NODEINST *)pc->actuallist; else
						nis[f] = ((NODEINST **)pc->actuallist)[0];
					var = getvalkey((INTBIG)nis[f], VNODEINST, VSTRING, el_node_name_key);
					if (var == NOVARIABLE) break;
					if ((var->type&VDISPLAY) == 0) break;
				}
				if (is[f] >= sg->cellcount[f])
				{
					is[f] = 0;
					pc = (PCOMP *)sg->celllist[f][is[f]];
					if (pc->numactual == 1) nis[f] = (NODEINST *)pc->actuallist; else
						nis[f] = ((NODEINST **)pc->actuallist)[0];
				}
			}

			/* copy "NCCMatch" information if possible */
			localcell[0] = nis[0]->parent;
			localcell[1] = nis[1]->parent;
			var0 = getvalkey((INTBIG)nis[0], VNODEINST, VSTRING, net_ncc_matchkey);
			var1 = getvalkey((INTBIG)nis[1], VNODEINST, VSTRING, net_ncc_matchkey);
			if (var0 != NOVARIABLE && var1 != NOVARIABLE)
			{
				/* both have a name: warn if different */
				if (namesame((CHAR *)var0->addr, (CHAR *)var1->addr) != 0)
				{
					ttyputmsg(x_("WARNING: want to match nodes %s:s and %s:%s but they are already tagged '%s' and '%s'"),
						describenodeproto(localcell[0]), describenodeinst(nis[0]),
						describenodeproto(localcell[1]), describenodeinst(nis[1]),
						(CHAR *)var0->addr, (CHAR *)var1->addr);
				}
				var0 = var1 = NOVARIABLE;
			}
			if (var0 == NOVARIABLE && var1 != NOVARIABLE)
			{
				/* node in cell 0 has no name, see if it can take the name from cell 1 */
				estrcpy(uniquename, (CHAR *)var1->addr);
				for(ni = localcell[0]->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				{
					var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, net_ncc_matchkey);
					if (var == NOVARIABLE) continue;
					if (namesame((CHAR *)var->addr, uniquename) == 0) break;
				}
				if (ni != NONODEINST) var1 = NOVARIABLE; else
				{
					/* copy from cell 1 to cell 0 */
					startobjectchange((INTBIG)nis[0], VNODEINST);
					newtype = VSTRING;
					if ((net_ncc_options&NCCHIDEMATCHTAGS) == 0) newtype |= VDISPLAY;
					var = setvalkey((INTBIG)nis[0], VNODEINST, net_ncc_matchkey,
						(INTBIG)uniquename, newtype);
					if (var != NOVARIABLE)
						defaulttextsize(3, var->textdescript);
					endobjectchange((INTBIG)nis[0], VNODEINST);
				}
			}
			if (var0 != NOVARIABLE && var1 == NOVARIABLE)
			{
				/* node in cell 1 has no name, see if it can take the name from cell 0 */
				estrcpy(uniquename, (CHAR *)var0->addr);
				for(ni = localcell[1]->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				{
					var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, net_ncc_matchkey);
					if (var == NOVARIABLE) continue;
					if (namesame((CHAR *)var->addr, uniquename) == 0) break;
				}
				if (ni != NONODEINST) var0 = NOVARIABLE; else
				{
					/* copy from cell 0 to cell 1 */
					startobjectchange((INTBIG)nis[1], VNODEINST);
					newtype = VSTRING;
					if ((net_ncc_options&NCCHIDEMATCHTAGS) == 0) newtype |= VDISPLAY;
					var = setvalkey((INTBIG)nis[1], VNODEINST, net_ncc_matchkey,
						(INTBIG)uniquename, newtype);
					if (var != NOVARIABLE)
						defaulttextsize(3, var->textdescript);
					endobjectchange((INTBIG)nis[1], VNODEINST);
				}
			}
			if (var0 == NOVARIABLE && var1 == NOVARIABLE)
			{
				/* neither has a name: find a unique name and tag the selected nodes */
				for(u=1; ; u++)
				{
					esnprintf(uniquename, 30, x_("NCCmatch%ld"), u);
					for(f=0; f<2; f++)
					{
						for(ni = localcell[f]->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
						{
							var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, net_ncc_matchkey);
							if (var == NOVARIABLE) continue;
							if (namesame((CHAR *)var->addr, uniquename) == 0) break;
						}
						if (ni != NONODEINST) break;
					}
					if (f >= 2) break;
				}
				for(f=0; f<2; f++)
				{
					startobjectchange((INTBIG)nis[f], VNODEINST);
					newtype = VSTRING;
					if ((net_ncc_options&NCCHIDEMATCHTAGS) == 0) newtype |= VDISPLAY;
					var = setvalkey((INTBIG)nis[f], VNODEINST, net_ncc_matchkey,
						(INTBIG)uniquename, newtype);
					if (var != NOVARIABLE)
						defaulttextsize(3, var->textdescript);
					endobjectchange((INTBIG)nis[f], VNODEINST);
				}
			}
			if ((net_ncc_options&NCCSUPALLAMBREP) == 0)
			{
				ttyputmsg(_("--- Forcing a random match of nodes '%s:%s' and '%s:%s', called %s (%ld symmetry groups with %ld nets and %ld nodes unmatched)"),
					describenodeproto(localcell[0]), describenodeinst(nis[0]),
						describenodeproto(localcell[1]), describenodeinst(nis[1]),
							uniquename, symgroupcount, unmatchednets, unmatchedcomps);
			}
			net_forceamatch(sg, 1, &is[0], 1, &is[1], 0.0, 0.0, verbose, ignorepwrgnd);
			return(1);
		} else
		{
			/* look for any ambiguous networks and randomly match them */
			for(f=0; f<2; f++)
			{
				any = -1;
				for(is[f]=0; is[f] < sg->cellcount[f]; is[f]++)
				{
					pn = (PNET *)sg->celllist[f][is[f]];
					nets[f] = pn->network;
					if (nets[f] == NONETWORK) continue;
					any = is[f];
					if (nets[f]->namecount == 0 ||
						nets[f]->tempname != 0) break;
				}
				if (is[f] >= sg->cellcount[f])
				{
					if (any < 0) nets[f] = NONETWORK; else
					{
						is[f] = any;
						pn = (PNET *)sg->celllist[f][any];
						nets[f] = pn->network;
					}
				}
			}
			if (nets[0] != NONETWORK && nets[1] != NONETWORK)
			{
				/* find a unique name and tag the selected networks */
				for(u=1; ; u++)
				{
					esnprintf(uniquename, 30, x_("NCCmatch%ld"), u);
					for(f=0; f<2; f++)
					{
						for(ai = nets[f]->parent->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
						{
							var = getvalkey((INTBIG)ai, VARCINST, VSTRING, net_ncc_matchkey);
							if (var == NOVARIABLE) continue;
							if (namesame(uniquename, (CHAR *)var->addr) == 0) break;
						}
						if (ai != NOARCINST) break;
					}
					if (f >= 2) break;
				}
				for(f=0; f<2; f++)
				{
					for(ai = nets[f]->parent->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
					{
						if (ai->network != nets[f]) continue;
						startobjectchange((INTBIG)ai, VARCINST);
						newtype = VSTRING;
						if ((net_ncc_options&NCCHIDEMATCHTAGS) == 0) newtype |= VDISPLAY;
						var = setvalkey((INTBIG)ai, VARCINST, net_ncc_matchkey,
							(INTBIG)uniquename, newtype);
						if (var != NOVARIABLE)
							defaulttextsize(4, var->textdescript);
						endobjectchange((INTBIG)ai, VARCINST);

						/* pickup new net number and remember it in the data structures */
						for(osg = net_firstsymgroup; osg != NOSYMGROUP; osg = osg->nextsymgroup)
						{
							if (osg->grouptype == SYMGROUPCOMP) continue;
							for(i=0; i<osg->cellcount[f]; i++)
							{
								pn = (PNET *)osg->celllist[f][i];
								if (pn->network == nets[f])
									pn->network = ai->network;
							}
						}
						nets[f] = ai->network;
						break;
					}
				}

				if ((net_ncc_options&NCCSUPALLAMBREP) == 0)
				{
					ttyputmsg(_("--- Forcing a random match of networks '%s' in cells %s and %s (%ld symmetry groups with %ld nets and %ld nodes unmatched)"),
						uniquename, describenodeproto(nets[0]->parent), describenodeproto(nets[1]->parent),
							symgroupcount, unmatchednets, unmatchedcomps);
				}
				net_forceamatch(sg, 1, &is[0], 1, &is[1], 0.0, 0.0, verbose, ignorepwrgnd);
				return(-1);
			}
		}
	}

	return(0);
}

/*
 * Routine to look through ambiguous symmetry group "sg" for export names that cause a match.
 * If one is found, it is reported and matched and the routine returns true.
 * Flags "verbose" and "ignorepwrgnd" apply to the match.  Tallys "total", "unmatchednets",
 * and "unmatchedcomps" are reported.
 */
BOOLEAN net_findexportnamematch(SYMGROUP *sg, INTBIG verbose, BOOLEAN ignorepwrgnd,
	INTBIG total, INTBIG unmatchednets, INTBIG unmatchedcomps)
{
	REGISTER INTBIG f, i0, i1, i, ip, i0base, i1base, comp;
	INTBIG count[2];
	REGISTER PNET *pn;
	REGISTER PORTPROTO *pp;

	/* build a list of all export names */
	for(f=0; f<2; f++)
	{
		count[f] = 0;
		for(i=0; i<sg->cellcount[f]; i++)
		{
			pn = (PNET *)sg->celllist[f][i];
			for(ip=0; ip<pn->realportcount; ip++)
			{
				if (pn->realportcount == 1) pp = (PORTPROTO *)pn->realportlist; else
					pp = ((PORTPROTO **)pn->realportlist)[ip];
				net_addtonamematch(&net_namematch[f], &net_namematchtotal[f], &count[f],
					pp->protoname, i, NONODEINST);
			}
		}
		esort(net_namematch[f], count[f], sizeof(NAMEMATCH), net_sortnamematches);
	}

	/* now look for unique matches */
	i0 = i1 = 0;
	for(;;)
	{
		if (i0 >= count[0] || i1 >= count[1]) break;
		comp = namesame(net_namematch[0][i0].name, net_namematch[1][i1].name);
		i0base = i0;   i1base = i1;
		while (i0+1 < count[0] &&
			namesame(net_namematch[0][i0].name, net_namematch[0][i0+1].name) == 0)
				i0++;
		while (i1+1 < count[1] &&
			namesame(net_namematch[1][i1].name, net_namematch[1][i1+1].name) == 0)
				i1++;
		if (comp == 0)
		{
			if (i0 == i0base && i1 == i1base)
			{
				/* found a unique match */
				ttyputmsg(_("--- Forcing a match based on the export name '%s' (%ld symmetry groups with %ld nets and %ld nodes unmatched)"),
					net_namematch[0][i0].name, total, unmatchednets, unmatchedcomps);
				net_forceamatch(sg, 1, &net_namematch[0][i0].number, 1, &net_namematch[1][i1].number,
					0.0, 0.0, verbose, ignorepwrgnd);
				return(TRUE);
			}
			i0++;   i1++;
		} else
		{
			if (comp < 0) i0++; else i1++;
		}
	}
	return(FALSE);
}

BOOLEAN net_findpowerandgroundmatch(SYMGROUP *sg, INTBIG verbose, INTBIG total, INTBIG unmatchednets, INTBIG unmatchedcomps)
{
	REGISTER INTBIG f, i, power[2], ground[2];
	REGISTER PNET *pn;

	power[0] = power[1] = ground[0] = ground[1] = -1;
	for(f=0; f<2; f++)
	{
		for(i=0; i<sg->cellcount[f]; i++)
		{
			pn = (PNET *)sg->celllist[f][i];
			if ((pn->flags&POWERNET) != 0)
			{
				if (power[f] == -1) power[f] = i; else power[f] = -2;
			}
			if ((pn->flags&GROUNDNET) != 0)
			{
				if (ground[f] == -1) ground[f] = i; else ground[f] = -2;
			}
		}
	}

	/* if there is exactly 1 power net in each side, disambiguate it */
	if (power[0] >= 0 && power[1] >= 0)
	{
		/* found a unique match */
		ttyputmsg(_("--- Forcing a match based on the power characteristic (%ld symmetry groups with %ld nets and %ld nodes unmatched)"),
			total, unmatchednets, unmatchedcomps);
		net_forceamatch(sg, 1, &power[0], 1, &power[1], 0.0, 0.0, verbose, FALSE);
		return(TRUE);
	}

	/* if there is exactly 1 ground net in each side, disambiguate it */
	if (ground[0] >= 0 && ground[1] >= 0)
	{
		/* found a unique match */
		ttyputmsg(_("--- Forcing a match based on the ground characteristic (%ld symmetry groups with %ld nets and %ld nodes unmatched)"),
			total, unmatchednets, unmatchedcomps);
		net_forceamatch(sg, 1, &ground[0], 1, &ground[1], 0.0, 0.0, verbose, FALSE);
		return(TRUE);
	}
	return(FALSE);
}

/*
 * Routine to look through ambiguous symmetry group "sg" for network names that cause a match.
 * If one is found, it is reported and matched and the routine returns true.
 * If "usenccmatches" is true, allow "NCCMatch" tags.
 * If "allowpseudonames" is zero and such names are found, the routine returns true.
 * Flags "verbose" and "ignorepwrgnd" apply to the match.  Tallys "total", "unmatchednets",
 * and "unmatchedcomps" are reported.
 */
BOOLEAN net_findnetworknamematch(SYMGROUP *sg, BOOLEAN usenccmatches, INTBIG verbose,
	BOOLEAN ignorepwrgnd, INTBIG total, INTBIG unmatchednets, INTBIG unmatchedcomps)
{
	REGISTER INTBIG i0, i1, i, ip, f, comp, i0base, i1base;
	INTBIG count[2];
	REGISTER BOOLEAN foundexport;
	REGISTER PNET *pn;
	REGISTER VARIABLE *var;
	REGISTER ARCINST *ai;
	REGISTER NETWORK *net;
	REGISTER CHAR *netname;

	/* build a list of all network names */
	foundexport = FALSE;
	for(f=0; f<2; f++)
	{
		count[f] = 0;
		for(i=0; i<sg->cellcount[f]; i++)
		{
			pn = (PNET *)sg->celllist[f][i];
			net = pn->network;
			if (net == NONETWORK) continue;
			if ((pn->flags&EXPORTEDNET) != 0) foundexport = TRUE;
			if (usenccmatches)
			{
				for(ai = net_cell[f]->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
				{
					if (ai->network != net) continue;
					var = getvalkey((INTBIG)ai, VARCINST, VSTRING, net_ncc_matchkey);
					if (var == NOVARIABLE) continue;
					net_addtonamematch(&net_namematch[f], &net_namematchtotal[f], &count[f],
						(CHAR *)var->addr, i, NONODEINST);
				}
			} else
			{
				if (net->namecount == 0 || net->tempname != 0) continue;
				for(ip=0; ip<net->namecount; ip++)
				{
					netname = networkname(net, ip);
					net_addtonamematch(&net_namematch[f], &net_namematchtotal[f], &count[f],
						netname, i, NONODEINST);
				}
			}
		}
		esort(net_namematch[f], count[f], sizeof(NAMEMATCH), net_sortnamematches);
	}

	/* now look for unique matches */
	i0 = i1 = 0;
	for(;;)
	{
		if (i0 >= count[0] || i1 >= count[1]) break;
		comp = namesame(net_namematch[0][i0].name, net_namematch[1][i1].name);
		i0base = i0;   i1base = i1;
		while (i0+1 < count[0] &&
			namesame(net_namematch[0][i0].name, net_namematch[0][i0+1].name) == 0)
				i0++;
		while (i1+1 < count[1] &&
			namesame(net_namematch[1][i1].name, net_namematch[1][i1+1].name) == 0)
				i1++;
		if (comp == 0)
		{
			if (i0 == i0base && i1 == i1base)
			{
				/* found a unique match */
				if ((net_ncc_options&NCCSUPALLAMBREP) == 0 || foundexport)
				{
					ttyputmsg(_("--- Forcing a match based on the network name '%s' (%ld symmetry groups with %ld nets and %ld nodes unmatched)"),
						net_namematch[0][i0].name, total, unmatchednets, unmatchedcomps);
				}
				net_forceamatch(sg, 1, &net_namematch[0][i0].number, 1, &net_namematch[1][i1].number,
					0.0, 0.0, verbose, ignorepwrgnd);
				return(TRUE);
			}
			i0++;   i1++;
		} else
		{
			if (comp < 0) i0++; else i1++;
		}
	}

	/* no unique name found */
	return(FALSE);
}

/*
 * Routine to look through ambiguous symmetry group "sg" for component names that cause a match.
 * If one is found, it is reported and matched and the routine returns true.
 * If "usenccmatches" is true, use "NCCMatch" tags.
 * If "allowpseudonames" is zero and such names are found, the routine returns true.
 * Flags "verbose" and "ignorepwrgnd" apply to the match.  Tallys "total", "unmatchednets",
 * and "unmatchedcomps" are reported.
 */
BOOLEAN net_findcomponentnamematch(SYMGROUP *sg, BOOLEAN usenccmatches,
	INTBIG verbose, BOOLEAN ignorepwrgnd, INTBIG total, INTBIG unmatchednets, INTBIG unmatchedcomps)
{
	REGISTER INTBIG i0, i1, i0base, i1base, i, j, f, comp, i0ptr, i1ptr;
	INTBIG count[2];
	REGISTER PCOMP *pc;
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var;

	/* build a list of all component names */
	for(f=0; f<2; f++)
	{
		count[f] = 0;
		for(i=0; i<sg->cellcount[f]; i++)
		{
			pc = (PCOMP *)sg->celllist[f][i];
			if (pc->numactual != 1) continue;
			ni = (NODEINST *)pc->actuallist;
			if (usenccmatches)
			{
				var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, net_ncc_matchkey);
				if (var == NOVARIABLE) continue;
				net_addtonamematch(&net_namematch[f], &net_namematchtotal[f], &count[f],
					(CHAR *)var->addr, i, ni);
			} else
			{
				var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
				if (var != NOVARIABLE)
				{
					if ((var->type&VDISPLAY) != 0)
					{
						net_addtonamematch(&net_namematch[f], &net_namematchtotal[f], &count[f],
							(CHAR *)var->addr, i, ni);
					}
				}
			}
		}
		esort(net_namematch[f], count[f], sizeof(NAMEMATCH), net_sortnamematches);
	}

	/* now look for unique matches */
	i0 = i1 = 0;
	i0ptr = i1ptr = 0;
	for(;;)
	{
		if (i0 >= count[0] || i1 >= count[1]) break;
		comp = namesame(net_namematch[0][i0].name, net_namematch[1][i1].name);
		if (comp == 0)
		{
			/* gather all with the same name */
			i0base = i0;   i1base = i1;
			while (i0+1 < count[0] &&
				namesame(net_namematch[0][i0].name, net_namematch[0][i0+1].name) == 0)
					i0++;
			while (i1+1 < count[1] &&
				namesame(net_namematch[1][i1].name, net_namematch[1][i1+1].name) == 0)
					i1++;

			/* make a list of entries from cell 0 */
			j = i0 - i0base + 1;
			if (j >= net_compmatch0total)
			{
				if (net_compmatch0total > 0) efree((CHAR *)net_compmatch0list);
				net_compmatch0total = 0;
				net_compmatch0list = (INTBIG *)emalloc(j * SIZEOFINTBIG, net_tool->cluster);
				if (net_compmatch0list == 0) return(FALSE);
				net_compmatch0total = j;
			}
			i0ptr = 0;
			for(i=i0base; i<=i0; i++)
				net_compmatch0list[i0ptr++] = net_namematch[0][i].number;
			esort(net_compmatch0list, i0ptr, SIZEOFINTBIG, sort_intbigdescending);
			j = 0; for(i=0; i<i0ptr; i++)
			{
				if (i == 0 || net_compmatch0list[i-1] != net_compmatch0list[i])
					net_compmatch0list[j++] = net_compmatch0list[i];
			}
			i0ptr = j;

			/* make a list of entries from cell 1 */
			j = i1 - i1base + 1;
			if (j >= net_compmatch1total)
			{
				if (net_compmatch1total > 0) efree((CHAR *)net_compmatch1list);
				net_compmatch1total = 0;
				net_compmatch1list = (INTBIG *)emalloc(j * SIZEOFINTBIG, net_tool->cluster);
				if (net_compmatch1list == 0) return(FALSE);
				net_compmatch1total = j;
			}
			i1ptr = 0;
			for(i=i1base; i<=i1; i++)
				net_compmatch1list[i1ptr++] = net_namematch[1][i].number;
			esort(net_compmatch1list, i1ptr, SIZEOFINTBIG, sort_intbigdescending);
			j = 0; for(i=0; i<i1ptr; i++)
			{
				if (i == 0 || net_compmatch1list[i-1] != net_compmatch1list[i])
					net_compmatch1list[j++] = net_compmatch1list[i];
			}
			i1ptr = j;
			if (i0ptr != 0 && i1ptr != 0 && i0ptr != sg->cellcount[0] && i1ptr != sg->cellcount[1])
			{
				/* found a unique match */
				if ((net_ncc_options&NCCSUPALLAMBREP) == 0)
				{
					ttyputmsg(_("--- Forcing a match based on the nodes named '%s' in cells %s and %s (%ld symmetry groups with %ld nets and %ld nodes unmatched)"),
						net_namematch[0][i0].name, describenodeproto(net_namematch[0][i0].original->parent),
							describenodeproto(net_namematch[1][i1].original->parent), total,
								unmatchednets, unmatchedcomps);
				}
				net_forceamatch(sg, i0ptr, net_compmatch0list, i1ptr, net_compmatch1list,
					0.0, 0.0, verbose, ignorepwrgnd);
				return(TRUE);
			}
			i0++;   i1++;
		} else
		{
			if (comp < 0) i0++; else i1++;
		}
	}

	/* no unique name found */
	return(FALSE);
}

/*
 * Helper routine to build the list of names.
 */
void net_addtonamematch(NAMEMATCH **match, INTBIG *total, INTBIG *count,
	CHAR *name, INTBIG number, NODEINST *orig)
{
	REGISTER INTBIG i, newtotal;
	REGISTER NAMEMATCH *newmatch;

	if (*count >= *total)
	{
		newtotal = *total * 2;
		if (newtotal <= *count) newtotal = *count + 25;
		newmatch = (NAMEMATCH *)emalloc(newtotal * (sizeof (NAMEMATCH)), net_tool->cluster);
		if (newmatch == 0) return;
		for(i=0; i < *count; i++)
		{
			newmatch[i].name = (*match)[i].name;
			newmatch[i].number = (*match)[i].number;
			newmatch[i].original = (*match)[i].original;
		}
		if (*total > 0) efree((CHAR *)*match);
		*match = newmatch;
		*total = newtotal;
	}
	(*match)[*count].name = name;
	(*match)[*count].number = number;
	(*match)[*count].original = orig;
	(*count)++;
}

/*
 * Helper routine to sort the list of names.
 */
int net_sortnamematches(const void *e1, const void *e2)
{
	REGISTER NAMEMATCH *nm1, *nm2;

	nm1 = (NAMEMATCH *)e1;
	nm2 = (NAMEMATCH *)e2;
	return(namesame(nm1->name, nm2->name));
}

int net_sortsymgroups(const void *e1, const void *e2)
{
	REGISTER SYMGROUP *sg1, *sg2;
	REGISTER INTBIG sg1size, sg2size;

	sg1 = *((SYMGROUP **)e1);
	sg2 = *((SYMGROUP **)e2);
	sg1size = sg1->cellcount[0] + sg1->cellcount[1];
	sg2size = sg2->cellcount[0] + sg2->cellcount[1];
#ifdef NEWNCC
	if (sg1->hashvalue == 0 || (sg1->cellcount[0] < 2 && sg1->cellcount[1] < 2)) sg1size = 0;
	if (sg2->hashvalue == 0 || (sg2->cellcount[0] < 2 && sg2->cellcount[1] < 2)) sg2size = 0;
#else
	if (sg1->hashvalue == 0 || sg1->cellcount[0] < 2 || sg1->cellcount[1] < 2) sg1size = 0;
	if (sg2->hashvalue == 0 || sg2->cellcount[0] < 2 || sg2->cellcount[1] < 2) sg2size = 0;
#endif
	return(sg1size - sg2size);
}

int net_sortpcomp(const void *e1, const void *e2)
{
	REGISTER PCOMP *pc1, *pc2;
	REGISTER CHAR *pt1, *pt2;

	pc1 = *((PCOMP **)e1);
	pc2 = *((PCOMP **)e2);
	if (pc2->wirecount != pc1->wirecount)
		return(pc2->wirecount - pc1->wirecount);
	pt1 = pc1->hashreason;
	pt2 = pc2->hashreason;
	return(namesame(pt1, pt2));
}

int net_sortpnet(const void *e1, const void *e2)
{
	REGISTER PNET *pn1, *pn2;
	REGISTER NETWORK *net1, *net2;
	REGISTER NODEPROTO *cell1, *cell2;
	REGISTER INTBIG un1, un2;

	pn1 = *((PNET **)e1);
	pn2 = *((PNET **)e2);
	if (pn2->nodecount != pn1->nodecount)
		return(pn2->nodecount - pn1->nodecount);
	un1 = un2 = 0;
	if ((pn1->flags&(POWERNET|GROUNDNET|EXPORTEDNET)) == 0 &&
		(pn1->network == NONETWORK || pn1->network->namecount == 0)) un1 = 1;
	if ((pn2->flags&(POWERNET|GROUNDNET|EXPORTEDNET)) == 0 &&
		(pn2->network == NONETWORK || pn2->network->namecount == 0)) un2 = 1;
	if (un1 == 0 && un2 == 0)
	{
		return(namesame(net_describepnet(pn1), net_describepnet(pn2)));
	}
	if (un1 != 0 && un2 != 0)
	{
		net1 = pn1->network;
		net2 = pn2->network;
		if (net1 != NONETWORK && net2 != NONETWORK)
		{
			cell1 = net1->parent;
			cell2 = net2->parent;
			return(namesame(cell1->protoname, cell2->protoname));
		}
		return(0);
	}
	return(un1 - un2);
}

/*
 * Routine to search symmetry group "sg" for a size factor that will distinguish part of
 * the group.  Returns true if a distinguishing size is found (and places it in "sizew" and
 * "sizel").
 */
BOOLEAN net_findcommonsizefactor(SYMGROUP *sg, float *sizew, float *sizel)
{
	REGISTER INTBIG i, j, f, newtotal, p0, p1, bestind0, bestind1;
	INTBIG sizearraycount[2];
	REGISTER PCOMP *pc;
	REGISTER NODESIZE *newnodesizes, *ns0, *ns1;
	REGISTER BOOLEAN firsttime;
	float diff, bestdiff, wantlength, wantwidth;

	for(f=0; f<2; f++)
	{
		sizearraycount[f] = 0;
		for(i=0; i<sg->cellcount[f]; i++)
		{
			pc = (PCOMP *)sg->celllist[f][i];
			if ((pc->flags&(COMPHASWIDLEN|COMPHASAREA)) == 0) continue;
			if (sizearraycount[f] >= net_sizearraytotal[f])
			{
				newtotal = net_sizearraytotal[f] * 2;
				if (sizearraycount[f] >= newtotal) newtotal = sizearraycount[f] + 20;
				newnodesizes = (NODESIZE *)emalloc(newtotal * (sizeof (NODESIZE)), net_tool->cluster);
				if (newnodesizes == 0) return(FALSE);
				for(j=0; j<sizearraycount[f]; j++)
					newnodesizes[j] = net_sizearray[f][j];
				if (net_sizearraytotal[f] > 0) efree((CHAR *)net_sizearray[f]);
				net_sizearray[f] = newnodesizes;
				net_sizearraytotal[f] = newtotal;
			}
			j = sizearraycount[f]++;
			if ((pc->flags&COMPHASWIDLEN) != 0)
			{
				net_sizearray[f][j].length = pc->length;
				net_sizearray[f][j].width = pc->width;
			} else
			{
				net_sizearray[f][j].length = pc->length;
				net_sizearray[f][j].width = 0.0;
			}
		}
		if (sizearraycount[f] > 0)
			esort(net_sizearray[f], sizearraycount[f], sizeof (NODESIZE), net_sortsizearray);
	}

	/* now find the two values that are closest */
	p0 = p1 = bestind0 = bestind1 = 0;
	firsttime = TRUE;
	for(;;)
	{
		if (p0 >= sizearraycount[0]) break;
		if (p1 >= sizearraycount[1]) break;

		ns0 = &net_sizearray[0][p0];
		ns1 = &net_sizearray[1][p1];
		diff = (float)(fabs(ns0->length-ns1->length) + fabs(ns0->width-ns1->width));
		if (firsttime || diff < bestdiff)
		{
			bestdiff = diff;
			bestind0 = p0;
			bestind1 = p1;
			firsttime = FALSE;
		}
		if (ns0->length + ns0->width < ns1->length + ns1->width) p0++; else
			p1++;
	}
	if (firsttime) return(FALSE);

	/* found the two closest values: see if they are indeed close */
	ns0 = &net_sizearray[0][bestind0];
	ns1 = &net_sizearray[1][bestind1];
	if (!net_componentequalvalue(ns0->length, ns1->length) ||
		!net_componentequalvalue(ns0->width, ns1->width)) return(FALSE);
	wantlength = (ns0->length + ns1->length) / 2.0f;
	wantwidth = (ns0->width + ns1->width) / 2.0f;

	/* make sure these values distinguish */
	ns0 = &net_sizearray[0][0];
	ns1 = &net_sizearray[0][sizearraycount[0]-1];
	if (net_componentequalvalue(ns0->length, wantlength) &&
		net_componentequalvalue(ns1->length, wantlength) &&
		net_componentequalvalue(ns0->width, wantwidth) &&
		net_componentequalvalue(ns1->width, wantwidth)) return(FALSE);
	ns0 = &net_sizearray[1][0];
	ns1 = &net_sizearray[1][sizearraycount[1]-1];
	if (net_componentequalvalue(ns0->length, wantlength) &&
		net_componentequalvalue(ns1->length, wantlength) &&
		net_componentequalvalue(ns0->width, wantwidth) &&
		net_componentequalvalue(ns1->width, wantwidth)) return(FALSE);

	/* return the size */
	*sizew = wantwidth;
	*sizel = wantlength;
	return(TRUE);
}

int net_sortsizearray(const void *e1, const void *e2)
{
	REGISTER NODESIZE *ns1, *ns2;
	REGISTER float v1, v2;

	ns1 = (NODESIZE *)e1;
	ns2 = (NODESIZE *)e2;
	v1 = ns1->length + ns1->width;
	v2 = ns2->length + ns2->width;
	if (floatsequal(v1, v2)) return(0);
	if (v1 < v2) return(-1);
	return(1);
}

/*
 * Routine to force a match between parts of symmetry group "sg".  If "sizefactorsplit" is
 * zero, then "c0" entries in "i0" in cell 0 and "c1" entries in "i1" in cell 1 are to be matched.
 * Otherwise, those components with size factor "sizew/sizel" are to be matched.
 */
void net_forceamatch(SYMGROUP *sg, INTBIG c0, INTBIG *i0, INTBIG c1, INTBIG *i1,
	float sizew, float sizel, INTBIG verbose, BOOLEAN ignorepwrgnd)
{
	REGISTER SYMGROUP *sgnewc, *sgnewn;
	REGISTER HASHTYPE hashvalue;
	REGISTER BOOLEAN match;
	REGISTER PNET *pn, *pn0, *pn1;
	REGISTER PCOMP *pc, *pc0, *pc1;
	REGISTER INTBIG i, f, ind;

	if (sg->grouptype == SYMGROUPCOMP)
	{
		hashvalue = net_uniquesymmetrygrouphash(SYMGROUPCOMP);
		sgnewc = net_newsymgroup(SYMGROUPCOMP, hashvalue, 0);

		if (sizew == 0.0 && sizel == 0.0)
		{
			/* matching two like-named nodes */
			for(i=0; i<c0; i++)
			{
				ind = i0[i];
				pc0 = (PCOMP *)sg->celllist[0][ind];
				net_removefromsymgroup(sg, 0, ind);
				if (net_addtosymgroup(sgnewc, 0, (void *)pc0)) return;
				pc0->symgroup = sgnewc;
				pc0->hashvalue = hashvalue;
				if (verbose != 0)
					(void)reallocstring(&pc0->hashreason, x_("name matched"), net_tool->cluster);
			}
			for(i=0; i<c1; i++)
			{
				ind = i1[i];
				pc1 = (PCOMP *)sg->celllist[1][ind];
				net_removefromsymgroup(sg, 1, ind);
				if (net_addtosymgroup(sgnewc, 1, (void *)pc1)) return;
				pc1->symgroup = sgnewc;
				pc1->hashvalue = hashvalue;
				if (verbose != 0)
					(void)reallocstring(&pc1->hashreason, x_("name matched"), net_tool->cluster);
			}
		} else
		{
			/* matching nodes with size "sizefactor" */
			for(f=0; f<2; f++)
			{
				for(i=sg->cellcount[f]-1; i>=0; i--)
				{
					pc = (PCOMP *)sg->celllist[f][i];
					match = FALSE;
					if ((pc->flags&COMPHASWIDLEN) != 0)
					{
						if (net_componentequalvalue(sizew, pc->width) &&
							net_componentequalvalue(sizel, pc->length)) match = TRUE;
					} else
					{
						if (net_componentequalvalue(sizel, pc->length)) match = TRUE;
					}
					if (match)
					{
						net_removefromsymgroup(sg, f, i);
						if (net_addtosymgroup(sgnewc, f, (void *)pc)) return;
						pc->symgroup = sgnewc;
						pc->hashvalue = hashvalue;
						if (verbose != 0)
							(void)reallocstring(&pc->hashreason, x_("size matched"), net_tool->cluster);
					}
				}
			}
		}

		/* set the remaining components in this symmetry group to a nonzero hash */
		hashvalue = net_uniquesymmetrygrouphash(SYMGROUPCOMP);
		sgnewc = net_newsymgroup(SYMGROUPCOMP, hashvalue, 0);
		for(f=0; f<2; f++)
		{
			for(i=sg->cellcount[f]-1; i>=0; i--)
			{
				pc = (PCOMP *)sg->celllist[f][i];
				net_removefromsymgroup(sg, f, i);
				if (net_addtosymgroup(sgnewc, f, (void *)pc)) return;
				pc->symgroup = sgnewc;
				pc->hashvalue = hashvalue;
				if (verbose != 0)
					(void)reallocstring(&pc->hashreason, x_("redeemed"), net_tool->cluster);
			}
		}
		sgnewn = NOSYMGROUP;
	} else
	{
		hashvalue = net_uniquesymmetrygrouphash(SYMGROUPNET);
		sgnewn = net_newsymgroup(SYMGROUPNET, hashvalue, 0);

		for(i=0; i<c0; i++)
		{
			ind = i0[i];
			pn0 = (PNET *)sg->celllist[0][ind];
			net_removefromsymgroup(sg, 0, ind);
			if (net_addtosymgroup(sgnewn, 0, (void *)pn0)) return;
			pn0->symgroup = sgnewn;
			pn0->hashvalue = hashvalue;
			if (verbose != 0)
				(void)reallocstring(&pn0->hashreason, x_("export matched"), net_tool->cluster);
		}
		for(i=0; i<c1; i++)
		{
			ind = i1[i];
			pn1 = (PNET *)sg->celllist[1][ind];
			net_removefromsymgroup(sg, 1, ind);
			if (net_addtosymgroup(sgnewn, 1, (void *)pn1)) return;
			pn1->symgroup = sgnewn;
			pn1->hashvalue = hashvalue;
			if (verbose != 0)
				(void)reallocstring(&pn1->hashreason, x_("export matched"), net_tool->cluster);
		}

		/* set the remaining nets in this symmetry group to a nonzero hash */
		hashvalue = net_uniquesymmetrygrouphash(SYMGROUPNET);
		sgnewn = net_newsymgroup(SYMGROUPNET, hashvalue, 0);
		for(f=0; f<2; f++)
		{
			for(i=sg->cellcount[f]-1; i>=0; i--)
			{
				pn = (PNET *)sg->celllist[f][i];
				net_removefromsymgroup(sg, f, i);
				if (net_addtosymgroup(sgnewn, f, (void *)pn)) return;
				pn->symgroup = sgnewn;
				pn->hashvalue = hashvalue;
				if (verbose != 0)
					(void)reallocstring(&pn->hashreason, x_("redeemed"), net_tool->cluster);
			}
		}
		sgnewc = NOSYMGROUP;
	}

	net_redeemzerogroups(sgnewc, sgnewn, verbose, ignorepwrgnd);
}

void net_redeemzerogroups(SYMGROUP *sgnewc, SYMGROUP *sgnewn, INTBIG verbose, BOOLEAN ignorepwrgnd)
{
	REGISTER SYMGROUP *sg;
	REGISTER HASHTYPE hashvalue;
	REGISTER PNET *pn;
	REGISTER PCOMP *pc;
	REGISTER INTBIG i, f;

	/* redeem all zero-symmetry groups */
	sg = net_findsymmetrygroup(SYMGROUPNET, 0, 0);
	if (sg != NOSYMGROUP)
	{
		if (sgnewn == NOSYMGROUP)
		{
			hashvalue = net_uniquesymmetrygrouphash(SYMGROUPNET);
			sgnewn = net_newsymgroup(SYMGROUPNET, hashvalue, 0);
		}
		for(f=0; f<2; f++)
		{
			for(i=sg->cellcount[f]-1; i>=0; i--)
			{
				pn = (PNET *)sg->celllist[f][i];
				if (ignorepwrgnd && (pn->flags&(POWERNET|GROUNDNET)) != 0) continue;
				net_removefromsymgroup(sg, f, i);
				if (net_addtosymgroup(sgnewn, f, (void *)pn)) return;
				pn->symgroup = sgnewn;
				pn->hashvalue = sgnewn->hashvalue;
				if (verbose != 0)
					(void)reallocstring(&pn->hashreason, x_("redeemed"), net_tool->cluster);
			}
		}
	}

	sg = net_findsymmetrygroup(SYMGROUPCOMP, 0, 0);
	if (sg != NOSYMGROUP)
	{
		if (sgnewc == NOSYMGROUP)
		{
			hashvalue = net_uniquesymmetrygrouphash(SYMGROUPCOMP);
			sgnewc = net_newsymgroup(SYMGROUPCOMP, hashvalue, 0);
		}
		for(f=0; f<2; f++)
		{
			for(i=sg->cellcount[f]-1; i>=0; i--)
			{
				pc = (PCOMP *)sg->celllist[f][i];
				net_removefromsymgroup(sg, f, i);
				if (net_addtosymgroup(sgnewc, f, (void *)pc)) return;
				pc->symgroup = sgnewc;
				pc->hashvalue = sgnewc->hashvalue;
				if (verbose != 0)
					(void)reallocstring(&pc->hashreason, x_("redeemed"), net_tool->cluster);
			}
		}
	}
}

/*
 * Routine to return a string describing size factor "sizefactor".
 */
CHAR *net_describesizefactor(float sizew, float sizel)
{
	static CHAR sizedesc[80];

	if (sizew == 0)
	{
		esnprintf(sizedesc, 80, x_("%g"), sizel/WHOLE);
	} else
	{
		esnprintf(sizedesc, 80, x_("%gx%g"), sizew/WHOLE, sizel/WHOLE);
	}
	return(sizedesc);
}

/******************************** HASH CODE EVALUATION ********************************/

/*
 * Routine to return a hash code for component "pc".
 */
HASHTYPE net_getcomphash(PCOMP *pc, INTBIG verbose)
{
	REGISTER INTBIG function, i, portfactor;
	REGISTER HASHTYPE hashvalue;
	REGISTER NODEINST *ni;
	REGISTER NETWORK *net;
	REGISTER PNET *pn;
	REGISTER PORTPROTO *pp;
	REGISTER void *infstr = 0;

	/* get the node's function */
	if (pc->numactual == 1) ni = (NODEINST *)pc->actuallist; else
		ni = ((NODEINST **)pc->actuallist)[0];
	if (ni->proto->primindex == 0)
	{
		/* if the function is overridden for the cell, use it */
		if (pc->function != NPUNKNOWN) function = pc->function; else
		{
			/* a cell instance: use the node's prototype address */
			function = (INTBIG)(ni->proto->temp1 & NETCELLCODE) >> NETCELLCODESH;
		}
	} else
	{
		/* a primitive: use the node's function */
		function = pc->function;
	}

	/* initialize the hash factor */
	hashvalue = function * net_functionMultiplier + pc->forcedassociation;

	if (verbose != 0)
	{
		/* compute new hash values and store an explanation */
		infstr = initinfstr();
		formatinfstr(infstr, x_("%ld(fun)"), function);
		if (pc->forcedassociation != 0)
			formatinfstr(infstr, x_("+%ld(force)"), pc->forcedassociation);

		/* now add in all networks as a function of the port's network */
		for(i=0; i<pc->wirecount; i++)
		{
			pp = pc->portlist[i];
			pn = pc->netnumbers[i];
			portfactor = pc->portindices[i];
			hashvalue += (portfactor * net_portNetFactorMultiplier) *
				(pn->hashvalue * net_portHashFactorMultiplier);
			net = pn->network;
			if (net == NONETWORK)
			{
				formatinfstr(infstr, x_(" + %ld[%s]"), portfactor, pp->protoname);
			} else
			{
				formatinfstr(infstr, x_(" + %ld(%s)"), portfactor, describenetwork(net));
			}
			formatinfstr(infstr, x_("x%s(hash)"), hugeinttoa(pn->hashvalue));
		}
		(void)reallocstring(&pc->hashreason, returninfstr(infstr), net_tool->cluster);
	} else
	{
		/* now add in all networks as a function of the port's network */
		for(i=0; i<pc->wirecount; i++)
		{
			pn = pc->netnumbers[i];
			portfactor = pc->portindices[i];
			hashvalue += (portfactor * net_portNetFactorMultiplier) *
				(pn->hashvalue * net_portHashFactorMultiplier);
		}
	}
	return(hashvalue);
}

/*
 * Routine to return a hash code for network "pn".
 */
HASHTYPE net_getnethash(PNET *pn, INTBIG verbose)
{
	REGISTER INTBIG i, index, portfactor;
	REGISTER BOOLEAN validcomponents;
	REGISTER HASHTYPE hashvalue;
	REGISTER PCOMP *pc;
	REGISTER void *infstr=0;

	/* initialize the hash factor */
	hashvalue = 0;

	/* start with the number of components on this net */
	hashvalue += (pn->nodecount+1) * net_nodeCountMultiplier + pn->forcedassociation;

	validcomponents = FALSE;
	if (verbose != 0)
	{
		infstr = initinfstr();
		formatinfstr(infstr, x_("%ld(cnt)"), pn->nodecount);
		if (pn->forcedassociation != 0)
			formatinfstr(infstr, x_("+%ld(force)"), pn->forcedassociation);

		/* add in information for each component */
		for(i=0; i<pn->nodecount; i++)
		{
			pc = pn->nodelist[i];
			index = pn->nodewire[i];
			portfactor = pc->portindices[index];
			hashvalue += portfactor * net_portFactorMultiplier * pc->hashvalue;
			if (pc->hashvalue != 0) validcomponents = TRUE;
			formatinfstr(infstr, x_(" + %ld(port)x%s(hash)"), portfactor, hugeinttoa(pc->hashvalue));
		}

		/* if no components had valid hash values, make this net zero */
		if (!validcomponents && pn->nodecount != 0) hashvalue = 0;
		(void)reallocstring(&pn->hashreason, returninfstr(infstr), net_tool->cluster);
	} else
	{
		for(i=0; i<pn->nodecount; i++)
		{
			pc = pn->nodelist[i];
			index = pn->nodewire[i];
			portfactor = pc->portindices[index];
			hashvalue += portfactor * net_portFactorMultiplier * pc->hashvalue;
			if (pc->hashvalue != 0) validcomponents = TRUE;
		}

		/* if no components had valid hash values, make this net zero */
		if (!validcomponents && pn->nodecount != 0) hashvalue = 0;
	}

	return(hashvalue);
}

/******************************** DEBUGGING ********************************/

/*
 * Routine to show the components in group "sg" because they have been matched.
 */
void net_showmatchedgroup(SYMGROUP *sg)
{
	NODEPROTO *topcell;
	NODEINST *ni;
	XARRAY xf, xformtrans, temp;
	INTBIG xp1, yp1, xp2, yp2, xp3, yp3, xp4, yp4, plx, phx, ply, phy;
	REGISTER INTBIG f, j, lx, hx, ly, hy;
	REGISTER PCOMP *pc;
	REGISTER WINDOWPART *w;

	us_hbox.col = HIGHLIT;

	/* show the components in this group if requested */
	if (sg->grouptype == SYMGROUPCOMP)
	{
		for(f=0; f<2; f++)
		{
			pc = (PCOMP *)sg->celllist[f][0];
			transid(xf);
			if (pc->numactual == 1) ni = (NODEINST *)pc->actuallist; else
				ni = ((NODEINST **)pc->actuallist)[0];
			topcell = ni->parent;
			for(j=pc->hierpathcount-1; j>=0; j--)
			{
				ni = pc->hierpath[j];
				if (ni->proto->cellview == el_iconview) break;
				topcell = ni->parent;
				maketrans(ni, xformtrans);
				transmult(xformtrans, xf, temp);
				makerot(ni, xformtrans);
				transmult(xformtrans, temp, xf);
			}
			if (j >= 0) continue;
			for(j=0; j<pc->numactual; j++)
			{
				if (pc->numactual == 1) ni = (NODEINST *)pc->actuallist; else
					ni = ((NODEINST **)pc->actuallist)[j];
				makerot(ni, xformtrans);
				transmult(xformtrans, xf, temp);
				nodesizeoffset(ni, &plx, &ply, &phx, &phy);
				lx = ni->lowx+plx;   hx = ni->highx-phx;
				ly = ni->lowy+ply;   hy = ni->highy-phy;
				xform(lx, ly, &xp1, &yp1, temp);
				xform(hx, hy, &xp2, &yp2, temp);
				xform(lx, hy, &xp3, &yp3, temp);
				xform(hx, ly, &xp4, &yp4, temp);
				for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
				{
					if (w->curnodeproto != topcell) continue;
					xp1 = applyxscale(w, xp1-w->screenlx) + w->uselx;
					yp1 = applyyscale(w, yp1-w->screenly) + w->usely;
					xp2 = applyxscale(w, xp2-w->screenlx) + w->uselx;
					yp2 = applyyscale(w, yp2-w->screenly) + w->usely;
					screendrawline(w, xp1, yp1, xp2, yp2, &us_hbox, 0);
					xp3 = applyxscale(w, xp3-w->screenlx) + w->uselx;
					yp3 = applyyscale(w, yp3-w->screenly) + w->usely;
					xp4 = applyxscale(w, xp4-w->screenlx) + w->uselx;
					yp4 = applyyscale(w, yp4-w->screenly) + w->usely;
					screendrawline(w, xp3, yp3, xp4, yp4, &us_hbox, 0);
					flushscreen();
				}
			}
		}
	}
}

/*
 * Debugging routine to show the hash codes on all symmetry groups.
 */
void net_showsymmetrygroups(INTBIG verbose, INTBIG type)
{
	WINDOWPART *win[2];
	REGISTER WINDOWPART *w;
	REGISTER INTBIG i, f;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	REGISTER SYMGROUP *sg;
	REGISTER PNET *pn;
	REGISTER PCOMP *pc;

	if ((verbose&NCCVERBOSEGRAPHICS) != 0)
	{
		/* find the windows associated with the cells */
		win[0] = win[1] = NOWINDOWPART;
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			for(f=0; f<2; f++)
				if (w->curnodeproto == net_cell[f]) win[f] = w;
		}
		if (win[0] == NOWINDOWPART || win[1] == NOWINDOWPART) return;

		/* clear all highlighting */
		(void)asktool(us_tool, x_("clear"));
		for(f=0; f<2; f++)
			screendrawbox(win[f], win[f]->uselx, win[f]->usehx, win[f]->usely, win[f]->usehy,
				&net_cleardesc);

		TDCLEAR(descript);
		TDSETSIZE(descript, TXTSETPOINTS(16));
		screensettextinfo(win[0], NOTECHNOLOGY, descript);
		screensettextinfo(win[1], NOTECHNOLOGY, descript);
	}
	for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
	{
		if (sg->grouptype != type) continue;
		switch (sg->grouptype)
		{
			case SYMGROUPCOMP:
				for(f=0; f<2; f++)
				{
					for(i=0; i<sg->cellcount[f]; i++)
					{
						pc = (PCOMP *)sg->celllist[f][i];
						net_showcomphash(win[f], pc, pc->hashvalue, sg->groupindex, verbose);
					}
				}
				break;
			case SYMGROUPNET:
				for(f=0; f<2; f++)
				{
					for(i=0; i<sg->cellcount[f]; i++)
					{
						pn = (PNET *)sg->celllist[f][i];
						net_shownethash(win[f], pn, pn->hashvalue, sg->groupindex, verbose);
					}
				}
				break;
		}
	}
	for(sg = net_firstmatchedsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
	{
		if (sg->grouptype != type) continue;
		switch (sg->grouptype)
		{
			case SYMGROUPCOMP:
				for(f=0; f<2; f++)
				{
					for(i=0; i<sg->cellcount[f]; i++)
					{
						pc = (PCOMP *)sg->celllist[f][i];
						net_showcomphash(win[f], pc, pc->hashvalue, sg->groupindex, verbose);
					}
				}
				break;
			case SYMGROUPNET:
				for(f=0; f<2; f++)
				{
					for(i=0; i<sg->cellcount[f]; i++)
					{
						pn = (PNET *)sg->celllist[f][i];
						net_shownethash(win[f], pn, pn->hashvalue, sg->groupindex, verbose);
					}
				}
				break;
		}
	}
}

void net_shownethash(WINDOWPART *win, PNET *pn, HASHTYPE hashvalue, INTBIG hashindex, INTBIG verbose)
{
	REGISTER NETWORK *net;
	CHAR msg[50];
	REGISTER PORTPROTO *pp;
	REGISTER ARCINST *ai;
	INTBIG tsx, tsy, px, py;
	REGISTER INTBIG j;
	INTBIG xp, yp;

	net = pn->network;
	if (net == NONETWORK) return;
	if ((verbose&NCCVERBOSEGRAPHICS) != 0)
	{
		if (hashindex != 0) esnprintf(msg, 50, x_("%ld"), hashindex); else
			estrcpy(msg, hugeinttoa(hashvalue));
		screengettextsize(win, msg, &tsx, &tsy);
		for(j=0; j<net->arccount; j++)
		{
			if (net->arctotal == 0) ai = (ARCINST *)net->arcaddr; else
				ai = ((ARCINST **)net->arcaddr)[j];
			if (ai->parent == win->curnodeproto)
			{
				xp = (ai->end[0].xpos + ai->end[1].xpos) / 2;
				yp = (ai->end[0].ypos + ai->end[1].ypos) / 2;
				if ((win->state&INPLACEEDIT) != 0) 
					xform(xp, yp, &xp, &yp, win->outofcell);
				xp = applyxscale(win, xp-win->screenlx) + win->uselx;
				yp = applyyscale(win, yp-win->screenly) + win->usely;
				px = xp - tsx/2;   py = yp - tsy/2;
				if (px < win->uselx) px = win->uselx;
				if (px+tsx > win->usehx) px = win->usehx - tsx;
				screendrawtext(win, px, py, msg, &net_msgdesc);
			}
		}
		if (net->portcount > 0)
		{
			for(pp = win->curnodeproto->firstportproto; pp != NOPORTPROTO;
				pp = pp->nextportproto)
			{
				if (pp->network != net) continue;
				portposition(pp->subnodeinst, pp->subportproto, &xp, &yp);
				if ((win->state&INPLACEEDIT) != 0) 
					xform(xp, yp, &xp, &yp, win->outofcell);
				xp = applyxscale(win, xp-win->screenlx) + win->uselx;
				yp = applyyscale(win, yp-win->screenly) + win->usely;
				px = xp - tsx/2;   py = yp - tsy/2;
				if (px < win->uselx) px = win->uselx;
				if (px+tsx > win->usehx) px = win->usehx - tsx;
				screendrawtext(win, px, py, msg, &net_msgdesc);
			}
		}
	}
	if ((verbose&NCCVERBOSETEXT) != 0)
	{
		if (hashindex != 0)
		{
			ttyputmsg(x_(" NETWORK %s:%s: %s (hash #%ld) = %s"), describenodeproto(net->parent),
				net_describepnet(pn), hugeinttoa(hashvalue), hashindex, pn->hashreason);
		} else
		{
			ttyputmsg(x_(" NETWORK %s:%s: %s (hash) = %s"), describenodeproto(net->parent),
				net_describepnet(pn), hugeinttoa(hashvalue), pn->hashreason);
		}
	}
}

void net_showcomphash(WINDOWPART *win, PCOMP *pc, HASHTYPE hashvalue, INTBIG hashindex, INTBIG verbose)
{
	INTBIG xp, yp;
	INTBIG tsx, tsy, px, py;
	REGISTER NODEINST *ni;
	CHAR msg[50];

	if (pc->numactual == 1) ni = (NODEINST *)pc->actuallist; else
		ni = ((NODEINST **)pc->actuallist)[0];
	if ((verbose&NCCVERBOSEGRAPHICS) != 0)
	{
		if (ni->parent == win->curnodeproto)
		{
			xp = (ni->lowx + ni->highx) / 2;
			yp = (ni->lowy + ni->highy) / 2;
			if ((win->state&INPLACEEDIT) != 0) 
				xform(xp, yp, &xp, &yp, win->outofcell);
			xp = applyxscale(win, xp-win->screenlx) + win->uselx;
			yp = applyyscale(win, yp-win->screenly) + win->usely;
			if (hashindex != 0) esnprintf(msg, 50, x_("%ld"), hashindex); else
				estrcpy(msg, hugeinttoa(hashvalue));
			screengettextsize(win, msg, &tsx, &tsy);
			px = xp - tsx/2;   py = yp - tsy/2;
			if (px < win->uselx) px = win->uselx;
			if (px+tsx > win->usehx) px = win->usehx - tsx;
			screendrawtext(win, px, py, msg, &net_msgdesc);
		}
	}
	if ((verbose&NCCVERBOSETEXT) != 0)
	{
		if (hashindex != 0)
		{
			ttyputmsg(x_(" COMPONENT %s: %s (hash #%ld) = %s"), describenodeinst(ni),
				hugeinttoa(hashvalue), hashindex, pc->hashreason);
		} else
		{
			ttyputmsg(x_(" COMPONENT %s: %s (hash) = %s"), describenodeinst(ni),
				hugeinttoa(hashvalue), pc->hashreason);
		}
	}
}

void net_dumpnetwork(PCOMP *pclist, PNET *pnlist)
{
	REGISTER PCOMP *pc;
	REGISTER PNET *pn;
	REGISTER INTBIG i;
	REGISTER NODEINST *ni;
	CHAR nettype[50];
	REGISTER void *infstr;

	ttyputmsg(x_("Nodes:"));
	for(pc = pclist; pc != NOPCOMP; pc = pc->nextpcomp)
	{
		if (pc->numactual == 1) ni = (NODEINST *)pc->actuallist; else
			ni = ((NODEINST **)pc->actuallist)[0];
		ttyputmsg(x_("  Node %s (fun=%d)"), describenodeinst(ni), pc->function);
	}
	ttyputmsg(x_("Nets:"));
	for(pn = pnlist; pn != NOPNET; pn = pn->nextpnet)
	{
		infstr = initinfstr();
		nettype[0] = 0;
		if ((pn->flags&(POWERNET|GROUNDNET)) != 0)
		{
			if ((pn->flags&POWERNET) != 0) estrcat(nettype, x_("POWER ")); else
				estrcat(nettype, x_("GROUND "));
		}
		formatinfstr(infstr, x_("  %sNet %s (%ld nodes):"), nettype, net_describepnet(pn), pn->nodecount);
		for(i=0; i<pn->nodecount; i++)
		{
			pc = pn->nodelist[i];
			if (pc->numactual == 1) ni = (NODEINST *)pc->actuallist; else
				ni = ((NODEINST **)pc->actuallist)[0];
			formatinfstr(infstr, x_(" %s"), describenodeinst(ni));
		}
		ttyputmsg(x_("%s"), returninfstr(infstr));
	}
}

/* NCC preanalysis */
static DIALOGITEM net_nccpredialogitems[] =
{
 /*  1 */ {0, {8,315,24,615}, MESSAGE, N_("Cell2")},
 /*  2 */ {0, {28,8,384,308}, SCROLL, x_("")},
 /*  3 */ {0, {8,8,24,308}, MESSAGE, N_("Cell1")},
 /*  4 */ {0, {28,315,384,615}, SCROLL, x_("")},
 /*  5 */ {0, {392,244,408,392}, RADIOA, N_("Components")},
 /*  6 */ {0, {416,20,432,212}, AUTOCHECK, N_("Tie lists vertically")},
 /*  7 */ {0, {416,244,432,392}, RADIOA, N_("Networks")},
 /*  8 */ {0, {392,20,408,212}, AUTOCHECK, N_("Show only differences")},
 /*  9 */ {0, {400,524,424,604}, DEFBUTTON, N_("Close")},
 /* 10 */ {0, {400,420,424,500}, BUTTON, N_("Compare")}
};
static DIALOG net_nccpredialog = {{75,75,516,700}, N_("NCC Preanalysis Results"), 0, 10, net_nccpredialogitems, x_("nccpre"), 0};

/* special items for the "nccpre" dialog: */
#define DNCP_CELL2NAME      1		/* cell 2 name (message) */
#define DNCP_CELL1LIST      2		/* cell 1 list (scroll) */
#define DNCP_CELL1NAME      3		/* cell 1 name (message) */
#define DNCP_CELL2LIST      4		/* cell 2 list (scroll) */
#define DNCP_SHOWCOMPS      5		/* Show components (radioa) */
#define DNCP_TIEVERTICALLY  6		/* Tie lists vertically (autocheck) */
#define DNCP_SHOWNETS       7		/* Show networks (radioa) */
#define DNCP_SHOWDIFFS      8		/* Show only differences (autocheck) */
#define DNCP_CLOSE          9		/* Close (defbutton) */
#define DNCP_COMPARE       10		/* Compare (button) */

class EDiaNetShowPreanalysis : public EDialogModeless
{
public:
	EDiaNetShowPreanalysis();
	void reset();
	void setCells(NODEPROTO *cell1, PCOMP *aPcomp1, PNET *aNodelist1,
		NODEPROTO *cell2, PCOMP *aPcomp2, PNET *aNodelist2, BOOLEAN aIgnorepwrgnd);
	static EDiaNetShowPreanalysis *dialog;
private:
	void itemHitAction(INTBIG itemHit);
	void putNetIntoDialog();
	void putCompIntoDialog();

	BOOLEAN vSynch;
	BOOLEAN showComps;
	BOOLEAN showAll;
	PCOMP *pcomp1, *pcomp2;
	PNET *nodelist1, *nodelist2;
	BOOLEAN ignorepwrgnd;
};

EDiaNetShowPreanalysis *EDiaNetShowPreanalysis::dialog = 0;

/*
 * Routine to show preanalysis results for cells
 */
void net_showpreanalysis(NODEPROTO *cell1, PCOMP *pcomp1, PNET *nodelist1,
	NODEPROTO *cell2, PCOMP *pcomp2, PNET *nodelist2, BOOLEAN ignorepwrgnd)
{
	if (EDiaNetShowPreanalysis::dialog == 0) EDiaNetShowPreanalysis::dialog = new EDiaNetShowPreanalysis();
	if (EDiaNetShowPreanalysis::dialog == 0) return;
	EDiaNetShowPreanalysis::dialog->setCells(cell1, pcomp1, nodelist1, cell2, pcomp2, nodelist2, ignorepwrgnd);
}

EDiaNetShowPreanalysis::EDiaNetShowPreanalysis()
	: EDialogModeless(&net_nccpredialog)
{
	reset();
}

void EDiaNetShowPreanalysis::reset()
{
	vSynch = TRUE;
	showComps = FALSE;
	showAll = FALSE;
	pcomp1 = pcomp2 = 0;
	nodelist1 = nodelist2 = 0;
	ignorepwrgnd = FALSE;
}

void EDiaNetShowPreanalysis::setCells(NODEPROTO *cell1, PCOMP *aPcomp1, PNET *aNodelist1,
	NODEPROTO *cell2, PCOMP *aPcomp2, PNET *aNodelist2, BOOLEAN aIgnorepwrgnd)
{
	REGISTER PCOMP *pc;

	show();

	initTextDialog(DNCP_CELL1LIST, DiaNullDlogList, DiaNullDlogItem,
		DiaNullDlogDone, -1, SCSELMOUSE|SCREPORT|SCHORIZBAR|SCSMALLFONT);
	initTextDialog(DNCP_CELL2LIST, DiaNullDlogList, DiaNullDlogItem,
		DiaNullDlogDone, -1, SCSELMOUSE|SCREPORT|SCHORIZBAR|SCSMALLFONT);

	/* make the horizontally-aligned lists scroll together */
	if (vSynch)
	{
		synchVScrolls(DNCP_CELL1LIST, DNCP_CELL2LIST, 0);
		setControl(DNCP_TIEVERTICALLY, 1);
	}
	if (showComps) setControl(DNCP_SHOWCOMPS, 1); else setControl(DNCP_SHOWNETS, 1);
	setControl(DNCP_SHOWDIFFS, !showAll);

	setText(DNCP_CELL1NAME, describenodeproto(cell1));
	setText(DNCP_CELL2NAME, describenodeproto(cell2));
	pcomp1 = aPcomp1;   nodelist1 = aNodelist1;
	pcomp2 = aPcomp2;   nodelist2 = aNodelist2;
	ignorepwrgnd = aIgnorepwrgnd;

	/* precache the component names */
	for(pc = pcomp1; pc != NOPCOMP; pc = pc->nextpcomp)
		allocstring(&pc->hashreason, net_describepcomp(pc), net_tool->cluster);
	for(pc = pcomp2; pc != NOPCOMP; pc = pc->nextpcomp)
		allocstring(&pc->hashreason, net_describepcomp(pc), net_tool->cluster);

	if (showComps) putCompIntoDialog(); else putNetIntoDialog();
}

typedef struct
{
	INTBIG     pcount;
	PORTPROTO *pp;
	NETWORK   *net;
} COMPPORT;

void EDiaNetShowPreanalysis::itemHitAction(INTBIG itemHit)
{
	REGISTER INTBIG i, j, total1, total2, pgcount1, pgcount2, i1, i2, maxlen, diff;
	REGISTER BOOLEAN first;
	REGISTER PCOMP *pc, *pc1, *pc2;
	REGISTER NETWORK *net;
	REGISTER PNET *pn;
	REGISTER void *infstr;
	REGISTER CHAR **netnames1, **netnames2, *pt1, *pt2;
	REGISTER NODEPROTO *np1, *np2, *np1orig, *np2orig;
	REGISTER PORTPROTO *pp1, *pp2;
	REGISTER NODEINST *ni, *ni1, *ni2;
	REGISTER ARCINST *ai;
	REGISTER COMPPORT *comportlist1, *comportlist2;

	if (itemHit == DNCP_CLOSE)
	{
		for(pc = pcomp1; pc != NOPCOMP; pc = pc->nextpcomp)
		{
			efree((CHAR *)pc->hashreason);
			pc->hashreason = 0;
		}
		for(pc = pcomp2; pc != NOPCOMP; pc = pc->nextpcomp)
		{
			efree((CHAR *)pc->hashreason);
			pc->hashreason = 0;
		}
		hide();
		return;
	}
	if (itemHit == DNCP_COMPARE)
	{
		if (showComps)
		{
			i = getCurLine(DNCP_CELL1LIST);
			for(pc1 = pcomp1; pc1 != NOPCOMP; pc1 = pc1->nextpcomp)
				if (pc1->timestamp == i) break;
			i = getCurLine(DNCP_CELL2LIST);
			for(pc2 = pcomp2; pc2 != NOPCOMP; pc2 = pc2->nextpcomp)
				if (pc2->timestamp == i) break;
			if (pc1 == NOPCOMP || pc2 == NOPCOMP)
			{
				ttyputmsg(_("First select a component from each list"));
				return;
			}

			/* compare components "pc1" and "pc2" */
			if (pc1->numactual == 1) ni1 = ((NODEINST *)pc1->actuallist); else
				ni1 = ((NODEINST **)pc1->actuallist)[0];
			np1orig = ni1->proto;
			np1 = contentsview(np1orig);
			if (np1 == NONODEPROTO) np1 = np1orig;
			if (pc2->numactual == 1) ni2 = ((NODEINST *)pc2->actuallist); else
				ni2 = ((NODEINST **)pc2->actuallist)[0];
			np2orig = ni2->proto;
			np2 = contentsview(np2orig);
			if (np2 == NONODEPROTO) np2 = np2orig;
			infstr = initinfstr();
			formatinfstr(infstr, _("Differences between component %s"), describenodeproto(np1));
			if (pc1->numactual > 1) formatinfstr(infstr, _(" (%ld merged nodes)"), pc1->numactual);
			formatinfstr(infstr, _(" and component %s"), describenodeproto(np2));
			if (pc2->numactual > 1) formatinfstr(infstr, _(" (%ld merged nodes)"), pc2->numactual);
			ttyputmsg(x_("%s"), returninfstr(infstr));

			/* gather information about component "pc1" */
			total1 = pgcount1 = 0;
			comportlist1 = 0;
			for(pp1 = np1->firstportproto; pp1 != NOPORTPROTO; pp1 = pp1->nextportproto)
			{
				if (portispower(pp1) || portisground(pp1))
				{
					if (pp1->network->buswidth <= 1) pgcount1++; else
						pgcount1 += pp1->network->buswidth;
					if (ignorepwrgnd) continue;
				}
				if (pp1->network->buswidth > 1)
					total1 += pp1->network->buswidth;
				total1++;
			}
			if (total1 > 0)
			{
				comportlist1 = (COMPPORT *)emalloc(total1 * (sizeof (COMPPORT)), net_tool->cluster);
				if (comportlist1 == 0) return;
				total1 = 0;
				for(pp1 = np1->firstportproto; pp1 != NOPORTPROTO; pp1 = pp1->nextportproto)
				{
					if (ignorepwrgnd && (portispower(pp1) || portisground(pp1))) continue;
					comportlist1[total1].pp = equivalentport(np1, pp1, np1orig);
					comportlist1[total1++].net = pp1->network;
					if (pp1->network->buswidth > 1)
					{
						for(i=0; i<pp1->network->buswidth; i++)
						{
							comportlist1[total1].pp = NOPORTPROTO;
							comportlist1[total1++].net = pp1->network->networklist[i];
						}
					}
				}
			}
			for(i=0; i<total1; i++) comportlist1[i].pcount = 0;
			for(i=0; i<pc1->wirecount; i++)
			{
				pp1 = pc1->portlist[i];
				for(j=0; j<total1; j++)
				{
					if (comportlist1[j].pp == pp1) comportlist1[j].pcount++;
				}
			}

			/* gather information about component "pc2" */
			total2 = pgcount2 = 0;
			comportlist2 = 0;
			for(pp2 = np2->firstportproto; pp2 != NOPORTPROTO; pp2 = pp2->nextportproto)
			{
				if (portispower(pp2) || portisground(pp2))
				{
					if (pp2->network->buswidth <= 1) pgcount2++; else
						pgcount2 += pp2->network->buswidth;
					if (ignorepwrgnd) continue;
				}
				total2++;
				if (pp2->network->buswidth > 1)
					total2 += pp2->network->buswidth;
			}
			if (total2 > 0)
			{
				comportlist2 = (COMPPORT *)emalloc(total2 * (sizeof (COMPPORT)), net_tool->cluster);
				if (comportlist2 == 0) return;
				total2 = 0;
				for(pp2 = np2->firstportproto; pp2 != NOPORTPROTO; pp2 = pp2->nextportproto)
				{
					if (ignorepwrgnd && (portispower(pp2) || portisground(pp2))) continue;
					comportlist2[total2].pp = equivalentport(np2, pp2, np2orig);
					comportlist2[total2++].net = pp2->network;
					if (pp2->network->buswidth > 1)
					{
						for(i=0; i<pp2->network->buswidth; i++)
						{
							comportlist2[total2].pp = NOPORTPROTO;
							comportlist2[total2++].net = pp2->network->networklist[i];
						}
					}
				}
			}
			for(i=0; i<total2; i++) comportlist2[i].pcount = 0;
			for(i=0; i<pc2->wirecount; i++)
			{
				pp2 = pc2->portlist[i];
				for(j=0; j<total2; j++)
				{
					if (comportlist2[j].pp == pp2) comportlist2[j].pcount++;
				}
			}

			/* analyze results */
			if (pgcount1 != pgcount2)
				ttyputmsg(_("  Cells %s has %ld power-and-ground exports but cell %s has %ld"),
					describenodeproto(np1), pgcount1, describenodeproto(np2), pgcount2);

			for(i=0; i<total1; i++)
			{
				if (comportlist1[i].net->buswidth > 1) continue;
				for(j=0; j<total2; j++)
					if (net_samenetworkname(comportlist1[i].net, comportlist2[j].net)) break;
				if (j < total2) continue;
				ttyputmsg(_("  Export %s exists only in %s"), describenetwork(comportlist1[i].net),
					describenodeproto(np1));
			}
			for(i=0; i<total2; i++)
			{
				if (comportlist2[i].net->buswidth > 1) continue;
				for(j=0; j<total1; j++)
					if (net_samenetworkname(comportlist2[i].net, comportlist1[j].net)) break;
				if (j < total1) continue;
				ttyputmsg(_("  Export %s exists only in %s"), describenetwork(comportlist2[i].net),
					describenodeproto(np2));
			}
			if (total1 > 0) efree((CHAR *)comportlist1);
			if (total2 > 0) efree((CHAR *)comportlist2);
			ttyputmsg(_("End of comparison"));


			/* now do another analysis */
			total1 = 0;
			netnames1 = 0;
			for(i=0; i<pc1->wirecount; i++)
			{
				pn = pc1->netnumbers[i];
				if (ignorepwrgnd && (pn->flags&(POWERNET|GROUNDNET)) != 0) continue;
				total1++;
			}
			if (total1 > 0)
			{
				netnames1 = (CHAR **)emalloc(total1 * (sizeof (CHAR *)), net_tool->cluster);
				if (netnames1 == 0) return;
				total1 = 0;
				for(i=0; i<pc1->wirecount; i++)
				{
					pn = pc1->netnumbers[i];
					if (ignorepwrgnd && (pn->flags&(POWERNET|GROUNDNET)) != 0) continue;
					netnames1[total1++] = pc1->portlist[i]->protoname;
				}
			}
			esort(netnames1, total1, sizeof (CHAR *), sort_stringascending);

			total2 = 0;
			netnames2 = 0;
			for(i=0; i<pc2->wirecount; i++)
			{
				pn = pc2->netnumbers[i];
				if (ignorepwrgnd && (pn->flags&(POWERNET|GROUNDNET)) != 0) continue;
				total2++;
			}
			if (total2 > 0)
			{
				netnames2 = (CHAR **)emalloc(total2 * (sizeof (CHAR *)), net_tool->cluster);
				if (netnames2 == 0) return;
				total2 = 0;
				for(i=0; i<pc2->wirecount; i++)
				{
					pn = pc2->netnumbers[i];
					if (ignorepwrgnd && (pn->flags&(POWERNET|GROUNDNET)) != 0) continue;
					netnames2[total2++] = pc2->portlist[i]->protoname;
				}
			}
			esort(netnames2, total2, sizeof (CHAR *), sort_stringascending);

			ttyputmsg(_("Comparison of wires on nodes %s and %s"), describenodeinst(ni1),
				describenodeinst(ni2));
			maxlen = estrlen(describenodeinst(ni1));
			for(i=0; i<total1; i++)
				maxlen = maxi(maxlen, estrlen(netnames1[i]));
			maxlen += 5;
			infstr = initinfstr();
			addstringtoinfstr(infstr, describenodeinst(ni1));
			for(i=estrlen(describenodeinst(ni1)); i<maxlen; i++) addtoinfstr(infstr, ' ');
			addstringtoinfstr(infstr, describenodeinst(ni2));
			ttyputmsg(x_("%s"), returninfstr(infstr));

			i1 = i2 = 0;
			for(;;)
			{
				if (i1 >= total1 && i2 >= total2) break;
				if (i1 < total1) pt1 = netnames1[i1]; else
					pt1 = x_("");
				if (i2 < total2) pt2 = netnames2[i2]; else
					pt2 = x_("");
				infstr = initinfstr();
				diff = namesame(pt1, pt2);
				if (diff < 0 && i1 >= total1) diff = 1;
				if (diff > 0 && i2 >= total2) diff = -1;
				if (diff == 0)
				{
					addstringtoinfstr(infstr, pt1);
					for(i=estrlen(pt1); i<maxlen; i++) addtoinfstr(infstr, ' ');
					addstringtoinfstr(infstr, pt2);
					if (i1 < total1) i1++;
					if (i2 < total2) i2++;
				} else if (diff < 0)
				{
					addstringtoinfstr(infstr, pt1);
					i1++;
				} else
				{
					for(i=0; i<maxlen; i++) addtoinfstr(infstr, ' ');
					addstringtoinfstr(infstr, pt2);
					i2++;
				}
				ttyputmsg(x_("%s"), returninfstr(infstr));
			}
			if (total1 > 0) efree((CHAR *)netnames1);
			if (total2 > 0) efree((CHAR *)netnames2);
		} else
		{
			ttyputmsg(_("Can only compare components"));
		}
		return;
	}
	if (itemHit == DNCP_TIEVERTICALLY)
	{
		vSynch = getControl(DNCP_TIEVERTICALLY) != 0;
		if (vSynch) synchVScrolls(DNCP_CELL1LIST, DNCP_CELL2LIST, 0); else unSynchVScrolls();
		return;
	}
	if (itemHit == DNCP_SHOWCOMPS || itemHit == DNCP_SHOWNETS || itemHit == DNCP_SHOWDIFFS)
	{
		showComps = getControl(DNCP_SHOWCOMPS) != 0;
		showAll = !getControl(DNCP_SHOWDIFFS);
		if (showComps) putCompIntoDialog(); else putNetIntoDialog();
		return;
	}
	if (itemHit == DNCP_CELL1LIST || itemHit == DNCP_CELL2LIST)
	{
		(void)asktool(us_tool, x_("clear"));
		infstr = initinfstr();
		first = TRUE;
		i = getCurLine(itemHit);
		if (showComps)
		{
			for(pc = (itemHit == DNCP_CELL1LIST ? pcomp1 : pcomp2); pc != NOPCOMP; pc = pc->nextpcomp)
			{
				if (pc->timestamp != i) continue;
				for(j=0; j<pc->numactual; j++)
				{
					if (pc->numactual == 1) ni = (NODEINST *)pc->actuallist; else
						ni = ((NODEINST **)pc->actuallist)[j];
					if (first) first = FALSE; else
						addtoinfstr(infstr, '\n');
					formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0;NOBBOX"),
						describenodeproto(ni->parent), (INTBIG)ni->geom);
				}
			}
		} else
		{
			for(pn = (itemHit == DNCP_CELL1LIST ? nodelist1 : nodelist2); pn != NOPNET; pn = pn->nextpnet)
			{
				if (pn->timestamp != i) continue;
				net = pn->network;
				if (net == NONETWORK) continue;
				for(ai = net->parent->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
				{
					if (ai->network != net)
					{
						if (ai->network->buswidth <= 1) continue;
						for(j=0; j<ai->network->buswidth; j++)
							if (ai->network->networklist[j] == net) break;
						if (j >= ai->network->buswidth) continue;
					}
					if (first) first = FALSE; else
						addtoinfstr(infstr, '\n');
					formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0;NOBBOX"),
						describenodeproto(ai->parent), (INTBIG)ai->geom);
				}
			}
		}
		(void)asktool(us_tool, x_("show-multiple"), (INTBIG)returninfstr(infstr));
		return;
	}
}

void EDiaNetShowPreanalysis::putNetIntoDialog()
{
	REGISTER INTBIG pn1total, pn2total, ind1, ind2, i, maxwirecount,
		reportedwid, curwid, line1, line2, *wirecountlist, numin1, numin2;
	CHAR *pt, line[200];
	REGISTER PNET **pn1list, **pn2list, *pn, *pn1, *pn2;
	REGISTER NETWORK *net1, *net2;
	REGISTER NODEPROTO *np1, *np2;
	REGISTER void *infstr;

	/* clear the scroll areas */
	loadTextDialog(DNCP_CELL1LIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);
	loadTextDialog(DNCP_CELL2LIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);

	/* show the networks */
	pn1total = 0;
	pn1list = 0;
	for(pn = nodelist1; pn != NOPNET; pn = pn->nextpnet)
	{
		pn->timestamp = -1;
		pn1total++;
	}
	if (pn1total > 0)
	{
		pn1list = (PNET **)emalloc(pn1total * (sizeof (PNET *)), net_tool->cluster);
		if (pn1list == 0) return;
		pn1total = 0;
		for(pn = nodelist1; pn != NOPNET; pn = pn->nextpnet)
			pn1list[pn1total++] = pn;
		esort(pn1list, pn1total, sizeof (PNET *), net_sortpnet);
	}

	/* count the number of networks in the second cell */
	pn2total = 0;
	pn2list = 0;
	for(pn = nodelist2; pn != NOPNET; pn = pn->nextpnet)
	{
		pn->timestamp = -1;
		pn2total++;
	}
	if (pn2total > 0)
	{
		pn2list = (PNET **)emalloc(pn2total * (sizeof (PNET *)), net_tool->cluster);
		if (pn2list == 0) return;
		pn2total = 0;
		for(pn = nodelist2; pn != NOPNET; pn = pn->nextpnet)
			pn2list[pn2total++] = pn;
		esort(pn2list, pn2total, sizeof (PNET *), net_sortpnet);
	}

	/* if reducing to the obvious differences, remove all with a nodecount if numbers match */
	if (!showAll)
	{
		maxwirecount = -1;
		for(pn = nodelist1; pn != NOPNET; pn = pn->nextpnet)
			if (pn->nodecount > maxwirecount) maxwirecount = pn->nodecount;
		for(pn = nodelist2; pn != NOPNET; pn = pn->nextpnet)
			if (pn->nodecount > maxwirecount) maxwirecount = pn->nodecount;
		if (maxwirecount > 0)
		{
			wirecountlist = (INTBIG *)emalloc((maxwirecount+1) * SIZEOFINTBIG, el_tempcluster);
			if (wirecountlist == 0) return;
			for(i=0; i<=maxwirecount; i++) wirecountlist[i] = 0;
			for(pn = nodelist1; pn != NOPNET; pn = pn->nextpnet)
				wirecountlist[pn->nodecount] = 1;
			for(pn = nodelist2; pn != NOPNET; pn = pn->nextpnet)
				wirecountlist[pn->nodecount] = 1;
			for(i=maxwirecount; i>=0; i--)
			{
				if (wirecountlist[i] == 0) continue;
				for(ind1 = 0, pn = nodelist1; pn != NOPNET; pn = pn->nextpnet)
					if (pn->nodecount == i) ind1++;
				for(ind2 = 0, pn = nodelist2; pn != NOPNET; pn = pn->nextpnet)
					if (pn->nodecount == i) ind2++;
				if (ind1 != ind2) continue;
				if (ind1 == 0) continue;
				for(pn = nodelist1; pn != NOPNET; pn = pn->nextpnet)
					if (pn->nodecount == i) pn->timestamp = 0;
				for(pn = nodelist2; pn != NOPNET; pn = pn->nextpnet)
					if (pn->nodecount == i) pn->timestamp = 0;
			}
			efree((CHAR *)wirecountlist);
		}
	}

	ind1 = ind2 = 0;
	line1 = line2 = 0;
	reportedwid = -1;
	for(;;)
	{
		if (ind1 >= pn1total) pn1 = NOPNET; else
		{
			pn1 = pn1list[ind1];
			if (pn1->timestamp == 0)
			{
				ind1++;
				pn1->timestamp = -1;
				continue;
			}
			if (ignorepwrgnd && (pn1->flags&(POWERNET|GROUNDNET)) != 0)
			{
				ind1++;
				pn1->timestamp = -1;
				continue;
			}
		}
		if (ind2 >= pn2total) pn2 = NOPNET; else
		{
			pn2 = pn2list[ind2];
			if (pn2->timestamp == 0)
			{
				ind2++;
				pn2->timestamp = -1;
				continue;
			}
			if (ignorepwrgnd && (pn2->flags&(POWERNET|GROUNDNET)) != 0)
			{
				ind2++;
				pn2->timestamp = -1;
				continue;
			}
		}
		if (pn1 == NOPNET && pn2 == NOPNET) break;
		if (pn1 != NOPNET && pn2 != NOPNET)
		{
			if (pn1->nodecount < pn2->nodecount) pn1 = NOPNET; else
				if (pn1->nodecount > pn2->nodecount) pn2 = NOPNET;
		}
		if (pn1 != NOPNET) curwid = pn1->nodecount; else curwid = pn2->nodecount;
		if (curwid != reportedwid)
		{
			numin1 = 0;
			if (pn1 != NOPNET)
			{
				for(i=ind1; i<pn1total; i++)
				{
					pn = pn1list[i];
					if (pn->timestamp == 0) continue;
					if (ignorepwrgnd && (pn->flags&(POWERNET|GROUNDNET)) != 0) continue;
					if (pn->nodecount != pn1->nodecount) break;
					numin1++;
				}
			}
			numin2 = 0;
			if (pn2 != NOPNET)
			{
				for(i=ind2; i<pn2total; i++)
				{
					pn = pn2list[i];
					if (pn->timestamp == 0) continue;
					if (ignorepwrgnd && (pn->flags&(POWERNET|GROUNDNET)) != 0) continue;
					if (pn->nodecount != pn2->nodecount) break;
					numin2++;
				}
			}
			esnprintf(line, 200, _(":::::::: nets with %ld components (%ld) ::::::::"), curwid, numin1);
			stuffLine(DNCP_CELL1LIST, line);
			esnprintf(line, 200, _(":::::::: nets with %ld components (%ld) ::::::::"), curwid, numin2);
			stuffLine(DNCP_CELL2LIST, line);
			line1++;   line2++;
			reportedwid = curwid;
		}

		if (pn1 == NOPNET) stuffLine(DNCP_CELL1LIST, x_("")); else
		{
			pn1->timestamp = line1;
			if ((pn1->flags&(POWERNET|GROUNDNET|EXPORTEDNET)) == 0 &&
				(pn1->network == NONETWORK || pn1->network->namecount == 0))
			{
				/* unnamed internal network: merge with others like it */
				net1 = pn1->network;
				if (net1 != NONETWORK) np1 = net1->parent; else
					np1 = NONODEPROTO;
				i = 0;
				for(;;)
				{
					i++;
					ind1++;
					if (ind1 >= pn1total) break;
					pn = pn1list[ind1];
					if (pn->nodecount != pn1->nodecount) break;
					if ((pn->flags&(POWERNET|GROUNDNET|EXPORTEDNET)) != 0 ||
						(pn->network != NONETWORK && pn->network->namecount != 0)) break;
					net2 = pn->network;
					if (net2 != NONETWORK) np2 = net2->parent; else
						np2 = NONODEPROTO;
					if (np1 != np2) break;
					pn->timestamp = line1;
				}
				if (i == 1) estrcpy(line, _("Unnamed net")); else
					esnprintf(line, 200, _("Unnamed nets (%ld)"), i);
				if (np1 != NONODEPROTO)
				{
					estrcat(line, _(" in cell "));
					estrcat(line, describenodeproto(np1));
				}
				stuffLine(DNCP_CELL1LIST, line);
			} else
			{
				(void)allocstring(&pt, net_describepnet(pn1), el_tempcluster);
				infstr = initinfstr();
				addstringtoinfstr(infstr, pt);
				i = 0;
				for(;;)
				{
					i++;
					ind1++;
					if (ind1 >= pn1total) break;
					pn = pn1list[ind1];
					if (pn->nodecount != pn1->nodecount) break;
					if (namesame(pt, net_describepnet(pn)) != 0) break;
					pn->timestamp = line1;
				}
				efree(pt);
				if (i > 1)
					formatinfstr(infstr, x_(" (%ld)"), i);
				stuffLine(DNCP_CELL1LIST, returninfstr(infstr));
			}
		}
		line1++;

		if (pn2 == NOPNET) stuffLine(DNCP_CELL2LIST, x_("")); else
		{
			pn2->timestamp = line2;
			if ((pn2->flags&(POWERNET|GROUNDNET|EXPORTEDNET)) == 0 &&
				(pn2->network == NONETWORK || pn2->network->namecount == 0))
			{
				/* unnamed internal network: merge with others like it */
				net2 = pn2->network;
				if (net2 != NONETWORK) np2 = net2->parent; else
					np2 = NONODEPROTO;
				i = 0;
				for(;;)
				{
					i++;
					ind2++;
					if (ind2 >= pn2total) break;
					pn = pn2list[ind2];
					if (pn->nodecount != pn2->nodecount) break;
					if ((pn->flags&(POWERNET|GROUNDNET|EXPORTEDNET)) != 0 ||
						(pn->network != NONETWORK && pn->network->namecount != 0)) break;
					net1 = pn->network;
					if (net1 != NONETWORK) np1 = net1->parent; else
						np1 = NONODEPROTO;
					if (np1 != np2) break;
					pn->timestamp = line2;
				}
				if (i == 1) estrcpy(line, _("Unnamed net")); else
					esnprintf(line, 200, _("Unnamed nets (%ld)"), i);
				if (np2 != NONODEPROTO)
				{
					estrcat(line, _(" in cell "));
					estrcat(line, describenodeproto(np2));
				}
				stuffLine(DNCP_CELL2LIST, line);
			} else
			{
				(void)allocstring(&pt, net_describepnet(pn2), el_tempcluster);
				infstr = initinfstr();
				addstringtoinfstr(infstr, pt);
				i = 0;
				for(;;)
				{
					i++;
					ind2++;
					if (ind2 >= pn2total) break;
					pn = pn2list[ind2];
					if (pn->nodecount != pn2->nodecount) break;
					if (namesame(pt, net_describepnet(pn)) != 0) break;
					pn->timestamp = line2;
				}
				efree(pt);
				if (i > 1)
					formatinfstr(infstr, x_(" (%ld)"), i);
				stuffLine(DNCP_CELL2LIST, returninfstr(infstr));
			}
		}
		line2++;
	}
	if (pn1total > 0) efree((CHAR *)pn1list);
	if (pn2total > 0) efree((CHAR *)pn2list);
	selectLine(DNCP_CELL1LIST, 0);
	selectLine(DNCP_CELL2LIST, 0);
}

void EDiaNetShowPreanalysis::putCompIntoDialog()
{
	REGISTER INTBIG ind1, ind2, i, maxwirecount, numin1, numin2,
		reportedwid, curwid, pc1total, pc2total, w, line1, line2;
	CHAR line[200];
	REGISTER PNET *pn;
	REGISTER PCOMP **pc1list, **pc2list, *pc, *pc1, *pc2;
	REGISTER void *infstr;

	/* clear the scroll areas */
	loadTextDialog(DNCP_CELL1LIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);
	loadTextDialog(DNCP_CELL2LIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);

	/* count the number of components in the first cell */
	pc1total = 0;
	pc1list = 0;
	for(pc = pcomp1; pc != NOPCOMP; pc = pc->nextpcomp)
	{
		pc1total++;
		pc->timestamp = -1;

		/* adjust the number of wires, removing ignored power and ground */
		w = 0;
		for(i=0; i<pc->wirecount; i++)
		{
			pn = pc->netnumbers[i];
			if (ignorepwrgnd && (pn->flags&(POWERNET|GROUNDNET)) != 0) continue;
			w++;
		}
		pc->hashvalue = pc->wirecount;
		pc->wirecount = (INTSML)w;
	}

	/* make a sorted list of the components in the first cell */
	if (pc1total > 0)
	{
		pc1list = (PCOMP **)emalloc(pc1total * (sizeof (PCOMP *)), net_tool->cluster);
		if (pc1list == 0) return;
		pc1total = 0;
		for(pc = pcomp1; pc != NOPCOMP; pc = pc->nextpcomp)
			pc1list[pc1total++] = pc;
		esort(pc1list, pc1total, sizeof (PCOMP *), net_sortpcomp);
	}

	/* count the number of components in the second cell */
	pc2total = 0;
	pc2list = 0;
	for(pc = pcomp2; pc != NOPCOMP; pc = pc->nextpcomp)
	{
		pc2total++;
		pc->timestamp = -1;

		/* adjust the number of wires, removing ignored power and ground */
		w = 0;
		for(i=0; i<pc->wirecount; i++)
		{
			pn = pc->netnumbers[i];
			if (ignorepwrgnd && (pn->flags&(POWERNET|GROUNDNET)) != 0) continue;
			w++;
		}
		pc->hashvalue = pc->wirecount;
		pc->wirecount = (INTSML)w;
	}

	/* make a sorted list of the components in the second cell */
	if (pc2total > 0)
	{
		pc2list = (PCOMP **)emalloc(pc2total * (sizeof (PCOMP *)), net_tool->cluster);
		if (pc2list == 0) return;
		pc2total = 0;
		for(pc = pcomp2; pc != NOPCOMP; pc = pc->nextpcomp)
			pc2list[pc2total++] = pc;
		esort(pc2list, pc2total, sizeof (PCOMP *), net_sortpcomp);
	}

	/* if reducing to the obvious differences, remove all with a wirecount if numbers match */
	if (!showAll)
	{
		maxwirecount = -1;
		for(pc = pcomp1; pc != NOPCOMP; pc = pc->nextpcomp)
			if (pc->wirecount > maxwirecount) maxwirecount = pc->wirecount;
		for(pc = pcomp2; pc != NOPCOMP; pc = pc->nextpcomp)
			if (pc->wirecount > maxwirecount) maxwirecount = pc->wirecount;
		for(i=maxwirecount; i>=0; i--)
		{
			for(ind1 = 0, pc = pcomp1; pc != NOPCOMP; pc = pc->nextpcomp)
				if (pc->wirecount == i) ind1++;
			for(ind2 = 0, pc = pcomp2; pc != NOPCOMP; pc = pc->nextpcomp)
				if (pc->wirecount == i) ind2++;
			if (ind1 != ind2) continue;
			if (ind1 == 0) continue;
			for(pc = pcomp1; pc != NOPCOMP; pc = pc->nextpcomp)
				if (pc->wirecount == i) pc->timestamp = 0;
			for(pc = pcomp2; pc != NOPCOMP; pc = pc->nextpcomp)
				if (pc->wirecount == i) pc->timestamp = 0;
		}
	}

	ind1 = ind2 = 0;
	line1 = line2 = 0;
	reportedwid = -1;
	for(;;)
	{
		if (ind1 >= pc1total) pc1 = NOPCOMP; else
		{
			pc1 = pc1list[ind1];
			if (pc1->timestamp == 0)
			{
				ind1++;
				pc1->timestamp = -1;
				continue;
			}
		}
		if (ind2 >= pc2total) pc2 = NOPCOMP; else
		{
			pc2 = pc2list[ind2];
			if (pc2->timestamp == 0)
			{
				ind2++;
				pc2->timestamp = -1;
				continue;
			}
		}
		if (pc1 == NOPCOMP && pc2 == NOPCOMP) break;
		if (pc1 != NOPCOMP && pc2 != NOPCOMP)
		{
			if (pc1->wirecount < pc2->wirecount) pc1 = NOPCOMP; else
				if (pc1->wirecount > pc2->wirecount) pc2 = NOPCOMP;
		}
		if (pc1 != NOPCOMP) curwid = pc1->wirecount; else curwid = pc2->wirecount;
		if (curwid != reportedwid)
		{
			numin1 = 0;
			if (pc1 != NOPCOMP)
			{
				for(i=ind1; i<pc1total; i++)
				{
					if (pc1list[i]->wirecount == pc1->wirecount) numin1++; else break;
				}
			}
			numin2 = 0;
			if (pc2 != NOPCOMP)
			{
				for(i=ind2; i<pc2total; i++)
				{
					if (pc2list[i]->wirecount == pc2->wirecount) numin2++; else break;
				}
			}
			esnprintf(line, 200, _(":::::::: components with %ld nets (%ld) ::::::::"), curwid, numin1);
			stuffLine(DNCP_CELL1LIST, line);
			esnprintf(line, 200, _(":::::::: components with %ld nets (%ld) ::::::::"), curwid, numin2);
			stuffLine(DNCP_CELL2LIST, line);
			line1++;   line2++;
			reportedwid = curwid;
		}

		if (pc1 == NOPCOMP) stuffLine(DNCP_CELL1LIST, x_("")); else
		{
			pc1->timestamp = line1;
			i = 0;
			for(;;)
			{
				i++;
				ind1++;
				if (ind1 >= pc1total) break;
				if (pc1list[ind1]->timestamp == 0) break;
				if (namesame(pc1->hashreason, pc1list[ind1]->hashreason) != 0)
					break;
				pc1list[ind1]->timestamp = line1;
			}
			infstr = initinfstr();
			if (i > 1)
				formatinfstr(infstr, x_("(%ld) "), i);
			addstringtoinfstr(infstr, pc1->hashreason);
			stuffLine(DNCP_CELL1LIST, returninfstr(infstr));
		}
		line1++;

		if (pc2 == NOPCOMP) stuffLine(DNCP_CELL2LIST, x_("")); else
		{
			pc2->timestamp = line2;
			i = 0;
			for(;;)
			{
				i++;
				ind2++;
				if (ind2 >= pc2total) break;
				if (pc2list[ind2]->timestamp == 0) break;
				if (namesame(pc2->hashreason, pc2list[ind2]->hashreason) != 0)
					break;
				pc2list[ind2]->timestamp = line2;
			}
			infstr = initinfstr();
			if (i > 1)
				formatinfstr(infstr, x_("(%ld) "), i);
			addstringtoinfstr(infstr, pc2->hashreason);
			stuffLine(DNCP_CELL2LIST, returninfstr(infstr));
		}
		line2++;
	}
	selectLine(DNCP_CELL1LIST, 0);
	selectLine(DNCP_CELL2LIST, 0);

	/* restore true wire counts */
	for(pc = pcomp1; pc != NOPCOMP; pc = pc->nextpcomp)
		pc->wirecount = (INTSML)pc->hashvalue;
	for(pc = pcomp2; pc != NOPCOMP; pc = pc->nextpcomp)
		pc->wirecount = (INTSML)pc->hashvalue;
	if (pc1total > 0) efree((CHAR *)pc1list);
	if (pc2total > 0) efree((CHAR *)pc2list);
}

/******************************** SYMMETRY GROUP SUPPORT ********************************/

/*
 * Routine to create a new symmetry group of type "grouptype" with hash value "hashvalue".
 * The group is linked into the global list of symmetry groups.
 * Returns NOSYMGROUP on error.
 */
SYMGROUP *net_newsymgroup(INTBIG grouptype, HASHTYPE hashvalue, INTBIG checksum)
{
	REGISTER SYMGROUP *sg;

	if (net_symgroupfree == NOSYMGROUP)
	{
		sg = (SYMGROUP *)emalloc(sizeof (SYMGROUP), net_tool->cluster);
		if (sg == 0) return(NOSYMGROUP);
		sg->celltotal[0] = sg->celltotal[1] = 0;
	} else
	{
		sg = net_symgroupfree;
		net_symgroupfree = sg->nextsymgroup;
	}
	sg->hashvalue = hashvalue;
	sg->grouptype = grouptype;
	sg->groupflags = 0;
	sg->groupindex = net_symgroupnumber++;
	sg->checksum = checksum;
	sg->cellcount[0] = sg->cellcount[1] = 0;
	sg->nextsymgroup = net_firstsymgroup;
	net_firstsymgroup = sg;

	/* put it in the hash table */
	if (net_insertinhashtable(sg))
		net_rebuildhashtable();
	return(sg);
}

/*
 * Routine to free symmetry group "sg" to the pool of unused ones.
 */
void net_freesymgroup(SYMGROUP *sg)
{
	sg->nextsymgroup = net_symgroupfree;
	net_symgroupfree = sg;
}

/*
 * Routine to find a unique hash number for a new symmetry group.
 */
HASHTYPE net_uniquesymmetrygrouphash(INTBIG grouptype)
{
	REGISTER SYMGROUP *sg;

	for( ; ; net_uniquehashvalue--)
	{
		sg = net_findsymmetrygroup(grouptype, net_uniquehashvalue, 0);
		if (sg == NOSYMGROUP) break;
	}
	return(net_uniquehashvalue);
}

/*
 * Routine to find the symmetry group of type "grouptype" with hash value "hashvalue".
 * The "checksum" value is an additional piece of information about this hash value
 * to ensure that there are not clashes in the codes.  Returns NOSYMGROUP if none is found.
 */
SYMGROUP *net_findsymmetrygroup(INTBIG grouptype, HASHTYPE hashvalue, INTBIG checksum)
{
	REGISTER SYMGROUP *sg;
	REGISTER INTBIG i, hashindex;

	if (grouptype == SYMGROUPNET)
	{
		hashindex = abs((INTBIG)(hashvalue % net_symgrouphashnetsize));
		for(i=0; i<net_symgrouphashnetsize; i++)
		{
			sg = net_symgrouphashnet[hashindex];
			if (sg == NOSYMGROUP) break;
			if (sg->hashvalue == hashvalue)
			{
				if (sg->checksum != checksum && hashvalue != 0 && !net_nethashclashtold)
				{
					ttyputerr(_("-- POSSIBLE NETWORK HASH CLASH (%ld AND %ld)"),
						sg->checksum, checksum);
					net_nethashclashtold = TRUE;
				}
				return(sg);
			}
			hashindex++;
			if (hashindex >= net_symgrouphashnetsize) hashindex = 0;
		}
	} else
	{
		hashindex = abs((INTBIG)(hashvalue % net_symgrouphashcompsize));
		for(i=0; i<net_symgrouphashcompsize; i++)
		{
			sg = net_symgrouphashcomp[hashindex];
			if (sg == NOSYMGROUP) break;
			if (sg->hashvalue == hashvalue)
			{
				if (sg->checksum != checksum && hashvalue != 0 && !net_comphashclashtold)
				{
					ttyputerr(_("-- POSSIBLE COMPONENT HASH CLASH (%ld AND %ld)"),
						sg->checksum, checksum);
					net_comphashclashtold = TRUE;
				}
				return(sg);
			}
			hashindex++;
			if (hashindex >= net_symgrouphashcompsize) hashindex = 0;
		}
	}
	return(NOSYMGROUP);
}

void net_rebuildhashtable(void)
{
	REGISTER INTBIG i;
	REGISTER BOOLEAN problems;
	REGISTER SYMGROUP *sg;

	for(;;)
	{
		problems = FALSE;
		for(i=0; i<net_symgrouphashcompsize; i++) net_symgrouphashcomp[i] = NOSYMGROUP;
		for(i=0; i<net_symgrouphashnetsize; i++) net_symgrouphashnet[i] = NOSYMGROUP;
		for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
		{
			problems = net_insertinhashtable(sg);
			if (problems) break;
		}
		for(sg = net_firstmatchedsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
		{
			problems = net_insertinhashtable(sg);
			if (problems) break;
		}
		if (!problems) break;
	}
}

/*
 * Routine to insert symmetry group "sg" into a hash table.  Returns true
 * if the hash table needed to be expanded (and is thus invalid now).
 */
BOOLEAN net_insertinhashtable(SYMGROUP *sg)
{
	REGISTER INTBIG i, hashindex, newsize, *newhashtableck;
	REGISTER SYMGROUP **newhashtable;

	if (sg->grouptype == SYMGROUPNET)
	{
		hashindex = abs((INTBIG)(sg->hashvalue % net_symgrouphashnetsize));
		for(i=0; i<net_symgrouphashnetsize; i++)
		{
			if (net_symgrouphashnet[hashindex] == NOSYMGROUP)
			{
				net_symgrouphashnet[hashindex] = sg;
				net_symgrouphashcknet[hashindex] = sg->checksum;
				break;
			}
			hashindex++;
			if (hashindex >= net_symgrouphashnetsize) hashindex = 0;
		}
		if (i >= net_symgrouphashnetsize)
		{
			newsize = pickprime(net_symgrouphashnetsize * 2);
			newhashtable = (SYMGROUP **)emalloc(newsize * (sizeof (SYMGROUP *)),
				net_tool->cluster);
			if (newhashtable == 0) return(FALSE);
			newhashtableck = (INTBIG *)emalloc(newsize * SIZEOFINTBIG,
				net_tool->cluster);
			if (newhashtableck == 0) return(FALSE);
			efree((CHAR *)net_symgrouphashnet);
			efree((CHAR *)net_symgrouphashcknet);
			net_symgrouphashnet = newhashtable;
			net_symgrouphashcknet = newhashtableck;
			net_symgrouphashnetsize = newsize;
			ttyputmsg(x_(" -- EXPANDING SIZE OF NETWORK HASH TABLE TO %ld ENTRIES"),
				newsize);
			return(TRUE);
		}
	} else
	{
		hashindex = abs((INTBIG)(sg->hashvalue % net_symgrouphashcompsize));
		for(i=0; i<net_symgrouphashcompsize; i++)
		{
			if (net_symgrouphashcomp[hashindex] == NOSYMGROUP)
			{
				net_symgrouphashcomp[hashindex] = sg;
				net_symgrouphashckcomp[hashindex] = sg->checksum;
				break;
			}
			hashindex++;
			if (hashindex >= net_symgrouphashcompsize) hashindex = 0;
		}
		if (i >= net_symgrouphashcompsize)
		{
			newsize = pickprime(net_symgrouphashcompsize * 2);
			newhashtable = (SYMGROUP **)emalloc(newsize * (sizeof (SYMGROUP *)),
				net_tool->cluster);
			if (newhashtable == 0) return(FALSE);
			newhashtableck = (INTBIG *)emalloc(newsize * SIZEOFINTBIG,
				net_tool->cluster);
			if (newhashtableck == 0) return(FALSE);
			efree((CHAR *)net_symgrouphashcomp);
			efree((CHAR *)net_symgrouphashckcomp);
			net_symgrouphashcomp = newhashtable;
			net_symgrouphashckcomp = newhashtableck;
			net_symgrouphashcompsize = newsize;
			ttyputmsg(x_(" -- EXPANDING SIZE OF COMPONENT HASH TABLE TO %ld ENTRIES"),
				newsize);
			return(TRUE);
		}
	}
	return(FALSE);
}

/*
 * Routine to add object "obj" to cell "f" (0 or 1) of symmetry group "sg".
 * Returns true on error.
 */
BOOLEAN net_addtosymgroup(SYMGROUP *sg, INTBIG f, void *obj)
{
	INTBIG newtotal, i, count;
	REGISTER PNET *pn;
	REGISTER PCOMP *pc;
	void **newlist;

	count = sg->cellcount[f];
	if (count >= sg->celltotal[f])
	{
		newtotal = count + 10;
		newlist = (void **)emalloc(newtotal * (sizeof (void *)), net_tool->cluster);
		if (newlist == 0) return(TRUE);
		for(i=0; i < count; i++)
			newlist[i] = sg->celllist[f][i];
		if (sg->celltotal[f] > 0) efree((CHAR *)sg->celllist[f]);
		sg->celllist[f] = newlist;
		sg->celltotal[f] = newtotal;
	}
	sg->celllist[f][count] = obj;
	sg->cellcount[f]++;
	if (sg->grouptype == SYMGROUPNET)
	{
		pn = (PNET *)obj;
		pn->timestamp = net_timestamp;
	} else
	{
		pc = (PCOMP *)obj;
		pc->timestamp = net_timestamp;
	}
	return(FALSE);
}

/*
 * Routine to remove entry "index" from cell "f" (0 or 1) of symmetry group "sg".
 */
void net_removefromsymgroup(SYMGROUP *sg, INTBIG f, INTBIG index)
{
	REGISTER INTBIG count;

	count = sg->cellcount[f];
	sg->celllist[f][index] = sg->celllist[f][count-1];
	sg->cellcount[f]--;
}

/*********************** NCC MATCH CACHING ***********************/

/*
 * Routine to preserve NCC results on cell "np1" (after being compared with "np2")
 */
void net_preserveresults(NODEPROTO *np1, NODEPROTO *np2)
{
	REGISTER time_t curtime;
	UINTBIG matchdate;
	NODEPROTO *othercell;
	INTBIG count;
	REGISTER INTBIG i, index, highestindex;
	REGISTER VARIABLE *var;
	REGISTER void *sa;
	REGISTER CHAR *name1, *name2, *pt;
	REGISTER PNET *pn1, *pn2;
	REGISTER SYMGROUP *sg;
	CHAR line[300], **stringarray;

	sa = newstringarray(net_tool->cluster);
	curtime = getcurrenttime();
	esnprintf(line, 300, x_("TIME %lu"), curtime);
	addtostringarray(sa, line);
	esnprintf(line, 300, x_("MATCH %s:%s"), np2->lib->libname, nldescribenodeproto(np2));
	addtostringarray(sa, line);
	for(sg = net_firstsymgroup; sg != NOSYMGROUP; sg = sg->nextsymgroup)
	{
		if (sg->cellcount[0] == 1 && sg->cellcount[1] == 1 && sg->grouptype == SYMGROUPNET)
		{
			/* see if names match */
			if (np1 == net_cell[0])
			{
				pn1 = (PNET *)sg->celllist[0][0];
				pn2 = (PNET *)sg->celllist[1][0];
			} else
			{
				pn1 = (PNET *)sg->celllist[1][0];
				pn2 = (PNET *)sg->celllist[0][0];
			}

			if ((pn1->flags&EXPORTEDNET) == 0 || (pn2->flags&EXPORTEDNET) == 0) continue;
			if (pn1->realportcount == 0 &&
				(pn1->network == NONETWORK || pn1->network->namecount == 0)) continue;
			if (pn2->realportcount == 0 &&
				(pn2->network == NONETWORK || pn2->network->namecount == 0)) continue;

			if (pn1->realportcount == 0)
			{
				name1 = networkname(pn1->network, 0);
			} else if (pn1->realportcount == 1)
			{
				name1 = ((PORTPROTO *)pn1->realportlist)->protoname;
			} else
			{
				name1 = (((PORTPROTO **)pn1->realportlist)[0])->protoname;
			}
			if (pn2->realportcount == 0)
			{
				name2 = networkname(pn2->network, 0);
			} else if (pn2->realportcount == 1)
			{
				name2 = ((PORTPROTO *)pn2->realportlist)->protoname;
			} else
			{
				name2 = (((PORTPROTO **)pn2->realportlist)[0])->protoname;
			}
			esnprintf(line, 300, x_("EXPORT %s:%s"), name1, name2);
			addtostringarray(sa, line);
		}
	}
	stringarray = getstringarray(sa, &count);

	/* find a place to store this information */
	highestindex = 0;
	for(i=0; i<np1->numvar; i++)
	{
		var = &np1->firstvar[i];
		pt = makename(var->key);
		if (namesamen(pt, x_("NET_ncc_last_result"), 19) != 0) continue;
		index = eatoi(&pt[19]);
		if (index > highestindex) highestindex = index;

		/* if this information is intended for the other cell, overwrite it */
		net_parsenccresult(np1, var, &othercell, &matchdate);
		if (othercell == np2)
		{
			highestindex = index-1;
			break;
		}
	}
	esnprintf(line, 300, x_("NET_ncc_last_result%ld"), highestindex+1);
	(void)setval((INTBIG)np1, VNODEPROTO, line, (INTBIG)stringarray,
		VSTRING|VISARRAY|(count<<VLENGTHSH));
	killstringarray(sa);
}

/*
 * Routine to return true if the cells "cell1" and "cell2" are already NCC'd.
 */
BOOLEAN net_nccalreadydone(NODEPROTO *cell1, NODEPROTO *cell2)
{
	REGISTER VARIABLE *var1, *var2;
	UINTBIG cell1matchdate, cell2matchdate, cell1changedate, cell2changedate;

	/* see if cell 1 has match information with cell 2 */
	var1 = net_nccfindmatch(cell1, cell2, &cell1matchdate);
	if (var1 == NOVARIABLE) return(FALSE);
	cell1changedate = net_recursiverevisiondate(cell1);
	if (cell1changedate > cell1matchdate) return(FALSE);

	/* see if cell 2 has match information with cell 1 */
	var2 = net_nccfindmatch(cell2, cell1, &cell2matchdate);
	if (var2 == NOVARIABLE) return(FALSE);
	cell2changedate = net_recursiverevisiondate(cell2);
	if (cell2changedate > cell2matchdate) return(FALSE);

	/* the cells are already checked */
	return(TRUE);
}

/*
 * Routine to recursively obtain the most recent revision date for cell "cell"
 * or any of its subcells.
 */
UINTBIG net_recursiverevisiondate(NODEPROTO *cell)
{
	REGISTER UINTBIG latestrevision, instancerevision;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np, *cnp;

	latestrevision = cell->revisiondate;
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		np = ni->proto;
		if (np->primindex != 0) continue;
		cnp = contentsview(np);
		if (cnp == NONODEPROTO) cnp = np;
		if (cnp == cell) continue;
		instancerevision = net_recursiverevisiondate(cnp);
		if (instancerevision > latestrevision)
			latestrevision = instancerevision;
	}
	return(latestrevision);
}

/*
 * Routine to scan cell "np" looking for an NCC match to cell "onp".  If found,
 * the date of the match is returned in "matchdate" and the variable describing
 * the match information is returned.  Returns NOVARIABLE if not found.
 */
VARIABLE *net_nccfindmatch(NODEPROTO *np, NODEPROTO *onp, UINTBIG *matchdate)
{
	REGISTER INTBIG i;
	NODEPROTO *othernp;
	REGISTER VARIABLE *var;
	REGISTER CHAR *pt;

	/* find a place to store this information */
	for(i=0; i<np->numvar; i++)
	{
		var = &np->firstvar[i];
		pt = makename(var->key);
		if (namesamen(pt, x_("NET_ncc_last_result"), 19) != 0) continue;
		net_parsenccresult(np, var, &othernp, matchdate);
		if (othernp == onp) return(var);
	}
	return(NOVARIABLE);
}

/*
 * Routine to return nonzero if cell "np" has NCC match information that is
 * still current with any other cell.
 */
INTBIG net_ncchasmatch(NODEPROTO *np)
{
	REGISTER INTBIG i;
	UINTBIG matchdate, revisiondate;
	NODEPROTO *othernp;
	REGISTER VARIABLE *var;
	REGISTER CHAR *pt;

	for(i=0; i<np->numvar; i++)
	{
		var = &np->firstvar[i];
		pt = makename(var->key);
		if (namesamen(pt, x_("NET_ncc_last_result"), 19) != 0) continue;
		net_parsenccresult(np, var, &othernp, &matchdate);
		revisiondate = net_recursiverevisiondate(np);
		if (revisiondate <= matchdate) return(1);
	}
	return(0);
}

/*
 * Routine to remove all NCC match information on cell "np".
 */
void net_nccremovematches(NODEPROTO *np)
{
	REGISTER INTBIG i;
	REGISTER VARIABLE *var;
	REGISTER CHAR *pt;

	/* find a place to store this information */
	for(i=0; i<np->numvar; i++)
	{
		var = &np->firstvar[i];
		pt = makename(var->key);
		if (namesamen(pt, x_("NET_ncc_last_result"), 19) != 0) continue;
		(void)delvalkey((INTBIG)np, VNODEPROTO, var->key);
		i--;
	}
}

/*
 * Routine to parse the NCC results on cell "np" and store the information in
 * "cellmatch" (the cell that was matched) and "celldate" (the time of the match)
 */
void net_nccmatchinfo(NODEPROTO *np, NODEPROTO **cellmatch, UINTBIG *celldate)
{
	REGISTER VARIABLE *var;
	REGISTER INTBIG i;
	REGISTER CHAR *pt;

	*cellmatch = NONODEPROTO;
	*celldate = 0;
	for(i=0; i<np->numvar; i++)
	{
		var = &np->firstvar[i];
		pt = makename(var->key);
		if (namesamen(pt, x_("NET_ncc_last_result"), 19) != 0) continue;
		net_parsenccresult(np, var, cellmatch, celldate);
		break;
	}
}

/*
 * Routine to parse the NCC results on cell "np" in variable "var" and store the
 * information in "cellmatch" (the cell that was matched) and "celldate" (the
 * time of the match).
 */
void net_parsenccresult(NODEPROTO *np, VARIABLE *var, NODEPROTO **cellmatch,
	UINTBIG *celldate)
{
	REGISTER INTBIG len, i;
	REGISTER CHAR **strings, *pt;
	Q_UNUSED( np );

	*cellmatch = NONODEPROTO;
	*celldate = 0;
	if (var == NOVARIABLE) return;
	len = getlength(var);
	strings = (CHAR **)var->addr;
	for(i=0; i<len; i++)
	{
		pt = strings[i];
		if (namesamen(pt, x_("TIME "), 5) == 0)
		{
			*celldate = eatoi(&pt[5]);
			continue;
		}
		if (namesamen(pt, x_("MATCH "), 6) == 0)
		{
			*cellmatch = getnodeproto(&pt[6]);
			continue;
		}
	}
}

/*********************** HELPER ROUTINES ***********************/

CHAR *net_describepcomp(PCOMP *pc)
{
	REGISTER INTBIG i;
	REGISTER NODEINST *ni;
	REGISTER void *infstr;

	infstr = initinfstr();
	for(i=0; i<pc->numactual; i++)
	{
		if (pc->numactual == 1) ni = (NODEINST *)pc->actuallist; else
			ni = ((NODEINST **)pc->actuallist)[i];
		if (i != 0) addtoinfstr(infstr, '/');
		addstringtoinfstr(infstr, ntdescribenodeinst(ni));
	}
	return(returninfstr(infstr));
}

CHAR *net_describepnet(PNET *pn)
{
	REGISTER PORTPROTO *pp;
	REGISTER INTBIG i;
	REGISTER NETWORK *net;
	REGISTER void *infstr;

	net = pn->network;
	infstr = initinfstr();
	if ((pn->flags&POWERNET) != 0) addstringtoinfstr(infstr, _("POWER "));
	if ((pn->flags&GROUNDNET) != 0) addstringtoinfstr(infstr, _("GROUND "));
	if ((pn->flags&EXPORTEDNET) == 0)
	{
		if (net == NONETWORK) addstringtoinfstr(infstr, _("INTERNAL")); else
		{
			if (net->globalnet >= 0 && net->globalnet < net->parent->globalnetcount)
			{
				addstringtoinfstr(infstr, describenetwork(net));
			} else
			{
				formatinfstr(infstr, _("INTERNAL %s:%s"), describenodeproto(net->parent),
					describenetwork(net));
			}
		}
	} else
	{
#ifdef PATHTOPNET
		if (pn->hierpathcount > 0)
		{
			for(i=0; i<pn->hierpathcount; i++)
			{
				addstringtoinfstr(infstr, describenodeinst(pn->hierpath[i]));
				if (pn->hierindex[i] != 0) formatinfstr(infstr, "[%ld]", pn->hierindex[i]);
				addstringtoinfstr(infstr, ".");
			}
		} else addstringtoinfstr(infstr, "TOP.");
#endif
		if (pn->realportcount == 1)
		{
			pp = (PORTPROTO *)pn->realportlist;
			if (pp->network->buswidth > 1 && net != NONETWORK)
				addstringtoinfstr(infstr, describenetwork(net)); else
					addstringtoinfstr(infstr, pp->protoname);
		} else if (pn->realportcount > 1)
		{
			for(i=0; i<pn->realportcount; i++)
			{
				pp = ((PORTPROTO **)pn->realportlist)[i];
				if (i > 0) addtoinfstr(infstr, ',');
				if (i == 0 && pp->network->buswidth > 1 && net != NONETWORK)
					addstringtoinfstr(infstr, describenetwork(net)); else
						addstringtoinfstr(infstr, pp->protoname);
			}
		} else
		{
			net = pn->network;
			addstringtoinfstr(infstr, describenetwork(net));
		}
	}
	return(returninfstr(infstr));
}

/*
 * Routine to see if the component values "v1" and "v2" are within the prescribed
 * tolerance (in "net_ncc_tolerance" and "net_ncc_tolerance_amt").  Returns true if so.
 */
BOOLEAN net_componentequalvalue(float v1, float v2)
{
	float tolerance, largest, diff;

	/* first see if it is within tolerance amount */
	diff = (float)fabs(v1 - v2);
	if (diff <= net_ncc_tolerance_amt) return(TRUE);

	/* now see if it is within tolerance percentage */
	if (v1 > v2) largest = v1; else largest = v2;
	tolerance = largest * net_ncc_tolerance / (WHOLE * 100.0f);
	if (diff <= tolerance) return(TRUE);

	/* not within any tolerance */
	return(FALSE);
}

/*
 * Routine to return true if component "pc" is a SPICE component.
 */
BOOLEAN net_isspice(PCOMP *pc)
{
	REGISTER NODEINST *ni;

	switch (pc->function)
	{
		case NPMETER:
		case NPSOURCE:
			return(TRUE);
	}
	if (pc->numactual == 1)
	{
		ni = (NODEINST *)pc->actuallist;
		if (ni->proto->primindex == 0 &&
			namesamen(ni->proto->lib->libname, x_("spiceparts"), 10) == 0)
				return(TRUE);
	}
	return(FALSE);
}

void net_initdiff(void)
{
	REGISTER INTBIG i;

	if (net_nodeCountMultiplier == 0)
	{
		i = 0;
		net_nodeCountMultiplier = getprime(i++);
		net_portFactorMultiplier = getprime(i++);
		net_functionMultiplier = getprime(i++);
		net_portNetFactorMultiplier = getprime(i++);
		net_portHashFactorMultiplier = getprime(i++);
	}
}
