/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: sc1interface.c
 * Modules concerned with interface ports of cells for the QUISC Silicon Compiler.
 * and with verifying the correctness of specified complex cells (connections)
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

#include "global.h"
#include "sc1.h"

extern SCCELL	*sc_curcell;
extern SCCELL	*sc_cells;

/* prototypes for local routines */
static int Sc_verify_cell(SCCELL*);
static int Sc_verify_ext_node(SCEXTNODE*, CHAR*);

/***********************************************************************
Module:  Sc_export
------------------------------------------------------------------------
Description:
	Export a port for possible future access.
------------------------------------------------------------------------
*/

int Sc_export(int count, CHAR *pars[])
{
	SCNITREE	**nptr, **searchnptr, *newnptr;
	int		type, err;
	SCPORT	*newport;
	SCNIPORT	*niport, *port;

	/* check to see if working in a cell */
	if (sc_curcell == NULL)
		return(Sc_seterrmsg(SC_NOCELL));
	if (!count)
		return(Sc_seterrmsg(SC_EXPORTNONODE));

	/* search current cell for node */
	nptr = Sc_findni(&sc_curcell->niroot, pars[0]);
	if (*nptr == NULL)
		return(Sc_seterrmsg(SC_EXPORTNODENOFIND, pars[0]));
	if (count < 2)
		return(Sc_seterrmsg(SC_EXPORTNOPORT));

	/* search for port */
	port = Sc_findpp(*nptr, pars[1]);
	if (port == (SCNIPORT *)NULL)
		return(Sc_seterrmsg(SC_EXPORTPORTNOFIND, pars[1], pars[0]));

	/* check for export name */
	if (count < 3)
		return(Sc_seterrmsg(SC_EXPORTNONAME));

	/* check possible port type */
	if (count > 3)
	{
		if (namesame(pars[3], x_("input")) == 0)
		{
			type = SCINPORT;
		} else if (namesame(pars[3], x_("output")) == 0)
		{
			type = SCOUTPORT;
		} else if (namesame(pars[3], x_("bidirectional")) == 0)
		{
			type = SCBIDIRPORT;
		} else
		{
			return(Sc_seterrmsg(SC_EXPORTXPORTTYPE, pars[3]));
		}
	} else
	{
		type = SCUNPORT;
	}

	/* create special node */
	searchnptr = Sc_findni(&sc_curcell->niroot, pars[2]);
	if (*searchnptr)
		return(Sc_seterrmsg(SC_EXPORTNAMENOTUNIQUE, pars[2]));
	if ((newnptr = Sc_new_instance(pars[2], SCSPECIALCELL)) == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	newnptr->number = sc_curcell->max_node_num++;
	*searchnptr = newnptr;
	niport = (SCNIPORT *)emalloc(sizeof(SCNIPORT), sc_tool->cluster);
	if (niport == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	niport->port = (CHAR *)NULL;
	niport->ext_node = NULL;
	niport->next = NULL;
	newnptr->ports = niport;

	/* add to export port list */
	newport = (SCPORT *)emalloc(sizeof(SCPORT), sc_tool->cluster);
	if (newport == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	niport->port = (CHAR *)newport;
	if (allocstring(&newport->name, pars[2], sc_tool->cluster))
	{
		efree((CHAR *)newport);
		return(Sc_seterrmsg(SC_NOMEMORY));
	}
	newport->node = newnptr;
	newport->parent = sc_curcell;
	newport->bits = type;
	newport->next = NULL;
	if (sc_curcell->lastport == NULL)
	{
		sc_curcell->ports = sc_curcell->lastport = newport;
	} else
	{
		sc_curcell->lastport->next = newport;
		sc_curcell->lastport = newport;
	}

	/* add to connect list */
	if ((err = Sc_conlist(*nptr, port, newnptr, niport)))
		return(err);
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_verify
------------------------------------------------------------------------
Description:
	Top module for the QUISC Verification Module.  Verification checks
	for any irregularities in created complex cells.  The possible
	errors include two output ports tied together, no output driving an
	input port, short between power and ground, etc.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_verify();

Name		Type		Description
----		----		-----------
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_verify(void)
{
	SCCELL	*cell;
	int		err;

	if (sc_cells == NULL)
		return(Sc_seterrmsg(SC_VERIFY_NO_CELLS));
	for (cell = sc_cells; cell; cell = cell->next)
	{
		if ((err = Sc_verify_cell(cell)))
			return(err);
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_verify_cell
------------------------------------------------------------------------
Description:
	Verify a single cell.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_verify_cell(cell);

Name		Type		Description
----		----		-----------
cell		*SCCELL		Pointer to cell.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_verify_cell(SCCELL *cell)
{
	SCEXTNODE	*enode;
	int		err;

	if (cell == NULL)
		return(SC_NOERROR);
	if (cell->niroot == NULL)
	{
		ttyputmsg(_("WARNING - Cell '%s' has no instance list, execute 'EXTRACT' command before verifying."),
			cell->name);
		return(SC_NOERROR);
	}
/*  for (inst = cell->niroot; inst; inst = inst->next)
	{
	} */
	for (enode = cell->ex_nodes; enode; enode = enode->next)
	{
		if ((err = Sc_verify_ext_node(enode, cell->name)))
			return(err);
	}
	if ((err = Sc_verify_ext_node(cell->power, cell->name)))
		return(err);
	if ((err = Sc_verify_ext_node(cell->ground, cell->name)))
		return(err);

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_verify_ext_node
------------------------------------------------------------------------
Description:
	Check the ports of an extracted node for irregularities.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_verify_ext_node(enode, cell_name);

Name		Type		Description
----		----		-----------
enode		*SCEXTNODE	Pointer to extracted node.
cell_name	*char		Pointer to parent cell name.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_verify_ext_node(SCEXTNODE *enode, CHAR *cell_name)
{
	int		num_output, num_bidirect;
	int		power, ground, port_type;
	SCEXTNODE	*enode2;
	SCEXTPORT	*eport;
	SCPORT	*cell_port;
	CHAR	*port_name;

	if (enode == NULL)
		return(SC_NOERROR);

	/* check for unique name */
	for (enode2 = enode->next; enode2; enode2 = enode2->next)
	{
		if (namesame(enode->name, enode2->name) == 0)
		{
			ttyputmsg(_("WARNING - node name '%s' is not unique in cell '%s'"),
				enode->name, cell_name);
		}
	}

	num_output = num_bidirect = 0;
	power = ground = 0;

	/* check for special power or ground node */
	if (namesame(enode->name, x_("power")) == 0)
		power++;
	if (namesame(enode->name, x_("ground")) == 0)
		ground++;

	for (eport = enode->firstport; eport; eport = eport->next)
	{
		port_type = 0;
		switch (eport->node->type)
		{
			case SCLEAFCELL:
				port_type = Sc_leaf_port_type(eport->port->port);
				break;
			case SCCOMPLEXCELL:
				cell_port = (SCPORT *)eport->port->port;
				port_type = cell_port->bits & SCPORTTYPE;
				break;
			case SCSPECIALCELL:
				cell_port = (SCPORT *)eport->port->port;
				port_type = cell_port->bits & SCPORTTYPE;
				switch (port_type)
				{
					case SCINPORT:
						port_type = SCOUTPORT;
						break;
					case SCOUTPORT:
						port_type = SCINPORT;
						break;
					default:
						break;
				}
				break;
			default:
				ttyputmsg(_("WARNING - instance '%s' in cell '%s' has an invalid type."),
					eport->node->name, cell_name);
				break;
		}

		switch (port_type)
		{
			case SCGNDPORT:
				ground++;
				break;
			case SCPWRPORT:
				power++;
				break;
			case SCBIDIRPORT:
				num_bidirect++;
				break;
			case SCOUTPORT:
				num_output++;
				break;
			case SCINPORT:
				break;
			default:
				break;
		}
	}

	/* check for power-ground short */
	if (power && ground)
	{
		ttyputmsg(_("ERROR - power to ground shorted on node '%s' in cell '%s'."),
			enode->name, cell_name);
	}

	/* check for only single driving source */
	if (num_output > 1)
	{
		ttyputmsg(_("WARNING - %d outputs tied together in cell '%s' on node '%s'."),
			num_output, cell_name, enode->name);
		for (eport = enode->firstport; eport; eport = eport->next)
		{
			port_type = 0;
			switch (eport->node->type)
			{
				case SCLEAFCELL:
					port_type = Sc_leaf_port_type(eport->port->port);
					port_name = Sc_leaf_port_name(eport->port->port);
					break;
				case SCCOMPLEXCELL:
					cell_port = (SCPORT *)eport->port->port;
					port_type = cell_port->bits & SCPORTTYPE;
					port_name = cell_port->name;
					break;
				case SCSPECIALCELL:
					cell_port = (SCPORT *)eport->port->port;
					port_type = cell_port->bits & SCPORTTYPE;
					port_name = cell_port->name;
					switch (port_type)
					{
						case SCINPORT:
							port_type = SCOUTPORT;
							break;
						case SCOUTPORT:
							port_type = SCINPORT;
							break;
						default:
							break;
					}
					break;
				default:
					break;
			}
			if (port_type == SCOUTPORT)
			{
				ttyputmsg(_("    Instance '%s' port '%s'."), eport->node->name,
					port_name);
			}
		}
		if (power)
			ttyputmsg(_("    Also to POWER."));
		if (ground)
			ttyputmsg(_("    Also to GROUND."));
	} else if (num_output == 1)
	{
		if (power)
			ttyputmsg(_("WARNING - output port tied to power in cell '%s' on node '%s'."),
				cell_name, enode->name);
		if (ground)
			ttyputmsg(_("WARNING - output port tied to ground in cell '%s' on node '%s'."),
				cell_name, enode->name);
		if (power || ground)
		{
			for (eport = enode->firstport; eport; eport = eport->next)
			{
				port_type = 0;
				switch (eport->node->type)
				{
					case SCLEAFCELL:
						port_type = Sc_leaf_port_type(eport->port->port);
						port_name = Sc_leaf_port_name(eport->port->port);
						break;
					case SCCOMPLEXCELL:
						cell_port = (SCPORT *)eport->port->port;
						port_type = cell_port->bits & SCPORTTYPE;
						port_name = cell_port->name;
						break;
					case SCSPECIALCELL:
						cell_port = (SCPORT *)eport->port->port;
						port_type = cell_port->bits & SCPORTTYPE;
						port_name = cell_port->name;
						switch (port_type)
						{
							case SCINPORT:
								port_type = SCOUTPORT;
								break;
							case SCOUTPORT:
								port_type = SCINPORT;
								break;
							default:
								break;
						}
						break;
					default:
						break;
				}
				if (port_type == SCOUTPORT)
				{
					ttyputmsg(_("    Instance '%s' port '%s'."), eport->node->name,
						port_name);
					break;
				}
			}
		}
	}
	else if (!num_bidirect && !power && !ground)
	{
		ttyputmsg(_("WARNING - undriven input node '%s' in cell '%s'."),
			enode->name, cell_name);
		for (eport = enode->firstport; eport; eport = eport->next)
		{
			port_type = 0;
			switch (eport->node->type)
			{
				case SCLEAFCELL:
					port_type = Sc_leaf_port_type(eport->port->port);
					port_name = Sc_leaf_port_name(eport->port->port);
					break;
				case SCCOMPLEXCELL:
					cell_port = (SCPORT *)eport->port->port;
					port_type = cell_port->bits & SCPORTTYPE;
					port_name = cell_port->name;
					break;
				case SCSPECIALCELL:
					cell_port = (SCPORT *)eport->port->port;
					port_type = cell_port->bits & SCPORTTYPE;
					port_name = cell_port->name;
					switch (port_type)
					{
						case SCINPORT:
							port_type = SCOUTPORT;
							break;
						case SCOUTPORT:
							port_type = SCINPORT;
							break;
						default:
							break;
					}
					break;
				default:
					break;
			}
			if (port_type == SCINPORT)
				ttyputmsg(_("    Instance '%s' port '%s'."), eport->node->name,
					port_name);
		}
	}

	return(SC_NOERROR);
}

#endif  /* SCTOOL - at top */
