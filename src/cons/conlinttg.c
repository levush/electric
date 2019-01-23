/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: conlinttg.c
 * Linear inequality constraint system: text-to-graphics conversion
 * Written by: Steven M. Rubin, Static Free Software
 *
 * Copyright (c) 2001 Static Free Software.
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
#include "database.h"
#include "conlin.h"

static BOOLEAN (*cli_realcharhandler)(WINDOWPART*, INTSML, INTBIG);
static void (*cli_oldttermhandler)(WINDOWPART*);	/* former text editor termination routine */

/* prototypes for local routines */
static BOOLEAN cli_charhandler(WINDOWPART*, INTSML, INTBIG);
static void cli_termhandler(WINDOWPART*);
static void cli_changehandler(WINDOWPART*, INTBIG, CHAR*, CHAR*, INTBIG);
static void cli_changeexport(CHAR*, CHAR*, INTBIG, WINDOWPART*);
static void cli_changedeclaration(CHAR*, CHAR*, INTBIG, WINDOWPART*);
static void cli_changeconnection(CHAR*, CHAR*, INTBIG, WINDOWPART*);
static void cli_updateattrs(ATTR*, ATTR*, INTBIG, INTBIG);
static void cli_deletedeclaration(CHAR*);
static void cli_deletewire(CHAR*);
static void cli_deleteexportline(CHAR*);
static BOOLEAN cli_adddeclaration(CHAR*, INTBIG, WINDOWPART*);
static BOOLEAN cli_addwire(CHAR*, INTBIG, WINDOWPART*);
static void cli_addexport(CHAR*, INTBIG, WINDOWPART*);
static PORTPROTO *cli_getexportsubport(EXPORT*, NODEINST*);
static void cli_getfactors(NODEINST*, NODEPROTO*, COMPONENT*, INTBIG*, INTBIG*, INTBIG*,
				INTBIG*, INTBIG*, INTBIG*, INTBIG, INTBIG);
static void cli_killthenode(NODEINST*);
static void cli_commentline(INTBIG, WINDOWPART*);
static void cli_restoreline(CHAR*, INTBIG, WINDOWPART*);

/************************ EDITOR WINDOW CONTROL ************************/

/*
 * routine to create a new window that can do text editing.  The window
 * title is in "header" and the number of characters/lines are returned
 * in "chars" and "lines".  Returns NOWINDOWPART if there is an error.
 */
WINDOWPART *cli_makeeditorwindow(CHAR *header, INTBIG *chars, INTBIG *lines)
{
	REGISTER WINDOWPART *win;

	win = (WINDOWPART *)asktool(us_tool, x_("window-new"));
	if (win == NOWINDOWPART) return(NOWINDOWPART);
	if (asktool(us_tool, x_("edit-starteditor"), (INTBIG)win, (INTBIG)header,
		(INTBIG)chars, (INTBIG)lines) != 0) return(NOWINDOWPART);

	cli_realcharhandler = win->charhandler;
	cli_oldttermhandler = win->termhandler;
	win->charhandler = cli_charhandler;
	win->changehandler = cli_changehandler;
	win->termhandler = cli_termhandler;
	cli_texton = TRUE;
	return(win);
}

/*
 * private character handler for the text window.  This routine normally
 * passes all commands to the editing character handler.  However, it
 * interprets M(=) which is for highlighting the equivalent graphics
 */
BOOLEAN cli_charhandler(WINDOWPART *win, INTSML i, INTBIG special)
{
	REGISTER CHAR *line;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni;
	REGISTER COMPONENTDEC *comd;
	REGISTER COMPONENT *compo;
	REGISTER INTBIG count, type;

	/* simply pass most characters to the real editor character handler */
	if (special == 0 || (i != '=' && cli_realcharhandler != 0))
	{
		return((*cli_realcharhandler)(win, i, special));
	}

	/* M(=) typed: highlight the equivalent line */
	if (cli_curcell == NONODEPROTO)
	{
		ttyputmsg(M_("No valid cell to associate"));
		return(FALSE);
	}
	line = (CHAR *)asktool(us_tool, x_("edit-getline"), (INTBIG)win, -1);
	if (line == NOSTRING) type = LINEUNKN; else type = cli_linetype(line);

	switch (type)
	{
		case LINECOMM:
			ttyputmsg(M_("No graphic equivalent of comment line"));
			break;
		case LINEBEGIN:
			ttyputmsg(M_("No graphic equivalent of BEGINCELL line"));
			break;
		case LINEEND:
			ttyputmsg(M_("No graphic equivalent of ENDCELL line"));
			break;
		case LINEEXPORT:
			ttyputmsg(M_("No graphic equivalent of EXPORT line"));
			break;
		case LINEUNKN:
			ttyputmsg(M_("No graphic equivalent of this line"));
			break;

		case LINECONN:
			ai = cli_findarcname(line);
			if (ai == NOARCINST)
			{
				ttyputmsg(M_("Cannot find equivalent arc for this line"));
				return(0);
			}
			(void)asktool(us_tool, x_("clear"));
			(void)asktool(us_tool, x_("show-object"), (INTBIG)ai->geom);
			break;

		case LINEDECL:
			comd = cli_parsecomp(line, TRUE);
			if (comd == NOCOMPONENTDEC)
			{
				ttyputmsg(M_("Cannot parse this declaration line"));
				return(FALSE);
			}
			count = 0;
			for(compo = comd->firstcomponent; compo != NOCOMPONENT; compo = compo->nextcomponent)
			{
				ni = cli_findnodename(compo->name);
				if (ni == NONODEINST) continue;
				if (count == 0) (void)asktool(us_tool, x_("clear"));
				(void)asktool(us_tool, x_("show-object"), (INTBIG)ni->geom);
				count++;
			}
			if (count == 0)
			{
				ttyputmsg(M_("Cannot find equivalent node(s) for this line"));
				return(FALSE);
			}
			break;
	}
	return(FALSE);
}

/*
 * routine that is informed when the text window is deleted
 */
void cli_termhandler(WINDOWPART *win)
{
	cli_texton = FALSE;
	cli_curcell = NONODEPROTO;
	if (cli_oldttermhandler!= 0) (*cli_oldttermhandler)(win);
}

/*
 * routine to handle changes to the text window.
 */
