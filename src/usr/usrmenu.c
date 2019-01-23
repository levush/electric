/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrmenu.c
 * User interface tool: menu display control
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
#include "usr.h"
#include "efunction.h"
#include "tecart.h"
#include "tecschem.h"

extern GRAPHICS us_ebox, us_nmbox, us_box;

/* for drawing text (nothing changes) */
GRAPHICS us_menufigs = {LAYERA, MENGLY, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};

/* for drawing menu shadows */
static GRAPHICS us_shadowdesc = {LAYERA, MENGLY, PATTERNED, PATTERNED,
	{0x5555,0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,
	0x5555,0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA}, NOVARIABLE, 0};

/* for drawing menu text (nothing changes) */
GRAPHICS us_menutext = {LAYERA, MENTXT, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};

#define MENUARCLENGTH   8	/* length of arcs in menu entries */
#define MENUSHADOW      2	/* size of shadow around popup menus */

/* prototypes for local routines */
static INTBIG us_groupfunction(NODEPROTO*);
#ifndef USEQT
static void us_pmfirst(void);
static void us_pmdone(void);
static BOOLEAN us_pmeachchar(INTBIG, INTBIG, INTSML);
static BOOLEAN us_pmeachdown(INTBIG, INTBIG);
static void us_pminvertline(void);
#endif

/****************************** MENU DISPLAY ******************************/

/*
 * routine to draw the entire display screen.  If "scaletofit" is positive,
 * each window will be rescaled to fit properly.  If "scaletofit" is negative,
 * each window will be adjusted to fit (but not shrunk back from edges).
 */
void us_drawmenu(INTBIG scaletofit, WINDOWFRAME *mw)
{
	REGISTER INTBIG x, y;
	BOOLEAN placemenu;
	REGISTER WINDOWPART *w;
	REGISTER VARIABLE *var;

	/* re-draw windows */
	if (scaletofit != 0) placemenu = TRUE; else
		placemenu = FALSE;
	us_windowfit(mw, placemenu, scaletofit);
	us_erasescreen(mw);
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		/* draw window border and its contents */
		if (mw != NOWINDOWFRAME && mw != w->frame) continue;
		us_drawwindow(w, el_colwinbor);
		if (w->redisphandler != 0) (*w->redisphandler)(w);
	}
	if (el_curwindowpart != NOWINDOWPART) us_highlightwindow(el_curwindowpart, TRUE);

	/* draw menus if requested */
	if ((us_tool->toolstate&MENUON) != 0)
	{
#ifdef USEQT
		if (us_menuframe == NOWINDOWFRAME || (mw != NOWINDOWFRAME && us_menuframe != mw)) return;
#else
		if (mw != NOWINDOWFRAME && us_menuframe != NOWINDOWFRAME &&
			us_menuframe != mw) return;
#endif

		/* draw the menu entries */
		var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_menu_key);
		if (var == NOVARIABLE) return;

		if (us_menupos <= 1)
		{
			/* horizontal menus */
			for(y=0; y<us_menuy; y++) for(x=0; x<us_menux; x++)
				us_drawmenuentry(x, y, ((CHAR **)var->addr)[y * us_menux + x]);
		} else
		{
			/* vertical menus */
			for(x=0; x<us_menux; x++) for(y=0; y<us_menuy; y++)
				us_drawmenuentry(x, y, ((CHAR **)var->addr)[x * us_menuy + y]);
		}

		/* highlight any special entry (shouldn't update status display too) */
		us_menuhnx = us_menuhax = -1;
		us_showcurrentnodeproto();
		us_showcurrentarcproto();
	}
}

