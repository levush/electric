/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: data.c
 * global data definitions
 * Written by: Steven M. Rubin, Static Free Software
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

#include "global.h"

/******************************* GLOBAL *******************************/

CHAR       **el_namespace;				/* names in name space */
INTBIG       el_numnames;				/* number of names in name space */

/* some keys to commonly used variable names */
INTBIG       el_node_name_key;			/* key for "NODE_name" */
INTBIG       el_arc_name_key;			/* key for "ARC_name" */
INTBIG       el_arc_radius_key;			/* key for "ARC_radius" */
INTBIG       el_trace_key;				/* key for "trace" */
INTBIG       el_cell_message_key;		/* key for "FACET_message" */
INTBIG       el_schematic_page_size_key;/* key for "FACET_schematic_page_size" */
INTBIG       el_transistor_width_key;	/* key for "transistor_width" */
INTBIG       el_prototype_center_key;	/* key for "prototype_center" */
INTBIG       el_essential_bounds_key;	/* key for "FACET_essentialbounds" */
INTBIG       el_node_size_default_key;	/* key for "NODE_size_default" */
INTBIG       el_arc_width_default_key;	/* key for "ARC_width_default" */
INTBIG       el_attrkey_width;			/* key for "ATTR_width" */
INTBIG       el_attrkey_length;			/* key for "ATTR_length" */
INTBIG       el_attrkey_area;			/* key for "ATTR_area" */
INTBIG       el_attrkey_M;			    /* key for "ATTR_M" */
INTBIG       el_techstate_key;			/* key for "TECH_state" */

CLUSTER     *el_tempcluster;			/* cluster for temporary allocation */
CLUSTER     *db_cluster;				/* database general allocation */

INTBIG       el_maxtools;				/* current number of tools */
LIBRARY     *el_curlib;					/* pointer to current library (list head) */

VIEW        *el_views;					/* list of existing view */
VIEW        *el_unknownview;			/* the unknown view */
VIEW        *el_layoutview;				/* the layout view */
VIEW        *el_schematicview;			/* the schematic view */
VIEW        *el_iconview;				/* the icon view */
VIEW        *el_simsnapview;			/* the simulation-snapshot view */
VIEW        *el_skeletonview;			/* the skeleton view */
VIEW        *el_compview;				/* the compensated view */
VIEW        *el_vhdlview;				/* the VHDL view (text) */
VIEW        *el_verilogview;			/* the Verilog view (text) */
VIEW        *el_netlistview;			/* the netlist view, generic (text) */
VIEW        *el_netlistnetlispview;		/* the netlist view, netlisp (text) */
VIEW        *el_netlistalsview;			/* the netlist view, als (text) */
VIEW        *el_netlistquiscview;		/* the netlist view, quisc (text) */
VIEW        *el_netlistrsimview;		/* the netlist view, rsim (text) */
VIEW        *el_netlistsilosview;		/* the netlist view, silos (text) */
VIEW        *el_docview;				/* the documentation view (text) */

WINDOWPART  *el_topwindowpart;			/* top window in list */
WINDOWPART  *el_curwindowpart;			/* current window partition */

WINDOWFRAME *el_firstwindowframe = NOWINDOWFRAME;
WINDOWFRAME *el_curwindowframe;

TECHNOLOGY  *el_technologies;			/* defined in "tectable.c" */
TECHNOLOGY  *el_curtech;				/* pointer to current technology */
TECHNOLOGY  *el_curlayouttech;			/* last layout technology referenced */
INTBIG       el_maxtech;				/* current number of technologies */

CONSTRAINT  *el_curconstraint;			/* current constraint solver */

CHAR        *el_libdir;					/* pointer to library directory */

INTBIG       el_filetypetext;			/* Plain text disk file descriptor */

XARRAY       el_matid;					/* identity matrix */
INTBIG       el_pleasestop;				/* nonzero if abort is requested */
INTBIG       el_units;					/* display and internal units */

/******************************* GRAPHICS *******************************/

INTBIG       el_maplength;				/* number of entries in color map */
INTBIG       el_colcelltxt;				/* color to use for cell text and port names */
INTBIG       el_colcell;				/* color to use for cell outline */
INTBIG       el_colwinbor;				/* color to use for window border */
INTBIG       el_colhwinbor;				/* color to use for highlighted window border */
INTBIG       el_colmenbor;				/* color to use for menu border */
INTBIG       el_colhmenbor;				/* color to use for highlighted menu border */
INTBIG       el_colmentxt;				/* color to use for menu text */
INTBIG       el_colmengly;				/* color to use for menu glyphs */
INTBIG       el_colcursor;				/* color to use for cursor */
