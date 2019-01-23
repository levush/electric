/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrstatus.c
 * User interface tool: status display routines
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
#include "drc.h"
#include "network.h"
#include "efunction.h"
#include "edialogs.h"
#include "tecgen.h"
#include "tecschem.h"
#include "usrdiacom.h"

static INTBIG *us_layer_letter_data = 0;	/* cache for "us_layerletters" */
static BOOLEAN us_shadowarcdetails = FALSE;	/* true if the arc status field has details */

/* working memory for "us_uniqueportname()" */
static CHAR *us_uniqueretstr = NOSTRING;

/* working memory for "us_shadowarcproto()" */
static CHAR *us_lastnetworkname;
static INTBIG us_lastnetworknametotal = 0;

/* structure for "usrbits" information parsing */
struct explanation
{
	INTBIG bit;			/* bit mask in word */
	INTBIG shift;		/* shift of bit mask (-1 if a flag) */
	CHAR  *name;		/* name of the field */
};

/* prototypes for local routines */
static void us_addobjects(NODEPROTO*);
static BOOLEAN us_wildmatch(CHAR*, CHAR*);
static void us_explaintool(INTBIG, struct explanation[]);
static void us_drawarcmenu(ARCPROTO *ap);
static BOOLEAN us_isuniquename(CHAR *name, NODEPROTO *cell, INTBIG type, void *exclude);

static STATUSFIELD *us_clearstatuslines[3] = {0, 0, 0};

/*
 * Routine to free all memory associated with this module.
 */
void us_freestatusmemory(void)
{
	REGISTER INTBIG i, lines;

	if (us_layer_letter_data != 0) efree((CHAR *)us_layer_letter_data);
	if (us_statusalign != 0) ttyfreestatusfield(us_statusalign);
	if (us_statusangle != 0) ttyfreestatusfield(us_statusangle);
	if (us_statusarc != 0) ttyfreestatusfield(us_statusarc);
	if (us_statuscell != 0) ttyfreestatusfield(us_statuscell);
	if (us_statuscellsize != 0) ttyfreestatusfield(us_statuscellsize);
	if (us_statusselectcount != 0) ttyfreestatusfield(us_statusselectcount);
	if (us_statusgridsize != 0) ttyfreestatusfield(us_statusgridsize);
	if (us_statuslambda != 0) ttyfreestatusfield(us_statuslambda);
	if (us_statusnode != 0) ttyfreestatusfield(us_statusnode);
	if (us_statustechnology != 0) ttyfreestatusfield(us_statustechnology);
	if (us_statusxpos != 0) ttyfreestatusfield(us_statusxpos);
	if (us_statusypos != 0) ttyfreestatusfield(us_statusypos);
	if (us_statusproject != 0) ttyfreestatusfield(us_statusproject);
	if (us_statusroot != 0) ttyfreestatusfield(us_statusroot);
	if (us_statuspart != 0) ttyfreestatusfield(us_statuspart);
	if (us_statuspackage != 0) ttyfreestatusfield(us_statuspackage);
	if (us_statusselection != 0) ttyfreestatusfield(us_statusselection);

	lines = ttynumstatuslines();
	for(i=0; i<lines; i++) ttyfreestatusfield(us_clearstatuslines[i]);

	if (us_uniqueretstr != NOSTRING) efree(us_uniqueretstr);
	if (us_lastnetworknametotal > 0) efree((CHAR *)us_lastnetworkname);
}

/******************** STATUS DISPLAY MANIPULATION ********************/

/* routine to determine the state of the status fields */
void us_initstatus(void)
{
	REGISTER INTBIG lines, i;
	static BOOLEAN first = TRUE;

	if (first)
	{
		first = FALSE;
		lines = ttynumstatuslines();
		for(i=0; i<lines; i++) us_clearstatuslines[i] = ttydeclarestatusfield(i+1, 0, 100, x_(""));
		us_statuscell = us_statusnode = us_statusarc = 0;
		us_statuslambda = us_statustechnology = us_statusxpos = us_statusypos = 0;
		us_statusangle = us_statusalign = us_statuscellsize = us_statusgridsize = 0;
		us_statusproject = us_statusroot = us_statuspart = us_statuspackage = 0;
		us_statusselection = us_statusselectcount = 0;
		us_statuscell        = ttydeclarestatusfield(0,  0,   0, x_(""));
		us_statusnode        = ttydeclarestatusfield(1,  0,  20, _("NODE: "));
		us_statusarc         = ttydeclarestatusfield(1, 20,  40, _("ARC: "));
		us_statusselectcount = ttydeclarestatusfield(1, 40,  52, _("SELECTED: "));
		us_statuscellsize    = ttydeclarestatusfield(1, 52,  69, _("SIZE: "));
		us_statuslambda      = ttydeclarestatusfield(1, 69,  82, _("LAMBDA: "));
		us_statustechnology  = ttydeclarestatusfield(1, 82, 100, _("TECHNOLOGY: "));
		us_statusxpos        = ttydeclarestatusfield(1, 69,  82, _("X: "));
		us_statusypos        = ttydeclarestatusfield(1, 82, 100, _("Y: "));
		if (lines > 1)
		{
			us_statusangle    = ttydeclarestatusfield(2,  0,  25, _("ANGLE: "));
			us_statusalign    = ttydeclarestatusfield(2, 25,  50, _("ALIGN: "));
			us_statusgridsize = ttydeclarestatusfield(2, 75, 100, _("GRID: "));
		}
	}
}

/*
 * Routine to create a field in the status display.  "line" is the status line number (line 0
 * is the window header).  "startper" and "endper" are the starting and ending percentages
 * of the line to use (ignored for the window header).  "label" is the prefix to use.
 * Returns an object to use with subsequent calls to "ttysetstatusfield" (zero on error).
 */
STATUSFIELD *ttydeclarestatusfield(INTBIG line, INTBIG startper, INTBIG endper, CHAR *label)
{
	STATUSFIELD *sf;

	sf = (STATUSFIELD *)emalloc((sizeof (STATUSFIELD)), us_tool->cluster);
	if (sf == 0) return(0);
	if (line < 0 || line > ttynumstatuslines()) return(0);
	if (line > 0 && (startper < 0 || endper > 100 || startper >= endper)) return(0);
	sf->line = line;
	sf->startper = startper;
	sf->endper = endper;
	(void)allocstring(&sf->label, label, us_tool->cluster);
#ifdef USEQT
	ttysetstatusfield(NOWINDOWFRAME, sf, x_(""), FALSE);
#endif
	return(sf);
}

/* write the current grid alignment from "us_alignment_ratio" */
void us_setalignment(WINDOWFRAME *frame)
{
	CHAR valstring[60];

	(void)estrcpy(valstring, frtoa(us_alignment_ratio));
	if (us_edgealignment_ratio != 0)
	{
		(void)estrcat(valstring, x_("/"));
		(void)estrcat(valstring, frtoa(us_edgealignment_ratio));
	}
	ttysetstatusfield(frame, us_statusalign, valstring, FALSE);
}

/* write the current lambda value from the current library/technology in microns */
void us_setlambda(WINDOWFRAME *frame)
{
	REGISTER INTBIG len;
	float lambdainmicrons;
	CHAR hold[50];

	/* show nothing in X/Y mode */
	if ((us_tool->toolstate&SHOWXY) != 0) return;

	lambdainmicrons = scaletodispunit(el_curlib->lambda[el_curtech->techindex], DISPUNITMIC);
	(void)esnprintf(hold, 50, x_("%g"), lambdainmicrons);
	for(;;)
	{
		len = estrlen(hold)-1;
		if (hold[len] != '0') break;
		hold[len] = 0;
	}
	if (hold[len] == '.') hold[len] = 0;
	(void)estrcat(hold, x_("u"));
	ttysetstatusfield(NOWINDOWFRAME, us_statuslambda, hold, FALSE);
}

/* write the name of the current technology from "el_curtech->techname" */
void us_settechname(WINDOWFRAME *frame)
{
	/* show nothing in XY mode */
	if ((us_tool->toolstate&SHOWXY) != 0) return;

	ttysetstatusfield(NOWINDOWFRAME, us_statustechnology, el_curtech->techname, TRUE);
}

/* write the cursor coordinates */
void us_setcursorpos(WINDOWFRAME *frame, INTBIG x, INTBIG y)
{
	static INTBIG lastx=0, lasty=0;

	/* show nothing in non XY mode */
	if ((us_tool->toolstate&SHOWXY) == 0) return;

	if (lastx != x)
	{
		ttysetstatusfield(frame, us_statusxpos, latoa(x, 0), FALSE);
		lastx = x;
	}

	if (lasty != y)
	{
		ttysetstatusfield(frame, us_statusypos, latoa(y, 0), FALSE);
		lasty = y;
	}
}

/* set the current nodeproto */
void us_setnodeproto(NODEPROTO *np)
{
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_node_key, (INTBIG)np, VNODEPROTO|VDONTSAVE);
}

/*
 * set the current arcproto to "ap".  If "always" is false, do this only if sensible.
 */
void us_setarcproto(ARCPROTO *ap, BOOLEAN always)
{
	/* don't do it if setting universal arc when not in generic technology */
	if (!always && ap != NOARCPROTO && ap->tech != el_curtech)
	{
		if (ap == gen_universalarc) return;
	}
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_arc_key, (INTBIG)ap, VARCPROTO|VDONTSAVE);
}

