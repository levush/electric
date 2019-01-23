/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrcomtz.c
 * User interface tool: command handler for T through V
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
#include "edialogs.h"
#include "usr.h"
#include "usrtrack.h"
#include "tecgen.h"
#include "efunction.h"

/* working memory for "us_var()" */
static INTBIG *us_varaddr1, *us_vartype1, us_varlimit1=0;
static INTBIG *us_varaddr2, *us_vartype2, us_varlimit2=0;
static INTBIG *us_varaddr3, *us_vartype3, us_varlimit3=0;
static CHAR *us_varqualsave = 0;

static WINDOWPART *us_wpop; /* editor window part */
static BOOLEAN us_wpopcharhandler(INTSML chr, INTBIG special);
static BOOLEAN us_wpopbuttonhandler(INTBIG x, INTBIG y, INTBIG but);

/*
 * Routine to free all memory associated with this module.
 */
void us_freecomtvmemory(void)
{
	if (us_varqualsave != 0) efree((CHAR *)us_varqualsave);
	if (us_varlimit1 > 0)
	{
		efree((CHAR *)us_vartype1);
		efree((CHAR *)us_varaddr1);
	}
	if (us_varlimit2 > 0)
	{
		efree((CHAR *)us_vartype2);
		efree((CHAR *)us_varaddr2);
	}
	if (us_varlimit3 > 0)
	{
		efree((CHAR *)us_vartype3);
		efree((CHAR *)us_varaddr3);
	}
}

void us_technology(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG l, oldlam;
	REGISTER CHAR *pp;
	CHAR *newpar[3];
	extern COMCOMP us_technologyp;
	REGISTER TECHNOLOGY *tech, *newtech;
	REGISTER NODEPROTO *np, *newnp;
	REGISTER NODEINST *ni;
	REGISTER WINDOWPART *w;
	REGISTER ARCINST *ai;
	REGISTER LIBRARY *lib;

	/* ensure there is a technology option */
	if (count == 0)
	{
		count = ttygetparam(M_("Technology option: "), &us_technologyp, MAXPARS, par);
		if (count == 0)
		{
			us_abortedmsg();
			return;
		}
	}
	l = estrlen(pp = par[0]);

	/* handle technology editing */
	if (namesamen(pp, x_("edit"), l) == 0)
	{
		us_tecedentry(count-1, &par[1]);
		return;
	}

	if (namesamen(pp, x_("autoswitch"), l) == 0)
	{
		if (count < 2)
		{
			ttyputusage(x_("technology autoswitch on|off"));
			return;
		}
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("on"), l) == 0 && l >= 2)
		{
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey,
				us_useroptions | AUTOSWITCHTECHNOLOGY, VINTEGER);
			ttyputverbose(M_("Technology will automatically switch to match cell"));
			return;
		}
		if (namesamen(pp, x_("off"), l) == 0 && l >= 2)
		{
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey,
				us_useroptions & ~AUTOSWITCHTECHNOLOGY, VINTEGER);
			ttyputverbose(M_("Automatic technology changing disabled"));
			return;
		}
		ttyputbadusage(x_("technology autoswitch"));
		return;
	}

	/* get the technology */
	if (count <= 1)
	{
		us_abortcommand(_("Must specify a technology name"));
		return;
	}
	tech = gettechnology(par[1]);
	if (tech == NOTECHNOLOGY)
	{
		us_abortcommand(_("No technology called %s"), par[1]);
		return;
	}

	/* handle documentation of technology */
	if (namesamen(pp, x_("document"), l) == 0)
	{
		us_printtechnology(tech);
		return;
	}

	/* handle technology conversion */
	if (namesamen(pp, x_("convert"), l) == 0)
	{
		np = us_needcell();
		if (np == NONODEPROTO) return;
		newnp = us_convertcell(np, tech);
		if (newnp == NONODEPROTO) return;
		newpar[0] = describenodeproto(newnp);
		newpar[1] = x_("new-window");
		us_editcell(2, newpar);
		return;
	}

	/* handle technology switching */
	if (namesamen(pp, x_("use"), l) == 0 && l >= 2)
	{
		if (el_curtech == tech)
		{
			ttyputverbose(M_("Already in %s technology"), el_curtech->techname);
			return;
		}

		ttyputverbose(M_("Switching to %s"), tech->techdescript);
		us_setnodeproto(NONODEPROTO);
		us_setarcproto(NOARCPROTO, TRUE);
		oldlam = el_curlib->lambda[el_curtech->techindex];
		us_getcolormap(tech, COLORSEXISTING, TRUE);
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_technology_key, (INTBIG)tech,
			VTECHNOLOGY|VDONTSAVE);

		/* if lambda changed and grid is drawn, erase it */
		if (oldlam != el_curlib->lambda[el_curtech->techindex])
		{
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
			{
				if ((w->state & GRIDON) == 0) continue;
				startobjectchange((INTBIG)w, VWINDOWPART);
				us_gridset(w, 0);
				endobjectchange((INTBIG)w, VWINDOWPART);
				w->state |= GRIDON;
			}
		}

		/* redraw grids */
		if (oldlam != el_curlib->lambda[el_curtech->techindex])
		{
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
			{
				if ((w->state & GRIDON) == 0) continue;
				w->state &= ~GRIDON;
				startobjectchange((INTBIG)w, VWINDOWPART);
				us_gridset(w, GRIDON);
				endobjectchange((INTBIG)w, VWINDOWPART);
			}
		}

		/* fix up the menu entries */
		us_setmenunodearcs();
		if ((us_state&NONPERSISTENTCURNODE) == 0) us_setnodeproto(tech->firstnodeproto);
		us_setarcproto(tech->firstarcproto, TRUE);
		return;
	}

	/* handle technology deletion */
	if (namesamen(pp, x_("kill"), l) == 0)
	{
		/* make sure there are no objects from this technology */
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		{
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
					if (ni->proto->primindex != 0 && ni->proto->tech == tech) break;
				if (ni != NONODEINST) break;
				for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
					if (ai->proto->tech == tech) break;
				if (ai != NOARCINST) break;
			}
			if (np != NONODEPROTO)
			{
				us_abortcommand(_("Technology %s is still in use"), tech->techname);
				return;
			}
		}

		/* cannot delete generic technology */
		if (tech == gen_tech)
		{
			us_abortcommand(_("Cannot delete the generic technology"));
			return;
		}

		/* switch technologies if killing current one */
		if (tech == el_curtech)
		{
			newtech = tech->nexttechnology;
			if (newtech == NOTECHNOLOGY)
			{
				newtech = el_technologies;
				if (newtech == tech)
				{
					us_abortcommand(_("Cannot delete the last technology"));
					return;
				}
			}
			ttyputmsg(_("Switching to %s"), newtech->techdescript);
			us_setnodeproto(NONODEPROTO);
			us_setarcproto(NOARCPROTO, TRUE);
			us_getcolormap(newtech, COLORSEXISTING, TRUE);
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_technology_key,
				(INTBIG)newtech, VTECHNOLOGY|VDONTSAVE);

			/* fix up the menu entries */
			us_setmenunodearcs();
			if ((us_state&NONPERSISTENTCURNODE) == 0) us_setnodeproto(newtech->firstnodeproto);
			us_setarcproto(newtech->firstarcproto, TRUE);
		}
		if (killtechnology(tech))
			ttyputerr(_("Unable to delete technology %s"), tech->techname);
		return;
	}

	/* handle technology information setting */
	if (namesamen(pp, x_("tell"), l) == 0)
	{
		if (tech->setmode != 0)
			telltech(tech, count-2, &par[2]); else
				ttyputerr(_("This technology accepts no commands"));
		return;
	}
	ttyputbadusage(x_("technology"));
}

void us_telltool(INTBIG count, CHAR *par[])
{
	extern COMCOMP us_telltoolp;
	REGISTER TOOL *tool;

	if (count == 0)
	{
		count = ttygetparam(M_("Which tool: "), &us_telltoolp, MAXPARS, par);
		if (count == 0)
		{
			us_abortedmsg();
			return;
		}
	}
	tool = gettool(par[0]);
	if (tool == NOTOOL)
	{
		us_abortcommand(_("No tool called %s"), par[0]);
		return;
	}
	if (tool->setmode == 0)
	{
		us_abortcommand(_("Tool %s cannot take commands"), tool->toolname);
		return;
	}
	(*tool->setmode)(count-1, &par[1]);
}

