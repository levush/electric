/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: routmaze.c
 * Maze routing
 * Written by: Glen M. Lawson
 * Modified by: Steven M. Rubin, Static Free Software
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
#if ROUTTOOL

#include "global.h"
#include "database.h"
#include "efunction.h"
#include "egraphics.h"
#include "usr.h"
#include "tecgen.h"
#include "rout.h"

#define MAXGRIDSIZE   1000		/* maximum size of maze */
#define BLOCKAGELIMIT   10		/* max grid points to "excavate" for initial grid access from a port */
#define MAXBUSPORT     256		/* maximum width of bus wire */

/* bit width of long word */
#define SRMAXLAYERS (INTBIG)(sizeof (UINTBIG) * 8)

/******************* SRREGION *******************/
/* Defines a routing region to the router */

#define NOSRREGION ((SRREGION*) 0)

typedef struct
{
	INTBIG           lx, ly; 				/* lower bound of the region */
	INTBIG           hx, hy;				/* upper bound of the region */
	struct _srlayer *layers[SRMAXLAYERS];	/* the array of layers */
	struct _srnet   *nets;					/* the list of nets */
} SRREGION;

/******************* SRGPT *******************/
/* Defines a single grid point structure. All ports, blockages, and
 * fixed nets are defined with the following bits (w/ SR_GSET). If
 * SR_GSET is off, then the grid point can be used by the router for
 * routing (and temporary structure).
 */
typedef UCHAR1 SRGPT;

/* common bit masks */
#define SR_GCLEAR	0xFF				/* clear all bits */
#define SR_GSET		0x80				/* grid set (permanent) */
#define SR_GNOVIA	0x40				/* no via is allowed */
#define SR_GWAVE	0x20				/* is a wavefront point */
#define SR_GPORT	0x10				/* is a port */

/* for maze cells */
#define SR_GMASK	0x0F				/* mask 4 bits */
#define SR_GMAX		15 					/* maximum mark value */
#define SR_GSTART	1					/* start value */

typedef enum
{
	SRVERTPREF,
	SRHORIPREF,
	SRALL
} SRDIRECTION;

typedef struct _srlayer
{
	UCHAR1          *hused, *vused;
	INTBIG           index;				/* the layer index (sort order) */
	UINTBIG          mask;				/* the layer mask */
	INTBIG           transx, transy;	/* translation value for grid */
	INTBIG           wid, hei;			/* the width and height of the grid */
	INTBIG           allocwid, allochei;/* the allocated width and height of the grid */
	INTBIG           allocarray;		/* the allocated size of the grid */
	INTBIG           gridx, gridy;		/* the grid units */
	INTBIG           lx, ly, hx, hy;	/* bounds of the current maze */
	SRGPT          **grids;				/* the two dimensional grid array */
	SRGPT           *array;				/* the grid array as linear data */
	struct _srlayer *up, *down;			/* up/down pointer to next layer */
	SRDIRECTION      dir;				/* allowed direction of routes */
} SRLAYER;

#define NOSRLAYER ((SRLAYER *) 0)

/******************* SRNET *******************/
/* Defines a net in a region. Note that no existing segments of a net is predefined.
 * Only ports are allowed. Not all nets in region need to be defined.
 */

#define NOSRNET ((SRNET *) 0)

typedef struct _srnet
{
	BOOLEAN         routed;				/* route state flag */
	SRREGION       *region;				/* the parent region */
	struct _srport *ports;				/* the list of ports on the net */
	struct _srpath *paths;				/* the list of paths */
	struct _srpath *lastpath;			/* the last path in the list */
	struct _srnet  *next;				/* next in the list of nets */
} SRNET;

/******************* SRPORT *******************/
/* Defines a port on a net. Bounds of the port are in grid units.
 * An index of 0 means all layers, 1 = 1st layer, 2 = 2nd layer, 4 - 3rd layer, etc.
 */

#define NOSRPORT ((SRPORT *) 0)

typedef struct _srport
{
	INTBIG            index;			/* the port index */
	UINTBIG           layers;			/* the layer mask */
	INTBIG            lx, ly, hx, hy;	/* bounds of the port */
	NODEINST         *ni;				/* node that this port comes from (may be NONODEINST) */
	PORTPROTO        *pp;				/* port on the originating node */
	struct _srport   *master;	     	/* the master (connected) port */
	struct _srpath   *paths;			/* the list of connected paths */
	struct _srpath   *lastpath;			/* the last path in the list */
	struct _srwavept *wavefront;		/* the current wave front */
	SRNET            *net;				/* the parent net */
	struct _srport   *next;				/* next in the list of ports */
} SRPORT;


/******************* SRPATH *******************/
/* Path segment definition, note x, y values are in world coordinates.
 * End always defines vias (even if the original technology does not have any).
 */

#define NOSRPATH ((SRPATH *) 0)

typedef enum { SREPORT, SREVIA, SREVIAUP, SREVIADN, SREEND } SREND;
typedef enum { SRPFIXED, SRPROUTED } SRPTYPE;
typedef struct _srpath
{
	INTBIG          x[2], y[2];			/* end points of the path */
	INTBIG          wx[2], wy[2];		/* end pt in world units */
	SRLAYER        *layer;				/* the layer of the path */
	SREND           end[2];				/* end style of the path */
	SRPTYPE         type;  				/* the type of path */
	SRPORT         *port;				/* the port path is attached to */
	struct _srpath *next;				/* next in the list of paths */
} SRPATH;

/* routing data types */
typedef struct _srwavept
{
	INTBIG            x, y;			    /* x y location of the point */
	SRLAYER          *layer;			/* the layer for the point */
	SRPORT           *port;				/* the port for this point */
	struct _srwavept *next, *prev;		/* next/prev in the list of points */
} SRWAVEPT;

#define NOSRWAVEPT ((SRWAVEPT *) 0)

/* miscellaneous defines, return codes from ro_mazeexpand_wavefront */
#define SRSUCCESS 	0
#define SRERROR		1
#define SRROUTED	2
#define SRBLOCKED	3
#define SRUNROUTED  4

typedef enum {SR_MOR, SR_MAND, SR_MSET} SRMODE;

#define SCH_HORILAYER 	0				/* draw only on vertical layer */
#define SCH_VERTLAYER 	1				/* draw only on horizontal layer */
#define SCH_ALLLAYER    2				/* draw all layers */

/* macro functions */
#define GRIDX(Mv, Ml) (((Mv) - Ml->transx) / Ml->gridx)
#define GRIDY(Mv, Ml) (((Mv) - Ml->transy) / Ml->gridy)
#define WORLDX(Mv, Ml) (((Mv) * Ml->gridx) + Ml->transx)
#define WORLDY(Mv, Ml) (((Mv) * Ml->gridy) + Ml->transy)

#define DRAW_BOX(Mlx,Mly,Mhx,Mhy,Ml)												\
{																					\
	if (Ml == SCH_HORILAYER || Ml == SCH_ALLLAYER)									\
		ro_mazeset_box(region,region->layers[SCH_HORILAYER],SR_GSET,Mlx,Mly,Mhx,Mhy,SR_MSET);	\
	if (Ml == SCH_VERTLAYER || Ml == SCH_ALLLAYER)									\
		ro_mazeset_box(region,region->layers[SCH_VERTLAYER],SR_GSET,Mlx,Mly,Mhx,Mhy,SR_MSET);	\
}

#define DRAW_LINE(Mlx,Mly,Mhx,Mhy,Ml)												\
{																					\
	if (Ml == SCH_HORILAYER || Ml == SCH_ALLLAYER)									\
		ro_mazeset_line(region,region->layers[SCH_HORILAYER],SR_GSET,Mlx,Mly,Mhx,Mhy,SR_MSET);	\
	if (Ml == SCH_VERTLAYER || Ml == SCH_ALLLAYER)									\
		ro_mazeset_line(region,region->layers[SCH_VERTLAYER],SR_GSET,Mlx,Mly,Mhx,Mhy,SR_MSET);	\
}

static BOOLEAN    ro_mazedebug = FALSE;
static ARCPROTO  *ro_mazevertwire, *ro_mazehoriwire;
static NODEPROTO *ro_mazesteiner;
static FILE      *ro_mazedebuggingout;
static SRREGION  *ro_theregion = NOSRREGION;
       INTBIG     ro_mazegridx, ro_mazegridy;
       INTBIG     ro_mazeoffsetx, ro_mazeoffsety;
       INTBIG     ro_mazeboundary;

/* database access functions */
static SRNET     *ro_mazenet_free_list = NOSRNET;
static SRPORT    *ro_mazeport_free_list = NOSRPORT;
static SRPATH    *ro_mazepath_free_list = NOSRPATH;
static SRWAVEPT  *ro_mazewavept_free_list = NOSRWAVEPT;

/* prototypes for local routines */
static INTBIG      ro_anglediff(INTBIG ang1, INTBIG ang2);
static TECHNOLOGY *ro_findtech(NODEINST *ni, PORTPROTO *pp);
static SRLAYER    *ro_mazeadd_layer(SRREGION *region, INTBIG index,  SRDIRECTION direction, INTBIG gridx, INTBIG gridy, INTBIG alignx, INTBIG aligny);
static SRNET      *ro_mazeadd_net(SRREGION *region);
static SRPORT     *ro_mazeadd_port(SRNET *net, UINTBIG layers, INTBIG wx1, INTBIG wy1, INTBIG wx2, INTBIG wy2, NODEINST *ni, PORTPROTO *pp);
static BOOLEAN     ro_mazeadd_wavept(SRPORT *port, SRLAYER *layer, INTBIG x, INTBIG y, INTBIG code);
static void        ro_mazeadjustpath(SRPATH *paths, SRPATH *thispath, INTBIG end, INTBIG dx, INTBIG dy);
static SRNET      *ro_mazealloc_net(void);
static SRPATH     *ro_mazealloc_path(void);
static SRPORT     *ro_mazealloc_port(void);
static SRWAVEPT   *ro_mazealloc_wavept(void);
static void        ro_mazeclear_maze(SRNET *net);
static void        ro_mazecreate_wavefront(SRPORT *port);
static SRREGION   *ro_mazedefine_region(NODEPROTO *np, INTBIG lx, INTBIG ly, INTBIG hx, INTBIG hy);
static void        ro_mazedelete_net(SRNET *net);
static void        ro_mazedelete_port(SRPORT *port);
static void        ro_mazecleanout_region(SRREGION *region);
static UINTBIG     ro_mazedetermine_dir(NODEINST *ni, INTBIG cx, INTBIG cy);
static void        ro_mazedraw_arcinst(ARCINST *ai, XARRAY trans, NETWORK *net, SRREGION *region);
static void        ro_mazedraw_cell(NODEINST *ni, XARRAY prevtrans, NETWORK *net, SRREGION *region);
static void        ro_mazedraw_nodeinst(NODEINST *ni, XARRAY prevtrans, NETWORK *net, SRREGION *region);
static void        ro_mazedraw_showpoly(POLYGON *obj, SRREGION *region, INTBIG layer);
static void        ro_mazedump_layer(CHAR *message, SRREGION *region, UINTBIG layers);
static INTBIG      ro_mazeexamine_pt(SRPORT *port, SRLAYER *layer, INTBIG x, INTBIG y, INTBIG code);
static INTBIG      ro_mazeexpand_wavefront(SRPORT *port, INTBIG code);
static BOOLEAN     ro_mazeextract_paths(NODEPROTO *parent, SRNET *net);
static INTBIG      ro_mazefind_paths(SRWAVEPT *wavept, SRPATH *path);
static INTBIG      ro_mazefindport(NODEPROTO*, INTBIG, INTBIG, ARCPROTO*, NODEINST *nis[], PORTPROTO *pps[], BOOLEAN);
static INTBIG      ro_mazefindport_geom(NODEPROTO *cell, INTBIG x, INTBIG y, ARCPROTO *ap, NODEINST *nis[], PORTPROTO *pps[], ARCINST **ai, BOOLEAN forcefind);
static void        ro_mazefree_net(SRNET *net);
static void        ro_mazefree_path(SRPATH *path);
static void        ro_mazefree_port(SRPORT *port);
static void        ro_mazefree_wavept(SRWAVEPT *wavept);
static SRPATH     *ro_mazeget_path(SRPORT *port, SRLAYER *layer, SREND e1, SREND e2, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2);
static SRREGION   *ro_mazeget_region(INTBIG wlx, INTBIG wly, INTBIG whx, INTBIG why, INTBIG gridx, INTBIG gridy, INTBIG offsetx, INTBIG offsety);
static INTBIG      ro_mazeinit_path(SRPORT *port, SRLAYER *layer, SRWAVEPT *wavept, INTBIG x, INTBIG y, INTBIG code);
static BOOLEAN     ro_mazeroute_net(SRNET *net);
static BOOLEAN     ro_mazeroutenet(NETWORK *net);
static SRWAVEPT   *ro_mazesearch_wavefront(SRPORT *port, SRLAYER *layer, INTBIG x, INTBIG y);
static void        ro_mazeset_box(SRREGION *region, SRLAYER *layer, SRGPT type, INTBIG wx1, INTBIG wy1, INTBIG wx2, INTBIG wy2, SRMODE mode);
static void        ro_mazeset_line(SRREGION *region, SRLAYER *layer, SRGPT type, INTBIG wx1, INTBIG wy1, INTBIG wx2, INTBIG wy2, SRMODE mode);
static void        ro_mazeset_point(SRLAYER *layer, SRGPT type, INTBIG x, INTBIG y, SRMODE mode);
static INTBIG      ro_mazetest_pt(SRGPT pt, INTBIG code);