/* shadow changes to the current nodeproto */
void us_shadownodeproto(WINDOWFRAME *frame, NODEPROTO *np)
{
	REGISTER CHAR *pt;
	HIGHLIGHT high;
	REGISTER VARIABLE *var;
	REGISTER INTBIG len;
	REGISTER NODEINST *ni;
	REGISTER void *infstr;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	us_curnodeproto = np;

	if (np == NONODEPROTO) pt = x_(""); else
		pt = describenodeproto(np);

	/* see if the selection further qualifies the node description */
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var != NOVARIABLE)
	{
		len = getlength(var);
		if (len == 1)
		{
			if (!us_makehighlight(((CHAR **)var->addr)[0], &high))
			{
				if ((high.status&HIGHTEXT) != 0)
				{
					infstr = initinfstr();
					var = high.fromvar;
					if (var == NOVARIABLE)
					{
						if (high.fromport == NOPORTPROTO)
						{
							formatinfstr(infstr, _("%s (cell name)"),
								describenodeinst(high.fromgeom->entryaddr.ni));
						} else
						{
							formatinfstr(infstr, _("%s (export)"), high.fromport->protoname);
						}
					} else
					{
						if (high.fromgeom == NOGEOM)
						{
							formatinfstr(infstr, _("%s on cell"),
								us_trueattributename(var));
						} else if (high.fromport != NOPORTPROTO)
						{
							formatinfstr(infstr, _("%s on export"),
								us_trueattributename(var));
						} else
						{
							if (high.fromgeom->entryisnode)
							{
								ni = high.fromgeom->entryaddr.ni;
								if (ni->proto == gen_invispinprim)
								{
									addstringtoinfstr(infstr, us_invisiblepintextname(var));
								} else
								{
									formatinfstr(infstr, _("%s on node"), us_trueattributename(var));
								}
							} else
							{
								formatinfstr(infstr, _("%s on arc"), us_trueattributename(var));
							}
						}
					}
					pt = returninfstr(infstr);
				} else if ((high.status&HIGHFROM) != 0 && high.fromgeom->entryisnode)
				{
					ni = high.fromgeom->entryaddr.ni;
					if (ni->proto == np)
					{
						/* one node selected that is the same as the current proto: qualify proto */
						infstr = initinfstr();
						addstringtoinfstr(infstr, us_describenodeinsttype(np, ni, ni->userbits&NTECHBITS));
						var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
						if (var != NOVARIABLE)
							formatinfstr(infstr, x_(" [%s]"), (CHAR *)var->addr);
						pt = returninfstr(infstr);
					}
				}
			}
		}
	}
	ttysetstatusfield(frame, us_statusnode, pt, TRUE);

	/* if a node was described with details, clear them */
	if (us_shadowarcdetails) us_shadowarcproto(frame, us_curarcproto);

	/* highlight the menu entry */
	us_showcurrentnodeproto();
}

/*
 * routine to draw highlighting around the menu entry with the current
 * node prototype
 */
void us_showcurrentnodeproto(void)
{
	REGISTER CHAR *str;
	REGISTER INTBIG x, y;
	REGISTER VARIABLE *var;
	REGISTER NODEPROTO *np;
	COMMANDBINDING commandbinding;

	if (us_curnodeproto == NONODEPROTO) return;
	if ((us_tool->toolstate&MENUON) == 0) return;
	if ((us_tool->toolstate&ONESHOTMENU) != 0) return;
	if (us_menuhnx >= 0) us_highlightmenu(us_menuhnx, us_menuhny, el_colmenbor);
	us_menuhnx = -1;
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_menu_key);
	if (var != NOVARIABLE) for(x=0; x<us_menux; x++) for(y=0; y<us_menuy; y++)
	{
		if (us_menupos <= 1) str = ((CHAR **)var->addr)[y * us_menux + x]; else
			str = ((CHAR **)var->addr)[x * us_menuy + y];
		us_parsebinding(str, &commandbinding);
		np = commandbinding.nodeglyph;
		us_freebindingparse(&commandbinding);
		if (np == us_curnodeproto)
		{
			us_menuhnx = x;   us_menuhny = y;
			us_highlightmenu(us_menuhnx, us_menuhny, el_colhmenbor);
			break;
		}
	}
}

/* shadow changes to the current arcproto */
void us_shadowarcproto(WINDOWFRAME *frame, ARCPROTO *ap)
{
	REGISTER ARCPROTO *oldap;
	REGISTER NETWORK *net, **netlist;
	REGISTER CHAR *pt;
	REGISTER INTBIG len;
	REGISTER void *infstr;

	/* first show the arc/network in the status bar */
	netlist = net_gethighlightednets(FALSE);
	net = netlist[0];
	if (net == NONETWORK || (net->globalnet < 0 && net->namecount <= 0))
	{
		pt = x_("");
		us_shadowarcdetails = FALSE;
	} else
	{
		pt = describenetwork(net);
		us_shadowarcdetails = TRUE;
	}
	if (ap != us_curarcproto || us_lastnetworkname == 0 || namesame(us_lastnetworkname, pt) != 0)
	{
		len = estrlen(pt) + 1;
		if (len > us_lastnetworknametotal)
		{
			if (us_lastnetworknametotal > 0) efree((CHAR *)us_lastnetworkname);
			us_lastnetworknametotal = 0;
			us_lastnetworkname = (CHAR *)emalloc(len * SIZEOFCHAR, us_tool->cluster);
			if (us_lastnetworkname == 0) return;
			us_lastnetworknametotal = len;
		}
		estrcpy(us_lastnetworkname, pt);
		pt = describearcproto(ap);
		if (*us_lastnetworkname != 0)
		{
			infstr = initinfstr();
			formatinfstr(infstr, x_("%s [%s]"), pt, us_lastnetworkname);
			pt = returninfstr(infstr);
		}

		ttysetstatusfield(frame, us_statusarc, pt, TRUE);
	}

	/* next handle menu highlighting */
	if (us_curarcproto != ap)
	{
		/* undraw the menu entry with the last arc proto in it */
		oldap = us_curarcproto;
		us_curarcproto = ap;

		/* adjust the component menu entries */
		if (oldap != NOARCPROTO) us_drawarcmenu(oldap);
		if (us_curarcproto != NOARCPROTO) us_drawarcmenu(us_curarcproto);
		us_showcurrentarcproto();
	}
}

/*
 * Routine to find arcproto "ap" in the component menu and redraw that entry.
 */
void us_drawarcmenu(ARCPROTO *ap)
{
	REGISTER CHAR *str;
	REGISTER INTBIG x, y;
	REGISTER VARIABLE *var;
	REGISTER ARCPROTO *pap;
	COMMANDBINDING commandbinding;

	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_menu_key);
	if (var == NOVARIABLE) return;
	for(x=0; x<us_menux; x++)
		for(y=0; y<us_menuy; y++)
	{
		if (us_menupos <= 1) str = ((CHAR **)var->addr)[y * us_menux + x]; else
			str = ((CHAR **)var->addr)[x * us_menuy + y];
		us_parsebinding(str, &commandbinding);
		pap = commandbinding.arcglyph;
		us_freebindingparse(&commandbinding);
		if (pap == ap)
		{
			us_drawmenuentry(x, y, str);
			return;
		}
	}
}

/*
 * routine to draw highlighting around the menu entry with the current
 * arc prototype
 */
void us_showcurrentarcproto(void)
{
	REGISTER CHAR *str;
	REGISTER INTBIG x, y;
	REGISTER VARIABLE *var;
	REGISTER ARCPROTO *ap;
	COMMANDBINDING commandbinding;

	/* highlight the menu entry */
	if (us_curarcproto == NOARCPROTO) return;
	if ((us_tool->toolstate&MENUON) == 0) return;
	if ((us_tool->toolstate&ONESHOTMENU) != 0) return;
	if (us_menuhax >= 0) us_highlightmenu(us_menuhax, us_menuhay, el_colmenbor);
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_menu_key);
	if (var != NOVARIABLE) for(x=0; x<us_menux; x++) for(y=0; y<us_menuy; y++)
	{
		if (us_menupos <= 1) str = ((CHAR **)var->addr)[y * us_menux + x]; else
			str = ((CHAR **)var->addr)[x * us_menuy + y];
		us_parsebinding(str, &commandbinding);
		ap = commandbinding.arcglyph;
		us_freebindingparse(&commandbinding);
		if (ap == us_curarcproto)
		{
			us_menuhax = x;   us_menuhay = y;
			us_highlightmenu(us_menuhax, us_menuhay, el_colhmenbor);
			break;
		}
	}
}

/* write the default placement angle from "tool:user.USER_placement_angle" */
void us_setnodeangle(WINDOWFRAME *frame)
{
	CHAR hold[5];
	REGISTER VARIABLE *var;
	REGISTER INTBIG pangle;

	var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_placement_angle_key);
	if (var == NOVARIABLE) pangle = 0; else pangle = var->addr;
	(void)estrcpy(hold, frtoa(pangle%3600*WHOLE/10));
	if (pangle >= 3600) (void)estrcat(hold, x_("T"));
	ttysetstatusfield(frame, us_statusangle, hold, FALSE);
}

/* write the name of the cell in window "w" */
void us_setcellname(WINDOWPART *w)
{
	REGISTER EDITOR *ed;
	REGISTER void *infstr;

	/* do not show name of explorer window when it is split */
	if ((w->state&WINDOWTYPE) == EXPLORERWINDOW &&
		estrcmp(w->location, x_("entire")) != 0) return;

	/* if this is part of a frame and the current window is in another part, don't write name */
	if (el_curwindowpart != NOWINDOWPART && w != el_curwindowpart &&
		w->frame == el_curwindowpart->frame) return;

	/* write the cell name */
	infstr = initinfstr();
	switch (w->state&WINDOWTYPE)
	{
		case TEXTWINDOW:
		case POPTEXTWINDOW:
			if (w->curnodeproto == NONODEPROTO)
			{
				ed = w->editor;
				if (ed != NOEDITOR)
				{
					addstringtoinfstr(infstr, ed->header);
					break;
				}
			}
			/* FALLTHROUGH */ 
		case DISPWINDOW:
		case DISP3DWINDOW:
			if (w->curnodeproto != NONODEPROTO)
			{
				formatinfstr(infstr, x_("%s:%s"), w->curnodeproto->lib->libname,
					nldescribenodeproto(w->curnodeproto));
				if (w->curnodeproto->lib != el_curlib)
					formatinfstr(infstr, _(" - Current library: %s"), el_curlib->libname);
			}
			break;
		case EXPLORERWINDOW:
			addstringtoinfstr(infstr, _("Cell Explorer"));
			break;
		case WAVEFORMWINDOW:
			if (w->curnodeproto != NONODEPROTO)
			{
				formatinfstr(infstr, _("Waveform of %s"), describenodeproto(w->curnodeproto));
			} else
			{
				addstringtoinfstr(infstr, _("Waveform"));
			}
			break;
	}

	ttysetstatusfield(w->frame, us_statuscell, returninfstr(infstr), TRUE);
}

/*
 * Routine to update the number of selected objects.
 */
void us_setselectioncount(void)
{
	CHAR valstring[50];
	REGISTER VARIABLE *var;
	REGISTER WINDOWPART *w;

	valstring[0] = 0;
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var != NOVARIABLE)
		esnprintf(valstring, 50, x_("%ld"), getlength(var));

	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		if ((w->state&WINDOWTYPE) == DISPWINDOW)
			ttysetstatusfield(w->frame, us_statusselectcount, valstring, FALSE); else
				ttysetstatusfield(w->frame, us_statusselectcount, x_(""), FALSE);
	}
}

/*
 * write the size of the cell in window "w" from the size of "w->curnodeproto"
 */
