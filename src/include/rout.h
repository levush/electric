/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: rout.h
 * Header file for the wire routing tool
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

/* the meaning of "ROUT_state" */
#define STITCHMODE              07			/* type of stitching being done */
#define NOSTITCH                 0			/* no stitching */
#define AUTOSTITCH               1			/* automatic stitching by placement */
#define MIMICSTITCH              2			/* mimic user stitching */
#define SELECT                 010			/* bit set if each wire should be OKed */
#define SELSKIP                020			/* bit set if OK's should be skipped */
#define SELDONE                040			/* bit set if any selection was done */

/* the meaning of "ROUT_options" */
#define MIMICINTERACTIVE       010			/* bit set if mimic runs interactively */
#define MIMICIGNORENODESIZE    020			/* bit set if mimic ignores node size */
#define MIMICIGNORENODETYPE    040			/* bit set if mimic ignores node type */
#define MIMICUNROUTES         0100			/* bit set if mimic can unroute */
#define MIMICIGNOREPORTS      0200			/* bit set if mimic ignores ports */
#define MIMICONSIDERARCCOUNT 01000			/* bit set if mimic considers arc count */
#define MIMICOTHARCTHISDIR   02000			/* bit set if mimic allows other arcs in this direction */

extern TOOL     *ro_tool;				/* the Router tool object */
extern INTBIG    ro_statekey;			/* variable key for "ROUT_state" */
extern INTBIG    ro_state;				/* cached value for "ROUT_state" */
extern INTBIG    ro_preferedkey;		/* variable key for "ROUT_prefered_arc" */

/****************************** AUTO-STITCH ROUTING ******************************/

/* check modules */
#define NORCHECK ((RCHECK *)-1)

typedef struct Icheck
{
	NODEINST      *entity;				/* node instance being checked */
	struct Icheck *nextcheck;			/* next in list */
} RCHECK;
extern RCHECK   *ro_firstrcheck;
extern POLYLIST *ro_autostitchplist;

/****************************** MIMIC ROUTING ******************************/

typedef struct
{
	INTBIG     numcreatedarcs;			/* number of arcs just created */
	ARCINST   *createdarcs[3];			/* up to 3 newly created arcs */
	INTBIG     numcreatednodes;			/* number of nodes just created */
	NODEINST  *creatednodes[3];			/* up to 3 newly created nodes */
	INTBIG     numdeletedarcs;			/* number of arcs just created */
	ARCINST   *deletedarcs[2];			/* up to 2 deleted arcs */
	INTBIG     numdeletednodes;			/* number of arcs just created */
	NODEINST  *deletednodes[3];			/* up to 3 deleted nodes */
} ROUTACTIVITY;

extern ROUTACTIVITY ro_lastactivity;	/* last routing activity */
extern NODEINST    *ro_deletednodes[2];	/* nodes at end of last deleted arc */
extern PORTPROTO   *ro_deletedports[2];	/* ports on nodes at end of last deleted arc */

/****************************** MAZE ROUTING ******************************/

extern INTBIG ro_mazegridx;
extern INTBIG ro_mazegridy;
extern INTBIG ro_mazeoffsetx;
extern INTBIG ro_mazeoffsety;
extern INTBIG ro_mazeboundary;

/****************************** ROUTINES ******************************/

/* prototypes for tool interface */
void ro_init(INTBIG*, CHAR1*[], TOOL*);
void ro_done(void);
void ro_set(INTBIG, CHAR*[]);
void ro_slice(void);
void ro_startbatch(TOOL*, BOOLEAN);
void ro_endbatch(void);
void ro_modifynodeinst(NODEINST*,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG);
void ro_modifyarcinst(ARCINST*,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG);
void ro_modifyportproto(PORTPROTO*, NODEINST*, PORTPROTO*);
void ro_newobject(INTBIG, INTBIG);
void ro_killobject(INTBIG, INTBIG);
void ro_newvariable(INTBIG, INTBIG, INTBIG, INTBIG);

/* prototypes for intratool interface */
void    ro_autostitch(void);
INTBIG  ro_findnetends(NETWORK *net, NODEINST ***nilist, PORTPROTO ***pplist,
		INTBIG **xplist, INTBIG **yplist);
void    ro_freemazememory(void);
void    ro_freeautomemory(void);
void    ro_freemimicmemory(void);
void    ro_freerivermemory(void);
void    ro_freercheck(RCHECK*);
INTBIG  ro_getoptions(void);
void    ro_mazeroutecell(void);
void    ro_mazerouteselected(void);
void    ro_mimicstitch(BOOLEAN forced);
BOOLEAN ro_river(NODEPROTO*);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
