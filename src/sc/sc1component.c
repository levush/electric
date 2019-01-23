/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: sc1component.c
 * Modules concerned this components for the QUISC Silicon Compiler
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

extern SCCELL	*sc_cells, *sc_curcell;

/* prototypes for local routines */
static SCNITREE **Sc_find_parent_pointer_addr(SCNITREE**, SCNITREE*);

/***********************************************************************
Module:  Sc_create
------------------------------------------------------------------------
Description:
	Create command parser.  Currently supported create command include:
		1.  cell
		2.  instance
------------------------------------------------------------------------
*/

int Sc_create(int count, CHAR *pars[])
{
	SCNITREE	 **ntp;
	SCCELL	 *newcell, *cell;
	SCPORT	 *port;
	SCNIPORT	 *niport, *oldniport;
	CHAR         *sptr, *noden, *nodep, *proto, *bc, *bp;
	int          l, type, size;

	if (count == 0)
		return(Sc_seterrmsg(SC_NCREATE));

	l = estrlen(sptr = pars[0]);
	l = maxi(l, 1);
	if (namesamen(sptr, x_("cell"), l) == 0)
	{
		if (--count == 0)
			return(Sc_seterrmsg(SC_NCREATECELL));

		l = estrlen(sptr = pars[1]);

		/* check if cell already exists in cell list */
		for (cell = sc_cells; cell; cell = cell->next)
		{
			if (namesame(sptr, cell->name) == 0)
				break;
		}
		if (cell)
		{
			sc_curcell = cell;
			return(Sc_seterrmsg(SC_CELLEXISTS, sptr));
		}

		/* generate warning message if a leaf cell of the same name exists */
		if (Sc_find_leaf_cell(sptr))
			ttyputmsg(_("WARNING - cell %s may be overridden by created cell."),
				sptr);

		/* create new cell */
		newcell = (SCCELL *)emalloc(sizeof(SCCELL), sc_tool->cluster);
		if (newcell  == 0)
			return(Sc_seterrmsg(SC_NOMEMORY));
		if (allocstring(&(newcell->name), sptr, sc_tool->cluster))
			return(Sc_seterrmsg(SC_NOMEMORY));
		newcell->max_node_num = 0;
		newcell->niroot = NULL;
		newcell->nilist = NULL;
		newcell->siminfo = NULL;
		newcell->ex_nodes = NULL;
		newcell->bits = 0;
		newcell->power = NULL;
		newcell->ground = NULL;
		newcell->ports = NULL;
		newcell->lastport = NULL;
		newcell->placement = NULL;
		newcell->route = NULL;
		newcell->next = sc_cells;
		sc_cells = newcell;
		sc_curcell = newcell;

		/* create dummy ground and power nodes */
		ntp = Sc_findni(&(sc_curcell->niroot), x_("ground"));
		if (*ntp) return(Sc_seterrmsg(SC_NIEXISTS, x_("ground")));
		if ((*ntp = Sc_new_instance(x_("ground"), SCSPECIALCELL)) == 0)
			return(Sc_seterrmsg(SC_NOMEMORY));
		if (Sc_new_instance_port(*ntp) == NULL)
			return(Sc_seterrmsg(SC_NOMEMORY));
		(*ntp)->number = sc_curcell->max_node_num++;
		ntp = Sc_findni(&(sc_curcell->niroot), x_("power"));
		if (*ntp) return(Sc_seterrmsg(SC_NIEXISTS, x_("power")));
		if ((*ntp = Sc_new_instance(x_("power"), SCSPECIALCELL)) == 0)
			return(Sc_seterrmsg(SC_NOMEMORY));
		if (Sc_new_instance_port(*ntp) == NULL)
			return(Sc_seterrmsg(SC_NOMEMORY));
		(*ntp)->number = sc_curcell->max_node_num++;
		return(0);
	}

	if (namesamen(sptr, x_("instance"), l) == 0)
	{
		if (--count == 0)
			return(Sc_seterrmsg(SC_CRNODENONAME));
		l = estrlen(sptr = pars[1]);
		noden = sptr;
		l = estrlen(sptr = pars[2]);
		if (--count == 0 || l == 0)
			return(Sc_seterrmsg(SC_CRNODENOPROTO));
		nodep = sptr;

		/* search for cell in cell list */
		for (cell = sc_cells; cell; cell = cell->next)
		{
			if (namesame(cell->name, nodep) == 0)
				break;
		}
		if (cell == NULL)
		{
			/* search for leaf cell in library */
			if ((bc = Sc_find_leaf_cell(nodep)) == NULL)
				return(Sc_seterrmsg(SC_CRNODEPROTONF, nodep));
			type = SCLEAFCELL;
			proto = bc;
			size = Sc_leaf_cell_xsize(bc);
		} else
		{
			type = SCCOMPLEXCELL;
			proto = (CHAR *)cell;
			size = 0;
		}

		/* check if currently working in a cell */
		if (sc_curcell == NULL)
			return(Sc_seterrmsg(SC_NOCELL));

		/* check if instance name already exits */
		ntp = Sc_findni(&(sc_curcell->niroot), noden);
		if (*ntp)
			return(Sc_seterrmsg(SC_NIEXISTS, noden));

		/* add instance name to tree */
		if ((*ntp = Sc_new_instance(noden, type)) == 0)
			return(Sc_seterrmsg(SC_NOMEMORY));
		(*ntp)->number = sc_curcell->max_node_num++;
		(*ntp)->np = proto;
		(*ntp)->size = size;

		/* create ni port list */
		if (type == SCCOMPLEXCELL)
		{
			oldniport = NULL;
			for (port = ((SCCELL *)proto)->ports; port; port = port->next)
			{
				switch (port->bits & SCPORTTYPE)
				{
					case SCGNDPORT:
						niport = (SCNIPORT *)emalloc(sizeof(SCNIPORT), sc_tool->cluster);
						if (niport == 0)
							return(Sc_seterrmsg(SC_NOMEMORY));
						niport->port = (CHAR *)port;
						niport->ext_node = NULL;
						niport->bits = 0;
						niport->xpos = 0;
						niport->next = (*ntp)->ground;
						(*ntp)->ground = niport;
						break;
					case SCPWRPORT:
						niport = (SCNIPORT *)emalloc(sizeof(SCNIPORT), sc_tool->cluster);
						if (niport == 0)
							return(Sc_seterrmsg(SC_NOMEMORY));
						niport->port = (CHAR *)port;
						niport->ext_node = NULL;
						niport->bits = 0;
						niport->xpos = 0;
						niport->next = (*ntp)->power;
						(*ntp)->power = niport;
						break;
					default:
						niport = (SCNIPORT *)emalloc(sizeof(SCNIPORT), sc_tool->cluster);
						if (niport == 0)
							return(Sc_seterrmsg(SC_NOMEMORY));
						niport->port = (CHAR *)port;
						niport->ext_node = NULL;
						niport->bits = 0;
						niport->xpos = 0;
						niport->next = NULL;
						if (oldniport == NULL)
						{
							(*ntp)->ports = niport;
						} else
						{
							oldniport->next = niport;
						}
						oldniport = niport;
						break;
				}
			}
		} else
		{
			oldniport = NULL;
			for (bp = Sc_first_leaf_port(proto); bp != NULL;
				bp = Sc_next_leaf_port(bp))
			{
				switch (Sc_leaf_port_type(bp))
				{
					case SCGNDPORT:
						niport = (SCNIPORT *)emalloc(sizeof(SCNIPORT), sc_tool->cluster);
						if (niport == 0) return(Sc_seterrmsg(SC_NOMEMORY));
						niport->port = bp;
						niport->ext_node = NULL;
						niport->bits = 0;
						niport->xpos = Sc_leaf_port_xpos(bp);
						niport->next = (*ntp)->ground;
						(*ntp)->ground = niport;
						break;
					case SCPWRPORT:
						niport = (SCNIPORT *)emalloc(sizeof(SCNIPORT), sc_tool->cluster);
						if (niport == 0) return(Sc_seterrmsg(SC_NOMEMORY));
						niport->port = bp;
						niport->ext_node = NULL;
						niport->bits = 0;
						niport->xpos = Sc_leaf_port_xpos(bp);
						niport->next = (*ntp)->power;
						(*ntp)->power = niport;
						break;
					default:
						niport = (SCNIPORT *)emalloc(sizeof(SCNIPORT), sc_tool->cluster);
						if (niport == 0) return(Sc_seterrmsg(SC_NOMEMORY));
						niport->port = bp;
						niport->ext_node = NULL;
						niport->bits = 0;
						niport->xpos = Sc_leaf_port_xpos(bp);
						niport->next = NULL;
						if (oldniport == NULL)
						{
							(*ntp)->ports = niport;
						} else
						{
							oldniport->next = niport;
						}
						oldniport = niport;
						break;
				}
			}
		}
		return(0);
	}
	return(Sc_seterrmsg(SC_XCREATE, sptr));
}

