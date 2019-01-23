/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tecfpga.c
 * FPGA technology
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

#define USE_REDOPRIM            /* use "net_redoprim" for network calculation */

#include "config.h"
#include "global.h"
#include "egraphics.h"
#include "tech.h"
#include "tecfpga.h"
#include "tecgen.h"
#include "efunction.h"
#include "usr.h"
#include "edialogs.h"
#ifdef USE_REDOPRIM
# include "network.h"
#endif

/******************** TREE STRUCTURE FOR ARCHITECTURE FILE ********************/

#define MAXLINE	   500		/* max characters on FPGA input line */
#define MAXDEPTH	50		/* max depth of FPGA nesting */

#define PARAMBRANCH  1		/* parameter is a subtree */
#define PARAMATOM    2		/* parameter is atomic */

typedef struct
{
	CHAR   *keyword;
	INTBIG  lineno;
	INTBIG  paramtotal;
	INTBIG  parameters;
	INTBIG *paramtype;
	void  **paramvalue;
} LISPTREE;

static LISPTREE *fpga_treestack[MAXDEPTH];
static INTBIG    fpga_treedepth;
static LISPTREE *fpga_treepos;
static LISPTREE *fpga_freelisptree = 0;

/******************** ADDITIONAL INFORMATION ABOUT PRIMITIVES ********************/

#define ACTIVEPART   1			/* set if segment or pip is active */
#define ACTIVESAVE   2			/* saved area for segment/pip activity */

typedef struct
{
	INTBIG     posx, posy;
	INTBIG     con;
	PORTPROTO *pp;
} FPGAPORT;

typedef struct
{
	CHAR      *netname;
	INTBIG     segactive;
	INTBIG     segcount;
	INTBIG    *segfx, *segfy;
	INTBIG    *segtx, *segty;
} FPGANET;

typedef struct
{
	CHAR      *pipname;
	INTBIG     pipactive;
	INTBIG     con1, con2;
	INTBIG     posx, posy;
} FPGAPIP;

typedef struct
{
	INTBIG       portcount;
	FPGAPORT   **portlist;
	INTBIG       netcount;
	FPGANET    **netlist;
	INTBIG       pipcount;
	FPGAPIP    **piplist;
} FPGANODE;

static INTBIG     fpga_nodecount;
static FPGANODE **fpga_nodes;

/******************** STANDARD TECHNOLOGY STRUCTURES ********************/

/* the options table */
static COMCOMP fpgareadp = {NOKEYWORD, topoffile, nextfile, NOPARAMS,
	0, x_(" \t"), M_("FPGA Architecture file"), x_("")};
