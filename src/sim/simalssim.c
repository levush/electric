/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: simalssim.c
 * Asynchronous Logic Simulator engine
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

static LINKPTR simals_freelink = 0;
BOOLEAN  simals_tracing = FALSE;
static CHAR *simals_statedesc[] = {N_("High"), N_("Undefined"), N_("Low")};
static CHAR *simals_strengthdesc[] = {N_("Off-"), N_("Weak-"), N_("Weak-"), x_(""), x_(""),
	N_("Strong-"), N_("Strong-")};

/* prototypes for local routines */
static BOOLEAN simals_fire_event(void);
static void    simals_create_check_list(NODEPTR, LINKPTR);
static BOOLEAN simals_schedule_new_events(void);
static void    simals_calculate_clock_time(LINKPTR, ROWPTR);
static BOOLEAN simals_calculate_event_time(MODPTR, ROWPTR);

/*
 * Routine to free all memory associated with this module.
 */
void simals_freesimmemory(void)
{
	LINKPTR link;

	while (simals_freelink != 0)
	{
		link = simals_freelink;
		simals_freelink = simals_freelink->left;
		efree((CHAR *)link);
	}
}

/*
 * Name: simals_initialize_simulator
 *
 * Description:
 *	This procedure initializes the simulator for a simulation run.  The
 * vector link list is copied to a master event scheduling link list and
 * the database is initialized to its starting values.  After these housekeeping
 * tasks are completed the simulator is ready to start the actual simulation.
 * Returns the time where the simulation quiesces.
 */
double simals_initialize_simulator(BOOLEAN force)
{
	LINKPTR linkptr2, linkhead, link;
	NODEPTR nodehead;
	STATPTR stathead;
	double tmin, tmax;
	REGISTER VARIABLE *var;
	double mintime, maxtime;
	REGISTER INTBIG tr;
	BOOLEAN first;

	simals_time_abs = 0.0;
	simals_trakptr = simals_trakfull = 0;

	while (simals_linkfront)
	{
		link = simals_linkfront;
		if (simals_linkfront->down)
		{
			simals_linkfront->down->right = simals_linkfront->right;
			simals_linkfront = simals_linkfront->down;
		} else
		{
			simals_linkfront = simals_linkfront->right;
		}
		simals_free_link_mem(link);
	}
	simals_linkback = 0;

	for (linkhead = simals_setroot; linkhead; linkhead = linkhead->right)
	{
		linkptr2 = simals_alloc_link_mem();
		if (linkptr2 == 0) return(simals_time_abs);
		linkptr2->type = linkhead->type;
		linkptr2->ptr = linkhead->ptr;
		linkptr2->state = linkhead->state;
		linkptr2->strength = linkhead->strength;
		linkptr2->priority = linkhead->priority;
		linkptr2->time = linkhead->time;
		linkptr2->primhead = 0;
		simals_insert_link_list(linkptr2);
	}

	for (nodehead = simals_noderoot; nodehead; nodehead = nodehead->next)
	{
		nodehead->sum_state = LOGIC_LOW;
		nodehead->sum_strength = OFF_STRENGTH;
		nodehead->new_state = LOGIC_LOW;
		nodehead->new_strength = OFF_STRENGTH;
		nodehead->maxsize = 0;
		nodehead->arrive = 0;
		nodehead->depart = 0;
		nodehead->tk_sec = 0.0;
		nodehead->t_last = 0.0;
		for (stathead = nodehead->statptr; stathead; stathead = stathead->next)
		{
			stathead->new_state = LOGIC_LOW;
			stathead->new_strength = OFF_STRENGTH;
			stathead->sched_op = 0;
		}
	}

	if (simals_seed_flag) srand(3);

	/* now run the simulation */
	if (force) var = NOVARIABLE; else
		var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, simals_no_update_key);
	if (var == NOVARIABLE || var->addr == 0)
	{
		/* determine range of displayed time */
		sim_window_inittraceloop();
		first = TRUE;
		for(;;)
		{
			tr = sim_window_nexttraceloop();
			if (tr == 0) break;
			sim_window_gettimerange(tr, &mintime, &maxtime);
			if (first)
			{
				tmin = mintime;
				tmax = maxtime;
				first = FALSE;
			} else
			{
				if (mintime < tmin) tmin = mintime;
				if (maxtime > tmax) tmax = maxtime;
			}
		}
		if (first) return(simals_time_abs);

		/* fire events until end of time or quiesced */
		while (simals_linkfront && (simals_linkfront->time <= tmax))
		{
			if (stopping(STOPREASONSIMULATE)) break;
			if (simals_fire_event()) break;
			if (simals_chekroot)
			{
				if (simals_schedule_new_events()) break;
			}
		}

		/* redisplay results */
		if (simals_levelptr != 0) simals_fill_display_arrays();
		sim_window_redraw();
		sim_window_updatelayoutwindow();
		ttyputverbose(M_("Simulation completed at time %s"),
			sim_windowconvertengineeringnotation(simals_time_abs));
	}

	return(simals_time_abs);
}

