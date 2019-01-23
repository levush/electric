/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: simalscom.c
 * Asynchronous Logic Simulator command handler
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
#include "usrdiacom.h"

/* prototypes for local routines */
static void      simals_erase_submod(CONPTR);
static void      simals_print_in_entry(ROWPTR);
static void      simals_print_out_entry(ROWPTR);
static void      simals_print_xref_entry(CONPTR, INTBIG);
static CHAR     *simals_strengthstring(INTSML);
static void      simals_makeactelmodel(CHAR *name, FILE *io, LIBRARY *lib);
static void      simals_sdfannotate(CONPTR cellhead);
static NODEINST *simals_getcellinstance(CHAR *celltype, CHAR *instance);
static void      simals_sdfportdelay(CONPTR cellhead, NODEINST *ni, CHAR *path);
static INTBIG    simals_getportdelayvalue(CHAR *datastring, CHAR *transition, DELAY_TYPES delaytype);
static INTBIG    simals_getdval(CHAR *tstring, DELAY_TYPES delaytype);
static void      simals_order_save(void);
static void      simals_order_restore(CHAR *list);
static void      simals_update_netlist(void);
static CHAR     *simals_parent_level(CHAR *child);
static INTBIG    simals_get_tdindex(CHAR *name);

/* local variables */
static DELAY_TYPES simals_sdfdelaytype;
static CHAR *simals_tnames[] = {x_("01"), x_("10"), x_("0Z"), x_("Z1"), x_("1Z"), x_("Z0"), x_("0X"), x_("X1"), x_("1X"), x_("X0"), x_("XZ"), x_("ZX")};

/****************************** ACTEL MODELS ******************************/

typedef enum {inputparam, outputparam, bidirparam, edgesenseinputparam,
	fourstatebiputparam} PARAMTYPE;

typedef struct Iactelparam
{
	CHAR               *paramname;
	PARAMTYPE           paramtype;
	BOOLEAN             duplicate;
	struct Iactelparam *next;
} ACTELPARAM;

#define MAXCHRS 300

/*
 * Name: simals_build_actel_command
 *
 * Description:
 *	This procedure reads an Actel table file and creates a library with
 * ALS-netlist cells.
 */
void simals_build_actel_command(INTBIG count, CHAR *par[])
{
	FILE *io;
	INTBIG len, i, filecount;
	CHAR *filename, line[MAXCHRS], **filelist;
	REGISTER CHAR *pt, *start;
	static INTBIG filetypeacteltab = -1;
	REGISTER void *infstr;

	if (count == 0)
	{
		ttyputusage(x_("telltool simulation als build-actel-model TABLEFILE"));
		return;
	}
	len = estrlen(par[0]);
	for(i=len-1; i>0; i--) if (par[0][i] == DIRSEP) break;
	par[0][i] = 0;
	filecount = filesindirectory(par[0], &filelist);
	if (filecount == 0)
	{
		ttyputerr(M_("There are no files in directory '%s'"), par[0]);
		return;
	}
	for(i=0; i<filecount; i++)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, par[0]);
		addtoinfstr(infstr, DIRSEP);
		addstringtoinfstr(infstr, filelist[i]);
		pt = returninfstr(infstr);
		if (filetypeacteltab < 0)
			filetypeacteltab = setupfiletype(x_("tab"), x_("*.tab"), MACFSTAG('TEXT'), FALSE, x_("actel"), M_("QuickPartTable"));
		io = xopen(pt, filetypeacteltab, x_(""), &filename);
		if (io == 0)
		{
			ttyputerr(M_("Cannot find Actel file %s"), pt);
			break;
		}
		for(;;)
		{
			if (xfgets(line, MAXCHRS, io) != 0) break;
			pt = line;
			while (*pt == ' ' || *pt == '\t') pt++;
			if (*pt == 0 || *pt == '#') continue;

			if (namesamen(pt, x_("model "), 6) != 0) continue;
			pt += 6;
			while (*pt == ' ' || *pt == '\t') pt++;
			start = pt;
			while (*pt != 0 && *pt != ':') pt++;
			if (*pt != 0) *pt = 0;
			simals_makeactelmodel(start, io, el_curlib);
		}
		xclose(io);
	}
}

