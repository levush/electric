/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: simalsflat.c
 * Asynchronous Logic Simulator network flattening
 * From algorithms by: Brent Serbin and Peter J. Gallant
 * Last maintained by: Steven M. Rubin
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
#if SIMTOOL

#include "global.h"
#include "sim.h"
#include "simals.h"

static MODPTR simals_primptr2;
static MODEL **simals_primptr1;
static CHAR *simals_mainname = 0;

/* prototypes for local routines */
static BOOLEAN simals_flatten_model(CONPTR);
static BOOLEAN simals_process_connect_list(CONPTR, CONPTR);
static MODPTR  simals_find_model(CHAR*);
static EXPTR   simals_find_xref_entry(CONPTR, CHAR*);
static BOOLEAN simals_process_gate(CONPTR, MODPTR);
static BOOLEAN simals_process_io_entry(MODPTR, CONPTR, IOPTR, CHAR);
static BOOLEAN simals_create_pin_entry(MODPTR, CHAR*, NODEPTR);
static STATPTR simals_create_stat_entry(MODPTR, CHAR*, NODEPTR);
static float   simals_find_load_value(MODPTR, CHAR*);
static BOOLEAN simals_process_set_entry(CONPTR, IOPTR);
static BOOLEAN simals_process_function(CONPTR, MODPTR);

/*
 * Routine to free all memory associated with this module.
 */
void simals_freeflatmemory(void)
{
	if (simals_mainname != 0) efree((CHAR *)simals_mainname);
}

/*
 * Name: simals_flatten_network
 *
 * Description:
 *	This procedure calls a series of routines which convert the hierarchical
 * network description into a flattened database representation.  The actual
 * simulation must take place on the flattened network.  Returns true on error.
 */
BOOLEAN simals_flatten_network(void)
{
	MODPTR   modhead;
	EXPTR    exhead;
	NODEPTR  nodehead;
	CONPTR   temproot;

	simals_nseq = simals_pseq = 0;
	simals_primptr1 = &simals_primroot;

	/*
	 * create a "dummy" level to use as a mixed signal destination for plotting and
	 * screen display.  This level should be bypassed for structure checking and general
	 * simulation, however, so in the following code, references to "simals_cellroot"
	 * have been changed to simals_cellroot->next (pointing to simals_mainproto).
	 * Peter Gallant July 16, 1990
	 */
	simals_cellroot = (CONPTR) simals_alloc_mem((INTBIG)sizeof(CONNECT));
	if (simals_cellroot == 0) return(TRUE);
	simals_cellroot->inst_name = x_("[MIXED_SIGNAL_LEVEL]");
	simals_cellroot->model_name = simals_cellroot->inst_name;
	simals_cellroot->exptr = 0;
	simals_cellroot->parent = 0;
	simals_cellroot->child = 0;
	simals_cellroot->next = 0;
	simals_cellroot->display_page = 0;
	simals_cellroot->num_chn = 0;
	temproot = simals_cellroot;

	/* get upper-case version of main proto */
	if (simals_mainname != 0) efree(simals_mainname);
	(void)allocstring(&simals_mainname, simals_mainproto->protoname, sim_tool->cluster);
	simals_convert_to_upper(simals_mainname);

	simals_cellroot = (CONPTR) simals_alloc_mem((INTBIG)sizeof(CONNECT));
	if (simals_cellroot == 0) return(TRUE);
	simals_cellroot->inst_name = simals_mainname;
	simals_cellroot->model_name = simals_cellroot->inst_name;
	simals_cellroot->exptr = 0;
	simals_cellroot->parent = 0;
	simals_cellroot->child = 0;
	simals_cellroot->next = 0;
	simals_cellroot->display_page = 0;
	simals_cellroot->num_chn = 0;

	/* these lines link the mixed level as the head followed by simals_mainproto PJG */
	temproot->next = simals_cellroot;		/* shouldn't this be zero? ... smr */
	temproot->child = simals_cellroot;
	simals_cellroot = temproot;

	/* this code checks to see if model simals_mainproto is present in the netlist PJG */
	modhead = simals_find_model(simals_mainname);
	if (modhead == 0) return(TRUE);
	for (exhead = modhead->exptr; exhead; exhead = exhead->next)
	{
		if (simals_find_xref_entry(simals_cellroot->next, exhead->node_name) == 0)
			return(TRUE);
	}

	if (simals_flatten_model(simals_cellroot->next)) return(TRUE);

	for (nodehead = simals_noderoot; nodehead; nodehead = nodehead->next)
	{
		if (nodehead->load < 1.0) nodehead->load = 1.0;
		nodehead->plot_node = 0;
	}
	return(FALSE);
}