void us_terminal(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG l, forcealpha;
	REGISTER CHAR *pt, *pp;
	REGISTER VARIABLE *err;
	REGISTER CHAR *varname;
	INTBIG top, left, bottom, right;
	INTBIG msgcoord[4];
	CHAR *truename;
	REGISTER COMCOMP *comcomp;
	float newfloat;
	extern COMCOMP us_libraryup,
		us_colorreadp, us_helpp, us_technologyup, us_technologyeenp, us_viewfp,
		us_technologyeeap, us_technologyeelp, us_arrayxp, us_colorentryp, us_viewc2p,
		us_purelayerp, us_defnodesp, us_editcellp, us_spreaddp, us_portlp,
		us_defnodexsp, us_lambdachp, us_replacep, us_showp, us_viewn1p, us_showop,
		us_copycellp, us_gridalip, us_defarcsp, us_portep, us_visiblelayersp,
		us_menup, us_colorpvaluep, us_artlookp, us_colorwritep, us_librarywp,
		us_varvep, us_nodetp, us_technologyedlp, us_librarydp, us_technologyctdp,
		us_textdsp, us_noyesp, us_librarywriteformatp, us_windowrnamep, us_defnodep,
		us_showdp, us_renameop, us_renamenp, us_findnamep, us_varbs1p, us_findintp,
		us_colorhighp, us_bindgkeyp, us_gridp, us_technologytp, us_nodetcaip,
		us_librarykp, us_findobjap, us_window3dp, us_nodetptlp, us_renamecp,
		us_findexportp, us_findnnamep, us_showup, us_varop, us_copycelldp,
		us_findnodep, us_technologycnnp, us_showep, us_textfp, us_textsp, us_viewdp,
		us_interpretcp, us_findarcp, us_varvmp, us_showrp, us_varbdp, us_portcp,
		us_showfp, us_librarynp, us_viewc1p, us_libraryrp, us_defnodenotp,
		us_nodetptmp;

	if (count == 0)
	{
		ttyputusage(x_("terminal OPTIONS"));
		return;
	}
	l = estrlen(pp = par[0]);

	if (namesamen(pp, x_("not"), l) == 0)
	{
		if (count < 2)
		{
			ttyputusage(x_("terminal not OPTION"));
			return;
		}
		l = estrlen(pp = par[1]);
		if (namesame(pp, x_("lock-keys-on-error")) == 0)
		{
			(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate | NOKEYLOCK, VINTEGER);
			ttyputverbose(M_("Command errors will not lockout single-key commands"));
			return;
		}
		if (namesame(pp, x_("only-informative-messages")) == 0)
		{
			(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate & ~JUSTTHEFACTS, VINTEGER);
			ttyputverbose(M_("All messages will be displayed"));
			return;
		}
		if (namesame(pp, x_("use-electric-commands")) == 0)
		{
			(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate | NODETAILS, VINTEGER);
			ttyputverbose(M_("Messages will not mention specific commands"));
			return;
		}
		if (namesame(pp, x_("display-dialogs")) == 0)
		{
			(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate & ~USEDIALOGS, VINTEGER);
			ttyputverbose(M_("Dialogs will not be used"));
			return;
		}
		if (namesame(pp, x_("permanent-menu-highlighting")) == 0)
		{
			(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate | ONESHOTMENU, VINTEGER);
			ttyputverbose(M_("Menu highlighting will clear after each command"));
			return;
		}
		if (namesame(pp, x_("track-cursor-coordinates")) == 0)
		{
			(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate & ~SHOWXY, VINTEGER);
			ttyputverbose(M_("Technology and Lambda will be displayed where cursor coordinates are"));
			us_redostatus(NOWINDOWFRAME);
			return;
		}
		if (namesame(pp, x_("beep")) == 0)
		{
			(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate & ~TERMBEEP, VINTEGER);
			ttyputverbose(M_("Terminal beep disabled"));
			return;
		}
		if (namesamen(pp, x_("enable-interrupts"), l) == 0)
		{
			(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate | TERMNOINTERRUPT, VINTEGER);
			ttyputmsg(M_("Interrupts disabled"));
			return;
		}			
		if (namesamen(pp, x_("audit"), l) == 0)
		{
			if (us_termaudit != NULL) xclose(us_termaudit);
			us_termaudit = NULL;
			(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate & ~TTYAUDIT, VINTEGER);
			ttyputverbose(M_("No longer saving messages"));
			return;
		}
		ttyputbadusage(x_("terminal not"));
		return;
	}

	if (namesamen(pp, x_("clear"), l) == 0)
	{
		ttyclearmessages();
		return;
	}

	if (namesamen(pp, x_("get-location"), l) == 0)
	{
		getmessagesframeinfo(&top, &left, &bottom, &right);
		msgcoord[0] = top;
		msgcoord[1] = left;
		msgcoord[2] = bottom;
		msgcoord[3] = right;
		(void)setval((INTBIG)us_tool, VTOOL, x_("USER_messages_position"),
			(INTBIG)msgcoord, VINTEGER|VISARRAY|(4<<VLENGTHSH));
		ttyputmsg(_("Window location will be saved with the options"));
		return;
	}

	if (namesamen(pp, x_("lock-keys-on-error"), l) == 0 && l >= 2)
	{
		(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate & ~NOKEYLOCK, VINTEGER);
		ttyputverbose(M_("Command errors will lockout single-key commands"));
		return;
	}

	if (namesamen(pp, x_("only-informative-messages"), l) == 0)
	{
		(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate | JUSTTHEFACTS, VINTEGER);
		ttyputverbose(M_("Nonessential messages will be supressed"));
		return;
	}

	if (namesamen(pp, x_("display-dialogs"), l) == 0)
	{
		(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate | USEDIALOGS, VINTEGER);
		ttyputverbose(M_("Dialogs will be use where appropriate"));
		return;
	}

	if (namesamen(pp, x_("permanent-menu-highlighting"), l) == 0)
	{
		(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate & ~ONESHOTMENU, VINTEGER);
		ttyputverbose(M_("Menu highlighting will show current node/arc"));
		return;
	}

	if (namesame(pp, x_("track-cursor-coordinates")) == 0)
	{
		(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate | SHOWXY, VINTEGER);
		ttyputverbose(M_("Cursor coordinates will be displayed where TECH/LAMBDA are"));
		us_redostatus(NOWINDOWFRAME);
		return;
	}
	if (namesamen(pp, x_("use-electric-commands"), l) == 0)
	{
		(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate & ~NODETAILS, VINTEGER);
		ttyputverbose(M_("Messages will use specific Electric commands"));
		return;
	}
	if (namesamen(pp, x_("beep"), l) == 0)
	{
		(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate | TERMBEEP, VINTEGER);
		ttyputverbose(M_("Terminal beep enabled"));
		return;
	}
	if (namesamen(pp, x_("enable-interrupts"), l) == 0)
	{
		(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate & ~TERMNOINTERRUPT, VINTEGER);
		ttyputmsg(M_("Interrupts enabled"));
		return;
	}			
	if (namesamen(pp, x_("audit"), l) == 0)
	{
		us_termaudit = xcreate(x_("emessages.txt"), el_filetypetext, 0, &truename);
		if (us_termaudit == 0)
		{
			ttyputerr(_("Cannot create 'emessages.txt'"));
			return;
		}
		ttyputmsg(_("Saving messages to '%s'"), truename);
		(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate | TTYAUDIT, VINTEGER);
		return;
	}

	if (namesamen(pp, x_("input"), l) == 0)
	{
		/* make sure there are the right number of parameters */
		if (count < 3)
		{
			ttyputusage(x_("terminal input LETTER PROMPT [TYPE]"));
			return;
		}

		/* make sure the command interpreter variable is a single letter */
		if (!isalpha(par[1][0]) != 0 || par[1][1] != 0)
		{
			us_abortcommand(_("Input value must be a single letter"));
			return;
		}
		varname = us_commandvarname(par[1][0]);

		/* get the value */
		forcealpha = 0;
		if (count == 4 && namesame(par[3], x_("string")) == 0)
		{
			count--;
			forcealpha = 1;
		}
		if (count < 4)
		{
			pt = ttygetline(par[2]);
			if (pt == 0) pt = x_("");
		} else
		{
			forcealpha = 1;
			comcomp = NOCOMCOMP;

			if (namesame(par[3], x_("about")) == 0)         comcomp = &us_showep; else
			if (namesame(par[3], x_("alignment")) == 0)     comcomp = &us_gridalip; else
			if (namesame(par[3], x_("annulus")) == 0)       comcomp = &us_nodetcaip; else
			if (namesame(par[3], x_("array")) == 0)         comcomp = &us_arrayxp; else
			if (namesame(par[3], x_("artlook")) == 0)       comcomp = &us_artlookp; else
			if (namesame(par[3], x_("attreport")) == 0)     comcomp = &us_varbs1p; else
			if (namesame(par[3], x_("attributes")) == 0)    comcomp = &us_varop; else
			if (namesame(par[3], x_("attparam")) == 0)      comcomp = &us_varbdp; else
			if (namesame(par[3], x_("change")) == 0)        comcomp = &us_replacep; else
			if (namesame(par[3], x_("chglibrary")) == 0)    comcomp = &us_librarykp; else
			if (namesame(par[3], x_("copycell")) == 0)      comcomp = &us_copycellp; else
			if (namesame(par[3], x_("copyrightopt")) == 0)  comcomp = &us_defnodenotp; else
			if (namesame(par[3], x_("defarc")) == 0)        comcomp = &us_defarcsp; else
			if (namesame(par[3], x_("defnode")) == 0)       comcomp = &us_defnodexsp; else
			if (namesame(par[3], x_("deftext")) == 0)       comcomp = &us_textdsp; else
			if (namesame(par[3], x_("delcell")) == 0)       comcomp = &us_showup; else
			if (namesame(par[3], x_("delview")) == 0)       comcomp = &us_viewdp; else
			if (namesame(par[3], x_("dependentlibs")) == 0) comcomp = &us_technologyedlp; else
			if (namesame(par[3], x_("depth3d")) == 0)       comcomp = &us_window3dp; else
			if (namesame(par[3], x_("editcell")) == 0)      comcomp = &us_editcellp; else
			if (namesame(par[3], x_("edtecarc")) == 0)      comcomp = &us_technologyeeap; else
			if (namesame(par[3], x_("edteclayer")) == 0)    comcomp = &us_technologyeelp; else
			if (namesame(par[3], x_("edtecnode")) == 0)     comcomp = &us_technologyeenp; else
			if (namesame(par[3], x_("enumattrs")) == 0)     comcomp = &us_varvmp; else
			if (namesame(par[3], x_("cellinfo")) == 0)      comcomp = &us_defnodep; else
			if (namesame(par[3], x_("celllist")) == 0)      comcomp = &us_showfp; else
			if (namesame(par[3], x_("cellinst")) == 0)      comcomp = &us_viewc1p; else
			if (namesame(par[3], x_("cell")) == 0)          comcomp = &us_showdp; else
			if (namesame(par[3], x_("file")) == 0)          comcomp = &us_colorreadp; else
			if (namesame(par[3], x_("find")) == 0)          comcomp = &us_textfp; else
			if (namesame(par[3], x_("frameopt")) == 0)      comcomp = &us_viewfp; else
			if (namesame(par[3], x_("genopt")) == 0)        comcomp = &us_showrp; else
			if (namesame(par[3], x_("globsignal")) == 0)    comcomp = &us_portcp; else
			if (namesame(par[3], x_("grid")) == 0)          comcomp = &us_gridp; else
			if (namesame(par[3], x_("help")) == 0)          comcomp = &us_helpp; else
			if (namesame(par[3], x_("highlayer")) == 0)     comcomp = &us_colorhighp; else
			if (namesame(par[3], x_("iconopt")) == 0)       comcomp = &us_copycelldp; else
			if (namesame(par[3], x_("javaopt")) == 0)       comcomp = &us_interpretcp; else
			if (namesame(par[3], x_("labels")) == 0)        comcomp = &us_portlp; else
			if (namesame(par[3], x_("lambda")) == 0)        comcomp = &us_lambdachp; else
			if (namesame(par[3], x_("layerpatterns")) == 0) comcomp = &us_colorpvaluep; else
			if (namesame(par[3], x_("layers")) == 0)        comcomp = &us_colorentryp; else
			if (namesame(par[3], x_("library")) == 0)       comcomp = &us_libraryup; else
			if (namesame(par[3], x_("libtotech")) == 0)     comcomp = &us_technologycnnp; else
			if (namesame(par[3], x_("menu")) == 0)          comcomp = &us_menup; else
			if (namesame(par[3], x_("newview")) == 0)       comcomp = &us_viewn1p; else
			if (namesame(par[3], x_("node")) == 0)          comcomp = &us_defnodesp; else
			if (namesame(par[3], x_("noyes")) == 0)         comcomp = &us_noyesp; else
			if (namesame(par[3], x_("ofile")) == 0)         comcomp = &us_colorwritep; else
			if (namesame(par[3], x_("optionexamine")) == 0) comcomp = &us_librarynp; else
			if (namesame(par[3], x_("optionfind")) == 0)    comcomp = &us_libraryrp; else
			if (namesame(par[3], x_("optionsave")) == 0)    comcomp = &us_librarywp; else
			if (namesame(par[3], x_("path")) == 0)          comcomp = &us_librarydp; else
			if (namesame(par[3], x_("placetext")) == 0)     comcomp = &us_nodetptlp; else
			if (namesame(par[3], x_("plot")) == 0)          comcomp = &us_librarywriteformatp; else
			if (namesame(par[3], x_("port")) == 0)          comcomp = &us_portep; else
			if (namesame(par[3], x_("purelayer")) == 0)     comcomp = &us_purelayerp; else
			if (namesame(par[3], x_("quickkey")) == 0)      comcomp = &us_bindgkeyp; else
			if (namesame(par[3], x_("renameexport")) == 0)  comcomp = &us_renamecp; else
			if (namesame(par[3], x_("renamelib")) == 0)     comcomp = &us_renameop; else
			if (namesame(par[3], x_("renametech")) == 0)    comcomp = &us_renamenp; else
			if (namesame(par[3], x_("renamecell")) == 0)    comcomp = &us_findnamep; else
			if (namesame(par[3], x_("renamenet")) == 0)     comcomp = &us_findintp; else
			if (namesame(par[3], x_("romgen")) == 0)        comcomp = &us_nodetptmp; else
			if (namesame(par[3], x_("selectarc")) == 0)     comcomp = &us_findarcp; else		
			if (namesame(par[3], x_("selectnet")) == 0)     comcomp = &us_findnnamep; else		
			if (namesame(par[3], x_("selectnode")) == 0)    comcomp = &us_findnodep; else		
			if (namesame(par[3], x_("selectopt")) == 0)     comcomp = &us_findobjap; else		
			if (namesame(par[3], x_("selectport")) == 0)    comcomp = &us_findexportp; else		
			if (namesame(par[3], x_("showdetail")) == 0)    comcomp = &us_showop; else
			if (namesame(par[3], x_("show")) == 0)          comcomp = &us_showp; else
			if (namesame(par[3], x_("spread")) == 0)        comcomp = &us_spreaddp; else
			if (namesame(par[3], x_("technology")) == 0)    comcomp = &us_technologyup; else
			if (namesame(par[3], x_("techopt")) == 0)       comcomp = &us_technologytp; else
			if (namesame(par[3], x_("techvars")) == 0)      comcomp = &us_technologyctdp; else
			if (namesame(par[3], x_("textsize")) == 0)      comcomp = &us_textsp; else
			if (namesame(par[3], x_("trace")) == 0)         comcomp = &us_nodetp; else
			if (namesame(par[3], x_("variable")) == 0)      comcomp = &us_varvep; else
			if (namesame(par[3], x_("view")) == 0)          comcomp = &us_viewc2p; else
			if (namesame(par[3], x_("visiblelayers")) == 0) comcomp = &us_visiblelayersp; else
			if (namesame(par[3], x_("windowview")) == 0)    comcomp = &us_windowrnamep; else
				comcomp = us_getcomcompfromkeyword(par[3]);
			if (comcomp == NOCOMCOMP)
			{
				us_abortcommand(_("Unknown input type: %s"), par[3]);
				return;
			}
			count = ttygetparam(par[2], comcomp, MAXPARS-3, &par[3]);
			if (count < 1) pt = x_(""); else pt = par[3];
		}
		if (*pt == 0)
		{
			if (getval((INTBIG)us_tool, VTOOL, -1, varname) != NOVARIABLE)
				(void)delval((INTBIG)us_tool, VTOOL, varname);
			return;
		}

		/* store the value */
		if (isanumber(pt) && forcealpha == 0)
		{
			for(pp = pt; *pp != 0; pp++) if (*pp == '.')
			{
				newfloat = (float)eatof(pt);
				err = setval((INTBIG)us_tool, VTOOL, varname, castint(newfloat), VFLOAT|VDONTSAVE);
				break;
			}
			if (*pp == 0)
				err = setval((INTBIG)us_tool, VTOOL, varname, myatoi(pt), VINTEGER|VDONTSAVE);
		} else err = setval((INTBIG)us_tool, VTOOL, varname, (INTBIG)pt, VSTRING|VDONTSAVE);
		if (err == NOVARIABLE)
		{
			us_abortcommand(_("Problem setting variable %s"), par[1]);
			return;
		}
		return;
	}

	if (namesamen(pp, x_("session"), l) == 0)
	{
		if (!graphicshas(CANLOGINPUT))
		{
			us_abortcommand(_("Sorry, this display driver does not log sessions"));
			return;
		}

		/* make sure there are the right number of parameters */
		if (count <= 1) l = estrlen(pp = x_("?")); else
			l = estrlen(pp = par[1]);

		if (namesamen(pp, x_("playback"), l) == 0)
		{
			if (count <= 2)
			{
				ttyputusage(x_("terminal session playback FILE"));
				return;
			}
			if (logplayback(par[2]))
				us_abortcommand(_("Cannot read playback file %s"), par[2]);
			return;
		}

		if (namesamen(pp, x_("begin-record"), l) == 0)
		{
			if (us_logrecord != NULL)
			{
				us_abortcommand(_("Session is already being recorded"));
				return;
			}
			logstartrecord();
			return;
		}

		/* make sure session logging is on */
		if (us_logrecord == NULL)
		{
			us_abortcommand(_("Session is not being recorded"));
			return;
		}

		if (namesamen(pp, x_("end-record"), l) == 0)
		{
			logfinishrecord();
			return;
		}

		if (namesamen(pp, x_("rewind-record"), l) == 0)
		{
			logfinishrecord();
			logstartrecord();
			return;
		}

		if (namesamen(pp, x_("checkpoint-frequency"), l) == 0)
		{
			if (count <= 2)
			{
				ttyputusage(x_("terminal session checkpoint-frequency F"));
				return;
			}
			us_logflushfreq = myatoi(par[2]);
			ttyputverbose(M_("Session logging will be guaranteed every %ld commands"),
				us_logflushfreq);
			return;
		}

		ttyputbadusage(x_("terminal session"));
		return;
	}

	ttyputbadusage(x_("terminal"));
}