void us_setcellsize(WINDOWPART *w)
{
	REGISTER NODEPROTO *np;
	CHAR valstring[50];
	REGISTER INTBIG lines, lambda;

	np = w->curnodeproto;
	valstring[0] = 0;
	if (np != NONODEPROTO)
	{
		switch (w->state&WINDOWTYPE)
		{
			case TEXTWINDOW:
			case POPTEXTWINDOW:
				lines = us_totallines(w);
				esnprintf(valstring, 50, _("%ld lines"), lines);
				break;
			case DISPWINDOW:
			case DISP3DWINDOW:
				lambda = lambdaofcell(np);
				(void)estrcpy(valstring, latoa(np->highx - np->lowx, lambda));
				(void)estrcat(valstring, x_("x"));
				(void)estrcat(valstring, latoa(np->highy - np->lowy, lambda));
				break;
		}
	}
	ttysetstatusfield(w->frame, us_statuscellsize, valstring, FALSE);
}

/* write the grid size of window "w" from "w->gridx" and "w->gridy" */
void us_setgridsize(WINDOWPART *w)
{
	REGISTER NODEPROTO *np;
	CHAR valstring[50];

	np = w->curnodeproto;
	valstring[0] = 0;
	if (np != NONODEPROTO)
	{
		switch (w->state&WINDOWTYPE)
		{
			case DISPWINDOW:
			case DISP3DWINDOW:
				(void)estrcpy(valstring, frtoa(w->gridx));
				(void)estrcat(valstring, x_("x"));
				(void)estrcat(valstring, frtoa(w->gridy));
				break;
		}
	}
	ttysetstatusfield(NOWINDOWFRAME, us_statusgridsize, valstring, FALSE);
}

/* routine to re-write the status information (on all frames if frame is 0) */
void us_redostatus(WINDOWFRAME *frame)
{
	REGISTER WINDOWPART *w;
	REGISTER ARCPROTO *ap;
	REGISTER NODEPROTO *np;
	REGISTER INTBIG lines, i;

	/* clear the status bar */
	lines = ttynumstatuslines();
	for(i=0; i<lines; i++) ttysetstatusfield(NOWINDOWFRAME, us_clearstatuslines[i], x_(""), FALSE);

	/* node and arc */
	ap = us_curarcproto;   np = us_curnodeproto;
	us_curarcproto = (ARCPROTO *)0;   us_curnodeproto = (NODEPROTO *)0;
	us_shadownodeproto(NOWINDOWFRAME, np);
	us_shadowarcproto(NOWINDOWFRAME, ap);

	/* write the window information on the window lines */
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		us_setcellname(w);
		if (graphicshas(CANSTATUSPERFRAME) || w->frame == frame)
			us_setcellsize(w);
		us_setgridsize(w);
	}
	if (el_curwindowpart != NOWINDOWPART)
		us_setcellsize(el_curwindowpart);

	/* other fields */
	us_setnodeangle(frame);
	us_setalignment(frame);
	us_setlambda(frame);
	us_settechname(frame);
	us_setselectioncount();
}

/******************** OUTPUT PREPARATION ********************/

/*
 * routine to list macros by the package of origin
 */
void us_printmacros(void)
{
	INTBIG len, coms, keyboard, gotany, i, j, count;
	CHAR line[150], *build[MAXPARS];
	REGISTER CHAR *name, *comname, *inter;
	REGISTER MACROPACK *pack, *thispack;
	REGISTER VARIABLE *var;

	/* count the number of macros in each package */
	for(pack=us_macropacktop; pack!=NOMACROPACK; pack=pack->nextmacropack)
		pack->total = 0;
	keyboard = gotany = 0;
	for(i=0; i<us_tool->numvar; i++)
	{
		if (namesamen(makename(us_tool->firstvar[i].key), x_("USER_macro_"), 11) == 0)
		{
			var = &us_tool->firstvar[i];
			if (getlength(var) <= 3) continue;
			count = us_parsecommand(((CHAR **)var->addr)[0], build);
			for(j=0; j<count; j++)
				if (namesamen(build[j], x_("macropack="), 10) == 0)
			{
				pack = us_getmacropack(&build[j][10]);
				if (pack != NOMACROPACK) pack->total++;
				break;
			}
			if (j >= count) keyboard++;
			gotany++;
		}
	}
	if (gotany == 0)
	{
		ttyputmsg(M_("No macros defined"));
		return;
	}

	ttyputmsg(M_("---------- Macros by Package ----------"));

	/* now print the macros by package */
	for(pack = us_macropacktop; ; pack = pack->nextmacropack)
	{
		if (pack == NOMACROPACK)
		{
			if (keyboard == 0) break;
			name = M_("Keyboard-defined");
		} else
		{
			if (pack->total == 0) continue;
			name = pack->packname;
		}
		(void)estrcpy(line, name);
		(void)estrcat(line, M_(" macros:"));
		len = estrlen(line);
		inter = x_("");
		for(i=0; i<us_tool->numvar; i++)
			if (namesamen(makename(us_tool->firstvar[i].key), x_("USER_macro_"), 11) == 0)
		{
			var = &us_tool->firstvar[i];
			if (getlength(var) <= 3) continue;
			count = us_parsecommand(((CHAR **)var->addr)[0], build);
			thispack = NOMACROPACK;
			for(j=0; j<count; j++)
				if (namesamen(build[j], x_("macropack="), 10) == 0)
			{
				thispack = us_getmacropack(&build[j][10]);
				break;
			}
			if (thispack != pack) continue;
			(void)estrcat(line, inter);  len += estrlen(inter);
			inter = x_(",");
			comname = &makename(var->key)[11];
			coms = estrlen(comname) + 2;
			if (len + coms >= MESSAGESWIDTH)
			{
				ttyputmsg(line);  (void)estrcpy(line, x_("  "));  len = 2;
			}
			(void)estrcat(line, x_(" "));
			(void)estrcat(line, comname);  len += coms;

		}
		if (len > 2) ttyputmsg(line);
		if (pack == NOMACROPACK) break;
	}
}

/*
 * print macro "mac".  Show every command in the macro.
 */
void us_printmacro(VARIABLE *mac)
{
	REGISTER INTBIG j, len;

	if (mac == NOVARIABLE) return;
	len = getlength(mac);
	if (len <= 3)
	{
		ttyputmsg(M_("Macro '%s' is not defined"), &makename(mac->key)[11]);
		return;
	}

	ttyputmsg(M_("Macro '%s' has %ld %s"), &makename(mac->key)[11], len-3,
		makeplural(M_("command"), len-3));
	for(j=3; j<len; j++) ttyputmsg(x_("   %s"), ((CHAR **)mac->addr)[j]);
}

static struct
{
	CHAR *typestring;
	INTBIG typecode;
} us_vartypename[] =
{
	{x_("Unknown"),      VUNKNOWN},
	{x_("Integer"),      VINTEGER},
	{x_("Address"),      VADDRESS},
	{x_("Character"),    VCHAR},
	{x_("String"),       VSTRING},
	{x_("Float"),        VFLOAT},
	{x_("Double"),       VDOUBLE},
	{x_("Nodeinst"),     VNODEINST},
	{x_("Nodeproto"),    VNODEPROTO},
	{x_("Portarcinst"),  VPORTARCINST},
	{x_("Portexpinst"),  VPORTEXPINST},
	{x_("Portproto"),    VPORTPROTO},
	{x_("Arcinst"),      VARCINST},
	{x_("Arcproto"),     VARCPROTO},
	{x_("Geometry"),     VGEOM},
	{x_("Library"),      VLIBRARY},
	{x_("Technology"),   VTECHNOLOGY},
	{x_("Tool"),         VTOOL},
	{x_("R-tree"),       VRTNODE},
	{x_("Fixed-Point"),  VFRACT},
	{x_("Network"),      VNETWORK},
	{x_("View"),         VVIEW},
	{x_("WindowPart"),   VWINDOWPART},
	{x_("Graphics"),     VGRAPHICS},
	{x_("Short"),        VSHORT},
	{x_("Boolean"),      VBOOLEAN},
	{x_("Constraint"),   VCONSTRAINT},
	{x_("General"),      VGENERAL},
	{x_("Window-Frame"), VWINDOWFRAME},
	{x_("Polygon"),      VPOLYGON},
	{NULL, 0}
};

CHAR *us_variabletypename(INTBIG type)
{
	INTBIG i;

	for(i=0; us_vartypename[i].typestring != 0; i++)
		if (us_vartypename[i].typecode == (type&VTYPE))
			return(us_vartypename[i].typestring);
	return(x_("unknown"));
}

INTBIG us_variabletypevalue(CHAR *name)
{
	INTBIG i;

	for(i=0; us_vartypename[i].typestring != 0; i++)
		if (namesame(name, us_vartypename[i].typestring) == 0)
			return(us_vartypename[i].typecode);
	return(0);
}

/*
 * routine to describe variable "var".  If "index" is nonnegative then describe
 * entry "index" of an array variable.  The description is returned.
 */
CHAR *us_variableattributes(VARIABLE *var, INTBIG aindex)
{
	CHAR *arr, line[30];
	REGISTER INTBIG len;
	REGISTER void *infstr;

	if ((var->type & VISARRAY) == 0 || aindex >= 0) arr = x_(""); else
	{
		len = getlength(var);
		if (len == 0) arr = x_(" array[]"); else
		{
			if ((var->type&VTYPE) == VGENERAL) len /= 2;
			(void)esnprintf(line, 30, x_(" array[%ld]"), len);
			arr = line;
		}
	}
	infstr = initinfstr();
	if ((var->type&VDONTSAVE) != 0) addstringtoinfstr(infstr, _("Temporary "));
	if ((var->type&VDISPLAY) != 0)
	{
		addstringtoinfstr(infstr, _("Displayable("));
		addstringtoinfstr(infstr, us_describetextdescript(var->textdescript));
		addstringtoinfstr(infstr, x_(") "));
	}
	if ((var->type&(VCODE1|VCODE2)) != 0)
	{
		switch (var->type&(VCODE1|VCODE2))
		{
			case VLISP: addstringtoinfstr(infstr, x_("LISP "));  break;
			case VTCL:  addstringtoinfstr(infstr, x_("TCL "));   break;
			case VJAVA: addstringtoinfstr(infstr, x_("Java "));  break;
		}
	}
	addstringtoinfstr(infstr, us_variabletypename(var->type));
	addstringtoinfstr(infstr, arr);
	return(returninfstr(infstr));
}

