/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iopsout.cpp
 * Input/output analysis tool: PostScript generation
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

/*
 * Rules used in creating the PostScript file.
 *
 * 1) Plain PostScript - when using an area-highlight objects completely
 *                         outside the highlight area are not printed
 *                     - file created with a '.ps' extension
 *
 * 2) Encapsulated PostScript - when using an area-highlight the image clip
 *                                path is set to the highlight area size
 *                            - file created with a '.eps' extension
 *
 * The rules for EPS are such because in most cases the EPS file will be used
 * inside a publishing package.
 */

#include "config.h"
#include "global.h"
#include "egraphics.h"
#include "eio.h"
#include "usr.h"
#include "edialogs.h"

/* #define DEBUGHIERPS 1 */	/* uncomment to debug hierarchical PostScript (shows memory usage) */

#define PSSCALE             4
#define CORNERDATESIZE     14			/* size of date information in corner */

#define PSHEADERDOT         1			/* write macros for dot drawing */
#define PSHEADERLINE        2			/* write macros for line drawing */
#define PSHEADERPOLYGON     3			/* write macros for polygon drawing */
#define PSHEADERFPOLYGON    4			/* write macros for filled polygon drawing */
#define PSHEADERSTRING      5			/* write macros for text drawing */

static FILE       *io_psout;
static WINDOWPART  io_pswindow;
static INTBIG      io_whichlayer, io_maxpslayer;
static XARRAY      io_psmatrix = {{0,0,0},{0,0,0},{0,0,0}};
static GRAPHICS  **io_psgraphicsseen;
static INTBIG      io_psgraphicsseenlimit = 0;
static INTBIG      io_psgraphicsseencount;
static BOOLEAN     io_psusecolor;			/* nonzero to use color PostScript */
static BOOLEAN     io_psusecolorstip;		/* nonzero to use stippled color PostScript */
static BOOLEAN     io_psusecolormerge;		/* nonzero to use merged color PostScript */
static BOOLEAN     io_psdotput;				/* PostScript for dot put out */
static BOOLEAN     io_psdrawlineput;		/* PostScript for line put out */
static BOOLEAN     io_pspolygonput;			/* PostScript for polygon put out */
static BOOLEAN     io_psfilledpolygonput;	/* PostScript for filled polygon put out */
static BOOLEAN     io_psstringput;			/* PostScript for strings put out */
static BOOLEAN     io_psfake;				/* nonzero if faking output (just counting) */
static INTBIG     *io_redmap, *io_greenmap, *io_bluemap;
static INTBIG      io_lastred, io_lastgreen, io_lastblue;
static INTBIG      io_pspolygoncount;		/* number of polygons in job so far */
static INTBIG      io_pspolygontotal;		/* number of polygons in job */
static INTBIG      io_psopcount;			/* chunks of code */
static CHAR        io_psprefix[10];			/* indentation for PostScript */
static void       *io_psprogressdialog;		/* for showing output progress */

static GRAPHICS io_psblack = {LAYERO, BLACK, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
	0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};
CHAR *io_psstringheader[] =
{
	/*
		* Code to do super and subscripts:
		*
		* example:
		*	"NORMAL\dSUB\}   NORMAL\uSUP\}"
		*
		* will subscript "SUB" and superscript "SUP", so "\d"  starts a
		* subscript, "\u" starts a superscript, "\}" returns to
		* normal.  Sub-subscripts, and super-superscripts are not
		* supported.  To print a "\", use "\\".
		*
		* changes:
		*
		* all calls to stringwidth were changed to calls to StringLength,
		*    which returns the same info (assumes non-rotated text), but
		*    takes sub- and super-scripts into account.
		* all calls to show were changes to calls to StringShow, which
		*    handles sub- and super-scripts.
		* note that TSize is set to the text height, and is passed to
		*    StringLength and StringShow.
		*/
	x_("/ComStart 92 def"),								/* "\", enter command mode */
	x_("/ComSub  100 def"),								/* "d", start subscript */
	x_("/ComSup  117 def"),								/* "u", start superscript */
	x_("/ComNorm 125 def"),								/* "}", return to normal */
	x_("/SSSize .70 def"),								/* sub- and super-script size */
	x_("/SubDy  -.20 def"),								/* Dy for sub-script */
	x_("/SupDy   .40 def"),								/* Dy for super-script*/

	x_("/StringShow {"),								/* str size StringShow */
	x_("    /ComMode 0 def"),							/* command mode flag */
	x_("    /TSize exch def"),							/* text size */
	x_("    /TString exch def"),						/* string to draw */
	x_("    /NormY currentpoint exch pop def"),			/* save Y coord of string */
	x_("    TSize scaleFont"),
	x_("    TString {"),								/* scan string char by char */
	x_("        /CharCode exch def"),					/* save char */
	x_("        ComMode 1 eq {"),
	x_("            /ComMode 0 def"),					/* command mode */
	x_("            CharCode ComSub eq {"),				/* start subscript */
	x_("                TSize SSSize mul scaleFont"),
	x_("                currentpoint pop NormY TSize SubDy mul add moveto"),
	x_("            } if"),
	x_("            CharCode ComSup eq {"),				/* start superscript */
	x_("                TSize SSSize mul scaleFont"),
	x_("                currentpoint pop NormY TSize SupDy mul add moveto"),
	x_("            } if"),
	x_("            CharCode ComNorm eq {"),			/* end sub- or super-script */
	x_("                TSize scaleFont"),
	x_("                currentpoint pop NormY moveto"),
	x_("            } if"),
	x_("            CharCode ComStart eq {"),			/* print a "\" */
	x_("                ( ) dup 0 CharCode put show"),
	x_("            } if"),
	x_("        }"),
	x_("        {"),
	x_("            CharCode ComStart eq {"),
	x_("                /ComMode 1 def"),				/* enter command mode */
	x_("            }"),
	x_("            {"),
	x_("                ( ) dup 0 CharCode put show"),	/* print char */
	x_("            } ifelse"),
	x_("        } ifelse"),
	x_("    } forall "),
	x_("} def"),

	x_("/StringLength {"),								/* str size StringLength */
	x_("    /ComMode 0 def"),							/* command mode flag */
	x_("    /StrLen 0 def"),							/* total string length */
	x_("    /TSize exch def"),							/* text size */
	x_("    /TString exch def"),						/* string to draw */
	x_("    TSize scaleFont"),
	x_("    TString {"),								/* scan string char by char */
	x_("        /CharCode exch def"),					/* save char */
	x_("        ComMode 1 eq {"),
	x_("            /ComMode 0 def"),					/* command mode */
	x_("            CharCode ComSub eq {"),				/* start subscript */
	x_("                TSize SSSize mul scaleFont"),
	x_("            } if"),
	x_("            CharCode ComSup eq {"),				/* start superscript */
	x_("                TSize SSSize mul scaleFont"),
	x_("            } if"),
	x_("            CharCode ComNorm eq {"),			/* end sub- or super-script */
	x_("                TSize scaleFont"),
	x_("            } if"),
	x_("            CharCode ComStart eq {"),			/* add "\" to length */
	x_("                ( ) dup 0 CharCode put stringwidth pop StrLen add"),
	x_("                /StrLen exch def"),
	x_("            } if"),
	x_("        }"),
	x_("        {"),
	x_("            CharCode ComStart eq {"),
	x_("                /ComMode 1 def"),				/* enter command mode */
	x_("            }"),
	x_("            {"),								/* add char to length */
	x_("                ( ) dup 0 CharCode put stringwidth pop StrLen add"),
	x_("                /StrLen exch def"),
	x_("            } ifelse"),
	x_("        } ifelse"),
	x_("    } forall "),
	x_("    StrLen 0"),									/* return info like stringwidth */
	x_("} def"),

	x_("/Centerstring {"),								/* x y str sca */
	x_("    dup /TSize exch def"),						/* save size */
	x_("    dup scaleFont exch dup TSize StringLength"), /* x y sca str xw yw */
	x_("    pop 3 -1 roll .5 mul"),						/* x y str xw sca*.5 (was .8) */
	x_("    exch 5 -1 roll exch 2 div sub"),			/* y str sca*.5 x-xw/2 */
	x_("    exch 4 -1 roll exch 2 div sub"),			/* str x-xw/2 y-sca*.5/2 */
	x_("    moveto TSize StringShow"),
	x_("} def"),

	x_("/Topstring {"),									/* x y str sca */
	x_("    dup /TSize exch def"),						/* save size */
	x_("    dup scaleFont exch dup TSize StringLength"), /* x y sca str xw yw */
	x_("    pop 3 -1 roll .5 mul"),						/* x y str xw sca*.5 (was .8) */
	x_("    exch 5 -1 roll exch 2 div sub"),			/* y str sca*.5 x-xw/2 */
	x_("    exch 4 -1 roll exch sub"),					/* str x-xw/2 y-sca*.5 */
	x_("    moveto TSize StringShow"),
	x_("} def"),

	x_("/Botstring {"),									/* x y str sca */
	x_("    dup /TSize exch def"),						/* save size */
	x_("    scaleFont dup TSize StringLength pop"),		/* x y str xw */
	x_("    4 -1 roll exch 2 div sub"),					/* y str x-xw/2 */
	x_("    3 -1 roll moveto TSize StringShow"),
	x_("} def"),

	x_("/Leftstring {"),								/* x y str sca */
	x_("    dup /TSize exch def"),						/* save size */
	x_("    dup scaleFont .4 mul"),						/* x y str sca*.4 */
	x_("    3 -1 roll exch sub"),						/* x str y-sca*.4 */
	x_("    3 -1 roll exch"),							/* str x y-sca*.4 */
	x_("    moveto TSize StringShow"),
	x_("} def"),

	x_("/Rightstring {"),								/* x y str sca */
	x_("    dup /TSize exch def"),						/* save size */
	x_("    dup scaleFont exch dup TSize StringLength"), /* x y sca str xw yw */
	x_("    pop 3 -1 roll .4 mul"),						/* x y str xw sca*.4 */
	x_("    exch 5 -1 roll exch sub"),					/* y str sca*.4 x-xw */
	x_("    exch 4 -1 roll exch sub"),					/* str x-xw y-sca*.4 */
	x_("    moveto TSize StringShow"),
	x_("} def"),

	x_("/Topleftstring {"),								/* x y str sca */
	x_("    dup /TSize exch def"),						/* save size */
	x_("    dup scaleFont .5 mul"),						/* x y str sca*.5 (was .8) */
	x_("    3 -1 roll exch sub"),						/* x str y-sca*.5 */
	x_("    3 -1 roll exch"),							/* str x y-sca*.5 */
	x_("    moveto TSize StringShow"),
	x_("} def"),

	x_("/Toprightstring {"),							/* x y str sca */
	x_("    dup /TSize exch def"),						/* save size */
	x_("    dup scaleFont exch dup TSize StringLength"), /* x y sca str xw yw */
	x_("    pop 3 -1 roll .5 mul"),						/* x y str xw sca*.5 (was .8) */
	x_("    exch 5 -1 roll exch sub"),					/* y str sca*.5 x-xw */
	x_("    exch 4 -1 roll exch sub"),					/* str x-xw y-sca*.5 */
	x_("    moveto TSize StringShow"),
	x_("} def"),

	x_("/Botleftstring {"),								/* x y str sca */
	x_("    dup /TSize exch def"),						/* save size */
	x_("    scaleFont 3 1 roll moveto TSize StringShow"),
	x_("} def"),

	x_("/Botrightstring {"),							/* x y str sca */
	x_("    dup /TSize exch def"),						/* save size */
	x_("    scaleFont dup TSize StringLength"),
	x_("    pop 4 -1 roll exch"),
	x_("    sub 3 -1 roll"),
	x_("    moveto TSize StringShow"),
	x_("} def"),

	x_("/Min {"),										/* leave minimum of top two */
	x_("    dup 3 -1 roll dup"),
	x_("    3 1 roll gt"),
	x_("    {exch} if pop"),
	x_("} def"),

	x_("/Boxstring {"),									/* x y mx my str sca */
	x_("    dup /TSize exch def"),						/* save size */
	x_("    dup scaleFont"),							/* x y mx my str sca */
	x_("    exch dup TSize StringLength pop"),			/* x y mx my sca str xw */
	x_("    3 -1 roll dup"),							/* x y mx my str xw sca sca */
	x_("    6 -1 roll mul"),							/* x y my str xw sca sca*mx */
	x_("    3 -1 roll div"),							/* x y my str sca sca*mx/xw */
	x_("    4 -1 roll"),								/* x y str sca sca*mx/xw my */
	x_("    Min Min"),									/* x y str minsca */
	x_("    Centerstring"),
	x_("} def"),
0};

