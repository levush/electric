/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dbnoproto.c
 * Database cell manipulation module
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
#include "network.h"
#include "usr.h"

#define PRINTFRAMEQLAMBDA 60		/* size of text in frames (in quarter lambda units) */

/* hierarchical traversal data structures */
typedef struct
{
	NODEINST **path;
	INTBIG    *index;
	INTBIG     depth;
	INTBIG     limit;
} HTRAVERSAL;

static HTRAVERSAL db_hiertraverse = {0, 0, 0, 0};

static BOOLEAN    db_hiertraversing = FALSE;
static INTBIG     db_hiertempclimb = 0;
       UINTBIG    db_traversaltimestamp = 0;
static INTBIG     db_descent_path_key = 0;	/* variable key for "USER_descent_path" */

/* prototypes for local routines */
static CHAR   *db_getframedesc(NODEPROTO*);
static INTBIG  db_copyxlibvars(INTBIG, INTBIG, INTBIG, INTBIG, NODEPROTO*, LIBRARY*);
static BOOLEAN db_portassociate(NODEINST*, NODEINST*, BOOLEAN);
static BOOLEAN db_doesntconnect(ARCPROTO*[], PORTPROTO*);
static void    db_changeallports(PORTPROTO*);
static BOOLEAN db_addtohierpath(NODEINST *ni, INTBIG index, BOOLEAN bottom);
static void    db_copyhierarchicaltraversal(HTRAVERSAL *src, HTRAVERSAL *dst);
       void    db_correctcellgroups(NODEPROTO *cell);

/*
 * Routine to free all memory associated with this module.
 */
void db_freenoprotomemory(void)
{
	if (db_hiertraverse.limit > 0)
	{
		efree((CHAR *)db_hiertraverse.path);
		efree((CHAR *)db_hiertraverse.index);
	}		
}

/************************* NODEPROTOS *************************/

/*
 * routine to allocate a nodeproto from memory cluster "cluster" and return
 * its address.  The routine returns NONODEPROTO if allocation fails.
 */
NODEPROTO *allocnodeproto(CLUSTER *cluster)
{
	REGISTER NODEPROTO *np;

	if (cluster == NOCLUSTER)
		return((NODEPROTO *)db_error(DBNOMEM|DBALLOCNODEPROTO));
	np = (NODEPROTO *)emalloc((sizeof (NODEPROTO)), cluster);
	if (np == 0) return((NODEPROTO *)db_error(DBNOMEM|DBALLOCNODEPROTO));
	np->protoname = x_("ERROR!");
	np->changeaddr = (CHAR *)NOCHANGE;
	np->userbits = np->temp1 = np->temp2 = 0;
	np->numvar = 0;
	np->firstvar = NOVARIABLE;
	np->firstinst = NONODEINST;
	np->firstnodeinst = NONODEINST;
	np->firstarcinst = NOARCINST;
	np->firstportproto = NOPORTPROTO;
	np->cachedequivcell = NONODEPROTO;
	np->nextcellgrp = np;
	np->nextcont = NONODEPROTO;
	np->numportprotos = 0;
	np->portprotohashtablesize = 0;
	np->tech = NOTECHNOLOGY;
	np->lib = NOLIBRARY;
	np->nextnodeproto = NONODEPROTO;
	np->cellview = NOVIEW;
	np->version = 1;
	np->prevversion = NONODEPROTO;
	np->newestversion = NONODEPROTO;
	np->creationdate = 0;
	np->revisiondate = 0;
	np->firstnetwork = NONETWORK;
	np->netd = 0;
	np->globalnetcount = 0;
	np->rtree = NORTNODE;
	np->primindex = 0;
	np->lowx = np->highx = np->lowy = np->highy = 0;
	np->prevnodeproto = NONODEPROTO;
	np->adirty = 0;
	return(np);
}

/*
 * routine to return nodeproto "np" to the pool of free nodes
 */
void freenodeproto(NODEPROTO *np)
{
	REGISTER INTBIG i;

	if (np == NONODEPROTO) return;
	if (np->globalnetcount > 0)
	{
		for(i=0; i<np->globalnetcount; i++)
			efree((CHAR *)np->globalnetnames[i]);
		efree((CHAR *)np->globalnetchar);
		efree((CHAR *)np->globalnetworks);
		efree((CHAR *)np->globalnetnames);
	}
	if (np->portprotohashtablesize > 0) efree((CHAR *)np->portprotohashtable);
	if (np->numvar != 0) db_freevars(&np->firstvar, &np->numvar);
	net_freenetprivate(np);
	efree((CHAR *)np);
}

/*
 * routine to create a new cell named "fname", in library
 * "lib".  The address of the cell is returned (NONODEPROTO if an error occurs).
 */
NODEPROTO *newnodeproto(CHAR *fname, LIBRARY *lib)
{
	REGISTER NODEPROTO *np, *onp;
	REGISTER CHAR *ptr, *opt, *cpt, *nameend;
	REGISTER CHAR save;
	REGISTER INTBIG explicitversion;
	REGISTER VIEW *view, *v;
	REGISTER void *infstr;

	/* copy the name to a temporary location so that it can be modified */
	infstr = initinfstr();
	addstringtoinfstr(infstr, fname);
	fname = returninfstr(infstr);

	/* search for view specification */
	view = el_unknownview;
	nameend = 0;
	for(ptr = fname; *ptr != 0; ptr++) if (*ptr == '{') break;
	if (*ptr == '{')
	{
		nameend = ptr;
		for(opt = ptr+1; *opt != 0; opt++) if (*opt == '}') break;
		if (*opt == 0 || opt[1] != 0)
		{
			db_donextchangequietly = FALSE;
			return((NODEPROTO *)db_error(DBBADNAME|DBNEWNODEPROTO));
		}
		*opt = 0;
		for(v = el_views; v != NOVIEW; v = v->nextview)
			if (namesame(&ptr[1], v->sviewname) == 0) break;
		if (v == NOVIEW) for(v = el_views; v != NOVIEW; v = v->nextview)
			if (namesame(&ptr[1], v->viewname) == 0) break;

		/* special case of schematic pages: create a view */
		if (v == NOVIEW && (ptr[1] == 'p' || ptr[1] == 'P') && isanumber(&ptr[2]))
		{
			/* create a new schematic page view */
			cpt = (CHAR *)emalloc((16+estrlen(&ptr[2])) * SIZEOFCHAR, el_tempcluster);
			if (cpt == 0)
			{
				db_donextchangequietly = FALSE;
				return(NONODEPROTO);
			}
			(void)estrcpy(cpt, x_("schematic-page-"));
			(void)estrcat(cpt, &ptr[2]);
			v = newview(cpt, &ptr[1]);
			efree(cpt);
		}

		*opt = '}';
		if (v == NOVIEW)
		{
			db_donextchangequietly = FALSE;
			return((NODEPROTO *)db_error(DBBADNAME|DBNEWNODEPROTO));
		}
		view = v;
	}

	/* search for version specification */
	explicitversion = 0;
	for(ptr = fname; *ptr != 0; ptr++) if (*ptr == ';') break;
	if (*ptr == ';')
	{
		if (nameend == 0 || ptr < nameend) nameend = ptr;
		explicitversion = eatoi(&ptr[1]);
		if (explicitversion <= 0)
		{
			db_donextchangequietly = FALSE;
			return((NODEPROTO *)db_error(DBBADNAME|DBNEWNODEPROTO));
		}
	}
	if (nameend == 0) nameend = ptr;

	/* scan main cell name for illegal characters */
	if (*fname == 0)
	{
		db_donextchangequietly = FALSE;
		return((NODEPROTO *)db_error(DBBADNAME|DBNEWNODEPROTO));
	}
	for(opt = fname; opt != ptr; opt++)
		if (*opt <= ' ' || *opt == ':' || *opt >= 0177)
	{
		db_donextchangequietly = FALSE;
		return((NODEPROTO *)db_error(DBBADNAME|DBNEWNODEPROTO));
	}

	/* create the cell */
	np = allocnodeproto(lib->cluster);
	if (np == NONODEPROTO)
	{
		db_donextchangequietly = FALSE;
		return(NONODEPROTO);
	}

	/* initialize */
	save = *nameend;
	*nameend = 0;
	(void)allocstring(&np->protoname, fname, lib->cluster);
	*nameend = save;
	np->primindex = 0;
	np->lowx = np->highx = 0;
	np->lowy = np->highy = 0;
	np->firstinst = NONODEINST;
	np->firstnodeinst = NONODEINST;
	np->firstarcinst = NOARCINST;
	np->tech = NOTECHNOLOGY;
	np->lib = lib;
	np->firstportproto = NOPORTPROTO;
	np->adirty = 0;
	np->cellview = view;
	np->tech = whattech(np);
	np->creationdate = (UINTBIG)getcurrenttime();
	np->revisiondate = (UINTBIG)getcurrenttime();
	net_initnetprivate(np);

	/* determine version number of this cell */
	if (explicitversion > 0)
	{
		np->version = explicitversion;
		for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
			if (onp->cellview == view && onp->version == explicitversion &&
				namesame(onp->protoname, np->protoname) == 0)
		{
			db_donextchangequietly = FALSE;
			return((NODEPROTO *)db_error(DBBADNAME|DBNEWNODEPROTO));
		}
	} else
	{
		np->version = 1;
		for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
			if (onp->cellview == view && namesame(onp->protoname, np->protoname) == 0 &&
				onp->version >= np->version) np->version = onp->version + 1;
	}

	/* figure out which cellgroup to add this to */
	for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
		if (namesame(onp->protoname, np->protoname) == 0 && onp->newestversion == onp) break;
	if (onp != NONODEPROTO) np->nextcellgrp = onp;

	/* insert in the library and cell structures */
	db_insertnodeproto(np);

	/* create initial R-tree data */
	if (geomstructure(np))
	{
		db_donextchangequietly = FALSE;
		return(NONODEPROTO);
	}

	/* handle change control, constraint, and broadcast */
	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		/* report the new cell */
		np->changeaddr = (CHAR *)db_change((INTBIG)np, NODEPROTONEW, 0, 0, 0, 0, 0, 0);

		/* tell constraint system about new cell */
		(*el_curconstraint->newobject)((INTBIG)np, VNODEPROTO);

		db_setchangecell(np);
	}
	db_donextchangequietly = FALSE;

	/* tell which cell it is */
	return(np);
}

/*
 * routine to create a new primitive nodeproto with name "fname", default
 * size "sx" by "sy", and primitive index "pi".  The nodeproto is placed in
 * technology "tech".  The address of the primitive nodeproto is returned.
 * Upon error, the value NONODEPROTO is returned.
 */
NODEPROTO *db_newprimnodeproto(CHAR *fname, INTBIG sx, INTBIG sy, INTBIG pi,
	TECHNOLOGY *tech)
{
	REGISTER NODEPROTO *np, *ent;
	REGISTER CHAR *pp;

	/* check for errors */
	for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (np->primindex == pi)
		{
			ttyputmsg(_("Index %ld of technology %s in use"), pi, tech->techname);
			return(NONODEPROTO);
		}
		if (namesame(fname, np->protoname) == 0)
		{
			ttyputmsg(_("Error: Two technologies with same name: %s and %s"), fname, tech->techname);
			return(NONODEPROTO);
		}
	}
	if (pi <= 0)
	{
		ttyputmsg(_("Error: Component index must be positive but %s:%s has %ld"),
			tech->techname, fname, pi);
		return(NONODEPROTO);
	}
	if (sx < 0 || sy < 0)
	{
		ttyputmsg(_("Error: Component size must be positive, but %s:%s is %ldx%ld"),
			tech->techname, fname, sx, sy);
		return(NONODEPROTO);
	}
	for(pp = fname; *pp != 0; pp++) if (*pp <= ' ' || *pp == ':' || *pp >= 0177)
	{
		ttyputmsg(_("Error: Primitive name must not contain ':' and others but '%s:%s' does"),
			tech->techname, fname);
		return(NONODEPROTO);
	}

	/* create the nodeproto */
	np = allocnodeproto(tech->cluster);
	if (np == NONODEPROTO)
	{
		ttyputnomemory();
		return(NONODEPROTO);
	}

	/* insert at the end of the list of primitives */
	if (tech->firstnodeproto == NONODEPROTO)
	{
		tech->firstnodeproto = np;   ent = NONODEPROTO;
	} else
	{
		ent = tech->firstnodeproto;
		while (ent->nextnodeproto != NONODEPROTO) ent = ent->nextnodeproto;
		ent->nextnodeproto = np;
	}
	np->prevnodeproto = ent;
	np->nextnodeproto = NONODEPROTO;

	/* initialize name and size */
	if (allocstring(&np->protoname, fname, tech->cluster))
	{
		ttyputnomemory();
		return(NONODEPROTO);
	}
	np->primindex = pi;
	np->lowx = -sx/2;   np->highx = np->lowx + sx;
	np->lowy = -sy/2;   np->highy = np->lowy + sy;
	np->firstinst = NONODEINST;
	np->firstnodeinst = NONODEINST;
	np->firstarcinst = NOARCINST;
	np->tech = tech;
	np->firstportproto = NOPORTPROTO;
	np->adirty = 0;

	/* tell which nodeproto it is */
	return(np);
}

/*
 * routine to kill nodeproto "np".  Returns true on error
 */
BOOLEAN killnodeproto(NODEPROTO *np)
{
	REGISTER NODEPROTO *lnt;
	REGISTER PORTPROTO *pp, *npt;

	/* cannot delete primitive nodeprotos */
	if (np == NONODEPROTO)
	{
		db_donextchangequietly = FALSE;
		return(db_error(DBBADPROTO|DBKILLNODEPROTO)!=0);
	}
	if (np->primindex != 0)
	{
		db_donextchangequietly = FALSE;
		return(db_error(DBPRIMITIVE|DBKILLNODEPROTO)!=0);
	}

	/* search the library for the nodeproto */
	for(lnt = np->lib->firstnodeproto; lnt != NONODEPROTO; lnt = lnt->nextnodeproto)
		if (np == lnt) break;
	if (lnt == NONODEPROTO)
	{
		db_donextchangequietly = FALSE;
		return(db_error(DBBADCELL|DBKILLNODEPROTO)!=0);
	}

	/* cannot delete nodeproto with any instances */
	if (np->firstinst != NONODEINST)
	{
		db_donextchangequietly = FALSE;
		return(db_error(DBHASINSTANCES|DBKILLNODEPROTO)!=0);
	}

	/* delete everything in the cell */
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = npt)
	{
		npt = pp->nextportproto;
		if (killportproto(np, pp)) ttyputerr(_("killportproto error"));
	}
	while (np->firstarcinst != NOARCINST) db_killarcinst(np->firstarcinst);
	while (np->firstnodeinst != NONODEINST) db_killnodeinst(np->firstnodeinst);

	/* remove nodeproto from library */
	db_retractnodeproto(np);

	/* handle change control, constraint, and broadcast */
	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		/* remove any "changecell" modules for this cell */
		db_removechangecell(np);

		/* record the change to the cell for broadcasting */
		np->changeaddr = (CHAR *)db_change((INTBIG)np, NODEPROTOKILL, 0, 0, 0, 0, 0, 0);

		/* tell constraint system about killed cell */
		(*el_curconstraint->killobject)((INTBIG)np, VNODEPROTO);
	} else
	{
		/* delete the cell now: no change control */
		freenodeproto(np);
	}
	db_donextchangequietly = FALSE;

	return(FALSE);
}

