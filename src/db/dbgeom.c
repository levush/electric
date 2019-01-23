/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dbgeom.c
 * Database geometry and search modules
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
#include "database.h"
#include "egraphics.h"
#include "tech.h"
#include "tecgen.h"
#include "tecschem.h"
#include "tecart.h"
#include <math.h>

#define mmini(a, b) ((a) < (b) ? (a) : (b))
#define mmaxi(a, b) ((a) > (b) ? (a) : (b))

#define MAXDEPTH 100			/* maximum depth of tree (for search) */
/* #define USEFLOATS */			/* uncomment to use floating point */

#define NORSEARCH ((RSEARCH *)-1)
typedef struct
{
	RTNODE  *rtn[MAXDEPTH];
	INTBIG   position[MAXDEPTH];
	INTBIG   depth;
	INTBIG   lx, hx, ly, hy;
} RSEARCH;

static RSEARCH *db_rsearchfree = NORSEARCH;
static void    *db_geomsearchmutex = 0;			/* mutex for geometric search modules */

/* prototypes for local routines */
static RSEARCH *db_allocrsearch(void);
static void     db_freersearch(RSEARCH*);
static void     db_removertnode(RTNODE*, INTBIG, NODEPROTO*);
static void     db_reinsert(RTNODE*, NODEPROTO*);
static BOOLEAN  db_findgeom(RTNODE*, GEOM*, RTNODE**, INTBIG*);
static BOOLEAN  db_findgeomanywhere(RTNODE*, GEOM*, RTNODE**, INTBIG*);
static void     db_figbounds(RTNODE*);

/*
 * Routine to free all memory associated with this module.
 */
void db_freegeomemory(void)
{
	REGISTER RSEARCH *rs;

	/* free all window partitions */
	while (db_rsearchfree != NORSEARCH)
	{
		rs = db_rsearchfree;
		db_rsearchfree = (RSEARCH *)db_rsearchfree->lx;
		efree((CHAR *)rs);
	}
}

/*
 * Routine to initialize the geometry module.
 */
void db_initgeometry(void)
{
	if (ensurevalidmutex(&db_geomsearchmutex, TRUE)) return;
}

/*********************** MODULE ALLOCATION ***********************/

/*
 * routine to allocate a Rtree node from cluster "cluster".
 */
RTNODE *allocrtnode(CLUSTER *cluster)
{
	REGISTER RTNODE *rtn;
	REGISTER INTBIG i;

	rtn = (RTNODE *)emalloc((sizeof (RTNODE)), cluster);
	if (rtn == 0) return(NORTNODE);
	rtn->total = 0;
	rtn->parent = NORTNODE;
	rtn->numvar = 0;
	rtn->firstvar = NOVARIABLE;
	rtn->lowx = rtn->highx = rtn->lowy = rtn->highy = 0;
	rtn->flag = 0;
	for (i=0; i<MAXRTNODESIZE; i++) rtn->pointers[i] = 0;
	return(rtn);
}

/*
 * routine to free rtnode "rtn".
 */
void freertnode(RTNODE *rtn)
{
	if (rtn == NORTNODE) return;
	if (rtn->numvar != 0) db_freevars(&rtn->firstvar, &rtn->numvar);
	efree((CHAR *)rtn);
}

/*
 * Routine to allocate a geometry modules from memory cluster "cluster".
 * The routine returns NOGEOM if allocation fails.
 */
GEOM *allocgeom(CLUSTER *cluster)
{
	REGISTER GEOM *geom;

	geom = (GEOM *)emalloc((sizeof (GEOM)), cluster);
	if (geom == 0) return(NOGEOM);
	geom->numvar = 0;
	geom->firstvar = NOVARIABLE;
	geom->entryisnode = FALSE;
	geom->entryaddr.ni = NONODEINST;
	geom->entryaddr.ai = NOARCINST;
	geom->entryaddr.blind = 0;
	geom->lowx = geom->highx = geom->lowy = geom->highy = 0;
	return(geom);
}

/*
 * routine to free a geometry module
 */
void freegeom(GEOM *geom)
{
	if (geom == NOGEOM) return;
	if (geom->numvar != 0) db_freevars(&geom->firstvar, &geom->numvar);
	efree((CHAR *)geom);
}

/*
 * routine to allocate a search module from the pool (if any) or memory
 * the routine returns NOSEARCH if allocation fails.
 */
RSEARCH *db_allocrsearch(void)
{
	REGISTER RSEARCH *rs;

	/* grab a free module from the global list */
	if (db_multiprocessing) emutexlock(db_geomsearchmutex);
	rs = NORSEARCH;
	if (db_rsearchfree != NORSEARCH)
	{
		/* take search module from free list */
		rs = db_rsearchfree;
		db_rsearchfree = (RSEARCH *)db_rsearchfree->lx;
	}
	if (db_multiprocessing) emutexunlock(db_geomsearchmutex);

	/* if none on the list, allocate one */
	if (rs == NORSEARCH)
	{
		/* no free search modules...allocate one */
		rs = (RSEARCH *)emalloc((sizeof (RSEARCH)), db_cluster);
		if (rs == 0) return(NORSEARCH);
	}
	return(rs);
}

/*
 * routine to free a search module
 */
void db_freersearch(RSEARCH *rs)
{
	if (rs == NORSEARCH) return;
	if (db_multiprocessing) emutexlock(db_geomsearchmutex);
	rs->lx = (INTBIG)db_rsearchfree;
	db_rsearchfree = rs;
	if (db_multiprocessing) emutexunlock(db_geomsearchmutex);
}

