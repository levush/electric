/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrmisc.c
 * User interface tool: miscellaneous control
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
#include "efunction.h"
#include "edialogs.h"
#include "usr.h"
#include "usreditemacs.h"
#include "usreditpac.h"
#include "usrtrack.h"
#include "tecgen.h"
#include "tecart.h"
#include "sim.h"
#include <math.h>

#define ALLTEXTWIDTH	70		/* width of side menu when it is all text */

/******************** INITIALIZATION ********************/

#define INITIALMENUX    18	/* default height of component menu */
#define INITIALMENUY    2	/* default width of component menu */

/*
 * default tablet state: the table below shows the set of commands
 * that will be bound to the tablet buttons.  If there is a button
 * with the name in "us_tablet[i].but[0]" or "us_tablet[i].but[1]"
 * then that button will be bound to the command "us_tablet[i].com".
 * If no tablet button exists for an entry "i", then the command
 * there will be bound to the key "us_tablet[i].key".  Thus, tablets
 * with varying numbers of buttons can be handled by placing commands
 * on the keys if they don't fit on the buttons.
 */
typedef struct
{
	CHAR   *but[3];		/* the button names for this command */
	CHAR   *key;		/* the key name for this command */
	CHAR   *com;		/* the actual command */
	INTBIG  count;		/* number of parameters to the command */
	CHAR   *args[2];	/* parameters to the command */
	BOOLEAN used;		/* set when the command is bound */
} INITBUTTONS;

static INITBUTTONS us_initialbutton[] =
{
	/* left/middle/right (white/yellow/blue) are basic commands */
	{{x_("LEFT"),x_("WHITE"),x_("BUTTON")},x_("f"), x_("find"),    2, {x_("port"),          x_("extra-info")}, FALSE},
	{{x_("RIGHT"),x_("YELLOW"),x_("")},    x_("m"), x_("move"),    0, {x_(""),              x_("")},           FALSE},
	{{x_("MIDDLE"),x_("BLUE"),x_("")},     x_("n"), x_("create"),  0, {x_(""),              x_("")},           FALSE},

	/* on four-button puck, add one more basic command */
	{{x_("GREEN"),x_(""),x_("")},          x_("o"), x_("find"),    2, {x_("another"),       x_("port")},       FALSE},

	/* on mice with shift-buttons, add in others still */
	{{x_("SLEFT"),x_(""),x_("")},          x_(""),  x_("find"),    1, {x_("more"),          x_("")},           FALSE},
	{{x_("SRIGHT"),x_(""),x_("")},         x_(""),  x_("var"),     2, {x_("textedit"),      x_("~")},          FALSE},
	{{x_("SMIDDLE"),x_(""),x_("")},        x_(""),  x_("create"),  1, {x_("join-angle"),    x_("")},           FALSE},
	{{NULL, NULL, NULL}, NULL, NULL, 0, {NULL, NULL}, FALSE} /* 0 */
};

/*
 * default keyboard state: the table below shows the set of commands
 * that will be bound to the keyboard keys.
 */
typedef struct
{
	CHAR  *key;
	CHAR  *command;
	INTBIG count;
	CHAR  *args[2];
} INITKEYS;

static INITKEYS us_initialkeyboard[] =
{
	{x_("-"), x_("telltool"), 1, {x_("user"), x_("")}},
	{NULL, NULL, 0, {NULL, NULL}} /* 0 */
};

/*
 * When information, detected during broadcast, evokes a reaction that causes
 * change, that change must be queued until the next slice.  For example:
 * deletion of the variable associated with a text window.
 * These routines queue the changes and then execute them when requested
 */
#define NOUBCHANGE ((UBCHANGE *)-1)
#define UBKILLFM          1		/* remove cell_message variable */
#define UBNEWFC           2		/* add cell-center */
#define UBSPICEPARTS      3		/* check for new spice parts package */
#define UBTECEDDELLAYER   4		/* deleted a technology-edit layer cell */
#define UBTECEDDELNODE    5		/* deleted a technology-edit node cell */
#define UBTECEDRENAME     6		/* renamed a technology-edit cell */
#define UBTOOLISON        7		/* tool was turned on */

typedef struct Iubchange
{
	INTBIG     change;		/* type of change */
	void      *object;		/* object that is being changed */
	void      *parameter;	/* parameter that is being changed */
	struct Iubchange *nextubchange;
} UBCHANGE;
static UBCHANGE *us_ubchangefree = NOUBCHANGE;
static UBCHANGE *us_ubchanges = NOUBCHANGE;

static NODEPROTO *us_layouttextprim;
static INTBIG    *us_printcolordata = 0;

/* prototypes for local routines */
static void       us_splitwindownames(CHAR*, CHAR*, CHAR*, CHAR*, CHAR*);
static BOOLEAN    us_newubchange(INTBIG change, void *object, void *parameter);
static void       us_freeubchange(UBCHANGE*);
static BOOLEAN    us_pointonexparc(INTBIG cx, INTBIG cy, INTBIG sx, INTBIG sy, INTBIG ex, INTBIG ey, INTBIG x, INTBIG y);
static void       us_rotatedescriptArb(GEOM *geom, UINTBIG *descript, BOOLEAN invert);
static void       us_scanquickkeys(POPUPMENU *pm, CHAR **quickkeylist, INTBIG quickkeycount, BOOLEAN *warnofchanges);
static void       us_layouttextpolygon(INTBIG layer, TECHNOLOGY *tech, INTBIG *x, INTBIG *y, INTBIG count);
static INTBIG     us_inheritaddress(INTBIG addr, INTBIG type, VARIABLE *var);
static NODEPROTO *us_findcellinotherlib(NODEPROTO *cell, LIBRARY *lib);
static void       us_correctxlibref(VARIABLE **firstvar, INTSML *numvar, LIBRARY *oldlib, LIBRARY *newlib);
static void       us_adjustpopupmenu(POPUPMENU *pm, INTBIG pindex);
static void       us_inheritexportattributes(PORTPROTO *pp, NODEINST *ni, NODEPROTO *np);
static void       us_inheritcellattribute(VARIABLE *var, NODEINST *ni, NODEPROTO *np, NODEINST *icon);
static void       us_advancecircuittext(CHAR *search, INTBIG bits);
static INTBIG     us_stringinstring(CHAR *string, CHAR *search, INTBIG bits);

/*
 * Routine to free all memory associated with this module.
 */
void us_freemiscmemory(void)
{
	UBCHANGE *ubc;

	if (us_printcolordata != 0)
		efree((CHAR *)us_printcolordata);
	while(us_ubchanges != NOUBCHANGE)
	{
		ubc = us_ubchanges;
		us_ubchanges = ubc->nextubchange;
		us_freeubchange(ubc);
	}
	while(us_ubchangefree != NOUBCHANGE)
	{
		ubc = us_ubchangefree;
		us_ubchangefree = ubc->nextubchange;
		efree((CHAR *)ubc);
	}
}

/*
 * initialization routine to bind keys and buttons to functions
 * returns true upon error
 */
BOOLEAN us_initialbinding(void)
{
	REGISTER INTBIG i, k, menusave, menux, menuy;
	INTBIG j;
	CHAR si[50], sj[20], *par[MAXPARS+7];
	REGISTER CHAR **temp;

	/* make the variables with the bindings */
	i = maxi(maxi(NUMKEYS, NUMBUTS), INITIALMENUX*INITIALMENUY);
	temp = (CHAR **)emalloc(i * (sizeof (CHAR *)), el_tempcluster);
	if (temp == 0) return(TRUE);
	temp[0] = x_("a/");
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_binding_keys_key, (INTBIG)temp,
		VSTRING|VISARRAY|VDONTSAVE|(1<<VLENGTHSH));
	for(j=0; j<i; j++) temp[j] = x_("");
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_binding_buttons_key, (INTBIG)temp,
		VSTRING|VISARRAY|VDONTSAVE|(NUMBUTS<<VLENGTHSH));
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_binding_menu_key, (INTBIG)temp,
		VSTRING|VISARRAY|VDONTSAVE|((INITIALMENUX*INITIALMENUY)<<VLENGTHSH));
	efree((CHAR *)temp);

	/* bind the keys */
	for(i=0; us_initialkeyboard[i].key != 0; i++)
	{
		par[0] = x_("set");   par[1] = x_("key");
		par[2] = us_initialkeyboard[i].key;
		par[3] = us_initialkeyboard[i].command;
		for(j=0; j<us_initialkeyboard[i].count; j++)
			par[j+4] = us_initialkeyboard[i].args[j];
		us_bind(us_initialkeyboard[i].count+4, par);
	}

	/* bind the mouse commands that fit on the mouse */
	for(i=0; i<buttoncount(); i++)
	{
		(void)estrcpy(si, buttonname(i, &j));
		for(j=0; us_initialbutton[j].but[0] != 0; j++)
			if (!us_initialbutton[j].used)
		{
			for(k=0; k<3; k++)
				if (namesame(si, us_initialbutton[j].but[k]) == 0)
			{
				par[0] = x_("set");   par[1] = x_("button");   par[2] = si;
				par[3] = us_initialbutton[j].com;
				for(k=0; k<us_initialbutton[j].count; k++)
					par[k+4] = us_initialbutton[j].args[k];
				us_bind(us_initialbutton[j].count+4, par);
				us_initialbutton[j].used = TRUE;
				break;
			}
		}
	}

	/* now bind those mouse commands that can't fit on the mouse */
	for(j=0; us_initialbutton[j].but[0] != 0; j++)
		if (!us_initialbutton[j].used && *us_initialbutton[j].key != 0)
	{
		par[0] = x_("set");   par[1] = x_("key");   par[2] = us_initialbutton[j].key;
		par[3] = us_initialbutton[j].com;
		for(k=0; k<us_initialbutton[j].count; k++)
			par[k+4] = us_initialbutton[j].args[k];
		us_bind(us_initialbutton[j].count+4, par);
	}

	/* bind the component menu entries to all "getproto" */
	if (us_menupos <= 1)
	{
		menux = INITIALMENUX;
		menuy = INITIALMENUY;
	} else
	{
		menux = INITIALMENUY;
		menuy = INITIALMENUX;
	}
	us_setmenusize(menux, menuy, us_menupos, FALSE);
	menusave = us_tool->toolstate&MENUON;   us_tool->toolstate &= ~MENUON;
	for(i=0; i<(INITIALMENUX*INITIALMENUY); i++)
	{
		par[0] = x_("set");   par[1] = x_("menu");
		(void)esnprintf(si, 50, x_("%ld"), i%INITIALMENUX);
		(void)esnprintf(sj, 20, x_("%ld"), i/INITIALMENUX);
		if (us_menupos <= 1)
		{
			par[2] = sj;  par[3] = si;
		} else
		{
			par[2] = si;  par[3] = sj;
		}
		par[4] = x_("rem");
		par[5] = x_("getproto");
		us_bind(6, par);
	}

	/* now fill in the "getproto" commands properly */
	us_setmenunodearcs();

	if (menusave != 0) us_tool->toolstate |= MENUON; else
		us_tool->toolstate &= ~MENUON;
	return(FALSE);
}

/*
 * routine to determine for technology "tech" which node and arc prototypes
 * have opaque layers and set the bits in the prototype->userbits.
 * The rules for layer orderings are that the transparent layers
 * must come first followed by the opaque layers.  The field that is
 * set in the "userbits" is then the index of the first opaque layer.
 */
void us_figuretechopaque(TECHNOLOGY *tech)
{
	REGISTER INTBIG j, k;
	REGISTER NODEPROTO *np;
	REGISTER ARCPROTO *ap;
	REGISTER NODEINST *ni;
	NODEINST node;
	REGISTER ARCINST *ai;
	ARCINST arc;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if ((np->userbits&NHASOPA) != 0) continue;
		np->userbits &= ~NHASOPA;
		ni = &node;   initdummynode(ni);
		ni->proto = np;
		ni->lowx = np->lowx;   ni->highx = np->highx;
		ni->lowy = np->lowy;   ni->highy = np->highy;
		j = nodepolys(ni, 0, NOWINDOWPART);
		for(k=0; k<j; k++)
		{
			shapenodepoly(ni, k, poly);
			if (poly->desc->bits == LAYERN) continue;
			if ((poly->desc->bits & ~(LAYERT1|LAYERT2|LAYERT3|LAYERT4|LAYERT5)) == 0)
			{
				/* transparent layer found, make sure it is at start */
				if ((np->userbits&NHASOPA) != 0)
					ttyputerr(_("%s: node %s has layers out of order!"), tech->techname, np->protoname);
				continue;
			}

			/* opaque layer found, mark its index if it is the first */
			if ((np->userbits&NHASOPA) == 0)
				np->userbits = (np->userbits & ~NFIRSTOPA) | (k << NFIRSTOPASH);
			np->userbits |= NHASOPA;
		}
	}
	for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
	{
		ap->userbits &= ~AHASOPA;
		ai = &arc;   initdummyarc(ai);
		ai->proto = ap;
		ai->userbits = ISDIRECTIONAL;
		j = arcpolys(ai, NOWINDOWPART);
		for(k=0; k<j; k++)
		{
			shapearcpoly(ai, k, poly);
			if (poly->desc->bits == LAYERN) continue;
			if ((poly->desc->bits & ~(LAYERT1|LAYERT2|LAYERT3|LAYERT4|LAYERT5)) == 0)
			{
				/* transparent layer found, make sure it is at start */
				if ((ap->userbits&AHASOPA) != 0)
					ttyputerr(_("Arc %s:%s has layers out of order!"), tech->techname, ap->protoname);
				continue;
			}

			/* opaque layer found, mark its index if it is the first */
			if ((ap->userbits&AHASOPA) == 0)
				ap->userbits = (ap->userbits & ~AFIRSTOPA) | (k << AFIRSTOPASH);
			ap->userbits |= AHASOPA;
		}
	}
}

/*
 * Routine to recompute the "NINVISIBLE" and "AINVISIBLE" bits on node and arc protos
 * according to whether or not all layers are invisible.
 */
void us_figuretechselectability(void)
{
	REGISTER INTBIG j, k;
	REGISTER TECHNOLOGY *tech;
	REGISTER NODEPROTO *np;
	REGISTER ARCPROTO *ap;
	REGISTER NODEINST *ni;
	NODEINST node;
	REGISTER ARCINST *ai;
	ARCINST arc;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			np->userbits &= ~NINVISIBLE;
			ni = &node;   initdummynode(ni);
			ni->proto = np;
			ni->lowx = np->lowx;   ni->highx = np->highx;
			ni->lowy = np->lowy;   ni->highy = np->highy;
			j = nodepolys(ni, 0, NOWINDOWPART);
			for(k=0; k<j; k++)
			{
				shapenodepoly(ni, k, poly);
				if (poly->desc->bits == LAYERN) continue;
				if ((poly->desc->colstyle&INVISIBLE) == 0) break;
			}
			if (k >= j) np->userbits |= NINVISIBLE;
		}
		for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
		{
			ap->userbits &= ~AINVISIBLE;
			ai = &arc;   initdummyarc(ai);
			ai->proto = ap;
			j = arcpolys(ai, NOWINDOWPART);
			for(k=0; k<j; k++)
			{
				shapearcpoly(ai, k, poly);
				if (poly->desc->bits == LAYERN) continue;
				if ((poly->desc->colstyle&INVISIBLE) == 0) break;
			}
			if (k >= j) ap->userbits |= AINVISIBLE;
		}
	}
}

/*
 * routine to examine the current window structure and fit their sizes
 * to the screen.  If "placemenu" is nonzero, set the menu location too.
 */
void us_windowfit(WINDOWFRAME *whichframe, BOOLEAN placemenu, INTBIG scaletofit)
{
	REGISTER WINDOWPART *w;
	REGISTER INTBIG lowy, highy, ulx, uhx, uly, uhy, drawlx, drawhx, drawly, drawhy,
		mtop, mleft, alltext;
	INTBIG swid, shei, mwid, mhei, pwid;
	REGISTER INTBIG i, total, newwid, newhei, offx, offy;
	INTBIG slx, shx, sly, shy;
	REGISTER WINDOWFRAME *frame;
	REGISTER VARIABLE *var;
	COMMANDBINDING commandbinding;

	for(frame = el_firstwindowframe; frame != NOWINDOWFRAME; frame = frame->nextwindowframe)
	{
		if (whichframe != NOWINDOWFRAME && whichframe != frame) continue;
		getwindowframesize(frame, &swid, &shei);
		lowy = 0;   highy = shei - 1;

		/* presume that there is no menu */
		drawlx = 0;      drawhx = swid-1;
		drawly = lowy;   drawhy = highy;

		/* if there is a menu, figure it out */
		if ((us_tool->toolstate&MENUON) != 0)
		{
			/* see if the menu is all text */
			alltext = 0;
			var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_menu_key);
			if (var != NOVARIABLE)
			{
				total = us_menux*us_menuy;
				for(i=0; i<total; i++)
				{
					us_parsebinding(((CHAR **)var->addr)[i], &commandbinding);
					if (*commandbinding.command == 0 || commandbinding.nodeglyph != NONODEPROTO ||
						commandbinding.arcglyph != NOARCPROTO)
					{
						us_freebindingparse(&commandbinding);
						break;
					}
					us_freebindingparse(&commandbinding);
				}
				if (i >= total) alltext = 1;
			}
			if (us_menuframe == NOWINDOWFRAME)
			{
				/* menus come out of the only editor window */
				switch (us_menupos)
				{
					case 0:		/* menu at top */
						us_menuxsz = us_menuysz = swid / us_menux;
						us_menulx = (swid - us_menux*us_menuxsz)/2;
						us_menuhx = swid - us_menulx;
						us_menuly = highy - us_menuysz*us_menuy;
						us_menuhy = highy;
						drawlx = 0;      drawhx = swid-1;
						drawly = lowy;   drawhy = us_menuly-1;
						break;
					case 1:		/* menu at bottom */
						us_menuxsz = us_menuysz = swid / us_menux;
						us_menulx = (swid - us_menux*us_menuxsz)/2;
						us_menuhx = swid - us_menulx;
						us_menuly = lowy;
						us_menuhy = lowy + us_menuysz * us_menuy;
						drawlx = 0;             drawhx = swid-1;
						drawly = us_menuhy+1;   drawhy = highy;
						break;
					case 2:		/* menu on left */
						us_menuxsz = us_menuysz = (highy-lowy) / us_menuy;

						/* if the menu is all text, allow nonsquare menus */
						if (alltext != 0) us_menuxsz = ALLTEXTWIDTH / us_menux;

						us_menulx = 0;
						us_menuhx = us_menuxsz * us_menux;
						us_menuly = ((highy-lowy) - us_menuy*us_menuysz)/2 + lowy;
						us_menuhy = us_menuly + us_menuy*us_menuysz;
						drawlx = us_menuhx+1;   drawhx = swid-1;
						drawly = lowy;          drawhy = highy;
						break;
					case 3:		/* menu on right */
						us_menuxsz = us_menuysz = (highy-lowy) / us_menuy;

						/* if the menu is all text, allow nonsquare menus */
						if (alltext != 0) us_menuxsz = ALLTEXTWIDTH / us_menux;

						us_menulx = swid - us_menuxsz * us_menux;
						us_menuhx = swid-1;
						us_menuly = ((highy-lowy) - us_menuy*us_menuysz)/2 + lowy;
						us_menuhy = us_menuly + us_menuy*us_menuysz;
						drawlx = 0;      drawhx = us_menulx-1;
						drawly = lowy;   drawhy = highy;
						break;
				}
			} else
			{
				/* floating menu window */
				if (frame == us_menuframe && placemenu)
				{
					getpaletteparameters(&mwid, &mhei, &pwid);
					switch (us_menupos)
					{
						case 0:		/* menu at top */
						case 1:		/* menu at bottom */
							us_menuxsz = us_menuysz = mwid / us_menux;
							if (us_menuysz * us_menuy > pwid)
								us_menuxsz = us_menuysz = pwid / us_menuy;
							us_menulx = 0;
							us_menuhx = us_menux * us_menuxsz;
							us_menuly = 0;
							us_menuhy = us_menuy * us_menuysz;
							mleft = 0;
							if (us_menupos == 0)
							{
								/* menu on the top */
								mtop = 1;
							} else
							{
								/* menu on the bottom */
								mtop = mhei - us_menuysz*us_menuy - 3;
							}
							break;

						case 2:		/* menu on left */
						case 3:		/* menu on right */
							/* determine size of menu entries */
							us_menuxsz = us_menuysz = mhei / us_menuy;
							if (us_menuxsz * us_menux > pwid)
								us_menuxsz = us_menuysz = pwid / us_menux;

							/* if the menu is all text, allow nonsquare menus */
							if (alltext != 0) us_menuxsz = ALLTEXTWIDTH / us_menux;

							/* compute menu parameters */
							us_menuly = 0;
							us_menuhy = us_menuy * us_menuysz;
							us_menulx = 0;
							us_menuhx = us_menux * us_menuxsz;
							mtop = 0;
							if (us_menupos == 2)
							{
								/* menu on the left */
								mleft = 0;
							} else
							{
								/* menu on the right */
								mleft = mwid - us_menuxsz * us_menux - 2;
							}
							break;
						default:
							mtop = mleft = 0;
					}
					sizewindowframe(us_menuframe, us_menuhx-us_menulx, us_menuhy-us_menuly);
					movewindowframe(us_menuframe, mleft, mtop);
				}
			}
		}

		/* now fit the windows in the remaining space */
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			/* this window must be on the right frame */
			if (w->frame != frame) continue;

			/* entire window is handled simply */
			if (estrcmp(w->location, x_("entire")) == 0)
			{
				ulx = drawlx;              uhx = drawhx;
				uly = drawly;              uhy = drawhy;
			} else if (estrncmp(w->location, x_("top"), 3) == 0)
			{
				ulx = drawlx;              uhx = drawhx;
				uly = (drawhy-drawly)*(100-w->vratio)/100;
				uhy = drawhy;
			} else if (estrncmp(w->location, x_("bot"), 3) == 0)
			{
				ulx = drawlx;              uhx = drawhx;
				uly = drawly;              uhy = (drawhy-drawly)*w->vratio/100;
			} else if (estrcmp(w->location, x_("left")) == 0)
			{
				ulx = drawlx;              uhx = (drawhx-drawlx)*w->hratio/100;
				uly = drawly;              uhy = drawhy;
			} else if (estrcmp(w->location, x_("right")) == 0)
			{
				ulx = (drawhx-drawlx)*(100-w->hratio)/100;
				uhx = drawhx;
				uly = drawly;              uhy = drawhy;
			} else ulx = uhx = uly = uhy = 0;

			/* subdivide for fractions of half windows */
			i = 3;
			while (w->location[i] == '-')
			{
				switch (w->location[i+1])
				{
					case 'l': uhx = (uhx - ulx)*w->hratio/100;         break;
					case 'r': ulx = (uhx - ulx)*(100-w->hratio)/100;   break;
					case 't': uly = (uhy - uly)*(100-w->vratio)/100;   break;
					case 'b': uhy = (uhy - uly)*w->vratio/100;         break;
				}
				i += 2;
			}
			if (estrcmp(w->location, x_("entire")) != 0)
			{
				ulx++;   uhx--;   uly++;   uhy--;
			}

			/* make sure window has some size */
			if (ulx >= uhx) uhx = ulx + 1;
			if (uly >= uhy) uhy = uly + 1;

			/* make room for border if in a mode */
			if ((w->state&WINDOWMODE) != 0)
			{
				ulx += WINDOWMODEBORDERSIZE;   uhx -= WINDOWMODEBORDERSIZE;
				uly += WINDOWMODEBORDERSIZE;   uhy -= WINDOWMODEBORDERSIZE;
			}

			/* make room for sliders if a display window */
			if ((w->state&WINDOWTYPE) == DISPWINDOW)
			{
				uhx -= DISPLAYSLIDERSIZE;
				uly += DISPLAYSLIDERSIZE;
			}
			if ((w->state&WINDOWTYPE) == WAVEFORMWINDOW)
			{
				ulx += DISPLAYSLIDERSIZE;
				uly += DISPLAYSLIDERSIZE;
			}

			/* update if the extent changed */
			if (w->uselx != ulx || w->usehx != uhx ||
				w->usely != uly || w->usehy != uhy)
			{
				/* set the window extent */
				w->uselx = ulx;      w->usehx = uhx;
				w->usely = uly;      w->usehy = uhy;

				/* now adjust the database extents of the window */
				slx = w->screenlx;   shx = w->screenhx;
				sly = w->screenly;   shy = w->screenhy;
				if (scaletofit > 0)
				{
					us_squarescreen(w, NOWINDOWPART, FALSE, &slx, &shx, &sly, &shy, 0);
				} else if (scaletofit < 0)
				{
					newwid = (INTBIG)(((float)(uhx - ulx)) / w->scalex + 0.5);
					newhei = (INTBIG)(((float)(uhy - uly)) / w->scaley + 0.5);
					offx = newwid - (shx - slx);
					offy = newhei - (shy - sly);
					slx -= offx / 2;
					shx = slx + newwid;
					sly -= offy / 2;
					shy = sly + newhei;
				}
				w->screenlx = slx;   w->screenhx = shx;
				w->screenly = sly;   w->screenhy = shy;
				computewindowscale(w);
			}
		}
	}
}

/*
 * routine to adjust the actual drawing area of window "win" to account for
 * the appearance or disappearance of the red simulation border
 */
void us_setwindowmode(WINDOWPART *win, INTBIG oldstate, INTBIG newstate)
{
	if ((oldstate&WINDOWMODE) != 0)
	{
		/* was in a mode: remove border */
		win->uselx -= WINDOWMODEBORDERSIZE;   win->usehx += WINDOWMODEBORDERSIZE;
		win->usely -= WINDOWMODEBORDERSIZE;   win->usehy += WINDOWMODEBORDERSIZE;
	}
	if ((newstate&WINDOWMODE) != 0)
	{
		/* now in a mode: add border */
		win->uselx += WINDOWMODEBORDERSIZE;   win->usehx -= WINDOWMODEBORDERSIZE;
		win->usely += WINDOWMODEBORDERSIZE;   win->usehy -= WINDOWMODEBORDERSIZE;
	}
	win->state = newstate;
}

/*
 * routine to tell the names of the windows that result when the window
 * with name "w" is split.  The strings "hwind1" and "hwind2" are filled
 * with the names if the window is split horizontally.  The strings "vwind1"
 * and "vwind2" are filled with the names if the window is split verticaly.
 */
void us_splitwindownames(CHAR *w, CHAR *hwind1, CHAR *hwind2, CHAR *vwind1, CHAR *vwind2)
{
	REGISTER INTBIG i;

	if (estrcmp(w, x_("entire")) == 0)
	{
		(void)estrcpy(hwind1, x_("top"));   (void)estrcpy(hwind2, x_("bottom"));
		(void)estrcpy(vwind1, x_("left"));  (void)estrcpy(vwind2, x_("right"));
		return;
	}
	if (estrcmp(w, x_("top")) == 0)
	{
		(void)estrcpy(hwind1, x_("top-l")); (void)estrcpy(hwind2, x_("top-r"));
		(void)estrcpy(vwind1, x_(""));      (void)estrcpy(vwind2, x_(""));
		return;
	}
	if (estrcmp(w, x_("bottom")) == 0)
	{
		(void)estrcpy(hwind1, x_("bot-l")); (void)estrcpy(hwind2, x_("bot-r"));
		(void)estrcpy(vwind1, x_(""));      (void)estrcpy(vwind2, x_(""));
		return;
	}
	if (estrcmp(w, x_("left")) == 0)
	{
		(void)estrcpy(vwind1, x_("top-l")); (void)estrcpy(vwind2, x_("bot-l"));
		(void)estrcpy(hwind1, x_(""));      (void)estrcpy(hwind2, x_(""));
		return;
	}
	if (estrcmp(w, x_("right")) == 0)
	{
		(void)estrcpy(vwind1, x_("top-r")); (void)estrcpy(vwind2, x_("bot-r"));
		(void)estrcpy(hwind1, x_(""));      (void)estrcpy(hwind2, x_(""));
		return;
	}
	(void)estrcpy(hwind1, w);   (void)estrcpy(hwind2, w);
	(void)estrcpy(vwind1, w);   (void)estrcpy(vwind2, w);
	i = w[estrlen(w)-1];
	if (i == 'l' || i == 'r')
	{
		(void)estrcat(vwind1, x_("-t"));  (void)estrcat(vwind2, x_("-b"));
		(void)estrcpy(hwind1, x_(""));    (void)estrcpy(hwind2, x_(""));
	} else
	{
		(void)estrcat(hwind1, x_("-l"));  (void)estrcat(hwind2, x_("-r"));
		(void)estrcpy(vwind1, x_(""));    (void)estrcpy(vwind2, x_(""));
	}
}

/*
 * Routine to create a new window with whatever method is available on the
 * current machine (new window in its own frame or just a split of the current
 * window).  If "orientation" is 1, make it a horizontal window; if 2,
 * make it vertical.  Otherwise use any configuration.  Prints an error and
 * returns NOWINDOWPART on failure.
 */
WINDOWPART *us_wantnewwindow(INTBIG orientation)
{
	REGISTER WINDOWPART *w;
	WINDOWFRAME *wf;

	if (graphicshas(CANUSEFRAMES) && orientation == 0)
	{
		/* create a default window space on this frame */
		wf = newwindowframe(FALSE, 0);
		if (wf == NOWINDOWFRAME) wf = getwindowframe(FALSE);
		w = newwindowpart(x_("entire"), NOWINDOWPART);
		if (w == NOWINDOWPART)
		{
			us_abortcommand(_("Cannot create new window"));
			return(NOWINDOWPART);
		}
		w->frame = wf;
		w->buttonhandler = DEFAULTBUTTONHANDLER;
		w->charhandler = DEFAULTCHARHANDLER;
		w->changehandler = DEFAULTCHANGEHANDLER;
		w->termhandler = DEFAULTTERMHANDLER;
		w->redisphandler = DEFAULTREDISPHANDLER;
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)w,
			VWINDOWPART|VDONTSAVE);

		/* now draw everything */
		us_drawmenu(0, wf);
		return(w);
	}

	if (us_needwindow()) return(NOWINDOWPART);
	w = us_splitcurrentwindow(orientation, FALSE, 0, 50);
	return(w);
}

/*
 * routine to split the current window into two windows.  If "splitkey" is zero,
 * nature of split is unspecified.  If "splitkey" is 1, split horizontally
 * (only when splitting top window).  If "splitkey" is 2, split vertically.
 * If "fillboth" is true, fill both windows with the contents of the
 * old one.  Otherwise, leave the new current window empty.  Returns the address
 * of the new current window, and the other half in "other" (NOWINDOWPART on error).
 * The partitions are divided according to "percentage" of coverage.
 */