/*
 * routine to add nodeproto "np" to the list of cells in the library
 */
void db_insertnodeproto(NODEPROTO *np)
{
	REGISTER NODEPROTO *ent, *onp, *cellgrpchain1, *cellgrpchain2, *snp;
	REGISTER LIBRARY *lib;
	REGISTER INTBIG i, j;
	REGISTER BOOLEAN rebuildhash;

	/* see if this name/view already exists */
	lib = np->lib;
	rebuildhash = FALSE;
	snp = db_findnodeprotoname(np->protoname, np->cellview, lib);
	if (snp != NONODEPROTO)
	{
		/* this name/view does exist: see if we have a newer version */
		if (np->version > snp->version)
		{
			/* this new cell is the newest version */
			FOR_CELLGROUP(onp, snp)
				if (onp->nextcellgrp == snp) break;
			if (onp == NONODEPROTO)
			{
				ttyputmsg(M_("ERROR: Cannot replace newer version (v%ld) of cell %s"),
					np->version, np->protoname);
				return;
			}

			/* replace snp with np in the cellgroup chain */
			onp->nextcellgrp = np;
			np->nextcellgrp = snp->nextcellgrp;

			/* put old one in its own group */
			snp->nextcellgrp = snp;
			np->prevversion = snp;
			for(; snp != NONODEPROTO; snp = snp->prevversion)
				snp->newestversion = np;
			np->newestversion = np;

			/* this version is newer: rebuild hash table */
			rebuildhash = TRUE;
		} else
		{
			/* this new cell is an old version: find place in list of versions */
			np->nextcellgrp = np;
			np->newestversion = snp;
			ent = NONODEPROTO;
			for(; snp != NONODEPROTO; snp = snp->prevversion)
			{
				if (np->version >= snp->version) break;
				ent = snp;
			}
			ent->prevversion = np;
			np->prevversion = snp;
		}
	} else
	{
		/* insert into a cellgroup if requested */
		if (np->nextcellgrp != np)
		{
			cellgrpchain1 = np->nextcellgrp;
			cellgrpchain2 = cellgrpchain1->nextcellgrp;
			if (cellgrpchain2 == cellgrpchain1)
			{
				/* cell group has only 2 in it now */
				cellgrpchain1->nextcellgrp = np;
			} else
			{
				/* insert into longer chain */
				cellgrpchain1->nextcellgrp = np;
				np->nextcellgrp = cellgrpchain2;
			}
		}

		/* this name/view does not exist: add it to the hash table */
		np->newestversion = np;
		np->prevversion = NONODEPROTO;
		if (lib->numnodeprotos < lib->nodeprotohashtablesize/2)
		{
			i = db_namehash(np->protoname) + (INTBIG)np->cellview;
			i = abs(i) % lib->nodeprotohashtablesize;
			for(j=1; j<=lib->nodeprotohashtablesize; j += 2)
			{
				if (lib->nodeprotohashtable[i] == NONODEPROTO)
				{
					lib->nodeprotohashtable[i] = np;
					lib->nodeprotoviewhashtable[i] = np->cellview;
					break;
				}
				i += j;
				if (i >= lib->nodeprotohashtablesize) i -= lib->nodeprotohashtablesize;
			}
		} else
		{
			rebuildhash = TRUE;
		}
	}

	/* add this cell to the list of cells in the library */
	np->prevnodeproto = lib->tailnodeproto;
	np->nextnodeproto = NONODEPROTO;
	if (lib->tailnodeproto == NONODEPROTO) lib->firstnodeproto = np; else
		lib->tailnodeproto->nextnodeproto = np;
	lib->tailnodeproto = np;
	lib->numnodeprotos++;

	/* rebuild the hash table if requested */
	if (rebuildhash) db_buildnodeprotohashtable(lib);

#if 0
	/* insert into a cellgroup if requested */
	if (np->nextcellgrp != np && np->newestversion == np)
	{
		cellgrpchain1 = np->nextcellgrp;
		cellgrpchain2 = cellgrpchain1->nextcellgrp;
		if (cellgrpchain2 == cellgrpchain1)
		{
			/* cell group has only 2 in it now */
			cellgrpchain1->nextcellgrp = np;
		} else
		{
			/* insert into longer chain */
			cellgrpchain1->nextcellgrp = np;
			np->nextcellgrp = cellgrpchain2;
		}
	}
#endif

	/* mark a change to the database */
	db_changetimestamp++;
}

/*
 * routine to remove cell "np" from all lists in the library and cell
 */
void db_retractnodeproto(NODEPROTO *np)
{
	REGISTER NODEPROTO *onp, *lastnp;
	REGISTER LIBRARY *lib;

	/* remove cell from list in the library */
	lib = np->lib;
	if (np->prevnodeproto == NONODEPROTO)
		lib->firstnodeproto = np->nextnodeproto; else
			np->prevnodeproto->nextnodeproto = np->nextnodeproto;
	if (np->nextnodeproto == NONODEPROTO)
		lib->tailnodeproto = np->prevnodeproto; else
			np->nextnodeproto->prevnodeproto = np->prevnodeproto;
	lib->numnodeprotos--;

	if (np->newestversion != np)
	{
		/* easy to handle removal of old version */
		lastnp = NONODEPROTO;
		for(onp = np->newestversion; onp != NONODEPROTO; onp = onp->prevversion)
		{
			if (onp == np) break;
			lastnp = onp;
		}
		if (lastnp != NONODEPROTO) lastnp->prevversion = np->prevversion;
			else ttyputerr(_("Unusual version link in database"));
	} else
	{
		/* bring any older versions to the front */
		for(onp = np->prevversion; onp != NONODEPROTO; onp = onp->prevversion)
			onp->newestversion = np->prevversion;
	}

	/* clear cache of port associations in this cell */
	db_clearportcache(np);

	/* remove cellgroup link */
	db_removecellfromgroup(np);

	/* rebuild hash table of nodeproto names */
	db_buildnodeprotohashtable(lib);

	/* mark a change to the database */
	db_changetimestamp++;
}

/*
 * Routine to remove cell "np" from its cellgroup.
 */
void db_removecellfromgroup(NODEPROTO *np)
{
	REGISTER NODEPROTO *onp, *enp;
	REGISTER INTBIG count, truelimit;

	if (np->nextcellgrp != np)
	{
		/* find node that points to this */
		count = truelimit = 0;
		FOR_CELLGROUP(onp, np)
		{
			if (onp->nextcellgrp == np) break;
			count++;
			if (count > 1000)
			{
				if (truelimit == 0)
				{
					for(enp = np->lib->firstnodeproto; enp != NONODEPROTO; enp = enp->nextnodeproto)
						truelimit++;
					truelimit += 100;
				} else
				{
					if (count > truelimit) return;
				}
			}
		}
		if (onp != NONODEPROTO)
		{
			if (np->nextcellgrp == onp) onp->nextcellgrp = onp; else
				onp->nextcellgrp = np->nextcellgrp;
		}
	}
}

/*
 * Routine to rebuild the hash table of nodeproto names in library "lib".
 */
void db_buildnodeprotohashtable(LIBRARY *lib)
{
	REGISTER INTBIG i, j, numnodeprotos, hashtablesize;
	REGISTER NODEPROTO *np;

	/* free the former table */
	if (lib->nodeprotohashtablesize > 0)
	{
		lib->nodeprotohashtablesize = 0;
		efree((CHAR *)lib->nodeprotohashtable);
		efree((CHAR *)lib->nodeprotoviewhashtable);
	}

	/* determine the size of the hash table */
	numnodeprotos = 0;
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto) numnodeprotos++;
	if (numnodeprotos != lib->numnodeprotos)
	{
		ttyputmsg(_("Warning: number of nodeprotos in library %s corrected"), lib->libname);
		lib->numnodeprotos = numnodeprotos;
	}
	hashtablesize = pickprime(numnodeprotos * 4);

	/* create the hash table and clear it */
	lib->nodeprotohashtable = (NODEPROTO **)emalloc(hashtablesize * (sizeof (NODEPROTO *)), lib->cluster);
	if (lib->nodeprotohashtable == 0) return;
	lib->nodeprotoviewhashtable = (VIEW **)emalloc(hashtablesize * (sizeof (VIEW *)), lib->cluster);
	if (lib->nodeprotoviewhashtable == 0) return;
	for(i=0; i<hashtablesize; i++)
		lib->nodeprotohashtable[i] = NONODEPROTO;
	lib->nodeprotohashtablesize = hashtablesize;

	/* insert all nodeprotos into the table */
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (np->newestversion != np) continue;
		i = db_namehash(np->protoname) + (INTBIG)np->cellview;
		i = abs(i) % hashtablesize;
		for(j=1; j<=hashtablesize; j += 2)
		{
			if (lib->nodeprotohashtable[i] == NONODEPROTO)
			{
				lib->nodeprotohashtable[i] = np;
				lib->nodeprotoviewhashtable[i] = np->cellview;
				break;
			}
			i += j;
			if (i >= hashtablesize) i -= hashtablesize;
		}
	}
}

/*
 * Routine to find the nodeproto named "name" with view "view" in library "lib".
 * Returns NONODEPROTO if not found.
 */
NODEPROTO *db_findnodeprotoname(CHAR *name, VIEW *view, LIBRARY *lib)
{
	REGISTER NODEPROTO *np;
	REGISTER INTBIG i, j;

	if (lib->nodeprotohashtablesize > 0)
	{
		i = db_namehash(name) + (INTBIG)view;
		i = abs(i) % lib->nodeprotohashtablesize;
		for(j=1; j<=lib->nodeprotohashtablesize; j += 2)
		{
			np = lib->nodeprotohashtable[i];
			if (np == NONODEPROTO) break;
			if (namesame(name, np->protoname) == 0 &&
				lib->nodeprotoviewhashtable[i] == view) return(np);
			i += j;
			if (i >= lib->nodeprotohashtablesize) i -= lib->nodeprotohashtablesize;
		}
	} else
	{
		for (np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			if (namesame(name, np->protoname) == 0 && view == np->cellview) return(np);
	}
	return(NONODEPROTO);
}

#define	HASH_SH		5
#define	HASH_MSK	((1 << HASH_SH) - 1)

/*
 * Routine to encode a cell or portproto name for the hash table.
 */
INTBIG db_namehash(CHAR *name)
{
	REGISTER INTBIG hash, shiftout, c;

	hash = 0;
	while ((c = *name++ & 0377) != 0)
	{
		c = tolower(c);
		shiftout = (hash >> (31 - HASH_SH)) & HASH_MSK;
		hash = ((hash << HASH_SH) | shiftout) ^ c;
	}
	hash &= 0x7fffffff;
	return(hash);
}

/*
 * routine to return the current cell (the one in the current window).
 * returns NONODEPROTO if there is none.
 */
NODEPROTO *getcurcell(void)
{
	if (el_curwindowpart != NOWINDOWPART) return(el_curwindowpart->curnodeproto);
	return(NONODEPROTO);
}

/******************** HIERARCHICAL TRAVERSER ********************/

/*
 * routine to return the current path to this location (cell "here") in the traversal.  The depth is put
 * in "depth" and the list of NODEINSTs is put in "nilist".  The first entry in the array
 * is the top of the hierarchy, and the last entry is the instance in whose definition
 * the traversal currently resides.
 */
void gettraversalpath(NODEPROTO *here, WINDOWPART *win, NODEINST ***nilist, INTBIG **indexlist, INTBIG *depth, INTBIG maxdepth)
{
	REGISTER NODEPROTO *np, *cnp;
	REGISTER NODEINST *ni;
	INTBIG index;
	Q_UNUSED( maxdepth );

	if (db_hiertraversing)
	{
		/* see if any additional hierarchy can be found at the top of the list */
		if (db_hiertraverse.depth > 0) np = db_hiertraverse.path[0]->parent; else
			np = here;
		if (np != NONODEPROTO)
		{
			for(;;)
			{
				if (np == NONODEPROTO) break;
				ni = descentparent(np, &index, win, 0);
				if (ni == NONODEINST) break;
				if (db_addtohierpath(ni, 0, FALSE)) break;
				np = ni->parent;
				cnp = contentsview(np);
				if (cnp != NONODEPROTO) np = cnp;
			}
		}
		*depth = db_hiertraverse.depth - db_hiertempclimb;
		*indexlist = db_hiertraverse.index;
		*nilist = db_hiertraverse.path;
	} else
	{
		/* not in an active traversal, see if one exists from edit sequences */
		np = here;
		for(;;)
		{
			if (np == NONODEPROTO) break;
			ni = descentparent(np, &index, win, 0);
			if (ni == NONODEINST) break;
			if (db_addtohierpath(ni, 0, FALSE)) break;
			np = ni->parent;
			cnp = contentsview(np);
			if (cnp != NONODEPROTO) np = cnp;
		}
		*depth = db_hiertraverse.depth - db_hiertempclimb;
		*indexlist = db_hiertraverse.index;
		*nilist = db_hiertraverse.path;
		db_hiertraverse.depth = 0;
	}
}

/*
 * internal routine to return the current path to this location in the traversal,
 * ignoring implicit paths made by editing.  The depth is put in "depth" and the list
 * of NODEINSTs is put in "nilist".  The first entry in the array is the top of the
 * hierarchy, and the last entry is the instance in whose definition the traversal
 * currently resides.
 */
void db_gettraversalpath(NODEPROTO *here, WINDOWPART *win, NODEINST ***nilist, INTBIG *depth)
{
	Q_UNUSED( here );
	*nilist = db_hiertraverse.path;
	*depth = db_hiertraverse.depth - db_hiertempclimb;
}

/*
 * routine to begin the traversal of the hierarchy.  At each level of the
 * traversal, the routine "downhierarchy" is called with the cell and the user data.
 */
void begintraversehierarchy(void)
{
	db_hiertraversing = TRUE;
	db_hiertraverse.depth = 0;
	db_hiertempclimb = 0;
	db_traversaltimestamp++;
}

/*
 * routine to stop the traversal of the hierarchy.
 */
void endtraversehierarchy(void)
{
	db_hiertraversing = FALSE;
}

/*
 * routine to traversal down the hierarchy, into nodeinst "ni", index "index" of
 * the array (if the node is arrayed).
 */
void downhierarchy(NODEINST *ni, NODEPROTO *np, INTBIG index)
{
	Q_UNUSED( np );
	/* add this node to the traversal stack */
	if (db_addtohierpath(ni, index, TRUE)) return;
	db_traversaltimestamp++;
}

/*
 * Routine to report the levels of hierarchy current being "popped out".
 */
INTBIG getpopouthierarchy(void)
{
	return(db_hiertempclimb);
}

/*
 * Temporarily climb "climb" levels out of the hierarchy stack.
 */
void popouthierarchy(INTBIG climb)
{
	db_hiertempclimb = climb;
}

BOOLEAN db_addtohierpath(NODEINST *ni, INTBIG index, BOOLEAN bottom)
{
	REGISTER INTBIG i, newlimit, *newindex;
	REGISTER NODEINST **nilist;

	/* make sure there is room in the list of nodes traversed */
	if (db_hiertraverse.depth >= db_hiertraverse.limit)
	{
		newlimit = db_hiertraverse.limit+10;
		if (newlimit < db_hiertraverse.depth) newlimit = db_hiertraverse.depth;
		nilist = (NODEINST **)emalloc(newlimit * (sizeof (NODEINST *)), db_cluster);
		if (nilist == 0) return(TRUE);
		newindex = (INTBIG *)emalloc(newlimit * SIZEOFINTBIG, db_cluster);
		if (newindex == 0) return(TRUE);
		for(i=0; i<db_hiertraverse.depth; i++)
		{
			nilist[i] = db_hiertraverse.path[i];
			newindex[i] = db_hiertraverse.index[i];
		}
		if (db_hiertraverse.limit > 0)
		{
			efree((CHAR *)db_hiertraverse.path);
			efree((CHAR *)db_hiertraverse.index);
		}
		db_hiertraverse.path = nilist;
		db_hiertraverse.index = newindex;
		db_hiertraverse.limit = newlimit;
	}
	if (bottom)
	{
		db_hiertraverse.path[db_hiertraverse.depth] = ni;
		db_hiertraverse.index[db_hiertraverse.depth] = index;
	} else
	{
		for(i=db_hiertraverse.depth; i>0; i--)
		{
			db_hiertraverse.path[i] = db_hiertraverse.path[i-1];
			db_hiertraverse.index[i] = db_hiertraverse.index[i-1];
		}
		db_hiertraverse.path[0] = ni;
		db_hiertraverse.index[0] = index;
	}
	db_hiertraverse.depth++;
	return(FALSE);
}

/*
 * routine to back out of a hierarchy traversal
 */
void uphierarchy(void)
{
	if (db_hiertraverse.depth <= 0) return;

	/* pop the node off the traversal stack */
	db_hiertraverse.depth--;
	db_traversaltimestamp++;
}

/*
 * Routine to determine the node instance that is the proper parent of cell "np"
 * in window "win" (if "win" is NOWINDOWPART, ignore that factor).
 * This is done by examining the variable "USER_descent_path" on the cell, which
 * is created when the user goes "down the hierarchy" into this cell.  The routine
 * not only extracts this information, but it verifies that the instance still
 * exists.  Returns NONODEINST if there is no parent.
 */
NODEINST *descentparent(NODEPROTO *np, INTBIG *index, WINDOWPART *win, INTBIG *viewinfo)
{
	REGISTER VARIABLE *var;
	REGISTER INTBIG *curlist, i, j, len;
	REGISTER NODEINST *ni, *vni;
	REGISTER WINDOWPART *w;
	REGISTER NODEPROTO *inp;

	/* find this level in the stack */
	if (db_descent_path_key == 0)
		db_descent_path_key = makekey(x_("USER_descent_path"));
	var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER|VISARRAY, db_descent_path_key);
	if (var == NOVARIABLE) return(NONODEINST);
	len = getlength(var);
	curlist = (INTBIG *)var->addr;
	for(i=0; i<len; i+=25)
	{
		ni = (NODEINST *)curlist[i];
		w = (WINDOWPART *)curlist[i+1];
		if (ni->parent == np) continue;
		if (win != NOWINDOWPART && win != w) continue;

		*index = curlist[i+2];
		if (viewinfo != 0)
		{
			for(j=0; j<22; j++) viewinfo[j] = curlist[i+3+j];
		}

		/* validate this pointer: node must be a current instance of this cell */
		for(vni = np->firstinst; vni != NONODEINST; vni = vni->nextinst)
			if (vni == ni) return(ni);

		/* might be an icon */
		for(inp = iconview(np); inp != NONODEPROTO; inp = inp->prevversion)
		{
			for(vni = inp->firstinst; vni != NONODEINST; vni = vni->nextinst)
				if (vni == ni) return(ni);
		}
	}
	return(NONODEINST);
}

