/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: sc1extract.c
 * Modules concerned with the extraction of common electrical nodes
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

/* prototypes for local routines */
static void Sc_extract_clear_flag(SCNITREE*);
static int  Sc_extract_find_nodes(SCNITREE*, SCEXTNODE**);
static int  Sc_extract_snake(SCNITREE*, SCNIPORT*, SCCONLIST*, SCEXTNODE**);
static int  Sc_extract_add_node(SCEXTNODE*, SCNITREE*, SCNIPORT*, SCEXTNODE**, SCEXTNODE**);
static void Sc_extract_find_power(SCNITREE*, SCCELL*);
static int  Sc_extract_collect_unconnected(SCNITREE*, SCCELL*);

/***********************************************************************
Module:  Sc_extract
------------------------------------------------------------------------
Description:
	Extract the node netlist for a given cell.
------------------------------------------------------------------------
*/

int Sc_extract(int count, CHAR *pars[])
{
	int		err, nodenum;
	SCEXTNODE	*nlist, *oldnlist, *ext;
	SCEXTPORT   *plist, *oldplist;
	SCPORT	*port;
	CHAR	buf[40];
	Q_UNUSED( pars );

	if (sc_curcell == NULL)
		return(Sc_seterrmsg(SC_NOCELL));
	sc_curcell->nilist = NULL;
	Sc_make_nilist(sc_curcell->niroot, sc_curcell);
	Sc_extract_clear_flag(sc_curcell->nilist);
	sc_curcell->ex_nodes = NULL;

	if ((err = Sc_extract_find_nodes(sc_curcell->nilist,
		&(sc_curcell->ex_nodes))))
			return(err);

	/* get ground nodes */
	sc_curcell->ground = (SCEXTNODE *)emalloc(sizeof(SCEXTNODE), sc_tool->cluster);
	if (sc_curcell->ground == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	if (allocstring(&sc_curcell->ground->name, x_("ground"), sc_tool->cluster))
		return(Sc_seterrmsg(SC_NOMEMORY));
	sc_curcell->ground->flags = 0;
	sc_curcell->ground->ptr = NULL;
	sc_curcell->ground->firstport = NULL;
	sc_curcell->ground->next = NULL;
	oldnlist = sc_curcell->ex_nodes;
	for (nlist = sc_curcell->ex_nodes; nlist; nlist = nlist->next)
	{
		oldplist = nlist->firstport;
		for (plist = nlist->firstport; plist; plist = plist->next)
		{
			if (plist->node->number == GND)
			{
				sc_curcell->ground->firstport = nlist->firstport;
				if (oldnlist == nlist)
				{
					sc_curcell->ex_nodes = nlist->next;
				} else
				{
					oldnlist->next = nlist->next;
				}
				efree((CHAR *)nlist);
				if (oldplist == plist)
				{
					sc_curcell->ground->firstport = plist->next;
				} else
				{
					oldplist->next = plist->next;
				}
				efree((CHAR *)plist);
				break;
			}
			oldplist = plist;
		}
		if (plist)
			break;
		oldnlist = nlist;
	}
	for (plist = sc_curcell->ground->firstport; plist; plist = plist->next)
		plist->port->ext_node = sc_curcell->ground;

	/* get power nodes */
	sc_curcell->power = (SCEXTNODE *)emalloc(sizeof(SCEXTNODE), sc_tool->cluster);
	if (sc_curcell->power == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	if (allocstring(&sc_curcell->power->name, x_("power"), sc_tool->cluster))
		return(Sc_seterrmsg(SC_NOMEMORY));
	sc_curcell->power->flags = 0;
	sc_curcell->power->ptr = NULL;
	sc_curcell->power->firstport = NULL;
	sc_curcell->power->next = NULL;
	oldnlist = sc_curcell->ex_nodes;
	for (nlist = sc_curcell->ex_nodes; nlist; nlist = nlist->next)
	{
		oldplist = nlist->firstport;
		for (plist = nlist->firstport; plist; plist = plist->next)
		{
			if (plist->node->number == PWR)
			{
				sc_curcell->power->firstport = nlist->firstport;
				if (oldnlist == nlist)
				{
					sc_curcell->ex_nodes = nlist->next;
				} else
				{
					oldnlist->next = nlist->next;
				}
				efree((CHAR *)nlist);
				if (oldplist == plist)
				{
					sc_curcell->power->firstport = plist->next;
				} else
				{
					oldplist->next = plist->next;
				}
				efree((CHAR *)plist);
				break;
			}
			oldplist = plist;
		}
		if (plist)
			break;
		oldnlist = nlist;
	}
	for (plist = sc_curcell->power->firstport; plist; plist = plist->next)
		plist->port->ext_node = sc_curcell->power;

	Sc_extract_find_power(sc_curcell->nilist, sc_curcell);

	if ((err = Sc_extract_collect_unconnected(sc_curcell->nilist, sc_curcell)))
		return(err);

	/* give the names of the cell ports to the extracted node */
	for (port = sc_curcell->ports; port; port = port->next)
	{
		switch (port->bits & SCPORTTYPE)
		{
			case SCPWRPORT:
			case SCGNDPORT:
				break;
			default:
				/* Note that special nodes only have one niport */
				if (allocstring(&(port->node->ports->ext_node->name),
					port->name, sc_tool->cluster))
						return(Sc_seterrmsg(SC_NOMEMORY));
				break;
		}
	}

	/* give arbitrary names to unnamed extracted nodes */
	nodenum = 2;
	for (ext = sc_curcell->ex_nodes; ext; ext = ext->next)
	{
		if (ext->name == (CHAR *)NULL)
		{
			(void)esnprintf(buf, 40, x_("n%d"), nodenum++);
			if (allocstring(&ext->name, buf, sc_tool->cluster))
				return(Sc_seterrmsg(SC_NOMEMORY));
		}
	}
	if (count)
		Sc_extract_print_nodes(sc_curcell);

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_extract_clear_flag
------------------------------------------------------------------------
Description:
	Clear the extract pointer on all node instance ports.
------------------------------------------------------------------------
*/

void Sc_extract_clear_flag(SCNITREE *ntp)
{
	SCNIPORT	*port;

	for ( ; ntp; ntp = ntp->next)
	{
		ntp->flags &= SCBITS_EXTRACT;
		for (port = ntp->ports; port; port = port->next)
			port->ext_node = NULL;
	}
}

/***********************************************************************
Module:  Sc_extract_find_nodes
------------------------------------------------------------------------
Description:
	Go though the INSTANCE list, finding all resultant connections.
------------------------------------------------------------------------
*/

int Sc_extract_find_nodes(SCNITREE *nitree, SCEXTNODE **simnode)
{
	int		err;
	SCCONLIST	*cl;

	for ( ; nitree; nitree = nitree->next)
	{
		/* process node */
		nitree->flags |= SCBITS_EXTRACT;
		for (cl = nitree->connect; cl; cl = cl->next)
		{
			if ((err = Sc_extract_snake(nitree, cl->portA, cl, simnode)))
				return(err);
		}
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_extract_snake
------------------------------------------------------------------------
Description:
	Snake through connection list extracting common connections.
------------------------------------------------------------------------
*/

int Sc_extract_snake(SCNITREE *nodeA, SCNIPORT *portA, SCCONLIST *cl, SCEXTNODE **simnode)
{
	int		err;
	SCEXTNODE	*common;

	for ( ; cl; cl = cl->next)
	{
		if (cl->portA != portA)
			continue;
		if (portA && portA->ext_node)
		{
			if (!(cl->portB && cl->portB->ext_node))
			{
				if ((err = Sc_extract_add_node(portA->ext_node, cl->nodeB,
					cl->portB, simnode, &common)))
						return(err);
				if (!(cl->nodeB->flags & SCBITS_EXTRACT))
				{
					cl->nodeB->flags |= SCBITS_EXTRACT;
					if ((err = Sc_extract_snake(cl->nodeB, cl->portB,
						cl->nodeB->connect, simnode)))
							return(err);
					cl->nodeB->flags ^= SCBITS_EXTRACT;
				}
			}
		} else
		{
			if (cl->portB && cl->portB->ext_node)
			{
				if ((err = Sc_extract_add_node(cl->portB->ext_node, nodeA,
					portA, simnode, &common)))
						return(err);
			} else
			{
				if ((err = Sc_extract_add_node((SCEXTNODE *)NULL, nodeA, portA,
					simnode, &common))) return(err);
				if ((err = Sc_extract_add_node(common, cl->nodeB,
					cl->portB, simnode, &common))) return(err);
				if (!(cl->nodeB->flags & SCBITS_EXTRACT))
				{
					cl->nodeB->flags |= SCBITS_EXTRACT;
					if ((err = Sc_extract_snake(cl->nodeB, cl->portB,
						cl->nodeB->connect, simnode)))
							return(err);
					cl->nodeB->flags ^= SCBITS_EXTRACT;
				}
			}
		}
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_extract_add_node
------------------------------------------------------------------------
Description:
	Add a node and port to a SCEXTNODE list.  Modify the root if
	necessary.
------------------------------------------------------------------------
*/

int Sc_extract_add_node(SCEXTNODE *simnode, SCNITREE *node, SCNIPORT *port,
	SCEXTNODE **root, SCEXTNODE **actnode)
{
	SCEXTPORT	*newport;

	newport = (SCEXTPORT *)emalloc(sizeof(SCEXTPORT), sc_tool->cluster);
	if (newport == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));

	if (simnode == (SCEXTNODE *)NULL)
	{
		simnode = (SCEXTNODE *)emalloc(sizeof(SCEXTNODE), sc_tool->cluster);
		if (simnode == 0)
			return(Sc_seterrmsg(SC_NOMEMORY));
		simnode->firstport = newport;
		simnode->flags = FALSE;
		simnode->ptr = NULL;
		simnode->name = (CHAR *)NULL;
		newport->node = node;
		newport->port = port;
		if (port)
			port->ext_node = simnode;
		newport->next = NULL;
		simnode->next = *root;
		*root = simnode;
	} else
	{
		newport->node = node;
		newport->port = port;
		if (port)
			port->ext_node = simnode;
		newport->next = simnode->firstport;
		simnode->firstport = newport;
	}
	*actnode = simnode;

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_extract_find_power
------------------------------------------------------------------------
Description:
	Find the implicit power and ground ports by doing a search
	of the instance tree and add to the appropriate port list.
	Skip over the dummy ground and power instances and special cells.
------------------------------------------------------------------------
*/

void Sc_extract_find_power(SCNITREE *ntp, SCCELL *vars)
{
	SCNIPORT	*port;
	SCEXTPORT	*plist;

	for ( ; ntp; ntp = ntp->next)
	{
		/* process node */
		if (ntp->number > PWR)
		{
			switch (ntp->type)
			{
				case SCCOMPLEXCELL:
					break;
				case SCSPECIALCELL:
					break;
				case SCLEAFCELL:
					for (port = ntp->ground; port; port = port->next)
					{
						plist = (SCEXTPORT *)emalloc(sizeof(SCEXTPORT), sc_tool->cluster);
						if (plist == 0)
						{
							(void)Sc_seterrmsg(SC_NOMEMORY);
							return;
						}
						plist->node = ntp;
						plist->port = port;
						port->ext_node = vars->ground;
						plist->next = vars->ground->firstport;
						vars->ground->firstport = plist;
					}
					for (port = ntp->power; port; port = port->next)
					{
						plist = (SCEXTPORT *)emalloc(sizeof(SCEXTPORT), sc_tool->cluster);
						if (plist == 0)
						{
							(void)Sc_seterrmsg(SC_NOMEMORY);
							return;
						}
						plist->node = ntp;
						plist->port = (SCNIPORT *)port;
						port->ext_node = vars->power;
						plist->next = vars->power->firstport;
						vars->power->firstport = plist;
					}
					break;
				default:
					break;
			}
		}
	}
}

/***********************************************************************
Module:  Sc_extract_print_nodes
------------------------------------------------------------------------
Description:
	Print the common nodes found.
------------------------------------------------------------------------
*/

void Sc_extract_print_nodes(SCCELL *vars)
{
	int		i;
	SCEXTNODE	*simnode;
	SCEXTPORT	*plist;
	CHAR	*portname;

	i = 0;
	if (vars->ground)
	{
		ttyputmsg(M_("Node %d  %s:"), i, vars->ground->name);
		for (plist = vars->ground->firstport; plist; plist = plist->next)
		{
			switch (plist->node->type)
			{
				case SCSPECIALCELL:
					portname = M_("Special");
					break;
				case SCCOMPLEXCELL:
					portname = ((SCPORT *)(plist->port->port))->name;
					break;
				case SCLEAFCELL:
					portname = Sc_leaf_port_name(plist->port->port);
					break;
				default:
					portname = M_("Unknown");
					break;
			}
			ttyputmsg(x_("    %-20s    %s"), plist->node->name, portname);
		}
	}
	i++;

	if (vars->power)
	{
		ttyputmsg(M_("Node %d  %s:"), i, vars->power->name);
		for (plist = vars->power->firstport; plist; plist = plist->next)
		{
			switch (plist->node->type)
			{
				case SCSPECIALCELL:
					portname = M_("Special");
					break;
				case SCCOMPLEXCELL:
					portname = ((SCPORT *)(plist->port->port))->name;
					break;
				case SCLEAFCELL:
					portname = Sc_leaf_port_name(plist->port->port);
					break;
				default:
					portname = M_("Unknown");
					break;
			}
			ttyputmsg(x_("    %-20s    %s"), plist->node->name, portname);
		}
	}
	i++;

	for (simnode = vars->ex_nodes; simnode; simnode = simnode->next)
	{
		ttyputmsg(M_("Node %d  %s:"), i, simnode->name);
		for (plist = simnode->firstport; plist; plist = plist->next)
		{
			switch (plist->node->type)
			{
				case SCSPECIALCELL:
					portname = M_("Special");
					break;
				case SCCOMPLEXCELL:
					portname = ((SCPORT *)(plist->port->port))->name;
					break;
				case SCLEAFCELL:
					portname = Sc_leaf_port_name(plist->port->port);
					break;
				default:
					portname = M_("Unknown");
					break;
			}
			ttyputmsg(x_("    %-20s    %s"), plist->node->name, portname);
		}
		i++;
	}
}

/***********************************************************************
Module:  Sc_extract_collect_unconnected
------------------------------------------------------------------------
Description:
	Collect the unconnected ports and create an extracted node for
	each.
------------------------------------------------------------------------
*/

int Sc_extract_collect_unconnected(SCNITREE *nptr, SCCELL *cell)
{
	SCNIPORT	*port;
	SCEXTNODE	*ext;
	SCEXTPORT	*eport;

	for ( ; nptr; nptr = nptr->next)
	{
		/* process node */
		switch (nptr->type)
		{
			case SCCOMPLEXCELL:
			case SCLEAFCELL:
				for (port = nptr->ports; port; port = port->next)
				{
					if (port->ext_node == NULL)
					{
						ext = (SCEXTNODE *)emalloc(sizeof(SCEXTNODE), sc_tool->cluster);
						if (ext == 0)
							return(Sc_seterrmsg(SC_NOMEMORY));
						ext->name = (CHAR *)NULL;
						eport = (SCEXTPORT *)emalloc(sizeof(SCEXTPORT), sc_tool->cluster);
						if (eport == 0)
						{
							efree((CHAR *)ext);
							return(Sc_seterrmsg(SC_NOMEMORY));
						}
						eport->node = nptr;
						eport->port = port;
						eport->next = NULL;
						ext->firstport = eport;
						ext->flags = 0;
						ext->ptr = NULL;
						ext->next = cell->ex_nodes;
						cell->ex_nodes = ext;
						port->ext_node = ext;
					}
				}
				break;
			default:
				break;
		}
	}

	return(SC_NOERROR);
}

#endif  /* SCTOOL - at top */