/*
 * Name: simals_flatten_model
 *
 * Description:
 *	This procedure flattens a single model.  If other models are referenced
 * in connection statements in the netlist, this routine is called recursively
 * until a totally flat model is obtained.  Returns true on error.
 *
 * Calling Arguments:
 *	cellhead = pointer to a data structure containing information about
 *		  the model that is going to be flattened
 */
BOOLEAN simals_flatten_model(CONPTR cellhead)
{
	MODPTR  modhead;
	CONPTR  subcell;

	modhead = simals_find_model(cellhead->model_name);
	if (modhead == 0) return(TRUE);
	switch (modhead->type)
	{
		case 'F':
			if (simals_process_function(cellhead, modhead)) return(TRUE);
			break;

		case 'G':
			if (simals_process_gate(cellhead, modhead)) return(TRUE);
			break;

		case 'M':
			if (simals_process_connect_list(cellhead, (CONPTR)modhead->ptr)) return(TRUE);
			for (subcell = cellhead->child; subcell; subcell = subcell->next)
			{
				if (simals_flatten_model(subcell)) return(TRUE);
			}
			break;
	}

	if (modhead->setptr)
	{
		if (simals_process_set_entry(cellhead, modhead->setptr)) return(TRUE);
	}
	return(FALSE);
}

/*
 * Name: simals_process_connect_list
 *
 * Description:
 *	This procedure steps through the connection list specified by the
 * connection list pointer (conhead).  Values are entered into the cross
 * reference table for the present level of hierarchy and new data structures
 * are created for the lower level of hierarchy to store their cross
 * reference tables.  Returns true on error.
 *
 * Calling Arguments:
 *	cellhead = pointer to the cross reference data structure for the model
 *		  that is going to be flattened
 *	conhead  = pointer to a list of connection statements for the model
 *		  that is being flattened by this procedure
 */
BOOLEAN simals_process_connect_list(CONPTR cellhead, CONPTR conhead)
{
	EXPORT  **xrefptr1;
	CONNECT **cellptr1;
	EXPTR     exhead, xrefhead, xrefptr2;
	MODPTR    modhead;
	CONPTR    cellptr2;

	cellptr1 = &cellhead->child;
	while (conhead)
	{
		cellptr2 = (CONPTR) simals_alloc_mem((INTBIG)sizeof(CONNECT));
		if (cellptr2 == 0) return(TRUE);
		cellptr2->inst_name = conhead->inst_name;
		cellptr2->model_name = conhead->model_name;
		cellptr2->exptr = 0;
		cellptr2->parent = cellhead;
		cellptr2->child = 0;
		cellptr2->next = 0;
		cellptr2->display_page = 0;
		*cellptr1 = cellptr2;
		cellptr1 = &cellptr2->next;

		modhead = simals_find_model(conhead->model_name);
		if (modhead == 0) return(TRUE);
		simals_exptr2 = modhead->exptr;
		for (exhead = conhead->exptr; exhead; exhead = exhead->next)
		{
			xrefhead = simals_find_xref_entry(cellhead, exhead->node_name);
			if (xrefhead == 0) return(TRUE);

			if (! simals_exptr2)
			{
				ttyputerr(_("Insufficient parameters declared for model '%s' in netlist"),
					conhead->model_name);
				return(TRUE);
			}

			xrefptr1 = &cellptr2->exptr;
			for(;;)
			{
				if (*xrefptr1 == 0)
				{
					xrefptr2 = (EXPTR) simals_alloc_mem((INTBIG)sizeof(EXPORT));
					if (xrefptr2 == 0) return(TRUE);
					xrefptr2->node_name = simals_exptr2->node_name;
					xrefptr2->nodeptr = xrefhead->nodeptr;
					xrefptr2->next = 0;
					*xrefptr1 = xrefptr2;
					break;
				}
				xrefptr2 = *xrefptr1;
				if (! estrcmp(xrefptr2->node_name, simals_exptr2->node_name))
				{
					ttyputerr(_("Node '%s' in model '%s' connected more than once"),
						simals_exptr2->node_name, conhead->model_name);
					return(TRUE);
				}
				xrefptr1 = &xrefptr2->next;
			}

			simals_exptr2 = simals_exptr2->next;
		}

		conhead = conhead->next;
	}
	return(FALSE);
}