/*********************** CREATING A NEW R-TREE ***********************/

/*
 * routine to build geometry module structure in cell "np".
 * returns true upon error.
 */
BOOLEAN geomstructure(NODEPROTO *np)
{
	REGISTER RTNODE *top;

	top = allocrtnode(np->lib->cluster);
	if (top == NORTNODE) return(TRUE);
	top->lowx = top->highx = top->lowy = top->highy = 0;
	top->total = 0;
	top->flag = 1;
	top->parent = NORTNODE;
	np->rtree = top;
	return(FALSE);
}

/*
 * Routine to free the r-tree structure headed by "rtn".
 */
void db_freertree(RTNODE *rtn)
{
	REGISTER INTBIG i;

	if (rtn->flag == 0)
	{
		for(i=0; i<rtn->total; i++)
			db_freertree((RTNODE *)rtn->pointers[i]);
	}
	freertnode(rtn);
}

/*********************** ADDING TO AN R-TREE ***********************/

/*
 * routine to link geometry module "geom" into the R-tree.  The parent
 * nodeproto is in "parnt".
 */
void linkgeom(GEOM *geom, NODEPROTO *parnt)
{
	REGISTER RTNODE *rtn, *subrtn;
	REGISTER INTBIG i, bestsubnode;
#ifdef	USEFLOATS
	REGISTER float area, newarea, bestexpand, expand;
#else
	REGISTER INTBIG bestexpand, area, newarea, expand, scaledown;
#endif
	REGISTER INTBIG lxv, hxv, lyv, hyv, xd, yd;

	/* setup the bounding box of "geom" */
	boundobj(geom, &geom->lowx, &geom->highx, &geom->lowy, &geom->highy);

	/* find the leaf that would expand least by adding this node */
	rtn = parnt->rtree;
	for(;;)
	{
		/* if R-tree node contains primitives, exit loop */
		if (rtn->flag != 0) break;

#ifndef	USEFLOATS
		/* compute scaling factor */
		scaledown = MAXINTBIG;
		for(i=0; i<rtn->total; i++)
		{
			subrtn = (RTNODE *)rtn->pointers[i];
			xd = subrtn->highx - subrtn->lowx;
			yd = subrtn->highy - subrtn->lowy;
			if (xd < scaledown) scaledown = xd;
			if (yd < scaledown) scaledown = yd;
		}
		if (scaledown <= 0) scaledown = 1;
#endif

		/* find sub-node that would expand the least */
		for(i=0; i<rtn->total; i++)
		{
			/* get bounds and area of sub-node */
			subrtn = (RTNODE *)rtn->pointers[i];
			lxv = subrtn->lowx;
			hxv = subrtn->highx;
			lyv = subrtn->lowy;
			hyv = subrtn->highy;
#ifdef	USEFLOATS
			area = hxv - lxv;   area *= hyv - lyv;
#else
			area = ((hxv - lxv) / scaledown) * ((hyv - lyv) / scaledown);
#endif

			/* get area of sub-node with new element */
			if (geom->highx > hxv) hxv = geom->highx;
			if (geom->lowx < lxv) lxv = geom->lowx;
			if (geom->highy > hyv) hyv = geom->highy;
			if (geom->lowy < lyv) lyv = geom->lowy;
#ifdef	USEFLOATS
			newarea = hxv - lxv;   newarea *= hyv - lyv;
#else
			newarea = ((hxv - lxv) / scaledown) * ((hyv - lyv) / scaledown);
#endif

			/* accumulate the least expansion */
			expand = newarea - area;

			/* LINTED "bestexpand" used in proper order */
			if (i != 0 && expand > bestexpand) continue;
			bestexpand = expand;
			bestsubnode = i;
		}

		/* recurse down to sub-node that expanded least */
		rtn = (RTNODE *)rtn->pointers[bestsubnode];
	}

	/* add this geometry element to the correct leaf R-tree node */
	(void)db_addtortnode((UINTBIG)geom, rtn, parnt);
}

/*
 * routine to add "object" to R-tree node "rtn".  Routine may have to
 * split the node and recurse up the tree
 */
