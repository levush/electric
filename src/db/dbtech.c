/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dbtech.c
 * Database technology helper routines
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
#include "tecart.h"
#include "tecschem.h"
#include "tecmocmos.h"
#include "drc.h"
#include "efunction.h"
#include <math.h>

       POLYLOOP    tech_oneprocpolyloop;

/* for 3D height and thickness */
static TECHNOLOGY *db_3dcurtech = NOTECHNOLOGY;		/* technology whose height/thickness is cached */
static float      *db_3dheight;						/* cache for height of each layer in technology */
static float      *db_3dthickness;					/* cache for thickness of each layer in technology */
static INTBIG      db_3darraylength = 0;			/* maximum number of layers in height/thickness caches */

/* for finding equivalent layers */
static BOOLEAN    *db_equivtable = 0;
static void       *db_layertabmutex = 0;			/* mutex for layer equivalence table */

/* for converting pseudo-layers to real ones */
static TECHNOLOGY *db_convertpseudotech = NOTECHNOLOGY;
static INTBIG     *db_convertpseudoarray = 0;
static void       *db_convertpseudomutex = 0;		/* mutex for layer equivalence table */

/* keys to variables */
static INTBIG      db_techlayer3dheightkey;			/* key for "TECH_layer_3dheight" */
static INTBIG      db_techlayer3dthicknesskey;		/* key for "TECH_layer_3dthickness" */
       INTBIG      db_tech_node_width_offset_key;	/* key for "TECH_node_width_offset" */
       INTBIG      db_tech_layer_function_key;		/* key for "TECH_layer_function" */
       INTBIG      db_tech_layer_names_key;			/* key for "TECH_layer_names" */
       INTBIG      db_tech_arc_width_offset_key;	/* key for "TECH_arc_width_offset" */

/* cached variables */
static INTBIG    **tech_node_widoff = 0;			/* cache for "nodesizeoffset" */
static INTBIG    **tech_layer_function = 0;			/* cache for "layerfunction" */
static CHAR     ***tech_layer_names = 0;			/* cache for "layername" */
static INTBIG    **tech_arc_widoff = 0;				/* cache for "arcwidthoffset" */
static INTBIG    **tech_drcmaxdistances = 0;		/* cache for "maxdrcsurround" */
static INTBIG     *tech_drcwidelimit = 0;			/* cache for "drcmindistance" */
static INTBIG    **tech_drcminwidth = 0;			/* cache for "drcminwidth" */
static INTBIG    **tech_drcminnodesize = 0;			/* cache for "drcminnodesize" */
static CHAR     ***tech_drcminwidthrule = 0;		/* cache for "drcminwidth" */
static CHAR     ***tech_drcminnodesizerule = 0;		/* cache for "drcminnodesize" */
static INTBIG    **tech_drcconndistance = 0;		/* cache for "drcmindistance" */
static INTBIG    **tech_drcuncondistance = 0;		/* cache for "drcmindistance" */
static INTBIG    **tech_drcconndistancew = 0;		/* cache for "drcmindistance" */
static INTBIG    **tech_drcuncondistancew = 0;		/* cache for "drcmindistance" */
static INTBIG    **tech_drcconndistancem = 0;		/* cache for "drcmindistance" */
static INTBIG    **tech_drcuncondistancem = 0;		/* cache for "drcmindistance" */
static INTBIG    **tech_drcedgedistance = 0;		/* cache for "drcmindistance" */
static CHAR     ***tech_drccondistancerule = 0;		/* cache for "drcmindistance" */
static CHAR     ***tech_drcuncondistancerule = 0;	/* cache for "drcmindistance" */
static CHAR     ***tech_drccondistancewrule = 0;	/* cache for "drcmindistance" */
static CHAR     ***tech_drcuncondistancewrule = 0;	/* cache for "drcmindistance" */
static CHAR     ***tech_drccondistancemrule = 0;	/* cache for "drcmindistance" */
static CHAR     ***tech_drcuncondistancemrule = 0;	/* cache for "drcmindistance" */
static CHAR     ***tech_drcedgedistancerule = 0;	/* cache for "drcmindistance" */
#ifdef SURROUNDRULES
static INTBIG    **tech_drcsurroundlayerpairs = 0;	/* cache for "drcsurroundrules" */
static INTBIG    **tech_drcsurrounddistances = 0;	/* cache for "drcsurroundrules" */
static CHAR     ***tech_drcsurroundrule = 0;		/* cache for "drcsurroundrules" */
#endif

#define NOROUTINELIST ((ROUTINELIST *)-1)

typedef struct Iroutinelist
{
	void               (*routine)(void);
	INTBIG               count;
	INTBIG              *variablekeys;
	struct Iroutinelist *nextroutinelist;
} ROUTINELIST;
static ROUTINELIST *db_firsttechroutinecache = NOROUTINELIST;

/* shared prototypes */
void   tech_initmaxdrcsurround(void);

/* prototypes for local routines */
static INTBIG  tech_getdrcmindistance(TECHNOLOGY*, INTBIG, INTBIG, BOOLEAN, INTBIG, BOOLEAN, INTBIG*, CHAR **);
static void    tech_3ddefaultlayerheight(TECHNOLOGY *tech);
static INTBIG  tech_arcprotowidthoffset(ARCPROTO *ap, INTBIG lambda);
static void    tech_initnodesizeoffset(void);
static void    tech_initlayerfunction(void);
static void    tech_initlayername(void);
static void    tech_initarcwidthoffset(void);
static BOOLEAN tech_findresistors(NODEPROTO *np);

/*
 * Routine to free all memory associated with this module.
 */
void db_freetechnologymemory(void)
{
	REGISTER ROUTINELIST *rl;

	while (db_firsttechroutinecache != NOROUTINELIST)
	{
		rl = db_firsttechroutinecache;
		if (rl->count > 0) efree((CHAR *)rl->variablekeys);
		db_firsttechroutinecache = db_firsttechroutinecache->nextroutinelist;
		efree((CHAR *)rl);
	}
	if (db_equivtable != 0) efree((CHAR *)db_equivtable);
	if (db_convertpseudoarray != 0) efree((CHAR *)db_convertpseudoarray);

	if (tech_node_widoff != 0) efree((CHAR *)tech_node_widoff);
	if (tech_layer_function != 0) efree((CHAR *)tech_layer_function);
	if (tech_layer_names != 0) efree((CHAR *)tech_layer_names);
	if (tech_arc_widoff != 0) efree((CHAR *)tech_arc_widoff);
	if (tech_drcmaxdistances != 0) efree((CHAR *)tech_drcmaxdistances);
	if (tech_drcwidelimit != 0) efree((CHAR *)tech_drcwidelimit);
	if (tech_drcminwidth != 0) efree((CHAR *)tech_drcminwidth);
	if (tech_drcminnodesize != 0) efree((CHAR *)tech_drcminnodesize);
	if (tech_drcminwidthrule != 0) efree((CHAR *)tech_drcminwidthrule);
	if (tech_drcminnodesizerule != 0) efree((CHAR *)tech_drcminnodesizerule);
	if (tech_drcconndistance != 0) efree((CHAR *)tech_drcconndistance);
	if (tech_drcuncondistance != 0) efree((CHAR *)tech_drcuncondistance);
	if (tech_drcconndistancew != 0) efree((CHAR *)tech_drcconndistancew);
	if (tech_drcuncondistancew != 0) efree((CHAR *)tech_drcuncondistancew);
	if (tech_drcconndistancem != 0) efree((CHAR *)tech_drcconndistancem);
	if (tech_drcuncondistancem != 0) efree((CHAR *)tech_drcuncondistancem);
	if (tech_drcedgedistance != 0) efree((CHAR *)tech_drcedgedistance);
	if (tech_drccondistancerule != 0) efree((CHAR *)tech_drccondistancerule);
	if (tech_drcuncondistancerule != 0) efree((CHAR *)tech_drcuncondistancerule);
	if (tech_drccondistancewrule != 0) efree((CHAR *)tech_drccondistancewrule);
	if (tech_drcuncondistancewrule != 0) efree((CHAR *)tech_drcuncondistancewrule);
	if (tech_drccondistancemrule != 0) efree((CHAR *)tech_drccondistancemrule);
	if (tech_drcuncondistancemrule != 0) efree((CHAR *)tech_drcuncondistancemrule);
	if (tech_drcedgedistancerule != 0) efree((CHAR *)tech_drcedgedistancerule);
#ifdef SURROUNDRULES
	if (tech_drcsurroundlayerpairs != 0) efree((CHAR *)tech_drcsurroundlayerpairs);
	if (tech_drcsurrounddistances != 0) efree((CHAR *)tech_drcsurrounddistances);
	if (tech_drcsurroundrule != 0) efree((CHAR *)tech_drcsurroundrule);
#endif

	if (db_3darraylength > 0)
	{
		efree((CHAR *)db_3dheight);
		efree((CHAR *)db_3dthickness);
	}
}

void db_inittechnologies(void)
{
	(void)ensurevalidmutex(&db_layertabmutex, TRUE);
	(void)ensurevalidmutex(&db_convertpseudomutex, TRUE);
}

/******************** TECHNOLOGY ALLOCATION ********************/

/*
 * routine to allocate a technology from memory cluster "cluster" and return its
 * address.  The routine returns NOTECHNOLOGY if allocation fails.
 */
TECHNOLOGY *alloctechnology(CLUSTER *cluster)
{
	REGISTER TECHNOLOGY *tech;

	tech = (TECHNOLOGY *)emalloc((sizeof (TECHNOLOGY)), cluster);
	if (tech == 0) return((TECHNOLOGY *)db_error(DBNOMEM|DBALLOCTECHNOLOGY));
	tech->techname = NOSTRING;
	tech->techindex = 0;
	tech->deflambda = 2000;
	tech->firstnodeproto = NONODEPROTO;
	tech->firstarcproto = NOARCPROTO;
	tech->firstvar = NOVARIABLE;
	tech->numvar = 0;
	tech->parse = NOCOMCOMP;
	tech->cluster = cluster;
	tech->techdescript = NOSTRING;
	tech->init = 0;
	tech->term = 0;
	tech->setmode = 0;
	tech->request = 0;
	tech->nodepolys = 0;
	tech->nodeEpolys = 0;
	tech->shapenodepoly = 0;
	tech->shapeEnodepoly = 0;
	tech->allnodepolys = 0;
	tech->allnodeEpolys = 0;
	tech->nodesizeoffset = 0;
	tech->shapeportpoly = 0;
	tech->arcpolys = 0;
	tech->shapearcpoly = 0;
	tech->allarcpolys = 0;
	tech->arcwidthoffset = 0;
	tech->nexttechnology = NOTECHNOLOGY;
	tech->userbits = tech->temp1 = tech->temp2 = 0;
	tech->variables = (TECH_VARIABLES *)-1;
	tech->layercount = 0;
	tech->layers = NULL;
	tech->arcprotocount = 0;
	tech->arcprotos = NULL;
	tech->nodeprotocount = 0;
	tech->nodeprotos = NULL;
	return(tech);
}

/*
 * routine to return technology "tech" to the pool of free technologies
 */
void freetechnology(TECHNOLOGY *tech)
{
	REGISTER CLUSTER *clus;
	REGISTER INTBIG i, j;
	REGISTER NODEPROTO *np, *nextnp;
	REGISTER PORTPROTO *pp, *nextpp;
	REGISTER ARCPROTO *ap, *nextap;
	REGISTER TECH_NODES *tn;

	if (tech == NOTECHNOLOGY) return;
	for(np = tech->firstnodeproto; np != NONODEPROTO; np = nextnp)
	{
		nextnp = np->nextnodeproto;
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = nextpp)
		{
			nextpp = pp->nextportproto;
			efree((CHAR *)pp->protoname);
			freeportproto(pp);
		}
		efree((CHAR *)np->protoname);
		freenodeproto(np);
	}
	for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = nextap)
	{
		nextap = ap->nextarcproto;
		efree((CHAR *)ap->protoname);
		db_freearcproto(ap);
	}
	if (tech->numvar != 0) db_freevars(&tech->firstvar, &tech->numvar);
	efree((CHAR *)tech->techname);
	efree((CHAR *)tech->techdescript);
	if ((tech->userbits&STATICTECHNOLOGY) == 0)
	{
		for(i=0; i<tech->layercount; i++)
			efree((CHAR *)tech->layers[i]);
		efree((CHAR *)tech->layers);
		for(i=0; i<tech->arcprotocount; i++)
		{
			efree((CHAR *)tech->arcprotos[i]->arcname);
			efree((CHAR *)tech->arcprotos[i]->list);
			efree((CHAR *)tech->arcprotos[i]);
		}
		efree((CHAR *)tech->arcprotos);
		for(i=0; i<tech->nodeprotocount; i++)
		{
			tn = tech->nodeprotos[i];
			efree((CHAR *)tn->nodename);
			for(j=0; j<tn->portcount; j++)
				efree((CHAR *)tn->portlist[j].protoname);
			efree((CHAR *)tn->portlist);
			if (tn->special != SERPTRANS)
				efree((CHAR *)tn->layerlist); else
			{
				efree((CHAR *)tn->gra);
				efree((CHAR *)tn->ele);
			}
			efree((CHAR *)tn);
		}
		efree((CHAR *)tech->nodeprotos);
	}
	clus = tech->cluster;
	efree((CHAR *)tech);
	freecluster(clus);
}

/*
 * routine to insert technology "tech" into the global linked list and to
 * announce this change to all cached routines.
 */
void addtechnology(TECHNOLOGY *tech)
{
	REGISTER TECHNOLOGY *t, *lastt;
	REGISTER ROUTINELIST *rl;
	REGISTER INTBIG *newlam;
	REGISTER INTBIG i;
	REGISTER LIBRARY *lib;

	/* link it at the end of the list */
	for(t = el_technologies; t != NOTECHNOLOGY; t = t->nexttechnology) lastt = t;
	lastt->nexttechnology = tech;
	tech->nexttechnology = NOTECHNOLOGY;

	/* recount the number of technologies and renumber them */
	el_maxtech = 0;
	for(t = el_technologies; t != NOTECHNOLOGY; t = t->nexttechnology)
		t->techindex = el_maxtech++;

	/* adjust the "lambda" array in all libraries */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		newlam = emalloc(((el_maxtech+1) * SIZEOFINTBIG), db_cluster);
		if (newlam == 0)
		{
			(void)db_error(DBNOMEM|DBADDTECHNOLOGY);
			return;
		}
		for(i=0; i<el_maxtech-1; i++) newlam[i] = lib->lambda[i];
		newlam[el_maxtech-1] = tech->deflambda;
		newlam[el_maxtech] = -1;
		efree((CHAR *)lib->lambda);
		lib->lambda = newlam;
	}

	/* announce the change to the number of technologies */
	for(rl = db_firsttechroutinecache; rl != NOROUTINELIST; rl = rl->nextroutinelist)
		(*rl->routine)();
}

/*
 * routine to delete technology "tech" from the global list.  Returns
 * true on error.
 */