WINDOWPART *us_splitcurrentwindow(INTBIG splitkey, BOOLEAN fillboth, WINDOWPART **other,
	INTBIG percentage)
{
	CHAR wind1[40], wind2[40], vwind1[40], vwind2[40];
	REGISTER CHAR *win1, *win2;
	WINDOWPART windowsave;
	REGISTER WINDOWPART *w2, *w3, *w, *retwin;
	REGISTER INTBIG curwx, curwy, horizsplit;
	REGISTER INTBIG x, y, l;
	REGISTER NODEPROTO *np;

	/* figure out new name of windows */
	if (other != 0) *other = NOWINDOWPART;
	horizsplit = 1;
	us_splitwindownames(el_curwindowpart->location, wind1, wind2, vwind1, vwind2);

	/* use the horizontal window split unless there is none */
	if (*wind1 == 0) win1 = vwind1; else win1 = wind1;
	if (*wind2 == 0) win2 = vwind2; else win2 = wind2;

	/* special case when splitting just one window: which way to split */
	if (estrcmp(el_curwindowpart->location, x_("entire")) == 0)
	{
		/* see if a "horizontal" or "vertical" parameter was given */
		if (splitkey == 2)
		{
			/* vertical window specified explicitly */
			win1 = vwind1;   win2 = vwind2;
			horizsplit = 0;
		} else if (splitkey == 0)
		{
			/* make a guess about window splitting */
			switch (el_curwindowpart->state&WINDOWTYPE)
			{
				case EXPLORERWINDOW:
					win1 = vwind2;   win2 = vwind1;
					horizsplit = 0;
					break;
				default:
					np = el_curwindowpart->curnodeproto;
					if (np != NONODEPROTO)
					{
						curwx = el_curwindowpart->usehx - el_curwindowpart->uselx;
						curwy = el_curwindowpart->usehy - el_curwindowpart->usely;
						x = np->highx - np->lowx;
						y = np->highy - np->lowy;
						l = el_curlib->lambda[el_curtech->techindex];
						if (muldiv(x, curwy/2, l) + muldiv(y, curwx, l) >=
							muldiv(x, curwy, l) + muldiv(y, curwx/2, l))
						{
							/* vertical window makes more sense */
							win1 = vwind1;   win2 = vwind2;
							horizsplit = 0;
						}
					}
					break;
			}
		}
	} else
	{
		l = estrlen(wind1) - 1;
		if (wind1[l] == 'l' || wind1[l] == 'r') horizsplit = 0;
	}

	/* turn off object and window highlighting */
	us_pushhighlight();
	us_clearhighlightcount();
	w = el_curwindowpart;
	copywindowpart(&windowsave, el_curwindowpart);

	/* make two new windows in "w2" and "w3" to replace "el_curwindowpart" */
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)NOWINDOWPART,
		VWINDOWPART|VDONTSAVE);
	startobjectchange((INTBIG)us_tool, VTOOL);
	w2 = newwindowpart(win1, w);
	w3 = newwindowpart(win2, w);
	if (w2 == NOWINDOWPART || w3 == NOWINDOWPART)
	{
		ttyputnomemory();
		return(NOWINDOWPART);
	}

	/* make sure the split is even in the split direction */
	if (horizsplit != 0)
	{
		w2->vratio = percentage;
		w3->vratio = 100 - percentage;
	} else
	{
		w2->hratio = percentage;
		w3->hratio = 100 - percentage;
	}

	/* if splitting an editor window, move the editor and explorer structure */
	if ((w->state&WINDOWTYPE) == TEXTWINDOW || (w->state&WINDOWTYPE) == POPTEXTWINDOW)
	{
		(void)setval((INTBIG)w3, VWINDOWPART, x_("editor"), (INTBIG)w->editor, VADDRESS);
		(void)setval((INTBIG)w, VWINDOWPART, x_("editor"), -1, VADDRESS);
	}
	if ((w->state&WINDOWTYPE) == EXPLORERWINDOW)
	{
		(void)setval((INTBIG)w3, VWINDOWPART, x_("expwindow"), (INTBIG)w->expwindow, VADDRESS);
		(void)setval((INTBIG)w, VWINDOWPART, x_("expwindow"), -1, VADDRESS);
	}

	/* free the current window */
	killwindowpart(w);

	/* zap the returned window if both are not to be filled */
	retwin = w2;
	if (other != 0) *other = w3;
	if ((windowsave.state&WINDOWTYPE) != DISPWINDOW) fillboth = FALSE;
	if (!fillboth)
	{
		retwin->state = (retwin->state & ~(WINDOWTYPE|GRIDON|WINDOWMODE)) | DISPWINDOW;
		retwin->buttonhandler = DEFAULTBUTTONHANDLER;
		retwin->charhandler = DEFAULTCHARHANDLER;
		retwin->changehandler = DEFAULTCHANGEHANDLER;
		retwin->termhandler = DEFAULTTERMHANDLER;
		retwin->redisphandler = DEFAULTREDISPHANDLER;
		retwin->curnodeproto = NONODEPROTO;
		retwin->editor = NOEDITOR;
		retwin->expwindow = (void *)-1;
	}

	/* set the window extents */
	us_windowfit(w2->frame, FALSE, 1);

	/* use former window for scaling */
	w = &windowsave;

	/* windows might have got bigger: see if grid can be drawn */
	if ((w2->state&GRIDTOOSMALL) != 0) us_gridset(w2, GRIDON);
	if ((w2->state&GRIDTOOSMALL) != 0) us_gridset(w3, GRIDON);

	endobjectchange((INTBIG)us_tool, VTOOL);
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)w2,
		VWINDOWPART|VDONTSAVE);

	/* restore all highlighting */
	us_pophighlight(FALSE);
	return(retwin);
}

/*
 * routine to kill a window.  Kills the current window if "thisw" is true.
 * Kills the other window, making the current one larger, if "thisw" is false.
 */
void us_killcurrentwindow(BOOLEAN thisw)
{
	REGISTER WINDOWPART *w1, *w2, *wnew, *w;
	WINDOWPART windowsave;
	REGISTER INTBIG windows;
	CHAR windcomb[40], windother[40], wind1[40], wind2[40], vwind1[40], vwind2[40];
	REGISTER WINDOWFRAME *wf;

	w1 = el_curwindowpart;

	/* if this is the only partition, see if the window frame can be deleted */
	if (estrcmp(w1->location, x_("entire")) == 0)
	{
		if (!graphicshas(CANHAVENOWINDOWS))
		{
			/* disallow deletion if this is the last window */
			windows = 0;
			for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
				if (wf->floating == 0) windows++;
			if (windows <= 1)
			{
				ttyputerr(_("Sorry, cannot delete the last window"));
				return;
			}
		}
		if (graphicshas(CANUSEFRAMES))
		{
			/* save highlighting and turn it off */
			us_pushhighlight();
			us_clearhighlightcount();

			/* kill the window */
			startobjectchange((INTBIG)us_tool, VTOOL);
			us_killwindowpickanother(w1);
			endobjectchange((INTBIG)us_tool, VTOOL);

			/* restore highlighting */
			us_pophighlight(FALSE);
			return;
		}
	}

	/* figure out which other window to merge this with */
	if (estrcmp(w1->location, x_("top")) == 0 || estrcmp(w1->location, x_("bottom")) == 0 ||
		estrcmp(w1->location, x_("left")) == 0 || estrcmp(w1->location, x_("right")) == 0)
			(void)estrcpy(windcomb, x_("entire")); else
	{
		(void)estrcpy(windcomb, w1->location);
		windcomb[estrlen(windcomb)-2] = 0;
		if (estrcmp(windcomb, x_("bot")) == 0) (void)estrcpy(windcomb, x_("bottom"));
	}

	/* see what divisions this higher window typically makes */
	us_splitwindownames(windcomb, wind1, wind2, vwind1, vwind2);

	/* look for the other window of the typical split */
	(void)estrcpy(windother, x_(""));
	if (estrcmp(wind2, w1->location) == 0) (void)estrcpy(windother, wind1);
	if (estrcmp(wind1, w1->location) == 0) (void)estrcpy(windother, wind2);
	if (estrcmp(vwind2, w1->location) == 0) (void)estrcpy(windother, vwind1);
	if (estrcmp(vwind1, w1->location) == 0) (void)estrcpy(windother, vwind2);

	/* see if there is a window with that name */
	for(w2 = el_topwindowpart; w2 != NOWINDOWPART; w2 = w2->nextwindowpart)
		if (estrcmp(w2->location, windother) == 0) break;

	/* if the other window can't be found, try one more hack */
	if (w2 == NOWINDOWPART)
	{
		/* special case for quadrants that get split strangely */
		if ((estrncmp(w1->location, x_("top-"), 4) == 0 || estrncmp(w1->location, x_("bot-"), 4) == 0) &&
			estrlen(w1->location) == 5)
		{
			if (*w1->location == 't') (void)estrcpy(windother, x_("bot-l")); else
				(void)estrcpy(windother, x_("top-l"));
			windother[4] = w1->location[4];
			if (windother[4] == 'l') (void)estrcpy(windcomb, x_("left")); else
				(void)estrcpy(windcomb, x_("right"));
			for(w2 = el_topwindowpart; w2 != NOWINDOWPART; w2 = w2->nextwindowpart)
				if (estrcmp(w2->location, windother) == 0) break;
		}
	}
	if (w2 == NOWINDOWPART)
	{
		us_abortcommand(_("Cannot kill the current window"));
		return;
	}

	/* if the other window is to be killed, swap them */
	if (!thisw)
	{
		w = w1;   w1 = w2;   w2 = w;
	}

	/* turn off highlighting */
	us_pushhighlight();
	us_clearhighlightcount();

	/* create a new window to cover both */
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)NOWINDOWPART,
		VWINDOWPART|VDONTSAVE);
	startobjectchange((INTBIG)us_tool, VTOOL);
	wnew = newwindowpart(windcomb, w2);
	if (wnew == NOWINDOWPART) return;

	/* save information from the old window */
	copywindowpart(&windowsave, w2);

	/* if merging an editor or explorer window, move the structure */
	if ((w2->state&WINDOWTYPE) == TEXTWINDOW || (w2->state&WINDOWTYPE) == POPTEXTWINDOW)
	{
		(void)setval((INTBIG)wnew, VWINDOWPART, x_("editor"), (INTBIG)w2->editor, VADDRESS);
		(void)setval((INTBIG)w2, VWINDOWPART, x_("editor"), -1, VADDRESS);
	}
	if ((w2->state&WINDOWTYPE) == EXPLORERWINDOW)
	{
		(void)setval((INTBIG)wnew, VWINDOWPART, x_("expwindow"), (INTBIG)w2->expwindow, VADDRESS);
		(void)setval((INTBIG)w2, VWINDOWPART, x_("expwindow"), -1, VADDRESS);
	}

	/* remove old windows */
	killwindowpart(w1);
	killwindowpart(w2);

	/* set window extents */
	us_windowfit(wnew->frame, FALSE, 1);

	/* use former window for scaling */
	w = &windowsave;

	/* window might have got bigger: see if grid can be drawn */
	if ((wnew->state&GRIDTOOSMALL) != 0) us_gridset(wnew, GRIDON);

	(void)setval((INTBIG)el_curlib, VLIBRARY, x_("curnodeproto"), (INTBIG)wnew->curnodeproto,
		VNODEPROTO);
	endobjectchange((INTBIG)us_tool, VTOOL);
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)wnew,
		VWINDOWPART|VDONTSAVE);

	/* restore highlighting */
	us_pophighlight(FALSE);
}

/*
 * routine to determine whether the division of window "w1" into windows
 * "w2" and "w3" can be done with block transfer.  This requires that
 * the windows have identical aspect ratios in one axis
 */
BOOLEAN us_windowcansplit(WINDOWPART *w1, WINDOWPART *w2, WINDOWPART *w3)
{
	REGISTER INTBIG new2l, new2h;

	if (w2->usehx-w2->uselx == w3->usehx-w3->uselx &&
		w2->screenhx-w2->screenlx == w3->screenhx-w3->screenlx)
	{
		/* first see if it is an obvious split */
		if (w2->usehx-w2->uselx == w1->usehx-w1->uselx &&
			w2->screenhx-w2->screenlx == w1->screenhx-w1->screenlx)
				return(TRUE);

		/* now see if it is a relative fit for changed window size */
		new2l = muldiv(((w1->usehx-w1->uselx) - (w2->usehx-w2->uselx))/2,
			w1->screenhx-w1->screenlx, w1->usehx-w1->uselx) + w1->screenlx;
		new2h = w2->screenlx + muldiv(w2->usehx-w2->uselx,
			w1->screenhx-w1->screenlx, w1->usehx-w1->uselx);
		if (new2l == w2->screenlx && new2h == w2->screenhx) return(TRUE);
	}
	if (w2->usehy-w2->usely == w3->usehy-w3->usely &&
		w2->screenhy-w2->screenly == w3->screenhy-w3->screenly)
	{
		/* first see if it is an obvious split */
		if (w2->usehy-w2->usely == w1->usehy-w1->usely &&
			w2->screenhy-w2->screenly == w1->screenhy-w1->screenly)
				return(TRUE);

		/* now see if it is a relative fit for changed window size */
		new2l = muldiv(((w1->usehy-w1->usely) - (w2->usehy-w2->usely))/2,
			w1->screenhy-w1->screenly, w1->usehy-w1->usely) + w1->screenly;
		new2h = w2->screenly + muldiv(w2->usehy-w2->usely,
			w1->screenhy-w1->screenly, w1->usehy-w1->usely);
		if (new2l == w2->screenly && new2h == w2->screenhy) return(TRUE);
	}
	return(FALSE);
}

/*
 * routine to ensure that the cell "np"
 * is displayed somewhere on the screen.  If not, it
 * is displayed in the current window
 */
void us_ensurewindow(NODEPROTO *np)
{
	REGISTER WINDOWPART *w;
	CHAR *par[1];

	/* if nothing specified, quit */
	if (np == NONODEPROTO) return;

	/* see if that cell is in a window */
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		if (w->curnodeproto == np) break;

	/* if the cell is not in a window, put it there */
	if (w == NOWINDOWPART)
	{
		par[0] = describenodeproto(np);
		us_editcell(1, par);
		us_endchanges(NOWINDOWPART);
	}
}

/*
 * Routine to kill window "w" and set the current window to some other.
 */
void us_killwindowpickanother(WINDOWPART *w)
{
	REGISTER NODEPROTO *np;

	killwindowpart(w);

	if (w != el_curwindowpart) return;

	w = el_topwindowpart;
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)w,
		VWINDOWPART|VDONTSAVE);
	if (w != NOWINDOWPART) np = w->curnodeproto; else np = NONODEPROTO;
	(void)setval((INTBIG)el_curlib, VLIBRARY, x_("curnodeproto"), (INTBIG)np, VNODEPROTO);
}

/*
 * routine to adjust the coordinate values in (x, y) from screen space to
 * the space of window "w"
 */
void us_scaletowindow(INTBIG *x, INTBIG *y, WINDOWPART *w)
{
	*x = muldiv(*x - w->uselx, w->screenhx - w->screenlx, w->usehx - w->uselx) + w->screenlx;
	*y = muldiv(*y - w->usely, w->screenhy - w->screenly, w->usehy - w->usely) + w->screenly;
}

/******************** TEXT EDITING ********************/

EDITORTABLE us_editortable[] =
{
	/* the point-and-click editor */
	{x_("Point-and-click"),
	us_editpacmakeeditor, us_editpacterminate, us_editpactotallines, us_editpacgetline,
	us_editpacaddline, us_editpacreplaceline, us_editpacdeleteline, us_editpachighlightline,
	us_editpacsuspendgraphics, us_editpacresumegraphics,
	us_editpacwritetextfile, us_editpacreadtextfile,
	us_editpaceditorterm, us_editpacshipchanges, us_editpacgotchar,
	us_editpaccut, us_editpaccopy, us_editpacpaste,
	us_editpacundo, us_editpacsearch, us_editpacpan},

	/* the EMACS-like editor */
	{x_("EMACS-like"),
	us_editemacsmakeeditor, us_editemacsterminate, us_editemacstotallines, us_editemacsgetline,
	us_editemacsaddline, us_editemacsreplaceline, us_editemacsdeleteline, us_editemacshighlightline,
	us_editemacssuspendgraphics, us_editemacsresumegraphics,
	us_editemacswritetextfile, us_editemacsreadtextfile,
	us_editemacseditorterm, us_editemacsshipchanges, us_editemacsgotchar,
	us_editemacscut, us_editemacscopy, us_editemacspaste,
	us_editemacsundo, us_editemacssearch, us_editemacspan},

	{NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL,
	NULL, NULL,
	NULL, NULL, NULL,
	NULL, NULL, NULL,
	NULL, NULL, NULL}
};

/*
 * dispatch routine to describe this editor
 */
void us_describeeditor(CHAR **name)
{
	*name = us_editortable[us_currenteditor].editorname;
}

/*
 * dispatch routine for creating a new editor
 */
WINDOWPART *us_makeeditor(WINDOWPART *oriwin, CHAR *header, INTBIG *chars, INTBIG *lines)
{
	return((*us_editortable[us_currenteditor].makeeditor)(oriwin,header,chars,lines));
}

/*
 * dispatch routine to return the total number of valid lines in the edit buffer
 */
INTBIG us_totallines(WINDOWPART *win)
{
	return((*us_editortable[us_currenteditor].totallines)(win));
}

/*
 * dispatch routine to get the string on line "lindex" (0 based).  A negative line
 * returns the current line.  Returns -1 if the index is beyond the file limit
 */
CHAR *us_getline(WINDOWPART *win, INTBIG lindex)
{
	return((*us_editortable[us_currenteditor].getline)(win, lindex));
}

/*
 * dispatch routine to add line "str" to the text cell to become line "lindex"
 */
void us_addline(WINDOWPART *win, INTBIG lindex, CHAR *str)
{
	(*us_editortable[us_currenteditor].addline)(win, lindex, str);
}

/*
 * dispatch routine to replace the line number "lindex" with the string "str".
 */
void us_replaceline(WINDOWPART *win, INTBIG lindex, CHAR *str)
{
	(*us_editortable[us_currenteditor].replaceline)(win, lindex, str);
}

/*
 * dispatch routine to delete line number "lindex"
 */
void us_deleteline(WINDOWPART *win, INTBIG lindex)
{
	(*us_editortable[us_currenteditor].deleteline)(win, lindex);
}

/*
 * dispatch routine to highlight lines "lindex" to "hindex" in the text window
 */
void us_highlightline(WINDOWPART *win, INTBIG lindex, INTBIG hindex)
{
	(*us_editortable[us_currenteditor].highlightline)(win, lindex, hindex);
}

/*
 * dispatch routine to stop the graphic display of changes (for batching)
 */
void us_suspendgraphics(WINDOWPART *win)
{
	(*us_editortable[us_currenteditor].suspendgraphics)(win);
}

/*
 * dispatch routine to restart the graphic display of changes and redisplay (for batching)
 */
void us_resumegraphics(WINDOWPART *win)
{
	(*us_editortable[us_currenteditor].resumegraphics)(win);
}

/*
 * dispatch routine to write the text file to "file"
 */
void us_writetextfile(WINDOWPART *win, CHAR *file)
{
	(*us_editortable[us_currenteditor].writetextfile)(win, file);
}

/*
 * dispatch routine to read the text file "file"
 */
void us_readtextfile(WINDOWPART *win, CHAR *file)
{
	(*us_editortable[us_currenteditor].readtextfile)(win, file);
}

/*
 * dispatch routine to get the next character
 */
void us_editorterm(WINDOWPART *w)
{
	(*us_editortable[us_currenteditor].editorterm)(w);
}

/*
 * dispatch routine to force changes from the editor in window "w"
 */
void us_shipchanges(WINDOWPART *w)
{
	(*us_editortable[us_currenteditor].shipchanges)(w);
}

/*
 * dispatch routine to get the next character
 */
BOOLEAN us_gotchar(WINDOWPART *w, INTSML i, INTBIG special)
{
	return((*us_editortable[us_currenteditor].gotchar)(w, i, special));
}

/*
 * dispatch routine to cut text
 */
void us_cuttext(WINDOWPART *w)
{
	(*us_editortable[us_currenteditor].cut)(w);
}

/*
 * dispatch routine to copy text
 */
void us_copytext(WINDOWPART *w)
{
	(*us_editortable[us_currenteditor].copy)(w);
}

/*
 * dispatch routine to paste text
 */
void us_pastetext(WINDOWPART *w)
{
	(*us_editortable[us_currenteditor].paste)(w);
}

/*
 * dispatch routine to undo text changes
 */
void us_undotext(WINDOWPART *w)
{
	(*us_editortable[us_currenteditor].undo)(w);
}

/*
 * dispatch routine to search and/or replace text.  If "replace" is nonzero, this is
 * a replace.  The meaning of "bits" is as follows:
 *   1   search from top
 *   2   replace all
 *   4   case sensitive
 *   8   search upwards
 */
void us_searchtext(WINDOWPART *w, CHAR *str, CHAR *replace, INTBIG bits)
{
	(*us_editortable[us_currenteditor].search)(w, str, replace, bits);
}

/*
 * dispatch routine to pan the text window by (dx, dy)
 */
void us_pantext(WINDOWPART *w, INTBIG dx, INTBIG dy)
{
	(*us_editortable[us_currenteditor].pan)(w, dx, dy);
}

/*
 * support routine to allocate a new editor from the pool (if any) or memory
 * routine returns NOEDITOR upon error
 */
EDITOR *us_alloceditor(void)
{
	REGISTER EDITOR *e;

	e = (EDITOR *)emalloc((sizeof (EDITOR)), us_tool->cluster);
	if (e == 0) return(NOEDITOR);
	e->state = 0;
	e->nexteditor = NOEDITOR;
	e->editobjvar = NOVARIABLE;
	return(e);
}

/*
 * support routine to return editor "e" to the pool of free editors
 */
void us_freeeditor(EDITOR *e)
{
	if (e == 0 || e == NOEDITOR) return;
	efree(e->header);
	(*us_editortable[us_currenteditor].terminate)(e);
	efree((CHAR *)e);
}

/******************** SPECIAL WINDOW HANDLERS ********************/

/*
 * routine to accept changes in an edit window examining a variable.  If "nature" is:
 *  REPLACETEXTLINE  line "changed" goes from "oldline" to "newline"
 *  DELETETEXTLINE   line "changed deleted (was "oldline")
 *  INSERTTEXTLINE   line "newline" inserted before line "changed"
 *  REPLACEALLTEXT   "changed" lines "newline" replace all text
 */
void us_varchanges(WINDOWPART *w, INTBIG nature, CHAR *oldline, CHAR *newline, INTBIG changed)
{
	REGISTER INTBIG j, l, save;
	REGISTER BOOLEAN res;
	REGISTER INTBIG newval, i, len;
	REGISTER CHAR **newlist, *pt;
	float newfloat;
	REGISTER EDITOR *ed;
	Q_UNUSED( oldline );

	ed = w->editor;
	if (ed->editobjvar == NOVARIABLE) return;
	if ((ed->editobjvar->type&VCANTSET) != 0)
	{
		ttyputerr(M_("This variable cannot be changed"));
		ed->editobjvar = NOVARIABLE;
		return;
	}
	len = getlength(ed->editobjvar);

	/* when replacing the entire text, reduce to individual calls */
	if (nature == REPLACEALLTEXT)
	{
		newlist = (CHAR **)newline;
		for(i=0; i<changed; i++)
			us_varchanges(w, REPLACETEXTLINE, x_(""), newlist[i], i);
		for(i=len-1; i>=changed; i--)
			us_varchanges(w, DELETETEXTLINE, x_(""), x_(""), i);
		return;
	}

	if (nature == DELETETEXTLINE && len == 1)
	{
		/* delete of last entry: instead, replace it with a null */
		newline = x_("");
		nature = REPLACETEXTLINE;
	} else if (nature == REPLACETEXTLINE && changed >= len)
	{
		/* change of line beyond end: instead make an insert */
		nature = INSERTTEXTLINE;
	}

	/* disallow deletions and insertions if the number of lines is fixed */
	if (nature == DELETETEXTLINE || nature == INSERTTEXTLINE)
	{
		if ((ed->state&LINESFIXED) != 0) return;
	}

	/* get the value */
	j = 0;
	save = 0;
	newval = 0;
	if (nature == REPLACETEXTLINE || nature == INSERTTEXTLINE)
	{
		pt = newline;
		while (*pt == ' ' || *pt == '\t') pt++;
		switch (ed->editobjvar->type&VTYPE)
		{
			case VINTEGER:
			case VSHORT:
			case VBOOLEAN:
			case VADDRESS:
				newval = myatoi(pt);
				break;
			case VFRACT:
				newval = atofr(pt);
				break;
			case VFLOAT:
			case VDOUBLE:
				newfloat = (float)eatof(pt);
				newval = castint(newfloat);
				break;
			case VSTRING:
				j = l = estrlen(newline);
				if (estrcmp(&newline[l-3], x_(" */")) == 0)
				{
					for(j = l-5; j >= 0; j--)
						if (estrncmp(&newline[j], x_("/* "), 3) == 0) break;
					while (j > 0 && newline[j-1] == ' ') j--;
					if (j < 0) j = l;
				}
				save = newline[j];
				newline[j] = 0;
				newval = (INTBIG)newline;
				break;
			default:
				ttyputmsg(_("Cannot update this type of variable (0%o)"), ed->editobjvar->type);
				break;
		}
	}

	/* make the change */
	res = TRUE;
	switch (nature)
	{
		case REPLACETEXTLINE:
			if (changed < 0 || changed >= len) return;
			res = setind((INTBIG)ed->editobjaddr, ed->editobjtype, ed->editobjqual, changed, newval);
			break;
		case DELETETEXTLINE:
			if (changed < 0 || changed >= len) return;
			res = delind((INTBIG)ed->editobjaddr, ed->editobjtype, ed->editobjqual, changed);
			break;
		case INSERTTEXTLINE:
			if (changed <= 0 || changed > len) return;
			res = insind((INTBIG)ed->editobjaddr, ed->editobjtype, ed->editobjqual, changed, newval);

			/* why is this next line necessary? */
			ed->editobjvar = getval((INTBIG)ed->editobjaddr, ed->editobjtype, -1, ed->editobjqual);
			break;
	}

	/* clean-up string if one was passed */
	if ((nature == REPLACETEXTLINE || nature == INSERTTEXTLINE) &&
		(ed->editobjvar->type&VTYPE) == VSTRING) newline[j] = (CHAR)save;

	if (res)
	{
		ttyputerr(_("Error changing variable: ignoring further changes"));
		ed->editobjvar = NOVARIABLE;
	}
}

/*
 * routine to accept changes in an edit window examining a textual cell.  If "nature" is:
 *  REPLACETEXTLINE  line "changed" goes from "oldline" to "newline"
 *  DELETETEXTLINE   line "changed" deleted (was "oldline")
 *  INSERTTEXTLINE   line "newline" inserted before line "changed"
 *  REPLACEALLTEXT   "changed" lines "newline" replace all text
 */
void us_textcellchanges(WINDOWPART *w, INTBIG nature, CHAR *oldline, CHAR *newline, INTBIG changed)
{
	REGISTER INTBIG len;
	REGISTER BOOLEAN res;
	REGISTER EDITOR *ed;
	Q_UNUSED( oldline );

	ed = w->editor;
	if (ed->editobjvar == NOVARIABLE) return;
	if ((ed->editobjvar->type&VTYPE) != VSTRING) return;
	if ((ed->editobjvar->type&VCANTSET) != 0) return;
	len = getlength(ed->editobjvar);

	if (nature == DELETETEXTLINE && len == 1)
	{
		/* delete of last line: instead, replace it with a blank */
		newline = x_("");
		nature = REPLACETEXTLINE;
	} else if (nature == REPLACETEXTLINE && changed >= len)
	{
		/* change of line one beyond end: instead make an insert */
		nature = INSERTTEXTLINE;
	}

	res = TRUE;
	switch (nature)
	{
		case REPLACETEXTLINE:
			if (changed < 0 || changed >= len) return;
			res = setindkey((INTBIG)ed->editobjaddr, VNODEPROTO, el_cell_message_key,
				changed, (INTBIG)newline);
			break;
		case DELETETEXTLINE:
			if (changed < 0 || changed >= len) return;
			res = delindkey((INTBIG)ed->editobjaddr, VNODEPROTO, el_cell_message_key,
				changed);
			break;
		case INSERTTEXTLINE:
			res = insindkey((INTBIG)ed->editobjaddr, VNODEPROTO, el_cell_message_key,
				changed, (INTBIG)newline);
			break;
		case REPLACEALLTEXT:
			if (setvalkey((INTBIG)ed->editobjaddr, VNODEPROTO, el_cell_message_key,
				(INTBIG)newline, VSTRING | VISARRAY | (changed << VLENGTHSH)) != NOVARIABLE)
					res = FALSE;
			break;
	}

	if (res)
	{
		ttyputerr(_("Error changing variable: ignoring further changes"));
		ed->editobjvar = NOVARIABLE;
	}
}

/*
 * private character handler for the text window.  This routine normally
 * passes all commands to the editor's character handler.  However, it
 * interprets M(=) which is for editing the cell on the current line
 */
BOOLEAN us_celledithandler(WINDOWPART *w, INTSML ch, INTBIG special)
{
	CHAR *newpar[2], *str;
	REGISTER INTBIG i;
	REGISTER BOOLEAN meta;
	REGISTER EDITOR *ed;
	extern INTBIG us_lastemacschar;

	/* the EMACS text editor must be running */
	us_describeeditor(&str);
	if (namesame(str, x_("emacs")) != 0) return(us_gotchar(w, ch, special));

	/* see if the meta key is held down (serious black magic) */
	meta = FALSE;
	if ((special&ACCELERATORDOWN) != 0) meta = TRUE;
	if ((us_lastemacschar&2) != 0) meta = TRUE;

	/* pass character on to the editor if not M(=) */
	if (!meta || ch != '=') return(us_gotchar(w, ch, special));

	/* M(=) typed: parse current line to edit named cell */
	ed = w->editor;
	(void)allocstring(&str, us_getline(w, ed->curline), el_tempcluster);

	/* first drop everything past the first space character */
	for(i=0; str[i] != 0; i++) if (str[i] == ' ') break;
	if (str[i] != 0) str[i] = 0;

	if (str[0] == 0) ttyputerr(_("No cell specified on this line")); else
	{
		/* issue the "editcell" command */
		newpar[0] = x_("editcell");
		newpar[1] = str;
		telltool(us_tool, 2, newpar);
		setactivity(_("Cell Selection"));
	}

	/* clean up */
	efree(str);
	return(FALSE);
}

/******************** COMMAND SUPPORT ********************/

BOOLEAN us_demandxy(INTBIG *x, INTBIG *y)
{
	BOOLEAN ret;

	ret = getxy(x, y);
	if (ret) ttyputmsg(_("Cursor must be in an editing window"));
	return(ret);
}

static INTBIG us_curx, us_cury;

/*
 * routine to get the co-ordinates of the cursor into the reference parameters
 * "x" and "y".  If "GOTXY" is set in the global variable "us_state" then
 * this has already been done.  The routine returns true if there is not a
 * valid cursor position.
 */
BOOLEAN getxy(INTBIG *x, INTBIG *y)
{
	INTBIG gx, gy;
	REGISTER BOOLEAN ret;

	if ((us_state&GOTXY) == 0)
	{
		readtablet(&gx, &gy);
		ret = us_setxy(gx, gy);
	} else ret = FALSE;
	*x = us_curx;
	*y = us_cury;
	return(ret);
}

