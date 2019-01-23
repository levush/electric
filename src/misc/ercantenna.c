/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: ercantenna.c
 * Electrical Rules Checking tool: Antenna Rules check
 * Written by: Steven M. Rubin
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

/*
 * Antenna rules are required by some IC manufacturers to ensure that the transistors of the
 * chip are not destroyed during fabrication.  This is because, during fabrication, the wafer is
 * bombarded with ions while making the polysilicon and metal layers.
 * These ions must find a path to through the wafer (to the substrate and active layers at the bottom).
 * If there is a large area of poly or metal, and if it connects ONLY to gates of transistors
 * (not to source or drain or any other active material) then these ions will travel through
 * the transistors.  If the ratio of the poly or metal layers to the area of the transistors
 * is too large, the transistors will be fried.
 */

#include "config.h"
#if ERCTOOL

#include "global.h"
#include "efunction.h"
#include "egraphics.h"
#include "edialogs.h"
#include "erc.h"

/*
 * Things to do:
 *   Have errors show the gates, too
 *   Progress indicator
 *   Extended comment explaining what antenna is
 */
/* #define DEBUGANTENNA 1 */			/* uncomment to debug */

#define NOANTENNAOBJ ((ANTENNAOBJ *)-1)

typedef struct Iantennaobj
{
	GEOM      *geom;		/* the object */
	INTBIG     depth;		/* the depth of hierarchy at this node */
	INTBIG     otherend;	/* end of arc to walk along */
	NODEINST **hierstack;	/* the hierarchical stack at this node */
	INTBIG     total;		/* total size of the hierarchical stack */
	struct Iantennaobj *nextantennaobj;
} ANTENNAOBJ;

static ANTENNAOBJ *erc_antfirstspreadantennaobj;/* head of linked list of antenna objects to spread */
static ANTENNAOBJ *erc_antantennaobjfree = NOANTENNAOBJ;

static POLYLIST   *erc_antpolylist = 0;			/* for gathering polygons */
static TECHNOLOGY *erc_anttech;					/* current technology being considered */
static INTBIG      erc_antlambda;				/* value of lambda for this technology */
static INTBIG     *erc_antlayerlevel;			/* level of each layer in the technology */
static INTBIG     *erc_antlayerforlevel;		/* layer associated with each antenna-check level */
static NODEPROTO  *erc_anttopcell;				/* top-level cell being checked */
static float       erc_antgatearea;				/* accumulated gate area */
static float       erc_antregionarea;			/* accumulated region area */
static void       *erc_anterror;				/* error report being generated */
static TECHNOLOGY *erc_antcachedtech = NOTECHNOLOGY;
static INTBIG     *erc_antcachedratios;
static INTBIG      erc_antworstratio;			/* the worst ratio found */

static ANTENNAOBJ **erc_antpathhashtable;			/* the antenna objects hash table */
static INTBIG       erc_antpathhashtablesize = 0;	/* size of the antenna objects hash table */
static INTBIG       erc_antpathhashtablecount;		/* number of antenna objects in the hash table */
       INTBIG       erc_antennaarcratiokey;			/* key for "ERC_antenna_arc_ratio" */

/* the level meanings (for arcprotos and layers) */
#define ERCANTIGNORE     0
#define ERCANTDIFFUSION  1
#define ERCANTLEVEL1     2

#define ERCANTPATHNULL   0		/* nothing found on the path */
#define ERCANTPATHGATE   1		/* found a gate on the path */
#define ERCANTPATHACTIVE 2		/* found active on the path */

#define ERCDEFPOLYLIMIT  200	/* maximum ratio of poly to gate area */
#define ERCDEFMETALLIMIT 400	/* maximum ratio of metal to gate area */

/* prototypes for local routines */
static void        erc_antcheckthiscell(NODEPROTO *cell, INTBIG level);
static INTBIG      erc_antfollownode(NODEINST *ni, PORTPROTO *pp, INTBIG level, XARRAY trans);
static void        erc_antgetpolygonarea(INTBIG layer, TECHNOLOGY *tech, INTBIG *x, INTBIG *y, INTBIG count);
static void        erc_antputpolygoninerror(INTBIG layer, TECHNOLOGY *tech, INTBIG *x, INTBIG *y, INTBIG count);
static ANTENNAOBJ *erc_antallocantennaobj(void);
static void        erc_antloadantennaobj(ANTENNAOBJ *ao, INTBIG depth, NODEINST **stack);
static void        erc_antaddantennaobj(ANTENNAOBJ *ao);
static BOOLEAN     erc_anthaveantennaobj(ANTENNAOBJ *ao);
static void        erc_antfreeantennaobj(ANTENNAOBJ *ao);
static INTBIG      erc_antfindarcs(NODEINST *ni, PORTPROTO *pp, INTBIG level, INTBIG depth, NODEINST **antstack);
static INTBIG      erc_antfindexports(NODEINST *ni, PORTPROTO *pp, INTBIG level, INTBIG depth, NODEINST **antstack);
static void        erc_antinsertantennaobj(ANTENNAOBJ *ao);
static INTBIG      erc_antgethash(ANTENNAOBJ *ao);
static INTBIG      erc_antratio(INTBIG level);
static BOOLEAN     erc_hasdiffusion(NODEINST *ni);