/***********************************************************************
Module:  Sc_findni
------------------------------------------------------------------------
Description:
	Returns the address of a pointer to a SCNITREE structure where the
node should be placed.  If the pointer does not have a value of
NULL, then the object already exits.
------------------------------------------------------------------------
*/
SCNITREE **Sc_findni(SCNITREE **nptr, CHAR *name)
{
	int    i;

	if (*nptr == NULL) return(nptr);
	if ((i = namesame(name, (*nptr)->name)) > 0)
		return(Sc_findni(&(*nptr)->rptr, name));
	if (i) return(Sc_findni(&(*nptr)->lptr, name));
	return(nptr);
}

/***********************************************************************
Module:  Sc_make_nilist
------------------------------------------------------------------------
Description:
	Create instance list running through the instance tree to provide
	optional database operations.
------------------------------------------------------------------------
Calling Sequence:  Sc_make_nilist(nptr, cell);

Name		Type		Description
----		----		-----------
nptr		*SCNITREE	Pointer to current tree node.
cell		*SCCELL		Pointer to parent cell.
------------------------------------------------------------------------
*/

void Sc_make_nilist(SCNITREE *nptr, SCCELL *cell)
{
	static SCNITREE	*itemp;

	if (nptr == NULL) return;
	Sc_make_nilist(nptr->lptr, cell);

	/* process node */
	if (cell->nilist == NULL)
	{
		cell->nilist = nptr;
	} else
	{
		itemp->next = nptr;
	}
	itemp = nptr;
	nptr->next = NULL;
	Sc_make_nilist(nptr->rptr, cell);
}

