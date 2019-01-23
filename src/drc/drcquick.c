/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: drcquick.c
 * Design-rule check tool: hierarchical checker
 * WRitten by: Steven M. Rubin, Static Free Software
 *
 * Copyright (c) 2002 Static Free Software.
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
#if DRCTOOL

#include "global.h"
#include "egraphics.h"
#include "tech.h"
#include "tecgen.h"
#include "drc.h"
#include "efunction.h"
#include "usr.h"
#include <math.h>

/*
 * This is the "quick" DRC which does full hierarchical examination of the circuit.
 *
 * The "quick" DRC works as follows:
 *    It first examines every primitive node and arc in the cell
 *        For each layer on these objects, it examines everything surrounding it, even in subcells
 *            R-trees are used to limit search.
 *            Where cell instances are found, the contents are examined recursively
 *    It next examines every cell instance in the cell
 *        All other instances within the surrounding area are considered
 *            When another instance is found, the two instances are examined for interaction
 *                A cache is kept of instance pairs in specified configurations to speed-up arrays
 *            All objects in the other instance that are inside the bounds of the first are considered
 *                The other instance is hierarchically examined to locate primitives in the area of consideration
 *            For each layer on each primitive object found in the other instance,
 *                Examine the contents of the first instance for interactions about that layer
 *
 * Since Electric understands connectivity, it uses this information to determine whether two layers
 * are connected or not.  However, if such global connectivity is propagated in the standard Electric
 * way (placing numbers on exports, descending into the cell, and pulling the numbers onto local networks)
 * then it is not possible to decompose the DRC for multiple processors, since two different processors
 * may want to write global network information on the same local networks at once.
 *
 * To solve this problem, the "quick" DRC determines how many instances of each cell exist.  For every
 * network in every cell, an array is built that is as large as the number of instances of that cell.
 * This array contains the global network number for that each instance of the cell.  The algorithm for
 * building these arrays is quick (1 second for a million-transistor chip) and the memory requirement
 * is not excessive (8 megabytes for a million-transistor chip).  It uses the CHECKINST and CHECKPROTO
 * objects.
 */

/* #define ALLERRORS       1 */			/* uncomment to generate all errors instead of first on object */
#define VALIDATENETWORKS   1			/* comment out when we believe this works */

#define MAXCHECKOBJECTS   50			/* number to hand-out to each processor */
#define NONET             -1

/* the different types of errors */
#define SPACINGERROR       1
#define MINWIDTHERROR      2
#define NOTCHERROR         3
#define MINSIZEERROR       4
#define BADLAYERERROR      5
#define LAYERSURROUNDERROR 6

/*
 * The CHECKINST object sits on every cell instance in the library.
 * It helps determine network information on a global scale.
 * It takes a "global-index" parameter, inherited from above (intially zero).
 * It then computes its own index number as follows:
 *   thisindex = global-index * multiplier + localindex + offset
 * This individual index is used to lookup an entry on each network in the cell
 * (an array is stored on each network, giving its global net number).
 */
#define NOCHECKINST ((CHECKINST *)-1)

typedef struct
{
	INTBIG localindex;
	INTBIG multiplier;
	INTBIG offset;
} CHECKINST;


/*
 * The CHECKPROTO object is placed on every cell and is used only temporarily
 * to number the instances.
 */
#define NOCHECKPROTO ((CHECKPROTO *)-1)

typedef struct Icheckproto
{
	INTBIG    timestamp;					/* time stamp for counting within a particular parent */
	INTBIG    instancecount;				/* number of instances of this cell in a particular parent */
	INTBIG    hierinstancecount;			/* total number of instances of this cell, hierarchically */
	INTBIG    totalpercell;					/* number of instances of this cell in a particular parent */
	BOOLEAN   cellchecked;					/* true if this cell has been checked */
	BOOLEAN   cellparameterized;			/* true if this cell has parameters */
	NODEINST *firstincell;					/* head of list of instances in a particular parent */
	struct Icheckproto *nextcheckproto;		/* next in list of these modules found */
} CHECKPROTO;

/*
 * The CHECKSTATE object exists for each processor and has global information for that thread.
 */
typedef struct
{
	POLYLIST  *cellinstpolylist;
	POLYLIST  *nodeinstpolylist;
	POLYLIST  *arcinstpolylist;
	POLYLIST  *subpolylist;
	POLYLIST  *cropnodepolylist;
	POLYLIST  *croparcpolylist;
	POLYLIST  *layerlookpolylist;
	POLYLIST  *activecroppolylist;
	POLYGON   *checkdistpoly1rebuild;
	POLYGON   *checkdistpoly2rebuild;
	INTBIG     globalindex;
	NODEINST  *tinynodeinst;
	GEOM      *tinyobj;
	void      *hierarchybasesnapshot;		/* traversal path at start of instance interaction check */
	void      *hierarchytempsnapshot;		/* traversal path when at bottom of one interaction check */
} CHECKSTATE;


/*
 * The INSTINTER object records interactions between two cell instances and prevents checking
 * them multiple times.
 */
#define NOINSTINTER ((INSTINTER *)-1)

typedef struct
{
	NODEPROTO *cell1,  *cell2;			/* the two cell instances being compared */
	INTBIG     rot1,     trn1;			/* orientation of cell instance 1 */
	INTBIG     rot2,     trn2;			/* orientation of cell instance 2 */
	INTBIG     dx, dy;					/* distance from instance 1 to instance 2 */
} INSTINTER;

static INSTINTER  **dr_quickinstinter;				/* an array of interactions that have been checked */
static INTBIG       dr_quickinstintertotal = 0;		/* size of interaction array */
static INTBIG       dr_quickinstintercount = 0;		/* number of entries in interaction array */
static INSTINTER   *dr_quickinstinterfree = NOINSTINTER;	/* an array of interactions that have been checked */

typedef struct
{
	NODEPROTO *cell;
	POLYGON   *poly;
	INTBIG     lx, hx, ly, hy;
} DRCEXCLUSION;

static INTBIG        dr_quickexclusioncount;		/* number of areas being excluded */
static INTBIG        dr_quickexclusiontotal = 0;	/* number of polygons allocated in exclusion list */
static DRCEXCLUSION *dr_quickexclusionlist;			/* list of excluded polygons */
static BOOLEAN       dr_quickexclusionwarned;		/* true if user warned about overflow */

static BOOLEAN      dr_quickparalleldrc;			/* true if doing DRC on multiple processes */
static INTBIG       dr_quicknumprocesses;			/* number of processes for doing DRC */
static INTBIG       dr_quickmainthread;				/* the main thread */
static CHECKSTATE **dr_quickstate;					/* one for each process */
static void        *dr_quickmutexnode = 0;			/* for locking node distribution */
static void        *dr_quickmutexio = 0;			/* for locking I/O */
static void        *dr_quickmutexinteract = 0;		/* for locking interaction checks */
static void       **dr_quickprocessdone;			/* lock to tell when process is done */
static INTBIG       dr_quickstatecount = 0;			/* number of per-process state blocks */

static NODEINST    *dr_quickparallelcellinst;		/* next cell instance to be checked */
static NODEINST    *dr_quickparallelnodeinst;		/* next primitive node to be checked */
static ARCINST     *dr_quickparallelarcinst;		/* next arc to be checked */

static INTBIG       dr_quickchecktimestamp;
static INTBIG       dr_quickchecknetnumber;
static INTBIG       dr_quickcheckunconnetnumber;
static CHECKPROTO  *dr_quickcheckprotofree = NOCHECKPROTO;
static CHECKINST   *dr_quickcheckinstfree = NOCHECKINST;
static INTBIG       dr_quickoptions;				/* DRC options for this run */
static INTBIG       dr_quickinteractiondistance;	/* maximum area to examine (based on worst design rule) */
static INTBIG       dr_quickerrorsfound;			/* number of errors found */
static INTBIG       dr_quickarealx;					/* low X of area being checked (if checking in area) */
static INTBIG       dr_quickareahx;					/* high X of area being checked (if checking in area) */
static INTBIG       dr_quickarealy;					/* low Y of area being checked (if checking in area) */
static INTBIG       dr_quickareahy;					/* high Y of area being checked (if checking in area) */
static BOOLEAN      dr_quickjustarea;				/* true if checking in an area only */

/* for figuring out which layers are valid for DRC */
       TECHNOLOGY  *dr_curtech = NOTECHNOLOGY;		/* currently technology with cached layer info */
static INTBIG       dr_layertotal = 0;				/* number of layers in cached technology */
       BOOLEAN     *dr_layersvalid;					/* list of valid layers in cached technology */

/* for tracking which layers interact with which nodes */
static TECHNOLOGY  *dr_quicklayerintertech = NOTECHNOLOGY;
static INTBIG       dr_quicklayerinternodehashsize = 0;
static NODEPROTO  **dr_quicklayerinternodehash;
static BOOLEAN    **dr_quicklayerinternodetable;
static INTBIG       dr_quicklayerinterarchashsize = 0;
static ARCPROTO   **dr_quicklayerinterarchash;
static BOOLEAN    **dr_quicklayerinterarctable;

static void        dr_quickaccumulateexclusion(INTBIG depth, NODEPROTO **cell, XARRAY *trans);
static BOOLEAN     dr_quickactiveontransistor(POLYGON *poly1, INTBIG layer1, INTBIG net1,
					POLYGON *poly2, INTBIG layer2, INTBIG net2, TECHNOLOGY *tech, NODEPROTO *cell, INTBIG globalindex);
static BOOLEAN     dr_quickactiveontransistorrecurse(INTBIG blx, INTBIG bhx, INTBIG bly, INTBIG bhy,
					INTBIG net1, INTBIG net2, NODEPROTO *cell, INTBIG globalindex, XARRAY trans);
static CHECKINST  *dr_quickalloccheckinst(void);
static CHECKPROTO *dr_quickalloccheckproto(void);
static INSTINTER  *dr_quickallocinstinter(void);
static BOOLEAN     dr_quickbadbox(POLYGON *poly, INTBIG layer, INTBIG net, TECHNOLOGY *tech, GEOM *geom,
					XARRAY trans, NODEPROTO *cell, INTBIG globalindex, CHECKSTATE *state);
static BOOLEAN     dr_quickbadboxinarea(POLYGON *poly, INTBIG layer, TECHNOLOGY *tech, INTBIG net, GEOM *geom, XARRAY trans,
					INTBIG globalindex,
					INTBIG lxbound, INTBIG hxbound, INTBIG lybound, INTBIG hybound, NODEPROTO *cell, INTBIG cellglobalindex,
					NODEPROTO *topcell, INTBIG topglobalindex, XARRAY toptrans, INTBIG minsize, BOOLEAN basemulti,
					CHECKSTATE *state, BOOLEAN sameinstance);
static BOOLEAN     dr_quickbadsubbox(POLYGON *poly, INTBIG layer, INTBIG net, TECHNOLOGY *tech, GEOM *geom, XARRAY trans,
					INTBIG globalindex, NODEPROTO *cell, NODEINST *oni, INTBIG topglobalindex, CHECKSTATE *state);
static void        dr_quickbuildlayerinteractions(TECHNOLOGY *tech);
static BOOLEAN     dr_quickcheckarcinst(ARCINST *ai, INTBIG globalindex, CHECKSTATE *state);
static BOOLEAN     dr_quickcheckdist(TECHNOLOGY *tech, NODEPROTO *cell, INTBIG globalindex,
					POLYGON *poly1, INTBIG layer1, INTBIG net1, GEOM *geom1, XARRAY trans1, INTBIG globalindex1,
					POLYGON *poly2, INTBIG layer2, INTBIG net2, GEOM *geom2, XARRAY trans2, INTBIG globalindex2,
					BOOLEAN con, INTBIG dist, INTBIG edge, CHAR *rule, CHECKSTATE *state);
static void        dr_quickcheckenumerateinstances(NODEPROTO *cell);
static void        dr_quickcheckenumeratenetworks(NODEPROTO *cell, INTBIG globalindex);
static BOOLEAN     dr_quickcheckcellinst(NODEINST *ni, INTBIG globalindex, CHECKSTATE *state);
static BOOLEAN     dr_quickcheckcellinstcontents(INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy, NODEPROTO *np,
					XARRAY uptrans, INTBIG globalindex, NODEINST *oni, INTBIG topglobalindex, CHECKSTATE *state);
static BOOLEAN     dr_quickcheckgeomagainstinstance(GEOM *geom, NODEINST *ni, CHECKSTATE *state);
static BOOLEAN     dr_quickcheckinteraction(NODEINST *ni1, NODEINST *ni2, INSTINTER **dii);
static BOOLEAN     dr_quickchecklayerwitharc(INTBIG layer, ARCPROTO *ap);
static BOOLEAN     dr_quickchecklayerwithnode(INTBIG layer, NODEPROTO *np);
static BOOLEAN     dr_quickcheckminwidth(GEOM *geom, INTBIG layer, POLYGON *poly, TECHNOLOGY *tech, CHECKSTATE *state);
static BOOLEAN     dr_quickchecknodeinst(NODEINST *ni, INTBIG globalindex, CHECKSTATE *state);
static void        dr_quickchecktheseinstances(NODEPROTO *cell, INTBIG count,
					NODEINST **nodestocheck, BOOLEAN *validity);
static INTBIG      dr_quickcheckthiscell(NODEPROTO *cell, INTBIG globalindex, INTBIG lx, INTBIG hx,
					INTBIG ly, INTBIG hy, BOOLEAN justarea);
static void        dr_quickclearinstancecache(void);
static void        dr_quickcropactivearc(ARCINST *ai, POLYLIST *plist, CHECKSTATE *state);
static BOOLEAN     dr_quickcroparcinst(ARCINST *ai, INTBIG lay, XARRAY transin,
					INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy, CHECKSTATE *state);
static INTBIG      dr_quickcropbox(INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy, INTBIG bx, INTBIG ux, INTBIG by,
					INTBIG uy, INTBIG nlx, INTBIG nhx, INTBIG nly, INTBIG nhy);
static BOOLEAN     dr_quickcropnodeinst(NODEINST *ni, INTBIG globalindex, CHECKSTATE *state,
					XARRAY trans, INTBIG nlx, INTBIG nhx, INTBIG nly, INTBIG nhy,
					INTBIG nlayer, INTBIG nnet, GEOM *ngeom, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy);
static void       *dr_quickdothread(void *argument);
static INSTINTER  *dr_quickfindinteraction(INSTINTER *dii);
static INTBIG      dr_quickfindinterveningpoints(POLYGON*, POLYGON*, INTBIG*, INTBIG*, INTBIG*, INTBIG*);
static BOOLEAN     dr_quickfindparameters(NODEPROTO *cell);
static void        dr_quickfreecheckinst(CHECKINST *ci);
static void        dr_quickfreecheckproto(CHECKPROTO *cp);
static void        dr_quickfreeinstinter(INSTINTER *dii);
static void        dr_quickgetarcpolys(ARCINST *ai, POLYLIST *plist, XARRAY trans);
static INTBIG      dr_quickgetnetnumber(PORTPROTO *pp, NODEINST *ni, INTBIG globalindex);
static INTBIG      dr_quickgetnextparallelgeoms(GEOM **list, CHECKSTATE *state);
static void        dr_quickgetnodeEpolys(NODEINST *ni, POLYLIST *plist, XARRAY trans);
static void        dr_quickgetnodepolys(NODEINST *ni, POLYLIST *plist, XARRAY trans);
static INTBIG      dr_quickhalfcropbox(INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy,
					INTBIG bx, INTBIG ux, INTBIG by, INTBIG uy);
static BOOLEAN     dr_quickinsertinteraction(INSTINTER *dii);
static BOOLEAN     dr_quickismulticut(NODEINST *ni);
static BOOLEAN     dr_quicklookforlayer(NODEPROTO *cell, INTBIG layer, XARRAY moretrans, CHECKSTATE *state,
					INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy, INTBIG xf1, INTBIG yf1, BOOLEAN *p1found,
					INTBIG xf2, INTBIG yf2, BOOLEAN *p2found, INTBIG xf3, INTBIG yf3, BOOLEAN *p3found);
static BOOLEAN     dr_quicklookforpoints(INTBIG xf1, INTBIG yf1, INTBIG xf2, INTBIG yf2,
					INTBIG layer, NODEPROTO *cell, BOOLEAN needboth, CHECKSTATE *state);
static BOOLEAN     dr_quickmakestateblocks(INTBIG numstates);
static BOOLEAN     dr_quickobjtouch(GEOM *geom1, GEOM *geom2);
static void        dr_quickreporterror(INTBIG errtype, TECHNOLOGY *tech, CHAR *msg,
					NODEPROTO *cell, INTBIG limit, INTBIG actual, CHAR *rule,
					POLYGON *poly1, GEOM *geom1, INTBIG layer1, INTBIG net1,
					POLYGON *poly2, GEOM *geom2, INTBIG layer2, INTBIG net2);

#define MAXHIERARCHYDEPTH 100

/*
 * This is the entry point for DRC.
 *
 * Routine to do a hierarchical DRC check on cell "cell".
 * If "count" is zero, check the entire cell.
 * If "count" is nonzero, only check that many instances (in "nodestocheck") and set the
 * entry in "validity" TRUE if it is DRC clean.
 * If "justarea" is TRUE, only check in the selected area.
 */
void dr_quickcheck(NODEPROTO *cell, INTBIG count, NODEINST **nodestocheck, BOOLEAN *validity,
	BOOLEAN justarea)
{
	REGISTER NODEPROTO *np, *snp;
	REGISTER NODEINST *ni;
	REGISTER CHECKINST *ci;
	REGISTER CHECKPROTO *cp, *ocp;
	REGISTER LIBRARY *lib;
	REGISTER NETWORK *net;
	REGISTER TECHNOLOGY *tech;
	REGISTER VARIABLE *var;
	REGISTER INTBIG totalnetworks, i, l, *netnumbers, errorcount;
	INTBIG lx, hx, ly, hy;
	float t;
	NODEPROTO *cellarray[MAXHIERARCHYDEPTH];
	XARRAY xarrayarray[MAXHIERARCHYDEPTH];

	/* start the clock */
	starttimer();

	/* get the current DRC options */
	dr_quickoptions = dr_getoptionsvalue();

	/* see if DRC will be done with multiple processes */
	if (graphicshas(CANDOTHREADS) && (dr_quickoptions&DRCMULTIPROC) != 0)
	{
		dr_quicknumprocesses = (dr_quickoptions&DRCNUMPROC) >> DRCNUMPROCSH;
	} else
	{
		dr_quicknumprocesses = 1;
	}

	/* if checking specific instances, adjust options and processor count */
	if (count > 0)
	{
		dr_quickoptions |= DRCFIRSTERROR;
		dr_quicknumprocesses = 1;
	}

	/* cache valid layers for this technology */
	tech = cell->tech;
	dr_cachevalidlayers(tech);
	dr_quickbuildlayerinteractions(tech);

	/* clean out the cache of instances */
	dr_quickclearinstancecache();

	/* determine maximum DRC interaction distance */
	dr_quickinteractiondistance = 0;
	for(i = 0; i < tech->layercount; i++)
	{
		l = maxdrcsurround(tech, cell->lib, i);
		if (l > dr_quickinteractiondistance) dr_quickinteractiondistance = l;
	}

	/* initialize all cells for hierarchical network numbering */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			cp = dr_quickalloccheckproto();
			cp->instancecount = 0;
			cp->timestamp = 0;
			cp->hierinstancecount = 0;
			cp->totalpercell = 0;
			cp->cellchecked = FALSE;
			cp->cellparameterized = FALSE;
			for(i=0; i<np->numvar; i++)
			{
				var = &np->firstvar[i];
				if (TDGETISPARAM(var->textdescript) != 0) break;
			}
			if (i < np->numvar) cp->cellparameterized = TRUE;
			np->temp1 = (INTBIG)cp;
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto->primindex != 0) continue;

				/* ignore documentation icons */
				if (isiconof(ni->proto, np)) continue;

				ci = dr_quickalloccheckinst();
				if (ci == NOCHECKINST) return;
				ni->temp1 = (INTBIG)ci;
			}
		}
	}

	/* see if any parameters are used below this cell */
	if (dr_quickfindparameters(cell))
	{
		/* parameters found: cannot use multiple processors */
		ttyputmsg(_("Parameterized layout being used: multiprocessor decomposition disabled"));
		dr_quicknumprocesses = 1;
	}

	/* now recursively examine, setting information on all instances */
	cp = (CHECKPROTO *)cell->temp1;
	cp->hierinstancecount = 1;
	dr_quickchecktimestamp = 0;
	dr_quickcheckenumerateinstances(cell);

	/* now allocate space for hierarchical network arrays */
	totalnetworks = 0;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			cp = (CHECKPROTO *)np->temp1;
			if (cp->hierinstancecount > 0)
			{
				/* allocate net number lists for every net in the cell */
				for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
				{
					netnumbers = (INTBIG *)emalloc(cp->hierinstancecount * SIZEOFINTBIG,
						dr_tool->cluster);
					net->temp1 = (INTBIG)netnumbers;
					for(i=0; i<cp->hierinstancecount; i++) netnumbers[i] = 0;
					totalnetworks += cp->hierinstancecount;
				}
			}
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto->primindex != 0) continue;

				/* ignore documentation icons */
				if (isiconof(ni->proto, np)) continue;

				ci = (CHECKINST *)ni->temp1;
				ocp = (CHECKPROTO *)ni->proto->temp1;
				ci->offset = ocp->totalpercell;
			}
			dr_quickchecktimestamp++;
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto->primindex != 0) continue;

				/* ignore documentation icons */
				if (isiconof(ni->proto, np)) continue;

				ocp = (CHECKPROTO *)ni->proto->temp1;
				if (ocp->timestamp != dr_quickchecktimestamp)
				{
					ci = (CHECKINST *)ni->temp1;
					ocp->timestamp = dr_quickchecktimestamp;
					ocp->totalpercell += cp->hierinstancecount * ci->multiplier;
				}
			}
		}
	}

	/* now fill in the hierarchical network arrays */
	dr_quickchecktimestamp = 0;
	dr_quickchecknetnumber = 1;
	for(net = cell->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		net->temp2 = dr_quickchecknetnumber++;
	dr_quickcheckenumeratenetworks(cell, 0);
	dr_quickcheckunconnetnumber = dr_quickchecknetnumber;

#ifdef VALIDATENETWORKS 	/* assert: all network numbers must be filled-in */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			cp = (CHECKPROTO *)np->temp1;
			if (cp->hierinstancecount > 0)
			{
				/* allocate net number lists for every net in the cell */
				for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
				{
					netnumbers = (INTBIG *)net->temp1;
					for(i=0; i<cp->hierinstancecount; i++)
						if (netnumbers[i] == 0)
							ttyputmsg(x_("Missing network numbers on network %s in cell %s"),
								describenetwork(net), describenodeproto(np));
				}
			}
		}
	}