/* routine to recursively count and print the nodes in cell "cell" */
void us_describecontents(NODEPROTO *cell)
{
	REGISTER INTBIG i;
	REGISTER NODEPROTO *np;
	REGISTER LIBRARY *lib, *printlib;
	REGISTER TECHNOLOGY *curtech, *printtech, *tech;
	REGISTER NODEPROTO **sortedcells;

	/* first zero the count of each nodeproto */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = 0;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = 0;

	/* now look at every object recursively in this cell */
	us_addobjects(cell);

	/* print the totals */
	ttyputmsg(_("Contents of cell %s:"), describenodeproto(cell));
	printtech = NOTECHNOLOGY;
	for(curtech = el_technologies; curtech != NOTECHNOLOGY; curtech = curtech->nexttechnology)
	{
		for(np = curtech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			if (np->temp1 != 0)
		{
			if (curtech != printtech)
			{
				ttyputmsg(_("%s technology:"), curtech->techname);
				printtech = curtech;
			}
			ttyputmsg(_("%6d %s %s"), np->temp1, describenodeproto(np),
				makeplural(_("node"), np->temp1));
		}
	}
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		sortedcells = us_sortlib(lib, (CHAR *)0);
		if (sortedcells == 0) ttyputnomemory(); else
		{
			printlib = NOLIBRARY;
			for(i = 0; sortedcells[i] != NONODEPROTO; i++)
			{
				np = sortedcells[i];
				if (np->temp1 == 0) continue;
				if (lib != printlib)
				{
					ttyputmsg(_("%s library:"), lib->libname);
					printlib = lib;
				}
				ttyputmsg(_("%6d %s %s"), np->temp1, nldescribenodeproto(np),
					makeplural(_("node"), np->temp1));
			}
			efree((CHAR *)sortedcells);
		}
	}
}

/*
 * routine to recursively examine cell "np" and update the number of
 * instantiated primitive nodeprotos in the "temp1" field of the nodeprotos.
 */
void us_addobjects(NODEPROTO *np)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *cnp;

	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		ni->proto->temp1++;
		if (ni->proto->primindex != 0) continue;

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(ni->proto, np)) continue;
		cnp = contentsview(ni->proto);
		if (cnp == NONODEPROTO) cnp = ni->proto;
		us_addobjects(cnp);
	}
}

static struct
{
	NODEPROTO **prim;
	INTBIG      bits;
	CHAR       *statusmessage;
} us_specialstatus[] =
{
	{&sch_transistorprim, TRANNMOS,     x_("nMOS")},
	{&sch_transistorprim, TRANDMOS,     x_("dMOS")},
	{&sch_transistorprim, TRANPMOS,     x_("pMOS")},
	{&sch_transistorprim, TRANNPN,      x_("NPN")},
	{&sch_transistorprim, TRANPNP,      x_("PNP")},
	{&sch_transistorprim, TRANNJFET,    x_("NJFET")},
	{&sch_transistorprim, TRANPJFET,    x_("PJFET")},
	{&sch_transistorprim, TRANDMES,     x_("DMES")},
	{&sch_transistorprim, TRANEMES,     x_("EMES")},
	{&sch_transistor4prim,TRANNMOS,     x_("nMOS")},
	{&sch_transistor4prim,TRANDMOS,     x_("dMOS")},
	{&sch_transistor4prim,TRANPMOS,     x_("pMOS")},
	{&sch_transistor4prim,TRANNPN,      x_("NPN")},
	{&sch_transistor4prim,TRANPNP,      x_("PNP")},
	{&sch_transistor4prim,TRANNJFET,    x_("NJFET")},
	{&sch_transistor4prim,TRANPJFET,    x_("PJFET")},
	{&sch_transistor4prim,TRANDMES,     x_("DMES")},
	{&sch_transistor4prim,TRANEMES,     x_("EMES")},
	{&sch_twoportprim,    TWOPVCCS,     x_("VCCS")},
	{&sch_twoportprim,    TWOPCCVS,     x_("CCVS")},
	{&sch_twoportprim,    TWOPVCVS,     x_("VCVS")},
	{&sch_twoportprim,    TWOPCCCS,     x_("CCCS")},
	{&sch_twoportprim,    TWOPTLINE,    x_("Transmission")},
	{&sch_diodeprim,      DIODEZENER,   x_("Zener")},
	{&sch_capacitorprim,  CAPACELEC,    x_("Electrolytic")},
	{&sch_ffprim,         FFTYPERS,     x_("RS")},
	{&sch_ffprim,         FFTYPEJK,     x_("JK")},
	{&sch_ffprim,         FFTYPED,      x_("D")},
	{&sch_ffprim,         FFTYPET,      x_("T")},
	{0, 0, 0}
};

/*
 * Routine to describe the node in the commandbinding structure "commandbinding".
 */
CHAR *us_describemenunode(COMMANDBINDING *commandbinding)
{
	REGISTER INTBIG bits;

	if (commandbinding->menumessage == 0) bits = 0; else
		bits = myatoi(commandbinding->menumessage);
	return(us_describenodeinsttype(commandbinding->nodeglyph, NONODEINST, bits));
}

/*
 * Routine to return a description of node "np" that is qualified with bits "bits".
 * It may also be qualified by having an instance "ni" (but it may be NONODEINST).
 */
CHAR *us_describenodeinsttype(NODEPROTO *np, NODEINST *ni, INTBIG bits)
{
	REGISTER INTBIG i;
	REGISTER CHAR *pt;
	REGISTER void *infstr;

	infstr = initinfstr();
	addstringtoinfstr(infstr, describenodeproto(np));
	for(i=0; us_specialstatus[i].prim != 0; i++)
		if (us_specialstatus[i].bits == bits &&
			*us_specialstatus[i].prim == np) break;
	if (us_specialstatus[i].prim != 0)
	{
		formatinfstr(infstr, x_(" (%s)"), us_specialstatus[i].statusmessage);
	} else
	{
		/* add information if from technology editor */
		if (ni != NONODEINST)
		{
			pt = us_teceddescribenode(ni);
			if (pt != 0) formatinfstr(infstr, x_(" (%s)"), pt);
		}
	}
	return(returninfstr(infstr));
}

/*
 * routine to sort the cells in library "lib" by cell name and return an
 * array of NODEPROTO pointers (terminating with NONODEPROTO).  If "matchspec"
 * is nonzero, it is a string that forces wildcard matching of cell names.
 */
NODEPROTO **us_sortlib(LIBRARY *lib, CHAR *matchspec)
{
	REGISTER NODEPROTO **sortindex, *np;
	REGISTER INTBIG total, i;
	REGISTER CHAR *pt;

	/* remove library name if it is part of spec */
	if (matchspec != 0)
	{
		for(pt = matchspec; *pt != 0; pt++) if (*pt == ':') break;
		if (*pt == ':') matchspec = pt+1;
	}

	/* count the number of cells in the library */
	total = 0;
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		if (us_wildmatch(nldescribenodeproto(np), matchspec)) total++;

	/* allocate array for sorting entries */
	sortindex = (NODEPROTO **)emalloc(((total+1) * (sizeof (NODEPROTO *))), el_tempcluster);
	if (sortindex == 0) return(0);
	for(np = lib->firstnodeproto, i = 0; np != NONODEPROTO; np = np->nextnodeproto)
		if (us_wildmatch(nldescribenodeproto(np), matchspec))
			sortindex[i++] = np;
	sortindex[i] = NONODEPROTO;

	/* sort the cell names */
	esort(sortindex, total, sizeof (NODEPROTO *), sort_cellnameascending);
	return(sortindex);
}

/*
 * routine to match a name in "name" with a wildcard specification in "spec".
 * If "spec" is zero, all names are valid.  Returns true if the name matches.
 */
BOOLEAN us_wildmatch(CHAR *name, CHAR *spec)
{
	REGISTER CHAR *pt, *pt2;
	REGISTER CHAR c1, c2;

	if (spec == 0) return(TRUE);

	pt = name;
	pt2 = spec;
	for(;;)
	{
		if (*pt == 0 && *pt2 == 0) break;
		if (*pt != 0 && *pt2 == '?')
		{
			pt++;
			pt2++;
			continue;
		}
		if (*pt2 == '*')
		{
			for(;;)
			{
				if (us_wildmatch(pt, &pt2[1]) != 0) return(TRUE);
				if (*pt == 0) break;
				pt++;
			}
			return(FALSE);
		}
		if (isupper(*pt)) c1 = tolower(*pt); else c1 = *pt;
		if (isupper(*pt2)) c2 = tolower(*pt2); else c2 = *pt2;
		if (c1 != c2) return(FALSE);
		pt++;
		pt2++;
	}
	return(TRUE);
}

/* routine to print the tool information about nodeinst "ni" */
void us_printnodetoolinfo(NODEINST *ni)
{
	static struct explanation explain[] =
	{
		{DEADN,       -1,      N_("dead")},
		{NEXPAND,     -1,      N_("expand")},
		{WIPED,       -1,      N_("wiped")},
		{NSHORT,      -1,      N_("short")},
		{0, -1, NULL}
	};
	us_explaintool(ni->userbits, explain);
}

/* routine to print the tool information about arcinst "ai" */
void us_printarctoolinfo(ARCINST *ai)
{
	static struct explanation explain[] =
	{
		{FIXED,         -1,                 N_("rigid")},
		{CANTSLIDE,     -1,                 N_("rigid-in-ports")},
		{FIXANG,        -1,                 N_("fixed-angle")},
		{DEADA,         -1,                 N_("dead")},
		{AANGLE,        AANGLESH,           N_("angle")},
		{ASHORT,        -1,                 N_("short")},
		{AFUNCTION,     APBUS<<AFUNCTIONSH, N_("bus")},
		{NOEXTEND,      -1,                 N_("ends-shortened")},
		{ISNEGATED,     -1,                 N_("negated")},
		{ISDIRECTIONAL, -1,                 N_("directional")},
		{NOTEND0,       -1,                 N_("skip-tail")},
		{NOTEND1,       -1,                 N_("skip-head")},
		{REVERSEEND,    -1,                 N_("direction-reversed")},
		{0, -1, NULL}
	};
	us_explaintool(ai->userbits, explain);
}

/* helper routine for "us_printnodetoolinfo" and "us_printarctoolinfo" */
void us_explaintool(INTBIG bits, struct explanation explain[])
{
	REGISTER INTBIG j, k;
	CHAR partial[40];
	REGISTER void *infstr;

	/* print the tool bit information */
	infstr = initinfstr();
	(void)esnprintf(partial, 40, _("User state = %ld"), bits);
	addstringtoinfstr(infstr, partial);
	k = 0;
	for(j=0; explain[j].bit != 0; j++)
	{
		if (explain[j].shift != -1)
		{
			if (k == 0) addtoinfstr(infstr, '('); else
				addstringtoinfstr(infstr, x_(", "));
			addstringtoinfstr(infstr, TRANSLATE(explain[j].name));
			addtoinfstr(infstr, '=');
			(void)esnprintf(partial, 40, x_("%ld"),(bits&explain[j].bit)>>explain[j].shift);
			addstringtoinfstr(infstr, partial);
			k++;
			continue;
		}
		if ((bits & explain[j].bit) == 0) continue;
		if (k == 0) addtoinfstr(infstr, '('); else addstringtoinfstr(infstr, x_(", "));
		addstringtoinfstr(infstr, TRANSLATE(explain[j].name));
		k++;
	}
	if (k != 0) addtoinfstr(infstr, ')');
	ttyputmsg(x_("%s"), returninfstr(infstr));
}

