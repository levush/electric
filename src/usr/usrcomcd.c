/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrcomcd.c
 * User interface tool: command handler for C through D
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
#include "database.h"
#include "tecgen.h"
#include "tecart.h"
#include "tecschem.h"
#include "efunction.h"
#include "usrdiacom.h"
#include <math.h>

#define MAXLINE	   200		/* max characters on color map input line */
static struct {
	INTBIG bits;
	INTBIG set;
} us_overlappables[] =
{
	{LAYERT1, 0},
	{LAYERT2, 0},
	{LAYERT3, 0},
	{LAYERT4, 0},
	{LAYERT5, 0},
	{0, 0}
};

void us_color(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG i, j, k, l, high, totalcolor, max, red, green, blue,
		newmax, style, newraster;
	CHAR line[MAXLINE], *layerlabel[5], *layerabbrev[5], *filename, *truename, **layernames;
	INTBIG redt[256], greent[256], bluet[256], rr, rg, rb, *layerdata;
	float hue, sat, inten, amt;
	REGISTER VARIABLE *varred, *vargreen, *varblue;
	REGISTER CHAR *pp, *orig, *s, *la;
	GRAPHICS *desc;
	REGISTER FILE *f;
	extern COMCOMP us_colorp, us_colorentryp, us_colorreadp, us_colorwritep;
	REGISTER void *infstr;

	if (count == 0)
	{
		count = ttygetparam(M_("COLOR option: "), &us_colorp, MAXPARS, par);
		if (count == 0)
		{
			us_abortedmsg();
			return;
		}
	}
	l = estrlen(pp = par[0]);

	if (namesamen(pp, x_("black-background"), l) == 0 && l >= 1)
	{
		us_getcolormap(el_curtech, COLORSBLACK, TRUE);
		ttyputverbose(M_("Black-background colormap loaded"));
		return;
	}

	if (namesamen(pp, x_("white-background"), l) == 0 && l >= 1)
	{
		us_getcolormap(el_curtech, COLORSWHITE, TRUE);
		ttyputverbose(M_("White-background colormap loaded"));
		return;
	}

	if (namesamen(pp, x_("default"), l) == 0 && l >= 1)
	{
		us_getcolormap(el_curtech, COLORSDEFAULT, TRUE);
		ttyputverbose(M_("Default colormap loaded"));
		return;
	}

	if (namesamen(pp, x_("highlight"), l) == 0 && l >= 1)
	{
		if (us_needwindow()) return;
		if (count < 2)
		{
			ttyputusage(x_("color highlight LAYERLETTERS [AMOUNT]"));
			return;
		}
		amt = 0.2f;
		if (count >= 3)
		{
			amt = (float)atofr(par[2]);
			amt /= WHOLE;
		}
		if (amt < 0.0 || amt > 1.0)
		{
			us_abortcommand(_("Highlight amount must be from 0 to 1"));
			return;
		}

		/* check the list of letters */
		for(k=0; us_overlappables[k].bits != 0; k++)
			us_overlappables[k].set = 0;
		for(i=0; par[1][i] != 0 && par[1][i] != '('; i++)
		{
			for(j=0; j<el_curtech->layercount; j++)
			{
				la = us_layerletters(el_curtech, j);
				for(k=0; la[k] != 0; k++) if (par[1][i] == la[k]) break;
				if (la[k] != 0) break;
			}
			if (j >= el_curtech->layercount)
			{
				us_abortcommand(_("Unknown layer letter: %c"), par[1][i]);
				return;
			}
			for(k=0; us_overlappables[k].bits != 0; k++)
				if (el_curtech->layers[j]->bits == us_overlappables[k].bits) break;
			if (us_overlappables[k].bits == 0)
			{
				us_abortcommand(_("Layer %s(%c) is not transparent"), layername(el_curtech, j), par[1][i]);
				return;
			}
			us_overlappables[k].set = 1;
		}

		/* get the color map */
		varred = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_red_key);
		vargreen = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_green_key);
		varblue = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_blue_key);
		if (varred == NOVARIABLE || vargreen == NOVARIABLE || varblue == NOVARIABLE)
		{
			ttyputerr(_("Cannot get current color map"));
			return;
		}

		/* copy the color arrays from the variables */
		for(i=0; i<256; i++)
		{
			redt[i] = ((INTBIG *)varred->addr)[i];
			greent[i] = ((INTBIG *)vargreen->addr)[i];
			bluet[i] = ((INTBIG *)varblue->addr)[i];
		}

		/* adjust the color arrays */
		for(i=1; i<256; i++)
		{
			/* ignore if nonoverlappable, grid, or highlight */
			if ((i&(LAYERG|LAYERH|LAYEROE)) != 0) continue;

			/* skip if it is one of the highlighted layers */
			for(k=0; us_overlappables[k].bits != 0; k++)
				if ((us_overlappables[k].bits&i) != 0 && us_overlappables[k].set != 0) break;
			if (us_overlappables[k].bits != 0) continue;

			/* dim the layer */
			us_rgbtohsv(redt[i], greent[i], bluet[i], &hue, &sat, &inten);
			sat *= amt;
			us_hsvtorgb(hue, sat, inten, &rr, &rg, &rb);
			redt[i] = rr;   greent[i] = rg;   bluet[i] = rb;
		}

		/* set the new color map */
		startobjectchange((INTBIG)us_tool, VTOOL);
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_colormap_red_key, (INTBIG)redt,
			VINTEGER|VISARRAY|(256<<VLENGTHSH));
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_colormap_green_key, (INTBIG)greent,
			VINTEGER|VISARRAY|(256<<VLENGTHSH));
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_colormap_blue_key, (INTBIG)bluet,
			VINTEGER|VISARRAY|(256<<VLENGTHSH));
		endobjectchange((INTBIG)us_tool, VTOOL);
		return;
	}

	if ((namesamen(pp, x_("entry"), l) == 0 ||
		namesamen(pp, x_("pattern"), l) == 0) && l >= 1)
	{
		/* force pattern references on black&white display */
		style = *pp;
		if (us_needwindow()) return;

		/* get the entry number */
		if (count < 2)
		{
			count = ttygetparam(M_("Entry: "), &us_colorentryp, MAXPARS-1, &par[1]) + 1;
			if (count == 1)
			{
				us_abortedmsg();
				return;
			}
		}

		if (style == 'p')
		{
			/* pattern specification */
			j = el_curtech->layercount;
			totalcolor = us_getcolormapentry(par[1], TRUE);
		} else
		{
			/* color map entry specification */
			j = el_maplength;
			totalcolor = us_getcolormapentry(par[1], FALSE);
		}
		if (totalcolor < 0) return;
		if (totalcolor >= j)
		{
			us_abortcommand(_("Entry must be from 0 to %ld"), j-1);
			return;
		}

		/* get color arrays */
		varred = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_red_key);
		vargreen = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_green_key);
		varblue = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_blue_key);
		if (varred == NOVARIABLE || vargreen == NOVARIABLE || varblue == NOVARIABLE)
		{
			ttyputerr(_("Cannot get current color map"));
			return;
		}

		/* if more values specified, get new entry */
		if (count >= 3)
		{
			if (style == 'p')
			{
				/* get a stipple pattern for this layer */
				desc = el_curtech->layers[totalcolor];
				pp = par[2];
				for(j=0; j<8; j++)
				{
					if (*pp == 0) pp = par[2];
					if (*pp == '/')
					{
						pp++;
						continue;
					}
					newraster = myatoi(pp);
					(void)setind((INTBIG)desc, VGRAPHICS, x_("raster"), j, newraster);
					while (*pp != '/' && *pp != 0) pp++;
					if (*pp == '/') pp++;
				}
			} else
			{
				/* get three colors for the map */
				if (count < 5)
				{
					ttyputusage(x_("color entry INDEX [RED GREEN BLUE]"));
					return;
				}
				us_setcolorentry(totalcolor, myatoi(par[2]), myatoi(par[3]),
					myatoi(par[4]), 0, TRUE);
			}
		}

		/* report the current color values */
		if (style == 'p')
		{
			ttyputmsg(_("Entry %ld (%s) is bits:"), totalcolor,
				layername(el_curtech, totalcolor));
			desc = el_curtech->layers[totalcolor];
			for(j=0; j<8; j++)
			{
				(void)esnprintf(line, MAXLINE, x_("0x%04x |"), desc->raster[j]&0xFFFF);
				for(k=0; k<16; k++)
					if ((desc->raster[j] & (1<<(15-k))) != 0)
						(void)estrcat(line, x_("X")); else
							(void)estrcat(line, x_(" "));
				(void)estrcat(line, x_("|"));
				ttyputmsg(x_("%s"), line);
			}
		} else us_printcolorvalue(totalcolor);
		return;
	}

	if (namesamen(pp, x_("mix"), l) == 0 && l >= 1)
	{
		if (us_needwindow()) return;
		for(i=0; i<5; i++) layerlabel[i] = 0;
		j = el_curtech->layercount;
		for(i=0; i<j; i++)
		{
			desc = el_curtech->layers[i];
			switch (desc->bits)
			{
				case LAYERT1: k = 0;   break;
				case LAYERT2: k = 1;   break;
				case LAYERT3: k = 2;   break;
				case LAYERT4: k = 3;   break;
				case LAYERT5: k = 4;   break;
				default:      k = -1;  break;
			}
			if (k < 0) continue;
			if (layerlabel[k] != 0) continue;
			layerlabel[k] = layername(el_curtech, i);
			layerabbrev[k] = (CHAR *)emalloc(2 * SIZEOFCHAR, el_tempcluster);
			layerabbrev[k][0] = *us_layerletters(el_curtech, i);
			layerabbrev[k][1] = 0;
		}
		layerdata = us_getprintcolors(el_curtech);
		if (layerdata == 0)
		{
			ttyputerr(_("Cannot get print colors for technology '%s'"), el_curtech->techname);
			return;
		}
		for(i=0; i<5; i++) if (layerlabel[i] == 0)
		{
			layerlabel[i] = x_("UNUSED");
			(void)allocstring(&layerabbrev[i], x_("?"), el_tempcluster);
		}
		layernames = (CHAR **)emalloc(el_curtech->layercount*(sizeof (CHAR *)), el_tempcluster);
		if (layernames == 0) return;
		for(i=0; i<el_curtech->layercount; i++)
			layernames[i] = layername(el_curtech, i);
		if (us_colormixdlog(layerlabel, el_curtech->layercount, layernames, layerdata))
		{
			/* data changed: update the print colors (other information is already set) */
			setval((INTBIG)el_curtech, VTECHNOLOGY, x_("USER_print_colors"), (INTBIG)layerdata,
				VINTEGER|VISARRAY|((el_curtech->layercount*5)<<VLENGTHSH));
		}
		for(i=0; i<5; i++) efree(layerabbrev[i]);
		efree((CHAR *)layernames);
		return;
	}

	if (namesamen(pp, x_("read"), l) == 0 && l >= 1)
	{
		if (count < 2)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("color/"));
			addstringtoinfstr(infstr, _("Color Map File"));
			count = ttygetparam(returninfstr(infstr), &us_colorreadp, MAXPARS-1, &par[1]) + 1;
			if (count == 1)
			{
				us_abortedmsg();
				return;
			}
		}
		orig = par[1];

		/* get color map from file */
		f = xopen(orig, us_filetypecolormap, el_libdir, &filename);
		if (f == NULL)
		{
			us_abortcommand(_("Cannot find %s"), orig);
			return;
		}
		ttyputmsg(_("Reading %s"), orig);

		/* see if technology name is right */
		(void)xfgets(line, MAXLINE, f);
		if (estrncmp(line, x_("technology="), 11) != 0)
		{
			us_abortcommand(_("Invalid color map file"));
			xclose(f);
			return;
		}
		pp = &line[11];
		if (estrcmp(pp, el_curtech->techname) != 0)
			ttyputverbose(M_("Warning: color map is for %s technology"), pp);

		max = 0;
		for(;;)
		{
			if (xfgets(line, MAXLINE, f))
			{
				us_abortcommand(_("Map ends prematurely"));
				return;
			}
			(void)estrcat(line, x_("\t\t\t\t"));
			pp = line;   red = eatoi(pp);
			while (*pp != '\t' && *pp != 0) pp++;  green = eatoi(++pp);
			while (*pp != '\t' && *pp != 0) pp++;  blue = eatoi(++pp);
			while (*pp != '\t' && *pp != 0) pp++;  newmax = eatoi(++pp);
			if (newmax < max || newmax >= 256)
			{
				us_abortcommand(_("Bad map indices: %s"), line);
				xclose(f);
				return;
			}
			for(; max <= newmax; max++)
			{
				redt[max] = red&0377;
				greent[max] = green&0377;
				bluet[max] = blue&0377;
			}
			if (max >= 256) break;
		}

		/* set the color map */
		startobjectchange((INTBIG)us_tool, VTOOL);
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_colormap_red_key, (INTBIG)redt,
			VINTEGER|VISARRAY|(256<<VLENGTHSH));
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_colormap_green_key, (INTBIG)greent,
			VINTEGER|VISARRAY|(256<<VLENGTHSH));
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_colormap_blue_key, (INTBIG)bluet,
			VINTEGER|VISARRAY|(256<<VLENGTHSH));
		endobjectchange((INTBIG)us_tool, VTOOL);

		/* now get the bit-map information */
		if (xfgets(line, MAXLINE, f))
		{
			ttyputmsg(_("Warning: no bit-map information in file"));
			return;
		}
		j = eatoi(line);
		high = el_curtech->layercount;
		for(i=0; i<j; i++)
		{
			if (xfgets(line, MAXLINE, f))
			{
				ttyputmsg(_("Warning: EOF during bit-map data"));
				break;
			}
			for(k=0; k<high; k++)
			{
				pp = us_layerletters(el_curtech, k);
				for(l=0; pp[l] != 0; l++) if (pp[l] == line[0]) break;
				if (pp[l] != 0)
				{
					desc = el_curtech->layers[k];
					pp = &line[1];
					for(l=0; l<8; l++)
					{
						while (*pp != ' ' && *pp != 0) pp++;
						newraster = myatoi(++pp);
						(void)setind((INTBIG)desc, VGRAPHICS, x_("raster"), l, newraster);
					}
					break;
				}
			}
			if (k >= high) ttyputmsg(_("Warning: no layer '%c' in this technology"), line[0]);
		}
		xclose(f);

		ttyputmsg(_("Color map in %s read"), orig);
		return;
	}

	if (namesamen(pp, x_("write"), l) == 0 && l >= 1)
	{
		if (count < 2)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("color/"));
			addstringtoinfstr(infstr, _("Color map file name: "));
			count = ttygetparam(returninfstr(infstr), &us_colorwritep, MAXPARS-1, &par[1]) + 1;
			if (count == 1)
			{
				us_abortedmsg();
				return;
			}
		}
		orig = par[1];

		/* get color arrays */
		varred = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_red_key);
		vargreen = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_green_key);
		varblue = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_blue_key);
		if (varred == NOVARIABLE || vargreen == NOVARIABLE || varblue == NOVARIABLE)
		{
			ttyputerr(_("Cannot get current color map"));
			return;
		}

		/* write color map to file */
		f = xcreate(orig, us_filetypecolormap, _("Color Map File"), &truename);
		if (f == NULL)
		{
			if (truename != 0) us_abortcommand(_("Cannot write %s"), truename);
			return;
		}
		xprintf(f, x_("technology=%s\n"), el_curtech->techname);
		for(i=0; i<256; i++)
			if (i == 255 || ((INTBIG *)varred->addr)[i] != ((INTBIG *)varred->addr)[i+1] ||
				((INTBIG *)vargreen->addr)[i] != ((INTBIG *)vargreen->addr)[i+1] ||
					((INTBIG *)varblue->addr)[i] != ((INTBIG *)varblue->addr)[i+1])
		{
			xprintf(f, x_("%ld\t%ld\t%ld\t%ld\n"), ((INTBIG *)varred->addr)[i],
				((INTBIG *)vargreen->addr)[i], ((INTBIG *)varblue->addr)[i], i);
		}

		/* write the bit maps */
		high = el_curtech->layercount;
		xprintf(f, x_("%ld\n"), high);
		for(i=0; i<high; i++)
		{
			desc = el_curtech->layers[i];
			s = us_layerletters(el_curtech, i);
			xprintf(f, x_("%c"), s[0]);
			for(j=0; j<8; j++)
				xprintf(f, x_(" 0%o"), desc->raster[j] & 0xFFFF);
			xprintf(f, x_("\n"));
		}
		xclose(f);
		ttyputverbose(M_("%s written"), truepath(orig));
		return;
	}

	ttyputbadusage(x_("color"));
}

