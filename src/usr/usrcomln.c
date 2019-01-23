/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrcomln.c
 * User interface tool: command handler for L through N
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
#include "edialogs.h"
#include "conlay.h"
#include "tecart.h"
#include "tecgen.h"
#include "tecschem.h"
#if SIMTOOL
#  include "sim.h"
#endif

void us_lambda(INTBIG count, CHAR *par[])
{
	REGISTER CHAR *pp;
	REGISTER INTBIG lam, oldlam, newunit, how;
	REGISTER INTBIG l;
	REGISTER WINDOWPART *w;
	REGISTER NODEPROTO *np;
	REGISTER LIBRARY *lib;
	REGISTER TECHNOLOGY *tech;
	TECHNOLOGY *techarray[1];
	INTBIG newlam[1];
	extern COMCOMP us_lambdachp, us_noyesp;

	if (count > 0)
	{
		l = estrlen(pp = par[0]);

		/* handle display unit setting */
		if (namesamen(pp, x_("display-units"), l) == 0)
		{
			if (count >= 2)
			{
				l = estrlen(pp = par[1]);
				if (namesamen(pp, x_("lambda"), l) == 0) newunit = DISPUNITLAMBDA; else
				if (namesamen(pp, x_("inch"), l) == 0) newunit = DISPUNITINCH; else
				if (namesamen(pp, x_("centimeter"), l) == 0 && l >= 7) newunit = DISPUNITCM; else
				if (namesamen(pp, x_("millimeter"), l) == 0 && l >= 4) newunit = DISPUNITMM; else
				if (namesamen(pp, x_("mil"), l) == 0 && l >= 3) newunit = DISPUNITMIL; else
				if (namesamen(pp, x_("micron"), l) == 0 && l >= 3) newunit = DISPUNITMIC; else
				if (namesamen(pp, x_("centimicron"), l) == 0 && l >= 7) newunit = DISPUNITCMIC; else
				if (namesamen(pp, x_("nanometer"), l) == 0) newunit = DISPUNITNM; else
				{
					us_abortcommand(_("Unknown display unit: %s"), pp);
					return;
				}
				setvalkey((INTBIG)us_tool, VTOOL, us_displayunitskey, newunit, VINTEGER);
			}
			switch (el_units&DISPLAYUNITS)
			{
				case DISPUNITLAMBDA:
					ttyputverbose(M_("Distance expressed in lambda (currently %ld %ss)"),
						el_curlib->lambda[el_curtech->techindex], unitsname(el_units));
					break;
				case DISPUNITINCH:
					ttyputverbose(M_("Distance expressed in inches"));
					break;
				case DISPUNITCM:
					ttyputverbose(M_("Distance expressed in centimeters"));
					break;
				case DISPUNITMM:
					ttyputverbose(M_("Distance expressed in millimeters"));
					break;
				case DISPUNITMIL:
					ttyputverbose(M_("Distance expressed in mils"));
					break;
				case DISPUNITMIC:
					ttyputverbose(M_("Distance expressed in microns"));
					break;
				case DISPUNITCMIC:
					ttyputverbose(M_("Distance expressed in centimicrons"));
					break;
				case DISPUNITNM:
					ttyputverbose(M_("Distance expressed in nanometers"));
					break;
			}
			return;
		}

		/* handle internal unit setting */
		if (namesamen(pp, x_("internal-units"), l) == 0)
		{
			if (count >= 2)
			{
				l = estrlen(pp = par[1]);
				if (namesamen(pp, x_("half-decimicron"), l) == 0 && l >= 6)
				{
					changeinternalunits(NOLIBRARY, el_units, INTUNITHDMIC);
				} else if (namesamen(pp, x_("half-nanometer"), l) == 0 && l >= 6)
				{
					changeinternalunits(NOLIBRARY, el_units, INTUNITHNM);
				} else
				{
					us_abortcommand(_("Unknown internal unit: %s"), pp);
					return;
				}
			}
			ttyputverbose(M_("Smallest internal unit is %s"), unitsname(el_units));
			return;
		}

		if (namesamen(pp, x_("change-tech"), l) == 0 && l >= 8) how = 0; else
			if (namesamen(pp, x_("change-lib"), l) == 0 && l >= 8) how = 1; else
				if (namesamen(pp, x_("change-all-libs"), l) == 0 && l >= 8) how = 2; else
		{
			ttyputusage(x_("lambda change-tech|change-lib|change-all-libs VALUE"));
			return;
		}

		if (count <= 1)
		{
			count = ttygetparam(M_("Lambda value: "), &us_lambdachp, MAXPARS-1, &par[1]) + 1;
			if (count == 1)
			{
				us_abortedmsg();
				return;
			}
		}

		lam = eatoi(par[1]);
		if (lam <= 0)
		{
			us_abortcommand(_("Lambda value must be positive"));
			return;
		}
		if (lam / 4 * 4 != lam)
		{
			count = ttygetparam(_("Instability may result if lambda is not divisible by four.  Continue? [n]"),
				&us_noyesp, MAXPARS, par);
			if (count <= 0 || namesamen(par[0], x_("yes"), estrlen(par[0])) != 0)
			{
				ttyputverbose(M_("No change made"));
				return;
			}
		}

		/* save highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		oldlam = el_curlib->lambda[el_curtech->techindex];
		techarray[0] = el_curtech;
		newlam[0] = lam;
		changelambda(1, techarray, newlam, el_curlib, how);
		us_setlambda(NOWINDOWFRAME);
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if (w->curnodeproto == NONODEPROTO) continue;
			if (w->curnodeproto->tech != el_curtech) continue;
			if (how == 2 || (how == 1 && w->curnodeproto->lib == el_curlib))
			{
				w->screenlx = muldiv(w->screenlx, lam, oldlam);
				w->screenhx = muldiv(w->screenhx, lam, oldlam);
				w->screenly = muldiv(w->screenly, lam, oldlam);
				w->screenhy = muldiv(w->screenhy, lam, oldlam);
				computewindowscale(w);
			} else us_redisplay(w);
		}

		/* restore highlighting */
		us_pophighlight(FALSE);
	}

	/* determine which technologies to report */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology) tech->temp1 = 0;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->tech->temp1++;
	el_curtech->temp1++;
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		if (tech->temp1 == 0) continue;
		ttyputverbose(M_("Lambda size for %s is %s"), tech->techname,
			latoa(el_curlib->lambda[tech->techindex], 0));
	}
}

