/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: routriver.c
 * Routines for the river-routing option of the routing tool
 * Written by: Telle Whitney, Schlumberger Palo Alto Research
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

/*
 *	River Route - takes two sets of parallel points (connectors, ports, etc) and routes wires
 *        between them.  All wires are routed in a single layer with non intersecting lines.
 *
 *
 *                       p1        p2         p3      p4
 *                        |        |           |       |  /\ cell_off2
 *                       _|        |           |   ____|  \/
 *                      |          |           |  |
 *                    __|    ______|           |  |
 *                   |      |                  |  |
 *                   |   ___|      ____________|  |
 *                   |  |         | /\ pitch      |
 *                 __|  |      ___| \/____________|
 *  cell_off1 /\  |     |     |    <>|
 *            \/  |     |     |      |
 *               a1    a2    a3     a4
 *
 * Restrictions
 *   (1)     The distance between the ports (p1..pn) and (a1..an) is >= pitch
 *   (2)     The parameter "width" specifies the width of all wires
 *           The parameter "space" specifies the distance between wires
 *           pitch = 2*(width/2) + space = space + width
 *
 * Extension - allow routing to and from the two sides
 *
 *                        SIDE3
 *       ________________________________________
 *       |  route  |                  |  route  |
 *     S |  right  |                  |  left   | S
 *     I | (last)  |   normal right   | (last)  | I
 *     D |_________|  and left route  |_________| D
 *     E |  route  |     (middle)     |  route  | E
 *     4 |  left   |                  |  right  | 2
 *       | (first) |                  | (first) |
 *       |_________|__________________|_________|
 *                        SIDE1
 */

#include "config.h"
#if ROUTTOOL

#include "global.h"
#include "rout.h"
#include "tecgen.h"

#define ROUTEINX      1
#define ROUTEINY      2
#define ILLEGALROUTE -1

#define BOTTOP        1					/* bottom to top  -- side 1 to side 3 */
#define FROMSIDE      2					/* side   to top  -- side 2 or side 4 to side 3 */
#define TOSIDE        3					/* bottom to side -- side 1 to side 3 or side 2 */

/******** RPOINT ********/

#define NOSIDE -1
#define SIDE1   1
#define SIDE2   2
#define SIDE3   3
#define SIDE4   4

#define NULLRPOINT ((RPOINT *)0)

typedef struct rivpoint
{
	INTBIG           side;				/* the side this point is on */
	INTBIG           x, y;				/* points coordinates */
	INTBIG           first, second;		/* nonrotated coordinates */
	struct rivpoint *next;				/* next one in the list */
} RPOINT;

/******** RDESC ********/

#define NULLRDESC ((RDESC *)0)

typedef struct routedesc
{
	RPOINT           *from;
	RPOINT           *to;
	INTBIG            sortval;
	ARCINST          *unroutedwire1;
	ARCINST          *unroutedwire2;
	INTBIG            unroutedend1;
	INTBIG            unroutedend2;
	struct rpath     *path;
	struct routedesc *next;
} RDESC;

/******** RPATH ********/

#define NULLRPATH ((RPATH *)0)

typedef struct rpath
{
	INTBIG           width;				/* the width of this path */
	ARCPROTO        *pathtype;			/* the paty type for this wire */
	INTBIG           routetype;			/* how the wire needs to be routed - as above */
	RPOINT          *pathdesc;			/* the path */
	RPOINT          *lastp;				/* the last point on the path */
	struct rpath    *next;
} RPATH;

/******** TRANSFORM ********/

#define NULLTRANSFORM ((TRANSFORM *)0)

typedef struct itran
{
	INTBIG t11, t12;					/* graphics transformation */
	INTBIG t21, t22;
	INTBIG tx, ty;
} TRANSFORM;

/******** RCOORD ********/

#define NULLCOORD ((RCOORD *)0)

typedef struct coord
{
	INTBIG        val;					/* the coordinate */
	INTBIG        total;				/* number of wires voting for this coordinate */
	struct coord *next;
} RCOORD;

/******** VARIABLES ********/

typedef struct
{
	NODEINST  *topcell;
	BOOLEAN    cellvalid;
} MOVECELL;

static MOVECELL ro_rrmovecell;

typedef struct
{
	TRANSFORM    *origmatrix, *invmatrix;
	RDESC        *rightp, *leftp;
	INTBIG        fromline;				/* the initial coordinate of the route */
	INTBIG        toline;				/* final coordinate of the route */
	INTBIG        startright;			/* where to start wires on the right */
	INTBIG        startleft;			/* where to start wires on the left */
	INTBIG        height;
	INTBIG        llx, lly, urx, ury;
	INTBIG        xx;					/*  ROUTEINX route in X direction,
											ROUTEINY route in Y direction */
	RCOORD       *xaxis, *yaxis;		/* linked list of possible routing coordinates */
} ROUTEINFO;

static ROUTEINFO ro_rrinfo;
static BOOLEAN   ro_initialized = FALSE;

static TRANSFORM ro_rrnorot     = { 1,  0,	/* X increasing, y2>y1                */
									0,  1,
                                    0,  0};
static TRANSFORM ro_rrrot90     = { 0,  1,	/* Y decreasing, x2>x1                */
								   -1,  0,
                                    0,  0};
static TRANSFORM ro_rrrot180    = {-1,  0,	/* X decreasing, y2<y1                */
									0, -1,
                                    0,  0};
static TRANSFORM ro_rrrot270    = { 0, -1,	/* Y increasing, x2<x1                */
									1,  0,	/* or rot -90                         */
									0,  0};
static TRANSFORM ro_rrmirrorx   = {-1,  0,	/* X decreasing, y2>y1                */
								    0,  1,	/* mirror X coordinate, around Y axis */
								    0,  0};
static TRANSFORM ro_rrrot90mirx = { 0,  1,	/* Y increasing, x2>x1                */
									1,  0,	/* rot90 and mirror X                 */
									0,  0};
static TRANSFORM ro_rrmirrory   = { 1,  0,	/* X increasing, y2<y1                */
									0, -1,	/* mirror Y coordinate, around X axis */
									0,  0};
static TRANSFORM ro_rrmirxr90   = { 0, -1,	/* Y decreasing, x2<x1                */
								   -1,  0,	/* mirror X, rot90                    */
									0,  0};
											/*    original		inverse   */
static TRANSFORM ro_rrinverse   = { 1,  0,	/*   [ cos0  sin0] [ cos0  -sin0] */
									0,  1,	/*   [-sin0  cos0] [ sin0   cos0] */
									0,  0};	/*	tx,ty			  */

static INTBIG ro_rrbx, ro_rrby, ro_rrex, ro_rrey;