void us_commandfile(INTBIG count, CHAR *par[])
{
	REGISTER FILE *in;
	REGISTER BOOLEAN verbose;
	REGISTER CHAR *pp;
	CHAR *filename;
	REGISTER MACROPACK *lastmacropack;
	extern COMCOMP us_colorreadp;
	REGISTER void *infstr;

	if (count >= 2 && namesamen(par[1], x_("verbose"), estrlen(par[1])) == 0) verbose = TRUE; else
		verbose = FALSE;

	if (count == 0)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("macro/"));
		addstringtoinfstr(infstr, _("Command File: "));
		count = ttygetparam(returninfstr(infstr), &us_colorreadp, MAXPARS, par);
		if (count == 0)
		{
			us_abortedmsg();
			return;
		}
	}
	pp = par[0];

	in = xopen(pp, us_filetypemacro, el_libdir, &filename);
	if (in == NULL)
	{
		us_abortcommand(_("Command file %s not found"), pp);
		return;
	}

	/* create a new macro package entry */
	lastmacropack = us_curmacropack;
	us_curmacropack = us_newmacropack(pp);

	/* read the commands */
	us_docommands(in, verbose, x_(""));
	xclose(in);

	/* now there is no macro package */
	us_curmacropack = lastmacropack;
}

void us_constraint(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG l, c;
	REGISTER CHAR *pp;

	/* get the name of the constraint solver */
	if (count <= 1)
	{
		ttyputusage(x_("constraint (use | tell) SOLVER [OPTIONS]"));
		return;
	}
	for(c=0; el_constraints[c].conname != 0; c++)
		if (namesame(el_constraints[c].conname, par[1]) == 0) break;
	if (el_constraints[c].conname == 0)
	{
		us_abortcommand(_("No constraint solver called %s"), par[1]);
		return;
	}

	l = estrlen(pp = par[0]);

	if (namesamen(pp, x_("tell"), l) == 0 && l >= 1)
	{
		(*(el_constraints[c].setmode))(count-2, &par[2]);
		return;
	}

	if (namesamen(pp, x_("use"), l) == 0 && l >= 1)
	{
		if (el_curconstraint == &el_constraints[c])
		{
			ttyputerr(_("Already using that constraint solver"));
			return;
		}
		us_clearhighlightcount();
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_constraint_key,
			(INTBIG)&el_constraints[c], VCONSTRAINT|VDONTSAVE);
		ttyputmsg(M_("Switching to %s"), TRANSLATE(el_constraints[c].condesc));
		return;
	}

	ttyputbadusage(x_("constraint"));
}