void simals_makeactelmodel(CHAR *name, FILE *io, LIBRARY *lib)
{
	CHAR line[MAXCHRS];
	REGISTER CHAR *pt, *start, save, *cellname;
	REGISTER NODEPROTO *np;
	INTBIG statetablestate;
	BOOLEAN doinginput, edgesense, fourstate;
	ACTELPARAM *ap, *lastap, *inputlist, *outputlist, *paramlist, *paramtail, *nextap, *newap;
	void *sa;
	REGISTER void *infstr;

	sa = newstringarray(sim_tool->cluster);
	if (sa == 0) return;

	/* dump header information for the model */
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("# Actel model for "));
	addstringtoinfstr(infstr, name);
	addtostringarray(sa, returninfstr(infstr));

	inputlist = outputlist = paramlist = 0;
	statetablestate = 0;
	for(;;)
	{
		if (xfgets(line, MAXCHRS, io) != 0) break;

		/* find the first keyword */
		pt = line;
		while (*pt == ' ' || *pt == '\t') pt++;
		if (*pt == '#') continue;
		if (*pt == 0) continue;

		/* stop if done with table */
		if (namesamen(pt, x_("end"), 3) == 0) break;

		/* see if gathering state table information */
		if (statetablestate != 0)
		{
			if (statetablestate == 1)
			{
				/* get state table header */
				statetablestate = 2;
				doinginput = TRUE;
				paramtail = 0;
				for(;;)
				{
					while (*pt == ' ' || *pt == '\t') pt++;
					start = pt;
					while (*pt != 0 && *pt != ' ' && *pt != '\t' && *pt != ',' && *pt != ';' &&
						*pt != ':') pt++;
					save = *pt;   *pt = 0;
					if (doinginput)
					{
						/* search input list for this one */
						lastap = 0;
						for(ap = inputlist; ap != 0; ap = ap->next)
						{
							if (estrcmp(ap->paramname, start) == 0) break;
							lastap = ap;
						}
						if (ap == 0)
						{
							/* see if it is on the output list */
							for(ap = outputlist; ap != 0; ap = ap->next)
								if (estrcmp(ap->paramname, start) == 0) break;
							if (ap == 0)
							{
								ttyputerr(M_("In model %s, symbol %s not found in parameter lists"),
									name, start);
								return;
							}
							newap = (ACTELPARAM *)emalloc(sizeof (ACTELPARAM), sim_tool->cluster);
							if (newap == 0) return;
							(void)allocstring(&newap->paramname, ap->paramname, sim_tool->cluster);
							newap->paramtype = ap->paramtype;
							newap->duplicate = TRUE;
							ap = newap;
						} else
						{
							if (lastap == 0) inputlist = ap->next; else
								lastap->next = ap->next;
						}
					} else
					{
						/* search output list for this one */
						lastap = 0;
						for(ap = outputlist; ap != 0; ap = ap->next)
						{
							if (estrcmp(ap->paramname, start) == 0) break;
							lastap = ap;
						}
						if (ap == 0)
						{
							/* see if it is already on the input list */
							for(ap = paramlist; ap != 0; ap = ap->next)
								if (estrcmp(ap->paramname, start) == 0) break;
							if (ap == 0)
							{
								ttyputerr(M_("In model %s, symbol %s not found in parameter lists"),
									name, start);
								return;
							}
							newap = (ACTELPARAM *)emalloc(sizeof (ACTELPARAM), sim_tool->cluster);
							if (newap == 0) return;
							(void)allocstring(&newap->paramname, ap->paramname, sim_tool->cluster);
							newap->paramtype = ap->paramtype;
							newap->duplicate = TRUE;
							ap = newap;
						} else
						{
							if (lastap == 0) outputlist = ap->next; else
								lastap->next = ap->next;
						}
					}
					ap->next = 0;
					if (paramtail == 0) paramlist = paramtail = ap; else
					{
						paramtail->next = ap;
						paramtail = ap;
					}
					*pt = save;
					while (*pt == ' ' || *pt == '\t') pt++;
					if (*pt == ';') break;
					if (*pt == ':') { pt += 2;   doinginput = FALSE;   continue; }
					if (*pt == ',') { pt++;   continue; }
					ttyputerr(M_("In model %s, missing comma in Actel symbol list"), name);
					return;
				}

				/* now dump the model header */
				infstr = initinfstr();
				addstringtoinfstr(infstr, x_("gate "));
				addstringtoinfstr(infstr, name);
				addstringtoinfstr(infstr, x_("("));
				for(ap = paramlist; ap != 0; ap = ap->next)
				{
					if (ap->duplicate) continue;
					if (ap != paramlist) addstringtoinfstr(infstr, x_(", "));
					addstringtoinfstr(infstr, ap->paramname);
				}
				addstringtoinfstr(infstr, x_(")"));
				addtostringarray(sa, returninfstr(infstr));
				addtostringarray(sa, x_("t: delta=1.0e-9"));
				continue;
			} else
			{
				/* get state table entry */
				ap = paramlist;
				infstr = initinfstr();
				addstringtoinfstr(infstr, x_("i: "));
				for(;;)
				{
					while (*pt == ' ' || *pt == '\t') pt++;
					addstringtoinfstr(infstr, ap->paramname);
					addstringtoinfstr(infstr, x_("="));
					switch (*pt)
					{
						case '0': addstringtoinfstr(infstr, x_("L"));   break;
						case '1': addstringtoinfstr(infstr, x_("H"));   break;
						case '?': addstringtoinfstr(infstr, x_("X"));   break;
						case '[': addstringtoinfstr(infstr, x_("E"));   break;
						case '(': addstringtoinfstr(infstr, x_("F"));   break;
						case 'Z': addstringtoinfstr(infstr, x_("Z"));   break;
						case 'X': addstringtoinfstr(infstr, x_("XX"));  break;
						case '!': addstringtoinfstr(infstr, x_("N"));   break;
					}
					addstringtoinfstr(infstr, x_(" "));

					while (*pt != 0 && *pt != ',' && *pt != ';' && *pt != ':') pt++;
					if (*pt == ';') break;
					if (*pt == ':') { addstringtoinfstr(infstr, x_("o: "));   pt++; }
					pt++;
					ap = ap->next;
				}
				addtostringarray(sa, returninfstr(infstr));
				continue;
			}
		}

		/* handle "edge_sense" prefix */
		edgesense = FALSE;
		if (namesamen(pt, x_("edge_sense"), 10) == 0)
		{
			edgesense = TRUE;
			pt += 10;
			while (*pt == ' ' || *pt == '\t') pt++;
		}

		/* handle "four_state" prefix */
		fourstate = FALSE;
		if (namesamen(pt, x_("four_state"), 10) == 0)
		{
			fourstate = TRUE;
			pt += 10;
			while (*pt == ' ' || *pt == '\t') pt++;
		}

		/* gather inputs */
		if (namesamen(pt, x_("input"), 5) == 0)
		{
			pt += 5;
			for(;;)
			{
				while (*pt == ' ' || *pt == '\t') pt++;
				start = pt;
				while (*pt != 0 && *pt != ' ' && *pt != '\t' && *pt != ',' && *pt != ';') pt++;
				save = *pt;   *pt = 0;
				ap = (ACTELPARAM *)emalloc(sizeof (ACTELPARAM), sim_tool->cluster);
				if (ap == 0) break;
				(void)allocstring(&ap->paramname, start, sim_tool->cluster);
				if (edgesense) ap->paramtype = edgesenseinputparam; else
					ap->paramtype = inputparam;
				ap->duplicate = FALSE;
				ap->next = inputlist;
				inputlist = ap;
				*pt = save;
				while (*pt == ' ' || *pt == '\t') pt++;
				if (*pt == ';') break;
				if (*pt == ',') { pt++;   continue; }
				ttyputerr(M_("In model %s, missing comma in Actel input list"), name);
				return;
			}
			continue;
		}

		/* gather outputs */
		if (namesamen(pt, x_("output"), 6) == 0)
		{
			pt += 6;
			for(;;)
			{
				while (*pt == ' ' || *pt == '\t') pt++;
				start = pt;
				while (*pt != 0 && *pt != ' ' && *pt != '\t' && *pt != ',' && *pt != ';') pt++;
				save = *pt;   *pt = 0;
				ap = (ACTELPARAM *)emalloc(sizeof (ACTELPARAM), sim_tool->cluster);
				if (ap == 0) break;
				(void)allocstring(&ap->paramname, start, sim_tool->cluster);
				ap->paramtype = outputparam;
				ap->duplicate = FALSE;
				ap->next = outputlist;
				outputlist = ap;
				*pt = save;
				while (*pt == ' ' || *pt == '\t') pt++;
				if (*pt == ';') break;
				if (*pt == ',') { pt++;   continue; }
				ttyputerr(M_("In model %s, missing comma in Actel output list"), name);
				return;
			}
			continue;
		}

		/* gather bidirectionals */
		if (namesamen(pt, x_("biput"), 5) == 0)
		{
			pt += 5;
			for(;;)
			{
				while (*pt == ' ' || *pt == '\t') pt++;
				start = pt;
				while (*pt != 0 && *pt != ' ' && *pt != '\t' && *pt != ',' && *pt != ';') pt++;
				save = *pt;   *pt = 0;
				ap = (ACTELPARAM *)emalloc(sizeof (ACTELPARAM), sim_tool->cluster);
				if (ap == 0) break;
				(void)allocstring(&ap->paramname, start, sim_tool->cluster);
				if (fourstate) ap->paramtype = fourstatebiputparam; else
					ap->paramtype = bidirparam;
				ap->duplicate = FALSE;
				ap->next = inputlist;
				inputlist = ap;
				*pt = save;
				while (*pt == ' ' || *pt == '\t') pt++;
				if (*pt == ';') break;
				if (*pt == ',') { pt++;   continue; }
				ttyputerr(M_("In model %s, missing comma in Actel biput list"), name);
				return;
			}
			continue;
		}

		/* start state table */
		if (namesamen(pt, x_("state_table"), 11) == 0)
		{
			statetablestate = 1;
			continue;
		}
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("load"));
	for(ap = paramlist; ap != 0; ap = nextap)
	{
		nextap = ap->next;
		if (ap->paramtype == inputparam || ap->paramtype == edgesenseinputparam)
		{
			addstringtoinfstr(infstr, x_(" "));
			addstringtoinfstr(infstr, ap->paramname);
			addstringtoinfstr(infstr, x_("=1.0"));
		}
		efree(ap->paramname);
		efree((CHAR *)ap);
	}
	addtostringarray(sa, returninfstr(infstr));

	/* now put strings onto a cell */
	infstr = initinfstr();
	addstringtoinfstr(infstr, name);
	addstringtoinfstr(infstr, x_("{net-als}"));
	cellname = returninfstr(infstr);
	np = newnodeproto(cellname, lib);
	if (np == NONODEPROTO)
	{
		ttyputerr(M_("Cannot create cell %s"), cellname);
		return;
	}
	stringarraytotextcell(sa, np, TRUE);
	killstringarray(sa);
	ttyputmsg(M_("Created %s"), describenodeproto(np));
}

/****************************** CLOCK ******************************/

/*
 * Name: simals_clock_command
 *
 * Description:
 *	This procedure enters a complex clock vector into the user defined
 * event list.  The user is polled for the node name and timing parameters
 * before any entry is made into the linklist.
 */
void simals_clock_command(INTBIG count, CHAR *par[])
{
	double   time, totaltime;
	NODEPTR  nodehead;
	NODEPROTO *np;
	LINKPTR  vectptr1, vectptr2, sethead;
	ROWPTR   clokhead;
	float    linear;
	LINKPTR  vectroot;
	CHAR     **vectptr;
	INTSML    strength;
	INTBIG    num, l, i;

	if (sim_window_isactive(&np) == 0)
	{
		ttyputerr(M_("No simulator active"));
		return;
	}
	if (count < 1)
	{
		ttyputusage(x_("telltool simulation als clock NODENAME (freq | period | custom)"));
		return;
	}

	simals_convert_to_upper(par[0]);
	nodehead = simals_find_node(par[0]);
	if (! nodehead)
	{
		ttyputerr(M_("ERROR: Unable to find node %s"), par[0]);
		return;
	}

	if (count < 2)
	{
		count = sim_alsclockdlog(&par[1]) + 1;
		if (count < 2) return;
	}

	/* see if there are frequency/period parameters */
	l = estrlen(par[1]);
	if (namesamen(par[1], x_("frequency"), l) == 0 || namesamen(par[1], x_("period"), l) == 0)
	{
		if (count < 3)
		{
			ttyputusage(x_("telltool simulation als clock NODENAME frequency/period PERIOD"));
			return;
		}
		time = eatof(par[2]);
		if (time <= 0.0)
		{
			ttyputerr(M_("Clock timing parameter must be greater than 0"));
			return;
		}

		if (namesamen(par[1], x_("frequency"), l) == 0) time = 1.0f / time;

		vectptr2 = simals_alloc_link_mem();
		if (vectptr2 == 0) return;
		vectptr2->type = 'N';
		vectptr2->ptr = (CHAR *)nodehead;
		vectptr2->state = LOGIC_HIGH;
		vectptr2->strength = VDD_STRENGTH;
		vectptr2->priority = 1;
		vectptr2->time = 0.0;
		vectptr2->right = 0;

		vectptr1 = simals_alloc_link_mem();
		if (vectptr1 == 0) return;
		vectptr1->type = 'N';
		vectptr1->ptr = (CHAR *)nodehead;
		vectptr1->state = LOGIC_LOW;
		vectptr1->strength = VDD_STRENGTH;
		vectptr1->priority = 1;
		vectptr1->time = time / 2.0f;
		vectptr1->right = vectptr2;

		clokhead = (ROWPTR)simals_alloc_mem((INTBIG)sizeof(ROW));
		if (clokhead == 0) return;
		clokhead->inptr = (IOPTR)vectptr1;
		clokhead->outptr = 0;
		clokhead->delta = (float)time;
		clokhead->linear = 0.0;
		clokhead->exp = 0.0;
		clokhead->abs = 0.0;
		clokhead->random = 0.0;
		clokhead->next = 0;
		clokhead->delay = 0;

		sethead = simals_alloc_link_mem();
		if (sethead == 0) return;
		sethead->type = 'C';
		sethead->ptr = (CHAR *)clokhead;
		sethead->state = 0;
		sethead->priority = 1;
		sethead->time = 0.0;
		sethead->right = 0;
		simals_insert_set_list(sethead);

		(void)simals_initialize_simulator(FALSE);
		return;
	}

	if (namesamen(par[1], x_("custom"), l) != 0)
	{
		ttyputbadusage(x_("telltool simulation als clock"));
		return;
	}

	/* handle custom clock specification */
	if (count < 7)
	{
		ttyputusage(x_("telltool simulation als clock custom RAN STR CY (L D) *"));
		return;
	}

	linear = (float)eatof(par[2]);
	strength = (INTSML)simals_atoi(par[3])*2;
	num = simals_atoi(par[4]);

	totaltime = 0.0;
	vectroot = 0;
	vectptr = (CHAR**) &vectroot;
	for(i=5; i<count; i += 2)
	{
		vectptr2 = simals_alloc_link_mem();
		if (vectptr2 == 0) return;
		vectptr2->type = 'N';
		vectptr2->ptr = (CHAR *) nodehead;
		vectptr2->state = simals_trans_state_to_number(par[i]);
		vectptr2->strength = strength;
		vectptr2->priority = 1;
		vectptr2->time = eatof(par[i+1]);
		totaltime += vectptr2->time;
		vectptr2->right = 0;
		*vectptr = (CHAR*) vectptr2;
		vectptr = (CHAR**) &(vectptr2->right);
	}
	vectptr2->time = 0.0;

	clokhead = (ROWPTR) simals_alloc_mem((INTBIG)sizeof(ROW));
	if (clokhead == 0) return;
	clokhead->inptr = (IOPTR) vectroot;
	clokhead->outptr = 0;
	clokhead->delta = (float)totaltime;
	clokhead->linear = linear;
	clokhead->exp = 0.0;
	clokhead->abs = 0.0;
	clokhead->random = 0.0;
	clokhead->next = 0;
	clokhead->delay = 0;

	sethead = simals_alloc_link_mem();
	if (sethead == 0) return;
	sethead->type = 'C';
	sethead->ptr = (CHAR *) clokhead;
	sethead->state = num;
	sethead->priority = 1;
	sethead->time = 0.0;
	sethead->right = 0;
	simals_insert_set_list(sethead);

	(void)simals_initialize_simulator(FALSE);
}