void cli_changehandler(WINDOWPART *win, INTBIG nature, CHAR *oldline, CHAR *newline,
	INTBIG changed)
{
	CHAR *newpar[3];
	REGISTER CHAR *oldname, *newname, **newlist;
	REGISTER INTBIG otype, ntype;
	REGISTER INTBIG i, count;
	REGISTER NODEPROTO *np;
	REGISTER WINDOWPART *w;
	REGISTER void *infstr;

	switch (nature)
	{
		case REPLACETEXTLINE:                break;
		case DELETETEXTLINE: newline = x_("");   break;
		case INSERTTEXTLINE: oldline = x_("");   break;
		case REPLACEALLTEXT:
			/* this is not totally correct, because it ignores the former line values!!! */
			newlist = (CHAR **)newline;
			count = changed;
			for(i=0; i<count; i++)
				cli_changehandler(win, REPLACETEXTLINE, x_(""), newlist[i], i);
			return;
	}

	/* determine the nature of the changed lines */
	ntype = cli_linetype(newline);
	otype = cli_linetype(oldline);

	/* comment out an unknown line type */
	if (ntype == LINEUNKN)
	{
		ttyputerr(M_("Unrecognized line '%s' commented out"), newline);
		if (otype == LINEUNKN || otype == LINECOMM) cli_commentline(changed, win); else
			cli_restoreline(oldline, changed, win);
		return;
	}

	/* handle rewrites of the same type of line */
	if (otype == ntype)
	{
		/* ignore changes if cell is not valid */
		if (cli_curcell == NONODEPROTO) return;

		switch (otype)
		{
			case LINEDECL:
				cli_changedeclaration(oldline, newline, changed, win);
				break;
			case LINECONN:
				cli_changeconnection(oldline, newline, changed, win);
				break;
			case LINEEXPORT:
				cli_changeexport(oldline, newline, changed, win);
				break;
			case LINEBEGIN:
				oldname = cli_parsebegincell(oldline, FALSE);
				newname = cli_parsebegincell(newline, FALSE);
				if (oldname != NOSTRING && newname != NOSTRING)
				{
					if (namesame(oldname, newname) != 0)
					{
						(void)setval((INTBIG)cli_curcell, VNODEPROTO, x_("protoname"), (INTBIG)newname, VSTRING);
						ttyputmsg(M_("Cell name changed from %s to %s"), oldname, newname);
					}
				} else if (newname == NOSTRING)
				{
					ttyputmsg(M_("Erroneous BEGINCELL line"));
					cli_restoreline(oldline, changed, win);
				}
				if (oldname != NOSTRING) efree(oldname);
				if (newname != NOSTRING) efree(newname);
				break;
			case LINEEND:
				break;
			case LINECOMM:
				break;
		}
		return;
	}

	/* handle the deletion of text lines */
	if (otype != LINECOMM && otype != LINEUNKN)
	{
		switch (otype)
		{
			case LINEDECL:
				if (cli_curcell != NONODEPROTO) cli_deletedeclaration(oldline);
				cli_textlines--;
				break;
			case LINECONN:
				if (cli_curcell != NONODEPROTO) cli_deletewire(oldline);
				cli_textlines--;
				break;
			case LINEEXPORT:
				if (cli_curcell != NONODEPROTO) cli_deleteexportline(oldline);
				break;
			case LINEBEGIN:
				cli_curcell = NONODEPROTO;
				break;
			case LINEEND:
				break;
		}
	}

	/* handle the addition of new text lines */
	if (ntype != LINECOMM)
	{
		switch (ntype)
		{
			case LINEDECL:
				if (cli_curcell != NONODEPROTO)
				{
					if (cli_adddeclaration(newline, changed, win)) break;
				}
				cli_textlines++;
				break;

			case LINECONN:
				if (cli_curcell != NONODEPROTO)
				{
					if (cli_addwire(newline, changed, win)) break;
				}
				cli_textlines++;
				break;

			case LINEEXPORT:
				if (cli_curcell != NONODEPROTO)
					(void)cli_addexport(newline, changed, win);
				break;

			case LINEBEGIN:
				/* must be the first line entered */
				if (cli_textlines > 0)
				{
					ttyputmsg(M_("Already a valid cell definition"));
					cli_commentline(changed, win);
					return;
				}

				/* get the cell name */
				newname = cli_parsebegincell(newline, FALSE);
				if (newname == NOSTRING)
				{
					ttyputmsg(M_("Erroneous BEGINCELL line removed"));
					cli_commentline(changed, win);
					return;
				}

				/* delete the specified cell if it exists */
				np = getnodeproto(newname);
				if (np != NONODEPROTO && np->primindex != 0)
				{
					newpar[0] = x_("killcell");
					newpar[1] = newname;
					telltool(us_tool, 2, newpar);
				}

				/* find a window in which to place the cell */
				for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
					if (w->editor == NOEDITOR && w->curnodeproto == NONODEPROTO) break;
				if (w == NOWINDOWPART)
				{
					for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
						if (w->editor == NOEDITOR) break;
				}

				/* create the specified cell */
				if (w != NOWINDOWPART)
				{
					newpar[0] = x_("window");
					newpar[1] = x_("use");
					newpar[2] = w->location;
					telltool(us_tool, 3, newpar);
					newpar[0] = x_("editcell");
					newpar[1] = newname;
					telltool(us_tool, 2, newpar);
					cli_curcell = getcurcell();
				} else cli_curcell = newnodeproto(newname, el_curlib);
				efree(newname);
				break;

			case LINEEND:
				/* put the line at the bottom of the file */
				infstr = initinfstr();
				addstringtoinfstr(infstr, (CHAR *)asktool(us_tool, x_("edit-getline"), (INTBIG)win, changed));
				(void)asktool(us_tool, x_("edit-deleteline"), (INTBIG)win, changed);
				cli_replaceendcell(returninfstr(infstr), win);
				break;
		}
	}
}

/***************************** TEXT CHANGE *****************************/

/*
 * routine to handle the change of export line "oldline" to "newline".
 * The index of the line that changed is in "changed".  Text was changed
 * in window "win".
 */
void cli_changeexport(CHAR *oldline, CHAR *newline, INTBIG changed, WINDOWPART *win)
{
	REGISTER EXPORT *olde, *newe;
	REGISTER NODEINST *oldni, *newni;
	REGISTER PORTPROTO *oldpp, *oldsubpp, *newsubpp;

	/* parse the old line */
	olde = cli_parseexport(oldline, FALSE);
	if (olde == NOEXPORT) return;

	/* parse the new line */
	newe = cli_parseexport(newline, FALSE);

	/* if the new line has an error, restore the old line */
	if (newe == NOEXPORT)
	{
		cli_restoreline(oldline, changed, win);
		cli_deleteexport(olde);
		return;
	}

	/* get the former port */
	oldpp = getportproto(cli_curcell, olde->portname);
	if (oldpp == NOPORTPROTO)
	{
		ttyputerr(M_("Cannot parse old port name"));
		cli_restoreline(oldline, changed, win);
		cli_deleteexport(olde);   cli_deleteexport(newe);
		return;
	}

	/* get the nodes on the ports */
	oldni = cli_findnodename(olde->component);
	newni = cli_findnodename(newe->component);
	if (oldni == NONODEINST || newni == NONODEINST)
	{
		ttyputerr(M_("Cannot parse subnode name"));
		cli_restoreline(oldline, changed, win);
		cli_deleteexport(olde);   cli_deleteexport(newe);
		return;
	}

	/* get the subports on the ports */
	oldsubpp = cli_getexportsubport(olde, oldni);
	newsubpp = cli_getexportsubport(newe, newni);
	if (oldsubpp == NOPORTPROTO || newsubpp == NOPORTPROTO)
	{
		ttyputerr(M_("Cannot parse subport name"));
		cli_restoreline(oldline, changed, win);
		cli_deleteexport(olde);   cli_deleteexport(newe);
		return;
	}

	/* see if the port characteristics changed */
	if (olde->bits != newe->bits)
	{
		if ((olde->bits&STATEBITS) != (newe->bits&STATEBITS))
			oldpp->userbits = (oldpp->userbits & ~STATEBITS) | (newe->bits & STATEBITS);
	}

	/* see if miscellaneous port variables changed */
	cli_updateattrs(olde->firstattr, newe->firstattr, (INTBIG)oldpp, VPORTPROTO);

	/* see if the port name changed */
	if (namesame(olde->portname, newe->portname) != 0)
	{
		if (getportproto(cli_curcell, newe->portname) != NOPORTPROTO)
		{
			ttyputerr(M_("New port name not unique"));
			cli_restoreline(oldline, changed, win);
			cli_deleteexport(olde);   cli_deleteexport(newe);
			return;
		}
		if (oldni == newni && oldsubpp == newsubpp)
			(void)db_change((INTBIG)oldni, OBJECTSTART, VNODEINST, 0, 0, 0, 0, 0);
		(void)reallocstring(&oldpp->protoname, newe->portname, oldpp->parent->lib->cluster);
		if (oldni == newni && oldsubpp == newsubpp)
			(void)db_change((INTBIG)oldni, OBJECTEND, VNODEINST, 0, 0, 0, 0, 0);
	}

	/* see if the port moved */
	if (oldni != newni || oldsubpp != newsubpp)
	{
		cli_ownchanges = TRUE;
		(void)moveportproto(cli_curcell, oldpp, newni, newsubpp);
		cli_ownchanges = FALSE;
	}
}

