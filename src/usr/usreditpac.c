/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usreditpac.c
 * Point-and-click editor
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
#include "usreditpac.h"
#include "usrtrack.h"
#include "edialogs.h"

#define THUMBSIZE   16					/* width of the thumb area in scroll slider */
#define HEADERLINES  2					/* number of lines of text at top of editor */
#define LEFTINDENT   4					/* space between left edge and text */
#define DEFAULTWID   1					/* default number of characters on a line */

static INTBIG      us_editpactwid, us_editpacthei;		/* size of a single letter */
static INTBIG      us_editpacfont;			/* font size for editor */
static CHAR       *us_editpacbuffer = 0;	/* the cut/copy/paste buffer */

/* variables for cursor tracking */
static WINDOWPART *us_editpaccurrentwin;
static INTBIG      us_editpacdeltasofar;
static INTBIG      us_editpacinitialthumb;
static INTBIG      us_editpacorigcurline, us_editpacorigcurchar;
static INTBIG      us_editpacorigendline, us_editpacorigendchar;
static INTBIG      us_editpaclastc, us_editpaclastl;

/* variables for undo */
static INTBIG      us_editpacundooldcurline, us_editpacundooldcurchar;
static INTBIG      us_editpacundonewcurline, us_editpacundonewcurchar;
static INTBIG      us_editpacundonewendline, us_editpacundonewendchar;
static CHAR       *us_editpacundobuffer;
static INTBIG      us_editpacundobuffersize = 0;

extern GRAPHICS us_ebox, us_menutext, us_menufigs;

/* prototypes for local routines */
static void    us_editpacredraweditor(WINDOWPART*);
static void    us_editpacgotbutton(WINDOWPART*, INTBIG, INTBIG, INTBIG);
static BOOLEAN us_editpacclickdown(INTBIG, INTBIG);
static BOOLEAN us_editpacdclickdown(INTBIG, INTBIG);
static BOOLEAN us_editpacdoclickdown(INTBIG, INTBIG, BOOLEAN);
static BOOLEAN us_editpacdownarrow(INTBIG, INTBIG);
static BOOLEAN us_editpacuparrow(INTBIG, INTBIG);
static BOOLEAN us_editpacdownpage(INTBIG, INTBIG);
static BOOLEAN us_editpacuppage(INTBIG, INTBIG);
static BOOLEAN us_editpacrightarrow(INTBIG, INTBIG);
static BOOLEAN us_editpacleftarrow(INTBIG, INTBIG);
static BOOLEAN us_editpacrightpage(INTBIG, INTBIG);
static BOOLEAN us_editpacleftpage(INTBIG, INTBIG);
static BOOLEAN us_editpacimplementchar(WINDOWPART*, INTSML, INTBIG, BOOLEAN);
static void    us_editpacremoveselection(WINDOWPART*);
static void    us_editpacredrawscreen(WINDOWPART*);
static void    us_editpacdrawsliders(WINDOWPART*);
static void    us_editpacdrawhscroll(WINDOWPART*);
static void    us_editpacdrawvscroll(WINDOWPART*);
static void    us_editpaccleanupline(WINDOWPART*, INTBIG);
static void    us_editpacredrawlines(WINDOWPART*);
static void    us_editpacsetheader(WINDOWPART*, CHAR*);
static void    us_editpacinvertselection(WINDOWPART*);
static void    us_editpacdonetyping(EDITOR*);
static void    us_editpacamtyping(EDITOR*);
static void    us_editpactypeddelete(EDITOR*);
static INTBIG  us_editpaccharstocurrent(EDITOR*, INTBIG, INTBIG);
static void    us_editpacloadselection(EDITOR*, CHAR*);
static void    us_editpaccomputebounds(WINDOWPART*, EDITOR*);
static void    us_editpacbeginwork(WINDOWPART*);
static void    us_editpacaddmorelines(EDITOR*);
static void    us_editpacaddmorechars(EDITOR*, INTBIG, INTBIG);
static void    us_editpacmovebox(WINDOWPART*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static void    us_editpacclearbox(WINDOWPART*, INTBIG, INTBIG, INTBIG, INTBIG);
static void    us_editpactext(WINDOWPART*, CHAR*, INTBIG, INTBIG);
static void    us_editpacbackupposition(EDITOR *e, INTBIG *chr, INTBIG *lne);
static void    us_editpacadvanceposition(EDITOR *e, INTBIG *chr, INTBIG *lne);
static BOOLEAN us_editpacsinglesearch(WINDOWPART *win, CHAR *str, CHAR *replace, INTBIG bits);
static void    us_editpacpanwindow(WINDOWPART *w, INTBIG dx, INTBIG dy);
static void    us_editpachthumbtrackingtextcallback(INTBIG delta);
static void    us_editpacvthumbtrackingtextcallback(INTBIG delta);
static void    us_editpacensurelinevisible(WINDOWPART *win, INTBIG lindex, BOOLEAN showcursor);

/******************** ROUTINES IN THE EDITOR TABLE ********************/

/*
 * routine to convert window "oriwin" to a point-and-click text editor.
 * If "oriwin" is NOWINDOWPART, create a popup window with one line.  The window
 * header is in "header".  The number of characters and lines are placed in
 * "chars" and "lines".  Returns the window (NOWINDOWPART if the editor cannot be started).
 */
WINDOWPART *us_editpacmakeeditor(WINDOWPART *oriwin, CHAR *header, INTBIG *chars, INTBIG *lines)
{
	INTBIG x, y, i, xs, ys, swid, shei, pwid;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	REGISTER EDITOR *e;
	REGISTER VARIABLE *var;
	REGISTER WINDOWPART *win;

	/*
	 * the font that is used by the editor is the fixed-width font, TXTEDITOR.
	 * This can be changed by setting the variable "USER_textedit_font" on the user tool
	 * object.  The value is the point size minus 4 divided by 2 (the value 0 is the
	 * default font TXTEDITOR).  For example, to set the font to be 14 points, type:
	 *     -var set tool:user.USER_textedit_font 5
	 */
	us_editpacfont = TXTEDITOR;
	var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_textedit_font"));
	if (var != NOVARIABLE && var->addr >= 4)
		us_editpacfont = var->addr;

	/* if no window exists, create one as a popup */
	win = oriwin;
	if (win == NOWINDOWPART)
	{
		getpaletteparameters(&swid, &shei, &pwid);
		if (el_curwindowpart == NOWINDOWPART)
		{
			x = swid / 2;
			y = shei / 2;
		} else
		{
			(void)getxy(&x, &y);
			x = applyxscale(el_curwindowpart, x-el_curwindowpart->screenlx) + el_curwindowpart->uselx;
			y = applyyscale(el_curwindowpart, y-el_curwindowpart->screenly) + el_curwindowpart->usely;
		}

		/* create a window that covers the popup */
		startobjectchange((INTBIG)us_tool, VTOOL);
		win = newwindowpart(x_("popup"), el_curwindowpart);
		TDCLEAR(descript);
		TDSETSIZE(descript, us_editpacfont);
		screensettextinfo(win, NOTECHNOLOGY, descript);
		screengettextsize(win, x_("X"), &us_editpactwid, &us_editpacthei);
		xs = 400;   ys = us_editpacthei*(HEADERLINES+1)+DISPLAYSLIDERSIZE+4;
		win->uselx = maxi(x-xs/2, 0);
		win->usehx = mini(win->uselx+xs, swid-1);
		win->usely = maxi(y-ys, 0);
		win->usehy = mini(win->usely+ys, shei-1);
		win->state = (win->state & ~WINDOWTYPE) | POPTEXTWINDOW;
	} else
	{
		/* make sure there is room for at least one line */
		if (win->usehy - win->usely - us_editpacthei*HEADERLINES - 1 - DISPLAYSLIDERSIZE < us_editpacthei)
			return(NOWINDOWPART);

		if ((win->state&WINDOWTYPE) == DISPWINDOW)
		{
			win->usehx += DISPLAYSLIDERSIZE;
			win->usely -= DISPLAYSLIDERSIZE;
		}
		if ((win->state&WINDOWTYPE) == WAVEFORMWINDOW)
		{
			win->uselx -= DISPLAYSLIDERSIZE;
			win->usely -= DISPLAYSLIDERSIZE;
		}
		win->state = (win->state & ~(WINDOWTYPE|WINDOWMODE)) | TEXTWINDOW;
		TDCLEAR(descript);
		TDSETSIZE(descript, us_editpacfont);
		screensettextinfo(win, NOTECHNOLOGY, descript);
		screengettextsize(win, x_("X"), &us_editpactwid, &us_editpacthei);
	}

	win->curnodeproto = NONODEPROTO;
	win->buttonhandler = us_editpacgotbutton;
	win->charhandler = us_editpacgotchar;
	win->termhandler = us_editpaceditorterm;
	win->redisphandler = us_editpacredraweditor;
	win->changehandler = 0;
	win->screenlx = win->uselx;   win->screenhx = win->usehx;
	win->screenly = win->usely;   win->screenhy = win->usehy;
	computewindowscale(win);

	/* create a new editor object */
	e = us_alloceditor();
	if (e == NOEDITOR) return(NOWINDOWPART);
	e->state = (e->state & ~(EDITORTYPE|EGRAPHICSOFF|LINESFIXED|TEXTTYPING|TEXTTYPED)) | PACEDITOR;
	(void)allocstring(&e->header, header, us_tool->cluster);
	e->curline = e->curchar = 0;
	e->endline = e->endchar = 0;
	e->firstline = 0;
	e->horizfactor = 0;
	e->linecount = 1;
	e->charposition = 0;
	us_editpaccomputebounds(win, e);
	*chars = e->screenchars;
	*lines = e->screenlines;

	/* initialize the EDITOR structure */
	if ((e->state&EDITORINITED) == 0)
	{
		/* first time: allocate buffers */
		e->state |= EDITORINITED;
		e->maxlines = e->screenlines;
		if (e->maxlines <= 0) e->maxlines = 1;
		e->mostchars = e->screenchars;
		e->formerline = (CHAR *)emalloc((e->mostchars+1) * SIZEOFCHAR, us_tool->cluster);
		if (e->formerline == 0) ttyputnomemory();
		e->textarray = (CHAR **)emalloc((e->maxlines * (sizeof (CHAR *))), us_tool->cluster);
		if (e->textarray == 0) ttyputnomemory();
		e->maxchars = (INTBIG *)emalloc((e->maxlines * SIZEOFINTBIG), us_tool->cluster);
		if (e->maxchars == 0) ttyputnomemory();
		for(i=0; i<e->maxlines; i++)
		{
			e->textarray[i] = (CHAR *)emalloc(DEFAULTWID * SIZEOFCHAR, us_tool->cluster);
			if (e->textarray[i] == 0) ttyputnomemory();
			e->maxchars[i] = DEFAULTWID;
		}
	} else
	{
		/* make sure buffers cover the screen */
		while (e->screenlines > e->maxlines) us_editpacaddmorelines(e);
	}
	for(i=0; i<e->maxlines; i++)
		e->textarray[i][0] = 0;
	e->formerline[0] = 0;
	e->working = 0;
	e->dirty = FALSE;

	/* now finish initializing window */
	win->editor = e;
	if (oriwin != NOWINDOWPART)
	{
		/* clear window and write header */
		us_editpacredrawscreen(win);
	} else
	{
		/* finish initializing popup text window */
		e->savedbox = screensavebox(win, win->uselx, win->usehx, win->usely, win->usehy);
		endobjectchange((INTBIG)us_tool, VTOOL);
	}
	return(win);
}

/*
 * routine to free all memory associated with this module (at exit time)
 */
void us_freeedpacmemory(void)
{
	if (us_editpacbuffer != 0) efree(us_editpacbuffer);
}

/*
 * routine to free all memory associated with this editor
 * Called prior to destruction of the window.
 */
void us_editpacterminate(EDITOR *e)
{
	REGISTER INTBIG i;

	efree((CHAR *)e->formerline);
	for(i=0; i<e->maxlines; i++)
		efree((CHAR *)e->textarray[i]);
	efree((CHAR *)e->textarray);
	efree((CHAR *)e->maxchars);
	if (us_editpacundobuffersize > 0)
	{
		efree(us_editpacundobuffer);
		us_editpacundobuffersize = 0;
	}
}

/*
 * routine to return the total number of valid lines in the edit buffer
 */
INTBIG us_editpactotallines(WINDOWPART *win)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return(0);
	return(e->linecount);
}