/* the parsing structure for interactive routing options */
static KEYWORD ro_riveropt[] =
{
	{x_("route"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("move-cell"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("abort"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP ro_riverp = {ro_riveropt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("River-routing action"), 0};

/******** prototypes for local routines ********/

static RPATH *ro_newrpath(INTBIG, ARCPROTO*, INTBIG);
static void ro_freerpath(RPATH*);
static RPOINT *ro_newrivpoint(RPATH*, INTBIG, INTBIG, RPOINT*);
static RPOINT *ro_newrvpoint(INTBIG, INTBIG, INTBIG);
static void ro_freerivpoint(RPOINT*);
static RCOORD *ro_newcoord(INTBIG);
static RDESC *ro_newroutedesc(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, ARCINST*, INTBIG, ARCINST*, INTBIG, RDESC*);
static void ro_freeroutedesc(RDESC*);
static RDESC **ro_newrdarray(INTBIG);
static void ro_freerdarray(RDESC**);
static void ro_initialize(void);
static void ro_clipwire(RPOINT*, INTBIG, INTBIG);
static BOOLEAN ro_check_points(RDESC*, INTBIG, INTBIG);
static RDESC *ro_addright(RDESC*, RDESC*);
static RDESC *ro_addleft(RDESC*, RDESC*);
static void ro_structure_points(RDESC*);
static BOOLEAN ro_check_structured_points(RDESC*, RDESC*, INTBIG, INTBIG, INTBIG);
static RDESC *ro_reverse(RDESC*);
static void ro_cleanup(void);
static INTBIG ro_routepathtype(RPOINT*, RPOINT*);
static RPATH *ro_makeorigpath(INTBIG, ARCPROTO*, INTBIG, RPOINT*, RPOINT*);
static RPATH *ro_addpath(RPATH*, INTBIG, ARCPROTO*, RPOINT*, RPOINT*, INTBIG, INTBIG, INTBIG);
static RPATH *ro_makesideorigpath(INTBIG, ARCPROTO*, INTBIG, RPOINT*, RPOINT*);
static RPATH *ro_sideaddpath(RPATH*, INTBIG, ARCPROTO*, RPOINT*, RPOINT*, INTBIG, INTBIG, INTBIG);
static BOOLEAN ro_process_right(INTBIG, ARCPROTO*, RDESC*, INTBIG, INTBIG, INTBIG);
static BOOLEAN ro_process_left(INTBIG, ARCPROTO*, RDESC*, INTBIG, INTBIG, INTBIG);
static void ro_remap_points(RPOINT*, TRANSFORM*);
static INTBIG ro_remap_second(INTBIG, TRANSFORM*);
static INTBIG ro_height_coordinate(INTBIG);
static BOOLEAN ro_calculate_height_and_process(RDESC*, RDESC*, INTBIG, INTBIG, INTBIG, INTBIG*);
static void ro_calculate_bb(INTBIG*, INTBIG*, INTBIG*, INTBIG*, RDESC*, RDESC*);
static BOOLEAN ro_sorted_rivrot(ARCPROTO*, RDESC*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static RDESC *ro_quicksort(RDESC*);
static BOOLEAN ro_unsorted_rivrot(ARCPROTO*, RDESC*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static void ro_checkthecell(NODEINST*);
static void ro_pseudomake(RDESC*, NODEPROTO*);
static void ro_makepseudogeometry(NODEPROTO*);
static NODEINST *ro_thenode(RDESC*, NODEPROTO*, RPOINT*, NODEPROTO*);
static PORTPROTO *ro_theport(PORTPROTO*, RDESC*, RPOINT*);
static void ro_makegeometry(RDESC*, NODEPROTO*);
static void ro_clear_flags(NODEPROTO*);
static BOOLEAN ro_isunroutedpin(NODEINST*);
static BOOLEAN ro_is_interesting_arc(ARCINST*);
static void ro_set_flags(ARCINST*);
static void ro_mark_tobedeleted(ARCINST*);
static BOOLEAN ro_delnodeinst(NODEINST*);
static void ro_kill_wires(NODEPROTO*);
static void ro_makethegeometry(NODEPROTO*);
static RCOORD *ro_tallyvote(RCOORD*, INTBIG);
static void ro_vote(INTBIG, INTBIG, INTBIG, INTBIG);
static INTBIG ro_simplefigxx(RCOORD*, RCOORD*, RCOORD*, RCOORD*, INTBIG, RCOORD**, RCOORD**);
static RCOORD *ro_largest(RCOORD*);
static RCOORD *ro_next_largest(RCOORD*, RCOORD*);
static void ro_figureoutrails(INTBIG);
static INTBIG ro_point_val(RPOINT*, INTBIG);
static void ro_swap_points(RDESC*);
static void ro_set_wires_to_rails(RDESC*);
static RDESC *ro_addwire(RDESC*, ARCINST*, BOOLEAN*);
static RDESC *ro_findhighlightedwires(NODEPROTO*, INTBIG*);
static RDESC *ro_findnormalwires(NODEPROTO*, INTBIG*);
static BOOLEAN ro_findwires(NODEPROTO*);
static BOOLEAN ro_move_instance(void);
static BOOLEAN ro_query_user(void);
static void ro_sumup(PORTARCINST*);
static int ro_rivpointascending(const void*, const void*);

/******************** MEMORY MANAGEMENT ROUTINES ****************************/

RPATH *ro_newrpath(INTBIG width, ARCPROTO *ptype, INTBIG routetype)
{
	RPATH *rp;

	rp = (RPATH *)emalloc(sizeof(RPATH), ro_tool->cluster);
	if (rp == 0) return(0);
	rp->width = width;   rp->pathtype = ptype;   rp->routetype = routetype;
	rp->pathdesc = NULLRPOINT;   rp->lastp = NULLRPOINT;
	rp->next = NULLRPATH;
	return(rp);
}

void ro_freerpath(RPATH *rp)
{
	RPOINT *rvp, *rvpnext;

	for(rvp = rp->pathdesc, rvpnext = (rvp ?rvp->next:NULLRPOINT);
		rvp; rvp = rvpnext, rvpnext = (rvpnext?rvpnext->next:NULLRPOINT))
			ro_freerivpoint(rvp);
	efree((CHAR *)rp);
}

RPOINT *ro_newrivpoint(RPATH *rp, INTBIG first, INTBIG sec, RPOINT *next)
{
	RPOINT *rvp;

	rvp = (RPOINT *)emalloc(sizeof(RPOINT), ro_tool->cluster);
	if (rvp == 0) return(0);
	rvp->first = first;   rvp->second = sec;   rvp->x = 0;   rvp->y = 0;
	rvp->side = NOSIDE;
	rvp->next = next;
	if (rvp->next != 0) return(rvp);
	rp->lastp = rvp;
	return(rvp);
}

/* called to set up the route */
RPOINT *ro_newrvpoint(INTBIG x, INTBIG y, INTBIG side)
{
	RPOINT *rvp;

	rvp = (RPOINT *)emalloc(sizeof(RPOINT), ro_tool->cluster);
	if (rvp == 0) return(0);
	rvp->x = x;   rvp->y = y;   rvp->first = 0;   rvp->second = 0;
	rvp->side = side;
	rvp->next = NULLRPOINT;
	return(rvp);
}

void ro_freerivpoint(RPOINT *rvp)
{
	efree((CHAR *)rvp);
}

RCOORD *ro_newcoord(INTBIG c)
{
	RCOORD *cc;

	cc = (RCOORD *)emalloc(sizeof(RCOORD), ro_tool->cluster);
	if (cc == 0) return(0);
	cc->val = c;   cc->total = 0;
	cc->next = NULLCOORD;
	return(cc);
}

RDESC *ro_newroutedesc(INTBIG fx, INTBIG fy, INTBIG fside, INTBIG sx, INTBIG sy,
	INTBIG sside, ARCINST *ai1, INTBIG ae1, ARCINST *ai2, INTBIG ae2, RDESC *next)
{
	RDESC *rd;

	rd = (RDESC *)emalloc(sizeof(RDESC), ro_tool->cluster);
	if (rd == 0) return(0);
	rd->from = ro_newrvpoint(fx, fy, fside);
	if (rd->from == 0) return(0);
	rd->to = ro_newrvpoint(sx, sy, sside);
	if (rd->to == 0) return(0);
	rd->sortval = -MAXINTBIG;
	rd->unroutedwire1 = ai1;   rd->unroutedend1 = ae1;
	rd->unroutedwire2 = ai2;   rd->unroutedend2 = ae2;
	rd->path = NULLRPATH;
	rd->next = next;
	return(rd);
}

void ro_freeroutedesc(RDESC *rd)
{
	ro_freerivpoint(rd->from);   ro_freerivpoint(rd->to);
	efree((CHAR *)rd);
}

RDESC **ro_newrdarray(INTBIG size)
{
	RDESC **r;
	INTBIG i;

	r = (RDESC **)emalloc(((size+1)*sizeof(RDESC *)), ro_tool->cluster);
	if (r == 0) return(0);
	for(i=0; i<=size; i++) r[i] = NULLRDESC;
	return(r);
}

void ro_freerdarray(RDESC **r)
{
	efree((CHAR *)r);
}

void ro_initialize(void)
{
	RCOORD *c;

	ro_rrinfo.rightp = NULLRDESC;   ro_rrinfo.leftp = NULLRDESC;
	ro_rrinfo.origmatrix = NULLTRANSFORM;
	ro_rrinfo.invmatrix = NULLTRANSFORM;
	ro_rrinfo.fromline = ro_rrinfo.toline = -MAXINTBIG;
	ro_rrinfo.startright = -MAXINTBIG;   ro_rrinfo.startleft = -MAXINTBIG;
	ro_rrinfo.height = -MAXINTBIG;   ro_rrinfo.xx = ILLEGALROUTE;

	if (!ro_initialized)
	{
		ro_initialized = TRUE;
		ro_rrinfo.xaxis = 0;
		ro_rrinfo.yaxis = 0;
	}
	for(c = ro_rrinfo.xaxis; c; c = c->next) c->total = -1;
	for(c = ro_rrinfo.yaxis; c; c = c->next) c->total = -1;
	ro_rrmovecell.topcell = NONODEINST;   ro_rrmovecell.cellvalid = TRUE;
}

void ro_freerivermemory(void)
{
	RCOORD *c;

	while (ro_rrinfo.xaxis != 0)
	{
		c = ro_rrinfo.xaxis;
		ro_rrinfo.xaxis = ro_rrinfo.xaxis->next;
		efree((CHAR *)c);
	}
	while (ro_rrinfo.yaxis != 0)
	{
		c = ro_rrinfo.yaxis;
		ro_rrinfo.yaxis = ro_rrinfo.yaxis->next;
		efree((CHAR *)c);
	}
}

/************************************************************************/
/*	Point validation - ensure that the paths are valid		*/
/*		havoc occurs when the data is not what the program	*/
/*		expects.  It does something but wires get shorted	*/
/*		together and terminals don't connect			*/
/************************************************************************/

void ro_clipwire(RPOINT *p, INTBIG b1, INTBIG b2)
{
	INTBIG diff1, diff2;

	diff1 = abs(b1 - p->first);   diff2 = abs(b2 - p->first);
	if (diff1 < diff2)
	{
		p->first = b1;   p->side = SIDE4;
	} else
	{
		p->first = b2;   p->side = SIDE2;
	}
}

BOOLEAN ro_check_points(RDESC *listp, INTBIG width, INTBIG space)
{
	RDESC *llist, *listlast;
	RPOINT *lastfrom, *lastto;
	TRANSFORM *tmatrix;
	INTBIG val1, val2;
	INTBIG diff1, diff2;
	INTBIG bound1, bound2;
	INTBIG temp;

	if (listp == NULLRDESC)
	{
		/* need at least one point */
		ttyputerr(_("River router: Not enought points"));
		return(FALSE);
	}

	listlast = NULLRDESC;
	for(llist = listp; llist; llist=llist->next) listlast = llist;
	if (!listlast->from || !listlast->to)
	{
		ttyputerr(_("River router: Not the same number of points"));
		return(FALSE);	/* not the same number of points */
	}

	/*  decide route orientation */
	if (ro_rrinfo.xx == ROUTEINX)
	{
		/* route in x direction */
		if (listp->to->x >= listp->from->x)
		{						/* x2>x1 */
			if (listlast->from->y >= listp->from->y)
				tmatrix = &ro_rrrot90mirx;			/* Y increasing */
					else tmatrix = &ro_rrrot90;		/* Y decreasing */
		} else
		{						/* x2<x1 */
			if (listlast->from->y >= listp->from->y)
				tmatrix = &ro_rrrot270;				/* Y increasing */
					else tmatrix = &ro_rrmirxr90;	/* Y decreasing */
		}
		val1 = ro_rrinfo.fromline = ro_rrinfo.fromline*tmatrix->t12;
		val2 = ro_rrinfo.toline = ro_rrinfo.toline*tmatrix->t12;
	} else if (ro_rrinfo.xx == ROUTEINY)
	{
		/* route in y direction */
		if (listp->to->y >= listp->from->y)
		{						/* y2>y1 */
			if (listlast->from->x >= listp->from->x)
				tmatrix = &ro_rrnorot;				/* X increasing */
					else tmatrix = &ro_rrmirrorx;	/* X decreasing */
		} else
		{						/* y2<y1 */
			if (listlast->from->x >= listp->from->x)
				tmatrix = &ro_rrmirrory;				/* X increasing */
					else tmatrix = &ro_rrrot180;		/* X decreasing */
		}
		val1 = ro_rrinfo.fromline = ro_rrinfo.fromline*tmatrix->t22;
		val2 = ro_rrinfo.toline = ro_rrinfo.toline*tmatrix->t22;
	} else
	{
		ttyputerr(_("River router: Not between two parallel lines"));
		return(FALSE);		/* not on manhattan parallel lines */
	}

	/* check ordering of coordinates */
	for (llist = listp; llist != NULLRDESC; llist = llist->next)
	{
		/* do not check the last point */
		if (llist->next == NULLRDESC) continue;

		/* make sure there are no crossings */
		if (ro_rrinfo.xx == ROUTEINY)
		{
			if ((llist->from->x > llist->next->from->x && llist->to->x < llist->next->to->x) ||
				(llist->from->x < llist->next->from->x && llist->to->x > llist->next->to->x))
			{
				ttyputerr(_("River router: Connections may not cross"));
				return(FALSE);
			}
		} else
		{
			if ((llist->from->y > llist->next->from->y && llist->to->y < llist->next->to->y) ||
				(llist->from->y < llist->next->from->y && llist->to->y > llist->next->to->y))
			{
				ttyputerr(_("River router: Connections may not cross"));
				return(FALSE);
			}
		}
	}

	bound1 = ro_rrinfo.llx*tmatrix->t11 + ro_rrinfo.lly*tmatrix->t21;
	bound2 = ro_rrinfo.urx*tmatrix->t11 + ro_rrinfo.ury*tmatrix->t21;
	if (bound2 < bound1)
	{
		temp = bound2;   bound2 = bound1;   bound1 = temp;
	}
	lastfrom = NULLRPOINT;   lastto = NULLRPOINT;

	/* transform points and clip to boundary */
	for(llist = listp; llist; llist = llist->next)
	{
		llist->from->first = (llist->from->x*tmatrix->t11) + (llist->from->y*tmatrix->t21);
		llist->from->second = (llist->from->x*tmatrix->t12) + (llist->from->y*tmatrix->t22);
		llist->to->first = (llist->to->x*tmatrix->t11) + (llist->to->y*tmatrix->t21);
		llist->to->second = (llist->to->x*tmatrix->t12) + (llist->to->y*tmatrix->t22);
		if (llist->from->second != val1) ro_clipwire(llist->from, bound1, bound2);
		if (llist->to->second != val2)  ro_clipwire(llist->to, bound1, bound2);

		if (lastfrom && llist->from->side == SIDE1)
		{
			diff1 = abs(lastfrom->first - llist->from->first);
			if (diff1 < width+space)
			{
				ttyputerr(_("River router: Ports not design rule distance apart"));
				return(FALSE);
			}
		}
		if (lastto && llist->to->side == SIDE3)
		{
			diff2 = abs(lastto->first - llist->to->first);
			if (diff2 < width+space)
			{
				ttyputerr(_("River router: Ports not design rule distance apart"));
				return(FALSE);
			}
		}		/* not far enough apart */
		lastfrom = ((llist->from->side == SIDE1) ? llist->from : NULLRPOINT);
		lastto = ((llist->to->side == SIDE3) ? llist->to : NULLRPOINT);
	}

	/* matrix to take route back to original coordinate system */
	ro_rrinverse.t11 = tmatrix->t11;   ro_rrinverse.t12 = tmatrix->t21;
	ro_rrinverse.t21 = tmatrix->t12;   ro_rrinverse.t22 = tmatrix->t22;
	ro_rrinverse.tx = listp->from->first; ro_rrinverse.ty = listp->from->second;
								/* right now these last terms are not used */
	ro_rrinfo.origmatrix = tmatrix;   ro_rrinfo.invmatrix = &ro_rrinverse;
	ro_rrinfo.fromline = val1;   ro_rrinfo.toline = val2;
	return(TRUE);
}

RDESC *ro_addright(RDESC *r, RDESC *npb)
{
	if (r) r->next = npb;
		else ro_rrinfo.rightp = npb;
	return(npb);
}

RDESC *ro_addleft(RDESC *l, RDESC *npb)
{
	if (l) l->next = npb;
		else ro_rrinfo.leftp = npb;
	return(npb);
}


void ro_structure_points(RDESC *listr)
{
	RDESC *rd, *llist, *rlist;

	rlist = NULLRDESC; llist = NULLRDESC;
	for(rd = listr; rd; rd = rd->next)
	{
		if (rd->to->first >= rd->from->first) rlist = ro_addright(rlist, rd);
			else llist = ro_addleft(llist, rd);
	}
	if (rlist) rlist->next = NULLRDESC;
	if (llist) llist->next = NULLRDESC;
}

BOOLEAN ro_check_structured_points(RDESC *right, RDESC *left, INTBIG co1, INTBIG width, INTBIG space)
{
	RDESC *r, *l;
	INTBIG fromside1, toside2, fromside2, toside3, toside4;
	INTBIG botoffs2, botoffs4;

	fromside1 = FALSE;   toside2 = FALSE;   botoffs2 = 0;

	/* ensure ordering is correct */
	for(r = right; r; r = r->next)
	{
		switch (r->from->side)
		{
			case SIDE1:
				fromside1 = TRUE;
				break;
			default:
			case SIDE2:
			case SIDE3:
				ttyputerr(_("River router: Improper sides for bottom right ports"));
				return(FALSE);
			case SIDE4:
				if (fromside1)
				{
					ttyputerr(_("River router: Improper ordering of bottom right ports"));
					return(FALSE);
				}
				break;
		}
		switch (r->to->side)
		{
			case SIDE1:
			case SIDE4:
			default:
				ttyputerr(_("River router: Improper sides for top right ports"));
				return(FALSE);
			case SIDE2:
				if (!toside2) botoffs2 = ro_rrinfo.fromline+co1+(width/2);
					else botoffs2 += space+width;
				toside2 = TRUE;
				break;
			case SIDE3:
				if (toside2)
				{
					ttyputerr(_("River router: Improper ordering of top right ports"));
					return(FALSE);
				}
				break;
		}
	}

	fromside2 = FALSE;   toside3 = FALSE;   toside4 = FALSE;   botoffs4 = 0;
	for(l = left; l; l = l->next)
	{
		switch (l->from->side)
		{
			case SIDE1:
				if (fromside2)
				{
					ttyputerr(_("River router: Improper Ordering of Bottom Left Ports"));
					return(FALSE);
				}
				break;
			case SIDE2:
				fromside2 = TRUE;
				break;
			case SIDE3:
			case SIDE4:
			default:
				ttyputerr(_("River router: Improper sides for Bottom Left Ports"));
				return(FALSE);
		}
		switch (l->to->side)
		{
			case SIDE1:
			case SIDE2:
			default:
				ttyputerr(_("River router: Improper sides for Top Left Ports"));
				return(FALSE);
			case SIDE3:
				toside3 = TRUE;
				break;
			case SIDE4:
				if (!toside3)
				{
					if (!toside4) botoffs4 = ro_rrinfo.fromline+co1+(width/2);
						else botoffs4 += space+width;
				} else
				{
					ttyputerr(_("River router: Improper Ordering of Top Left Ports"));
					return(FALSE);
				}
				toside4 = TRUE;
				break;
		}
	}
	if (botoffs2 == 0) ro_rrinfo.startright = ro_rrinfo.fromline+co1+(width/2);
		else	       ro_rrinfo.startright = botoffs2+space+width;

	if (botoffs4 == 0) ro_rrinfo.startleft = ro_rrinfo.fromline+co1+(width/2);
		else	       ro_rrinfo.startleft = botoffs4+space+width;
	return(TRUE);
}

RDESC *ro_reverse(RDESC *p)
{
	RDESC *q, *r;

	if (!p) return(NULLRDESC);
	if (!p->next) return(p);

	q = p;   p = p->next;   q->next = NULLRDESC;
	while (p) { r = p->next;   p->next = q;   q = p;   p = r;}
	return(q);
}

void ro_cleanup(void)
{
	RDESC *rd, *rdnext;

	for(rd = ro_rrinfo.rightp, rdnext = (rd ?rd->next:NULLRDESC);
		rd; rd = rdnext, rdnext = (rdnext?rdnext->next:NULLRDESC))
	{
		if (rd->path) ro_freerpath(rd->path);
		ro_freeroutedesc(rd);
	}

	for(rd = ro_rrinfo.leftp, rdnext = (rd ?rd->next:NULLRDESC);
		rd; rd = rdnext, rdnext = (rdnext?rdnext->next:NULLRDESC))
	{
		if (rd->path) ro_freerpath(rd->path);
		ro_freeroutedesc(rd);
	}
}

/*********************** ACTUALLY DO THE ROUTE *******************************/

/*
 * the type of route for this wire: side to top, bottom to side, bottom to top
 */
INTBIG ro_routepathtype(RPOINT *b, RPOINT *t)
{
	if (b && t)
	{
		if (b->side != SIDE1) return(FROMSIDE);
		if (b->side != SIDE3) return(TOSIDE);
	}
	return(BOTTOP);
}

RPATH *ro_makeorigpath(INTBIG width, ARCPROTO *ptype, INTBIG co1, RPOINT *b, RPOINT *t)
{
	RPATH *rp;
	RPOINT *i1, *i2;

	rp = ro_newrpath(width, ptype, ro_routepathtype(b, t));
	if (rp == 0) return(0);

	i1 = ro_newrivpoint(rp, t->first, b->second+(width/2)+co1, NULLRPOINT);
	if (i1 == 0) return(0);
	i2 = ro_newrivpoint(rp, b->first, b->second+(width/2)+co1, i1);
	if (i2 == 0) return(0);
	rp->pathdesc = ro_newrivpoint(rp, b->first, b->second, i2);
	if (rp->pathdesc == 0) return(0);
	rp->lastp->side = t->side;
	return(rp);
}

RPATH *ro_addpath(RPATH *path, INTBIG width, ARCPROTO *ptype, RPOINT *b, RPOINT *t,
	INTBIG space, INTBIG co1,INTBIG dir)
{
	RPATH *rp;
	INTBIG newfirst, minfirst, maxfirst;
	RPOINT *lp, *lastp, *i1;

	rp = ro_newrpath(width, ptype, ro_routepathtype(b, t));
	if (rp == 0) return(0);
	i1 = ro_newrivpoint(rp, b->first, b->second+(rp->width/2)+co1, NULLRPOINT);
	if (i1 == 0) return(0);
	rp->pathdesc = ro_newrivpoint(rp, b->first, b->second, i1);
	if (rp->pathdesc == 0) return(0);
	minfirst = mini(b->first, t->first);
	maxfirst = maxi(b->first, t->first);
	lp = path->pathdesc;

	lastp = rp->lastp;

	newfirst = lp->first+dir*(space+rp->width);
	while (lp && minfirst <= newfirst && newfirst <= maxfirst)
	{
		/* if first point then inconsistent second(y) offset */
		if (lp == path->pathdesc)
			lastp->next = ro_newrivpoint(rp, newfirst, lastp->second, NULLRPOINT); else
				lastp->next = ro_newrivpoint(rp, newfirst, lp->second+space+rp->width, NULLRPOINT);
		if (lastp->next == 0) return(0);
		lastp = lastp->next;   lp = lp->next;
		if (lp) newfirst = lp->first+dir*(space+rp->width);
	}
	lastp->next = ro_newrivpoint(rp, t->first, lastp->second, NULLRPOINT);
	if (lastp->next == 0) return(0);
	rp->lastp->side = t->side;
	return(rp);
}

RPATH *ro_makesideorigpath(INTBIG width, ARCPROTO *ptype, INTBIG startoff, RPOINT *b, RPOINT *t)
{
	RPATH *rp;
	RPOINT *i1;

	rp = ro_newrpath(width, ptype, ro_routepathtype(b, t));
	if (rp == 0) return(0);
	i1 = ro_newrivpoint(rp, t->first, startoff, NULLRPOINT);
	if (i1 == 0) return(0);
	rp->pathdesc = ro_newrivpoint(rp, b->first, startoff, i1);
	if (rp->pathdesc == 0) return(0);
	rp->lastp->side = t->side;
	return(rp);
}

RPATH *ro_sideaddpath(RPATH *path, INTBIG  width, ARCPROTO *ptype, RPOINT *b, RPOINT *t,
	INTBIG space, INTBIG offset, INTBIG dir)
{
	RPATH *rp;
	INTBIG newfirst, minfirst, maxfirst;
	RPOINT *lp, *lastp;

	rp = ro_newrpath(width, ptype, ro_routepathtype(b, t));
	if (rp == 0) return(0);
	rp->pathdesc = ro_newrivpoint(rp, b->first, offset, NULLRPOINT);
	if (rp->pathdesc == 0) return(0);

	minfirst = mini(b->first, t->first);
	maxfirst = maxi(b->first, t->first);
	lp = path->pathdesc;

	lastp = rp->lastp;

	newfirst = lp->first+dir*(space+rp->width);
	while (lp && minfirst <= newfirst && newfirst <= maxfirst)
	{
		/* if first point then inconsistent second(y) offset */
		if (lp == path->pathdesc)
			lastp->next = ro_newrivpoint(rp, newfirst, maxi(lastp->second, offset), NULLRPOINT);
		else
			lastp->next = ro_newrivpoint(rp, newfirst, maxi(lp->second+space+rp->width, offset),
				NULLRPOINT);
		if (lastp->next == 0) return(0);
		lastp = lastp->next;   lp = lp->next;
		if (lp) newfirst = lp->first+dir*(space+rp->width);
	}
	lastp->next = ro_newrivpoint(rp, t->first, lastp->second, NULLRPOINT);
	if (lastp->next == 0) return(0);
	rp->lastp->side = t->side;
	return(rp);
}

BOOLEAN ro_process_right(INTBIG width, ARCPROTO *ptype, RDESC *rout, INTBIG co1, INTBIG space,
	INTBIG dir)
{
	BOOLEAN firsttime;
	RDESC *reversedrd, *rd;
	RPATH *lastp;
	INTBIG offset;

	firsttime = TRUE;   lastp = NULLRPATH;   offset = ro_rrinfo.startleft;

	for(rd = reversedrd = ro_reverse(rout); rd; rd = rd->next)
	{
		if (rd->from->side != SIDE4)
		{
			/* starting from bottom (side1) */
			if (firsttime)
			{
				rd->path = ro_makeorigpath(width, ptype, co1, rd->from, rd->to);
				if (rd->path == 0) return(TRUE);
				firsttime = FALSE;
			} else
				rd->path = ro_addpath(lastp, width, ptype, rd->from, rd->to, space, co1, dir);
			if (rd->path == 0) return(TRUE);
		} else
		{
			if (firsttime)
			{
				rd->path = ro_makesideorigpath(width, ptype, offset, rd->from, rd->to);
				if (rd->path == 0) return(TRUE);
				firsttime = FALSE;
			} else
			{
				rd->path = ro_sideaddpath(lastp, width, ptype, rd->from, rd->to, space, offset, dir);
				if (rd->path == 0) return(TRUE);
			}
			offset += space+width;
		}
		lastp = rd->path;
	}
	(void)ro_reverse(reversedrd);  /* return to normal */
	return(FALSE);
}

BOOLEAN ro_process_left(INTBIG width, ARCPROTO *ptype, RDESC *rout, INTBIG co1, INTBIG space,
	INTBIG dir)
{
	BOOLEAN firsttime;
	RPATH *lastp;
	INTBIG offset;

	firsttime = TRUE; lastp = NULLRPATH; offset = ro_rrinfo.startright;
	for(; rout; rout = rout->next)
	{
		if (rout->from->side != SIDE2)
		{
			if (firsttime)
			{
				rout->path = ro_makeorigpath(width, ptype, co1, rout->from, rout->to);
				if (rout->path == 0) return(TRUE);
				firsttime = FALSE;
			} else
			rout->path = ro_addpath(lastp, width, ptype, rout->from, rout->to, space, co1, dir);
			if (rout->path == 0) return(TRUE);
		} else
		{
			if (firsttime)
			{
				rout->path = ro_makesideorigpath(width, ptype, offset, rout->from, rout->to);
				if (rout->path == 0) return(TRUE);
				firsttime = FALSE;
			} else
			{
				rout->path = ro_sideaddpath(lastp, width, ptype, rout->from, rout->to, space,
					offset, dir);
				if (rout->path == 0) return(TRUE);
			}
			offset += space+width;
		}
		lastp = rout->path;
	}
	return(FALSE);
}

/*
 * calculate the height of the channel, and remap the points back into the
 * original coordinate system
 */
void ro_remap_points(RPOINT *rp, TRANSFORM *matrix)
{
	for(; rp; rp = rp->next)
	{
		rp->x = (rp->first*matrix->t11) + (rp->second*matrix->t21);
		rp->y = (rp->first*matrix->t12) + (rp->second*matrix->t22);
	}
}

INTBIG ro_remap_second(INTBIG sec, TRANSFORM *matrix)
{
	if (ro_rrinfo.xx == ROUTEINY) return(sec*matrix->t22);
	return(sec*matrix->t12);
}

/*
 * put it into the propoer coordinate system
 */
INTBIG ro_height_coordinate(INTBIG h)
{
	return(h+ro_rrinfo.fromline);
}

BOOLEAN ro_calculate_height_and_process(RDESC *right, RDESC *left, INTBIG width, INTBIG co2,
	INTBIG minheight, INTBIG *height)
{
	INTBIG maxheight;
	RDESC *rright, *lleft;
	RPOINT *lastp;

	maxheight = -MAXINTBIG;
	for(rright = right; rright; rright = rright->next)
		maxheight = maxi(maxheight, rright->path->lastp->second);

	for(lleft = left; lleft; lleft = lleft->next)
		maxheight = maxi(maxheight, lleft->path->lastp->second);

	if (minheight != 0) maxheight = maxi(minheight, maxheight+(width/2)+co2);
		else maxheight = maxheight+(width/2)+co2;
	maxheight = maxi(maxheight, ro_rrinfo.toline);

	/* make sure its at least where the coordinates are */
	for(rright = right; rright; rright = rright->next)
	{
		lastp =	rright->path->lastp;
		if (lastp->side != SIDE2)
		{
			lastp->next = ro_newrivpoint(rright->path, lastp->first, maxheight, NULLRPOINT);
			if (lastp->next == 0) return(FALSE);
		}
		ro_remap_points(rright->path->pathdesc, ro_rrinfo.invmatrix);
	}
	for(lleft = left; lleft; lleft = lleft->next)
	{
		lastp = lleft->path->lastp;
		if (lastp->side != SIDE4)
		{
			lastp->next = ro_newrivpoint(lleft->path, lastp->first, maxheight, NULLRPOINT);
			if (lastp->next == 0) return(FALSE);
		}
		ro_remap_points(lleft->path->pathdesc, ro_rrinfo.invmatrix);
	}
	ro_rrinfo.toline = ro_remap_second(ro_rrinfo.toline, ro_rrinfo.invmatrix);
	ro_rrinfo.fromline = ro_remap_second(ro_rrinfo.fromline, ro_rrinfo.invmatrix);
	*height = ro_remap_second(maxheight,ro_rrinfo.invmatrix);
	return(TRUE);
}

void ro_calculate_bb(INTBIG *llx, INTBIG *lly, INTBIG *urx, INTBIG *ury, RDESC *right, RDESC *left)
{
	RDESC *rright, *lleft;
	RPOINT *rvp;
	INTBIG lx, ly, ux, uy;

	lx = ly = MAXINTBIG; ux = uy = -MAXINTBIG;
	for(rright = right; rright; rright = rright->next)
	{
		for(rvp = rright->path->pathdesc; rvp; rvp = rvp->next)
		{
			lx = mini(lx, rvp->x);
			ly = mini(ly, rvp->y);
			ux = maxi(ux, rvp->x);
			uy = maxi(uy, rvp->y);
		}
	}
	for(lleft = left; lleft; lleft = lleft->next)
	{
		for(rvp = lleft->path->pathdesc; rvp; rvp = rvp->next)
		{
			lx = mini(lx, rvp->x);
			ly = mini(ly, rvp->y);
			ux = maxi(ux, rvp->x);
			uy = maxi(uy, rvp->y);
		}
	}
	*llx = lx;   *lly = ly;   *urx = ux;   *ury = uy;
}

/*
 * takes two sorted list of ports and routes between them
 * warning - if the width is not even, there will be round off problems
 */
BOOLEAN ro_sorted_rivrot(ARCPROTO *layerdesc, RDESC *listr, INTBIG width,
	INTBIG space, INTBIG celloff1, INTBIG celloff2, INTBIG fixedheight)
{
	INTBIG height;

	/* ports invalid */
	if (!ro_check_points(listr, width, space)) return(FALSE);
	ro_structure_points(listr);				/* put in left/right */
	if (!ro_check_structured_points(ro_rrinfo.rightp, ro_rrinfo.leftp, celloff1, width, space))
		return(FALSE);
	if (ro_process_right(width, layerdesc, ro_rrinfo.rightp, celloff1, space, -1)) return(FALSE);
	if (ro_process_left(width, layerdesc, ro_rrinfo.leftp, celloff1, space, 1)) return(FALSE);
	if (fixedheight != 0) fixedheight = ro_height_coordinate(fixedheight);
	if (!ro_calculate_height_and_process(ro_rrinfo.rightp, ro_rrinfo.leftp, width, celloff2,
		fixedheight, &height)) return(FALSE);
	if (fixedheight > 0 && height > fixedheight)
	{
		/* can't make it in fixed height */
		ttyputerr(_("River router: Unable to route in %s wide channel, need %s"), latoa(fixedheight, 0),
			latoa(height, 0));
		return(FALSE);
	}
	ro_calculate_bb(&ro_rrinfo.llx, &ro_rrinfo.lly, &ro_rrinfo.urx, &ro_rrinfo.ury,
		ro_rrinfo.rightp, ro_rrinfo.leftp);
	ro_rrinfo.height = height;
	return(TRUE);
}

/*
 * Helper routine for "esort" that makes river points go in ascending order.
 * Used from "ro_quicksort()".
 */
int ro_rivpointascending(const void *e1, const void *e2)
{
	REGISTER RDESC *c1, *c2;

	c1 = *((RDESC **)e1);
	c2 = *((RDESC **)e2);
	return(c1->sortval - c2->sortval);
}

/* Quicksorts the rivpoints */
RDESC *ro_quicksort(RDESC *rd)
{
	RDESC **thearray, *r;
	INTBIG xdir, i, len;

	for(len = 0, r = rd; r; len++, r = r->next)
		;
	if (len == 0) return(NULLRDESC);
	thearray = ro_newrdarray(len);
	if (thearray == 0) return(NULLRDESC);
	xdir = (ro_rrinfo.xx == ROUTEINX ? FALSE : TRUE);

	for(i=0; rd; i++, rd = rd->next)
	{
		thearray[i] = rd;
		rd->sortval = (xdir ? rd->from->x : rd->from->y);
	}

	/* instead of "ro_sortquick(0, len-1, len-1, thearray)"... */
	esort(thearray, len, sizeof (RDESC *), ro_rivpointascending);
	for(i=0; i<len; i++) thearray[i]->next = thearray[i+1];
	r = *thearray;   ro_freerdarray(thearray);
	return(r);
}

/*
 * takes two unsorted list of ports and routes between them
 * warning - if the width is not even, there will be round off problems
 */
BOOLEAN ro_unsorted_rivrot(ARCPROTO *layerdesc, RDESC *lists, INTBIG width,
	INTBIG space, INTBIG celloff1, INTBIG celloff2, INTBIG fixedheight)
{
	return(ro_sorted_rivrot(layerdesc, ro_quicksort(lists), width, space,
		celloff1, celloff2, fixedheight));
}

/*
 * once the route occurs, make some geometry and move some cells around
 */
void ro_checkthecell(NODEINST *ni)
{
	if (ni->proto->primindex == 0)
	{
		/* the node is nonprimitive */
		if (!ro_rrmovecell.cellvalid) return;
		if (ro_rrmovecell.topcell == NONODEINST)  /* first one */
			ro_rrmovecell.topcell = ni;
		else if (ro_rrmovecell.topcell != ni) ro_rrmovecell.cellvalid = FALSE;
	}
}

void ro_pseudomake(RDESC *rd, NODEPROTO *cell)
{
	RPATH *path;
	REGISTER RPOINT *rp, *prev;

	path = rd->path;

	prev = path->pathdesc;
	for(rp = prev->next; rp != NULLRPOINT; rp = rp->next)
	{
		if (rp->next)
		{
			if (prev->x == rp->x && rp->x == rp->next->x) continue;
			if (prev->y == rp->y && rp->y == rp->next->y) continue;
		}
		(void)asktool(us_tool, x_("show-line"), prev->x, prev->y, rp->x, rp->y, cell);
		prev = rp;
	}
	ro_checkthecell(rd->unroutedwire2->end[rd->unroutedend2].nodeinst);
}

/*
 * draw lines on the screen denoting what the route would look
 * like if it was done
 */
void ro_makepseudogeometry(NODEPROTO *cell)
{
	RDESC *q;
	INTBIG lambda;

	lambda = lambdaofcell(cell);
	ttyputmsg(_("Routing bounds %s <= X <= %s   %s <= Y <= %s"), latoa(ro_rrinfo.llx, lambda),
		latoa(ro_rrinfo.urx, lambda), latoa(ro_rrinfo.lly, lambda), latoa(ro_rrinfo.ury, lambda));

	/* remove highlighting */
	(void)asktool(us_tool, x_("clear"));

	for(q = ro_rrinfo.rightp; q != NULLRDESC; q = q->next) ro_pseudomake(q, cell);
	for(q = ro_rrinfo.leftp; q != NULLRDESC; q = q->next) ro_pseudomake(q, cell);
}

NODEINST *ro_thenode(RDESC *rd, NODEPROTO *dn, RPOINT *p, NODEPROTO *np)
{
	REGISTER NODEINST *ni;
	INTBIG wdiv2, hdiv2, xs, ys;

	if (p->x == ro_rrbx && p->y == ro_rrby)
		return(rd->unroutedwire1->end[rd->unroutedend1].nodeinst);

	if (p->x == ro_rrex && p->y == ro_rrey)
		return(rd->unroutedwire2->end[rd->unroutedend2].nodeinst);

	defaultnodesize(dn, &xs, &ys);
	wdiv2 = xs / 2;
	hdiv2 = ys / 2;
	ni = newnodeinst(dn, p->x - wdiv2, p->x + wdiv2, p->y - hdiv2, p->y + hdiv2, 0, 0, np);
	if (ni != NONODEINST) endobjectchange((INTBIG)ni, VNODEINST);
	return(ni);
}

PORTPROTO *ro_theport(PORTPROTO *dp, RDESC *rd, RPOINT *p)
{
	if (p->x == ro_rrbx && p->y == ro_rrby)
		return(rd->unroutedwire1->end[rd->unroutedend1].portarcinst->proto);

	if (p->x == ro_rrex && p->y == ro_rrey)
		return(rd->unroutedwire2->end[rd->unroutedend2].portarcinst->proto);

	return(dp);
}

/*
 * make electric geometry
 */
void ro_makegeometry(RDESC *rd, NODEPROTO *np)
{
	RPATH *path;
	REGISTER RPOINT *rp, *prev;
	NODEINST *prevnodeinst, *rpnodeinst;
	NODEPROTO *defnode;
	PORTPROTO *defport, *prevport, *rpport;
	REGISTER ARCINST *ai;

	path = rd->path;

	portposition(rd->unroutedwire1->end[rd->unroutedend1].nodeinst,
				 rd->unroutedwire1->end[rd->unroutedend1].portarcinst->proto, &ro_rrbx, &ro_rrby);
	portposition(rd->unroutedwire2->end[rd->unroutedend2].nodeinst,
				rd->unroutedwire2->end[rd->unroutedend2].portarcinst->proto, &ro_rrex, &ro_rrey);

	defnode = getpinproto(path->pathtype);
	defport = defnode->firstportproto; /* there is always only one */

	prev = path->pathdesc;
	prevnodeinst = ro_thenode(rd, defnode, prev, np);
	prevport = ro_theport(defport, rd, prev);

	for(rp = prev->next; rp != NULLRPOINT; rp = rp->next)
	{
		if (rp->next)
		{
			if (prev->x == rp->x && rp->x == rp->next->x) continue;
			if (prev->y == rp->y && rp->y == rp->next->y) continue;
		}
		rpnodeinst = ro_thenode(rd, defnode, rp, np);
		rpport = ro_theport(defport, rd, rp);

		ai = newarcinst(path->pathtype, path->width, FIXANG,
			prevnodeinst, prevport, prev->x, prev->y, rpnodeinst, rpport, rp->x, rp->y, np);
		if (ai != NOARCINST) endobjectchange((INTBIG)ai, VARCINST);
		prev = rp;   prevnodeinst = rpnodeinst;   prevport = rpport;
	}
}

void ro_clear_flags(NODEPROTO *np)
{
	ARCINST *ai;

	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		ai->temp1 = 0;
		ai->end[0].nodeinst->temp1 = 0;
		ai->end[1].nodeinst->temp1 = 0;
	}
}

/*
 * routine to return true if nodeinst "ni" is an unrouted pin
 */
BOOLEAN ro_isunroutedpin(NODEINST *ni)
{
	/* only want the unrouted pin */
	if (ni->proto != gen_unroutedpinprim && ni->proto != gen_univpinprim) return(FALSE);

	/* found one */
	return(TRUE);
}

BOOLEAN ro_is_interesting_arc(ARCINST *ai)
{
	/* skip arcs already considered */
	if (ai->temp1 != 0) return(FALSE);

	/* only want "unrouted" arc in generic technology */
	if (ai->proto != gen_unroutedarc) return(FALSE);

	return(TRUE);
}

void ro_set_flags(ARCINST *ai)
{
	ai->temp1++;
	if (ro_isunroutedpin(ai->end[0].nodeinst)) ai->end[0].nodeinst->temp1++;
	if (ro_isunroutedpin(ai->end[1].nodeinst)) ai->end[1].nodeinst->temp1++;
}

void ro_mark_tobedeleted(ARCINST *ai)
{
	PORTARCINST *pi;
	ARCINST *oai, *ae;
	INTBIG e;

	if (!ro_is_interesting_arc(ai)) return;

	ro_set_flags(ai);
	ae = ai;  e = 0;
	for(;;)
	{
		if (!ro_isunroutedpin(ae->end[e].nodeinst)) break;
		for(pi = ae->end[e].nodeinst->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			oai = pi->conarcinst;
			if (oai->temp1 == 0) break;
		}
		if (pi == NOPORTARCINST) break;
		ro_set_flags(oai);
		if (oai->end[0].nodeinst == ae->end[e].nodeinst) e = 1; else e = 0;
		ae = oai;
	}
	ae = ai;  e = 1;
	for(;;)
	{
		if (!ro_isunroutedpin(ae->end[e].nodeinst)) break;
		for(pi = ae->end[e].nodeinst->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			oai = pi->conarcinst;
			if (oai->temp1 == 0) break;
		}
		if (pi == NOPORTARCINST) break;
		ro_set_flags(oai);
		if (oai->end[0].nodeinst == ae->end[e].nodeinst) e = 1; else e = 0;
		ae = oai;
	}
}

BOOLEAN ro_delnodeinst(NODEINST *ni)
{
	REGISTER PORTARCINST *pi;

	/* see if any arcs connect to this node */
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		if (pi->conarcinst != NOARCINST) return(FALSE);
	}

	/* see if this nodeinst is a portinst of the cell */
	if (ni->firstportexpinst != NOPORTEXPINST) return(FALSE);

	/* now erase the nodeinst */
	startobjectchange((INTBIG)ni, VNODEINST);
	return(killnodeinst(ni));
}

void ro_kill_wires(NODEPROTO *np)
{
	ARCINST *ai;
	NODEINST *ni;

	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->temp1 != 0)
		{
			startobjectchange((INTBIG)ai, VARCINST);
			if (killarcinst(ai))
				ttyputmsg(_("River router: Error occured while killing arc"));
		}
	}
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->temp1 != 0 && ro_isunroutedpin(ni))
		{
			if (ro_delnodeinst(ni))
				ttyputmsg(_("River router: Error occured while killing node"));
		}
	}
}

void ro_makethegeometry(NODEPROTO *np)
{
	RDESC *q;

	(void)asktool(us_tool, x_("clear"));
	ro_clear_flags(np);
	for(q = ro_rrinfo.rightp; q != NULLRDESC; q = q->next)
	{
		ro_makegeometry(q, np);   ro_mark_tobedeleted(q->unroutedwire1);
		if (q->unroutedwire1 != q->unroutedwire2) ro_mark_tobedeleted(q->unroutedwire2);
	}
	for(q = ro_rrinfo.leftp; q != NULLRDESC; q = q->next)
	{
		ro_makegeometry(q, np);   ro_mark_tobedeleted(q->unroutedwire2);
		if (q->unroutedwire1 != q->unroutedwire2) ro_mark_tobedeleted(q->unroutedwire2);
	}
	ro_kill_wires(np);

}

/*
 * Figure out which way to route (x and y) and the top coordinate and
 * bottom coordinate
 */
RCOORD *ro_tallyvote(RCOORD *cc, INTBIG c)
{
	RCOORD *cclast, *ccinit;

	if (cc == NULLCOORD)
	{
		cc = ro_newcoord(c);
		if (cc == 0) return(0);
		cc->total = 1;
		return(cc);
	}

	ccinit = cc;
	for(cclast = NULLCOORD; (cc && cc->total >= 0 && cc->val != c); cclast = cc, cc = cc->next) ;
	if (cc == NULLCOORD)
	{
		cc = ro_newcoord(c);
		if (cc == 0) return(0);
		cclast->next = cc;
		cc->total = 1;
		return(ccinit);
	} else
	{
		if (cc->total <0)
		{
			cc->val = c; cc->total = 1;
		} else cc->total++;
	}
	return(ccinit);
}

void ro_vote(INTBIG ffx, INTBIG ffy, INTBIG ttx, INTBIG tty)
{
	ro_rrinfo.xaxis = ro_tallyvote(ro_rrinfo.xaxis, ffx);
	if (ro_rrinfo.xaxis == 0) return;
	ro_rrinfo.yaxis = ro_tallyvote(ro_rrinfo.yaxis, ffy);
	if (ro_rrinfo.yaxis == 0) return;
	ro_rrinfo.xaxis = ro_tallyvote(ro_rrinfo.xaxis, ttx);
	if (ro_rrinfo.xaxis == 0) return;
	ro_rrinfo.yaxis = ro_tallyvote(ro_rrinfo.yaxis, tty);
}

INTBIG ro_simplefigxx(RCOORD *x1, RCOORD *x2, RCOORD *y1, RCOORD *y2, INTBIG total, RCOORD **from,
	RCOORD **to)
{
	*from = NULLCOORD;   *to = NULLCOORD;
	if (x1 && x2 && x1->total == total && x2->total == total)
	{
		*from = x1;   *to = x2;
		return(ROUTEINX);
	}
	if (y1 && y2 && y1->total == total && y2->total == total)
	{
		*from = y1;   *to = y2;
		return(ROUTEINY);
	}
	if (x1 && x1->total == (2*total))
	{
		*from = *to = x1;
		return(ROUTEINX);
	}
	if (y1 && y1->total == (2*total))
	{
		*from = *to = y1;
		return(ROUTEINY);
	}
	return(ILLEGALROUTE);
}

RCOORD *ro_largest(RCOORD *cc)
{
	RCOORD *largest;

	for(largest = cc, cc = cc->next; cc; cc =cc->next)
	{
		if (cc->total > largest->total) largest = cc;
	}
	return(largest);
}

RCOORD *ro_next_largest(RCOORD *cc, RCOORD *largest)
{
	RCOORD *nlargest;

	for(nlargest = NULLCOORD; cc; cc =cc->next)
	{
		if ((!nlargest) && (cc != largest)) nlargest = cc; else
			if (nlargest && cc != largest && cc->total > nlargest->total) nlargest = cc;
	}
	return(nlargest);
}

void ro_figureoutrails(INTBIG total)
{
	INTBIG fxx;
	RCOORD *lx, *ly, *nlx, *nly, *from, *to, *tmp;

	from = to = NULLCOORD;
	lx = ro_largest(ro_rrinfo.xaxis);   ly = ro_largest(ro_rrinfo.yaxis);
	fxx = ro_simplefigxx(lx, (nlx = ro_next_largest(ro_rrinfo.xaxis, lx)),
		ly, (nly = ro_next_largest(ro_rrinfo.yaxis, ly)), total, &from, &to);
	if (fxx == ILLEGALROUTE)
	{
		if (lx->total >= total)
		{
			/* lx->total == total --- the other one an unusual case */
			/* lx->total > total  --- both go to the same line */
			fxx = ROUTEINX;   from = lx;
			to = (lx->total > total ? lx : nlx);
		} else if (ly->total >= total)
		{
			/* ly->total == total --- the other one an unusual case */
			/* ly->total > total  --- both go to the same line */
			fxx = ROUTEINY;   from = ly;
			to = (ly->total > total ? ly : nly);
		} else
		{
			fxx = (((ly->total+nly->total)>=(lx->total+nlx->total)) ? ROUTEINY : ROUTEINX);
			from = (fxx == ROUTEINY ? ly : lx);
			to = (fxx == ROUTEINY ? nly : nlx);
		}
	}

	if (to->val < from->val)
	{
		tmp = from;   from = to;   to = tmp;
	}

	ro_rrinfo.xx = fxx;
	ro_rrinfo.fromline = from->val;   ro_rrinfo.toline = to->val;
}

INTBIG ro_point_val(RPOINT *rp, INTBIG xx)
{
	return(xx == ROUTEINX ? rp->x : rp->y);
}

void ro_swap_points(RDESC *r)
{
	RPOINT *tmp;
	ARCINST *tmpwire;
	INTBIG tmpe;

	if (r->from->side != SIDE1 || r->to->side != SIDE3)
		ttyputerr(_("River router: Unexpected side designation"));

	tmp = r->from;   r->from = r->to;
	r->to = tmp;

	r->from->side = SIDE1;   r->to->side = SIDE3;
	tmpwire = r->unroutedwire1;   tmpe = r->unroutedend1;
	r->unroutedwire1 = r->unroutedwire2;   r->unroutedend1 = r->unroutedend2;
	r->unroutedwire2 = tmpwire;   r->unroutedend2 = tmpe;
}

void ro_set_wires_to_rails(RDESC *lists)
{
	RDESC *r;
	INTBIG fval, tval;

	for(r = lists; r; r = r->next)
	{
		fval = ro_point_val(r->from, ro_rrinfo.xx);
		tval = ro_point_val(r->to, ro_rrinfo.xx);
		if ((fval != ro_rrinfo.fromline && tval == ro_rrinfo.fromline) ||
			(tval != ro_rrinfo.toline && fval == ro_rrinfo.toline))
				ro_swap_points(r);
	}
}

/*
 * figure out the wires to route at all
 */
RDESC *ro_addwire(RDESC *list, ARCINST *ai, BOOLEAN *wireadded)
{
	PORTARCINST *pi;
	ARCINST *oai, *ae1, *ae2;
	INTBIG e1, e2;
	INTBIG bx, by, ex, ey;

	if (!ro_is_interesting_arc(ai))
	{
		*wireadded = FALSE;
		return(list);
	}
	*wireadded = TRUE;

	ai->temp1++;
	ae1 = ai;   e1 = 0;
	for(;;)
	{
		if (!ro_isunroutedpin(ae1->end[e1].nodeinst)) break;
		for(pi = ae1->end[e1].nodeinst->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			oai = pi->conarcinst;
			if (oai->temp1 == 0) break;
		}
		if (pi == NOPORTARCINST) break;
		oai->temp1++;
		if (oai->end[0].nodeinst == ae1->end[e1].nodeinst) e1 = 1; else e1 = 0;
		ae1 = oai;
	}
	ae2 = ai;   e2 = 1;
	for(;;)
	{
		if (!ro_isunroutedpin(ae2->end[e2].nodeinst)) break;
		for(pi = ae2->end[e2].nodeinst->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			oai = pi->conarcinst;
			if (oai->temp1 == 0) break;
		}
		if (pi == NOPORTARCINST) break;
		oai->temp1++;
		if (oai->end[0].nodeinst == ae2->end[e2].nodeinst) e2 = 1; else e2 = 0;
		ae2 = oai;
	}

	portposition(ae1->end[e1].nodeinst, ae1->end[e1].portarcinst->proto, &bx, &by);
	portposition(ae2->end[e2].nodeinst, ae2->end[e2].portarcinst->proto, &ex, &ey);
	list = ro_newroutedesc(bx, by, SIDE1, ex, ey, SIDE3, ae1, e1, ae2, e2, list);
	if (list == 0) return(0);
	ro_vote(list->from->x, list->from->y, list->to->x, list->to->y);
	ro_sumup(ae1->end[e1].portarcinst);
	ro_sumup(ae2->end[e2].portarcinst);
	return(list);
}

RDESC *ro_findhighlightedwires(NODEPROTO *np, INTBIG *tot)
{
	RDESC *thelist;
	GEOM **list;
	ARCINST *ai;
	INTBIG i, total;
	BOOLEAN wireadded;

	/* get list of all highlighted arcs */
	list = (GEOM **)asktool(us_tool, x_("get-all-arcs"));
	if (list[0] == NOGEOM)
	{
		total = 0;   tot = &total;
		return(NULLRDESC);
	}

	/* get boundary of highlight */
	(void)asktool(us_tool, x_("get-highlighted-area"), (INTBIG)&ro_rrinfo.llx,
		(INTBIG)&ro_rrinfo.urx, (INTBIG)&ro_rrinfo.lly, (INTBIG)&ro_rrinfo.ury);

	/* reset flags on all arcs in this cell */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst) ai->temp1 = 0;

	thelist = NULLRDESC;
	total = 0;

	/* search the list */
	for(i = 0; list[i] != NOGEOM; i++)
	{
		ai = list[i]->entryaddr.ai;
		thelist = ro_addwire(thelist, ai, &wireadded);
		if (thelist == 0) return(NULLRDESC);
		if (wireadded) total++;
	}

	*tot = total;
	return(thelist);
}

