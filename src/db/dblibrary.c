/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dblibrary.c
 * Database library and lambda control module
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
#include "network.h"

/* prototypes for local routines */
static void   db_scalecell(NODEPROTO*, INTBIG, TECHNOLOGY**, INTBIG*);
static void   db_validatearcinst(ARCINST*);
static void   db_setarcinst(ARCINST*, INTBIG, INTBIG, INTBIG, INTBIG);
static INTBIG db_scaleunits(INTBIG *value, INTBIG num, INTBIG den);
static void   db_scalearcinst(ARCINST *ai, INTBIG num, INTBIG denom);
static void   db_scalenodeinst(NODEINST *ni, INTBIG num, INTBIG denom);

/****************************** LIBRARIES ******************************/

/*
 * routine to allocate a library, places it in its own cluster, and return its
 * address.  The routine returns NOLIBRARY if allocation fails.
 */
LIBRARY *alloclibrary(void)
{
	REGISTER LIBRARY *lib;
	REGISTER TECHNOLOGY *tech;
	REGISTER CLUSTER *cluster;

	cluster = alloccluster(x_(""));
	if (cluster == NOCLUSTER) return((LIBRARY *)db_error(DBNOMEM|DBALLOCLIBRARY));
	lib = (LIBRARY *)emalloc((sizeof (LIBRARY)), cluster);
	if (lib == 0) return((LIBRARY *)db_error(DBNOMEM|DBALLOCLIBRARY));
	lib->cluster = cluster;

	/* allocate space for lambda array */
	lib->lambda = emalloc(((el_maxtech+1) * SIZEOFINTBIG), cluster);
	if (lib->lambda == 0) return((LIBRARY *)db_error(DBNOMEM|DBALLOCLIBRARY));
	lib->lambda[el_maxtech] = -1;

	lib->userbits = lib->temp1 = lib->temp2 = 0;
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		if (el_curlib == NOLIBRARY)
		{
			lib->lambda[tech->techindex] = tech->deflambda;
		} else
		{
			lib->lambda[tech->techindex] = el_curlib->lambda[tech->techindex];
		}
	}
	lib->numvar = 0;
	lib->libname = NOSTRING;
	lib->libfile = NOSTRING;
	lib->firstvar = NOVARIABLE;
	lib->firstnodeproto = NONODEPROTO;
	lib->tailnodeproto = NONODEPROTO;
	lib->curnodeproto = NONODEPROTO;
	lib->nextlibrary = NOLIBRARY;
	lib->numnodeprotos = 0;
	lib->nodeprotohashtablesize = 0;
	lib->freenetwork = NONETWORK;
	return(lib);
}

/*
 * routine to return library "lib" to the pool of free libraries
 */
void freelibrary(LIBRARY *lib)
{
	REGISTER CLUSTER *clus;
	REGISTER NETWORK *net, *nextnet;

	if (lib == NOLIBRARY) return;
	if (lib->numvar != 0) db_freevars(&lib->firstvar, &lib->numvar);
	efree((CHAR *)lib->lambda);
	if (lib->nodeprotohashtablesize > 0)
	{
		efree((CHAR *)lib->nodeprotohashtable);
		efree((CHAR *)lib->nodeprotoviewhashtable);
	}
	for(net = lib->freenetwork; net != NONETWORK; net = nextnet)
	{
		nextnet = net->nextnetwork;
		if (net->arctotal != 0) efree((CHAR *)net->arcaddr);
		efree((CHAR *)net);
	}
	clus = lib->cluster;
	efree((CHAR *)lib);
	freecluster(clus);
}

/*
 * routine to create a new library and return its address.  The library name
 * is "name" and its disk file is "file".  If there is any error, NOLIBRARY
 * is returned.
 */