/*
 * routine to get the string on line "lindex" (0 based).  A negative line
 * returns the current line.  Returns -1 if the index is beyond the file limit
 */
CHAR *us_editpacgetline(WINDOWPART *win, INTBIG lindex)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return(x_(""));
	if (lindex < 0) lindex = e->curline;
	if (lindex >= e->linecount) return(NOSTRING);
	return(e->textarray[lindex]);
}

/*
 * routine to add line "str" to the text cell to become line "lindex"
 */
void us_editpacaddline(WINDOWPART *win, INTBIG lindex, CHAR *str)
{
	REGISTER CHAR *pt;
	REGISTER INTBIG savedsline, savedschar, savedeline, savedechar;
	REGISTER EDITOR *e;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	e = win->editor;
	if (e == NOEDITOR) return;
	if ((e->state&EGRAPHICSOFF) == 0)
	{
		TDCLEAR(descript);
		TDSETSIZE(descript, us_editpacfont);
		screensettextinfo(win, NOTECHNOLOGY, descript);
	}

	/* send out any pending changes */
	us_editpacshipchanges(win);

	/* change selection to the current line */
	us_editpacinvertselection(win);
	savedschar = e->curchar;   savedsline = e->curline;
	savedechar = e->endchar;   savedeline = e->endline;
	e->curline = e->endline = lindex;   e->curchar = e->endchar = 0;
	us_editpacbeginwork(win);

	/* fake the characters */
	for(pt = str; *pt != 0; pt++) (void)us_editpacimplementchar(win, *pt, 0, FALSE);
	(void)us_editpacimplementchar(win, '\n', 0, FALSE);
	e->state &= ~TEXTTYPED;

	/* send out pending changes */
	us_editpacshipchanges(win);

	/* restore selection */
	e->curchar = savedschar;   e->curline = savedsline;
	e->endchar = savedechar;   e->endline = savedeline;
	us_editpacinvertselection(win);
	us_editpacbeginwork(win);
}

/*
 * routine to replace the line number "lindex" with the string "str".
 */
void us_editpacreplaceline(WINDOWPART *win, INTBIG lindex, CHAR *str)
{
	REGISTER CHAR *pt;
	REGISTER INTBIG savedsline, savedschar, savedeline, savedechar;
	REGISTER EDITOR *e;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	e = win->editor;
	if (e == NOEDITOR) return;
	if ((e->state&EGRAPHICSOFF) == 0)
	{
		TDCLEAR(descript);
		TDSETSIZE(descript, us_editpacfont);
		screensettextinfo(win, NOTECHNOLOGY, descript);
	}

	/* send out any pending changes */
	us_editpacshipchanges(win);

	/* change selection to the current line */
	us_editpacinvertselection(win);
	savedschar = e->curchar;   savedsline = e->curline;
	savedechar = e->endchar;   savedeline = e->endline;
	e->curline = e->endline = lindex;
	e->curchar = 0;   e->endchar = estrlen(e->textarray[lindex]);
	us_editpacbeginwork(win);

	/* fake the characters */
	for(pt = str; *pt != 0; pt++) (void)us_editpacimplementchar(win, *pt, 0, FALSE);
	e->state &= ~TEXTTYPED;

	/* send out pending changes */
	us_editpacshipchanges(win);

	/* restore selection */
	e->curchar = savedschar;   e->curline = savedsline;
	e->endchar = savedechar;   e->endline = savedeline;
	us_editpacinvertselection(win);
	us_editpacbeginwork(win);
}

/*
 * routine to delete line number "lindex"
 */
void us_editpacdeleteline(WINDOWPART *win, INTBIG lindex)
{
	REGISTER INTBIG savedsline, savedschar, savedeline, savedechar;
	REGISTER EDITOR *e;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	e = win->editor;
	if (e == NOEDITOR) return;
	if ((e->state&EGRAPHICSOFF) == 0)
	{
		TDCLEAR(descript);
		TDSETSIZE(descript, us_editpacfont);
		screensettextinfo(win, NOTECHNOLOGY, descript);
	}

	/* send out any pending changes */
	us_editpacshipchanges(win);

	/* change selection to the current line */
	us_editpacinvertselection(win);
	savedschar = e->curchar;   savedsline = e->curline;
	savedechar = e->endchar;   savedeline = e->endline;
	e->curline = lindex;   e->endline = lindex+1;
	e->curchar = e->endchar = 0;
	us_editpacbeginwork(win);

	/* fake the deletion character */
	(void)us_editpacimplementchar(win, BACKSPACEKEY, 0, FALSE);
	e->state &= ~TEXTTYPED;

	/* send out pending changes */
	us_editpacshipchanges(win);

	/* restore selection */
	e->curchar = savedschar;   e->curline = savedsline;
	e->endchar = savedechar;   e->endline = savedeline;
	us_editpacinvertselection(win);
	us_editpacbeginwork(win);
}

/*
 * routine to highlight lines "lindex" to "hindex" in the text window
 */
void us_editpachighlightline(WINDOWPART *win, INTBIG lindex, INTBIG hindex)
{
	REGISTER EDITOR *e;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	e = win->editor;
	if (e == NOEDITOR) return;
	TDCLEAR(descript);
	TDSETSIZE(descript, us_editpacfont);
	screensettextinfo(win, NOTECHNOLOGY, descript);
	us_editpacinvertselection(win);
	e->curline = lindex;
	e->curchar = 0;
	e->endline = hindex;
	e->endchar = estrlen(e->textarray[hindex]);
	us_editpacinvertselection(win);
	us_editpacbeginwork(win);

	/* ensure that line "lindex" is shown in the display */
	us_editpacensurelinevisible(win, lindex, TRUE);
}

/*
 * Routine to ensure that line "lindex" is visible in the edit window.
 * If not, the text is shifted.
 */
void us_editpacensurelinevisible(WINDOWPART *win, INTBIG lindex, BOOLEAN showcursor)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;
	if (lindex-e->firstline >= e->screenlines || lindex < e->firstline)
	{
		e->firstline = maxi(0, lindex - e->screenlines/2);
		us_editpacredrawlines(win);
		if (!showcursor) us_editpacinvertselection(win);
		if ((win->state&WINDOWTYPE) != POPTEXTWINDOW)
			us_editpacdrawvscroll(win);
	}
}

/*
 * routine to stop the graphic display of changes (for batching)
 */
void us_editpacsuspendgraphics(WINDOWPART *win)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;
	e->state |= EGRAPHICSOFF;
}

/*
 * routine to restart the graphic display of changes and redisplay (for batching)
 */
void us_editpacresumegraphics(WINDOWPART *win)
{
	REGISTER EDITOR *e;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	e = win->editor;
	if (e == NOEDITOR) return;
	e->state &= ~EGRAPHICSOFF;
	TDCLEAR(descript);
	TDSETSIZE(descript, us_editpacfont);
	screensettextinfo(win, NOTECHNOLOGY, descript);
	us_editpacredrawscreen(win);
}

/*
 * routine to write the text file to "file"
 */
void us_editpacwritetextfile(WINDOWPART *win, CHAR *file)
{
	REGISTER EDITOR *e;
	REGISTER INTBIG i;
	REGISTER FILE *f;
	CHAR *truename;

	e = win->editor;
	if (e == NOEDITOR) return;

	/* find the last line */
	if (e->linecount <= 0)
	{
		ttyputerr(_("File is empty"));
		return;
	}

	f = xcreate(file, el_filetypetext, _("Text File"), &truename);
	if (f == NULL)
	{
		if (truename != 0) ttyputerr(_("Cannot write %s"), truename);
		return;
	}

	for(i = 0; i < e->linecount; i++) xprintf(f, x_("%s\n"), e->textarray[i]);
	xclose(f);
	ttyputmsg(_("%s written"), truename);
}

/*
 * routine to read the text file "file"
 */
