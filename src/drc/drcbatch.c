/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: drcbatch.c
 * Hierarchical design-rule check tool
 * Written by: Steven M. Rubin
 * Inspired by: Mark Moraes, University of Toronto
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
#if DRCTOOL

#include "global.h"
#include "efunction.h"
#include "drc.h"
#include "database.h"
#include "tech.h"
#include "egraphics.h"
#include "usr.h"

/*
 * Notes on parallel DRC:
 *
 * This DRC will not work if there are different technologies involved, because the
 * cached tables of layer interactions ("drcb_layerintertech", etc) are globals.
 *
 * Because text processing routines (describenodeproto, describenodeinst, etc.) are
 * not parallelizable, they must be enclosed in a mutex.
 */

/*********** HASH TABLE ***********/

/*
 * Initial sizes for various hashtables.  This is chosen so it will not
 * have to grow the hashtable for the common case, only for cases with a
 * lot more entries than normal
 */
#define HTABLE_NODEARC     71
#define HTABLE_PORT       113

#define HASH_FULL          -2
#define HASH_EXISTS        -1

#define LISTSIZECACHE 100
static CHAR **drcb_listcache[LISTSIZECACHE];
static INTBIG drcb_listcacheinited = 0;

#define NOHASHTABLE ((HASHTABLE *)-1)

typedef struct
{
	CHAR **thelist;		/* override of hashing */
	INTBIG thetype;		/* override of hashing (1:port, 2:node, 3:arc) */
	INTBIG thelen;		/* override of hashing */
} HASHTABLE;

/*
 * The Hash functions consider key to be an arbitrary pointer --
 * the hash function is simply a modulo of the hash table size.
 * drcb_hashinsert will cause the table to grow if it isn't large enough.
 */
static HASHTABLE  *drcb_freehashtables = NOHASHTABLE;
static HASHTABLE  *drcb_incrementalhpp = NOHASHTABLE;
static HASHTABLE  *drcb_incrementalhai = NOHASHTABLE;
static HASHTABLE  *drcb_incrementalhni = NOHASHTABLE;

/*********** POLYGONS AND SHAPES ***********/

#define NOSHAPE ((SHAPE *)-1)

/*
 * temp1 in a NODEPROTO is non-zero if the proto has been checked, zero
 * if it has not been checked yet. temp1 in the intersect lists (stored
 * in a dummy NODEPROTO structure) has a count of the number of
 * intersected elements.
 */
typedef struct Ishape
{
	INTBIG layer;
	BOOLEAN entryisnode;
	union
	{
		NODEINST *ni;
		ARCINST  *ai;
	} entryaddr;
	INTBIG     net;
	HASHTABLE *hpp;
	XARRAY     trans;
	POLYGON   *poly;
} SHAPE;

/*********** MISCELLANEOUS ***********/

#define SPACINGERROR  1
#define MINWIDTHERROR 2
#define NOTCHERROR    3
#define MINSIZEERROR  4
#define BADLAYERERROR 5

#define NONET ((INTBIG)-1)

/*
 * The STATE structure contains all the global/static data that used to
 * be scattered around in the DRC procedures. We need one STATE per
 * processor, and all procedures with static writable data of any form
 * must get it from here, so they must be passed this structure. The
 * ONLY safe alternative to this is to lock on every access to those
 * structures. Multiprocessors are such fun...
 */
typedef struct
{
	/* used by checkarcinst, checknodeinst, drcb_getnodeinstshapes, drcb_getarcinstshapes */
	POLYLIST  *polylist;

	/* used by badbox, which is called by checkarcinst, checknodeinst, drcb_checkshape */
	POLYLIST  *subpolylist;

	/* used by drcb_cropactivearc */
	POLYLIST  *activecroppolylist;

	/* used by "drcb_checkshape()" */
	POLYGON   *checkshapepoly;

	/* used by "drcb_lookforlayer()" */
	POLYLIST  *lookforlayerpolylist;

	/* used by "drcb_checkdist()" */
	POLYGON   *checkdistpoly1rebuild, *checkdistpoly2rebuild;
	POLYGON   *checkdistpoly1orig, *checkdistpoly2orig;

	/*
	 * cropnodeinst and croparcinst can share one polylist, because they
	 * don't call each other.  These two are called by checkdist, which
	 * is called by badbox.
	 */
	POLYLIST  *croppolylist;
	NODEINST  *tinynodeinst;
	GEOM      *tinyobj;
	INTBIG     netindex;
	HASHTABLE *hni, *hai;
	HASHTABLE *nhpp;

	/* for allocation */
	SHAPE     *freeshapes;

	/* the cell being examined */
	NODEPROTO *topcell;

	/* used to pass initial parameters to the thread */
	XARRAY     paramtrans;
	HASHTABLE *paramhpp, *paramhai;
	NODEPROTO *paramintersect_list;
} STATE;

typedef struct
{
	STATE     *state;
	NODEINST  *ni;
	XARRAY     vtrans;
	XARRAY     dtrans;
	NODEPROTO *intersectlist;
} CHECKSHAPEARG;

/* for parallel DRC */
static BOOLEAN     drcb_paralleldrc;			/* true if doing DRC on multiple processes */
static INTBIG      drcb_numprocesses;			/* number of processes for doing DRC */
static INTBIG      drcb_statecount = 0;			/* number of per-process state blocks */
static STATE     **drcb_state;					/* One for each process */
static INTBIG      drcb_mainthread;				/* The thread number of the main thread */
static void      **drcb_processdone;			/* lock to tell when process is done */
static void       *drcb_mutexhash = 0;			/* for locking hash table allocation */
static void       *drcb_mutexnode = 0;			/* for locking node distribution */
static void       *drcb_mutexio = 0;			/* for locking I/O */

/* for tracking which layers interact with which nodes */
static TECHNOLOGY *drcb_layerintertech = NOTECHNOLOGY;
static INTBIG      drcb_layerinternodehashsize = 0;
static NODEPROTO **drcb_layerinternodehash;
static BOOLEAN   **drcb_layerinternodetable;
static INTBIG      drcb_layerinterarchashsize = 0;
static ARCPROTO  **drcb_layerinterarchash;
static BOOLEAN   **drcb_layerinterarctable;

/* miscellaneous */
static INTBIG      drcb_options;				/* cached options */
static NODEPROTO  *drcb_topcellalways = NONODEPROTO;
static INTBIG      drcb_interactiondistance;	/* maximum DRC interaction distance */
static BOOLEAN     drcb_hierarchicalcheck;		/* true if checking hierarchically */

/*********** prototypes for local routines ***********/
static BOOLEAN    drcb_activeontransistor(CHECKSHAPEARG*, GEOM*, INTBIG, INTBIG, POLYGON*,
					GEOM*, INTBIG, INTBIG, POLYGON*, STATE*, TECHNOLOGY*);
static HASHTABLE *drcb_allochashtable(INTBIG len);
static SHAPE     *drcb_allocshape(STATE*);
static BOOLEAN    drcb_badbox(CHECKSHAPEARG*, STATE*, GEOM*, INTBIG, TECHNOLOGY*, NODEPROTO*, POLYGON*,
					INTBIG, BOOLEAN);
static void       drcb_box(NODEINST*, INTBIG*, INTBIG*, INTBIG*, INTBIG*);
static BOOLEAN    drcb_boxesintersect(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static INTBIG     drcb_checkallprotocontents(STATE*, NODEPROTO*, BOOLEAN, INTBIG, INTBIG, INTBIG, INTBIG);
static BOOLEAN    drcb_checkarcinst(STATE*, ARCINST*, BOOLEAN);
static BOOLEAN    drcb_checkdist(CHECKSHAPEARG*, STATE*, TECHNOLOGY*, 
					INTBIG, INTBIG, GEOM*, POLYGON*, HASHTABLE*,
					INTBIG, INTBIG, GEOM*, POLYGON*, HASHTABLE*,
					BOOLEAN, INTBIG, INTBIG, CHAR *, NODEPROTO*);
static INTBIG     drcb_checkintersections(STATE*, NODEINST*, NODEPROTO*, HASHTABLE*, HASHTABLE*, XARRAY);
static BOOLEAN    drcb_checkminwidth(STATE *state, GEOM *geom, INTBIG layer, POLYGON *poly, TECHNOLOGY *tech,
					BOOLEAN withincell);
static BOOLEAN    drcb_checknodeinst(STATE*, NODEINST*, HASHTABLE*, BOOLEAN);
static INTBIG     drcb_checknodeinteractions(STATE*, NODEINST*, XARRAY, HASHTABLE*, HASHTABLE*, NODEPROTO*, BOOLEAN,
					INTBIG, INTBIG, INTBIG, INTBIG);
static BOOLEAN    drcb_checkprotocontents(STATE*, NODEPROTO*, HASHTABLE*, HASHTABLE*, BOOLEAN, INTBIG, INTBIG, INTBIG, INTBIG);
static BOOLEAN    drcb_checkshape(GEOM*, CHECKSHAPEARG*);
static void       drcb_cropactivearc(STATE *state, ARCINST *ai, POLYLIST *plist);
static BOOLEAN    drcb_croparcinst(STATE*, ARCINST*, INTBIG, INTBIG*, INTBIG*, INTBIG*, INTBIG*);
static BOOLEAN    drcb_cropnodeinst(CHECKSHAPEARG*, STATE*, NODEINST*, XARRAY, HASHTABLE*, INTBIG,
					INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, GEOM*, INTBIG*, INTBIG*, INTBIG*, INTBIG*);
static NODEPROTO *drcb_findintersectingelements(STATE*, NODEINST*, INTBIG, INTBIG, INTBIG,
					INTBIG, XARRAY, HASHTABLE*, HASHTABLE*, NODEPROTO*);
static void       drcb_flatprop2(ARCINST*, HASHTABLE*, CHAR*);
static void       drcb_freehashtable(HASHTABLE *ht);
static CHAR      *drcb_freehpp(CHAR*, CHAR*, CHAR*);
static void       drcb_freeintersectingelements(NODEPROTO*, STATE*);
static void       drcb_freertree(RTNODE*, STATE*);
static void       drcb_freeshape(SHAPE *shape, STATE *state);
static BOOLEAN    drcb_getarcinstshapes(STATE*, NODEPROTO*, ARCINST*, XARRAY, INTBIG, INTBIG,
					INTBIG, INTBIG, HASHTABLE*);
static HASHTABLE *drcb_getarcnetworks(NODEPROTO*, HASHTABLE*, INTBIG*);
static void       drcb_getarcpolys(ARCINST*, POLYLIST*);
static HASHTABLE *drcb_getinitnets(NODEPROTO*, INTBIG*);
static void       drcb_getnodeEpolys(NODEINST*, POLYLIST*, XARRAY);
static BOOLEAN    drcb_getnodeinstshapes(STATE*, NODEPROTO*, NODEINST*, XARRAY, INTBIG, INTBIG,
					INTBIG, INTBIG, HASHTABLE*);
static void       drcb_getnodenetworks(NODEINST*, HASHTABLE*, HASHTABLE*, HASHTABLE*, INTBIG*);
static void       drcb_getnodepolys(NODEINST*, POLYLIST*, XARRAY);
static HASHTABLE *drcb_hashcopy(HASHTABLE*);
static HASHTABLE *drcb_hashcreate(INTBIG, INTBIG, NODEPROTO*);
static void       drcb_hashdestroy(HASHTABLE*);
static INTBIG     drcb_hashinsert(HASHTABLE*, CHAR*, CHAR*, INTBIG);
static CHAR      *drcb_hashsearch (HASHTABLE*, CHAR*);
static CHAR      *drcb_hashwalk(HASHTABLE*, CHAR*(*)(CHAR*, CHAR*, CHAR*), CHAR*);
static void       drcb_highlighterror(POLYGON *poly1, POLYGON *poly2, NODEPROTO *cell);
static BOOLEAN    drcb_init(NODEPROTO*);
static BOOLEAN    drcb_initonce(void);
static BOOLEAN    drcb_intersectsubtree(STATE*, NODEPROTO*, NODEPROTO*, XARRAY, XARRAY, INTBIG, INTBIG,
					INTBIG, INTBIG, HASHTABLE*);
static void       drcb_linkgeom(GEOM*, NODEPROTO*);
static BOOLEAN    drcb_lookforlayer(STATE *state, NODEPROTO *cell, INTBIG layer, XARRAY moretrans,
					INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy, INTBIG xf1, INTBIG yf1, BOOLEAN *p1found,
					INTBIG xf2, INTBIG yf2, BOOLEAN *p2found, INTBIG xf3, INTBIG yf3, BOOLEAN *p3found);
static BOOLEAN    drcb_lookforpoints(STATE *state, GEOM *geom1, GEOM *geom2, INTBIG layer, NODEPROTO *cell,
					TECHNOLOGY *tech, CHECKSHAPEARG *csap, INTBIG xf1, INTBIG yf1, INTBIG xf2, INTBIG yf2,
					BOOLEAN needboth);
static BOOLEAN    drcb_makestateblocks(INTBIG numstates);
static INTBIG     drcb_network(HASHTABLE*, CHAR*, INTBIG);
static void       drcb_reporterror(STATE*, INTBIG, TECHNOLOGY*, CHAR*, BOOLEAN, INTBIG, INTBIG, CHAR *,
					POLYGON*, GEOM*, INTBIG, INTBIG, POLYGON*, GEOM*, INTBIG, INTBIG);
static BOOLEAN    drcb_walkrtree(RTNODE*, BOOLEAN(*)(GEOM*, CHECKSHAPEARG*), CHECKSHAPEARG*);

static void       drcb_buildlayerinteractions(TECHNOLOGY *tech);
static BOOLEAN    drcb_checklayerwithnode(INTBIG layer, NODEPROTO *np);
static BOOLEAN    drcb_checklayerwitharc(INTBIG layer, ARCPROTO *ap);
static BOOLEAN    drcb_ismulticut(NODEINST *ni);
static BOOLEAN    drcb_objtouch(GEOM*, GEOM*);
static INTBIG     drcb_findinterveningpoints(POLYGON*, POLYGON*, INTBIG*, INTBIG*, INTBIG*, INTBIG*);
static INTBIG     drcb_halfcropbox(INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy,
					INTBIG bx, INTBIG ux, INTBIG by, INTBIG uy);
static INTBIG     drcb_cropbox(INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy, INTBIG bx, INTBIG ux, INTBIG by,
					INTBIG uy, INTBIG nlx, INTBIG nhx, INTBIG nly, INTBIG nhy);

/************************ DRC CONTROL ************************/

BOOLEAN drcb_initonce(void)
{
	REGISTER INTBIG i;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	if (drcb_listcacheinited == 0)
	{
		drcb_listcacheinited = 1;
		for(i=0; i<LISTSIZECACHE; i++)
			drcb_listcache[i] = 0;
	}

	if (drcb_makestateblocks(1)) return(TRUE);
	return(FALSE);
}

BOOLEAN drcb_makestateblocks(INTBIG numstates)
{
	REGISTER INTBIG i;
	REGISTER STATE **newstates;
	REGISTER void **newlocks;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	if (numstates <= drcb_statecount) return(FALSE);
	newstates = (STATE **)emalloc(numstates * (sizeof (STATE *)), dr_tool->cluster);
	if (newstates == 0) return(TRUE);
	newlocks = (void **)emalloc(numstates * (sizeof (void *)), dr_tool->cluster);
	if (newlocks == 0) return(TRUE);
	for(i=0; i<drcb_statecount; i++)
	{
		newstates[i] = drcb_state[i];
		newlocks[i] = drcb_processdone[i];
	}
	for(i=drcb_statecount; i<numstates; i++)
	{
		newstates[i] = (STATE *)emalloc(sizeof(STATE), dr_tool->cluster);
		if (newstates[i] == 0) return(TRUE);
		newstates[i]->polylist = allocpolylist(dr_tool->cluster);
		if (newstates[i]->polylist == 0) return(TRUE);

		newstates[i]->subpolylist = allocpolylist(dr_tool->cluster);
		if (newstates[i]->subpolylist == 0) return(TRUE);

		newstates[i]->activecroppolylist = allocpolylist(dr_tool->cluster);
		if (newstates[i]->activecroppolylist == 0) return(TRUE);

		newstates[i]->croppolylist = allocpolylist(dr_tool->cluster);
		if (newstates[i]->croppolylist == 0)
		{
			efree((CHAR *)newstates[i]->polylist);
			efree((CHAR *)newstates[i]->subpolylist);
			return(TRUE);
		}

		newstates[i]->checkshapepoly = NOPOLYGON;
		(void)needstaticpolygon(&newstates[i]->checkshapepoly, 4, dr_tool->cluster);

		newstates[i]->lookforlayerpolylist = allocpolylist(dr_tool->cluster);
		if (newstates[i]->lookforlayerpolylist == 0) return(TRUE);

		newstates[i]->checkdistpoly1rebuild = NOPOLYGON;
		(void)needstaticpolygon(&newstates[i]->checkdistpoly1rebuild, 4, dr_tool->cluster);
		newstates[i]->checkdistpoly2rebuild = NOPOLYGON;
		(void)needstaticpolygon(&newstates[i]->checkdistpoly2rebuild, 4, dr_tool->cluster);
		newstates[i]->checkdistpoly1orig = NOPOLYGON;
		(void)needstaticpolygon(&newstates[i]->checkdistpoly1orig, 4, dr_tool->cluster);
		newstates[i]->checkdistpoly2orig = NOPOLYGON;
		(void)needstaticpolygon(&newstates[i]->checkdistpoly2orig, 4, dr_tool->cluster);

		newlocks[i] = 0;
		if (ensurevalidmutex(&newlocks[i], TRUE)) return(TRUE);

		newstates[i]->topcell = NONODEPROTO;

		newstates[i]->freeshapes = NOSHAPE;
	}
	if (drcb_statecount > 0)
	{
		efree((CHAR *)drcb_state);
		efree((CHAR *)drcb_processdone);
	}
	drcb_state = newstates;
	drcb_processdone = newlocks;
	drcb_statecount = numstates;
	return(FALSE);
}

void drcb_term(void)
{
	REGISTER INTBIG i;
	REGISTER CHAR **list;
	REGISTER HASHTABLE *ht;
	REGISTER SHAPE *shape;

	for(i=0; i<drcb_statecount; i++)
	{
		while (drcb_state[i]->freeshapes != NOSHAPE)
		{
			shape = drcb_state[i]->freeshapes;
			drcb_state[i]->freeshapes = (SHAPE *)drcb_state[i]->freeshapes->hpp;
			efree((CHAR *)shape);
		}

		freepolylist(drcb_state[i]->polylist);
		freepolylist(drcb_state[i]->subpolylist);
		freepolylist(drcb_state[i]->croppolylist);
		freepolylist(drcb_state[i]->activecroppolylist);
		freepolylist(drcb_state[i]->lookforlayerpolylist);
		efree((CHAR *)drcb_state[i]);
		/* should free "mutex" object "drcb_processdone[i]" */
	}
	if (drcb_statecount > 0)
	{
		efree((CHAR *)drcb_state);
		efree((CHAR *)drcb_processdone);
	}

	if (drcb_incrementalhpp != NOHASHTABLE) drcb_hashdestroy(drcb_incrementalhpp);
	if (drcb_incrementalhai != NOHASHTABLE) drcb_hashdestroy(drcb_incrementalhai);
	if (drcb_incrementalhni != NOHASHTABLE)
	{
		drcb_hashwalk(drcb_incrementalhni, drcb_freehpp, 0);
		drcb_hashdestroy(drcb_incrementalhni);
	}
	for(i=0; i<LISTSIZECACHE; i++)
	{
		while (drcb_listcache[i] != 0)
		{
			list = drcb_listcache[i];
			drcb_listcache[i] = (CHAR **)(list[0]);
			efree((CHAR *)list);
		}
	}
	while (drcb_freehashtables != NOHASHTABLE)
	{
		ht = drcb_freehashtables;
		drcb_freehashtables = (HASHTABLE *)drcb_freehashtables->thelist;
		efree((CHAR *)ht);
	}

	if (drcb_layerinternodehashsize > 0)
	{
		for(i=0; i<drcb_layerinternodehashsize; i++)
			if (drcb_layerinternodetable[i] != 0)
				efree((CHAR *)drcb_layerinternodetable[i]);
		efree((CHAR *)drcb_layerinternodetable);
		efree((CHAR *)drcb_layerinternodehash);
	}
	if (drcb_layerinterarchashsize > 0)
	{
		for(i=0; i<drcb_layerinterarchashsize; i++)
			if (drcb_layerinterarctable[i] != 0)
				efree((CHAR *)drcb_layerinterarctable[i]);
		efree((CHAR *)drcb_layerinterarctable);
		efree((CHAR *)drcb_layerinterarchash);
	}
}

/*
 * Initialization of checking for cell "cell".
 */
BOOLEAN drcb_init(NODEPROTO *cell)
{
	REGISTER TECHNOLOGY *tech;
	REGISTER INTBIG i, l;

	/* do one-time allocation */
	if (drcb_initonce()) return(TRUE);
	drcb_options = dr_getoptionsvalue();

	/* see if DRC will be done with multiple processes */
	drcb_paralleldrc = FALSE;
	if (graphicshas(CANDOTHREADS) && (drcb_options&DRCMULTIPROC) != 0)
	{
		drcb_numprocesses = (drcb_options&DRCNUMPROC) >> DRCNUMPROCSH;
		if (drcb_numprocesses > 1)
		{
			if (drcb_makestateblocks(drcb_numprocesses)) return(TRUE);
			if (ensurevalidmutex(&drcb_mutexhash, TRUE)) return(TRUE);
			if (ensurevalidmutex(&drcb_mutexnode, TRUE)) return(TRUE);
			if (ensurevalidmutex(&drcb_mutexio,   TRUE)) return(TRUE);
			drcb_paralleldrc = TRUE;
			drcb_mainthread = drcb_numprocesses - 1;
		}
	}

	/* determine technology to use */
	tech = (cell != NONODEPROTO) ? cell->tech : el_curtech;
	dr_cachevalidlayers(tech);

	/* determine maximum DRC interaction distance */
	drcb_interactiondistance = 0;
	for(i = 0; i < tech->layercount; i++)
	{
		l = maxdrcsurround(tech, cell->lib, i);
		if (l > drcb_interactiondistance) drcb_interactiondistance = l;
	}

	/* clear errors */
	initerrorlogging(_("DRC"));

	/* mark that this will be a hierarchical check */
	drcb_hierarchicalcheck = TRUE;

	return(FALSE);
}

/*
 * Routine to initialize for an incremental check of cell "cell".
 */
void drcb_initincrementalcheck(NODEPROTO *cell)
{
	REGISTER NODEINST *ni;
	REGISTER HASHTABLE *h;

	/* do one-time allocation */
	if (drcb_initonce()) return;
	drcb_options = dr_getoptionsvalue();

	/* mark that this will not be done in parallel */
	drcb_paralleldrc = FALSE;

	/* delete previous hashtables */
	if (drcb_incrementalhpp != NOHASHTABLE) drcb_hashdestroy(drcb_incrementalhpp);
	if (drcb_incrementalhai != NOHASHTABLE) drcb_hashdestroy(drcb_incrementalhai);
	if (drcb_incrementalhni != NOHASHTABLE)
	{
		drcb_hashwalk(drcb_incrementalhni, drcb_freehpp, 0);
		drcb_hashdestroy(drcb_incrementalhni);
	}
	drcb_state[0]->hni = NULL;
	drcb_state[0]->hai = NULL;

	dr_cachevalidlayers(cell->tech);

	/* initialize tables for this cell */
	drcb_state[0]->netindex = 1;
	drcb_mainthread = 0;
	drcb_topcellalways = drcb_state[0]->topcell = cell;
	drcb_incrementalhpp = drcb_getinitnets(cell, &drcb_state[0]->netindex);
	if (drcb_incrementalhpp == NOHASHTABLE) return;

	drcb_incrementalhai = drcb_getarcnetworks(cell, drcb_incrementalhpp,
		&drcb_state[0]->netindex);
	if (drcb_incrementalhai == NOHASHTABLE) return;

	drcb_incrementalhni = drcb_hashcreate(HTABLE_NODEARC, 2, cell);
	if (drcb_incrementalhni == NOHASHTABLE) return;

	/* build network tables for all objects */
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex != 0)
		{
			h = drcb_hashcreate(HTABLE_PORT, 1, ni->proto);
			if (h == NOHASHTABLE) return;
			drcb_getnodenetworks(ni, drcb_incrementalhpp, drcb_incrementalhai,
				h, &drcb_state[0]->netindex);
			if (drcb_hashinsert(drcb_incrementalhni, (CHAR *)ni, (CHAR *)h, 0) != 0)
			{
				ttyputerr(x_("DRC error 1 inserting node %s into hash table"),
					describenodeinst(ni));
				return;
			}
		}
	}
	drcb_state[0]->hni = drcb_incrementalhni;
	drcb_state[0]->hai = drcb_incrementalhai;

	/* mark that this will be a nonhierarchical check */
	drcb_hierarchicalcheck = FALSE;
}