LIBRARY *newlibrary(CHAR *name, CHAR *file)
{
	REGISTER LIBRARY *lib;
	REGISTER CHAR *ch, nc;
	REGISTER BOOLEAN renamed;

	/* error checks */
	renamed = FALSE;
	for(ch = name; *ch != 0; ch++)
	{
		nc = *ch;
		if (nc == ' ') nc = '-';
		if (nc == '\t' || nc == ':') nc = '-';
		if (nc != *ch) renamed = TRUE;
		*ch = nc;
	}
	if (renamed) ttyputerr(_("Warning: library renamed to '%s'"), name);
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		if (namesame(name, lib->libname) == 0)
			return((LIBRARY *)db_error(DBBADLIB|DBNEWLIBRARY));

	/* create the library */
	lib = alloclibrary();
	if (lib == NOLIBRARY) return((LIBRARY *)db_error(DBNOMEM|DBNEWLIBRARY));
	(void)estrcpy(lib->cluster->clustername, x_("lib:"));
	(void)estrncpy(&lib->cluster->clustername[4], name, 25);

	/* set library name and file */
	if (allocstring(&lib->libname, name, lib->cluster)) return(NOLIBRARY);
	if (allocstring(&lib->libfile, file, lib->cluster)) return(NOLIBRARY);

	/* set units */
	lib->userbits |= (((el_units & INTERNALUNITS) >> INTERNALUNITSSH) << LIBUNITSSH);

	/* link in the new library after the current library */
	if (el_curlib == NOLIBRARY)
	{
		/* this is the first library: make it the current one */
		lib->nextlibrary = NOLIBRARY;
		el_curlib = lib;
	} else
	{
		/* add this library to the list headed by "el_curlib" */
		lib->nextlibrary = el_curlib->nextlibrary;
		el_curlib->nextlibrary = lib;
	}

	/* tell constraint system about new library */
	(*el_curconstraint->newlib)(lib);

	/* mark a change to the database */
	db_changetimestamp++;

	/* report library address */
	return(lib);
}

/*
 * routine to set the current library (represented by the global "el_curlib")
 * to "lib"
 */
void selectlibrary(LIBRARY *lib, BOOLEAN changelambda)
{
	REGISTER LIBRARY *l, *lastlib;
	REGISTER TECHNOLOGY *tech;

	/* quit if already done */
	if (lib == NOLIBRARY) return;
	if (el_curlib == lib) return;

	/* unlink library from its current position */
	lastlib = NOLIBRARY;
	for(l = el_curlib; l != NOLIBRARY; l = l->nextlibrary)
	{
		if (l == lib) break;
		lastlib = l;
	}
	if (lastlib != NOLIBRARY) lastlib->nextlibrary = lib->nextlibrary;

	/* link in at the head of the list */
	lib->nextlibrary = el_curlib;
	el_curlib = lib;

	if (changelambda)
	{
		for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
			changetechnologylambda(tech, el_curlib->lambda[tech->techindex]);
	}

	/* mark a change to the database */
	db_changetimestamp++;
}

void killlibrary(LIBRARY *lib)
{
	REGISTER LIBRARY *l, *lastlib;

	if (lib == el_curlib)
	{
		(void)db_error(DBBADLIB|DBKILLLIBRARY);
		return;
	}

	/* tell constraint system about killed library */
	(*el_curconstraint->killlib)(lib);

	/* unlink current library */
	lastlib = NOLIBRARY;
	for(l = el_curlib; l != NOLIBRARY; l = l->nextlibrary)
	{
		if (l == lib) break;
		lastlib = l;
	}
	if (lastlib != NOLIBRARY) lastlib->nextlibrary = lib->nextlibrary;

	/* kill the requested library */
	eraselibrary(lib);
	efree(lib->libfile);
	efree(lib->libname);
	freelibrary(lib);
}

/*
 * routine to erase the contents of a library of cells.
 * The index of the library is in "libindex".
 */