/*
 * Name: simals_find_model
 *
 * Description:
 *	This procedure returns a pointer to the model referenced by the
 * calling argument character string.  Returns zero on error.
 *
 * Calling Arguments:
 *	model_name = pointer to a string which contains the name of the model
 *		    to be located by the search procedure
 */
MODPTR  simals_find_model(CHAR *model_name)
{
	MODPTR  modhead;
	CHAR	propername[256], *pt;

	/* convert to proper name */
	estrcpy(propername, model_name);
	for(pt = propername; *pt != 0; pt++)
		if (!isalnum(*pt)) *pt = '_';

	modhead = simals_modroot;
	for(;;)
	{
		if (modhead == 0)
		{
			ttyputerr(_("ERROR: Model '%s' not found, simulation aborted"), propername);
			break;
		}
		if (! estrcmp(modhead->name, propername)) return(modhead);
		modhead = modhead->next;
	}
	return(0);
}

/*
 * Name: simals_find_xref_entry
 *
 * Description:
 *	This procedure returns the flattened database node number for the
 * specified model and node name.  Returns zero on error.
 *
 * Calling Arguments:
 *	cellhead = pointer to the xref table for the model being processed
 *	name    = pointer to a char string containing the node name
 */
EXPTR simals_find_xref_entry(CONPTR cellhead, CHAR *name)
{
	EXPORT **xrefptr1;
	EXPTR    xrefptr2;
	NODEPTR  nodeptr2;

	xrefptr1 = &cellhead->exptr;
	for(;;)
	{
		if (*xrefptr1 == 0)
		{
			xrefptr2 = (EXPTR)simals_alloc_mem((INTBIG)sizeof(EXPORT));
			if (xrefptr2 == 0) return(0);
			xrefptr2->node_name = name;
			/* this could be a problem during model erase...smr!!! */
			xrefptr2->next = 0;
			*xrefptr1 = xrefptr2;
			break;
		}
		xrefptr2 = *xrefptr1;
		if (! estrcmp(xrefptr2->node_name, name)) return(xrefptr2);
		xrefptr1 = &xrefptr2->next;
	}

	nodeptr2 = (NODEPTR)simals_alloc_mem((INTBIG)sizeof(NODE));
	if (nodeptr2 == 0) return(0);
	nodeptr2->cellptr = cellhead;
	nodeptr2->num = simals_nseq;
	++simals_nseq;
	nodeptr2->plot_node = 0;
	nodeptr2->statptr = 0;
	nodeptr2->pinptr = 0;
	nodeptr2->load = -1.0;
	nodeptr2->visit = 0;
	nodeptr2->tracenode = FALSE;
	nodeptr2->next = simals_noderoot;
	xrefptr2->nodeptr = simals_noderoot = nodeptr2;
	return(xrefptr2);
}

/*
 * Name: simals_process_gate
 *
 * Description:
 *	This procedure steps through the gate truth tables and examines all
 * node references to insure that they have been included in the cross
 * reference table for the model.  Returns true on error.
 *
 * Calling Arguments:
 *	cellhead = pointer to the cross reference data structure for the model
 *		  that is going to be flattened
 *	modhead  = pointer to the dtat structure containing the hierarchical
 *		  node references
 */