/*
 * Routine to check object "geom" incrementally.  If "partial" is TRUE, then every object
 * in the cell is being checked, and it is only necessary to compare this to others
 * that are "higher" in the canonical ordering (to prevent duplicate errors).
 *
 * The routine "drcb_initincrementalcheck()" must have been called before calling this.
 */
void drcb_checkincremental(GEOM *geom, BOOLEAN partial)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER HASHTABLE *h;

	if (geom->entryisnode)
	{
		ni = geom->entryaddr.ni;
		if (ni->parent != drcb_state[0]->topcell) return;
		if (ni->proto->primindex == 0) return;
		h = (HASHTABLE *)drcb_hashsearch(drcb_incrementalhni, (CHAR *)ni);
		if (h == NULL)
		{
			ttyputerr(x_("DRC error 2 locating node %s in hash table"),
				describenodeinst(ni));
			return;
		}
		drcb_state[0]->nhpp = h;
		(void)drcb_checknodeinst(drcb_state[0], ni, h, partial);
	} else
	{
		ai = geom->entryaddr.ai;
		if (ai->parent != drcb_state[0]->topcell) return;
		drcb_state[0]->nhpp = NULL;
		(void)drcb_checkarcinst(drcb_state[0], ai, partial);
	}
}

/*
 * Main entry point for hierarchical check: initializes all variables, caches some
 * information, and starts the DRC.  Invoked by "telltool drc hierarchical run"
 * to run hierarchically on the current node.  Returns number of errors found.
 */
INTBIG drcb_check(NODEPROTO *cell, BOOLEAN report, BOOLEAN justarea)
{
	REGISTER NODEPROTO *np, *snp;
	REGISTER LIBRARY *lib;
	REGISTER INTBIG errorcount, i;
	INTBIG lx, hx, ly, hy;
	REGISTER float elapsed;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	if (cell == NONODEPROTO) return(0);
	if (report) starttimer();
	if (drcb_init(cell)) return(0);

	ttyputmsg(x_("Maximum interaction distance is %s"),
		latoa(drcb_interactiondistance, 0));

	/* mark all cells unchecked */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = 0;

	for(i=0; i<drcb_statecount; i++)
	{
		/* allocate parcel of net number space to each processor */
		drcb_state[i]->netindex = 1;

		/* remember the top of the tree for error display purposes */
		drcb_state[i]->topcell = cell;
	}

	/* remember the top of the tree for error display purposes */
	drcb_topcellalways = cell;

	/* get area to check */
	if (justarea)
	{
		snp = us_getareabounds(&lx, &hx, &ly, &hy);
		if (snp != cell)
		{
			ttyputmsg(_("Cannot check selection: it is not in the current cell"));
			justarea = FALSE;
		}
	} else lx = hx = ly = hy = 0;

	/* recursively check this and all lower cells */
	if (drcb_checkallprotocontents(drcb_state[0], cell, justarea, lx, hx, ly, hy) > 0) return(0);

	/* sort the errors by layer */
	sorterrors();
	errorcount = numerrors();
	if (report)
	{
		elapsed = endtimer();
		if (elapsed > 60.0 && (us_useroptions&BEEPAFTERLONGJOB) != 0)
			ttybeep(SOUNDBEEP, TRUE);
		ttyputmsg(_("%ld errors found (took %s)"), errorcount, explainduration(elapsed));
	}
	termerrorlogging(report);
	return(errorcount);
}

/************************ CONTENTS CHECKING ************************/

/*
 * recursively walks through a cell, checking all
 * instances and sub-instances.  Returns positive on error,
 * negative if the cell didn't need to be checked.
 */
INTBIG drcb_checkallprotocontents(STATE *state, NODEPROTO *np, BOOLEAN justarea, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy)
{
	REGISTER HASHTABLE *nhpp, *nhai;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *savetop;
	REGISTER VARIABLE *var;
	float elapsed;
	REGISTER INTBIG ret, localerrors, errcount;
	REGISTER BOOLEAN allsubcellsstillok;
	INTBIG slx, shx, sly, shy;
	XARRAY xrot, xtrn, trans;
	REGISTER UINTBIG lastgooddate, lastchangedate;
	NODEINST top;	/* Dummy node instance for node */
	GEOM topgeom;	/* Dummy geometry module for dummy top node */

	/* first check all subcells */
	allsubcellsstillok = TRUE;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex != 0) continue;
		if (ni->proto->temp1 != 0) continue;

		/* if only checking a specific area, check it and pass it */
		if (justarea)
		{
			if (ni->geom->highx < lx || ni->geom->lowx > hx) continue;
			if (ni->geom->highy < ly || ni->geom->lowy > hy) continue;

			/* transform the bounding box */
			makerotI(ni, xrot);
			maketransI(ni, xtrn);
			transmult(xrot, xtrn, trans);
			slx = lx;   shx = hx;   sly = ly;   shy = hy;
			xformbox(&slx, &shx, &sly, &shy, trans);
		} else slx = shx = sly = shy = 0;

		/* recursively check the subcell */
		savetop = state->topcell;
		state->topcell = ni->proto;
		ret = drcb_checkallprotocontents(state, ni->proto, justarea, slx, shx, sly, shy);
		state->topcell = savetop;
		if (ret > 0) return(1);
		if (ret == 0) allsubcellsstillok = FALSE;
	}

	/* remember how many errors there are on entry */
	errcount = numerrors();

	/* prepare to check cell */
	np->temp1++;

	/* if the cell hasn't changed since the last good check, stop now */
	if (allsubcellsstillok)
	{
		var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER, dr_lastgooddrckey);
		if (var != NOVARIABLE)
		{
			lastgooddate = (UINTBIG)var->addr;
			lastchangedate = np->revisiondate;
			if (lastchangedate <= lastgooddate) return(-1);
		}
	}

	/* announce progress */
	ttyputmsg(_("Checking cell %s"), describenodeproto(np));

	/* get information about this cell */
	state->netindex = 1;    /* !! per-processor initialization */
	nhpp = drcb_getinitnets(np, &state->netindex);
	if (nhpp == NOHASHTABLE) return(1);
	nhai = drcb_getarcnetworks(np, nhpp, &state->netindex);
	if (nhai == NOHASHTABLE) return(1);

	/* check the cell contents nonhierarchically */
	if (drcb_checkprotocontents(state, np, nhpp, nhai, justarea, lx, hx, ly, hy)) return(1);

	/*
	 * Create dummy node instance for top level cell. It has the same
	 * coordinates as the nodeproto, and no parent. Those are the only
	 * relevant members for it.
	 */
	initdummynode(&top);
	topgeom.firstvar = NOVARIABLE;
	topgeom.numvar = 0;
	topgeom.entryisnode = TRUE;
	topgeom.entryaddr.ni = &top;

	top.lowx = topgeom.lowx = np->lowx;
	top.highx = topgeom.highx = np->highx;
	top.lowy = topgeom.lowy = np->lowy;
	top.highy = topgeom.highy = np->highy;
	top.proto = np;
	top.geom = &topgeom;

	/* check interactions related to this cell (hierarchically) */
	ret = drcb_checknodeinteractions(state, &top, el_matid, nhpp, nhai, NONODEPROTO,
		justarea, lx, hx, ly, hy);
	drcb_hashdestroy(nhpp);
	drcb_hashdestroy(nhai);
	if (ret > 0) return(1);

	/* stop now if interrupted */
	if (state == drcb_state[drcb_mainthread])
		if (stopping(STOPREASONDRC)) return(0);

	/* if there were no errors, remember that */
	elapsed = endtimer();
	if (elapsed > 60.0 && (us_useroptions&BEEPAFTERLONGJOB) != 0)
		ttybeep(SOUNDBEEP, TRUE);
	localerrors = numerrors() - errcount;
	if (localerrors == 0)
	{
		(void)setvalkey((INTBIG)np, VNODEPROTO, dr_lastgooddrckey,
			(INTBIG)getcurrenttime(), VINTEGER);
		ttyputmsg(_("   No errors found (%s so far)"), explainduration(elapsed));
	} else
	{
		ttyputmsg(_("   %ld errors found (%s so far)"), localerrors,
			explainduration(elapsed));
	}
	return(0);
}

/*
 * Actual check of a node prototype - goes through all primitive
 * nodeinsts and all arcinsts in a nodeproto and checks that they are
 * design rule correct within the cell.  Any errors are caused by excess
 * paint are reported.  Any errors caused by the absence of paint are
 * possibly spurious -- they're stored in case a subcell gets rid of
 * them.  Even if there are no subcells within interacting distance of an
 * "absence of paint" error, it can be spurious if it gets removed by
 * some overlapping instance. !! If it isn't removed in any instance of
 * the cell, then it should be reported as being a proto error, if it is
 * removed in some instances but not in others, then it should be
 * reported as an instance error.  How do we tell?
 * Returns true on error.
 */
