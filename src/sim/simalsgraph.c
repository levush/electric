/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: simalsgraph.c
 * Asynchronous Logic Simulator graphics interface
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
#include "usr.h"
#include "network.h"
#include "edialogs.h"

static INTBIG  sim_window_iter;
static BOOLEAN sim_als_wantweak = FALSE, sim_als_wantstrong = FALSE, sim_als_wantfull = FALSE;

/* prototypes for local routines */
static BOOLEAN  simals_topofsignals(CHAR**);
static CHAR    *simals_nextsignals(void);
static INTBIG   simals_count_channels(CONPTR);
static void     simals_helpwindow(BOOLEAN waveform);
static int      simals_sortbusnames(const void *e1, const void *e2);

BOOLEAN simals_topofsignals(CHAR **c)
{
	Q_UNUSED( c );
	sim_window_iter = 0;
	return(TRUE);
}

CHAR *simals_nextsignals(void)
{
	CHAR *nextname;

	for(; ; sim_window_iter++)
	{
		if (sim_window_iter >= simals_levelptr->num_chn) return(0);
		if (simals_levelptr->display_page[sim_window_iter+1].nodeptr == 0) continue;
		if (simals_levelptr->display_page[sim_window_iter+1].displayptr == 0) break;
	}
	nextname = simals_levelptr->display_page[sim_window_iter+1].name;
	sim_window_iter++;
	return(nextname);
}

static COMCOMP sim_window_pickalstrace = {NOKEYWORD, simals_topofsignals,
	simals_nextsignals, NOPARAMS, 0, x_(" \t"), N_("pick a signal to display"), x_("")};

/*
 * Routine to start ALS simulation of cell "np".
 */
BOOLEAN simals_startsimulation(NODEPROTO *np)
{
	extern TOOL *vhdl_tool;
	REGISTER INTBIG error;

	/* initialize memory */
	simals_init();

	/* demand an ALS netlist */
	error = asktool(vhdl_tool, x_("want-netlist-input"), (INTBIG)np, sim_filetypeals);
	if (error != 0) return(TRUE);

	/* read netlist */
	simals_erase_model();
	if (simals_read_net_desc(np)) return(TRUE);
	if (simals_flatten_network()) return(TRUE);

	if (simals_title != 0)
	{
		efree((CHAR *)simals_title);
		simals_title = 0;
	}

	/* initialize display */
	simals_init_display();
	return(FALSE);
}

/*
 * The character handler for the waveform window of ALS simulation
 */