void us_drawmenuentry(INTBIG x, INTBIG y, CHAR *str)
{
	REGISTER INTBIG i, scaleu, scaled, x1, y1, lowx, lowy, highx, highy, xl,
		yl, xh, yh, ofun, fun, lambda, bits, wid, twid;
	REGISTER NODEINST *ni;
	NODEINST node;
	REGISTER ARCINST *ai;
	ARCINST arc;
	REGISTER NODEPROTO *tnp;
	REGISTER ARCPROTO *tap;
	REGISTER TECHNOLOGY *tech;
	VARIABLE colorvar[2];
	WINDOWPART ww;
	XARRAY trans;
	CHAR line[100];
	INTBIG swid, shei, pangle;
	INTBIG pxs, pys;
	static POLYGON *poly = NOPOLYGON;
	WINDOWFRAME *menuframe;
	COMMANDBINDING commandbinding;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* setup the fake variable for coloring artwork objects in the menu */
	colorvar[0].key = art_colorkey;
	colorvar[0].type = VINTEGER;
	colorvar[0].addr = el_colmengly;

	if (stopping(STOPREASONDISPLAY)) return;
	if ((us_tool->toolstate&MENUON) == 0) return;

	/* parse the menu entry string */
	us_parsebinding(str, &commandbinding);

	/* set drawing bounds to address the world */
	if (us_menuframe != NOWINDOWFRAME) menuframe = us_menuframe; else
		menuframe = el_curwindowpart->frame;
	getwindowframesize(menuframe, &swid, &shei);
	ww.screenlx = ww.screenly = ww.uselx = ww.usely = 0;
	ww.screenhx = ww.usehx = swid-1;   ww.screenhy = ww.usehy = shei-1;
	ww.frame = menuframe;
	ww.curnodeproto = NONODEPROTO;
	ww.state = DISPWINDOW;
	computewindowscale(&ww);

	/* draw border around menu entry */
	xl = x * us_menuxsz + us_menulx;   xh = xl + us_menuxsz;
	yl = y * us_menuysz + us_menuly;   yh = yl + us_menuysz;
	maketruerectpoly(xl, xh, yl, yh, poly);
	poly->desc = &us_ebox;
	poly->style = FILLEDRECT;
	us_showpoly(poly, &ww);

	if (commandbinding.nodeglyph != NONODEPROTO || commandbinding.arcglyph != NOARCPROTO)
	{
		/* graphics will be drawn: draw an outline of background color */
		poly->desc = &us_box;
		us_box.col = commandbinding.backgroundcolor;
		poly->style = CLOSEDRECT;
		if (commandbinding.arcglyph != NOARCPROTO &&
			commandbinding.arcglyph == us_curarcproto)
		{
			maketruerectpoly(xl+2, xh-2, yl+2, yh-2, poly);
			us_showpoly(poly, &ww);
			maketruerectpoly(xl+3, xh-3, yl+3, yh-3, poly);
			us_showpoly(poly, &ww);
			maketruerectpoly(xl+4, xh-4, yl+4, yh-4, poly);
			us_showpoly(poly, &ww);
		}
		maketruerectpoly(xl+1, xh-1, yl+1, yh-1, poly);
	} else
	{
		/* text will be drawn: fill the entry with background color */
		poly->desc = &us_box;
		us_box.col = commandbinding.backgroundcolor;
		poly->style = FILLEDRECT;
	}
	us_showpoly(poly, &ww);

	maketruerectpoly(xl, xh, yl, yh, poly);
	poly->desc = &us_nmbox;
	us_nmbox.col = el_colmenbor;
	poly->style = CLOSEDRECT;
	us_showpoly(poly, &ww);

	/* stop now if there is no entry in this menu square */
	if (*commandbinding.command == 0)
	{
		us_freebindingparse(&commandbinding);
		return;
	}

	/* draw the internal glyph if there is one */
	if (commandbinding.nodeglyph != NONODEPROTO || commandbinding.arcglyph != NOARCPROTO)
	{
		if (commandbinding.nodeglyph != NONODEPROTO)
		{
			/* see if an alternate icon should be displayed */
			tnp = iconview(commandbinding.nodeglyph);
			if (tnp != NONODEPROTO) commandbinding.nodeglyph = tnp;

			/* build the dummy nodeinst for menu display */
			tech = commandbinding.nodeglyph->tech;
			if (tech == NOTECHNOLOGY) tech = el_curtech;
			lambda = el_curlib->lambda[tech->techindex];
			ni = &node;   initdummynode(ni);
			ni->proto = commandbinding.nodeglyph;
			pangle = us_getplacementangle(ni->proto);
			ni->rotation = (INTSML)(pangle % 3600);
			ni->transpose = (INTSML)(pangle / 3600);
			defaultnodesize(commandbinding.nodeglyph, &pxs, &pys);
			ni->userbits |= NEXPAND;
			lowx = ni->lowx  = -pxs / 2;
			highx = ni->highx = ni->lowx + pxs;
			lowy = ni->lowy  = -pys / 2;
			highy = ni->highy = ni->lowy + pys;

			/*
			 * Special hack: the Schematic Transistors do not fill their bounding box
			 * but stops 1 lambda short of the top.  To center it in the menu square,
			 * that amount is rotated and the node is shifted thusly.
			 */
			if (ni->proto == sch_transistorprim || ni->proto == sch_transistor4prim)
			{
				makerot(ni, trans);
				xform(0, el_curlib->lambda[sch_tech->techindex]/2, &pxs, &pys, trans);
				ni->lowx += pxs;
				ni->highx += pxs;
				ni->lowy += pys;
				ni->highy += pys;
			}

			/* fake the variable "ART_color" to be the glyph color */
			ni->firstvar = &colorvar[0];
			ni->numvar = 1;
			if (commandbinding.menumessage != 0)
			{
				/* add node specialization bits */
				bits = myatoi(commandbinding.menumessage);
				ni->userbits |= bits;
			}
		}
		if (commandbinding.arcglyph != NOARCPROTO)
		{
			/* build dummy display arcinst for menu */
			tech = commandbinding.arcglyph->tech;
			lambda = el_curlib->lambda[tech->techindex];
			ai = &arc;   initdummyarc(ai);
			ai->proto = commandbinding.arcglyph;
			ai->length = MENUARCLENGTH * lambda;
			ai->width = defaultarcwidth(commandbinding.arcglyph);
			if (ai->width > ai->length)
				ai->length = ai->width * 2;
			ai->end[0].xpos = -ai->length/2;
			ai->end[0].ypos = ai->end[1].ypos = 0;
			ai->end[1].xpos = ai->length/2;
			lowx = -(ai->width+ai->length)/2;  highx = -lowx;
			lowy = -ai->width / 2;             highy = -lowy;

			/* fake the variable "ART_color" to be the glyph color */
			ai->firstvar = &colorvar[0];
			ai->numvar = 1;
		}

		/* temporarily make everything visible */
		for(i=0; i<tech->layercount; i++)
		{
			tech->layers[i]->colstyle &= ~INVTEMP;
			if ((tech->layers[i]->colstyle&INVISIBLE) == 0) continue;
			tech->layers[i]->colstyle |= INVTEMP;
			tech->layers[i]->colstyle &= ~INVISIBLE;
		}

		/* compute the scale for graphics */
		scaleu = scaled = maxi(highx-lowx, highy-lowy);
		if (scaleu == 0) scaleu = scaled = 1;
#define INSET 2
		if (commandbinding.nodeglyph != NONODEPROTO)
		{
			/* make all nodes of the same class be the same scale */
			ofun = us_groupfunction(commandbinding.nodeglyph);
			for(tnp = tech->firstnodeproto; tnp != NONODEPROTO; tnp = tnp->nextnodeproto)
			{
				fun = us_groupfunction(tnp);
				if (fun != ofun) continue;
				defaultnodesize(tnp, &pxs, &pys);
				scaled = maxi(scaled, maxi(pxs, pys));
			}

			/* for pins, make them the same scale as the arcs */
			if (ofun == NPPIN)
			{
				for(tap = tech->firstarcproto; tap != NOARCPROTO; tap = tap->nextarcproto)
					scaled = maxi(scaled, MENUARCLENGTH*lambda + defaultarcwidth(tap));
			}
		} else if (commandbinding.arcglyph != NOARCPROTO)
		{
			/* make all arcs be the same scale */
			twid = defaultarcwidth(commandbinding.arcglyph);
			for(tap = tech->firstarcproto; tap != NOARCPROTO; tap = tap->nextarcproto)
			{
				wid = defaultarcwidth(tap);
				if (twid <= MENUARCLENGTH*lambda && wid <= MENUARCLENGTH*lambda)
					scaled = maxi(scaled, MENUARCLENGTH*lambda + wid);
				if (twid > MENUARCLENGTH*lambda && wid > MENUARCLENGTH*lambda)
					scaled = maxi(scaled, wid*3);
			}

			/* and make them at least the same scale as the pins */
			for(tnp = tech->firstnodeproto; tnp != NONODEPROTO; tnp = tnp->nextnodeproto)
			{
				fun = us_groupfunction(tnp);
				if (fun != NPPIN) continue;
				scaled = maxi(scaled, maxi(tnp->highx-tnp->lowx, tnp->highy-tnp->lowy));
			}
		}

		/* modify the window to transform nodeinst into the menu space */
		ww.uselx = xl+INSET;   ww.usehx = xh-INSET;
		ww.usely = yl+INSET;   ww.usehy = yh-INSET;
		ww.screenlx = muldiv(lowx, scaled, scaleu) - 1;
		ww.screenly = muldiv(lowy, scaled, scaleu) - 1;
		ww.screenhx = muldiv(highx, scaled, scaleu) + 1;
		ww.screenhy = muldiv(highy, scaled, scaleu) + 1;
		x1 = ww.screenhx - ww.screenlx;
		y1 = ww.screenhy - ww.screenly;
		if (x1 > y1)
		{
			ww.screenly -= (x1-y1) / 2;
			ww.screenhy += (x1-y1) / 2;
		} else
		{
			ww.screenlx -= (y1-x1) / 2;
			ww.screenhx += (y1-x1) / 2;
		}
		computewindowscale(&ww);

		/* draw the object into the menu */
		if (commandbinding.nodeglyph != NONODEPROTO)
		{
			begintraversehierarchy();
			makerot(ni, trans);
			if (us_drawnodeinst(ni, LAYERA, trans, 1, &ww) == 2)
				(void)us_drawnodeinst(ni, LAYERA, trans, 2, &ww);
			endtraversehierarchy();
		} else if (commandbinding.arcglyph != NOARCPROTO)
		{
			if (us_drawarcinst(ai, LAYERA, el_matid, 1, &ww) == 2)
				(void)us_drawarcinst(ai, LAYERA, el_matid, 2, &ww);
		}

		/* restore visibilities */
		for(i=0; i<tech->layercount; i++)
			if ((tech->layers[i]->colstyle&INVTEMP) != 0)
				tech->layers[i]->colstyle |= INVISIBLE;
	} else
	{
		/* no glyph: use text */
		makerectpoly(xl,xh, yl,yh, poly);
		poly->desc = &us_menutext;
		us_menutext.col = el_colmentxt;
		poly->style = TEXTBOX;
		TDCLEAR(poly->textdescript);
		TDSETSIZE(poly->textdescript, commandbinding.menumessagesize);
		poly->tech = el_curtech;
		if (commandbinding.menumessage != 0)
			poly->string = commandbinding.menumessage; else
		{
			(void)estrncpy(line, commandbinding.command, 99);
			poly->string = line;
			if (islower(line[0])) line[0] = toupper(line[0]);
		}
		us_showpoly(poly, &ww);
	}
	us_freebindingparse(&commandbinding);
}