/* prototypes for local routines */
static void    io_psarc(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static void    io_pscircle(INTBIG, INTBIG, INTBIG, INTBIG);
static void    io_psdisc(INTBIG, INTBIG, INTBIG, INTBIG, GRAPHICS*);
static void    io_psdot(INTBIG, INTBIG);
static void    io_psdumpcontents(NODEPROTO *np);
static void    io_psdumpcells(NODEPROTO *np);
static CHAR   *io_pscellname(NODEPROTO *np);
static void    io_psline(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static CHAR    io_pspattern(GRAPHICS*);
static void    io_pspoly(POLYGON*, WINDOWPART*);
static void    io_pspolycount(POLYGON*, WINDOWPART*);
static void    io_pspolygon(INTBIG*, INTBIG*, INTBIG, GRAPHICS*);
static void    io_psputheader(INTBIG which);
static void    io_pssendfiletoprinter(CHAR *file);
static void    io_pstext(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, CHAR*, UINTBIG*);
static INTBIG  io_pstruefontsize(INTBIG font, WINDOWPART *w, TECHNOLOGY *tech);
static void    io_pswriteexplorer(WINDOWPART *win, BOOLEAN printit);
static BOOLEAN io_pswritecell(NODEPROTO*, CHAR*, BOOLEAN);
static void    io_psxform(INTBIG, INTBIG, INTBIG*, INTBIG*);

/*
 * Routine to free all memory associated with this module.
 */
void io_freepostscriptmemory(void)
{
	if (io_psgraphicsseenlimit != 0) efree((CHAR *)io_psgraphicsseen);
}

extern "C" { extern COMCOMP us_yesnop; }

/*
 * Routine to write out a PostScript library.
 * Actually prints the document if "printit" is TRUE.
 */
BOOLEAN io_writepostscriptlibrary(LIBRARY *lib, BOOLEAN printit)
{
	CHAR *par[MAXPARS];
	REGISTER INTBIG i, numsyncs, synchother, filestatus;
	REGISTER NODEPROTO *np;
	REGISTER LIBRARY *olib;
	REGISTER VARIABLE *var, *vardate;
	REGISTER UINTBIG mdate;

	/* see if there are synchronization links */
	synchother = 0;
	if (!printit)
	{
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			var = getvalkey((INTBIG)np, VNODEPROTO, VSTRING, io_postscriptfilenamekey);
			if (var != NOVARIABLE && np != lib->curnodeproto) synchother++;
		}
		if (synchother != 0)
		{
			i = ttygetparam(_("Would you like to synchronize all PostScript drawings?"),
				&us_yesnop, MAXPARS, par);
			if (i > 0 && namesamen(par[0], x_("no"), estrlen(par[0])) == 0)
				synchother = 0;
		}
	}

	np = lib->curnodeproto;
	if (np == NONODEPROTO && synchother == 0)
	{
		if ((el_curwindowpart->state&WINDOWTYPE) == EXPLORERWINDOW)
		{
			io_pswriteexplorer(el_curwindowpart, printit);
			return(TRUE);
		}
		ttyputerr(_("No current cell to plot"));
		return(TRUE);
	}

	/* just write the current cell if no synchronization needed */
	if (synchother == 0)
		return(io_pswritecell(np, 0, printit));

	/* synchronize all cells */
	numsyncs = 0;
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		var = getvalkey((INTBIG)np, VNODEPROTO, VSTRING, io_postscriptfilenamekey);
		if (var == NOVARIABLE) continue;

		/* existing file: check the date to see if it should be overwritten */
		vardate = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER, io_postscriptfiledatekey);
		if (vardate != NOVARIABLE)
		{
			mdate = (UINTBIG)vardate->addr;
			if (np->revisiondate <= mdate)
			{
				/*
				 * cell revision date is not newer than last PostScript output date
				 * ignore writing this file if it already exists
				 */
				filestatus = fileexistence((CHAR *)var->addr);
				if (filestatus == 1 || filestatus == 3) continue;
			}
		}
		if (io_pswritecell(np, (CHAR *)var->addr, FALSE)) return(TRUE);
		numsyncs++;

		/* this is tricky: because the "setvalkey" modifies the cell, the date must be set by hand */
		(void)setvalkey((INTBIG)np, VNODEPROTO, io_postscriptfiledatekey,
			(INTBIG)np->revisiondate, VINTEGER);
		vardate = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER, io_postscriptfiledatekey);
		if (vardate != NOVARIABLE)
			vardate->addr = (INTBIG)np->revisiondate;
	}
	if (numsyncs == 0)
		ttyputmsg(_("No PostScript files needed to be written"));

	return(FALSE);
}

/*
 * Routine to write the explorer window in "win".  It is automatically printed if "printit" is TRUE.
 */
void io_pswriteexplorer(WINDOWPART *win, BOOLEAN printit)
{
	CHAR file[100], *truename, *name, *header;
	REGISTER void *infstr;
	REGISTER INTBIG topmargin, ypos, page, lastboxxpos, boxsize, dashindent, xpos,
		fontHeight, pagewid, pagehei, yposbox, leftmargin, i, l, topypos, filetype;
	INTBIG indent, style;
	REGISTER VARIABLE *var;
	BOOLEAN *done;

	/* an explorer window: print text */
	us_explorerreadoutinit(win);

	filetype = io_filetypeps;
	if (printit)
	{
		estrcpy(file, x_("/tmp/ElectricPSOut.XXXXXX"));
		filetype |= FILETYPETEMPFILE;
	} else
	{
		estrcpy(file, x_("Explorer.ps"));
	}
	io_psout = xcreate(file, filetype, _("PostScript File"), &truename);
	if (io_psout == NULL)
	{
		ttyputerr(_("Cannot write temporary file %s"), file);
		return;
	}

	/* print the header */
	io_pswrite(x_("%%!PS-Adobe-1.0\n"));
	io_pswrite(x_("%%%%Title: Cell Explorer\n"));
	io_pswrite(x_("/Times-Roman findfont 10 scalefont setfont\n"));
	io_pswrite(x_("/Drawline {\n"));
	io_pswrite(x_("    newpath moveto lineto stroke} def\n"));

	var = getval((INTBIG)io_tool, VTOOL, VFRACT, x_("IO_postscript_width"));
	if (var == NOVARIABLE) pagewid = DEFAULTPSWIDTH; else
		pagewid = muldiv(var->addr, 75, WHOLE);
	var = getval((INTBIG)io_tool, VTOOL, VFRACT, x_("IO_postscript_height"));
	if (var == NOVARIABLE) pagehei = DEFAULTPSHEIGHT; else
		pagehei = muldiv(var->addr, 75, WHOLE);
	var = getval((INTBIG)io_tool, VTOOL, VFRACT, x_("IO_postscript_margin"));
	if (var == NOVARIABLE)
	{
		leftmargin = DEFAULTPSMARGIN;
	} else
	{
		leftmargin = muldiv(var->addr, 75, WHOLE);
	}

	fontHeight = 15;
	topmargin = fontHeight*6;
	ypos = pagehei - topmargin;
	page = 1;
	lastboxxpos = -1;
	boxsize = fontHeight * 2 / 3;
	dashindent = boxsize / 5;

	for(;;)
	{
		name = us_explorerreadoutnext(win, &indent, &style, &done, 0);
		if (name == 0) break;

		xpos = leftmargin + indent * fontHeight;
		yposbox = ypos + (fontHeight-boxsize) / 2;

		/* insert page breaks */
		if (ypos < topmargin)
		{
			ypos = pagehei - topmargin;
			io_pswrite(x_("showpage\n"));
		}

		/* print the header */
		if (ypos == pagehei - topmargin)
		{
			infstr = initinfstr();
			formatinfstr(infstr, _("Cell Explorer    Page %ld"), page);
			header = returninfstr(infstr);

			io_pswrite(x_("%ld %ld moveto "), pagewid/3, yposbox+fontHeight*2);
			io_pswritestring(header);
			io_pswrite(x_(" show\n"));
			page++;
		}

		/* draw the box */
		io_pswrite(x_("%ld %ld %ld %ld Drawline\n"), xpos,         yposbox,         xpos+boxsize, yposbox);
		io_pswrite(x_("%ld %ld %ld %ld Drawline\n"), xpos+boxsize, yposbox,         xpos+boxsize, yposbox+boxsize);
		io_pswrite(x_("%ld %ld %ld %ld Drawline\n"), xpos+boxsize, yposbox+boxsize, xpos,         yposbox+boxsize);
		io_pswrite(x_("%ld %ld %ld %ld Drawline\n"), xpos,         yposbox+boxsize, xpos,         yposbox);
		if (style >= 2)
		{
			/* draw horizontal line in box */
			io_pswrite(x_("%ld %ld %ld %ld Drawline\n"), xpos+dashindent, yposbox+boxsize/2,
				xpos+boxsize-dashindent, yposbox+boxsize/2);
			if (style == 2)
			{
				/* draw vertical line in box */
				io_pswrite(x_("%ld %ld %ld %ld Drawline\n"), xpos+boxsize/2, yposbox+dashindent,
					xpos+boxsize/2, yposbox+boxsize-dashindent);
			}
		}

		/* draw the connecting lines (dotted) */
		if (indent > 0)
		{
			io_pswrite(x_("[1 2] 0 setdash\n"));
			io_pswrite(x_("%ld %ld %ld %ld Drawline\n"), xpos, yposbox+boxsize/2,
				xpos-fontHeight+boxsize/2, yposbox+boxsize/2);
			for(i=0; i<indent; i++)
			{
				if (done[i]) continue;
				l = leftmargin+i*fontHeight+boxsize/2;
				if (l == lastboxxpos)
					topypos = yposbox+boxsize/2+fontHeight-boxsize/2; else
						topypos = yposbox+boxsize/2+fontHeight;
				io_pswrite(x_("%ld %ld %ld %ld Drawline\n"), l, yposbox+boxsize/2, l, topypos);
			}
			io_pswrite(x_("[] 0 setdash\n"));
		}
		lastboxxpos = xpos + boxsize/2;

		/* show the text */
		io_pswrite(x_("%ld %ld moveto "), xpos+boxsize+boxsize/2, yposbox);
		io_pswritestring(name);
		io_pswrite(x_(" show\n"));
		ypos -= fontHeight;
	}
	io_pswrite(x_("showpage\n"));
	xclose(io_psout);
	if (printit)
	{
		io_pssendfiletoprinter(truename);
	} else
	{
		ttyputmsg(_("%s written"), truename);
	}
}