/*
 * routine to take the values (realx, realy) from the tablet and store
 * them in the variables (us_curx, us_cury) which are in design-space
 * co-ordinates.  "GOTXY" in the global variable "us_state" is set to indicate
 * that the co-ordinates are valid.  The current window is set according
 * to the cursor position.  The routine returns true if the position
 * is not in a window.
 */
BOOLEAN us_setxy(INTBIG x, INTBIG y)
{
	REGISTER WINDOWPART *w;
	REGISTER WINDOWFRAME *wf;

	us_curx = x;   us_cury = y;

	/* figure out which window it is in */
	wf = getwindowframe(TRUE);
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		if (w->frame != wf) continue;
		if (x >= w->uselx && x <= w->usehx && y >= w->usely && y <= w->usehy)
		{
			/* make this window the current one */
			if (w != el_curwindowpart)
			{
				(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)w,
					VWINDOWPART|VDONTSAVE);
				(void)setval((INTBIG)el_curlib, VLIBRARY, x_("curnodeproto"),
					(INTBIG)w->curnodeproto, VNODEPROTO);
			}
			us_scaletowindow(&us_curx, &us_cury, w);
			xform(us_curx, us_cury, &us_curx, &us_cury, w->intocell);
			us_state |= GOTXY;
			return(FALSE);
		}
	}
	return(TRUE);
}

/*
 * routine to force the parameters "xcur" and "ycur" to align to the
 * nearest "alignment" units
 */
void gridalign(INTBIG *xcur, INTBIG *ycur, INTBIG alignmentdivisor, NODEPROTO *cell)
{
	INTBIG otheralign;
	REGISTER INTBIG val, alignment;
	REGISTER TECHNOLOGY *tech;

	tech = NOTECHNOLOGY;
	if (cell != NONODEPROTO) tech = cell->tech;
	if (tech == NOTECHNOLOGY) tech = el_curtech;
	alignment = muldiv(us_alignment_ratio, el_curlib->lambda[tech->techindex], WHOLE) /
		alignmentdivisor;
	val = us_alignvalue(*xcur, alignment, &otheralign);
	if (abs(*xcur-val) < abs(*xcur-otheralign)) *xcur = val; else
		*xcur = otheralign;
	val = us_alignvalue(*ycur, alignment, &otheralign);
	if (abs(*ycur-val) < abs(*ycur-otheralign)) *ycur = val; else
		*ycur = otheralign;
}

/*
 * routine to return "value", aligned to the nearest "alignment" units.
 * The next closest alignment value (if "value" is not on the grid)
 * is returned in "otheralign".
 */
INTBIG us_alignvalue(INTBIG value, INTBIG alignment, INTBIG *otheralign)
{
	REGISTER INTBIG i, v1, v2;
	REGISTER INTBIG sign;

	/* determine the sign of the value */
	if (value < 0) { sign = -1; value = -value; } else sign = 1;

	/* compute the two aligned values */
	if (alignment == 0) v1 = value; else
		v1 = value / alignment * alignment;
	if (v1 == value) v2 = value; else v2 = v1 + alignment;
	v1 *= sign;   v2 *= sign;

	/* make sure "v1" is the closest aligned value */
	if (abs(v1-value) > abs(v2-value)) { i = v1;   v1 = v2;   v2 = i; }

	*otheralign = v2;
	return(v1);
}

/* routine to ensure that a current window exists */
BOOLEAN us_needwindow(void)
{
	if (el_curwindowpart != NOWINDOWPART) return(FALSE);
	us_abortcommand(_("No current window"));
	return(TRUE);
}

/* routine to ensure that a cell exists in the current window */
NODEPROTO *us_needcell(void)
{
	REGISTER NODEPROTO *np;

	np = getcurcell();
	if (np != NONODEPROTO) return(np);
	if (el_curwindowpart == NOWINDOWPART)
	{
		us_abortcommand(_("No current window (select one with Cells/Edit Cell)"));
	} else
	{
		if ((us_tool->toolstate&NODETAILS) != 0)
			us_abortcommand(_("No cell in this window (select one with Cells/Edit Cell)")); else
				us_abortcommand(_("No cell in this window (select one with '-editcell')"));
	}
	return(NONODEPROTO);
}

/*
 * Routine to ensure that node "item" can be modified in cell "cell".
 * If "item" is NONODEINST, check that cell "cell" can be modified at all.
 * Returns true if the edit cannot be done.
 */
BOOLEAN us_cantedit(NODEPROTO *cell, NODEINST *item, BOOLEAN giveerror)
{
	REGISTER INTBIG count;
	extern COMCOMP us_noyesalwaysp;
	CHAR *pars[5];
	REGISTER void *infstr;

	/* if an instance is specified, check it */
	if (item != NONODEINST)
	{
		if ((item->userbits&NILOCKED) != 0)
		{
			if (!giveerror) return(TRUE);
			infstr = initinfstr();
			formatinfstr(infstr, _("Changes to locked node %s are disallowed.  Change anyway?"),
				describenodeinst(item));
			count = ttygetparam(returninfstr(infstr), &us_noyesalwaysp, 5, pars);
			if (count <= 0 || namesamen(pars[0], x_("no"), estrlen(pars[0])) == 0) return(TRUE);
			if (namesamen(pars[0], x_("always"), estrlen(pars[0])) == 0)
				(void)setval((INTBIG)item, VNODEINST, x_("userbits"),
					item->userbits & ~NILOCKED, VINTEGER);
		}
		if (item->proto->primindex != 0)
		{
			/* see if a primitive is locked */
			if ((item->proto->userbits&LOCKEDPRIM) != 0 &&
				(us_useroptions&NOPRIMCHANGES) != 0)
			{
				if (!giveerror) return(TRUE);
				infstr = initinfstr();
				formatinfstr(infstr, _("Changes to locked primitives (such as %s) are disallowed.  Change anyway?"),
					describenodeinst(item));
				count = ttygetparam(returninfstr(infstr), &us_noyesalwaysp, 5, pars);
				if (count <= 0 || namesamen(pars[0], x_("no"), estrlen(pars[0])) == 0) return(TRUE);
				if (namesamen(pars[0], x_("always"), estrlen(pars[0])) == 0)
					(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey,
						 us_useroptions & ~NOPRIMCHANGES, VINTEGER);
			}
		} else
		{
			/* see if this type of cell is locked */
			if ((cell->userbits&NPILOCKED) != 0)
			{
				if (!giveerror) return(TRUE);
				infstr = initinfstr();
				formatinfstr(infstr, _("Instances in cell %s are locked.  You cannot move %s.  Change anyway?"),
					describenodeproto(cell), describenodeinst(item));
				count = ttygetparam(returninfstr(infstr), &us_noyesalwaysp, 5, pars);
				if (count <= 0 || namesamen(pars[0], x_("no"), estrlen(pars[0])) == 0) return(TRUE);
				if (namesamen(pars[0], x_("always"), estrlen(pars[0])) == 0)
					setval((INTBIG)cell, VNODEPROTO, x_("userbits"),
						cell->userbits & ~NPILOCKED, VINTEGER);
			}
		}
	}

	/* check for general changes to the cell */
	if ((cell->userbits&NPLOCKED) != 0)
	{
		if (!giveerror) return(TRUE);
		infstr = initinfstr();
		formatinfstr(infstr, _("Changes to cell %s are locked.  Change anyway?"),
			describenodeproto(cell));
		count = ttygetparam(returninfstr(infstr), &us_noyesalwaysp, 5, pars);
		if (count <= 0 || namesamen(pars[0], x_("no"), estrlen(pars[0])) == 0) return(TRUE);
		if (namesamen(pars[0], x_("always"), estrlen(pars[0])) == 0)
			setval((INTBIG)cell, VNODEPROTO, x_("userbits"), cell->userbits & ~NPLOCKED, VINTEGER);
	}
	return(FALSE);
}

/*
 * routine to determine the proper position of the cursor given that
 * it must adjust to the nearest "angle" tenth-degree radial coming out of
 * the point (tx,ty) and that it is currently at (nx, ny).  The
 * adjusted point is placed into (fx, fy) and the proper radial starting
 * point in "poly" is placed in (tx, ty).
 */
void us_getslide(INTBIG angle, INTBIG tx, INTBIG ty, INTBIG nx, INTBIG ny, INTBIG *fx,
	INTBIG *fy)
{
	REGISTER INTBIG ang;
	INTBIG ix, iy;

	/* if angle is unconstrained, use the exact cursor position */
	if (angle <= 0)
	{
		*fx = nx;   *fy = ny;
		return;
	}

	/* check all permissable angles */
	for(ang = 0; ang < 3600; ang += angle)
	{
		/* get close point to (nx,ny) on "ang" tenth-degree radial from (tx,ty) */
		(void)intersect(tx, ty, ang, nx, ny, (ang+900)%3600, &ix, &iy);

		/* accumulate the intersection closest to the cursor */
		if (ang != 0 && abs(*fx-nx) + abs(*fy-ny) < abs(ix-nx) + abs(iy-ny)) continue;
		*fx = ix;   *fy = iy;
	}
}

/*
 * routine to convert command interpreter letter "letter" to the full
 * variable name on the user tool object
 */
CHAR *us_commandvarname(INTSML letter)
{
	static CHAR varname[20];

	if (isupper(letter))
	{
		(void)estrcpy(varname, x_("USER_local_capX"));
		varname[14] = tolower(letter);
	} else
	{
		(void)estrcpy(varname, x_("USER_local_X"));
		varname[11] = (CHAR)letter;
	}
	return(varname);
}

/*
 * routine to parse the variable path in "str" and return the object address
 * and type on which this variable resides along with the variable name.
 * The object address and type are placed in "objaddr" and "objtype";
 * the variable name is placed in "varname".  "comvar" is set to true
 * if the variable is a command-interpreter variable (as opposed to a
 * database variable).  If an array index specification is given (a "[]")
 * then the index value is returned in "aindex" (otherwise the value is set
 * to -1).  The routine returns true if the variable path is invalid.
 */
BOOLEAN us_getvar(CHAR *str, INTBIG *objaddr, INTBIG *objtype, CHAR **varname,
	BOOLEAN *comvar, INTBIG *aindex)
{
	REGISTER INTBIG i;
	static CHAR fullvarname[50];

	/* see if an array index is specified */
	*aindex = -1;
	i = estrlen(str);
	if (str[i-1] == ']') for(i--; i >= 0; i--) if (str[i] == '[')
	{
		*aindex = myatoi(&str[i+1]);
		break;
	}

	/* see if this is a command interpreter variable */
	*comvar = FALSE;
	if (str[1] == 0 || str[1] == '[')
	{
		if (str[0] >= 'a' && str[0] <= 'z') *comvar = TRUE;
		if (str[0] >= 'A' && str[0] <= 'Z') *comvar = TRUE;
	}

	/* replace the actual name for command interpreter variables */
	if (*comvar)
	{
		(void)esnprintf(fullvarname, 50, x_("tool:user.%s"), us_commandvarname(str[0]));
		str = fullvarname;
	}

	/* pick apart the variable path */
	return(us_evaluatevariable(str, objaddr, objtype, varname));
}

#define LINELIMIT 300

/*
 * Routine to dump the copyright information to file "f".  Each line of the copyright
 * is prefixed by "prefix" and postfixed by "postfix".
 */
void us_emitcopyright(FILE *f, CHAR *prefix, CHAR *postfix)
{
	REGISTER VARIABLE *var;
	REGISTER FILE *io;
	CHAR *dummy, buffer[LINELIMIT];

	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING, us_copyright_file_key);
	if (var == NOVARIABLE) return;
	io = xopen((CHAR *)var->addr, el_filetypetext, el_libdir, &dummy);
	if (io == 0) return;
	for(;;)
	{
		if (xfgets(buffer, LINELIMIT, io)) break;
		fprintf(f, x_("%s%s%s\n"), prefix, buffer, postfix);
	}
	xclose(io);
}

/******************** QUICK-KEY BINDING ********************/

/*
 * Routine to find the entry in the key binding variable ("USER_binding_keys")
 * that corresponds to key "key", special code "special".  Returns negative if
 * the key is not in the variable.  If the key is found, the binding string
 * is returned in "binding".
 */
INTBIG us_findboundkey(INTSML key, INTBIG special, CHAR **binding)
{
	REGISTER INTBIG i, len;
	INTSML boundkey;
	INTBIG boundspecial;
	REGISTER VARIABLE *var;
	REGISTER CHAR *thisbinding, *pt;

	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_keys_key);
	if (var == NOVARIABLE) return(-1);
	len = getlength(var);
	pt = NOSTRING;
	for(i=0; i<len; i++)
	{
		thisbinding = ((CHAR **)var->addr)[i];
		pt = us_getboundkey(thisbinding, &boundkey, &boundspecial);
		if (pt == 0) continue;
		if (us_samekey(key, special, boundkey, boundspecial)) break;
	}
	if (i >= len) return(-1);
	*binding = pt;
	return(i);
}

/*
 * Routine to set the binding of key "key", special code "special" to the string
 * "binding".  If "quietly" is true, do this without change control.
 */
void us_setkeybinding(CHAR *binding, INTSML key, INTBIG special, BOOLEAN quietly)
{
	CHAR *pt, *justone[1], **bigger, *str;
	REGISTER INTBIG i, j, len, inserted;
	INTSML boundkey;
	INTBIG boundspecial;
	REGISTER VARIABLE *var;

	if ((special&ACCELERATORDOWN) != 0) key = toupper(key);
	i = us_findboundkey(key, special, &pt);
	if (i >= 0)
	{
		if (quietly) nextchangequiet();
		(void)setindkey((INTBIG)us_tool, VTOOL, us_binding_keys_key, i, (INTBIG)binding);
		return;
	}

	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_keys_key);
	if (var == NOVARIABLE)
	{
		justone[0] = binding;
		if (quietly) nextchangequiet();
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_binding_keys_key, (INTBIG)justone,
			VSTRING|VDONTSAVE|VISARRAY|(1<<VLENGTHSH));
	} else
	{
		len = getlength(var);
		bigger = (CHAR **)emalloc((len+1) * (sizeof (CHAR *)), el_tempcluster);
		if (bigger == 0) return;
		j = 0;
		inserted = 0;
		for(i=0; i<len; i++)
		{
			pt = ((CHAR **)var->addr)[i];
			str = us_getboundkey(pt, &boundkey, &boundspecial);
			if (str == 0) continue;
			if (namesame(str, x_("command=")) == 0) continue;
			if (inserted == 0)
			{
				if (boundspecial > special || (boundspecial == special && boundkey > key))
				{
					(void)allocstring(&bigger[j++], binding, el_tempcluster);
					inserted = 1;
				}
			}
			(void)allocstring(&bigger[j++], pt, el_tempcluster);
		}
		if (inserted == 0)
			(void)allocstring(&bigger[j++], binding, el_tempcluster);
		if (quietly) nextchangequiet();
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_binding_keys_key, (INTBIG)bigger,
			VSTRING|VDONTSAVE|VISARRAY|(j<<VLENGTHSH));
		for(i=0; i<j; i++) efree((CHAR *)bigger[i]);
		efree((CHAR *)bigger);
	}
}

/*
 * Routine to describe key "boundkey"/"boundspecial".  Formats it according to
 * "readable":
 *  -1 for end of menu ("/A")
 *   0 for beginning of menu storage ("A/")
 *   1 for display ("Ctrl-A")
 */
CHAR *us_describeboundkey(INTSML boundkey, INTBIG boundspecial, INTBIG readable)
{
	CHAR *acceleratorstring, *acceleratorprefix, *shiftprefix, *accprefix;
	REGISTER void *infstr;

	getacceleratorstrings(&acceleratorstring, &acceleratorprefix);
	if (readable > 0)
	{
		accprefix = acceleratorprefix;
		shiftprefix = x_("S-");
	} else
	{
		accprefix = x_("m-");
		shiftprefix = x_("S");
	}
	infstr = initinfstr();
	if ((boundspecial&SPECIALKEYDOWN) != 0)
	{
		if (readable < 0) addtoinfstr(infstr, '/');
		switch ((boundspecial&SPECIALKEY)>>SPECIALKEYSH)
		{
			case SPECIALKEYF1:  addstringtoinfstr(infstr, x_("F1"));    break;
			case SPECIALKEYF2:  addstringtoinfstr(infstr, x_("F2"));    break;
			case SPECIALKEYF3:  addstringtoinfstr(infstr, x_("F3"));    break;
			case SPECIALKEYF4:  addstringtoinfstr(infstr, x_("F4"));    break;
			case SPECIALKEYF5:  addstringtoinfstr(infstr, x_("F5"));    break;
			case SPECIALKEYF6:  addstringtoinfstr(infstr, x_("F6"));    break;
			case SPECIALKEYF7:  addstringtoinfstr(infstr, x_("F7"));    break;
			case SPECIALKEYF8:  addstringtoinfstr(infstr, x_("F8"));    break;
			case SPECIALKEYF9:  addstringtoinfstr(infstr, x_("F9"));    break;
			case SPECIALKEYF10: addstringtoinfstr(infstr, x_("F10"));   break;
			case SPECIALKEYF11: addstringtoinfstr(infstr, x_("F11"));   break;
			case SPECIALKEYF12: addstringtoinfstr(infstr, x_("F12"));   break;
			case SPECIALKEYARROWL:
				if ((boundspecial&ACCELERATORDOWN) != 0)
					addstringtoinfstr(infstr, accprefix);
				if ((boundspecial&SHIFTDOWN) != 0)
					addstringtoinfstr(infstr, shiftprefix);
				addstringtoinfstr(infstr, x_("LEFT"));
				break;
			case SPECIALKEYARROWR:
				if ((boundspecial&ACCELERATORDOWN) != 0)
					addstringtoinfstr(infstr, accprefix);
				if ((boundspecial&SHIFTDOWN) != 0)
					addstringtoinfstr(infstr, shiftprefix);
				addstringtoinfstr(infstr, x_("RIGHT"));
				break;
			case SPECIALKEYARROWU:
				if ((boundspecial&ACCELERATORDOWN) != 0)
					addstringtoinfstr(infstr, accprefix);
				if ((boundspecial&SHIFTDOWN) != 0)
					addstringtoinfstr(infstr, shiftprefix);
				addstringtoinfstr(infstr, x_("UP"));
				break;
			case SPECIALKEYARROWD:
				if ((boundspecial&ACCELERATORDOWN) != 0)
					addstringtoinfstr(infstr, accprefix);
				if ((boundspecial&SHIFTDOWN) != 0)
					addstringtoinfstr(infstr, shiftprefix);
				addstringtoinfstr(infstr, x_("DOWN"));
				break;
		}
		if (readable == 0) addtoinfstr(infstr, '/');
	} else
	{
		if ((boundspecial&ACCELERATORDOWN) != 0)
		{
			if (readable < 0) addtoinfstr(infstr, '/');
			if (readable > 0) addstringtoinfstr(infstr, accprefix);
			if (boundkey > 0 && boundkey < 033) formatinfstr(infstr, x_("^%c"), boundkey + 0100); else
				addtoinfstr(infstr, (CHAR)boundkey);
			if (readable == 0) addtoinfstr(infstr, '/');
		} else
		{
			if (readable < 0) addtoinfstr(infstr, '\\');
			if (boundkey > 0 && boundkey < 033) formatinfstr(infstr, x_("^%c"), boundkey + 0100); else
				if (boundkey == 0177) addstringtoinfstr(infstr, x_("DEL")); else
					addtoinfstr(infstr, (CHAR)boundkey);
			if (readable == 0) addtoinfstr(infstr, '\\');
		}
	}
	return(returninfstr(infstr));
}

/*
 * Returns true if the keys "key1"/"special1" is the same as "key2"/"special2"
 */
BOOLEAN us_samekey(INTSML key1, INTBIG special1, INTSML key2, INTBIG special2)
{
	if (special1 != special2) return(FALSE);
	if ((special1&SPECIALKEYDOWN) == 0)
	{
		if ((special1&ACCELERATORDOWN) != 0)
		{
			key1 = toupper(key1);
			key2 = toupper(key2);
		}
		if (key1 != key2) return(FALSE);
	}
	return(TRUE);
}

/*
 * Routine to parse the key binding string in "origbinding" (an entry in "USER_binding_keys")
 * Extracts the key information from the start of the string and returns it in
 * "boundkey" and "boundspecial".  Then returns the rest of the string.
 * Returns zero on error.
 */
CHAR *us_getboundkey(CHAR *origbinding, INTSML *boundkey, INTBIG *boundspecial)
{
	REGISTER CHAR *pt, save, *word;
	REGISTER INTBIG offset, len;
	CHAR binding[200], *acceleratorstring, *acceleratorprefix;

	estrcpy(binding, origbinding);
	*boundspecial = 0;
	if (binding[0] != 0 && binding[1] == 0)
	{
		*boundkey = binding[0];
		return(x_(""));
	}
	if (*binding == '/' || *binding == '\\')
	{
		pt = binding;
		save = *pt;
		word = binding + 1;
	} else
	{
		for(pt = binding; *pt != 0; pt++)
			if (*pt == '/' || *pt == '\\') break;
		save = *pt;
		*pt = 0;
		word = binding;
	}

	getacceleratorstrings(&acceleratorstring, &acceleratorprefix);
	len = estrlen(acceleratorprefix);
	if (namesamen(word, acceleratorprefix, len) == 0)
	{
		word += len;
		*boundspecial |= ACCELERATORDOWN;
	} else if (word[0] == 'm' && word[1] == '-')
	{
		word += 2;
		*boundspecial |= ACCELERATORDOWN;
	}

	/* handle single keystrokes */
	if (word[1] == 0)
	{
		*boundkey = word[0];
		if (save == '/') *boundspecial |= ACCELERATORDOWN;
	} else
	{
		/* look for special names */
		if (word[0] == '^')
		{
			if (word[1] == '^')
			{
				*boundkey = '^';
			} else if (isdigit(word[1]) != 0)
			{
				*boundkey = (INTSML)myatoi(&word[1]);
			} else
			{
				*boundkey = toupper(word[1]) - 0100;
			}
		} else
		{
			if (namesame(word, x_("del")) == 0) *boundkey = 0177; else
			if (namesame(word, x_("left")) == 0) *boundspecial |= SPECIALKEYDOWN|(SPECIALKEYARROWL<<SPECIALKEYSH); else
			if (namesame(word, x_("sleft")) == 0) *boundspecial |= SPECIALKEYDOWN|(SPECIALKEYARROWL<<SPECIALKEYSH)|SHIFTDOWN; else
			if (namesame(word, x_("right")) == 0) *boundspecial |= SPECIALKEYDOWN|(SPECIALKEYARROWR<<SPECIALKEYSH); else
			if (namesame(word, x_("sright")) == 0) *boundspecial |= SPECIALKEYDOWN|(SPECIALKEYARROWR<<SPECIALKEYSH)|SHIFTDOWN; else
			if (namesame(word, x_("up")) == 0) *boundspecial |= SPECIALKEYDOWN|(SPECIALKEYARROWU<<SPECIALKEYSH); else
			if (namesame(word, x_("sup")) == 0) *boundspecial |= SPECIALKEYDOWN|(SPECIALKEYARROWU<<SPECIALKEYSH)|SHIFTDOWN; else
			if (namesame(word, x_("down")) == 0) *boundspecial |= SPECIALKEYDOWN|(SPECIALKEYARROWD<<SPECIALKEYSH); else
			if (namesame(word, x_("sdown")) == 0) *boundspecial |= SPECIALKEYDOWN|(SPECIALKEYARROWD<<SPECIALKEYSH)|SHIFTDOWN; else
			if (namesame(word, x_("f1")) == 0) *boundspecial |= SPECIALKEYDOWN|(SPECIALKEYF1<<SPECIALKEYSH); else
			if (namesame(word, x_("f2")) == 0) *boundspecial |= SPECIALKEYDOWN|(SPECIALKEYF2<<SPECIALKEYSH); else
			if (namesame(word, x_("f3")) == 0) *boundspecial |= SPECIALKEYDOWN|(SPECIALKEYF3<<SPECIALKEYSH); else
			if (namesame(word, x_("f4")) == 0) *boundspecial |= SPECIALKEYDOWN|(SPECIALKEYF4<<SPECIALKEYSH); else
			if (namesame(word, x_("f5")) == 0) *boundspecial |= SPECIALKEYDOWN|(SPECIALKEYF5<<SPECIALKEYSH); else
			if (namesame(word, x_("f6")) == 0) *boundspecial |= SPECIALKEYDOWN|(SPECIALKEYF6<<SPECIALKEYSH); else
			if (namesame(word, x_("f7")) == 0) *boundspecial |= SPECIALKEYDOWN|(SPECIALKEYF7<<SPECIALKEYSH); else
			if (namesame(word, x_("f8")) == 0) *boundspecial |= SPECIALKEYDOWN|(SPECIALKEYF8<<SPECIALKEYSH); else
			if (namesame(word, x_("f9")) == 0) *boundspecial |= SPECIALKEYDOWN|(SPECIALKEYF9<<SPECIALKEYSH); else
			if (namesame(word, x_("f10")) == 0) *boundspecial |= SPECIALKEYDOWN|(SPECIALKEYF10<<SPECIALKEYSH); else
			if (namesame(word, x_("f11")) == 0) *boundspecial |= SPECIALKEYDOWN|(SPECIALKEYF11<<SPECIALKEYSH); else
			if (namesame(word, x_("f12")) == 0) *boundspecial |= SPECIALKEYDOWN|(SPECIALKEYF12<<SPECIALKEYSH);
		}
	}
	*pt++ = save;
	offset = pt - binding;
	return(origbinding+offset);	
}

/*
 * Routine to recursively replace all mention of popup menu "oldpm" with "newpm".
 */
void us_recursivelyreplacemenu(POPUPMENU *pm, POPUPMENU *oldpm, POPUPMENU *newpm)
{
	REGISTER INTBIG i;
	REGISTER POPUPMENUITEM *mi;
	REGISTER USERCOM *uc;
	REGISTER VARIABLE *var;
	CHAR varname[200], **strings;

	for(i=0; i<pm->total; i++)
	{
		mi = &pm->list[i];
		uc = mi->response;
		if (uc->menu == NOPOPUPMENU) continue;

		if (uc->menu == oldpm)
		{
			esnprintf(varname, 200, x_("USER_binding_popup_%s"), pm->name);
			var = getval((INTBIG)us_tool, VTOOL, -1, varname);
			strings = (CHAR **)var->addr;
			setind((INTBIG)us_tool, VTOOL, varname, i+1, (INTBIG)strings[i+1]);
		}
		us_recursivelyreplacemenu(uc->menu, oldpm, newpm);
	}
}

/*
 * Routine to adjust the quick keys according to the variable "var".
 */
void us_adjustquickkeys(VARIABLE *var, BOOLEAN warnofchanges)
{
	REGISTER INTBIG len, i, j, clen, elen;
	INTSML key;
	INTBIG special;
	REGISTER CHAR *keybinding, *menuname, *menucommand, *pt, **quickkeylist,
		*expected;
	REGISTER POPUPMENU *pm;
	REGISTER POPUPMENUITEM *mi;
	REGISTER void *infstr;

	/* first scan all existing menus and make sure they are still in the quick key list */
	len = getlength(var);
	quickkeylist = (CHAR **)var->addr;
	for(i=0; i<us_pulldownmenucount; i++)
		us_scanquickkeys(us_pulldowns[i], quickkeylist, len, &warnofchanges);

	/* next scan the list of quick keys and make sure they are attached to menus */
	for(i=0; i<len; i++)
	{
		keybinding = quickkeylist[i];
		menuname = us_getboundkey(keybinding, &key, &special);
		for(pt = menuname; *pt != 0 && *pt != '/'; pt++) ;
		if (*pt == 0) continue;
		*pt = 0;
		pm = us_getpopupmenu(menuname);
		*pt = '/';
		if (pm == NOPOPUPMENU) continue;
		menucommand = pt + 1;
		mi = NOPOPUPMENUITEM;
		for(j=0; j<pm->total; j++)
		{
			mi = &pm->list[j];
			if (namesame(us_removeampersand(mi->attribute), menucommand) == 0) break;
		}
		if (j >= pm->total) continue;

		/* see if this menu item has the quick key */
		expected = us_describeboundkey(key, special, -1);
		elen = estrlen(expected);
		clen = estrlen(mi->attribute);
		if (mi->attribute[clen-1] == '<') clen--;
		if (clen-elen > 0 &&
			namesamen(&mi->attribute[clen-elen], expected, elen) == 0) continue;

		/* not there: see if there is another key bound */
		infstr = initinfstr();
		for(pt = mi->attribute; *pt != 0; pt++)
		{
			if (*pt == '/' || *pt == '\\' || *pt == '<') break;
			addtoinfstr(infstr, *pt);
		}
		addstringtoinfstr(infstr, us_describeboundkey(key, special, -1));
		clen = estrlen(mi->attribute) - 1;
		if (mi->attribute[clen] == '<') addstringtoinfstr(infstr, x_("<"));
		pt = returninfstr(infstr);
		(void)reallocstring(&mi->attribute, pt, us_tool->cluster);
		nativemenurename(pm, j);
		us_adjustpopupmenu(pm, j);

		/* make it the Meta key */
		infstr = initinfstr();
		addstringtoinfstr(infstr, us_describeboundkey(key, special, 0));
		addstringtoinfstr(infstr, x_("command="));
		addstringtoinfstr(infstr, mi->response->comname);
		us_appendargs(infstr, mi->response);
		us_setkeybinding(returninfstr(infstr), key, special, TRUE);

		if (warnofchanges)
		{
			warnofchanges = FALSE;
			ttyputmsg(_("Warning: key bindings have been changed by this library"));
			ttyputmsg(_("  (for example, the '%s' key is now bound to '%s')"),
				us_describeboundkey(key, special, 1), us_removeampersand(mi->attribute));
		}
	}
}

/*
 * Helper routine for "us_quickkeydlog" to recursively examine menu "pm" and
 * load the quick keys tables.
 */