void eraselibrary(LIBRARY *lib)
{
	REGISTER NODEPROTO *np, *lnp;
	REGISTER PORTPROTO *pp, *lpt;
	REGISTER NODEINST *ni, *nni;
	REGISTER ARCINST *ai, *nai;
	REGISTER PORTARCINST *pi, *lpo;
	REGISTER PORTEXPINST *pe, *lpe;
	REGISTER NETWORK *net, *nnet;
	REGISTER INTBIG i;

	/* see if this library exists */
	if (lib == NOLIBRARY) return;

	/* flush all batched changes */
	noundoallowed();

	for(i=0; i<el_maxtools; i++)
		if ((el_tools[i].toolstate & TOOLON) != 0 && el_tools[i].eraselibrary != 0)
			(*el_tools[i].eraselibrary)(lib);

	/* erase the nodes, ports, arcs, and geometry modules in each nodeproto */
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		/* erase all arcs and nodes in this cell */
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = nni)
		{
			nni = ni->nextnodeinst;

			/* remove nodeinst link */
			if (ni->nextinst != NONODEINST) ni->nextinst->previnst = ni->previnst;
			if (ni->previnst != NONODEINST) ni->previnst->nextinst = ni->nextinst; else
				ni->proto->firstinst = ni->nextinst;

			/* erase the portarcs on this nodeinst */
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = lpo)
			{
				lpo = pi->nextportarcinst;
				freeportarcinst(pi);
			}

			/* erase the portexps on this nodeinst */
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = lpe)
			{
				lpe = pe->nextportexpinst;
				freeportexpinst(pe);
			}

			/* erase the nodeinst */
			freegeom(ni->geom);
			freenodeinst(ni);
		}
		for(ai = np->firstarcinst; ai != NOARCINST; ai = nai)
		{
			nai = ai->nextarcinst;
			freegeom(ai->geom);
			freearcinst(ai);
		}
		for(net = np->firstnetwork; net != NONETWORK; net = nnet)
		{
			nnet = net->nextnetwork;
			net_freenetwork(net, np);
		}
		db_freertree(np->rtree);
	}

	/* now erase the portprotos and nodeprotos */
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = lnp)
	{
		lnp = np->nextnodeproto;

		/* free the portproto entries */
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = lpt)
		{
			lpt = pp->nextportproto;
			efree(pp->protoname);
			freeportproto(pp);
		}

		/* free the nodeinst proto */
		freenodeproto(np);
	}
	lib->firstnodeproto = NONODEPROTO;
	lib->tailnodeproto = NONODEPROTO;
	lib->curnodeproto = NONODEPROTO;

	/* now erase the library information */
	if (lib->numvar != 0) db_freevars(&lib->firstvar, &lib->numvar);

	/* mark a change to the database */
	db_changetimestamp++;
}

/*
 * routine to determine the appropriate library associated with the
 * variable whose address is "addr" and type is "type".
 */
LIBRARY *whichlibrary(INTBIG addr, INTBIG type)
{
	REGISTER GEOM *geom;
	REGISTER NODEPROTO *np;

	switch (type&VTYPE)
	{
		case VNODEINST: return(((NODEINST *)addr)->parent->lib);
		case VNODEPROTO:
			np = (NODEPROTO *)addr;
			if (np->primindex == 0) return(np->lib);
			return(NOLIBRARY);
		case VPORTARCINST:
			return(((PORTARCINST *)addr)->conarcinst->parent->lib);
		case VPORTEXPINST:
			return(((PORTEXPINST *)addr)->exportproto->parent->lib);
		case VPORTPROTO:
			np = ((PORTPROTO *)addr)->parent;
			if (np->primindex == 0) return(np->lib);
			return(NOLIBRARY);
		case VARCINST: return(((ARCINST *)addr)->parent->lib);
		case VGEOM: geom = (GEOM *)addr;
			if (geom->entryisnode)
				return(geom->entryaddr.ni->parent->lib);
			return(geom->entryaddr.ai->parent->lib);
		case VLIBRARY: return((LIBRARY *)addr);
		case VNETWORK: return(((NETWORK *)addr)->parent->lib);
	}
	return(NOLIBRARY);
}

/****************************** LAMBDA ******************************/

/*
 * routine to change value of lambda in "count" technologies.  The
 * technologies are in "techarray" and the new lambda values for them
 * are in "newlam".
 * If "how" is 0, only change the technology (no libraries)
 * If "how" is 1, change technology and library "whichlib"
 * If "how" is 2, change technology and all libraries, specifically "whichlib"
 */