RDESC *ro_findnormalwires(NODEPROTO *np, INTBIG *tot)
{
	REGISTER ARCINST *ai;
	REGISTER INTBIG total;
	RDESC *thelist;
	INTBIG llx, lly, urx, ury;
	BOOLEAN wireadded;

	thelist = NULLRDESC;   llx = lly = MAXINTBIG;   urx = ury = -MAXINTBIG;
	total = 0;

	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst) ai->temp1 = 0;
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		thelist = ro_addwire(thelist, ai, &wireadded);
		if (thelist == 0) return(NULLRDESC);
		if (wireadded)
		{
			total++;
			llx = mini(mini(llx, thelist->from->x), thelist->to->x);
			lly = mini(mini(lly, thelist->from->y), thelist->to->y);
			urx = maxi(maxi(urx, thelist->from->x), thelist->to->x);
			ury = maxi(maxi(ury, thelist->from->y), thelist->to->y);
		}
	}
	ro_rrinfo.llx = llx;   ro_rrinfo.lly = lly;
	ro_rrinfo.urx = urx;   ro_rrinfo.ury = ury;
	*tot = total;
	return(thelist);
}

BOOLEAN ro_findwires(NODEPROTO *np)
{
	REGISTER ARCPROTO *ap, *wantap;
	REGISTER TECHNOLOGY *tech, *curtech;
	REGISTER INTBIG amt;
	static POLYGON *poly = NOPOLYGON;
	REGISTER ARCINST *ai;
	ARCINST arc;
	INTBIG total;
	INTBIG edge;
	RDESC *thelist;
	REGISTER VARIABLE *var;

	ro_initialize();
	if (np->primindex != 0)
	{
		ttyputerr(_("River router cannot route primitives"));
		return(FALSE);
	}

	curtech = np->tech;

	/* initialize the list of possible arc prototypes for routing */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			ap->temp1 = 0;

	/* look for all unrouted arcs */
	if (!(thelist = ro_findhighlightedwires(np, &total)))
		thelist = ro_findnormalwires(np, &total);

	if (!thelist) return(FALSE);

	/* first look for the current arcproto */
	wantap = NOARCPROTO;
	var = getval((INTBIG)us_tool, VTOOL, VARCPROTO, x_("USER_current_arc"));
	if (var != NOVARIABLE)
	{
		ap = (ARCPROTO *)var->addr;
		if (ap->tech->techindex != 0)
			if (ap->temp1 == total*2) wantap = ap;
	}

	/* look in the current technology if not */
	if (wantap == NOARCPROTO)
	{
		for(ap = curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			if (ap->temp1 == total*2)
		{
			wantap = ap;
			break;
		}
	}

	/* look for ANY arc prototype */
	if (wantap == NOARCPROTO)
		for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			if (ap->temp1 == total*2)
		{
			wantap = ap;
			break;
		}
	}

	if (wantap == NOARCPROTO)
	{
		ttyputerr(_("River router: Cannot find arc that will connect"));
		return(FALSE);
	}
	ttyputmsg(_("River routing with %s arcs"), describearcproto(wantap));

	ro_figureoutrails(total);   ro_set_wires_to_rails(thelist);

	/* figure out the worst design rule spacing for this type of arc */
	(void)needstaticpolygon(&poly, 4, ro_tool->cluster);
	ai = &arc;   initdummyarc(ai);
	ai->proto = wantap;
	ai->width = defaultarcwidth(wantap);
	ai->length = 10000;
	(void)arcpolys(ai, NOWINDOWPART);
	shapearcpoly(ai, 0, poly);
	amt = drcmindistance(ai->proto->tech, np->lib, poly->layer, ai->width,
		poly->layer, ai->width, FALSE, FALSE, &edge, 0);
	if (amt < 0) amt = lambdaofarc(ai);
	return(ro_unsorted_rivrot(wantap, thelist, ai->width, amt, amt, amt, 0));
}