/*
 * routine to return the function of nodeproto "np", grouped according to its
 * general function (i.e. all transistors return the same value).
 */
INTBIG us_groupfunction(NODEPROTO *np)
{
	REGISTER INTBIG fun;

	fun = (np->userbits&NFUNCTION) >> NFUNCTIONSH;
	switch (fun)
	{
		/* combine all transistors */
		case NPTRANMOS:   case NPTRA4NMOS:
		case NPTRADMOS:   case NPTRA4DMOS:
		case NPTRAPMOS:   case NPTRA4PMOS:
		case NPTRANPN:    case NPTRA4NPN:
		case NPTRAPNP:    case NPTRA4PNP:
		case NPTRANJFET:  case NPTRA4NJFET:
		case NPTRAPJFET:  case NPTRA4PJFET:
		case NPTRADMES:   case NPTRA4DMES:
		case NPTRAEMES:   case NPTRA4EMES:
		case NPTRANSREF:   fun = NPTRANS;       break;

		/* combine all analog parts */
		case NPRESIST:
		case NPCAPAC:
		case NPECAPAC:
		case NPDIODE:
		case NPDIODEZ:     fun = NPINDUCT;      break;

		/* combine all two-port parts */
		case NPCCVS:
		case NPCCCS:
		case NPVCVS:
		case NPVCCS:
		case NPTLINE:      fun = NPTLINE;       break;

		/* combine all transistor parts */
		case NPBASE:
		case NPEMIT:       fun = NPCOLLECT;     break;

		/* combine all logic parts */
		case NPBUFFER:
		case NPGATEAND:
		case NPGATEOR:
		case NPMUX:        fun = NPGATEXOR;     break;

		/* combine power and ground */
		case NPCONPOWER:   fun = NPCONGROUND;   break;

		/* combine all SPICE parts */
		case NPMETER:      fun = NPSOURCE;      break;

		/* combine all bias parts */
		case NPSUBSTRATE:  fun = NPWELL;        break;
	}
	return(fun);
}

/******************** MENU COMPUTATION ********************/

/*
 * routine to change the "getproto" commands in the menu to
 * properly reflect the primitive nodes and arcs in the current
 * technology
 */
void us_setmenunodearcs(void)
{
	REGISTER ARCPROTO *ap;
	REGISTER NODEPROTO *np, *pinnp;
	REGISTER INTBIG didinst, i;
	REGISTER BOOLEAN isgetproto;
	REGISTER VARIABLE *var;
	REGISTER void *infstr;
	CHAR si[20], sj[20], *par[MAXPARS+6];
	COMMANDBINDING commandbinding;
	extern COMCOMP us_userp;

	/* bind menu entries for arcs */
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_menu_key);
	if (var == NOVARIABLE) return;
	ap = el_curtech->firstarcproto;
	np = el_curtech->firstnodeproto;
	didinst = 0;
	for(i=0; i<us_menuy*us_menux; i++)
	{
		us_parsebinding(((CHAR **)var->addr)[i], &commandbinding);

		/* make sure the menu item is a "getproto" command */
		isgetproto = FALSE;
		if (commandbinding.command != 0)
		{
			if (namesamen(commandbinding.command, x_("getproto"), 8) == 0 ||
				namesamen(commandbinding.command, x_("pmgetproto"), 10) == 0 ||
				namesamen(commandbinding.command, x_("rem getproto"), 12) == 0 ||
				namesamen(commandbinding.command, x_("rem pmgetproto"), 14) == 0)
			{
				isgetproto = TRUE;
			}
		}
		if (isgetproto)
		{
			/* make sure the command is getting a primitive in a technology */
			if (commandbinding.nodeglyph == NONODEPROTO ||
				commandbinding.nodeglyph->primindex != 0)
			{
				/* load up the strings for the menu indices */
				if (us_menupos <= 1)
				{
					(void)esnprintf(si, 20, x_("%ld"), i%us_menux);
					(void)esnprintf(sj, 20, x_("%ld"), i/us_menux);
				} else
				{
					(void)esnprintf(si, 20, x_("%ld"), i/us_menuy);
					(void)esnprintf(sj, 20, x_("%ld"), i%us_menuy);
				}
				par[0] = x_("set");          par[1] = x_("menu");
				par[2] = x_("background");   par[3] = x_("blue");
				par[4] = sj;             par[5] = si;
				par[6] = x_("getproto");

				/* found a "getproto" command: now change it */
				if (ap != NOARCPROTO)
				{
					for(;;)
					{
						if (ap == NOARCPROTO) break;
						if ((ap->userbits&ANOTUSED) == 0)
						{
							pinnp = getpinproto(ap);
							if (pinnp == NONODEPROTO) break;
							if ((pinnp->userbits & NNOTUSED) == 0) break;
						}
						ap = ap->nextarcproto;
					}
					if (ap != NOARCPROTO)
					{
						par[3] = x_("red");
						par[7] = x_("arc");   par[8] = describearcproto(ap);
						us_bind(9, par);
						ap = ap->nextarcproto;
						us_freebindingparse(&commandbinding);
						continue;
					}
				}

				/* insert the instance popup between the arcs and the nodes */
				if (didinst == 0)
				{
					didinst = 1;
					if (parse(x_("pmgetproto"), &us_userp, FALSE) >= 0)
					{
						par[2] = x_("message");
						par[3] = _("Inst.");
						par[6] = x_("pmgetproto");
						par[7] = x_("instance");
						us_bind(8, par);
						us_freebindingparse(&commandbinding);
						continue;
					}
				}

				/* done with arcs, do the nodes */
				if (np != NONODEPROTO)
				{
					while (np != NONODEPROTO && (np->userbits & NNOTUSED) != 0)
						np = np->nextnodeproto;
					if (np != NONODEPROTO &&
						((np->userbits&NFUNCTION)>>NFUNCTIONSH) != NPNODE)
					{
						par[4] = x_("glyph");
						par[5] = describenodeproto(np);
						par[6] = sj;             par[7] = si;
						par[8] = x_("rem");
						par[9] = x_("getproto");
						par[10] = x_("node");
						infstr = initinfstr();
						formatinfstr(infstr, "%s:%s", np->tech->techname, nldescribenodeproto(np));
						par[11] = returninfstr(infstr);
						us_bind(12, par);
						np = np->nextnodeproto;
						us_freebindingparse(&commandbinding);
						continue;
					}
				}

				/* no nodes or arcs left: make it a blank entry */
				par[6] = x_("-");
				us_bind(7, par);
			}
		}
		us_freebindingparse(&commandbinding);
	}
}