void us_editpacreadtextfile(WINDOWPART *win, CHAR *file)
{
	REGISTER EDITOR *e;
	REGISTER INTBIG i, filelength, charsread;
	REGISTER INTBIG c;
	REGISTER FILE *f;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	CHAR *filename;
	REGISTER void *infstr, *dia;

	e = win->editor;
	if (e == NOEDITOR) return;

	/* get the file */
	f = xopen(file, el_filetypetext, x_(""), &filename);
	if (f == NULL)
	{
		ttyputerr(_("Cannot read %s"), file);
		return;
	}
	filelength = filesize(f);
	if (filelength > 0)
	{
		infstr = initinfstr();
		formatinfstr(infstr, _("Reading %s..."), file);
		dia = DiaInitProgress(returninfstr(infstr), 0);
		if (dia == 0)
		{
			xclose(f);
			return;
		}
		DiaSetProgress(dia, 1, filelength);
	} else
		ttyputmsg(_("Reading %s"), file);

	e->state |= EGRAPHICSOFF;
	for(i=0; i<e->maxlines; i++) e->textarray[i][0] = 0;
	e->curline = e->curchar = e->firstline = 0;
	e->endline = e->endchar = 0;
	e->dirty = FALSE;
	e->linecount = 0;

	/* read the file */
	charsread = 0;
	for(;;)
	{
		c = xgetc(f);
		if (c == EOF) break;
		charsread++;
		if (c == '\t') c = ' ';
		if (c == '\n' || c == '\r')
		{
			if (e->curline >= e->maxlines-1) us_editpacaddmorelines(e);
			e->linecount++;
			e->curline++;
			e->curchar = 0;
		} else
		{
			/* see if line needs to be extended */
			i = estrlen(e->textarray[e->curline]) + 2;
			if (i > e->maxchars[e->curline])
				us_editpacaddmorechars(e, e->curline, i+10);

			e->textarray[e->curline][e->curchar] = (CHAR)c;
			e->textarray[e->curline][e->curchar+1] = 0;
			e->curchar++;
		}
		if (filelength > 0 && (charsread%100) == 0)
		{
			DiaSetProgress(dia, charsread, filelength);
		}
	}
	xclose(f);

	/* announce the changes (go backwards so the array expands only once) */
	if (filelength > 0)
	{
		DiaSetProgress(dia, 999, 1000);
		DiaSetTextProgress(dia, _("Cleaning up..."));
	}
	(*win->changehandler)(win, REPLACEALLTEXT, x_(""), (CHAR *)e->textarray, e->linecount);
	if (filelength > 0) DiaDoneProgress(dia);

	/* restore the display */
	e->state &= ~EGRAPHICSOFF;
	TDCLEAR(descript);
	TDSETSIZE(descript, us_editpacfont);
	screensettextinfo(win, NOTECHNOLOGY, descript);
	e->curline = e->curchar = 0;
	e->endline = e->endchar = 0;
	us_editpacbeginwork(win);
	us_editpacredrawlines(win);
	if ((win->state&WINDOWTYPE) != POPTEXTWINDOW) us_editpacdrawvscroll(win);
	us_editpacdrawhscroll(win);
}

void us_editpaceditorterm(WINDOWPART *win)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	/* first flush any pending edit changes */
	us_editpacshipchanges(win);

	/* restore window state */
	if ((win->state&WINDOWTYPE) == POPTEXTWINDOW)
	{
		screenrestorebox(e->savedbox, 0);
	}
}

void us_editpacshipchanges(WINDOWPART *win)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	if (e->working >= 0 && e->dirty)
	{
		if (win->changehandler != 0)
			(*win->changehandler)(win, REPLACETEXTLINE, e->formerline, e->textarray[e->working],
				e->working);
		e->dirty = FALSE;
		e->working = -1;
	}
}

/*
 * keyboard interrupt routine for the text window
 */
BOOLEAN us_editpacgotchar(WINDOWPART *win, INTSML i, INTBIG special)
{
	REGISTER EDITOR *e;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	/* pass accelerated keys to the main system */
	if ((special&ACCELERATORDOWN) != 0)
	{
		return((*DEFAULTCHARHANDLER)(win, i, special));
	}

	e = win->editor;
	if (e == NOEDITOR) return(TRUE);
	TDCLEAR(descript);
	TDSETSIZE(descript, us_editpacfont);
	screensettextinfo(win, NOTECHNOLOGY, descript);
	us_editpacinvertselection(win);
	if (us_editpacimplementchar(win, i, special, TRUE)) return(TRUE);
	us_editpacinvertselection(win);
	setactivity(_("Text Editing"));
	return(FALSE);
}

void us_editpaccut(WINDOWPART *win)
{
	REGISTER EDITOR *e;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	e = win->editor;
	if (e == NOEDITOR) return;

	/* copy the selection to the cut/copy buffer */
	us_editpaccopy(win);

	/* remove the selection from the edit buffer */
	TDCLEAR(descript);
	TDSETSIZE(descript, us_editpacfont);
	screensettextinfo(win, NOTECHNOLOGY, descript);
	us_editpacinvertselection(win);
	(void)us_editpacimplementchar(win, BACKSPACEKEY, 0, TRUE);
	us_editpacinvertselection(win);
}

void us_editpaccopy(WINDOWPART *win)
{
	REGISTER EDITOR *e;
	REGISTER INTBIG len;

	e = win->editor;
	if (e == NOEDITOR) return;

	/* free the former cut/copy buffer */
	if (us_editpacbuffer != 0)
	{
		efree(us_editpacbuffer);
		us_editpacbuffer = 0;
	}

	/* determine number of characters that are selected */
	len = us_editpaccharstocurrent(e, e->endchar, e->endline);
	if (len <= 0) return;

	/* allocate a new cut/copy buffer */
	us_editpacbuffer = (CHAR *)emalloc((len+1) * SIZEOFCHAR, us_tool->cluster);
	if (us_editpacbuffer == 0) return;

	/* fill the buffer */
	us_editpacloadselection(e, us_editpacbuffer);

	/* store it in the system-wide cut buffer */
	setcutbuffer(us_editpacbuffer);
}

void us_editpacpaste(WINDOWPART *win)
{
	REGISTER EDITOR *e;
	REGISTER INTBIG i;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	INTBIG len;
	CHAR *str;

	/* flush the internal buffer and get the system-wide cut buffer */
	if (us_editpacbuffer != 0) efree(us_editpacbuffer);
	us_editpacbuffer = 0;
	str = getcutbuffer();
	len = estrlen(str);
	if (len > 0)
	{
		us_editpacbuffer = (CHAR *)emalloc((len+1) * SIZEOFCHAR, us_tool->cluster);
		if (us_editpacbuffer == 0) return;
		estrcpy(us_editpacbuffer, str);
	}
	if (us_editpacbuffer == 0) return;

	e = win->editor;
	if (e == NOEDITOR) return;
	TDCLEAR(descript);
	TDSETSIZE(descript, us_editpacfont);
	screensettextinfo(win, NOTECHNOLOGY, descript);

	us_editpacsuspendgraphics(win);
	us_editpacinvertselection(win);
	for(i=0; us_editpacbuffer[i] != 0; i++)
	{
		if (i > 0)
		{
			if (us_editpacbuffer[i] == '\n' && us_editpacbuffer[i-1] == '\r') continue;
		}
		(void)us_editpacimplementchar(win, us_editpacbuffer[i], 0, TRUE);
	}
	us_editpacinvertselection(win);
	us_editpacresumegraphics(win);
}

void us_editpacundo(WINDOWPART *win)
{
	CHAR *savebuffer;
	REGISTER EDITOR *e;
	REGISTER INTBIG i;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	e = win->editor;
	if (e == NOEDITOR) return;

	/* if no change made, stop */
	if ((e->state&TEXTTYPED) == 0) return;

	/* finish a set of changes */
	us_editpacdonetyping(e);

	/* save the replacement buffer */
	(void)allocstring(&savebuffer, us_editpacundobuffer, el_tempcluster);

	/* replace what was typed */
	TDCLEAR(descript);
	TDSETSIZE(descript, us_editpacfont);
	screensettextinfo(win, NOTECHNOLOGY, descript);
	us_editpacinvertselection(win);
	e->curline = us_editpacundonewcurline;    e->curchar = us_editpacundonewcurchar;
	e->endline = us_editpacundonewendline;    e->endchar = us_editpacundonewendchar;
	us_editpacbeginwork(win);
	if (savebuffer[0] == 0) (void)us_editpacimplementchar(win, BACKSPACEKEY, 0, TRUE); else
		for(i=0; savebuffer[i] != 0; i++)
			(void)us_editpacimplementchar(win, savebuffer[i], 0, TRUE);
	us_editpacinvertselection(win);

	efree(savebuffer);
}

/*
 * routine to search and/or replace text.  If "replace" is nonzero, this is
 * a replace.  The meaning of "bits" is as follows:
 *   1   search from top
 *   2   replace all
 *   4   case sensitive
 *   8   search upwards
 */
void us_editpacsearch(WINDOWPART *win, CHAR *str, CHAR *replace, INTBIG bits)
{
	if ((bits&2) != 0 && replace != 0)
	{
		for(;;)
		{
			if (!us_editpacsinglesearch(win, str, replace, bits)) break;
		}
	} else
	{
		(void)us_editpacsinglesearch(win, str, replace, bits);
	}
}

void us_editpacpan(WINDOWPART *win, INTBIG dx, INTBIG dy)
{
	us_editpacpanwindow(win, dx, dy);
}

/*
 * Helper routine to do a search and replace.  Returns true if a replacement
 * was done.
 */
BOOLEAN us_editpacsinglesearch(WINDOWPART *win, CHAR *str, CHAR *replace, INTBIG bits)
{
	REGISTER INTBIG i, want, mchr, mlne;
	INTBIG startchr, startlne, chr, lne;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return(FALSE);

	if ((bits&1) != 0)
	{
		/* search from the top of the buffer */
		startchr = 0;   startlne = 0;
	} else
	{
		/* search from current location */
		if ((bits&8) != 0)
		{
			/* search from before the start of the current selection */
			startchr = e->curchar;   startlne = e->curline;
			us_editpacbackupposition(e, &startchr, &startlne);
		} else
		{
			/* search from the end of the current selection */
			startchr = e->endchar;   startlne = e->endline;
		}
	}
	chr = startchr;   lne = startlne;
	for(;;)
	{
		/* see if the string matches at this point */
		mchr = chr;   mlne = lne;
		for(i=0; str[i] != 0; i++)
		{
			want = str[i];
			if (want == '\n') want = 0;
			if ((bits&4) != 0)
			{
				if (e->textarray[mlne][mchr] != str[i]) break;
			} else
			{
				if (tolower(e->textarray[mlne][mchr]) != tolower(str[i])) break;
			}

			/* advance match pointer to the next character in the file */
			if (e->textarray[mlne][mchr] != 0) mchr++; else
			{
				mchr = 0;
				mlne++;
				if (mlne >= e->linecount) break;
			}
		}
		if (str[i] == 0) break;

		/* advance to the next character in the file */
		if ((bits&8) != 0)
		{
			us_editpacbackupposition(e, &chr, &lne);
		} else
		{
			us_editpacadvanceposition(e, &chr, &lne);
		}

		/* if it came all the way around, stop */
		if (chr == startchr && lne == startlne)
		{
			ttybeep(SOUNDBEEP, TRUE);
			return(FALSE);
		}
	}

	/* string found */
	TDCLEAR(descript);
	TDSETSIZE(descript, us_editpacfont);
	screensettextinfo(win, NOTECHNOLOGY, descript);
	us_editpacdonetyping(e);
	us_editpacinvertselection(win);
	e->curline = lne;   e->curchar = chr;
	for(i=0; str[i] != 0; i++)
		us_editpacadvanceposition(e, &chr, &lne);
	e->endline = lne;   e->endchar = chr;
	us_editpacinvertselection(win);
	us_editpacbeginwork(win);

	if (replace != 0)
	{
		us_editpacinvertselection(win);
		if (replace[0] == 0)
		{
			(void)us_editpacimplementchar(win, BACKSPACEKEY, 0, TRUE);
		} else
		{
			for(i=0; replace[i] != 0; i++)
				(void)us_editpacimplementchar(win, replace[i], 0, TRUE);
		}
		us_editpacinvertselection(win);
	}

	/* ensure that line "e->curline" is shown in the display */
	us_editpacensurelinevisible(win, e->curline, TRUE);
	return(TRUE);
}

