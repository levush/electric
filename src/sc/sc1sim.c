/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: sc1sim.c
 * Modules concerned with simulation output for the QUISC Silicon Compiler
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

int sc_sim_format = SC_SILOS_FORMAT;

/* prototypes for local routines */
static int   Sc_simformat(int, CHAR*[]);
static int   Sc_simset(int, CHAR*[]);
static int   Sc_simshow(int, CHAR*[]);
static int   Sc_simwrite(int, CHAR*[]);
static void  Sc_simwrite_clear_flags(void);
static int   Sc_simwrite_model(SCCELL*, CHAR*, FILE*);
static int   Sc_simwrite_subelements(SCNITREE*, FILE*);
static int   Sc_simwrite_cell_instances(SCCELL*, FILE*);
static CHAR *Sc_sim_isaport(SCCELL*, CHAR*);
static int   Sc_simwrite_gate(CHAR*, FILE*);
static void  Sc_fprintf_80(FILE *fp, CHAR *fstring, ...);

/***********************************************************************
Module:  Sc_simulation
------------------------------------------------------------------------
Description:
	Interprete and execute the simulation command.  Valid options are:
		1.  set - set a cell's model
		2.  show - show a cell's model
		3.  write - output the simulation file
		4.  format - select the output format (silos or als (default)
------------------------------------------------------------------------
*/

int Sc_simulation(int count, CHAR *pars[])
{
	int		l;

	if (!count)
		return(Sc_seterrmsg(SC_SIMNPARS));
	l = estrlen(pars[0]);

	if (namesamen(pars[0], x_("set"), l) == 0)
		return(Sc_simset(count-1, &pars[1]));
	if (namesamen(pars[0], x_("show"), l) == 0)
		return(Sc_simshow(count-1, &pars[1]));
	if (namesamen(pars[0], x_("write"), l) == 0)
		return(Sc_simwrite(count-1, &pars[1]));
	if (namesamen(pars[0], x_("format"), l) == 0)
		return(Sc_simformat(count-1, &pars[1]));
	return(Sc_seterrmsg(SC_SIMXCMD, pars[0]));
}

/*************************************************************
Module:  Sc_simformat
-------------------------------------------------------------
Description: Set the simulator format to be used
------------------------------------------------------------------
*/

int Sc_simformat(int count, CHAR *pars[])
{
	if (!count)
		return(Sc_seterrmsg(SC_SIMNPARS));
	if (namesamen(pars[0], x_("als"), 3) == 0)
	{
		sc_sim_format = SC_ALS_FORMAT;
		return(SC_NOERROR);
	}
	if (namesamen(pars[0], x_("silos"), 5) == 0)
	{
		sc_sim_format = SC_SILOS_FORMAT;
		return(SC_NOERROR);
	}
	return(Sc_seterrmsg(SC_UNKNOWN, pars[0]));
}

/***********************************************************************
Module:  Sc_simset
------------------------------------------------------------------------
Description:
	Handle the setting of the simulation model for a node and/or cell.
------------------------------------------------------------------------
*/