BOOLEAN db_addtortnode(UINTBIG object, RTNODE *rtn, NODEPROTO *cell)
{
	RTNODE temp;
	REGISTER INTBIG i, oldcount, oldn, newn, bestoldnode, bestnewnode;
#ifdef	USEFLOATS
	REGISTER float bestoldexpand, bestnewexpand, newareaplus, oldareaplus, newarea, oldarea;
#else
	REGISTER INTBIG bestoldexpand, bestnewexpand, newareaplus, oldareaplus, newarea, oldarea, scaledown;
#endif
	REGISTER INTBIG olddist, newdist, dist;
	INTBIG lowestxv, highestxv, lowestyv, highestyv, lxv, hxv, lyv, hyv;
	REGISTER RTNODE *newroot, *newrtn, *r;

	/* see if there is room in the R-tree node */
	if (rtn->total >= MAXRTNODESIZE)
	{
		/*
		 * no room: find the element farthest from new object
		 * also, compute scale and copy node to temporary
		 */
#ifndef	USEFLOATS
		scaledown = MAXINTBIG;
#endif
		newdist = 0;
		temp.flag = rtn->flag;
		temp.pointers[0] = object;
		db_rtnbbox(&temp, 0, &lowestxv, &highestxv, &lowestyv, &highestyv);
		for(i=0; i<rtn->total; i++)
		{
			temp.pointers[i] = rtn->pointers[i];
			db_rtnbbox(rtn, i, &lxv, &hxv, &lyv, &hyv);
			dist = computedistance((lxv+hxv)/2, (lyv+hyv)/2,
				(lowestxv+highestxv)/2, (lowestyv+highestyv)/2);
			if (dist >= newdist)
			{
				newdist = dist;
				newn = i;
			}
#ifndef	USEFLOATS
			if (hxv-lxv < scaledown) scaledown = hxv - lxv;
			if (hyv-lyv < scaledown) scaledown = hyv - lyv;
#endif
		}
#ifndef	USEFLOATS
		if (scaledown <= 0) scaledown = 1;
#endif

		/* now find element farthest from "newn" */
		olddist = 0;
		db_rtnbbox(rtn, newn, &lowestxv, &highestxv, &lowestyv, &highestyv);
		for(i=0; i<rtn->total; i++)
		{
			if (i == newn) continue;
			db_rtnbbox(rtn, i, &lxv, &hxv, &lyv, &hyv);
			dist = computedistance((lxv+hxv)/2, (lyv+hyv)/2,
				(lowestxv+highestxv)/2, (lowestyv+highestyv)/2);
			if (dist >= olddist)
			{
				olddist = dist;
				oldn = i;
			}
		}

		/* allocate a new R-tree node and put in first seed element */
		newrtn = allocrtnode(cell->lib->cluster);
		if (newrtn == NORTNODE) return(TRUE);
		newrtn->flag = rtn->flag;
		newrtn->parent = rtn->parent;
		newrtn->pointers[0] = temp.pointers[newn];
		temp.pointers[newn] = (UINTBIG)NORTNODE;
		if (newrtn->flag == 0) ((RTNODE *)newrtn->pointers[0])->parent = newrtn;
		newrtn->total = 1;
		db_rtnbbox(newrtn, 0, &newrtn->lowx, &newrtn->highx, &newrtn->lowy, &newrtn->highy);
#ifdef	USEFLOATS
		newarea = newrtn->highx - newrtn->lowx;   newarea *= newrtn->highy - newrtn->lowy;
#else
		newarea = ((newrtn->highx-newrtn->lowx) / scaledown) *
			((newrtn->highy-newrtn->lowy) / scaledown);
#endif

		/* initialize old R-tree node and put in other seed element */
		oldcount = rtn->total;
		rtn->pointers[0] = temp.pointers[oldn];
		temp.pointers[oldn] = (UINTBIG)NORTNODE;
		if (rtn->flag == 0) ((RTNODE *)rtn->pointers[0])->parent = rtn;
		rtn->total = 1;
		db_rtnbbox(rtn, 0, &rtn->lowx, &rtn->highx, &rtn->lowy, &rtn->highy);
#ifdef	USEFLOATS
		oldarea = rtn->highx - rtn->lowx;   oldarea *= rtn->highy - rtn->lowy;
#else
		oldarea = ((rtn->highx - rtn->lowx) / scaledown) * ((rtn->highy - rtn->lowy) / scaledown);
#endif

		/* cluster the rest of the nodes */
		for(;;)
		{
			/* search for a cluster about each new node */
			bestnewnode = bestoldnode = -1;
			for(i=0; i<oldcount; i++)
			{
				if (temp.pointers[i] == (UINTBIG)NORTNODE) continue;
				db_rtnbbox(&temp, i, &lxv, &hxv, &lyv, &hyv);

#ifdef	USEFLOATS
				newareaplus = mmaxi(hxv, newrtn->highx) - mmini(lxv, newrtn->lowx);
				newareaplus *= mmaxi(hyv, newrtn->highy) - mmini(lyv, newrtn->lowy);
				oldareaplus = mmaxi(hxv, rtn->highx) - mmini(lxv, rtn->lowx);
				oldareaplus *= mmaxi(hyv, rtn->highy) - mmini(lyv, rtn->lowy);
#else
				newareaplus = ((mmaxi(hxv, newrtn->highx) - mmini(lxv, newrtn->lowx)) / scaledown) *
					((mmaxi(hyv, newrtn->highy) - mmini(lyv, newrtn->lowy)) / scaledown);
				oldareaplus = ((mmaxi(hxv, rtn->highx) - mmini(lxv, rtn->lowx)) / scaledown) *
					((mmaxi(hyv, rtn->highy) - mmini(lyv, rtn->lowy)) / scaledown);
#endif

				/* LINTED "bestnewexpand" used in proper order */
				if (bestnewnode < 0 || newareaplus-newarea < bestnewexpand)
				{
					bestnewexpand = newareaplus-newarea;
					bestnewnode = i;
				}

				/* LINTED "bestoldexpand" used in proper order */
				if (bestoldnode < 0 || oldareaplus-oldarea < bestoldexpand)
				{
					bestoldexpand = oldareaplus-oldarea;
					bestoldnode = i;
				}
			}

			/* if there were no nodes added, all have been clustered */
			if (bestnewnode == -1 && bestoldnode == -1) break;

			/* if both selected the same object, select another "oldn" */
			if (bestnewnode == bestoldnode)
			{
				bestoldnode = -1;
				for(i=0; i<oldcount; i++)
				{
					if (temp.pointers[i] == (UINTBIG)NORTNODE) continue;
					if (i == bestnewnode) continue;
					db_rtnbbox(&temp, i, &lxv, &hxv, &lyv, &hyv);

#ifdef	USEFLOATS
					oldareaplus = mmaxi(hxv, rtn->highx) - mmini(lxv, rtn->lowx);
					oldareaplus *= mmaxi(hyv, rtn->highy) - mmini(lyv, rtn->lowy);
#else
					oldareaplus = ((mmaxi(hxv, rtn->highx) - mmini(lxv, rtn->lowx)) / scaledown) *
						((mmaxi(hyv, rtn->highy) - mmini(lyv, rtn->lowy)) / scaledown);
#endif
					if (bestoldnode < 0 || oldareaplus-oldarea < bestoldexpand)
					{
						bestoldexpand = oldareaplus-oldarea;
						bestoldnode = i;
					}
				}
			}

			/* add to "oldn" cluster */
			if (bestoldnode != -1)
			{
				/* add this node to "rtn" */
				rtn->pointers[rtn->total] = temp.pointers[bestoldnode];
				temp.pointers[bestoldnode] = (UINTBIG)NORTNODE;
				if (rtn->flag == 0) ((RTNODE *)rtn->pointers[rtn->total])->parent = rtn;
				db_rtnbbox(rtn, rtn->total, &lxv, &hxv, &lyv, &hyv);
				rtn->total++;
				if (lxv < rtn->lowx) rtn->lowx = lxv;
				if (hxv > rtn->highx) rtn->highx = hxv;
				if (lyv < rtn->lowy) rtn->lowy = lyv;
				if (hyv > rtn->highy) rtn->highy = hyv;
#ifdef	USEFLOATS
				oldarea = rtn->highx - rtn->lowx;
				oldarea *= rtn->highy - rtn->lowy;
#else
				oldarea = ((rtn->highx - rtn->lowx) / scaledown) *
					((rtn->highy - rtn->lowy) / scaledown);
#endif
			}

			/* add to "newn" cluster */
			if (bestnewnode != -1)
			{
				/* add this node to "newrtn" */
				newrtn->pointers[newrtn->total] = temp.pointers[bestnewnode];
				temp.pointers[bestnewnode] = (UINTBIG)NORTNODE;
				if (newrtn->flag == 0) ((RTNODE *)newrtn->pointers[newrtn->total])->parent = newrtn;
				db_rtnbbox(newrtn, newrtn->total, &lxv, &hxv, &lyv, &hyv);
				newrtn->total++;
				if (lxv < newrtn->lowx) newrtn->lowx = lxv;
				if (hxv > newrtn->highx) newrtn->highx = hxv;
				if (lyv < newrtn->lowy) newrtn->lowy = lyv;
				if (hyv > newrtn->highy) newrtn->highy = hyv;
#ifdef	USEFLOATS
				newarea = newrtn->highx - newrtn->lowx;
				newarea *= newrtn->highy - newrtn->lowy;
#else
				newarea = ((newrtn->highx-newrtn->lowx) / scaledown) *
					((newrtn->highy-newrtn->lowy) / scaledown);
#endif
			}
		}

		/* sensibility check */
		if (oldcount != rtn->total + newrtn->total)
			ttyputerr(_("R-trees: %ld nodes split to %d and %d!"),
				oldcount, rtn->total, newrtn->total);

		/* now recursively insert this new element up the tree */
		if (rtn->parent == NORTNODE)
		{
			/* at top of tree: create a new level */
			newroot = allocrtnode(cell->lib->cluster);
			if (newroot == 0) return(TRUE);
			newroot->total = 2;
			newroot->pointers[0] = (UINTBIG)rtn;
			newroot->pointers[1] = (UINTBIG)newrtn;
			newroot->flag = 0;
			newroot->parent = NORTNODE;
			rtn->parent = newrtn->parent = newroot;
			newroot->lowx = mmini(rtn->lowx, newrtn->lowx);
			newroot->highx = mmaxi(rtn->highx, newrtn->highx);
			newroot->lowy = mmini(rtn->lowy, newrtn->lowy);
			newroot->highy = mmaxi(rtn->highy, newrtn->highy);
			cell->rtree = newroot;
		} else
		{
			/* first recompute bounding box of R-tree nodes up the tree */
			for(r = rtn->parent; r != NORTNODE; r = r->parent) db_figbounds(r);

			/* now add the new node up the tree */
			if (db_addtortnode((UINTBIG)newrtn, rtn->parent, cell)) return(TRUE);
		}
	}

	/* now add this element to the R-tree node */
	rtn->pointers[rtn->total] = object;
	db_rtnbbox(rtn, rtn->total, &lxv, &hxv, &lyv, &hyv);
	rtn->total++;

	/* special case when adding the first node in a cell */
	if (rtn->total == 1 && rtn->parent == NORTNODE)
	{
		rtn->lowx = lxv;
		rtn->highx = hxv;
		rtn->lowy = lyv;
		rtn->highy = hyv;
		return(FALSE);
	}

	/* recursively update node sizes */
	for(;;)
	{
		rtn->lowx = mmini(rtn->lowx, lxv);
		rtn->highx = mmaxi(rtn->highx, hxv);
		rtn->lowy = mmini(rtn->lowy, lyv);
		rtn->highy = mmaxi(rtn->highy, hyv);
		if (rtn->parent == NORTNODE) break;
		rtn = rtn->parent;
	}
	return(FALSE);
}

