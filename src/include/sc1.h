/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: sc1.h
 * Header file for the QUISC Silicon Compiler
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

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
extern "C"
{
#endif

/***********************************************************************
	General Constants
------------------------------------------------------------------------
*/
#define VERSION			x_("1.00")
#define DATE			x_("Feb 24, 1987")
#define PARS			10
#define MAXLINE         80
#define GND				0
#define PWR				1
#define SCSIMFILE       x_("simset.tmp")

/***********************************************************************
	Error Codes
------------------------------------------------------------------------
*/

#define SC_NOERROR					0
#define SC_UNKNOWN					1
#define SC_NCREATE					2
#define SC_NCREATECELL				3
#define SC_CELLEXISTS				4
#define SC_CELLNOMAKE				5
#define SC_XCREATE					6
#define SC_NLIBRARY					7
#define SC_XLIBRARY					8
#define SC_NLIBRARYUSE				9
#define SC_NOLIBRARY				10
#define SC_NLIBRARYREAD				11
#define SC_LIBNOMAKE				12
#define SC_NOMEMORY					13
#define SC_LIBNSTYLE				14
#define SC_LIBNOREAD				15
#define SC_CRNODENONAME				16
#define SC_CRNODENOPROTO			17
#define SC_CRNODENOLIB				18
#define SC_CRNODEPROTONF			19
#define SC_NOCELL					20
#define SC_CRNODENOMAKE				21
#define SC_NODENAMENOMAKE			22
#define SC_NIEXISTS					23
#define SC_NSHOW					24
#define SC_XSHOW					25
#define SC_SHOWPNOCELL				26
#define SC_CELLNOFIND				27
#define SC_NCONNECT					28
#define SC_PORTNFIND				29
#define SC_NOUARC					30
#define SC_ARCNOMAKE				31
#define SC_COMPNFILE				32
#define SC_COMPFILENOPEN			33
#define SC_XCONLIST					34
#define SC_NPERR					35
#define SC_EDGEERROR				36
#define SC_SIMNPARS					37
#define SC_SIMXCMD					38
#define SC_SIMSETNPARS				39
#define SC_SIMXSETVAL				40
#define SC_SIMSHOWNPARS				41
#define SC_SIMNOVAR					42
#define SC_NIVARNOMAKE				43
#define SC_SIMWRITENPARS			44
#define SC_SIMWRITEFOPEN			45
#define SC_SCVARNOFIND				46
#define SC_SETNCMD					47
#define SC_SETXCMD					48
#define SC_NPNOTFIND				49
#define SC_PPNOTFIND				50
#define SC_SETXPORTDIR				51
#define SC_EXPORTNONODE				52
#define SC_EXPORTNODENOFIND			53
#define SC_EXPORTNOPORT				54
#define SC_EXPORTPORTNOFIND			55
#define SC_EXPORTXPORTTYPE			56
#define SC_EXPORTNONAME				57
#define SC_EXPORTNAMENOTUNIQUE		58
#define SC_SIMSETFERROR				59
#define SC_COMPVHDLERR				60
#define SC_NINOFIND					61
#define SC_ORDERNPARS				62
#define SC_SELECTNCMD				63
#define SC_SELECT_CELL_NONAME		64
#define SC_SELECT_XCMD				65
#define SC_SET_OUTPUT_FILE_XOPEN	66
#define SC_PLACE_NO_CONNECTIONS		67
#define SC_PLACE_XCMD				68
#define SC_PLACE_SET_NOCMD			69
#define SC_PLACE_SET_XCMD			70
#define SC_CELL_NO_PLACE			71
#define SC_ROUTE_XCMD				72
#define SC_ROUTE_SET_XCMD			73
#define SC_ROUTE_SET_NOCMD			74
#define SC_CELL_NO_ROUTE			75
#define SC_MAKE_XCMD				76
#define SC_MAKE_SET_NOCMD			77
#define SC_MAKE_SET_XCMD			78
#define SC_MAKER_NOCREATE_LEAF_CELL	79
#define SC_MAKER_NOCREATE_LEAF_INST	80
#define SC_MAKER_NOCREATE_LEAF_FEED	81
#define SC_MAKER_NOCREATE_VIA		82
#define SC_MAKER_NOCREATE_LAYER2	83
#define SC_MAKER_NOCREATE_LAYER1	84
#define SC_MAKER_NOCREATE_XPORT		85
#define SC_NOSET_CELL_NUMS			86
#define SC_SET_CNUMS_XOPT			87
#define SC_NO_LAYER1_NODE			88
#define SC_NO_LAYER2_NODE			89
#define SC_NO_VIA					90
#define SC_NO_LAYER1_ARC			91
#define SC_NO_LAYER2_ARC			92
#define SC_NO_LAYER_PWELL			93
#define SC_NOCREATE_PWELL			94
#define SC_NO_ALS					95
#define SC_NO_VHDL_PROG				96
#define SC_ORDER_XOPEN_FILE			97
#define SC_VERIFY_NO_CELLS			98
#define SC_SET_NAME_NO_PARS			99
#define SC_SET_NAME_INST_NO_FIND	100
#define SC_SET_NAME_PORT_NO_FIND	101
#define SC_SET_NAME_NO_NODE			102

/***********************************************************************
    Savable Parameters
------------------------------------------------------------------------
*/
#define SC_PARAM_MAKE_HORIZ_ARC          1
#define SC_PARAM_MAKE_VERT_ARC           2
#define SC_PARAM_MAKE_L1_WIDTH           3
#define SC_PARAM_MAKE_L2_WIDTH           4
#define SC_PARAM_MAKE_PWR_WIDTH          5
#define SC_PARAM_MAKE_MAIN_PWR_WIDTH     6
#define SC_PARAM_MAKE_MAIN_PWR_RAIL      7
#define SC_PARAM_MAKE_PWELL_SIZE         8
#define SC_PARAM_MAKE_PWELL_OFFSET       9
#define SC_PARAM_MAKE_NWELL_SIZE        10
#define SC_PARAM_MAKE_NWELL_OFFSET      11
#define SC_PARAM_MAKE_VIA_SIZE          12
#define SC_PARAM_MAKE_MIN_SPACING       13
#define SC_PARAM_ROUTE_FEEDTHRU_SIZE    14
#define SC_PARAM_ROUTE_PORT_X_MIN_DIST  15
#define SC_PARAM_ROUTE_ACTIVE_DIST      16
#define SC_PARAM_PLACE_NUM_ROWS         17

/* default values for maker */
#define DEFAULT_MIN_SPACING			2400		/* minimum metal spacing */
#define DEFAULT_VIA_SIZE			1600		/* VIA size */
#define DEFAULT_ARC_HORIZONTAL		NOARCPROTO	/* arc name on layer 1 */
#define DEFAULT_ARC_VERTICAL		NOARCPROTO	/* arc name on layer 2 */
#define DEFAULT_L2_TRACK_WIDTH		1600		/* layer 2 track width */
#define DEFAULT_L1_TRACK_WIDTH		1600		/* layer 1 track width */
#define DEFAULT_POWER_TRACK_WIDTH	2000		/* power track width */
#define DEFAULT_MAIN_POWER_WIDTH	3200		/* main power buses width */
#define DEFAULT_MAIN_POWER_RAIL     0			/* main power on horizontal */
#define DEFAULT_PWELL_SIZE			0			/* P-well size */
#define DEFAULT_PWELL_OFFSET		0			/* P-well offset from bottom */
#define DEFAULT_NWELL_SIZE			0			/* N-well size */
#define DEFAULT_NWELL_OFFSET		0			/* N-well offset from bottom */

/* default values for router */
#define DEFAULT_FEED_THROUGH_SIZE	6400	/* feed through size */
#define DEFAULT_PORT_X_MIN_DISTANCE	3200	/* min distance between ports*/
#define DEFAULT_ACTIVE_DISTANCE		3200	/* minimum distance to active*/

/* default values for placer */
#define DEFAULT_NUM_OF_ROWS			4	/* default number of rows */

/***********************************************************************
	QUISC Cell Structure
------------------------------------------------------------------------
*/

typedef struct Isccell
{
	CHAR              *name;			/* name of complex cell */
	int                max_node_num;	/* maximum number of nodes */
	struct Iscnitree  *niroot;			/* root to instance tree for cell */
	struct Iscnitree  *nilist;			/* list of instances for cell */
	struct Iscsim     *siminfo;			/* simulation information */
	struct Iscextnode *ex_nodes;		/* extracted nodes */
	int                bits;			/* flags for processing cell */
	struct Iscextnode *power;			/* list of power ports */
	struct Iscextnode *ground;			/* list of ground ports */
	struct Iscport    *ports, *lastport; /* list of ports */
	struct Iscplace   *placement;		/* placement information of cell */
	struct Iscroute   *route;			/* routing information for cell */
	struct Isccell    *next;			/* list of SC cells */
} SCCELL;

typedef struct Iscport
{
	CHAR             *name;				/* name of port */
	struct Iscnitree *node;				/* special node */
	struct Isccell   *parent;			/* complex cell on which */
										/* this port resides */
	int               bits;				/* port attributes */
	struct Iscport   *next;				/* pointer to next port */
} SCPORT;

typedef struct Isccellnums
{
	int			top_active;			/* active area from top */
	int			bottom_active;		/* active are from bottom */
	int			left_active;		/* active area from left */
	int			right_active;		/* active are from right */
} SCCELLNUMS;

/***********************************************************************
	Instance Tree Structure
------------------------------------------------------------------------
*/

/***** Types of Instances *****/
#define SCLEAFCELL     0
#define SCCOMPLEXCELL  1
#define SCSPECIALCELL  2
#define SCFEEDCELL     3
#define SCSTITCH       4
#define SCLATERALFEED  5

typedef struct Iscnitree
{
	CHAR				*name;		/* pointer to string of instance name */
	int					number;		/* alternative number of node */
	int					type;		/* type of instance */
	CHAR				*np;		/* pointer to leaf cell */
									/* or SCCELL if complex */
	int					size;		/* x size if leaf cell */
	struct Iscconlist	*connect;	/* pointer to connection list */
	struct Iscniport	*ports;		/* list of io ports and ext nodes */
	struct Iscniport	*power;		/* list of actual power ports */
	struct Iscniport	*ground;	/* list of actual ground ports */
	int					flags;		/* bits for silicon compiler */
	CHAR				*tp;		/* generic temporary pointer */
	struct Iscnitree	*next;		/* pointer to next instance in list */
	struct Iscnitree	*lptr;		/* left pointer for tree structure */
	struct Iscnitree	*rptr;		/* right pointer for tree structure */
} SCNITREE;

typedef struct Iscniport
{
	CHAR				*port;		/* leaf port or */
									/* SCPORT if on complex cell */
	struct Iscextnode	*ext_node;	/* extracted node */
	int					bits;		/* bits for processing */
	int					xpos;		/* x position if leaf port */
	struct Iscniport	*next;		/* list of instance ports */
} SCNIPORT;
#define SCNIPORTSEEN	0x00000001

/***********************************************************************
	Connection Structures
------------------------------------------------------------------------
*/

typedef struct Iscconlist
{
	struct Iscniport	*portA;		/* pointer to port on node A */
	struct Iscnitree	*nodeB;		/* pointer to node B */
	struct Iscniport	*portB;		/* pointer to port on node B */
	struct Iscextnode	*ext_node;	/* pointer to extracted node */
	struct Iscconlist   *next;		/* pointer to next list element */
} SCCONLIST;

/***********************************************************************
	Extraction Structures
------------------------------------------------------------------------
*/

typedef struct Iscextport
{
	struct Iscnitree	*node;		/* instance of extracted node */
	struct Iscniport	*port;		/* instance port */
	struct Iscextport	*next;		/* next in list of common node */
} SCEXTPORT;

#define SCEXTNODECLUSE	0x0003
#define SCEXTNODEGROUP1	0x0001
#define SCEXTNODEGROUP2	0x0002

typedef struct Iscextnode
{
	CHAR				*name;		/* optional name of port */
	struct Iscextport	*firstport;	/* link list of ports */
	int					flags;		/* flags for processing */
	CHAR				*ptr;		/* generic pointer for processing */
	struct Iscextnode	*next;		/* link list of nodes */
} SCEXTNODE;

/***********************************************************************
	Simulation Structures
------------------------------------------------------------------------
*/

#define SCSIMWRITEBITS	0x00000003	/* flag bits for sim write */
#define SCSIMWRITENEED	0x00000001	/* needs to be written */
#define SCSIMWRITESEEN	0x00000002	/* has been written */

typedef struct Iscsim
{
	CHAR		*model;
	struct Iscsim	*next;
} SCSIM;

/***********************************************************************
	Placement Structures and Constants
------------------------------------------------------------------------
*/

/***** general placement information *****/
typedef struct Iscplace
{
	int						num_inst;	/* number of instances */
	int						size_inst;	/* total size of instances */
	int						avg_size;	/* average size of inst */
	int						avg_height;	/* average height of inst */
	int						num_rows;	/* number of rows */
	int						size_rows;	/* target size of each row */
	struct Iscrowlist		*rows;		/* rows of placed cells */
	struct Iscnbplace		*plist;		/* start of cell list */
	struct Iscnbplace		*endlist;	/* end of cell list */
} SCPLACE;

typedef struct Iscplacecontrol
{
	int		stats_flag;			/* TRUE = print statistics */
	int		sort_flag;			/* TRUE = sort cluster tree */
	int		net_balance_flag;	/* TRUE = do net balance */
	int		net_balance_limit;	/* limit of movement */
	int		vertical_cost;		/* scaling factor */
} SCPLACECONTROL;

#define SCBITS_PLACEMASK	0x01
#define SCBITS_PLACED		0x01
#define SCBITS_EXTRACT		0x02

typedef struct Isccluster
{
	struct Iscnitree		*node;	  /* instance of cluster */
	int						number;   /* number of cluster */
	int						size;	  /* total size of members */
	struct Isccluster		*last;	  /* pointer to last cluster */
	struct Isccluster		*next;	  /* pointer to next cluster */
} SCCLUSTER;

typedef struct Iscclustertree
{
	struct Isccluster		*cluster; /* pointer to cluster */
										  /* NOSCCLUSTER if intermediate node*/
	int						bits;	  /* working bits */
	struct Iscclustertree	*parent;  /* parent node */
	struct Iscclustertree	*next;	  /* pointer to nodes on same level */
	struct Iscclustertree	*lptr;	  /* pointer to one group */
	struct Iscclustertree	*rptr;	  /* pointer to second group */
} SCCLUSTERTREE;

typedef struct Iscclconnect
{
	struct Iscclustertree	*node[2]; /* pointers to names of nodes */
	int						count;    /* number of connections */
	struct Iscclconnect		*next;    /* pointer to next list element */
	struct Iscclconnect		*last;    /* pointer to previous list element*/
} SCCLCONNECT;

typedef struct Iscrowlist
{
	struct Iscnbplace	*start;		/* start of row cells */
	struct Iscnbplace	*end;		/* end of row cells */
	int					row_num;	/* row number (0 = bottom) */
	int					row_size;	/* current row size */
	struct Iscrowlist	*next;		/* next in row list */
	struct Iscrowlist	*last;		/* last in row list */
} SCROWLIST;

#define NOSCNBPLACE	((SCNBPLACE *)-1)
typedef struct Iscnbplace
{
	struct Iscnitree	*cell;		/* pointer to cell */
	int					xpos;		/* x position (0 at left) */
	struct Iscnbplace	*last;		/* pointer to last in list */
	struct Iscnbplace	*next;		/* pointer to right in list */
} SCNBPLACE;

typedef struct Iscchannel
{
	int					number;		/* number of channel */
	struct Iscnbtrunk	*trunks;	/* list of trunks */
	struct Iscchannel	*last;		/* last in list of channels */
	struct Iscchannel	*next;		/* next in list of channels */
} SCCHANNEL;

typedef struct Iscnbtrunk
{
	struct Iscextnode	*ext_node;	/* pointer to extracted node */
	int					minx;		/* minimum trunk going left */
	int					maxx;		/* maximum trunk going right */
	struct Iscnbtrunk	*same;		/* same in next channel */
	struct Iscnbtrunk	*next;		/* pointer to next trunk */
} SCNBTRUNK;

#define SC_PLACE_SORT_ALL_TREES	0x0000000F
#define SC_PLACE_SORT_TREE_0	0x00000001
#define SC_PLACE_SORT_TREE_1	0x00000002
#define SC_PLACE_SORT_TREE_2	0x00000004
#define SC_PLACE_SORT_TREE_3	0x00000008
#define SC_PLACE_SORT_MASK_1	0x0000000D
#define SC_PLACE_SORT_MASK_2	0x0000000B
#define SC_PLACE_SORT_CASE_1	0x00000005
#define SC_PLACE_SORT_CASE_2	0x0000000A


/***********************************************************************
	Routing structures and constants
------------------------------------------------------------------------
*/

/***** Directions that ports can be attached to *****/
#define SCPORTDIRMASK	0x0000000F	/* mask for port direction */
#define SCPORTDIRUP		0x00000001	/* port direction up */
#define SCPORTDIRDOWN	0x00000002	/* port direction down */
#define SCPORTDIRRIGHT	0x00000004	/* port direction right */
#define SCPORTDIRLEFT	0x00000008	/* port direction left */
#define SCPORTTYPE		0x000003F0	/* port type mask */
#define SCGNDPORT		0x00000010	/* ground port */
#define SCPWRPORT		0x00000020	/* power port */
#define SCBIDIRPORT		0x00000040	/* bidirectional port */
#define SCOUTPORT		0x00000080	/* output port */
#define SCINPORT		0x00000100	/* input port */
#define SCUNPORT		0x00000200	/* unknown port */

#define SCROUTEMASK		0x00000007	/* mask for all bits */
#define SCROUTESEEN		0x00000001	/* seen in processing */
#define SCROUTEUNUSABLE	0x00000002	/* unusable in current track */
#define SCROUTETEMPNUSE	0x00000004	/* temporary not use */

typedef struct Iscroute
{
	struct Iscroutechannel	*channels;	/* list of channels */
	struct Iscrouteexport	*exports;	/* exported ports */
	struct Iscrouterow		*rows;		/* route rows */
} SCROUTE;

typedef struct Iscroutecontrol
{
	int			verbose;				/* verbose flag */
	int			fuzzy_window_limit;		/* for pass through window */
} SCROUTECONTROL;

typedef struct Iscrouterow
{
	int						number;		/* number, 0 = bottom */
	struct Iscroutenode		*nodes;		/* list of extracted nodes */
	struct Iscrowlist		*row;		/* reference actual row */
	struct Iscrouterow		*last;		/* last in row list */
	struct Iscrouterow		*next;		/* next in row list */
} SCROUTEROW;

typedef struct Iscroutenode
{
	struct Iscextnode		*ext_node;	/* extracted node */
	struct Iscrouterow		*row;		/* reference row */
	struct Iscrouteport		*firstport;	/* first port in row */
	struct Iscrouteport		*lastport;	/* last port in row */
	struct Iscroutenode		*same_next;	/* same nodes in above rows */
	struct Iscroutenode		*same_last;	/* same nodes in below rows */
	struct Iscroutenode		*next;		/* nodes in same row */
} SCROUTENODE;

typedef struct Iscrouteport
{
	struct Iscnbplace		*place;		/* reference place */
	struct Iscniport		*port;		/* particular port */
	struct Iscroutenode		*node;		/* reference node */
	int						flags;		/* flags for processing */
	struct Iscrouteport		*last;		/* previous port in list */
	struct Iscrouteport		*next;		/* next port in list */
} SCROUTEPORT;

typedef struct Iscroutechannel
{
	int						number;		/* number, 0 is bottom */
	struct Iscroutechnode	*nodes;		/* list of nodes */
	struct Iscroutetrack	*tracks;	/* list of tracks */
	struct Iscroutechannel	*last;		/* last in channel list */
	struct Iscroutechannel	*next;		/* next in channel list */
} SCROUTECHANNEL;

typedef struct Iscroutechnode
{
	struct Iscextnode		*ext_node;	/* extracted node */
	int						number;		/* optional net number */
	struct Iscroutechport	*firstport;	/* first port in row */
	struct Iscroutechport	*lastport;	/* last port in row */
	struct Iscroutechannel	*channel;	/* reference channel */
	int						flags;		/* flags for processing */
	struct Iscroutechnode	*same_next;	/* same nodes in above rows */
	struct Iscroutechnode	*same_last;	/* same nodes in below rows */
	struct Iscroutechnode	*next;		/* nodes in same row */
} SCROUTECHNODE;

typedef struct Iscroutechport
{
	struct Iscrouteport		*port;		/* reference port */
	struct Iscroutechnode	*node;		/* reference channel node */
	int						xpos;		/* x position */
	int						flags;		/* flags for processing */
	struct Iscroutechport	*last;		/* previous port in list */
	struct Iscroutechport	*next;		/* next port in list */
} SCROUTECHPORT;

typedef struct Iscroutevcg
{
	struct Iscroutechnode	*chnode;	/* channel node */
	int						flags;		/* flags for processing */
	struct Iscroutevcgedge	*edges;		/* edges of graph */
} SCROUTEVCG;

typedef struct Iscroutevcgedge
{
	struct Iscroutevcg		*node;		/* to which node */
	struct Iscroutevcgedge	*next;		/* next in list */
} SCROUTEVCGEDGE;

typedef struct Iscroutezrg
{
	int						number;		/* number of zone */
	struct Iscroutezrgmem	*chnodes;	/* list of channel nodes */
	struct Iscroutezrg		*last;		/* last zone */
	struct Iscroutezrg		*next;		/* next zone */
} SCROUTEZRG;

typedef struct Iscroutezrgmem
{
	struct Iscroutechnode	*chnode;	/* channel node */
	struct Iscroutezrgmem	*next;		/* next in zone */
} SCROUTEZRGMEM;

typedef struct Iscroutetrack
{
	int						number;		/* number of track, 0 = top */
	struct Iscroutetrackmem	*nodes;		/* track member */
	struct Iscroutetrack	*last;		/* last track in list */
	struct Iscroutetrack	*next;		/* next track in list */
} SCROUTETRACK;

typedef struct Iscroutetrackmem
{
	struct Iscroutechnode	*node;		/* channel node */
	struct Iscroutetrackmem	*next;		/* next in same track */
} SCROUTETRACKMEM;

typedef struct Iscrouteexport
{
	struct Iscport			*xport;		/* export port */
	struct Iscroutechport	*chport;	/* channel port */
	struct Iscrouteexport	*next;		/* next export port */
} SCROUTEEXPORT;

/***********************************************************************
	MAKER Structures
------------------------------------------------------------------------
*/

typedef struct Iscmakerinfo
{
	int			x_size;				/* size in X */
	int			y_size;				/* size in Y */
	int			area;				/* total area */
	int			min_x;				/* minimum X coordinate */
	int			max_x;				/* maximum X coordinate */
	int			min_y;				/* minimum Y coordinate */
	int			max_y;				/* maximum Y coordinate */
	int			num_leaf_cells;		/* number of leaf cells */
	int			num_feeds;			/* number of feed throughs */
	int			num_rows;			/* number of rows of cells */
	int			num_channels;		/* number of routing channels */
	int			num_tracks;			/* number of routing tracks */
	int			track_length;		/* total track length */
} SCMAKERINFO;

typedef struct Iscmakerdata
{
	struct Isccell			*cell;		/* cell being layed out */
	struct Iscmakerrow		*rows;		/* list of rows */
	struct Iscmakerchannel	*channels;	/* list of channels */
	struct Iscmakerpower	*power;		/* list of vdd ports */
	struct Iscmakerpower	*ground;	/* list of ground ports */
	int						minx;		/* minimum x position */
	int						maxx;		/* maximum x position */
	int						miny;		/* minimum y position */
	int						maxy;		/* maximum y position */
} SCMAKERDATA;

typedef struct Iscmakerrow
{
	int						number;		/* row number */
	struct Iscmakerinst		*members;	/* instances in rows */
	int						minx;		/* minimum X position */
	int						maxx;		/* maximum X position */
	int						miny;		/* minimum Y position */
	int						maxy;		/* maximum Y position */
	int						flags;		/* processing bits */
	struct Iscmakerrow		*last;		/* last row */
	struct Iscmakerrow		*next;		/* next row */
} SCMAKERROW;

typedef struct Iscmakerinst
{
	struct Iscnbplace		*place;		/* reference place */
	struct Iscmakerrow		*row;		/* reference row */
	int						xpos;		/* X position */
	int						ypos;		/* Y position */
	int						xsize;		/* size in X */
	int						ysize;		/* size in Y */
	int						flags;		/* processing flags */
	CHAR					*instance;	/* leaf instance */
	struct Iscmakerinst		*next;		/* next in row */
} SCMAKERINST;

typedef struct Iscmakerchannel
{
	int						number;		/* number of channel */
	struct Iscmakertrack	*tracks;	/* list of tracks */
	int						num_tracks;	/* number of tracks */
	int						miny;		/* minimum Y position */
	int						ysize;		/* Y size */
	int						flags;		/* processing bits */
	struct Iscmakerchannel	*last;		/* last channel */
	struct Iscmakerchannel	*next;		/* next channel */
} SCMAKERCHANNEL;

typedef struct Iscmakertrack
{
	int						number;		/* track number */
	struct Iscmakernode		*nodes;		/* nodes in track */
	struct Iscroutetrack	*track;		/* reference track */
	int						ypos;		/* Y position */
	int						flags;		/* processing bits */
	struct Iscmakertrack	*last;		/* previous track */
	struct Iscmakertrack	*next;		/* next track */
} SCMAKERTRACK;

typedef struct Iscmakernode
{
	struct Iscmakervia		*vias;		/* list of vias */
	struct Iscmakernode		*next;		/* next node in track */
} SCMAKERNODE;

#define SCVIASPECIAL	0x00000001
#define SCVIAEXPORT		0x00000002
#define SCVIAPOWER		0x00000004
typedef struct Iscmakervia
{
	int						xpos;		/* X position */
	struct Iscroutechport	*chport;	/* associated channel port */
	CHAR					*instance;	/* associated leaf instance */
	int						flags;		/* flags for processing */
	struct Iscrouteexport	*xport;		/* export port */
	struct Iscmakervia		*next;		/* next via */
} SCMAKERVIA;

typedef struct Iscmakerpower
{
	struct Iscmakerpowerport *ports;	/* list of power ports */
	int						ypos;		/* vertical position of row */
	struct Iscmakerpower	*next;		/* next in row list */
	struct Iscmakerpower	*last;		/* last in row list */
} SCMAKERPOWER;

typedef struct Iscmakerpowerport
{
	struct Iscmakerinst		*inst;		/* instance */
	struct Iscniport		*port;		/* port on instance */
	int						xpos;		/* resultant x position */
	struct Iscmakerpowerport *next;		/* next in list */
	struct Iscmakerpowerport *last;		/* last in list */
} SCMAKERPOWERPORT;

/***********************************************************************
	CPU Time Usage Constants and Structures
------------------------------------------------------------------------
*/

#define TIME_RESET	0
#define TIME_REL	1
#define TIME_ABS	2

typedef struct tbuffer
{
	int		proc_user_time;
	int		proc_system_time;
	int		child_user_time;
	int		child_system_time;
} TBUFFER;

/***********************************************************************
	Simulator information
-----------------------------------------------------------------------
*/
#define SC_ALS_FORMAT 1
#define SC_SILOS_FORMAT 2

extern TOOL       *sc_tool;				/* the Silicon Compiler tool object */
extern INTBIG      sc_filetypescsim;	/* Silicon compiler simulation file descriptor */
extern INTBIG      sc_filetypesctab;	/* Silicon compiler table file descriptor */

/* prototypes for tool interface */
void sc_init(INTBIG*, CHAR1*[], TOOL*);
void sc_done(void);
void sc_set(INTBIG, CHAR*[]);
INTBIG sc_request(CHAR*, va_list);
void sc_slice(void);

/* prototypes for intratool interface */
void Sc_clear_stop(void);
int Sc_stop(void);
int Sc_connect(int, CHAR*[]);
int Sc_create(int, CHAR*[]);
int Sc_delete(void);
int Sc_export(int, CHAR*[]);
int Sc_extract(int, CHAR*[]);
int Sc_maker(int, CHAR*[]);
int Sc_place(int, CHAR*[]);
int Sc_route(int, CHAR*[]);
void Sc_schematic(void);
int Sc_simulation(int, CHAR*[]);
int Sc_verify(void);
CHAR *Sc_find_leaf_cell(CHAR*);
CHAR *Sc_first_leaf_port(CHAR*);
CHAR *Sc_next_leaf_port(CHAR*);
int Sc_leaf_port_type(CHAR*);
int Sc_leaf_port_bits(CHAR*);
CHAR *Sc_leaf_port_name(CHAR*);
void Sc_leaf_cell_get_nums(CHAR*, SCCELLNUMS*);
void Sc_extract_print_nodes(SCCELL*);
void Sc_route_print_channel(SCROUTECHANNEL*);
CHAR *Sc_leaf_cell_name(CHAR*);
int Sc_leaf_cell_set_nums(CHAR*, SCCELLNUMS*);
SCNITREE **Sc_findni(SCNITREE**, CHAR*);
CHAR *Sc_find_leaf_port(CHAR*, CHAR*);
int *Sc_leaf_port_bits_address(CHAR*);
int Sc_library_read(CHAR*);
int Sc_library_use(CHAR*);
void Sc_leaf_port_set_next(CHAR*, CHAR*);
void Sc_leaf_port_set_first(CHAR*, CHAR*);
void Sc_remove_inst_from_itree(SCNITREE**, SCNITREE*);
void Sc_make_nilist(SCNITREE*, SCCELL*);
int Sc_seterrmsg(int, ...);
int Sc_leaf_cell_xsize(CHAR*);
int Sc_leaf_cell_ysize(CHAR*);
int Sc_leaf_port_xpos(CHAR*);
int Sc_leaf_port_ypos(CHAR*);
void Sc_initialize(void);
void Sc_main(void);
void Sc_one_command(int, CHAR*[]);
SCNIPORT *Sc_findpp(SCNITREE*, CHAR*);
SCNITREE *Sc_new_instance(CHAR*, int);
int Sc_conlist(SCNITREE*, SCNIPORT*, SCNITREE*, SCNIPORT*);
CHAR *Sc_cpu_time(int);
CHAR *Sc_create_leaf_cell(CHAR*);
int Sc_setup_for_maker(ARCPROTO*, ARCPROTO*);
CHAR *Sc_create_leaf_instance(CHAR*, CHAR*, int, int, int, int, int, int, CHAR*);
CHAR *Sc_create_layer2_node(int, int, int, int, CHAR*);
CHAR *Sc_create_via(int, int, CHAR*);
CHAR *Sc_create_layer1_node(int, int, int, int, CHAR*);
CHAR *Sc_create_nwell(int, int, int, int, CHAR*);
CHAR *Sc_create_pwell(int, int, int, int, CHAR*);
CHAR *Sc_create_track_layer1(CHAR*, CHAR*, CHAR*, CHAR*, int, CHAR*);
CHAR *Sc_create_track_layer2(CHAR*, CHAR*, CHAR*, CHAR*, int, CHAR*);
CHAR *Sc_create_export_port(CHAR*, CHAR*, CHAR*, int, CHAR*);
int Sc_free_placement(SCPLACE*);
int Sc_free_route(SCROUTE*);
int Sc_leaf_port_direction(CHAR*);
SCNIPORT *Sc_new_instance_port(SCNITREE*);
CHAR **Sc_leaf_cell_sim_info(CHAR*);
int Sc_leaf_cell_set_sim(SCSIM*, CHAR*);
CHAR *Sc_first_leaf_cell(void);
CHAR *Sc_next_leaf_cell(CHAR*);
int *Sc_leaf_cell_bits_address(CHAR*);
int Sc_leaf_cell_bits(CHAR*);
INTBIG ScGetParameter(INTBIG paramnum);
void ScSetParameter(INTBIG paramnum, INTBIG addr);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
