/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: simalsuser.c
 * Asynchronous Logic Simulator user functions
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

extern BOOLEAN simals_tracing;
extern CHAR *simals_statedesc[], *simals_strengthdesc[];

/* prototypes for local routines */
static BOOLEAN simals_schedule_node_update(MODPTR, EXPTR, UCHAR, INTBIG, INTSML, double);
static void   simals_calculate_bidir_outputs(MODPTR, EXPTR*, INTSML);
static BOOLEAN simals_bidir_path_search(MODPTR);
static void   simals_PMOSTRAN(MODPTR);
static void   simals_NMOSTRAN(MODPTR);
static void   simals_PMOSTRANWeak(MODPTR);
static void   simals_NMOSTRANWeak(MODPTR);
static void   simals_JK_FF(MODPTR);
static void   simals_DFFLOP(MODPTR);
static void   simals_BUS_TO_STATE(MODPTR);
static void   simals_STATE_TO_BUS(MODPTR);
#if 0			/* unused user functions */
static void   simals_Q_CALC(MODPTR);
static void   simals_STATS(MODPTR);
static void   simals_COUNTER(MODPTR);
static void   simals_DELAY_CALC(MODPTR);
static void   simals_FIFO(MODPTR);
static void   simals_RX_DATA(MODPTR);
static void   simals_A_F_REGISTERS(MODPTR);
static void   simals_CONTROL_LOGIC(MODPTR);
static void   simals_MOD2_ADDER(MODPTR);
static void   simals_ABOVE_ADDER(MODPTR);
static void   simals_WRITE_OUT(MODPTR);
static void   simals_BUS12_TO_STATE(MODPTR);
#endif

/*
 * Name: simals_get_function_address
 *
 * Description:
 *	This procedure returns the address of the function specified by
 * the calling argument character string.  Each time a routine is added to the
 * user library of event driven "C" functions an entry must be made into this
 * procedure to include it in the known list of user routines.  It is highly
 * recommended that all user defined procedures be named with capital letters.
 * This is done for two reasons: 1) the text in the netlist is converted to
 * upper case by the parser and this means caps must be used in the string
 * compare statements below, and 2) the user defined function routines will
 * be easily identifiable in source code listings.  Returns zero on error.
 *
 * Calling Arguments:
 *	s1 = pointer to character string containing the name of the user
 *	    defined function
 */
static struct
{
	CHAR  *name;
	void  (*address)(MODPTR);
} func_table[] =
{
	{x_("PMOStran"),       simals_PMOSTRAN},			/* pMOS transistor */
	{x_("nMOStran"),       simals_NMOSTRAN},			/* nMOS transistor */
	{x_("nMOStranWeak"),   simals_NMOSTRANWeak},		/* Weak nMOS transistor */
	{x_("pMOStranWeak"),   simals_PMOSTRANWeak},		/* Weak pMOS transistor */
	{x_("JKFFLOP"),        simals_JK_FF},				/* JK flip-flop */
	{x_("DFFLOP"),         simals_DFFLOP},				/* D flip-flop */
	{x_("BUS_TO_STATE"),   simals_BUS_TO_STATE},
	{x_("STATE_TO_BUS"),   simals_STATE_TO_BUS},
#if 0			/* unused user functions */
	{x_("Q_CALC"),         simals_Q_CALC},
	{x_("STATS"),          simals_STATS},
	{x_("COUNTER"),        simals_COUNTER},
	{x_("DELAY_CALC"),     simals_DELAY_CALC},
	{x_("FIFO"),           simals_FIFO},
	{x_("RX_DATA"),        simals_RX_DATA},
	{x_("A_F_REGISTERS"),  simals_A_F_REGISTERS},
	{x_("CONTROL_LOGIC"),  simals_CONTROL_LOGIC},
	{x_("MOD2_ADDER"),     simals_MOD2_ADDER},
	{x_("ABOVE_ADDER"),    simals_ABOVE_ADDER},
	{x_("WRITE_OUT"),      simals_WRITE_OUT},
	{x_("BUS12_TO_STATE"), simals_BUS12_TO_STATE},
#endif
	{NULL, NULL}  /* 0 */
};

/************************** SUPPORT ROUTINES **************************/

INTBIG *simals_get_function_address(CHAR *s1)
{
	INTBIG i;

	for (i = 0; func_table[i].name; ++i)
	{
		if (! namesame(s1, func_table[i].name))
			return((INTBIG *)func_table[i].address);
	}
	ttyputerr(_("ERROR: Unable to find user function %s in library"), s1);
	return(0);
}

/*
 * Name: simals_schedule_node_update
 *
 * Description:
 *	This procedure can be used to insert a node updating task into the
 * time scheduling link list.  The user must specify the node address, signal
 * state, signal strength, and update time in the calling arguments.  If the
 * user updates the value of a node manually without calling this procedure,
 * the event driving algorithm will not be able to detect this change.  This means
 * that the effects of the node update will not be propagated throughout the
 * simulation circuit automatically.  By scheduling the event through the master
 * link list, this problem can be avoided.  Returns true on error.
 *
 * Calling Arguments:
 *	nodehead = pointer to the node data structure to be updated
 *	operatr  = char indicating operation to be performed on node
 *	state    = integer value representing the state of the node
 *	strength = integer value representing the logic stregth of the signal
 *	time     = double value representing the time the change is to take place
 */
