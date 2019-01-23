/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrcomrs.c
 * User interface tool: command handler for R through S
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
#include "usrtrack.h"
#include "efunction.h"
#include "conlay.h"
#include "tecgen.h"
#include "usrdiacom.h"

void us_redraw(INTBIG count, CHAR *par[])
{
	Q_UNUSED( count );
	Q_UNUSED( par );

	/* save highlighting */
	us_pushhighlight();
	us_clearhighlightcount();

	/* re-draw the status display */
	us_redostatus(NOWINDOWFRAME);

	/* redraw the color screen */
	us_drawmenu(0, NOWINDOWFRAME);

	/* restore highlighting */
	us_pophighlight(FALSE);

	/* flush queued display */
	us_endchanges(NOWINDOWPART);
}

void us_remember(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG i, newtotal;
	REGISTER CHAR **newpar;
	REGISTER USERCOM *uc;

	if (count > us_lastcommandtotal)
	{
		newtotal = us_lastcommandtotal * 2;
		if (count > us_lastcommandtotal) newtotal = count + 4;
		newpar = (CHAR **)emalloc(newtotal * (sizeof (CHAR *)), us_tool->cluster);
		if (newpar == 0) return;
		for(i=0; i<us_lastcommandtotal; i++)
			newpar[i] = us_lastcommandpar[i];
		for(i=us_lastcommandtotal; i<newtotal; i++) newpar[i] = 0;
		if (us_lastcommandtotal > 0) efree((CHAR *)us_lastcommandpar);
		us_lastcommandpar = newpar;
		us_lastcommandtotal = newtotal;
	}
	for(i=0; i<count; i++)
	{
		if (us_lastcommandpar[i] != 0)
			efree((CHAR *)us_lastcommandpar[i]);
		(void)allocstring(&us_lastcommandpar[i], par[i], us_tool->cluster);
	}
	us_lastcommandcount = count;

	/* and execute the command, too */
	uc = us_buildcommand(count, par);
	if (uc == NOUSERCOM) return;
	us_executesafe(uc, FALSE, FALSE, FALSE);
	us_freeusercom(uc);
}

void us_rename(INTBIG count, CHAR *par[])
{
	CHAR prompt[80], newfile[100],*newpar[10], *oldname, si[10], sj[10];
	REGISTER INTBIG k, len, i, savei, command, savecommand, variable, savevariable,
		varnewkey, varoldkey;
	REGISTER CHAR *ch, *pt, *netname, *savenetname, **newlist, *str, *which;
	REGISTER NODEPROTO *np, *lnt, *savenp, *curcell;
	REGISTER NODEINST *ni;
	REGISTER PORTPROTO *pp, *savepp;
	REGISTER LIBRARY *lib, *olib, *savelib;
	REGISTER TECHNOLOGY *tech, *otech, *savetech;
	REGISTER ARCPROTO *ap, *oat, *saveap;
	REGISTER VARIABLE *macvar, *savemacvar, *var;
	REGISTER POPUPMENU *opm;
	POPUPMENU *pm, *savepm;
	REGISTER NETWORK *net;
	REGISTER USERCOM *item;
	COMMANDBINDING commandbinding;
	REGISTER void *infstr;

	/* get the former name */
	if (count < 2)
	{
		ttyputusage(x_("rename OLDNAME NEWNAME [TYPE]"));
		return;
	}
	pt = par[0];
	curcell = getcurcell();

	/* see if any nodeprotos have that name */
	np = getnodeproto(pt);

	/* see if any arcprotos have that name */
	ap = getarcproto(pt);

	/* see if any libraries have that name */
	lib = getlibrary(pt);

	/* see if any ports in this cell have that name */
	if (curcell == NONODEPROTO) pp = NOPORTPROTO; else
		pp = getportproto(curcell, pt);

	/* see if any macros have that name */
	macvar = us_getmacro(pt);

	/* see if any technologies have that name (search by hand because "gettechnology" handles partial matches) */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		if (namesame(pt, tech->techname) == 0) break;

	/* see if any user commands have that name */
	for(i=0; us_lcommand[i].name != 0; i++)
		if (namesame(us_lcommand[i].name, pt) == 0) break;
	if (us_lcommand[i].name == 0) command = -1; else command = i;

	/* see if any database variables have that name */
	for(i=0; i<el_numnames; i++)
		if (namesame(el_namespace[i], pt) == 0) break;
	if (i >= el_numnames) variable = -1; else variable = i;

	/* see if any popup menus have that name */
	for(pm = us_firstpopupmenu; pm != NOPOPUPMENU; pm = pm->nextpopupmenu)
		if (namesame(pm->name, pt) == 0) break;

	/* see if any networks have that name */
	netname = 0;
	net = NONETWORK;
	if (curcell != NONODEPROTO)
	{
		len = estrlen(pt);
		for(net = curcell->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		{
			for(k=0; k<net->namecount; k++)
			{
				ch = networkname(net, k);
				if (namesamen(ch, pt, len) == 0 && (ch[len] == '[' || ch[len] == 0))
				{
					netname = pt;
					break;
				}
			}
			if (netname != 0) break;
		}
	}

	/* special case: if port and network are possible, exclude the network */
	if (pp != NOPORTPROTO && netname != 0) netname = 0;

	/* see how many different things have that name */
	i = 0;
	if (np != NONODEPROTO) i++;
	if (ap != NOARCPROTO) i++;
	if (lib != NOLIBRARY) i++;
	if (pp != NOPORTPROTO) i++;
	if (macvar != NOVARIABLE) i++;
	if (tech != NOTECHNOLOGY) i++;
	if (command >= 0) i++;
	if (variable >= 0) i++;
	if (pm != NOPOPUPMENU) i++;
	if (netname != 0) i++;

	/* quit if the name doesn't exist */
	if (i == 0)
	{
		us_abortcommand(_("Nothing named %s"), pt);
		return;
	}

	/* if name matches more than one type, exclude matches that are not exact */
	if (i != 1)
	{
		savenp = np;
		saveap = ap;
		savelib = lib;
		savepp = pp;
		savemacvar = macvar;
		savetech = tech;
		savecommand = command;
		savevariable = variable;
		savepm = pm;
		savenetname = netname;
		savei = i;

		if (np != NONODEPROTO && namesame(np->protoname, pt) != 0)
		{
			np = NONODEPROTO;   i--;
		}
		if (ap != NOARCPROTO && namesame(ap->protoname, pt) != 0)
		{
			ap = NOARCPROTO;   i--;
		}
		if (lib != NOLIBRARY && namesame(lib->libname, pt) != 0)
		{
			lib = NOLIBRARY;   i--;
		}
		if (pp != NOPORTPROTO && namesame(pp->protoname, pt) != 0)
		{
			pp = NOPORTPROTO;   i--;
		}
		if (macvar != NOVARIABLE && namesame(&makename(macvar->key)[11], pt) != 0)
		{
			macvar = NOVARIABLE;   i--;
		}
		if (tech != NOTECHNOLOGY && namesame(tech->techname, pt) != 0)
		{
			tech = NOTECHNOLOGY;   i--;
		}
		if (command >= 0 && namesame(us_lcommand[command].name, pt) != 0)
		{
			command = -1;   i--;
		}
		if (variable >= 0 && namesame(el_namespace[variable], pt) != 0)
		{
			variable = -1;   i--;
		}
		if (pm != NOPOPUPMENU && namesame(pm->name, pt) != 0)
		{
			pm = NOPOPUPMENU;   i--;
		}
		if (netname != 0 && namesame(netname, pt) != 0)
		{
			netname = 0;   i--;
		}

		if (i <= 0)
		{
			np = savenp;
			ap = saveap;
			lib = savelib;
			pp = savepp;
			macvar = savemacvar;
			tech = savetech;
			command = savecommand;
			variable = savevariable;
			pm = savepm;
			netname = savenetname;
			i = savei;
		}
	}

	/* build the ambiguity string */
	(void)estrcpy(prompt, M_("Rename the"));
	i = 0;
	if (np != NONODEPROTO)
	{
		(void)estrcat(prompt, M_(" nodeProto")); i++;
	}
	if (ap != NOARCPROTO)
	{
		if (i) (void)estrcat(prompt, M_(" or"));
		(void)estrcat(prompt, M_(" Arc")); i++;
	}
	if (lib != NOLIBRARY)
	{
		if (i) (void)estrcat(prompt, M_(" or"));
		(void)estrcat(prompt, M_(" Library")); i++;
	}
	if (pp != NOPORTPROTO)
	{
		if (i) (void)estrcat(prompt, M_(" or"));
		(void)estrcat(prompt, M_(" poRt")); i++;
	}
	if (macvar != NOVARIABLE)
	{
		if (i) (void)estrcat(prompt, M_(" or"));
		(void)estrcat(prompt, M_(" Macro")); i++;
	}
	if (tech != NOTECHNOLOGY)
	{
		if (i) (void)estrcat(prompt, M_(" or"));
		(void)estrcat(prompt, M_(" Technology")); i++;
	}
	if (command >= 0)
	{
		if (i) (void)estrcat(prompt, M_(" or"));
		(void)estrcat(prompt, M_(" commanD")); i++;
	}
	if (variable >= 0)
	{
		if (i) (void)estrcat(prompt, M_(" or"));
		(void)estrcat(prompt, M_(" Variable")); i++;
	}
	if (pm != NOPOPUPMENU)
	{
		if (i) (void)estrcat(prompt, M_(" or"));
		(void)estrcat(prompt, M_(" pop-Up-menu")); i++;
	}
	if (netname != 0)
	{
		if (i) (void)estrcat(prompt, M_(" or"));
		(void)estrcat(prompt, M_(" Network")); i++;
	}
	(void)estrcat(prompt, x_(": "));

	/* if name is more than one type of object, ask which */
	while (i != 1)
	{
		if (count >= 3)
		{
			which = par[2];
			count = 2;
		} else
		{
			which = ttygetline(prompt);
			if (which == 0) return;
		}
		switch (*which)
		{
			case 'p':   case 'P':
				ap=NOARCPROTO;     lib=NOLIBRARY;     pp=NOPORTPROTO;
				macvar=NOVARIABLE; tech=NOTECHNOLOGY; command = -1;
				variable = -1;     pm = NOPOPUPMENU;
				netname = 0;       i=1;               break;
			case 'a':   case 'A':
				np=NONODEPROTO;    lib=NOLIBRARY;     pp=NOPORTPROTO;
				macvar=NOVARIABLE; tech=NOTECHNOLOGY; command = -1;
				variable = -1;     pm = NOPOPUPMENU;
				netname = 0;       i=1;               break;
			case 'l':   case 'L':
				ap=NOARCPROTO;     np=NONODEPROTO;    pp=NOPORTPROTO;
				macvar=NOVARIABLE; tech=NOTECHNOLOGY; command = -1;
				variable = -1;     pm = NOPOPUPMENU;
				netname = 0;       i=1;               break;
			case 'r':   case 'R':
				ap=NOARCPROTO;     lib=NOLIBRARY;     np=NONODEPROTO;
				macvar=NOVARIABLE; tech=NOTECHNOLOGY; command = -1;
				variable = -1;     pm = NOPOPUPMENU;
				netname = 0;       i=1;               break;
			case 'm':   case 'M':
				ap=NOARCPROTO;     lib=NOLIBRARY;     pp=NOPORTPROTO;
				np=NONODEPROTO;    tech=NOTECHNOLOGY; command = -1;
				variable = -1;     pm = NOPOPUPMENU;
				netname = 0;       i=1;               break;
			case 't':   case 'T':
				ap=NOARCPROTO;     lib=NOLIBRARY;     pp=NOPORTPROTO;
				np=NONODEPROTO;    macvar=NOVARIABLE; command = -1;
				variable = -1;     pm = NOPOPUPMENU;
				netname = 0;       i=1;               break;
			case 'd':   case 'D':
				ap=NOARCPROTO;     lib=NOLIBRARY;     pp=NOPORTPROTO;
				np=NONODEPROTO;    tech=NOTECHNOLOGY; macvar=NOVARIABLE;
				variable = -1;     pm = NOPOPUPMENU;
				netname = 0;       i=1;               break;
			case 'v':   case 'V':
				ap=NOARCPROTO;     lib=NOLIBRARY;     pp=NOPORTPROTO;
				np=NONODEPROTO;    tech=NOTECHNOLOGY; macvar=NOVARIABLE;
				command = -1;      pm = NOPOPUPMENU;
				netname = 0;       i=1;               break;
			case 'u':   case 'U':
				ap=NOARCPROTO;     lib=NOLIBRARY;     pp=NOPORTPROTO;
				np=NONODEPROTO;    tech=NOTECHNOLOGY; macvar=NOVARIABLE;
				variable = -1;     command = -1;
				netname = 0;       i=1;               break;
			case 'n':   case 'N':
				ap=NOARCPROTO;     lib=NOLIBRARY;     pp=NOPORTPROTO;
				np=NONODEPROTO;    tech=NOTECHNOLOGY; macvar=NOVARIABLE;
				variable = -1;     command = -1;
				pm=NOPOPUPMENU;    i=1;               break;
			case 0:   us_abortedmsg();  return;
		}
	}

	/* get new name */
	pt = par[1];

	/* handle nodeproto name change */
	if (np != NONODEPROTO)
	{
		/* be sure the name is legal */
		if (!us_validname(pt, VNODEPROTO)) return;

		/* check for duplicate name */
		if (estrcmp(np->protoname, pt) == 0)
		{
			ttyputmsg(_("Nodeproto name has not changed"));
			return;
		}
		if (np->primindex != 0)
		{
			/* check for duplicate primitive name */
			for(lnt = np->tech->firstnodeproto; lnt != NONODEPROTO; lnt = lnt->nextnodeproto)
				if (np != lnt && namesame(lnt->protoname, pt) == 0)
			{
				us_abortcommand(_("Already a primitive with that name"));
				return;
			}
		} else
		{
			/* check for duplicate cell name */
			for(lnt = np->lib->firstnodeproto; lnt != NONODEPROTO; lnt = lnt->nextnodeproto)
			{
				if (np == lnt) continue;
				if (np->cellview == lnt->cellview && estrcmp(lnt->protoname, pt) == 0)
				{
					us_abortcommand(_("Already a cell with that name"));
					return;
				}
			}
		}

		/* change the node name */
		ttyputverbose(M_("Nodeproto %s renamed to %s"), np->protoname, pt);
		lnt = us_curnodeproto;
		if (lnt == np) us_setnodeproto(NONODEPROTO);
		(void)setval((INTBIG)np, VNODEPROTO, x_("protoname"), (INTBIG)pt, VSTRING);
		if (lnt == np) us_setnodeproto(np);
	}

	/* handle arcproto name change */
	if (ap != NOARCPROTO)
	{
		/* be sure the name is legal */
		if (!us_validname(pt, VARCPROTO)) return;

		/* check for duplicate name */
		if (estrcmp(ap->protoname, pt) == 0)
		{
			ttyputmsg(_("Arc name has not changed"));
			return;
		}
		for(oat = ap->tech->firstarcproto; oat != NOARCPROTO; oat = oat->nextarcproto)
			if (ap != oat && namesame(pt, oat->protoname) == 0)
		{
			us_abortcommand(_("Already an arc of that name"));
			return;
		}

		/* change the arc name */
		(void)allocstring(&oldname, ap->protoname, el_tempcluster);
		ttyputverbose(M_("Arc prototype %s renamed to %s"), describearcproto(ap), pt);
		oat = us_curarcproto;
		if (oat == ap) us_setarcproto(NOARCPROTO, TRUE);
		(void)setval((INTBIG)ap, VARCPROTO, x_("protoname"), (INTBIG)pt, VSTRING);
		if (oat == ap) us_setarcproto(ap, TRUE);

		/* change any component menu entries that mention this arc */
		var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_menu_key);
		if (var != NOVARIABLE)
		{
			for(i=0; i<us_menuy*us_menux; i++)
			{
				us_parsebinding(((CHAR **)var->addr)[i], &commandbinding);
				item = us_makecommand(commandbinding.command);
				if (namesame(item->comname, x_("getproto")) == 0)
				{
					if (item->count >= 2 && namesame(item->word[0], x_("arc")) == 0)
					{
						if (namesame(item->word[1], oldname) == 0)
						{
							/* rename this item */
							if (us_menupos <= 1)
							{
								(void)esnprintf(si, 10, x_("%ld"), i%us_menux);
								(void)esnprintf(sj, 10, x_("%ld"), i/us_menux);
							} else
							{
								(void)esnprintf(si, 10, x_("%ld"), i/us_menuy);
								(void)esnprintf(sj, 10, x_("%ld"), i%us_menuy);
							}
							newpar[0] = x_("set");          newpar[1] = x_("menu");
							newpar[2] = x_("background");   newpar[3] = _("red");
							newpar[4] = sj;             newpar[5] = si;
							newpar[6] = x_("getproto");     newpar[7] = x_("arc");
							newpar[8] = describearcproto(ap);
							us_bind(9, newpar);
						}
					}
				}
				us_freeusercom(item);
				us_freebindingparse(&commandbinding);
			}
		}
		efree(oldname);
	}

	/* handle library name change */
	if (lib != NOLIBRARY)
	{
		/* get pure library name if path was given */
		ch = skippath(pt);

		/* be sure the name is legal */
		if (!us_validname(ch, VLIBRARY)) return;

		/* remove any ".elib" extension */
		k = 0;
		for(str = ch; *str != 0; str++)
			if (namesame(str, x_(".elib")) == 0)
		{
			k = 1;
			*str = 0;
			break;
		}

		/* check for duplicate name */
		if (estrcmp(lib->libname, ch) == 0 && ch == pt)
		{
			ttyputverbose(M_("Library name has not changed"));
			return;
		}
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			if (olib != lib && namesame(olib->libname, ch) == 0)
		{
			us_abortcommand(_("Already a library of that name"));
			return;
		}

		/* change the library name */
		ttyputmsg(_("Library %s renamed to %s"), lib->libname, ch);
		(void)setval((INTBIG)lib, VLIBRARY, x_("libname"), (INTBIG)ch, VSTRING);

		/* change the library file name too */
		if (k != 0) *str = '.';
		if (ch == pt)
		{
			/* no path given: use old path */
			(void)estrcpy(newfile, lib->libfile);
			ch = skippath(newfile);
			(void)estrcpy(ch, pt);
			pt = newfile;
		}
		(void)setval((INTBIG)lib, VLIBRARY, x_("libfile"), (INTBIG)pt, VSTRING);

		/* mark for saving, all libraries that depend on this */
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		{
			if ((olib->userbits&HIDDENLIBRARY) != 0) continue;
			if (olib == lib) continue;

			/* see if any cells in this library reference the renamed one */
			for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				{
					if (ni->proto->primindex != 0) continue;
					if (ni->proto->lib == lib) break;
				}
				if (ni != NONODEINST) break;
			}
			if (np != NONODEPROTO) olib->userbits |= LIBCHANGEDMAJOR;
		}
	}

	/* handle port name change */
	if (pp != NOPORTPROTO)
	{
		/* be sure the name is legal */
		if (!us_validname(pt, VPORTPROTO)) return;

		/* rename the port */
		us_renameport(pp, pt);
	}

	/* handle macro change */
	if (macvar != NOVARIABLE)
	{
		/* be sure the name is legal */
		if (!us_validname(pt, VSTRING)) return;

		/* check for duplicate name */
		if (namesame(&makename(macvar->key)[11], pt) == 0)
		{
			ttyputmsg(_("Macro name has not changed"));
			return;
		}
		var = us_getmacro(pt);
		if (var != macvar)
		{
			us_abortcommand(_("Already a macro of that name"));
			return;
		}

		/* make sure macro name isn't overloading existing command or popup */
		for(i=0; us_lcommand[i].name != 0; i++)
			if (namesame(pt, us_lcommand[i].name) == 0)
		{
			us_abortcommand(_("There is a command with that name"));
			return;
		}
		for(opm=us_firstpopupmenu; opm!=NOPOPUPMENU; opm=opm->nextpopupmenu)
			if (namesame(pt, opm->name) == 0)
		{
			us_abortcommand(_("There is a popup menu with that name"));
			return;
		}

		/* save the macro data */
		len = getlength(macvar);
		newlist = (CHAR **)emalloc(len * (sizeof (CHAR *)), el_tempcluster);
		if (newlist == 0) return;
		for(i=0; i<len; i++)
			(void)allocstring(&newlist[i], ((CHAR **)var->addr)[i], el_tempcluster);

		/* change the macro name */
		ttyputverbose(M_("Macro %s renamed to %s"), &makename(macvar->key)[11], pt);
		(void)delvalkey((INTBIG)us_tool, VTOOL, (INTBIG)macvar->key);
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("USER_macro_"));
		addstringtoinfstr(infstr, pt);
		(void)setval((INTBIG)us_tool, VTOOL, returninfstr(infstr), (INTBIG)newlist,
			VSTRING|VISARRAY|(len<<VLENGTHSH)|VDONTSAVE);
		for(i=0; i<len; i++) efree(newlist[i]);
		efree((CHAR *)newlist);
	}

	/* handle technology change */
	if (tech != NOTECHNOLOGY)
	{
		/* be sure the name is legal */
		if (!us_validname(pt, VTECHNOLOGY)) return;

		/* check for duplicate name */
		if (estrcmp(tech->techname, pt) == 0)
		{
			ttyputmsg(_("Technology name has not changed"));
			return;
		}
		for(otech = el_technologies; otech != NOTECHNOLOGY; otech = otech->nexttechnology)
			if (otech != tech && namesame(otech->techname, pt) == 0)
		{
			us_abortcommand(_("Already a technology of that name"));
			return;
		}

		/* change the technology name */
		ttyputmsg(_("Technology %s renamed to %s"), tech->techname, pt);
		(void)setval((INTBIG)tech, VTECHNOLOGY, x_("techname"), (INTBIG)pt, VSTRING);
	}

	/* handle user command change */
	if (command >= 0)
	{
		/* be sure the name is legal */
		if (!us_validname(pt, VSTRING)) return;

		/* check for duplicate name */
		if (estrcmp(us_lcommand[command].name, pt) == 0)
		{
			ttyputmsg(_("Command name has not changed"));
			return;
		}
		for(i=0; us_lcommand[i].name != 0; i++)
			if (i != command && namesame(pt, us_lcommand[i].name) == 0)
		{
			us_abortcommand(_("Already a command of that name"));
			return;
		}

		/* make sure command name isn't overloading existing macro or popup */
		if (us_getmacro(pt) != NOVARIABLE)
		{
			us_abortcommand(_("There is a macro with that name"));
			return;
		}
		for(opm=us_firstpopupmenu; opm!=NOPOPUPMENU; opm=opm->nextpopupmenu)
			if (namesame(pt, opm->name) == 0)
		{
			us_abortcommand(_("There is a popup menu with that name"));
			return;
		}

		/*
		 * change the command name
		 * Note: this allocates space that is never freed !!!
		 */
		ttyputverbose(M_("Command %s renamed to %s"), us_lcommand[command].name, pt);
		if (allocstring(&us_lcommand[command].name, pt, us_tool->cluster))
			ttyputnomemory();
	}

	/* handle variable change */
	if (variable >= 0)
	{
		/* be sure the name is legal */
		if (!us_validname(pt, VSTRING)) return;

		/* check for duplicate name */
		if (namesame(el_namespace[variable], pt) == 0)
		{
			ttyputmsg(_("Variable name has not changed"));
			return;
		}
		for(i=0; i<el_numnames; i++)
			if (namesame(pt, el_namespace[i]) == 0)
		{
			us_abortcommand(_("Already a variable of that name"));
			return;
		}

		/* change the variable name */
		ttyputverbose(M_("Variable %s renamed to %s"), el_namespace[variable], pt);
		renameval(el_namespace[variable], pt);
	}

	/* handle popup menu change */
	if (pm != NOPOPUPMENU)
	{
		/* be sure the name is legal */
		if (!us_validname(pt, VSTRING)) return;

		/* check for duplicate name */
		if (namesame(pm->name, pt) == 0)
		{
			ttyputmsg(_("Popup menu name has not changed"));
			return;
		}
		for(opm=us_firstpopupmenu; opm!=NOPOPUPMENU; opm=opm->nextpopupmenu)
			if (namesame(pt, opm->name) == 0)
		{
			us_abortcommand(_("Already a popup menu with that name"));
			return;
		}

		/* make sure popup name isn't overloading existing command or menu */
		for(i=0; us_lcommand[i].name != 0; i++)
			if (namesame(pt, us_lcommand[i].name) == 0)
		{
			us_abortcommand(_("There is a command with that name"));
			return;
		}
		if (us_getmacro(pt) != NOVARIABLE)
		{
			us_abortcommand(_("There is a macro with that name"));
			return;
		}

		/* change the popup name */
		ttyputverbose(M_("Popup menu %s renamed to %s"), pm->name, pt);

		/* find the old popup menu */
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("USER_binding_popup_"));
		addstringtoinfstr(infstr, pm->name);
		varoldkey = makekey(returninfstr(infstr));
		var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, varoldkey);
		if (var == NOVARIABLE)
		{
			us_abortcommand(_("Cannot find popup menu %s"), pm->name);
			return;
		}
		len = getlength(var);

		/* create the new popup menu with the new name and old data */
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("USER_binding_popup_"));
		addstringtoinfstr(infstr, pt);
		varnewkey = makekey(returninfstr(infstr));
		(void)setvalkey((INTBIG)us_tool, VTOOL, varnewkey, (INTBIG)var->addr,
			VSTRING|VISARRAY|VDONTSAVE|(len<<VLENGTHSH));

		/* now delete the former popup menu */
		(void)delvalkey((INTBIG)us_tool, VTOOL, varoldkey);
	}

	/* handle network name change */
	if (netname != 0)
	{
		/* be sure the name is legal */
		if (!us_validname(pt, VNETWORK)) return;

		(void)asktool(net_tool, x_("rename"), (INTBIG)netname, (INTBIG)pt, (INTBIG)net->parent);
		return;
	}
}