/*
 * Routine to free all memory associated with this module.
 */
void ro_freemazememory(void)
{
	SRWAVEPT *wavept;
	SRPORT *port;
	SRPATH *path;
	REGISTER INTBIG i;
	SRLAYER *layer;
	SRNET *net;

	if (ro_theregion != NOSRREGION)
	{
		ro_mazecleanout_region(ro_theregion);
		for(i=0; i<SRMAXLAYERS; i++)
		{
			layer = ro_theregion->layers[i];
			if (layer == NOSRLAYER) continue;
			if (layer->grids != 0) efree((CHAR *)layer->grids);
			if (layer->vused != 0) efree((CHAR *)layer->vused);
			if (layer->hused != 0) efree((CHAR *)layer->hused);
			if (layer->array != 0) efree((CHAR *)layer->array);
			efree((CHAR *)layer);
		}
		efree((CHAR *)ro_theregion);
	}
	while (ro_mazenet_free_list != NOSRNET)
	{
		net = ro_mazenet_free_list;
		ro_mazenet_free_list = ro_mazenet_free_list->next;
		efree((CHAR *)net);
	}
	while (ro_mazewavept_free_list != NOSRWAVEPT)
	{
		wavept = ro_mazewavept_free_list;
		ro_mazewavept_free_list = ro_mazewavept_free_list->next;
		efree((CHAR *)wavept);
	}
	while (ro_mazeport_free_list != NOSRPORT)
	{
		port = ro_mazeport_free_list;
		ro_mazeport_free_list = ro_mazeport_free_list->next;
		efree((CHAR *)port);
	}
	while (ro_mazepath_free_list != NOSRPATH)
	{
		path = ro_mazepath_free_list;
		ro_mazepath_free_list = ro_mazepath_free_list->next;
		efree((CHAR *)path);
	}
}

/************************ CONTROL ***********************/

/*
 * Routine to replace the selected unrouted arcs with routed geometry
 */
void ro_mazerouteselected(void)
{
	GEOM **list;
	REGISTER NODEPROTO *np;
	REGISTER BOOLEAN found;
	REGISTER ARCINST *ai;
	REGISTER INTBIG selectcount, i;
	REGISTER NETWORK *net;

	/* see what is selected */
	list = us_gethighlighted(WANTARCINST, 0, 0);
	for(selectcount=0; list[selectcount] != NOGEOM; selectcount++) ;
	if (selectcount == 0)
	{
		ttyputerr(_("Select arcs to route"));
		return;
	}
	np = geomparent(list[0]);

	/* loop while searching for networks to route */
	found = TRUE;
	while (found)
	{
		found = FALSE;
		for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		{
			/* see if anything selected is on this net */
			for(i=0; i<selectcount; i++)
			{
				if (list[i] == NOGEOM) continue;
				ai = list[i]->entryaddr.ai;
				if (ai->network == net) break;
			}
			if (i >= selectcount) continue;

			/* found arc to route: now remove all on this network from list */
			for(i=0; i<selectcount; i++)
			{
				if (list[i] == NOGEOM) continue;
				ai = list[i]->entryaddr.ai;
				if (ai->network == net) list[i] = NOGEOM;
			}

			/* now route the network */
			if (ro_mazeroutenet(net)) return;
			found = TRUE;
			break;
		}
	}
}

/*
 * Routine to reroute every unrouted network in the cell.
 */
void ro_mazeroutecell(void)
{
	REGISTER NODEPROTO *np;
	REGISTER ARCINST *ai;

	np = us_needcell();
	if (np == NONODEPROTO) return;

	/* loop while searching for networks to route */
	for(;;)
	{
		/* look for an unrouted arc */
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			if (ai->proto == gen_unroutedarc) break;
		if (ai == NOARCINST) break;

		/* now route the network */
		if (ro_mazeroutenet(ai->network)) return;
	}
}

/*
 * Routine to reroute networks "net".  Returns true on error.
 */
BOOLEAN ro_mazeroutenet(NETWORK *net)
{
	INTBIG lx, ly, hx, hy, lambda;
	INTBIG i, first;
	NODEPROTO *np;
	NODEINST **nilist;
	PORTPROTO **pplist;
	TECHNOLOGY *tech;
	INTBIG *xplist, *yplist, count;
	NODEINST *ni, *nextni;
	ARCINST *ai, *nextai;
	CHAR *dummy;
	PORTPROTO *pp;
	SRREGION *region;
	SRNET *srnet;
	SRPORT *fsp;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, ro_tool->cluster);

	/* get extent of net and mark nodes and arcs on it */
	count = ro_findnetends(net, &nilist, &pplist, &xplist, &yplist);
	if (count != 2)
	{
		ttyputerr(_("Can only route nets with 2 ends, this has %ld"), count);
		return(TRUE);
	}

	/* determine technology and grid size */
	for(i=0; i<count; i++)
	{
		tech = ro_findtech(nilist[i], pplist[i]);
		lambda = nilist[i]->parent->lib->lambda[tech->techindex];
		if (i == 0) ro_mazegridx = lambda; else
		{
			if (lambda < ro_mazegridx) ro_mazegridx = lambda;
		}
	}
	ro_mazegridy = ro_mazegridx;
	ro_mazeboundary = ro_mazegridx * 20;

	/* determine bounds of this networks */
	np = net->parent;
	first = 1;
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->network != net) continue;
		if (first != 0)
		{
			first = 0;
			lx = ai->geom->lowx;   hx = ai->geom->highx;
			ly = ai->geom->lowy;   hy = ai->geom->highy;
		} else
		{
			if (ai->geom->lowx < lx) lx = ai->geom->lowx;
			if (ai->geom->highx > hx) hx = ai->geom->highx;
			if (ai->geom->lowy < ly) ly = ai->geom->lowy;
			if (ai->geom->highy > hy) hy = ai->geom->highy;
		}
	}
	if (first != 0)
	{
		ttyputerr(_("Internal error: no bounding area for routing"));
		return(TRUE);
	}

	/* turn off highlighting */
	us_clearhighlightcount();

	/* remove marked networks */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = nextai)
	{
		nextai = ai->nextarcinst;
		if (ai->temp1 == 0) continue;
		startobjectchange((INTBIG)ai, VARCINST);
		if (killarcinst(ai))
		{
			ttyputerr(_("Error deleting original arc"));
			return(TRUE);
		}
	}
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = nextni)
	{
		nextni = ni->nextnodeinst;
		if (ni->temp1 == 0) continue;
		startobjectchange((INTBIG)ni, VNODEINST);
		if (killnodeinst(ni))
		{
			ttyputerr(_("Error deleting intermediate pin"));
			return(TRUE);
		}
	}

	/* now create the routing region */
	region = ro_mazedefine_region(np, lx, ly, hx, hy);
	if (region == NOSRREGION) return(TRUE);

	/* create the net in the region */
	srnet = ro_mazeadd_net(region);
	if (srnet == NOSRNET)
	{
		ttyputerr(_("Could not allocate internal net"));
		return(TRUE);
	}

	/* add the ports to the net */
	for(i=0; i<count; i++)
	{
		ni = nilist[i];
		pp = pplist[i];
		lx = hx = xplist[i];
		ly = hy = yplist[i];
		fsp = ro_mazeadd_port(srnet, ro_mazedetermine_dir(ni, (lx+hx)/2, (ly+hy)/2),
			lx, ly, hx, hy, ni, pp);
		if (fsp == NOSRPORT)
		{
			ttyputerr(_("Port could not be defined"));
			return(TRUE);
		}
	}

	/* do maze routing */
	if (ro_mazedebug)
	{
		ro_mazedebuggingout = xcreate(x_("routing"), el_filetypetext, 0, &dummy);
		if (ro_mazedebuggingout == NULL) return(TRUE);
		ro_mazedump_layer(_("before"), region, (UINTBIG)3);
	}
	if (ro_mazeroute_net(srnet))
	{
		ttyputerr(_("Could not route net"));
		return(TRUE);
	}
	if (ro_mazedebug)
	{
		ro_mazedump_layer(_("after"), region, (UINTBIG)3);
		xclose(ro_mazedebuggingout);
	}

	/* extract paths to create arcs */
	if (ro_mazeextract_paths(np, srnet))
	{
		ttyputerr(_("Could not create paths"));
		return(TRUE);
	}

	/* ensure that the cell is sized right and the networks are reevaluated */
	db_endbatch();
	setactivity(_("Maze routing"));
	return(FALSE);
}

/* internal helper routines */

SRREGION *ro_mazedefine_region(NODEPROTO *np, INTBIG lx, INTBIG ly, INTBIG hx, INTBIG hy)
{
	SRREGION *region;
	XARRAY trans;
	GEOM *look;
	INTBIG search;

	/* determine routing region bounds */
	lx = lx - ro_mazegridx - ro_mazeboundary;
	hx = hx + ro_mazegridx + ro_mazeboundary;
	ly = ly - ro_mazegridy - ro_mazeboundary;
	hy = hy + ro_mazegridy + ro_mazeboundary;

	/* allocate region and layers */
	region = ro_mazeget_region(lx, ly, hx, hy, ro_mazegridx, ro_mazegridy, ro_mazeoffsetx, ro_mazeoffsety);
	if (region == NOSRREGION)
	{
		ttyputerr(_("Could not allocate routing region (%ld<=X<=%ld %ld<=Y<=%ld)"), lx, hx, ly, hy);
		return NOSRREGION;
	}
	if (stopping(STOPREASONROUTING)) return(NOSRREGION);

	/* search region for nodes/arcs to add to database */
	search = initsearch(lx, hx, ly, hy, np);
	transid(trans);
	while ((look = nextobject(search)) != NOGEOM)
	{
		if (stopping(STOPREASONROUTING)) return(NOSRREGION);
		if (look->entryisnode)
		{
			/* draw this cell */
			ro_mazedraw_cell(look->entryaddr.ni, trans, NONETWORK, region);
		} else
		{
			/* draw this arc */
			ro_mazedraw_arcinst(look->entryaddr.ai, trans, NONETWORK, region);
		}
	}
	return region;
}

/*
 * Routine to recursively adjust paths to account for port positions that may not
 * be on the grid.
 */
void ro_mazeadjustpath(SRPATH *paths, SRPATH *thispath, INTBIG end, INTBIG dx, INTBIG dy)
{
	SRPATH *opath;
	INTBIG formerx, formery, formerthis, formerother;

	if (dx != 0)
	{
		formerthis = thispath->wx[end];
		formerother = thispath->wx[1-end];
		thispath->wx[end] += dx;
		if (formerthis == formerother)
		{
			formerx = thispath->wx[1-end];
			formery = thispath->wy[1-end];
			thispath->wx[1-end] += dx;
			for (opath = paths; opath != NOSRPATH; opath = opath->next)
			{
				if (opath->wx[0] == formerx && opath->wy[0] == formery)
				{
					ro_mazeadjustpath(paths, opath, 0, dx, 0);
					break;
				}
				if (opath->wx[1] == formerx && opath->wy[1] == formery)
				{
					ro_mazeadjustpath(paths, opath, 1, dx, 0);
					break;
				}
			}
		}
	}

	if (dy != 0)
	{
		formerthis = thispath->wy[end];
		formerother = thispath->wy[1-end];
		thispath->wy[end] += dy;
		if (formerthis == formerother)
		{
			formerx = thispath->wx[1-end];
			formery = thispath->wy[1-end];
			thispath->wy[1-end] += dy;
			for (opath = paths; opath != NOSRPATH; opath = opath->next)
			{
				if (opath->wx[0] == formerx && opath->wy[0] == formery)
				{
					ro_mazeadjustpath(paths, opath, 0, 0, dy);
					break;
				}
				if (opath->wx[1] == formerx && opath->wy[1] == formery)
				{
					ro_mazeadjustpath(paths, opath, 1, 0, dy);
					break;
				}
			}
		}
	}
}