BOOLEAN ro_move_instance(void)
{
	INTBIG lx, ly;
	NODEINST *ni;

	ni = ro_rrmovecell.topcell;
	if (!ro_rrmovecell.cellvalid || ni == NONODEINST)
	{
		ttyputmsg(_("River router: Cannot determine cell to move"));
		return(FALSE);
	}

	lx = (ro_rrinfo.xx == ROUTEINX ? ro_rrinfo.height + ni->lowx - ro_rrinfo.toline : ni->lowx);
	ly = (ro_rrinfo.xx == ROUTEINY ? ro_rrinfo.height + ni->lowy - ro_rrinfo.toline : ni->lowy);
	if (lx == ni->lowx && ly == ni->lowy) return(TRUE);
	startobjectchange((INTBIG)ni, VNODEINST);
	modifynodeinst(ni, lx - ni->lowx, ly - ni->lowy, lx - ni->lowx, ly - ni->lowy, 0, 0);
	endobjectchange((INTBIG)ni, VNODEINST);
	return(TRUE);
}

BOOLEAN ro_query_user(void)
{
	CHAR *par[MAXPARS];
	INTBIG count;

	/* wait for user response */
	for(;;)
	{
		count = ttygetparam(_("River-route option: "), &ro_riverp, MAXPARS, par);
		if (count == 0) continue;
		if (par[0][0] == 'r') break;
		if (par[0][0] == 'm')
		{
			if (ro_move_instance()) return(TRUE);
		}
		if (par[0][0] == 'a') return(FALSE);
	}
	return(TRUE);
}

