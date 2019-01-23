/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: planmos.h
 * Definitions and global declarations for the nMOS PLA generator
 * Written by: Sundaravarathan R. Iyengar, Schlumberger Palo Alto Research
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
#define ERRORRET   -1
#define TRANS       1					/* Transposition */
#define NOTRANS     0
#define NOROT       0					/* Rotation */

#define LEFT        0					/* Four orientations of the programming */
#define RIGHT       1					/*   transistor */
#define UP          2
#define DOWN        3
#define VDD		    0					/* Used in making Vdd, Gnd lists */
#define GND		    1

#define INPUTSIDE   1					/* Used while drawing the arcs connecting the */
#define OUTPUTSIDE  2					/* programming transistors */
#define PULLUPSIDE  3

#define DUMMY       0
#define ANDTOPGND   1					/* Gnd on top of AND plane */
#define ANDRIGHTGND 2					/* Gnd on right of AND plane */
#define ANDLEFTVDD  3					/* Vdd on left of AND plane */
#define ORTOPVDD    4					/* Vdd on top of OR plane */
#define ORRIGHTGND  5					/* Gnd on right of OR plane */
#define INCELLGND   6					/* Gnd within input cells */
#define INCELLVDD   7					/* Vdd within input cells */
#define OUTCELLGND  8					/* Gnd within output cells */
#define OUTCELLVDD  9					/* Vdd within output cells */


#define DIFF        0					/* Three arc types */
#define POLY        1
#define METAL       2

#define PLADEFTECH  x_("nmos")
#define DEFNAME     x_("Anonymous")
#define INFILE      x_("program")			/* default file name */
#define NULLSTR     x_("")

/* Define String Names for the PLA cells */
#define nmosInput   x_("nmos_Input")
#define nmosOutput  x_("nmos_Output")
#define nmosPullup  x_("nmos_Pullup")
#define nmosConnect x_("nmos_Connect")
#define nmosProgram x_("nmos_Program")

/* Declare the Node and Arc prototypes */
extern NODEPROTO *pla_pu_proto;			/* Pullup Prototype */
extern NODEPROTO *pla_in_proto;			/* Input Prototype */
extern NODEPROTO *pla_out_proto;		/* Output Prototype */
extern NODEPROTO *pla_prog_proto;		/* Programming Transistor Prototype */
extern NODEPROTO *pla_connect_proto;	/* Connect Prototype */

extern NODEPROTO *pla_cell;				/* The PLA itself */

extern NODEPROTO *pla_md_proto;			/* Metal-Diff contact */
extern NODEPROTO *pla_mp_proto;			/* Metal-Poly contact */
extern NODEPROTO *pla_bp_proto;			/* Metal (blue) pin */
extern NODEPROTO *pla_dp_proto;			/* Diff pin */
extern NODEPROTO *pla_pp_proto;			/* Poly pin */
extern NODEPROTO *pla_bc_proto;			/* Butting/Buried contact */
extern NODEPROTO *pla_dtran_proto;		/* Depletion mode transistor */
extern NODEPROTO *pla_etran_proto;		/* Enhancement mode transistor */

extern ARCPROTO  *pla_da_proto;			/* Diff arc */
extern ARCPROTO  *pla_pa_proto;			/* Poly arc */
extern ARCPROTO  *pla_ma_proto;			/* Metal arc */

/*
 * Structures to remember points on the AND and OR planes which have
 * to be connected together in metal and diff after programming.
 * We need keep track of metal arcs to be drawn for each pterm
 * and the diff arcs to be drawn for each input on the AND plane.
 * On the OR plane, there are metal lines, one for each output and there
 * are diff lines one for every two pterms.
 */

#define NOPORT ((PORT *) 0)

typedef struct pt
{
	PORTPROTO *port;					/* Port */
	INTBIG     x, y;					/* Center coordinates */
}
PORT;


#define NOPROGTRANS ((PROGTRANS *) 0)

typedef struct progtrans
{
	INTBIG            row, column;
	NODEINST         *trans;
	PORT             *diffport;			/* Diffusion arc port */
	PORT             *metalport;		/* Metal arc port */
	PORT             *polyblport;		/* Poly Bottom/Left port */
	PORT             *polytrport;		/* Poly Top/Right port */
	struct progtrans *uprogtrans;		/* Up Progtrans */
	struct progtrans *rprogtrans;		/* Right Progtrans */
} PROGTRANS;