/* routing grid database routines */
BOOLEAN ro_mazeextract_paths(NODEPROTO *parent, SRNET *net)
{
	SRPATH *path;
	SRPORT *port;
	NODEINST *fni[MAXBUSPORT], *tni[MAXBUSPORT];
	PORTPROTO *fpp[MAXBUSPORT], *tpp[MAXBUSPORT];
	ARCINST *ai;
	ARCPROTO *ap;
	INTBIG fcnt, tcnt, fx, fy, tx, ty, ofx, ofy, bits, xs, ys, xc, yc;
	static POLYGON *poly = NOPOLYGON;

	(void)needstaticpolygon(&poly, 4, ro_tool->cluster);

	/* adjust paths to account for precise port location */
	for (path = net->paths; path != NOSRPATH; path = path->next)
	{
		if (path->type == SRPFIXED) continue;
		port = path->port;
		fx = ofx = path->wx[0];   fy = ofy = path->wy[0];
		if (path->end[0] == SREPORT && port->ni != NONODEINST)
		{
			if (fx < port->lx) fx = port->lx;
			if (fx > port->hx) fx = port->hx;
			if (fy < port->ly) fy = port->ly;
			if (fy > port->hy) fy = port->hy;
			if (fx != ofx || fy != ofy)
				ro_mazeadjustpath(net->paths, path, 0, fx-ofx, fy-ofy);
		} else
		{
			ap = path->layer->index ? ro_mazevertwire : ro_mazehoriwire;
			fcnt = ro_mazefindport(parent, fx, fy, ap, fni, fpp, TRUE);
			if (fcnt > 0)
			{
				shapeportpoly(fni[0], fpp[0], poly, FALSE);
				closestpoint(poly, &fx, &fy);
				if (fx != ofx || fy != ofy)
					ro_mazeadjustpath(net->paths, path, 0, fx-ofx, fy-ofy);
			}
		}

		fx = ofx = path->wx[1];   fy = ofy = path->wy[1];
		if (path->end[1] == SREPORT && port->ni != NONODEINST)
		{
			if (fx < port->lx) fx = port->lx;
			if (fx > port->hx) fx = port->hx;
			if (fy < port->ly) fy = port->ly;
			if (fy > port->hy) fy = port->hy;
			if (fx != ofx || fy != ofy)
				ro_mazeadjustpath(net->paths, path, 1, fx-ofx, fy-ofy);
		} else
		{
			ap = path->layer->index ? ro_mazevertwire : ro_mazehoriwire;
			fcnt = ro_mazefindport(parent, fx, fy, ap, fni, fpp, TRUE);
			if (fcnt > 0)
			{
				shapeportpoly(fni[0], fpp[0], poly, FALSE);
				closestpoint(poly, &fx, &fy);
				if (fx != ofx || fy != ofy)
					ro_mazeadjustpath(net->paths, path, 1, fx-ofx, fy-ofy);
			}
		}
	}

	for (path = net->paths; path != NOSRPATH; path = path->next)
	{
		if (path->type == SRPFIXED) continue;
		ap = path->layer->index ? ro_mazevertwire : ro_mazehoriwire;

		/* create arc between the end points */
		fx = path->wx[0];   fy = path->wy[0];
		fcnt = ro_mazefindport(parent, fx, fy, ap, fni, fpp, FALSE);
		if (fcnt == 0)
		{
			/* create the from pin */
			defaultnodesize(ro_mazesteiner, &xs, &ys);
			xc = fx + (ro_mazesteiner->lowx + ro_mazesteiner->highx) / 2;
			yc = fy + (ro_mazesteiner->lowy + ro_mazesteiner->highy) / 2;
			fni[0] = newnodeinst(ro_mazesteiner, xc-xs/2, xc+xs/2,
				yc-ys/2, yc+ys/2, 0, 0, parent);
			if (fni[0] == NONODEINST)
			{
				ttyputerr(_("Could not create pin"));
				return(TRUE);
			}
			endobjectchange((INTBIG)fni[0], VNODEINST);
			fpp[0] = ro_mazesteiner->firstportproto;
		}

		tx = path->wx[1];   ty = path->wy[1];
		tcnt = ro_mazefindport(parent, tx, ty, ap, tni, tpp, FALSE);
		if (tcnt == 0)
		{
			/* create the from pin */
			defaultnodesize(ro_mazesteiner, &xs, &ys);
			xc = tx + (ro_mazesteiner->lowx + ro_mazesteiner->highx) / 2;
			yc = ty + (ro_mazesteiner->lowy + ro_mazesteiner->highy) / 2;
			tni[0] = newnodeinst(ro_mazesteiner, xc-xs/2, xc+xs/2,
				yc-ys/2, yc+ys/2, 0, 0, parent);
			if (tni[0] == NONODEINST)
			{
				ttyputerr(_("Could not create pin"));
				return(TRUE);
			}
			endobjectchange((INTBIG)tni[0], VNODEINST);
			tpp[0] = ro_mazesteiner->firstportproto;
		}

		/* now connect (note only nodes for now, no bus like connections) */
		if (fni[0] != NONODEINST && tni[0] != NONODEINST)
		{
			/* now make the connection (simple wire to wire for now) */
			bits = us_makearcuserbits(ap);
			ai = newarcinst(ap, defaultarcwidth(ap), bits, fni[0], fpp[0],
				fx, fy, tni[0], tpp[0], tx, ty, parent);
			if (ai == NOARCINST)
			{
				ttyputerr(_("Could not create path (arc)"));
				return(TRUE);
			}
			endobjectchange((INTBIG)ai, VARCINST);
			if (ai->network != NONETWORK) ai->network->temp1 = 2;
		}
	}

	return(FALSE);
}

UINTBIG ro_mazedetermine_dir(NODEINST *ni, INTBIG cx, INTBIG cy)
{
	INTBIG ncx, ncy;
	double dx, dy, pdx, pdy, area;

	if (ni == NONODEINST) return 3;

	/* get the center of the NODEINST */
	ncx = (ni->geom->lowx + ni->geom->highx) / 2;
	ncy = (ni->geom->lowy + ni->geom->highy) / 2;

	/* center, all edges */
	if (ncx == cx && ncy == cy) return 3; /* all layers */
	dx = ni->geom->highx - ncx; dy = ni->geom->highy - ncy;
	pdx = abs(cx - ncx); pdy = abs(cy - ncy);

	/* consider a point on a triangle, if left/right the seq center, port,
	 * upper left/right edge will be counter-clockwise, if top/bottom the seq.
	 * will be clock-wise :
	 * x1 * y2 + x2 * y3 + x3 * y1 - y1 * x2 - y2 * x3 - y3 * x1 == abs(area)
	 * where area < 0 == clockwise, area > 0 == counter-clockwise
	 */
	area = pdx * dy - pdy * dx;

	if (area > 0.0) return 1;		/* horizontal */
	if (area < 0.0) return 2;		/* vertical */
	return 3;						/* corner, all layers */
}

/* module: ro_mazedraw_showpoly
 * function: will write polys into EDIF syntax
 * inputs: poly
 */
void ro_mazedraw_showpoly(POLYGON *obj, SRREGION *region, INTBIG layer)
{
	INTBIG i, lx, ux, ly, uy, six, siy;
	void io_compute_center(INTBIG xc, INTBIG yc, INTBIG x1, INTBIG y1,
		INTBIG x2, INTBIG y2, INTBIG *cx, INTBIG *cy);

	/* now draw the polygon */
	switch (obj->style)
	{
		case CIRCLE: case THICKCIRCLE:
		case DISC:						/* a circle */
			i = computedistance(obj->xv[0], obj->yv[0], obj->xv[1], obj->yv[1]);
			lx = obj->xv[0] - i; ly = obj->yv[0] - i;
			ux = obj->xv[0] + i; uy = obj->yv[0] + i;
			DRAW_BOX(lx, ly, ux, uy, layer);
			break;

		case CIRCLEARC: case THICKCIRCLEARC:
			/* arcs at [i] points [1+i] [2+i] clockwise */
			if (obj->count == 0) break;
			if ((obj->count % 3) != 0) break;
			for (i = 0; i < obj->count; i += 3)
			{
				io_compute_center(obj->xv[i], obj->yv[i], obj->xv[i+1], obj->yv[i+1],
					obj->xv[i+2], obj->yv[i+2], &six, &siy);
				DRAW_LINE(obj->xv[i+1], obj->yv[i+1], six, siy, layer);
				DRAW_LINE(six, siy, obj->xv[i+2], obj->yv[i+2], layer);
			}
			break;

		case FILLED:						/* filled polygon */
		case FILLEDRECT:					/* filled rectangle */
		case CLOSED:						/* closed polygon outline */
		case CLOSEDRECT:					/* closed rectangle outline */
			if (isbox(obj, &lx, &ux, &ly, &uy))
			{
				DRAW_BOX(lx, ly, ux, uy, layer);
				break;
			}
			for (i = 1; i < obj->count; i++)
			{
				DRAW_LINE(obj->xv[i-1], obj->yv[i-1], obj->xv[i], obj->yv[i], layer);
			}

			/* close the region */
			if (obj->count > 2)
			{
				DRAW_LINE(obj->xv[obj->count-1], obj->yv[obj->count-1],
					obj->xv[0], obj->yv[0], layer);
			}
			break;

		case OPENED:						/* opened polygon outline */
		case OPENEDT1:						/* opened polygon outline, texture 1 */
		case OPENEDT2:						/* opened polygon outline, texture 2 */
		case OPENEDT3:						/* opened polygon outline, texture 3 */
		case OPENEDO1:						/* extended opened polygon outline */
			if (isbox(obj, &lx, &ux, &ly, &uy))
			{
				DRAW_BOX(lx, ly, ux, uy, layer);
				break;
			}
			for (i = 0; i < obj->count; i++)
			{
				DRAW_LINE(obj->xv[i], obj->yv[i], obj->xv[i+1], obj->yv[i+1], layer);
			}
			break;

		case VECTORS:
			for (i = 0; i < obj->count; i += 2)
			{
				DRAW_LINE(obj->xv[i], obj->yv[i], obj->xv[i+1], obj->yv[i+1], layer);
			}
			break;
	}
}


/*
 * routine to draw an arcinst.  Returns indicator of what else needs to
 * be drawn.  Returns negative if display interrupted
 */
void ro_mazedraw_arcinst(ARCINST *ai, XARRAY trans, NETWORK *net, SRREGION *region)
{
	INTBIG i, j;
	INTBIG width;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, ro_tool->cluster);

	/* if selected net or group route and temp1 is set */
	if (net != NONETWORK && ai->network == net) return;
	if (net == NONETWORK)
	{
		if (ai->network != NONETWORK && ai->network->temp1 == 1) return;
	}

	/* get the polygons of the arcinst */
	i = arcpolys(ai, NOWINDOWPART);
	for (j = 0; j < i; j++)
	{
		/* generate a polygon, force line for path generation */
		width = ai->width;
		ai->width = 0;
		shapearcpoly(ai, j, poly);
		ai->width = width;

		/* transform the polygon */
		xformpoly(poly, trans);

		/* draw the polygon */
		if (poly->xv[0] == poly->xv[1])
			ro_mazedraw_showpoly(poly, region, SCH_VERTLAYER); else
				if (poly->yv[0] == poly->yv[1])
					ro_mazedraw_showpoly(poly, region, SCH_HORILAYER); else
						ro_mazedraw_showpoly(poly, region, SCH_ALLLAYER);
	}
}

/*
 * routine to symbol "ni" when transformed through "prevtrans".
 */
void ro_mazedraw_nodeinst(NODEINST *ni, XARRAY prevtrans, NETWORK *net, SRREGION *region)
{
	INTBIG j, high;
	XARRAY localtran, subrot;
	INTBIG bx, by, ux, uy, swap;
	static POLYGON *poly = NOPOLYGON;
	GRAPHICS *gra;
	NODEPROTO *np;
	NODEINST *ino;
	ARCINST *iar;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, ro_tool->cluster);

	/* don't draw invisible pins */
	np = ni->proto;
	if (np == gen_invispinprim) return;

	/* get outline of nodeinst in the window */
	xform(ni->lowx, ni->lowy, &bx, &by, prevtrans);
	xform(ni->highx, ni->highy, &ux, &uy, prevtrans);
	if (bx > ux) { swap = bx;   bx = ux;   ux = swap; }
	if (by > uy) { swap = by;   by = uy;   uy = swap; }

	if (np->primindex != 0)
	{
		/* primitive nodeinst: ask the technology how to draw it */
		high = nodepolys(ni, 0, NOWINDOWPART);
		for (j = 0; j < high; j++)
		{
			/* get description of this layer */
			shapenodepoly(ni, j, poly);

			/* ignore if this layer is not being displayed */
			gra = poly->desc;
			if ((gra->colstyle & INVISIBLE) != 0) continue;

			/* draw the nodeinst */
			xformpoly(poly, prevtrans);

			/* draw the nodeinst and restore the color */
			ro_mazedraw_showpoly(poly, region, SCH_ALLLAYER);
		}
	} else
	{
		/* draw cell rectangle */
		maketruerectpoly(ni->lowx, ni->highx, ni->lowy, ni->highy, poly);
		poly->style = CLOSEDRECT;
		xformpoly(poly, prevtrans);
		ro_mazedraw_showpoly(poly, region, SCH_ALLLAYER);

		/* transform into the cell for display of its guts */
		maketrans(ni, localtran);
		transmult(localtran, prevtrans, subrot);

		/* search through cell */
		for (ino = np->firstnodeinst; ino != NONODEINST; ino = ino->nextnodeinst)
			ro_mazedraw_cell(ino, subrot, net, region);
		for (iar = np->firstarcinst; iar != NOARCINST; iar = iar->nextarcinst)
			ro_mazedraw_arcinst(iar, subrot, net, region);
	}
}

