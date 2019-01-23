/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: sc1err.c
 * Contains error messages for the Silicon Compiler tool
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
#include "global.h"
#if SCTOOL

CHAR *sc_errmsg;

CHAR *sc_standard_errmsg[] =
{
	x_(""),																		/* SC_NOERROR */
	N_("Unknown command '%s'"),												/* SC_UNKNOWN */
	N_("No keyword for CREATE command"),									/* SC_NCREATE */
	N_("No name for CREATE CELL command"),									/* SC_NCREATECELL */
	N_("Cell '%s' already exists in current library"),						/* SC_CELLEXISTS */
	N_("Cannot create cell '%s'"),											/* SC_CELLNOMAKE */
	N_("Unknown CREATE command '%s'"),										/* SC_XCREATE */
	N_("No keyword for LIBRARY command"),									/* SC_NLIBRARY */
	N_("Unknown LIBRARY command '%s'"),										/* SC_XLIBRARY */
	N_("No name for LIBRARY USE command"),									/* SC_NLIBRARYUSE */
	N_("Library '%s' not selected"),										/* SC_NOLIBRARY */
	N_("No name for LIBRARY READ command"),									/* SC_NLIBRARYREAD */
	N_("Cannot create library '%s'"),										/* SC_LIBNOMAKE */
	N_("No memory"),														/* SC_NOMEMORY */
	N_("Cannot set library style"),											/* SC_LIBNSTYLE */
	N_("Cannot read library '%s'"),											/* SC_LIBNOREAD */
	N_("No instance name for CREATE INSTANCE command"),						/* SC_CRNODENONAME */
	N_("No type name for CREATE INSTANCE command"),							/* SC_CRNODENOPROTO */
	N_("Library '%s' not found"),											/* SC_CRNODENOLIB */
	N_("There is no '%s' in the standard cell library"),					/* SC_CRNODEPROTONF */
	N_("No cell selected"),													/* SC_NOCELL */
	N_("Cannot create instance '%s'"),										/* SC_CRNODENOMAKE */
	N_("Cannot set instance name '%s'"),									/* SC_NODENAMENOMAKE */
	N_("Instance '%s' already exists"),										/* SC_NIEXISTS */
	N_("No SHOW option selected"),											/* SC_NSHOW */
	N_("Invalid option '%s' for SHOW command"),								/* SC_XSHOW */
	N_("No cell name for SHOW PORTS command"),								/* SC_SHOWPNOCELL */
	N_("Cannot find cell '%s'"),											/* SC_CELLNOFIND */
	N_("Not enough parameters for CONNECT command"),						/* SC_NCONNECT */
	N_("Cannot find port '%s' on instance '%s'"),							/* SC_PORTNFIND */
	N_("Cannot find universal arc"),										/* SC_NOUARC */
	N_("Cannot make arc"),													/* SC_ARCNOMAKE */
	N_("No file name for COMPILE command"),									/* SC_COMPNFILE */
	N_("Cannot open file '%s' for COMPILE command"),						/* SC_COMPFILENOPEN */
	N_("Error creating connection list for '%s' to '%s'"),					/* SC_XCONLIST */
	N_("Error moving instance"),											/* SC_NPERR */
	N_("Error with edge routines"),											/* SC_EDGEERROR */
	N_("No option for SIMULATION command"),									/* SC_SIMNPARS */
	N_("Incorrect option '%s' for SIMULATION command"),						/* SC_SIMXCMD */
	N_("No cell name specified for SIMULATION SET command"),				/* SC_SIMSETNPARS */
	N_("Error setting simulation variable"),								/* SC_SIMXSETVAL */
	N_("No cell name specified for SIMULATION SHOW command"),				/* SC_SIMSHOWNPARS */
	N_("No simulation variable for cell '%s'"),								/* SC_SIMNOVAR */
	N_("Cannot create sc_nitree variable"),									/* SC_NIVARNOMAKE */
	N_("Not enough parameters for SIMULATION WRITE command"),				/* SC_SIMWRITENPARS */
	N_("Error opening output simulation file '%s'"),						/* SC_SIMWRITEFOPEN	 */
	N_("Cannot find sc_sim variable for cell '%s'"),						/* SC_SCVARNOFIND */
	N_("No option for SET command"),										/* SC_SETNCMD */
	N_("Unknown option '%s' for SET command"),								/* SC_SETXCMD */
	N_("Cannot find cell '%s'"),											/* SC_NPNOTFIND */
	N_("Cannot find port '%s' on cell '%s'"),								/* SC_PPNOTFIND */
	N_("Unknown port direction specifier '%s'"),							/* SC_SETXPORTDIR */
	N_("No instance specified for EXPORT command"),							/* SC_EXPORTNONODE */
	N_("Cannot find instance '%s' for EXPORT command"),						/* SC_EXPORTNODENOFIND */
	N_("No port specified for EXPORT command"),								/* SC_EXPORTNOPORT */
	N_("Cannot find port '%s' on instance '%s' for EXPORT command"),		/* SC_EXPORTPORTNOFIND */
	N_("Unknown port type '%s' for EXPORT command"),						/* SC_EXPORTXPORTTYPE */
	N_("No export name specified for EXPORT command"),						/* SC_EXPORTNONAME */
	N_("Export name '%s' is not unique"),									/* SC_EXPORTNAMENOTUNIQUE */
	N_("File error with SIM SET temp file"),								/* SC_SIMSETFERROR */
	N_("Error status from VHDL Compiler for file '%s'"),					/* SC_COMPVHDLERR */
	N_("Cannot find instance '%s'"),										/* SC_NINOFIND */
	N_("No leaf cell specified for ORDER command"),							/* SC_ORDERNPARS */
	N_("No options specified for the SELECT command"),						/* SC_SELECTNCMD */
	N_("No cell name specified for the SELECT CELL command"),				/* SC_SELECT_CELL_NONAME */
	N_("Invalid SELECT option '%s'"),										/* SC_SELECT_XCMD */
	N_("Error opening file '%s' for SET OUTPUT-DEVICE command"),			/* SC_SET_OUTPUT_FILE_XOPEN */
	N_("No connections found to allow PLACEing"),							/* SC_PLACE_NO_CONNECTIONS */
	N_("Unknown command option '%s' for PLACE command"),					/* SC_PLACE_XCMD */
	N_("No command for PLACE SET-CONTROL command"),							/* SC_PLACE_SET_NOCMD */
	N_("Unknown command option '%s' for PLACE SET-CONTROL command"),		/* SC_PLACE_SET_XCMD */
	N_("No PLACEMENT structure for cell '%s'"),								/* SC_CELL_NO_PLACE */
	N_("Unknown command option '%s' for ROUTE command"),					/* SC_ROUTE_XCMD */
	N_("Unknown command option '%s' for ROUTE SET-CONTROL command"),		/* SC_ROUTE_SET_XCMD */
	N_("No command specified for ROUTE SET-CONTROL command"),				/* SC_ROUTE_SET_NOCMD */
	N_("No ROUTE structure for cell '%s'"),									/* SC_CELL_NO_ROUTE */
	N_("Unknown command option '%s' for MAKE command"),						/* SC_MAKE_XCMD */
	N_("No command for MAKE SET-CONTROL command"),							/* SC_MAKE_SET_NOCMD */
	N_("Unknown command option '%s' for MAKE SET-CONTROL command"),			/* SC_MAKE_SET_XCMD */
	N_("Cannot create leaf cell '%s' in MAKER"),							/* SC_MAKER_NOCREATE_LEAF_CELL */
	N_("Cannot create leaf instance '%s' in MAKER"),						/* SC_MAKER_NOCREATE_LEAF_INST */
	N_("Cannot create leaf feed in MAKER"),									/* SC_MAKER_NOCREATE_LEAF_FEED */
	N_("Cannot create via in MAKER"),										/* SC_MAKER_NOCREATE_VIA */
	N_("Cannot create layer2 track in MAKER"),								/* SC_MAKER_NOCREATE_LAYER2 */
	N_("Cannot create layer1 track in MAKER"),								/* SC_MAKER_NOCREATE_LAYER1 */
	N_("Cannot create export port '%s' in MAKER"),							/* SC_MAKER_NOCREATE_XPORT */
	N_("Cannot set cell numbers structure"),								/* SC_NOSET_CELL_NUMS */
	N_("Unknown option '%s' for SET LEAF-CELL-NUMBERS command"),			/* SC_SET_CNUMS_XOPT */
	N_("Unable to get LAYER1-NODE for MAKER"),								/* SC_NO_LAYER1_NODE */
	N_("Unable to get LAYER2-NODE for MAKER"),								/* SC_NO_LAYER2_NODE */
	N_("Unable to get VIA for MAKER"),										/* SC_NO_VIA */
	N_("Unable to get LAYER1-ARC for MAKER"),								/* SC_NO_LAYER1_ARC */
	N_("Unable to get LAYER2-ARC for MAKER"),								/* SC_NO_LAYER2_ARC */
	N_("Unable to get LAYER P-WELL for MAKER"),								/* SC_NO_LAYER_PWELL */
	N_("Unable to create P-WELL in MAKER"),									/* SC_NOCREATE_PWELL */
	N_("No ALS program specified for system"),								/* SC_NO_ALS */
	N_("No VHDL Compiler program specified for system"),					/* SC_NO_VHDL_PROG */
	N_("Error openning file '%s' in ORDER command"),						/* SC_ORDER_XOPEN_FILE */
	N_("No cells to check in VERIFY command"),								/* SC_VERIFY_NO_CELLS */
	N_("Insufficent parameters for SET NODE-NAME command"),					/* SC_SET_NAME_NO_PARS */
	N_("Cannot find instance '%s' in SET NODE-NAME command"),				/* SC_SET_NAME_INST_NO_FIND */
	N_("Cannot find port '%s' on instance '%s' in SET NODE-NAME command"),	/* SC_SET_NAME_PORT_NO_FIND */
	N_("Cannot find extracted node to set name in SET NODE-NAME command"),	/* SC_SET_NAME_NO_NODE */
	0
};

#endif  /* SCTOOL - at top */