void sethierarchicalparent(NODEPROTO *np, NODEINST *parentni, WINDOWPART *win, INTBIG thisindex, INTBIG *viewinfo)
{
	REGISTER WINDOWPART *w, *ow;
	REGISTER VARIABLE *var;
	REGISTER NODEINST *ni;
	REGISTER INTBIG i, j, k, len, *curlist, *newlist, index;
	INTBIG oneentry[25];

	if (db_descent_path_key == 0)
		db_descent_path_key = makekey(x_("USER_descent_path"));
	var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER|VISARRAY, db_descent_path_key);
	if (var == NOVARIABLE)
	{
		oneentry[0] = (INTBIG)parentni;
		oneentry[1] = (INTBIG)win;
		oneentry[2] = thisindex;
		if (viewinfo == 0)
		{
			for(i=0; i<22; i++) oneentry[i+3] = 0;
		} else
		{
			for(i=0; i<22; i++) oneentry[i+3] = viewinfo[i];
		}
		(void)setvalkey((INTBIG)np, VNODEPROTO, db_descent_path_key, (INTBIG)oneentry,
			VINTEGER|VISARRAY|(25<<VLENGTHSH)|VDONTSAVE);
	} else
	{
		/* validate the entries */
		len = getlength(var);
		newlist = (INTBIG *)emalloc((len+25) * SIZEOFINTBIG, el_tempcluster);
		if (newlist == 0) return;
		curlist = (INTBIG *)var->addr;
		j = 0;
		for(i=0; i<len; i += 25)
		{
			ni = (NODEINST *)curlist[i];
			w = (WINDOWPART *)curlist[i+1];
			index = curlist[i+2];

			/* remove entries that point to the same parent instance */
			if (ni == parentni) continue;

			/* remove entries that point to the same window */
			if (w == win) continue;

			/* remove obsolete windows */
			if (w != NOWINDOWPART)
			{
				for(ow = el_topwindowpart; ow != NOWINDOWPART; ow = ow->nextwindowpart)
					if (ow == w) break;
				if (ow == NOWINDOWPART) continue;
			}

			/* keep the existing entry */
			newlist[j++] = (INTBIG)ni;
			newlist[j++] = (INTBIG)w;
			newlist[j++] = index;
			for(k=0; k<22; k++) newlist[j++] = curlist[i+3+k];
		}

		/* add the new entry */
		newlist[j++] = (INTBIG)parentni;
		newlist[j++] = (INTBIG)win;
		newlist[j++] = thisindex;
		if (viewinfo == 0)
		{
			for(i=0; i<22; i++) newlist[j++] = 0;
		} else
		{
			for(i=0; i<22; i++) newlist[j++] = viewinfo[i];
		}
		(void)setvalkey((INTBIG)np, VNODEPROTO, db_descent_path_key, (INTBIG)newlist,
			VINTEGER|VISARRAY|(j<<VLENGTHSH)|VDONTSAVE);
		efree((CHAR *)newlist);
	}
}

/*
 * Routine to create a hierarchical traversal snapshot object and return it.
 * Returns zero on error.
 */
void *newhierarchicaltraversal(void)
{
	REGISTER HTRAVERSAL *ht;

	ht = (HTRAVERSAL *)emalloc(sizeof (HTRAVERSAL), db_cluster);
	if (ht == 0) return(0);
	ht->limit = 0;
	return((void *)ht);
}

/*
 * Routine to take a snapshot of the current hierarchical traversal and
 * save it in "snapshot" (an object returned by "newhierarchicaltraversal()").
 */
void gethierarchicaltraversal(void *snapshot)
{
	REGISTER HTRAVERSAL *ht;

	ht = (HTRAVERSAL *)snapshot;
	db_copyhierarchicaltraversal(&db_hiertraverse, ht);
}

/*
 * Routine to set saved hierarchical traversal "snapshot" as the current hierarchical traversal.
 */
void sethierarchicaltraversal(void *snapshot)
{
	REGISTER HTRAVERSAL *ht;

	ht = (HTRAVERSAL *)snapshot;
	db_copyhierarchicaltraversal(ht, &db_hiertraverse);
}

/*
 * Routine to delete a saved hierarchy traversal snapshot.
 */
void killhierarchicaltraversal(void *snapshot)
{
	REGISTER HTRAVERSAL *ht;

	ht = (HTRAVERSAL *)snapshot;
	if (ht->limit > 0)
	{
		efree((CHAR *)ht->path);
		efree((CHAR *)ht->index);
	}
	efree((CHAR *)ht);
}

/*
 * Internal routine to copy hierarchical traversal stacks.
 */
void db_copyhierarchicaltraversal(HTRAVERSAL *src, HTRAVERSAL *dst)
{
	REGISTER INTBIG i, *newindex;
	REGISTER NODEINST **newpath;

	if (src->depth > dst->limit)
	{
		newpath = (NODEINST **)emalloc(src->depth * (sizeof (NODEINST *)), db_cluster);
		if (newpath == 0) return;
		newindex = (INTBIG *)emalloc(src->depth * SIZEOFINTBIG, db_cluster);
		if (newindex == 0) return;

		if (dst->limit > 0)
		{
			efree((CHAR *)dst->path);
			efree((CHAR *)dst->index);
		}
		dst->path = newpath;
		dst->index = newindex;
		dst->limit = src->depth;
	}
	for(i=0; i<src->depth; i++)
	{
		dst->path[i] = src->path[i];
		dst->index[i] = src->index[i];
	}
	dst->depth = src->depth;
}

/************************* VIEWS *************************/

/*
 * routine to allocate a view and return its address.  The routine returns
 * NOVIEW if allocation fails.
 */
VIEW *allocview(void)
{
	REGISTER VIEW *v;

	v = (VIEW *)emalloc((sizeof (VIEW)), db_cluster);
	if (v == 0) return((VIEW *)db_error(DBNOMEM|DBALLOCVIEW));
	v->viewname = v->sviewname = NOSTRING;
	v->nextview = NOVIEW;
	v->temp1 = v->temp2 = 0;
	v->viewstate = 0;
	v->firstvar = NOVARIABLE;
	v->numvar = 0;
	return(v);
}

/*
 * routine to return view "v" to free memory
 */
void freeview(VIEW *v)
{
	if (v == NOVIEW) return;
	if (v->numvar != 0) db_freevars(&v->firstvar, &v->numvar);
	efree((CHAR *)v);
}

/*
 * routine to create a new view with name "vname" and short view name "svname".
 * The address of the view is returned (NOVIEW if an error occurs).
 */
VIEW *newview(CHAR *vname, CHAR *svname)
{
	REGISTER VIEW *v;
	REGISTER CHAR *ptr;

	/* error checks */
	for(ptr = vname; *ptr != 0; ptr++)
		if (*ptr <= ' ' || *ptr == ':' || *ptr >= 0177)
	{
		db_donextchangequietly = FALSE;
		return((VIEW *)db_error(DBBADNAME|DBNEWVIEW));
	}
	for(ptr = svname; *ptr != 0; ptr++)
		if (*ptr <= ' ' || *ptr == ':' || *ptr >= 0177)
	{
		db_donextchangequietly = 0;
		return((VIEW *)db_error(DBBADNAME|DBNEWVIEW));
	}
	for(v = el_views; v != NOVIEW; v = v->nextview)
		if (namesame(v->viewname, vname) == 0 || namesame(v->sviewname, svname) == 0)
	{
		db_donextchangequietly = 0;
		return((VIEW *)db_error(DBDUPLICATE|DBNEWVIEW));
	}

	/* create the view */
	v = allocview();
	if (v == NOVIEW)
	{
		db_donextchangequietly = 0;
		return(NOVIEW);
	}

	/* initialize view names */
	if (allocstring(&v->viewname, vname, db_cluster))
	{
		db_donextchangequietly = 0;
		return(NOVIEW);
	}
	if (allocstring(&v->sviewname, svname, db_cluster))
	{
		db_donextchangequietly = 0;
		return(NOVIEW);
	}
	if (namesamen(vname, x_("Schematic-Page-"), 15) == 0) v->viewstate |= MULTIPAGEVIEW;

	/* insert in list of views */
	v->nextview = el_views;
	el_views = v;

	/* handle change control and broadcast */
	if (db_donextchangequietly == 0 && !db_dochangesquietly)
	{
		(void)db_change((INTBIG)v, OBJECTNEW, VVIEW, 0, 0, 0, 0, 0);
	}
	db_donextchangequietly = 0;

	/* mark a change to the database */
	db_changetimestamp++;

	/* return the view */
	return(v);
}

/*
 * routine to remove view "view".  Returns true on error
 */
BOOLEAN killview(VIEW *view)
{
	REGISTER VIEW *v, *lastv;
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;

	/* make sure it is not one of the permanent (undeletable) views */
	if ((view->viewstate&PERMANENTVIEW) != 0)
	{
		db_donextchangequietly = 0;
		return(TRUE);
	}

	/* make sure it is not in any cell */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			if (np->cellview == view)
	{
		db_donextchangequietly = 0;
		return(TRUE);
	}

	/* find the view */
	lastv = NOVIEW;
	for(v = el_views; v != NOVIEW; v = v->nextview)
	{
		if (v == view) break;
		lastv = v;
	}
	if (v == NOVIEW) return(TRUE);

	/* delete the view */
	if (lastv == NOVIEW) el_views = v->nextview; else
		lastv->nextview = v->nextview;

	/* handle change control and broadcast */
	if (db_donextchangequietly == 0 && !db_dochangesquietly)
	{
		(void)db_change((INTBIG)v, OBJECTKILL, VVIEW, 0, 0, 0, 0, 0);
	}
	db_donextchangequietly = 0;

	/* mark a change to the database */
	db_changetimestamp++;
	return(FALSE);
}

/*
 * routine to change the view type of cell "np" to "view".  Returns true
 * upon error.
 */