int Sc_simset(int count, CHAR *pars[])
{
	SCCELL	*cell;
	CHAR	**simlist, buffer[MAXLINE];
	CHAR	*leafcell, *filename;
	SCSIM	*simline, *oldline, *startline;
	int		type, i;
	FILE	*fp;

	if (!count)
		return(Sc_seterrmsg(SC_SIMSETNPARS));

	/* check for a complex cell */
	for (cell = sc_cells; cell; cell = cell->next)
	{
		if (namesame(pars[0], cell->name) == 0) break;
	}
	if (cell == NULL)
	{
		/* try to find leaf cell in current library */
		if ((leafcell = Sc_find_leaf_cell(pars[0])) == NULL)
			return(Sc_seterrmsg(SC_CELLNOFIND, pars[0]));
		type = SCLEAFCELL;
	} else
	{
		type = SCCOMPLEXCELL;
	}

	/* check if simulation information already exits */
	if (type == SCCOMPLEXCELL)
	{
		if (cell->siminfo)
		{
			if ((fp = xcreate(SCSIMFILE, sc_filetypescsim, _("Simulation File"), 0)) == NULL)
				return(Sc_seterrmsg(SC_SIMSETFERROR));
			for (simline = (SCSIM *)cell->siminfo; simline; simline = simline->next)
				xprintf(fp, x_("%s\n"), simline->model);
			xclose(fp);
		}
	} else
	{
		/* write out simulation information for leafcell if it exists */
		if ((simlist = Sc_leaf_cell_sim_info(leafcell)))
		{
			if ((fp = xcreate(SCSIMFILE, sc_filetypescsim, _("Simulation File"), 0)) == NULL)
				return(Sc_seterrmsg(SC_SIMSETFERROR));
			for (i = 0; simlist[i]; i++)
				xprintf(fp, x_("%s\n"), simlist[i]);
			xclose(fp);
		}
	}

	/* get lines */
	if ((fp = xopen(SCSIMFILE, sc_filetypescsim, x_(""), &filename)) == NULL)
		return(Sc_seterrmsg(SC_SIMSETFERROR));
	oldline = NULL;
	startline = NULL;
	for(;;)
	{
		if (xfgets(buffer, MAXLINE, fp)) break;
		/*
		 * BUG ALERT:
		 *    xfgets has a bug, and strips of the '\n'.
		 *    Fixing the bug causes problems elsewhere.
		 *    Therefore, check for '\n' before stripping it off.
		 * LWS 12/11/89
		 */
		i = estrlen(buffer);
		if (buffer[i-1] == '\n') buffer[i-1] = '\0';
		/* buffer[strlen(buffer) - 1] = NULL; */	/* strip off newline */
		simline = (SCSIM *)emalloc(sizeof(SCSIM), sc_tool->cluster);
		if (simline == 0)
			return(Sc_seterrmsg(SC_NOMEMORY));
		if (oldline)
		{
			oldline->next = simline;
		} else
		{
			startline = simline;
		}
		if (allocstring(&simline->model, buffer, sc_tool->cluster))
			return(Sc_seterrmsg(SC_NOMEMORY));
		simline->next = NULL;
		oldline = simline;
	}
	xclose(fp);

	/* set similation information based on object type */
	if (type == SCCOMPLEXCELL)
	{
		if (cell->siminfo)
		{
			for (simline = (SCSIM *)cell->siminfo; simline; )
			{
				if (simline->model)
					efree((CHAR *)simline->model);
				oldline = simline;
				simline = simline->next;
				efree((CHAR *)oldline);
			}
			cell->siminfo = NULL;
		}
		cell->siminfo = startline;
	} else
	{
		(void)Sc_leaf_cell_set_sim(startline, leafcell);
	}
	return(0);
}

/***********************************************************************
Module:  Sc_simshow
------------------------------------------------------------------------
Description:
	Handle the showing of the simulation model for a complex or leaf cell.
------------------------------------------------------------------------
*/

int Sc_simshow(int count, CHAR *pars[])
{
	SCCELL		*cell;
	SCSIM		*simline;
	CHAR		**simlist, *leafcell, simhdr[10];
	int			i;

	if (!count)
		return(Sc_seterrmsg(SC_SIMSHOWNPARS));

	if (sc_sim_format == SC_ALS_FORMAT)
	{
		estrcpy(simhdr, x_("ALS"));
	} else if (sc_sim_format == SC_SILOS_FORMAT)
	{
		estrcpy(simhdr, x_("SILOS"));
	}
	for (cell = sc_cells; cell; cell = cell->next)
	{
		if (namesame(pars[0], cell->name) == 0)
			break;
	}
	if (cell == NULL)
	{
		if ((leafcell = Sc_find_leaf_cell(pars[0])) == NULL)
			return(Sc_seterrmsg(SC_CELLNOFIND, pars[0]));
		ttyputmsg(_("%s SIMULATION Model for leaf cell %s:"), simhdr, pars[0]);
		if ((simlist = Sc_leaf_cell_sim_info(leafcell)))
		{
			for (i = 0; simlist[i]; i++)
			{
				if (*(simlist[i]))
				{
					ttyputmsg(simlist[i]);
				} else
				{
					ttyputmsg(x_(" "));
				}
			}
		}
	} else
	{
		ttyputmsg(_("%s SIMULATION Model for cell %s:"), simhdr, pars[0]);
		for (simline = cell->siminfo; simline; simline = simline->next)
		{
			if (*(simline->model))
			{
				ttyputmsg(simline->model);
			} else
			{
				ttyputmsg(x_(" "));
			}
		}
	}
	return(0);
}