void changelambda(INTBIG count, TECHNOLOGY **techarray, INTBIG *newlam,
	LIBRARY *whichlib, INTBIG how)
{
	REGISTER INTBIG *oldlam, i;
	REGISTER NODEPROTO *np;
	REGISTER LIBRARY *lib;
	REGISTER TECHNOLOGY *tech;

	if (how == 0)
	{
		/* special case when only changing technology values */
		for(i=0; i<count; i++)
			changetechnologylambda(techarray[i], newlam[i]);
		return;
	}

	/* make the technology agree with the current library */
	for(i=0; i<count; i++)
		changetechnologylambda(techarray[i], newlam[i]);

	/* update lambda values in the libraries */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		lib->temp2 = 0;
		if (how == 1 && whichlib != lib) continue;

		/* announce start of changes to this library */
		startobjectchange((INTBIG)lib, VLIBRARY);

		/* preserve lambda for this library */
		oldlam = (INTBIG *)emalloc(count * SIZEOFINTBIG, el_tempcluster);
		if (oldlam == 0) return;
		for(i=0; i<count; i++)
		{
			tech = techarray[i];
			oldlam[i] = lib->lambda[tech->techindex];
			lib->lambda[tech->techindex] = newlam[i];
		}

		lib->temp2 = (INTBIG)oldlam;

	}

	/* mark all cells in the library for scaling */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = 0;

	/* scale all the cells */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if (how == 1 && whichlib != lib) continue;
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			db_scalecell(np, count, techarray, newlam);
		if (lib->firstnodeproto != NONODEPROTO)
			lib->userbits |= LIBCHANGEDMAJOR;
	}

	/* free old lambda values */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if (how == 1 && whichlib != lib) continue;
		efree((CHAR *)lib->temp2);

		/* announce end of changes to this library */
		endobjectchange((INTBIG)lib, VLIBRARY);
	}

}

/*
 * routine to recursively scale the contents of cell "np" by "newlam/oldlam"
 */
void db_scalecell(NODEPROTO *np, INTBIG count, TECHNOLOGY **techarray, INTBIG *newlamarray)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER VARIABLE *var;
	REGISTER INTBIG i, *oldlamarray, *position, oldlam, newlam;

	/* quit if the cell is already done */
	if (np->temp1 != 0) return;

	/* first look for sub-cells that are not yet scaled */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		if (ni->proto->primindex == 0)
			db_scalecell(ni->proto, count, techarray, newlamarray);

	/* mark this cell "scaled" */
	np->temp1++;

	/* see if this cell gets scaled */
	for(i=0; i<count; i++)
		if (np->tech == techarray[i]) break;
	if (i < count)
	{
		/* get the old lambda value */
		oldlamarray = (INTBIG *)np->lib->temp2;
		if (oldlamarray == 0) return;
		oldlam = oldlamarray[i];
		newlam = newlamarray[i];

		/* scale nodes and arcs in the cell */
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			db_scalenodeinst(ni, newlam, oldlam);
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			db_scalearcinst(ai, newlam, oldlam);

		/* scale the cell */
		db_boundcell(np, &np->lowx, &np->highx, &np->lowy, &np->highy);

		/* scale variables such as cell-center and characteristic spacing */
		var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER|VISARRAY, el_prototype_center_key);
		if (var != NOVARIABLE)
		{
			position = (INTBIG *)var->addr;
			position[0] = muldiv(position[0], newlam, oldlam);
			position[1] = muldiv(position[1], newlam, oldlam);
		}
		var = getval((INTBIG)np, VNODEPROTO, VINTEGER|VISARRAY, x_("FACET_characteristic_spacing"));
		if (var != NOVARIABLE)
		{
			position = (INTBIG *)var->addr;
			position[0] = muldiv(position[0], newlam, oldlam);
			position[1] = muldiv(position[1], newlam, oldlam);
		}
	}
}