/*
 * routine to set the number of entries in the menu to "x" by "y", and to
 * place the menu on the proper side of the screen according to "position":
 * 0: top,  1: bottom,  2: left,  3: right.  If "allgetproto" is true,
 * set every entry to the appropriate "getproto" command.
 */
void us_setmenusize(INTBIG x, INTBIG y, INTBIG position, BOOLEAN allgetproto)
{
	REGISTER INTBIG i, oldlen;
	REGISTER CHAR **temp, *last;
	REGISTER VARIABLE *oldvar;
	COMMANDBINDING commandbinding;

	/* validate orientation of menu */
	if (position <= 1)
	{
		/* menu runs horizontally across the top or bottom */
		if (y > x) { i = x;  x = y;  y = i; }
	} else
	{
		/* menu runs vertically down the left or right */
		if (y < x) { i = x;  x = y;  y = i; }
	}
	if (us_menux == x && us_menuy == y && us_menupos == position && !allgetproto)
		return;

	/* set new size, position */
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_menu_x_key, x, VINTEGER|VDONTSAVE);
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_menu_y_key, y, VINTEGER|VDONTSAVE);
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_menu_position_key, position, VINTEGER|VDONTSAVE);

	/* rebuild the binding array size */
	temp = (CHAR **)emalloc(x*y * (sizeof (CHAR *)), el_tempcluster);
	if (temp == 0) return;
	if (allgetproto)
	{
		for(i=0; i<x*y; i++) temp[i] = x_("command=getproto");
		oldlen = 0;
		last = 0;
	} else
	{
		for(i=0; i<x*y; i++) temp[i] = x_("");
		oldvar = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_menu_key);
		if (oldvar == NOVARIABLE) oldlen = 0; else oldlen = getlength(oldvar);
		last = NOSTRING;
		for(i=0; i<mini(oldlen, x*y); i++)
		{
			last = ((CHAR **)oldvar->addr)[i];
			(void)allocstring(&temp[i], last, el_tempcluster);
		}

		/* if old menu ends with a "getproto", extend their range */
		if (last != NOSTRING)
		{
			us_parsebinding(last, &commandbinding);
			if (i < x*y && *commandbinding.command != 0 &&
				namesamen(commandbinding.command, x_("getproto"), 8) == 0)
			{
				for(; i<x*y; i++) temp[i] = x_("command=getproto");
			} else last = NOSTRING;
			us_freebindingparse(&commandbinding);
		}
	}
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_binding_menu_key, (INTBIG)temp,
		VSTRING|VISARRAY|VDONTSAVE|((x*y)<<VLENGTHSH));
	for(i=0; i<mini(oldlen, x*y); i++) efree(temp[i]);
	efree((CHAR *)temp);
	if (last != NOSTRING) us_setmenunodearcs();
}

/******************** POPUP MENUS ********************/

#ifdef USEQT

/*
 * routine to display the popup menu "menu" and allow picking.  The menu
 * terminates when any button is clicked (a down-click is necessary if "*waitfordown"
 * is true).  If "header" is true, display a header.  If "top" and "left" are not
 * negative, they are used as the coordinates of the upper-left of the menu.  If
 * "quitline" is nonzero, then exit the menu when the cursor moves over that line
 * (1: left side, 2: right side, 3: pulldown menu extent, 4: popup menu extent).
 * Returns the address of the item in the menu that is selected and sets "menu" to
 * the menu (if a submenu in a hierarchy was used).  Returns NOPOPUPMENUITEM
 * if no entry was selected.  Returns zero if this function cannot be done.  If the
 * mouse is still down (because it crossed the quit line) then "waitfordown" is set
 * true.  Also sets the "changed" items to nonzero for any entries that are modified.
 */
POPUPMENUITEM *us_popupmenu(POPUPMENU **cmenu, BOOLEAN *waitfordown, BOOLEAN header,
	INTBIG left, INTBIG top, INTBIG quitline)
{
	INTBIG i = nativepopupmenu(cmenu, header, left, top);
	if (i < 0) return(NOPOPUPMENUITEM);
	return(&(*cmenu)->list[i]);
}

#else /* USEQT */

#define ATTVALGAP 20		/* distance between attribute and value columns */

/* structure for popup menus */
typedef struct
{
	WINDOWPART     popupwindow;		/* window in which popup resides */
	INTBIG         hline;			/* currently highlighted line in popup menu (-1 if none) */
	INTBIG         posx, posy;		/* position of lower-left corner of popup menu */
	INTBIG         sizex, sizey;	/* size of popup menu */
	INTBIG         txthei;			/* height of a line of the popup menu */
	INTBIG         yshift;			/* shift up amount when menu bumps bottom */
	INTBIG         attlen, vallen;	/* width of attribute and value fields in popup menu */
	INTBIG         curpos;			/* current character position in value field */
	BOOLEAN        header;			/* true if a header is to be displayed in popup menu */
	INTBIG         quitline;		/* nonzero if menu should abort when it crosses a line */
	INTBIG         inarea;			/* global flag for being inside an area */
	POPUPMENUITEM *retval;			/* return item */
	POPUPMENU     *retmenu;			/* menu of returned item */
	POPUPMENU     *current;			/* current popup menu structure */
} POPUPINST;