void us_copycell(INTBIG count, CHAR *par[])
{
	REGISTER NODEPROTO *np, *onp, *oinp;
	REGISTER LIBRARY *fromlib, *tolib;
	REGISTER CHAR *pt, *ppt, *lastch, save;
	extern COMCOMP us_copycellp;
	REGISTER NODEINST *ni, *newni;
	REGISTER VIEW *nview;
	REGISTER INTBIG i;
	CHAR *subpar[5];
	BOOLEAN quiet, norelatedviews, showcopy, move, nosubcells, newframe, useexisting;
	INTBIG lx, hx, ly, hy;
	REGISTER void *infstr;
	extern COMCOMP us_yesnop;

	/* get name of old cell */
	if (count == 0)
	{
		count = ttygetparam(M_("Old cell name: "), &us_copycellp, MAXPARS, par);
		if (count == 0)
		{
			us_abortedmsg();
			return;
		}
	}
	np = getnodeproto(par[0]);
	if (np == NONODEPROTO)
	{
		us_abortcommand(_("Cannot find source cell %s"), par[0]);
		return;
	}
	if (np->primindex != 0)
	{
		us_abortcommand(_("Cannot copy primitives"));
		return;
	}
	fromlib = np->lib;
	tolib = fromlib;

	/* get name of new cell */
	newframe = showcopy = quiet = norelatedviews = move = nosubcells = useexisting = FALSE;
	if (count <= 1) pt = np->protoname; else
	{
		/* if there is a version number in the new name, remove it */
		for(ppt = par[1]; *ppt != 0; ppt++) if (*ppt == ';') break;
		if (*ppt != 0)
		{
			infstr = initinfstr();
			*ppt = 0;
			addstringtoinfstr(infstr, par[1]);
			*ppt++ = ';';
			for( ; *ppt != 0; ppt++) if (*ppt == '{') break;
			addstringtoinfstr(infstr, ppt);
			par[1] = returninfstr(infstr);
		}
		pt = par[1];

		for(ppt = pt; *ppt != 0; ppt++) if (*ppt == ':') break;
		if (*ppt == ':')
		{
			*ppt++ = 0;
			tolib = getlibrary(pt);
			if (tolib == NOLIBRARY)
			{
				us_abortcommand(_("Unknown destination library: %s"), pt);
				return;
			}
			pt = ppt;
		}
		if (namesame(pt, x_("{ic}")) == 0 || namesame(pt, x_("{sk}")) == 0)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, np->protoname);
			addstringtoinfstr(infstr, par[1]);
			pt = returninfstr(infstr);
		}
		while (count > 2)
		{
			i = estrlen(ppt = par[2]);
			if (namesamen(ppt, x_("quiet"), i) == 0) quiet = TRUE; else
			if (namesamen(ppt, x_("move"), i) == 0) move = TRUE; else
			if (namesamen(ppt, x_("replace-copy"), i) == 0) showcopy = TRUE; else
			if (namesamen(ppt, x_("show-copy"), i) == 0) showcopy = newframe = TRUE; else
			if (namesamen(ppt, x_("no-related-views"), i) == 0 && i >= 4) norelatedviews = TRUE; else
			if (namesamen(ppt, x_("no-subcells"), i) == 0 && i >= 4) nosubcells = TRUE; else
			if (namesamen(ppt, x_("use-existing-subcells"), i) == 0) useexisting = TRUE; else
			{
				ttyputbadusage(x_("copycell"));
				return;
			}
			count--;
			par++;
		}
	}

	/* see if icon generation is requested */
	i = estrlen(pt);
	if (count > 1 && i > 4 && namesame(&pt[i-4], x_("{ic}")) == 0 && np->cellview != el_iconview)
	{
		/* cannot iconize text-only views */
		if ((np->cellview->viewstate&TEXTVIEW) != 0)
		{
			us_abortcommand(_("Cannot iconize textual views: only layout and schematic"));
			return;
		}

		/* cannot cross libraries when iconizing */
		if (fromlib != tolib)
		{
			us_abortcommand(_("Can only generate icons within the same library"));
			return;
		}

		/* generate the icon */
		onp = us_makeiconcell(np->firstportproto, np->protoname, pt, tolib);
		if (onp == NONODEPROTO) return;
		if (!quiet)
			ttyputmsg(_("Cell %s created with an iconic representation of %s"),
				describenodeproto(onp), describenodeproto(np));
		if (showcopy)
		{
			us_fullview(onp, &lx, &hx, &ly, &hy);
			if (el_curwindowpart == NOWINDOWPART) newframe = TRUE;
			us_switchtocell(onp, lx, hx, ly, hy, NONODEINST, NOPORTPROTO, newframe, FALSE, FALSE);
		}
		return;
	}

	/* see if skeletonization is requested */
	i = estrlen(pt);
	if (count > 1 && i > 4 && namesame(&pt[i-4], x_("{sk}")) == 0 && np->cellview != el_skeletonview)
	{
		onp = us_skeletonize(np, pt, tolib, quiet);
		if (onp == NONODEPROTO) return;
		if (showcopy)
		{
			us_fullview(onp, &lx, &hx, &ly, &hy);
			if (el_curwindowpart == NOWINDOWPART) newframe = TRUE;
			us_switchtocell(onp, lx, hx, ly, hy, NONODEINST, NOPORTPROTO, newframe, FALSE, FALSE);
		}
		return;
	}

	/* see if the name already exists in the destination library */
	for(onp = tolib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
	{
		if (namesame(onp->protoname, pt) != 0) continue;
		if (np->cellview == onp->cellview) break;
	}
	if (onp != NONODEPROTO)
	{
		infstr = initinfstr();
		formatinfstr(infstr, _("Cell %s already exists; create a new version?"), describenodeproto(onp));
		i = ttygetparam(returninfstr(infstr), &us_yesnop, MAXPARS, subpar);
		if (i > 0 && namesamen(subpar[0], x_("no"), estrlen(subpar[0])) == 0) return;
	}

	/* if going within the same library, let the database do it */
	if (fromlib == tolib)
	{
		onp = copynodeproto(np, tolib, pt, FALSE);
		if (onp == NONODEPROTO)
		{
			ttyputerr(_("Error copying cell"));
			return;
		}
		if (onp->cellview != np->cellview || !insamecellgrp(onp, np))
		{
			if (!quiet)
				ttyputmsg(_("Cell %s copied to %s"), describenodeproto(np),
					describenodeproto(onp));

			/* see if renaming a schematic with an icon of itself inside */
			for(ppt = pt; *ppt != 0; ppt++) if (*ppt == '{') break;
			if (*ppt == 0)
			{
				/* just a rename, not to a different view type */
				for(ni = onp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				{
					if (ni->proto->primindex != 0) continue;
					if (isiconof(ni->proto, np)) break;
				}
				if (ni != NONODEINST)
				{
					/* need a properly-named icon: see if it exists */
					infstr = initinfstr();
					formatinfstr(infstr, x_("%s:%s{ic}"), tolib->libname, pt);
					oinp = getnodeproto(returninfstr(infstr));
					if (oinp == NONODEPROTO)
					{
						/* duplicate the icon and replace the old one with the duplicate */
						oinp = copynodeproto(ni->proto, tolib, pt, FALSE);
					}
					if (oinp == NONODEPROTO)
					{
						ttyputerr(_("Error duplicating icon node %s to %s"),
							describenodeinst(ni), pt);
					} else
					{
						startobjectchange((INTBIG)ni, VNODEINST);
						newni = replacenodeinst(ni, oinp, TRUE, TRUE);
						if (newni != NONODEINST)
							endobjectchange((INTBIG)newni, VNODEINST);
					}
				}
			}
		} else
		{
			if (!quiet)
				ttyputmsg(_("New version of cell %s created, old is %s"),
					describenodeproto(onp), describenodeproto(np));

			/* set current nodeproto to itself to redisplay its name */
			if (np == getcurcell())
				(void)setval((INTBIG)el_curlib, VLIBRARY, x_("curnodeproto"), (INTBIG)np, VNODEPROTO);

			/* update display of any unexpanded old versions */
			for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
			{
				if ((ni->userbits&NEXPAND) != 0) continue;
				startobjectchange((INTBIG)ni, VNODEINST);
				endobjectchange((INTBIG)ni, VNODEINST);
			}
		}
		if (showcopy)
		{
			us_fullview(onp, &lx, &hx, &ly, &hy);
			if (el_curwindowpart == NOWINDOWPART) newframe = TRUE;
			us_switchtocell(onp, lx, hx, ly, hy, NONODEINST, NOPORTPROTO, newframe, FALSE, FALSE);
		}
		return;
	}

	/* get the new view type */
	nview = np->cellview;
	for(ppt = pt; *ppt != 0; ppt++) if (*ppt == '{') break;
	if (*ppt == '{')
	{
		lastch = &pt[estrlen(pt)-1];
		if (*lastch != '}')
		{
			us_abortcommand(_("View name '%s'must end with a '}'"), &ppt[1]);
			return;
		}
		*lastch = 0;
		nview = getview(&ppt[1]);
		*lastch = '}';
		if (nview == NOVIEW)
		{
			us_abortcommand(_("Unknown view abbreviation: %s"), ppt);
			return;
		}
	}

	save = *ppt;
	*ppt = 0;
	onp = us_copyrecursively(np, pt, tolib, nview, TRUE, move, x_(""),
		norelatedviews, nosubcells, useexisting);
	*ppt = save;
	if (showcopy)
	{
		us_fullview(onp, &lx, &hx, &ly, &hy);
		if (el_curwindowpart == NOWINDOWPART) newframe = TRUE;
		us_switchtocell(onp, lx, hx, ly, hy, NONODEINST, NOPORTPROTO, newframe, FALSE, FALSE);
	}
}

void us_create(INTBIG count, CHAR *par[])
{
	REGISTER NODEPROTO *np, *curcell;
	REGISTER NODEINST *ni, *newno, *ni2;
	REGISTER PORTPROTO *pp, *ppt, *fpt, *pp1, *pp2;
	PORTPROTO *fromport, *toport;
	REGISTER ARCPROTO *ap, *wantap;
	REGISTER ARCINST *ai, *newai;
	REGISTER TECHNOLOGY *tech;
	NODEINST *ani[2];
	REGISTER INTBIG ang, pangle, l, bestdist, bestdist1, bestdist2, wid, truewid, i, j, k,
		bits1, bits2, dist, alignment, edgealignment;
	BOOLEAN remainhighlighted, ishor, isver, join, angledcreate,
		waitfordown, centeredprimitives, getcontents;
	INTBIG px, py, nx, ny, cx, cy, x, y, xp[2], yp[2], xcur, ycur, otheralign, pxs, pys,
		lx, hx, ly, hy;
	PORTPROTO *app[2], *splitpp;
	REGISTER VARIABLE *var;
	static POLYGON *poly = NOPOLYGON, *poly2 = NOPOLYGON;
	ARCINST *alt1, *alt2;
	NODEINST *con1, *con2;
	CHAR *oldarcname, *newpar[2];
	HIGHLIGHT newhigh, *high;
	REGISTER GEOM *foundgeom, *geom, **list;
	GEOM *fromgeom, *togeom;
	extern GRAPHICS us_hbox;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);
	(void)needstaticpolygon(&poly2, 4, us_tool->cluster);

	/* make sure there is a cell being edited */
	if (us_needwindow()) return;
	if ((el_curwindowpart->state&WINDOWTYPE) != DISPWINDOW)
	{
		us_abortcommand(_("Can only place components in graphical editing windows"));
		return;
	}
	curcell = us_needcell();
	if (curcell == NONODEPROTO) return;

	/* in outline edit, create a point */
	if ((el_curwindowpart->state&WINDOWOUTLINEEDMODE) != 0)
	{
		par[0] = x_("trace");
		par[1] = x_("add-point");
		us_node(2, par);
		return;
	}

	if ((us_useroptions&CENTEREDPRIMITIVES) != 0) centeredprimitives = TRUE; else
		centeredprimitives = FALSE;
	join = angledcreate = remainhighlighted = getcontents = FALSE;
	waitfordown = FALSE;
	while (count > 0)
	{
		l = estrlen(par[0]);
		if (namesamen(par[0], x_("remain-highlighted"), l) == 0 && l >= 1)
		{
			remainhighlighted = TRUE;
			count--;
			par++;
			continue;
		}
		if (namesamen(par[0], x_("contents"), l) == 0 && l >= 1)
		{
			getcontents = TRUE;
			count--;
			par++;
			continue;
		}
		if (namesamen(par[0], x_("wait-for-down"), l) == 0 && l >= 1)
		{
			waitfordown = TRUE;
			count--;
			par++;
			continue;
		}
		if ((namesamen(par[0], x_("angle"), l) == 0 ||
			namesamen(par[0], x_("join-angle"), l) == 0) && l >= 1)
		{
			if (namesamen(par[0], x_("join-angle"), l) == 0) join = TRUE;
			angledcreate = TRUE;
			count--;
			par++;
			if (count >= 1)
			{
				ang = atofr(par[0])*10/WHOLE;
				count--;
				par++;
			} else ang = ((us_curarcproto->userbits&AANGLEINC) >> AANGLEINCSH) * 10;
		}
		break;
	}

	/* create a node or arc */
	if (count == 0)
	{
		/* if no "angle" specified, simply create a node */
		if (!angledcreate)
		{
			/* determine the node type being created */
			np = us_nodetocreate(getcontents, curcell);
			if (np == NONODEPROTO) return;

			/* adjust the cursor position if selecting interactively */
			us_clearhighlightcount();
			if ((us_state&NONPERSISTENTCURNODE) != 0) us_setnodeproto(np);

			/* get placement angle */
			pangle = us_getplacementangle(np);

			if ((us_tool->toolstate&INTERACTIVE) != 0)
			{
				us_createinit(0, 0, np, pangle, FALSE, NOGEOM, NOPORTPROTO);
				trackcursor(waitfordown, us_ignoreup, us_createbegin, us_dragdown,
					us_stoponchar, us_dragup, TRACKDRAGGING);
				if (el_pleasestop != 0) return;
			}
			if (us_demandxy(&xcur, &ycur)) return;
			gridalign(&xcur, &ycur, 1, curcell);

			/* get current cell (in case interactive placement changed the window) */
			curcell = us_needcell();

			/* disallow creating if lock is on */
			if (us_cantedit(curcell, NONODEINST, TRUE)) return;

			/* determine size of the nodeinst */
			corneroffset(NONODEINST, np, pangle%3600, pangle/3600, &cx, &cy,
				centeredprimitives);
			defaultnodesize(np, &pxs, &pys);

			/* adjust size if technologies differ */
			us_adjustfornodeincell(np, curcell, &cx, &cy);
			us_adjustfornodeincell(np, curcell, &pxs, &pys);

			xcur -= cx;   ycur -= cy;
			newno = newnodeinst(np, xcur, xcur+pxs, ycur, ycur+pys,
				pangle/3600, pangle%3600, curcell);
			if (newno == NONODEINST)
			{
				us_abortcommand(_("Cannot create node"));
				return;
			}
			if ((np->userbits&WANTNEXPAND) != 0) newno->userbits |= NEXPAND;
			if (np == gen_cellcenterprim) newno->userbits |= HARDSELECTN|NVISIBLEINSIDE; else
				if (np == gen_essentialprim) newno->userbits |= HARDSELECTN;

			/* copy "inheritable" attributes */
			us_inheritattributes(newno);
			endobjectchange((INTBIG)newno, VNODEINST);

			newhigh.status = HIGHFROM;
			newhigh.cell = newno->parent;
			newhigh.fromgeom = newno->geom;
			newhigh.fromport = NOPORTPROTO;
			newhigh.frompoint = 0;
			us_addhighlight(&newhigh);
			return;
		}

		/* see if two objects are selected (and arc should connect them) */
		if (!us_gettwoobjects(&fromgeom, &fromport, &togeom, &toport))
		{
			/* two objects highlighted: create an arc between them */
			if (us_demandxy(&xcur, &ycur)) return;
			gridalign(&xcur, &ycur, 1, geomparent(togeom));
			if (geomparent(fromgeom) != geomparent(togeom))
			{
				us_abortcommand(_("Cannot run an arc between different cells"));
				return;
			}

			if (us_cantedit(curcell, NONODEINST, TRUE)) return;

			/* run an arc from "fromgeom" to "togeom" */
			if (remainhighlighted) us_pushhighlight();
			us_clearhighlightcount();
			if (!angledcreate) ang = 900;
			ai = aconnect(fromgeom, fromport, togeom, toport, us_curarcproto,
				xcur, ycur, &alt1, &alt2, &con1, &con2, ang, FALSE, TRUE);
			if (remainhighlighted) us_pophighlight(FALSE);
			if (ai == NOARCINST) return;

			/* unless indicated, leave only one object highlighted */
			if (!remainhighlighted)
			{
				/* if an arc was highlighted and it got broken, fix the highlight */
				geom = togeom;
				if (!geom->entryisnode && (geom->entryaddr.ai->userbits&DEADA) != 0)
				{
					geom = fromgeom;
					if (!geom->entryisnode && (geom->entryaddr.ai->userbits&DEADA) != 0) return;
				}

				newhigh.status = HIGHFROM;
				newhigh.cell = geomparent(geom);
				newhigh.fromgeom = geom;
				newhigh.fromport = NOPORTPROTO;
				us_addhighlight(&newhigh);
			}
			return;
		}

		/* "create angle": get cursor co-ordinates */
		if (us_demandxy(&xcur, &ycur)) return;
		gridalign(&xcur, &ycur, 1, curcell);

		/* get nodeinst from which to draw */
		list = us_gethighlighted(WANTNODEINST|WANTARCINST, 0, 0);
		if (list[0] == NOGEOM || list[1] != NOGEOM)
		{
			us_abortcommand(_("Must start new arcs from one node or arc"));
			return;
		}
		splitpp = NOPORTPROTO;
		if (list[0]->entryisnode)
		{
			ni = list[0]->entryaddr.ni;
			high = us_getonehighlight();
			if (high != NOHIGHLIGHT && (high->status&HIGHTYPE) == HIGHFROM)
				splitpp = high->fromport;
		} else
		{
			ni = us_getnodeonarcinst(&list[0], &splitpp, list[0], NOPORTPROTO, xcur, ycur, 0);
			if (ni == NONODEINST)
			{
				us_abortcommand(_("Cannot find starting point of new arc"));
				return;
			}
			us_endchanges(NOWINDOWPART);
		}

		/* ensure that cursor is in proper window */
		if (ni->parent != curcell)
		{
			us_abortcommand(_("Cursor must be in the same cell as the highlighted node"));
			return;
		}
		if (us_cantedit(curcell, NONODEINST, TRUE)) return;

		/* find the closest port to the cursor */
		bestdist1 = bestdist2 = MAXINTBIG;
		ppt = fpt = NOPORTPROTO;
		for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			if (splitpp != NOPORTPROTO && pp != splitpp) continue;
			shapeportpoly(ni, pp, poly, FALSE);
			k = polydistance(poly, xcur, ycur);

			/* find the closest port to the cursor */
			if (k < bestdist1)
			{
				bestdist1 = k;
				ppt = pp;  /* pp is the closest port to (xcur, ycur) */
			}

			/* find the closest port that can connect with the current arc */
			if (k < bestdist2)
			{
				for (i=0; pp->connects[i] != NOARCPROTO; i++)
				{
					if (pp->connects[i] == us_curarcproto)
					{
						/* can connect */
						bestdist2 = k;
						fpt = pp;
					}
				}
			}
		}

		/* error if there is no port */
		if (ppt == NOPORTPROTO)
		{
			us_abortcommand(_("Cannot run wires out of this node"));
			return;
		}

		/*
		 * now ppt is the closest port, and fpt is the closest port that can
		 * connect with the current arc.
		 */
		if (fpt == NOPORTPROTO)
		{
			/* no arcproto of correct type: first reset all arcprotos */
			for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
				for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
					ap->userbits &= ~CANCONNECT;

			/* now set all arcs that can connect this node's closest port */
			for(i=0; ppt->connects[i] != NOARCPROTO; i++)
				ppt->connects[i]->userbits |= CANCONNECT;

			/* see if current arcproto is acceptable */
			if ((us_curarcproto->userbits&CANCONNECT) == 0)
			{
				/* search this technology for another arcproto */
				for(ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
				{
					if ((ap->userbits&CANCONNECT) == 0) continue;
					if (getpinproto(ap) == NONODEPROTO) return;
					us_setarcproto(ap, FALSE);
					break;
				}

				/* if nothing in this technology can connect, see what can */
				if (ap == NOARCPROTO && ppt->connects[0] != NOARCPROTO)
				{
					ap = ppt->connects[0];
					if (getpinproto(ap) == NONODEPROTO) return;
					us_setarcproto(ap, FALSE);
				}

				/* error if no acceptable arcprotos */
				if (ap == NOARCPROTO)
				{
					us_abortcommand(_("No ports on this node"));
					return;
				}
			}
		} else ppt = fpt;
		splitpp = ppt;

		/* make sure the selected arc connects to this port */
		wantap = us_curarcproto;
		for(i=0; splitpp->connects[i] != NOARCPROTO; i++)
			if (splitpp->connects[i] == us_curarcproto) break;
		if (splitpp->connects[i] == NOARCPROTO)
		{
			/* cannot: see if anything in this technology can connect */
			for(i=0; splitpp->connects[i] != NOARCPROTO; i++)
				if (splitpp->connects[i]->tech == us_curarcproto->tech) break;
			if (splitpp->connects[i] != NOARCPROTO) wantap = splitpp->connects[i]; else
			{
				/* cannot: see if anything nongeneric can connect */
				for(i=0; splitpp->connects[i] != NOARCPROTO; i++)
					if (splitpp->connects[i]->tech != gen_tech) break;
				if (splitpp->connects[i] != NOARCPROTO) wantap = splitpp->connects[i]; else
				{
					/* cannot: select the first possible arcproto */
					wantap = splitpp->connects[0];
				}
			}
		}

		/* turn off highlighting */
		us_clearhighlightcount();

		/* specify precise location for new port */
		poly->xv[0] = xcur;   poly->yv[0] = ycur;   poly->count = 1;
		shapeportpoly(ni, ppt, poly, 1);

		/* determine pin for making the connection */
		np = getpinproto(wantap);
		if (np == NONODEPROTO) return;

		/* get placement angle */
		pangle = us_getplacementangle(np);

		/* determine location of pin */
		defaultnodesize(np, &pxs, &pys);
		corneroffset(NONODEINST, np, pangle%3600, pangle/3600, &cx, &cy,
			centeredprimitives);

		/* adjust size if technologies differ */
		us_adjustfornodeincell(np, curcell, &cx, &cy);
		us_adjustfornodeincell(np, curcell, &pxs, &pys);
		px = pxs / 2;   py = pys / 2;

		/* adjust the cursor position if selecting interactively */
		switch (poly->style)
		{
			case FILLED:
			case FILLEDRECT:
			case CROSSED:
			case CROSS:
			case BIGCROSS:
			case TEXTCENT:
			case TEXTTOP:
			case TEXTBOT:
			case TEXTLEFT:
			case TEXTRIGHT:
			case TEXTTOPLEFT:
			case TEXTBOTLEFT:
			case TEXTTOPRIGHT:
			case TEXTBOTRIGHT:
			case TEXTBOX:
			case DISC:
				getcenter(poly, &x, &y);
				nx = x;   ny = y;
				gridalign(&nx, &ny, 1, curcell);
				if (nx != x || ny != y)
				{
					if (isinside(nx, ny, poly))
					{
						x = nx;
						y = ny;
					}
				}
				break;

			default:
				x = xcur;   y = ycur;
				closestpoint(poly, &x, &y);
				break;
		}
		if ((us_tool->toolstate&INTERACTIVE) != 0)
		{
			fromgeom = ni->geom;
			us_createinit(x-px+cx, y-py+cy, np, ang, join, fromgeom, ppt);
			trackcursor(waitfordown, us_ignoreup, us_createabegin, us_createadown,
				us_stoponchar, us_createaup, TRACKDRAGGING);
			if (el_pleasestop != 0) return;
			if (us_demandxy(&xcur, &ycur)) return;
			if (join)
			{
				us_createajoinedobject(&togeom, &toport);
				if (togeom != NOGEOM)
				{
					if (us_figuredrawpath(fromgeom, ppt, togeom, toport, &xcur, &ycur))
						return;
					us_clearhighlightcount();
					ai = aconnect(fromgeom, ppt, togeom, toport, wantap,
						xcur, ycur, &alt1, &alt2, &con1, &con2, ang, FALSE, TRUE);
					if (ai == NOARCINST) return;

					/* if an arc was highlighted and it got broken, fix the highlight */
					if (!togeom->entryisnode && (togeom->entryaddr.ai->userbits&DEADA) != 0)
					{
						togeom = fromgeom;
						if (!togeom->entryisnode && (togeom->entryaddr.ai->userbits&DEADA) != 0) return;
					}

					newhigh.status = HIGHFROM;
					newhigh.cell = geomparent(togeom);
					newhigh.fromgeom = togeom;
					newhigh.fromport = toport;
					us_addhighlight(&newhigh);
					return;
				}
			}
			gridalign(&xcur, &ycur, 1, curcell);
		}

		/* determine the proper point on the polygon to draw radials */
		us_getslide(ang, x-px+cx, y-py+cy, xcur, ycur, &nx, &ny);

		/* adjust for the box corner */
		nx -= cx;
		ny -= cy;

		/* handle connection if not interactive */
		if ((us_tool->toolstate&INTERACTIVE) == 0)
		{
			/* find the object under the cursor */
			if (!join) foundgeom = NOGEOM; else
			{
				alignment = muldiv(us_alignment_ratio, el_curlib->lambda[el_curtech->techindex], WHOLE);
				foundgeom = us_getclosest(nx+px, ny+py, alignment/2, ni->parent);
			}
			if (foundgeom != NOGEOM)
			{
				/* found another object: connect to it */
				fpt = ppt = NOPORTPROTO;
				bestdist = bestdist2 = MAXINTBIG;
				if (foundgeom->entryisnode)
				{
					/* find the closest port */
					ni = foundgeom->entryaddr.ni;
					for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
					{
						shapeportpoly(ni, pp, poly, FALSE);
						dist = polydistance(poly, xcur, ycur);

						/* find the closest port to the cursor */
						if (dist < bestdist)
						{
							bestdist = dist;
							fpt = pp;
						}

						/* find the closest port that can connect with the current arc */
						if (dist < bestdist2)
						{
							for (i=0; pp->connects[i] != NOARCPROTO; i++)
							{
								if (pp->connects[i] == wantap)
								{
									/* can connect */
									bestdist2 = dist;
									ppt = pp;
								}
							}
						}
					}
					if (fpt != ppt)
					{
						/* the closest port cannot connect, but maybe it can with a different arc */
						for(i=0; fpt->connects[i] != NOARCPROTO; i++)
						{
							if (fpt->connects[i]->tech == gen_tech) continue;
							for(j=0; splitpp->connects[j] != NOARCPROTO; j++)
							{
								if (fpt->connects[i] == splitpp->connects[j]) break;
							}
							if (splitpp->connects[j] != NOARCPROTO) break;
						}
						if (fpt->connects[i] != NOARCPROTO)
						{
							us_setarcproto(fpt->connects[i], FALSE);
							ppt = fpt;
						}
					}
					if (ppt != NOPORTPROTO) fpt = ppt;
				}
				(void)aconnect(ni->geom, splitpp, foundgeom,
					fpt, wantap, xcur, ycur, &alt1, &alt2, &con1, &con2, ang, TRUE, TRUE);

				/* if an arc will be highlighted and it broke, fix the highlight */
				if (!foundgeom->entryisnode &&
					(foundgeom->entryaddr.ai->userbits&DEADA) != 0) return;

				/* highlight the found object */
				newhigh.status = HIGHFROM;
				newhigh.cell = ni->parent;
				newhigh.fromgeom = foundgeom;
				newhigh.fromport = fpt;
				newhigh.frompoint = 0;
				us_addhighlight(&newhigh);
				return;
			}
		}

		/* determine arc width */
		truewid = maxi(defaultarcwidth(wantap), us_widestarcinst(wantap, ni, ppt));
		i = us_stretchtonodes(wantap, truewid, ni, ppt, xcur, ycur);
		if (i > truewid) truewid = i;
		wid = truewid - arcprotowidthoffset(wantap);

		/* see if arc edge alignment is appropriate */
		if (us_edgealignment_ratio != 0 && (x == nx || y == ny))
		{
			edgealignment = muldiv(us_edgealignment_ratio, WHOLE, el_curlib->lambda[el_curtech->techindex]);
			if (x != nx) isver = FALSE; else
			{
				isver = TRUE;
				ny = us_alignvalue(ny + wid/2, edgealignment, &otheralign) - wid/2;
			}
			if (y != ny) ishor = FALSE; else
			{
				ishor = TRUE;
				nx = us_alignvalue(nx + wid/2, edgealignment, &otheralign) - wid/2;
			}

			shapeportpoly(ni, ppt, poly, FALSE);
			i = us_alignvalue(x + wid/2, edgealignment, &otheralign) - wid/2;
			otheralign -= wid/2;
			if (isinside(i, y, poly))
			{
				if (isver) nx = i;
				x = i;
			} else if (isinside(otheralign, y, poly))
			{
				if (isver) nx = otheralign;
				x = otheralign;
			}

			i = us_alignvalue(y + wid/2, edgealignment, &otheralign) - wid/2;
			otheralign -= wid/2;
			if (isinside(x, i, poly))
			{
				if (ishor) ny = i;
				y = i;
			} else if (isinside(x, otheralign, poly))
			{
				if (ishor) ny = otheralign;
				y = otheralign;
			}
		}

		/* create the pin and the arcinst to it */
		defaultnodesize(np, &pxs, &pys);
		us_adjustfornodeincell(np, curcell, &pxs, &pys);
		newno = newnodeinst(np, nx, nx+pxs, ny, ny+pys,
			pangle/3600, pangle%3600, curcell);
		if (newno == NONODEINST)
		{
			us_abortcommand(_("Cannot create pin"));
			return;
		}
		nx += px;   ny += py;
		if ((np->userbits&WANTNEXPAND) != 0) newno->userbits |= NEXPAND;
		endobjectchange((INTBIG)newno, VNODEINST);
		if (((ppt->userbits&PORTARANGE) >> PORTARANGESH) == 180)
		{
			ai = us_runarcinst(ni, ppt, x, y, newno,
				newno->proto->firstportproto, nx, ny, wantap, truewid);
			if (ai != NOARCINST) us_reportarcscreated(0, 1, wantap->protoname, 0, x_(""));
		} else
		{
			k = us_bottomrecurse(ni, ppt) % 3600;
			if (k < 0) k += 3600;
			xcur = x + mult(cosine(k), ni->highx-ni->lowx);
			ycur = y + mult(sine(k), ni->highy-ni->lowy);
			(void)aconnect(ni->geom, ppt, newno->geom, newno->proto->firstportproto,
				wantap, xcur, ycur, &alt1, &alt2, &con1, &con2, ang, TRUE, TRUE);
		}

		newhigh.status = HIGHFROM;
		newhigh.cell = newno->parent;
		newhigh.fromgeom = newno->geom;
		newhigh.fromport = NOPORTPROTO;
		newhigh.frompoint = 0;
		us_addhighlight(&newhigh);
		return;
	}

	/* handle the creation of an icon */
	if (namesamen(par[0], x_("icon"), l) == 0 && l >= 2)
	{
		/* find the icon for the current cell */
		np = iconview(curcell);
		if (np == NONODEPROTO)
		{
			us_abortcommand(_("No icon view for cell %s"),
				describenodeproto(curcell));
			return;
		}

		/* make sure there isn't already one of these */
		for(ni = curcell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			if (ni->proto == np) break;
		if (ni != NONODEINST) return;

		var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_icon_style"));
		if (var != NOVARIABLE) j = var->addr; else j = ICONSTYLEDEFAULT;
		i = (j & ICONINSTLOC) >> ICONINSTLOCSH;
		px = np->highx - np->lowx;
		py = np->highy - np->lowy;
		switch (i)
		{
			case 0:		/* upper-right */
				lx = curcell->highx - px;
				ly = curcell->highy - py;
				break;
			case 1:		/* upper-left */
				lx = curcell->lowx;
				ly = curcell->highy - py;
				break;
			case 2:		/* lower-right */
				lx = curcell->highx - px;
				ly = curcell->lowy;
				break;
			case 3:		/* lower-left */
				lx = curcell->lowx;
				ly = curcell->lowy;
				break;
		}
		gridalign(&lx, &ly, 1, curcell);
		hx = lx + px;   hy = ly + py;
		ni = newnodeinst(np, lx, hx, ly, hy, 0, 0, curcell);
		if (ni != NONODEINST)
		{
			ni->userbits |= NEXPAND;
			us_inheritattributes(ni);
			endobjectchange((INTBIG)ni, VNODEINST);
			us_clearhighlightcount();
			newhigh.status = HIGHFROM;
			newhigh.cell = curcell;
			newhigh.fromgeom = ni->geom;
			newhigh.fromport = NOPORTPROTO;
			newhigh.frompoint = 0;
			us_addhighlight(&newhigh);
			if (lx > el_curwindowpart->screenhx || hx < el_curwindowpart->screenlx ||
				ly > el_curwindowpart->screenhy || hy < el_curwindowpart->screenly)
			{
				newpar[0] = x_("center-highlight");
				us_window(1, newpar);
			}
		}
		return;
	}

	/* handle the absolute placement option */
	if (namesamen(par[0], x_("to"), l) == 0 && l >= 1)
	{
		/* get the location */
		if (count < 3)
		{
			ttyputusage(x_("create to X Y"));
			return;
		}
		nx = atola(par[1], 0);
		ny = atola(par[2], 0);

		/* disallow copying if lock is on */
		np = us_nodetocreate(TRUE, curcell);
		if (np == NONODEPROTO) return;

		if (us_cantedit(curcell, NONODEINST, TRUE)) return;

		/* get placement angle */
		pangle = us_getplacementangle(np);

		corneroffset(NONODEINST, np, pangle%3600, pangle/3600, &cx, &cy,
			centeredprimitives);
		nx -= cx;   ny -= cy;
		defaultnodesize(np, &pxs, &pys);
		newno = newnodeinst(np, nx, nx+pxs, ny, ny+pys,
			pangle/3600, pangle%3600, curcell);
		if (newno == NONODEINST)
		{
			us_abortcommand(_("Cannot create pin"));
			return;
		}
		if ((np->userbits&WANTNEXPAND) != 0) newno->userbits |= NEXPAND;
		if (np == gen_cellcenterprim) newno->userbits |= HARDSELECTN|NVISIBLEINSIDE; else
			if (np == gen_essentialprim) newno->userbits |= HARDSELECTN;
		endobjectchange((INTBIG)newno, VNODEINST);

		us_clearhighlightcount();
		newhigh.status = HIGHFROM;
		newhigh.cell = newno->parent;
		newhigh.fromgeom = newno->geom;
		newhigh.fromport = NOPORTPROTO;
		newhigh.frompoint = 0;
		us_addhighlight(&newhigh);
		return;
	}

	if (namesamen(par[0], x_("insert"), l) == 0 && l >= 2)
	{
		if (us_demandxy(&xcur, &ycur)) return;
		gridalign(&xcur, &ycur, 1, curcell);

		/* get the highlighted arc */
		ai = (ARCINST *)us_getobject(VARCINST, FALSE);
		if (ai == NOARCINST) return;
		ap = ai->proto;

		/* get the node to insert */
		np = us_nodetocreate(TRUE, curcell);
		if (np == NONODEPROTO) return;

		/* disallow creating if lock is on */
		if (us_cantedit(curcell, NONODEINST, TRUE)) return;

		/* turn off highlighting */
		us_clearhighlightcount();
		if ((us_state&NONPERSISTENTCURNODE) != 0) us_setnodeproto(np);

		/* adjust position interactively if possible */
		if ((us_tool->toolstate&INTERACTIVE) != 0)
		{
			us_createinsinit(ai, np);
			trackcursor(waitfordown, us_ignoreup, us_createinsbegin, us_createinsdown,
				us_stoponchar, us_dragup, TRACKDRAGGING);
			if (el_pleasestop != 0) return;
		}
		if (us_demandxy(&xcur, &ycur)) return;
		gridalign(&xcur, &ycur, 1, curcell);

		/* find two ports on this node that connect to the arc */
		pp1 = pp2 = NOPORTPROTO;
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			/* make sure this port can connect to this arc */
			for(i=0; pp->connects[i] != NOARCPROTO; i++)
				if (pp->connects[i] == ap) break;
			if (pp->connects[i] == NOARCPROTO) continue;

			/* save the port */
			if (pp1 == NOPORTPROTO) pp1 = pp; else
				if (pp2 == NOPORTPROTO) pp2 = pp;
		}

		/* make sure there are two ports for connection */
		if (pp1 == NOPORTPROTO)
		{
			us_abortcommand(_("No ports on %s node connect to %s arcs"), describenodeproto(np),
				describearcproto(ap));
			return;
		}
		if (pp2 == NOPORTPROTO) pp2 = pp1;

		/* determine position of the new node */
		ang = ((ai->userbits&AANGLE) >> AANGLESH) * 10;
		(void)intersect(ai->end[0].xpos, ai->end[0].ypos, ang, xcur,
			ycur, (ang+900)%3600, &nx, &ny);
		cx = mini(ai->end[0].xpos, ai->end[1].xpos);
		if (nx < cx) nx = cx;
		cx = maxi(ai->end[0].xpos, ai->end[1].xpos);
		if (nx > cx) nx = cx;
		cy = mini(ai->end[0].ypos, ai->end[1].ypos);
		if (ny < cy) ny = cy;
		cy = maxi(ai->end[0].ypos, ai->end[1].ypos);
		if (ny > cy) ny = cy;
		defaultnodesize(np, &px, &py);
		x = nx - px/2;   y = ny - py/2;

		/* create the new node */
		ni = newnodeinst(np, x,x+px, y,y+py, 0, 0, curcell);
		if (ni == NONODEINST)
		{
			us_abortcommand(_("Cannot create node %s"), describenodeproto(np));
			return;
		}
		endobjectchange((INTBIG)ni, VNODEINST);
		portposition(ni, pp1, &nx, &ny);
		portposition(ni, pp2, &cx, &cy);

		/* see if any rotation needs to be applied to this node */
		if (pp1 != pp2)
		{
			k = figureangle(nx, ny, cx, cy);
			if (k != ang)
			{
				if ((k+1800)%3600 == ang)
				{
					/* simple 180 degree rotation: reverse the ports */
					pp = pp1;   pp1 = pp2;   pp2 = pp;
				} else
				{
					/* complex rotation: rotate the node */
					ang -= k;   if (ang < 0) ang += 3600;
					startobjectchange((INTBIG)ni, VNODEINST);
					modifynodeinst(ni, 0, 0, 0, 0, ang, 0);
					endobjectchange((INTBIG)ni, VNODEINST);
					portposition(ni, pp1, &nx, &ny);
					portposition(ni, pp2, &cx, &cy);
				}
			}
		}

		/* see if node edge alignment is appropriate */
		if (us_edgealignment_ratio != 0 && (ai->end[0].xpos == ai->end[1].xpos ||
			ai->end[0].ypos == ai->end[1].ypos))
		{
			edgealignment = muldiv(us_edgealignment_ratio, WHOLE, el_curlib->lambda[el_curtech->techindex]);
			px = us_alignvalue(x, edgealignment, &otheralign);
			py = us_alignvalue(y, edgealignment, &otheralign);
			if (px != x || py != y)
			{
				/* shift the node and make sure the ports are still valid */
				startobjectchange((INTBIG)ni, VNODEINST);
				modifynodeinst(ni, px-x, py-y, px-x, py-y, 0, 0);
				endobjectchange((INTBIG)ni, VNODEINST);
				(void)shapeportpoly(ni, pp1, poly, FALSE);
				if (!isinside(nx, ny, poly)) getcenter(poly, &nx, &ny);
				(void)shapeportpoly(ni, pp2, poly, FALSE);
				if (!isinside(cx, cy, poly)) getcenter(poly, &cx, &cy);
			}
		}

		/* now save the arc information and delete it */
		for(i=0; i<2; i++)
		{
			xp[i] = ai->end[i].xpos;
			yp[i] = ai->end[i].ypos;
			app[i] = ai->end[i].portarcinst->proto;
			ani[i] = ai->end[i].nodeinst;
		}
		wid = ai->width;
		bits1 = bits2 = ai->userbits;
		if ((bits1&ISNEGATED) != 0)
		{
			if ((bits1&REVERSEEND) == 0) bits2 &= ~ISNEGATED; else
				bits1 &= ~ISNEGATED;
		}
		var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
		if (var == NOVARIABLE) oldarcname = 0; else
		{
			(void)allocstring(&oldarcname, (CHAR *)var->addr, el_tempcluster);
			(void)asktool(net_tool, x_("delname"), (INTBIG)oldarcname,
				(INTBIG)curcell);
		}
		startobjectchange((INTBIG)ai, VARCINST);
		(void)killarcinst(ai);

		/* create the two new arcs */
		newai = newarcinst(ap, wid, bits1, ani[0], app[0], xp[0], yp[0], ni, pp1, nx, ny, curcell);
		if (oldarcname != 0 && newai != NOARCINST)
		{
			(void)asktool(net_tool, x_("setname"), (INTBIG)newai, (INTBIG)oldarcname);
			var = setvalkey((INTBIG)newai, VARCINST, el_arc_name_key, (INTBIG)oldarcname, VSTRING|VDISPLAY);
			if (var != NOVARIABLE)
				defaulttextsize(4, var->textdescript);
			efree(oldarcname);
		}
		if (newai != NOARCINST) endobjectchange((INTBIG)newai, VARCINST);
		newai = newarcinst(ap, wid, bits2, ni, pp2, cx, cy, ani[1],app[1],xp[1],yp[1], curcell);
		if (newai != NOARCINST) endobjectchange((INTBIG)newai, VARCINST);

		/* highlight the node */
		newhigh.status = HIGHFROM;
		newhigh.cell = curcell;
		newhigh.fromgeom = ni->geom;
		newhigh.fromport = NOPORTPROTO;
		newhigh.frompoint = 0;
		us_addhighlight(&newhigh);
		return;
	}

	if (namesamen(par[0], x_("breakpoint"), l) == 0 && l >= 1)
	{
		/* get the highlighted arc */
		ai = (ARCINST *)us_getobject(VARCINST, FALSE);
		if (ai == NOARCINST) return;
		ap = ai->proto;

		/* get the pin to insert */
		np = getpinproto(ap);
		if (np == NONODEPROTO)
		{
			us_abortcommand(_("Cannot determine pin type to insert in arc"));
			return;
		}
		ppt = np->firstportproto;

		if (us_cantedit(curcell, NONODEINST, TRUE)) return;

		/* turn off highlighting */
		us_clearhighlightcount();
		if ((us_state&NONPERSISTENTCURNODE) != 0) us_setnodeproto(np);

		/* adjust position interactively if possible */
		if ((us_tool->toolstate&INTERACTIVE) != 0)
		{
			/* highlight the arc */
			i = ai->width - arcwidthoffset(ai);
			if (curvedarcoutline(ai, poly, OPENEDO1, i))
				makearcpoly(ai->length, i, ai, poly, OPENEDO1);
			if (poly->limit < poly->count+1) (void)extendpolygon(poly, poly->count+1);
			poly->xv[poly->count] = poly->xv[0];
			poly->yv[poly->count] = poly->yv[0];
			poly->count++;
			poly->desc = &us_hbox;
			poly->desc->col = HIGHLIT;
			us_showpoly(poly, el_curwindowpart);

			/* interactively select the break location */
			us_createinsinit(ai, np);
			trackcursor(waitfordown, us_ignoreup, us_createinsbegin, us_createinsdown,
				us_stoponchar, us_dragup, TRACKDRAGGING);

			/* unhighlight the arc */
			poly->desc->col = 0;
			us_showpoly(poly, el_curwindowpart);
			if (el_pleasestop != 0) return;
		}
		if (us_demandxy(&xcur, &ycur)) return;
		gridalign(&xcur, &ycur, 1, curcell);

		/* determine position of the new pins */
		ang = ((ai->userbits&AANGLE) >> AANGLESH) * 10;
		(void)intersect(ai->end[0].xpos, ai->end[0].ypos, ang, xcur, ycur, (ang+900)%3600, &nx, &ny);
		cx = mini(ai->end[0].xpos, ai->end[1].xpos);
		if (nx < cx) nx = cx;
		cx = maxi(ai->end[0].xpos, ai->end[1].xpos);
		if (nx > cx) nx = cx;
		cy = mini(ai->end[0].ypos, ai->end[1].ypos);
		if (ny < cy) ny = cy;
		cy = maxi(ai->end[0].ypos, ai->end[1].ypos);
		if (ny > cy) ny = cy;
		defaultnodesize(np, &px, &py);
		x = nx - px/2;   y = ny - py/2;

		/* create the break pins */
		ni = newnodeinst(np, x,x+px, y,y+py, 0, 0, curcell);
		if (ni == NONODEINST)
		{
			us_abortcommand(_("Cannot create pin %s"), describenodeproto(np));
			return;
		}
		endobjectchange((INTBIG)ni, VNODEINST);
		ni2 = newnodeinst(np, x,x+px, y,y+py, 0, 0, curcell);
		if (ni2 == NONODEINST)
		{
			us_abortcommand(_("Cannot create pin %s"), describenodeproto(np));
			return;
		}
		endobjectchange((INTBIG)ni2, VNODEINST);

		/* get location of connection to these pins */
		portposition(ni, ppt, &nx, &ny);

		/* see if edge alignment is appropriate */
		if (us_edgealignment_ratio != 0 && (ai->end[0].xpos == ai->end[1].xpos ||
			ai->end[0].ypos == ai->end[1].ypos))
		{
			edgealignment = muldiv(us_edgealignment_ratio, WHOLE, el_curlib->lambda[el_curtech->techindex]);
			px = us_alignvalue(x, edgealignment, &otheralign);
			py = us_alignvalue(y, edgealignment, &otheralign);
			if (px != x || py != y)
			{
				/* shift the nodes and make sure the ports are still valid */
				startobjectchange((INTBIG)ni, VNODEINST);
				modifynodeinst(ni, px-x, py-y, px-x, py-y, 0, 0);
				endobjectchange((INTBIG)ni, VNODEINST);
				startobjectchange((INTBIG)ni2, VNODEINST);
				modifynodeinst(ni2, px-x, py-y, px-x, py-y, 0, 0);
				endobjectchange((INTBIG)ni2, VNODEINST);
				(void)shapeportpoly(ni, ppt, poly, FALSE);
				if (!isinside(nx, ny, poly)) getcenter(poly, &nx, &ny);
			}
		}

		/* now save the arc information and delete it */
		for(i=0; i<2; i++)
		{
			xp[i] = ai->end[i].xpos;
			yp[i] = ai->end[i].ypos;
			app[i] = ai->end[i].portarcinst->proto;
			ani[i] = ai->end[i].nodeinst;
		}
		wid = ai->width;
		bits1 = bits2 = ai->userbits;
		if ((bits1&ISNEGATED) != 0)
		{
			if ((bits1&REVERSEEND) == 0) bits2 &= ~ISNEGATED; else
				bits1 &= ~ISNEGATED;
		}
		var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
		if (var == NOVARIABLE) oldarcname = 0; else
		{
			(void)allocstring(&oldarcname, (CHAR *)var->addr, el_tempcluster);
			(void)asktool(net_tool, x_("delname"), (INTBIG)oldarcname,
				(INTBIG)curcell);
		}
		startobjectchange((INTBIG)ai, VARCINST);
		(void)killarcinst(ai);

		/* create the new arcs */
		newai = newarcinst(ap, wid, bits1, ani[0], app[0], xp[0], yp[0], ni, ppt, nx, ny, curcell);
		if (oldarcname != 0 && newai != NOARCINST)
		{
			(void)asktool(net_tool, x_("setname"), (INTBIG)newai, (INTBIG)oldarcname);
			var = setvalkey((INTBIG)newai, VARCINST, el_arc_name_key, (INTBIG)oldarcname, VSTRING|VDISPLAY);
			if (var != NOVARIABLE)
				defaulttextsize(4, var->textdescript);
			efree(oldarcname);
		}
		if (newai != NOARCINST) endobjectchange((INTBIG)newai, VARCINST);
		newai = newarcinst(ap, wid, bits2, ni2, ppt, nx, ny, ani[1],app[1],xp[1],yp[1], curcell);
		if (newai != NOARCINST) endobjectchange((INTBIG)newai, VARCINST);
		bits2 &= ~ISNEGATED;
		ang = (ang + 900) % 3600;
		newai = newarcinst(ap, wid, bits2, ni, ppt, nx, ny, ni2, ppt, nx, ny, curcell);
		if (newai != NOARCINST)
		{
			endobjectchange((INTBIG)newai, VARCINST);

			/* must set the angle explicitly since zero-length arcs have indeterminate angle */
			newai->userbits = (newai->userbits & ~AANGLE) | (((ang+5)/10) << AANGLESH);
		}

		/* highlight the arc */
		newhigh.status = HIGHFROM;
		newhigh.cell = curcell;
		newhigh.fromgeom = newai->geom;
		newhigh.fromport = NOPORTPROTO;
		newhigh.frompoint = 0;
		us_addhighlight(&newhigh);
		return;
	}

	ttyputbadusage(x_("create"));
}

