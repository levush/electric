/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: sc1.c
 * Main modules for the QUISC Silicon Compiler
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
#include "usrdiacom.h"
#include "sc1.h"
#include "sim.h"
#include <setjmp.h>
#include <math.h>

extern CHAR *sc_errmsg;
extern CHAR *sc_standard_errmsg[];
extern COMCOMP    sc_silcomp;

/*********** Global Variables ******************************************/

SCCELL			*sc_cells, *sc_curcell;
jmp_buf			sc_please_stop;

/* prototypes for local routines */
static int  Sc_Mytoupper(CHAR);
static int  Sc_parse(int, CHAR*[]);
static int  Sc_compile(int, CHAR*[]);
static int  Sc_show(int, CHAR*[]);
static void Sc_nitreelist(SCNITREE*, int);
static int  Sc_select(int, CHAR*[]);
static int  Sc_xset(int, CHAR*[]);
static int  Sc_library(int, CHAR*[]);
static int  Sc_order(int, CHAR*[]);
static int  Sc_pull(int, CHAR*[]);
static int  Sc_pull_inst(SCNITREE*, SCCELL*, int);
static int  Sc_help(int, CHAR*[]);

/***********************************************************************
Module:  Sc_initialize
------------------------------------------------------------------------
Description:
	Initialize routine for the Silicon Compiler tool.
------------------------------------------------------------------------
Calling Sequence:  Sc_initialize();
------------------------------------------------------------------------
*/

void Sc_initialize(void)
{
	sc_cells = NULL;
	sc_curcell = NULL;
}

/***********************************************************************
Module:  Sc_main
------------------------------------------------------------------------
Description:
	Main module of the Silicon Compiler.
------------------------------------------------------------------------
Calling Sequence:  Sc_main();
------------------------------------------------------------------------
*/

void Sc_main(void)
{
	int	   count, l, err;
	CHAR   *pars[PARS];

	ttyputmsg(M_("QUISC - Queen's University Interactive Silicon Compiler"));
	ttyputmsg(M_("Version %s, %s"), VERSION, DATE);

	if (setjmp(sc_please_stop))
	{
		Sc_clear_stop();
		ttyputmsg(M_("Command Aborted..."));
	}

	for (;;)
	{
		count = ttygetfullparam(M_("Enter command> "), &sc_silcomp, PARS, pars);
		if (count < 0) break;
		if (count == 0) continue;
		l = estrlen(pars[0]);
		l = maxi(l, 3);
		if ((namesamen(pars[0], x_("quit"), l) == 0) && l >= 0) break;
		if ((err = Sc_parse(count, pars)))
			ttyputmsg(x_("SCERR %d - %s"), err, sc_errmsg);
	}
}

/***********************************************************************
Module:  Sc_one_command
------------------------------------------------------------------------
Description:
	Execute one command as specified in the passed parameters.
------------------------------------------------------------------------
Calling Sequence:  Sc_one_command(count, pars);

Name		Type		Description
----		----		-----------
count		int			Number of parameters.
pars		*char[]		Array of pointer to parameters.
------------------------------------------------------------------------
*/

void Sc_one_command(int count, CHAR *pars[])
{
	int		err;

	if (setjmp(sc_please_stop))
	{
		Sc_clear_stop();
		ttyputmsg(M_("Command Aborted..."));
		return;
	}

	if ((err = Sc_parse(count, pars)))
		ttyputmsg(x_("SCERR %d - %s"), err, sc_errmsg);
}

/***********************************************************************
Module:  Sc_Mytoupper
------------------------------------------------------------------------
Description:
	Convert a character to uppercase only if it is lowercase.
------------------------------------------------------------------------
Calling Sequence:  c = Sc_Mytoupper(c);

Name		Type		Description
----		----		-----------
c			char		Character to be converted.
------------------------------------------------------------------------
*/

int Sc_Mytoupper(CHAR c)
{
	if (islower(c)) c = toupper(c);
	return(c);
}

/***********************************************************************
Module:  Sc_seterrmsg
------------------------------------------------------------------------
Description:
	Set the error message using the format passed and the work space
	sc_swork.
------------------------------------------------------------------------
*/
int Sc_seterrmsg(int errnum, ...)
{
	static CHAR sc_swork[MAXLINE];
	va_list ap;

	var_start(ap, errnum);
	evsnprintf(sc_swork, MAXLINE, TRANSLATE(sc_standard_errmsg[errnum]), ap);
	va_end(ap);
	sc_errmsg = sc_swork;
	return(errnum);
}

/***********************************************************************
Module:  Sc_parse
------------------------------------------------------------------------
Description:
	Main parsing routine for the Silicon Compiler.
------------------------------------------------------------------------
*/