/*
 * Routine to write the cell "np" to a PostScript file.  If "synchronize" is nonzero,
 * synchronize the cell with that file.  If "printit" is TRUE, queue this for print.
 */
BOOLEAN io_pswritecell(NODEPROTO *np, CHAR *synchronize, BOOLEAN printit)
{
	CHAR file[100], *truename, *ptr;
	INTBIG lx, hx, ly, hy, gridlx, gridly, gridx, gridy,
		cx, cy, i, j, psulx, psuhx, psuly, psuhy, sulx, suhx,
		suly, suhy, sslx, sshx, ssly, sshy, oldoptions;
	REGISTER INTBIG pagewid, pagehei, pagemarginps, pagemargin, *curstate, lambda;
	REGISTER BOOLEAN *layarr;
	REGISTER BOOLEAN plotdates, rotateplot, useplotter, hierpostscript, epsformat;
	INTBIG bblx, bbhx, bbly, bbhy, t1, t2, unitsx, unitsy;
	time_t curdate;
	REGISTER NODEPROTO *onp;
	REGISTER LIBRARY *lib;
	VARIABLE *var;
	REGISTER VARIABLE *varred, *vargreen, *varblue;
	static POLYGON *poly = NOPOLYGON;
	REGISTER void *infstr;

	/* cannot write text-only cells */
	if ((np->cellview->viewstate&TEXTVIEW) != 0)
	{
		if (!printit)
		{
			ttyputerr(_("Cannot write textual cells"));
			return(TRUE);
		}

		/* print a textual cell */
		estrcpy(file, x_("/tmp/ElectricPSOut.XXXXXX"));
		io_psout = xcreate(file, io_filetypeps|FILETYPETEMPFILE, 0, &truename);
		if (io_psout == NULL)
		{
			ttyputerr(_("Cannot write temporary file %s"), file);
			return(TRUE);
		}

		/* print the header */
		infstr = initinfstr();
		formatinfstr(infstr, _("Library: %s   Cell: %s\n"), np->lib->libname,
			describenodeproto(np));
		xprintf(io_psout, returninfstr(infstr));

		if ((us_useroptions&NODATEORVERSION) == 0)
		{
			infstr = initinfstr();
			if (np->creationdate != 0)
				formatinfstr(infstr, _("Created: %s"), timetostring((time_t)np->creationdate));
			if (np->revisiondate != 0)
				formatinfstr(infstr, _("   Revised: %s"), timetostring((time_t)np->revisiondate));
			xprintf(io_psout,  returninfstr(infstr));
		}
		xprintf(io_psout, x_("\n\n\n"));

		/* print the text of the cell */
		var = getvalkey((INTBIG)np, VNODEPROTO, VSTRING|VISARRAY, el_cell_message_key);
		if (var != NOVARIABLE)
		{
			j = getlength(var);
			for(i=0; i<j; i++)
			{
				ptr = ((CHAR **)var->addr)[i];
				xprintf(io_psout, x_("%s\n"), ptr);
			}
		}
		xclose(io_psout);
		io_pssendfiletoprinter(file);
		return(FALSE);
	}

	/* clear flags that tell whether headers have been included */
	io_psdotput = FALSE;
	io_psdrawlineput = FALSE;
	io_pspolygonput = FALSE;
	io_psfilledpolygonput = FALSE;
	io_psstringput = FALSE;

	/* get control options */
	epsformat = FALSE;
	hierpostscript = useplotter = rotateplot = plotdates = FALSE;
	io_psfake = FALSE;
	curstate = io_getstatebits();
	switch (curstate[0]&(PSCOLOR1|PSCOLOR2))
	{
		case 0:
			io_psusecolor = io_psusecolorstip = io_psusecolormerge = FALSE;
			break;
		case PSCOLOR1:
			io_psusecolor = TRUE;
			io_psusecolorstip = io_psusecolormerge = FALSE;
			break;
		case PSCOLOR1|PSCOLOR2:
			io_psusecolor = io_psusecolorstip = TRUE;
			io_psusecolormerge = FALSE;
			break;
		case PSCOLOR2:
			io_psusecolor = io_psusecolormerge = TRUE;
			io_psusecolorstip = FALSE;
			break;
	}
	if ((curstate[0]&EPSPSCRIPT) != 0) epsformat = TRUE;
	if ((curstate[0]&PSPLOTTER) != 0) useplotter = TRUE;
	if ((curstate[0]&PLOTDATES) != 0) plotdates = TRUE;
	if (printit) epsformat = FALSE;

	var = getval((INTBIG)io_tool, VTOOL, VFRACT, x_("IO_postscript_width"));
	if (var == NOVARIABLE) pagewid = DEFAULTPSWIDTH; else
		pagewid = muldiv(var->addr, 75, WHOLE);
	var = getval((INTBIG)io_tool, VTOOL, VFRACT, x_("IO_postscript_height"));
	if (var == NOVARIABLE) pagehei = DEFAULTPSHEIGHT; else
		pagehei = muldiv(var->addr, 75, WHOLE);
	var = getval((INTBIG)io_tool, VTOOL, VFRACT, x_("IO_postscript_margin"));
	if (var == NOVARIABLE)
	{
		pagemarginps = DEFAULTPSMARGIN;
		pagemargin = muldiv(DEFAULTPSMARGIN, WHOLE, 75);
	} else
	{
		pagemarginps = muldiv(var->addr, 75, WHOLE);
		pagemargin = var->addr;
	}

	/* cache color maps if using color */
	if (io_psusecolor)
	{
		varred = getval((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, x_("USER_colormap_red"));
		vargreen = getval((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, x_("USER_colormap_green"));
		varblue = getval((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, x_("USER_colormap_blue"));
		if (varred == NOVARIABLE || vargreen == NOVARIABLE || varblue == NOVARIABLE)
		{
			ttyputmsg(_("Cannot get colors!"));
			return(TRUE);
		}
		io_redmap = (INTBIG *)varred->addr;
		io_greenmap = (INTBIG *)vargreen->addr;
		io_bluemap = (INTBIG *)varblue->addr;
	}

	/* determine the area of interest */
	if (io_getareatoprint(np, &lx, &hx, &ly, &hy, FALSE)) return(TRUE);

	if ((curstate[0]&PSROTATE) != 0) rotateplot = TRUE; else
	{
		if ((curstate[1]&PSAUTOROTATE) != 0)
		{
			if (((pagehei > pagewid || useplotter) && hx-lx > hy-ly) ||
				(pagewid > pagehei && hy-ly > hx-lx)) rotateplot = TRUE;
		}
	}

	/* if plotting, compute height from width */
	if (useplotter)
	{
		if (rotateplot)
		{
			pagehei = muldiv(pagewid, hx-lx, hy-ly);
		} else
		{
			pagehei = muldiv(pagewid, hy-ly, hx-lx);
		}
	}

	/* create the PostScript file */
	if (printit)
	{
		estrcpy(file, x_("/tmp/ElectricPSOut.XXXXXX"));
		io_psout = xcreate(file, io_filetypeps|FILETYPETEMPFILE, 0, &truename);
		if (io_psout == NULL)
		{
			ttyputerr(_("Cannot write temporary file %s"), file);
			return(TRUE);
		}
	} else if (synchronize != 0)
	{
		(void)esnprintf(file, 100, x_("%s"), synchronize);
		io_psout = xcreate(file, io_filetypeps, 0, &truename);
		if (io_psout == NULL)
		{
			ttyputerr(_("Cannot synchronize cell %s with file %s"),
				describenodeproto(np), file);
			return(TRUE);
		}
	} else
	{
		(void)esnprintf(file, 100, x_("%s.%s"), np->protoname,
			epsformat ? x_("eps") : x_("ps"));
		io_psout = xcreate(file, io_filetypeps, _("PostScript File"), &truename);
		if (io_psout == NULL)
		{
			if (truename != 0) ttyputerr(_("Cannot create %s"), truename);
			return(TRUE);
		}
	}

	/* for pure color plotting, use special merging code */
	if (io_psusecolormerge)
	{
		io_pscolorplot(np, epsformat, useplotter, pagewid, pagehei, pagemarginps);
		xclose(io_psout);
		if (printit)
		{
			io_pssendfiletoprinter(file);
		} else
		{
			ttyputmsg(_("%s written"), truename);
		}
		return(FALSE);
	}

	if (io_verbose < 0)
	{
		io_psprogressdialog = DiaInitProgress(_("Preparing PostScript Output..."), 0);
		if (io_psprogressdialog == 0) return(TRUE);
		DiaSetProgress(io_psprogressdialog, 0, 1);
	}

	/* build pseudowindow for text scaling */
	psulx = psuly = 0;
	psuhx = psuhy = 1000;
	io_pswindow.uselx = psulx;
	io_pswindow.usely = psuly;
	io_pswindow.usehx = psuhx;
	io_pswindow.usehy = psuhy;
	io_pswindow.screenlx = lx;
	io_pswindow.screenhx = hx;
	io_pswindow.screenly = ly;
	io_pswindow.screenhy = hy;
	io_pswindow.state = DISPWINDOW;
	computewindowscale(&io_pswindow);

	/* set the bounding box in internal units */
	bblx = lx;  bbhx = hx;  bbly = ly;  bbhy = hy;

	/* PostScript: compute the transformation matrix */
	cx = (hx + lx) / 2;
	cy = (hy + ly) / 2;
	unitsx = (pagewid-pagemarginps*2) * PSSCALE;
	unitsy = (pagehei-pagemarginps*2) * PSSCALE;
	if (epsformat)
	{
		var = getvalkey((INTBIG)np, VNODEPROTO, VFRACT, io_postscriptepsscalekey);
		if (var != NOVARIABLE && var->addr != 0)
		{
			unitsx = muldiv(unitsx, var->addr, WHOLE*2);
			unitsy = muldiv(unitsy, var->addr, WHOLE*2);
		}
	}
	if (useplotter)
	{
		i = muldiv(unitsx, 65536, hx - lx);
		j = muldiv(unitsx, 65536, hy - ly);
	} else
	{
		i = mini(muldiv(unitsx, 65536, hx - lx), muldiv(unitsy, 65536, hy - ly));
		j = mini(muldiv(unitsx, 65536, hy - ly), muldiv(unitsy, 65536, hx - lx));
	}
	if (rotateplot) i = j;
	io_psmatrix[0][0] = i;   io_psmatrix[0][1] = 0;
	io_psmatrix[1][0] = 0;   io_psmatrix[1][1] = i;
	io_psmatrix[2][0] = - muldiv(i, cx, 65536) + unitsx / 2 + pagemarginps * PSSCALE;
	if (useplotter)
	{
		io_psmatrix[2][1] = - muldiv(i, ly, 65536) + pagemarginps * PSSCALE;
	} else
	{
		io_psmatrix[2][1] = - muldiv(i, cy, 65536) + unitsy / 2 + pagemarginps * PSSCALE;
	}

	/* write PostScript header */
	if (epsformat) io_pswrite(x_("%%!PS-Adobe-2.0 EPSF-2.0\n")); else
		io_pswrite(x_("%%!PS-Adobe-1.0\n"));
	io_pswrite(x_("%%%%Title: %s\n"), describenodeproto(np));
	if ((us_useroptions&NODATEORVERSION) == 0)
	{
		io_pswrite(x_("%%%%Creator: Electric VLSI Design System version %s\n"), el_version);
		curdate = getcurrenttime();
		io_pswrite(x_("%%%%CreationDate: %s\n"), timetostring(curdate));
	} else
	{
		io_pswrite(x_("%%%%Creator: Electric VLSI Design System\n"));
	}
	if (epsformat) io_pswrite(x_("%%%%Pages: 0\n")); else
		io_pswrite(x_("%%%%Pages: 1\n"));

	/* transform to PostScript units */
	io_psxform(bblx, bbly, &bblx, &bbly);
	io_psxform(bbhx, bbhy, &bbhx, &bbhy);

	if (rotateplot)
	{
		/*
		 * fiddle with the bbox if image rotated on page
		 * (at this point, bbox coordinates are absolute printer units)
		 */
		t1 = bblx;
		t2 = bbhx;
		bblx = -bbhy + muldiv(pagehei, 300, 75);
		bbhx = -bbly + muldiv(pagehei, 300, 75);
		bbly = t1 + muldiv(pagemargin*2, 300, 75);
		bbhy = t2 + muldiv(pagemargin*2, 300, 75);
	}

	if (bblx > bbhx) { i = bblx;  bblx = bbhx;  bbhx = i; }
	if (bbly > bbhy) { i = bbly;  bbly = bbhy;  bbhy = i; }
	bblx = roundfloat(((float)bblx) / (PSSCALE * 75.0f) * 72.0f) * (bblx>=0 ? 1 : -1);
	bbly = roundfloat(((float)bbly) / (PSSCALE * 75.0f) * 72.0f) * (bbly>=0 ? 1 : -1);
	bbhx = roundfloat(((float)bbhx) / (PSSCALE * 75.0f) * 72.0f) * (bbhx>=0 ? 1 : -1);
	bbhy = roundfloat(((float)bbhy) / (PSSCALE * 75.0f) * 72.0f) * (bbhy>=0 ? 1 : -1);

	/*
	 * Increase the size of the bbox by one "pixel" to
	 * prevent the edges from being obscured by some drawing tools
	 */
	io_pswrite(x_("%%%%BoundingBox: %ld %ld %ld %ld\n"), bblx-1, bbly-1, bbhx+1, bbhy+1);
	io_pswrite(x_("%%%%DocumentFonts: Times-Roman\n"));
	io_pswrite(x_("%%%%EndComments\n"));

	/* PostScript: add some debugging info */
	if (np != NONODEPROTO)
	{
		io_pswrite(x_("%% cell dimensions: %ld wide x %ld high (database units)\n"),
			np->highx-np->lowx, np->highy-np->lowy);
		io_pswrite(x_("%% origin: %ld %ld\n"), np->lowx, np->lowy);
	}

	/* disclaimers */
	if (epsformat)
	{
		io_pswrite(x_("%% The EPS header should declare a private dictionary.\n"));
	} else
	{
		io_pswrite(x_("%% The non-EPS header does not claim conformance to Adobe-2.0\n"));
		io_pswrite(x_("%% because the structure may not be exactly correct.\n"));
	}
	io_pswrite(x_("%%\n"));

	/* set the page size if this is a plotter */
	if (useplotter)
	{
		io_pswrite(x_("<< /PageSize [%ld %ld] >> setpagedevice\n"),
			muldiv(pagewid, 72, 75), muldiv(pagehei, 72, 75));
	}

	/* make the scale be exactly equal to one page pixel */
	io_pswrite(x_("72 %ld div 72 %ld div scale\n"), PSSCALE*75, PSSCALE*75);

	/* set the proper typeface */
	io_pswrite(x_("/DefaultFont /Times-Roman def\n"));
	io_pswrite(x_("/scaleFont {\n"));
	io_pswrite(x_("    DefaultFont findfont\n"));
	io_pswrite(x_("    exch scalefont setfont} def\n"));

	/* make the line width proper */
	io_pswrite(x_("%ld setlinewidth\n"), PSSCALE/2);

	/* make the line ends look right */
	io_pswrite(x_("1 setlinecap\n"));

	/* rotate the image if requested */
	if (rotateplot)
	{
		if (useplotter)
		{
			io_pswrite(x_("%s 300 mul %s 300 mul translate 90 rotate\n"),
				frtoa(muldiv(pagewid, WHOLE, 75)),
				frtoa(muldiv((pagehei-pagewid)/2, WHOLE, 75)));
		} else
		{
			io_pswrite(x_("%s 300 mul %s 300 mul translate 90 rotate\n"),
				frtoa(muldiv((pagehei+pagewid)/2, WHOLE, 75)),
				frtoa(muldiv((pagehei-pagewid)/2, WHOLE, 75)));
		}
	}

	/* initialize list of GRAPHICS modules that have been put out */
	io_psgraphicsseencount = 0;

	/* plot everything */
	sulx = el_curwindowpart->uselx;      suhx = el_curwindowpart->usehx;
	suly = el_curwindowpart->usely;      suhy = el_curwindowpart->usehy;
	sslx = el_curwindowpart->screenlx;   sshx = el_curwindowpart->screenhx;
	ssly = el_curwindowpart->screenly;   sshy = el_curwindowpart->screenhy;
	el_curwindowpart->uselx = io_pswindow.uselx;
	el_curwindowpart->usehx = io_pswindow.usehx;
	el_curwindowpart->usely = io_pswindow.usely;
	el_curwindowpart->usehy = io_pswindow.usehy;
	el_curwindowpart->screenlx = io_pswindow.screenlx;
	el_curwindowpart->screenhx = io_pswindow.screenhx;
	el_curwindowpart->screenly = io_pswindow.screenly;
	el_curwindowpart->screenhy = io_pswindow.screenhy;
	computewindowscale(el_curwindowpart);
	io_lastred = io_lastgreen = io_lastblue = -1;

	if (!hierpostscript)
	{
		/* flat PostScript: disable "tiny cell" removal */
		oldoptions = us_useroptions;
		us_useroptions |= DRAWTINYCELLS;

		/* count the number of polygons in the job */
		io_pspolygoncount = 0;
		if (io_verbose < 0)
		{
			(void)asktool(us_tool, x_("display-to-routine"), io_pspolycount);
			io_pspolygontotal = io_pspolygoncount;
			if (io_pspolygontotal == 0) io_pspolygontotal = 1;
			DiaSetTextProgress(io_psprogressdialog, _("Writing PostScript..."));
		}
		io_pspolygoncount = 0;

		io_psprefix[0] = 0;
		if (io_psusecolor)
		{
			/* color: plot layers in proper order */
			io_maxpslayer = io_setuptechorder(el_curtech);
			io_pspolygontotal *= (io_maxpslayer+1);
			for(i=0; i<io_maxpslayer; i++)
			{
				if (stopping(STOPREASONDECK)) break;
				io_whichlayer = io_nextplotlayer(i) + 1;
				(void)asktool(us_tool, x_("display-to-routine"), io_pspoly);
			}
			if (stopping(STOPREASONDECK))
			{
				xclose(io_psout);
				return(FALSE);
			}
			io_whichlayer = 0;
			(void)asktool(us_tool, x_("display-to-routine"), io_pspoly);
		} else
		{
			/* gray-scale: just plot it once */
			io_whichlayer = -1;
			(void)asktool(us_tool, x_("display-to-routine"), io_pspoly);
		}

		/* restore "tiny cell" removal option */
		us_useroptions = oldoptions;
	} else
	{
		/* hierarchical PostScript: figure out which layers are there */
		if (io_psusecolor)
		{
			/* color: setup proper order for plotting */
			io_maxpslayer = io_setuptechorder(el_curtech);

			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			{
				for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
				{
					onp->temp1 = 0;
					layarr = (BOOLEAN *)emalloc((io_maxpslayer+1) * (sizeof (BOOLEAN)),
						io_tool->cluster);
					if (layarr == 0) return(TRUE);
					for(i=0; i<=io_maxpslayer; i++) layarr[i] = FALSE;
					onp->temp2 = (INTBIG)layarr;
				}
			}
		}

		/* count the number of symbols used by the code */
		io_psopcount = 0;
		io_psfake = TRUE;
		io_psdumpcells(el_curwindowpart->curnodeproto);
		io_psfake = FALSE;
		io_psgraphicsseencount = 0;

		/* clear flags of which cells have been written */
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
				onp->temp1 = 0;

		if (io_verbose < 0)
			DiaSetTextProgress(io_psprogressdialog, _("Writing PostScript..."));
		io_pspolygontotal = 0;

#ifdef DEBUGHIERPS
		io_pswrite(x_("/ypos 100 def\n"));
		io_pswrite(x_("24 scaleFont\n"));
		io_pswrite(x_("300 ypos moveto (max) show\n"));
		io_pswrite(x_("100 ypos moveto (used) show\n"));
		io_pswrite(x_("/dovmstats {\n"));
		io_pswrite(x_("   ypos 26 add /ypos exch def\n"));
		io_pswrite(x_("   ypos 3200 gt\n"));
		io_pswrite(x_("     { /ypos 100 def showpage 72 300 div 72 300 div scale 24 scaleFont} if\n"));
		io_pswrite(x_("   vmstatus\n"));
		io_pswrite(x_("   300 ypos moveto 50 string cvs show\n"));
		io_pswrite(x_("   100 ypos moveto 50 string cvs show\n"));
		io_pswrite(x_("   pop\n"));
		io_pswrite(x_("   500 ypos moveto show\n"));
		io_pswrite(x_("} def\n"));
#endif

		/* put out all macros first */
		io_psputheader(PSHEADERDOT);
		io_psputheader(PSHEADERLINE);
		io_psputheader(PSHEADERPOLYGON);
		io_psputheader(PSHEADERFPOLYGON);
		io_psputheader(PSHEADERSTRING);

		/* dump all necessary cells */
		estrcpy(io_psprefix, x_("    "));
		io_psdumpcells(el_curwindowpart->curnodeproto);

		/* dump the current cell */
		io_pswrite(x_("\n%% Invocation of top-level cell\n"));
#ifdef DEBUGHIERPS
		io_pswrite(x_("(FINISHED) dovmstats\n"));
#else
		if (io_psusecolor)
		{
			for(i=0; i<io_maxpslayer; i++)
			{
				io_whichlayer = io_nextplotlayer(i) + 1;
				if (((BOOLEAN *)np->temp2)[io_whichlayer])
					io_pswrite(x_("%s\n"), io_pscellname(np));
			}
			io_whichlayer = 0;
			if (((BOOLEAN *)np->temp2)[io_whichlayer])
				io_pswrite(x_("%s\n"), io_pscellname(np));
		} else
		{
			io_pswrite(x_("%s\n"), io_pscellname(np));
		}
#endif

		/* free memory */
		if (io_psusecolor)
		{
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
				for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
					efree((CHAR *)onp->temp2);
		}
		ttyputmsg(M_("Hierarchical PostScript uses %ld units of VM"), io_psopcount);
	}

	/* draw the grid if requested */
	if ((el_curwindowpart->state&(GRIDON|GRIDTOOSMALL)) == GRIDON)
	{
		lambda = np->lib->lambda[np->tech->techindex];
		gridx = muldiv(el_curwindowpart->gridx, lambda, WHOLE);
		gridy = muldiv(el_curwindowpart->gridy, lambda, WHOLE);
		var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_gridfloatskey);
		if (var == NOVARIABLE || var->addr == 0)
		{
			gridlx = lx  / gridx * gridx;
			gridly = ly  / gridy * gridy;
		} else
		{
			grabpoint(np, &gridlx, &gridly);
			gridalign(&gridlx, &gridly, 1, np);
			gridlx += gridlx / gridx * gridx;
			gridly += gridly / gridy * gridy;
		}

		/* adjust to ensure that the first point is inside the range */
		if (gridlx > lx) gridlx -= (gridlx - lx) / gridx * gridx;
		if (gridly > ly) gridly -= (gridly - ly) / gridy * gridy;
		while (gridlx < lx) gridlx += gridx;
		while (gridly < ly) gridly += gridy;

		/* PostScript: write the grid loop */
		io_pswrite(x_("%ld %ld %ld\n{\n"), gridlx, gridx, hx);
		io_pswrite(x_("    %ld %ld %ld\n    {\n"), gridly, gridy, hy);	/* x y */
		io_pswrite(x_("        dup 3 -1 roll dup dup\n"));				/* y y x x x */
		io_pswrite(x_("        5 1 roll 3 1 roll\n"));					/* x y x y x */
		io_pswrite(x_("        %ld mul exch %ld mul add 65536 div %ld add\n"),
			io_psmatrix[0][0], io_psmatrix[1][0],
			io_psmatrix[2][0]);										/* x y x x' */
		io_pswrite(x_("        3 1 roll\n"));							/* x x' y x */
		io_pswrite(x_("        %ld mul exch %ld mul add 65536 div %ld add\n"),
			io_psmatrix[0][1], io_psmatrix[1][1],
			io_psmatrix[2][1]);										/* x x' y' */
		io_pswrite(x_("        newpath moveto 0 0 rlineto stroke\n"));
		io_pswrite(x_("    } for\n"));
		io_pswrite(x_("} for\n"));
	}

	/* draw frame if it is there */
	j = framepolys(np);
	if (j != 0)
	{
		(void)needstaticpolygon(&poly, 4, io_tool->cluster);
		io_whichlayer = -1;
		for(i=0; i<j; i++)
		{
			framepoly(i, poly, np);
			(void)io_pspoly(poly, el_curwindowpart);
		}
	}

	/* put out dates if requested */
	if (plotdates)
	{
		infstr = initinfstr();
		formatinfstr(infstr, _("Cell: %s"), describenodeproto(np));
		io_pswrite(x_("%s%ld %ld "), io_psprefix, io_pswindow.uselx,
			io_pswindow.usely + 2 * CORNERDATESIZE * PSSCALE);
		io_pswritestring(returninfstr(infstr));
		io_pswrite(x_(" %ld Botleftstring\n"), CORNERDATESIZE * PSSCALE);

		infstr = initinfstr();
		formatinfstr(infstr, _("Created: %s"), timetostring((time_t)np->creationdate));
		io_pswrite(x_("%s%ld %ld "), io_psprefix, io_pswindow.uselx,
			io_pswindow.usely + CORNERDATESIZE * PSSCALE);
		io_pswritestring(returninfstr(infstr));
		io_pswrite(x_(" %ld Botleftstring\n"), CORNERDATESIZE * PSSCALE);

		infstr = initinfstr();
		formatinfstr(infstr, _("Revised: %s"), timetostring((time_t)np->revisiondate));
		io_pswrite(x_("%s%ld %ld "), io_psprefix, io_pswindow.uselx, io_pswindow.usely);
		io_pswritestring(returninfstr(infstr));
		io_pswrite(x_(" %ld Botleftstring\n"), CORNERDATESIZE * PSSCALE);
	}

	io_pswrite(x_("showpage\n"));
	io_pswrite(x_("%%%%Trailer\n"));
	xclose(io_psout);
	if (io_verbose < 0)
		DiaDoneProgress(io_psprogressdialog);

	/* restore window */
	el_curwindowpart->uselx = sulx;
	el_curwindowpart->usehx = suhx;
	el_curwindowpart->usely = suly;
	el_curwindowpart->usehy = suhy;
	el_curwindowpart->screenlx = sslx;   el_curwindowpart->screenhx = sshx;
	el_curwindowpart->screenly = ssly;   el_curwindowpart->screenhy = sshy;
	computewindowscale(el_curwindowpart);

	/* print the PostScript if requested */
	if (printit)
	{
		io_pssendfiletoprinter(file);
	} else
	{
		ttyputmsg(_("%s written"), truename);
	}
	return(FALSE);
}

void io_pssendfiletoprinter(CHAR *file)
{
	CHAR *printer, *defprinter;
	REGISTER VARIABLE *var;
	REGISTER void *infstr;

	/* determine printer to use */
	defprinter = 0;
	var = getval((INTBIG)io_tool, VTOOL, VSTRING, x_("IO_default_printer"));
	if (var != NOVARIABLE) defprinter = (CHAR *)var->addr; else
	{
		printer = egetenv(x_("PRINTER"));
		if (printer != 0 && *printer != 0) defprinter = printer;
	}

	EProcess pr_process;
	pr_process.addArgument( x_("lpr") );
	if (defprinter != 0)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("-P"));
		addstringtoinfstr(infstr, defprinter);
		pr_process.addArgument( returninfstr(infstr) );
	}
	pr_process.addArgument(	file );
	pr_process.setCommunication( FALSE, FALSE, FALSE );
	pr_process.start();
	pr_process.wait();
	if (defprinter == 0) ttyputmsg(_("Print queued")); else
		ttyputmsg(_("Printing to %s"), defprinter);
}