void us_scanquickkeys(POPUPMENU *pm, CHAR **quickkeylist, INTBIG quickkeycount, BOOLEAN *warnofchanges)
{
	REGISTER INTBIG j, i, checked, len;
	INTBIG special;
	REGISTER POPUPMENUITEM *mi;
	REGISTER USERCOM *uc;
	REGISTER POPUPMENU *thispm;
	CHAR *pt, menuline[200], menukey[50];
	REGISTER CHAR *mname, *menuname;
	INTSML key;

	for(i=0; i<pm->total; i++)
	{
		mi = &pm->list[i];
		uc = mi->response;
		if (uc->menu != NOPOPUPMENU)
		{
			us_scanquickkeys(uc->menu, quickkeylist, quickkeycount, warnofchanges);
			continue;
		}
		if (uc->active < 0 && *mi->attribute == 0) continue;

		/* see if this item has a quick key */
		estrcpy(menuline, mi->attribute);
		j = estrlen(menuline) - 1;
		checked = 0;
		if (menuline[j] == '<')
		{
			menuline[j] = 0;
			checked = 1;
		}
		for(mname = menuline; *mname != 0; mname++)
			if (*mname == '/' || *mname == '\\') break;
		if (*mname == 0) continue;
		estrcpy(menukey, &mname[1]);
		mname[1] = 0;
		estrcat(menukey, mname);
		len = estrlen(menukey);

		/* item has a quick key: see if it is in the list */
		for(j=0; j<quickkeycount; j++)
		{
			if (estrncmp(quickkeylist[j], menukey, len) != 0) continue;
			menuname = us_getboundkey(quickkeylist[j], &key, &special);
			for(pt = menuname; *pt != 0 && *pt != '/'; pt++) ;
			if (*pt == 0) continue;
			*pt = 0;
			thispm = us_getpopupmenu(menuname);
			*pt = '/';
			if (thispm != pm) continue;
			if (estrncmp(quickkeylist[j], menukey, len) != 0) continue;
			if (namesame(&pt[1], us_removeampersand(mi->attribute)) == 0) break;
		}
		if (j < quickkeycount) continue;

		/* remove the Meta key */
		(void)us_getboundkey(menukey, &key, &special);
		us_setkeybinding(x_(""), key, special, TRUE);

		/* this menu entry is not in the quick key list: remove its quick key */
		for(mname = mi->attribute; *mname != 0; mname++)
			if (*mname == '/' || *mname == '\\') break;
		if (*mname != 0)
		{
			if (checked != 0) *mname++ = '<';
			*mname = 0;
		}
		nativemenurename(pm, i);
		us_adjustpopupmenu(pm, i);
		if (*warnofchanges)
		{
			*warnofchanges = FALSE;
			ttyputmsg(_("Warning: key bindings have been changed by this library"));
			pt = us_removeampersand(mi->attribute);
			if (*pt == '>') pt++;
			ttyputmsg(_("  (for example, the '%s' command is no longer attached to key '%s')"),
				pt, us_describeboundkey(key, special, 1));
		}
	}
}

/*
 * Helper routine to remove special characters from menu item "name".
 */
CHAR *us_removeampersand(CHAR *name)
{
	REGISTER CHAR *pt;
	REGISTER INTBIG len;
	REGISTER void *infstr;

	len = estrlen(name);
	if (name[len-1] == '<' && name[0] == '>') name++;
	infstr = initinfstr();
	for(pt = name; *pt != 0; pt++)
	{
		if (*pt == '/' || *pt == '\\') break;
		if (*pt == '<' || *pt == '&') continue;
		addtoinfstr(infstr, *pt);
	}
	return(returninfstr(infstr));
}

/*
 * Routine to search all of the pulldown menus and find the entry named "name".
 * Returns the variable key for that menu and the item index.
 */
void us_findnamedmenu(CHAR *name, INTBIG *key, INTBIG *item)
{
	REGISTER POPUPMENU *pm;
	REGISTER INTBIG i;
	REGISTER void *infstr;

	for(pm = us_firstpopupmenu; pm != NOPOPUPMENU; pm = pm->nextpopupmenu)
	{
		for(i=0; i<pm->total; i++)
			if (us_matchpurecommand(pm->list[i].attribute, name)) break;
		if (i < pm->total)
		{
			infstr = initinfstr();
			formatinfstr(infstr, x_("USER_binding_popup_%s"), pm->name);
			*key = makekey(returninfstr(infstr));
			*item = i+1;
			break;
		}
	}
}

/*
 * Routine to take item "menuitem" of the popup at variable "menukey" and substitute "command"
 * for the text there (preserving quick-key bindings).
 */
CHAR *us_plugincommand(INTBIG menukey, INTBIG menuitem, CHAR *command)
{
	REGISTER VARIABLE *var;
	REGISTER CHAR *pt;
	REGISTER INTBIG i;
	REGISTER void *infstr;

	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, menukey);
	if (var == NOVARIABLE) return(x_(""));
	if (menuitem >= getlength(var)) return(x_(""));
	pt = ((CHAR **)var->addr)[menuitem];

	/* look for "message=" and substitute the result */
	infstr = initinfstr();
	for(i=0; pt[i] != 0; i++)
	{
		if (namesamen(&pt[i], x_("message=\""), 9) == 0) break;
		addtoinfstr(infstr, pt[i]);
	}
	if (pt[i] != 0)
	{
		/* do the substitution */
		formatinfstr(infstr, x_("message=\"%s\""), command);
		pt += 9;
		while (*pt != 0 && *pt != '"') pt++;
		if (*pt == '"') pt++;
		addstringtoinfstr(infstr, pt);
	}
	return(returninfstr(infstr));
}

/*
 * Routine to compare command "command" from the pulldown menus with "purecommand" that doesn't
 * have any menu control information.  Returns TRUE if they are the same.
 */
BOOLEAN us_matchpurecommand(CHAR *command, CHAR *purecommand)
{
	CHAR c, pc;

	for(;;)
	{
		if (*command == '&') command++;
		c = *command++;
		if (c == '/' || c == '\\') c = 0;
		pc = *purecommand++;
		if (c != pc) return(FALSE);
		if (c == 0) break;
	}
	return(TRUE);
}

/*
 * routine to determine whether the command bound to key "key" is the
 * last instance of the "telltool user" command that is bound to a key.
 * This is important to know because if the last "telltool user" is unbound,
 * there is no way to execute any long commands!
 */
BOOLEAN us_islasteval(INTSML key, INTBIG special)
{
	REGISTER INTBIG i, j, keytotal, foundanother;
	REGISTER BOOLEAN retval;
	INTBIG boundspecial;
	INTSML boundkey;
	REGISTER VARIABLE *var;
	CHAR *pt;
	COMMANDBINDING commandbindingthis, commandbindingother;

	/* get the command on this key */
	retval = FALSE;
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_keys_key);
	if (var == NOVARIABLE) return(FALSE);
	i = us_findboundkey(key, special, &pt);
	if (i < 0) return(FALSE);
	us_parsebinding(pt, &commandbindingthis);
	if (*commandbindingthis.command != 0)
	{
		/* see if it is "telltool user" */
		if (namesame(commandbindingthis.command, x_("telltool user")) == 0)
		{
			/* this is "telltool user"...check all other keys for this command */
			keytotal = getlength(var);
			foundanother = 0;
			for(j=0; j<keytotal; j++)
			{
				pt = us_getboundkey(((CHAR **)var->addr)[j], &boundkey, &boundspecial);
				if (us_samekey(key, special, boundkey, boundspecial)) continue;
				us_parsebinding(pt, &commandbindingother);
				if (*commandbindingother.command != 0 &&
					namesame(commandbindingother.command, x_("telltool user")) == 0) foundanother++;
				us_freebindingparse(&commandbindingother);
				if (foundanother != 0) break;
			}
			if (foundanother == 0) retval = TRUE;
		}
	}
	us_freebindingparse(&commandbindingthis);
	return(retval);
}

/*
 * routine to parse the binding string in "st" and fill in the binding structure
 * in "commandbinding".
 */
void us_parsebinding(CHAR *st, COMMANDBINDING *commandbinding)
{
	CHAR *pt;
	REGISTER CHAR *str;
	REGISTER void *infstr;

	commandbinding->nodeglyph = NONODEPROTO;
	commandbinding->arcglyph = NOARCPROTO;
	commandbinding->backgroundcolor = 0;
	commandbinding->menumessagesize = TXTSETPOINTS(20);
	commandbinding->menumessage = 0;
	commandbinding->popup = NOPOPUPMENU;
	commandbinding->inputpopup = FALSE;
	commandbinding->command = 0;

	pt = st;
	for(;;)
	{
		str = getkeyword(&pt, x_("= "));
		if (str == NOSTRING || *str == 0) break;
		(void)tonextchar(&pt);

		if (namesame(str, x_("node")) == 0)
		{
			str = getkeyword(&pt, x_(" "));
			if (str == NOSTRING) break;
			commandbinding->nodeglyph = getnodeproto(str);
			continue;
		}
		if (namesame(str, x_("arc")) == 0)
		{
			str = getkeyword(&pt, x_(" "));
			if (str == NOSTRING) break;
			commandbinding->arcglyph = getarcproto(str);
			continue;
		}
		if (namesame(str, x_("background")) == 0)
		{
			str = getkeyword(&pt, x_(" "));
			if (str == NOSTRING) break;
			commandbinding->backgroundcolor = myatoi(str);
			continue;
		}
		if (namesame(str, x_("popup")) == 0)
		{
			str = getkeyword(&pt, x_(" "));
			if (str == NOSTRING) break;
			commandbinding->popup = us_getpopupmenu(str);
			continue;
		}
		if (namesame(str, x_("inputpopup")) == 0)
		{
			str = getkeyword(&pt, x_(" "));
			if (str == NOSTRING) break;
			commandbinding->popup = us_getpopupmenu(str);
			commandbinding->inputpopup = TRUE;
			continue;
		}
		if (namesame(str, x_("message")) == 0)
		{
			(void)tonextchar(&pt);
			infstr = initinfstr();
			for(str = pt; *str != 0; str++)
			{
				if (*str == '"') break;
				if (*str == '^') str++;
				addtoinfstr(infstr, *str);
			}
			pt = str+1;
			(void)allocstring(&commandbinding->menumessage, returninfstr(infstr), us_tool->cluster);
			continue;
		}
		if (namesame(str, x_("messagesize")) == 0)
		{
			str = getkeyword(&pt, x_(" "));
			if (str == NOSTRING) break;
			commandbinding->menumessagesize = myatoi(str);
		}
		if (namesame(str, x_("command")) == 0) break;
	}
	(void)allocstring(&commandbinding->command, pt, us_tool->cluster);
}

/*
 * routine to free any allocated things in the "commandbinding" structure that was filled-in
 * by "us_parsebinding()".
 */
void us_freebindingparse(COMMANDBINDING *commandbinding)
{
	if (commandbinding->menumessage != 0) efree(commandbinding->menumessage);
	commandbinding->menumessage = 0;
	if (commandbinding->command != 0) efree(commandbinding->command);
	commandbinding->command = 0;
}

/*
 * routine to set the trace information in the "size" coordinate pairs in
 * "newlist" onto the node "ni".
 */
void us_settrace(NODEINST *ni, INTBIG *newlist, INTBIG size)
{
	REGISTER INTBIG lx, hx, ly, hy, x, y, i;
	INTBIG lxo, hxo, lyo, hyo;

	/* get the extent of the data */
	lx = hx = newlist[0];   ly = hy = newlist[1];
	for(i=1; i<size; i++)
	{
		x = newlist[i*2];
		y = newlist[i*2+1];
		lx = mini(lx, x);   hx = maxi(hx, x);
		ly = mini(ly, y);   hy = maxi(hy, y);
	}

	/* make these co-ordinates relative to the center */
	x = (hx+lx) / 2;   y = (hy+ly) / 2;
	for(i=0; i<size; i++)
	{
		newlist[i*2] = newlist[i*2] - x;
		newlist[i*2+1] = newlist[i*2+1] - y;
	}

	/* adjust size for node size offset */
	nodesizeoffset(ni, &lxo, &lyo, &hxo, &hyo);
	lx -= lxo;   hx += hxo;
	ly -= lyo;   hy += hyo;

	/* erase the node instance */
	startobjectchange((INTBIG)ni, VNODEINST);

	/* change the trace data */
	(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)newlist,
		VINTEGER|VISARRAY|((size*2)<<VLENGTHSH));

	/* scale the node (which redraws too) */
	if (ni->proto->primindex != 0 && (lx != ni->lowx || hx != ni->highx || ly != ni->lowy ||
		hy != ni->highy || ni->rotation != 0 || ni->transpose != 0))
			modifynodeinst(ni, lx-ni->lowx, ly-ni->lowy, hx-ni->highx, hy-ni->highy,
				-ni->rotation, ni->transpose);

	/* redisplay */
	endobjectchange((INTBIG)ni, VNODEINST);

	/* restore original data */
	for(i=0; i<size; i++)
	{
		newlist[i*2] = newlist[i*2] + x;
		newlist[i*2+1] = newlist[i*2+1] + y;
	}
}

/*
 * routine to scale the trace information on node "ni" given that it will change
 * in size to "nlx"->"nhx" and "nly"->"nhy".
 */
void us_scaletraceinfo(NODEINST *ni, INTBIG nlx, INTBIG nhx, INTBIG nly, INTBIG nhy)
{
	REGISTER VARIABLE *var;
	REGISTER INTBIG *newlist, oldx, oldy, denx, deny, len, i;

	/* stop now if no trace information */
	var = gettrace(ni);
	if (var == NOVARIABLE) return;

	/* get new array for new trace */
	len = getlength(var);
	newlist = (INTBIG *)emalloc(len * SIZEOFINTBIG, el_tempcluster);
	if (newlist == 0) return;

	/* copy the data and scale it */
	denx = ni->highx - ni->lowx;
	if (denx == 0) denx = nhx - nlx;
	deny = ni->highy - ni->lowy;
	if (deny == 0) deny = nhy - nly;
	for(i=0; i<len; i += 2)
	{
		oldx = ((INTBIG *)var->addr)[i];
		oldy = ((INTBIG *)var->addr)[i+1];
		oldx = muldiv(oldx, nhx - nlx, denx);
		oldy = muldiv(oldy, nhy - nly, deny);
		newlist[i] = oldx;   newlist[i+1] = oldy;
	}

	/* store the new list */
	(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)newlist,
		VINTEGER|VISARRAY|(len<<VLENGTHSH));
	efree((CHAR *)newlist);
}

/*
 * Routine to fillet the two highlighted objects
 */
void us_dofillet(void)
{
	REGISTER VARIABLE *var;
	HIGHLIGHT thishigh, otherhigh;
	REGISTER NODEINST *ni1, *ni2, *swapni;
	double startoffset, endangle, srot, erot, newangle, dx, dy;
	INTBIG ix, iy, ix1, iy1, ix2, iy2;
	REGISTER BOOLEAN on1, on2;
	REGISTER INTBIG ang1, ang2, size1, size2, arc1, arc2, swapsize, icount, newrot;
	REGISTER INTBIG *newlist1, *newlist2, *line1xs, *line1ys, *line1xe, *line1ye,
		*line2xs, *line2ys, *line2xe, *line2ye, x, y, i, *swaplist;
	XARRAY trans;

	/* must be exactly two nodes selected */
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var == NOVARIABLE)
	{
		us_abortcommand(_("Must select two nodes before filleting"));
		return;
	}
	if (getlength(var) != 2)
	{
		us_abortcommand(_("Must select two nodes before filleting"));
		return;
	}

	if (us_makehighlight(((CHAR **)var->addr)[0], &thishigh) ||
		us_makehighlight(((CHAR **)var->addr)[1], &otherhigh)) return;

	/* get the two objects */
	if ((thishigh.status&HIGHTYPE) != HIGHFROM || (otherhigh.status&HIGHTYPE) != HIGHFROM)
	{
		us_abortcommand(_("Must select two nodes before filleting"));
		return;
	}

	if (!thishigh.fromgeom->entryisnode || !otherhigh.fromgeom->entryisnode)
	{
		us_abortcommand(_("Must select two nodes before filleting"));
		return;
	}

	/* get description of first node */
	ni1 = thishigh.fromgeom->entryaddr.ni;
	if (ni1->proto == art_circleprim || ni1->proto == art_thickcircleprim)
	{
		getarcdegrees(ni1, &startoffset, &endangle);
		if (startoffset == 0.0 && endangle == 0.0)
		{
			us_abortcommand(_("Must select arcs, not circles before filleting"));
			return;
		}
		newlist1 = emalloc((6*SIZEOFINTBIG), el_tempcluster);
		if (newlist1 == 0) return;
		newlist1[0] = (ni1->lowx + ni1->highx) / 2;
		newlist1[1] = (ni1->lowy + ni1->highy) / 2;
		getarcendpoints(ni1, startoffset, endangle, &newlist1[2], &newlist1[3],
			&newlist1[4], &newlist1[5]);
		size1 = 0;
		arc1 = 1;
	} else if (ni1->proto == art_openedpolygonprim || ni1->proto == art_openeddottedpolygonprim ||
		ni1->proto == art_openeddashedpolygonprim || ni1->proto == art_openedthickerpolygonprim ||
		ni1->proto == art_closedpolygonprim)
	{
		var = gettrace(ni1);
		if (var == NOVARIABLE)
		{
			us_abortcommand(_("Must select nodes with outline information before filleting"));
			return;
		}

		/* transform the traces */
		size1 = getlength(var) / 2;
		newlist1 = emalloc((size1*2*SIZEOFINTBIG), el_tempcluster);
		if (newlist1 == 0) return;
		makerot(ni1, trans);
		x = (ni1->highx + ni1->lowx) / 2;
		y = (ni1->highy + ni1->lowy) / 2;
		for(i=0; i<size1; i++)
			xform(((INTBIG *)var->addr)[i*2]+x, ((INTBIG *)var->addr)[i*2+1]+y, &newlist1[i*2],
				&newlist1[i*2+1], trans);
		arc1 = 0;
	} else
	{
		us_abortcommand(_("Node %s cannot be filleted"), describenodeinst(ni1));
		return;
	}

	/* get description of second node */
	ni2 = otherhigh.fromgeom->entryaddr.ni;
	if (ni2->proto == art_circleprim || ni2->proto == art_thickcircleprim)
	{
		getarcdegrees(ni2, &startoffset, &endangle);
		if (startoffset == 0.0 && endangle == 0.0)
		{
			us_abortcommand(_("Must select arcs, not circles before filleting"));
			return;
		}
		newlist2 = emalloc((6*SIZEOFINTBIG), el_tempcluster);
		if (newlist2 == 0) return;
		newlist2[0] = (ni2->lowx + ni2->highx) / 2;
		newlist2[1] = (ni2->lowy + ni2->highy) / 2;
		getarcendpoints(ni2, startoffset, endangle, &newlist2[2], &newlist2[3],
			&newlist2[4], &newlist2[5]);
		size2 = 0;
		x = y = 0;
		arc2 = 1;
	} else if (ni2->proto == art_openedpolygonprim || ni2->proto == art_openeddottedpolygonprim ||
		ni2->proto == art_openeddashedpolygonprim || ni2->proto == art_openedthickerpolygonprim ||
		ni2->proto == art_closedpolygonprim)
	{
		var = gettrace(ni2);
		if (var == NOVARIABLE)
		{
			us_abortcommand(_("Must select nodes with outline information before filleting"));
			return;
		}

		/* transform the traces */
		size2 = getlength(var) / 2;
		newlist2 = emalloc((size2*2*SIZEOFINTBIG), el_tempcluster);
		if (newlist2 == 0) return;
		makerot(ni2, trans);
		x = (ni2->highx + ni2->lowx) / 2;
		y = (ni2->highy + ni2->lowy) / 2;
		for(i=0; i<size2; i++)
			xform(((INTBIG *)var->addr)[i*2]+x, ((INTBIG *)var->addr)[i*2+1]+y, &newlist2[i*2],
				&newlist2[i*2+1], trans);
		arc2 = 0;
	} else
	{
		us_abortcommand(_("Node %s cannot be filleted"), describenodeinst(ni2));
		return;
	}

	/* handle different types of filleting */
	if (arc1 != 0 && arc2 != 0)
	{
		/* cannot handle arc-to-arc filleting */
		us_abortcommand(_("Cannot fillet two curves"));
	} else if (arc1 == 0 && arc2 == 0)
	{
		/* handle line-to-line filleting: find out which endpoints are closest */
		if (computedistance(x,y, newlist1[0],newlist1[1]) <
			computedistance(x,y, newlist1[size1*2-2],newlist1[size1*2-1]))
		{
			line1xs = &newlist1[0];
			line1ys = &newlist1[1];
			line1xe = &newlist1[2];
			line1ye = &newlist1[3];
		} else
		{
			line1xs = &newlist1[size1*2-2];
			line1ys = &newlist1[size1*2-1];
			line1xe = &newlist1[size1*2-4];
			line1ye = &newlist1[size1*2-3];
		}
		if (computedistance(*line1xs,*line1ys, newlist2[0],newlist2[1]) <
			computedistance(*line1xs,*line1ys, newlist2[size2*2-2],newlist2[size2*2-1]))
		{
			line2xs = &newlist2[0];
			line2ys = &newlist2[1];
			line2xe = &newlist2[2];
			line2ye = &newlist2[3];
		} else
		{
			line2xs = &newlist2[size2*2-2];
			line2ys = &newlist2[size2*2-1];
			line2xe = &newlist2[size2*2-4];
			line2ye = &newlist2[size2*2-3];
		}

		/* compute intersection point */
		ang1 = figureangle(*line1xs, *line1ys, *line1xe, *line1ye);
		ang2 = figureangle(*line2xs, *line2ys, *line2xe, *line2ye);
		if (intersect(*line1xs, *line1ys, ang1, *line2xs, *line2ys, ang2, &ix, &iy) != 0)
			us_abortcommand(_("Lines do not intersect")); else
		{
			*line1xs = ix;   *line1ys = iy;
			*line2xs = ix;   *line2ys = iy;
			us_pushhighlight();
			us_clearhighlightcount();
			us_settrace(ni1, newlist1, size1);
			us_settrace(ni2, newlist2, size2);
			us_pophighlight(TRUE);
		}
	} else
	{
		/* handle arc-to-line filleting */
		if (arc1 == 0)
		{
			swaplist = newlist1;   newlist1 = newlist2;   newlist2 = swaplist;
			swapsize = size1;      size1 = size2;         size2 = swapsize;
			swapni = ni1;          ni1 = ni2;             ni2 = swapni;
		}

		/* "newlist1" describes the arc, "newlist2" describes the line */
		if (computedistance(newlist1[0],newlist1[1], newlist2[0],newlist2[1]) <
			computedistance(newlist1[0],newlist1[1], newlist2[size2*2-2],newlist2[size2*2-1]))
		{
			line2xs = &newlist2[0];
			line2ys = &newlist2[1];
			line2xe = &newlist2[2];
			line2ye = &newlist2[3];
		} else
		{
			line2xs = &newlist2[size2*2-2];
			line2ys = &newlist2[size2*2-1];
			line2xe = &newlist2[size2*2-4];
			line2ye = &newlist2[size2*2-3];
		}
		icount = circlelineintersection(newlist1[0],newlist1[1], newlist1[2],newlist1[3],
			*line2xs, *line2ys, *line2xe, *line2ye, &ix1, &iy1, &ix2, &iy2, 0);
		if (icount == 0)
		{
			us_abortcommand(_("Line does not intersect arc: cannot fillet"));
		} else
		{
			if (icount == 2)
			{
				on1 = us_pointonexparc(newlist1[0],newlist1[1], newlist1[2],newlist1[3],
					newlist1[4],newlist1[5], ix1, iy1);
				on2 = us_pointonexparc(newlist1[0],newlist1[1], newlist1[2],newlist1[3],
					newlist1[4],newlist1[5], ix2, iy2);
				if (!on1 && on2)
				{
					icount = 1;
					ix1 = ix2;   iy1 = iy2;
				} else if (on1 && !on2)
				{
					icount = 1;
				}
			}
			if (icount == 2)
			{
				x = (*line2xs + *line2xe) / 2;
				y = (*line2ys + *line2ye) / 2;
				if (computedistance(ix1,iy1, x,y) > computedistance(ix2,iy2, x,y))
				{
					ix1 = ix2;   iy1 = iy2;
				}
			}

			/* make them fillet at (ix1,iy1) */
			us_pushhighlight();
			us_clearhighlightcount();

			/* adjust the arc (node ni1) */
			dx = (double)(newlist1[2]-newlist1[0]);   dy = (double)(newlist1[3]-newlist1[1]);
			if (dx == 0.0 && dy == 0.0)
			{
				us_abortcommand(_("Domain error during fillet"));
				return;
			}
			srot = atan2(dy, dx);
			if (srot < 0.0) srot += EPI*2.0;

			dx = (double)(newlist1[4]-newlist1[0]);   dy = (double)(newlist1[5]-newlist1[1]);
			if (dx == 0.0 && dy == 0.0)
			{
				us_abortcommand(_("Domain error during fillet"));
				return;
			}
			erot = atan2(dy, dx);
			if (erot < 0.0) erot += EPI*2.0;

			dx = (double)(ix1-newlist1[0]);   dy = (double)(iy1-newlist1[1]);
			if (dx == 0.0 && dy == 0.0)
			{
				us_abortcommand(_("Domain error during fillet"));
				return;
			}
			newangle = atan2(dy, dx);
			if (newangle < 0.0) newangle += EPI*2.0;
			if (computedistance(ix1,iy1, newlist1[2],newlist1[3]) <
				computedistance(ix1,iy1, newlist1[4],newlist1[5])) srot = newangle; else
					erot = newangle;
			erot -= srot;
			if (erot < 0.0) erot += EPI*2.0;
			newrot = rounddouble(srot * 1800.0 / EPI);
			srot -= ((double)newrot) * EPI / 1800.0;
			startobjectchange((INTBIG)ni1, VNODEINST);
			modifynodeinst(ni1, 0, 0, 0, 0, newrot - ni1->rotation, 0);
			setarcdegrees(ni1, srot, erot);
			endobjectchange((INTBIG)ni1, VNODEINST);

			/* adjust the line (node ni2) */
			*line2xs = ix1;   *line2ys = iy1;
			us_settrace(ni2, newlist2, size2);

			/* restore highlighting */
			us_pophighlight(TRUE);
		}
	}
	efree((CHAR *)newlist1);
	efree((CHAR *)newlist2);
}

/*
 * Houtine to convert the text in "msg" to bits on the display.
 */
void us_layouttext(CHAR *layer, INTBIG tsize, INTBIG scale, INTBIG font, INTBIG italic,
	INTBIG bold, INTBIG underline, INTBIG separation, CHAR *msg)
{
	REGISTER NODEPROTO *np;
	REGISTER INTBIG x, y;
	REGISTER NODEINST *ni, **nodelist;
	REGISTER BOOLEAN err;
	REGISTER INTBIG cx, cy, lambda, gridsize, i;
	INTBIG wid, hei, bx, by, count;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	HIGHLIGHT high;
	UCHAR1 **rowstart, *data;
	REGISTER void *merge=0;
	LISTINTBIG *linodes;
	static POLYGON *poly = NOPOLYGON;

	np = us_needcell();
	if (np == NONODEPROTO) return;

	/* convert the text to bits */
	TDCLEAR(descript);
	if (tsize < 4) tsize = 4;
	if (tsize > TXTMAXPOINTS) tsize = TXTMAXPOINTS;
	TDSETSIZE(descript, TXTSETPOINTS(tsize));
	TDSETFACE(descript, font);
	if (italic != 0) TDSETITALIC(descript, VTITALIC);
	if (bold != 0) TDSETBOLD(descript, VTBOLD);
	if (underline != 0) TDSETUNDERLINE(descript, VTUNDERLINE);
	screensettextinfo(el_curwindowpart, NOTECHNOLOGY, descript);
	err = gettextbits(el_curwindowpart, msg, &wid, &hei, &rowstart);
	if (err)
	{
		us_abortcommand(_("Sorry, this system cannot layout text"));
		return;
	}

	/* determine the primitive to use for the layout */
	for(us_layouttextprim = el_curtech->firstnodeproto; us_layouttextprim != NONODEPROTO;
		us_layouttextprim = us_layouttextprim->nextnodeproto)
			if (namesame(us_layouttextprim->protoname, layer) == 0) break;
	if (us_layouttextprim == NONODEPROTO)
	{
		us_abortcommand(_("Cannot find '%s' node"), layer);
		return;
	}

	lambda = el_curlib->lambda[el_curtech->techindex];
	separation = separation * lambda / 2;
	gridsize = lambda * scale;
	bx = (el_curwindowpart->screenlx+el_curwindowpart->screenhx -
		wid * gridsize) / 2;
	by = (el_curwindowpart->screenly+el_curwindowpart->screenhy -
		hei * gridsize) / 2;
	gridalign(&bx, &by, 1, np);
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);
	if (separation == 0) merge = mergenew(us_tool->cluster); else
		linodes = newintlistobj(us_tool->cluster);

	for(y=0; y<hei; y++)
	{
		cy = by - y * gridsize;
		data = rowstart[y];
		for(x=0; x<wid; x++)
		{
			cx = bx + x * gridsize;
			if (*data != 0)
			{
				poly->xv[0] = cx;
				poly->yv[0] = cy;
				poly->xv[1] = cx + gridsize;
				poly->yv[1] = cy + gridsize;
				poly->style = FILLEDRECT;
				poly->count = 2;
				if (separation == 0) mergeaddpolygon(merge, 0, el_curtech, poly); else
				{
					/* place the node now */
					ni = newnodeinst(us_layouttextprim, poly->xv[0]+separation,
						poly->xv[1]-separation, poly->yv[0]+separation,
						poly->yv[1]-separation, 0, 0, np);
					if (ni != NONODEINST)
						addtointlistobj(linodes, (INTBIG)ni, FALSE);
				}
			}
			data++;
		}
	}
	us_clearhighlightcount();
	if (separation == 0)
	{
		mergeextract(merge, us_layouttextpolygon);
		mergedelete(merge);
	} else
	{
		nodelist = (NODEINST **)getintlistobj(linodes, &count);
		for(i=0; i<count; i++)
		{
			ni = nodelist[i];
			endobjectchange((INTBIG)ni, VNODEINST);
			high.status = HIGHFROM;
			high.fromgeom = ni->geom;
			high.fromport = NOPORTPROTO;
			high.frompoint = 0;
			high.cell = np;
			us_addhighlight(&high);
		}
	}
}

/*
 * Helper routine for "us_layouttext" to process a polygon and convert it to layout.
 */
void us_layouttextpolygon(INTBIG layer, TECHNOLOGY *tech, INTBIG *x, INTBIG *y, INTBIG count)
{
	REGISTER NODEPROTO *cell;
	REGISTER NODEINST *ni;
	REGISTER INTBIG lx, hx, ly, hy, cx, cy, *newlist;
	REGISTER INTBIG i;
	HIGHLIGHT high;
	Q_UNUSED( layer );
	Q_UNUSED( tech );

	cell = us_needcell();
	if (cell == NONODEPROTO) return;
	lx = hx = x[0];
	ly = hy = y[0];
	for(i=1; i<count; i++)
	{
		if (x[i] < lx) lx = x[i];
		if (x[i] > hx) hx = x[i];
		if (y[i] < ly) ly = y[i];
		if (y[i] > hy) hy = y[i];
	}
	cx = (lx+hx) / 2;   cy = (ly+hy) / 2;
	ni = newnodeinst(us_layouttextprim, lx, hx, ly, hy, 0, 0, cell);
	newlist = (INTBIG *)emalloc(count * 2 * SIZEOFINTBIG, el_tempcluster);
	if (newlist == 0) return;
	for(i=0; i<count; i++)
	{
		newlist[i*2] = x[i] - cx;
		newlist[i*2+1] = y[i] - cy;
	}
	(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)newlist,
		VINTEGER|VISARRAY|((count*2)<<VLENGTHSH));
	endobjectchange((INTBIG)ni, VNODEINST);
	high.status = HIGHFROM;
	high.fromgeom = ni->geom;
	high.fromport = NOPORTPROTO;
	high.frompoint = 0;
	high.cell = cell;
	us_addhighlight(&high);
}

/*
 * Routine to determine whether the point (x,y) is on the arc centered at (cx,cy), starting
 * at (sx,sy), and ending at (ex,ey).  Returns true if on the arc.
 */