void us_replace(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG i, total, len;
	REGISTER BOOLEAN universal, ignoreportnames, allowmissingports, connected,
		nodeswitharcs, thiscell, thislibrary;
	REGISTER CHAR *pt;
	REGISTER NODEPROTO *np, *oldntype, *cell, *curcell;
	REGISTER ARCPROTO *ap, *oldatype;
	REGISTER NODEINST *ni, *newno, *lni, *onlynewno, *rni;
	REGISTER ARCINST *ai, *newar, *lai, *onlynewar, *rai;
	REGISTER PORTARCINST *pi, *opi;
	HIGHLIGHT newhigh;
	REGISTER GEOM **list, *firstgeom, *geom;
	REGISTER LIBRARY *lib;
	REGISTER void *infstr;
	extern COMCOMP us_replacep;

	/* find highlighted object to be replaced */
	list = us_gethighlighted(WANTNODEINST|WANTARCINST, 0, 0);
	firstgeom = list[0];
	if (firstgeom == NOGEOM) return;
	curcell = us_needcell();
	if (curcell == NONODEPROTO) return;

	/* handle node replacement */
	if (firstgeom->entryisnode)
	{
		/* get node to be replaced */
		ni = firstgeom->entryaddr.ni;

		/* disallow replacing if lock is on */
		if (us_cantedit(ni->parent, ni, TRUE)) return;

		/* get nodeproto to replace it with */
		if (count == 0)
		{
			count = ttygetparam(M_("Node name: "), &us_replacep, MAXPARS, par);
			if (count == 0)
			{
				us_abortedmsg();
				return;
			}
		}
		np = getnodeproto(par[0]);
		if (np == NONODEPROTO)
		{
			us_abortcommand(_("Nothing called '%s'"), par[0]);
			return;
		}

		/* sanity check */
		oldntype = ni->proto;
		if (oldntype == np)
		{
			us_abortcommand(_("Node already of type %s"), describenodeproto(np));
			return;
		}

		/* get any arguments to the replace */
		ignoreportnames = allowmissingports = connected = thiscell = thislibrary = universal = FALSE;
		if (count > 1)
		{
			len = estrlen(pt = par[1]);
			if (namesamen(pt, x_("connected"), len) == 0) connected = TRUE;
			if (namesamen(pt, x_("this-cell"), len) == 0) thiscell = TRUE;
			if (namesamen(pt, x_("this-library"), len) == 0) thislibrary = TRUE;
			if (namesamen(pt, x_("universally"), len) == 0) universal = TRUE;
			if (namesamen(pt, x_("ignore-port-names"), len) == 0) ignoreportnames = TRUE;
			if (namesamen(pt, x_("allow-missing-ports"), len) == 0) allowmissingports = TRUE;
		}

		/* clear highlighting */
		us_clearhighlightcount();

		/* replace the nodeinsts */
		infstr = initinfstr();
		for(ni = curcell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			ni->temp1 = 0;
		for(i=0; list[i] != NOGEOM; i++)
		{
			geom = list[i];
			if (!geom->entryisnode) continue;
			ni = geom->entryaddr.ni;
			onlynewno = us_replacenodeinst(ni, np, ignoreportnames, allowmissingports);
			if (onlynewno == NONODEINST)
			{
				us_abortcommand(_("%s does not fit in the place of %s"), describenodeproto(np),
					describenodeproto(oldntype));
				newhigh.status = HIGHFROM;
				newhigh.cell = curcell;
				newhigh.fromgeom = ni->geom;
				newhigh.fromport = NOPORTPROTO;
				newhigh.frompoint = 0;
				newhigh.fromvar = NOVARIABLE;
				newhigh.fromvarnoeval = NOVARIABLE;
				us_addhighlight(&newhigh);
				(void)returninfstr(infstr);
				return;
			}
			onlynewno->temp1 = 1;
			formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0\n"),
				describenodeproto(onlynewno->parent), (INTBIG)onlynewno->geom);
		}

		/* do additional replacements if requested */
		total = 1;
		if (universal)
		{
			/* replace in all cells of library if "universally" used */
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
				for(cell = lib->firstnodeproto; cell != NONODEPROTO;
					cell = cell->nextnodeproto)
			{
				for(lni = cell->firstnodeinst; lni != NONODEINST; lni = lni->nextnodeinst)
				{
					if (lni->proto != oldntype) continue;

					/* do not replace the example icon */
					if (isiconof(oldntype, cell))
					{
						ttyputmsg(_("Example icon in cell %s not replaced"), describenodeproto(cell));
						continue;
					}

					/* disallow replacing if lock is on */
					if (us_cantedit(cell, lni, TRUE)) continue;

					newno = us_replacenodeinst(lni, np, ignoreportnames, allowmissingports);
					if (newno != NONODEINST)
					{
						total++;
						formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0\n"),
							describenodeproto(newno->parent), (INTBIG)newno->geom);
					}
					if (stopping(STOPREASONREPLACE)) break;
				}
			}
			ttyputmsg(_("All %ld %s nodes in all libraries replaced with %s"), total,
				describenodeproto(oldntype), describenodeproto(np));
		} else if (thislibrary)
		{
			/* replace throughout this library if "this-library" used */
			for(cell = el_curlib->firstnodeproto; cell != NONODEPROTO;
				cell = cell->nextnodeproto)
			{
				for(lni = cell->firstnodeinst; lni != NONODEINST; lni = lni->nextnodeinst)
					if (lni->proto == oldntype)
				{
					/* disallow replacing if lock is on */
					if (us_cantedit(cell, lni, TRUE)) continue;

					newno = us_replacenodeinst(lni, np, ignoreportnames, allowmissingports);
					if (newno != NONODEINST)
					{
						total++;
						formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0\n"),
							describenodeproto(newno->parent), (INTBIG)newno->geom);
					}
					if (stopping(STOPREASONREPLACE)) break;
				}
			}
			ttyputmsg(_("All %ld %s nodes in library %s replaced with %s"), total,
				describenodeproto(oldntype), el_curlib->libname, describenodeproto(np));
		} else if (thiscell)
		{
			/* replace throughout this cell if "this-cell" used */
			for(lni = curcell->firstnodeinst; lni != NONODEINST; lni = lni->nextnodeinst)
				if (lni->proto == oldntype)
			{
				/* disallow replacing if lock is on */
				if (us_cantedit(curcell, lni, TRUE)) continue;

				newno = us_replacenodeinst(lni, np, ignoreportnames, allowmissingports);
				if (newno != NONODEINST)
				{
					total++;
					formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0\n"),
						describenodeproto(newno->parent), (INTBIG)newno->geom);
				}
				if (stopping(STOPREASONREPLACE)) break;
			}
			ttyputmsg(_("All %ld %s nodes in cell %s replaced with %s"), total,
				describenodeproto(oldntype), describenodeproto(ni->parent), describenodeproto(np));
		} else if (connected)
		{
			/* replace all connected to this in the cell if "connected" used */
			for(lni = curcell->firstnodeinst; lni != NONODEINST; lni = lni->nextnodeinst)
				if (lni->proto == oldntype)
			{
				for(pi = lni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				{
					for(rni = curcell->firstnodeinst; rni != NONODEINST; rni = rni->nextnodeinst)
					{
						if (rni->temp1 == 0) continue;
						for(opi = rni->firstportarcinst; opi != NOPORTARCINST; opi = opi->nextportarcinst)
						{
							if (pi->conarcinst->network == opi->conarcinst->network) break;
						}
						if (opi != NOPORTARCINST) break;
					}
					if (rni != NONODEINST) break;
				}
				if (pi == NOPORTARCINST) continue;

				/* disallow replacing if lock is on */
				if (us_cantedit(curcell, lni, TRUE)) continue;

				newno = us_replacenodeinst(lni, np, ignoreportnames, allowmissingports);
				if (newno != NONODEINST)
				{
					total++;
					formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0\n"),
						describenodeproto(newno->parent), (INTBIG)newno->geom);
				}
				if (stopping(STOPREASONREPLACE)) break;
			}
			ttyputmsg(_("All %ld %s nodes connected to this replaced with %s"), total,
				describenodeproto(oldntype), describenodeproto(np));
		} else ttyputmsg(_("Node %s replaced with %s"), describenodeproto(oldntype),
			describenodeproto(np));

		/* clean up */
		us_setnodeproto(np);
		us_setmultiplehighlight(returninfstr(infstr), FALSE);
	} else
	{
		/* get arc to be replaced */
		ai = firstgeom->entryaddr.ai;

		/* disallow replacement if lock is on */
		if (us_cantedit(ai->parent, NONODEINST, TRUE)) return;

		/* get arcproto to replace it with */
		if (count == 0)
		{
			count = ttygetparam(M_("Arc name: "), &us_replacep, MAXPARS, par);
			if (count == 0)
			{
				us_abortedmsg();
				return;
			}
		}
		ap = getarcproto(par[0]);
		if (ap == NOARCPROTO)
		{
			us_abortcommand(_("Nothing called '%s'"), par[0]);
			return;
		}

		/* sanity check */
		oldatype = ai->proto;
		if (oldatype == ap)
		{
			us_abortcommand(_("Arc already of type %s"), describearcproto(ap));
			return;
		}

		/* get any arguments to the replace */
		connected = thiscell = thislibrary = universal = nodeswitharcs = FALSE;
		while (count > 1)
		{
			len = estrlen(pt = par[1]);
			if (namesamen(pt, x_("connected"), len) == 0) connected = TRUE;
			if (namesamen(pt, x_("this-cell"), len) == 0) thiscell = TRUE;
			if (namesamen(pt, x_("this-library"), len) == 0) thislibrary = TRUE;
			if (namesamen(pt, x_("universally"), len) == 0) universal = TRUE;
			if (namesamen(pt, x_("nodes-too"), len) == 0) nodeswitharcs = TRUE;
			par++;
			count--;
		}

		/* special case when replacing nodes, too */
		if (nodeswitharcs)
		{
			if (thislibrary)
			{
				for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
					us_replaceallarcs(np, list, ap, FALSE, TRUE);
			} else
			{
				us_replaceallarcs(ai->parent, list, ap, connected, thiscell);
			}
			return;
		}

		/* remove highlighting */
		us_clearhighlightcount();

		/* replace the arcinst */
		infstr = initinfstr();
		for(ai = curcell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			ai->temp1 = 0;
		for(i=0; list[i] != NOGEOM; i++)
		{
			geom = list[i];
			if (geom->entryisnode) continue;
			ai = geom->entryaddr.ai;
			if (ai->proto != oldatype) continue;
			startobjectchange((INTBIG)ai, VARCINST);
			onlynewar = replacearcinst(ai, ap);
			if (onlynewar == NOARCINST)
			{
				us_abortcommand(_("%s does not fit in the place of %s"), describearcproto(ap),
					describearcproto(oldatype));
				(void)returninfstr(infstr);
				return;
			}
			endobjectchange((INTBIG)onlynewar, VARCINST);
			onlynewar->temp1 = 1;
			formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0\n"),
				describenodeproto(onlynewar->parent), (INTBIG)onlynewar->geom);
		}

		/* do additional replacements if requested */
		total = 1;
		if (universal)
		{
			/* replace in all cells of library if "universally" used */
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
				for(cell = lib->firstnodeproto; cell != NONODEPROTO;
					cell = cell->nextnodeproto)
			{
				for(lai = cell->firstarcinst; lai != NOARCINST; lai = lai->nextarcinst)
					if (lai->proto == oldatype)
				{
					/* disallow replacing if lock is on */
					if (us_cantedit(cell, NONODEINST, TRUE)) continue;

					startobjectchange((INTBIG)lai, VARCINST);
					newar = replacearcinst(lai, ap);
					if (newar != NOARCINST)
					{
						total++;
						formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0\n"),
							describenodeproto(newar->parent), (INTBIG)newar->geom);
						endobjectchange((INTBIG)newar, VARCINST);
					}
					if (stopping(STOPREASONREPLACE)) break;
				}
			}
			ttyputmsg(_("All %ld %s arcs in the library replaced with %s"), total,
				describearcproto(oldatype), describearcproto(ap));
		} else if (thislibrary)
		{
			/* replace throughout this library if "this-library" used */
			for(cell = el_curlib->firstnodeproto; cell != NONODEPROTO;
				cell = cell->nextnodeproto)
			{
				for(lai = cell->firstarcinst; lai != NOARCINST; lai = lai->nextarcinst)
					if (lai->proto == oldatype)
				{
					startobjectchange((INTBIG)lai, VARCINST);
					newar = replacearcinst(lai, ap);
					if (newar != NOARCINST)
					{
						total++;
						formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0\n"),
							describenodeproto(newar->parent), (INTBIG)newar->geom);
						endobjectchange((INTBIG)newar, VARCINST);
					}
					if (stopping(STOPREASONREPLACE)) break;
				}
			}
			ttyputmsg(_("All %ld %s arcs in library %s replaced with %s"), total,
				describearcproto(oldatype), el_curlib->libname, describearcproto(ap));
		} else if (thiscell)
		{
			/* replace throughout this cell if "this-cell" used */
			for(lai = curcell->firstarcinst; lai != NOARCINST; lai = lai->nextarcinst)
				if (lai->proto == oldatype)
			{
				startobjectchange((INTBIG)lai, VARCINST);
				newar = replacearcinst(lai, ap);
				if (newar != NOARCINST)
				{
					total++;
					formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0\n"),
						describenodeproto(newar->parent), (INTBIG)newar->geom);
					endobjectchange((INTBIG)newar, VARCINST);
				}
				if (stopping(STOPREASONREPLACE)) break;
			}
			ttyputmsg(_("All %ld %s arcs in cell %s replaced with %s"), total,
				describearcproto(oldatype), describenodeproto(ai->parent), describearcproto(ap));
		} else if (connected)
		{
			/* replace all connected to this if "connected" used */
			for(lai = curcell->firstarcinst; lai != NOARCINST; lai = lai->nextarcinst)
				if (lai->proto == oldatype)
			{
				for(rai = curcell->firstarcinst; rai != NOARCINST; rai = rai->nextarcinst)
				{
					if (rai->temp1 == 0) continue;
					if (lai->network != rai->network) continue;
					startobjectchange((INTBIG)lai, VARCINST);
					newar = replacearcinst(lai, ap);
					if (newar != NOARCINST)
					{
						total++;
						formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0\n"),
							describenodeproto(newar->parent), (INTBIG)newar->geom);
						endobjectchange((INTBIG)newar, VARCINST);
					}
				}
				if (stopping(STOPREASONREPLACE)) break;
			}
			ttyputmsg(_("All %ld %s arcs connected to this replaced with %s"), total,
				describearcproto(oldatype), describearcproto(ap));
		} else ttyputmsg(_("Arc %s replaced with %s"), describearcproto(oldatype),
			describearcproto(ap));

		/* clean up */
		us_setarcproto(ap, TRUE);
		us_setmultiplehighlight(returninfstr(infstr), FALSE);
	}
}