BOOLEAN simals_schedule_node_update(MODPTR primhead, EXPTR exhead, UCHAR operatr,
	INTBIG state, INTSML strength, double time)
{
	STATPTR stathead;
	LINKPTR linkptr2;
	CHAR s2[300];

	stathead = (STATPTR)exhead->node_name;
	if (stathead->sched_op == operatr && stathead->sched_state == state &&
		stathead->sched_strength == strength)
	{
		return(FALSE);
	}
	if (simals_tracing)
	{
		simals_compute_node_name(stathead->nodeptr, s2);
		ttyputmsg(_("      Schedule(F) gate %s%s, net %s  at %s"),
			stathead->primptr->name, stathead->primptr->level, s2,
				sim_windowconvertengineeringnotation(time));
	}
	linkptr2 = simals_alloc_link_mem();
	if (linkptr2 == 0) return(TRUE);
	linkptr2->type = 'G';
	linkptr2->ptr = (CHAR *)stathead;
	linkptr2->operatr = stathead->sched_op = operatr;
	linkptr2->state = stathead->sched_state = state;
	linkptr2->strength = stathead->sched_strength = strength;
	linkptr2->priority = 1;
	linkptr2->time = time;
	linkptr2->primhead = primhead;
	simals_insert_link_list(linkptr2);
	return(FALSE);
}

/*
 * Name: simals_calculate_bidir_outputs
 *
 * Description:
 *	This procedure examines all the elements feeding into a node that is
 * connected to the current bidirectional element and insures that there are no
 * bidirectional "loops" found in the network paths.  If a loop is found the
 * effects of the element are ignored from the node summing calculation.
 *
 * Calling Arguments:
 *	primhead    = pointer to current bidirectional element
 *	side[]      = pointers to nodes on each side of the bidir element
 *  outstrength = output strength
 */
static NODEPTR  simals_target_node;
static INTBIG   simals_bidirclock = 0;

#define FASTERSIM 1			/* new method prevents excessive graph searching */

void simals_calculate_bidir_outputs(MODPTR primhead, EXPTR *side, INTSML outstrength)
{
	INTBIG i, state, thisstate;
	INTSML strength, thisstrength;
	double time;
	NODEPTR sum_node;
	STATPTR stathead;
	EXPTR thisside, otherside;
	FUNCPTR funchead;

	for(i=0; i<2; i++)
	{
		thisside = side[i];
		otherside = side[(i+1)%2];
		sum_node = thisside->nodeptr;
		simals_target_node = otherside->nodeptr;
		if (simals_target_node == simals_drive_node) continue;
		state = sum_node->new_state;
		strength = sum_node->new_strength;

#ifdef FASTERSIM
		simals_bidirclock++;
#else
		sum_node->visit = 1;
#endif

		for(stathead = sum_node->statptr; stathead; stathead = stathead->next)
		{
			if (stathead->primptr == primhead) continue;

#ifdef FASTERSIM
			sum_node->visit = simals_bidirclock;
#endif
#if 0		/* hack to make it work (normally "1") */
			if (simals_bidir_path_search(stathead->primptr)) continue;
#endif

			thisstrength = stathead->new_strength;
			thisstate = stathead->new_state;

			if (thisstrength > strength)
			{
				state = thisstate;
				strength = thisstrength;
				continue;
			}

			if (thisstrength == strength)
			{
				if (thisstate != state) state = LOGIC_X;
			}
		}

#ifndef FASTERSIM
		sum_node->visit = 0;
#endif

		/* make strength no more than maximum output strength */
		if (strength > outstrength) strength = outstrength;

		funchead = (FUNCPTR)primhead->ptr;
		time = simals_time_abs + (funchead->delta * simals_target_node->load);
		(void)simals_schedule_node_update(primhead, otherside, '=',
			state, strength, time);
	}
}

/*
 * Name: simals_bidir_path_search
 *
 * Description:
 *	This procedure performs a walk around the network to check if there are
 * any bidir path loops.  This procedure is called recursively.  Each time a new
 * node is explored it is pushed onto the path list.  This list keeps track
 * of all the nodes visited during the current tree walk.
 *
 * Calling Arguments:
 *	primhead = pointer to current bidirectional element
 */
BOOLEAN simals_bidir_path_search(MODPTR primhead)
{
	BOOLEAN i = FALSE;
	FUNCPTR funchead;
	EXPTR sidea, ctl, sideb;
	NODEPTR end_node;
	LOADPTR pinhead;

	if (primhead->type != 'F') return(FALSE);
	funchead = (FUNCPTR)primhead->ptr;
	if (funchead->procptr != simals_NMOSTRAN && funchead->procptr != simals_PMOSTRAN)
		return(FALSE);

	sidea = primhead->exptr;
	ctl = sidea->next;
	sideb = ctl->next;

#ifdef FASTERSIM
	if (sidea->nodeptr->visit == simals_bidirclock)
#else
	if (sidea->nodeptr->visit)
#endif
	{
		end_node = sideb->nodeptr;
	} else
	{
#ifdef FASTERSIM
		if (sideb->nodeptr->visit == simals_bidirclock)
#else
		if (sideb->nodeptr->visit)
#endif
		{
			end_node = sidea->nodeptr;
		} else
		{
			return(FALSE);
		}
	}

	if (end_node == simals_target_node) return(TRUE);

	if ((funchead->procptr == simals_NMOSTRAN && ctl->nodeptr->sum_state == LOGIC_LOW) ||
		(funchead->procptr == simals_PMOSTRAN && ctl->nodeptr->sum_state == LOGIC_HIGH))
			return(FALSE);

#ifdef FASTERSIM
	if (end_node->visit == simals_bidirclock) return(FALSE);
	end_node->visit = simals_bidirclock;
#else
	if (end_node->visit) return(FALSE);
	end_node->visit = 1;
#endif

	for (pinhead = end_node->pinptr; pinhead; pinhead = pinhead->next)
	{
		if (pinhead->ptr == (CHAR *)primhead) continue;

		i = simals_bidir_path_search((MODPTR)pinhead->ptr);
		if (i) break;
	}
#ifndef FASTERSIM
	end_node->visit = 0;
#endif
	return(i);
}