#define DEBUGMERGE 1

#ifdef DEBUGMERGE
INTBIG us_debugmergeoffset;
void us_debugwritepolygon(INTBIG layer, TECHNOLOGY *tech, INTBIG *x, INTBIG *y, INTBIG count);

void us_debugwritepolygon(INTBIG layer, TECHNOLOGY *tech, INTBIG *x, INTBIG *y, INTBIG count)
{
	REGISTER NODEPROTO *cell, *prim;
	REGISTER NODEINST *ni;
	REGISTER INTBIG lx, hx, ly, hy, cx, cy, *newlist;
	REGISTER INTBIG i;
	HIGHLIGHT high;

	prim = getnodeproto(x_("metal-1-node"));
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
	ni = newnodeinst(prim, lx, hx, ly+us_debugmergeoffset, hy+us_debugmergeoffset, 0, 0, cell);
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
#endif

/*#define TESTMODELESS 1*/

#ifdef TESTMODELESS
#include "edialogs.h"
/* test */
static DIALOGITEM us_testdialogitems[] =
{
 /*  1 */ {0, {172,164,196,244}, BUTTON, N_("Done")},
 /*  2 */ {0, {28,8,44,88}, BUTTON, N_("Button")},
 /*  3 */ {0, {28,100,44,216}, CHECK, N_("Check Box")},
 /*  4 */ {0, {56,8,72,124}, RADIO, N_("Radio 1")},
 /*  5 */ {0, {76,8,92,124}, RADIO, N_("Radio 2")},
 /*  6 */ {0, {96,8,112,124}, RADIO, N_("Radio 3")},
 /*  7 */ {0, {132,8,232,132}, SCROLL, x_("")},
 /*  8 */ {0, {4,4,20,220}, MESSAGE, N_("A Modeless Dialog")},
 /*  9 */ {0, {56,140,72,280}, EDITTEXT, x_("")},
 /* 10 */ {0, {120,8,121,288}, DIVIDELINE, x_("")},
 /* 11 */ {0, {93,140,109,280}, POPUP, x_("")}
};
static DIALOG us_testdialog = {{75,75,316,372}, x_("Modeless Dialog"), 0, 11, us_testdialogitems, 0, 0};
void *us_testmodelessdia;
void us_modelessdialoghitroutine(void *dia, INTBIG item);
void us_modelessdialoghitroutine(void *dia, INTBIG item)
{
	if (item == 1)
	{
		DiaDoneDialog(us_testmodelessdia);
		return;
	}
	if (item == 2)
	{
		ttyputmsg(x_("Button clicked"));
		return;
	}
	if (item == 3)
	{
		DiaSetControl(us_testmodelessdia, item, 1-DiaGetControl(us_testmodelessdia, item));
		return;
	}
	if (item == 4 || item == 5 || item == 6)
	{
		DiaSetControl(us_testmodelessdia, 4, 0);
		DiaSetControl(us_testmodelessdia, 5, 0);
		DiaSetControl(us_testmodelessdia, 6, 0);
		DiaSetControl(us_testmodelessdia, item, 1);
		return;
	}
	if (item == 9)
	{
		ttyputmsg(x_("Edit now is '%s'"), DiaGetText(us_testmodelessdia, 9));
		return;
	}
	if (item == 11)
	{
		ttyputmsg(x_("Selected popup item %d"), DiaGetPopupEntry(us_testmodelessdia, 11));
		return;
	}
}
#endif

void us_debug(INTBIG count, CHAR *par[])
{
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER LIBRARY *lib;
	REGISTER VARIABLE *var;
	REGISTER INTBIG i, j, l, let1, let2, let3, filestatus;
	CHAR line[200], *pt, *libname, *libfile;
	extern BOOLEAN db_printerrors;

	if (count == 0) return;
	l = estrlen(par[0]);

	/********** THESE ARE SPECIAL CASES **********/
#ifdef TESTMODELESS
	if (namesamen(par[0], x_("modeless"), l) == 0)
	{
		CHAR *popups[3];
		us_testmodelessdia = DiaInitDialogModeless(&us_testdialog, us_modelessdialoghitroutine);
		if (us_testmodelessdia == 0) return;
		DiaInitTextDialog(us_testmodelessdia, 7, DiaNullDlogList, DiaNullDlogItem,
			DiaNullDlogDone, -1, SCSELMOUSE|SCREPORT);
		DiaStuffLine(us_testmodelessdia, 7, x_("First Line"));
		DiaStuffLine(us_testmodelessdia, 7, x_("Second Line"));
		DiaStuffLine(us_testmodelessdia, 7, x_("Third Line"));
		DiaStuffLine(us_testmodelessdia, 7, x_("Fourth Line"));
		DiaStuffLine(us_testmodelessdia, 7, x_("Fifth Line"));
		DiaStuffLine(us_testmodelessdia, 7, x_("Sixth Line"));
		DiaStuffLine(us_testmodelessdia, 7, x_("Seventh Line"));
		DiaStuffLine(us_testmodelessdia, 7, x_("Eighth Line"));
		DiaStuffLine(us_testmodelessdia, 7, x_("Ninth Line"));
		DiaStuffLine(us_testmodelessdia, 7, x_("Tenth Line"));
		DiaSelectLine(us_testmodelessdia, 7, -1);
		DiaSetText(us_testmodelessdia, 9, x_("Initial edit"));
		popups[0] = x_("First popup");
		popups[1] = x_("Second popup");
		popups[2] = x_("Third popup");
		DiaSetPopup(us_testmodelessdia, 11, 3, popups);
		DiaSetControl(us_testmodelessdia, 4, 1);
		return;
	}
#endif

#if 0		/* place 1 million parts */
#  define MILLIONSIZE   1000			/* square this to get number of nodes */
	if (namesamen(par[0], x_("million"), l) == 0)
	{
		NODEPROTO *item;
		extern TOOL *dr_tool, *ro_tool;
		REGISTER ARCPROTO *apwire;
		REGISTER PORTPROTO *ppy, *ppa;
		float duration;
		NODEINST *nilast;
		REGISTER void *dia;
		INTBIG lx, hx, ly, hy, wid, hei, maxy, maxx, nilastx, nilasty, nix, niy;

		toolturnoff(dr_tool, FALSE);
		toolturnoff(ro_tool, FALSE);
		toolturnoff(us_tool, FALSE);
		apwire = getarcproto(x_("schematic:wire"));
		item = getnodeproto(x_("schematic:Off-Page"));
		if (item == NONODEPROTO) return;
		ppy = getportproto(item, x_("y"));
		ppa = getportproto(item, x_("a"));
		wid = item->highx - item->lowx;
		hei = item->highy - item->lowy;
		np = newnodeproto(x_("million"), el_curlib);
		dia = DiaInitProgress(x_("Building..."));
		if (dia == 0) return;
		maxy = MILLIONSIZE;    maxx = MILLIONSIZE;
		starttimer();
		for(i=0; i<maxy; i++)
		{
			DiaSetProgress(dia, i, maxy);
			nilast = NONODEINST;
			for(j=0; j<maxx; j++)
			{
				lx = wid*j*2;   hx = lx + wid;
				ly = hei*i*2;   hy = ly + hei;
				ni = newnodeinst(item, lx, hx, ly, hy, 0, 0, np);
				if (nilast != NONODEINST)
				{
					portposition(nilast, ppy, &nilastx, &nilasty);
					portposition(ni, ppa, &nix, &niy);
					newarcinst(apwire, 0, 0, nilast, ppy, nilastx, nilasty,
						ni, ppa, nix, niy, np);
				}
				nilast = ni;
			}
		}
		duration = endtimer();
		ttyputmsg(x_("Took %s"), explainduration(duration));
		DiaDoneProgress(dia);
		toolturnon(us_tool);
		toolturnon(ro_tool);
		return;
	}
#endif

#ifdef DEBUGMERGE
	if (namesamen(par[0], x_("merge"), l) == 0)
	{
		static POLYGON *poly = NOPOLYGON;
		NODEINST **nilist, *nisub;
		GEOM **list;
		void *merge;
		INTBIG total, done, thisx, thisy, lastx, lasty;

		(void)needstaticpolygon(&poly, 4, us_tool->cluster);
		np = us_needcell();
		if (np == NONODEPROTO) return;

		/* see what is highlighted (highlighted node is subtracted) */
		nisub = NONODEINST;
		list = us_gethighlighted(WANTNODEINST, 0, 0);
		if (list[0] != NOGEOM && list[1] == NOGEOM) nisub = list[0]->entryaddr.ni;

		us_debugmergeoffset = np->highy - np->lowy +
			el_curlib->lambda[el_curtech->techindex]*4;
		total = 0;
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if (ni->proto->primindex == 0) continue;
			if (((ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH) != NPNODE) continue;
			if (ni == nisub) continue;
			total++;
		}
		if (total == 0) return;
		nilist = (NODEINST **)emalloc(total * (sizeof (NODEINST *)), el_tempcluster);
		total = 0;
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if (ni->proto->primindex == 0) continue;
			if (((ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH) != NPNODE) continue;
			if (ni == nisub) continue;
			nilist[total++] = ni;
		}
		done = 0;
		while (done == 0)
		{
			done = 1;
			for(i=1; i<total; i++)
			{
				thisx = (nilist[i]->lowx + nilist[i]->highx) / 2;
				thisy = (nilist[i]->lowy + nilist[i]->highy) / 2;
				lastx = (nilist[i-1]->lowx + nilist[i-1]->highx) / 2;
				lasty = (nilist[i-1]->lowy + nilist[i-1]->highy) / 2;
				if (lasty > thisy || (lasty == thisy && lastx <= thisx)) continue;
				ni = nilist[i];   nilist[i] = nilist[i-1];   nilist[i-1] = ni;
				done = 0;
			}
		}
		merge = mergenew(us_tool->cluster);
		for(l=0; l<total; l++)
		{
			ni = nilist[l];
			j = nodepolys(ni, 0, NOWINDOWPART);
			for(i=0; i<j; i++)
			{
				shapenodepoly(ni, i, poly);
				mergeaddpolygon(merge, poly->layer, poly->tech, poly);
			}
		}
		if (nisub != NONODEINST)
		{
			/* subtract this one */
			j = nodepolys(nisub, 0, NOWINDOWPART);
			for(i=0; i<j; i++)
			{
				shapenodepoly(nisub, i, poly);
				mergesubpolygon(merge, poly->layer, poly->tech, poly);
			}
		}
		mergeextract(merge, us_debugwritepolygon);
		mergedelete(merge);
		efree((CHAR *)nilist);
		return;
	}
#endif

#if 0		/* find floating ends of arcs */
	if (namesamen(par[0], x_("show-isolated-pins"), l) == 0)
	{
		INTBIG fun;
		HIGHLIGHT newhigh;
		PORTARCINST *pi;

		j = 0;
		np = us_needcell();
		if (np == NONODEPROTO) return;
		us_clearhighlightcount();

		/* look for pins sitting alone at the end of an arc */
		for(ni=np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			/* see if it is a pin */
			fun = nodefunction(ni);
			if (fun != NPPIN) continue;

			/* ignore if it has exports on it */
			if (ni->firstportexpinst != NOPORTEXPINST) continue;

			/* must connect to exactly 1 arc */
			if (ni->firstportarcinst == NOPORTARCINST) continue;
			if (ni->firstportarcinst->nextportarcinst != NOPORTARCINST) continue;

			/* highlight the node */
			j++;
			newhigh.status = HIGHFROM;
			newhigh.cell = ni->parent;
			newhigh.fromgeom = ni->geom;
			newhigh.fromport = NOPORTPROTO;
			newhigh.frompoint = 0;
			us_addhighlight(&newhigh);
		}

		/* look for bus pins that don't connect to bus wires */
		for(ni=np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			/* see if it is a pin */
			if (ni->proto != sch_buspinprim) continue;

			/* ignore if it has exports on it */
			if (ni->firstportexpinst != NOPORTEXPINST) continue;

			/* must not connect to any bus arcs */
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				if (pi->conarcinst->proto == sch_busarc) break;
			if (pi != NOPORTARCINST) continue;

			/* highlight the node */
			j++;
			newhigh.status = HIGHFROM;
			newhigh.cell = ni->parent;
			newhigh.fromgeom = ni->geom;
			newhigh.fromport = NOPORTPROTO;
			newhigh.frompoint = 0;
			us_addhighlight(&newhigh);
		}
		if (j == 0) ttyputmsg("No floating pins"); else
			ttyputmsg("Found %ld floating pins", j);
		return;
	}
#endif

	if (namesamen(par[0], x_("postscript-schematics"), l) == 0 && l >= 2)
	{
		WINDOWPART *win;
		win = el_curwindowpart;
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		{
			if ((lib->userbits&HIDDENLIBRARY) != 0) continue;

			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				NODEPROTO *onp;

				if (np->cellview != el_schematicview) continue;
				ttyputmsg("Dumping Postscript for %s", describenodeproto(np));
				onp = lib->curnodeproto;
				lib->curnodeproto = np;
				win->curnodeproto = np;
				(void)asktool(io_tool, x_("write"), (INTBIG)lib, (INTBIG)"postscript");
				win->curnodeproto = onp;
				lib->curnodeproto = onp;
			}
		}
		return;
	}

	if (namesamen(par[0], x_("list-cells-with-universal-arcs"), l) == 0 && l >= 2)
	{
		LIBRARY *libwalk;
		NODEPROTO *np;
		ARCINST *ai;

		for (libwalk = el_curlib; libwalk != NOLIBRARY; libwalk = libwalk->nextlibrary) {
			for (np = libwalk->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto) {
				/* check for universal arcs only in layout and schematic cells */
				if (np->cellview == el_schematicview || np->cellview == el_layoutview) {
					for (ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst) {
						if (ai->proto == gen_universalarc) {
							ttyputmsg(x_("Found universal arc in %s, arc %s"), describenodeproto(np), describearcinst(ai));
						}
					}
				}
			}
		}
		return;
	}

	if (namesamen(par[0], x_("find-universal-arcs"), l) == 0 && l >= 2)
	{
		NODEPROTO *cell;
		void *infstr;
		ARCINST *ai;
		INTBIG count;

		count = 0;
		cell = getcurcell();
		if (cell == NONODEPROTO) {
			ttyputerr(_("No current cell to search for universal arcs"));
			return;
		}
		/* remove highlighting */
		(void)asktool(us_tool, x_("clear"));

		/* find any universal arcs in current cell and highlight them */
		infstr = initinfstr();
		for (ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst) {
			if (ai->proto == gen_universalarc) {
				us_highlighteverywhere(ai->geom, NOPORTPROTO, 1, TRUE, getecolor("white"), FALSE);
				/* JKG: this isn't quite right, but I'm not sure what is...disabling for now */
				count++;
			}
		}
		if (count == 0) ttyputmsg(_("none found"));
		else ttyputmsg(x_("%d universal arcs found"), count);
		return;
	}

	if (namesamen(par[0], x_("prepare-tsmc-io"), l) == 0 && l >= 2)
	{
		INTBIG total;
		CHAR **filelist, libfile[200], libname[50];
		LIBRARY *leflib, *gdslib, *prepfile;
		extern TOOL *dr_tool;

#define TSMCELECDIRECTORY x_("E:\\DevelE\\Artisan\\TPZ872G_150C\\ELIB\\")
#define TSMCGDSDIRECTORY  x_("E:\\DevelE\\Artisan\\TPZ872G_150C\\GDS\\")
#define TSMCLEFFILE       x_("E:\\DevelE\\Artisan\\TPZ872G_150C\\LEF\\tpz872g_4lm.lef")
#define TSMCPREPFILE      x_("E:\\DevelE\\Artisan\\tsmc25m4.txt")

		/* turn off DRC */
		toolturnoff(dr_tool, TRUE);

		/* read the TSMC preparation file */
		estrcpy(libname, x_("prep"));
		estrcpy(libfile, TSMCPREPFILE);
		prepfile = newlibrary(libname, libfile);
		ttyputmsg(x_("Reading Preparation file %s"), libfile);
		if (asktool(io_tool, x_("read"), (INTBIG)prepfile, (INTBIG)x_("text"), 0))
		{
			ttyputerr(x_("Cannot find TSMC preparation file %s"), libfile);
			return;
		}

		/* read the LEF library with ports */
		estrcpy(libname, x_("lef"));
		estrcpy(libfile, TSMCLEFFILE);
		leflib = newlibrary(libname, libfile);
		ttyputmsg(x_("Reading LEF file %s"), libfile);
		if (asktool(io_tool, x_("read"), (INTBIG)leflib, (INTBIG)x_("lef"), 0) != 0)
		{
			ttyputerr(x_("Cannot find LEF file %s"), libfile);
			return;
		}

		/* create a library for the GDS files */
		estrcpy(libname, x_("gds"));
		estrcpy(libfile, x_("gds.elib"));
		gdslib = newlibrary(libname, libfile);
		selectlibrary(gdslib, TRUE);

		total = filesindirectory(TSMCGDSDIRECTORY, &filelist);
		for(i=0; i<total; i++)
		{
			if (filelist[i][0] == '.') continue;

			/* read the GDS file */
			eraselibrary(gdslib);
			estrcpy(libfile, TSMCGDSDIRECTORY);
			estrcat(libfile, filelist[i]);
			(void)reallocstring(&gdslib->libfile, libfile, gdslib->cluster);
			ttyputmsg(x_("Reading GDS file %s"), libfile);
			if (asktool(io_tool, x_("read"), (INTBIG)gdslib, (INTBIG)x_("gds"), 0) != 0) break;

			/* synchronize the ports */
			us_portsynchronize(leflib);

			/* mark all cells as part of a "cell library" */
			for(np = gdslib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				np->userbits |= INCELLLIBRARY;

			/* write the Electric library */
			estrcpy(libfile, TSMCELECDIRECTORY);
			estrcat(libfile, filelist[i]);
			j = estrlen(libfile);
			if (namesame(&libfile[j-4], x_(".gds")) == 0) libfile[j-4] = 0;
			(void)reallocstring(&gdslib->libfile, libfile, gdslib->cluster);
			gdslib->userbits |= READFROMDISK;
			if (asktool(io_tool, x_("write"), (INTBIG)gdslib, (INTBIG)x_("binary")) != 0) break;
		}
		prepfile->userbits &= ~(LIBCHANGEDMAJOR|LIBCHANGEDMINOR);
		leflib->userbits &= ~(LIBCHANGEDMAJOR|LIBCHANGEDMINOR);
		return;
	}

	if (namesamen(par[0], x_("erase-bits"), l) == 0 && l >= 2)
	{
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		{
			/* clear the library bits */
			lib->temp1 = lib->temp2 = 0;
			lib->userbits &= (LIBCHANGEDMAJOR | LIBCHANGEDMINOR | LIBUNITS);

			/* look at every cell in the library */
			for(np=lib->firstnodeproto; np!=NONODEPROTO; np=np->nextnodeproto)
			{
				/* clear the node prototype bits */
				np->temp1 = np->temp2 = 0;
				np->userbits &= WANTNEXPAND;

				/* clear the port prototype bits */
				for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				{
					pp->temp1 = pp->temp2 = 0;
					pp->userbits &= (STATEBITS|PORTANGLE|PORTARANGE);
				}

				/* clear the node instance bits */
				for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				{
					ni->temp1 = ni->temp2 = 0;
					ni->userbits &= NEXPAND;

					/* compute WIPED bit on node instance */
					ni->userbits |= us_computewipestate(ni);
				}

				/* clear the arc instance bits */
				for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
				{
					ai->temp1 = ai->temp2 = 0;
					ai->userbits &= (FIXED|FIXANG|ISNEGATED|NOEXTEND|NOTEND0|NOTEND1|REVERSEEND|CANTSLIDE);
					determineangle(ai);
					(void)setshrinkvalue(ai, FALSE);
				}
			}
		}
		ttyputmsg(x_("All tool words reset"));
		return;
	}

	if (namesamen(par[0], x_("label-cell"), l) == 0 && l >= 1)
	{
		np = us_needcell();
		if (np == NONODEPROTO) return;
		us_clearhighlightcount();
		let1 = 'A';   let2 = let3 = 0;
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			line[0] = (CHAR)let1;   line[1] = (CHAR)let2;
			line[2] = (CHAR)let3;   line[3] = 0;
			var = getvalkey((INTBIG)ni, VNODEINST, -1, el_node_name_key);
			if (var != NOVARIABLE) continue;
			var = setvalkey((INTBIG)ni, VNODEINST, el_node_name_key, (INTBIG)line,
				VSTRING|VDISPLAY);
			if (var != NOVARIABLE)
				defaulttextsize(3, var->textdescript);
			let1++;
			if (let1 > 'Z')
			{
				let1 = 'A';
				if (let2 == 0) let2 = 'A'; else let2++;
				if (let2 > 'Z')
				{
					let2 = 'A';
					if (let3 == 0) let3 = 'A'; else let3++;
				}
			}
		}
		i = 1;
		for(ai = np->firstarcinst; ai != NOARCINST;
			ai = ai->nextarcinst)
		{
			(void)esnprintf(line, 200, x_("%ld"), i++);
			var = getvalkey((INTBIG)ai, VARCINST, -1, el_arc_name_key);
			if (var != NOVARIABLE) continue;
			var = setvalkey((INTBIG)ai, VARCINST, el_arc_name_key, (INTBIG)line,
				VSTRING|VDISPLAY);
			if (var != NOVARIABLE)
				defaulttextsize(4, var->textdescript);
		}
		us_redisplay(el_curwindowpart);
		return;
	}

	/********** THESE ARE STANDARD **********/

	if (namesamen(par[0], x_("arena"), l) == 0 && l >= 1)
	{
		if (count > 1) db_printclusterarena(par[1]); else
			db_printclusterarena(x_(""));
		return;
	}

	if (namesamen(par[0], x_("check-database"), l) == 0 && l >= 1)
	{
		if (count > 1 && namesamen(par[1], x_("verbose"), estrlen(par[1])) == 0)
			us_checkdatabase(TRUE); else
				us_checkdatabase(FALSE);
		return;
	}

	if (namesamen(par[0], x_("translate"), l) == 0 && l >= 1)
	{
		us_translationdlog();
		return;
	}

	if (namesamen(par[0], x_("dialog-edit"), l) == 0 && l >= 1)
	{
		us_dialogeditor();
		return;
	}

	if (namesamen(par[0], x_("examine-options"), l) == 0 && l >= 2)
	{
		libname = us_tempoptionslibraryname();
		libfile = optionsfilepath();
		filestatus = fileexistence(truepath(libfile));
		if (filestatus == 1 || filestatus == 3)
		{
			lib = newlibrary(libname, libfile);
			if (lib == NOLIBRARY) return;
			(void)asktool(io_tool, x_("read"), (INTBIG)lib, (INTBIG)x_("binary"));
			(void)asktool(io_tool, x_("write"), (INTBIG)lib, (INTBIG)x_("text"));
			killlibrary(lib);
		}
		return;
	}

	if (namesamen(par[0], x_("freeze-user-interface"), l) == 0 && l >= 1)
	{
		j = l = 0;
		for(i=0; i<us_tool->numvar; i++)
		{
			var = &us_tool->firstvar[i];
			pt = makename(var->key);
			if (namesamen(pt, x_("USER_binding_"), 13) == 0)
			{
				if (namesame(pt, x_("USER_binding_menu")) == 0) continue;
				var->type &= ~VDONTSAVE;
				j++;
			} else if (namesamen(pt, x_("USER_macro_"), 11) == 0)
			{
				var->type &= ~VDONTSAVE;

				/*
				 * when saving macros to disk, must erase entry 2, which
				 * is the address of the parsing structures: these cannot
				 * be valid when read back in.
				 */
				setindkey((INTBIG)us_tool, VTOOL, var->key, 2, (INTBIG)x_(""));
				l++;
			}
		}
		ttyputmsg(x_("Made %ld macros and %ld bindings permanent"), l, j);
		return;
	}

	if (namesamen(par[0], x_("internal-errors"), l) == 0 && l >= 1)
	{
		db_printerrors = !db_printerrors;
		ttyputmsg(x_("Internal database errors will %sbe printed"),
			(db_printerrors ? x_("") : x_("not ")));
		return;
	}

	if (namesamen(par[0], x_("namespace"), l) == 0 && l >= 1)
	{
		ttyputmsg(x_("%ld names in global namespace:"), el_numnames);
		for(i=0; i<el_numnames; i++) ttyputmsg(x_("'%s'"), el_namespace[i]);
		return;
	}

	if (namesamen(par[0], x_("options-changed"), l) == 0 && l >= 1)
	{
		explainoptionchanges(-1, 0);
		return;
	}

	if (namesamen(par[0], x_("rtree"), l) == 0 && l >= 2)
	{
		np = us_needcell();
		if (np == NONODEPROTO) return;
		ttyputmsg(x_("R-tree for cell %s (nodes hold from %d to %d entries)"),
			describenodeproto(np), MINRTNODESIZE, MAXRTNODESIZE);
		db_printrtree(NORTNODE, np->rtree, 0);
		return;
	}

	if (namesamen(par[0], x_("undo"), l) == 0 && l >= 1)
	{
		db_undodlog();
		return;
	}

	ttyputbadusage(x_("debug"));
}