BOOLEAN drcb_checkprotocontents(STATE *state, NODEPROTO *np, HASHTABLE *hpp, HASHTABLE *hai,
	BOOLEAN justarea, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER INTBIG freq;
	REGISTER BOOLEAN ret, wanttostop;
	REGISTER HASHTABLE *hni, *nhpp, *h;

	hni = drcb_hashcreate(HTABLE_NODEARC, 2, np);
	if (hni == NOHASHTABLE) return(TRUE);

	/* build network tables for all objects */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex != 0)
		{
			nhpp = drcb_hashcreate(HTABLE_PORT, 1, ni->proto);
			if (nhpp == NOHASHTABLE) return(TRUE);
			drcb_getnodenetworks(ni, hpp, hai, nhpp, &state->netindex);
			if (drcb_hashinsert(hni, (CHAR *)ni, (CHAR *)nhpp, 0) != 0)
			{
				ttyputerr(x_("DRC error 3 inserting node %s into hash table"),
					describenodeinst(ni));
			}
		}
	}
	state->hni = hni;
	state->hai = hai;

	/* now check every primitive node object */
	freq = 0;
	wanttostop = FALSE;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (state == drcb_state[drcb_mainthread] && ((++freq)%50) == 0)
		{
			if (stopping(STOPREASONDRC)) break;
		}
		if (ni->proto->primindex != 0)
		{
			if (justarea)
			{
				if (ni->geom->highx < lx || ni->geom->lowx > hx) continue;
				if (ni->geom->highy < ly || ni->geom->lowy > hy) continue;
			}
			h = (HASHTABLE *)drcb_hashsearch(hni, (CHAR *)ni);
			if (h == NULL)
			{
				ttyputerr(x_("DRC error 4 locating node %s in hash table"),
					describenodeinst(ni));
			}
			state->nhpp = h;
			ret = drcb_checknodeinst(state, ni, h, TRUE);
			if (ret && (drcb_options&DRCFIRSTERROR) != 0) { wanttostop = TRUE;   break; }
			if (el_pleasestop != 0) { wanttostop = TRUE;   break; }
		}
	}

	/* check all arcs */
	if (!wanttostop)
	{
		state->nhpp = NULL;
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		{
			if (justarea)
			{
				if (ai->geom->highx < lx || ai->geom->lowx > hx) continue;
				if (ai->geom->highy < ly || ai->geom->lowy > hy) continue;
			}
			if (state == drcb_state[drcb_mainthread] && ((++freq)%50) == 0)
				if (stopping(STOPREASONDRC)) break;
			ret = drcb_checkarcinst(state, ai, TRUE);
			if (ret && (drcb_options&DRCFIRSTERROR) != 0) break;
			if (el_pleasestop != 0) break;
		}
	}

	/* clean up */
	drcb_hashwalk(hni, drcb_freehpp, 0);
	drcb_hashdestroy(hni);
	state->hni = NULL;
	state->hai = NULL;
	return(FALSE);
}

/*
 * routine to check the design rules about nodeinst "ni".  Only check those
 * other objects whose geom pointers are greater than this one (i.e. only
 * check any pair of objects once).
 * Returns true if an error was found.
 */
BOOLEAN drcb_checknodeinst(STATE *state, NODEINST *ni, HASHTABLE *nhpp, BOOLEAN partial)
{
	REGISTER INTBIG tot, j, actual, minsize;
	REGISTER BOOLEAN ret, errorsfound;
	XARRAY trans;
	CHAR *rule;
	INTBIG minx, miny, lx, hx, ly, hy;
	REGISTER INTBIG net;
	REGISTER POLYLIST *polylist;
	REGISTER POLYGON *poly;

	makerot(ni, trans);

	/* get all of the polygons on this node */
	polylist = state->polylist;
	drcb_getnodeEpolys(ni, polylist, trans);
	tot = polylist->polylistcount;

	/* examine the polygons on this node */
	errorsfound = FALSE;
	for(j=0; j<tot; j++)
	{
		poly = polylist->polygons[j];
		if (poly->layer < 0) continue;
		if (poly->portproto == NOPORTPROTO) net = NONET; else
			net = drcb_network(nhpp, (CHAR *)poly->portproto, 1);
		ret = drcb_badbox(0, state, ni->geom, poly->layer,
			ni->proto->tech, ni->parent, poly, net, partial);
		if (ret)
		{
			errorsfound = TRUE;
			if ((drcb_options&DRCFIRSTERROR) != 0) break;
		}
		ret = drcb_checkminwidth(state, ni->geom, poly->layer, poly, ni->proto->tech, TRUE);
		if (ret)
		{
			errorsfound = TRUE;
			if ((drcb_options&DRCFIRSTERROR) != 0) break;
		}
		if (el_pleasestop != 0) break;
		if (poly->tech == dr_curtech && !dr_layersvalid[poly->layer])
		{
			drcb_reporterror(state, BADLAYERERROR, dr_curtech, 0, TRUE, 0, 0, 0,
				poly, ni->geom, poly->layer, NONET, NOPOLYGON, NOGEOM, 0, NONET);
			errorsfound = TRUE;
			if ((drcb_options&DRCFIRSTERROR) != 0) break;
		}
	}

	/* check node for minimum size */
	drcminnodesize(ni->proto, ni->parent->lib, &minx, &miny, &rule);
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
			drcb_reporterror(state, MINSIZEERROR, ni->proto->tech, 0, TRUE, minsize, actual, rule,
				NOPOLYGON, ni->geom, 0, NONET, NOPOLYGON, NOGEOM, 0, NONET);
		}
	}
	return(errorsfound);
}

/*
 * routine to check the design rules about arcinst "ai".
 * Returns true if errors were found.
 */
BOOLEAN drcb_checkarcinst(STATE *state, ARCINST *ai, BOOLEAN partial)
{
	REGISTER INTBIG tot, j;
	REGISTER BOOLEAN ret, errorsfound;
	REGISTER INTBIG net;
	REGISTER POLYLIST *polylist;
	REGISTER POLYGON *poly;

	/* get all of the polygons on this arc */
	polylist = state->polylist;
	drcb_getarcpolys(ai, polylist);
	drcb_cropactivearc(state, ai, polylist);
	tot = polylist->polylistcount;

	/* examine the polygons on this arc */
	errorsfound = FALSE;
	for(j=0; j<tot; j++)
	{
		poly = polylist->polygons[j];
		if (poly->layer < 0) continue;
		net = drcb_network(state->hai, (CHAR *)ai, 0);
		ret = drcb_badbox(0, state, ai->geom, poly->layer,
			ai->proto->tech, ai->parent, poly, net, partial);
		if (ret)
		{
			errorsfound = TRUE;
			if ((drcb_options&DRCFIRSTERROR) != 0) break;
		}
		ret = drcb_checkminwidth(state, ai->geom, poly->layer, poly, ai->proto->tech, TRUE);
		if (ret)
		{
			errorsfound = TRUE;
			if ((drcb_options&DRCFIRSTERROR) != 0) break;
		}
		if (poly->tech == dr_curtech && !dr_layersvalid[poly->layer])
		{
			drcb_reporterror(state, BADLAYERERROR, dr_curtech, 0, TRUE, 0, 0, 0,
				poly, ai->geom, poly->layer, NONET, NOPOLYGON, NOGEOM, 0, NONET);
			errorsfound = TRUE;
			if ((drcb_options&DRCFIRSTERROR) != 0) break;
		}
		if (el_pleasestop != 0) break;
	}
	return(errorsfound);
}

/*
 * Routine to ensure that polygon "poly" on layer "layer" from object "geom" in
 * technology "tech" meets minimum width rules.  If it is too narrow, other layers
 * in the vicinity are checked to be sure it is indeed an error.  Returns true
 * if an error is found.
 */
BOOLEAN drcb_checkminwidth(STATE *state, GEOM *geom, INTBIG layer, POLYGON *poly,
	TECHNOLOGY *tech, BOOLEAN withincell)
{
	REGISTER INTBIG minwidth, actual;
	INTBIG lx, hx, ly, hy, ix, iy;
	CHAR *rule;
	BOOLEAN p1found, p2found, p3found;
	REGISTER INTBIG ang, oang, perpang;
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
		if (drcb_lookforlayer(state, cell, layer, el_matid, lx-1, hx+1, ly-1, hy+1,
			xl1, yl1, &p1found, xl2, yl2, &p2found, xl3, yl3, &p3found)) return(FALSE);

		p1found = p2found = p3found = FALSE;
		if (drcb_lookforlayer(state, cell, layer, el_matid, lx-1, hx+1, ly-1, hy+1,
			xr1, yr1, &p1found, xr2, yr2, &p2found, xr3, yr3, &p3found)) return(FALSE);

		drcb_reporterror(state, MINWIDTHERROR, tech, 0, withincell, minwidth, actual, rule,
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
		drcb_reporterror(state, MINWIDTHERROR, tech, 0, withincell, minwidth, actual, rule,
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
		ang = figureangle(fx, fy, tx, ty);
		cx = (fx + tx) / 2;
		cy = (fy + ty) / 2;
		perpang = (ang + 900) % 3600;
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
			oang = figureangle(ofx, ofy, otx, oty);
			if ((ang%1800) == (oang%1800))
			{
				/* lines are parallel: see if they are colinear */
				if (isonline(fx, fy, tx, ty, ofx, ofy)) continue;
				if (isonline(fx, fy, tx, ty, otx, oty)) continue;
				if (isonline(ofx, ofy, otx, oty, fx, fy)) continue;
				if (isonline(ofx, ofy, otx, oty, tx, ty)) continue;
			}
			if (intersect(cx, cy, perpang, ofx, ofy, oang, &ix, &iy) < 0) continue;
			if (ix < mini(ofx, otx) || ix > maxi(ofx, otx)) continue;
			if (iy < mini(ofy, oty) || iy > maxi(ofy, oty)) continue;
			actual = computedistance(cx, cy, ix, iy);
			if (actual < minwidth)
			{
				/* look between the points to see if it is minimum width or notch */
				if (isinside((cx+ix)/2, (cy+iy)/2, poly))
				{
					drcb_reporterror(state, MINWIDTHERROR, tech, 0, withincell, minwidth,
						actual, rule, poly, geom, layer, NONET, NOPOLYGON, NOGEOM, 0, NONET);
				} else
				{
					drcb_reporterror(state, NOTCHERROR, tech, 0, withincell, minwidth,
						actual, rule, poly, geom, layer, NONET, poly, geom, layer, NONET);
				}
				return(TRUE);
			}
		}
	}
	return(FALSE);
}

/************************ INTERACTIONS BETWEEN CELLS ************************/

static NODEINST *drcb_parallelnodeinst;
static BOOLEAN   drcb_errorsfound;
static BOOLEAN   drcb_justarea;
static INTBIG    drcb_arealx, drcb_areahx, drcb_arealy, drcb_areahy;

static INTBIG    drcb_analyzenode(STATE *state, NODEINST *ni, XARRAY mytrans,
					HASHTABLE *hpp, HASHTABLE *hai, NODEPROTO *intersect_list,
					BOOLEAN justarea, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy);
static void     *drcb_dothread(void *argument);
static NODEINST *drcb_getnextparallelnodeinst(STATE *state);

/*
 * Recursive check of a node instance (node prototype + transformation).
 * Returns positive on error, zero if no errors found, negative if errors found.
 *
 * This is the place to implement parallelism.  This routine takes up 98% of the CPU
 * time, and if each node can be sent to a different processor, the work can run in
 * parallel.
 */
INTBIG drcb_checknodeinteractions(STATE *state, NODEINST *nodeinst, XARRAY mytrans,
	HASHTABLE *hpp, HASHTABLE *hai, NODEPROTO *intersect_list, BOOLEAN justarea,
	INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy)
{
	REGISTER NODEINST *ni;
	REGISTER INTBIG ret, i, numprocesses, numtasks;
	REGISTER BOOLEAN errorsfound;

	/* see if multiple processes should be invoked here */
	if (intersect_list == NONODEPROTO && drcb_paralleldrc)
	{
		/* code cannot be called by multiple procesors: uses globals */
		NOT_REENTRANT;

		/* requesting multiple processors: see if it is sensible */
		numprocesses = drcb_numprocesses;
		numtasks = 0;
		for(ni = nodeinst->proto->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			if (ni->proto->primindex == 0) numtasks++;
		if (numtasks < numprocesses) numprocesses = numtasks;
		if (numprocesses > 1)
		{
			/* break up into multiple processes */
			drcb_parallelnodeinst = nodeinst->proto->firstnodeinst;
			drcb_errorsfound = FALSE;
			drcb_justarea = justarea;
			drcb_arealx = lx;   drcb_areahx = hx;
			drcb_arealy = ly;   drcb_areahy = hy;
			setmultiprocesslocks(TRUE);

			/* startup extra threads */
			for(i=0; i<numprocesses; i++)
			{
				transcpy(mytrans, drcb_state[i]->paramtrans);
				drcb_state[i]->paramhpp = hpp;
				drcb_state[i]->paramhai = hai;
				drcb_state[i]->paramintersect_list = intersect_list;
				if (i != 0)
				{
					/* copy globals from the main block */
					drcb_state[i]->netindex = state->netindex;
				}

				/* mark this process as "still running" */
				emutexlock(drcb_processdone[i]);
				if (i == drcb_mainthread)
				{
					/* the last process is run locally, not in a new thread */
					(void)drcb_dothread((void *)i);
				} else
				{
					/* create a thread to run this one */
					enewthread(drcb_dothread, (void *)i);
				}
			}

			/* now wait for all processes to finish */
			for(i=0; i<numprocesses; i++)
			{
				emutexlock(drcb_processdone[i]);
				emutexunlock(drcb_processdone[i]);
			}
			errorsfound = drcb_errorsfound;
			setmultiprocesslocks(FALSE);
			if (errorsfound) return(-1);
			return(0);
		}
	}

	/* single processor or a recursive entry: just keep running on the same processor */
	errorsfound = FALSE;
	for(ni = nodeinst->proto->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex != 0) continue;
		if (state == drcb_state[drcb_mainthread])
			if (stopping(STOPREASONDRC)) break;

		if (justarea)
		{
			if (ni->geom->highx < lx || ni->geom->lowx > hx) continue;
			if (ni->geom->highy < ly || ni->geom->lowy > hy) continue;
		}

		ret = drcb_analyzenode(state, ni, mytrans, hpp, hai, intersect_list,
			justarea, lx, hx, ly, hy);
		if (ret > 0) return(1);
		if (ret < 0)
		{
			errorsfound = TRUE;
			if ((drcb_options&DRCFIRSTERROR) != 0) break;
		}
	}
	if (errorsfound) return(-1);
	return(0);
}

/*
 * Routine to return the next node instance that needs to be checked.
 * This is called by parallel processes, and so it locks the section that
 * accesses the global pointer to the list of nodes.
 */
NODEINST *drcb_getnextparallelnodeinst(STATE *state)
{
	NODEINST *ni;

	if (drcb_parallelnodeinst == NONODEINST) return(NONODEINST);

	/* only the main thread can call "stopping" */
	if (state == drcb_state[drcb_mainthread])
		(void)stopping(STOPREASONDRC);

	/* all threads check whether interrupt has been requested */
	if (el_pleasestop != 0)
	{
		drcb_parallelnodeinst = NONODEINST;
		return(NONODEINST);
	}

	if (drcb_errorsfound)
	{
		if ((drcb_options&DRCFIRSTERROR) != 0)
		{
			drcb_parallelnodeinst = NONODEINST;
			return(NONODEINST);
		}
	}

	emutexlock(drcb_mutexnode);
	for(;;)
	{
		ni = drcb_parallelnodeinst;
		if (ni == NONODEINST) break;
		drcb_parallelnodeinst = ni->nextnodeinst;
		if (ni->proto->primindex == 0) break;
	}
	emutexunlock(drcb_mutexnode);
	return(ni);
}

/*
 * Routine to run a thread that repeatedly fetches a node and analyzes it.
 */
void *drcb_dothread(void *argument)
{
	REGISTER NODEINST *ni;
	REGISTER INTBIG ret;
	REGISTER STATE *state;

	state = drcb_state[(INTBIG)argument];
	for(;;)
	{
		ni = drcb_getnextparallelnodeinst(state);
		if (ni == NONODEINST) break;
		if (drcb_justarea)
		{
			if (ni->geom->highx < drcb_arealx || ni->geom->lowx > drcb_areahx) continue;
			if (ni->geom->highy < drcb_arealy || ni->geom->lowy > drcb_areahy) continue;
		}
		ret = drcb_analyzenode(state, ni, state->paramtrans, state->paramhpp,
			state->paramhai, state->paramintersect_list,
			drcb_justarea, drcb_arealx, drcb_areahx, drcb_arealy, drcb_areahy);
		if (ret > 0) break;
		if (ret < 0) drcb_errorsfound = TRUE;
	}

	/* mark this process as "done" */
	emutexunlock(drcb_processdone[(INTBIG)argument]);
	return(0);
}

/*
 * Check node instance "ni", transformed by "mytrans", with "hpp" and "hai" as port and
 * arc hash tables.  Check against "intersect_list".
 * Returns positive on error, zero if no errors found, negative if errors found.
 */
