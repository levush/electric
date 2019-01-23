/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tecgen.c
 * Generic technology description
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
#include "egraphics.h"
#include "tech.h"
#include "efunction.h"
#include "tecgen.h"
#include "tecart.h"

typedef struct
{
	INTBIG incol,  inbits;
	INTBIG intcol, intbits;
	INTBIG portbox1, portbox3, portbox9, portbox11;
} GENPOLYLOOP;

static GENPOLYLOOP gen_oneprocpolyloop;

TECHNOLOGY *gen_tech;
NODEPROTO  *gen_univpinprim, *gen_invispinprim, *gen_unroutedpinprim, *gen_cellcenterprim,
	*gen_portprim, *gen_drcprim, *gen_essentialprim, *gen_simprobeprim;
ARCPROTO   *gen_universalarc, *gen_invisiblearc, *gen_unroutedarc;
ARCPROTO  **gen_upconn = 0;

/* prototypes for local routines */
static void gen_makeunivlist(void);
static INTBIG gen_intnodepolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win, POLYLOOP *pl,
			GENPOLYLOOP *genpl);
static void gen_intshapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl, GENPOLYLOOP *genpl);

/******************** LAYERS ********************/

#define MAXLAYERS       6		/* total layers below */
#define LUNIV           0		/* universal layer (connects all) */
#define LINVIS          1		/* invisible layer (nonelectrical) */
#define LUNROUTED       2		/* unrouted layer (for routers) */
#define LGLYPH          3		/* glyph layer (for menu icons) */
#define LDRC            4		/* drc ignore layer */
#define LSIMPROBE       5		/* simulation probe layer */

