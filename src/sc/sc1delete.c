/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: sc1delete.c
 * Modules for freeing data structures allocated by the QUISC Silicon Compiler
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

#include	"global.h"
#include	"sc1.h"


/***********************************************************************
	External Variables
------------------------------------------------------------------------
*/

extern SCCELL	*sc_cells, *sc_curcell;

/* prototypes for local routines */
static int Sc_free_cell(SCCELL*);
static int Sc_free_string(CHAR*);
static int Sc_free_port(SCPORT*);
static int Sc_free_instance(SCNITREE*);
static int Sc_free_instance_port(SCNIPORT*);
static int Sc_free_extracted_node(SCEXTNODE*);
static int Sc_free_row(SCROWLIST*);
static int Sc_free_place(SCNBPLACE*);
static int Sc_free_route_channel(SCROUTECHANNEL*);
static int Sc_free_channel_node(SCROUTECHNODE*);
static int Sc_free_channel_track(SCROUTETRACK*);
static int Sc_free_route_row(SCROUTEROW*);
static int Sc_free_route_node(SCROUTENODE*);

/***********************************************************************
Module:  Sc_delete
------------------------------------------------------------------------
Description:
	Delete (free) the entire QUISC allocated database.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_delete();

Name		Type		Description
----		----		-----------
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_delete(void)
{
	SCCELL	*cel, *nextcel;
	int		err;

	/* free all cells */
	for (cel = sc_cells; cel; cel = nextcel)
	{
		nextcel = cel->next;
		err = Sc_free_cell(cel);
		if (err) return(err);
	}

	sc_cells = NULL;
	sc_curcell = NULL;
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_free_cell
------------------------------------------------------------------------
Description:
	Free the memory consumed by the indicated cell and its components.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_free_cell(cell);