/****************************** ERASE ******************************/

void simals_erase_model(void)
{
	MODPTR modptr, nextmodptr;
	EXPTR exptr, nextexptr;
	CONPTR conptr, nextconptr;
	IOPTR ioptr, nextioptr;
	ROWPTR rowptr, nextrowptr;
	LOADPTR loadptr, nextloadptr;
	FUNCPTR funcptr;
	NODEPTR node, nextnode;
	STATPTR statptr, nextstatptr;

	/* reset miscellaneous simulation variables */
	simals_linkfront = 0;
	simals_linkback = 0;

	/* loop through all test vectors */
	simals_clearallvectors(TRUE);

	/* loop throuth all cells in flattened network */
	if (simals_cellroot != 0)
	{
		simals_erase_submod(simals_cellroot);
		efree((CHAR *)simals_cellroot);
		simals_cellroot = 0;
	}
	simals_levelptr = 0;

	/* loop through all nodes in flattened network */
	for(node = simals_noderoot; node != 0; node = nextnode)
	{
		nextnode = node->next;

		/* erase all loads on the nodes */
		for(loadptr = node->pinptr; loadptr != 0; loadptr = nextloadptr)
		{
			nextloadptr = loadptr->next;
			efree((CHAR *)loadptr);
		}

		/* erase all stats on the nodes */
		for(statptr = node->statptr; statptr != 0; statptr = nextstatptr)
		{
			nextstatptr = statptr->next;
			efree((CHAR *)statptr);
		}
		efree((CHAR *)node);
	}
	simals_noderoot = 0;

	/* loop through all primitives in flattened network */
	for(modptr = simals_primroot; modptr != 0; modptr = nextmodptr)
	{
		nextmodptr = modptr->next;
		if (modptr->type == 'F')
		{
			/* loop through each formal port on the function */
			for(exptr = modptr->exptr; exptr != 0; exptr = nextexptr)
			{
				nextexptr = exptr->next;
				efree((CHAR *)exptr);
			}

			/* loop through each parameter on the function */
			funcptr = (FUNCPTR)modptr->ptr;
			for(exptr = funcptr->inptr; exptr != 0; exptr = nextexptr)
			{
				nextexptr = exptr->next;
				efree((CHAR *)exptr);
			}

			efree((CHAR *)funcptr);
		}
		if (modptr->type == 'G')
		{
			/* loop through each row in the gate */
			for(rowptr = (ROWPTR)modptr->ptr; rowptr != 0; rowptr = nextrowptr)
			{
				nextrowptr = rowptr->next;

				/* loop through each input on the row */
				for(ioptr = rowptr->inptr; ioptr != 0; ioptr = nextioptr)
				{
					nextioptr = ioptr->next;
					efree((CHAR *)ioptr);
				}

				/* loop through each output on the row */
				for(ioptr = rowptr->outptr; ioptr != 0; ioptr = nextioptr)
				{
					nextioptr = ioptr->next;
					efree((CHAR *)ioptr);
				}
				if (rowptr->delay != 0) efree((CHAR *)rowptr->delay);
				efree((CHAR *)rowptr);
			}
		}
		if (modptr->level != 0) efree((CHAR *)modptr->level);
		efree((CHAR *)modptr);
	}
	simals_primroot = 0;

	/* loop through each model/gate/function in hierarchical description */
	for(modptr = simals_modroot; modptr != 0; modptr = nextmodptr)
	{
		nextmodptr = modptr->next;
		efree((CHAR *)modptr->name);

		/* loop through each formal port on the model/gate/function */
		for(exptr = modptr->exptr; exptr != 0; exptr = nextexptr)
		{
			nextexptr = exptr->next;
			efree((CHAR *)exptr->node_name);
			efree((CHAR *)exptr);
		}

		/* loop through each "SET" instruction in the model/gate/function */
		for(ioptr = modptr->setptr; ioptr != 0; ioptr = nextioptr)
		{
			nextioptr = ioptr->next;
			efree((CHAR *)ioptr->nodeptr);
			efree((CHAR *)ioptr);
		}

		/* special cleanup for functions */
		if (modptr->type == 'F')
		{
			funcptr = (FUNCPTR)modptr->ptr;

			/* loop through each load in the function */
			for(loadptr = modptr->loadptr; loadptr != 0; loadptr = nextloadptr)
			{
				nextloadptr = loadptr->next;
				efree((CHAR *)loadptr->ptr);
				efree((CHAR *)loadptr);
			}

			/* loop through each input on the function */
			for(exptr = funcptr->inptr; exptr != 0; exptr = nextexptr)
			{
				nextexptr = exptr->next;
				efree((CHAR *)exptr->node_name);
				efree((CHAR *)exptr);
			}
			efree((CHAR *)funcptr);
		}

		/* special cleanup for models */
		if (modptr->type == 'M')
		{
			/* loop through each instance in the model */
			for(conptr = (CONPTR)modptr->ptr; conptr != 0; conptr = nextconptr)
			{
				nextconptr = conptr->next;
				efree((CHAR *)conptr->inst_name);
				if (conptr->model_name != 0) efree(conptr->model_name);

				/* loop through each actual port on the instance */
				for(exptr = conptr->exptr; exptr != 0; exptr = nextexptr)
				{
					nextexptr = exptr->next;
					efree((CHAR *)exptr->node_name);
					efree((CHAR *)exptr);
				}
				efree((CHAR *)conptr);
			}
		}

		/* special cleanup for gates */
		if (modptr->type == 'G')
		{
			/* loop through each row in the gate */
			for(rowptr = (ROWPTR)modptr->ptr; rowptr != 0; rowptr = nextrowptr)
			{
				nextrowptr = rowptr->next;

				/* loop through each input on the row */
				for(ioptr = rowptr->inptr; ioptr != 0; ioptr = nextioptr)
				{
					nextioptr = ioptr->next;
					efree((CHAR *)ioptr->nodeptr);
					efree((CHAR *)ioptr);
				}

				/* loop through each output on the row */
				for(ioptr = rowptr->outptr; ioptr != 0; ioptr = nextioptr)
				{
					nextioptr = ioptr->next;
					efree((CHAR *)ioptr->nodeptr);
					efree((CHAR *)ioptr);
				}
				if (rowptr->delay != 0) efree((CHAR *)rowptr->delay);
				efree((CHAR *)rowptr);
			}

			/* loop through each load in the gate */
			for(loadptr = modptr->loadptr; loadptr != 0; loadptr = nextloadptr)
			{
				nextloadptr = loadptr->next;
				efree((CHAR *)loadptr->ptr);
				efree((CHAR *)loadptr);
			}
		}

		if (modptr->level != 0) efree((CHAR *)modptr->level);
		efree((CHAR *)modptr);
	}
	simals_modroot = 0;
}

/*
 * Routine to clear all test vectors (even the power and ground vectors if "pwrgnd"
 * is true).
 */
void simals_clearallvectors(BOOLEAN pwrgnd)
{
	LINKPTR thisset, vecthead, nextvec, nextset, lastset;
	ROWPTR clokhead;

	lastset = 0;
	for(thisset = simals_setroot; thisset != 0; thisset = nextset)
	{
		nextset = thisset->right;
		if (pwrgnd || thisset->strength != VDD_STRENGTH)
		{
			if (thisset->type == 'C')
			{
				clokhead = (ROWPTR)thisset->ptr;
				for (vecthead = (LINKPTR)clokhead->inptr; vecthead; vecthead = nextvec)
				{
					nextvec = vecthead->right;
					simals_free_link_mem(vecthead);
				}
				efree((CHAR *)clokhead);
			}
			simals_free_link_mem(thisset);

			if (lastset == 0) simals_setroot = nextset; else
				lastset->right = nextset;
		} else
		{
			lastset = thisset;
		}
	}
}

