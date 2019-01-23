/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dbcreate.c
 * Database general manipulation routines
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
#include "egraphics.h"
#include "database.h"

/* prototypes for local routines */
static ARCPROTO *db_allocarcproto(CLUSTER*);
static INTBIG    db_checkshortening(NODEINST*, PORTPROTO*);
static void      db_killportproto(PORTPROTO*);

/************************* NODES *************************/

/*
 * Routine to allocate a nodeinst from memory cluster "cluster".
 * The routine returns NONODEINST if allocation fails.
 */
NODEINST *allocnodeinst(CLUSTER *cluster)
{
	REGISTER NODEINST *ni;

	ni = (NODEINST *)emalloc((sizeof (NODEINST)), cluster);
	if (ni == 0) return(NONODEINST);
	ni->lowx = ni->highx = ni->lowy = ni->highy = 0;
	ni->transpose = 0;
	ni->rotation = 0;
	ni->proto = NONODEPROTO;
	ni->parent = NONODEPROTO;
	ni->prevnodeinst = NONODEINST;
	ni->nextnodeinst = NONODEINST;
	ni->geom = NOGEOM;
	ni->previnst = NONODEINST;
	ni->nextinst = NONODEINST;
	ni->firstportarcinst = NOPORTARCINST;
	ni->firstportexpinst = NOPORTEXPINST;
	TDCLEAR(ni->textdescript);
	defaulttextsize(5, ni->textdescript);
	TDSETPOS(ni->textdescript, VTPOSBOXED);
	ni->arraysize = 0;
	ni->changeaddr = (CHAR *)NOCHANGE;
	ni->changed = 0;
	ni->userbits = ni->temp1 = ni->temp2 = 0;
	ni->firstvar = NOVARIABLE;
	ni->numvar = 0;
	return(ni);
}

/*
 * routine to return nodeinst "ni" to the pool of free nodes
 */
void freenodeinst(NODEINST *ni)
{
	if (ni == NONODEINST) return;
	if (ni->numvar != 0) db_freevars(&ni->firstvar, &ni->numvar);
	efree((CHAR *)ni);
}

void startobjectchange(INTBIG addr, INTBIG type)
{
	/* handle change control and broadcast */
	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		(void)db_change(addr, OBJECTSTART, type, 0, 0, 0, 0, 0);
	}
	db_donextchangequietly = FALSE;
}

void endobjectchange(INTBIG addr, INTBIG type)
{
	/* handle change control and broadcast */
	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		(void)db_change(addr, OBJECTEND, type, 0, 0, 0, 0, 0);
	}
	db_donextchangequietly = FALSE;
}

/*
 * create a new nodeinst of proto "typ" located at (lx-hx, ly-hy), transposed
 * if "trans" is nonzero, rotated "angle" tenth-degrees.  The nodeinst is located
 * in cell "parnt". The address of the nodeinst is returned.
 * If NONODEINST is returned, there is an error creating the nodeinst.
 */
NODEINST *newnodeinst(NODEPROTO *np, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy,
	INTBIG trans, INTBIG angle, NODEPROTO *parnt)
{
	/* make sure values are correct */
	if (trans != 0 && trans != 1)
	{
		db_donextchangequietly = FALSE;
		return((NODEINST *)db_error(DBBADTRANS|DBNEWNODEINST));
	}
	if (angle < 0 || angle >= 3600)
	{
		db_donextchangequietly = FALSE;
		return((NODEINST *)db_error(DBBADROT|DBNEWNODEINST));
	}
	if (np == NONODEPROTO)
	{
		db_donextchangequietly = FALSE;
		return((NODEINST *)db_error(DBBADPROTO|DBNEWNODEINST));
	}
	if (parnt == NONODEPROTO)
	{
		db_donextchangequietly = FALSE;
		return((NODEINST *)db_error(DBBADPARENT|DBNEWNODEINST));
	}
	if (isachildof(parnt, np))
	{
		db_donextchangequietly = FALSE;
		return((NODEINST *)db_error(DBRECURSIVE|DBNEWNODEINST));
	}

	/* build the nodeinst */
	return(db_newnodeinst(np, lx, hx, ly, hy, trans, angle, parnt));
}

/*
 * internal procedure to create a new nodeinst of proto "np" located at
 * (lx-hx, ly-hy), transposed if "trans" is nonzero, rotated "angle" tenth-degrees.
 * The nodeinst is located in cell "parnt". The address of the nodeinst is
 * returned.  NONODEINST is returned upon error.
 */
NODEINST *db_newnodeinst(NODEPROTO *np, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy,
	INTBIG trans, INTBIG angle, NODEPROTO *parnt)
{
	REGISTER NODEINST *ni;

	/* allocate the nodeinst */
	ni = allocnodeinst(parnt->lib->cluster);
	if (ni == NONODEINST)
	{
		db_donextchangequietly = FALSE;
		return(NONODEINST);
	}

	/* initialize the nodeinst */
	ni->proto = np;
	ni->rotation = (INTSML)angle;   ni->transpose = (INTSML)trans;
	ni->lowx = lx;       ni->highx = hx;
	ni->lowy = ly;       ni->highy = hy;
	ni->parent = parnt;

	/* create its ports */
	ni->firstportarcinst = NOPORTARCINST;
	ni->firstportexpinst = NOPORTEXPINST;

	/* make a geometry module for this nodeinst */
	ni->geom = allocgeom(parnt->lib->cluster);
	if (ni->geom == NOGEOM) return(NONODEINST);
	ni->geom->entryisnode = TRUE;   ni->geom->entryaddr.ni = ni;

	/* put it in appropriate lists */
	db_enternodeinst(ni);

	/* handle change control, constraint, and broadcast */
	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		/* set the change cell environment */
		db_setchangecell(parnt);

		/* tell all tools about this nodeinst */
		ni->changeaddr = (CHAR *)db_change((INTBIG)ni, NODEINSTNEW, 0, 0, 0, 0, 0, 0);

		/* tell constraint system about new node */
		(*el_curconstraint->newobject)((INTBIG)ni, VNODEINST);
	}
	db_donextchangequietly = FALSE;

	/* report nodeinst address */
	return(ni);
}

/*
 * routine to enter nodeinst "ni" in the database structure by linking it into
 * the list of nodes of this proto, putting it in the geometry list,
 * and announcing the existance of the nodeinst.
 */
void db_enternodeinst(NODEINST *ni)
{
	REGISTER NODEPROTO *np, *par;

	/* link the geometry entry for the nodeinst */
	par = ni->parent;
	linkgeom(ni->geom, par);

	/* adjust parent size if this node extends over edge */

	/* put in list of nodes of this proto */
	np = ni->proto;
	if (np->firstinst != NONODEINST) np->firstinst->previnst = ni;
	ni->nextinst = np->firstinst;
	ni->previnst = NONODEINST;
	np->firstinst = ni;

	/* put in list of nodes in this cell */
	if (par->firstnodeinst != NONODEINST) par->firstnodeinst->prevnodeinst = ni;
	ni->nextnodeinst = par->firstnodeinst;
	ni->prevnodeinst = NONODEINST;
	par->firstnodeinst = ni;

	/* mark the nodeinst alive */
	ni->userbits &= ~DEADN;

	/* mark a change to the database */
	db_changetimestamp++;
}

/*
 * free up network nodeinst pointed to by "ni".
 * Returns true if the nodeinst has arcs still connected to it (an error)
 */
BOOLEAN killnodeinst(NODEINST *ni)
{
	/* error if nodeinst is not valid */
	if (ni == NONODEINST)
	{
		db_donextchangequietly = FALSE;
		return(db_error(DBBADINST|DBKILLNODEINST) != 0);
	}

	/* error if any arcs still connected to this nodeinst */
	if (ni->firstportarcinst != NOPORTARCINST)
	{
		db_donextchangequietly = FALSE;
		return(db_error(DBHASARCS|DBKILLNODEINST) != 0);
	}

	/* error if any exports */
	if (ni->firstportexpinst != NOPORTEXPINST)
	{
		db_donextchangequietly = FALSE;
		return(db_error(DBHASPORTS|DBKILLNODEINST) != 0);
	}

	/* remove nodeinst from lists */
	db_killnodeinst(ni);

	/* freeing of the nodeinst and ports will be done later */
	return(FALSE);
}

