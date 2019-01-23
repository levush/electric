/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: sc1connect.c
 * Modules concerned with connections for the QUISC Silicon Compiler
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

/***********************************************************************
Module:  Sc_connect
------------------------------------------------------------------------
Description:
	Connect the two indicated ports together.
------------------------------------------------------------------------
*/

int Sc_connect(int count, CHAR *pars[])
{
	SCNIPORT	*portA, *portB;
	SCNITREE	**ntpA, **ntpB;

	if (count < 3)
		return(Sc_seterrmsg(SC_NCONNECT));

	/* search for the first node */
	if (*(ntpA = Sc_findni(&(sc_curcell->niroot), pars[0])) == NULL)
		return(Sc_seterrmsg(SC_NINOFIND, pars[0]));
	if ((portA = Sc_findpp(*ntpA, pars[1])) == NULL)
		return(Sc_seterrmsg(SC_PORTNFIND, pars[1], pars[0]));

	/* search for the second node */
	if (*(ntpB = Sc_findni(&(sc_curcell->niroot), pars[2])) == NULL)
		return(Sc_seterrmsg(SC_NINOFIND, pars[2]));

	/* check for special power or ground node */
	if ((*ntpB)->type == SCSPECIALCELL)
	{
		portB = (*ntpB)->ports;
	} else
	{
		if ((portB = Sc_findpp(*ntpB, pars[3])) == NULL)
			return(Sc_seterrmsg(SC_PORTNFIND, pars[3], pars[2]));
	}
	if (Sc_conlist(*ntpA, portA, *ntpB, portB))
		return(Sc_seterrmsg(SC_XCONLIST, (*ntpA)->name, (*ntpB)->name));
	return(0);
}

/***********************************************************************
Module:  Sc_findpp
------------------------------------------------------------------------
Description:
	Find the port on the given node instance.  Return NULL if
	port is not found.
------------------------------------------------------------------------
*/

SCNIPORT  *Sc_findpp(SCNITREE *ntp, CHAR *name)
{
	SCNIPORT	*port;

	if (ntp == NULL) return((SCNIPORT *)NULL);
	switch (ntp->type)
	{
		case SCSPECIALCELL:
			port = ntp->ports;
			break;
		case SCCOMPLEXCELL:
			for (port = ntp->ports; port; port = port->next)
			{
				if (namesame( ((SCPORT *)(port->port))->name, name) == 0)
					break;
			}
			break;
		case SCLEAFCELL:
			for (port = ntp->ports; port; port = port->next)
			{
				if (namesame(Sc_leaf_port_name(port->port), name) == 0)
					break;
			}
			break;
		default:
			port = (SCNIPORT *)NULL;
			break;
	}
	return(port);
}

/***********************************************************************
Module:  Sc_conlist
------------------------------------------------------------------------
Description:
	Add a connection count for the two node instances indicated.
	Note special consideration for power and ground nodes.
------------------------------------------------------------------------
*/

int Sc_conlist(SCNITREE *ntpA, SCNIPORT *portA, SCNITREE *ntpB, SCNIPORT *portB)
{
	SCCONLIST	*cl;

	/* add connection to instance A */
	cl = (SCCONLIST *)emalloc(sizeof(SCCONLIST), sc_tool->cluster);
	if (cl == 0) return(SC_NOMEMORY);
	cl->portA = portA;
	cl->nodeB = ntpB;
	cl->portB = portB;
	cl->ext_node = NULL;

	/* add to head of the list */
	cl->next = ntpA->connect;
	ntpA->connect = cl;

	/* add connection to instance B */
	cl = (SCCONLIST *)emalloc(sizeof(SCCONLIST), sc_tool->cluster);
	if (cl == 0) return(SC_NOMEMORY);
	cl->portA = portB;
	cl->nodeB = ntpA;
	cl->portB = portA;
	cl->ext_node = NULL;

	/* add to head of the list */
	cl->next = ntpB->connect;
	ntpB->connect = cl;

	return(SC_NOERROR);
}

#endif  /* SCTOOL - at top */