void simals_erase_submod(CONPTR conhead)
{
	CONPTR conptr, nextconptr;
	EXPTR exptr, nextexptr;
	REGISTER INTBIG chn, i;

	for(conptr = conhead->child; conptr != 0; conptr = nextconptr)
	{
		nextconptr = conptr->next;
		simals_erase_submod(conptr);
		for(exptr = conptr->exptr; exptr != 0; exptr = nextexptr)
		{
			nextexptr = exptr->next;
			efree((CHAR *)exptr);
		}
		efree((CHAR *)conptr);
	}
	if (conhead->display_page != 0)
	{
		chn = conhead->num_chn + 1;
		for (i = 1; i < chn; i++)
			efree((CHAR *)conhead->display_page[i].name);
		efree((CHAR *)conhead->display_page);
	}
}

/****************************** GO ******************************/

/*
 * Name: simals_go_command
 *
 * Description:
 *	This procedure parses the command line for the go command from the
 * keyboard.  The maximum execution time must also be specified for the
 * simulation run.
 */
void simals_go_command(INTBIG count, CHAR *par[])
{
	double max;
	NODEPROTO *np;

	if (sim_window_isactive(&np) == 0)
	{
		ttyputerr(M_("No simulator active"));
		return;
	}
	if (count < 1)
	{
		ttyputerr(M_("Must specify simulation time"));
		return;
	}
	max = eatof(par[0]);
	if (max <= 0.0)
	{
		ttyputerr(M_("Simulation time must be greater than 0 seconds"));
		return;
	}
	sim_window_settimerange(0, 0.0, max);

	(void)simals_initialize_simulator(TRUE);
}

/****************************** HELP ******************************/

/*
 * Name: simals_help_command
 *
 * Description:
 *	This procedure process the help command and display help information.
 */
void simals_help_command(void)
{
	(void)us_helpdlog(x_("ALS"));
}

/****************************** LEVEL ******************************/

/*
 * Name: simals_level_up_command
 *
 * Description:
 *	This procedure changes the level of hierarchy up one level.
 */
void simals_level_up_command(void)
{
	CONPTR  cellptr;
	INTBIG   l, len, i;
	double maintime, exttime;
	NODEPROTO *np;

	if (sim_window_isactive(&np) == 0)
	{
		ttyputerr(_("No simulator active"));
		return;
	}

	if (simals_levelptr == 0)
	{
		ttyputerr(_("No simulation is running"));
		return;
	}
	cellptr = simals_levelptr->parent;
	if (! cellptr)
	{
		ttyputerr(_("ERROR: Currently at top level of hierarchy"));
		return;
	}

	simals_levelptr = cellptr;

	/* determine the level title */
	if (simals_title != 0)
	{
		len = strlen(simals_title);
		for(i=len-1; i>0; i--)
			if (simals_title[i] == '.') break;
		if (i > 0) simals_title[i] = 0; else
		{
			efree((CHAR *)simals_title);
			simals_title = 0;
		}
	}

	/* reinitialize simulator while preserving time information */
	maintime = sim_window_getmaincursor();
	exttime = sim_window_getextensioncursor();
	if (simals_set_current_level()) return;
	sim_window_setmaincursor(maintime);
	sim_window_setextensioncursor(exttime);

	l = simals_levelptr->num_chn;
	sim_window_setnumframes(l);
	sim_window_settopvisframe(0);
	(void)simals_initialize_simulator(TRUE);
}

/*
 * Name: simals_level_set_command
 *
 * Description:
 *	This procedure changes the level of hierarchy to a specified new (lower) level.
 */
void simals_level_set_command(CHAR *instname)
{
	CONPTR  cellptr;
	INTBIG   l;
	double maintime, exttime;
	CHAR    *pt;
	NODEPROTO *np;
	void *infstr;

	if (sim_window_isactive(&np) == 0)
	{
		ttyputerr(_("No simulator active"));
		return;
	}

	if (instname != 0)
	{
		for(pt = instname; *pt != 0; pt++) if (*pt == ' ') break;
		*pt = 0;
		simals_convert_to_upper(instname);
		cellptr = simals_find_level(instname);
		if (! cellptr)
		{
			ttyputerr(M_("ERROR: Unable to find level %s"), instname);
			return;
		}

		simals_levelptr = cellptr;

		/* determine the level title */
		infstr = initinfstr();
		if (simals_title != 0) formatinfstr(infstr, x_("%s."), simals_title);
		addstringtoinfstr(infstr, simals_levelptr->inst_name);
		if (simals_title != 0) efree((CHAR *)simals_title);
		allocstring(&simals_title, returninfstr(infstr), sim_tool->cluster);
	}

	/* reinitialize simulator while preserving time information */
	maintime = sim_window_getmaincursor();
	exttime = sim_window_getextensioncursor();
	if (simals_set_current_level()) return;
	sim_window_setmaincursor(maintime);
	sim_window_setextensioncursor(exttime);

	l = simals_levelptr->num_chn;
	sim_window_setnumframes(l);
	sim_window_settopvisframe(0);
	(void)simals_initialize_simulator(TRUE);
}

/****************************** PRINT ******************************/

/*
 * routine to print out the display screen status and information
 */
void simals_print_command(INTBIG count, CHAR *par[])
{
	INTBIG l;
	CHAR *pt, *s1;
	LINKPTR  linkhead;
	NODEPTR  nodehead;
	STATPTR  stathead;
	MODPTR   primhead;
	ROWPTR   rowhead;
	EXPTR    exhead;
	FUNCPTR  funchead;
	CONPTR   cellhead;
	REGISTER void *infstr;

	if (count < 1)
	{
		ttyputusage(x_("telltool simulation als print OPTION"));
		return;
	}
	l = estrlen(pt = par[0]);

	if (namesamen(pt, x_("size"), l) == 0 && l >= 2)
	{
		ttyputmsg(M_("Number of Primitive Elements in Database = %ld"), simals_pseq);
		ttyputmsg(M_("Number of Nodes in Database = %ld"), simals_nseq);
		return;
	}

	if (namesamen(pt, x_("vector"), l) == 0 && l >= 1)
	{
		linkhead = simals_setroot;
		ttyputmsg(M_("** VECTOR LINKLIST **"));
		while (linkhead)
		{
			switch (linkhead->type)
			{
				case 'N':
					nodehead = (NODEPTR)linkhead->ptr;
					s1 = simals_trans_number_to_state(linkhead->state);
					ttyputmsg(M_("***** vector: $N%ld, state = %s, strength = %s, time = %g, priority = %d"),
						nodehead->num, s1, simals_strengthstring(linkhead->strength),
							linkhead->time, linkhead->priority);
					break;
				case 'F':
					stathead = (STATPTR)linkhead->ptr;
					nodehead = stathead->nodeptr;
					s1 = simals_trans_number_to_state(linkhead->state);
					ttyputmsg(M_("***** function: $N%ld, state = %s, strength = %s, time = %g, priority = %d"),
						nodehead->num, s1, simals_strengthstring(linkhead->strength),
							linkhead->time, linkhead->priority);
					break;
				case 'R':
					ttyputmsg(M_("***** rowptr = %ld, time = %g, priority = %d"),
						linkhead->ptr, linkhead->time, linkhead->priority);
					break;
				case 'C':
					ttyputmsg(M_("***** clokptr = %ld, time = %g, priority = %d"),
						linkhead->ptr, linkhead->time, linkhead->priority);
			}
			linkhead = linkhead->right;
		}
		return;
	}

	if (namesamen(pt, x_("netlist"), l) == 0 && l >= 1)
	{
		ttyputmsg(M_("** NETWORK DESCRIPTION **"));
		for (primhead = simals_primroot; primhead; primhead = primhead->next)
		{
			if (stopping(STOPREASONDISPLAY)) return;
			switch (primhead->type)
			{
				case 'F':
					infstr = initinfstr();
					formatinfstr(infstr, M_("FUNCTION %ld: %s (instance %s) ["), primhead->num, primhead->name,
						(primhead->level == NULL) ? M_("null") : primhead->level);
					for (exhead = primhead->exptr; exhead; exhead=exhead->next)
					{
						if (exhead != primhead->exptr) addstringtoinfstr(infstr, x_(", "));
						formatinfstr(infstr, x_("N%ld"), exhead->nodeptr->num);
					}
					addstringtoinfstr(infstr, x_("]"));
					ttyputmsg(x_("%s"), returninfstr(infstr));
					infstr = initinfstr();
					addstringtoinfstr(infstr, M_("  Event Driving Inputs:"));
					funchead = (FUNCPTR)primhead->ptr;
					for (exhead = funchead->inptr; exhead; exhead=exhead->next)
						formatinfstr(infstr, x_(" N%ld"), exhead->nodeptr->num);
					ttyputmsg(x_("%s"), returninfstr(infstr));
					infstr = initinfstr();
					addstringtoinfstr(infstr, M_("  Output Ports:"));
					for (exhead = primhead->exptr; exhead; exhead=exhead->next)
					{
						if (exhead->node_name)
							formatinfstr(infstr, x_(" N%ld"), ((STATPTR)exhead->node_name)->nodeptr->num);
					}
					ttyputmsg(x_("%s"), returninfstr(infstr));
					ttyputmsg(M_("  Timing: D=%g, L=%g, E=%g, R=%g, A=%g"), funchead->delta,
						funchead->linear, funchead->exp, funchead->random, funchead->abs);
					ttyputmsg(M_("  Firing Priority = %d"), primhead->priority);
					break;
				case 'G':
					ttyputmsg(M_("GATE %ld: %s (instance %s)"), primhead->num, primhead->name,
						(primhead->level == NULL) ? M_("null") : primhead->level);
					for (rowhead = (ROWPTR)primhead->ptr; rowhead; rowhead=rowhead->next)
					{
						ttyputmsg(M_("  Timing: D=%g, L=%g, E=%g, R=%g, A=%g"), rowhead->delta,
							rowhead->linear, rowhead->exp, rowhead->random, rowhead->abs);
						ttyputmsg(M_("  Delay type: %s"), (rowhead->delay == NULL) ? M_("null") : rowhead->delay);
						simals_print_in_entry(rowhead);
						simals_print_out_entry(rowhead);
					}
					ttyputmsg(M_("  Firing Priority = %d"), primhead->priority);
					break;
				default:
					ttyputerr(M_("Illegal primitive type '%c', database is bad"), primhead->type);
					break;
			}
		}
		return;
	}

	if (namesamen(pt, x_("xref"), l) == 0 && l >= 1)
	{
		ttyputmsg(M_("** CROSS REFERENCE TABLE **"));
		simals_print_xref_entry(simals_levelptr, 0);
		return;
	}

	if (namesamen(pt, x_("state"), l) == 0 && l >= 2)
	{
		if (count < 2)
		{
			ttyputusage(x_("telltool simulation als print state NODENAME"));
			return;
		}
		simals_convert_to_upper(par[1]);
		nodehead = simals_find_node(par[1]);
		if (nodehead == 0)
		{
			ttyputerr(M_("ERROR: Unable to find node %s"), par[1]);
			return;
		}

		s1 = simals_trans_number_to_state(nodehead->new_state);
		ttyputmsg(M_("Node %s: State = %s, Strength = %s"), par[1], s1,
			simals_strengthstring(nodehead->new_strength));
		stathead = nodehead->statptr;
		while (stathead)
		{
			s1 = simals_trans_number_to_state(stathead->new_state);
			ttyputmsg(M_("Primitive %ld:    State = %s, Strength = %s"), stathead->primptr->num,
				s1, simals_strengthstring(stathead->new_strength));
			stathead = stathead->next;
		}
		return;
	}

	if (namesamen(pt, x_("instances"), l) == 0 && l >= 1)
	{
		ttyputmsg(M_("Instances at level: %s"), simals_compute_path_name(simals_levelptr));
		for (cellhead = simals_levelptr->child; cellhead; cellhead = cellhead->next)
		{
			if (stopping(STOPREASONDISPLAY)) break;
			ttyputmsg(M_("Name: %s, Model: %s"), cellhead->inst_name, cellhead->model_name);
		}
		return;
	}
	ttyputbadusage(x_("telltool simulation als print"));
}