void db_scalenodeinst(NODEINST *ni, INTBIG newlam, INTBIG oldlam)
{
	REGISTER VARIABLE *var;
	REGISTER INTBIG sizex, sizey, dx, dy, len, i, newlx, newhx, newly, newhy;

	newlx = muldiv(ni->lowx,  newlam, oldlam);
	newhx = muldiv(ni->highx, newlam, oldlam);
	newly = muldiv(ni->lowy,  newlam, oldlam);
	newhy = muldiv(ni->highy, newlam, oldlam);

	if (ni->proto->primindex == 0)
	{
		sizex = ni->proto->highx - ni->proto->lowx;
		sizey = ni->proto->highy - ni->proto->lowy;
		dx = ni->highx - ni->lowx - sizex;
		dy = ni->highy - ni->lowy - sizey;
		ni->lowx = newlx;   ni->highx = newhx;
		ni->lowy = newly;   ni->highy = newhy;
		if (dx != 0 || dy != 0)
		{
			dx = ni->highx - ni->lowx - sizex;
			dy = ni->highy - ni->lowy - sizey;
			if (dx != 0 || dy != 0)
			{
				ttyputmsg(_("Cell %s, node %s size and position adjusted"),
					describenodeproto(ni->parent), describenodeinst(ni));
				ni->lowx += dx/2;   ni->highx = ni->lowx + sizex;
				ni->lowy += dy/2;   ni->highy = ni->lowy + sizey;
			}
		}
	} else
	{
		/* look for trace data on primitives */
		ni->lowx = newlx;   ni->highx = newhx;
		ni->lowy = newly;   ni->highy = newhy;
		var = gettrace(ni);
		if (var != NOVARIABLE)
		{
			len = getlength(var);
			for(i=0; i<len; i++)
				((INTBIG *)var->addr)[i] = muldiv(((INTBIG *)var->addr)[i], newlam, oldlam);
		}
	}
	updategeom(ni->geom, ni->parent);
}

void db_scalearcinst(ARCINST *ai, INTBIG newlam, INTBIG oldlam)
{
	REGISTER VARIABLE *var;

	ai->width = muldiv(ai->width, newlam, oldlam);
	ai->end[0].xpos = muldiv(ai->end[0].xpos, newlam, oldlam);
	ai->end[0].ypos = muldiv(ai->end[0].ypos, newlam, oldlam);
	ai->end[1].xpos = muldiv(ai->end[1].xpos, newlam, oldlam);
	ai->end[1].ypos = muldiv(ai->end[1].ypos, newlam, oldlam);
	ai->length = computedistance(ai->end[0].xpos, ai->end[0].ypos,
		ai->end[1].xpos, ai->end[1].ypos);
	db_validatearcinst(ai);
	updategeom(ai->geom, ai->parent);

	/* look for curvature data on primitives */
	var = getvalkey((INTBIG)ai, VARCINST, VINTEGER, el_arc_radius_key);
	if (var != NOVARIABLE) var->addr = muldiv(var->addr, newlam, oldlam);
}

/*
 * routine to validate arcinst "ai" after being scaled.
 */
void db_validatearcinst(ARCINST *ai)
{
	REGISTER BOOLEAN inside0, inside1;
	INTBIG lx0, lx1, hx0, hx1, ly0, ly1, hy0, hy1, fx, fy, tx, ty;
	REGISTER POLYGON *poly0, *poly1;

	/* make sure there is a polygon */
	poly0 = allocpolygon(4, db_cluster);
	poly1 = allocpolygon(4, db_cluster);

	/* if nothing is outside port, quit */
	fx = ai->end[0].xpos;
	fy = ai->end[0].ypos;
	tx = ai->end[1].xpos;
	ty = ai->end[1].ypos;
	shapeportpoly(ai->end[0].nodeinst, ai->end[0].portarcinst->proto, poly0, FALSE);
	inside0 = isinside(fx, fy, poly0);
	shapeportpoly(ai->end[1].nodeinst, ai->end[1].portarcinst->proto, poly1, FALSE);
	inside1 = isinside(tx, ty, poly1);
	if (!inside0 || !inside1)
	{
		/* if arcinst is not fixed-angle, run it directly to the port centers */
		if ((ai->userbits&FIXANG) == 0)
		{
			if (!inside0) closestpoint(poly0, &fx, &fy);
			if (!inside1) closestpoint(poly1, &tx, &ty);
			db_setarcinst(ai, fx, fy, tx, ty);
		} else
		{
			/* get bounding boxes of polygons */
			getbbox(poly0, &lx0, &hx0, &ly0, &hy0);
			getbbox(poly1, &lx1, &hx1, &ly1, &hy1);

			/* if fixed-angle path runs between the ports, adjust the arcinst */
			if (arcconnects(((ai->userbits&AANGLE) >> AANGLESH) * 10, lx0,hx0, ly0,hy0, lx1,hx1,
				ly1,hy1, &fx,&fy, &tx,&ty))
			{
				closestpoint(poly0, &fx, &fy);
				closestpoint(poly1, &tx, &ty);
				db_setarcinst(ai, fx, fy, tx, ty);
			} else
			{
				/* give up and remove the constraint */
				fx = ai->end[0].xpos;
				fy = ai->end[0].ypos;
				tx = ai->end[1].xpos;
				ty = ai->end[1].ypos;
				if (!inside0) closestpoint(poly0, &fx, &fy);
				if (!inside1) closestpoint(poly1, &tx, &ty);
				ai->userbits &= ~FIXANG;
				db_setarcinst(ai, fx, fy, tx, ty);
				ttyputmsg(_("Cell %s, arc %s no longer fixed-angle"),
					describenodeproto(ai->parent), describearcinst(ai));
			}
		}
	}
	freepolygon(poly0);
	freepolygon(poly1);
}