/*
 * Name: simals_fire_event
 *
 * Description:
 *	This procedure gets the entry from the front of the event scheduling
 * link list and updates the database accordingly.  If a node is updated by a
 * user defined vector the node value is changed as specified in the linklist
 * entry.  If a transition fired, all the output nodes are updated as specified
 * in the truth table for the transtion.  Returns true on error.
 */
BOOLEAN simals_fire_event(void)
{
	INTBIG operand, state;
	UCHAR  operatr;
	double time;
	LINKPTR	linkhead, linkptr2, vecthead;
	NODEPTR	nodehead;
	ROWPTR rowhead;
	STATPTR	stathead;
	CHAR s2[300];

	simals_time_abs = simals_linkfront->time;
	linkhead = simals_linkfront;
	if (simals_linkfront->down)
	{
		simals_linkfront = simals_linkfront->down;
		simals_linkfront->left = 0;
		simals_linkfront->right = linkhead->right;
		simals_linkfront->up = linkhead->up;
		if (simals_linkfront->right)
		{
			simals_linkfront->right->left = simals_linkfront;
		} else
		{
			simals_linkback = simals_linkfront;
		}
	} else
	{
		simals_linkfront = simals_linkfront->right;
		if (simals_linkfront)
		{
			simals_linkfront->left = 0;
		} else
		{
			simals_linkback = 0;
		}
	}

	simals_tracing = FALSE;
	switch (linkhead->type)
	{
		case 'G':
			stathead = (STATPTR)linkhead->ptr;
			if (simals_trace_all_nodes || stathead->nodeptr->tracenode)
			{
				simals_compute_node_name(stathead->nodeptr, s2);
				ttyputmsg(x_("%s: Firing gate %s%s, net %s"), sim_windowconvertengineeringnotation(simals_time_abs),
					stathead->primptr->name, stathead->primptr->level, s2);
				simals_tracing = TRUE;
			}
			if (stathead->sched_state != linkhead->state ||
				stathead->sched_op != linkhead->operatr ||
				stathead->sched_strength != linkhead->strength)
			{
				break;
			}
			stathead->sched_op = 0;

			operatr = linkhead->operatr;
			if (operatr < 128)
			{
				operand = (INTBIG)linkhead->state;
			} else
			{
				operatr -= 128;
				nodehead = (NODEPTR)linkhead->state;
				operand = nodehead->sum_state;
			}

			switch (operatr)
			{
				case '=':
					state = operand;
					break;
				case '+':
					state = stathead->nodeptr->sum_state + operand;
					break;
				case '-':
					state = stathead->nodeptr->sum_state - operand;
					break;
				case '*':
					state = stathead->nodeptr->sum_state * operand;
					break;
				case '/':
					state = stathead->nodeptr->sum_state / operand;
					break;
				case '%':
					state = stathead->nodeptr->sum_state % operand;
					break;
				default:
					ttyputerr(_("Invalid arithmetic operator: %c"), operatr);
					return(TRUE);
			}

			if (state == stathead->new_state &&
				linkhead->strength == stathead->new_strength)
			{
				break;
			}
			stathead->new_state = state;
			stathead->new_strength = linkhead->strength;
			simals_create_check_list(stathead->nodeptr, linkhead);
			break;

		case 'N':
			nodehead = (NODEPTR)linkhead->ptr;
			if (simals_trace_all_nodes || nodehead->tracenode)
			{
				simals_compute_node_name(nodehead, s2);
				ttyputmsg(x_("%s: Changed state of net %s"), sim_windowconvertengineeringnotation(simals_time_abs), s2);
				simals_tracing = TRUE;
			}
			if (linkhead->state == nodehead->new_state &&
				linkhead->strength == nodehead->new_strength)
					break;

			nodehead->new_state = linkhead->state;
			nodehead->new_strength = linkhead->strength;
			simals_create_check_list(nodehead, linkhead);
			break;

		case 'C':
			time = simals_time_abs;
			rowhead = (ROWPTR)linkhead->ptr;
			for (vecthead = (LINKPTR)rowhead->inptr; vecthead; vecthead = vecthead->right)
			{
				linkptr2 = simals_alloc_link_mem();
				if (linkptr2 == 0) return(TRUE);
				linkptr2->type = 'N';
				linkptr2->ptr = vecthead->ptr;
				linkptr2->state = vecthead->state;
				linkptr2->strength = vecthead->strength;
				linkptr2->priority = vecthead->priority;
				linkptr2->time = time;
				linkptr2->primhead = 0;
				simals_insert_link_list(linkptr2);
				time += vecthead->time;
			}
			if (linkhead->state == 0)
			{
				simals_calculate_clock_time(linkhead, rowhead);
				return(FALSE);
			}
			--(linkhead->state);
			if (linkhead->state)
			{
				simals_calculate_clock_time(linkhead, rowhead);
				return(FALSE);
			}
	}

	simals_free_link_mem(linkhead);
	return(FALSE);
}