/************************** USER FUNCTION ROUTINES **************************/

/*
 * Name: simals_PMOSTRAN
 *
 * Description:
 *	These procedures model a bidirectional p-channel transistor.  If the
 * routine is called, a new node summing calculation is performed and the effects
 * of the gate output on the node are ignored.  This algorithm is used in the
 * XEROX Aquarius simulation engine.
 *
 * Calling Arguments:
 *	primhead = pointer to a structure containing the calling arguments for
 *		 the user defined function.  The nodes are stored in the list
 *		 in the following order: ctl, sidea, sideb
 */
void simals_PMOSTRAN(MODPTR primhead)
{
	EXPTR ctl, side[2];

	ctl = primhead->exptr;
	side[0] = ctl->next;
	side[1] = side[0]->next;

	if (ctl->nodeptr->sum_state == LOGIC_HIGH)
	{
		(void)simals_schedule_node_update(primhead, side[0], '=',
			LOGIC_X, OFF_STRENGTH, simals_time_abs);
		(void)simals_schedule_node_update(primhead, side[1], '=',
			LOGIC_X, OFF_STRENGTH, simals_time_abs);
		return;
	}
	simals_calculate_bidir_outputs(primhead, side, GATE_STRENGTH);
}

void simals_PMOSTRANWeak(MODPTR primhead)
{
	EXPTR ctl, side[2];

	ctl = primhead->exptr;
	side[0] = ctl->next;
	side[1] = side[0]->next;

	if (ctl->nodeptr->sum_state == LOGIC_HIGH)
	{
		(void)simals_schedule_node_update(primhead, side[0], '=',
			LOGIC_X, OFF_STRENGTH, simals_time_abs);
		(void)simals_schedule_node_update(primhead, side[1], '=',
			LOGIC_X, OFF_STRENGTH, simals_time_abs);
		return;
	}
	simals_calculate_bidir_outputs(primhead, side, NODE_STRENGTH);
}

/*
 * Name: simals_NMOSTRAN
 *
 * Description:
 *	These procedures model a bidirectional n-channel transistor.  If the
 * routine is called, a new node summing calculation is performed and the effects
 * of the gate output on the node are ignored.  This algorithm is used in the
 * XEROX Aquarius simulation engine.
 *
 * Calling Arguments:
 *	primhead = pointer to a structure containing the calling arguments for
 *		 the user defined function.  The nodes are stored in the list
 *		 in the following order: ctl, sidea, sideb
 */
void simals_NMOSTRAN(MODPTR primhead)
{
	EXPTR ctl, side[2];

	ctl = primhead->exptr;
	side[0] = ctl->next;
	side[1] = side[0]->next;

	if (ctl->nodeptr->sum_state == LOGIC_LOW)
	{
		(void)simals_schedule_node_update(primhead, side[0], '=',
			LOGIC_X, OFF_STRENGTH, simals_time_abs);
		(void)simals_schedule_node_update(primhead, side[1], '=',
			LOGIC_X, OFF_STRENGTH, simals_time_abs);
		return;
	}
	simals_calculate_bidir_outputs(primhead, side, GATE_STRENGTH);
}

void simals_NMOSTRANWeak(MODPTR primhead)
{
	EXPTR ctl, side[2];

	ctl = primhead->exptr;
	side[0] = ctl->next;
	side[1] = side[0]->next;

	if (ctl->nodeptr->sum_state == LOGIC_LOW)
	{
		(void)simals_schedule_node_update(primhead, side[0], '=',
			LOGIC_X, OFF_STRENGTH, simals_time_abs);
		(void)simals_schedule_node_update(primhead, side[1], '=',
			LOGIC_X, OFF_STRENGTH, simals_time_abs);
		return;
	}
	simals_calculate_bidir_outputs(primhead, side, NODE_STRENGTH);
}

/*
 * Name: simals_JK_FF
 *
 * Description:
 *	This procedure fakes out the function of a JK flipflop.
 *
 * Calling Arguments:
 *	primhead = pointer to a structure containing the calling arguments for
 *		 the user defined function.  The nodes are stored in the list
 *		 in the following order: ck, j, k, q, qbar
 */
void simals_JK_FF(MODPTR primhead)
{
	INTBIG ck, j, k, out;
	EXPTR argptr, argptrbar;

	argptr = primhead->exptr;
	ck = argptr->nodeptr->sum_state;
	if (ck != LOGIC_LOW) return;

	argptr = argptr->next;
	j = argptr->nodeptr->sum_state;
	argptr = argptr->next;
	k = argptr->nodeptr->sum_state;
	argptr = argptr->next;
	argptrbar = argptr->next;

	if (j == LOGIC_LOW)
	{
		if (k == LOGIC_LOW) return;
		(void)simals_schedule_node_update(primhead, argptr, '=',
			LOGIC_LOW, GATE_STRENGTH, simals_time_abs);
		(void)simals_schedule_node_update(primhead, argptrbar, '=',
			LOGIC_HIGH, GATE_STRENGTH, simals_time_abs);
		return;
	}
	if (k == LOGIC_LOW)
	{
		(void)simals_schedule_node_update(primhead, argptr, '=',
			LOGIC_HIGH, GATE_STRENGTH, simals_time_abs);
		(void)simals_schedule_node_update(primhead, argptrbar, '=',
			LOGIC_LOW, GATE_STRENGTH, simals_time_abs);
		return;
	}

	out = argptr->nodeptr->sum_state;
	if (out == LOGIC_HIGH)
	{
		(void)simals_schedule_node_update(primhead, argptr, '=',
			LOGIC_LOW, GATE_STRENGTH, simals_time_abs);
		(void)simals_schedule_node_update(primhead, argptrbar, '=',
			LOGIC_HIGH, GATE_STRENGTH, simals_time_abs);
	} else
	{
		(void)simals_schedule_node_update(primhead, argptr, '=',
			LOGIC_HIGH, GATE_STRENGTH, simals_time_abs);
		(void)simals_schedule_node_update(primhead, argptrbar, '=',
			LOGIC_LOW, GATE_STRENGTH, simals_time_abs);
	}
}