BOOLEAN us_pointonexparc(INTBIG cx, INTBIG cy, INTBIG sx, INTBIG sy, INTBIG ex, INTBIG ey, INTBIG x, INTBIG y)
{
	REGISTER INTBIG as, ae, a;

	as = figureangle(cx, cy, sx, sy);
	ae = figureangle(cx, cy, ex, ey);
	a = figureangle(cx, cy, x, y);

	if (ae > as)
	{
		if (a >= as && a <= ae) return(TRUE);
	} else
	{
		if (a >= as || a <= ae) return(TRUE);
	}
	return(FALSE);
}

/*
 * routine to recursively check sub-cell revision times
 * P. Attfield
 */
void us_check_cell_date(NODEPROTO *np, UINTBIG rev_time)
{
	REGISTER NODEPROTO *np2;
	REGISTER NODEINST *ni;

	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		np2 = ni->proto;
		if (np2->primindex != 0) continue; /* ignore if primitive */

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(np2, np)) continue;
		if (np2->temp1 != 0) continue; /* ignore if already seen */
		us_check_cell_date(np2, rev_time); /* recurse */
	}

	/* check this cell */
	np->temp1++; /* flag that we have seen this one */
	if (np->revisiondate <= rev_time) return;

	/* possible error in hierarchy */
	ttyputerr(_("WARNING: sub-cell '%s' has been edited"), describenodeproto(np));
	ttyputmsg(_("         since the last revision to the current cell"));
}

/*
 * routine to switch to library "lib"
 */
void us_switchtolibrary(LIBRARY *lib)
{
	CHAR *newpar[2];
	REGISTER INTBIG oldlam;
	REGISTER WINDOWPART *w;

	/* select the new library */
	us_clearhighlightcount();
	oldlam = el_curlib->lambda[el_curtech->techindex];
	selectlibrary(lib, TRUE);
	us_setlambda(NOWINDOWFRAME);
	if ((us_curnodeproto == NONODEPROTO || us_curnodeproto->primindex == 0) &&
		(us_state&NONPERSISTENTCURNODE) == 0)
			us_setnodeproto(el_curtech->firstnodeproto);
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		us_setcellname(w);

	/* if nothing displayed and new library has top cell, show it */
	if (el_curwindowpart == NOWINDOWPART ||
		el_curwindowpart->curnodeproto == NONODEPROTO)
	{
		if (el_curlib->curnodeproto != NONODEPROTO)
		{
			newpar[0] = describenodeproto(el_curlib->curnodeproto);
			us_editcell(1, newpar);
		}
	}

	/* redo the explorer window (if it is up) */
	us_redoexplorerwindow();
}

/*
 * Routine to replace all cross-library references that point into
 * "oldlib" with equivalent ones that point to "newlib".  This is called
 * when a library is re-read from disk ("oldlib" is the former one).
 */
void us_replacelibraryreferences(LIBRARY *oldlib, LIBRARY *newlib)
{
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np, *newnp;
	REGISTER NODEINST *ni, *nextni, *newni;
	REGISTER ARCINST *ai;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;

	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if (lib == oldlib || lib == newlib) continue;

		/* look for cross-library references to "oldlib" */
		us_correctxlibref(&lib->firstvar, &lib->numvar, oldlib, newlib);
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			us_correctxlibref(&np->firstvar, &np->numvar, oldlib, newlib);
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = nextni)
			{
				nextni = ni->nextnodeinst;

				/* correct variables */
				us_correctxlibref(&ni->firstvar, &ni->numvar, oldlib, newlib);
				us_correctxlibref(&ni->geom->firstvar, &ni->geom->numvar, oldlib, newlib);
				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
					us_correctxlibref(&pi->firstvar, &pi->numvar, oldlib, newlib);
				for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
					us_correctxlibref(&pe->firstvar, &pe->numvar, oldlib, newlib);

				if (ni->proto->primindex != 0) continue;
				if (ni->proto->lib == oldlib)
				{
					/* find the equivalent name in the new library */
					newnp = us_findcellinotherlib(ni->proto, newlib);
					if (newnp == NONODEPROTO)
					{
						ttyputerr(_("Error: cell %s{%s};%ld no longer present in library %s"),
							ni->proto->protoname, ni->proto->cellview->sviewname,
								ni->proto->version, newlib->libname);
						continue;
					}
					newni = replacenodeinst(ni, newnp, FALSE, TRUE);
					if (newni == NONODEINST)
					{
						ttyputerr(_("Error: node %s{%s};%ld could not be replaced with equivalent in library %s"),
							ni->proto->protoname, ni->proto->cellview->sviewname,
								ni->proto->version, newlib->libname);
						continue;
					}
				}
			}
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			{
				us_correctxlibref(&ai->firstvar, &ai->numvar, oldlib, newlib);
				us_correctxlibref(&ai->geom->firstvar, &ai->geom->numvar, oldlib, newlib);
			}
		}
	}
}

/*
 * Helper routine to search "numvar" variables in "firstvar" and replace references
 * to anything in library "oldlib" to point to an equivalent in "newlib".
 */
void us_correctxlibref(VARIABLE **firstvar, INTSML *numvar, LIBRARY *oldlib, LIBRARY *newlib)
{
	REGISTER INTBIG i, j, checkcell;
	REGISTER VARIABLE *var, *svar, *dvar;
	REGISTER NODEPROTO *np, *newnp;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER LIBRARY *lib;
	REGISTER NETWORK *net;
	REGISTER GEOM *geom;

	for(i=0; i < *numvar; i++)
	{
		var = &(*firstvar)[i];
		if ((var->type&VISARRAY) != 0) continue;
		checkcell = 0;
		np = NONODEPROTO;
		switch (var->type&VTYPE)
		{
			case VNODEINST:
				ni = (NODEINST *)var->addr;
				np = ni->parent;
				checkcell = 1;
				break;
			case VNODEPROTO:
				np = (NODEPROTO *)var->addr;
				if (np->primindex != 0) break;
				if (np->lib != oldlib) break;
				newnp = us_findcellinotherlib(np, newlib);
				var->addr = (INTBIG)newnp;
				break;
			case VPORTARCINST:
				pi = (PORTARCINST *)var->addr;
				np = pi->conarcinst->parent;
				checkcell = 1;
				break;
			case VPORTEXPINST:
				pe = (PORTEXPINST *)var->addr;
				np = pe->exportproto->parent;
				checkcell = 1;
				break;
			case VPORTPROTO:
				pp = (PORTPROTO *)var->addr;
				np = pp->parent;
				checkcell = 1;
				break;
			case VARCINST:
				ai = (ARCINST *)var->addr;
				np = ai->parent;
				checkcell = 1;
				break;
			case VGEOM:
				geom = (GEOM *)var->addr;
				np = geomparent(geom);
				checkcell = 1;
				break;
			case VLIBRARY:
				lib = (LIBRARY *)var->addr;
				if (lib != oldlib) break;
				var->addr = (INTBIG)newlib;
				break;
			case VNETWORK:
				net = (NETWORK *)var->addr;
				np = net->parent;
				checkcell = 1;
				break;
		}
		if (checkcell != 0)
		{
			if (np->primindex != 0) continue;
			if (np->lib != oldlib) continue;

			/* cannot correct the reference: delete the variable */
			for(j=i+1; j < *numvar; j++)
			{
				svar = &(*firstvar)[j];
				dvar = &(*firstvar)[j-1];
				*dvar = *svar;
			}
			(*numvar)--;
			i--;
		}
	}
}

/*
 * Helper routine to find the equivalent to cell "cell" in library "lib".
 */
NODEPROTO *us_findcellinotherlib(NODEPROTO *cell, LIBRARY *lib)
{
	REGISTER NODEPROTO *newnp;

	for(newnp = lib->firstnodeproto; newnp != NONODEPROTO; newnp = newnp->nextnodeproto)
	{
		if (namesame(cell->protoname, newnp->protoname) != 0) continue;
		if (cell->cellview != newnp->cellview) continue;
		if (cell->version != newnp->version) continue;
		break;
	}
	return(newnp);
}

/******************** TEXT OBJECTS ********************/

/*
 * routine to recompute the text descriptor in "descript" to change the
 * grab-point according to the location of (xcur, ycur), given that the
 * text centers at (xc, yc) and is "xw" by "yw" in size.
 */
void us_figuregrabpoint(UINTBIG *descript, INTBIG xcur, INTBIG ycur, INTBIG xc,
	INTBIG yc, INTBIG xw, INTBIG yw)
{
	if (xcur < xc - xw/2)
	{
		/* grab-point is on the bottom left */
		if (ycur < yc - yw/2)
		{
			TDSETPOS(descript, VTPOSUPRIGHT);
			return;
		}

		/* grab-point is on the top left */
		if (ycur > yc + yw/2)
		{
			TDSETPOS(descript, VTPOSDOWNRIGHT);
			return;
		}

		/* grab-point is on the left */
		TDSETPOS(descript, VTPOSRIGHT);
		return;
	}

	if (xcur > xc + xw/2)
	{
		/* grab-point is on the bottom right */
		if (ycur < yc - yw/2)
		{
			TDSETPOS(descript, VTPOSUPLEFT);
			return;
		}

		/* grab-point is on the top right */
		if (ycur > yc + yw/2)
		{
			TDSETPOS(descript, VTPOSDOWNLEFT);
			return;
		}

		/* grab-point is on the right */
		TDSETPOS(descript, VTPOSLEFT);
		return;
	}

	/* grab-point is on the bottom */
	if (ycur < yc - yw/2)
	{
		TDSETPOS(descript, VTPOSUP);
		return;
	}

	/* grab-point is on the top */
	if (ycur > yc + yw/2)
	{
		TDSETPOS(descript, VTPOSDOWN);
		return;
	}

	/* grab-point is in the center: check for VERY center */
	if (ycur >= yc - yw/6 && ycur <= yc + yw/6 && xcur >= xc - xw/6 &&
		xcur <= xc + xw/6)
	{
		TDSETPOS(descript, VTPOSBOXED);
		return;
	}

	/* grab-point is simply centered */
	TDSETPOS(descript, VTPOSCENT);
}

/*
 * routine to return the new text descriptor field, given that the old is in
 * "formerdesc".  The instructions for changing this variable are in "count"
 * and "par", where "count" where the first two values are the X and Y offset,
 * and the third value is the text position.  Returns -1 on error.
 */
void us_figurevariableplace(UINTBIG *formerdesc, INTBIG count, CHAR *par[])
{
	INTBIG xval, yval;
	REGISTER INTBIG grab;

	if (count >= 2)
	{
		xval = atofr(par[0]) * 4 / WHOLE;
		yval = atofr(par[1]) * 4 / WHOLE;
		us_setdescriptoffset(formerdesc, xval, yval);
	}
	if (count >= 3)
	{
		grab = us_gettextposition(par[2]);
		TDSETPOS(formerdesc, grab);
	}
}

/*
 * routine to change the X and Y offset factors in the text descriptor
 * "formerdesc" to "xval" and "yval".  Returns the new text descriptor.
 */
void us_setdescriptoffset(UINTBIG *formerdesc, INTBIG xval, INTBIG yval)
{
	REGISTER INTBIG oldx, oldy;

	/* make sure the range is proper */
	oldx = xval;   oldy = yval;
	propervaroffset(&xval, &yval);
	if (xval != oldx || yval != oldy)
		ttyputmsg(_("Text offset adjusted"));

	TDSETOFF(formerdesc, xval, yval);
}

/*
 * routine to rotate the text descriptor in "descript" to account for
 * the rotation of the object on which it resides: "geom".  The new
 * text descriptor is returned.
 */
void us_rotatedescript(GEOM *geom, UINTBIG *descript)
{
	us_rotatedescriptArb(geom, descript, FALSE);
}

/*
 * routine to undo the rotation of the text descriptor in "descript" to account for
 * the rotation of the object on which it resides: "geom".  The new
 * text descriptor is returned.
 */
void us_rotatedescriptI(GEOM *geom, UINTBIG *descript)
{
	us_rotatedescriptArb(geom, descript, TRUE);
}

/*
 * routine to rotate the text descriptor in "descript" to account for
 * the rotation of the object on which it resides: "geom".  Inverts the
 * sense of the rotation if "invert" is true.  The new
 * text descriptor is returned.
 */
void us_rotatedescriptArb(GEOM *geom, UINTBIG *descript, BOOLEAN invert)
{
	REGISTER INTBIG style;
	REGISTER NODEINST *ni;
	XARRAY trans;

	/* arcs do not rotate */
	if (!geom->entryisnode) return;

	switch (TDGETPOS(descript))
	{
		case VTPOSCENT:
		case VTPOSBOXED:     return;
		case VTPOSUP:        style = TEXTBOT;       break;
		case VTPOSDOWN:      style = TEXTTOP;       break;
		case VTPOSLEFT:      style = TEXTRIGHT;     break;
		case VTPOSRIGHT:     style = TEXTLEFT;      break;
		case VTPOSUPLEFT:    style = TEXTBOTRIGHT;  break;
		case VTPOSUPRIGHT:   style = TEXTBOTLEFT;   break;
		case VTPOSDOWNLEFT:  style = TEXTTOPRIGHT;  break;
		case VTPOSDOWNRIGHT: style = TEXTTOPLEFT;   break;
		default:             return;
	}
	ni = geom->entryaddr.ni;
	if (invert)
	{
		if (ni->transpose == 0) makeangle((3600 - ni->rotation)%3600, 0, trans); else
			makeangle(ni->rotation, ni->transpose, trans);
	} else
	{
		makeangle(ni->rotation, ni->transpose, trans);
	}
	style = rotatelabel(style, TDGETROTATION(descript), trans);
	switch (style)
	{
		case TEXTBOT:       TDSETPOS(descript, VTPOSUP);          break;
		case TEXTTOP:       TDSETPOS(descript, VTPOSDOWN);        break;
		case TEXTRIGHT:     TDSETPOS(descript, VTPOSLEFT);        break;
		case TEXTLEFT:      TDSETPOS(descript, VTPOSRIGHT);       break;
		case TEXTBOTRIGHT:  TDSETPOS(descript, VTPOSUPLEFT);      break;
		case TEXTBOTLEFT:   TDSETPOS(descript, VTPOSUPRIGHT);     break;
		case TEXTTOPRIGHT:  TDSETPOS(descript, VTPOSDOWNLEFT);    break;
		case TEXTTOPLEFT:   TDSETPOS(descript, VTPOSDOWNRIGHT);   break;
	}
}

/*
 * routine to adjust the displayable text on node "ni" to account for new
 * size/rotation factors in the node.  This is only done for invisible pins
 * in the generic technology, where the displayable text must track the node.
 */
void us_adjustdisplayabletext(NODEINST *ni)
{
	REGISTER INTBIG i;
	REGISTER INTBIG halfx, halfy, lambda;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	REGISTER VARIABLE *var;

	/* make sure this is the invisible pin */
	if (ni->proto != gen_invispinprim) return;

	/* search for displayable text */
	for(i=0; i<ni->numvar; i++)
	{
		var = &ni->firstvar[i];
		if ((var->type&VDISPLAY) == 0) continue;

		/* compute the proper display offset */
		TDCOPY(descript, var->textdescript);
		lambda = lambdaofnode(ni);
		halfx = (ni->highx - ni->lowx) * 2 / lambda;
		halfy = (ni->highy - ni->lowy) * 2 / lambda;
		switch (TDGETPOS(descript))
		{
			case VTPOSCENT:
			case VTPOSBOXED:
				us_setdescriptoffset(descript, 0, 0);
				break;
			case VTPOSUP:
				us_setdescriptoffset(descript, 0, -halfy);
				break;
			case VTPOSDOWN:
				us_setdescriptoffset(descript, 0, halfy);
				break;
			case VTPOSLEFT:
				us_setdescriptoffset(descript, halfx, 0);
				break;
			case VTPOSRIGHT:
				us_setdescriptoffset(descript, -halfx, 0);
				break;
			case VTPOSUPLEFT:
				us_setdescriptoffset(descript, halfx, -halfy);
				break;
			case VTPOSUPRIGHT:
				us_setdescriptoffset(descript, -halfx, -halfy);
				break;
			case VTPOSDOWNLEFT:
				us_setdescriptoffset(descript, halfx, halfy);
				break;
			case VTPOSDOWNRIGHT:
				us_setdescriptoffset(descript, -halfx, halfy);
				break;
		}
		modifydescript((INTBIG)ni, VNODEINST, var, descript);
	}
}

/*
 * routine to parse the "grab-point" specification in "pp" and return the
 * code.  Prints an error message on error.
 */
INTBIG us_gettextposition(CHAR *pp)
{
	REGISTER INTBIG l;

	l = estrlen(pp);
	if (namesamen(pp, x_("centered"), l) == 0 && l >= 1) return(VTPOSCENT);
	if (namesamen(pp, x_("boxed"), l) == 0 && l >= 1) return(VTPOSBOXED);
	if (namesame(pp, x_("up")) == 0) return(VTPOSUP);
	if (namesame(pp, x_("down")) == 0) return(VTPOSDOWN);
	if (namesamen(pp, x_("left"), l) == 0 && l >= 1) return(VTPOSLEFT);
	if (namesamen(pp, x_("right"), l) == 0 && l >= 1) return(VTPOSRIGHT);
	if (namesamen(pp, x_("up-left"), l) == 0 && l >= 4) return(VTPOSUPLEFT);
	if (namesamen(pp, x_("up-right"), l) == 0 && l >= 4) return(VTPOSUPRIGHT);
	if (namesamen(pp, x_("down-left"), l) == 0 && l >= 6) return(VTPOSDOWNLEFT);
	if (namesamen(pp, x_("down-right"), l) == 0 && l >= 6) return(VTPOSDOWNRIGHT);
	us_abortcommand(_("Unrecognized grab-point: %s"), pp);
	return(VTPOSCENT);
}

/*
 * routine to parse the "text size" specification in "pp" and return the
 * code.  Prints an error message and returns -1 on error.
 */
INTBIG us_gettextsize(CHAR *pp, INTBIG old)
{
	REGISTER INTBIG i, l;

	l = estrlen(pp);
	if (tolower(pp[l-1]) == 'p')
	{
		l = eatoi(pp);
		if (l <= 0) l = 8;
		if (l > TXTMAXPOINTS) l = TXTMAXPOINTS;
		return(TXTSETPOINTS(l));
	}
	if (tolower(pp[l-1]) == 'l')
	{
		l = atofr(pp) * 4 / WHOLE;
		if (l <= 0) l = 4;
		if (l > TXTMAXQLAMBDA) l = TXTMAXQLAMBDA;
		return(TXTSETQLAMBDA(l));
	}
	if (namesamen(pp, x_("up"), l) == 0 && l >= 1)
	{
		i = TXTGETPOINTS(old);
		if (i > 0 && i < TXTMAXPOINTS) old = TXTSETPOINTS(i+1); else
		{
			i = TXTGETQLAMBDA(old);
			if (i > 0 && i < TXTMAXQLAMBDA) old = TXTSETQLAMBDA(i+1);
		}
		return(old);
	} else if (namesamen(pp, x_("down"), l) == 0 && l >= 1)
	{
		i = TXTGETPOINTS(old);
		if (i > 1) old = TXTSETPOINTS(i-1); else
		{
			i = TXTGETQLAMBDA(old);
			if (i > 1) old = TXTSETQLAMBDA(i-1);
		}
		return(old);
	}
	us_abortcommand(_("Unrecognized text size: %s"), pp);
	return(-1);
}

static NODEINST  *us_searchcircuittextni;
static ARCINST   *us_searchcircuittextai;
static PORTPROTO *us_searchcircuittextpp;
static INTBIG     us_searchcircuittextvarnum;	/* number of the variable on this text */
static INTBIG     us_searchcircuittextline;		/* line number (in multiline text) */
static INTBIG     us_searchcircuittextstart;	/* starting character position */
static INTBIG     us_searchcircuittextend;		/* ending character position */
static BOOLEAN    us_searchcircuittextfound;	/* true if text is selected */

/*
 * Routine to initialize text searching in edit window "win".
 */
void us_initsearchcircuittext(WINDOWPART *win)
{
	REGISTER NODEPROTO *np;

	np = win->curnodeproto;
	if (np == NONODEPROTO) return;
	us_searchcircuittextni = np->firstnodeinst;
	us_searchcircuittextai = np->firstarcinst;
	us_searchcircuittextpp = np->firstportproto;
	us_searchcircuittextvarnum = 0;
	us_searchcircuittextfound = FALSE;
}

void us_advancecircuittext(CHAR *search, INTBIG bits)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp;
	REGISTER CHAR *pt;
	REGISTER INTBIG len;
	REGISTER VARIABLE *var;
	HIGHLIGHT newhigh;

	/* look for a node with this string */
	while (us_searchcircuittextni != NONODEINST)
	{
		ni = us_searchcircuittextni;
		while (us_searchcircuittextvarnum < ni->numvar)
		{
			var = &ni->firstvar[us_searchcircuittextvarnum];
			us_searchcircuittextvarnum++;
			if ((var->type&VDISPLAY) == 0) continue;
			if ((var->type&VTYPE) != VSTRING) continue;
			if ((var->type&VISARRAY) == 0)
			{
				pt = (CHAR *)var->addr;
				us_searchcircuittextstart = us_stringinstring(pt, search, bits);
				if (us_searchcircuittextstart < 0) continue;
				us_searchcircuittextend = us_searchcircuittextstart + estrlen(search);

				/* show the text */
				us_clearhighlightcount();
				newhigh.status = HIGHTEXT;
				newhigh.cell = ni->parent;
				newhigh.fromgeom = ni->geom;
				newhigh.fromport = NOPORTPROTO;
				newhigh.frompoint = 0;
				newhigh.fromvar = var;
				newhigh.fromvarnoeval = NOVARIABLE;
				us_addhighlight(&newhigh);
				us_showallhighlight();
				us_endchanges(NOWINDOWPART);
				us_searchcircuittextfound = TRUE;
				return;
			} else
			{
				len = getlength(var);
				for(us_searchcircuittextline=0; us_searchcircuittextline<len; us_searchcircuittextline++)
				{
					pt = ((CHAR **)var->addr)[us_searchcircuittextline];
					us_searchcircuittextstart = us_stringinstring(pt, search, bits);
					if (us_searchcircuittextstart < 0) continue;
					us_searchcircuittextend = us_searchcircuittextstart + estrlen(search);

					/* show the text */
					us_clearhighlightcount();
					newhigh.status = HIGHTEXT;
					newhigh.cell = ni->parent;
					newhigh.fromgeom = ni->geom;
					newhigh.fromport = NOPORTPROTO;
					newhigh.frompoint = 0;
					newhigh.fromvar = var;
					newhigh.fromvarnoeval = NOVARIABLE;
					us_addhighlight(&newhigh);
					us_showallhighlight();
					us_endchanges(NOWINDOWPART);
					us_searchcircuittextfound = TRUE;
					return;
				}
			}
		}
		us_searchcircuittextni = ni->nextnodeinst;
		us_searchcircuittextvarnum = 0;
	}

	/* look for an arc with this string */
	while (us_searchcircuittextai != NOARCINST)
	{
		ai = us_searchcircuittextai;
		while (us_searchcircuittextvarnum < ai->numvar)
		{
			var = &ai->firstvar[us_searchcircuittextvarnum];
			us_searchcircuittextvarnum++;
			if ((var->type&VDISPLAY) == 0) continue;
			if ((var->type&VTYPE) != VSTRING) continue;
			if ((var->type&VISARRAY) == 0)
			{
				pt = (CHAR *)var->addr;
				us_searchcircuittextstart = us_stringinstring(pt, search, bits);
				if (us_searchcircuittextstart < 0) continue;
				us_searchcircuittextend = us_searchcircuittextstart + estrlen(search);

				/* show the text */
				us_clearhighlightcount();
				newhigh.status = HIGHTEXT;
				newhigh.cell = ai->parent;
				newhigh.fromgeom = ai->geom;
				newhigh.fromport = NOPORTPROTO;
				newhigh.frompoint = 0;
				newhigh.fromvar = var;
				newhigh.fromvarnoeval = NOVARIABLE;
				us_addhighlight(&newhigh);
				us_showallhighlight();
				us_endchanges(NOWINDOWPART);
				us_searchcircuittextfound = TRUE;
				return;
			} else
			{
				len = getlength(var);
				for(us_searchcircuittextline=0; us_searchcircuittextline<len; us_searchcircuittextline++)
				{
					pt = ((CHAR **)var->addr)[us_searchcircuittextline];
					us_searchcircuittextstart = us_stringinstring(pt, search, bits);
					if (us_searchcircuittextstart < 0) continue;
					us_searchcircuittextend = us_searchcircuittextstart + estrlen(search);

					/* show the text */
					us_clearhighlightcount();
					newhigh.status = HIGHTEXT;
					newhigh.cell = ai->parent;
					newhigh.fromgeom = ai->geom;
					newhigh.fromport = NOPORTPROTO;
					newhigh.frompoint = 0;
					newhigh.fromvar = var;
					newhigh.fromvarnoeval = NOVARIABLE;
					us_addhighlight(&newhigh);
					us_showallhighlight();
					us_endchanges(NOWINDOWPART);
					us_searchcircuittextfound = TRUE;
					return;
				}
			}
		}
		us_searchcircuittextai = ai->nextarcinst;
		us_searchcircuittextvarnum = 0;
	}

	/* look for an export with this string */
	while (us_searchcircuittextpp != NOPORTPROTO)
	{
		pp = us_searchcircuittextpp;
		us_searchcircuittextpp = pp->nextportproto;
		us_searchcircuittextstart = us_stringinstring(pp->protoname, search, bits);
		if (us_searchcircuittextstart < 0) continue;
		us_searchcircuittextend = us_searchcircuittextstart + estrlen(search);

		/* show the text */
		us_clearhighlightcount();
		newhigh.status = HIGHTEXT;
		newhigh.cell = pp->parent;
		newhigh.fromgeom = pp->subnodeinst->geom;
		newhigh.fromport = pp;
		newhigh.frompoint = 0;
		newhigh.fromvar = NOVARIABLE;
		newhigh.fromvarnoeval = NOVARIABLE;
		us_addhighlight(&newhigh);
		us_showallhighlight();
		us_endchanges(NOWINDOWPART);
		us_searchcircuittextfound = TRUE;
		return;
	}
	us_searchcircuittextfound = FALSE;
}

/*
 * Routine to look for the substring "search" inside of the larger string "string".
 * if "bits" has 4 in it, the search is case sensitive.  Returns the character
 * position in "string" of where "search" is found (-1 if not found).
 */
INTBIG us_stringinstring(CHAR *string, CHAR *search, INTBIG bits)
{
	REGISTER INTBIG stringlen, searchlen, searchwid, i;

	stringlen = estrlen(string);
	searchlen = estrlen(search);
	searchwid = stringlen - searchlen;
	if (searchwid < 0) return(-1);
	if ((bits&4) == 0)
	{
		for(i=0; i<=searchwid; i++)
			if (namesamen(&string[i], search, searchlen) == 0) return(i);
	} else
	{
		for(i=0; i<=searchwid; i++)
			if (estrncmp(&string[i], search, searchlen) == 0) return(i);
	}
	return(-1);
}

/*
 * Routine to find the string "search" in the circuit in window "win".  "bits" is:
 *  2  replace all with "replaceall"
 *  4  case sensitive
 *  8  find reverse
 */
void us_searchcircuittext(WINDOWPART *win, CHAR *search, CHAR *replaceall, INTBIG bits)
{
	REGISTER NODEPROTO *np;
	REGISTER INTBIG total;

	np = win->curnodeproto;
	if (np == NONODEPROTO) return;

	if ((bits&2) != 0)
	{
		total = 0;
		for(;;)
		{
			/* advance to the next circuit text */
			us_advancecircuittext(search, bits);
			if (!us_searchcircuittextfound) break;
			us_replacecircuittext(win, replaceall);
			total++;
		}
		if (total == 0) ttybeep(SOUNDBEEP, TRUE); else
			ttyputmsg(_("Replaced %ld times"), total);
		return;
	}
	us_advancecircuittext(search, bits);
	if (!us_searchcircuittextfound) ttybeep(SOUNDBEEP, TRUE);
}

/*
 * Routine to replace the text last selected with "us_searchcircuittext()" with
 * the string "replace".
 */
void us_replacecircuittext(WINDOWPART *win, CHAR *replace)
{
	REGISTER HIGHLIGHT *high;
	REGISTER INTBIG i, len, addr, type;
	REGISTER VARIABLE *var;
	REGISTER NODEINST *ni;
	REGISTER PORTPROTO *pp;
	REGISTER CHAR *pt;
	CHAR *newname;
	REGISTER void *infstr;
	Q_UNUSED( win );

	if (!us_searchcircuittextfound) return;
	high = us_getonehighlight();
	if (high->status != HIGHTEXT) return;
	if (high->fromgeom->entryisnode && high->fromvar == NOVARIABLE)
	{
		/* export text */
		ni = high->fromgeom->entryaddr.ni;
		pp = high->fromport;
		infstr = initinfstr();
		for(i=0; i<us_searchcircuittextstart; i++)
			addtoinfstr(infstr, pp->protoname[i]);
		addstringtoinfstr(infstr, replace);
		len = estrlen(pp->protoname);
		for(i=us_searchcircuittextend; i<len; i++)
			addtoinfstr(infstr, pp->protoname[i]);
		us_pushhighlight();
		us_clearhighlightcount();
		us_renameport(pp, returninfstr(infstr));
		us_pophighlight(FALSE);
		us_endchanges(NOWINDOWPART);
		return;
	}

	/* rename variable */
	var = high->fromvar;
	if (var == NOVARIABLE) return;
	addr = (INTBIG)high->fromgeom->entryaddr.blind;
	if (high->fromgeom->entryisnode) type = VNODEINST; else
		type = VARCINST;
	us_pushhighlight();
	us_clearhighlightcount();
	startobjectchange(addr, type);
	if ((var->type&VISARRAY) != 0)
	{
		pt = ((CHAR **)var->addr)[us_searchcircuittextline];
		infstr = initinfstr();
		for(i=0; i<us_searchcircuittextstart; i++)
			addtoinfstr(infstr, pt[i]);
		addstringtoinfstr(infstr, replace);
		len = estrlen(pt);
		for(i=us_searchcircuittextend; i<len; i++)
			addtoinfstr(infstr, pt[i]);
		allocstring(&newname, returninfstr(infstr), el_tempcluster);
		(void)setindkey(addr, type, var->key, us_searchcircuittextline,
			(INTBIG)newname);
		efree(newname);
	} else
	{
		pt = (CHAR *)var->addr;
		infstr = initinfstr();
		for(i=0; i<us_searchcircuittextstart; i++)
			addtoinfstr(infstr, pt[i]);
		addstringtoinfstr(infstr, replace);
		len = estrlen(pt);
		for(i=us_searchcircuittextend; i<len; i++)
			addtoinfstr(infstr, pt[i]);
		allocstring(&newname, returninfstr(infstr), el_tempcluster);
		(void)setvalkey(addr, type, var->key, (INTBIG)newname,
			var->type);
		efree(newname);
	}
	endobjectchange(addr, type);
	us_pophighlight(FALSE);
	us_endchanges(NOWINDOWPART);
}

/************************ IN-PLACE VARIABLE EDITING ************************/