/*********************** DELETING FROM AN R-TREE ***********************/

/*
 * routine to remove geometry module "geom" from the R-tree in cell "parnt"
 */
void undogeom(GEOM *geom, NODEPROTO *parnt)
{
	RTNODE *whichrtn;
	INTBIG whichind;

	/* find this node in the tree */
	if (!db_findgeom(parnt->rtree, geom, &whichrtn, &whichind))
	{
		if (!db_findgeomanywhere(parnt->rtree, geom, &whichrtn, &whichind))
		{
			ttyputerr(_("Internal warning: cannot find %s in R-Tree of %s"),
				geomname(geom), describenodeproto(parnt));
			return;
		}
		ttyputmsg(_("Internal warning: %s not in proper R-Tree location"),
			geomname(geom));
	}

	/* delete geom from this R-tree node */
	db_removertnode(whichrtn, whichind, parnt);
}

/*
 * routine to remove entry "ind" from R-tree node "rtn" in cell "cell"
 */
void db_removertnode(RTNODE *rtn, INTBIG ind, NODEPROTO *cell)
{
	REGISTER RTNODE *prtn;
	RTNODE temp;
	REGISTER INTBIG i, j, t;

	/* delete entry from this R-tree node */
	j = 0;
	for(i=0; i<rtn->total; i++)
		if (i != ind) rtn->pointers[j++] = rtn->pointers[i];
	rtn->total = (INTSML)j;

	/* see if node is now too small */
	if (rtn->total < MINRTNODESIZE)
	{
		/* if recursed to top, shorten R-tree */
		prtn = rtn->parent;
		if (prtn == NORTNODE)
		{
			/* if tree has no hierarchy, allow short node */
			if (rtn->flag != 0)
			{
				/* compute correct bounds of the top node */
				db_figbounds(rtn);
				return;
			}

			/* save all top-level entries */
			for(i=0; i<rtn->total; i++) temp.pointers[i] = rtn->pointers[i];
			t = rtn->total;

			/* erase top level */
			rtn->total = 0;
			rtn->flag = 1;

			/* reinsert all data */
			for(i=0; i<t; i++) db_reinsert((RTNODE *)temp.pointers[i], cell);
			return;
		}

		/* node has too few entries, must delete it and reinsert members */
		for(i=0; i<prtn->total; i++)
			if (prtn->pointers[i] == (UINTBIG)rtn) break;
		if (i >= prtn->total) ttyputerr(_("R-trees: cannot find entry in parent"));

		/* remove this entry from its parent */
		db_removertnode(prtn, i, cell);

		/* reinsert the entries */
		db_reinsert(rtn, cell);
		return;
	}

	/* recompute bounding box of this R-tree node and all up the tree */
	for(;;)
	{
		db_figbounds(rtn);
		if (rtn->parent == NORTNODE) break;
		rtn = rtn->parent;
	}
}