void us_defarc(INTBIG count, CHAR *par[])
{
	REGISTER CHAR *aname, *pt;
	REGISTER BOOLEAN negate;
	REGISTER INTBIG l, i, wid, style, bits;
	REGISTER ARCPROTO *ap;
	REGISTER PORTPROTO *pp;
	REGISTER NODEPROTO *np;
	extern COMCOMP us_defarcsp, us_defarcnotp, us_defarcwidp, us_defarcpinp, us_defarcangp;
	REGISTER VARIABLE *var;
	REGISTER void *infstr;

	if (count == 0)
	{
		count = ttygetparam(_("Arc prototype: "), &us_defarcsp, MAXPARS, par);
		if (count == 0)
		{
			us_abortedmsg();
			return;
		}
	}

	/* get arc prototype name */
	if (estrcmp(par[0], x_("*")) == 0)
	{
		ap = NOARCPROTO;
		aname = x_("all");
	} else
	{
		ap = getarcproto(par[0]);
		if (ap == NOARCPROTO)
		{
			us_abortcommand(_("Unknown arc prototype: %s"), par[0]);
			return;
		}
		aname = ap->protoname;
	}

	if (count <= 1)
	{
		/* get style to report */
		if (ap != NOARCPROTO)
		{
			var = getvalkey((INTBIG)ap, VARCPROTO, VINTEGER, us_arcstylekey);
			if (var != NOVARIABLE) style = var->addr; else style = ap->userbits;
		} else
		{
			var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_arcstylekey);
			if (var != NOVARIABLE) style = var->addr; else style = -1;
			if (style == -1)
			{
				ttyputmsg(_("No overriding defaults set for arcs"));
				return;
			}
		}
		infstr = initinfstr();
		formatinfstr(infstr, _("%s arcs drawn "), aname);
		if ((style&WANTFIX) != 0) addstringtoinfstr(infstr, _("rigid")); else
		{
			addstringtoinfstr(infstr, _("nonrigid, "));
			if ((style&WANTFIXANG) == 0) addstringtoinfstr(infstr, _("not fixed angle, ")); else
				addstringtoinfstr(infstr, _("fixed angle, "));
			if ((style&WANTCANTSLIDE) != 0) addstringtoinfstr(infstr, _("nonslidable")); else
				addstringtoinfstr(infstr, _("slidable"));
		}
		if ((style&WANTDIRECTIONAL) != 0) addstringtoinfstr(infstr, _(", directional"));
		if ((style&WANTNEGATED) != 0) addstringtoinfstr(infstr, _(", negated"));
		addstringtoinfstr(infstr, x_(", "));
		if ((style&WANTNOEXTEND) != 0) addstringtoinfstr(infstr, _("not ends-extended")); else
			addstringtoinfstr(infstr, _("ends-extended"));
		if (ap != NOARCPROTO)
			formatinfstr(infstr, _(", width %s, angle %ld"),
				latoa(defaultarcwidth(ap)-arcprotowidthoffset(ap), 0),
					(ap->userbits&AANGLEINC) >> AANGLEINCSH);
		ttyputmsg(x_("%s"), returninfstr(infstr));
		return;
	}

	l = estrlen(pt = par[1]);
	negate = FALSE;

	if (namesamen(pt, x_("not"), l) == 0 && l >= 2)
	{
		if (count <= 2)
		{
			count = ttygetparam(_("Defarc option: "), &us_defarcnotp, MAXPARS-2, &par[2])+2;
			if (count == 2)
			{
				us_abortedmsg();
				return;
			}
		}
		count--;
		par++;
		negate = TRUE;
		l = estrlen(pt = par[1]);
	}

	if (namesamen(pt, x_("default"), l) == 0 && l >= 2)
	{
		if (ap != NOARCPROTO)
		{
			(void)delvalkey((INTBIG)ap, VARCPROTO, us_arcstylekey);
			ttyputverbose(M_("%s arcs drawn in default style"), describearcproto(ap));
			return;
		}
		(void)delvalkey((INTBIG)us_tool, VTOOL, us_arcstylekey);
		ttyputverbose(M_("All arcs drawn in default style"));
		return;
	}

	if ((namesamen(pt, x_("manhattan"), l) == 0 || namesamen(pt, x_("fixed-angle"), l) == 0) && l >= 1)
	{
		if (namesamen(pt, x_("manhattan"), l) == 0)
			ttyputmsg(_("It is now proper to use 'defarc fixed-angle' instead of 'defarc manhattan'"));
		if (ap != NOARCPROTO)
		{
			var = getvalkey((INTBIG)ap, VARCPROTO, VINTEGER, us_arcstylekey);
			if (var != NOVARIABLE) style = var->addr; else style = ap->userbits;
			if (negate) style &= ~WANTFIXANG; else style |= WANTFIXANG;
			(void)setvalkey((INTBIG)ap, VARCPROTO, us_arcstylekey, style, VINTEGER);
		} else
		{
			var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_arcstylekey);
			if (var != NOVARIABLE) style = var->addr; else style = 0;
			if (negate) style &= ~WANTFIXANG; else style |= WANTFIXANG;
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_arcstylekey, style, VINTEGER|VDONTSAVE);
		}
		if (negate) ttyputverbose(M_("%s arcs drawn not fixed-angle"), aname); else
			ttyputverbose(M_("%s arcs drawn fixed-angle"), aname);
		return;
	}

	if (namesamen(pt, x_("rigid"), l) == 0 && l >= 1)
	{
		if (ap != NOARCPROTO)
		{
			var = getvalkey((INTBIG)ap, VARCPROTO, VINTEGER, us_arcstylekey);
			if (var != NOVARIABLE) style = var->addr; else style = ap->userbits;
			if (negate) style &= ~WANTFIX; else style |= WANTFIX;
			(void)setvalkey((INTBIG)ap, VARCPROTO, us_arcstylekey, style, VINTEGER);
		} else
		{
			var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_arcstylekey);
			if (var != NOVARIABLE) style = var->addr; else style = WANTFIXANG;
			if (negate) style &= ~WANTFIX; else style |= WANTFIX;
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_arcstylekey, style, VINTEGER|VDONTSAVE);
		}
		if (negate) ttyputverbose(M_("%s arcs drawn un-rigid"), aname); else
			ttyputverbose(M_("%s arcs drawn rigid"), aname);
		return;
	}

	if (namesamen(pt, x_("directional"), l) == 0 && l >= 2)
	{
		if (ap != NOARCPROTO)
		{
			var = getvalkey((INTBIG)ap, VARCPROTO, VINTEGER, us_arcstylekey);
			if (var != NOVARIABLE) style = var->addr; else style = ap->userbits;
			if (negate) style &= ~WANTDIRECTIONAL; else
				style |= WANTDIRECTIONAL;
			(void)setvalkey((INTBIG)ap, VARCPROTO, us_arcstylekey, style, VINTEGER);
		} else
		{
			var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_arcstylekey);
			if (var != NOVARIABLE) style = var->addr; else style = WANTFIXANG;
			if (negate) style &= ~WANTDIRECTIONAL; else
				style |= WANTDIRECTIONAL;
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_arcstylekey, style, VINTEGER|VDONTSAVE);
		}
		if (negate) ttyputverbose(M_("%s arcs drawn nondirectional"), aname); else
			ttyputverbose(M_("%s arcs drawn directional"), aname);
		return;
	}

	if (namesamen(pt, x_("slide"), l) == 0 && l >= 1)
	{
		if (ap != NOARCPROTO)
		{
			var = getvalkey((INTBIG)ap, VARCPROTO, VINTEGER, us_arcstylekey);
			if (var != NOVARIABLE) style = var->addr; else style = ap->userbits;
			if (negate) style &= ~WANTCANTSLIDE; else
				style |= WANTCANTSLIDE;
			(void)setvalkey((INTBIG)ap, VARCPROTO, us_arcstylekey, style, VINTEGER);
		} else
		{
			var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_arcstylekey);
			if (var != NOVARIABLE) style = var->addr; else style = WANTFIXANG;
			if (negate) style &= ~WANTCANTSLIDE; else
				style |= WANTCANTSLIDE;
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_arcstylekey, style, VINTEGER|VDONTSAVE);
		}
		if (negate) ttyputverbose(M_("%s arcs can not slide in their ports"), aname); else
			ttyputverbose(M_("%s arcs can slide in their ports"), aname);
		return;
	}

	if (namesamen(pt, x_("negated"), l) == 0 && l >= 2)
	{
		if (ap != NOARCPROTO)
		{
			var = getvalkey((INTBIG)ap, VARCPROTO, VINTEGER, us_arcstylekey);
			if (var != NOVARIABLE) style = var->addr; else style = ap->userbits;
			if (negate) style &= ~WANTNEGATED; else style |= WANTNEGATED;
			(void)setvalkey((INTBIG)ap, VARCPROTO, us_arcstylekey, style, VINTEGER);
		} else
		{
			var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_arcstylekey);
			if (var != NOVARIABLE) style = var->addr; else style = WANTFIXANG;
			if (negate) style &= ~WANTNEGATED; else style |= WANTNEGATED;
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_arcstylekey, style, VINTEGER|VDONTSAVE);
		}
		if (negate) ttyputverbose(M_("%s arcs drawn un-negated"), aname); else
			ttyputverbose(M_("%s arcs drawn negated"), aname);
		return;
	}

	if (namesamen(pt, x_("ends-extend"), l) == 0 && l >= 1)
	{
		if (ap != NOARCPROTO)
		{
			var = getvalkey((INTBIG)ap, VARCPROTO, VINTEGER, us_arcstylekey);
			if (var != NOVARIABLE) style = var->addr; else style = ap->userbits;
			if (negate) style |= WANTNOEXTEND; else style &= ~WANTNOEXTEND;
			(void)setvalkey((INTBIG)ap, VARCPROTO, us_arcstylekey, style, VINTEGER);
		} else
		{
			var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_arcstylekey);
			if (var != NOVARIABLE) style = var->addr; else style = WANTFIXANG;
			if (negate) style |= WANTNOEXTEND; else style &= ~WANTNOEXTEND;
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_arcstylekey, style, VINTEGER|VDONTSAVE);
		}
		if (negate) ttyputverbose(M_("%s arcs drawn with ends not extended"), aname); else
			ttyputverbose(M_("%s arcs drawn with ends extended by half-width"), aname);
		return;
	}

	if (namesamen(pt, x_("width"), l) == 0 && l >= 1)
	{
		if (ap == NOARCPROTO)
		{
			us_abortcommand(_("Default width requires specific arc prototypes"));
			return;
		}
		if (count <= 2)
		{
			count = ttygetparam(_("Width: "), &us_defarcwidp, MAXPARS-2, &par[2]) + 2;
			if (count == 2)
			{
				us_abortedmsg();
				return;
			}
		}
		pt = par[2];
		i = atola(pt, 0);
		if (i&1) i++;
		if (i < 0)
		{
			us_abortcommand(_("Width must not be negative"));
			return;
		}
		wid = arcprotowidthoffset(ap) + i;
		wid = wid * WHOLE / el_curlib->lambda[ap->tech->techindex];
		(void)setvalkey((INTBIG)ap, VARCPROTO, el_arc_width_default_key, wid, VINTEGER);
		ttyputverbose(M_("%s arcs default to %s wide"), aname, latoa(i, 0));
		return;
	}

	if (namesamen(pt, x_("angle"), l) == 0 && l >= 1)
	{
		if (ap == NOARCPROTO)
		{
			us_abortcommand(_("Default angle increment requires specific arc prototypes"));
			return;
		}
		if (count <= 2)
		{
			count = ttygetparam(_("Angle: "), &us_defarcangp, MAXPARS-2, &par[2]) + 2;
			if (count == 2)
			{
				us_abortedmsg();
				return;
			}
		}
		pt = par[2];
		i = eatoi(pt) % 360;
		if (i < 0)
		{
			us_abortcommand(_("Angle increment must not be negative"));
			return;
		}
		bits = (ap->userbits & ~AANGLEINC) | (i << AANGLEINCSH);
		(void)setval((INTBIG)ap, VARCPROTO, x_("userbits"), bits, VINTEGER);
		ttyputverbose(M_("%s arcs will run at %ld degree angle increments"), aname, i);
		return;
	}

	if (namesamen(pt, x_("pin"), l) == 0 && l >= 1)
	{
		if (ap == NOARCPROTO)
		{
			us_abortcommand(_("Default pin requires specific arc prototypes"));
			return;
		}
		if (count <= 2)
		{
			count = ttygetparam(_("Pin: "), &us_defarcpinp, MAXPARS-2, &par[2]) + 2;
			if (count == 2)
			{
				us_abortedmsg();
				return;
			}
		}
		np = getnodeproto(par[2]);
		if (np == NONODEPROTO)
		{
			us_abortcommand(_("Cannot find primitive %s"), par[2]);
			return;
		}
		pp = np->firstportproto;
		for(i=0; pp->connects[i] != NOARCPROTO; i++)
			if (pp->connects[i] == ap) break;
		if (pp->connects[i] == NOARCPROTO)
		{
			us_abortcommand(_("Cannot: the %s node must connect to %s arcs on its first port (%s)"),
				describenodeproto(np), aname, pp->protoname);
			return;
		}
		(void)setval((INTBIG)ap, VARCPROTO, x_("ARC_Default_Pin"), (INTBIG)np, VNODEPROTO|VDONTSAVE);
		ttyputverbose(M_("Default pins for arc %s will be %s"), aname, describenodeproto(np));
		return;
	}

	ttyputbadusage(x_("defarc"));
}