/*
 * routine to handle the change of declaration line "oldline" to "newline".
 * The index of the line that changed is in "changed".  Text was changed
 * in window "win".
 */
void cli_changedeclaration(CHAR *oldline, CHAR *newline, INTBIG changed,
	WINDOWPART *win)
{
	REGISTER COMPONENTDEC *oldcd, *newcd;
	REGISTER COMPONENT *newcomp, *oldcomp;
	REGISTER NODEPROTO *newnp, *oldnp;
	REGISTER NODEINST *ni;
	REGISTER INTBIG newun, oldun;
	REGISTER VARIABLE *var;
	INTBIG newlx, newly, newhx, newhy, oldlx, oldly, oldhx, oldhy, lx, hx, ly, hy;
	INTBIG newrot, oldrot, newtrans, oldtrans, rot, trn;

	/* parse the old line */
	oldcd = cli_parsecomp(oldline, FALSE);
	if (oldcd == NOCOMPONENTDEC) return;

	/* parse the new line */
	newcd = cli_parsecomp(newline, FALSE);

	/* if the new line has an error, restore the old line */
	if (newcd == NOCOMPONENTDEC)
	{
		cli_restoreline(oldline, changed, win);
		cli_deletecomponentdec(oldcd);
		return;
	}

	/* get the new and old component prototypes */
	newnp = getnodeproto(newcd->protoname);
	if (newnp == NONODEPROTO)
	{
		ttyputerr(M_("Unknown component: %s"), newcd->protoname);
		cli_restoreline(oldline, changed, win);
		cli_deletecomponentdec(oldcd);   cli_deletecomponentdec(newcd);
		return;
	}
	oldnp = getnodeproto(oldcd->protoname);
	if (oldnp == NONODEPROTO)
	{
		ttyputerr(M_("Unknown old component: %s"), oldcd->protoname);
		cli_restoreline(oldline, changed, win);
		cli_deletecomponentdec(oldcd);   cli_deletecomponentdec(newcd);
		return;
	}

	/* associate the component lists */
	for(newcomp = newcd->firstcomponent; newcomp != NOCOMPONENT; newcomp = newcomp->nextcomponent)
		newcomp->assoc = NOCOMPONENT;

	/* first associate by name */
	for(oldcomp = oldcd->firstcomponent; oldcomp != NOCOMPONENT; oldcomp = oldcomp->nextcomponent)
	{
		oldcomp->assoc = NOCOMPONENT;

		/* search for an unassociated component with the same name */
		for(newcomp = newcd->firstcomponent; newcomp != NOCOMPONENT; newcomp = newcomp->nextcomponent)
		{
			if (newcomp->assoc != NOCOMPONENT) continue;
			if (namesame(oldcomp->name, newcomp->name) != 0) continue;

			/* name association found */
			oldcomp->assoc = newcomp;
			newcomp->assoc = oldcomp;
			break;
		}
	}

	/* now associate by characteristic (in case only name changed) */
	for(oldcomp = oldcd->firstcomponent; oldcomp != NOCOMPONENT; oldcomp = oldcomp->nextcomponent)
	{
		if (oldcomp->assoc != NOCOMPONENT) continue;

		/* search for an unassociated component with the same characteristics */
		for(newcomp = newcd->firstcomponent; newcomp != NOCOMPONENT; newcomp = newcomp->nextcomponent)
		{
			if (newcomp->assoc != NOCOMPONENT) continue;
			if (newcomp->flag != oldcomp->flag) continue;
			if (newcomp->flag == 0) continue;
			if ((newcomp->flag&COMPSIZE) != 0)
			{
				if (newcomp->sizex != oldcomp->sizex || newcomp->sizey != oldcomp->sizey) continue;
			}
			if ((newcomp->flag&COMPLOC) != 0)
			{
				if (newcomp->locx != oldcomp->locx || newcomp->locy != oldcomp->locy) continue;
			}
			if ((newcomp->flag&COMPROT) != 0)
			{
				if (newcomp->rot != oldcomp->rot || newcomp->trans != oldcomp->trans) continue;
			}

			/* characteristic association found */
			oldcomp->assoc = newcomp;
			newcomp->assoc = oldcomp;
			break;
		}
	}

	/* count the number of unassociated components */
	newun = oldun = 0;
	for(oldcomp = oldcd->firstcomponent; oldcomp != NOCOMPONENT; oldcomp = oldcomp->nextcomponent)
		if (oldcomp->assoc == NOCOMPONENT) oldun++;
	for(newcomp = newcd->firstcomponent; newcomp != NOCOMPONENT; newcomp = newcomp->nextcomponent)
		if (newcomp->assoc == NOCOMPONENT) newun++;

	/* force an association if there is the same number in each category */
	if (newun == oldun && newnp == oldnp)
	{
		oldcomp = oldcd->firstcomponent;
		newcomp = newcd->firstcomponent;
		for(;;)
		{
			while (oldcomp != NOCOMPONENT && oldcomp->assoc != NOCOMPONENT)
				oldcomp = oldcomp->nextcomponent;
			while (newcomp != NOCOMPONENT && newcomp->assoc != NOCOMPONENT)
				newcomp = newcomp->nextcomponent;
			if (oldcomp == NOCOMPONENT || newcomp == NOCOMPONENT) break;
			oldcomp->assoc = newcomp;
			newcomp->assoc = oldcomp;
			oldun--;
			newun--;
			oldcomp = oldcomp->nextcomponent;
			newcomp = newcomp->nextcomponent;
		}
	}

	/* first see what was deleted */
	for(oldcomp = oldcd->firstcomponent; oldcomp != NOCOMPONENT; oldcomp = oldcomp->nextcomponent)
	{
		if (oldcomp->assoc != NOCOMPONENT) continue;
		ni = cli_findnodename(oldcomp->name);
		if (ni != NONODEINST)
		{
			cli_ownchanges = TRUE;
			cli_killthenode(ni);
			cli_ownchanges = FALSE;
		}
	}

	/* next see what was added */
	for(newcomp = newcd->firstcomponent; newcomp != NOCOMPONENT; newcomp = newcomp->nextcomponent)
	{
		if (newcomp->assoc != NOCOMPONENT) continue;

		/* first see if this name exists in the cell */
		ni = cli_findnodename(newcomp->name);
		if (ni != NONODEINST)
		{
			ttyputerr(M_("Already a component called '%s'"), newcomp->name);
			cli_restoreline(oldline, changed, win);
			cli_deletecomponentdec(oldcd);   cli_deletecomponentdec(newcd);
			return;
		}
		cli_getfactors(NONODEINST, newnp, newcomp, &lx, &hx, &ly, &hy, &rot, &trn, 0, 0);

		cli_ownchanges = TRUE;
		ni = newnodeinst(newnp, lx, hx, ly, hy, trn, rot, cli_curcell);
		if (ni != NONODEINST)
		{
			var = setvalkey((INTBIG)ni, VNODEINST, el_node_name_key, (INTBIG)newcomp->name,
				VSTRING|VDISPLAY);
			if (var != NOVARIABLE)
				defaulttextsize(3, var->textdescript);
			endobjectchange((INTBIG)ni, VNODEINST);
		}
		cli_ownchanges = FALSE;

		/* should also add attributes here!!! */
	}

	/* finally see what was changed */
	for(newcomp = newcd->firstcomponent; newcomp != NOCOMPONENT; newcomp = newcomp->nextcomponent)
	{
		oldcomp = newcomp->assoc;
		if (oldcomp == NOCOMPONENT) continue;

		/* get the component being changed */
		ni = cli_findnodename(oldcomp->name);
		if (ni == NONODEINST) continue;

		/* component changed prototype */
		if (newnp != oldnp)
		{
			cli_ownchanges = TRUE;
			startobjectchange((INTBIG)ni, VNODEINST);
			ni = replacenodeinst(ni, newnp, FALSE, FALSE);
			endobjectchange((INTBIG)ni, VNODEINST);
			cli_ownchanges = FALSE;
			if (ni == NONODEINST)
			{
				/*
				 * if replacing multiple components, earlier ones may have
				 * worked and this one failed.  Then the replacement of the
				 * original line will not be correct!!!
				 */
				ttyputerr(M_("New component type cannot be replaced"));
				cli_deletecomponentdec(oldcd);   cli_deletecomponentdec(newcd);
				cli_restoreline(oldline, changed, win);
				return;
			}
		}

		/* component changed name */
		if (namesame(oldcomp->name, newcomp->name) != 0)
		{
			if (cli_findnodename(newcomp->name) != NONODEINST)
			{
				ttyputerr(M_("Already a component called '%s'"), newcomp->name);
				cli_deletecomponentdec(oldcd);   cli_deletecomponentdec(newcd);
				cli_restoreline(oldline, changed, win);
				return;
			}
			(void)db_change((INTBIG)ni, OBJECTSTART, VNODEINST, 0, 0, 0, 0, 0);
			cli_ownchanges = TRUE;
			var = setvalkey((INTBIG)ni, VNODEINST, el_node_name_key, (INTBIG)newcomp->name,
				VSTRING|VDISPLAY);
			if (var != NOVARIABLE)
				defaulttextsize(3, var->textdescript);
			cli_ownchanges = FALSE;
			(void)db_change((INTBIG)ni, OBJECTEND, VNODEINST, 0, 0, 0, 0, 0);

			/* rewrite the name elsewhere in the text */
			cli_replacename(oldcomp->name, newcomp->name);
		}

		/* see if miscellaneous node variables changed */
		cli_updateattrs(oldcomp->firstattr, newcomp->firstattr, (INTBIG)ni, VNODEINST);

		/* component changed size/location/orientation */
		cli_getfactors(NONODEINST, oldnp, oldcomp, &oldlx, &oldhx, &oldly,
			&oldhy, &oldrot, &oldtrans, (ni->highx + ni->lowx) / 2, (ni->highy + ni->lowy) / 2);
		cli_getfactors(ni, newnp, newcomp, &newlx, &newhx, &newly, &newhy, &newrot, &newtrans, 0, 0);
		if (newlx != oldlx || newly != oldly || newhx != oldhx ||
			newhy != oldhy || newrot != oldrot || newtrans != oldtrans)
		{
			cli_ownchanges = TRUE;
			startobjectchange((INTBIG)ni, VNODEINST);
			modifynodeinst(ni, newlx - oldlx, newly - oldly, newhx - oldhx,
				newhy - oldhy, newrot - oldrot, newtrans != oldtrans);
			endobjectchange((INTBIG)ni, VNODEINST);
			cli_ownchanges = FALSE;
		}
	}
	cli_deletecomponentdec(oldcd);
	cli_deletecomponentdec(newcd);
}