void us_library(INTBIG count, CHAR *par[])
{
	REGISTER LIBRARY *lib, *olib, *lastlib, *mergelib, *oldlib;
	REGISTER INTBIG i, l, makecurrent, multifile, found, total, stripopts,
		retval, arg, newstate;
	REGISTER CHAR *pp, *pt, *style, *libf, *oldlibfile, save;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER BOOLEAN first, doquiet;
	REGISTER VARIABLE *var;
	REGISTER EDITOR *ed;
	REGISTER WINDOWPART *w, *curw;
	INTBIG lx, hx, ly, hy;
	INTBIG dummy;
	CHAR *newpar[5], *libn, *extn;
	extern COMCOMP us_libraryp, us_yesnop;
	REGISTER void *infstr;

	if (count == 0)
	{
		count = ttygetparam(M_("Library option: "), &us_libraryp, MAXPARS, par);
		if (count == 0)
		{
			us_abortedmsg();
			return;
		}
	}
	l = estrlen(pp = par[0]);

	if (namesamen(pp, x_("default-path"), l) == 0 && l >= 9)
	{
		if (count < 2)
		{
			ttyputusage(x_("library default-path PATHNAME"));
			return;
		}
		setlibdir(par[1]);
		ttyputverbose(M_("Default library path is now '%s'"), el_libdir);
		return;
	}

	if (namesamen(pp, x_("touch"), l) == 0 && l >= 1)
	{
		/* mark all libraries as "changed" */
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		{
			if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
			lib->userbits |= LIBCHANGEDMAJOR | LIBCHANGEDMINOR;
		}
		ttyputmsg(_("All libraries now need to be saved"));
		return;
	}

	if (namesamen(pp, x_("purge"), l) == 0 && l >= 1)
	{
		/* delete all cells that are not the most recent and are unused */
		found = 1;
		total = 0;
		while (found != 0)
		{
			found = 0;
			for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				if (np->newestversion == np) continue;
				if (np->firstinst != NONODEINST) continue;
				ttyputmsg(_("Deleting cell %s"), describenodeproto(np));
				us_dokillcell(np);
				found++;
				total++;
				break;
			}
		}
		if (total == 0) ttyputmsg(_("No unused old cell versions to delete")); else
			ttyputmsg(_("Deleted %ld cells"), total);
		return;
	}

	if (namesamen(pp, x_("kill"), l) == 0 && l >= 1)
	{
		if (count <= 1) lib = el_curlib; else
		{
			lib = getlibrary(par[1]);
			if (lib == NOLIBRARY)
			{
				us_abortcommand(_("No library called %s"), par[1]);
				return;
			}
		}

		/* make sure that this isn't the last library */
		i = 0;
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		{
			if ((olib->userbits&HIDDENLIBRARY) != 0) continue;
			if (olib == lib) continue;
			i++;
		}
		if (i == 0)
		{
			us_abortcommand(_("Cannot kill last library"));
			return;
		}

		/* check for usage by other libraries */
		first = TRUE;
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			olib->temp1 = 0;
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
				ni->parent->lib->temp1 = 1;
		}
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		{
			if (olib == lib) continue;
			if (olib->temp1 == 0) continue;
			if (first)
			{
				first = FALSE;
				ttyputerr(_("Cannot delete library %s because:"), lib->libname);
			}
			infstr = initinfstr();
			formatinfstr(infstr, _("   These cells (in library %s) use it:"),
				olib->libname);
			for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				{
					if (ni->proto->primindex != 0) continue;
					if (ni->proto->lib == lib) break;
				}
				if (ni == NONODEINST) continue;
				formatinfstr(infstr, x_(" %s"), nldescribenodeproto(np));
			}
			ttyputmsg(x_("%s"), returninfstr(infstr));
		}
		if (!first) return;

		/* make sure it hasn't changed (unless "safe" option is given) */
		if (count <= 2 || namesamen(par[2], x_("safe"), estrlen(par[2])) != 0)
		{
			if (us_preventloss(lib, 1, TRUE)) return;
		}

		/* switch to another library if killing current one */
		if (lib == el_curlib)
		{
			us_switchtolibrary(el_curlib->nextlibrary);
			ttyputverbose(M_("Current library is now %s"), el_curlib->libname);
		} else ttyputverbose(M_("Library %s killed"), lib->libname);

		/* remove display of all cells in this library */
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if (w->curnodeproto == NONODEPROTO) continue;
			if (w->curnodeproto->lib == lib) us_clearwindow(w);
		}

		ttyputmsg(x_("Library %s closed"), lib->libname);

		/* kill the library */
		killlibrary(lib);

		/*
		 * the cell structure has changed
		 *
		 * this wouldn't be necessary if the "killlibrary()" routine did a
		 * broadcast.  Then it would be detected properly in "us_killobject"
		 */
		us_redoexplorerwindow();
		return;
	}

	if (namesamen(pp, x_("read"), l) == 0 && l >= 1)
	{
		if (count < 2)
		{
			ttyputusage(x_("library read FILENAME [options]"));
			return;
		}

		/* get style of input */
		libf = par[1];
		makecurrent = 0;
		style = x_("binary");
		extn = x_(".elib");
		mergelib = NOLIBRARY;
		doquiet = FALSE;
		while (count >= 3)
		{
			l = estrlen(pp = par[2]);
			if (namesamen(pp, x_("binary"), l) == 0 && l >= 1) { style = x_("binary"); extn = x_(".elib"); doquiet = TRUE; } else
			if (namesamen(pp, x_("cif"),    l) == 0 && l >= 1) { style = x_("cif");    extn = x_(".cif");  doquiet = TRUE; } else
			if (namesamen(pp, x_("def"),    l) == 0 && l >= 1) { style = x_("def");    extn = x_(".def");                  } else
			if (namesamen(pp, x_("dxf"),    l) == 0 && l >= 1) { style = x_("dxf");    extn = x_(".dxf");                  } else
			if (namesamen(pp, x_("edif"),   l) == 0 && l >= 1) { style = x_("edif");   extn = x_(".edif");                 } else
			if (namesamen(pp, x_("gds"),    l) == 0 && l >= 1) { style = x_("gds");    extn = x_(".gds");  doquiet = TRUE; } else
			if (namesamen(pp, x_("lef"),    l) == 0 && l >= 1) { style = x_("lef");    extn = x_(".lef");                  } else
			if (namesamen(pp, x_("sdf"),    l) == 0 && l >= 2) { style = x_("sdf");    extn = x_(".sdf");  doquiet = TRUE; } else
			if (namesamen(pp, x_("sue"),    l) == 0 && l >= 2) { style = x_("sue");    extn = x_(".sue");  doquiet = TRUE; } else
			if (namesamen(pp, x_("text"),   l) == 0 && l >= 1) { style = x_("text");   extn = x_(".txt");  doquiet = TRUE; } else
			if (namesamen(pp, x_("vhdl"),   l) == 0 && l >= 1) { style = x_("vhdl");   extn = x_(".vhdl"); doquiet = TRUE; } else
			if (namesamen(pp, x_("make-current"), l) == 0 && l >= 2) makecurrent++; else
			if (namesamen(pp, x_("merge"), l) == 0 && l >= 2)
			{
				if (count < 4)
				{
					ttyputusage(x_("library read FILENAME merge LIBRARYNAME"));
					return;
				}
				mergelib = getlibrary(par[3]);
				if (mergelib == NOLIBRARY)
				{
					us_abortcommand(_("Cannot find merge library %s"), par[3]);
					return;
				}
				count--;
				par++;
			} else
			{
				us_abortcommand(_("Read options: binary|cif|def|dxf|edif|gds|lef|sdf|sue|text|vhdl|make-current|(merge LIB)"));
				return;
			}
			count--;
			par++;
		}

		/* turn off highlighting */
		us_clearhighlightcount();

		/* see if multiple files were selected */
		multifile = 0;
		for(pt = libf; *pt != 0; pt++) if (*pt == NONFILECH) multifile++;

		/* loop through all files */
		retval = 0;
		for(i=0; ; i++)
		{
			/* get the next file into "libf" */
			for(pt = libf; *pt != 0 && *pt != NONFILECH; pt++) ;
			save = *pt;
			*pt = 0;

			/* get file name and remove extension from library name */
			(void)allocstring(&libn, skippath(libf), el_tempcluster);
			l = estrlen(libn) - estrlen(extn);
			if (namesame(&libn[l], extn) == 0)
			{
				libn[l] = 0;

				/* remove extension from library file if not binary */
				if (estrcmp(extn, x_(".elib")) != 0) libf[estrlen(libf) - estrlen(extn)] = 0;
			}

			oldlib = NOLIBRARY;
			if (multifile != 0)
			{
				/* place the new library file name onto the current library */
				lib = el_curlib;
				oldlibfile = lib->libfile;
				lib->libfile = libf;
			} else if (mergelib != NOLIBRARY)
			{
				/* place the new library file name onto the merge library */
				lib = mergelib;
				oldlibfile = lib->libfile;
				lib->libfile = libf;
			} else
			{
				/* reading a new library (or replacing an old one) */
				oldlib = getlibrary(libn);
				if (oldlib != NOLIBRARY)
				{
					/* not a new library: make sure the old library is saved */
					if (us_preventloss(oldlib, 2, TRUE) != 0)
					{
						efree(libn);
						return;
					}

					/* replacing library: clear windows */
					for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
					{
						if (w->curnodeproto != NONODEPROTO &&
							w->curnodeproto->lib == oldlib)
						{
							w->curnodeproto = NONODEPROTO;
							us_redisplaynow(w, TRUE);
						}
					}

					/* rename this library, delete it later */
					oldlib->libname[0] = 0;
				}

				/* create the library */
				lib = newlibrary(libn, libf);
				if (lib == NOLIBRARY)
				{
					us_abortcommand(_("Cannot create library %s"), libn);
					efree(libn);
					return;
				}
			}

			/* read the library */
			if (multifile == 0) arg = 0; else
			{
				arg = 2;
				if (i == 0) arg = 1; else
					if (i == multifile) arg = 3;
			}
			if (mergelib == NOLIBRARY && doquiet) changesquiet(TRUE);
			if (asktool(io_tool, x_("read"), (INTBIG)lib, (INTBIG)style, arg) != 0)
				retval = 1;
			if (mergelib == NOLIBRARY && doquiet) changesquiet(FALSE);
			if (multifile != 0 || mergelib != NOLIBRARY) lib->libfile = oldlibfile;

			/* delete the former library if this replaced it */
			if (oldlib != NOLIBRARY)
			{
				us_replacelibraryreferences(oldlib, lib);
				if (oldlib == el_curlib) selectlibrary(lib, TRUE);
				killlibrary(oldlib);
			}

			/* advance to the next file in the list */
			*pt = (CHAR)save;
			if (save == 0) break;
			libf = pt + 1;
			if (retval != 0) break;
		}

		if (retval == 0)
		{
			if (makecurrent != 0 || lib == el_curlib)
			{
#if SIMTOOL
				sim_window_stopsimulation();
#endif

				selectlibrary(lib, TRUE);
				us_setlambda(NOWINDOWFRAME);
				if (us_curnodeproto == NONODEPROTO || us_curnodeproto->primindex == 0)
				{
					nextchangequiet();
					(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_node_key,
						(INTBIG)el_curtech->firstnodeproto, VNODEPROTO|VDONTSAVE);
					us_shadownodeproto(NOWINDOWFRAME, el_curtech->firstnodeproto);
				}
				np = el_curlib->curnodeproto;
				if (el_curwindowpart != NOWINDOWPART && 
					el_curwindowpart->curnodeproto == NONODEPROTO &&
						(el_curwindowpart->state&WINDOWTYPE) != EXPLORERWINDOW)
							curw = el_curwindowpart; else
				{
					for(curw = el_topwindowpart; curw != NOWINDOWPART; curw = curw->nextwindowpart)
						if (curw->curnodeproto == NONODEPROTO &&
							(curw->state&WINDOWTYPE) != EXPLORERWINDOW) break;
				}
				if (np != NONODEPROTO)
				{
					if (curw == NOWINDOWPART)
					{
						curw = us_wantnewwindow(0);
						if (curw == NOWINDOWPART)
						{
							us_abortcommand(_("Cannot create new window"));
							return;
						}
					} else
					{
						if (curw->termhandler != 0)
							(*curw->termhandler)(curw);
						if ((curw->state&WINDOWTYPE) == POPTEXTWINDOW)
							screenrestorebox(curw->editor->savedbox, -1);
						if ((curw->state&WINDOWTYPE) == TEXTWINDOW ||
							(curw->state&WINDOWTYPE) == POPTEXTWINDOW)
								us_freeeditor(curw->editor);
					}
					if ((np->cellview->viewstate&TEXTVIEW) != 0)
					{
						/* text window: make an editor */
						if (us_makeeditor(curw, describenodeproto(np), &dummy, &dummy) == NOWINDOWPART)
						{
							efree(libn);
							return;
						}

						/* get the text that belongs here */
						var = getvalkey((INTBIG)np, VNODEPROTO, VSTRING|VISARRAY, el_cell_message_key);
						if (var == NOVARIABLE)
						{
							newpar[0] = x_("");
							var = setvalkey((INTBIG)np, VNODEPROTO, el_cell_message_key,
								(INTBIG)newpar, VSTRING|VISARRAY|(1<<VLENGTHSH));
							if (var == NOVARIABLE)
							{
								efree(libn);
								return;
							}
						}

						/* setup for editing */
						curw->curnodeproto = np;
						ed = curw->editor;
						ed->editobjaddr = (CHAR *)np;
						ed->editobjtype = VNODEPROTO;
						ed->editobjqual = x_("FACET_message");
						ed->editobjvar = var;

						/* load the text into the window */
						us_suspendgraphics(curw);
						l = getlength(var);
						for(i=0; i<l; i++)
						{
							pp = ((CHAR **)var->addr)[i];
							if (i == l-1 && *pp == 0) continue;
							us_addline(curw, i, pp);
						}
						us_resumegraphics(curw);
						curw->changehandler = us_textcellchanges;
						us_setcellname(curw);
					} else
					{
						/* adjust the new window to account for borders in the old one */
						if ((curw->state&WINDOWTYPE) != DISPWINDOW)
						{
							curw->usehx -= DISPLAYSLIDERSIZE;
							curw->usely += DISPLAYSLIDERSIZE;
						}
						if ((curw->state&WINDOWTYPE) == WAVEFORMWINDOW)
						{
							curw->uselx -= DISPLAYSLIDERSIZE;
							curw->usely -= DISPLAYSLIDERSIZE;
						}
						if ((curw->state&WINDOWMODE) != 0)
						{
							curw->uselx -= WINDOWMODEBORDERSIZE;
							curw->usehx += WINDOWMODEBORDERSIZE;
							curw->usely -= WINDOWMODEBORDERSIZE;
							curw->usehy += WINDOWMODEBORDERSIZE;
						}
						curw->curnodeproto = np;
						newstate = (curw->state & ~(WINDOWTYPE|WINDOWMODE)) | DISPWINDOW;
						if ((np->userbits&TECEDITCELL) != 0) newstate |= WINDOWTECEDMODE;
						us_setwindowmode(curw, curw->state, newstate);
						curw->editor = NOEDITOR;
						curw->buttonhandler = DEFAULTBUTTONHANDLER;
						curw->charhandler = DEFAULTCHARHANDLER;
						curw->changehandler = DEFAULTCHANGEHANDLER;
						curw->termhandler = DEFAULTTERMHANDLER;
						curw->redisphandler = DEFAULTREDISPHANDLER;
						us_fullview(np, &lx, &hx, &ly, &hy);
						us_squarescreen(curw, NOWINDOWPART, FALSE, &lx, &hx, &ly, &hy, 0);
						curw->screenlx = lx;
						curw->screenhx = hx;
						curw->screenly = ly;
						curw->screenhy = hy;
						computewindowscale(curw);
						us_setcellname(curw);
						us_setcellsize(curw);
						us_setgridsize(curw);
						if (curw->redisphandler != 0)
							(*curw->redisphandler)(curw);
						us_state |= CURCELLCHANGED;
					}
				}

				/* redisplay the explorer window */
				us_redoexplorerwindow();
			}
		} else if (mergelib == NOLIBRARY)
		{
			/* library is incomplete: remove it from the list */
			if (el_curlib != lib)
			{
				/* this is not the current library: safe to delete */
				lastlib = NOLIBRARY;
				for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
				{
					if (olib == lib) break;
					lastlib = olib;
				}
				if (lastlib != NOLIBRARY) lastlib->nextlibrary = lib->nextlibrary;
			} else
			{
				/* current library is incomplete */
				if (el_curlib->nextlibrary == NOLIBRARY)
				{
					/* must create a new library: it is the only one */
					el_curlib = alloclibrary();
					(void)allocstring(&el_curlib->libname, lib->libname, el_curlib->cluster);
					(void)allocstring(&el_curlib->libfile, lib->libfile, el_curlib->cluster);
					el_curlib->nextlibrary = NOLIBRARY;
				} else el_curlib = el_curlib->nextlibrary;

				us_clearhighlightcount();
				us_setlambda(NOWINDOWFRAME);
				if (us_curnodeproto == NONODEPROTO || us_curnodeproto->primindex == 0)
				{
					if ((us_state&NONPERSISTENTCURNODE) != 0) us_setnodeproto(NONODEPROTO); else
						us_setnodeproto(el_curtech->firstnodeproto);
				}
				if (el_curlib->curnodeproto != NONODEPROTO)
				{
					newpar[0] = describenodeproto(el_curlib->curnodeproto);
					us_editcell(1, newpar);
				}
			}
		}
		us_endbatch();
		efree(libn);
		noundoallowed();
		return;
	}

	if (namesamen(pp, x_("use"), l) == 0 && l >= 1)
	{
		if (count <= 1)
		{
			ttyputusage(x_("library use LIBNAME"));
			return;
		}

		/* find actual library name */
		libn = skippath(par[1]);

		lib = getlibrary(libn);
		if (lib != NOLIBRARY)
		{
			if (el_curlib == lib) return;
			us_switchtolibrary(lib);
			return;
		}

		/* create new library */
		lib = newlibrary(libn, par[1]);
		if (lib == NOLIBRARY)
		{
			us_abortcommand(_("Cannot create library %s"), libn);
			return;
		}

		/* make this library the current one */
		us_switchtolibrary(lib);
		ttyputverbose(M_("New library: %s"), libn);
		return;
	}

	if (namesamen(pp, x_("new"), l) == 0 && l >= 1)
	{
		if (count <= 1)
		{
			ttyputusage(x_("library new LIBNAME"));
			return;
		}

		/* find actual library name */
		libn = skippath(par[1]);

		lib = getlibrary(libn);
		if (lib != NOLIBRARY)
		{
			us_abortcommand(_("Already a library called '%s'"), libn);
			return;
		}

		/* create new library */
		lib = newlibrary(libn, par[1]);
		if (lib == NOLIBRARY)
		{
			us_abortcommand(_("Cannot create library %s"), libn);
			return;
		}

		ttyputverbose(M_("New library: %s"), libn);
		return;
	}

	if (namesamen(pp, x_("save"), l) == 0 && l >= 2)
	{
		if (us_saveeverything())
		{
			/* manage reset of session log (if all information is saved) */
			us_continuesessionlogging();
		}
		return;
	}

	if (namesamen(pp, x_("write"), l) == 0 && l >= 1)
	{
		if (count < 2)
		{
			ttyputusage(x_("library write LIBNAME [options]"));
			return;
		}

		/* get style of output */
		style = x_("binary");
		if (count >= 3)
		{
			l = estrlen(pp = par[2]);
			if (l >= 1 && namesamen(pp, x_("binary"), l) == 0) style = x_("binary"); else
			if (l >= 1 && namesamen(pp, x_("cif"), l) == 0) style = x_("cif"); else
			if (l >= 1 && namesamen(pp, x_("dxf"), l) == 0) style = x_("dxf"); else
			if (l >= 2 && namesamen(pp, x_("eagle"), l) == 0) style = x_("eagle"); else
			if (l >= 2 && namesamen(pp, x_("ecad"), l) == 0) style = x_("ecad"); else
			if (l >= 2 && namesamen(pp, x_("edif"), l) == 0) style = x_("edif"); else
			if (l >= 1 && namesamen(pp, x_("gds"), l) == 0) style = x_("gds"); else
			if (l >= 1 && namesamen(pp, x_("hpgl"), l) == 0) style = x_("hpgl"); else
			if (l >= 2 && namesamen(pp, x_("lef"), l) == 0) style = x_("lef"); else
			if (l >= 1 && namesamen(pp, x_("l"), l) == 0) style = x_("l"); else
			if (l >= 2 && namesamen(pp, x_("pads"), l) == 0) style = x_("pads"); else
			if (l >= 2 && namesamen(pp, x_("postscript"), l) == 0) style = x_("postscript"); else
			if (l >= 2 && namesamen(pp, x_("printed-postscript"), l) == 0) style = x_("printed-postscript"); else
			if (l >= 1 && namesamen(pp, x_("quickdraw"), l) == 0) style = x_("quickdraw"); else
			if (l >= 1 && namesamen(pp, x_("skill"), l) == 0) style = x_("skill"); else
			if (l >= 1 && namesamen(pp, x_("text"), l) == 0) style = x_("text"); else
			{
				us_abortcommand(_("File formats: binary|cif|dxf|eagle|ecad|edif|gds|hpgl|l|lef|pads|postscript|printed-postscript|quickdraw|skill|text"));
				return;
			}
		}

		/* get library to write */
		lib = getlibrary(par[1]);
		if (lib == NOLIBRARY)
		{
			us_abortcommand(_("No library called %s"), par[1]);
			return;
		}

		/* do it */
		if (namesame(style, x_("binary")) == 0 || namesame(style, x_("text")) == 0) stripopts = 1; else
			stripopts = 0;
		if (stripopts != 0) makeoptionstemporary(lib);
		retval = asktool(io_tool, x_("write"), (INTBIG)lib, (INTBIG)style);
		if (stripopts != 0) restoreoptionstate(lib);
		if (retval != 0) return;

		/* offer to save any referenced libraries */
		if (namesame(style, x_("binary")) == 0)
		{
			for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
				for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
					np->temp2 = 0;
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
					if (ni->proto->primindex == 0)
						ni->proto->temp2 = 1;
			for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			{
				if (olib == lib) continue;
				for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
					if (np->temp2 != 0) break;
				if (np == NONODEPROTO) continue;
				if ((olib->userbits&(LIBCHANGEDMAJOR | LIBCHANGEDMINOR)) == 0) continue;

				infstr = initinfstr();
				if ((olib->userbits&LIBCHANGEDMAJOR) == 0)
				{
					formatinfstr(infstr, _("Also save referenced library %s (which has changed insignificantly)?"),
						olib->libname);
				} else
				{
					formatinfstr(infstr, _("Also save referenced library %s (which has changed significantly)?"),
						olib->libname);
				}
				i = ttygetparam(returninfstr(infstr), &us_yesnop, 3, newpar);
				if (i == 1 && newpar[0][0] == 'n') continue;
				makeoptionstemporary(olib);
				(void)asktool(io_tool, x_("write"), (INTBIG)olib, (INTBIG)style);
				restoreoptionstate(olib);
			}
		}

		/* rewind session recording if writing in permanent format */
		if (*style == 'b' || *style == 't')
		{
			/* manage reset of session log (if all information is saved) */
			us_continuesessionlogging();
		}
		return;
	}

	ttyputbadusage(x_("library"));
}