/*
 * Routine to determine the PostScript name of cell "np".
 */
CHAR *io_pscellname(NODEPROTO *np)
{
	REGISTER CHAR *ch;
	REGISTER void *infstr;

	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("Cell_"));
	addstringtoinfstr(infstr, np->protoname);
	if (np->cellview != el_unknownview)
	{
		addtoinfstr(infstr, '_');
		addstringtoinfstr(infstr, np->cellview->sviewname);
	}
	if (io_psusecolor)
	{
		addtoinfstr(infstr, '_');
		if (io_whichlayer == 0) addstringtoinfstr(infstr, x_("REST")); else
		{
			ch = layername(el_curtech, io_whichlayer-1);
			addstringtoinfstr(infstr, ch);
		}
	}
	return(returninfstr(infstr));
}

/*
 * Routine to recursively dump the contents of cell "np" and its contents.
 */
void io_psdumpcells(NODEPROTO *np)
{
	REGISTER NODEINST *ni;
	REGISTER INTBIG i;

	if (stopping(STOPREASONDECK)) return;
	if (np->temp1 != 0) return;
	np->temp1 = 1;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex != 0) continue;
		if ((ni->userbits&NEXPAND) == 0) continue;
		io_psdumpcells(ni->proto);
		if (io_psusecolor && io_psfake)
		{
			for(i=0; i<=io_maxpslayer; i++)
				((BOOLEAN *)np->temp2)[i] |= ((BOOLEAN *)ni->proto->temp2)[i];
		}
	}

	if (!io_psfake)
	{
		io_pswrite(x_("\n%% Definition of cell %s\n"), describenodeproto(np));
#ifdef DEBUGHIERPS
		io_pswrite(x_("(%s) dovmstats\n"), np->protoname);
#endif
	}
	if (io_psusecolor)
	{
		/* color: plot layers in proper order */
		for(i=0; i<io_maxpslayer; i++)
		{
			io_whichlayer = io_nextplotlayer(i) + 1;
			if (io_psfake || ((BOOLEAN *)np->temp2)[io_whichlayer])
				io_psdumpcontents(np);
		}
		io_whichlayer = 0;
		if (io_psfake || ((BOOLEAN *)np->temp2)[0])
			io_psdumpcontents(np);
	} else
	{
		io_psdumpcontents(np);
	}
}

