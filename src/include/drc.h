/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: drc.h
 * Design-rule check tool
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

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
extern "C"
{
#endif

/* #define SURROUNDRULES 1 */		/* uncomment to add surround rule code (not ready yet) */

/* the meaning of "DRC_options" */
#define DRCFIRSTERROR              2		/* set to stop after first error in a cell */
#define DRCREASONABLE              4		/* set to only examine "reasonable" number of polygons per node */
#define DRCMULTIPROC             010		/* set to use multiple processors for DRC */
#define DRCNUMPROC          07777760		/* number of processors to use for DRC */
#define DRCNUMPROCSH               4		/* right-shift of DRCNUMPROC */

/* the bits in the return value of "dr_rulesdlog()" */
#define RULECHANGEMINWID          01		/* minimum width changed */
#define RULECHANGEMINWIDR         02		/* minimum width rule changed */
#define RULECHANGECONSPA          04		/* connected spacing changed */
#define RULECHANGECONSPAR        010		/* connected spacing rule changed */
#define RULECHANGEUCONSPA        020		/* unconnected spacing changed */
#define RULECHANGEUCONSPAR       040		/* unconnected spacing rule changed */
#define RULECHANGECONSPAW       0100		/* wide connected spacing changed */
#define RULECHANGECONSPAWR      0200		/* wide connected spacing rule changed */
#define RULECHANGEUCONSPAW      0400		/* wide unconnected spacing changed */
#define RULECHANGEUCONSPAWR    01000		/* wide unconnected spacing rule changed */
#define RULECHANGECONSPAM      02000		/* multicut connected spacing changed */
#define RULECHANGECONSPAMR     04000		/* multicut connected spacing rule changed */
#define RULECHANGEUCONSPAM    010000		/* multicut unconnected spacing changed */
#define RULECHANGEUCONSPAMR   020000		/* multicut unconnected spacing rule changed */
#define RULECHANGEEDGESPA     040000		/* edge spacing changed */
#define RULECHANGEEDGESPAR   0100000		/* edge spacing rule changed */
#define RULECHANGEWIDLIMIT   0200000		/* width limit changed */
#define RULECHANGEMINSIZE    0400000		/* minimum node size changed */
#define RULECHANGEMINSIZER  01000000		/* minimum node size rule changed */


#define NODRCRULES ((DRCRULES *)-1)

typedef struct
{
	CHAR    *techname;			/* name of the technology */
	INTBIG   numlayers;			/* number of layers in the technology */
	INTBIG   utsize;			/* size of upper-triangle of layers */
	INTBIG   widelimit;			/* width limit that triggers wide rules */
	CHAR   **layernames;		/* names of layers */
	INTBIG  *minwidth;			/* minimum width of layers */
	CHAR   **minwidthR;			/* minimum width rules */
	INTBIG  *conlist;			/* minimum distances when connected */
	CHAR   **conlistR;			/* minimum distance ruless when connected */
	INTBIG  *unconlist;			/* minimum distances when unconnected */
	CHAR   **unconlistR;		/* minimum distance rules when unconnected */
	INTBIG  *conlistW;			/* minimum distances when connected (wide) */
	CHAR   **conlistWR;			/* minimum distance rules when connected (wide) */
	INTBIG  *unconlistW;		/* minimum distances when unconnected (wide) */
	CHAR   **unconlistWR;		/* minimum distance rules when unconnected (wide) */
	INTBIG  *conlistM;			/* minimum distances when connected (multi-cut) */
	CHAR   **conlistMR;			/* minimum distance rules when connected (multi-cut) */
	INTBIG  *unconlistM;		/* minimum distances when unconnected (multi-cut) */
	CHAR   **unconlistMR;		/* minimum distance rules when unconnected (multi-cut) */
	INTBIG  *edgelist;			/* edge distances */
	CHAR   **edgelistR;			/* edge distance rules */
	INTBIG   numnodes;			/* number of nodes in the technology */
	CHAR   **nodenames;			/* names of nodes */
	INTBIG  *minnodesize;		/* minimim node size in the technology */
	CHAR   **minnodesizeR;		/* minimim node size rules */
} DRCRULES;

extern TOOL       *dr_tool;								/* the DRC tool object */
extern INTBIG      dr_max_distanceskey;					/* key for "DRC_max_distances" */
extern INTBIG      dr_wide_limitkey;					/* key for "DRC_wide_limit" */
extern INTBIG      dr_min_widthkey;						/* key for "DRC_min_width" */
extern INTBIG      dr_min_width_rulekey;				/* key for "DRC_min_width_rule" */
extern INTBIG      dr_min_node_sizekey;					/* key for "DRC_min_node_size" */
extern INTBIG      dr_min_node_size_rulekey;			/* key for "DRC_min_node_size_rule" */
extern INTBIG      dr_connected_distanceskey;			/* key for "DRC_min_connected_distances" */
extern INTBIG      dr_connected_distances_rulekey;		/* key for "DRC_min_connected_distances_rule" */
extern INTBIG      dr_unconnected_distanceskey;			/* key for "DRC_min_unconnected_distances" */
extern INTBIG      dr_unconnected_distances_rulekey;	/* key for "DRC_min_unconnected_distances_rule" */
extern INTBIG      dr_connected_distancesWkey;			/* key for "DRC_min_connected_distances_wide" */
extern INTBIG      dr_connected_distancesW_rulekey;		/* key for "DRC_min_connected_distances_wide_rule" */
extern INTBIG      dr_unconnected_distancesWkey;		/* key for "DRC_min_unconnected_distances_wide" */
extern INTBIG      dr_unconnected_distancesW_rulekey;	/* key for "DRC_min_unconnected_distances_wide_rule" */
extern INTBIG      dr_connected_distancesMkey;			/* key for "DRC_min_connected_distances_multi" */
extern INTBIG      dr_connected_distancesM_rulekey;		/* key for "DRC_min_connected_distances_multi_rule" */
extern INTBIG      dr_unconnected_distancesMkey;		/* key for "DRC_min_unconnected_distances_multi" */
extern INTBIG      dr_unconnected_distancesM_rulekey;	/* key for "DRC_min_unconnected_distances_multi_rule" */
extern INTBIG      dr_edge_distanceskey;				/* key for "DRC_min_edge_distances" */
extern INTBIG      dr_edge_distances_rulekey;			/* key for "DRC_min_edge_distances_rule" */
#ifdef SURROUNDRULES
extern INTBIG      dr_surround_layer_pairskey;			/* key for "DRC_surround_layer_pairs" */
extern INTBIG      dr_surround_distanceskey;			/* key for "DRC_surround_distances" */
extern INTBIG      dr_surround_rulekey;					/* key for "DRC_surround_rule" */
#endif
extern INTBIG      dr_ignore_listkey;					/* key for tool:drc.DRC_ignore_list */
extern INTBIG      dr_lastgooddrckey;					/* key for "DRC_last_good_drc" */
extern BOOLEAN     dr_logerrors;						/* TRUE to log errors in error reporting system */
extern TECHNOLOGY *dr_curtech;							/* technology whose valid layers are cached */
extern BOOLEAN    *dr_layersvalid;						/* list of valid layers in cached technology */

void      dr_init(INTBIG*, CHAR1*[], TOOL*);
void      dr_done(void);
void      dr_set(INTBIG, CHAR*[]);
void      dr_examinenodeproto(NODEPROTO*);
void      dr_slice(void);
INTBIG    dr_request(CHAR *command, va_list ap);
void      dr_modifynodeinst(NODEINST*,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG);
void      dr_modifyarcinst(ARCINST*,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG);
void      dr_newobject(INTBIG, INTBIG);
void      dr_killobject(INTBIG, INTBIG);
void      dr_newvariable(INTBIG, INTBIG, INTBIG, INTBIG);
void      dr_eraselibrary(LIBRARY*);

/* intertool prototypes */
void      dr_flatwrite(NODEPROTO*);
void      dr_flatignore(CHAR*);
void      dr_flatunignore(CHAR*);
INTBIG    dr_rulesdlog(TECHNOLOGY *tech, DRCRULES *rules);
DRCRULES *dr_allocaterules(INTBIG layercount, INTBIG nodecount, CHAR *techname);
void      dr_freerules(DRCRULES *rules);
INTBIG    dr_getoptionsvalue(void);
void      dr_reset_dates(void);
void      dr_cachevalidlayers(TECHNOLOGY *tech);
INTBIG    dr_adjustedmindist(TECHNOLOGY *tech, LIBRARY *lib, INTBIG layer1, INTBIG size1,
			INTBIG layer2, INTBIG size2, BOOLEAN con, BOOLEAN multi, INTBIG *edge, CHAR **rule);

INTBIG    drcb_check(NODEPROTO *cell, BOOLEAN report, BOOLEAN justarea);
void      drcb_initincrementalcheck(NODEPROTO *cell);
void      drcb_checkincremental(GEOM *geom, BOOLEAN partial);
void      drcb_term(void);

void      dr_quickcheck(NODEPROTO *cell, INTBIG count, NODEINST **nodestocheck, BOOLEAN *validity, BOOLEAN justarea);
void      dr_quickterm(void);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