/******************** ROUTINES IN THE WINDOW STRUCTURE ********************/

void us_editpacredraweditor(WINDOWPART *win)
{
	REGISTER EDITOR *e;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	e = win->editor;
	if (e == NOEDITOR) return;
	TDCLEAR(descript);
	TDSETSIZE(descript, us_editpacfont);
	screensettextinfo(win, NOTECHNOLOGY, descript);

	/* compute window extents */
	us_editpaccomputebounds(win, e);

	while (e->screenlines > e->maxlines) us_editpacaddmorelines(e);

	us_editpacredrawscreen(win);
}

void us_editpacgotbutton(WINDOWPART *win, INTBIG but, INTBIG x, INTBIG y)
{
	REGISTER EDITOR *e;
	REGISTER INTBIG xc, yc, len;
	BOOLEAN (*clickroutine)(INTBIG, INTBIG);
	UINTBIG descript[TEXTDESCRIPTSIZE];

	/* changes to the mouse-wheel are handled by the user interface */
	if (wheelbutton(but))
	{
		us_buttonhandler(win, but, x, y);
		return;
	}
	ttynewcommand();

	e = win->editor;
	if (e == NOEDITOR) return;
	TDCLEAR(descript);
	TDSETSIZE(descript, us_editpacfont);
	screensettextinfo(win, NOTECHNOLOGY, descript);
	us_editpaccurrentwin = win;

	if ((win->state&WINDOWTYPE) != POPTEXTWINDOW && x > e->offx + e->swid)
	{
		/* cursor in vertical slider */
		if (e->linecount <= e->screenlines) return;
		if (y < e->revy - e->shei + DISPLAYSLIDERSIZE)
		{
			/* the down arrow */
			trackcursor(FALSE, us_nullup, us_nullvoid, us_editpacdownarrow,
				us_nullchar, us_nullvoid, TRACKNORMAL);
			return;
		}
		if (y > e->revy - DISPLAYSLIDERSIZE)
		{
			/* the up arrow */
			trackcursor(FALSE, us_nullup, us_nullvoid, us_editpacuparrow,
				us_nullchar, us_nullvoid, TRACKNORMAL);
			return;
		}
		if (y < e->vthumbpos-THUMBSIZE/2 && y >= e->revy - e->shei + DISPLAYSLIDERSIZE)
		{
			/* scroll down one page */
			trackcursor(FALSE, us_nullup, us_nullvoid, us_editpacdownpage,
				us_nullchar, us_nullvoid, TRACKNORMAL);
			return;
		}
		if (y > e->vthumbpos+THUMBSIZE/2 && y <= e->revy - DISPLAYSLIDERSIZE)
		{
			/* scroll up one page */
			trackcursor(FALSE, us_nullup, us_nullvoid, us_editpacuppage,
				us_nullchar, us_nullvoid, TRACKNORMAL);
			return;
		}
		if (y >= e->vthumbpos-THUMBSIZE/2 && y <= e->vthumbpos+THUMBSIZE/2)
		{
			/* drag slider appropriately */
			us_editpacdeltasofar = 0;
			us_editpacinitialthumb = e->vthumbpos;
			us_vthumbbegin(y, win, win->usehx-DISPLAYSLIDERSIZE, win->usely+DISPLAYSLIDERSIZE,
				win->usehy-us_editpacthei*HEADERLINES, FALSE, us_editpacvthumbtrackingtextcallback);
			trackcursor(FALSE, us_nullup, us_nullvoid, us_vthumbdown, us_nullchar,
				us_vthumbdone, TRACKNORMAL);
			return;
		}
	}
	if (y <= e->revy - e->shei)
	{
		/* cursor in horizontal slider */
		if (x > e->offx + e->swid - DISPLAYSLIDERSIZE)
		{
			/* the right arrow */
			trackcursor(FALSE, us_nullup, us_nullvoid, us_editpacrightarrow,
				us_nullchar, us_nullvoid, TRACKNORMAL);
			return;
		}
		if (x < e->offx + DISPLAYSLIDERSIZE - LEFTINDENT)
		{
			/* the left arrow */
			trackcursor(FALSE, us_nullup, us_nullvoid, us_editpacleftarrow,
				us_nullchar, us_nullvoid, TRACKNORMAL);
			return;
		}
		if (x > e->hthumbpos+THUMBSIZE/2 && x <= e->offx + e->swid - DISPLAYSLIDERSIZE)
		{
			/* scroll right one page */
			trackcursor(FALSE, us_nullup, us_nullvoid, us_editpacrightpage,
				us_nullchar, us_nullvoid, TRACKNORMAL);
			return;
		}
		if (x < e->hthumbpos-THUMBSIZE/2 && x >= e->offx + DISPLAYSLIDERSIZE - LEFTINDENT)
		{
			/* scroll left one page */
			trackcursor(FALSE, us_nullup, us_nullvoid, us_editpacleftpage,
				us_nullchar, us_nullvoid, TRACKNORMAL);
			return;
		}
		if (x >= e->hthumbpos-THUMBSIZE/2 && x <= e->hthumbpos+THUMBSIZE/2)
		{
			/* drag slider appropriately */
			us_editpacdeltasofar = 0;
			us_editpacinitialthumb = (win->thumblx + win->thumbhx) / 2;
			us_hthumbbegin(x, win, win->usely+DISPLAYSLIDERSIZE, win->uselx,
				win->usehx-DISPLAYSLIDERSIZE, us_editpachthumbtrackingtextcallback);
			trackcursor(FALSE, us_nullup, us_nullvoid, us_hthumbdown, us_nullchar,
				us_hthumbdone, TRACKNORMAL);
			return;
		}
	}

	/* find the character position in the edit buffer */
	if (y > e->revy) return;
	yc = (e->revy-y) / us_editpacthei + e->firstline;
	if (yc >= e->screenlines + e->firstline) yc = e->screenlines - 1 + e->firstline;
	if (yc >= e->linecount)
	{
		yc = e->linecount-1;
		xc = estrlen(e->textarray[yc]);
	} else
	{
		xc = (x-e->offx+us_editpactwid/2) / us_editpactwid + e->horizfactor;
		if (xc < 0) xc = 0;
		len = estrlen(e->textarray[yc]);
		if (xc > len) xc = len;
	}

	/* turn off the former selection */
	us_editpacinvertselection(win);
	us_editpacdonetyping(e);

	/* determine the selection from this click */
	if (shiftbutton(but))
	{
		if (yc < e->curline || (yc == e->curline && xc < e->curchar))
		{
			e->curline = yc;   e->curchar = xc;
		} else
		{
			e->endline = yc;   e->endchar = xc;
		}
	} else
	{
		e->curline = e->endline = yc;
		e->curchar = e->endchar = xc;
	}
	if (doublebutton(but))
	{
		/* double click: select word about cursor */
		for(e->curchar = xc-1; e->curchar >= 0; e->curchar--)
			if (!isalnum(e->textarray[yc][e->curchar])) break;
		e->curchar++;
		for(e->endchar = e->curchar; e->textarray[yc][e->endchar] != 0; e->endchar++)
			if (!isalnum(e->textarray[yc][e->endchar])) break;
		clickroutine = us_editpacdclickdown;
	} else clickroutine = us_editpacclickdown;

	/* show the new selection */
	us_editpacinvertselection(win);

	/* extend the selection while the cursor remains down */
	us_editpacorigcurline = e->curline;   us_editpacorigcurchar = e->curchar;
	us_editpacorigendline = e->endline;   us_editpacorigendchar = e->endchar;
	us_editpaclastc = us_editpaclastl = -1;
	trackcursor(FALSE, us_nullup, us_nullvoid, clickroutine,
		us_nullchar, us_nullvoid, TRACKNORMAL);
	us_editpacbeginwork(win);
	e->charposition = e->endchar;
}

/******************** TRACKING ROUTINES ********************/

void us_editpachthumbtrackingtextcallback(INTBIG delta)
{
	REGISTER INTBIG point, i;
	REGISTER EDITOR *e;

	us_editpacdeltasofar += delta;
	e = us_editpaccurrentwin->editor;
	point = us_editpacinitialthumb + us_editpacdeltasofar;
	i = e->swid - DISPLAYSLIDERSIZE*2 + LEFTINDENT - THUMBSIZE;
	i = ((point - (e->offx+DISPLAYSLIDERSIZE-LEFTINDENT+THUMBSIZE/2)) *
		e->screenchars*10 + i/2) / i;
	if (i < 0) i = 0;
	if (i == e->horizfactor) return;
	e->horizfactor = i;
	us_editpacredrawlines(us_editpaccurrentwin);
	us_editpacdrawhscroll(us_editpaccurrentwin);
}

void us_editpacvthumbtrackingtextcallback(INTBIG delta)
{
	REGISTER INTBIG point, thumbarea, thumbpos;
	REGISTER EDITOR *e;

	us_editpacdeltasofar += delta;
	e = us_editpaccurrentwin->editor;
	thumbarea = e->linecount - e->screenlines;
	thumbpos = us_editpacinitialthumb + us_editpacdeltasofar;
	point = ((e->revy - DISPLAYSLIDERSIZE - THUMBSIZE/2 - thumbpos) * thumbarea - thumbarea/2) /
		(e->shei-DISPLAYSLIDERSIZE*2-THUMBSIZE);
	if (point < 0) point = 0;
	e->firstline = point;
	us_editpacredrawlines(us_editpaccurrentwin);
	us_editpacdrawvscroll(us_editpaccurrentwin);
}

BOOLEAN us_editpacclickdown(INTBIG x, INTBIG y)
{
	return(us_editpacdoclickdown(x, y, FALSE));
}

BOOLEAN us_editpacdclickdown(INTBIG x, INTBIG y)
{
	return(us_editpacdoclickdown(x, y, TRUE));
}