/************************ CONTROL ***********************/

/*
 * Main entry point for antenna check rules.  Checks rules in cell "cell".
 */
void erc_antcheckcell(NODEPROTO *cell)
{
	REGISTER LIBRARY *lib;
	REGISTER INTBIG i, fun, level, highestlevel, lasterrorcount;
	REGISTER NODEPROTO *np;
	REGISTER ARCINST *ai;
	REGISTER ARCPROTO *ap;
	float t;

	erc_anttopcell = cell;
	erc_anttech = cell->tech;
	erc_antlambda = el_curlib->lambda[erc_anttech->techindex];

	/* clear marks on all arcs in all cells */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
				ai->temp1 = 0;
		}
	}

	/* initialize properties of the arc prototypes */
	highestlevel = ERCANTLEVEL1;
	for(ap = erc_anttech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
	{
		ap->temp1 = ERCANTIGNORE;
		fun = (ap->userbits&AFUNCTION) >> AFUNCTIONSH;
		switch (fun)
		{
			case APDIFF:
			case APDIFFP:
			case APDIFFN:
			case APDIFFS:
			case APDIFFW:   ap->temp1 = ERCANTDIFFUSION;  break;
			case APPOLY1:   ap->temp1 = ERCANTLEVEL1;     break;
			case APMETAL1:  ap->temp1 = ERCANTLEVEL1+1;   break;
			case APMETAL2:  ap->temp1 = ERCANTLEVEL1+2;   break;
			case APMETAL3:  ap->temp1 = ERCANTLEVEL1+3;   break;
			case APMETAL4:  ap->temp1 = ERCANTLEVEL1+4;   break;
			case APMETAL5:  ap->temp1 = ERCANTLEVEL1+5;   break;
			case APMETAL6:  ap->temp1 = ERCANTLEVEL1+6;   break;
			case APMETAL7:  ap->temp1 = ERCANTLEVEL1+7;   break;
			case APMETAL8:  ap->temp1 = ERCANTLEVEL1+8;   break;
			case APMETAL9:  ap->temp1 = ERCANTLEVEL1+9;   break;
			case APMETAL10: ap->temp1 = ERCANTLEVEL1+10;  break;
			case APMETAL11: ap->temp1 = ERCANTLEVEL1+11;  break;
			case APMETAL12: ap->temp1 = ERCANTLEVEL1+12;  break;
		}
		if (ap->temp1 > highestlevel) highestlevel = ap->temp1;
	}

	/* initialize the properties of the layers */
	erc_antlayerlevel = (INTBIG *)emalloc(erc_anttech->layercount * SIZEOFINTBIG, erc_tool->cluster);
	if (erc_antlayerlevel == 0) return;
	erc_antlayerforlevel = (INTBIG *)emalloc(erc_anttech->layercount * SIZEOFINTBIG, erc_tool->cluster);
	if (erc_antlayerforlevel == 0) return;
	for(i=0; i<erc_anttech->layercount; i++) erc_antlayerforlevel[i] = 0;
	for(i=erc_anttech->layercount-1; i>=0; i--)
	{
		erc_antlayerlevel[i] = ERCANTIGNORE;
		fun = layerfunction(erc_anttech, i);
		if ((fun&LFPSEUDO) != 0) continue;
		switch (fun&LFTYPE)
		{
			case LFDIFF:    erc_antlayerlevel[i] = ERCANTDIFFUSION;  break;
			case LFPOLY1:   erc_antlayerlevel[i] = ERCANTLEVEL1;     break;
			case LFMETAL1:  erc_antlayerlevel[i] = ERCANTLEVEL1+1;   break;
			case LFMETAL2:  erc_antlayerlevel[i] = ERCANTLEVEL1+2;   break;
			case LFMETAL3:  erc_antlayerlevel[i] = ERCANTLEVEL1+3;   break;
			case LFMETAL4:  erc_antlayerlevel[i] = ERCANTLEVEL1+4;   break;
			case LFMETAL5:  erc_antlayerlevel[i] = ERCANTLEVEL1+5;   break;
			case LFMETAL6:  erc_antlayerlevel[i] = ERCANTLEVEL1+6;   break;
			case LFMETAL7:  erc_antlayerlevel[i] = ERCANTLEVEL1+7;   break;
			case LFMETAL8:  erc_antlayerlevel[i] = ERCANTLEVEL1+8;   break;
			case LFMETAL9:  erc_antlayerlevel[i] = ERCANTLEVEL1+9;   break;
			case LFMETAL10: erc_antlayerlevel[i] = ERCANTLEVEL1+10;  break;
			case LFMETAL11: erc_antlayerlevel[i] = ERCANTLEVEL1+11;  break;
			case LFMETAL12: erc_antlayerlevel[i] = ERCANTLEVEL1+12;  break;
		}
		if (erc_antlayerlevel[i] > highestlevel) highestlevel = erc_antlayerlevel[i];
		erc_antlayerforlevel[erc_antlayerlevel[i]] = i;
	}

	if (erc_antpolylist == 0)
		erc_antpolylist = allocpolylist(erc_tool->cluster);

	/* start the clock */
	starttimer();

	/* initialize error logging */
	initerrorlogging(x_("ERC antenna rules"));

	/* now check the cell recursively */
	lasterrorcount = 0;
	erc_antworstratio = 0;
#ifdef DEBUGANTENNA
	level = ERCANTLEVEL1+2;
#else
	for(level = ERCANTLEVEL1; level <= highestlevel; level++)
#endif
	{
		ttyputmsg(_("Checking Antenna rules for %s..."),
			layername(erc_anttech, erc_antlayerforlevel[level]));

		/* clear timestamps on all cell */
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				np->temp1 = 0;

		/* do the check for this level */
		erc_antcheckthiscell(cell, level);
		i = numerrors();
		if (i != lasterrorcount)
		{
			ttyputmsg(_("  Found %ld errors"), i - lasterrorcount);
			lasterrorcount = i;
		}
	}

	t = endtimer();
	i = numerrors();
	ttyputmsg(_("Worst antenna ratio found is %ld"), erc_antworstratio);
	if (i == 0) ttyputmsg(_("No Antenna errors found (took %s)"), explainduration(t)); else
		ttyputmsg(_("Found %ld Antenna errors (took %s)"), i, explainduration(t));

	/* finish error gathering */
	termerrorlogging(TRUE);

	/* clean up */
	efree((CHAR *)erc_antlayerlevel);
	efree((CHAR *)erc_antlayerforlevel);
}