CHAR *simals_strengthstring(INTSML strength)
{
	if (strength == OFF_STRENGTH) return(_("off"));
	if (strength <= NODE_STRENGTH) return(_("node"));
	if (strength <= GATE_STRENGTH) return(_("gate"));
	return(_("power"));
}

/*
 * Name: simals_print_in_entry
 *
 * Description:
 *	This procedure examines an input entry and prints out the condition
 * that it represents.  It is possible for an input entry operand to represent
 * a logic value, integer value, or another node address.
 *
 * Calling Arguments:
 *	rowhead = pointer to the row being printed
 */
void simals_print_in_entry(ROWPTR rowhead)
{
	INTBIG	  num;
	CHAR	  flag, s1[15], *s2;
	UCHAR     operatr;
	IOPTR	  iohead;
	NODEPTR	  nodehead;
	REGISTER void *infstr;

	flag = 0;
	infstr = initinfstr();
	addstringtoinfstr(infstr, M_("  Input: "));

	for (iohead = rowhead->inptr; iohead; iohead = iohead->next)
	{
		if (flag) addstringtoinfstr(infstr, x_("& "));
		flag = 1;

		nodehead = (NODEPTR)iohead->nodeptr;
		(void)esnprintf(s1, 15, x_("N%ld"), nodehead->num);
		addstringtoinfstr(infstr, s1);

		if (iohead->operatr > 127)
		{
			operatr = iohead->operatr - 128;
			nodehead = (NODEPTR) iohead->operand;
			(void)esnprintf(s1, 15, x_("%cN%ld"), operatr, nodehead->num);
			addstringtoinfstr(infstr, s1);
			continue;
		}

		addtoinfstr(infstr, iohead->operatr);

		num = (INTBIG)iohead->operand;
		s2 = simals_trans_number_to_state(num);
		addstringtoinfstr(infstr, s2);
		addstringtoinfstr(infstr, x_(" "));
	}
	ttyputmsg(x_("%s"), returninfstr(infstr));
}

/*
 * Name: simals_print_out_entry
 *
 * Description:
 *	This procedure examines an output entry and prints out the condition
 * that it represents.  It is possible for an output entry operand to represent
 * a logic value, integer value, or another node address.
 *
 * Calling Arguments:
 *	rowhead = pointer to the row being printed
 */
void simals_print_out_entry(ROWPTR rowhead)
{
	INTBIG	  num;
	CHAR      flag, *s2, s1[50];
	UCHAR     operatr;
	IOPTR	  iohead;
	STATPTR	stathead;
	NODEPTR	nodehead;
	REGISTER void *infstr;

	flag = 0;
	infstr = initinfstr();
	addstringtoinfstr(infstr, M_("  Output: "));

	for (iohead = rowhead->outptr; iohead; iohead = iohead->next)
	{
		if (flag) addstringtoinfstr(infstr, x_("& "));
		flag = 1;

		stathead = (STATPTR)iohead->nodeptr;
		nodehead = stathead->nodeptr;
		(void)esnprintf(s1, 50, x_("N%ld"), nodehead->num);
		addstringtoinfstr(infstr, s1);

		if (iohead->operatr > 127)
		{
			operatr = iohead->operatr - 128;
			nodehead = (NODEPTR) iohead->operand;
			(void)esnprintf(s1, 50, x_("%cN%ld@%d "), operatr, nodehead->num, (iohead->strength+1)/2);
			addstringtoinfstr(infstr, s1);
			continue;
		}

		addtoinfstr(infstr, iohead->operatr);

		num = (INTBIG)iohead->operand;
		s2 = simals_trans_number_to_state(num);
		addstringtoinfstr(infstr, s2);
		(void)esnprintf(s1, 50, x_("@%d "), (iohead->strength+1)/2);
		addstringtoinfstr(infstr, s1);
	}
	ttyputmsg(x_("%s"), returninfstr(infstr));
}

/*
 * Name: simals_print_xref_entry
 *
 * Description:
 *	This procedure prints entries from the cross reference table that was
 * generated to transform the hierarchical network description into a totally flat
 * network description.  The calling arguments define the root of the reference
 * table column and the level of indentation for the column.
 *
 * Calling Arguments:
 *	cellhead = pointer to cross reference table
 *	tab	= integer value indicating level of output indentation
 */
void simals_print_xref_entry(CONPTR cellhead, INTBIG tab)
{
	INTBIG  i, k, delay;
	CHAR    tabsp[200], ts[256];
	EXPTR   exhead;
	CONPTR  subcell;
	REGISTER void *infstr;

	if (stopping(STOPREASONDISPLAY)) return;
	for (i = 0; i < tab; ++i) tabsp[i] = ' ';
	tabsp[tab] = 0;
	ttyputmsg(M_("%sLevel: %s, Model: %s"), tabsp, simals_compute_path_name(cellhead),
		cellhead->model_name);

	for (exhead = cellhead->exptr; exhead; exhead = exhead->next)
	{
		if (stopping(STOPREASONDISPLAY)) return;
		k = 0;
		infstr = 0;
		for (i=0; i<12; i++)
		{
			delay = exhead->td[i];
			if (delay != 0)
			{
				if (k == 0) infstr = initinfstr();
				(void)esnprintf(ts, 256, x_("%s=%ld "), simals_tnames[i], delay);
				addstringtoinfstr(infstr, ts);
				k++;
			}
		}
		if (k == 0) ttyputmsg(x_("%s%14s --> N%ld"), tabsp, exhead->node_name, exhead->nodeptr->num); else
			ttyputmsg(x_("%s%14s --> N%ld (%s)"), tabsp, exhead->node_name, exhead->nodeptr->num, returninfstr(infstr));
	}

	if (simals_instbuf[simals_instptr[1]] == 'X') return;

	for (subcell = cellhead->child; subcell; subcell = subcell->next)
		simals_print_xref_entry(subcell, tab + 10);
}

/****************************** SEED ******************************/

/*
 * Name: simals_seed_command
 *
 * Description:
 *	This procedure sets a flag which tells the simulator if it is necessary
 * to reseed the Random Number Generator each time a simulation is run.
 */
void simals_seed_command(INTBIG count, CHAR *par[])
{
	if (count < 1)
	{
		ttyputusage(x_("telltool simulation als seed (reset | no-reset)"));
		return;
	}
	if (namesamen(par[0], x_("reset"), estrlen(par[0])) == 0)
		simals_seed_flag = FALSE; else simals_seed_flag = TRUE;
}

/****************************** SET ******************************/