/***********************************************************************
Module:  Sc_simwrite
------------------------------------------------------------------------
Description:
	Handle the writing of the simulation file for a cell.
------------------------------------------------------------------------
*/

int Sc_simwrite(int count, CHAR *pars[])
{
	CHAR	*strtime, outfile[60];
	FILE	*fp;
	SCCELL	*cell;
	time_t	tsec;
	int		err;
	SCPORT	*port;

	if (count < 1)
		return(Sc_seterrmsg(SC_SIMWRITENPARS));

	/* search for cell in current complex cell list */
	for (cell = sc_cells; cell; cell = cell->next)
	{
		if (namesame(pars[0], cell->name) == 0)
			break;
	}
	if (cell == NULL)
		return(Sc_seterrmsg(SC_CELLNOFIND, pars[0]));

	/* open output file */
	if (count > 1)
	{
		(void)estrcpy(outfile, pars[1]);
	} else
	{
		(void)estrcpy(outfile, cell->name);
		if (sc_sim_format == SC_ALS_FORMAT)
		{
			(void)estrcat(outfile, x_(".net"));
		} else if (sc_sim_format == SC_SILOS_FORMAT)
		{
			(void)estrcat(outfile, x_(".sil"));
		}
	}

	if ((fp = xcreate(outfile, sc_filetypescsim, _("Simulation File"), 0)) == NULL)
		return(Sc_seterrmsg(SC_SIMWRITEFOPEN, outfile));
	tsec = getcurrenttime();
	strtime = timetostring(tsec);
	Sc_simwrite_clear_flags();

	if (sc_sim_format == SC_ALS_FORMAT)
	{
		Sc_fprintf_80(fp, x_("#*************************************************\n"));
		Sc_fprintf_80(fp, x_("#  ALS Design System netlist file\n"));
		Sc_fprintf_80(fp, x_("#  File Created by Electric (vhdl-compiler) on:\n"));
		Sc_fprintf_80(fp, x_("#  %s\n"), strtime);
		Sc_fprintf_80(fp, x_("#  For main cell:    %s\n"), pars[0]);
		Sc_fprintf_80(fp, x_("#-------------------------------------------------\n\n"));
		err = Sc_simwrite_model(cell, x_("main"), fp);
		Sc_fprintf_80(fp, x_("\n#********* End of netlist file *******************\n"));
	} else if (sc_sim_format == SC_SILOS_FORMAT)
	{
		Sc_fprintf_80(fp, x_("$*************************************************\n"));
		Sc_fprintf_80(fp, x_("$  SILOS netlist file\n"));
		Sc_fprintf_80(fp, x_("$  File Created by Electric (vhdl-compiler) on:\n"));
		Sc_fprintf_80(fp, x_("$  %s\n"), strtime);
		Sc_fprintf_80(fp, x_("$  For main cell:    %s\n"), pars[0]);
		Sc_fprintf_80(fp, x_("$-------------------------------------------------\n\n"));
		Sc_fprintf_80(fp, x_(".GLOBAL power\npower .CLK 0 S1\n"));
		Sc_fprintf_80(fp, x_(".GLOBAL ground\nground .CLK 0 S0\n\n"));
		err = Sc_simwrite_model(cell, cell->name, fp);
		Sc_fprintf_80(fp, x_("(%s %s "),cell->name, cell->name);	/* top level call*/
		for (port = cell->ports; port; port = port->next)
		{
			switch (port->bits & SCPORTTYPE)
			{
				/* ports are actual */
				case SCPWRPORT:
				case SCGNDPORT:
					break;
				default:
					Sc_fprintf_80(fp, x_(" %s"), port->name);
					break;
			}
		}
		Sc_fprintf_80(fp, x_("\n"));
		Sc_fprintf_80(fp, x_("\n$********* End of netlist file *******************\n"));
	}
	xclose(fp);
	return(err);
}