void erc_antterm(void)
{
	REGISTER ANTENNAOBJ *ao;

	while (erc_antantennaobjfree != NOANTENNAOBJ)
	{
		ao = erc_antantennaobjfree;
		erc_antantennaobjfree = ao->nextantennaobj;
		if (ao->total > 0) efree((CHAR *)ao->hierstack);
		efree((CHAR *)ao);
	}
	if (erc_antpolylist != 0)
		freepolylist(erc_antpolylist);
	if (erc_antpathhashtablesize > 0)
		efree((CHAR *)erc_antpathhashtable);
	if (erc_antcachedtech != NOTECHNOLOGY)
		efree((CHAR *)erc_antcachedratios);
}

/*
 * Routine to check the contents of cell "cell", considering geometry on level "level" of antenna depth.
 */
void erc_antcheckthiscell(NODEPROTO *cell, INTBIG level)
{
	REGISTER NODEINST *ni, *oni;
	REGISTER INTBIG found, ratio, neededratio, i, j, tot;
	REGISTER POLYGON *poly;
	REGISTER PORTARCINST *pi;
	REGISTER ANTENNAOBJ *ao;
	REGISTER PORTPROTO *pp;
	XARRAY trans, rtrans, ttrans, temptrans;
	REGISTER void *infstr, *vmerge;

	/* examine every node and follow all relevant arcs */
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst) ni->temp1 = 0;

	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->temp1 != 0) continue;
		ni->temp1 = 1;

		/* check every connection on the node */
		for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			/* ignore if an arc on this port is already seen */
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				if (pi->proto == pp && pi->conarcinst->temp1 != 0) break;
			if (pi != NOPORTARCINST) continue;

			erc_antgatearea = 0.0;
			erc_antpathhashtablecount = 0;
			for(i=0; i<erc_antpathhashtablesize; i++) erc_antpathhashtable[i] = NOANTENNAOBJ;

			found = erc_antfollownode(ni, pp, level, el_matid);
			if (found == ERCANTPATHGATE)
			{
				/* gather the geometry here */
				vmerge = 0;
				for(j=0; j<erc_antpathhashtablesize; j++)
				{
					ao = erc_antpathhashtable[j];
					if (ao == NOANTENNAOBJ) continue;

					if (ao->geom->entryisnode)
					{
						oni = ao->geom->entryaddr.ni;
						makerot(oni, trans);
						for(i=ao->depth-1; i>=0; i--)
						{
							maketrans(ao->hierstack[i], ttrans);
							transmult(trans, ttrans, temptrans);
							makerot(ao->hierstack[i], rtrans);
							transmult(temptrans, rtrans, trans);
						}

						tot = allnodeEpolys(oni, erc_antpolylist, NOWINDOWPART, TRUE);
						for(i=0; i<tot; i++)
						{
							poly = erc_antpolylist->polygons[i];
							if (poly->tech != erc_anttech) continue;
							if (erc_antlayerlevel[poly->layer] != level) continue;
							if (vmerge == 0)
								vmerge = mergenew(erc_tool->cluster);
							xformpoly(poly, trans);
							mergeaddpolygon(vmerge, poly->layer, poly->tech, poly);
						}
					} else
					{
						transid(trans);
						for(i=ao->depth-1; i>=0; i--)
						{
							maketrans(ao->hierstack[i], ttrans);
							transmult(trans, ttrans, temptrans);
							makerot(ao->hierstack[i], rtrans);
							transmult(temptrans, rtrans, trans);
						}

						tot = allarcpolys(ao->geom->entryaddr.ai, erc_antpolylist, NOWINDOWPART);
						for(i=0; i<tot; i++)
						{
							poly = erc_antpolylist->polygons[i];
							if (poly->tech != erc_anttech) continue;
							if (erc_antlayerlevel[poly->layer] != level) continue;
							if (vmerge == 0)
								vmerge = mergenew(erc_tool->cluster);
							xformpoly(poly, trans);
							mergeaddpolygon(vmerge, poly->layer, poly->tech, poly);
						}
					}
				}
				if (vmerge != 0)
				{
					/* get the area of the antenna */
#ifdef DEBUGANTENNA
					asktool(us_tool, x_("clear"));
#endif
					erc_antregionarea = 0.0;
					mergeextract(vmerge, erc_antgetpolygonarea);

					/* see if it is an antenna violation */
					ratio = (INTBIG)(erc_antregionarea / erc_antgatearea);
					neededratio = erc_antratio(level);
					if (ratio > erc_antworstratio) erc_antworstratio = ratio;
					if (ratio >= neededratio)
					{
						/* error */
						infstr = initinfstr();
						formatinfstr(infstr, _("layer %s has area %s; gates have area %s, ratio is %ld but limit is %ld"),
							layername(erc_anttech, erc_antlayerforlevel[level]),
							latoa((INTBIG)(erc_antregionarea/erc_antlambda), erc_antlambda),
							latoa((INTBIG)(erc_antgatearea/erc_antlambda), erc_antlambda),
							ratio, neededratio);
						erc_anterror = logerror(returninfstr(infstr), cell, 0);
						mergeextract(vmerge, erc_antputpolygoninerror);
					}
#ifdef DEBUGANTENNA
					infstr = initinfstr();
					formatinfstr(infstr, x_("Cell %s: Level %d: Gate size %s, Region size %s, ratio=%g"),
						describenodeproto(cell), level-1,
						latoa((INTBIG)(erc_antgatearea/erc_antlambda), erc_antlambda),
						latoa((INTBIG)(erc_antregionarea/erc_antlambda), erc_antlambda),
						erc_antregionarea / erc_antgatearea);
					if (*ttygetline(returninfstr(infstr)) == 'q') return;
#endif
					mergedelete(vmerge);
				}
			}

			/* free the antenna object list */
			for(i=0; i<erc_antpathhashtablesize; i++)
			{
				ao = erc_antpathhashtable[i];
				if (ao == NOANTENNAOBJ) continue;
				erc_antfreeantennaobj(ao);
			}
		}
	}

	/* now look at subcells */
	cell->temp1 = 1;
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex != 0) continue;
		if (ni->proto->temp1 != 0) continue;

		erc_antcheckthiscell(ni->proto, level);
	}
}