/*
 * routine to print information about port prototype "pp" on nodeinst "ni".
 * If the port prototype's "temp1" is nonzero, this port has already been
 * mentioned and should not be done again.
 */
void us_chatportproto(NODEINST *ni, PORTPROTO *pp)
{
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER ARCINST *ai;
	REGISTER INTBIG i, lambda;
	INTBIG lx, hx, ly, hy;
	REGISTER CHAR *activity;
	CHAR locx[40], locy[40];
	static POLYGON *poly = NOPOLYGON;
	REGISTER void *infstr;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* if this port has already been discussed, quit now */
	if (pp->temp1 != 0) return;
	pp->temp1++;

	/* talk about the port prototype */
	lambda = lambdaofnode(ni);
	infstr = initinfstr();
	activity = describeportbits(pp->userbits);
	if (*activity != 0)
	{
		formatinfstr(infstr, _("%s port %s connects to "), activity, pp->protoname);
	} else
	{
		formatinfstr(infstr, _("Port %s connects to "), pp->protoname);
	}
	for(i=0; pp->connects[i] != NOARCPROTO; i++)
	{
		if (i != 0) addstringtoinfstr(infstr, x_(", "));
		addstringtoinfstr(infstr, pp->connects[i]->protoname);
	}
	ttyputmsg(x_("%s"), returninfstr(infstr));

	if (pp->subnodeinst == NONODEINST)
	{
		shapeportpoly(ni, pp, poly, FALSE);

		/* see if the port is a single point */
		for(i=1; i<poly->count; i++)
			if (poly->xv[i] != poly->xv[i-1] || poly->yv[i] != poly->yv[i-1]) break;
		if (i >= poly->count)
		{
			ttyputmsg(_("  Located at (%s, %s)"), latoa(poly->xv[0], lambda), latoa(poly->yv[0], lambda));
		} else if (isbox(poly, &lx,&hx, &ly,&hy))
		{
			if (lx == hx) (void)esnprintf(locx, 40, _("at %s"), latoa(lx, lambda)); else
				(void)esnprintf(locx, 40, _("from %s to %s"), latoa(lx, lambda), latoa(hx, lambda));
			if (ly == hy) (void)esnprintf(locy, 40, _("at %s"), latoa(ly, lambda)); else
				(void)esnprintf(locy, 40, _("from %s to %s"), latoa(ly, lambda), latoa(hy, lambda));
			ttyputmsg(_("  Located %s in X and %s in Y"), locx, locy);
		} else
		{
			infstr = initinfstr();
			for(i=0; i<poly->count; i++)
				formatinfstr(infstr, x_(" (%s, %s)"), latoa(poly->xv[i], lambda), latoa(poly->yv[i], lambda));
			ttyputmsg(_("  Located inside polygon%s"), returninfstr(infstr));
		}
	} else ttyputmsg(_("  Located at subnode %s subport %s"),
		describenodeinst(pp->subnodeinst), pp->subportproto->protoname);

	/* talk about any arcs on this prototype */
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		if (pi->proto != pp) continue;
		ai = pi->conarcinst;
		if (ai->end[0].portarcinst == pi) i = 0; else i = 1;
		ttyputmsg(_("  Connected at (%s,%s) to %s"), latoa(ai->end[i].xpos, lambda),
			latoa(ai->end[i].ypos, lambda), describearcinst(ai));
	}

	/* talk about any exports of this prototype */
	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		if (pe->proto == pp)
	{
		infstr = initinfstr();
		formatinfstr(infstr, _("  Available as %s export '%s' (label %s"),
			describeportbits(pe->exportproto->userbits), pe->exportproto->protoname,
				us_describetextdescript(pe->exportproto->textdescript));
		if ((pe->exportproto->userbits&PORTDRAWN) != 0) addstringtoinfstr(infstr, _(",always-drawn"));
		if ((pe->exportproto->userbits&BODYONLY) != 0) addstringtoinfstr(infstr, _(",only-on-body"));
		addstringtoinfstr(infstr, x_(") "));
		ttyputmsg(x_("%s"), returninfstr(infstr));
	}
}

/*
 * routine to construct a string that describes the text descriptor field
 * in "descript".  The string has the form "POSITION+(DX,DY),SIZE" if there
 * is an offset, or "POSITION,SIZE" if there is not.  The string is returned.
 */
CHAR *us_describetextdescript(UINTBIG *descript)
{
	REGISTER INTBIG xdist, ydist;
	CHAR offset[50];
	REGISTER void *infstr;

	infstr = initinfstr();
	addstringtoinfstr(infstr, us_describestyle(TDGETPOS(descript)));
	if (TDGETXOFF(descript) != 0 || TDGETYOFF(descript) != 0)
	{
		xdist = TDGETXOFF(descript);
		ydist = TDGETYOFF(descript);
		(void)esnprintf(offset, 50, x_("+(%s,%s)"), frtoa(xdist*WHOLE/4), frtoa(ydist*WHOLE/4));
		addstringtoinfstr(infstr, offset);
	}
	addtoinfstr(infstr, ',');
	addstringtoinfstr(infstr, us_describefont(TDGETSIZE(descript)));
	return(returninfstr(infstr));
}

/*
 * routine to return a string describing text style "style".
 */
CHAR *us_describestyle(INTBIG style)
{
	switch (style)
	{
		case VTPOSCENT:      return(_("centered"));
		case VTPOSBOXED:     return(_("boxed"));
		case VTPOSUP:        return(_("up"));
		case VTPOSDOWN:      return(_("down"));
		case VTPOSLEFT:      return(_("left"));
		case VTPOSRIGHT:     return(_("right"));
		case VTPOSUPLEFT:    return(_("up-left"));
		case VTPOSUPRIGHT:   return(_("up-right"));
		case VTPOSDOWNLEFT:  return(_("down-left"));
		case VTPOSDOWNRIGHT: return(_("down-right"));
	}
	return(x_("?"));
}

/*
 * routine to return a string describing text font "font".
 */
CHAR *us_describefont(INTBIG font)
{
	static CHAR description[60];
	if (TXTGETPOINTS(font) != 0)
	{
		esnprintf(description, 60, _("%ld-points"), TXTGETPOINTS(font));
		return(description);
	}
	if (TXTGETQLAMBDA(font) != 0)
	{
		esnprintf(description, 60, _("%s-lambda"), frtoa(TXTGETQLAMBDA(font) * WHOLE / 4));
		return(description);
	}
	return(x_("?"));
}

/*
 * Routine to return the type of nonlayout text that is in variable "var".
 */
CHAR *us_invisiblepintextname(VARIABLE *var)
{
	if (namesame(makename(var->key), x_("VERILOG_code")) == 0)
		return(_("Verilog code"));
	if (namesame(makename(var->key), x_("VERILOG_declaration")) == 0)
		return(_("Verilog declaration"));
	if (namesame(makename(var->key), x_("SIM_spice_card")) == 0)
		return(_("SPICE card"));
	return(_("Nonlayout Text"));
}

typedef struct
{
	CHAR *origvarname;
	CHAR *uservarname;
} USERVARIABLES;

USERVARIABLES us_uservariables[] =
{
	{x_("ARC_name"),                 N_("Arc Name")},
	{x_("ARC_radius"),               N_("Arc Radius")},
	{x_("ART_color"),                N_("Color")},
	{x_("ART_degrees"),              N_("Number of Degrees")},
	{x_("ART_message"),              N_("Text")},
	{x_("NET_ncc_match"),            N_("NCC equivalence")},
	{x_("NET_ncc_forcedassociation"),N_("NCC association")},
	{x_("NODE_name"),                N_("Node Name")},
	{x_("SCHEM_capacitance"),        N_("Capacitance")},
	{x_("SCHEM_diode"),              N_("Diode Size")},
	{x_("SCHEM_inductance"),         N_("Inductance")},
	{x_("SCHEM_resistance"),         N_("Resistance")},
	{x_("SIM_fall_delay"),           N_("Fall Delay")},
	{x_("SIM_fasthenry_group_name"), N_("FastHenry Group")},
	{x_("SIM_rise_delay"),           N_("Rise Delay")},
	{x_("SIM_spice_model"),          N_("SPICE model")},
	{x_("transistor_width"),         N_("Transistor Width")},
	{x_("SCHEM_global_name"),        N_("Global Signal Name")},
	{0, 0}
};

/*
 * Routine to see if variable name "name" is a known variable.  If
 * so, returns a better name for it.  If not, returns zero.
 */
CHAR *us_bettervariablename(CHAR *name)
{
	REGISTER INTBIG i;

	/* handle standard variable names */
	for(i=0; us_uservariables[i].origvarname != 0; i++)
	{
		if (namesame(name, us_uservariables[i].origvarname) == 0)
			return(TRANSLATE(us_uservariables[i].uservarname));
	}
	return(0);
}

/*
 * Routine to see if variable "var" should have a units qualifier after it.
 * If so, returns the type.  If not, returns zero.
 */
CHAR *us_variableunits(VARIABLE *var)
{
	REGISTER INTBIG units;

	/* specialized fields that adjust with units */
	units = TDGETUNITS(var->textdescript);
	switch (units)
	{
		case VTUNITSRES:		/* resistance */
			return(TRANSLATE(us_resistancenames[(us_electricalunits&INTERNALRESUNITS) >> INTERNALRESUNITSSH]));
		case VTUNITSCAP:		/* capacitance */
			return(TRANSLATE(us_capacitancenames[(us_electricalunits&INTERNALCAPUNITS) >> INTERNALCAPUNITSSH]));
		case VTUNITSIND:		/* inductance */
			return(TRANSLATE(us_inductancenames[(us_electricalunits&INTERNALINDUNITS) >> INTERNALINDUNITSSH]));
		case VTUNITSCUR:		/* current */
			return(TRANSLATE(us_currentnames[(us_electricalunits&INTERNALCURUNITS) >> INTERNALCURUNITSSH]));
		case VTUNITSVOLT:		/* voltage */
			return(TRANSLATE(us_voltagenames[(us_electricalunits&INTERNALVOLTUNITS) >> INTERNALVOLTUNITSSH]));
		case VTUNITSTIME:		/* time */
			return(TRANSLATE(us_timenames[(us_electricalunits&INTERNALTIMEUNITS) >> INTERNALTIMEUNITSSH]));
	}
	return(0);
}

/*
 * Routine to return a user-friendly name of variable "var". 
 */