BOOLEAN changecellview(NODEPROTO *np, VIEW *view)
{
	REGISTER NODEPROTO *rnp, *otheringrp;

	if ((np->cellview->viewstate&TEXTVIEW) != 0 && (view->viewstate&TEXTVIEW) == 0)
	{
		ttyputerr(_("Sorry, textual views cannot be made nontextual"));
		return(TRUE);
	}
	if ((np->cellview->viewstate&TEXTVIEW) == 0 && (view->viewstate&TEXTVIEW) != 0)
	{
		ttyputerr(_("Sorry, nontextual views cannot be made textual"));
		return(TRUE);
	}

	/* remove this cell from the linked lists */
	otheringrp = np->nextcellgrp;
	db_retractnodeproto(np);

	/* assign a newer version number if there are others with the new view */
	if (otheringrp != np)
	{
		FOR_CELLGROUP(rnp, otheringrp)
		{
			if (rnp->cellview == view)
			{
				if (np->version <= rnp->version) np->version = rnp->version + 1;
				break;
			}
		}
	}

	/* set the view (sorry, not undoable) */
	np->cellview = view;

	/* reinsert the cell in the linked lists */
	db_insertnodeproto(np);
	return(FALSE);
}

/*
 * Routine to return true if cell "np" is a schematic view
 */
BOOLEAN isschematicview(NODEPROTO *np)
{
	if (np->cellview == el_schematicview ||
		(np->cellview->viewstate&MULTIPAGEVIEW) != 0) return(TRUE);
	return(FALSE);
}

/*
 * Routine to return true if cell "np" is an icon view
 */
BOOLEAN isiconview(NODEPROTO *np)
{
	if (np->cellview == el_iconview) return(TRUE);
	return(FALSE);
}

/*
 * Routine to return TRUE if cells "a" and "b" are in the same cell group.
 */
BOOLEAN insamecellgrp(NODEPROTO *a, NODEPROTO *b)
{
	REGISTER NODEPROTO *np;

	a = a->newestversion;
	b = b->newestversion;
	FOR_CELLGROUP(np, a)
		if (np == b) return(TRUE);
	return(FALSE);
}

/*
 * Routine to return TRUE if cell "subnp" is an icon of the cell "cell".
 */
BOOLEAN isiconof(NODEPROTO *subnp, NODEPROTO *cell)
{
	REGISTER NODEPROTO *np;
	REGISTER INTBIG i;

	if (subnp->cellview != el_iconview) return(FALSE);
	subnp = subnp->newestversion;
	i = 0;
	FOR_CELLGROUP(np, cell)
	{
		if (np == subnp) return(TRUE);
		if (i++ > 1000)
		{
			db_correctcellgroups(cell);
			break;
		}
	}
	return(FALSE);
}

/*
 * Routine to repair the cellgroups when there is an infinite loop
 */