void us_macbegin(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG i, l;
	REGISTER BOOLEAN verbose, execute;
	REGISTER INTBIG key;
	REGISTER CHAR *ch, *pp;
	CHAR *newmacro[3];
	extern COMCOMP us_macbeginnp;
	REGISTER VARIABLE *var;
	REGISTER void *infstr;

	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING, us_macrobuildingkey);
	if (var != NOVARIABLE)
	{
		us_abortcommand(_("Already defining a macro"));
		return;
	}

	/* get macro name */
	if (count == 0)
	{
		count = ttygetparam(M_("Macro name: "), &us_macbeginnp, MAXPARS, par);
		if (count == 0)
		{
			us_abortedmsg();
			return;
		}
	}
	pp = par[0];

	/* get switches */
	verbose = FALSE;   execute = TRUE;
	for(i=1; i<count; i++)
	{
		ch = par[i];
		l = estrlen(ch);
		if (namesamen(ch, x_("verbose"), l) == 0) verbose = TRUE; else
		if (namesamen(ch, x_("no-execute"), l) == 0) execute = FALSE; else
		{
			ttyputusage(x_("macbegin MACRONAME [verbose|no-execute]"));
			return;
		}
	}

	/* make sure macro name isn't overloading existing command */
	for(i=0; us_lcommand[i].name != 0; i++)
		if (namesame(pp, us_lcommand[i].name) == 0)
	{
		us_abortcommand(_("There is a command with that name"));
		return;
	}

	/* see if macro name already exists */
	if (us_getmacro(pp) == NOVARIABLE)
	{
		/* new macro name, check for validity */
		for(ch = pp; *ch != 0; ch++) if (*ch == ' ' || *ch == '\t')
		{
			us_abortcommand(_("Macro name must not have embedded spaces"));
			return;
		}
	}

	/* create the macro variable */
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("USER_macro_"));
	addstringtoinfstr(infstr, pp);
	key = makekey(returninfstr(infstr));

	/* build the first two lines of the macro */
	infstr = initinfstr();
	if (us_curmacropack != NOMACROPACK)
	{
		addstringtoinfstr(infstr, x_("macropack="));
		addstringtoinfstr(infstr, us_curmacropack->packname);
	}
	if (verbose) addstringtoinfstr(infstr, x_(" verbose"));
	if (!execute) addstringtoinfstr(infstr, x_(" noexecute"));
	newmacro[0] = returninfstr(infstr);
	newmacro[1] = x_("");		/* no parameters yet */
	newmacro[2] = x_("");		/* no parsing structures yet */
	(void)setvalkey((INTBIG)us_tool, VTOOL, key, (INTBIG)newmacro,
		VSTRING|VISARRAY|(3<<VLENGTHSH)|VDONTSAVE);

	/* turn on macro remembering */
	nextchangequiet();
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_macrobuildingkey, (INTBIG)pp, VSTRING|VDONTSAVE);
	nextchangequiet();
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_macrorunningkey, (INTBIG)pp, VSTRING|VDONTSAVE);

	ttyputmsg(_("Remembering %s..."), pp);
}

void us_macdone(INTBIG count, CHAR *par[])
{
	REGISTER VARIABLE *var;

	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING, us_macrorunningkey);
	if (var == NOVARIABLE)
	{
		us_abortcommand(_("Can only stop execution of a macro if one is running"));
		return;
	}
	us_state |= MACROTERMINATED;
}

void us_macend(INTBIG count, CHAR *par[])
{
	REGISTER VARIABLE *var;

	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING, us_macrobuildingkey);
	if (var == NOVARIABLE)
	{
		us_abortcommand(_("Not in a macro"));
		return;
	}
	ttyputmsg(_("%s defined"), (CHAR *)var->addr);
	nextchangequiet();
	(void)delvalkey((INTBIG)us_tool, VTOOL, us_macrobuildingkey);
	if (getvalkey((INTBIG)us_tool, VTOOL, VSTRING, us_macrorunningkey) != NOVARIABLE)
	{
		nextchangequiet();
		(void)delvalkey((INTBIG)us_tool, VTOOL, us_macrorunningkey);
	}
}