CHAR *us_trueattributename(VARIABLE *var)
{
	REGISTER CHAR *pt, *bettername, *unitname;
	REGISTER void *infstr;

	infstr = initinfstr();
	pt = makename(var->key);
	if (namesamen(pt, x_("ATTR_"), 5) == 0)
	{
		if (TDGETISPARAM(var->textdescript) != 0)
			formatinfstr(infstr, _("Parameter '%s'"), &pt[5]); else
				formatinfstr(infstr, _("Attribute '%s'"), &pt[5]);
	} else
	{
		bettername = us_bettervariablename(pt);
		if (bettername != 0) addstringtoinfstr(infstr, bettername); else
			formatinfstr(infstr, _("Variable '%s'"), pt);
	}
	unitname = us_variableunits(var);
	if (unitname != 0) formatinfstr(infstr, x_(" (%s)"), unitname);
	return(returninfstr(infstr));
}

/*
 * routine to print a description of color map entry "ind"
 */
void us_printcolorvalue(INTBIG ind)
{
	GRAPHICS *desc;
	CHAR line[40];
	REGISTER INTBIG i, j, high;
	REGISTER VARIABLE *varred, *vargreen, *varblue;
	REGISTER void *infstr;

	/* get color arrays */
	varred = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_red_key);
	vargreen = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_green_key);
	varblue = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_blue_key);
	if (varred == NOVARIABLE || vargreen == NOVARIABLE || varblue == NOVARIABLE)
	{
		ttyputerr(_("Cannot get current color map"));
		return;
	}

	infstr = initinfstr();
	if ((ind&(LAYERH|LAYERG)) == 0)
	{
		(void)esnprintf(line, 40, _("Entry %ld"), ind);
		addstringtoinfstr(infstr, line);
		j = 0;
		high = el_curtech->layercount;
		for(i=0; i<high; i++)
		{
			desc = el_curtech->layers[i];
			if (desc->bits == LAYERO && desc->col != ind) continue;
			if (desc->bits != LAYERO && ((desc->col&ind) == 0 || (ind&LAYEROE) != 0)) continue;
			if (j == 0) addstringtoinfstr(infstr, x_(" (")); else
				addtoinfstr(infstr, ',');
			j++;
			addstringtoinfstr(infstr, layername(el_curtech, i));
		}
		if (j != 0) addtoinfstr(infstr, ')');
		addstringtoinfstr(infstr, _(" is"));
	} else
	{
		if ((ind&LAYERG) != 0) addstringtoinfstr(infstr, _("Grid entry is")); else
			if ((ind&LAYERH) != 0) addstringtoinfstr(infstr, _("Highlight entry is"));
	}
	(void)esnprintf(line, 40, _(" color (%ld,%ld,%ld)"), ((INTBIG *)varred->addr)[ind],
		((INTBIG *)vargreen->addr)[ind], ((INTBIG *)varblue->addr)[ind]);
	addstringtoinfstr(infstr, line);
	ttyputmsg(x_("%s"), returninfstr(infstr));
}

/*
 * routine to make a line of text that describes cell "np".  The text
 * includes the cell name, version, creation, and revision dates.  The
 * amount of space to leave for the cell name is "maxlen" characters
 * (which is negative if dumping to a file).
 */
CHAR *us_makecellline(NODEPROTO *np, INTBIG maxlen)
{
	CHAR line[40];
	REGISTER INTBIG l, i, gooddrc, len, total, lambda;
	INTBIG year, month, mday, hour, minute, second;
	REGISTER CHAR *pt;
	REGISTER VARIABLE *var;
	REGISTER NODEINST *ni;
	UINTBIG lastgooddate;
	REGISTER void *infstr;

	infstr = initinfstr();
	pt = describenodeproto(np);
	addstringtoinfstr(infstr, pt);
	if (maxlen < 0) addtoinfstr(infstr, '\t'); else
	{
		l = estrlen(pt);
		for(i=l; i<maxlen; i++) addtoinfstr(infstr, ' ');
	}

	/* add the version number */
	(void)esnprintf(line, 40, x_("%5ld"), np->version);
	addstringtoinfstr(infstr, line);
	if (maxlen < 0) addtoinfstr(infstr, '\t'); else
		addstringtoinfstr(infstr, x_("   "));

	/* add the creation date */
	if (np->creationdate == 0)
	{
		if (maxlen < 0) addstringtoinfstr(infstr, _("UNRECORDED")); else
			addstringtoinfstr(infstr, _("     UNRECORDED     "));
	} else
	{
		if (maxlen < 0)
		{
			parsetime((time_t)np->creationdate, &year, &month, &mday, &hour, &minute, &second);
			esnprintf(line, 40, x_("%4ld-%02ld-%02ld %02ld:%02ld:%02ld"), year, month, mday, hour,
				minute, second);
			addstringtoinfstr(infstr, line);
		} else
		{
			pt = timetostring(np->creationdate);
			if (pt == NULL)
			{
				addstringtoinfstr(infstr, _("     UNAVAILABLE     "));
			} else
			{
				for(i=4; i<=9; i++) addtoinfstr(infstr, pt[i]);
				for(i=19; i<=23; i++) addtoinfstr(infstr, pt[i]);
				for(i=10; i<=18; i++) addtoinfstr(infstr, pt[i]);
			}
		}
	}

	/* add the revision date */
	if (maxlen < 0) addtoinfstr(infstr, '\t'); else
		addstringtoinfstr(infstr, x_("  "));
	if (np->revisiondate == 0)
	{
		if (maxlen < 0) addstringtoinfstr(infstr, _("UNRECORDED")); else
			addstringtoinfstr(infstr, _("     UNRECORDED     "));
	} else
	{
		if (maxlen < 0)
		{
			parsetime((time_t)np->revisiondate, &year, &month, &mday, &hour, &minute, &second);
			esnprintf(line, 40, x_("%4ld-%02ld-%02ld %02ld:%02ld:%02ld"), year, month, mday, hour,
				minute, second);
			addstringtoinfstr(infstr, line);
		} else
		{
			pt = timetostring(np->revisiondate);
			if (pt == NULL)
			{
				addstringtoinfstr(infstr, _("     UNAVAILABLE     "));
			} else
			{
				for(i=4; i<=9; i++) addtoinfstr(infstr, pt[i]);
				for(i=19; i<=23; i++) addtoinfstr(infstr, pt[i]);
				for(i=10; i<=18; i++) addtoinfstr(infstr, pt[i]);
			}
		}
	}

	/* add the size */
	if (maxlen < 0) addtoinfstr(infstr, '\t'); else
		addstringtoinfstr(infstr, x_("  "));
	if ((np->cellview->viewstate&TEXTVIEW) != 0)
	{
		var = getvalkey((INTBIG)np, VNODEPROTO, VSTRING|VISARRAY, el_cell_message_key);
		if (var == NOVARIABLE) len = 0; else
			len = getlength(var);
		if (maxlen < 0) esnprintf(line, 40, _("%ld lines"), len); else
			esnprintf(line, 40, _("%8ld lines   "), len);
		addstringtoinfstr(infstr, line);
	} else
	{
		lambda = lambdaofcell(np);
		estrcpy(line, latoa(np->highx-np->lowx, lambda));
		if (maxlen >= 0)
		{
			for(i = estrlen(line); i<8; i++) addtoinfstr(infstr, ' ');
		}
		addstringtoinfstr(infstr, line);
		addtoinfstr(infstr, 'x');
		estrcpy(line, latoa(np->highy-np->lowy, lambda));
		addstringtoinfstr(infstr, line);
		if (maxlen >= 0)
		{
			for(i = estrlen(line); i<8; i++) addtoinfstr(infstr, ' ');
		}
	}
	if (maxlen < 0) addtoinfstr(infstr, '\t');

	/* count the number of instances */
	total = 0;
	for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst) total++;
	if (maxlen < 0) esnprintf(line, 40, x_("%ld"), total); else
		esnprintf(line, 40, x_("%4ld"), total);
	addstringtoinfstr(infstr, line);

	/* show other factors about the cell */
	if (maxlen < 0) addtoinfstr(infstr, '\t'); else
		addstringtoinfstr(infstr, x_("   "));
	if ((np->userbits&NPLOCKED) != 0) addstringtoinfstr(infstr, x_("L")); else
		addstringtoinfstr(infstr, x_(" "));
	if (maxlen < 0) addtoinfstr(infstr, '\t'); else
		addstringtoinfstr(infstr, x_(" "));
	if ((np->userbits&NPILOCKED) != 0) addstringtoinfstr(infstr, x_("I")); else
		addstringtoinfstr(infstr, x_(" "));
	if (maxlen < 0) addtoinfstr(infstr, '\t'); else
		addstringtoinfstr(infstr, x_(" "));
	if ((np->userbits&INCELLLIBRARY) != 0) addstringtoinfstr(infstr, x_("C")); else
		addstringtoinfstr(infstr, x_(" "));
	if (maxlen < 0) addtoinfstr(infstr, '\t'); else
		addstringtoinfstr(infstr, x_(" "));
	gooddrc = 0;
	var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER, dr_lastgooddrckey);
	if (var != NOVARIABLE)
	{
		lastgooddate = (UINTBIG)var->addr;
		if (np->revisiondate <= lastgooddate) gooddrc = 1;
	}
	if (gooddrc != 0) addstringtoinfstr(infstr, x_("D")); else
		addstringtoinfstr(infstr, x_(" "));
	if (maxlen < 0) addtoinfstr(infstr, '\t'); else
		addstringtoinfstr(infstr, x_(" "));
	if (net_ncchasmatch(np) != 0) addstringtoinfstr(infstr, x_("N")); else
		addstringtoinfstr(infstr, x_(" "));
	return(returninfstr(infstr));
}

/******************** KEYBOARD SUPPORT ********************/

/*
 * routine to check to see if library "lib" has changed and warn the
 * user prior to destroying changes to that library.  If "lib" is NOLIBRARY, all
 * libraries are checked.  "action" is the type of action to be performed:
 * 0=quit, 1=close library, 2=replace library
 * If the operation can be cancelled, "cancancel" is true.  Returns true
 * if the operation is to be aborted.
 */