INTBIG drcb_analyzenode(STATE *state, NODEINST *ni, XARRAY mytrans,
	HASHTABLE *hpp, HASHTABLE *hai, NODEPROTO *intersect_list,
	BOOLEAN justarea, INTBIG arealx, INTBIG areahx, INTBIG arealy, INTBIG areahy)
{
	REGISTER NODEPROTO *child_intersects;
	XARRAY subtrans, temptrans1, temptrans2;
	INTBIG lx, hx, ly, hy;
	REGISTER INTBIG ret;
	REGISTER BOOLEAN errorsfound;
	REGISTER HASHTABLE *nhai, *nhpp;

	/* cell: recursively check it */
	nhpp = drcb_hashcreate(HTABLE_PORT, 1, ni->proto);
	if (nhpp == NOHASHTABLE) return(1);

	drcb_getnodenetworks(ni, hpp, hai, nhpp, &state->netindex);
	nhai = drcb_getarcnetworks(ni->proto, nhpp, &state->netindex);
	if (nhai == NOHASHTABLE) return(1);

	/*
	 * Compute transformation matrix that maps the contents of the
	 * instance back to the cell that contains the instance.
	 */
	makerot(ni, temptrans1);
	transmult(temptrans1, mytrans, temptrans2);
	maketrans(ni, temptrans1);
	transmult(temptrans1, temptrans2, subtrans);

	/*
	 * drcb_box returns the bounding box of nodeinst - this box is
	 * relative to the parent prototype, which is fine, because that's
	 * what we'll be searching for intersecting elements
	 */
	drcb_box(ni, &lx, &hx, &ly, &hy);
	if (justarea)
	{
		if (lx > areahx || hx < arealx) return(0);
		if (ly > areahy || hy < arealy) return(0);
		if (lx < arealx) lx = arealx;
		if (hx > areahx) hx = areahx;
		if (ly < arealy) ly = arealy;
		if (hy > areahy) hy = areahy;
	}

	/*
	 * find intersecting elements returns elements relative to
	 * nodeinst.  all checking of the nodeinst can then be done
	 * in the nodeinst's coordinate system
	 */
	child_intersects = drcb_findintersectingelements(state, ni,
		lx, hx, ly, hy, subtrans, hpp, hai, intersect_list);

	/*
	 * Check each intersecting element with any interacting primitive
	 * nodes in the nodeinst.
	 */
	errorsfound = FALSE;
	if (child_intersects != NONODEPROTO)
	{
		ret = drcb_checkintersections(state, ni, child_intersects, nhpp, nhai, subtrans);
		if (ret > 0) return(1);
		if (ret < 0) errorsfound = TRUE;

		/* recursively examine contents */
		if ((drcb_options&DRCFIRSTERROR) == 0 || !errorsfound)
		{
			ret = drcb_checknodeinteractions(state, ni, subtrans, nhpp, nhai, child_intersects,
				FALSE, 0, 0, 0, 0);
			if (ret > 0) return(1);
			if (ret < 0) errorsfound = TRUE;
		}
		drcb_freeintersectingelements(child_intersects, state);
	}
	drcb_hashdestroy(nhpp);
	drcb_hashdestroy(nhai);
	if (errorsfound) return(-1);
	return(0);
}

/*
 * Computes the drc box for a node instance - a drc box is the bounding
 * box of the node, extended outward by the maximum drid
 */
void drcb_box(NODEINST *ni, INTBIG *lxp, INTBIG *hxp, INTBIG *lyp, INTBIG *hyp)
{
	*lxp = ni->geom->lowx - drcb_interactiondistance;
	*lyp = ni->geom->lowy - drcb_interactiondistance;
	*hxp = ni->geom->highx + drcb_interactiondistance;
	*hyp = ni->geom->highy + drcb_interactiondistance;
}

/*
 * Actual check of a node instance - goes through each intersecting
 * element in the instance and makes sure the node elements that
 * intersect with that element are drc correct.  Checks if any spurious
 * errors are removed or confirmed.
 * Returns positive on error, zero if no errors found, negative if errors found.
 */
INTBIG drcb_checkintersections(STATE *state, NODEINST *nip, NODEPROTO *intersect_list,
	HASHTABLE *hpp, HASHTABLE *hai, XARRAY mytrans)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *niproto;
	REGISTER HASHTABLE *hni, *nhpp;
	CHECKSHAPEARG csa;
	REGISTER BOOLEAN ret;

	niproto = nip->proto;
	hni = drcb_hashcreate(HTABLE_NODEARC, 2, niproto);
	if (hni == NOHASHTABLE) return(1);

	csa.ni = nip;
	csa.state = state;
	state->hni = hni;
	state->hai = hai;

	/* build network tables for all objects in nip's prototype */
	for(ni = niproto->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex != 0)
		{
			nhpp = drcb_hashcreate(HTABLE_PORT, 1, ni->proto);
			if (nhpp == NOHASHTABLE) return(1);
			drcb_getnodenetworks(ni, hpp, hai, nhpp, &state->netindex);
			if (drcb_hashinsert(hni, (CHAR *)ni, (CHAR *)nhpp, 0) != 0)
			{
				ttyputerr(x_("DRC error 5 inserting node %s into hash table"),
					describenodeinst(ni));
			}
		}
	}

	/*
	 * transformation matrix for visual debugging.  This transforms
	 * the shape from the coordinate space of nip's nodeproto to the
	 * coordinate space of nip's parent, which is what is being
	 * displayed.
	 */
	transcpy(mytrans, csa.vtrans);

	/* check each geom in the intersect list */
	csa.intersectlist = intersect_list;
	ret = drcb_walkrtree(intersect_list->rtree, drcb_checkshape, &csa);

	drcb_hashwalk(hni, drcb_freehpp, 0);
	drcb_hashdestroy(hni);
	state->hni = NULL;
	state->hai = NULL;
	if (ret) return(-1);
	return(0);
}

/*
 * Walks an R Tree hierarchy - traversing the tree in a depth first
 * search running the supplied function 'func' on all terminal nodes.
 * This should probably be an Electric database routine. 'func' is called
 * as (*func)(GEOM *geom, CHECKSHAPEARG *arg) where geom is a pointer to the
 * geometry object stored in that R tree node. 'func' may NOT modify the
 * geometry related contents of geom (since that would mean a change in
 * the R tree structure).
 * If the function returns true, this routine returns true.
 */
BOOLEAN drcb_walkrtree(RTNODE *rtn, BOOLEAN (*func)(GEOM*, CHECKSHAPEARG*),
	CHECKSHAPEARG *arg)
{
	REGISTER BOOLEAN ret, errorsfound;
	REGISTER INTBIG j;

	errorsfound = FALSE;
	for(j = 0; j < rtn->total; j++)
	{
		if (rtn->flag != 0)
		{
			ret = (*func)((GEOM *)(rtn->pointers[j]), arg);
		} else
		{
			ret = drcb_walkrtree((RTNODE *)rtn->pointers[j], func, arg);
		}
		if (ret)
		{
			errorsfound = TRUE;
			if ((drcb_options&DRCFIRSTERROR) != 0) break;
		}
		if (el_pleasestop != 0) break;
	}
	return(errorsfound);
}

/*
 * Routine to check a shape in the R-Tree.  Returns true if an error was found.
 */
BOOLEAN drcb_checkshape(GEOM *g, CHECKSHAPEARG *csap)
{
	REGISTER NODEINST *sni, *ni;
	REGISTER BOOLEAN ret;
	REGISTER ARCINST *sai;
	REGISTER SHAPE *shape;
	REGISTER POLYGON *poly;

	/*
	 * Check shape against all intersecting elements in the nodeinst.
	 * We don't need to transform anything.
	 */
	ni = csap->ni;
	shape = (SHAPE *)g->entryaddr.blind;

	if (shape->poly == NOPOLYGON)
	{
		poly = csap->state->checkshapepoly;
		poly->xv[0] = g->lowx;    poly->yv[0] = g->lowy;
		poly->xv[1] = g->highx;   poly->yv[1] = g->lowy;
		poly->xv[2] = g->highx;   poly->yv[2] = g->highy;
		poly->xv[3] = g->lowx;    poly->yv[3] = g->highy;
		poly->count = 4;
	} else
	{
		poly = shape->poly;
	}

	transcpy(shape->trans, csap->dtrans);
	if (shape->entryisnode)
	{
		sni = shape->entryaddr.ni;
		csap->state->nhpp = shape->hpp;
		ret = drcb_badbox(csap, csap->state, sni->geom, shape->layer,
			sni->proto->tech, ni->proto, poly, shape->net, FALSE);
		csap->state->nhpp = NULL;
	} else
	{
		sai = shape->entryaddr.ai;
		csap->state->nhpp = NULL;
		ret = drcb_badbox(csap, csap->state, sai->geom, shape->layer,
			sai->proto->tech, ni->proto, poly, shape->net, FALSE);
	}
	return(ret);
}

/*
 * To intersect elements, drcb_findintersectingelements creates a
 * transformation that describes how to go from nodeinst->parent
 * (cell that contains the instance) to nodeinst->proto (prototype of
 * the instance). This is the inverse of the matrix obtained by makerot
 * and maketrans, i.e. use makerotI and maketransI.
 * drcb_findintersectingelements then transforms all intersecting elements
 * it finds by that matrix. For child node instances,
 * drcb_findintersectingelements calls drcb_intersectsubtree on them, giving
 * them a new transformation matrix. This matrix is the product of the
 * transformation to go from the child node instance's prototype to the
 * parent (i.e. the result of makerot/maketrans on the child instance)
 * and the current transformation. Thus we hand the transformation down
 * the hierarchy.
 *
 * We also hand a clip box down to drcb_findintersectingelements.
 * The box is inverse transformed from the coordinate
 * system of the parent to the coordinates of the child instance's
 * prototype, and handed in to the intersect procedure for the prototype
 * along with the matrix above. The child instance's prototype is
 * searched using the transformed clip box -- this may recurse.
 *
 * All transformation matrices and clip boxes that are passed down via
 * recursion are on the stack.
 *
 * !! Try to avoid having drcb_findintersectingelements AND drcb_intersectsubtree
 * even if it means passing in a nodeinst value to each level?
 */

/*
 * Computes the intersecting elements above a node instance, and store
 * it in the instance.
 */
NODEPROTO *drcb_findintersectingelements(STATE *state, NODEINST *nodeinst,
	INTBIG lowx, INTBIG highx, INTBIG lowy, INTBIG highy, XARRAY mytrans,
		HASHTABLE *hpp, HASHTABLE *hai, NODEPROTO *parentintersectcell)
{
	REGISTER HASHTABLE *nhpp;
	REGISTER INTBIG searchkey, foundmore;
	REGISTER NODEPROTO *intersect_list;
	REGISTER NODEINST *ni;
	REGISTER GEOM *geom;
	XARRAY itrans, subtrans, temptrans1, temptrans2, temptrans3;
	INTBIG lx, hx, ly, hy;

	if (nodeinst->parent == NONODEPROTO) return(NONODEPROTO);

	intersect_list = allocnodeproto(dr_tool->cluster);
	if (intersect_list == NONODEPROTO) return(NONODEPROTO);
	intersect_list->lib = el_curlib;
	intersect_list->temp1 = 0;	/* stores a count of number of intersections */
	if (geomstructure(intersect_list))
	{
		freenodeproto(intersect_list);
		return(NONODEPROTO);
	}

	/* compute matrix to transform the shapes into nodeinst's coordinate space */
	makerotI(nodeinst, temptrans1);
	maketransI(nodeinst, temptrans2);
	transmult(temptrans1, temptrans2, itrans);

	foundmore = 0;
	searchkey = initsearch(lowx, highx, lowy, highy, nodeinst->parent);
	while((geom = nextobject(searchkey)) != NOGEOM)
	{
		if (geom->entryisnode)
		{
			ni = geom->entryaddr.ni;

			/* skip the instance which "called" this */
			if (ni == nodeinst) continue;
			foundmore = 1;
			nhpp = drcb_hashcreate(HTABLE_PORT, 1, ni->proto);
			if (nhpp == NOHASHTABLE) return(NONODEPROTO);

			/* Compute networks for the nodeinst */
			if (ni->proto->primindex == 0)
			{
				drcb_getnodenetworks(ni, hpp, hai, nhpp, &state->netindex);

				/*
				 * transform the intersecting box to the child
				 * nodeproto's coordinate system so we can recurse down
				 * the tree still intersecting with the same box.
				 */
				makerotI(ni, temptrans1);
				maketransI(ni, temptrans2);
				transmult(temptrans1, temptrans2, temptrans3);
				lx = lowx;
				hx = highx;
				ly = lowy;
				hy = highy;
				xformbox(&lx, &hx, &ly, &hy, temptrans3);

				/*
				 * Compute the inverse transform to apply to objects
				 * that intersect so they will get transformed back to
				 * the original nodeinst's coordinate space. Make a new
				 * itrans for ni in subtrans.
				 */
				maketrans(ni, temptrans1);
				makerot(ni, temptrans2);
				transmult(temptrans1, temptrans2, temptrans3);
				transmult(temptrans3, itrans, subtrans);

				/*
				 * mytrans transforms to the displayed prototype's
				 * coordinate system. Make a new mytrans for the ni in
				 * temptrans1 so we can pass it down as we recurse.
				 */
				transmult(temptrans3, mytrans, temptrans1);
				if (drcb_intersectsubtree(state, intersect_list, ni->proto, subtrans,
					temptrans1, lx, hx, ly, hy, nhpp)) return(NONODEPROTO);
			} else
			{
				drcb_getnodenetworks(ni, hpp, hai, nhpp, &state->netindex);
				if (drcb_getnodeinstshapes(state, intersect_list, ni, itrans,
					lowx, highx, lowy, highy, nhpp)) return(NONODEPROTO);
			}
			drcb_hashdestroy(nhpp);
		} else
		{
			foundmore = 1;
			if (drcb_getarcinstshapes(state, intersect_list, geom->entryaddr.ai, itrans,
				lowx, highx, lowy, highy, hai)) return(NONODEPROTO);
		}
	}

	if (foundmore == 0)
	{
		drcb_freeintersectingelements(intersect_list, state);
		intersect_list = NONODEPROTO;
	} else
	{
		/*
		 * !! Now go through the parent's intersect list 'parentintersectcell' and select
		 * those shapes that intersect. Transform by itrans and push onto
		 * intersect_list. We allocate new copies -- if we used only
		 * pointers, then it would be faster but we'd have to keep refcnts
		 * to keep track of them when freeing.
		 */
		if (parentintersectcell != NONODEPROTO)
		{
			searchkey = initsearch(lowx, highx, lowy, highy, parentintersectcell);
			while((geom = nextobject(searchkey)) != NOGEOM)
			{
				REGISTER GEOM *g;
				SHAPE *oldshape, *newshape;

				/* we're only interested if the shape intersects the drc box */
				if (!drcb_boxesintersect(geom->lowx, geom->highx, geom->lowy, geom->highy,
					lowx, highx, lowy, highy)) continue;

				oldshape = (SHAPE *)geom->entryaddr.blind;
				g = allocgeom(dr_tool->cluster);
				if (g == NOGEOM) continue;
				*g = *geom;

				newshape = drcb_allocshape(state);
				if (newshape == 0)
				{
					ttyputerr(_("DRC error allocating memory for shape"));
					continue;
				}
				*newshape = *oldshape;
				g->entryaddr.blind = newshape;
				if (oldshape->hpp)
				{
					newshape->hpp = drcb_hashcopy(oldshape->hpp);
					if (newshape->hpp == NOHASHTABLE) return(NONODEPROTO);
				}
				if (oldshape->poly != NOPOLYGON)
				{
					newshape->poly = allocpolygon(oldshape->poly->count, dr_tool->cluster);
					if (newshape->poly == NOPOLYGON) return(NONODEPROTO);
					polycopy(newshape->poly, oldshape->poly);
					xformpoly(newshape->poly, itrans);
				}
				xformbox(&(g->lowx), &(g->highx), &(g->lowy), &(g->highy), itrans);
				drcb_linkgeom(g, intersect_list);
				intersect_list->temp1++;
			}
		}
	}
	/* !! How about the error list ? */
	return intersect_list;
}

/*
 * Descends through a complex node prototype and it's instances, finding
 * all elements intersecting the box described by lowx, highx, lowy,
 * highy and adding their polygons to plpp. itrans is the matrix
 * accumulated through the recursion down the tree that maps objects
 * back to the nodeinst that the search started from. mytrans is the
 * matrix accumulated at the same time describing the transformation
 * that maps objects back to the nodeproto being displayed.
 * Returns true on error.
 */
BOOLEAN drcb_intersectsubtree(STATE *state, NODEPROTO *intersect_list,
	NODEPROTO *np, XARRAY itrans, XARRAY mytrans, INTBIG lowx, INTBIG highx,
		INTBIG lowy, INTBIG highy, HASHTABLE *hpp)
{
	REGISTER HASHTABLE *nhpp, *hai;
	REGISTER INTBIG searchkey;
	REGISTER GEOM *geom;
	XARRAY subtrans, temptrans1, temptrans2, temptrans3;
	INTBIG lx, hx, ly, hy;
	REGISTER NODEINST *ni;

	hai = drcb_getarcnetworks(np, hpp, &state->netindex);
	if (hai == NOHASHTABLE) return(TRUE);

	searchkey = initsearch(lowx, highx, lowy, highy, np);
	while((geom = nextobject(searchkey)) != NOGEOM)
	{
		if (geom->entryisnode)
		{
			ni = geom->entryaddr.ni;

			/* Compute networks for the child */
			nhpp = drcb_hashcreate(HTABLE_PORT, 1, ni->proto);
			if (nhpp == NOHASHTABLE) return(TRUE);
			drcb_getnodenetworks(ni, hpp, hai, nhpp, &state->netindex);
			if (ni->proto->primindex == 0)
			{
				/*
				 * transform the intersecting box to the child
				 * nodeproto's coordinate system so we can recurse down
				 * the tree still intersecting with the same box.
				 */
				makerotI(ni, temptrans1);
				maketransI(ni, temptrans2);
				transmult(temptrans1, temptrans2, temptrans3);
				lx = lowx;
				hx = highx;
				ly = lowy;
				hy = highy;
				xformbox(&lx, &hx, &ly, &hy, temptrans3);

				/*
				 * Compute the inverse transform to apply to objects
				 * that intersect so they will get transformed back to
				 * the original nodeinst's coordinate space. Make a new
				 * itrans for ni in subtrans.
				 */
				maketrans(ni, temptrans1);
				makerot(ni, temptrans2);
				transmult(temptrans1, temptrans2, temptrans3);
				transmult(temptrans3, itrans, subtrans);

				/*
				 * mytrans transforms to the displayed prototype's
				 * coordinate system. Make a new mytrans for the ni in
				 * temptrans1 so we can pass it down as we recurse.
				 */
				transmult(temptrans3, mytrans, temptrans1);
				if (drcb_intersectsubtree(state, intersect_list, ni->proto,
					subtrans, temptrans1, lx, hx, ly, hy, nhpp)) return(TRUE);
			} else
			{
				if (drcb_getnodeinstshapes(state, intersect_list, ni, itrans,
					lowx, highx, lowy, highy, nhpp)) return(TRUE);
			}
			drcb_hashdestroy(nhpp);
		} else
		{
			if (drcb_getarcinstshapes(state, intersect_list, geom->entryaddr.ai,
				itrans, lowx, highx, lowy, highy, hai)) return(TRUE);
		}
	}
	drcb_hashdestroy(hai);
	return(FALSE);
}