/*
 * Routine to follow node "ni" around the cell.
 *    Returns ERCANTPATHNULL   if it found no gate or active on the path.
 *    Returns ERCANTPATHGATE   if it found gates on the path.
 *    Returns ERCANTPATHACTIVE if it found active on the path.
 */
INTBIG erc_antfollownode(NODEINST *ni, PORTPROTO *pp, INTBIG level, XARRAY trans)
{
	REGISTER INTBIG depth, i, found;
	INTBIG length, width;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *thisni;
	REGISTER BOOLEAN ret, seen;
	REGISTER ANTENNAOBJ *ao;
	NODEINST *antstack[200];

	/* presume that nothing was found */
	ret = ERCANTPATHNULL;
	erc_antfirstspreadantennaobj = NOANTENNAOBJ;
	depth = 0;

	/* keep walking along the nodes and arcs */
	for(;;)
	{
		/* if this is a subcell, recurse on it */
		ni->temp1 = 1;
		thisni = ni;
		while (thisni->proto->primindex == 0)
		{
			antstack[depth] = thisni;
			depth++;
			thisni = pp->subnodeinst;
			pp = pp->subportproto;
		}

		/* see if we hit a transistor */
		seen = FALSE;
		if (isfet(thisni->geom))
		{
			/* stop tracing */
			if (thisni->proto->firstportproto->network == pp->network)
			{
				/* touching the gate side of the transistor */
				transistorsize(thisni, &length, &width);
				erc_antgatearea += (float)(length * width);
				ret = ERCANTPATHGATE;
			} else
			{
				/* touching the diffusion side of the transistor */
				return(ERCANTPATHACTIVE);
			}
		} else
		{
			/* normal primitive: propagate */
			if (erc_hasdiffusion(thisni)) return(ERCANTPATHACTIVE);
			ao = erc_antallocantennaobj();
			if (ao == NOANTENNAOBJ) return(ERCANTPATHACTIVE);
			erc_antloadantennaobj(ao, depth, antstack);
			ao->geom = ni->geom;

			if (erc_anthaveantennaobj(ao))
			{
				/* already in the list: free this object */
				erc_antfreeantennaobj(ao);
				seen = TRUE;
			} else
			{
				/* not in the list: add it */
				erc_antaddantennaobj(ao);
			}
		}

		/* look at all arcs on the node */
		if (!seen)
		{
			found = erc_antfindarcs(thisni, pp, level, depth, antstack);
			if (found == ERCANTPATHACTIVE) return(found);
			if (depth > 0)
			{
				found = erc_antfindexports(thisni, pp, level, depth, antstack);
				if (found == ERCANTPATHACTIVE) return(found);
			}
		}

		/* look for an unspread antenna object and keep walking */
		if (erc_antfirstspreadantennaobj == NOANTENNAOBJ) break;
		ao = erc_antfirstspreadantennaobj;
		erc_antfirstspreadantennaobj = ao->nextantennaobj;

		ai = ao->geom->entryaddr.ai;
		ni = ai->end[ao->otherend].nodeinst;
		pp = ai->end[ao->otherend].portarcinst->proto;
		depth = ao->depth;
		for(i=0; i<depth; i++)
			antstack[i] = ao->hierstack[i];
	}
	return(ret);
}