/*
 * routine to remove nodeinst "ni" from the database structure by un-linking
 * it from the list of nodes of this proto, removing it from the geometry
 * list, and announcing the deletion of the nodeinst.
 */
void db_killnodeinst(NODEINST *ni)
{
	db_retractnodeinst(ni);

	/* handle change control, constraint, and broadcast */
	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		/* set the change cell environment */
		db_setchangecell(ni->parent);

		/* tell all tools about this nodeinst */
		ni->changeaddr = (CHAR *)db_change((INTBIG)ni, NODEINSTKILL, 0, 0, 0, 0, 0, 0);

		/* tell constraint system about killed node */
		(*el_curconstraint->killobject)((INTBIG)ni, VNODEINST);
	} else
	{
		/* must deallocate now: no change control */
		freegeom(ni->geom);
		freenodeinst(ni);
	}
	db_donextchangequietly = FALSE;
}

/*
 * routine to remove nodeinst "ni" from the database structure by un-linking
 * it from the list of nodes of this proto and removing it from the geometry list.
 */
void db_retractnodeinst(NODEINST *ni)
{
	/* remove from list of nodes of this kind */
	if (ni->nextinst != NONODEINST) ni->nextinst->previnst = ni->previnst;
	if (ni->previnst != NONODEINST) ni->previnst->nextinst = ni->nextinst; else
		ni->proto->firstinst = ni->nextinst;

	/* remove from list of nodes in this cell */
	if (ni->nextnodeinst != NONODEINST) ni->nextnodeinst->prevnodeinst = ni->prevnodeinst;
	if (ni->prevnodeinst != NONODEINST) ni->prevnodeinst->nextnodeinst = ni->nextnodeinst; else
		ni->parent->firstnodeinst = ni->nextnodeinst;

	/* remove from R-tree */
	undogeom(ni->geom, ni->parent);

	/* mark the nodeinst dead */
	ni->userbits |= DEADN;

	/* mark a change to the database */
	db_changetimestamp++;
}

/*
 * Routine to determine the default size of primitive node "np" and return it in
 * (xs, ys).
 */
void defaultnodesize(NODEPROTO *np, INTBIG *xs, INTBIG *ys)
{
	REGISTER VARIABLE *var;

	/* take default size from the prototype */
	*xs = np->highx - np->lowx;
	*ys = np->highy - np->lowy;

	/* cells always use this */
	if (np->primindex == 0) return;

	/* see if there is an override on the node */
	var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER|VISARRAY, el_node_size_default_key);
	if (var != NOVARIABLE)
	{
		*xs = ((INTBIG *)var->addr)[0] * el_curlib->lambda[np->tech->techindex] / WHOLE;
		*ys = ((INTBIG *)var->addr)[1] * el_curlib->lambda[np->tech->techindex] / WHOLE;
	}
}

/*
 * routine to modify nodeinst "ni" by "deltalx" in low X, "deltaly" in low Y,
 * "deltahx" in high X, "deltahy" in high Y, "deltarot" in rotation, and
 * "deltatrans" in transgeometry.
 */
void modifynodeinst(NODEINST *ni, INTBIG deltalx, INTBIG deltaly, INTBIG deltahx,
	INTBIG deltahy, INTBIG deltarot, INTBIG deltatrans)
{
	/* examine the nature of the changes */
	deltarot = deltarot % 3600;
	if (deltarot < 0) deltarot += 3600;
	if (deltatrans != 0) deltatrans = 1;
	if (deltalx == 0 && deltahx == 0 && deltaly == 0 && deltahy == 0 && deltarot == 0 &&
		deltatrans == 0)
	{
		db_donextchangequietly = FALSE;
		return;
	}

	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		/* tell constraint system about modified node */
		(*el_curconstraint->modifynodeinst)(ni, deltalx, deltaly, deltahx, deltahy, deltarot, deltatrans);
	} else
	{
		/* make the change now: no constraint system */
		ni->lowx += deltalx;   ni->highx += deltahx;
		ni->lowy += deltaly;   ni->highy += deltahy;
		ni->rotation += (INTSML)deltarot;
		ni->transpose += (INTSML)deltatrans;
	}
	db_donextchangequietly = FALSE;

	/* set the change cell environment */
	db_setchangecell(ni->parent);

	/* mark a change to the database */
	db_changetimestamp++;
}

/*
 * routine to modify "count" nodeinsts "ni" by "deltalx" in low X, "deltaly" in low Y,
 * "deltahx" in high X, "deltahy" in high Y, "deltarot" in rotation, and
 * "deltatrans" in transgeometry.
 */
void modifynodeinsts(INTBIG count, NODEINST **nis, INTBIG *deltalxs, INTBIG *deltalys,
	INTBIG *deltahxs, INTBIG *deltahys, INTBIG *deltarots, INTBIG *deltatranss)
{
	REGISTER INTBIG i, deltalx, deltaly, deltahx, deltahy;
	REGISTER INTBIG deltarot, deltatrans;
	REGISTER NODEINST *ni;

	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		/* tell constraint system about modified node */
		(*el_curconstraint->modifynodeinsts)(count, nis, deltalxs, deltalys, deltahxs, deltahys, deltarots, deltatranss);
		for(i=0; i<count; i++)
			db_setchangecell(nis[i]->parent);
	} else
	{
		for(i=0; i<count; i++)
		{
			ni = nis[i];
			deltalx = deltalxs[i];
			deltaly = deltalys[i];
			deltahx = deltahxs[i];
			deltahy = deltahys[i];
			deltarot = deltarots[i];
			deltatrans = deltatranss[i];

			/* examine the nature of the changes */
			deltarot = deltarot % 3600;
			if (deltarot < 0) deltarot += 3600;
			if (deltatrans != 0) deltatrans = 1;
			if (deltalx == 0 && deltahx == 0 && deltaly == 0 && deltahy == 0 && deltarot == 0 &&
				deltatrans == 0) continue;

			/* make the change now: no constraint system */
			ni->lowx += deltalx;   ni->highx += deltahx;
			ni->lowy += deltaly;   ni->highy += deltahy;
			ni->rotation += (INTSML)deltarot;
			ni->transpose += (INTSML)deltatrans;
		}
	}

	/* turn off temp flag for quiet changes */
	db_donextchangequietly = FALSE;

	/* mark a change to the database */
	db_changetimestamp++;
}

/*
 * routine to initialize a nodeinst that it not in the
 * database (for miscellaneous use)
 */
void initdummynode(NODEINST *ni)
{
	ni->rotation = 0;
	ni->transpose = 0;
	ni->proto = el_technologies->firstnodeproto;
	ni->parent = NONODEPROTO;
	ni->firstportarcinst = NOPORTARCINST;
	ni->firstportexpinst = NOPORTEXPINST;
	ni->userbits = 0;
	TDCLEAR(ni->textdescript);
	defaulttextsize(5, ni->textdescript);
	TDSETPOS(ni->textdescript, VTPOSBOXED);
	ni->numvar = 0;
}

/************************* ARC PROTOTYPES *************************/

/*
 * routine to allocate "count" arcprotos from memory cluster "cluster"
 * (arcprotos are never freed) and place them in the array at "addresses".
 * The routine sets an element to NOARCPROTO if allocation fails.
 */
