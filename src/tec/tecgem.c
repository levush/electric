/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tecgem.c
 * Group Element Model (from Amy Lansky) technology description
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

#include "config.h"
#include "global.h"
#include "egraphics.h"
#include "tech.h"
#include "tecgem.h"
#include "efunction.h"

/* prototypes for local routines */
static void gem_addtailplus(POLYGON*, INTBIG, INTBIG*, INTBIG*, INTBIG);
static void gem_addheadplus(POLYGON*, INTBIG, INTBIG*, INTBIG*, INTBIG);
static void gem_add1linebody(POLYGON*, INTBIG, INTBIG, INTBIG, INTBIG);
static void gem_addzigzagbody(POLYGON*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static void gem_intshapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl);
static void gem_intshapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly, POLYLOOP *pl);

static TECHNOLOGY *gem_tech;

/****************************** LAYERS **************************************/

#define MAXLAYERS  7		/* total layers below */

#define LELEGROUP  0		/* elements and groups */
#define LGENARC    1		/* general arcs */
#define LTEMPARC   2		/* temporal arcs */
#define LCAUSEARC  3		/* causal arcs */
#define LPREARC    4		/* prerequisite arcs */
#define LNONARC    5		/* nondeterministic arcs */
#define LFORKARC   6		/* fork arcs */