void db_correctcellgroups(NODEPROTO *cell)
{
	NODEPROTO *onp, *first, *thefirst;

	ttyputerr(_("Problems with cell group %s...repairing"), describenodeproto(cell));
	first = NONODEPROTO;
	for(onp = cell->lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
	{
		if (namesame(onp->protoname, cell->protoname) != 0) continue;
		if (onp->newestversion != onp)
		{
			onp->nextcellgrp = onp;
			continue;
		}
		if (first != NONODEPROTO) first->nextcellgrp = onp; else
			thefirst = onp;
		first = onp;
	}
	if (first != NONODEPROTO) first->nextcellgrp = thefirst;
}

/*
 * routine to see if cell "np" has an icon, and if so, return that cell.
 * Returns NONODEPROTO if the cell does not have an icon
 */
NODEPROTO *iconview(NODEPROTO *np)
{
	REGISTER NODEPROTO *rnp;
	REGISTER INTBIG i;

	if (np == NONODEPROTO) return(NONODEPROTO);

	/* must be complex */
	if (np->primindex != 0) return(NONODEPROTO);

	/* can only get icon view if this is a schematic */
	if (!isschematicview(np)) return(NONODEPROTO);

	/* now look for views */
	i = 0;
	FOR_CELLGROUP(rnp, np)
	{
		if (rnp->cellview == el_iconview) return(rnp);
		if (i++ > 1000)
		{
			db_correctcellgroups(np);
			break;
		}
	}

	return(NONODEPROTO);
}

/*
 * routine to see if cell "np" is an icon or a skeleton, and if so, return its
 * true contents cell.  Returns NONODEPROTO if the contents cell is not found.
 */
NODEPROTO *contentsview(NODEPROTO *np)
{
	REGISTER NODEPROTO *rnp;
	REGISTER INTBIG i;

	/* primitives have no contents view */
	if (np == NONODEPROTO) return(NONODEPROTO);
	if (np->primindex != 0) return(NONODEPROTO);

	/* can only consider contents if this cell is an icon */
	if (np->cellview != el_iconview && np->cellview != el_skeletonview)
		return(NONODEPROTO);

	/* first check to see if there is a schematics link */
	i = 0;
	FOR_CELLGROUP(rnp, np)
	{
		if (rnp->cellview == el_schematicview) return(rnp);
		if ((rnp->cellview->viewstate&MULTIPAGEVIEW) != 0) return(rnp);
		if (i++ > 1000)
		{
			db_correctcellgroups(np);
			break;
		}
	}

	/* now check to see if there is any layout link */
	FOR_CELLGROUP(rnp, np)
		if (rnp->cellview == el_layoutview) return(rnp);

	/* finally check to see if there is any "unknown" link */
	FOR_CELLGROUP(rnp, np)
		if (rnp->cellview == el_unknownview) return(rnp);

	/* no contents found */
	return(NONODEPROTO);
}

/* function to return a layout view if there is one */
NODEPROTO *layoutview(NODEPROTO *np)
{
	REGISTER NODEPROTO *rnp, *laynp, *schnp, *uknnp;
	REGISTER INTBIG i;

	if (np == NONODEPROTO) return(NONODEPROTO);

	/* must be complex */
	if (np->primindex != 0) return(NONODEPROTO);

	/* Now look for views */
	laynp = schnp = uknnp = NONODEPROTO;
	i = 0;
	FOR_CELLGROUP(rnp, np)
	{
		if (rnp->cellview == el_layoutview) laynp = rnp;
		if (rnp->cellview == el_schematicview ||
			(rnp->cellview->viewstate&MULTIPAGEVIEW) != 0) schnp = rnp;
		if (rnp->cellview == el_unknownview) uknnp = rnp;
		if (i++ > 1000)
		{
			db_correctcellgroups(np);
			break;
		}
	}

	/* if this is an icon, look for schematic first */
	if (np->cellview == el_iconview && schnp != NONODEPROTO) return(schnp);

	/* layout has first precedence */
	if (laynp != NONODEPROTO) return(laynp);

	/* schematics come next */
	if (schnp != NONODEPROTO) return(schnp);

	/* then look for unknown */
	if (uknnp != NONODEPROTO) return(uknnp);

	/* keep what we have */
	return(NONODEPROTO);
}

/*
 * routine to see if cell "np" has an equivalent of view "v", and if so, return that cell.
 * Returns NONODEPROTO if the cell does not have an equivalent in that view.
 */
NODEPROTO *anyview(NODEPROTO *np, VIEW *v)
{
	REGISTER NODEPROTO *rnp;
	REGISTER INTBIG i;

	/* primitives have no icon view */
	if (np->primindex != 0) return(NONODEPROTO);

	/* return self if already that type */
	if (np->cellview == v) return(np);

	i = 0;
	FOR_CELLGROUP(rnp, np)
	{
		if (rnp->cellview == v) return(rnp);
		if (i++ > 1000)
		{
			db_correctcellgroups(np);
			break;
		}
	}
	return(NONODEPROTO);
}

/*
 * routine to obtain the equivalent port on a cell with an alternate view.
 * The original cell is "np" with port "pp".  The routine returns the port
 * on equivalent cell "enp".  If the port association cannot be found,
 * the routine returns NOPORTPROTO and prints an error message.
 */
PORTPROTO *equivalentport(NODEPROTO *np, PORTPROTO *pp, NODEPROTO *enp)
{
	REGISTER PORTPROTO *epp, *opp;

	/* don't waste time searching if the two views are the same */
	if (np == enp) return(pp);
	if (pp == NOPORTPROTO) return(pp);

	/* load the cache if not already there */
	if (enp != np->cachedequivcell)
	{
		for(opp = np->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
			opp->cachedequivport = NOPORTPROTO;
		for(opp = np->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
		{
			epp = getportproto(enp, opp->protoname);
			if (epp != NOPORTPROTO) opp->cachedequivport = epp;
		}
		np->cachedequivcell = enp;
	}
	epp = pp->cachedequivport;
	if (epp != 0 && epp != NOPORTPROTO) return(epp);

	/* don't report errors for global ports not on icons */
	if (epp == NOPORTPROTO)
	{
		if (enp->cellview != el_iconview || (pp->userbits&BODYONLY) == 0)
			ttyputerr(_("Warning: no port in cell %s corresponding to port %s in cell %s"),
				describenodeproto(enp), pp->protoname, describenodeproto(np));
	}
	pp->cachedequivport = (PORTPROTO *)0;
	return(NOPORTPROTO);
}

/************************* SCHEMATIC WINDOWS *************************/

static GRAPHICS tech_frame = {LAYERO, MENTXT, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
	0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};

#define FRAMESCALE  700.0f
#define HASCHXSIZE  ( 8.5f  / FRAMESCALE)
#define HASCHYSIZE  ( 5.5f  / FRAMESCALE)
#define ASCHXSIZE   (11.0f  / FRAMESCALE)
#define ASCHYSIZE   ( 8.5f  / FRAMESCALE)
#define BSCHXSIZE   (17.0f  / FRAMESCALE)
#define BSCHYSIZE   (11.0f  / FRAMESCALE)
#define CSCHXSIZE   (24.0f  / FRAMESCALE)
#define CSCHYSIZE   (17.0f  / FRAMESCALE)
#define DSCHXSIZE   (36.0f  / FRAMESCALE)
#define DSCHYSIZE   (24.0f  / FRAMESCALE)
#define ESCHXSIZE   (48.0f  / FRAMESCALE)
#define ESCHYSIZE   (36.0f  / FRAMESCALE)
#define FRAMEWID    ( 0.15f / FRAMESCALE)
#define XLOGOBOX    ( 2.0f  / FRAMESCALE)
#define YLOGOBOX    ( 1.0f  / FRAMESCALE)

/*
 * routine to determine whether cell "np" should have a schematic frame drawn in it.
 * Returns 0: there should be a frame whose size is absolute (sets "x" and "y" to the size of the frame)
 * Returns 1: there should be a frame but it combines with other stuff in the cell ("x/y" are frame size)
 * Returns 2: there is no frame.
 */
INTBIG framesize(INTBIG *x, INTBIG *y, NODEPROTO *np)
{
	CHAR *size;
	float wid, hei;
	REGISTER INTBIG retval;

	size = db_getframedesc(np);
	if (*size == 0) return(2);
	retval = 0;
	switch (size[0])
	{
		case 'h': wid = HASCHXSIZE;  hei = HASCHYSIZE;  break;
		case 'a': wid = ASCHXSIZE;   hei = ASCHYSIZE;   break;
		case 'b': wid = BSCHXSIZE;   hei = BSCHYSIZE;   break;
		case 'c': wid = CSCHXSIZE;   hei = CSCHYSIZE;   break;
		case 'd': wid = DSCHXSIZE;   hei = DSCHYSIZE;   break;
		case 'e': wid = ESCHXSIZE;   hei = ESCHYSIZE;   break;
		case 'x': wid = XLOGOBOX+FRAMEWID;    hei = YLOGOBOX+FRAMEWID;    retval = 1;   break;
		default:  wid = DSCHXSIZE;   hei = DSCHYSIZE;   break;
	}
	if (size[1] == 'v')
	{
		*x = scalefromdispunit(hei - FRAMEWID, DISPUNITINCH);
		*y = scalefromdispunit(wid - FRAMEWID, DISPUNITINCH);
	} else
	{
		*x = scalefromdispunit(wid - FRAMEWID, DISPUNITINCH);
		*y = scalefromdispunit(hei - FRAMEWID, DISPUNITINCH);
	}
	return(retval);
}

INTBIG framepolys(NODEPROTO *np)
{
	REGISTER INTBIG xsections, ysections, tpolys, total;
	CHAR *size;

	size = db_getframedesc(np);
	if (*size == 0) return(0);
	if (size[0] == 'x')
	{
		tpolys = 12;
		xsections = 0;
		ysections = 0;
	} else
	{
		tpolys = 12;
		size++;
		while (*size != 0)
		{
			if (*size == 'n') tpolys = 0;
			size++;
		}
		xsections = 4;
		ysections = 8;
	}

	/* compute number of polygons */
	total = 2+tpolys;
	if (xsections > 0 && ysections > 0)
		total += (xsections-1)*2+(ysections-1)*2+xsections*2+ysections*2;
	return(total);
}

void framepoly(INTBIG boxnum, POLYGON *poly, NODEPROTO *np)
{
	REGISTER INTBIG pindex, xsections, ysections, dotitle, titleoffset;
	REGISTER INTBIG xsecsize, ysecsize, schxsize, schysize, framewid, xlogobox, ylogobox;
	float wid, hei;
	REGISTER VARIABLE *var;
	static CHAR line[10];
	static CHAR ndstring[320];
	CHAR *size;

	/* get true sizes */
	size = db_getframedesc(np);
	dotitle = 1;
	xsections = 8;
	ysections = 4;
	switch (size[0])
	{
		case 'h': wid = HASCHXSIZE;  hei = HASCHYSIZE;  break;
		case 'a': wid = ASCHXSIZE;   hei = ASCHYSIZE;   break;
		case 'b': wid = BSCHXSIZE;   hei = BSCHYSIZE;   break;
		case 'c': wid = CSCHXSIZE;   hei = CSCHYSIZE;   break;
		case 'd': wid = DSCHXSIZE;   hei = DSCHYSIZE;   break;
		case 'e': wid = ESCHXSIZE;   hei = ESCHYSIZE;   break;
		case 'x': wid = XLOGOBOX+FRAMEWID*3;    hei = YLOGOBOX+FRAMEWID*3;    xsections = ysections = 0; break;
		default:  wid = DSCHXSIZE;   hei = DSCHYSIZE;   break;
	}
	size++;
	framewid = scalefromdispunit(FRAMEWID, DISPUNITINCH);
	xlogobox = scalefromdispunit(XLOGOBOX, DISPUNITINCH);
	ylogobox = scalefromdispunit(YLOGOBOX, DISPUNITINCH);
	schxsize = scalefromdispunit(wid, DISPUNITINCH) - framewid;
	schysize = scalefromdispunit(hei, DISPUNITINCH) - framewid;
	while (*size != 0)
	{
		if (*size == 'v')
		{
			xsections = 4;
			ysections = 8;
			schxsize = scalefromdispunit(hei, DISPUNITINCH) - framewid;
			schysize = scalefromdispunit(wid, DISPUNITINCH) - framewid;
		} else if (*size == 'n')
		{
			dotitle = 0;
		}
		size++;
	}

	if (poly->limit < 4) (void)extendpolygon(poly, 4);

	if (xsections > 0 && ysections > 0)
	{
		xsecsize = (schxsize - framewid*2) / xsections;
		ysecsize = (schysize - framewid*2) / ysections;
		titleoffset = (xsections-1)*2+(ysections-1)*2+xsections*2+ysections*2;
	} else titleoffset = 0;
	if (boxnum <= 1)
	{
		/* draw the frame */
		if (xsections == 0)
		{
			if (boxnum == 0)
			{
				poly->xv[0] =  schxsize/2 - framewid - xlogobox;
				poly->yv[0] = -schysize/2 + framewid;
				poly->xv[1] =  schxsize/2 - framewid;
				poly->yv[1] = -schysize/2 + framewid;
			} else
			{
				poly->xv[0] =  schxsize/2 - framewid;
				poly->yv[0] = -schysize/2 + framewid + ylogobox;
				poly->xv[1] =  schxsize/2 - framewid;
				poly->yv[1] = -schysize/2 + framewid;
			}
		} else
		{
			if (boxnum == 0)
			{
				poly->xv[0] = -schxsize/2;   poly->yv[0] = -schysize/2;
				poly->xv[1] =  schxsize/2;   poly->yv[1] =  schysize/2;
			} else
			{
				poly->xv[0] = -schxsize/2 + framewid;
				poly->yv[0] = -schysize/2 + framewid;
				poly->xv[1] =  schxsize/2 - framewid;
				poly->yv[1] =  schysize/2 - framewid;
			}
		}
		poly->count = 2;
		poly->style = CLOSEDRECT;
	} else if (boxnum < xsections*2)
	{
		/* tick marks along top and bottom sides */
		pindex = (boxnum-2) % (xsections-1) + 1;
		poly->xv[0] = poly->xv[1] = pindex * xsecsize - (schxsize/2 - framewid);
		if (boxnum <= xsections)
		{
			poly->yv[0] = schysize/2 - framewid;
			poly->yv[1] = schysize/2 - framewid/2;
		} else
		{
			poly->yv[0] = -schysize/2 + framewid;
			poly->yv[1] = -schysize/2 + framewid/2;
		}
		poly->count = 2;
		poly->style = OPENED;
	} else if (boxnum < xsections*2+ysections*2-2)
	{
		/* tick marks along left and right sides */
		pindex = (boxnum-2-(xsections-1)*2) % (ysections-1) + 1;
		poly->yv[0] = poly->yv[1] = pindex * ysecsize - (schysize/2 - framewid);
		if (boxnum <= (xsections-1)*2+ysections)
		{
			poly->xv[0] = schxsize/2 - framewid;
			poly->xv[1] = schxsize/2 - framewid/2;
		} else
		{
			poly->xv[0] = -schxsize/2 + framewid;
			poly->xv[1] = -schxsize/2 + framewid/2;
		}
		poly->count = 2;
		poly->style = OPENED;
	} else if (boxnum < xsections*2+ysections*2-2+xsections*2)
	{
		/* section numbers along top and bottom */
		pindex = (boxnum-2-(xsections-1)*2-(ysections-1)*2) % xsections;
		poly->xv[0] = poly->xv[1] = pindex * xsecsize - (schxsize/2 - framewid);
		poly->xv[2] = poly->xv[3] = poly->xv[0] + xsecsize;
		if (boxnum <= (xsections-1)*2+(ysections-1)*2+xsections+1)
		{
			poly->yv[0] = poly->yv[3] = schysize/2 - framewid;
			poly->yv[1] = poly->yv[2] = schysize/2;
		} else
		{
			poly->yv[0] = poly->yv[3] = -schysize/2;
			poly->yv[1] = poly->yv[2] = -schysize/2 + framewid;
		}
		poly->count = 4;
		poly->style = TEXTBOX;
		TDCLEAR(poly->textdescript);
		TDSETSIZE(poly->textdescript, TXTSETQLAMBDA(PRINTFRAMEQLAMBDA));
		poly->tech = el_curtech;
		(void)esnprintf(line, 10, x_("%ld"), xsections-pindex);
		poly->string = line;
	} else if (boxnum < xsections*2+ysections*2-2+xsections*2+ysections*2)
	{
		/* section numbers along left and right */
		pindex = (boxnum-2-(xsections-1)*2-(ysections-1)*2-xsections*2) % ysections;
		poly->yv[0] = poly->yv[1] = pindex * ysecsize - (schysize/2 - framewid);
		poly->yv[2] = poly->yv[3] = poly->yv[0] + ysecsize;
		if (boxnum <= (xsections-1)*2+(ysections-1)*2+xsections*2+ysections+1)
		{
			poly->xv[0] = poly->xv[3] = schxsize/2 - framewid;
			poly->xv[1] = poly->xv[2] = schxsize/2;
		} else
		{
			poly->xv[0] = poly->xv[3] = -schxsize/2;
			poly->xv[1] = poly->xv[2] = -schxsize/2 + framewid;
		}
		poly->count = 4;
		poly->style = TEXTBOX;
		TDCLEAR(poly->textdescript);
		TDSETSIZE(poly->textdescript, TXTSETQLAMBDA(PRINTFRAMEQLAMBDA));
		poly->tech = el_curtech;
		line[0] = (CHAR)('A' + pindex);
		line[1] = 0;
		poly->string = line;
	} else
	{
		switch (boxnum-titleoffset-2)
		{
			case 0:			/* frame around cell name/version */
				poly->xv[0] =  schxsize/2 - framewid - xlogobox;
				poly->yv[0] = -schysize/2 + framewid + ylogobox / 3;
				poly->xv[1] =  schxsize/2 - framewid;
				poly->yv[1] = -schysize/2 + framewid + ylogobox / 2;
				poly->count = 2;
				poly->style = CLOSEDRECT;
				break;
			case 1:			/* cell name/version */
				if (np == NONODEPROTO) break;
				(void)esnprintf(ndstring, 320, _("Name: %s"), describenodeproto(np));
				poly->xv[0] = poly->xv[3] = schxsize/2 - framewid - xlogobox;
				poly->xv[1] = poly->xv[2] = schxsize/2 - framewid;
				poly->yv[0] = poly->yv[1] = -schysize/2 + framewid + ylogobox;
				poly->yv[2] = poly->yv[3] = poly->yv[0] + ylogobox / 4;
				poly->count = 4;
				poly->style = TEXTBOX;
				TDCLEAR(poly->textdescript);
				TDSETSIZE(poly->textdescript, TXTSETQLAMBDA(PRINTFRAMEQLAMBDA));
				poly->tech = el_curtech;
				poly->string = ndstring;
				break;
			case 2:			/* frame around designer name */
				poly->xv[0] =  schxsize/2 - framewid - xlogobox;
				poly->yv[0] = -schysize/2 + framewid + ylogobox / 2;
				poly->xv[1] =  schxsize/2 - framewid;
				poly->yv[1] = -schysize/2 + framewid + ylogobox / 4 * 3;
				poly->count = 2;
				poly->style = CLOSEDRECT;
				break;
			case 3:			/* designer name */
				var = getval((INTBIG)np->lib, VLIBRARY, VSTRING, x_("USER_drawing_designer_name"));
				if (var == NOVARIABLE)
					var = getval((INTBIG)us_tool, VTOOL, VSTRING, x_("USER_drawing_designer_name"));
				if (var == NOVARIABLE) break;
				poly->xv[0] = poly->xv[3] = schxsize/2 - framewid - xlogobox;
				poly->xv[1] = poly->xv[2] = schxsize/2 - framewid;
				poly->yv[0] = poly->yv[1] = -schysize/2 + framewid + ylogobox / 2;
				poly->yv[2] = poly->yv[3] = poly->yv[0] + ylogobox / 4;
				poly->count = 4;
				poly->style = TEXTBOX;
				TDCLEAR(poly->textdescript);
				TDSETSIZE(poly->textdescript, TXTSETQLAMBDA(PRINTFRAMEQLAMBDA));
				poly->tech = el_curtech;
				poly->string = (CHAR *)var->addr;
				break;
			case 4:			/* frame around creation date */
				poly->xv[0] =  schxsize/2 - framewid - xlogobox;
				poly->yv[0] = -schysize/2 + framewid + ylogobox / 6;
				poly->xv[1] =  schxsize/2 - framewid;
				poly->yv[1] = -schysize/2 + framewid + ylogobox / 3;
				poly->count = 2;
				poly->style = CLOSEDRECT;
				break;
			case 5:			/* creation date */
				if (np == NONODEPROTO) break;
				(void)esnprintf(ndstring, 320, _("Created: %s"), timetostring((time_t)np->creationdate));
				poly->xv[0] = poly->xv[3] = schxsize/2 - framewid - xlogobox;
				poly->xv[1] = poly->xv[2] = schxsize/2 - framewid;
				poly->yv[0] = poly->yv[1] = -schysize/2 + framewid + ylogobox / 6;
				poly->yv[2] = poly->yv[3] = poly->yv[0] + ylogobox / 6;
				poly->count = 4;
				poly->style = TEXTBOX;
				TDCLEAR(poly->textdescript);
				TDSETSIZE(poly->textdescript, TXTSETQLAMBDA(PRINTFRAMEQLAMBDA));
				poly->tech = el_curtech;
				poly->string = ndstring;
				break;
			case 6:			/* frame around revision date */
				poly->xv[0] =  schxsize/2 - framewid - xlogobox;
				poly->yv[0] = -schysize/2 + framewid;
				poly->xv[1] =  schxsize/2 - framewid;
				poly->yv[1] = -schysize/2 + framewid + ylogobox / 6;
				poly->count = 2;
				poly->style = CLOSEDRECT;
				break;
			case 7:			/* revision date */
				if (np == NONODEPROTO) break;
				(void)esnprintf(ndstring, 320, _("Revised: %s"), timetostring((time_t)np->revisiondate));
				poly->xv[0] = poly->xv[3] = schxsize/2 - framewid - xlogobox;
				poly->xv[1] = poly->xv[2] = schxsize/2 - framewid;
				poly->yv[0] = poly->yv[1] = -schysize/2 + framewid;
				poly->yv[2] = poly->yv[3] = poly->yv[0] + ylogobox / 6;
				poly->count = 4;
				poly->style = TEXTBOX;
				TDCLEAR(poly->textdescript);
				TDSETSIZE(poly->textdescript, TXTSETQLAMBDA(PRINTFRAMEQLAMBDA));
				poly->tech = el_curtech;
				poly->string = ndstring;
				break;
			case 8:			/* frame around project name */
				poly->xv[0] =  schxsize/2 - framewid - xlogobox;
				poly->yv[0] = -schysize/2 + framewid + ylogobox / 4 * 3;
				poly->xv[1] =  schxsize/2 - framewid;
				poly->yv[1] = -schysize/2 + framewid + ylogobox;
				poly->count = 2;
				poly->style = CLOSEDRECT;
				break;
			case 9:			/* project name */
				var = getval((INTBIG)np->lib, VLIBRARY, VSTRING, x_("USER_drawing_project_name"));
				if (var == NOVARIABLE)
					var = getval((INTBIG)us_tool, VTOOL, VSTRING, x_("USER_drawing_project_name"));
				if (var == NOVARIABLE) break;
				poly->xv[0] = poly->xv[3] = schxsize/2 - framewid - xlogobox;
				poly->xv[1] = poly->xv[2] = schxsize/2 - framewid;
				poly->yv[0] = poly->yv[1] = -schysize/2 + framewid + ylogobox / 4 * 3;
				poly->yv[2] = poly->yv[3] = poly->yv[0] + ylogobox / 4;
				poly->count = 4;
				poly->style = TEXTBOX;
				TDCLEAR(poly->textdescript);
				TDSETSIZE(poly->textdescript, TXTSETQLAMBDA(PRINTFRAMEQLAMBDA));
				poly->tech = el_curtech;
				poly->string = (CHAR *)var->addr;
				break;
			case 10:		/* frame around company name */
				poly->xv[0] =  schxsize/2 - framewid - xlogobox;
				poly->yv[0] = -schysize/2 + framewid + ylogobox;
				poly->xv[1] =  schxsize/2 - framewid;
				poly->yv[1] = -schysize/2 + framewid + ylogobox / 4 * 5;
				poly->count = 2;
				poly->style = CLOSEDRECT;
				break;
			case 11:		/* company name */
				var = getval((INTBIG)np->lib, VLIBRARY, VSTRING, x_("USER_drawing_company_name"));
				if (var == NOVARIABLE)
					var = getval((INTBIG)us_tool, VTOOL, VSTRING, x_("USER_drawing_company_name"));
				if (var == NOVARIABLE) break;
				poly->xv[0] = poly->xv[3] = schxsize/2 - framewid - xlogobox;
				poly->xv[1] = poly->xv[2] = schxsize/2 - framewid;
				poly->yv[0] = poly->yv[1] = -schysize/2 + framewid + ylogobox / 3;
				poly->yv[2] = poly->yv[3] = poly->yv[0] + ylogobox / 6;
				poly->count = 4;
				poly->style = TEXTBOX;
				TDCLEAR(poly->textdescript);
				TDSETSIZE(poly->textdescript, TXTSETQLAMBDA(PRINTFRAMEQLAMBDA));
				poly->tech = el_curtech;
				poly->string = (CHAR *)var->addr;
				break;
		}
	}
	poly->layer = -1;
	poly->desc = &tech_frame;
}

CHAR *db_getframedesc(NODEPROTO *np)
{
	static CHAR line[10];
	REGISTER VARIABLE *var;
	REGISTER CHAR *pt;

	(void)estrcpy(line, x_(""));
	if (np != NONODEPROTO)
	{
		var = getvalkey((INTBIG)np, VNODEPROTO, VSTRING, el_schematic_page_size_key);
		if (var != NOVARIABLE)
		{
			(void)estrncpy(line, (CHAR *)var->addr, 9);
			for(pt = line; *pt != 0; pt++) if (isupper(*pt)) *pt = tolower(*pt);
		}
	}
	return(line);
}

/************************* COPYING AND REPLACING *************************/

/*
 * routine to copy nodeproto "fromnt" to the library "tolib" with the nodeproto
 * name "toname".  If "useexisting" is TRUE, then a cross-library copy that finds
 * instances in the destination library which are the same as in the source library
 * will use those existing instances instead of making them into cross-library references.
 * Returns address of new nodeproto copy if sucessful, NONODEPROTO if not.
 */
NODEPROTO *copynodeproto(NODEPROTO *fromnp, LIBRARY *tolib, CHAR *toname, BOOLEAN useexisting)
{
	NODEINST *ono[2];
	PORTPROTO *opt[2];
	REGISTER NODEINST *ni, *toni;
	REGISTER ARCINST *ai, *toai;
	REGISTER NODEPROTO *np, *lnt;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER BOOLEAN maysubstitute;
	REGISTER INTBIG i, failures, res;
	REGISTER PORTPROTO *pp, *ppt, *p1, *p2;
	REGISTER CHAR *ptr, *cellname;
	REGISTER LIBRARY *destlib;
	INTBIG lx, hx, ly, hy;
	REGISTER void *infstr;

	/* check for validity */
	if (fromnp == NONODEPROTO) return((NODEPROTO *)db_error(DBBADCELL|DBCOPYNODEPROTO));
	if (tolib == NOLIBRARY) return((NODEPROTO *)db_error(DBBADLIB|DBCOPYNODEPROTO));
	if (fromnp->primindex != 0) return((NODEPROTO *)db_error(DBPRIMITIVE|DBCOPYNODEPROTO));

	/* make sure name of new cell is valid */
	for(ptr = toname; *ptr != 0; ptr++)
		if (*ptr <= ' ' || *ptr == ':' || *ptr >= 0177)
			return((NODEPROTO *)db_error(DBBADNAME|DBCOPYNODEPROTO));

	/* determine whether this copy is to a different library */
	if (tolib == fromnp->lib) destlib = NOLIBRARY; else destlib = tolib;

	/* mark the proper prototype to use for each node */
	for(ni = fromnp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		ni->temp1 = (INTBIG)ni->proto;

	/* if doing a cross-library copy and can use existing ones from new library, do it */
	if (destlib != NOLIBRARY)
	{
		/* scan all subcells to see if they are found in the new library */
		for(ni = fromnp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if (ni->proto->primindex != 0) continue;

			/* keep cross-library references */
			if (ni->proto->lib != fromnp->lib) continue;

			maysubstitute = useexisting;
			if (!maysubstitute)
			{
				/* force substitution for documentation icons */
				if (ni->proto->cellview == el_iconview)
				{
					if (isiconof(ni->proto, fromnp)) maysubstitute = TRUE;
				}
			}
			if (!maysubstitute) continue;

			/* search for cell with same name and view in new library */
			for(lnt = tolib->firstnodeproto; lnt != NONODEPROTO; lnt = lnt->nextnodeproto)
				if (namesame(lnt->protoname, ni->proto->protoname) == 0 &&
					lnt->cellview == ni->proto->cellview) break;
			if (lnt == NONODEPROTO) continue;

			/* make sure all used ports can be found on the uncopied cell */
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			{
				pp = pi->proto;
				ppt = getportproto(lnt, pp->protoname);
				if (ppt != NOPORTPROTO)
				{
					/* the connections must match, too */
					if (pp->connects != ppt->connects) ppt = NOPORTPROTO;
				}
				if (ppt == NOPORTPROTO)
				{
					ttyputerr(_("Cannot use subcell %s in library %s: exports don't match"),
						nldescribenodeproto(lnt), destlib->libname);
					break;
				}
			}
			if (pi != NOPORTARCINST) continue;
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
			{
				pp = pe->proto;
				ppt = getportproto(lnt, pp->protoname);
				if (ppt != NOPORTPROTO)
				{
					/* the connections must match, too */
					if (pp->connects != ppt->connects) ppt = NOPORTPROTO;
				}
				if (ppt == NOPORTPROTO)
				{
					ttyputerr(_("Cannot use subcell %s in library %s: exports don't match"),
						nldescribenodeproto(lnt), destlib->libname);
					break;
				}
			}

			/* match found: use the prototype from the destination library */
			ni->temp1 = (INTBIG)lnt;
		}
	}

	/* create the nodeproto */
	if (toname[estrlen(toname)-1] == '}' || fromnp->cellview->sviewname[0] == 0)
		cellname = toname; else
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, toname);
		addtoinfstr(infstr, '{');
		addstringtoinfstr(infstr, fromnp->cellview->sviewname);
		addtoinfstr(infstr, '}');
		cellname = returninfstr(infstr);
	}
	np = newnodeproto(cellname, tolib);
	if (np == NONODEPROTO) return(NONODEPROTO);
	np->userbits = fromnp->userbits;

	/* zero the count of variables that failed to copy */
	failures = 0;

	/* copy nodes */
	for(ni = fromnp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* create the new nodeinst */
		lnt = (NODEPROTO *)ni->temp1;
		toni = db_newnodeinst(lnt, ni->lowx, ni->highx, ni->lowy, ni->highy,
			ni->transpose, ni->rotation, np);
		if (toni == NONODEINST) return(NONODEPROTO);

		/* save the new nodeinst address in the old nodeinst */
		ni->temp1 = (UINTBIG)toni;

		/* copy miscellaneous information */
		TDCOPY(toni->textdescript, ni->textdescript);
		toni->userbits = ni->userbits;
	}

	/* now copy the variables on the nodes */
	for(ni = fromnp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		toni = (NODEINST *)ni->temp1;
		res = db_copyxlibvars((INTBIG)ni, VNODEINST, (INTBIG)toni, VNODEINST, fromnp, destlib);
		if (res < 0) return(NONODEPROTO);
		failures += res;

		/* variables may affect geometry size */
		boundobj(toni->geom, &lx, &hx, &ly, &hy);
		if (lx != toni->geom->lowx || hx != toni->geom->highx ||
			ly != toni->geom->lowy || hy != toni->geom->highy)
				updategeom(toni->geom, toni->parent);
	}

	/* copy arcs */
	for(ai = fromnp->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		/* find the nodeinst and portinst connections for this arcinst */
		for(i=0; i<2; i++)
		{
			opt[i] = NOPORTPROTO;
			ono[i] = (NODEINST *)ai->end[i].nodeinst->temp1;
			if (ono[i]->proto->primindex != 0)
			{
				/* primitives associate ports directly */
				for(pp = ai->end[i].nodeinst->proto->firstportproto,
					ppt = ono[i]->proto->firstportproto; pp != NOPORTPROTO;
						pp = pp->nextportproto, ppt = ppt->nextportproto)
							if (pp == ai->end[i].portarcinst->proto)
				{
					opt[i] = ppt;
					break;
				}
			} else
			{
				/* cells associate ports by name */
				pp = ai->end[i].portarcinst->proto;
				ppt = getportproto(ono[i]->proto, pp->protoname);
				if (ppt != NOPORTPROTO) opt[i] = ppt;
			}
			if (opt[i] == NOPORTPROTO)
				ttyputerr(_("Error: no port for %s arc on %s node"), describearcproto(ai->proto),
					describenodeproto(ai->end[i].nodeinst->proto));
		}

		/* create the arcinst */
		toai = db_newarcinst(ai->proto, ai->width, ai->userbits, ono[0],opt[0],
			ai->end[0].xpos, ai->end[0].ypos, ono[1],opt[1], ai->end[1].xpos, ai->end[1].ypos, np);
		if (toai == NOARCINST) return(NONODEPROTO);

		/* copy arcinst variables */
		res = db_copyxlibvars((INTBIG)ai, VARCINST, (INTBIG)toai, VARCINST, fromnp, destlib);
		if (res < 0) return(NONODEPROTO);
		failures += res;
		res = db_copyxlibvars((INTBIG)ai->end[0].portarcinst, VPORTARCINST,
			(INTBIG)toai->end[0].portarcinst, VPORTARCINST, fromnp, destlib);
		if (res < 0) return(NONODEPROTO);
		failures += res;
		res = db_copyxlibvars((INTBIG)ai->end[1].portarcinst, VPORTARCINST,
			(INTBIG)toai->end[1].portarcinst, VPORTARCINST, fromnp, destlib);
		if (res < 0) return(NONODEPROTO);
		failures += res;

		/* copy miscellaneous information */
		toai->userbits = ai->userbits;

		/* variables may affect geometry size */
		boundobj(toai->geom, &lx, &hx, &ly, &hy);
		if (lx != toai->geom->lowx || hx != toai->geom->highx ||
			ly != toai->geom->lowy || hy != toai->geom->highy)
				updategeom(toai->geom, toai->parent);
	}

	/* copy the portprotos */
	for(pp = fromnp->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		/* match sub-portproto in old nodeinst to sub-portproto in new one */
		ni = (NODEINST *)pp->subnodeinst->temp1;
		for(p1 = ni->proto->firstportproto, p2 = pp->subnodeinst->proto->firstportproto;
			p1 != NOPORTPROTO && p2 != NOPORTPROTO;
				p1 = p1->nextportproto, p2 = p2->nextportproto)
					if (pp->subportproto == p2) break;
		if (pp->subportproto != p2)
			ttyputerr(_("Error: no port on %s cell"), describenodeproto(pp->subnodeinst->proto));

		/* create the nodeinst portinst */
		ppt = db_newportproto(np, ni, p1, pp->protoname);
		if (ppt == NOPORTPROTO) return(NONODEPROTO);

		/* copy portproto variables */
		res = db_copyxlibvars((INTBIG)pp, VPORTPROTO, (INTBIG)ppt, VPORTPROTO, fromnp, destlib);
		if (res < 0) return(NONODEPROTO);
		failures += res;
		res = db_copyxlibvars((INTBIG)pp->subportexpinst, VPORTEXPINST,
			(INTBIG)ppt->subportexpinst, VPORTEXPINST, fromnp, destlib);
		if (res < 0) return(NONODEPROTO);
		failures += res;

		/* copy miscellaneous information */
		ppt->userbits = pp->userbits;
		TDCOPY(ppt->textdescript, pp->textdescript);
	}

	/* copy cell variables */
	res = db_copyxlibvars((INTBIG)fromnp, VNODEPROTO, (INTBIG)np, VNODEPROTO, fromnp, destlib);
	if (res < 0) return(NONODEPROTO);
	failures += res;

	/* report failures to copy variables across libraries */
	if (failures != 0)
		ttyputmsg(_("WARNING: cross-library copy of cell %s deleted %ld variables"),
			describenodeproto(fromnp), failures);

	/* reset (copy) date information */
	np->creationdate = fromnp->creationdate;
	np->revisiondate = fromnp->revisiondate;

	return(np);
}

/*
 * helper routine for "copynodeproto()" to copy the variables in the list
 * "fromfirstvar"/"fromnumvar" to "tofirstvar"/"tonumvar".  The variables are
 * originally part of cell "cell",  If "destlib" is not NOLIBRARY,
 * this copy is from a different library and, therefore, pointers to objects in one library
 * should be converted if possible, or not copied.  Returns negative on error, positive
 * to indicate the number of variables that could not be copied (because of cross-
 * library reference inablilties), and zero for complete success.
 *
 * Note that there is only a limited number of cross-library conversion functions
 * implemented, specifically those that are used elsewhere in Electric.
 * At this time, the routine can copy NODEPROTO pointers (scalar and array)
 * and it can copy NODEINST pointers (scalar only).
 */
INTBIG db_copyxlibvars(INTBIG fromaddr, INTBIG fromtype, INTBIG toaddr,
	INTBIG totype, NODEPROTO *cell, LIBRARY *destlib)
{
	REGISTER INTBIG i, j, failures;
	BOOLEAN skipit;
	REGISTER INTBIG key, addr, type, len, *newaddr;
	INTSML *numvar;
	VARIABLE **firstvar;
	REGISTER VARIABLE *var;
	REGISTER NODEPROTO *np, *onp;
	REGISTER NODEINST *ni;

	if (db_getvarptr(fromaddr, fromtype, &firstvar, &numvar)) return(-1);

	failures = 0;
	for(i=0; i<(*numvar); i++)
	{
		key = (*firstvar)[i].key;
		addr = (*firstvar)[i].addr;
		type = (*firstvar)[i].type;
		newaddr = 0;
		skipit = FALSE;
		if (destlib != NOLIBRARY)
		{
			switch (type&VTYPE)
			{
				case VNODEPROTO:
					if ((type&VISARRAY) == 0)
					{
						np = (NODEPROTO *)addr;
						if (np == NONODEPROTO) break;
						if (np->primindex != 0) break;
						for(onp = destlib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
							if (namesame(onp->protoname, np->protoname) == 0 &&
								onp->cellview == np->cellview) break;
						if (onp != NONODEPROTO)
						{
							addr = (INTBIG)onp;
							break;
						}
					} else
					{
						len = (type&VLENGTH) >> VLENGTHSH;
						if (len != 0)
						{
							newaddr = (INTBIG *)emalloc(len * SIZEOFINTBIG, el_tempcluster);
							if (newaddr == 0) return(-1);
							for(j=0; j<len; j++)
							{
								np = ((NODEPROTO **)addr)[j];
								newaddr[j] = (INTBIG)np;
								if (np == NONODEPROTO) continue;
								if (np->primindex != 0) continue;
								for(onp = destlib->firstnodeproto; onp != NONODEPROTO;
									onp = onp->nextnodeproto)
										if (namesame(onp->protoname, np->protoname) == 0 &&
											onp->cellview == np->cellview) break;
								newaddr[j] = (INTBIG)onp;
								if (onp == NONODEPROTO) failures++;
							}
							addr = (INTBIG)newaddr;
							break;
						}
					}
					skipit = TRUE;
					failures++;
					break;
				case VPORTARCINST:
				case VPORTEXPINST:
				case VPORTPROTO:
				case VARCINST:
				case VGEOM:
				case VRTNODE:
				case VLIBRARY:
					skipit = TRUE;
					failures++;
					break;
			}
		}

		/* always convert NODEINST references, regardless of destination library */
		if ((type&VTYPE) == VNODEINST)
		{
			if ((type&VISARRAY) == 0)
			{
				ni = (NODEINST *)addr;
				if (ni != NONODEINST)
				{
					if (ni->parent == cell) addr = ni->temp1;
				}
			} else
			{
				skipit = TRUE;
				failures++;
			}
		}

		if (skipit) continue;
		var = setvalkey(toaddr, totype, key, addr, type);
		if (var == NOVARIABLE) return(-1);
		TDCOPY(var->textdescript, (*firstvar)[i].textdescript);
		if (newaddr != 0) efree((CHAR *)newaddr);
	}
	return(failures);
}

/*
 * routine to replace nodeinst "ni" with one of type "np", leaving all arcs
 * intact.  The routine returns the address of the new replacing nodeinst if
 * successful, NONODEINST if the replacement cannot be done.  If "ignoreportnames"
 * is true, do not use port names when determining association between old
 * and new prototype.  If "allowmissingports" is true, allow replacement
 * to have missing ports and, therefore, delete the arcs that used to be there.
 */
NODEINST *replacenodeinst(NODEINST *ni, NODEPROTO *np, BOOLEAN ignoreportnames,
	BOOLEAN allowmissingports)
{
	REGISTER PORTPROTO *opt, **newportprotos, **newexpportprotos;
	INTBIG endx[2], endy[2], bx, by, cx, cy, lxo, hxo, lyo, hyo, lx, hx, ly, hy, psx, psy,
		arcdx, arcdy, arccount, expcount, thisend, xp, yp, other;
	REGISTER INTBIG i, j, zigzag, ang, fixedalignment, dx, dy, alignment, lambda;
	XARRAY trans;
	REGISTER VARIABLE *varold, *varnew;
	NODEINST *endnodeinst[2];
	PORTPROTO *endportproto[2];
	REGISTER PORTARCINST *pi, **newports;
	REGISTER PORTEXPINST *pe, **newexports;
	REGISTER ARCINST *ai, *newai, *newai2, *aiswap;
	REGISTER NODEPROTO *pinnp;
	REGISTER NODEINST *newno, *newni;
	static POLYGON *poly = NOPOLYGON;

	/* make sure there is a polygon */
	(void)needstaticpolygon(&poly, 4, db_cluster);

	/* check for recursion */
	if (isachildof(ni->parent, np))
		return((NODEINST *)db_error(DBRECURSIVE|DBREPLACENODEINST));

	/* set the new node size */
	fixedalignment = 0;
	if (np->primindex == 0)
	{
		/* cell replacement: determine location of replacement */
		if (ni->proto->primindex == 0)
		{
			/* replacing one cell with another */
			lx = ni->lowx - ni->proto->lowx + np->lowx;
			hx = ni->highx - ni->proto->highx + np->highx;
			ly = ni->lowy - ni->proto->lowy + np->lowy;
			hy = ni->highy - ni->proto->highy + np->highy;
		} else
		{
			/* replacing a primitive with a cell */
			lx = (ni->lowx+ni->highx)/2 - (np->highx-np->lowx)/2;
			hx = lx + np->highx-np->lowx;
			ly = (ni->lowy+ni->highy)/2 - (np->highy-np->lowy)/2;
			hy = ly + np->highy-np->lowy;
		}

		/* adjust position for cell center alignment */
		varold = getvalkey((INTBIG)ni->proto, VNODEPROTO, VINTEGER|VISARRAY, el_prototype_center_key);
		varnew = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER|VISARRAY, el_prototype_center_key);
		if (varold != NOVARIABLE && varnew != NOVARIABLE)
		{
			fixedalignment = 1;
			corneroffset(ni, ni->proto, ni->rotation, ni->transpose, &bx, &by, FALSE);
			corneroffset(NONODEINST, np, ni->rotation, ni->transpose, &cx, &cy, FALSE);
			lx = ni->lowx;   hx = lx + np->highx - np->lowx;
			ly = ni->lowy;   hy = ly + np->highy - np->lowy;
			lx += bx-cx;   hx += bx-cx;
			ly += by-cy;   hy += by-cy;
		} else
		{
			/* no cell centers: if sizes differ, make sure alignment is sensible */
			if (ni->highx-ni->lowx != np->highx-np->lowx ||
				ni->highy-ni->lowy != np->highy-np->lowy)
			{
				lambda = lambdaofcell(np);
				alignment = muldiv(us_alignment_ratio, lambda, WHOLE);
				dx = lx - us_alignvalue(lx, alignment, &other);
				lx -= dx;   hx -= dx;
				dy = ly - us_alignvalue(ly, alignment, &other);
				ly -= dy;   hy -= dy;
			}
		}
	} else
	{
		/* primitive replacement: compute proper size of new part */
		nodesizeoffset(ni, &lxo, &lyo, &hxo, &hyo);
		nodeprotosizeoffset(np, &lx, &ly, &hx, &hy, ni->parent);
		lx = ni->lowx + lxo - lx;   hx = ni->highx - hxo + hx;
		ly = ni->lowy + lyo - ly;   hy = ni->highy - hyo + hy;
	}

	/* first create the new nodeinst in place */
	newno = db_newnodeinst(np, lx, hx, ly, hy, ni->transpose, ni->rotation, ni->parent);
	if (newno == NONODEINST) return(NONODEINST);

	/* set the change cell environment */
	db_setchangecell(ni->parent);

	/* draw new node expanded if appropriate */
	if (ni->proto->primindex == 0)
	{
		/* replacing an instance: copy the expansion bit */
		newno->userbits |= ni->userbits&NEXPAND;
	} else
	{
		/* replacing a primitive: use default expansion for the cell */
		if ((np->userbits&WANTNEXPAND) != 0) newno->userbits |= NEXPAND;
	}

	/* associate the ports between these nodes */
	if (db_portassociate(ni, newno, ignoreportnames))
	{
		db_killnodeinst(newno);
		return((NODEINST *)db_error(DBNOMEM|DBREPLACENODEINST));
	}
	expcount = 0;
	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst) expcount++;
	if (expcount > 0)
	{
		newexports = (PORTEXPINST **)emalloc(expcount * (sizeof (PORTEXPINST *)), db_cluster);
		if (newexports == 0) return((NODEINST *)db_error(DBNOMEM|DBREPLACENODEINST));
		newexpportprotos = (PORTPROTO **)emalloc(expcount * (sizeof (PORTPROTO *)), db_cluster);
		if (newexpportprotos == 0) return((NODEINST *)db_error(DBNOMEM|DBREPLACENODEINST));
		expcount = 0;
		for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		{
			newexports[expcount] = pe;
			newexpportprotos[expcount] = (PORTPROTO *)pe->proto->temp1;
			expcount++;
		}
	}

	/* see if the old arcs can connect to ports */
	arcdx = arcdy = 0;
	arccount = 0;
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		/* make sure there is an association for this port */
		opt = (PORTPROTO *)pi->proto->temp1;
		if (opt == NOPORTPROTO)
		{
			if (allowmissingports) continue;
			if (db_printerrors)
				ttyputmsg(_("No port on new node corresponds to old port: %s"), pi->proto->protoname);
			db_killnodeinst(newno);
			return((NODEINST *)db_error(DBPORTMM|DBREPLACENODEINST));
		}

		/* make sure the arc can connect to this type of port */
		ai = pi->conarcinst;
		for(i = 0; opt->connects[i] != NOARCPROTO; i++)
			if (opt->connects[i] == ai->proto) break;
		if (opt->connects[i] == NOARCPROTO)
		{
			if (allowmissingports) continue;
			if (db_printerrors)
				ttyputmsg(_("%s arc on old port %s cannot connect to new port %s"),
					describearcinst(ai), pi->proto->protoname, opt->protoname);
			db_killnodeinst(newno);
			return((NODEINST *)db_error(DBPORTMM|DBREPLACENODEINST));
		}

		/* see if the arc fits in the new port */
		if (ai->end[0].portarcinst == pi) thisend = 0; else thisend = 1;
		shapeportpoly(newno, opt, poly, FALSE);
		if (!isinside(ai->end[thisend].xpos, ai->end[thisend].ypos, poly))
		{
			/* arc doesn't fit: accumulate error distance */
			portposition(newno, opt, &xp, &yp);
			arcdx += xp - ai->end[thisend].xpos;
			arcdy += yp - ai->end[thisend].ypos;
		}
		arccount++;
	}
	if (fixedalignment == 0)
	{
		if (arccount > 0)
		{
			arcdx /= arccount;   arcdy /= arccount;
			lambda = lambdaofcell(np);
			alignment = muldiv(us_alignment_ratio, lambda, WHOLE);
			arcdx = us_alignvalue(arcdx, alignment, &other);
			arcdy = us_alignvalue(arcdy, alignment, &other);
			if (arcdx != 0 || arcdy != 0)
			{
				makeangle(newno->rotation, newno->transpose, trans);
				xform(arcdx, arcdy, &arcdx, &arcdy, trans);
				modifynodeinst(newno, arcdx, arcdy, arcdx, arcdy, 0, 0);
			}
		}
	}

	/* see if the old exports are the same */
	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
	{
		/* make sure there is an association for this port */
		opt = (PORTPROTO *)pe->proto->temp1;
		if (opt == NOPORTPROTO)
		{
			if (db_printerrors)
				ttyputmsg(_("No port on new node corresponds to old port: %s"),
					pe->proto->protoname);
			db_killnodeinst(newno);
			return((NODEINST *)db_error(DBPORTMM|DBREPLACENODEINST));
		}

		/* ensure that all arcs connected at exports still connect */
		if (db_doesntconnect(opt->connects, pe->exportproto))
		{
			db_killnodeinst(newno);
			return((NODEINST *)db_error(DBPORTMM|DBREPLACENODEINST));
		}
	}

	/* preserve the information about the replacement ports */
	arccount = 0;
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst) arccount++;
	if (arccount > 0)
	{
		newports = (PORTARCINST **)emalloc(arccount * (sizeof (PORTARCINST *)), db_cluster);
		if (newports == 0) return((NODEINST *)db_error(DBNOMEM|DBREPLACENODEINST));
		newportprotos = (PORTPROTO **)emalloc(arccount * (sizeof (PORTPROTO *)), db_cluster);
		if (newportprotos == 0) return((NODEINST *)db_error(DBNOMEM|DBREPLACENODEINST));
		arccount = 0;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			newports[arccount] = pi;
			newportprotos[arccount] = (PORTPROTO *)pi->proto->temp1;
			arccount++;
		}

		/* replace the arcs */
		for(j=0; j<arccount; j++)
		{
			pi = newports[j];
			ai = pi->conarcinst;
			if ((ai->userbits&DEADA) != 0) continue;
			for(i=0; i<2; i++)
			{
				if (ai->end[i].portarcinst == pi)
				{
					endnodeinst[i] = newno;
					endportproto[i] = newportprotos[j];
					if (endportproto[i] == NOPORTPROTO)
					{
						if (!allowmissingports)
							ttyputerr(_("Cannot re-connect %s arc"), describearcinst(ai));
						break;
					}
					endx[i] = ai->end[i].xpos;   endy[i] = ai->end[i].ypos;
					shapeportpoly(newno, endportproto[i], poly, FALSE);
					if (!isinside(endx[i], endy[i], poly)) getcenter(poly, &endx[i], &endy[i]);
				} else
				{
					endnodeinst[i] = ai->end[i].nodeinst;
					endportproto[i] = ai->end[i].portarcinst->proto;
					endx[i] = ai->end[i].xpos;   endy[i] = ai->end[i].ypos;
				}
			}
			if (endportproto[0] == NOPORTPROTO || endportproto[1] == NOPORTPROTO)
			{
				if (!allowmissingports)
				{
					ttyputerr(_("Cannot re-connect %s arc"), describearcinst(ai));
				} else
				{
					db_killarcinst(ai);
				}
				continue;
			}

			/* see if a bend must be made in the wire */
			zigzag = 0;
			if ((ai->userbits&FIXANG) != 0)
			{
				if (endx[0] != endx[1] || endy[0] != endy[1])
				{
					i = figureangle(endx[0],endy[0], endx[1],endy[1]);
					ang = ((ai->userbits&AANGLE) >> AANGLESH) * 10;
					if (i%1800 != ang%1800) zigzag = 1;
				}
			}
			if (zigzag != 0)
			{
				/* make that two wires */
				cx = endx[0];   cy = endy[1];
				pinnp = getpinproto(ai->proto);
				defaultnodesize(pinnp, &psx, &psy);
				lx = cx - psx / 2;
				hx = lx + psx;
				ly = cy - psy / 2;
				hy = ly + psy;
				newni = db_newnodeinst(pinnp, lx,hx, ly,hy, 0, 0, ni->parent);
				newai = db_newarcinst(ai->proto, ai->width, ai->userbits, endnodeinst[0],
					endportproto[0], endx[0],endy[0], newni, pinnp->firstportproto, cx, cy, ni->parent);
				i = ai->userbits;
				if ((i&ISNEGATED) != 0) i &= ~ISNEGATED;
				newai2 = db_newarcinst(ai->proto, ai->width, i, newni, pinnp->firstportproto, cx, cy,
					endnodeinst[1], endportproto[1], endx[1],endy[1], ni->parent);
				if (newai == NOARCINST || newai2 == NOARCINST) return(NONODEINST);
				if (copyvars((INTBIG)ai->end[0].portarcinst, VPORTARCINST,
					(INTBIG)newai->end[0].portarcinst, VPORTARCINST, FALSE))
						return((NODEINST *)db_error(DBREPLACENODEINST|DBNOMEM));
				if (copyvars((INTBIG)ai->end[1].portarcinst, VPORTARCINST,
					(INTBIG)newai2->end[1].portarcinst, VPORTARCINST, FALSE))
						return((NODEINST *)db_error(DBREPLACENODEINST|DBNOMEM));
				if (endnodeinst[1] == ni)
				{
					aiswap = newai;   newai = newai2;   newai2 = aiswap;
				}
				if (copyvars((INTBIG)ai, VARCINST, (INTBIG)newai, VARCINST, FALSE))
					return((NODEINST *)db_error(DBREPLACENODEINST|DBNOMEM));
			} else
			{
				/* replace the arc with another arc */
				newai = db_newarcinst(ai->proto, ai->width, ai->userbits, endnodeinst[0],
					endportproto[0], endx[0],endy[0], endnodeinst[1], endportproto[1], endx[1],endy[1],
						ni->parent);
				if (newai == NOARCINST) return(NONODEINST);
				if (copyvars((INTBIG)ai, VARCINST, (INTBIG)newai, VARCINST, FALSE))
					return((NODEINST *)db_error(DBREPLACENODEINST|DBNOMEM));
				for(i=0; i<2; i++)
					if (copyvars((INTBIG)ai->end[i].portarcinst, VPORTARCINST,
						(INTBIG)newai->end[i].portarcinst, VPORTARCINST, FALSE))
							return((NODEINST *)db_error(DBREPLACENODEINST|DBNOMEM));
			}
			db_killarcinst(ai);
		}
		efree((CHAR *)newports);
		efree((CHAR *)newportprotos);
	}

	/* now replace the exports */
	if (expcount > 0)
	{
		for(j=0; j<expcount; j++)
		{
			pe = newexports[j];
			if (moveportproto(ni->parent, pe->exportproto, newno, newexpportprotos[j])) break;
		}
		efree((CHAR *)newexports);
		efree((CHAR *)newexpportprotos);
	}

	/* copy all variables on the nodeinst */
	(void)copyvars((INTBIG)ni, VNODEINST, (INTBIG)newno, VNODEINST, FALSE);
	TDCOPY(newno->textdescript, ni->textdescript);

	/* now delete the original nodeinst */
	db_killnodeinst(ni);
	return(newno);
}