#endif

	if (count <= 0)
	{
		t = endtimer();
		ttyputmsg(_("Found %ld networks, allocated %ld bytes for network numbering (took %s)"),
			dr_quickchecknetnumber, totalnetworks*SIZEOFINTBIG, explainduration(t));
	}

	/* now search for DRC exclusion areas */
	dr_quickexclusioncount = 0;
	cellarray[0] = cell;
	dr_quickexclusionwarned = FALSE;
	transid(xarrayarray[0]);
	dr_quickaccumulateexclusion(1, cellarray, xarrayarray);

	if (justarea)
	{
		snp = us_getareabounds(&lx, &hx, &ly, &hy);
		if (snp != cell)
		{
			ttyputmsg(_("Cannot check selection: it is not in the current cell"));
			justarea = FALSE;
		}
	} else lx = hx = ly = hy = 0;

	/* initialize for multiple processors */
	if (dr_quickmakestateblocks(dr_quicknumprocesses)) return;
	if (ensurevalidmutex(&dr_quickmutexnode, TRUE)) return;
	if (ensurevalidmutex(&dr_quickmutexio,   TRUE)) return;
	if (ensurevalidmutex(&dr_quickmutexinteract,   TRUE)) return;
	if (dr_quicknumprocesses <= 1) dr_quickparalleldrc = FALSE; else
	{
		dr_quickparalleldrc = TRUE;
		dr_quickmainthread = dr_quicknumprocesses - 1;
	}

	/* now do the DRC */
	initerrorlogging(_("DRC"));
	if (count == 0)
	{
		/* just do standard DRC here */
		if (!dr_quickparalleldrc) begintraversehierarchy();
		(void)dr_quickcheckthiscell(cell, 0, lx, hx, ly, hy, justarea);
		if (!dr_quickparalleldrc) endtraversehierarchy();
	} else
	{
		/* check only these "count" instances */
		dr_quickchecktheseinstances(cell, count, nodestocheck, validity);
	}

	/* sort the errors by layer */
	if (count <= 0)
	{
		sorterrors();
		errorcount = numerrors();
		t = endtimer();
		if (t > 60.0 && (us_useroptions&BEEPAFTERLONGJOB) != 0)
			ttybeep(SOUNDBEEP, TRUE);
		ttyputmsg(_("%ld errors found (took %s)"), errorcount, explainduration(t));
	}
	termerrorlogging(TRUE);

	/* deallocate temporary memory used in DRC */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			cp = (CHECKPROTO *)np->temp1;
			if (cp->hierinstancecount > 0)
			{
				/* free net number lists on every net in the cell */
				for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
				{
					netnumbers = (INTBIG*)net->temp1;
					efree((CHAR *)netnumbers);
				}
			}
			dr_quickfreecheckproto(cp);
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto->primindex != 0) continue;

				/* ignore documentation icons */
				if (isiconof(ni->proto, np)) continue;

				ci = (CHECKINST *)ni->temp1;
				dr_quickfreecheckinst(ci);
			}
		}
	}
}

/*
 * Routine to cleanup memory used by the quick-DRC.
 */
void dr_quickterm(void)
{
	REGISTER INTBIG i;
	REGISTER INSTINTER *dii;
	REGISTER CHECKPROTO *cp;
	REGISTER CHECKINST *ci;

	if (dr_quicklayerinternodehashsize > 0)
	{
		for(i=0; i<dr_quicklayerinternodehashsize; i++)
			if (dr_quicklayerinternodetable[i] != 0)
				efree((CHAR *)dr_quicklayerinternodetable[i]);
		efree((CHAR *)dr_quicklayerinternodetable);
		efree((CHAR *)dr_quicklayerinternodehash);
	}
	if (dr_quicklayerinterarchashsize > 0)
	{
		for(i=0; i<dr_quicklayerinterarchashsize; i++)
			if (dr_quicklayerinterarctable[i] != 0)
				efree((CHAR *)dr_quicklayerinterarctable[i]);
		efree((CHAR *)dr_quicklayerinterarctable);
		efree((CHAR *)dr_quicklayerinterarchash);
	}
	if (dr_layertotal > 0)
		efree((CHAR *)dr_layersvalid);
	dr_quickclearinstancecache();
	while (dr_quickinstinterfree != NOINSTINTER)
	{
		dii = dr_quickinstinterfree;
		dr_quickinstinterfree = (INSTINTER *)dii->cell1;
		efree((CHAR *)dii);
	}
	if (dr_quickinstintertotal > 0)
		efree((CHAR *)dr_quickinstinter);
	while (dr_quickcheckprotofree != NOCHECKPROTO)
	{
		cp = dr_quickcheckprotofree;
		dr_quickcheckprotofree = cp->nextcheckproto;
		efree((CHAR *)cp);
	}
	while (dr_quickcheckinstfree != NOCHECKINST)
	{
		ci = dr_quickcheckinstfree;
		dr_quickcheckinstfree = (CHECKINST *)ci->localindex;
		efree((CHAR *)ci);
	}

	/* free DRC exclusion areas */
	for(i=0; i<dr_quickexclusiontotal; i++)
		freepolygon(dr_quickexclusionlist[i].poly);
	if (dr_quickexclusiontotal > 0) efree((CHAR *)dr_quickexclusionlist);

	/* free per-processor state blocks */
	for(i=0; i<dr_quickstatecount; i++)
	{
		freepolylist(dr_quickstate[i]->cellinstpolylist);
		freepolylist(dr_quickstate[i]->nodeinstpolylist);
		freepolylist(dr_quickstate[i]->arcinstpolylist);
		freepolylist(dr_quickstate[i]->subpolylist);
		freepolylist(dr_quickstate[i]->cropnodepolylist);
		freepolylist(dr_quickstate[i]->croparcpolylist);
		freepolylist(dr_quickstate[i]->layerlookpolylist);
		freepolylist(dr_quickstate[i]->activecroppolylist);
		killhierarchicaltraversal(dr_quickstate[i]->hierarchybasesnapshot);
		killhierarchicaltraversal(dr_quickstate[i]->hierarchytempsnapshot);
		efree((CHAR *)dr_quickstate[i]);
	}
	if (dr_quickstatecount > 0) 
	{
		efree((CHAR *)dr_quickstate);
		efree((CHAR *)dr_quickprocessdone);
	}
}

/*************************** QUICK DRC CELL EXAMINATION ***************************/

/*
 * Routine to check the contents of cell "cell" with global network index "globalindex".
 * Returns positive if errors are found, zero if no errors are found, negative on internal error.
 */
INTBIG dr_quickcheckthiscell(NODEPROTO *cell, INTBIG globalindex, INTBIG lx, INTBIG hx,
	INTBIG ly, INTBIG hy, BOOLEAN justarea)
{
	REGISTER CHECKPROTO *cp;
	REGISTER CHECKINST *ci;
	REGISTER NODEPROTO *np;
	REGISTER BOOLEAN allsubcellsstillok;
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var;
	REGISTER INTBIG localindex, retval, errcount, localerrors, i;
	float elapsed;
	REGISTER UINTBIG lastgooddate, lastchangedate;

	/* first check all subcells */
	allsubcellsstillok = TRUE;
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		np = ni->proto;
		if (np->primindex != 0) continue;

		/* ignore documentation icons */
		if (isiconof(ni->proto, cell)) continue;

		/* ignore if not in the area */
		if (justarea)
		{
			if (ni->geom->lowx >= hx || ni->geom->highx <= lx ||
				ni->geom->lowy >= hy || ni->geom->highy <= ly) continue;
		}

		cp = (CHECKPROTO *)np->temp1;
		if (cp->cellchecked && !cp->cellparameterized) continue;

		/* recursively check the subcell */
		ci = (CHECKINST *)ni->temp1;
		localindex = globalindex * ci->multiplier + ci->localindex + ci->offset;
		if (!dr_quickparalleldrc) downhierarchy(ni, np, 0);
		retval = dr_quickcheckthiscell(np, localindex, 0, 0, 0, 0, FALSE);
		if (!dr_quickparalleldrc) uphierarchy();
		if (retval < 0) return(-1);
		if (retval > 0) allsubcellsstillok = FALSE;
	}

	/* prepare to check cell */
	cp = (CHECKPROTO *)cell->temp1;
	cp->cellchecked = TRUE;

	/* if the cell hasn't changed since the last good check, stop now */
	if (allsubcellsstillok)
	{
		var = getvalkey((INTBIG)cell, VNODEPROTO, VINTEGER, dr_lastgooddrckey);
		if (var != NOVARIABLE)
		{
			lastgooddate = (UINTBIG)var->addr;
			lastchangedate = cell->revisiondate;
			if (lastchangedate <= lastgooddate) return(0);
		}
	}

	/* announce progress */
	ttyputmsg(_("Checking cell %s"), describenodeproto(cell));

	/* remember how many errors there are on entry */
	errcount = numerrors();

	/* now look at every primitive node and arc here */
	dr_quickerrorsfound = 0;
	dr_quickparallelcellinst = cell->firstnodeinst;
	dr_quickparallelnodeinst = cell->firstnodeinst;
	dr_quickparallelarcinst = cell->firstarcinst;
	dr_quickjustarea = justarea;
	dr_quickarealx = lx;   dr_quickareahx = hx;
	dr_quickarealy = ly;   dr_quickareahy = hy;
	if (dr_quickparalleldrc && dr_quicknumprocesses > 1)
	{
		/* code cannot be called by multiple procesors: uses globals */
		NOT_REENTRANT;

		/* break up into multiple processes */
		setmultiprocesslocks(TRUE);

		/* startup extra threads */
		for(i=0; i<dr_quicknumprocesses; i++)
		{
			/* mark this process as "still running" */
			dr_quickstate[i]->globalindex = globalindex;
			emutexlock(dr_quickprocessdone[i]);
			if (i == dr_quickmainthread)
			{
				/* the last process is run locally, not in a new thread */
				(void)dr_quickdothread((void *)i);
			} else
			{
				/* create a thread to run this one */
				enewthread(dr_quickdothread, (void *)i);
			}
		}

		/* now wait for all processes to finish */
		for(i=0; i<dr_quicknumprocesses; i++)
		{
			emutexlock(dr_quickprocessdone[i]);
			emutexunlock(dr_quickprocessdone[i]);
		}
		setmultiprocesslocks(FALSE);
	} else
	{
		dr_quickstate[0]->globalindex = globalindex;
		(void)dr_quickdothread(0);
	}

	/* if there were no errors, remember that */
	elapsed = endtimer();
	localerrors = numerrors() - errcount;
	if (localerrors == 0)
	{
		(void)setvalkey((INTBIG)cell, VNODEPROTO, dr_lastgooddrckey,
			(INTBIG)getcurrenttime(), VINTEGER);
		ttyputmsg(_("   No errors found (%s so far)"), explainduration(elapsed));
	} else
	{
		ttyputmsg(_("   FOUND %ld ERRORS (%s so far)"), localerrors,
			explainduration(elapsed));
	}

	return(dr_quickerrorsfound);
}

/*
 * routine to check the design rules about nodeinst "ni".  Only check those
 * other objects whose geom pointers are greater than this one (i.e. only
 * check any pair of objects once).
 * Returns true if an error was found.
 */
BOOLEAN dr_quickchecknodeinst(NODEINST *ni, INTBIG globalindex, CHECKSTATE *state)
{
	REGISTER INTBIG tot, j, actual, minsize, fun;
	REGISTER NODEPROTO *cell;
	REGISTER BOOLEAN ret, errorsfound;
	XARRAY trans;
	CHAR *rule;
	INTBIG minx, miny, lx, hx, ly, hy;
	REGISTER INTBIG net;
	REGISTER POLYGON *poly;

	cell = ni->parent;
	makerot(ni, trans);

	/* get all of the polygons on this node */
	fun = nodefunction(ni);
	dr_quickgetnodeEpolys(ni, state->nodeinstpolylist, trans);
	tot = state->nodeinstpolylist->polylistcount;

	/* examine the polygons on this node */
	errorsfound = FALSE;
	for(j=0; j<tot; j++)
	{
		poly = state->nodeinstpolylist->polygons[j];
		if (poly->layer < 0) continue;

		/* determine network for this polygon */
		net = dr_quickgetnetnumber(poly->portproto, ni, globalindex);
		ret = dr_quickbadbox(poly, poly->layer, net, ni->proto->tech, ni->geom, trans, cell, globalindex, state);
		if (ret)
		{
			if ((dr_quickoptions&DRCFIRSTERROR) != 0) return(TRUE);
			errorsfound = TRUE;
		}
		ret = dr_quickcheckminwidth(ni->geom, poly->layer, poly, ni->proto->tech, state);
		if (ret)
		{
			if ((dr_quickoptions&DRCFIRSTERROR) != 0) return(TRUE);
			errorsfound = TRUE;
		}
		if (poly->tech == dr_curtech && !dr_layersvalid[poly->layer])
		{
			dr_quickreporterror(BADLAYERERROR, dr_curtech, 0, cell, 0, 0, 0,
				poly, ni->geom, poly->layer, NONET, NOPOLYGON, NOGEOM, 0, NONET);
			if ((dr_quickoptions&DRCFIRSTERROR) != 0) return(TRUE);
			errorsfound = TRUE;
		}

#ifdef SURROUNDRULES
		/* check surround if this layer is from a pure-layer node */
		if (fun == NPNODE)
		{
			count = drcsurroundrules(poly->tech, poly->layer, &surlayers, &surdist, &surrules);
			for(i=0; i<count; i++)
			{
				if (dr_quickfindsurround(poly, surlayers[i], surdist[i], cell, state)) continue;
				dr_quickreporterror(LAYERSURROUNDERROR, dr_curtech, 0, cell, surdist[i], 0,
					surrules[i], poly, ni->geom, poly->layer, NONET,
						NOPOLYGON, NOGEOM, surlayers[i], NONET);
				if ((dr_quickoptions&DRCFIRSTERROR) != 0) return(TRUE);
				errorsfound = TRUE;
			}
		}
#endif
	}

	/* check node for minimum size */
	drcminnodesize(ni->proto, cell->lib, &minx, &miny, &rule);
	if (minx > 0 && miny > 0)
	{
		if (ni->highx-ni->lowx < minx || ni->highy-ni->lowy < miny)
		{
			nodesizeoffset(ni, &lx, &ly, &hx, &hy);
			if (minx - (ni->highx-ni->lowx) > miny - (ni->highy-ni->lowy))
			{
				minsize = minx - lx - hx;
				actual = ni->highx - ni->lowx - lx - hx;
			} else
			{
				minsize = miny - ly - hy;
				actual = ni->highy - ni->lowy - ly - hy;
			}
			dr_quickreporterror(MINSIZEERROR, ni->proto->tech, 0, cell, minsize, actual, rule,
				NOPOLYGON, ni->geom, 0, NONET, NOPOLYGON, NOGEOM, 0, NONET);
		}
	}
	return(errorsfound);
}

/*
 * routine to check the design rules about arcinst "ai".
 * Returns true if errors were found.
 */
BOOLEAN dr_quickcheckarcinst(ARCINST *ai, INTBIG globalindex, CHECKSTATE *state)
{
	REGISTER INTBIG tot, j;
	REGISTER BOOLEAN ret, errorsfound;
	REGISTER INTBIG net;
	REGISTER POLYGON *poly;

	/* ignore arcs with no topology */
	if (ai->network == NONETWORK) return(FALSE);

	/* get all of the polygons on this arc */
	dr_quickgetarcpolys(ai, state->arcinstpolylist, el_matid);
	dr_quickcropactivearc(ai, state->arcinstpolylist, state);
	tot = state->arcinstpolylist->polylistcount;

	/* examine the polygons on this arc */
	errorsfound = FALSE;
	for(j=0; j<tot; j++)
	{
		poly = state->arcinstpolylist->polygons[j];
		if (poly->layer < 0) continue;
		net = ((INTBIG *)ai->network->temp1)[globalindex];
		ret = dr_quickbadbox(poly, poly->layer, net, ai->proto->tech, ai->geom, el_matid, ai->parent, globalindex, state);
		if (ret)
		{
			if ((dr_quickoptions&DRCFIRSTERROR) != 0) return(TRUE);
			errorsfound = TRUE;
		}
		ret = dr_quickcheckminwidth(ai->geom, poly->layer, poly, ai->proto->tech, state);
		if (ret)
		{
			if ((dr_quickoptions&DRCFIRSTERROR) != 0) return(TRUE);
			errorsfound = TRUE;
		}
		if (poly->tech == dr_curtech && !dr_layersvalid[poly->layer])
		{
			dr_quickreporterror(BADLAYERERROR, dr_curtech, 0, ai->parent, 0, 0, 0,
				poly, ai->geom, poly->layer, NONET, NOPOLYGON, NOGEOM, 0, NONET);
			if ((dr_quickoptions&DRCFIRSTERROR) != 0) return(TRUE);
			errorsfound = TRUE;
		}
	}
	return(errorsfound);
}

/*
 * routine to check the design rules about cell instance "ni".  Only check other
 * instances, and only check the parts of each that are within striking range.
 * Returns true if an error was found.
 */