/*
 * Name: simals_set_command
 *
 * Description:
 *	This procedure sets the specified node to the state that is indicated
 * in the command line.
*/
void simals_set_command(INTBIG count, CHAR *par[])
{
	INTBIG	state;
	INTSML	strength;
	double   time;
	NODEPTR  nodehead;
	LINKPTR  sethead;
	NODEPROTO *np;

	if (sim_window_isactive(&np) == 0)
	{
		ttyputerr(_("No simulator active"));
		return;
	}
	if (count < 4)
	{
		ttyputusage(x_("telltool simulation als set NODE LEVEL STRENGTH TIME"));
		return;
	}
	simals_convert_to_upper(par[0]);
	nodehead = simals_find_node(par[0]);
	if (! nodehead)
	{
		ttyputerr(_("ERROR: Unable to find node %s"), par[0]);
		return;
	}

	state = simals_trans_state_to_number(par[1]);
	strength = (INTSML)simals_atoi(par[2])*2;
	time = eatof(par[3]);

	sethead = simals_alloc_link_mem();
	if (sethead == 0) return;
	sethead->type = 'N';
	sethead->ptr = (CHAR *)nodehead;
	sethead->state = state;
	sethead->strength = strength;
	sethead->priority = 2;
	sethead->time = time;
	sethead->right = 0;
	simals_insert_set_list(sethead);

	ttyputmsg(M_("Node '%s' scheduled, state = %s, strength = %s, time = %g"), par[0], par[1],
		simals_strengthstring(strength), time);
	(void)simals_initialize_simulator(FALSE);
}

/****************************** TRACE ******************************/

/*
 * Name: simals_trace_command
 *
 * Description:
 *	This procedure turns on/off the trace buffer.  If it is turned off, no
 * timing diagram information will be stored in memory for plotting.
 */
void simals_trace_command(INTBIG count, CHAR *par[])
{
	INTBIG l;
	CHAR *pt;

	if (count < 1)
	{
		ttyputusage(x_("telltool simulation als trace OPTION"));
		return;
	}
	l = estrlen(pt = par[0]);
	if (namesamen(pt, x_("on"), l) == 0 && l >= 2)
	{
		(void)setvalkey((INTBIG)sim_tool, VTOOL, simals_no_update_key, 0, VINTEGER);
		return;
	}
	if (namesamen(pt, x_("off"), l) == 0 && l >= 2)
	{
		(void)setvalkey((INTBIG)sim_tool, VTOOL, simals_no_update_key, 1, VINTEGER);
		return;
	}
	ttyputbadusage(x_("telltool simulation als trace"));
}

/****************************** VECTOR ******************************/

void simals_vector_command(INTBIG count, CHAR *par[])
{
	INTBIG	 l, flag;
	INTSML   strength;
	CHAR     s1[256], *s2, *pt, **vectptr1, **backptr;
	CHAR    *filename, *truename;
	FILE    *vin, *vout;
	LINKPTR  sethead, vecthead, vectptr2, nextvec;
	ROWPTR   clokhead;
	NODEPTR  nodehead;
	double   time;
	NODEPROTO *np;
	REGISTER void *infstr;

	if (sim_window_isactive(&np) == 0)
	{
		ttyputerr(_("No simulator active"));
		return;
	}
	if (count < 1)
	{
		ttyputusage(x_("telltool simulation als vector OPTION"));
		return;
	}
	l = estrlen(pt = par[0]);

	if (namesamen(pt, x_("load"), l) == 0)
	{
		if (count < 2)
		{
			par[1] = fileselect(_("ALS vector file"), sim_filetypealsvec, x_(""));
			if (par[1] == 0) return;
		}
		vin = xopen(par[1], sim_filetypealsvec, x_(""), &filename);
		if (! vin)
		{
			ttyputerr(_("ERROR: Can't open %s"), par[1]);
			return;
		}

		/* clear all vectors */
		while (simals_setroot)
		{
			sethead = simals_setroot;
			simals_setroot = simals_setroot->right;
			if (sethead->type == 'C')
			{
				clokhead = (ROWPTR) sethead->ptr;
				for (vectptr2 = (LINKPTR) clokhead->inptr; vectptr2; vectptr2 = nextvec)
				{
					nextvec = vectptr2->right;
					simals_free_link_mem(vectptr2);
				}
				efree((CHAR *)clokhead);
			}
			simals_free_link_mem(sethead);
		}

		flag = 1;
		for(;;)
		{
			if (flag)
			{
				if (xfgets(s1, 255, vin))
				{
					xclose(vin);
					(void)simals_initialize_simulator(FALSE);
					break;
				}
				simals_convert_to_upper(s1);
				if (simals_fragment_command(s1)) break;
			}

			if (! estrcmp(simals_instbuf, x_("CLOCK")))
			{
				simals_convert_to_upper(&(simals_instbuf[simals_instptr[1]]));
				nodehead = simals_find_node(&(simals_instbuf[simals_instptr[1]]));
				if (! nodehead)
				{
					ttyputerr(_("ERROR: Unable to find node %s"),
						&(simals_instbuf[simals_instptr[1]]));
					flag = 1;
					continue;
				}
				strength = eatoi(&(simals_instbuf[simals_instptr[9]]))*2;

				sethead = simals_alloc_link_mem();
				if (sethead == 0) return;
				sethead->type = 'C';
				sethead->ptr = (CHAR*)(clokhead = (ROWPTR) simals_alloc_mem((INTBIG)sizeof(ROW)));
				if (sethead->ptr == 0) return;
				sethead->state = eatoi(&(simals_instbuf[simals_instptr[13]]));
				sethead->priority = 1;
				sethead->time = eatof(&(simals_instbuf[simals_instptr[11]]));
				sethead->right = 0;
				simals_insert_set_list(sethead);

				clokhead->delta = (float)eatof(&(simals_instbuf[simals_instptr[3]]));
				clokhead->linear = (float)eatof(&(simals_instbuf[simals_instptr[5]]));
				clokhead->exp = (float)eatof(&(simals_instbuf[simals_instptr[7]]));
				clokhead->abs = 0.0;
				clokhead->random = 0.0;
				clokhead->next = 0;
				clokhead->delay = 0;

				vectptr1 = (CHAR**) &(clokhead->inptr);
				for(;;)
				{
					if (xfgets(s1, 255, vin))
					{
						xclose(vin);
						(void)simals_initialize_simulator(FALSE);
						return;
					}
					simals_convert_to_upper(s1);
					if (simals_fragment_command(s1)) return;
					if (!estrcmp(simals_instbuf, x_("CLOCK")) || !estrcmp(simals_instbuf, x_("SET")))
					{
						flag = 0;
						break;
					}
					vectptr2 = simals_alloc_link_mem();
					if (vectptr2 == 0) return;
					vectptr2->type = 'N';
					vectptr2->ptr = (CHAR *)nodehead;
					vectptr2->state = simals_trans_state_to_number(simals_instbuf);
					vectptr2->strength = strength;
					vectptr2->priority = 1;
					vectptr2->time = eatof(&(simals_instbuf[simals_instptr[1]]));
					vectptr2->right = 0;
					*vectptr1 = (CHAR*) vectptr2;
					vectptr1 = (CHAR**) &(vectptr2->right);
				}
			}

			if (! estrcmp(simals_instbuf, x_("SET")))
			{
				simals_convert_to_upper(&(simals_instbuf[simals_instptr[1]]));
				nodehead = simals_find_node(&(simals_instbuf[simals_instptr[1]]));
				if (! nodehead)
				{
					ttyputerr(_("ERROR: Unable to find node %s"),
						&(simals_instbuf[simals_instptr[1]]));
					flag = 1;
					continue;
				}

				sethead = simals_alloc_link_mem();
				if (sethead == 0) return;
				sethead->type = 'N';
				sethead->ptr = (CHAR *) nodehead;
				sethead->state = simals_trans_state_to_number(&(simals_instbuf[simals_instptr[2]]));
				sethead->strength = eatoi(&(simals_instbuf[simals_instptr[3]]))*2;
				sethead->priority = 2;
				sethead->time = eatof(&(simals_instbuf[simals_instptr[5]]));
				sethead->right = 0;
				simals_insert_set_list(sethead);
				flag = 1;
			}
		}
		return;
	}

	if (namesamen(pt, x_("new"), l) == 0)
	{
		/* clear all vectors */
		simals_clearallvectors(FALSE);
		(void)simals_initialize_simulator(FALSE);
		return;
	}

	if (namesamen(pt, x_("save"), l) == 0)
	{
		if (count < 2)
		{
			infstr = initinfstr();
			formatinfstr(infstr, x_("%s.vec"), el_curlib->libname);
			par[1] = fileselect(_("ALS vector file"), sim_filetypealsvec|FILETYPEWRITE,
				returninfstr(infstr));
			if (par[1] == 0) return;
		}
		vout = xcreate(par[1], sim_filetypealsvec, 0, &truename);
		if (vout == 0)
		{
			if (truename != 0) ttyputerr(_("ERROR: Can't create %s"), truename);
			return;
		}

		for (sethead = simals_setroot; sethead; sethead = sethead->right)
		{
			switch (sethead->type)
			{
				case 'C':
					clokhead = (ROWPTR)sethead->ptr;
					vecthead = (LINKPTR)clokhead->inptr;
					simals_compute_node_name((NODEPTR)vecthead->ptr, s1);
					xprintf(vout, x_("CLOCK %s D=%g L=%g E=%g "), s1, clokhead->delta,
						clokhead->linear, clokhead->exp);
					xprintf(vout, x_("STRENGTH=%d TIME=%g CYCLES=%ld\n"), vecthead->strength/2,
						sethead->time, sethead->state);
					for (; vecthead; vecthead = vecthead->right)
					{
						s2 = simals_trans_number_to_state(vecthead->state);
						xprintf(vout, x_("  %s %g\n"), s2, vecthead->time);
					}
					break;
				case 'N':
					simals_compute_node_name((NODEPTR)sethead->ptr, s1);
					s2 = simals_trans_number_to_state(sethead->state);
					xprintf(vout, x_("SET %s=%s@%d TIME=%g\n"), s1, s2, sethead->strength/2,
						sethead->time);
			}
		}
		xclose(vout);
		return;
	}

	if (namesamen(pt, x_("delete"), l) == 0)
	{
		if (count < 3)
		{
			ttyputusage(x_("telltool simulation als vector delete NODE OPTIONS"));
			return;
		}
		simals_convert_to_upper(par[1]);
		nodehead = simals_find_node(par[1]);
		if (! nodehead)
		{
			ttyputerr(_("ERROR: Unable to find node %s"), par[1]);
			return;
		}

		backptr = (CHAR**) &simals_setroot;
		sethead = simals_setroot;

		if (par[2][0] == 'a')
		{
			while (sethead)
			{
				if (sethead->type == 'C')
				{
					clokhead = (ROWPTR)sethead->ptr;
					vecthead = (LINKPTR)clokhead->inptr;
					if ((NODEPTR)vecthead->ptr == nodehead)
					{
						*backptr = (CHAR *)sethead->right;
						simals_free_link_mem(sethead);
						sethead = (LINKPTR)*backptr;
						efree((CHAR *)clokhead);
						for (; vecthead; vecthead = nextvec)
						{
							nextvec = vecthead->right;
							simals_free_link_mem(vecthead);
						}
						continue;
					}
				} else
				{
					if ((NODEPTR)sethead->ptr == nodehead)
					{
						*backptr = (CHAR *)sethead->right;
						simals_free_link_mem(sethead);
						sethead = (LINKPTR)*backptr;
						continue;
					}
				}

				backptr = (CHAR**) &(sethead->right);
				sethead = sethead->right;
			}
			(void)simals_initialize_simulator(FALSE);
			return;
		}

		if (count < 4)
		{
			ttyputusage(x_("telltool simulation als vector delete time TIME"));
			return;
		}
		time = eatof(par[2]);
		while (sethead)
		{
			if (sethead->time == time)
			{
				if (sethead->type == 'C')
				{
					clokhead = (ROWPTR)sethead->ptr;
					vecthead = (LINKPTR)clokhead->inptr;
					if ((NODEPTR)vecthead->ptr == nodehead)
					{
						*backptr = (CHAR*)sethead->right;
						simals_free_link_mem(sethead);
						sethead = (LINKPTR)*backptr;
						efree((CHAR *)clokhead);
						for (; vecthead; vecthead = nextvec)
						{
							nextvec = vecthead->right;
							simals_free_link_mem(vecthead);
						}
						(void)simals_initialize_simulator(FALSE);
						return;
					}
				} else
				{
					if ((NODEPTR)sethead->ptr == nodehead)
					{
						*backptr = (CHAR *)sethead->right;
						simals_free_link_mem(sethead);
						sethead = (LINKPTR)*backptr;
						(void)simals_initialize_simulator(FALSE);
						return;
					}
				}
			}

			backptr = (CHAR**) &(sethead->right);
			sethead = sethead->right;
		}
		return;
	}

	ttyputbadusage(x_("telltool simulation als vector"));
}