/*
 * Extracts the electrical shapes (presently only boxes) from ni,
 * transforms them by itrans, and inserts them into intersect_list.
 * Returns true on error.
 */
BOOLEAN drcb_getnodeinstshapes(STATE *state, NODEPROTO *intersect_list, NODEINST *ni,
	XARRAY itrans, INTBIG lowx, INTBIG highx, INTBIG lowy, INTBIG highy, HASHTABLE *hpp)
{
	XARRAY rtrans, trans;
	INTBIG lx, hx, ly, hy, net;
	REGISTER INTBIG j, n;
	REGISTER LIBRARY *lib;
	REGISTER POLYLIST *polylist;
	REGISTER POLYGON *poly;
	REGISTER GEOM *geom;
	REGISTER TECHNOLOGY *tech;
	REGISTER SHAPE *shape;

	makerot(ni, rtrans);
	transmult(rtrans, itrans, trans);
	polylist = state->polylist;
	drcb_getnodeEpolys(ni, polylist, rtrans);
	n = polylist->polylistcount;
	tech = ni->proto->tech;
	lib = ni->parent->lib;
	for(j = 0; j < n; j++)
	{
		poly = polylist->polygons[j];
		if (poly->layer < 0) continue;
		if (maxdrcsurround(tech, lib, poly->layer) < 0) continue;

		/* stop now if this box doesn't intersect the drc box */
		getbbox(poly, &lx, &hx, &ly, &hy);
		if (!drcb_boxesintersect(lx, hx, ly, hy, lowx, highx, lowy, highy))
			continue;

		/* find the global electrical net for this layer */
		if (poly->portproto == NOPORTPROTO) net = NONET; else
			net = drcb_network(hpp, (CHAR *)poly->portproto, 1);

		/*
		 * transform the box to the coordinates of the nodeinstance that
		 * initiated the search
		 */
		xformpoly(poly, itrans);

		/* save this in a shape structure */
		shape = drcb_allocshape(state);
		if (shape == 0) return(TRUE);
		shape->layer = poly->layer;
		shape->entryisnode = TRUE;
		shape->entryaddr.ni = ni;
		shape->net = net;
		transcpy(trans, shape->trans);

		/*
		 * we're wasting a fair bit of memory by copying an entire
		 * network table for a nodeinst for each shape in the intersect
		 * list.  Unfortunately, there's no simple way out of it; any
		 * more memory efficient scheme involves keeping reference
		 * counts for freeing, and sharing the tables as far as possible.
		 */
		shape->hpp = drcb_hashcopy(hpp);
		if (shape->hpp == NOHASHTABLE) return(TRUE);
		if (!isbox(poly, &lx, &hx, &ly, &hy))
		{
			getbbox(poly, &lx, &hx, &ly, &hy);
			shape->poly = allocpolygon(poly->count, dr_tool->cluster);
			if (shape->poly == NOPOLYGON) return(TRUE);
			polycopy(shape->poly, poly);
		}
		geom = allocgeom(dr_tool->cluster);
		if (geom == NOGEOM) return(TRUE);
		geom->entryisnode = FALSE;
		geom->entryaddr.blind = shape;
		geom->lowx = lx;
		geom->highx = hx;
		geom->lowy = ly;
		geom->highy = hy;
		drcb_linkgeom(geom, intersect_list);
		intersect_list->temp1++;
	}
	return(FALSE);
}

/*
 * Extracts the electrical shapes (presently only boxes) from ai,
 * transforms them by trans, and inserts them into intersect_list.
 * Returns true on error.
 */
BOOLEAN drcb_getarcinstshapes(STATE *state, NODEPROTO *intersect_list, ARCINST *ai,
	XARRAY trans, INTBIG lowx, INTBIG highx, INTBIG lowy, INTBIG highy, HASHTABLE *hai)
{
	INTBIG lx, hx, ly, hy;
	REGISTER INTBIG j, n;
	REGISTER POLYGON *poly;
	REGISTER POLYLIST *polylist;
	REGISTER GEOM *geom;
	REGISTER LIBRARY *lib;
	REGISTER TECHNOLOGY *tech;
	REGISTER SHAPE *shape;

	polylist = state->polylist;
	drcb_getarcpolys(ai, polylist);
	drcb_cropactivearc(state, ai, polylist);
	n = polylist->polylistcount;
	if (n < 0) return(FALSE);
	tech = ai->proto->tech;
	lib = ai->parent->lib;
	for(j = 0; j < n; j++)
	{
		poly = polylist->polygons[j];
		if (poly->layer < 0) continue;
		if (maxdrcsurround(tech, lib, poly->layer) < 0) continue;

		/* stop now if this box doesn't intersect the drc box */
		lx = polylist->lx[j];   hx = polylist->hx[j];
		ly = polylist->ly[j];   hy = polylist->hy[j];
		if (!drcb_boxesintersect(lx, hx, ly, hy, lowx, highx, lowy, highy))
			continue;

		/*
		 * transform the box to the coordinates of the nodeinstance that
		 * initiated the search
		 */
		xformpoly(poly, trans);

		/* save this in a shape structure */
		shape = drcb_allocshape(state);
		if (shape == 0) return(TRUE);
		shape->layer = poly->layer;
		shape->entryisnode = FALSE;
		shape->entryaddr.ai = ai;
		shape->net = drcb_network(hai, (CHAR *)ai, 0);
		transcpy(trans, shape->trans);
		shape->hpp = NULL;
		geom = allocgeom(dr_tool->cluster);
		if (geom == NOGEOM) return(TRUE);
		geom->entryisnode = FALSE;
		geom->entryaddr.blind = shape;
		if (!isbox(poly, &lx, &hx, &ly, &hy))
		{
			getbbox(poly, &lx, &hx, &ly, &hy);
			shape->poly = allocpolygon(poly->count, dr_tool->cluster);
			if (shape->poly == NOPOLYGON) return(TRUE);
			polycopy(shape->poly, poly);
		}
		geom->lowx = lx;
		geom->highx = hx;
		geom->lowy = ly;
		geom->highy = hy;
		drcb_linkgeom(geom, intersect_list);
		intersect_list->temp1++;
		geom = NOGEOM;
	}
	return(FALSE);
}

/*
 * routine to link geometry module "geom" into the R-tree.  The parent
 * nodeproto is in "parnt". (slightly modified from linkgeom -- this one
 * doesn't call boundobj to get the geom bounds -- it expects the ones in
 * geom to be valid when it is passed in.
 */
void drcb_linkgeom(GEOM *geom, NODEPROTO *parnt)
{
	REGISTER RTNODE *rtn;
	REGISTER INTBIG i, bestsubnode;
	REGISTER INTBIG bestexpand, area, newarea, expand, scaledown;
	INTBIG lxv, hxv, lyv, hyv;

	/* find the leaf that would expand least by adding this node */
	rtn = parnt->rtree;
	for(;;)
	{
		/* if R-tree node contains primitives, exit loop */
		if (rtn->flag != 0) break;

		/* compute scaling factor */
		scaledown = MAXINTBIG;
		for(i=0; i<rtn->total; i++)
		{
			db_rtnbbox(rtn, i, &lxv, &hxv, &lyv, &hyv);
			if (hxv-lxv < scaledown) scaledown = hxv - lxv;
			if (hyv-lyv < scaledown) scaledown = hyv - lyv;
		}
		if (scaledown <= 0) scaledown = 1;

		/* find sub-node that would expand the least */
		bestexpand = MAXINTBIG;
		for(i=0; i<rtn->total; i++)
		{
			/* get bounds and area of sub-node */
			db_rtnbbox(rtn, i, &lxv, &hxv, &lyv, &hyv);
			area = ((hxv - lxv) / scaledown) * ((hyv - lyv) / scaledown);

			/* get area of sub-node with new element */
			newarea = ((maxi(hxv, geom->highx) - mini(lxv, geom->lowx)) / scaledown) *
				((maxi(hyv, geom->highy) - mini(lyv, geom->lowy)) / scaledown);

			/* accumulate the least expansion */
			expand = newarea - area;
			if (expand > bestexpand) continue;
			bestexpand = expand;
			bestsubnode = i;
		}

		/* recurse down to sub-node that expanded least */
		rtn = (RTNODE *)rtn->pointers[bestsubnode];
	}

	/* add this geometry element to the correct leaf R-tree node */
	(void)db_addtortnode((UINTBIG)geom, rtn, parnt);
}

/************************ ACTUAL DESIGN RULE CHECKING ************************/

INTBIG drcb_network(HASHTABLE *ht, CHAR *cp, INTBIG type)
{
	REGISTER CHAR *net;

	if (ht == 0) return(NONET);
	net = drcb_hashsearch(ht, cp);
	if (net == 0)
	{
		if (type != 0)
		{
			ttyputmsg(x_("DRC error locating port %s in hashtable"),
				((PORTPROTO *)cp)->protoname);
		} else
		{
			ttyputmsg(x_("DRC error locating arc %s in hashtable"),
				describearcinst((ARCINST *)cp));
		}
		return(NONET);
	}
	return((INTBIG)net);
}

CHAR *drcb_freehpp(CHAR *key, CHAR *datum, CHAR *arg)
{
	drcb_hashdestroy((HASHTABLE *)datum);
	return(0);
}

/*
 * routine to make a design-rule check on polygon "poly" which is from object
 * "geom", on layer "layer", in technology "tech".
 * If "partial" is nonzero then only objects whose geometry modules are
 * greater than this one will be checked (to prevent any pair of objects
 * from being checked twice).  "state->nhpp" has the network tables
 * for geom if geom is a NODEINST, and can be NULL if geom is an ARCINST.
 * "state->hni" is a hash table containing the network tables for all
 * nodeinsts in cell, keyed by the nodeinst address.
 *
 * if "csap" is nonzero, this is a cross-hierarchical check, and the polygon
 * is actually from a pseudo-cell created to examine the vicinity of
 * an instance of "cell".  The routine examines everything in "cell" and compares
 * it to the polygon (which is in the coordinate space of the cell).
 *
 * Returns true if an error was found.
 */
BOOLEAN drcb_badbox(CHECKSHAPEARG *csap, STATE *state, GEOM *geom,
	INTBIG layer, TECHNOLOGY *tech, NODEPROTO *cell, POLYGON *poly, INTBIG net,
		BOOLEAN partial)
{
	REGISTER GEOM *ngeom;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER ARCINST *ai;
	REGISTER ARCPROTO *ap;
	REGISTER BOOLEAN touch, ret, multi, basemulti;
	REGISTER INTBIG nnet, bound, search, dist, j, tot, minsize, nminsize,
		lxbound, hxbound, lybound, hybound, count;
	REGISTER POLYLIST *subpolylist;
	REGISTER POLYGON *npoly;
	XARRAY trans;
	CHAR *rule;
	REGISTER BOOLEAN con;
	REGISTER HASHTABLE *pihpp;
	INTBIG lx, hx, ly, hy, edge;

	/* see how far around the box it is necessary to search */
	bound = maxdrcsurround(tech, cell->lib, layer);
	if (bound < 0) return(FALSE);

	/* get bounds */
	getbbox(poly, &lx, &hx, &ly, &hy);
	minsize = polyminsize(poly);

	/* determine if original object has multiple contact cuts */
	if (geom->entryisnode) basemulti = drcb_ismulticut(geom->entryaddr.ni); else
		basemulti = FALSE;

	/* search in the area surrounding the box */
	subpolylist = state->subpolylist;
	lxbound = lx-bound;   hxbound = hx+bound;
	lybound = ly-bound;   hybound = hy+bound;
	search = initsearch(lxbound, hxbound, lybound, hybound, cell);
	count = 0;
	for(;;)
	{
		if (state == drcb_state[drcb_mainthread] && (++count % 5) == 0)
		{
			if (stopping(STOPREASONDRC))
			{
				termsearch(search);
				return(FALSE);
			}
		}
		if ((ngeom = nextobject(search)) == NOGEOM) break;
		if (partial)
		{
			if (ngeom == geom) continue;
			if (ngeom < geom) continue;
		}

		if (ngeom->entryisnode)
		{
			ni = ngeom->entryaddr.ni;   np = ni->proto;

			/* ignore nodes that are not primitive */
			if (np->primindex == 0) continue;

			/* don't check between technologies */
			if (np->tech != tech) continue;

			/* see if this type of node can interact with this layer */
			if (!drcb_checklayerwithnode(layer, np)) continue;

			/* see if the objects directly touch */
			touch = drcb_objtouch(ngeom, geom);

			/* prepare to examine every layer in this nodeinst */
			makerot(ni, trans);

			if (state->hni == NULL) continue;
			pihpp = (HASHTABLE *)drcb_hashsearch(state->hni, (CHAR *)ni);
			if (pihpp == NULL)
			{
				ttyputerr(x_("DRC error 6 locating node %s in hash table"),
					describenodeinst(ni));
				continue;
			}

			/* get the shape of each nodeinst layer */
			drcb_getnodeEpolys(ni, subpolylist, trans);
			tot = subpolylist->polylistcount;
			multi = basemulti;
			if (!multi) multi = drcb_ismulticut(ni);
			for(j=0; j<tot; j++)
			{
				npoly = subpolylist->polygons[j];
				if (subpolylist->lx[j] > hxbound ||
					subpolylist->hx[j] < lxbound ||
					subpolylist->ly[j] > hybound ||
					subpolylist->hy[j] < lybound) continue;
				if (npoly->layer < 0) continue;

				/* find the global electrical net for this layer */
				if (npoly->portproto == NOPORTPROTO) nnet = NONET; else
					nnet = drcb_network(pihpp, (CHAR *)npoly->portproto, 1);

				/* see whether the two objects are electrically connected */
				if (nnet != NONET && nnet == net) con = TRUE; else con = FALSE;

				/* if they connect electrically and adjoin, don't check */
				if (con && touch) continue;

				nminsize = polyminsize(npoly);
				dist = dr_adjustedmindist(tech, cell->lib, layer, minsize,
					npoly->layer, nminsize, con, multi, &edge, &rule);
				if (dist < 0) continue;

				/* check the distance */
				ret = drcb_checkdist(csap, state, tech, layer, net, geom, poly, state->nhpp,
					npoly->layer, nnet, ngeom, npoly, pihpp, con, dist, edge, rule, cell);
				if (ret) { termsearch(search);   return(TRUE); }
			}
		} else
		{
			ai = ngeom->entryaddr.ai;   ap = ai->proto;

			/* don't check between technologies */
			if (ap->tech != tech) continue;

			/* see if this type of arc can interact with this layer */
			if (!drcb_checklayerwitharc(layer, ap)) continue;
			if (state->hai == NULL) continue;

			/* see if the objects directly touch */
			touch = drcb_objtouch(ngeom, geom);

			/* see whether the two objects are electrically connected */
			nnet = drcb_network(state->hai, (CHAR *)ai, 0);
			if (net != NONET && nnet == net) con = TRUE; else con = FALSE;

			/* if they connect electrically and adjoin, don't check */
			if (con && touch) continue;

			/* get the shape of each arcinst layer */
			drcb_getarcpolys(ai, subpolylist);
			drcb_cropactivearc(state, ai, subpolylist);
			tot = subpolylist->polylistcount;
			multi = basemulti;
			for(j=0; j<tot; j++)
			{
				npoly = subpolylist->polygons[j];
				if (subpolylist->lx[j] > hxbound ||
					subpolylist->hx[j] < lxbound ||
					subpolylist->ly[j] > hybound ||
					subpolylist->hy[j] < lybound) continue;
				if (npoly->layer < 0) continue;

				/* see how close they can get */
				nminsize = polyminsize(npoly);
				dist = dr_adjustedmindist(tech, cell->lib, layer,
					minsize, npoly->layer, nminsize, con, multi, &edge, &rule);
				if (dist < 0) continue;

				/* check the distance */
				ret = drcb_checkdist(csap, state, tech, layer, net, geom, poly,
					state->nhpp, npoly->layer, nnet, ngeom, npoly,
						(HASHTABLE *)NULL, con, dist, edge, rule, cell);
				if (ret) { termsearch(search);   return(TRUE); }
			}
		}
	}
	return(FALSE);
}

/*
 * Routine to determine whether node "ni" is a multiple cut contact.
 */
BOOLEAN drcb_ismulticut(NODEINST *ni)
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
 * Routine to compare:
 *    polygon "poly1" object "geom1" layer "layer1" network "net1" net table "hpp1"
 * with:
 *    polygon "poly2" object "geom2" layer "layer2" network "net2" net table "hpp2"
 * The polygons are both in technology "tech" and are connected if "con" is nonzero.
 * They cannot be less than "dist" apart (if "edge" is nonzero, check edges only).
 * "csap" is the hierarchical path information and is zero if the two objects are
 * at the same level of hierarchy.
 *
 * If "csap" is nonzero, this is a cross-hierarchical check.  Polygon 1 is from a
 * pseudo-cell created to examine the vicinity of an instance.  Polygon 2 is from
 * an object inside of the instance.  Checking is in the coordinate space of the instance.
 *
 * A true return indicates that an error has been detected and reported.
 * The network tables are needed only if "geom1" or "geom2"
 * respectively are NODEINSTs.  (for cropping with shapes on the same
 * networks) If "geom1" or "geom2" is an ARCINST, we don't need the
 * corresponding network table, it can be NULL.
 */