BOOLEAN dr_quickcheckcellinst(NODEINST *ni, INTBIG globalindex, CHECKSTATE *state)
{
	REGISTER INTBIG lx, hx, ly, hy, search, localindex;
	INTBIG sublx, subhx, subly, subhy;
	REGISTER GEOM *geom;
	REGISTER NODEINST *oni;
	XARRAY rtrans, ttrans, downtrans, uptrans;
	REGISTER CHECKINST *ci;
	INSTINTER *dii;

	/* get current position in traversal hierarchy */
	gethierarchicaltraversal(state->hierarchybasesnapshot);

	/* look for other instances surrounding this one */
	lx = ni->geom->lowx - dr_quickinteractiondistance;
	hx = ni->geom->highx + dr_quickinteractiondistance;
	ly = ni->geom->lowy - dr_quickinteractiondistance;
	hy = ni->geom->highy + dr_quickinteractiondistance;
	search = initsearch(lx, hx, ly, hy, ni->parent);
	for(;;)
	{
		geom = nextobject(search);
		if (geom == NOGEOM) break;
		if (!geom->entryisnode) continue;
		oni = geom->entryaddr.ni;

		/* only check other nodes that are numerically higher (so each pair is only checked once) */
		if ((INTBIG)oni <= (INTBIG)ni) continue;
		if (oni->proto->primindex != 0) continue;

		/* see if this configuration of instances has already been done */
		if (dr_quickcheckinteraction(ni, oni, &dii)) continue;

		/* found other instance "oni", look for everything in "ni" that is near it */
		sublx = oni->geom->lowx - dr_quickinteractiondistance;
		subhx = oni->geom->highx + dr_quickinteractiondistance;
		subly = oni->geom->lowy - dr_quickinteractiondistance;
		subhy = oni->geom->highy + dr_quickinteractiondistance;
		makerotI(ni, rtrans);
		maketransI(ni, ttrans);
		transmult(rtrans, ttrans, downtrans);
		xformbox(&sublx, &subhx, &subly, &subhy, downtrans);

		maketrans(ni, ttrans);
		makerot(ni, rtrans);
		transmult(ttrans, rtrans, uptrans);

		ci = (CHECKINST *)ni->temp1;
		localindex = globalindex * ci->multiplier + ci->localindex + ci->offset;

		/* recursively search instance "ni" in the vicinity of "oni" */
		if (!dr_quickparalleldrc) downhierarchy(ni, ni->proto, 0);
		(void)dr_quickcheckcellinstcontents(sublx, subhx, subly, subhy, ni->proto, uptrans,
			localindex, oni, globalindex, state);
		if (!dr_quickparalleldrc) uphierarchy();
	}
	return(FALSE);
}

/*
 * Routine to recursively examine the area (lx-hx, ly-hy) in cell "np" with global index "globalindex".
 * The objects that are found are transformed by "uptrans" to be in the space of a top-level cell.
 * They are then compared with objects in "oni" (which is in that top-level cell),
 * which has global index "topglobalindex".
 */
BOOLEAN dr_quickcheckcellinstcontents(INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy, NODEPROTO *np,
	XARRAY uptrans, INTBIG globalindex, NODEINST *oni, INTBIG topglobalindex, CHECKSTATE *state)
{
	REGISTER INTBIG subsearch, tot, j, net, localindex;
	INTBIG sublx, subhx, subly, subhy;
	REGISTER GEOM *geom;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER BOOLEAN errorsfound, ret;
	XARRAY rtrans, ttrans, trans, subuptrans;
	REGISTER CHECKINST *ci;
	REGISTER POLYGON *poly;

	errorsfound = FALSE;
	subsearch = initsearch(lx, hx, ly, hy, np);
	for(;;)
	{
		geom = nextobject(subsearch);
		if (geom == NOGEOM) break;
		if (geom->entryisnode)
		{
			ni = geom->entryaddr.ni;
			if (ni->proto->primindex == 0)
			{
				sublx = lx;   subhx = hx;
				subly = ly;   subhy = hy;
				makerotI(ni, rtrans);
				maketransI(ni, ttrans);
				transmult(rtrans, ttrans, trans);
				xformbox(&sublx, &subhx, &subly, &subhy, trans);

				maketrans(ni, ttrans);
				makerot(ni, rtrans);
				transmult(ttrans, rtrans, trans);
				transmult(trans, uptrans, subuptrans);

				ci = (CHECKINST *)ni->temp1;
				localindex = globalindex * ci->multiplier + ci->localindex + ci->offset;

				if (!dr_quickparalleldrc) downhierarchy(ni, ni->proto, 0);
				dr_quickcheckcellinstcontents(sublx, subhx, subly, subhy, ni->proto,
					subuptrans, localindex, oni, topglobalindex, state);
				if (!dr_quickparalleldrc) uphierarchy();
			} else
			{
				makerot(ni, rtrans);
				transmult(rtrans, uptrans, trans);
				dr_quickgetnodeEpolys(ni, state->cellinstpolylist, trans);
				tot = state->cellinstpolylist->polylistcount;
				for(j=0; j<tot; j++)
				{
					poly = state->cellinstpolylist->polygons[j];
					if (poly->layer < 0) continue;

					/* determine network for this polygon */
					net = dr_quickgetnetnumber(poly->portproto, ni, globalindex);
					ret = dr_quickbadsubbox(poly, poly->layer, net, ni->proto->tech, ni->geom, trans,
						globalindex, np, oni, topglobalindex, state);
					if (ret)
					{
						if ((dr_quickoptions&DRCFIRSTERROR) != 0)
						{
							termsearch(subsearch);
							return(TRUE);
						}
						errorsfound = TRUE;
					}
				}
			}
		} else
		{
			ai = geom->entryaddr.ai;
			dr_quickgetarcpolys(ai, state->cellinstpolylist, uptrans);
			dr_quickcropactivearc(ai, state->cellinstpolylist, state);
			tot = state->cellinstpolylist->polylistcount;
			for(j=0; j<tot; j++)
			{
				poly = state->cellinstpolylist->polygons[j];
				if (poly->layer < 0) continue;
				if (ai->network == NONETWORK) net = NONET; else
					net = ((INTBIG *)ai->network->temp1)[globalindex];
				ret = dr_quickbadsubbox(poly, poly->layer, net, ai->proto->tech, ai->geom, uptrans,
					globalindex, np, oni, topglobalindex, state);
				if (ret)
				{
					if ((dr_quickoptions&DRCFIRSTERROR) != 0)
					{
						termsearch(subsearch);
						return(TRUE);
					}
					errorsfound = TRUE;
				}
			}
		}
	}
	return(errorsfound);
}

/*
 * Routine to examine polygon "poly" layer "layer" network "net" technology "tech" geometry "geom"
 * which is in cell "cell" and has global index "globalindex".
 * The polygon is compared against things inside node "oni", and that node's parent has global index "topglobalindex". 
 */
BOOLEAN dr_quickbadsubbox(POLYGON *poly, INTBIG layer, INTBIG net, TECHNOLOGY *tech, GEOM *geom, XARRAY trans,
	INTBIG globalindex, NODEPROTO *cell, NODEINST *oni, INTBIG topglobalindex, CHECKSTATE *state)
{
	REGISTER BOOLEAN basemulti, retval;
	REGISTER INTBIG minsize, bound, lxbound, hxbound, lybound, hybound, localindex;
	INTBIG lx, hx, ly, hy;
	XARRAY rtrans, ttrans, downtrans, uptrans;
	REGISTER CHECKINST *ci;

	/* see how far around the box it is necessary to search */
	bound = maxdrcsurround(tech, cell->lib, layer);
	if (bound < 0) return(FALSE);

	/* get bounds */
	getbbox(poly, &lx, &hx, &ly, &hy);
	makerotI(oni, rtrans);
	maketransI(oni, ttrans);
	transmult(rtrans, ttrans, downtrans);
	xformbox(&lx, &hx, &ly, &hy, downtrans);
	minsize = polyminsize(poly);

	maketrans(oni, ttrans);
	makerot(oni, rtrans);
	transmult(ttrans, rtrans, uptrans);

	ci = (CHECKINST *)oni->temp1;
	localindex = topglobalindex * ci->multiplier + ci->localindex + ci->offset;

	/* determine if original object has multiple contact cuts */
	if (geom->entryisnode) basemulti = dr_quickismulticut(geom->entryaddr.ni); else
		basemulti = FALSE;

	/* remember the current position in the hierarchy traversal and set the base one */
	gethierarchicaltraversal(state->hierarchytempsnapshot);
	sethierarchicaltraversal(state->hierarchybasesnapshot);

	/* search in the area surrounding the box */
	lxbound = lx-bound;   hxbound = hx+bound;
	lybound = ly-bound;   hybound = hy+bound;
	retval = dr_quickbadboxinarea(poly, layer, tech, net, geom, trans, globalindex,
		lxbound, hxbound, lybound, hybound, oni->proto, localindex,
		oni->parent, topglobalindex, uptrans, minsize, basemulti, state, FALSE);

	/* restore the proper hierarchy traversal position */
	sethierarchicaltraversal(state->hierarchytempsnapshot);
	return(retval);
}

/*
 * Routine to examine a polygon to see if it has any errors with its surrounding area.
 * The polygon is "poly" on layer "layer" on network "net" from technology "tech" from object "geom".
 * Checking looks in cell "cell" global index "globalindex".
 * Object "geom" can be transformed to the space of this cell with "trans".
 * Returns TRUE if a spacing error is found relative to anything surrounding it at or below
 * this hierarchical level.
 */
BOOLEAN dr_quickbadbox(POLYGON *poly, INTBIG layer, INTBIG net, TECHNOLOGY *tech, GEOM *geom,
	XARRAY trans, NODEPROTO *cell, INTBIG globalindex, CHECKSTATE *state)
{
	REGISTER BOOLEAN basemulti;
	REGISTER INTBIG minsize, bound, lxbound, hxbound, lybound, hybound;
	INTBIG lx, hx, ly, hy;

	/* see how far around the box it is necessary to search */
	bound = maxdrcsurround(tech, cell->lib, layer);
	if (bound < 0) return(FALSE);

	/* get bounds */
	getbbox(poly, &lx, &hx, &ly, &hy);
	minsize = polyminsize(poly);

	/* determine if original object has multiple contact cuts */
	if (geom->entryisnode) basemulti = dr_quickismulticut(geom->entryaddr.ni); else
		basemulti = FALSE;

	/* search in the area surrounding the box */
	lxbound = lx-bound;   hxbound = hx+bound;
	lybound = ly-bound;   hybound = hy+bound;
	return(dr_quickbadboxinarea(poly, layer, tech, net, geom, trans, globalindex,
		lxbound, hxbound, lybound, hybound, cell, globalindex,
		cell, globalindex, el_matid, minsize, basemulti, state, TRUE));
}

/*
 * Routine to recursively examine a polygon to see if it has any errors with its surrounding area.
 * The polygon is "poly" on layer "layer" from technology "tech" on network "net" from object "geom"
 * which is associated with global index "globalindex".
 * Checking looks in the area (lxbound-hxbound, lybound-hybound) in cell "cell" global index "cellglobalindex".
 * The polygon coordinates are in the space of cell "topcell", global index "topglobalindex",
 * and objects in "cell" can be transformed by "toptrans" to get to this space.
 * The base object, in "geom" can be transformed by "trans" to get to this space.
 * The minimum size of this polygon is "minsize" and "basemulti" is TRUE if it comes from a multicut contact.
 * If the two objects are in the same cell instance (nonhierarchical DRC), then "sameinstance" is TRUE.
 * If they are from different instances, then "sameinstance" is FALSE.
 *
 * Returns TRUE if errors are found.
 */
BOOLEAN dr_quickbadboxinarea(POLYGON *poly, INTBIG layer, TECHNOLOGY *tech, INTBIG net, GEOM *geom, XARRAY trans,
	INTBIG globalindex,
	INTBIG lxbound, INTBIG hxbound, INTBIG lybound, INTBIG hybound, NODEPROTO *cell, INTBIG cellglobalindex,
	NODEPROTO *topcell, INTBIG topglobalindex, XARRAY toptrans, INTBIG minsize, BOOLEAN basemulti,
	CHECKSTATE *state, BOOLEAN sameinstance)
{
	REGISTER GEOM *ngeom;
	REGISTER NODEPROTO *np;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni;
	REGISTER ARCPROTO *ap;
	REGISTER BOOLEAN touch, ret, multi;
	REGISTER INTBIG nnet, search, dist, j, tot, nminsize,
		localindex, count;
	REGISTER POLYGON *npoly;
	XARRAY rtrans, ttrans, subtrans, temptrans;
	CHAR *rule;
	REGISTER BOOLEAN con;
	INTBIG edge, slx, shx, sly, shy, rlxbound, rhxbound, rlybound, rhybound;
	REGISTER CHECKINST *ci;

	rlxbound = lxbound;   rhxbound = hxbound;
	rlybound = lybound;   rhybound = hybound;
	xformbox(&rlxbound, &rhxbound, &rlybound, &rhybound, toptrans);

	search = initsearch(lxbound, hxbound, lybound, hybound, cell);
	count = 0;
	for(;;)
	{
		ngeom = nextobject(search);
		if (ngeom == NOGEOM) break;
		if (sameinstance && (ngeom == geom)) continue;
		if (ngeom->entryisnode)
		{
			ni = ngeom->entryaddr.ni;
			np = ni->proto;

			/* ignore nodes that are not primitive */
			if (np->primindex == 0)
			{
				/* instance found: look inside it for offending geometry */
				makerotI(ni, rtrans);
				maketransI(ni, ttrans);
				transmult(rtrans, ttrans, temptrans);
				slx = lxbound;   shx = hxbound;
				sly = lybound;   shy = hybound;
				xformbox(&slx, &shx, &sly, &shy, temptrans);

				ci = (CHECKINST *)ni->temp1;
				localindex = cellglobalindex * ci->multiplier + ci->localindex + ci->offset;

				maketrans(ni, ttrans);
				makerot(ni, rtrans);
				transmult(ttrans, rtrans, temptrans);
				transmult(temptrans, toptrans, subtrans);

				/* compute localindex */
				if (!dr_quickparalleldrc) downhierarchy(ni, np, 0);
				dr_quickbadboxinarea(poly, layer, tech, net, geom, trans, globalindex,
					slx, shx, sly, shy, np, localindex,
					topcell, topglobalindex, subtrans, minsize, basemulti, state, sameinstance);
				if (!dr_quickparalleldrc) uphierarchy();
			} else
			{
				/* don't check between technologies */
				if (np->tech != tech) continue;

				/* see if this type of node can interact with this layer */
				if (!dr_quickchecklayerwithnode(layer, np)) continue;

				/* see if the objects directly touch */
				touch = dr_quickobjtouch(ngeom, geom);

				/* prepare to examine every layer in this nodeinst */
				makerot(ni, rtrans);
				transmult(rtrans, toptrans, subtrans);

				/* get the shape of each nodeinst layer */
				dr_quickgetnodeEpolys(ni, state->subpolylist, subtrans);
				tot = state->subpolylist->polylistcount;
				multi = basemulti;
				if (!multi) multi = dr_quickismulticut(ni);
				for(j=0; j<tot; j++)
				{
					npoly = state->subpolylist->polygons[j];

					/* can't do this because "lxbound..." is local but the poly bounds are global */
					if (state->subpolylist->lx[j] > rhxbound ||
						state->subpolylist->hx[j] < rlxbound ||
						state->subpolylist->ly[j] > rhybound ||
						state->subpolylist->hy[j] < rlybound) continue;
					if (npoly->layer < 0) continue;

					/* determine network for this polygon */
					nnet = dr_quickgetnetnumber(npoly->portproto, ni, cellglobalindex);

					/* see whether the two objects are electrically connected */
					if (nnet != NONET && nnet == net) con = TRUE; else con = FALSE;

					/* if they connect electrically and adjoin, don't check */
					if (con && touch) continue;

					nminsize = polyminsize(npoly);
					dist = dr_adjustedmindist(tech, cell->lib, layer, minsize,
						npoly->layer, nminsize, con, multi, &edge, &rule);
					if (dist < 0) continue;

					/* check the distance */
					ret = dr_quickcheckdist(tech, topcell, topglobalindex,
						poly, layer, net, geom, trans, globalindex,
						npoly, npoly->layer, nnet, ngeom, subtrans, cellglobalindex,
						con, dist, edge, rule, state);
#ifndef ALLERRORS
					if (ret)
					{
						termsearch(search);
						return(TRUE);
					}
#endif
				}
			}
		} else
		{
			ai = ngeom->entryaddr.ai;   ap = ai->proto;

			/* don't check between technologies */
			if (ap->tech != tech) continue;

			/* see if this type of arc can interact with this layer */
			if (!dr_quickchecklayerwitharc(layer, ap)) continue;

			/* see if the objects directly touch */
			touch = dr_quickobjtouch(ngeom, geom);

			/* see whether the two objects are electrically connected */
			nnet = ((INTBIG *)ai->network->temp1)[cellglobalindex];
			if (net != NONET && nnet == net) con = TRUE; else con = FALSE;

			/* if they connect electrically and adjoin, don't check */
			if (con && touch) continue;

			/* get the shape of each arcinst layer */
			dr_quickgetarcpolys(ai, state->subpolylist, toptrans);
			dr_quickcropactivearc(ai, state->subpolylist, state);
			tot = state->subpolylist->polylistcount;
			multi = basemulti;
			for(j=0; j<tot; j++)
			{
				npoly = state->subpolylist->polygons[j];

				/* can't do this because "lxbound..." is local but the poly bounds are global */
				if (state->subpolylist->lx[j] > rhxbound ||
					state->subpolylist->hx[j] < rlxbound ||
					state->subpolylist->ly[j] > rhybound ||
					state->subpolylist->hy[j] < rlybound) continue;
				if (npoly->layer < 0) continue;

				/* see how close they can get */
				nminsize = polyminsize(npoly);
				dist = dr_adjustedmindist(tech, cell->lib, layer,
					minsize, npoly->layer, nminsize, con, multi, &edge, &rule);
				if (dist < 0) continue;

				/* check the distance */
				ret = dr_quickcheckdist(tech, topcell, topglobalindex,
					poly, layer, net, geom, trans, globalindex,
					npoly, npoly->layer, nnet, ngeom, toptrans, cellglobalindex,
					con, dist, edge, rule, state);
#ifndef ALLERRORS
				if (ret)
				{
					termsearch(search);
					return(TRUE);
				}
#endif
			}
		}
	}
	return(FALSE);
}

/* #define DEBUGDRC 1 */

/*
 * Routine to compare:
 *    polygon "poly1" layer "layer1" network "net1" object "geom1"
 * with:
 *    polygon "poly2" layer "layer2" network "net2" object "geom2"
 * The polygons are both in technology "tech" and are in the space of cell "cell"
 * which has global index "globalindex".
 * Note that to transform object "geom1" to this space, use "trans1" and to transform
 * object "geom2" to this space, use "trans2".
 * They are connected if "con" is nonzero.
 * They cannot be less than "dist" apart (if "edge" is nonzero, check edges only)
 * and the rule for this is "rule".
 *
 * Returns TRUE if an error has been found.
 */