void db_setarcinst(ARCINST *ai, INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty)
{
	/* check for null arcinst motion */
	if (fx == ai->end[0].xpos && fy == ai->end[0].ypos &&
		tx == ai->end[1].xpos && ty == ai->end[1].ypos) return;

	ai->end[0].xpos = fx;   ai->end[0].ypos = fy;
	ai->end[1].xpos = tx;   ai->end[1].ypos = ty;
	ai->length = computedistance(fx,fy, tx,ty);
	determineangle(ai);
	updategeom(ai->geom, ai->parent);
}

/*
 * Routine to change the value of lambda in technology "tech" to
 * "newlambda".
 */
void changetechnologylambda(TECHNOLOGY *tech, INTBIG newlambda)
{
	REGISTER NODEPROTO *np;
	REGISTER ARCPROTO *ap;
	REGISTER INTBIG oldlambda;

	oldlambda = tech->deflambda;
	if (newlambda <= 0) newlambda = 1;
	if (newlambda == oldlambda) return;

	/* change the default width of the primitive arc prototypes */
	for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
	{
		ap->nominalwidth = muldiv(ap->nominalwidth, newlambda, oldlambda);
	}

	/* now change the default size of the primitive node prototypes */
	for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		np->lowx  = muldiv(np->lowx,  newlambda, oldlambda);
		np->highx = muldiv(np->highx, newlambda, oldlambda);
		np->lowy  = muldiv(np->lowy,  newlambda, oldlambda);
		np->highy = muldiv(np->highy, newlambda, oldlambda);
	}

	/* finally, reset the default lambda value */
	tech->deflambda = newlambda;

	/* mark a change to the database */
	db_changetimestamp++;
}

/*
 * Routine to change the internal units in library "whichlib" from "oldunits"
 * to "newunits".  If "whichlib" is NOLIBRARY, change the entire database (all
 * libraries and technologies).
 */