BOOLEAN us_editpacdoclickdown(INTBIG x, INTBIG y, BOOLEAN bywords)
{
	REGISTER INTBIG c, l, len, low, high;
	REGISTER EDITOR *e;

	e = us_editpaccurrentwin->editor;

	if (y > e->revy && (us_editpaccurrentwin->state&WINDOWTYPE) != POPTEXTWINDOW)
	{
		/* if cursor is above the window, may shift lines */
		if (e->firstline == 0) return(FALSE);
		e->firstline--;
		us_editpacredrawlines(us_editpaccurrentwin);
		us_editpacdrawvscroll(us_editpaccurrentwin);
	}
	if (y < e->revy-e->shei && (us_editpaccurrentwin->state&WINDOWTYPE) != POPTEXTWINDOW)
	{
		/* if cursor is below the window, may shift lines */
		if (e->linecount-e->firstline <= e->screenlines) return(FALSE);
		e->firstline++;
		us_editpacredrawlines(us_editpaccurrentwin);
		us_editpacdrawvscroll(us_editpaccurrentwin);
	}
	if (x < e->offx)
	{
		/* if cursor is to the left of the window, may shift screen */
		if (e->horizfactor > 0)
		{
			e->horizfactor--;
			us_editpacredrawlines(us_editpaccurrentwin);
			us_editpacdrawhscroll(us_editpaccurrentwin);
		}
	}
	if (x > e->offx+e->swid)
	{
		/* if cursor is to the right of the window, may shift screen */
		if (e->horizfactor < e->screenchars*10)
		{
			e->horizfactor++;
			us_editpacredrawlines(us_editpaccurrentwin);
			us_editpacdrawhscroll(us_editpaccurrentwin);
		}
	}
	l = (e->revy-y) / us_editpacthei + e->firstline;
	if (l >= e->screenlines + e->firstline) l = e->screenlines - 1 + e->firstline;
	if (l >= e->linecount)
	{
		l = e->linecount-1;
		c = estrlen(e->textarray[l]);
	} else
	{
		if (l < 0) l = 0;
		c = (x-e->offx+us_editpactwid/2) / us_editpactwid + e->horizfactor;
		if (c < 0) c = 0;
		len = estrlen(e->textarray[l]);
		if (c > len) c = len;
	}

	/* only interested if the character position changed */
	if (us_editpaclastc != -1 && us_editpaclastl != -1 &&
		us_editpaclastc == c && us_editpaclastl == l) return(FALSE);
	us_editpaclastc = c;   us_editpaclastl = l;

	/* if going by words, advance that much at a time */
	if (bywords && isalnum(e->textarray[l][c]) && c > 0 && isalnum(e->textarray[l][c-1]))
	{
		for(low = c-1; low >= 0; low--)
			if (!isalnum(e->textarray[l][low])) break;
		low++;
		for(high = low; e->textarray[l][high] != 0; high++)
			if (!isalnum(e->textarray[l][high])) break;
		if (l > us_editpacorigendline || (l == us_editpacorigendline && high > us_editpacorigendchar))
			c = high;
		if (l < us_editpacorigcurline || (l == us_editpacorigcurline && low < us_editpacorigcurchar))
			c = low;
	}

	us_editpacinvertselection(us_editpaccurrentwin);
	if (l > us_editpacorigendline || (l == us_editpacorigendline && c >= us_editpacorigendchar))
	{
		e->curline = us_editpacorigcurline;   e->curchar = us_editpacorigcurchar;
		e->endline = l;                       e->endchar = c;
	}
	if (l < us_editpacorigcurline || (l == us_editpacorigcurline && c <= us_editpacorigcurchar))
	{
		e->curline = l;                       e->curchar = c;
		e->endline = us_editpacorigendline;   e->endchar = us_editpacorigendchar;
	}
	us_editpacinvertselection(us_editpaccurrentwin);
	return(FALSE);
}

BOOLEAN us_editpacdownarrow(INTBIG x, INTBIG y)
{
	REGISTER EDITOR *e;

	e = us_editpaccurrentwin->editor;
	if (x <= e->offx + e->swid || x > e->offx + e->swid + DISPLAYSLIDERSIZE) return(FALSE);
	if (y >= e->revy - e->shei + DISPLAYSLIDERSIZE || y < e->revy - e->shei) return(FALSE);
	if (e->linecount-e->firstline <= e->screenlines) return(TRUE);

	us_editpacpanwindow(us_editpaccurrentwin, 0, 1);
	return(FALSE);
}

BOOLEAN us_editpacuparrow(INTBIG x, INTBIG y)
{
	REGISTER EDITOR *e;

	e = us_editpaccurrentwin->editor;
	if (x <= e->offx + e->swid || x > e->offx + e->swid + DISPLAYSLIDERSIZE) return(FALSE);
	if (y <= e->revy - DISPLAYSLIDERSIZE || y > e->revy) return(FALSE);
	if (e->firstline <= 0) return(TRUE);

	us_editpacpanwindow(us_editpaccurrentwin, 0, -1);
	return(FALSE);
}

BOOLEAN us_editpacdownpage(INTBIG x, INTBIG y)
{
	REGISTER EDITOR *e;

	e = us_editpaccurrentwin->editor;
	if (x <= e->offx + e->swid || x > e->offx + e->swid + DISPLAYSLIDERSIZE) return(FALSE);
	if (y >= e->vthumbpos-THUMBSIZE/2 || y < e->revy - e->shei + DISPLAYSLIDERSIZE) return(FALSE);
	if (e->linecount-e->firstline <= e->screenlines) return(TRUE);
	e->firstline += e->screenlines-1;
	if (e->linecount-e->firstline <= e->screenlines)
		e->firstline = e->linecount-e->screenlines;
	us_editpacredrawlines(us_editpaccurrentwin);
	us_editpacdrawvscroll(us_editpaccurrentwin);
	return(FALSE);
}

BOOLEAN us_editpacuppage(INTBIG x, INTBIG y)
{
	REGISTER EDITOR *e;

	e = us_editpaccurrentwin->editor;
	if (x <= e->offx + e->swid || x > e->offx + e->swid + DISPLAYSLIDERSIZE) return(FALSE);
	if (y <= e->vthumbpos+THUMBSIZE/2 || y > e->revy - DISPLAYSLIDERSIZE) return(FALSE);
	if (e->firstline <= 0) return(TRUE);
	e->firstline -= e->screenlines-1;
	if (e->firstline < 0) e->firstline = 0;
	us_editpacredrawlines(us_editpaccurrentwin);
	us_editpacdrawvscroll(us_editpaccurrentwin);
	return(FALSE);
}

BOOLEAN us_editpacrightarrow(INTBIG x, INTBIG y)
{
	REGISTER EDITOR *e;

	e = us_editpaccurrentwin->editor;
	if (y > e->revy - e->shei || y < e->revy - e->shei - DISPLAYSLIDERSIZE) return(FALSE);
	if (x <= e->offx + e->swid - DISPLAYSLIDERSIZE || x > e->offx + e->swid) return(FALSE);
	us_editpacpanwindow(us_editpaccurrentwin, 1, 0);
	return(FALSE);
}

BOOLEAN us_editpacleftarrow(INTBIG x, INTBIG y)
{
	REGISTER EDITOR *e;

	e = us_editpaccurrentwin->editor;
	if (y > e->revy - e->shei || y < e->revy - e->shei - DISPLAYSLIDERSIZE) return(FALSE);
	if (x >= e->offx + DISPLAYSLIDERSIZE - LEFTINDENT || x < e->offx - LEFTINDENT) return(FALSE);
	us_editpacpanwindow(us_editpaccurrentwin, -1, 0);
	return(FALSE);
}

BOOLEAN us_editpacrightpage(INTBIG x, INTBIG y)
{
	REGISTER EDITOR *e;

	e = us_editpaccurrentwin->editor;
	if (y > e->revy - e->shei || y < e->revy - e->shei - DISPLAYSLIDERSIZE) return(FALSE);
	if (x <= e->hthumbpos+THUMBSIZE/2 || x > e->offx + e->swid - DISPLAYSLIDERSIZE) return(FALSE);
	if (e->horizfactor >= e->screenchars*10) return(TRUE);
	e->horizfactor += 10;
	if (e->horizfactor >= e->screenchars*10) e->horizfactor = e->screenchars*10;
	us_editpacredrawlines(us_editpaccurrentwin);
	us_editpacdrawhscroll(us_editpaccurrentwin);
	return(FALSE);
}

BOOLEAN us_editpacleftpage(INTBIG x, INTBIG y)
{
	REGISTER EDITOR *e;

	e = us_editpaccurrentwin->editor;
	if (y > e->revy - e->shei || y < e->revy - e->shei - DISPLAYSLIDERSIZE) return(FALSE);
	if (x >= e->hthumbpos-THUMBSIZE/2 || x < e->offx + DISPLAYSLIDERSIZE - LEFTINDENT) return(FALSE);
	if (e->horizfactor <= 0) return(TRUE);
	e->horizfactor -= 10;
	if (e->horizfactor <= 0) e->horizfactor = 0;
	us_editpacredrawlines(us_editpaccurrentwin);
	us_editpacdrawhscroll(us_editpaccurrentwin);
	return(FALSE);
}

/******************** CHANGES TO EDIT BUFFER ********************/

/*
 * routine to implement character "i".  The key is issued internally if "fromuser"
 * is false.  Routine returns true if the editor window has been terminated.
 */