static POPUPINST us_pmcur;

/*
 * routine to display the popup menu "menu" and allow picking.  The menu
 * terminates when any button is clicked (a down-click is necessary if "*waitfordown"
 * is true).  If "header" is true, display a header.  If "top" and "left" are not
 * negative, they are used as the coordinates of the upper-left of the menu.  If
 * "quitline" is nonzero, then exit the menu when the cursor moves over that line
 * (1: left side, 2: right side, 3: pulldown menu extent, 4: popup menu extent).
 * Returns the address of the item in the menu that is selected and sets "menu" to
 * the menu (if a submenu in a hierarchy was used).  Returns NOPOPUPMENUITEM
 * if no entry was selected.  Returns zero if this function cannot be done.  If the
 * mouse is still down (because it crossed the quit line) then "waitfordown" is set
 * true.  Also sets the "changed" items to nonzero for any entries that are modified.
 */
POPUPMENUITEM *us_popupmenu(POPUPMENU **cmenu, BOOLEAN *waitfordown, BOOLEAN header,
	INTBIG left, INTBIG top, INTBIG quitline)
{
	REGISTER INTBIG i, yp, startfont;
	BOOLEAN waitdown;
	REGISTER CHAR *savepos, *pt;
	REGISTER INTBIG textscale, save_pix;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	INTBIG x, y, chwid, swid, shei;
	REGISTER POPUPMENU *menu;
	POPUPMENU *whichmenu;
	REGISTER POPUPMENUITEM *mi;
	REGISTER WINDOWFRAME *frame;

	/* see if this is a simple popup that could be handled by the OS */
	menu = *cmenu;
	for(i=0; i<menu->total; i++)
	{
		mi = &menu->list[i];
		if (mi->response != NOUSERCOM && mi->response->menu != NOPOPUPMENU)
			continue;
		if (mi->value != 0) break;
	}
	if (i >= menu->total)
	{
		/* no type-in entries: see if OS can draw this simple popup menu */
		if (graphicshas(CANSHOWPOPUP))
		{
			whichmenu = menu;
			i = nativepopupmenu(&whichmenu, header, left, top);
			if (i < 0) return(NOPOPUPMENUITEM);
			return(&whichmenu->list[i]);
		}
	}

	/* must draw the popup by hand */
	if (us_needwindow()) return(0);

	/* determine width of attribute and value halves in menu */
	waitdown = *waitfordown;
	*waitfordown = FALSE;
	us_pmcur.header = header;
	us_pmcur.current = menu;
	us_pmcur.inarea = 0;
	us_pmcur.quitline = quitline;
	us_pmcur.retval = NOPOPUPMENUITEM;
	us_pmcur.retmenu = menu;
	us_pmcur.yshift = 0;

	/* get size of current window */
	frame = getwindowframe(FALSE);
	getwindowframesize(frame, &swid, &shei);
	us_pmcur.popupwindow.screenlx = us_pmcur.popupwindow.screenly = 0;
	us_pmcur.popupwindow.uselx = us_pmcur.popupwindow.usely = 0;
	us_pmcur.popupwindow.screenhx = us_pmcur.popupwindow.usehx = swid-1;
	us_pmcur.popupwindow.screenhy = us_pmcur.popupwindow.usehy = shei-1;
	us_pmcur.popupwindow.frame = frame;
	us_pmcur.popupwindow.state = DISPWINDOW;
	computewindowscale(&us_pmcur.popupwindow);

	/* loop through fonts to find one that fits the menu */
#ifdef	sun
	startfont = 20;
#else
	startfont = 14;
#endif
	for(textscale = startfont; textscale >= 4; textscale--)
	{
		us_pmcur.attlen = us_pmcur.vallen = 0;
		TDCLEAR(descript);
		if (textscale == startfont) TDSETSIZE(descript, TXTMENU); else
			TDSETSIZE(descript, textscale);
		screensettextinfo(&us_pmcur.popupwindow, NOTECHNOLOGY, descript);
		screengettextsize(&us_pmcur.popupwindow, x_("0"), &chwid, &y);
		us_pmcur.txthei = y+2;
		for(i=0; i<us_pmcur.current->total; i++)
		{
			mi = &us_pmcur.current->list[i];
			mi->changed = FALSE;
			pt = us_stripampersand(mi->attribute);
			screengettextsize(&us_pmcur.popupwindow, pt, &x, &y);
			if (x > us_pmcur.attlen) us_pmcur.attlen = x;
			if (mi->value == 0) continue;
			screengettextsize(&us_pmcur.popupwindow, mi->value, &x, &y);
			if (x > us_pmcur.vallen) us_pmcur.vallen = x;
			if (mi->maxlen*chwid > us_pmcur.vallen) us_pmcur.vallen = mi->maxlen*chwid;
		}

		/* make sure the header will fit in the menu */
		if (header)
		{
			pt = us_stripampersand(us_pmcur.current->header);
			screengettextsize(&us_pmcur.popupwindow, pt, &x, &y);
			if (us_pmcur.vallen == 0)
			{
				if (x > us_pmcur.attlen) us_pmcur.attlen = x;
			} else
			{
				if (x > us_pmcur.attlen+us_pmcur.vallen+ATTVALGAP)
				{
					i = x - (us_pmcur.attlen+us_pmcur.vallen+ATTVALGAP);
					us_pmcur.attlen += i/2;
					us_pmcur.vallen = x - us_pmcur.attlen - ATTVALGAP;
				}
			}
		}

		/* determine the actual size of the menu */
		us_pmcur.sizex = us_pmcur.attlen + 4;
		if (us_pmcur.vallen != 0) us_pmcur.sizex += us_pmcur.vallen + ATTVALGAP;
		us_pmcur.sizey = us_pmcur.current->total * us_pmcur.txthei + 3;
		if (header) us_pmcur.sizey += us_pmcur.txthei;

		/* if the menu fits, quit */
		if (us_pmcur.sizex+MENUSHADOW <= swid &&
			us_pmcur.sizey+MENUSHADOW <= shei) break;
	}

	/* no text size will fit */
	if (textscale < 4) return(0);

	/* determine the location of the menu */
	readtablet(&x, &y);
	if (top < 0 || left < 0)
	{
		us_pmcur.posx = x-us_pmcur.sizex/2;
		us_pmcur.posy = y-us_pmcur.sizey/2;
	} else
	{
		if (quitline == 2) us_pmcur.posx = left - us_pmcur.sizex; else
			us_pmcur.posx = left;
		us_pmcur.posy = top-us_pmcur.sizey;
	}
	if (us_pmcur.posx < 0) us_pmcur.posx = 0;
	if (us_pmcur.posy <= MENUSHADOW)
	{
		us_pmcur.yshift = MENUSHADOW+1-us_pmcur.posy;
		us_pmcur.posy = MENUSHADOW+1;
	}
	if (us_pmcur.posx+us_pmcur.sizex+MENUSHADOW >= swid)
		us_pmcur.posx = swid - us_pmcur.sizex-MENUSHADOW-1;
	if (us_pmcur.posy+us_pmcur.sizey > shei)
		us_pmcur.posy = shei - us_pmcur.sizey;

	/* save the display under the menu */
	save_pix = screensavebox(&us_pmcur.popupwindow, us_pmcur.posx,
		us_pmcur.posx+us_pmcur.sizex+MENUSHADOW,
		us_pmcur.posy-MENUSHADOW-1, us_pmcur.posy+us_pmcur.sizey-1);

	/* write the menu on the screen */
	us_menufigs.col = el_colmengly;
	us_menutext.col = el_colmentxt;
	screendrawbox(&us_pmcur.popupwindow, us_pmcur.posx,
		us_pmcur.posx+us_pmcur.sizex-1,
		us_pmcur.posy, us_pmcur.posy+us_pmcur.sizey-1, &us_ebox);
	screendrawline(&us_pmcur.popupwindow, us_pmcur.posx, us_pmcur.posy,
		us_pmcur.posx+us_pmcur.sizex-1, us_pmcur.posy, &us_menufigs, 0);
	screendrawline(&us_pmcur.popupwindow, us_pmcur.posx+us_pmcur.sizex-1,
		us_pmcur.posy, us_pmcur.posx+us_pmcur.sizex-1,
		us_pmcur.posy+us_pmcur.sizey-1, &us_menufigs, 0);
	screendrawline(&us_pmcur.popupwindow, us_pmcur.posx+us_pmcur.sizex-1,
		us_pmcur.posy+us_pmcur.sizey-1, us_pmcur.posx,
			us_pmcur.posy+us_pmcur.sizey-1, &us_menufigs, 0);
	screendrawline(&us_pmcur.popupwindow, us_pmcur.posx,
		us_pmcur.posy+us_pmcur.sizey-1,
		us_pmcur.posx, us_pmcur.posy, &us_menufigs, 0);

	/* now draw a shadow */
	us_shadowdesc.col = el_colmengly;
	screendrawbox(&us_pmcur.popupwindow, us_pmcur.posx+MENUSHADOW,
		us_pmcur.posx+us_pmcur.sizex, us_pmcur.posy-MENUSHADOW-1,
		us_pmcur.posy-1, &us_shadowdesc);
	if (quitline != 2)
		screendrawbox(&us_pmcur.popupwindow, us_pmcur.posx+us_pmcur.sizex,
			us_pmcur.posx+us_pmcur.sizex+MENUSHADOW, us_pmcur.posy-MENUSHADOW-1,
			us_pmcur.posy+us_pmcur.sizey-MENUSHADOW, &us_shadowdesc);

	/* write the header */
	if (header)
	{
		screendrawline(&us_pmcur.popupwindow, us_pmcur.posx,
			us_pmcur.posy+us_pmcur.sizey-us_pmcur.txthei-2,
			us_pmcur.posx+us_pmcur.sizex-1,
			us_pmcur.posy+us_pmcur.sizey-us_pmcur.txthei-2,
			&us_menufigs, 0);
		pt = us_stripampersand(us_pmcur.current->header);
		screendrawtext(&us_pmcur.popupwindow, us_pmcur.posx+2,
			us_pmcur.posy+us_pmcur.sizey-us_pmcur.txthei-1,
			pt, &us_menutext);
		screeninvertbox(&us_pmcur.popupwindow, us_pmcur.posx+1,
			us_pmcur.posx+us_pmcur.sizex-2,
			us_pmcur.posy+us_pmcur.sizey-us_pmcur.txthei-1,
			us_pmcur.posy+us_pmcur.sizey-2);
		yp = us_pmcur.posy+us_pmcur.sizey-us_pmcur.txthei*2-2;
	} else yp = us_pmcur.posy+us_pmcur.sizey-us_pmcur.txthei-2;

	/* write the rest of the text in the menu */
	for(i=0; i<us_pmcur.current->total; i++)
	{
		mi = &us_pmcur.current->list[i];
		if (*mi->attribute == 0 && mi->value == 0)
		{
			/* no entry: draw a dotted line */
			screendrawline(&us_pmcur.popupwindow, us_pmcur.posx, yp+us_pmcur.txthei/2,
				us_pmcur.posx+us_pmcur.sizex-1, yp+us_pmcur.txthei/2,
				&us_menufigs, 1);
		} else
		{
			/* draw a text entry */
			savepos = &mi->attribute[estrlen(mi->attribute)-1];
			if (mi->attribute[0] != '>' && *savepos == '<') *savepos = 0; else
				savepos = 0;
			pt = us_stripampersand(mi->attribute);
			screendrawtext(&us_pmcur.popupwindow, us_pmcur.posx+2, yp, pt,
				&us_menutext);
			if (savepos != 0) *savepos = '<';
			if (mi->value != 0)
			{
				screendrawtext(&us_pmcur.popupwindow,
					us_pmcur.posx+us_pmcur.attlen+ATTVALGAP+2,
					yp, mi->value, &us_menutext);
			}
		}
		yp -= us_pmcur.txthei;
	}

	/* set the initial state of highlighted lines */
	if (x < us_pmcur.posx || x >= us_pmcur.posx+us_pmcur.sizex ||
		y < us_pmcur.posy || y >= us_pmcur.posy+us_pmcur.sizey-us_pmcur.txthei-2)
			us_pmcur.hline = -1; else
	{
		us_pmcur.hline = us_pmcur.current->total - (y - us_pmcur.posy) / us_pmcur.txthei - 1;
		us_pmcur.retval = &us_pmcur.current->list[us_pmcur.hline];
		us_pminvertline();
		us_pmcur.curpos = -1;
	}

	/* do command completion while handling input to menu */
	trackcursor(waitdown, us_pmeachdown, us_pmfirst, us_pmeachdown, us_pmeachchar,
		us_pmdone, (quitline == 0 ? TRACKSELECTING : TRACKHSELECTING));

	/* restore what was under the menu */
	screenrestorebox(save_pix, 1);

	/* report the menu entry selected */
	if (us_pmcur.inarea < 0) *waitfordown = TRUE;
	if (us_pmcur.retval != NOPOPUPMENUITEM) *cmenu = us_pmcur.retmenu;
	return(us_pmcur.retval);
}

