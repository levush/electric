/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: sc1route.c
 * Modules for routing with the QUISC Silicon Compiler
 * Written by: Andrew R. Kostiuk, Queen's University
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
#if SCTOOL

#include	<setjmp.h>
#include	<math.h>
#include    "global.h"
#include    "sc1.h"


/******************** Router Version Blerb ********************/

static CHAR *sc_routerblerb[] =
{
	M_("**          QUISC ROUTING MODULE - VERSION 1.04         **"),
	M_("Preliminary Channel Assignment:"),
	M_("    o  Squeeze cells together"),
	M_("    o  Below unless required above"),
	M_("    o  Includes stitches and lateral feeds"),
	M_("Feed Through Decision:"),
	M_("    o  Preferred window for path"),
	M_("    o  Include Fuzzy Window"),
	M_("Make Exports:"),
	M_("    o  Path to closest outside edge"),
	M_("Track Routing:"),
	M_("    o  Create Vertical Constraint Graph"),
	M_("    o  Create Zone Representation for channel"),
	M_("    o  Decrease height of VCG and maximize channel use"),
	0
} ;


/***********************************************************************
	File Variables
------------------------------------------------------------------------
*/

#define DEFAULT_VERBOSE				0		/* not verbose default */
#define DEFAULT_FUZZY_WINDOW_LIMIT	128000	/* fuzzy window for pass th. */

static SCROUTECONTROL sc_routecontrol =
{
	DEFAULT_VERBOSE,
	DEFAULT_FUZZY_WINDOW_LIMIT
};

static long		sc_feednumber;	/* global feed through number */

/***********************************************************************
	External Variables
------------------------------------------------------------------------
*/

extern jmp_buf		sc_please_stop;
extern SCCELL		*sc_curcell;

/* prototypes for local routines */
static int Sc_route_set_control(int, CHAR*[]);
static void Sc_route_show_control(void);
static int Sc_route_squeeze_cells(SCROWLIST*);
static int Sc_route_create_row_list(SCROWLIST*, SCROUTEROW**, SCCELL*);
static int Sc_route_create_channel_list(int, SCROUTECHANNEL**);
static int Sc_route_channel_assign(SCROUTEROW*, SCROUTECHANNEL*, SCCELL*);
static int Sc_route_nearest_port(SCROUTEPORT*, SCROUTENODE*, int, SCCELL*);
static int Sc_route_add_port_to_channel(SCROUTEPORT*, SCEXTNODE*, SCROUTECHANNEL*, int);
static int Sc_route_add_lateral_feed(SCROUTEPORT*, SCROUTECHANNEL*, int, int, SCCELL*);
static int Sc_route_create_pass_throughs(SCROUTECHANNEL*, SCROWLIST*);
static int Sc_route_between_ch_nodes(SCROUTECHNODE*, SCROUTECHNODE*, SCROUTECHANNEL*, SCROWLIST*);
static int Sc_route_min_port_pos(SCROUTECHNODE*);
static int Sc_route_max_port_pos(SCROUTECHNODE*);
static int Sc_route_port_position(SCROUTEPORT*);
static int Sc_route_insert_feed_through(SCNBPLACE*, SCROWLIST*, SCROUTECHANNEL*, int, SCROUTECHNODE*);
static void Sc_route_resolve_new_xpos(SCNBPLACE*, SCROWLIST*);
static int Sc_route_decide_exports(SCCELL*, SCROUTEEXPORT**);
static int Sc_route_create_special(SCNITREE*, SCROUTECHPORT*, int, SCROUTECHPORT**, SCCELL*);
static int Sc_route_tracks_in_channels(SCROUTECHANNEL*, SCCELL*);
static int Sc_route_create_VCG(SCROUTECHANNEL*, SCROUTEVCG**, SCCELL*);
static int Sc_route_VCG_create_dependents(SCROUTEVCG*, SCROUTECHANNEL*);
static int Sc_route_VCG_set_dependents(SCROUTEVCG*);
static int Sc_route_VCG_cyclic_check(SCROUTEVCG*, SCNBPLACE**, int*);
static int Sc_route_VCG_single_cycle(SCROUTEVCG*, SCROUTEVCG**);
static void Sc_route_print_VCG(SCROUTEVCGEDGE*, int);
static int Sc_route_create_ZRG(SCROUTECHANNEL*, SCROUTEZRG**);
static SCROUTECHNODE *Sc_route_find_leftmost_chnode(SCROUTECHNODE*);
static void Sc_route_create_zrg_temp_list(SCROUTECHNODE*, int, SCROUTECHNODE*[]);
static int Sc_route_zrg_list_compatible(SCROUTECHNODE*[], SCROUTEZRG*);
static int Sc_route_zrg_add_chnodes(SCROUTECHNODE*[], SCROUTEZRG*);
static void Sc_free_zrg(SCROUTEZRG*);
static int Sc_route_track_assignment(SCROUTEVCG*, SCROUTEZRG*, SCROUTECHNODE*, SCROUTETRACK**);
static void Sc_route_mark_zones(SCROUTECHNODE*, SCROUTEZRG*, int);
static SCROUTECHNODE *Sc_route_longest_VCG(SCROUTEVCG*);
static int Sc_route_path_length(SCROUTEVCG*);
static int Sc_route_add_node_to_track(SCROUTECHNODE*, SCROUTETRACK*);
static int Sc_route_find_best_nodes(SCROUTEVCG*, SCROUTEZRG*, SCROUTECHNODE*[], int);
static void Sc_route_delete_from_VCG(SCROUTETRACK*, SCROUTEVCG*);
static void Sc_route_mark_VCG(SCROUTEVCGEDGE*);
static int Sc_route_create_power_ties(SCROUTECHANNEL*, SCROUTEVCG*, SCCELL*);
static int Sc_route_create_ground_ties(SCROUTECHANNEL*, SCROUTEVCG*, SCCELL*);