static GRAPHICS gen_un_lay = {LAYERO, MENTXT, SOLIDC, SOLIDC,
/* univ */					{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS gen_in_lay = {LAYERO, GRAY,  SOLIDC, SOLIDC,
/* invis */					{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS gen_int_lay = {LAYERO, BLACK,  SOLIDC, SOLIDC,
/* invis text */			{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS gen_ur_lay = {LAYERO, DGRAY, SOLIDC, SOLIDC,
/* unrouted */				{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS gen_gl_lay = {LAYERO, MENGLY, SOLIDC, SOLIDC,
/* glyph */					{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS gen_dr_lay = {LAYERO, ORANGE, SOLIDC, SOLIDC,
/* drc */					{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};
static GRAPHICS gen_sp_lay = {LAYERO, GREEN, SOLIDC, SOLIDC,
/* sim probe */				{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, NOVARIABLE, 0};

/* these tables must be updated togehter */
GRAPHICS *gen_layers[MAXLAYERS+1] = {&gen_un_lay, &gen_in_lay,
	&gen_ur_lay, &gen_gl_lay, &gen_dr_lay, &gen_sp_lay, NOGRAPHICS};
static CHAR *gen_layer_names[MAXLAYERS] = {x_("Universal"), x_("Invisible"),
	x_("Unrouted"), x_("Glyph"), x_("DRC"), x_("Sim-Probe")};
static CHAR *gen_cif_layers[MAXLAYERS] = {x_(""), x_(""), x_(""), x_(""), x_("DRC"), x_("")};
static INTBIG gen_layer_function[MAXLAYERS] = {LFUNKNOWN, LFUNKNOWN|LFNONELEC,
	LFUNKNOWN, LFART|LFNONELEC, LFART|LFNONELEC, LFART|LFNONELEC};
static CHAR *gen_layer_letters[MAXLAYERS] = {x_("u"), x_("i"), x_("r"), x_("g"), x_("d"), x_("s")};

/******************** ARCS ********************/

#define ARCPROTOCOUNT     3
#define GENAUNIV          0
#define GENAINVIS         1
#define GENAUNROUTED      2

/* universal arc */
static TECH_ARCLAY gen_al_u[] = {{LUNIV,0,FILLED}};
static TECH_ARCS gen_a_u = {
	x_("Universal"),0,GENAUNIV,NOARCPROTO,					/* name */
	1,gen_al_u,												/* layers */
	(APUNKNOWN<<AFUNCTIONSH)|WANTFIXANG|(45<<AANGLEINCSH)};	/* userbits */

/* invisible arc */
static TECH_ARCLAY gen_al_i[] = {{LINVIS,0,FILLED}};
static TECH_ARCS gen_a_i = {
	x_("Invisible"),0,GENAINVIS,NOARCPROTO,					/* name */
	1,gen_al_i,												/* layers */
	(APNONELEC<<AFUNCTIONSH)|WANTFIXANG|(45<<AANGLEINCSH)};	/* userbits */

/* unrouted arc */
static TECH_ARCLAY gen_al_r[] = {{LUNROUTED,0,FILLED}};
static TECH_ARCS gen_a_r = {
	x_("Unrouted"),0,GENAUNROUTED,NOARCPROTO,				/* name */
	1,gen_al_r,												/* layers */
	(APUNROUTED<<AFUNCTIONSH)|(0<<AANGLEINCSH)};			/* userbits */

TECH_ARCS *gen_arcprotos[ARCPROTOCOUNT+1] = {
	&gen_a_u, &gen_a_i, &gen_a_r, ((TECH_ARCS *)-1)};

/******************** PORT CONNECTIONS **************************/

/* these values are replaced with actual arcproto addresses */
static INTBIG gen_pc_iu[]  = {-1, GENAINVIS, GENAUNIV, -1};
static INTBIG gen_pc_riu[] = {-1, GENAUNROUTED, GENAINVIS, GENAUNIV, -1};

/******************** NODES ********************/

#define NODEPROTOCOUNT     8
#define NUNIV              1		/* universal pin */
#define NINVIS             2		/* invisible pin */
#define NUNROUTED          3		/* unrouted pin */
#define NCENTER            4		/* cell center */
#define NPORT              5		/* port declaration (for tech edit) */
#define NDRC               6		/* drc ignore mask (for ECAD's DRC) */
#define NESSENTIAL         7		/* essential bounds marker */
#define NSIMPROBE          8		/* simulation probe */

static INTBIG gen_fullbox[]    = {LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE};
static INTBIG gen_disccenter[] = {CENTER,   CENTER,  RIGHTEDGE, CENTER};
static INTBIG gen_in2box[]     = {LEFTIN2,  BOTIN2,  RIGHTIN2,  TOPIN2};
static INTBIG gen_center_lc[]  = {LEFTEDGE, BOTEDGE, LEFTEDGE,  BOTEDGE};
static INTBIG gen_portabox[]   = {CENTER,   CENTER,  CENTER,    CENTER,
								  CENTER,   CENTER};
static INTBIG gen_essentiall[] = {CENTERL1, CENTER,  CENTER,    CENTER,
								  CENTER,   CENTERD1};
/* Universal pin */
static TECH_PORTS gen_u_p[] = {						/* ports */
	{(INTBIG *)0, x_("univ"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON gen_u_l[] = {					/* layers */
	{LUNIV, 0, 2, DISC, POINTS, gen_disccenter}};
static TECH_NODES gen_u = {
	x_("Universal-Pin"),NUNIV,NONODEPROTO,			/* name */
	K1,K1,											/* size */
	1,gen_u_p,										/* ports */
	1,gen_u_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|HOLDSTRACE|WIPEON1OR2,		/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Invisible Pin */
static TECH_PORTS gen_i_p[] = {{					/* ports */
	gen_pc_iu, x_("center"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON gen_i_l[] = {					/* layers */
	{LINVIS, 0, 4, CLOSEDRECT, BOX, gen_fullbox}};
static TECH_NODES gen_i = {
	x_("Invisible-Pin"),NINVIS,NONODEPROTO,			/* name */
	K1,K1,											/* size */
	1,gen_i_p,										/* ports */
	1,gen_i_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2,				/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Unrouted Pin */
static TECH_PORTS gen_r_p[] = {{					/* ports */
	gen_pc_riu, x_("unrouted"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON gen_r_l[] = {					/* layers */
	{LUNROUTED, 0, 2, DISC, POINTS, gen_disccenter}};
static TECH_NODES gen_r = {
	x_("Unrouted-Pin"),NUNROUTED,NONODEPROTO,		/* name */
	K1,K1,											/* size */
	1,gen_r_p,										/* ports */
	1,gen_r_l,										/* layers */
	(NPPIN<<NFUNCTIONSH)|WIPEON1OR2,				/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Cell Center */
static TECH_PORTS gen_c_p[] = {{					/* ports */
	gen_pc_iu, x_("center"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, LEFTEDGE, BOTEDGE}};
static TECH_POLYGON gen_c_l[] = {					/* layers */
	{LGLYPH, 0, 4, CLOSED,   BOX,    gen_fullbox},
	{LGLYPH, 0, 2, BIGCROSS, POINTS, gen_center_lc}};
static TECH_NODES gen_c = {
	x_("Facet-Center"),NCENTER,NONODEPROTO,			/* name */
	K0,K0,											/* size */
	1,gen_c_p,										/* ports */
	2,gen_c_l,										/* layers */
	(NPART<<NFUNCTIONSH),							/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Port */
static TECH_PORTS gen_p_p[] = {{					/* ports */
	gen_pc_iu, x_("center"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON gen_p_l[] = {					/* layers */
	{LGLYPH, 0,         4, CLOSED,   BOX,    gen_in2box},
	{LGLYPH, 0,         3, OPENED,   POINTS, gen_portabox}};
static TECH_NODES gen_p = {
	x_("Port"),NPORT,NONODEPROTO,					/* name */
	K6,K6,											/* size */
	1,gen_p_p,										/* ports */
	2,gen_p_l,										/* layers */
	(NPART<<NFUNCTIONSH),							/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* DRC Node */
static TECH_PORTS gen_d_p[] = {{					/* ports */
	gen_pc_iu, x_("center"), NOPORTPROTO, (180<<PORTARANGESH),
		CENTER, CENTER, CENTER, CENTER}};
static TECH_POLYGON gen_d_l[] = {					/* layers */
	{LDRC, 0, 4, CLOSEDRECT, BOX, gen_fullbox}};
static TECH_NODES gen_d = {
	x_("DRC-Node"),NDRC,NONODEPROTO,				/* name */
	K2,K2,											/* size */
	1,gen_d_p,										/* ports */
	1,gen_d_l,										/* layers */
	(NPNODE<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	POLYGONAL,0,0,0,0,0,0,0,0};						/* characteristics */

/* Essential Bounds marker */
static TECH_PORTS gen_e_p[] = {{					/* ports */
	gen_pc_iu, x_("center"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, LEFTEDGE, BOTEDGE}};
static TECH_POLYGON gen_e_l[] = {					/* layers */
	{LGLYPH, 0, 3, OPENED, POINTS, gen_essentiall}};
static TECH_NODES gen_e = {
	x_("Essential-Bounds"),NESSENTIAL,NONODEPROTO,	/* name */
	K0,K0,											/* size */
	1,gen_e_p,										/* ports */
	1,gen_e_l,										/* layers */
	(NPART<<NFUNCTIONSH),							/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

/* Simulation Probe node */
static TECH_PORTS gen_s_p[] = {{					/* ports */
	gen_pc_riu, x_("center"), NOPORTPROTO, (180<<PORTARANGESH),
		LEFTEDGE, BOTEDGE, RIGHTEDGE, TOPEDGE}};
static TECH_POLYGON gen_s_l[] = {					/* layers */
	{LSIMPROBE, 0, 4, FILLEDRECT, BOX, gen_fullbox}};
static TECH_NODES gen_s = {
	x_("Simulation-Probe"),NSIMPROBE,NONODEPROTO,	/* name */
	K10,K10,										/* size */
	1,gen_s_p,										/* ports */
	1,gen_s_l,										/* layers */
	(NPART<<NFUNCTIONSH)|HOLDSTRACE,				/* userbits */
	0,0,0,0,0,0,0,0,0};								/* characteristics */

TECH_NODES *gen_nodeprotos[NODEPROTOCOUNT+1] = {&gen_u, &gen_i,
	&gen_r, &gen_c, &gen_p, &gen_d, &gen_e, &gen_s, ((TECH_NODES *)-1)};

static INTBIG gen_node_widoff[NODEPROTOCOUNT*4] = {0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, K2,K2,K2,K2, 0,0,0,0, 0,0,0,0, 0,0,0,0};

/******************** VARIABLE AGGREGATION ********************/

TECH_VARIABLES gen_variables[] =
{
	/* set general information about the technology */
	{x_("TECH_layer_names"), (CHAR *)gen_layer_names, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_layer_function"), (CHAR *)gen_layer_function, 0.0,
		VINTEGER|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{x_("TECH_node_width_offset"), (CHAR *)gen_node_widoff, 0.0,
		VFRACT|VDONTSAVE|VISARRAY|((NODEPROTOCOUNT*4)<<VLENGTHSH)},

	/* set information for the I/O tool */
	{x_("IO_cif_layer_names"), (CHAR *)gen_cif_layers, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},

	/* set information for the USER tool */
	{x_("USER_layer_letters"), (CHAR *)gen_layer_letters, 0.0,
		VSTRING|VDONTSAVE|VISARRAY|(MAXLAYERS<<VLENGTHSH)},
	{NULL, NULL, 0.0, 0}
};

/******************** ROUTINES ********************/

BOOLEAN gen_initprocess(TECHNOLOGY *tech, INTBIG pass)
{
	switch (pass)
	{
		case 0:
			/* initialize the technology variable */
			gen_tech = tech;
			break;

		case 1:
			/* create list of ALL arcs (now and when technologies change) */
			registertechnologycache(gen_makeunivlist, 0, 0);

			gen_univpinprim = getnodeproto(x_("Generic:Universal-Pin"));
			gen_invispinprim = getnodeproto(x_("Generic:Invisible-Pin"));
			gen_unroutedpinprim = getnodeproto(x_("Generic:Unrouted-Pin"));
			gen_cellcenterprim = getnodeproto(x_("Generic:Facet-Center"));
			gen_portprim = getnodeproto(x_("Generic:Port"));
			gen_drcprim = getnodeproto(x_("Generic:DRC-Node"));
			gen_essentialprim = getnodeproto(x_("Generic:Essential-Bounds"));
			gen_simprobeprim = getnodeproto(x_("Generic:Simulation-Probe"));

			gen_universalarc = getarcproto(x_("Generic:Universal"));
			gen_invisiblearc = getarcproto(x_("Generic:Invisible"));
			gen_unroutedarc = getarcproto(x_("Generic:Unrouted"));
			break;

		case 2:
			/* set colors properly */
			gen_gl_lay.col = el_colmengly;
			break;
	}
	return(FALSE);
}

void gen_termprocess(void)
{
	if (gen_upconn != 0) efree((CHAR *)gen_upconn);
}

/*
 * routine to update the connecitivity list for universal and invisible pins so that
 * they can connect to ALL arcs.  This is called at initialization and again
 * whenever the number of technologies changes
 */
void gen_makeunivlist(void)
{
	REGISTER INTBIG tot;
	REGISTER ARCPROTO *ap;
	REGISTER TECHNOLOGY *t;
	static BOOLEAN first = TRUE;

	/* count the number of arcs in all technologies */
	tot = 0;
	for(t = el_technologies; t != NOTECHNOLOGY; t = t->nexttechnology)
	{
		for(ap = t->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto) tot++;
	}

	/* make an array for each arc */
	if (gen_upconn != 0) efree((CHAR *)gen_upconn);
	gen_upconn = (ARCPROTO **)emalloc(((tot+2) * (sizeof (ARCPROTO *))), gen_tech->cluster);
	if (gen_upconn == 0) return;

	/* fill the array */
	tot = 0;
	if (first)
		gen_upconn[tot++] = (ARCPROTO *)0;
	for(t = el_technologies; t != NOTECHNOLOGY; t = t->nexttechnology)
	{
		for(ap = t->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			gen_upconn[tot++] = ap;
	}
	gen_upconn[tot] = NOARCPROTO;

	/* store the array in this technology */
	if (first)
	{
		/* on first entry, load the local descriptor */
		gen_u_p[0].portarcs = (INTBIG *)gen_upconn;
		gen_i_p[0].portarcs = (INTBIG *)gen_upconn;
	} else
	{
		/* after initialization, simply change the connection array */
		gen_univpinprim->firstportproto->connects = gen_upconn;
		gen_invispinprim->firstportproto->connects = gen_upconn;
	}
	first = FALSE;
}

INTBIG gen_nodepolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win)
{
	return(gen_intnodepolys(ni, reasonable, win, &tech_oneprocpolyloop, &gen_oneprocpolyloop));
}

INTBIG gen_intnodepolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win, POLYLOOP *pl,
	GENPOLYLOOP *genpl)
{
	REGISTER INTBIG pindex, count;
	REGISTER INTBIG start, end;
	REGISTER INTBIG lambda;
	static INTBIG portanglekey = 0, portrangekey = 0;
	REGISTER VARIABLE *var, *var2;

	pindex = ni->proto->primindex;
	count = gen_nodeprotos[pindex-1]->layercount;
	if (pindex == NUNIV || pindex == NINVIS || pindex == NUNROUTED)
	{
		if (tech_pinusecount(ni, win)) count = 0;
		if (pindex == NINVIS)
		{
			genpl->inbits = LAYERO;
			genpl->incol = GRAY;
			genpl->intbits = LAYERO;
			genpl->intcol = BLACK;
			var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, art_colorkey);
			if (var != NOVARIABLE)
			{
				switch (var->addr)
				{
					case LAYERT1: genpl->incol = COLORT1;  genpl->inbits = LAYERT1;  break;
					case LAYERT2: genpl->incol = COLORT2;  genpl->inbits = LAYERT2;  break;
					case LAYERT3: genpl->incol = COLORT3;  genpl->inbits = LAYERT3;  break;
					case LAYERT4: genpl->incol = COLORT4;  genpl->inbits = LAYERT4;  break;
					case LAYERT5: genpl->incol = COLORT5;  genpl->inbits = LAYERT5;  break;
					default:
						if ((var->addr&(LAYERG|LAYERH|LAYEROE)) == LAYEROE)
							genpl->inbits = LAYERO; else
								genpl->inbits = LAYERA;
						genpl->incol = var->addr;
						break;
				}
				genpl->intbits = genpl->inbits;
				genpl->intcol = genpl->incol;
			}
		}
	} else if (pindex == NPORT)
	{
		if (portanglekey == 0) portanglekey = makekey(x_("EDTEC_portangle"));
		if (portrangekey == 0) portrangekey = makekey(x_("EDTEC_portrange"));

		/* port node becomes a cross when it is 1x1 */
		lambda = lambdaofnode(ni);
		if (ni->highx-ni->lowx == lambda*2 && ni->highy-ni->lowy == lambda*2)
			gen_p_l[0].style = CROSS; else gen_p_l[0].style = CLOSED;

		var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, portanglekey);
		var2 = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, portrangekey);
		if (var == NOVARIABLE || var2 == NOVARIABLE) count--; else
		{
			start = (var->addr - var2->addr) * 10;
			end = (var->addr + var2->addr) * 10;
			while (start < 0) start += 3600;
			while (start > 3600) start -= 3600;
			while (end < 0) end += 3600;
			while (end > 3600) end -= 3600;
			genpl->portbox1 = mult(cosine(start), K2);
			genpl->portbox3 = mult(sine(start), K2);
			genpl->portbox9 = mult(cosine(end), K2);
			genpl->portbox11 = mult(sine(end), K2);
		}
	}

	/* add in displayable variables */
	pl->realpolys = count;
	count += tech_displayablenvars(ni, pl->curwindowpart, pl);
	if (reasonable != 0) *reasonable = count;
	return(count);
}

void gen_shapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly)
{
	gen_intshapenodepoly(ni, box, poly, &tech_oneprocpolyloop, &gen_oneprocpolyloop);
}

void gen_intshapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl, GENPOLYLOOP *genpl)
{
	REGISTER TECH_POLYGON *lay;
	REGISTER INTBIG pindex, count, i, j;
	REGISTER INTBIG x, y, xoff, yoff, cross, lambda;
	REGISTER VARIABLE *var;

	/* handle displayable variables */
	pindex = ni->proto->primindex;
	if (box >= pl->realpolys)
	{
		(void)tech_filldisplayablenvar(ni, poly, pl->curwindowpart, 0, pl);
		if (pindex == NINVIS)
		{
			/* code cannot be called by multiple procesors: uses globals */
			NOT_REENTRANT;

			gen_int_lay.bits = genpl->intbits;
			gen_int_lay.col = genpl->intcol;
			poly->desc = &gen_int_lay;
		}
		return;
	}

	lambda = lambdaofnode(ni);
	lay = &gen_nodeprotos[pindex - 1]->layerlist[box];
	if (lay->portnum < 0) poly->portproto = NOPORTPROTO; else
		poly->portproto = ni->proto->tech->nodeprotos[pindex-1]->portlist[lay->portnum].addr;

	switch (pindex)
	{
		case NINVIS:
			/* code cannot be called by multiple procesors: uses globals */
			NOT_REENTRANT;

			/* invisible pins take programmable appearance */
			gen_in_lay.bits = genpl->inbits;
			gen_in_lay.col = genpl->incol;
			break;

		case NPORT:
			/* code cannot be called by multiple procesors: uses globals */
			NOT_REENTRANT;

			/* ports take precomputed range indicators */
			gen_portabox[1] = genpl->portbox1;
			gen_portabox[3] = genpl->portbox3;
			gen_portabox[9] = genpl->portbox9;
			gen_portabox[11] = genpl->portbox11;
			break;

		case NUNIV:
			/* universal pins may have trace information */
			var = gettrace(ni);
			if (var != NOVARIABLE)
			{
				/* make sure polygon can hold this description */
				count = getlength(var) / 2;
				j = count*4 + (count-1)*2;
				if (poly->limit < j) (void)extendpolygon(poly, j);

				/* fill the polygon */
				xoff = (ni->highx + ni->lowx) / 2;
				yoff = (ni->highy + ni->lowy) / 2;
				cross = lambda / 4;
				j = 0;
				for(i=0; i<count; i++)
				{
					x = ((INTBIG *)var->addr)[i*2];
					y = ((INTBIG *)var->addr)[i*2+1];
					poly->xv[j] = x-cross+xoff;
					poly->yv[j] = y-cross+yoff;   j++;
					poly->xv[j] = x+cross+xoff;
					poly->yv[j] = y+cross+yoff;   j++;
					poly->xv[j] = x-cross+xoff;
					poly->yv[j] = y+cross+yoff;   j++;
					poly->xv[j] = x+cross+xoff;
					poly->yv[j] = y-cross+yoff;   j++;
				}
				for(i=1; i<count; i++)
				{
					poly->xv[j] = ((INTBIG *)var->addr)[(i-1)*2]+xoff;
					poly->yv[j] = ((INTBIG *)var->addr)[(i-1)*2+1]+yoff;   j++;
					poly->xv[j] = ((INTBIG *)var->addr)[i*2]+xoff;
					poly->yv[j] = ((INTBIG *)var->addr)[i*2+1]+yoff;   j++;
				}

				/* add in peripheral information */
				poly->layer = lay->layernum;
				poly->style = VECTORS;
				poly->count = j;
				poly->desc = gen_layers[poly->layer];
				return;
			}
			break;
	}

	/* nontrace pins draw the normal way */
	tech_fillpoly(poly, lay, ni, lambda, FILLED);
	poly->desc = gen_layers[poly->layer];
}

INTBIG gen_allnodepolys(NODEINST *ni, POLYLIST *plist, WINDOWPART *win, BOOLEAN onlyreasonable)
{
	REGISTER INTBIG tot, j;
	INTBIG reasonable;
	REGISTER NODEPROTO *np;
	REGISTER POLYGON *poly;
	POLYLOOP mypl;
	GENPOLYLOOP mygenpl;

	np = ni->proto;
	mypl.curwindowpart = win;
	tot = gen_intnodepolys(ni, &reasonable, win, &mypl, &mygenpl);
	if (onlyreasonable) tot = reasonable;
	if (mypl.realpolys < tot) tot = mypl.realpolys;
	if (ensurepolylist(plist, tot, db_cluster)) return(-1);
	for(j = 0; j < tot; j++)
	{
		poly = plist->polygons[j];
		poly->tech = gen_tech;
		gen_intshapenodepoly(ni, j, poly, &mypl, &mygenpl);
	}
	return(tot);
}

void gen_shapeportpoly(NODEINST *ni, PORTPROTO *pp, POLYGON *poly, XARRAY trans,
	BOOLEAN purpose)
{
	REGISTER INTBIG pindex, style;
	Q_UNUSED( purpose );

	pindex = ni->proto->primindex;
	if (pindex == NSIMPROBE) style = FILLED; else
		style = OPENED;
	tech_fillportpoly(ni, pp, poly, trans, gen_nodeprotos[pindex-1], style, lambdaofnode(ni));
}