BOOLEAN dr_quickcheckdist(TECHNOLOGY *tech, NODEPROTO *cell, INTBIG globalindex,
	POLYGON *poly1, INTBIG layer1, INTBIG net1, GEOM *geom1, XARRAY trans1, INTBIG globalindex1,
	POLYGON *poly2, INTBIG layer2, INTBIG net2, GEOM *geom2, XARRAY trans2, INTBIG globalindex2,
	BOOLEAN con, INTBIG dist, INTBIG edge, CHAR *rule, CHECKSTATE *state)
{
	REGISTER BOOLEAN isbox1, isbox2, needboth, maytouch;
	INTBIG lx1, hx1, ly1, hy1, lx2, hx2, ly2, hy2, xf1, yf1, xf2, yf2;
	CHAR *msg, *sizerule;
	void *infstr;
	REGISTER POLYGON *origpoly1, *origpoly2;
	REGISTER INTBIG pdx, pdy, pd, pdedge, fun, errtype, minwidth,
		lxb, hxb, lyb, hyb, actual, intervening;
#ifdef DEBUGDRC
	REGISTER BOOLEAN debug;
#endif
#ifdef ALLERRORS
	REGISTER BOOLEAN returnflag;
	returnflag = FALSE;
#endif

	/* turn off flag that the nodeinst may be undersized */
	state->tinynodeinst = NONODEINST;

	origpoly1 = poly1;
	origpoly2 = poly2;
	isbox1 = isbox(poly1, &lx1, &hx1, &ly1, &hy1);
	if (!isbox1) getbbox(poly1, &lx1, &hx1, &ly1, &hy1);
	isbox2 = isbox(poly2, &lx2, &hx2, &ly2, &hy2);
	if (!isbox2) getbbox(poly2, &lx2, &hx2, &ly2, &hy2);
#ifdef DEBUGDRC
debug=FALSE;
if (layer1 == 9 && layer2 == 9) debug = TRUE;
if (layer2 == 9 && layer1 == 9) debug = TRUE;
if (debug)
{
	CHAR *layername1, *layername2;
	void *infstr; CHAR *pt;

	layername1 = layername(tech, layer1);
	layername2 = layername(tech, layer2);
	ttyputmsg("Comparing layer %s on %s is %s<=X<=%s and %s<=Y<=%s", layername1, geomname(geom1), latoa(lx1,0), latoa(hx1,0), latoa(ly1,0), latoa(hy1,0));
	ttyputmsg("     with layer %s on %s is %s<=X<=%s and %s<=Y<=%s", layername2, geomname(geom2), latoa(lx2,0), latoa(hx2,0), latoa(ly2,0), latoa(hy2,0));
	asktool(us_tool, x_("clear"));
	asktool(us_tool, x_("show-area"), lx1, hx1, ly1, hy1, (INTBIG)cell);
	asktool(us_tool, x_("show-area"), lx2, hx2, ly2, hy2, (INTBIG)cell);
	asktool(us_tool, x_("show-line"), lx2, ly2, hx2, hy2, (INTBIG)cell);
	asktool(us_tool, x_("show-line"), lx2, hy2, hx2, ly2, (INTBIG)cell);
	infstr = initinfstr();
	formatinfstr(infstr, x_("Layers %s and %s:"), layername1, layername2);
	pt = ttygetline(returninfstr(infstr));
	if (pt == 0 || *pt == 'q') debug = FALSE;
}
#endif

	/*
	 * special rule for allowing touching:
	 *   the layers are the same and either:
	 *     they connect and are *NOT* contact layers
	 *   or:
	 *     they don't connect and are implant layers (substrate/well)
	 */
	maytouch = FALSE;
	if (samelayer(tech, layer1, layer2))
	{
		fun = layerfunction(tech, layer1) & LFTYPE;
		if (con)
		{
			if (!layeriscontact(fun)) maytouch = TRUE;
		} else
		{
			if (fun == LFSUBSTRATE || fun == LFWELL || fun == LFIMPLANT) maytouch = TRUE;
		}
	}

	/* special code if both polygons are manhattan */
	if (isbox1 && isbox2)
	{
		/* manhattan */
		pdx = maxi(lx2-hx1, lx1-hx2);
		pdy = maxi(ly2-hy1, ly1-hy2);
		if (pdx == 0 && pdy == 0) pd = 1; else
			pd = maxi(pdx, pdy);
		if (maytouch)
		{
			/* they are electrically connected: see if they touch */
			if (pd <= 0)
			{
				/* they are electrically connected and they touch: look for minimum size errors */
				minwidth = drcminwidth(tech, cell->lib, layer1, &sizerule);
				lxb = maxi(lx1, lx2);
				hxb = mini(hx1, hx2);
				lyb = maxi(ly1, ly2);
				hyb = mini(hy1, hy2);
				actual = computedistance(lxb, lyb, hxb, hyb);
				if (actual != 0 && actual < minwidth)
				{
					if (hxb-lxb > hyb-lyb)
					{
						/* horizontal abutment: check for minimum width */
						if (!dr_quicklookforpoints(lxb-1, lyb-1, lxb-1, hyb+1, layer1, cell, TRUE, state) &&
								!dr_quicklookforpoints(hxb+1, lyb-1, hxb+1, hyb+1, layer1, cell, TRUE, state))
						{
							lyb -= minwidth/2;   hyb += minwidth/2;
							state->checkdistpoly1rebuild->xv[0] = lxb;
							state->checkdistpoly1rebuild->yv[0] = lyb;
							state->checkdistpoly1rebuild->xv[1] = hxb;
							state->checkdistpoly1rebuild->yv[1] = hyb;
							state->checkdistpoly1rebuild->count = 2;
							state->checkdistpoly1rebuild->style = FILLEDRECT;
							dr_quickreporterror(MINWIDTHERROR, tech, 0, cell, minwidth,
								actual, sizerule, state->checkdistpoly1rebuild,
									geom1, layer1, NONET, NOPOLYGON, NOGEOM, 0, NONET);
#ifdef ALLERRORS
							returnflag = TRUE;
#else
							return(TRUE);
#endif
						}
					} else
					{
						/* vertical abutment: check for minimum width */
						if (!dr_quicklookforpoints(lxb-1, lyb-1, hxb+1, lyb-1, layer1, cell, TRUE, state) &&
								!dr_quicklookforpoints(lxb-1, hyb+1, hxb+1, hyb+1, layer1, cell, TRUE, state))
						{
							lxb -= minwidth/2;   hxb += minwidth/2;
							state->checkdistpoly1rebuild->xv[0] = lxb;
							state->checkdistpoly1rebuild->yv[0] = lyb;
							state->checkdistpoly1rebuild->xv[1] = hxb;
							state->checkdistpoly1rebuild->yv[1] = hyb;
							state->checkdistpoly1rebuild->count = 2;
							state->checkdistpoly1rebuild->style = FILLEDRECT;
							dr_quickreporterror(MINWIDTHERROR, tech, 0, cell, minwidth,
								actual, sizerule, state->checkdistpoly1rebuild,
									geom1, layer1, NONET, NOPOLYGON, NOGEOM, 0, NONET);
#ifdef ALLERRORS
							returnflag = TRUE;
#else
							return(TRUE);
#endif
						}
					}
				}
			}
		}

		/* crop out parts of any arc that is covered by an adjoining node */
		if (geom1->entryisnode)
		{
			if (dr_quickcropnodeinst(geom1->entryaddr.ni, globalindex1, state, trans1,
				lx1, hx1, ly1, hy1, layer2, net2, geom2, &lx2, &hx2, &ly2, &hy2))
			{
#ifndef ALLERRORS
				return(FALSE);
#endif
			}
		} else
		{
			if (dr_quickcroparcinst(geom1->entryaddr.ai, layer1, trans1, &lx1, &hx1, &ly1, &hy1, state))
			{
#ifndef ALLERRORS
				return(FALSE);
#endif
			}
		}
		if (geom2->entryisnode)
		{
			if (dr_quickcropnodeinst(geom2->entryaddr.ni, globalindex2, state, trans2,
				lx2, hx2, ly2, hy2, layer1, net1, geom1, &lx1, &hx1, &ly1, &hy1))
			{
#ifndef ALLERRORS
				return(FALSE);
#endif
			}
		} else
		{
			if (dr_quickcroparcinst(geom2->entryaddr.ai, layer2, trans2, &lx2, &hx2, &ly2, &hy2, state))
			{
#ifndef ALLERRORS
				return(FALSE);
#endif
			}
		}
#ifdef DEBUGDRC
if (debug)
{
	void *infstr; CHAR *pt;

	asktool(us_tool, x_("clear"));
	asktool(us_tool, x_("show-area"), lx1, hx1, ly1, hy1, (INTBIG)cell);
	asktool(us_tool, x_("show-area"), lx2, hx2, ly2, hy2, (INTBIG)cell);
	asktool(us_tool, x_("show-line"), lx2, ly2, hx2, hy2, (INTBIG)cell);
	asktool(us_tool, x_("show-line"), lx2, hy2, hx2, ly2, (INTBIG)cell);
	infstr = initinfstr();
	formatinfstr(infstr, x_("Cropped:"));
	pt = ttygetline(returninfstr(infstr));
	if (pt == 0 || *pt == 'q') debug = FALSE;
}
#endif

		makerectpoly(lx1, hx1, ly1, hy1, state->checkdistpoly1rebuild);
		state->checkdistpoly1rebuild->style = FILLED;
		makerectpoly(lx2, hx2, ly2, hy2, state->checkdistpoly2rebuild);
		state->checkdistpoly2rebuild->style = FILLED;
		poly1 = state->checkdistpoly1rebuild;
		poly2 = state->checkdistpoly2rebuild;

		/* compute the distance */
		if (edge != 0)
		{
			/* calculate the spacing between the box edges */
			pdedge = mini(
				mini(mini(abs(lx1-lx2), abs(lx1-hx2)), mini(abs(hx1-lx2), abs(hx1-hx2))),
				mini(mini(abs(ly1-ly2), abs(ly1-hy2)), mini(abs(hy1-ly2), abs(hy1-hy2))));
			pd = maxi(pd, pdedge);
		} else
		{
			pdx = maxi(lx2-hx1, lx1-hx2);
			pdy = maxi(ly2-hy1, ly1-hy2);
			if (pdx == 0 && pdy == 0) pd = 1; else
			{
				pd = maxi(pdx, pdy);
				if (pd < dist && pd > 0) pd = polyseparation(poly1, poly2);
			}
		}
	} else
	{
		/* nonmanhattan */
		switch (poly1->style)
		{
			case FILLEDRECT:
			case CLOSEDRECT:
				maketruerect(poly1);
				break;
			case FILLED:
			case CLOSED:
			case CROSSED:
			case OPENED:
			case OPENEDT1:
			case OPENEDT2:
			case OPENEDT3:
			case VECTORS:
				break;
			default:
				return(FALSE);
		}

		switch (poly2->style)
		{
			case FILLEDRECT:
			case CLOSEDRECT:
				maketruerect(poly2);
				break;
			case FILLED:
			case CLOSED:
			case CROSSED:
			case OPENED:
			case OPENEDT1:
			case OPENEDT2:
			case OPENEDT3:
			case VECTORS:
				break;
			default:
				return(FALSE);
		}

		/* make sure polygons don't intersect */
		if (polyintersect(poly1, poly2)) pd = 0; else
		{
			/* find distance between polygons */
			pd = polyseparation(poly1, poly2);
		}
	}

	/* see if the design rule is met */
	if (pd >= dist)
	{ 
#ifndef ALLERRORS
		return(FALSE);
#endif
	}
	errtype = SPACINGERROR;
#ifdef DEBUGDRC
if (debug) ttyputmsg("distance=%s but limit=%s", latoa(pd,0), latoa(dist,0));
#endif

	/*
	 * special case: ignore errors between two active layers connected
	 * to either side of a field-effect transistor that is inside of
	 * the error area.
	 */
	if (dr_quickactiveontransistor(poly1, layer1, net1,
		poly2, layer2, net2, tech, cell, globalindex))
	{
#ifndef ALLERRORS
		return(FALSE);
#endif
	}
#ifdef DEBUGDRC
if (debug) ttyputmsg("Active-on-transistor rule didn't help");
#endif

	/* special cases if the layers are the same */
	if (samelayer(tech, layer1, layer2))
	{
		/* special case: check for "notch" */
		if (maytouch)
		{
			/* if they touch, it is acceptable */
			if (pd <= 0) 
			{
#ifndef ALLERRORS
				return(FALSE);
#endif
			}

			/* see if the notch is filled */
			intervening = dr_quickfindinterveningpoints(poly1, poly2, &xf1, &yf1, &xf2, &yf2);
			if (intervening == 0)
			{
#ifndef ALLERRORS
				return(FALSE);
#endif
			}
			if (intervening == 1) needboth = FALSE; else
				needboth = TRUE;
			if (dr_quicklookforpoints(xf1, yf1, xf2, yf2, layer1, cell, needboth, state))
			{
#ifndef ALLERRORS
				return(FALSE);
#endif
			}

			/* look further if on the same net and diagonally separate (1 intervening point) */
			if (net1 == net2 && intervening == 1)
			{
#ifndef ALLERRORS
				return(FALSE);
#endif
			}
			errtype = NOTCHERROR;
		}
	}
#ifdef DEBUGDRC
if (debug) ttygetline("Touching rules didn't help.  THIS IS AN ERROR");
#endif

	msg = 0;
	if (state->tinynodeinst != NONODEINST)
	{
		/* see if the node/arc that failed was involved in the error */
		if ((state->tinynodeinst->geom == geom1 || state->tinynodeinst->geom == geom2) &&
			(state->tinyobj == geom1 || state->tinyobj == geom2))
		{
			infstr = initinfstr();
			if (dr_quickparalleldrc)
				emutexlock(dr_quickmutexio);	/* BEGIN critical section */
			formatinfstr(infstr, _("%s is too small for the %s"),
				describenodeinst(state->tinynodeinst), geomname(state->tinyobj));
			if (dr_quickparalleldrc)
				emutexunlock(dr_quickmutexio);	/* END critical section */
			(void)allocstring(&msg, returninfstr(infstr), dr_tool->cluster);
		}
	}

	dr_quickreporterror(errtype, tech, msg, cell, dist, pd, rule,
		origpoly1, geom1, layer1, net1,
		origpoly2, geom2, layer2, net2);
#ifdef ALLERRORS
	return(returnflag);
#else
	return(TRUE);
#endif
}

/*************************** QUICK DRC SEE IF INSTANCES CAUSE ERRORS ***************************/

/*
 * Routine to examine, in cell "cell", the "count" instances in "nodestocheck".
 * If they are DRC clean, set the associated entry in "validity" to TRUE.
 */
void dr_quickchecktheseinstances(NODEPROTO *cell, INTBIG count, NODEINST **nodestocheck, BOOLEAN *validity)
{
	REGISTER INTBIG lx, hx, ly, hy, search, globalindex, localindex, i, j;
	INTBIG sublx, subhx, subly, subhy;
	REGISTER GEOM *geom;
	REGISTER NODEINST *ni, *oni;
	XARRAY rtrans, ttrans, downtrans, uptrans;
	REGISTER CHECKINST *ci;
	REGISTER CHECKSTATE *state;

	globalindex = 0;
	state = dr_quickstate[0];
	state->globalindex = globalindex;

	/* loop through all of the instances to be checked */
	for(i=0; i<count; i++)
	{
		ni = nodestocheck[i];
		validity[i] = TRUE;

		/* look for other instances surrounding this one */
		lx = ni->geom->lowx - dr_quickinteractiondistance;
		hx = ni->geom->highx + dr_quickinteractiondistance;
		ly = ni->geom->lowy - dr_quickinteractiondistance;
		hy = ni->geom->highy + dr_quickinteractiondistance;
		search = initsearch(lx, hx, ly, hy, ni->parent);
		for(;;)
		{
			geom = nextobject(search);
			if (geom == NOGEOM) break;
			if (!geom->entryisnode)
			{
				if (dr_quickcheckgeomagainstinstance(geom, ni, state))
				{
					validity[i] = FALSE;
					termsearch(search);
					break;
				}
				continue;
			}
			oni = geom->entryaddr.ni;
			if (oni->proto->primindex != 0)
			{
				/* found a primitive node: check it against the instance contents */
				if (dr_quickcheckgeomagainstinstance(geom, ni, state))
				{
					validity[i] = FALSE;
					termsearch(search);
					break;
				}
				continue;
			}

			/* ignore if it is one of the instances in the list */
			for(j=0; j<count; j++)
				if (oni == nodestocheck[j]) break;
			if (j < count) continue;

			/* found other instance "oni", look for everything in "ni" that is near it */
			sublx = oni->geom->lowx - dr_quickinteractiondistance;
			subhx = oni->geom->highx + dr_quickinteractiondistance;
			subly = oni->geom->lowy - dr_quickinteractiondistance;
			subhy = oni->geom->highy + dr_quickinteractiondistance;
			makerotI(ni, rtrans);
			maketransI(ni, ttrans);
			transmult(rtrans, ttrans, downtrans);
			xformbox(&sublx, &subhx, &subly, &subhy, downtrans);

			maketrans(ni, ttrans);
			makerot(ni, rtrans);
			transmult(ttrans, rtrans, uptrans);

			ci = (CHECKINST *)ni->temp1;
			localindex = globalindex * ci->multiplier + ci->localindex + ci->offset;

			/* recursively search instance "ni" in the vicinity of "oni" */
			if (dr_quickcheckcellinstcontents(sublx, subhx, subly, subhy, ni->proto, uptrans,
				localindex, oni, globalindex, dr_quickstate[0]))
			{
				/* errors were found: bail */
				validity[i] = FALSE;
				termsearch(search);
				break;
			}
		}
	}
}

/*
 * Routine to check primitive object "geom" (an arcinst or primitive nodeinst) against cell instance "ni".
 * Returns TRUE if there are design-rule violations in their interaction.
 */
BOOLEAN dr_quickcheckgeomagainstinstance(GEOM *geom, NODEINST *ni, CHECKSTATE *state)
{
	REGISTER NODEPROTO *np, *subcell;
	REGISTER ARCINST *oai;
	REGISTER NODEINST *oni;
	REGISTER TECHNOLOGY *tech;
	REGISTER BOOLEAN basemulti;
	REGISTER INTBIG tot, j, bound, net, minsize, localindex, globalindex;
	INTBIG lx, hx, ly, hy, slx, shx, sly, shy;
	XARRAY rtrans, ttrans, temptrans, subtrans, trans;
	REGISTER POLYGON *poly;
	REGISTER CHECKINST *ci;

	np = ni->proto;
	globalindex = 0;
	subcell = geomparent(geom);
	if (geom->entryisnode)
	{
		/* get all of the polygons on this node */
		oni = geom->entryaddr.ni;
		makerot(oni, trans);
		dr_quickgetnodeEpolys(oni, state->nodeinstpolylist, trans);
		basemulti = dr_quickismulticut(oni);
		tech = oni->proto->tech;
	} else
	{
		oai = geom->entryaddr.ai;
		transid(trans);
		dr_quickgetarcpolys(oai, state->nodeinstpolylist, trans);
		basemulti = FALSE;
		tech = oai->proto->tech;
	}
	tot = state->nodeinstpolylist->polylistcount;

	ci = (CHECKINST *)ni->temp1;
	localindex = globalindex * ci->multiplier + ci->localindex + ci->offset;

	/* examine the polygons on this node */
	for(j=0; j<tot; j++)
	{
		poly = state->nodeinstpolylist->polygons[j];
		if (poly->layer < 0) continue;

		/* see how far around the box it is necessary to search */
		bound = maxdrcsurround(tech, subcell->lib, poly->layer);
		if (bound < 0) continue;

		/* determine network for this polygon */
		if (geom->entryisnode)
		{
			net = dr_quickgetnetnumber(poly->portproto, oni, globalindex);
		} else
		{
			net = ((INTBIG *)oai->network->temp1)[globalindex];
		}

		/* determine if original object has multiple contact cuts */
		minsize = polyminsize(poly);

		/* determine area to search inside of cell to check this layer */
		getbbox(poly, &lx, &hx, &ly, &hy);
		slx = lx-bound;   shx = hx+bound;
		sly = ly-bound;   shy = hy+bound;
		makerotI(ni, rtrans);
		maketransI(ni, ttrans);
		transmult(rtrans, ttrans, temptrans);
		xformbox(&slx, &shx, &sly, &shy, temptrans);

		maketrans(ni, ttrans);
		makerot(ni, rtrans);
		transmult(ttrans, rtrans, subtrans);

		/* see if this polygon has errors in the cell */
		if (dr_quickbadboxinarea(poly, poly->layer, tech, net, geom, trans, globalindex,
			slx, shx, sly, shy, np, localindex,
			subcell, globalindex, subtrans, minsize, basemulti, state, FALSE)) return(TRUE);
	}
	return(FALSE);
}

/*************************** QUICK DRC CACHE OF INSTANCE INTERACTIONS ***************************/

/*
 * Routine to look for an interaction between instances "ni1" and "ni2".  If it is found,
 * return TRUE.  If not found, add to the list and return FALSE.  In either case,
 * sets "thedii" to the address of the interaction object for this instance pair.
 */