BOOLEAN killtechnology(TECHNOLOGY *tech)
{
	REGISTER TECHNOLOGY *t, *lastt;
	REGISTER ROUTINELIST *rl;
	REGISTER LIBRARY *lib;
	REGISTER INTBIG *newlam;
	REGISTER INTBIG i, j;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;

	/* cannot kill the generic technology */
	if (tech == gen_tech)
	{
		(void)db_error(DBLASTECH|DBKILLTECHNOLOGY);
		return(TRUE);
	}

	/* make sure there are no objects from this technology */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				if (ni->proto->primindex != 0 && ni->proto->tech == tech) break;
			if (ni != NONODEINST) break;
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
				if (ai->proto->tech == tech) break;
			if (ai != NOARCINST) break;
		}
		if (np != NONODEPROTO)
		{
			(void)db_error(DBTECINUSE|DBKILLTECHNOLOGY);
			return(TRUE);
		}
	}

	/* adjust the "lambda" array in all libraries */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		newlam = emalloc((el_maxtech * SIZEOFINTBIG), db_cluster);
		if (newlam == 0)
		{
			(void)db_error(DBNOMEM|DBKILLTECHNOLOGY);
			return(TRUE);
		}
		for(i = j = 0, t = el_technologies; t != NOTECHNOLOGY; t = t->nexttechnology, j++)
		{
			if (t == tech) continue;
			newlam[i++] = lib->lambda[j];
		}
		newlam[i] = -1;
		efree((CHAR *)lib->lambda);
		lib->lambda = newlam;
	}

	/* remove "tech" from linked list */
	lastt = NOTECHNOLOGY;
	for(t = el_technologies; t != NOTECHNOLOGY; t = t->nexttechnology)
	{
		if (t == tech) break;
		lastt = t;
	}
	if (lastt == NOTECHNOLOGY) el_technologies = tech->nexttechnology; else
		lastt->nexttechnology = tech->nexttechnology;

	/* deallocate the technology */
	if (tech->numvar != 0) db_freevars(&tech->firstvar, &tech->numvar);
	freetechnology(tech);

	/* recount the number of technologies and renumber them */
	el_maxtech = 0;
	for(t = el_technologies; t != NOTECHNOLOGY; t = t->nexttechnology)
		t->techindex = el_maxtech++;

	/* announce the change to the number of technologies */
	for(rl = db_firsttechroutinecache; rl != NOROUTINELIST; rl = rl->nextroutinelist)
		(*rl->routine)();
	return(0);
}

void telltech(TECHNOLOGY *tech, INTBIG count, CHAR *par[])
{
	if (tech->setmode == 0) return;
	(*tech->setmode)(count, par);
}

INTBIG asktech(TECHNOLOGY *tech, CHAR *command, ...)
{
	va_list ap;
	INTBIG result;

	if (tech->request == 0) return(0);
	var_start(ap, command);
	result = (*tech->request)(command, ap);
	va_end(ap);
	return(result);
}

/*
 * Routine to determine the default schematic technology, presuming that the
 * desired technology is "deftech".  This is the technology to really use when
 * the current technology is "schematics" and you want a layout technology.
 */
TECHNOLOGY *defschematictechnology(TECHNOLOGY *deftech)
{
	REGISTER TECHNOLOGY *t, *tech;
	REGISTER LIBRARY *lib;
	REGISTER VARIABLE *var;
	REGISTER NODEPROTO *np;
	REGISTER INTBIG i;

	/* see if the default schematics technology is already set */
	var = getval((INTBIG)sch_tech, VTECHNOLOGY, VSTRING, x_("TECH_layout_technology"));
	if (var != NOVARIABLE)
	{
		tech = gettechnology((CHAR *)var->addr);
		if (tech != NOTECHNOLOGY) return(tech);
	}

	/* look at all circuitry and see which technologies are in use */
	for(t = el_technologies; t != NOTECHNOLOGY; t = t->nexttechnology)
		t->temp2 = 0;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			if (np->tech != NOTECHNOLOGY) np->tech->temp2++;

	/* ignore nonlayout technologies */
	for(t = el_technologies; t != NOTECHNOLOGY; t = t->nexttechnology)
	{
		if (t == sch_tech || t == gen_tech ||
			(t->userbits&(NONELECTRICAL|NOPRIMTECHNOLOGY)) != 0) t->temp2 = -1;
	}

	/* if the desired technology is possible, use it */
	if (deftech->temp2 >= 0) return(deftech);

	/* figure out the most popular technology */
	i = -1;
	tech = NOTECHNOLOGY;
	for(t = el_technologies; t != NOTECHNOLOGY; t = t->nexttechnology)
	{
		if (t->temp2 <= i) continue;
		i = t->temp2;
		tech = t;
	}
	if (i == 0) return(gettechnology(DEFTECH));
	return(tech);
}

/******************** VARIABLE CACHING ********************/

/* this should be called whenever "DRC_min_unconnected_distances" changes!!! */

/*
 * routine to initialize the database variables "DRC_max_distances",
 * "DRC_min_unconnected_distances", "DRC_min_connected_distances", and "DRC_min_width".
 * This is called once at initialization and again whenever the arrays are changed.
 */
void tech_initmaxdrcsurround(void)
{
	REGISTER VARIABLE *var;
	REGISTER TECHNOLOGY *t;
	REGISTER INTBIG j, total, l, *dist, m;
	INTBIG edge;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	if (tech_drcmaxdistances != 0) efree((CHAR *)tech_drcmaxdistances);
	if (tech_drcwidelimit != 0) efree((CHAR *)tech_drcwidelimit);
	if (tech_drcminwidth != 0) efree((CHAR *)tech_drcminwidth);
	if (tech_drcminnodesize != 0) efree((CHAR *)tech_drcminnodesize);
	if (tech_drcminwidthrule != 0) efree((CHAR *)tech_drcminwidthrule);
	if (tech_drcminnodesizerule != 0) efree((CHAR *)tech_drcminnodesizerule);
	if (tech_drcconndistance != 0) efree((CHAR *)tech_drcconndistance);
	if (tech_drcuncondistance != 0) efree((CHAR *)tech_drcuncondistance);
	if (tech_drcconndistancew != 0) efree((CHAR *)tech_drcconndistancew);
	if (tech_drcuncondistancew != 0) efree((CHAR *)tech_drcuncondistancew);
	if (tech_drcconndistancem != 0) efree((CHAR *)tech_drcconndistancem);
	if (tech_drcuncondistancem != 0) efree((CHAR *)tech_drcuncondistancem);
	if (tech_drcedgedistance != 0) efree((CHAR *)tech_drcedgedistance);
	if (tech_drccondistancerule != 0) efree((CHAR *)tech_drccondistancerule);
	if (tech_drcuncondistancerule != 0) efree((CHAR *)tech_drcuncondistancerule);
	if (tech_drccondistancewrule != 0) efree((CHAR *)tech_drccondistancewrule);
	if (tech_drcuncondistancewrule != 0) efree((CHAR *)tech_drcuncondistancewrule);
	if (tech_drccondistancemrule != 0) efree((CHAR *)tech_drccondistancemrule);
	if (tech_drcuncondistancemrule != 0) efree((CHAR *)tech_drcuncondistancemrule);
	if (tech_drcedgedistancerule != 0) efree((CHAR *)tech_drcedgedistancerule);
#ifdef SURROUNDRULES
	if (tech_drcsurroundrule != 0) efree((CHAR *)tech_drcsurroundrule);
	if (tech_drcsurroundlayerpairs != 0) efree((CHAR *)tech_drcsurroundlayerpairs);
	if (tech_drcsurrounddistances != 0) efree((CHAR *)tech_drcsurrounddistances);
#endif

	tech_drcmaxdistances = (INTBIG **)emalloc((el_maxtech * (sizeof (INTBIG *))), db_cluster);
	if (tech_drcmaxdistances == 0) return;
	tech_drcwidelimit = (INTBIG *)emalloc((el_maxtech * SIZEOFINTBIG), db_cluster);
	if (tech_drcwidelimit == 0) return;
	tech_drcminwidth = (INTBIG **)emalloc((el_maxtech * (sizeof (INTBIG *))), db_cluster);
	if (tech_drcminwidth == 0) return;
	tech_drcminnodesize = (INTBIG **)emalloc((el_maxtech * (sizeof (INTBIG *))), db_cluster);
	if (tech_drcminnodesize == 0) return;
	tech_drcminwidthrule = (CHAR ***)emalloc((el_maxtech * (sizeof (CHAR **))), db_cluster);
	if (tech_drcminwidthrule == 0) return;
	tech_drcminnodesizerule = (CHAR ***)emalloc((el_maxtech * (sizeof (CHAR **))), db_cluster);
	if (tech_drcminnodesizerule == 0) return;
	tech_drcconndistance = (INTBIG **)emalloc((el_maxtech * (sizeof (INTBIG *))), db_cluster);
	if (tech_drcconndistance == 0) return;
	tech_drcuncondistance = (INTBIG **)emalloc((el_maxtech * (sizeof (INTBIG *))), db_cluster);
	if (tech_drcuncondistance == 0) return;
	tech_drcconndistancew = (INTBIG **)emalloc((el_maxtech * (sizeof (INTBIG *))), db_cluster);
	if (tech_drcconndistancew == 0) return;
	tech_drcuncondistancew = (INTBIG **)emalloc((el_maxtech * (sizeof (INTBIG *))), db_cluster);
	if (tech_drcuncondistancew == 0) return;
	tech_drcconndistancem = (INTBIG **)emalloc((el_maxtech * (sizeof (INTBIG *))), db_cluster);
	if (tech_drcconndistancem == 0) return;
	tech_drcuncondistancem = (INTBIG **)emalloc((el_maxtech * (sizeof (INTBIG *))), db_cluster);
	if (tech_drcuncondistancem == 0) return;
	tech_drcedgedistance = (INTBIG **)emalloc((el_maxtech * (sizeof (INTBIG *))), db_cluster);
	if (tech_drcedgedistance == 0) return;
	tech_drccondistancerule = (CHAR ***)emalloc((el_maxtech * (sizeof (CHAR **))), db_cluster);
	if (tech_drccondistancerule == 0) return;
	tech_drcuncondistancerule = (CHAR ***)emalloc((el_maxtech * (sizeof (CHAR **))), db_cluster);
	if (tech_drcuncondistancerule == 0) return;
	tech_drccondistancewrule = (CHAR ***)emalloc((el_maxtech * (sizeof (CHAR **))), db_cluster);
	if (tech_drccondistancewrule == 0) return;
	tech_drcuncondistancewrule = (CHAR ***)emalloc((el_maxtech * (sizeof (CHAR **))), db_cluster);
	if (tech_drcuncondistancewrule == 0) return;
	tech_drccondistancemrule = (CHAR ***)emalloc((el_maxtech * (sizeof (CHAR **))), db_cluster);
	if (tech_drccondistancemrule == 0) return;
	tech_drcuncondistancemrule = (CHAR ***)emalloc((el_maxtech * (sizeof (CHAR **))), db_cluster);
	if (tech_drcuncondistancemrule == 0) return;
	tech_drcedgedistancerule = (CHAR ***)emalloc((el_maxtech * (sizeof (CHAR **))), db_cluster);
	if (tech_drcedgedistancerule == 0) return;
#ifdef SURROUNDRULES
	tech_drcsurroundlayerpairs = (INTBIG **)emalloc((el_maxtech * (sizeof (INTBIG *))), db_cluster);
	if (tech_drcsurroundlayerpairs == 0) return;
	tech_drcsurrounddistances = (INTBIG **)emalloc((el_maxtech * (sizeof (INTBIG *))), db_cluster);
	if (tech_drcsurrounddistances == 0) return;
	tech_drcsurroundrule = (CHAR ***)emalloc((el_maxtech * (sizeof (CHAR **))), db_cluster);
	if (tech_drcsurroundrule == 0) return;
#endif

	for(t = el_technologies; t != NOTECHNOLOGY; t = t->nexttechnology)
	{
		tech_drcmaxdistances[t->techindex] = 0;
		tech_drcwidelimit[t->techindex] = -1;
		tech_drcminwidth[t->techindex] = 0;
		tech_drcminnodesize[t->techindex] = 0;
		tech_drcminwidthrule[t->techindex] = 0;
		tech_drcminnodesizerule[t->techindex] = 0;
		tech_drcconndistance[t->techindex] = 0;
		tech_drcuncondistance[t->techindex] = 0;
		tech_drcconndistancew[t->techindex] = 0;
		tech_drcuncondistancew[t->techindex] = 0;
		tech_drcconndistancem[t->techindex] = 0;
		tech_drcuncondistancem[t->techindex] = 0;
		tech_drcedgedistance[t->techindex] = 0;
		tech_drccondistancerule[t->techindex] = 0;
		tech_drcuncondistancerule[t->techindex] = 0;
		tech_drccondistancewrule[t->techindex] = 0;
		tech_drcuncondistancewrule[t->techindex] = 0;
		tech_drccondistancemrule[t->techindex] = 0;
		tech_drcuncondistancemrule[t->techindex] = 0;
		tech_drcedgedistancerule[t->techindex] = 0;
#ifdef SURROUNDRULES
		tech_drcsurroundrule[t->techindex] = 0;
		tech_drcsurroundlayerpairs[t->techindex] = 0;
		tech_drcsurrounddistances[t->techindex] = 0;
#endif

		var = getvalkey((INTBIG)t, VTECHNOLOGY, VFRACT, dr_wide_limitkey);
		if (var != NOVARIABLE) tech_drcwidelimit[t->techindex] = var->addr;
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VFRACT|VISARRAY, dr_min_widthkey);
		if (var != NOVARIABLE) tech_drcminwidth[t->techindex] = (INTBIG *)var->addr;
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VFRACT|VISARRAY, dr_min_node_sizekey);
		if (var != NOVARIABLE) tech_drcminnodesize[t->techindex] = (INTBIG *)var->addr;
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VSTRING|VISARRAY, dr_min_width_rulekey);
		if (var != NOVARIABLE) tech_drcminwidthrule[t->techindex] = (CHAR **)var->addr;
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VSTRING|VISARRAY, dr_min_node_size_rulekey);
		if (var != NOVARIABLE) tech_drcminnodesizerule[t->techindex] = (CHAR **)var->addr;
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VFRACT|VISARRAY, dr_connected_distanceskey);
		if (var != NOVARIABLE) tech_drcconndistance[t->techindex] = (INTBIG *)var->addr;
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VFRACT|VISARRAY, dr_unconnected_distanceskey);
		if (var != NOVARIABLE) tech_drcuncondistance[t->techindex] = (INTBIG *)var->addr;
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VFRACT|VISARRAY, dr_connected_distancesWkey);
		if (var != NOVARIABLE) tech_drcconndistancew[t->techindex] = (INTBIG *)var->addr;
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VFRACT|VISARRAY, dr_unconnected_distancesWkey);
		if (var != NOVARIABLE) tech_drcuncondistancew[t->techindex] = (INTBIG *)var->addr;
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VFRACT|VISARRAY, dr_connected_distancesMkey);
		if (var != NOVARIABLE) tech_drcconndistancem[t->techindex] = (INTBIG *)var->addr;
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VFRACT|VISARRAY, dr_unconnected_distancesMkey);
		if (var != NOVARIABLE) tech_drcuncondistancem[t->techindex] = (INTBIG *)var->addr;
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VFRACT|VISARRAY, dr_edge_distanceskey);
		if (var != NOVARIABLE) tech_drcedgedistance[t->techindex] = (INTBIG *)var->addr;
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VSTRING|VISARRAY, dr_connected_distances_rulekey);
		if (var != NOVARIABLE) tech_drccondistancerule[t->techindex] = (CHAR **)var->addr;
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VSTRING|VISARRAY, dr_unconnected_distances_rulekey);
		if (var != NOVARIABLE) tech_drcuncondistancerule[t->techindex] = (CHAR **)var->addr;
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VSTRING|VISARRAY, dr_connected_distancesW_rulekey);
		if (var != NOVARIABLE) tech_drccondistancewrule[t->techindex] = (CHAR **)var->addr;
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VSTRING|VISARRAY, dr_unconnected_distancesW_rulekey);
		if (var != NOVARIABLE) tech_drcuncondistancewrule[t->techindex] = (CHAR **)var->addr;
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VSTRING|VISARRAY, dr_connected_distancesM_rulekey);
		if (var != NOVARIABLE) tech_drccondistancemrule[t->techindex] = (CHAR **)var->addr;
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VSTRING|VISARRAY, dr_unconnected_distancesM_rulekey);
		if (var != NOVARIABLE) tech_drcuncondistancemrule[t->techindex] = (CHAR **)var->addr;
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VSTRING|VISARRAY, dr_edge_distances_rulekey);
		if (var != NOVARIABLE) tech_drcedgedistancerule[t->techindex] = (CHAR **)var->addr;