void us_text(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG grabpoint, newgrabpoint, len, font;
	REGISTER INTBIG i, l;
	INTBIG xw, yw, xcur, ycur, adjxc, adjyc;
	UINTBIG descript[TEXTDESCRIPTSIZE], olddescript[TEXTDESCRIPTSIZE];
	INTBIG tsx, tsy, style;
	REGISTER CHAR *pp, *str;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER VARIABLE *var;
	REGISTER WINDOWPART *w;
	REGISTER HIGHLIGHT *high;
	REGISTER TECHNOLOGY *tech;
	static POLYGON *poly = NOPOLYGON;
	static CHAR *smartstyle[3] = {N_("off"), N_("inside"), N_("outside")};

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	if (count == 0)
	{
		ttyputusage(x_("text style|size|editor|default-*|read|write"));
		return;
	}
	l = estrlen(pp = par[0]);

	/* handle text saving */
	if (namesamen(pp, x_("write"), l) == 0 && l >= 1)
	{
		if (count < 2)
		{
			ttyputusage(x_("text write FILENAME"));
			return;
		}

		if (us_needwindow()) return;

		/* make sure the current window is textual */
		if ((el_curwindowpart->state&WINDOWTYPE) != POPTEXTWINDOW &&
			(el_curwindowpart->state&WINDOWTYPE) != TEXTWINDOW)
		{
			us_abortcommand(_("Current window is not textual"));
			return;
		}

		us_writetextfile(el_curwindowpart, par[1]);
		return;
	}

	/* handle text cell reading */
	if (namesamen(pp, x_("read"), l) == 0 && l >= 1)
	{
		if (count < 2)
		{
			ttyputusage(x_("text read FILENAME"));
			return;
		}

		if (us_needwindow()) return;

		/* make sure the current window is textual */
		if ((el_curwindowpart->state&WINDOWTYPE) != POPTEXTWINDOW &&
			(el_curwindowpart->state&WINDOWTYPE) != TEXTWINDOW)
		{
			us_abortcommand(_("Current window is not textual"));
			return;
		}

		us_readtextfile(el_curwindowpart, par[1]);
		return;
	}

	/* handle text cell manipulation */
	if (namesamen(pp, x_("cut"), l) == 0 && l >= 2)
	{
		/* first see if this applies to messages window */
		if (cutfrommessages()) return;

		/* next see if it applies to a text edit window */
		if (us_needwindow()) return;
		switch (el_curwindowpart->state&WINDOWTYPE)
		{
			case POPTEXTWINDOW:
			case TEXTWINDOW:
				us_cuttext(el_curwindowpart);
				break;
			case DISPWINDOW:
				us_cutobjects(el_curwindowpart);
				break;
			case WAVEFORMWINDOW:
				ttyputmsg(_("To copy waveforms, use 'p' to preserve a snapshot"));
				break;
			case EXPLORERWINDOW:
				ttyputmsg(_("You cannot cut from a cell explorer window, but you can copy"));
				break;
			default:
				ttyputmsg(_("Cannot cut from this type of window"));
				break;
		}
		return;
	}
	if (namesamen(pp, x_("copy"), l) == 0 && l >= 2)
	{
		/* first see if this applies to messages window */
		if (copyfrommessages()) return;

		/* next see if it applies to a text edit window */
		if (us_needwindow()) return;
		switch (el_curwindowpart->state&WINDOWTYPE)
		{
			case POPTEXTWINDOW:
			case TEXTWINDOW:
				us_copytext(el_curwindowpart);
				break;
			case DISPWINDOW:
				us_copyobjects(el_curwindowpart);
				break;
			case WAVEFORMWINDOW:
				ttyputmsg(_("To copy waveforms, use 'p' to preserve a snapshot"));
				break;
			case EXPLORERWINDOW:
				us_copyexplorerwindow();
				break;
			default:
				ttyputmsg(_("Cannot copy from this type of window"));
				break;
		}
		return;
	}
	if (namesamen(pp, x_("paste"), l) == 0 && l >= 1)
	{
		/* first see if this applies to messages window */
		if (pastetomessages()) return;

		/* next see if it applies to a text edit window */
		if (us_needwindow()) return;
		switch (el_curwindowpart->state&WINDOWTYPE)
		{
			case POPTEXTWINDOW:
			case TEXTWINDOW:
				us_pastetext(el_curwindowpart);
				break;
			case DISPWINDOW:
				us_pasteobjects(el_curwindowpart);
				break;
			default:
				ttyputmsg(_("Cannot paste to this type of window"));
				break;
		}
		return;
	}
	if (namesamen(pp, x_("find"), l) == 0 && l >= 1)
	{
		if (count < 2)
		{
			ttyputusage(x_("text find STRING"));
			return;
		}

		/* make sure the current window is textual */
		if (us_needwindow()) return;
		if ((el_curwindowpart->state&WINDOWTYPE) != POPTEXTWINDOW &&
			(el_curwindowpart->state&WINDOWTYPE) != TEXTWINDOW)
		{
			us_abortcommand(_("Current window is not textual"));
			return;
		}

		us_searchtext(el_curwindowpart, par[1], 0, 0);
		return;
	}

	/* handle editor selection */
	if (namesamen(pp, x_("editor"), l) == 0 && l >= 2)
	{
		if (count < 2)
		{
			ttyputmsg(_("Current text editor is %s"), us_editortable[us_currenteditor].editorname);
			return;
		}
		l = estrlen(pp = par[1]);

		/* make sure there are no text editors running */
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
			if ((w->state&WINDOWTYPE) == TEXTWINDOW || (w->state&WINDOWTYPE) == POPTEXTWINDOW)
		{
			us_abortcommand(_("Must terminate all active editors before switching"));
			return;
		}

		for(i=0; us_editortable[i].editorname != 0; i++)
			if (namesamen(pp, us_editortable[i].editorname, l) == 0) break;
		if (us_editortable[i].editorname == 0)
		{
			us_abortcommand(_("Unknown editor: %s"), pp);
			return;
		}
		ttyputverbose(M_("Now using %s editor for text"), us_editortable[i].editorname);
		setvalkey((INTBIG)us_tool, VTOOL, us_text_editorkey, (INTBIG)us_editortable[i].editorname,
			VSTRING);
		return;
	}

	if (namesamen(pp, x_("easy-text-selection"), l) == 0 && l >= 2)
	{
		if ((us_useroptions&NOTEXTSELECT) == 0)
			ttyputverbose(M_("Annotation text is already easy to select")); else
		{
			ttyputverbose(M_("Annotation text will be easily selectable"));
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey,
				us_useroptions | NOTEXTSELECT, VINTEGER);
		}
		return;
	}

	if (namesamen(pp, x_("hard-text-selection"), l) == 0 && l >= 1)
	{
		if ((us_useroptions&NOTEXTSELECT) != 0)
			ttyputverbose(M_("Annotation text is already hard to select")); else
		{
			ttyputverbose(M_("Annotation text will be hard to select (only when the 'find' command does NOT have the 'port' option)"));
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey,
				us_useroptions & ~NOTEXTSELECT, VINTEGER);
		}
		return;
	}

	/* handle default node text size */
	if (namesamen(pp, x_("default-node-size"), l) == 0 && l >= 9)
	{
		/* get new value */
		if (count < 2)
		{
			ttyputusage(x_("text default-node-size SIZE"));
			return;
		}
		defaulttextsize(3, descript);
		font = us_gettextsize(par[1], TDGETSIZE(descript));
		if (font < 0) return;
		TDSETSIZE(descript, font);

		/* set the default text size */
		setval((INTBIG)us_tool, VTOOL, x_("USER_default_node_text_size"),
			(INTBIG)descript, VINTEGER|VISARRAY|(TEXTDESCRIPTSIZE<<VLENGTHSH)|VDONTSAVE);
		ttyputverbose(M_("Default node text size is now %s"), us_describefont(font));
		return;
	}

	/* handle default arc text size */
	if (namesamen(pp, x_("default-arc-size"), l) == 0 && l >= 9)
	{
		/* get new value */
		if (count < 2)
		{
			ttyputusage(x_("text default-arc-size SIZE"));
			return;
		}
		defaulttextsize(4, descript);
		font = us_gettextsize(par[1], TDGETSIZE(descript));
		if (font < 0) return;
		TDSETSIZE(descript, font);

		/* set the default text size */
		setval((INTBIG)us_tool, VTOOL, x_("USER_default_arc_text_size"),
			(INTBIG)descript, VINTEGER|VISARRAY|(TEXTDESCRIPTSIZE<<VLENGTHSH)|VDONTSAVE);
		ttyputverbose(M_("Default arc text size is now %s"), us_describefont(font));
		return;
	}

	/* handle default export text size */
	if (namesamen(pp, x_("default-export-size"), l) == 0 && l >= 11)
	{
		/* get new value */
		if (count < 2)
		{
			ttyputusage(x_("text default-export-size SIZE"));
			return;
		}
		defaulttextsize(1, descript);
		font = us_gettextsize(par[1], TDGETSIZE(descript));
		if (font < 0) return;
		TDSETSIZE(descript, font);

		/* set the default text size */
		setval((INTBIG)us_tool, VTOOL, x_("USER_default_export_text_size"),
			(INTBIG)descript, VINTEGER|VISARRAY|(TEXTDESCRIPTSIZE<<VLENGTHSH)|VDONTSAVE);
		ttyputverbose(M_("Default export text size is now %s"), us_describefont(font));
		return;
	}

	/* handle default nonlayout text size */
	if (namesamen(pp, x_("default-nonlayout-text-size"), l) == 0 && l >= 9)
	{
		/* get new value */
		if (count < 2)
		{
			ttyputusage(x_("text default-nonlayout-text-size SIZE"));
			return;
		}
		defaulttextsize(2, descript);
		font = us_gettextsize(par[1], TDGETSIZE(descript));
		if (font < 0) return;
		TDSETSIZE(descript, font);

		/* set the default text size */
		setval((INTBIG)us_tool, VTOOL, x_("USER_default_nonlayout_text_size"),
			(INTBIG)descript, VINTEGER|VISARRAY|(TEXTDESCRIPTSIZE<<VLENGTHSH)|VDONTSAVE);
		ttyputverbose(M_("Default nonlayout text size is now %s"), us_describefont(font));
		return;
	}

	/* handle default instance name size */
	if (namesamen(pp, x_("default-instance-size"), l) == 0 && l >= 9)
	{
		/* get new value */
		if (count < 2)
		{
			ttyputusage(x_("text default-instance-size SIZE"));
			return;
		}
		defaulttextsize(5, descript);
		font = us_gettextsize(par[1], TDGETSIZE(descript));
		if (font < 0) return;
		TDSETSIZE(descript, font);

		/* set the default text size */
		setval((INTBIG)us_tool, VTOOL, x_("USER_default_instance_text_size"),
			(INTBIG)descript, VINTEGER|VISARRAY|(TEXTDESCRIPTSIZE<<VLENGTHSH)|VDONTSAVE);
		ttyputverbose(M_("Default instance text size is now %s"), us_describefont(font));
		return;
	}

	/* handle default cell text size */
	if (namesamen(pp, x_("default-cell-size"), l) == 0 && l >= 9)
	{
		/* get new value */
		if (count < 2)
		{
			ttyputusage(x_("text default-cell-size SIZE"));
			return;
		}
		defaulttextsize(6, descript);
		font = us_gettextsize(par[1], TDGETSIZE(descript));
		if (font < 0) return;
		TDSETSIZE(descript, font);

		/* set the default text size */
		setval((INTBIG)us_tool, VTOOL, x_("USER_default_facet_text_size"),
			(INTBIG)descript, VINTEGER|VISARRAY|(TEXTDESCRIPTSIZE<<VLENGTHSH)|VDONTSAVE);
		ttyputverbose(M_("Default cell text size is now %s"), us_describefont(font));
		return;
	}

	/* handle default interior style */
	if (namesamen(pp, x_("default-interior-only"), l) == 0 && l >= 9)
	{
		/* get current value */
		var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_default_text_style"));
		if (var == NOVARIABLE) grabpoint = VTPOSCENT; else grabpoint = var->addr;

		/* set the default interior style */
		setval((INTBIG)us_tool, VTOOL, x_("USER_default_text_style"),
			grabpoint|VTINTERIOR, VINTEGER|VDONTSAVE);
		ttyputverbose(M_("Default text is visible only inside cell"));
		return;
	}
	if (namesamen(pp, x_("default-exterior"), l) == 0 && l >= 11)
	{
		/* get current value */
		var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_default_text_style"));
		if (var == NOVARIABLE) grabpoint = VTPOSCENT; else grabpoint = var->addr;

		/* set the default interior style */
		setval((INTBIG)us_tool, VTOOL, x_("USER_default_text_style"),
			grabpoint & ~VTINTERIOR, VINTEGER|VDONTSAVE);
		ttyputverbose(M_("Default text is visible outside of cell"));
		return;
	}

	/* handle default style */
	if (namesamen(pp, x_("default-style"), l) == 0 && l >= 9)
	{
		/* get current value */
		var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_default_text_style"));
		if (var == NOVARIABLE) grabpoint = VTPOSCENT; else grabpoint = var->addr;

		/* if no argument provided, show current value */
		if (count < 2)
		{
			ttyputmsg(M_("Default text style is %s"), us_describestyle(grabpoint));
			if ((grabpoint&VTINTERIOR) != 0)
				ttyputmsg(M_("Default text is visible only inside cell")); else
					ttyputmsg(M_("Default text is visible outside of cell"));
			return;
		}

		/* get new value */
		newgrabpoint = us_gettextposition(par[1]);
		grabpoint = (grabpoint & ~VTPOSITION) | newgrabpoint;

		/* set the default text style */
		setval((INTBIG)us_tool, VTOOL, x_("USER_default_text_style"),
			grabpoint, VINTEGER|VDONTSAVE);
		ttyputverbose(M_("Default text style is now %s"), us_describestyle(grabpoint));
		return;
	}

	/* handle default horizontal and vertical style */
	if (namesamen(pp, x_("default-horizontal-style"), l) == 0 && l >= 9)
	{
		/* get current value */
		var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_default_text_smart_style"));
		if (var == NOVARIABLE) grabpoint = 0; else grabpoint = var->addr;

		/* if no argument provided, show current value */
		if (count < 2)
		{
			ttyputmsg(M_("Default horizontal text style is %s"),
				TRANSLATE(smartstyle[grabpoint&03]));
			return;
		}

		/* get new value */
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("none"), l) == 0) grabpoint = (grabpoint & ~03) | 0; else
		if (namesamen(pp, x_("inside"), l) == 0) grabpoint = (grabpoint & ~03) | 1; else
		if (namesamen(pp, x_("outside"), l) == 0) grabpoint = (grabpoint & ~03) | 2; else
		{
			ttyputusage(x_("text default-horizontal-style [none|inside|outside]"));
			return;
		}

		/* set the default text style */
		setval((INTBIG)us_tool, VTOOL, x_("USER_default_text_smart_style"), grabpoint, VINTEGER|VDONTSAVE);
		ttyputverbose(M_("Default horizontal text style is now %s"), TRANSLATE(smartstyle[grabpoint&03]));
		return;
	}
	if (namesamen(pp, x_("default-vertical-style"), l) == 0 && l >= 9)
	{
		/* get current value */
		var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_default_text_smart_style"));
		if (var == NOVARIABLE) grabpoint = 0; else grabpoint = var->addr;

		/* if no argument provided, show current value */
		if (count < 2)
		{
			ttyputmsg(M_("Default vertical text style is %s"),
				TRANSLATE(smartstyle[(grabpoint>>2)&03]));
			return;
		}

		/* get new value */
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("none"), l) == 0) grabpoint = (grabpoint & ~014) | 0; else
		if (namesamen(pp, x_("inside"), l) == 0) grabpoint = (grabpoint & ~014) | (1<<2); else
		if (namesamen(pp, x_("outside"), l) == 0) grabpoint = (grabpoint & ~014) | (2<<2); else
		{
			ttyputusage(x_("text default-vertical-style [none|inside|outside]"));
			return;
		}

		/* set the default text style */
		setval((INTBIG)us_tool, VTOOL, x_("USER_default_text_smart_style"), grabpoint, VINTEGER|VDONTSAVE);
		ttyputverbose(M_("Default vertical text style is now %s"), TRANSLATE(smartstyle[(grabpoint>>2)&03]));
		return;
	}

	/* get the text object */
	high = us_getonehighlight();
	if (high == NOHIGHLIGHT) return;
	if ((high->status&HIGHTYPE) != HIGHTEXT)
	{
		us_abortcommand(_("Find a single text object first"));
		return;
	}

	/* handle grab-point motion */
	if (namesamen(pp, x_("style"), l) == 0 && l >= 2)
	{
		/* make sure the cursor is in the right cell */
		np = us_needcell();
		if (np == NONODEPROTO) return;
		if (np != high->cell)
		{
			us_abortcommand(_("Must have cursor in this cell"));
			return;
		}

		/* get the center of the text */
		us_gethightextcenter(high, &adjxc, &adjyc, &style);

		/* get the text and current descriptor */
		str = us_gethighstring(high);
		us_gethighdescript(high, olddescript);

		/* get size of text */
		len = 1;
		if (high->fromvar != NOVARIABLE && (high->fromvar->type&VISARRAY) != 0)
			len = getlength(high->fromvar);
		if (len > 1)
		{
			xw = high->fromgeom->highx - high->fromgeom->lowx;
			yw = high->fromgeom->highy - high->fromgeom->lowy;
		} else
		{
			if (!high->fromgeom->entryisnode)
				tech = high->fromgeom->entryaddr.ai->proto->tech; else
					tech = high->fromgeom->entryaddr.ni->proto->tech;
			screensettextinfo(el_curwindowpart, tech, olddescript);
			screengettextsize(el_curwindowpart, str, &tsx, &tsy);
			xw = muldiv(tsx, el_curwindowpart->screenhx-el_curwindowpart->screenlx,
				el_curwindowpart->usehx-el_curwindowpart->uselx);
			yw = muldiv(tsy, el_curwindowpart->screenhy-el_curwindowpart->screenly,
				el_curwindowpart->usehy-el_curwindowpart->usely);
		}

		/* save highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* see if a specific grab point has been mentioned */
		if (count >= 2)
		{
			grabpoint = us_gettextposition(par[1]);
			TDCOPY(descript, olddescript);
			TDSETPOS(descript, grabpoint);
		} else
		{
			/* get co-ordinates of cursor */
			if (us_demandxy(&xcur, &ycur))
			{
				us_pophighlight(FALSE);
				return;
			}
			gridalign(&xcur, &ycur, 1, np);

			/* adjust the cursor position if selecting interactively */
			if ((us_tool->toolstate&INTERACTIVE) != 0)
			{
				us_textgrabinit(olddescript, xw, yw, adjxc, adjyc, high->fromgeom);
				trackcursor(FALSE, us_ignoreup, us_textgrabbegin, us_textgrabdown,
					us_stopandpoponchar, us_dragup, TRACKDRAGGING);
				if (el_pleasestop != 0) return;
				if (us_demandxy(&xcur, &ycur)) return;
			}

			/* determine grab point from current cursor location */
			TDCOPY(descript, olddescript);
			us_figuregrabpoint(descript, xcur, ycur, adjxc, adjyc, xw, yw);
		}

		/* set the new descriptor */
		startobjectchange((INTBIG)high->fromgeom->entryaddr.blind,
			high->fromgeom->entryisnode ? VNODEINST : VARCINST);
		us_rotatedescriptI(high->fromgeom, descript);
		us_modifytextdescript(high, descript);

		/* redisplay the text */
		endobjectchange((INTBIG)high->fromgeom->entryaddr.blind,
			high->fromgeom->entryisnode ? VNODEINST : VARCINST);

		/* restore highlighting */
		us_pophighlight(FALSE);
		return;
	}

	if (namesamen(pp, x_("size"), l) == 0 && l >= 2)
	{
		if (count < 2)
		{
			ttyputusage(x_("text size SCALE"));
			return;
		}

		/* get old descriptor */
		if (high->fromvar != NOVARIABLE)
		{
			TDCOPY(olddescript, high->fromvar->textdescript);
		} else if (high->fromport != NOPORTPROTO)
		{
			TDCOPY(olddescript, high->fromport->textdescript);
		} else if (high->fromgeom->entryisnode)
		{
			ni = high->fromgeom->entryaddr.ni;
			TDCOPY(olddescript, ni->textdescript);
		}
		font = TDGETSIZE(olddescript);
		font = us_gettextsize(par[1], font);
		if (font < 0) return;

		/* save highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* set the new size */
		startobjectchange((INTBIG)high->fromgeom->entryaddr.blind,
			high->fromgeom->entryisnode ? VNODEINST : VARCINST);

		TDSETSIZE(olddescript, font);
		us_modifytextdescript(high, olddescript);

		/* redisplay the text */
		endobjectchange((INTBIG)high->fromgeom->entryaddr.blind,
			high->fromgeom->entryisnode ? VNODEINST : VARCINST);

		/* restore highlighting */
		us_pophighlight(FALSE);
		return;
	}

	if (namesamen(pp, x_("interior-only"), l) == 0)
	{
		if (high->fromvar == NOVARIABLE)
		{
			us_abortcommand(_("Interior style only applies to text variables"));
			return;
		}
		if (!high->fromgeom->entryisnode)
		{
			us_abortcommand(_("Interior style only applies to nodes"));
			return;
		}

		ni = high->fromgeom->entryaddr.ni;
		startobjectchange((INTBIG)ni, VNODEINST);
		TDCOPY(descript, high->fromvar->textdescript);
		TDSETINTERIOR(descript, VTINTERIOR);
		us_modifytextdescript(high, descript);
		endobjectchange((INTBIG)ni, VNODEINST);
		return;
	}

	if (namesamen(pp, x_("exterior"), l) == 0 && l >= 2)
	{
		if (high->fromvar == NOVARIABLE)
		{
			us_abortcommand(_("Interior style only applies to text variables"));
			return;
		}
		if (!high->fromgeom->entryisnode)
		{
			us_abortcommand(_("Interior style only applies to nodes"));
			return;
		}
		ni = high->fromgeom->entryaddr.ni;
		startobjectchange((INTBIG)ni, VNODEINST);
		TDCOPY(descript, high->fromvar->textdescript);
		TDSETINTERIOR(descript, 0);
		us_modifytextdescript(high, descript);
		endobjectchange((INTBIG)ni, VNODEINST);
		return;
	}

	ttyputbadusage(x_("text"));
}