/*
 * Routine to return TRUE if node "ni" has diffusion on it.
 */
BOOLEAN erc_hasdiffusion(NODEINST *ni)
{
	REGISTER INTBIG i, total, fun;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, io_tool->cluster);

	/* stop if this is a pin */
	if (nodefunction(ni) == NPPIN) return(FALSE);

	/* analyze to see if there is diffusion here */
	total = nodepolys(ni, 0, NOWINDOWPART);
	for(i=0; i<total; i++)
	{
		shapenodepoly(ni, i, poly);
		fun = layerfunction(poly->tech, poly->layer);
		if ((fun&LFTYPE) == LFDIFF) return(TRUE);
	}
	return(FALSE);
}

INTBIG erc_antfindarcs(NODEINST *ni, PORTPROTO *pp, INTBIG level, INTBIG depth, NODEINST **antstack)
{
	REGISTER PORTARCINST *pi;
	REGISTER ARCINST *ai;
	REGISTER INTBIG other;
	REGISTER ANTENNAOBJ *ao;

	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		if (pi->proto != pp) continue;
		ai = pi->conarcinst;

		/* see if it is the desired layer */
		if (ai->proto->temp1 == ERCANTIGNORE) continue;
		if (ai->proto->temp1 == ERCANTDIFFUSION) return(ERCANTPATHACTIVE);
		if (ai->proto->temp1 > level) continue;

		/* make an antenna object for this arc */
		ai->temp1 = 1;
		ao = erc_antallocantennaobj();
		if (ao == NOANTENNAOBJ) return(ERCANTPATHACTIVE);
		erc_antloadantennaobj(ao, depth, antstack);
		ao->geom = ai->geom;

		if (erc_anthaveantennaobj(ao))
		{
			erc_antfreeantennaobj(ao);
			continue;
		}

		if (ai->end[0].portarcinst == pi) other = 1; else other = 0;
		ao->otherend = other;
		erc_antaddantennaobj(ao);

		/* add to the list of "unspread" antenna objects */
		ao->nextantennaobj = erc_antfirstspreadantennaobj;
		erc_antfirstspreadantennaobj = ao;
	}
	return(ERCANTPATHNULL);
}