/*
 * Name: simals_create_check_list
 *
 * Description:
 *	This procedure calculates the sum state and strength for a node and if
 * it has changed from a previous check it will enter the input transition list
 * into a master check list that is used by the event scheduling routine.
 * It should be noted that it is neccessary to calculate the sum strength and
 * state for a node because it is possible to have nodes that have more than
 * one transition driving it.
 */

void simals_create_check_list(NODEPTR nodehead, LINKPTR linkhead)
{
	REGISTER INTBIG state, thisstate;
	INTSML	strength, thisstrength;
	STATPTR stathead;
	TRAKPTR trakhead;
	Q_UNUSED( linkhead );

	/* get initial state of the node */
	state = nodehead->new_state;
	strength = nodehead->new_strength;

	/* print state of signal if this signal is being traced */
	if (simals_tracing)
	{
		ttyputmsg(x_("  Formerly %s%s, starts at %s%s"),
			TRANSLATE(simals_strengthdesc[nodehead->sum_strength]),
			TRANSLATE(simals_statedesc[nodehead->sum_state+3]),
			TRANSLATE(simals_strengthdesc[strength]),
			TRANSLATE(simals_statedesc[state+3]));
	}

	/* look at all factors affecting the node */
	for (stathead = nodehead->statptr; stathead; stathead = stathead->next)
	{
		thisstate = stathead->new_state;
		thisstrength = stathead->new_strength;
		if (simals_tracing)
			ttyputmsg(x_("    %s%s from %s%s"), TRANSLATE(simals_strengthdesc[thisstrength]),
			TRANSLATE(simals_statedesc[thisstate+3]), stathead->primptr->name, stathead->primptr->level);

		/* higher strength overrides previous node state */
		if (thisstrength > strength)
		{
			state = thisstate;
			strength = thisstrength;
			continue;
		}

		/* same strength: must arbitrate */
		if (thisstrength == strength)
		{
			if (thisstate != state)
				state = LOGIC_X;
		}
	}

	/* if the node has nothing driving it, set it to the old value */
	if (strength == OFF_STRENGTH)
	{
		state = nodehead->sum_state;
		strength = NODE_STRENGTH;
	}

	/* stop now if node state did not change */
	if (nodehead->sum_state == state && nodehead->sum_strength == strength)
	{
		if (simals_tracing) ttyputmsg(x_("    NO CHANGE"));
		return;
	}

	if (nodehead->plot_node)
	{
		trakhead = &(simals_trakroot[simals_trakptr]);
		trakhead->ptr = (NODEPTR)nodehead;
		trakhead->state = state;
		trakhead->strength = strength;
		trakhead->time = simals_time_abs;
		simals_trakptr = (simals_trakptr + 1) % simals_trace_size;
		if (! simals_trakptr)
			simals_trakfull = 1;
	}
	if (simals_tracing)
		ttyputmsg(x_("    BECOMES %s%s"), TRANSLATE(simals_strengthdesc[strength]),
			TRANSLATE(simals_statedesc[state+3]));

	nodehead->sum_state = state;
	nodehead->sum_strength = strength;
	nodehead->t_last = simals_time_abs;

	simals_drive_node = nodehead;
	simals_chekroot = nodehead->pinptr;
}

