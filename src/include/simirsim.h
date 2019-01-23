/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: simals.h
 * Header file for asynchronous logic simulator
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

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
extern "C"
{
#endif

/* the meaning of "SIM_irsim_state": */
#define IRSIMPARASITICS   03		/* the nature of parasitics for IRSIM */
#define IRSIMPARAQUICK     0		/*   quick IRSIM parasitics (transistor only) */
#define IRSIMPARALOCAL     1		/*   local IRSIM parasitics (cell level only) */
#define IRSIMPARAFULL      2		/*   full IRSIM parasitics (hierarchical) */
#define IRSIMSHOWCOMMANDS 04		/* show executing commands on messages window */ 

#define DEFIRSIMTIMERANGE  10.0E-9f			/* initial size of simulation window: 10ns */
#define DEFIRSIMPARAMFILE  x_("scmos0.3.prm")	/* default parameter file name */
#define DEFIRSIMSTATE      IRSIMPARALOCAL	/* default value of "SIM_irsim_state" */

#define NOIRSIMTRANSISTOR ((IRSIMTRANSISTOR *)-1)

typedef struct Iirsimtransistor
{
	INTBIG                   transistortype;			/* type of transistor */
	struct Iirsimtransistor *nextsourcelist;			/* next in list of transistors with source on diffusion area */
	struct Iirsimtransistor *nextdrainlist;				/* next in list of transistors with drain on diffusion area */
	INTBIG                   xpos, ypos;				/* location of transistor */
	INTBIG                   length, width;				/* size of transistor */
	INTBIG                   source, gate, drain;		/* network of source, gate, drain */
	INTBIG                   sourcex, sourcey;			/* location of transistor source */
	INTBIG                   drainx, drainy;			/* location of transistor drain */
	float                    sarea, darea;				/* area of source/drain */
	INTBIG                   sperimeter, dperimeter;	/* perimeter of source/drain */
	float                    m;                         /* magnitude factor */
	struct Iirsimtransistor *nextirsimtransistor;		/* next in list of all transistors */
	struct Iirsimtransistor *nextcellirsimtransistor;	/* next in list of transistors in this cell */
} IRSIMTRANSISTOR;

/* the meaning of IRSIMNETWORK->flags: */
#define IRSIMNETVALID      1					/* set if network is valid */
#define IRSIMNETSHOWN      2					/* set if network is shown */

typedef struct Iirsimnetwork
{
	CHAR      *name;							/* net name in IRSIM */
	INTBIG     signal;							/* waveform pointer */
	INTBIG     flags;							/* flags for this network */
} IRSIMNETWORK;

#if SIMFSDBWRITE != 0
typedef struct Iirsimalias
{
	CHAR      *name;							/* net name in IRSIM (IRSIMNETWORK.name) */
	CHAR      *alias;                           /* its alias */
} IRSIMALIAS;
#endif

extern IRSIMNETWORK    *sim_irsimnets;
extern IRSIMTRANSISTOR *sim_firstirsimtransistor;
extern INTBIG           sim_irsimnetnumber;
extern INTBIG           sim_irsim_statekey;		/* variable key for "SIM_irsim_state" */
#if SIMFSDBWRITE != 0
extern IRSIMALIAS      *sim_irsimaliases;
extern INTBIG           sim_irsimaliasnumber;
#endif

void     sim_irsimgeneratedeck(NODEPROTO *cell, FILE *f);
#if SIMTOOLIRSIM != 0
BOOLEAN  irsim_startsimulation(NODEPROTO *np);
BOOLEAN  irsim_charhandlerschem(WINDOWPART *w, INTSML chr, INTBIG special);
BOOLEAN  irsim_charhandlerwave(WINDOWPART *w, INTSML chr, INTBIG special);
void     irsim_clearallvectors(void);
void     irsim_loadvectorfile(CHAR *filename);
void     irsim_savevectorfile(CHAR *filename);
void     irsim_level_up(NODEPROTO *cell);
void     irsim_level_set(CHAR *level, NODEPROTO *cell);
BOOLEAN  irsim_topofinstances(CHAR **c);
CHAR    *irsim_nextinstance(void);
void     irsim_freememory(void);
void     irsim_reportsignals(WINDOWPART *simwin, void *(*addbranch)(CHAR*, void*),
			void *(*findbranch)(CHAR*, void*), void *(*addleaf)(CHAR*, void*), CHAR *(*nodename)(void*));
void     irsim_adddisplayedsignal(CHAR *sig);
#endif

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