Name		Type		Description
----		----		-----------
cell		*SCCELL		Pointer to cell to free.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_free_cell(SCCELL *cell)
{
	int			err;
	SCNITREE	*inst, *nextinst;
	SCSIM		*siminfo, *nextsiminfo;
	SCEXTNODE	*enode, *nextenode;
	SCPORT		*port, *nextport;

	if (cell == NULL)
		return(SC_NOERROR);

	/* free the name */
	err = Sc_free_string(cell->name);
	if (err) return(err);

	/* free all instances */
	for (inst = cell->nilist; inst; inst = nextinst)
	{
		nextinst = inst->next;
		err = Sc_free_instance(inst);
		if (err) return(err);
	}

	/* free any simulation information */
	for (siminfo = cell->siminfo; siminfo; siminfo = nextsiminfo)
	{
		nextsiminfo = siminfo->next;
		err = Sc_free_string(siminfo->model);
		/* is it a memory leak to NOT free siminfo here !!! */
		if (err) return(err);
	}

	/* free extracted nodes */
	for (enode = cell->ex_nodes; enode; enode = nextenode)
	{
		nextenode = enode->next;
		err = Sc_free_extracted_node(enode);
		if (err) return(err);
	}

	/* free power and ground */
	if ((err = Sc_free_extracted_node(cell->power)))
		return(err);
	if ((err = Sc_free_extracted_node(cell->ground)))
		return(err);

	/* free ports */
	for (port = cell->ports; port; port = nextport)
	{
		nextport = port->next;
		err = Sc_free_port(port);
		if (err) return(err);
	}

	/* free placement information */
	if ((err = Sc_free_placement(cell->placement)))
		return(err);

	/* free routing information */
	if ((err = Sc_free_route(cell->route)))
		return(err);

	efree((CHAR *)cell);

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_free_string
------------------------------------------------------------------------
Description:
	Free the memory consumed by the indicated string.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_free_string(string);

Name		Type		Description
----		----		-----------
string		*char		Pointer to string.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_free_string(CHAR *string)
{
	if (string) efree(string);
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_free_port
------------------------------------------------------------------------
Description:
	Free the memory consumed by the indicated port and its components.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_free_port(port);

Name		Type		Description
----		----		-----------
port		*SCPORT		Port to be freed.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_free_port(SCPORT *port)
{
	int		err;

	if (port)
	{
		if ((err = Sc_free_string(port->name)))
			return(err);
		efree((CHAR *)port);
	}
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_free_instance
------------------------------------------------------------------------
Description:
	Free the memory consumed by the indicated instance and its components.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_free_instance(inst);

Name		Type		Description
----		----		-----------
inst		*SCNITREE	Instance to be freed.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_free_instance(SCNITREE *inst)
{
	int			err;
	SCCONLIST	*con, *nextcon;
	SCNIPORT	*nport, *nextnport;

	if (inst)
	{
		if ((err = Sc_free_string(inst->name)))
			return(err);
		for (con = inst->connect; con; con = nextcon)
		{
			nextcon = con->next;
			efree((CHAR *)con);
		}
		for (nport = inst->ports; nport; nport = nextnport)
		{
			nextnport = nport->next;
			if ((err = Sc_free_instance_port(nport)))
				return(err);
		}
		if ((err = Sc_free_instance_port(inst->power)))
			return(err);
		if ((err = Sc_free_instance_port(inst->ground)))
			return(err);
		efree((CHAR *)inst);
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_free_instance_port
------------------------------------------------------------------------
Description:
	Free the memory consumed by the indicated instance port.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_free_instance_port(port);

Name		Type		Description
----		----		-----------
port		*SCNIPORT	Pointer to instance port.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_free_instance_port(SCNIPORT *port)
{
	if (port) efree((CHAR *)port);
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_free_extracted_node
------------------------------------------------------------------------
Description:
	Free the indicated extraced node and its contents.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_free_extracted_node(enode);

Name		Type		Description
----		----		-----------
enode		*SCEXTNODE	Pointer to extracted node.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_free_extracted_node(SCEXTNODE *enode)
{
	int			err;
	SCEXTPORT	*eport, *nexteport;

	if (enode)
	{
		if ((err = Sc_free_string(enode->name)))
			return(err);
		for (eport = enode->firstport; eport; eport = nexteport)
		{
			nexteport = eport->next;
			efree((CHAR *)eport);
		}
		efree((CHAR *)enode);
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_free_placement
------------------------------------------------------------------------
Description:
	Free the indicated placement structure and its contents.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_free_placement(placement);

Name		Type		Description
----		----		-----------
placement	*SCPLACE	Pointer to placement structure.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_free_placement(SCPLACE *placement)
{
	int			err;
	SCROWLIST	*row, *nextrow;

	if (placement)
	{
		for (row = placement->rows; row; row = nextrow)
		{
			nextrow = row->next;
			if ((err = Sc_free_row(row)))
				return(err);
		}
		efree((CHAR *)placement);
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_free_row
------------------------------------------------------------------------
Description:
	Free the indicated row structure and its contents.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_free_row(row);

Name		Type		Description
----		----		-----------
row			*SCROWLIST	Pointer to row.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_free_row(SCROWLIST *row)
{
	int			err;
	SCNBPLACE	*place, *nextplace;

	if (row)
	{
		for (place = row->start; place; place = nextplace)
		{
			nextplace = place->next;
			if ((err = Sc_free_place(place)))
				return(err);
		}
		efree((CHAR *)row);
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_free_place
------------------------------------------------------------------------
Description:
	Free the indicated place structure and its contents.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_free_place(place);

Name		Type		Description
----		----		-----------
place		*SCNBPLACE	Pointer to place.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_free_place(SCNBPLACE *place)
{
	if (place) efree((CHAR *)place);

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_free_route
------------------------------------------------------------------------
Description:
	Free the indicated route structure and its contents.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_free_route(route);

Name		Type		Description
----		----		-----------
route		*SCROUTE	Pointer to the route structure.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_free_route(SCROUTE *route)
{
	int				err;
	SCROUTECHANNEL	*chan, *nextchan;
	SCROUTEEXPORT	*xport, *nextxport;
	SCROUTEROW		*row, *nextrow;

	if (route)
	{
		for (chan = route->channels; chan; chan = nextchan)
		{
			nextchan = chan->next;
			if ((err = Sc_free_route_channel(chan)))
				return(err);
		}
		for (xport = route->exports; xport; xport = nextxport)
		{
			nextxport = xport->next;
			efree((CHAR *)xport);
		}
		for (row = route->rows; row; row = nextrow)
		{
			nextrow = row->next;
			if ((err = Sc_free_route_row(row)))
				return(err);
		}
		efree((CHAR *)route);
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_free_route_channel
------------------------------------------------------------------------
Description:
	Free the indicated route channel structure and its contents.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_free_route_channel(chan);

Name		Type			Description
----		----			-----------
chan		*SCROUTECHANNEL	Route channel structure.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_free_route_channel(SCROUTECHANNEL *chan)
{
	int				err;
	SCROUTECHNODE	*node, *nextnode;
	SCROUTETRACK	*track, *nexttrack;

	if (chan)
	{
		for (node = chan->nodes; node; node = nextnode)
		{
			nextnode = node->next;
			if ((err = Sc_free_channel_node(node)))
				return(err);
		}
		for (track = chan->tracks; track; track = nexttrack)
		{
			nexttrack = track->next;
			if ((err = Sc_free_channel_track(track)))
				return(err);
		}
		efree((CHAR *)chan);
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_free_channel_node
------------------------------------------------------------------------
Description:
	Free the indicated route channel node structure and its contents.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_free_route_channel_node(node);

Name		Type			Description
----		----			-----------
node		*SCROUTECHNODE	Pointer to route channel node.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_free_channel_node(SCROUTECHNODE *node)
{
	SCROUTECHPORT	*chport, *nextchport;

	if (node)
	{
		for (chport = node->firstport; chport; chport = nextchport)
		{
			nextchport = chport->next;
			efree((CHAR *)chport);
		}
		efree((CHAR *)node);
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_free_channel_track
------------------------------------------------------------------------
Description:
	Free the indicated route channel track structure and its contents.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_free_route_channel_track(track);

Name		Type			Description
----		----			-----------
track		*SCROUTETRACK	Pointer to route channel track.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_free_channel_track(SCROUTETRACK *track)
{
	SCROUTETRACKMEM	*mem, *nextmem;

	if (track)
	{
		for (mem = track->nodes; mem; mem = nextmem)
		{
			nextmem = mem->next;
			efree((CHAR *)mem);
		}
		efree((CHAR *)track);
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_free_route_row
------------------------------------------------------------------------
Description:
	Free the indicated route row structure and its contents.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_free_route_row(row);

Name		Type		Description
----		----		-----------
row			*SCROUTEROW	Pointer to route row.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_free_route_row(SCROUTEROW *row)
{
	int				err;
	SCROUTENODE		*node, *nextnode;

	if (row)
	{
		for (node = row->nodes; node; node = nextnode)
		{
			nextnode = node->next;
			if ((err = Sc_free_route_node(node))) return(err);
		}
		efree((CHAR *)row);
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_free_route_node
------------------------------------------------------------------------
Description:
	Free the indicated route route node structure and its contents.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_free_route_route_node(node);

Name		Type			Description
----		----			-----------
node		*SCROUTENODE	Pointer to route node.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_free_route_node(SCROUTENODE *node)
{
	SCROUTEPORT		*port, *nextport;

	if (node)
	{
		for (port = node->firstport; port; port = nextport)
		{
			nextport = port->next;
			efree((CHAR *)port);
		}
		efree((CHAR *)node);
	}

	return(SC_NOERROR);
}

#endif  /* SCTOOL - at top */