/*
 * routine to reinsert the tree of nodes below "rtn" into cell "cell".
 */
void db_reinsert(RTNODE *rtn, NODEPROTO *cell)
{
	REGISTER INTBIG i;

	if (rtn->flag != 0)
	{
		for(i=0; i<rtn->total; i++) linkgeom((GEOM *)rtn->pointers[i], cell);
	} else
	{
		for(i=0; i<rtn->total; i++)
			db_reinsert((RTNODE *)rtn->pointers[i], cell);
	}
	freertnode(rtn);
}

/*
 * routine to find the location of geometry module "geom" in the R-tree
 * at "rtn".  The subnode that contains this module is placed in "subrtn"
 * and the index in that subnode is placed in "subind".  The routine returns
 * false if it is unable to find the geometry module.
 */
BOOLEAN db_findgeom(RTNODE *rtn, GEOM *geom, RTNODE **subrtn, INTBIG *subind)
{
	REGISTER INTBIG i;
	INTBIG lxv, hxv, lyv, hyv;

	/* if R-tree node contains primitives, search for direct hit */
	if (rtn->flag != 0)
	{
		for(i=0; i<rtn->total; i++)
		{
			if (rtn->pointers[i] == (UINTBIG)geom)
			{
				*subrtn = rtn;
				*subind = i;
				return(TRUE);
			}
		}
		return(FALSE);
	}

	/* recurse on all sub-nodes that would contain this geometry module */
	for(i=0; i<rtn->total; i++)
	{
		/* get bounds and area of sub-node */
		db_rtnbbox(rtn, i, &lxv, &hxv, &lyv, &hyv);
		if (geom->lowx < lxv || geom->highx > hxv || geom->lowy < lyv || geom->highy > hyv)
			continue;
		if (db_findgeom((RTNODE *)rtn->pointers[i], geom, subrtn, subind)) return(TRUE);
	}
	return(FALSE);
}

/*
 * routine to find the location of geometry module "geom" anywhere in the R-tree
 * at "rtn".  The subnode that contains this module is placed in "subrtn"
 * and the index in that subnode is placed in "subind".  The routine returns
 * false if it is unable to find the geometry module.
 */
BOOLEAN db_findgeomanywhere(RTNODE *rtn, GEOM *geom, RTNODE **subrtn, INTBIG *subind)
{
	REGISTER INTBIG i;

	/* if R-tree node contains primitives, search for direct hit */
	if (rtn->flag != 0)
	{
		for(i=0; i<rtn->total; i++)
		{
			if (rtn->pointers[i] == (UINTBIG)geom)
			{
				*subrtn = rtn;
				*subind = i;
				return(1);
			}
		}
		return(FALSE);
	}

	/* recurse on all sub-nodes */
	for(i=0; i<rtn->total; i++)
		if (db_findgeomanywhere((RTNODE *)rtn->pointers[i], geom, subrtn, subind))
			return(TRUE);
	return(FALSE);
}

/************************ CHANGING AN R-TREE ************************/

/*
 * routine to adjust the 8-way linked list when the size or position
 * of the object in "geom" has changed.
 */