/***********************************************************************
Module:  Sc_route
------------------------------------------------------------------------
Description:
	Main procedure of QUISC router.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route(count, pars);

Name		Type		Description
----		----		-----------
count		int			Number of parameters.
pars		*char[]		Array of pointer to parameters.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route(int count, CHAR *pars[])
{
	int			i, err, l;
	SCROUTE		*route;
	SCROUTEROW		*row_list;
	SCROUTECHANNEL	*channel_list;

	/* parameter check */
	if (count)
	{
		l = estrlen(pars[0]);
		l = maxi(l, 2);
		if (namesamen(pars[0], x_("set-control"), l) == 0)
			return(Sc_route_set_control(count - 1, &pars[1]));
		if (namesamen(pars[0], x_("show-control"), l) == 0)
		{
			Sc_route_show_control();
			return(SC_NOERROR);
		}
		return(Sc_seterrmsg(SC_ROUTE_XCMD, pars[0]));
	}

	for (i = 0; sc_routerblerb[i]; i++)
		ttyputverbose(TRANSLATE(sc_routerblerb[i]));

	/* check if working in a cell */
	if (sc_curcell == NULL)
		return(Sc_seterrmsg(SC_NOCELL));

	/* check if placement structure exists */
	if (sc_curcell->placement == NULL)
		return(Sc_seterrmsg(SC_CELL_NO_PLACE, sc_curcell->name));

	/* create route structure */
	(void)Sc_free_route(sc_curcell->route);
	route = (SCROUTE *)emalloc(sizeof(SCROUTE), sc_tool->cluster);
	if (route == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	sc_curcell->route = route;
	route->channels = NULL;
	route->exports = NULL;
	route->rows = NULL;

	/* show controlling parameters */
	if (sc_routecontrol.verbose)
		Sc_route_show_control();

	(void)Sc_cpu_time(TIME_RESET);
	ttyputmsg(_("Starting ROUTER..."));

	/* first squeeze cell together */
	if ((err = Sc_route_squeeze_cells(sc_curcell->placement->rows)))
		return(err);

	/* create list of rows and their usage of extracted nodes */
	row_list = NULL;
	if ((err = Sc_route_create_row_list(sc_curcell->placement->rows, &row_list,
		sc_curcell)))
			return(err);
	route->rows = row_list;
	ttyputverbose(M_("    Time to create Router Row List         = %s."),
		Sc_cpu_time(TIME_REL));

	/* create Route Channel List */
	if ((err = Sc_route_create_channel_list(sc_curcell->placement->num_rows
		+ 1, &channel_list)))
			return(err);
	ttyputverbose(M_("    Time to create Router Channel List     = %s."),
		Sc_cpu_time(TIME_REL));
	route->channels = channel_list;

	/* Do primary channel assignment */
	if ((err = Sc_route_channel_assign(row_list, channel_list, sc_curcell)))
		return(err);
	ttyputverbose(M_("    Time to do primary channel assignment  = %s."),
		Sc_cpu_time(TIME_REL));

	/* decide upon any pass through cells required */
	if ((err = Sc_route_create_pass_throughs(channel_list,
		sc_curcell->placement->rows)))
		return(err);
	ttyputverbose(M_("    Time to create pass throughs           = %s."),
		Sc_cpu_time(TIME_REL));

	/* decide upon export positions */
	if ((err = Sc_route_decide_exports(sc_curcell, &(route->exports))))
		return(err);
	ttyputverbose(M_("    Time to place exports                  = %s."),
		Sc_cpu_time(TIME_REL));

	/* route tracks in each channel */
	if ((err = Sc_route_tracks_in_channels(channel_list, sc_curcell)))
		return(err);
	ttyputverbose(M_("    Time to route tracks                   = %s."),
		Sc_cpu_time(TIME_REL));
	ttyputmsg(_("Done (time = %s)"), Sc_cpu_time(TIME_ABS));

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_set_control
------------------------------------------------------------------------
Description:
	Set the route control structure values.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_set_control(count, pars);

Name		Type		Description
----		----		-----------
count		int			Number of parameters.
pars		*char[]		Array of pointers to parameters.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_set_control(int count, CHAR *pars[])
{
	int		numcmd, l, n;

	if (count)
	{
		for (numcmd = 0; numcmd < count; numcmd++)
		{
			l = estrlen(pars[numcmd]);
			n = maxi(l, 2);
			if (namesamen(pars[numcmd], x_("verbose"), n) == 0)
			{
				sc_routecontrol.verbose = TRUE;
				continue;
			}
			if (namesamen(pars[numcmd], x_("feed-through-size"), n) == 0)
			{
				if (++numcmd < count)
					ScSetParameter(SC_PARAM_ROUTE_FEEDTHRU_SIZE, eatoi(pars[numcmd]));
				continue;
			}
			if (namesamen(pars[numcmd], x_("fuzzy-window-limit"), n) == 0)
			{
				if (++numcmd < count)
					sc_routecontrol.fuzzy_window_limit = eatoi(pars[numcmd]);
				continue;
			}
			if (namesamen(pars[numcmd], x_("port-x-min-distance"), n) == 0)
			{
				if (++numcmd < count)
					ScSetParameter(SC_PARAM_ROUTE_PORT_X_MIN_DIST, eatoi(pars[numcmd]));
				continue;
			}
			if (namesamen(pars[numcmd], x_("active-distance"), n) == 0)
			{
				if (++numcmd < count)
					ScSetParameter(SC_PARAM_ROUTE_ACTIVE_DIST, eatoi(pars[numcmd]));
				continue;
			}
			if (namesamen(pars[numcmd], x_("default"), n) == 0)
			{
				sc_routecontrol.verbose = DEFAULT_VERBOSE;
				ScSetParameter(SC_PARAM_ROUTE_FEEDTHRU_SIZE, DEFAULT_FEED_THROUGH_SIZE);
				ScSetParameter(SC_PARAM_ROUTE_PORT_X_MIN_DIST, DEFAULT_PORT_X_MIN_DISTANCE);
				sc_routecontrol.fuzzy_window_limit = DEFAULT_FUZZY_WINDOW_LIMIT;
				ScSetParameter(SC_PARAM_ROUTE_ACTIVE_DIST, DEFAULT_ACTIVE_DISTANCE);
				continue;
			}
			n = maxi(l, 5);
			if (namesamen(pars[numcmd], x_("no-verbose"), n) == 0)
			{
				sc_routecontrol.verbose = FALSE;
				continue;
			}
			return(Sc_seterrmsg(SC_ROUTE_SET_XCMD, pars[numcmd]));
		}
	} else
	{
		return(Sc_seterrmsg(SC_ROUTE_SET_NOCMD));
	}

	Sc_route_show_control();
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_show_control
------------------------------------------------------------------------
Description:
	Print the router control structure values.
------------------------------------------------------------------------
Calling Sequence:  Sc_route_show_control();
------------------------------------------------------------------------
*/

void Sc_route_show_control(void)
{
	CHAR	*str1;

	ttyputverbose(M_("************ ROUTER CONTROL STRUCTURE"));
	if (sc_routecontrol.verbose)
	{
		str1 = M_("TRUE");
	} else
	{
		str1 = M_("FALSE");
	}
	ttyputverbose(M_("Verbose output           =  %s"), str1);
	ttyputverbose(M_("Feed Through Size        =  %d"),
		ScGetParameter(SC_PARAM_ROUTE_FEEDTHRU_SIZE));
	ttyputverbose(M_("Port X Minimum Distance  =  %d"),
		ScGetParameter(SC_PARAM_ROUTE_PORT_X_MIN_DIST));
	ttyputverbose(M_("Fuzzy Window Limit       =  %d"),
		sc_routecontrol.fuzzy_window_limit);
	ttyputverbose(M_("Minimum Active distance  =  %d"),
		ScGetParameter(SC_PARAM_ROUTE_ACTIVE_DIST));
}

/***********************************************************************
Module:  Sc_route_squeeze_cells
------------------------------------------------------------------------
Description:
	Try to squeeze adjacent cells in a row as close together by
	checking where their active areas start and using the minimum
	active distance.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_squeeze_cells(rows);

Name		Type		Description
----		----		-----------
rows		*SCROWLIST	Pointer to start of row list.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_squeeze_cells(SCROWLIST *rows)
{
	SCROWLIST	*row;
	SCNBPLACE	*place, *place2;
	SCCELLNUMS	cell1_nums, cell2_nums;
	int		overlap;

	for (row = rows; row; row = row->next)
	{
		for (place = row->start; place; place = place->next)
		{
			if (place->next == NULL)
				continue;

			/* determine allowable overlap */
			Sc_leaf_cell_get_nums(place->cell->np, &cell1_nums);
			Sc_leaf_cell_get_nums(place->next->cell->np, &cell2_nums);
			if (row->row_num % 2)
			{
				/* odd row, cell are transposed */
				overlap = cell2_nums.right_active + cell1_nums.left_active
					- ScGetParameter(SC_PARAM_ROUTE_ACTIVE_DIST);
			} else
			{
				/* even row */
				overlap = cell1_nums.right_active + cell2_nums.left_active
					- ScGetParameter(SC_PARAM_ROUTE_ACTIVE_DIST);
			}

			/* move rest of row */
			for (place2 = place->next; place2; place2 = place2->next)
				place2->xpos -= overlap;
		}
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_create_row_list
------------------------------------------------------------------------
Description:
	Create list of which extracted nodes each member of the rows of
	a placement need connection to.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_create_row_list(rows, row_list, cell);

Name		Type			Description
----		----			-----------
rows		*SCROWLIST		Pointer to start of placement rows.
row_list	**SCROUTEROW	Address of where to write created list.
cell		*SCCELL			Pointer to parent cell.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_create_row_list(SCROWLIST *rows, SCROUTEROW **row_list, SCCELL *cell)
{
	SCROWLIST	*row;
	SCROUTEROW	*new_rrow, *first_rrow, *last_rrow;
	SCROUTENODE	*new_node, *same_node, *last_node;
	SCROUTEPORT	*new_port;
	SCEXTNODE	*enode;
	SCNBPLACE	*place;
	SCNIPORT	*port;
	int		i;

	/* clear all reference pointers in extracted node list */
	for (enode = cell->ex_nodes; enode; enode = enode->next)
		enode->ptr = NULL;

	/* create a route row list for each placement row */
	first_rrow = last_rrow = NULL;
	same_node = NULL;
	for (row = rows; row; row = row->next)
	{
		if (Sc_stop())
			longjmp(sc_please_stop, 1);
		new_rrow = (SCROUTEROW *)emalloc(sizeof(SCROUTEROW), sc_tool->cluster);
		if (new_rrow == 0)
			return(Sc_seterrmsg(SC_NOMEMORY));
		new_rrow->number = row->row_num;
		new_rrow->nodes = NULL;
		new_rrow->row = row;
		new_rrow->last = last_rrow;
		new_rrow->next = NULL;
		if (last_rrow)
		{
			last_rrow->next = new_rrow;
			last_rrow = new_rrow;
		} else
		{
			first_rrow = last_rrow = new_rrow;
		}

		/* create an entry of every extracted node in each row */
		last_node = NULL;
		for (enode = cell->ex_nodes; enode; enode = enode->next)
		{
			new_node = (SCROUTENODE *)emalloc(sizeof(SCROUTENODE), sc_tool->cluster);
			if (new_node == 0)
				return(Sc_seterrmsg(SC_NOMEMORY));
			new_node->ext_node = enode;
			new_node->row = new_rrow;
			new_node->firstport = NULL;
			new_node->lastport = NULL;
			new_node->same_next = NULL;
			new_node->same_last = same_node;
			new_node->next = NULL;
			if (last_node)
			{
				last_node->next = new_node;
			} else
			{
				new_rrow->nodes = new_node;
			}
			last_node = new_node;
			if (same_node)
			{
				same_node->same_next = new_node;
				same_node = same_node->next;
			} else
			{
				enode->ptr = (CHAR *)new_node;
			}
		}
		same_node = new_rrow->nodes;

		/* set reference to all ports on row */
		for (place = row->start; place; place = place->next)
		{
			for (port = place->cell->ports; port; port = port->next)
			{
				new_node = (SCROUTENODE *)port->ext_node->ptr;
				if (new_node)
				{
					for (i = 0; i < row->row_num; i++)
						new_node = new_node->same_next;
					new_port = (SCROUTEPORT *)emalloc(sizeof(SCROUTEPORT), sc_tool->cluster);
					if (new_port == 0)
						return(Sc_seterrmsg(SC_NOMEMORY));
					new_port->place = place;
					new_port->port = port;
					new_port->node = new_node;
					new_port->next = NULL;
					new_port->last = new_node->lastport;
					if (new_node->lastport)
					{
						new_node->lastport->next = new_port;
					} else
					{
						new_node->firstport = new_port;
					}
					new_node->lastport = new_port;
				}
			}
		}
	}

	*row_list = first_rrow;
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_create_channel_list
------------------------------------------------------------------------
Description:
	Create the basic channel list.  The number of channels is one more
	than the number of rows.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_create_channel_list(number, channels);

Name		Type				Description
----		----				-----------
number		int					Number of channels to create.
channels	**SCROUTECHANNEL	Address of where to write result list.
err			int					Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_create_channel_list(int number, SCROUTECHANNEL **channels)
{
	SCROUTECHANNEL	*new_chan, *first_chan, *last_chan;
	int			i;

	/* create channel list */
	first_chan = last_chan = NULL;
	for (i = 0; i < number; i++)
	{
		new_chan = (SCROUTECHANNEL *)emalloc(sizeof(SCROUTECHANNEL), sc_tool->cluster);
		if (new_chan == 0)
			return(Sc_seterrmsg(SC_NOMEMORY));
		new_chan->number = i;
		new_chan->nodes = NULL;
		new_chan->tracks = NULL;
		new_chan->next = NULL;
		new_chan->last = last_chan;
		if (last_chan)
		{
			last_chan->next = new_chan;
		} else
		{
			first_chan = new_chan;
		}
		last_chan = new_chan;
	}

	*channels = first_chan;
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_channel_assign
------------------------------------------------------------------------
Description:
	Do primary channel assignment for all ports.  The basis algorithm
	is:

		if no ports higher
			use below channel
		else
			if ports lower
				use channel with closest to other ports
			else
				use above channel
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_channel_assign(rows, channels, cell);

Name		Type			Description
----		----			-----------
rows		*SCROUTEROW		List of rows of ports.
channels	*SCROUTECHANNEL	List of channels.
cell		*SCCELL			Pointer to parent cell.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_channel_assign(SCROUTEROW *rows, SCROUTECHANNEL *channels, SCCELL *cell)
{
	SCROUTEROW		*row;
	SCROUTENODE		*node, *node2;
	SCROUTEPORT		*port;
	SCPORT		*xport;
	int			ports_above, ports_below, offset, err, direct;

	/* clear flags */
	for (row = rows; row; row = row->next)
	{
		for (node = row->nodes; node; node = node->next)
		{
			for (port = node->firstport; port; port = port->next)
				port->flags &= ~SCROUTESEEN;
		}
	}

	for (row = rows; row; row = row->next)
	{
		for (node = row->nodes; node; node = node->next)
		{
			if (node->firstport == NULL)
				continue;

			/* check for ports above */
			ports_above = FALSE;
			for (node2 = node->same_next; node2; node2 = node2->same_next)
			{
				if (node2->firstport)
				{
					ports_above = TRUE;
					break;
				}
			}

			/* if none found above, any ports in this list only going up */
			if (!ports_above && node->firstport != node->lastport)
			{
				for (port = node->firstport; port; port = port->next)
				{
					direct = Sc_leaf_port_direction(port->port->port);
					if ((direct & SCPORTDIRUP) && !(direct & SCPORTDIRDOWN))
					{
						ports_above = TRUE;
						break;
					}
				}
			}

			/* check for ports below */
			ports_below = FALSE;
			for (node2 = node->same_last; node2; node2 = node2->same_last)
			{
				if (node2->firstport)
				{
					ports_below = TRUE;
					break;
				}
			}

			/* if none found below, any ports in this row only going down */
			if (!ports_below && node->firstport != node->lastport)
			{
				for (port = node->firstport; port; port = port->next)
				{
					direct = Sc_leaf_port_direction(port->port->port);
					if ((direct & SCPORTDIRDOWN) && !(direct & SCPORTDIRUP))
					{
						ports_below = TRUE;
						break;
					}
				}
			}

			/* do not add if only one port unless an export */
			if (!ports_above && !ports_below)
			{
				if (node->firstport == node->lastport)
				{
					for (xport = cell->ports; xport; xport = xport->next)
					{
						if (xport->node->ports->ext_node == node->ext_node)
							break;
					}
					if (!xport)
						continue;

					/* if top row, put in above channel */
					if (row->number && row->next == NULL)
						ports_above = TRUE;
				}
			}

			/* assign ports to channel */
			for (port = node->firstport; port; port = port->next)
			{
				if (port->flags & SCROUTESEEN)
					continue;

				/* check how ports can be connected to */
				direct = Sc_leaf_port_direction(port->port->port);

				/* for ports both up and down */
				if (direct & SCPORTDIRUP && direct & SCPORTDIRDOWN)
				{
					if (!ports_above)
					{
						/* add to channel below */
						if ((err = Sc_route_add_port_to_channel(port,
							node->ext_node, channels, row->number)))
								return(err);
					} else
					{
						if (ports_below)
						{
							/* add to channel where closest */
							offset = Sc_route_nearest_port(port, node,
								row->number, cell);
							if (offset > 0)
							{
								offset = 1;
							} else
							{
								offset = 0;
							}
							if ((err = Sc_route_add_port_to_channel(port,
								node->ext_node, channels,
								row->number + offset)))
									return(err);
						} else
						{
							/* add to channel above */
							if ((err = Sc_route_add_port_to_channel(port,
								node->ext_node, channels, row->number + 1)))
									return(err);
						}
					}
					port->flags |= SCROUTESEEN;
				}

				/* for ports only up */
				else if (direct & SCPORTDIRUP)
				{
					/* add to channel above */
					if ((err = Sc_route_add_port_to_channel(port,
						node->ext_node, channels, row->number + 1)))
							return(err);
					port->flags |= SCROUTESEEN;
				}

				/* for ports only down */
				else if (direct & SCPORTDIRDOWN)
				{
					/* add to channel below */
					if ((err = Sc_route_add_port_to_channel(port,
						node->ext_node, channels, row->number)))
							return(err);
					port->flags |= SCROUTESEEN;
				}

				/* ports left */
				else if (direct & SCPORTDIRLEFT)
				{
					if ((err = Sc_route_add_lateral_feed(port, channels,
						ports_above, ports_below, cell)))
							return(err);
				}

				/* ports right */
				else if (direct & SCPORTDIRRIGHT)
				{
					if ((err = Sc_route_add_lateral_feed(port, channels,
						ports_above, ports_below, cell)))
							return(err);
				} else
				{
					ttyputmsg(_("ERROR - no direction for %s port %s"),
						port->place->cell->name,
						Sc_leaf_port_name(port->port->port));
					port->flags |= SCROUTESEEN;
				}
			}
		}
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_nearest_port
------------------------------------------------------------------------
Description:
	Return the offset to the row which has the closest port to the
	indicated port.  The offset is +1 for every row above, -1 for
	every row below.
------------------------------------------------------------------------
Calling Sequence:  offset = Sc_route_nearest_port(port, node, row_num,
							cell);

Name		Type			Description
----		----			-----------
port		*SCROUTEPORT	Pointer to current port.
node		*SCROUTENODE	Pointer to reference node.
row_num		int				Row number of port.
cell		*SCCELL			Pointer to parent cell.
offset		int				Returned offset of row of closest port.
------------------------------------------------------------------------
*/

int Sc_route_nearest_port(SCROUTEPORT *port, SCROUTENODE *node, int row_num, SCCELL *cell)
{
	int			offset, min_dist;
	int			which_row, dist;
	int			xpos1, xpos2;
	SCROUTENODE		*nnode;
	SCROUTEPORT		*nport;

	min_dist = MAXINTBIG;
	which_row = 0;
	if (row_num % 2)
	{
		xpos1 = port->place->xpos + port->place->cell->size -
			port->port->xpos;
	} else
	{
		xpos1 = port->place->xpos + port->port->xpos;
	}

	/* find closest above */
	offset = 0;
	for (nnode = node->same_next; nnode; nnode = nnode->same_next)
	{
		offset++;
		for (nport = nnode->firstport; nport; nport = nport->next)
		{
			dist = abs(offset) * cell->placement->avg_height * 2;
			if ((row_num + offset) % 2)
			{
				xpos2 = nport->place->xpos + nport->place->cell->size -
					nport->port->xpos;
			} else
			{
				xpos2 = nport->place->xpos + nport->port->xpos;
			}
			dist += abs(xpos2 - xpos1);
			if (dist < min_dist)
			{
				min_dist = dist;
				which_row = offset;
			}
		}
	}

	/* check below */
	offset = 0;
	for (nnode = node->same_last; nnode; nnode = nnode->same_last)
	{
		offset--;
		for (nport = nnode->firstport; nport; nport = nport->next)
		{
			dist = abs(offset) * cell->placement->avg_height * 2;
			if ((row_num + offset) % 2)
			{
				xpos2 = nport->place->xpos + nport->place->cell->size -
					nport->port->xpos;
			} else
			{
				xpos2 = nport->place->xpos + nport->port->xpos;
			}
			dist += abs(xpos2 - xpos1);
			if (dist < min_dist)
			{
				min_dist = dist;
				which_row = offset;
			}
		}
	}

	return(which_row);
}

/***********************************************************************
Module:  Sc_route_add_port_to_channel
------------------------------------------------------------------------
Description:
	Add the indicated port to the indicated channel.  Create node for
	that channel if it doesn't already exist.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_add_port_to_channel(port, ext_node,
						 channels, chan_num);

Name		Type			Description
----		----			-----------
port		*SCROUTEPORT	Pointer to route port.
ext_node	*SCEXTNODE		Value of reference extracted node.
channels	*SCROUTECHANNEL	Start of channel list.
chan_num	int				Number of wanted channel.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_add_port_to_channel(SCROUTEPORT *port, SCEXTNODE *ext_node,
	SCROUTECHANNEL *channels, int chan_num)
{
	SCROUTECHANNEL	*channel, *nchan;
	int			i;
	SCROUTECHNODE	*node, *nnode;
	SCROUTECHPORT	*nport;

	/* get correct channel */
	channel = channels;
	for (i = 0; i < chan_num; i++)
		channel = channel->next;

	/* check if node already exists for this channel */
	for (node = channel->nodes; node; node = node->next)
	{
		if (node->ext_node == ext_node)
			break;
	}
	if (!node)
	{
		node = (SCROUTECHNODE *)emalloc(sizeof(SCROUTECHNODE), sc_tool->cluster);
		if (node == 0)
			return(Sc_seterrmsg(SC_NOMEMORY));
		node->ext_node = ext_node;
		node->number = 0;
		node->firstport = NULL;
		node->lastport = NULL;
		node->channel = channel;
		node->flags = SCROUTESEEN;
		node->same_next = NULL;
		node->same_last = NULL;
		node->next = channel->nodes;
		channel->nodes = node;

		/* resolve any references to other channels */
		/* check previous channels */
		for (nchan = channel->last; nchan; nchan = nchan->last)
		{
			for (nnode = nchan->nodes; nnode; nnode = nnode->next)
			{
				if (nnode->ext_node == ext_node)
				{
					nnode->same_next = node;
					node->same_last = nnode;
					break;
				}
			}
			if (nnode)
				break;
		}

		/* check later channels */
		for (nchan = channel->next; nchan; nchan = nchan->next)
		{
			for (nnode = nchan->nodes; nnode; nnode = nnode->next)
			{
				if (nnode->ext_node == ext_node)
				{
					nnode->same_last = node;
					node->same_next = nnode;
					break;
				}
			}
			if (nnode)
				break;
		}
	}

	/* add port to node */
	nport = (SCROUTECHPORT *)emalloc(sizeof(SCROUTECHPORT), sc_tool->cluster);
	if (nport == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	nport->port = port;
	nport->node = node;
	nport->xpos = 0;
	nport->flags = 0;
	nport->next = NULL;
	nport->last = node->lastport;
	if (node->lastport)
	{
		node->lastport->next = nport;
	} else
	{
		node->firstport = nport;
	}
	node->lastport = nport;

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_add_lateral_feed
------------------------------------------------------------------------
Description:
	Add a lateral feed for the port indicated.  Add a "stitch" if port
	of same type adjecent, else add full lateral feed.  Add to
	appropriate channel(s) if full feed.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_add_lateral_feed(port, channels,
								ports_above, ports_below, cell);

Name		Type			Description
----		----			-----------
port		*SCROUTEPORT	Pointer to port in question.
channels	*SCROUTECHANNEL	List of channels.
ports_above	int				TRUE if ports above.
ports_below	int				TRUE if ports below.
cell		*SCCELL			Pointer to parent cell.
err			int				Returned error flag, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_add_lateral_feed(SCROUTEPORT *port, SCROUTECHANNEL *channels,
	int ports_above, int ports_below, SCCELL *cell)
{
	int			direct, sdirect, err, offset;
	SCNBPLACE		*nplace, *splace;
	SCNITREE		*sinst;
	SCNIPORT		*sport, *sport2;
	SCROUTEPORT		*port2, *nport;

	direct = Sc_leaf_port_direction(port->port->port);

	/* determine if stitch */
	nplace = NULL;
	if (direct & SCPORTDIRLEFT)
	{
		if (port->node->row->number % 2)
		{
			/* odd row */
			for (nplace = port->place->next; nplace; nplace = nplace->next)
			{
				if (nplace->cell->type == SCLEAFCELL)
					break;
			}
		} else
		{
			/* even row */
			for (nplace = port->place->last; nplace; nplace = nplace->last)
			{
				if (nplace->cell->type == SCLEAFCELL)
					break;
			}
		}
		sdirect = SCPORTDIRRIGHT;
	} else
	{
		if (port->node->row->number % 2)
		{
			/* odd row */
			for (nplace = port->place->last; nplace; nplace = nplace->last)
			{
				if (nplace->cell->type == SCLEAFCELL)
					break;
			}
		} else
		{
			/* even row */
			for (nplace = port->place->next; nplace; nplace = nplace->next)
			{
				if (nplace->cell->type == SCLEAFCELL)
					break;
			}
		}
		sdirect = SCPORTDIRLEFT;
	}
	if (nplace)
	{
		/* search for same port with correct direction */
		for (port2 = port->next; port2; port2 = port2->next)
		{
			if (port2->place == nplace &&
				Sc_leaf_port_direction(port2->port->port)
				== sdirect)
					break;
		}
		if (port2)
		{
			/* stitch feed */
			port->flags |= SCROUTESEEN;
			port2->flags |= SCROUTESEEN;
			splace = (SCNBPLACE *)emalloc(sizeof(SCNBPLACE), sc_tool->cluster);
			if (splace == 0)
				return(Sc_seterrmsg(SC_NOMEMORY));
			splace->cell = NULL;
			if ((sinst = Sc_new_instance(x_("Stitch"), SCSTITCH)) == 0)
				return(Sc_seterrmsg(SC_NOMEMORY));
			splace->cell = sinst;

			/* save two ports */
			if ((sport = Sc_new_instance_port(sinst)) == 0)
				return(Sc_seterrmsg(SC_NOMEMORY));
			sport->port = (CHAR *)port;
			if ((sport2 = Sc_new_instance_port(sinst)) == 0)
				return(Sc_seterrmsg(SC_NOMEMORY));
			sport2->port = (CHAR *)port2;

			/* insert in place */
			if (direct & SCPORTDIRLEFT)
			{
				if (port->node->row->number % 2)
				{
					splace->last = port->place;
					splace->next = port->place->next;
					if (splace->last) splace->last->next = splace;
					if (splace->next) splace->next->last = splace;
				} else
				{
					splace->last = port->place->last;
					splace->next = port->place;
					if (splace->last) splace->last->next = splace;
					if (splace->next) splace->next->last = splace;
				}
			} else
			{
				if (port->node->row->number % 2)
				{
					splace->last = port->place->last;
					splace->next = port->place;
					if (splace->last) splace->last->next = splace;
					if (splace->next) splace->next->last = splace;
				} else
				{
					splace->last = port->place;
					splace->next = port->place->next;
					if (splace->last) splace->last->next = splace;
					if (splace->next) splace->next->last = splace;
				}
			}
			return(SC_NOERROR);
		}
	}

	/* full lateral feed */
	port->flags |= SCROUTESEEN;
	splace = (SCNBPLACE *)emalloc(sizeof(SCNBPLACE), sc_tool->cluster);
	if (splace == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	splace->cell = NULL;
	if ((sinst = Sc_new_instance(x_("Lateral Feed"), SCLATERALFEED)) == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	sinst->size = ScGetParameter(SC_PARAM_ROUTE_FEEDTHRU_SIZE);
	splace->cell = sinst;

	/* save port */
	if ((sport = Sc_new_instance_port(sinst)) == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	sport->xpos = ScGetParameter(SC_PARAM_ROUTE_FEEDTHRU_SIZE) >> 1;

	/* create new route port */
	nport = (SCROUTEPORT *)emalloc(sizeof(SCROUTEPORT), sc_tool->cluster);
	if (nport == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	nport->place = port->place;
	nport->port = port->port;
	nport->node = port->node;
	nport->flags = 0;
	nport->last = NULL;
	nport->next = NULL;
	sport->port = (CHAR *)nport;

	/* insert in place */
	if (direct & SCPORTDIRLEFT)
	{
		if (port->node->row->number % 2)
		{
			splace->last = port->place;
			splace->next = port->place->next;
		} else
		{
			splace->last = port->place->last;
			splace->next = port->place;
		}
	} else
	{
		if (port->node->row->number % 2)
		{
			splace->last = port->place->last;
			splace->next = port->place;
		} else
		{
			splace->last = port->place;
			splace->next = port->place->next;
		}
	}
	if (splace->last)
	{
		splace->last->next = splace;
	} else
	{
		port->node->row->row->start = splace;
	}
	if (splace->next)
	{
		splace->next->last = splace;
	} else
	{
		port->node->row->row->end = splace;
	}
	Sc_route_resolve_new_xpos(splace, port->node->row->row);

	/* change route port to lateral feed */
	port->place = splace;
	port->port = sport;

	/* channel assignment of lateral feed */
	if (!ports_above)
	{
		/* add to channel below */
		if ((err = Sc_route_add_port_to_channel(port,
			port->node->ext_node, channels, port->node->row->number)))
				return(err);
	} else
	{
		if (ports_below)
		{
			/* add to channel where closest */
			offset = Sc_route_nearest_port(port, port->node,
				port->node->row->number, cell);
			if (offset > 0)
			{
				offset = 1;
			} else
			{
				offset = 0;
			}
			if ((err = Sc_route_add_port_to_channel(port,
				port->node->ext_node, channels,
				port->node->row->number + offset)))
					return(err);
		} else
		{
			/* add to channel above */
			if ((err = Sc_route_add_port_to_channel(port,
				port->node->ext_node, channels, port->node->row->number + 1)))
					return(err);
		}
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_print_channel
------------------------------------------------------------------------
Description:
	Print out the channel track assignments.
------------------------------------------------------------------------
Calling Sequence:  Sc_route_print_channel(channels);

Name		Type			Description
----		----			-----------
channels	*SCROUTECHANNEL	Pointer to channel list.
------------------------------------------------------------------------
*/

void Sc_route_print_channel(SCROUTECHANNEL *channels)
{
	SCROUTECHANNEL	*channel;
	SCROUTETRACK	*track;
	SCROUTETRACKMEM	*mem;
	SCROUTECHPORT	*port;

	/* list by channel and track */
	for (channel = channels; channel; channel = channel->next)
	{
		ttyputmsg(M_("********** Channel #%2d **********"), channel->number);
		for (track = channel->tracks; track; track = track->next)
		{
			ttyputmsg(M_("    For Track #%d:"), track->number);
			for (mem = track->nodes; mem; mem = mem->next)
			{
				ttyputmsg(M_("        Node #%d - (%6d, %6d)"),
					mem->node->number, mem->node->firstport->xpos,
					mem->node->lastport->xpos);
				for (port = mem->node->firstport; port; port = port->next)
				{
					if (port->port->place->cell->type == SCLEAFCELL)
					{
						ttyputmsg(M_("            Port at (%2d, %6d) -  %25s  %s"),
							port->port->node->row->number,
							port->xpos, port->port->place->cell->name,
							Sc_leaf_port_name(port->port->port->port));
					}
					else if (port->port->place->cell->type == SCFEEDCELL)
					{
						ttyputmsg(M_("            Port at (%2d, %6d) -  %25s  %d"),
							port->port->node->row->number,
							port->xpos, port->port->place->cell->name,
							port->port->port->port);
					}
				}
			}
		}
	}
}

/***********************************************************************
Module:  Sc_route_create_pass_throughs
------------------------------------------------------------------------
Description:
	Create pass throughs required to join electrically equivalent nodes
	in different channels.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_create_pass_throughs(channels, rows);

Name		Type			Description
----		----			-----------
channels	*SCROUTECHANNEL	Pointer to current channels.
rows		*SCROWLIST		Pointer to placed rows.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_create_pass_throughs(SCROUTECHANNEL *channels, SCROWLIST *rows)
{
	SCROUTECHANNEL	*chan;
	SCROUTECHNODE	*chnode, *chnode2, *old_chnode;
	int			err;

	sc_feednumber = 0;

	/* clear the flag on all channel nodes */
	for (chan = channels; chan; chan = chan->next)
	{
		for (chnode = chan->nodes; chnode; chnode = chnode->next)
			chnode->flags &= ~SCROUTESEEN;
	}

	/* find all nodes which exist in more than one channel */
	for (chan = channels; chan; chan = chan->next)
	{
		for (chnode = chan->nodes; chnode; chnode = chnode->next)
		{
			if (chnode->flags & SCROUTESEEN)
				continue;
			chnode->flags |= SCROUTESEEN;
			old_chnode = chnode;
			for (chnode2 = chnode->same_next; chnode2;
				chnode2 = chnode2->same_next)
			{
				chnode2->flags |= SCROUTESEEN;
				if ((err = Sc_route_between_ch_nodes(old_chnode, chnode2,
					channels, rows)))
						return(err);
				old_chnode = chnode2;
			}
		}
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_between_ch_nodes
------------------------------------------------------------------------
Description:
	Route between two channel nodes.  Consider both the use of pass
	throughs and the use of nodes of the same extracted node in a
	row.  Note that there may be more than one row between the two
	channels.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_between_ch_nodes(node1, node2, channels,
						 rows);

Name		Type			Description
----		----			-----------
node1		*SCROUTECHNODE	First node (below).
node2		*SCROUTECHNODE	Second node (above).
channels	*SCROUTECHANNEL	List of channels.
rows		*SCROWLIST		List of placed rows.
------------------------------------------------------------------------
*/

int Sc_route_between_ch_nodes(SCROUTECHNODE *node1, SCROUTECHNODE *node2,
	SCROUTECHANNEL *channels, SCROWLIST *rows)
{
	int			minx1, maxx1, minx2, maxx2;
	int			pminx, pmaxx, pos, err, direct;
	SCEXTNODE		*ext_node;
	SCROWLIST		*row;
	SCROUTECHANNEL	*chan;
	SCROUTENODE		*rnode;
	SCROUTEPORT		*rport;
	SCROUTECHNODE	*node;
	SCROUTECHPORT	*chport;
	SCNBPLACE		*place, *bestplace;
	int			bestpos;

	ext_node = node1->ext_node;

	/* determine limits of second channel */
	minx2 = Sc_route_min_port_pos(node2);
	maxx2 = Sc_route_max_port_pos(node2);

	/* do for all intervening channels */
	for (chan = node1->channel; chan != node2->channel; chan = chan->next,
		node1 = node1->same_next)
	{
		/* determine limits of first channel node */
		minx1 = Sc_route_min_port_pos(node1);
		maxx1 = Sc_route_max_port_pos(node1);

		/* determine preferred region of pass through */
		if (maxx1 <= minx2)
		{
			/* no overlap with first node to left */
			pminx = maxx1;
			pmaxx = minx2;
		} else if (maxx2 <= minx1)
		{
			/* no overlap with first node to right */
			pminx = maxx2;
			pmaxx = minx1;
		} else
		{
			/* have some overlap */
			pminx = maxi(minx1, minx2);
			pmaxx = mini(minx1, minx2);
		}

		/* set window fuzzy limits */
		pminx -= sc_routecontrol.fuzzy_window_limit;
		pmaxx += sc_routecontrol.fuzzy_window_limit;

		/* determine which row we are in */
		for (row = rows; row; row = row->next)
		{
			if (row->row_num == chan->number)
				break;
		}

		/* check for any possible ports which can be used */
		for (rnode = (SCROUTENODE *)ext_node->ptr; rnode;
			rnode = rnode->same_next)
		{
			if (rnode->row->number == row->row_num)
				break;
		}
		if (rnode)
		{
			/* port of correct type exists somewhere in this row */
			for (rport = rnode->firstport; rport; rport = rport->next)
			{
				pos = Sc_route_port_position(rport);
				direct = Sc_leaf_port_direction(rport->port->port);
				if (!(direct & SCPORTDIRUP && direct & SCPORTDIRDOWN))
					continue;
				if (pos >= pminx && pos <= pmaxx)
					break;
			}
			if (rport)
			{
				/* found suitable port, ensure it exists in both channels */
				chport = NULL;
				for (node = chan->nodes; node; node = node->next)
				{
					if (node->ext_node == node1->ext_node)
					{
						for (chport = node->firstport; chport;
							chport = chport->next)
						{
							if (chport->port == rport)
								break;
						}
					}
				}
				if (!chport)
				{
					/* add port to this channel */
					if ((err = Sc_route_add_port_to_channel(rport,
						ext_node, channels, chan->number)))
							return(err);
				}
				chport = NULL;
				for (node = chan->next->nodes; node; node = node->next)
				{
					if (node->ext_node == node1->ext_node)
					{
						for (chport = node->firstport; chport;
							chport = chport->next)
						{
							if (chport->port == rport)
								break;
						}
					}
				}
				if (!chport)
				{
					/* add port to next channel */
					if ((err = Sc_route_add_port_to_channel(rport,
						ext_node, channels, chan->next->number)))
							return(err);
				}
				continue;
			}
		}

		/* if no port found, find best position for feed through */
		bestpos = MAXINTBIG;
		bestplace = NULL;
		for (place = row->start; place; place = place->next)
		{
			/* not allowed to feed at stitch */
			if (place->cell->type == SCSTITCH ||
				(place->last && place->last->cell->type == SCSTITCH))
					continue;

			/* not allowed to feed at lateral feed */
			if (place->cell->type == SCLATERALFEED ||
				(place->last && place->last->cell->type == SCLATERALFEED))
					continue;
			if (place->xpos >= pminx && place->xpos <= pmaxx)
			{
				bestplace = place;
				break;
			}
			if (place->xpos < pminx)
			{
				pos = abs(pminx - place->xpos);
			} else
			{
				pos = abs(pmaxx - place->xpos);
			}
			if (pos < bestpos)
			{
				bestpos = pos;
				bestplace = place;
			}
		}

		/* insert feed through at the indicated place */
		if ((err = Sc_route_insert_feed_through(bestplace, row, channels,
			chan->number, node1)))
				return(err);
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_min_port_pos
------------------------------------------------------------------------
Description:
	Return the position of the port which is farthest left (minimum).
------------------------------------------------------------------------
Calling Sequence:  minx = Sc_route_min_port_pos(node);

Name		Type			Description
----		----			-----------
node		*SCROUTECHNODE	Pointer to channel node.
minx		int				Returned leftmost port position.
------------------------------------------------------------------------
*/

int Sc_route_min_port_pos(SCROUTECHNODE *node)
{
	int			minx, pos;
	SCROUTECHPORT	*chport;

	minx = MAXINTBIG;
	for (chport = node->firstport; chport; chport = chport->next)
	{
		/* determine position */
		pos = Sc_route_port_position(chport->port);

		/* check versus minimum */
		if (pos < minx)
			minx = pos;
	}

	return(minx);
}

/***********************************************************************
Module:  Sc_route_max_port_pos
------------------------------------------------------------------------
Description:
	Return the position of the port which is farthest right (maximum).
------------------------------------------------------------------------
Calling Sequence:  maxx = Sc_route_max_port_pos(node);

Name		Type			Description
----		----			-----------
node		*SCROUTECHNODE	Pointer to channel node.
maxx		int				Returned rightmost port position.
------------------------------------------------------------------------
*/

int Sc_route_max_port_pos(SCROUTECHNODE *node)
{
	int			maxx, pos;
	SCROUTECHPORT	*chport;

	maxx = -MAXINTBIG;
	for (chport = node->firstport; chport; chport = chport->next)
	{
		/* determine position */
		pos = Sc_route_port_position(chport->port);

		/* check versus maximum */
		if (pos > maxx)
			maxx = pos;
	}

	return(maxx);
}

/***********************************************************************
Module:  Sc_route_port_position
------------------------------------------------------------------------
Description:
	Return the x position of the indicated port.
------------------------------------------------------------------------
Calling Sequence:  pos = Sc_route_port_position(port);

Name		Type			Description
----		----			-----------
port		*SCROUTEPORT	Pointer to port in question.
pos			int				Returned x position.
------------------------------------------------------------------------
*/

int Sc_route_port_position(SCROUTEPORT *port)
{
	int		pos;

	pos = port->place->xpos;
	if (port->node->row->number % 2)
	{
		pos += port->place->cell->size - port->port->xpos;
	} else
	{
		pos += port->port->xpos;
	}

	return(pos);
}

/***********************************************************************
Module:  Sc_route_insert_feed_through
------------------------------------------------------------------------
Description:
	Insert a feed through in front of the indicated place.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_insert_feed_through(place, row, channels,
								chan_num, node);

Name		Type			Description
----		----			-----------
place		*SCNBPLACE		Place where to insert infront of.
row			*SCROWLIST		Row of place.
chan		*SCROUTECHANNEL	Pointer to start of channel list.
chan_num	int				Number of particular channel below.
node		*SCROUTECHNODE	Channel node within the channel.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_insert_feed_through(SCNBPLACE *place, SCROWLIST *row,
	SCROUTECHANNEL *channels, int chan_num, SCROUTECHNODE *node)
{
	SCNITREE		*inst;
	SCNBPLACE		*nplace;
	SCNIPORT		*port;
	SCROUTENODE		*rnode;
	SCROUTEPORT		*rport;
	int			err;

	/* create a special instance */
	inst = (SCNITREE *)emalloc(sizeof(SCNITREE), sc_tool->cluster);
	if (inst == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	inst->name = x_("Feed_Through");
	inst->number = 0;
	inst->type = SCFEEDCELL;
	inst->np = NULL;
	inst->size = ScGetParameter(SC_PARAM_ROUTE_FEEDTHRU_SIZE);
	inst->connect = NULL;
	inst->ports = NULL;
	inst->flags = 0;
	inst->tp = NULL;
	inst->next = NULL;
	inst->lptr = NULL;
	inst->rptr = NULL;

	/* create instance port */
	port = (SCNIPORT *)emalloc(sizeof(SCNIPORT), sc_tool->cluster);
	if (port == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	port->port = (CHAR *)sc_feednumber++;
	port->ext_node = node->ext_node;
	port->bits = 0;
	port->xpos = ScGetParameter(SC_PARAM_ROUTE_FEEDTHRU_SIZE) >> 1;
	port->next = NULL;
	inst->ports = port;

	/* create the appropriate place */
	nplace = (SCNBPLACE *)emalloc(sizeof(SCNBPLACE), sc_tool->cluster);
	if (nplace == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	nplace->cell = inst;
	nplace->last = place->last;
	nplace->next = place;
	if (nplace->last)
		nplace->last->next = nplace;
	place->last = nplace;
	if (place == row->start)
		row->start = nplace;

	Sc_route_resolve_new_xpos(nplace, row);

	/* create a route port entry for this new port */
	for (rnode = (SCROUTENODE *)node->ext_node->ptr; rnode;
		rnode = rnode->same_next)
	{
		if (rnode->row->number == row->row_num)
			break;
	}
	rport = (SCROUTEPORT *)emalloc(sizeof(SCROUTEPORT), sc_tool->cluster);
	if (rport == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	rport->place = nplace;
	rport->port = port;
	rport->node = rnode;
	rport->flags = 0;
	rport->next = NULL;
	rport->last = rnode->lastport;
	if (rnode->lastport)
	{
		rnode->lastport->next = rport;
	} else
	{
		rnode->firstport = rport;
	}
	rnode->lastport = rport;

	/* add to channels */
	if ((err = Sc_route_add_port_to_channel(rport, node->ext_node, channels,
		chan_num)))
			return(err);
	if ((err = Sc_route_add_port_to_channel(rport, node->ext_node, channels,
		chan_num + 1)))
			return(err);

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_resolve_new_xpos
------------------------------------------------------------------------
Description:
	Resolve the position of the new place and update the row.
------------------------------------------------------------------------
Calling Sequence:  Sc_route_resolve_new_xpos(place, row);

Name		Type		Description
----		----		-----------
place		*SCNBPLACE	New place.
row			*SCROWLIST	Pointer to existing row.
------------------------------------------------------------------------
*/

void Sc_route_resolve_new_xpos(SCNBPLACE *place, SCROWLIST *row)
{
	int		oldxpos, nxpos, xpos, overlap;
	SCCELLNUMS	cnums;

	if (place->last)
	{
		if (place->last->cell->type == SCLEAFCELL)
		{
			Sc_leaf_cell_get_nums(place->last->cell->np, &cnums);
			xpos = place->last->xpos + place->last->cell->size;
			if (row->row_num % 2)
			{
				/* odd row, cells are transposed */
				overlap = cnums.left_active - ScGetParameter(SC_PARAM_ROUTE_ACTIVE_DIST);
			} else
			{
				/* even row */
				overlap = cnums.right_active - ScGetParameter(SC_PARAM_ROUTE_ACTIVE_DIST);
			}
			if (overlap < 0 && place->cell->type != SCLATERALFEED)
				overlap = 0;
			xpos -= overlap;
			place->xpos = xpos;
			xpos += place->cell->size;
		} else
		{
			xpos = place->last->xpos + place->last->cell->size;
			place->xpos = xpos;
			xpos += place->cell->size;
		}
	} else
	{
		place->xpos = 0;
		xpos = place->cell->size;
	}

	if (place->next)
	{
		oldxpos = place->next->xpos;
		if (place->next->cell->type == SCLEAFCELL)
		{
			Sc_leaf_cell_get_nums(place->next->cell->np, &cnums);
			if (row->row_num % 2)
			{
				/* odd row, cells are transposed */
				overlap = cnums.right_active - ScGetParameter(SC_PARAM_ROUTE_ACTIVE_DIST);
			} else
			{
				/* even row */
				overlap = cnums.left_active - ScGetParameter(SC_PARAM_ROUTE_ACTIVE_DIST);
			}
			if (overlap < 0 && place->cell->type != SCLATERALFEED)
				overlap = 0;
			nxpos = xpos - overlap;
		} else
		{
			nxpos = xpos;
		}

		/* update rest of the row */
		for (place = place->next; place; place = place->next)
			place->xpos += nxpos - oldxpos;
		row->row_size += nxpos - oldxpos;
	}
}

/***********************************************************************
Module:  Sc_route_decide_exports
------------------------------------------------------------------------
Description:
	Decide upon the exports positions.  If port is available on
	either the top or bottom channel, no action is required.  If however
	the port is not available, add special place to the beginning or
	end of a row to allow routing to left or right edge of cell
	(whichever is shorter).
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_decide_exports(cell, exports);

Name		Type			Description
----		----			-----------
cell		*SCCELL			Pointer to cell.
exports		**SCROUTEEXPORT	Address of where to write created data.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_decide_exports(SCCELL *cell, SCROUTEEXPORT **exports)
{
	SCROUTEEXPORT	*lexport, *nexport;
	SCPORT		*port;
	SCEXTNODE		*enode;
	SCROUTECHANNEL	*chan;
	SCROUTECHNODE	*chnode, *chnode2;
	SCROUTECHPORT	*chport, *best_chport;
	int			top, bottom, left, right, err, best_dist, maxx, dist;

	lexport = NULL;

	/* check all exports */
	for (port = cell->ports; port; port = port->next)
	{
		/* get extracted node */
		enode = port->node->ports->ext_node;

		for (chan = cell->route->channels; chan; chan = chan->next)
		{
			for (chnode = chan->nodes; chnode; chnode = chnode->next)
			{
				if (chnode->ext_node == enode)
					break;
			}
			if (chnode)
				break;
		}

		/* find limits of channel node */
		bottom = top = left = right = FALSE;
		best_dist = MAXINTBIG;
		best_chport = NULL;
		for (chnode2 = chnode; chnode2; chnode2 = chnode2->same_next)
		{
			for (chport = chnode2->firstport; chport; chport = chport->next)
			{
				/* check for bottom channel */
				if (chport->node->channel->number == 0)
				{
					bottom = TRUE;
					best_chport = chport;
					break;
				}

				/* check for top channel */
				if (chport->node->channel->number ==
					cell->placement->num_rows)
				{
					top = TRUE;
					best_chport = chport;
					break;
				}

				/* check distance to left boundary */
				dist = Sc_route_port_position(chport->port);
				if (dist < best_dist)
				{
					best_dist = dist;
					left = TRUE;
					right = FALSE;
					best_chport = chport;
				}

				/* check distance to right boundary */
				maxx = chport->port->node->row->row->end->xpos +
					chport->port->node->row->row->end->cell->size;
				dist = maxx - Sc_route_port_position(chport->port);
				if (dist < best_dist)
				{
					best_dist = dist;
					right = TRUE;
					left = FALSE;
					best_chport = chport;
				}
			}
			if (chport)
				break;
		}
		if (top)
		{
			/* EMPTY */ 
		} else if (bottom)
		{
			/* EMPTY */ 
		} else if (right)
		{
			/* create special place for export at end of row */
			if ((err = Sc_route_create_special(port->node, best_chport, FALSE,
				&best_chport, cell)))
					return(err);
		} else if (left)
		{
			/* create special place for export at start of row */
			if ((err = Sc_route_create_special(port->node, best_chport, TRUE,
				&best_chport, cell)))
					return(err);
		}

		/* add port to export list */
		nexport = (SCROUTEEXPORT *)emalloc(sizeof(SCROUTEEXPORT), sc_tool->cluster);
		if (nexport == 0)
			return(Sc_seterrmsg(SC_NOMEMORY));
		nexport->xport = port;
		nexport->chport = best_chport;
		nexport->next = lexport;
		lexport = nexport;
	}

	*exports = lexport;
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_create_special
------------------------------------------------------------------------
Description:
	Create a special place on either the start or end of the row where
	the passed channel port real port resides.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_create_special(inst, chport, where,
								&nchport);

Name		Type			Description
----		----			-----------
inst		*SCNITREE		Instance for place to point to.
chport		*SCROUTECHNODE	Channel port in question.
where		int				TRUE at start, FALSE at end.
nchport		**SCROUTECHPORT	Address to write newly created channel port.
cell		*SCCELL			Parent cell.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_create_special(SCNITREE *inst, SCROUTECHPORT *chport, int where,
	SCROUTECHPORT **nchport, SCCELL *cell)
{
	SCNBPLACE		*nplace;
	SCROWLIST		*row;
	SCROUTENODE		*rnode;
	SCROUTEPORT		*rport;
	int			err, xpos;

	inst->size = ScGetParameter(SC_PARAM_ROUTE_FEEDTHRU_SIZE);
	inst->ports->xpos = ScGetParameter(SC_PARAM_ROUTE_FEEDTHRU_SIZE) >> 1;

	/* find row */
	row = chport->port->node->row->row;

	/* create appropriate place */
	nplace = (SCNBPLACE *)emalloc(sizeof(SCNBPLACE), sc_tool->cluster);
	if (nplace == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	nplace->cell = inst;
	if (where)
	{
		if (row->start)
		{
			xpos = row->start->xpos - ScGetParameter(SC_PARAM_ROUTE_FEEDTHRU_SIZE);
			nplace->xpos = xpos;
			row->start->last = nplace;
		} else
		{
			nplace->xpos = 0;
		}
		nplace->last = NULL;
		nplace->next = row->start;
		row->start = nplace;
	} else
	{
		if (row->end)
		{
			nplace->xpos = row->end->xpos + row->end->cell->size;
			row->end->next = nplace;
		} else
		{
			nplace->xpos = 0;
		}
		nplace->next = NULL;
		nplace->last = row->end;
		row->end = nplace;
	}

	/* create a route port entry for this new port */
	for (rnode = (SCROUTENODE *)chport->node->ext_node->ptr; rnode;
		rnode = rnode->same_next)
	{
		if (rnode->row->number == row->row_num)
			break;
	}
	rport = (SCROUTEPORT *)emalloc(sizeof(SCROUTEPORT), sc_tool->cluster);
	if (rport == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	rport->place = nplace;
	rport->port = inst->ports;
	rport->node = rnode;
	rport->flags = 0;
	rport->next = NULL;
	rport->last = rnode->lastport;
	if (rnode->lastport)
	{
		rnode->lastport->next = rport;
	} else
	{
		rnode->firstport = rport;
	}
	rnode->lastport = rport;

	/* add to channel */
	if ((err = Sc_route_add_port_to_channel(rport, chport->port->node->ext_node,
		cell->route->channels, chport->node->channel->number)))
			return(err);

	*nchport = chport->node->lastport;

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_tracks_in_channels
------------------------------------------------------------------------
Description:
	Route the tracks in each channel by using an improved channel
	router.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_tracks_in_channels(channels, cell);

Name		Type			Description
----		----			-----------
channels	*SCROUTECHANNEL	List of all channels.
cell		*SCCELL			Pointer to parent cell.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_tracks_in_channels(SCROUTECHANNEL *channels, SCCELL *cell)
{
	SCROUTECHANNEL	*chan;
	int			err;
	SCROUTEVCG		*v_graph;
	SCROUTEZRG		*zr_graph;
	SCROUTETRACK	*tracks;

	/* do for each channel individually */
	for (chan = channels; chan; chan = chan->next)
	{
		if (Sc_stop())
			longjmp(sc_please_stop, 1);
		if (sc_routecontrol.verbose)
			ttyputmsg(M_("**** Routing tracks for Channel %d ****"), chan->number);

		/* create Vertical Constraint Graph (VCG) */
		if ((err = Sc_route_create_VCG(chan, &v_graph, cell)))
			return(err);

		/* create Zone Representation Graph (ZRG) */
		if ((err = Sc_route_create_ZRG(chan, &zr_graph)))
			return(err);

		/* do track assignment */
		if ((err = Sc_route_track_assignment(v_graph, zr_graph, chan->nodes,
			&tracks)))
				return(err);

		/* free Zone Representation Graph */
		Sc_free_zrg(zr_graph);

		chan->tracks = tracks;
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_create_VCG
------------------------------------------------------------------------
Description:
	Create the Vertical Constrain Graph (VCG) for the indicated channel.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_create_VCG(channel, v_graph, cell);

Name		Type			Description
----		----			-----------
channel		*SCROUTECHANNEL Pointer to channel.
v_graph		**SCROUTEVCG	Address of where to write created VCG.
cell		*SCCELL			Pointer to parent cell.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_create_VCG(SCROUTECHANNEL *channel, SCROUTEVCG **v_graph, SCCELL *cell)
{
	SCROUTECHNODE	*chnode;
	int			net_number, err;
	SCROUTECHPORT	*chport, *port2;
	SCROUTEVCG		*vcg_root, *vcg_node;
	SCROUTEVCGEDGE	*vcg_edge, *edge1;

	/* first number channel nodes to represent nets */
	net_number = 0;
	for (chnode = channel->nodes; chnode; chnode = chnode->next)
	{
		chnode->number = net_number++;

		/* calculate actual port position */
		for (chport = chnode->firstport; chport; chport = chport->next)
			chport->xpos = Sc_route_port_position(chport->port);

		/* sort all channel ports on node from leftmost to rightmost */
		for (chport = chnode->firstport; chport; chport = chport->next)
		{
			/* bubble port left if necessay */
			for (port2 = chport->last; port2; port2 = chport->last)
			{
				if (port2->xpos <= chport->xpos)
					break;

				/* move chport left */
				chport->last = port2->last;
				port2->last = chport;
				if (chport->last)
					chport->last->next = chport;
				port2->next = chport->next;
				chport->next = port2;
				if (port2->next)
					port2->next->last = port2;
				if (port2 == chnode->firstport)
					chnode->firstport = chport;
				if (chport == chnode->lastport)
					chnode->lastport = port2;
			}
		}
	}

	/* create the VCG root node */
	vcg_root = (SCROUTEVCG *)emalloc(sizeof(SCROUTEVCG), sc_tool->cluster);
	if (vcg_root == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	vcg_root->chnode = NULL;
	vcg_root->edges = NULL;

	/* create a VCG node for each channel node (or net) */
	for (chnode = channel->nodes; chnode; chnode = chnode->next)
	{
		vcg_node = (SCROUTEVCG *)emalloc(sizeof(SCROUTEVCG), sc_tool->cluster);
		if (vcg_node == 0)
			return(Sc_seterrmsg(SC_NOMEMORY));
		vcg_node->chnode = chnode;
		vcg_node->edges = NULL;
		vcg_edge = (SCROUTEVCGEDGE *)emalloc(sizeof(SCROUTEVCGEDGE), sc_tool->cluster);
		if (vcg_edge == 0)
			return(Sc_seterrmsg(SC_NOMEMORY));
		vcg_edge->node = vcg_node;
		vcg_edge->next = vcg_root->edges;
		vcg_root->edges = vcg_edge;
	}

	if ((err = Sc_route_VCG_create_dependents(vcg_root, channel)))
		return(err);

	/* add any ports in this channel tied to power */
	if ((err = Sc_route_create_power_ties(channel, vcg_root, cell)))
		return(err);

	/* add any ports in this channel tied to ground */
	if ((err = Sc_route_create_ground_ties(channel, vcg_root, cell)))
		return(err);

	/* remove all dependent nodes from root of constraint graph*/
	/* clear seen flag */
	for (vcg_edge = vcg_root->edges; vcg_edge; vcg_edge = vcg_edge->next)
		vcg_edge->node->flags &= ~SCROUTESEEN;

	/* mark all VCG nodes that are called by others */
	for (vcg_edge = vcg_root->edges; vcg_edge; vcg_edge = vcg_edge->next)
	{
		for (edge1 = vcg_edge->node->edges; edge1; edge1 = edge1->next)
			edge1->node->flags |= SCROUTESEEN;
	}

	/* remove all edges from root which are marked */
	edge1 = vcg_root->edges;
	for (vcg_edge = vcg_root->edges; vcg_edge; vcg_edge = vcg_edge->next)
	{
		if (vcg_edge->node->flags & SCROUTESEEN)
		{
			if (vcg_edge == vcg_root->edges)
			{
				vcg_root->edges = vcg_edge->next;
				edge1 = vcg_edge->next;
			} else
			{
				edge1->next = vcg_edge->next;
			}
		} else
		{
			edge1 = vcg_edge;
		}
	}

	/* print out Vertical Constraint Graph if verbose flag set */
	if (sc_routecontrol.verbose)
	{
		ttyputmsg(M_("************ VERTICAL CONSTRAINT GRAPH"));
		for (edge1 = vcg_root->edges; edge1; edge1 = edge1->next)
		{
			ttyputmsg(M_("Net %d:"), edge1->node->chnode->number);
			Sc_route_print_VCG(edge1->node->edges, 1);
		}
	}

	*v_graph = vcg_root;

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_VCG_create_dependents
------------------------------------------------------------------------
Description:
	Resolve any cyclic dependencies in the Vertical Constraint Graph.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_VCG_create_dependents(vcg_root,
								channel);

Name		Type			Description
----		----			-----------
vcg_root	*SCROUTEVCG		Pointer to root of VCG.
channel		*SCROUTECHANNEL	Pointer to particular channel.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_VCG_create_dependents(SCROUTEVCG *vcg_root, SCROUTECHANNEL *channel)
{
	int			check, err, diff;
	SCNBPLACE		*place, *place2;
	SCROUTECHNODE	*chnode;
	SCROUTECHPORT	*chport, *port2;

	check = TRUE;
	while (check)
	{
		check = FALSE;
		if ((err = Sc_route_VCG_set_dependents(vcg_root)))
			return(err);
		if (Sc_route_VCG_cyclic_check(vcg_root, &place, &diff))
		{
			check = TRUE;

			/* move place and update row */
			for (place2 = place; place2; place2 = place2->next)
				place2->xpos += diff;

			/* update channel port positions */
			for (chnode = channel->nodes; chnode; chnode = chnode->next)
			{
				/* calculate actual port position */
				for (chport = chnode->firstport; chport;chport = chport->next)
					chport->xpos = Sc_route_port_position(chport->port);

				/* reorder port positions from left to right */
				for (chport = chnode->firstport; chport;chport = chport->next)
				{
					for (port2 = chport->last; port2; port2 = chport->last)
					{
						if (port2->xpos <= chport->xpos)
							break;

						/* move chport left */
						chport->last = port2->last;
						port2->last = chport;
						if (chport->last)
							chport->last->next = chport;
						port2->next = chport->next;
						chport->next = port2;
						if (port2->next)
							port2->next->last = port2;
						if (port2 == chnode->firstport)
							chnode->firstport = chport;
						if (chport == chnode->lastport)
							chnode->lastport = port2;
					}
				}
			}
		}
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_VCG_set_dependents
------------------------------------------------------------------------
Description:
	Create a directed edge if one channel node must be routed before
	another.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_VCG_set_dependents(vcg_root);

Name		Type		Description
----		----		-----------
vcg_root	*SCROUTEVCG	Root of Vertical Constraint Graph.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_VCG_set_dependents(SCROUTEVCG *vcg_root)
{
	SCROUTEVCGEDGE	*edge1, *edge2, *vcg_edge;
	int			depend1, depend2;
	SCROUTECHPORT	*port1, *port2;

	/* clear all dependencies */
	for (edge1 = vcg_root->edges; edge1; edge1 = edge1->next)
		edge1->node->edges = NULL;

	/* set all dependencies */
	for (edge1 = vcg_root->edges; edge1; edge1 = edge1->next)
	{
		for (edge2 = edge1->next; edge2; edge2 = edge2->next)
		{
			/* Given two channel nodes, create a directed edge if */
			/* one must be routed before the other */
			depend1 = depend2 = FALSE;
			for (port1 = edge1->node->chnode->firstport; port1;
				port1 = port1->next)
			{
				for (port2 = edge2->node->chnode->firstport; port2;
					port2 = port2->next)
				{
					if (abs(port1->xpos - port2->xpos) <
						ScGetParameter(SC_PARAM_ROUTE_PORT_X_MIN_DIST))
					{
						/* determine which one goes first */
						if (port1->port->node->row->number >
							port2->port->node->row->number)
						{
							depend1 = TRUE;
						} else
						{
							depend2 = TRUE;
						}
					}
				}
			}
			if (depend1)
			{
				vcg_edge = (SCROUTEVCGEDGE *)emalloc(sizeof(SCROUTEVCGEDGE), sc_tool->cluster);
				if (vcg_edge  == 0)
					return(Sc_seterrmsg(SC_NOMEMORY));
				vcg_edge->node = edge2->node;
				vcg_edge->next = edge1->node->edges;
				edge1->node->edges = vcg_edge;
			}
			if (depend2)
			{
				vcg_edge = (SCROUTEVCGEDGE *)emalloc(sizeof(SCROUTEVCGEDGE), sc_tool->cluster);
				if (vcg_edge == 0)
					return(Sc_seterrmsg(SC_NOMEMORY));
				vcg_edge->node = edge1->node;
				vcg_edge->next = edge2->node->edges;
				edge2->node->edges = vcg_edge;
			}
		}
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_VCG_cyclic_check
------------------------------------------------------------------------
Description:
	Return TRUE if cyclic dependency is found in Vertical Constraint
	Graph.  Also set place and offset needed to resolve this conflict.
	Note that only the top row may be moved around as the bottom row
	may have already been used by another channel.
------------------------------------------------------------------------
Calling Sequence:  check = Sc_route_VCG_cyclic_check(vcg_root, place,
								diff);

Name		Type		Description
----		----		-----------
vcg_root	*SCROUTEVCG	Root of Vertical Constraint Graph.
place		**SCNBPLACE	Address to write pointer to place.
diff		*int		Address to write offset required.
check		int			Returned flag, TRUE = cyclic found.
------------------------------------------------------------------------
*/

int Sc_route_VCG_cyclic_check(SCROUTEVCG *vcg_root, SCNBPLACE **place, int *diff)
{
	SCROUTEVCGEDGE	*edge, *edge2, *edge3;
	SCROUTEVCG		*last_node;
	SCROUTECHPORT	*port1, *port2;

	/* check each VCG node */
	for (edge = vcg_root->edges; edge; edge = edge->next)
	{
		/* clear all flags */
		for (edge3 = vcg_root->edges; edge3; edge3 = edge3->next)
		{
			edge3->node->flags &= ~(SCROUTESEEN | SCROUTETEMPNUSE);
		}

		/* mark this node */
		edge->node->flags |= SCROUTESEEN;

		/* check single cycle */
		for (edge2 = edge->node->edges; edge2; edge2 = edge2->next)
		{
			last_node = edge->node;
			if (Sc_route_VCG_single_cycle(edge2->node, &last_node))
			{
				/* find place of conflict */
				for (port1 = edge->node->chnode->firstport; port1; port1 = port1->next)
				{
					for (port2 = last_node->chnode->firstport; port2; port2 = port2->next)
					{
						if (abs(port1->xpos - port2->xpos) <
							ScGetParameter(SC_PARAM_ROUTE_PORT_X_MIN_DIST))
						{
							/* determine which one goes first */
							if (port1->port->node->row->number >
								port2->port->node->row->number)
							{
								*place = port1->port->place;
								if (port1->xpos < port2->xpos)
								{
									*diff = (port2->xpos - port1->xpos) +
										ScGetParameter(SC_PARAM_ROUTE_PORT_X_MIN_DIST);
								} else
								{
									*diff = ScGetParameter(SC_PARAM_ROUTE_PORT_X_MIN_DIST) -
										(port1->xpos - port2->xpos);
								}
							} else if (port2->port->node->row->number >
								port1->port->node->row->number)
							{
								*place = port2->port->place;
								if (port2->xpos < port1->xpos)
								{
									*diff = (port1->xpos - port2->xpos) +
										ScGetParameter(SC_PARAM_ROUTE_PORT_X_MIN_DIST);
								} else
								{
									*diff = ScGetParameter(SC_PARAM_ROUTE_PORT_X_MIN_DIST) -
										(port2->xpos - port1->xpos);
								}
							} else
							{
								ttyputmsg(_("SEVERE ERROR - Cyclic conflict to same row, check leaf cells."));
								ttyputmsg(_("At %s %s to %s %s."),
									port1->port->place->cell->name,
									Sc_leaf_port_name(port1->port->port->port),
									port2->port->place->cell->name,
									Sc_leaf_port_name(port2->port->port->port));
								longjmp(sc_please_stop, 1);
							}
							return(TRUE);
						}
					}
				}
				ttyputmsg(_("SEVERE WARNING - Cyclic conflict discovered but cannot find place to resolve."));
			}
		}
	}

	return(FALSE);
}

/***********************************************************************
Module:  Sc_route_VCG_single_cycle
------------------------------------------------------------------------
Description:
	Return TRUE if a recursive Breadth First Search encounters the
	marked node.
------------------------------------------------------------------------
Calling Sequence:  found = Sc_route_VCG_single_cycle(node, last_node);

Name		Type			Description
----		----			-----------
node		*SCROUTEVCG		Node to start search.
last_node	**SCROUTEVCG	Address to save last node searched.
found		int				Returned TRUE if marked node found.
------------------------------------------------------------------------
*/

int Sc_route_VCG_single_cycle(SCROUTEVCG *node, SCROUTEVCG **last_node)
{
	SCROUTEVCGEDGE	*edge;
	SCROUTEVCG		*save_node;

	if (node == NULL)
		return(FALSE);
	if (node->flags & SCROUTESEEN)
	{
		/* marked node found */
		return(TRUE);
	}
	if (node->flags & SCROUTETEMPNUSE)
	{
		/* been here before */
		return(FALSE);
	} else
	{
		/* check others */
		node->flags |= SCROUTETEMPNUSE;
		save_node = *last_node;
		for (edge = node->edges; edge; edge = edge->next)
		{
			*last_node = node;
			if (Sc_route_VCG_single_cycle(edge->node, last_node))
				return(TRUE);
		}
		*last_node = save_node;
		return(FALSE);
	}
}

/***********************************************************************
Module:  Sc_route_print_VCG
------------------------------------------------------------------------
Description:
	Recursively print out the VCG for the indicated edge list.
------------------------------------------------------------------------
Calling Sequence:  Sc_route_print_VCG(edges, level);

Name		Type			Description
----		----			-----------
edges		*SCROUTEVCGEDGE	List of VCG edges.
level		int				Level of indentation.
------------------------------------------------------------------------
*/

void Sc_route_print_VCG(SCROUTEVCGEDGE *edges, int level)
{
	CHAR	spacer[MAXLINE];
	int		i, j;

	if (edges == NULL) return;

	i = level << 2;
	for (j = 0; j < i; j++)
		spacer[j] = ' ';
	spacer[i] = 0;

	for (; edges; edges = edges->next)
	{
		ttyputmsg(_("%sbefore Net %d"), spacer, edges->node->chnode->number);
		Sc_route_print_VCG(edges->node->edges, level + 1);
	}
}

/***********************************************************************
Module:  Sc_route_create_ZRG
------------------------------------------------------------------------
Description:
	Create the Zone Representation Graph (ZRG) for the indicated
	channel.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_create_ZRG(channel, zr_graph);

Name		Type			Description
----		----			-----------
channel		*SCROUTECHANNEL	Pointer to channel.
zr_graph	**SCROUTEZRG	Address of where to write created ZRG.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_create_ZRG(SCROUTECHANNEL *channel, SCROUTEZRG **zr_graph)
{
	int			err, z_number, num_chnodes;
	SCROUTEZRG		*first_zone, *last_zone, *zone;
	SCROUTEZRGMEM	*mem;
	SCROUTECHNODE	*left_chnode, **chnode_list, *chnode;

	first_zone = last_zone = NULL;
	z_number = 0;

	/* create first zone */
	zone = (SCROUTEZRG *)emalloc(sizeof(SCROUTEZRG), sc_tool->cluster);
	if (zone == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	zone->number = z_number++;
	zone->chnodes = NULL;
	zone->next = NULL;
	zone->last = NULL;
	first_zone = last_zone = zone;

	/* clear flag on all channel nodes */
	num_chnodes = 0;
	for (chnode = channel->nodes; chnode; chnode = chnode->next)
	{
		chnode->flags &= ~SCROUTESEEN;
		num_chnodes++;
	}

	/* allocate enough space for channel node temporary list */
	chnode_list = (SCROUTECHNODE **)emalloc(sizeof(SCROUTECHNODE *)
		* (num_chnodes + 1), sc_tool->cluster);
	if (chnode_list == 0)
		return(	Sc_seterrmsg(SC_NOMEMORY));

	while ((left_chnode = Sc_route_find_leftmost_chnode(channel->nodes)))
	{
		Sc_route_create_zrg_temp_list(channel->nodes,
			left_chnode->firstport->xpos, chnode_list);
		if (Sc_route_zrg_list_compatible(chnode_list, zone))
		{
			if ((err = Sc_route_zrg_add_chnodes(chnode_list, zone)))
				return(err);
		} else
		{
			zone = (SCROUTEZRG *)emalloc(sizeof(SCROUTEZRG), sc_tool->cluster);
			if (zone == 0)
				return(Sc_seterrmsg(SC_NOMEMORY));
			zone->number = z_number++;
			zone->chnodes = NULL;
			zone->next = NULL;
			zone->last = last_zone;
			last_zone->next = zone;
			last_zone = zone;
			if ((err = Sc_route_zrg_add_chnodes(chnode_list, zone)))
				return(err);
		}
		left_chnode->flags |= SCROUTESEEN;
	}

	/* print out zone representation if verbose flag set */
	if (sc_routecontrol.verbose)
	{
		ttyputmsg(M_("************ ZONE REPRESENTATION GRAPH"));
		for (zone = first_zone; zone; zone = zone->next)
		{
			ttyputmsg(M_("Zone %d:"), zone->number);
			for (mem = zone->chnodes; mem; mem = mem->next)
				ttyputmsg(M_("    Node %d"), mem->chnode->number);
		}
	}

	efree((CHAR *)chnode_list);
	*zr_graph = first_zone;
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_find_leftmost_chnode
------------------------------------------------------------------------
Description:
	Return a pointer to the unmarked channel node of the indicated
	channel which has the left-most first port.  If no channel nodes
	suitable found, return NULL.
------------------------------------------------------------------------
Calling Sequence:  left_chnode = Sc_route_find_leftmost_chnode(nodes);

Name		Type			Description
----		----			-----------
nodes		*SCROUTECHNODE	Pointer to a list of channel nodes.
left_chnode	*SCROUTECHNODE	Returned pointer to leftmost node,
								NULL if none unmarked found.
------------------------------------------------------------------------
*/

SCROUTECHNODE  *Sc_route_find_leftmost_chnode(SCROUTECHNODE *nodes)
{
	SCROUTECHNODE	*left_chnode, *node;
	int			left_xpos;

	left_chnode = NULL;
	left_xpos = MAXINTBIG;

	for (node = nodes; node; node = node->next)
	{
		if (node->flags & SCROUTESEEN)
			continue;
		if (node->firstport->xpos < left_xpos)
		{
			left_xpos = node->firstport->xpos;
			left_chnode = node;
		}
	}

	return(left_chnode);
}

/***********************************************************************
Module:  Sc_route_create_zrg_temp_list
------------------------------------------------------------------------
Description:
	Fill in the temporary list of all channel nodes which encompass the
	indicated x position.
------------------------------------------------------------------------
Calling Sequence:  Sc_route_create_zrg_temp_list(nodes, xpos, list);

Name		Type				Description
----		----				-----------
nodes		*SCROUTECHNODE		List of channel nodes.
xpos		int					X position of interest.
list		*SCROUTECHNODE[]	Array of pointer to fill in, terminate
								  with a NULL.
------------------------------------------------------------------------
*/

void Sc_route_create_zrg_temp_list(SCROUTECHNODE *nodes, int xpos, SCROUTECHNODE *list[])
{
	int			i;
	SCROUTECHNODE	*node;

	i = 0;
	for (node = nodes; node; node = node->next)
	{
		if (xpos > node->firstport->xpos - ScGetParameter(SC_PARAM_ROUTE_PORT_X_MIN_DIST)
			&& xpos < node->lastport->xpos+ScGetParameter(SC_PARAM_ROUTE_PORT_X_MIN_DIST))
				list[i++] = node;
	}
	list[i] = NULL;
}

/***********************************************************************
Module:  Sc_route_zrg_list_compatible
------------------------------------------------------------------------
Description:
	Return a TRUE if the indicated list of channel nodes is compatible
	with the indicated zone, else return FALSE.
------------------------------------------------------------------------
Calling Sequence:  compat = Sc_route_zrg_list_compatible(list, zone);

Name		Type				Description
----		----				-----------
list		*SCROUTECHNODE[]	Array of pointers to channel nodes.
zone		*SCROUTEZRG			Pointer to current zone.
compat		int					Returned flag, TRUE if compatible.
------------------------------------------------------------------------
*/

int Sc_route_zrg_list_compatible(SCROUTECHNODE *list[], SCROUTEZRG *zone)
{
	SCROUTEZRGMEM	*mem;
	int			i;

	if (zone->chnodes)
	{
		/* check each member of current zone being in the list */
		for (mem = zone->chnodes; mem; mem = mem->next)
		{
			for (i = 0; list[i]; i++)
			{
				if (mem->chnode == list[i])
					break;
			}
			if (list[i] == NULL)
				return(FALSE);
		}
		return(TRUE);
	} else
	{
		/* no current channel nodes, so compatible */
		return(TRUE);
	}
}

/***********************************************************************
Module:  Sc_route_zrg_add_chnodes
------------------------------------------------------------------------
Description:
	Add the channel nodes in the list to the indicated zone if they
	are not already in the zone.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_zrg_add_chnodes(list, zone);

Name		Type				Description
----		----				-----------
list		*SCROUTECHNODE[]	List of channel nodes.
zone		*SCROUTEZRG			Pointer to current zone.
err			int					Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_zrg_add_chnodes(SCROUTECHNODE *list[], SCROUTEZRG *zone)
{
	int			i;
	SCROUTEZRGMEM	*mem;

	for (i = 0; list[i]; i++)
	{
		for (mem = zone->chnodes; mem; mem = mem->next)
		{
			if (mem->chnode == list[i])
				break;
		}
		if (mem == NULL)
		{
			mem = (SCROUTEZRGMEM *)emalloc(sizeof(SCROUTEZRGMEM), sc_tool->cluster);
			if (mem == 0)
				return(Sc_seterrmsg(SC_NOMEMORY));
			mem->chnode = list[i];
			mem->next = zone->chnodes;
			zone->chnodes = mem;
		}
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_free_zrg
------------------------------------------------------------------------
Description:
	Free the Zone Representation Graph.
------------------------------------------------------------------------
Calling Sequence:  Sc_free_zrg(zrg);

Name		Type		Description
----		----		-----------
zrg			*SCROUTEZRG	Pointer to Zone Representation Graph.
------------------------------------------------------------------------
*/

void Sc_free_zrg(SCROUTEZRG *zrg)
{
	SCROUTEZRGMEM	*mem, *nextmem;
	SCROUTEZRG		*nextzrg;

	for ( ; zrg; zrg = nextzrg)
	{
		nextzrg = zrg->next;
		for (mem = zrg->chnodes; mem; mem = nextmem)
		{
			nextmem = mem->next;
			efree((CHAR *)mem);
		}
		efree((CHAR *)zrg);
	}
}

/***********************************************************************
Module:  Sc_route_track_assignment
------------------------------------------------------------------------
Description:
	Using the Vertical Constraint Graph and the Zone Representation
	Graph, assign channel nodes to tracks.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_track_assignment(vcg, zrg, nodes,
								tracks);

Name	Type			Description
-----	----			-----------
vcg		*SCROUTEVCG		Pointer to Vertical Constraint Graph.
zrg		*SCROUTEZRG		Pointer to Zone Representation Graph.
nodes	*SCROUTECHNODE	Pointer to list of channel nodes.
tracks	**SCROUTETRACK	Address of where to write created tracks.
err		int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_track_assignment(SCROUTEVCG *vcg, SCROUTEZRG *zrg, SCROUTECHNODE *nodes,
	SCROUTETRACK **tracks)
{
	SCROUTETRACK	*first_track, *last_track, *track;
	SCROUTETRACKMEM	*mem;
	SCROUTECHNODE	*node, **n_list, *node2;
	int			track_number, err, number_nodes, i;

	first_track = last_track = NULL;
	track_number = 0;

	/* create first track */
	track = (SCROUTETRACK *)emalloc(sizeof(SCROUTETRACK), sc_tool->cluster);
	if (track == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	track->number = track_number++;
	track->nodes = NULL;
	track->last = NULL;
	track->next = NULL;
	first_track = last_track = track;

	/* clear flags on all channel nodes */
	number_nodes = 0;
	for (node = nodes; node; node = node->next)
	{
		node->flags = 0;
		number_nodes++;
	}
	n_list = (SCROUTECHNODE **)emalloc(sizeof(SCROUTECHNODE *)
		* (number_nodes+1), sc_tool->cluster);
	if (n_list == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));

	/* get channel node on longest path of VCG */
	while ((node = Sc_route_longest_VCG(vcg)))
	{
		/* clear flags of all nodes */
		for (node2 = nodes; node2; node2 = node2->next)
			node2->flags = 0;

		/* add node to track */
		if ((err = Sc_route_add_node_to_track(node, track)))
			return(err);

		/* mark all other nodes in the same zones as not usable */
		Sc_route_mark_zones(node, zrg, SCROUTEUNUSABLE);

		/* find set of remaining nodes which can be added to track */
		if ((err = Sc_route_find_best_nodes(vcg, zrg, n_list,
			number_nodes + 1)))
				return(err);

		/* add to track */
		for (i = 0; n_list[i]; i++)
		{
			if ((err = Sc_route_add_node_to_track(n_list[i], track)))
				return(err);
		}

		/* delete track entries from VCG */
		Sc_route_delete_from_VCG(track, vcg);

		/* create next track */
		track = (SCROUTETRACK *)emalloc(sizeof(SCROUTETRACK), sc_tool->cluster);
		if (track == 0)
			return(Sc_seterrmsg(SC_NOMEMORY));
		track->number = track_number++;
		track->nodes = NULL;
		track->last = last_track;
		last_track->next = track;
		last_track = track;
	}

	/* delete last track if empty */
	if (track->nodes == NULL)
	{
		if (track->last)
		{
			track->last->next = NULL;
		} else 
		{
			first_track = NULL;
		}
	}

	/* print out track assignment if verbose flag set */
	if (sc_routecontrol.verbose)
	{
		ttyputmsg(M_("************ TRACK ASSIGNMENT"));
		for (track = first_track; track; track = track->next)
		{
			ttyputmsg(M_("For Track #%d:"), track->number);
			for (mem = track->nodes; mem; mem = mem->next)
				ttyputmsg(x_("    %2d    %8d  %8d"), mem->node->number,
					mem->node->firstport->xpos, mem->node->lastport->xpos);
		}
	}

	*tracks = first_track;
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_mark_zones
------------------------------------------------------------------------
Description:
	Mark all channel nodes in the same zones as the indicated zone
	as indicated.
------------------------------------------------------------------------
Calling Sequence:  Sc_route_mark_zones(node, zrg, bits);

Name		Type			Description
----		----			-----------
node		*SCROUTECHNODE	Channel node in question.
zrg			*SCROUTEZRG		Zone representation graph.
bits		int				Bits to OR in to nodes flag field.
------------------------------------------------------------------------
*/

void Sc_route_mark_zones(SCROUTECHNODE *node, SCROUTEZRG *zrg, int bits)
{
	SCROUTEZRG		*zone;
	SCROUTEZRGMEM	*zmem;

	/* mark unusable nodes */
	for (zone = zrg; zone; zone = zone->next)
	{
		for (zmem = zone->chnodes; zmem; zmem = zmem->next)
		{
			if (zmem->chnode == node) break;
		}
		if (zmem)
		{
			for (zmem = zone->chnodes; zmem; zmem = zmem->next)
				zmem->chnode->flags |= bits;
		}
	}
}

/***********************************************************************
Module:  Sc_route_longest_VCG
------------------------------------------------------------------------
Description:
	Return a pointer to the channel node which is not dependent on
	any other nodes (i.e. top of Vertical Constraint Graph) and on
	the longest path of the VCG.  If a tie, return the first one.
	If none found, return NULL.  Remove and update VCG.
------------------------------------------------------------------------
Calling Sequence:  node = Sc_route_longest_VCG(vcg);

Name	Type			Description
----	----			-----------
vcg		*SCROUTEVCG		Pointer to Vertical Constraint Graph.
node	*SCROUTECHNODE	Returned channel node, NULL if node.
------------------------------------------------------------------------
*/

SCROUTECHNODE  *Sc_route_longest_VCG(SCROUTEVCG *vcg)
{
	SCROUTECHNODE	*node;
	SCROUTEVCGEDGE	*edge;
	int			longest_path, path;

	node = NULL;
	longest_path = 0;

	/* check for all entries at the top level */
	for (edge = vcg->edges; edge; edge = edge->next)
	{
		path = Sc_route_path_length(edge->node);
		if (path > longest_path)
		{
			longest_path = path;
			node = edge->node->chnode;
		}
	}

	return(node);
}

/***********************************************************************
Module:  Sc_route_path_length
------------------------------------------------------------------------
Description:
	Return the length of the longest path starting at the indicated
	Vertical Constraint Graph Node.
------------------------------------------------------------------------
Calling Sequence:  path = Sc_route_path_length(vcg_node);

Name		Type		Description
----		----		-----------
vcg_node	*SCROUTEVCG	Vertical Constraint Graph node.
path		int			Returned longest path length.
------------------------------------------------------------------------
*/

int Sc_route_path_length(SCROUTEVCG *vcg_node)
{
	int			path, longest;
	SCROUTEVCGEDGE	*edge;

	if (vcg_node->edges == NULL)
		return(1);

	/* check path for all edges */
	longest = 0;
	for (edge = vcg_node->edges; edge; edge = edge->next)
	{
		path = Sc_route_path_length(edge->node);
		if (path > longest)
			longest = path;
	}

	return(longest + 1);
}

/***********************************************************************
Module:  Sc_route_add_node_to_track
------------------------------------------------------------------------
Description:
	Add the indicated channel node to the track and mark as seen.
	Note add the node in left to right order.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_add_node_to_track(node, track);

Name		Type			Description
----		----			-----------
node		*SCROUTECHNODE	Pointer to channel node to add.
track		*SCROUTETRACK	Pointer to track to add to.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_add_node_to_track(SCROUTECHNODE *node, SCROUTETRACK *track)
{
	SCROUTETRACKMEM	*mem, *mem2, *oldmem;

	mem = (SCROUTETRACKMEM *)emalloc(sizeof(SCROUTETRACKMEM), sc_tool->cluster);
	if (mem == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	mem->node = node;
	mem->next = NULL;
	if (track->nodes == NULL)
	{
		track->nodes = mem;
	} else
	{
		oldmem = track->nodes;
		for (mem2 = track->nodes; mem2; mem2 = mem2->next)
		{
			if (mem->node->firstport->xpos > mem2->node->firstport->xpos)
			{
				oldmem = mem2;
			} else
			{
				break;
			}
		}
		mem->next = mem2;
		if (mem2 == track->nodes)
		{
			track->nodes = mem;
		} else
		{
			oldmem->next = mem;
		}
	}

	node->flags |= SCROUTESEEN;

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_find_best_nodes
------------------------------------------------------------------------
Description:
	Find the set of remaining nodes with no ancestors in the Vertical
	Constraint Graph which are available and are of maximum combined
	length.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_find_best_nodes(vcg, zrg, n_list, num);

Name	Type				Description
----	----				-----------
vcg		*SCROUTEVCG			Vertical Constraint Graph.
zrg		*SCROUTEZRG			Zone Representation Graph.
n_list	*SCROUTECHNODE[]	Array to write list of selected nodes.
num		int					Maximum size of n_list.
err		int					Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_find_best_nodes(SCROUTEVCG *vcg, SCROUTEZRG *zrg, SCROUTECHNODE *n_list[],
	int num)
{
	int			length, max_length, i;
	SCROUTEVCGEDGE	*edge, *edge2;
	Q_UNUSED( num );

	i = 0;
	n_list[i] = NULL;

	/* try all combinations */
	for(;;)
	{
		/* find longest, usable edge */
		edge2 = NULL;
		max_length = 0;
		for (edge = vcg->edges; edge; edge = edge->next)
		{
			if (edge->node->chnode->flags & (SCROUTESEEN | SCROUTEUNUSABLE))
				continue;
			length = edge->node->chnode->lastport->xpos -
				edge->node->chnode->firstport->xpos;
			if (length >= max_length)
			{
				max_length = length;
				edge2 = edge;
			}
		}
		if (!edge2)
		{
			break;
		} else
		{
			/* add to list */
			n_list[i++] = edge2->node->chnode;
			n_list[i] = NULL;
			Sc_route_mark_zones(edge2->node->chnode, zrg, SCROUTEUNUSABLE);
		}
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_delete_from_VCG
------------------------------------------------------------------------
Description:
	Delete all channel nodes in the track from the top level of the
	Vertical Constraint Graph and update VCG.
------------------------------------------------------------------------
Calling Sequence:  Sc_route_delete_from_VCG(track, vcg);

Name		Type			Description
----		----			-----------
track		*SCROUTETRACK	Pointer to track.
vcg			*SCROUTEVCG		Pointer to Vertical Constraint Graph.
------------------------------------------------------------------------
*/

void Sc_route_delete_from_VCG(SCROUTETRACK *track, SCROUTEVCG *vcg)
{
	SCROUTETRACKMEM	*mem;
	SCROUTEVCGEDGE	*edge, *edge2, *edge3;

	/* for all track entries in VCG */
	for (mem = track->nodes; mem; mem = mem->next)
	{
		edge2 = vcg->edges;
		for (edge = vcg->edges; edge; edge = edge->next)
		{
			if (edge->node->chnode != mem->node)
			{
				edge2 = edge;
				continue;
			}

			/* remove from top level VCG */
			if (edge == vcg->edges) vcg->edges = edge->next;
				else edge2->next = edge->next;

			/* check if its edges have nodes which should be added to VCG */
			for (edge2 = edge->node->edges; edge2; edge2 = edge2->next)
				edge2->node->flags &= ~SCROUTESEEN;

			/* mark any child edges */
			for (edge2 = edge->node->edges; edge2; edge2 = edge2->next)
				Sc_route_mark_VCG(edge2->node->edges);

			Sc_route_mark_VCG(vcg->edges);
			for (edge2 = edge->node->edges; edge2; edge2 = edge3)
			{
				edge3 = edge2->next;
				if (!(edge2->node->flags & SCROUTESEEN))
				{
					/* add to top level */
					edge2->next = vcg->edges;
					vcg->edges = edge2;
				}
				else efree((CHAR *)edge2);
			}
			efree((CHAR *)edge->node);
			efree((CHAR *)edge);
			break;
		}
	}
}

/***********************************************************************
Module:  Sc_route_mark_VCG
------------------------------------------------------------------------
Description:
	Recursively mark all nodes of Vertical Constraint Graph called
	by other nodes.
------------------------------------------------------------------------
Calling Sequence:  Sc_route_mark_VCG(edges);

Name		Type			Description
----		----			-----------
edges		*SCROUTEVCGEDGE	List of edges.
------------------------------------------------------------------------
*/

void Sc_route_mark_VCG(SCROUTEVCGEDGE *edges)
{
	if (edges == NULL) return;

	for ( ; edges; edges = edges->next)
	{
		edges->node->flags |= SCROUTESEEN;
		Sc_route_mark_VCG(edges->node->edges);
	}
}

/***********************************************************************
Module:  Sc_route_create_power_ties
------------------------------------------------------------------------
Description:
	Add data to insure that input ports of the row below tied to power
	are handled correctly.  Due to the fact that the power ports are
	assumed to be in the horizontal routing layer, these ties must
	be at the bottom of the routing channel.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_create_power_ties(chan, vcg, cell);

Name	Type			Description
----	----			-----------
chan	*SCROUTECHANNEL	Pointer to current channel.
vcg		*SCROUTEVCG		Pointer to Vertical Constrant Graph.
cell	*SCCELL			Pointer to parent cell.
err		int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_create_power_ties(SCROUTECHANNEL *chan, SCROUTEVCG *vcg, SCCELL *cell)
{
	SCROWLIST		*row;
	SCNBPLACE		*place;
	SCNIPORT		*port;
	SCROUTEROW		*rrow;
	SCROUTENODE		*rnode;
	SCROUTEPORT		*rport1, *rport2;
	SCROUTEVCG		*vnode;
	SCROUTEVCGEDGE	*edge1, *edge2, *vedge;
	SCROUTECHNODE	*chnode;
	SCROUTECHPORT	*chport1, *chport2;
	int			num, minx, maxx;

	/* check for bottom channel */
	if (chan->number == 0)
		return(SC_NOERROR);

	/* get correct row */
	row = cell->placement->rows;
	for (num = 1; num < chan->number && row != 0; num++)
		row = row->next;
	if (row == 0) return(SC_NOERROR);

	/* get correct route row */
	rrow = cell->route->rows;
	for (num = 1; num < chan->number; num++)
		rrow = rrow->next;

	/* check all places in row if Base Cell */
	for (place = row->start; place; place = place->next)
	{
		if (place->cell->type != SCLEAFCELL)
			continue;

		/* check all ports of instance for reference to power */
		for (port = place->cell->ports; port; port = port->next)
		{
			if (port->ext_node == cell->power)
			{
				/* found one */
				/* should be a power port on this instance */
				if (place->cell->power == NULL)
				{
					ttyputmsg(_("WARNING - Cannot find power on %s"),
						place->cell->name);
					continue;
				}

				/* create new route node */
				rnode = (SCROUTENODE *)emalloc(sizeof(SCROUTENODE), sc_tool->cluster);
				if (rnode == 0)
					return(Sc_seterrmsg(SC_NOMEMORY));
				rnode->ext_node = port->ext_node;
				rnode->row = rrow;
				rnode->firstport = NULL;
				rnode->lastport = NULL;
				rnode->same_next = NULL;
				rnode->same_last = NULL;
				rnode->next = rrow->nodes;
				rrow->nodes = rnode;

				/* create route ports to these ports */
				rport1 = (SCROUTEPORT *)emalloc(sizeof(SCROUTEPORT), sc_tool->cluster);
				if (rport1 == 0)
					return(Sc_seterrmsg(SC_NOMEMORY));
				rport1->place = place;
				rport1->port = port;
				rport1->node = rnode;
				rnode->firstport = rport1;
				rport1->flags = 0;
				rport1->last = NULL;
				rport1->next = NULL;
				rport2 = (SCROUTEPORT *)emalloc(sizeof(SCROUTEPORT), sc_tool->cluster);
				if (rport2 == 0)
					return(Sc_seterrmsg(SC_NOMEMORY));
				rport2->place = place;
				rport2->port = place->cell->power;
				rport2->node = rnode;
				rnode->lastport = rport2;
				rport2->flags = 0;
				rport2->last = rport1;
				rport1->next = rport2;
				rport2->next = NULL;

				/* create channel node */
				chnode = (SCROUTECHNODE *)emalloc(sizeof(SCROUTECHNODE), sc_tool->cluster);
				if (chnode == 0)
					return(Sc_seterrmsg(SC_NOMEMORY));
				chnode->ext_node = port->ext_node;
				chnode->number = 0;
				chnode->firstport = NULL;
				chnode->lastport = NULL;
				chnode->channel = chan;
				chnode->flags = 0;
				chnode->same_next = NULL;
				chnode->same_last = NULL;
				chnode->next = chan->nodes;
				chan->nodes = chnode;

				/* create channel ports */
				chport1 =(SCROUTECHPORT *)emalloc(sizeof(SCROUTECHPORT), sc_tool->cluster);
				if (chport1 == 0)
					return(Sc_seterrmsg(SC_NOMEMORY));
				chport1->port = rport1;
				chport1->node = chnode;
				chport1->xpos = Sc_route_port_position(rport1);
				chport1->flags = 0;
				chport1->last = NULL;
				chport1->next = NULL;
				chport2 = (SCROUTECHPORT *)emalloc(sizeof(SCROUTECHPORT), sc_tool->cluster);
				if (chport2 == 0)
					return(Sc_seterrmsg(SC_NOMEMORY));
				chport2->port = rport2;
				chport2->node = chnode;
				chport2->xpos = Sc_route_port_position(rport2);
				chport2->flags = 0;
				chport2->last = NULL;
				chport2->next = NULL;
				if (chport1->xpos <= chport2->xpos)
				{
					chnode->firstport = chport1;
					chnode->lastport = chport2;
					chport1->next = chport2;
					chport2->last = chport1;
				} else
				{
					chnode->firstport = chport2;
					chnode->lastport = chport1;
					chport2->next = chport1;
					chport1->last = chport2;
				}

				/* create a VCG node */
				vnode = (SCROUTEVCG *)emalloc(sizeof(SCROUTEVCG), sc_tool->cluster);
				if (vnode == 0)
					return(Sc_seterrmsg(SC_NOMEMORY));
				vnode->chnode = chnode;
				vnode->flags = 0;
				vnode->edges = NULL;

				/* create a VCG edge */
				vedge = (SCROUTEVCGEDGE *)emalloc(sizeof(SCROUTEVCGEDGE), sc_tool->cluster);
				if (vedge == 0)
					return(Sc_seterrmsg(SC_NOMEMORY));
				vedge->node = vnode;
				vedge->next = vcg->edges;
				vcg->edges = vedge;

				/* make this port dependent on any others which are */
				/* too close to the power port edge */
				for (edge1 = vedge->next; edge1; edge1 = edge1->next)
				{
					minx = edge1->node->chnode->firstport->xpos -
						ScGetParameter(SC_PARAM_ROUTE_PORT_X_MIN_DIST);
					maxx = edge1->node->chnode->lastport->xpos +
						ScGetParameter(SC_PARAM_ROUTE_PORT_X_MIN_DIST);
					if (chport2->xpos > minx && chport2->xpos < maxx)
					{
						/* create dependency */
						edge2 = (SCROUTEVCGEDGE *)emalloc(sizeof(SCROUTEVCGEDGE), sc_tool->cluster);
						if (edge2 == 0)
							return(Sc_seterrmsg(SC_NOMEMORY));
						edge2->node = vnode;
						edge2->next = edge1->node->edges;
						edge1->node->edges = edge2;
					}
				}
			}
		}
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_route_create_ground_ties
------------------------------------------------------------------------
Description:
	Add data to insure that input ports of the row below tied to ground
	are handled correctly.  Due to the fact that the ground ports are
	assumed to be in the horizontal routing layer, these ties must
	be at the top of the routing channel.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_route_create_ground_ties(chan, vcg, cell);

Name	Type			Description
----	----			-----------
chan	*SCROUTECHANNEL	Pointer to current channel.
vcg		*SCROUTEVCG		Pointer to Vertical Constrant Graph.
cell	*SCCELL			Pointer to parent cell.
err		int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_route_create_ground_ties(SCROUTECHANNEL *chan, SCROUTEVCG *vcg, SCCELL *cell)
{
	SCROWLIST		*row;
	SCNBPLACE		*place;
	SCNIPORT		*port;
	SCROUTEROW		*rrow;
	SCROUTENODE		*rnode;
	SCROUTEPORT		*rport1, *rport2;
	SCROUTEVCG		*vnode;
	SCROUTEVCGEDGE	*edge1, *edge2, *vedge;
	SCROUTECHNODE	*chnode;
	SCROUTECHPORT	*chport1, *chport2;
	int			num, minx, maxx;

	/* check for not top channel */
	if (chan->number == cell->placement->num_rows)
		return(SC_NOERROR);

	/* get correct row (above) */
	row = cell->placement->rows;
	for (num = 0; num < chan->number && row != 0; num++)
		row = row->next;
	if (row == 0) return(SC_NOERROR);

	/* get correct route row (above) */
	rrow = cell->route->rows;
	for (num = 0; num < chan->number; num++)
		rrow = rrow->next;

	/* check all places in row if Base Cell */
	for (place = row->start; place; place = place->next)
	{
		if (place->cell->type != SCLEAFCELL)
			continue;

		/* check all ports of instance for reference to ground */
		for (port = place->cell->ports; port; port = port->next)
		{
			if (port->ext_node == cell->ground)
			{
				/* found one */
				/* should be a ground port on this instance */
				if (place->cell->ground == NULL)
				{
					ttyputmsg(_("WARNING - Cannot find ground on %s"),
						place->cell->name);
					continue;
				}

				/* create new route node */
				rnode = (SCROUTENODE *)emalloc(sizeof(SCROUTENODE), sc_tool->cluster);
				if (rnode == 0)
					return(Sc_seterrmsg(SC_NOMEMORY));
				rnode->ext_node = port->ext_node;
				rnode->row = rrow;
				rnode->firstport = NULL;
				rnode->lastport = NULL;
				rnode->same_next = NULL;
				rnode->same_last = NULL;
				rnode->next = rrow->nodes;
				rrow->nodes = rnode;

				/* create route ports to these ports */
				rport1 = (SCROUTEPORT *)emalloc(sizeof(SCROUTEPORT), sc_tool->cluster);
				if (rport1 == 0)
					return(Sc_seterrmsg(SC_NOMEMORY));
				rport1->place = place;
				rport1->port = port;
				rport1->node = rnode;
				rnode->firstport = rport1;
				rport1->flags = 0;
				rport1->last = NULL;
				rport1->next = NULL;
				rport2 = (SCROUTEPORT *)emalloc(sizeof(SCROUTEPORT), sc_tool->cluster);
				if (rport2 == 0)
					return(Sc_seterrmsg(SC_NOMEMORY));
				rport2->place = place;
				rport2->port = place->cell->ground;
				rport2->node = rnode;
				rnode->lastport = rport2;
				rport2->flags = 0;
				rport2->last = rport1;
				rport1->next = rport2;
				rport2->next = NULL;

				/* create channel node */
				chnode = (SCROUTECHNODE *)emalloc(sizeof(SCROUTECHNODE), sc_tool->cluster);
				if (chnode == 0)
					return(Sc_seterrmsg(SC_NOMEMORY));
				chnode->ext_node = port->ext_node;
				chnode->number = 0;
				chnode->firstport = NULL;
				chnode->lastport = NULL;
				chnode->channel = chan;
				chnode->flags = 0;
				chnode->same_next = NULL;
				chnode->same_last = NULL;
				chnode->next = chan->nodes;
				chan->nodes = chnode;

				/* create channel ports */
				chport1 = (SCROUTECHPORT *)emalloc(sizeof(SCROUTECHPORT), sc_tool->cluster);
				if (chport1 == 0)
					return(Sc_seterrmsg(SC_NOMEMORY));
				chport1->port = rport1;
				chport1->node = chnode;
				chport1->xpos = Sc_route_port_position(rport1);
				chport1->flags = 0;
				chport1->last = NULL;
				chport1->next = NULL;
				chport2 = (SCROUTECHPORT *)emalloc(sizeof(SCROUTECHPORT), sc_tool->cluster);
				if (chport2 == 0)
					return(Sc_seterrmsg(SC_NOMEMORY));
				chport2->port = rport2;
				chport2->node = chnode;
				chport2->xpos = Sc_route_port_position(rport2);
				chport2->flags = 0;
				chport2->last = NULL;
				chport2->next = NULL;
				if (chport1->xpos <= chport2->xpos)
				{
					chnode->firstport = chport1;
					chnode->lastport = chport2;
					chport1->next = chport2;
					chport2->last = chport1;
				} else
				{
					chnode->firstport = chport2;
					chnode->lastport = chport1;
					chport2->next = chport1;
					chport1->last = chport2;
				}

				/* create a VCG node */
				vnode = (SCROUTEVCG *)emalloc(sizeof(SCROUTEVCG), sc_tool->cluster);
				if (vnode == 0)
					return(Sc_seterrmsg(SC_NOMEMORY));
				vnode->chnode = chnode;
				vnode->flags = 0;
				vnode->edges = NULL;

				/* create a VCG edge */
				vedge = (SCROUTEVCGEDGE *)emalloc(sizeof(SCROUTEVCGEDGE), sc_tool->cluster);
				if (vedge == 0)
					return(Sc_seterrmsg(SC_NOMEMORY));
				vedge->node = vnode;
				vedge->next = vcg->edges;
				vcg->edges = vedge;

				/* make all others VCG nodes which are too close to */
				/* the ground port edge dependent on this node */
				for (edge1 = vedge->next; edge1; edge1 = edge1->next)
				{
					minx = edge1->node->chnode->firstport->xpos -
						ScGetParameter(SC_PARAM_ROUTE_PORT_X_MIN_DIST);
					maxx = edge1->node->chnode->lastport->xpos +
						ScGetParameter(SC_PARAM_ROUTE_PORT_X_MIN_DIST);
					if (chport2->xpos > minx && chport2->xpos < maxx)
					{
						/* create dependency */
						edge2 = (SCROUTEVCGEDGE *)emalloc(sizeof(SCROUTEVCGEDGE), sc_tool->cluster);
						if (edge2 == 0)
							return(Sc_seterrmsg(SC_NOMEMORY));
						edge2->node = edge1->node;
						edge2->next = vnode->edges;
						vnode->edges = edge2;
					}
				}
			}
		}
	}

	return(SC_NOERROR);
}

#endif  /* SCTOOL - at top */