/*
 * routine to handle the change of connection line "oldline" to "newline".
 * The index of the line that changed is in "changed".  Text was changed
 * in window "win".
 */
void cli_changeconnection(CHAR *oldline, CHAR *newline, INTBIG changed, WINDOWPART *win)
{
	REGISTER CONNECTION *oldcon, *newcon;
	REGISTER ARCINST *ai, *newai;
	ARCPROTO *newap, *oldap;
	NODEINST *oldend1, *oldend2, *newend1, *newend2;
	PORTPROTO *oldport1, *oldport2, *newport1, *newport2;
	REGISTER INTBIG newxoff, newyoff, lambda;
	REGISTER VARIABLE *var;
	REGISTER BOOLEAN resolve, offchanged;
	REGISTER INTBIG variable, varx, vary;
	INTBIG x1, y1, x2, y2, widnew, widold;
	BOOLEAN ret;
	REGISTER CONS *oldcons, *newcons;

	/* parse the old line */
	oldcon = cli_parseconn(oldline, FALSE);
	if (oldcon == NOCONNECTION) return;

	/* parse the new line */
	newcon = cli_parseconn(newline, FALSE);

	/* if the new line has an error, restore the old line */
	if (newcon == NOCONNECTION)
	{
		cli_restoreline(oldline, changed, win);
		cli_deleteconnection(oldcon);
		return;
	}

	/* get the new and old wire types */
	ai = cli_findarcname(oldline);
	if (ai == NOARCINST)
	{
		ttyputerr(M_("Cannot figure out which wire changed"));
		cli_restoreline(oldline, changed, win);
		cli_deleteconnection(oldcon);   cli_deleteconnection(newcon);
		return;
	}

	/* evaluate the elements of the former wire */
	if (cli_pickwire(oldcon, &oldend1, &oldport1, &oldend2, &oldport2, &oldap,
		&widold, ai, NONODEINST, NONODEINST))
	{
		cli_restoreline(oldline, changed, win);
		cli_deleteconnection(oldcon);   cli_deleteconnection(newcon);
		return;
	}

	/* evaluate the elements of the former wire */
	if (cli_pickwire(newcon, &newend1, &newport1, &newend2, &newport2, &newap,
		&widnew, NOARCINST, oldend1, oldend2))
	{
		cli_restoreline(oldline, changed, win);
		cli_deleteconnection(oldcon);   cli_deleteconnection(newcon);
		return;
	}

	/* initially assume that constraints do not need to be re-solved */
	resolve = FALSE;

	/* determine the correct extent of the arc */
	if ((newcon->flag&OFFSETVALID) != 0)
	{
		newxoff = newcon->xoff;
		newyoff = newcon->yoff;
	} else
	{
		newxoff = ai->end[1].xpos - ai->end[0].xpos;
		newyoff = ai->end[1].ypos - ai->end[0].ypos;

		/* if offsets were deleted, re-solve circuit */
		if ((oldcon->flag&OFFSETVALID) != 0) resolve = TRUE;
	}

	/* associate the constraint lists */
	for(newcons = newcon->firstcons; newcons != NOCONS; newcons = newcons->nextcons)
		newcons->assoc = NOCONS;

	/* associate by all factors */
	for(oldcons = oldcon->firstcons; oldcons != NOCONS; oldcons = oldcons->nextcons)
	{
		oldcons->assoc = NOCONS;

		/* search for an unassociated constraint with the same characteristics */
		for(newcons = newcon->firstcons; newcons != NOCONS; newcons = newcons->nextcons)
		{
			if (newcons->assoc != NOCONS) continue;
			if (namesame(newcons->direction, oldcons->direction) != 0) continue;
			if (newcons->amount != oldcons->amount) continue;
			if (newcons->flag != oldcons->flag) continue;

			/* total association found */
			oldcons->assoc = newcons;
			newcons->assoc = oldcons;
			break;
		}
	}

	/* rename the nodes if the new names cannot be found */
	if (namesame(newcon->end1, oldcon->end1) != 0 && oldend1 == newend1)
	{
		if (cli_findnodename(newcon->end1) != NONODEINST)
		{
			ttyputerr(M_("Already a component called '%s'"), newcon->end1);
			cli_restoreline(oldline, changed, win);
			cli_deleteconnection(oldcon);   cli_deleteconnection(newcon);
			return;
		}

		/* make the change */
		(void)db_change((INTBIG)oldend1, OBJECTSTART, VNODEINST, 0, 0, 0, 0, 0);
		cli_ownchanges = TRUE;
		var = setvalkey((INTBIG)oldend1, VNODEINST, el_node_name_key, (INTBIG)newcon->end1,
			VSTRING|VDISPLAY);
		if (var != NOVARIABLE)
			defaulttextsize(3, var->textdescript);
		cli_ownchanges = FALSE;
		(void)db_change((INTBIG)oldend1, OBJECTEND, VNODEINST, 0, 0, 0, 0, 0);

		/* replace the name wherever it appears */
		cli_replacename(oldcon->end1, newcon->end1);
	}
	if (namesame(newcon->end2, oldcon->end2) != 0 && oldend2 == newend2)
	{
		if (cli_findnodename(newcon->end2) != NONODEINST)
		{
			ttyputerr(M_("Already a component called '%s'"), newcon->end2);
			cli_restoreline(oldline, changed, win);
			cli_deleteconnection(oldcon);   cli_deleteconnection(newcon);
			return;
		}

		/* make the change */
		(void)db_change((INTBIG)oldend2, OBJECTSTART, VNODEINST, 0, 0, 0, 0, 0);
		cli_ownchanges = TRUE;
		var = setvalkey((INTBIG)oldend2, VNODEINST, el_node_name_key, (INTBIG)newcon->end2,
			VSTRING|VDISPLAY);
		if (var != NOVARIABLE)
			defaulttextsize(3, var->textdescript);
		cli_ownchanges = FALSE;
		(void)db_change((INTBIG)oldend2, OBJECTEND, VNODEINST, 0, 0, 0, 0, 0);

		/* replace the name wherever it appears */
		cli_replacename(oldcon->end2, newcon->end2);
	}

	/* recreate the arc if an end or port changed */
	if (oldend1 != newend1 || oldport1 != newport1 || oldend2 != newend2 || oldport2 != newport2)
	{
		portposition(newend1, newport1, &x1, &y1);
		portposition(newend2, newport2, &x2, &y2);
		cli_ownchanges = TRUE;
		newai = newarcinst(newap, widnew, 0, newend1, newport1, x1, y1,
			newend2, newport2, x2, y2, cli_curcell);
		cli_ownchanges = FALSE;
		if (newai == NOARCINST)
		{
			ttyputerr(M_("Cannot create this new wire"));
			cli_restoreline(oldline, changed, win);
			cli_deleteconnection(oldcon);   cli_deleteconnection(newcon);
			return;
		}
		endobjectchange((INTBIG)newai, VARCINST);
		cli_ownchanges = TRUE;
		startobjectchange((INTBIG)ai, VARCINST);
		(void)killarcinst(ai);
		cli_ownchanges = FALSE;
		ai = newai;
		resolve = TRUE;

		/* should set attributes and copy lists so that they are equal!!! */

		/* equate the factors that will now be correct */
		oldap = newap;
		widold = widnew;
		for(oldcons = oldcon->firstcons; oldcons != NOCONS; oldcons = oldcons->nextcons)
			oldcons->assoc = oldcons;
		for(newcons = newcon->firstcons; newcons != NOCONS; newcons = newcons->nextcons)
			newcons->assoc = NOCONS;
	}

	/* see if miscellaneous arc variables changed */
	cli_updateattrs(oldcon->firstattr, newcon->firstattr, (INTBIG)ai, VARCINST);

	/* replace the arc type if that was requested */
	if (oldap != newap)
	{
		cli_ownchanges = TRUE;
		startobjectchange((INTBIG)ai, VARCINST);
		ai = replacearcinst(ai, newap);
		endobjectchange((INTBIG)ai, VARCINST);
		cli_ownchanges = FALSE;
		if (ai == NOARCINST)
		{
			ttyputerr(M_("Cannot change the type of this wire"));
			cli_restoreline(oldline, changed, win);
			cli_deleteconnection(oldcon);   cli_deleteconnection(newcon);
			return;
		}

		/* if width not specified, ensure that default is used for this wire */
		if ((newcon->flag&WIDTHVALID) == 0 && ai->width != newap->nominalwidth)
			widnew = newap->nominalwidth;
	}

	/* change the arc width if that was requested */
	if (widold != widnew)
	{
		cli_ownchanges = TRUE;
		startobjectchange((INTBIG)ai, VARCINST);
		ret = modifyarcinst(ai, widnew-widold, 0, 0, 0, 0);
		endobjectchange((INTBIG)ai, VARCINST);
		cli_ownchanges = FALSE;
		if (ret)
		{
			ttyputerr(M_("Cannot change the width of this wire"));
			cli_restoreline(oldline, changed, win);
			cli_deleteconnection(oldcon);   cli_deleteconnection(newcon);
			return;
		}
	}

	/* see which constraints were deleted */
	for(oldcons = oldcon->firstcons; oldcons != NOCONS; oldcons = oldcons->nextcons)
	{
		if (oldcons->assoc != NOCONS) continue;
		resolve = TRUE;
		if (namesame(oldcons->direction, x_("left")) == 0) variable = CLLEFT; else
			if (namesame(oldcons->direction, x_("right")) == 0) variable = CLRIGHT; else
				if (namesame(oldcons->direction, x_("down")) == 0) variable = CLDOWN; else
					if (namesame(oldcons->direction, x_("up")) == 0) variable = CLUP;
		cli_ownchanges = TRUE;
		cli_deletearcconstraint(ai, variable, oldcons->flag, oldcons->amount);
		cli_ownchanges = FALSE;
	}

	/* see which constraints were added */
	for(newcons = newcon->firstcons; newcons != NOCONS; newcons = newcons->nextcons)
	{
		if (newcons->assoc != NOCONS) continue;
		resolve = TRUE;
		if (namesame(newcons->direction, x_("left")) == 0) variable = CLLEFT; else
			if (namesame(newcons->direction, x_("right")) == 0) variable = CLRIGHT; else
				if (namesame(newcons->direction, x_("down")) == 0) variable = CLDOWN; else
					if (namesame(newcons->direction, x_("up")) == 0) variable = CLUP;
		cli_ownchanges = TRUE;
		(void)cli_addarcconstraint(ai, variable, newcons->flag, newcons->amount, 1);
		cli_ownchanges = FALSE;
	}

	/* free the parsing structures */
	cli_deleteconnection(oldcon);   cli_deleteconnection(newcon);

	/* add temporary forcing constraints if the offset was changed */
	offchanged = FALSE;
	if (newxoff != ai->end[1].xpos - ai->end[0].xpos ||
		newyoff != ai->end[1].ypos - ai->end[0].ypos)
	{
		offchanged = TRUE;
		resolve = TRUE;

		/* compute the constraint in X */
		if (newxoff >= 0) varx = CLRIGHT; else
		{
			newxoff = -newxoff;
			varx = CLLEFT;
		}
		lambda = lambdaofarc(ai);
		newxoff = newxoff * WHOLE / lambda;

		/* compute the constraint in Y */
		if (newyoff >= 0) vary = CLUP; else
		{
			newyoff = -newyoff;
			vary = CLDOWN;
		}
		newyoff = newyoff * WHOLE / lambda;

		/* set the new constraints */
		(void)cli_addarcconstraint(ai, varx, CLEQUALS, newxoff, 1);
		(void)cli_addarcconstraint(ai, vary, CLEQUALS, newyoff, 1);
	}

	/* make sure all constraints are met */
	if (resolve) cli_solvecell(cli_curcell, FALSE, TRUE);

	/* remove temporary constraints to handle offset specification */
	if (offchanged)
	{
		(void)cli_deletearcconstraint(ai, varx, CLEQUALS, newxoff);
		(void)cli_deletearcconstraint(ai, vary, CLEQUALS, newyoff);
	}
}