BOOLEAN us_editpacimplementchar(WINDOWPART *win, INTSML i, INTBIG special, BOOLEAN fromuser)
{
	REGISTER EDITOR *e;
	INTBIG j, len;
	CHAR save;
	CHAR *nextline;

	e = win->editor;
	if (e == NOEDITOR) return(TRUE);

	/* ESCAPE quits the editor */
	if (i == ESCKEY) return(TRUE);

	if ((special&SPECIALKEYDOWN) != 0)
	{
		switch ((special&SPECIALKEY) >> SPECIALKEYSH)
		{
			case SPECIALKEYARROWL:
				us_editpacdonetyping(e);
				if (e->curline == e->endline && e->curchar == e->endchar)
				{
					if (e->curchar != 0) e->curchar--; else
					{
						if (e->curline > 0)
						{
							e->curline--;
							e->curchar = estrlen(e->textarray[e->curline]);
							us_editpacbeginwork(win);
						}
					}
				}
				e->endline = e->curline;
				e->endchar = e->curchar;
				e->charposition = e->endchar;
				us_editpacensurelinevisible(win, e->curline, FALSE);
				return(FALSE);
			case SPECIALKEYARROWR:
				us_editpacdonetyping(e);
				if (e->curline == e->endline && e->curchar == e->endchar)
				{
					if (e->textarray[e->endline][e->endchar] != 0) e->endchar++; else
					{
						if (e->endline < e->linecount-1)
						{
							e->endline++;
							e->endchar = 0;
							us_editpacbeginwork(win);
						}
					}
				}
				e->curline = e->endline;
				e->curchar = e->endchar;
				e->charposition = e->endchar;
				us_editpacensurelinevisible(win, e->curline, FALSE);
				return(FALSE);
			case SPECIALKEYARROWU:
				us_editpacdonetyping(e);
				if (e->curline > 0)
				{
					e->curline--;
					len = estrlen(e->textarray[e->curline]);
					if (e->charposition > e->curchar) e->curchar = e->charposition;
					if (e->curchar > len) e->curchar = len;
					e->endline = e->curline;
					e->endchar = e->curchar;
					us_editpacbeginwork(win);
					us_editpacensurelinevisible(win, e->curline, FALSE);
				}
				return(FALSE);
			case SPECIALKEYARROWD:
				us_editpacdonetyping(e);
				if (e->endline < e->linecount-1)
				{
					e->endline++;
					len = estrlen(e->textarray[e->endline]);
					if (e->charposition > e->endchar) e->endchar = e->charposition;
					if (e->endchar > len) e->endchar = len;
					e->curline = e->endline;
					e->curchar = e->endchar;
					us_editpacbeginwork(win);
					us_editpacensurelinevisible(win, e->curline, FALSE);
				}
				return(FALSE);
		}
	}

	/* if delete key hit and nothing selected, set to erase previous character */
	if (i == BACKSPACEKEY || i == DELETEKEY)
	{
		if (e->curline == e->endline && e->curchar == e->endchar)
		{
#ifdef WIN32
			if (i == DELETEKEY)
			{
				/* advance the end character */
				len = estrlen(e->textarray[e->endline]);
				if (e->endchar < len) e->endchar++; else
				{
					if (e->endline < e->linecount)
					{
						e->endchar = 0;
						e->endline++;
					}
				}
			} else
#endif
			{
				/* backup the start character */
				if (e->curchar > 0) e->curchar--; else
				{
					if (e->curline > 0)
					{
						e->curline--;
						e->curchar = estrlen(e->textarray[e->curline]);
						us_editpacbeginwork(win);
					}
				}
			}
		}
	}

	/* see if lines are fixed */
	if ((e->state&LINESFIXED) != 0)
	{
		/* disallow line insertion, deletion or any action on the last line */
		if (e->curline != e->endline || i == '\n' || i == '\r' || e->curline == e->linecount-1)
		{
			ttybeep(SOUNDBEEP, TRUE);
			return(FALSE);
		}
	}

	us_editpacamtyping(e);
	if (i == BACKSPACEKEY || i == DELETEKEY) us_editpactypeddelete(e);

	/* remove the selected characters */
	us_editpacremoveselection(win);
	e->endline = e->curline;
	e->endchar = e->curchar;
	e->charposition = e->endchar;

	/* mark a dirty edit buffer */
	e->dirty = TRUE;

	/* ignore delete or backspace characters */
	if (i == BACKSPACEKEY || i == DELETEKEY) return(FALSE);

	/* handle end-of-line characters */
	if (i == '\n' || i == '\r')
	{
		/* exit editor if only editing one line */
		if ((win->state&WINDOWTYPE) == POPTEXTWINDOW) return(TRUE);

		/* see if there is room in the file */
		if (e->linecount >= e->maxlines) us_editpacaddmorelines(e);

		if (fromuser)
		{
			if (e->curline == e->firstline+e->screenlines-1)
			{
				e->firstline++;
				if ((e->state&EGRAPHICSOFF) == 0)
				{
					us_editpacmovebox(win, 0, 0, e->swid, us_editpacthei*(e->screenlines-1), 0, 1);
					us_editpacclearbox(win, 0, e->screenlines-1, e->swid, us_editpacthei);
					if (e->screenlines+e->firstline <= e->linecount)
						us_editpactext(win, e->textarray[e->screenlines-1+e->firstline], 0,
							e->screenlines-1);
				}
			}
		}

		/* shift lines down */
		for(j = e->linecount; j > e->curline; j--)
		{
			if (j == e->curline+1) nextline = &e->textarray[e->curline][e->curchar]; else
				nextline = e->textarray[j-1];
			len = estrlen(nextline) + 1;
			if (len > e->maxchars[j])
				us_editpacaddmorechars(e, j, len);
			(void)estrcpy(e->textarray[j], nextline);
		}
		e->textarray[e->curline][e->curchar] = 0;
		if ((e->state&EGRAPHICSOFF) == 0 && e->curline-e->firstline < e->screenlines &&
			e->curline >= e->firstline) us_editpaccleanupline(win, e->curline);
		e->curline = e->endline = e->curline+1;
		e->linecount++;
		e->curchar = 0;   e->endchar = 0;

		/* ship changes on the current line */
		us_editpacshipchanges(win);
		if (win->changehandler != 0)
			(*win->changehandler)(win, INSERTTEXTLINE, x_(""), e->textarray[e->curline], e->curline);
		us_editpacbeginwork(win);

		/* shift lines down */
		if (e->curline+1-e->firstline < e->screenlines &&
			e->curline+1 >= e->firstline && (e->state&EGRAPHICSOFF) == 0)
				us_editpacmovebox(win, 0, e->curline+1-e->firstline,
					e->swid, e->shei-(e->curline+1-e->firstline)*us_editpacthei, 0,
						e->curline-e->firstline);

		/* draw current line */
		if (e->curline-e->firstline < e->screenlines &&
			e->curline >= e->firstline && (e->state&EGRAPHICSOFF) == 0)
		{
			save = e->textarray[e->curline][0];
			e->textarray[e->curline][0] = 0;
			us_editpaccleanupline(win, e->curline);
			e->textarray[e->curline][0] = save;
			us_editpactext(win, e->textarray[e->curline], 0, e->curline-e->firstline);
		}
		if ((e->state&EGRAPHICSOFF) == 0) us_editpacdrawvscroll(win);
		e->charposition = e->endchar;
		return(FALSE);
	}

	/* self-insert of normal characters */
	if (i == '\t') i = ' ';

	/* see if line needs to be extended */
	j = estrlen(e->textarray[e->curline]) + 2;
	if (j > e->maxchars[e->curline])
		us_editpacaddmorechars(e, e->curline, j+10);

	/* see if new character is at end of line */
	if (e->textarray[e->curline][e->curchar] != 0)
	{
		/* it is not: shift letters on the line */
		len = estrlen(e->textarray[e->curline]);
		for(j = len+1; j > e->curchar; j--)
			e->textarray[e->curline][j] = e->textarray[e->curline][j-1];
	} else e->textarray[e->curline][e->curchar+1] = 0;

	/* clear from here to the end of the line */
	if (e->curline-e->firstline < e->screenlines && e->curline >= e->firstline &&
		(e->state&EGRAPHICSOFF) == 0)
	{
		e->textarray[e->curline][e->curchar] = 0;
		us_editpaccleanupline(win, e->curline);
	}

	/* insert the character */
	e->textarray[e->curline][e->curchar] = (CHAR)i;

	/* put the rest of the line on the display */
	if (e->curline-e->firstline < e->screenlines && e->curline >= e->firstline &&
		e->curchar-e->horizfactor < e->screenchars && (e->state&EGRAPHICSOFF) == 0)
	{
		us_editpactext(win, &e->textarray[e->curline][e->curchar], e->curchar,
			e->curline-e->firstline);
	}

	/* advance the character pointer */
	e->curchar++;   e->endchar++;
	e->charposition = e->endchar;
	return(FALSE);
}

void us_editpacremoveselection(WINDOWPART *win)
{
	REGISTER EDITOR *e;
	REGISTER INTBIG j, k, diff;
	REGISTER CHAR save;

	e = win->editor;
	if (e == NOEDITOR) return;

	/* nothing to do if there is a null selection */
	diff = e->endline - e->curline;
	if (e->curline == e->endline && e->curchar == e->endchar) return;

	/* report the deletion */
	if (win->changehandler != 0)
	{
		/* send out changes made so far if this is a multiline delete */
		if (diff > 0) us_editpacshipchanges(win);
		for(j=diff-1; j>=0; j--)
			(*win->changehandler)(win, DELETETEXTLINE, e->textarray[e->curline+j], x_(""),
				e->curline+j);
	}
	if (diff > 0)
		us_editpacbeginwork(win);

	/* append text beyond end of selection to the start of the selection */
	j = estrlen(&e->textarray[e->endline][e->endchar]) + e->curchar + 1;
	if (j > e->maxchars[e->curline])
		us_editpacaddmorechars(e, e->curline, j);
	(void)estrcpy(&e->textarray[e->curline][e->curchar],
		&e->textarray[e->endline][e->endchar]);

	/* redraw this line */
	if (e->curline-e->firstline < e->screenlines && e->curline >= e->firstline &&
		e->curchar-e->horizfactor < e->screenchars-1 && (e->state&EGRAPHICSOFF) == 0)
	{
		save = e->textarray[e->curline][e->curchar];
		e->textarray[e->curline][e->curchar] = 0;
		us_editpaccleanupline(win, e->curline);
		e->textarray[e->curline][e->curchar] = save;
		us_editpactext(win, &e->textarray[e->curline][e->curchar], e->curchar,
			e->curline-e->firstline);
	}

	/* now shift up text (if a multiline delete) */
	if (diff > 0)
	{
		for(j=e->curline+1; j<e->linecount-diff; j++)
		{
			/* make sure there is room for the shifted line */
			k = estrlen(e->textarray[j+diff]) + 1;
			if (k > e->maxchars[j]) us_editpacaddmorechars(e, j, k);

			/* copy new line */
			if (j-e->firstline < e->screenlines && j >= e->firstline &&
				(e->state&EGRAPHICSOFF) == 0)
			{
				/* erase this line */
				e->textarray[j][0] = 0;
				us_editpaccleanupline(win, j);

				(void)estrcpy(e->textarray[j], e->textarray[j+diff]);
				us_editpactext(win, e->textarray[j], 0, j-e->firstline);
			} else (void)estrcpy(e->textarray[j], e->textarray[j+diff]);
		}
		for(j=e->linecount-diff; j<e->linecount; j++)
		{
			e->textarray[j][0] = 0;
			if (j-e->firstline < e->screenlines && j >= e->firstline && (e->state&EGRAPHICSOFF) == 0)
				us_editpaccleanupline(win, j);
		}
		e->linecount -= diff;
	}
}

/******************** DISPLAY OF EDIT WINDOW ********************/

/*
 * routine to redisplay the text screen
 */
void us_editpacredrawscreen(WINDOWPART *win)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;
	if ((e->state&EGRAPHICSOFF) != 0) return;

	/* put header on display */
	us_editpacsetheader(win, e->header);
	us_editpacdrawsliders(win);

	us_editpacredrawlines(win);
}

void us_editpacdrawsliders(WINDOWPART *win)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	/* draw horizontal slider */
	us_drawhorizontalslider(win, win->usely+DISPLAYSLIDERSIZE, win->uselx,
		win->usehx-DISPLAYSLIDERSIZE, 0);
	us_editpacdrawhscroll(win);

	/* draw vertical slider if appropriate */
	if ((win->state&WINDOWTYPE) != POPTEXTWINDOW)
	{
		us_drawverticalslider(win, win->usehx-DISPLAYSLIDERSIZE,
			win->usely+DISPLAYSLIDERSIZE, win->usehy-us_editpacthei*HEADERLINES, FALSE);

		/* the corner */
		us_drawslidercorner(win, win->usehx-DISPLAYSLIDERSIZE+1, win->usehx, win->usely,
			win->usely+DISPLAYSLIDERSIZE-1, FALSE);
		us_editpacdrawvscroll(win);
	}
}

/*
 * Routine to set the horizontal scroll bar
 */