#ifdef SURROUNDRULES
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VINTEGER|VISARRAY, dr_surround_layer_pairskey);
		if (var != NOVARIABLE) tech_drcsurroundlayerpairs[t->techindex] = (INTBIG *)var->addr;
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VFRACT|VISARRAY, dr_surround_distanceskey);
		if (var != NOVARIABLE) tech_drcsurrounddistances[t->techindex] = (INTBIG *)var->addr;
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VSTRING|VISARRAY, dr_surround_rulekey);
		if (var != NOVARIABLE) tech_drcsurroundrule[t->techindex] = (CHAR **)var->addr;
#endif

		/* compute max distances if there are any there */
		if (tech_drcuncondistance[t->techindex] == 0) continue;
		total = t->layercount;
		dist = emalloc((total * SIZEOFINTBIG), el_tempcluster);
		if (dist == 0) continue;
		for(l=0; l<total; l++)
		{
			m = XX;
			for(j=0; j<total; j++)
				m = maxi(m, tech_getdrcmindistance(t, l, j, FALSE, 1, FALSE, &edge, 0));
			dist[l] = m;
		}
		nextchangequiet();
		if (setvalkey((INTBIG)t, VTECHNOLOGY, dr_max_distanceskey, (INTBIG)dist,
			VFRACT|VDONTSAVE|VISARRAY|(total<<VLENGTHSH)) != NOVARIABLE)
		{
			var = getvalkey((INTBIG)t, VTECHNOLOGY, VFRACT|VISARRAY, dr_max_distanceskey);
			if (var != NOVARIABLE) tech_drcmaxdistances[t->techindex] = (INTBIG *)var->addr;
		}
		efree((CHAR *)dist);
	}
}

/*
 * routine to initialize the database variable "TECH_node_width_offset".  This
 * is called once at initialization and again whenever the array is changed.
 */
void tech_initnodesizeoffset(void)
{
	REGISTER VARIABLE *var;
	REGISTER TECHNOLOGY *tech;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	/* free the old cache list if it exists */
	if (tech_node_widoff != 0) efree((CHAR *)tech_node_widoff);

	/* allocate a new cache list */
	tech_node_widoff = (INTBIG **)emalloc((el_maxtech * (sizeof (INTBIG *))), db_cluster);
	if (tech_node_widoff == 0) return;

	/* load the cache list */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		var = getvalkey((INTBIG)tech, VTECHNOLOGY, VFRACT|VISARRAY, db_tech_node_width_offset_key);
		tech_node_widoff[tech->techindex] = (var == NOVARIABLE ? 0 : (INTBIG *)var->addr);
	}
}

/*
 * routine to initialize the database variable "TECH_layer_function".  This
 * is called once at initialization and again whenever the array is changed.
 */
void tech_initlayerfunction(void)
{
	REGISTER VARIABLE *var;
	REGISTER TECHNOLOGY *t;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	if (tech_layer_function != 0) efree((CHAR *)tech_layer_function);

	tech_layer_function = (INTBIG **)emalloc((el_maxtech * (sizeof (INTBIG *))), db_cluster);
	if (tech_layer_function == 0) return;

	for(t = el_technologies; t != NOTECHNOLOGY; t = t->nexttechnology)
	{
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VINTEGER|VISARRAY, db_tech_layer_function_key);
		tech_layer_function[t->techindex] = (var == NOVARIABLE ? 0 : (INTBIG *)var->addr);
	}
}

/*
 * routine to initialize the database variable "TECH_layer_names".  This
 * is called once at initialization and again whenever the array is changed.
 */
void tech_initlayername(void)
{
	REGISTER VARIABLE *var;
	REGISTER TECHNOLOGY *t;

	if (tech_layer_names != 0) efree((CHAR *)tech_layer_names);

	tech_layer_names = (CHAR ***)emalloc((el_maxtech * (sizeof (CHAR **))), db_cluster);
	if (tech_layer_names == 0) return;

	for(t = el_technologies; t != NOTECHNOLOGY; t = t->nexttechnology)
	{
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VSTRING|VISARRAY, db_tech_layer_names_key);
		tech_layer_names[t->techindex] = (var == NOVARIABLE ? 0 : (CHAR **)var->addr);
	}
}

/*
 * routine to initialize the database variable "TECH_arc_width_offset".  This
 * is called once at initialization and again whenever the array is changed.
 */
void tech_initarcwidthoffset(void)
{
	REGISTER VARIABLE *var;
	REGISTER TECHNOLOGY *tech;

	if (tech_arc_widoff != 0) efree((CHAR *)tech_arc_widoff);

	tech_arc_widoff = (INTBIG **)emalloc((el_maxtech * (sizeof (INTBIG *))), db_cluster);
	if (tech_arc_widoff == 0) return;

	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		var = getvalkey((INTBIG)tech, VTECHNOLOGY, VFRACT|VISARRAY, db_tech_arc_width_offset_key);
		tech_arc_widoff[tech->techindex] = (var == NOVARIABLE ? 0 : (INTBIG *)var->addr);
	}
}

/*
 * routine to register function "proc" in a list that will be invoked
 * whenever the number of technologies changes.  Also the "count"
 * variable keys in "variablekeys" will be checked for updates, such that
 * whenever those keys are changed on any technology, the routine will
 * be invoked.
 */
void registertechnologycache(void (*proc)(void), INTBIG count, INTBIG *variablekeys)
{
	REGISTER ROUTINELIST *rl;
	REGISTER INTBIG i;

	/* allocate a ROUTINELIST object */
	rl = (ROUTINELIST *)emalloc(sizeof (ROUTINELIST), db_cluster);
	if (rl == 0) return;

	/* put this object at the start */
	rl->nextroutinelist = db_firsttechroutinecache;
	db_firsttechroutinecache = rl;

	/* insert the data and run the routine */
	rl->routine = proc;
	rl->count = count;
	if (count > 0)
	{
		rl->variablekeys = (INTBIG *)emalloc(count * SIZEOFINTBIG, db_cluster);
		for(i=0; i<count; i++) rl->variablekeys[i] = variablekeys[i];
	}
	(*proc)();
}

/*
 * routine called once at initialization to register the database
 * functions that cache technology-related variables
 */
void db_inittechcache(void)
{
	INTBIG keys[20];

	keys[0] = dr_wide_limitkey;
	keys[1] = dr_min_widthkey;
	keys[2] = dr_min_width_rulekey;
	keys[3] = dr_min_node_sizekey;
	keys[4] = dr_min_node_size_rulekey;
	keys[5] = dr_connected_distanceskey;
	keys[6] = dr_connected_distances_rulekey;
	keys[7] = dr_unconnected_distanceskey;
	keys[8] = dr_unconnected_distances_rulekey;
	keys[9] = dr_connected_distancesWkey;
	keys[10] = dr_connected_distancesW_rulekey;
	keys[11] = dr_unconnected_distancesWkey;
	keys[12] = dr_unconnected_distancesW_rulekey;
	keys[13] = dr_connected_distancesMkey;
	keys[14] = dr_connected_distancesM_rulekey;
	keys[15] = dr_unconnected_distancesMkey;
	keys[16] = dr_unconnected_distancesM_rulekey;
	keys[17] = dr_edge_distanceskey;
	keys[18] = dr_edge_distances_rulekey;
	keys[19] = dr_max_distanceskey;

	registertechnologycache(tech_initmaxdrcsurround, 20, keys);

	keys[0] = db_tech_node_width_offset_key;
	registertechnologycache(tech_initnodesizeoffset, 1, keys);

	keys[0] = db_tech_layer_function_key;
	registertechnologycache(tech_initlayerfunction, 1, keys);

	keys[0] = db_tech_layer_names_key;
	registertechnologycache(tech_initlayername, 1, keys);

	keys[0] = db_tech_arc_width_offset_key;
	registertechnologycache(tech_initarcwidthoffset, 1, keys);
}

/*
 * Routine called when key "keyval" on a technology changes.
 */
void changedtechnologyvariable(INTBIG keyval)
{
	REGISTER ROUTINELIST *rl;
	REGISTER INTBIG i;

	for(rl = db_firsttechroutinecache; rl != NOROUTINELIST; rl = rl->nextroutinelist)
	{
		for(i=0; i<rl->count; i++)
		{
			if (rl->variablekeys[i] == keyval)
			{
				(*rl->routine)();
				return;
			}
		}
	}
}

/******************** NODEINST DESCRIPTION ********************/

/*
 * routine to report the number of distinct graphical polygons used to compose
 * primitive nodeinst "ni".  If "reasonable" is nonzero, it is loaded with
 * a smaller number of polygons (when multicut contacts grow too large, then
 * a reasonable number of polygons is returned instead of the true number).
 */
INTBIG nodepolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win)
{
	REGISTER NODEPROTO *np;
	REGISTER INTBIG count;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	np = ni->proto;
	tech_oneprocpolyloop.curwindowpart = win;
	if (np->primindex == 0)
	{
		if (reasonable != 0) *reasonable = 0;
		return(0);
	}

	/* if the technology has its own routine, use it */
	if (np->tech->nodepolys != 0)
	{
		count = (*(np->tech->nodepolys))(ni, reasonable, win);
		return(count);
	}

	return(tech_nodepolys(ni, reasonable, win, &tech_oneprocpolyloop));
}

INTBIG tech_nodepolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win, POLYLOOP *pl)
{
	REGISTER INTBIG pindex, count, reasonablecount, cutcount, displayablepolys;
	INTBIG fewer;
	TECH_NODES *thistn;
	REGISTER NODEPROTO *np;

	np = ni->proto;
	pindex = np->primindex;
	if (pindex == 0) return(0);

	thistn = np->tech->nodeprotos[pindex-1];
	reasonablecount = count = thistn->layercount;
	switch (thistn->special)
	{
		case MULTICUT:
			cutcount = tech_moscutcount(ni, thistn->f1, thistn->f2, thistn->f3, thistn->f4, &fewer, pl);
			count += cutcount - 1;
			reasonablecount += fewer - 1;
			break;
		case SERPTRANS:
			reasonablecount = count = tech_inittrans(count, ni, pl);
			break;
	}

	/* zero the count if this node is not to be displayed */
	if ((ni->userbits&WIPED) != 0) reasonablecount = count = 0; else
	{
		/* zero the count if this node erases when connected to 1 or 2 arcs */
		if ((np->userbits&WIPEON1OR2) != 0)
		{
			if (tech_pinusecount(ni, win)) reasonablecount = count = 0;
		}
	}

	/* add in displayable variables */
	pl->realpolys = count;
	displayablepolys = tech_displayablenvars(ni, win, pl);
	count += displayablepolys;
	reasonablecount += displayablepolys;
	if (reasonable != 0) *reasonable = reasonablecount;
	return(count);
}

/*
 * routine to report the number of distinct electrical polygons used to
 * compose primitive nodeinst "ni".  If "reasonable" is nonzero, it is loaded with
 * a smaller number of polygons (when multicut contacts grow too large, then
 * a reasonable number of polygons is returned instead of the true number).
 */
INTBIG nodeEpolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win)
{
	REGISTER INTBIG count;

	tech_oneprocpolyloop.curwindowpart = win;
	if (ni->proto->primindex == 0)
	{
		if (reasonable != 0) *reasonable = 0;
		return(0);
	}

	/* if the technology has its own routine, use it */
	if (ni->proto->tech->nodeEpolys != 0)
	{
		count = (*(ni->proto->tech->nodeEpolys))(ni, reasonable, win);
		return(count);
	}

	return(tech_nodeEpolys(ni, reasonable, win, &tech_oneprocpolyloop));
}

INTBIG tech_nodeEpolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win, POLYLOOP *pl)
{
	REGISTER INTBIG pindex, count, reasonablecount, cutcount;
	INTBIG fewer;
	REGISTER TECH_NODES *thistn;

	pindex = ni->proto->primindex;
	if (pindex == 0) return(0);

	thistn = ni->proto->tech->nodeprotos[pindex-1];
	reasonablecount = count = thistn->layercount;
	switch (thistn->special)
	{
		case MULTICUT:
			cutcount = tech_moscutcount(ni, thistn->f1, thistn->f2, thistn->f3, thistn->f4, &fewer, pl);
			count += cutcount - 1;
			reasonablecount += fewer - 1;
			break;
		case SERPTRANS:
			reasonablecount = count = tech_inittrans(thistn->f1, ni, pl);
			break;
	}
	if (reasonable != 0) *reasonable = reasonablecount;
	return(count);
}

/*
 * routine to report the shape of graphical polygon "box" of primitive
 * nodeinst "ni".  The polygon is returned in "poly".
 */
void shapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly)
{
	REGISTER NODEPROTO *np;

	np = ni->proto;
	poly->tech = np->tech;
	if (np->primindex == 0) return;

	/* if the technology has its own routine, use it */
	if (np->tech->shapenodepoly != 0)
	{
		(*(np->tech->shapenodepoly))(ni, box, poly);
		return;
	}

	tech_shapenodepoly(ni, box, poly, &tech_oneprocpolyloop);
}

void tech_shapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl)
{
	TECH_POLYGON *lay;
	REGISTER TECH_PORTS *p;
	REGISTER INTBIG pindex, count, i, lambda;
	REGISTER NODEPROTO *np;
	INTBIG localpoints[8];
	REGISTER TECH_NODES *thistn;

	np = ni->proto;
	poly->tech = np->tech;
	pindex = np->primindex;
	if (pindex == 0) return;

	/* handle displayable variables */
	if (box >= pl->realpolys)
	{
		(void)tech_filldisplayablenvar(ni, poly, pl->curwindowpart, 0, pl);
		return;
	}

	thistn = np->tech->nodeprotos[pindex-1];
	lambda = lambdaofnode(ni);
	switch (thistn->special)
	{
		case SERPTRANS:
			if (box > 1 || (ni->userbits&NSHORT) == 0) p = (TECH_PORTS *)0; else
				p = thistn->portlist;
			tech_filltrans(poly, &lay, thistn->gra, ni, lambda, box, p, pl);
			break;

		case MULTICUT:
			count = thistn->layercount - 1;
			if (box >= count)
			{
				lay = &thistn->layerlist[count];
				for(i=0; i<8; i++) localpoints[i] = lay->points[i];
				tech_moscutpoly(ni, box-count, localpoints, pl);
				poly->layer = lay->layernum;
				if (lay->style == FILLEDRECT || lay->style == CLOSEDRECT)
				{
					if (poly->limit < 2) (void)extendpolygon(poly, 2);
					subrange(ni->lowx, ni->highx, localpoints[0], localpoints[1],
						localpoints[4], localpoints[5], &poly->xv[0], &poly->xv[1], lambda);
					subrange(ni->lowy, ni->highy, localpoints[2], localpoints[3],
						localpoints[6], localpoints[7], &poly->yv[0], &poly->yv[1], lambda);
					poly->count = 2;
				} else
				{
					if (poly->limit < 4) (void)extendpolygon(poly, 4);
					subrange(ni->lowx, ni->highx, localpoints[0], localpoints[1],
						localpoints[4], localpoints[5], &poly->xv[0], &poly->xv[2], lambda);
					subrange(ni->lowy, ni->highy, localpoints[2], localpoints[3],
						localpoints[6], localpoints[7], &poly->yv[0], &poly->yv[1], lambda);
					poly->xv[1] = poly->xv[0];   poly->xv[3] = poly->xv[2];
					poly->yv[3] = poly->yv[0];   poly->yv[2] = poly->yv[1];
					poly->count = 4;
				}
				poly->style = lay->style;
				break;
			}

		default:
			lay = &thistn->layerlist[box];
			tech_fillpoly(poly, lay, ni, lambda, FILLED);
			break;
	}

	poly->desc = np->tech->layers[poly->layer];
}