INTBIG erc_antfindexports(NODEINST *ni, PORTPROTO *pp, INTBIG level, INTBIG depth, NODEINST **antstack)
{
	REGISTER PORTEXPINST *pe;
	REGISTER INTBIG found;

	depth--;
	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
	{
		if (pe->proto != pp) continue;
		ni = antstack[depth];
		pp = pe->exportproto;
		found = erc_antfindarcs(ni, pp, level, depth, antstack);
		if (found == ERCANTPATHACTIVE) return(found);
		if (depth > 0)
		{
			found = erc_antfindexports(ni, pp, level, depth, antstack);
			if (found == ERCANTPATHACTIVE) return(found);
		}
	}
	return(ERCANTPATHNULL);
}

/*
 * Coroutine of polygon merging to obtain a polygon and store it in the error that is being logged.
 */
void erc_antputpolygoninerror(INTBIG layer, TECHNOLOGY *tech, INTBIG *xv, INTBIG *yv, INTBIG count)
{
	REGISTER INTBIG i, lasti, lastx, lasty, x, y;

	for(i=0; i<count; i++)
	{
		if (i == 0) lasti = count-1; else lasti = i-1;
		lastx = xv[lasti];   lasty = yv[lasti];
		x = xv[i];           y = yv[i];
		addlinetoerror(erc_anterror, lastx, lasty, x, y);
	}
}

/*
 * Coroutine of polygon merging to obtain a polygon and accumulate its area.
 */
void erc_antgetpolygonarea(INTBIG layer, TECHNOLOGY *tech, INTBIG *xv, INTBIG *yv, INTBIG count)
{
	erc_antregionarea += areapoints(count, xv, yv);
#ifdef DEBUGANTENNA
	{
		REGISTER INTBIG i, lasti, lastx, lasty, x, y;

		for(i=0; i<count; i++)
		{
			if (i == 0) lasti = count-1; else lasti = i-1;
			lastx = xv[lasti];   lasty = yv[lasti];
			x = xv[i];           y = yv[i];
			asktool(us_tool, x_("show-line"), lastx, lasty, x, y, (INTBIG)erc_anttopcell);
		}
	}
#endif
}

INTBIG erc_antgethash(ANTENNAOBJ *ao)
{
	REGISTER INTBIG i, j;

	i = ao->depth + (INTBIG)ao->geom;
	for(j=0; j<ao->depth; j++)
		i += (INTBIG)ao->hierstack[j];
	return(abs(i % erc_antpathhashtablesize));
}

/*
 * Routine to return TRUE if antenna object "ao" is already in the list.
 */
BOOLEAN erc_anthaveantennaobj(ANTENNAOBJ *ao)
{
	REGISTER ANTENNAOBJ *oao;
	REGISTER INTBIG i, j, index;

	if (erc_antpathhashtablesize == 0) return(FALSE);
	index = erc_antgethash(ao);
	for(j=0; j<erc_antpathhashtablesize; j++)
	{
		oao = erc_antpathhashtable[index];
		if (oao == NOANTENNAOBJ) break;

		if (oao->geom == ao->geom && oao->depth == ao->depth)
		{
			for(i=0; i<oao->depth; i++)
				if (oao->hierstack[i] != ao->hierstack[i]) break;
			if (i >= oao->depth) return(TRUE);
		}
		index++;
		if (index >= erc_antpathhashtablesize) index = 0;		
	}
	return(FALSE);
}

/*
 * Routine to add antenna object "ao" to the list of antenna objects on this path.
 */
void erc_antaddantennaobj(ANTENNAOBJ *ao)
{
	REGISTER INTBIG i, oldsize, newsize;
	REGISTER ANTENNAOBJ **newtable, **oldtable, *oao;

	if (erc_antpathhashtablecount*2 >= erc_antpathhashtablesize)
	{
		/* expand the table */
		newsize = erc_antpathhashtablesize * 2;
		if (newsize <= erc_antpathhashtablecount*2) newsize = erc_antpathhashtablecount*2 + 50;
		newsize = pickprime(newsize);
		newtable = (ANTENNAOBJ **)emalloc(newsize * (sizeof (ANTENNAOBJ *)), erc_tool->cluster);
		if (newtable == 0) return;

		for(i=0; i<newsize; i++)
			newtable[i] = NOANTENNAOBJ;

		oldtable = erc_antpathhashtable;
		oldsize = erc_antpathhashtablesize;

		erc_antpathhashtablesize = newsize;
		erc_antpathhashtable = newtable;

		for(i=0; i<oldsize; i++)
		{
			oao = oldtable[i];
			if (oao == NOANTENNAOBJ) continue;
			erc_antinsertantennaobj(oao);
		}

		if (oldsize > 0) efree((CHAR *)oldtable);
	}
	erc_antinsertantennaobj(ao);
	erc_antpathhashtablecount++;
}