void us_undo(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG ret, i, j;
	REGISTER BOOLEAN majorchange, doingundo;
	REGISTER CHAR *direction;
	TOOL *tool;
	REGISTER NODEPROTO *np;
	REGISTER LIBRARY *lib;
	INTBIG *totals;

	if (count > 0 && namesamen(par[0], x_("clear"), estrlen(par[0])) == 0)
	{
		noundoallowed();
		return;
	}

	if (count > 0 && namesamen(par[0], x_("save"), estrlen(par[0])) == 0)
	{
		if (count < 2)
		{
			ttyputusage(x_("undo save AMOUNT"));
			return;
		}

		/* set new history list size */
		i = eatoi(par[1]);
		if (i <= 0)
		{
			us_abortcommand(_("Must have positive history list size"));
			return;
		}
		j = historylistsize(i);
		ttyputverbose(M_("History list size changed from %ld to %ld entries"), j, i);
		return;
	}

	doingundo = TRUE;
	direction = x_("undo");
	if (count > 0 && namesamen(par[0], x_("redo"), estrlen(par[0])) == 0)
	{
		count--;
		doingundo = FALSE;
		direction = x_("redo");
	}

	/* special case if current window is text editor: undo handled by editor */
	if (el_curwindowpart != NOWINDOWPART && doingundo)
	{
		if ((el_curwindowpart->state&WINDOWTYPE) == POPTEXTWINDOW ||
			(el_curwindowpart->state&WINDOWTYPE) == TEXTWINDOW)
		{
			us_undotext(el_curwindowpart);
			return;
		}
	}

	if (count != 0)
	{
		i = eatoi(par[0]);
		if (i <= 0)
		{
			us_abortcommand(_("Must %s a positive number of changes"), direction);
			return;
		}
	} else i = -1;

	/* do the undo */
	totals = (INTBIG *)emalloc((el_maxtools * SIZEOFINTBIG), el_tempcluster);
	if (totals == 0)
	{
		ttyputnomemory();
		return;
	}
	for(j=0; j<el_maxtools; j++) totals[j] = 0;

	/* do the undo/redo */
	majorchange = FALSE;
	for(j=0; ; j++)
	{
		if (doingundo) ret = undoabatch(&tool); else
			ret = redoabatch(&tool);
		if (ret == 0)
		{
			if (j != 0) ttyputmsg(_("Partial change %sne"), direction); else
				us_abortcommand(_("No changes to %s"), direction);
			break;
		}
		totals[tool->toolindex]++;
		if (i < 0)
		{
			/* no batch count specified: go back to significant change */
			if (ret > 0) majorchange = TRUE;
			if (majorchange)
			{
				/* if redoing, keep on redoing unimportant batches */
				if (!doingundo)
				{
					for(;;)
					{
						if (undonature(FALSE) >= 0) break;
						(void)redoabatch(&tool);
					}
				}
				break;
			}
		} else
		{
			/* batch count given: stop when requested number done */
			if (j >= i-1) break;
		}
	}

	for(j=0; j<el_maxtools; j++) if (totals[j] != 0)
		ttyputverbose(x_("%ld %s %s %sne"), totals[j], el_tools[j].toolname,
			makeplural(_("change"), totals[j]), direction);
	efree((CHAR *)totals);

	/* if a cell is the current node, make sure it still exists */
	if (us_curnodeproto != NONODEPROTO && us_curnodeproto->primindex == 0 &&
		(us_state&NONPERSISTENTCURNODE) == 0)
	{
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				if (us_curnodeproto == np) break;
		if (np == NONODEPROTO)
		{
			np = el_curlib->firstnodeproto;
			if (np == NONODEPROTO) np = el_curtech->firstnodeproto;
			us_setnodeproto(np);
		}
	}
}

/*
 * routine to determine whether the string "testcase" satisfies the pattern "pattern".
 * Returns true if it matches.
 */
static BOOLEAN us_patternmatch(CHAR *pattern, CHAR *testcase);
static BOOLEAN us_patternmatchhere(CHAR *pattern, CHAR *testcase);

BOOLEAN us_patternmatch(CHAR *pattern, CHAR *testcase)
{
	REGISTER INTBIG i, testlen;

	/* loop through every position in the testcase, seeing if the pattern matches */
	testlen = estrlen(testcase);
	for(i=0; i<testlen; i++)
		if (us_patternmatchhere(pattern, &testcase[i]) != 0) return(TRUE);

	/* not a match */
	return(FALSE);
}

BOOLEAN us_patternmatchhere(CHAR *pattern, CHAR *testcase)
{
	REGISTER INTBIG i, j, patlen;
	CHAR *subpattern;

	/* loop through every character in the pattern, seeing if it matches */
	patlen = estrlen(pattern);
	for(i=0; i<patlen; i++)
	{
		if (pattern[i] == '*')
		{
			subpattern = &pattern[i+1];
			if (*subpattern == 0) return(TRUE);
			for(j=0; testcase[i+j] != 0; j++)
			{
				if (us_patternmatchhere(subpattern, &testcase[i+j])) return(TRUE);
			}
			return(FALSE);
		}
		if (pattern[i] != testcase[i]) return(FALSE);
	}

	/* made it through the pattern, it matches */
	return(TRUE);
}