void us_rotate(INTBIG count, CHAR *par[])
{
	REGISTER NODEINST *ni, *theni, *subni, **nilist, **newnilist;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp, *thepp;
	REGISTER ARCINST *ai, **ailist, **newailist;
	REGISTER GEOM **list;
	REGISTER VARIABLE *var;
	REGISTER HIGHLIGHT *high;
	REGISTER INTBIG amt, startangle, endangle, rotatemore;
	INTBIG xstart, ystart, xend, yend, gx, gy, cx, cy, rotcx, rotcy, aicount,
		x, y, thex, they, newnicount;
	REGISTER INTBIG lx, hx, ly, hy, nicount, dist, bestdist, i, j;
	XARRAY transtz, rot, transfz, t1, t2;

	/* handle interactive rotation */
	if (count == 1 && namesamen(par[0], x_("interactively"), estrlen(par[0])) == 0)
	{
		ni = (NODEINST *)us_getobject(VNODEINST, FALSE);
		if (ni == NONODEINST)
		{
			us_abortcommand(_("Must highlight one node for interactive rotation"));
			return;
		}

		/* disallow rotating if lock is on */
		if (us_cantedit(ni->parent, ni, TRUE)) return;

		/* save highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		if (us_demandxy(&xstart, &ystart)) return;
		us_rotateinit(ni);
		trackcursor(FALSE, us_ignoreup, us_rotatebegin, us_rotatedown,
			us_stopandpoponchar, us_dragup, TRACKDRAGGING);
		if (el_pleasestop != 0) return;
		if (us_demandxy(&xend, &yend)) return;
		startangle = figureangle((ni->lowx+ni->highx)/2, (ni->lowy+ni->highy)/2, xstart, ystart);
		endangle = figureangle((ni->lowx+ni->highx)/2, (ni->lowy+ni->highy)/2, xend, yend);
		if (startangle == endangle)
		{
			ttyputverbose(M_("Null node rotation"));
			us_pophighlight(FALSE);
			return;
		}
		if (ni->transpose == 0) amt = endangle - startangle; else
			amt = startangle - endangle;
		while (amt < 0) amt += 3600;
		while (amt > 3600) amt -= 3600;

		/* do the rotation */
		startobjectchange((INTBIG)ni, VNODEINST);
		modifynodeinst(ni, 0, 0, 0, 0, amt, 0);
		endobjectchange((INTBIG)ni, VNODEINST);

		/* restore highlighting */
		us_pophighlight(TRUE);
		return;
	}

	/* determine rotation amount */
	if (count < 1)
	{
		ttyputusage(x_("rotate ANGLE"));
		return;
	}
	amt = atofr(par[0]);
	amt = amt * 10 / WHOLE;
	rotatemore = 0;
	if (count >= 2 && namesamen(par[1], x_("more"), estrlen(par[1])) == 0)
	{
		count--;
		par++;
		rotatemore++;
	}

	/* get all highlighted objects for rotation */
	list = us_gethighlighted(WANTNODEINST|WANTARCINST, 0, 0);
	if (list[0] == NOGEOM)
	{
		us_abortcommand(_("Must highlight node(s) to be rotated"));
		return;
	}
	np = geomparent(list[0]);

	/* disallow rotating if lock is on */
	if (us_cantedit(np, NONODEINST, TRUE)) return;

	/* figure out which nodes get rotated */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		ni->temp1 = 0;
	nicount = 0;
	theni = NONODEINST;
	lx = ly = hx = hy = 0;
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (!list[i]->entryisnode) continue;
		ni = list[i]->entryaddr.ni;
		if (us_cantedit(np, ni, TRUE)) return;
		ni->temp1 = 1;
		if (nicount == 0)
		{
			lx = ni->lowx;   hx = ni->highx;
			ly = ni->lowy;   hy = ni->highy;
		} else
		{
			if (ni->lowx < lx) lx = ni->lowx;
			if (ni->highx > hx) hx = ni->highx;
			if (ni->lowy < ly) ly = ni->lowy;
			if (ni->highy > hy) hy = ni->highy;
		}
		theni = ni;
		nicount++;
	}

	/* must be at least 1 node */
	if (nicount <= 0)
	{
		us_abortcommand(_("Must select at least 1 node for rotation"));
		return;
	}

	/* if multiple nodes, find the center one */
	if (nicount > 1)
	{
		theni = NONODEINST;
		bestdist = MAXINTBIG;
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if (ni->temp1 == 0) continue;
			dist = computedistance((lx+hx)/2, (ly+hy)/2, (ni->lowx+ni->highx)/2,
				(ni->lowy+ni->highy)/2);

			/* LINTED "bestdist" used in proper order */
			if (theni == NONODEINST || dist < bestdist)
			{
				theni = ni;
				bestdist = dist;
			}
		}
	}

	/* compute rotation, given the node */
	if (amt == 0)
	{
		ttyputverbose(M_("Null rotation"));
		return;
	}

	/* handle rotation about the grab point */
	i = estrlen(par[1]);
	if (count >= 2 && namesamen(par[1], x_("sensibly"), i) == 0)
	{
		if (nicount == 1)
		{
			if ((el_curwindowpart->state&WINDOWOUTLINEEDMODE) != 0)
			{
				par[1] = x_("about-trace-point");
			} else
			{
				if (theni->proto->primindex == 0)
				{
					var = getvalkey((INTBIG)theni->proto, VNODEPROTO, VINTEGER|VISARRAY, el_prototype_center_key);
					if (var != NOVARIABLE)
					{
						par[1] = x_("about-grab-point");
					}
				}
			}
		}
	}
	if (count >= 2 && namesamen(par[1], x_("about-grab-point"), i) == 0 && i >= 7)
	{
		if (nicount > 1)
		{
			us_abortcommand(_("Must highlight one node for rotation about the grab-point"));
			return;
		}
		ni = theni;

		/* disallow rotating if lock is on */
		if (us_cantedit(ni->parent, ni, TRUE)) return;

		/* find the grab point */
		corneroffset(ni, ni->proto, ni->rotation, ni->transpose, &gx, &gy, FALSE);
		gx += ni->lowx;   gy += ni->lowy;

		/* build transformation for this operation */
		transid(transtz);   transtz[2][0] = -gx;   transtz[2][1] = -gy;
		makeangle(amt, 0, rot);
		transid(transfz);   transfz[2][0] = gx;    transfz[2][1] = gy;
		transmult(transtz, rot, t1);
		transmult(t1, transfz, t2);
		cx = (ni->lowx+ni->highx)/2;   cy = (ni->lowy+ni->highy)/2;
		xform(cx, cy, &gx, &gy, t2);
		gx -= cx;   gy -= cy;

		/* save highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* do the rotation */
		startobjectchange((INTBIG)ni, VNODEINST);

		/* rotate and translate */
		modifynodeinst(ni, gx, gy, gx, gy, amt, 0);

		/* end change */
		endobjectchange((INTBIG)ni, VNODEINST);

		/* restore highlighting */
		us_pophighlight(TRUE);
		return;
	}

	/* handle rotation about a trace point */
	if (count >= 2 && namesamen(par[1], x_("about-trace-point"), i) == 0 && i >= 7)
	{
		if (nicount > 1)
		{
			us_abortcommand(_("Must highlight one node for rotation about an outline point"));
			return;
		}
		ni = theni;

		/* disallow rotating if lock is on */
		if (us_cantedit(ni->parent, ni, TRUE)) return;

		/* get the trace information */
		var = gettrace(ni);
		if (var == NOVARIABLE)
		{
			us_abortcommand(_("Highlighted node must have outline information"));
			return;
		}

		/* find the pivot point */
		high = us_getonehighlight();
		i = high->frompoint;   if (i != 0) i--;
		makerot(ni, t1);
		gx = (ni->highx + ni->lowx) / 2;
		gy = (ni->highy + ni->lowy) / 2;
		xform(((INTBIG *)var->addr)[i*2]+gx, ((INTBIG *)var->addr)[i*2+1]+gy, &gx, &gy, t1);

		/* build transformation for this operation */
		transid(transtz);   transtz[2][0] = -gx;   transtz[2][1] = -gy;
		makeangle(amt, 0, rot);
		transid(transfz);   transfz[2][0] = gx;    transfz[2][1] = gy;
		transmult(transtz, rot, t1);
		transmult(t1, transfz, t2);
		cx = (ni->lowx+ni->highx)/2;   cy = (ni->lowy+ni->highy)/2;
		xform(cx, cy, &gx, &gy, t2);
		gx -= cx;   gy -= cy;

		/* save highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* do the rotation */
		startobjectchange((INTBIG)ni, VNODEINST);

		/* rotate and translate */
		modifynodeinst(ni, gx, gy, gx, gy, amt, 0);

		/* end change */
		endobjectchange((INTBIG)ni, VNODEINST);

		/* restore highlighting */
		us_pophighlight(TRUE);
		return;
	}

	/* see which nodes already connect to the main rotation node (theni) */
	for(ni = theni->parent->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		ni->temp1 = 0;
	theni->temp1 = 1;
	for(ai = theni->parent->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		ai->temp1 = 0;
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (list[i]->entryisnode) continue;
		ai = list[i]->entryaddr.ai;
		ai->temp1 = 1;
	}
	us_spreadrotateconnection(theni);

	/* now make sure that it is all connected */
	aicount = newnicount = 0;
	nilist = 0;
	ailist = 0;
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (!list[i]->entryisnode) continue;
		ni = list[i]->entryaddr.ni;
		if (ni == theni) continue;
		if (ni->temp1 != 0) continue;

		thepp = theni->proto->firstportproto;
		if (thepp == NOPORTPROTO)
		{
			/* no port on the cell: create one */
			subni = newnodeinst(gen_univpinprim, theni->proto->lowx, theni->proto->highx,
				theni->proto->lowy, theni->proto->highy, 0, 0, theni->proto);
			if (subni == NONODEINST) break;
			thepp = newportproto(theni->proto, subni, subni->proto->firstportproto, x_("temp"));
			if (thepp == NOPORTPROTO) break;

			/* add to the list of temporary nodes */
			newnilist = (NODEINST **)emalloc((newnicount+1) * (sizeof (NODEINST *)), el_tempcluster);
			if (newnilist == 0) break;

			/* LINTED "nilist" used in proper order */
			for(j=0; j<newnicount; j++) newnilist[j] = nilist[j];
			if (newnicount > 0) efree((CHAR *)nilist);
			nilist = newnilist;
			nilist[newnicount] = subni;
			newnicount++;
		}
		pp = ni->proto->firstportproto;
		if (pp != NOPORTPROTO)
		{
			portposition(theni, thepp, &thex, &they);
			portposition(ni, pp, &x, &y);
			ai = newarcinst(gen_invisiblearc, 0, FIXED, theni, thepp, thex, they,
				ni, pp, x, y, np);
			if (ai == NOARCINST) break;
			endobjectchange((INTBIG)ai, VARCINST);

			newailist = (ARCINST **)emalloc((aicount+1) * (sizeof (ARCINST *)), el_tempcluster);
			if (newailist == 0) break;

			/* LINTED "ailist" used in proper order */
			for(j=0; j<aicount; j++) newailist[j] = ailist[j];
			if (aicount > 0) efree((CHAR *)ailist);
			ailist = newailist;
			ailist[aicount] = ai;
			aicount++;
		}
	}

	/* make all selected arcs temporarily rigid */
	us_modarcbits(6, FALSE, x_(""), list);

	/* save highlighting */
	us_pushhighlight();
	us_clearhighlightcount();

	/* see if there is a snap point */
	if (!us_getonesnappoint(&rotcx, &rotcy))
	{
		/* no snap point, use center of node */
		rotcx = (theni->lowx + theni->highx) / 2;
		rotcy = (theni->lowy + theni->highy) / 2;
	}

	/* build transformation for this operation */
	transid(transtz);   transtz[2][0] = -rotcx;   transtz[2][1] = -rotcy;
	makeangle(amt, 0, rot);
	transid(transfz);   transfz[2][0] = rotcx;    transfz[2][1] = rotcy;
	transmult(transtz, rot, t1);
	transmult(t1, transfz, t2);
	cx = (theni->lowx+theni->highx)/2;   cy = (theni->lowy+theni->highy)/2;
	xform(cx, cy, &gx, &gy, t2);
	gx -= cx;   gy -= cy;

	/* do the rotation */
	startobjectchange((INTBIG)theni, VNODEINST);
	modifynodeinst(theni, gx, gy, gx, gy, amt, 0);
	endobjectchange((INTBIG)theni, VNODEINST);

	/* delete intermediate arcs used to constrain */
	for(i=0; i<aicount; i++)
	{
		startobjectchange((INTBIG)ailist[i], VARCINST);
		(void)killarcinst(ailist[i]);
	}
	if (aicount > 0) efree((CHAR *)ailist);

	/* delete intermediate nodes used to constrain */
	for(i=0; i<newnicount; i++)
	{
		(void)killportproto(nilist[i]->parent, nilist[i]->firstportexpinst->exportproto);
		(void)killnodeinst(nilist[i]);
	}
	if (newnicount > 0) efree((CHAR *)nilist);

	/* restore highlighting */
	us_pophighlight(TRUE);
}