/*
 * Name: simals_DFFLOP
 *
 * Description:
 *	This procedure mimics the function of a D flip flop.
 *
 * Arguments:
 *	primhead = pointer to a structure containing the
 *		 calling arguments for the user function.
 *		The nodes are expected to appear in the
 *		 parameter list in the following order:
 *
 *		 data_in, clk, q
 */
void simals_DFFLOP(MODPTR primhead)
{
	INTBIG d_in, clk, q;
	EXPTR argptr;

	argptr = primhead->exptr;

	/* data_in signal now selected */
	d_in = argptr->nodeptr->sum_state;
	argptr = argptr->next;

	/* clk signal now selected */
	clk = argptr->nodeptr->sum_state;

	/* do nothing if not a +ve clock edge */
	if (clk != LOGIC_LOW) return;

	/* If this part of the procedure has been reached, the
		data_out signal should be updated since clk is high.
		Therefore, the value of data_out should be set to
		the value of d_in (one of LOGIC_LOW, LOGIC_HIGH,
		or LOGIC_X). */

	/* select data_out signal */
	argptr = argptr->next;
	q = d_in;
	(void)simals_schedule_node_update(primhead, argptr, '=',
		q, GATE_STRENGTH, simals_time_abs);
}

/*
 * Name: simals_BUS_TO_STATE
 *
 * Description:
 *	This procedure converts the value of 8 input bits into a state
 * representation in the range 0x00-0xff.  This function can be called for
 * the compact representation of the state of a bus structure in hexadecimal
 * format.
 *
 * Calling Arguments:
 *	primhead = pointer to a structure containing the calling arguments for
 *		 the user defined function.  The nodes are ordered b7, b6, b5,
 *		 b4, b3, b2, b1, b0, output in this list.
 */
void simals_BUS_TO_STATE(MODPTR primhead)
{
	INTBIG i, bit, state;
	EXPTR argptr;

	argptr = primhead->exptr;
	state = 0;
	for (i = 7; i > -1; --i)
	{
		bit = argptr->nodeptr->sum_state;
		if (bit == LOGIC_HIGH) state += (0x01 << i);
		argptr = argptr->next;
	}
	(void)simals_schedule_node_update(primhead, argptr, '=',
		state, VDD_STRENGTH, simals_time_abs);
}

/*
 * Name: simals_STATE_TO_BUS
 *
 * Description:
 *	This procedure converts a value in the range of 0x00-0xff into an 8 bit
 * logic representation (logic L = -1 and H = -3).  This function can be called to
 * provide an easy means of representing bus values with a single integer entry.
 *
 * Calling Arguments:
 *	primhead = pointer to a structure containing the calling arguments for
 *		 the user defined function.  The nodes are stored in the list
 *		 in the following order: input, b7, b6, b5, b4, b3, b2, b1, b0
 */
void simals_STATE_TO_BUS(MODPTR primhead)
{
	INTBIG i, input, mask;
	EXPTR argptr;

	argptr = primhead->exptr;
	input = argptr->nodeptr->sum_state;

	for (i = 7; i > -1; --i)
	{
		argptr = argptr->next;

		mask = (0x01 << i);
		if (input & mask)
		{
			(void)simals_schedule_node_update(primhead, argptr, '=',
				LOGIC_HIGH, VDD_STRENGTH, simals_time_abs);
		} else
		{
			(void)simals_schedule_node_update(primhead, argptr, '=',
				LOGIC_LOW, VDD_STRENGTH, simals_time_abs);
		}
	}
}

/************************** UNUSED USER FUNCTION ROUTINES **************************/

#if 0			/* special case simulation elements */
/*
 * Name: simals_Q_CALC
 *
 * Description:
 *	This procedure is an event driven function which is called to calculate
 * statistics for nodes which are defined as queues.  Statistics are calculated
 * for the maximum number of tokens present, the number of tokens arriving
 * and departing, the time of last access, and the cumulative number of token
 * seconds.  These statistics are used to generate data regarding the average
 * number of tokens present in the queue and the amount of delay a token
 * experiences going through the queues in a simulation run.
 *
 * Calling Arguments:
 *	primhead = pointer to a structure containing the calling arguments for
 *		 the user defined function.  The nodes are stored in the list
 *		 in the following order: q_node
 */