/***********************************************************************
Module:  Sc_simwrite_clear_flags
------------------------------------------------------------------------
Description:
	Clear the simwrite required and written flags on all complex cells
	and leaf cells.
------------------------------------------------------------------------
*/

void Sc_simwrite_clear_flags(void)
{
	SCCELL	*cell;
	CHAR	*leafcell;
	int		*bits;

	/* clear flags on all complex cells */
	for (cell = sc_cells; cell; cell = cell->next)
		cell->bits &= ~SCSIMWRITEBITS;

	/* clear flags on all leafcells */
	for (leafcell = Sc_first_leaf_cell(); leafcell != NULL;
		leafcell = Sc_next_leaf_cell(leafcell))
	{
		bits = Sc_leaf_cell_bits_address(leafcell);
		*bits &= ~SCSIMWRITEBITS;
	}
}

/***********************************************************************
Module:  Sc_simwrite_model
------------------------------------------------------------------------
Description:
	Procedure to write a model netlist file.
------------------------------------------------------------------------
*/

int Sc_simwrite_model(SCCELL *cell, CHAR *name, FILE *fp)
{
	SCPORT	*port;
	int		err, first;
	SCSIM	*simline;

	cell->bits |= SCSIMWRITESEEN;
	if (cell->siminfo)
	{
		for (simline = cell->siminfo; simline; simline = simline->next)
		{
			if (*(simline->model))
			{
				ttyputmsg(simline->model);
			} else
			{
				ttyputmsg(x_(" "));
			}
		}
	} else
	{
		/* first print submodels */
		if ((err = Sc_simwrite_subelements(cell->nilist, fp)))
			return(err);
	}

	if (sc_sim_format == SC_ALS_FORMAT)
	{
		Sc_fprintf_80(fp, x_("model %s("), name);
	} else if (sc_sim_format == SC_SILOS_FORMAT)
	{
		Sc_fprintf_80(fp, x_(".MACRO %s"), name);
	}
	first = TRUE;
	for (port = cell->ports; port; port = port->next)
	{
		switch (port->bits & SCPORTTYPE)
		{
			case SCPWRPORT:
			case SCGNDPORT:
				break;
			default:
				if (sc_sim_format == SC_ALS_FORMAT)
				{
					if (first)
					{
						Sc_fprintf_80(fp, x_("%s"), port->name);
						first = FALSE;
					} else
					{
						Sc_fprintf_80(fp, x_(", %s"), port->name);
					}
				} else if (sc_sim_format == SC_SILOS_FORMAT)
				{
					Sc_fprintf_80(fp, x_(" %s"),
					Sc_sim_isaport(cell, port->name));
				}
				break;
		}
	}
	if (sc_sim_format == SC_ALS_FORMAT)
	{
		Sc_fprintf_80(fp, x_(")\n"));
	} else if (sc_sim_format == SC_SILOS_FORMAT)
	{
		Sc_fprintf_80(fp, x_("\n"));
	}

	/* print instances of cells */
	if (!(cell->siminfo))
	{
		if ((err = Sc_simwrite_cell_instances(cell, fp)))
			return(err);
	}

	if (sc_sim_format == SC_SILOS_FORMAT)
	{
		Sc_fprintf_80(fp, x_(".EOM\n\n"));
	} else
	{
		Sc_fprintf_80(fp, x_("\n"));
	}
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_simwrite_subelements
------------------------------------------------------------------------
Description:
	Recursively write any subelements of the node instance tree.
------------------------------------------------------------------------
*/

int Sc_simwrite_subelements(SCNITREE *nptr, FILE *fp)
{
	int		err;

	for (; nptr; nptr = nptr->next)
	{
		switch (nptr->type)
		{
			case SCCOMPLEXCELL:
				if (!(((SCCELL *)(nptr->np))->bits & SCSIMWRITESEEN))
				{
					if ((err = Sc_simwrite_model(((SCCELL *)(nptr->np)),
						((SCCELL *)(nptr->np))->name, fp)))
							return(err);
				}
				break;
			case SCLEAFCELL:
				if (!(Sc_leaf_cell_bits(nptr->np) & SCSIMWRITESEEN))
				{
					if ((err = Sc_simwrite_gate(nptr->np, fp)))
						return(err);
				}
				break;
			default:
				break;
		}
	}
	return(0);
}

/***********************************************************************
Module:  Sc_simwrite_cell_instances
------------------------------------------------------------------------
Description:
	Print instances of complex or leaf cells within a cell
------------------------------------------------------------------------
*/

int Sc_simwrite_cell_instances(SCCELL *cell, FILE *fp)
{
	int		first, power, ground;
	SCNIPORT	*port;
	SCNITREE    *nptr;

	nptr = cell->nilist;
	if (nptr == NULL)
		return(0);
	power = ground = FALSE;
	for ( ; nptr; nptr = nptr->next)
	{
		switch (nptr->type)
		{
			case SCCOMPLEXCELL:
				if (sc_sim_format == SC_ALS_FORMAT)
				{
					Sc_fprintf_80(fp, x_("%s: "), nptr->name);
					Sc_fprintf_80(fp, x_("%s("), ((SCCELL *)(nptr->np))->name);
				} else if (sc_sim_format == SC_SILOS_FORMAT)
				{
					Sc_fprintf_80(fp, x_("(%s %s"), nptr->name,
						((SCCELL *)(nptr->np))->name);
				}
				first = TRUE;
				for (port = nptr->ports; port; port = port->next)
				{
					if (sc_sim_format == SC_ALS_FORMAT)
					{
						if (first)
						{
							Sc_fprintf_80(fp, x_("%s"), port->ext_node->name);
							first = FALSE;
						} else
						{
							Sc_fprintf_80(fp, x_(", %s"), port->ext_node->name);
						}
					} else if (sc_sim_format == SC_SILOS_FORMAT)
					{
						if (port->ext_node->firstport->next == NULL)
						{
							Sc_fprintf_80(fp, x_(" .SKIP"));
						} else
						{
							Sc_fprintf_80(fp, x_(" %s"),
							Sc_sim_isaport(cell,port->ext_node->name));
						}
					}
					if (namesame(port->ext_node->name, x_("ground")) == 0)
					{
						ground = TRUE;
					} else if (namesame(port->ext_node->name, x_("power")) == 0)
					{
						power = TRUE;
					}
				}
				if (sc_sim_format == SC_ALS_FORMAT)
				{
					Sc_fprintf_80(fp, x_(")\n"));
				} else
				{
					Sc_fprintf_80(fp, x_("\n"));
				}
				break;
			case SCLEAFCELL:
				if (sc_sim_format == SC_ALS_FORMAT)
				{
					Sc_fprintf_80(fp, x_("%s: "), nptr->name);
					Sc_fprintf_80(fp, x_("%s("), Sc_leaf_cell_name(nptr->np));
				} else if (sc_sim_format == SC_SILOS_FORMAT)
				{
					Sc_fprintf_80(fp, x_("(%s %s "), nptr->name,
						Sc_leaf_cell_name(nptr->np));
				}
				first = TRUE;
				for (port = nptr->ports; port; port = port->next)
				{
					if (sc_sim_format == SC_ALS_FORMAT)
					{
						if (first)
						{
							Sc_fprintf_80(fp, x_("%s"), port->ext_node->name);
							first = FALSE;
						} else
						{
							Sc_fprintf_80(fp, x_(", %s"), port->ext_node->name);
						}
					} else if (sc_sim_format == SC_SILOS_FORMAT)
					{
						if (port->ext_node->firstport->next == NULL)
						{
							Sc_fprintf_80(fp, x_(" .SKIP"));
						} else
						{
							Sc_fprintf_80(fp, x_(" %s"),
							Sc_sim_isaport(cell, port->ext_node->name));
						}
					}
					if (namesame(port->ext_node->name, x_("ground")) == 0)
					{
						ground = TRUE;
					} else if (namesame(port->ext_node->name, x_("power")) == 0)
					{
						power = TRUE;
					}
				}
				if (sc_sim_format == SC_ALS_FORMAT)
				{
					Sc_fprintf_80(fp, x_(")\n"));
				} else
				{
					Sc_fprintf_80(fp, x_("\n"));
				}
				break;
			case SCSPECIALCELL:
			default:
				break;
		}
	}
	if (sc_sim_format == SC_ALS_FORMAT)
	{
		if (ground)
			Sc_fprintf_80(fp, x_("set ground=L@3\n"));
		if (power)
			Sc_fprintf_80(fp, x_("set power=H@3\n"));
	}
	return(0);
}

/**********************************************************************
Module: Sc_sim_isaport
-----------------------------------------------------------------------
Description:
	Determine if this instance port is also one of the ports on the
		model, and if it subscripted;
----------------------------------------------------------------------
Calling sequence:
	 Sc_sim_isaport(cell, portname)
Where: cell is the model (macro) in which the instance exists
	   portname is the name of this port

Returns: the original string, or one which is modified to replace
the '[' with '_'
----------------------------------------------------------------------
*/

CHAR *Sc_sim_isaport(SCCELL *cell, CHAR *portname)
{
	SCPORT *test;
	CHAR *ptr;
	static CHAR newname[100];

	if (estrchr(portname, '[') == 0) return(portname);

	for (test = cell->ports; test != NULL; test = test->next)
	{
		if (namesame(portname, test->name) == 0)
		{
			for(ptr = newname; *portname;  ptr++, portname ++)
			{
				if (*portname == '[')
				{
					estrcpy(ptr, x_("__"));
					ptr++;
				} else if (*ptr == ']')
				{
					continue;
				} else
				{
					*ptr = *portname;
				}
			}
			*ptr = 0;
			return(newname);
		}
	}
	return(portname);
}

/***********************************************************************
Module:  Sc_simwrite_gate
------------------------------------------------------------------------
Description:
	Write a gate description for the indicated leaf cell.
------------------------------------------------------------------------
*/

int Sc_simwrite_gate(CHAR *leafcell, FILE *fp)
{
	CHAR	**simlist;
	int		i, *bits;

	bits = Sc_leaf_cell_bits_address(leafcell);
	*bits |= SCSIMWRITESEEN;
	if ((simlist = Sc_leaf_cell_sim_info(leafcell)))
	{
		for (i = 0; simlist[i]; i++)
		{
			Sc_fprintf_80(fp, simlist[i]);
			Sc_fprintf_80(fp, x_("\n"));
		}
		Sc_fprintf_80(fp, x_("\n"));
		return(0);
	} else
	{
		return(Sc_seterrmsg(SC_SCVARNOFIND,Sc_leaf_cell_name(leafcell)));
	}
}

/***********************************************************************
Module:  Sc_fprintf_80
------------------------------------------------------------------------
Description:
	Print the passed format string and elements to the indicated file
	while keeping the maximum line length to less than 80.  It does this
	by inserting new lines where necessary.  Note that the number of
	elements is limited to eight.
	Modified Aug. 1989 (SRP) for SILOS continuation character.
------------------------------------------------------------------------
Calling Sequence:  Sc_fprintf_80(fp, fstring, arg1, arg2, arg3, ...);

Name			Type		Description
----			----		-----------
fp				*FILE		Pointer to output file.
fstring			*char		Pointer to formating string.
arg1,arg2,...	int			Generic values (pointers) to items.
------------------------------------------------------------------------
*/

void Sc_fprintf_80(FILE *fp, CHAR *fstring, ...)
{
	static int	lline = 0;
	int			length;
	CHAR		buff[256], *sptr, save[10];
	va_list		ap;

	var_start(ap, fstring);
	evsnprintf(buff, 256, fstring, ap);
	va_end(ap);
	length = estrlen(buff);
	sptr = buff;
	if ((lline + length) > 70)
	{
		/* insert newline at first white space */
		while (*sptr)
		{
			if (isspace(*sptr))
			{
				estrncpy(save, sptr, 3);
				estrcpy(sptr, x_("\n"));
				if (sc_sim_format == SC_SILOS_FORMAT)
				{
					estrcat(sptr, x_("+"));
					lline = 1;
				} else
				{
					lline = 0;
				}
				xprintf(fp, buff);
				estrncpy(sptr, save, 3);
				break;
			}
			sptr++;
		}
		if (*sptr == 0)
		{
			sptr = buff;
		}
	}
	xprintf(fp, sptr);
	for ( ; *sptr != 0; sptr++)
	{
		if (*sptr == '\n')
		{
			lline = 0;
		} else
		{
			lline++;
		}
	}
}

#endif  /* SCTOOL - at top */