#define MAXPORTTYPE 16

void us_show(INTBIG count, CHAR *par[])
{
	CHAR line[100], *activity, *name, *colorname, *colorsymbol, *dumpfilename, *truename;
	REGISTER CHAR *pt, *matchspec, *str, **keybindings, **buttonbindings, *pt1, *pt2, save,
		*prefix, **dummylibs, **newdummylibs;
	INTBIG plx, ply, phx, phy, keyindex[NUMKEYS], idummy32, porttype[MAXPORTTYPE], len, wid, xp, yp,
		boundspecial, shortcols, numtypes, num_found;
	INTSML boundkey;
	REGISTER INTBIG i, j, k, l, m, tot, *equivlist, *buslist, lx, hx, ly, hy,
		verbose, maxlen, total, columns, rows, key, but, menu, popup, lambda,
		shortcolwidth, shortrows, x, y, fun, keytotal, unnamed, dummylibcount;
	REGISTER BOOLEAN graphiclist, graphiclistlocal, first, notbelow, placeholders,
		recursivenodes, givedates, contentslist, summarize, editlist;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np, *wnp, **sortindex;
	REGISTER PORTPROTO *pp, **pplist, *opp, **sortedbuslist;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER ARCINST *ai;
	REGISTER ARCPROTO *ap;
	REGISTER POPUPMENU *pm, *wantpm;
	REGISTER USERCOM *rb;
	REGISTER HIGHLIGHT *high;
	REGISTER LIBRARY *lib, *olib;
	REGISTER VIEW *v;
	REGISTER WINDOWPART *w;
	REGISTER TECHNOLOGY *tech;
	REGISTER VARIABLE *var, *varkey, *varbutton;
	REGISTER NETWORK *net;
	FILE *dumpfile;
	CONSTRAINT *con;
	COMMANDBINDING commandbinding;
	extern COMCOMP us_showp;
	REGISTER void *infstr;

	if (count == 0)
	{
		count = ttygetparam(M_("Show option: "), &us_showp, MAXPARS, par);
		if (count == 0)
		{
			us_abortedmsg();
			return;
		}
	}
	l = estrlen(pt = par[0]);

	if (namesamen(pt, x_("tools"), l) == 0 && l >= 1)
	{
		ttyputmsg(_(" Which Tool       Information"));
		for(i=0; i<el_maxtools; i++)
		{
			infstr = initinfstr();
			if ((el_tools[i].toolstate&TOOLON) == 0) addstringtoinfstr(infstr, _("Off")); else
				addstringtoinfstr(infstr, _("On"));
			if ((el_tools[i].toolstate&TOOLBG) != 0) addstringtoinfstr(infstr, _(", Background"));
			if ((el_tools[i].toolstate&TOOLFIX) != 0) addstringtoinfstr(infstr, _(", Correcting"));
			if ((el_tools[i].toolstate&TOOLLANG) != 0) addstringtoinfstr(infstr, _(", Interpreted"));
			if ((el_tools[i].toolstate&TOOLINCREMENTAL) != 0) addstringtoinfstr(infstr, _(", Incremental"));
			if ((el_tools[i].toolstate&TOOLANALYSIS) != 0) addstringtoinfstr(infstr, _(", Analysis"));
			if ((el_tools[i].toolstate&TOOLSYNTHESIS) != 0) addstringtoinfstr(infstr, _(", Synthesis"));
			ttyputmsg(x_("%-16s %s"), el_tools[i].toolname, returninfstr(infstr));
		}
		return;
	}

	if (namesamen(pt, x_("bindings"), l) == 0 && l >= 1)
	{
		if (count <= 1)
		{
			ttyputusage(x_("show bindings key|menu|button|popup|all|short"));
			return;
		}
		l = estrlen(pt = par[1]);

		if (namesamen(pt, x_("short"), l) == 0 && l >= 1)
		{
			varkey = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_keys_key);
			varbutton = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_buttons_key);
			if (varkey == NOVARIABLE || varbutton == NOVARIABLE)
			{
				ttyputerr(M_("Cannot find key and button bindings"));
				return;
			}
			keytotal = getlength(varkey);
			keybindings = (CHAR **)varkey->addr;
			buttonbindings = (CHAR **)varbutton->addr;

			/* print the button bindings */
			j = buttoncount();
			if (j > 0)
			{
				infstr = initinfstr();
				for(i=0; i<(MESSAGESWIDTH-25)/2; i++) addtoinfstr(infstr, '-');
				addstringtoinfstr(infstr, M_(" Single Button Commands: "));
				for(i=0; i<(MESSAGESWIDTH-25)/2; i++) addtoinfstr(infstr, '-');
				ttyputmsg(x_("%s"), returninfstr(infstr));

				/* count the number of bound buttons, compute longest name */
				for(i=0, k=0, j=0; i<buttoncount(); i++)
				{
					us_parsebinding(buttonbindings[i], &commandbinding);
					if (*commandbinding.command != 0)
					{
						k++;
						for(l=0; commandbinding.command[l] != 0; l++)
							if (commandbinding.command[l] == ' ') break;
						j = maxi(j, l + estrlen(buttonname(i, &idummy32)));
					}
					us_freebindingparse(&commandbinding);
				}

				/* compute number of rows and columns */
				shortcols = mini(MESSAGESWIDTH / (j+4), k);
				shortcolwidth = MESSAGESWIDTH / shortcols;
				shortrows = (k+shortcols-1) / shortcols;

				/* print the buttons */
				i = -1;
				for(j=0; j<shortrows; j++)
				{
					infstr = initinfstr();
					for(m=0; m<shortcols; m++)
					{
						/* find next bound button */
						for (;;)
						{
							i++;
							if (i >= buttoncount()) break;
							us_parsebinding(buttonbindings[i], &commandbinding);
							if (*commandbinding.command != 0) break;
							us_freebindingparse(&commandbinding);
						}
						if (i >= buttoncount()) break;

						/* place button name */
						pt = buttonname(i, &idummy32);
						addstringtoinfstr(infstr, pt);
						addstringtoinfstr(infstr, x_(": "));
						k = estrlen(pt);

						/* place command name */
						for(l=0; commandbinding.command[l] != 0; l++)
							if (commandbinding.command[l] == ' ') break;
								else addtoinfstr(infstr, commandbinding.command[l]);
						us_freebindingparse(&commandbinding);
						k += l;

						/* pad out the field if not at the end */
						if (m<shortcols-1)
							for(k = k+4; k < shortcolwidth; k++)
								addtoinfstr(infstr, ' ');
					}
					ttyputmsg(x_("%s"), returninfstr(infstr));
				}
			}

			/* print the key bindings */
			infstr = initinfstr();
			for(i=0; i<(MESSAGESWIDTH-22)/2; i++) addtoinfstr(infstr, '-');
			addstringtoinfstr(infstr, M_(" Single Key Commands: "));
			for(i=0; i<(MESSAGESWIDTH-22)/2; i++) addtoinfstr(infstr, '-');
			ttyputmsg(x_("%s"), returninfstr(infstr));

			/* count the number of bound keys, compute longest name */
			for(i=0, k=0, j=0; i<keytotal; i++)
			{
				keyindex[i] = -1;
				pt = us_getboundkey(keybindings[i], &boundkey, &boundspecial);
				us_parsebinding(pt, &commandbinding);
				if (*commandbinding.command != 0)
				{
					keyindex[i] = k++;
					for(l=0; commandbinding.command[l] != 0; l++)
						if (commandbinding.command[l] == ' ') break;
					j = maxi(j, l);
				}
				us_freebindingparse(&commandbinding);
			}

			/* compute number of rows and columns */
			shortcols = MESSAGESWIDTH / (j+8);
			shortcolwidth = MESSAGESWIDTH / shortcols;
			shortrows = (k+shortcols-1) / shortcols;

			/* print the keys */
			for(j=0; j<shortrows; j++)
			{
				infstr = initinfstr();
				for(m=0; m<shortcols; m++) for(i=0; i<keytotal; i++)
					if (keyindex[i] == m*shortrows+j)
				{
					/* place key name */
					str = us_getboundkey(keybindings[i], &boundkey, &boundspecial);
					pt = us_describeboundkey(boundkey, boundspecial, 1);
					for(k=estrlen(pt); k<8; k++) addtoinfstr(infstr, ' ');
					addstringtoinfstr(infstr, pt);
					addtoinfstr(infstr, ' ');

					/* place command name */
					us_parsebinding(str, &commandbinding);
					for(l=0; commandbinding.command[l] != 0; l++)
						if (commandbinding.command[l] == ' ') break; else
							addtoinfstr(infstr, commandbinding.command[l]);
					us_freebindingparse(&commandbinding);

					/* pad out the field if not at the end */
					if (m<shortcols-1)
						for(k=l+4; k < shortcolwidth; k++)
							addtoinfstr(infstr, ' ');
				}
				ttyputmsg(x_("%s"), returninfstr(infstr));
			}
			return;
		}

		key = but = menu = popup = 0;
		if (namesamen(pt, x_("all"), l) == 0 && l >= 1)
		{
			but++;   key++;   menu++;   popup++;
		}
		if (namesamen(pt, x_("key"), l) == 0 && l >= 1) key++;
		if (namesamen(pt, x_("menu"), l) == 0 && l >= 1) menu++;
		if (namesamen(pt, x_("button"), l) == 0 && l >= 1) but++;
		if (namesamen(pt, x_("popup"), l) == 0 && l >= 1) popup++;

		if (key == 0 && but == 0 && menu == 0 && popup == 0)
		{
			ttyputbadusage(x_("show bindings"));
			return;
		}

		/* if key bindings are requested, list them */
		if (key)
		{
			var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_keys_key);
			if (var == NOVARIABLE)
			{
				ttyputerr(M_("Cannot find key binding attributes"));
				return;
			}
			l = getlength(var);
			for(i=0; i<l; i++)
			{
				pt = us_getboundkey(((CHAR **)var->addr)[i], &boundkey, &boundspecial);
				us_parsebinding(pt, &commandbinding);
				if (*commandbinding.command != 0)
					ttyputmsg(M_("Key %s: %s"), us_describeboundkey(boundkey, boundspecial, 1),
						commandbinding.command);
				us_freebindingparse(&commandbinding);
			}
		}

		/* if button bindings are requested, list them */
		if (but)
		{
			var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_buttons_key);
			if (var == NOVARIABLE)
			{
				ttyputerr(M_("Cannot find button binding attributes"));
				return;
			}
			l = getlength(var);
			for(i=0; i<l; i++)
			{
				us_parsebinding(((CHAR **)var->addr)[i], &commandbinding);
				if (*commandbinding.command != 0) ttyputmsg(_("Button %s: %s"),
					buttonname(i, &idummy32), commandbinding.command);
				us_freebindingparse(&commandbinding);
			}
		}

		/* if menu bindings are requested, list them */
		if (menu)
		{
			var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_menu_key);
			if (var == NOVARIABLE)
			{
				ttyputerr(M_("Cannot find menu binding attributes"));
				return;
			}
			for(x=0; x<us_menux; x++) for(y=0; y<us_menuy; y++)
			{
				if (us_menupos <= 1)
					str = ((CHAR **)var->addr)[y * us_menux + x]; else
						str = ((CHAR **)var->addr)[x * us_menuy + y];
				us_parsebinding(str, &commandbinding);
				infstr = initinfstr();
				formatinfstr(infstr, M_("Menu row %ld column %ld: "), y, x);
				if (*commandbinding.command == 0) addstringtoinfstr(infstr, _("NOT DEFINED")); else
					addstringtoinfstr(infstr, commandbinding.command);
				if (commandbinding.menumessage != 0)
				{
					addstringtoinfstr(infstr, x_(" [message=\""));
					addstringtoinfstr(infstr, commandbinding.menumessage);
					addstringtoinfstr(infstr, x_("\"]"));
				} else if (commandbinding.nodeglyph != NONODEPROTO)
				{
					addstringtoinfstr(infstr, x_(" [node="));
					addstringtoinfstr(infstr, describenodeproto(commandbinding.nodeglyph));
					addtoinfstr(infstr, ']');
				} else if (commandbinding.arcglyph != NOARCPROTO)
				{
					addstringtoinfstr(infstr, x_(" [arc="));
					addstringtoinfstr(infstr, describearcproto(commandbinding.arcglyph));
					addtoinfstr(infstr, ']');
				}
				if (commandbinding.backgroundcolor != 0)
				{
					if (ecolorname(commandbinding.backgroundcolor, &colorname, &colorsymbol))
						colorname = x_("**UNKNOWN**");
					formatinfstr(infstr, x_(" [background=%s]"), colorname);
				}
				ttyputmsg(x_("%s"), returninfstr(infstr));
				us_freebindingparse(&commandbinding);
			}
		}

		/* if popup menu bindings are requested, list them */
		if (popup)
		{
			if (count <= 2) wantpm = NOPOPUPMENU; else
			{
				wantpm = us_getpopupmenu(par[2]);
				if (wantpm == NOPOPUPMENU)
				{
					us_abortcommand(M_("Cannot find popup menu '%s'"), par[2]);
					return;
				}
			}

			for(pm = us_firstpopupmenu; pm != NOPOPUPMENU; pm = pm->nextpopupmenu)
			{
				if (wantpm != NOPOPUPMENU && wantpm != pm) continue;
				ttyputmsg(_("Popup menu %s has %ld entries:"), pm->name, pm->total);
				for(i=0; i<pm->total; i++)
				{
					rb = pm->list[i].response;
					infstr = initinfstr();
					formatinfstr(infstr, M_("Entry %ld: "), i);
					if (rb->active < 0) addstringtoinfstr(infstr, M_("NOT DEFINED")); else
					{
						addstringtoinfstr(infstr, rb->comname);
						us_appendargs(infstr, rb);
					}
					if (rb->message != 0)
						formatinfstr(infstr, x_(" [message=\"%s\"]"), rb->message);
					ttyputmsg(x_("   %s"), returninfstr(infstr));
				}
			}
		}
		return;
	}

	if (namesamen(pt, x_("coverage"), l) == 0 && l >= 2)
	{
		/* determine coverage of the metal and polysilicon layers */
		np = us_needcell();
		if (np == NONODEPROTO) return;
		us_showlayercoverage(np);
		return;
	}

	if (namesamen(pt, x_("dates"), l) == 0 && l >= 1)
	{
		if (count < 2)
		{
			np = us_needcell();
			if (np == NONODEPROTO) return;
		} else
		{
			np = getnodeproto(par[1]);
			if (np == NONODEPROTO || np->primindex != 0)
			{
				us_abortcommand(_("No such cell: %s"), par[1]);
				return;
			}
		}

		/* give requested info */
		if (np->creationdate == 0)
			ttyputmsg(_("Cell %s has no recorded creation date"), describenodeproto(np)); else
				ttyputmsg(_("Cell %s was created %s"), describenodeproto(np),
					timetostring((time_t)np->creationdate));
		ttyputmsg(_("Version %ld was last revised %s"), np->version, timetostring((time_t)np->revisiondate));
		return;
	}

	if (namesamen(pt, x_("error"), l) == 0 && l >= 2)
	{
		if (count < 2)
		{
			ttyputusage(x_("show error (next | last)"));
			return;
		}
		l = estrlen(pt = par[1]);
		if (namesamen(pt, x_("next"), l) == 0)
		{
			ttyputmsg(x_("%s"), reportnexterror(1, 0, 0));
			return;
		}
		if (namesamen(pt, x_("last"), l) == 0)
		{
			ttyputmsg(x_("%s"), reportpreverror());
			return;
		}
		ttyputbadusage(x_("show error"));
		return;
	}

	if (namesamen(pt, x_("environment"), l) == 0 && l >= 2)
	{
		ttyputmsg(_("This is Electric, version %s"), el_version);
		pt = languagename();
		if (pt != NOSTRING) ttyputmsg(_("Includes built-in %s"), pt);
		ttyputmsg(_("Default library directory is %s"), el_libdir);

		if (count > 1 && namesamen(par[1], x_("authors"), estrlen(par[1])) == 0)
		{
			ttyputmsg(_("Electric was written by Steven M. Rubin"));
			ttyputmsg(_("   and a cast of thousands:"));
			for(i=0; us_castofthousands[i].name != 0; i++)
			ttyputmsg(x_("      %s"), us_castofthousands[i].name);
		}
		return;
	}

	if (namesamen(pt, x_("cells"), l) == 0 && l >= 2)
	{
		editlist = givedates = graphiclist = graphiclistlocal = FALSE;
		contentslist = notbelow = recursivenodes = placeholders = FALSE;
		dumpfilename = 0;
		np = getcurcell();
		lib = el_curlib;
		matchspec = 0;
		while (count >= 2)
		{
			l = estrlen(pt = par[1]);
			if (namesamen(pt, x_("dates"), l) == 0) givedates = TRUE; else
			if (namesamen(pt, x_("edit"), l) == 0) editlist = TRUE; else
			if (namesamen(pt, x_("graphically"), l) == 0) graphiclist = TRUE; else
			if (namesamen(pt, x_("from-here-graphically"), l) == 0 && l >= 2) graphiclistlocal = TRUE; else
			if (namesamen(pt, x_("placeholders"), l) == 0) placeholders = TRUE; else
			if (namesamen(pt, x_("contained-in-this"), l) == 0) contentslist = TRUE; else
			if (namesamen(pt, x_("recursive-nodes"), l) == 0) recursivenodes = TRUE; else
			if (namesamen(pt, x_("not-below"), l) == 0) notbelow = TRUE; else
			if (namesamen(pt, x_("matching"), l) == 0)
			{
				if (count < 3)
				{
					ttyputusage(x_("show cells matching MATCHSPEC"));
					return;
				}
				if (matchspec != 0)
				{
					us_abortcommand(_("Can only have one 'matching' clause"));
					return;
				}
				matchspec = par[2];
				for(pt = matchspec; *pt != 0; pt++)
					if (*pt == ':') break;
				if (*pt == ':')
				{
					*pt++ = 0;
					lib = getlibrary(matchspec);
					if (lib == NOLIBRARY)
					{
						us_abortcommand(_("Cannot find library '%s'"), matchspec);
						return;
					}
					matchspec = pt;
				}
				par++;
				count--;
			} else if (namesamen(pt, x_("library"), l) == 0)
			{
				if (count < 3)
				{
					ttyputusage(x_("show cells library LIBNAME"));
					return;
				}
				lib = getlibrary(par[2]);
				if (lib == NOLIBRARY)
				{
					us_abortcommand(_("No library called %s"), par[2]);
					return;
				}
				par++;
				count--;
			} else if (namesamen(pt, x_("file"), l) == 0 && l >= 2)
			{
				if (count < 3)
				{
					ttyputusage(x_("show cells file FILENAME"));
					return;
				}
				dumpfilename = par[2];
				par++;
				count--;
			} else
			{
				ttyputbadusage(x_("show cells"));
				return;
			}
			par++;
			count--;
		}

		if (contentslist)
		{
			if (givedates || editlist || graphiclist ||
				matchspec != 0 || dumpfilename != 0 || recursivenodes ||
				placeholders || graphiclistlocal || notbelow)
					ttyputerr(_("Other options for 'contained-in-this' ignored"));

			wnp = us_needcell();
			if (wnp == NONODEPROTO) return;
			for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
				for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
					np->temp1 = 0;
			for(ni = wnp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				np = ni->proto;
				if (np->primindex != 0) continue;
				np->temp1++;
			}
			first = TRUE;
			for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
				for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
					if (np->temp1 != 0)
			{
				if (first)
					ttyputmsg(_("Cell instances appearing in %s"), describenodeproto(wnp));
				first = FALSE;
				infstr = initinfstr();
				formatinfstr(infstr, _("   %ld instances of %s at"), np->temp1,
					describenodeproto(np));
				lambda = lambdaofcell(wnp);
				for(ni = wnp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				{
					if (ni->proto != np) continue;
					us_getnodedisplayposition(ni, &xp, &yp);
					formatinfstr(infstr, x_(" (%s,%s)"), latoa(xp, lambda), latoa(yp, lambda));
				}
				ttyputmsg(x_("%s"), returninfstr(infstr));
			}
			if (first)
				ttyputmsg(_("There are no cell instances in %s"), describenodeproto(wnp));
			return;
		}

		/* graph the cells if requested */
		if (graphiclist || graphiclistlocal)
		{
			if (givedates || editlist || contentslist || matchspec != 0 ||
				dumpfilename != 0 || recursivenodes || placeholders || notbelow)
					ttyputerr(M_("Other options for 'graphically' ignored"));
			np = NONODEPROTO;
			if (graphiclistlocal)
			{
				np = us_needcell();
				if (np == NONODEPROTO) return;
			}
			us_graphcells(np);
			return;
		}

		/* show contents of cell if requested */
		if (recursivenodes)
		{
			if (givedates || editlist || contentslist ||
				matchspec != 0 || dumpfilename != 0 || graphiclist ||
				placeholders || graphiclistlocal || notbelow)
					ttyputerr(M_("Other options for 'recursive-nodes' ignored"));
			np = us_needcell();
			if (np == NONODEPROTO) return;
			us_describecontents(np);
			return;
		}

		/* show all cells not used below this one */
		if (notbelow)
		{
			if (givedates || editlist || contentslist || matchspec != 0 ||
				dumpfilename != 0 || graphiclist || placeholders ||
				graphiclistlocal || recursivenodes)
					ttyputerr(M_("Other options for 'not-below' ignored"));
			wnp = us_needcell();
			if (wnp == NONODEPROTO) return;
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			{
				for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
					np->temp1 = 0;
			}
			us_recursivemark(wnp);
			first = TRUE;
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			{
				if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
				i = 0;
				infstr = 0;
				for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				{
					if (np->temp1 != 0) continue;
					if (i == 0)
					{
						infstr = initinfstr();
						formatinfstr(infstr, _("  From library %s:"), lib->libname);
					}
					formatinfstr(infstr, x_(" %s"), nldescribenodeproto(np));
					i++;
				}
				if (i != 0)
				{
					if (first)
					{
						ttyputmsg(_("These cells are not used by %s"), describenodeproto(wnp));
						first = FALSE;
					}
					ttyputmsg(x_("%s"), returninfstr(infstr));
				}
			}
			return;
		}

		/* show placeholders (cells created to handle cross-library input errors) if requested */
		if (placeholders)
		{
			if (givedates || editlist || contentslist || matchspec != 0 ||
				dumpfilename != 0 || recursivenodes || notbelow)
					ttyputerr(M_("Other options for 'placeholders' ignored"));
			i = 0;
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			{
				for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				{
					var = getval((INTBIG)np, VNODEPROTO, VSTRING, x_("IO_true_library"));
					if (var == NOVARIABLE) continue;
					ttyputmsg(_("Cell %s is a placeholder"), describenodeproto(np));
					i++;
				}
			}
			if (i == 0)
				ttyputmsg(_("There are no placeholder cells"));
			return;
		}

		/* allocate array for sorting entries */
		sortindex = us_sortlib(lib, matchspec);
		if (sortindex == 0)
		{
			ttyputnomemory();
			return;
		}

		/* compute the longest cell name and the number of cells */
		maxlen = 0;   total = 0;
		for(i=0; sortindex[i] != NONODEPROTO; i++)
		{
			l = estrlen(nldescribenodeproto(sortindex[i]));
			maxlen = maxi(maxlen, l);
			total++;
		}

		/* short list in columns */
		if (!givedates && !editlist)
		{
			if (dumpfilename != 0)
				ttyputerr(_("Cannot write to file unless 'dates' option specified"));
			ttyputmsg(_("----- Cells in library %s -----"), lib->libname);
			maxlen += 2;  columns = MESSAGESWIDTH / maxlen;
			if (columns <= 0) columns = 1;
			rows = (total + columns - 1) / columns;
			for(j=0; j<rows; j++)
			{
				infstr = initinfstr();
				for(k=0; k<columns; k++)
				{
					i = j + k*rows;
					if (i >= total) continue;
					np = sortindex[i];
					pt = nldescribenodeproto(np);
					addstringtoinfstr(infstr, pt);
					l = estrlen(pt);
					if (k != columns-1)
						for(i=l; i<maxlen; i++) addtoinfstr(infstr, ' ');
				}
				ttyputmsg(x_("%s"), returninfstr(infstr));
			}

			/* free the sort list */
			efree((CHAR *)sortindex);
			return;
		}

		/* full list with dates */
		if (givedates)
		{
			/* create the dump file if requested */
			if (dumpfilename != 0)
			{
				dumpfile = xopen(dumpfilename, el_filetypetext|FILETYPEWRITE, 0, &truename);
				if (dumpfile == 0)
				{
					us_abortcommand(_("Cannot write %s"), dumpfilename);
					return;
				}
				pt = timetostring(getcurrenttime());
				efprintf(dumpfile, _("List of cells in library %s created on %s\n"), lib->libname, pt);
				efprintf(dumpfile, _("Cell\tVersion\tCreation date\tRevision Date\tSize\tUsage\tLock\tInst-lock\tCell-lib\tDRC\tNCC\n"));
				for(j=0; j<total; j++)
					efprintf(dumpfile, x_("%s\n"), us_makecellline(sortindex[j], -1));
				xclose(dumpfile);
				ttyputmsg(_("Wrote %s"), truename);
			} else
			{
				maxlen = maxi(maxlen+2, 7);
				infstr = initinfstr();
				addstringtoinfstr(infstr, _("Cell"));
				for(i=5; i<maxlen; i++) addtoinfstr(infstr, '-');
				addstringtoinfstr(infstr, _("Version-----Creation date"));
				addstringtoinfstr(infstr, _("---------Revision Date------------Size-------Usage-L-I-C-D-N"));
				ttyputmsg(x_("%s"), returninfstr(infstr));
				for(j=0; j<total; j++)
					ttyputmsg(x_("%s"), us_makecellline(sortindex[j], maxlen));
			}

			/* free the sort list */
			efree((CHAR *)sortindex);
			return;
		}

		/* editable list */
		if (editlist)
		{
			if (dumpfilename != 0)
				ttyputerr(_("Cannot write to file when 'edit' option specified"));
			w = us_wantnewwindow(0);
			if (w == NOWINDOWPART) return;
			infstr = initinfstr();
			us_describeeditor(&name);
			addstringtoinfstr(infstr, name);
			addstringtoinfstr(infstr, _(" editor of cell names in library "));
			addstringtoinfstr(infstr, lib->libname);
			if (us_makeeditor(w, returninfstr(infstr), &idummy32, &idummy32) == NOWINDOWPART) return;
			w->charhandler = us_celledithandler;
			us_suspendgraphics(w);
			maxlen = maxi(maxlen+2, 7);
			infstr = initinfstr();
			addstringtoinfstr(infstr, _("Cell"));
			for(i=5; i<maxlen; i++) addtoinfstr(infstr, '-');
			addstringtoinfstr(infstr, _("Version-----Creation date"));
			addstringtoinfstr(infstr, _("---------Revision Date------------Size-------Usage-L-I-C-D-N"));
			us_addline(w, 0, returninfstr(infstr));
			for(j=0; j<total; j++)
				us_addline(w, j+1, us_makecellline(sortindex[j], maxlen));
			us_resumegraphics(w);
			us_describeeditor(&name);
			if (namesame(name, x_("emacs")) == 0)
				ttyputmsg(_("Use M(=) to edit the cell on the current line"));

			/* free the sort list */
			efree((CHAR *)sortindex);
			return;
		}
	}

	if (namesamen(pt, x_("history"), l) == 0 && l >= 1)
	{
		if (count <= 1)
		{
			showhistorylist(-1);
			return;
		}
		showhistorylist(myatoi(par[1]));
		return;
	}

	if (namesamen(pt, x_("libraries"), l) == 0 && l >= 1)
	{
		ttyputmsg(_("----- Libraries -----"));
		k = 0;
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		{
			if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
			infstr = initinfstr();
			addstringtoinfstr(infstr, lib->libname);
			if ((lib->userbits&(LIBCHANGEDMAJOR | LIBCHANGEDMINOR)) != 0)
			{
				addtoinfstr(infstr, '*');
				k++;
			}
			if (estrcmp(lib->libname, lib->libfile) != 0)
				formatinfstr(infstr, _(" (disk file: %s)"), lib->libfile);
			ttyputmsg(x_("%s"), returninfstr(infstr));

			/* see if there are dependencies */
			dummylibcount = 0;
			dummylibs = 0;
			for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
				olib->temp1 = 0;
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				{
					if (ni->proto->primindex != 0) continue;
					var = getval((INTBIG)ni->proto, VNODEPROTO, VSTRING, x_("IO_true_library"));
					if (var != NOVARIABLE)
					{
						pt = (CHAR *)var->addr;
						for(i=0; i<dummylibcount; i++)
							if (namesame(pt, dummylibs[i]) == 0) break;
						if (i >= dummylibcount)
						{
							newdummylibs = (CHAR **)emalloc((dummylibcount+1) * (sizeof (CHAR *)),
								us_tool->cluster);
							if (newdummylibs == 0) return;
							for(i=0; i<dummylibcount; i++)
								newdummylibs[i] = dummylibs[i];
							(void)allocstring(&newdummylibs[dummylibcount], pt, us_tool->cluster);
							if (dummylibcount > 0) efree((CHAR *)dummylibs);
							dummylibs = newdummylibs;
							dummylibcount++;
						}
					}
					ni->proto->lib->temp1 = 1;
				}
			}
			for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			{
				if (olib == lib) continue;
				if (olib->temp1 == 0) continue;
				infstr = initinfstr();
				formatinfstr(infstr, _("   Depends on library %s from cell(s):"),
					olib->libname);
				for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				{
					for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
					{
						if (ni->proto->primindex != 0) continue;
						if (ni->proto->lib == olib) break;
					}
					if (ni == NONODEINST) continue;
					formatinfstr(infstr, x_(" %s"), nldescribenodeproto(np));
				}
				ttyputmsg(x_("%s"), returninfstr(infstr));
			}
			if (dummylibcount > 0)
			{
				for(i=0; i<dummylibcount; i++)
				{
					infstr = initinfstr();
					formatinfstr(infstr, _("   Wanted unknown library %s from cell(s):"),
						dummylibs[i]);
					for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
					{
						for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
						{
							if (ni->proto->primindex != 0) continue;
							var = getval((INTBIG)ni->proto, VNODEPROTO, VSTRING, x_("IO_true_library"));
							if (var == NOVARIABLE) continue;
							pt = (CHAR *)var->addr;
							if (namesame(pt, dummylibs[i]) == 0) break;
						}
						if (ni == NONODEINST) continue;
						formatinfstr(infstr, x_(" %s"), nldescribenodeproto(np));
					}
					ttyputmsg(x_("%s"), returninfstr(infstr));
					efree(dummylibs[i]);
				}
				efree((CHAR *)dummylibs);
			}
		}
		if (k != 0) ttyputmsg(_("   (* means library has changed)"));
		return;
	}

	if (namesamen(pt, x_("macros"), l) == 0 && l >= 1)
	{
		if (count >= 2)
		{
			var = us_getmacro(par[1]);
			if (var != NOVARIABLE)
			{
				us_printmacro(var);
				return;
			}
			us_abortcommand(_("No macro named %s"), par[1]);
			return;
		}
		us_printmacros();
		return;
	}

	if (namesamen(pt, x_("networks"), l) == 0 && l >= 1)
	{
		np = us_needcell();
		if (np == NONODEPROTO) return;
		total = 0;
		unnamed = 0;
		for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		{
			if (net->namecount == 0 && net->globalnet < 0)
			{
				unnamed++;
				continue;
			}
			infstr = initinfstr();
			formatinfstr(infstr, x_("'%s'"), describenetwork(net));
			if (net->buswidth > 1)
			{
				formatinfstr(infstr, _(" (bus with %d signals)"), net->buswidth);
			}
			if (net->arccount == 0 && net->portcount == 0 && net->buslinkcount == 0)
				addstringtoinfstr(infstr, _(" not connected")); else
			{
				addstringtoinfstr(infstr, _(" connected on"));
				if (net->arccount != 0)
				{
					formatinfstr(infstr, x_(" %d %s"), net->arccount, makeplural(_("arc"), net->arccount));
				}
				if (net->portcount != 0)
				{
					if (net->arccount != 0) addstringtoinfstr(infstr, x_(", "));
					formatinfstr(infstr, x_(" %d %s"), net->portcount, makeplural(_("port"), net->portcount));
				}
				if (net->buslinkcount > 0)
				{
					if (net->arccount != 0 || net->portcount != 0) addstringtoinfstr(infstr, x_(", "));
					formatinfstr(infstr, x_(" %d "), net->buslinkcount);
					if (net->buslinkcount != 1) addstringtoinfstr(infstr, _("busses")); else
						addstringtoinfstr(infstr, _("bus"));
				}
			}
			ttyputmsg(_("Network %s"), returninfstr(infstr));
			total++;
		}
		if (unnamed != 0)
		{
			if (total == 0)
			{
				ttyputmsg(_("Cell has %ld unnamed %s"), unnamed,
					makeplural(x_("network"), unnamed));
			} else
			{
				ttyputmsg(_("Plus %ld unnamed %s"), unnamed, makeplural(x_("network"), unnamed));
			}
		} else
		{
			if (total == 0) ttyputmsg(_("There are no named networks"));
		}
		return;
	}

	if (namesamen(pt, x_("object"), l) == 0 && l >= 1)
	{
		verbose = 0;
		if (count > 1)
		{
			l = estrlen(par[1]);
			if (namesamen(par[1], x_("short"), l) == 0 && l >= 1) verbose = -1;
			if (namesamen(par[1], x_("long"), l) == 0 && l >= 1) verbose = 1;
		}

		high = us_getonehighlight();
		if (high == NOHIGHLIGHT) return;

		if ((high->status&HIGHTYPE) == HIGHTEXT)
		{
			/* describe highlighted text */
			if (high->fromvar != NOVARIABLE)
			{
				ttyputmsg(_("%s variable '%s' is on %s"), us_variableattributes(high->fromvar, -1),
					describevariable(high->fromvar, -1, -1), geomname(high->fromgeom));
				return;
			}
			if (high->fromport != NOPORTPROTO)
			{
				infstr = initinfstr();
				activity = describeportbits(high->fromport->userbits);
				if (*activity != 0)
				{
					formatinfstr(infstr, _("%s port name '%s' is on %s (label %s"), activity,
						high->fromport->protoname, geomname(high->fromgeom),
							us_describetextdescript(high->fromport->textdescript));
				} else
				{
					formatinfstr(infstr, _("Port name '%s' is on %s (label %s"),
						high->fromport->protoname, geomname(high->fromgeom),
							us_describetextdescript(high->fromport->textdescript));
				}
				if ((high->fromport->userbits&PORTDRAWN) != 0)
					addstringtoinfstr(infstr, _(",always-drawn"));
				if ((high->fromport->userbits&BODYONLY) != 0)
					addstringtoinfstr(infstr, _(",only-on-body"));
				addstringtoinfstr(infstr, x_(") "));
				ttyputmsg(x_("%s"), returninfstr(infstr));
				return;
			}
			if (high->fromgeom->entryisnode)
			{
				ni = high->fromgeom->entryaddr.ni;
				infstr = initinfstr();
				formatinfstr(infstr, _("Cell name '%s' (label %s)"), describenodeproto(ni->proto),
					us_describetextdescript(ni->textdescript));
				ttyputmsg(x_("%s"), returninfstr(infstr));
				return;
			}
		}
		if ((high->status&HIGHTYPE) != HIGHFROM)
		{
			us_abortcommand(_("Find a single node or arc first"));
			return;
		}

		/* describe a nodeinst */
		if (high->fromgeom->entryisnode)
		{
			/* give basic information about nodeinst */
			ni = high->fromgeom->entryaddr.ni;
			np = ni->proto;
			lambda = lambdaofnode(ni);
			nodesizeoffset(ni, &plx, &ply, &phx, &phy);
			infstr = initinfstr();
			if (np->primindex != 0)
			{
				formatinfstr(infstr, _("Node %s is %sx%s"), describenodeinst(ni),
					latoa(ni->highx-ni->lowx-plx-phx, lambda), latoa(ni->highy-ni->lowy-ply-phy, lambda));
			} else
			{
				formatinfstr(infstr, _("Cell %s is %sx%s"), describenodeinst(ni),
					latoa(ni->highx-ni->lowx-plx-phx, lambda), latoa(ni->highy-ni->lowy-ply-phy, lambda));
			}

			/* special case for serpentine transistors: print true size */
			if (np->primindex != 0 && (np->userbits&HOLDSTRACE) != 0)
			{
				fun = (np->userbits&NFUNCTION) >> NFUNCTIONSH;
				if (fun == NPTRANMOS || fun == NPTRADMOS || fun == NPTRAPMOS)
				{
					var = gettrace(ni);
					if (var != NOVARIABLE)
					{
						transistorsize(ni, &len, &wid);
						if (len != -1 && wid != -1)
							formatinfstr(infstr, _(" (actually %sx%s)"), latoa(len, lambda),
								latoa(wid, lambda));
					}
				}
			}
			addstringtoinfstr(infstr, x_(", "));
			if (ni->transpose != 0) addstringtoinfstr(infstr, _("transposed and "));
			formatinfstr(infstr, _("rotated %s, center (%s,%s)"), frtoa(ni->rotation*WHOLE/10),
				latoa((ni->highx+ni->lowx)/2, lambda), latoa((ni->highy+ni->lowy)/2, lambda));
			ttyputmsg(x_("%s"), returninfstr(infstr));

			/* reset the "chat" indicator on the port prototypes */
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				pp->temp1 = 0;

			/* always describe the highlighted port */
			if (high->fromport != NOPORTPROTO)
				us_chatportproto(ni, high->fromport);

			/* describe all arcs and ports if not short option */
			if (verbose >= 0)
			{
				for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
					us_chatportproto(ni, pe->proto);
				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
					us_chatportproto(ni, pi->proto);
			}

			/* long option: describe tool information, variables, etc */
			if (verbose > 0)
			{
				/* talk about every port in long option */
				for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
					us_chatportproto(ni, pp);

				/* print tool information */
				us_printnodetoolinfo(ni);

				/* print variables */
				for(i=0; i<ni->numvar; i++)
					ttyputmsg(_("%s variable '%s' is %s"), us_variableattributes(&ni->firstvar[i], -1),
						makename(ni->firstvar[i].key), describevariable(&ni->firstvar[i], -1, -1));

				/* describe contents */
				if (np->primindex == 0) us_describecontents(ni->proto);
			}
		} else if (!high->fromgeom->entryisnode)
		{
			/* print the basic information about the arcinst */
			ai = high->fromgeom->entryaddr.ai;
			lambda = lambdaofarc(ai);

			/* compute the arc length and width */
			wid = ai->width - arcwidthoffset(ai);
			len = ai->length;
			if ((ai->userbits&NOEXTEND) == 0) len += wid; else
			{
				if ((ai->userbits&NOTEND0) != 0) len += wid/2;
				if ((ai->userbits&NOTEND1) != 0) len += wid/2;
			}

			/* build a string of constraints and properties */
			infstr = initinfstr();
			formatinfstr(infstr, _("%s arc is "), describearcinst(ai));
			if (el_curconstraint == cla_constraint)
			{
				if (((ai->userbits&FIXED) == 0 || ai->changed == cla_changeclock+3) &&
					ai->changed != cla_changeclock+2)
				{
					if (ai->changed == cla_changeclock+3) addstringtoinfstr(infstr, _("temporarily "));
					addstringtoinfstr(infstr, _("stretchable, "));
					if ((ai->userbits&FIXANG) == 0) addstringtoinfstr(infstr, _("not fixed-angle, ")); else
						addstringtoinfstr(infstr, _("fixed-angle, "));
					if ((ai->userbits&CANTSLIDE) != 0) addstringtoinfstr(infstr, _("nonslidable, ")); else
						addstringtoinfstr(infstr, _("slidable, "));
				} else
				{
					if (ai->changed == cla_changeclock+2) addstringtoinfstr(infstr, _("temporarily "));
					addstringtoinfstr(infstr, _("rigid, "));
				}
			} else
			{
				pt = (CHAR *)(*(el_curconstraint->request))(x_("describearc"), (INTBIG)ai);
				if (*pt != 0)
					formatinfstr(infstr, _("constrained to %s, "));
			}
			if ((ai->userbits&ISNEGATED) != 0) addstringtoinfstr(infstr, _("negated, "));
			if ((ai->userbits&ISDIRECTIONAL) != 0) addstringtoinfstr(infstr, _("directional, "));
			if ((ai->userbits&(NOTEND0|NOTEND1)) != 0)
			{
				addstringtoinfstr(infstr, _("with "));
				switch (ai->userbits & (NOTEND0|NOTEND1))
				{
					case NOTEND0:         addstringtoinfstr(infstr, _("tail"));             break;
					case NOTEND1:         addstringtoinfstr(infstr, _("head"));             break;
					case NOTEND0|NOTEND1: addstringtoinfstr(infstr, _("head and tail"));    break;
				}
				addstringtoinfstr(infstr, _(" skipped, "));
			}
			if ((ai->userbits&REVERSEEND) != 0) addstringtoinfstr(infstr, _("ends reversed, "));

			/* add bus width */
			if (ai->network != NONETWORK && ai->network->buswidth > 1)
				formatinfstr(infstr, _("has %d signals, "), ai->network->buswidth);

			/* add in width and length */
			formatinfstr(infstr, _("%s wide and %s long"), latoa(wid, lambda), latoa(len, lambda));

			/* if the arc is unnamed but on a network, report this */
			var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
			if (var == NOVARIABLE)
			{
				if (ai->network != NONETWORK && ai->network->namecount != 0)
				{
					formatinfstr(infstr, _(", on network %s"), describenetwork(ai->network));
				}
			}

			/* print the message */
			ttyputmsg(x_("%s"), returninfstr(infstr));

			/* tell about the ends if default or long option */
			if (verbose >= 0)
			{
				ttyputmsg(_("Tail on port %s of %s at (%s,%s) runs %ld degrees to"),
					ai->end[0].portarcinst->proto->protoname, describenodeinst(ai->end[0].nodeinst),
						latoa(ai->end[0].xpos, lambda), latoa(ai->end[0].ypos, lambda),
							(ai->userbits & AANGLE) >> AANGLESH);
				ttyputmsg(_("Head on port %s of %s at (%s,%s)"),
					ai->end[1].portarcinst->proto->protoname, describenodeinst(ai->end[1].nodeinst),
						latoa(ai->end[1].xpos, lambda), latoa(ai->end[1].ypos, lambda));
			}

			/* tell about tool values and variables if long option */
			if (verbose > 0)
			{
				/* print tool information */
				us_printarctoolinfo(ai);

				/* print variables */
				for(i=0; i<ai->numvar; i++)
					ttyputmsg(_("%s variable '%s' is %s"), us_variableattributes(&ai->firstvar[i], -1),
						makename(ai->firstvar[i].key), describevariable(&ai->firstvar[i], -1, -1));
			}
		}
		return;
	}

	if (namesamen(pt, x_("ports"), l) == 0 && l >= 2)
	{
		np = us_needcell();
		if (np == NONODEPROTO) return;
		numtypes = 0;
		summarize = TRUE;
		if (count > 1) summarize = FALSE;
		if (count == 1) par[count++] = x_("all");
		for (i=1; i<count; i++)
		{
			l = estrlen(par[i]);
			if (namesamen(par[i], x_("clock"), l) == 0 && l >= 1)
			{
				porttype[numtypes++] = CLKPORT;		/* basic clock */
				porttype[numtypes++] = C1PORT;		/* clock phase 1 */
				porttype[numtypes++] = C2PORT;		/* clock phase 2 */
				porttype[numtypes++] = C3PORT;		/* clock phase 3 */
				porttype[numtypes++] = C4PORT;		/* clock phase 4 */
				porttype[numtypes++] = C5PORT;		/* clock phase 5 */
				porttype[numtypes++] = C6PORT;		/* clock phase 6 */
			} else if (namesamen(par[i], x_("input"), l) == 0 && l >= 1)
				porttype[numtypes++] = INPORT;		/* input */
			else if (namesamen(par[i], x_("output"), l) == 0 && l >= 1)
				porttype[numtypes++] = OUTPORT;		/* output */
			else if (namesamen(par[i], x_("bidirectional"), l) == 0 && l >= 1)
				porttype[numtypes++] = BIDIRPORT;	/* bidirectional */
			else if (namesamen(par[i], x_("power"), l) == 0 && l >= 1)
				porttype[numtypes++] = PWRPORT;		/* power */
			else if (namesamen(par[i], x_("ground"), l) == 0 && l >= 2)
				porttype[numtypes++] = GNDPORT;		/* ground */
			else if (namesamen(par[i], x_("reference"), l) == 0 && l >= 1)
			{
				porttype[numtypes++] = REFOUTPORT;	/* ref out */
				porttype[numtypes++] = REFINPORT;	/* ref in */
				porttype[numtypes++] = REFBASEPORT;	/* ref base */
			} else if (namesamen(par[i], x_("generic"), l) == 0 && l >= 2)
				porttype[numtypes++] = 0;
			else if (namesamen(par[i], x_("all"), l) == 0 && l >= 1)
			{
				porttype[numtypes++] = 0;			/* generic */
				porttype[numtypes++] = CLKPORT;		/* basic clock */
				porttype[numtypes++] = C1PORT;		/* clock phase 1 */
				porttype[numtypes++] = C2PORT;		/* clock phase 2 */
				porttype[numtypes++] = C3PORT;		/* clock phase 3 */
				porttype[numtypes++] = C4PORT;		/* clock phase 4 */
				porttype[numtypes++] = C5PORT;		/* clock phase 5 */
				porttype[numtypes++] = C6PORT;		/* clock phase 6 */
				porttype[numtypes++] = INPORT;		/* input */
				porttype[numtypes++] = OUTPORT;		/* output */
				porttype[numtypes++] = BIDIRPORT;	/* bidirectional */
				porttype[numtypes++] = PWRPORT;		/* power */
				porttype[numtypes++] = GNDPORT;		/* ground */
				porttype[numtypes++] = REFOUTPORT;	/* ref out */
				porttype[numtypes++] = REFINPORT;	/* ref in */
				porttype[numtypes++] = REFBASEPORT;	/* ref base */
			} else
			{
				 ttyputbadusage(x_("show port"));
				 return;
			}
		}

		/* compute the associated cell to check */
		wnp = contentsview(np);
		if (wnp == NONODEPROTO) wnp = iconview(np);
		if (wnp == np) wnp = NONODEPROTO;

		/* count the number of exports */
		num_found = 0;
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			m = pp->userbits & STATEBITS;
			for(i=0; i<numtypes; i++) if (porttype[i] == m) break;
			if (i >= numtypes) continue;
			num_found++;
		}
		if (num_found == 0)
		{
			ttyputmsg(_("There are no exports on cell %s"), describenodeproto(np));
			return;
		}

		/* make a list of exports */
		pplist = (PORTPROTO **)emalloc(num_found * (sizeof (PORTPROTO *)), el_tempcluster);
		if (pplist == 0) return;
		equivlist = (INTBIG *)emalloc(num_found * SIZEOFINTBIG, el_tempcluster);
		if (equivlist == 0) return;
		buslist = (INTBIG *)emalloc(num_found * SIZEOFINTBIG, el_tempcluster);
		if (buslist == 0) return;
		num_found = 0;
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			m = pp->userbits & STATEBITS;
			for(i=0; i<numtypes; i++) if (porttype[i] == m) break;
			if (i >= numtypes) continue;
			pplist[num_found] = pp;
			equivlist[num_found] = -1;
			buslist[num_found] = -1;
			num_found++;
		}

		/* sort exports by name within type */
		esort(pplist, num_found, sizeof (PORTPROTO *), us_exportnametypeascending);

		/* if summarizing, make associations that combine exports */
		if (summarize)
		{
			/* make associations among electrically equivalent exports */
			for(j=0; j<num_found; j++)
			{
				if (equivlist[j] != -1 || buslist[j] != -1) continue;
				for(k=j+1; k<num_found; k++)
				{
					if (equivlist[k] != -1 || buslist[k] != -1) continue;
					if ((pplist[j]->userbits&STATEBITS) != (pplist[k]->userbits&STATEBITS))
						break;
					if (pplist[j]->network != pplist[k]->network) continue;
					equivlist[k] = j;
					equivlist[j] = -2;
				}
			}

			/* make associations among bussed exports */
			for(j=0; j<num_found; j++)
			{
				if (equivlist[j] != -1 || buslist[j] != -1) continue;
				for(k=j+1; k<num_found; k++)
				{
					if (equivlist[k] != -1 || buslist[k] != -1) continue;
					if ((pplist[j]->userbits&STATEBITS) != (pplist[k]->userbits&STATEBITS))
						break;
					pt1 = pplist[j]->protoname;
					pt2 = pplist[k]->protoname;
					for(;;)
					{
						if (tolower(*pt1) != tolower(*pt2)) break;
						if (*pt1 == '[') break;
						pt1++;   pt2++;
					}
					if (*pt1 != '[' || *pt2 != '[') continue;
					buslist[k] = j;
					buslist[j] = -2;
				}
			}
		}

		/* describe each export */
		lambda = lambdaofcell(np);
		ttyputmsg(_("----- Exports on cell %s -----"), describenodeproto(np));
		for(j=0; j<num_found; j++)
		{
			if (equivlist[j] >= 0 || buslist[j] >= 0) continue;
			pp = pplist[j];

			/* reset flags for arcs that can connect */
			for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
				for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
					ap->temp1 = 0;

			infstr = initinfstr();
			activity = describeportbits(pp->userbits);
			if (*activity == 0) activity = _("Unknown");
			for(k=j+1; k<num_found; k++) if (equivlist[k] == j) break;
			lx = hx = ly = hy = 0;
			if (k < num_found)
			{
				/* many exports that are electrically equivalent */
				formatinfstr(infstr, _("%s exports "), activity);
				for(k=j; k<num_found; k++)
				{
					if (j != k && equivlist[k] != j) continue;
					if (j != k) addstringtoinfstr(infstr, x_(", "));
					opp = pplist[k];
					formatinfstr(infstr, x_("'%s'"), opp->protoname);
					portposition(opp->subnodeinst, opp->subportproto, &xp, &yp);
					if (j == k)
					{
						lx = hx = xp;   ly = hy = yp;
					} else
					{
						if (xp < lx) lx = xp;
						if (xp > hx) hx = xp;
						if (yp < ly) ly = yp;
						if (yp > hy) hy = yp;
					}
					for(i=0; opp->connects[i] != NOARCPROTO; i++)
						opp->connects[i]->temp1 = 1;
				}
				formatinfstr(infstr, _(" at (%s<=X<=%s, %s<=Y<=%s), electrically connected to"),
					latoa(lx, lambda), latoa(hx, lambda), latoa(ly, lambda), latoa(hy, lambda));
				us_addpossiblearcconnections(infstr);
			} else
			{
				for(k=j+1; k<num_found; k++) if (buslist[k] == j) break;
				if (k < num_found)
				{
					/* many exports from the same bus */
					tot = 0;
					for(k=j; k<num_found; k++)
					{
						if (j != k && buslist[k] != j) continue;
						tot++;
						opp = pplist[k];
						portposition(opp->subnodeinst, opp->subportproto, &xp, &yp);
						if (j == k)
						{
							lx = hx = xp;   ly = hy = yp;
						} else
						{
							if (xp < lx) lx = xp;
							if (xp > hx) hx = xp;
							if (yp < ly) ly = yp;
							if (yp > hy) hy = yp;
						}
						for(i=0; opp->connects[i] != NOARCPROTO; i++)
							opp->connects[i]->temp1 = 1;
					}
					sortedbuslist = (PORTPROTO **)emalloc(tot * (sizeof (PORTPROTO *)),
						el_tempcluster);
					if (sortedbuslist == 0) return;
					tot = 0;
					sortedbuslist[tot++] = pplist[j];
					for(k=j+1; k<num_found; k++)
						if (buslist[k] == j) sortedbuslist[tot++] = pplist[k];

					/* sort the bus by indices */
					esort(pplist, tot, sizeof (PORTPROTO *), us_exportnameindexascending);

					pt1 = sortedbuslist[0]->protoname;
					while (*pt1 != 0 && *pt1 != '[') pt1++;
					k = *pt1;   *pt1 = 0;
					formatinfstr(infstr, _("%s ports '%s["), activity, sortedbuslist[0]->protoname);
					*pt1 = (CHAR)k;
					for(k=0; k<tot; k++)
					{
						if (k != 0) addstringtoinfstr(infstr, x_(","));
						pt1 = sortedbuslist[k]->protoname;
						while (*pt1 != 0 && *pt1 != '[') pt1++;
						if (*pt1 == '[') pt1++;
						while (*pt1 != 0 && *pt1 != ']') addtoinfstr(infstr, *pt1++);
					}
					formatinfstr(infstr, _("] at (%s<=X<=%s, %s<=Y<=%s), same bus, connects to"),
						latoa(lx, lambda), latoa(hx, lambda), latoa(ly, lambda), latoa(hy, lambda));
					us_addpossiblearcconnections(infstr);
					efree((CHAR *)sortedbuslist);
				} else
				{
					/* isolated export */
					portposition(pp->subnodeinst, pp->subportproto, &xp, &yp);
					formatinfstr(infstr, _("%s export '%s' at (%s, %s) connects to"), activity, pp->protoname,
						latoa(xp, lambda), latoa(yp, lambda));
					for(i=0; pp->connects[i] != NOARCPROTO; i++)
						pp->connects[i]->temp1 = 1;
					us_addpossiblearcconnections(infstr);

					/* check for the export in the associated cell */
					if (wnp != NONODEPROTO)
					{
						if (equivalentport(np, pp, wnp) == NOPORTPROTO)
							formatinfstr(infstr, _(" *** no equivalent in %s"), describenodeproto(wnp));
					}
				}
			}

			str = returninfstr(infstr);
			prefix = x_("");
			while (estrlen(str) > 80)
			{
				for(i=80; i > 0; i--) if (str[i] == ' ' || str[i] == ',') break;
				if (i <= 0) i = 80;
				if (str[i] == ',') i++;
				save = str[i];
				str[i] = 0;
				ttyputmsg(x_("%s%s"), prefix, str);
				str[i] = save;
				str = &str[i];
				if (str[0] == ' ') str++;
				prefix = x_("   ");
			}
			ttyputmsg(x_("%s%s"), prefix, str);
		}
		if (wnp != NONODEPROTO)
		{
			for(pp = wnp->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			{
				m = pp->userbits & STATEBITS;
				for(i=0; i<numtypes; i++) if (porttype[i] == m) break;
				if (i >= numtypes) continue;
				if (equivalentport(wnp, pp, np) == NOPORTPROTO)
					ttyputmsg(_("*** Export %s, found in cell %s, is missing here"),
						pp->protoname, describenodeproto(wnp));
			}
		}
		efree((CHAR *)pplist);
		efree((CHAR *)equivlist);
		efree((CHAR *)buslist);
		return;
	}

	if (namesamen(pt, x_("primitives"), l) == 0 && l >= 2)
	{
		/* list the primitive cell names */
		ttyputmsg(_("----- Primitive Node Prototypes (%s) -----"), el_curtech->techname);
		maxlen = 0;
		for(np = el_curtech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			maxlen = maxi(maxlen, estrlen(np->protoname));
		maxlen += 2;  columns = MESSAGESWIDTH / maxlen;
		total = 0;
		for(np = el_curtech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if ((total % columns) == 0) (void)estrcpy(line, np->protoname); else
				(void)estrcat(line, np->protoname);
			if (np->nextnodeproto != NONODEPROTO && (total%columns) != columns-1)
				for(i = estrlen(np->protoname); i < maxlen; i++)
					(void)estrcat(line, x_(" "));
			if ((total % columns) == columns-1) ttyputmsg(line);
			total++;
		}
		if ((total % columns) != 0) ttyputmsg(line);
		return;
	}

	if (namesamen(pt, x_("solvers"), l) == 0 && l >= 2)
	{
		ttyputmsg(_("Current constraint solver is %s (%s)"), el_curconstraint->conname,
			TRANSLATE(el_curconstraint->condesc));
		ttyputmsg(_("Other constraint solvers:"));
		for(i=0; el_constraints[i].conname != 0; i++)
		{
			con = &el_constraints[i];
			if (con == el_curconstraint) continue;
			ttyputmsg(x_("  %s (%s)"), con->conname, TRANSLATE(con->condesc));
		}
		return;
	}

	if (namesamen(pt, x_("technologies"), l) == 0 && l >= 1)
	{
		ttyputmsg(_("Current technology is %s"), el_curtech->techname);
		ttyputmsg(_("----- Technologies -----"));
		for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
			ttyputmsg(x_("%-9s %s"), tech->techname, tech->techdescript);
		return;
	}

	if (namesamen(pt, x_("usage"), l) == 0 && l >= 1)
	{
		if (count <= 1)
		{
			ttyputusage(x_("show usage CELL"));
			return;
		}
		np = getnodeproto(par[1]);
		if (np == NONODEPROTO || np->primindex != 0)
		{
			us_abortcommand(_("'%s' is not a cell"), par[1]);
			return;
		}
		if (np->firstinst == NONODEINST)
		{
			ttyputmsg(_("Cell %s is not used anywhere"), describenodeproto(np));
			return;
		}

		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			for(wnp = olib->firstnodeproto; wnp != NONODEPROTO; wnp = wnp->nextnodeproto)
				wnp->temp1 = 0;
		for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
			ni->parent->temp1++;
		ttyputmsg(_("Cell %s is used in these locations:"), describenodeproto(np));
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			for(wnp = olib->firstnodeproto; wnp != NONODEPROTO; wnp = wnp->nextnodeproto)
				if (wnp->temp1 != 0)
					ttyputmsg(_("  %ld %s in cell %s"), wnp->temp1,
						makeplural(_("instance"), wnp->temp1), describenodeproto(wnp));
		return;
	}

	if (namesamen(pt, x_("views"), l) == 0 && l >= 1)
	{
		if (count == 1)
		{
			ttyputmsg(_("Current views:"));
			for(v = el_views; v != NOVIEW; v = v->nextview)
				ttyputmsg(_("%s (short name %s)"), v->viewname, v->sviewname);
			return;
		}
		np = getnodeproto(par[1]);
		if (np == NONODEPROTO || np->primindex != 0)
		{
			us_abortcommand(_("'%s' is not a cell"), par[1]);
			return;
		}

		infstr = initinfstr();
		first = FALSE;
		FOR_CELLGROUP(wnp, np)
		{
			if (first) addstringtoinfstr(infstr, x_(", "));
			first = TRUE;
			addstringtoinfstr(infstr, describenodeproto(wnp));
		}
		ttyputmsg(_("View contains: %s"), returninfstr(infstr));
		return;
	}
	ttyputbadusage(x_("show"));
}