/*
 * routine to update the list of attributes from "oldlist" to "newlist"
 * on object "addr" of type "type"
 */
void cli_updateattrs(ATTR *oldlist, ATTR *newlist, INTBIG addr, INTBIG type)
{
	REGISTER ATTR *a, *b;

	/* stop now if both lists are empty */
	if (oldlist == NOATTR && newlist == NOATTR) return;

	/* clear associations */
	for(a = oldlist; a != NOATTR; a = a->nextattr) a->equiv = NOATTR;
	for(a = newlist; a != NOATTR; a = a->nextattr) a->equiv = NOATTR;

	for(a = oldlist; a != NOATTR; a = a->nextattr)
	{
		for(b = newlist; b != NOATTR; b = b->nextattr)
		{
			if (namesame(a->name, b->name) != 0) continue;
			a->equiv = b;
			b->equiv = a;
		}
	}

	/* look for the deleted attributes */
	for(a = oldlist; a != NOATTR; a = a->nextattr)
		if (a->equiv == NOATTR)
	{
		cli_ownchanges = TRUE;
		(void)delval(addr, type, a->name);
		cli_ownchanges = FALSE;
	}

	/* look for the added attributes */
	for(a = newlist; a != NOATTR; a = a->nextattr)
		if (a->equiv == NOATTR)
	{
		cli_ownchanges = TRUE;
		(void)setval(addr, type, a->name, a->value, a->type);
		cli_ownchanges = FALSE;
	}

	/* look for changed attributes */
	for(a = oldlist; a != NOATTR; a = a->nextattr)
	{
		b = a->equiv;
		if (b == NOATTR) continue;
		if (a->type == b->type)
		{
			if ((a->type&VTYPE) == VINTEGER)
			{
				if (a->value == b->value) continue;
			} else if ((a->type&VTYPE) == VSTRING)
			{
				if (namesame((CHAR *)a->value, (CHAR *)b->value) == 0) continue;
			}
		}
		cli_ownchanges = TRUE;
		(void)delval(addr, type, a->name);
		(void)setval(addr, type, b->name, b->value, b->type);
		cli_ownchanges = FALSE;
	}
}