/*
 * Routine to dump the contents of cell "np".
 */
void io_psdumpcontents(NODEPROTO *np)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp;
	static POLYGON *poly = NOPOLYGON;
	REGISTER INTBIG i, tot, cx, cy, tx, ty, ptx, pty;
	INTBIG pcx, pcy;
	XARRAY trans;
	void (*savepolyroutine)(POLYGON*, WINDOWPART*);
	INTBIG savestate;

	if (stopping(STOPREASONDECK)) return;
	if (io_psfake) io_psopcount += 2; else
		io_pswrite(x_("/%s {\n"), io_pscellname(np));
	(void)needstaticpolygon(&poly, 4, io_tool->cluster);
	io_lastred = io_lastgreen = io_lastblue = -1;

	/* first write calls to subcells */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex != 0) continue;
		if ((ni->userbits&NEXPAND) == 0) continue;
		if (io_psusecolor && !((BOOLEAN *)ni->proto->temp2)[io_whichlayer])
			continue;

		/* expanded: invoke lower level cell */
		tx = (ni->lowx + ni->highx - ni->proto->lowx - ni->proto->highx)/2;
		ty = (ni->lowy + ni->highy - ni->proto->lowy - ni->proto->highy)/2;
		ptx = muldiv(tx, io_psmatrix[0][0], 65536) + muldiv(ty, io_psmatrix[1][0], 65536);
		pty = muldiv(tx, io_psmatrix[0][1], 65536) + muldiv(ty, io_psmatrix[1][1], 65536);

		if (ni->rotation == 0 && ni->transpose == 0)
		{
			if (io_psfake) io_psopcount += 7; else
			{
				io_pswrite(x_("%s%ld %ld translate %s %ld %ld translate\n"),
					io_psprefix, ptx, pty, io_pscellname(ni->proto), -ptx, -pty);
			}
		} else
		{
			cx = (ni->proto->lowx + ni->proto->highx)/2;
			cy = (ni->proto->lowy + ni->proto->highy)/2;
			io_psxform(cx, cy, &pcx, &pcy);

			if (io_psfake)
			{
				io_psopcount += 13;
				if (ni->transpose != 0) io_psopcount += 10;
				if (ni->rotation != 0) io_psopcount += 4;
			} else
			{
				io_pswrite(x_("%s%ld %ld translate"), io_psprefix, ptx+pcx, pty+pcy);
				if (ni->transpose != 0)
					io_pswrite(x_(" 90 rotate -1 1 scale"));
				if (ni->rotation != 0)
					io_pswrite(x_(" %g rotate"), (float)ni->rotation/10.0f);
				io_pswrite(x_(" %ld %ld translate\n"), -pcx, -pcy);
				io_pswrite(x_("%s    %s\n"), io_psprefix, io_pscellname(ni->proto));
				io_pswrite(x_("%s%ld %ld translate"), io_psprefix, pcx, pcy);
				if (ni->rotation != 0)
					io_pswrite(x_(" %g rotate"), -(float)ni->rotation/10.0f);
				if (ni->transpose != 0)
					io_pswrite(x_(" -1 1 scale -90 rotate"));
				io_pswrite(x_(" %ld %ld translate\n"), -ptx-pcx, -pty-pcy);
			}
		}
	}

	/* now write primitives and unexpanded subcells */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex == 0)
		{
			/* subcell */
			if ((ni->userbits&NEXPAND) == 0)
			{
				if (io_psusecolor && io_psfake)
					((BOOLEAN *)np->temp2)[0] = TRUE;
				makerot(ni, trans);
				maketruerectpoly(ni->lowx, ni->highx, ni->lowy, ni->highy, poly);
				xformpoly(poly, trans);
				poly->style = CLOSEDRECT;
				poly->desc = &io_psblack;
				poly->layer = -1;
				io_pspoly(poly, el_curwindowpart);

				poly->style = TEXTCENT;
				TDCLEAR(poly->textdescript);
				TDSETSIZE(poly->textdescript, TXTSETQLAMBDA(12));
				poly->string = describenodeproto(ni->proto);
				io_pspoly(poly, el_curwindowpart);
			}
		} else
		{
			/* primitive */
			makerot(ni, trans);
			tot = nodepolys(ni, 0, NOWINDOWPART);
			for(i=0; i<tot; i++)
			{
				shapenodepoly(ni, i, poly);
				if (io_psusecolor && io_psfake)
				{
					if (poly->desc->bits != LAYERN && poly->desc->col != ALLOFF &&
						(poly->desc->colstyle&INVISIBLE) == 0)
							((BOOLEAN *)np->temp2)[poly->layer+1] = TRUE;
				}
				xformpoly(poly, trans);
				io_pspoly(poly, el_curwindowpart);
			}
		}
	}

	/* now write arcs */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		tot = arcpolys(ai, NOWINDOWPART);
		for(i=0; i<tot; i++)
		{
			shapearcpoly(ai, i, poly);
			if (io_psusecolor && io_psfake)
			{
				if (poly->desc->bits != LAYERN && poly->desc->col != ALLOFF &&
					(poly->desc->colstyle&INVISIBLE) == 0)
						((BOOLEAN *)np->temp2)[poly->layer+1] = TRUE;
			}
			io_pspoly(poly, el_curwindowpart);
		}
	}


	/* save and alter graphics state so that text can be dumped */
	if (np == el_curwindowpart->curnodeproto)
	{
		savestate = el_curwindowpart->state;
		el_curwindowpart->state = (el_curwindowpart->state & ~WINDOWTYPE) | DISPWINDOW;
		savepolyroutine = us_displayroutine;
		us_displayroutine = io_pspoly;

		/* write cell text */
		us_drawnodeprotovariables(np, el_matid, el_curwindowpart, FALSE);

		/* write port text */
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			makerot(pp->subnodeinst, trans);
			us_writeprotoname(pp, LAYERA, trans, LAYERO,
				el_colcelltxt, el_curwindowpart, 0, 0, 0, 0,
					(us_useroptions&PORTLABELS) >> PORTLABELSSH);
			us_drawportprotovariables(pp, 1, trans, el_curwindowpart, FALSE);
		}

		/* restore graphics state */
		el_curwindowpart->state = savestate;
		us_displayroutine = savepolyroutine;
	}

	if (io_psfake) io_psopcount += 2; else
		io_pswrite(x_("} def\n"));
}