void changeinternalunits(LIBRARY *whichlib, INTBIG oldunits, INTBIG newunits)
{
	REGISTER INTBIG hardscaleerror, softscaleerror, len;
	INTBIG num, den;
	REGISTER INTBIG i;
	REGISTER CHAR *errortype, *errorlocation;
	REGISTER TECHNOLOGY *tech;
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;
	REGISTER ARCPROTO *ap;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER VARIABLE *var;
	REGISTER WINDOWPART *w;

	hardscaleerror = softscaleerror = 0;
	db_getinternalunitscale(&num, &den, oldunits, newunits);
	if (whichlib == NOLIBRARY)
	{
		/* global change: set units */
		if ((oldunits&INTERNALUNITS) == newunits) return;
		el_units = (el_units & ~INTERNALUNITS) | newunits;

		/* scale all technologies */
		for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		{
			/* scale arc width */
			for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			{
				hardscaleerror += db_scaleunits(&ap->nominalwidth, num, den);
			}

			/* scale node sizes */
			for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				hardscaleerror += db_scaleunits(&np->lowx,  num, den);
				hardscaleerror += db_scaleunits(&np->highx, num, den);
				hardscaleerror += db_scaleunits(&np->lowy,  num, den);
				hardscaleerror += db_scaleunits(&np->highy, num, den);
			}

			/* finally, scale lambda */
			hardscaleerror += db_scaleunits(&tech->deflambda, num, den);
			if (tech->deflambda <= 0) tech->deflambda = 1;
		}

		/* scale lambda in libraries */
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			for(i=0; i<el_maxtech; i++)
				hardscaleerror += db_scaleunits(&lib->lambda[i], num, den);

		/* scale all display windows */
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if (w->curnodeproto == NONODEPROTO) continue;
			softscaleerror += db_scaleunits(&w->screenlx, num, den);
			softscaleerror += db_scaleunits(&w->screenly, num, den);
			softscaleerror += db_scaleunits(&w->screenhx, num, den);
			softscaleerror += db_scaleunits(&w->screenhy, num, den);
		}
	}

	/* scale all appropriate libraries */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		/* only scale requested library if request was made */
		if (whichlib != NOLIBRARY && whichlib != lib) continue;

		/* scale each cell */
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			/* scale nodes */
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				hardscaleerror += db_scaleunits(&ni->lowx,  num, den);
				hardscaleerror += db_scaleunits(&ni->highx, num, den);
				hardscaleerror += db_scaleunits(&ni->lowy,  num, den);
				hardscaleerror += db_scaleunits(&ni->highy, num, den);
				if (ni->proto->primindex != 0)
				{
					/* look for trace data on primitives */
					var = gettrace(ni);
					if (var != NOVARIABLE)
					{
						len = getlength(var);
						for(i=0; i<len; i++)
							hardscaleerror += db_scaleunits(&((INTBIG *)var->addr)[i], num, den);
					}
				}
				updategeom(ni->geom, np);
			}

			/* scale arcs */
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			{
				hardscaleerror += db_scaleunits(&ai->width, num, den);
				hardscaleerror += db_scaleunits(&ai->end[0].xpos, num, den);
				hardscaleerror += db_scaleunits(&ai->end[0].ypos, num, den);
				hardscaleerror += db_scaleunits(&ai->end[1].xpos, num, den);
				hardscaleerror += db_scaleunits(&ai->end[1].ypos, num, den);
				ai->length = computedistance(ai->end[0].xpos, ai->end[0].ypos,
					ai->end[1].xpos, ai->end[1].ypos);
				updategeom(ai->geom, np);

				/* look for curvature data */
				var = getvalkey((INTBIG)ai, VARCINST, VINTEGER, el_arc_radius_key);
				if (var != NOVARIABLE) hardscaleerror += db_scaleunits(&var->addr, num, den);
			}

			/* scale cell size */
			db_boundcell(np, &np->lowx, &np->highx, &np->lowy, &np->highy);
		}
		lib->userbits = (lib->userbits & ~LIBUNITS) |
			(((newunits & INTERNALUNITS) >> INTERNALUNITSSH) << LIBUNITSSH);
		if (lib->firstnodeproto != NONODEPROTO) lib->userbits |= LIBCHANGEDMAJOR;
	}

	/* report any failures */
	if (hardscaleerror != 0 || softscaleerror != 0)
	{
		if (den > num) errortype = _("roundoff"); else
			errortype = _("overflow");
		if (hardscaleerror == 0) errorlocation = _("display"); else
		{
			if (whichlib == NOLIBRARY) errorlocation = _("database"); else
				errorlocation = _("library");
		}
		ttyputerr(_("Change caused %s errors in %s"), errortype, errorlocation);
		if (hardscaleerror != 0)
		{
			if (whichlib == NOLIBRARY)
			{
				ttyputmsg(_("Recommend check of database for validity and keep old library files"));
				for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
					lib->userbits &= ~READFROMDISK;
			} else
			{
				ttyputmsg(_("Recommend check of database for validity and keep old library file"));
				whichlib->userbits &= ~READFROMDISK;
			}
		} else
		{
			ttyputmsg(_("Recommend redisplay"));
		}
	}

	/* mark a change to the database */
	db_changetimestamp++;
}

/*
 * Routine to scale "value" by "num"/"den".  Returns zero if scale worked,
 * nonzero if overflow/underflow occurred.
 */
INTBIG db_scaleunits(INTBIG *value, INTBIG num, INTBIG den)
{
	REGISTER INTBIG orig, scaled, reconstructed;

	orig = *value;
	scaled = muldiv(orig, num, den);
	*value = scaled;
	reconstructed = muldiv(scaled, den, num);
	if (orig == reconstructed) return(0);
	return(1);
}