/***************************** TEXT DELETION *****************************/

/*
 * routine to delete the graphics associated with the declaration in line
 * "oldline"
 */
void cli_deletedeclaration(CHAR *oldline)
{
	REGISTER COMPONENTDEC *dec;
	REGISTER COMPONENT *compo;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;

	/* parse the declaration statement */
	dec = cli_parsecomp(oldline, FALSE);
	if (dec == NOCOMPONENTDEC) return;

	/* determine the prototype of the node */
	np = getnodeproto(dec->protoname);
	if (np == NONODEPROTO)
	{
		ttyputerr(M_("Unknown component type: %s"), dec->protoname);
		cli_deletecomponentdec(dec);
		return;
	}

	/* check each component in the declaration */
	for(compo = dec->firstcomponent; compo != NOCOMPONENT; compo = compo->nextcomponent)
	{
		/* get the node that is to be deleted */
		ni = cli_findnodename(compo->name);
		if (ni == NONODEINST) continue;

		/* delete all arcs on that node */
		cli_killthenode(ni);
	}
	cli_deletecomponentdec(dec);
}

/*
 * routine to delete the graphics associated with the connection in line
 * "oldline"
 */
void cli_deletewire(CHAR *oldline)
{
	REGISTER ARCINST *ai;

	ai = cli_findarcname(oldline);
	if (ai == NOARCINST) return;
	cli_ownchanges = TRUE;
	startobjectchange((INTBIG)ai, VARCINST);
	(void)killarcinst(ai);
	cli_ownchanges = FALSE;
}

/*
 * routine to delete the graphics associated with the export line "oldline"
 */