static INTBIG      us_editvarstartline, us_editvarstartchar;
static INTBIG      us_editvarlabellen;
static INTBIG      us_editvarendline, us_editvarendchar;
static INTBIG      us_editvarclickline, us_editvarclickchar;
static VARIABLE   *us_editvariable;
static INTBIG      us_editvarlength;
static INTBIG      us_editvarlineheight;
static INTBIG      us_editvarobjtype;
static INTBIG      us_editvarobjaddr;
static BOOLEAN     us_editvariabledoubleclick;
static CHAR      **us_editvarlines;
static CHAR       *us_editvaroneline[1];
static CHAR       *us_editvarvarname = 0;
static TECHNOLOGY *us_editvartech;

static void    us_editvariabletexthighlight(void);
static BOOLEAN us_editvariabletexteachdown(INTBIG x, INTBIG y);
static BOOLEAN us_editvariabletextfindpos(INTBIG x, INTBIG y, INTBIG *line, INTBIG *chr);
static void    us_editvariableforcefullwords(INTBIG *startline, INTBIG *startchar, INTBIG *endline, INTBIG *endchar);
static BOOLEAN us_editvariabletexthandlebutton(INTBIG x, INTBIG y, INTBIG but);
static BOOLEAN us_editvariabletexthandlechar(INTSML chr, INTBIG special);
static void    us_editvariabletextreplacetext(CHAR *replace);

void us_editvariabletext(VARIABLE *var, INTBIG objtype, INTBIG objaddr, CHAR *varname)
{
	INTBIG tsx, tsy, newtype, newvalue, units;
	UINTBIG textdescript[TEXTDESCRIPTSIZE];

	/* preserve information in globals */
	us_editvariable = var;
	us_editvarobjtype = objtype & VTYPE;
	us_editvarobjaddr = objaddr;
	if (us_editvarvarname == 0) (void)allocstring(&us_editvarvarname, varname, us_tool->cluster); else
		(void)reallocstring(&us_editvarvarname, varname, us_tool->cluster);
	us_editvarlabellen = 0;
	if ((us_editvariable->type&VISARRAY) == 0)
	{
		us_editvarlength = 1;
		(void)allocstring(&us_editvaroneline[0], describedisplayedvariable(var, -1, -1), us_tool->cluster);
		if (TDGETDISPPART(var->textdescript) != VTDISPLAYVALUE)
		{
			TDCOPY(textdescript, var->textdescript);
			TDSETDISPPART(var->textdescript, VTDISPLAYVALUE);
			us_editvarlabellen = estrlen(us_editvaroneline[0]) - estrlen(describedisplayedvariable(var, -1, -1));
			TDCOPY(var->textdescript, textdescript);
		}
		us_editvarlines = us_editvaroneline;
	} else
	{
		us_editvarlength = getlength(var);
		us_editvarlines = (CHAR **)var->addr;
	}
	switch (objtype)
	{
		case VNODEINST:
			us_editvartech = ((NODEINST *)objaddr)->parent->tech;
			break;
		case VARCINST:
			us_editvartech = ((ARCINST *)objaddr)->parent->tech;
			break;
		case VPORTPROTO:
			us_editvartech = ((PORTPROTO *)objaddr)->subnodeinst->parent->tech;
			break;
		case VNODEPROTO:
			us_editvartech = ((NODEPROTO *)objaddr)->tech;
			break;
	}

	/* flush graphics */
	us_endchanges(NOWINDOWPART);

	/* determine height of a line of text */
	screensettextinfo(el_curwindowpart, us_editvartech, us_editvariable->textdescript);
	screengettextsize(el_curwindowpart, x_("Xy"), &tsx, &tsy);
	us_editvarlineheight = tsy;

	/* set highlighting to cover all text */
	us_editvarstartline = 0;
	us_editvarstartchar = us_editvarlabellen;
	us_editvarendline = us_editvarlength-1;
	us_editvarendchar = estrlen(us_editvarlines[us_editvarendline]);
	us_editvariabletexthighlight();

	/* loop while editing text */
	modalloop(us_editvariabletexthandlechar, us_editvariabletexthandlebutton, IBEAMCURSOR);

	/* remove highlighting */
	us_editvariabletexthighlight();

	/* recalculate the proper type now that editing is done */
	if ((us_editvariable->type&VISARRAY) == 0)
	{
		units = TDGETUNITS(us_editvariable->textdescript);
		getsimpletype(&us_editvarlines[0][us_editvarlabellen], &newtype, &newvalue, units);
		newtype = (us_editvariable->type & ~VTYPE) | newtype;
		if (us_editvarobjtype != VPORTPROTO)
			startobjectchange(us_editvarobjaddr, us_editvarobjtype); else
				startobjectchange((INTBIG)(((PORTPROTO *)us_editvarobjaddr)->subnodeinst), VNODEINST);
		if (us_editvarobjtype == VNODEPROTO)
			us_undrawcellvariable(us_editvariable, (NODEPROTO *)us_editvarobjaddr);
		var = setval(us_editvarobjaddr, us_editvarobjtype,
			us_editvarvarname, newvalue, newtype);
		if (var != NOVARIABLE) us_editvariable = var;
		if (us_editvarobjtype == VNODEPROTO)
			us_drawcellvariable(us_editvariable, (NODEPROTO *)us_editvarobjaddr);
		if (us_editvarobjtype != VPORTPROTO)
			endobjectchange(us_editvarobjaddr, us_editvarobjtype); else
				endobjectchange((INTBIG)(((PORTPROTO *)us_editvarobjaddr)->subnodeinst), VNODEINST);
		us_endchanges(NOWINDOWPART);
	}
}

static BOOLEAN us_editvariabletexthandlebutton(INTBIG x, INTBIG y, INTBIG but)
{
	INTBIG line, chr;

	us_editvariabledoubleclick = doublebutton(but);
	if (!us_editvariabletextfindpos(x, y, &line, &chr)) return(TRUE);
	us_editvariabletexthighlight();
	if (line == 0 && chr < us_editvarlabellen) chr = us_editvarlabellen;
	if (shiftbutton(but))
	{
		if (line < us_editvarstartline || (line == us_editvarstartline &&
			chr < us_editvarstartchar))
		{
			us_editvarstartline = line;
			us_editvarstartchar = chr;
		} else if (line > us_editvarendline || (line == us_editvarendline &&
			chr > us_editvarendchar))
		{
			us_editvarendline = line;
			us_editvarendchar = chr;
		}
	} else
	{
		us_editvarstartline = line;
		us_editvarstartchar = chr;
		us_editvarendline = line;
		us_editvarendchar = chr;
	}
	us_editvarclickline = line;
	us_editvarclickchar = chr;
	if (us_editvariabledoubleclick)
		us_editvariableforcefullwords(&us_editvarstartline, &us_editvarstartchar,
			&us_editvarendline, &us_editvarendchar);
	us_editvariabletexthighlight();

	trackcursor(FALSE, us_ignoreup, us_nullvoid, us_editvariabletexteachdown, us_stoponchar,
			us_nullvoid, TRACKNORMAL);
	return(FALSE);
}

BOOLEAN us_editvariabletexthandlechar(INTSML chr, INTBIG special)
{
	CHAR replace[2], *pt;
	REGISTER INTBIG startchar, endchar, i, j;
	REGISTER void *infstr;

	if ((special&SPECIALKEYDOWN) != 0)
	{
		switch ((special&SPECIALKEY)>>SPECIALKEYSH)
		{
			case SPECIALKEYARROWL:
				us_editvariabletexthighlight();
				if (us_editvarstartline == us_editvarendline &&
					us_editvarstartchar == us_editvarendchar)
				{
					if (us_editvarstartchar > 0)
					{
						if (us_editvarstartline != 0 || us_editvarstartchar > us_editvarlabellen)
							us_editvarstartchar--;
					} else
					{
						if (us_editvarstartline > 0)
						{
							us_editvarstartline--;
							us_editvarstartchar = estrlen(us_editvarlines[us_editvarstartline]);
						}
					}
				}
				us_editvarendline = us_editvarstartline;
				us_editvarendchar = us_editvarstartchar;
				us_editvariabletexthighlight();
				return(FALSE);
			case SPECIALKEYARROWR:
				us_editvariabletexthighlight();
				if (us_editvarstartline == us_editvarendline &&
					us_editvarstartchar == us_editvarendchar)
				{
					if (us_editvarendchar < (INTBIG)estrlen(us_editvarlines[us_editvarendline]))
						us_editvarendchar++; else
					{
						if (us_editvarendline < us_editvarlength-1)
						{
							us_editvarendline++;
							us_editvarendchar = 0;
						}
					}
				}
				us_editvarstartline = us_editvarendline;
				us_editvarstartchar = us_editvarendchar;
				us_editvariabletexthighlight();
				return(FALSE);
			case SPECIALKEYARROWU:
				us_editvariabletexthighlight();
				if (us_editvarstartline > 0)
				{
					us_editvarstartline--;
					if (us_editvarstartchar > (INTBIG)estrlen(us_editvarlines[us_editvarstartline]))
						us_editvarstartchar = estrlen(us_editvarlines[us_editvarstartline]);
				}
				us_editvarendline = us_editvarstartline;
				us_editvarendchar = us_editvarstartchar;
				us_editvariabletexthighlight();
				return(FALSE);
			case SPECIALKEYARROWD:
				us_editvariabletexthighlight();
				if (us_editvarendline < us_editvarlength-1)
				{
					us_editvarendline++;
					if (us_editvarendchar > (INTBIG)estrlen(us_editvarlines[us_editvarendline]))
						us_editvarendchar = estrlen(us_editvarlines[us_editvarendline]);
				}
				us_editvarstartline = us_editvarendline;
				us_editvarstartchar = us_editvarendchar;
				us_editvariabletexthighlight();
				return(FALSE);
		}
	}

	/* handle paste */
	if ((special&ACCELERATORDOWN) != 0)
	{
		if (chr == 'v' || chr == 'V')
		{
			pt = getcutbuffer();
			us_editvariabletexthighlight();
			us_editvariabletextreplacetext(pt);
			if ((us_editvariable->type&VISARRAY) == 0)
			{
				(void)reallocstring(&us_editvaroneline[0], describedisplayedvariable(us_editvariable, -1, -1),
					us_tool->cluster);
				us_editvarlines = us_editvaroneline;
			} else
			{
				us_editvarlength = getlength(us_editvariable);
				us_editvarlines = (CHAR **)us_editvariable->addr;
			}
			us_editvariabletexthighlight();
			return(FALSE);
		}

		/* handle copy/cut */
		if (chr == 'c' || chr == 'C' ||
			chr == 'x' || chr == 'X')
		{
			infstr = initinfstr();
			for(i=us_editvarstartline; i<=us_editvarendline; i++)
			{
				if (i > us_editvarstartline) addtoinfstr(infstr, '\n');
				startchar = 0;
				endchar = estrlen(us_editvarlines[i]);
				if (i == us_editvarstartline) startchar = us_editvarstartchar;
				if (i == us_editvarendline) endchar = us_editvarendchar;
				for(j=startchar; j<endchar; j++)
					addtoinfstr(infstr, us_editvarlines[i][j]);
			}
			setcutbuffer(returninfstr(infstr));
			if (chr == 'c' || chr == 'C') return(FALSE);
			chr = 0;
		}
	}

	/* delete what is selected and insert what was typed */
	if (chr == '\n' || chr == '\r')
	{
		/* cannot insert second line if text is not an array */
		if ((us_editvariable->type&VISARRAY) == 0) return(TRUE);
	}
	us_editvariabletexthighlight();
	if (chr == DELETEKEY || chr == BACKSPACEKEY)
	{
		chr = 0;
		if (us_editvarstartline == us_editvarendline &&
			us_editvarstartchar == us_editvarendchar)
		{
			if (us_editvarstartchar > 0)
			{
				if (us_editvarstartline != 0 || us_editvarstartchar > us_editvarlabellen)
					us_editvarstartchar--;
			} else
			{
				if (us_editvarstartline > 0)
				{
					us_editvarstartline--;
					us_editvarstartchar = estrlen(us_editvarlines[us_editvarstartline]);
				}
			}
		}
	}
	replace[0] = (CHAR)chr;
	replace[1] = 0;
	us_editvariabletextreplacetext(replace);
	if ((us_editvariable->type&VISARRAY) == 0)
	{
		(void)reallocstring(&us_editvaroneline[0], describedisplayedvariable(us_editvariable, -1, -1),
			us_tool->cluster);
		us_editvarlines = us_editvaroneline;
	} else
	{
		us_editvarlength = getlength(us_editvariable);
		us_editvarlines = (CHAR **)us_editvariable->addr;
	}
	us_editvariabletexthighlight();
	return(FALSE);
}

void us_editvariabletextreplacetext(CHAR *replace)
{
	void *stringarray;
	REGISTER INTBIG i, newline, newchar;
	CHAR **newtext;
	REGISTER VARIABLE *var;
	INTBIG newtype, newvalue, count;
	REGISTER void *infstr;

	stringarray = newstringarray(el_tempcluster);

	/* add all lines before the start of selection */
	newline = 0;
	for(i=0; i<us_editvarstartline; i++)
	{
		if (i != 0) addtostringarray(stringarray, us_editvarlines[i]); else
			addtostringarray(stringarray, &us_editvarlines[i][us_editvarlabellen]);			
		newline++;
	}

	/* build the line with the selection start */
	if (newline == 0) newchar = us_editvarlabellen; else newchar = 0;
	infstr = initinfstr();
	for(i=newchar; i<us_editvarstartchar; i++)
	{
		addtoinfstr(infstr, us_editvarlines[us_editvarstartline][i]);
		newchar++;
	}

	/* now add the replacement text */
	for(i=0; i<(INTBIG)estrlen(replace); i++)
	{
		if (replace[i] == '\n' || replace[i] == '\r')
		{
			addtostringarray(stringarray, returninfstr(infstr));
			infstr = initinfstr();
			newline++;
			newchar = 0;
		} else
		{
			addtoinfstr(infstr, replace[i]);
			newchar++;
		}
	}

	/* now add the line with the selection end */
	for(i=us_editvarendchar; i<(INTBIG)estrlen(us_editvarlines[us_editvarendline]); i++)
		addtoinfstr(infstr, us_editvarlines[us_editvarendline][i]);
	addtostringarray(stringarray, returninfstr(infstr));

	/* add all lines after the end of selection */
	for(i=us_editvarendline+1; i<us_editvarlength; i++)
		addtostringarray(stringarray, us_editvarlines[i]);

	/* get the new text and put it on the object */
	newtext = getstringarray(stringarray, &count);
	if (us_editvarobjtype != VPORTPROTO)
		startobjectchange(us_editvarobjaddr, us_editvarobjtype); else
			startobjectchange((INTBIG)(((PORTPROTO *)us_editvarobjaddr)->subnodeinst), VNODEINST);
	if (us_editvarobjtype == VNODEPROTO)
		us_undrawcellvariable(us_editvariable, (NODEPROTO *)us_editvarobjaddr);
	if ((us_editvariable->type&VISARRAY) == 0)
	{
		/* presume a string for now: recompute type when done editing */
		newvalue = (INTBIG)newtext[0];
		newtype = (us_editvariable->type & ~VTYPE) | VSTRING;
		var = setval(us_editvarobjaddr, us_editvarobjtype,
			us_editvarvarname, newvalue, newtype);
	} else
	{
		newtype = (us_editvariable->type & ~VLENGTH) | (count << VLENGTHSH);
		var = setval(us_editvarobjaddr, us_editvarobjtype,
			us_editvarvarname, (INTBIG)newtext, newtype);
	}
	if (var != NOVARIABLE) us_editvariable = var;
	if (us_editvarobjtype == VNODEPROTO)
		us_drawcellvariable(us_editvariable, (NODEPROTO *)us_editvarobjaddr);
	if (us_editvarobjtype != VPORTPROTO)
		endobjectchange(us_editvarobjaddr, us_editvarobjtype); else
			endobjectchange((INTBIG)(((PORTPROTO *)us_editvarobjaddr)->subnodeinst), VNODEINST);
	us_endchanges(NOWINDOWPART);
	killstringarray(stringarray);

	/* set the new selection point */
	us_editvarstartline = us_editvarendline = newline;
	us_editvarstartchar = us_editvarendchar = newchar;
}

BOOLEAN us_editvariabletexteachdown(INTBIG x, INTBIG y)
{
	INTBIG line, chr, startline, startchar, endline, endchar;

	if (!us_editvariabletextfindpos(x, y, &line, &chr)) return(FALSE);
	if (line == 0 && chr < us_editvarlabellen) chr = us_editvarlabellen;
	startline = us_editvarstartline;
	startchar = us_editvarstartchar;
	endline = us_editvarendline;
	endchar = us_editvarendchar;
	if (line > us_editvarclickline || (line == us_editvarclickline && chr > us_editvarclickchar))
	{
		startline = us_editvarclickline;
		startchar = us_editvarclickchar;
		endline = line;
		endchar = chr;
		if (us_editvariabledoubleclick)
			us_editvariableforcefullwords(&startline, &startchar, &endline, &endchar);
	}
	if (line < us_editvarclickline || (line == us_editvarclickline && chr < us_editvarclickchar))
	{
		startline = line;
		startchar = chr;
		endline = us_editvarclickline;
		endchar = us_editvarclickchar;
		if (us_editvariabledoubleclick)
			us_editvariableforcefullwords(&startline, &startchar, &endline, &endchar);
	}
	if (startline != us_editvarstartline || startchar != us_editvarstartchar ||
		endline != us_editvarendline || endchar != us_editvarendchar)
	{
		us_editvariabletexthighlight();
		us_editvarstartline = startline;
		us_editvarstartchar = startchar;
		us_editvarendline = endline;
		us_editvarendchar = endchar;
		us_editvariabletexthighlight();
	}
	return(FALSE);
}

void us_editvariableforcefullwords(INTBIG *startline, INTBIG *startchar, INTBIG *endline, INTBIG *endchar)
{
	CHAR *pt;
	INTBIG len;

	pt = us_editvarlines[*startline];
	while (*startchar > 0 && isalnum(pt[*startchar - 1]))
		(*startchar)--;

	pt = us_editvarlines[*endline];
	len = estrlen(pt);
	while (*endchar < len && isalnum(pt[*endchar]))
		(*endchar)++;
}

BOOLEAN us_editvariabletextfindpos(INTBIG xp, INTBIG yp, INTBIG *line, INTBIG *chr)
{
	REGISTER INTBIG i, j, screenlx, screenhx, screenly, screenhy;
	CHAR save;
	INTBIG x, y;
	INTBIG tsx, tsy;
	REGISTER INTBIG lasttsx, charwid;

	/* determine text size */
	screensettextinfo(el_curwindowpart, us_editvartech, us_editvariable->textdescript);
	for(i = 0; i < us_editvarlength; i++)
	{
		getdisparrayvarlinepos(us_editvarobjaddr, us_editvarobjtype, us_editvartech,
			el_curwindowpart, us_editvariable, i, &x, &y, TRUE);
		screenlx = applyxscale(el_curwindowpart, x - el_curwindowpart->screenlx) +
			el_curwindowpart->uselx;
		screenly = applyyscale(el_curwindowpart, y - el_curwindowpart->screenly) +
			el_curwindowpart->usely;
		screengettextsize(el_curwindowpart, us_editvarlines[i], &tsx, &tsy);
		screenhx = screenlx + tsx;
		screenhy = screenly + us_editvarlineheight;
		if (yp < screenly || yp > screenhy) continue;
		if (xp < screenlx-us_editvarlineheight ||
			xp > screenhx+us_editvarlineheight) continue;
		*line = i;
		lasttsx = 0;
		for(j=1; j<=(INTBIG)estrlen(us_editvarlines[i]); j++)
		{
			save = us_editvarlines[i][j];
			us_editvarlines[i][j] = 0;
			screengettextsize(el_curwindowpart, us_editvarlines[i], &tsx, &tsy);
			charwid = tsx - lasttsx;
			lasttsx = tsx;
			us_editvarlines[i][j] = save;
			if (xp < screenlx + tsx - charwid/2) break;
		}
		*chr = j-1;
		return(TRUE);
	}
	return(FALSE);
}

void us_editvariabletexthighlight(void)
{
	REGISTER INTBIG i, j, startch;
	CHAR save;
	INTBIG x, y, screenlx, screenhx, screenly, screenhy;
	INTBIG tsx, tsy;

	/* determine text size */
	screensettextinfo(el_curwindowpart, us_editvartech, us_editvariable->textdescript);
	for(i = us_editvarstartline; i <= us_editvarendline; i++)
	{
		getdisparrayvarlinepos(us_editvarobjaddr, us_editvarobjtype, us_editvartech,
			el_curwindowpart, us_editvariable, i, &x, &y, TRUE);
		screenlx = applyxscale(el_curwindowpart, x - el_curwindowpart->screenlx) +
			el_curwindowpart->uselx;
		screenly = applyyscale(el_curwindowpart, y - el_curwindowpart->screenly) +
			el_curwindowpart->usely;
		startch = 0;
		if (i == us_editvarstartline && us_editvarstartchar != 0)
		{
			save = us_editvarlines[i][us_editvarstartchar];
			us_editvarlines[i][us_editvarstartchar] = 0;
			screengettextsize(el_curwindowpart, us_editvarlines[i], &tsx, &tsy);
			screenlx += tsx;
			us_editvarlines[i][us_editvarstartchar] = save;
			startch = us_editvarstartchar;
		}
		if (i == us_editvarendline) j = us_editvarendchar; else
			j = estrlen(us_editvarlines[i]);
		save = us_editvarlines[i][j];
		us_editvarlines[i][j] = 0;
		screengettextsize(el_curwindowpart, &us_editvarlines[i][startch], &tsx, &tsy);
		screenhx = screenlx + tsx;
		us_editvarlines[i][j] = save;
		screenhy = screenly + us_editvarlineheight;
		if (screenlx <= el_curwindowpart->uselx) screenlx = el_curwindowpart->uselx+1;
		if (screenhx > el_curwindowpart->usehx) screenhx = el_curwindowpart->usehx;
		if (screenly < el_curwindowpart->usely) screenly = el_curwindowpart->usely;
		if (screenhy > el_curwindowpart->usehy) screenhy = el_curwindowpart->usehy;
		if ((el_curwindowpart->state&INPLACEEDIT) != 0) 
			xformbox(&screenlx, &screenhx, &screenly, &screenhy, el_curwindowpart->outofcell);
		screeninvertbox(el_curwindowpart, screenlx-1, screenhx-1,
			screenly, screenhy-1);
	}
}

/******************** USER-BROADCAST CHANGES ********************/

/*
 * routine to allocate a new ubchange from the pool (if any) or memory,
 * fill in the "cell", "change", "x", and "y" fields, and link it to the
 * global list.  Returns true on error.
 */
BOOLEAN us_newubchange(INTBIG change, void *object, void *parameter)
{
	REGISTER UBCHANGE *d;

	if (us_ubchangefree == NOUBCHANGE)
	{
		d = (UBCHANGE *)emalloc((sizeof (UBCHANGE)), us_tool->cluster);
		if (d == 0) return(TRUE);
	} else
	{
		/* take ubchange from free list */
		d = us_ubchangefree;
		us_ubchangefree = (UBCHANGE *)d->nextubchange;
	}
	d->object = object;
	d->parameter = parameter;
	d->change = change;
	d->nextubchange = us_ubchanges;
	us_ubchanges = d;
	return(FALSE);
}

/*
 * routine to return ubchange "d" to the pool of free ubchanges
 */
void us_freeubchange(UBCHANGE *d)
{
	d->nextubchange = us_ubchangefree;
	us_ubchangefree = d;
}

/*
 * routine to remove all queued user broadcast changes to cell "np"
 * because it was deleted
 */
void us_removeubchange(NODEPROTO *np)
{
	REGISTER UBCHANGE *d, *lastd, *nextd;
	REGISTER NODEPROTO *thisnp;

	lastd = NOUBCHANGE;
	for(d = us_ubchanges; d != NOUBCHANGE; d = nextd)
	{
		nextd = d->nextubchange;
		if (d->change == UBNEWFC) thisnp = ((NODEINST *)d->object)->parent; else
			if (d->change == UBKILLFM) thisnp = (NODEPROTO *)d->object; else
				thisnp = NONODEPROTO;
		if (thisnp == np)
		{
			if (lastd == NOUBCHANGE) us_ubchanges = nextd; else
				lastd->nextubchange = nextd;
			us_freeubchange(d);
			continue;
		}
		lastd = d;
	}
}

/*
 * routine to remove variable "FACET_message" from cell "np".
 */
void us_delcellmessage(NODEPROTO *np)
{
	(void)us_newubchange(UBKILLFM, np, 0);
}

/*
 * routine to add a cell-center to cell "np"
 */
void us_addcellcenter(NODEINST *ni)
{
	(void)us_newubchange(UBNEWFC, ni, 0);
}

/*
 * Routine to queue a check of the SPICE parts.
 */
void us_checkspiceparts(void)
{
	(void)us_newubchange(UBSPICEPARTS, 0, 0);
}

/*
 * Routine to queue a deletion of a technology-edit layer cell.
 */
void us_deltecedlayercell(NODEPROTO *np)
{
	(void)us_newubchange(UBTECEDDELLAYER, np, 0);
}

/*
 * Routine to queue a deletion of a technology-edit node cell.
 */
void us_deltecednodecell(NODEPROTO *np)
{
	(void)us_newubchange(UBTECEDDELNODE, np, 0);
}

/*
 * Routine to queue a rename of a technology-edit cell.
 */
void us_renametecedcell(NODEPROTO *np, CHAR *oldname)
{
	(void)us_newubchange(UBTECEDRENAME, np, oldname);
}

/*
 * Routine to queue a turn-on of a tool.
 */
void us_toolturnedon(TOOL *tool)
{
	(void)us_newubchange(UBTOOLISON, tool, 0);
}

/*
 * routine to implement all user broadcast changes queued during the last broadcast
 */
void us_doubchanges(void)
{
	REGISTER UBCHANGE *d, *nextd;
	REGISTER WINDOWPART *w;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni, *oni;
	REGISTER EDITOR *ed;
	REGISTER INTBIG bit;
	REGISTER LIBRARY *lib;
	REGISTER VARIABLE *var;
	REGISTER TOOL *tool;
	REGISTER CHAR *partsname;
	CHAR *par[1];
	REGISTER void *infstr;

	for(d = us_ubchanges; d != NOUBCHANGE; d = nextd)
	{
		nextd = d->nextubchange;

		switch (d->change)
		{
			case UBKILLFM:	/* remove cell_message */
				np = (NODEPROTO *)d->object;
				for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
				{
					if (w->curnodeproto != np) continue;
					if ((w->state&WINDOWTYPE) != TEXTWINDOW) continue;

					/* see if the window still has a valid variable */
					ed = w->editor;
					if (ed->editobjvar == NOVARIABLE) continue;
					var = getval((INTBIG)ed->editobjaddr, ed->editobjtype, -1, ed->editobjqual);
					if (var == NOVARIABLE)
					{
						(void)newwindowpart(w->location, w);
						killwindowpart(w);
					}
				}
				break;
			case UBNEWFC:	/* added cell-center */
				ni = (NODEINST *)d->object;
				np = ni->parent;
				for(oni = np->firstnodeinst; oni != NONODEINST; oni = oni->nextnodeinst)
				{
					if (oni == ni) continue;
					if (oni->proto == gen_cellcenterprim)
					{
						ttyputerr(_("Can only be one cell-center in a cell: new one deleted"));
						us_clearhighlightcount();
						startobjectchange((INTBIG)ni, VNODEINST);
						(void)killnodeinst(ni);
						us_setnodeprotocenter(oni->lowx, oni->lowy, np);
						break;
					}
				}
				if (oni == NONODEINST)
					us_setnodeprotocenter(ni->lowx, ni->lowy, np);
				break;
			case UBSPICEPARTS:		/* check for new spice parts */
				var = getvalkey((INTBIG)sim_tool, VTOOL, VSTRING, sim_spice_partskey);
				if (var == NOVARIABLE) break;
				partsname = (CHAR *)var->addr;
				if (namesame(partsname, sim_spice_parts) == 0) break;
				(void)reallocstring(&sim_spice_parts, partsname, sim_tool->cluster);

				/* invoke the command file */
				infstr = initinfstr();
				addstringtoinfstr(infstr, el_libdir);
				addstringtoinfstr(infstr, sim_spice_parts);
				par[0] = returninfstr(infstr);
				us_commandfile(1, par);
				break;
			case UBTECEDDELLAYER:		/* tell technology edit that layer cell was deleted */
				us_teceddeletelayercell((NODEPROTO *)d->object);
				break;
			case UBTECEDDELNODE:		/* tell technology edit that node cell was deleted */
				us_teceddeletenodecell((NODEPROTO *)d->object);
				break;
			case UBTECEDRENAME:			/* tell technology edit that cell was renamed */
				np = (NODEPROTO *)d->object;
				us_tecedrenamecell((CHAR *)d->parameter, np->protoname);
				break;
			case UBTOOLISON:
				/* look through all cells and update those that need it */
				tool = (TOOL *)d->object;
				bit = 1 << tool->toolindex;
				for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
					for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				{
					if ((np->adirty & bit) != 0)
					{
						if (tool->examinenodeproto != 0)
							(*tool->examinenodeproto)(np);
					}
					np->adirty &= ~bit;
				}
				break;
		}

		/* cleanup */
		us_freeubchange(d);
	}
	us_ubchanges = NOUBCHANGE;
}

/******************** COLOR ********************/

/*
 * setup the color map for the graphics of technology "tech".  "style" is:
 *  COLORSEXISTING  continue existing colors
 *  COLORSDEFAULT   use default opaque colors
 *  COLORSBLACK     use black background colors
 *  COLORSWHITE     use white background colors
 * A set of transparent colors is obtained from technology "tech", combined
 * with the appropriate opaque colors, and set into the proper
 *  variables on the "user" tool (and subsequently displayed).
 * The 256 entries are organized thusly:
 * Bit 0 is for highlighting; bit 1 is an escape for
 * opaque colors, the next 5 bits are the transparent colors (if the opaque
 * escape is off) or the opaque value (if the bit is set).
 * The last bit is for the grid, although it may not appear if there are 128 entries.
 * This routine uses the database variable "USER_color_map" on the
 * technologies.
 */