/*
 * routine to report the shape of electrical polygon "box" of primitive
 * nodeinst "ni".  The polygon is returned in "poly".
 */
void shapeEnodepoly(NODEINST *ni, INTBIG box, POLYGON *poly)
{
	REGISTER NODEPROTO *np;

	np = ni->proto;
	if (np->primindex == 0) return;
	poly->tech = np->tech;

	/* if the technology has its own routine, use it */
	if (np->tech->shapeEnodepoly != 0)
	{
		(*(np->tech->shapeEnodepoly))(ni, box, poly);
		return;
	}

	tech_shapeEnodepoly(ni, box, poly, &tech_oneprocpolyloop);
}

void tech_shapeEnodepoly(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl)
{
	TECH_POLYGON *lay;
	REGISTER INTBIG pindex, count;
	REGISTER NODEPROTO *np;
	REGISTER INTBIG lambda, i;
	INTBIG localpoints[8];
	REGISTER TECH_NODES *thistn;

	np = ni->proto;
	pindex = np->primindex;
	if (pindex == 0) return;
	poly->tech = np->tech;

	thistn = np->tech->nodeprotos[pindex-1];
	lambda = lambdaofnode(ni);
	switch (thistn->special)
	{
		case MOSTRANS:
			lay = &((TECH_POLYGON *)thistn->ele)[box];
			tech_fillpoly(poly, lay, ni, lambda, FILLED);
			break;

		case SERPTRANS:
			tech_filltrans(poly, &lay, thistn->ele, ni, lambda, box, (TECH_PORTS *)0, pl);
			break;

		case MULTICUT:
			count = thistn->layercount - 1;
			if (box >= count)
			{
				lay = &thistn->layerlist[count];
				for(i=0; i<8; i++) localpoints[i] = lay->points[i];
				tech_moscutpoly(ni, box-count, localpoints, pl);
				poly->layer = lay->layernum;
				if (poly->limit < 2) (void)extendpolygon(poly, 2);
				subrange(ni->lowx, ni->highx, localpoints[0], localpoints[1],
					localpoints[4], localpoints[5], &poly->xv[0], &poly->xv[1], lambda);
				subrange(ni->lowy, ni->highy, localpoints[2], localpoints[3],
					localpoints[6], localpoints[7], &poly->yv[0], &poly->yv[1], lambda);
				poly->count = 2;
				poly->style = lay->style;
				break;
			}

		default:
			lay = &thistn->layerlist[box];
			tech_fillpoly(poly, lay, ni, lambda, FILLED);
			break;
	}

	/* handle port prototype association */
	if (lay->portnum < 0) poly->portproto = NOPORTPROTO; else
		poly->portproto = thistn->portlist[lay->portnum].addr;

	poly->desc = np->tech->layers[poly->layer];
}

/* 
 * Routine to fill the polygon list "plist" with all polygons associated with node "ni"
 * when displayed in window "win" (but excluding displayable variables).  If "onlyreasonable"
 * is true, only count a reasonable number of polygons (which means, exclude center cuts in
 * large multi-cut contacts).  Returns the number of polygons (negative on error).
 */
INTBIG allnodepolys(NODEINST *ni, POLYLIST *plist, WINDOWPART *win, BOOLEAN onlyreasonable)
{
	REGISTER INTBIG tot, j;
	INTBIG reasonable;
	REGISTER NODEPROTO *np;
	REGISTER POLYGON *poly;
	POLYLOOP mypl;

	np = ni->proto;
	if (np->primindex == 0) return(0);

	/* if the technology has its own routine, use it */
	if (np->tech->allnodepolys != 0)
	{
		tot = (*(np->tech->allnodepolys))(ni, plist, win, onlyreasonable);
		return(tot);
	}

	mypl.curwindowpart = win;
	tot = tech_nodepolys(ni, &reasonable, win, &mypl);
	if (onlyreasonable) tot = reasonable;
	if (mypl.realpolys < tot) tot = mypl.realpolys;
	if (ensurepolylist(plist, tot, db_cluster)) return(-1);
	for(j = 0; j < tot; j++)
	{
		poly = plist->polygons[j];
		poly->tech = np->tech;
		tech_shapenodepoly(ni, j, poly, &mypl);
	}
	return(tot);
}

/* 
 * Routine to fill the polygon list "plist" with all electrical polygons associated with
 * node "ni" when displayed in window "win" (as with all "electrical" routines, this does not
 * include displayable variables).  If "onlyreasonable" is true, only count a reasonable
 * number of polygons (which means, exclude center cuts in large multi-cut contacts).
 * Returns the number of polygons (negative on error).
 */
INTBIG allnodeEpolys(NODEINST *ni, POLYLIST *plist, WINDOWPART *win, BOOLEAN onlyreasonable)
{
	REGISTER INTBIG tot, j;
	INTBIG reasonable;
	REGISTER NODEPROTO *np;
	REGISTER POLYGON *poly;
	POLYLOOP mypl;

	np = ni->proto;
	if (np->primindex == 0) return(0);

	/* if the technology has its own routine, use it */
	if (np->tech->allnodeEpolys != 0)
	{
		tot = (*(np->tech->allnodeEpolys))(ni, plist, win, onlyreasonable);
		return(tot);
	}

	mypl.curwindowpart = win;
	tot = tech_nodeEpolys(ni, &reasonable, win, &mypl);
	if (onlyreasonable) tot = reasonable;
	if (ensurepolylist(plist, tot, db_cluster)) return(-1);
	for(j = 0; j < tot; j++)
	{
		poly = plist->polygons[j];
		poly->tech = np->tech;
		tech_shapeEnodepoly(ni, j, poly, &mypl);
	}
	return(tot);
}

/*
 * routine to report the acutal size offsets of nodeinst "ni".
 * This is not always obvious since the extent of a nodeinst is not
 * necessarily its size.  This routine accesses the "node_width_offset"
 * variable on the technology objects.
 */
void nodesizeoffset(NODEINST *ni, INTBIG *lx, INTBIG *ly, INTBIG *hx, INTBIG *hy)
{
	REGISTER NODEPROTO *np;

	np = ni->proto;
	if (np->primindex == 0) { *lx = *ly = *hx = *hy = 0;   return; }

	/* if the technology has its own routine, use it */
	if (np->tech->nodesizeoffset != 0)
	{
		(*(np->tech->nodesizeoffset))(ni, lx, ly, hx, hy);
		return;
	}

	/* use value from prototype */
	tech_nodeprotosizeoffset(ni->proto, lx, ly, hx, hy, lambdaofnode(ni));
}

/*
 * routine to report the acutal size offsets of node prototype "np" as it will
 * appear in cell "parent" (which may be NONODEPROTO if unknown).
 * This is not always obvious since the extent of a nodeinst is not
 * necessarily its size.  This routine accesses the "node_width_offset"
 * variable on the technology objects.
 */
void nodeprotosizeoffset(NODEPROTO *np, INTBIG *lx, INTBIG *ly, INTBIG *hx, INTBIG *hy, NODEPROTO *parent)
{
	REGISTER INTBIG lambda;

	if (parent != NONODEPROTO) lambda = parent->lib->lambda[parent->tech->techindex]; else
		lambda = el_curlib->lambda[np->tech->techindex];
	tech_nodeprotosizeoffset(np, lx, ly, hx, hy, lambda);
}

/*
 * support routine for "nodesizeoffset" and "nodeprotosizeoffset" to report
 * the acutal size offsets of a node prototype.
 */
void tech_nodeprotosizeoffset(NODEPROTO *np, INTBIG *lx, INTBIG *ly, INTBIG *hx, INTBIG *hy,
	INTBIG lambda)
{
	REGISTER INTBIG *base, *addr;

	*lx = *ly = *hx = *hy = 0;
	if (np->primindex == 0) return;

	/* make sure cache of information is valid */
	if (tech_node_widoff == 0)
	{
		tech_initnodesizeoffset();
		if (tech_node_widoff == 0) return;
	}

	addr = tech_node_widoff[np->tech->techindex];
	if (addr == 0) return;

	base = &addr[(np->primindex-1)*4];
	*lx = *base++ * lambda/WHOLE;
	*hx = *base++ * lambda/WHOLE;
	*ly = *base++ * lambda/WHOLE;
	*hy = *base * lambda/WHOLE;
}

/*
 * routine to return the function code for nodeinst "ni" (see "efunction.h").
 */
INTBIG nodefunction(NODEINST *ni)
{
	REGISTER INTBIG type;

	if (ni->proto->primindex == 0) return(NPUNKNOWN);
	type = (ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH;
	switch (type)
	{
		case NPCAPAC:		/* capacitor */
			switch (ni->userbits&NTECHBITS)
			{
				case CAPACNORM: type = NPCAPAC;    break;	/* Capacitor is normal */
				case CAPACELEC: type = NPECAPAC;   break;	/* Capacitor is Electrolytic */
			}
			break;

		case NPDIODE:		/* diode */
			switch (ni->userbits&NTECHBITS)
			{
				case DIODENORM:  type = NPDIODE;    break;	/* Diode is normal */
				case DIODEZENER: type = NPDIODEZ;   break;	/* Diode is Zener */
			}
			break;

		case NPTRANS:		/* undefined transistor: look at its transistor type */
			switch (ni->userbits&NTECHBITS)
			{
				case TRANNMOS:  type = NPTRANMOS;    break;	/* Transistor is N channel MOS */
				case TRANDMOS:  type = NPTRADMOS;    break;	/* Transistor is Depletion MOS */
				case TRANPMOS:  type = NPTRAPMOS;    break;	/* Transistor is P channel MOS */
				case TRANNPN:   type = NPTRANPN;     break;	/* Transistor is NPN Junction */
				case TRANPNP:   type = NPTRAPNP;     break;	/* Transistor is PNP Junction */
				case TRANNJFET: type = NPTRANJFET;   break;	/* Transistor is N Channel Junction FET */
				case TRANPJFET: type = NPTRAPJFET;   break;	/* Transistor is P Channel Junction FET */
				case TRANDMES:  type = NPTRADMES;    break;	/* Transistor is Depletion MESFET */
				case TRANEMES:  type = NPTRAEMES;    break;	/* Transistor is Enhancement MESFET */
			}
			break;

		case NPTRANS4:		/* undefined 4-port-transistor: look at its transistor type */
			switch (ni->userbits&NTECHBITS)
			{
				case TRANNMOS:  type = NPTRA4NMOS;   break;	/* Transistor is N channel MOS */
				case TRANDMOS:  type = NPTRA4DMOS;   break;	/* Transistor is Depletion MOS */
				case TRANPMOS:  type = NPTRA4PMOS;   break;	/* Transistor is P channel MOS */
				case TRANNPN:   type = NPTRA4NPN;    break;	/* Transistor is NPN Junction */
				case TRANPNP:   type = NPTRA4PNP;    break;	/* Transistor is PNP Junction */
				case TRANNJFET: type = NPTRA4NJFET;  break;	/* Transistor is N Channel Junction FET */
				case TRANPJFET: type = NPTRA4PJFET;  break;	/* Transistor is P Channel Junction FET */
				case TRANDMES:  type = NPTRA4DMES;   break;	/* Transistor is Depletion MESFET */
				case TRANEMES:  type = NPTRA4EMES;   break;	/* Transistor is Enhancement MESFET */
			}
			break;

		case NPTLINE:		/* two-port device: look for more information */
			switch (ni->userbits&NTECHBITS)
			{
				case TWOPVCCS:  type = NPVCCS;     break;	/* Two-port is Transconductance (VCCS) */
				case TWOPCCVS:  type = NPCCVS;     break;	/* Two-port is Transresistance (CCVS) */
				case TWOPVCVS:  type = NPVCVS;     break;	/* Two-port is Voltage gain (VCVS) */
				case TWOPCCCS:  type = NPCCCS;     break;	/* Two-port is Current gain (CCCS) */
				case TWOPTLINE: type = NPTLINE;    break;	/* Two-port is Transmission Line */
			}
			break;
	}
	return(type);
}

/* these must match the "define"s in "efunction.h" */
typedef struct
{
	CHAR *nodefunname;		/* node function name */
	CHAR *shortfunname;		/* short node function name */
	CHAR *constantfunname;	/* actual code name */
} NODEFUNCTION;

static NODEFUNCTION db_nodefunname[] =
{
	{N_("unknown"),						N_("node"),     x_("NPUNKNOWN")},	/* NPUNKNOWN */
	{N_("pin"),							N_("pin"),      x_("NPPIN")},		/* NPPIN */
	{N_("contact"),						N_("contact"),  x_("NPCONTACT")},	/* NPCONTACT */
	{N_("pure-layer-node"),				N_("plnode"),   x_("NPNODE")},		/* NPNODE */
	{N_("connection"),					N_("conn"),     x_("NPCONNECT")},	/* NPCONNECT */
	{N_("nMOS-transistor"),				N_("nmos"),     x_("NPTRANMOS")},	/* NPTRANMOS */
	{N_("DMOS-transistor"),				N_("dmos"),     x_("NPTRADMOS")},	/* NPTRADMOS */
	{N_("pMOS-transistor"),				N_("pmos"),     x_("NPTRAPMOS")},	/* NPTRAPMOS */
	{N_("NPN-transistor"),				N_("npn"),      x_("NPTRANPN")},	/* NPTRANPN */
	{N_("PNP-transistor"),				N_("pnp"),      x_("NPTRAPNP")},	/* NPTRAPNP */
	{N_("n-type-JFET-transistor"),		N_("njfet"),    x_("NPTRANJFET")},	/* NPTRANJFET */
	{N_("p-type-JFET-transistor"),		N_("pjfet"),    x_("NPTRAPJFET")},	/* NPTRAPJFET */
	{N_("depletion-mesfet"),			N_("dmes"),     x_("NPTRADMES")},	/* NPTRADMES */
	{N_("enhancement-mesfet"),			N_("emes"),     x_("NPTRAEMES")},	/* NPTRAEMES */
	{N_("prototype-defined-transistor"),N_("tref"),     x_("NPTRANSREF")},	/* NPTRANSREF */
	{N_("transistor"),					N_("trans"),    x_("NPTRANS")},		/* NPTRANS */
	{N_("4-port-nMOS-transistor"),		N_("nmos4p"),   x_("NPTRA4NMOS")},	/* NPTRA4NMOS */
	{N_("4-port-DMOS-transistor"),		N_("dmos4p"),   x_("NPTRA4DMOS")},	/* NPTRA4DMOS */
	{N_("4-port-pMOS-transistor"),		N_("pmos4p"),   x_("NPTRA4PMOS")},	/* NPTRA4PMOS */
	{N_("4-port-NPN-transistor"),		N_("npn4p"),    x_("NPTRA4NPN")},	/* NPTRA4NPN */
	{N_("4-port-PNP-transistor"),		N_("pnp4p"),    x_("NPTRA4PNP")},	/* NPTRA4PNP */
	{N_("4-port-n-type-JFET-transistor"),N_("njfet4p"), x_("NPTRA4NJFET")},	/* NPTRA4NJFET */
	{N_("4-port-p-type-JFET-transistor"),N_("pjfet4p"), x_("NPTRA4PJFET")},	/* NPTRA4PJFET */
	{N_("4-port-depletion-mesfet"),		N_("dmes4p"),   x_("NPTRA4DMES")},	/* NPTRA4DMES */
	{N_("4-port-enhancement-mesfet"),	N_("emes4p"),   x_("NPTRA4EMES")},	/* NPTRA4EMES */
	{N_("4-port-transistor"),			N_("trans4p"),  x_("NPTRANS4")},	/* NPTRANS4 */
	{N_("resistor"),					N_("res"),      x_("NPRESIST")},	/* NPRESIST */
	{N_("capacitor"),					N_("cap"),      x_("NPCAPAC")},		/* NPCAPAC */
	{N_("electrolytic-capacitor"),		N_("ecap"),     x_("NPECAPAC")},	/* NPECAPAC */
	{N_("diode"),						N_("diode"),    x_("NPDIODE")},		/* NPDIODE */
	{N_("zener-diode"),					N_("zdiode"),   x_("NPDIODEZ")},	/* NPDIODEZ */
	{N_("inductor"),					N_("ind"),      x_("NPINDUCT")},	/* NPINDUCT */
	{N_("meter"),						N_("meter"),    x_("NPMETER")},		/* NPMETER */
	{N_("base"),						N_("base"),     x_("NPBASE")},		/* NPBASE */
	{N_("emitter"),						N_("emit"),     x_("NPEMIT")},		/* NPEMIT */
	{N_("collector"),					N_("coll"),     x_("NPCOLLECT")},	/* NPCOLLECT */
	{N_("buffer"),						N_("buf"),      x_("NPBUFFER")},	/* NPBUFFER */
	{N_("AND-gate"),					N_("and"),      x_("NPGATEAND")},	/* NPGATEAND */
	{N_("OR-gate"),						N_("or"),       x_("NPGATEOR")},	/* NPGATEOR */
	{N_("XOR-gate"),					N_("xor"),      x_("NPGATEXOR")},	/* NPGATEXOR */
	{N_("flip-flop"),					N_("ff"),       x_("NPFLIPFLOP")},	/* NPFLIPFLOP */
	{N_("multiplexor"),					N_("mux"),      x_("NPMUX")},		/* NPMUX */
	{N_("power"),						N_("pwr"),      x_("NPCONPOWER")},	/* NPCONPOWER */
	{N_("ground"),						N_("gnd"),      x_("NPCONGROUND")},	/* NPCONGROUND */
	{N_("source"),						N_("source"),   x_("NPSOURCE")},	/* NPSOURCE */
	{N_("substrate"),					N_("substr"),   x_("NPSUBSTRATE")},	/* NPSUBSTRATE */
	{N_("well"),						N_("well"),     x_("NPWELL")},		/* NPWELL */
	{N_("artwork"),						N_("art"),      x_("NPART")},		/* NPART */
	{N_("array"),						N_("array"),    x_("NPARRAY")},		/* NPARRAY */
	{N_("align"),						N_("align"),    x_("NPALIGN")},		/* NPALIGN */
	{N_("ccvs"),						N_("ccvs"),     x_("NPCCVS")},		/* NPCCVS */
	{N_("cccs"),						N_("cccs"),     x_("NPCCCS")},		/* NPCCCS */
	{N_("vcvs"),						N_("vcvs"),     x_("NPVCVS")},		/* NPVCVS */
	{N_("vccs"),						N_("vccs"),     x_("NPVCCS")},		/* NPVCCS */
	{N_("transmission-line"),			N_("transm"),   x_("NPTLINE")}		/* NPTLINE */
};

/*
 * routine to return the name of node "ni" with function "fun"
 */
CHAR *nodefunctionname(INTBIG fun, NODEINST *ni)
{
	REGISTER void *infstr;

	if (fun == NPTRANSREF && ni != NONODEINST)
	{
		infstr = initinfstr();
		formatinfstr(infstr, _("Transistor-%s"), ni->proto->protoname);
		return(returninfstr(infstr));
	}
	if (fun < 0 || fun >= MAXNODEFUNCTION) return(x_(""));
	return(TRANSLATE(db_nodefunname[fun].nodefunname));
}

/*
 * routine to return the short name of node function "fun"
 */
CHAR *nodefunctionshortname(INTBIG fun)
{
	if (fun < 0 || fun >= MAXNODEFUNCTION) return(x_(""));
	return(TRANSLATE(db_nodefunname[fun].shortfunname));
}

/*
 * routine to return the constant name of node function "fun"
 */
CHAR *nodefunctionconstantname(INTBIG fun)
{
	if (fun < 0 || fun >= MAXNODEFUNCTION) return(x_(""));
	return(db_nodefunname[fun].constantfunname);
}

/*
 * routine to tell whether geometry module "pos" points to a field-effect
 * transtor.  Returns true if so.
 */
BOOLEAN isfet(GEOM *pos)
{
	REGISTER INTBIG fun;

	if (!pos->entryisnode) return(FALSE);
	fun = nodefunction(pos->entryaddr.ni);
	switch (fun)
	{
		case NPTRANMOS:   case NPTRA4NMOS:
		case NPTRADMOS:   case NPTRA4DMOS:
		case NPTRAPMOS:   case NPTRA4PMOS:
		case NPTRADMES:   case NPTRA4DMES:
		case NPTRAEMES:   case NPTRA4EMES:	
			return(TRUE);
	}
	return(FALSE);
}

/*
 * Routine to return TRUE if cell "np" or anything below it has a resistor.
 */
BOOLEAN hasresistors(NODEPROTO *np)
{
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *onp;

	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
			onp->temp1 = 0;
	return(tech_findresistors(np));
}

/*
 * Helper routine for "hasresistors" to recursively examine the hierarchy.
 */
BOOLEAN tech_findresistors(NODEPROTO *np)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *subnp, *cnp;

	if (np->temp1 != 0) return(FALSE);
	np->temp1 = 1;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		subnp = ni->proto;
		if (subnp->primindex == 0)
		{
			/* recurse */
			if (tech_findresistors(subnp)) return(TRUE);
			cnp = contentsview(subnp);
			if (cnp != NONODEPROTO)
			{
				if (tech_findresistors(cnp)) return(TRUE);
			}
			continue;
		}
		if (subnp == sch_resistorprim) return(TRUE);
	}
	return(FALSE);
}