BOOLEAN drcb_checkdist(CHECKSHAPEARG *csap, STATE *state, TECHNOLOGY *tech,
	INTBIG layer1, INTBIG net1, GEOM *geom1, POLYGON *poly1, HASHTABLE *hpp1,
	INTBIG layer2, INTBIG net2, GEOM *geom2, POLYGON *poly2, HASHTABLE *hpp2,
	BOOLEAN con, INTBIG dist, INTBIG edge, CHAR *rule, NODEPROTO *cell)
{
	REGISTER BOOLEAN isbox1, isbox2, needboth, withincell, maytouch;
	INTBIG lx1, hx1, ly1, hy1, lx2, hx2, ly2, hy2, xf1, yf1, xf2, yf2;
	CHAR *msg, *sizerule;
	REGISTER POLYGON *origpoly1, *origpoly2;
	REGISTER INTBIG pdx, pdy, pd, pdedge, fun, errtype, minwidth,
		lxb, hxb, lyb, hyb, actual, intervening;
	XARRAY trans1, trans2;
	REGISTER void *infstr;

	/* turn off flag that the nodeinst may be undersized */
	state->tinynodeinst = NONODEINST;

	origpoly1 = poly1;
	origpoly2 = poly2;
	isbox1 = isbox(poly1, &lx1, &hx1, &ly1, &hy1);
	if (!isbox1) getbbox(poly1, &lx1, &hx1, &ly1, &hy1);
	isbox2 = isbox(poly2, &lx2, &hx2, &ly2, &hy2);
	if (!isbox2) getbbox(poly2, &lx2, &hx2, &ly2, &hy2);

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
		if (maytouch)
		{
			/* they are electrically connected: see if they touch */
			pdx = maxi(lx2-hx1, lx1-hx2);
			pdy = maxi(ly2-hy1, ly1-hy2);
			if (pdx == 0 && pdy == 0) pd = 1; else
				pd = maxi(pdx, pdy);
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
						if (!drcb_lookforpoints(state, geom1, geom2, layer1, cell, tech,
							csap, lxb-1, lyb-1, lxb-1, hyb+1, TRUE) &&
								!drcb_lookforpoints(state, geom1, geom2, layer1, cell, tech,
									csap, hxb+1, lyb-1, hxb+1, hyb+1, TRUE))
						{
							lyb -= minwidth/2;   hyb += minwidth/2;
							state->checkdistpoly1rebuild->xv[0] = lxb;
							state->checkdistpoly1rebuild->yv[0] = lyb;
							state->checkdistpoly1rebuild->xv[1] = hxb;
							state->checkdistpoly1rebuild->yv[1] = hyb;
							state->checkdistpoly1rebuild->count = 2;
							state->checkdistpoly1rebuild->style = FILLEDRECT;
							if (csap != 0)
								xformpoly(state->checkdistpoly1rebuild, csap->vtrans);
							drcb_reporterror(state, MINWIDTHERROR, tech, 0, FALSE, minwidth,
								actual, sizerule, state->checkdistpoly1rebuild,
									geom1, layer1, NONET, NOPOLYGON, NOGEOM, 0, NONET);
							return(1);
						}
					} else
					{
						/* vertical abutment: check for minimum width */
						if (!drcb_lookforpoints(state, geom1, geom2, layer1, cell, tech,
							csap, lxb-1, lyb-1, hxb+1, lyb-1, TRUE) &&
								!drcb_lookforpoints(state, geom1, geom2, layer1, cell, tech,
									csap, lxb-1, hyb+1, hxb+1, hyb+1, TRUE))
						{
							lxb -= minwidth/2;   hxb += minwidth/2;
							state->checkdistpoly1rebuild->xv[0] = lxb;
							state->checkdistpoly1rebuild->yv[0] = lyb;
							state->checkdistpoly1rebuild->xv[1] = hxb;
							state->checkdistpoly1rebuild->yv[1] = hyb;
							state->checkdistpoly1rebuild->count = 2;
							state->checkdistpoly1rebuild->style = FILLEDRECT;
							if (csap != 0)
								xformpoly(state->checkdistpoly1rebuild, csap->vtrans);
							drcb_reporterror(state, MINWIDTHERROR, tech, 0, FALSE, minwidth,
								actual, sizerule, state->checkdistpoly1rebuild,
									geom1, layer1, NONET, NOPOLYGON, NOGEOM, 0, NONET);
							return(1);
						}
					}
				}
			}
		}

		/* crop out parts of any arc that is covered by an adjoining node */
		if (geom1->entryisnode)
		{
			if (csap == 0) makerot(geom1->entryaddr.ni, trans1); else
				transcpy(csap->dtrans, trans1);
			if (drcb_cropnodeinst(csap, state, geom1->entryaddr.ni, trans1, hpp1, net1,
				lx1, hx1, ly1, hy2, layer2, net2, geom2, &lx2, &hx2, &ly2, &hy2))
					return(0);
		} else
		{
			if (drcb_croparcinst(state, geom1->entryaddr.ai, layer1, &lx1, &hx1, &ly1, &hy1))
				return(0);
		}
		if (geom2->entryisnode)
		{
			makerot(geom2->entryaddr.ni, trans2);
			if (drcb_cropnodeinst(csap, state, geom2->entryaddr.ni, trans2, hpp2, net2,
				lx2, hx2, ly2, hy2, layer1, net1, geom1, &lx1, &hx1, &ly1, &hy1))
					return(0);
		} else
		{
			if (drcb_croparcinst(state, geom2->entryaddr.ai, layer2, &lx2, &hx2, &ly2, &hy2))
				return(0);
		}

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
				return(0);
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
				return(0);
		}

		/* make sure polygons don't intersect */
		if (polyintersect(poly1, poly2)) pd = 0; else
		{
			/* find distance between polygons */
			pd = polyseparation(poly1, poly2);
		}
	}

	/* see if the design rule is met */
	if (pd >= dist) return(0);
	errtype = SPACINGERROR;

	/*
	 * special case: ignore errors between two active layers connected
	 * to either side of a field-effect transistor that is inside of
	 * the error area.
	 */
	if (drcb_activeontransistor(csap, geom1, layer1, net1, poly1,
		geom2, layer2, net2, poly2, state, tech)) return(0);

	/* special cases if the layers are the same */
	if (samelayer(tech, layer1, layer2))
	{
		/* special case: check for "notch" */
		if (maytouch)
		{
			/* if they touch, it is acceptable */
			if (pd <= 0) return(0);

			/* see if the notch is filled */
			intervening = drcb_findinterveningpoints(poly1, poly2, &xf1, &yf1, &xf2, &yf2);
			if (intervening == 0) return(0);
			if (intervening == 1) needboth = FALSE; else
				needboth = TRUE;
			if (drcb_lookforpoints(state, geom1, geom2, layer1, cell, tech, csap,
				xf1, yf1, xf2, yf2, needboth)) return(0);

			/* look further if on the same net and diagonally separate (1 intervening point) */
			if (net1 == net2 && intervening == 1)
				return(0);
			errtype = NOTCHERROR;
		}
	}

	msg = 0;
	if (state->tinynodeinst != NONODEINST)
	{
		/* see if the node/arc that failed was involved in the error */
		if ((state->tinynodeinst->geom == geom1 || state->tinynodeinst->geom == geom2) &&
			(state->tinyobj == geom1 || state->tinyobj == geom2))
		{
			infstr = initinfstr();
			if (drcb_paralleldrc)
				emutexlock(drcb_mutexio);	/* BEGIN critical section */
			formatinfstr(infstr, _("%s is too small for the %s"),
				describenodeinst(state->tinynodeinst), geomname(state->tinyobj));
			if (drcb_paralleldrc)
				emutexunlock(drcb_mutexio);	/* END critical section */
			(void)allocstring(&msg, returninfstr(infstr), dr_tool->cluster);
		}
	}

	withincell = TRUE;
	if (csap != 0)
	{
		polycopy(state->checkdistpoly1orig, origpoly1);
		polycopy(state->checkdistpoly2orig, origpoly2);
		origpoly1 = state->checkdistpoly1orig;
		origpoly2 = state->checkdistpoly2orig;
		xformpoly(origpoly1, csap->vtrans);
		xformpoly(origpoly2, csap->vtrans);
		withincell = FALSE;
	}

	drcb_reporterror(state, errtype, tech, msg, withincell, dist, pd, rule,
		origpoly1, geom1, layer1, net1,
		origpoly2, geom2, layer2, net2);
	return(1);
}

/*
 * Routine to explore the points (xf1,yf1) and (xf2,yf2) to see if there is
 * geometry on layer "layer" (in cell "cell").  Returns true if there is.
 * If "needboth" is true, both points must have geometry, otherwise only 1 needs it.
 */
BOOLEAN drcb_lookforpoints(STATE *state, GEOM *geom1, GEOM *geom2, INTBIG layer,
	NODEPROTO *cell, TECHNOLOGY *tech, CHECKSHAPEARG *csap,
		INTBIG xf1, INTBIG yf1, INTBIG xf2, INTBIG yf2, BOOLEAN needboth)
{
	INTBIG xf3, yf3, flx, fhx, fly, fhy, lx1, hx1, ly1, hy1;
	BOOLEAN p1found, p2found, p3found, allfound;
	REGISTER GEOM *g;
	REGISTER INTBIG sea;
	REGISTER SHAPE *shape;

	xf3 = (xf1+xf2) / 2;
	yf3 = (yf1+yf2) / 2;

	/* compute bounds for searching inside cells */
	flx = mini(xf1, xf2);   fhx = maxi(xf1, xf2);
	fly = mini(yf1, yf2);   fhy = maxi(yf1, yf2);

	/* search the cell for geometry that fills the notch */
	p1found = p2found = p3found = FALSE;
	allfound = drcb_lookforlayer(state, cell, layer, el_matid, flx, fhx, fly, fhy,
		xf1, yf1, &p1found, xf2, yf2, &p2found, xf3, yf3, &p3found);
	if (needboth)
	{
		if (allfound) return(TRUE);
	} else
	{
		if (p1found || p2found) return(TRUE);
	}

	/* in hierarchical situations, search the pseudo-cell for layers that cover the notch */
	if (csap != 0)
	{
		sea = initsearch(flx, fhx, fly, fhy, csap->intersectlist);
		if (sea < 0) return(FALSE);
		for(;;)
		{
			g = nextobject(sea);
			if (g == NOGEOM) break;
			if (g == geom1 || g == geom2) continue;
			shape = (SHAPE *)g->entryaddr.blind;
			if (!samelayer(tech, shape->layer, layer)) continue;

			if (shape->poly != NOPOLYGON)
			{
				if (isinside(xf1, yf1, shape->poly)) p1found = TRUE;
				if (isinside(xf2, yf2, shape->poly)) p2found = TRUE;
				if (isinside(xf3, yf3, shape->poly)) p3found = TRUE;
			} else
			{
				lx1 = g->lowx;   hx1 = g->highx;
				ly1 = g->lowy;   hy1 = g->highy;

				if (xf1 >= lx1 && xf1 <= hx1 && yf1 >= ly1 && yf1 <= hy1) p1found = TRUE;
				if (xf2 >= lx1 && xf2 <= hx1 && yf2 >= ly1 && yf2 <= hy1) p2found = TRUE;
				if (xf3 >= lx1 && xf3 <= hx1 && yf3 >= ly1 && yf3 <= hy1) p3found = TRUE;
			}
			if (needboth)
			{
				if (p1found && p2found && p3found)
				{
					termsearch(sea);
					return(TRUE);
				}
			} else
			{
				if (p1found || p2found)
				{
					termsearch(sea);
					return(TRUE);
				}
			}
		}
	}

	return(FALSE);
}

/*
 * Routine to find two points between polygons "poly1" and "poly2" that can be used to test
 * for notches.  The points are returned in (xf1,yf1) and (xf2,yf2).  Returns zero if no
 * points exist in the gap between the polygons (becuase they don't have common facing edges).
 * Returns 1 if only one of the reported points needs to be filled (because the polygons meet
 * at a point).  Returns 2 if both reported points need to be filled.
 */
INTBIG drcb_findinterveningpoints(POLYGON *poly1, POLYGON *poly2, INTBIG *xf1, INTBIG *yf1,
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

	/* boxes are a nonmanhattan situation */
	*xf1 = (lx1 + hx1) / 2;   *yf1 = (ly2 + hy2) / 2;
	*xf2 = (lx2 + hx2) / 2;   *yf2 = (ly1 + hy1) / 2;
	return(1);
}

/*
 * Routine to examine cell "cell" in the area (lx<=X<=hx, ly<=Y<=hy) for objects
 * on layer "layer".  Apply transformation "moretrans" to the objects.  If polygons are
 * found at (xf1,yf1) or (xf2,yf2) or (xf3,yf3) then sets "p1found/p2found/p3found" to 1.
 * If all locations are found, returns true.
 */