void us_var(INTBIG count, CHAR *par[])
{
	REGISTER BOOLEAN negated, inplace;
	REGISTER INTBIG maxleng, i, j, k, l, function, newval, search, oldaddr, language,
		annotate, *newarray, oldlen, len1, len2, count1, count2, count3, disppart;
	INTBIG aindex, objaddr, objaddr1, objaddr2, objtype, objtype1, objtype2, newtype, newaddr;
	UINTBIG newdescript[TEXTDESCRIPTSIZE];
	INTBIG dummy;
	BOOLEAN comvar;
	BOOLEAN but;
	REGISTER VARIABLE *var, *tvar, *res, *var1, *var2, *nvar;
	static VARIABLE fvar;
	REGISTER GEOM **list, *geom;
	REGISTER NODEINST *ni;
	VARIABLE *evar, fvar1, fvar2;
	REGISTER NODEPROTO *np;
	REGISTER LIBRARY *lib;
	REGISTER ARCPROTO *ap;
	POPUPMENU *me, *cme;
	REGISTER POPUPMENUITEM *mi;
	REGISTER WINDOWPART *w, *oldw;
	REGISTER EDITOR *ed;
	REGISTER float oldfloat, oldden, newfloat;
	CHAR *pp, *qual, *qual1, *qual2, varname[100], line[50], *name, *code,
		*dummyfile[1], *header, *edname, *savequal, qualname[300];
	extern COMCOMP us_varvep, us_varvdp, us_varvsp, us_varvalp, us_varvalcp;
	static CHAR nullstr[] = {x_("")};
	REGISTER void *infstr;

	/* show all command interpreter variables if nothing specified */
	if (count == 0)
	{
		j = 0;
		for(i=0; i<52; i++)
		{
			if (i < 26) l = 'a' + i; else
				l = 'A' + i - 26;
			var = getval((INTBIG)us_tool, VTOOL, -1, us_commandvarname((INTSML)l));
			if (var == NOVARIABLE) continue;
			(void)esnprintf(varname, 100, x_("%%%c"), (CHAR)l);
			ttyputmsg(_("%s variable '%s' is %s"), us_variableattributes(var, -1),
				varname, describevariable(var, -1, 0));
			j++;
		}
		if (j == 0) ttyputmsg(_("No command interpreter variables are set"));
		return;
	}

	/* get the variable option */
	l = estrlen(pp = par[0]);

	if (namesamen(pp, x_("options"), l) == 0 && l >= 2)
	{
		if (count <= 1)
		{
			var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_ignoreoptionchangeskey);
			if (var == NOVARIABLE)
				ttyputmsg(_("Option changes are being tracked")); else
					ttyputmsg(_("Option changes are being ignored"));
			return;
		}
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("ignore"), l) == 0)
		{
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_ignoreoptionchangeskey, 1,
				VINTEGER|VDONTSAVE);
			ttyputverbose(M_("Option changes are being ignored"));
			return;
		}
		if (namesamen(pp, x_("track"), l) == 0)
		{
			var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_ignoreoptionchangeskey);
			if (var != NOVARIABLE)
				(void)delvalkey((INTBIG)us_tool, VTOOL, us_ignoreoptionchangeskey);
			ttyputverbose(M_("Option changes are being tracked"));
			return;
		}
		if (namesamen(pp, x_("save"), l) == 0)
		{
			us_saveoptions();
			return;
		}
		ttyputusage(x_("var options [ignore | track | save]"));
		return;
	}

	if (namesamen(pp, x_("reinherit"), l) == 0 && l >= 3)
	{
		list = us_gethighlighted(WANTNODEINST, 0, 0);
		if (list[0] == NOGEOM)
		{
			us_abortcommand(_("Must select nodes for reinheriting attributes"));
			return;
		}
		for(i=0; list[i] != NOGEOM; i++)
		{
			geom = list[i];
			if (!geom->entryisnode) continue;
			ni = geom->entryaddr.ni;
			us_inheritattributes(ni);
		}
		ttyputmsg(_("%ld nodes have had their parameters reinherited"), i);
		return;
	}

	if (namesamen(pp, x_("total-reinherit"), l) == 0 && l >= 9)
	{
		ttyputmsg(_("Reinheriting parameters on all instances in all libraries..."));
		i = 0;
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		{
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				{
					us_inheritattributes(ni);
					i++;
				}
			}
		}
		ttyputmsg(_("%ld nodes have had their parameters reinherited"), i);
		return;
	}


	if (namesamen(pp, x_("relocate"), l) == 0 && l >= 3)
	{
		list = us_gethighlighted(WANTNODEINST, 0, 0);
		if (list[0] == NOGEOM)
		{
			us_abortcommand(_("Must select nodes for relocating attributes"));
			return;
		}
		j = 0;
		for(i=0; list[i] != NOGEOM; i++)
		{
			geom = list[i];
			if (!geom->entryisnode) continue;
			ni = geom->entryaddr.ni;
			if (us_adjustparameterlocations(ni)) j++;
		}
		ttyputmsg(_("%ld nodes have had their parameters relocated"), j);
		return;
	}

	if (namesamen(pp, x_("total-relocate"), l) == 0 && l >= 9)
	{
		ttyputmsg(_("Relocating parameters on all instances in all libraries..."));
		i = 0;
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		{
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				{
					if (us_adjustparameterlocations(ni)) i++;
				}
			}
		}
		ttyputmsg(_("%ld nodes have had their parameters relocated"), i);
		return;
	}

	if (namesamen(pp, x_("visible-all"), l) == 0 && l >= 9)
	{
		/* make all parameters on the current node(s) visible */
		list = us_gethighlighted(WANTNODEINST, 0, 0);
		for(i=0; list[i] != NOGEOM; i++)
		{
			ni = list[i]->entryaddr.ni;
			if (ni->proto->primindex != 0) continue;
			for(j=0; j<ni->numvar; j++)
			{
				var = &ni->firstvar[j];
				nvar = us_findparametersource(var, ni);
				if (nvar == NOVARIABLE) continue;
				if ((var->type&VDISPLAY) != 0) continue;
				startobjectchange((INTBIG)ni, VNODEINST);
				var->type |= VDISPLAY;
				endobjectchange((INTBIG)ni, VNODEINST);
			}
		}
		return;
	}

	if (namesamen(pp, x_("visible-none"), l) == 0 && l >= 9)
	{
		/* make all parameters on the current node(s) invisible */
		list = us_gethighlighted(WANTNODEINST, 0, 0);
		for(i=0; list[i] != NOGEOM; i++)
		{
			ni = list[i]->entryaddr.ni;
			if (ni->proto->primindex != 0) continue;
			for(j=0; j<ni->numvar; j++)
			{
				var = &ni->firstvar[j];
				nvar = us_findparametersource(var, ni);
				if (nvar == NOVARIABLE) continue;
				if ((var->type&VDISPLAY) == 0) continue;
				startobjectchange((INTBIG)ni, VNODEINST);
				var->type &= ~VDISPLAY;
				endobjectchange((INTBIG)ni, VNODEINST);
			}
		}
		return;
	}

	if (namesamen(pp, x_("visible-default"), l) == 0 && l >= 9)
	{
		/* make all parameters on the current node(s) visible or invisible according to the cell */
		list = us_gethighlighted(WANTNODEINST, 0, 0);
		for(i=0; list[i] != NOGEOM; i++)
		{
			ni = list[i]->entryaddr.ni;
			if (ni->proto->primindex != 0) continue;
			for(j=0; j<ni->numvar; j++)
			{
				var = &ni->firstvar[j];
				nvar = us_findparametersource(var, ni);
				if (nvar == NOVARIABLE) continue;
				if (TDGETINTERIOR(nvar->textdescript) == 0)
				{
					/* prototype wants parameter to be visible */
					if ((var->type&VDISPLAY) != 0) continue;
					startobjectchange((INTBIG)ni, VNODEINST);
					var->type |= VDISPLAY;
					endobjectchange((INTBIG)ni, VNODEINST);
				} else
				{
					/* prototype wants parameter to be invisible */
					if ((var->type&VDISPLAY) == 0) continue;
					startobjectchange((INTBIG)ni, VNODEINST);
					var->type &= ~VDISPLAY;
					endobjectchange((INTBIG)ni, VNODEINST);
				}
			}
		}
		return;
	}

	if (namesamen(pp, x_("examine"), l) == 0 && l >= 1)
	{
		if (count <= 1)
		{
			count = ttygetparam(M_("Variable: "), &us_varvep, MAXPARS-1, &par[1]) + 1;
			if (count == 1)
			{
				us_abortedmsg();
				return;
			}
		}
		if (us_getvar(par[1], &objaddr, &objtype, &qual, &comvar, &aindex))
		{
			us_abortcommand(_("Incorrect variable name: %s"), par[1]);
			return;
		}
		code = 0;
		if (*qual != 0)
		{
			search = initobjlist(objaddr, objtype, FALSE);
			if (search == 0)
			{
				us_abortcommand(_("Object is invalid"));
				return;
			}
			for(;;)
			{
				pp = nextobjectlist(&evar, search);
				if (pp == 0)
				{
					us_abortcommand(_("No variable %s on the object"), qual);
					return;
				}
				if (namesame(pp, qual) == 0) break;
			}
			language = evar->type & (VCODE1|VCODE2);
			if (language != 0)
			{
				code = (CHAR *)evar->addr;
				fvar.key = evar->key;
				fvar.type = evar->type;
				var = doquerry(code, language, evar->type);
				if (var == NOVARIABLE) code = 0; else
				{
					var->key = evar->key;
					TDCOPY(var->textdescript, evar->textdescript);
					evar = var;
				}
			}
			var = evar;
		} else
		{
			fvar.addr = objaddr;   fvar.type = objtype;
			var = &fvar;
		}
		if (aindex >= 0 && (var->type&VISARRAY) == 0)
		{
			us_abortcommand(_("%s is not an indexable array"), par[1]);
			return;
		}
		(void)esnprintf(varname, 100, x_("%c%s"), (comvar ? '%' : '$'), par[1]);
		infstr = initinfstr();
		if (code != 0)
		{
			addstringtoinfstr(infstr, code);
			addstringtoinfstr(infstr, x_(" => "));
		}
		addstringtoinfstr(infstr, describevariable(var, aindex, 0));
		ttyputmsg(_("%s variable '%s' is %s"), us_variableattributes(var, aindex),
			varname, returninfstr(infstr));
		return;
	}

	if (namesamen(pp, x_("change"), l) == 0 && l >= 1)
	{
		if (count <= 2)
		{
			ttyputusage(x_("var change VARIABLE STYLE"));
			return;
		}
		if (us_getvar(par[1], &objaddr, &objtype, &qual, &comvar, &aindex))
		{
			us_abortcommand(_("Incorrect variable name: %s"), par[1]);
			return;
		}
		if (comvar)
		{
			us_abortcommand(_("Can only modify database ($) variables"));
			return;
		}
		if (*qual != 0)
		{
			var = getval(objaddr, objtype, -1, qual);
			if (var == NOVARIABLE)
			{
				us_abortcommand(_("No variable %s on the object"), qual);
				return;
			}
		} else
		{
			fvar.addr = objaddr;   fvar.type = objtype;
			var = &fvar;
		}
		if (aindex >= 0 && (var->type&VISARRAY) == 0)
		{
			us_abortcommand(_("%s is not an indexable array"), par[1]);
			return;
		}

		/* make the change */
		l = estrlen(pp = par[2]);
		negated = FALSE;
		if (namesamen(pp, x_("not"), l) == 0 && l >= 2)
		{
			negated = TRUE;
			par++;
			count--;
			if (count <= 2)
			{
				ttyputusage(x_("var change VARIABLE not STYLE"));
				return;
			}
			l = estrlen(pp = par[2]);
		}
		if ((namesamen(pp, x_("display"), l) == 0 && l >= 1) ||
			(namesamen(pp, x_("na-va-display"), l) == 0 && l >= 2) ||
			(namesamen(pp, x_("in-na-va-display"), l) == 0 && l >= 3) ||
			(namesamen(pp, x_("inall-na-va-display"), l) == 0 && l >= 3))
		{
			/* compute the new type field */
			TDCOPY(newdescript, var->textdescript);
			us_figurevariableplace(newdescript, count-3, &par[3]);

			/* signal start of change to nodes and arcs */
			if (objtype == VNODEINST || objtype == VARCINST)
			{
				us_pushhighlight();
				us_clearhighlightcount();
				startobjectchange(objaddr, objtype);
			} else if (objtype == VNODEPROTO)
			{
				us_undrawcellvariable(var, (NODEPROTO *)objaddr);
			}

			/* change the variable */
			if (!negated)
			{
				var->type |= VDISPLAY;
				modifydescript(objaddr, objtype, var, newdescript);
			} else var->type &= ~VDISPLAY;
			disppart = 0;
			if (namesamen(pp, x_("display"), l) == 0 && l >= 1)
				disppart = VTDISPLAYVALUE; else
			if (namesamen(pp, x_("na-va-display"), l) == 0 && l >= 2)
				disppart = VTDISPLAYNAMEVALUE; else
			if (namesamen(pp, x_("in-na-va-display"), l) == 0 && l >= 3)
				disppart = VTDISPLAYNAMEVALINH; else
			if (namesamen(pp, x_("inall-na-va-display"), l) == 0 && l >= 3)
				disppart = VTDISPLAYNAMEVALINHALL;
			if ((disppart == VTDISPLAYNAMEVALINH || disppart == VTDISPLAYNAMEVALINHALL) &&
			    (objtype&VTYPE) != VNODEPROTO)
			{
				ttyputmsg(_("%s is allowed for cells only, changing to na-va-display"), pp);
				disppart = VTDISPLAYNAMEVALUE;	
			}
			TDSETDISPPART(var->textdescript, disppart);

			/* signal end of change to nodes and arcs */
			if (objtype == VNODEINST || objtype == VARCINST)
			{
				endobjectchange(objaddr, objtype);
				us_pophighlight(FALSE);
			} else if (objtype == VNODEPROTO)
			{
				us_drawcellvariable(var, (NODEPROTO *)objaddr);
			}
			return;
		}
		if (negated && namesamen(pp, x_("language"), l) == 0 && l >= 1)
		{
			var->type |= (VCODE1|VCODE2);
			return;
		}
		if (!negated && namesamen(pp, x_("lisp"), l) == 0 && l >= 1)
		{
			var->type |= VLISP;
			return;
		}
		if (!negated && namesamen(pp, x_("tcl"), l) == 0 && l >= 1)
		{
			var->type |= VTCL;
			return;
		}
		if (!negated && namesamen(pp, x_("java"), l) == 0 && l >= 1)
		{
			var->type |= VJAVA;
			return;
		}
		if (namesamen(pp, x_("temporary"), l) == 0 && l >= 1)
		{
			if (!negated) var->type |= VDONTSAVE; else
				var->type &= ~VDONTSAVE;
			return;
		}
		if (namesamen(pp, x_("cannot-change"), l) == 0 && l >= 1)
		{
			if (!negated) var->type |= VCANTSET; else
				var->type &= ~VCANTSET;
			return;
		}
		if (namesamen(pp, x_("interior-only"), l) == 0 && l >= 3)
		{
			if (!negated)
			{
				TDSETINTERIOR(var->textdescript, VTINTERIOR);
			} else
			{
				TDSETINTERIOR(var->textdescript, 0);
			}
			return;
		}
		ttyputbadusage(x_("var change"));
		return;
	}

	if (namesamen(pp, x_("delete"), l) == 0 && l >= 1)
	{
		if (count <= 1)
		{
			count = ttygetparam(_("Variable: "), &us_varvdp, MAXPARS-1, &par[1]) + 1;
			if (count == 1)
			{
				us_abortedmsg();
				return;
			}
		}
		if (us_getvar(par[1], &objaddr, &objtype, &qual, &comvar, &aindex))
		{
			us_abortcommand(_("Incorrect variable name: %s"), par[1]);
			return;
		}
		if (comvar)
		{
			us_abortcommand(_("Can't delete command interpreter variables yet"));
			return;
		}
		if (aindex >= 0)
		{
			us_abortcommand(_("Cannot delete a single variable entry"));
			return;
		}
		if (*qual == 0)
		{
			us_abortcommand(_("Must delete a variable ON some object"));
			return;
		}
		(void)allocstring(&savequal, qual, el_tempcluster);
		var = getval(objaddr, objtype, -1, savequal);
		if (var == NOVARIABLE)
		{
			efree(savequal);
			us_abortcommand(_("No variable %s to delete"), savequal);
			return;
		}

		/* signal start of change to nodes and arcs */
		if (objtype == VNODEINST || objtype == VARCINST)
		{
			us_pushhighlight();
			us_clearhighlightcount();
			startobjectchange(objaddr, objtype);
		} else if (objtype == VNODEPROTO)
		{
			us_undrawcellvariable(var, (NODEPROTO *)objaddr);
		}

		if (delval(objaddr, objtype, savequal))
			ttyputerr(_("Problem deleting variable %s"), par[1]);

		/* signal end of change to nodes and arcs */
		if (objtype == VNODEINST || objtype == VARCINST)
		{
			endobjectchange(objaddr, objtype);
			us_pophighlight(FALSE);
		}
		efree(savequal);
		return;
	}

	if (namesamen(pp, x_("textedit"), l) == 0 && l >= 2)
	{
		if (count <= 1)
		{
			count = ttygetparam(M_("Variable: "), &us_varvep, MAXPARS-1, &par[1]) + 1;
			if (count == 1)
			{
				us_abortedmsg();
				return;
			}
		}
		header = 0;
		inplace = FALSE;
		name = par[1];
		while (count > 2)
		{
			l = estrlen(pp=par[2]);
			if (namesamen(pp, x_("header"), l) == 0)
			{
				if (count > 3) header = par[3]; else
				{
					ttyputusage(x_("var textedit VARIABLE header HEADER"));
					return;
				}
				count--;
				par++;
			} else if (namesamen(pp, x_("in-place"), l) == 0)
			{
				inplace = TRUE;
			} else
			{
				ttyputbadusage(x_("var textedit"));
				return;
			}
			count--;
			par++;
		}
		if (us_getvar(name, &objaddr, &objtype, &qual, &comvar, &aindex))
		{
			us_abortcommand(_("Incorrect variable name: %s"), name);
			return;
		}
		if (*qual != 0)
		{
			var = getval(objaddr, objtype, -1, qual);
			if (var == NOVARIABLE)
			{
				dummyfile[0] = x_("");
				(void)setval(objaddr, objtype, qual, (INTBIG)dummyfile, VSTRING|VISARRAY|(1<<VLENGTHSH));
				var = getval(objaddr, objtype, -1, qual);
				if (var == NOVARIABLE)
				{
					us_abortcommand(_("Cannot create %s on the object"), qual);
					return;
				}
				ttyputverbose(M_("Creating %s on the object"), qual);
			}
			fvar.addr = var->addr;   fvar.type = var->type;
			fvar.key = var->key;     TDCOPY(fvar.textdescript, var->textdescript);
		} else
		{
			fvar.addr = objaddr;   fvar.type = objtype;
		}
		var = &fvar;

		if (inplace)
		{
			if ((var->type&VDISPLAY) == 0)
			{
				us_abortcommand(_("Only displayable variables can be edited in place"));
				return;
			}
			if ((var->type&VTYPE) != VSTRING)
			{
				us_abortcommand(_("Only string variables can be edited in place"));
				return;
			}

			/* save and clear highlighting */
			us_pushhighlight();
			us_clearhighlightcount();

			/* edit the variable */
			us_editvariabletext(var, objtype, objaddr, qual);

			/* restore highlighting */
			us_pophighlight(FALSE);
			return;
		}

		if ((var->type&VISARRAY) == 0)
		{
			/* ensure that the variable is settable */
			if ((var->type&VCANTSET) != 0)
			{
				ttyputmsg(_("Variable cannot be changed; use 'examine' option"));
				return;
			}

			/* save the information about this variable */
			if (us_varqualsave == 0) (void)allocstring(&us_varqualsave, qual, us_tool->cluster); else
				(void)reallocstring(&us_varqualsave, qual, us_tool->cluster);

			/* create the editor window and load it with the variable */
			if (header == 0)
			{
				infstr = initinfstr();
				us_describeeditor(&edname);
				addstringtoinfstr(infstr, edname);
				addstringtoinfstr(infstr, _(" Editor for variable: "));
				addstringtoinfstr(infstr, name);
				header = returninfstr(infstr);
			}
			us_wpop = us_makeeditor(NOWINDOWPART, header, &dummy, &dummy);
			if (us_wpop == NOWINDOWPART) return;

			ed = us_wpop->editor;
			ed->editobjqual = us_varqualsave;
			ed->editobjaddr = (CHAR *)objaddr;
			ed->editobjtype = objtype&VTYPE;
			ed->editobjvar = var;

			/* turn off editor display as data is loaded into it */
			us_suspendgraphics(us_wpop);
			us_addline(us_wpop, 0, describevariable(var, -1, -1));
			us_resumegraphics(us_wpop);

			/* loop on input */
			oldw = el_curwindowpart;
			el_curwindowpart = us_wpop;
			ttyputverbose(M_("You are typing to the editor"));
			modalloop(us_wpopcharhandler, us_wpopbuttonhandler, us_normalcursor);
			el_curwindowpart = oldw;

			/* read back the contents of the editor window */
			(void)allocstring(&pp, us_getline(us_wpop, 0), el_tempcluster);

			/* terminate this popup window */
			startobjectchange((INTBIG)us_tool, VTOOL);
			killwindowpart(us_wpop);
			endobjectchange((INTBIG)us_tool, VTOOL);

			/* signal start of change to nodes and arcs */
			if ((objtype&VTYPE) == VNODEINST || (objtype&VTYPE) == VARCINST)
			{
				us_pushhighlight();
				us_clearhighlightcount();
				startobjectchange(objaddr, objtype&VTYPE);
			}

			switch (ed->editobjvar->type&VTYPE)
			{
				case VINTEGER:
				case VSHORT:
				case VADDRESS:
					res = setval((INTBIG)ed->editobjaddr, ed->editobjtype,
						ed->editobjqual, myatoi(pp), ed->editobjvar->type);
					break;
				case VFLOAT:
				case VDOUBLE:
					newfloat = (float)eatof(pp);
					res = setval((INTBIG)ed->editobjaddr, ed->editobjtype,
						ed->editobjqual, castint(newfloat), ed->editobjvar->type);
					break;
				case VSTRING:
					res = setval((INTBIG)ed->editobjaddr, ed->editobjtype,
						ed->editobjqual, (INTBIG)pp, ed->editobjvar->type);
					break;
				case VPORTPROTO:
					us_renameport((PORTPROTO *)ed->editobjaddr, pp);
					break;
				default:
					ttyputmsg(_("Cannot update this type of variable"));
					break;
			}
			if (res == NOVARIABLE) ttyputerr(_("Error changing variable"));

			/* signal end of change to nodes and arcs */
			if ((objtype&VTYPE) == VNODEINST || (objtype&VTYPE) == VARCINST)
			{
				endobjectchange(objaddr, objtype&VTYPE);
				us_pophighlight(FALSE);
			}
			efree(pp);
			return;
		}

		/* array variable: use full editor */
		if (aindex >= 0) ttyputmsg(_("WARNING: index specification being ignored"));

		/* save the information about this variable */
		if (us_varqualsave == 0) (void)allocstring(&us_varqualsave, qual, us_tool->cluster); else
			(void)reallocstring(&us_varqualsave, qual, us_tool->cluster);

		/* get a new window, put an editor in it */
		w = us_wantnewwindow(0);
		if (w == NOWINDOWPART) return;
		if (header == 0)
		{
			infstr = initinfstr();
			us_describeeditor(&edname);
			addstringtoinfstr(infstr, edname);
			addstringtoinfstr(infstr, _(" Editor for variable: "));
			addstringtoinfstr(infstr, name);
			header = returninfstr(infstr);
		}
		if (us_makeeditor(w, header, &dummy, &dummy) == NOWINDOWPART) return;
		ed = w->editor;
		ed->editobjqual = us_varqualsave;
		ed->editobjaddr = (CHAR *)objaddr;
		ed->editobjtype = objtype;
		ed->editobjvar = var;
		us_suspendgraphics(w);

		/* determine the type of this variable for annotation */
		annotate = 0;
		if (us_varqualsave[0] != 0 && (objtype&VTYPE) == VTECHNOLOGY)
		{
			/* these variables correspond to layer names */
			if (namesame(us_varqualsave, x_("SIM_spice_resistance")) == 0 ||
				namesame(us_varqualsave, x_("SIM_spice_capacitance")) == 0 ||
				namesame(us_varqualsave, x_("TECH_layer_function")) == 0 ||
				namesame(us_varqualsave, x_("USER_layer_letters")) == 0 ||
				namesame(us_varqualsave, x_("DRC_max_distances")) == 0 ||
				namesame(us_varqualsave, x_("IO_gds_layer_numbers")) == 0 ||
				namesame(us_varqualsave, x_("IO_dxf_layer_names")) == 0 ||
				namesame(us_varqualsave, x_("IO_cif_layer_names")) == 0 ||
				namesame(us_varqualsave, x_("IO_skill_layer_names")) == 0)
			{
				annotate = 1;
				tvar = getval(objaddr, objtype, VSTRING|VISARRAY, x_("TECH_layer_names"));
				if (tvar == NOVARIABLE) annotate = 0;
			}

			/* these variables correspond to a primitive nodeproto */
			if (namesame(us_varqualsave, x_("TECH_node_width_offset")) == 0)
			{
				annotate = 2;
				np = ((TECHNOLOGY *)objaddr)->firstnodeproto;
			}

			/* these variables correspond to a color map */
			if (namesame(us_varqualsave, x_("USER_color_map")) == 0) annotate = 3;

			/* these variables correspond to an upper-diagonal layer table */
			if (namesame(us_varqualsave, x_("DRC_min_connected_distances")) == 0 ||
				namesame(us_varqualsave, x_("DRC_min_connected_distances_rule")) == 0 ||
				namesame(us_varqualsave, x_("DRC_min_unconnected_distances")) == 0 ||
				namesame(us_varqualsave, x_("DRC_min_unconnected_distances_rule")) == 0 ||
				namesame(us_varqualsave, x_("DRC_min_connected_distances_wide")) == 0 ||
				namesame(us_varqualsave, x_("DRC_min_connected_distances_wide_rule")) == 0 ||
				namesame(us_varqualsave, x_("DRC_min_unconnected_distances_wide")) == 0 ||
				namesame(us_varqualsave, x_("DRC_min_unconnected_distances_wide_rule")) == 0 ||
				namesame(us_varqualsave, x_("DRC_min_connected_distances_multi")) == 0 ||
				namesame(us_varqualsave, x_("DRC_min_connected_distances_multi_rule")) == 0 ||
				namesame(us_varqualsave, x_("DRC_min_unconnected_distances_multi")) == 0 ||
				namesame(us_varqualsave, x_("DRC_min_unconnected_distances_multi_rule")) == 0 ||
				namesame(us_varqualsave, x_("DRC_min_edge_distances")) == 0 ||
				namesame(us_varqualsave, x_("DRC_min_edge_distances_rule")) == 0)
			{
				annotate = 4;
				maxleng = ((TECHNOLOGY *)objaddr)->layercount;
				j = k = 0;
				tvar = getval(objaddr, objtype, VSTRING|VISARRAY, x_("TECH_layer_names"));
				if (tvar == NOVARIABLE) annotate = 0;
			}

			/* these variables correspond to pairs of X and Y coordinates */
			if (namesame(us_varqualsave, x_("prototype_center")) == 0 ||
				namesame(us_varqualsave, x_("trace")) == 0) annotate = 5;

			/* these variables correspond to a primitive arcproto */
			if (namesame(us_varqualsave, x_("TECH_arc_width_offset")) == 0)
			{
				annotate = 6;
				ap = ((TECHNOLOGY *)objaddr)->firstarcproto;
			}
		}

		l = getlength(var);
		for(i=0; i<l; i++)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, describevariable(var, i, -1));
			switch (annotate)
			{
				case 0: break;
				case 1:
					if (i < getlength(tvar))
					{
						addstringtoinfstr(infstr, x_("     /* "));
						addstringtoinfstr(infstr, ((CHAR **)tvar->addr)[i]);
						addstringtoinfstr(infstr, x_(" */"));
					}
					break;
				case 2:
					if ((i%4) != 0) break;
					if (np != NONODEPROTO)
					{
						addstringtoinfstr(infstr, x_("     /* "));
						addstringtoinfstr(infstr, np->protoname);
						np = np->nextnodeproto;
						addstringtoinfstr(infstr, x_(" */"));
					}
					break;
				case 3:
					(void)esnprintf(line, 50, M_("     /* entry %ld "), i/4);
					addstringtoinfstr(infstr, line);
					switch (i%4)
					{
						case 0: addstringtoinfstr(infstr, M_("red */"));     break;
						case 1: addstringtoinfstr(infstr, M_("green */"));   break;
						case 2: addstringtoinfstr(infstr, M_("blue */"));    break;
						case 3: addstringtoinfstr(infstr, M_("letter */"));  break;
					}
					break;
				case 4:
					if (j < getlength(tvar) && k < getlength(tvar))
					{
						addstringtoinfstr(infstr, x_("     /* "));
						addstringtoinfstr(infstr, ((CHAR **)tvar->addr)[j]);
						addstringtoinfstr(infstr, x_(" TO "));
						addstringtoinfstr(infstr, ((CHAR **)tvar->addr)[k]);
						addstringtoinfstr(infstr, x_(" */"));
					}
					k++;
					if (k >= maxleng)
					{
						j++;
						k = j;
					}
					break;
				case 5:
					if ((i%1) == 0) (void)esnprintf(line, 50, x_("     /* X[%ld] */"), i/2); else
						(void)esnprintf(line, 50, x_("     /* Y[%ld] */"), i/2);
					addstringtoinfstr(infstr, line);
					break;
				case 6:
					if (ap != NOARCPROTO)
					{
						addstringtoinfstr(infstr, x_("     /* "));
						addstringtoinfstr(infstr, describearcproto(ap));
						ap = ap->nextarcproto;
						addstringtoinfstr(infstr, x_(" */"));
					}
					break;
			}
			us_addline(w, i, returninfstr(infstr));
		}
		us_resumegraphics(w);
		w->changehandler = us_varchanges;
		if (annotate != 0) ed->state |= LINESFIXED;
		return;
	}

	if (namesamen(pp, x_("pick"), l) == 0 && l >= 1)
	{
		/* get the name of the object to show in a popup menu */
		if (count <= 1)
		{
			count = ttygetparam(_("Object: "), &us_varvep, MAXPARS-1, &par[1]) + 1;
			if (count == 1)
			{
				us_abortedmsg();
				return;
			}
		}

		/* parse the name into an object, type, qualification, index, etc. */
		if (us_getvar(par[1], &objaddr, &objtype, &qual, &comvar, &aindex))
		{
			us_abortcommand(_("Incorrect variable name: %s"), par[1]);
			return;
		}

		/* cannot examine database variables */
		if (comvar)
		{
			us_abortcommand(_("Cannot use database variables"));
			return;
		}

		/* if the object is qualified, examine the qualification */
		if (*qual != 0)
		{
			var = getval(objaddr, objtype, -1, qual);
			if (var == NOVARIABLE)
			{
				us_abortcommand(_("No variable %s on the object"), qual);
				return;
			}
			objaddr = var->addr;   objtype = var->type;
		}

		/* cannot examine objects with no structure */
		if ((objtype&VTYPE) < VNODEINST || (objtype&VTYPE) == VFRACT || (objtype&VTYPE) == VSHORT || (objtype&VTYPE) == VBOOLEAN)
		{
			us_abortcommand(_("Cannot examine simple objects"));
			return;
		}

		/* if the object is indexed, grab that entry */
		if (aindex >= 0)
		{
			if ((objtype&VISARRAY) == 0)
			{
				us_abortcommand(_("Variable is not an array: cannot index it"));
				return;
			}
			objaddr = ((INTBIG *)objaddr)[aindex];
			objtype &= ~VISARRAY;
		}

		/* cannot examine arrays */
		if ((objtype&VISARRAY) != 0)
		{
			us_abortcommand(_("Must pick single entries of arrays"));
			return;
		}

		/* look through all attributes on the variable */
		search = initobjlist(objaddr, objtype, TRUE);
		if (search == 0)
		{
			us_abortcommand(_("Cannot access variable components"));
			return;
		}

		/* count the number of nonarray attributes and get longest value */
		maxleng = 0;
		for(j=0; ;)
		{
			pp = nextobjectlist(&evar, search);
			if (pp == 0) break;
			if ((evar->type&VISARRAY) != 0) continue;
			j++;
			i = estrlen(describevariable(evar, -1, 0));
			if (i > maxleng) maxleng = i;
		}
		maxleng += 5;

		/* allocate the pop-up menu structure */
		me = (POPUPMENU *)emalloc(sizeof(POPUPMENU), el_tempcluster);
		if (me == 0)
		{
			ttyputnomemory();
			return;
		}
		mi = (POPUPMENUITEM *)emalloc((j * (sizeof (POPUPMENUITEM))), el_tempcluster);
		if (mi == 0)
		{
			ttyputnomemory();
			return;
		}

		/* load up the menu structure */
		search = initobjlist(objaddr, objtype, TRUE);
		if (search == 0)
		{
			us_abortcommand(_("Strange error accessing component"));
			return;
		}
		l = 0;
		for(i=0; ;)
		{
			pp = nextobjectlist(&evar, search);
			if (pp == 0) break;
			if ((evar->type&VISARRAY) != 0) continue;
			if (evar->key == (UINTBIG)-1 || (evar->type&VCANTSET) != 0)
			{
				(void)allocstring(&mi[i].attribute, pp, el_tempcluster);
				mi[i].maxlen = -1;
				mi[i].valueparse = &us_varvalcp;
			} else
			{
				mi[i].attribute = (CHAR *)emalloc((estrlen(pp)+3) * SIZEOFCHAR, el_tempcluster);
				if (mi[i].attribute == 0)
				{
					ttyputnomemory();
					return;
				}
				(void)estrcpy(mi[i].attribute, x_("* "));
				(void)estrcat(mi[i].attribute, pp);
				mi[i].maxlen = maxleng;
				mi[i].valueparse = &us_varvalp;
				l++;
			}
			mi[i].value = (CHAR *)emalloc((maxleng+1) * SIZEOFCHAR, el_tempcluster);
			if (mi[i].value == 0)
			{
				ttyputnomemory();
				return;
			}
			(void)estrcpy(mi[i].value, describevariable(evar, -1, 0));
			mi[i].changed = FALSE;
			mi[i].response = NOUSERCOM;
			i++;
		}
		me->name = x_("noname");
		me->list = mi;
		me->total = j;
		if (l != 0) me->header = _("Set values (* only)"); else
			me->header = _("Examine values");
		but = TRUE;
		cme = me;
		if (us_popupmenu(&cme, &but, TRUE, -1, -1, 0) == 0)
			us_abortcommand(_("This display does not support popup menus"));

		/* now deal with changes made */
		search = initobjlist(objaddr, objtype, TRUE);
		if (search == 0)
		{
			us_abortcommand(_("Strange error accessing component"));
			return;
		}
		for(i=0; ;)
		{
			pp = nextobjectlist(&evar, search);
			if (pp == 0) break;
			if ((evar->type&VISARRAY) != 0) continue;
			if (!mi[i].changed) { i++;   continue; }
			l = 0;
			switch (evar->type & VTYPE)
			{
				case VFLOAT:
				case VDOUBLE:   j = castint((float)eatof(mi[i].value));   break;
				case VSHORT:
				case VBOOLEAN:
				case VINTEGER:  j = myatoi(mi[i].value);          break;
				case VCHAR:     j = (INTBIG)*mi[i].value;          break;
				case VSTRING:
					qual = mi[i].value;
					if (*qual == '"' && qual[estrlen(qual)-1] == '"')
					{
						qual[estrlen(qual)-1] = 0;
						qual++;
					}
					j = (INTBIG)qual;
					break;
				default:
					us_abortcommand(_("Cannot change attribute %s: bad type"), pp);
					l++;
					break;
			}
			if (l == 0)
			{
				(void)setval(objaddr, objtype, pp, j, evar->type);
				ttyputmsg(_("Changed attribute %s to %s"), pp, mi[i].value);
			}
			i++;
		}

		/* free the space */
		for(i=0; i<me->total; i++)
		{
			efree(mi[i].attribute);
			efree(mi[i].value);
		}
		efree((CHAR *)mi);
		efree((CHAR *)me);
		return;
	}

	/* handle array operations */
	if (namesamen(pp, x_("vector"), l) == 0 && l >= 2)
	{
		if (count < 4)
		{
			ttyputusage(x_("var vector DEST SOURCE1 OPERATOR [SOURCE2]"));
			return;
		}

		/* get the destination variable */
		if (us_getvar(par[1], &objaddr, &objtype, &pp, &comvar, &aindex))
		{
			us_abortcommand(_("Incorrect variable name: %s"), par[1]);
			return;
		}
		if (aindex >= 0)
		{
			us_abortcommand(_("Use 'var set' to assign values to array entries, not 'var vector'"));
			return;
		}
		if (us_varqualsave == 0) (void)allocstring(&us_varqualsave, pp, us_tool->cluster); else
			(void)reallocstring(&us_varqualsave, pp, us_tool->cluster);

		/* get the first source variable */
		if (us_getvar(par[2], &objaddr1, &objtype1, &qual1, &comvar, &aindex))
		{
			us_abortcommand(_("Incorrect variable name: %s"), par[2]);
			return;
		}
		if (*qual1 != 0)
		{
			var1 = getval(objaddr1, objtype1, -1, qual1);
			if (var1 == NOVARIABLE)
			{
				us_abortcommand(_("Cannot find first source: %s"), par[2]);
				return;
			}
		} else
		{
			fvar1.addr = objaddr1;
			fvar1.type = objtype1;
			var1 = &fvar1;
		}
		len1 = getlength(var1);
		if (len1 < 0) len1 = 1;
		if (us_expandaddrtypearray(&us_varlimit1, &us_varaddr1, &us_vartype1, len1)) return;
		if ((var1->type&VISARRAY) == 0)
		{
			us_varaddr1[0] = var1->addr;
			us_vartype1[0] = var1->type;
			count1 = 1;
		} else
		{
			if ((var1->type&VTYPE) == VGENERAL)
			{
				for(i=0; i<len1; i += 2)
				{
					us_varaddr1[i/2] = ((INTBIG *)var1->addr)[i];
					us_vartype1[i/2] = ((INTBIG *)var1->addr)[i+1];
				}
				count1 = len1 / 2;
			} else
			{
				for(i=0; i<len1; i++)
				{
					us_varaddr1[i] = ((INTBIG *)var1->addr)[i];
					us_vartype1[i] = var1->type;
				}
				count1 = len1;
			}
		}

		/* get the operator */
		l = estrlen(pp = par[3]);
		count3 = 0;
		if (namesamen(pp, x_("pattern"), l) == 0 && l >= 1)
		{
			/* set 1 where the pattern matches */
			count3 = count1;
			if (us_expandaddrtypearray(&us_varlimit3, &us_varaddr3, &us_vartype3, count3)) return;
			var = &fvar;
			for(i=0; i<count1; i++)
			{
				fvar.type = us_vartype1[i] & VTYPE;
				fvar.addr = us_varaddr1[i];
				pp = describevariable(var, -1, -1);
				if (us_patternmatch(par[4], pp)) us_varaddr3[i] = 1; else
					us_varaddr3[i] = 0;
				us_vartype3[i] = VINTEGER;
			}
		} else if (namesamen(pp, x_("set"), l) == 0 && l >= 3)
		{
			/* copy the first operand to the destination */
			count3 = count1;
			if (us_expandaddrtypearray(&us_varlimit3, &us_varaddr3, &us_vartype3, count3)) return;
			for(i=0; i<count1; i++)
			{
				us_varaddr3[i] = us_varaddr1[i];
				us_vartype3[i] = us_vartype1[i];
			}
		} else if (namesamen(pp, x_("type"), l) == 0 && l >= 1)
		{
			/* set 1 where the type matches */
			count3 = count1;
			if (us_expandaddrtypearray(&us_varlimit3, &us_varaddr3, &us_vartype3, count3)) return;
			j = us_variabletypevalue(par[4]);
			for(i=0; i<count1; i++)
			{
				if ((us_vartype1[i]&VTYPE) == j) us_varaddr3[i] = 1; else us_varaddr3[i] = 0;
				us_vartype3[i] = VINTEGER;
			}
		} else
		{
			/* two-operand operation: get the second source variable */
			if (count < 4)
			{
				ttyputusage(x_("var vector DEST SOURCE1 OP SOURCE2"));
				return;
			}

			if (us_getvar(par[4], &objaddr2, &objtype2, &qual2, &comvar, &aindex))
			{
				us_abortcommand(_("Incorrect variable name: %s"), par[4]);
				return;
			}
			if (*qual2 != 0)
			{
				var2 = getval(objaddr2, objtype2, -1, qual2);
				if (var2 == NOVARIABLE)
				{
					us_abortcommand(_("Cannot find second source: %s"), par[4]);
					return;
				}
			} else
			{
				fvar2.addr = objaddr2;
				fvar2.type = objtype2;
				var2 = &fvar2;
			}
			len2 = getlength(var2);
			if (len2 < 0) len2 = 1;
			if (us_expandaddrtypearray(&us_varlimit2, &us_varaddr2, &us_vartype2, len2)) return;
			if ((var2->type&VISARRAY) == 0)
			{
				us_varaddr2[0] = var2->addr;
				us_vartype2[0] = var2->type;
				count2 = 1;
			} else
			{
				if ((var2->type&VTYPE) == VGENERAL)
				{
					for(i=0; i<len2; i += 2)
					{
						us_varaddr2[i/2] = ((INTBIG *)var2->addr)[i];
						us_vartype2[i/2] = ((INTBIG *)var2->addr)[i+1];
					}
					count2 = len2 / 2;
				} else
				{
					for(i=0; i<len2; i++)
					{
						us_varaddr2[i] = ((INTBIG *)var2->addr)[i];
						us_vartype2[i] = var2->type;
					}
					count2 = len2;
				}
			}

			if (namesamen(pp, x_("concat"), l) == 0 && l >= 1)
			{
				count3 = count1 + count2;
				if (us_expandaddrtypearray(&us_varlimit3, &us_varaddr3, &us_vartype3, count3)) return;
				for(i=0; i<count1; i++)
				{
					us_varaddr3[i] = us_varaddr1[i];
					us_vartype3[i] = us_vartype1[i];
				}
				for(i=0; i<count2; i++)
				{
					us_varaddr3[i+count1] = us_varaddr2[i];
					us_vartype3[i+count1] = us_vartype2[i];
				}
			} else if (namesamen(pp, x_("select"), l) == 0 && l >= 3)
			{
				count3 = 0;
				if (count1 < count2) count2 = count1;
				for(i=0; i<count2; i++)
					if ((us_vartype2[i]&VTYPE) == VINTEGER && us_varaddr2[i] != 0) count3++;
				if (us_expandaddrtypearray(&us_varlimit3, &us_varaddr3, &us_vartype3, count3)) return;
				count3 = 0;
				for(i=0; i<count2; i++)
					if ((us_vartype2[i]&VTYPE) == VINTEGER && us_varaddr2[i] != 0)
				{
					us_varaddr3[count3] = us_varaddr1[i];
					us_vartype3[count3] = us_vartype1[i];
					count3++;
				}
			}
		}

		/* assign the result to the destination */
		if (count3 == 0)
		{
			/* null array */
			us_varaddr3[0] = -1;
			setval(objaddr, objtype, us_varqualsave, (INTBIG)us_varaddr3, VUNKNOWN|VISARRAY|VDONTSAVE);
			return;
		}
		if (count3 == 1)
		{
			setval(objaddr, objtype, us_varqualsave, us_varaddr3[0], (us_vartype3[0]&VTYPE)|VDONTSAVE);
		} else
		{
			for(i=1; i<count3; i++)
				if ((us_vartype3[i-1]&VTYPE) != (us_vartype3[i]&VTYPE)) break;
			if (count3 > 1 && i < count3)
			{
				/* general array needed because not all entries are the same type */
				newarray = (INTBIG *)emalloc(count3*2 * SIZEOFINTBIG, el_tempcluster);
				if (newarray == 0) return;
				for(i=0; i<count3; i++)
				{
					newarray[i*2] = us_varaddr3[i];
					newarray[i*2+1] = us_vartype3[i];
				}
				setval(objaddr, objtype, us_varqualsave, (INTBIG)newarray,
					VGENERAL|VISARRAY|((count3*2)<<VLENGTHSH)|VDONTSAVE);
				efree((CHAR *)newarray);
			} else
			{
				/* uniform array */
				setval(objaddr, objtype, us_varqualsave, (INTBIG)us_varaddr3,
					(us_vartype3[0]&VTYPE)|VISARRAY|(count3<<VLENGTHSH)|VDONTSAVE);
			}
		}
		return;
	}

	/* handle the setting and modifying options */
	function = 0;
	if (namesamen(pp, x_("set"), l) == 0 && l >= 1) function = 's';
	if (namesamen(pp, x_("+"), l) == 0 && l >= 1) function = '+';
	if (namesamen(pp, x_("-"), l) == 0 && l >= 1) function = '-';
	if (namesamen(pp, x_("*"), l) == 0 && l >= 1) function = '*';
	if (namesamen(pp, x_("/"), l) == 0 && l >= 1) function = '/';
	if (namesamen(pp, x_("mod"), l) == 0 && l >= 1) function = 'm';
	if (namesamen(pp, x_("and"), l) == 0 && l >= 1) function = 'a';
	if (namesamen(pp, x_("or"), l) == 0 && l >= 1) function = 'o';
	if (namesamen(pp, x_("|"), l) == 0 && l >= 1) function = '|';
	if (function == 0)
	{
		ttyputusage(x_("var [examine|set|delete|OP] VARIABLE MOD"));
		ttyputmsg(x_("OP: + - * / mod | or and"));
		return;
	}

	/* make sure there is a variable name to set/modify */
	if (count <= 1)
	{
		count = ttygetparam(x_("Variable: "), &us_varvsp, MAXPARS-1, &par[1]) + 1;
		if (count == 1)
		{
			us_abortedmsg();
			return;
		}
	}
	if (us_getvar(par[1], &objaddr, &objtype, &qual, &comvar, &aindex))
	{
		us_abortcommand(_("Incorrect variable name: %s"), par[1]);
		return;
	}
	estrcpy(qualname, qual);
	var = getval(objaddr, objtype, -1, qualname);
	if (var != NOVARIABLE)
	{
		newtype = var->type;
		TDCOPY(newdescript, var->textdescript);

		/* do some pre-computation on existing variables */
		if ((newtype&VCANTSET) != 0)
		{
			us_abortcommand(_("%s is not a settable variable"), par[1]);
			return;
		}
		oldlen = getlength(var);
		if (aindex >= 0 && (newtype&VISARRAY) == 0)
		{
			us_abortcommand(_("%s is not an array: cannot index it"), par[1]);
			return;
		}
		if (aindex < 0 && (newtype&VISARRAY) != 0)
		{
			ttyputmsg(_("Warning: %s was an array: now scalar"), par[1]);
			newtype &= ~VISARRAY;
		}
	}

	/* make sure there is a value to set/adjust */
	if (count <= 2)
	{
		count = ttygetparam(M_("Value: "), &us_varvalp, MAXPARS-2, &par[2]) + 2;
		if (count == 2)
		{
			us_abortedmsg();
			return;
		}
	}

	/* make sure modifications are done on the right types */
	if (function != 's')
	{
		/* operation is "+", "-", "*", "/", "m", "a", "o", "|" */
		if (var == NOVARIABLE)
		{
			us_abortcommand(_("Variable does not exist: cannot be modified"));
			return;
		}
		if (aindex < 0 && (newtype&VISARRAY) != 0)
		{
			us_abortcommand(_("%s is an array: must index it"), par[1]);
			return;
		}
		if (aindex < 0) oldaddr = var->addr; else
		{
			if ((newtype&VISARRAY) == 0)
			{
				us_abortcommand(_("%s is not an array: cannot index it"), par[1]);
				return;
			}
			if (aindex >= oldlen)
			{
				us_abortcommand(_("%s has only %ld entries, cannot use %ld"), par[1], oldlen, aindex);
				return;
			}
			if ((newtype&VTYPE) == VDOUBLE)
				oldaddr = (INTBIG)(((double *)var->addr)[aindex]); else
					oldaddr = ((INTBIG *)var->addr)[aindex];
		}
		if (function != '|')
		{
			/* operation is "+", "-", "*", "/", "m", "a", "o" */
			if ((newtype&VTYPE) != VINTEGER && (newtype&VTYPE) != VFLOAT &&
				(newtype&VTYPE) != VDOUBLE && (newtype&VTYPE) != VFRACT &&
				(newtype&VTYPE) != VSHORT)
			{
				us_abortcommand(_("Can only do arithmetic on numbers"));
				return;
			}

			if ((newtype&VTYPE) == VFLOAT || (newtype&VTYPE) == VDOUBLE)
			{
				oldfloat = castfloat(oldaddr);
			} else if ((newtype&VTYPE) == VINTEGER)
			{
				/* might be able to re-cast integers as floats */
				for(pp = par[2]; *pp != 0; pp++) if (*pp == '.')
				{
					if ((newtype&VISARRAY) != 0)
					{
						us_abortcommand(_("Cannot change array to floating point"));
						return;
					}
					newtype = (newtype & ~VTYPE) | VFLOAT;
					oldfloat = (float)oldaddr;
					break;
				}
			}
			if ((newtype&VTYPE) != VINTEGER && (function == 'o' || function == 'a' || function == 'm'))
			{
				us_abortcommand(_("Must do bit or modulo arithmetic on integers"));
				return;
			}
		} else if ((newtype&VTYPE) != VSTRING)
		{
			us_abortcommand(_("Can only do concatentation on strings"));
			return;
		}
	} else
	{
		/* setting a nonexistent variable: determine the type */
		if (var == NOVARIABLE || (var->type&VISARRAY) == 0)
		{
			/* establish the type of this parameter if it is being set */
			getsimpletype(par[2], &newtype, &newaddr, 0);
			if (objtype == VARCINST) geom = ((ARCINST *)objaddr)->geom; else
				if (objtype == VNODEINST) geom = ((NODEINST *)objaddr)->geom; else
					geom = NOGEOM;
			TDCLEAR(newdescript);
			defaulttextdescript(newdescript, geom);
		}
	}

	/* get options on how to change the variable */
	disppart = 0;
	if (count >= 4)
	{
		l = estrlen(pp = par[3]);
		if (namesamen(pp, x_("display"), l) == 0 && l >= 1)
		{
			newtype |= VDISPLAY;
			disppart = VTDISPLAYVALUE;
			us_figurevariableplace(newdescript, count-4, &par[4]);
			l = 0;
		} else if (namesamen(pp, x_("na-va-display"), l) == 0 && l >= 1)
		{
			newtype |= VDISPLAY;
			disppart = VTDISPLAYNAMEVALUE;
			us_figurevariableplace(newdescript, count-4, &par[4]);
			l = 0;
		} else if (namesamen(pp, x_("in-na-va-display"), l) == 0 && l >= 3)
		{
			newtype |= VDISPLAY;
			disppart = VTDISPLAYNAMEVALINH;
			if ((objtype&VTYPE) != VNODEPROTO)
			{
				ttyputmsg(_("%s is allowed for cells only, changing to na-va-display"), pp);
				disppart = VTDISPLAYNAMEVALUE;	
			}
			us_figurevariableplace(newdescript, count-4, &par[4]);
			l = 0;
		} else if (namesamen(pp, x_("inall-na-va-display"), l) == 0 && l >= 3)
		{
			newtype |= VDISPLAY;
			disppart = VTDISPLAYNAMEVALINHALL;
			if ((objtype&VTYPE) != VNODEPROTO)
			{
				ttyputmsg(_("%s is allowed for cells only, changing to na-va-display"), pp);
				disppart = VTDISPLAYNAMEVALUE;	
			}
			us_figurevariableplace(newdescript, count-4, &par[4]);
			l = 0;
		} else if (namesamen(pp, x_("lisp"), l) == 0 && l >= 1)
		{
			if (aindex < 0 || var == NOVARIABLE) newtype = (newtype & ~(VCODE1|VCODE2)) | VLISP;
			l = 0;
		} else if (namesamen(pp, x_("tcl"), l) == 0 && l >= 1)
		{
			if (aindex < 0 || var == NOVARIABLE) newtype = (newtype & ~(VCODE1|VCODE2)) | VTCL;
			l = 0;
		} else if (namesamen(pp, x_("java"), l) == 0 && l >= 1)
		{
			if (aindex < 0 || var == NOVARIABLE) newtype = (newtype & ~(VCODE1|VCODE2)) | VJAVA;
			l = 0;
		} else if (namesamen(pp, x_("temporary"), l) == 0 && l >= 1)
		{
			if (aindex < 0 || var == NOVARIABLE) newtype |= VDONTSAVE;
			l = 0;
		} else if (namesamen(pp, x_("fractional"), l) == 0 && l >= 1)
		{
			if (aindex < 0 || var == NOVARIABLE) newtype = (newtype & ~VTYPE) | VFRACT;
			l = 0;
		} else if (namesamen(pp, x_("float"), l) == 0 && l >= 1)
		{
			if (aindex < 0 || var == NOVARIABLE) newtype = (newtype & ~VTYPE) | VFLOAT;
			l = 0;
		} else if (namesamen(pp, x_("cannot-change"), l) == 0 && l >= 1)
		{
			if (aindex < 0 || var == NOVARIABLE) newtype |= VCANTSET;
			l = 0;
		}
		if (l > 0)
		{
			ttyputusage(x_("var set VARIABLE [OPTION]"));
			ttyputusage(x_("OPTION: lisp|tcl|java|display|temporary|fractional|float|cannot-change"));
			return;
		}
	}

	/* do the modification */
	switch (function)
	{
		case '+':
			if ((newtype&VTYPE) == VINTEGER) newval = oldaddr + myatoi(par[2]); else
				if ((newtype&VTYPE) == VFRACT) newval = oldaddr + atofr(par[2]); else
					newval = castint(oldfloat + (float)eatof(par[2]));
			break;
		case '-':
			if ((newtype&VTYPE) == VINTEGER) newval = oldaddr - myatoi(par[2]); else
				if ((newtype&VTYPE) == VFRACT) newval = oldaddr - atofr(par[2]); else
					newval = castint(oldfloat - (float)eatof(par[2]));
			break;
		case '*':
			if ((newtype&VTYPE) == VINTEGER) newval = oldaddr * myatoi(par[2]); else
				if ((newtype&VTYPE) == VFRACT) newval = muldiv(oldaddr, atofr(par[2]), WHOLE); else
					newval = castint(oldfloat * (float)eatof(par[2]));
			break;
		case '/':
			if ((newtype&VTYPE) == VINTEGER)
			{
				i = myatoi(par[2]);
				if (i != 0) newval = oldaddr / i; else
				{
					ttyputerr(_("Attempt to divide by zero"));
					newval = 0;
				}
			} else if ((newtype&VTYPE) == VFRACT)
			{
				i = atofr(par[2]);
				if (i != 0) newval = muldiv(oldaddr, WHOLE, i); else
				{
					ttyputerr(_("Attempt to divide by zero"));
					newval = 0;
				}
			} else
			{
				oldden = (float)eatof(par[2]);
				if (oldden != 0.0)
					newval = castint((float)(oldfloat / oldden)); else
				{
					ttyputerr(_("Attempt to divide by zero"));
					newval = castint(0.0);
				}
			}
			break;
		case 'm':
			i = myatoi(par[2]);
			if (i != 0) newval = oldaddr % i; else
			{
				ttyputerr(_("Attempt to modulo by zero"));
				newval = 0;
			}
			break;
		case 'a':
			newval = oldaddr & myatoi(par[2]);   break;
		case 'o':
			newval = oldaddr | myatoi(par[2]);   break;
		case '|':
			infstr = initinfstr();
			addstringtoinfstr(infstr, (CHAR *)oldaddr);
			addstringtoinfstr(infstr, par[2]);
			newval = (INTBIG)returninfstr(infstr);
			break;
		case 's':
			if ((newtype&VTYPE) == VINTEGER || (newtype&VTYPE) == VSHORT || (newtype&VTYPE) == VBOOLEAN)
				newval = myatoi(par[2]); else
					if ((newtype&VTYPE) == VFRACT) newval = atofr(par[2]); else
						if ((newtype&VTYPE) == VFLOAT || (newtype&VTYPE) == VDOUBLE)
							newval = castint((float)eatof(par[2])); else
								newval = (INTBIG)par[2];
			break;
	}

	/* ensure that command interpreter variables are always temporary */
	if (comvar) newtype |= VDONTSAVE;

	/* signal start of change to nodes and arcs */
	if (objtype == VNODEINST || objtype == VARCINST)
	{
		us_pushhighlight();
		us_clearhighlightcount();
		startobjectchange(objaddr, objtype);
	} else if (objtype == VNODEPROTO)
	{
		if (var != NOVARIABLE) us_undrawcellvariable(var, (NODEPROTO *)objaddr);
	}

	/* change the variable */
	if (aindex < 0) res = setval(objaddr, objtype, qualname, newval, newtype); else
	{
		if (var == NOVARIABLE)
		{
			/* creating array variable */
			newarray = emalloc(((aindex+1) * SIZEOFINTBIG), el_tempcluster);
			if (newarray == 0)
			{
				ttyputnomemory();
				return;
			}
			for(j=0; j<=aindex; j++) newarray[j] = newval;
			newtype |= VISARRAY | ((aindex+1)<<VLENGTHSH);
			res = setval(objaddr, objtype, qualname, (INTBIG)newarray, newtype);
			efree((CHAR *)newarray);
		} else if (oldlen <= aindex)
		{
			/* extend existing array variable */
			newarray = emalloc(((aindex+1) * SIZEOFINTBIG), el_tempcluster);
			if (newarray == 0)
			{
				ttyputnomemory();
				return;
			}
			if ((newtype&VTYPE) == VSTRING)
			{
				for(j=0; j<oldlen; j++)
					(void)allocstring((CHAR **)&newarray[j],
						(CHAR *)((INTBIG *)var->addr)[j], el_tempcluster);
				for(j=oldlen; j<aindex; j++) newarray[j] = (INTBIG)nullstr;
			} else
			{
				for(j=0; j<oldlen; j++) newarray[j] = ((INTBIG *)var->addr)[j];
				for(j=oldlen; j<aindex; j++) newarray[j] = 0;
			}
			newarray[aindex] = newval;
			newtype = (newtype & ~VLENGTH) | ((aindex+1)<<VLENGTHSH);
			res = setval(objaddr, objtype, qualname, (INTBIG)newarray, newtype);
			if ((newtype&VTYPE) == VSTRING)
				for(j=0; j<oldlen; j++) efree((CHAR *)newarray[j]);
			efree((CHAR *)newarray);
		} else
		{
			if (setind(objaddr, objtype, qualname, aindex, newval))
				res = NOVARIABLE; else res = (VARIABLE *)1;
		}
	}
	if (res == NOVARIABLE)
	{
		ttyputnomemory();
		return;
	}
	if (res != (VARIABLE *)1)
	{
		TDCOPY(res->textdescript, newdescript);
		TDSETDISPPART(res->textdescript, disppart);
	}

	/* signal end of change to nodes and arcs */
	if (objtype == VNODEINST || objtype == VARCINST)
	{
		endobjectchange(objaddr, objtype);
		us_pophighlight(FALSE);
	} else if (objtype == VNODEPROTO)
	{
		if (res != NOVARIABLE) us_drawcellvariable(res, (NODEPROTO *)objaddr);
	}
}