void simals_Q_CALC(MODPTR primhead)
{
	EXPTR argptr;
	NODEPTR nodehead;
	FUNCPTR funchead;

	argptr = primhead->exptr;
	nodehead = argptr->nodeptr;
	funchead = (FUNCPTR) primhead->ptr;
	if (! nodehead->maxsize)
	{
		funchead->userint = 0;
		funchead->userfloat = 0.0;
	}

	nodehead->tk_sec += (nodehead->t_last - funchead->userfloat) * funchead->userint;
	if (nodehead->sum_state > funchead->userint)
	{
		nodehead->arrive += nodehead->sum_state - funchead->userint;
		if (nodehead->sum_state > nodehead->maxsize)
			nodehead->maxsize = nodehead->sum_state;
	} else
	{
		nodehead->depart += funchead->userint - nodehead->sum_state;
	}

	funchead->userint = nodehead->sum_state;
	funchead->userfloat = nodehead->t_last;
}

/*
 * Name: simals_STATS
 *
 * Description:
 *	This procedure is an event driven function which is called to summarize
 * the number of characters arriving into a station.
 *
 * Calling Arguments:
 *	primhead = pointer to a structure containing the calling arguments for
 *		 the user defined function.  The nodes are stored in the list
 *		 in the following order: size
 */
void simals_STATS(MODPTR primhead)
{
	EXPTR argptr;

	argptr = primhead->exptr;
	if (argptr->nodeptr->sum_state % 100) return;

	ttyputmsg(M_("Characters arriving into Station (N%ld) = %ld, Time = %g"),
		argptr->nodeptr->num, argptr->nodeptr->sum_state, simals_time_abs);
}

/*
 * Name: simals_COUNTER
 *
 * Description:
 *	This procedure fakes out the function of a synchronous reset counter.
 *
 * Calling Arguments:
 *	primhead = pointer to a structure containing the calling arguments for
 *		 the user defined function.  The nodes are stored in the list
 *		 in the following order: ck, reset, count, out
 */
void simals_COUNTER(MODPTR primhead)
{
	INTBIG ck, reset, count, out;
	EXPTR argptr, countptr;

	argptr = primhead->exptr;
	ck = argptr->nodeptr->sum_state;
	if (ck != LOGIC_HIGH) return;

	argptr = argptr->next;
	reset = argptr->nodeptr->sum_state;
	argptr = argptr->next;
	count = argptr->nodeptr->sum_state;
	countptr = argptr;
	argptr = argptr->next;
	out = argptr->nodeptr->sum_state;

	if (reset == LOGIC_LOW)
	{
		(void)simals_schedule_node_update(primhead, countptr, '=',
			0, GATE_STRENGTH, simals_time_abs + 30e-9);
		(void)simals_schedule_node_update(primhead, argptr, '=',
			LOGIC_LOW, GATE_STRENGTH, simals_time_abs + 30e-9);
		return;
	}

	count = (count + 1) % 16;
	(void)simals_schedule_node_update(primhead, countptr, '=',
		count, GATE_STRENGTH, simals_time_abs + 20e-9);

	if (count == 15)
	{
		(void)simals_schedule_node_update(primhead, argptr, '=',
			LOGIC_HIGH, GATE_STRENGTH, simals_time_abs + 18e-9);
	} else
	{
		(void)simals_schedule_node_update(primhead, argptr, '=',
			LOGIC_LOW, GATE_STRENGTH, simals_time_abs + 22e-9);
	}
}

/*
 * Name: simals_DELAY_CALC
 *
 * Description:
 *	This procedure is an event driven function which is called to calculate
 * the binary exponential backoff time delay for an Ethernet system.
 *
 * Calling Arguments:
 *	primhead = pointer to a structure containing the calling arguments for
 *		 the user defined function.  The nodes are stored in the list
 *		 in the following order: retx, server
 */
void simals_DELAY_CALC(MODPTR primhead)
{
	double delay, base_delay = 10e-6;
	EXPTR retx, server;

	retx = primhead->exptr;
	server = retx->next;

	if (retx->nodeptr->sum_state == 0) return;

	delay = 2.0 * (rand() / MAXINTBIG) * base_delay *
		(1 << (retx->nodeptr->sum_state - 1)) + simals_time_abs;
	(void)simals_schedule_node_update(primhead, server, '=',
		1, VDD_STRENGTH, delay);
	(void)simals_schedule_node_update(primhead, server, '=',
		LOGIC_X, OFF_STRENGTH, delay);
}

/*
 * Name: simals_FIFO
 *
 * Description:
 *	This procedure simulates the operation of a FIFO element in a data
 * communication system.  Data is stored in a block of memory of fixed
 * size.  Read and write counters are used to index the memory array.  This
 * approach was chosen because this structure will simulate much faster than a
 * linked list.
 *
 * Calling Arguments:
 *	primhead = pointer to a structure containing the calling arguments for
 *		 the user defined function.  The nodes are stored in the list
 *		 in the following order: input, output, size
 */
#define FIFO_SIZE  2048