ARCPROTO *db_allocarcproto(CLUSTER *cluster)
{
	REGISTER ARCPROTO *ap;

	ap = (ARCPROTO *)emalloc((sizeof (ARCPROTO)), cluster);
	if (ap == 0) return(NOARCPROTO);
	ap->userbits = ap->temp1 = ap->temp2 = 0;
	ap->numvar = 0;
	ap->protoname = NOSTRING;
	ap->firstvar = NOVARIABLE;
	ap->tech = NOTECHNOLOGY;
	ap->nextarcproto = NOARCPROTO;
	ap->nominalwidth = 0;
	ap->arcindex = 0;
	return(ap);
}

/*
 * routine to return arcproto "ap" to the pool of free arcs
 */
void db_freearcproto(ARCPROTO *ap)
{
	if (ap == NOARCPROTO) return;
	if (ap->numvar != 0) db_freevars(&ap->firstvar, &ap->numvar);
	efree((CHAR *)ap);
}

/*
 * routine to create a new arc prototype with name "name" and default wire
 * width "nomwidth".  The arc prototype is placed in technology "tech".  The
 * index of the arc prototype is in "ind" which must be a unique number that
 * identifies this arcinst proto.  The routine returns zero if the new arc
 * prototype has successfully been created.  It returns NOARCPROTO if the arc
 * prototype cannot be created.
 */
ARCPROTO *db_newarcproto(TECHNOLOGY *tech, CHAR *name, INTBIG nomwidth, INTBIG ind)
{
	REGISTER ARCPROTO *ap;
	REGISTER ARCPROTO *lat, *tat;
	REGISTER CHAR *pp;

	/* make sure name is valid */
	for(pp = name; *pp != 0; pp++) if (*pp <= ' ' || *pp >= 0177) return(NOARCPROTO);

	ap = db_allocarcproto(tech->cluster);
	if (ap == NOARCPROTO) return(NOARCPROTO);
	for(lat = NOARCPROTO, tat = tech->firstarcproto; tat != NOARCPROTO; tat = tat->nextarcproto)
		lat = tat;
	if (lat == NOARCPROTO) tech->firstarcproto = ap; else
		lat->nextarcproto = ap;
	ap->nextarcproto = NOARCPROTO;
	if (allocstring(&ap->protoname, name, tech->cluster)) return(NOARCPROTO);
	ap->nominalwidth = nomwidth;
	ap->arcindex = ind;
	ap->tech = tech;
	return(ap);
}

/************************* ARC INSTANCES *************************/

/*
 * Routine to allocate an arcinst from memory cluster "cluster".
 * The routine returns NOARCINST if allocation fails.
 */
ARCINST *allocarcinst(CLUSTER *cluster)
{
	REGISTER ARCINST *ai;

	ai = (ARCINST *)emalloc((sizeof (ARCINST)), cluster);
	if (ai == 0) return(NOARCINST);
	ai->changed = 0;   ai->changeaddr = (CHAR *)NOCHANGE;
	ai->userbits = ai->temp1 = ai->temp2 = 0;
	ai->numvar = 0;
	ai->network = NONETWORK;
	ai->firstvar = NOVARIABLE;
	ai->nextarcinst = NOARCINST;
	ai->geom = NOGEOM;
	ai->proto = NOARCPROTO;
	ai->length = 0;
	ai->width = 0;
	ai->endshrink = 0;
	ai->end[0].xpos = ai->end[0].ypos = 0;
	ai->end[1].xpos = ai->end[1].ypos = 0;
	ai->end[0].nodeinst = ai->end[1].nodeinst = NONODEINST;
	ai->end[0].portarcinst = ai->end[1].portarcinst = NOPORTARCINST;
	ai->prevarcinst = NOARCINST;
	ai->parent = NONODEPROTO;
	return(ai);
}

/*
 * routine to return arcinst "ai" to the pool of free arc instances
 */
void freearcinst(ARCINST *ai)
{
	if (ai == NOARCINST) return;
	if (ai->numvar != 0) db_freevars(&ai->firstvar, &ai->numvar);
	efree((CHAR *)ai);
}

/*
 * Routine to allocate a portarcinst from memory cluster "cluster".
 * The routine returns NOPORTARCINST if allocation fails.
 */
PORTARCINST *allocportarcinst(CLUSTER *cluster)
{
	REGISTER PORTARCINST *pi;

	pi = (PORTARCINST *)emalloc((sizeof (PORTARCINST)), cluster);
	if (pi == 0) return(NOPORTARCINST);
	pi->firstvar = NOVARIABLE;
	pi->numvar = 0;
	pi->nextportarcinst = NOPORTARCINST;
	pi->proto = NOPORTPROTO;
	pi->conarcinst = NOARCINST;
	return(pi);
}

/*
 * routine to return portarcinst "pi" to the pool of free ports
 */
void freeportarcinst(PORTARCINST *pi)
{
	if (pi == NOPORTARCINST) return;
	if (pi->numvar != 0) db_freevars(&pi->firstvar, &pi->numvar);
	efree((CHAR *)pi);
}

/*
 * create a new arcinst of proto "typ", width "width", and initial userbits in
 * "initialbits".  One end is connected to portproto "pA" of nodeinst
 * "nA" and the other end is connected to portproto "pB" of nodeinst "nB".
 * The arcinst is located in cell "parnt".  The address of the arcinst is
 * returned (NOARCINST if an error is found).
 */