/****************************** ANNOTATE ******************************/

/*
 * Name: simals_annotate_command
 *
 * Description:
 *   Annotate node information onto corresponding schematic.
 */
void simals_annotate_command(INTBIG count, CHAR *par[])
{
	if (count < 1)
	{
		ttyputusage(x_("telltool simulation als annotate [minimum | typical | maximum]"));
		return;
	}

	if (simals_levelptr == 0)
	{
		ttyputerr(M_("Must start simulator before annotating delay information"));
		return;
	}

	if (namesamen(par[0], x_("min"), 3) == 0) simals_sdfdelaytype = DELAY_MIN;
		else if (namesamen(par[0], x_("typ"), 3) == 0) simals_sdfdelaytype = DELAY_TYP;
			else if (namesamen(par[0], x_("max"), 3) == 0) simals_sdfdelaytype = DELAY_MAX;
				else
	{
		ttyputbadusage(x_("telltool simulation als annotate"));
		return;
	}

	simals_sdfannotate(simals_levelptr);
	simals_update_netlist();
	ttyputmsg(M_("Completed annotation of SDF %s delay values"), par[0]);
}

/*
 * Routine to annotate SDF port delay info onto ALS netlist.
 */
void simals_sdfannotate(CONPTR cellhead)
{
	CHAR    *s1;
	CONPTR  subcell;
	NODEINST *ni;

	if (stopping(STOPREASONDISPLAY)) return;
	s1 = simals_compute_path_name(cellhead);

	ni = simals_getcellinstance(cellhead->model_name, s1);
	if (ni != NONODEINST)
	{
		simals_sdfportdelay(cellhead, ni, s1);
	}

	if (simals_instbuf[simals_instptr[1]] == 'X') return;

	for (subcell = cellhead->child; subcell; subcell = subcell->next)
		simals_sdfannotate(subcell);
}

/*
 * Routine to get a NODEINST for specified cell instance.
 */
NODEINST *simals_getcellinstance(CHAR *celltype, CHAR *instance)
{
	NODEINST *ni;
	NODEPROTO *np;
	VARIABLE *var;
	CHAR *pt, **instlist, *str, tmp[256];
	INTBIG i, j, count = 1;
	Q_UNUSED( celltype );

	np = getcurcell();
	ni = NONODEINST;

	/* count number of hierarchy levels */
	(void)esnprintf(tmp, 256, x_("%s"), instance);
	for (pt = tmp; *pt != 0; pt++) if (*pt == '.') count++;

	/* separate out each hiearchy level - skip first level which is the top */
	instlist = (CHAR **)emalloc(count * (sizeof(CHAR *)), el_tempcluster);
	pt = instance;
	for (i=0, j=0; i<count; i++)
	{
		str = getkeyword(&pt, x_("."));
		if (i >= 2) if (allocstring(&instlist[j++], str, el_tempcluster))
			return(NONODEINST);
		(void)tonextchar(&pt);
	}
	count -= 2;

	if (count == 0) return(NONODEINST);

	/* find the NODEINST corresponding to bottom level of hierarchy */
	for(i=0; i<count; i++)
	{
		for (ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
			if (var == NOVARIABLE) continue;
			if (namesame((CHAR *)var->addr, instlist[i]) != 0) continue;
			np = ni->proto;
			break;
		}
		if (ni == NONODEINST) break;
		if (np->primindex != 0) break;
	}

	return(ni);
}

/*
 * Routine to extract SDF port delay information and annotate it to ALS netlist.
 */
void simals_sdfportdelay(CONPTR cellhead, NODEINST *ni, CHAR *path)
{
	VARIABLE *var;
	PORTARCINST *pi;
	INTBIG len, i, j, delay;
	EXPTR exhead;

	for (exhead = cellhead->exptr; exhead; exhead = exhead->next)
	{
		for (pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			if (namesame(pi->proto->protoname, exhead->node_name) == 0)
			{
				var = getval((INTBIG)pi, VPORTARCINST, VSTRING|VISARRAY, x_("SDF_absolute_port_delay"));
				if (var != NOVARIABLE)
				{
					len = getlength(var);
					for (i=0; i<len; i++)
					{
						if (namesamen(path, ((CHAR **)var->addr)[i], estrlen(path)) == 0)
						{
							for (j=0; j<12; j++)
							{
								delay = simals_getportdelayvalue(((CHAR **)var->addr)[i],
									simals_tnames[j], simals_sdfdelaytype);
								if (delay != -1) exhead->td[j] = delay; else
									exhead->td[j] = 0;
							}
						}
					}
				}
			}
		}
	}
}

/*
 * Routine to extract delay value from delay data string.
 */
INTBIG simals_getportdelayvalue(CHAR *datastring, CHAR *transition, DELAY_TYPES delaytype)
{
	CHAR *pt, *ts, **instlist, *str, tmp[256];
	INTBIG i, count = 1;

	/* count number of parts in data string */
	(void)esnprintf(tmp, 256, x_("%s"), datastring);
	for (pt = tmp; *pt != 0; pt++) if (*pt == ' ') count++;

	/* split data string into separate pieces */
	instlist = (CHAR **)emalloc(count * (sizeof(CHAR *)), el_tempcluster);
	pt = datastring;
	for (i=0; i<count; i++)
	{
		str = getkeyword(&pt, x_(" "));
		if (allocstring(&instlist[i], str, el_tempcluster)) return(-1);
	}

	/* get piece that corresponds to specified transition */
	for (i=0; i<count; i++)
	{
		if (namesamen(instlist[i], transition, estrlen(transition)) == 0)
		{
			if (allocstring(&ts, instlist[i], el_tempcluster)) return(-1);
			return(simals_getdval(ts, delaytype));
		}
	}

	return(-1);
}

/*
 * Routine to get a delay value string from a transition string.
 *     if tstring is '01(111:222:333)' and delaytype is DELAY_TYP return 222
 */