void simals_FIFO(MODPTR primhead)
{
	typedef struct fifo_ds
	{
		INTBIG r_ptr;
		INTBIG w_ptr;
		INTBIG state[FIFO_SIZE];
	} FIFO_MEM;
	typedef FIFO_MEM *FIFOPTR;
	INTBIG new_size;
	EXPTR argptr, input_ptr, output_ptr, size_ptr;
	FUNCPTR funchead;
	FIFOPTR fifohead;

	argptr = primhead->exptr;
	input_ptr = argptr;
	argptr = argptr->next;
	output_ptr = argptr;
	argptr = argptr->next;
	size_ptr = argptr;

	funchead = (FUNCPTR) primhead->ptr;
	fifohead = (FIFOPTR) funchead->userptr;
	if (! fifohead)
	{
		fifohead = (FIFOPTR) simals_alloc_mem((INTBIG)sizeof(FIFO_MEM));
		if (fifohead == 0) return;
		fifohead->r_ptr = 0;
		fifohead->w_ptr = 0;
		funchead->userptr = (CHAR *)fifohead;
	}

	if (input_ptr->nodeptr->sum_state > 0)
	{
		(void)simals_schedule_node_update(primhead, input_ptr, '=',
			0, VDD_STRENGTH, simals_time_abs);
		(void)simals_schedule_node_update(primhead, input_ptr, '=',
			LOGIC_X, OFF_STRENGTH, simals_time_abs);
		if (size_ptr->nodeptr->sum_state >= FIFO_SIZE)
		{
			ttyputerr(M_("Data loss has occured: Value = %ld, Time = %g"),
			input_ptr->nodeptr->sum_state, simals_time_abs);
			simals_window_max = simals_time_abs;
			return;
		}
		new_size = size_ptr->nodeptr->sum_state + 1;
		(void)simals_schedule_node_update(primhead, size_ptr, '=',
			new_size, VDD_STRENGTH, simals_time_abs);
		(void)simals_schedule_node_update(primhead, size_ptr, '=',
			LOGIC_X, OFF_STRENGTH, simals_time_abs);
		fifohead->state[fifohead->w_ptr] = input_ptr->nodeptr->sum_state;
		fifohead->w_ptr = ((fifohead->w_ptr) + 1) % FIFO_SIZE;
	}

	if ((output_ptr->nodeptr->sum_state == 0) && (size_ptr->nodeptr->sum_state))
	{
		(void)simals_schedule_node_update(primhead, output_ptr, '=',
			fifohead->state[fifohead->r_ptr], VDD_STRENGTH, simals_time_abs);
		(void)simals_schedule_node_update(primhead, output_ptr, '=',
			LOGIC_X, OFF_STRENGTH, simals_time_abs);
		new_size = size_ptr->nodeptr->sum_state - 1;
		(void)simals_schedule_node_update(primhead, size_ptr, '=',
			new_size, VDD_STRENGTH, simals_time_abs);
		(void)simals_schedule_node_update(primhead, size_ptr, '=',
			LOGIC_X, OFF_STRENGTH, simals_time_abs);
		fifohead->r_ptr = ((fifohead->r_ptr) + 1) % FIFO_SIZE;
	}
}

/*
 * Name: simals_RX_DATA
 *
 * Description:
 *	This procedure examines the bus to see if a message is destined for it.
 * If so, the data value is examined to determine if the link that the data byte
 * passed across is in a "congested" state.  If so this element must fire an
 * XOFF character to the station on the remote side of the connection.  The
 * data value is also passed to a gate which will examine the byte and increment
 * the appropriate counter.
 *
 * Calling Arguments:
 *	primhead = pointer to a structure containing the calling arguments for
 *		 the user defined function.  The nodes are stored in the list
 *		 in the following order: address, add_in, data_in, data_type,
 *					 rem_xoff, add_out, data_out
 */
void simals_RX_DATA(MODPTR primhead)
{
	EXPTR address, add_in, data_in, data_type, rem_xoff, add_out, data_out;

	address = primhead->exptr;
	add_in = address->next;
	if (address->nodeptr->sum_state != add_in->nodeptr->sum_state) return;
	data_in = add_in->next;
	data_type = data_in->next;
	rem_xoff = data_type->next;
	add_out = rem_xoff->next;
	data_out = add_out->next;

	(void)simals_schedule_node_update(primhead, add_in, '=',
		0, VDD_STRENGTH, simals_time_abs);
	(void)simals_schedule_node_update(primhead, add_in, '=',
		LOGIC_X, OFF_STRENGTH, simals_time_abs);
	(void)simals_schedule_node_update(primhead, data_in, '=',
		0, VDD_STRENGTH, simals_time_abs);
	(void)simals_schedule_node_update(primhead, data_in, '=',
		LOGIC_X, OFF_STRENGTH, simals_time_abs);
	if ((data_in->nodeptr->sum_state) % 2)
	{
		(void)simals_schedule_node_update(primhead, data_type, '=',
			data_in->nodeptr->sum_state, VDD_STRENGTH, simals_time_abs);
		(void)simals_schedule_node_update(primhead, data_type, '=',
			LOGIC_X, OFF_STRENGTH, simals_time_abs);
		if (rem_xoff->nodeptr->sum_state)
		{
			(void)simals_schedule_node_update(primhead, rem_xoff, '=',
				0, VDD_STRENGTH, simals_time_abs);
			(void)simals_schedule_node_update(primhead, rem_xoff, '=',
				LOGIC_X, OFF_STRENGTH, simals_time_abs);
			(void)simals_schedule_node_update(primhead, add_out, '=',
				address->nodeptr->sum_state, VDD_STRENGTH, simals_time_abs + 50e-6);
			(void)simals_schedule_node_update(primhead, add_out, '=',
				LOGIC_X, OFF_STRENGTH, simals_time_abs + 50e-6);
			(void)simals_schedule_node_update(primhead, data_out, '=',
				5, VDD_STRENGTH, simals_time_abs + 50e-6);
			(void)simals_schedule_node_update(primhead, data_out, '=',
				LOGIC_X, OFF_STRENGTH, simals_time_abs + 50e-6);
		}
	} else
	{
		(void)simals_schedule_node_update(primhead, data_type, '=',
			((data_in->nodeptr->sum_state) - 1), VDD_STRENGTH, simals_time_abs);
		(void)simals_schedule_node_update(primhead, data_type, '=',
			LOGIC_X, OFF_STRENGTH, simals_time_abs);
		if (! (rem_xoff->nodeptr->sum_state))
		{
			(void)simals_schedule_node_update(primhead, rem_xoff, '=',
				1, VDD_STRENGTH, simals_time_abs);
			(void)simals_schedule_node_update(primhead, rem_xoff, '=',
				LOGIC_X, OFF_STRENGTH, simals_time_abs);
			(void)simals_schedule_node_update(primhead, add_out, '=',
				address->nodeptr->sum_state, VDD_STRENGTH, simals_time_abs + 50e-6);
			(void)simals_schedule_node_update(primhead, add_out, '=',
				LOGIC_X, OFF_STRENGTH, simals_time_abs + 50e-6);
			(void)simals_schedule_node_update(primhead, data_out, '=',
				7, VDD_STRENGTH, simals_time_abs + 50e-6);
			(void)simals_schedule_node_update(primhead, data_out, '=',
				LOGIC_X, OFF_STRENGTH, simals_time_abs + 50e-6);
		}
	}
}