ARCINST *newarcinst(ARCPROTO *typ, INTBIG wid, INTBIG initialbits, NODEINST *nA,
	PORTPROTO *pA, INTBIG xA, INTBIG yA, NODEINST *nB, PORTPROTO *pB, INTBIG xB, INTBIG yB,
	NODEPROTO *parnt)
{
	REGISTER INTBIG i, lambda;
	REGISTER PORTPROTO *pp;
	REGISTER ARCINST *ai;
	REGISTER POLYGON *poly;

	/* check for missing specifications */
	if (typ == NOARCPROTO)
	{
		db_donextchangequietly = FALSE;
		return((ARCINST *)db_error(DBBADPROTO|DBNEWARCINST));
	}
	if (parnt == NONODEPROTO)
	{
		db_donextchangequietly = FALSE;
		return((ARCINST *)db_error(DBBADPARENT|DBNEWARCINST));
	}
	if (wid < 0)
	{
		db_donextchangequietly = FALSE;
		return((ARCINST *)db_error(DBBADWIDTH|DBNEWARCINST));
	}
	if (nA == NONODEINST || pA == NOPORTPROTO)
	{
		db_donextchangequietly = FALSE;
		return((ARCINST *)db_error(DBBADENDAN|DBNEWARCINST));
	}
	if (nB == NONODEINST || pB == NOPORTPROTO)
	{
		db_donextchangequietly = FALSE;
		return((ARCINST *)db_error(DBBADENDBN|DBNEWARCINST));
	}

	/* make sure that the node is in the cell */
	if (nA->parent != parnt)
	{
		db_donextchangequietly = FALSE;
		return((ARCINST *)db_error(DBBADENDAN|DBNEWARCINST));
	}
	if (nB->parent != parnt)
	{
		db_donextchangequietly = FALSE;
		return((ARCINST *)db_error(DBBADENDBN|DBNEWARCINST));
	}

	/* make sure that the port proto is on the node */
	for(pp = nA->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		if (pp == pA) break;
	if (pp == NOPORTPROTO)
	{
		db_donextchangequietly = FALSE;
		return((ARCINST *)db_error(DBBADENDAN|DBNEWARCINST));
	}
	for(pp = nB->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		if (pp == pB) break;
	if (pp == NOPORTPROTO)
	{
		db_donextchangequietly = FALSE;
		return((ARCINST *)db_error(DBBADENDBN|DBNEWARCINST));
	}

	/* make sure that the arc can connect to the port */
	for(i=0; pA->connects[i] != NOARCPROTO; i++)
		if (pA->connects[i] == typ) break;
	if (pA->connects[i] == NOARCPROTO)
	{
		(void)db_error(DBBADENDAC|DBNEWARCINST);
		if (db_printerrors)
			ttyputmsg(_("Port %s of node %s in cell %s cannot connect to arc %s"), pA->protoname,
				describenodeinst(nA), describenodeproto(parnt), describearcproto(typ));
		db_donextchangequietly = FALSE;
		return(NOARCINST);
	}
	for(i=0; pB->connects[i] != NOARCPROTO; i++)
		if (pB->connects[i] == typ) break;
	if (pB->connects[i] == NOARCPROTO)
	{
		(void)db_error(DBBADENDBC|DBNEWARCINST);
		if (db_printerrors)
			ttyputmsg(_("Port %s of node %s in cell %s cannot connect to arc %s"), pB->protoname,
				describenodeinst(nB), describenodeproto(parnt), describearcproto(typ));
		db_donextchangequietly = FALSE;
		return(NOARCINST);
	}

	/* check that arcinst ends in proper portinst area */
	poly = allocpolygon(4, db_cluster);
	shapeportpoly(nA, pA, poly, FALSE);
	if (!isinside(xA, yA, poly))
	{
		(void)db_error(DBBADENDAP|DBNEWARCINST);
		if (db_printerrors)
		{
			lambda = lambdaofcell(parnt);
			ttyputmsg(_("Point (%s,%s) not inside port %s of node %s in cell %s"), latoa(xA, lambda),
				latoa(yA, lambda), pA->protoname, describenodeinst(nA), describenodeproto(parnt));
		}
		db_donextchangequietly = FALSE;
		freepolygon(poly);
		return(NOARCINST);
	}
	shapeportpoly(nB, pB, poly, FALSE);
	if (!isinside(xB, yB, poly))
	{
		(void)db_error(DBBADENDBP|DBNEWARCINST);
		if (db_printerrors)
		{
			lambda = lambdaofcell(parnt);
			ttyputmsg(_("Point (%s,%s) not inside port %s of node %s in cell %s"), latoa(xB, lambda),
				latoa(yB, lambda), pB->protoname, describenodeinst(nB), describenodeproto(parnt));
		}
		db_donextchangequietly = FALSE;
		freepolygon(poly);
		return(NOARCINST);
	}
	freepolygon(poly);

	/* mark the start of a change to any cell instances that this arc connects */
	if (nA->proto->primindex == 0) startobjectchange((INTBIG)nA, VNODEINST);
	if (nB->proto->primindex == 0) startobjectchange((INTBIG)nB, VNODEINST);

	/* create the arcinst */
	ai = db_newarcinst(typ, wid, initialbits, nA,pA,xA,yA, nB,pB,xB,yB, parnt);

	/* mark the start of a change to any cell instances that this arc connects */
	if (nA->proto->primindex == 0) endobjectchange((INTBIG)nA, VNODEINST);
	if (nB->proto->primindex == 0) endobjectchange((INTBIG)nB, VNODEINST);

	return(ai);
}

/*
 * internal procedure to create a new arcinst of proto "typ" with width
 * "width".  One end is connected to portproto "pA" of nodeinst "nA" and the
 * other end is connected to portproto "pB" of nodeinst "nB".  The arcinst
 * is located in cell "parnt".  The address of the arcinst is returned.
 * NOARCINST is returned upon error.
 */
ARCINST *db_newarcinst(ARCPROTO *typ, INTBIG wid, INTBIG initialbits, NODEINST *nA,
	PORTPROTO *pA, INTBIG xA, INTBIG yA, NODEINST *nB, PORTPROTO *pB, INTBIG xB, INTBIG yB,
	NODEPROTO *parnt)
{
	REGISTER ARCINST *ai;
	REGISTER PORTARCINST *npi;

	ai = allocarcinst(parnt->lib->cluster);
	if (ai == NOARCINST)
	{
		db_donextchangequietly = FALSE;
		return(NOARCINST);
	}
	ai->proto = typ;
	ai->width = wid;
	ai->length = computedistance(xA,yA, xB,yB);
	ai->end[0].xpos = xA;   ai->end[0].ypos = yA;
	ai->end[1].xpos = xB;   ai->end[1].ypos = yB;
	ai->end[0].nodeinst = nA;
	ai->end[1].nodeinst = nB;
	ai->userbits = initialbits;
	determineangle(ai);
	ai->parent = parnt;
	ai->endshrink = 0;

	ai->end[0].portarcinst = allocportarcinst(parnt->lib->cluster);
	npi = ai->end[0].portarcinst;
	npi->proto = pA;
	db_addportarcinst(nA, npi);

	ai->end[1].portarcinst = allocportarcinst(parnt->lib->cluster);
	npi = ai->end[1].portarcinst;
	npi->proto = pB;
	db_addportarcinst(nB, npi);

	/* create a geometry module for this arcinst */
	ai->geom = allocgeom(parnt->lib->cluster);
	if (ai->geom == NOGEOM) return(NOARCINST);
	ai->geom->entryisnode = FALSE;
	ai->geom->entryaddr.ai = ai;

	/* enter arcinst in appropriate lists */
	db_enterarcinst(ai);

	/* handle change control, constraint, and broadcast */
	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		/* set the change cell environment */
		db_setchangecell(parnt);

		/* tell all tools about this arcinst */
		ai->changeaddr = (CHAR *)db_change((INTBIG)ai, ARCINSTNEW, 0, 0, 0, 0, 0, 0);

		/* tell constraint system about new arc */
		(*el_curconstraint->newobject)((INTBIG)ai, VARCINST);
	}
	db_donextchangequietly = FALSE;

	/* report arcinst address */
	return(ai);
}

/*
 * routine to enter arcinst "ai" in the database structure by including it in
 * the nodes it connects, putting it in the geometry list, and announcing
 * the existance of the arcinst.
 */
void db_enterarcinst(ARCINST *ai)
{
	REGISTER INTBIG i;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *oai;
	REGISTER PORTARCINST *pi;

	/* place connection in the nodes */
	for(i=0; i<2; i++) ai->end[i].portarcinst->conarcinst = ai;

	/* put in list of arcs in this cell */
	np = ai->parent;
	if (np->firstarcinst != NOARCINST) np->firstarcinst->prevarcinst = ai;
	ai->nextarcinst = np->firstarcinst;
	ai->prevarcinst = NOARCINST;
	np->firstarcinst = ai;
	(void)setshrinkvalue(ai, TRUE);

	/* link the geometry entry for this arcinst */
	linkgeom(ai->geom, np);

	/* special case: if connecting nodes that change size with connectivity, update their geometry */
	if ((ai->end[0].nodeinst->proto->userbits&WIPEON1OR2) != 0)
		updategeom(ai->end[0].nodeinst->geom, np);
	if ((ai->end[1].nodeinst->proto->userbits&WIPEON1OR2) != 0)
		updategeom(ai->end[1].nodeinst->geom, np);

	/* special case: update end shrinkage on connected arcs */
	for(i=0; i<2; i++)
	{
		ni = ai->end[i].nodeinst;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			oai = pi->conarcinst;
			if (oai == ai) continue;
			if (setshrinkvalue(oai, TRUE) != 0)
				updategeom(oai->geom, oai->parent);
		}
	}

	/* mark the arcinst alive */
	ai->userbits &= ~DEADA;

	/* mark a change to the database */
	db_changetimestamp++;
}

/*
 * routine to add portarcinst "npi" to the list of portarcinsts on nodeinst
 * "ni".
 */
void db_addportarcinst(NODEINST *ni, PORTARCINST *npi)
{
	REGISTER PORTPROTO *pp, *pr;
	REGISTER PORTARCINST *pi, *lpi;

	pr = npi->proto;
	pp = ni->proto->firstportproto;
	lpi = NOPORTARCINST;
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		while (pp != pi->proto && pp != pr) pp = pp->nextportproto;
		if (pp == pr)
		{
			if (lpi == NOPORTARCINST) ni->firstportarcinst = npi; else
				lpi->nextportarcinst = npi;
			npi->nextportarcinst = pi;
			return;
		}
		lpi = pi;
	}

	/* not found inside list: append to end */
	if (lpi == NOPORTARCINST) ni->firstportarcinst = npi; else
		lpi->nextportarcinst = npi;
	npi->nextportarcinst = NOPORTARCINST;
}