void us_view(INTBIG count, CHAR *par[])
{
	REGISTER CHAR *pp, *abbrev, *pt;
	REGISTER INTBIG i, l;
	REGISTER BOOLEAN found;
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;
	REGISTER VARIABLE *var;
	REGISTER VIEW *nv;
	REGISTER WINDOWPART *w;
	REGISTER EDITOR *ed;
	REGISTER void *infstr;
	CHAR line[10];

	if (count == 0)
	{
		ttyputusage(x_("view new | delete | change | frame"));
		return;
	}

	l = estrlen(pp = par[0]);

	if (namesamen(pp, x_("new"), l) == 0 && l >= 1)
	{
		if (count < 2)
		{
			ttyputusage(x_("view new NEWVIEWNAME [ABBREVIATION [TYPE]]"));
			return;
		}

		pp = par[1];
		nv = getview(pp);

		/* special case for multi-page schematics */
		if (namesamen(pp, x_("schematic-page-"), 15) == 0)
		{
			/* check validity of abbreviation for schematic page view */
			pt = &pp[15];
			if (!isanumber(&pp[15]))
			{
				us_abortcommand(_("Page number must follow 'schematics-page-'"));
				return;
			}
			if (nv != NOVIEW)
			{
				ttyputverbose(M_("Schematic page %s already exists"), &pp[15]);
				return;
			}
		}

		/* make sure name is unique */
		if (nv != NOVIEW)
		{
			us_abortcommand(_("Already a view called '%s'"), nv->viewname);
			return;
		}

		if (count >= 3)
		{
			/* get the short view name */
			abbrev = par[2];
			if (namesamen(pp, x_("schematic-page-"), 15) == 0)
			{
				if ((abbrev[0] != 'p' && abbrev[0] != 'P') || estrcmp(&pp[15], &abbrev[1]) != 0)
				{
					us_abortcommand(_("Incorrect abbreviation for schematic page"));
					count = 2;
				}
			} else
			{
				/* make sure the abbreviation is not confused with schematic one */
				if ((abbrev[0] == 'p' || abbrev[0] == 'P') && isanumber(&abbrev[1]))
				{
					us_abortcommand(_("Can only use 'Pnumber' abbreviation for multipage schematic views"));
					return;
				}
			}
		}
		if (count < 3)
		{
			/* generate a short view name */
			if (namesamen(pp, x_("schematic-page-"), 15) == 0)
			{
				/* special case for schematics pages */
				abbrev = (CHAR *)emalloc((estrlen(&pp[15])+2) * SIZEOFCHAR, el_tempcluster);
				abbrev[0] = 'p';
				(void)estrcpy(&abbrev[1], &pp[15]);
			} else
			{
				/* simply find initial unique letters */
				for(i=0; pp[i] != 0; i++)
				{
					for(nv = el_views; nv != NOVIEW; nv = nv->nextview)
						if (namesamen(nv->sviewname, pp, i+1) == 0) break;
					if (nv == NOVIEW) break;
				}
				abbrev = (CHAR *)emalloc((i+2) * SIZEOFCHAR, el_tempcluster);
				(void)estrncpy(abbrev, pp, i+1);
				abbrev[i+1] = 0;
			}
			ttyputverbose(M_("Using '%s' as abbreviation"), abbrev);
		}

		/* see if the view already exists */
		nv = getview(pp);
		if (nv != NOVIEW)
		{
			if (namesame(nv->sviewname, abbrev) != 0)
				ttyputmsg(_("View %s already exists with abbreviation %s"), nv->viewname, nv->sviewname);
			return;
		}

		/* create the view */
		nv = newview(pp, abbrev);
		if (nv == NOVIEW)
		{
			us_abortcommand(_("Could not create the new view"));
			return;
		}

		/* get type of view */
		if (count >= 4)
		{
			pp = par[3];
			if (namesamen(pp, x_("text"), estrlen(pp)) == 0) nv->viewstate |= TEXTVIEW;
		}
		ttyputverbose(M_("View '%s' created"), pp);

		/* clean up */
		if (count < 3) efree(abbrev);
		return;
	}

	if (namesamen(pp, x_("delete"), l) == 0 && l >= 1)
	{
		if (count < 2)
		{
			ttyputusage(x_("view delete VIEWNAME"));
			return;
		}

		/* make sure name exists */
		nv = getview(par[1]);
		if (nv == NOVIEW)
		{
			us_abortcommand(_("Unknown view: %s"), par[1]);
			return;
		}

		if ((nv->viewstate&PERMANENTVIEW) != 0)
		{
			us_abortcommand(_("Cannot delete important views like %s"), nv->viewname);
			return;
		}

		/* make sure no cells in any library have this view */
		found = FALSE;
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				if (np->cellview == nv)
		{
			if (!found)
			{
				us_abortcommand(_("Must first remove these cells in view %s"), nv->viewname);
				found = TRUE;
			}
			ttyputmsg(x_("   %s"), describenodeproto(np));
		}
		if (found) return;

		/* delete the view */
		if (killview(nv)) ttyputerr(_("Problem deleting view"));
			else ttyputverbose(M_("View '%s' deleted"), par[1]);
		return;
	}

	if (namesamen(pp, x_("change"), l) == 0 && l >= 1)
	{
		if (count != 3)
		{
			ttyputusage(x_("view change CELL VIEW"));
			return;
		}

		/* get cell */
		np = getnodeproto(par[1]);
		if (np == NONODEPROTO || np->primindex != 0)
		{
			us_abortcommand(_("'%s' is not a cell"), par[1]);
			return;
		}

		/* disallow change if lock is on */
		if (us_cantedit(np, NONODEINST, TRUE)) return;

		/* get view */
		nv = getview(par[2]);
		if (nv == NOVIEW)
		{
			us_abortcommand(_("Unknown view type: %s"), par[2]);
			return;
		}

		if (!changecellview(np, nv))
		{
			ttyputverbose(M_("Cell %s is now %s"), par[1], describenodeproto(np));

			/* set current nodeproto to itself to redisplay its name */
			if (np == getcurcell())
				(void)setval((INTBIG)el_curlib, VLIBRARY, x_("curnodeproto"), (INTBIG)np, VNODEPROTO);

			/* update status display if necessary */
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
			{
				if (w->curnodeproto != np) continue;

				/* if this window is textual, change the header string */
				if ((w->state&WINDOWTYPE) == TEXTWINDOW)
				{
					ed = w->editor;
					if (ed != NOEDITOR)
					{
						(void)reallocstring(&ed->header, describenodeproto(np), us_tool->cluster);
						(*w->redisphandler)(w);
					}
				}
				us_setcellname(w);
			}
			if (us_curnodeproto != NONODEPROTO && us_curnodeproto == np)
			{
				us_curnodeproto = NONODEPROTO;
				us_setnodeproto(np);
			}
		}
		return;
	}

	if (namesamen(pp, x_("frame"), l) == 0 && l >= 1)
	{
		/* get cell */
		np = us_needcell();
		if (np == NONODEPROTO) return;

		if (count == 1)
		{
			/* report the frame in use here */
			var = getvalkey((INTBIG)np, VNODEPROTO, VSTRING, el_schematic_page_size_key);
			if (var == NOVARIABLE)
			{
				ttyputmsg(M_("This cell has no frame"));
				return;
			}
			pt = (CHAR *)var->addr;
			infstr = initinfstr();
			switch (*pt)
			{
				case 'x': case 'X': addtoinfstr(infstr, 'X');   break;
				case 'a': case 'A': addtoinfstr(infstr, 'A');   break;
				case 'b': case 'B': addtoinfstr(infstr, 'B');   break;
				case 'c': case 'C': addtoinfstr(infstr, 'C');   break;
				case 'd': case 'D': addtoinfstr(infstr, 'D');   break;
				case 'e': case 'E': addtoinfstr(infstr, 'E');   break;
			}
			pt++;
			while (*pt != 0)
			{
				if (*pt == 'v' || *pt == 'V') addstringtoinfstr(infstr, M_(", vertical"));
				if (*pt == 'n' || *pt == 'N') addstringtoinfstr(infstr, M_(", no title box"));
				pt++;
			}
			ttyputmsg(M_("This cell has a frame of size %s"), returninfstr(infstr));
			return;
		}

		/* set the frame size */
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("none"), l) == 0 && l >= 1)
		{
			if (getvalkey((INTBIG)np, VNODEPROTO, VSTRING, el_schematic_page_size_key) != NOVARIABLE)
				(void)delvalkey((INTBIG)np, VNODEPROTO, el_schematic_page_size_key);
			return;
		}
		if (namesamen(pp, x_("title-only"), l) == 0 && l >= 1)
		{
			estrcpy(line, x_("x"));
			(void)setvalkey((INTBIG)np, VNODEPROTO, el_schematic_page_size_key, (INTBIG)line, VSTRING);
			return;
		}
		line[0] = 0;
		if (namesamen(pp, x_("A"), l) == 0 && l >= 1) estrcat(line, x_("a")); else
		if (namesamen(pp, x_("B"), l) == 0 && l >= 1) estrcat(line, x_("b")); else
		if (namesamen(pp, x_("C"), l) == 0 && l >= 1) estrcat(line, x_("c")); else
		if (namesamen(pp, x_("D"), l) == 0 && l >= 1) estrcat(line, x_("d")); else
		if (namesamen(pp, x_("E"), l) == 0 && l >= 1) estrcat(line, x_("e")); else
		if (namesamen(pp, x_("Half-A"), l) == 0 && l >= 1) estrcat(line, x_("h")); else
		{
			ttyputbadusage(x_("view frame"));
			return;
		}
		for(i=2; i<count; i++)
		{
			if (namesamen(par[i], x_("vertical"), estrlen(par[i])) == 0)
				estrcat(line, x_("v"));
			if (namesamen(par[i], x_("no-title"), estrlen(par[i])) == 0)
				estrcat(line, x_("n"));
		}
		(void)setvalkey((INTBIG)np, VNODEPROTO, el_schematic_page_size_key, (INTBIG)line, VSTRING);
		return;
	}
	ttyputbadusage(x_("view"));
}