/*
 * Name: simals_A_F_REGISTERS
 */
void simals_A_F_REGISTERS(MODPTR primhead)
{
	EXPTR ck, ain, aload, fin, fload, amid, aout, fmid, fout;

	ck = primhead->exptr;
	ain = ck->next;
	aload = ain->next;
	fin = aload->next;
	fload = fin->next;
	amid = fload->next;
	aout = amid->next;
	fmid = aout->next;
	fout = fmid->next;

	if (ck->nodeptr->sum_state == LOGIC_HIGH)
	{
		if (aload->nodeptr->sum_state == LOGIC_HIGH)
		{
			(void)simals_schedule_node_update(amid, '=',
				ain->nodeptr->sum_state, GATE_STRENGTH, simals_time_abs + 3.0e-9);
		}
		if (fload->nodeptr->sum_state == LOGIC_HIGH)
		{
			(void)simals_schedule_node_update(fmid, '=',
				fin->nodeptr->sum_state, GATE_STRENGTH, simals_time_abs + 3.0e-9);
		}
		return;
	}

	if (ck->nodeptr->sum_state == LOGIC_LOW)
	{
		if (aload->nodeptr->sum_state == LOGIC_HIGH)
		{
			(void)simals_schedule_node_update(aout, '=',
				amid->nodeptr->sum_state, GATE_STRENGTH, simals_time_abs + 3.0e-9);
		}
		if (fload->nodeptr->sum_state == LOGIC_HIGH)
		{
			(void)simals_schedule_node_update(fout, '=',
				fmid->nodeptr->sum_state, GATE_STRENGTH, simals_time_abs + 3.0e-9);
		}
		return;
	}

	(void)simals_schedule_node_update(amid, '=',
		LOGIC_X, GATE_STRENGTH, simals_time_abs + 3.0e-9);
	(void)simals_schedule_node_update(aout, '=',
		LOGIC_X, GATE_STRENGTH, simals_time_abs + 3.0e-9);
	(void)simals_schedule_node_update(fmid, '=',
		LOGIC_X, GATE_STRENGTH, simals_time_abs + 3.0e-9);
	(void)simals_schedule_node_update(aout, '=',
		LOGIC_X, GATE_STRENGTH, simals_time_abs + 3.0e-9);
}

/*
 * Name: simals_CONTROL_LOGIC
 */
void simals_CONTROL_LOGIC(MODPTR primhead)
{
	EXPTR ain, fin, b, msb, aout, fout;

	ain = primhead->exptr;
	fin = ain->next;
	b = fin->next;
	msb = b->next;
	aout = msb->next;
	fout = aout->next;

	if (b->nodeptr->sum_state == LOGIC_HIGH)
	{
		(void)simals_schedule_node_update(aout, '=',
			ain->nodeptr->sum_state, GATE_STRENGTH, simals_time_abs + 3.0e-9);
	} else
	{
		 if (b->nodeptr->sum_state == LOGIC_LOW)
		 {
			(void)simals_schedule_node_update(aout, '=',
				LOGIC_LOW, GATE_STRENGTH, simals_time_abs + 3.0e-9);
		 } else
		 {
			(void)simals_schedule_node_update(aout, '=',
				LOGIC_X, GATE_STRENGTH, simals_time_abs + 3.0e-9);
		 }
	}

	if (msb->nodeptr->sum_state == LOGIC_HIGH)
	{
		(void)simals_schedule_node_update(fout, '=',
			fin->nodeptr->sum_state, GATE_STRENGTH, simals_time_abs + 3.0e-9);
	} else
	{
		if (msb->nodeptr->sum_state == LOGIC_LOW)
		{
			(void)simals_schedule_node_update(fout, '=',
				LOGIC_LOW, GATE_STRENGTH, simals_time_abs + 3.0e-9);
		} else
		{
			(void)simals_schedule_node_update(fout, '=',
				LOGIC_X, GATE_STRENGTH, simals_time_abs + 3.0e-9);
		}
	}
}

/*
 * Name: simals_MOD2_ADDER
 */