BOOLEAN simals_charhandlerwave(WINDOWPART *w, INTSML chr, INTBIG special)
{
	REGISTER INTBIG i, j, traces, pos, thispos, strength, *tracelist,
		tr, trl, nexttr, prevtr, highsig, *highsigs, numbits, bitoffset;
	INTBIG state, *theBits;
	CHAR *par[30], *name;
	NODEPTR node, nodehead;
	NODEPROTO *np;
	double endtime;
	LINKPTR sethead;

	ttynewcommand();

	/* special characters are not handled here */
	if (special != 0)
		return(us_charhandler(w, chr, special));

	/* can always preserve snapshot */
	if (chr == 'p')
	{
		sim_window_savegraph();
		sim_als_wantweak = sim_als_wantstrong = sim_als_wantfull = FALSE;
		return(FALSE);
	}

	/* can always do help */
	if (chr == '?')
	{
		simals_helpwindow(TRUE);
		sim_als_wantweak = sim_als_wantstrong = sim_als_wantfull = FALSE;
		return(FALSE);
	}

	/* if not simulating, don't handle any simulation commands */
	if (sim_window_isactive(&np) == 0)
		return(us_charhandler(w, chr, special));

	numbits = 0;
	switch (chr)
	{
		/* update display */
		case 'u':
			endtime = simals_initialize_simulator(TRUE);
			if ((sim_window_state&ADVANCETIME) != 0) sim_window_setmaincursor(endtime);
			return(FALSE);

		/* convert busses */
		case 'b':
			(void)sim_window_buscommand();
			return(FALSE);

		/* add trace */
		case 'a':
			i = ttygetparam(_("Signal to add"), &sim_window_pickalstrace, 3, par);
			if (i == 0) return(FALSE);

			/* find the signal */
			for(i=0; i<simals_levelptr->num_chn; i++)
			{
				node = simals_levelptr->display_page[i+1].nodeptr;
				if (node == 0) continue;
				if (simals_levelptr->display_page[i+1].displayptr != 0) continue;
				name = simals_levelptr->display_page[i+1].name;
				if (namesame(name, par[0]) == 0) break;
			}
			if (i >= simals_levelptr->num_chn) return(FALSE);

			/* ready to add: remove highlighting */
			sim_window_cleartracehighlight();

			/* count the number of traces */
			sim_window_inittraceloop();
			for(traces=0; ; traces++) if (sim_window_nexttraceloop() == 0) break;

			/* if there are no traces, put new trace on line zero */
			if (traces == 0) j = 0; else
			{
				/* other traces exist, make a new line in the plot */
				j = sim_window_getnumframes();
				sim_window_setnumframes(j+1);
			}

			/* create a new trace in the last slot */
			tr = sim_window_newtrace(j, simals_levelptr->display_page[i+1].name,
				(INTBIG)simals_levelptr->display_page[i+1].nodeptr);
			simals_levelptr->display_page[i+1].displayptr = tr;
			simals_fill_display_arrays();
			sim_window_redraw();
			sim_window_addhighlighttrace(tr);
			sim_als_wantweak = sim_als_wantstrong = sim_als_wantfull = FALSE;
			return(FALSE);

		/* handle weak and strong prefixes */
		case 'w':
			if (sim_als_wantweak || sim_als_wantstrong) ttybeep(SOUNDBEEP, FALSE);
			sim_als_wantweak = TRUE;
			return(FALSE);
		case 's':
			if (sim_als_wantweak || sim_als_wantstrong) ttybeep(SOUNDBEEP, FALSE);
			sim_als_wantstrong = TRUE;
			return(FALSE);
		case 'f':
			if (sim_als_wantfull) ttybeep(SOUNDBEEP, FALSE);
			sim_als_wantfull = TRUE;
			return(FALSE);

		/* different flavors of low values */
		case '0':
		case 'l':
			state = LOGIC_LOW;    strength = GATE_STRENGTH;
			if (sim_als_wantweak) strength = NODE_STRENGTH; else
				if (sim_als_wantstrong) strength = VDD_STRENGTH;
			break;

		/* different flavors of high values */
		case '1':
		case 'h':
			state = LOGIC_HIGH;   strength = GATE_STRENGTH;
			if (sim_als_wantweak) strength = NODE_STRENGTH; else
				if (sim_als_wantstrong) strength = VDD_STRENGTH;
			break;

		/* different flavors of undefined values */
		case 'x':
			state = LOGIC_X;      strength = GATE_STRENGTH;
			if (sim_als_wantweak) strength = NODE_STRENGTH; else
				if (sim_als_wantstrong) strength = VDD_STRENGTH;
			break;

		/* different flavors of wide numeric values */
		case 'v':
			numbits = sim_window_getwidevalue(&theBits);
			if (numbits < 0) return(FALSE);
			strength = GATE_STRENGTH;
			if (sim_als_wantweak) strength = NODE_STRENGTH; else
				if (sim_als_wantstrong) strength = VDD_STRENGTH;
			break;

		/* signal tracing */
		case 't':
			if (sim_als_wantfull)
			{
				simals_trace_all_nodes = TRUE;
				(void)simals_initialize_simulator(TRUE);
				simals_trace_all_nodes = FALSE;
			} else
			{
				highsigs = sim_window_gethighlighttraces();
				if (highsigs[0] == 0)
				{
					ttyputerr(_("Select signal names first"));
					return(FALSE);
				}
				for(i=0; highsigs[i] != 0; i++)
					((NODEPTR)sim_window_gettracedata(highsigs[i]))->tracenode = TRUE;
				(void)simals_initialize_simulator(TRUE);
				for(i=0; highsigs[i] != 0; i++)
					((NODEPTR)sim_window_gettracedata(highsigs[i]))->tracenode = FALSE;
			}
			return(FALSE);

		/* signal clock setting, info, erasing, removing (all handled later) */
		case 'c':
		case 'i':
		case 'e':
		case 'r':
		case DELETEKEY:
			strength = OFF_STRENGTH;
			break;

		default:
			sim_als_wantweak = sim_als_wantstrong = sim_als_wantfull = FALSE;
			return(us_charhandler(w, chr, special));
	}
	sim_als_wantweak = sim_als_wantstrong = sim_als_wantfull = FALSE;

	/* the following commands demand a current trace...get it */
	highsigs = sim_window_gethighlighttraces();
	if (highsigs[0] == 0)
	{
		ttyputerr(_("Select a signal name first"));
		return(FALSE);
	}
	if (chr == 'r' || chr == DELETEKEY)		/* remove trace */
	{
		sim_window_cleartracehighlight();

		/* delete them */
		nexttr = prevtr = 0;
		for(j=0; highsigs[j] != 0; j++)
		{
			highsig = highsigs[j];
			thispos = sim_window_gettraceframe(highsig);
			sim_window_inittraceloop();
			nexttr = prevtr = 0;
			for(;;)
			{
				trl = sim_window_nexttraceloop();
				if (trl == 0) break;
				pos = sim_window_gettraceframe(trl);
				if (pos > thispos)
				{
					pos--;
					if (pos == thispos) nexttr = trl;
					sim_window_settraceframe(trl, pos);
				} else if (pos == thispos-1) prevtr = trl;
			}

			/* remove from the simulator's list */
			for(i=0; i<simals_levelptr->num_chn; i++)
			{
				node = simals_levelptr->display_page[i+1].nodeptr;
				if (node == 0) continue;
				if (simals_levelptr->display_page[i+1].displayptr == highsig)
				{
					simals_levelptr->display_page[i+1].displayptr = 0;
					break;
				}
			}

			/* kill trace */
			sim_window_killtrace(highsig);
		}
		if (nexttr != 0) sim_window_addhighlighttrace(nexttr); else
			if (prevtr != 0) sim_window_addhighlighttrace(prevtr);
		sim_window_redraw();
		return(FALSE);
	}

	if (highsigs[1] != 0)
	{
		ttyputerr(_("Select just one signal name first"));
		return(FALSE);
	}
	highsig = highsigs[0];

	if (chr == 'c')		/* set clock waveform */
	{
		tracelist = sim_window_getbustraces(highsig);
		if (tracelist != 0 && tracelist[0] != 0)
		{
			ttyputerr(_("Cannot set clock waveform on a bus signal"));
			return(FALSE);
		}
		par[0] = sim_window_gettracename(highsig);
		simals_clock_command(1, par);
		return(FALSE);
	}
	if (chr == 'i')		/* print signal info */
	{
		par[0] = x_("state");
		par[1] = sim_window_gettracename(highsig);
		simals_print_command(2, par);
		return(FALSE);
	}
	if (chr == 'e')		/* clear signal vectors */
	{
		tracelist = sim_window_getbustraces(highsig);
		if (tracelist != 0 && tracelist[0] != 0)
		{
			for(i=0; tracelist[i] != 0; i++)
			{
				par[0] = x_("delete");
				par[1] = sim_window_gettracename(tracelist[i]);
				par[2] = x_("all");
				simals_vector_command(3, par);
			}
		} else
		{
			par[0] = x_("delete");
			par[1] = sim_window_gettracename(highsig);
			par[2] = x_("all");
			simals_vector_command(3, par);
		}
		return(FALSE);
	}

	/* handle setting of values on signals */
	if (chr == 'v')
	{
		tracelist = sim_window_getbustraces(highsig);
		if (tracelist == 0 || tracelist[0] == 0)
		{
			ttyputerr(_("Select a bus signal before setting numeric values on it"));
			return(FALSE);
		}
		for(i=0; tracelist[i] != 0; i++) ;
		bitoffset = numbits - i;
		for(i=0; tracelist[i] != 0; i++)
		{
			nodehead = simals_find_node(sim_window_gettracename(tracelist[i]));
			if (! nodehead) return(FALSE);
			sethead = simals_alloc_link_mem();
			if (sethead == 0) return(FALSE);
			sethead->type = 'N';
			sethead->ptr = (CHAR *)nodehead;
			if (i+bitoffset < 0 || theBits[i+bitoffset] == 0) sethead->state = LOGIC_LOW; else
				sethead->state = LOGIC_HIGH;
			sethead->strength = (INTSML)strength;
			sethead->priority = 2;
			sethead->time = sim_window_getmaincursor();
			sethead->right = 0;
			simals_insert_set_list(sethead);
		}
	} else
	{
		nodehead = simals_find_node(sim_window_gettracename(highsig));
		if (! nodehead) return(FALSE);
		sethead = simals_alloc_link_mem();
		if (sethead == 0) return(FALSE);
		sethead->type = 'N';
		sethead->ptr = (CHAR *)nodehead;
		sethead->state = state;
		sethead->strength = (INTSML)strength;
		sethead->priority = 2;
		sethead->time = sim_window_getmaincursor();
		sethead->right = 0;
		simals_insert_set_list(sethead);
	}

	endtime = simals_initialize_simulator(FALSE);
	if ((sim_window_state&ADVANCETIME) != 0) sim_window_setmaincursor(endtime);
	return(FALSE);
}