/*
 * routine to determine the length and width of the primitive transistor
 * node "ni" and return it in the reference integers "length" and "width".
 * The value returned is in internal units.
 * If the value cannot be determined, -1 is returned in the length and width.
 * Mar. 1991 SRP: If the first character of *extra is not a digit or a
 * sign, then do not call latoa.  This allows us to put a model name after
 * the type string 'npn', etc. in SCHEM_transistortype that does not include
 * any size data.
 */
void transistorsize(NODEINST *ni, INTBIG *length, INTBIG *width)
{
	INTBIG lx, ly, hx, hy;
	REGISTER INTBIG count, i;
	REGISTER INTBIG fx, fy, tx, ty, lambda;
	CHAR *pt;
	REGISTER VARIABLE *var, *varl, *varw;

	*length = *width = -1;
	switch ((ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH)
	{
		case NPTRANMOS:   case NPTRA4NMOS:
		case NPTRADMOS:   case NPTRA4DMOS:
		case NPTRAPMOS:   case NPTRA4PMOS:
		case NPTRANJFET:  case NPTRA4NJFET:
		case NPTRAPJFET:  case NPTRA4PJFET:
		case NPTRADMES:   case NPTRA4DMES:
		case NPTRAEMES:   case NPTRA4EMES:
			var = gettrace(ni);
			if (var != NOVARIABLE)
			{
				/* serpentine transistor: compute path length */
				*width = 0;
				count = getlength(var) / 2;
				for(i=1; i<count; i++)
				{
					fx = ((INTBIG *)var->addr)[i*2-2];
					fy = ((INTBIG *)var->addr)[i*2-1];
					tx = ((INTBIG *)var->addr)[i*2];
					ty = ((INTBIG *)var->addr)[i*2+1];
					*width += computedistance(fx,fy, tx,ty);
				}

				var = getvalkey((INTBIG)ni, VNODEINST, VFRACT, el_transistor_width_key);
				if (var != NOVARIABLE) *length = var->addr * lambdaofnode(ni)/WHOLE; else
				{
					nodesizeoffset(ni, &lx, &ly, &hx, &hy);
					*length = ni->proto->highy-hy - (ni->proto->lowy+ly);
				}
			} else
			{
				/* normal transistor: subtract offset for active area */
				nodesizeoffset(ni, &lx, &ly, &hx, &hy);
				*length = ni->highy-hy - (ni->lowy+ly);
				*width = ni->highx-hx - (ni->lowx+lx);

				/* special case if scalable transistors */
				if (ni->proto == mocmos_scalablentransprim || ni->proto == mocmos_scalableptransprim)
				{
					varw = getvalkey((INTBIG)ni, VNODEINST, -1, el_attrkey_width);
					if (varw != NOVARIABLE)
					{
						lambda = lambdaofnode(ni);
						pt = describevariable(varw, -1, -1);
						if (*pt == '-' || *pt == '+' || isdigit(*pt))
						{
							*width = muldiv(atofr(pt), lambda, WHOLE);
						}
					}
				}
			}
			break;

		case NPTRANS:
		case NPTRANS4:
			lambda = lambdaofnode(ni);
			varl = getvalkey((INTBIG)ni, VNODEINST, -1, el_attrkey_length);
			varw = getvalkey((INTBIG)ni, VNODEINST, -1, el_attrkey_width);
			if (varl != NOVARIABLE || varw != NOVARIABLE)
			{
				if (varl == NOVARIABLE) *length = lambda * 2; else
				{
					pt = describevariable(varl, -1, -1);
					if (*pt == '-' || *pt == '+' || isdigit(*pt))
					{
						*length = muldiv(atofr(pt), lambda, WHOLE);
					}
				}

				/* why did JG change "width" to "length" below!!! */
				if (varw == NOVARIABLE) *width = lambda * 3; else
				{
					pt = describevariable(varw, -1, -1);
					if (*pt == '-' || *pt == '+' || isdigit(*pt))
					{
						*width = muldiv(atofr(pt), lambda, WHOLE);
					}
				}
			} else
			{
				/* no length/width, look for area */
				var = getvalkey((INTBIG)ni, VNODEINST, -1, el_attrkey_area);
				if (var != NOVARIABLE)
				{
					pt = describevariable(var, -1, -1);
					if (*pt == '-' || *pt == '+' || isdigit(*pt))
					{
						*length = muldiv(atofr(pt), lambda, WHOLE);
						*width = lambda;
					}
				}
			}
			break;
	}
}

/*
 * routine to return the ports of transistor "ni" in "gateleft", "gateright",
 * "activetop", and "activebottom".  If "gateright" is NOPORTPROTO, there is only
 * one gate port.
 *
 * This code is predicated upon the fact that all MOS transistors have four ports
 * in the same sequence: gateleft, activetop, gateright, activebottom.  The schematic
 * transistor, which has only three ports, is ordered: gate, source, drain.
 * We have to allow for multiple ported transistors, so we will look at the
 * nodefunction again (SRP)
 */
void transistorports(NODEINST *ni, PORTPROTO **gateleft, PORTPROTO **gateright,
	PORTPROTO **activetop, PORTPROTO **activebottom)
{
	REGISTER INTBIG fun;

	fun = nodefunction(ni);
	*activetop = *gateright = *activebottom = NOPORTPROTO;
	*gateleft = ni->proto->firstportproto;
	if (*gateleft == NOPORTPROTO) return;
	*activetop = (*gateleft)->nextportproto;
	if (*activetop == NOPORTPROTO) return;
	*gateright = (*activetop)->nextportproto;
	if ((*gateright)->nextportproto == NOPORTPROTO || fun == NPTRANPN ||
		fun == NPTRAPNP || fun == NPTRA4NMOS || fun == NPTRA4DMOS ||
		fun == NPTRA4PMOS || fun == NPTRA4NPN || fun == NPTRA4PNP ||
		fun == NPTRA4NJFET || fun == NPTRA4PJFET ||
		fun == NPTRA4DMES || fun == NPTRA4EMES)
	{
		*activebottom = *gateright;
		*gateright = NOPORTPROTO;
	} else
		*activebottom = (*gateright)->nextportproto;
}

/*
 * routine to get the starting and ending angle of the arc described by node "ni".
 * Sets "startoffset" to the fractional difference between the node rotation and the
 * true starting angle of the arc (this will be less than a tenth of a degree, since
 * node rotation is in tenths of a degree).  Sets "endangle" to the ending rotation
 * of the arc (the true ending angle is this plus the node rotation and "startoffset").
 * Both "startoffset" and "endangle" are in radians).
 * If the node is not circular, both values are set to zero.
 */
void getarcdegrees(NODEINST *ni, double *startoffset, double *endangle)
{
	REGISTER VARIABLE *var;
	float sof, eaf;

	*startoffset = *endangle = 0.0;
	if (ni->proto != art_circleprim && ni->proto != art_thickcircleprim) return;
	var = getvalkey((INTBIG)ni, VNODEINST, -1, art_degreeskey);
	if (var == NOVARIABLE) return;
	if ((var->type&VTYPE) == VINTEGER)
	{
		*startoffset = 0.0;
		*endangle = (double)var->addr * EPI / 1800.0;
		return;
	}
	if ((var->type&(VTYPE|VISARRAY)) == (VFLOAT|VISARRAY))
	{
		sof = ((float *)var->addr)[0];
		eaf = ((float *)var->addr)[1];
		*startoffset = (double)sof;
		*endangle = (double)eaf;
	}
}

/*
 * routine to set the starting and ending angle of the arc described by node "ni".
 * Sets "startoffset" to the fractional difference between the node rotation and the
 * true starting angle of the arc (this will be less than a tenth of a degree, since
 * node rotation is in tenths of a degree).  Sets "endangle" to the ending rotation
 * of the arc (the true ending angle is this plus the node rotation and "startoffset").
 * Both "startoffset" and "endangle" are in radians).
 * If the node is not circular, this call does nothing.
 */
void setarcdegrees(NODEINST *ni, double startoffset, double endangle)
{
	REGISTER INTBIG angle;
	REGISTER double rangle;
	float degs[2];

	if (ni->proto != art_circleprim && ni->proto != art_thickcircleprim) return;
	if (startoffset == 0.0 && endangle == 0.0)
	{
		/* no arc on this circle: remove any data */
		if (getvalkey((INTBIG)ni, VNODEINST, -1, art_degreeskey) == NOVARIABLE) return;
		(void)delvalkey((INTBIG)ni, VNODEINST, art_degreeskey);
	} else
	{
		/* put arc information on the circle */
		angle = rounddouble(endangle * 1800.0 / EPI);
		rangle = (double)angle * EPI / 1800.0;
		if (startoffset == 0.0 && rangle == endangle)
		{
			(void)setvalkey((INTBIG)ni, VNODEINST, art_degreeskey, angle, VINTEGER);
		} else
		{
			degs[0] = (float)startoffset;
			degs[1] = (float)endangle;
			(void)setvalkey((INTBIG)ni, VNODEINST, art_degreeskey, (INTBIG)degs,
				VFLOAT|VISARRAY|(2<<VLENGTHSH));
		}
	}
	updategeom(ni->geom, ni->parent);
	db_setchangecell(ni->parent);
}

/*
 * Routine to return the endpoints of the arc on node "ni" that has a starting offset of
 * "startoffset" and an ending angle of "endangle" (from "getarcdegrees()" above).  Returns
 * the coordinates in (fx,fy) and (tx,ty).
 */
void getarcendpoints(NODEINST *ni, double startoffset, double endangle, INTBIG *fx, INTBIG *fy,
	INTBIG *tx, INTBIG *ty)
{
	REGISTER INTBIG cx, cy, radius;

	cx = (ni->lowx + ni->highx) / 2;
	cy = (ni->lowy + ni->highy) / 2;
	radius = (ni->highx - ni->lowx) / 2;
	startoffset += ((double)ni->rotation) * EPI / 1800.0;
	if (ni->transpose != 0)
	{
		startoffset = 1.5 * EPI - startoffset - endangle;
		if (startoffset < 0.0) startoffset += EPI * 2.0;
	}
	*fx = cx + rounddouble(cos(startoffset) * radius);
	*fy = cy + rounddouble(sin(startoffset) * radius);
	*tx = cx + rounddouble(cos(startoffset+endangle) * radius);
	*ty = cy + rounddouble(sin(startoffset+endangle) * radius);
}

/*
 * Routine to get the primitive node in technology "tech" that is the pure-layer node
 * for layer "layer" (if layer is negative, then look for the pure-layer node with
 * function "function").  Returns NONODEPROTO if none is found.
 */
NODEPROTO *getpurelayernode(TECHNOLOGY *tech, INTBIG layer, INTBIG function)
{
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	NODEINST node;
	REGISTER INTBIG i, fun;
	REGISTER POLYGON *poly;

	/* get polygon */
	poly = allocpolygon(4, db_cluster);

	ni = &node;   initdummynode(ni);
	ni->lowx = ni->highx = 0;
	ni->lowy = ni->highy = 0;
	for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (((np->userbits&NFUNCTION) >> NFUNCTIONSH) != NPNODE) continue;
		ni->proto = np;
		i = nodepolys(ni, 0, NOWINDOWPART);
		if (i != 1) continue;
		shapenodepoly(ni, 0, poly);
		if (layer >= 0)
		{
			if (poly->layer == layer) break;
		} else
		{
			fun = layerfunction(tech, poly->layer);
			if ((fun&(LFTYPE|LFPTYPE|LFNTYPE)) == function) break;
		}
	}
	freepolygon(poly);
	return(np);
}