void us_defnode(INTBIG count, CHAR *par[])
{
	REGISTER CHAR *pp, *nname;
	BOOLEAN negated;
	REGISTER INTBIG l, pangle;
	REGISTER INTBIG i, j;
	INTBIG plx, ply, phx, phy, nodesize[2];
	REGISTER VARIABLE *var;
	REGISTER NODEPROTO *np, *onp;
	extern COMCOMP us_defnodesp, us_defnodexsp, us_defnodeysp;

	if (count == 0)
	{
		count = ttygetparam(_("Node prototype: "), &us_defnodesp, MAXPARS, par);
		if (count == 0)
		{
			us_abortedmsg();
			return;
		}
	}

	/* get node prototype name */
	if (estrcmp(par[0], x_("*")) == 0)
	{
		np = NONODEPROTO;
		nname = _("All complex");
	} else
	{
		np = getnodeproto(par[0]);
		if (np == NONODEPROTO)
		{
			us_abortcommand(_("Unknown node prototype: %s"), par[0]);
			return;
		}
		nname = describenodeproto(np);
	}

	if (count <= 1)
	{
		ttyputusage(x_("defnode OPTIONS"));
		return;
	}

	l = estrlen(pp = par[1]);

	/* handle negation */
	if (namesamen(pp, x_("not"), l) == 0 && l >= 1)
	{
		negated = TRUE;
		count--;
		par++;
		l = estrlen(pp = par[1]);
	} else negated = FALSE;

	if (namesamen(pp, x_("from-cell-library"), l) == 0)
	{
		if (np != NONODEPROTO && np->primindex != 0)
		{
			us_abortcommand(_("Cell library marking can only be done for cells"));
			return;
		}
		for(onp = el_curlib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
		{
			if (np != NONODEPROTO && np != onp) continue;
			if (!negated)
			{
				if ((onp->userbits&INCELLLIBRARY) != 0)
				{
					ttyputverbose(M_("Cell %s is already part of a cell library"),
						describenodeproto(onp));
				} else
				{
					ttyputverbose(M_("Cell %s is now part of a cell library"),
						describenodeproto(onp));
				}
				(void)setval((INTBIG)onp, VNODEPROTO, x_("userbits"),
					onp->userbits | INCELLLIBRARY, VINTEGER);
			} else
			{
				if ((onp->userbits&INCELLLIBRARY) == 0)
				{
					ttyputverbose(M_("Cell %s is already not part of a cell library"),
						describenodeproto(onp));
				} else
				{
					ttyputverbose(M_("Cell %s is now not part of a cell library"),
						describenodeproto(onp));
				}
				(void)setval((INTBIG)onp, VNODEPROTO, x_("userbits"),
					onp->userbits & ~INCELLLIBRARY, VINTEGER);
			}
		}
		return;
	}

	if (namesamen(pp, x_("check-dates"), l) == 0 && l >= 2)
	{
		if (np != NONODEPROTO)
		{
			us_abortcommand(_("Date checking must be done for all cells (*)"));
			return;
		}
		if (!negated)
		{
			if ((us_useroptions&CHECKDATE) != 0)
				ttyputverbose(M_("Cell revision dates already being checked")); else
					ttyputverbose(M_("Cell revision date errors will be identified"));
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey,
				us_useroptions | CHECKDATE, VINTEGER);
		} else
		{
			if ((us_useroptions&CHECKDATE) == 0)
				ttyputverbose(M_("Cell revision dates already being ignored")); else
					ttyputverbose(M_("Cell revision date errors will be ignored"));
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey,
				us_useroptions & ~CHECKDATE, VINTEGER);
		}
		return;
	}

	if (namesamen(pp, x_("centered-primitives"), l) == 0 && l >= 2)
	{
		if (np != NONODEPROTO)
		{
			us_abortcommand(_("Primitive centering must be done for all nodes (*)"));
			return;
		}
		if (!negated)
		{
			if ((us_useroptions&CENTEREDPRIMITIVES) != 0)
				ttyputverbose(M_("Primitives are already being placed by their center")); else
					ttyputverbose(M_("Primitives will be placed by their center"));
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey,
				us_useroptions | CENTEREDPRIMITIVES, VINTEGER);
		} else
		{
			if ((us_useroptions&CENTEREDPRIMITIVES) == 0)
				ttyputverbose(M_("Primitives are already being placed by edges")); else
					ttyputverbose(M_("Primitives will be placed by edges"));
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey,
				us_useroptions & ~CENTEREDPRIMITIVES, VINTEGER);
		}
		return;
	}

	if (namesamen(pp, x_("copy-ports"), l) == 0 && l >= 2)
	{
		if (np != NONODEPROTO)
		{
			us_abortcommand(_("Port copying must be done for all nodes (*)"));
			return;
		}
		if (!negated)
		{
			if ((us_useroptions&DUPCOPIESPORTS) != 0)
				ttyputverbose(M_("Ports are already being copied by duplicate/array/yanknode")); else
					ttyputverbose(M_("Ports will be copied by duplicate/array/yanknode"));
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey,
				us_useroptions | DUPCOPIESPORTS, VINTEGER);
		} else
		{
			if ((us_useroptions&DUPCOPIESPORTS) == 0)
				ttyputverbose(M_("Ports are already not being copied by duplicate/array/yanknode")); else
					ttyputverbose(M_("Ports will not be copied by duplicate/array/yanknode"));
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey,
				us_useroptions & ~DUPCOPIESPORTS, VINTEGER);
		}
		return;
	}

	if (namesamen(pp, x_("alterable"), l) == 0 && l >= 1)
	{
		if (np != NONODEPROTO)
		{
			/* set alterability of cells */
			if (!negated)
			{
				if ((np->userbits&NPLOCKED) == 0)
				{
					ttyputverbose(M_("Contents of cell %s are already able to be freely altered"),
						describenodeproto(np));
				} else
				{
					ttyputverbose(M_("Contents of cell %s may now be freely edited"),
						describenodeproto(np));
				}
				(void)setval((INTBIG)np, VNODEPROTO, x_("userbits"),
					np->userbits & ~NPLOCKED, VINTEGER);
			} else
			{
				if ((np->userbits&NPLOCKED) != 0)
				{
					ttyputverbose(M_("Contents of cell %s are already locked"),
						describenodeproto(np));
				} else
				{
					ttyputverbose(M_("Contents of cell %s will not be allowed to change"),
						describenodeproto(np));
				}
				(void)setval((INTBIG)np, VNODEPROTO, x_("userbits"),
					np->userbits | NPLOCKED, VINTEGER);
			}
		} else
		{
			us_abortcommand(_("Must set alterability for a specific cell"));
		}
		return;
	}

	if (namesamen(pp, x_("instances-locked"), l) == 0 && l >= 1)
	{
		if (np == NONODEPROTO)
		{
			us_abortcommand(_("Cell instance locking cannot be done for all prototypes (*)"));
			return;
		}

		/* set alterability of cell instances */
		if (negated)
		{
			if ((np->userbits&NPILOCKED) == 0)
			{
				ttyputverbose(M_("Instances in cell %s are already able to be freely altered"),
					describenodeproto(np));
			} else
			{
				ttyputverbose(M_("Instances in cell %s may now be freely edited"),
					describenodeproto(np));
			}
			(void)setval((INTBIG)np, VNODEPROTO, x_("userbits"),
				np->userbits & ~NPILOCKED, VINTEGER);
		} else
		{
			if ((np->userbits&NPILOCKED) != 0)
			{
				ttyputverbose(M_("Instances in cell %s are already locked"),
					describenodeproto(np));
			} else
			{
				ttyputverbose(M_("Instances in cell %s will not be allowed to change"),
					describenodeproto(np));
			}
			(void)setval((INTBIG)np, VNODEPROTO, x_("userbits"),
				np->userbits | NPILOCKED, VINTEGER);
		}
		return;
	}

	if (namesamen(pp, x_("locked-primitives"), l) == 0 && l >= 1)
	{
		if (np != NONODEPROTO)
		{
			us_abortcommand(_("Primitive lock must be done for all prototypes (*)"));
			return;
		}
		if (negated)
		{
			if ((us_useroptions&NOPRIMCHANGES) == 0)
				ttyputverbose(M_("Locked primitives are already able to be freely altered")); else
					ttyputverbose(M_("Locked primitives may now be freely edited"));
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey,
				us_useroptions & ~NOPRIMCHANGES, VINTEGER);
		} else
		{
			if ((us_useroptions&NOPRIMCHANGES) != 0)
				ttyputverbose(M_("Locked primitive changes are already being disallowed")); else
					ttyputverbose(M_("Locked primitive changes will not be allowed"));
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey,
				us_useroptions | NOPRIMCHANGES, VINTEGER);
		}
		return;
	}

	if (namesamen(pp, x_("expanded"), l) == 0 && l >= 1)
	{
		if (np == NONODEPROTO)
		{
			for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				if (!negated) (void)setval((INTBIG)np, VNODEPROTO, x_("userbits"),
					np->userbits | WANTNEXPAND, VINTEGER); else
						(void)setval((INTBIG)np, VNODEPROTO, x_("userbits"),
							np->userbits & ~WANTNEXPAND, VINTEGER);
			}
		} else
		{
			if (np->primindex != 0)
			{
				if (!negated) us_abortcommand(_("Primitives are expanded by definition")); else
					us_abortcommand(_("Cannot un-expand primitives"));
				return;
			}
			if (!negated) (void)setval((INTBIG)np, VNODEPROTO, x_("userbits"),
				np->userbits | WANTNEXPAND, VINTEGER); else
					(void)setval((INTBIG)np, VNODEPROTO, x_("userbits"),
						np->userbits & ~WANTNEXPAND, VINTEGER);
		}
		ttyputverbose(M_("%s node prototypes will be %sexpanded on creation"), nname, (negated ? _("un-") : x_("")));
		return;
	}

	if (namesamen(pp, x_("size"), l) == 0 && l >= 1)
	{
		if (np == NONODEPROTO)
		{
			us_abortcommand(_("Must change size on a single node"));
			return;
		}
		if (np->primindex == 0)
		{
			us_abortcommand(_("Can only change default size on primitives"));
			return;
		}
		if (count <= 2)
		{
			count = ttygetparam(_("X size: "), &us_defnodexsp, MAXPARS-2, &par[2]) + 2;
			if (count == 2)
			{
				us_abortedmsg();
				return;
			}
		}
		pp = par[2];
		i = atola(pp, 0);
		if (i&1) i++;
		if (count <= 3)
		{
			count = ttygetparam(_("Y size: "), &us_defnodeysp, MAXPARS-3, &par[3]) + 3;
			if (count == 3)
			{
				us_abortedmsg();
				return;
			}
		}
		pp = par[3];
		j = atola(pp, 0);
		if (j&1) j++;
		if (i < 0 || j < 0)
		{
			us_abortcommand(_("Sizes must not be negative"));
			return;
		}
		nodeprotosizeoffset(np, &plx, &ply, &phx, &phy, NONODEPROTO);
		nodesize[0] = i+plx+phx;
		nodesize[1] = j+ply+phy;
		nodesize[0] = nodesize[0] * WHOLE / el_curlib->lambda[np->tech->techindex];
		nodesize[1] = nodesize[1] * WHOLE / el_curlib->lambda[np->tech->techindex];
		(void)setvalkey((INTBIG)np, VNODEPROTO, el_node_size_default_key,
			(INTBIG)nodesize, VINTEGER|VISARRAY|(2<<VLENGTHSH));
		ttyputverbose(M_("%s nodes will be created %sx%s in size"), describenodeproto(np),
			latoa(i, 0), latoa(j, 0));
		return;
	}

	if (namesamen(pp, x_("placement"), l) == 0 && l >= 1)
	{
		if (np != NONODEPROTO)
		{
			/* get placement angle for this node */
			var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER, us_placement_angle_key);
			if (var == NOVARIABLE) pangle = 0; else pangle = var->addr;

			if (count > 2)
			{
				pangle = (atofr(par[2])*10/WHOLE) % 3600;
				if (pangle < 0) pangle += 3600;
				if (namesame(&par[2][estrlen(par[2])-1], x_("t")) == 0) pangle += 3600;
			} else if (pangle >= 6300) pangle = 0; else
				pangle += 900;
			(void)setvalkey((INTBIG)np, VNODEPROTO, us_placement_angle_key, pangle, VINTEGER);
		} else
		{
			/* get placement angle for all nodes */
			var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_placement_angle_key);
			if (var == NOVARIABLE) pangle = 0; else pangle = var->addr;

			if (count > 2)
			{
				pangle = (atofr(par[2])*10/WHOLE) % 3600;
				if (pangle < 0) pangle += 3600;
				if (namesame(&par[2][estrlen(par[2])-1], x_("t")) == 0) pangle += 3600;
			} else if (pangle >= 6300) pangle = 0; else
				pangle += 900;
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_placement_angle_key, pangle, VINTEGER);
		}
		return;
	}

	ttyputbadusage(x_("defnode"));
}