BOOLEAN simals_process_gate(CONPTR cellhead, MODPTR modhead)
{
	ROWPTR  rowhead;

	simals_primptr2 = (MODPTR)simals_alloc_mem((INTBIG)sizeof(MODEL));
	if (simals_primptr2 == 0) return(TRUE);
	simals_primptr2->num = simals_pseq;
	++simals_pseq;
	simals_primptr2->name = modhead->name;
	simals_primptr2->type = 'G';
	simals_primptr2->ptr = 0;
	simals_primptr2->exptr = 0;
	simals_primptr2->setptr = 0;
	simals_primptr2->loadptr = 0;
	simals_primptr2->fanout = modhead->fanout;
	simals_primptr2->priority = modhead->priority;
	simals_primptr2->next = 0;
	(void)allocstring(&(simals_primptr2->level), simals_compute_path_name(cellhead), sim_tool->cluster);
	*simals_primptr1 = simals_primptr2;
	simals_primptr1 = &simals_primptr2->next;

	simals_rowptr1 = &simals_primptr2->ptr;
	rowhead = (ROWPTR) modhead->ptr;
	while (rowhead)
	{
		simals_rowptr2 = (ROWPTR) simals_alloc_mem((INTBIG)sizeof(ROW));
		if (simals_rowptr2 == 0) return(TRUE);
		simals_rowptr2->inptr = 0;
		simals_rowptr2->outptr = 0;
		simals_rowptr2->delta = rowhead->delta;
		simals_rowptr2->linear = rowhead->linear;
		simals_rowptr2->exp = rowhead->exp;
		simals_rowptr2->abs = rowhead->abs;
		simals_rowptr2->random = rowhead->random;
		simals_rowptr2->delay = rowhead->delay;
		if (rowhead->delay == 0) simals_rowptr2->delay = 0; else
			(void)allocstring(&(simals_rowptr2->delay), rowhead->delay, sim_tool->cluster);
		simals_rowptr2->next = 0;
		*simals_rowptr1 = (CHAR*) simals_rowptr2;
		simals_rowptr1 = (CHAR**) &(simals_rowptr2->next);

		simals_ioptr1 = (CHAR**) &(simals_rowptr2->inptr);
		if (simals_process_io_entry(modhead, cellhead, rowhead->inptr, 'I')) return(TRUE);

		simals_ioptr1 = (CHAR**) &(simals_rowptr2->outptr);
		if (simals_process_io_entry(modhead, cellhead, rowhead->outptr, 'O')) return(TRUE);

		rowhead = rowhead->next;
	}
	return(FALSE);
}

/*
 * Name: simals_process_io_entry
 *
 * Description:
 *	This procedure steps through the node references contained within a
 * row of a transition table and insures that they are included in the cross
 * reference table in the event they were not previously specified in a
 * connection statement.  Returns true on error.
 *
 * Calling Arguments:
 *	modhead  = pointer to model that is being flattened
 *	cellhead = pointer to the cross reference data structure for the model
 *		  that is going to be flattened
 *	iohead   = pointer to a row of node references to be checked for
 *		  entry into the cross reference table
 *	flag    = character indicating if the node is an input or output
 */
BOOLEAN simals_process_io_entry(MODPTR modhead, CONPTR cellhead, IOPTR iohead, CHAR flag)
{
	EXPTR    xrefhead;

	while (iohead)
	{
		xrefhead = simals_find_xref_entry(cellhead, (CHAR *)iohead->nodeptr);
		if (xrefhead == 0) return(TRUE);
		simals_ioptr2 = (IOPTR) simals_alloc_mem((INTBIG)sizeof(IO));
		if (simals_ioptr2 == 0) return(TRUE);
		simals_ioptr2->nodeptr = xrefhead->nodeptr;
		simals_ioptr2->operatr = iohead->operatr;

		if (simals_ioptr2->operatr > 127)
		{
			xrefhead = simals_find_xref_entry(cellhead, iohead->operand);
			if (xrefhead == 0) return(TRUE);
			simals_ioptr2->operand = (CHAR *) xrefhead->nodeptr;
		} else
		{
			simals_ioptr2->operand = iohead->operand;
		}

		simals_ioptr2->strength = iohead->strength;
		simals_ioptr2->next = 0;
		*simals_ioptr1 = (CHAR*) simals_ioptr2;
		simals_ioptr1 = (CHAR**) &(simals_ioptr2->next);

		switch (flag)
		{
			case 'I':
				if (simals_create_pin_entry(modhead, (CHAR *)iohead->nodeptr,
					simals_ioptr2->nodeptr)) return(TRUE);
				break;
			case 'O':
				simals_ioptr2->nodeptr = (NODE *)simals_create_stat_entry(modhead,
					(CHAR *)iohead->nodeptr, simals_ioptr2->nodeptr);
				if (simals_ioptr2->nodeptr == 0) return(TRUE);
		}

		if (simals_ioptr2->operatr > 127)
		{
			if (simals_create_pin_entry(modhead, (CHAR *)iohead->operand,
				(NODEPTR)simals_ioptr2->operand)) return(TRUE);
		}

		iohead = iohead->next;
	}
	return(FALSE);
}