/*
 * Name: simals_schedule_new_events
 *
 * Description:
 *	This procedure examines the truth tables for the transitions that are
 * specified in the checking list.  If there is a match between a truth table
 * entry and the state of the logic network the transition is scheduled
 * for firing.  Returns true on error.
 */
BOOLEAN simals_schedule_new_events(void)
{
	INTBIG operand;
	UCHAR1 flag;
	UCHAR operatr;
	LOADPTR	chekhead;
	MODPTR primhead;
	ROWPTR rowhead;
	IOPTR iohead;
	NODEPTR	nodehead;
	FUNCPTR	funchead;

	for (chekhead = simals_chekroot; chekhead; chekhead = chekhead->next)
	{
		primhead = (MODPTR)chekhead->ptr;

		if (primhead->type == 'F')
		{
			funchead = (FUNCPTR)primhead->ptr;
			(*(funchead->procptr))(primhead);
			continue;
		}

		for (rowhead = (ROWPTR)primhead->ptr; rowhead; rowhead = rowhead->next)
		{
			flag = 1;
			for (iohead = rowhead->inptr; iohead; iohead = iohead->next)
			{
				operatr = iohead->operatr;
				if (operatr < 128)
				{
					operand = (INTBIG)iohead->operand;
				} else
				{
					operatr -= 128;
					nodehead = (NODEPTR)iohead->operand;
					operand = nodehead->sum_state;
				}

				switch (operatr)
				{
					case '=':
						if (iohead->nodeptr->sum_state != operand) flag = 0;
						break;
					case '!':
						if (iohead->nodeptr->sum_state == operand) flag = 0;
						break;
					case '<':
						if (iohead->nodeptr->sum_state >= operand) flag = 0;
						break;
					case '>':
						if (iohead->nodeptr->sum_state <= operand) flag = 0;
						break;
					default:
						ttyputerr(_("Invalid logical operator: %c"), operatr);
						return(TRUE);
				}

				if (! flag) break;
			}

			if (flag)
			{
				if (simals_calculate_event_time(primhead, rowhead)) return(TRUE);
				break;
			}
		}
	}
	simals_chekroot = 0;
	return(FALSE);
}

/*
 * Name: simals_calculate_clock_time
 *
 * Description:
 *	This procedure calculates the time when the next occurance of a set of
 * clock vectors is to be added to the event scheduling linklist.
 *
 * Calling Arguments:
 *	linkhead = pointer to the link element to be reinserted into the list
 *  rowhead  = pointer to a row element containing timing information
 */