int Sc_parse(int count, CHAR *pars[])
{
	int		l;
	CHAR	*sptr;

	if (Sc_stop()) longjmp(sc_please_stop, 1);

	if (count == 0) return(0);
	l = estrlen(sptr = pars[0]);
	l = maxi(l, 3);

	if (namesamen(sptr, x_("compile"), l) == 0)
		return(Sc_compile(count-1, &pars[1]));

	if (namesamen(sptr, x_("connect"), l) == 0)
		return(Sc_connect(count-1, &pars[1]));

	if (namesamen(sptr, x_("create"), l) == 0)
		return(Sc_create(count-1, &pars[1]));

	if (namesamen(sptr, x_("delete"), l) == 0)
		return(Sc_delete());

	if (namesamen(sptr, x_("export"), l) == 0)
		return(Sc_export(count-1, &pars[1]));

	if (namesamen(sptr, x_("extract"), l) == 0)
		return(Sc_extract(count-1, &pars[1]));

	if (namesamen(sptr, x_("help"), l) == 0)
		return(Sc_help(count-1, &pars[1]));

	if (namesamen(sptr, x_("library"), l) == 0)
		return(Sc_library(count-1, &pars[1]));

	if (namesamen(sptr, x_("make"), l) == 0)
		return(Sc_maker(count - 1, &pars[1]));

	if (namesamen(sptr, x_("order"), l) == 0)
		return(Sc_order(count-1, &pars[1]));

	if (namesamen(sptr, x_("place"), l) == 0)
		return(Sc_place(count-1, &pars[1]));

	if (namesamen(sptr, x_("pull"), l) == 0)
		return(Sc_pull(count - 1, &pars[1]));

	if (namesamen(sptr, x_("route"), l) == 0)
		return(Sc_route(count - 1, &pars[1]));

	if (namesamen(sptr, x_("schematic"), l) == 0)
	{
		Sc_schematic();
		return(SC_NOERROR);
	}

	if (namesamen(sptr, x_("select"), l) == 0)
		return(Sc_select(count-1, &pars[1]));

	if (namesamen(sptr, x_("set"), l) == 0)
		return(Sc_xset(count-1, &pars[1]));

	if (namesamen(sptr, x_("show"), l) == 0)
		return(Sc_show(count-1, &pars[1]));

	if (namesamen(sptr, x_("simulation"), l) == 0)
		return(Sc_simulation(count-1, &pars[1]));

	if (namesamen(sptr, x_("verify"), l) == 0)
		return(Sc_verify());

	return(Sc_seterrmsg(SC_UNKNOWN, sptr));
}

/***********************************************************************
Module:  Sc_compile
------------------------------------------------------------------------
Description:
	Read the netlist associated with the current cell (either on disk or
	in the Netlist view of this cell).
------------------------------------------------------------------------
*/