void erc_antinsertantennaobj(ANTENNAOBJ *ao)
{
	REGISTER ANTENNAOBJ *oao;
	REGISTER INTBIG j, index;

	index = erc_antgethash(ao);
	for(j=0; j<erc_antpathhashtablesize; j++)
	{
		oao = erc_antpathhashtable[index];
		if (oao == NOANTENNAOBJ)
		{
			erc_antpathhashtable[index] = ao;
			return;
		}

		index++;
		if (index >= erc_antpathhashtablesize) index = 0;		
	}
}

/*
 * Routine to load antenna object "ao" with the hierarchical stack in "stack" that is
 * "depth" deep.
 */
void erc_antloadantennaobj(ANTENNAOBJ *ao, INTBIG depth, NODEINST **stack)
{
	REGISTER INTBIG i;

	ao->depth = depth;
	if (depth > ao->total)
	{
		if (ao->total > 0) efree((CHAR *)ao->hierstack);
		ao->total = 0;
		ao->hierstack = (NODEINST **)emalloc(depth * (sizeof (NODEINST *)), erc_tool->cluster);
		if (ao->hierstack == 0) return;
		ao->total = depth;
	}
	for(i=0; i<depth; i++) ao->hierstack[i] = stack[i];
}

/*
 * Routine to allocate an antenna object and return it.
 */
ANTENNAOBJ *erc_antallocantennaobj(void)
{
	REGISTER ANTENNAOBJ *ao;

	if (erc_antantennaobjfree != NOANTENNAOBJ)
	{
		ao = erc_antantennaobjfree;
		erc_antantennaobjfree = ao->nextantennaobj;
	} else
	{
		ao = (ANTENNAOBJ *)emalloc(sizeof (ANTENNAOBJ), erc_tool->cluster);
		if (ao == 0) return (NOANTENNAOBJ);
		ao->total = 0;
	}
	return(ao);
}

/*
 * Routine to free the antenna object "ao".
 */
void erc_antfreeantennaobj(ANTENNAOBJ *ao)
{
	ao->nextantennaobj = erc_antantennaobjfree;
	erc_antantennaobjfree = ao;
}

/*
 * Routine to return the maximum antenna ratio for analysis at level "level".
 */