/***********************************************************************
Module:  Sc_remove_inst_from_itree
------------------------------------------------------------------------
Description:
	Remove the indicated instance from the instance tree.  If the
	instance has no successors, it is simply removed.  If it has only
	one successor, the pointer of the parent node is set to that
	successor.  If there are both successor subtrees, the inorder
	successor of the node to be deleted replaces the node being removed
	and its right child becomes the left child of the parent of the
	inorder sucessor.
------------------------------------------------------------------------
Calling Sequence:  Sc_remove_inst_from_itree(root, inst);

Name		Type		Description
----		----		-----------
root		**SCNITREE	Address of the root pointer.
inst		*SCNITREE	Tree instance (node) to be deleted.
------------------------------------------------------------------------
*/

void Sc_remove_inst_from_itree(SCNITREE **root, SCNITREE *inst)
{
	SCNITREE	**parent, *tptr, *t2ptr, **parent2;

	/* first find parent of instance */
	parent = Sc_find_parent_pointer_addr(root, inst);

	/* check for no children */
	if (inst->lptr == NULL && inst->rptr == NULL)
	{
		*parent = NULL;
	} else if (inst->lptr == NULL)
	{
		/* only one child, the right child */
		*parent = inst->rptr;
	} else if (inst->rptr == NULL)
	{
		/* only one child, the left child */
		*parent = inst->lptr;
	} else
	{
		/* has two children */
		/* find inorder successor of right subtree */
		tptr = inst->rptr;
		while (tptr->lptr != NULL)
			tptr = tptr->lptr;

		/* remember right subtree of this node and get parent */
		t2ptr = tptr->rptr;
		parent2 = Sc_find_parent_pointer_addr(root, tptr);

		/* set this node in place of node being removed */
		*parent = tptr;
		tptr->lptr = inst->lptr;

		/* check if inorder successor is child of node being deleted */
		if (tptr == inst->rptr)
		{
			/* no action required */
			/* EMPTY */ 
		} else
		{
			tptr->rptr = inst->rptr;

			/* set parent of inorder successor to right subtree */
			*parent2 = t2ptr;
		}
	}
}