int Sc_compile(int count, CHAR *pars[])
{
	int		lnum, icount, err, i;
	CHAR	inbuf[MAXLINE], *par[PARS], *sptr, *intended;
	extern TOOL *vhdl_tool;
	REGISTER NODEPROTO *np;
	Q_UNUSED( count );
	Q_UNUSED( pars );

	/* demand a QUISC netlist */
	np = getcurcell();
	if (np == NONODEPROTO)
	{
		ttyputerr(_("No current cell"));
		return(SC_NOERROR);
	}

	(void)asktool(vhdl_tool, x_("want-netlist-input"), (INTBIG)np, sim_filetypequisc);

	/* begin input of the netlist */
	i = asktool(vhdl_tool, x_("begin-netlist-input"), (INTBIG)np, sim_filetypequisc, (INTBIG)&intended);
	if (i != 0)
	{
		ttyputerr(_("Cannot find netlist in %s"), intended);
		return(SC_NOERROR);
	}
	ttyputmsg(_("Compiling netlist in %s..."), intended);

	/* read file until end is found */
	(void)Sc_cpu_time(TIME_RESET);
	for(;;)
	{
		lnum = asktool(vhdl_tool, x_("get-line"), (INTBIG)inbuf);
		if (lnum == 0) break;

		/* check for a comment line or empty line */
		if (*inbuf == '!' || estrlen(inbuf) == 0) continue;
		icount = 0;

		/* strip off last newline */
		sptr = &inbuf[estrlen(inbuf) - 1];
		if (*sptr == '\n') *sptr = 0;
		sptr = inbuf;
		while (*sptr)
		{
			/* get rid of leading white space */
			while (*sptr && (*sptr == ' ' || *sptr == '\t')) sptr++;
			if (*sptr == 0) break;

			/* check for string */
			if (*sptr == '"')
			{
				par[icount++] = ++sptr;

				/* move until end of string of second double quote */
				while (*sptr && *sptr != '"') sptr++;
				if (*sptr)
				{
					*sptr = '\0';
					sptr++;
				}
			} else
			{
				par[icount++] = sptr;

				/* move sptr until white space or end of line */
				sptr++;
				while (*sptr && *sptr != ' ' && *sptr != '\t') sptr++;
				if (*sptr)
				{
					*sptr = '\0';
					sptr++;
				}
			}
		}
		if ((err = Sc_parse(icount, par)))
		{
			ttyputmsg(_("ERROR line %d : SCERR %d - %s"), lnum, err, sc_errmsg);
			ttyputverbose(inbuf);
			ttyputmsg(_("Aborting compile..."));
			break;
		}
	}
	(void)asktool(vhdl_tool, x_("end-input"));
	ttyputmsg(_("Done (time = %s)"), Sc_cpu_time(TIME_ABS));
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_show
------------------------------------------------------------------------
Description:
	Show command parser.  Currently supported options include:
		1.  cells
		2.  nodes
		3.  ports
		4.  instances
		5.  placement
		6.  routing
------------------------------------------------------------------------
*/
int Sc_show(int count, CHAR *pars[])
{
	int         l, bits, all_flag;
	CHAR        *sptr, *ptype, pdir[40];
	CHAR	*leafcell, *leafport;
	SCPORT	*port;
	SCCELL	*cells;
	SCROWLIST	*row;
	SCNBPLACE	*place;
	SCCELLNUMS	cnums;

	if (count == 0)
		return(Sc_seterrmsg(SC_NSHOW));

	l = estrlen(sptr = pars[0]);
	l = maxi(l, 2);
	if (namesamen(sptr, x_("cells"), l) == 0)
	{
		ttyputmsg(M_("***** List of cells *****"));
		for (cells = sc_cells; cells; cells = cells->next)
		{
			if (cells == sc_curcell)
				ttyputmsg(x_(" -->%s"), cells->name); else
					ttyputmsg(x_("    %s"), cells->name);
		}
		ttyputmsg(M_("***** End of cell list *****"));
		return(0);
	}

	if (namesamen(sptr, x_("ports"), l) == 0) 
	{
		if (--count == 0)
		{
			if (sc_curcell == NULL) return(Sc_seterrmsg(SC_NOCELL));
			cells = sc_curcell;
			sptr = cells->name;
		} else
		{
			sptr = pars[1];

			/* try to find cell */
			for (cells = sc_cells; cells; cells = cells->next)
			{
				if (namesame(cells->name, sptr) == 0)
					break;
			}
			if (cells == NULL)
			{
				if ((leafcell = Sc_find_leaf_cell(sptr)) == NULL)
					return(Sc_seterrmsg(SC_CELLNOFIND, sptr));
			}
		}

		/* list all exports on the cell */
		ttyputmsg(M_("Ports for cell %s:"), sptr);
		if (cells != NULL)
		{
			for (port = cells->ports; port; port = port->next)
			{
				switch (port->bits & SCPORTTYPE)
				{
					case SCGNDPORT:
						ptype = M_("Ground port");
						break;
					case SCPWRPORT:
						ptype = M_("Power port");
						break;
					case SCBIDIRPORT:
						ptype = M_("Bidirectional port");
						break;
					case SCOUTPORT:
						ptype = M_("Output port");
						break;
					case SCINPORT:
						ptype = M_("Input port");
						break;
					default:
						ptype = M_("Unknown type");
						break;
				}
				*pdir = 0;
				bits = port->bits;
				if (bits & SCPORTDIRUP)
					(void)estrcat(pdir, M_(" up"));
				if (bits & SCPORTDIRDOWN)
					(void)estrcat(pdir, M_(" down"));
				if (bits & SCPORTDIRRIGHT)
					(void)estrcat(pdir, M_(" right"));
				if (bits & SCPORTDIRLEFT)
					(void)estrcat(pdir, M_(" left"));
				if (!(bits & SCPORTDIRMASK))
					(void)estrcat(pdir, M_(" none"));
				ttyputmsg(x_("  %-20s    %-20s    %-20s"), port->name,
					ptype, pdir);
			}
		} else
		{
			for (leafport = Sc_first_leaf_port(leafcell); leafport != NULL;
				leafport = Sc_next_leaf_port(leafport))
			{
				switch (Sc_leaf_port_type(leafport))
				{
					case SCGNDPORT:
						ptype = M_("Ground port");
						break;
					case SCPWRPORT:
						ptype = M_("Power port");
						break;
					case SCBIDIRPORT:
						ptype = M_("Bidirectional port");
						break;
					case SCOUTPORT:
						ptype = M_("Output port");
						break;
					case SCINPORT:
						ptype = M_("Input port");
						break;
					default:
						ptype = M_("Unknown type");
						break;
				}
				*pdir = 0;
				bits = Sc_leaf_port_bits(leafport);
				if (bits & SCPORTDIRUP)
					(void)estrcat(pdir, M_(" up"));
				if (bits & SCPORTDIRDOWN)
					(void)estrcat(pdir, M_(" down"));
				if (bits & SCPORTDIRRIGHT)
					(void)estrcat(pdir, M_(" right"));
				if (bits & SCPORTDIRLEFT)
					(void)estrcat(pdir, M_(" left"));
				if (!(bits & SCPORTDIRMASK))
					(void)estrcat(pdir, M_(" none"));
				ttyputmsg(x_("  %-20s    %-20s    %-20s"),
					Sc_leaf_port_name(leafport), ptype, pdir);
			}
		}
		ttyputmsg(M_("End of port list."));
		return(0);
	}

	if (namesamen(sptr, x_("leaf-cell-numbers"), l) == 0)
	{
		if ((leafcell = Sc_find_leaf_cell(pars[1])) == NULL)
		{
			return(Sc_seterrmsg(SC_CELLNOFIND, pars[1]));
		}
		Sc_leaf_cell_get_nums(leafcell, &cnums);
		ttyputmsg(x_("****************************************"));
		ttyputmsg(M_("**      LEAF CELL MAGIC NUMBERS       **"));
		ttyputmsg(x_("****************************************"));
		ttyputmsg(x_(" "));
		ttyputmsg(M_("Top active distance      =  %d"), cnums.top_active);
		ttyputmsg(M_("Bottom active distance   =  %d"), cnums.bottom_active);
		ttyputmsg(M_("Left active distance     =  %d"), cnums.left_active);
		ttyputmsg(M_("Right active distance    =  %d"), cnums.right_active);
		ttyputmsg(x_("----------------------------------------"));
		return(SC_NOERROR);
	}

	if (namesamen(sptr, x_("instances"), l) == 0)
	{
		if (--count == 0)
		{
			if (sc_curcell == NULL) return(Sc_seterrmsg(SC_NOCELL));
			cells = sc_curcell;
		} else
		{
			sptr = pars[1];

			/* try to find cell */
			for (cells = sc_cells; cells; cells = cells->next)
			{
				if (namesame(cells->name, sptr) == 0)
					break;
			}
			if (cells == NULL)
				return(Sc_seterrmsg(SC_CELLNOFIND, sptr));
		}

		/* check for all option */
		if (--count > 0)
		{
			all_flag = TRUE;
		} else
		{
			all_flag = FALSE;
		}
		ttyputmsg(M_("Instances for cell %s:"), cells->name);
		Sc_nitreelist(cells->niroot, all_flag);
		return(0);
	}

	if (namesamen(sptr, x_("nodes"), l) == 0)
	{
		if (sc_curcell == NULL)
			return(Sc_seterrmsg(SC_NOCELL));
		ttyputmsg(M_("List of extracted nodes for cell %s:"), sc_curcell->name);
		Sc_extract_print_nodes(sc_curcell);
		return(SC_NOERROR);
	}

	if (namesamen(sptr, x_("placement"), l) == 0)
	{
		if (--count == 0)
		{
			if (sc_curcell == NULL)
				return(Sc_seterrmsg(SC_NOCELL));
			cells = sc_curcell;
		} else
		{
			/* try to find cell */
			sptr = pars[1];
			for (cells = sc_cells; cells; cells = cells->next)
			{
				if (namesame(cells->name, sptr) == 0)
					break;
			}
			if (cells == NULL)
				return(Sc_seterrmsg(SC_CELLNOFIND, sptr));
		}
		if (cells->placement == NULL)
			return(Sc_seterrmsg(SC_CELL_NO_PLACE, cells->name));
		ttyputmsg(M_("*****  PLACEMENT for cell %s *****"), sptr);
		for (row = cells->placement->rows; row; row = row->next)
		{
			ttyputmsg(M_("For Row #%d, size %d:"), row->row_num, row->row_size);
			for (place = row->start; place; place = place->next)
				ttyputmsg(x_("    %8d    %-s"), place->xpos, place->cell->name);
		}
		return(SC_NOERROR);
	}

	if (namesamen(sptr, x_("routing"), l) == 0)
	{
		if (--count == 0)
		{
			if (sc_curcell == NULL)
				return(Sc_seterrmsg(SC_NOCELL));
			cells = sc_curcell;
		} else
		{
			/* try to find cell */
			sptr = pars[1];
			for (cells = sc_cells; cells; cells = cells->next)
			{
				if (namesame(cells->name, sptr) == 0)
					break;
			}
			if (cells == NULL)
				return(Sc_seterrmsg(SC_CELLNOFIND, sptr));
		}
		if (cells->route == NULL)
			return(Sc_seterrmsg(SC_CELL_NO_ROUTE, cells->name));
		ttyputmsg(x_("****************************************"));
		ttyputmsg(M_("**     CHANNEL ROUTING INFORMATION    **"));
		ttyputmsg(x_("****************************************"));
		ttyputmsg(x_(" "));
		Sc_route_print_channel(cells->route->channels);
		ttyputmsg(x_("----------------------------------------"));
		return(SC_NOERROR);
	}

	return(Sc_seterrmsg(SC_XSHOW, sptr));
}

/***********************************************************************
Module:  Sc_nitreelist
------------------------------------------------------------------------
Description:
	Print the node names for the current working cell by doing a preorder
	transversal of the ni tree.
------------------------------------------------------------------------
*/

void Sc_nitreelist(SCNITREE *ntp, int all_flag)
{
	CHAR	*type;

	if (ntp == NULL) return;
	if (ntp->lptr)
		Sc_nitreelist(ntp->lptr, all_flag);
	switch (ntp->type)
	{
		case SCSPECIALCELL:
			type = _("Special");
			break;
		case SCCOMPLEXCELL:
			type = ((SCCELL *)(ntp->np))->name;
			break;
		case SCLEAFCELL:
			type = Sc_leaf_cell_name(ntp->np);
			break;
		default:
			break;
	}
	if (all_flag || ntp->type != SCSPECIALCELL)
		ttyputmsg(x_("    %-20s    type = %s"), ntp->name, type);
	if (ntp->rptr)
		Sc_nitreelist(ntp->rptr, all_flag);
}

/***********************************************************************
Module:  Sc_select
------------------------------------------------------------------------
Description:
	Execute the SELECT command.
------------------------------------------------------------------------
*/

int Sc_select(int count, CHAR *pars[])
{
	SCCELL	*cell;

	if (count < 1)
		return(Sc_seterrmsg(SC_SELECT_CELL_NONAME));

	/* try to find cell */
	for (cell = sc_cells; cell; cell = cell->next)
	{
		if (namesame(cell->name, pars[0]) == 0)
			break;
	}
	if (cell == NULL)
		return(Sc_seterrmsg(SC_CELLNOFIND, pars[0]));
	sc_curcell = cell;

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_xset
------------------------------------------------------------------------
Description:
	Execute the SET command.  Current options are:
		1.  leaf-cell-numbers  = Magic numbers for leaf cells.
		2.  node-name	    = Name an extracted node.
		3.  port-direction  = Direction allowed to attach to a port.
------------------------------------------------------------------------
*/

int Sc_xset(int count, CHAR *pars[])
{
	SCCELL	*cell;
	SCCELLNUMS	cnums;
	SCPORT	*port;
	CHAR	*leafcell, *leafport;
	CHAR	*dir;
	int		*bits, numpar, err, l;
	SCNITREE	**instptr;
	SCNIPORT	*iport;

	if (!count)
		return(Sc_seterrmsg(SC_SETNCMD));

	l = maxi(estrlen(pars[0]), 1);
	if (namesamen(x_("leaf-cell-numbers"), pars[0], l) == 0)
	{
		if ((leafcell = Sc_find_leaf_cell(pars[1])) == NULL)
			return(Sc_seterrmsg(SC_NPNOTFIND, pars[1]));
		Sc_leaf_cell_get_nums(leafcell, &cnums);
		numpar = 2;
		while (numpar < count)
		{
			if (namesame(x_("top-active"), pars[numpar]) == 0)
			{
				numpar++;
				if (numpar < count)
					cnums.top_active = eatoi(pars[numpar++]);
				continue;
			}
			if (namesame(x_("bottom-active"), pars[numpar]) == 0)
			{
				numpar++;
				if (numpar < count)
					cnums.bottom_active = eatoi(pars[numpar++]);
				continue;
			}
			if (namesame(x_("left-active"), pars[numpar]) == 0)
			{
				numpar++;
				if (numpar < count)
					cnums.left_active = eatoi(pars[numpar++]);
				continue;
			}
			if (namesame(x_("right-active"), pars[numpar]) == 0)
			{
				numpar++;
				if (numpar < count)
					cnums.right_active = eatoi(pars[numpar++]);
				continue;
			}
			return(Sc_seterrmsg(SC_SET_CNUMS_XOPT, pars[numpar]));
		}
		if ((err = Sc_leaf_cell_set_nums(leafcell, &cnums)))
			return(err);
		return(SC_NOERROR);
	}

	if (namesamen(x_("node-name"), pars[0], l) == 0)
	{
		/* check for sufficient parameters */
		if (count < 4)
			return(Sc_seterrmsg(SC_SET_NAME_NO_PARS));

		/* search for instance */
		if (*(instptr = Sc_findni(&sc_curcell->niroot, pars[1])) == NULL)
			return(Sc_seterrmsg(SC_SET_NAME_INST_NO_FIND, pars[1]));

		/* search for port on instance */
		for (iport = (*instptr)->ports; iport; iport = iport->next)
		{
			if ((*instptr)->type == SCLEAFCELL)
			{
				if (namesame(Sc_leaf_port_name(iport->port), pars[2]) == 0)
					break;
			} else if ((*instptr)->type == SCCOMPLEXCELL)
			{
				if (namesame(((SCPORT *)(iport->port))->name, pars[2]) == 0)
					break;
			}
		}
		if (!iport)
			return(Sc_seterrmsg(SC_SET_NAME_PORT_NO_FIND, pars[2], pars[1]));

		/* set extracted node name if possible */
		if (iport->ext_node)
		{
			if (iport->ext_node->name)
				efree(iport->ext_node->name);
			if (allocstring(&iport->ext_node->name, pars[3], sc_tool->cluster))
				return(Sc_seterrmsg(SC_NOMEMORY));
			return(SC_NOERROR);
		} else
		{
			return(Sc_seterrmsg(SC_SET_NAME_NO_NODE));
		}
	}

	if (namesamen(x_("port-direction"), pars[0], l) == 0)
	{
		for (cell = sc_cells; cell; cell = cell->next)
		{
			if (namesame(cell->name, pars[1]) == 0)
				break;
		}
		if (cell == NULL)
		{
			if ((leafcell = Sc_find_leaf_cell(pars[1])) == NULL)
				return(Sc_seterrmsg(SC_NPNOTFIND, pars[1]));
			if ((leafport = Sc_find_leaf_port(leafcell, pars[2])) == NULL)
				return(Sc_seterrmsg(SC_PPNOTFIND, pars[2], pars[1]));
			bits = Sc_leaf_port_bits_address(leafport);
		} else
		{
			for (port = cell->ports; port; port = port->next)
			{
				if (namesame(port->name, pars[2]) == 0)
					break;
			}
			if (port == NULL)
				return(Sc_seterrmsg(SC_PPNOTFIND, pars[2], pars[1]));
			bits = &(port->bits);
		}
		*bits &= ~SCPORTDIRMASK;
		dir = pars[3];
		while (*dir)
		{
			switch (*dir)
			{
				case 'u':
					*bits |= SCPORTDIRUP;
					break;
				case 'd':
					*bits |= SCPORTDIRDOWN;
					break;
				case 'r':
					*bits |= SCPORTDIRRIGHT;
					break;
				case 'l':
					*bits |= SCPORTDIRLEFT;
					break;
				default:
					return(Sc_seterrmsg(SC_SETXPORTDIR, pars[3]));
			}
			dir++;
		}
	} else
	{
		return(Sc_seterrmsg(SC_SETXCMD, pars[0]));
	}
	return(0);
}

/***********************************************************************
Module:  Sc_library
------------------------------------------------------------------------
Description:
	Library command parser.  Currently supported commands include:
		1.  read
------------------------------------------------------------------------
*/

int Sc_library(int count, CHAR *pars[])
{
	int        l, err;
	CHAR       *sptr;

	l = estrlen((sptr = pars[0]));
	if (count == 0 || l == 0) return(Sc_seterrmsg(SC_NLIBRARY));
	if (namesamen(sptr, x_("use"), l) == 0)
	{
		l = estrlen(sptr = pars[1]);
		if (--count == 0 || l == 0) return(Sc_seterrmsg(SC_NLIBRARYUSE));
		err = Sc_library_use(sptr);
		return(err);
	}
	if (namesamen(sptr, x_("read"), l) == 0)
	{
		l = estrlen(sptr = pars[1]);
		if (--count == 0 || l == 0) return(Sc_seterrmsg(SC_NLIBRARYREAD));
		err = Sc_library_read(sptr);
		return(err);
	}
	return(Sc_seterrmsg(SC_XLIBRARY, sptr));
}

/***********************************************************************
Module:  Sc_order
------------------------------------------------------------------------
Description:
	Order the ports of the indicated leaf cell.  Used to tool
	users by ordering the ports in some reasonable fashion.
------------------------------------------------------------------------
*/

int Sc_order(int count, CHAR *pars[])
{
	CHAR	order[MAXLINE], *sptr, *filename;
	int		i, j, k, err_flag;
	CHAR	*leafcell, *port, *whichpp[100];
	FILE	*fp;

	if (!count)
		return(Sc_seterrmsg(SC_ORDERNPARS));

	/* search for leaf cell */
	if ((leafcell = Sc_find_leaf_cell(pars[0])) == NULL)
		return(Sc_seterrmsg(SC_NPNOTFIND, pars[0]));

	/* check if from file or by user */
	fp = (FILE *)NULL;
	if (count > 1)
	{
		if ((fp = xopen(pars[1], sc_filetypesctab, x_(""), &filename)) == NULL)
			return(Sc_seterrmsg(SC_ORDER_XOPEN_FILE, pars[1]));
	}

	/* get ordering, terminated by a <CR> for manual input */
	for (i = 0; i < 100; i++)
	{
		if (fp == NULL)
		{
			sptr = ttygetline(M_("Port>  "));
			if (sptr == 0 || estrlen(sptr) == 0) break;
			(void)estrcpy(order, sptr);
		} else
		{
			order[0] = 0;
			while (!xeof(fp))
			{
				(void)xfgets(order, MAXLINE, fp);

				/* find end of string */
				for (j = 0; j < MAXLINE; j++)
				{
					if (isspace(order[j]))
						order[j] = 0;
				}
				if (order[0] != 0)
					break;
			}
			if (order[0] == 0)
				break;
		}

		/* check that all ports exist on leaf cell */
		/* and that no port appears more than once */
		err_flag = FALSE;
		for (port = Sc_first_leaf_port(leafcell); port;
			port = Sc_next_leaf_port(port))
		{
			if (namesame(order, Sc_leaf_port_name(port)) == 0)
			{
				whichpp[i] = port;
				break;
			}
		}
		if (port == NULL)
		{
			ttyputmsg(_("Error - port %s not found"), order);
			err_flag = TRUE;
		} else
		{
			for (k = 0; k < i; k++)
			{
				if (port == whichpp[k])
				{
					ttyputmsg(_("Error - port %s appears more than once"),
						order);
					err_flag = TRUE;
				}
			}
		}
		if (err_flag)
		{
			if (fp) xclose(fp);
			return(0);
		}
	}

	if (i == 0)
	{
		ttyputmsg(_("Warning - no ports specified"));
		if (fp) xclose(fp);
		return(0);
	}

	/* add any ports which were not specified */
	for (port = Sc_first_leaf_port(leafcell); port;
		port = Sc_next_leaf_port(port))
	{
		for (j = 0; j < i; j++)
		{
			if (whichpp[j] == port)
				break;
		}
		if (j == i)
			whichpp[i++] = port;
	}

	/* order ports */
	for (j = 0; j < (i - 1); j++)
		Sc_leaf_port_set_next(whichpp[j], whichpp[j + 1]);
	Sc_leaf_port_set_next(whichpp[i - 1], (CHAR *)0);
	Sc_leaf_port_set_first(leafcell, whichpp[0]);

	if (fp) xclose(fp);
	return(0);
}

/***********************************************************************
Module:  Sc_pull
------------------------------------------------------------------------
Description:
	Execute the PULL command to flatten all complex cells in the current
	cell.  It does this by creating instances of all instances from the
	complex cells in the current cell.  To insure the uniqueness of all
	instance names, the new instances have names "parent_inst.inst"
	where "parent_inst" is the name of the instance being expanded and
	"inst" is the name of the subinstance being pulled up.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_pull(count, pars);

Name		Type		Description
----		----		-----------
count		int			Number of parameters.
pars		*char[]		Array of pointers to parameters.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_pull(int count, CHAR *pars[])
{
	int		info_flag, err;
	SCNITREE	*inst;

	info_flag = FALSE;

	/* check if any parameters passed */
	if (count)
	{
		if (namesame(pars[0], x_("information")) == 0)
			info_flag = TRUE;
	}

	/* check if a cell is currently selected */
	if (sc_curcell == NULL)
		return(Sc_seterrmsg(SC_NOCELL));

	/* expand all instances of complex cell type */
	for (inst = sc_curcell->nilist; inst; inst = inst->next)
	{
		if (inst->type == SCCOMPLEXCELL)
		{
			if ((err = Sc_pull_inst(inst, sc_curcell, info_flag)))
				return(err);
		}
	}
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_pull_inst
------------------------------------------------------------------------
Description:
	Pull the indicated instance of a complex cell into the indicated
	parent cell.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_pull_inst(inst, cell, info_flag);

Name		Type		Description
----		----		-----------
inst		*SCNITREE	Pointer to instance to be pulled up.
cell		*SCCELL		Pointer to parent cell.
info_flag	int			TRUE if process information is to
								be printed.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_pull_inst(SCNITREE *inst, SCCELL *cell, int info_flag)
{
	SCCELL	*subcell;
	SCNITREE	*subinst, *ninst;
	int		err, oldnum, newnum;
	CHAR	newname[MAXLINE], *pars[PARS], *sptr;
	SCEXTNODE	*enode, *bnode;
	SCNIPORT	*iport;
	SCEXTPORT	*eport, *nport;

	if (Sc_stop())
		longjmp(sc_please_stop, 1);

	subcell = (SCCELL *)inst->np;
	if (info_flag)
		ttyputmsg(_("Pulling instance %s of type %s into cell %s."),
		   inst->name, subcell->name, cell->name);

	/* first create components */
	for (subinst = subcell->nilist; subinst; subinst = subinst->next)
	{
		if (subinst->type != SCSPECIALCELL)
		{
			pars[0] = x_("instance");
			(void)esnprintf(newname, MAXLINE, x_("%s.%s"), inst->name, subinst->name);
			pars[1] = newname;
			if (subinst->type == SCLEAFCELL)
			{
				pars[2] = Sc_leaf_cell_name(subinst->np);
			} else
			{
				pars[2] = ((SCCELL *)(subinst->np))->name;
			}
			if ((err = Sc_create(3, pars)))
				return(err);
		}
	}

	/* remove instance from instance tree */
	Sc_remove_inst_from_itree(&cell->niroot, inst);

	/* update instance list */
	cell->nilist = NULL;
	Sc_make_nilist(cell->niroot, cell);

	/* create connections among these subinstances by using the */
	/* subcell's extracted node list.  Also resolve connections */
	/* to the parent cell instances by using exported port info. */
	for (enode = subcell->ex_nodes; enode; enode = enode->next)
	{
		bnode = NULL;

		/* check if the extracted node is an exported node */
		for (iport = inst->ports; iport; iport = iport->next)
		{
			if (((SCPORT *)(iport->port))->node->ports->ext_node == enode)
			{
				bnode = iport->ext_node;
				break;
			}
		}
		if (bnode == NULL)
		{
			/* this is a new internal node */
			bnode = (SCEXTNODE *)emalloc(sizeof(SCEXTNODE), sc_tool->cluster);
			if (bnode == NULL)
				return(Sc_seterrmsg(SC_NOMEMORY));
			bnode->name = NULL;
			bnode->firstport = NULL;
			bnode->ptr = NULL;
			bnode->flags = 0;
			bnode->next = cell->ex_nodes;
			cell->ex_nodes = bnode;
		}

		/* add ports to extracted node bnode */
		for (eport = enode->firstport; eport; eport = eport->next)
		{
			/* only add leaf cells or complex cells */
			if (eport->node->type != SCSPECIALCELL)
			{
				nport = (SCEXTPORT *)emalloc(sizeof(SCEXTPORT), sc_tool->cluster);
				if (nport == NULL)
					return(Sc_seterrmsg(SC_NOMEMORY));
				(void)esnprintf(newname, MAXLINE, x_("%s.%s"), inst->name, eport->node->name);
				nport->node = *(Sc_findni(&cell->niroot, newname));

				/* add reference to extracted node to instance port list */
				for (iport = nport->node->ports; iport; iport = iport->next)
				{
					if (iport->port == eport->port->port)
					{
						nport->port = iport;
						iport->ext_node = bnode;
						nport->next = bnode->firstport;
						bnode->firstport = nport;
						break;
					}
				}
			}
		}
	}

	/* add power ports for new instances */
	for (eport = subcell->power->firstport; eport; eport = eport->next)
	{
		if (eport->node->type == SCSPECIALCELL)
			continue;
		nport = (SCEXTPORT *)emalloc(sizeof(SCEXTPORT), sc_tool->cluster);
		if (nport  == NULL)
			return(Sc_seterrmsg(SC_NOMEMORY));
		(void)esnprintf(newname, MAXLINE, x_("%s.%s"), inst->name, eport->node->name);
		nport->node = *(Sc_findni(&cell->niroot, newname));

		/* add reference to extracted node to instance port list */
		for (iport = nport->node->ports; iport; iport = iport->next)
		{
			if (iport->port == eport->port->port)
			{
				nport->port = iport;
				iport->ext_node = cell->power;
				break;
			}
		}
		if (!iport)
		{
			for (iport = nport->node->power; iport; iport = iport->next)
			{
				if (iport->port == eport->port->port)
				{
					nport->port = iport;
					iport->ext_node = cell->power;
					break;
				}
			}
		}
		nport->next = cell->power->firstport;
		cell->power->firstport = nport;
	}

	/* remove references to original instance in power list */
	nport = cell->power->firstport;
	for (eport = cell->power->firstport; eport; eport = eport->next)
	{
		if (eport->node == inst)
		{
			if (eport == nport)
			{
				cell->power->firstport = eport->next;
				nport = eport->next;
			} else
			{
				nport->next = eport->next;
			}
		} else
		{
			nport = eport;
		}
	}

	/* add ground ports */
	for (eport = subcell->ground->firstport; eport; eport = eport->next)
	{
		if (eport->node->type == SCSPECIALCELL)
			continue;
		nport = (SCEXTPORT *)emalloc(sizeof(SCEXTPORT), sc_tool->cluster);
		if (nport == NULL)
			return(Sc_seterrmsg(SC_NOMEMORY));
		(void)esnprintf(newname, MAXLINE, x_("%s.%s"), inst->name, eport->node->name);
		nport->node = *(Sc_findni(&cell->niroot, newname));

		/* add reference to extracted node to instance port list */
		for (iport = nport->node->ports; iport; iport = iport->next)
		{
			if (iport->port == eport->port->port)
			{
				nport->port = iport;
				iport->ext_node = cell->ground;
				break;
			}
		}
		if (!iport)
		{
			for (iport = nport->node->ground; iport; iport = iport->next)
			{
				if (iport->port == eport->port->port)
				{
					nport->port = iport;
					iport->ext_node = cell->ground;
					break;
				}
			}
		}
		nport->next = cell->ground->firstport;
		cell->ground->firstport = nport;
	}

	/* remove references to original instance in ground list */
	nport = cell->ground->firstport;
	for (eport = cell->ground->firstport; eport; eport = eport->next)
	{
		if (eport->node == inst)
		{
			if (eport == nport)
			{
				cell->ground->firstport = eport->next;
				nport = eport->next;
			} else
			{
				nport->next = eport->next;
			}
		} else
		{
			nport = eport;
		}
	}

	/* remove references to instance in exported node list */
	for (enode = cell->ex_nodes; enode; enode = enode->next)
	{
		nport = enode->firstport;
		for (eport = enode->firstport; eport; eport = eport->next)
		{
			if (eport->node == inst)
			{
				if (eport == nport)
				{
					enode->firstport = eport->next;
					nport = eport->next;
				} else
				{
					nport->next = eport->next;
				}
			} else
			{
				nport = eport;
			}
		}
	}

	/* find the value of the largest generically named extracted node */
	oldnum = 0;
	for (enode = cell->ex_nodes; enode; enode = enode->next)
	{
		if (enode->name)
		{
			sptr = enode->name;
			if (Sc_Mytoupper(*sptr) == 'N')
			{
				sptr++;
				while (*sptr)
				{
					if (!isdigit(*sptr))
						break;
					sptr++;
				}
				if (*sptr == 0)
				{
					newnum = eatoi(enode->name + 1);
					if (newnum > oldnum)
						oldnum = newnum;
				}
			}
		}
	}

	/* set the name of any unnamed nodes */
	for (enode = cell->ex_nodes; enode; enode = enode->next)
	{
		if (enode->name == NULL)
		{
			(void)esnprintf(newname, MAXLINE, x_("n%d"), ++oldnum);
			(void)allocstring(&(enode->name), newname, sc_tool->cluster);
		}
	}

	/* flatten any subinstances which are also complex cells */
	for (subinst = subcell->nilist; subinst; subinst = subinst->next)
	{
		if (subinst->type == SCCOMPLEXCELL)
		{
			(void)esnprintf(newname, MAXLINE, x_("%s.%s"), inst->name, subinst->name);
			ninst = *(Sc_findni(&cell->niroot, newname));
			if ((err = Sc_pull_inst(ninst, cell, info_flag)))
				return(err);
		}
	}
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_cpu_time
------------------------------------------------------------------------
Description:
	Set or report cpu usage time.  The action taken depends on the
	passed parameter:

		o  TIME_RESET	- Reset the time to zero.
		o  TIME_REL	- Return a string of the number of seconds of
						  CPU time since last call.
		o  TIME_ABS	- Return a string of the number of seconds of
						  CPU time since reset.
------------------------------------------------------------------------
Calling Sequence:  seconds = Sc_cpu_time(action);

Name		Type		Description
----		----		-----------
action		int			Action to take.
seconds		*char		Pointer to string giving time in seconds.
------------------------------------------------------------------------
*/

CHAR  *Sc_cpu_time(int action)
{
	static CHAR		sbuf[200];
	static float	sofar;
	float thisone;

	switch (action)
	{
		case TIME_ABS:
			thisone = endtimer();
			sofar += thisone;
			(void)estrcpy(sbuf, explainduration(sofar));
			return(sbuf);
		case TIME_REL:
			thisone = endtimer();
			sofar += thisone;
			(void)estrcpy(sbuf, explainduration(thisone));
			return(sbuf);
		case TIME_RESET:
			starttimer();
			sofar = 0.0;
			return(x_("0.00"));
	}
	return(_("NO_TIME"));
}

/***********************************************************************
Module:  Sc_help
------------------------------------------------------------------------
Description:
	Help for the silicon compiler.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_help(count, pars);

Name		Type		Description
----		----		-----------
count		int			Number of parameters.
pars		*char[]		Array of pointers to parameters.
err			int			Returned error count, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_help(int count, CHAR *pars[])
{
	Q_UNUSED( count );
	Q_UNUSED( pars );

	(void)us_helpdlog(x_("QUISC"));
	return(SC_NOERROR);
}

#endif  /* SCTOOL - at top */