void us_menu(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG l, s, i, total, changed, position, x, y, varkey,
		j, len, arctot, pintot, comptot, puretot, fun, coms;
	REGISTER BOOLEAN autoset, verbose;
	BOOLEAN butstate;
	REGISTER CHAR *pp, **temp;
	CHAR number[10], *pt, *implicit[6];
	POPUPMENU *pm, *cpm, *newpm;
	REGISTER POPUPMENUITEM *mi, *miret;
	NODEPROTO *np;
	ARCPROTO *ap;
	REGISTER USERCOM *uc;
	REGISTER VARIABLE *var;
	COMCOMP *comarray[MAXPARS];
	COMMANDBINDING commandbinding;
	extern COMCOMP us_menup;
	REGISTER void *infstr;

	/* with no arguments, toggle the menu state */
	if (count == 0)
	{
		count = ttygetparam(_("Menu display option: "), &us_menup, MAXPARS, par);
		if (count == 0)
		{
			us_abortedmsg();
			return;
		}
	}

	/* check for simple changes of the menu display */
	l = estrlen(pp = par[0]);
	if (namesamen(pp, x_("on"), l) == 0 && l >= 2)
	{
		if ((us_tool->toolstate&MENUON) != 0)
		{
			us_abortcommand(_("Menu is already displayed"));
			return;
		}

		/* save highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		startobjectchange((INTBIG)us_tool, VTOOL);
		(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate|MENUON,
			VINTEGER);
		endobjectchange((INTBIG)us_tool, VTOOL);

		/* restore highlighting */
		us_pophighlight(FALSE);
		return;
	}

	if (namesamen(pp, x_("off"), l) == 0 && l >= 2)
	{
		if ((us_tool->toolstate&MENUON) == 0)
		{
			us_abortcommand(_("Menu is not displayed"));
			return;
		}

		/* save highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		startobjectchange((INTBIG)us_tool, VTOOL);
		(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate & ~MENUON, VINTEGER);
		endobjectchange((INTBIG)us_tool, VTOOL);

		/* restore highlighting */
		us_pophighlight(FALSE);
		return;
	}

	if (namesamen(pp, x_("dopopup"), l) == 0 && l >= 1)
	{
		if (count <= 1)
		{
			implicit[0] = x_("telltool");
			implicit[1] = x_("user");
			uc = us_buildcommand(2, implicit);
		} else uc = us_buildcommand(count-1, &par[1]);
		if (uc == NOUSERCOM)
		{
			us_abortcommand(_("Command '%s' not found"), par[1]);
			return;
		}

		/* loop through the popup menus of this command */
		for(;;)
		{
			/* convert to a string of command completion objects */
			total = us_fillcomcomp(uc, comarray);
			if (total <= uc->count) break;

			/* count the number of options */
			us_pathiskey = comarray[uc->count]->ifmatch;
			pt = x_("");
			(void)(*comarray[uc->count]->toplist)(&pt);
			for(coms=0; (pp = (*comarray[uc->count]->nextcomcomp)()); coms++) ;
			if ((comarray[uc->count]->interpret&INCLUDENOISE) != 0) coms++;

			/* build the prompt */
			infstr = initinfstr();
			addstringtoinfstr(infstr, comarray[uc->count]->noise);

			/* if there are no choices, prompt directly */
			if (coms == 0)
			{
				addstringtoinfstr(infstr, x_(": "));
				pp = ttygetline(returninfstr(infstr));
				if (pp == 0) break;
				(void)allocstring(&uc->word[uc->count], pp, us_tool->cluster);
				uc->count++;
				continue;
			}

			/* create the popup menu */
			pm = (POPUPMENU *)emalloc(sizeof(POPUPMENU), el_tempcluster);
			if (pm == 0) return;
			pm->name = x_("noname");
			(void)allocstring(&pm->header, returninfstr(infstr), el_tempcluster);
			pm->total = coms;
			mi = (POPUPMENUITEM *)emalloc((coms * (sizeof (POPUPMENUITEM))), el_tempcluster);
			if (mi == 0) return;
			pm->list = mi;

			/* fill the popup menu */
			pt = x_("");
			(void)(*comarray[uc->count]->toplist)(&pt);
			for(i=0; i<coms; i++)
			{
				mi[i].valueparse = NOCOMCOMP;
				mi[i].response = NOUSERCOM;

				/* if noise is includable, do it first */
				if (i == 0 && (comarray[uc->count]->interpret&INCLUDENOISE) != 0)
				{
					mi[i].attribute = comarray[uc->count]->noise;
					if (comarray[uc->count]->def != 0)
					{
						mi[i].maxlen = maxi(20, estrlen(comarray[uc->count]->def));
						mi[i].value = (CHAR *)emalloc((mi[i].maxlen+1) * SIZEOFCHAR, el_tempcluster);
						(void)estrcpy(mi[i].value, comarray[uc->count]->def);
					} else
					{
						mi[i].maxlen = 20;
						mi[i].value = (CHAR *)emalloc((mi[i].maxlen+1) * SIZEOFCHAR, el_tempcluster);
						(void)estrcpy(mi[i].value, x_("*"));
					}
					continue;
				}

				/* normal entry */
				mi[i].attribute = (*comarray[uc->count]->nextcomcomp)();
				mi[i].value = 0;
				mi[i].maxlen = -1;

				/* see what would come next with this keyword */
				(void)allocstring(&uc->word[uc->count], mi[i].attribute, el_tempcluster);
				uc->count++;
				total = us_fillcomcomp(uc, comarray);
				uc->count--;
				efree(uc->word[uc->count]);
				if (total > uc->count+1)
				{
					if ((comarray[uc->count+1]->interpret&INPUTOPT) != 0)
					{
						mi[i].valueparse = comarray[uc->count+1];
						if (comarray[uc->count+1]->def != 0)
						{
							mi[i].maxlen = maxi(20, estrlen(comarray[uc->count+1]->def));
							mi[i].value = (CHAR *)emalloc((mi[i].maxlen+1) * SIZEOFCHAR, el_tempcluster);
							(void)estrcpy(mi[i].value, comarray[uc->count+1]->def);
						} else
						{
							mi[i].maxlen = 20;
							mi[i].value = (CHAR *)emalloc((mi[i].maxlen+1) * SIZEOFCHAR, el_tempcluster);
							(void)estrcpy(mi[i].value, x_("*"));
						}
					}
				}
			}

			/* invoke the popup menu */
			butstate = TRUE;
			cpm = pm;
			miret = us_popupmenu(&cpm, &butstate, TRUE, -1, -1, 0);
			if (miret == 0) us_abortcommand(_("Sorry, this display cannot support popup menus"));
			if (miret == NOPOPUPMENUITEM || miret == 0)
			{
				efree((CHAR *)mi);
				efree((CHAR *)pm->header);
				efree((CHAR *)pm);
				break;
			}

			/* see if multiple answers were given */
			changed = 0;
			for(i=0; i<pm->total; i++) if (mi[i].changed) changed++;
			if (changed > 1 || (changed == 1 && miret->changed == 0))
			{
				if ((comarray[uc->count]->interpret&MULTIOPT) == 0)
				{
					us_abortcommand(_("Multiple commands cannot be selected"));
					return;
				}

				/* tack on all of the answers */
				for(i=0; i<pm->total; i++) if (mi[i].changed)
				{
					if (uc->count >= MAXPARS-1) break;
					(void)allocstring(&uc->word[uc->count++], mi[i].attribute, us_tool->cluster);
					(void)allocstring(&uc->word[uc->count++], mi[i].value, us_tool->cluster);
				}
				if (!miret->changed && uc->count < MAXPARS)
					(void)allocstring(&uc->word[uc->count++], miret->attribute, us_tool->cluster);

				for(i=0; i<pm->total; i++)
					if (mi[i].maxlen >= 0) efree(mi[i].value);
				efree((CHAR *)mi);
				efree((CHAR *)pm);
				break;
			}

			/* if noise was mentioned, copy the value */
			if (miret == mi && miret->changed &&
				(comarray[uc->count]->interpret&INCLUDENOISE) != 0)
			{
				(void)allocstring(&uc->word[uc->count++], miret->value, us_tool->cluster);
			} else
			{
				(void)allocstring(&uc->word[uc->count++], miret->attribute, us_tool->cluster);
				if (miret->changed)
					(void)allocstring(&uc->word[uc->count++], miret->value, us_tool->cluster);
			}
			for(i=0; i<pm->total; i++)
				if (mi[i].maxlen >= 0) efree(mi[i].value);
			efree((CHAR *)mi);
			efree((CHAR *)pm);
		}

		/* issue the completed command */
		if ((us_tool->toolstate&ECHOBIND) != 0) verbose = TRUE; else
			verbose = FALSE;
		us_execute(uc, verbose, FALSE, TRUE);
		us_freeusercom(uc);
		return;
	}

	if (namesamen(pp, x_("popup"), l) == 0 && l >= 1)
	{
		if (count <= 2)
		{
			ttyputusage(x_("menu popup NAME OPTIONS"));
			return;
		}

		/* check out the name of the pop-up menu */
		pm = us_getpopupmenu(par[1]);

		/* determine what to do with the menu */
		l = estrlen(pp = par[2]);
		if (namesamen(pp, x_("header"), l) == 0 && l >= 1)
		{
			/* set header: ensure it is specified */
			if (count <= 3)
			{
				ttyputusage(x_("menu popup header MESSAGE"));
				return;
			}
			if (pm == NOPOPUPMENU)
			{
				us_abortcommand(_("No popup menu called %s"), par[1]);
				return;
			}
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("USER_binding_popup_"));
			addstringtoinfstr(infstr, par[1]);
			varkey = makekey(returninfstr(infstr));
			var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, varkey);
			if (var == NOVARIABLE)
			{
				us_abortcommand(_("Cannot find the menu"));
				return;
			}
			(void)setindkey((INTBIG)us_tool, VTOOL, varkey, 0, (INTBIG)par[3]);
			return;
		}
		if (namesamen(pp, x_("size"), l) == 0 && l >= 1)
		{
			/* set size: ensure it is specified */
			if (count <= 3)
			{
				ttyputusage(x_("menu popup NAME size SIZE"));
				return;
			}
			s = myatoi(par[3]);
			if (s <= 0)
			{
				us_abortcommand(_("Popup menu size must be positive"));
				return;
			}

			/* set size of menu */
			if (pm == NOPOPUPMENU)
			{
				/* create the variable with the popup menu */
				temp = (CHAR **)emalloc(((s+1) * (sizeof (CHAR *))), el_tempcluster);
				(void)allocstring(&temp[0], x_("MENU!"), el_tempcluster);
				for(i=0; i<s; i++)
				{
					infstr = initinfstr();
					addstringtoinfstr(infstr, x_("inputpopup="));
					addstringtoinfstr(infstr, par[1]);
					addstringtoinfstr(infstr, x_(" command=bind set popup "));
					addstringtoinfstr(infstr, par[1]);
					(void)esnprintf(number, 10, x_(" %ld"), i);
					addstringtoinfstr(infstr, number);
					(void)allocstring(&temp[i+1], returninfstr(infstr), el_tempcluster);
				}
				infstr = initinfstr();
				addstringtoinfstr(infstr, x_("USER_binding_popup_"));
				addstringtoinfstr(infstr, par[1]);
				varkey = makekey(returninfstr(infstr));
				(void)setvalkey((INTBIG)us_tool, VTOOL, varkey, (INTBIG)temp,
					VSTRING|VISARRAY|VDONTSAVE|((s+1)<<VLENGTHSH));
				for(i=0; i<=s; i++) efree(temp[i]);
				efree((CHAR *)temp);
			} else
			{
				/* get the former popup menu */
				infstr = initinfstr();
				addstringtoinfstr(infstr, x_("USER_binding_popup_"));
				addstringtoinfstr(infstr, par[1]);
				varkey = makekey(returninfstr(infstr));
				var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, varkey);
				if (var == NOVARIABLE)
				{
					us_abortcommand(_("Cannot find popup menu %s"), par[1]);
					return;
				}
				len = getlength(var);

				/* create the new popup menu */
				temp = (CHAR **)emalloc(((s+1) * (sizeof (CHAR *))), el_tempcluster);
				if (temp == 0) return;
				(void)allocstring(&temp[0], ((CHAR **)var->addr)[0], el_tempcluster);

				/* copy the existing menu entries */
				for(i=1; i<mini((s+1), len); i++)
				{
					us_parsebinding(((CHAR **)var->addr)[i], &commandbinding);
					infstr = initinfstr();
					if (commandbinding.inputpopup) addstringtoinfstr(infstr, x_("input"));
					addstringtoinfstr(infstr, x_("popup="));
					addstringtoinfstr(infstr, par[1]);
					if (commandbinding.menumessage != 0)
					{
						addstringtoinfstr(infstr, x_(" message=\""));
						addstringtoinfstr(infstr, commandbinding.menumessage);
						addtoinfstr(infstr, '"');
					}
					addstringtoinfstr(infstr, x_(" command="));
					addstringtoinfstr(infstr, commandbinding.command);
					(void)allocstring(&temp[i], returninfstr(infstr), el_tempcluster);
					us_freebindingparse(&commandbinding);
				}

				for(j=i; j<s+1; j++)
				{
					infstr = initinfstr();
					addstringtoinfstr(infstr, x_("inputpopup="));
					addstringtoinfstr(infstr, par[1]);
					addstringtoinfstr(infstr, x_(" command=bind set popup "));
					addstringtoinfstr(infstr, par[1]);
					(void)esnprintf(number, 10, x_(" %ld"), j);
					addstringtoinfstr(infstr, number);
					(void)allocstring(&temp[j], returninfstr(infstr), el_tempcluster);
				}
				(void)setvalkey((INTBIG)us_tool, VTOOL, varkey, (INTBIG)temp,
					VSTRING|VISARRAY|VDONTSAVE|((s+1)<<VLENGTHSH));
				for(i=0; i<=s; i++) efree(temp[i]);
				efree((CHAR *)temp);
				newpm = us_getpopupmenu(par[1]);
				for(i=0; i<us_pulldownmenucount; i++)
					us_recursivelyreplacemenu(us_pulldowns[i], pm, newpm);
		}
			return;
		}
		ttyputbadusage(x_("menu popup"));
		return;
	}

	if (namesamen(pp, x_("setmenubar"), l) == 0 && l >= 2)
	{
		/* free former menubar information */
		if (us_pulldownmenucount != 0)
		{
			efree((CHAR *)us_pulldowns);
			efree((CHAR *)us_pulldownmenupos);
		}
		us_pulldownmenucount = 0;

		/* allocate new menubar information */
		if (count > 1)
		{
			us_pulldowns = (POPUPMENU **)emalloc((count-1) * (sizeof (POPUPMENU *)), us_tool->cluster);
			us_pulldownmenupos = (INTBIG *)emalloc((count-1) * SIZEOFINTBIG, us_tool->cluster);
			if (us_pulldowns == 0 || us_pulldownmenupos == 0) return;
			us_pulldownmenucount = count-1;
		}

		/* load the menubar information */
		for(i=1; i<count; i++)
		{
			/* check out the name of the pop-up menu */
			pm = us_getpopupmenu(par[i]);
			if (pm == NOPOPUPMENU)
			{
				ttyputbadusage(x_("menu setmenubar"));
				return;
			}
			us_pulldowns[i-1] = pm;
			us_validatemenu(pm);
		}

		/* always use native menu handling */
		nativemenuload(count-1, &par[1]);
		return;
	}

	/* see if new fixed menu location has been specified */
	position = us_menupos;
	x = us_menux;   y = us_menuy;
	if (namesamen(pp, x_("top"), l) == 0 && l >= 1)
	{
		position = 0;
		count--;   par++;
	} else if (namesamen(pp, x_("bottom"), l) == 0 && l >= 1)
	{
		position = 1;
		count--;   par++;
	} else if (namesamen(pp, x_("left"), l) == 0 && l >= 1)
	{
		position = 2;
		count--;   par++;
	} else if (namesamen(pp, x_("right"), l) == 0 && l >= 1)
	{
		position = 3;
		count--;   par++;
	}

	/* check for new fixed menu size */
	autoset = FALSE;
	if (count > 0)
	{
		if (namesamen(par[0], x_("size"), estrlen(par[0])) != 0)
		{
			ttyputbadusage(x_("menu"));
			return;
		}
		if (count == 2 && namesamen(par[1], x_("auto"), estrlen(par[1])) == 0)
		{
			autoset = TRUE;
			arctot = pintot = comptot = puretot = 0;
			for(ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			{
				if ((ap->userbits&ANOTUSED) != 0) continue;
				np = getpinproto(ap);
				if (np != NONODEPROTO && (np->userbits & NNOTUSED) != 0) continue;
				arctot++;
			}
			for(np = el_curtech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				if ((np->userbits & NNOTUSED) != 0) continue;
				fun = (np->userbits&NFUNCTION) >> NFUNCTIONSH;
				if (fun == NPPIN) pintot++; else
					if (fun == NPNODE) puretot++; else
						comptot++;
			}
			if (pintot + comptot == 0) pintot = puretot;
			x = arctot + pintot + comptot + 1;   y = 1;
			if (x > 40)
			{
				x = (x+2) / 3;
				y = 3;
			} else if (x > 20)
			{
				x = (x+1) / 2;
				y = 2;
			}
			if (position > 1) { i = x;   x = y;   y = i; }
		} else if (count == 3)
		{
			x = eatoi(par[1]);
			y = eatoi(par[2]);
			if (x <= 0 || y <= 0)
			{
				us_abortcommand(_("Menu sizes must be positive numbers"));
				return;
			}
		} else
		{
			ttyputusage(x_("menu size (ROWS COLUMNS) | auto"));
			return;
		}
	}

	/* set the fixed menu up */
	if (x == us_menux && y == us_menuy && position == us_menupos &&
		(us_tool->toolstate&MENUON) != 0 && !autoset)
	{
		ttyputverbose(M_("Menu has not changed"));
		return;
	}

	if (position != us_menupos) resetpaletteparameters();
	startobjectchange((INTBIG)us_tool, VTOOL);
	if (x != us_menux || y != us_menuy || position != us_menupos || autoset)
		us_setmenusize(x, y, position, autoset);
	if ((us_tool->toolstate&MENUON) == 0)
		(void)setval((INTBIG)us_tool, VTOOL, x_("toolstate"), us_tool->toolstate|MENUON, VINTEGER);
	endobjectchange((INTBIG)us_tool, VTOOL);
}