/*
 * coroutine to count the number of polygons that will be plotted (for progress info).
 */
void io_pspolycount(POLYGON *poly, WINDOWPART *win)
{
	io_pspolygoncount++;
}

/*
 * coroutine to plot the polygon "poly"
 */
void io_pspoly(POLYGON *poly, WINDOWPART *win)
{
	REGISTER INTBIG red, green, blue, k, type, size;
	REGISTER float r, g, b;
	INTBIG xl, xh, yl, yh, x, y, listx[4], listy[4];
	REGISTER TECHNOLOGY *tech;

	io_pspolygoncount++;
	if (io_pspolygontotal > 0 && (io_pspolygoncount % 100) == 99)
	{
		if (io_verbose < 0)
			DiaSetProgress(io_psprogressdialog, io_pspolygoncount, io_pspolygontotal);
	}

	/* ignore null layers */
	if (poly->desc->bits == LAYERN || poly->desc->col == ALLOFF) return;
	if ((poly->desc->colstyle&INVISIBLE) != 0) return;

	/* ignore grids */
	if (poly->style == GRIDDOTS) return;

	/* ignore layers that are not supposed to be dumped at this time */
	if (io_whichlayer >= 0)
	{
		if (io_whichlayer == 0)
		{
			if (poly->tech == el_curtech)
			{
				for(k=0; k<io_maxpslayer; k++) if (io_nextplotlayer(k) == poly->layer)
					return;
			}
		} else
		{
			if (poly->tech != el_curtech || io_whichlayer-1 != poly->layer)
				return;
		}
	}

	/* set color if requested */
	if (io_psusecolor)
	{
		red = io_redmap[poly->desc->col];
		green = io_greenmap[poly->desc->col];
		blue = io_bluemap[poly->desc->col];
		if (red != io_lastred || green != io_lastgreen || blue != io_lastblue)
		{
			io_lastred = red;
			io_lastgreen = green;
			io_lastblue = blue;
			if (io_psfake) io_psopcount += 4; else
			{
				r = (float)red / 255.0f;
				g = (float)green / 255.0f;
				b = (float)blue / 255.0f;
				io_pswrite(x_("%s%g %g %g setrgbcolor\n"), io_psprefix, r, g, b);
			}
		}
	}

	switch (poly->style)
	{
		case FILLED:
		case FILLEDRECT:
			if (isbox(poly, &xl, &xh, &yl, &yh))
			{
				if (xl == xh)
				{
					if (yl == yh) io_psdot(xl, yl); else
						io_psline(xl, yl, xl, yh, 0);
					break;
				} else if (yl == yh)
				{
					io_psline(xl, yl, xh, yl, 0);
					break;
				}
				listx[0] = xl;   listy[0] = yl;
				listx[1] = xl;   listy[1] = yh;
				listx[2] = xh;   listy[2] = yh;
				listx[3] = xh;   listy[3] = yl;
				io_pspolygon(listx, listy, 4, poly->desc);
			} else
			{
				if (poly->count == 1)
				{
					io_psdot(poly->xv[0], poly->yv[0]);
					break;
				}
				if (poly->count == 2)
				{
					io_psline(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1], 0);
					break;
				}
				io_pspolygon(poly->xv, poly->yv, poly->count, poly->desc);
			}
			break;

		case CLOSED:
		case CLOSEDRECT:
			if (isbox(poly, &xl, &xh, &yl, &yh))
			{
				io_psline(xl, yl, xl, yh, 0);
				io_psline(xl, yh, xh, yh, 0);
				io_psline(xh, yh, xh, yl, 0);
				io_psline(xh, yl, xl, yl, 0);
				break;
			}
			/* FALLTHROUGH */ 

		case OPENED:
		case OPENEDT1:
		case OPENEDT2:
		case OPENEDT3:
			switch (poly->style)
			{
				case OPENEDT1: type = 1; break;
				case OPENEDT2: type = 2; break;
				case OPENEDT3: type = 3; break;
				default:       type = 0; break;
			}
			for (k = 1; k < poly->count; k++)
				io_psline(poly->xv[k-1], poly->yv[k-1], poly->xv[k], poly->yv[k], type);
			if (poly->style == CLOSED)
			{
				k = poly->count - 1;
				io_psline(poly->xv[k], poly->yv[k], poly->xv[0], poly->yv[0], type);
			}
			break;

		case VECTORS:
			for(k=0; k<poly->count; k += 2)
				io_psline(poly->xv[k], poly->yv[k], poly->xv[k+1], poly->yv[k+1], 0);
			break;

		case CROSS:
		case BIGCROSS:
			getcenter(poly, &x, &y);
			io_psline(x-5, y, x+5, y, 0);
			io_psline(x, y+5, x, y-5, 0);
			break;

		case CROSSED:
			getbbox(poly, &xl, &xh, &yl, &yh);
			io_psline(xl, yl, xl, yh, 0);
			io_psline(xl, yh, xh, yh, 0);
			io_psline(xh, yh, xh, yl, 0);
			io_psline(xh, yl, xl, yl, 0);
			io_psline(xh, yh, xl, yl, 0);
			io_psline(xh, yl, xl, yh, 0);
			break;

		case DISC:
			io_psdisc(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1], poly->desc);
			/* FALLTHROUGH */ 

		case CIRCLE: case THICKCIRCLE:
			io_pscircle(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1]);
			break;

		case CIRCLEARC: case THICKCIRCLEARC:
			io_psarc(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1],
				poly->xv[2], poly->yv[2]);
			break;

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
			tech = poly->tech;
			size = io_pstruefontsize(TDGETSIZE(poly->textdescript), win, tech);
			getbbox(poly, &xl, &xh, &yl, &yh);
			io_pstext(poly->style, xl, xh, yl, yh, size, poly->string, poly->textdescript);
			break;
	}
}

INTBIG io_pstruefontsize(INTBIG font, WINDOWPART *w, TECHNOLOGY *tech)
{
	REGISTER INTBIG size, lambda, height;
	INTBIG psx1, psy1, psx2, psy2;
	REGISTER LIBRARY *lib;

	/* absolute font sizes are easy */
	if ((font&TXTPOINTS) != 0) return((font&TXTPOINTS) >> TXTPOINTSSH);

	/* detemine default, min, and max size of font */
	if (w->curnodeproto == NONODEPROTO) lib = el_curlib; else
		lib = w->curnodeproto->lib;
	lambda = lib->lambda[tech->techindex];
	height = TXTGETQLAMBDA(font);
	height = height * lambda / 4;
	io_psxform(0, 0, &psx1, &psy1);
	io_psxform(0, height, &psx2, &psy2);
	size = computedistance(psx1, psy1, psx2, psy2);
	return(size);
}