/* module: ro_mazedraw_cell
 * function: will output a specific symbol cell
 */
void ro_mazedraw_cell(NODEINST *ni, XARRAY prevtrans, NETWORK *net, SRREGION *region)
{
	XARRAY localtran, trans;

	if (ni->proto == ro_mazesteiner && ni->firstportarcinst != NOPORTARCINST &&
		((net == NONETWORK && ni->firstportarcinst->conarcinst->network->temp1) ||
		(net != NONETWORK && ni->firstportarcinst->conarcinst->network == net))) return;

	/* make transformation matrix within the current nodeinst */
	if (ni->rotation == 0 && ni->transpose == 0)
	{
		ro_mazedraw_nodeinst(ni, prevtrans, net, region);
	} else
	{
		makerot(ni, localtran);
		transmult(localtran, prevtrans, trans);
		ro_mazedraw_nodeinst(ni, trans, net, region);
	}
}

/* module: ro_mazefindport_geom
   function: will locate the nodeinstance and portproto corresponding to
   to a direct intersection with the given point.
   inputs:
   cell - cell to search
   x, y  - the point to exam
   ap    - the arc used to connect port (must match pp)
   nis   - pointer to ni pointer buffer.
   pps   - pointer to portproto pointer buffer.
   outputs:
   returns cnt if found, 0 not found, -1 on error
   ni = found ni instance
   pp = found pp proto.
 */