BOOLEAN dr_quickcheckinteraction(NODEINST *ni1, NODEINST *ni2, INSTINTER **thedii)
{
	REGISTER NODEINST *swapni;
	REGISTER INSTINTER **newlist, **oldlist;
	INSTINTER *dii;
	REGISTER BOOLEAN found;
	REGISTER INTBIG newtotal, oldtotal, i;
	REGISTER CHECKPROTO *cp;

	/* must recheck parameterized instances always */
	*thedii = NOINSTINTER;
	cp = (CHECKPROTO *)ni1->proto->temp1;
	if (cp->cellparameterized) return(FALSE);
	cp = (CHECKPROTO *)ni2->proto->temp1;
	if (cp->cellparameterized) return(FALSE);

	/* keep the instances in proper numeric order */
	if ((INTBIG)ni1 < (INTBIG)ni2)
	{
		swapni = ni1;   ni1 = ni2;   ni2 = swapni;
	} else if (ni1 == ni2)
	{
		if (ni1->rotation + ni1->transpose*3600 < ni2->rotation + ni2->transpose*3600)
		{
			swapni = ni1;   ni1 = ni2;   ni2 = swapni;
		}
	}

	/* BEGIN critical section */
	if (dr_quickparalleldrc) emutexlock(dr_quickmutexinteract);

	/* get essential information about their interaction */
	dii = dr_quickallocinstinter();
	if (dii == NOINSTINTER) return(FALSE);
	dii->cell1 = ni1->proto;
	dii->rot1 = ni1->rotation;
	dii->trn1 = ni1->transpose;

	dii->cell2 = ni2->proto;
	dii->rot2 = ni2->rotation;
	dii->trn2 = ni2->transpose;
	dii->dx = (ni2->lowx + ni2->highx - (ni1->lowx + ni1->highx)) / 2;
	dii->dy = (ni2->lowy + ni2->highy - (ni1->lowy + ni1->highy)) / 2;

	/* if found, stop now */
	*thedii = dr_quickfindinteraction(dii);
	if (*thedii != NOINSTINTER)
	{
		dr_quickfreeinstinter(dii);

		/* END critical section */
		if (dr_quickparalleldrc) emutexunlock(dr_quickmutexinteract);
		return(TRUE);
	}

	/* build a hash code to locate this interaction */
	if (dr_quickinstintercount*2 >= dr_quickinstintertotal)
	{
		newtotal = dr_quickinstintertotal * 2;
		if (dr_quickinstintercount*2 >= newtotal)
			newtotal = dr_quickinstintercount+50;
		newtotal = pickprime(newtotal);
		newlist = (INSTINTER **)emalloc(newtotal * (sizeof (INSTINTER *)), dr_tool->cluster);
		if (newlist == 0)
		{
			if (dr_quickparalleldrc) emutexunlock(dr_quickmutexinteract);
			return(FALSE);
		}
		for(i=0; i<newtotal; i++)
			newlist[i] = NOINSTINTER;
		oldlist = dr_quickinstinter;
		oldtotal = dr_quickinstintertotal;
		dr_quickinstinter = newlist;
		dr_quickinstintertotal = newtotal;

		/* reinsert former entries */
		dr_quickinstintercount = 0;
		for(i=0; i<oldtotal; i++)
			if (oldlist[i] != NOINSTINTER)
				dr_quickinsertinteraction(oldlist[i]);
		if (oldtotal > 0) efree((CHAR *)oldlist);
	}

	/* see if this entry is there */
	found = FALSE;
	*thedii = dr_quickfindinteraction(dii);
	if (*thedii != NOINSTINTER)
	{
		dr_quickfreeinstinter(dii);
		found = TRUE;
	} else
	{
		/* insert it now */
		(void)dr_quickinsertinteraction(dii);
		*thedii = dii;
	}

	/* END critical section */
	if (dr_quickparalleldrc) emutexunlock(dr_quickmutexinteract);
	return(found);
}

/*
 * Routine to remove all instance interaction information.
 */
void dr_quickclearinstancecache(void)
{
	REGISTER INTBIG i;

	for(i=0; i<dr_quickinstintertotal; i++)
	{
		if (dr_quickinstinter[i] == NOINSTINTER) continue;
		dr_quickfreeinstinter(dr_quickinstinter[i]);
		dr_quickinstinter[i] = NOINSTINTER;
	}
}

/*
 * Routine to look for the instance-interaction in "dii" in the global list of instances interactions
 * that have already been checked.  Returns the entry if it is found, NOINSTINTER if not.
 */ 
INSTINTER *dr_quickfindinteraction(INSTINTER *dii)
{
	REGISTER INTBIG hash, i;
	REGISTER INSTINTER *diientry;

	if (dr_quickinstintertotal == 0) return(NOINSTINTER);
	hash = abs(((INTBIG)dii->cell1 + dii->rot1 + dii->trn1 +
		(INTBIG)dii->cell2 + dii->rot2 + dii->trn2 + dii->dx + dii->dy) %
			dr_quickinstintertotal);
	for(i=0; i<dr_quickinstintertotal; i++)
	{
		diientry = dr_quickinstinter[hash];
		if (diientry == NOINSTINTER) break;
		if (diientry->cell1 == dii->cell1 && diientry->rot1 == dii->rot1 && diientry->trn1 == dii->trn1 &&
			diientry->cell2 == dii->cell2 && diientry->rot2 == dii->rot2 && diientry->trn2 == dii->trn2 &&
			diientry->dx == dii->dx && diientry->dy == dii->dy) return(diientry);
		hash++;
		if (hash >= dr_quickinstintertotal) hash = 0;
	}
	return(NOINSTINTER);
}

/*
 * Routine to insert the instance-interaction in "dii" into the global list of instances interactions
 * that have already been checked.  Returns TRUE if there is no room.
 */ 
BOOLEAN dr_quickinsertinteraction(INSTINTER *dii)
{
	REGISTER INTBIG hash, i;

	hash = abs(((INTBIG)dii->cell1 + dii->rot1 + dii->trn1 +
		(INTBIG)dii->cell2 + dii->rot2 + dii->trn2 + dii->dx + dii->dy) %
			dr_quickinstintertotal);
	for(i=0; i<dr_quickinstintertotal; i++)
	{
		if (dr_quickinstinter[hash] == NOINSTINTER)
		{
			dr_quickinstinter[hash] = dii;
			dr_quickinstintercount++;
			return(FALSE);
		}
		hash++;
		if (hash >= dr_quickinstintertotal) hash = 0;
	}
	return(TRUE);
}

INSTINTER *dr_quickallocinstinter(void)
{
	REGISTER INSTINTER *dii;

	if (dr_quickinstinterfree == NOINSTINTER)
	{
		dii = (INSTINTER *)emalloc(sizeof (INSTINTER), dr_tool->cluster);
		if (dii == 0) return(NOINSTINTER);
	} else
	{
		dii = dr_quickinstinterfree;
		dr_quickinstinterfree = (INSTINTER *)dii->cell1;
	}
	return(dii);
}

/*
 * Routine to free INSTINTER object "dii".
 */
void dr_quickfreeinstinter(INSTINTER *dii)
{
	dii->cell1 = (NODEPROTO *)dr_quickinstinterfree;
	dr_quickinstinterfree = dii;
}

/*************************** QUICK DRC MULTIPROCESSOR SUPPORT ***************************/

/*
 * Routine to initialize the various state blocks and interlocks to prepare for multiprocessor
 * DRC using "numstates" processors.  Returns TRUE on error.
 */
BOOLEAN dr_quickmakestateblocks(INTBIG numstates)
{
	REGISTER INTBIG i;
	REGISTER CHECKSTATE **newstates;
	REGISTER void **newlocks;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	if (numstates <= dr_quickstatecount) return(FALSE);
	newstates = (CHECKSTATE **)emalloc(numstates * (sizeof (CHECKSTATE *)), dr_tool->cluster);
	if (newstates == 0) return(TRUE);
	newlocks = (void **)emalloc(numstates * (sizeof (void *)), dr_tool->cluster);
	if (newlocks == 0) return(TRUE);
	for(i=0; i<dr_quickstatecount; i++)
	{
		newstates[i] = dr_quickstate[i];
		newlocks[i] = dr_quickprocessdone[i];
	}
	for(i=dr_quickstatecount; i<numstates; i++)
	{
		newstates[i] = (CHECKSTATE *)emalloc(sizeof(CHECKSTATE), dr_tool->cluster);
		if (newstates[i] == 0) return(TRUE);

		newstates[i]->cellinstpolylist = allocpolylist(dr_tool->cluster);
		if (newstates[i]->cellinstpolylist == 0) return(TRUE);

		newstates[i]->nodeinstpolylist = allocpolylist(dr_tool->cluster);
		if (newstates[i]->nodeinstpolylist == 0) return(TRUE);

		newstates[i]->arcinstpolylist = allocpolylist(dr_tool->cluster);
		if (newstates[i]->arcinstpolylist == 0) return(TRUE);

		newstates[i]->subpolylist = allocpolylist(dr_tool->cluster);
		if (newstates[i]->subpolylist == 0) return(TRUE);

		newstates[i]->cropnodepolylist = allocpolylist(dr_tool->cluster);
		if (newstates[i]->cropnodepolylist == 0) return(TRUE);

		newstates[i]->croparcpolylist = allocpolylist(dr_tool->cluster);
		if (newstates[i]->croparcpolylist == 0) return(TRUE);

		newstates[i]->layerlookpolylist = allocpolylist(dr_tool->cluster);
		if (newstates[i]->layerlookpolylist == 0) return(TRUE);

		newstates[i]->activecroppolylist = allocpolylist(dr_tool->cluster);
		if (newstates[i]->activecroppolylist == 0) return(TRUE);

		newstates[i]->checkdistpoly1rebuild = NOPOLYGON;
		(void)needstaticpolygon(&newstates[i]->checkdistpoly1rebuild, 4, dr_tool->cluster);
		newstates[i]->checkdistpoly2rebuild = NOPOLYGON;
		(void)needstaticpolygon(&newstates[i]->checkdistpoly2rebuild, 4, dr_tool->cluster);

		newstates[i]->hierarchybasesnapshot = newhierarchicaltraversal();
		newstates[i]->hierarchytempsnapshot = newhierarchicaltraversal();

		newlocks[i] = 0;
		if (ensurevalidmutex(&newlocks[i], TRUE)) return(TRUE);
	}
	if (dr_quickstatecount > 0)
	{
		efree((CHAR *)dr_quickstate);
		efree((CHAR *)dr_quickprocessdone);
	}
	dr_quickstate = newstates;
	dr_quickprocessdone = newlocks;
	dr_quickstatecount = numstates;
	return(FALSE);
}

/*
 * Routine to return the next set of nodes or arcs to check.
 * Returns the number of objects to check (up to MAXCHECKOBJECTS)
 * and places them in "list".
 */
INTBIG dr_quickgetnextparallelgeoms(GEOM **list, CHECKSTATE *state)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER INTBIG i;

	/* grab a bunch of primitive nodes first */
	i = 0;
	if (dr_quickparallelnodeinst != NONODEINST)
	{
		/* only the main thread can call "stopping" */
		if (state == dr_quickstate[dr_quickmainthread])
			(void)stopping(STOPREASONDRC);

		/* all threads check whether interrupt has been requested */
		if (el_pleasestop != 0)
		{
			dr_quickparallelnodeinst = NONODEINST;
			dr_quickparallelcellinst = NONODEINST;
			dr_quickparallelarcinst = NOARCINST;
			return(0);
		}

		/* if errors were found and only the first is requested, stop now */
		if (dr_quickerrorsfound && (dr_quickoptions&DRCFIRSTERROR) != 0)
		{
			dr_quickparallelnodeinst = NONODEINST;
			dr_quickparallelcellinst = NONODEINST;
			dr_quickparallelarcinst = NOARCINST;
			return(0);
		}

		emutexlock(dr_quickmutexnode);
		for(;;)
		{
			ni = dr_quickparallelnodeinst;
			if (ni == NONODEINST) break;
			dr_quickparallelnodeinst = ni->nextnodeinst;

			/* ignore cell instances */
			if (ni->proto->primindex == 0) continue;

			/* ignore if not in requested area */
			if (dr_quickjustarea)
			{
				if (ni->geom->lowx >= dr_quickareahx || ni->geom->highx <= dr_quickarealx ||
					ni->geom->lowy >= dr_quickareahy || ni->geom->highy <= dr_quickarealy) continue;
			}

			list[i] = ni->geom;
			i++;
			if (i >= MAXCHECKOBJECTS) break;
		}
		emutexunlock(dr_quickmutexnode);
		if (i > 0) return(i);
	}

	/* grab a bunch of arcs next */
	if (dr_quickparallelarcinst != NOARCINST)
	{
		/* only the main thread can call "stopping" */
		if (state == dr_quickstate[dr_quickmainthread])
			(void)stopping(STOPREASONDRC);

		/* all threads check whether interrupt has been requested */
		if (el_pleasestop != 0)
		{
			dr_quickparallelnodeinst = NONODEINST;
			dr_quickparallelcellinst = NONODEINST;
			dr_quickparallelarcinst = NOARCINST;
			return(0);
		}

		/* if errors were found and only the first is requested, stop now */
		if (dr_quickerrorsfound && (dr_quickoptions&DRCFIRSTERROR) != 0)
		{
			dr_quickparallelnodeinst = NONODEINST;
			dr_quickparallelcellinst = NONODEINST;
			dr_quickparallelarcinst = NOARCINST;
			return(0);
		}

		emutexlock(dr_quickmutexnode);
		for(;;)
		{
			ai = dr_quickparallelarcinst;
			if (ai == NOARCINST) break;
			dr_quickparallelarcinst = ai->nextarcinst;

			/* ignore if not in requested area */
			if (dr_quickjustarea)
			{
				if (ai->geom->lowx >= dr_quickareahx || ai->geom->highx <= dr_quickarealx ||
					ai->geom->lowy >= dr_quickareahy || ai->geom->highy <= dr_quickarealy) continue;
			}

			list[i] = ai->geom;
			i++;
			if (i >= MAXCHECKOBJECTS) break;
		}
		emutexunlock(dr_quickmutexnode);
		if (i > 0) return(i);
	}

	/* grab cell instances, one at a time */
	if (dr_quickparallelcellinst == NONODEINST) return(0);

	/* only the main thread can call "stopping" */
	if (state == dr_quickstate[dr_quickmainthread])
		(void)stopping(STOPREASONDRC);

	/* all threads check whether interrupt has been requested */
	if (el_pleasestop != 0)
	{
		dr_quickparallelnodeinst = NONODEINST;
		dr_quickparallelcellinst = NONODEINST;
		dr_quickparallelarcinst = NOARCINST;
		return(0);
	}

	/* if errors were found and only the first is requested, stop now */
	if (dr_quickerrorsfound && (dr_quickoptions&DRCFIRSTERROR) != 0)
	{
		dr_quickparallelnodeinst = NONODEINST;
		dr_quickparallelcellinst = NONODEINST;
		dr_quickparallelarcinst = NOARCINST;
		return(0);
	}

	emutexlock(dr_quickmutexnode);
	for(;;)
	{
		ni = dr_quickparallelcellinst;
		if (ni == NONODEINST) break;
		dr_quickparallelcellinst = ni->nextnodeinst;

		/* only want cell instances */
		if (ni->proto->primindex != 0) continue;

		/* ignore if not in requested area */
		if (dr_quickjustarea)
		{
			if (ni->geom->lowx >= dr_quickareahx || ni->geom->highx <= dr_quickarealx ||
				ni->geom->lowy >= dr_quickareahy || ni->geom->highy <= dr_quickarealy) continue;
		}

		list[0] = ni->geom;
		i++;
		break;
	}
	emutexunlock(dr_quickmutexnode);
	return(i);
}

/*
 * Routine to run a thread that repeatedly fetches a node and analyzes it.
 */
void *dr_quickdothread(void *argument)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER GEOM *geom;
	REGISTER BOOLEAN ret;
	GEOM *list[MAXCHECKOBJECTS];
	REGISTER INTBIG i, count, globalindex;
	REGISTER CHECKSTATE *state;

	state = dr_quickstate[(INTBIG)argument];
	globalindex = state->globalindex;
	for(;;)
	{
		count = dr_quickgetnextparallelgeoms(list, state);
		if (count == 0) break;
		for(i=0; i<count; i++)
		{
			geom = list[i];
			if (geom->entryisnode)
			{
				ni = geom->entryaddr.ni;
				if (ni->proto->primindex == 0)
				{
					ret = dr_quickcheckcellinst(ni, globalindex, state);
				} else
				{
					ret = dr_quickchecknodeinst(ni, globalindex, state);
				}
			} else
			{
				ai = geom->entryaddr.ai;
				ret = dr_quickcheckarcinst(ai, globalindex, state);
			}
			if (ret)
			{
				dr_quickerrorsfound++;
				if ((dr_quickoptions&DRCFIRSTERROR) != 0) break;
			}
		}
	}

	/* mark this process as "done" */
	emutexunlock(dr_quickprocessdone[(INTBIG)argument]);
	return(0);
}

/************************* QUICK DRC HIERARCHICAL NETWORK BUILDING *************************/

/*
 * Routine to recursively examine the hierarchy below cell "cell" and fill in the
 * CHECKINST objects on every cell instance.  Uses the CHECKPROTO objects
 * to keep track of cell usage.
 */
void dr_quickcheckenumerateinstances(NODEPROTO *cell)
{
	REGISTER NODEINST *ni;
	REGISTER CHECKINST *ci;
	REGISTER CHECKPROTO *cp, *firstcp;

	/* number all of the instances in this cell */
	dr_quickchecktimestamp++;
	firstcp = NOCHECKPROTO;
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex != 0) continue;

		/* ignore documentation icons */
		if (isiconof(ni->proto, cell)) continue;

		cp = (CHECKPROTO *)ni->proto->temp1;
		if (cp->timestamp != dr_quickchecktimestamp)
		{
			cp->timestamp = dr_quickchecktimestamp;
			cp->instancecount = 0;
			cp->firstincell = NONODEINST;
			cp->nextcheckproto = firstcp;
			firstcp = cp;
		}
		ci = (CHECKINST *)ni->temp1;
		ci->localindex = cp->instancecount++;
		ni->temp2 = (INTBIG)cp->firstincell;
		cp->firstincell = ni;
	}

	/* update the counts for this cell */
	for(cp = firstcp; cp != NOCHECKPROTO; cp = cp->nextcheckproto)
	{
		cp->hierinstancecount += cp->instancecount;
		for(ni = cp->firstincell; ni != NONODEINST; ni = (NODEINST *)ni->temp2)
		{
			ci = (CHECKINST *)ni->temp1;
			ci->multiplier = cp->instancecount;
		}
	}

	/* now recurse */
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex != 0) continue;

		/* ignore documentation icons */
		if (isiconof(ni->proto, cell)) continue;

		dr_quickcheckenumerateinstances(ni->proto);
	}
}

void dr_quickcheckenumeratenetworks(NODEPROTO *cell, INTBIG globalindex)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *subcell;
	REGISTER CHECKINST *ci;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER ARCINST *ai;
	REGISTER NETWORK *net;
	REGISTER INTBIG localindex, *netnumbers;

#ifdef VALIDATENETWORKS	/* assert: index must be valid */
	REGISTER CHECKPROTO *cp;
	cp = (CHECKPROTO *)cell->temp1;
	if (globalindex >= cp->hierinstancecount)
		ttyputmsg(x_("Invalid global index (%d) in cell %s (limit is %d)"), globalindex,
			describenodeproto(cell), cp->hierinstancecount);
#endif

	/* store all network information in the appropriate place */
	for(net = cell->firstnetwork; net != NONETWORK; net = net->nextnetwork)
	{
		netnumbers = (INTBIG *)net->temp1;

#ifdef VALIDATENETWORKS	/* assert: must only fill each entry once */
	if (netnumbers[globalindex] != 0)
		ttyputmsg(x_("Duplicate network index (%d) on network %s in cell %s"), globalindex,
			describenetwork(net), describenodeproto(cell));
#endif
		netnumbers[globalindex] = net->temp2;
	}

	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex != 0) continue;

		/* ignore documentation icons */
		if (isiconof(ni->proto, cell)) continue;

		/* compute the index of this instance */
		ci = (CHECKINST *)ni->temp1;
		localindex = globalindex * ci->multiplier + ci->localindex + ci->offset;

		/* propagate down the hierarchy */
		subcell = ni->proto;
		for(net = subcell->firstnetwork; net != NONETWORK; net = net->nextnetwork)
			net->temp2 = 0;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			ai = pi->conarcinst;
			if (ai->network == NONETWORK) continue;
			pi->proto->network->temp2 = ai->network->temp2;
		}
		for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		{
			if (pe->proto->network->temp2 == 0)
				pe->proto->network->temp2 = pe->exportproto->network->temp2;
		}
		for(net = subcell->firstnetwork; net != NONETWORK; net = net->nextnetwork)
			if (net->temp2 == 0) net->temp2 = dr_quickchecknetnumber++;
		dr_quickcheckenumeratenetworks(subcell, localindex);
	}
}