/*
 * routine to remove arcinst "ai" from the database.  The routine returns
 * FALSE if successful, TRUE if an error.
 */
BOOLEAN killarcinst(ARCINST *ai)
{
	REGISTER NODEINST *ni1, *ni2;

	if (ai == NOARCINST)
	{
		db_donextchangequietly = FALSE;
		return(db_error(DBBADINST|DBKILLARCINST) != 0);
	}

	/* mark the start of a change to any cell instances that this arc connects */
	ni1 = ai->end[0].nodeinst;   ni2 = ai->end[1].nodeinst;
	if (ni1->proto->primindex == 0) startobjectchange((INTBIG)ni1, VNODEINST);
	if (ni2->proto->primindex == 0) startobjectchange((INTBIG)ni2, VNODEINST);

	/* remove arcinst from appropriate lists */
	db_killarcinst(ai);

	/* mark the start of a change to any cell instances that this arc connects */
	if (ni1->proto->primindex == 0) endobjectchange((INTBIG)ni1, VNODEINST);
	if (ni2->proto->primindex == 0) endobjectchange((INTBIG)ni2, VNODEINST);

	/* freeing of the arcinst will be done later */
	return(FALSE);
}

void db_killarcinst(ARCINST *ai)
{
	db_retractarcinst(ai);

	/* handle change control, constraint, and broadcast */
	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		/* set the change cell environment */
		db_setchangecell(ai->parent);

		/* tell all tools about this arcinst */
		ai->changeaddr = (CHAR *)db_change((INTBIG)ai, ARCINSTKILL, 0, 0, 0, 0, 0, 0);

		/* tell constraint system about killed arc */
		(*el_curconstraint->killobject)((INTBIG)ai, VARCINST);
	} else
	{
		/* delete the arc now: no change control */
		freeportarcinst(ai->end[0].portarcinst);
		freeportarcinst(ai->end[1].portarcinst);
		freegeom(ai->geom);
		freearcinst(ai);
	}
	db_donextchangequietly = FALSE;
}

void db_retractarcinst(ARCINST *ai)
{
	REGISTER INTBIG i;
	REGISTER NODEINST *ni;
	REGISTER PORTARCINST *pi, *lpi;
	REGISTER ARCINST *oai;

	/* remove from list of arcs in this cell */
	if (ai->nextarcinst != NOARCINST) ai->nextarcinst->prevarcinst = ai->prevarcinst;
	if (ai->prevarcinst != NOARCINST) ai->prevarcinst->nextarcinst = ai->nextarcinst; else
		ai->parent->firstarcinst = ai->nextarcinst;

	/* remove from R-tree geometric list */
	undogeom(ai->geom, ai->parent);

	/* now update any nodes touching this arcinst */
	for(i=0; i<2; i++)
	{
		ni = ai->end[i].nodeinst;
		if (ni == NONODEINST) continue;
		if ((ni->userbits&DEADN) != 0) continue;
		lpi = NOPORTARCINST;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			if (pi->conarcinst == ai)
			{
				if (lpi == NOPORTARCINST) ni->firstportarcinst = pi->nextportarcinst; else
					lpi->nextportarcinst = pi->nextportarcinst;
			}
			lpi = pi;
		}
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			oai = pi->conarcinst;
			if (oai == ai) continue;
			if (setshrinkvalue(oai, TRUE) != 0)
				updategeom(oai->geom, oai->parent);
		}

		/* special case: if disconnecting from node that changes size with connectivity, update its geometry */
		if ((ni->proto->userbits&WIPEON1OR2) != 0)
			updategeom(ni->geom, ai->parent);
	}

	/* mark it dead */
	ai->userbits |= DEADA;

	/* mark a change to the database */
	db_changetimestamp++;
}

/*
 * Routine to return the default width of primitive arc "ap".
 */
INTBIG defaultarcwidth(ARCPROTO *ap)
{
	REGISTER INTBIG width;
	REGISTER VARIABLE *var;

	/* take default width from the prototype */
	width = ap->nominalwidth;

	/* see if there is an override on the arc */
	var = getvalkey((INTBIG)ap, VARCPROTO, VINTEGER, el_arc_width_default_key);
	if (var != NOVARIABLE)
		width = var->addr * el_curlib->lambda[ap->tech->techindex] / WHOLE;

	return(width);
}

/*
 * routine to modify arcinst "ai" by "deltawid" in width, "deltax" in both
 * ends X geometry and "deltay" in both ends Y position.  The X and Y
 * motion must not cause the connecting nodes to move.  The routine
 * returns true if an error is detected.  Null arc motion is allowed
 * to queue some other change to the arc.
 */
BOOLEAN modifyarcinst(ARCINST *ai, INTBIG deltawid, INTBIG deltax1, INTBIG deltay1,
	INTBIG deltax2, INTBIG deltay2)
{
	REGISTER INTBIG oldwid, oldlen, oldx0, oldy0, oldx1, oldy1;
	REGISTER BOOLEAN e1, e2;

	if (ai == NOARCINST)
	{
		db_donextchangequietly = FALSE;
		return(db_error(DBBADINST|DBMODIFYARCINST) != 0);
	}
	if (ai->width + deltawid < 0)
	{
		db_donextchangequietly = FALSE;
		return(db_error(DBBADNEWWID|DBMODIFYARCINST) != 0);
	}
	if (deltax1 != 0 || deltay1 != 0 || deltax2 != 0 || deltay2 != 0)
	{
		if ((ai->userbits&(FIXED|CANTSLIDE)) != 0)
		{
			db_donextchangequietly = FALSE;
			return(db_error(DBNOSLIDING|DBMODIFYARCINST) != 0);
		}

		/* temporarily set the width and check for port validity */
		ai->width += deltawid;
		e1 = db_stillinport(ai, 0, ai->end[0].xpos+deltax1, ai->end[0].ypos+deltay1);
		e2 = db_stillinport(ai, 1, ai->end[1].xpos+deltax2, ai->end[1].ypos+deltay2);
		ai->width -= deltawid;
		if (!e1 || !e2)
		{
			db_donextchangequietly = FALSE;
			return(db_error(DBNOTINPORT|DBMODIFYARCINST) != 0);
		}
	}

	/* change the arcinst */
	oldwid = ai->width;   oldlen = ai->length;
	ai->width += deltawid;
	oldx0 = ai->end[0].xpos;   ai->end[0].xpos += deltax1;
	oldy0 = ai->end[0].ypos;   ai->end[0].ypos += deltay1;
	oldx1 = ai->end[1].xpos;   ai->end[1].xpos += deltax2;
	oldy1 = ai->end[1].ypos;   ai->end[1].ypos += deltay2;
	ai->length = computedistance(ai->end[0].xpos, ai->end[0].ypos, ai->end[1].xpos, ai->end[1].ypos);
	(void)setshrinkvalue(ai, TRUE);
	updategeom(ai->geom, ai->parent);

	/* handle change control, constraint, and broadcast */
	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		/* tell all tools about this arcinst */
		ai->changeaddr = (CHAR *)db_change((INTBIG)ai, ARCINSTMOD, oldx0, oldy0, oldx1, oldy1,
			oldwid, oldlen);

		/* tell constraint system about modified arc */
		(*el_curconstraint->modifyarcinst)(ai, oldx0, oldy0, oldx1, oldy1, oldwid, oldlen);

		/* set the change cell environment */
		db_setchangecell(ai->parent);
	}
	db_donextchangequietly = FALSE;

	/* mark a change to the database */
	db_changetimestamp++;

	return(FALSE);
}

/*
 * Routine to set the "endshrink" value for arcinst "ai".  Returns
 * nonzero if the value changed.
 */