/***********************************************************************
Module:  Sc_find_parent_pointer_addr
------------------------------------------------------------------------
Description:
	Return the address of the pointer in the instance tree starting at
	root which refers to the indicated node.
------------------------------------------------------------------------
Calling Sequence:  parent = Sc_find_parent_pointer_addr(root, node);

Name		Type		Description
----		----		-----------
root		**SCNITREE	Address of pointer to current root.
node		*SCNITREE	Pointer to node being searched for.
parent		**SCNITREE	Returned address of parent pointer,
								NULL if not found.
------------------------------------------------------------------------
*/

SCNITREE  **Sc_find_parent_pointer_addr(SCNITREE **root, SCNITREE *node)
{
	int		i;

	if (*root == NULL) return((SCNITREE **)NULL);
	if (*root == node) return(root);
	if ((i = namesame(node->name, (*root)->name)) > 0)
		return(Sc_find_parent_pointer_addr(&(*root)->rptr, node));
	if (i) return(Sc_find_parent_pointer_addr(&(*root)->lptr, node));

	/* shouldn't get here */
	return((SCNITREE **)NULL);
}

/***********************************************************************
Module:  Sc_new_instance
------------------------------------------------------------------------
Description:
	Return a new instance and set the passed name and type.  Return
	NULL if error.
------------------------------------------------------------------------
Calling Sequence:  ninst = Sc_new_instance(name, type);

Name		Type		Description
----		----		-----------
name		*char		Pointer to string to copy.
type		int			Type of instance.
ninst		*SCNITREE	Returned new instance, NULL if error.
------------------------------------------------------------------------
*/

SCNITREE  *Sc_new_instance(CHAR *name, int type)
{
	SCNITREE	*ninst;

	ninst = (SCNITREE *)emalloc(sizeof(SCNITREE), sc_tool->cluster);
	if (ninst == 0)
		return(NULL);
	if (allocstring(&(ninst->name), name, sc_tool->cluster))
		return(NULL);
	ninst->type = type;
	ninst->number = 0;
	ninst->np = NULL;
	ninst->size = 0;
	ninst->connect = NULL;
	ninst->ports = NULL;
	ninst->power = NULL;
	ninst->ground = NULL;
	ninst->flags = 0;
	ninst->tp = NULL;
	ninst->next = NULL;
	ninst->lptr = NULL;
	ninst->rptr = NULL;
	return(ninst);
}

/***********************************************************************
Module:  Sc_new_instance_port
------------------------------------------------------------------------
Description:
	Return a new instance port and add to the given instance.  Return
	NULL if error.
------------------------------------------------------------------------
Calling Sequence:  nport = Sc_new_instance_port(instance);

Name		Type		Description
----		----		-----------
instance	*SCNITREE	Pointer to instance.
nport		*SCNIPORT	Returned new instance port, NULL if error.
------------------------------------------------------------------------
*/

SCNIPORT  *Sc_new_instance_port(SCNITREE *instance)
{
	SCNIPORT	*nport;

	nport = (SCNIPORT *)emalloc(sizeof(SCNIPORT), sc_tool->cluster);
	if (nport == 0)
		return(NULL);
	nport->port = NULL;
	nport->ext_node = NULL;
	nport->bits = 0;
	nport->xpos = 0;
	nport->next = instance->ports;
	instance->ports = nport;

	return(nport);
}

#endif  /* SCTOOL - at top */