/*
 * Routine to allocate a "CHECKINST" object.  Returns NOCHECKINST on error.
 */
CHECKINST *dr_quickalloccheckinst(void)
{
	REGISTER CHECKINST *ci;

	if (dr_quickcheckinstfree == NOCHECKINST)
	{
		ci = (CHECKINST *)emalloc(sizeof (CHECKINST), dr_tool->cluster);
		if (ci == 0) return(NOCHECKINST);
	} else
	{
		ci = dr_quickcheckinstfree;
		dr_quickcheckinstfree = (CHECKINST *)ci->localindex;
	}
	return(ci);
}

/*
 * Routine to free CHECKINST object "ci".
 */
void dr_quickfreecheckinst(CHECKINST *ci)
{
	ci->localindex = (INTBIG)dr_quickcheckinstfree;
	dr_quickcheckinstfree = ci;
}

/*
 * Routine to allocate a "CHECKPROTO" object.  Returns NOCHECKPROTO on error.
 */
CHECKPROTO *dr_quickalloccheckproto(void)
{
	REGISTER CHECKPROTO *cp;

	if (dr_quickcheckprotofree == NOCHECKPROTO)
	{
		cp = (CHECKPROTO *)emalloc(sizeof (CHECKPROTO), dr_tool->cluster);
		if (cp == 0) return(NOCHECKPROTO);
	} else
	{
		cp = dr_quickcheckprotofree;
		dr_quickcheckprotofree = cp->nextcheckproto;
	}
	return(cp);
}

/*
 * Routine to free CHECKPROTO object "cp".
 */
void dr_quickfreecheckproto(CHECKPROTO *cp)
{
	cp->nextcheckproto = dr_quickcheckprotofree;
	dr_quickcheckprotofree = cp;
}

/*********************************** QUICK DRC SUPPORT ***********************************/

/*
 * Routine to ensure that polygon "poly" on layer "layer" from object "geom" in
 * technology "tech" meets minimum width rules.  If it is too narrow, other layers
 * in the vicinity are checked to be sure it is indeed an error.  Returns true
 * if an error is found.
 */
BOOLEAN dr_quickcheckminwidth(GEOM *geom, INTBIG layer, POLYGON *poly, TECHNOLOGY *tech, CHECKSTATE *state)
{
	REGISTER INTBIG minwidth, actual;
	INTBIG lx, hx, ly, hy, ix, iy;
	CHAR *rule;
	double rang, roang, fdx, fdy;
	BOOLEAN p1found, p2found, p3found;
	REGISTER double ang, oang, perpang;
	REGISTER NODEPROTO *cell;
	REGISTER INTBIG xl1, yl1, xl2, yl2, xl3, yl3, xr1, yr1, xr2, yr2, xr3, yr3,
		cx, cy, fx, fy, tx, ty, ofx, ofy, otx, oty, i, j;

	cell = geomparent(geom);
	minwidth = drcminwidth(tech, cell->lib, layer, &rule);
	if (minwidth < 0) return(FALSE);

	/* simpler analysis if manhattan */
	if (isbox(poly, &lx, &hx, &ly, &hy))
	{
		if (hx - lx >= minwidth && hy - ly >= minwidth) return(FALSE);
		if (hx - lx < minwidth)
		{
			actual = hx - lx;
			xl1 = xl2 = xl3 = lx - 1;
			xr1 = xr2 = xr3 = hx + 1;
			yl1 = yr1 = ly;
			yl2 = yr2 = hy;
			yl3 = yr3 = (yl1+yl2)/2;
		} else
		{
			actual = hy - ly;
			xl1 = xr1 = lx;
			xl2 = xr2 = hx;
			xl3 = xr3 = (xl1+xl2)/2;
			yl1 = yl2 = yl3 = ly - 1;
			yr1 = yr2 = yr3 = hy + 1;
		}

		/* see if there is more of this layer adjoining on either side */
		p1found = p2found = p3found = FALSE;
		if (dr_quicklookforlayer(cell, layer, el_matid, state, lx-1, hx+1, ly-1, hy+1,
			xl1, yl1, &p1found, xl2, yl2, &p2found, xl3, yl3, &p3found)) return(FALSE);

		p1found = p2found = p3found = FALSE;
		if (dr_quicklookforlayer(cell, layer, el_matid, state, lx-1, hx+1, ly-1, hy+1,
			xr1, yr1, &p1found, xr2, yr2, &p2found, xr3, yr3, &p3found)) return(FALSE);

		dr_quickreporterror(MINWIDTHERROR, tech, 0, cell, minwidth, actual, rule,
			poly, geom, layer, NONET, NOPOLYGON, NOGEOM, 0, NONET);
		return(TRUE);
	}

	/* nonmanhattan polygon: stop now if it has no size */
	switch (poly->style)
	{
		case FILLED:
		case CLOSED:
		case CROSSED:
		case OPENED:
		case OPENEDT1:
		case OPENEDT2:
		case OPENEDT3:
		case VECTORS:
			break;
		default:
			return(FALSE);
	}

	/* simple check of nonmanhattan polygon for minimum width */
	getbbox(poly, &lx, &hx, &ly, &hy);
	actual = mini(hx-lx, hy-ly);
	if (actual < minwidth)
	{
		dr_quickreporterror(MINWIDTHERROR, tech, 0, cell, minwidth, actual, rule,
			poly, geom, layer, NONET, NOPOLYGON, NOGEOM, 0, NONET);
		return(TRUE);
	}

	/* check distance of each line's midpoint to perpendicular opposite point */
	for(i=0; i<poly->count; i++)
	{
		if (i == 0)
		{
			fx = poly->xv[poly->count-1];   fy = poly->yv[poly->count-1];
		} else
		{
			fx = poly->xv[i-1];   fy = poly->yv[i-1];
		}
		tx = poly->xv[i];   ty = poly->yv[i];
		if (fx == tx && fy == ty) continue;
		ang = ffigureangle(fx, fy, tx, ty);
		cx = (fx + tx) / 2;
		cy = (fy + ty) / 2;
		perpang = ang + EPI/2;
		for(j=0; j<poly->count; j++)
		{
			if (j == i) continue;
			if (j == 0)
			{
				ofx = poly->xv[poly->count-1];   ofy = poly->yv[poly->count-1];
			} else
			{
				ofx = poly->xv[j-1];   ofy = poly->yv[j-1];
			}
			otx = poly->xv[j];   oty = poly->yv[j];
			if (ofx == otx && ofy == oty) continue;
			oang = ffigureangle(ofx, ofy, otx, oty);
			rang = ang;   while (rang > EPI) rang -= EPI;
			roang = oang;   while (roang > EPI) roang -= EPI;
			if (doublesequal(oang, roang))
			{
				/* lines are parallel: see if they are colinear */
				if (isonline(fx, fy, tx, ty, ofx, ofy)) continue;
				if (isonline(fx, fy, tx, ty, otx, oty)) continue;
				if (isonline(ofx, ofy, otx, oty, fx, fy)) continue;
				if (isonline(ofx, ofy, otx, oty, tx, ty)) continue;
			}
			if (fintersect(cx, cy, perpang, ofx, ofy, oang, &ix, &iy) < 0) continue;
			if (ix < mini(ofx, otx) || ix > maxi(ofx, otx)) continue;
			if (iy < mini(ofy, oty) || iy > maxi(ofy, oty)) continue;
			fdx = cx - ix;   fdy = cy - iy;
			actual = rounddouble(sqrt(fdx*fdx + fdy*fdy));

			/* becuase this is done in integer, accuracy may suffer */
			actual += 2;

			if (actual < minwidth)
			{
				/* look between the points to see if it is minimum width or notch */
				if (isinside((cx+ix)/2, (cy+iy)/2, poly))
				{
					dr_quickreporterror(MINWIDTHERROR, tech, 0, cell, minwidth,
						actual, rule, poly, geom, layer, NONET, NOPOLYGON, NOGEOM, 0, NONET);
				} else
				{
					dr_quickreporterror(NOTCHERROR, tech, 0, cell, minwidth,
						actual, rule, poly, geom, layer, NONET, poly, geom, layer, NONET);
				}
				return(TRUE);
			}
		}
	}
	return(FALSE);
}

/*
 * Routine to examine the hierarchy below cell "cell" and return TRUE if any of it is
 * parameterized.
 */
BOOLEAN dr_quickfindparameters(NODEPROTO *cell)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER CHECKPROTO *cp;

	cp = (CHECKPROTO *)cell->temp1;
	if (cp->cellparameterized) return(TRUE);
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		np = ni->proto;
		if (np->primindex != 0) continue;
		if (isiconof(np, cell)) continue;
		if (dr_quickfindparameters(np)) return(TRUE);
	}
	return(FALSE);
}

/*
 * Routine to determine whether node "ni" is a multiple cut contact.
 */
BOOLEAN dr_quickismulticut(NODEINST *ni)
{
	REGISTER NODEPROTO *np;
	REGISTER TECHNOLOGY *tech;
	TECH_NODES *thistn;
	INTBIG fewer, cutcount;
	POLYLOOP mypl;

	np = ni->proto;
	if (np->primindex == 0) return(FALSE);
	tech = np->tech;
	thistn = tech->nodeprotos[np->primindex-1];
	if (thistn->special != MULTICUT) return(FALSE);
	cutcount = tech_moscutcount(ni, thistn->f1, thistn->f2, thistn->f3, thistn->f4,
		&fewer, &mypl);
	if (cutcount > 1) return(TRUE);
	return(FALSE);
}

/*
 * routine to tell whether the objects at geometry modules "geom1" and "geom2"
 * touch directly (that is, an arcinst connected to a nodeinst).  The routine
 * returns true if they touch
 */
BOOLEAN dr_quickobjtouch(GEOM *geom1, GEOM *geom2)
{
	REGISTER GEOM *temp;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER INTBIG i;

	if (geom1->entryisnode)
	{
		if (geom2->entryisnode) return(FALSE);
		temp = geom1;   geom1 = geom2;   geom2 = temp;
	}
	if (!geom2->entryisnode)
		return(FALSE);

	/* see if the arcinst at "geom1" touches the nodeinst at "geom2" */
	ni = geom2->entryaddr.ni;
	ai = geom1->entryaddr.ai;
	for(i=0; i<2; i++)
		if (ai->end[i].nodeinst == ni)
			return(TRUE);
	return(FALSE);
}

/*
 * Routine to find two points between polygons "poly1" and "poly2" that can be used to test
 * for notches.  The points are returned in (xf1,yf1) and (xf2,yf2).  Returns zero if no
 * points exist in the gap between the polygons (becuase they don't have common facing edges).
 * Returns 1 if only one of the reported points needs to be filled (because the polygons meet
 * at a point).  Returns 2 if both reported points need to be filled.
 */
INTBIG dr_quickfindinterveningpoints(POLYGON *poly1, POLYGON *poly2, INTBIG *xf1, INTBIG *yf1,
	INTBIG *xf2, INTBIG *yf2)
{
	REGISTER BOOLEAN isbox1, isbox2;
	INTBIG lx1, hx1, ly1, hy1, lx2, hx2, ly2, hy2;
	REGISTER INTBIG xc, yc;

	isbox1 = isbox(poly1, &lx1, &hx1, &ly1, &hy1);
	isbox2 = isbox(poly2, &lx2, &hx2, &ly2, &hy2);
	if (isbox1 && isbox2)
	{
		/* handle vertical gap between polygons */
		if (lx1 > hx2 || lx2 > hx1)
		{
			/* see if the polygons are horizontally aligned */
			if (ly1 <= hy2 && ly2 <= hy1)
			{
				if (lx1 > hx2) *xf1 = *xf2 = (lx1 + hx2) / 2; else
					*xf1 = *xf2 = (lx2 + hx1) / 2;
				*yf1 = maxi(ly1, ly2);
				*yf2 = mini(hy1, hy2);
				return(2);
			}
		} else if (ly1 > hy2 || ly2 > hy1)
		{
			/* see if the polygons are horizontally aligned */
			if (lx1 <= hx2 && lx2 <= hx1)
			{
				if (ly1 > hy2) *yf1 = *yf2 = (ly1 + hy2) / 2; else
					*yf1 = *yf2 = (ly2 + hy1) / 2;
				*xf1 = maxi(lx1, lx2);
				*xf2 = mini(hx1, hx2);
				return(2);
			}
		} else if ((lx1 == hx2 || lx2 == hx1) || (ly1 == hy2 || ly2 == hy2))
		{
			/* handle touching at a point */
			if (lx1 == hx2) xc = lx1; else
				xc = lx2;
			if (ly1 == hy2) yc = ly1; else
				yc = ly2;
			*xf1 = xc + 1;   *yf1 = yc + 1;
			*xf2 = xc - 1;   *yf2 = yc - 1;
			if ((*xf1 < lx1 || *xf1 > hx1 || *yf1 < ly1 || *yf1 > hy1) &&
				(*xf1 < lx2 || *xf1 > hx2 || *yf1 < ly2 || *yf1 > hy2)) return(1);
			*xf1 = xc + 1;   *yf1 = yc - 1;
			*xf2 = xc - 1;   *yf2 = yc + 1;
			return(1);
		}

		/* handle manhattan objects that are on a diagonal */
		if (lx1 > hx2)
		{
			if (ly1 > hy2)
			{
				*xf1 = lx1;   *yf1 = ly1;
				*xf2 = hx2;   *yf2 = hy2;
				return(1);
			}
			if (ly2 > hy1)
			{
				*xf1 = lx1;   *yf1 = hy1;
				*xf2 = hx2;   *yf2 = ly2;
				return(1);
			}
		}
		if (lx2 > hx1)
		{
			if (ly1 > hy2)
			{
				*xf1 = hx1;   *yf1 = hy1;
				*xf2 = lx2;   *yf2 = ly2;
				return(1);
			}
			if (ly2 > hy1)
			{
				*xf1 = hx1;   *yf1 = ly1;
				*xf2 = lx2;   *yf2 = hy2;
				return(1);
			}
		}
	}

	/* boxes don't line up or this is a nonmanhattan situation */
	*xf1 = (lx1 + hx1) / 2;   *yf1 = (ly2 + hy2) / 2;
	*xf2 = (lx2 + hx2) / 2;   *yf2 = (ly1 + hy1) / 2;
	return(1);
}

/*
 * Routine to explore the points (xf1,yf1) and (xf2,yf2) to see if there is
 * geometry on layer "layer" (in or below cell "cell").  Returns true if there is.
 * If "needboth" is true, both points must have geometry, otherwise only 1 needs it.
 */
BOOLEAN dr_quicklookforpoints(INTBIG xf1, INTBIG yf1, INTBIG xf2, INTBIG yf2,
	INTBIG layer, NODEPROTO *cell, BOOLEAN needboth, CHECKSTATE *state)
{
	INTBIG xf3, yf3, flx, fhx, fly, fhy;
	BOOLEAN p1found, p2found, p3found, allfound;

	xf3 = (xf1+xf2) / 2;
	yf3 = (yf1+yf2) / 2;

	/* compute bounds for searching inside cells */
	flx = mini(xf1, xf2);   fhx = maxi(xf1, xf2);
	fly = mini(yf1, yf2);   fhy = maxi(yf1, yf2);

	/* search the cell for geometry that fills the notch */
	p1found = p2found = p3found = FALSE;
	allfound = dr_quicklookforlayer(cell, layer, el_matid, state, flx, fhx, fly, fhy,
		xf1, yf1, &p1found, xf2, yf2, &p2found, xf3, yf3, &p3found);
	if (needboth)
	{
		if (allfound) return(TRUE);
	} else
	{
		if (p1found || p2found) return(TRUE);
	}

	return(FALSE);
}

/*
 * Routine to examine cell "cell" in the area (lx<=X<=hx, ly<=Y<=hy) for objects
 * on layer "layer".  Apply transformation "moretrans" to the objects.  If polygons are
 * found at (xf1,yf1) or (xf2,yf2) or (xf3,yf3) then sets "p1found/p2found/p3found" to 1.
 * If all locations are found, returns true.
 */
BOOLEAN dr_quicklookforlayer(NODEPROTO *cell, INTBIG layer, XARRAY moretrans, CHECKSTATE *state,
	INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy, INTBIG xf1, INTBIG yf1, BOOLEAN *p1found,
		INTBIG xf2, INTBIG yf2, BOOLEAN *p2found, INTBIG xf3, INTBIG yf3, BOOLEAN *p3found)
{
	REGISTER INTBIG sea, i, tot;
	REGISTER GEOM *g;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER BOOLEAN reasonable;
	INTBIG newlx, newhx, newly, newhy;
	XARRAY trans, rot, bound;
	REGISTER POLYGON *poly;

	sea = initsearch(lx, hx, ly, hy, cell);
	for(;;)
	{
		g = nextobject(sea);
		if (g == NOGEOM) break;
		if (g->entryisnode)
		{
			ni = g->entryaddr.ni;
			if (ni->proto->primindex == 0)
			{
				/* compute bounding area inside of sub-cell */
				makerotI(ni, rot);
				maketransI(ni, trans);
				transmult(rot, trans, bound);
				newlx = lx;   newly = ly;   newhx = hx;   newhy = hy;
				xformbox(&newlx, &newhx, &newly, &newhy, bound);

				/* compute new matrix for sub-cell examination */
				maketrans(ni, trans);
				makerot(ni, rot);
				transmult(trans, rot, bound);
				transmult(bound, moretrans, trans);
				if (dr_quicklookforlayer(ni->proto, layer, trans, state, newlx, newhx, newly, newhy,
					xf1, yf1, p1found, xf2, yf2, p2found, xf3, yf3, p3found))
				{
					termsearch(sea);
					return(TRUE);
				}
				continue;
			}
			makerot(ni, rot);
			transmult(rot, moretrans, bound);
			if ((dr_quickoptions&DRCREASONABLE) != 0) reasonable = TRUE; else reasonable = FALSE;
			tot = allnodepolys(ni, state->layerlookpolylist, NOWINDOWPART, reasonable);
			for(i=0; i<tot; i++)
			{
				poly = state->layerlookpolylist->polygons[i];
				if (!samelayer(poly->tech, poly->layer, layer)) continue;
				xformpoly(poly, bound);
				if (isinside(xf1, yf1, poly)) *p1found = TRUE;
				if (isinside(xf2, yf2, poly)) *p2found = TRUE;
				if (isinside(xf3, yf3, poly)) *p3found = TRUE;
			}
		} else
		{
			ai = g->entryaddr.ai;
			tot = allarcpolys(ai, state->layerlookpolylist, NOWINDOWPART);
			for(i=0; i<tot; i++)
			{
				poly = state->layerlookpolylist->polygons[i];
				if (!samelayer(poly->tech, poly->layer, layer)) continue;
				xformpoly(poly, moretrans);
				if (isinside(xf1, yf1, poly)) *p1found = TRUE;
				if (isinside(xf2, yf2, poly)) *p2found = TRUE;
				if (isinside(xf3, yf3, poly)) *p3found = TRUE;
			}
		}
		if (*p1found && *p2found && *p3found)
		{
			termsearch(sea);
			return(TRUE);
		}
	}
	return(FALSE);
}

/*
 * routine to see if the two boxes are active elements, connected to opposite
 * sides of a field-effect transistor that resides inside of the box area.
 * Returns true if so.
 */
BOOLEAN dr_quickactiveontransistor(POLYGON *poly1, INTBIG layer1, INTBIG net1,
	POLYGON *poly2, INTBIG layer2, INTBIG net2, TECHNOLOGY *tech, NODEPROTO *cell, INTBIG globalindex)
{
	REGISTER INTBIG blx, bhx, bly, bhy, fun;
	INTBIG lx1, hx1, ly1, hy1, lx2, hx2, ly2, hy2;

	/* networks must be different */
	if (net1 == net2) return(FALSE);

	/* layers must be active or active contact */
	fun = layerfunction(tech, layer1);
	if ((fun&LFTYPE) != LFDIFF)
	{
		if (!layeriscontact(fun) || (fun&LFCONDIFF) == 0) return(FALSE);
	}
	fun = layerfunction(tech, layer2);
	if ((fun&LFTYPE) != LFDIFF)
	{
		if (!layeriscontact(fun) || (fun&LFCONDIFF) == 0) return(FALSE);
	}

	/* search for intervening transistor in the cell */
	getbbox(poly1, &lx1, &hx1, &ly1, &hy1);
	getbbox(poly2, &lx2, &hx2, &ly2, &hy2);
	blx = mini(lx1,lx2);   bhx = maxi(hx1,hx2);
	bly = mini(ly1,ly2);   bhy = maxi(hy1,hy2);
	return(dr_quickactiveontransistorrecurse(blx, bhx, bly, bhy, net1, net2, cell, globalindex, el_matid));
}