/*
 * The character handler for the schematic/layout window of ALS simulation
 */
BOOLEAN simals_charhandlerschem(WINDOWPART *w, INTSML chr, INTBIG special)
{
	INTBIG state, i, highsigcount, *highsiglist;
	REGISTER INTSML strength;
	REGISTER NETWORK *net, **netlist;
	CHAR *par[30], *pt;
	double endtime;
	NODEPTR nodehead;
	LINKPTR sethead;

	ttynewcommand();

	/* special characters are not handled here */
	if (special != 0)
		return(us_charhandler(w, chr, special));

	switch (chr)
	{
		case '?':
			simals_helpwindow(FALSE);
			sim_als_wantweak = sim_als_wantstrong = FALSE;
			return(FALSE);
		case 'u':
			endtime = simals_initialize_simulator(TRUE);
			if ((sim_window_state&ADVANCETIME) != 0) sim_window_setmaincursor(endtime);
			return(FALSE);
		case 'w':
			if (sim_als_wantweak || sim_als_wantstrong) ttybeep(SOUNDBEEP, FALSE);
			sim_als_wantweak = TRUE;
			return(FALSE);
		case 's':
			if (sim_als_wantweak || sim_als_wantstrong) ttybeep(SOUNDBEEP, FALSE);
			sim_als_wantstrong = TRUE;
			return(FALSE);
		case '0':
		case 'l':
			state = LOGIC_LOW;    strength = GATE_STRENGTH;
			if (sim_als_wantweak) strength = NODE_STRENGTH; else
				if (sim_als_wantstrong) strength = VDD_STRENGTH;
			break;
		case '1':
		case 'h':
			state = LOGIC_HIGH;   strength = GATE_STRENGTH;
			if (sim_als_wantweak) strength = NODE_STRENGTH; else
				if (sim_als_wantstrong) strength = VDD_STRENGTH;
			break;
		case 'x':
			state = LOGIC_X;      strength = GATE_STRENGTH;
			if (sim_als_wantweak) strength = NODE_STRENGTH; else
				if (sim_als_wantstrong) strength = VDD_STRENGTH;
			break;
		case 'c':
		case 'i':
		case 't':
		case 'e':
			strength = OFF_STRENGTH;
			break;
		default:
			sim_als_wantweak = sim_als_wantstrong = FALSE;
			return(us_charhandler(w, chr, special));
	}
	sim_als_wantweak = sim_als_wantstrong = FALSE;

	/* find the net in the waveform window */
	netlist = net_gethighlightednets(FALSE);
	net = netlist[0];
	if (net == NONETWORK) return(FALSE);
	if (net->namecount > 0) pt = networkname(net, 0); else
		pt = describenetwork(net);
	highsiglist = sim_window_findtrace(pt, &highsigcount);
	if (highsigcount == 0)
	{
		ttyputerr(_("Cannot find network %s in simulation"), pt);
		return(FALSE);
	}
	sim_window_cleartracehighlight();
	for(i=0; i<highsigcount; i++)
		sim_window_addhighlighttrace(highsiglist[i]);

	if (chr == 'c')
	{
		par[0] = sim_window_gettracename(highsiglist[0]);
		simals_clock_command(1, par);
		return(FALSE);
	}
	if (chr == 'i')
	{
		par[0] = x_("state");
		par[1] = sim_window_gettracename(highsiglist[0]);
		simals_print_command(2, par);
		return(FALSE);
	}
	if (chr == 't')
	{
		nodehead = (NODEPTR)sim_window_gettracedata(highsiglist[0]);
		nodehead->tracenode = TRUE;
		(void)simals_initialize_simulator(TRUE);
		nodehead->tracenode = FALSE;
		return(FALSE);
	}
	if (chr == 'e')
	{
		par[0] = x_("delete");
		par[1] = sim_window_gettracename(highsiglist[0]);
		par[2] = x_("all");
		simals_vector_command(3, par);
		simals_fill_display_arrays();
		sim_window_redraw();
		return(FALSE);
	}

	nodehead = simals_find_node(sim_window_gettracename(highsiglist[0]));
	if (nodehead == 0) return(FALSE);

	sethead = simals_alloc_link_mem();
	if (sethead == 0) return(FALSE);
	sethead->type = 'N';
	sethead->ptr = (CHAR *)nodehead;
	sethead->state = state;
	sethead->strength = strength;
	sethead->priority = 2;
	sethead->time = sim_window_getmaincursor();
	sethead->right = 0;
	simals_insert_set_list(sethead);
	endtime = simals_initialize_simulator(FALSE);

	if ((sim_window_state&ADVANCETIME) != 0) sim_window_setmaincursor(endtime);
	return(FALSE);
}