void simals_calculate_clock_time(LINKPTR linkhead, ROWPTR rowhead)
{
	double time, prob;

	time = simals_time_abs;

	if (rowhead->delta) time += rowhead->delta;
	if (rowhead->linear)
	{
		prob = rand() / MAXINTBIG;
		time += 2.0 * prob * rowhead->linear;
	}

	/*
	 * if (rowhead->exp)
	 * {
	 * 	prob = rand() / MAXINTBIG;
	 * 	time += (-log(prob) * (rowhead->exp));
	 * }
	 */

	linkhead->time = time;
	simals_insert_link_list(linkhead);

	return;
}

/*
 * Name: simals_calculate_event_time
 *
 * Description:
 *	This procedure calculates the time of occurance of an event and then
 * places an entry into the event scheduling linklist for later execution.
 * Returns true on error.
 *
 * Calling Arguments:
 *	primhead  = pointer to the primitive to be scheduled for firing
 *	rowhead  = pointer to the row containing the event to be scheduled
 */
BOOLEAN simals_calculate_event_time(MODPTR primhead, ROWPTR rowhead)
{
	INTSML priority;
	double time, prob;
	STATPTR	stathead;
	IOPTR iohead;
	LINKPTR	linkptr2;

	time = 0.0;
	priority = primhead->priority;

	if (rowhead->delta) time += rowhead->delta;
	if (rowhead->abs) time += rowhead->abs;
	if (rowhead->linear)
	{
		prob = rand() / MAXINTBIG;
		time += 2.0 * prob * rowhead->linear;
	}

	/*
	 * if (rowhead->exp)
	 * {
	 * 	prob = rand() / MAXINTBIG;
	 * 	time += (-log(prob) * (rowhead->exp));
	 * }
	 */

	if (rowhead->random)
	{
		prob = rand() / MAXINTBIG;
		if (prob <= rowhead->random)
		{
			priority = -1;
		}
	}

	if (primhead->fanout)
	{
		stathead = (STATPTR)rowhead->outptr->nodeptr;
		time *= stathead->nodeptr->load;
	}
	time += simals_time_abs;

	for (iohead = rowhead->outptr; iohead; iohead = iohead->next)
	{
		stathead = (STATPTR)iohead->nodeptr;
		if (stathead->sched_op == iohead->operatr &&
			stathead->sched_state == (INTBIG)iohead->operand &&
				stathead->sched_strength == iohead->strength)
		{
			continue;
		}

		linkptr2 = simals_alloc_link_mem();
		if (linkptr2 == 0) return(TRUE);
		linkptr2->type = 'G';
		linkptr2->ptr = (CHAR *)stathead;
		linkptr2->operatr = stathead->sched_op = iohead->operatr;
		linkptr2->state = stathead->sched_state = (INTBIG)iohead->operand;
		linkptr2->strength = stathead->sched_strength = iohead->strength;
		linkptr2->time = time;
		linkptr2->priority = priority;
		linkptr2->primhead = primhead;
		if (simals_tracing)
		{
			ttyputmsg(_("      Schedule(G): %s%s at %s"), stathead->primptr->name,
				stathead->primptr->level, sim_windowconvertengineeringnotation(time));
		}
		simals_insert_link_list(linkptr2);
	}
	return(FALSE);
}

/*
 * Name: simals_alloc_link_mem
 *
 * Description:
 *	This procedure allocates a block of memory for a single entry in the
 * link list data structure.  This procedure uses its own memory pool if some is
 * available to speed up the simulation process.  Returns zero on error.
 */
LINKPTR simals_alloc_link_mem(void)
{
	LINKPTR link;

	if (simals_freelink == 0)
	{
		link = (LINKPTR)simals_alloc_mem((INTBIG)sizeof(LINK));
		if (link == 0) return(0);
	} else
	{
		link = simals_freelink;
		simals_freelink = simals_freelink->left;
	}
	return(link);
}