#define NOCELLIST ((CELLIST *) 0)

typedef struct cellist
{
	INTBIG         findex;				/* cell number */
	NODEINST      *cellinst;			/* The cell itself */
	PORT          *lport;				/* Left column port */
	PORT          *rport;				/* Right column port */
	PORT          *gport;				/* Ground port */
	PROGTRANS     *lcoltrans;			/* The two programming transistors */
	PROGTRANS     *rcoltrans;			/* connected to this cell */
  struct cellist *nextcell;				/* Next cell on this list */
} CELLIST;


#define NOVDDGND ((VDDGND *) 0)

/* For keeping track of VDD and GND points */
typedef struct vddgnd
{
	NODEINST      *inst;				/* The instance itself */
	PORT          *port;				/* The port within it (firsportproto) */
	struct vddgnd *nextinst;			/* Next instance on this list */
} VDDGND;

/*  Options accessible to the designer */
extern BOOLEAN pla_verbose, pla_buttingcontact, pla_samesideoutput;
extern INTBIG  pla_userbits;
extern INTBIG  pla_inputs, pla_outputs, pla_pterms;
extern INTBIG  pla_VddWidth;			/* VDD width in use */
extern INTBIG  pla_GndWidth;			/* Vdd and Gnd Widths in use */
extern CHAR    pla_infile[100];			/* Name of the input file */
extern CHAR    pla_name[32];			/* Name of the PLA cell */

/*   Miscellaneous global variables  */
extern INTBIG  pla_halfVdd, pla_halfGnd;   /* To save repeated division by 2 */
extern VDDGND *pla_alv;					/* And left Vdd points */
extern VDDGND *pla_atg;					/* And top Gnd points */
extern VDDGND *pla_arg;					/* And right Gnd points */
extern VDDGND *pla_otv;					/* Or top Vdd points */
extern VDDGND *pla_org;					/* Or right Gnd points */
extern VDDGND *pla_icg;					/* Input cell Gnd points */
extern VDDGND *pla_icv;					/* Input cell Vdd points */
extern VDDGND *pla_ocg;					/* Output cell Gnd points */
extern VDDGND *pla_ocv;					/* Output cell Vdd points */
extern FILE   *pla_fin;					/* Pointer to the input file */

/*
 * NOTE: The following  #define statements are related to the KeyWords
 *       array declaration in planopt.c.  If the order of keywords in
 *       pla_KeyWords[] is changed, corresponding changes must be made
 *       in these statements as well.
 */
#define MAXKEYWORDS     22

#define OFF             0
#define ON              1
#define ONE             2
#define ZERO            3
#define DONTCAREX       4
#define DONTCAREM       5
#define SET             6
#define EQUAL           7
#define VERBOSE         8
#define FLEXIBLE        9
#define FIXEDANGLE      10
#define BUTTINGCONTACT  11
#define SAMESIDEOUTPUT  12
#define INPUTS          13
#define OUTPUTS         14
#define PTERMS          15
#define VDDWIDTH        16
#define GNDWIDTH        17
#define FILENAME        18
#define NAME            19
#define BEGIN           20
#define END				21
#define UNDEFINED       22

extern TOOL *pla_tool;		/* the PLA tool object */

/* prototypes for intramodule routines */
void pla_displayoptions(void);
void pla_setoption(CHAR*, CHAR*);
BOOLEAN pla_Make(void);
NODEPROTO *pla_nmos_Pullup(void);
NODEPROTO *pla_nmos_Input(void);
NODEPROTO *pla_nmos_Connect(void);
NODEPROTO *pla_nmos_Program(void);
NODEPROTO *pla_nmos_Output(void);
BOOLEAN pla_ProcessFlags(void);
INTBIG pla_GetKeyWord(CHAR*);
VDDGND *pla_MkVddGnd(INTBIG, VDDGND**);
PORT *pla_AssignPort(NODEINST*, INTBIG, INTBIG);
PORTPROTO *pla_FindPort(NODEINST*, INTBIG, INTBIG);
void pla_ConnectPlanes(INTBIG, INTBIG, INTBIG, CELLIST*, CELLIST*, CELLIST*, CELLIST*, NODEPROTO*);
void pla_RunVddGnd(INTBIG, INTBIG, NODEPROTO*);