void updategeom(GEOM *geom, NODEPROTO *parnt)
{
	/* first remove the module from the R-tree */
	undogeom(geom, parnt);

	/* now re-insert the module in the R-tree */
	linkgeom(geom, parnt);
}

/*********************** INFORMATION ***********************/

void db_printrtree(RTNODE *expectedparent, RTNODE *rtn, INTBIG indent)
{
	REGISTER INTBIG i, j;
	INTBIG lx, hx, ly, hy, lowestxv, highestxv, lowestyv, highestyv;
	CHAR line[100];
	REGISTER void *infstr;

	infstr = initinfstr();
	for(i=0; i<indent; i++) addtoinfstr(infstr, ' ');
	(void)esnprintf(line, 100, M_("Node X(%s-%s) Y(%s-%s) has %d children:"),
		latoa(rtn->lowx, 0), latoa(rtn->highx, 0), latoa(rtn->lowy, 0), latoa(rtn->highy, 0), rtn->total);
	addstringtoinfstr(infstr, line);

	/* sensibility check of this R-tree node */
	if (rtn->parent != expectedparent) addstringtoinfstr(infstr, M_(" WRONG-PARENT"));
	if (rtn->total > MAXRTNODESIZE) addstringtoinfstr(infstr, M_(" TOO-MANY"));
	if (rtn->total < MINRTNODESIZE && rtn->parent != NORTNODE)
		addstringtoinfstr(infstr, M_(" TOO-FEW"));
	if (rtn->total != 0)
	{
		db_rtnbbox(rtn, 0, &lowestxv, &highestxv, &lowestyv, &highestyv);
		for(j=1; j<rtn->total; j++)
		{
			db_rtnbbox(rtn, j, &lx, &hx, &ly, &hy);
			if (lx < lowestxv) lowestxv = lx;
			if (hx > highestxv) highestxv = hx;
			if (ly < lowestyv) lowestyv = ly;
			if (hy > highestyv) highestyv = hy;
		}
		if (rtn->lowx != lowestxv || rtn->highx != highestxv ||
			rtn->lowy != lowestyv || rtn->highy != highestyv)
				addstringtoinfstr(infstr, M_(" WRONG-BOUNDS"));
		rtn->lowx = lowestxv;
		rtn->highx = highestxv;
		rtn->lowy = lowestyv;
		rtn->highy = highestyv;
	}
	ttyputmsg(x_("%s"), returninfstr(infstr));

	for(j=0; j<rtn->total; j++)
	{
		if (rtn->flag != 0)
		{
			infstr = initinfstr();
			for(i=0; i<indent+3; i++) addtoinfstr(infstr, ' ');
			addstringtoinfstr(infstr, geomname((GEOM *)rtn->pointers[j]));
			db_rtnbbox(rtn, j, &lx, &hx, &ly, &hy);
			(void)esnprintf(line, 100, x_(" X(%s-%s) Y(%s-%s)"),
				latoa(lx, 0), latoa(hx, 0), latoa(ly, 0), latoa(hy, 0));
			addstringtoinfstr(infstr, line);
			ttyputmsg(x_("%s"), returninfstr(infstr));
		} else db_printrtree(rtn, (RTNODE *)rtn->pointers[j], indent+3);
	}
}

/*
 * routine to recompute the bounds of R-tree node "rtn"
 */
void db_figbounds(RTNODE *rtn)
{
	REGISTER INTBIG i;
	INTBIG lx, hx, ly, hy;

	if (rtn->total == 0)
	{
		rtn->lowx = rtn->highx = rtn->lowy = rtn->highy = 0;
		return;
	}
	db_rtnbbox(rtn, 0, &rtn->lowx, &rtn->highx, &rtn->lowy, &rtn->highy);
	for(i=1; i<rtn->total; i++)
	{
		db_rtnbbox(rtn, i, &lx, &hx, &ly, &hy);
		if (lx < rtn->lowx) rtn->lowx = lx;
		if (hx > rtn->highx) rtn->highx = hx;
		if (ly < rtn->lowy) rtn->lowy = ly;
		if (hy > rtn->highy) rtn->highy = hy;
	}
}

/*
 * routine to get the bounding box of child "child" of R-tree node "rtn" and
 * place it in the reference parameters "lx", "hx", "ly", and "hy".
 */
void db_rtnbbox(RTNODE *rtn, INTBIG child, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy)
{
	REGISTER RTNODE *subrtn;
	REGISTER GEOM *geom;

	if (rtn->flag == 0)
	{
		subrtn = (RTNODE *)rtn->pointers[child];
		*lx = subrtn->lowx;
		*hx = subrtn->highx;
		*ly = subrtn->lowy;
		*hy = subrtn->highy;
	} else
	{
		geom = (GEOM *)rtn->pointers[child];
		*lx = geom->lowx;
		*hx = geom->highx;
		*ly = geom->lowy;
		*hy = geom->highy;
	}
}

/*
 * routine to establish the bounding box of geometry module "geom" by filling
 * the reference parameters "lx", "hx", "ly", and "hy" with the minimum
 * bounding rectangle.
 */