void io_psputheader(INTBIG which)
{
	REGISTER CHAR **strings;
	REGISTER INTBIG i;
	static CHAR *putdot[] =
	{
		x_("/Putdot {"),				/* print dot at stack pos */
		x_("    newpath moveto 0 0 rlineto stroke} def"),
	0};
	static CHAR *drawline[] =
	{
		x_("/Drawline {"),				/* draw line on stack */
		x_("    newpath moveto lineto stroke} def"),
	0};
	static CHAR *polygon[] =
	{
		x_("/Polygon {"),			/* put array into path */
		x_("    aload"),
		x_("    length 2 idiv /len exch def"),
		x_("    newpath"),
		x_("    moveto"),
		x_("    len 1 sub {lineto} repeat"),
		x_("    closepath"),
		x_("} def"),
	0};
	static CHAR *filledpolygon[] =
	{
		x_("/BuildCharDict 10 dict def"),	/* ref Making a User Defined (PostScript Cookbook) */

		x_("/StippleFont1 7 dict def"),
		x_("StippleFont1 begin"),
		x_("    /FontType 3 def"),
		x_("    /FontMatrix [1 0 0 1 0 0] def"),
		x_("    /FontBBox [0 0 1 1] def"),
		x_("    /Encoding 256 array def"),
		x_("    0 1 255 {Encoding exch /.notdef put} for"),
		x_("    /CharacterDefs 40 dict def"),
		x_("    CharacterDefs /.notdef {} put"),
		x_("    /BuildChar"),
		x_("        { BuildCharDict begin"),
		x_("            /char exch def"),
		x_("            /fontdict exch def"),
		x_("            /charname fontdict /Encoding get"),
		x_("            char get def"),
		x_("            /charproc fontdict /CharacterDefs get"),
		x_("            charname get def"),
		x_("            1 0 0 0 1 1 setcachedevice"),
		x_("            gsave charproc grestore"),
		x_("        end"),
		x_("    } def"),
		x_("end"),

		x_("/StippleFont StippleFont1 definefont pop"),

		x_("/StippleCharYSize 128 def"),
		x_("/StippleCharXSize StippleCharYSize def"),

		x_("/Filledpolygon {"),
		x_("    gsave"),
		x_("    /StippleFont findfont StippleCharYSize scalefont setfont"),
		x_("    /LowY exch def /LowX exch def"),
		x_("    /HighY exch LowY add def /HighX exch LowX add def"),
		x_("    Polygon clip"),
		x_("    /Char exch def"),
		x_("    /LowY LowY StippleCharYSize div truncate StippleCharYSize mul def"),
		x_("    /LowX LowX StippleCharXSize div truncate StippleCharXSize mul def"),
		x_("    /HighY HighY StippleCharYSize div 1 add truncate StippleCharYSize mul def"),
		x_("    /HighX HighX StippleCharXSize div 1 add truncate StippleCharXSize mul def"),
		x_("    LowY StippleCharYSize HighY "),
		x_("    { LowX exch moveto "),
		x_("        LowX StippleCharXSize HighX "),
		x_("        { Char show pop "),
		x_("        } for "),
		x_("    } for"),
		x_("    grestore"),
		x_("} def"),
	0};

	switch (which)
	{
		case PSHEADERDOT:
			if (io_psdotput) return;
			io_psdotput = TRUE;
			strings = putdot;
			break;
		case PSHEADERLINE:
			if (io_psdrawlineput) return;
			io_psdrawlineput = TRUE;
			strings = drawline;
			break;
		case PSHEADERPOLYGON:
			if (io_pspolygonput) return;
			io_pspolygonput = TRUE;
			strings = polygon;
			break;
		case PSHEADERFPOLYGON:
			if (io_psfilledpolygonput) return;
			io_psfilledpolygonput = TRUE;
			strings = filledpolygon;
			break;
		case PSHEADERSTRING:
			if (io_psstringput) return;
			io_psstringput = TRUE;
			strings = io_psstringheader;
			break;
	}
	for(i=0; strings[i] != 0; i++)
		io_pswrite(x_("%s\n"), strings[i]);
}

/* draw a dot */
void io_psdot(INTBIG x, INTBIG y)
{
	INTBIG psx, psy;

	if (io_psfake)
	{
		io_psopcount += 3;
		return;
	}

	io_psxform(x, y, &psx, &psy);
	io_psputheader(PSHEADERDOT);
	io_pswrite(x_("%s%ld %ld Putdot\n"), io_psprefix, psx, psy);
}

/* draw a line */
void io_psline(INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, INTBIG pattern)
{
	INTBIG psx1, psy1, psx2, psy2, i;

	if (io_psfake)
	{
		io_psopcount += 5;
		if (pattern != 0) io_psopcount += 10;
		return;
	}

	io_psxform(x1, y1, &psx1, &psy1);
	io_psxform(x2, y2, &psx2, &psy2);
	io_psputheader(PSHEADERLINE);
	i = PSSCALE / 2;
	switch (pattern)
	{
		case 0:
			io_pswrite(x_("%s%ld %ld %ld %ld Drawline\n"), io_psprefix, psx1, psy1, psx2, psy2);
			break;
		case 1:
			io_pswrite(x_("%s[%ld %ld] 0 setdash "), io_psprefix, i, i*3);
			io_pswrite(x_("%s%ld %ld %ld %ld Drawline "), io_psprefix, psx1, psy1, psx2, psy2);
			io_pswrite(x_("%s [] 0 setdash\n"), io_psprefix);
			break;
		case 2:
			io_pswrite(x_("%s[%ld %ld] 6 setdash "), io_psprefix, i*6, i*3);
			io_pswrite(x_("%s%ld %ld %ld %ld Drawline "), io_psprefix, psx1, psy1, psx2, psy2);
			io_pswrite(x_("%s [] 0 setdash\n"), io_psprefix);
			break;
		case 3:
			io_pswrite(x_("%ld setlinewidth "), PSSCALE);
			io_pswrite(x_("%s%ld %ld %ld %ld Drawline "), io_psprefix, psx1, psy1, psx2, psy2);
			io_pswrite(x_("%ld setlinewidth\n"), PSSCALE/2);
			break;
	}
}

/* draw an arc of a circle */
void io_psarc(INTBIG centerx,INTBIG centery, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2)
{
	INTBIG radius, pscx, pscy, psx1, psy1, psx2, psy2, startangle, endangle;

	if (io_psfake)
	{
		io_psopcount += 8;
		return;
	}

	io_psxform(centerx, centery, &pscx, &pscy);
	io_psxform(x1, y1, &psx1, &psy1);
	io_psxform(x2, y2, &psx2, &psy2);
	radius = computedistance(pscx, pscy, psx1, psy1);
	startangle = (figureangle(pscx, pscy, psx2, psy2) + 5) / 10;
	endangle = (figureangle(pscx, pscy, psx1, psy1) + 5) / 10;
	io_pswrite(x_("%snewpath %ld %ld %ld %ld %ld arc stroke\n"), io_psprefix, pscx,
		pscy, radius, startangle, endangle);
}

/* draw a circle (unfilled) */
void io_pscircle(INTBIG atx, INTBIG aty, INTBIG ex, INTBIG ey)
{
	INTBIG radius, pscx, pscy, psex, psey;

	if (io_psfake)
	{
		io_psopcount += 8;
		return;
	}

	io_psxform(atx, aty, &pscx, &pscy);
	io_psxform(ex, ey, &psex, &psey);
	radius = computedistance(pscx, pscy, psex, psey);
	io_pswrite(x_("%snewpath %ld %ld %ld 0 360 arc stroke\n"), io_psprefix, pscx,
		pscy, radius);
}

/* draw a filled circle */
void io_psdisc(INTBIG atx, INTBIG aty, INTBIG ex, INTBIG ey, GRAPHICS *desc)
{
	INTBIG radius, pscx, pscy, psex, psey;

	if (io_psfake)
	{
		io_psopcount += 8;
		return;
	}

	io_psxform(atx, aty, &pscx, &pscy);
	io_psxform(ex, ey, &psex, &psey);
	radius = computedistance(pscx, pscy, psex, psey);
	io_pswrite(x_("%snewpath %ld %ld %ld 0 360 arc fill\n"),
		io_psprefix, pscx, pscy, radius);
}

/* draw a polygon */
void io_pspolygon(INTBIG *x, INTBIG *y, INTBIG count, GRAPHICS *desc)
{
	REGISTER INTBIG lx, hx, ly, hy, i, stipplepattern;
	INTBIG psx, psy;

	if (count == 0) return;

	/* use solid color if solid pattern or no pattern */
	stipplepattern = 0;
	if (io_psusecolor)
	{
		if (io_psusecolorstip)
		{
			/* use stippled color if possible */
			if ((desc->colstyle&NATURE) == PATTERNED ||
				(desc->bwstyle&NATURE) == PATTERNED) stipplepattern = 1;
		} else
		{
			/* use solid color if default color style is solid */
			if ((desc->colstyle&NATURE) == PATTERNED) stipplepattern = 1;
		}
	} else
	{
		if ((desc->bwstyle&NATURE) == PATTERNED) stipplepattern = 1;
	}

	/* if stipple pattern is solid, just use solid fill */
	if (stipplepattern != 0)
	{
		for(i=0; i<8; i++)
			if ((desc->raster[i]&0xFFFF) != 0xFFFF) break;
		if (i >= 8) stipplepattern = 0;
	}

	/* put out solid fill if appropriate */
	if (stipplepattern == 0)
	{
		if (io_psfake) io_psopcount += count*2+4; else
		{
			io_psputheader(PSHEADERPOLYGON);
			io_pswrite(x_("%s["), io_psprefix);
			for(i=0; i<count; i++)
			{
				if (i != 0) io_pswrite(x_(" "));
				io_psxform(x[i], y[i], &psx, &psy);
				io_pswrite(x_("%ld %ld"), psx, psy);
			}
			io_pswrite(x_("] Polygon fill\n"));
		}
		return;
	}

	/*
	 * patterned fill: the hard one
	 * Generate filled polygons by defining a stipple font and then tiling the
	 * polygon to fill with 128x128 pixel characters, clipping to the polygon edge.
	 */
	if (io_psfake) io_psopcount += count*2+9; else
	{
		io_psputheader(PSHEADERPOLYGON);
		io_psputheader(PSHEADERFPOLYGON);
		io_pswrite(x_("%s(%c) ["), io_psprefix, io_pspattern(desc));
		io_psxform(x[0], y[0], &psx, &psy);
		lx = hx = psx;
		ly = hy = psy;
		for(i=0; i<count; i++)
		{
			if (i != 0) io_pswrite(x_(" "));
			io_psxform(x[i], y[i], &psx, &psy);
			io_pswrite(x_("%ld %ld"), psx, psy);
			if (psx < lx) lx = psx;   if (psx > hx) hx = psx;
			if (psy < ly) ly = psy;   if (psy > hy) hy = psy;
		}
		io_pswrite(x_("] %ld %ld %ld %ld Filledpolygon\n"), hx-lx+1, hy-ly+1, lx, ly);
	}
}