BOOLEAN ro_river(NODEPROTO *np)
{
	BOOLEAN valid_route;
	RDESC *q;

	/* locate wires */
	if (ro_findwires(np))
	{
		/* see if user selection is requested */
		valid_route = TRUE;
		if ((ro_state&SELECT) == 0)
		{
			/* make wires */
			for(q = ro_rrinfo.rightp; q != NULLRDESC; q = q->next)
				ro_checkthecell(q->unroutedwire2->end[q->unroutedend2].nodeinst);
			for(q = ro_rrinfo.leftp; q != NULLRDESC; q = q->next)
				ro_checkthecell(q->unroutedwire2->end[q->unroutedend2].nodeinst);

			/* if there is motion to be done, do it */
			if (ro_rrmovecell.cellvalid && ro_rrmovecell.topcell != NONODEINST)
			{
				if (ro_move_instance()) ro_makethegeometry(np); else
					valid_route = FALSE;
			} else ro_makethegeometry(np);
		} else
		{
			/* show where wires will go and allow user confirmation */
			ro_makepseudogeometry(np);
			if (ro_query_user()) ro_makethegeometry(np); else
				valid_route = FALSE;
		}
	} else valid_route = FALSE;
	ro_cleanup();
	return(valid_route);
}

void ro_sumup(PORTARCINST *pi)
{
	REGISTER INTBIG i;

	/*
	 * for every layer (or arcproto) that this PORT allows to connect to it,
	 * increment the flag bits (temp1) IN the prototype thus indicating that
	 * this river route point is allowed to connect to it
	 */
	for(i=0; pi->proto->connects[i] != NOARCPROTO; i++)
		pi->proto->connects[i]->temp1++;
}

#endif  /* ROUTTOOL - at top */