INTBIG ro_mazefindport_geom(NODEPROTO *cell, INTBIG x, INTBIG y, ARCPROTO *ap,
	NODEINST *nis[], PORTPROTO *pps[], ARCINST **ai, BOOLEAN forcefind)
{
	REGISTER INTBIG j, cnt, sea, dist, bestdist;
	REGISTER GEOM *geom;
	static POLYGON *poly = NOPOLYGON;
	REGISTER PORTPROTO *pp, *closestpp;
	REGISTER NODEINST *ni, *closestni;

	(void)needstaticpolygon(&poly, 4, ro_tool->cluster);

	cnt = 0;
	closestni = NONODEINST;
	bestdist = 0;
	closestpp = NOPORTPROTO;
	sea = initsearch(x, x, y, y, cell);
	for(;;)
	{
		geom = nextobject(sea);
		if (geom == NOGEOM) break;

		switch (geom->entryisnode)
		{
			case TRUE:
				ni = geom->entryaddr.ni;
				if (ni->proto->primindex != 0 &&
					(((ni->proto->userbits & NFUNCTION) >> NFUNCTIONSH) == NPPIN) &&
					ni->firstportarcinst != NOPORTARCINST &&
					(ni->firstportarcinst->conarcinst->network != NONETWORK &&
					ni->firstportarcinst->conarcinst->network->temp1 < 2)) break;

				/* now locate a portproto */
				for (pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				{
					shapeportpoly(ni, pp, poly, FALSE);
					if (isinside(x, y, poly))
					{
						/* check if port connects to arc ...*/
						for (j = 0; pp->connects[j] != NOARCPROTO; j++)
							if (pp->connects[j] == ap)
						{
							nis[cnt] = ni;
							pps[cnt] = pp;
							cnt++;
						}
					} else
					{
						dist = polydistance(poly, x, y);

						/* LINTED "bestdist" used in proper order */
						if (closestni == NONODEINST || dist < bestdist)
						{
							bestdist = dist;
							closestpp = pp;
							closestni = ni;
						}
					}
				}
				break;

			case FALSE:
				if ((geom->entryaddr.ai->network != NONETWORK &&
					geom->entryaddr.ai->network->temp1 != 2)) break;
				*ai = geom->entryaddr.ai;
				break;
		}
	}

	if (cnt == 0 && forcefind && closestni != NONODEINST && bestdist < ro_mazegridx)
	{
		nis[cnt] = closestni;
		pps[cnt] = closestpp;
		cnt++;
	}
	return cnt;
}

INTBIG ro_mazefindport(NODEPROTO *cell, INTBIG x, INTBIG y, ARCPROTO *ap, NODEINST *nis[],
	PORTPROTO *pps[], BOOLEAN forcefind)
{
	INTBIG cnt;
	ARCINST *ai = NOARCINST, *ar1,*ar2;
	ARCPROTO *nap;
	NODEINST *ni, *fno, *tno;
	PORTPROTO *fpt, *tpt, *pp;
	NODEPROTO *np, *pnt;
	INTBIG wid, bits1, bits2, lx, hx, ly, hy, j, xs, ys;
	INTBIG fendx, fendy, tendx, tendy;

	cnt = ro_mazefindport_geom(cell, x, y, ap, nis, pps, &ai, forcefind);

	if (cnt == 0 && ai != NOARCINST)
	{
		/* direct hit on an arc, verify connection */
		nap = ai->proto;
		np = getpinproto(nap);
		if (np == NONODEPROTO) return 0;
		pp = np->firstportproto;
		for (j = 0; pp->connects[j] != NOARCPROTO; j++)
			if (pp->connects[j] == ap) break;
		if (pp->connects[j] == NOARCPROTO) return 0;

		/* try to split arc (from us_getnodeonarcinst)*/
		/* break is at (prefx, prefy): save information about the arcinst */
		fno = ai->end[0].nodeinst;	fpt = ai->end[0].portarcinst->proto;
		tno = ai->end[1].nodeinst;	tpt = ai->end[1].portarcinst->proto;
		fendx = ai->end[0].xpos;	  fendy = ai->end[0].ypos;
		tendx = ai->end[1].xpos;	  tendy = ai->end[1].ypos;
		wid = ai->width;  pnt = ai->parent;
		bits1 = bits2 = ai->userbits;
		if ((bits1&ISNEGATED) != 0)
		{
			if ((bits1&REVERSEEND) == 0) bits2 &= ~ISNEGATED; else
				bits1 &= ~ISNEGATED;
		}
		if (figureangle(fendx,fendy, x,y) != figureangle(x,y, tendx, tendy))
		{
			bits1 &= ~FIXANG;
			bits2 &= ~FIXANG;
		}

		/* create the splitting pin */
		defaultnodesize(np, &xs, &ys);
		lx = x - xs/2;   hx = lx + xs;
		ly = y - ys/2;   hy = ly + ys;
		ni = newnodeinst(np, lx,hx, ly,hy, 0, 0, pnt);
		if (ni == NONODEINST)
		{
			ttyputerr(_("Cannot create splitting pin"));
			return 0;
		}
		endobjectchange((INTBIG)ni, VNODEINST);

		/* set the node, and port */
		nis[cnt] = ni;
		pps[cnt++] = pp;

		/* create the two new arcinsts */
		ar1 = newarcinst(nap, wid, bits1, fno, fpt, fendx, fendy, ni, pp, x, y, pnt);
		ar2 = newarcinst(nap, wid, bits2, ni, pp, x, y, tno, tpt, tendx, tendy, pnt);
		if (ar1 == NOARCINST || ar2 == NOARCINST)
		{
			ttyputerr(_("Error creating the split arc parts"));
			return cnt;
		}
		(void)copyvars((INTBIG)ai, VARCINST, (INTBIG)ar1, VARCINST, FALSE);
		endobjectchange((INTBIG)ar1, VARCINST);
		endobjectchange((INTBIG)ar2, VARCINST);

		/* delete the old arcinst */
		startobjectchange((INTBIG)ai, VARCINST);
		if (killarcinst(ai))
			ttyputerr(_("Error deleting original arc"));
	}

	return cnt;
}

/************************ MAZE ROUTING SUPPORT ***********************/

SRNET *ro_mazealloc_net(void)
{
	SRNET *net;

	if (ro_mazenet_free_list != NOSRNET)
	{
		net = ro_mazenet_free_list;
		ro_mazenet_free_list = net->next;
	} else
	{
		net = (SRNET *)emalloc(sizeof (SRNET), ro_tool->cluster);
		if (net == 0) return(NOSRNET);
	}
	return net;
}

void ro_mazefree_net(SRNET *net)
{
	net->next = ro_mazenet_free_list;
	ro_mazenet_free_list = net;
}

void ro_mazedelete_net(SRNET *net)
{
	SRPORT *port;
	SRPATH *path;

	while (net->ports != NOSRPORT)
	{
		port = net->ports;
		net->ports = net->ports->next;
		ro_mazedelete_port(port);
	}
	while (net->paths != NOSRPATH)
	{
		path = net->paths;
		net->paths = net->paths->next;
		ro_mazefree_path(path);
	}
	ro_mazefree_net(net);
}

SRPATH *ro_mazealloc_path(void)
{
	SRPATH *path;

	if (ro_mazepath_free_list != NOSRPATH)
	{
		path = ro_mazepath_free_list;
		ro_mazepath_free_list = path->next;
	} else
	{
		path = (SRPATH *)emalloc(sizeof (SRPATH), ro_tool->cluster);
		if (path == 0) return(NOSRPATH);
	}
	return path;
}

void ro_mazefree_path(SRPATH *path)
{
	path->next = ro_mazepath_free_list;
	ro_mazepath_free_list = path;
}

SRPORT *ro_mazealloc_port(void)
{
	SRPORT *port;

	if (ro_mazeport_free_list != NOSRPORT)
	{
		port = ro_mazeport_free_list;
		ro_mazeport_free_list = port->next;
	} else
	{
		port = (SRPORT *)emalloc(sizeof (SRPORT), ro_tool->cluster);
		if (port == 0) return(NOSRPORT);
	}
	return port;
}

void ro_mazefree_port(SRPORT *port)
{
	port->next = ro_mazeport_free_list;
	ro_mazeport_free_list = port;
}

void ro_mazedelete_port(SRPORT *port)
{
	SRWAVEPT *wavept;
	SRPATH *path;

	while (port->wavefront != NOSRWAVEPT)
	{
		wavept = port->wavefront;
		port->wavefront = port->wavefront->next;
		ro_mazefree_wavept(wavept);
	}

	while (port->paths != NOSRPATH)
	{
		path = port->paths;
		port->paths = port->paths->next;
		ro_mazefree_path(path);
	}
	ro_mazefree_port(port);
}

SRWAVEPT *ro_mazealloc_wavept(void)
{
	SRWAVEPT *wavept;

	if (ro_mazewavept_free_list != NOSRWAVEPT)
	{
		wavept = ro_mazewavept_free_list;
		ro_mazewavept_free_list = wavept->next;
	} else
	{
		wavept = (SRWAVEPT *)emalloc(sizeof (SRWAVEPT), ro_tool->cluster);
		if (wavept == 0) return(NOSRWAVEPT);
	}
	return wavept;
}

void ro_mazefree_wavept(SRWAVEPT *wavept)
{
	wavept->next = ro_mazewavept_free_list;
	ro_mazewavept_free_list = wavept;
}

/* general control commands */
SRREGION *ro_mazeget_region(INTBIG wlx, INTBIG wly, INTBIG whx, INTBIG why,
	INTBIG gridx, INTBIG gridy, INTBIG offsetx, INTBIG offsety)
{
	INTBIG index;
	SRLAYER *vlayer, *hlayer;
	Q_UNUSED( gridx );
	Q_UNUSED( gridy );
	Q_UNUSED( offsetx );
	Q_UNUSED( offsety );

	if (wlx > whx || wly > why) return NOSRREGION;

	if (ro_theregion == NOSRREGION)
	{
		ro_theregion = (SRREGION *)emalloc(sizeof (SRREGION), ro_tool->cluster);
		if (ro_theregion == NOSRREGION) return NOSRREGION;
		for (index = 0; index < SRMAXLAYERS; index++)
			ro_theregion->layers[index] = NOSRLAYER;
		ro_theregion->nets = NOSRNET;
	} else
	{
		ro_mazecleanout_region(ro_theregion);
	}

	/* now set bounds */
	ro_theregion->lx = wlx;
	ro_theregion->hx = whx;
	ro_theregion->ly = wly;
	ro_theregion->hy = why;

	hlayer = ro_mazeadd_layer(ro_theregion, SCH_HORILAYER, SRHORIPREF,
		ro_mazegridx, ro_mazegridy, ro_mazeoffsetx, ro_mazeoffsety);
	if (hlayer == NOSRLAYER)
	{
		ttyputerr(_("Could not allocate horizontal layer"));
		return NOSRREGION;
	}
	vlayer = ro_mazeadd_layer(ro_theregion, SCH_VERTLAYER, SRVERTPREF,
		ro_mazegridx, ro_mazegridy, ro_mazeoffsetx, ro_mazeoffsety);
	if (vlayer == NOSRLAYER)
	{
		ttyputerr(_("Could not allocate vertical layer"));
		return NOSRREGION;
	}
	if (stopping(STOPREASONROUTING)) return(NOSRREGION);

	return ro_theregion;
}

void ro_mazecleanout_region(SRREGION *region)
{
	SRNET *net;

	/* free the lists */
	while (region->nets != NOSRNET)
	{
		net = region->nets;
		region->nets = region->nets->next;
		ro_mazedelete_net(net);
	}
}

SRLAYER *ro_mazeadd_layer(SRREGION *region, INTBIG index, SRDIRECTION direction,
	INTBIG gridx, INTBIG gridy, INTBIG alignx, INTBIG aligny)
{
	SRLAYER *layer, *alayer;
	INTBIG x, y, tx, ty, lx, ly, hx, hy;

	/* check for common index */
	if (region->layers[index] == NOSRLAYER)
	{
		/* allocate and initialize the layer */
		layer = (SRLAYER *)emalloc(sizeof (SRLAYER), ro_tool->cluster);
		if (layer == NOSRLAYER) return NOSRLAYER;
		region->layers[index] = layer;
		layer->grids = 0;
		layer->array = 0;
		layer->vused = 0;
		layer->hused = 0;
		layer->allocwid = 0;
		layer->allochei = 0;
		layer->allocarray = 0;
	} else layer = region->layers[index];

	layer->index = index;
	layer->mask = 1<<index;
	layer->dir = direction;

	/* determine the range in grid units */
	if ((tx = alignx % gridx) < 0) tx += gridx;
	if ((ty = aligny % gridy) < 0) ty += gridy;

	/* determine the actual bounds of the world */
	/* round low bounds up, high bounds down */
	lx = ((region->lx - tx + gridx - 1) / gridx) * gridx + tx;
	ly = ((region->ly - ty + gridy - 1) / gridy) * gridy + ty;
	hx = ((region->hx - tx) / gridx) * gridx + tx;
	hy = ((region->hy - ty) / gridy) * gridy + ty;

	/* translate the region lx, ly into grid units */
	layer->wid = (hx - lx) / gridx + 1;
	layer->hei = (hy - ly) / gridy + 1;
	layer->transx = lx /* + tx */;
	layer->transy = ly /* + ty */;
	layer->gridx = gridx;
	layer->gridy = gridy;

	/* sensibility check */
	if (layer->wid > MAXGRIDSIZE || layer->hei > MAXGRIDSIZE)
	{
		ttyputerr(_("This route is too large to solve (limit is %dx%d grid, this is %ldx%ld)"),
			MAXGRIDSIZE, MAXGRIDSIZE, layer->wid, layer->hei);
		return NOSRLAYER;
	}

	/* check that the hx, hy of grid is in bounds of region */
	if (WORLDX(layer->wid - 1, layer) > region->hx) layer->wid--;
	if (WORLDY(layer->hei - 1, layer) > region->hy) layer->hei--;

	/* now allocate a grid array */
	if (layer->wid > layer->allocwid)
	{
		if (layer->grids != 0)
		{
			efree((CHAR *)layer->grids);
			layer->grids = 0;
		}
		layer->grids = (SRGPT **)emalloc(sizeof (SRGPT *) * layer->wid, ro_tool->cluster);
		if (layer->grids == NULL) return NOSRLAYER;
		if (layer->vused != 0)
		{
			efree((CHAR *)layer->vused);
			layer->vused = 0;
		}
		layer->vused = (UCHAR1 *)emalloc(sizeof (UCHAR1) * layer->wid, ro_tool->cluster);
		if (layer->vused == NULL) return NOSRLAYER;
		layer->allocwid = layer->wid;
	}
	if (layer->hei > layer->allochei)
	{
		if (layer->hused != 0)
		{
			efree((CHAR *)layer->hused);
			layer->hused = 0;
		}
		layer->hused = (UCHAR1 *)emalloc(sizeof (UCHAR1) * layer->hei, ro_tool->cluster);
		if (layer->hused == NULL) return NOSRLAYER;
		layer->allochei = layer->hei;
	}
	if (layer->wid * layer->hei > layer->allocarray)
	{
		if (layer->array != 0)
		{
			efree((CHAR *)layer->array);
			layer->array = 0;
		}
		layer->array = (SRGPT *)emalloc(sizeof (SRGPT *) * layer->wid * layer->hei, ro_tool->cluster);
		if (layer->array == NULL) return NOSRLAYER;
		layer->allocarray = layer->wid * layer->hei;
	}

	for (x = 0; x < layer->wid; x++)
	{
		/* get address for a column of grid points */
		layer->grids[x] = &layer->array[x * layer->hei];

		/* clear all points */
		for (y = 0; y < layer->hei; y++)
			layer->grids[x][y] = 0;
	}

	for (x = 0; x < layer->wid; x++) layer->vused[x] = 0;
	for (y = 0; y < layer->hei; y++) layer->hused[y] = 0;

	/* set up/down pointers */
	layer->up = layer->down = NOSRLAYER;
	if (index != 0)
	{
		alayer = region->layers[index-1];
		if (alayer != NOSRLAYER)
		{
			layer->down = alayer;
			alayer->up = layer;
		}
	}
	if (index < SRMAXLAYERS - 1)
	{
		alayer = region->layers[index+1];
		if (alayer != NOSRLAYER)
		{
			layer->up = alayer;
			alayer->down = layer;
		}
	}

	return layer;
}

/* drawing functions */
void ro_mazeset_point(SRLAYER *layer, SRGPT type, INTBIG x, INTBIG y, SRMODE mode)
{
	switch (mode)
	{
		case SR_MOR:
			layer->grids[x][y] |= type;
			layer->vused[x] |= type;
			layer->hused[y] |= type;
			break;
		case SR_MAND:
			layer->grids[x][y] &= type;
			layer->vused[x] &= type;
			layer->hused[y] &= type;
			break;
		case SR_MSET:
			layer->grids[x][y] = type;
			layer->vused[x] = type;
			layer->hused[y] = type;
			break;
	}
}

void ro_mazeset_line(SRREGION *region, SRLAYER *layer, SRGPT type, INTBIG wx1, INTBIG wy1,
	INTBIG wx2, INTBIG wy2, SRMODE mode)
{
	INTBIG x[2], y[2], lx, hx, ly, hy, diff, dx, dy, e, xi, yi;
	Q_UNUSED( region );

	/* convert to grid coordinates */
	x[0] = GRIDX(wx1, layer);   x[1] = GRIDX(wx2, layer);
	y[0] = GRIDY(wy1, layer);   y[1] = GRIDY(wy2, layer);
	if (wx1 < wx2) { lx = 0; hx = 1; }
		else { lx = 1; hx = 0; }
	if (wy1 < wy2) { ly = 0; hy = 1; }
		else { ly = 1; hy = 0; }

	if (x[hx] < 0 || x[lx] >= layer->wid || y[hy] < 0 || y[ly] >= layer->hei) return;

	dx = x[hx] - x[lx];
	dy = y[hy] - y[ly];

	/* clip x */
	if ((diff = -x[lx]) > 0)
	{
		y[lx] += (y[hx] - y[lx]) * diff / dx;
		x[lx] = 0;
	}
	if ((diff = x[hx] - (layer->wid - 1)) > 0)
	{
		y[hx] -= (y[hx] - y[lx]) * diff / dx;
		x[hx] = layer->wid - 1;
	}

	/* now clip y */
	if ((diff = -y[ly]) > 0)
	{
		x[ly] += (x[hy] - x[ly]) * diff / dy;
		y[ly] = 0;
	}
	if ((diff = y[hy] - (layer->hei - 1)) > 0)
	{
		x[hy] -= (x[hy] - x[ly]) * diff / dy;
		y[hy] = layer->hei - 1;
	}

	/* after clip ... */
	dx = x[hx] - x[lx];
	dy = y[hy] - y[ly];

	/* use Bresenham's algorithm to set intersecting grid points */
	if (dy < dx)
	{
		/* for 0 <= dy <= dx */
		e = (dy<<1) - dx;
		yi = y[lx];
		if (y[hx] - y[lx] < 0) diff = -1;
			else diff = 1;
		for (xi = x[lx]; xi <= x[hx]; xi++)
		{
			ro_mazeset_point(layer, type, xi, yi, mode);
			if (e > 0)
			{
				yi += diff;
				e = e + (dy<<1) - (dx<<1);
			} else e = e + (dy<<1);
		}
	} else
	{
		/* for 0 <= dx < dy */
		e = (dx<<1) - dy;
		xi = x[ly];
		if (x[hy] - x[ly] < 0) diff = -1;
			else diff = 1;
		for (yi = y[ly]; yi <= y[hy]; yi++)
		{
			ro_mazeset_point(layer, type, xi, yi, mode);
			if (e > 0)
			{
				xi += diff;
				e = e + (dx<<1) - (dy<<1);
			} else e = e + (dx<<1);
		}
	}
}

void ro_mazeset_box(SRREGION *region, SRLAYER *layer, SRGPT type, INTBIG wx1, INTBIG wy1, INTBIG wx2, INTBIG wy2, SRMODE mode)
{
	INTBIG lx, ly, hx, hy, x, y;
	Q_UNUSED( region );

	lx = GRIDX(wx1, layer);   ly = GRIDY(wy1, layer);
	hx = GRIDX(wx2, layer);   hy = GRIDY(wy2, layer);
	if (lx > hx) { x = lx; lx = hx; hx = x; }
	if (ly > hy) { y = ly; ly = hy; hy = y; }

	if (hx < 0 || lx >= layer->wid || hy < 0 || ly >= layer->hei) return;

	/* clip (simple orthogonal) */
	if (lx < 0) lx = 0;
	if (hx >= layer->wid) hx = layer->wid - 1;
	if (ly < 0) ly = 0;
	if (hy >= layer->hei) hy = layer->hei - 1;

	/* now fill the box */
	for (x = lx; x <= hx; x++)
		for (y = ly; y <= hy; y++)
			ro_mazeset_point(layer, type, x, y, mode);
}

/* routing definition functions */
SRNET *ro_mazeadd_net(SRREGION *region)
{
	SRNET *net;

	net = ro_mazealloc_net();
	if (net == NOSRNET) return NOSRNET;

	net->routed = FALSE;
	net->ports = NOSRPORT;
	net->paths = NOSRPATH;
	net->lastpath = NOSRPATH;
	net->region = region;

	/* link into region list */
	net->next = region->nets;
	region->nets = net;

	return net;
}

SRPATH *ro_mazeget_path(SRPORT *port, SRLAYER *layer,
	SREND e1, SREND e2, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2)
{
	SRPATH *path;

	path = ro_mazealloc_path();
	if (path == NOSRPATH) return NOSRPATH;

	path->x[0] = x1; path->y[0] = y1;
	path->x[1] = x2; path->y[1] = y2;
	path->end[0] = e1; path->end[1] = e2;
	path->layer = layer;
	path->port = port;
	path->type = SRPROUTED;

	path->wx[0] = WORLDX(x1, layer); path->wy[0] = WORLDY(y1, layer);
	path->wx[1] = WORLDX(x2, layer); path->wy[1] = WORLDY(y2, layer);

	/* insert the path at the end of the list */
	path->next = NOSRPATH;
	if (port->lastpath == NOSRPATH) port->paths = path; else
		port->lastpath->next = path;
	port->lastpath = path;

	/* now draw it */
	ro_mazeset_line(port->net->region, path->layer, SR_GPORT | SR_GSET,
		path->wx[0], path->wy[0], path->wx[1], path->wy[1], SR_MOR);

	return path;
}

SRPORT *ro_mazeadd_port(SRNET *net, UINTBIG layers, INTBIG wx1, INTBIG wy1, INTBIG wx2, INTBIG wy2,
	NODEINST *ni, PORTPROTO *pp)
{
	SRPORT *port, *lport;
	INTBIG index;
	INTBIG mask;

	port = ro_mazealloc_port();
	if (port == NOSRPORT) return NOSRPORT;

	if (wx1 < wx2) { port->lx = wx1; port->hx = wx2; }
		else { port->lx = wx2; port->hx = wx1; }
	if (wy1 < wy2) { port->ly = wy1; port->hy = wy2; }
		else { port->ly = wy2; port->hy = wy1; }

	port->layers = layers;
	port->wavefront = NOSRWAVEPT;

	for (index = 0, mask = 1; index < SRMAXLAYERS; index++, mask = mask<<1)
	{
		if (layers & mask && net->region->layers[index] != NOSRLAYER)
		{
			ro_mazeset_box(net->region, net->region->layers[index], SR_GPORT | SR_GSET,
				port->lx, port->ly, port->hx, port->hy, SR_MOR);
		}
	}

	/* link into net */
	port->next = NOSRPORT;
	port->master = NOSRPORT;
	port->paths = NOSRPATH;
	port->lastpath = NOSRPATH;
	port->net = net;
	port->ni = ni;
	port->pp = pp;
	lport = net->ports;
	if (lport == NOSRPORT)
	{
		index = 0;
		net->ports = port;
	} else
	{
		index = 1;
		while (lport->next != NOSRPORT) { index++; lport = lport->next; }
		lport->next = port;
	}
	port->index = index;

	return port;
}

/* routing commands */
BOOLEAN ro_mazeadd_wavept(SRPORT *port, SRLAYER *layer, INTBIG x, INTBIG y, INTBIG code)
{
	SRWAVEPT *wavept;

	wavept = ro_mazealloc_wavept();
	if (wavept == NOSRWAVEPT) return TRUE;
	wavept->x = x;
	wavept->y = y;

	/* set the grid */
	layer->grids[x][y] = (SRGPT)((layer->grids[x][y] & ~SR_GMASK) | code | SR_GWAVE);
	wavept->layer = layer;

	/* set maze bounds */
	if (layer->lx > x) layer->lx = x;
	if (layer->hx < x) layer->hx = x;
	if (layer->ly > y) layer->ly = y;
	if (layer->hy < y) layer->hy = y;

	wavept->prev = NOSRWAVEPT;
	if (port->master)
	{
		wavept->port = port->master;
		wavept->next = port->master->wavefront;
		port->master->wavefront = wavept;
	} else
	{
		wavept->port = port;
		wavept->next = port->wavefront;
		port->wavefront = wavept;
	}
	if (wavept->next != NOSRWAVEPT)
		wavept->next->prev = wavept;
	return FALSE;
}

void ro_mazecreate_wavefront(SRPORT *port)
{
	INTBIG x, y, dx, dy, lx, ly, hx, hy, cx, cy, ang, angrange, spread;
	INTBIG index;
	BOOLEAN onedge;
	UINTBIG mask;
	SRLAYER *layer;
	SRPATH *path;
	SRPORT *master;

	master = port->master;
	if (master == NOSRPORT) master = port;

	/* first assign each layer of the port as wavefront points */
	for (index = 0, mask = 1; index < SRMAXLAYERS; index++, mask = mask<<1)
	{
		if (mask & port->layers)
		{
			layer = port->net->region->layers[index];
			if (layer == NOSRLAYER) continue;

			/* convert to grid points */
			lx = GRIDX(port->lx, layer);   ly = GRIDY(port->ly, layer);
			hx = GRIDX(port->hx, layer);   hy = GRIDY(port->hy, layer);
			if (lx >= layer->wid || hx < 0 || ly >= layer->hei || hy < 0) continue;

			/* clip to window */
			if (lx < 0) lx = 0;
			if (hx >= layer->wid) hx = layer->wid - 1;
			if (ly < 0) ly = 0;
			if (hy >= layer->hei) hy = layer->hei - 1;

			/*
			 * added detection of immediate blockage ... smr
			 */
			if (ro_mazedebug)
				xprintf(ro_mazedebuggingout, x_("Port %ld<=X<=%ld %ld<=Y<=%ld\n"), lx, hx, ly, hy);
			onedge = FALSE;
			for (x = lx; x <= hx; x++)
			{
				for (y = ly; y <= hy; y++)
				{
					if (ro_mazeadd_wavept(master, layer, x, y, SR_GSTART))
						return;
					if (x < layer->wid-1 && layer->grids[x+1][y] == 0) onedge = TRUE;
					if (x > 0 && layer->grids[x-1][y] == 0) onedge = TRUE;
					if (y < layer->hei-1 && layer->grids[x][y+1] == 0) onedge = TRUE;
					if (y > 0 && layer->grids[x][y-1] == 0) onedge = TRUE;
				}
			}
			if (!onedge)
			{
				/* port is inside of blocked area: search for opening */
				cx = (lx + hx) / 2;
				cy = (ly + hy) / 2;
				angrange = (port->pp->userbits&PORTARANGE) >> PORTARANGESH;
				ang = (port->pp->userbits&PORTANGLE) >> PORTANGLESH;
				ang += (port->ni->rotation+5) / 10;
				if (port->ni->transpose != 0) { ang = 270 - ang; if (ang < 0) ang += 360; }
				if (ro_anglediff(ang, 0) <= angrange)
				{
					/* port faces right */
					for(spread=1; spread<BLOCKAGELIMIT; spread++)
					{
						if (hx+spread >= layer->wid) break;
						if (layer->grids[hx+spread][cy] == 0) { onedge = TRUE;   break; }
						layer->grids[hx+spread][cy] = 0;
					}
				}
				if (ro_anglediff(ang, 90) <= angrange)
				{
					/* port faces up */
					for(spread=1; spread<BLOCKAGELIMIT; spread++)
					{
						if (hy+spread >= layer->hei) break;
						if (layer->grids[cx][hy+spread] == 0) { onedge = TRUE;   break; }
						layer->grids[cx][hy+spread] = 0;
					}
				}
				if (ro_anglediff(ang, 180) <= angrange)
				{
					/* port faces left */
					for(spread=1; spread<BLOCKAGELIMIT; spread++)
					{
						if (lx-spread < 0) break;
						if (layer->grids[lx-spread][cy] == 0) { onedge = TRUE;   break; }
						layer->grids[lx-spread][cy] = 0;
					}
				}
				if (ro_anglediff(ang, 270) <= angrange)
				{
					/* port faces down */
					for(spread=1; spread<BLOCKAGELIMIT; spread++)
					{
						if (ly-spread < 0) break;
						if (layer->grids[cx][ly-spread] == 0) { onedge = TRUE;   break; }
						layer->grids[cx][ly-spread] = 0;
					}
				}
				if (!onedge)
				{
					ttyputerr(_("Node %s is blocked"), describenodeinst(port->ni));
					return;
				}
			}
		}
	}

	/* now assign the paths of the port */
	for (path = port->paths; path != NOSRPATH; path = path->next)
	{
		/* note paths are always in the working area */
		if (path->x[0] == path->x[1])
		{
			/* vertical path */
			if (path->y[0] < path->y[1]) dy = 1;
				else dy = -1;
			for (x = path->x[0], y = path->y[0];
				(dy < 0) ? y >= path->y[1] : y <= path->y[1]; y += dy)
			{
				if (ro_mazeadd_wavept(master, path->layer, x, y, SR_GSTART)) return;
			}
		} else if (path->y[0] == path->y[1])
		{
			/* horizontal path */
			if (path->x[0] < path->x[1]) dx = 1;
				else dx = -1;
			for (y = path->y[0], x = path->x[0];
				(dx < 0) ? x >= path->x[1] : x <= path->x[1]; x += dx)
			{
				if (ro_mazeadd_wavept(master, path->layer, x, y, SR_GSTART)) return;
			}
		} else
		{
			/* a 45 degree path, note assume x,y difference is equal */
			if (path->x[0] < path->x[1]) dx = 1;
				else dx = -1;
			if (path->y[0] < path->y[1]) dy = 1;
				else dy = -1;

			for (x = path->x[0], y = path->y[0];
				(dx < 0) ? x >= path->x[1] : x <= path->x[1]; x += dx, y += dy)
			{
				if (ro_mazeadd_wavept(master, path->layer, x, y, SR_GSTART)) return;
			}
		}
	}
}

SRWAVEPT *ro_mazesearch_wavefront(SRPORT *port, SRLAYER *layer, INTBIG x, INTBIG y)
{
	SRWAVEPT *wavept;

	/* scans port's wavefront for common point */
	for (wavept = port->wavefront; wavept != NOSRWAVEPT; wavept = wavept->next)
	{
		if (wavept->layer == layer && wavept->x == x && wavept->y == y) return wavept;
	}
	return NOSRWAVEPT;
}

INTBIG ro_mazetest_pt(SRGPT pt, INTBIG code)
{
	/* don't check other wavefront points */
	if (!(pt & SR_GWAVE))
	{
		/* check for permanent grid object (blockage or port) */
		if (pt & SR_GSET)
		{
			/* check for port */
			if ((pt & SR_GPORT) != 0 && (pt & SR_GMASK) == code) return SRROUTED;
		} else
		{
			/* not permanent, check for matching code */
			if ((pt & SR_GMASK) == code) return SRSUCCESS;
		}
	}
	return SRUNROUTED;
}

/* module: ro_mazefind_paths
 * function: will find the path through the maze to the target point.
 *	Will merge the starting point path with the first internal path
 *	if possible.
 */
INTBIG ro_mazefind_paths(SRWAVEPT *wavept, SRPATH *path)
{
	INTBIG sx, sy, ex, ey, code, dx, dy, nx, ny;
	INTBIG status, pstart = 0;
	SRLAYER *layer;

	/* Start scan from the first point */
	sx = wavept->x; sy = wavept->y;

	layer = wavept->layer;
	code = layer->grids[sx][sy] & SR_GMASK;
	if (code == SR_GSTART) code = SR_GMAX;
		else code--;
	for(;;)
	{
		dx = dy = 0;

		/* scan around the point */
		for(;;)
		{
			if (pstart == 1)
			{
				/* always try to jump layer after the first path */
				/* now try jumping layers */
				if (layer->up != NOSRLAYER)
				{
					ex = sx; ey = sy;
					status = ro_mazetest_pt(layer->up->grids[ex][ey], code);
					if (status == SRROUTED) return SRSUCCESS;
					if (status == SRSUCCESS)
					{
						layer = layer->up;
						if (code == SR_GSTART) code = SR_GMAX;
							else code--;
						pstart = 2;
						continue;
					}
				}
				if (layer->down != NOSRLAYER)
				{
					ex = sx; ey = sy;
					status = ro_mazetest_pt(layer->down->grids[ex][ey], code);
					if (status == SRROUTED) return SRSUCCESS;
					if (status == SRSUCCESS)
					{
						layer = layer->down;
						if (code == SR_GSTART) code = SR_GMAX;
							else code--;
						pstart = 2;
						continue;
					}
				}
			}
			if (layer->dir == SRALL || layer->dir == SRHORIPREF)
			{
				/* try right first */
				if ((ex = sx + 1) != layer->wid)
				{
					ey = sy;
					status = ro_mazetest_pt(layer->grids[ex][ey], code);
					if (status == SRROUTED)
					{
						/* check for common original path */
						if (path != NOSRPATH && path->layer == layer &&
							path->y[0] == path->y[1] && path->y[0] == ey &&
								maxi(path->x[0], path->x[1]) == sx)
						{
							if (path->x[0] == sx)
							{
								path->x[0] = ex;
								path->wx[0] = WORLDX(ex, layer);
							} else
							{
								path->x[1] = ex;
								path->wx[1] = WORLDX(ex, layer);
							}
							ro_mazeset_line(path->port->net->region, path->layer, SR_GPORT | SR_GSET,
								path->wx[0], path->wy[0], path->wx[1], path->wy[1], SR_MOR);
							return SRSUCCESS;
						}
						if (ro_mazeget_path(wavept->port, layer, SREEND, SREPORT, sx, sy, ex, ey) == NOSRPATH)
							return SRERROR;
						return SRSUCCESS;
					} else if (status == SRSUCCESS)
					{
						dx = 1;
						break;
					}
				}
			}
			if (layer->dir == SRALL || layer->dir == SRVERTPREF)
			{
				/* try down first */
				if ((ey = sy - 1) >= 0)
				{
					ex = sx;
					status = ro_mazetest_pt(layer->grids[ex][ey], code);
					if (status == SRROUTED)
					{
						/* check for common original path */
						if (path != NOSRPATH && path->layer == layer &&
							path->x[0] == path->x[1] && path->x[0] == ex &&
								mini(path->y[0], path->y[1]) == sy)
						{
							if (path->y[0] == sy)
							{
								path->y[0] = ey;
								path->wy[0] = WORLDY(ey, layer);
							} else
							{
								path->y[1] = ey;
								path->wy[1] = WORLDY(ey, layer);
							}
							ro_mazeset_line(path->port->net->region, path->layer, SR_GPORT | SR_GSET,
								path->wx[0], path->wy[0], path->wx[1], path->wy[1], SR_MOR);
							return SRSUCCESS;
						}
						if (ro_mazeget_path(wavept->port, layer, SREEND, SREPORT, sx, sy, ex, ey) == NOSRPATH)
							return SRERROR;
						return SRSUCCESS;
					} else if (status == SRSUCCESS)
					{
						dy = -1;
						break;
					}
				}
			}
			if (layer->dir == SRALL || layer->dir == SRHORIPREF)
			{
				/* try left */
				if ((ex = sx - 1) >= 0)
				{
					ey = sy;
					status = ro_mazetest_pt(layer->grids[ex][ey], code);
					if (status == SRROUTED)
					{
						/* check for common original path */
						if (path != NOSRPATH && path->layer == layer &&
							path->y[0] == path->y[1] && path->y[0] == ey &&
								mini(path->x[0], path->x[1]) == sx)
						{
							if (path->x[0] == sx)
							{
								path->x[0] = ex;
								path->wx[0] = WORLDX(ex, layer);
							} else
							{
								path->x[1] = ex;
								path->wx[1] = WORLDX(ex, layer);
							}
							ro_mazeset_line(path->port->net->region, path->layer, SR_GPORT | SR_GSET,
								path->wx[0], path->wy[0], path->wx[1], path->wy[1], SR_MOR);
							return SRSUCCESS;
						}
						if (ro_mazeget_path(wavept->port, layer, SREEND, SREPORT, sx, sy, ex, ey) == NOSRPATH)
							return SRERROR;
						return SRSUCCESS;
					} else if (status == SRSUCCESS)
					{
						dx = -1;
						break;
					}
				}
			}
			if (layer->dir == SRALL || layer->dir == SRVERTPREF)
			{
				/* try up */
				if ((ey = sy + 1) != layer->hei)
				{
					ex = sx;
					status = ro_mazetest_pt(layer->grids[ex][ey], code);
					if (status == SRROUTED)
					{
						/* check for common original path */
						if (path != NOSRPATH && path->layer == layer &&
							path->x[0] == path->x[1] && path->x[0] == ex &&
								maxi(path->y[0], path->y[1]) == sy)
						{
							if (path->y[0] == sy)
							{
								path->y[0] = ey;
								path->wy[0] = WORLDY(ey, layer);
							} else
							{
								path->y[1] = ey;
								path->wy[1] = WORLDY(ey, layer);
							}
							ro_mazeset_line(path->port->net->region, path->layer, SR_GPORT | SR_GSET,
								path->wx[0], path->wy[0], path->wx[1], path->wy[1], SR_MOR);
							return SRSUCCESS;
						}
						if (ro_mazeget_path(wavept->port, layer, SREEND, SREPORT, sx, sy, ex, ey) == NOSRPATH)
							return SRERROR;
						return SRSUCCESS;
					} else if (status == SRSUCCESS)
					{
						dy = 1;
						break;
					}
				}
			}

			/* now try jumping layers */
			if (pstart == 0)
			{
				if (layer->up != NOSRLAYER)
				{
					ex = sx; ey = sy;
					status = ro_mazetest_pt(layer->up->grids[ex][ey], code);
					if (status == SRROUTED) return SRSUCCESS;
					if (status == SRSUCCESS)
					{
						layer = layer->up;
						if (code == SR_GSTART) code = SR_GMAX;
							else code--;
						continue;
					}
				}
				if (layer->down != NOSRLAYER)
				{
					ex = sx; ey = sy;
					status = ro_mazetest_pt(layer->down->grids[ex][ey], code);
					if (status == SRROUTED) return SRSUCCESS;
					if (status == SRSUCCESS)
					{
						layer = layer->down;
						if (code == SR_GSTART) code = SR_GMAX;
							else code--;
						continue;
					}
				}
			}

			/* could not start route, just return */
			return SRERROR;
		}

		/* set path started */
		pstart = 1;

		/* now continue scan until the end of the path */
		for(;;)
		{
			if (code == SR_GSTART) code = SR_GMAX;
				else code--;
			if (dx)
			{
				/* horizontal scan */
				nx = ex + dx;
				ny = ey;
				status = ro_mazetest_pt(layer->grids[nx][ny], code);
				if (status == SRROUTED)
				{
					/* check for common original path */
					if (path != NOSRPATH && path->layer == layer &&
						path->y[0] == path->y[1] && path->y[0] == sy &&
						((dx < 0 && mini(path->x[0], path->x[1]) == sx) ||
						(dx > 0 && maxi(path->x[0], path->x[1]) == sx)))
					{
						if (path->x[0] == sx)
						{
							path->x[0] = nx;
							path->wx[0] = WORLDX(nx, layer);
						} else
						{
							path->x[1] = nx;
							path->wx[1] = WORLDX(nx, layer);
						}
						ro_mazeset_line(path->port->net->region, path->layer, SR_GPORT | SR_GSET,
							path->wx[0], path->wy[0], path->wx[1], path->wy[1], SR_MOR);
						return SRSUCCESS;
					}
					if (ro_mazeget_path(wavept->port, layer, SREEND, SREPORT, sx, sy, nx, ny) == NOSRPATH)
						return SRERROR;
					return SRSUCCESS;
				} else if (status == SRSUCCESS)
				{
					ex = nx;
					continue;
				}
			}
			if (dy)
			{
				/* veritical scan */
				nx = ex;
				ny = ey + dy;
				status = ro_mazetest_pt(layer->grids[nx][ny], code);
				if (status == SRROUTED)
				{
					/* check for common original path */
					if (path != NOSRPATH && path->layer == layer &&
						path->x[0] == path->x[1] && path->x[0] == sx &&
						((dy < 0 && mini(path->y[0], path->y[1]) == sy) ||
						(dy > 0 && maxi(path->y[0], path->y[1]) == sy)))
					{
						if (path->y[0] == sy)
						{
							path->y[0] = ny;
							path->wy[0] = WORLDY(ny, layer);
						} else
						{
							path->y[1] = ny;
							path->wy[1] = WORLDY(ny, layer);
						}
						ro_mazeset_line(path->port->net->region, path->layer, SR_GPORT | SR_GSET,
							path->wx[0], path->wy[0], path->wx[1], path->wy[1], SR_MOR);
						return SRSUCCESS;
					}
					if (ro_mazeget_path(wavept->port, layer, SREEND, SREPORT, sx, sy, nx, ny) == NOSRPATH)
						return SRERROR;
					return SRSUCCESS;
				} else if (status == SRSUCCESS)
				{
					ey = ny;
					continue;
				}
			}

			/* end of the path, add and break loop */

			/* check for common original path */
			if (path != NOSRPATH && path->layer == layer &&
				/* horizontal path check */
				((sy == ey && path->y[0] == path->y[1] && path->y[0] == ey &&
				(path->x[0] == sx || path->x[1] == sx)) ||
				/* vertical path check */
				(sx == ex && path->x[0] == path->x[1] && path->x[0] == ex &&
				(path->y[0] == sy || path->y[1] == sy))))
			{
				/* vertical path ? */
				if (sx == ex)
				{
					if (sy == path->y[0])
					{
						path->y[0] = ey;
						path->wy[0] = WORLDY(ey, layer);
					} else
					{
						path->y[1] = ey;
						path->wy[1] = WORLDY(ey, layer);
					}
				}

				/* horizontal path ? */
				else
				{
					if (sx == path->x[0])
					{
						path->x[0] = ex;
						path->wx[0] = WORLDX(ex, layer);
					} else
					{
						path->x[1] = ex;
						path->wx[1] = WORLDX(ex, layer);
					}
				}

				ro_mazeset_line(path->port->net->region, path->layer, SR_GPORT | SR_GSET,
					path->wx[0], path->wy[0], path->wx[1], path->wy[1], SR_MOR);
				if (ro_mazedebug)
				{
					CHAR line[256];
					esnprintf(line, 256, x_("path (%ld,%ld) to (%ld,%ld)"), sx, sy, ex, ey);
					ro_mazedump_layer(line, path->port->net->region, (UINTBIG)3);
				}
				path = NOSRPATH;
			} else if (ro_mazeget_path(wavept->port, layer, SREEND, SREEND, sx, sy, ex, ey) == NOSRPATH)
				return SRERROR;
			sx = ex;
			sy = ey;
			break;
		}
	}
}

INTBIG ro_mazeinit_path(SRPORT *port, SRLAYER *layer, SRWAVEPT *wavept, INTBIG x, INTBIG y, INTBIG code)
{
	SRPORT *target, *sport;
	SRWAVEPT *twavept;
	SRPATH *path;
	Q_UNUSED( code );

	/* search for others */
	for (target = port->net->ports; target != NOSRPORT; target = target->next)
	{
		if (target == port || target->master != NOSRPORT) continue;
		twavept = ro_mazesearch_wavefront(target, layer, x, y);
		if (twavept != NOSRWAVEPT) break;
	}
	if (target == NOSRPORT) return SRERROR;

	/* now move the target's paths to the master. This is done to retain the
	* original path creation order (port out), and also insures the existance
	* of the arc in t-junction connections */
	if (port->lastpath != NOSRPATH)
	{
		if ((port->lastpath->next = target->paths) != NOSRPATH)
			port->lastpath = target->lastpath;
	} else
	{
		/* this should never happen */
		port->paths = target->paths;
		port->lastpath = target->lastpath;
	}
	target->paths = NOSRPATH;
	target->lastpath = NOSRPATH;

	/* connect the port with target */
	if (wavept->layer == twavept->layer)
	{
		path = ro_mazeget_path(port, layer, SREEND, SREEND, wavept->x, wavept->y,
			twavept->x, twavept->y);
		if (path == NOSRPATH) return SRERROR;
	} else path = NOSRPATH;

	/* now create paths to each target point */
	if (ro_mazefind_paths(wavept, path) != SRSUCCESS)
		return SRERROR;
	if (ro_mazefind_paths(twavept, path) != SRSUCCESS)
		return SRERROR;

	/* now set the target master */
	target->master = port;

	/* now scan through all ports and change target as master to port */
	for (sport = port->net->ports; sport != NOSRPORT; sport = sport->next)
	{
		if (sport->master == target) sport->master = port;
	}

	/* now move the rest of the paths to the master port */
	if (port->lastpath != NOSRPATH)
	{
		if ((port->lastpath->next = target->paths) != NOSRPATH)
			port->lastpath = target->lastpath;
	} else
	{
		/* this should never happen */
		port->paths = target->paths;
		port->lastpath = target->lastpath;
	}
	target->paths = NOSRPATH;
	target->lastpath = NOSRPATH;

	return SRROUTED;
}

INTBIG ro_mazeexamine_pt(SRPORT *port, SRLAYER *layer, INTBIG x, INTBIG y, INTBIG code)
{
	/* point is set */
	if (layer->grids[x][y] & SR_GWAVE)
	{
		/* look for common point in this wavefront */
		if (ro_mazesearch_wavefront(port, layer, x, y) == NOSRWAVEPT)
		{
			return SRROUTED;
		}
	} else if (layer->grids[x][y] == 0)
	{
		/* point is not set */
		if (ro_mazeadd_wavept(port, layer, x, y, code)) return SRERROR;
		return SRSUCCESS;
	}
	return SRBLOCKED;
}

INTBIG ro_mazeexpand_wavefront(SRPORT *port, INTBIG code)
{
	SRWAVEPT *wavept, *next;
	SRWAVEPT bwavept;
	SRLAYER *layer, *blayer;
	INTBIG x, y, bx, by, status = SRSUCCESS;
	BOOLEAN connected, found = FALSE;

	/* begin expansion of all wavepts */
	/* disconnect wavepts from the port */
	wavept = port->wavefront;
	if (wavept == NOSRWAVEPT) return SRBLOCKED;

	for (wavept = port->wavefront; wavept != NOSRWAVEPT ; wavept = next)
	{
		connected = FALSE;
		layer = wavept->layer;
		if (layer->dir == SRALL || layer->dir == SRHORIPREF)
		{
			/* try horizontal route */
			x = wavept->x + 1;
			if (x != layer->wid)
			{
				status = ro_mazeexamine_pt(port, layer, x, wavept->y, code);
				if (status == SRROUTED)
				{
					/* LINTED "bwavept" used in proper order */
					if (!found || (layer->hused[bwavept.y] & SR_GSET)  ||
						(!(layer->hused[wavept->y] & SR_GSET) &&
						layer->hused[wavept->y] < layer->hused[bwavept.y]))
					{
						bwavept = *wavept;
						bx = x; by = wavept->y;
						blayer = layer;
					}
					found = TRUE;
					connected = TRUE;
				} else if (status == SRERROR) return SRERROR;
			}
			if (!connected && (x = wavept->x - 1) >= 0)
			{
				status = ro_mazeexamine_pt(port, layer, x, wavept->y, code);
				if (status == SRROUTED)
				{
					if (!found ||
						(layer->hused[bwavept.y] & SR_GSET)  ||
						(!(layer->hused[wavept->y] & SR_GSET) &&
						layer->hused[wavept->y] < layer->hused[bwavept.y]))
					{
						bwavept = *wavept;
						bx = x; by = wavept->y;
						blayer = layer;
					}
					found = TRUE;
					connected = TRUE;
				} else if (status == SRERROR) return SRERROR;
			}
		}
		if (layer->dir == SRALL || layer->dir == SRVERTPREF)
		{
			/* try vertical route */
			if (!connected && (y = wavept->y + 1) != layer->hei)
			{
				status = ro_mazeexamine_pt(port, layer, wavept->x, y, code);
				if (status == SRROUTED)
				{
					if (!found ||
						(layer->vused[bwavept.x] & SR_GSET)  ||
						(!(layer->vused[wavept->x] & SR_GSET) &&
						layer->vused[wavept->x] < layer->vused[bwavept.x]))
					{
						bwavept = *wavept;
						bx = wavept->x; by = y;
						blayer = layer;
					}
					found = TRUE;
					connected = TRUE;
				} else if (status == SRERROR) return SRERROR;
			}
			if (!connected && (y = wavept->y - 1) >= 0)
			{
				status = ro_mazeexamine_pt(port, layer, wavept->x, y, code);
				if (status == SRROUTED)
				{
					if (!found ||
						(layer->vused[bwavept.x] & SR_GSET)  ||
						(!(layer->vused[wavept->x] & SR_GSET) &&
						layer->vused[wavept->x] < layer->vused[bwavept.x]))
					{
						bwavept = *wavept;
						bx = wavept->x; by = y;
						blayer = layer;
					}
					found = TRUE;
					connected = TRUE;
				} else if (status == SRERROR) return SRERROR;
			}
		}
		if (!connected && layer->up != NOSRLAYER)
		{
			/* try via up */
			status = ro_mazeexamine_pt(port, layer->up, wavept->x, wavept->y, code);
			if (status == SRROUTED)
			{
				if (!found)
				{
					bwavept = *wavept;
					bx = wavept->x; by = wavept->y;
					blayer = layer->up;
				}
				found = TRUE;
				connected = TRUE;
			} else if (status == SRERROR) return SRERROR;
		}
		if (!connected && layer->down != NOSRLAYER)
		{
			/* try via down */
			status = ro_mazeexamine_pt(port, layer->down, wavept->x, wavept->y, code);
			if (status == SRROUTED)
			{
				if (!found)
				{
					bwavept = *wavept;
					bx = wavept->x; by = wavept->y;
					blayer = layer->down;
				}
				found = TRUE;
				connected = TRUE;
			} else if (status == SRERROR) return SRERROR;
		}
		next = wavept->next;

		/* now release this wavept */
		if (wavept->prev == NOSRWAVEPT)
		{
			port->wavefront = wavept->next;
			if (wavept->next != NOSRWAVEPT)
			wavept->next->prev = NOSRWAVEPT;
		} else
		{
			wavept->prev->next = wavept->next;
			if (wavept->next != NOSRWAVEPT)
			wavept->next->prev = wavept->prev;
		}

		/* set the grid point to a core point */
		if (!connected) layer->grids[wavept->x][wavept->y] &= ~SR_GWAVE;
		ro_mazefree_wavept(wavept);
	}

	if (found)
		return ro_mazeinit_path(port, blayer, &bwavept, bx, by, code);

	if (port->wavefront == NOSRWAVEPT) return SRBLOCKED;
	return SRSUCCESS;
}

void ro_mazeclear_maze(SRNET *net)
{
	SRLAYER *layer;
	SRPORT *port;
	SRWAVEPT *wavept;
	INTBIG x, y;
	INTBIG index;

	/* clear each region, and reset bounds */
	for (index = 0; index < SRMAXLAYERS; index++)
	{
		layer = net->region->layers[index];
		if (layer != NOSRLAYER)
		{
			for (x = layer->lx; x <= layer->hx; x++)
				for (y = layer->ly; y <= layer->hy; y++)
					layer->grids[x][y] = layer->grids[x][y] & ~(SR_GMASK | SR_GWAVE);
			layer->lx = layer->wid; layer->ly = layer->hei;
			layer->hx = -1; layer->hy = -1;
		}
	}
	for (port = net->ports; port != NOSRPORT; port = port->next)
	{
		while ((wavept = port->wavefront) != NOSRWAVEPT)
		{
			port->wavefront = wavept->next;
			ro_mazefree_wavept(wavept);
		}
	}
	return;
}

BOOLEAN ro_mazeroute_net(SRNET *net)
{
	INTBIG index, status;
	INTBIG pcount, blocked, code, i, prio, x, y, count;
	BOOLEAN ret = FALSE;
	SRLAYER *layer;
	SRPORT *port;
	ARCPROTO *ap, *routingarc;
	TECHNOLOGY *tech;
	PORTPROTO *pp;

	/*
	 * figure out what arcs to use for this route
	 * ... smr
	 */

	/* presume routing with the current arc */
	routingarc = us_curarcproto;
	if (routingarc == gen_unroutedarc) routingarc = NOARCPROTO;
	if (routingarc != NOARCPROTO)
	{
		/* see if the default arc can be used to route */
		for(port = net->ports; port != NOSRPORT; port = port->next)
		{
			pp = port->pp;
			for(i = 0; pp->connects[i] != NOARCPROTO; i++)
				if (pp->connects[i] == routingarc) break;
			if (pp->connects[i] == NOARCPROTO) break;
		}
		if (port != NOSRPORT) routingarc = NOARCPROTO;
	}

	/* if current arc cannot run, look for any that can */
	if (routingarc == NOARCPROTO)
	{
		/* check out all arcs for use in this route */
		for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
			for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
				ap->temp1 = 0;
		for(port = net->ports; port != NOSRPORT; port = port->next)
		{
			pp = port->pp;
			for(i = 0; pp->connects[i] != NOARCPROTO; i++)
				pp->connects[i]->temp1 = 1;
		}
		for(tech = el_technologies->nexttechnology; tech != NOTECHNOLOGY;
			tech = tech->nexttechnology)
		{
			for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			{
				if (ap->temp1 == 0) continue;
				for(port = net->ports; port != NOSRPORT; port = port->next)
				{
					pp = port->pp;
					for(i = 0; pp->connects[i] != NOARCPROTO; i++)
						if (pp->connects[i] == ap) break;
					if (pp->connects[i] == NOARCPROTO) break;
				}
				if (port == NOSRPORT) break;
			}
			if (ap != NOARCPROTO) break;
		}
		if (tech != NOTECHNOLOGY) routingarc = ap;
	}
	if (routingarc == NOARCPROTO)
	{
		ttyputerr(_("Cannot find wire to route"));
		return(TRUE);
	}
	ro_mazevertwire = routingarc;
	ro_mazehoriwire = routingarc;
	ro_mazesteiner = getpinproto(routingarc);

	/* initialize all layers and ports for this route */
	for (index = 0; index < SRMAXLAYERS; index++)
	{
		layer = net->region->layers[index];
		if (layer != NOSRLAYER)
		{
			/* unused bounds */
			layer->lx = layer->wid; layer->hx = -1;
			layer->ly = layer->hei; layer->hy = -1;

			/* now set the vertical and horizontal preference */
			for (x = 0, count = 0; x < layer->wid; x++)
			{
				if (layer->vused[x] & SR_GSET)
				{
					if (count)
					{
						for (i = 1, prio = count>>1; i <= count; i++, prio--)
							layer->vused[x-i] = abs(prio);
						count = 0;
					}
				} else count++;
			}

			/* get the remainder */
			if (count)
			{
				for (i = 1, prio = count>>1; i < count; i++)
				{
					layer->vused[layer->wid-i] = abs(prio);
					if (--prio == 0 && !(count&1)) prio = -1;
				}
			}

			/* now horizontal tracks */
			for (y = 0, count = 0; y < layer->hei; y++)
			{
				if (layer->hused[y] & SR_GSET)
				{
					if (count)
					{
						for (i = 1, prio = count>>1; i <= count; i++, prio--)
							layer->hused[y-i] = abs(prio);
						count = 0;
					}
				} else count++;
			}

			/* and the remainder ... */
			if (count)
			{
				for (i = 1, prio = count>>1; i < count; i++)
				{
					layer->hused[layer->hei-i] = abs(prio);
					if (--prio == 0 && !(count&1)) prio = -1;
				}
			}
		}
	}

	/* prepare each port for routing */
	for (port = net->ports, pcount = 0; port != NOSRPORT; port = port->next, pcount++)
		ro_mazecreate_wavefront(port);

	/* now begin routing until all ports merged */
	code = SR_GSTART;
	do
	{
		if (++code > SR_GMAX) code = SR_GSTART;
		if (stopping(STOPREASONROUTING)) return(TRUE);
		for (blocked = 0, port = net->ports; port != NOSRPORT; port = port->next)
		{
			/* if part of other wavefront, get the next one */
			if (port->master != NOSRPORT) continue;

			/* expand the wavefront */
			status = ro_mazeexpand_wavefront(port, code);
			if (ro_mazedebug)
			{
				CHAR *mstatus;
				if (status == SRERROR) mstatus = x_("error"); else
				if (status == SRROUTED) mstatus = x_("routed"); else
				if (status == SRBLOCKED) mstatus = x_("blocked"); else
					mstatus = x_("unknown");
				ro_mazedump_layer(mstatus, net->region, (UINTBIG)3);
			}
			if (status == SRERROR) return(TRUE);
			if (status == SRROUTED) break;
			if (status == SRBLOCKED) blocked++;
		}

		/* check for successful routing */
		if (port != NOSRPORT && status == SRROUTED)
		{
			/* now clear routing region and restart expansion */
			ro_mazeclear_maze(net);
			if (--pcount > 1)
			{
				/* prepare each port for routing */
				for (port = net->ports; port != NOSRPORT; port = port->next)
					ro_mazecreate_wavefront(port);
			}
			code = SR_GSTART;
		} else
		{
			/* check for blocked net */
			if (blocked == pcount)
			{
				ret = TRUE;
				ro_mazeclear_maze(net);
				break;
			}
		}
	} while (pcount > 1);

	/* move all the port paths to the net */
	for (port = net->ports; port != NOSRPORT; port = port->next)
	{
		if (net->lastpath == NOSRPATH)
		{
			net->paths = port->paths;
			if (net->paths != NOSRPATH) net->lastpath = port->lastpath;
		} else
		{
			net->lastpath->next = port->paths;
			if (net->lastpath->next != NOSRPATH) net->lastpath = port->lastpath;
		}
		port->paths = NOSRPATH;
		port->lastpath = NOSRPATH;
	}
	if (!ret) net->routed = TRUE;
	return(ret);
}

INTBIG ro_anglediff(INTBIG ang1, INTBIG ang2)
{
	INTBIG diff;

	diff = abs(ang1 - ang2);
	if (diff > 180) diff = 360 - diff;
	return(diff);
}

TECHNOLOGY *ro_findtech(NODEINST *ni, PORTPROTO *pp)
{
	if (ni->proto->primindex != 0) return(ni->proto->tech);
	return(ro_findtech(pp->subnodeinst, pp->subportproto));
}

void ro_mazedump_layer(CHAR *message, SRREGION *region, UINTBIG layers)
{
	SRLAYER *layer;
	INTBIG x, y, hei, wid;
	INTBIG index;
	UINTBIG mask;
	CHAR gpt;

	/* scan for the first layer */
	for (index = 0, mask = 1; index < SRMAXLAYERS; index++, mask = mask<<1)
	{
		if (!(mask & layers)) continue;
		layer = region->layers[index];
		if (layer == NOSRLAYER) continue;
		break;
	}
	hei = layer->hei;
	wid = layer->wid;
	xprintf(ro_mazedebuggingout, x_("====================== %s ======================\n"), message);
	xprintf(ro_mazedebuggingout, x_("   "));
	for (x = 0; x < wid; x++) xprintf(ro_mazedebuggingout, x_("%ld"), x / 10);
	xprintf(ro_mazedebuggingout, x_("\n   "));
	for (x = 0; x < wid; x++) xprintf(ro_mazedebuggingout, x_("%ld"), x % 10);
	xprintf(ro_mazedebuggingout, x_("\n"));
	for (y = hei-1; y >= 0; y--)
	{
		xprintf(ro_mazedebuggingout, x_("%3d"), y);
		for (x = 0; x < wid; x++)
		{
			gpt = ' ';
			for (index = 0, mask = 1; index < SRMAXLAYERS; index++, mask = mask<<1)
			{
				if (!(mask & layers)) continue;
				layer = region->layers[index];
				if (layer == NOSRLAYER) continue;
				if (layer->grids[x][y] & SR_GSET)
				{
					if (layer->grids[x][y] & SR_GPORT) gpt = 'P'; else
						gpt = '*';
				} else
				{
					if (layer->grids[x][y] & SR_GWAVE) gpt = 'W'; else
						if (layer->grids[x][y] != 0)
							gpt = 'A' + (layer->grids[x][y] & SR_GMASK) - SR_GSTART;
				}
			}
			xprintf(ro_mazedebuggingout, x_("%c"), gpt);
		}
		xprintf(ro_mazedebuggingout, x_("\n"));
	}
	xprintf(ro_mazedebuggingout, x_("\n"));
}

#endif  /* ROUTTOOL - at top */