void us_mirror(INTBIG count, CHAR *par[])
{
	REGISTER NODEINST *ni, *theni, *subni, **nilist, **newnilist;
	REGISTER INTBIG amt, gamt, i;
	REGISTER INTBIG nicount, lx, hx, ly, hy, dist, bestdist, aicount, newnicount;
	REGISTER CHAR *pt;
	REGISTER VARIABLE *var, *tempvar;
	REGISTER ARCINST *ai, **ailist, **newailist;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *thepp, *pp;
	REGISTER GEOM **list, *geom;
	REGISTER HIGHLIGHT *high;
	XARRAY transtz, rot, transfz, t1, t2;
	INTBIG gx, gy, cx, cy, x, y, thex, they, tempcenter[2];
	extern COMCOMP us_mirrorp;

	list = us_gethighlighted(WANTARCINST|WANTNODEINST, 0, 0);
	if (list[0] == NOGEOM)
	{
		us_abortcommand(_("Must select objects to mirror"));
		return;
	}
	np = geomparent(list[0]);

	/* figure out which nodes get mirrored */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		ni->temp1 = 0;
	for(i=0; list[i] != NOGEOM; i++)
	{
		geom = list[i];
		if (geom->entryisnode)
		{
			ni = geom->entryaddr.ni;
			if (us_cantedit(np, ni, TRUE)) continue;
			ni->temp1 = 1;
		} else
		{
			if (us_cantedit(np, NONODEINST, TRUE)) return;
			ai = geom->entryaddr.ai;
			ai->end[0].nodeinst->temp1 = 1;
			ai->end[1].nodeinst->temp1 = 1;
		}
	}

	/* find the one node that is to be mirrored */
	nicount = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->temp1 == 0) continue;
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
	if (nicount > 1)
	{
		/* multiple nodes: find the center one */
		theni = NONODEINST;
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

	if (count == 0)
	{
		count = ttygetparam(M_("Horizontally or vertically? "), &us_mirrorp, MAXPARS, par);
		if (count == 0)
		{
			us_abortedmsg();
			return;
		}
	}
	pt = par[0];
	if (*pt != 'h' && *pt != 'v')
	{
		ttyputusage(x_("mirror horizontal|vertical"));
		return;
	}

	/* determine amount of rotation to effect the mirror */
	if (*pt == 'h')
	{
		if (theni->transpose == 0) amt = 2700; else amt = 900;
	} else
	{
		if (theni->transpose == 0) amt = 900; else amt = 2700;
	}

	/* determine translation when mirroring about grab or trace point */
	if (count >= 2) i = estrlen(par[1]);
	tempvar = NOVARIABLE;
	if (count >= 2 && namesamen(par[1], x_("sensibly"), i) == 0)
	{
		if (nicount == 1)
		{
			if ((el_curwindowpart->state&WINDOWOUTLINEEDMODE) != 0)
			{
				par[1] = x_("about-trace-point");
			} else
			{
				var = getvalkey((INTBIG)theni->proto, VNODEPROTO, VINTEGER|VISARRAY, el_prototype_center_key);
				if (var != NOVARIABLE)
				{
					par[1] = x_("about-grab-point");
				} else
				{
					/* special cases: schematic primitives should mirror sensibly */
					if (theni->proto->primindex != 0 && theni->proto->tech == sch_tech)
					{
						if (theni->proto == sch_transistorprim ||
							theni->proto == sch_transistor4prim)
						{
							tempcenter[0] = 0;
							tempcenter[1] = -(theni->proto->highy - theni->proto->lowy) / 2;
							tempvar = setvalkey((INTBIG)theni->proto, VNODEPROTO, el_prototype_center_key,
								(INTBIG)tempcenter, VINTEGER|VISARRAY|(2<<VLENGTHSH));
							par[1] = x_("about-grab-point");
						} else if (theni->proto == sch_gndprim)
						{
							tempcenter[0] = 0;
							tempcenter[1] = (theni->proto->highy - theni->proto->lowy) / 2;
							tempvar = setvalkey((INTBIG)theni->proto, VNODEPROTO, el_prototype_center_key,
								(INTBIG)tempcenter, VINTEGER|VISARRAY|(2<<VLENGTHSH));
							par[1] = x_("about-grab-point");
						}
					}
				}
			}
		}
	}
	if (count >= 2 && namesamen(par[1], x_("about-grab-point"), i) == 0 && i >= 7)
	{
		if (nicount != 1)
		{
			us_abortcommand(_("Must select 1 node to mirror about its grab point"));
			return;
		}

		/* find the grab point */
		corneroffset(theni, theni->proto, theni->rotation, theni->transpose, &gx, &gy, FALSE);
		gx += theni->lowx;   gy += theni->lowy;

		/* build transformation for this operation */
		transid(transtz);   transtz[2][0] = -gx;   transtz[2][1] = -gy;
		if (*pt == 'h') gamt = 2700; else gamt = 900;
		makeangle(gamt, 1, rot);
		transid(transfz);   transfz[2][0] = gx;   transfz[2][1] = gy;
		transmult(transtz, rot, t1);
		transmult(t1, transfz, t2);

		/* find out where true center moves */
		cx = (theni->lowx+theni->highx)/2;   cy = (theni->lowy+theni->highy)/2;
		xform(cx, cy, &gx, &gy, t2);
		gx -= cx;   gy -= cy;
		if (tempvar != NOVARIABLE)
			(void)delvalkey((INTBIG)theni->proto, VNODEPROTO, el_prototype_center_key);
	} else if (count >= 2 && namesamen(par[1], x_("about-trace-point"), i) == 0 && i >= 7)
	{
		if (nicount != 1)
		{
			us_abortcommand(_("Must select 1 node to mirror about its outline point"));
			return;
		}

		/* get the trace information */
		var = gettrace(theni);
		if (var == NOVARIABLE)
		{
			us_abortcommand(_("Highlighted node must have outline information"));
			return;
		}

		/* find the pivot point */
		high = us_getonehighlight();
		i = high->frompoint;   if (i != 0) i--;
		makerot(theni, t1);
		gx = (theni->highx + theni->lowx) / 2;
		gy = (theni->highy + theni->lowy) / 2;
		xform(((INTBIG *)var->addr)[i*2]+gx, ((INTBIG *)var->addr)[i*2+1]+gy, &gx, &gy, t1);

		/* build transformation for this operation */
		transid(transtz);   transtz[2][0] = -gx;   transtz[2][1] = -gy;
		if (*pt == 'h') gamt = 2700; else gamt = 900;
		makeangle(gamt, 1, rot);
		transid(transfz);   transfz[2][0] = gx;   transfz[2][1] = gy;
		transmult(transtz, rot, t1);
		transmult(t1, transfz, t2);

		/* find out where true center moves */
		cx = (theni->lowx+theni->highx)/2;   cy = (theni->lowy+theni->highy)/2;
		xform(cx, cy, &gx, &gy, t2);
		gx -= cx;   gy -= cy;
	} else gx = gy = 0;

	/* now make sure that it is all connected */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		ai->userbits &= ~ARCFLAGBIT;
	aicount = newnicount = 0;
	for(;;)
	{
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			ni->userbits &= ~NODEFLAGBIT;
		us_nettravel(theni, NODEFLAGBIT);
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if (ni->temp1 == 0) continue;
			if ((ni->userbits&NODEFLAGBIT) == 0) break;
		}
		if (ni == NONODEINST) break;

		/* this node is unconnected: connect it */
		thepp = theni->proto->firstportproto;
		if (thepp == NOPORTPROTO)
		{
			/* no port on the cell: create one */
			subni = newnodeinst(gen_univpinprim, theni->proto->lowx, theni->proto->highx,
				theni->proto->lowy, theni->proto->highy, 0, 0, theni->proto);
			if (subni == NONODEINST) break;
			thepp = newportproto(theni->proto, subni, subni->proto->firstportproto, x_("temp"));

			/* add to the list of temporary nodes */
			newnilist = (NODEINST **)emalloc((newnicount+1) * (sizeof (NODEINST *)), el_tempcluster);
			if (newnilist == 0) break;

			/* LINTED "nilist" used in proper order */
			for(i=0; i<newnicount; i++) newnilist[i] = nilist[i];
			if (newnicount > 0) efree((CHAR *)nilist);
			nilist = newnilist;
			nilist[newnicount] = subni;
			newnicount++;
		}
		pp = ni->proto->firstportproto;
		portposition(theni, thepp, &thex, &they);
		portposition(ni, pp, &x, &y);
		ai = newarcinst(gen_invisiblearc, 0, 0, theni, thepp, thex, they,
			ni, pp, x, y, np);
		if (ai == NOARCINST) break;

		/* add to the list of temporary arcs */
		newailist = (ARCINST **)emalloc((aicount+1) * (sizeof (ARCINST *)), el_tempcluster);
		if (newailist == 0) break;

		/* LINTED "ailist" used in proper order */
		for(i=0; i<aicount; i++) newailist[i] = ailist[i];
		if (aicount > 0) efree((CHAR *)ailist);
		ailist = newailist;
		ailist[aicount] = ai;
		aicount++;
	}

	/* save highlighting */
	us_pushhighlight();
	us_clearhighlightcount();

	/* set arc constraints appropriately */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->end[0].nodeinst->temp1 != 0 && ai->end[1].nodeinst->temp1 != 0)
		{
			/* arc connecting two mirrored nodes: make rigid */
			(void)(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPETEMPRIGID, 0);
		}
	}

	/* mirror the node */
	startobjectchange((INTBIG)theni, VNODEINST);
	modifynodeinst(theni, gx, gy, gx, gy, amt, 1-theni->transpose*2);
	endobjectchange((INTBIG)theni, VNODEINST);

	/* delete intermediate arcs used to constrain */
	for(i=0; i<aicount; i++)
		(void)killarcinst(ailist[i]);
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

	if (*pt == 'h') ttyputverbose(M_("Node mirrored horizontally")); else
		ttyputverbose(M_("Node mirrored vertically"));
}

static DIALOGITEM us_movebydialogitems[] =
{
 /*  1 */ {0, {64,96,88,160}, BUTTON, N_("OK")},
 /*  2 */ {0, {64,8,88,72}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,48,24,142}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,20,24,42}, MESSAGE, N_("dX:")},
 /*  5 */ {0, {32,48,48,142}, EDITTEXT, x_("")},
 /*  6 */ {0, {32,20,48,42}, MESSAGE, N_("dY:")}
};
static DIALOG us_movebydialog = {{50,75,147,244}, N_("Move By Amount"), 0, 6, us_movebydialogitems, 0, 0};

/* special items for the "move by" dialog: */
#define DMVB_XBY       3		/* X change (edit text) */
#define DMVB_XBY_L     4		/* X change label (stat text) */
#define DMVB_YBY       5		/* Y change (edit text) */
#define DMVB_YBY_L     6		/* Y change label (stat text) */