INTBIG setshrinkvalue(ARCINST *ai, BOOLEAN extend)
{
	REGISTER INTBIG i, result, ret;
	INTBIG shorter[2];
	REGISTER NODEINST *ni;
	REGISTER ARCINST *oar;
	REGISTER INTBIG newshrink;
	REGISTER PORTARCINST *pi;

	/* reset bits and compute shortening amount for each end of arcinst */
	for(i=0; i<2; i++)
	{
		if (ai->end[i].nodeinst == NONODEINST) return(0);
		ai->end[i].nodeinst->userbits &= ~NSHORT;
if (ai->end[i].portarcinst->proto == NOPORTPROTO) continue;
		shorter[i] = db_checkshortening(ai->end[i].nodeinst, ai->end[i].portarcinst->proto);
	}

	/* reset the arcinst shortening bits */
	ai->userbits &= ~ASHORT;

	/* set shortening bits if the shortening factor is nonzero */
	result = (shorter[1] << 16) | shorter[0];
	if (result != 0)
	{
		ai->userbits |= ASHORT;
		for(i=0; i<2; i++) if (shorter[i] != 0)
		{
			ni = ai->end[i].nodeinst;
			if ((ni->proto->userbits&NODESHRINK) != 0) ni->userbits |= NSHORT;
			if (!extend) continue;
			if ((ni->proto->userbits&ARCSHRINK) != 0)
			{

				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				{
					oar = pi->conarcinst;
					if (oar == ai) continue;
					oar->userbits |= ASHORT;
					if (oar->end[0].nodeinst == ni)
						newshrink = (oar->endshrink & 0xFFFF0000) | shorter[i]; else
							newshrink = (oar->endshrink & 0xFFFF) | (shorter[i] << 16);
					if (newshrink != oar->endshrink)
					{
						oar->endshrink = newshrink;
						updategeom(oar->geom, oar->parent);
					}
				}
			}
		}
	}

	/* return the shrink value for this arcinst */
	if (ai->endshrink == result) ret = 0; else ret = 1;
	ai->endshrink = result;
	return(ret);
}

/*
 * routine to determine, for the arcinst on portinst "pp" of nodeinst "ni",
 * whether the nodeinst and/or the arcs must be shortened to compensate
 * for nonmanhattan geometry and if so, by what amount.  The
 * routine returns the angle between the nonmanhattan objects.
 */
#define MAXANGLES 3
INTBIG db_checkshortening(NODEINST *ni, PORTPROTO *pp)
{
	REGISTER PORTARCINST *pi;
	REGISTER NODEPROTO *np;
	REGISTER ARCINST *ai;
	REGISTER INTBIG ang, off90, total;
	INTBIG angles[MAXANGLES];

	/* quit now if we don't have to worry about this kind of nodeinst */
	np = ni->proto;
	if (np == NONODEPROTO) return(0);
	if ((np->userbits&(NODESHRINK|ARCSHRINK)) == 0) return(0);

	/* gather the angles of the nodes/arcs */
	total = off90 = 0;
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		ai = pi->conarcinst;

		/* ignore zero-size arcs */
		if (ai->width == 0) continue;

		/* ignore this arcinst if it is not on the desired port */
		if ((np->userbits&ARCSHRINK) == 0 && pi->proto != pp) continue;

		/* compute the angle */
		ang = (ai->userbits&AANGLE) >> AANGLESH;
		if (ai->end[1].portarcinst->proto == pi->proto && ai->end[1].nodeinst == ni) ang += 180;
		ang %= 360;
		if ((ang%90) != 0) off90++;
		if (total < MAXANGLES) angles[total++] = ang; else
			break;
	}

	/* throw in the nodeinst rotation factor if it is important */
	if ((np->userbits&NODESHRINK) != 0)
	{
		ang = (pp->userbits & PORTANGLE) >> PORTANGLESH;
		ang += (ni->rotation+5) / 10;
		if (ni->transpose != 0) { ang = 270 - ang; if (ang < 0) ang += 360; }
		ang = (ang+180)%360;
		if ((ang%90) != 0) off90++;
		if (total < MAXANGLES) angles[total++] = ang;
	}

	/* all fine if all manhattan angles involved */
	if (off90 == 0) return(0);

	/* give up if too many arcinst angles */
	if (total != 2) return(0);

	/* compute and return factor */
	ang = abs(angles[1]-angles[0]);
	if (ang > 180) ang = 360 - ang;
	if (ang > 90) ang = 180 - ang;
	return(ang);
}

/*
 * routine to set the angle offset of arcinst "ai".  This value is the number
 * of degrees counter-clockwise from the right in the range of 0 to 359.
 * It is kept in the "userbits" field.
 */
void determineangle(ARCINST *ai)
{
	INTBIG ang;
	REGISTER NODEINST *ni;
	REGISTER INTBIG cx0, cy0, cx1, cy1;

	/* if length is zero, look at nodes */
	if (ai->end[0].xpos == ai->end[1].xpos && ai->end[0].ypos == ai->end[1].ypos)
	{
		ni = ai->end[0].nodeinst;
		cx0 = (ni->lowx + ni->highx) / 2;
		cy0 = (ni->lowy + ni->highy) / 2;
		ni = ai->end[1].nodeinst;
		cx1 = (ni->lowx + ni->highx) / 2;
		cy1 = (ni->lowy + ni->highy) / 2;
		if (abs(cx0-cx1) > abs(cy0-cy1))
		{
			/* horizontal nodes */
			if (cx0 > cx1) ang = 1800; else
				ang = 0;
		} else
		{
			if (cy0 > cy1) ang = 900; else
				ang = 2700;
		}
	} else
	{
		ang = figureangle(ai->end[0].xpos, ai->end[0].ypos, ai->end[1].xpos, ai->end[1].ypos);
	}
	ai->userbits = (ai->userbits & ~AANGLE) | ((ang+5)/10 << AANGLESH);
}

/*
 * routine to return the address of an arcinst that it not in the
 * database for miscellaneous use
 */
void initdummyarc(ARCINST *ai)
{
	static NODEINST node;

	ai->proto = el_technologies->firstarcproto;
	ai->length = 0;
	ai->parent = NONODEPROTO;
	ai->width = 0;
	ai->endshrink = 0;
	initdummynode(&node);
	ai->end[0].nodeinst = ai->end[1].nodeinst = &node;
	ai->end[0].portarcinst = ai->end[1].portarcinst = NOPORTARCINST;
	ai->userbits = 0;
	ai->numvar = 0;
}

/******************** PORT PROTOTYPES *************************/

/*
 * Routine to allocate a portproto from memory cluster "cluster".
 * The routine returns NOPORTPROTO if allocation fails.
 */
PORTPROTO *allocportproto(CLUSTER *cluster)
{
	REGISTER PORTPROTO *pp;

	pp = (PORTPROTO *)emalloc((sizeof (PORTPROTO)), cluster);
	if (pp == 0) return(NOPORTPROTO);
	pp->userbits = pp->temp1 = pp->temp2 = 0;
	pp->network = NONETWORK;
	pp->connects = 0;
	pp->numvar = 0;
	pp->changeaddr = (CHAR *)NOCHANGE;
	pp->protoname = (CHAR *)-1;
	pp->firstvar = NOVARIABLE;
	pp->nextportproto = NOPORTPROTO;
	pp->parent = NONODEPROTO;
	pp->subnodeinst = NONODEINST;
	pp->subportexpinst = NOPORTEXPINST;
	pp->subportproto = NOPORTPROTO;
	TDCLEAR(pp->textdescript);
	defaulttextsize(1, pp->textdescript);
	TDSETPOS(pp->textdescript, VTPOSBOXED);
	pp->cachedequivport = NOPORTPROTO;
	return(pp);
}

/*
 * routine to return portproto "pp" to the pool of free ports
 */
void freeportproto(PORTPROTO *pp)
{
	if (pp == NOPORTPROTO) return;
	if (pp->numvar != 0) db_freevars(&pp->firstvar, &pp->numvar);
	efree((CHAR *)pp);
}