void us_editpacdrawhscroll(WINDOWPART *win)
{
	INTBIG left, right;
	INTBIG f;
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	/* get the area of the slider without arrows */
	left = e->offx+DISPLAYSLIDERSIZE-LEFTINDENT;
	right = e->offx + e->swid - DISPLAYSLIDERSIZE;

	/* compute position of vertical thumb area */
	f = e->horizfactor;   f *= right-left-THUMBSIZE;   f /= e->screenchars*10;
	e->hthumbpos = left + THUMBSIZE/2 + f;
	if (e->hthumbpos > right-THUMBSIZE/2) e->hthumbpos = right-THUMBSIZE/2;

	/* draw the thumb */
	win->thumblx = e->hthumbpos - THUMBSIZE/2;   win->thumbhx = e->hthumbpos + THUMBSIZE/2;
	us_drawhorizontalsliderthumb(win, win->usely+DISPLAYSLIDERSIZE, win->uselx,
		win->usehx-DISPLAYSLIDERSIZE, win->thumblx, win->thumbhx, 0);
}

/*
 * Routine to set the vertical scroll bar
 */
void us_editpacdrawvscroll(WINDOWPART *win)
{
	INTBIG top, bottom;
	INTBIG den;
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	/* get the area of the slider without arrows */
	top = e->revy - DISPLAYSLIDERSIZE;
	bottom = e->revy - e->shei + DISPLAYSLIDERSIZE;

	/* if there is nothing to scroll, clear this area */
	if (e->linecount <= e->screenlines)
	{
		us_drawverticalsliderthumb(win, win->usehx-DISPLAYSLIDERSIZE, win->usely+DISPLAYSLIDERSIZE,
			win->usehy-us_editpacthei*HEADERLINES, 1, 0);
		return;
	}

	/* compute position of vertical thumb area */
	den = e->linecount - e->screenlines;
	e->vthumbpos = e->revy - DISPLAYSLIDERSIZE - THUMBSIZE/2 -
		(e->firstline * (e->shei-DISPLAYSLIDERSIZE*2-THUMBSIZE) + den/2) / den;
	if (e->vthumbpos >= top-THUMBSIZE/2) e->vthumbpos = top-THUMBSIZE/2-1;
	if (e->vthumbpos <= bottom+THUMBSIZE/2) e->vthumbpos = bottom+THUMBSIZE/2+1;

	win->thumbly = e->vthumbpos - THUMBSIZE/2;
	win->thumbhy = e->vthumbpos + THUMBSIZE/2;
	us_drawverticalsliderthumb(win, win->usehx-DISPLAYSLIDERSIZE, win->usely+DISPLAYSLIDERSIZE,
		win->usehy-us_editpacthei*HEADERLINES, win->thumbly, win->thumbhy);
}

/*
 * routine to clear the bits from the last character to the end of the line
 */
void us_editpaccleanupline(WINDOWPART *win, INTBIG line)
{
	REGISTER INTBIG residue, linelen;
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;
	linelen = estrlen(e->textarray[line]);
	residue = e->swid-(linelen - e->horizfactor)*us_editpactwid;
	if (residue <= 0) return;
	us_editpacclearbox(win, linelen - e->horizfactor, line-e->firstline,
		residue, us_editpacthei);
}

/*
 * routine to redisplay the text screen
 */
void us_editpacredrawlines(WINDOWPART *win)
{
	REGISTER INTBIG j;
	REGISTER INTBIG bottom;
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;
	if ((e->state&EGRAPHICSOFF) != 0) return;

	/* clear the screen */
	bottom = e->revy - e->shei;
	screendrawbox(win, e->offx-LEFTINDENT, e->offx+e->swid+1, bottom-1, e->revy,
		&us_ebox);

	/* rewrite each line */
	for(j=0; j<e->screenlines; j++) if (j+e->firstline < e->linecount)
		us_editpactext(win, e->textarray[j+e->firstline], 0, j);
	us_editpacinvertselection(win);
}

/*
 * routine to write the header string
 */
void us_editpacsetheader(WINDOWPART *win, CHAR *header)
{
	REGISTER INTBIG save, formerhf, bottom;
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;
	screendrawbox(win, win->uselx, win->usehx, e->revy+1,
		e->revy+us_editpacthei*HEADERLINES, &us_ebox);
	if ((INTBIG)estrlen(header) > e->screenchars)
	{
		save = header[e->screenchars];
		header[e->screenchars] = 0;
	} else save = 0;
	formerhf = e->horizfactor;
	e->horizfactor = 0;
	us_editpactext(win, header, 0, -2);
	if ((win->state&WINDOWTYPE) == POPTEXTWINDOW)
		us_editpactext(win, _("Point-and-Click editor: type RETURN when done"), 0, -1); else
	{
		if (estrcmp(win->location, x_("entire")) != 0)
			us_editpactext(win, _("Point-and-Click editor: type ESC when done"), 0, -1); else
		{
			if (graphicshas(CANUSEFRAMES))
				us_editpactext(win, _("Point-and-Click editor: close the window when done"), 0, -1); else
					us_editpactext(win, _("Point-and-Click editor: text-only cell"), 0, -1);
		}
	}
	e->horizfactor = formerhf;

	if (save != 0) header[e->screenchars] = (CHAR)save;
	bottom = e->revy - e->shei - 1;
	screeninvertbox(win, win->uselx, win->usehx, e->revy+1,
		e->revy+us_editpacthei*HEADERLINES);
	us_menufigs.col = el_colmengly;
	screendrawline(win, win->uselx, e->revy, win->uselx, bottom, &us_menufigs, 0);
	if ((win->state&WINDOWTYPE) == POPTEXTWINDOW)
	{
		screendrawline(win, win->uselx, win->usely, win->uselx, win->usehy, &us_menufigs, 0);
		screendrawline(win, win->uselx, win->usely, win->usehx, win->usely, &us_menufigs, 0);
		screendrawline(win, win->usehx, win->usely, win->usehx, win->usehy, &us_menufigs, 0);
	}
}

/*
 * Routine to shift the edit window by the direction in the coordinates (dx, dy)
 */
void us_editpacpanwindow(WINDOWPART *w, INTBIG dx, INTBIG dy)
{
	REGISTER EDITOR *e;
	REGISTER INTBIG i;

	e = w->editor;
	us_editpacinvertselection(w);
	if (dx == 0)
	{
		/* vertical panning */
		if (dy > 0)
		{
			/* pan down by 1 line */
			if (e->firstline >= e->linecount) return;
			e->firstline++;

			/* shift the window contents up by 1 line (us_editpacthei) */
			screenmovebox(w, e->offx-LEFTINDENT, e->revy-e->shei,
				e->swid+LEFTINDENT, e->shei-us_editpacthei, e->offx-LEFTINDENT,
					e->revy-e->shei+us_editpacthei);

			/* fill in a new last line */
			screendrawbox(w, e->offx-LEFTINDENT, e->offx+e->swid,
				e->revy-e->shei, e->revy-e->shei+us_editpacthei, &us_ebox);
			i = e->firstline + e->screenlines - 1;
			if (i < e->linecount)
				us_editpactext(w, e->textarray[i], 0, i-e->firstline);
		} else
		{
			/* pan up by 1 line */
			if (e->firstline <= 0) return;
			e->firstline--;

			/* shift the window contents down by 1 line (us_editpacthei) */
			screenmovebox(w, e->offx-LEFTINDENT, e->revy-e->shei+us_editpacthei, e->swid+LEFTINDENT,
				e->shei-us_editpacthei, e->offx-LEFTINDENT, e->revy-e->shei);

			/* fill in a new first line */
			screendrawbox(w, e->offx-LEFTINDENT, e->offx+e->swid,
				e->revy-us_editpacthei, e->revy, &us_ebox);
			us_editpactext(w, e->textarray[e->firstline], 0, 0);
		}
		us_editpacinvertselection(w);
		us_editpacdrawvscroll(w);
	} else
	{
		/* horizontal panning */
		if (dx > 0)
		{
			/* pan to the right */
			if (e->horizfactor >= e->screenchars*10) return;
			e->horizfactor++;
			us_editpacredrawlines(w);
			us_editpacdrawhscroll(w);
		} else
		{
			/* pan to the left */
			if (e->horizfactor <= 0) return;
			e->horizfactor--;
			us_editpacredrawlines(w);
			us_editpacdrawhscroll(w);
		}
	}
}

/*
 * routine to invert the selected text, turning it on or off
 */
void us_editpacinvertselection(WINDOWPART *win)
{
	INTBIG i, startchar, endchar, ll, lc, hl, hc, lx, hx, ly, hy;
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	if ((e->state&EGRAPHICSOFF) != 0) return;
	ll = e->curline;   lc = e->curchar;
	hl = e->endline;   hc = e->endchar;
	if (ll > hl || (ll == hl && lc > hc))
	{
		i = hl;   hl = ll;   ll = i;
		i = hc;   hc = lc;   lc = i;
	}
	for(i = ll; i <= hl; i++)
	{
		if (i < e->firstline) continue;
		if (i-e->firstline >= e->screenlines) continue;
		if (i == ll) startchar = lc; else startchar = 0;
		if (i == hl) endchar = hc; else endchar = e->mostchars+e->horizfactor;
		if (endchar == 0 && ll != hl) continue;

		/* compute bounds of box to invert */
		lx = e->offx + (startchar-e->horizfactor) * us_editpactwid - 1;
		hx = e->offx + (endchar-e->horizfactor) * us_editpactwid - 1;
		ly = e->revy - (i-e->firstline) * us_editpacthei - us_editpacthei;
		hy = e->revy - (i-e->firstline) * us_editpacthei - 1;

		/* limit to display */
		if (lx > e->offx + e->swid || hx < e->offx-1) continue;
		if (lx < e->offx-1) lx = e->offx-1;
		if (hx > e->offx + e->swid) hx = e->offx + e->swid;
		if (ly > e->revy || hy < e->revy - e->shei) continue;
		if (ly < e->revy - e->shei) ly = e->revy - e->shei;
		if (hy > e->revy) hy = e->revy;

		screeninvertbox(win, lx, hx, ly, hy);
	}
}

/******************** UNDO CONTROL ********************/

void us_editpacdonetyping(EDITOR *e)
{
	if ((e->state&TEXTTYPING) == 0) return;

	if (e->curline < us_editpacundonewcurline ||
		(e->curline == us_editpacundonewcurline && e->curchar < us_editpacundonewcurchar))
	{
		us_editpacundonewcurline = e->curline;    us_editpacundonewcurchar = e->curchar;
	}
	if (e->endline > us_editpacundonewendline ||
		(e->endline == us_editpacundonewendline && e->endchar > us_editpacundonewendchar))
	{
		us_editpacundonewendline = e->endline;    us_editpacundonewendchar = e->endchar;
	}

	e->state &= ~TEXTTYPING;
}