void us_move(INTBIG count, CHAR *par[])
{
	REGISTER NODEINST *ni, **nodelist;
	REGISTER NODEPROTO *np;
	REGISTER ARCINST *ai;
	REGISTER INTBIG ang;
	static INTBIG movebyx = 0, movebyy = 0;
	REGISTER INTBIG l, total, i, wid, len, itemHit, amt;
	INTBIG numtexts, snapx, snapy, bestlx, bestly, lx, hx, ly, hy,
		dx, dy, xcur, ycur, p1x, p1y, p2x, p2y;
	REGISTER GEOM **list;
	HIGHLIGHT thishigh, otherhigh;
	BOOLEAN centeredprimitives;
	REGISTER CHAR *pp;
	CHAR **textlist;
	static POLYGON *poly = NOPOLYGON;
	REGISTER VARIABLE *var;
	REGISTER void *dia;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* make sure there is a current cell */
	np = us_needcell();
	if (np == NONODEPROTO) return;

	/* get the objects to be moved (mark nodes with nonzero "temp1") */
	list = us_gethighlighted(WANTNODEINST | WANTARCINST, &numtexts, &textlist);

	if (list[0] == NOGEOM && numtexts == 0)
	{
		us_abortcommand(_("First select objects to move"));
		return;
	}

	/* make sure they are all in the same cell */
	for(i=0; list[i] != NOGEOM; i++) if (np != geomparent(list[i]))
	{
		us_abortcommand(_("All moved objects must be in the same cell"));
		return;
	}

	/* count the number of nodes */
	for(total=0, ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->temp1 == 0) continue;
		if (us_cantedit(np, ni, TRUE)) return;
		total++;
	}
	if (total == 0 && numtexts == 0) return;

	/* build a list that includes all nodes touching moved arcs */
	if (total > 0)
	{
		nodelist = (NODEINST **)emalloc((total * (sizeof (NODEINST *))), el_tempcluster);
		if (nodelist == 0) return;
		for(i=0, ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			if (ni->temp1 != 0) nodelist[i++] = ni;
	}

	if (count > 0)
	{
		l = estrlen(pp = par[0]);
		if (namesamen(pp, x_("valign"), l) == 0 && l >= 1)
		{
			/* handle vertical alignment */
			if (count < 2) ttyputusage(x_("move valign EDGE")); else
			{
				l = estrlen(pp = par[1]);
				if (namesamen(pp, x_("top"), l) == 0 && l >= 1)
				{
					us_alignnodes(total, nodelist, FALSE, 0);
				} else if (namesamen(pp, x_("bottom"), l) == 0 && l >= 1)
				{
					us_alignnodes(total, nodelist, FALSE, 1);
				} else if (namesamen(pp, x_("center"), l) == 0 && l >= 1)
				{
					us_alignnodes(total, nodelist, FALSE, 2);
				} else ttyputbadusage(x_("move valign"));
			}
			if (total > 0) efree((CHAR *)nodelist);
			return;
		}
		if (namesamen(pp, x_("halign"), l) == 0 && l >= 1)
		{
			/* handle horizontal alignment */
			if (count < 2) ttyputusage(x_("move halign EDGE")); else
			{
				l = estrlen(pp = par[1]);
				if (namesamen(pp, x_("left"), l) == 0 && l >= 1)
				{
					us_alignnodes(total, nodelist, TRUE, 0);
				} else if (namesamen(pp, x_("right"), l) == 0 && l >= 1)
				{
					us_alignnodes(total, nodelist, TRUE, 1);
				} else if (namesamen(pp, x_("center"), l) == 0 && l >= 1)
				{
					us_alignnodes(total, nodelist, TRUE, 2);
				} else ttyputbadusage(x_("move halign"));
			}
			if (total > 0) efree((CHAR *)nodelist);
			return;
		}
	}

	/* figure out lower-left corner of this collection of objects */
	if (total == 0)
	{
		bestlx = (np->lowx + np->highx) / 2;
		bestly = (np->lowy + np->highy) / 2;
	} else
	{
		us_getlowleft(nodelist[0], &bestlx, &bestly);
	}
	for(i=1; i<total; i++)
	{
		us_getlowleft(nodelist[i], &lx, &ly);
		if (lx < bestlx) bestlx = lx;
		if (ly < bestly) bestly = ly;
	}
	for(i=0; list[i] != NOGEOM; i++) if (!list[i]->entryisnode)
	{
		ai = list[i]->entryaddr.ai;
		wid = ai->width - arcwidthoffset(ai);
		makearcpoly(ai->length, wid, ai, poly, FILLED);
		getbbox(poly, &lx, &hx, &ly, &hy);
		if (lx < bestlx) bestlx = lx;
		if (ly < bestly) bestly = ly;
	}

	/* special case when moving one node: account for cell center */
	if (total == 1 && list[1] == NOGEOM && numtexts == 0)
	{
		/* get standard corner offset */
		ni = nodelist[0];
		if ((us_useroptions&CENTEREDPRIMITIVES) != 0) centeredprimitives = TRUE; else
			centeredprimitives = FALSE;
		corneroffset(ni, ni->proto, ni->rotation, ni->transpose, &lx, &ly,
			centeredprimitives);
		bestlx = ni->lowx + lx;
		bestly = ni->lowy + ly;
	}

	/* special case if there is exactly one snap point */
	if (us_getonesnappoint(&snapx, &snapy))
	{
		bestlx = snapx;   bestly = snapy;
	}

	/* special case if two objects are selected with snap points */
	if (count == 0)
	{
		var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
		if (var != NOVARIABLE)
		{
			len = getlength(var);
			if (len == 2)
			{
				if (!us_makehighlight(((CHAR **)var->addr)[0], &thishigh) &&
					!us_makehighlight(((CHAR **)var->addr)[1], &otherhigh))
				{
					/* describe these two objects */
					if ((thishigh.status&HIGHSNAP) != 0 && (otherhigh.status&HIGHSNAP) != 0)
					{
						/* move first to second */
						us_getsnappoint(&thishigh, &p1x, &p1y);
						us_getsnappoint(&otherhigh, &p2x, &p2y);
						dx = p2x - p1x;
						dy = p2y - p1y;
						if (dx == 0 && dy == 0)
						{
							us_abortcommand(_("Points are already aligned"));
							if (total > 0) efree((CHAR *)nodelist);
							return;
						}
						ni = thishigh.fromgeom->entryaddr.ni;
						us_pushhighlight();
						us_clearhighlightcount();
						startobjectchange((INTBIG)ni, VNODEINST);
						modifynodeinst(ni, dx, dy, dx, dy, 0, 0);
						endobjectchange((INTBIG)ni, VNODEINST);
						us_pophighlight(TRUE);
						if (total > 0) efree((CHAR *)nodelist);
						return;
					}
				}
			}
		}
	}

	/* no arguments: simple motion */
	if (count == 0)
	{
		if ((us_tool->toolstate&INTERACTIVE) != 0)
		{
			/* interactive motion: track cursor */
			us_multidraginit(bestlx, bestly, list, nodelist, total, 0, TRUE);
			trackcursor(FALSE, us_ignoreup, us_multidragbegin, us_multidragdown,
				us_stoponchar, us_multidragup, TRACKDRAGGING);
			if (el_pleasestop != 0)
			{
				if (total > 0) efree((CHAR *)nodelist);
				return;
			}
		}

		/* get co-ordinates of cursor */
		if (us_demandxy(&xcur, &ycur))
		{
			if (total > 0) efree((CHAR *)nodelist);
			return;
		}
		gridalign(&xcur, &ycur, 1, np);

		/* make the move if it is valid */
		if (xcur == bestlx && ycur == bestly) ttyputverbose(M_("Null motion")); else
		{
			us_pushhighlight();
			us_clearhighlightcount();
			us_manymove(list, nodelist, total, xcur-bestlx, ycur-bestly);
			us_moveselectedtext(numtexts, textlist, list, xcur-bestlx, ycur-bestly);
			us_pophighlight(TRUE);
		}
		if (total > 0) efree((CHAR *)nodelist);
		return;
	}

	l = estrlen(pp = par[0]);

	/* handle absolute motion option "move to X Y" */
	if (namesamen(pp, x_("to"), l) == 0 && l >= 1)
	{
		/* get absolute position to place object */
		if (count < 3)
		{
			ttyputusage(x_("move to X Y"));
			if (total > 0) efree((CHAR *)nodelist);
			return;
		}
		xcur = atola(par[1], 0);
		ycur = atola(par[2], 0);

		/* make the move if it is valid */
		if (xcur == bestlx && ycur == bestly) ttyputverbose(M_("Null motion")); else
		{
			us_pushhighlight();
			us_clearhighlightcount();
			us_manymove(list, nodelist, total, xcur-bestlx, ycur-bestly);
			us_moveselectedtext(numtexts, textlist, list, xcur-bestlx, ycur-bestly);
			us_pophighlight(TRUE);
		}
		if (total > 0) efree((CHAR *)nodelist);
		return;
	}

	/* handle relative motion option "move by X Y" */
	if (namesamen(pp, x_("by"), l) == 0 && l >= 1)
	{
		/* get absolute position to place object */
		if (count == 2 && namesamen(par[1], x_("dialog"), estrlen(par[1])) == 0)
		{
			/* get coordinates from dialog */
			dia = DiaInitDialog(&us_movebydialog);
			if (dia == 0)
			{
				if (total > 0) efree((CHAR *)nodelist);
				return;
			}
			DiaSetText(dia, DMVB_XBY, frtoa(movebyx));
			DiaSetText(dia, DMVB_YBY, frtoa(movebyy));
			for(;;)
			{
				itemHit = DiaNextHit(dia);
				if (itemHit == CANCEL || itemHit == OK) break;
			}
			movebyx = atofr(DiaGetText(dia, DMVB_XBY));
			movebyy = atofr(DiaGetText(dia, DMVB_YBY));
			xcur = atola(DiaGetText(dia, DMVB_XBY), 0);
			ycur = atola(DiaGetText(dia, DMVB_YBY), 0);
			DiaDoneDialog(dia);
			if (itemHit == CANCEL)
			{
				if (total > 0) efree((CHAR *)nodelist);
				return;
			}
		} else
		{
			/* get coordinates from command line */
			if (count < 3)
			{
				ttyputusage(x_("move by X Y"));
				if (total > 0) efree((CHAR *)nodelist);
				return;
			}
			xcur = atola(par[1], 0);
			ycur = atola(par[2], 0);
		}

		/* make the move if it is valid */
		if (xcur == 0 && ycur == 0) ttyputverbose(M_("Null motion")); else
		{
			us_pushhighlight();
			us_clearhighlightcount();
			us_manymove(list, nodelist, total, xcur, ycur);
			us_moveselectedtext(numtexts, textlist, list, xcur, ycur);
			us_pophighlight(TRUE);
		}
		if (total > 0) efree((CHAR *)nodelist);
		return;
	}

	/* handle motion to cursor along an angle: "move angle ANG" */
	if (namesamen(pp, x_("angle"), l) == 0 && l >= 1)
	{
		/* get co-ordinates of cursor */
		if (us_demandxy(&xcur, &ycur))
		{
			if (total > 0) efree((CHAR *)nodelist);
			return;
		}
		gridalign(&xcur, &ycur, 1, np);

		/* get angle to align along */
		if (count < 2)
		{
			ttyputusage(x_("move angle ANGLE"));
			if (total > 0) efree((CHAR *)nodelist);
			return;
		}
		ang = atofr(par[1])*10/WHOLE;

		/* adjust the cursor position if selecting interactively */
		if ((us_tool->toolstate&INTERACTIVE) != 0)
		{
			us_multidraginit(bestlx, bestly, list, nodelist, total, ang, TRUE);
			trackcursor(FALSE, us_ignoreup, us_multidragbegin, us_multidragdown,
				us_stoponchar, us_multidragup, TRACKDRAGGING);
			if (el_pleasestop != 0)
			{
				if (total > 0) efree((CHAR *)nodelist);
				return;
			}
			if (us_demandxy(&xcur, &ycur))
			{
				if (total > 0) efree((CHAR *)nodelist);
				return;
			}
			gridalign(&xcur, &ycur, 1, np);
		}

		/* compute sliding for this highlighting */
		us_getslide(ang, bestlx, bestly, xcur, ycur, &dx, &dy);

		/* make the move if it is nonnull */
		if (dx == bestlx && dy == bestly) ttyputverbose(M_("Null motion")); else
		{
			us_pushhighlight();
			us_clearhighlightcount();
			us_manymove(list, nodelist, total, dx-bestlx, dy-bestly);
			us_moveselectedtext(numtexts, textlist, list, xcur-bestlx, ycur-bestly);
			us_pophighlight(TRUE);
		}
		if (total > 0) efree((CHAR *)nodelist);
		return;
	}

	/* if direction is specified, get it */
	dx = dy = 0;
	if (namesamen(pp, x_("up"), l) == 0 && l >= 1)
	{
		/* in outline edit, this makes no sense */
		if ((el_curwindowpart->state&WINDOWOUTLINEEDMODE) != 0)
		{
			if (total > 0) efree((CHAR *)nodelist);
			return;
		}
		if (count < 2) amt = el_curlib->lambda[el_curtech->techindex]; else
		{
			l = el_units;
			el_units = (el_units & ~DISPLAYUNITS) | DISPUNITLAMBDA;
			amt = atola(par[1], 0);
			el_units = l;
		}
		dy += amt;
	} else if (namesamen(pp, x_("down"), l) == 0 && l >= 1)
	{
		/* in outline edit, this makes no sense */
		if ((el_curwindowpart->state&WINDOWOUTLINEEDMODE) != 0)
		{
			if (total > 0) efree((CHAR *)nodelist);
			return;
		}
		if (count < 2) amt = el_curlib->lambda[el_curtech->techindex]; else
		{
			l = el_units;
			el_units = (el_units & ~DISPLAYUNITS) | DISPUNITLAMBDA;
			amt = atola(par[1], 0);
			el_units = l;
		}
		dy -= amt;
	} else if (namesamen(pp, x_("left"), l) == 0 && l >= 1)
	{
		/* in outline edit, change selection to previous point */
		if ((el_curwindowpart->state&WINDOWOUTLINEEDMODE) != 0)
		{
			par[0] = x_("trace");
			par[1] = x_("prev-point");
			us_node(2, par);
			if (total > 0) efree((CHAR *)nodelist);
			return;
		}
		if (count < 2) amt = el_curlib->lambda[el_curtech->techindex]; else
		{
			l = el_units;
			el_units = (el_units & ~DISPLAYUNITS) | DISPUNITLAMBDA;
			amt = atola(par[1], 0);
			el_units = l;
		}
		dx -= amt;
	} else if (namesamen(pp, x_("right"), l) == 0 && l >= 1)
	{
		/* in outline edit, change selection to next point */
		if ((el_curwindowpart->state&WINDOWOUTLINEEDMODE) != 0)
		{
			par[0] = x_("trace");
			par[1] = x_("next-point");
			us_node(2, par);
			if (total > 0) efree((CHAR *)nodelist);
			return;
		}
		if (count < 2) amt = el_curlib->lambda[el_curtech->techindex]; else
		{
			l = el_units;
			el_units = (el_units & ~DISPLAYUNITS) | DISPUNITLAMBDA;
			amt = atola(par[1], 0);
			el_units = l;
		}
		dx += amt;
	} else
	{
		ttyputbadusage(x_("move"));
		if (total > 0) efree((CHAR *)nodelist);
		return;
	}

	/* transform to space of this window */
	if ((el_curwindowpart->state&INPLACEEDIT) != 0)
	{
		xform(0, 0, &snapx, &snapy, el_curwindowpart->outofcell);
		snapx += dx;   snapy += dy;
		xform(snapx, snapy, &dx, &dy, el_curwindowpart->intocell);
	}

	/* now move the object */
	if (dx != 0 || dy != 0)
	{
		us_pushhighlight();
		us_clearhighlightcount();
		us_manymove(list, nodelist, total, dx, dy);
		us_moveselectedtext(numtexts, textlist, list, dx, dy);
		us_pophighlight(TRUE);
	}
	if (total > 0) efree((CHAR *)nodelist);
}