void simals_helpwindow(BOOLEAN waveform)
{
	REGISTER INTBIG active;
	NODEPROTO *np;

	active = sim_window_isactive(&np);
	if (active == 0)
	{
		ttyputmsg(_("There is no current simulation"));
		return;
	}

	if (waveform)
		ttyputmsg(_("These keys may be typed in the ALS Waveform window:")); else
			ttyputmsg(_("These keys may be typed in the ALS Schematic window:"));
	ttyputinstruction(x_(";,0"), 4, _("Set signal low (normal strength)"));
	ttyputinstruction(x_("wl"),  4, _("Set signal low (weak strength)"));
	ttyputinstruction(x_("sl"),  4, _("Set signal low (strong strength)"));
	ttyputinstruction(x_("h,1"), 4, _("Set signal high (normal strength)"));
	ttyputinstruction(x_("wh"),  4, _("Set signal high (weak strength)"));
	ttyputinstruction(x_("sh"),  4, _("Set signal high (strong strength)"));
	ttyputinstruction(x_(" x"),  4, _("Set signal undefined (normal strength)"));
	ttyputinstruction(x_("wx"),  4, _("Set signal undefined (weak strength)"));
	ttyputinstruction(x_("sx"),  4, _("Set signal undefined (strong strength)"));
	if (waveform)
	{
		ttyputinstruction(x_(" v"),  4, _("Set bus to wide value (normal strength)"));
		ttyputinstruction(x_("wv"),  4, _("Set bus to wide value (weak strength)"));
		ttyputinstruction(x_("sv"),  4, _("Set bus to wide value (strong strength)"));
	}
	ttyputinstruction(x_(" c"),  4, _("Set signal to be a clock"));
	ttyputinstruction(x_(" e"),  4, _("Delete all vectors on signal"));
	ttyputinstruction(x_(" i"),  4, _("Print information about signal"));
	ttyputinstruction(x_(" t"),  4, _("Trace events on signal(s)"));
	ttyputinstruction(x_(" u"),  4, _("Update display (resimulate)"));

	if (waveform)
	{
		ttyputinstruction(x_(" a"), 4, _("Add signal to simulation window"));
		ttyputinstruction(x_(" r"), 4, _("Remove signal from the window"));
		ttyputinstruction(x_(" b"), 4, _("Combine selected signals into a bus"));
		ttyputinstruction(x_("ft"), 4, _("Full Trace of all events"));
		ttyputinstruction(x_(" p"), 4, _("Preserve snapshot of simulation window in database"));
	}
}