void simals_MOD2_ADDER(MODPTR primhead)
{
	INTBIG sum = 0;
	EXPTR ain, fin, pin, ck, out;

	ain = primhead->exptr;
	fin = ain->next;
	pin = fin->next;
	ck = pin->next;
	out = ck->next;

	if (ck->nodeptr->sum_state == LOGIC_LOW)
	{
		(void)simals_schedule_node_update(out, '=',
			LOGIC_LOW, GATE_STRENGTH, simals_time_abs + 3.0e-9);
		return;
	}

	if (ck->nodeptr->sum_state == LOGIC_HIGH)
	{
		if (ain->nodeptr->sum_state == LOGIC_HIGH) ++sum;
		if (fin->nodeptr->sum_state == LOGIC_HIGH) ++sum;
		if (pin->nodeptr->sum_state == LOGIC_HIGH) ++sum;

		sum %= 2;
		if (sum)
		{
			(void)simals_schedule_node_update(out, '=',
				LOGIC_HIGH, GATE_STRENGTH, simals_time_abs + 5.0e-9);
		} else
		{
			(void)simals_schedule_node_update(out, '=',
				LOGIC_LOW, GATE_STRENGTH, simals_time_abs + 5.0e-9);
		}
		return;
	}

	(void)simals_schedule_node_update(out, '=',
		LOGIC_X, GATE_STRENGTH, simals_time_abs + 5.0e-9);
}

/*
 * Name: simals_ABOVE_ADDER
 */
void simals_ABOVE_ADDER(MODPTR primhead)
{
	EXPTR ck, sync, load_osr, sum_in, osr_in, osr_mid, osr_out, pmid, pout;

	ck = primhead->exptr;
	sync = ck->next;
	load_osr = sync->next;
	sum_in = load_osr->next;
	osr_in = sum_in->next;
	osr_mid = osr_in->next;
	osr_out = osr_mid->next;
	pmid = osr_out->next;
	pout = pmid->next;

	if (ck->nodeptr->sum_state == LOGIC_HIGH)
	{
		if (load_osr->nodeptr->sum_state == LOGIC_LOW)
		{
			(void)simals_schedule_node_update(pmid, '=',
				sum_in->nodeptr->sum_state, GATE_STRENGTH, simals_time_abs + 3.0e-9);
			(void)simals_schedule_node_update(osr_mid, '=',
				osr_in->nodeptr->sum_state, GATE_STRENGTH, simals_time_abs + 3.0e-9);
		}
		if (load_osr->nodeptr->sum_state == LOGIC_HIGH)
		{
			(void)simals_schedule_node_update(osr_mid, '=',
				sum_in->nodeptr->sum_state, GATE_STRENGTH, simals_time_abs + 3.0e-9);
		}
		if (sync->nodeptr->sum_state == LOGIC_HIGH)
		{
			(void)simals_schedule_node_update(pmid, '=',
				LOGIC_LOW, GATE_STRENGTH, simals_time_abs + 3.0e-9);
		}
		return;
	}

	if (ck->nodeptr->sum_state == LOGIC_LOW)
	{
		if (load_osr->nodeptr->sum_state == LOGIC_LOW)
		{
			(void)simals_schedule_node_update(osr_out, '=',
				osr_mid->nodeptr->sum_state, GATE_STRENGTH, simals_time_abs + 3.0e-9);
			(void)simals_schedule_node_update(pout, '=',
				pmid->nodeptr->sum_state, GATE_STRENGTH, simals_time_abs + 3.0e-9);
		}
		return;
	}

	(void)simals_schedule_node_update(osr_mid, '=',
		LOGIC_X, GATE_STRENGTH, simals_time_abs + 3.0e-9);
	(void)simals_schedule_node_update(osr_out, '=',
		LOGIC_X, GATE_STRENGTH, simals_time_abs + 3.0e-9);
	(void)simals_schedule_node_update(pmid, '=',
		LOGIC_X, GATE_STRENGTH, simals_time_abs + 3.0e-9);
	(void)simals_schedule_node_update(pout, '=',
		LOGIC_X, GATE_STRENGTH, simals_time_abs + 3.0e-9);
}

/*
 * Name: simals_WRITE_OUT
 *
 * This routine is used to output the current timestamp and the 8bit value passed
 * to it.
 *
 * calling arguments:
 *	primhead = pointer to a structure containing the calling args for
 *		   this function. The args are in the following order:
 *		 current state.
 */
void simals_WRITE_OUT(MODPTR primhead)
{
	INTBIG state;
	CHAR *truename
	FILE *fp;
	EXPTR ptr;

	fp = xcreate(x_("als.out"), el_filetypetext, M_("Simulation Trace File"), &truename);
	if (fp == 0)
	{
		if (truename != 0) ttyputmsg(M_("ERROR: cannot open %s"), truename);
		return;
	}

	ptr = primhead->exptr;
	state = ptr->nodeptr->sum_state;
	xprintf(fp, x_("time: %e ; node: %s ; state: %ld"), simals_time_abs, ptr->node_name, state);
	xclose(fp);
}

/*
 * Name: simals_BUS12_TO_STATE
 *
 * Description:
 *	This procedure converts the value of 12 input bits into a state
 * representation in the range.  This function can be called for
 * the compact representation of the state of a bus structure in hexadecimal
 * format.
 *
 * Calling Arguments:
 *	primhead = pointer to a structure containing the calling arguments for
 *		 the user defined function.  The nodes are ordered b7, b6, b5,
 *		 b4, b3, b2, b1, b0, output in this list.
 */
void simals_BUS12_TO_STATE(MODPTR primhead)
{
	INTBIG i, bit, state;
	EXPTR argptr;

	argptr = primhead->exptr;
	state = 0;
	for (i = 11; i > -1; --i)
	{
		bit = argptr->nodeptr->sum_state;
		if (bit == LOGIC_HIGH) state += (0x01 << i);
		argptr = argptr->next;
	}

	(void)simals_schedule_node_update(argptr, '=',
		state, VDD_STRENGTH, simals_time_abs);
}
#endif

#endif  /* SIMTOOL - at top */