void us_node(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG amt, i, *newlist, x, y, curlamb, inside, outside, l, negated, size, nogood,
		total, addpoint, deletepoint, whichpoint, p, movepoint, pointgiven, segs, degs,
		nextpoint, prevpoint;
	BOOLEAN waitfordown, stillok;
	XARRAY trans;
	INTBIG d[2], xcur, ycur, initlist[8];
	REGISTER CHAR *pt, *ch;
	REGISTER GEOM **list;
	extern COMCOMP us_nodep;
	REGISTER TECHNOLOGY *tech;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni, *store;
	REGISTER HIGHLIGHT *high;
	HIGHLIGHT newhigh;
	REGISTER VARIABLE *var;

	/* disallow node modification if lock is on */
	np = us_needcell();
	if (np == NONODEPROTO) return;

	if (count == 0)
	{
		count = ttygetparam(M_("Node option: "), &us_nodep, MAXPARS, par);
		if (count == 0)
		{
			us_abortedmsg();
			return;
		}
	}
	l = estrlen(pt = par[0]);

	/* handle negation */
	if (namesamen(pt, x_("not"), l) == 0 && l >= 2)
	{
		negated = 1;
		count--;
		par++;
		l = estrlen(pt = par[0]);
	} else negated = 0;

	if (namesamen(pt, x_("expand"), l) == 0 && l >= 1)
	{
		if (count < 2) amt = MAXINTBIG; else amt = eatoi(par[1]);
		list = us_gethighlighted(WANTNODEINST, 0, 0);
		if (list[0] == NOGEOM)
		{
			us_abortcommand(_("Select nodes to be expanded"));
			return;
		}

		/* save highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* pre-compute */
		for(i=0; list[i] != NOGEOM; i++)
		{
			ni = list[i]->entryaddr.ni;
			if (negated)
			{
				if ((ni->userbits & NEXPAND) != 0)
					(void)us_setunexpand(ni, amt);
			}
			startobjectchange((INTBIG)ni, VNODEINST);
		}

		/* do the change */
		for(i=0; list[i] != NOGEOM; i++)
		{
			ni = list[i]->entryaddr.ni;
			if (negated == 0) us_doexpand(ni, amt, 0); else
				us_dounexpand(ni);
		}

		/* post-draw */
		for(i=0; list[i] != NOGEOM; i++)
		{
			ni = list[i]->entryaddr.ni;
			endobjectchange((INTBIG)ni, VNODEINST);
		}

		/* restore highlighting */
		us_pophighlight(FALSE);
		return;
	}

	if (namesamen(pt, x_("name"), l) == 0 && l >= 2)
	{
		ni = (NODEINST *)us_getobject(VNODEINST, FALSE);
		if (ni == NONODEINST) return;

		if (negated == 0)
		{
			if (count < 2)
			{
				ttyputusage(x_("node name NODENAME"));
				return;
			}
			ch = par[1];

			/* sensibility check for multiple nodes with the same name */
			for(store = ni->parent->firstnodeinst; store != NONODEINST; store = store->nextnodeinst)
			{
				if (store == ni) continue;
				var = getvalkey((INTBIG)store, VNODEINST, VSTRING, el_node_name_key);
				if (var == NOVARIABLE) continue;
				if (namesame(ch, (CHAR *)var->addr) == 0)
				{
					ttyputmsg(_("Warning: already a node in this cell called %s"), ch);
					break;
				}
			}
		}

		/* save highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* change the name of the nodeinst */
		startobjectchange((INTBIG)ni, VNODEINST);
		if (negated != 0) (void)delvalkey((INTBIG)ni, VNODEINST, el_node_name_key); else
		{
			var = setvalkey((INTBIG)ni, VNODEINST, el_node_name_key, (INTBIG)ch, VSTRING|VDISPLAY);
			if (var != NOVARIABLE)
			{
				defaulttextsize(3, var->textdescript);

				/* shift text down if on a cell instance */
				if (ni->proto->primindex == 0)
				{
					us_setdescriptoffset(var->textdescript,
						0, (ni->highy-ni->lowy) / el_curlib->lambda[el_curtech->techindex]);
				}
			}
		}
		endobjectchange((INTBIG)ni, VNODEINST);

		/* restore highlighting */
		us_pophighlight(FALSE);

		/* report results and quit */
		if (negated != 0) ttyputverbose(M_("Node name removed")); else
			ttyputverbose(M_("Node name is '%s'"), ch);
		return;
	}

	/* everything after this point is structural and must check for locks */
	if (us_cantedit(np, NONODEINST, TRUE)) return;

	if (namesamen(pt, x_("cover-implant"), l) == 0 && l >= 1)
	{
		us_coverimplant();
		return;
	}

	if (namesamen(pt, x_("regrid-selected"), l) == 0 && l >= 1)
	{
		us_regridselected();
		return;
	}

	if (namesamen(pt, x_("trace"), l) == 0 && l >= 1)
	{
		/* special case for converting text to layout */
		if (count < 2)
		{
			ttyputusage(x_("node trace OPTIONS"));
			return;
		}
		l = estrlen(pt = par[1]);
		if (namesamen(pt, x_("init-points"), l) == 0 && l >= 1)
		{
			store = (NODEINST *)us_getobject(VNODEINST, FALSE);
			if (store == NONODEINST) return;
			if ((store->proto->userbits&HOLDSTRACE) == 0)
			{
				us_abortcommand(_("Sorry, %s nodes cannot hold outline information"),
					describenodeproto(store->proto));
				return;
			}
			if (store->proto == art_openedpolygonprim || store->proto == art_openeddottedpolygonprim ||
				store->proto == art_openeddashedpolygonprim || store->proto == art_openedthickerpolygonprim ||
				store->proto == art_splineprim)
			{
				x = (store->lowx + store->highx) / 2;
				y = (store->lowy + store->highy) / 2;
				tech = store->parent->tech;
				curlamb = store->parent->lib->lambda[tech->techindex];
				initlist[0] = x - curlamb * 3;
				initlist[1] = y - curlamb * 3;
				initlist[2] = x - curlamb;
				initlist[3] = y + curlamb * 3;
				initlist[4] = x + curlamb;
				initlist[5] = y - curlamb * 3;
				initlist[6] = x + curlamb * 3;
				initlist[7] = y + curlamb * 3;
				us_pushhighlight();
				us_clearhighlightcount();
				us_settrace(store, initlist, 4);
				us_pophighlight(FALSE);
			} else if (store->proto == art_closedpolygonprim || store->proto == art_filledpolygonprim)
			{
				x = (store->lowx + store->highx) / 2;
				y = (store->lowy + store->highy) / 2;
				tech = store->parent->tech;
				curlamb = store->parent->lib->lambda[tech->techindex];
				initlist[0] = x;
				initlist[1] = y - curlamb * 3;
				initlist[2] = x - curlamb * 3;
				initlist[3] = y;
				initlist[4] = x;
				initlist[5] = y + curlamb * 3;
				initlist[6] = x + curlamb * 3;
				initlist[7] = y - curlamb * 3;
				us_pushhighlight();
				us_clearhighlightcount();
				us_settrace(store, initlist, 4);
				us_pophighlight(FALSE);
			}
			return;
		}
		if (namesamen(pt, x_("place-text"), l) == 0 && l >= 2)
		{
			if (count < 10)
			{
				ttyputusage(x_("node trace place-text LAYER SIZE FONT ITALIC BOLD UNDERLINE SEPARATION MESSAGE"));
				return;
			}
			us_layouttext(par[2], eatoi(par[3]), 1, eatoi(par[4]), eatoi(par[5]),
				eatoi(par[6]), eatoi(par[7]), eatoi(par[8]), par[9]);
			return;
		}

		/* special case for filleting */
		if (namesamen(pt, x_("fillet"), l) == 0)
		{
			us_dofillet();
			return;
		}

		/* special case for annulus construction */
		if (namesamen(pt, x_("construct-annulus"), l) == 0)
		{
			if (count < 4)
			{
				ttyputusage(x_("node trace construct-annulus INSIDE OUTSIDE [RESOLUTION] [DEGREES]"));
				return;
			}
			inside = atola(par[2], 0);
			outside = atola(par[3], 0);
			segs = 16;
			if (count >= 5) segs = myatoi(par[4]);
			if (segs < 4) segs = 4;
			degs = 3600;
			if (count >= 6) degs = atofr(par[5])*10/WHOLE;
			if (degs <= 0) degs = 3600;
			if (degs > 3600) degs = 3600;

			/* make sure node can handle trace information */
			store = (NODEINST *)us_getobject(VNODEINST, FALSE);
			if (store == NONODEINST) return;
			if ((store->proto->userbits&HOLDSTRACE) == 0)
			{
				us_abortcommand(_("Sorry, %s nodes cannot hold outline information"),
					describenodeproto(store->proto));
				return;
			}

			/* allocate space for the trace */
			newlist = emalloc(((segs+1)*4*SIZEOFINTBIG), el_tempcluster);
			if (newlist == 0)
			{
				ttyputnomemory();
				return;
			}

			l = 0;
			if (inside > 0)
			{
				for(i=0; i<=segs; i++)
				{
					p = degs*i/segs;
					x = mult(inside, cosine(p)) + (store->lowx+store->highx)/2;
					y = mult(inside, sine(p)) + (store->lowy+store->highy)/2;
					newlist[l++] = x;   newlist[l++] = y;
				}
			}
			for(i=segs; i>=0; i--)
			{
				p = degs*i/segs;
				x = mult(outside, cosine(p)) + (store->lowx+store->highx)/2;
				y = mult(outside, sine(p)) + (store->lowy+store->highy)/2;
				newlist[l++] = x;   newlist[l++] = y;
			}

			/* save highlighting, set trace, restore highlighting */
			us_pushhighlight();
			us_clearhighlightcount();
			us_settrace(store, newlist, l/2);
			us_pophighlight(FALSE);
			efree((CHAR *)newlist);
			return;
		}

		/* parse the options */
		store = NONODEINST;
		waitfordown = FALSE;
		addpoint = deletepoint = movepoint = pointgiven = nextpoint = prevpoint = 0;
		for(i=1; i<count; i++)
		{
			l = estrlen(pt = par[i]);

			if (namesamen(pt, x_("store-trace"), l) == 0 && l >= 1)
			{
				store = (NODEINST *)us_getobject(VNODEINST, FALSE);
				if (store == NONODEINST) return;
			} else if (namesamen(pt, x_("add-point"), l) == 0 && l >= 1)
			{
				addpoint++;
				store = (NODEINST *)us_getobject(VNODEINST, FALSE);
				if (store == NONODEINST) return;
				high = us_getonehighlight();
				whichpoint = high->frompoint;
				if (count > i+2 && isanumber(par[i+1]) && isanumber(par[i+2]))
				{
					xcur = atola(par[i+1], 0);
					ycur = atola(par[i+2], 0);
					i += 2;
					pointgiven++;
				}
			} else if (namesamen(pt, x_("move-point"), l) == 0 && l >= 1)
			{
				movepoint++;
				store = (NODEINST *)us_getobject(VNODEINST, FALSE);
				if (store == NONODEINST) return;
				high = us_getonehighlight();
				whichpoint = high->frompoint;
				if (count > i+2 && isanumber(par[i+1]) && isanumber(par[i+2]))
				{
					xcur = atola(par[i+1], 0);
					ycur = atola(par[i+2], 0);
					i += 2;
					pointgiven++;
				}
			} else if (namesamen(pt, x_("delete-point"), l) == 0 && l >= 1)
			{
				deletepoint++;
				store = (NODEINST *)us_getobject(VNODEINST, FALSE);
				if (store == NONODEINST) return;
				high = us_getonehighlight();
				whichpoint = high->frompoint;
			} else if (namesamen(pt, x_("next-point"), l) == 0 && l >= 1)
			{
				nextpoint++;
				store = (NODEINST *)us_getobject(VNODEINST, FALSE);
				if (store == NONODEINST) return;
				high = us_getonehighlight();
				whichpoint = high->frompoint;
			} else if (namesamen(pt, x_("prev-point"), l) == 0 && l >= 2)
			{
				prevpoint++;
				store = (NODEINST *)us_getobject(VNODEINST, FALSE);
				if (store == NONODEINST) return;
				high = us_getonehighlight();
				whichpoint = high->frompoint;
			} else if (namesamen(pt, x_("wait-for-down"), l) == 0 && l >= 1)
			{
				waitfordown = TRUE;
			} else
			{
				ttyputbadusage(x_("node trace"));
				return;
			}
		}

		if (addpoint + deletepoint + movepoint + nextpoint + prevpoint > 1)
		{
			us_abortcommand(_("Can only add OR delete OR move OR change point"));
			return;
		}

		/* make sure node can handle trace information */
		if (store != NONODEINST)
		{
			if ((store->proto->userbits&HOLDSTRACE) == 0)
			{
				us_abortcommand(_("Sorry, %s nodes cannot hold outline information"),
					describenodeproto(store->proto));
				return;
			}
		}

		/* handle moving around the trace */
		if (nextpoint != 0)
		{
			var = gettrace(store);
			if (var == NOVARIABLE)
			{
				us_abortcommand(_("No points on this node"));
				return;
			}
			size = getlength(var) / 2;
			whichpoint++;
			if (whichpoint >= size) whichpoint = 0;

			us_clearhighlightcount();
			newhigh.status = HIGHFROM;
			newhigh.fromgeom = store->geom;
			newhigh.fromport = NOPORTPROTO;
			newhigh.frompoint = whichpoint;
			newhigh.cell = store->parent;
			us_addhighlight(&newhigh);
			return;
		}
		if (prevpoint != 0)
		{
			var = gettrace(store);
			if (var == NOVARIABLE)
			{
				us_abortcommand(_("No points on this node"));
				return;
			}
			size = getlength(var) / 2;
			whichpoint--;
			if (whichpoint < 0) whichpoint = size-1;

			us_clearhighlightcount();
			newhigh.status = HIGHFROM;
			newhigh.fromgeom = store->geom;
			newhigh.fromport = NOPORTPROTO;
			newhigh.frompoint = whichpoint;
			newhigh.cell = store->parent;
			us_addhighlight(&newhigh);
			return;
		}

		/* handle freeform cursor traces */
		if (addpoint == 0 && deletepoint == 0 && movepoint == 0)
		{
			/* just do tracking if no storage requested */
			if (store == NONODEINST)
			{
				/* save highlighting */
				us_pushhighlight();
				us_clearhighlightcount();

				trackcursor(waitfordown, us_ignoreup, us_tracebegin,
					us_tracedown, us_stoponchar, us_traceup, TRACKDRAWING);

				/* restore highlighting */
				us_pophighlight(FALSE);
				return;
			}

			/* clear highlighting */
			us_clearhighlightcount();

			/* get the trace */
			trackcursor(waitfordown, us_ignoreup, us_tracebegin, us_tracedown,
				us_stoponchar, us_traceup, TRACKDRAWING);
			if (el_pleasestop != 0) return;

			/* get the trace data in the "%T" variable */
			var = getval((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_commandvarname('T'));
			if (var == NOVARIABLE)
			{
				us_abortcommand(_("Invalid outline information"));
				return;
			}
			size = getlength(var) / 2;

			/* create a place to keep this data */
			newlist = emalloc((size*2*SIZEOFINTBIG), el_tempcluster);
			if (newlist == 0)
			{
				ttyputnomemory();
				return;
			}

			/* copy the trace from "%T" to "newlist" */
			curlamb = el_curlib->lambda[el_curtech->techindex];
			el_curlib->lambda[el_curtech->techindex] = 4;
			nogood = 0;
			for(i=0; i<size; i++)
			{
				x = ((INTBIG *)var->addr)[i*2];
				y = ((INTBIG *)var->addr)[i*2+1];
				if (us_setxy(x, y)) nogood++;
				(void)getxy(&xcur, &ycur);
				newlist[i*2] = xcur;   newlist[i*2+1] = ycur;
			}
			el_curlib->lambda[el_curtech->techindex] = curlamb;

			/* if data is valid, store it in the node */
			if (nogood != 0) ttyputerr(_("Outline not inside window")); else
			{
				us_settrace(store, newlist, size);

				/* highlighting the node */
				newhigh.status = HIGHFROM;
				newhigh.fromgeom = store->geom;
				newhigh.fromport = NOPORTPROTO;
				newhigh.frompoint = size+1;
				newhigh.cell = store->parent;
				us_addhighlight(&newhigh);
			}
			efree((CHAR *)newlist);
			return;
		}

		/* add a point to a trace on this object */
		if (addpoint != 0)
		{
			/* clear highlighting */
			us_clearhighlightcount();

			stillok = TRUE;
			if (pointgiven == 0)
			{
				if (us_demandxy(&xcur, &ycur)) stillok = FALSE; else
				{
					gridalign(&xcur, &ycur, 1, np);

					/* adjust the cursor position if selecting interactively */
					if ((us_tool->toolstate&INTERACTIVE) != 0)
					{
						us_pointinit(store, whichpoint);
						trackcursor(FALSE, us_ignoreup, us_pointbegin, us_addpdown,
							us_stoponchar, us_dragup, TRACKDRAGGING);
						if (el_pleasestop != 0) stillok = FALSE; else
						{
							if (us_demandxy(&xcur, &ycur)) stillok = FALSE; else
								gridalign(&xcur, &ycur, 1, np);
						}
					}
				}
			}

			if (stillok)
			{
				var = gettrace(store);
				if (var == NOVARIABLE)
				{
					d[0] = xcur;   d[1] = ycur;
					us_settrace(store, d, 1);
					whichpoint = 1;
				} else
				{
					size = getlength(var) / 2;
					newlist = emalloc(((size+1)*2*SIZEOFINTBIG), el_tempcluster);
					if (newlist == 0)
					{
						ttyputnomemory();
						return;
					}
					p = 0;
					makerot(store, trans);
					x = (store->highx + store->lowx) / 2;
					y = (store->highy + store->lowy) / 2;
					if (whichpoint == 0) whichpoint++;
					for(i=0; i<size; i++)
					{
						xform(((INTBIG *)var->addr)[i*2]+x, ((INTBIG *)var->addr)[i*2+1]+y, &newlist[p],
							&newlist[p+1], trans);
						p += 2;
						if (i+1 == whichpoint)
						{
							newlist[p++] = xcur;
							newlist[p++] = ycur;
						}
					}

					/* now re-draw this trace */
					us_settrace(store, newlist, size+1);
					whichpoint++;
					efree((CHAR *)newlist);
				}
			}

			/* highlighting the node */
			newhigh.status = HIGHFROM;
			newhigh.fromgeom = store->geom;
			newhigh.fromport = NOPORTPROTO;
			newhigh.frompoint = whichpoint;
			newhigh.cell = store->parent;
			us_addhighlight(&newhigh);
			return;
		}

		/* delete the current point */
		if (deletepoint != 0)
		{
			var = gettrace(store);
			if (var == NOVARIABLE)
			{
				us_abortcommand(_("No outline data to delete"));
				return;
			}
			if (whichpoint == 0)
			{
				us_abortcommand(_("Highlight a point or line to delete"));
				return;
			}
			size = getlength(var) / 2;
			if (size <= 1)
			{
				us_abortcommand(_("Outline must retain at least one point"));
				return;
			}
			newlist = emalloc(((size-1)*2*SIZEOFINTBIG), el_tempcluster);
			if (newlist == 0)
			{
				ttyputnomemory();
				return;
			}
			p = 0;
			makerot(store, trans);
			x = (store->highx + store->lowx) / 2;
			y = (store->highy + store->lowy) / 2;
			total = 0;
			for(i=0; i<size; i++)
			{
				if (i+1 == whichpoint) { total++;   continue;}
				xform(((INTBIG *)var->addr)[i*2]+x, ((INTBIG *)var->addr)[i*2+1]+y,
					&newlist[p], &newlist[p+1], trans);
				p += 2;
			}

			/* clear highlighting */
			us_clearhighlightcount();

			/* now re-draw this trace */
			us_settrace(store, newlist, size-total);
			whichpoint = maxi(whichpoint-total, 1);
			efree((CHAR *)newlist);

			/* highlighting the node */
			newhigh.status = HIGHFROM;
			newhigh.fromgeom = store->geom;
			newhigh.fromport = NOPORTPROTO;
			newhigh.frompoint = whichpoint;
			newhigh.cell = store->parent;
			us_addhighlight(&newhigh);
			return;
		}

		/* move a point on a trace on this object */
		if (movepoint != 0)
		{
			var = gettrace(store);
			if (var == NOVARIABLE)
			{
				us_abortcommand(_("No outline data to move"));
				return;
			}
			if (whichpoint == 0)
			{
				us_abortcommand(_("Highlight a point or line to move"));
				return;
			}

			/* save highlighting */
			us_pushhighlight();
			us_clearhighlightcount();

			stillok = TRUE;
			if (pointgiven == 0)
			{
				if (us_demandxy(&xcur, &ycur)) stillok = FALSE; else
				{
					gridalign(&xcur, &ycur, 1, np);

					/* adjust the cursor position if selecting interactively */
					if ((us_tool->toolstate&INTERACTIVE) != 0)
					{
						us_pointinit(store, whichpoint);
						trackcursor(FALSE, us_ignoreup, us_pointbegin, us_movepdown,
							us_stopandpoponchar, us_dragup, TRACKDRAGGING);
						if (el_pleasestop != 0) stillok = FALSE; else
						{
							if (us_demandxy(&xcur, &ycur)) stillok = FALSE; else
								gridalign(&xcur, &ycur, 1, np);
						}
					}
				}
			}

			if (stillok)
			{
				size = getlength(var) / 2;
				newlist = emalloc((size*2*SIZEOFINTBIG), el_tempcluster);
				if (newlist == 0)
				{
					ttyputnomemory();
					us_pophighlight(FALSE);
					return;
				}
				makerot(store, trans);
				x = (store->highx + store->lowx) / 2;
				y = (store->highy + store->lowy) / 2;
				for(i=0; i<size; i++)
				{
					if (i+1 == whichpoint)
					{
						newlist[i*2] = xcur;
						newlist[i*2+1] = ycur;
					} else xform(((INTBIG *)var->addr)[i*2]+x, ((INTBIG *)var->addr)[i*2+1]+y,
						&newlist[i*2], &newlist[i*2+1], trans);
				}

				/* now re-draw this trace */
				us_settrace(store, newlist, size);
				efree((CHAR *)newlist);
			}

			/* restore highlighting */
			us_pophighlight(TRUE);
			return;
		}
		return;
	}

	ttyputbadusage(x_("node"));
}