void us_size(INTBIG count, CHAR *par[])
{
	REGISTER BOOLEAN serptrans, justnodes, justarcs, usetransformation, flipxy;
	REGISTER INTBIG i, j, k, l, dxs, dys, lx, hx, ly, hy, dist, bestdist, otherx, othery,
		rot, wid, dx, dy, fixedcorner, nc, ac, tot, edgealignment, *drot, *dtran, fun,
		cursorbased, isin, otherin, nodecount, arccount, bits, lambda;
	INTBIG offxl, offyl, offxh, offyh, truewid, rx, ry, orx, ory,
		otheralign, xcur, ycur, xs, ys, *dlxs, *dlys, *dhxs, *dhys;
	XARRAY trans;
	REGISTER NODEINST *ni, *nodetosize, **nis;
	REGISTER ARCINST *ai;
	REGISTER NODEPROTO *np;
	REGISTER CHAR *pp;
	REGISTER GEOM **list, *geom;
	REGISTER VARIABLE *var;
	static POLYGON *poly = NOPOLYGON;
	extern COMCOMP us_sizep, us_sizeyp, us_sizewp;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* get options */
	cursorbased = 0;
	justnodes = justarcs = FALSE;
	l = 0;
	pp = 0;
	while (count > 0)
	{
		l = estrlen(pp = par[0]);
		if (namesamen(pp, x_("corner-fixed"), l) == 0 && l >= 2)
		{
			cursorbased = 1;
			count--;
			par++;
			continue;
		}
		if (namesamen(pp, x_("center-fixed"), l) == 0 && l >= 2)
		{
			cursorbased = 2;
			count--;
			par++;
			continue;
		}
		if (namesamen(pp, x_("grab-point-fixed"), l) == 0)
		{
			us_abortcommand(_("Cannot size about the grab-point yet"));
			return;
		}
		if (namesamen(pp, x_("nodes"), l) == 0)
		{
			justnodes = TRUE;
			count--;
			par++;
			continue;
		}
		if (namesamen(pp, x_("arcs"), l) == 0)
		{
			justarcs = TRUE;
			count--;
			par++;
			continue;
		}
		break;
	}

	nodetosize = NONODEINST;
	list = us_gethighlighted(WANTARCINST|WANTNODEINST, 0, 0);
	if (list[0] == NOGEOM)
	{
		us_abortcommand(_("First select nodes or arcs to resize"));
		return;
	}

	/* disallow sizing if lock is on */
	np = geomparent(list[0]);
	if (us_cantedit(np, NONODEINST, TRUE)) return;

	/* see how many are selected */
	nodecount = arccount = 0;
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (list[i]->entryisnode)
		{
			nodecount++;
			nodetosize = list[i]->entryaddr.ni;
			if (!justarcs && nodetosize->proto->primindex == 0)
			{
				us_abortcommand(_("Cannot change cell size"));
				return;
			}
		} else
		{
			arccount++;
		}
	}

	/* see if a serpentine transistor is being scaled */
	serptrans = FALSE;
	if (nodecount == 1 && arccount == 0 && !justarcs)
	{
		if (us_cantedit(np, nodetosize, TRUE)) return;
		if (nodetosize->proto->primindex != 0 && (nodetosize->proto->userbits&HOLDSTRACE) != 0)
		{
			fun = (nodetosize->proto->userbits&NFUNCTION) >> NFUNCTIONSH;
			if (fun == NPTRANMOS || fun == NPTRADMOS || fun == NPTRAPMOS)
			{
				var = gettrace(nodetosize);
				if (var != NOVARIABLE) serptrans = TRUE;
			}
		}
	}

	if (cursorbased != 0)
	{
		nc = nodecount;   ac = arccount;
		if (justnodes) ac = 0;
		if (justarcs) nc = 0;
		if (nc + ac != 1)
		{
			us_abortcommand(_("Can only size one node or arc at a time"));
			return;
		}
	}

	if (count <= 0)
	{
		if ((nodecount > 1 || justnodes) && !justarcs)
		{
			if (serptrans)
				count = ttygetparam(M_("Transistor width: "), &us_sizep, MAXPARS, par); else
					count = ttygetparam(M_("X size: "), &us_sizep, MAXPARS, par);
			if (count == 0)
			{
				us_abortedmsg();
				return;
			}
			l = estrlen(pp = par[0]);
		} else if ((arccount > 1 || justarcs) && !justnodes)
		{
			count = ttygetparam(M_("Width: "), &us_sizewp, MAXPARS, par);
			if (count == 0)
			{
				us_abortedmsg();
				return;
			}
			l = estrlen(pp = par[0]);
		}
	}

	if ((nodecount != 0 || justnodes) && !justarcs)
	{
		if (cursorbased != 0)
		{
			if (serptrans)
			{
				us_abortcommand(_("No cursor scaling on serpentine transistors"));
				ttyputmsg(_("Use explicit width instead"));
				return;
			}

			/* save highlighting */
			us_pushhighlight();
			us_clearhighlightcount();

			/* figure out the new size of the nodeinst */
			nodesizeoffset(nodetosize, &offxl, &offyl, &offxh, &offyh);
			lx = nodetosize->lowx+offxl;   hx = nodetosize->highx-offxh;
			ly = nodetosize->lowy+offyl;   hy = nodetosize->highy-offyh;

			if (cursorbased == 2)
			{
				/* center of node is fixed and all corners stretch */
				if (us_demandxy(&xcur, &ycur)) return;
				gridalign(&xcur, &ycur, 2, np);

				/* adjust the cursor position if selecting interactively */
				if ((us_tool->toolstate&INTERACTIVE) != 0)
				{
					us_sizeinit(nodetosize);
					trackcursor(FALSE, us_ignoreup, us_nullvoid, us_sizecdown,
						us_stopandpoponchar, us_dragup, TRACKDRAGGING);
					if (el_pleasestop != 0) return;
					if (us_demandxy(&xcur, &ycur)) return;
					gridalign(&xcur, &ycur, 2, np);
				}

				/* transform cursor to account for node orientation */
				makeangle((3600 - nodetosize->rotation)%3600, nodetosize->transpose, trans);
				xform(xcur-(hx+lx)/2, ycur-(hy+ly)/2, &rx, &ry, trans);
				xcur = (hx+lx)/2 + rx;
				ycur = (hy+ly)/2 + ry;

				/* compute new size of the nodeinst */
				j = (hx+lx)/2;   i = abs(j - xcur);
				lx = j-i;   hx = j+i;
				j = (hy+ly)/2;   i = abs(j - ycur);
				ly = j-i;   hy = j+i;
				lx -= offxl;   hx += offxh;
				ly -= offyl;   hy += offyh;
			} else if (cursorbased == 1)
			{
				/* closest corner of node stretches to cursor */
				/* adjust the cursor position if selecting interactively */
				fixedcorner = -1;
				if ((us_tool->toolstate&INTERACTIVE) != 0)
				{
					us_sizeinit(nodetosize);
					trackcursor(TRUE, us_sizedown, us_nullvoid, us_sizedown,
						us_stopandpoponchar, us_dragup, TRACKDRAGGING);
					if (el_pleasestop != 0) return;
					if (us_demandxy(&xcur, &ycur)) return;
					gridalign(&xcur, &ycur, 1, np);
					fixedcorner = us_sizeterm();
				}
				if (us_demandxy(&xcur, &ycur)) return;
				gridalign(&xcur, &ycur, 1, np);
				makerot(nodetosize, trans);

				/* determine which corner is fixed */
				if (fixedcorner < 0)
				{
					xform(lx, ly, &rx, &ry, trans);
					bestdist = abs(rx - xcur) + abs(ry - ycur);
					fixedcorner = 1;	/* lower-left */
					xform(hx, ly, &rx, &ry, trans);
					dist = abs(rx - xcur) + abs(ry - ycur);
					if (dist < bestdist)
					{
						bestdist = dist;   fixedcorner = 2;	/* lower-right */
					}
					xform(lx, hy, &rx, &ry, trans);
					dist = abs(rx - xcur) + abs(ry - ycur);
					if (dist < bestdist)
					{
						bestdist = dist;   fixedcorner = 3;	/* upper-left */
					}
					xform(hx, hy, &rx, &ry, trans);
					dist = abs(rx - xcur) + abs(ry - ycur);
					if (dist < bestdist) fixedcorner = 4;	/* upper-right */
				}

				/* check the lower-left corner */
				bits = getbuckybits();
				switch (fixedcorner)
				{
					case 1:		/* lower-left */
						otherx = hx;   othery = hy;
						if ((bits&CONTROLDOWN) != 0)
						{
							xform(lx, ly, &orx, &ory, trans);
							xform(xcur, ycur, &rx, &ry, trans);
							if (abs(rx-orx) < abs(ry-ory)) xcur = lx; else
								ycur = ly;
						}
						break;
					case 2:		/* lower-right */
						otherx = lx;   othery = hy;
						if ((bits&CONTROLDOWN) != 0)
						{
							xform(hx, ly, &orx, &ory, trans);
							xform(xcur, ycur, &rx, &ry, trans);
							if (abs(rx-orx) < abs(ry-ory)) xcur = hx; else
								ycur = ly;
						}
						break;
					case 3:		/* upper-left */
						otherx = hx;   othery = ly;
						if ((bits&CONTROLDOWN) != 0)
						{
							xform(lx, hy, &orx, &ory, trans);
							xform(xcur, ycur, &rx, &ry, trans);
							if (abs(rx-orx) < abs(ry-ory)) xcur = lx; else
								ycur = hy;
						}
						break;
					case 4:		/* upper-right */
						otherx = lx;   othery = ly;
						if ((bits&CONTROLDOWN) != 0)
						{
							xform(hx, hy, &orx, &ory, trans);
							xform(xcur, ycur, &rx, &ry, trans);
							if (abs(rx-orx) < abs(ry-ory)) xcur = hx; else
								ycur = hy;
						}
						break;
					default:
						otherx = othery = 0;
				}

				/* transform the cursor back through the node's orientation */
				xform(otherx, othery, &rx, &ry, trans);
				lx = mini(xcur, rx);
				ly = mini(ycur, ry);
				hx = maxi(xcur, rx);
				hy = maxi(ycur, ry);

				makeangle((3600 - nodetosize->rotation)%3600, nodetosize->transpose, trans);
				xform(lx-(hx+lx)/2, ly-(hy+ly)/2, &rx, &ry, trans);
				xform(hx-(hx+lx)/2, hy-(hy+ly)/2, &xs, &ys, trans);
				rx += (hx+lx)/2;   ry += (hy+ly)/2;
				xs += (hx+lx)/2;   ys += (hy+ly)/2;
				lx = mini(xs, rx) - offxl;
				ly = mini(ys, ry) - offyl;
				hx = maxi(xs, rx) + offxh;
				hy = maxi(ys, ry) + offyh;
			} else
			{
				/* grab-point fixed and all corners stretch */
				ttyputerr(_("Cannot do grab-point stretching yet"));
				return;
			}

			/* make sure size is actually changing */
			if (lx == nodetosize->lowx && ly == nodetosize->lowy && hx == nodetosize->highx &&
				hy == nodetosize->highy)
			{
				ttyputverbose(M_("Null node scaling"));
				us_pophighlight(FALSE);
				return;
			}

			if ((nodetosize->proto->userbits&NSQUARE) != 0)
			{
				/* make sure the node is square */
				xs = hx - lx;   ys = hy - ly;
				if (xs != ys)
				{
					if (xs < ys)
					{
						lx = (lx + hx) / 2 - ys/2;   hx = lx + ys;
					} else
					{
						ly = (ly + hy) / 2 - xs/2;   hy = ly + xs;
					}
				}
			}

			/* modify the node */
			startobjectchange((INTBIG)nodetosize, VNODEINST);
			us_scaletraceinfo(nodetosize, lx, hx, ly, hy);
			modifynodeinst(nodetosize, lx-nodetosize->lowx, ly-nodetosize->lowy,
				hx-nodetosize->highx, hy-nodetosize->highy, 0, 0);
			endobjectchange((INTBIG)nodetosize, VNODEINST);

			/* adjust text descriptors on sized invisible pins */
			us_adjustdisplayabletext(nodetosize);

			/* restore highlighting */
			us_pophighlight(TRUE);
		} else
		{
			/* handle serpentine transistors specially */
			if (serptrans)
			{
				i = atofr(pp);
				if (i <= 0)
				{
					us_abortcommand(_("Width must be positive"));
					return;
				}

				/* save highlighting */
				us_pushhighlight();
				us_clearhighlightcount();

				/* size the node */
				startobjectchange((INTBIG)nodetosize, VNODEINST);
				(void)setvalkey((INTBIG)nodetosize, VNODEINST, el_transistor_width_key, i, VFRACT);
				endobjectchange((INTBIG)nodetosize, VNODEINST);

				/* restore highlighting */
				us_pophighlight(TRUE);
				return;
			}

			/* get the new X size */
			lambda = lambdaofnode(nodetosize);
			i = atola(pp, lambda);
			if (i&1) i++;
			if (i < 0)
			{
				us_abortcommand(_("X size must be positive"));
				return;
			}

			/* get the new Y size */
			if (count <= 1)
			{
				if ((nodetosize->proto->userbits&NSQUARE) != 0) par[1] = pp; else
				{
					count = ttygetparam(_("Y size: "), &us_sizeyp, MAXPARS-1, &par[1]) + 1;
					if (count == 1)
					{
						us_abortedmsg();
						return;
					}
				}
			}
			j = atola(par[1], lambda);
			if (j&1) j++;
			if (j < 0)
			{
				us_abortcommand(_("Y size must not be negative"));
				return;
			}

			/* see if transformations should be considered */
			usetransformation = FALSE;
			if (count > 2 && namesamen(par[2], x_("use-transformation"), l) == 0)
				usetransformation = TRUE;

			/* count the number of nodes that will be changed */
			tot = 0;
			for(k=0; list[k] != NOGEOM; k++)
			{
				geom = list[k];
				if (!geom->entryisnode) continue;
				nodetosize = geom->entryaddr.ni;
				if (nodetosize->proto->primindex == 0) continue;
				tot++;
			}
			if (tot <= 0)
			{
				ttyputmsg(_("No nodes changed"));
				return;
			}
			nis = (NODEINST **)emalloc(tot * (sizeof (NODEINST *)), us_tool->cluster);
			if (nis == 0) return;
			dlxs = (INTBIG *)emalloc(tot * SIZEOFINTBIG, us_tool->cluster);
			if (dlxs == 0) return;
			dlys = (INTBIG *)emalloc(tot * SIZEOFINTBIG, us_tool->cluster);
			if (dlys == 0) return;
			dhxs = (INTBIG *)emalloc(tot * SIZEOFINTBIG, us_tool->cluster);
			if (dhxs == 0) return;
			dhys = (INTBIG *)emalloc(tot * SIZEOFINTBIG, us_tool->cluster);
			if (dhys == 0) return;
			drot = (INTBIG *)emalloc(tot * SIZEOFINTBIG, us_tool->cluster);
			if (drot == 0) return;
			dtran = (INTBIG *)emalloc(tot * SIZEOFINTBIG, us_tool->cluster);
			if (dtran == 0) return;

			/* save highlighting */
			us_pushhighlight();
			us_clearhighlightcount();

			tot = 0;
			for(k=0; list[k] != NOGEOM; k++)
			{
				geom = list[k];
				if (!geom->entryisnode) continue;
				nodetosize = geom->entryaddr.ni;
				if (nodetosize->proto->primindex == 0) continue;

				/* see if X and Y size factors should be flipped */
				flipxy = FALSE;
				if (usetransformation)
				{
					rot = nodetosize->rotation;
					if (nodetosize->transpose != 0) rot = (rot + 900) % 3600;
					if (rot == 900 || rot == 2700) flipxy = TRUE;
				}

				nodesizeoffset(nodetosize, &offxl, &offyl, &offxh, &offyh);
				if (flipxy)
				{
					if (pp[0] != 0) dy = i; else
					{
						dy = (nodetosize->highy - nodetosize->lowy) - offyl - offyh;
					}
					if (par[1][0] != 0) dx = j; else
					{
						dx = (nodetosize->highx - nodetosize->lowx) - offxl - offxh;
					}
				} else
				{
					if (pp[0] != 0) dx = i; else
					{
						dx = (nodetosize->highx - nodetosize->lowx) - offxl - offxh;
					}
					if (par[1][0] != 0) dy = j; else
					{
						dy = (nodetosize->highy - nodetosize->lowy) - offyl - offyh;
					}
				}
				if ((nodetosize->proto->userbits&NSQUARE) != 0 && dx != dy)
				{
					dx = dy = maxi(dx, dy);
					if (nodecount == 1 && arccount == 0)
					{
						lambda = lambdaofnode(nodetosize);
						ttyputmsg(_("Warning: node must be square, making it %sx%s"),
							latoa(dx, lambda), latoa(dy, lambda));
					}
				}
				dxs = dx+offxl+offxh - (nodetosize->highx - nodetosize->lowx);
				dys = dy+offyl+offyh - (nodetosize->highy - nodetosize->lowy);
				if (dxs == 0 && dys == 0)
				{
					if (nodecount == 1 && arccount == 0)
						ttyputverbose(M_("Null node scaling"));
					continue;
				}

				nis[tot] = nodetosize;
				dlxs[tot] = -dxs/2;
				dlys[tot] = -dys/2;
				dhxs[tot] = dxs/2;
				dhys[tot] = dys/2;
				drot[tot] = 0;
				dtran[tot] = 0;
				tot++;
			}

			/* size the nodes */
			for(i=0; i<tot; i++)
			{
				nodetosize = nis[i];
				startobjectchange((INTBIG)nodetosize, VNODEINST);
				us_scaletraceinfo(nodetosize, nodetosize->lowx+dlxs[i],
					nodetosize->highx+dhxs[i], nodetosize->lowy+dlys[i],
						nodetosize->highy+dhys[i]);
			}

			/* modify all of the nodes at once */
			modifynodeinsts(tot, nis, dlxs, dlys, dhxs, dhys, drot, dtran);

			/* clean up change */
			for(i=0; i<tot; i++)
			{
				nodetosize = nis[i];
				endobjectchange((INTBIG)nodetosize, VNODEINST);

				/* adjust text descriptors on sized invisible pins */
				us_adjustdisplayabletext(nodetosize);
			}

			/* free memory */
			efree((CHAR *)nis);
			efree((CHAR *)dlxs);
			efree((CHAR *)dlys);
			efree((CHAR *)dhxs);
			efree((CHAR *)dhys);
			efree((CHAR *)drot);
			efree((CHAR *)dtran);

			/* solve constraints */
			(*el_curconstraint->solve)(np);

			/* restore highlighting */
			us_pophighlight(TRUE);
		}
	} else if ((arccount != 0 || justarcs) && !justnodes)
	{
		if (cursorbased != 0 && (arccount != 1 || nodecount != 0))
		{
			us_abortcommand(_("Can only use cursor-based scaling on one arc"));
			return;
		}

		/* save highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		for(j=0; list[j] != NOGEOM; j++)
		{
			geom = list[j];
			if (geom->entryisnode) continue;
			ai = geom->entryaddr.ai;
			ai->end[0].nodeinst->temp1 = 0;
			ai->end[1].nodeinst->temp1 = 0;
		}

		for(j=0; list[j] != NOGEOM; j++)
		{
			geom = list[j];
			if (geom->entryisnode) continue;
			ai = geom->entryaddr.ai;
			if (cursorbased != 0)
			{
				/* adjust the cursor position if selecting interactively */
				if ((us_tool->toolstate&INTERACTIVE) != 0)
				{
					us_sizeainit(ai);
					trackcursor(FALSE, us_ignoreup, us_sizeabegin, us_sizeadown,
						us_stopandpoponchar, us_dragup, TRACKDRAGGING);
					if (el_pleasestop != 0)
					{
						us_pophighlight(FALSE);
						return;
					}
				}

				/* get location of cursor */
				if (us_demandxy(&xcur, &ycur))
				{
					us_pophighlight(FALSE);
					return;
				}
				gridalign(&xcur, &ycur, 2, np);

				/* figure out the new size of the arcinst (manhattan only) */
				if (ai->end[0].xpos == ai->end[1].xpos)
					wid = abs(xcur - ai->end[0].xpos) * 2; else
						wid = abs(ycur - ai->end[0].ypos) * 2;
			} else
			{
				lambda = lambdaofarc(ai);
				wid = atola(pp, lambda);
				if (wid&1) wid++;
				if (wid < 0)
				{
					us_abortcommand(_("Width must not be negative"));
					us_pophighlight(FALSE);
					return;
				}
			}
			truewid = arcwidthoffset(ai) + wid;
			if (truewid == ai->width)
			{
				ttyputverbose(M_("Null arc scaling"));
				continue;
			}

			/* handle edge alignment if possible */
			dxs = dys = 0;
			if (us_edgealignment_ratio != 0 && (ai->end[0].xpos == ai->end[1].xpos ||
				ai->end[0].ypos == ai->end[1].ypos))
			{
				edgealignment = muldiv(us_edgealignment_ratio, WHOLE, el_curlib->lambda[el_curtech->techindex]);
				if (ai->end[0].xpos == ai->end[1].xpos)
				{
					/* vertical arc */
					otherx = us_alignvalue(ai->end[0].xpos + wid/2, edgealignment, &otheralign) -
						wid/2;
					otheralign -= wid/2;
					isin = otherin = 0;
					for(i=0; i<2; i++)
					{
						shapeportpoly(ai->end[i].nodeinst, ai->end[i].portarcinst->proto, poly, FALSE);
						if (isinside(otherx, ai->end[i].ypos, poly)) isin++;
						if (isinside(otheralign, ai->end[i].ypos, poly)) otherin++;
					}
					if (isin == 2) dxs = otherx - ai->end[0].xpos; else
						if (otherin == 2) dxs = otheralign - ai->end[0].xpos;
				}
				if (ai->end[0].ypos == ai->end[1].ypos)
				{
					/* horizontal arc */
					othery = us_alignvalue(ai->end[0].ypos + wid/2,
						edgealignment, &otheralign) - wid/2;
					otheralign -= wid/2;
					isin = otherin = 0;
					for(i=0; i<2; i++)
					{
						shapeportpoly(ai->end[i].nodeinst, ai->end[i].portarcinst->proto, poly, FALSE);
						if (isinside(ai->end[i].xpos, othery, poly)) isin++;
						if (isinside(ai->end[i].xpos, otheralign, poly)) otherin++;
					}
					if (isin == 2) dys = othery - ai->end[0].ypos; else
						if (otherin == 2) dys = otheralign - ai->end[0].ypos;
				}
			}
			startobjectchange((INTBIG)ai, VARCINST);
			if (modifyarcinst(ai, truewid - ai->width, dxs, dys, dxs, dys))
			{
				lambda = lambdaofarc(ai);
				ttyputerr(_("Cannot set arc width to %s"), latoa(truewid, lambda));
			} else endobjectchange((INTBIG)ai, VARCINST);

			/* scale the pins on either end */
			if (dxs == 0 && dys == 0) for(i=0; i<2; i++)
			{
				/* get a node that hasn't been scaled already */
				ni = ai->end[i].nodeinst;
				if (ni->temp1 != 0) continue;
				ni->temp1 = 1;

				/* make sure it is a pin */
				if (ni->proto->primindex == 0) continue;
				if (((ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH) != NPPIN) continue;

				/* see if it is already large enough */
				nodesizeoffset(ni, &offxl, &offyl, &offxh, &offyh);
				lx = ni->lowx+offxl;   hx = ni->highx-offxh;
				ly = ni->lowy+offyl;   hy = ni->highy-offyh;
				if (hx - lx >= wid && hy - ly >= wid) continue;

				/* increase its size */
				if (hx - lx < wid) dx = (wid - (hx-lx)) / 2; else dx = 0;
				if (hy - ly < wid) dy = (wid - (hy-ly)) / 2; else dy = 0;
				startobjectchange((INTBIG)ni, VNODEINST);
				modifynodeinst(ni, -dx, -dy, dx, dy, 0, 0);
				endobjectchange((INTBIG)ni, VNODEINST);
			}
		}

		/* restore highlighting */
		us_pophighlight(TRUE);
	}
}