/*
 * routine to associate the ports on node instances "ni1" and "ni2".
 * Each port prototype on "ni1" will have the address of the corresponding
 * port prototype on "ni2" in its "temp1" field (NOPORTPROTO if the match
 * cannot be found).  The routine returns true if there is an error.
 */
BOOLEAN db_portassociate(NODEINST *ni1, NODEINST *ni2, BOOLEAN ignoreportnames)
{
	REGISTER INTBIG i, j, total2;
	REGISTER INTBIG *xpos2, *ypos2;
	INTBIG xpos1, ypos1;
	REGISTER PORTPROTO *pp1, *pp2, *mpt;
	static POLYGON *poly1 = NOPOLYGON, *poly2 = NOPOLYGON;

	/* make sure there is a polygon */
	(void)needstaticpolygon(&poly1, 4, db_cluster);
	(void)needstaticpolygon(&poly2, 4, db_cluster);

	/* initialize */
	for(total2 = 0, pp2 = ni2->proto->firstportproto; pp2 != NOPORTPROTO; pp2 = pp2->nextportproto)
	{
		pp2->temp1 = (INTBIG)NOPORTPROTO;
		total2++;
	}
	for(pp1 = ni1->proto->firstportproto; pp1 != NOPORTPROTO; pp1 = pp1->nextportproto)
		pp1->temp1 = (INTBIG)NOPORTPROTO;

	/* create center-position arrays for the ports on node 2 */
	xpos2 = emalloc((total2 * SIZEOFINTBIG), el_tempcluster);
	if (xpos2 == 0) return(TRUE);
	ypos2 = emalloc((total2 * SIZEOFINTBIG), el_tempcluster);
	if (ypos2 == 0) return(TRUE);
	for(i = 0, pp2 = ni2->proto->firstportproto; pp2 != NOPORTPROTO; pp2 = pp2->nextportproto, i++)
	{
		shapeportpoly(ni2, pp2, poly2, FALSE);
		getcenter(poly2, &xpos2[i], &ypos2[i]);
	}

	/* associate on port name matches */
	if (!ignoreportnames)
	{
		for(pp1 = ni1->proto->firstportproto; pp1 != NOPORTPROTO; pp1 = pp1->nextportproto)
		{
			for(pp2 = ni2->proto->firstportproto; pp2 != NOPORTPROTO; pp2 = pp2->nextportproto)
				if (pp2->temp1 == -1)
			{
				/* stop if the ports have different name */
				if (namesame(pp2->protoname, pp1->protoname) != 0) continue;

				/* store the correct association of ports */
				pp1->temp1 = (INTBIG)pp2;
				pp2->temp1 = (INTBIG)pp1;
			}
		}
	}

	/* associate ports that are in the same position */
	for(pp1 = ni1->proto->firstportproto; pp1 != NOPORTPROTO; pp1 = pp1->nextportproto)
		if ((PORTPROTO *)pp1->temp1 == NOPORTPROTO)
	{
		shapeportpoly(ni1, pp1, poly1, FALSE);
		getcenter(poly1, &xpos1, &ypos1);
		for(i = 0, pp2 = ni2->proto->firstportproto; pp2 != NOPORTPROTO; pp2 = pp2->nextportproto, i++)
		{
			/* if this port is already associated, ignore it */
			if (pp2->temp1 != -1) continue;

			/* if the port centers are different, go no further */
			if (xpos2[i] != xpos1 || ypos2[i] != ypos1) continue;

			/* compare actual polygons to be sure */
			shapeportpoly(ni2, pp2, poly2, FALSE);
			if (!polysame(poly1, poly2)) continue;

			/* handle confusion if multiple ports have the same polygon */
			if ((PORTPROTO *)pp1->temp1 != NOPORTPROTO)
			{
				mpt = (PORTPROTO *)pp1->temp1;

				/* see if one of the associations has the same connectivity */
				for(j=0; mpt->connects[j] != NOARCPROTO && pp1->connects[j] != NOARCPROTO; j++)
					if (mpt->connects[j] != pp1->connects[j]) break;
				if (mpt->connects[j] == NOARCPROTO && pp1->connects[j] == NOARCPROTO) continue;
			}

			/* store the correct association of ports */
			if ((PORTPROTO *)pp1->temp1 != NOPORTPROTO)
				((PORTPROTO *)pp1->temp1)->temp1 = (INTBIG)NOPORTPROTO;
			pp1->temp1 = (INTBIG)pp2;
			pp2->temp1 = (INTBIG)pp1;
		}
	}

	/* finally, associate ports that have the same center */
	for(pp1 = ni1->proto->firstportproto; pp1 != NOPORTPROTO; pp1 = pp1->nextportproto)
		if ((PORTPROTO *)pp1->temp1 == NOPORTPROTO)
	{
		shapeportpoly(ni1, pp1, poly1, FALSE);
		getcenter(poly1, &xpos1, &ypos1);
		for(i = 0, pp2 = ni2->proto->firstportproto; pp2 != NOPORTPROTO; pp2 = pp2->nextportproto, i++)
		{
			/* if this port is already associated, ignore it */
			if (pp2->temp1 != -1) continue;

			/* if the port centers are different, go no further */
			if (xpos2[i] != xpos1 || ypos2[i] != ypos1) continue;

			/* handle confusion if multiple ports have the same polygon */
			if ((PORTPROTO *)pp1->temp1 != NOPORTPROTO)
			{
				mpt = (PORTPROTO *)pp1->temp1;

				/* see if one of the associations has the same connectivity */
				for(j=0; mpt->connects[j] != NOARCPROTO && pp1->connects[j] != NOARCPROTO; j++)
					if (mpt->connects[j] != pp1->connects[j]) break;
				if (mpt->connects[j] == NOARCPROTO && pp1->connects[j] == NOARCPROTO) continue;
			}

			/* store the correct association of ports */
			if ((PORTPROTO *)pp1->temp1 != NOPORTPROTO)
				((PORTPROTO *)pp1->temp1)->temp1 = (INTBIG)NOPORTPROTO;
			pp1->temp1 = (INTBIG)pp2;
			pp2->temp1 = (INTBIG)pp1;
		}
	}

	/* free the port center information */
	efree((CHAR *)xpos2);
	efree((CHAR *)ypos2);
	return(FALSE);
}