void us_duplicate(INTBIG count, CHAR *par[])
{
	REGISTER NODEPROTO *np;
	REGISTER GEOM **list;
	REGISTER BOOLEAN interactiveplace;

	/* get object */
	np = us_needcell();
	if (np == NONODEPROTO) return;
	list = us_gethighlighted(WANTNODEINST | WANTARCINST, 0, 0);
	if (list[0] == NOGEOM)
	{
		us_abortcommand(_("First select objects to duplicate"));
		return;
	}

	if ((us_useroptions&NOMOVEAFTERDUP) != 0) interactiveplace = FALSE; else
		interactiveplace = TRUE;
	us_copylisttocell(list, np, np, TRUE, interactiveplace, TRUE);
}

#if 0		/* test code for John Wood */

#define NUMTRANSISTOR 5
#define DEFTRANSISTORWIDTH 3
#define TRANSISTORWIDTH 30
#define CELLNAME x_("multifingertransistor{lay}")

void us_debugjohn(void)
{
	NODEPROTO *cell, *tra, *contact;
	NODEINST *transni, *contactni, *lastcontactni;
	PORTPROTO *topport, *botport, *conport;
	ARCPROTO *active;
	ARCINST *ai;
	CHAR *par[2];
	INTBIG lx, hx, ly, hy, sizex, sizey, sep, lambda, sizeycontact, bits,
		tpx, tpy, cpx, cpy, wid, i;

	lambda = el_curlib->lambda[el_curtech->techindex];
	cell = newnodeproto(CELLNAME, el_curlib);
	if (cell == NONODEPROTO) return;

	/* get the objects we need */
	tra = getnodeproto(x_("mocmos:N-Transistor"));
	if (tra == NONODEPROTO) return;
	contact = getnodeproto(x_("mocmos:Metal-1-N-Active-Con"));
	conport = contact->firstportproto;
	topport = getportproto(tra, x_("n-trans-diff-top"));
	botport = getportproto(tra, x_("n-trans-diff-bottom"));
	active = getarcproto(x_("mocmos:N-Active"));

	/* get information for layout */
	sizex = tra->highx - tra->lowx;
	sizey = tra->highy - tra->lowy;
	sizeycontact = contact->highy - contact->lowy;
	sep = sizey * 2 - 32 * lambda;
	bits = us_makearcuserbits(active);
	wid = active->nominalwidth;

	/* lay down the transistors */
	for(i=0; i<NUMTRANSISTOR; i++)
	{
		lx = 0;   hx = sizex + (TRANSISTORWIDTH - DEFTRANSISTORWIDTH) * lambda;
		ly = i*sep;   hy = ly + sizey;
		transni = newnodeinst(tra, lx, hx, ly, hy, 0, 0, cell);
		endobjectchange((INTBIG)transni, VNODEINST);
		if (i < NUMTRANSISTOR-1)
		{
			ly = hy - lambda * 29 / 2;   hy = ly + sizeycontact;
			contactni = newnodeinst(contact, lx, hx, ly, hy, 0, 0, cell);
			endobjectchange((INTBIG)contactni, VNODEINST);

			/* wire to the transistor */
			portposition(transni, topport, &tpx, &tpy);
			portposition(contactni, conport, &cpx, &cpy);
			ai = newarcinst(active, wid, bits, transni, topport, tpx, tpy,
				contactni, conport, cpx, cpy, cell);
			endobjectchange((INTBIG)ai, VARCINST);
		}
		if (i > 0)
		{
			portposition(transni, botport, &tpx, &tpy);
			portposition(lastcontactni, conport, &cpx, &cpy);
			ai = newarcinst(active, wid, bits, transni, botport, tpx, tpy,
				lastcontactni, conport, cpx, cpy, cell);
			endobjectchange((INTBIG)ai, VARCINST);
		}
		lastcontactni = contactni;
	}

	/* display the result */
	par[0] = CELLNAME;
	us_editcell(1, par);
}
#endif