void us_spread(INTBIG count, CHAR *par[])
{
	REGISTER NODEINST *ni;
	REGISTER PORTARCINST *pi;
	REGISTER GEOM **list;
	CHAR *direction;
	REGISTER INTBIG amount, i, lambda, moved, tot;
	static POLYGON *poly = NOPOLYGON;

	if (el_curconstraint != cla_constraint)
	{
		us_abortcommand(_("Must use the layout constraint system to spread"));
		return;
	}

	/* get the nodeinst about which things must spread */
	list = us_gethighlighted(WANTNODEINST, 0, 0);
	if (list[0] == NOGEOM)
	{
		us_abortcommand(_("Must select nodes to spread"));
		return;
	}

	/* get the direction of spread */
	if (count == 0)
	{
		ttyputusage(x_("spread DIRECTION [AMOUNT]"));
		return;
	}
	direction = par[0];
	if (*direction != 'u' && *direction != 'd' &&
		*direction != 'r' && *direction != 'l')
	{
		us_abortcommand(_("Direction must be 'left', 'right', 'up', or 'down'"));
		return;
	}

	/* get the amount to spread */
	if (count >= 2) amount = atola(par[1], 0); else
	{
		ni = list[0]->entryaddr.ni;
		if (ni->proto->primindex != 0)
		{
			/* get polygon */
			(void)needstaticpolygon(&poly, 4, us_tool->cluster);

			/* get design-rule surround around this node as spread amount */
			tot = nodepolys(ni, 0, NOWINDOWPART);   amount = 0;
			for(i=0; i<tot; i++)
			{
				shapenodepoly(ni, i, poly);
				amount = maxi(amount,
					maxdrcsurround(ni->proto->tech, ni->parent->lib, poly->layer));
			}

			/* also count maximum width of arcs connected to this nodeinst */
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			{
				if (pi->conarcinst == NOARCINST) continue;
				amount = maxi(amount, pi->conarcinst->width);
			}

			/* if all else fails, spread by "lambda" */
			lambda = lambdaofcell(ni->parent);
			if (amount < lambda) amount = lambda;
		} else
		{
			/* for cell, use cell size along spread axis */
			if (*direction == 'l' || *direction == 'r') amount = ni->highx - ni->lowx; else
				amount = ni->highy - ni->lowy;
		}
	}

	/* save highlighting */
	us_pushhighlight();
	us_clearhighlightcount();

	moved = 0;
	for(i=0; list[i] != NOGEOM; i++)
	{
		ni = list[i]->entryaddr.ni;

		/* disallow spreading if lock is on */
		if (us_cantedit(ni->parent, NONODEINST, TRUE)) continue;

		/* spread around the node */
		moved += us_spreadaround(ni, amount, direction);
	}
	if (moved == 0) ttyputverbose(M_("Nothing changed"));

	/* restore highlighting */
	us_pophighlight(TRUE);
}

void us_system(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG l;
	REGISTER CHAR *pt;

	if (count == 0) l = estrlen(pt = x_("x")); else
		l = estrlen(pt = par[0]);

	if (namesamen(pt, x_("setstatusfont"), l) == 0 && l >= 1)
	{
		setmessagesfont();
		return;
	}
#if defined(WIN32) || defined(USEQT)
	if (namesamen(pt, x_("print"), l) == 0 && l >= 1)
	{
		printewindow();
		return;
	}
	ttyputusage(x_("system print | setstatusfont"));
#else
	ttyputusage(x_("system setstatusfont"));
#endif
}