INTBIG erc_antratio(INTBIG level)
{
	REGISTER VARIABLE *var;
	REGISTER ARCPROTO *ap;
	REGISTER INTBIG fun, total, i, index;

	if (erc_anttech != erc_antcachedtech)
	{
		total = 0;
		for(ap = erc_anttech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto) total++;
		if (erc_antcachedtech != NOTECHNOLOGY)
			efree((CHAR *)erc_antcachedratios);
		erc_antcachedratios = (INTBIG *)emalloc(total * SIZEOFINTBIG, erc_tool->cluster);
		if (erc_antcachedratios == 0) return(ERCDEFMETALLIMIT);
		for(i=0; i<total; i++) erc_antcachedratios[i] = ERCDEFMETALLIMIT;

		var = getvalkey((INTBIG)erc_anttech, VTECHNOLOGY, VINTEGER|VISARRAY, erc_antennaarcratiokey);

		for(ap = erc_anttech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
		{
			fun = (ap->userbits&AFUNCTION) >> AFUNCTIONSH;
			switch (fun)
			{
				case APPOLY1:   index = ERCANTLEVEL1;     break;
				case APMETAL1:  index = ERCANTLEVEL1+1;   break;
				case APMETAL2:  index = ERCANTLEVEL1+2;   break;
				case APMETAL3:  index = ERCANTLEVEL1+3;   break;
				case APMETAL4:  index = ERCANTLEVEL1+4;   break;
				case APMETAL5:  index = ERCANTLEVEL1+5;   break;
				case APMETAL6:  index = ERCANTLEVEL1+6;   break;
				case APMETAL7:  index = ERCANTLEVEL1+7;   break;
				case APMETAL8:  index = ERCANTLEVEL1+8;   break;
				case APMETAL9:  index = ERCANTLEVEL1+9;   break;
				case APMETAL10: index = ERCANTLEVEL1+10;  break;
				case APMETAL11: index = ERCANTLEVEL1+11;  break;
				case APMETAL12: index = ERCANTLEVEL1+12;  break;
				default:        index = 0;                break;
			}
			if (index == 0) continue;
			if (index == ERCANTLEVEL1) erc_antcachedratios[index-2] = ERCDEFPOLYLIMIT; else
				erc_antcachedratios[index-2] = ERCDEFMETALLIMIT;
			if (var != NOVARIABLE && ap->arcindex < getlength(var))
				erc_antcachedratios[index-2] = ((INTBIG *)var->addr)[ap->arcindex];
		}
		erc_antcachedtech = erc_anttech;
	}
	return(erc_antcachedratios[level-2]);
}

/* Antenna Rules Options */
static DIALOGITEM erc_antruldialogitems[] =
{
 /*  1 */ {0, {204,208,228,288}, BUTTON, N_("OK")},
 /*  2 */ {0, {204,32,228,112}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {24,8,168,316}, SCROLL, x_("")},
 /*  4 */ {0, {4,4,20,160}, MESSAGE, N_("Arcs in technology:")},
 /*  5 */ {0, {4,160,20,316}, MESSAGE, x_("")},
 /*  6 */ {0, {172,216,188,312}, EDITTEXT, x_("")},
 /*  7 */ {0, {172,8,188,208}, MESSAGE, N_("Maximum antenna ratio:")}
};
static DIALOG erc_antruldialog = {{75,75,312,401}, N_("Antenna Rules Options"), 0, 7, erc_antruldialogitems, 0, 0};

/* special items for the "ERC Options" dialog: */
#define DANT_ARCLIST     3		/* List of arcs (scroll) */
#define DANT_TECHNAME    5		/* Current technology name (stat text) */
#define DANT_NEWRATIO    6		/* New ratio for selected arc (edit text) */

void erc_antoptionsdlog(void)
{
	REGISTER INTBIG itemHit, fun, amt, i, total, which, *original;
	REGISTER void *dia, *infstr;
	CHAR buf[50];
	REGISTER ARCPROTO *ap, **aplist;
	REGISTER VARIABLE *var;

	dia = DiaInitDialog(&erc_antruldialog);
	if (dia == 0) return;
	DiaSetText(dia, DANT_TECHNAME, el_curtech->techname);
	DiaInitTextDialog(dia, DANT_ARCLIST, DiaNullDlogList, DiaNullDlogItem,
		DiaNullDlogDone, -1, SCSELMOUSE|SCREPORT);
	var = getvalkey((INTBIG)el_curtech, VTECHNOLOGY, VINTEGER|VISARRAY, erc_antennaarcratiokey);
	total = 0;
	for(ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto) total++;
	if (total > 0)
	{
		original = (INTBIG *)emalloc(total * SIZEOFINTBIG, erc_tool->cluster);
		if (original == 0) return;
		aplist = (ARCPROTO **)emalloc(total * (sizeof (ARCPROTO *)), erc_tool->cluster);
		if (aplist == 0) return;
		for(i=0; i<total; i++) aplist[i] = NOARCPROTO;
	}
	i = 0;
	for(ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
	{
		if ((ap->userbits&ANOTUSED) != 0) continue;
		fun = (ap->userbits&AFUNCTION) >> AFUNCTIONSH;
		if (fun == APPOLY1) amt = ERCDEFPOLYLIMIT; else
			amt = ERCDEFMETALLIMIT;
		if (var != NOVARIABLE && ap->arcindex < getlength(var))
			amt = ((INTBIG *)var->addr)[ap->arcindex];
		ap->temp1 = amt;
		aplist[i] = ap;
		original[i] = amt;
		i++;
		infstr = initinfstr();
		formatinfstr(infstr, x_("%s (%ld)"), ap->protoname, amt);
		DiaStuffLine(dia, DANT_ARCLIST, returninfstr(infstr));
	}
	DiaSelectLine(dia, DANT_ARCLIST, 0);
	if (aplist[0] != NOARCPROTO)
	{
		esnprintf(buf, 50, x_("%ld"), aplist[0]->temp1);
		DiaSetText(dia, DANT_NEWRATIO, buf);
	}

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DANT_ARCLIST)
		{
			which = DiaGetCurLine(dia, DANT_ARCLIST);
			if (aplist[which] == NOARCPROTO) continue;
			esnprintf(buf, 50, x_("%ld"), aplist[which]->temp1);
			DiaSetText(dia, DANT_NEWRATIO, buf);
			continue;
		}
		if (itemHit == DANT_NEWRATIO)
		{
			which = DiaGetCurLine(dia, DANT_ARCLIST);
			if (aplist[which] == NOARCPROTO) continue;
			aplist[which]->temp1 = eatoi(DiaGetText(dia, DANT_NEWRATIO));
			infstr = initinfstr();
			formatinfstr(infstr, x_("%s (%ld)"), aplist[which]->protoname, aplist[which]->temp1);
			DiaSetScrollLine(dia, DANT_ARCLIST, which, returninfstr(infstr));
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		for(i=0; i<total; i++)
		{
			if (aplist[i] == NOARCPROTO) continue;
			if (aplist[i]->temp1 != original[i]) break;
		}
		if (i < total)
		{
			i = 0;
			for(ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
				original[i++] = ap->temp1;
			setvalkey((INTBIG)el_curtech, VTECHNOLOGY, erc_antennaarcratiokey,
				(INTBIG)original, VINTEGER|VISARRAY|(total<<VLENGTHSH));
		}
	}
	DiaDoneDialog(dia);
}

#endif  /* ERCTOOL - at top */