CHAR io_pspattern(GRAPHICS *col)
{
	INTBIG i, j, k, bl, bh, bld, bhd;
	GRAPHICS **newgraphicsseen;

	/* see if this graphics has been seen already */
	for(i=0; i<io_psgraphicsseencount; i++)
		if (io_psgraphicsseen[i] == col) return((CHAR)i+'A');

	/* add to list */
	if (io_psgraphicsseencount >= io_psgraphicsseenlimit)
	{
		newgraphicsseen = (GRAPHICS **)emalloc((io_psgraphicsseenlimit + 50) *
			(sizeof (GRAPHICS *)), io_tool->cluster);
		if (newgraphicsseen == 0) return(0);
		for(i=0; i<io_psgraphicsseencount; i++)
			newgraphicsseen[i] = io_psgraphicsseen[i];
		if (io_psgraphicsseenlimit != 0) efree((CHAR *)io_psgraphicsseen);
		io_psgraphicsseen = newgraphicsseen;
		io_psgraphicsseenlimit += 50;
	}
	io_psgraphicsseen[io_psgraphicsseencount++] = col;

	/*
	 * Generate filled polygons by defining a stipple font,
	 * and then tiling the polygon to fill with 128x128 pixel
	 * characters, clipping to the polygon edge.
	 *
	 * Take Electric's 16x8 bit images, double each bit,
	 * and then output 4 times to get 128 bit wide image.
	 * Double vertically by outputting each row twice.
	 * Note that full vertical size need not be generated,
	 * as PostScript will just reuse the lines until the 128
	 * size is reached.
	 *
	 * see: "Making a User Defined Font", PostScript Cookbook
	 */
	if (io_psfake) io_psopcount += 96; else
	{
		io_pswrite(x_("%sStippleFont1 begin\n"), io_psprefix);
		io_pswrite(x_("%s    Encoding (%c) 0 get /Stipple%c put\n"), io_psprefix,
			i+'A', i+'A');
		io_pswrite(x_("%s    CharacterDefs /Stipple%c {\n"), io_psprefix, i+'A');
		io_pswrite(x_("%s        128 128 true [128 0 0 -128 0 128]\n"), io_psprefix);
		io_pswrite(x_("%s        { <\n"), io_psprefix);
		for(i=0; i<8; i++)
		{
			bl = col->raster[i] & 0x00FF;
			bh = (col->raster[i] & 0xFF00) >> 8;
			bld = bhd = 0;
			for (k=0; k<8; ++k)
			{
				bld = (bld << 1);
				bld |= (bl & 0x1);
				bld = (bld << 1);
				bld |= (bl & 0x1);
				bl = (bl >> 1);
				bhd = (bhd << 1);
				bhd |= (bh & 0x1);
				bhd = (bhd << 1);
				bhd |= (bh & 0x1);
				bh = (bh >> 1);
			}
			for (k=0; k<2; k++)
			{
				io_pswrite(x_("%s            "), io_psprefix);
				for(j=0; j<4; j++)
					io_pswrite(x_("%04x %04x "), bhd&0xFFFF, bld&0xFFFF);
				io_pswrite(x_("\n"));
			}
		}
		io_pswrite(x_("%s        > } imagemask\n"), io_psprefix);
		io_pswrite(x_("%s    } put\n"), io_psprefix);
		io_pswrite(x_("%send\n"), io_psprefix);
	}
	return((CHAR)(io_psgraphicsseencount+'A'-1));
}

/* draw text */
void io_pstext(INTBIG type, INTBIG lx, INTBIG ux, INTBIG ly, INTBIG uy, INTBIG size,
	CHAR *text, UINTBIG *textdescript)
{
	CHAR *pt, psfontname[300];
	INTBIG pslx, pshx, psly, pshy, changedfont, descenderoffset,
		facenumber, x, y, rot, xoff, yoff;
	static CHAR defaultfontname[200];
	static INTBIG defaultfontnameknown = 0;
	CHAR **facelist, *facename, *opname;
	REGISTER void *infstr;

	/* get the font size */
	if (size <= 0) return;

	if (io_psfake)
	{
		io_psopcount += 5;
		if (type == TEXTBOX) io_psopcount += 2;
		return;
	}

	/* make sure the string is valid */
	for(pt = text; *pt != 0; pt++) if (*pt != ' ' && *pt != '\t') break;
	if (*pt == 0) return;

	/* get the default font name */
	if (defaultfontnameknown == 0)
	{
		defaultfontnameknown = 1;
		estrcpy(defaultfontname, screengetdefaultfacename());
	}

	io_psxform(lx, ly, &pslx, &psly);
	io_psxform(ux, uy, &pshx, &pshy);
	io_psputheader(PSHEADERSTRING);

	changedfont = 0;
	facenumber = TDGETFACE(textdescript);
	if (facenumber == 0) facename = 0; else
	{
		(void)screengetfacelist(&facelist, FALSE);
		facename = facelist[facenumber];
	}
	if (facename != 0)
	{
		infstr = initinfstr();
		for(pt = facename; *pt != 0; pt++)
		{
			if (*pt == ' ') addtoinfstr(infstr, '-'); else
				addtoinfstr(infstr, *pt);
		}
		io_pswrite(x_("/DefaultFont /%s def\n"), returninfstr(infstr));
		changedfont = 1;
	} else
	{
		if (TDGETITALIC(textdescript) != 0)
		{
			if (TDGETBOLD(textdescript) != 0)
			{
				esnprintf(psfontname, 300, x_("/DefaultFont /%s-BoldItalic def\n"),
					defaultfontname);
				io_pswrite(psfontname);
				changedfont = 1;
			} else
			{
				esnprintf(psfontname, 300, x_("/DefaultFont /%s-Italic def\n"),
					defaultfontname);
				io_pswrite(psfontname);
				changedfont = 1;
			}
		} else if (TDGETBOLD(textdescript) != 0)
		{
			esnprintf(psfontname, 300, x_("/DefaultFont /%s-Bold def\n"),
				defaultfontname);
			io_pswrite(psfontname);
			changedfont = 1;
		}
	}
	if (type == TEXTBOX)
	{
		io_pswrite(x_("%s%ld %ld %ld %ld "), io_psprefix, (pslx+pshx)/2,
			(psly+pshy)/2, pshx-pslx, pshy-psly);
		io_pswritestring(text);
		io_pswrite(x_(" %ld Boxstring\n"), size);
	} else
	{
		switch (type)
		{
			case TEXTCENT:
				x = (pslx+pshx)/2;   y = (psly+pshy)/2;
				opname = x_("Centerstring");
				break;
			case TEXTTOP:
				x = (pslx+pshx)/2;   y = pshy;
				opname = x_("Topstring");
				break;
			case TEXTBOT:
				x = (pslx+pshx)/2;   y = psly;
				opname = x_("Botstring");
				break;
			case TEXTLEFT:
				x = pslx;   y = (psly+pshy)/2;
				opname = x_("Leftstring");
				break;
			case TEXTRIGHT:
				x = pshx;   y = (psly+pshy)/2;
				opname = x_("Rightstring");
				break;
			case TEXTTOPLEFT:
				x = pslx;   y = pshy;
				opname = x_("Topleftstring");
				break;
			case TEXTTOPRIGHT:
				x = pshx;   y = pshy;
				opname = x_("Toprightstring");
				break;
			case TEXTBOTLEFT:
				x = pslx;   y = psly;
				opname = x_("Botleftstring");
				break;
			case TEXTBOTRIGHT:
				x = pshx;   y = psly;
				opname = x_("Botrightstring");
				break;
		}
		descenderoffset = size / 12;
		rot = TDGETROTATION(textdescript);
		switch (rot)
		{
			case 0: y += descenderoffset;   break;
			case 1: x -= descenderoffset;   break;
			case 2: y -= descenderoffset;   break;
			case 3: x += descenderoffset;   break;
		}
		if (rot != 0)
		{
			if (rot == 1 || rot == 3)
			{
				switch (type)
				{
					case TEXTTOP:      opname = x_("Rightstring");     break;
					case TEXTBOT:      opname = x_("Leftstring");      break;
					case TEXTLEFT:     opname = x_("Botstring");       break;
					case TEXTRIGHT:    opname = x_("Topstring");       break;
					case TEXTTOPLEFT:  opname = x_("Botrightstring");  break;
					case TEXTBOTRIGHT: opname = x_("Topleftstring");   break;
				}
			}
			xoff = x;   yoff = y;
			x = y = 0;
			switch (rot)
			{
				case 1:		/* 90 degrees counterclockwise */
					io_pswrite(x_("%s%ld %ld translate 90 rotate\n"), io_psprefix, xoff, yoff);
					break;
				case 2:		/* 180 degrees */
					io_pswrite(x_("%s%ld %ld translate 180 rotate\n"), io_psprefix, xoff, yoff);
					break;
				case 3:		/* 90 degrees clockwise */
					io_pswrite(x_("%s%ld %ld translate 270 rotate\n"), io_psprefix, xoff, yoff);
					break;
			}
		}
		io_pswrite(x_("%s%ld %ld "), io_psprefix, x, y);
		io_pswritestring(text);
		io_pswrite(x_(" %ld %s\n"), size, opname);
		if (rot != 0)
		{
			switch (rot)
			{
				case 1:		/* 90 degrees counterclockwise */
					io_pswrite(x_("%s270 rotate %ld %ld translate\n"), io_psprefix, -xoff, -yoff);
					break;
				case 2:		/* 180 degrees */
					io_pswrite(x_("%s180 rotate %ld %ld translate\n"), io_psprefix, -xoff, -yoff);
					break;
				case 3:		/* 90 degrees clockwise */
					io_pswrite(x_("%s90 rotate %ld %ld translate\n"), io_psprefix, -xoff, -yoff);
					break;
			}
		}
	}

	if (changedfont != 0)
	{
		esnprintf(psfontname, 300, x_("/DefaultFont /%s def\n"), defaultfontname);
		io_pswrite(psfontname);
	}
}

/*
 * Routine to convert the coordinates (x,y) for display.  The coordinates for
 * printing are placed back into (x,y) and the PostScript coordinates are placed
 * in (psx,psy).
 */
void io_psxform(INTBIG x, INTBIG y, INTBIG *psx, INTBIG *psy)
{
	*psx = muldiv(x, io_psmatrix[0][0], 65536) + muldiv(y, io_psmatrix[1][0], 65536) + io_psmatrix[2][0];
	*psy = muldiv(x, io_psmatrix[0][1], 65536) + muldiv(y, io_psmatrix[1][1], 65536) + io_psmatrix[2][1];
}

void io_pswritestring(CHAR *str)
{
	io_pswrite(x_("("));
	for( ; *str != 0; str++)
	{
		if (*str == '(' || *str == ')' || *str == '\\') io_pswrite(x_("\\"));
		io_pswrite(x_("%c"), *str);
	}
	io_pswrite(x_(")"));
}

void io_pswrite(CHAR *s, ...)
{
	CHAR theline[100];
	va_list ap;

	var_start(ap, s);
	evsnprintf(theline, 100, s, ap);
	va_end(ap);

	xprintf(io_psout, x_("%s"), theline);
}