void cli_deleteexportline(CHAR *oldline)
{
	REGISTER EXPORT *e;
	REGISTER PORTPROTO *pp;

	/* parse the export statement */
	e = cli_parseexport(oldline, FALSE);
	if (e == NOEXPORT) return;

	/* find the exported port name */
	pp = getportproto(cli_curcell, e->portname);
	if (pp == NOPORTPROTO)
	{
		ttyputerr(M_("Unknown port: %s"), e->portname);
		cli_deleteexport(e);
		return;
	}

	/* remove the port prototype */
	cli_ownchanges = TRUE;
	(void)killportproto(cli_curcell, pp);
	cli_ownchanges = FALSE;
	cli_deleteexport(e);
}

/***************************** TEXT ADDTION *****************************/

/*
 * routine to add graphics equivalent to the new text declaration "newline"
 * with the line index "changed".  Text was changed in window "win".  Returns
 * true on error
 */
BOOLEAN cli_adddeclaration(CHAR *newline, INTBIG changed, WINDOWPART *win)
{
	REGISTER COMPONENTDEC *dec;
	REGISTER COMPONENT *compo, *ocomp;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var;
	INTBIG lx, hx, ly, hy;
	INTBIG rot, trn;

	/* parse the declaration statement */
	dec = cli_parsecomp(newline, FALSE);
	if (dec == NOCOMPONENTDEC)
	{
		cli_commentline(changed, win);
		return(TRUE);
	}

	/* determine the prototype being created */
	np = getnodeproto(dec->protoname);
	if (np == NONODEPROTO)
	{
		ttyputerr(M_("Unknown component type: %s"), dec->protoname);
		cli_commentline(changed, win);
		cli_deletecomponentdec(dec);
		return(TRUE);
	}

	/* check for duplicate component names */
	for(compo = dec->firstcomponent; compo != NOCOMPONENT; compo = compo->nextcomponent)
	{
		/* first see if this name exists in the cell */
		ni = cli_findnodename(compo->name);
		if (ni != NONODEINST)
		{
			ttyputerr(M_("Already a component called '%s'"), compo->name);
			cli_commentline(changed, win);
			cli_deletecomponentdec(dec);
			return(TRUE);
		}

		/* now see if the name is duplicated in this statement */
		for(ocomp = dec->firstcomponent; ocomp != compo; ocomp = ocomp->nextcomponent)
		{
			if (namesame(compo->name, ocomp->name) == 0)
			{
				ttyputerr(M_("Duplicate component name '%s'"), compo->name);
				cli_commentline(changed, win);
				cli_deletecomponentdec(dec);
				return(TRUE);
			}
		}
	}

	/* create the components */
	for(compo = dec->firstcomponent; compo != NOCOMPONENT; compo = compo->nextcomponent)
	{
		/* get the size/location/orientation of the component */
		cli_getfactors(NONODEINST, np, compo, &lx, &hx, &ly, &hy, &rot, &trn, 0, 0);

		cli_ownchanges = TRUE;
		ni = newnodeinst(np, lx, hx, ly, hy, trn, rot, cli_curcell);
		if (ni != NONODEINST)
		{
			var = setvalkey((INTBIG)ni, VNODEINST, el_node_name_key, (INTBIG)compo->name,
				VSTRING|VDISPLAY);
			if (var != NOVARIABLE)
				defaulttextsize(3, var->textdescript);
			endobjectchange((INTBIG)ni, VNODEINST);
		}
		cli_ownchanges = FALSE;
	}
	cli_deletecomponentdec(dec);
	return(FALSE);
}

/*
 * routine to add graphics equivalent to the new text connection "newline"
 * with the line index "changed".  Text was changed in window "win".  Returns
 * true on error
 */
BOOLEAN cli_addwire(CHAR *newline, INTBIG changed, WINDOWPART *win)
{
	REGISTER CONNECTION *dcl;
	REGISTER CONS *cons;
	NODEINST *nA, *nB;
	PORTPROTO *pA, *pB;
	ARCPROTO *ap;
	REGISTER ARCINST *ai;
	INTBIG xA, yA, xB, yB, wid;
	REGISTER INTBIG variable;
	REGISTER BOOLEAN gothor, gotver, addedmanhattan;

	/* parse the line and get the endpoints */
	dcl = cli_parseconn(newline, FALSE);
	if (dcl == NOCONNECTION)
	{
		cli_commentline(changed, win);
		return(TRUE);
	}

	if (cli_pickwire(dcl, &nA, &pA, &nB, &pB, &ap, &wid, NOARCINST, NONODEINST, NONODEINST))
	{
		cli_commentline(changed, win);
		cli_deleteconnection(dcl);
		return(TRUE);
	}

	/* create the arc */
	portposition(nA, pA, &xA, &yA);
	portposition(nB, pB, &xB, &yB);
	cli_ownchanges = TRUE;
	ai = newarcinst(ap, wid, 0, nA,pA,xA,yA, nB,pB,xB,yB, cli_curcell);
	cli_ownchanges = FALSE;
	if (ai == NOARCINST)
	{
		ttyputerr(M_("Cannot make the wire"));
		cli_commentline(changed, win);
		cli_deleteconnection(dcl);
		return(TRUE);
	}

	/* set constraints on the arc */
	gothor = gotver = FALSE;
	for(cons = dcl->firstcons; cons != NOCONS; cons = cons->nextcons)
	{
		if (namesame(cons->direction, x_("left")) == 0)
		{
			variable = CLLEFT;
			gothor = TRUE;
		} else if (namesame(cons->direction, x_("right")) == 0)
		{
			variable = CLRIGHT;
			gothor = TRUE;
		} else if (namesame(cons->direction, x_("down")) == 0)
		{
			variable = CLDOWN;
			gotver = TRUE;
		} else if (namesame(cons->direction, x_("up")) == 0)
		{
			variable = CLUP;
			gotver = TRUE;
		}
		cli_ownchanges = TRUE;
		(void)cli_addarcconstraint(ai, variable, cons->flag, cons->amount, 1);
		cli_ownchanges = FALSE;
	}
	cli_deleteconnection(dcl);
	endobjectchange((INTBIG)ai, VARCINST);

	/* add in manhattan constraints (temporarily) if applicable */
	addedmanhattan = FALSE;
	if (((!gothor && gotver) || (gothor && !gotver)) && cli_manhattan)
	{
		if (!gothor) variable = CLLEFT; else variable = CLDOWN;
	}

	/* make sure all constraints are met */
	cli_solvecell(cli_curcell, FALSE, TRUE);

	/* remove temporary manhattan constraint if it was added */
	if (addedmanhattan)
		(void)cli_deletearcconstraint(ai, variable, CLEQUALS, 0);
	return(FALSE);
}

/*
 * routine to add graphics equivalent to the new text connection "newline"
 * with the line index "changed".  Text was changed in window "win".
 */