/*
 * Name: simals_free_link_mem
 *
 * Description:
 *	This procedure frees a block of memory for a link list data structure.
 * Rather than returning the memory to the system it enters the memory location
 * inot its own memory pool.  This is done to speed up the performance of a
 * simulation.
 *
 * Calling Argument
 *	linkhead = pointer to data structure that should be added to memory pool
 */
void simals_free_link_mem(LINKPTR linkhead)
{
	linkhead->left = simals_freelink;
	simals_freelink = linkhead;
}

/*
 * Name: simals_insert_set_list
 *
 * Description:
 *	This procedure inserts a data element into a linklist that is sorted
 * by time and then priority.  This link list is used to schedule events
 * for the simulation.
 *
 * Calling Arguments:
 *	linkhead = pointer to the data element that is going to be inserted
 */
CHAR **simals_insert_set_list(LINKPTR linkhead)
{
	CHAR **linkptr1;
	LINKPTR linkptr2;

	linkptr1 = (CHAR **)&simals_setroot;
	for(;;)
	{
		if ((*linkptr1) == 0)
		{
			*linkptr1 = (CHAR *)linkhead;
			break;
		}
		linkptr2 = (LINKPTR)*linkptr1;
		if (linkptr2->time > linkhead->time || (linkptr2->time == linkhead->time &&
			linkptr2->priority > linkhead->priority))
		{
			linkhead->right = linkptr2;
			*linkptr1 = (CHAR *)linkhead;
			break;
		}
		linkptr1 = (CHAR **)&(linkptr2->right);
	}
	return(linkptr1);
}

/*
 * Name: simals_insert_link_list
 *
 * Description:
 *	This procedure inserts a data element into a linklist that is 2
 * dimensionally sorted first by time and then priority.  This link list is
 * used to schedule events for the simulation.
 *
 * Calling Arguments:
 *	linkhead = pointer to the data element that is going to be inserted
 */
void simals_insert_link_list(LINKPTR linkhead)
{
	CHAR **linkptr1;
	LINKPTR linkptr2, linkptr3;

	linkptr1 = (CHAR **)&simals_linkback;
	linkptr2 = simals_linkback;
	linkptr3 = 0;
	for(;;)
	{
		if (! linkptr2)
		{
			simals_linkfront = (LINKPTR)(*linkptr1 = (CHAR *)linkhead);
			linkhead->left = 0;
			linkhead->right = linkptr3;
			linkhead->up = linkhead;
			linkhead->down = 0;
			return;
		}

		if (linkptr2->time < linkhead->time)
		{
			*linkptr1 = (CHAR *)(linkptr2->right = linkhead);
			linkhead->left = linkptr2;
			linkhead->right = linkptr3;
			linkhead->up = linkhead;
			linkhead->down = 0;
			return;
		}

		if (linkptr2->time == linkhead->time)
		{
			if (linkptr2->priority > linkhead->priority)
			{
				linkhead->left = linkptr2->left;
				linkhead->right = linkptr2->right;
				linkhead->down = linkptr2;
				linkhead->up = linkptr2->up;
				linkptr2->up = (LINKPTR)(*linkptr1 = (CHAR *)linkhead);
				if (linkhead->left)
				{
					linkhead->left->right = linkhead;
				} else
				{
					simals_linkfront = linkhead;
				}
				return;
			}

			linkptr1 = (CHAR **)&(linkptr2->up);
			linkptr2 = linkptr2->up;
			linkptr3 = 0;
			for(;;)
			{
				if (linkptr2->priority <= linkhead->priority)
				{
					*linkptr1 = (CHAR *)(linkptr2->down = linkhead);
					linkhead->up = linkptr2;
					linkhead->down = linkptr3;
					return;
				}

				linkptr3 = linkptr2;
				linkptr1 = (CHAR **)&(linkptr2->up);
				linkptr2 = linkptr2->up;
			}
		}

		linkptr3 = linkptr2;
		linkptr1 = (CHAR **)&(linkptr2->left);
		linkptr2 = linkptr2->left;
	}
}

#endif  /* SIMTOOL - at top */