/*
 * helper routine to ensure that all arcs connected to port "pp" or any of
 * its export sites can connect to the list in "conn".  Returns true
 * if the connection cannot be made
 */
BOOLEAN db_doesntconnect(ARCPROTO *conn[], PORTPROTO *pp)
{
	REGISTER NODEINST *ni;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER INTBIG i;

	/* check every instance of this node */
	for(ni = pp->parent->firstinst; ni != NONODEINST; ni = ni->nextinst)
	{
		/* make sure all arcs on this port can connect */
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			if (pi->proto != pp) continue;
			for(i=0; conn[i] != NOARCPROTO; i++)
				if (conn[i] == pi->conarcinst->proto) break;
			if (conn[i] == NOARCPROTO)
			{
				if (db_printerrors)
					ttyputmsg(_("%s arc in cell %s cannot connect to port %s"),
						describearcinst(pi->conarcinst), describenodeproto(ni->parent), pp->protoname);
				return(TRUE);
			}
		}

		/* make sure all further exports are still valid */
		for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		{
			if (pe->proto != pp) continue;
			if (db_doesntconnect(conn, pe->exportproto)) return(TRUE);
		}
	}
	return(FALSE);
}

/*
 * routine to move an export to a different nodeinst in the cell.
 * The routine expects both ports to be in the same place and simply shifts
 * the arcs without re-constraining them. The portproto is currently "oldpt"
 * in cell "cell".  It is to be moved to nodeinst "newno", subportproto
 * "newsubpt".  The routine returns true if there is an error
 */