/*
 * Name: simals_count_channels
 *
 * Description:
 *	This function returns the number of exported channels in a given level.
 *
 * Calling Argument:
 *	level_pointer : pointer to a level
 */
INTBIG simals_count_channels(CONPTR level_pointer)
{
	INTBIG count=0;
	EXPTR exhead;

	for (exhead = level_pointer->exptr; exhead; exhead = exhead->next) count++;
	return(count);
}

/*
 * Name: simals_set_current_level
 *
 * Description:
 *	This procedure is used to initialize the display channels to the current
 * level of the database hierarchy.  Returns true on error.
 */
BOOLEAN simals_set_current_level(void)
{
	INTBIG i, j, k, buscount, oldsigcount, chn, bussignals[MAXSIMWINDOWBUSWIDTH],
		len, olen;
	CHAR **oldsignames;
	REGISTER VARIABLE *var;
	NODEPTR nodehead;
	NODE *node, *subnode;
	EXPTR exhead;
	CHAR *name, *subname, *pt, *opt, *start, save;
	void *sa;
	NODEPROTO *np;
	REGISTER void *infstr;

	for (nodehead = simals_noderoot; nodehead; nodehead = nodehead->next)
		nodehead->plot_node = 0;

	if (simals_levelptr->display_page)
	{
		exhead = simals_levelptr->exptr;
		for (i = 1; i <= simals_levelptr->num_chn; i++)
		{
			if (exhead)
			{
				exhead->nodeptr->plot_node = 1;
				exhead = exhead->next;
			}
		}
	} else
	{
		simals_levelptr->num_chn = simals_count_channels(simals_levelptr);
		chn = simals_levelptr->num_chn + 1;
		simals_levelptr->display_page = (CHNPTR)simals_alloc_mem((INTBIG)(chn * sizeof(CHANNEL)));
		if (simals_levelptr->display_page == 0) return(TRUE);

		exhead = simals_levelptr->exptr;
		for (i = 1; i < chn; i++)
		{
			if (exhead)
			{
				name = exhead->node_name;
				simals_levelptr->display_page[i].nodeptr = exhead->nodeptr;
				exhead->nodeptr->plot_node = 1;
				exhead = exhead->next;
			} else
			{
				name = x_("");
				simals_levelptr->display_page[i].nodeptr = 0;
			}
			(void)allocstring(&simals_levelptr->display_page[i].name, name, sim_tool->cluster);

			/* convert names to proper bracketed form */
			name = simals_levelptr->display_page[i].name;
			len = estrlen(name) - 1;
			if (name[len] == '_')
			{
				olen = len;
				while (len > 0 && isdigit(name[len-1])) len--;
				if (len != olen && name[len-1] == '_')
				{
					/* found the form "name_INDEX_" */
					name[len-1] = '[';
					name[olen] = ']';
				}
			}
		}
	}

	/* prepare the simulation window */
	oldsigcount = 0;
	i = sim_window_isactive(&np);
	if ((i&SIMWINDOWWAVEFORM) == 0)
	{
		if (i != 0 && np != simals_mainproto)
		{
			/* stop simulation of cell "np" */
			sim_window_stopsimulation();
		}

		/* no simulation running: start it up */
		if (simals_levelptr->num_chn <= 0) return(TRUE);

		var = getvalkey((INTBIG)simals_mainproto, VNODEPROTO, VSTRING|VISARRAY,
			sim_window_signalorder_key);
		if (var != NOVARIABLE)
		{
			oldsigcount = getlength(var);
			if (oldsigcount > 0)
			{
				sa = newstringarray(sim_tool->cluster);
				for(i=0; i<oldsigcount; i++)
					addtostringarray(sa, ((CHAR **)var->addr)[i]);
				oldsignames = getstringarray(sa, &oldsigcount);
			}
		}
		if (sim_window_create(simals_levelptr->num_chn, simals_mainproto,
			((sim_window_state&SHOWWAVEFORM) != 0 ? simals_charhandlerwave : 0),
				simals_charhandlerschem, ALS))
		{
			if (oldsigcount > 0) killstringarray(sa);
			return(TRUE);
		}
		sim_window_state = (sim_window_state & ~SIMENGINECUR) | SIMENGINECURALS;
	}

	/* set the window title */
	if (simals_title == 0) sim_window_titleinfo(_("Top Level")); else
	{
		sim_window_titleinfo(simals_title);
	}

	/* load the traces */
	sim_window_killalltraces(TRUE);
	for(i=0; i<simals_levelptr->num_chn; i++)
		simals_levelptr->display_page[i+1].displayptr = 0;

	/* add in saved signals */
	for(j=0; j<oldsigcount; j++)
	{
		/* see if the name is a bus */
		for(pt = oldsignames[j]; *pt != 0; pt++) if (*pt == '\t') break;
		if (*pt == '\t')
		{
			/* a bus */
			pt++;
			for(buscount = 0; ; )
			{
				for(start = pt; *pt != 0; pt++) if (*pt == '\t') break;
				save = *pt;
				*pt = 0;
				opt = start;
				if (*opt == '-') opt++;
				for( ; *opt != 0; opt++)
					if (!isdigit(*opt) || *opt == ':') break;
				if (*opt == ':') start = opt+1;
				for(i=0; i<simals_levelptr->num_chn; i++)
				{
					node = simals_levelptr->display_page[i+1].nodeptr;
					if (node == 0) continue;
					name = simals_levelptr->display_page[i+1].name;
					if (namesame(name, start) != 0) continue;
					bussignals[buscount] = sim_window_newtrace(-1, name, (INTBIG)node);
					simals_levelptr->display_page[i+1].displayptr = bussignals[buscount];
					buscount++;
					break;
				}
				*pt++ = save;
				if (save == 0) break;
			}

			/* create the bus */
			infstr = initinfstr();
			for(pt = oldsignames[j]; *pt != 0; pt++)
			{
				if (*pt == '\t') break;
				addtoinfstr(infstr, *pt);
			}
			(void)sim_window_makebus(buscount, bussignals, returninfstr(infstr));
		} else
		{
			/* a single signal */
			pt = oldsignames[j];
			if (*pt == '-') pt++;
			for( ; *pt != 0; pt++)
				if (!isdigit(*pt) || *pt == ':') break;
			if (*pt == ':') pt++; else pt = oldsignames[j];
			for(i=0; i<simals_levelptr->num_chn; i++)
			{
				node = simals_levelptr->display_page[i+1].nodeptr;
				if (node == 0) continue;
				name = simals_levelptr->display_page[i+1].name;
				if (namesame(name, pt) != 0) continue;
				simals_levelptr->display_page[i+1].displayptr = sim_window_newtrace(-1, name,
					(INTBIG)node);
				break;
			}
		}
	}

	/* add in other signals not in the saved list */
	for(i=0; i<simals_levelptr->num_chn; i++)
	{
		if (simals_levelptr->display_page[i+1].displayptr != 0) continue;
		node = simals_levelptr->display_page[i+1].nodeptr;
		if (node == 0) continue;
		name = simals_levelptr->display_page[i+1].name;
		for(k=0; name[k] != 0; k++) if (name[k] == '[') break;
		if (name[k] == '[')
		{
			/* found an arrayed signal: gather it into a bus */
			buscount = 0;
			for(j=0; j<simals_levelptr->num_chn; j++)
			{
				if (simals_levelptr->display_page[j+1].displayptr != 0) continue;
				if (simals_levelptr->display_page[j+1].nodeptr == 0) continue;
				subname = simals_levelptr->display_page[j+1].name;
				if (namesamen(subname, name, k) != 0) continue;
				if (buscount >= MAXSIMWINDOWBUSWIDTH) break;
				bussignals[buscount++] = j;
			}
			esort(bussignals, buscount, SIZEOFINTBIG, simals_sortbusnames);
			for(j=0; j<buscount; j++)
			{
				k = bussignals[j];
				subnode = simals_levelptr->display_page[k+1].nodeptr;
				subname = simals_levelptr->display_page[k+1].name;
				bussignals[j] = sim_window_newtrace(-1, subname, (INTBIG)subnode);
				simals_levelptr->display_page[k+1].displayptr = bussignals[j];
			}

			/* create the bus */
			if (buscount > 1)
			{
				(void)sim_window_makebus(buscount, bussignals, 0);
			}
		} else
		{
			simals_levelptr->display_page[i+1].displayptr = sim_window_newtrace(-1, name,
				(INTBIG)node);
		}
	}
	sim_window_cleartracehighlight();
	sim_window_settimerange(0, 0.0, 0.0000005f);
	sim_window_setmaincursor(0.0000002f);
	sim_window_setextensioncursor(0.0000003f);
	if (oldsigcount > 0) killstringarray(sa);
	return(FALSE);
}