BOOLEAN us_preventloss(LIBRARY *lib, INTBIG action, BOOLEAN cancancel)
{
	REGISTER INTBIG result, count;
	CHAR *par[10];
	REGISTER LIBRARY *l;
	extern COMCOMP us_yesnop;
	REGISTER void *infstr;

	for(l = el_curlib; l != NOLIBRARY; l = l->nextlibrary)
	{
		if ((l->userbits&HIDDENLIBRARY) != 0) continue;
		if (lib != NOLIBRARY && lib != l) continue;
		if ((l->userbits&(LIBCHANGEDMAJOR|LIBCHANGEDMINOR)) == 0) continue;

		infstr = initinfstr();
		if ((l->userbits&LIBCHANGEDMAJOR) != 0)
			formatinfstr(infstr, _("Library %s has changed significantly.  "), l->libname); else
				formatinfstr(infstr, _("Library %s has changed insignificantly.  "), l->libname);
		switch (action)
		{
			case 0: addstringtoinfstr(infstr, _("Save before quitting?"));   break;
			case 1: addstringtoinfstr(infstr, _("Save before closing?"));    break;
			case 2: addstringtoinfstr(infstr, _("Save before replacing?"));  break;
		}
		if (cancancel)
		{
			result = us_quitdlog(returninfstr(infstr), (action==0 ? 1 : 0));
			if (result == 0)
			{
				ttyputverbose(M_("Keep working"));
				return(TRUE);
			}
			if (result == 1) continue;
			if (result == 2) break;
			if (result == 3)
			{
				/* save the library */
				makeoptionstemporary(l);
				(void)asktool(io_tool, x_("write"), (INTBIG)l, (INTBIG)x_("binary"));
			}
		} else
		{
			count = ttygetparam(returninfstr(infstr), &us_yesnop, MAXPARS, par);
			if (count > 0 && namesamen(par[0], x_("yes"), estrlen(par[0])) == 0)
			{
				/* save the library */
				makeoptionstemporary(l);
				(void)asktool(io_tool, x_("write"), (INTBIG)l, (INTBIG)x_("binary"));
			}
		}
	}

	/* also check for option changes on quit */
	if (us_optionschanged && action == 0)
	{
		if (optionshavechanged())
		{
			if (us_saveoptdlog(TRUE))
			{
				ttyputverbose(M_("Keep working"));
				return(TRUE);
			}
		}
	}
	return(FALSE);
}

/*
 * Routine to save all options by creating a dummy library and saving it (without
 * making options temporary first).
 */
void us_saveoptions(void)
{
	REGISTER INTBIG retval;
	REGISTER LIBRARY *lib;
	REGISTER CHAR *libname, *libfile;

	libname = us_tempoptionslibraryname();
	libfile = optionsfilepath();
	lib = newlibrary(libname, libfile);
	if (lib == NOLIBRARY)
	{
		ttyputerr(_("Cannot create options library %s"), libfile);
		return;
	}
	lib->userbits |= READFROMDISK | HIDDENLIBRARY;
	retval = asktool(io_tool, x_("write"), (INTBIG)lib, (INTBIG)x_("nobackupbinary"));
	if (retval != 0) ttyputerr(_("Could not save options library")); else
		ttyputmsg(_("Options have been saved to %s"), lib->libfile);
	killlibrary(lib);
	us_optionschanged = FALSE;

	/* recache the options so that the system knows what has changed */
	cacheoptionbitvalues();
}

/*
 * Routine to construct a temporary library name that doesn't already exist.
 */
CHAR *us_tempoptionslibraryname(void)
{
	static CHAR libname[256];
	REGISTER LIBRARY *lib;
	REGISTER INTBIG i;

	for(i=1; ; i++)
	{
		esnprintf(libname, 256, x_("options%ld"), i);
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			if (namesame(libname, lib->libname) == 0) break;
		if (lib == NOLIBRARY) break;
	}
	return(libname);
}

/*
 * Routine to ensure that everything gets saved.  Returns TRUE if something got saved.
 */
BOOLEAN us_saveeverything(void)
{
	REGISTER INTBIG total, i, l, retval;
	REGISTER LIBRARY *lib, *deplib, **libsave;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER BOOLEAN gotsaved;

	/* see which libraries need to be saved */
	gotsaved = FALSE;
	total = 0;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
		if ((lib->userbits&(LIBCHANGEDMAJOR | LIBCHANGEDMINOR)) == 0) continue;
		total++;
	}
	if (total == 0) return(gotsaved);
	libsave = (LIBRARY **)emalloc(total * (sizeof (LIBRARY *)), el_tempcluster);
	if (libsave == 0) return(gotsaved);

	/* make a list of the libraries to save */
	total = 0;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
		if ((lib->userbits&(LIBCHANGEDMAJOR | LIBCHANGEDMINOR)) == 0) continue;
		libsave[total++] = lib;
	}

	/* be sure the most referenced libraries are saved first */
	for(i=0; i<total; i++) libsave[i]->temp1 = i;
	for(i=0; i<total; i++)
	{
		lib = libsave[i];
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto->primindex != 0) continue;
				deplib = ni->proto->lib;
				if (deplib == lib) continue;
				if (deplib->temp1 <= lib->temp1) continue;
				l = deplib->temp1;   deplib->temp1 = lib->temp1;   lib->temp1 = l;
			}
		}
	}
	esort(libsave, total, sizeof (LIBRARY *), us_librarytemp1ascending);

	/* now save every library that needs it */
	for(i=0; i<total; i++)
	{
		lib = libsave[i];

		/* save the library in binary format */
		makeoptionstemporary(lib);
		retval = asktool(io_tool, x_("write"), (INTBIG)lib, (INTBIG)x_("binary"));
		restoreoptionstate(lib);
		if (retval != 0) continue;
		gotsaved = TRUE;
	}

	/* clean up */
	efree((CHAR *)libsave);

	/* save options if any have changed */
	if (us_optionschanged && gotsaved && us_logrecord != NULL)
	{
		if (optionshavechanged())
			(void)us_saveoptdlog(FALSE);
	}
	return(gotsaved);
}

/* Prompt: Session Logging */
static DIALOGITEM us_seslogdialogitems[] =
{
 /*  1 */ {0, {204,100,228,268}, BUTTON, N_("Save All Information")},
 /*  2 */ {0, {240,100,264,268}, BUTTON, N_("Disable Session Logging")},
 /*  3 */ {0, {4,4,20,284}, MESSAGE, N_("Warning: not all information has been saved.")},
 /*  4 */ {0, {36,4,52,356}, MESSAGE, N_("Unless all libraries and options are saved together")},
 /*  5 */ {0, {56,4,72,356}, MESSAGE, N_("It is not possible to reconstruct the session after a crash.")},
 /*  6 */ {0, {88,4,104,356}, MESSAGE, N_("The following information has not been saved:")},
 /*  7 */ {0, {108,4,192,356}, SCROLL, x_("")}
};
static DIALOG us_seslogdialog = {{75,75,348,441}, N_("Session Logging Warning"), 0, 7, us_seslogdialogitems, 0, 0};

#define DSEL_SAVEALL    1		/* save all information (button) */
#define DSEL_DISLOG     2		/* disable session logging (button) */
#define DSEL_UNSAVED    7		/* what is unsaved (scroll) */

void us_continuesessionlogging(void)
{
	REGISTER BOOLEAN unsaved;
	REGISTER void *dia, *infstr;
	REGISTER INTBIG itemHit;
	REGISTER LIBRARY *lib;

	/* if session logging is already off, never mind */
	if (us_logrecord == NULL) return;

	/* see if there are any unsaved libraries */
	unsaved = FALSE;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
		if ((lib->userbits&(LIBCHANGEDMAJOR | LIBCHANGEDMINOR)) != 0)
			unsaved = TRUE;
	}
	if (us_optionschanged && optionshavechanged())
		unsaved = TRUE;

	/* if something is unsaved, prompt the user */
	if (unsaved)
	{
		dia = DiaInitDialog(&us_seslogdialog);
		if (dia == 0) return;
		DiaInitTextDialog(dia, DSEL_UNSAVED, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1, 0);
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		{
			if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
			if ((lib->userbits&(LIBCHANGEDMAJOR | LIBCHANGEDMINOR)) == 0) continue;
			infstr = initinfstr();
			formatinfstr(infstr, x_("Library %s"), lib->libname);
			DiaStuffLine(dia, DSEL_UNSAVED, returninfstr(infstr));
		}
		if (us_optionschanged && optionshavechanged())
			DiaStuffLine(dia, DSEL_UNSAVED, _("Options"));
		DiaSelectLine(dia, DSEL_UNSAVED, -1);
		for(;;)
		{
			itemHit = DiaNextHit(dia);
			if (itemHit == DSEL_SAVEALL || itemHit == DSEL_DISLOG) break;
		}
		DiaDoneDialog(dia);
		if (itemHit == DSEL_DISLOG)
		{
			/* disable session logging */
			logfinishrecord();
			return;
		}

		/* save everything */
		(void)us_saveeverything();
	}

	/* erase everything in the clipboard */
	us_clearclipboard();

	/* close all modeless dialogs */
	DiaCloseAllModeless();

	/* truncate the session log */
	logfinishrecord();
	logstartrecord();
}

/*
 * routine to determine the color map entry that corresponds to the string
 * "pp".  If the string is a number then return that number.  If the string
 * contains layer letters from the current technology, return that index.
 * The value of "purpose" determines how to interpret the layer letters: 0
 * means that the letters indicate entries in the color map and 1 means that
 * only one letter is allowed and its layer number should be returned.  If no
 * entry can be determined, the routine issues an error and returns -1.
 */
INTBIG us_getcolormapentry(CHAR *pp, BOOLEAN layerletter)
{
	CHAR *ch, *pt1;
	REGISTER INTBIG color, i, high;
	GRAPHICS *desc;

	/* first see if the string is all digits and return that value if so */
	if (isanumber(pp)) return(myatoi(pp));

	/* for layer conversion, the job is simple */
	high = el_curtech->layercount;
	if (layerletter)
	{
		if (pp[0] == 0 || pp[1] != 0)
		{
			us_abortcommand(_("Can only give one layer letter"));
			return(-1);
		}
		for(i=0; i<high; i++)
		{
			for(ch = us_layerletters(el_curtech, i); *ch != 0; ch++)
				if (*ch == pp[0]) return(i);
		}
		us_abortcommand(_("Letter '%s' is not a valid layer"), pp);
		return(-1);
	}

	/* accumulate the desired color */
	color = 0;
	for(pt1 = pp; *pt1 != 0; pt1++)
	{
		/* find the layer that corresponds to letter "*pt1" */
		for(i=0; i<high; i++)
		{
			/* see if this layer has the right letter */
			for(ch = us_layerletters(el_curtech, i); *ch != 0; ch++)
				if (*ch == *pt1) break;
			if (*ch == 0) continue;

			/* get the color characteristics of this layer */
			desc = el_curtech->layers[i];
			if ((desc->bits & color) != 0)
			{
				us_abortcommand(_("No single color for the letters '%s'"), pp);
				return(-1);
			}
			color |= desc->col;
			break;
		}
		if (i == high)
		{
			us_abortcommand(_("Letter '%c' is not a valid layer"), *pt1);
			return(-1);
		}
	}
	return(color);
}