BOOLEAN moveportproto(NODEPROTO *cell, PORTPROTO *oldpt, NODEINST *newno, PORTPROTO *newsubpt)
{
	REGISTER NODEINST *oldni;
	REGISTER PORTPROTO *oldpp;

	/* error checks */
	if (oldpt == NOPORTPROTO)
	{
		db_donextchangequietly = 0;
		return(db_error(DBBADPROTO|DBMOVEPORTPROTO) != 0);
	}
	if (cell == NONODEPROTO)
	{
		db_donextchangequietly = 0;
		return(db_error(DBBADCELL|DBMOVEPORTPROTO) != 0);
	}
	if (oldpt->parent != cell)
	{
		db_donextchangequietly = 0;
		return(db_error(DBBADCELL|DBMOVEPORTPROTO) != 0);
	}
	if (newno == NONODEINST)
	{
		db_donextchangequietly = 0;
		return(db_error(DBBADINST|DBMOVEPORTPROTO) != 0);
	}
	if (newsubpt == NOPORTPROTO)
	{
		db_donextchangequietly = 0;
		return(db_error(DBBADSUBPORT|DBMOVEPORTPROTO) != 0);
	}
	if (newno->parent != oldpt->parent)
	{
		db_donextchangequietly = 0;
		return(db_error(DBBADPARENT|DBMOVEPORTPROTO) != 0);
	}
	if (newsubpt->parent != newno->proto)
	{
		db_donextchangequietly = 0;
		return(db_error(DBBADSUBPORT|DBMOVEPORTPROTO) != 0);
	}
	if (db_doesntconnect(newsubpt->connects, oldpt))
	{
		db_donextchangequietly = 0;
		return(db_error(DBPORTMM|DBMOVEPORTPROTO) != 0);
	}

	/* remember old state */
	oldni = oldpt->subnodeinst;
	oldpp = oldpt->subportproto;

	/* change the port origin */
	db_changeport(oldpt, newno, newsubpt);

	/* handle change control, constraint, and broadcast */
	if (db_donextchangequietly == 0 && !db_dochangesquietly)
	{
		/* announce the change */
		oldpt->changeaddr = (CHAR *)db_change((INTBIG)oldpt, PORTPROTOMOD, (INTBIG)oldni,
			(INTBIG)oldpp, 0, 0, 0, 0);

		/* tell constraint system about modified port */
		(*el_curconstraint->modifyportproto)(oldpt, oldni, oldpp);

		/* mark this as changed */
		db_setchangecell(oldpt->parent);
		db_forcehierarchicalanalysis(oldpt->parent);
	}
	db_donextchangequietly = 0;

	return(FALSE);
}

/*
 * routine to change the origin of complex port "pp" to subnode "newsubno",
 * subport "newsubpt"
 */
void db_changeport(PORTPROTO *pp, NODEINST *newsubno, PORTPROTO *newsubpt)
{
	REGISTER PORTEXPINST *pe;

	/* remove the old linkage */
	db_removeportexpinst(pp);

	/* create the new linkage */
	pp->subnodeinst = newsubno;
	pp->subportproto = newsubpt;
	pp->userbits = (pp->userbits & STATEBITS) |
		(newsubpt->userbits & (PORTANGLE|PORTARANGE|PORTNET|PORTISOLATED));
	pp->connects = newsubpt->connects;
	pe = allocportexpinst(pp->parent->lib->cluster);
	pe->proto = newsubpt;
	db_addportexpinst(newsubno, pe);
	pe->exportproto = pp;
	pp->subportexpinst = pe;

	/* update all port characteristics exported from this one */
	changeallports(pp);

	/* mark a change to the database */
	db_changetimestamp++;
}

/*
 * routine to recursively alter the "userbits" and "connects" fields of
 * export "pp", given that it changed or moved.  Also changes associated
 * icon and contents cells.
 */
void changeallports(PORTPROTO *pp)
{
	REGISTER NODEPROTO *np, *onp;
	REGISTER PORTPROTO *opp;

	/* look at all instances of the cell that had export motion */
	db_changeallports(pp);

	/* look at associated cells and change their ports */
	np = pp->parent;
	if (np->cellview == el_iconview)
	{
		/* changed an export on an icon: find contents and change it there */
		onp = contentsview(np);
		if (onp != NONODEPROTO)
		{
			opp = equivalentport(np, pp, onp);
			if (opp != NOPORTPROTO)
			{
				opp->userbits = (opp->userbits & ~STATEBITS) | (pp->userbits & STATEBITS);
				db_changeallports(opp);
			}
		}
		return;
	}

	/* see if there is an icon to change */
	onp = iconview(np);
	if (onp != NONODEPROTO)
	{
		opp = equivalentport(np, pp, onp);
		if (opp != NOPORTPROTO)
		{
			opp->userbits = (opp->userbits & ~STATEBITS) | (pp->userbits & STATEBITS);
			db_changeallports(opp);
		}
	}
}

/*
 * routine to recursively alter the "userbits" and "connects" fields of
 * export "pp", given that it changed or moved.
 */
void db_changeallports(PORTPROTO *pp)
{
	REGISTER NODEINST *ni;
	REGISTER PORTEXPINST *pe;

	/* look at all instances of the cell that had port motion */
	for(ni = pp->parent->firstinst; ni != NONODEINST; ni = ni->nextinst)
	{
		/* see if an instance reexports the port */
		for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		{
			if (pe->proto != pp) continue;

			/* change this port and recurse up the hierarchy */
			if (pe->exportproto->userbits != pp->userbits)
			{
				setval((INTBIG)pe->exportproto, VPORTPROTO, x_("userbits"),
					pp->userbits, VINTEGER);
			}
			pe->exportproto->connects = pp->connects;
			db_changeallports(pe->exportproto);
		}
	}
}

/*
 * routine to replace arcinst "ai" with one of type "ap".  The routine
 * returns the address of the new replaced arcinst if successful, NOARCINST
 * if the replacement cannot be done.
 */
ARCINST *replacearcinst(ARCINST *ai, ARCPROTO *ap)
{
	REGISTER PORTPROTO *pp1, *pp2;
	REGISTER INTBIG i;
	REGISTER INTBIG newwid;
	REGISTER NODEINST *ni1, *ni2;
	REGISTER ARCINST *newar;

	/* check for connection allowance */
	ni1 = ai->end[0].nodeinst;   ni2 = ai->end[1].nodeinst;
	pp1 = ai->end[0].portarcinst->proto;
	pp2 = ai->end[1].portarcinst->proto;
	for(i=0; pp1->connects[i] != NOARCPROTO; i++)
		if (pp1->connects[i] == ap) break;
	if (pp1->connects[i] == NOARCPROTO)
		return((ARCINST *)db_error(DBBADENDAC|DBREPLACEARCINST));
	for(i=0; pp2->connects[i] != NOARCPROTO; i++)
		if (pp2->connects[i] == ap) break;
	if (pp2->connects[i] == NOARCPROTO)
		return((ARCINST *)db_error(DBBADENDBC|DBREPLACEARCINST));

	/* compute the new width */
	newwid = ai->width - arcwidthoffset(ai) + arcprotowidthoffset(ap);

	/* first create the new nodeinst in place */
	newar = db_newarcinst(ap, newwid, ai->userbits, ni1,pp1, ai->end[0].xpos,ai->end[0].ypos,
		ni2,pp2,ai->end[1].xpos, ai->end[1].ypos, ai->parent);
	if (newar == NOARCINST) return(NOARCINST);

	/* copy all variables on the arcinst */
	(void)copyvars((INTBIG)ai, VARCINST, (INTBIG)newar, VARCINST, FALSE);

	/* set the change cell environment */
	db_setchangecell(ai->parent);

	/* now delete the original nodeinst */
	db_killarcinst(ai);
	return(newar);
}