/*
 * Name: simals_create_pin_entry
 *
 * Description:
 *	This procedure makes an entry into the primitive input table for the
 * specified node.  This table keeps track of the primitives which use
 * this node as an input for event driven simulation.  Returns true on error.
 *
 * Calling Arguments:
 *	modhead   = pointer to the model structure from which the primitive
 *		   is being created
 *	node_name = pointer to a char string containing the name of the node
 *		   whose input list is being updated
 *	nodehead  = pointer to the node data structure allocated for this node
 */
BOOLEAN simals_create_pin_entry(MODPTR modhead, CHAR *node_name, NODEPTR nodehead)
{
	LOAD    **pinptr1;
	LOADPTR  pinptr2;

	pinptr1 = &nodehead->pinptr;
	for(;;)
	{
		if (*pinptr1 == 0)
		{
			pinptr2 = (LOADPTR)simals_alloc_mem((INTBIG)sizeof(LOAD));
			if (pinptr2 == 0) return(TRUE);
			pinptr2->ptr = (CHAR *) simals_primptr2;
			pinptr2->next = 0;
			*pinptr1 = pinptr2;
			nodehead->load += simals_find_load_value(modhead, node_name);
			break;
		}
		pinptr2 = *pinptr1;
		if ((MODPTR)pinptr2->ptr == simals_primptr2) break;
		pinptr1 = &pinptr2->next;
	}
	return(FALSE);
}

/*
 * Name: simals_create_stat_entry
 *
 * Description:
 *	This procedure makes an entry into the database for an output which
 * is connected to the specified node.  Statistics are maintained for each output
 * that is connected to a node.  Returns zero on error.
 *
 * Calling Arguments:
 *	modhead   = pointer to the model structure from which the primitive
 *		   is being created
 *	node_name = pointer to a char string containing the name of the node
 *		   whose output list is being updated
 *	nodehead  = pointer to the node data structure allocated for this node
 */
STATPTR  simals_create_stat_entry(MODPTR modhead, CHAR *node_name, NODEPTR nodehead)
{
	STAT    **statptr1;
	STATPTR  statptr2;

	statptr1 = &nodehead->statptr;
	for(;;)
	{
		if (*statptr1 == 0)
		{
			statptr2 = (STATPTR)simals_alloc_mem((INTBIG)sizeof(STAT));
			if (statptr2 == 0) break;
			statptr2->primptr = simals_primptr2;
			statptr2->nodeptr = nodehead;
			statptr2->next = 0;
			*statptr1 = statptr2;
			nodehead->load += simals_find_load_value(modhead, node_name);
			return(statptr2);
		}
		statptr2 = *statptr1;
		if (statptr2->primptr == simals_primptr2) return(statptr2);
		statptr1 = &statptr2->next;
	}
	return(0);
}

/*
 * Name: simals_find_load_value
 *
 * Description:
 *	This procedure returns the loading factor for the specified node.  If
 * the node can't be found in the load list it is assumed it has a default value
 * of 1.0.
 *
 * Calling Arguments:
 *	modhead   = pointer to the model structure from which the primitive
 *		   is being created
 *	node_name = pointer to a char string containing the name of the node
 *		   whose load value is to be determined
 */
float  simals_find_load_value(MODPTR modhead, CHAR *node_name)
{
	LOADPTR  loadhead;

	for (loadhead = modhead->loadptr; loadhead != 0; loadhead = loadhead->next)
	{
		if (! estrcmp(loadhead->ptr, node_name)) return(loadhead->load);
	}

	if (modhead->type == 'F') return(0.0);
	return(1.0);
}

/*
 * Name: simals_process_set_entry
 *
 * Description:
 *	This procedure goes through the set node list for the specified cell
 * and generates vectors for the node.  These vectors are executed at t=0 by
 * the simulator to initialize the node correctly.  Returns true on error.
 *
 * Calling Arguments:
 *	cellhead = pointer to the cross reference table where the node locations
 *		  are to be found
 *	iohead   = pointer to the set list containing node names and state info
 */
BOOLEAN simals_process_set_entry(CONPTR cellhead, IOPTR iohead)
{
	EXPTR    xrefhead;
	LINKPTR  sethead;

	for (; iohead; iohead = iohead->next)
	{
		xrefhead = simals_find_xref_entry(cellhead, (CHAR *)iohead->nodeptr);
		if (xrefhead == 0) return(TRUE);

		sethead = simals_alloc_link_mem();
		if (sethead == 0) return(TRUE);
		sethead->type = 'N';
		sethead->ptr = (CHAR *) xrefhead->nodeptr;
		sethead->state = (INTBIG) iohead->operand;
		sethead->strength = iohead->strength;
		sethead->priority = 2;
		sethead->time = 0.0;
		sethead->right = 0;
		simals_insert_set_list(sethead);
	}
	return(FALSE);
}