/*
 * routine to allocate a portexpinst from memory cluster "cluster".
 * The routine returns NOPORTEXPINST if allocation fails.
 */
PORTEXPINST *allocportexpinst(CLUSTER *cluster)
{
	REGISTER PORTEXPINST *pe;

	pe = (PORTEXPINST *)emalloc((sizeof (PORTEXPINST)), cluster);
	if (pe == 0) return(NOPORTEXPINST);
	pe->numvar = 0;
	pe->firstvar = NOVARIABLE;
	pe->nextportexpinst = NOPORTEXPINST;
	pe->proto = NOPORTPROTO;
	pe->exportproto = NOPORTPROTO;
	return(pe);
}

/*
 * routine to return portexpinst "pe" to the pool of free ports
 */
void freeportexpinst(PORTEXPINST *pe)
{
	if (pe == NOPORTEXPINST) return;
	if (pe->numvar != 0) db_freevars(&pe->firstvar, &pe->numvar);
	efree((CHAR *)pe);
}

/*
 * routine to add a portproto to a cell.  The port proto is in cell "np".
 * The location of the portproto is based on a sub-nodeinst and portproto on
 * that sub-nodeinst: the subnodeinst is "nodeinst" and the subportproto is
 * portproto "portaddress".  The name of the portproto is in "name".  The
 * routine returns the address of the portproto if sucessful, NOPORTPROTO
 * if the portproto cannot be created.
 */
PORTPROTO *newportproto(NODEPROTO *np, NODEINST *nodeinst, PORTPROTO *portaddress,
	CHAR *name)
{
	REGISTER PORTPROTO *pp;
	REGISTER CHAR *ptr;

	/* error checks */
	if (np == NONODEPROTO)
	{
		db_donextchangequietly = FALSE;
		return((PORTPROTO *)db_error(DBBADCELL|DBNEWPORTPROTO));
	}
	if (*name == 0)
	{
		db_donextchangequietly = FALSE;
		return((PORTPROTO *)db_error(DBBADNAME|DBNEWPORTPROTO));
	}
	for(ptr = name; *ptr != 0; ptr++) if (*ptr <= ' ' || *ptr >= 0177)
	{
		db_donextchangequietly = FALSE;
		return((PORTPROTO *)db_error(DBBADNAME|DBNEWPORTPROTO));
	}

	/* check the validity of the sub-nodeinst */
	for(pp = nodeinst->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		if (pp == portaddress) break;
	if (pp == NOPORTPROTO)
	{
		db_donextchangequietly = FALSE;
		return((PORTPROTO *)db_error(DBBADSUBPORT|DBNEWPORTPROTO));
	}

	/* look for duplicate names */
	pp = getportproto(np, name);
	if (pp != NOPORTPROTO)
	{
		db_donextchangequietly = FALSE;
		return((PORTPROTO *)db_error(DBDUPLICATE|DBNEWPORTPROTO));
	}

	return(db_newportproto(np, nodeinst, portaddress, name));
}

/*
 * routine to do actual port prototype creation.  returns NOPORTPROTO
 * upon error
 */
PORTPROTO *db_newportproto(NODEPROTO *np, NODEINST *nodeinst, PORTPROTO *portaddress,
	CHAR *name)
{
	REGISTER PORTPROTO *pp;

	/* allocate a new portproto */
	pp = allocportproto(np->lib->cluster);
	if (pp == NOPORTPROTO)
	{
		db_donextchangequietly = FALSE;
		return(NOPORTPROTO);
	}

	/* load up the port prototype */
	pp->connects = portaddress->connects;
	pp->parent = np;
	pp->subnodeinst = nodeinst;
	pp->subportproto = portaddress;
	pp->userbits = portaddress->userbits;
	TDCLEAR(pp->textdescript);
	defaulttextdescript(pp->textdescript, nodeinst->geom);
	defaulttextsize(1, pp->textdescript);
	if (allocstring(&pp->protoname, name, np->lib->cluster))
	{
		db_donextchangequietly = FALSE;
		return(NOPORTPROTO);
	}

	/* link it in and announce it */
	db_enterportproto(pp);

	/* handle change control, constraint, and broadcast */
	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		/* set the change cell environment */
		db_setchangecell(np);

		/* announce it */
		pp->changeaddr = (CHAR *)db_change((INTBIG)pp, PORTPROTONEW, 0, 0, 0, 0, 0, 0);

		/* tell constraint system about new port */
		(*el_curconstraint->newobject)((INTBIG)pp, VPORTPROTO);
	}
	db_donextchangequietly = FALSE;

	return(pp);
}

void db_enterportproto(PORTPROTO *pp)
{
	REGISTER INTBIG i, j;
	REGISTER PORTPROTO *p, *last;
	REGISTER NODEPROTO *cell;
	REGISTER PORTEXPINST *pe;

	cell = pp->parent;
	for(last = NOPORTPROTO, p = cell->firstportproto; p != NOPORTPROTO; p = p->nextportproto)
		last = p;
	if (last == NOPORTPROTO) cell->firstportproto = pp; else
		last->nextportproto = pp;
	pp->nextportproto = NOPORTPROTO;

	/* load up this portexpinst description */
	pe = allocportexpinst(cell->lib->cluster);
	pe->proto = pp->subportproto;
	db_addportexpinst(pp->subnodeinst, pe);
	pe->exportproto = pp;
	pp->subportexpinst = pe;

	/* insert into hash table of portproto names */
	cell->numportprotos++;
	if (cell->numportprotos < cell->portprotohashtablesize/2)
	{
		i = db_namehash(pp->protoname) % cell->portprotohashtablesize;
		for(j=1; j<=cell->portprotohashtablesize; j += 2)
		{
			if (cell->portprotohashtable[i] == NOPORTPROTO)
			{
				cell->portprotohashtable[i] = pp;
				break;
			}
			i += j;
			if (i >= cell->portprotohashtablesize) i -= cell->portprotohashtablesize;
		}
	} else
	{
		db_buildportprotohashtable(cell);
	}

	/* special case: if port is on node that changes size with connectivity, update its geometry */
	if ((pp->subnodeinst->proto->userbits&WIPEON1OR2) != 0)
		updategeom(pp->subnodeinst->geom, cell);

	/* clear cache of port associations in this cell */
	db_clearportcache(cell);

	/* mark a change to the database */
	db_changetimestamp++;
}

void db_clearportcache(NODEPROTO *cell)
{
	REGISTER NODEPROTO *np, *onp;

	FOR_CELLGROUP(np, cell)
	{
		for(onp = np; onp != NONODEPROTO; onp = onp->prevversion)
			onp->cachedequivcell = NONODEPROTO;
	}
}

/*
 * routine to add portexpinst "npe" to the list of portexpinsts on nodeinst
 * "ni".
 */
void db_addportexpinst(NODEINST *ni, PORTEXPINST *npe)
{
	REGISTER PORTPROTO *pp, *pr;
	REGISTER PORTEXPINST *pe, *lpe;

	pr = npe->proto;
	pp = ni->proto->firstportproto;
	lpe = NOPORTEXPINST;
	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
	{
		while (pp != pe->proto && pp != pr) pp = pp->nextportproto;
		if (pp == pr)
		{
			if (lpe == NOPORTEXPINST) ni->firstportexpinst = npe; else
				lpe->nextportexpinst = npe;
			npe->nextportexpinst = pe;
			return;
		}
		lpe = pe;
	}

	/* not found inside list: append to end */
	if (lpe == NOPORTEXPINST) ni->firstportexpinst = npe; else
		lpe->nextportexpinst = npe;
	npe->nextportexpinst = NOPORTEXPINST;
}

/*
 * routine to add a portproto to a primitive nodeproto.  The nodeproto is in
 * "np" and the portproto can connect to arcs listed in the array "arcs".
 * The name of the portproto is in "name".  The routine returns the address of
 * the portproto if sucessful, NOPORTPROTO if the portproto cannot be created.
 */