/******************** PORT DESCRIPTION ********************/

/*
 * routine to set polygon "poly" to the shape of port "pp" on nodeinst "ni".
 * If "purpose" is false, the entire port is desired.  If "purpose" is true,
 * the exact location of a new port is desired and that port should be
 * optimally close to the co-ordinates in (poly->xv[0],poly->yv[0]).
 */
void shapeportpoly(NODEINST *ni, PORTPROTO *pp, POLYGON *poly, BOOLEAN purpose)
{
	REGISTER INTBIG pindex;
	REGISTER TECH_NODES *thistn;
	XARRAY localtran, tempt1, tempt2, *t1, *t2, *swapt;

	/* look down to the bottom level node/port */
	t1 = (XARRAY *)tempt1;   t2 = (XARRAY *)tempt2;
	if (ni->rotation == 0 && ni->transpose == 0) transid(*t1); else
		makerot(ni, *t1);
	while (ni->proto->primindex == 0)
	{
		maketrans(ni, localtran);
		transmult(localtran, *t1, *t2);
		swapt = t1;   t1 = t2;   t2 = swapt;
		ni = pp->subnodeinst;
		pp = pp->subportproto;
		if (ni->rotation != 0 || ni->transpose != 0)
		{
			makerot(ni, localtran);
			transmult(localtran, *t1, *t2);
			swapt = t1;   t1 = t2;   t2 = swapt;
		}
	}

	/* if the technology has its own routine, use it */
	if (ni->proto->tech->shapeportpoly != 0)
	{
		(*(ni->proto->tech->shapeportpoly))(ni, pp, poly, *t1, purpose);
		return;
	}

	pindex = ni->proto->primindex;
	thistn = ni->proto->tech->nodeprotos[pindex-1];
	switch (thistn->special)
	{
		case SERPTRANS:
			tech_filltransport(ni, pp, poly, *t1, thistn, thistn->f2, thistn->f3,
				thistn->f4, thistn->f5, thistn->f6);
			break;

		default:
			tech_fillportpoly(ni, pp, poly, *t1, thistn, CLOSED, lambdaofnode(ni));
			break;
	}
}

/*
 * routine to set polygon "poly" to the shape of port "pp" on nodeinst "ni",
 * given that the node transformation is already known and is "trans".
 */
void shapetransportpoly(NODEINST *ni, PORTPROTO *pp, POLYGON *poly, XARRAY trans)
{
	REGISTER INTBIG pindex;
	REGISTER TECH_NODES *thistn;

	/* if the technology has its own routine, use it */
	if (ni->proto->tech->shapeportpoly != 0)
	{
		(*(ni->proto->tech->shapeportpoly))(ni, pp, poly, trans, FALSE);
		return;
	}

	pindex = ni->proto->primindex;
	thistn = ni->proto->tech->nodeprotos[pindex-1];
	switch (thistn->special)
	{
		case SERPTRANS:
			tech_filltransport(ni, pp, poly, trans, thistn, thistn->f2, thistn->f3,
				thistn->f4, thistn->f5, thistn->f6);
			break;

		default:
			tech_fillportpoly(ni, pp, poly, trans, thistn, CLOSED, lambdaofnode(ni));
			break;
	}
}

/*
 * routine to compute the center of port "pp" in nodeinst "ni" (taking
 * nodeinst position, transposition and rotation into account).  The location
 * is placed in the reference integer parameters "x" and "y"
 */
void portposition(NODEINST *ni, PORTPROTO *pp, INTBIG *x, INTBIG *y)
{
	REGISTER POLYGON *poly;

	/* make sure there is a polygon */
	poly = allocpolygon(4, db_cluster);

	/* get the polygon describing the port */
	shapeportpoly(ni, pp, poly, FALSE);

	/* determine the center of the polygon */
	getcenter(poly, x, y);

	/* free the polygon */
	freepolygon(poly);
}

/*
 * routine to return true if port "pp" is a power port
 */
BOOLEAN portispower(PORTPROTO *pp)
{
	if ((pp->userbits&STATEBITS) == PWRPORT) return(TRUE);
	if ((pp->userbits&STATEBITS) != 0) return(FALSE);
	if (portisnamedpower(pp)) return(TRUE);
	return(FALSE);
}

/*
 * routine to return true if port "pp" has a power port name
 */
BOOLEAN portisnamedpower(PORTPROTO *pp)
{
	REGISTER CHAR *pt;
	REGISTER INTBIG len;

	for(pt = pp->protoname; *pt != 0; pt++)
	{
		len = estrlen(pt);
		if (len >= 3)
		{
			if (namesamen(pt, x_("vdd"), 3) == 0) return(TRUE);
			if (namesamen(pt, x_("pwr"), 3) == 0) return(TRUE);
			if (namesamen(pt, x_("vcc"), 3) == 0) return(TRUE);
		}
		if (len >= 5)
		{
			if (namesamen(pt, x_("power"), 5) == 0) return(TRUE);
		}
	}
	return(FALSE);
}

/*
 * routine to return true if port "pp" is a ground port
 */
BOOLEAN portisground(PORTPROTO *pp)
{
	if ((pp->userbits&STATEBITS) == GNDPORT) return(TRUE);
	if ((pp->userbits&STATEBITS) != 0) return(FALSE);
	if (portisnamedground(pp)) return(TRUE);
	return(FALSE);
}

/*
 * routine to return true if port "pp" has a ground port name
 */
BOOLEAN portisnamedground(PORTPROTO *pp)
{
	REGISTER CHAR *pt;
	REGISTER INTBIG len;

	for(pt = pp->protoname; *pt != 0; pt++)
	{
		len = estrlen(pt);
		if (len >= 3)
		{
			if (namesamen(pt, x_("gnd"), 3) == 0) return(TRUE);
			if (namesamen(pt, x_("vss"), 3) == 0) return(TRUE);
		}
		if (len >= 6)
		{
			if (namesamen(pt, x_("ground"), 6) == 0) return(TRUE);
		}
	}
	return(FALSE);
}

/******************** ARCINST DESCRIPTION ********************/

/*
 * routine to report the number of distinct polygons used to compose
 * primitive arcinst "ai".
 */
INTBIG arcpolys(ARCINST *ai, WINDOWPART *win)
{
	REGISTER TECHNOLOGY *tech;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	/* if the technology has its own routine, use it */
	tech = ai->proto->tech;
	tech_oneprocpolyloop.curwindowpart = win;
	if (tech->arcpolys != 0) return((*(tech->arcpolys))(ai, win));

	return(tech_arcpolys(ai, win, &tech_oneprocpolyloop));
}

INTBIG tech_arcpolys(ARCINST *ai, WINDOWPART *win, POLYLOOP *pl)
{
	REGISTER INTBIG i;
	REGISTER TECHNOLOGY *tech;

	/* if the technology has its own routine, use it */
	tech = ai->proto->tech;

	/* reset negated bit if set and not allowed */
	if ((tech->userbits&NONEGATEDARCS) != 0 && (ai->userbits&ISNEGATED) != 0)
		tech_resetnegated(ai);

	/* get number of polygons in the arc */
	i = tech->arcprotos[ai->proto->arcindex]->laycount;

	/* add one layer if arc is directional and technology allows it */
	if ((tech->userbits&NODIRECTIONALARCS) == 0 && (ai->userbits&ISDIRECTIONAL) != 0) i++;

	/* if zero-length, not end-extended, not directional, not negated, ignore all */
	if (ai->end[0].xpos == ai->end[1].xpos && ai->end[0].ypos == ai->end[1].ypos &&
		(ai->userbits&(NOEXTEND|ISNEGATED|ISDIRECTIONAL|NOTEND0|NOTEND1)) == NOEXTEND)
			i = 0;

	/* add in displayable variables */
	pl->realpolys = i;
	i += tech_displayableavars(ai, win, pl);
	return(i);
}

/*
 * routine to describe polygon number "box" of arcinst "ai".  The description
 * is placed in the polygon "poly".
 */
void shapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly)
{
	REGISTER ARCPROTO *ap;
	REGISTER TECHNOLOGY *tech;

	/* if the technology has its own routine, use it */
	ap = ai->proto;
	tech = ap->tech;
	poly->tech = tech;
	if (tech->shapearcpoly != 0)
	{
		(*(tech->shapearcpoly))(ai, box, poly);
		return;
	}

	tech_shapearcpoly(ai, box, poly, &tech_oneprocpolyloop);
}

void tech_shapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly, POLYLOOP *pl)
{
	REGISTER INTBIG pindex;
	REGISTER ARCPROTO *ap;
	REGISTER TECHNOLOGY *tech;
	REGISTER TECH_ARCLAY *thista;

	/* if the technology has its own routine, use it */
	ap = ai->proto;
	tech = ap->tech;
	poly->tech = tech;

	/* handle displayable variables */
	if (box >= pl->realpolys)
	{
		(void)tech_filldisplayableavar(ai, poly, pl->curwindowpart, 0, pl);
		return;
	}

	pindex = ap->arcindex;
	if (box >= tech->arcprotos[pindex]->laycount) tech_makearrow(ai, poly); else
	{
		thista = &tech->arcprotos[pindex]->list[box];
		makearcpoly(ai->length, ai->width-thista->off*lambdaofarc(ai)/WHOLE, ai, poly,
			thista->style);
		poly->layer = thista->lay;
		poly->desc = tech->layers[poly->layer];
	}
}

/* 
 * Routine to fill the polygon list "plist" with all polygons associated with arc "ai"
 * when displayed in window "win" (but excluding displayable variables).  Returns the
 * number of polygons (negative on error).
 */
INTBIG allarcpolys(ARCINST *ai, POLYLIST *plist, WINDOWPART *win)
{
	REGISTER INTBIG tot, j;
	POLYLOOP mypl;

	mypl.curwindowpart = win;
	tot = tech_arcpolys(ai, win, &mypl);
	tot = mypl.realpolys;
	if (ensurepolylist(plist, tot, db_cluster)) return(-1);
	for(j = 0; j < tot; j++)
	{
		tech_shapearcpoly(ai, j, plist->polygons[j], &mypl);
	}
	return(tot);
}

/*
 * routine to fill polygon "poly" with the outline of the curved arc in
 * "ai" whose width is "wid".  The style of the polygon is set to "style".
 * If there is no curvature information in the arc, the routine returns true,
 * otherwise it returns false.
 */
BOOLEAN curvedarcoutline(ARCINST *ai, POLYGON *poly, INTBIG style, INTBIG wid)
{
	REGISTER INTBIG i, points, anglebase, anglerange, pieces, a;
	REGISTER INTBIG radius, centerx, centery, innerradius, outerradius, sin, cos;
	INTBIG x1, y1, x2, y2;
	REGISTER VARIABLE *var;

	/* get the radius information on the arc */
	var = getvalkey((INTBIG)ai, VARCINST, VINTEGER, el_arc_radius_key);
	if (var == NOVARIABLE) return(TRUE);
	radius = var->addr;

	/* see if the radius can work with these arc ends */
	if (abs(radius)*2 < ai->length) return(TRUE);

	/* determine the center of the circle */
	if (findcenters(abs(radius), ai->end[0].xpos, ai->end[0].ypos,
		ai->end[1].xpos, ai->end[1].ypos, ai->length, &x1,&y1, &x2,&y2)) return(TRUE);

	if (radius < 0)
	{
		radius = -radius;
		centerx = x1;   centery = y1;
	} else
	{
		centerx = x2;   centery = y2;
	}

	/* determine the base and range of angles */
	anglebase = figureangle(centerx,centery, ai->end[0].xpos,ai->end[0].ypos);
	anglerange = figureangle(centerx, centery, ai->end[1].xpos, ai->end[1].ypos);
	if ((ai->userbits&REVERSEEND) != 0)
	{
		i = anglebase;
		anglebase = anglerange;
		anglerange = i;
	}
	anglerange -= anglebase;
	if (anglerange < 0) anglerange += 3600;

	/* determine the number of intervals to use for the arc */
	pieces = anglerange;
	while (pieces > 16) pieces /= 2;

	/* initialize the polygon */
	points = (pieces+1) * 2;
	if (poly->limit < points) (void)extendpolygon(poly, points);
	poly->count = points;
	poly->style = style;

	/* get the inner and outer radii of the arc */
	outerradius = radius + wid / 2;
	innerradius = outerradius - wid;

	/* fill the polygon */
	for(i=0; i<=pieces; i++)
	{
		a = (anglebase + i * anglerange / pieces) % 3600;
		sin = sine(a);   cos = cosine(a);
		poly->xv[i] = mult(cos, innerradius) + centerx;
		poly->yv[i] = mult(sin, innerradius) + centery;
		poly->xv[points-1-i] = mult(cos, outerradius) + centerx;
		poly->yv[points-1-i] = mult(sin, outerradius) + centery;
	}
	return(FALSE);
}

/*
 * routine to make a polygon that describes the arcinst "ai" which is "len"
 * long and "wid" wide.  The polygon is in "poly", the style is set to "style".
 */