INTBIG simals_getdval(CHAR *tstring, DELAY_TYPES delaytype)
{
	CHAR *pt, *str, *bs, ts[256], *t1, *t2, *t3;
	INTBIG i, start = 0, stop = 0;

	bs = str = (CHAR *)emalloc((estrlen(tstring)+1) * SIZEOFCHAR, el_tempcluster);
	for (pt = tstring; *pt != 0; pt++)
	{
		if (*pt == ')') stop++;
		if (start && !stop) *str++ = *pt;
		if (*pt == '(') start++;
	}
	*str = 0;

	(void)esnprintf(ts, 256, x_("%s"), bs);

	/* delay string is not a triple, only one delay value implies typical */
	if (estrstr(ts, x_(":")) == NULL)
	{
		if (delaytype == DELAY_TYP) return(eatoi(ts));
		return(-1);
	}

	pt = ts;
	for (i=0; i<3; i++)
	{
		str = getkeyword(&pt, x_(":"));
		if (i == 0) if (allocstring(&t1, str, el_tempcluster)) return(-1);
		if (i == 1) if (allocstring(&t2, str, el_tempcluster)) return(-1);
		if (i == 2) if (allocstring(&t3, str, el_tempcluster)) return(-1);
		(void)tonextchar(&pt);
	}

	switch (delaytype)
	{
		case DELAY_MIN:
			return(eatoi(t1));
		case DELAY_TYP:
			return(eatoi(t2));
		case DELAY_MAX:
			return(eatoi(t3));
		default:
			return(-1);
	}
}

/****************************** ORDER ******************************/

/*
 * Name: simals_order_command
 *
 * Description:
 *   Save/restore signal trace order for waveform display.
 */
void simals_order_command(INTBIG count, CHAR *par[])
{
	if (count < 1)
	{
		ttyputusage(x_("telltool simulation als order [save | restore]"));
		return;
	}

	if (namesamen(par[0], x_("sav"), 3) == 0) simals_order_save();
		else if (namesamen(par[0], x_("res"), 3) == 0)
	{
		if (count != 2)
		{
			ttyputusage(x_("telltool simulation als order restore OPTION"));
			return;
		}
		simals_order_restore(par[1]);
	} else
	{
		ttyputbadusage(x_("telltool simulation als order"));
		return;
	}
}

void simals_order_save(void)
{
	INTBIG tr;
	BOOLEAN first = FALSE;
	CHAR str[256], *ts;
	NODEPROTO *curcell;
	REGISTER void *infstr = 0;

	sim_window_inittraceloop();
	while ((tr = sim_window_nexttraceloop()) != 0)
	{
		if (!first) infstr = initinfstr();
		(void)esnprintf(str, 256, x_("%s:"), sim_window_gettracename(tr));
		addstringtoinfstr(infstr, str);
		first = TRUE;
	}

	if (first)
	{
		ts = returninfstr(infstr);
		ts[(estrlen(ts)-1)] = 0;  /* chop off trailing ":" */

		/* save on current cell */
		curcell = getcurcell();
		if (curcell != NULL) (void)setval((INTBIG)curcell, VNODEPROTO,
			x_("SIM_als_trace_order"), (INTBIG)ts, VSTRING);
	}
}

void simals_order_restore(CHAR *list)
{
	INTBIG tc = 0, i, found, lines, thispos, pos, fromlib = 0;
	INTBIG tr, trl;
	NODEPTR node;
	VARIABLE *var;
	NODEPROTO *curcell;
	CHAR *pt, *str, **tl, tmp[256];

	if (namesame(list, x_("fromlib")) == 0)
	{
		curcell = getcurcell();
		if (curcell != NONODEPROTO)
		{
			var = getval((INTBIG)curcell, VNODEPROTO, VSTRING, x_("SIM_als_trace_order"));
			if (var != NOVARIABLE)
			{
				(void)esnprintf(tmp, 256, x_("%s"), (CHAR *)var->addr);
				fromlib++;
			}
			else return;
		}
		else return;
	}
	else (void)esnprintf(tmp, 256, x_("%s"), list);

	/* count number of traces and fill trace list array */
	for (pt = tmp; *pt != 0; pt++) if (*pt == ':') tc++;
	if (tc == 0) return;
	tc++;
	tl = (CHAR **)emalloc(tc * (sizeof(CHAR *)), el_tempcluster);
	pt = tmp;
	for (i=0; i<tc; i++)
	{
		str = getkeyword(&pt, x_(":"));
		(void)allocstring(&tl[i], str, el_tempcluster);
		(void)tonextchar(&pt);
	}

	/* delete traces not in restored list */
	sim_window_cleartracehighlight();
	sim_window_inittraceloop();
	while ((tr = sim_window_nexttraceloop()) != 0)
	{
		found = 0;
		for (i=0; i<tc; i++)
		{
			if (namesame(sim_window_gettracename(tr), tl[i]) == 0) found++;
		}
		if (!found)
		{
			thispos = sim_window_gettraceframe(tr);
			sim_window_inittraceloop2();
			for(;;)
			{
				trl = sim_window_nexttraceloop2();
				if (trl == 0) break;
				pos = sim_window_gettraceframe(trl);
				if (pos > thispos) sim_window_settraceframe(trl, pos-1);
			}
			lines = sim_window_getnumframes();
			if (lines > 1) sim_window_setnumframes(lines-1);

			/* remove from the simulator's list */
			for(i=0; i<simals_levelptr->num_chn; i++)
			{
				node = simals_levelptr->display_page[i+1].nodeptr;
				if (node == 0) continue;
				if (simals_levelptr->display_page[i+1].displayptr == tr)
				{
					simals_levelptr->display_page[i+1].displayptr = 0;
					break;
				}
			}

			/* kill trace, redraw */
			sim_window_killtrace(tr);
			sim_window_setnumframes(sim_window_getnumframes()-1);
		}
	}

	/* order the traces */
	sim_window_setnumframes(tc);
	sim_window_inittraceloop();
	while ((tr = sim_window_nexttraceloop()) != 0)
	{
		for (i=0; i<tc; i++)
		   if (namesame(tl[i], sim_window_gettracename(tr)) == 0) break;
		if (fromlib) sim_window_settraceframe(tr, tc-i-1); else  /* order from library save is bottom to top */
			sim_window_settraceframe(tr, i);
	}

	sim_window_redraw();
}

/*
 * Update the flattened netlist with the annotated delay values.
 */
void simals_update_netlist(void)
{
	MODPTR primhead;
	CONPTR cellhead;
	ROWPTR rowhead;
	EXPTR exhead;
	INTBIG delay, max_delay;

	for (primhead = simals_primroot; primhead; primhead = primhead->next)
	{
		switch(primhead->type)
		{
			case 'F':
				break;

			case 'G':
				/* cycle through all entries in table */
				for (rowhead = (ROWPTR)primhead->ptr; rowhead; rowhead=rowhead->next)
				{
					/* check for valid delay transition name for current entry */
					if (estrcmp(rowhead->delay, x_("XX")))
					{
						/* TESTING - get the max delay value of all input ports matching transition */
						cellhead = simals_find_level(simals_parent_level(primhead->level));
						max_delay = 0;
						for (exhead = cellhead->exptr; exhead; exhead = exhead->next)
						{
							delay = exhead->td[simals_get_tdindex(rowhead->delay)];
							if (max_delay < delay) max_delay = delay;
						}
						if (max_delay != 0)
						{
							rowhead->abs = (float)max_delay * 1.0e-12f;
						}
						ttyputmsg(M_("*** DEBUG *** gate: %s, level: %s, delay: %g(%s)"),
							primhead->name, primhead->level, (float)max_delay * 1.0e-12, rowhead->delay);
						ttyputmsg(M_("  Timing: D=%g, L=%g, E=%g, R=%g, A=%g"),
							rowhead->delta, rowhead->linear, rowhead->exp, rowhead->random, rowhead->abs);
						simals_print_in_entry(rowhead);
						simals_print_out_entry(rowhead);
					}
				}
				break;

			default:
				ttyputerr(M_("Illegal primitive type '%c', database is bad"), primhead->type);
				break;
		}
	}
}

/*
 * Return index for transition delays given text name.
 */
INTBIG simals_get_tdindex(CHAR *name)
{
	INTBIG i;

	for (i=0; i<12; i++)
	{
		if (!estrcmp(simals_tnames[i], name)) return(i);
	}
	return(0);  /* return '01' index */
}

/*
 * Return the parent level of the given child.
 *     if .TOP.NODE3.G1 is child, .TOP.NODE3 is parent
 */
CHAR *simals_parent_level(CHAR *child)
{
	CHAR tmp[256], *pt, *str, **instlist;
	INTBIG i, count = 1;
	REGISTER void *infstr;

	(void)esnprintf(tmp, 256, x_("%s"), child);
	for (pt = tmp; *pt != 0; pt++) if (*pt == '.') count++;

	/* separate out each hiearchy level */
	instlist = (CHAR **)emalloc(count * (sizeof(CHAR *)), el_tempcluster);
	pt = child;
	for (i=0; i<count; i++)
	{
		str = getkeyword(&pt, x_("."));
		(void)allocstring(&instlist[i], str, el_tempcluster);
		(void)tonextchar(&pt);
	}

	/* create the parent level name */
	infstr = initinfstr();
	for (i=0; i<count-1; i++)
	{
		addstringtoinfstr(infstr, instlist[i]);
		if (i != (count - 2)) addstringtoinfstr(infstr, x_("."));
	}

	return(returninfstr(infstr));
}

#endif  /* SIMTOOL - at top */