/*
 * Name: simals_process_function
 *
 * Description:
 *	This procedure steps through the event driving input list for a function
 * and enters the function into the primitive input list for the particular node.
 * In addition to this task the procedure sets up the calling argument node list
 * for the function when it is called.  Returns true on error.
 *
 * Calling Arguments:
 *	cellhead = pointer to the cross reference data structure for the model
 *		  that is going to be flattened
 *	modhead  = pointer to the data structure containing the hierarchical
 *		  node references
 */
BOOLEAN simals_process_function(CONPTR cellhead, MODPTR modhead)
{
	EXPTR    exhead, xrefhead;
	FUNCPTR  funchead, funcptr2;
	EXPORT **exptr1;

	simals_primptr2 = (MODPTR) simals_alloc_mem((INTBIG)sizeof(MODEL));
	if (simals_primptr2 == 0) return(TRUE);
	simals_primptr2->num = simals_pseq;
	++simals_pseq;
	simals_primptr2->name = modhead->name;
	simals_primptr2->type = 'F';
	simals_primptr2->ptr = (CHAR*) simals_alloc_mem((INTBIG)sizeof(FUNC));
	if (simals_primptr2->ptr == 0) return(TRUE);
	simals_primptr2->exptr = 0;
	simals_primptr2->setptr = 0;
	simals_primptr2->loadptr = 0;
	simals_primptr2->fanout = 0;
	simals_primptr2->priority = modhead->priority;
	simals_primptr2->next = 0;
	(void)allocstring(&(simals_primptr2->level), simals_compute_path_name(cellhead),
		sim_tool->cluster);
	*simals_primptr1 = simals_primptr2;
	simals_primptr1 = &simals_primptr2->next;

	funchead = (FUNCPTR)modhead->ptr;
	funcptr2 = (FUNCPTR)simals_primptr2->ptr;
	funcptr2->procptr = (void(*)(MODPTR))simals_get_function_address(modhead->name);
	if (funcptr2->procptr == 0) return(TRUE);
	funcptr2->inptr = 0;
	funcptr2->delta = funchead->delta;
	funcptr2->linear = funchead->linear;
	funcptr2->exp = funchead->exp;
	funcptr2->abs = funchead->abs;
	funcptr2->random = funchead->random;
	funcptr2->userptr = 0;
	funcptr2->userint = 0;
	funcptr2->userfloat = 0.0;

	exptr1 = &simals_primptr2->exptr;
	for (exhead = modhead->exptr; exhead; exhead = exhead->next)
	{
		xrefhead = simals_find_xref_entry(cellhead, exhead->node_name);
		if (xrefhead == 0) return(TRUE);
		simals_exptr2 = (EXPTR) simals_alloc_mem((INTBIG)sizeof(EXPORT));
		if (simals_exptr2 == 0) return(TRUE);
		if (exhead->nodeptr)
		{
			simals_exptr2->node_name = (CHAR *) simals_create_stat_entry(modhead,
				exhead->node_name, xrefhead->nodeptr);
			if (simals_exptr2->node_name == 0) return(TRUE);
		} else
		{
			simals_exptr2->node_name = 0;
		}
		simals_exptr2->nodeptr = xrefhead->nodeptr;
		simals_exptr2->next = 0;
		*exptr1 = simals_exptr2;
		exptr1 = &simals_exptr2->next;
	}

	exptr1 = &funcptr2->inptr;
	for (exhead = funchead->inptr; exhead; exhead = exhead->next)
	{
		xrefhead = simals_find_xref_entry(cellhead, exhead->node_name);
		if (xrefhead == 0) return(TRUE);
		simals_exptr2 = (EXPTR) simals_alloc_mem((INTBIG)sizeof(EXPORT));
		if (simals_exptr2 == 0) return(TRUE);
		simals_exptr2->nodeptr = xrefhead->nodeptr;
		simals_exptr2->next = 0;
		*exptr1 = simals_exptr2;
		exptr1 = &simals_exptr2->next;
		if (simals_create_pin_entry(modhead, exhead->node_name, xrefhead->nodeptr))
			return(TRUE);
	}
	return(FALSE);
}

#endif  /* SIMTOOL - at top */