void us_editpacamtyping(EDITOR *e)
{
	REGISTER INTBIG len;

	if ((e->state&TEXTTYPING) != 0)
	{
		if (e->curline < us_editpacundonewcurline ||
			(e->curline == us_editpacundonewcurline && e->curchar < us_editpacundonewcurchar))
		{
			us_editpacundonewcurline = e->curline;
			us_editpacundonewcurchar = e->curchar;
		}
		if (e->endline > us_editpacundonewendline ||
			(e->endline == us_editpacundonewendline && e->endchar > us_editpacundonewendchar))
		{
			us_editpacundonewendline = e->endline;
			us_editpacundonewendchar = e->endchar;
		}
		return;
	}
	e->state |= TEXTTYPING | TEXTTYPED;

	/* start of typing: save current position for what is typed */
	us_editpacundonewcurline = us_editpacundonewendline = e->curline;
	us_editpacundonewcurchar = us_editpacundonewendchar = e->curchar;

	/* remember former selection */
	us_editpacundooldcurline = e->curline;    us_editpacundooldcurchar = e->curchar;

	/* save what is selected */
	len = us_editpaccharstocurrent(e, e->endchar, e->endline) + 1;
	if (len > us_editpacundobuffersize)
	{
		if (us_editpacundobuffersize != 0) efree(us_editpacundobuffer);
		us_editpacundobuffersize = 0;
		us_editpacundobuffer = (CHAR *)emalloc(len * SIZEOFCHAR, us_tool->cluster);
		if (us_editpacundobuffer == 0) return;
		us_editpacundobuffersize = len;
	}
	us_editpacloadselection(e, us_editpacundobuffer);
}

void us_editpactypeddelete(EDITOR *e)
{
	REGISTER INTBIG len, i;
	REGISTER CHAR *newbuffer;

	if ((e->state&TEXTTYPING) == 0) return;

	/* if still beyond start of typing, quit */
	if (e->curline > us_editpacundooldcurline ||
		(e->curline == us_editpacundooldcurline && e->curchar >= us_editpacundooldcurchar))
			return;

	/* make sure there is room for one more character */
	len = estrlen(us_editpacundobuffer) +
		us_editpaccharstocurrent(e, us_editpacundooldcurchar, us_editpacundooldcurline) + 1;
	if (len > us_editpacundobuffersize)
	{
		newbuffer = (CHAR *)emalloc(len * SIZEOFCHAR, us_tool->cluster);
		if (newbuffer == 0) return;
		(void)estrcpy(newbuffer, us_editpacundobuffer);
		if (us_editpacundobuffersize > 0) efree(us_editpacundobuffer);
		us_editpacundobuffer = newbuffer;
		us_editpacundobuffersize = len;
	}

	/* shift the buffer up */
	for(i=len-1; i>0; i--) us_editpacundobuffer[i] = us_editpacundobuffer[i-1];
	us_editpacundobuffer[0] = e->textarray[e->curline][e->curchar];
	if (us_editpacundobuffer[0] == 0) us_editpacundobuffer[0] = '\n';

	us_editpacundooldcurline = e->curline;    us_editpacundooldcurchar = e->curchar;

	us_editpacundonewcurline = us_editpacundonewendline = e->curline;
	us_editpacundonewcurchar = us_editpacundonewendchar = e->curchar;
}

INTBIG us_editpaccharstocurrent(EDITOR *e, INTBIG endchar, INTBIG endline)
{
	REGISTER INTBIG len, line, startchar, startline, fromchar, tochar;

	if (e->curline > endline || (e->curline == endline && e->curchar > endchar))
	{
		startchar = endchar;   endchar = e->curchar;
		startline = endline;   endline = e->curline;
	} else
	{
		startchar = e->curchar;
		startline = e->curline;
	}
	len = 0;
	for(line = startline; line <= endline; line++)
	{
		if (line == startline) fromchar = startchar; else fromchar = 0;
		if (line == endline) tochar = endchar; else
			tochar = estrlen(e->textarray[line])+1;
		len += tochar - fromchar;
	}
	return(len);
}

void us_editpacloadselection(EDITOR *e, CHAR *buf)
{
	REGISTER INTBIG len, line, startch, endch, i, ch;

	len = 0;
	for(line = e->curline; line <= e->endline; line++)
	{
		if (line == e->curline) startch = e->curchar; else startch = 0;
		if (line == e->endline) endch = e->endchar; else endch = estrlen(e->textarray[line])+1;
		for(i=startch; i<endch; i++)
		{
			ch = e->textarray[line][i];
			if (ch == 0) ch = '\n';
			buf[len++] = (CHAR)ch;
		}
	}
	buf[len] = 0;
}

void us_editpacadvanceposition(EDITOR *e, INTBIG *chr, INTBIG *lne)
{
	if (e->textarray[*lne][*chr] != 0) (*chr)++; else
	{
		*chr = 0;
		(*lne)++;
		if (*lne >= e->linecount) *lne = 0;
	}
}

void us_editpacbackupposition(EDITOR *e, INTBIG *chr, INTBIG *lne)
{
	(*chr)--;
	if (*chr < 0)
	{
		(*lne)--;
		if (*lne < 0) *lne = e->linecount - 1;
		*chr = estrlen(e->textarray[*lne]) - 1;
	}
}

/******************** SUPPORT ********************/

/*
 * routine to determine the extents of the editing window
 */
void us_editpaccomputebounds(WINDOWPART *win, EDITOR *e)
{
	REGISTER INTBIG offset;

	e->offx = win->uselx + 1 + LEFTINDENT;
	e->swid = win->usehx - e->offx - 1;
	offset = 1;
	if ((win->state&WINDOWTYPE) != POPTEXTWINDOW) e->swid -= DISPLAYSLIDERSIZE;
		else offset++;
	e->shei = win->usehy - win->usely - us_editpacthei*HEADERLINES - offset - DISPLAYSLIDERSIZE;
	e->revy = e->shei + win->usely + offset + DISPLAYSLIDERSIZE;
	e->screenlines = e->shei / us_editpacthei;
	e->screenchars = e->swid / us_editpactwid;
}

void us_editpacbeginwork(WINDOWPART *win)
{
	EDITOR *e;
	REGISTER CHAR *newline;
	REGISTER INTBIG len;

	e = win->editor;
	if (e == NOEDITOR) return;
	if (e->working == e->curline) return;
	us_editpacshipchanges(win);
	e->working = e->curline;

	/* make sure there is room in the line-change buffer */
	len = estrlen(e->textarray[e->working]);
	if (len >= e->mostchars)
	{
		newline = (CHAR *)emalloc((len+1) * SIZEOFCHAR, us_tool->cluster);
		if (newline == 0) return;
		efree(e->formerline);
		e->formerline = newline;
		e->mostchars = len;
	}

	/* save the current line */
	(void)estrcpy(e->formerline, e->textarray[e->working]);
}

/*
 * routine to double the number of lines in the text buffer
 */
void us_editpacaddmorelines(EDITOR *e)
{
	REGISTER INTBIG oldlines, i;
	REGISTER INTBIG *maxchars;
	REGISTER CHAR **textarray;

	/* save former buffer size, double it */
	oldlines = e->maxlines;
	e->maxlines *= 2;
	if (e->maxlines <= 0) e->maxlines = 1;

	/* allocate new arrays */
	maxchars = (INTBIG *)emalloc((e->maxlines * SIZEOFINTBIG), us_tool->cluster);
	if (maxchars == 0) ttyputnomemory();
	textarray = (CHAR **)emalloc((e->maxlines * (sizeof (CHAR *))), us_tool->cluster);
	if (textarray == 0) ttyputnomemory();

	/* copy old information */
	for(i=0; i<e->maxlines; i++)
	{
		if (i >= oldlines)
		{
			textarray[i] = (CHAR *)emalloc(DEFAULTWID * SIZEOFCHAR, us_tool->cluster);
			if (textarray[i] == 0) ttyputnomemory();
			textarray[i][0] = 0;
			maxchars[i] = DEFAULTWID;
		} else
		{
			textarray[i] = e->textarray[i];
			maxchars[i] = e->maxchars[i];
		}
	}

	/* free old arrays */
	efree((CHAR *)e->textarray);
	efree((CHAR *)e->maxchars);

	/* setup pointers correctly */
	e->textarray = textarray;
	e->maxchars = maxchars;
}

/*
 * routine to increase the number of characters in line "line" to "need" (+1 for the null)
 */
void us_editpacaddmorechars(EDITOR *e, INTBIG line, INTBIG need)
{
	REGISTER CHAR *oldline;

	/* extend the current line */
	if (line >= e->maxlines) return;
	oldline = e->textarray[line];
	e->maxchars[line] = need+1;
	e->textarray[line] = (CHAR *)emalloc((need+1) * SIZEOFCHAR, us_tool->cluster);
	(void)estrcpy(e->textarray[line], oldline);
	efree(oldline);
}

/******************** LOW LEVEL GRAPHICS ********************/

/*
 * move text starting at character position (sx,sy) to character position
 * (dx, dy).
 */
void us_editpacmovebox(WINDOWPART *win, INTBIG dx, INTBIG dy, INTBIG wid, INTBIG hei, INTBIG sx,
	INTBIG sy)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;
	screenmovebox(win, sx*us_editpactwid+e->offx, e->revy-(sy*us_editpacthei+hei-1), wid, hei,
		dx*us_editpactwid+e->offx, e->revy-(dy*us_editpacthei+hei-1));
}

/*
 * erase text starting at character position (dx,dy) for (wid, hei)
 */
void us_editpacclearbox(WINDOWPART *win, INTBIG dx, INTBIG dy, INTBIG wid, INTBIG hei)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	dx = (dx * us_editpactwid) + e->offx;
	dy *= us_editpacthei;
	screendrawbox(win, dx, dx+wid-1, e->revy-(dy+hei-1), e->revy-dy, &us_ebox);
}

/*
 * write text "str" starting at character position (x,y)
 */
void us_editpactext(WINDOWPART *win, CHAR *str, INTBIG x, INTBIG y)
{
	INTBIG xpos, len, diff, i, clip;
	REGISTER EDITOR *e;
	REGISTER void *infstr;

	e = win->editor;
	if (e == NOEDITOR) return;

	len = estrlen(str);
	diff = e->horizfactor - x;

	/* make sure string doesn't start to left of edit window */
	if (diff > 0)
	{
		if (diff > len) return;
		str = &str[diff];
		x = e->horizfactor;
		len -= diff;
	}

	/* make sure string doesn't start to the right of the edit window */
	if (x-e->horizfactor >= e->screenchars) return;

	xpos = (x - e->horizfactor) * us_editpactwid;
	us_menutext.col = el_colmentxt;

	if (len + x - e->horizfactor > e->screenchars)
	{
		infstr = initinfstr();
		clip = e->screenchars + e->horizfactor - x;
		for(i=0; i<clip; i++) addtoinfstr(infstr, str[i]);
		str = returninfstr(infstr);
	}
	screendrawtext(win, xpos + e->offx, e->revy-(y+1)*us_editpacthei, str, &us_menutext);
}