void us_pmfirst(void) {}
void us_pmdone(void) {}

BOOLEAN us_pmeachchar(INTBIG x, INTBIG y, INTSML chr)
{
	REGISTER POPUPMENUITEM *mi;
	REGISTER INTBIG i;
	INTBIG xs, ys;

	/* set the cursor properly if it moved */
	(void)us_pmeachdown(x, y);

	/* quit now if carriage-return is typed */
	if (chr == '\n' || chr == '\r') return(TRUE);

	/* make sure there is a valid line */
	if (us_pmcur.hline == -1) return(FALSE);
	mi = &us_pmcur.current->list[us_pmcur.hline];
	if (mi->value == 0 || mi->maxlen < 0) return(FALSE);

	/* handle deletion of a character */
	if (chr == us_erasech)
	{
		if (us_pmcur.curpos <= 0) return(FALSE);
		us_pmcur.curpos--;
		mi->value[us_pmcur.curpos] = 0;
		screengettextsize(&us_pmcur.popupwindow, mi->value, &xs, &ys);
		us_pminvertline();
		screendrawbox(&us_pmcur.popupwindow,
			us_pmcur.posx+2+us_pmcur.attlen+ATTVALGAP+xs,
			us_pmcur.posx+us_pmcur.sizex-1,
			us_pmcur.posy+us_pmcur.sizey-us_pmcur.txthei*(us_pmcur.hline+2)-2,
			us_pmcur.posy+us_pmcur.sizey-us_pmcur.txthei*(us_pmcur.hline+1)-3,
			&us_ebox);
		us_pminvertline();
		return(FALSE);
	}

	/* handle deletion of the entire line */
	if (chr == us_killch)
	{
		if (us_pmcur.curpos <= 0) return(FALSE);
		us_pmcur.curpos = 0;
		mi->value[us_pmcur.curpos] = 0;
		us_pminvertline();
		screendrawbox(&us_pmcur.popupwindow,
			us_pmcur.posx+2+us_pmcur.attlen+ATTVALGAP,
			us_pmcur.posx+us_pmcur.sizex-1,
			us_pmcur.posy+us_pmcur.sizey-us_pmcur.txthei*(us_pmcur.hline+2)-2,
			us_pmcur.posy+us_pmcur.sizey-us_pmcur.txthei*(us_pmcur.hline+1)-3,
			&us_ebox);
		us_pminvertline();
		return(FALSE);
	}

	/* see if the new character will fit */
	if (us_pmcur.curpos >= us_pmcur.vallen || us_pmcur.curpos >= mi->maxlen) return(FALSE);

	/* do not allow unprintable characters */
	if (chr < 040) return(FALSE);

	/* on the first character, blank the line */
	if (us_pmcur.curpos < 0)
	{
		us_pmcur.curpos = 0;
		mi->changed = TRUE;
		us_pminvertline();
		screendrawbox(&us_pmcur.popupwindow,
			us_pmcur.posx+2+us_pmcur.attlen+ATTVALGAP,
			us_pmcur.posx+us_pmcur.sizex-1,
			us_pmcur.posy+us_pmcur.sizey-us_pmcur.txthei*(us_pmcur.hline+2)-2,
			us_pmcur.posy+us_pmcur.sizey-us_pmcur.txthei*(us_pmcur.hline+1)-3,
			&us_ebox);
		us_pminvertline();
		mi->value[0] = 0;
	}

	/* add a letter to a value field */
	screengettextsize(&us_pmcur.popupwindow, mi->value, &xs, &ys);
	i = us_pmcur.posx+2+us_pmcur.attlen+ATTVALGAP+xs;
	mi->value[us_pmcur.curpos] = (CHAR)chr;
	us_pmcur.curpos++;
	mi->value[us_pmcur.curpos] = 0;
	us_pminvertline();
	screendrawtext(&us_pmcur.popupwindow, i,
		us_pmcur.posy+us_pmcur.sizey-us_pmcur.txthei*(us_pmcur.hline+2)-2,
			&mi->value[us_pmcur.curpos-1], &us_menutext);
	us_pminvertline();
	return(FALSE);
}