void makearcpoly(INTBIG len, INTBIG wid, ARCINST *ai, POLYGON *poly, INTBIG style)
{
	REGISTER INTBIG x1, y1, x2, y2, e1, e2, angle;

	x1 = ai->end[0].xpos;   y1 = ai->end[0].ypos;
	x2 = ai->end[1].xpos;   y2 = ai->end[1].ypos;
	poly->style = style;

	/* zero-width polygons are simply lines */
	if (wid == 0)
	{
		if (poly->limit < 2) (void)extendpolygon(poly, 2);
		poly->count = 2;
		poly->xv[0] = x1;   poly->yv[0] = y1;
		poly->xv[1] = x2;   poly->yv[1] = y2;
		return;
	}

	/* determine the end extension on each end */
	e1 = e2 = wid/2;
	if ((ai->userbits&NOEXTEND) != 0)
	{
		/* nonextension arc: set extension to zero for all included ends */
		if ((ai->userbits&NOTEND0) == 0) e1 = 0;
		if ((ai->userbits&NOTEND1) == 0) e2 = 0;
	} else if ((ai->userbits&ASHORT) != 0)
	{
		/* shortened arc: compute variable extension */
		e1 = tech_getextendfactor(wid, ai->endshrink&0xFFFF);
		e2 = tech_getextendfactor(wid, (ai->endshrink>>16)&0xFFFF);
	}

	/* make the polygon */
	angle = (ai->userbits&AANGLE) >> AANGLESH;
	tech_makeendpointpoly(len, wid, angle*10, x1,y1, e1, x2,y2, e2, poly);
}

/*
 * routine to return the offset between the nominal width of arcinst "ai"
 * and the actual width.  This routine accesses the "arc_width_offset"
 * variable on the technology objects.
 */
INTBIG arcwidthoffset(ARCINST *ai)
{
	REGISTER ARCPROTO *ap;

	/* if the technology has its own routine, use it */
	ap = ai->proto;
	if (ap->tech->arcwidthoffset != 0)
		return((*(ap->tech->arcwidthoffset))(ai));

	return(tech_arcprotowidthoffset(ap, lambdaofarc(ai)));
}

/*
 * routine to return the offset between the nominal width of arcproto "ap"
 * and the actual width.  This routine accesses the "arc_width_offset"
 * variable on the technology objects.
 */
INTBIG arcprotowidthoffset(ARCPROTO *ap)
{
	return(tech_arcprotowidthoffset(ap, el_curlib->lambda[ap->tech->techindex]));
}

/*
 * support routine for "arcwidthoffset()" and "arcprotowidthoffset()" to return the offset
 * between the nominal width of arcproto "ap" and the actual width.
 */
INTBIG tech_arcprotowidthoffset(ARCPROTO *ap, INTBIG lambda)
{
	REGISTER INTBIG *addr;

	/* make sure cache of information is valid */
	if (tech_arc_widoff == 0)
	{
		tech_initarcwidthoffset();
		if (tech_arc_widoff == 0) return(0);
	}

	addr = tech_arc_widoff[ap->tech->techindex];
	if (addr == 0) return(0);
	return(addr[ap->arcindex]*lambda/WHOLE);
}

/*
 * routine to return the name of the arc with function "fun"
 */
CHAR *arcfunctionname(INTBIG fun)
{
	static CHAR *arcfunname[] =
	{
		N_("unknown"),				/* APUNKNOWN */
		N_("metal-1"),				/* APMETAL1 */
		N_("metal-2"),				/* APMETAL2 */
		N_("metal-3"),				/* APMETAL3 */
		N_("metal-4"),				/* APMETAL4 */
		N_("metal-5"),				/* APMETAL5 */
		N_("metal-6"),				/* APMETAL6 */
		N_("metal-7"),				/* APMETAL7 */
		N_("metal-8"),				/* APMETAL8 */
		N_("metal-9"),				/* APMETAL9 */
		N_("metal-10"),				/* APMETAL10 */
		N_("metal-11"),				/* APMETAL11 */
		N_("metal-12"),				/* APMETAL12 */
		N_("polysilicon-1"),		/* APPOLY1 */
		N_("polysilicon-2"),		/* APPOLY2 */
		N_("polysilicon-3"),		/* APPOLY3 */
		N_("diffusion"),			/* APDIFF */
		N_("p-diffusion"),			/* APDIFFP */
		N_("n-diffusion"),			/* APDIFFN */
		N_("substrate-diffusion"),	/* APDIFFS */
		N_("well-diffusion"),		/* APDIFFW */
		N_("bus"),					/* APBUS */
		N_("unrouted"),				/* APUNROUTED */
		N_("nonelectrical")			/* APNONELEC */
	};

	if (fun < 0 || fun > APNONELEC) return(x_(""));
	return(TRANSLATE(arcfunname[fun]));
}

/*
 * Routine to find the arc that has layer "layer" in technology "tech".
 * Returns NOARCPROTO if the layer doesn't have an arc.
 */
ARCPROTO *getarconlayer(INTBIG layer, TECHNOLOGY *tech)
{
	REGISTER ARCPROTO *ap;
	REGISTER ARCINST *ai;
	ARCINST arc;
	REGISTER POLYGON *poly;

	/* get polygon */
	poly = allocpolygon(4, db_cluster);

	ai = &arc;   initdummyarc(ai);
	for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
	{
		ai->proto = ap;
		(void)arcpolys(ai, NOWINDOWPART);
		shapearcpoly(ai, 0, poly);
		if (poly->layer == layer) break;
	}
	freepolygon(poly);
	return(ap);
}

/******************** LAYER DESCRIPTION ********************/

/*
 * routine to return the full name of layer "layer" in technology "tech".
 * This routine accesses the "layer_names" variable on the technology objects.
 */
CHAR *layername(TECHNOLOGY *tech, INTBIG layer)
{
	REGISTER CHAR **addr;

	if (layer < 0) return(x_(""));

	/* make sure cache of information is valid */
	if (tech_layer_names == 0)
	{
		tech_initlayername();
		if (tech_layer_names == 0) return(x_(""));
	}

	addr = tech_layer_names[tech->techindex];
	if (addr == 0) return(x_(""));
	return(addr[layer]);
}

/*
 * routine to return the function of layer "layer" in technology "tech".
 * This routine accesses the "layer_function" variable on the technology objects.
 */
INTBIG layerfunction(TECHNOLOGY *tech, INTBIG layer)
{
	REGISTER INTBIG *addr;

	if (layer < 0) return(LFUNKNOWN);

	/* make sure cache of information is valid */
	if (tech_layer_function == 0)
	{
		tech_initlayerfunction();
		if (tech_layer_function == 0) return(LFUNKNOWN);
	}

	addr = tech_layer_function[tech->techindex];
	if (addr == 0) return(LFUNKNOWN);
	return(addr[layer]);
}

/* Routine to return true if the layer function "fun" is metal  */
BOOLEAN layerismetal(INTBIG fun)
{
	fun &= LFTYPE;
	if (fun == LFMETAL1  || fun == LFMETAL2  || fun == LFMETAL3 ||
		fun == LFMETAL4  || fun == LFMETAL5  || fun == LFMETAL6 ||
		fun == LFMETAL7  || fun == LFMETAL8  || fun == LFMETAL9 ||
		fun == LFMETAL10 || fun == LFMETAL11 || fun == LFMETAL12) return(TRUE);
	return(FALSE);
}

/* Routine to return nonzero if the layer function "fun" is polysilicon  */
BOOLEAN layerispoly(INTBIG fun)
{
	fun &= LFTYPE;
	if (fun == LFPOLY1 || fun == LFPOLY2 || fun == LFPOLY3 || fun == LFGATE) return(TRUE);
	return(FALSE);
}

/* Routine to return nonzero if the layer function "fun" is polysilicon  */
BOOLEAN layerisgatepoly(INTBIG fun)
{
	fun &= LFINTRANS;
	if (fun == LFINTRANS) return(TRUE);
	return(FALSE);
}

/* Routine to return nonzero if the layer function "fun" is a contact/via  */
BOOLEAN layeriscontact(INTBIG fun)
{
	fun &= LFTYPE;
	if (fun == LFCONTACT1 || fun == LFCONTACT2 || fun == LFCONTACT3 ||
		fun == LFCONTACT4 || fun == LFCONTACT5 || fun == LFCONTACT6 ||
		fun == LFCONTACT7 || fun == LFCONTACT8 || fun == LFCONTACT9 ||
		fun == LFCONTACT10 || fun == LFCONTACT11 || fun == LFCONTACT12)  return(TRUE);
	return(FALSE);
}

/*
 * routine to return the height of layer function "funct".
 */
INTBIG layerfunctionheight(INTBIG funct)
{
	switch (funct & LFTYPE)
	{
		case LFWELL:       return(0);
		case LFSUBSTRATE:  return(1);
		case LFIMPLANT:    return(2);
		case LFTRANSISTOR: return(3);
		case LFRESISTOR:   return(4);
		case LFCAP:        return(5);
		case LFEMITTER:    return(6);
		case LFBASE:       return(7);
		case LFCOLLECTOR:  return(8);
		case LFGUARD:      return(9);
		case LFISOLATION:  return(10);
		case LFDIFF:       return(11);
		case LFPOLY1:      return(12);
		case LFPOLY2:      return(13);
		case LFPOLY3:      return(14);
		case LFGATE:       return(15);
		case LFCONTACT1:   return(16);
		case LFMETAL1:     return(17);
		case LFCONTACT2:   return(18);
		case LFMETAL2:     return(19);
		case LFCONTACT3:   return(20);
		case LFMETAL3:     return(21);
		case LFCONTACT4:   return(22);
		case LFMETAL4:     return(23);
		case LFCONTACT5:   return(24);
		case LFMETAL5:     return(25);
		case LFCONTACT6:   return(26);
		case LFMETAL6:     return(27);
		case LFCONTACT7:   return(28);
		case LFMETAL7:     return(29);
		case LFCONTACT8:   return(30);
		case LFMETAL8:     return(32);
		case LFCONTACT9:   return(32);
		case LFMETAL9:     return(33);
		case LFCONTACT10:  return(34);
		case LFMETAL10:    return(35);
		case LFCONTACT11:  return(36);
		case LFMETAL11:    return(37);
		case LFCONTACT12:  return(38);
		case LFMETAL12:    return(39);
		case LFPLUG:       return(40);
		case LFOVERGLASS:  return(41);
		case LFBUS:        return(42);
		case LFART:        return(43);
		case LFCONTROL:    return(44);
	}
	return(35);
}

/*
 * Routine to return the non-pseudo layer associated with layer "layer" in
 * technology "tech".
 */
INTBIG nonpseudolayer(INTBIG layer, TECHNOLOGY *tech)
{
	REGISTER INTBIG fun, ofun, i, j, result;

	if (db_multiprocessing) emutexlock(db_convertpseudomutex);
	if (tech != db_convertpseudotech)
	{
		if (db_convertpseudoarray != 0) efree((CHAR *)db_convertpseudoarray);
		db_convertpseudoarray = (INTBIG *)emalloc(tech->layercount * SIZEOFINTBIG, db_cluster);
		if (db_convertpseudoarray == 0)
		{
			db_convertpseudotech = NOTECHNOLOGY;
			if (db_multiprocessing) emutexunlock(db_convertpseudomutex);
			return(layer);
		}
		for(i=0; i<tech->layercount; i++)
		{
			db_convertpseudoarray[i] = i;
			fun = layerfunction(tech, i);
			if ((fun&LFPSEUDO) == 0) continue;
			for(j=0; j<tech->layercount; j++)
			{
				ofun = layerfunction(tech, j);
				if (ofun == (fun & ~LFPSEUDO)) break;
			}
			if (j < tech->layercount) db_convertpseudoarray[i] = j;
		}
		db_convertpseudotech = tech;
	}
	result = db_convertpseudoarray[layer];
	if (db_multiprocessing) emutexunlock(db_convertpseudomutex);
	return(result);
}

/*
 * Routine to obtain 3D information about layer "layer" in technology "tech".  The 3D
 * height is stored in "height" and its thickness in "thickness".  Returns true on
 * error.
 */
BOOLEAN get3dfactors(TECHNOLOGY *tech, INTBIG layer, float *height, float *thickness)
{
	static BOOLEAN heightsetup = FALSE;
	REGISTER INTBIG len, i;
	REGISTER TECHNOLOGY *itech;
	REGISTER VARIABLE *varh, *vart;

	/* make sure that every technology has layer height information */
	if (!heightsetup)
	{
		heightsetup = TRUE;
		db_techlayer3dheightkey = makekey(x_("TECH_layer_3dheight"));
		db_techlayer3dthicknesskey = makekey(x_("TECH_layer_3dthickness"));
		for(itech = el_technologies; itech != NOTECHNOLOGY; itech = itech->nexttechnology)
		{
			varh = getvalkey((INTBIG)itech, VTECHNOLOGY, -1, db_techlayer3dheightkey);
			vart = getvalkey((INTBIG)itech, VTECHNOLOGY, -1, db_techlayer3dthicknesskey);
			if (varh == NOVARIABLE || vart == NOVARIABLE)
				tech_3ddefaultlayerheight(itech);
		}
	}

	if (tech != db_3dcurtech)
	{
		db_3dcurtech = tech;
		varh = getvalkey((INTBIG)db_3dcurtech, VTECHNOLOGY, -1, db_techlayer3dheightkey);
		vart = getvalkey((INTBIG)db_3dcurtech, VTECHNOLOGY, -1, db_techlayer3dthicknesskey);
		if (varh == NOVARIABLE || vart == NOVARIABLE)
		{
			db_3dcurtech = NOTECHNOLOGY;
			return(TRUE);
		}
		len = mini(getlength(varh), getlength(vart));
		if (len > db_3darraylength)
		{
			if (db_3darraylength > 0)
			{
				efree((CHAR *)db_3dheight);
				efree((CHAR *)db_3dthickness);
			}
			db_3darraylength = 0;
			db_3dheight = (float *)emalloc(len * sizeof (float), db_cluster);
			if (db_3dheight == 0) return(TRUE);
			db_3dthickness = (float *)emalloc(len * sizeof (float), db_cluster);
			if (db_3dthickness == 0) return(TRUE);
			db_3darraylength = len;
		}
		for(i=0; i<len; i++)
		{
			if ((varh->type&VTYPE) == VINTEGER) db_3dheight[i] = (float)((INTBIG *)varh->addr)[i]; else
				db_3dheight[i] = ((float *)varh->addr)[i];
			if ((vart->type&VTYPE) == VINTEGER) db_3dthickness[i] = (float)((INTBIG *)vart->addr)[i]; else
				db_3dthickness[i] = ((float *)vart->addr)[i];
		}
	}
	if (layer < 0 || layer >= tech->layercount) return(TRUE);

	*thickness = db_3dthickness[layer];
	*height = db_3dheight[layer];
	return(FALSE);
}

void set3dheight(TECHNOLOGY *tech, float *depth)
{
	setvalkey((INTBIG)tech, VTECHNOLOGY, db_techlayer3dheightkey, (INTBIG)depth,
		VFLOAT|VISARRAY|(tech->layercount<<VLENGTHSH));
	db_3dcurtech = NOTECHNOLOGY;
}

void set3dthickness(TECHNOLOGY *tech, float *thickness)
{
	setvalkey((INTBIG)tech, VTECHNOLOGY, db_techlayer3dthicknesskey, (INTBIG)thickness,
		VFLOAT|VISARRAY|(tech->layercount<<VLENGTHSH));
	db_3dcurtech = NOTECHNOLOGY;
}

/*
 * Routine to return true if layers "layer1" and "layer2" in technology "tech"
 * should be considered equivalent for the purposes of cropping.
 */