/*
 * routine to return a unique port prototype name in cell "cell" given that
 * a new prototype wants to be named "name".  The routine allocates space
 * for the string that is returned so this must be freed when done.
 */
CHAR *us_uniqueportname(CHAR *name, NODEPROTO *cell)
{
	CHAR *str;

	str = us_uniqueobjectname(name, cell, VPORTPROTO, 0);

	if (us_uniqueretstr != NOSTRING) efree(us_uniqueretstr);
	(void)allocstring(&us_uniqueretstr, str, us_tool->cluster);
	return(us_uniqueretstr);
}

/*
 * Routine to determine whether the name "name" is unique in cell "cell"
 * (given that it is on objects of type "type").  Does not consider object
 * "exclude" (if nonzero).
 */
BOOLEAN us_isuniquename(CHAR *name, NODEPROTO *cell, INTBIG type, void *exclude)
{
	REGISTER PORTPROTO *pp;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var;

	switch (type)
	{
		case VPORTPROTO:
			pp = getportproto(cell, name);
			if (pp == NOPORTPROTO) break;
			if (exclude != 0 && (PORTPROTO *)exclude == pp) break;
			return(FALSE);
		case VNODEINST:
			for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (exclude != 0 && (NODEINST *)exclude == ni) continue;
				var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
				if (var == NOVARIABLE) continue;
				if (namesame(name, (CHAR *)var->addr) == 0) return(FALSE);
			}
			break;
		case VARCINST:
			for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			{
				if (exclude != 0 && (ARCINST *)exclude == ai) continue;
				var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
				if (var == NOVARIABLE) continue;
				if (namesame(name, (CHAR *)var->addr) == 0) return(FALSE);
			}
			break;
	}
	return(TRUE);
}

/*
 * routine to return a unique object name in cell "cell" starting with the
 * name "name".
 */
CHAR *us_uniqueobjectname(CHAR *name, NODEPROTO *cell, INTBIG type, void *exclude)
{
	CHAR *newname, separatestring[2];
	REGISTER INTBIG nextindex, i, possiblestart, possibleend, startindex, endindex, spacing,
		endpos, startpos;
	BOOLEAN foundnumbers;
	REGISTER void *infstr;

	/* first see if the name is unique */
	if (us_isuniquename(name, cell, type, exclude)) return(name);

	/* now see if the name ends in "]" */
	i = estrlen(name);
	if (name[i-1] == ']')
	{
		/* see if the array contents can be incremented */
		possiblestart = -1;
		endpos = i-1;
		for(;;)
		{
			/* find the range of characters in square brackets */
			for(startpos = endpos; startpos >= 0; startpos--)
				if (name[startpos] == '[') break;
			if (name[startpos] != '[') break;

			/* see if there is a comma in the bracketed expression */
			for(i=startpos+1; i<endpos; i++)
				if (name[i] == ',') break;
			if (name[i] == ',')
			{
				/* this bracketed expression cannot be incremented: move on */
				if (startpos > 0 && name[startpos-1] == ']')
				{
					endpos = startpos-1;
					continue;
				}
				break;
			}

			/* see if there is a colon in the bracketed expression */
			for(i=startpos+1; i<endpos; i++)
				if (name[i] == ':') break;
			if (name[i] == ':')
			{
				/* colon: make sure there are two numbers */
				name[i] = 0;
				name[endpos] = 0;
				foundnumbers = isanumber(&name[startpos+1]) && isanumber(&name[i+1]);
				name[i] = ':';
				name[endpos] = ']';
				if (foundnumbers)
				{
					startindex = atoi(&name[startpos+1]);
					endindex = atoi(&name[i+1]);
					spacing = abs(endindex - startindex) + 1;
					for(nextindex = 1; ; nextindex++)
					{
						infstr = initinfstr();
						for(i=0; i<startpos; i++) addtoinfstr(infstr, name[i]);
						formatinfstr(infstr, x_("[%ld:%ld"), startindex+spacing*nextindex,
							endindex+spacing*nextindex);
						addstringtoinfstr(infstr, &name[endpos]);
						newname = returninfstr(infstr);
						if (us_isuniquename(newname, cell, type, 0)) break;
					}
					return(newname);
				}

				/* this bracketed expression cannot be incremented: move on */
				if (startpos > 0 && name[startpos-1] == ']')
				{
					endpos = startpos-1;
					continue;
				}
				break;
			}

			/* see if this bracketed expression is a pure number */
			name[endpos] = 0;
			foundnumbers = isanumber(&name[startpos+1]);
			name[endpos] = ']';
			if (foundnumbers)
			{
				nextindex = myatoi(&name[startpos+1]) + 1;
				for(; ; nextindex++)
				{
					infstr = initinfstr();
					for(i=0; i<startpos; i++) addtoinfstr(infstr, name[i]);
					formatinfstr(infstr, x_("[%ld"), nextindex);
					addstringtoinfstr(infstr, &name[endpos]);
					newname = returninfstr(infstr);
					if (us_isuniquename(newname, cell, type, 0)) break;
				}
				return(newname);
			}

			/* remember the first index that could be incremented in a pinch */
			if (possiblestart < 0)
			{
				possiblestart = startpos;
				possibleend = endpos;
			}

			/* this bracketed expression cannot be incremented: move on */
			if (startpos > 0 && name[startpos-1] == ']')
			{
				endpos = startpos-1;
				continue;
			}
			break;
		}

		/* if there was a possible place to increment, do it */
		if (possiblestart >= 0)
		{
			/* nothing simple, but this one can be incremented */
			for(i=possibleend-1; i>possiblestart; i--)
				if (!isdigit(name[i])) break;
			nextindex = myatoi(&name[i+1]) + 1;
			startpos = i+1;
			if (name[startpos-1] == us_separatechar) startpos--;
			for(; ; nextindex++)
			{
				infstr = initinfstr();
				for(i=0; i<startpos; i++) addtoinfstr(infstr, name[i]);
				formatinfstr(infstr, x_("%c%ld"), us_separatechar, nextindex);
				addstringtoinfstr(infstr, &name[possibleend]);
				newname = returninfstr(infstr);
				if (us_isuniquename(newname, cell, type, 0)) break;
			}
			return(newname);
		}
	}

	/* array contents cannot be incremented: increment base name */
	for(startpos=0; name[startpos] != 0; startpos++)
		if (name[startpos] == '[') break;
	endpos = startpos;

	/* if there is a numeric part at the end, increment that */
	separatestring[0] = (CHAR)us_separatechar;
	separatestring[1] = 0;
	while (startpos > 0 && isdigit(name[startpos-1])) startpos--;
	if (startpos >= endpos)
	{
		nextindex = 1;
		if (startpos > 0 && name[startpos-1] == us_separatechar) startpos--;
	} else
	{
		nextindex = myatoi(&name[startpos]) + 1;
		separatestring[0] = 0;
	}

	for(; ; nextindex++)
	{
		infstr = initinfstr();
		for(i=0; i<startpos; i++) addtoinfstr(infstr, name[i]);
		formatinfstr(infstr, x_("%s%ld"), separatestring, nextindex);
		addstringtoinfstr(infstr, &name[endpos]);
		newname = returninfstr(infstr);
		if (us_isuniquename(newname, cell, type, 0)) break;
	}
	return(newname);
}

/*
 * routine to initialize the database variable "USER_layer_letters".  This
 * is called once at initialization and again whenever the array is changed.
 */
void us_initlayerletters(void)
{
	REGISTER VARIABLE *var;
	REGISTER TECHNOLOGY *t;

	if (us_layer_letter_data != 0) efree((CHAR *)us_layer_letter_data);
	us_layer_letter_data = emalloc((el_maxtech * SIZEOFINTBIG), us_tool->cluster);
	if (us_layer_letter_data == 0) return;
	for(t = el_technologies; t != NOTECHNOLOGY; t = t->nexttechnology)
	{
		var = getvalkey((INTBIG)t, VTECHNOLOGY, VSTRING|VISARRAY, us_layer_letters_key);
		us_layer_letter_data[t->techindex] = (var == NOVARIABLE ? 0 : var->addr);
	}
}

/*
 * routine to return a string of unique letters describing layer "layer"
 * in technology "tech".  The letters for all layers of a given technology
 * must not intersect.  This routine accesses the "USER_layer_letters"
 * variable on the technology objects.
 */
CHAR *us_layerletters(TECHNOLOGY *tech, INTBIG layer)
{
	REGISTER INTBIG addr;

	if (us_layer_letter_data == 0)
	{
		us_initlayerletters();
		if (us_layer_letter_data == 0) return(x_(""));
	}

	addr = us_layer_letter_data[tech->techindex];
	if (addr == 0) return(x_(""));
	return(((CHAR **)addr)[layer]);
}

/*
 * routine to change an tool state.  The name of the tool is in "pt" (if "pt" is
 * null then an tool name is prompted).  The state of the tool is set to "state"
 * (0 for off, 1 for permanently off, 2 for on).
 */
void us_settool(INTBIG count, CHAR *par[], INTBIG state)
{
	REGISTER INTBIG i;
	BOOLEAN toolstate;
	REGISTER CHAR *pt;
	extern COMCOMP us_onofftoolp;
	REGISTER TOOL *tool;

	if (count == 0)
	{
		count = ttygetparam(M_("Which tool: "), &us_onofftoolp, MAXPARS, par);
		if (count == 0)
		{
			us_abortcommand(M_("Specify an tool to control"));
			return;
		}
	}
	pt = par[0];

	for(i=0; i<el_maxtools; i++)
		if (namesame(el_tools[i].toolname, pt) == 0) break;
	if (i >= el_maxtools)
	{
		us_abortcommand(_("No tool called %s"), pt);
		return;
	}
	tool = &el_tools[i];

	if (tool == us_tool && state <= 1)
	{
		us_abortcommand(M_("No! I won't go!"));
		return;
	}
	if ((tool->toolstate&TOOLON) == 0 && state <= 1)
	{
		ttyputverbose(M_("%s already off"), pt);
		return;
	}
	if ((tool->toolstate&TOOLON) != 0 && state > 1)
	{
		ttyputverbose(M_("%s already on"), pt);
		return;
	}

	if (state <= 1)
	{
		ttyputverbose(M_("%s turned off"), tool->toolname);
		if (state != 0) toolstate = TRUE; else
			toolstate = FALSE;
		toolturnoff(tool, toolstate);
	} else
	{
		if ((tool->toolstate&TOOLINCREMENTAL) == 0)
			ttyputverbose(M_("%s turned on"), tool->toolname); else
				ttyputmsg(_("%s turned on, catching up on changes"), tool->toolname);
		toolturnon(tool);
	}
}