static KEYWORD fpgadltopt[] =
{
	{x_("on"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("off"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP fpgatdispp = {fpgadltopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("FPGA text display option"), x_("")};
static KEYWORD fpgadlopt[] =
{
	{x_("full"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("active"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("text"),          1,{&fpgatdispp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("empty"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP fpgadispp = {fpgadlopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("FPGA display level"), x_("")};
static KEYWORD fpgaopt[] =
{
	{x_("read-architecture-file"),    1,{&fpgareadp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("only-primitives-file"),      1,{&fpgareadp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("display-level"),             1,{&fpgadispp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clear-node-cache"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("wipe-cache"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP fpga_parse = {fpgaopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("FPGA option"), x_("")};

/* the display level */
#define DISPLAYLEVEL       07		/* level of display */
#define NOPRIMDISPLAY       0		/*   display no internals */
#define FULLPRIMDISPLAY    01		/*   display all internals */
#define ACTIVEPRIMDISPLAY  02		/*   display only active internals */
#define TEXTDISPLAY       010		/* set to display text */

static TECHNOLOGY    *fpga_tech;
static INTBIG         fpga_internaldisplay;
static INTBIG         fpga_curpip;
static INTBIG         fpga_curnet, fpga_cursegment;
static INTBIG         fpga_lineno;
static INTBIG         fpga_filesize;
static INTBIG         fpga_activepipskey;		/* variable key for "FPGA_activepips" */
static INTBIG         fpga_activerepeaterskey;	/* variable key for "FPGA_activerepeaters" */
static INTBIG         fpga_nodepipcachekey;		/* variable key for "FPGA_nodepipcache" */
static INTBIG         fpga_arcactivecachekey;	/* variable key for "FPGA_arcactivecache" */
static FPGANODE      *fpga_fn;					/* current pointer for pip examining */
static CHAR          *fpga_repeatername;		/* name of current repeater for activity examining */
static BOOLEAN        fpga_repeaterisactive;	/* nonzero if current repeater is found to be active */
static NODEPROTO     *fpga_wirepinprim;			/* wire pin */
static NODEPROTO     *fpga_repeaterprim;		/* repeater */

/* working memory for "fpga_arcactive()" */
static INTBIG         fpga_arcbufsize = 0;
static UCHAR1        *fpga_arcbuf;

/* working memory for "fpga_reevaluatepips()" */
static INTBIG         fpga_pipbufsize = 0;
static UCHAR1        *fpga_pipbuf;

/* prototypes for local routines */
static BOOLEAN    fpga_addparameter(LISPTREE *tree, INTBIG type, void *value);
static LISPTREE  *fpga_alloclisptree(void);
static BOOLEAN    fpga_arcactive(ARCINST *ai);
static BOOLEAN    fpga_arcendactive(ARCINST *ai, INTBIG j);
static void       fpga_clearcache(NODEINST *ni);
static void       fpga_describenetseg(NODEINST *ni, FPGANET *fnet, INTBIG whichseg, POLYGON *poly);
static void       fpga_describepip(NODEINST *ni, FPGANODE *fn, INTBIG pipindex, POLYGON *poly);
static void       fpga_findvariableobjects(NODEINST *ni, INTBIG varkey, void (*setit)(CHAR*));
static void       fpga_killptree(LISPTREE *lt);
static BOOLEAN    fpga_makeblockinstance(NODEPROTO *np, LISPTREE *lt);
static BOOLEAN    fpga_makeblocknet(NODEPROTO *cell, LISPTREE *lt);
static BOOLEAN    fpga_makeblockport(NODEPROTO *np, LISPTREE *lt);
static BOOLEAN    fpga_makeblockrepeater(NODEPROTO *np, LISPTREE *lt);
static NODEPROTO *fpga_makecell(LISPTREE *lt);
static BOOLEAN    fpga_makeprimitive(LISPTREE *lt);
static INTBIG     fpga_makeprimitives(LISPTREE *lt);
static BOOLEAN    fpga_makeprimnet(NODEPROTO *np, LISPTREE *lt, FPGANODE *fn, FPGANET *fnet);
static BOOLEAN    fpga_makeprimpip(NODEPROTO *np, LISPTREE *lt, FPGANODE *fn, FPGAPIP *fpip);
static BOOLEAN    fpga_makeprimport(NODEPROTO *np, LISPTREE *lt, FPGAPORT *fp);
static NODEPROTO *fpga_placeprimitives(LISPTREE *lt);
static BOOLEAN    fpga_pushkeyword(CHAR *pt);
static LISPTREE  *fpga_readfile(FILE *f, void *dia);
static void       fpga_reevaluatepips(NODEINST *ni, FPGANODE *fn);
static BOOLEAN    fpga_repeateractive(NODEINST *ni);
static void       fpga_setpips(CHAR *name);
static void       fpga_setrepeater(CHAR *name);
static INTBIG     fpga_intarcpolys(ARCINST *ai, WINDOWPART *win, POLYLOOP *pl);
static void       fpga_intshapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly, POLYLOOP *pl);
static INTBIG     fpga_intnodepolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win, POLYLOOP *pl);
static void       fpga_intshapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl);

/******************** LAYERS ********************/

#define MAXLAYERS   4		/* total layers below      */

#define LWIRE       0		/* wire                    */
#define LCOMP       1		/* component               */
#define LPIP        2		/* pip                     */
#define LREPEATER   3		/* repeater                */

static GRAPHICS fpga_w_lay = {LAYERO, RED, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
	0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};
static GRAPHICS fpga_c_lay = {LAYERO, BLACK, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
	0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};
static GRAPHICS fpga_p_lay = {LAYERO, GREEN, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
	0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};
static GRAPHICS fpga_r_lay = {LAYERO, BLUE, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
	0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};

/* these tables must be updated together */
GRAPHICS *fpga_layers[MAXLAYERS+1] = {&fpga_w_lay, &fpga_c_lay, &fpga_p_lay, &fpga_r_lay, NOGRAPHICS};
static CHAR *fpga_layer_names[MAXLAYERS] = {x_("Wire"), x_("Component"), x_("Pip"), x_("Repeater")};
static INTBIG fpga_layer_function[MAXLAYERS] = {LFMETAL1, LFART, LFART, LFART};
static CHAR *fpga_layer_letters[MAXLAYERS] = {x_("w"), x_("c"), x_("p"), x_("r")};

/******************** ARCS ********************/

#define ARCPROTOCOUNT   1

#define AWIRE           0	/* wire               */

/* wire arc */
static TECH_ARCLAY fpga_al_w[] = { {LWIRE,0,FILLED} };
static TECH_ARCS fpga_a_w = {
	x_("wire"),0,AWIRE,NOARCPROTO,													/* name */
	1,fpga_al_w,																	/* layers */
	(APMETAL1<<AFUNCTIONSH)|WANTFIXANG|WANTCANTSLIDE|AEDGESELECT|(45<<AANGLEINCSH)}; /* userbits */

TECH_ARCS *fpga_arcprotos[ARCPROTOCOUNT+1] = {&fpga_a_w, ((TECH_ARCS *)-1)};

/******************** PORTINST CONNECTIONS ********************/

/* these values are replaced with actual arcproto addresses */
static INTBIG fpga_pc_wire[]	= {-1, AWIRE, ALLGEN, -1};

/******************** NODES ********************/

#define NODEPROTOCOUNT	3

#define NWIREPIN	 1		/* wire pin */
#define NPIP		 2		/* pip */
#define NREPEATER	 3		/* repeater */

/******************** POLYGONS ********************/

static INTBIG fpga_g_pindisc[]   = {CENTER,    CENTER,    RIGHTEDGE, CENTER};
static INTBIG fpga_g_fullbox[8]  = {LEFTEDGE, BOTEDGE,  RIGHTEDGE,TOPEDGE};

/******************** NODES ********************/

/* wire-pin */
static TECH_PORTS fpga_wirepin_p[] = {				/* ports */
	{fpga_pc_wire, x_("wire"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON fpga_wirepin_l[] = {			/* layers */
	{LWIRE, 0, 2, DISC, POINTS, fpga_g_pindisc}};
static TECH_NODES fpga_wirepin = {
	x_("Wire_Pin"),NWIREPIN,NONODEPROTO,			/* name */
	K1,K1,											/* size */
	1,fpga_wirepin_p,								/* ports */
	1,fpga_wirepin_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|NSQUARE|WIPEON1OR2,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* pip */
static TECH_PORTS fpga_pip_p[] = {					/* ports */
	{fpga_pc_wire, x_("pip"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON fpga_pip_l[] = {				/* layers */
	{LPIP, 0, 4, FILLEDRECT, BOX, fpga_g_fullbox}};
static TECH_NODES fpga_pip = {
	x_("Pip"),NPIP,NONODEPROTO,						/* name */
	K2,K2,											/* size */
	1,fpga_pip_p,									/* ports */
	1,fpga_pip_l,									/* layers */
	(NPCONNECT<<NFUNCTIONSH)|NSQUARE,				/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* repeater */
static TECH_PORTS fpga_repeater_p[] = {				/* ports */
	{fpga_pc_wire, x_("a"), NOPORTPROTO, (180<<PORTANGLESH) | (45<<PORTARANGESH),
		LEFTEDGE, CENTER, LEFTEDGE, CENTER},
	{fpga_pc_wire, x_("b"), NOPORTPROTO, (45<<PORTARANGESH) | (1<<PORTNETSH),
		RIGHTEDGE, CENTER, RIGHTEDGE, CENTER}};
static TECH_POLYGON fpga_repeater_l[] = {			/* layers */
	{LREPEATER, 0, 4, FILLEDRECT, BOX, fpga_g_fullbox}};
static TECH_NODES fpga_repeater = {
	x_("Repeater"),NREPEATER,NONODEPROTO,			/* name */
	K10,K3,											/* size */
	2,fpga_repeater_p,								/* ports */
	1,fpga_repeater_l,								/* layers */
	(NPCONNECT<<NFUNCTIONSH),						/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

TECH_NODES *fpga_nodeprotos[NODEPROTOCOUNT+1] = {
	&fpga_wirepin,  &fpga_pip, &fpga_repeater,
	((TECH_NODES *)-1)
};

static INTBIG fpga_node_widoff[NODEPROTOCOUNT*4] = {
	 H0,H0,H0,H0,   H1,H1,H1,H1,   0,0,0,0
};

/******************** VARIABLE AGGREGATION ********************/

TECH_VARIABLES fpga_variables[] =
{
	/* set general information about the technology */
	{x_("TECH_layer_names"), (CHAR *)fpga_layer_names, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_layer_function"), (CHAR *)fpga_layer_function, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_node_width_offset"), (CHAR *)fpga_node_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|((NODEPROTOCOUNT*4)<<VLENGTHSH)},

	/* set information for the USER analysis tool */
	{x_("USER_layer_letters"), (CHAR *)fpga_layer_letters, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{NULL, NULL, 0.0, 0}
};

/******************** INTERFACE ROUTINES ********************/

BOOLEAN fpga_initprocess(TECHNOLOGY *tech, INTBIG pass)
{
	if (pass == 0) fpga_tech = tech; else
		if (pass == 1)
	{
		fpga_wirepinprim = getnodeproto(x_("fpga:Wire_Pin"));
		fpga_repeaterprim = getnodeproto(x_("fpga:Repeater"));
		fpga_activepipskey = makekey(x_("FPGA_activepips"));
		fpga_activerepeaterskey = makekey(x_("FPGA_activerepeaters"));
		fpga_nodepipcachekey = makekey(x_("FPGA_nodepipcache"));
		fpga_arcactivecachekey = makekey(x_("FPGA_arcactivecache"));
	}
	fpga_internaldisplay = ACTIVEPRIMDISPLAY | TEXTDISPLAY;
	fpga_nodecount = 0;
	return(FALSE);
}

void fpga_termprocess(void)
{
#ifdef DEBUGMEMORY
	LISPTREE *lt;
	REGISTER INTBIG i, j;
	REGISTER FPGANET *fnet;
	REGISTER FPGAPIP *fpip;
	REGISTER FPGANODE *fn;

	if (fpga_arcbufsize > 0) efree((CHAR *)fpga_arcbuf);
	if (fpga_pipbufsize > 0) efree((CHAR *)fpga_pipbuf);

	/* deallocate lisp-tree objects */
	while (fpga_freelisptree != 0)
	{
		lt = fpga_freelisptree;
		fpga_freelisptree = (LISPTREE *)fpga_freelisptree->paramvalue;
		efree((CHAR *)lt);
	}

	/* free the extra info associated with the technology */
	for(i=0; i<fpga_nodecount; i++)
	{
		fn = fpga_nodes[i];

#ifndef USE_REDOPRIM
		for(j=0; j<fn->portcount; j++)
		{
			fp = fn->portlist[j];
			if (fp->pp == NOPORTPROTO || fp->pp->network == NONETWORK) continue;

			for(k=j+1; k<fn->portcount; k++)
			{
				ofp = fn->portlist[k];
				if (ofp->pp == NOPORTPROTO || ofp->pp->network == NONETWORK) continue;
				if (ofp->pp->network == fp->pp->network) ofp->pp->network = NONETWORK;
			}
			efree((CHAR *)fp->pp->network);
		}
#endif
		if (fn->portcount > 0)
		{
			efree((CHAR *)fn->portlist[0]);
			efree((CHAR *)fn->portlist);
		}

		for(j=0; j<fn->netcount; j++)
		{
			fnet = fn->netlist[j];
			if (fnet->netname != 0) efree((CHAR *)fnet->netname);
			if (fnet->segcount > 0)
				efree((CHAR *)fnet->segfx);
		}
		if (fn->netcount > 0)
		{
			efree((CHAR *)fn->netlist[0]);
			efree((CHAR *)fn->netlist);
		}

		for(j=0; j<fn->pipcount; j++)
		{
			fpip = fn->piplist[j];
			if (fpip->pipname != 0) efree((CHAR *)fpip->pipname);
		}
		if (fn->pipcount > 0)
		{
			efree((CHAR *)fn->piplist[0]);
			efree((CHAR *)fn->piplist);
		}

		efree((CHAR *)fn);
	}
	if (fpga_nodecount > 0) efree((CHAR *)fpga_nodes);
#endif
}

void fpga_setmode(INTBIG count, CHAR *par[])
{
	REGISTER CHAR *pp;
	REGISTER NODEPROTO *topcell;
	REGISTER NODEINST *ni;
	CHAR *filename, *subpar[1];
	FILE *f;
	REGISTER INTBIG l, total;
	REGISTER LISPTREE *lt;
	static INTBIG filetypefpga = -1;
	REGISTER void *infstr, *dia;

	if (count == 0)
	{
		ttyputusage(x_("technology tell fpga OPTIONS"));
		return;
	}

	l = estrlen(pp = par[0]);
	if (namesamen(pp, x_("display-level"), l) == 0)
	{
		if (count == 1)
		{
			infstr = initinfstr();
			switch (fpga_internaldisplay & DISPLAYLEVEL)
			{
				case NOPRIMDISPLAY:     addstringtoinfstr(infstr, _("No internal display"));       break;
				case FULLPRIMDISPLAY:   addstringtoinfstr(infstr, _("Full internal display"));     break;
				case ACTIVEPRIMDISPLAY: addstringtoinfstr(infstr, _("Active internal display"));   break;
			}
			if ((fpga_internaldisplay & TEXTDISPLAY) == 0)
				 addstringtoinfstr(infstr, _(", no text")); else
					 addstringtoinfstr(infstr, _(", with text"));
			ttyputmsg(x_("%s"), returninfstr(infstr));
			return;
		}

		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("empty"), l) == 0)
		{
			fpga_internaldisplay = (fpga_internaldisplay & ~DISPLAYLEVEL) | NOPRIMDISPLAY;
			ttyputverbose(M_("No internal display"));
			return;
		}
		if (namesamen(pp, x_("full"), l) == 0)
		{
			fpga_internaldisplay = (fpga_internaldisplay & ~DISPLAYLEVEL) | FULLPRIMDISPLAY;
			ttyputverbose(M_("Full internal display"));
			return;
		}
		if (namesamen(pp, x_("active"), l) == 0)
		{
			fpga_internaldisplay = (fpga_internaldisplay & ~DISPLAYLEVEL) | ACTIVEPRIMDISPLAY;
			ttyputverbose(M_("Active internal display"));
			return;
		}
		if (namesamen(pp, x_("text"), l) == 0)
		{
			if (count == 2)
			{
				if ((fpga_internaldisplay & TEXTDISPLAY) == 0)
					ttyputmsg(M_("Text not displayed")); else
						ttyputmsg(M_("Text is displayed"));
				return;
			}
			l = estrlen(pp = par[2]);
			if (namesamen(pp, x_("on"), l) == 0 && l >= 2)
			{
				fpga_internaldisplay |= TEXTDISPLAY;
				ttyputverbose(M_("Text is displayed"));
				return;
			}
			if (namesamen(pp, x_("off"), l) == 0 && l >= 2)
			{
				fpga_internaldisplay &= ~TEXTDISPLAY;
				ttyputverbose(M_("Text not displayed"));
				return;
			}
			ttyputbadusage(x_("technology tell fpga display-level text"));
			return;
		}
		ttyputbadusage(x_("technology tell fpga display-level"));
		return;
	}

	if (namesamen(pp, x_("wipe-cache"), l) == 0)
	{
		fpga_clearcache(NONODEINST);
		return;
	}
	if (namesamen(pp, x_("clear-node-cache"), l) == 0)
	{
		ni = (NODEINST *)us_getobject(VNODEINST, FALSE);
		if (ni == NONODEINST) return;
		fpga_clearcache(ni);
		return;
	}

	if (namesamen(pp, x_("read-architecture-file"), l) == 0 || namesamen(pp, x_("only-primitives-file"), l) == 0)
	{
		if (fpga_nodecount != 0)
		{
			ttyputmsg(_("This technology already has primitives defined"));
			return;
		}

		if (count <= 1)
		{
			ttyputmsg(_("Need FILENAME after 'read' keyword"));
			return;
		}

		/* get architecture file */
		if (filetypefpga < 0)
			filetypefpga = setupfiletype(x_("fpga"), x_("*.fpga"), MACFSTAG('TEXT'), FALSE, x_("fpga"), _("FPGA Architecture"));
		f = xopen(par[1], filetypefpga, el_libdir, &filename);
		if (f == NULL)
		{
			ttyputerr(_("Cannot find %s"), par[1]);
			return;
		}

		/* prepare for input */
		fpga_filesize = filesize(f);
		dia = DiaInitProgress(_("Reading FPGA architecture file..."), 0);
		if (dia == 0)
		{
			xclose(f);
			return;
		}
		DiaSetProgress(dia, 0, fpga_filesize);

		/* read the file */
		lt = fpga_readfile(f, dia);
		DiaDoneProgress(dia);
		xclose(f);
		if (lt == 0)
		{
			ttyputerr(_("Error reading file"));
			return;
		}
		ttyputmsg(_("FPGA file %s read"), par[1]);

		/* turn the tree into primitives */
		total = fpga_makeprimitives(lt);
		ttyputmsg(_("Created %ld primitives"), total);
#ifdef USE_REDOPRIM
		net_redoprim();
#endif

		/* place and wire the primitives */
		if (namesamen(pp, x_("read-architecture-file"), l) == 0)
		{
			topcell = fpga_placeprimitives(lt);
			if (topcell != NONODEPROTO)
			{
				/* recompute bounds */
				(*el_curconstraint->solve)(NONODEPROTO);

				/* recompute networks */
				(void)asktool(net_tool, x_("total-re-number"));

				/* display top cell */
				subpar[0] = describenodeproto(topcell);
				us_editcell(1, subpar);
			}
		}

		/* release tree memory */
		fpga_killptree(lt);
		return;
	}
	ttyputbadusage(x_("technology tell fpga"));
}

INTBIG fpga_nodepolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win)
{
	return(fpga_intnodepolys(ni, reasonable, win, &tech_oneprocpolyloop));
}

INTBIG fpga_intnodepolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win, POLYLOOP *pl)
{
	REGISTER INTBIG total, i, pindex, j, otherend;
	INTBIG depth, *indexlist;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *oni;
	NODEINST **nilist;
	REGISTER PORTARCINST *pi;
	REGISTER FPGANODE *fn;

	/* get the default number of polygons and list of layers */
	pindex = ni->proto->primindex;
	if (pindex <= NODEPROTOCOUNT)
	{
		/* static primitive */
		total = fpga_nodeprotos[pindex-1]->layercount;
		switch(pindex)
		{
			case NWIREPIN:
				if (tech_pinusecount(ni, win)) total = 0;
				break;
			case NREPEATER:
				if ((fpga_internaldisplay&DISPLAYLEVEL) == ACTIVEPRIMDISPLAY)
				{
					if (!fpga_repeateractive(ni)) total = 0;
				}
				break;
		}
	} else
	{
		/* dynamic primitive */
		switch (fpga_internaldisplay & DISPLAYLEVEL)
		{
			case NOPRIMDISPLAY:
				total = 1;
				if ((fpga_internaldisplay&TEXTDISPLAY) != 0) total++;
				break;
			case ACTIVEPRIMDISPLAY:
				/* count number of active nets and pips */
				fn = fpga_nodes[pindex - NODEPROTOCOUNT - 1];

				/* hard reset of all segment and pip activity */
				for(i=0; i<fn->netcount; i++) fn->netlist[i]->segactive = 0;
				for(i=0; i<fn->pipcount; i++) fn->piplist[i]->pipactive = 0;

				/* determine the active segments and pips */
				fpga_reevaluatepips(ni, fn);

				/* save the activity bits */
				for(i=0; i<fn->netcount; i++)
					if ((fn->netlist[i]->segactive&ACTIVEPART) != 0)
						fn->netlist[i]->segactive |= ACTIVESAVE;
				for(i=0; i<fn->pipcount; i++)
					if ((fn->piplist[i]->pipactive&ACTIVEPART) != 0)
						fn->piplist[i]->pipactive |= ACTIVESAVE;

				/* propagate inactive segments to others that may be active */
				gettraversalpath(ni->parent, NOWINDOWPART, &nilist, &indexlist, &depth, 1);
				if (depth > 0)
				{
					oni = nilist[depth-1];
					uphierarchy();
					for(i=0; i<fn->netcount; i++)
					{
						if ((fn->netlist[i]->segactive&ACTIVESAVE) != 0) continue;
						for(j=0; j<fn->portcount; j++)
						{
							if (fn->portlist[j]->con != i) continue;
							for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
							{
								if (pi->proto != fn->portlist[j]->pp) continue;
								ai = pi->conarcinst;
								if (ai->end[0].nodeinst == ni) otherend = 1; else otherend = 0;
								if (fpga_arcendactive(ai, otherend)) break;
							}
							if (pi != NOPORTARCINST) break;
						}
						if (j < fn->portcount) fn->netlist[i]->segactive |= ACTIVESAVE;
					}
					downhierarchy(oni, oni->proto, 0);
				}

				/* add up the active segments */
				total = 1;
				for(i=0; i<fn->pipcount; i++)
					if ((fn->piplist[i]->pipactive&ACTIVESAVE) != 0) total++;
				for(i=0; i<fn->netcount; i++)
					if ((fn->netlist[i]->segactive&ACTIVESAVE) != 0)
						total += fn->netlist[i]->segcount;
				fpga_curpip = fpga_curnet = 0;   fpga_cursegment = -1;
				break;
			case FULLPRIMDISPLAY:
				fn = fpga_nodes[pindex - NODEPROTOCOUNT - 1];
				total = fn->pipcount + 1;
				for(i=0; i<fn->netcount; i++) total += fn->netlist[i]->segcount;
				fpga_curnet = fpga_cursegment = 0;
				break;
			default:
				total = 0;
		}
	}

	/* add in displayable variables */
	pl->realpolys = total;
	if ((fpga_internaldisplay&TEXTDISPLAY) != 0)
		total += tech_displayablenvars(ni, pl->curwindowpart, pl);
	if (reasonable != 0) *reasonable = total;
	return(total);
}

void fpga_shapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly)
{
	fpga_intshapenodepoly(ni, box, poly, &tech_oneprocpolyloop);
}

void fpga_intshapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl)
{
	REGISTER INTBIG pindex;
	REGISTER INTBIG lambda;
	REGISTER FPGANODE *fn;
	REGISTER FPGANET *fnet;

	/* handle displayable variables */
	if (box >= pl->realpolys)
	{
		(void)tech_filldisplayablenvar(ni, poly, pl->curwindowpart, 0, pl);
		return;
	}

	lambda = lambdaofnode(ni);
	pindex = ni->proto->primindex;
	if (pindex <= NODEPROTOCOUNT)
	{
		/* static primitive */
		tech_fillpoly(poly, &fpga_nodeprotos[pindex-1]->layerlist[box], ni, lambda,
			FILLED);
		poly->desc = fpga_layers[poly->layer];
		return;
	}

	/* dynamic primitive */
	if (box == 0)
	{
		/* first box is always the outline */
		if (poly->limit < 2) (void)extendpolygon(poly, 2);
		subrange(ni->lowx, ni->highx, -H0, 0, H0, 0, &poly->xv[0], &poly->xv[1], lambda);
		subrange(ni->lowy, ni->highy, -H0, 0, H0, 0, &poly->yv[0], &poly->yv[1], lambda);
		poly->count = 2;
		poly->style = CLOSEDRECT;
		poly->layer = LCOMP;
	} else
	{
		/* subsequent boxes depend on the display level */
		switch (fpga_internaldisplay & DISPLAYLEVEL)
		{
			case NOPRIMDISPLAY:
				/* just the name */
				if (poly->limit < 4) (void)extendpolygon(poly, 4);
				subrange(ni->lowx, ni->highx, -H0, 0, H0, 0, &poly->xv[0], &poly->xv[2], lambda);
				subrange(ni->lowy, ni->highy, -H0, 0, H0, 0, &poly->yv[0], &poly->yv[1], lambda);
				poly->xv[1] = poly->xv[0];   poly->xv[3] = poly->xv[2];
				poly->yv[3] = poly->yv[0];   poly->yv[2] = poly->yv[1];
				poly->count = 4;
				poly->style = TEXTBOX;
				poly->string = ni->proto->protoname;
				TDCLEAR(poly->textdescript);
				TDSETSIZE(poly->textdescript, TXTSETQLAMBDA(12));
				poly->tech = fpga_tech;
				poly->layer = LCOMP;
				break;

			case ACTIVEPRIMDISPLAY:
				fn = fpga_nodes[pindex - NODEPROTOCOUNT - 1];

				/* draw active segments */
				if (fpga_curnet < fn->netcount)
				{
					/* advance to next active net */
					if (fpga_cursegment < 0)
					{
						for( ; fpga_curnet<fn->netcount; fpga_curnet++)
							if ((fn->netlist[fpga_curnet]->segactive&ACTIVESAVE) != 0) break;
						fpga_cursegment = 0;
					}

					/* add in a net segment */
					if (fpga_curnet < fn->netcount)
					{
						fnet = fn->netlist[fpga_curnet];
						fpga_describenetseg(ni, fnet, fpga_cursegment, poly);

						/* advance to next segment */
						fpga_cursegment++;
						if (fpga_cursegment >= fn->netlist[fpga_curnet]->segcount)
						{
							fpga_curnet++;
							fpga_cursegment = -1;
						}
						break;
					}
				}

				/* draw active pips */
				if (fpga_curpip < fn->pipcount)
				{
					for( ; fpga_curpip<fn->pipcount; fpga_curpip++)
						if ((fn->piplist[fpga_curpip]->pipactive&ACTIVESAVE) != 0) break;
					if (fpga_curpip < fn->pipcount)
					{
						fpga_describepip(ni, fn, fpga_curpip, poly);
						fpga_curpip++;
						break;
					}
				}
				break;

			case FULLPRIMDISPLAY:
				/* show pips */
				fn = fpga_nodes[pindex - NODEPROTOCOUNT - 1];
				if (box <= fn->pipcount)
				{
					fpga_describepip(ni, fn, box-1, poly);
					break;
				}

				/* add in a net segment */
				fnet = fn->netlist[fpga_curnet];
				fpga_describenetseg(ni, fnet, fpga_cursegment, poly);

				/* advance to next segment */
				fpga_cursegment++;
				if (fpga_cursegment >= fn->netlist[fpga_curnet]->segcount)
				{
					fpga_curnet++;
					fpga_cursegment = 0;
				}
				break;
		}
	}
	poly->desc = fpga_layers[poly->layer];
}

/*
 * Warning: to make this routine truly callable in parallel, you must either:
 * (1) take care of the setting of globals such as "fpga_nodes".
 * (2) wrap the routine in mutual-exclusion locks
 */
INTBIG fpga_allnodepolys(NODEINST *ni, POLYLIST *plist, WINDOWPART *win, BOOLEAN onlyreasonable)
{
	REGISTER INTBIG tot, j;
	INTBIG reasonable;
	REGISTER NODEPROTO *np;
	REGISTER POLYGON *poly;
	POLYLOOP mypl;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	np = ni->proto;
	mypl.curwindowpart = win;
	tot = fpga_intnodepolys(ni, &reasonable, win, &mypl);
	if (onlyreasonable) tot = reasonable;
	if (mypl.realpolys < tot) tot = mypl.realpolys;
	if (ensurepolylist(plist, tot, db_cluster)) return(-1);
	for(j = 0; j < tot; j++)
	{
		poly = plist->polygons[j];
		poly->tech = fpga_tech;
		fpga_intshapenodepoly(ni, j, poly, &mypl);
	}
	return(tot);
}

INTBIG fpga_nodeEpolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win)
{
	Q_UNUSED( ni );
	Q_UNUSED( win );

	if (reasonable != 0) *reasonable = 0;
	return(0);
}

void fpga_shapeEnodepoly(NODEINST *ni, INTBIG box, POLYGON *poly)
{
	Q_UNUSED( ni );
	Q_UNUSED( box );
	Q_UNUSED( poly );
}

INTBIG fpga_allnodeEpolys(NODEINST *ni, POLYLIST *plist, WINDOWPART *win, BOOLEAN onlyreasonable)
{
	Q_UNUSED( ni );
	Q_UNUSED( plist );
	Q_UNUSED( win );
	Q_UNUSED( onlyreasonable );
	return(0);
}

void fpga_shapeportpoly(NODEINST *ni, PORTPROTO *pp, POLYGON *poly, XARRAY trans, BOOLEAN purpose)
{
	REGISTER INTBIG pindex, i;
	REGISTER FPGANODE *fn;
	Q_UNUSED( purpose );

	pindex = ni->proto->primindex;
	if (pindex <= NODEPROTOCOUNT)
	{
		/* static primitive */
		tech_fillportpoly(ni, pp, poly, trans, fpga_nodeprotos[pindex-1], CLOSED, lambdaofnode(ni));
		return;
	}

	/* dynamic primitive */
	if (poly->limit < 1) (void)extendpolygon(poly, 1);
	fn = fpga_nodes[pindex - NODEPROTOCOUNT - 1];
	poly->count = 1;
	poly->style = CROSS;
	poly->xv[0] = (ni->lowx+ni->highx) / 2;
	poly->yv[0] = (ni->lowy+ni->highy) / 2;
	for(i=0; i<fn->portcount; i++)
		if (fn->portlist[i]->pp == pp)
	{
		poly->xv[0] += fn->portlist[i]->posx;
		poly->yv[0] += fn->portlist[i]->posy;
		break;
	}
	xform(poly->xv[0], poly->yv[0], &poly->xv[0], &poly->yv[0], trans);
}

INTBIG fpga_arcpolys(ARCINST *ai, WINDOWPART *win)
{
	return(fpga_intarcpolys(ai, win, &tech_oneprocpolyloop));
}

INTBIG fpga_intarcpolys(ARCINST *ai, WINDOWPART *win, POLYLOOP *pl)
{
	REGISTER INTBIG i, aindex;

	aindex = ai->proto->arcindex;

	/* presume display of the arc */
	i = fpga_arcprotos[aindex]->laycount;
	if ((fpga_internaldisplay&DISPLAYLEVEL) == NOPRIMDISPLAY ||
		(fpga_internaldisplay&DISPLAYLEVEL) == ACTIVEPRIMDISPLAY)
	{
		if (!fpga_arcactive(ai)) i = 0;
	}

	/* add in displayable variables */
	pl->realpolys = i;
	if ((fpga_internaldisplay&TEXTDISPLAY) != 0)
		i += tech_displayableavars(ai, win, pl);
	return(i);
}

void fpga_shapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly)
{
	fpga_intshapearcpoly(ai, box, poly, &tech_oneprocpolyloop);
}

void fpga_intshapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly, POLYLOOP *pl)
{
	REGISTER INTBIG aindex;
	REGISTER TECH_ARCLAY *thista;

	/* handle displayable variables */
	if (box >= pl->realpolys)
	{
		(void)tech_filldisplayableavar(ai, poly, pl->curwindowpart, 0, pl);
		return;
	}

	/* initialize for the arc */
	aindex = ai->proto->arcindex;

	/* normal wires */
	thista = &fpga_arcprotos[aindex]->list[box];
	poly->layer = thista->lay;
	poly->desc = fpga_layers[poly->layer];

	/* simple wire arc */
	makearcpoly(ai->length, ai->width-thista->off*lambdaofarc(ai)/WHOLE,
		ai, poly, thista->style);
}

INTBIG fpga_allarcpolys(ARCINST *ai, POLYLIST *plist, WINDOWPART *win)
{
	REGISTER INTBIG tot, j;
	POLYLOOP mypl;

	mypl.curwindowpart = win;
	tot = fpga_intarcpolys(ai, win, &mypl);
	tot = mypl.realpolys;
	if (ensurepolylist(plist, tot, db_cluster)) return(-1);
	for(j = 0; j < tot; j++)
	{
		fpga_intshapearcpoly(ai, j, plist->polygons[j], &mypl);
	}
	return(tot);
}

/******************** TECHNOLOGY INTERFACE SUPPORT ********************/

BOOLEAN fpga_arcendactive(ARCINST *ai, INTBIG j)
{
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER ARCINST *oai;
	REGISTER NODEINST *ni, *oni, *subni;
	REGISTER PORTPROTO *pp, *opp;
	REGISTER INTBIG pindex, i, newend;
	REGISTER FPGANODE *fn;
	NODEINST **nilist;
	INTBIG depth, *indexlist;

	/* examine end */
	ni = ai->end[j].nodeinst;
	pi = ai->end[j].portarcinst;
	if (pi == NOPORTARCINST) return(FALSE);
	pp = pi->proto;
	pindex = ni->proto->primindex;
	if (pindex == 0)
	{
		/* follow down into cell */
		downhierarchy(ni, ni->proto, 0);
		subni = pp->subnodeinst;
		for(pi = subni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			oai = pi->conarcinst;
			if (oai->end[0].nodeinst == subni) newend = 1; else newend = 0;
			if (fpga_arcendactive(oai, newend)) break;
		}
		uphierarchy();
		if (pi != NOPORTARCINST) return(TRUE);
		return(FALSE);
	} else
	{
		/* primitive: see if it is one of ours */
		if (ni->proto->tech == fpga_tech && pindex > NODEPROTOCOUNT)
		{
			fn = fpga_nodes[pindex - NODEPROTOCOUNT - 1];
			downhierarchy(ni, ni->proto, 0);
			fpga_reevaluatepips(ni, fn);
			uphierarchy();
			if (fn->netcount != 0)
			{
				for(i = 0; i < fn->portcount; i++)
				{
					if (fn->portlist[i]->pp != pp) continue;
					if ((fn->netlist[fn->portlist[i]->con]->segactive&ACTIVEPART) != 0) return(TRUE);
					break;
				}
			}
		}
	}

	/* propagate */
	if (pp->network != NONETWORK)
	{
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			oai = pi->conarcinst;
			if (oai == ai) continue;
			if (pi->proto->network != pp->network) continue;
			if (oai->end[0].nodeinst == ni) newend = 1; else newend = 0;
			if (fpga_arcendactive(oai, newend)) return(TRUE);
		}

		gettraversalpath(ni->parent, NOWINDOWPART, &nilist, &indexlist, &depth, 1);
		if (depth > 0)
		{
			oni = nilist[depth-1];
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
			{
				opp = pe->exportproto;
				uphierarchy();

				for(pi = oni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				{
					oai = pi->conarcinst;
					if (pi->proto != opp) continue;
					if (oai->end[0].nodeinst == oni) newend = 1; else newend = 0;
					if (fpga_arcendactive(oai, newend)) break;
				}

				downhierarchy(oni, oni->proto, 0);
				if (pi != NOPORTARCINST) return(TRUE);
			}
		}
	}
	return(FALSE);
}

BOOLEAN fpga_arcactive(ARCINST *ai)
{
	REGISTER INTBIG i, size, cachedepth;
	REGISTER BOOLEAN value;
	INTBIG depth, *indexlist;
	REGISTER NODEINST *oni;
	REGISTER VARIABLE *var;
	NODEINST **nilist;
	UCHAR1 *ptr;

	if (ai->end[0].portarcinst == NOPORTARCINST) return(FALSE);

	/* see if there is a cache on the arc */
	gettraversalpath(ai->parent, NOWINDOWPART, &nilist, &indexlist, &depth, 0);
	var = getvalkey((INTBIG)ai, VARCINST, VCHAR|VISARRAY, fpga_arcactivecachekey);
	if (var != NOVARIABLE)
	{
		ptr = (UCHAR1 *)var->addr;
		cachedepth = ((INTBIG *)ptr)[0];   ptr += SIZEOFINTBIG;
		if (cachedepth == depth)
		{
			for(i=0; i<cachedepth; i++)
			{
				oni = ((NODEINST **)ptr)[0];   ptr += (sizeof (NODEINST *));
				if (oni != nilist[i]) break;
			}
			if (i >= cachedepth)
			{
				/* cache applies to this arc: get active factor */
				if (((INTSML *)ptr)[0] == 0) return(FALSE);
				return(TRUE);
			}
		}
	}

	/* compute arc activity */
	value = FALSE;
	if (fpga_arcendactive(ai, 0)) value = TRUE; else
		if (fpga_arcendactive(ai, 1)) value = TRUE;

	/* store the cache */
	size = depth * (sizeof (NODEINST *)) + SIZEOFINTBIG + SIZEOFINTSML;
	if (size > fpga_arcbufsize)
	{
		if (fpga_arcbufsize > 0) efree((CHAR *)fpga_arcbuf);
		fpga_arcbufsize = 0;
		fpga_arcbuf = (UCHAR1 *)emalloc(size, fpga_tech->cluster);
		if (fpga_arcbuf == 0) return(value);
		fpga_arcbufsize = size;
	}
	ptr = fpga_arcbuf;
	((INTBIG *)ptr)[0] = depth;   ptr += SIZEOFINTBIG;
	for(i=0; i<depth; i++)
	{
		((NODEINST **)ptr)[0] = nilist[i];   ptr += (sizeof (NODEINST *));
	}
	((INTSML *)ptr)[0] = value ? 1 : 0;
	nextchangequiet();
	setvalkey((INTBIG)ai, VARCINST, fpga_arcactivecachekey, (INTBIG)fpga_arcbuf,
		VCHAR|VISARRAY|(size<<VLENGTHSH)|VDONTSAVE);
	return(value);
}

/*
 * Routine to reevaluate primitive node "ni" (which is associated with internal
 * structure "fn").  Finds programming of pips and sets pip and net activity.
 */
void fpga_reevaluatepips(NODEINST *ni, FPGANODE *fn)
{
	REGISTER INTBIG i, value, size, cachedepth;
	INTBIG depth, *indexlist;
	REGISTER FPGAPIP *fpip;
	REGISTER NODEINST *oni;
	REGISTER VARIABLE *var;
	NODEINST **nilist;
	UCHAR1 *ptr;

	/* primitives with no pips or nets need no evaluation */
	if (fn->netcount == 0 && fn->pipcount == 0) return;

	/* see if there is a cache on the node */
	gettraversalpath(ni->parent, NOWINDOWPART, &nilist, &indexlist, &depth, 0);
	var = getvalkey((INTBIG)ni, VNODEINST, VCHAR|VISARRAY, fpga_nodepipcachekey);
	if (var != NOVARIABLE)
	{
		ptr = (UCHAR1 *)var->addr;
		cachedepth = ((INTBIG *)ptr)[0];   ptr += SIZEOFINTBIG;
		if (cachedepth == depth)
		{
			for(i=0; i<cachedepth; i++)
			{
				oni = ((NODEINST **)ptr)[0];   ptr += (sizeof (NODEINST *));
				if (oni != nilist[i]) break;
			}
			if (i >= cachedepth)
			{
				/* cache applies to this node: get values */
				for(i=0; i<fn->netcount; i++)
				{
					value = ((INTSML *)ptr)[0];   ptr += SIZEOFINTSML;
					if (value != 0) fn->netlist[i]->segactive |= ACTIVEPART; else
						fn->netlist[i]->segactive &= ~ACTIVEPART;
				}
				for(i=0; i<fn->pipcount; i++)
				{
					value = ((INTSML *)ptr)[0];   ptr += SIZEOFINTSML;
					if (value != 0) fn->piplist[i]->pipactive |= ACTIVEPART; else
						fn->piplist[i]->pipactive &= ~ACTIVEPART;
				}
				return;
			}
		}
	}

	/* reevaluate: presume all nets and pips are inactive */
	for(i=0; i<fn->netcount; i++) fn->netlist[i]->segactive &= ~ACTIVEPART;
	for(i=0; i<fn->pipcount; i++) fn->piplist[i]->pipactive &= ~ACTIVEPART;

	/* look for pip programming */
	fpga_fn = fn;
	fpga_findvariableobjects(ni, fpga_activepipskey, fpga_setpips);

	/* set nets active where they touch active pips */
	for(i=0; i<fn->pipcount; i++)
	{
		fpip = fn->piplist[i];
		if ((fpip->pipactive&ACTIVEPART) == 0) continue;
		if (fpip->con1 > 0) fn->netlist[fpip->con1]->segactive |= ACTIVEPART;
		if (fpip->con2 > 0) fn->netlist[fpip->con2]->segactive |= ACTIVEPART;
	}

	/* store the cache */
	size = depth * (sizeof (NODEINST *)) + SIZEOFINTBIG + fn->netcount * SIZEOFINTSML +
		fn->pipcount * SIZEOFINTSML;
	if (size > fpga_pipbufsize)
	{
		if (fpga_pipbufsize > 0) efree((CHAR *)fpga_pipbuf);
		fpga_pipbufsize = 0;
		fpga_pipbuf = (UCHAR1 *)emalloc(size, fpga_tech->cluster);
		if (fpga_pipbuf == 0) return;
		fpga_pipbufsize = size;
	}
	ptr = fpga_pipbuf;
	((INTBIG *)ptr)[0] = depth;   ptr += SIZEOFINTBIG;
	for(i=0; i<depth; i++)
	{
		((NODEINST **)ptr)[0] = nilist[i];   ptr += (sizeof (NODEINST *));
	}
	for(i=0; i<fn->netcount; i++)
	{
		if ((fn->netlist[i]->segactive&ACTIVEPART) != 0) ((INTSML *)ptr)[0] = 1; else
			((INTSML *)ptr)[0] = 0;
		ptr += SIZEOFINTSML;
	}
	for(i=0; i<fn->pipcount; i++)
	{
		if ((fn->piplist[i]->pipactive&ACTIVEPART) != 0) ((INTSML *)ptr)[0] = 1; else
			((INTSML *)ptr)[0] = 0;
		ptr += SIZEOFINTSML;
	}
	nextchangequiet();
	setvalkey((INTBIG)ni, VNODEINST, fpga_nodepipcachekey, (INTBIG)fpga_pipbuf,
		VCHAR|VISARRAY|(size<<VLENGTHSH)|VDONTSAVE);
}

/*
 * Helper routine for fpga_reevaluatepips() to set pip "name".
 */
void fpga_setpips(CHAR *name)
{
	REGISTER INTBIG i;

	for(i=0; i<fpga_fn->pipcount; i++)
		if (namesame(fpga_fn->piplist[i]->pipname, name) == 0)
	{
		fpga_fn->piplist[i]->pipactive |= ACTIVEPART;
		return;
	}
}

/*
 * Routine to examine primitive node "ni" and return true if the repeater is active.
 */
BOOLEAN fpga_repeateractive(NODEINST *ni)
{
	REGISTER VARIABLE *var;

	var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
	if (var == NOVARIABLE) return(FALSE);
	fpga_repeatername = (CHAR *)var->addr;
	fpga_repeaterisactive = FALSE;
	fpga_findvariableobjects(ni, fpga_activerepeaterskey, fpga_setrepeater);
	return(fpga_repeaterisactive);
}

/*
 * Helper routine for fpga_repeateractive() to determine whether repeater "name" is on.
 */
void fpga_setrepeater(CHAR *name)
{
	if (namesame(fpga_repeatername, name) == 0) fpga_repeaterisactive = TRUE;
}

/*
 * Routine to clear the cache of arc activity in the current cell.  If "ni" is NONODEINST,
 * clear all node caches as well, otherwise only clear the node cache on "ni".
 */
void fpga_clearcache(NODEINST *ni)
{
	REGISTER VARIABLE *var;
	REGISTER ARCINST *ai;
	REGISTER NODEPROTO *np;

	np = getcurcell();
	if (np == NONODEPROTO)
	{
		ttyputerr(_("Must edit a cell to clear its cache"));
		return;
	}
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		var = getvalkey((INTBIG)ai, VARCINST, VCHAR|VISARRAY, fpga_arcactivecachekey);
		if (var != NOVARIABLE)
			(void)delvalkey((INTBIG)ai, VARCINST, fpga_arcactivecachekey);
	}
	if (ni != NONODEINST)
	{
		var = getvalkey((INTBIG)ni, VNODEINST, VCHAR|VISARRAY, fpga_nodepipcachekey);
		if (var != NOVARIABLE)
			(void)delvalkey((INTBIG)ni, VNODEINST, fpga_nodepipcachekey);
	}
}

void fpga_findvariableobjects(NODEINST *ni, INTBIG varkey, void (*setit)(CHAR*))
{
	static NODEINST *mynilist[200];
	NODEINST **localnilist;
	REGISTER INTBIG curdepth, i, pathgood, depth;
	INTBIG localdepth, *indexlist;
	REGISTER CHAR *pt, *start, save1, save2, *dotpos;
	REGISTER VARIABLE *var;
	CHAR tempbuf[100];

	/* search hierarchical path */
	gettraversalpath(ni->parent, NOWINDOWPART, &localnilist, &indexlist, &localdepth, 0);
	depth = 0;
	for(i = localdepth - 1; i >= 0; i--)
	{
		ni = localnilist[i];
		mynilist[depth] = ni;
		depth++;
		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, varkey);
		if (var != NOVARIABLE)
		{
			pt = (CHAR *)var->addr;
			for(;;)
			{
				while (*pt == ' ' || *pt == '\t') pt++;
				start = pt;
				while (*pt != ' ' && *pt != '\t' && *pt != 0) pt++;
				save1 = *pt;
				*pt = 0;

				/* find pip name in "start" */
				pathgood = 1;
				for(curdepth = depth-2; curdepth >= 0; curdepth--)
				{
					if (*start == 0) { pathgood = 0;   break; }
					dotpos = start;
					while (*dotpos != '.' && *dotpos != 0) dotpos++;
					if (*dotpos != '.') break;

					save2 = *dotpos;   *dotpos = 0;
					estrcpy(tempbuf, start);
					*dotpos++ = save2;
					start = dotpos;

					/* make sure instance has the right name */
					var = getvalkey((INTBIG)mynilist[curdepth], VNODEINST, VSTRING, el_node_name_key);
					if (var == NOVARIABLE) { pathgood = 0;   break; }
					if (namesame((CHAR *)var->addr, tempbuf) != 0) { pathgood = 0;   break; }
				}
				if (pathgood != 0) setit(start);

				*pt = save1;
				if (*pt == 0) break;
			}
			return;
		}
	}
}

/*
 * Routine to fill polygon "poly" with a description of pip "pipindex" on node "ni"
 * which is a FPGA NODE "fn".
 */
void fpga_describepip(NODEINST *ni, FPGANODE *fn, INTBIG pipindex, POLYGON *poly)
{
	REGISTER INTBIG xc, yc, lambda;

	lambda = lambdaofnode(ni);
	if (poly->limit < 2) (void)extendpolygon(poly, 2);
	xc = (ni->lowx+ni->highx)/2;
	yc = (ni->lowy+ni->highy)/2;
	poly->xv[0] = fn->piplist[pipindex]->posx + xc - lambda;
	poly->yv[0] = fn->piplist[pipindex]->posy + yc - lambda;
	poly->xv[1] = fn->piplist[pipindex]->posx + xc + lambda;
	poly->yv[1] = fn->piplist[pipindex]->posy + yc + lambda;
	poly->count = 2;
	poly->style = FILLEDRECT;
	poly->layer = LPIP;
	poly->desc = fpga_layers[poly->layer];
}

/*
 * Routine to fill polygon "poly" with a description of network segment "whichseg"
 * on node "ni".  The network is in "fnet".
 */
void fpga_describenetseg(NODEINST *ni, FPGANET *fnet, INTBIG whichseg, POLYGON *poly)
{
	REGISTER INTBIG xc, yc;

	if (poly->limit < 2) (void)extendpolygon(poly, 2);
	xc = (ni->lowx+ni->highx)/2;
	yc = (ni->lowy+ni->highy)/2;
	poly->xv[0] = fnet->segfx[whichseg] + xc;
	poly->yv[0] = fnet->segfy[whichseg] + yc;
	poly->xv[1] = fnet->segtx[whichseg] + xc;
	poly->yv[1] = fnet->segty[whichseg] + yc;
	poly->count = 2;
	poly->style = OPENED;
	poly->layer = LWIRE;
}

/******************** ARCHITECTURE FILE READING ********************/

/*
 * Routine to read the FPGA file in "f" and create a LISPTREE structure which is returned.
 * Returns zero on error.
 */
LISPTREE *fpga_readfile(FILE *f, void *dia)
{
	CHAR line[MAXLINE];
	REGISTER CHAR save, *pt, *ptend;
	REGISTER INTBIG filepos;
	LISPTREE *treetop;

	/* make the tree top */
	treetop = (LISPTREE *)emalloc((sizeof (LISPTREE)), fpga_tech->cluster);
	if (treetop == 0) return(0);
	if (allocstring(&treetop->keyword, x_("TOP"), fpga_tech->cluster)) return(0);
	treetop->paramtotal = 0;
	treetop->parameters = 0;

	/* initialize current position and stack */
	fpga_treepos = treetop;
	fpga_treedepth = 0;
	fpga_lineno = 0;

	for(;;)
	{
		/* get the next line of text */
		if (xfgets(line, MAXLINE, f)) break;
		fpga_lineno++;
		if ((fpga_lineno%50) == 0)
		{
			filepos = xtell(f);
			DiaSetProgress(dia, filepos, fpga_filesize);
		}

		/* stop now if it is a comment */
		for(pt = line; *pt != 0; pt++) if (*pt != ' ' && *pt != '\t') break;
		if (*pt == '#') continue;

		/* keep parsing it */
		pt = line;
		for(;;)
		{
			/* skip spaces */
			while (*pt == ' ' || *pt == '\t') pt++;
			if (*pt == 0) break;

			/* check for special characters */
			if (*pt == ')')
			{
				save = pt[1];
				pt[1] = 0;
				if (fpga_pushkeyword(pt)) return(0);
				pt[1] = save;
				pt++;
				continue;
			}

			/* gather a keyword */
			ptend = pt;
			for(;;)
			{
				if (*ptend == ')' || *ptend == ' ' || *ptend == '\t' || *ptend == 0) break;
				if (*ptend == '"')
				{
					ptend++;
					for(;;)
					{
						if (*ptend == 0 || *ptend == '"') break;
						ptend++;
					}
					if (*ptend == '"') ptend++;
					break;
				}
				ptend++;
			}
			save = *ptend;   *ptend = 0;
			if (fpga_pushkeyword(pt)) return(0);
			*ptend = save;
			pt = ptend;
		}
	}

	if (fpga_treedepth != 0)
	{
		ttyputerr(_("Not enough close parenthesis in file"));
		return(0);
	}
	return(treetop);
}

/*
 * Routine to add the next keyword "keyword" to the lisp tree in the globals.
 * Returns true on error.
 */
BOOLEAN fpga_pushkeyword(CHAR *keyword)
{
	REGISTER LISPTREE *newtree;
	CHAR *savekey;
	REGISTER CHAR *pt;

	if (keyword[0] == '(')
	{
		if (fpga_treedepth >= MAXDEPTH)
		{
			ttyputerr(_("Nesting too deep (more than %d)"), MAXDEPTH);
			return(TRUE);
		}

		/* create a new tree branch */
		newtree = fpga_alloclisptree();
		newtree->parameters = 0;
		newtree->paramtotal = 0;
		newtree->lineno = fpga_lineno;

		/* add branch to previous branch */
		if (fpga_addparameter(fpga_treepos, PARAMBRANCH, newtree)) return(TRUE);

		/* add keyword */
		pt = &keyword[1];
		while (*pt == ' ' && *pt == '\t') pt++;
		if (allocstring(&newtree->keyword, pt, fpga_tech->cluster)) return(TRUE);

		/* push tree onto stack */
		fpga_treestack[fpga_treedepth] = fpga_treepos;
		fpga_treedepth++;
		fpga_treepos = newtree;
		return(FALSE);
	}

	if (estrcmp(keyword, x_(")")) == 0)
	{
		/* pop tree stack */
		if (fpga_treedepth <= 0)
		{
			ttyputerr(_("Too many close parenthesis"));
			return(TRUE);
		}
		fpga_treedepth--;
		fpga_treepos = fpga_treestack[fpga_treedepth];
		return(FALSE);
	}

	/* just add the atomic keyword */
	if (keyword[0] == '"' && keyword[estrlen(keyword)-1] == '"')
	{
		keyword++;
		keyword[estrlen(keyword)-1] = 0;
	}
	if (allocstring(&savekey, keyword, fpga_tech->cluster)) return(TRUE);
	if (fpga_addparameter(fpga_treepos, PARAMATOM, savekey)) return(TRUE);
	return(FALSE);
}

/*
 * Routine to add a parameter of type "type" and value "value" to the tree element "tree".
 * Returns true on memory error.
 */
BOOLEAN fpga_addparameter(LISPTREE *tree, INTBIG type, void *value)
{
	REGISTER INTBIG *newparamtypes;
	REGISTER INTBIG i, newlimit;
	REGISTER void **newparamvalues;

	if (tree->parameters >= tree->paramtotal)
	{
		newlimit = tree->paramtotal * 2;
		if (newlimit <= 0)
		{
			/* intelligent determination of parameter needs */
			if (namesame(tree->keyword, x_("port")) == 0) newlimit = 3; else
			if (namesame(tree->keyword, x_("name")) == 0) newlimit = 1; else
			if (namesame(tree->keyword, x_("position")) == 0) newlimit = 2; else
			if (namesame(tree->keyword, x_("direction")) == 0) newlimit = 1; else
			if (namesame(tree->keyword, x_("size")) == 0) newlimit = 2; else
			if (namesame(tree->keyword, x_("segment")) == 0) newlimit = 6; else
			if (namesame(tree->keyword, x_("pip")) == 0) newlimit = 3; else
			if (namesame(tree->keyword, x_("connectivity")) == 0) newlimit = 2; else
			if (namesame(tree->keyword, x_("repeater")) == 0) newlimit = 4; else
			if (namesame(tree->keyword, x_("porta")) == 0) newlimit = 2; else
			if (namesame(tree->keyword, x_("portb")) == 0) newlimit = 2; else
			if (namesame(tree->keyword, x_("type")) == 0) newlimit = 1; else
				newlimit = 3;
		}
		newparamtypes = (INTBIG *)emalloc(newlimit * SIZEOFINTBIG, fpga_tech->cluster);
		if (newparamtypes == 0) return(TRUE);
		newparamvalues = (void **)emalloc(newlimit * (sizeof (void *)), fpga_tech->cluster);
		if (newparamvalues == 0) return(TRUE);
		for(i=0; i<tree->parameters; i++)
		{
			newparamtypes[i] = tree->paramtype[i];
			newparamvalues[i] = tree->paramvalue[i];
		}
		if (tree->paramtotal > 0)
		{
			efree((CHAR *)tree->paramtype);
			efree((CHAR *)tree->paramvalue);
		}
		tree->paramtype = newparamtypes;
		tree->paramvalue = newparamvalues;
		tree->paramtotal = newlimit;
	}
	tree->paramtype[tree->parameters] = type;
	tree->paramvalue[tree->parameters] = value;
	tree->parameters++;
	return(FALSE);
}

LISPTREE *fpga_alloclisptree(void)
{
	LISPTREE *lt;

	if (fpga_freelisptree != 0)
	{
		lt = fpga_freelisptree;
		fpga_freelisptree = (LISPTREE *)lt->paramvalue;
	} else
	{
		lt = (LISPTREE *)emalloc(sizeof (LISPTREE), fpga_tech->cluster);
		if (lt == 0) return(0);
	}
	return(lt);
}

void fpga_killptree(LISPTREE *lt)
{
	REGISTER INTBIG i;

	if (lt->keyword != 0) efree(lt->keyword);
	for(i=0; i<lt->parameters; i++)
	{
		if (lt->paramtype[i] == PARAMBRANCH)
		{
			fpga_killptree((LISPTREE *)lt->paramvalue[i]);
		} else
		{
			efree((CHAR *)lt->paramvalue[i]);
		}
	}
	if (lt->parameters > 0)
	{
		efree((CHAR *)lt->paramtype);
		efree((CHAR *)lt->paramvalue);
	}

	/* put it back on the free list */
	lt->paramvalue = (void **)fpga_freelisptree;
	fpga_freelisptree = lt;
}

/******************** ARCHITECTURE PARSING: PRIMITIVES ********************/

/*
 * Routine to parse the entire tree and create primitives.
 * Returns the number of primitives made.
 */
INTBIG fpga_makeprimitives(LISPTREE *lt)
{
	REGISTER INTBIG i, total;
	REGISTER LISPTREE *sublt;

	/* look through top level for the "primdef"s */
	total = 0;
	for(i=0; i<lt->parameters; i++)
	{
		if (lt->paramtype[i] != PARAMBRANCH) continue;
		sublt = (LISPTREE *)lt->paramvalue[i];
		if (namesame(sublt->keyword, x_("primdef")) != 0) continue;

		/* create the primitive */
		if (fpga_makeprimitive(sublt)) return(0);
		total++;
	}
	return(total);
}

/*
 * Routine to create a primitive from a subtree "lt".
 * Tree has "(primdef...)" structure.
 */
BOOLEAN fpga_makeprimitive(LISPTREE *lt)
{
#ifdef USE_REDOPRIM
	REGISTER INTBIG i, j, k;
#else
	REGISTER INTBIG i, j, k, l;
#endif
	REGISTER INTBIG sizex, sizey, *newnodewidoff, lambda;
	REGISTER NODEPROTO *np, *primnp, *lastnp;
	REGISTER LISPTREE *scanlt, *ltattribute, *ltnets, *ltports, *ltcomponents;
	REGISTER VARIABLE *var;
	REGISTER FPGANODE *fn, **fnlist;
	REGISTER FPGANET *fnetblock;
	REGISTER FPGAPIP *fpipblock;
	REGISTER FPGAPORT *fpblock, *fp;
	REGISTER CHAR *primname, *primsizex, *primsizey;

	/* find all of the pieces of this primitive */
	ltattribute = ltnets = ltports = ltcomponents = 0;
	for(i=0; i<lt->parameters; i++)
	{
		if (lt->paramtype[i] != PARAMBRANCH) continue;
		scanlt = (LISPTREE *)lt->paramvalue[i];
		if (namesame(scanlt->keyword, x_("attributes")) == 0)
		{
			if (ltattribute != 0)
			{
				ttyputerr(_("Multiple 'attributes' sections for a primitive (line %ld)"), scanlt->lineno);
				return(TRUE);
			}
			ltattribute = scanlt;
			continue;
		}
		if (namesame(scanlt->keyword, x_("nets")) == 0)
		{
			if (ltnets != 0)
			{
				ttyputerr(_("Multiple 'nets' sections for a primitive (line %ld)"), scanlt->lineno);
				return(TRUE);
			}
			ltnets = scanlt;
			continue;
		}
		if (namesame(scanlt->keyword, x_("ports")) == 0)
		{
			if (ltports != 0)
			{
				ttyputerr(_("Multiple 'ports' sections for a primitive (line %ld)"), scanlt->lineno);
				return(TRUE);
			}
			ltports = scanlt;
			continue;
		}
		if (namesame(scanlt->keyword, x_("components")) == 0)
		{
			if (ltcomponents != 0)
			{
				ttyputerr(_("Multiple 'components' sections for a primitive (line %ld)"),
					scanlt->lineno);
				return(TRUE);
			}
			ltcomponents = scanlt;
			continue;
		}
	}

	/* scan the attributes section */
	if (ltattribute == 0)
	{
		ttyputerr(_("Missing 'attributes' sections on a primitive (line %ld)"), lt->lineno);
		return(TRUE);
	}
	primname = 0;
	primsizex = 0;
	primsizey = 0;
	for(j=0; j<ltattribute->parameters; j++)
	{
		if (ltattribute->paramtype[j] != PARAMBRANCH) continue;
		scanlt = (LISPTREE *)ltattribute->paramvalue[j];
		if (namesame(scanlt->keyword, x_("name")) == 0)
		{
			if (scanlt->parameters != 1 || scanlt->paramtype[0] != PARAMATOM)
			{
				ttyputerr(_("Primitive 'name' attribute should take a single atomic parameter (line %ld)"),
					scanlt->lineno);
				return(TRUE);
			}
			primname = (CHAR *)scanlt->paramvalue[0];
			continue;
		}
		if (namesame(scanlt->keyword, x_("size")) == 0)
		{
			if (scanlt->parameters != 2 || scanlt->paramtype[0] != PARAMATOM ||
				scanlt->paramtype[1] != PARAMATOM)
			{
				ttyputerr(_("Primitive 'size' attribute should take two atomic parameters (line %ld)"),
					scanlt->lineno);
				return(TRUE);
			}
			primsizex = (CHAR *)scanlt->paramvalue[0];
			primsizey = (CHAR *)scanlt->paramvalue[1];
			continue;
		}
	}

	/* make sure a name and size were given */
	if (primname == 0)
	{
		ttyputerr(_("Missing 'name' attribute in primitive definition (line %ld)"),
			ltattribute->lineno);
		return(TRUE);
	}
	if (primsizex == 0 || primsizey == 0)
	{
		ttyputerr(_("Missing 'size' attribute in primitive definition (line %ld)"),
			ltattribute->lineno);
		return(TRUE);
	}

	/* make the primitive */
	primnp = allocnodeproto(fpga_tech->cluster);
	if (primnp == NONODEPROTO) return(TRUE);
	if (allocstring(&primnp->protoname, primname, fpga_tech->cluster)) return(TRUE);
	lambda = el_curlib->lambda[fpga_tech->techindex];
	sizex = myatoi(primsizex) * lambda;
	sizey = myatoi(primsizey) * lambda;
	primnp->lowx = -sizex/2;   primnp->highx = primnp->lowx + sizex;
	primnp->lowy = -sizey/2;   primnp->highy = primnp->lowy + sizey;
	primnp->tech = fpga_tech;
	primnp->userbits |= LOCKEDPRIM;

	/* link it into this technology */
	lastnp = NONODEPROTO;
	for(np = fpga_tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		lastnp = np;
	lastnp->nextnodeproto = primnp;
	primnp->prevnodeproto = lastnp;
	primnp->primindex = lastnp->primindex + 1;

	/* extend the node width offset array */
	var = getval((INTBIG)fpga_tech, VTECHNOLOGY, VFRACT|VISARRAY, x_("TECH_node_width_offset"));
	if (var != NOVARIABLE)
	{
		newnodewidoff = (INTBIG *)emalloc(primnp->primindex * 4 * SIZEOFINTBIG, fpga_tech->cluster);
		if (newnodewidoff == 0) return(TRUE);
		for(j=0; j<(primnp->primindex-1)*4; j++)
			newnodewidoff[j] = ((INTBIG *)var->addr)[j];
		newnodewidoff[primnp->primindex*4-1] = 0;
		newnodewidoff[primnp->primindex*4-2] = 0;
		newnodewidoff[primnp->primindex*4-3] = 0;
		newnodewidoff[primnp->primindex*4-4] = 0;
		setval((INTBIG)fpga_tech, VTECHNOLOGY, x_("TECH_node_width_offset"), (INTBIG)newnodewidoff,
			VFRACT|VDONTSAVE|VISARRAY|((primnp->primindex*4)<<VLENGTHSH));
		efree((CHAR *)newnodewidoff);
	}

	/* add any unrecognized attributes */
	for(j=0; j<ltattribute->parameters; j++)
	{
		if (ltattribute->paramtype[j] != PARAMBRANCH) continue;
		scanlt = (LISPTREE *)ltattribute->paramvalue[j];
		if (namesame(scanlt->keyword, x_("name")) == 0) continue;
		if (namesame(scanlt->keyword, x_("size")) == 0) continue;

		if (scanlt->parameters != 1 || scanlt->paramtype[0] != PARAMATOM)
		{
			ttyputerr(_("Attribute '%s' attribute should take a single atomic parameter (line %ld)"),
				scanlt->keyword, scanlt->lineno);
			return(TRUE);
		}
		(void)setval((INTBIG)primnp, VNODEPROTO, scanlt->keyword, (INTBIG)scanlt->paramvalue[0],
			VSTRING);
	}

	/* create a local structure for this node */
	fn = (FPGANODE *)emalloc(sizeof (FPGANODE), fpga_tech->cluster);
	if (fn == 0) return(TRUE);
	fnlist = (FPGANODE **)emalloc((primnp->primindex - NODEPROTOCOUNT) * (sizeof (FPGANODE *)),
		fpga_tech->cluster);
	if (fnlist == 0) return(TRUE);
	for(j=0; j<primnp->primindex - NODEPROTOCOUNT - 1; j++)
		fnlist[j] = fpga_nodes[j];
	fnlist[primnp->primindex - NODEPROTOCOUNT - 1] = fn;
	if (fpga_nodecount > 0) efree((CHAR *)fpga_nodes);
	fpga_nodes = fnlist;
	fpga_nodecount++;

	/* get ports */
	fn->portcount = 0;
	if (ltports != 0)
	{
		/* count ports */
		for(j=0; j<ltports->parameters; j++)
		{
			if (ltports->paramtype[j] != PARAMBRANCH) continue;
			scanlt = (LISPTREE *)ltports->paramvalue[j];
			if (namesame(scanlt->keyword, x_("port")) == 0) fn->portcount++;
		}

		/* create local port structures */
		fn->portlist = (FPGAPORT **)emalloc(fn->portcount * (sizeof (FPGAPORT *)), fpga_tech->cluster);
		if (fn->portlist == 0) return(TRUE);
		fpblock = (FPGAPORT *)emalloc(fn->portcount * (sizeof (FPGAPORT)), fpga_tech->cluster);
		for(i=0; i<fn->portcount; i++)
		{
			fn->portlist[i] = fpblock;
			fn->portlist[i]->pp = NOPORTPROTO;
			fpblock++;
		}

		/* create the ports */
		for(j=i=0; j<ltports->parameters; j++)
		{
			if (ltports->paramtype[j] != PARAMBRANCH) continue;
			scanlt = (LISPTREE *)ltports->paramvalue[j];
			if (namesame(scanlt->keyword, x_("port")) == 0)
			{
				if (fpga_makeprimport(primnp, scanlt, fn->portlist[i])) return(TRUE);
				i++;
			}
		}
	}

	/* get nets */
	fn->netcount = 0;
	if (ltnets != 0)
	{
		/* count the nets */
		for(j=0; j<ltnets->parameters; j++)
		{
			if (ltnets->paramtype[j] != PARAMBRANCH) continue;
			scanlt = (LISPTREE *)ltnets->paramvalue[j];
			if (namesame(scanlt->keyword, x_("net")) == 0) fn->netcount++;
		}

		/* create local net structures */
		fn->netlist = (FPGANET **)emalloc(fn->netcount * (sizeof (FPGANET *)), fpga_tech->cluster);
		if (fn->netlist == 0) return(TRUE);
		fnetblock = (FPGANET *)emalloc(fn->netcount * (sizeof (FPGANET)), fpga_tech->cluster);
		if (fnetblock == 0) return(TRUE);
		for(i=0; i<fn->netcount; i++)
		{
			fn->netlist[i] = fnetblock;
			fn->netlist[i]->netname = 0;
			fn->netlist[i]->segcount = 0;
			fnetblock++;
		}

		/* create the nets */
		for(j=i=0; j<ltnets->parameters; j++)
		{
			if (ltnets->paramtype[j] != PARAMBRANCH) continue;
			scanlt = (LISPTREE *)ltnets->paramvalue[j];
			if (namesame(scanlt->keyword, x_("net")) == 0)
			{
				if (fpga_makeprimnet(primnp, scanlt, fn, fn->netlist[i])) return(TRUE);
				i++;
			}
		}
	}

	/* associate nets and ports */
	for(k=0; k<fn->portcount; k++)
	{
		fp = fn->portlist[k];
		for(i=0; i<fn->netcount; i++)
		{
			for(j=0; j<fn->netlist[i]->segcount; j++)
			{
				if ((fn->netlist[i]->segfx[j] == fp->posx && fn->netlist[i]->segfy[j] == fp->posy) ||
					(fn->netlist[i]->segtx[j] == fp->posx && fn->netlist[i]->segty[j] == fp->posy))
				{
					fp->con = i;
					break;
				}
			}
			if (j < fn->netlist[i]->segcount) break;
		}
	}

	/* set electrical connectivity */
#ifdef USE_REDOPRIM
	for(k=0; k<fn->portcount; k++)
	{
		if (fn->portlist[k]->con < 0)
		{
			fn->portlist[k]->pp->userbits |= PORTNET;
		} else
		{
			if (fn->portlist[k]->con >= (PORTNET >> PORTNETSH))
			{
				ttyputerr(_("Too many networks in FPGA primitive"));
			}
			fn->portlist[k]->pp->userbits |= (fn->portlist[k]->con >> PORTNETSH);
		}
	}
#else
	for(k=0; k<fn->portcount; k++)
	{
		if (fn->portlist[k]->con < 0) continue;
		for(l=k+1; l<fn->portcount; l++)
		{
			if (fn->portlist[k]->con != fn->portlist[l]->con) continue;
			if (fn->portlist[l]->pp->network != fn->portlist[k]->pp->network)
			{
				efree((CHAR *)fn->portlist[l]->pp->network);
				fn->portlist[l]->pp->network = fn->portlist[k]->pp->network;
			}
		}
	}
#endif

	/* get pips */
	fn->pipcount = 0;
	if (ltcomponents != 0)
	{
		/* count the pips */
		for(j=0; j<ltcomponents->parameters; j++)
		{
			if (ltcomponents->paramtype[j] != PARAMBRANCH) continue;
			scanlt = (LISPTREE *)ltcomponents->paramvalue[j];
			if (namesame(scanlt->keyword, x_("pip")) == 0) fn->pipcount++;
		}

		/* create local pips structures */
		fn->piplist = (FPGAPIP **)emalloc(fn->pipcount * (sizeof (FPGANET *)), fpga_tech->cluster);
		if (fn->piplist == 0) return(TRUE);
		fpipblock = (FPGAPIP *)emalloc(fn->pipcount * (sizeof (FPGANET)), fpga_tech->cluster);
		if (fpipblock == 0) return(TRUE);
		for(i=0; i<fn->pipcount; i++)
		{
			fn->piplist[i] = fpipblock;
			fpipblock++;
		}

		/* create the pips */
		for(j=i=0; j<ltcomponents->parameters; j++)
		{
			if (ltcomponents->paramtype[j] != PARAMBRANCH) continue;
			scanlt = (LISPTREE *)ltcomponents->paramvalue[j];
			if (namesame(scanlt->keyword, x_("pip")) == 0)
			{
				if (fpga_makeprimpip(primnp, scanlt, fn, fn->piplist[i])) return(TRUE);
				i++;
			}
		}
	}
	return(FALSE);
}

/*
 * Routine to add a port to primitive "np" from the tree in "lt" and
 * store information about it in the local structure "fp".
 * Tree has "(port...)" structure.  Returns true on error.
 */
BOOLEAN fpga_makeprimport(NODEPROTO *np, LISPTREE *lt, FPGAPORT *fp)
{
	REGISTER INTBIG j;
	REGISTER INTBIG lambda;
	PORTPROTO *pp, *newpp, *lastpp;
	REGISTER LISPTREE *ltname, *ltposition, *ltdirection, *scanlt;

	/* look for keywords */
	ltname = ltposition = ltdirection = 0;
	for(j=0; j<lt->parameters; j++)
	{
		if (lt->paramtype[j] != PARAMBRANCH) continue;
		scanlt = (LISPTREE *)lt->paramvalue[j];
		if (namesame(scanlt->keyword, x_("name")) == 0)
		{
			ltname = scanlt;
			continue;
		}
		if (namesame(scanlt->keyword, x_("position")) == 0)
		{
			ltposition = scanlt;
			continue;
		}
		if (namesame(scanlt->keyword, x_("direction")) == 0)
		{
			ltdirection = scanlt;
			continue;
		}
	}

	/* validate */
	if (ltname == 0)
	{
		ttyputerr(_("Port has no name (line %ld)"), lt->lineno);
		return(TRUE);
	}
	if (ltname->parameters != 1 || ltname->paramtype[0] != PARAMATOM)
	{
		ttyputerr(_("Port name must be a single atom (line %ld)"), ltname->lineno);
		return(TRUE);
	}
	if (ltposition == 0)
	{
		ttyputerr(_("Port has no position (line %ld)"), lt->lineno);
		return(TRUE);
	}
	if (ltposition->parameters != 2 || ltposition->paramtype[0] != PARAMATOM ||
		ltposition->paramtype[1] != PARAMATOM)
	{
		ttyputerr(_("Port position must be two atoms (line %ld)"), ltposition->lineno);
		return(TRUE);
	}
	pp = getportproto(np, (CHAR *)ltname->paramvalue[0]);
	if (pp != NOPORTPROTO)
	{
		ttyputerr(_("Duplicate port name (line %ld)"), lt->lineno);
		return(TRUE);
	}

	/* create the portproto */
	newpp = allocportproto(fpga_tech->cluster);
	if (newpp == NOPORTPROTO) return(TRUE);
	newpp->parent = np;
	if (allocstring(&newpp->protoname, (CHAR *)ltname->paramvalue[0], fpga_tech->cluster)) return(TRUE);
	TDCLEAR(newpp->textdescript);
	newpp->userbits = (180 << PORTARANGESH);
	newpp->connects = fpga_wirepinprim->firstportproto->connects;

#ifndef USE_REDOPRIM
	/* make a unique network for the port */
	newpp->network = (NETWORK *)emalloc(sizeof (NETWORK), fpga_tech->cluster);
	if (newpp->network == 0) return(TRUE);
	newpp->network->namecount = 0;
	newpp->network->buswidth = 1;
	newpp->network->nextnetwork  = NONETWORK;
	newpp->network->prevnetwork = NONETWORK;
	newpp->network->networklist = (NETWORK **)NONETWORK;
	newpp->network->parent = NONODEPROTO;
	newpp->network->netname = NOSTRING;
	newpp->network->arccount = 0;
	newpp->network->refcount = 0;
	newpp->network->portcount = 0;
	newpp->network->buslinkcount = 0;
	newpp->network->numvar = 0;
#endif

	/* add in directionality */
	if (ltdirection != 0)
	{
		if (ltdirection->parameters != 1 || ltdirection->paramtype[0] != PARAMATOM)
		{
			ttyputerr(_("Port direction must be a single atom (line %ld)"), ltdirection->lineno);
			return(TRUE);
		}
		if (namesame((CHAR *)ltdirection->paramvalue[0], x_("input")) == 0) newpp->userbits |= INPORT; else
			if (namesame((CHAR *)ltdirection->paramvalue[0], x_("output")) == 0) newpp->userbits |= OUTPORT; else
				if (namesame((CHAR *)ltdirection->paramvalue[0], x_("bidir")) == 0) newpp->userbits |= BIDIRPORT; else
		{
			ttyputerr(_("Unknown port direction (line %ld)"), ltdirection->lineno);
			return(TRUE);
		}
	}

	/* link it into the primitive */
	lastpp = NOPORTPROTO;
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		lastpp = pp;
	if (lastpp == NOPORTPROTO) np->firstportproto = newpp; else
		lastpp->nextportproto = newpp;

	/* set unique topology for this port */
	if (lastpp == NOPORTPROTO) j = 0; else
		j = ((lastpp->userbits & PORTNET) >> PORTNETSH) + 1;
	newpp->userbits |= (j << PORTNETSH);

	/* add it to the local port structure */
	lambda = el_curlib->lambda[fpga_tech->techindex];
	fp->posx = myatoi((CHAR *)ltposition->paramvalue[0]) * lambda + np->lowx;
	fp->posy = myatoi((CHAR *)ltposition->paramvalue[1]) * lambda + np->lowy;
	fp->pp = newpp;
	fp->con = -1;
	return(FALSE);
}

/*
 * Routine to add a net to primitive "np" from the tree in "lt" and store information
 * about it in the local object "fnet".
 * Tree has "(net...)" structure.  Returns true on error.
 */
BOOLEAN fpga_makeprimnet(NODEPROTO *np, LISPTREE *lt, FPGANODE *fn, FPGANET *fnet)
{
	REGISTER INTBIG i, j, k, pos;
	REGISTER INTBIG *newsegfx, *newsegfy, *newsegtx, *newsegty, *newsegcoords, lambda;
	REGISTER LISPTREE *scanlt;
	INTBIG sx[2], sy[2];

	/* scan for information in the tree */
	fnet->netname = 0;
	fnet->segcount = 0;
	for(j=0; j<lt->parameters; j++)
	{
		if (lt->paramtype[j] != PARAMBRANCH) continue;
		scanlt = (LISPTREE *)lt->paramvalue[j];
		if (namesame(scanlt->keyword, x_("name")) == 0 && scanlt->parameters == 1 &&
			scanlt->paramtype[0] == PARAMATOM)
		{
			if (fnet->netname != 0)
			{
				ttyputerr(_("Multiple names for network (line %ld)"), lt->lineno);
				return(TRUE);
			}
			if (allocstring(&fnet->netname, (CHAR *)scanlt->paramvalue[0], fpga_tech->cluster)) return(TRUE);
			continue;
		}
		if (namesame(scanlt->keyword, x_("segment")) == 0)
		{
			pos = 0;
			for(i=0; i<2; i++)
			{
				/* get end of net segment */
				if (scanlt->parameters < pos+1)
				{
					ttyputerr(_("Incomplete block net segment (line %ld)"), scanlt->lineno);
					return(TRUE);
				}
				if (scanlt->paramtype[pos] != PARAMATOM)
				{
					ttyputerr(_("Must have atoms in block net segment (line %ld)"), scanlt->lineno);
					return(TRUE);
				}
				if (namesame((CHAR *)scanlt->paramvalue[pos], x_("coord")) == 0)
				{
					if (scanlt->parameters < pos+3)
					{
						ttyputerr(_("Incomplete block net segment (line %ld)"), scanlt->lineno);
						return(TRUE);
					}
					if (scanlt->paramtype[pos+1] != PARAMATOM || scanlt->paramtype[pos+2] != PARAMATOM)
					{
						ttyputerr(_("Must have atoms in block net segment (line %ld)"), scanlt->lineno);
						return(TRUE);
					}
					lambda = el_curlib->lambda[fpga_tech->techindex];
					sx[i] = myatoi((CHAR *)scanlt->paramvalue[pos+1]) * lambda + np->lowx;
					sy[i] = myatoi((CHAR *)scanlt->paramvalue[pos+2]) * lambda + np->lowy;
					pos += 3;
				} else if (namesame((CHAR *)scanlt->paramvalue[pos], x_("port")) == 0)
				{
					if (scanlt->parameters < pos+2)
					{
						ttyputerr(_("Incomplete block net segment (line %ld)"), scanlt->lineno);
						return(TRUE);
					}
					if (scanlt->paramtype[pos+1] != PARAMATOM)
					{
						ttyputerr(_("Must have atoms in block net segment (line %ld)"), scanlt->lineno);
						return(TRUE);
					}

					/* find port */
					for(k=0; k<fn->portcount; k++)
					{
						if (namesame(fn->portlist[k]->pp->protoname, (CHAR *)scanlt->paramvalue[pos+1]) == 0)
							break;
					}
					if (k >= fn->portcount)
					{
						ttyputerr(_("Unknown port on primitive net segment (line %ld)"), scanlt->lineno);
						return(TRUE);
					}
					sx[i] = fn->portlist[k]->posx;
					sy[i] = fn->portlist[k]->posy;
					pos += 2;
				} else
				{
					ttyputerr(_("Unknown keyword '%s' in block net segment (line %ld)"),
						(CHAR *)scanlt->paramvalue[pos], scanlt->lineno);
					return(TRUE);
				}
			}

			newsegcoords = (INTBIG *)emalloc((fnet->segcount+1) * 4 * SIZEOFINTBIG, fpga_tech->cluster);
			if (newsegcoords == 0) return(TRUE);
			newsegfx = newsegcoords;
			newsegfy = newsegfx;   newsegfy += fnet->segcount+1;
			newsegtx = newsegfy;   newsegtx += fnet->segcount+1;
			newsegty = newsegtx;   newsegty += fnet->segcount+1;
			for(i=0; i<fnet->segcount; i++)
			{
				newsegfx[i] = fnet->segfx[i];
				newsegfy[i] = fnet->segfy[i];
				newsegtx[i] = fnet->segtx[i];
				newsegty[i] = fnet->segty[i];
			}
			newsegfx[fnet->segcount] = sx[0];
			newsegfy[fnet->segcount] = sy[0];
			newsegtx[fnet->segcount] = sx[1];
			newsegty[fnet->segcount] = sy[1];
			if (fnet->segcount > 0)
			{
				efree((CHAR *)fnet->segfx);
				efree((CHAR *)fnet->segfy);
				efree((CHAR *)fnet->segtx);
				efree((CHAR *)fnet->segty);
			}
			fnet->segfx = newsegfx;   fnet->segfy = newsegfy;
			fnet->segtx = newsegtx;   fnet->segty = newsegty;
			fnet->segcount++;
			continue;
		}
	}
	return(FALSE);
}

/*
 * Routine to add a pip to primitive "np" from the tree in "lt" and save
 * information about it in the local object "fpip".
 * Tree has "(pip...)" structure.  Returns true on error.
 */
BOOLEAN fpga_makeprimpip(NODEPROTO *np, LISPTREE *lt, FPGANODE *fn, FPGAPIP *fpip)
{
	REGISTER INTBIG i, j;
	REGISTER INTBIG lambda;
	REGISTER LISPTREE *scanlt;

	/* scan for information in this FPGAPIP object */
	fpip->pipname = 0;
	fpip->con1 = fpip->con2 = -1;
	for(j=0; j<lt->parameters; j++)
	{
		if (lt->paramtype[j] != PARAMBRANCH) continue;
		scanlt = (LISPTREE *)lt->paramvalue[j];
		if (namesame(scanlt->keyword, x_("name")) == 0 && scanlt->parameters == 1 &&
			scanlt->paramtype[0] == PARAMATOM)
		{
			if (fpip->pipname != 0)
			{
				ttyputerr(_("Multiple names for pip (line %ld)"), lt->lineno);
				return(TRUE);
			}
			if (allocstring(&fpip->pipname, (CHAR *)scanlt->paramvalue[0], fpga_tech->cluster)) return(TRUE);
			continue;
		}
		if (namesame(scanlt->keyword, x_("position")) == 0 && scanlt->parameters == 2 &&
			scanlt->paramtype[0] == PARAMATOM && scanlt->paramtype[1] == PARAMATOM)
		{
			lambda = el_curlib->lambda[fpga_tech->techindex];
			fpip->posx = myatoi((CHAR *)scanlt->paramvalue[0]) * lambda + np->lowx;
			fpip->posy = myatoi((CHAR *)scanlt->paramvalue[1]) * lambda + np->lowy;
			continue;
		}
		if (namesame(scanlt->keyword, x_("connectivity")) == 0 && scanlt->parameters == 2 &&
			scanlt->paramtype[0] == PARAMATOM && scanlt->paramtype[1] == PARAMATOM)
		{
			for(i=0; i<fn->netcount; i++)
			{
				if (namesame(fn->netlist[i]->netname, (CHAR *)scanlt->paramvalue[0]) == 0)
					fpip->con1 = i;
				if (namesame(fn->netlist[i]->netname, (CHAR *)scanlt->paramvalue[1]) == 0)
					fpip->con2 = i;
			}
			continue;
		}
	}
	return(FALSE);
}

/******************** ARCHITECTURE PARSING: LAYOUT ********************/

/*
 * Routine to scan the entire tree for block definitions and create them.
 */
NODEPROTO *fpga_placeprimitives(LISPTREE *lt)
{
	REGISTER INTBIG i;
	REGISTER LISPTREE *sublt;
	REGISTER NODEPROTO *np, *toplevel;

	/* look through top level for the "blockdef"s */
	toplevel = NONODEPROTO;
	for(i=0; i<lt->parameters; i++)
	{
		if (lt->paramtype[i] != PARAMBRANCH) continue;
		sublt = (LISPTREE *)lt->paramvalue[i];
		if (namesame(sublt->keyword, x_("blockdef")) != 0 &&
			namesame(sublt->keyword, x_("architecture")) != 0) continue;

		/* create the primitive */
		np = fpga_makecell(sublt);
		if (np == NONODEPROTO) return(NONODEPROTO);
		if (namesame(sublt->keyword, x_("architecture")) == 0) toplevel = np;
	}
	return(toplevel);
}

/*
 * Routine to create a cell from a subtree "lt".
 * Tree has "(blockdef...)" or "(architecture...)" structure.
 * Returns nonzero on error.
 */
NODEPROTO *fpga_makecell(LISPTREE *lt)
{
	REGISTER INTBIG i, j;
	REGISTER BOOLEAN gotsize;
	REGISTER INTBIG sizex, sizey, lambda;
	REGISTER NODEPROTO *cell;
	REGISTER LISPTREE *scanlt, *ltattribute, *ltnets, *ltports, *ltcomponents;
	REGISTER CHAR *blockname;

	/* find all of the pieces of this block */
	lambda = el_curlib->lambda[fpga_tech->techindex];
	ltattribute = ltnets = ltports = ltcomponents = 0;
	for(i=0; i<lt->parameters; i++)
	{
		if (lt->paramtype[i] != PARAMBRANCH) continue;
		scanlt = (LISPTREE *)lt->paramvalue[i];
		if (namesame(scanlt->keyword, x_("attributes")) == 0)
		{
			if (ltattribute != 0)
			{
				ttyputerr(_("Multiple 'attributes' sections for a block (line %ld)"), lt->lineno);
				return(NONODEPROTO);
			}
			ltattribute = scanlt;
			continue;
		}
		if (namesame(scanlt->keyword, x_("nets")) == 0)
		{
			if (ltnets != 0)
			{
				ttyputerr(_("Multiple 'nets' sections for a block (line %ld)"), lt->lineno);
				return(NONODEPROTO);
			}
			ltnets = scanlt;
			continue;
		}
		if (namesame(scanlt->keyword, x_("ports")) == 0)
		{
			if (ltports != 0)
			{
				ttyputerr(_("Multiple 'ports' sections for a block (line %ld)"), lt->lineno);
				return(NONODEPROTO);
			}
			ltports = scanlt;
			continue;
		}
		if (namesame(scanlt->keyword, x_("components")) == 0)
		{
			if (ltcomponents != 0)
			{
				ttyputerr(_("Multiple 'components' sections for a block (line %ld)"), lt->lineno);
				return(NONODEPROTO);
			}
			ltcomponents = scanlt;
			continue;
		}
	}

	/* scan the attributes section */
	if (ltattribute == 0)
	{
		ttyputerr(_("Missing 'attributes' sections on a block (line %ld)"), lt->lineno);
		return(NONODEPROTO);
	}
	blockname = 0;
	gotsize = FALSE;
	sizex = sizey = 0;
	for(j=0; j<ltattribute->parameters; j++)
	{
		if (ltattribute->paramtype[j] != PARAMBRANCH) continue;
		scanlt = (LISPTREE *)ltattribute->paramvalue[j];
		if (namesame(scanlt->keyword, x_("name")) == 0)
		{
			if (scanlt->parameters != 1 || scanlt->paramtype[0] != PARAMATOM)
			{
				ttyputerr(_("Block 'name' attribute should take a single atomic parameter (line %ld)"),
					scanlt->lineno);
				return(NONODEPROTO);
			}
			blockname = (CHAR *)scanlt->paramvalue[0];
			continue;
		}
		if (namesame(scanlt->keyword, x_("size")) == 0 && scanlt->parameters == 2 &&
			scanlt->paramtype[0] == PARAMATOM && scanlt->paramtype[1] == PARAMATOM)
		{
			gotsize = TRUE;
			sizex = myatoi((CHAR *)scanlt->paramvalue[0]) * lambda;
			sizey = myatoi((CHAR *)scanlt->paramvalue[1]) * lambda;
			continue;
		}
	}

	/* validate */
	if (blockname == 0)
	{
		ttyputerr(_("Missing 'name' attribute in block definition (line %ld)"),
			ltattribute->lineno);
		return(NONODEPROTO);
	}

	/* make the cell */
	cell = us_newnodeproto(blockname, el_curlib);
	if (cell == NONODEPROTO) return(NONODEPROTO);
	ttyputmsg(_("Creating cell '%s'"), blockname);

	/* force size by placing pins in the corners */
	if (gotsize)
	{
		newnodeinst(fpga_wirepinprim, 0, lambda, 0, lambda, 0, 0, cell);
		newnodeinst(fpga_wirepinprim, sizex-lambda, sizex, 0, lambda, 0, 0, cell);
		newnodeinst(fpga_wirepinprim, 0, lambda, sizey-lambda, sizey, 0, 0, cell);
		newnodeinst(fpga_wirepinprim, sizex-lambda, sizex, sizey-lambda, sizey, 0, 0, cell);
	}

	/* add any unrecognized attributes */
	for(j=0; j<ltattribute->parameters; j++)
	{
		if (ltattribute->paramtype[j] != PARAMBRANCH) continue;
		scanlt = (LISPTREE *)ltattribute->paramvalue[j];
		if (namesame(scanlt->keyword, x_("name")) == 0) continue;
		if (namesame(scanlt->keyword, x_("size")) == 0) continue;

		if (scanlt->parameters != 1 || scanlt->paramtype[0] != PARAMATOM)
		{
			ttyputerr(_("Attribute '%s' attribute should take a single atomic parameter (line %ld)"),
				scanlt->keyword, scanlt->lineno);
			return(NONODEPROTO);
		}
		(void)setval((INTBIG)cell, VNODEPROTO, scanlt->keyword, (INTBIG)scanlt->paramvalue[0],
			VSTRING);
	}

	/* place block components */
	if (ltcomponents != 0)
	{
		for(j=0; j<ltcomponents->parameters; j++)
		{
			if (ltcomponents->paramtype[j] != PARAMBRANCH) continue;
			scanlt = (LISPTREE *)ltcomponents->paramvalue[j];
			if (namesame(scanlt->keyword, x_("repeater")) == 0)
			{
				if (fpga_makeblockrepeater(cell, scanlt)) return(NONODEPROTO);
				continue;
			}
			if (namesame(scanlt->keyword, x_("instance")) == 0)
			{
				if (fpga_makeblockinstance(cell, scanlt)) return(NONODEPROTO);
				continue;
			}
		}
	}

	/* place block ports */
	if (ltports != 0)
	{
		for(j=0; j<ltports->parameters; j++)
		{
			if (ltports->paramtype[j] != PARAMBRANCH) continue;
			scanlt = (LISPTREE *)ltports->paramvalue[j];
			if (namesame(scanlt->keyword, x_("port")) == 0)
			{
				if (fpga_makeblockport(cell, scanlt)) return(NONODEPROTO);
			}
		}
	}

	/* place block nets */
	if (ltnets != 0)
	{
		/* read the block nets */
		for(j=0; j<ltnets->parameters; j++)
		{
			if (ltnets->paramtype[j] != PARAMBRANCH) continue;
			scanlt = (LISPTREE *)ltnets->paramvalue[j];
			if (namesame(scanlt->keyword, x_("net")) == 0)
			{
				if (fpga_makeblocknet(cell, scanlt)) return(NONODEPROTO);
				i++;
			}
		}
	}
	return(cell);
}

/*
 * Routine to place an instance in cell "cell" from the LISPTREE in "lt".
 * Tree has "(instance...)" structure.  Returns true on error.
 */
BOOLEAN fpga_makeblockinstance(NODEPROTO *cell, LISPTREE *lt)
{
	REGISTER INTBIG i, rotation;
	REGISTER INTBIG posx, posy, yoff, lambda;
	REGISTER LISPTREE *scanlt, *lttype, *ltname, *ltposition, *ltrotation, *ltattribute;
	REGISTER NODEPROTO *np;
	REGISTER VARIABLE *var;
	REGISTER NODEINST *ni;

	/* scan for information in this block instance object */
	lttype = ltname = ltposition = ltrotation = ltattribute = 0;
	for(i=0; i<lt->parameters; i++)
	{
		if (lt->paramtype[i] != PARAMBRANCH) continue;
		scanlt = (LISPTREE *)lt->paramvalue[i];
		if (namesame(scanlt->keyword, x_("type")) == 0)
		{
			if (lttype != 0)
			{
				ttyputerr(_("Multiple 'type' sections for a block (line %ld)"), lt->lineno);
				return(TRUE);
			}
			lttype = scanlt;
			continue;
		}
		if (namesame(scanlt->keyword, x_("name")) == 0)
		{
			if (ltname != 0)
			{
				ttyputerr(_("Multiple 'name' sections for a block (line %ld)"), lt->lineno);
				return(TRUE);
			}
			ltname = scanlt;
			continue;
		}
		if (namesame(scanlt->keyword, x_("position")) == 0)
		{
			if (ltposition != 0)
			{
				ttyputerr(_("Multiple 'position' sections for a block (line %ld)"), lt->lineno);
				return(TRUE);
			}
			ltposition = scanlt;
			continue;
		}
		if (namesame(scanlt->keyword, x_("rotation")) == 0)
		{
			if (ltrotation != 0)
			{
				ttyputerr(_("Multiple 'rotation' sections for a block (line %ld)"), lt->lineno);
				return(TRUE);
			}
			ltrotation = scanlt;
			continue;
		}
		if (namesame(scanlt->keyword, x_("attributes")) == 0)
		{
			if (ltattribute != 0)
			{
				ttyputerr(_("Multiple 'attributes' sections for a block (line %ld)"), lt->lineno);
				return(TRUE);
			}
			ltattribute = scanlt;
			continue;
		}
	}

	/* validate */
	if (lttype == 0)
	{
		ttyputerr(_("No 'type' specified for block instance (line %ld)"), lt->lineno);
		return(TRUE);
	}
	if (lttype->parameters != 1 || lttype->paramtype[0] != PARAMATOM)
	{
		ttyputerr(_("Need one atom in 'type' of block instance (line %ld)"), lttype->lineno);
		return(TRUE);
	}
	for(np = fpga_tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		if (namesame(np->protoname, (CHAR *)lttype->paramvalue[0]) == 0) break;
	if (np == NONODEPROTO)
		np = getnodeproto((CHAR *)lttype->paramvalue[0]);
	if (np == NONODEPROTO)
	{
		ttyputerr(_("Cannot find block type '%s' (line %ld)"), (CHAR *)lttype->paramvalue[0], lttype->lineno);
		return(TRUE);
	}
	if (ltposition == 0)
	{
		ttyputerr(_("No 'position' specified for block instance (line %ld)"), lt->lineno);
		return(TRUE);
	}
	if (ltposition->parameters != 2 || ltposition->paramtype[0] != PARAMATOM ||
		ltposition->paramtype[1] != PARAMATOM)
	{
		ttyputerr(_("Need two atoms in 'position' of block instance (line %ld)"), ltposition->lineno);
		return(TRUE);
	}
	rotation = 0;
	if (ltrotation != 0)
	{
		if (ltrotation->parameters != 1 || ltrotation->paramtype[0] != PARAMATOM)
		{
			ttyputerr(_("Need one atom in 'rotation' of block instance (line %ld)"), ltrotation->lineno);
			return(TRUE);
		}
		rotation = myatoi((CHAR *)ltrotation->paramvalue[0]) * 10;
	}

	/* place the instance */
	lambda = el_curlib->lambda[fpga_tech->techindex];
	posx = myatoi((CHAR *)ltposition->paramvalue[0]) * lambda;
	posy = myatoi((CHAR *)ltposition->paramvalue[1]) * lambda;
	ni = newnodeinst(np, posx, posx+np->highx-np->lowx, posy, posy+np->highy-np->lowy, 0, rotation, cell);
	if (ni == NONODEINST) return(TRUE);

	/* add any attributes */
	if (ltattribute != 0)
	{
		for(i=0; i<ltattribute->parameters; i++)
		{
			if (ltattribute->paramtype[i] != PARAMBRANCH) continue;
			scanlt = (LISPTREE *)ltattribute->paramvalue[i];
			if (scanlt->parameters != 1 || scanlt->paramtype[0] != PARAMATOM)
			{
				ttyputerr(_("Attribute '%s' attribute should take a single atomic parameter (line %ld)"),
					scanlt->keyword, scanlt->lineno);
				return(TRUE);
			}
			(void)setval((INTBIG)ni, VNODEINST, scanlt->keyword, (INTBIG)scanlt->paramvalue[0],
				VSTRING);
		}
	}

	/* name the instance if one is given */
	if (ltname != 0)
	{
		if (ltname->parameters != 1 || ltname->paramtype[0] != PARAMATOM)
		{
			ttyputerr(_("Need one atom in 'name' of block instance (line %ld)"), ltname->lineno);
			return(TRUE);
		}
		var = setvalkey((INTBIG)ni, VNODEINST, el_node_name_key, (INTBIG)ltname->paramvalue[0], VSTRING|VDISPLAY);
		if (var != NOVARIABLE)
		{
			yoff = (ni->highy-ni->lowy) / lambda;
			defaulttextsize(3, var->textdescript);
			TDSETPOS(var->textdescript, VTPOSBOXED);
			TDSETOFF(var->textdescript, TDGETXOFF(var->textdescript), yoff);
		}
	}
	return(FALSE);
}

/*
 * Routine to add a port to block "cell" from the tree in "lt".
 * Tree has "(port...)" structure.  Returns true on error.
 */
BOOLEAN fpga_makeblockport(NODEPROTO *cell, LISPTREE *lt)
{
	REGISTER INTBIG j;
	REGISTER INTBIG posx, posy, lambda;
	PORTPROTO *pp, *expp;
	REGISTER NODEINST *ni;
	REGISTER LISPTREE *ltname, *ltposition, *scanlt;

	ltname = ltposition = 0;
	for(j=0; j<lt->parameters; j++)
	{
		if (lt->paramtype[j] != PARAMBRANCH) continue;
		scanlt = (LISPTREE *)lt->paramvalue[j];
		if (namesame(scanlt->keyword, x_("name")) == 0)
		{
			ltname = scanlt;
			continue;
		}
		if (namesame(scanlt->keyword, x_("position")) == 0)
		{
			ltposition = scanlt;
			continue;
		}
		if (namesame(scanlt->keyword, x_("direction")) == 0)
		{
			/* ltdirection = scanlt; */
			continue;
		}
	}

	/* make the port */
	if (ltname == 0)
	{
		ttyputerr(_("Port has no name (line %ld)"), lt->lineno);
		return(TRUE);
	}
	if (ltname->parameters != 1 || ltname->paramtype[0] != PARAMATOM)
	{
		ttyputerr(_("Port name must be a single atom (line %ld)"), ltname->lineno);
	}
	if (ltposition == 0)
	{
		ttyputerr(_("Port has no position (line %ld)"), lt->lineno);
		return(TRUE);
	}
	if (ltposition->parameters != 2 || ltposition->paramtype[0] != PARAMATOM ||
		ltposition->paramtype[1] != PARAMATOM)
	{
		ttyputerr(_("Port position must be two atoms (line %ld)"), ltposition->lineno);
	}

	/* create the structure */
	lambda = el_curlib->lambda[fpga_tech->techindex];
	posx = myatoi((CHAR *)ltposition->paramvalue[0]) * lambda;
	posy = myatoi((CHAR *)ltposition->paramvalue[1]) * lambda;
	ni = newnodeinst(fpga_wirepinprim, posx, posx, posy, posy, 0, 0, cell);
	if (ni == NONODEINST)
	{
		ttyputerr(_("Error creating pin for port '%s' (line %ld)"),
			(CHAR *)ltname->paramvalue[0], lt->lineno);
		return(TRUE);
	}
	pp = fpga_wirepinprim->firstportproto;
	expp = newportproto(cell, ni, pp, (CHAR *)ltname->paramvalue[0]);
	if (expp == NOPORTPROTO)
	{
		ttyputerr(_("Error creating port '%s' (line %ld)"),
			(CHAR *)ltname->paramvalue[0], lt->lineno);
		return(TRUE);
	}
	return(FALSE);
}

/*
 * Routine to place a repeater in cell "cell" from the LISPTREE in "lt".
 * Tree has "(repeater...)" structure.  Returns true on error.
 */
BOOLEAN fpga_makeblockrepeater(NODEPROTO *cell, LISPTREE *lt)
{
	REGISTER INTBIG j, angle;
	REGISTER INTBIG portax, portay, portbx, portby, ctrx, ctry, length, lambda;
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var;
	REGISTER LISPTREE *ltname, *ltporta, *ltportb, *scanlt;

	ltname = ltporta = ltportb = 0;
	for(j=0; j<lt->parameters; j++)
	{
		if (lt->paramtype[j] != PARAMBRANCH) continue;
		scanlt = (LISPTREE *)lt->paramvalue[j];
		if (namesame(scanlt->keyword, x_("name")) == 0)
		{
			ltname = scanlt;
			continue;
		}
		if (namesame(scanlt->keyword, x_("porta")) == 0)
		{
			ltporta = scanlt;
			continue;
		}
		if (namesame(scanlt->keyword, x_("portb")) == 0)
		{
			ltportb = scanlt;
			continue;
		}
		if (namesame(scanlt->keyword, x_("direction")) == 0)
		{
			/* ltdirection = scanlt; */
			continue;
		}
	}

	/* make the repeater */
	if (ltporta == 0)
	{
		ttyputerr(_("Repeater has no 'porta' (line %ld)"), lt->lineno);
		return(TRUE);
	}
	if (ltporta->parameters != 2 || ltporta->paramtype[0] != PARAMATOM ||
		ltporta->paramtype[1] != PARAMATOM)
	{
		ttyputerr(_("Repeater 'porta' position must be two atoms (line %ld)"), ltporta->lineno);
	}
	if (ltportb == 0)
	{
		ttyputerr(_("Repeater has no 'portb' (line %ld)"), lt->lineno);
		return(TRUE);
	}
	if (ltportb->parameters != 2 || ltportb->paramtype[0] != PARAMATOM ||
		ltportb->paramtype[1] != PARAMATOM)
	{
		ttyputerr(_("Repeater 'portb' position must be two atoms (line %ld)"), ltportb->lineno);
	}

	/* create the repeater */
	lambda = el_curlib->lambda[fpga_tech->techindex];
	portax = myatoi((CHAR *)ltporta->paramvalue[0]) * lambda;
	portay = myatoi((CHAR *)ltporta->paramvalue[1]) * lambda;
	portbx = myatoi((CHAR *)ltportb->paramvalue[0]) * lambda;
	portby = myatoi((CHAR *)ltportb->paramvalue[1]) * lambda;
	angle = figureangle(portax, portay, portbx, portby);
	length = computedistance(portax, portay, portbx, portby);
	ctrx = (portax + portbx) / 2;
	ctry = (portay + portby) / 2;
	ni = newnodeinst(fpga_repeaterprim, ctrx-length/2, ctrx+length/2,
		ctry-lambda, ctry+lambda, 0, angle, cell);
	if (ni == NONODEINST)
	{
		ttyputerr(_("Error creating repeater (line %ld)"), lt->lineno);
		return(TRUE);
	}

	/* name the repeater if one is given */
	if (ltname != 0)
	{
		if (ltname->parameters != 1 || ltname->paramtype[0] != PARAMATOM)
		{
			ttyputerr(_("Need one atom in 'name' of block repeater (line %ld)"), ltname->lineno);
			return(TRUE);
		}
		var = setvalkey((INTBIG)ni, VNODEINST, el_node_name_key, (INTBIG)ltname->paramvalue[0], VSTRING);
		if (var != NOVARIABLE)
			defaulttextsize(3, var->textdescript);
	}
	return(FALSE);
}

/*
 * Routine to extract block net information from the LISPTREE in "lt".
 * Tree has "(net...)" structure.  Returns true on error.
 */
BOOLEAN fpga_makeblocknet(NODEPROTO *cell, LISPTREE *lt)
{
	REGISTER INTBIG i, j, pos;
	REGISTER NODEINST *ni;
	REGISTER PORTPROTO *pp;
	REGISTER ARCINST *ai;
	REGISTER VARIABLE *var;
	INTBIG px0, py0, px1, py1;
	REGISTER INTBIG x, y, sea, lambda;
	REGISTER GEOM *geom;
	NODEINST *nis[2];
	PORTPROTO *pps[2];
	REGISTER LISPTREE *scanlt;

	/* find the net name */
	for(j=0; j<lt->parameters; j++)
	{
		if (lt->paramtype[j] != PARAMBRANCH) continue;
		scanlt = (LISPTREE *)lt->paramvalue[j];
		if (namesame(scanlt->keyword, x_("name")) == 0)
		{
			if (scanlt->parameters != 1 || scanlt->paramtype[0] != PARAMATOM)
			{
				ttyputerr(_("Net name must be a single atom (line %ld)"), scanlt->lineno);
				return(TRUE);
			}
			/* ltname = scanlt; */
			continue;
		}
	}

	/* scan for segment objects */
	for(j=0; j<lt->parameters; j++)
	{
		if (lt->paramtype[j] != PARAMBRANCH) continue;
		scanlt = (LISPTREE *)lt->paramvalue[j];
		if (namesame(scanlt->keyword, x_("segment")) == 0)
		{
			pos = 0;
			for(i=0; i<2; i++)
			{
				/* get end of arc */
				if (scanlt->parameters < pos+1)
				{
					ttyputerr(_("Incomplete block net segment (line %ld)"), scanlt->lineno);
					return(TRUE);
				}
				if (scanlt->paramtype[pos] != PARAMATOM)
				{
					ttyputerr(_("Must have atoms in block net segment (line %ld)"), scanlt->lineno);
					return(TRUE);
				}
				if (namesame((CHAR *)scanlt->paramvalue[pos], x_("component")) == 0)
				{
					if (scanlt->parameters < pos+3)
					{
						ttyputerr(_("Incomplete block net segment (line %ld)"), scanlt->lineno);
						return(TRUE);
					}
					if (scanlt->paramtype[pos+1] != PARAMATOM || scanlt->paramtype[pos+2] != PARAMATOM)
					{
						ttyputerr(_("Must have atoms in block net segment (line %ld)"), scanlt->lineno);
						return(TRUE);
					}

					/* find component and port */
					for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
					{
						var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
						if (var == NOVARIABLE) continue;
						if (namesame((CHAR *)var->addr, (CHAR *)scanlt->paramvalue[pos+1]) == 0)
							break;
					}
					if (ni == NONODEINST)
					{
						ttyputerr(_("Cannot find component '%s' in block net segment (line %ld)"),
							(CHAR *)scanlt->paramvalue[pos+1], scanlt->lineno);
						return(TRUE);
					}
					nis[i] = ni;
					pps[i] = getportproto(ni->proto, (CHAR *)scanlt->paramvalue[pos+2]);
					if (pps[i] == NOPORTPROTO)
					{
						ttyputerr(_("Cannot find port '%s' on component '%s' in block net segment (line %ld)"),
							(CHAR *)scanlt->paramvalue[pos+2], (CHAR *)scanlt->paramvalue[pos+1], scanlt->lineno);
						return(TRUE);
					}
					pos += 3;
				} else if (namesame((CHAR *)scanlt->paramvalue[pos], x_("coord")) == 0)
				{
					if (scanlt->parameters < pos+3)
					{
						ttyputerr(_("Incomplete block net segment (line %ld)"), scanlt->lineno);
						return(TRUE);
					}
					if (scanlt->paramtype[pos+1] != PARAMATOM || scanlt->paramtype[pos+2] != PARAMATOM)
					{
						ttyputerr(_("Must have atoms in block net segment (line %ld)"), scanlt->lineno);
						return(TRUE);
					}
					lambda = el_curlib->lambda[fpga_tech->techindex];
					x = myatoi((CHAR *)scanlt->paramvalue[pos+1]) * lambda;
					y = myatoi((CHAR *)scanlt->paramvalue[pos+2]) * lambda;

					/* find pin at this point */
					sea = initsearch(x, x, y, y, cell);
					ni = NONODEINST;
					for(;;)
					{
						geom = nextobject(sea);
						if (geom == NOGEOM) break;
						if (!geom->entryisnode) continue;
						ni = geom->entryaddr.ni;
						if (ni->proto != fpga_wirepinprim) continue;
						if ((ni->lowx + ni->highx) / 2 == x && (ni->lowy + ni->highy) / 2 == y)
						{
							termsearch(sea);
							break;
						}
					}
					if (geom == NOGEOM)
					{
						ni = newnodeinst(fpga_wirepinprim, x, x, y, y, 0, 0, cell);
						if (ni == NONODEINST)
						{
							ttyputerr(_("Cannot create pin for block net segment (line %ld)"), scanlt->lineno);
							return(TRUE);
						}
					}
					nis[i] = ni;
					pps[i] = ni->proto->firstportproto;
					pos += 3;
				} else if (namesame((CHAR *)scanlt->paramvalue[pos], x_("port")) == 0)
				{
					if (scanlt->parameters < pos+2)
					{
						ttyputerr(_("Incomplete block net segment (line %ld)"), scanlt->lineno);
						return(TRUE);
					}
					if (scanlt->paramtype[pos+1] != PARAMATOM)
					{
						ttyputerr(_("Must have atoms in block net segment (line %ld)"), scanlt->lineno);
						return(TRUE);
					}

					/* find port */
					pp = getportproto(cell, (CHAR *)scanlt->paramvalue[pos+1]);
					if (pp == NOPORTPROTO)
					{
						ttyputerr(_("Cannot find port '%s' in block net segment (line %ld)"),
							(CHAR *)scanlt->paramvalue[pos+1], scanlt->lineno);
						return(TRUE);
					}
					pps[i] = pp->subportproto;
					nis[i] = pp->subnodeinst;
					pos += 2;
				} else
				{
					ttyputerr(_("Unknown keyword '%s' in block net segment (line %ld)"),
						(CHAR *)scanlt->paramvalue[pos], scanlt->lineno);
					return(TRUE);
				}
			}

			/* now create the arc */
			portposition(nis[0], pps[0], &px0, &py0);
			portposition(nis[1], pps[1], &px1, &py1);
			ai = newarcinst(fpga_tech->firstarcproto, 0, FIXANG, nis[0], pps[0], px0, py0,
				nis[1], pps[1], px1, py1, cell);
			if (ai == NOARCINST)
			{
				ttyputerr(_("Cannot run segment (line %ld)"), scanlt->lineno);
				return(TRUE);
			}
		}
	}
	return(FALSE);
}