static GRAPHICS gem_e_lay = {LAYERO, RED, SOLIDC, SOLIDC,
/* node */		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS gem_ga_lay = {LAYERO, BLUE, SOLIDC, SOLIDC,
/* general arc */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS gem_ta_lay = {LAYERO, GREEN, SOLIDC, SOLIDC,
/* temporal arc */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS gem_ca_lay = {LAYERO, BLACK, SOLIDC, SOLIDC,
/* causal arc */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS gem_pa_lay = {LAYERO, ORANGE, SOLIDC, SOLIDC,
/* prereq. arc */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS gem_na_lay = {LAYERO, YELLOW, SOLIDC, SOLIDC,
/* nondet arc */	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS gem_fa_lay = {LAYERO, PURPLE, SOLIDC, SOLIDC,
/* fork arc */		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};

/* these tables must be updated togehter */
GRAPHICS *gem_layers[MAXLAYERS+1] = {&gem_e_lay, &gem_ga_lay,
	&gem_ta_lay, &gem_ca_lay, &gem_pa_lay, &gem_na_lay, &gem_fa_lay, NOGRAPHICS};
static CHAR *gem_layer_names[MAXLAYERS] = {x_("Element"), x_("General-arc"),
	x_("Temporal-arc"), x_("Causal-arc"), x_("Prereq-arc"), x_("Nondet-arc"), x_("Fork-arc")};
static INTBIG gem_layer_function[MAXLAYERS] = {LFART|LFNONELEC,
	LFCONTROL|LFNONELEC, LFCONTROL|LFNONELEC, LFCONTROL|LFNONELEC,
	LFCONTROL|LFNONELEC, LFCONTROL|LFNONELEC, LFCONTROL|LFNONELEC};
static CHAR *gem_layer_letters[MAXLAYERS] = {x_("e"), x_("g"), x_("t"), x_("c"), x_("p"), x_("n"), x_("f")};

/********************************* ARCS *************************************/

#define ARCPROTOCOUNT  6
#define AGENERAL       0		/* general arc */
#define ATEMP          1		/* temporal arc */
#define ACAUSE         2		/* causal arc */
#define APREREQ        3		/* prerequisite arc */
#define ANONDET        4		/* nondeterministic arc */
#define AFORK          5		/* fork arc */

/* General arc */
static TECH_ARCLAY gem_gw[] = {{LGENARC,0,VECTORS}};
static TECH_ARCS gem_a_g = {
	x_("General"),0,AGENERAL,NOARCPROTO,				/* name */
	1,gem_gw,											/* layers */
	(APNONELEC<<AFUNCTIONSH)|CANWIPE|(0<<AANGLEINCSH)};	/* userbits */

/* Temporal arc */
static TECH_ARCLAY gem_tw[] = {{LTEMPARC,0,VECTORS}};
static TECH_ARCS gem_a_t = {
	x_("Temporal"),0,ATEMP,NOARCPROTO,					/* name */
	1,gem_tw,											/* layers */
	(APNONELEC<<AFUNCTIONSH)|CANWIPE|(0<<AANGLEINCSH)};	/* userbits */

/* Causal arc */
static TECH_ARCLAY gem_cw[] = {{LCAUSEARC,0,VECTORS}};
static TECH_ARCS gem_a_c = {
	x_("Causal"),0,ACAUSE,NOARCPROTO,					/* name */
	1,gem_cw,											/* layers */
	(APNONELEC<<AFUNCTIONSH)|CANWIPE|(0<<AANGLEINCSH)};	/* userbits */

/* Prereq arc */
static TECH_ARCLAY gem_pw[] = {{LPREARC,0,VECTORS}};
static TECH_ARCS gem_a_p = {
	x_("Prerequisite"),0,APREREQ,NOARCPROTO,			/* name */
	1,gem_pw,											/* layers */
	(APNONELEC<<AFUNCTIONSH)|CANWIPE|(0<<AANGLEINCSH)};	/* userbits */

/* Nondeterministic arc */
static TECH_ARCLAY gem_nw[] = {{LNONARC,0,VECTORS}};
static TECH_ARCS gem_a_n = {
	x_("Nondeterministic"),0,ANONDET,NOARCPROTO,		/* name */
	1,gem_nw,											/* layers */
	(APNONELEC<<AFUNCTIONSH)|CANWIPE|(0<<AANGLEINCSH)};	/* userbits */

/* Nondeterministic-fork arc */
static TECH_ARCLAY gem_fw[] = {{LFORKARC,0,VECTORS}};
static TECH_ARCS gem_a_f = {
	x_("Nondeterministic-fork"),0,AFORK,NOARCPROTO,		/* name */
	1,gem_fw,											/* layers */
	(APNONELEC<<AFUNCTIONSH)|CANWIPE|(0<<AANGLEINCSH)};	/* userbits */

TECH_ARCS *gem_arcprotos[ARCPROTOCOUNT+1] = {
	&gem_a_g, &gem_a_t, &gem_a_c, &gem_a_p, &gem_a_n, &gem_a_f, ((TECH_ARCS *)-1)};

/************************* PORT and POLYGON INFORMATION *********************/

/* these values are replaced with actual arcproto addresses */
static INTBIG gem_pc[] = {-1, AGENERAL, ATEMP, ACAUSE, APREREQ, ANONDET, AFORK,
	ALLGEN, -1};
static INTBIG gem_pcg[] = {-1, AGENERAL, ALLGEN, -1};
static INTBIG gem_pct[] = {-1, ATEMP, ALLGEN, -1};
static INTBIG gem_pcc[] = {-1, ACAUSE, ALLGEN, -1};
static INTBIG gem_pcp[] = {-1, APREREQ, ALLGEN, -1};
static INTBIG gem_pcn[] = {-1, ANONDET, ALLGEN, -1};
static INTBIG gem_pcf[] = {-1, AFORK, ALLGEN, -1};

/* polygon descriptions */
static CHAR gem_Element[] = {x_("Element")};
static CHAR gem_sc1[] = {x_("1")};
static CHAR gem_sc2[] = {x_("2")};
static CHAR gem_sc3[] = {x_("3")};
static CHAR gem_sc4[] = {x_("4")};

static INTBIG gem_circle[]      = {CENTER,   CENTER,   CENTER,    TOPEDGE};
static INTBIG gem_fullbox[]     = {LEFTEDGE, BOTEDGE,  RIGHTEDGE, TOPEDGE};
static CHAR *gem_elementname[] = {(CHAR *)-H0,  (CHAR *)0, (CHAR *)0, (CHAR *)K1,
								  (CHAR *)H0,  (CHAR *)0, (CHAR *)H0,  (CHAR *)0,
								  gem_Element};
static CHAR *gem_event1name[]  = {(CHAR *)-H0, (CHAR *)K2,  (CHAR *)0, (CHAR *)0,  gem_sc1};
static CHAR *gem_event2name[]  = {(CHAR *)-H0, (CHAR *)K2,  (CHAR *)0,(CHAR *)-K1, gem_sc2};
static CHAR *gem_event3name[]  = {(CHAR *)-H0, (CHAR *)K2,  (CHAR *)0,(CHAR *)-K2, gem_sc3};
static CHAR *gem_event4name[]  = {(CHAR *)-H0, (CHAR *)K2,  (CHAR *)0,(CHAR *)-K3, gem_sc4};

/********************************** NODES **********************************/

#define NODEPROTOCOUNT	 8
#define NELEMENT         1		/* element */
#define NGROUP           2		/* group */
#define NGENERALP        3		/* general arc pin */
#define NTEMPP           4		/* temporal arc pin */
#define NCAUSEP          5		/* causal arc pin */
#define NPREREQP         6		/* prerequisite arc pin */
#define NNONDETP         7		/* nondeterministic arc pin */
#define NFORKP           8		/* fork arc PIN */

/* element */
static TECH_PORTS gem_e_p[] = {				/* ports */
	{gem_pc, x_("port1"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN2, CENTERU0H, LEFTIN2, CENTERU0H},
	{gem_pc, x_("port2"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN2, CENTERD0H, LEFTIN2, CENTERD0H},
	{gem_pc, x_("port3"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN2, CENTERD1H, LEFTIN2, CENTERD1H},
	{gem_pc, x_("port4"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTIN2, CENTERD2H, LEFTIN2, CENTERD2H}};
static TECH_POLYGON gem_e_l[] = {			/* layers */
	{LELEGROUP, -1, 2, CIRCLE,      POINTS, gem_circle},
	{LELEGROUP, -1, 4, TEXTBOX,     BOX,    (INTBIG *)gem_elementname},
	{LELEGROUP, -1, 1, TEXTBOTLEFT, POINTS, (INTBIG *)gem_event1name},
	{LELEGROUP, -1, 1, TEXTBOTLEFT, POINTS, (INTBIG *)gem_event2name},
	{LELEGROUP, -1, 1, TEXTBOTLEFT, POINTS, (INTBIG *)gem_event3name},
	{LELEGROUP, -1, 1, TEXTBOTLEFT, POINTS, (INTBIG *)gem_event4name}};
static TECH_NODES gem_e = {
	x_("Element"),NELEMENT,NONODEPROTO,		/* name */
	K8,K8,									/* size */
	4,gem_e_p,								/* ports */
	6,gem_e_l,								/* layers */
	(NPUNKNOWN<<NFUNCTIONSH),				/* userbits */
	0,0,0,0,0,0,0,0,0};						/* characteristics */

/* group */
static TECH_PORTS gem_g_p[] = {				/* ports */
	{gem_pc, x_("group"),    NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON gem_g_l[] = {			/* layers */
	{LELEGROUP, -1, 4, CLOSEDRECT, BOX, gem_fullbox}};
static TECH_NODES gem_g = {
	x_("Group"),NGROUP,NONODEPROTO,			/* name */
	K10,K10,								/* size */
	1,gem_g_p,								/* ports */
	1,gem_g_l,								/* layers */
	(NPUNKNOWN<<NFUNCTIONSH)|HOLDSTRACE,	/* userbits */
	0,0,0,0,0,0,0,0,0};						/* characteristics */

/* general-pin */
static TECH_PORTS gem_gp_p[] = {			/* ports */
	{gem_pcg, x_("general"),    NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON gem_gp_l[] = {			/* layers */
	{LGENARC, -1, 2, DISC, POINTS, gem_circle}};
static TECH_NODES gem_gp = {
	x_("General-Pin"),NGENERALP,NONODEPROTO,/* name */
	K1,K1,									/* size */
	1,gem_gp_p,								/* ports */
	1,gem_gp_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE,			/* userbits */
	0,0,0,0,0,0,0,0,0};						/* characteristics */

/* temporal-pin */
static TECH_PORTS gem_tp_p[] = {			/* ports */
	{gem_pct, x_("temporal"),    NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON gem_tp_l[] = {			/* layers */
	{LTEMPARC, -1, 2, DISC, POINTS, gem_circle}};
static TECH_NODES gem_tp = {
	x_("Temporal-Pin"),NTEMPP,NONODEPROTO,	/* name */
	K1,K1,									/* size */
	1,gem_tp_p,								/* ports */
	1,gem_tp_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE,			/* userbits */
	0,0,0,0,0,0,0,0,0};						/* characteristics */

/* causal-pin */
static TECH_PORTS gem_cp_p[] = {			/* ports */
	{gem_pcc, x_("cause"),    NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON gem_cp_l[] = {			/* layers */
	{LCAUSEARC, -1, 2, DISC, POINTS, gem_circle}};
static TECH_NODES gem_cp = {
	x_("Cause-Pin"),NCAUSEP,NONODEPROTO,	/* name */
	K1,K1,									/* size */
	1,gem_cp_p,								/* ports */
	1,gem_cp_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE,			/* userbits */
	0,0,0,0,0,0,0,0,0};						/* characteristics */

/* prereq-pin */
static TECH_PORTS gem_pp_p[] = {			/* ports */
	{gem_pcp, x_("prereq"),    NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON gem_pp_l[] = {			/* layers */
	{LPREARC, -1, 2, DISC, POINTS, gem_circle}};
static TECH_NODES gem_pp = {
	x_("Prereq-Pin"),NPREREQP,NONODEPROTO,	/* name */
	K1,K1,									/* size */
	1,gem_pp_p,								/* ports */
	1,gem_pp_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE,			/* userbits */
	0,0,0,0,0,0,0,0,0};						/* characteristics */

/* nondeterministic-pin */
static TECH_PORTS gem_np_p[] = {			/* ports */
	{gem_pcn, x_("nondet"),    NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON gem_np_l[] = {			/* layers */
	{LNONARC, -1, 2, DISC, POINTS, gem_circle}};
static TECH_NODES gem_np = {
	x_("Nondet-Pin"),NNONDETP,NONODEPROTO,	/* name */
	K1,K1,									/* size */
	1,gem_np_p,								/* ports */
	1,gem_np_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE,			/* userbits */
	0,0,0,0,0,0,0,0,0};						/* characteristics */

/* fork-pin */
static TECH_PORTS gem_fp_p[] = {			/* ports */
	{gem_pcf, x_("fork"),    NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON gem_fp_l[] = {			/* layers */
	{LFORKARC, -1, 2, DISC, POINTS, gem_circle}};
static TECH_NODES gem_fp = {
	x_("Fork-Pin"),NFORKP,NONODEPROTO,		/* name */
	K1,K1,									/* size */
	1,gem_fp_p,								/* ports */
	1,gem_fp_l,								/* layers */
	(NPPIN<<NFUNCTIONSH)|ARCSWIPE,			/* userbits */
	0,0,0,0,0,0,0,0,0};						/* characteristics */

TECH_NODES *gem_nodeprotos[NODEPROTOCOUNT+1] = { &gem_e, &gem_g,
	&gem_gp, &gem_tp, &gem_cp, &gem_pp, &gem_np, &gem_fp, ((TECH_NODES *)-1)};

/******************** VARIABLE AGGREGATION ********************/

TECH_VARIABLES gem_variables[] =
{
	/* set general information about the technology */
	{x_("TECH_layer_names"), (CHAR *)gem_layer_names, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_layer_function"), (CHAR *)gem_layer_function, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},

	/* set information for the USER tool */
	{x_("USER_layer_letters"), (CHAR *)gem_layer_letters, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{NULL, NULL, 0.0, 0}
};

/******************** ROUTINES ********************/

BOOLEAN gem_initprocess(TECHNOLOGY *tech, INTBIG pass)
{
	/* initialize the technology variable */
	if (pass == 0) gem_tech = tech;
	return(FALSE);
}

void gem_shapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly)
{
	gem_intshapenodepoly(ni, box, poly, &tech_oneprocpolyloop);
}

void gem_intshapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl)
{
	REGISTER TECH_POLYGON *lay;
	REGISTER VARIABLE *var;
	REGISTER INTBIG pindex;
	static INTBIG elementkey = 0, event1key = 0, event2key = 0, event3key = 0, event4key = 0;

	/* handle displayable variables */
	if (box >= pl->realpolys)
	{
		(void)tech_filldisplayablenvar(ni, poly, pl->curwindowpart, 0, pl);
		return;
	}

	pindex = ni->proto->primindex;
	lay = &gem_nodeprotos[pindex-1]->layerlist[box];
	tech_fillpoly(poly, lay, ni, lambdaofnode(ni), CLOSED);
	poly->desc = gem_layers[poly->layer];

	if (box == 0) return;

	/* get element message */
	TDCLEAR(poly->textdescript);
	TDSETSIZE(poly->textdescript, TXTSETQLAMBDA(4));
	poly->tech = gem_tech;
	switch (box)
	{
		case 1:
			if (elementkey == 0) elementkey = makekey(x_("GEM_element"));
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, elementkey);
			TDCLEAR(poly->textdescript);
			TDSETSIZE(poly->textdescript, TXTSETQLAMBDA(12));
			break;
		case 2:
			if (event1key == 0) event1key = makekey(x_("GEM_event1"));
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, event1key);
			break;
		case 3:
			if (event2key == 0) event2key = makekey(x_("GEM_event2"));
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, event2key);
			break;
		case 4:
			if (event3key == 0) event3key = makekey(x_("GEM_event3"));
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, event3key);
			break;
		case 5:
			if (event4key == 0) event4key = makekey(x_("GEM_event4"));
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, event4key);
			break;
		default:
			var = NOVARIABLE;
	}
	if (var != NOVARIABLE) poly->string = (CHAR *)var->addr;
}

INTBIG gem_allnodepolys(NODEINST *ni, POLYLIST *plist, WINDOWPART *win, BOOLEAN onlyreasonable)
{
	REGISTER INTBIG tot, j;
	INTBIG reasonable;
	REGISTER NODEPROTO *np;
	REGISTER POLYGON *poly;
	POLYLOOP mypl;

	np = ni->proto;
	mypl.curwindowpart = win;
	tot = tech_nodepolys(ni, &reasonable, win, &mypl);
	if (onlyreasonable) tot = reasonable;
	if (mypl.realpolys < tot) tot = mypl.realpolys;
	if (ensurepolylist(plist, tot, db_cluster)) return(-1);
	for(j = 0; j < tot; j++)
	{
		poly = plist->polygons[j];
		poly->tech = gem_tech;
		gem_intshapenodepoly(ni, j, poly, &mypl);
	}
	return(tot);
}

void gem_shapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly)
{
	gem_intshapearcpoly(ai, box, poly, &tech_oneprocpolyloop);
}

void gem_intshapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly, POLYLOOP *pl)
{
	REGISTER INTBIG angle;
	REGISTER INTBIG aindex, i, dist, lambda;
	INTBIG x1, y1, x2, y2;
	REGISTER TECH_ARCLAY *thista;

	/* handle displayable variables */
	if (box >= pl->realpolys)
	{
		(void)tech_filldisplayableavar(ai, poly, pl->curwindowpart, 0, pl);
		return;
	}

	aindex = ai->proto->arcindex;
	thista = &gem_arcprotos[aindex]->list[box];
	poly->layer = thista->lay;
	poly->style = thista->style;
	poly->desc = gem_layers[poly->layer];
	poly->count = 0;
	lambda = lambdaofarc(ai);
	x1 = ai->end[0].xpos;   y1 = ai->end[0].ypos;
	x2 = ai->end[1].xpos;   y2 = ai->end[1].ypos;
	angle = ((ai->userbits&AANGLE) >> AANGLESH) * 10;
	if ((ai->userbits&REVERSEEND) != 0)
	{
		i = x1;   x1 = x2;   x2 = i;
		i = y1;   y1 = y2;   y2 = i;
		angle = (angle+1800) % 3600;
	}

	switch (aindex)
	{
		case AGENERAL:
			/* general arc is simply a straight line */
			gem_add1linebody(poly, x1,y1, x2,y2);
			break;

		case APREREQ:
			/* prerequisite arc has an arrow at the head */
			gem_add1linebody(poly, x1,y1, x2,y2);
			if ((ai->userbits&NOTEND1) == 0)
				tech_addheadarrow(poly, angle, x2,y2, lambda);
			break;

		case AFORK:
			/* fork arc has an arrow at the head and a "+" at the tail */
			if ((ai->userbits&NOTEND0) == 0)
				gem_addtailplus(poly, angle, &x1,&y1, lambda);
			if ((ai->userbits&NOTEND1) == 0)
				tech_addheadarrow(poly, angle, x2,y2, lambda);
			gem_add1linebody(poly, x1,y1, x2,y2);
			break;

		case ANONDET:
			/* nondeterministic arc has an arrow and a "+" at the head */
			if ((ai->userbits&NOTEND1) == 0)
			{
				gem_addheadplus(poly, angle, &x2,&y2, lambda);
				tech_addheadarrow(poly, angle, x2,y2, lambda);
			}
			gem_add1linebody(poly, x1,y1, x2,y2);
			break;

		case ATEMP:
			/* temporal arc has a double-line with an arrow head */
			dist = H0 * lambda / WHOLE;
			if ((ai->userbits&NOTEND1) == 0)
				tech_adddoubleheadarrow(poly, angle, &x2,&y2, dist);
			tech_add2linebody(poly, angle, x1,y1, x2,y2, dist);
			break;

		case ACAUSE:
			/* causal arc has zig-zag body */
			gem_addzigzagbody(poly, angle, x1,y1, x2,y2, lambda);
			break;
	}
}

INTBIG gem_allarcpolys(ARCINST *ai, POLYLIST *plist, WINDOWPART *win)
{
	REGISTER INTBIG tot, j;
	POLYLOOP mypl;

	mypl.curwindowpart = win;
	tot = tech_arcpolys(ai, win, &mypl);
	if (ensurepolylist(plist, tot, db_cluster)) return(-1);
	for(j = 0; j < tot; j++)
	{
		gem_intshapearcpoly(ai, j, plist->polygons[j], &mypl);
	}
	return(tot);
}

/*
 * helper routine to add a "+" to the tail of the arc in "poly", given that
 * the arc runs to (x,y) and is at an angle of "angle" tenth-degrees
 */
void gem_addtailplus(POLYGON *poly, INTBIG angle, INTBIG *x, INTBIG *y, INTBIG lambda)
{
	REGISTER INTBIG sin, cos, dist, threehalfdist, twicedist;
	REGISTER INTBIG c;

	c = poly->count;
	if (poly->limit < c+4) (void)extendpolygon(poly, c+4);
	dist = K1 * lambda / WHOLE;
	threehalfdist = dist + dist / 2;
	twicedist = dist * 2;
	sin = sine(angle);
	cos = cosine(angle);
	poly->xv[c] = *x;   poly->yv[c] = *y;   c++;
	poly->xv[c] = *x + mult(cos, threehalfdist);
	poly->yv[c] = *y + mult(sin, threehalfdist);   c++;
	poly->xv[c] = *x + mult(cosine((angle+450) % 3600), dist);
	poly->yv[c] = *y + mult(sine((angle+450) % 3600), dist);   c++;
	poly->xv[c] = *x + mult(cosine((angle+3150) % 3600), dist);
	poly->yv[c] = *y + mult(sine((angle+3150) % 3600), dist);   c++;
	*x += mult(cos, twicedist);   *y += mult(sin, twicedist);
	poly->count = c;
}

/*
 * helper routine to add a "+" to the head of the arc in "poly", given that
 * the arc runs from (x,y) and is at an angle of "angle" tenth-degrees
 */
void gem_addheadplus(POLYGON *poly, INTBIG angle, INTBIG *x, INTBIG *y, INTBIG lambda)
{
	REGISTER INTBIG c;
	REGISTER INTBIG sin, cos, dist, threehalfdist, twicedist;

	c = poly->count;
	if (poly->limit < c+4) (void)extendpolygon(poly, c+4);
	dist = K1 * lambda / WHOLE;
	threehalfdist = dist + dist / 2;
	twicedist = dist * 2;
	sin = sine((angle+1800) % 3600);
	cos = cosine((angle+1800) % 3600);
	poly->xv[c] = *x;   poly->yv[c] = *y;   c++;
	poly->xv[c] = *x + mult(cos, threehalfdist);
	poly->yv[c] = *y + mult(sin, threehalfdist);   c++;
	poly->xv[c] = *x + mult(cosine((angle+1350) % 3600), dist);
	poly->yv[c] = *y + mult(sine((angle+1350) % 3600), dist);   c++;
	poly->xv[c] = *x + mult(cosine((angle+2250) % 3600), dist);
	poly->yv[c] = *y + mult(sine((angle+2250) % 3600), dist);   c++;
	*x += mult(cos, twicedist);   *y += mult(sin, twicedist);
	poly->count = c;
}

/*
 * helper routine to add a single-line body to the arc in "poly", given that
 * the arc runs from (x1,y1) to (x2,y2)
 */
void gem_add1linebody(POLYGON *poly, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2)
{
	REGISTER INTBIG c;

	c = poly->count;
	if (poly->limit < c+2) (void)extendpolygon(poly, c+2);
	poly->xv[c] = x1;    poly->yv[c] = y1;   c++;
	poly->xv[c] = x2;    poly->yv[c] = y2;   c++;
	poly->count = c;
}

/*
 * helper routine to add a zig-zag body to the arc in "poly", given that
 * the arc runs from (x1,y1) to (x2,y2) and is at an angle of "angle" tenth-degrees.
 */
void gem_addzigzagbody(POLYGON *poly, INTBIG angle, INTBIG x1, INTBIG y1,
	INTBIG x2, INTBIG y2, INTBIG lambda)
{
	REGISTER INTBIG xc, yc, dist;
	REGISTER INTBIG c;

	/* get the center of the zig-zag */
	xc = (x1 + x2) / 2;  yc = (y1 + y2) / 2;

	c = poly->count;
	if (poly->limit < c+6) (void)extendpolygon(poly, c+6);
	dist = K1 * lambda / WHOLE;
	poly->xv[c] = x1;   poly->yv[c] = y1;   c++;
	poly->xv[c] = poly->xv[c+1] = xc + mult(cosine((angle+900) % 3600), dist);
	poly->yv[c] = poly->yv[c+1] = yc + mult(sine((angle+900) % 3600), dist);
	c += 2;
	poly->xv[c] = poly->xv[c+1] = xc + mult(cosine((angle+2700) % 3600),dist);
	poly->yv[c] = poly->yv[c+1] = yc + mult(sine((angle+2700) % 3600), dist);
	c += 2;
	poly->xv[c] = x2;   poly->yv[c] = y2;   c++;
	poly->count = c;
}