BOOLEAN dr_quickactiveontransistorrecurse(INTBIG blx, INTBIG bhx, INTBIG bly, INTBIG bhy,
	INTBIG net1, INTBIG net2, NODEPROTO *cell, INTBIG globalindex, XARRAY trans)
{
	REGISTER INTBIG sea, net, cx, cy, localindex;
	REGISTER BOOLEAN on1, on2, ret;
	REGISTER GEOM *g;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *badport;
	REGISTER PORTARCINST *pi;
	INTBIG slx, shx, sly, shy;
	XARRAY rtrans, ttrans, temptrans;
	REGISTER CHECKINST *ci;

	sea = initsearch(blx, bhx, bly, bhy, cell);
	if (sea < 0) return(FALSE);
	for(;;)
	{
		g = nextobject(sea);
		if (g == NOGEOM) break;
		if (!g->entryisnode) continue;
		ni = g->entryaddr.ni;
		np = ni->proto;
		if (np->primindex == 0)
		{
			makerotI(ni, rtrans);
			maketransI(ni, ttrans);
			transmult(rtrans, ttrans, temptrans);
			slx = blx;   shx = bhx;
			sly = bly;   shy = bhy;
			xformbox(&slx, &shx, &sly, &shy, temptrans);

			ci = (CHECKINST *)ni->temp1;
			localindex = globalindex * ci->multiplier + ci->localindex + ci->offset;

			if (!dr_quickparalleldrc) downhierarchy(ni, np, 0);
			ret = dr_quickactiveontransistorrecurse(slx, shx, sly, shy,
				net1, net2, np, localindex, trans);
			if (!dr_quickparalleldrc) uphierarchy();
			if (ret)
			{
				termsearch(sea);
				return(TRUE);
			}
			continue;
		}

		/* must be a transistor */
		if (!isfet(g)) continue;

		/* must be inside of the bounding box of the desired layers */
		cx = (ni->geom->lowx + ni->geom->highx) / 2;
		cy = (ni->geom->lowy + ni->geom->highy) / 2;
		if (cx < blx || cx > bhx || cy < bly || cy > bhy) continue;

		/* determine the poly port */
		badport = ni->proto->firstportproto;

		on1 = on2 = FALSE;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			/* ignore connections on poly/gate */
			if (pi->proto->network == badport->network) continue;

			net = ((INTBIG *)pi->conarcinst->network->temp1)[globalindex];
			if (net == NONET) continue;
			if (net == net1) on1 = TRUE;
			if (net == net2) on2 = TRUE;
		}

		/* if either side is not connected, ignore this */
		if (!on1 || !on2) continue;

		/* transistor found that connects to both actives */
		termsearch(sea);
		return(TRUE);
	}
	return(FALSE);
}

/*
 * routine to crop the box in the reference parameters (lx-hx, ly-hy)
 * against the box in (bx-ux, by-uy).  If the box is cropped into oblivion,
 * the routine returns 1.  If the boxes overlap but cannot be cleanly cropped,
 * the routine returns -1.  Otherwise the box is cropped and zero is returned
 */
INTBIG dr_quickcropbox(INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy, INTBIG bx, INTBIG ux, INTBIG by,
	INTBIG uy, INTBIG nlx, INTBIG nhx, INTBIG nly, INTBIG nhy)
{
	REGISTER INTBIG xoverlap, yoverlap;

	/* if the two boxes don't touch, just return */
	if (bx >= *hx || by >= *hy || ux <= *lx || uy <= *ly) return(0);

	/* if the box to be cropped is within the other, say so */
	if (bx <= *lx && ux >= *hx && by <= *ly && uy >= *hy) return(1);

	/* see which direction is being cropped */
	xoverlap = mini(*hx, ux) - maxi(*lx, bx);
	yoverlap = mini(*hy, uy) - maxi(*ly, by);
	if (xoverlap > yoverlap)
	{
		/* one above the other: crop in Y */
		if (bx <= *lx && ux >= *hx)
		{
			/* it covers in X...do the crop */
			if (uy >= *hy) *hy = by;
			if (by <= *ly) *ly = uy;
			if (*hy <= *ly) return(1);
			return(0);
		}
	} else
	{
		/* one next to the other: crop in X */
		if (by <= *ly && uy >= *hy)
		{
			/* it covers in Y...crop in X */
			if (ux >= *hx) *hx = bx;
			if (bx <= *lx) *lx = ux;
			if (*hx <= *lx) return(1);
			return(0);
		}
	}
	return(-1);
}

/*
 * routine to crop the box in the reference parameters (lx-hx, ly-hy)
 * against the box in (bx-ux, by-uy).  If the box is cropped into oblivion,
 * the routine returns 1.  If the boxes overlap but cannot be cleanly cropped,
 * the routine returns -1.  Otherwise the box is cropped and zero is returned
 */
INTBIG dr_quickhalfcropbox(INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy,
	INTBIG bx, INTBIG ux, INTBIG by, INTBIG uy)
{
	REGISTER BOOLEAN crops;
	REGISTER INTBIG lxe, hxe, lye, hye, biggestext;

	/* if the two boxes don't touch, just return */
	if (bx >= *hx || by >= *hy || ux <= *lx || uy <= *ly) return(0);

	/* if the box to be cropped is within the other, figure out which half to remove */
	if (bx <= *lx && ux >= *hx && by <= *ly && uy >= *hy)
	{
		lxe = *lx - bx;   hxe = ux - *hx;
		lye = *ly - by;   hye = uy - *hy;
		biggestext = maxi(maxi(lxe, hxe), maxi(lye, hye));
		if (biggestext == 0) return(1);
		if (lxe == biggestext)
		{
			*lx = (*lx + ux) / 2;
			if (*lx >= *hx) return(1);
			return(0);
		}
		if (hxe == biggestext)
		{
			*hx = (*hx + bx) / 2;
			if (*hx <= *lx) return(1);
			return(0);
		}
		if (lye == biggestext)
		{
			*ly = (*ly + uy) / 2;
			if (*ly >= *hy) return(1);
			return(0);
		}
		if (hye == biggestext)
		{
			*hy = (*hy + by) / 2;
			if (*hy <= *ly) return(1);
			return(0);
		}
	}

	/* reduce (lx-hx,ly-hy) by (bx-ux,by-uy) */
	crops = FALSE;
	if (bx <= *lx && ux >= *hx)
	{
		/* it covers in X...crop in Y */
		if (uy >= *hy) *hy = (*hy + by) / 2;
		if (by <= *ly) *ly = (*ly + uy) / 2;
		crops = TRUE;
	}
	if (by <= *ly && uy >= *hy)
	{
		/* it covers in Y...crop in X */
		if (ux >= *hx) *hx = (*hx + bx) / 2;
		if (bx <= *lx) *lx = (*lx + ux) / 2;
		crops = TRUE;
	}
	if (!crops) return(-1);
	return(0);
}

/*
 * routine to crop the box on layer "nlayer", electrical index "nnet"
 * and bounds (lx-hx, ly-hy) against the nodeinst "ni".  The geometry in nodeinst "ni"
 * that is being checked runs from (nlx-nhx, nly-nhy).  Only those layers
 * in the nodeinst that are the same layer and the same electrical network
 * are checked.  The routine returns true if the bounds are reduced
 * to nothing.
 */
BOOLEAN dr_quickcropnodeinst(NODEINST *ni, INTBIG globalindex, CHECKSTATE *state,
	XARRAY trans, INTBIG nlx, INTBIG nhx, INTBIG nly, INTBIG nhy,
	INTBIG nlayer, INTBIG nnet, GEOM *ngeom, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy)
{
	INTBIG xl, xh, yl, yh;
	REGISTER INTBIG tot, j, isconnected, net;
	REGISTER BOOLEAN allgone;
	REGISTER INTBIG temp;
	REGISTER POLYGON *poly;

	dr_quickgetnodeEpolys(ni, state->cropnodepolylist, trans);
	tot = state->cropnodepolylist->polylistcount;
	if (tot < 0) return(FALSE);
	isconnected = 0;
	for(j=0; j<tot; j++)
	{
		poly = state->cropnodepolylist->polygons[j];
		if (!samelayer(poly->tech, poly->layer, nlayer)) continue;
		if (nnet != NONET)
		{
			if (poly->portproto == NOPORTPROTO) continue;

			/* determine network for this polygon */
			net = dr_quickgetnetnumber(poly->portproto, ni, globalindex);
			if (net != NONET && net != nnet) continue;
		}
		isconnected++;
		break;
	}
	if (isconnected == 0) return(FALSE);

	/* get the description of the nodeinst layers */
	allgone = FALSE;
	dr_quickgetnodepolys(ni, state->cropnodepolylist, trans);
	tot = state->cropnodepolylist->polylistcount;
	if (tot < 0) return(FALSE);

	if (gettrace(ni) != NOVARIABLE)
	{
		/* node is defined with an outline: use advanced methods to crop */
		void *merge;

		merge = mergenew(dr_tool->cluster);
		makerectpoly(*lx, *hx, *ly, *hy, state->checkdistpoly1rebuild);
		mergeaddpolygon(merge, nlayer, el_curtech, state->checkdistpoly1rebuild);
		for(j=0; j<tot; j++)
		{
			poly = state->cropnodepolylist->polygons[j];
			if (!samelayer(poly->tech, poly->layer, nlayer)) continue;
			mergesubpolygon(merge, nlayer, el_curtech, poly);
		}
		if (!mergebbox(merge, lx, hx, ly, hy))
			allgone = TRUE;
		mergedelete(merge);
	} else
	{
		for(j=0; j<tot; j++)
		{
			poly = state->cropnodepolylist->polygons[j];
			if (!samelayer(poly->tech, poly->layer, nlayer)) continue;

			/* warning: does not handle arbitrary polygons, only boxes */
			if (!isbox(poly, &xl, &xh, &yl, &yh)) continue;
			temp = dr_quickcropbox(lx, hx, ly, hy, xl, xh, yl, yh, nlx, nhx, nly, nhy);
			if (temp > 0) { allgone = TRUE; break; }
			if (temp < 0)
			{
				state->tinynodeinst = ni;
				state->tinyobj = ngeom;
			}
		}
	}
	return(allgone);
}

/*
 * routine to crop away any part of layer "lay" of arcinst "ai" that coincides
 * with a similar layer on a connecting nodeinst.  The bounds of the arcinst
 * are in the reference parameters (lx-hx, ly-hy).  The routine returns false
 * normally, 1 if the arcinst is cropped into oblivion.
 */
BOOLEAN dr_quickcroparcinst(ARCINST *ai, INTBIG lay, XARRAY transin, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy, CHECKSTATE *state)
{
	INTBIG xl, xh, yl, yh;
	XARRAY trans, ttrans, rtrans, xtemp;
	REGISTER INTBIG i, j, tot;
	REGISTER INTBIG temp;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
	REGISTER POLYGON *poly;

	for(i=0; i<2; i++)
	{
		/* find the primitive nodeinst at the true end of the portinst */
		ni = ai->end[i].nodeinst;   np = ni->proto;
		pp = ai->end[i].portarcinst->proto;
		makerot(ni, xtemp);
		transmult(xtemp, transin, trans);
		while (np->primindex == 0)
		{
			maketrans(ni, ttrans);
			transmult(ttrans, trans, xtemp);
			ni = pp->subnodeinst;   np = ni->proto;
			pp = pp->subportproto;
			makerot(ni, rtrans);
			transmult(rtrans, xtemp, trans);
		}
		dr_quickgetnodepolys(ni, state->croparcpolylist, trans);
		tot = state->croparcpolylist->polylistcount;
		for(j=0; j<tot; j++)
		{
			poly = state->croparcpolylist->polygons[j];
			if (!samelayer(poly->tech, poly->layer, lay)) continue;

			/* warning: does not handle arbitrary polygons, only boxes */
			if (!isbox(poly, &xl, &xh, &yl, &yh)) continue;
			temp = dr_quickhalfcropbox(lx, hx, ly, hy, xl, xh, yl, yh);
			if (temp > 0) return(TRUE);
			if (temp < 0)
			{
				state->tinynodeinst = ni;
				state->tinyobj = ai->geom;
			}
		}
	}
	return(FALSE);
}

/*
 * routine to see if polygons in "plist" (describing arc "ai") should be cropped against a
 * connecting transistor.  Crops the polygon if so.
 */
void dr_quickcropactivearc(ARCINST *ai, POLYLIST *plist, CHECKSTATE *state)
{
	REGISTER INTBIG i, j, k, fun, tot, ntot;
	INTBIG lx, hx, ly, hy, nlx, nhx, nly, nhy;
	REGISTER NODEINST *ni;
	REGISTER POLYGON *poly, *npoly, *swappoly;
	REGISTER BOOLEAN cropped;
	XARRAY trans;

	/* look for an active layer in this arc */
	tot = plist->polylistcount;
	for(j=0; j<tot; j++)
	{
		poly = plist->polygons[j];
		fun = layerfunction(poly->tech, poly->layer);
		if ((fun&LFTYPE) == LFDIFF) break;
	}
	if (j >= tot) return;

	/* must be manhattan */
	if (!isbox(poly, &lx, &hx, &ly, &hy)) return;

	/* search for adjoining transistor in the cell */
	cropped = FALSE;
	for(i=0; i<2; i++)
	{
		ni = ai->end[i].nodeinst;
		if (!isfet(ni->geom)) continue;

		/* crop the arc against this transistor */
		makerot(ni, trans);
		dr_quickgetnodepolys(ni, state->activecroppolylist, trans);
		ntot = state->activecroppolylist->polylistcount;
		for(k=0; k<ntot; k++)
		{
			npoly = state->activecroppolylist->polygons[k];
			if (npoly->tech != poly->tech || npoly->layer != poly->layer) continue;
			if (!isbox(npoly, &nlx, &nhx, &nly, &nhy)) continue;
			if (dr_quickhalfcropbox(&lx, &hx, &ly, &hy, nlx, nhx, nly, nhy) == 1)
			{
				/* remove this polygon from consideration */
				swappoly = plist->polygons[j];
				plist->polygons[j] = plist->polygons[tot-1];
				plist->polygons[tot-1] = swappoly;
				plist->polylistcount--;
				return;
			}
			cropped = TRUE;
		}
	}
	if (cropped) makerectpoly(lx, hx, ly, hy, poly);
}

/*
 * These routines get all the polygons in a primitive instance, and
 * store them in the POLYLIST structure.
 */
void dr_quickgetnodeEpolys(NODEINST *ni, POLYLIST *plist, XARRAY trans)
{
	REGISTER INTBIG j;
	BOOLEAN convertpseudo, onlyreasonable;
	REGISTER POLYGON *poly;

	convertpseudo = FALSE;
	if (((ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH) == NPPIN)
	{
		if (ni->firstportarcinst == NOPORTARCINST &&
			ni->firstportexpinst != NOPORTEXPINST)
				convertpseudo = TRUE;
	}
	if ((dr_quickoptions&DRCREASONABLE) != 0) onlyreasonable = TRUE; else
		onlyreasonable = FALSE;
	plist->polylistcount = allnodeEpolys(ni, plist, NOWINDOWPART, onlyreasonable);
	for(j = 0; j < plist->polylistcount; j++)
	{
		poly = plist->polygons[j];
		xformpoly(poly, trans);
		getbbox(poly, &plist->lx[j], &plist->hx[j], &plist->ly[j], &plist->hy[j]);
		if (convertpseudo)
			poly->layer = nonpseudolayer(poly->layer, poly->tech);
	}
}

void dr_quickgetnodepolys(NODEINST *ni, POLYLIST *plist, XARRAY trans)
{
	REGISTER INTBIG j;
	REGISTER POLYGON *poly;
	BOOLEAN onlyreasonable;

	if ((dr_quickoptions&DRCREASONABLE) != 0) onlyreasonable = TRUE; else
		onlyreasonable = FALSE;
	plist->polylistcount = allnodepolys(ni, plist, NOWINDOWPART, onlyreasonable);
	for(j = 0; j < plist->polylistcount; j++)
	{
		poly = plist->polygons[j];
		xformpoly(poly, trans);
		getbbox(poly, &plist->lx[j], &plist->hx[j], &plist->ly[j], &plist->hy[j]);
	}
}

void dr_quickgetarcpolys(ARCINST *ai, POLYLIST *plist, XARRAY trans)
{
	REGISTER INTBIG j;
	REGISTER POLYGON *poly;

	plist->polylistcount = allarcpolys(ai, plist, NOWINDOWPART);
	for(j = 0; j < plist->polylistcount; j++)
	{
		poly = plist->polygons[j];
		xformpoly(poly, trans);
		getbbox(poly, &plist->lx[j], &plist->hx[j], &plist->ly[j], &plist->hy[j]);
	}
}

void dr_cachevalidlayers(TECHNOLOGY *tech)
{
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER ARCPROTO *ap;
	REGISTER ARCINST *ai;
	REGISTER INTBIG i, tot;
	NODEINST node;
	ARCINST arc;
	REGISTER POLYGON *poly;

	if (tech == NOTECHNOLOGY) return;
	if (dr_curtech == tech) return;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	dr_curtech = tech;

	/* determine the layers that are being used */
	if (tech->layercount > dr_layertotal)
	{
		if (dr_layertotal > 0)
			efree((CHAR *)dr_layersvalid);
		dr_layertotal = 0;
		dr_layersvalid = (BOOLEAN *)emalloc(tech->layercount * (sizeof (BOOLEAN)), dr_tool->cluster);
		dr_layertotal = tech->layercount;
	}
	for(i=0; i < tech->layercount; i++)
		dr_layersvalid[i] = FALSE;
	poly = allocpolygon(4, dr_tool->cluster);
	for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if ((np->userbits&NNOTUSED) != 0) continue;
		ni = &node;   initdummynode(ni);
		ni->proto = np;
		ni->lowx = np->lowx;
		ni->highx = np->highx;
		ni->lowy = np->lowy;
		ni->highy = np->highy;
		tot = nodepolys(ni, 0, NOWINDOWPART);
		for(i=0; i<tot; i++)
		{
			shapenodepoly(ni, i, poly);
			if (poly->tech != tech) continue;
			dr_layersvalid[poly->layer] = TRUE;
		}
	}
	for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
	{
		if ((ap->userbits&ANOTUSED) != 0) continue;
		ai = &arc;   initdummyarc(ai);
		ai->proto = ap;
		ai->end[0].xpos = 0;   ai->end[0].ypos = 0;
		ai->end[1].xpos = 10000;  ai->end[0].ypos = 0;
		ai->width = ap->nominalwidth;
		tot = arcpolys(ai, NOWINDOWPART);
		for(i=0; i<tot; i++)
		{
			shapearcpoly(ai, i, poly);
			if (poly->tech != tech) continue;
			dr_layersvalid[poly->layer] = TRUE;
		}
	}
	freepolygon(poly);
}

/*
 * Routine to determine the minimum distance between "layer1" and "layer2" in technology
 * "tech" and library "lib".  If "con" is true, the layers are connected.  Also forces
 * connectivity for same-implant layers.
 */
INTBIG dr_adjustedmindist(TECHNOLOGY *tech, LIBRARY *lib, INTBIG layer1, INTBIG size1,
	INTBIG layer2, INTBIG size2, BOOLEAN con, BOOLEAN multi, INTBIG *edge, CHAR **rule)
{
	INTBIG dist, fun;

	/* if they are implant on the same layer, they connect */
	if (!con && layer1 == layer2)
	{
		fun = layerfunction(tech, layer1) & LFTYPE;
		/* treat all wells as connected */
		if (!con)
		{
			if (fun == LFSUBSTRATE || fun == LFWELL || fun == LFIMPLANT) con = TRUE;
		}
	}

	/* see how close they can get */
	dist = drcmindistance(tech, lib, layer1, size1, layer2, size2, con, multi, edge, rule);
	return(dist);
}