void us_visiblelayers(INTBIG count, CHAR *par[])
{
	CHAR line[100];
	REGISTER INTBIG i, j, val;
	REGISTER BOOLEAN defstate, noprint, addstate;
	REGISTER INTBIG fun;
	REGISTER CHAR *ch, *la;

	/* if the user wants every layer, do it */
	if (us_needwindow()) return;
	noprint = FALSE;
	if (count > 0)
	{
		if (count > 1 && namesame(par[1], x_("no-list")) == 0) noprint = TRUE;
		if (estrcmp(par[0], x_("*")) == 0)
		{
			/* save highlighting */
			us_pushhighlight();
			us_clearhighlightcount();
			startobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);

			for(i=0; i<el_curtech->layercount; i++)
			{
				if ((el_curtech->layers[i]->colstyle&INVISIBLE) == 0) continue;
				(void)setval((INTBIG)el_curtech->layers[i], VGRAPHICS, x_("style"),
					el_curtech->layers[i]->colstyle & ~INVISIBLE, VSHORT);
			}

			/* restore highlighting and redisplay */
			endobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);
			us_pophighlight(FALSE);
		} else
		{
			/* handle specific request for visible layers */
			defstate = TRUE;
			addstate = FALSE;
			if (par[0][0] == '*' && par[0][1] == '-' && par[0][2] != 0)
			{
				par[0] += 2;
				defstate = FALSE;
			} else if (par[0][0] == '-' && par[0][1] != 0)
			{
				par[0] += 1;
				defstate = FALSE;
				addstate = TRUE;
			} else if (par[0][0] == '+' && par[0][1] != 0)
			{
				par[0] += 1;
				addstate = TRUE;
			}

			/* collect all of the layer letters */
			(void)estrcpy(line, x_(""));
			for(i=0; i<el_curtech->layercount; i++)
				(void)estrcat(line, us_layerletters(el_curtech, i));

			/* now see if the user-requested layers are valid names */
			for(ch = par[0]; *ch != 0 && *ch != '('; ch++)
			{
				for(i=0; line[i] != 0; i++) if (*ch == line[i]) break;
				if (line[i] == 0)
				{
					us_abortcommand(_("No such layer: %c"), *ch);
					return;
				}
			}

			/* initially set all layers to default if not adding */
			if (!addstate)
			{
				for(i=0; i<el_curtech->layercount; i++)
				{
					if (!defstate)
						el_curtech->layers[i]->colstyle &= ~INVTEMP; else
							el_curtech->layers[i]->colstyle |= INVTEMP;
				}
			}

			/* set the changes */
			for(i=0; i<el_curtech->layercount; i++)
			{
				la = us_layerletters(el_curtech, i);
				for(j=0; la[j] != 0; j++)
					for(ch = par[0]; *ch != 0 && *ch != '('; ch++)
						if (*ch == la[j])
				{
					if (!defstate) el_curtech->layers[i]->colstyle |= INVTEMP; else
						el_curtech->layers[i]->colstyle &= ~INVTEMP;
				}
			}

			/* save highlighting */
			us_pushhighlight();
			us_clearhighlightcount();
			startobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);

			/* make the changes */
			for(i=0; i<el_curtech->layercount; i++)
			{
				val = el_curtech->layers[i]->colstyle;
				if ((val&(INVTEMP|INVISIBLE)) == INVTEMP)
				{
					(void)setval((INTBIG)el_curtech->layers[i], VGRAPHICS,
						x_("style"), val | INVISIBLE, VSHORT);
					continue;
				}
				if ((val&(INVTEMP|INVISIBLE)) == INVISIBLE)
				{
					(void)setval((INTBIG)el_curtech->layers[i], VGRAPHICS,
						x_("style"), val & ~INVISIBLE, VSHORT);
					continue;
				}
			}

			/* restore highlighting */
			endobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);
			us_pophighlight(FALSE);
		}
	}

	/* tell which layers are visible */
	if (noprint) return;
	j = 0;
	for(i=0; i<el_curtech->layercount; i++)
	{
		if ((el_curtech->layers[i]->colstyle&INVISIBLE) != 0) continue;
		if (j++ == 0) ttyputmsg(M_("Visible layers:"));
		ch = us_layerletters(el_curtech, i);
		fun = layerfunction(el_curtech, i);
		if (*ch == 0) ttyputmsg(M_("Layer %s"), layername(el_curtech, i)); else
			ttyputmsg(M_("Layer %s, letters %s%s"), layername(el_curtech, i), us_layerletters(el_curtech, i),
				(fun&(LFTRANS1|LFTRANS2|LFTRANS3|LFTRANS4|LFTRANS5)) != 0 ? M_(" (transparent)") : x_(""));
	}
	if (j == 0) ttyputmsg(M_("No visible layers"));
}

static BOOLEAN us_wpopcharhandler(INTSML chr, INTBIG special)
{
	return((*us_wpop->charhandler)(us_wpop, chr, special));
}

static BOOLEAN us_wpopbuttonhandler(INTBIG x, INTBIG y, INTBIG but)
{
	if (us_wpop->buttonhandler != 0) (*us_wpop->buttonhandler)(us_wpop, but, x, y);
	return(FALSE);
}