void boundobj(GEOM *geom, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy)
{
	REGISTER INTBIG end0extend, end1extend, radius, angle, j, pts,
		xs, ys, xe, ye, xc, yc, xv, yv;
	INTBIG x1, y1, x2, y2, num, xp, yp, plx, phx, ply, phy;
	double startoffset, endangle, ctx, cty, a, b, p;
	XARRAY trans;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var;
	REGISTER POLYGON *poly;

	if (geom->entryisnode)
	{
		ni = geom->entryaddr.ni;

		/* handle special cases primitives */
		if (ni->proto->primindex != 0)
		{
			/* special case for arcs of circles: compute precise bounding box */
			if (ni->proto == art_circleprim || ni->proto == art_thickcircleprim)
			{
				/* see if there this circle is only a partial one */
				getarcdegrees(ni, &startoffset, &endangle);
				if (startoffset != 0.0 || endangle != 0.0)
				{
					/* get center of ellipse */
					ctx = (ni->lowx + ni->highx) / 2;
					cty = (ni->lowy + ni->highy) / 2;

					/* compute the length of the semi-major and semi-minor axes */
					a = (ni->highx - ni->lowx) / 2;
					b = (ni->highy - ni->lowy) / 2;

					pts = (INTBIG)(endangle * ELLIPSEPOINTS / (EPI * 2.0));
					makerot(ni, trans);
					for(j=0; j<pts; j++)
					{
						p = startoffset + j * endangle / (pts-1);
						xv = rounddouble(ctx + a * cos(p));
						yv = rounddouble(cty + b * sin(p));
						xform(xv, yv, &xp, &yp, trans);
						if (j == 0)
						{
							*lx = *hx = xp;
							*ly = *hy = yp;
						} else
						{
							if (xp < *lx) *lx = xp;
							if (xp > *hx) *hx = xp;
							if (yp < *ly) *ly = yp;
							if (yp > *hy) *hy = yp;
						}
					}
					return;
				}
			}

			/* special case for pins that become steiner points */
			if ((ni->proto->userbits&WIPEON1OR2) != 0 &&
				ni->firstportexpinst == NOPORTEXPINST)
			{
				if (tech_pinusecount(ni, NOWINDOWPART))
				{
					*lx = *hx = (ni->lowx + ni->highx) / 2;
					*ly = *hy = (ni->lowy + ni->highy) / 2;
					return;
				}
			}

			/* special case for polygonally-defined nodes: compute precise geometry */
			if ((ni->proto->userbits&HOLDSTRACE) != 0)
			{
				var = gettrace(ni);
				if (var != NOVARIABLE)
				{
					poly = allocpolygon(4, db_cluster);
					makerot(ni, trans);
					num = nodepolys(ni, 0, NOWINDOWPART);
					for(j=0; j<num; j++)
					{
						shapenodepoly(ni, j, poly);
						xformpoly(poly, trans);
						getbbox(poly, &plx, &phx, &ply, &phy);
						if (j == 0)
						{
							*lx = plx;   *hx = phx;
							*ly = ply;   *hy = phy;
						} else
						{
							if (plx < *lx) *lx = plx;
							if (phx > *hx) *hx = phx;
							if (ply < *ly) *ly = ply;
							if (phy > *hy) *hy = phy;
						}
					}
					freepolygon(poly);
					return;
				}
			}
		}

		/* standard bounds computation */
		if (ni->rotation == 0 && ni->transpose == 0)
		{
			*lx = ni->lowx;   *hx = ni->highx;
			*ly = ni->lowy;   *hy = ni->highy;
		} else
		{
			makerot(ni, trans);
			*lx = ni->lowx;   *hx = ni->highx;
			*ly = ni->lowy;   *hy = ni->highy;
			xformbox(lx, hx, ly, hy, trans);
		}
	} else
	{
		ai = geom->entryaddr.ai;

		/* special case if the arc is curved */
		if ((ai->proto->userbits&CANCURVE) != 0)
		{
			/* prototype can curve...does this one have curvature? */
			var = getvalkey((INTBIG)ai, VARCINST, VINTEGER, el_arc_radius_key);
			if (var != NOVARIABLE)
			{
				/* this one has curvature...verify its sensibility */
				radius = var->addr;
				if (abs(radius)*2 >= ai->length)
				{
					/* arc is sensible...determine possible circle centers */
					if (!findcenters(abs(radius), ai->end[0].xpos,
						ai->end[0].ypos, ai->end[1].xpos, ai->end[1].ypos,
							ai->length, &x1, &y1, &x2, &y2))
					{
						/* determine center */
						if (radius < 0)
						{
							xc = x1;   yc = y1;
						} else
						{
							xc = x2;   yc = y2;
						}

						/* determine start and endpoint */
						if ((ai->userbits&REVERSEEND) != 0)
						{
							xs = ai->end[1].xpos;
							ys = ai->end[1].ypos;
							xe = ai->end[0].xpos;
							ye = ai->end[0].ypos;
						} else
						{
							xs = ai->end[0].xpos;
							ys = ai->end[0].ypos;
							xe = ai->end[1].xpos;
							ye = ai->end[1].ypos;
						}

						/*
						 * compute bounding box for this arc.
						 * Note that the start and end are reversed because
						 * "arcbbox" takes clockwise arcs, but this one is
						 * computed with counterclockwise points.
						 */
						arcbbox(xe, ye, xs, ys, xc, yc, lx, hx, ly, hy);
						return;
					}
				}
			}
		}

		/* straight arc...get endpoint extension factor */
		end0extend = end1extend = ai->width / 2;
		if ((ai->userbits&NOEXTEND) != 0)
		{
			if ((ai->userbits&NOTEND0) == 0) end0extend = 0;
			if ((ai->userbits&NOTEND1) == 0) end1extend = 0;
		} else if ((ai->userbits&ASHORT) != 0)
		{
			end0extend = tech_getextendfactor(ai->width, ai->endshrink & 0xFFFF);
			end1extend = tech_getextendfactor(ai->width, (ai->endshrink >> 16) & 0xFFFF);
		}

		/* construct a polygon describing the arc */
		angle = (ai->userbits&AANGLE) >> AANGLESH;
		poly = allocpolygon(4, db_cluster);
		tech_makeendpointpoly(ai->length, ai->width, angle*10, ai->end[0].xpos,
			ai->end[0].ypos, end0extend, ai->end[1].xpos,
				ai->end[1].ypos, end1extend, poly);
		poly->style = FILLED;

		/* get the bounding box of the polygon as the arc extent */
		getbbox(poly, lx, hx, ly, hy);
		freepolygon(poly);
	}
}