BOOLEAN drcb_lookforlayer(STATE *state, NODEPROTO *cell, INTBIG layer, XARRAY moretrans,
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
	REGISTER POLYLIST *plist;

	plist = state->lookforlayerpolylist;
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
				if (drcb_lookforlayer(state, ni->proto, layer, trans, newlx, newhx, newly, newhy,
					xf1, yf1, p1found, xf2, yf2, p2found, xf3, yf3, p3found))
						return(TRUE);
				continue;
			}
			makerot(ni, rot);
			transmult(rot, moretrans, bound);
			if ((drcb_options&DRCREASONABLE) != 0) reasonable = TRUE; else reasonable = FALSE;
			tot = allnodepolys(ni, plist, NOWINDOWPART, reasonable);
			for(i=0; i<tot; i++)
			{
				poly = plist->polygons[i];
				if (!samelayer(poly->tech, poly->layer, layer)) continue;
				xformpoly(poly, bound);
				if (isinside(xf1, yf1, poly)) *p1found = TRUE;
				if (isinside(xf2, yf2, poly)) *p2found = TRUE;
				if (isinside(xf3, yf3, poly)) *p3found = TRUE;
			}
		} else
		{
			ai = g->entryaddr.ai;
			tot = allarcpolys(ai, plist, NOWINDOWPART);
			for(i=0; i<tot; i++)
			{
				poly = plist->polygons[i];
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
BOOLEAN drcb_activeontransistor(CHECKSHAPEARG *csap, GEOM *geom1, INTBIG layer1,
	INTBIG net1, POLYGON *poly1, GEOM *geom2, INTBIG layer2, INTBIG net2,
		POLYGON *poly2, STATE *state, TECHNOLOGY *tech)
{
	REGISTER INTBIG fun, sea, xf3, yf3, net, cx, cy, blx, bhx, bly, bhy;
	INTBIG lx1, hx1, ly1, hy1, lx2, hx2, ly2, hy2, xf1, yf1, xf2, yf2;
	REGISTER BOOLEAN on1, on2, p1found, p2found, p3found;
	REGISTER GEOM *g;
	REGISTER SHAPE *shape;
	REGISTER NODEINST *ni;
	REGISTER PORTPROTO *badport;
	REGISTER PORTARCINST *pi;
	REGISTER HASHTABLE *hpp;

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
	sea = initsearch(blx, bhx, bly, bhy, geomparent(geom2));
	if (sea < 0) return(FALSE);
	for(;;)
	{
		g = nextobject(sea);
		if (g == NOGEOM) break;
		if (!isfet(g)) continue;

		/* got a transistor */
		ni = g->entryaddr.ni;

		/* must be inside of the bounding box of the desired layers */
		cx = (ni->geom->lowx + ni->geom->highx) / 2;
		cy = (ni->geom->lowy + ni->geom->highy) / 2;
		if (cx < blx || cx > bhx || cy < bly || cy > bhy) continue;

		if (state->hni == NULL) continue;
		hpp = (HASHTABLE *)drcb_hashsearch(state->hni, (CHAR *)ni);
		if (hpp == NULL)
		{
			ttyputerr(x_("DRC error 7 locating node %s in hash table (csap=%ld)"),
				describenodeinst(ni), csap);
			continue;
		}

		/* determine the poly port */
		badport = ni->proto->firstportproto;

		on1 = on2 = FALSE;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			/* ignore connections on poly/gate */
			if (pi->proto->network == badport->network) continue;

			net = drcb_network(hpp, (CHAR *)pi->proto, 1);
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

	if (csap != 0)
	{
		/* working across hierarchical bounds: look for polysilicon layer in pseudo-cell */
		if (drcb_findinterveningpoints(poly1, poly2, &xf1, &yf1, &xf2, &yf2) == 0) return(FALSE);
		xf3 = (xf1+xf2) / 2;
		yf3 = (yf1+yf2) / 2;
		p1found = p2found = p3found = FALSE;
		sea = initsearch(mini(xf1,xf2), maxi(xf1,xf2), mini(yf1,yf2), maxi(yf1,yf2),
			csap->intersectlist);
		if (sea < 0) return(FALSE);
		for(;;)
		{
			g = nextobject(sea);
			if (g == NOGEOM) break;
			shape = (SHAPE *)g->entryaddr.blind;
			fun = layerfunction(tech, shape->layer) & LFTYPE;
			if (!layerispoly(fun)) continue;

			if (shape->poly != NOPOLYGON)
			{
				if (isinside(xf1, yf1, shape->poly)) p1found = TRUE;
				if (isinside(xf2, yf2, shape->poly)) p2found = TRUE;
				if (isinside(xf3, yf3, shape->poly)) p3found = TRUE;
			} else
			{
				lx1 = g->lowx;   hx1 = g->highx;
				ly1 = g->lowy;   hy1 = g->highy;

				if (xf1 >= lx1 && xf1 <= hx1 && yf1 >= ly1 && yf1 <= hy1) p1found = TRUE;
				if (xf2 >= lx1 && xf2 <= hx1 && yf2 >= ly1 && yf2 <= hy1) p2found = TRUE;
				if (xf3 >= lx1 && xf3 <= hx1 && yf3 >= ly1 && yf3 <= hy1) p3found = TRUE;
			}
			if (p1found && p2found && p3found)
			{
				termsearch(sea);
				return(TRUE);
			}
		}
	}
	return(FALSE);
}

/*
 * routine to see if polygons in "plist" (describing arc "ai") should be cropped against a
 * connecting transistor.  Crops the polygon if so.
 */
void drcb_cropactivearc(STATE *state, ARCINST *ai, POLYLIST *plist)
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
		drcb_getnodepolys(ni, state->activecroppolylist, trans);
		ntot = state->activecroppolylist->polylistcount;
		for(k=0; k<ntot; k++)
		{
			npoly = state->activecroppolylist->polygons[k];
			if (npoly->tech != poly->tech || npoly->layer != poly->layer) continue;
			if (!isbox(npoly, &nlx, &nhx, &nly, &nhy)) continue;
			if (drcb_halfcropbox(&lx, &hx, &ly, &hy, nlx, nhx, nly, nhy) == 1)
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
 * routine to crop the box on layer "nlayer", electrical index "nnet"
 * and bounds (lx-hx, ly-hy) against the nodeinst "ni".  The geometry in nodeinst "ni"
 * that is being checked runs from (nlx-nhx, nly-nhy).  Only those layers
 * in the nodeinst that are the same layer and the same electrical network
 * are checked.  The routine returns true if the bounds are reduced
 * to nothing.
 */
BOOLEAN drcb_cropnodeinst(CHECKSHAPEARG *csap, STATE *state, NODEINST *ni,
	XARRAY trans, HASHTABLE *hpp, INTBIG ninet, INTBIG nlx, INTBIG nhx, INTBIG nly, INTBIG nhy,
	INTBIG nlayer, INTBIG nnet, GEOM *ngeom, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy)
{
	INTBIG xl, xh, yl, yh;
	REGISTER INTBIG tot, j, isconnected;
	REGISTER BOOLEAN allgone;
	REGISTER INTBIG temp, net;
	REGISTER POLYLIST *polylist;
	REGISTER POLYGON *poly;

	polylist = state->croppolylist;
	drcb_getnodeEpolys(ni, polylist, el_matid);
	tot = polylist->polylistcount;
	if (tot < 0) return(FALSE);
	isconnected = 0;
	for(j=0; j<tot; j++)
	{
		poly = polylist->polygons[j];
		if (!samelayer(poly->tech, poly->layer, nlayer)) continue;
		if (nnet != NONET)
		{
			if (poly->portproto == NOPORTPROTO) continue;
			net = drcb_network(hpp, (CHAR *)poly->portproto, 1);
			if (net != NONET && net != nnet) continue;
		}
		isconnected++;
		break;
	}
	if (isconnected == 0) return(FALSE);

	/* get the description of the nodeinst layers */
	allgone = FALSE;
	drcb_getnodepolys(ni, polylist, trans);
	tot = polylist->polylistcount;
	if (tot < 0) return(FALSE);
	for(j=0; j<tot; j++)
	{
		poly = polylist->polygons[j];
		if (!samelayer(poly->tech, poly->layer, nlayer)) continue;

		/* warning: does not handle arbitrary polygons, only boxes */
		if (!isbox(poly, &xl, &xh, &yl, &yh)) continue;
		temp = drcb_cropbox(lx, hx, ly, hy, xl, xh, yl, yh, nlx, nhx, nly, nhy);
		if (temp > 0) { allgone = TRUE; break; }
		if (temp < 0)
		{
			state->tinynodeinst = ni;
			state->tinyobj = ngeom;
		}
	}
	return(allgone);
}

/*
 * routine to crop the box in the reference parameters (lx-hx, ly-hy)
 * against the box in (bx-ux, by-uy).  If the box is cropped into oblivion,
 * the routine returns 1.  If the boxes overlap but cannot be cleanly cropped,
 * the routine returns -1.  Otherwise the box is cropped and zero is returned
 */
INTBIG drcb_cropbox(INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy, INTBIG bx, INTBIG ux, INTBIG by,
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
		/* if the target object is within the "against" box, allow the crop */
		if (ux < *hx && (nhx >= *hx || nhx <= ux)) ux = *hx;
		if (bx > *lx && (nlx <= *lx || nlx >= bx)) bx = *lx;

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
		/* if the target object is within the "against" box, allow the crop */
		if (uy < *hy && (nhy >= *hy || nhy <= uy)) uy = *hy;
		if (by > *ly && (nly <= *ly || nly >= by)) by = *ly;

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
 * routine to crop away any part of layer "lay" of arcinst "ai" that coincides
 * with a similar layer on a connecting nodeinst.  The bounds of the arcinst
 * are in the reference parameters (lx-hx, ly-hy).  The routine returns false
 * normally, 1 if the arcinst is cropped into oblivion.
 */
BOOLEAN drcb_croparcinst(STATE *state, ARCINST *ai, INTBIG lay,
	INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy)
{
	INTBIG xl, xh, yl, yh;
	XARRAY trans;
	REGISTER INTBIG i, j, tot;
	REGISTER INTBIG temp;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
	REGISTER POLYLIST *polylist;
	REGISTER POLYGON *poly;

	polylist = state->croppolylist;
	for(i=0; i<2; i++)
	{
		/* find the primitive nodeinst at the true end of the portinst */
		ni = ai->end[i].nodeinst;   np = ni->proto;
		pp = ai->end[i].portarcinst->proto;
		while (np->primindex == 0)
		{
			ni = pp->subnodeinst;   np = ni->proto;
			pp = pp->subportproto;
		}
		makerot(ni, trans);
		drcb_getnodepolys(ni, polylist, trans);
		tot = polylist->polylistcount;
		for(j=0; j<tot; j++)
		{
			poly = polylist->polygons[j];
			if (!samelayer(poly->tech, poly->layer, lay)) continue;

			/* warning: does not handle arbitrary polygons, only boxes */
			if (!isbox(poly, &xl, &xh, &yl, &yh)) continue;
			temp = drcb_halfcropbox(lx, hx, ly, hy, xl, xh, yl, yh);
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
 * routine to crop the box in the reference parameters (lx-hx, ly-hy)
 * against the box in (bx-ux, by-uy).  If the box is cropped into oblivion,
 * the routine returns 1.  If the boxes overlap but cannot be cleanly cropped,
 * the routine returns -1.  Otherwise the box is cropped and zero is returned
 */
INTBIG drcb_halfcropbox(INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy,
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
 * routine to tell whether the objects at geometry modules "geom1" and "geom2"
 * touch directly (that is, an arcinst connected to a nodeinst).  The routine
 * returns true if they touch
 */
BOOLEAN drcb_objtouch(GEOM *geom1, GEOM *geom2)
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

/***************** SUPPORT ******************/

/*
 * Initializes the port numbers for a toplevel nodeproto np, starting
 * the net numbers from *netindex, and storing all portprotos and their
 * nets in ht
 */
HASHTABLE *drcb_getinitnets(NODEPROTO *np, INTBIG *netindex)
{
	REGISTER PORTPROTO *pp, *spp;
	REGISTER CHAR *cp;
	REGISTER HASHTABLE *ht;

	ht = drcb_hashcreate(HTABLE_PORT, 1, np);
	if (ht == NOHASHTABLE) return(NOHASHTABLE);

	/* re-assign net-list values, starting at each port of the top cell */
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		for(spp = np->firstportproto; spp != pp; spp = spp->nextportproto)
		{
			if (spp->network != pp->network) continue;

			/* this port is the same as another one, use same net */
			cp = drcb_hashsearch(ht, (CHAR *)spp);
			if (cp == NULL)
			{
				ttyputerr(x_("DRC error 8 locating port %s in hash table"), spp->protoname);
				return(NOHASHTABLE);
			}
			if (drcb_hashinsert(ht, (CHAR *)pp, cp, 0) != 0)
			{
				ttyputerr(x_("DRC error 9 inserting port %s into hash table"), pp->protoname);
				return(NOHASHTABLE);
			}
			break;
		}

		/* assign a new net number if the loop terminated normally */
		if (spp == pp)
		{
			cp = (CHAR *)*netindex;
			if (drcb_hashinsert(ht, (CHAR *)pp, cp, 0) != 0)
			{
				ttyputerr(x_("DRC error 10 inserting port %s into hash table"), pp->protoname);
				return(NOHASHTABLE);
			}
			(*netindex)++;
		}
	}
	return(ht);
}

/*
 * generate a hash table of (arcinst, network numbers) pairs, keyed by
 * arcinst for a NODEPROTO from the hash table for portprotos (hpp) in
 * the parent NODEPROTO of the NODEINST.  The net numbers are propagated
 * to all arcs.  Returns the table in hai, a hash table passed in by
 * the parent.
 */
HASHTABLE *drcb_getarcnetworks(NODEPROTO *np, HASHTABLE *hpp, INTBIG *netindex)
{
	REGISTER CHAR *cp;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp;
	REGISTER PORTARCINST *pi;
	REGISTER HASHTABLE *hai;

	hai = drcb_hashcreate(HTABLE_NODEARC, 3, np);
	if (hai == NOHASHTABLE) return(NOHASHTABLE);

	/* propagate port numbers to arcs in the nodeproto */
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		/* find a connecting arc on the node that this port comes from */
		for(pi = pp->subnodeinst->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			ai = pi->conarcinst;
			if (ai->network != pp->network) continue;
			if (drcb_hashsearch(hai, (CHAR *)ai) != 0) continue;
			cp = drcb_hashsearch(hpp, (CHAR *)pp);
			if (cp == NULL)
			{
				ttyputerr(x_("DRC error 11 locating port %s in hash table"), pp->protoname);
				return(NOHASHTABLE);
			}
			drcb_flatprop2(ai, hai, cp);
			break;
		}
	}

	/* set node numbers on arcs that are local */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		cp = drcb_hashsearch(hai, (CHAR *)ai);
		if (cp == NULL)
		{
			drcb_flatprop2(ai, hai, (CHAR *)*netindex);
			(*netindex)++;
		}
	}
	return(hai);
}

/*
 * routine to propagate the node number "cindex" to arcs
 * connected to arc "ai".
 */
void drcb_flatprop2(ARCINST *ai, HASHTABLE *hai, CHAR *cindex)
{
	REGISTER ARCINST *oai;
	REGISTER NODEINST *ni;
	REGISTER PORTARCINST *pi;
	REGISTER INTBIG i;

	/* set this arcinst to the current node number */
	if (drcb_hashinsert(hai, (CHAR *)ai, cindex, 0) != 0)
	{
		ttyputerr(x_("DRC error 12 inserting arc %s into hash table"), describearcinst(ai));
		return;
	}

	/* if signals do not flow through this arc, do not propagate */
	if (((ai->proto->userbits&AFUNCTION) >> AFUNCTIONSH) == APNONELEC) return;

	/* recursively set all arcs and nodes touching this */
	for(i=0; i<2; i++)
	{
		if ((ai->end[i].portarcinst->proto->userbits&PORTISOLATED) != 0) continue;
		ni = ai->end[i].nodeinst;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			/* select an arc that has not been examined */
			oai = pi->conarcinst;
			if (drcb_hashsearch(hai, (CHAR *)oai) != 0) continue;

			/* see if the two ports connect electrically */
			if (ai->end[i].portarcinst->proto->network != pi->proto->network) continue;

			/* recurse on the nodes of this arc */
			drcb_flatprop2(oai, hai, cindex);
		}
	}
}

/*
 * generate a hash table of (portproto, network numbers) pairs, keyed by
 * portproto for a NODEINST from corresponding hash tables for
 * portprotos (hpp) and arcs (hai) in the NODEINST's parent proto.  The
 * net numbers are derived from the arcs and exports in the
 * NODEINST.  Returns the table in nhpp, a hash table passed in by the
 * parent.
 */
void drcb_getnodenetworks(NODEINST *ni, HASHTABLE *hpp, HASHTABLE *hai,
	HASHTABLE *nhpp, INTBIG *netindex)
{
	REGISTER CHAR *cp;
	REGISTER PORTPROTO *pp, *spp;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;

	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		pp = pi->proto;
		if (drcb_hashsearch(nhpp, (CHAR *)pp) != 0) continue;
		cp = drcb_hashsearch(hai, (CHAR *)pi->conarcinst);
		if (cp == 0)
		{
			ttyputerr(x_("DRC error 13 locating arc %s in hash table"),
				describearcinst(pi->conarcinst));
			return;
		}
		if (drcb_hashinsert(nhpp, (CHAR *)pp, cp, 0) != 0)
		{
			ttyputerr(x_("DRC error 14 inserting port %s into hash table"), pp->protoname);
			return;
		}
	}
	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
	{
		pp = pe->proto;
		if (drcb_hashsearch(nhpp, (CHAR *)pp) != 0) continue;
		cp = drcb_hashsearch(hpp, (CHAR *)pe->exportproto);
		if (cp == 0)
		{
			ttyputerr(x_("DRC error 15 locating port %s in hash table"),
				pe->exportproto->protoname);
			return;
		}
		if (drcb_hashinsert(nhpp, (CHAR *)pp, cp, 0) != 0)
		{
			ttyputerr(x_("DRC error 16 inserting port %s into hash table"), pp->protoname);
			return;
		}
	}

	/* look for unassigned ports and give new or copied network numbers */
	for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		if (drcb_hashsearch(nhpp, (CHAR *)pp) != 0) continue;
		cp = 0;
		for(spp = ni->proto->firstportproto; spp != NOPORTPROTO;
			spp = spp->nextportproto)
		{
			if (spp->network != pp->network) continue;
			cp = drcb_hashsearch(nhpp, (CHAR *)spp);
			if (cp == 0) continue;
			if (drcb_hashinsert(nhpp, (CHAR *)pp, cp, 0) != 0)
			{
				ttyputerr(x_("DRC error 17 inserting port %s into hash table"),
					pp->protoname);
				return;
			}
			break;
		}
		if (cp == 0)
		{
			cp = (CHAR *)*netindex;
			if (drcb_hashinsert(nhpp, (CHAR *)pp, cp, 0) != 0)
			{
				ttyputerr(x_("DRC error 18 inserting port %s into hash table"),
					pp->protoname);
				return;
			}
			(*netindex)++;
		}
	}
}

/***************** LAYER INTERACTIONS ******************/

/*
 * Routine to build the internal data structures that tell which layers interact with
 * which primitive nodes in technology "tech".
 */
void drcb_buildlayerinteractions(TECHNOLOGY *tech)
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

	drcb_layerintertech = tech;

	/* build the node table */
	tablesize = pickprime(tech->nodeprotocount * 2);
	if (tablesize > drcb_layerinternodehashsize)
	{
		if (drcb_layerinternodehashsize > 0)
		{
			for(i=0; i<drcb_layerinternodehashsize; i++)
				if (drcb_layerinternodetable[i] != 0)
					efree((CHAR *)drcb_layerinternodetable[i]);
			efree((CHAR *)drcb_layerinternodetable);
			efree((CHAR *)drcb_layerinternodehash);
		}
		drcb_layerinternodehashsize = 0;
		drcb_layerinternodehash = (NODEPROTO **)emalloc(tablesize * (sizeof (NODEPROTO *)),
			dr_tool->cluster);
		if (drcb_layerinternodehash == 0) return;
		drcb_layerinternodetable = (BOOLEAN **)emalloc(tablesize * (sizeof (BOOLEAN *)),
			dr_tool->cluster);
		if (drcb_layerinternodetable == 0) return;
		drcb_layerinternodehashsize = tablesize;
	}
	for(i=0; i<drcb_layerinternodehashsize; i++)
	{
		drcb_layerinternodehash[i] = NONODEPROTO;
		drcb_layerinternodetable[i] = 0;
	}

	poly = allocpolygon(4, dr_tool->cluster);
	for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		index = ((INTBIG)np) % drcb_layerinternodehashsize;
		for(i=0; i<drcb_layerinternodehashsize; i++)
		{
			if (drcb_layerinternodehash[index] == NONODEPROTO) break;
			index++;
			if (index >= drcb_layerinternodehashsize) index = 0;
		}
		drcb_layerinternodehash[index] = np;
		drcb_layerinternodetable[index] = (BOOLEAN *)emalloc(tech->layercount * (sizeof (BOOLEAN)),
			dr_tool->cluster);
		for(i=0; i<tech->layercount; i++) drcb_layerinternodetable[index][i] = FALSE;

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
				drcb_layerinternodetable[index][layer] = TRUE;
			}
		}
	}

	/* build the arc table */
	tablesize = pickprime(tech->arcprotocount * 2);
	if (tablesize > drcb_layerinterarchashsize)
	{
		if (drcb_layerinterarchashsize > 0)
		{
			for(i=0; i<drcb_layerinterarchashsize; i++)
				if (drcb_layerinterarctable[i] != 0)
					efree((CHAR *)drcb_layerinterarctable[i]);
			efree((CHAR *)drcb_layerinterarctable);
			efree((CHAR *)drcb_layerinterarchash);
		}
		drcb_layerinterarchashsize = 0;
		drcb_layerinterarchash = (ARCPROTO **)emalloc(tablesize * (sizeof (ARCPROTO *)),
			dr_tool->cluster);
		if (drcb_layerinterarchash == 0) return;
		drcb_layerinterarctable = (BOOLEAN **)emalloc(tablesize * (sizeof (BOOLEAN *)),
			dr_tool->cluster);
		if (drcb_layerinterarctable == 0) return;
		drcb_layerinterarchashsize = tablesize;
	}
	for(i=0; i<drcb_layerinterarchashsize; i++)
	{
		drcb_layerinterarchash[i] = NOARCPROTO;
		drcb_layerinterarctable[i] = 0;
	}

	for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
	{
		index = ((INTBIG)ap) % drcb_layerinterarchashsize;
		for(i=0; i<drcb_layerinterarchashsize; i++)
		{
			if (drcb_layerinterarchash[index] == NOARCPROTO) break;
			index++;
			if (index >= drcb_layerinterarchashsize) index = 0;
		}
		drcb_layerinterarchash[index] = ap;
		drcb_layerinterarctable[index] = (BOOLEAN *)emalloc(tech->layercount * (sizeof (BOOLEAN)),
			dr_tool->cluster);
		for(i=0; i<tech->layercount; i++) drcb_layerinterarctable[index][i] = FALSE;

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
				drcb_layerinterarctable[index][layer] = TRUE;
			}
		}
	}
	freepolygon(poly);
}