BOOLEAN samelayer(TECHNOLOGY *tech, INTBIG layer1, INTBIG layer2)
{
	static TECHNOLOGY *layertabtech = NOTECHNOLOGY;
	REGISTER INTBIG countsquared, i, first, second;
	REGISTER BOOLEAN result;
	INTBIG *equivlist;

	if (layer1 == layer2) return(TRUE);
	if (layer1 < 0 || layer2 < 0) return(FALSE);
	if (tech == NOTECHNOLOGY) return(FALSE);

	if (db_multiprocessing) emutexlock(db_layertabmutex);
	if (tech != layertabtech)
	{
		if (db_equivtable != 0) efree((CHAR *)db_equivtable);
		db_equivtable = 0;
		equivlist = (INTBIG *)asktech(tech, x_("get-layer-equivalences"));
		if (equivlist != 0)
		{
			countsquared = tech->layercount * tech->layercount;
			db_equivtable = (BOOLEAN *)emalloc(countsquared * (sizeof (BOOLEAN)),
				db_cluster);
			if (db_equivtable == 0)
			{
				if (db_multiprocessing) emutexunlock(db_layertabmutex);
				return(FALSE);
			}
			for(i=0; i<countsquared; i++) db_equivtable[i] = FALSE;
			while (*equivlist >= 0)
			{
				first = *equivlist++;
				second = *equivlist++;
				db_equivtable[first*tech->layercount + second] = TRUE;
				db_equivtable[second*tech->layercount + first] = TRUE;
			}
		}
		layertabtech = tech;
	}
	if (db_equivtable == 0) result = FALSE; else
		result = db_equivtable[layer1*tech->layercount + layer2];
	if (db_multiprocessing) emutexunlock(db_layertabmutex);
	return(result);
}

/*
 * Routine to establish default layer height and thickness for
 * technology "tech".
 */
void tech_3ddefaultlayerheight(TECHNOLOGY *tech)
{
	REGISTER INTBIG *layerheight, *layerthickness, i, funct;

	layerheight = (INTBIG *)emalloc(tech->layercount * SIZEOFINTBIG, el_tempcluster);
	layerthickness = (INTBIG *)emalloc(tech->layercount * SIZEOFINTBIG, el_tempcluster);
	for(i=0; i<tech->layercount; i++)
	{
		funct = layerfunction(tech, i) & LFTYPE;
		layerheight[i] = layerfunctionheight(funct);
		layerthickness[i] = 0;
		if (layeriscontact(funct)) layerthickness[i] = 2;
	}
	setvalkey((INTBIG)tech, VTECHNOLOGY, db_techlayer3dheightkey, (INTBIG)layerheight,
		VINTEGER|VISARRAY|(tech->layercount<<VLENGTHSH)|VDONTSAVE);
	setvalkey((INTBIG)tech, VTECHNOLOGY, db_techlayer3dthicknesskey, (INTBIG)layerthickness,
		VINTEGER|VISARRAY|(tech->layercount<<VLENGTHSH)|VDONTSAVE);
	efree((CHAR *)layerheight);
	efree((CHAR *)layerthickness);
}

/*
 * routine to tell the minimum distance between layers "layer1" and "layer2" in
 * technology "tech".  If "connected" is false, the two layers are not connected,
 * if it is true, they are connected electrically.  A negative return means
 * that the two layers can overlap.  If the distance is an edge rule, "edge"
 * is set nonzero.  If there is a rule associated with this
 * it is returned in "rule".  This routine accesses the database
 * variables "DRC_min_connected_distances" and "DRC_min_unconnected_distances"
 * in the technologies.
 */
INTBIG tech_getdrcmindistance(TECHNOLOGY *tech, INTBIG layer1, INTBIG layer2,
	BOOLEAN connected, INTBIG wide, BOOLEAN multicut, INTBIG *edge, CHAR **rule)
{
	REGISTER INTBIG pindex, temp, *addr, *edgeaddr, *multiaddr, *wideaddr, ti, dist;
	REGISTER CHAR **rules, **edgerules, **multirules, **widerules;

	if (layer1 < 0 || layer2 < 0) return(XX);

	/* make sure cache of information is valid */
	ti = tech->techindex;
	multiaddr = wideaddr = 0;
	if (connected)
	{
		if (tech_drcconndistance == 0)
		{
			tech_initmaxdrcsurround();
			if (tech_drcconndistance == 0) return(XX);
		}
		addr = tech_drcconndistance[ti];
		rules = tech_drccondistancerule[ti];
		edgeaddr = 0;
		if (wide != 0)
		{
			wideaddr = tech_drcconndistancew[ti];
			widerules = tech_drccondistancewrule[ti];
		}
		if (multicut)
		{
			multiaddr = tech_drcconndistancem[ti];
			multirules = tech_drccondistancemrule[ti];
		}
	} else
	{
		if (tech_drcuncondistance == 0)
		{
			tech_initmaxdrcsurround();
			if (tech_drcuncondistance == 0) return(XX);
		}
		addr = tech_drcuncondistance[ti];
		rules = tech_drcuncondistancerule[ti];
		edgeaddr = tech_drcedgedistance[ti];
		edgerules = tech_drcedgedistancerule[ti];
		if (wide != 0)
		{
			wideaddr = tech_drcuncondistancew[ti];
			widerules = tech_drcuncondistancewrule[ti];
		}
		if (multicut)
		{
			multiaddr = tech_drcuncondistancem[ti];
			multirules = tech_drcuncondistancemrule[ti];
		}
	}
	if (addr == 0 && edgeaddr == 0) return(XX);

	/* compute index into connectedness tables */
	if (layer1 > layer2) { temp = layer1; layer1 = layer2;  layer2 = temp; }
	pindex = (layer1+1) * (layer1/2) + (layer1&1) * ((layer1+1)/2);
	pindex = layer2 + tech->layercount * layer1 - pindex;

	/* presume the standard rule */
	*edge = 0;
	dist = addr[pindex];
	if (edgeaddr != 0 && edgeaddr[pindex] > dist)
	{
		dist = edgeaddr[pindex];
		rules = edgerules;
		*edge = 1;
	}
	if (rule != 0)
	{
		*rule = 0; 
		if (rules != 0 && rules[pindex][0] != 0) 
			*rule = rules[pindex];
	}

	/* see if the multi-rule is there and is worse */
	if (multiaddr != 0 && multiaddr[pindex] > dist)
	{
		dist = multiaddr[pindex];
		*edge = 0;
		if (rule != 0)
		{
			*rule = 0; 
			if (multirules != 0 && multirules[pindex][0] != 0) 
				*rule = multirules[pindex];
		}
	}

	/* see if the wide-rule is there and is worse */
	if (wideaddr != 0 && wideaddr[pindex] > dist)
	{
		dist = wideaddr[pindex];
		*edge = 0;
		if (rule != 0)
		{
			*rule = 0; 
			if (widerules != 0 && widerules[pindex][0] != 0) 
				*rule = widerules[pindex];
		}
	}

	return(dist);
}

/*
 * routine to tell the maximum distance around layer "layer" in
 * technology "tech" that needs to be looked at for design-rule
 * checking (using library "lib").  This routine accesses the
 * database variable "DRC_max_distances" in the technologies.
 */
INTBIG maxdrcsurround(TECHNOLOGY *tech, LIBRARY *lib, INTBIG layer)
{
	REGISTER INTBIG i, *addr;

	if (layer < 0 || layer >= tech->layercount) return(XX);

	/* make sure cache of information is valid */
	if (tech_drcmaxdistances == 0)
	{
		tech_initmaxdrcsurround();
		if (tech_drcmaxdistances == 0) return(XX);
	}

	addr = tech_drcmaxdistances[tech->techindex];
	if (addr == 0) return(XX);
	i = addr[layer];
	if (i < 0) return(XX);
	i = i*lib->lambda[tech->techindex]/WHOLE;
	return(i);
}

/*
 * routine to tell the minimum distance between two layers "layer1" and
 * "layer2" in technology "tech".  If "connected" is false, the two layers
 * are not connected, if it is true, they are connected electrically.
 * The layers reside in library "lib".
 * A negative return means that the two layers can overlap.
 */
INTBIG drcmindistance(TECHNOLOGY *tech, LIBRARY *lib, INTBIG layer1, INTBIG size1,
	INTBIG layer2, INTBIG size2, BOOLEAN connected, BOOLEAN multicut, INTBIG *edge, CHAR **rule)
{
	REGISTER INTBIG i, widerules, widelimit, lambda;

	/* determine whether or not wide rules are to be used */
	widerules = 0;
	lambda = lib->lambda[tech->techindex];
	widelimit = tech_drcwidelimit[tech->techindex] * lambda / WHOLE;
	if (size1 > widelimit || size2 > widelimit) widerules = 1;

	/* get the un-scaled distance */
	i = tech_getdrcmindistance(tech, layer1, layer2, connected, widerules, multicut, edge, rule);

	/* scale result for current lambda value */
	if (i > 0) i = i * lambda / WHOLE;
	return(i);
}

/*
 * routine to return the minimum width of layer "layer" in technology "tech", given
 * that it appears in library "lib".  The rule that determines this is placed in "rule"
 * (ignored if "rule" is zero, set to 0 if no rule).
 */
INTBIG drcminwidth(TECHNOLOGY *tech, LIBRARY *lib, INTBIG layer, CHAR **rule)
{
	REGISTER INTBIG *addr;
	REGISTER CHAR **rules;

	/* make sure cache of information is valid */
	if (tech_drcminwidth == 0)
	{
		tech_initmaxdrcsurround();
		if (tech_drcminwidth == 0) return(0);
	}

	addr = tech_drcminwidth[tech->techindex];
	rules = tech_drcminwidthrule[tech->techindex];
	if (addr == 0) return(XX);
	if (rule != 0)
	{
		*rule = 0; 
		if (rules != 0 && rules[layer][0] != 0) 
			*rule = rules[layer];
	}
	return(addr[layer]*lib->lambda[tech->techindex]/WHOLE);
}

/*
 * routine to return the minimum size of primitive node "np", given
 * that it appears in library "lib".  The size is placed in "sizex/sizey" and the
 * rule that determines this is placed in "rule" (ignored if "rule" is zero, set to 0 if no rule).
 */
void drcminnodesize(NODEPROTO *np, LIBRARY *lib, INTBIG *sizex, INTBIG *sizey, CHAR **rule)
{
	REGISTER INTBIG *addr, index;
	REGISTER CHAR **rules;
	REGISTER TECHNOLOGY *tech;

	/* default answers */
	*sizex = *sizey = XX;
	if (rule != 0) *rule = 0;

	/* make sure cache of information is valid */
	if (tech_drcminnodesize == 0)
	{
		tech_initmaxdrcsurround();
		if (tech_drcminnodesize == 0) return;
	}

	tech = np->tech;
	index = np->primindex - 1;
	if (index < 0) return;
	addr = tech_drcminnodesize[tech->techindex];
	rules = tech_drcminnodesizerule[tech->techindex];
	if (addr != 0)
	{
		if (rule != 0 && rules != 0 && rules[index][0] != 0) 
			*rule = rules[index];
		*sizex = addr[index*2] * lib->lambda[tech->techindex] / WHOLE;
		*sizey = addr[index*2+1] * lib->lambda[tech->techindex] / WHOLE;
	}
}

#ifdef SURROUNDRULES
#define MAXSURROUNDRULES 20

/*
 * Routine to return surround rules for layer "layer" of technology "tech".
 * Returns the number of surround rules, and for each one sets an entry in the
 * arrays "layers" (the layer that must surround this), "dist" (the distance of the surround)
 * and "rules" (the rule that describes this).
 */
INTBIG drcsurroundrules(TECHNOLOGY *tech, INTBIG layer, INTBIG **layers, INTBIG **dist, CHAR ***rules)
{
	REGISTER INTBIG *pairs, index, count, i;
	static INTBIG mylayer[MAXSURROUNDRULES];
	static INTBIG mydist[MAXSURROUNDRULES];
	static CHAR *myrule[MAXSURROUNDRULES];

	/* make sure cache of information is valid */
	if (tech_drcsurroundlayerpairs == 0)
	{
		tech_initmaxdrcsurround();
		if (tech_drcsurroundlayerpairs == 0) return(0);
	}

	index = tech->techindex;
	pairs = tech_drcsurroundlayerpairs[index];

	count = 0;
	for(i=0; pairs[i] != 0; i++)
	{
		if ((pairs[i]&0xFFFF) != layer) break;
		if (count >= MAXSURROUNDRULES) break;
		mylayer[count] = (pairs[i] >> 16) & 0xFFFF;
		mydist[count] = tech_drcsurrounddistances[index][i];
		myrule[count] = tech_drcsurroundrule[index][i];
		count++;
	}
	*layers = mylayer;
	*dist = mydist;
	*rules = myrule;
	return(count);
}
#endif

/******************** MATHEMATICS ********************/

/*
 * The transformation that is done here is from one range specification
 * to another.  The original range is from "low" to "high".  The computed
 * area is from "newlow" to "newhigh".  The obvious way to do this is:
 *   center = (low + high) / 2;
 *   size = high - low;
 *   *newlow = center + size*lowmul/WHOLE + lowsum*lambda/WHOLE;
 *   *newhigh = center + size*highmul/WHOLE + highsum*lambda/WHOLE;
 * where "center" is the center co-ordinate and the complex expression is
 * the transformation factors.  However, this method is unstable for odd
 * range extents because there is no correct integral co-ordinate for the
 * center of the area.  Try it on a null transformation of (-1,0).  The
 * correct code is more complex, but rounds the center computation twice
 * in case it is not integral, adjusting the numerator to round properly.
 * The negative test is basically an adjustment by the "sign extend" value.
 *
 */
void subrange(INTBIG low, INTBIG high, INTBIG lowmul, INTBIG lowsum, INTBIG highmul,
	INTBIG highsum, INTBIG *newlow, INTBIG *newhigh, INTBIG lambda)
{
	REGISTER INTBIG total, size;

	size = high - low;
	if ((total = low + high) < 0) total--;

	/*
	 * Because the largest 32-bit number is 2147483647 and because WHOLE
	 * is 120, the value of "size" cannot be larger than 2147483647/120
	 * (which is 17895697) or there may be rounding problems.  For these large
	 * numbers, use "muldiv".
	 */
	if (size > 17895697)
	{
		*newlow = total/2 + muldiv(size, lowmul, WHOLE) + lowsum*lambda/WHOLE;
		*newhigh = (total+1)/2 + muldiv(size, highmul, WHOLE) + highsum*lambda/WHOLE;
	} else
	{
		*newlow = total/2 + size*lowmul/WHOLE + lowsum*lambda/WHOLE;
		*newhigh = (total+1)/2 + size*highmul/WHOLE + highsum*lambda/WHOLE;
	}
}

/*
 * routine to perform a range calculation (similar to "subrange") but on
 * only one value rather than two.  The extent of the range is from "low"
 * to "high" and the value of lambda is "lambda".  The routine returns
 * the value (low+high)/2 + (high-low)*mul/WHOLE + sum*lambda/WHOLE.
 */
INTBIG getrange(INTBIG low, INTBIG high, INTBIG mul,INTBIG sum, INTBIG lambda)
{
	REGISTER INTBIG total;

	total = low + high;
	if (total < 0) total--;

	/*
	 * Because the largest 32-bit number is 2147483647 and because WHOLE
	 * is 120, the value of "high-low" cannot be larger than 2147483647/120
	 * (which is 17895697) or there may be rounding problems.  For these large
	 * numbers, use "muldiv".
	 */
	if (high-low > 17895697)
		return(total/2 + muldiv(high-low, mul, WHOLE) + sum*lambda/WHOLE);
	return(total/2 + (high-low)*mul/WHOLE + sum*lambda/WHOLE);
}