/*
 * routine to compute the boundary of nodeproto "cell" and fill the
 * reference parameters "lx", "hx", "ly", and "hy" with the minimum
 * bounding rectangle
 */
void db_boundcell(NODEPROTO *cell, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER VARIABLE *var;
	REGISTER BOOLEAN first;
	REGISTER INTBIG i;

	/* cannot compute bounds if cell has nothing in it */
	*lx = *hx = *ly = *hy = 0;
	if (cell->firstnodeinst == NONODEINST) return;

	/* include all nodes in the cell */
	first = TRUE;
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* special case: do not include "cell center" primitives from Generic */
		if (ni->proto == gen_cellcenterprim /* || ni->proto == gen_essentialprim */ ) continue;

		/* special case for invisible pins: do not include if inheritable or interior-only */
		if (ni->proto == gen_invispinprim)
		{
			for(i=0; i<ni->numvar; i++)
			{
				var = &ni->firstvar[i];
				if ((var->type&VDISPLAY) != 0 &&
					(TDGETINTERIOR(var->textdescript) != 0 ||
						TDGETINHERIT(var->textdescript) != 0)) break;
			}
			if (i < ni->numvar) continue;
		}

		if (first)
		{
			*lx = ni->geom->lowx;
			*hx = ni->geom->highx;
			*ly = ni->geom->lowy;
			*hy = ni->geom->highy;
			first = FALSE;
			continue;
		}
		*lx = mmini(*lx, ni->geom->lowx);
		*hx = mmaxi(*hx, ni->geom->highx);
		*ly = mmini(*ly, ni->geom->lowy);
		*hy = mmaxi(*hy, ni->geom->highy);
	}

	/* include all arcs in the cell */
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (first)
		{
			*lx = ai->geom->lowx;
			*hx = ai->geom->highx;
			*ly = ai->geom->lowy;
			*hy = ai->geom->highy;
			first = FALSE;
			continue;
		}
		*lx = mmini(*lx, ai->geom->lowx);
		*hx = mmaxi(*hx, ai->geom->highx);
		*ly = mmini(*ly, ai->geom->lowy);
		*hy = mmaxi(*hy, ai->geom->highy);
	}
}

/************************ SEARCHING ************************/

/*
 * search routine initialization.  Begin search for objects that are in
 * cell "cell" and fall in the bounding box X from "lx" to "hx" and
 * Y from "ly" to "hy".  Routine returns value that is passed to
 * "nextobject" calls to run the search.
 */
INTBIG initsearch(INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy, NODEPROTO *cell)
{
	REGISTER RSEARCH *rs;

	rs = db_allocrsearch();
	if (rs == NORSEARCH) return(-1);
	rs->depth = 0;
	rs->rtn[0] = cell->rtree;
	rs->position[0] = 0;
	rs->lx = lx;
	rs->hx = hx;
	rs->ly = ly;
	rs->hy = hy;
	return((INTBIG)rs);
}

/*
 * second routine for searches: takes the search module returned by
 * "initsearch" and returns the next geometry module in the
 * search area.  If there are no more, this returns NOGEOM.
 */
GEOM *nextobject(INTBIG rsin)
{
	RSEARCH *rs;
	REGISTER RTNODE *rtn;
	REGISTER INTBIG i, depth;
	INTBIG lxv, hxv, lyv, hyv;

	rs = (RSEARCH *)rsin;
	if (rs == NORSEARCH) return(NOGEOM);
	for(;;)
	{
		depth = rs->depth;
		rtn = rs->rtn[depth];
		i = rs->position[depth]++;
		if (i < rtn->total)
		{
			db_rtnbbox(rtn, i, &lxv, &hxv, &lyv, &hyv);
			if (rs->lx > hxv || rs->hx < lxv || rs->ly > hyv || rs->hy < lyv) continue;
			if (rtn->flag != 0) return((GEOM *)rtn->pointers[i]);

			/* look down the hierarchy */
			if (rs->depth >= MAXDEPTH-1)
			{
				ttyputerr(_("R-trees: search too deep"));
				continue;
			}
			rs->depth++;
			rs->rtn[rs->depth] = (RTNODE *)rtn->pointers[i];
			rs->position[rs->depth] = 0;
		} else
		{
			/* pop up the hierarchy */
			if (depth == 0) break;
			rs->depth--;
		}
	}
	db_freersearch(rs);
	return(NOGEOM);
}

/*
 * routine to clean up after a search.  Takes the search module returned by
 * "initsearch" and frees it.  This routine only needs to be called if the loop
 * through all neighbors is aborted BEFORE "nextobject" returns NOGEOM.
 */
void termsearch(INTBIG rsin)
{
	RSEARCH *rs;

	rs = (RSEARCH *)rsin;
	if (rs != NORSEARCH) db_freersearch(rs);
}