void us_getcolormap(TECHNOLOGY *tech, INTBIG style, BOOLEAN broadcast)
{
	static TECH_COLORMAP colmap[38] =
	{
		{255,255,255}, /*   4(0004) WHITE:   white                           */
		{  0,  0,  0}, /*  12(0014) BLACK:   black                           */
		{255,  0,  0}, /*  20(0024) RED:     red                             */
		{  0,  0,255}, /*  28(0034) BLUE:    blue                            */
		{  0,255,  0}, /*  36(0044) GREEN:   green                           */
		{  0,255,255}, /*  44(0054) CYAN:    cyan                            */
		{255,  0,255}, /*  52(0064) MAGENTA: magenta                         */
		{255,255,  0}, /*  60(0074) YELLOW:  yellow                          */
		{  0,  0,  0}, /*  68(0104) CELLTXT: cell and port names             */
		{  0,  0,  0}, /*  76(0114) CELLOUT: cell outline                    */
		{  0,  0,  0}, /*  84(0124) WINBOR:  window border color             */
		{  0,255,  0}, /*  92(0134) HWINBOR: highlighted window border color */
		{  0,  0,  0}, /* 100(0144) MENBOR:  menu border color               */
		{255,255,255}, /* 108(0154) HMENBOR: highlighted menu border color   */
		{  0,  0,  0}, /* 116(0164) MENTXT:  menu text color                 */
		{  0,  0,  0}, /* 124(0174) MENGLY:  menu glyph color                */
		{  0,  0,  0}, /* 132(0204) CURSOR:  cursor color                    */
		{180,180,180}, /* 140(0214) GRAY:    gray                            */
		{255,190,  6}, /* 148(0224) ORANGE:  orange                          */
		{186,  0,255}, /* 156(0234) PURPLE:  purple                          */
		{139, 99, 46}, /* 164(0244) BROWN:   brown                           */
		{230,230,230}, /* 172(0254) LGRAY:   light gray                      */
		{100,100,100}, /* 180(0264) DGRAY:   dark gray                       */
		{255,150,150}, /* 188(0274) LRED:    light red                       */
		{159, 80, 80}, /* 196(0304) DRED:    dark red                        */
		{175,255,175}, /* 204(0314) LGREEN:  light green                     */
		{ 89,159, 85}, /* 212(0324) DGREEN:  dark green                      */
		{150,150,255}, /* 220(0334) LBLUE:   light blue                      */
		{  2, 15,159}, /* 228(0344) DBLUE:   dark blue                       */
		{  0,  0,  0}, /* 236(0354)          unassigned                      */
		{  0,  0,  0}, /* 244(0364)          unassigned                      */
		{  0,  0,  0}, /* 252(0374)          unassigned                      */
		{  0,  0,  0}, /*                    grid                            */
		{255,255,255}, /*                    highlight                       */
		{255,  0,  0}, /*                    black background highlight      */
		{255,  0,  0}, /*                    white background highlight      */
		{255,255,255}, /*                    black background cursor         */
		{  0,  0,  0}  /*                    white background cursor         */
	};
	static TECH_COLORMAP default_colmap[32] =
	{                  /*     transpar4 transpar3 transpar2 transpar1 transpar0 */
		{200,200,200}, /* 0:                                                    */
		{  0,  0,200}, /* 1:                                          transpar0 */
		{220,  0,120}, /* 2:                                transpar1           */
		{ 80,  0,160}, /* 3:                                transpar1+transpar0 */
		{ 70,250, 70}, /* 4:                      transpar2                     */
		{  0,140,140}, /* 5:                      transpar2+          transpar0 */
		{180,130,  0}, /* 6:                      transpar2+transpar1           */
		{ 55, 70,140}, /* 7:                      transpar2+transpar1+transpar0 */
		{250,250,  0}, /* 8:            transpar3                               */
		{ 85,105,160}, /* 9:            transpar3+                    transpar0 */
		{190, 80,100}, /* 10:           transpar3+          transpar1           */
		{ 70, 50,150}, /* 11:           transpar3+          transpar1+transpar0 */
		{ 80,210,  0}, /* 12:           transpar3+transpar2                     */
		{ 50,105,130}, /* 13:           transpar3+transpar2+          transpar0 */
		{170,110,  0}, /* 14:           transpar3+transpar2+transpar1           */
		{ 60, 60,130}, /* 15:           transpar3+transpar2+transpar1+transpar0 */
		{180,180,180}, /* 16: transpar4+                                        */
		{  0,  0,180}, /* 17: transpar4+                              transpar0 */
		{200,  0,100}, /* 18: transpar4+                    transpar1           */
		{ 60,  0,140}, /* 19: transpar4+                    transpar1+transpar0 */
		{ 50,230, 50}, /* 20: transpar4+          transpar2                     */
		{  0,120,120}, /* 21: transpar4+          transpar2+          transpar0 */
		{160,110,  0}, /* 22: transpar4+          transpar2+transpar1           */
		{ 35, 50,120}, /* 23: transpar4+          transpar2+transpar1+transpar0 */
		{230,230,  0}, /* 24: transpar4+transpar3                               */
		{ 65, 85,140}, /* 25: transpar4+transpar3+                    transpar0 */
		{170, 60, 80}, /* 26: transpar4+transpar3+          transpar1           */
		{ 50, 30,130}, /* 27: transpar4+transpar3+          transpar1+transpar0 */
		{ 60,190,  0}, /* 28: transpar4+transpar3+transpar2                     */
		{ 30, 85,110}, /* 29: transpar4+transpar3+transpar2+          transpar0 */
		{150, 90,  0}, /* 30: transpar4+transpar3+transpar2+transpar1           */
		{ 40, 40,110}, /* 31: transpar4+transpar3+transpar2+transpar1+transpar0 */
	};

	TECH_COLORMAP *mapptr, *thisptr;
	REGISTER INTBIG i;
	REGISTER VARIABLE *var, *rvar, *gvar, *bvar;
	static INTBIG USER_color_map_key = 0;
	INTBIG red[256], green[256], blue[256];
	extern GRAPHICS us_gbox;

	/* get the technology's color information */
	if (USER_color_map_key == 0) USER_color_map_key = makekey(x_("USER_color_map"));
	var = getvalkey((INTBIG)tech, VTECHNOLOGY, VCHAR|VISARRAY, USER_color_map_key);
	if (var != NOVARIABLE) mapptr = (TECH_COLORMAP *)var->addr; else mapptr = 0;

	/* get existing color information */
	rvar = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_red_key);
	gvar = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_green_key);
	bvar = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_blue_key);

	/* must have some colors */
	if (rvar == NOVARIABLE && gvar == NOVARIABLE && bvar == NOVARIABLE && var == NOVARIABLE)
	{
		mapptr = default_colmap;
	}

	if (style == COLORSEXISTING)
	{
		/* not resetting, get the old color values */
		for(i=0; i<256; i++)
		{
			red[i] = ((INTBIG *)rvar->addr)[i];
			green[i] = ((INTBIG *)gvar->addr)[i];
			blue[i] = ((INTBIG *)bvar->addr)[i];
			if ((i&(LAYERH|LAYERG|LAYEROE)) != 0) continue;
			if (mapptr == 0) continue;
			if (i == 0) continue;
			thisptr = &mapptr[i>>2];
			red[i]  = thisptr->red;   green[i]   = thisptr->green;
			blue[i] = thisptr->blue;
		}
	} else
	{
#if SIMTOOL
		/* update simulation window colors */
		switch (style)
		{
			case COLORSWHITE:
			case COLORSDEFAULT:
				sim_window_setdisplaycolor(OFF_STRENGTH, BLUE);
				sim_window_setdisplaycolor(NODE_STRENGTH, GREEN);
				sim_window_setdisplaycolor(GATE_STRENGTH, MAGENTA);
				sim_window_setdisplaycolor(VDD_STRENGTH, BLACK);
				sim_window_setdisplaycolor(LOGIC_LOW, BLUE);
				sim_window_setdisplaycolor(LOGIC_HIGH, MAGENTA);
				sim_window_setdisplaycolor(LOGIC_X, BLACK);
				sim_window_setdisplaycolor(LOGIC_Z, LGRAY);
				break;
			case COLORSBLACK:
				sim_window_setdisplaycolor(OFF_STRENGTH, GREEN);
				sim_window_setdisplaycolor(NODE_STRENGTH, CYAN);
				sim_window_setdisplaycolor(GATE_STRENGTH, MAGENTA);
				sim_window_setdisplaycolor(VDD_STRENGTH, LRED);
				sim_window_setdisplaycolor(LOGIC_LOW, GREEN);
				sim_window_setdisplaycolor(LOGIC_HIGH, MAGENTA);
				sim_window_setdisplaycolor(LOGIC_X, LRED);
				sim_window_setdisplaycolor(LOGIC_Z, DGRAY);
				break;
		}
#endif

		/* resetting: load entirely new color map */
		for(i=0; i<256; i++)
		{
			if ((i&LAYERH) != 0)
			{
				switch (style)
				{
					case COLORSDEFAULT: thisptr = &colmap[33];   break;	/* white */
					case COLORSBLACK:   thisptr = &colmap[34];   break;	/* red */
					case COLORSWHITE:   thisptr = &colmap[35];   break;	/* red */
				}
			} else if ((i&LAYERG) != 0)
			{
				switch (style)
				{
					case COLORSDEFAULT: thisptr = &colmap[32];   break;	/* black */
					case COLORSBLACK:   thisptr = &colmap[33];   break;	/* white */
					case COLORSWHITE:   thisptr = &colmap[32];   break;	/* black */
				}
			} else if ((i&LAYEROE) != 0)
			{
				thisptr = &colmap[i>>2];

				if (i == HMENBOR) switch (style)
				{
					case COLORSBLACK: thisptr = &colmap[2];   break;	/* red */
					case COLORSWHITE: thisptr = &colmap[2];   break;	/* red */
				}
				if (i == CURSOR) switch (style)
				{
					case COLORSDEFAULT: thisptr = &colmap[16];   break;	/* default */
					case COLORSBLACK:   thisptr = &colmap[36];   break;	/* white */
					case COLORSWHITE:   thisptr = &colmap[37];   break;	/* black */
				}

				/* reverse black and white when using black background */
				if (style == COLORSBLACK)
				{
					switch (i)
					{
						case CELLTXT:
						case CELLOUT:
						case WINBOR:
						case MENBOR:
						case MENTXT:
						case MENGLY:
						case BLACK:    thisptr = &colmap[33];   break;		/* white */
						case WHITE:    thisptr = &colmap[37];   break;		/* black */
					}
				}
			} else
			{
				if (rvar != NOVARIABLE) red[i] = ((INTBIG *)rvar->addr)[i];
				if (gvar != NOVARIABLE) green[i] = ((INTBIG *)gvar->addr)[i];
				if (bvar != NOVARIABLE) blue[i] = ((INTBIG *)bvar->addr)[i];
				if (mapptr != 0) thisptr = &mapptr[i>>2]; else thisptr = 0;
				if (i == ALLOFF) switch (style)
				{
					case COLORSDEFAULT: thisptr = &default_colmap[0]; break;	/* default */
					case COLORSBLACK:   thisptr = &colmap[32];        break;	/* black */
					case COLORSWHITE:   thisptr = &colmap[33];        break;	/* white */
				}
				if (thisptr == 0) continue;
			}
			red[i]  = thisptr->red;   green[i]   = thisptr->green;
			blue[i] = thisptr->blue;
		}

		/* also set the grid color appropriately if it doesn't have its own bitplane */
		if (el_maplength < 256)
		{
			switch (style)
			{
				case COLORSDEFAULT: us_gbox.col = BLACK;   break;	/* black */
				case COLORSBLACK:   us_gbox.col = WHITE;   break;	/* white */
				case COLORSWHITE:   us_gbox.col = BLACK;   break;	/* black */
			}
		}
	}

	/* set the color map */
	if (broadcast)
		startobjectchange((INTBIG)us_tool, VTOOL);
	if (!broadcast) nextchangequiet();
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_colormap_red_key, (INTBIG)red,
		VINTEGER|VISARRAY|(256<<VLENGTHSH));
	if (!broadcast) nextchangequiet();
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_colormap_green_key, (INTBIG)green,
		VINTEGER|VISARRAY|(256<<VLENGTHSH));
	if (!broadcast) nextchangequiet();
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_colormap_blue_key, (INTBIG)blue,
		VINTEGER|VISARRAY|(256<<VLENGTHSH));
	if (broadcast)
		endobjectchange((INTBIG)us_tool, VTOOL);
}

/*
 * routine to load entry "entry" of the global color map entries with the value
 * (red, green, blue), letter "letter".  Handles highlight and grid layers
 * right if "spread" is true.
 */
void us_setcolorentry(INTBIG entry1, INTBIG red, INTBIG green, INTBIG blue, INTBIG letter,
	BOOLEAN spread)
{
	REGISTER INTBIG j;
	Q_UNUSED( letter );

	startobjectchange((INTBIG)us_tool, VTOOL);
	(void)setindkey((INTBIG)us_tool, VTOOL, us_colormap_red_key, entry1, red);
	(void)setindkey((INTBIG)us_tool, VTOOL, us_colormap_green_key, entry1, green);
	(void)setindkey((INTBIG)us_tool, VTOOL, us_colormap_blue_key, entry1, blue);

	/* place in other entries if special */
	if ((entry1&LAYERH) == LAYERH && spread)
	{
		/* set all highlight colors */
		for(j=0; j<256; j++) if ((j&LAYERH) == LAYERH)
		{
			(void)setindkey((INTBIG)us_tool, VTOOL, us_colormap_red_key, j, red);
			(void)setindkey((INTBIG)us_tool, VTOOL, us_colormap_green_key, j, green);
			(void)setindkey((INTBIG)us_tool, VTOOL, us_colormap_blue_key, j, blue);
		}
	} else if ((entry1&(LAYERG|LAYERH)) == LAYERG && spread)
	{
		/* set all grid colors */
		for(j=0; j<256; j++) if ((j&(LAYERG|LAYERH)) == LAYERG)
		{
			(void)setindkey((INTBIG)us_tool, VTOOL, us_colormap_red_key, j, red);
			(void)setindkey((INTBIG)us_tool, VTOOL, us_colormap_green_key, j, green);
			(void)setindkey((INTBIG)us_tool, VTOOL, us_colormap_blue_key, j, blue);
		}
	}
	endobjectchange((INTBIG)us_tool, VTOOL);
}

/*
 * routine to convert a red/green/blue color in (ir,ig,ib) to a hue/saturation/
 * intensity color in (h,s,i)
 */
void us_rgbtohsv(INTBIG ir, INTBIG ig, INTBIG ib, float *h, float *s, float *i)
{
	float x, r, g, b, rdot, gdot, bdot;

	r = ir / 255.0f;
	g = ig / 255.0f;
	b = ib / 255.0f;

	/* "i" is maximum of "r", "g", and "b" */
	if (r > g) *i = r; else *i = g;
	if (b > *i) *i = b;

	/* "x" is minimum of "r", "g", and "b" */
	if (r < g) x = r; else x = g;
	if (b < x) x = b;

	/* "saturation" is (i-x)/i */
	if (*i == 0.0) *s = 0.0; else *s = (*i - x) / *i;

	if (*s == 0.0) *h = 0.0; else
	{
		rdot = (*i - r) / (*i - x);
		gdot = (*i - g) / (*i - x);
		bdot = (*i - b) / (*i - x);
		if (b == x && r == *i) *h = (1.0f - gdot) / 6.0f; else
		if (b == x && g == *i) *h = (1.0f + rdot) / 6.0f; else
		if (r == x && g == *i) *h = (3.0f - bdot) / 6.0f; else
		if (r == x && b == *i) *h = (3.0f + gdot) / 6.0f; else
		if (g == x && b == *i) *h = (5.0f - rdot) / 6.0f; else
		if (g == x && r == *i) *h = (5.0f + bdot) / 6.0f; else
			ttyputmsg(_("Cannot convert (%ld,%ld,%ld), for x=%g i=%g s=%g"), ir, ig, ib, x, *i, *s);
	}
}

/*
 * routine to convert a hue/saturation/intensity color in (h,s,v) to a red/
 * green/blue color in (r,g,b)
 */
void us_hsvtorgb(float h, float s, float v, INTBIG *r, INTBIG *g, INTBIG *b)
{
	REGISTER INTBIG i;
	REGISTER float f, m, n, k;

	h = h * 6.0f;
	i = (INTBIG)h;
	f = h - (float)i;
	m = v * (1.0f - s);
	n = v * (1.0f - s * f);
	k = v * (1.0f - s * (1.0f - f));
	switch (i)
	{
		case 0:
			*r = (INTBIG)(v*255.0); *g = (INTBIG)(k*255.0); *b = (INTBIG)(m*255.0);
			break;
		case 1:
			*r = (INTBIG)(n*255.0); *g = (INTBIG)(v*255.0); *b = (INTBIG)(m*255.0);
			break;
		case 2:
			*r = (INTBIG)(m*255.0); *g = (INTBIG)(v*255.0); *b = (INTBIG)(k*255.0);
			break;
		case 3:
			*r = (INTBIG)(m*255.0); *g = (INTBIG)(n*255.0); *b = (INTBIG)(v*255.0);
			break;
		case 4:
			*r = (INTBIG)(k*255.0); *g = (INTBIG)(m*255.0); *b = (INTBIG)(v*255.0);
			break;
		case 5:
			*r = (INTBIG)(v*255.0); *g = (INTBIG)(m*255.0); *b = (INTBIG)(n*255.0);
			break;
	}
	if (*r < 0 || *r > 255 || *g < 0 || *g > 255 || *b < 0 || *b > 255)
		ttyputmsg(x_("(%g,%g,%g) -> (%ld,%ld,%ld) (i=%ld)"),h, s, v, *r, *g, *b, i);
}

/*
 * Routine to return an array of integers that describes the print colors for technology "tech".
 * The array has 5 entries for each layer in the technology.  The first 3 are the red/green/blue
 * color.  The 4th entry is the opacity and the 5th entry is the foreground factor.
 *
 * Opacity indicates how much a layer should obscure lower levels.  It is
 * a fractional integer (where 1.0 is WHOLE).
 * Set it to WHOLE if the layer should be opaque, or a smaller fraction if lower
 * levels should partially show through.
 * 
 * Foreground should be 1 if the layer should show up beneath partially
 * transparent layers.  It is set to 0 for layers like wells and selects
 * that should not show through.
 * 
 * The R, G, and B coordinates are on a scale of 0 to 255 from dark to light.
 * White layers (R=G=B=255) will not be visible.
 */
INTBIG *us_getprintcolors(TECHNOLOGY *tech)
{
	REGISTER VARIABLE *var, *varred, *vargreen, *varblue;
	REGISTER INTBIG i, fun, col, tot;
	float opacity;

	/* make sure the array is there */
	if (us_printcolordata != 0) efree((CHAR *)us_printcolordata);
	us_printcolordata = (INTBIG *)emalloc(tech->layercount*5*SIZEOFINTBIG, io_tool->cluster);
	if (us_printcolordata == 0) return(0);

	/* presume no print colors and create them from the display colors */
	varred = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_red_key);
	vargreen = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_green_key);
	varblue = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_blue_key);
	if (varred == NOVARIABLE || vargreen == NOVARIABLE || varblue == NOVARIABLE) return(0);
	for(i=0; i<tech->layercount; i++)
	{
		fun = layerfunction(tech, i);
		if ((fun&LFPSEUDO) != 0) continue;
		col = tech->layers[i]->col;
		us_printcolordata[i*5] = ((INTBIG *)varred->addr)[col];
		us_printcolordata[i*5+1] = ((INTBIG *)vargreen->addr)[col];
		us_printcolordata[i*5+2] = ((INTBIG *)varblue->addr)[col];
		opacity = 1.0f;
		switch (fun&LFTYPE)
		{
			case LFMETAL12:   opacity *= 0.9f;
			case LFMETAL11:   opacity *= 0.9f;
			case LFMETAL10:   opacity *= 0.9f;
			case LFMETAL9:    opacity *= 0.9f;
			case LFMETAL8:    opacity *= 0.9f;
			case LFMETAL7:    opacity *= 0.9f;
			case LFMETAL6:    opacity *= 0.9f;
			case LFMETAL5:    opacity *= 0.9f;
			case LFMETAL4:    opacity *= 0.9f;
			case LFMETAL3:    opacity *= 0.9f;
			case LFMETAL2:    opacity *= 0.9f;
			case LFMETAL1:    opacity *= 0.9f;
		}
		us_printcolordata[i*5+3] = (INTBIG)(opacity*WHOLE);
		if ((fun&LFTYPE) == LFIMPLANT || (fun&LFTYPE) == LFSUBSTRATE ||
			(fun&LFTYPE) == LFWELL) us_printcolordata[i*5+4] = 0; else
				us_printcolordata[i*5+4] = 1;
	}

	/* if there are print colors, use them */
	var = getval((INTBIG)tech, VTECHNOLOGY, VINTEGER|VISARRAY, x_("USER_print_colors"));
	if (var != NOVARIABLE)
	{
		tot = getlength(var);
		if (tot > tech->layercount*5) tot = tech->layercount*5;
		for(i=0; i<tot; i++) us_printcolordata[i] = ((INTBIG *)var->addr)[i];
	}
	return(us_printcolordata);
}

/******************** MISCELLANEOUS ********************/

/*
 * Routine to determine the offset of the "length" and "width" attributes on
 * node "ni" given that it uses a text descriptor of "descript".
 */
void us_getlenwidoffset(NODEINST *ni, UINTBIG *descript, INTBIG *xoff, INTBIG *yoff)
{
	REGISTER INTBIG i;

	*xoff = *yoff = 0;
	i = TXTGETQLAMBDA(TDGETSIZE(descript));
	if (i > 4) i /= 2; else i = 2;
	switch (ni->rotation)
	{
		case 0:
			if (ni->transpose == 0) *yoff = i; else
				*xoff = -i;
			break;
		case 900:
			if (ni->transpose == 0) *xoff = i; else
				*yoff = i;
			break;
		case 1800:
			if (ni->transpose == 0) *yoff = -i; else
				*xoff = i;
			break;
		case 2700:
			if (ni->transpose == 0) *xoff = -i; else
				*yoff = -i;
			break;
	}
}

/*
 * Routine to return the placement angle to use for node "np".
 */
INTBIG us_getplacementangle(NODEPROTO *np)
{
	REGISTER VARIABLE *var;

	var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER, us_placement_angle_key);
	if (var != NOVARIABLE) return(var->addr);
	var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_placement_angle_key);
	if (var != NOVARIABLE) return(var->addr);
	return(0);
}

/*
 * Routine to adjust the coordinates (cx,cy) to account for the fact that they are from primitive
 * "prim" and in cell "cell".  If this is not a primitive, or if the technology of the primitive
 * matches the technology of the cell, then do nothing.  Otherwise, adjust the size of the coordinates
 * so that the technology/lambda of the primitive is adjusted to that of the cell.
 */
void us_adjustfornodeincell(NODEPROTO *prim, NODEPROTO *cell, INTBIG *cx, INTBIG *cy)
{
	REGISTER LIBRARY *lib;
	REGISTER INTBIG celllambda, primlambda;

	if (prim->primindex == 0) return;
	if (cell == NONODEPROTO) return;
	if (prim->tech == cell->tech) return;
	if (cell->tech == gen_tech) return;

	lib = cell->lib;
	celllambda = lib->lambda[cell->tech->techindex],
	primlambda = lib->lambda[prim->tech->techindex],
	*cx = muldiv(*cx, celllambda, primlambda);
	*cy = muldiv(*cy, celllambda, primlambda);
}

/*
 * Routine to get the "displayed" location of node "ni" and return it in
 * (xpos, ypos).
 */
void us_getnodedisplayposition(NODEINST *ni, INTBIG *xpos, INTBIG *ypos)
{
	REGISTER NODEPROTO *np;
	INTBIG cox, coy, plx, ply, phx, phy;
	REGISTER INTBIG dx, dy;
	REGISTER VARIABLE *var;
	XARRAY trans;

	np = ni->proto;
	if ((us_useroptions&CENTEREDPRIMITIVES) == 0)
	{
		corneroffset(ni, np, ni->rotation, ni->transpose, &cox, &coy, FALSE);
		*xpos = ni->lowx+cox;
		*ypos = ni->lowy+coy;
	} else
	{
		var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER|VISARRAY, el_prototype_center_key);
		if (var != NOVARIABLE)
		{
			dx = ((INTBIG *)var->addr)[0] + (ni->lowx+ni->highx)/2 -
				(np->lowx+np->highx)/2;
			dy = ((INTBIG *)var->addr)[1] + (ni->lowy+ni->highy)/2 -
				(np->lowy+np->highy)/2;
			makerot(ni, trans);
			xform(dx, dy, &cox, &coy, trans);
			*xpos = cox;
			*ypos = coy;
		} else
		{
			nodesizeoffset(ni, &plx, &ply, &phx, &phy);
			makerot(ni, trans);
			dx = (ni->lowx+plx+ni->highx-phx)/2;
			dy = (ni->lowy+ply+ni->highy-phy)/2;
			xform(dx, dy, xpos, ypos, trans);
		}
	}
}

/*
 * routine to put cell center (x, y) on cell "np".
 */
void us_setnodeprotocenter(INTBIG x, INTBIG y, NODEPROTO *np)
{
	INTBIG position[2];

	position[0] = x;   position[1] = y;
	nextchangequiet();
	(void)setvalkey((INTBIG)np, VNODEPROTO, el_prototype_center_key,
		(INTBIG)position, VINTEGER|VISARRAY|(2<<VLENGTHSH));
}

/*
 * routine to remove cell center from cell "np".
 */
void us_delnodeprotocenter(NODEPROTO *np)
{
	nextchangequiet();
	(void)delvalkey((INTBIG)np, VNODEPROTO, el_prototype_center_key);
}

/*
 * Routine called when any of the "essential bounds" nodes are created, destroyed, or moved
 * in cell "cell".
 */
void us_setessentialbounds(NODEPROTO *cell)
{
	REGISTER NODEINST *ni;
	REGISTER INTBIG lx, hx, ly, hy, x, y, essentialcount;
	REGISTER VARIABLE *var;
	INTBIG bounds[4];

	essentialcount = 0;
	lx = hx = ly = hy = 0;
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto != gen_essentialprim) continue;
		x = (ni->lowx + ni->highx) / 2;
		y = (ni->lowy + ni->highy) / 2;
		if (essentialcount == 0)
		{
			lx = hx = x;
			ly = hy = y;
		} else
		{
			if (x < lx) lx = x;
			if (x > hx) hx = x;
			if (y < ly) ly = y;
			if (y > hy) hy = y;
		}
		essentialcount++;
	}
	if (essentialcount > 2)
	{
		/* make sure they are all on the bounds */
		for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if (ni->proto != gen_essentialprim) continue;
			x = (ni->lowx + ni->highx) / 2;
			y = (ni->lowy + ni->highy) / 2;
			if ((x == lx || x == hx) && (y == ly || y == hy)) continue;
			ttyputerr(_("Warning: Essential bounds markers inconsistent in cell %s"),
				describenodeproto(cell));
			break;
		}
	}

	/* set the essential bounds */
	if (essentialcount <= 1)
	{
		/* delete the bounds */
		var = getvalkey((INTBIG)cell, VNODEPROTO, VINTEGER|VISARRAY, el_essential_bounds_key);
		if (var != NOVARIABLE)
		{
			nextchangequiet();
			delvalkey((INTBIG)cell, VNODEPROTO, el_essential_bounds_key);
		}
	} else
	{
		bounds[0] = lx;   bounds[1] = hx;
		bounds[2] = ly;   bounds[3] = hy;
		nextchangequiet();
		setvalkey((INTBIG)cell, VNODEPROTO, el_essential_bounds_key, (INTBIG)bounds,
			VINTEGER|VISARRAY|(4<<VLENGTHSH));
	}
}

void us_getlowleft(NODEINST *ni, INTBIG *x, INTBIG *y)
{
	INTBIG lx, ly, hx, hy;
	XARRAY trans;
	static POLYGON *poly = NOPOLYGON;

	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	nodesizeoffset(ni, &lx, &ly, &hx, &hy);
	maketruerectpoly(ni->lowx+lx, ni->highx-hx, ni->lowy+ly, ni->highy-hy, poly);
	if (ni->rotation != 0 || ni->transpose != 0)
	{
		makerot(ni, trans);
		xformpoly(poly, trans);
	}
	getbbox(poly, x, &hx, y, &hy);
}

/*
 * routine to modify the text descriptor in the highlighted object "high"
 */
void us_modifytextdescript(HIGHLIGHT *high, UINTBIG *descript)
{
	REGISTER VARIABLE *var;

	if (high->fromvar != NOVARIABLE)
	{
		var = high->fromvar;
		if (high->fromvarnoeval != NOVARIABLE) var = high->fromvarnoeval;
		if (TDDIFF(descript, var->textdescript))
		{
			if (high->fromport != NOPORTPROTO)
			{
				modifydescript((INTBIG)high->fromport, VPORTPROTO, var, descript);
			} else if (high->fromgeom == NOGEOM)
			{
				modifydescript((INTBIG)high->cell, VNODEPROTO, var, descript);
			} else
			{
				modifydescript((INTBIG)high->fromgeom->entryaddr.blind,
					high->fromgeom->entryisnode ? VNODEINST : VARCINST, var, descript);
			}
		}
		return;
	}
	if (high->fromport != NOPORTPROTO)
	{
		(void)setind((INTBIG)high->fromport, VPORTPROTO, x_("textdescript"), 0, descript[0]);
		(void)setind((INTBIG)high->fromport, VPORTPROTO, x_("textdescript"), 1, descript[1]);
		return;
	}
	if (high->fromgeom->entryisnode)
	{
		(void)setind((INTBIG)high->fromgeom->entryaddr.ni, VNODEINST,
			x_("textdescript"), 0, descript[0]);
		(void)setind((INTBIG)high->fromgeom->entryaddr.ni, VNODEINST,
			x_("textdescript"), 1, descript[1]);
		return;
	}
}

/*
 * Routine to adjust the popup menu stored on the user interface object to correspond
 * to changes made to entry "pindex" of memory-structure popup "pm".
 */
void us_adjustpopupmenu(POPUPMENU *pm, INTBIG pindex)
{
	CHAR **lines, *popupname;
	CHAR *pt;
	VARIABLE *var;
	COMMANDBINDING commandbinding;
	REGISTER void *infstr;

	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("USER_binding_popup_"));
	addstringtoinfstr(infstr, pm->name);
	popupname = returninfstr(infstr);
	var = getval((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, popupname);
	if (var != NOVARIABLE)
	{
		lines = (CHAR **)var->addr;
		us_parsebinding(lines[pindex+1], &commandbinding);
		infstr = initinfstr();
		for(pt = lines[pindex+1]; *pt != 0; pt++)
		{
			if (estrncmp(pt, x_("message=\""), 9) == 0)
			{
				addstringtoinfstr(infstr, x_("message=\""));
				pt += 9;
				while (*pt != 0 && *pt != '"') pt++;
				addstringtoinfstr(infstr, pm->list[pindex].attribute);
			}
			addtoinfstr(infstr, *pt);
		}
		nextchangequiet();
		(void)setind((INTBIG)us_tool, VTOOL, popupname, pindex+1, (INTBIG)returninfstr(infstr));
		us_freebindingparse(&commandbinding);
	}
}

/*
 * Routine to set the 9 entries of an XARRAY on a C object using change control.
 * This must be done as individual index changes because "setval()" cannot handle
 * arrays on fixed objects (objects in the C structures).  The array is attribute
 * "name" on object "addr" of type "type".  It is filled from "array".
 */
void us_setxarray(INTBIG addr, INTBIG type, CHAR *name, XARRAY array)
{
	REGISTER INTBIG i, x, y;

	i = 0;
	for(y=0; y<3; y++)
	{
		for(x=0; x<3; x++)
		{
			(void)setind(addr, type, name, i, array[y][x]);
			i++;
		}
	}
}

/*
 * Routine to return true if "name" is legal for objects of type "type".
 */