PORTPROTO *db_newprimportproto(NODEPROTO *np, ARCPROTO **arcs, CHAR *name)
{
	REGISTER PORTPROTO *last;
	REGISTER PORTPROTO *pp;
	REGISTER CHAR *ptr;

	/* error checks */
	if (np == NONODEPROTO) return(NOPORTPROTO);

	/* name must not have blank space in it */
	for(ptr = name; *ptr != 0; ptr++) if (*ptr <= ' ' || *ptr >= 0177)
	{
		ttyputmsg(_("Port '%s' on primitive '%s' has bad name (spaces?)"), name, np->protoname);
		return(NOPORTPROTO);
	}

	/* must be during initialization: no instances of this nodeproto */
	if (np->firstinst != NONODEINST) return(NOPORTPROTO);

	/* reject if there is already a portproto with this name */
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		if (namesame((last=pp)->protoname, name) == 0) return(NOPORTPROTO);

	/* create the portproto and link it into the nodeinst proto */
	pp = allocportproto(np->tech->cluster);
	if (pp == NOPORTPROTO) return(NOPORTPROTO);
	if (np->firstportproto == NOPORTPROTO) np->firstportproto = pp; else
		last->nextportproto = pp;
	pp->nextportproto = NOPORTPROTO;

	/* initialize the values in the portproto */
	pp->userbits = pp->temp1 = pp->temp2 = 0;
	pp->network = NONETWORK;
	pp->parent = np;       pp->connects = arcs;
	pp->subnodeinst = NONODEINST;  pp->subportproto = NOPORTPROTO;
	if (allocstring(&pp->protoname, name, np->tech->cluster)) return(NOPORTPROTO);

	return(pp);
}

/*
 * routine to delete portproto "pp" from nodeproto "np".  Returns FALSE if
 * successful, TRUE if the portproto could not be deleted (does not
 * exist).  If the portproto has arcs connected to it in instances of the
 * cell, the arcs are deleted.  If the portproto is an export of
 * the cell, those exports are deleted (recursively).
 */
BOOLEAN killportproto(NODEPROTO *np, PORTPROTO *pp)
{
	REGISTER PORTPROTO *spt;

	/* look at all portprotos to ensure it exists */
	if (np == NONODEPROTO)
	{
		db_donextchangequietly = FALSE;
		return(db_error(DBBADCELL|DBKILLPORTPROTO) != 0);
	}
	for(spt = np->firstportproto; spt != NOPORTPROTO; spt = spt->nextportproto)
		if (spt == pp) break;
	if (spt == NOPORTPROTO)
	{
		db_donextchangequietly = FALSE;
		return(db_error(DBBADPROTO|DBKILLPORTPROTO) != 0);
	}

	/* cannot delete port on primitive nodeproto */
	if (pp->subnodeinst == NONODEINST)
	{
		db_donextchangequietly = FALSE;
		return(db_error(DBPRIMITIVE|DBKILLPORTPROTO) != 0);
	}

	db_killportproto(pp);
	return(FALSE);
}

void db_killportproto(PORTPROTO *pp)
{
	REGISTER PORTARCINST *pi, *nextpi;
	REGISTER PORTEXPINST *pe, *nextpe;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;

	/* look at all instances of nodeproto for port use */
	np = pp->parent;
	for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
	{
		/* see if this port has arcs on it on the higher-level instances */
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = nextpi)
		{
			nextpi = pi->nextportarcinst;
			if (pi->proto == pp)
			{
				if ((pi->conarcinst->userbits&DEADA) != 0) continue;
				startobjectchange((INTBIG)pi->conarcinst, VARCINST);
				db_killarcinst(pi->conarcinst);
			}
		}

		/* see if this port is an export */
		for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = nextpe)
		{
			nextpe = pe->nextportexpinst;
			if (pe->proto != pp) continue;
			db_killportproto(pe->exportproto);
			db_forcehierarchicalanalysis(np);
		}
	}

	/* remove the port prototype */
	db_retractportproto(pp);

	/* handle change control, constraint, and broadcast */
	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		/* set the change cell environment */
		db_setchangecell(np);

		/* report the change */
		pp->changeaddr = (CHAR *)db_change((INTBIG)pp, PORTPROTOKILL, 0, 0, 0, 0, 0, 0);

		/* tell constraint system about killed port */
		(*el_curconstraint->killobject)((INTBIG)pp, VPORTPROTO);
	} else
	{
		/* delete the export now: no change control */
		efree(pp->protoname);
		freeportproto(pp);
	}
	db_donextchangequietly = FALSE;
}

void db_retractportproto(PORTPROTO *pp)
{
	REGISTER PORTPROTO *ppo, *lpo;
	REGISTER NODEPROTO *np;

	/* remove portexpinst linkage from database */
	db_removeportexpinst(pp);

	/* remove the portproto from the list */
	lpo = NOPORTPROTO;
	np = pp->parent;
	for(ppo = np->firstportproto; ppo != NOPORTPROTO; ppo = ppo->nextportproto)
	{
		if (ppo == pp)
		{
			if (lpo == NOPORTPROTO) np->firstportproto = pp->nextportproto; else
				lpo->nextportproto = pp->nextportproto;
			break;
		}
		lpo = ppo;
	}

	/* rebuild hash table of cell names */
	np->numportprotos--;
	db_buildportprotohashtable(np);

	/* special case: if port is on node that changes size with connectivity, update its geometry */
	if ((pp->subnodeinst->proto->userbits&WIPEON1OR2) != 0)
		updategeom(pp->subnodeinst->geom, np);

	/* clear cache of port associations in this cell */
	db_clearportcache(pp->parent);

	/* mark a change to the database */
	db_changetimestamp++;
}

/*
 * routine to remove the portexpinst corresponding to portproto "pp" and its
 * linkage in the nodeinst
 */
void db_removeportexpinst(PORTPROTO *pp)
{
	REGISTER PORTEXPINST *pe, *lpe;

	lpe = NOPORTEXPINST;
	for(pe = pp->subnodeinst->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
	{
		if (pe->exportproto == pp)
		{
			if (lpe == NOPORTEXPINST)
				pp->subnodeinst->firstportexpinst = pe->nextportexpinst; else
					lpe->nextportexpinst = pe->nextportexpinst;
			freeportexpinst(pe);
			break;
		}
		lpe = pe;
	}
}

/*
 * Routine to rebuild the hash table of cell names in library "lib".
 */
void db_buildportprotohashtable(NODEPROTO *cell)
{
	REGISTER INTBIG i, j, numportprotos, hashtablesize;
	REGISTER PORTPROTO *pp;

	/* free the former table */
	if (cell->portprotohashtablesize > 0)
	{
		cell->portprotohashtablesize = 0;
		efree((CHAR *)cell->portprotohashtable);
	}

	/* determine the size of the hash table */
	numportprotos = 0;
	for(pp = cell->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto) numportprotos++;
	if (numportprotos != cell->numportprotos)
	{
		ttyputmsg(_("Warning: number of ports in cell %s corrected"), describenodeproto(cell));
		cell->numportprotos = numportprotos;
	}
	hashtablesize = pickprime(numportprotos * 4);

	/* create the hash table and clear it */
	cell->portprotohashtable = (PORTPROTO **)emalloc(hashtablesize * (sizeof (PORTPROTO *)),
		cell->lib->cluster);
	if (cell->portprotohashtable == 0) return;
	for(i=0; i<hashtablesize; i++)
		cell->portprotohashtable[i] = NOPORTPROTO;
	cell->portprotohashtablesize = hashtablesize;

	/* insert all cells into the table */
	for(pp = cell->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		i = db_namehash(pp->protoname) % hashtablesize;
		for(j=1; j<=hashtablesize; j += 2)
		{
			if (cell->portprotohashtable[i] == NOPORTPROTO)
			{
				cell->portprotohashtable[i] = pp;
				break;
			}
			i += j;
			if (i >= hashtablesize) i -= hashtablesize;
		}
	}
}