/*
 * helper routine for popup menus to handle cursor movement
 */
BOOLEAN us_pmeachdown(INTBIG x, INTBIG y)
{
	REGISTER INTBIG line, maxy, i, lx;
	INTBIG sx, sy, swid, shei, pwid;
	REGISTER POPUPMENUITEM *pm;
	POPUPMENU *cmenu;
	REGISTER USERCOM *uc;
	POPUPINST savepmcur;
	BOOLEAN but;

	/* determine top coordinate of menu */
	maxy = us_pmcur.posy+us_pmcur.sizey-2;
	if (us_pmcur.header) maxy -= us_pmcur.txthei;

	getpaletteparameters(&swid, &shei, &pwid);

	for(;;)
	{
		/* compute current line in menu */
		if (x < us_pmcur.posx || x >= us_pmcur.posx+us_pmcur.sizex ||
			y < us_pmcur.posy || y > maxy) line = -1; else
		{
			line = us_pmcur.current->total - (y - us_pmcur.posy) / us_pmcur.txthei - 1;
			us_pmcur.inarea = 1;
		}
		if (line >= us_pmcur.current->total || line < 0) line = -1; else
		{
			pm = &us_pmcur.current->list[line];
			if (*pm->attribute == 0 && pm->value == 0) line = -1;
		}
		if (line != us_pmcur.hline)
		{
			if (us_pmcur.hline != -1)
			{
				us_pminvertline();
				us_pmcur.hline = -1;
			}
			us_pmcur.hline = line;
			if (us_pmcur.hline == -1) us_pmcur.retval = NOPOPUPMENUITEM; else
			{
				us_pmcur.retval = &us_pmcur.current->list[us_pmcur.hline];
				us_pmcur.retmenu = us_pmcur.current;
				us_pminvertline();
				us_pmcur.curpos = -1;

				/* new menu entry: see if it is hierarchical */
				uc = us_pmcur.retval->response;
				if (uc != NOUSERCOM && uc->menu != NOPOPUPMENU)
				{
					savepmcur = us_pmcur;
					but = FALSE;
					i = 1;
					lx = us_pmcur.posx+us_pmcur.sizex;
					if (us_pmcur.posx > us_pmcur.popupwindow.usehx-lx)
					{
						i = 2;
						lx = us_pmcur.posx;
						if (us_pmcur.posx < us_pmcur.sizex)
						{
							i = us_pmcur.quitline;
							lx = x-2;
						}
					}
					cmenu = uc->menu;
					pm = us_popupmenu(&cmenu, &but, FALSE, lx,
						us_pmcur.posy+(us_pmcur.current->total-line)*us_pmcur.txthei, i);
					us_pmcur = savepmcur;
					us_pmcur.retval = pm;
					us_pmcur.retmenu = cmenu;
					if (!but) return(TRUE);
					readtablet(&sx, &sy);
					x = sx;   y = sy;
					continue;
				}
			}
		}
		break;
	}

	/* if exiting over quit line, stop */
	switch (us_pmcur.quitline)
	{
		case 1:			/* quit if off left side */
			if (y < shei)
			{
				if (x >= us_pmcur.posx) break;
				if (y <= maxy-us_pmcur.yshift+2 &&
					y >= maxy-us_pmcur.yshift-us_pmcur.txthei-2) break;
			}
			us_pmcur.inarea = -1;
			return(TRUE);
		case 2:			/* quit if off right side */
			if (y < shei)
			{
				if (x <= us_pmcur.posx+us_pmcur.sizex) break;
				if (y <= maxy-us_pmcur.yshift+2 &&
					y >= maxy-us_pmcur.yshift-us_pmcur.txthei-2) break;
			}
			us_pmcur.inarea = -1;
			return(TRUE);
		case 3:			/* quit if out of this pulldown menu's menubar location */
			if (y <= maxy) break;
			lx = x;
			for(i=0; i<us_pulldownmenucount; i++)
			{
				if (us_pmcur.current == us_pulldowns[i]) break;
				lx = us_pulldownmenupos[i];
			}
			if (x >= lx && (x <= us_pulldownmenupos[i] || i == us_pulldownmenucount-1))
				break;
			us_pmcur.inarea = -1;
			return(TRUE);
		case 4:			/* quit if out of this popup menu's location */
			if (x >= us_pmcur.posx && x <= us_pmcur.posx+us_pmcur.sizex &&
				y >= us_pmcur.posy && y <= us_pmcur.posy+us_pmcur.sizey) break;
			us_pmcur.inarea = -1;
			return(TRUE);
	}
	return(FALSE);
}