/*
 * Routine to return the network number for port "pp" on node "ni", given that the node is
 * in a cell with global index "globalindex".
 */
INTBIG dr_quickgetnetnumber(PORTPROTO *pp, NODEINST *ni, INTBIG globalindex)
{
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER INTBIG netnumber;

	if (pp == NOPORTPROTO) return(-1);

	/* see if there is an arc connected */
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		if (pi->proto->network == pp->network)
			return(((INTBIG *)pi->conarcinst->network->temp1)[globalindex]);

	/* see if there is an export connected */
	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		if (pe->proto->network == pp->network)
			return(((INTBIG *)pe->exportproto->network->temp1)[globalindex]);

	/* generate a new unique network number */
	netnumber = dr_quickcheckunconnetnumber++;
	if (dr_quickcheckunconnetnumber < dr_quickchecknetnumber)
		dr_quickcheckunconnetnumber = dr_quickchecknetnumber;
	return(netnumber);
}

/***************** LAYER INTERACTIONS ******************/

/*
 * Routine to build the internal data structures that tell which layers interact with
 * which primitive nodes in technology "tech".
 */
void dr_quickbuildlayerinteractions(TECHNOLOGY *tech)
{
	REGISTER INTBIG i, tot, index, tablesize, dist, layer;
	INTBIG edge;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER ARCPROTO *ap;
	REGISTER ARCINST *ai;
	NODEINST node;
	ARCINST arc;
	REGISTER POLYGON *poly;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	dr_quicklayerintertech = tech;

	/* build the node table */
	tablesize = pickprime(tech->nodeprotocount * 2);
	if (tablesize > dr_quicklayerinternodehashsize)
	{
		if (dr_quicklayerinternodehashsize > 0)
		{
			for(i=0; i<dr_quicklayerinternodehashsize; i++)
				if (dr_quicklayerinternodetable[i] != 0)
					efree((CHAR *)dr_quicklayerinternodetable[i]);
			efree((CHAR *)dr_quicklayerinternodetable);
			efree((CHAR *)dr_quicklayerinternodehash);
		}
		dr_quicklayerinternodehashsize = 0;
		dr_quicklayerinternodehash = (NODEPROTO **)emalloc(tablesize * (sizeof (NODEPROTO *)),
			dr_tool->cluster);
		if (dr_quicklayerinternodehash == 0) return;
		dr_quicklayerinternodetable = (BOOLEAN **)emalloc(tablesize * (sizeof (BOOLEAN *)),
			dr_tool->cluster);
		if (dr_quicklayerinternodetable == 0) return;
		dr_quicklayerinternodehashsize = tablesize;
	}
	for(i=0; i<dr_quicklayerinternodehashsize; i++)
	{
		dr_quicklayerinternodehash[i] = NONODEPROTO;
		dr_quicklayerinternodetable[i] = 0;
	}

	poly = allocpolygon(4, dr_tool->cluster);
	for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		index = abs(((INTBIG)np) % dr_quicklayerinternodehashsize);
		for(i=0; i<dr_quicklayerinternodehashsize; i++)
		{
			if (dr_quicklayerinternodehash[index] == NONODEPROTO) break;
			index++;
			if (index >= dr_quicklayerinternodehashsize) index = 0;
		}
		dr_quicklayerinternodehash[index] = np;
		dr_quicklayerinternodetable[index] = (BOOLEAN *)emalloc(tech->layercount * (sizeof (BOOLEAN)),
			dr_tool->cluster);
		for(i=0; i<tech->layercount; i++) dr_quicklayerinternodetable[index][i] = FALSE;

		/* fill in the layers that interact with this node */
		ni = &node;   initdummynode(ni);
		ni->proto = np;
		ni->lowx = np->lowx;
		ni->highx = np->highx;
		ni->lowy = np->lowy;
		ni->highy = np->highy;
		tot = nodepolys(ni, 0, NOWINDOWPART);
		for(i=0; i<tot; i++)
		{
			shapenodepoly(ni, i, poly);
			if (poly->tech != tech) continue;
			for(layer = 0; layer < tech->layercount; layer++)
			{
				dist = drcmindistance(tech, el_curlib, layer, K1, poly->layer, K1,
					FALSE, FALSE, &edge, 0);
				if (dist < 0) continue;
				dr_quicklayerinternodetable[index][layer] = TRUE;
			}
		}
	}

	/* build the arc table */
	tablesize = pickprime(tech->arcprotocount * 2);
	if (tablesize > dr_quicklayerinterarchashsize)
	{
		if (dr_quicklayerinterarchashsize > 0)
		{
			for(i=0; i<dr_quicklayerinterarchashsize; i++)
				if (dr_quicklayerinterarctable[i] != 0)
					efree((CHAR *)dr_quicklayerinterarctable[i]);
			efree((CHAR *)dr_quicklayerinterarctable);
			efree((CHAR *)dr_quicklayerinterarchash);
		}
		dr_quicklayerinterarchashsize = 0;
		dr_quicklayerinterarchash = (ARCPROTO **)emalloc(tablesize * (sizeof (ARCPROTO *)),
			dr_tool->cluster);
		if (dr_quicklayerinterarchash == 0) return;
		dr_quicklayerinterarctable = (BOOLEAN **)emalloc(tablesize * (sizeof (BOOLEAN *)),
			dr_tool->cluster);
		if (dr_quicklayerinterarctable == 0) return;
		dr_quicklayerinterarchashsize = tablesize;
	}
	for(i=0; i<dr_quicklayerinterarchashsize; i++)
	{
		dr_quicklayerinterarchash[i] = NOARCPROTO;
		dr_quicklayerinterarctable[i] = 0;
	}

	for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
	{
		index = abs(((INTBIG)ap) % dr_quicklayerinterarchashsize);
		for(i=0; i<dr_quicklayerinterarchashsize; i++)
		{
			if (dr_quicklayerinterarchash[index] == NOARCPROTO) break;
			index++;
			if (index >= dr_quicklayerinterarchashsize) index = 0;
		}
		dr_quicklayerinterarchash[index] = ap;
		dr_quicklayerinterarctable[index] = (BOOLEAN *)emalloc(tech->layercount * (sizeof (BOOLEAN)),
			dr_tool->cluster);
		for(i=0; i<tech->layercount; i++) dr_quicklayerinterarctable[index][i] = FALSE;

		/* fill in the layers that interact with this arc */
		ai = &arc;   initdummyarc(ai);
		ai->proto = ap;
		ai->end[0].xpos = 0;   ai->end[0].ypos = 0;
		ai->end[1].xpos = 10000;  ai->end[0].ypos = 0;
		ai->width = ap->nominalwidth;
		tot = arcpolys(ai, NOWINDOWPART);
		for(i=0; i<tot; i++)
		{
			shapearcpoly(ai, i, poly);
			if (poly->tech != tech) continue;
			for(layer = 0; layer < tech->layercount; layer++)
			{
				dist = drcmindistance(tech, el_curlib, layer, K1, poly->layer, K1,
					FALSE, FALSE, &edge, 0);
				if (dist < 0) continue;
				dr_quicklayerinterarctable[index][layer] = TRUE;
			}
		}
	}
	freepolygon(poly);
}

/*
 * Routine to determine whether layer "layer" interacts in any way with a node of type "np".
 * If not, returns FALSE.
 */
BOOLEAN dr_quickchecklayerwithnode(INTBIG layer, NODEPROTO *np)
{
	REGISTER INTBIG index, i;

	if (np->primindex == 0) return(FALSE);
	if (np->tech != dr_quicklayerintertech)
	{
		dr_quickbuildlayerinteractions(np->tech);
	}
	if (dr_quicklayerinternodehashsize <= 0) return(TRUE);

	/* find this node in the table */
	index = abs(((INTBIG)np) % dr_quicklayerinternodehashsize);
	for(i=0; i<dr_quicklayerinternodehashsize; i++)
	{
		if (dr_quicklayerinternodehash[index] == np) break;
		index++;
		if (index >= dr_quicklayerinternodehashsize) index = 0;
	}
	if (i >= dr_quicklayerinternodehashsize) return(FALSE);
	return(dr_quicklayerinternodetable[index][layer]);
}

/*
 * Routine to determine whether layer "layer" interacts in any way with an arc of type "ap".
 * If not, returns FALSE.
 */
BOOLEAN dr_quickchecklayerwitharc(INTBIG layer, ARCPROTO *ap)
{
	REGISTER INTBIG index, i;

	if (ap->tech != dr_quicklayerintertech)
	{
		dr_quickbuildlayerinteractions(ap->tech);
	}
	if (dr_quicklayerinterarchashsize <= 0) return(TRUE);

	/* find this arc in the table */
	index = abs(((INTBIG)ap) % dr_quicklayerinterarchashsize);
	for(i=0; i<dr_quicklayerinterarchashsize; i++)
	{
		if (dr_quicklayerinterarchash[index] == ap) break;
		index++;
		if (index >= dr_quicklayerinterarchashsize) index = 0;
	}
	if (i >= dr_quicklayerinterarchashsize) return(FALSE);
	return(dr_quicklayerinterarctable[index][layer]);
}

/*
 * Routine to recursively scan cell "cell" (transformed with "trans") searching
 * for DRC Exclusion nodes.  Each node is added to the global list "dr_quickexclusionlist".
 */
void dr_quickaccumulateexclusion(INTBIG depth, NODEPROTO **celllist, XARRAY *translist)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *cell;
	REGISTER INTBIG i, j, k, newtotal;
	REGISTER POLYGON *poly;
	REGISTER DRCEXCLUSION *newlist;
	XARRAY transr, transt, transtemp;

	cell = celllist[depth-1];
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto == gen_drcprim)
		{
			/* create a DRC exclusion for every cell up the hierarchy */
			for(j=0; j<depth; j++)
			{
				if (j != depth-1) continue;  /* exlusion should only be for the current hierarchy */

				/* make sure there is a place to put the polygon */
				if (dr_quickexclusioncount >= dr_quickexclusiontotal)
				{
					newtotal = dr_quickexclusiontotal * 2;
					if (newtotal <= dr_quickexclusioncount) newtotal = dr_quickexclusioncount + 10;
					newlist = (DRCEXCLUSION *)emalloc(newtotal * (sizeof (DRCEXCLUSION)), dr_tool->cluster);
					if (newlist == 0) break;
					for(i=0; i<dr_quickexclusiontotal; i++)
						newlist[i] = dr_quickexclusionlist[i];
					for(i=dr_quickexclusiontotal; i<newtotal; i++)
						newlist[i].poly = allocpolygon(4, dr_tool->cluster);
					if (dr_quickexclusiontotal > 0) efree((CHAR *)dr_quickexclusionlist);
					dr_quickexclusionlist = newlist;
					dr_quickexclusiontotal = newtotal;
				}

				/* extract the information about this DRC exclusion node */
				poly = dr_quickexclusionlist[dr_quickexclusioncount].poly;
				dr_quickexclusionlist[dr_quickexclusioncount].cell = celllist[j];
				(void)nodepolys(ni, 0, NOWINDOWPART);
				shapenodepoly(ni, 0, poly);
				makerot(ni, transr);
				xformpoly(poly, transr);
				for(k=depth-1; k>j; k--)
					xformpoly(poly, translist[k]);
				getbbox(poly, &dr_quickexclusionlist[dr_quickexclusioncount].lx,
					&dr_quickexclusionlist[dr_quickexclusioncount].hx,
					&dr_quickexclusionlist[dr_quickexclusioncount].ly,
					&dr_quickexclusionlist[dr_quickexclusioncount].hy);

				/* see if it is already in the list */
				for(i=0; i<dr_quickexclusioncount; i++)
				{
					if (dr_quickexclusionlist[i].cell != celllist[j]) continue;
					if (dr_quickexclusionlist[i].lx != dr_quickexclusionlist[dr_quickexclusioncount].lx ||
						dr_quickexclusionlist[i].hx != dr_quickexclusionlist[dr_quickexclusioncount].hx ||
						dr_quickexclusionlist[i].ly != dr_quickexclusionlist[dr_quickexclusioncount].ly ||
						dr_quickexclusionlist[i].hy != dr_quickexclusionlist[dr_quickexclusioncount].hy)
							continue;
					break;
				}
				if (i >= dr_quickexclusioncount)
					dr_quickexclusioncount++;
			}
			continue;
		}

		if (ni->proto->primindex == 0)
		{
			/* examine contents */
			maketrans(ni, transt);
			makerot(ni, transr);
			transmult(transt, transr, transtemp);

			if (depth >= MAXHIERARCHYDEPTH)
			{
				if (!dr_quickexclusionwarned)
					ttyputerr(_("Depth of circuit exceeds %ld: unable to locate all DRC exclusion areas"),
						MAXHIERARCHYDEPTH);
				dr_quickexclusionwarned = TRUE;
				continue;
			}
			celllist[depth] = ni->proto;
			transcpy(transtemp, translist[depth]);
			dr_quickaccumulateexclusion(depth+1, celllist, translist);
		}
	}
}

/*********************************** QUICK DRC ERROR REPORTING ***********************************/

/* Adds details about an error to the error list */
void dr_quickreporterror(INTBIG errtype, TECHNOLOGY *tech, CHAR *msg,
	NODEPROTO *cell, INTBIG limit, INTBIG actual, CHAR *rule,
	POLYGON *poly1, GEOM *geom1, INTBIG layer1, INTBIG net1,
	POLYGON *poly2, GEOM *geom2, INTBIG layer2, INTBIG net2)
{
	REGISTER NODEPROTO *np1, *np2;
	REGISTER VARIABLE *var;
	REGISTER INTBIG i, len, sortlayer, lambda;
	REGISTER BOOLEAN showgeom;
	REGISTER GEOM *p1, *p2;
	REGISTER void *err, *infstr;
	REGISTER CHAR *errmsg;
	REGISTER POLYGON *poly;
	INTBIG lx, hx, ly, hy, lx2, hx2, ly2, hy2;

	/* if this error is being ignored, don't record it */
	var = getvalkey((INTBIG)cell, VNODEPROTO, VGEOM|VISARRAY, dr_ignore_listkey);
	if (var != NOVARIABLE)
	{
		len = getlength(var);
		for(i=0; i<len; i += 2)
		{
			p1 = ((GEOM **)var->addr)[i];
			p2 = ((GEOM **)var->addr)[i+1];
			if (p1 == geom1 && p2 == geom2) return;
			if (p1 == geom2 && p2 == geom1) return;
		}
	}

	/* if this error is in an ignored area, don't record it */
	if (dr_quickexclusioncount > 0)
	{
		/* determine the bounding box of the error */
		getbbox(poly1, &lx, &hx, &ly, &hy);
		if (poly2 != NOPOLYGON)
		{
			getbbox(poly2, &lx2, &hx2, &ly2, &hy2);
			if (lx2 < lx) lx = lx2;
			if (hx2 > hx) hx = hx2;
			if (ly2 < ly) ly = ly2;
			if (hy2 > hy) hy = hy2;
		}
		for(i=0; i<dr_quickexclusioncount; i++)
		{
			if (cell != dr_quickexclusionlist[i].cell) continue;
			poly = dr_quickexclusionlist[i].poly;
			if (isinside(lx, ly, poly) && isinside(lx, hy, poly) &&
				isinside(hx, ly, poly) && isinside(hx, hy, poly)) return;
		}
	}

	lambda = lambdaofcell(cell);

/* BEGIN critical section */
if (dr_quickparalleldrc) emutexlock(dr_quickmutexio);

	/* describe the error */
	infstr = initinfstr();
	np1 = geomparent(geom1);
	if (errtype == SPACINGERROR || errtype == NOTCHERROR)
	{
		/* describe spacing width error */
		if (errtype == SPACINGERROR) addstringtoinfstr(infstr, _("Spacing")); else
			addstringtoinfstr(infstr, _("Notch"));
		if (layer1 == layer2)
			formatinfstr(infstr, _(" (layer %s)"), layername(tech, layer1));
		addstringtoinfstr(infstr, x_(": "));
		np2 = geomparent(geom2);
		if (np1 != np2)
		{
			formatinfstr(infstr, _("cell %s, "), describenodeproto(np1));
		} else if (np1 != cell)
		{
			formatinfstr(infstr, _("[in cell %s] "), describenodeproto(np1));
		}
		if (geom1->entryisnode)
			formatinfstr(infstr, _("node %s"), describenodeinst(geom1->entryaddr.ni)); else
				formatinfstr(infstr, _("arc %s"), describearcinst(geom1->entryaddr.ai));
		if (layer1 != layer2)
			formatinfstr(infstr, _(", layer %s"), layername(tech, layer1));

		if (actual < 0) addstringtoinfstr(infstr, _(" OVERLAPS ")); else
			if (actual == 0) addstringtoinfstr(infstr, _(" TOUCHES ")); else
				formatinfstr(infstr, _(" LESS (BY %s) THAN %s TO "), latoa(limit-actual, lambda),
					latoa(limit, lambda));

		if (np1 != np2)
			formatinfstr(infstr, _("cell %s, "), describenodeproto(np2));
		if (geom2->entryisnode)
			formatinfstr(infstr, _("node %s"), describenodeinst(geom2->entryaddr.ni)); else
				formatinfstr(infstr, _("arc %s"), describearcinst(geom2->entryaddr.ai));
		if (layer1 != layer2)
			formatinfstr(infstr, _(", layer %s"), layername(tech, layer2));
		if (msg != NULL)
		{
			addstringtoinfstr(infstr, x_("; "));
			addstringtoinfstr(infstr, msg);
		}
		sortlayer = mini(layer1, layer2);
	} else
	{
		/* describe minimum width/size or layer error */
		switch (errtype)
		{
			case MINWIDTHERROR:
				addstringtoinfstr(infstr, _("Minimum width error:"));
				break;
			case MINSIZEERROR:
				addstringtoinfstr(infstr, _("Minimum size error:"));
				break;
			case BADLAYERERROR:
				formatinfstr(infstr, _("Invalid layer (%s):"), layername(tech, layer1));
				break;
			case LAYERSURROUNDERROR:
				addstringtoinfstr(infstr, _("Layer surround error:"));
				break;
		}
		formatinfstr(infstr, _(" cell %s"), describenodeproto(np1));
		if (geom1->entryisnode)
		{
			formatinfstr(infstr, _(", node %s"), describenodeinst(geom1->entryaddr.ni));
		} else
		{
			formatinfstr(infstr, _(", arc %s"), describearcinst(geom1->entryaddr.ai));
		}
		if (errtype == MINWIDTHERROR)
		{
			formatinfstr(infstr, _(", layer %s"), layername(tech, layer1));
			formatinfstr(infstr, _(" LESS THAN %s WIDE (IS %s)"),
				latoa(limit, lambda), latoa(actual, lambda));
		} else if (errtype == MINSIZEERROR)
		{
			formatinfstr(infstr, _(" LESS THAN %s IN SIZE (IS %s)"),
				latoa(limit, lambda), latoa(actual, lambda));
		} else if (errtype == LAYERSURROUNDERROR)
		{
			formatinfstr(infstr, _(", layer %s"), layername(tech, layer1));
			formatinfstr(infstr, _(" NEEDS SURROUND OF LAYER %s BY %s"),
				layername(tech, layer2), latoa(limit, lambda));
		}
		sortlayer = layer1;
	}
	if (rule != 0) formatinfstr(infstr, _(" [rule %s]"), rule);
	errmsg = returninfstr(infstr);
	if (dr_logerrors)
	{
		err = logerror(errmsg, cell, sortlayer);
		showgeom = TRUE;
		if (poly1 != NOPOLYGON) { showgeom = FALSE;   addpolytoerror(err, poly1); }
		if (poly2 != NOPOLYGON) { showgeom = FALSE;   addpolytoerror(err, poly2); }
		addgeomtoerror(err, geom1, showgeom, 0, 0);
		if (geom2 != NOGEOM) addgeomtoerror(err, geom2, showgeom, 0, 0);
	} else
	{
		ttyputerr(x_("%s"), errmsg);
	}

/* END critical section */
if (dr_quickparalleldrc) emutexunlock(dr_quickmutexio);
}

#endif  /* DRCTOOL - at top */