void cli_addexport(CHAR *newline, INTBIG changed, WINDOWPART *win)
{
	REGISTER EXPORT *e;
	REGISTER PORTPROTO *pp, *newpp;
	REGISTER NODEINST *ni;

	/* parse the export statement */
	e = cli_parseexport(newline, FALSE);
	if (e == NOEXPORT)
	{
		cli_commentline(changed, win);
		return;
	}

	/* make sure the export is unique */
	pp = getportproto(cli_curcell, e->portname);
	if (pp != NOPORTPROTO)
	{
		ttyputerr(M_("Already an export called %s"), pp->protoname);
		cli_commentline(changed, win);
		cli_deleteexport(e);
		return;
	}

	/* find the subcomponent */
	ni = cli_findnodename(e->component);
	if (ni == NONODEINST)
	{
		ttyputerr(M_("Cannot find node %s"), e->component);
		cli_commentline(changed, win);
		cli_deleteexport(e);
		return;
	}

	/* find the port on that subcomponent */
	pp = cli_getexportsubport(e, ni);
	if (pp == NOPORTPROTO)
	{
		cli_commentline(changed, win);
		cli_deleteexport(e);
		return;
	}

	/* create the port */
	cli_ownchanges = TRUE;
	newpp = newportproto(cli_curcell, ni, pp, e->portname);
	cli_ownchanges = FALSE;
	if (newpp == NOPORTPROTO)
	{
		ttyputerr(M_("Could not create the port"));
		cli_commentline(changed, win);
		cli_deleteexport(e);
		return;
	}
}

PORTPROTO *cli_getexportsubport(EXPORT *e, NODEINST *ni)
{
	REGISTER PORTPROTO *pp;

	if (e->subport != 0)
	{
		pp = getportproto(ni->proto, e->subport);
		if (pp == NOPORTPROTO)
			ttyputerr(M_("No such port %s on node %s"), e->subport, e->component);
	} else
	{
		pp = ni->proto->firstportproto;
		if (!cli_uniqueport(ni, pp))
		{
			ttyputerr(M_("Ambiguous port on node %s"), e->component);
			pp = NOPORTPROTO;
		}
	}
	return(pp);
}

/***************************** UTILITIES *****************************/

/*
 * routine to obtain the extent of node "ni", type "np", described by the
 * component structure "compo" and place it in the reference parameters "lx",
 * "hx", "ly", "hy", "rot", and "trn".  If "ni" is NONODEINST, do not presume
 * any defaults (and use "defcx/defcy" as the default location).
 */
void cli_getfactors(NODEINST *ni, NODEPROTO *np, COMPONENT *compo, INTBIG *lx,
	INTBIG *hx, INTBIG *ly, INTBIG *hy, INTBIG *rot, INTBIG *trn, INTBIG defcx, INTBIG defcy)
{
	REGISTER INTBIG xc, yc, xs, ys;
	INTBIG plx, phx, ply, phy;

	/* get the size of the component */
	if ((compo->flag&COMPSIZE) != 0)
	{
		xs = compo->sizex;
		ys = compo->sizey;
	} else
	{
		if (ni != NONODEINST)
		{
			xs = ni->highx - ni->lowx;
			ys = ni->highy - ni->lowy;
		} else
		{
			xs = np->highx - np->lowx;
			ys = np->highy - np->lowy;
		}
	}

	/* get the location of the component */
	if ((compo->flag&COMPLOC) != 0)
	{
		xc = compo->locx;
		yc = compo->locy;
	} else
	{
		if (ni != NONODEINST)
		{
			xc = (ni->highx + ni->lowx) / 2;
			yc = (ni->highy + ni->lowy) / 2;
		} else
		{
			xc = defcx;
			yc = defcy;
		}
	}

	/* compute the bounding box */
	*lx = xc - xs/2;   *hx = *lx + xs;
	*ly = yc - ys/2;   *hy = *ly + ys;

	/* adjust the size if it was taken from the specification */
	if ((compo->flag&COMPSIZE) != 0)
	{
		nodesizeoffset(ni, &plx, &ply, &phx, &phy);
		*lx -= plx;   *hx += phx;
		*ly -= ply;   *hy += phy;
	}

	/* get the rotation/transpose information */
	if ((compo->flag&COMPROT) != 0)
	{
		*rot = compo->rot;
		*trn = compo->trans;
	} else
	{
		if (ni != NONODEINST)
		{
			*rot = ni->rotation;
			*trn = ni->transpose;
		} else *rot = *trn = 0;
	}
}

/*
 * routine to delete node "ni" and all arcs associated with it
 */
void cli_killthenode(NODEINST *ni)
{
	REGISTER PORTARCINST *pi, *nextpi;
	REGISTER ARCINST *ai;
	REGISTER INTBIG i;
	REGISTER NODEINST *oni;

	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = nextpi)
	{
		nextpi = pi->nextportarcinst;
		ai = pi->conarcinst;
		if ((ai->userbits&DEADA) != 0) continue;
		cli_deleteequivcon(ai);
		for(i=0; i<2; i++)
		{
			oni = ai->end[i].nodeinst;
			if (oni == ni) continue;
			if (oni->firstportarcinst == NOPORTARCINST) cli_changeequivcomp(oni);
		}
		cli_ownchanges = TRUE;
		startobjectchange((INTBIG)ai, VARCINST);
		(void)killarcinst(ai);
		cli_ownchanges = FALSE;
	}

	/* delete the node */
	cli_ownchanges = TRUE;
	startobjectchange((INTBIG)ni, VNODEINST);
	(void)killnodeinst(ni);
	cli_ownchanges = FALSE;
}

/*
 * routine to insert a comment to remove the text line with the index
 * "cindex".  Text was changed in window "win".
 */
void cli_commentline(INTBIG cindex, WINDOWPART *win)
{
	REGISTER void *infstr;

	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("; "));
	addstringtoinfstr(infstr, (CHAR *)asktool(us_tool, x_("edit-getline"), (INTBIG)win, cindex));
	(void)asktool(us_tool, x_("edit-replaceline"), (INTBIG)win, cindex, (INTBIG)returninfstr(infstr));
}

/*
 * routine to comment line "cindex" and restore line "str" of text.  Text
 * was changed in window "win".
 */
void cli_restoreline(CHAR *str, INTBIG cindex, WINDOWPART *win)
{
	REGISTER void *infstr;

	/* first insert this old line */
	(void)asktool(us_tool, x_("edit-addline"), (INTBIG)win, cindex+1, (INTBIG)str);

	/* now comment out line "cindex" */
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("; "));
	addstringtoinfstr(infstr, (CHAR *)asktool(us_tool, x_("edit-getline"), (INTBIG)win, cindex));
	(void)asktool(us_tool, x_("edit-replaceline"), (INTBIG)win, cindex, (INTBIG)returninfstr(infstr));
}

void cli_replaceendcell(CHAR *str, WINDOWPART *win)
{
	REGISTER INTBIG i, maxlines, type;

	maxlines = asktool(us_tool, x_("edit-totallines"), (INTBIG)win);

	/* search for all other "begincell" lines */
	for(i = 0; i < maxlines; i++)
	{
		type = cli_linetype((CHAR *)asktool(us_tool, x_("edit-getline"), (INTBIG)win, i));
		if (type != LINEEND) continue;

		/* found another "begincell", delete the text */
		(void)asktool(us_tool, x_("edit-deleteline"), (INTBIG)win, i);
		i--;
		maxlines--;
	}

	/* place this line at the beginning */
	(void)asktool(us_tool, x_("edit-addline"), (INTBIG)win, maxlines, (INTBIG)str);
}