/*
 * routine to invert the text line at "us_pmcur.hline"
 */
void us_pminvertline(void)
{
	INTBIG yp;

	yp = us_pmcur.posy+us_pmcur.sizey-us_pmcur.txthei*(us_pmcur.hline+1)-2;
	if (us_pmcur.header) yp -= us_pmcur.txthei;
	screeninvertbox(&us_pmcur.popupwindow, us_pmcur.posx+1,
		us_pmcur.posx+us_pmcur.sizex-2, yp, yp+us_pmcur.txthei-1);
}
#endif /* USEQT */

/* 
 * Routine to remove the "&" from the string "msg" and return the new string.
 * This is used in menu entries, which use the "&" character to mark the
 * location of the mnemonic for the entry.
 */
CHAR *us_stripampersand(CHAR *msg)
{
	static CHAR buf[200];
	REGISTER CHAR *pt;

	pt = buf;
	while (*msg != 0)
	{
		if (*msg != '&') *pt++ = *msg;
		msg++;
	}
	*pt = 0;
	return(buf);
}

/*
 * routine to scan popupmenu "pm" for command-key equivalents and set them
 */
void us_scanforkeyequiv(POPUPMENU *pm)
{
	REGISTER POPUPMENUITEM *mi;
	REGISTER INTBIG j;

	for(j=0; j<pm->total; j++)
	{
		mi = &pm->list[j];
		if (mi->response == NOUSERCOM) continue;
		if (mi->response->active < 0) continue;
		if (mi->response->menu != NOPOPUPMENU)
		{
			us_scanforkeyequiv(mi->response->menu);
			continue;
		}
		us_setkeyequiv(pm, j+1);
	}
}

void us_setkeyequiv(POPUPMENU *pm, INTBIG i)
{
	REGISTER INTBIG len, special;
	REGISTER INTSML key;
	REGISTER CHAR *pt, lastchar;
	REGISTER POPUPMENUITEM *mi;
	REGISTER void *infstr;

	/* make sure it is a valid menu entry */
	mi = &pm->list[i-1];
	if (mi->response == NOUSERCOM) return;
	if (mi->response->active < 0) return;
	if (mi->response->menu != NOPOPUPMENU) return;

	/* remove ending "<" for checkmarks */
	len = estrlen(mi->attribute);
	lastchar = 0;
	if (mi->attribute[len-1] == '<')
	{
		lastchar = mi->attribute[len-1];
		mi->attribute[len-1] = 0;
	}

	/* see if it has a "/" or "\" in it */
	for(pt = mi->attribute; *pt != 0 && *pt != '/' && *pt != '\\'; pt++) ;
	if (*pt == 0) return;

	/* make it the Meta key */
	key = pt[1];
	if (key != 0)
	{
		if (*pt == '/')
		{
			special = ACCELERATORDOWN;
			key = tolower(key);
		} else special = 0;
		infstr = initinfstr();
		formatinfstr(infstr, x_("%c%c"), key, *pt);
		addstringtoinfstr(infstr, x_("command="));
		addstringtoinfstr(infstr, mi->response->comname);
		us_appendargs(infstr, mi->response);
		us_setkeybinding(returninfstr(infstr), key, special, FALSE);
	}

	/* restore ending "<" checkmark */
	len = estrlen(mi->attribute);
	mi->attribute[len] = lastchar;
}

/*
 * Routine to examine popup menu "pm" and ensure that no two letters have the same
 * "&"-invocation key.
 */
void us_validatemenu(POPUPMENU *pm)
{
	REGISTER INTBIG i, j, index;
	REGISTER POPUPMENUITEM *mi;
	REGISTER USERCOM *uc;
	REGISTER CHAR *pt;
	CHAR tagged[256];

	for(i=0; i<256; i++) tagged[i] = 0;
	for(j=0; j < pm->total; j++)
	{
		mi = &pm->list[j];
		uc = mi->response;
		if (uc->active < 0) continue;

		if (uc->menu != NOPOPUPMENU)
		{
			us_validatemenu(uc->menu);
		}
		for(pt = mi->attribute; *pt != 0; pt++) if (*pt == '&') break;
		if (*pt == 0) continue;
		index = tolower(pt[1]) & 0xFF;
		if (tagged[index] != 0)
		{
			ttyputerr(_("Warning: two entries share letter '%c' ('%s' and '%s')"),
				index, mi->attribute, pm->list[tagged[index]-1].attribute);
		}
		tagged[index] = (CHAR)(j+1);
	}
}