BOOLEAN us_validname(CHAR *name, INTBIG type)
{
	REGISTER CHAR *pt;

	for(pt = name; *pt != 0; pt++)
	{
		if (*pt == ' ' || *pt == '\t')
		{
			ttyputerr(_("Name cannot have embedded spaces"));
			return(FALSE);
		}
		if (*pt < ' ' || *pt >= 0177)
		{
			ttyputerr(_("Name has unprintable characters"));
			return(FALSE);
		}
		if (type == VNODEPROTO)
		{
			if (*pt == ':' || *pt == ';' || *pt == '{')
			{
				ttyputerr(_("Name cannot have '%c' in it"), *pt);
				return(FALSE);
			}
		}
		if (type == VARCPROTO || type == VTECHNOLOGY || type == VLIBRARY)
		{
			if (*pt == ':')
			{
				ttyputerr(_("Name cannot have '%c' in it"), *pt);
				return(FALSE);
			}
		}
	}
	return(TRUE);
}

/*
 * Helper routine to determine the proper "address" field to use from variable "var".
 * Normally, it is simply "var->addr", but if it is a string with the "++" or "--"
 * sequence in it, then it auto-increments/decrements a numeric value, and so the
 * "++"/"--" are removed, and the original variable (which resides on "addr"/"type")
 * is modified.
 */
INTBIG us_inheritaddress(INTBIG addr, INTBIG type, VARIABLE *var)
{
	REGISTER CHAR *str;
	CHAR line[30];
	REGISTER INTBIG i, j, len, incrpoint, retval, curval;
	REGISTER void *infstr;

	/* if it isn't a string, just return its address */
	if ((var->type&(VCODE1|VCODE1)) == 0 && (var->type & VTYPE) != VSTRING) return(var->addr);
	if ((var->type & VISARRAY) != 0) return(var->addr);

	str = (CHAR *)var->addr;
	len = estrlen(str);
	for(i=0; i<len; i++)
	{
		if (str[i] == '+' && str[i+1] == '+') break;
		if (str[i] == '-' && str[i+1] == '-') break;
	}
	if (i >= len) return(var->addr);

	/* construct the proper inherited string and increment the variable */
	incrpoint = 0;
	infstr = initinfstr();
	for(i=0; i<len; i++)
	{
		if (str[i] == '+' && str[i+1] == '+')
		{
			incrpoint = i;
			i++;
			continue;
		}
		if (str[i] == '-' && str[i+1] == '-')
		{
			incrpoint = i;
			i++;
			continue;
		}
		addtoinfstr(infstr, str[i]);
	}

	/* get the new value */
	retval = (INTBIG)returninfstr(infstr);

	/* increment the variable */
	for(i = incrpoint-1; i>0; i--)
		if (!isdigit(str[i])) break;
	i++;
	str[incrpoint] = 0;
	curval = myatoi(&str[i]);
	str[incrpoint] = str[incrpoint+1];
	if (str[incrpoint] == '+') curval++; else curval--;
	infstr = initinfstr();
	for(j=0; j<i; j++)
		addtoinfstr(infstr, str[j]);
	esnprintf(line, 30, x_("%ld"), curval);
	addstringtoinfstr(infstr, line);
	addstringtoinfstr(infstr, &str[incrpoint]);
	(void)setval(addr, type, makename(var->key), (INTBIG)returninfstr(infstr), var->type); 

	return(retval);
}

/*
 * Routine to update the location of parameters on node "ni".
 * Returns TRUE if adjustments were made.
 */
BOOLEAN us_adjustparameterlocations(NODEINST *ni)
{
	REGISTER INTBIG i, xo, yo;
	REGISTER UINTBIG pos, size, face, rot, it, bo, un;
	REGISTER BOOLEAN changed;
	REGISTER VARIABLE *npvar, *nivar, *iconvar;
	REGISTER NODEPROTO *np, *cnp;
	REGISTER NODEINST *icon;

	/* get the location from the example icon */
	np = ni->proto;
	cnp = contentsview(np);
	if (cnp == NONODEPROTO) return(FALSE);

	/* look for an example of the icon in the contents */
	for(icon = cnp->firstnodeinst; icon != NONODEINST; icon = icon->nextnodeinst)
		if (icon->proto == np) break;
	if (icon == NONODEINST) return(FALSE);

	/* now adjust the positions to match the icon */
	changed = FALSE;
	for(i=0; i<cnp->numvar; i++)
	{
		/* find a parameter */
		npvar = &cnp->firstvar[i];
		if (TDGETINHERIT(npvar->textdescript) == 0) continue;

		/* find the parameter on this instance */
		nivar = getvalkeynoeval((INTBIG)ni, VNODEINST, -1, npvar->key);
		if (nivar == NOVARIABLE) continue;

		/* see what the icon's position is */
		iconvar = getvalkeynoeval((INTBIG)icon, VNODEINST, -1, npvar->key);
		if (iconvar == NOVARIABLE) continue;

		/* see if the offsets are the same */
		xo = TDGETXOFF(iconvar->textdescript);
		yo = TDGETYOFF(iconvar->textdescript);
		pos = TDGETPOS(iconvar->textdescript);
		size = TDGETSIZE(iconvar->textdescript);
		face = TDGETFACE(iconvar->textdescript);
		rot = TDGETROTATION(iconvar->textdescript);
		it = TDGETITALIC(iconvar->textdescript);
		bo = TDGETBOLD(iconvar->textdescript);
		un = TDGETUNDERLINE(iconvar->textdescript);
		if (xo == TDGETXOFF(nivar->textdescript) && yo == TDGETYOFF(nivar->textdescript) &&
			pos == TDGETPOS(nivar->textdescript) && size == TDGETSIZE(nivar->textdescript) &&
			face == TDGETFACE(nivar->textdescript) && rot == TDGETROTATION(nivar->textdescript) &&
			it == TDGETITALIC(nivar->textdescript) && bo == TDGETBOLD(nivar->textdescript) &&
			un == TDGETUNDERLINE(nivar->textdescript)) continue;
		if (!changed)
			startobjectchange((INTBIG)ni, VNODEINST);
		TDSETOFF(nivar->textdescript, xo, yo);
		TDSETPOS(nivar->textdescript, pos);
		TDSETSIZE(nivar->textdescript, size);
		TDSETFACE(nivar->textdescript, face);
		TDSETROTATION(nivar->textdescript, rot);
		TDSETITALIC(nivar->textdescript, it);
		TDSETBOLD(nivar->textdescript, bo);
		TDSETUNDERLINE(nivar->textdescript, un);
		changed = TRUE;
	}
	if (changed) endobjectchange((INTBIG)ni, VNODEINST);
	return(changed);
}

/*
 * Routine to inherit all prototype attributes down to instance "ni".
 */
void us_inheritattributes(NODEINST *ni)
{
	REGISTER NODEPROTO *np, *cnp;
	REGISTER NODEINST *icon;
	REGISTER INTBIG i, j;
	REGISTER VARIABLE *var, *ovar;
	REGISTER BOOLEAN found, first;
	REGISTER PORTPROTO *pp, *cpp;

	/* ignore primitives */
	np = ni->proto;
	if (np->primindex != 0) return;

	/* first inherit directly from this node's prototype */
	for(i=0; i<np->numvar; i++)
	{
		var = &np->firstvar[i];
		if (TDGETINHERIT(var->textdescript) == 0) continue;
		us_inheritcellattribute(var, ni, np, NONODEINST);
	}

	/* inherit directly from each port's prototype */
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		us_inheritexportattributes(pp, ni, np);

	/* if this node is an icon, also inherit from the contents prototype */
	cnp = contentsview(np);
	if (cnp != NONODEPROTO)
	{
		/* look for an example of the icon in the contents */
		for(icon = cnp->firstnodeinst; icon != NONODEINST; icon = icon->nextnodeinst)
			if (icon->proto == np) break;

		for(i=0; i<cnp->numvar; i++)
		{
			var = &cnp->firstvar[i];
			if (TDGETINHERIT(var->textdescript) == 0) continue;
			us_inheritcellattribute(var, ni, cnp, icon);
		}
		for(cpp = cnp->firstportproto; cpp != NOPORTPROTO; cpp = cpp->nextportproto)
			us_inheritexportattributes(cpp, ni, cnp);
	}

	/* now delete parameters that are not in the prototype */
	cnp = contentsview(np);
	if (cnp == NONODEPROTO) cnp = np;
	found = TRUE;
	first = TRUE;
	while (found)
	{
		found = FALSE;
		for(i=0; i<ni->numvar; i++)
		{
			var = &ni->firstvar[i];
			if (TDGETISPARAM(var->textdescript) == 0) continue;
			for(j=0; j<cnp->numvar; j++)
			{
				ovar = &cnp->firstvar[j];
				if (TDGETISPARAM(ovar->textdescript) == 0) continue;
				if (ovar->key == var->key) break;
			}
			if (j >= cnp->numvar)
			{
				if (first) startobjectchange((INTBIG)ni, VNODEINST);
				first = FALSE;
				delvalkey((INTBIG)ni, VNODEINST, var->key);
				found = TRUE;
				break;
			}
		}
	}
	if (!first) endobjectchange((INTBIG)ni, VNODEINST);
}

/*
 * Routine to add all inheritable export variables from export "pp" on cell "np"
 * to instance "ni".
 */
void us_inheritexportattributes(PORTPROTO *pp, NODEINST *ni, NODEPROTO *np)
{
	REGISTER INTSML saverot, savetrn;
	REGISTER INTBIG i, dx, dy, lambda, style=0;
	INTBIG x, y;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	REGISTER VARIABLE *var, *newvar;
	REGISTER CHAR *pt, *attrname;
	XARRAY trans;
	REGISTER void *infstr;
	Q_UNUSED( np );

	for(i=0; i<pp->numvar; i++)
	{
		var = &pp->firstvar[i];
		if (TDGETINHERIT(var->textdescript) == 0) continue;
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("ATTRP_"));
		addstringtoinfstr(infstr, pp->protoname);
		addstringtoinfstr(infstr, x_("_"));
		pt = makename(var->key);
		addstringtoinfstr(infstr, &pt[5]);
		attrname = returninfstr(infstr);

		/* see if the attribute is already there */
		newvar = getval((INTBIG)ni, VNODEINST, -1, attrname);
		if (newvar != NOVARIABLE) continue;

		/* set the attribute */
		startobjectchange((INTBIG)ni, VNODEINST);
		newvar = setval((INTBIG)ni, VNODEINST, attrname,
			us_inheritaddress((INTBIG)pp, VPORTPROTO, var), var->type);
		if (newvar != NOVARIABLE)
		{
			lambda = lambdaofnode(ni);
			TDCOPY(descript, var->textdescript);
			dx = TDGETXOFF(descript);
			dx = dx * lambda / 4;
			dy = TDGETYOFF(descript);
			dy = dy * lambda / 4;

			saverot = pp->subnodeinst->rotation;
			savetrn = pp->subnodeinst->transpose;
			pp->subnodeinst->rotation = pp->subnodeinst->transpose = 0;
			portposition(pp->subnodeinst, pp->subportproto, &x, &y);
			pp->subnodeinst->rotation = saverot;
			pp->subnodeinst->transpose = savetrn;
			x += dx;   y += dy;
			makerot(pp->subnodeinst, trans);
			xform(x, y, &x, &y, trans);
			maketrans(ni, trans);
			xform(x, y, &x, &y, trans);
			makerot(ni, trans);
			xform(x, y, &x, &y, trans);
			x = x - (ni->lowx + ni->highx) / 2;
			y = y - (ni->lowy + ni->highy) / 2;
			switch (TDGETPOS(descript))
			{
				case VTPOSCENT:      style = TEXTCENT;      break;
				case VTPOSBOXED:     style = TEXTBOX;       break;
				case VTPOSUP:        style = TEXTBOT;       break;
				case VTPOSDOWN:      style = TEXTTOP;       break;
				case VTPOSLEFT:      style = TEXTRIGHT;     break;
				case VTPOSRIGHT:     style = TEXTLEFT;      break;
				case VTPOSUPLEFT:    style = TEXTBOTRIGHT;  break;
				case VTPOSUPRIGHT:   style = TEXTBOTLEFT;   break;
				case VTPOSDOWNLEFT:  style = TEXTTOPRIGHT;  break;
				case VTPOSDOWNRIGHT: style = TEXTTOPLEFT;   break;
			}
			makerot(pp->subnodeinst, trans);
			style = rotatelabel(style, TDGETROTATION(descript), trans);
			switch (style)
			{
				case TEXTCENT:     TDSETPOS(descript, VTPOSCENT);      break;
				case TEXTBOX:      TDSETPOS(descript, VTPOSBOXED);     break;
				case TEXTBOT:      TDSETPOS(descript, VTPOSUP);        break;
				case TEXTTOP:      TDSETPOS(descript, VTPOSDOWN);      break;
				case TEXTRIGHT:    TDSETPOS(descript, VTPOSLEFT);      break;
				case TEXTLEFT:     TDSETPOS(descript, VTPOSRIGHT);     break;
				case TEXTBOTRIGHT: TDSETPOS(descript, VTPOSUPLEFT);    break;
				case TEXTBOTLEFT:  TDSETPOS(descript, VTPOSUPRIGHT);   break;
				case TEXTTOPRIGHT: TDSETPOS(descript, VTPOSDOWNLEFT);  break;
				case TEXTTOPLEFT:  TDSETPOS(descript, VTPOSDOWNRIGHT); break;
			}
			x = x * 4 / lambda;
			y = y * 4 / lambda;
			TDSETOFF(descript, x, y);
			TDSETINHERIT(descript, 0);
			TDCOPY(newvar->textdescript, descript);
		}
		endobjectchange((INTBIG)ni, VNODEINST);
	}
}

/*
 * Routine to add inheritable variable "var" from cell "np" to instance "ni".
 * If "icon" is not NONODEINST, use the position of the variable from it.
 */
void us_inheritcellattribute(VARIABLE *var, NODEINST *ni, NODEPROTO *np, NODEINST *icon)
{
	REGISTER VARIABLE *newvar, *ivar, *posvar;
	REGISTER INTBIG xc, yc, lambda, i, newtype;

	/* see if the attribute is already there */
	newvar = getvalkey((INTBIG)ni, VNODEINST, -1, var->key);
	if (newvar != NOVARIABLE)
	{
		/* make sure visibility is OK */
		if (TDGETINTERIOR(var->textdescript) == 0)
		{
			/* parameter should be visible: make it so */
			if ((newvar->type&VDISPLAY) == 0)
			{
				startobjectchange((INTBIG)ni, VNODEINST);
				newvar->type |= VDISPLAY;
				endobjectchange((INTBIG)ni, VNODEINST);
			}
		} else
		{
			/* parameter not normally visible: make it invisible if it has the default value */
			if ((newvar->type&VDISPLAY) != 0)
			{
				if (estrcmp(describevariable(var, -1, -1), describevariable(newvar, -1, -1)) == 0)
				{
					startobjectchange((INTBIG)ni, VNODEINST);
					newvar->type &= ~VDISPLAY;
					endobjectchange((INTBIG)ni, VNODEINST);
				}
			}
		}
		return;
	}

	/* determine offset of the attribute on the instance */
	posvar = var;
	if (icon != NONODEINST)
	{
		ivar = NOVARIABLE;
		for(i=0; i<icon->numvar; i++)
		{
			ivar = &icon->firstvar[i];
			if (ivar->key == var->key) break;
		}
		if (i < icon->numvar) posvar = ivar;
	}

	lambda = np->lib->lambda[np->tech->techindex];
	xc = TDGETXOFF(posvar->textdescript) * lambda / 4;
	if (posvar == var) xc -= (np->lowx + np->highx) / 2;
	yc = TDGETYOFF(posvar->textdescript) * lambda / 4;
	if (posvar == var) yc -= (np->lowy + np->highy) / 2;
	lambda = lambdaofnode(ni);
	xc = xc * 4 / lambda;
	yc = yc * 4 / lambda;

	/* set the attribute */
	startobjectchange((INTBIG)ni, VNODEINST);
	newtype = (var->type & ~(VDISPLAY|VTYPE|VCODE1|VCODE2)) |
		(posvar->type & (VDISPLAY|VTYPE|VCODE1|VCODE2));
	if (TDGETINTERIOR(var->textdescript) != 0) newtype &= ~VDISPLAY;
	newvar = setvalkey((INTBIG)ni, VNODEINST, var->key,
		us_inheritaddress((INTBIG)np, VNODEPROTO, posvar), newtype);
	if (newvar != NOVARIABLE)
	{
		defaulttextsize(3, newvar->textdescript);
		TDCOPY(newvar->textdescript, posvar->textdescript);
		TDSETINHERIT(newvar->textdescript, 0);
		TDSETOFF(newvar->textdescript, xc, yc);
		if (TDGETISPARAM(var->textdescript) != 0)
		{
			TDSETINTERIOR(newvar->textdescript, 0);
			i = TDGETDISPPART(newvar->textdescript);
			if (i == VTDISPLAYNAMEVALINH || i == VTDISPLAYNAMEVALINHALL)
				TDSETDISPPART(newvar->textdescript, VTDISPLAYNAMEVALUE);
		}
	}
	endobjectchange((INTBIG)ni, VNODEINST);
}

/*
 * Routine to add a parameter attribute to node "ni".  The variable key is "key",
 * the new value is "addr", and the type is "type".
 */
void us_addparameter(NODEINST *ni, INTBIG key, INTBIG addr, INTBIG type, UINTBIG *descript)
{
	REGISTER VARIABLE *var;
	UINTBIG textdescript[TEXTDESCRIPTSIZE];

	us_findexistingparameter(ni, key, textdescript);
	startobjectchange((INTBIG)ni, VNODEINST);
	var = setvalkey((INTBIG)ni, VNODEINST, key, addr, type|VDISPLAY);
	if (var != NOVARIABLE)
	{
		TDCOPY(var->textdescript, textdescript);
		TDSETISPARAM(var->textdescript, VTISPARAMETER);
		TDSETINTERIOR(var->textdescript, 0);
		TDSETDISPPART(var->textdescript, VTDISPLAYNAMEVALUE);
		if (descript != 0) TDSETUNITS(var->textdescript, TDGETUNITS(descript));
	}
	endobjectchange((INTBIG)ni, VNODEINST);
}

/*
 * Routine to find the formal parameter that corresponds to the actual parameter
 * "var" on node "ni".  Returns NOVARIABLE if not a parameter or cannot be found.
 */
VARIABLE *us_findparametersource(VARIABLE *var, NODEINST *ni)
{
	REGISTER NODEPROTO *np, *cnp;
	REGISTER INTBIG k;
	REGISTER VARIABLE *nvar;

	/* find this parameter in the cell */
	np = ni->proto;
	cnp = contentsview(np);
	if (cnp != NONODEPROTO) np = cnp;
	for(k=0; k<np->numvar; k++)
	{
		nvar = &np->firstvar[k];
		if (namesame(makename(var->key), makename(nvar->key)) == 0) return(nvar);
	}
	return(NOVARIABLE);
}

/*
 * Routine to determine the location of parameter "key" on node "ni".
 * Returns the variable
 * The parameter offset is stored in (xoff, yoff).
 */
void us_findexistingparameter(NODEINST *ni, INTBIG key, UINTBIG *textdescript)
{
	REGISTER NODEINST *icon;
	REGISTER NODEPROTO *np, *cnp;
	REGISTER INTBIG i, numvar, count, xsum, yval, lowy, highy, xoff, yoff;
	REGISTER VARIABLE *firstvar, *var;

	/* special case if this is a parameter */
	np = ni->proto;
	cnp = contentsview(np);
	if (cnp != NONODEPROTO)
	{
		/* look for an example of the icon in the contents */
		for(icon = cnp->firstnodeinst; icon != NONODEINST; icon = icon->nextnodeinst)
			if (icon->proto == np) break;
		if (icon != NONODEINST)
		{
			var = NOVARIABLE;
			for(i=0; i<icon->numvar; i++)
			{
				var = &icon->firstvar[i];
				if (var->key == key) break;
			}
			if (i < icon->numvar)
			{
				TDCOPY(textdescript, var->textdescript);
				return;
			}
		}
	}

	numvar = ni->numvar;
	firstvar = ni->firstvar;
	count = xsum = 0;
	lowy = highy = 0;
	for(i=0; i<numvar; i++)
	{
		var = &firstvar[i];
		if (TDGETISPARAM(var->textdescript) == 0) continue;
		xsum += TDGETXOFF(var->textdescript);
		yval = TDGETYOFF(var->textdescript);
		if (count == 0) lowy = highy = yval; else
		{
			if (yval < lowy) lowy = yval;
			if (yval > highy) highy = yval;
		}
		count++;
	}
	defaulttextsize(3, textdescript);
	if (count == 0) xoff = yoff = 0; else
	{
		xoff = xsum / count;
		if (count == 1) yoff = lowy - 4; else
			yoff = lowy - (highy - lowy) / (count-1);
	}
	TDSETOFF(textdescript, xoff, yoff);
}

/*
 * Routine to determine the location of a new parameter in cell "np".
 * The parameter offset is stored in (xoff, yoff).
 */
void us_getnewparameterpos(NODEPROTO *np, INTBIG *xoff, INTBIG *yoff)
{
	REGISTER INTBIG i, numvar, count, xsum, yval, lowy, highy;
	REGISTER VARIABLE *firstvar, *var;

	numvar = np->numvar;
	firstvar = np->firstvar;
	count = xsum = 0;
	lowy = highy = 0;
	for(i=0; i<numvar; i++)
	{
		var = &firstvar[i];
		if (TDGETISPARAM(var->textdescript) == 0) continue;
		xsum += TDGETXOFF(var->textdescript);
		yval = TDGETYOFF(var->textdescript);
		if (count == 0) lowy = highy = yval; else
		{
			if (yval < lowy) lowy = yval;
			if (yval > highy) highy = yval;
		}
		count++;
	}
	if (count == 0) *xoff = *yoff = 0; else
	{
		*xoff = xsum / count;
		if (count == 1) *yoff = lowy - 4; else
			*yoff = lowy - (highy - lowy) / (count-1);
	}
}

static INTBIG      us_coveragepolyscrunched;
static INTBIG      us_coveragejobsize;
static INTBIG     *us_coveragelayers;
static INTBIG      us_coveragelayercount;
static TECHNOLOGY *us_coveragetech;
static void       *us_coveragedialog;
static float      *us_coveragearea;

void us_gathercoveragegeometry(NODEPROTO *np, XARRAY trans, void **merge);
void us_getcoveragegeometry(INTBIG layer, TECHNOLOGY *tech, INTBIG *x, INTBIG *y, INTBIG count);

/*
 * Routine to compute the percentage of coverage for the polysilicon and metal layers.
 */
void us_showlayercoverage(NODEPROTO *cell)
{
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;
	REGISTER VARIABLE *var;
	REGISTER INTBIG i, fun, lambda, percentcoverage;
	REGISTER float totalarea, coverageratio;
	REGISTER void **submerge, **polymerge;

	/* initialize for analysis */
	lambda = lambdaofcell(cell);
	us_coveragetech = cell->tech;

	/* determine which layers are being collected */
	us_coveragelayercount = 0;
	for(i=0; i<us_coveragetech->layercount; i++)
	{
		fun = layerfunction(us_coveragetech, i);
		if ((fun&LFPSEUDO) != 0) continue;
		if (!layerismetal(fun) && !layerispoly(fun)) continue;
		us_coveragelayercount++;
	}
	if (us_coveragelayercount == 0)
	{
		ttyputerr(_("No metal or polysilicon layers in this technology"));
		return;
	}
	us_coveragelayers = (INTBIG *)emalloc(us_coveragelayercount * SIZEOFINTBIG, us_tool->cluster);
	if (us_coveragelayers == 0) return;
	us_coveragelayercount = 0;
	for(i=0; i<us_coveragetech->layercount; i++)
	{
		fun = layerfunction(us_coveragetech, i);
		if ((fun&LFPSEUDO) != 0) continue;
		if (!layerismetal(fun) && !layerispoly(fun)) continue;
		us_coveragelayers[us_coveragelayercount++] = i;
	}

	/* show the progress dialog */
	us_coveragedialog = DiaInitProgress(_("Merging geometry..."), 0);
	if (us_coveragedialog == 0)
	{
		termerrorlogging(TRUE);
		return;
	}
	DiaSetProgress(us_coveragedialog, 0, 1);

	/* reset merging information */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			np->temp1 = 0;

			/* if this cell has parameters, force its polygons to be examined for every instance */
			for(i=0; i<np->numvar; i++)
			{
				var = &np->firstvar[i];
				if (TDGETISPARAM(var->textdescript) != 0) break;
			}
			if (i < np->numvar) np->temp1 = -1;
		}
	}

	/* run through the work and count the number of polygons */
	us_coveragepolyscrunched = 0;
	us_gathercoveragegeometry(cell, el_matid, 0);
	us_coveragejobsize = us_coveragepolyscrunched;

	/* now gather all of the geometry into the polygon merging system */
	polymerge = (void **)emalloc(us_coveragelayercount * (sizeof (void *)), us_tool->cluster);
	if (polymerge == 0) return;
	for(i=0; i<us_coveragelayercount; i++)
		polymerge[i] = mergenew(us_tool->cluster);
	us_coveragepolyscrunched = 0;
	us_gathercoveragegeometry(cell, el_matid, polymerge);

	/* extract the information */
	us_coveragearea = (float *)emalloc(us_coveragelayercount * (sizeof (float)), us_tool->cluster);
	if (us_coveragearea == 0) return;
	for(i=0; i<us_coveragelayercount; i++)
	{
		us_coveragearea[i] = 0.0;
		mergeextract(polymerge[i], us_getcoveragegeometry);
	}

	/* show the results */
	totalarea = (float)(cell->highx - cell->lowx);
	totalarea *= (float)(cell->highy - cell->lowy);
	ttyputmsg(x_("Cell is %g square lambda"), totalarea/(float)lambda/(float)lambda);
	for(i=0; i<us_coveragelayercount; i++)
	{
		if (us_coveragearea[i] == 0.0) continue;
		if (totalarea == 0.0) coverageratio = 0.0; else
			coverageratio = us_coveragearea[i] / totalarea;
		percentcoverage = (INTBIG)(coverageratio * 100.0 + 0.5);

		ttyputmsg(x_("Layer %s covers %g square lambda (%ld%%)"),
			layername(us_coveragetech, us_coveragelayers[i]),
			us_coveragearea[i]/(float)lambda/(float)lambda, percentcoverage);
	}

	/* delete merge information */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->temp1 == 0 || np->temp1 == -1) continue;
			submerge = (void **)np->temp1;
			for(i=0; i<us_coveragelayercount; i++)
				mergedelete(submerge[i]);
			efree((CHAR *)submerge);
		}
	}
	for(i=0; i<us_coveragelayercount; i++)
		mergedelete(polymerge[i]);
	efree((CHAR *)polymerge);

	/* clean up */
	efree((CHAR *)us_coveragelayers);
	DiaDoneProgress(us_coveragedialog);
}

/*
 * Recursive helper routine to gather all well areas in cell "np"
 * (transformed by "trans").  If "merge" is nonzero, merge geometry into it.
 * If "gathernet" is true, follow networks, looking for special primitives and nets. 
 */
void us_gathercoveragegeometry(NODEPROTO *np, XARRAY trans, void **merge)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *subnp;
	REGISTER ARCINST *ai;
	REGISTER GEOM *geom;
	REGISTER INTBIG i, j, tot, updatepct, sea;
	REGISTER void **submerge;
	static INTBIG checkstop = 0;
	XARRAY localrot, localtran, final, subrot;
	static POLYGON *poly = NOPOLYGON;

	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* do a spatial search so that polygon merging grows sensibly */
	updatepct = 0;
	sea = initsearch(np->lowx, np->highx, np->lowy, np->highy, np);
	for(;;)
	{
		geom = nextobject(sea);
		if (geom == NOGEOM) break;
		checkstop++;
		if ((checkstop%10) == 0)
		{
			if (stopping(STOPREASONERC)) { termsearch(sea);   return; }
		}
		if (!geom->entryisnode)
		{
			ai = geom->entryaddr.ai;
			tot = arcpolys(ai, NOWINDOWPART);
			us_coveragepolyscrunched += tot;
			if (merge != 0)
			{
				if (us_coveragejobsize > 0)
				{
					if (updatepct++ > 25)
					{
						DiaSetProgress(us_coveragedialog, us_coveragepolyscrunched, us_coveragejobsize);
						updatepct = 0;
					}
				}
				for(i=0; i<tot; i++)
				{
					shapearcpoly(ai, i, poly);
					if (poly->tech != us_coveragetech) continue;
					for(j=0; j<us_coveragelayercount; j++)
					{
						if (poly->layer == us_coveragelayers[j])
						{
							xformpoly(poly, trans);
							mergeaddpolygon(merge[j], poly->layer, poly->tech, poly);
						}
					}
				}
			}
			continue;
		}

		ni = geom->entryaddr.ni;
		subnp = ni->proto;
		makerot(ni, localrot);
		transmult(localrot, trans, final);
		if (subnp->primindex == 0)
		{
			maketrans(ni, localtran);
			transmult(localtran, final, subrot);

			if (merge != 0)
			{
				if (subnp->temp1 == -1)
				{
					/* parameterized cell: examine the contents recursively */
					us_gathercoveragegeometry(subnp, subrot, merge);
				} else
				{
					submerge = (void **)subnp->temp1;
					if (submerge == 0)
					{
						/* gather the subcell's merged information */
						submerge = (void **)emalloc(us_coveragelayercount * (sizeof (void *)),
							us_tool->cluster);
						if (submerge == 0) return;
						for(i=0; i<us_coveragelayercount; i++)
							submerge[i] = mergenew(us_tool->cluster);
						us_gathercoveragegeometry(subnp, el_matid, submerge);
						subnp->temp1 = (INTBIG)submerge;
					}

					for(i=0; i<us_coveragelayercount; i++)
						mergeaddmerge(merge[i], submerge[i], subrot);
				}
			}
		} else
		{
			tot = nodepolys(ni, 0, NOWINDOWPART);
			us_coveragepolyscrunched += tot;
			if (merge != 0)
			{
				if (us_coveragejobsize > 0)
				{
					if (updatepct++ > 25)
					{
						DiaSetProgress(us_coveragedialog, us_coveragepolyscrunched, us_coveragejobsize);
						updatepct = 0;
					}
				}
				for(i=0; i<tot; i++)
				{
					shapenodepoly(ni, i, poly);
					if (poly->tech != us_coveragetech) continue;
					for(j=0; j<us_coveragelayercount; j++)
					{
						if (poly->layer == us_coveragelayers[j])
						{
							xformpoly(poly, final);
							mergeaddpolygon(merge[j], poly->layer, poly->tech, poly);
						}
					}
				}
			}
		}
	}
}

/*
 * Coroutine of the polygon merging package that is given a merged polygon with
 * "count" points in (x,y) with area "area".
 */
void us_getcoveragegeometry(INTBIG layer, TECHNOLOGY *tech, INTBIG *x, INTBIG *y, INTBIG count)
{
	REGISTER INTBIG i;
	Q_UNUSED( tech );

	for(i=0; i<us_coveragelayercount; i++)
	{
		if (layer != us_coveragelayers[i]) continue;
		us_coveragearea[i] += areapoints(count, x, y);
		break;
	}
}