/*
 * Helper routine for "esort" that makes bus signals be numerically ordered
 */
int simals_sortbusnames(const void *e1, const void *e2)
{
	REGISTER CHAR *n1, *n2;
	REGISTER INTBIG i1, i2;

	i1 = *((INTBIG *)e1);
	i2 = *((INTBIG *)e2);
	n1 = simals_levelptr->display_page[i1+1].name;
	n2 = simals_levelptr->display_page[i2+1].name;
	if ((net_options&NETDEFBUSBASEDESC) != 0)
		return(namesamenumeric(n2, n1));
	return(namesamenumeric(n1, n2));
}

void simals_fill_display_arrays(void)
{
	INTBIG *numsteps, i, j, pos, num_chan;
	INTSML **statearrays;
	INTBIG *nodelist, displayobj;
	TRAKPTR trakhead;
	double min, max, **timearrays;

	/* determine size needed for waveform arrays */
	num_chan = simals_levelptr->num_chn;
	numsteps = (INTBIG *)emalloc(num_chan * SIZEOFINTBIG, el_tempcluster);
	if (numsteps == 0) return;
	nodelist = (INTBIG *)emalloc(num_chan * SIZEOFINTBIG, el_tempcluster);
	if (nodelist == 0) return;
	timearrays = (double **)emalloc(num_chan * (sizeof (double *)), el_tempcluster);
	if (timearrays == 0) return;
	statearrays = (INTSML **)emalloc(num_chan * (sizeof (INTSML *)), el_tempcluster);
	if (statearrays == 0) return;
	for(i=0; i<num_chan; i++)
	{
		numsteps[i] = 1;
		displayobj = simals_levelptr->display_page[i+1].displayptr;
		if (displayobj == 0) nodelist[i] = 0; else
			nodelist[i] = sim_window_gettracedata(displayobj);
	}

	sim_window_gettimeextents(&min, &max);
	if (simals_trakfull == 0) i = 0; else i = simals_trakptr;
	for( ; i < simals_trakptr && i < simals_trace_size; i++)
	{
		trakhead = &(simals_trakroot[i]);
		if (trakhead->time > max) break;
		for (j=0; j<num_chan; j++)
		{
			if (nodelist[j] == 0) continue;
			if (nodelist[j] != (INTBIG)trakhead->ptr) continue;
			numsteps[j]++;
		}
	}

	/* allocate space for the waveform arrays */
	for (j=0; j<num_chan; j++)
	{
		if (nodelist[j] == 0) continue;
		timearrays[j] = (double *)emalloc(numsteps[j] * (sizeof (double)), sim_tool->cluster);
		statearrays[j] = (INTSML *)emalloc(numsteps[j] * SIZEOFINTSML, sim_tool->cluster);
		if (timearrays[j] == 0 || statearrays[j] == 0) return;
	}

	/* fill the arrays */
	for (i=0; i<num_chan; i++)
	{
		if (nodelist[i] == 0) continue;
		numsteps[i] = 1;
		timearrays[i][0] = min;
		statearrays[i][0] = (LOGIC_LOW << 8) | OFF_STRENGTH;
	}
	if (simals_trakfull == 0) i = 0; else i = simals_trakptr;
	for( ; i < simals_trakptr && i < simals_trace_size; i++)
	{
		trakhead = &(simals_trakroot[i]);
		if (trakhead->time > max) break;
		for (j=0; j<num_chan; j++)
		{
			if (nodelist[j] == 0) continue;
			if (nodelist[j] != (INTBIG)trakhead->ptr) continue;
			pos = numsteps[j]++;
			timearrays[j][pos] = trakhead->time;
			statearrays[j][pos] = (INTSML)((trakhead->state << 8) | trakhead->strength);
		}
	}

	/* give the data to the simulation window system */
	for (j=0; j<num_chan; j++)
	{
		if (nodelist[j] == 0) continue;
		displayobj = simals_levelptr->display_page[j+1].displayptr;
		sim_window_loaddigtrace(displayobj, numsteps[j], timearrays[j], statearrays[j]);
		efree((CHAR *)timearrays[j]);
		efree((CHAR *)statearrays[j]);
	}
	efree((CHAR *)nodelist);
	efree((CHAR *)numsteps);
	efree((CHAR *)timearrays);
	efree((CHAR *)statearrays);
}

/*
 * Routine that feeds the current signals into the explorer window.
 */
void simals_reportsignals(WINDOWPART *simwin, void *(*addbranch)(CHAR*, void*),
	void *(*findbranch)(CHAR*, void*), void *(*addleaf)(CHAR*, void*), CHAR *(*nodename)(void*))
{
	(*addleaf)("NO SIGNALS YET", 0);
}

#endif  /* SIMTOOL - at top */