/*
 * Routine to determine whether layer "layer" interacts in any way with a node of type "np".
 * If not, returns FALSE.
 */
BOOLEAN drcb_checklayerwithnode(INTBIG layer, NODEPROTO *np)
{
	REGISTER INTBIG index, i;

	if (np->primindex == 0) return(FALSE);
	if (np->tech != drcb_layerintertech)
	{
		drcb_buildlayerinteractions(np->tech);
	}
	if (drcb_layerinternodehashsize <= 0) return(TRUE);

	/* find this node in the table */
	index = ((INTBIG)np) % drcb_layerinternodehashsize;
	for(i=0; i<drcb_layerinternodehashsize; i++)
	{
		if (drcb_layerinternodehash[index] == np) break;
		index++;
		if (index >= drcb_layerinternodehashsize) index = 0;
	}
	if (i >= drcb_layerinternodehashsize) return(FALSE);
	return(drcb_layerinternodetable[index][layer]);
}

/*
 * Routine to determine whether layer "layer" interacts in any way with an arc of type "ap".
 * If not, returns FALSE.
 */
BOOLEAN drcb_checklayerwitharc(INTBIG layer, ARCPROTO *ap)
{
	REGISTER INTBIG index, i;

	if (ap->tech != drcb_layerintertech)
	{
		drcb_buildlayerinteractions(ap->tech);
	}
	if (drcb_layerinterarchashsize <= 0) return(TRUE);

	/* find this arc in the table */
	index = ((INTBIG)ap) % drcb_layerinterarchashsize;
	for(i=0; i<drcb_layerinterarchashsize; i++)
	{
		if (drcb_layerinterarchash[index] == ap) break;
		index++;
		if (index >= drcb_layerinterarchashsize) index = 0;
	}
	if (i >= drcb_layerinterarchashsize) return(FALSE);
	return(drcb_layerinterarctable[index][layer]);
}

/***************** MATHEMATICS ******************/

/*
 * Computes the intersection of the two boxes. Returns nonzero if they
 * intersect, zero if they do not intersect.  Intersection is strict:
 * if the boxes touch, the check returns true.
 */
BOOLEAN drcb_boxesintersect(INTBIG lx1p, INTBIG hx1p, INTBIG ly1p, INTBIG hy1p,
	INTBIG lx2, INTBIG hx2, INTBIG ly2, INTBIG hy2)
{
	if (mini(hx1p, hx2) <= maxi(lx1p, lx2)) return(FALSE);
	if (mini(hy1p, hy2) <= maxi(ly1p, ly2)) return(FALSE);
	return(TRUE);
}

/***************** PRIMITIVE NODE/ARCINST TO SHAPES  ******************/

/*
 * These routines get all the polygons in a primitive instance, and
 * store them in the POLYLIST structure.
 */
void drcb_getnodeEpolys(NODEINST *ni, POLYLIST *plist, XARRAY trans)
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
	if ((drcb_options&DRCREASONABLE) != 0) onlyreasonable = TRUE; else
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

void drcb_getnodepolys(NODEINST *ni, POLYLIST *plist, XARRAY trans)
{
	REGISTER INTBIG j;
	BOOLEAN onlyreasonable;

	if ((drcb_options&DRCREASONABLE) != 0) onlyreasonable = TRUE; else
		onlyreasonable = FALSE;
	plist->polylistcount = allnodepolys(ni, plist, NOWINDOWPART, onlyreasonable);
	for(j = 0; j < plist->polylistcount; j++)
	{
		xformpoly(plist->polygons[j], trans);
		getbbox(plist->polygons[j], &plist->lx[j], &plist->hx[j],
			&plist->ly[j], &plist->hy[j]);
	}
}

void drcb_getarcpolys(ARCINST *ai, POLYLIST *plist)
{
	REGISTER INTBIG j;
	REGISTER POLYGON *poly;

	plist->polylistcount = allarcpolys(ai, plist, NOWINDOWPART);
	for(j = 0; j < plist->polylistcount; j++)
	{
		poly = plist->polygons[j];
		getbbox(poly, &plist->lx[j], &plist->hx[j],
			&plist->ly[j], &plist->hy[j]);
	}
}

/*********************************** HASH TABLE ***********************************/

/*
 * Creates a hashtable 'size' elements long. size should be a prime
 * number. The user should not poke around inside the hash table
 */
HASHTABLE *drcb_hashcreate(INTBIG size, INTBIG type, NODEPROTO *np)
{
	REGISTER HASHTABLE *newh;
	REGISTER INTBIG i, len;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp;

	len = 0;
	switch (type)
	{
		case 1:
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				pp->temp1 = len++;
			break;
		case 2:
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				ni->temp1 = len++;
			break;
		case 3:
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
				ai->temp1 = len++;
			break;
	}
	if (len <= 0) len = 1;

	/* allocate the table */
	newh = drcb_allochashtable(len);
	if (newh == NULL) return(NOHASHTABLE);
	newh->thetype = type;
	for(i=0; i<len; i++) newh->thelist[i] = 0;
	newh->thelen = len;
	return(newh);
}

/*
 * Frees a created hash table. It first walks the hash table, calling
 * free_proc for every element in the table. free_proc should return
 * NULL on each element or the free walk will terminate. The table will
 * be freed anyway.
 */
void drcb_hashdestroy(HASHTABLE *hash)
{
	drcb_freehashtable(hash);
}

/*
 * copies a table to a new table.  returns 0 if it failed, or the new
 * table if it succeeded.
 */
HASHTABLE *drcb_hashcopy(HASHTABLE *hash)
{
	REGISTER HASHTABLE *newh;
	REGISTER INTBIG i;

	newh = drcb_allochashtable(hash->thelen);
	if (newh == NULL) return(NOHASHTABLE);
	newh->thetype = hash->thetype;
	for(i=0; i<hash->thelen; i++) newh->thelist[i] = hash->thelist[i];
	newh->thelen = hash->thelen;
	return(newh);
}

/*
 * calls proc(key, datum, arg) for each item in the hash table. proc may
 * return a char *, in which case the walk terminates and the value is
 * returned to the caller
 */
CHAR *drcb_hashwalk(HASHTABLE *hash, CHAR *(*proc)(CHAR*, CHAR*, CHAR*), CHAR *arg)
{
	REGISTER INTBIG i;
	REGISTER CHAR *ret;

	for(i=0; i<hash->thelen; i++)
	{
		if (hash->thelist[i] != NULL)
		{
			ret = (*proc)((CHAR *)i, hash->thelist[i], arg);
			if (ret != NULL) return ret;
		}
	}
	return(0);
}

/*
 * inserts a pointer item in the hash table. returns 0 if it succeeds, If
 * replace is true, and the key already exists in the table, then it
 * replaces the old datum with the new one. If replace is false, the it
 * only inserts the datum if the key is not found in the table and
 * there's enough space. if replace is false, it returns HASH_EXISTS if
 * it finds the item in the table already, HASH_FULL if the table has no
 * room in it and it was unable to grow it to a new size.
 */
INTBIG drcb_hashinsert(HASHTABLE *hash, CHAR *key, CHAR *datum, INTBIG replace)
{
	REGISTER PORTPROTO *pp;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;

	switch (hash->thetype)
	{
		case 1:
			pp = (PORTPROTO *)key;
			hash->thelist[pp->temp1] = datum;
			break;
		case 2:
			ni = (NODEINST *)key;
			hash->thelist[ni->temp1] = datum;
			break;
		case 3:
			ai = (ARCINST *)key;
			hash->thelist[ai->temp1] = datum;
			break;
	}
	return(0);
}

CHAR *drcb_hashsearch(HASHTABLE *hash, CHAR *key)
{
	REGISTER PORTPROTO *pp;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;

	switch (hash->thetype)
	{
		case 1:
			pp = (PORTPROTO *)key;
			return(hash->thelist[pp->temp1]);
		case 2:
			ni = (NODEINST *)key;
			return(hash->thelist[ni->temp1]);
		case 3:
			ai = (ARCINST *)key;
			return(hash->thelist[ai->temp1]);
	}
	return(0);
}

/************************ ALLOCATION ************************/

/* Frees the list of intersecting elements in a node instance */
void drcb_freeintersectingelements(NODEPROTO *np, STATE *state)
{
	if (np == NONODEPROTO) return;
	drcb_freertree(np->rtree, state);
	np->rtree = NORTNODE;
	freenodeproto(np);
}

/*
 * Frees an R Tree hierarchy - traversing the tree in a depth first
 * search free-ing all nodes and their contents. This should probably be
 * an Electric database routine, but right now, it has knowledge of the
 * shape wired in here.  Should be abstracted out into a hook routine!!
 */
void drcb_freertree(RTNODE *rtn, STATE *state)
{
	REGISTER INTBIG j;
	REGISTER SHAPE *shape;

	if (rtn->flag != 0)
	{
		for(j = 0; j < rtn->total; j++)
		{
			/* free shape */
			shape = (SHAPE *)(((GEOM *)(rtn->pointers[j]))->entryaddr.blind);
			if (shape->hpp)
				drcb_hashdestroy(shape->hpp);
			if (shape->poly != NOPOLYGON)
				freepolygon(shape->poly);
			drcb_freeshape(shape, state);
			freegeom((GEOM *)(rtn->pointers[j]));
			rtn->pointers[j] = 0;
		}
		freertnode(rtn);
		return;
	}

	for(j = 0; j < rtn->total; j++)
	{
		drcb_freertree((RTNODE *)rtn->pointers[j], state);
		rtn->pointers[j] = 0;
	}
	freertnode(rtn);
}

SHAPE *drcb_allocshape(STATE *state)
{
	REGISTER SHAPE *shape;

	if (state->freeshapes != NOSHAPE)
	{
		shape = state->freeshapes;
		state->freeshapes = (SHAPE *)shape->hpp;
	} else
	{
		shape = (SHAPE *)emalloc(sizeof(SHAPE), dr_tool->cluster);
	}
	shape->poly = NOPOLYGON;
	return(shape);
}

void drcb_freeshape(SHAPE *shape, STATE *state)
{
	shape->hpp = (HASHTABLE *)state->freeshapes;
	state->freeshapes = shape;
}

/*
 * Routine to allocate a new hash table of size "len".  Returns zero on error.
 */
HASHTABLE *drcb_allochashtable(INTBIG len)
{
	REGISTER HASHTABLE *ht;
	REGISTER CHAR **thelist;

	/* BEGIN critical section */
	if (drcb_paralleldrc) emutexlock(drcb_mutexhash);

	/* grab a hash table object */
	ht = NOHASHTABLE;
	if (drcb_freehashtables != NOHASHTABLE)
	{
		ht = drcb_freehashtables;
		drcb_freehashtables = (HASHTABLE *)ht->thelist;
	}

	/* grab an array of hash pointers */
	thelist = 0;
	if (len <= LISTSIZECACHE && drcb_listcache[len-1] != 0)
	{
		thelist = drcb_listcache[len-1];
		drcb_listcache[len-1] = (CHAR **)(thelist[0]);
	}

	/* END critical section */
	if (drcb_paralleldrc) emutexunlock(drcb_mutexhash);

	/* if no hash table is available, allocate a new one */
	if (ht == NOHASHTABLE)
	{
		ht = (HASHTABLE *)emalloc(sizeof(HASHTABLE), dr_tool->cluster);
		if (ht == 0) return(0);
	}

	/* if no list is available, allocate new space */
	if (thelist == 0)
	{
		thelist = (CHAR **)emalloc(len * (sizeof (CHAR *)), dr_tool->cluster);
		if (thelist == 0) return(0);
	}

	/* put it together and return it */
	ht->thelist = thelist;
	return(ht);
}

/*
 * Routine to free hash table "ht".
 */
void drcb_freehashtable(HASHTABLE *ht)
{
	/* BEGIN critical section */
	if (drcb_paralleldrc) emutexlock(drcb_mutexhash);

	/* first free the array of values */
	if (ht->thelen <= LISTSIZECACHE)
	{
		/* small list: add it to the cache of lists this size */
		ht->thelist[0] = (CHAR *)drcb_listcache[ht->thelen-1];
		drcb_listcache[ht->thelen-1] = ht->thelist;
	} else
	{
		/* big list: just free the space */
		efree((CHAR *)ht->thelist);
	}

	ht->thelist = (CHAR **)drcb_freehashtables;
	drcb_freehashtables = ht;

	/* END critical section */
	if (drcb_paralleldrc) emutexunlock(drcb_mutexhash);
}

/************************ ERROR REPORTING ************************/

/* Adds details about an error to the error list */
void drcb_reporterror(STATE *state, INTBIG errtype, TECHNOLOGY *tech, CHAR *msg,
	BOOLEAN withincell, INTBIG limit, INTBIG actual, CHAR *rule,
	POLYGON *poly1, GEOM *geom1, INTBIG layer1, INTBIG net1,
	POLYGON *poly2, GEOM *geom2, INTBIG layer2, INTBIG net2)
{
	REGISTER NODEPROTO *np, *topcell, *np1, *np2;
	REGISTER VARIABLE *var;
	REGISTER INTBIG i, len, sortlayer, lambda;
	REGISTER BOOLEAN showgeom;
	REGISTER GEOM *p1, *p2;
	REGISTER void *err, *infstr;
	REGISTER CHAR *errmsg;

	/* if this error is being ignored, don't record it */
	if (withincell)
	{
		np = geomparent(geom1);
		var = getvalkey((INTBIG)np, VNODEPROTO, VGEOM|VISARRAY, dr_ignore_listkey);
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
	}
	if (withincell) topcell = geomparent(geom1); else
		topcell = state->topcell;

	lambda = lambdaofcell(topcell);

/* BEGIN critical section */
if (drcb_paralleldrc) emutexlock(drcb_mutexio);

	/* show the error (only for hierarchical, nonparallel) */
	if (!drcb_paralleldrc && drcb_hierarchicalcheck && !withincell &&
		drcb_topcellalways == topcell)
	{
		if (numerrors() == 0)
			(void)asktool(us_tool, x_("clear"));
		drcb_highlighterror(poly1, poly2, topcell);
	}

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
		} else if (np1 != topcell)
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
			formatinfstr(infstr, _(" LESS THAN %s WIDE (IS %s)"), latoa(limit, lambda), latoa(actual, lambda));
		} else if (errtype == MINSIZEERROR)
		{
			formatinfstr(infstr, _(" LESS THAN %s IN SIZE (IS %s)"), latoa(limit, lambda), latoa(actual, lambda));
		}
		sortlayer = layer1;
	}
	if (rule != 0) formatinfstr(infstr, _(" [rule %s]"), rule);
	errmsg = returninfstr(infstr);
	if (dr_logerrors)
	{
		err = logerror(errmsg, topcell, sortlayer);
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
if (drcb_paralleldrc) emutexunlock(drcb_mutexio);
}

void drcb_highlighterror(POLYGON *poly1, POLYGON *poly2, NODEPROTO *cell)
{
	INTBIG lx, hx, ly, hy;
	REGISTER INTBIG i, prev;

	if (isbox(poly1, &lx, &hx, &ly, &hy))
	{
		(void)asktool(us_tool, x_("show-area"), lx, hx, ly, hy, cell);
	} else
	{
		for(i=0; i<poly1->count; i++)
		{
			if (i == 0) prev = poly1->count-1; else prev = i-1;
			(void)asktool(us_tool, x_("show-line"), poly1->xv[prev], poly1->yv[prev],
				poly1->xv[i], poly1->yv[i], cell);
		}
	}
	if (poly2 != NOPOLYGON)
	{
		if (isbox(poly2, &lx, &hx, &ly, &hy))
		{
			(void)asktool(us_tool, x_("show-area"), lx, hx, ly, hy, cell);
		} else
		{
			for(i=0; i<poly2->count; i++)
			{
				if (i == 0) prev = poly2->count-1; else prev = i-1;
				(void)asktool(us_tool, x_("show-line"), poly2->xv[prev], poly2->yv[prev],
					poly2->xv[i], poly2->yv[i], cell);
			}
		}
	}
	flushscreen();
}

#endif  /* DRCTOOL - at top */
