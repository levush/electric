/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usreditemacs.c
 * User interface tool: EMACS-like text window handler
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
 * This EMACS-like text editor accepts the following commands:
 * ^A     cursor to start-of-line
 * ^B     backup one character
 * ^D     delete the next character (cannot delete line breaks)
 * ^E     cursor to end-of-line
 * ^F     forward one character
 * ^G     flush state (and force evaluation of changed line)
 * ^H     delete the previous character (cannot delete line breaks)
 * ^K     delete to end-of-line (or kill line if on a null line)
 * ^L     redraw screen
 * RETURN insert new line (only if at end of line)
 * ^N     next line
 * ^O     insert new line (only if at beginning of line)
 * ^P     previous line
 * ^R     reverse search
 * ^S     forward search
 * ^V     shift screen up one page
 * ^X^V   read a text file from disk (you are prompted for the file name)
 * ^X^W   write the text file to disk (you are prompted for the file name)
 * ^Xd    terminate this editor window
 * ^Y     restore line deleted with ^K
 * ^Z     shift screen up one line
 * DEL    delete the previous character (cannot delete line breaks)
 * M(B)   backup one word
 * M(D)   delete the next word (cannot delete line breaks)
 * M(F)   forward one word
 * M(H)   delete the previous word (cannot delete line breaks)
 * M(V)   shift screen down one page
 * M(X)   execute an Electric command
 * M(Z)   shift screen down one line
 * M(<)   cursor to beginning of file
 * M(>)   cursor to end of file
 * M(DEL) delete the previous word (cannot delete line breaks)
 *
 * All printing characters "self-insert"
 * Meta characters can be typed by holding the META key or by prefixing
 * the character with an ESCAPE.
 * In popup editor windows, all commands that would create a new line
 * cause the editor to exit
 */

#include "global.h"
#include "egraphics.h"
#include "usr.h"
#include "usreditemacs.h"

#define CTLA   01			/* ^A */
#define CTLB   02			/* ^B */
#define CTLC   03			/* ^C */
#define CTLD   04			/* ^D */
#define CTLE   05			/* ^E */
#define CTLF   06			/* ^F */
#define CTLG   07			/* ^G */
#define CTLH  010			/* ^H */
#define CTLK  013			/* ^K */
#define CTLL  014			/* ^L */
#define CTLN  016			/* ^N */
#define CTLO  017			/* ^O */
#define CTLP  020			/* ^P */
#define CTLR  022			/* ^R */
#define CTLS  023			/* ^S */
#define CTLV  026			/* ^V */
#define CTLW  027			/* ^W */
#define CTLX  030			/* ^X */
#define CTLY  031			/* ^Y */
#define CTLZ  032			/* ^Z */

/* the bits in us_lastemacschar */
#define CONTROLX 1			/* ^X was last character typed */
#define ESCAPE   2			/* ESCAPE was last character typed */

#define HEADERLINES 2

extern GRAPHICS us_ebox, us_menutext, us_menufigs;

INTBIG          us_lastemacschar;		/* state of last character typed */
static INTBIG   us_twid, us_thei;		/* size of a single letter */
static INTBIG   us_editemacsfont;		/* font size for editor */
static CHAR    *us_killbuf;				/* saved line when ^K is done */
static INTBIG   us_killbufchars = 0;	/* characters in kill buffer */
static CHAR    *us_searchbuf = 0;		/* search string for ^S and ^R */

/* prototypes for local routines */
static void us_editemacsgotbutton(WINDOWPART*, INTBIG, INTBIG, INTBIG);
static void us_editemacsredraweditor(WINDOWPART*);
static BOOLEAN us_editemacsimplementchar(WINDOWPART*, INTBIG, BOOLEAN, BOOLEAN);
static BOOLEAN us_editemacsbackupchar(WINDOWPART*, BOOLEAN);
static BOOLEAN us_editemacsadvancechar(WINDOWPART*, BOOLEAN);
static void us_editemacsdeletechar(WINDOWPART*, BOOLEAN fromuser);
static void us_editemacsshiftscreenup(WINDOWPART*);
static void us_editemacsshiftscreendown(WINDOWPART*);
static void us_editemacsensuretextshown(WINDOWPART*, INTBIG);
static void us_editemacsredrawscreen(WINDOWPART*);
static void us_editemacsredrawline(WINDOWPART*, INTBIG);
static void us_editemacssetheader(WINDOWPART*, CHAR*);
static void us_editemacsflashcursor(WINDOWPART*);
static void us_editemacsoffhighlight(WINDOWPART*);
static void us_editemacscleanupline(WINDOWPART*, INTBIG);
static void us_editemacsworkingoncurline(EDITOR*);
static void us_editemacsaddmorelines(EDITOR*);
static void us_editemacsaddmorechars(EDITOR*, INTBIG);
static void us_editemacsgetbuffers(EDITOR*);
static void us_editemacsmovebox(WINDOWPART*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static void us_editemacsclearbox(WINDOWPART*, INTBIG, INTBIG, INTBIG, INTBIG);
static void us_editemacsinvertbox(WINDOWPART*, INTBIG, INTBIG, INTBIG, INTBIG);
static void us_editemacstext(WINDOWPART*, CHAR*, INTBIG, INTBIG);
static void us_editemacsdosearch(WINDOWPART *win, CHAR *str, BOOLEAN reverse,
			BOOLEAN fromtop, BOOLEAN casesensitive, BOOLEAN fromuser);
static CHAR us_editemacsnextvalidcharacter(EDITOR *e);

/******************** INTERFACE FOR TEXT WINDOW ********************/

/*
 * routine to convert window "oriwin" to an EMACS editor.  If "oriwin" is
 * NOWINDOWPART, create a popup window with one line.  The window header is in
 * "header".  The number of characters and lines are placed in "chars" and
 * "lines".  Returns the window (NOWINDOWPART if the editor cannot be started).
 */
WINDOWPART *us_editemacsmakeeditor(WINDOWPART *oriwin, CHAR *header, INTBIG *chars, INTBIG *lines)
{
	INTBIG i, x, y;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	INTBIG swid, shei, pwid;
	REGISTER EDITOR *e;
	REGISTER VARIABLE *var;
	REGISTER WINDOWPART *win;

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
			x = applyxscale(el_curwindowpart, x-el_curwindowpart->screenlx) +
				el_curwindowpart->uselx;
			y = applyyscale(el_curwindowpart, y-el_curwindowpart->screenly) +
				el_curwindowpart->usely;
		}

		/* create a window that covers the popup */
		startobjectchange((INTBIG)us_tool, VTOOL);
		win = newwindowpart(x_("popup"), el_curwindowpart);
		win->uselx = maxi(x-200, 0);
		win->usely = maxi(y-us_thei-1, 0);
		win->usehx = mini(win->uselx+400, swid-1);
		win->usehy = mini(win->usely+us_thei*(HEADERLINES+1)+3, shei-1);
		win->screenlx = win->uselx;
		win->screenhx = win->usehx;
		win->screenly = win->usely;
		win->screenhy = win->usehy;
		computewindowscale(win);
		win->state = (win->state & ~WINDOWTYPE) | POPTEXTWINDOW;
	} else
	{
		if ((win->state&WINDOWTYPE) == DISPWINDOW)
		{
			win->usehx += DISPLAYSLIDERSIZE;
			win->usely -= DISPLAYSLIDERSIZE;
		}
		win->state = (win->state & ~(WINDOWTYPE|WINDOWMODE)) | TEXTWINDOW;
	}

	win->curnodeproto = NONODEPROTO;
	win->buttonhandler = us_editemacsgotbutton;
	win->charhandler = us_editemacsgotchar;
	win->termhandler = us_editemacseditorterm;
	win->redisphandler = us_editemacsredraweditor;
	win->changehandler = 0;
	win->screenlx = win->uselx;   win->screenhx = win->usehx;
	win->screenly = win->usely;   win->screenhy = win->usehy;
	computewindowscale(win);

	/*
	 * the font that is used by the editor is the fixed-width font, TXTEDITOR.
	 * This can be changed by setting the variable "USER_textedit_font" on the user tool
	 * object.  The value is the point size minus 4 divided by 2 (the value 0 is the
	 * default font TXTEDITOR).  For example, to set the font to be 14 points, type:
	 *     -var set tool:user.USER_textedit_font 5
	 */
	us_editemacsfont = TXTEDITOR;
	var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_textedit_font"));
	if (var != NOVARIABLE && var->addr >= 4)
		us_editemacsfont = var->addr;
	TDCLEAR(descript);
	TDSETSIZE(descript, us_editemacsfont);
	screensettextinfo(win, NOTECHNOLOGY, descript);
	screengettextsize(win, x_("X"), &us_twid, &us_thei);

	/* create a new editor object */
	e = us_alloceditor();
	if (e == NOEDITOR) return(NOWINDOWPART);
	e->state = (e->state & ~(EDITORTYPE|EGRAPHICSOFF|LINESFIXED)) | EMACSEDITOR;
	(void)allocstring(&e->header, header, us_tool->cluster);
	e->highlightedline = -1;
	us_lastemacschar = 0;
	e->curline = e->curchar = 0;
	e->working = -1;
	e->firstline = 0;
	e->swid = win->usehx - win->uselx - 2;
	e->shei = win->usehy - win->usely - us_thei*HEADERLINES - 1;
	e->offx = win->uselx + 1;
	e->revy = e->shei + win->usely + 1;
	*chars = e->screenchars = e->swid / us_twid;
	*lines = e->screenlines = e->shei / us_thei;

	/* turn this window into an EMACS editor */
	if ((e->state&EDITORINITED) == 0)
	{
		/* first time: allocate buffers */
		e->state |= EDITORINITED;
		e->maxlines = e->screenlines;
		e->mostchars = e->screenchars;
		e->textarray = (CHAR **)emalloc((e->maxlines * (sizeof (CHAR *))),
			us_tool->cluster);
		if (e->textarray == 0) ttyputnomemory();
		e->maxchars = (INTBIG *)emalloc((e->maxlines * SIZEOFINTBIG), us_tool->cluster);
		if (e->maxchars == 0) ttyputnomemory();
		us_editemacsgetbuffers(e);
		for(i=0; i<e->maxlines; i++)
		{
			e->textarray[i] = (CHAR *)emalloc((e->screenchars+1) * SIZEOFCHAR, us_tool->cluster);
			if (e->textarray[i] == 0) ttyputnomemory();
			e->textarray[i][0] = 0;
			e->maxchars[i] = e->screenchars;
		}
	} else
	{
		/* make sure buffers cover the screen */
		while (e->screenlines > e->maxlines) us_editemacsaddmorelines(e);
		for(i=0; i<e->maxlines; i++)
		{
			e->textarray[i][0] = 0;
			while (e->screenchars > e->maxchars[i])
				us_editemacsaddmorechars(e, i);
		}
	}

	/* now finish initializing window */
	win->editor = e;
	if (oriwin != NOWINDOWPART)
	{
		/* clear window and write header */
		us_editemacssetheader(win, header);

		/* show initial cursor */
		us_editemacsflashcursor(win);
	} else
	{
		/* finish initializing window if it is a new popup one */
		e->savedbox = screensavebox(win, maxi(e->offx-2,0),
			e->offx+e->swid+2, maxi(e->revy-e->shei-2,0),
				e->revy+us_thei*HEADERLINES+1);
		endobjectchange((INTBIG)us_tool, VTOOL);
	}
	return(win);
}

void us_freeedemacsmemory(void)
{
	if (us_searchbuf != 0) efree(us_searchbuf);
}

/*
 * routine to free all memory associated with this editor
 */
void us_editemacsterminate(EDITOR *e)
{
	REGISTER INTBIG i;

	for(i=0; i<e->maxlines; i++)
		efree((CHAR *)e->textarray[i]);
	efree((CHAR *)e->textarray);
	efree((CHAR *)e->maxchars);
	efree((CHAR *)e->formerline);
	efree((CHAR *)us_killbuf);
	us_killbufchars = 0;
}

/*
 * routine to return the total number of valid lines in the edit buffer
 */
INTBIG us_editemacstotallines(WINDOWPART *win)
{
	REGISTER INTBIG i;
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return(0);
	for(i = e->maxlines-1; i >= 0; i--) if (e->textarray[i][0] != 0) break;
	return(i+1);
}

/*
 * routine to get the string on line "lindex" (0 based).  A negative line
 * returns the current line.  Returns -1 if the index is beyond the file limit
 */
CHAR *us_editemacsgetline(WINDOWPART *win, INTBIG lindex)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return(x_(""));
	if (lindex < 0) lindex = e->curline;
	if (lindex >= e->maxlines) return(NOSTRING);
	return(e->textarray[lindex]);
}

/*
 * routine to add line "str" to the text cell to become line "lindex"
 */
void us_editemacsaddline(WINDOWPART *win, INTBIG lindex, CHAR *str)
{
	REGISTER CHAR *pt;
	REGISTER INTBIG savedline, savedchar;
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;
	us_editemacsflashcursor(win);
	savedchar = e->curchar;   savedline = e->curline;
	e->curline = lindex;   e->curchar = 0;
	(void)us_editemacsimplementchar(win, CTLO, FALSE, FALSE);
	if (str[0] == 0) (void)us_editemacsimplementchar(win, ' ', FALSE, FALSE); else
		for(pt = str; *pt != 0; pt++)
			(void)us_editemacsimplementchar(win, *pt, FALSE, FALSE);
	e->curchar = savedchar;   e->curline = savedline;
	us_editemacsflashcursor(win);
}

/*
 * routine to replace the line number "lindex" with the string "str".
 */
void us_editemacsreplaceline(WINDOWPART *win, INTBIG lindex, CHAR *str)
{
	REGISTER CHAR *pt;
	REGISTER INTBIG savedline, savedchar;
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;
	us_editemacsflashcursor(win);
	savedchar = e->curchar;   savedline = e->curline;
	if (e->curline == lindex) savedchar = 0;
	e->curline = lindex;   e->curchar = 0;
	if (e->textarray[lindex][0] != 0)
		(void)us_editemacsimplementchar(win, CTLK, FALSE, FALSE);
	for(pt = str; *pt != 0; pt++)
		(void)us_editemacsimplementchar(win, *pt, FALSE, FALSE);
	e->curchar = savedchar;   e->curline = savedline;
	us_editemacsflashcursor(win);
}

/*
 * routine to delete line number "lindex"
 */
void us_editemacsdeleteline(WINDOWPART *win, INTBIG lindex)
{
	REGISTER INTBIG savedchar, savedline;
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;
	us_editemacsflashcursor(win);
	savedchar = e->curchar;   savedline = e->curline;
	if (e->curline == lindex) { savedline++;   savedchar = 0; }
	e->curline = lindex;   e->curchar = 0;
	if (e->textarray[lindex][0] != 0)
		(void)us_editemacsimplementchar(win, CTLK, FALSE, FALSE);
	(void)us_editemacsimplementchar(win, CTLK, FALSE, FALSE);
	e->curchar = savedchar;   e->curline = savedline;
	us_editemacsflashcursor(win);
}

/*
 * routine to highlight lines "lindex" to "hindex" in the text window
 */
void us_editemacshighlightline(WINDOWPART *win, INTBIG lindex, INTBIG hindex)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;
	us_editemacsoffhighlight(win);
	if ((e->state&EGRAPHICSOFF) != 0) return;
	if (hindex != lindex)
		ttyputmsg(_("EMACS can only highlight a single line"));
	e->highlightedline = lindex;
	us_editemacsensuretextshown(win, e->highlightedline);
	if (e->highlightedline-e->firstline < e->screenlines &&
		e->highlightedline >= e->firstline)
			us_editemacsinvertbox(win, 0, e->highlightedline-e->firstline,
				e->swid, us_thei);
}

/*
 * routine to stop the graphic display of changes (for batching)
 */
void us_editemacssuspendgraphics(WINDOWPART *win)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;
	e->state |= EGRAPHICSOFF;
}

/*
 * routine to restart the graphic display of changes and redisplay (for batching)
 */
void us_editemacsresumegraphics(WINDOWPART *win)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;
	e->state &= ~EGRAPHICSOFF;
	us_editemacsredrawscreen(win);
	us_editemacsflashcursor(win);
}

/*
 * routine to write the text file to "file"
 */
void us_editemacswritetextfile(WINDOWPART *win, CHAR *file)
{
	REGISTER EDITOR *e;
	REGISTER INTBIG i, j;
	REGISTER FILE *f;
	CHAR *truename;

	e = win->editor;
	if (e == NOEDITOR) return;

	/* find the last line */
	for(j = e->maxlines-1; j >= 0; j--)
		if (e->textarray[j][0] != 0) break;
	if (j < 0)
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

	for(i=0; i<=j; i++) xprintf(f, x_("%s\n"), e->textarray[i]);
	xclose(f);
	ttyputmsg(_("%s written"), truename);
}

/*
 * routine to read the text file "file"
 */
void us_editemacsreadtextfile(WINDOWPART *win, CHAR *file)
{
	REGISTER EDITOR *e;
	REGISTER INTBIG linecount, i, c;
	REGISTER FILE *f;
	CHAR *filename;

	e = win->editor;
	if (e == NOEDITOR) return;

	/* get the file */
	f = xopen(file, el_filetypetext, x_(""), &filename);
	if (f == NULL)
	{
		ttyputerr(_("Cannot read %s"), file);
		return;
	}
	ttyputmsg(_("Reading %s"), file);

	/* erase the text that is there */
	e->state |= EGRAPHICSOFF;
	for(i=0; i<e->maxlines; i++)
		e->textarray[i][0] = 0;
	e->curline = e->curchar = e->firstline = 0;

	/* read the file */
	for(;;)
	{
		c = xgetc(f);
		if (c == EOF) break;
		(void)us_editemacsimplementchar(win, c&0177, FALSE, FALSE);
	}
	xclose(f);

	/* announce the new text */
	if (win->changehandler != 0)
	{
		linecount = e->curline;
		if (e->curchar != 0) linecount++;
		(*win->changehandler)(win, REPLACEALLTEXT, x_(""), (CHAR *)e->textarray,
			linecount);
	}

	/* restore the display */
	e->state &= ~EGRAPHICSOFF;
	us_editemacsredrawscreen(win);
}

/******************** WINDOW CONTROL ********************/

void us_editemacseditorterm(WINDOWPART *win)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	us_editemacsshipchanges(win);
	if ((win->state&WINDOWTYPE) == POPTEXTWINDOW)
	{
		screenrestorebox(e->savedbox, 0);
	}
}

void us_editemacsgotbutton(WINDOWPART *win, INTBIG but, INTBIG x, INTBIG y)
{
	REGISTER EDITOR *e;
	REGISTER INTBIG xc, yc, len;

	/* changes to the mouse-wheel are handled by the user interface */
	if (wheelbutton(but))
	{
		us_buttonhandler(win, but, x, y);
		return;
	}
	ttynewcommand();

	e = win->editor;
	if (e == NOEDITOR) return;
	xc = (x-e->offx) / us_twid;
	if (xc < 0) xc = 0;
	yc = (e->revy-y) / us_thei + e->firstline;
	if (yc < e->firstline) yc = e->firstline;
	if (yc >= e->maxlines) yc = e->maxlines-1;
	len = estrlen(e->textarray[yc]);
	if (xc > len) xc = len;
	us_editemacsflashcursor(win);
	e->curchar = xc;
	e->curline = yc;
	us_editemacsflashcursor(win);
}

void us_editemacsredraweditor(WINDOWPART *win)
{
	REGISTER INTBIG i;
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	/* compute window extents */
	e->swid = win->usehx - win->uselx - 2;
	e->shei = win->usehy - win->usely - us_thei*HEADERLINES - 1;
	e->offx = win->uselx + 1;
	e->revy = e->shei + win->usely + 1;
	e->screenlines = e->shei / us_thei;
	e->screenchars = e->swid / us_twid;

	while (e->screenlines > e->maxlines) us_editemacsaddmorelines(e);
	for(i=0; i<e->maxlines; i++)
	{
		while (e->screenchars > e->maxchars[i])
			us_editemacsaddmorechars(e, i);
	}

	us_editemacsredrawscreen(win);
	us_editemacsflashcursor(win);
}

/*
 * keyboard interrupt routine for the text window
 */
BOOLEAN us_editemacsgotchar(WINDOWPART *win, INTSML i, INTBIG special)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return(TRUE);

	/* convert arrow keys to movement keys */
	if ((special&SPECIALKEYDOWN) != 0)
	{
		switch ((special&SPECIALKEY) >> SPECIALKEYSH)
		{
			case SPECIALKEYARROWL: i = 'b';  special = ACCELERATORDOWN;   break;
			case SPECIALKEYARROWR: i = 'f';  special = ACCELERATORDOWN;   break;
			case SPECIALKEYARROWU: i = 'p';  special = ACCELERATORDOWN;   break;
			case SPECIALKEYARROWD: i = 'n';  special = ACCELERATORDOWN;   break;
		}
	}
	if ((special&ACCELERATORDOWN) != 0)
	{
		us_editemacsflashcursor(win);
#ifndef MACOS
		i = i - 'a' + CTLA;
		if (us_editemacsimplementchar(win, i, FALSE, TRUE)) return(TRUE);
#else
		if (us_editemacsimplementchar(win, i, TRUE, TRUE)) return(TRUE);
#endif
		us_editemacsflashcursor(win);
	} else
	{
		us_editemacsflashcursor(win);
		if (us_editemacsimplementchar(win, i, FALSE, TRUE)) return(TRUE);
		us_editemacsflashcursor(win);
	}
	setactivity(_("EMACS Editing"));
	return(FALSE);
}

void us_editemacscut(WINDOWPART *w) { ttybeep(SOUNDBEEP, TRUE); }

void us_editemacscopy(WINDOWPART *w) { ttybeep(SOUNDBEEP, TRUE); }

void us_editemacspaste(WINDOWPART *w) { ttybeep(SOUNDBEEP, TRUE); }

void us_editemacsundo(WINDOWPART *w) { ttybeep(SOUNDBEEP, TRUE); }

/*
 * routine to search and/or replace text.  If "replace" is nonzero, this is
 * a replace.  The meaning of "bits" is as follows:
 *   1   search from top
 *   2   replace all
 *   4   case sensitive
 *   8   search upwards
 */
void us_editemacssearch(WINDOWPART *w, CHAR *str, CHAR *replace, INTBIG bits)
{
	REGISTER BOOLEAN fromtop, reverse, casesensitive;

	if (replace != 0)
	{
		ttyputerr(_("EMACS cannot replace yet"));
		return;
	}
	if ((bits&1) != 0) fromtop = TRUE; else fromtop = FALSE;
	if ((bits&4) != 0) casesensitive = TRUE; else casesensitive = FALSE;
	if ((bits&8) != 0) reverse = TRUE; else reverse = FALSE;
	us_editemacsflashcursor(w);
	us_editemacsdosearch(w, str, reverse, fromtop, casesensitive, TRUE);
	us_editemacsflashcursor(w);
}

void us_editemacspan(WINDOWPART *win, INTBIG dx, INTBIG dy)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	/* EMACS cannot handle horizontal panning */
	if (dx != 0) return;
	if (dy < 0)
	{
		if (e->firstline <= 0) return;
		us_editemacsflashcursor(win);
		e->firstline--;
	} else
	{
		us_editemacsflashcursor(win);
		e->firstline++;
	}
	us_editemacsredrawscreen(win);
	us_editemacsflashcursor(win);
}

/******************** EDITOR CONTROL ********************/

/*
 * routine to implement EMACS character "i", with meta key held down if "m" is
 * 1.  The key is issued internally if "fromuser" is zero.  Routine returns
 * true if the editor window has been terminated.
 */
BOOLEAN us_editemacsimplementchar(WINDOWPART *win, INTBIG i, BOOLEAN m, BOOLEAN fromuser)
{
	REGISTER EDITOR *e;
	INTBIG j, k, savecur, len, nextlineno, nextcharno;
	CHAR s[2], *pt, *nextline, ch;
	REGISTER void *infstr;

	e = win->editor;
	if (e == NOEDITOR) return(TRUE);

	/* make sure line highlighting is off */
	us_editemacsoffhighlight(win);

	/* if ESCAPE was typed, set meta bit */
	if ((us_lastemacschar&ESCAPE) != 0)
	{
		m = TRUE;
		us_lastemacschar &= ~ESCAPE;
	}

	/* separate interpreter for ^X prefix */
	if ((us_lastemacschar&CONTROLX) != 0)
	{
		us_lastemacschar &= ~CONTROLX;

		/* save the text in a disk file */
		if (i == CTLW && !m)
		{
			if (fromuser) us_editemacsshipchanges(win);
			us_editemacswritetextfile(win, x_("EMACSbuffer"));
			return(FALSE);
		}

		/* read a text from a disk file */
		if (i == CTLV && !m)
		{
			if (fromuser) us_editemacsshipchanges(win);
			pt = (CHAR *)fileselect(_("File name: "), el_filetypetext, x_(""));
			if (pt == 0 || *pt == 0)
			{
				us_abortedmsg();
				return(FALSE);
			}
			us_editemacsreadtextfile(win, pt);
			return(FALSE);
		}

		ttyputerr(_("The sequence '^X%s' is not valid in EMACS"),
			us_describeboundkey((INTSML)i, (!m ? 0 : ACCELERATORDOWN), 1));
		return(FALSE);
	}

	/* ESCAPE prefix for the next character */
	if (i == ESCKEY && !m)
	{
		us_lastemacschar |= ESCAPE;
		return(FALSE);
	}

	/* ^X prefix for the next character */
	if (i == CTLX && !m)
	{
		us_lastemacschar |= CONTROLX;
		return(FALSE);
	}

	/* self-insert of normal characters */
	if (i == '\t') i = ' ';
	if (i >= ' ' && i <= '~' && !m)
	{
		if (fromuser) us_editemacsworkingoncurline(e);

		/* see if line needs to be extended */
		if ((INTBIG)estrlen(e->textarray[e->curline]) >= e->maxchars[e->curline])
			us_editemacsaddmorechars(e, e->curline);

		/* see if new character is at end of line */
		if (e->textarray[e->curline][e->curchar] != 0)
		{
			/* it is not: shift letters on the line */
			for(j=e->maxchars[e->curline]; j>e->curchar; j--)
				e->textarray[e->curline][j] = e->textarray[e->curline][j-1];
			if (e->curline-e->firstline < e->screenlines &&
				e->curline >= e->firstline && e->curchar < e->screenchars-1 &&
					(e->state&EGRAPHICSOFF) == 0)
			{
				us_editemacsmovebox(win, e->curchar+1, e->curline-e->firstline,
					e->swid-e->curchar*us_twid-us_twid, us_thei, e->curchar,
						e->curline-e->firstline);
				us_editemacscleanupline(win, e->curline);
			}
		} else e->textarray[e->curline][e->curchar+1] = 0;

		/* put the character on the display and into image memory */
		if (e->curline-e->firstline < e->screenlines &&
			e->curline >= e->firstline && e->curchar < e->screenchars &&
				(e->state&EGRAPHICSOFF) == 0)
		{
			us_editemacsclearbox(win, e->curchar, e->curline-e->firstline,
				us_twid, us_thei);
			s[0] = (CHAR)i;   s[1] = 0;
			us_editemacstext(win, s, e->curchar, e->curline-e->firstline);
			us_editemacscleanupline(win, e->curline);
		}
		e->textarray[e->curline][e->curchar] = (CHAR)i;

		/* advance the character pointer */
		e->curchar++;
		return(FALSE);
	}

	/* search for a string */
	if (i == CTLS && !m)
	{
		if (us_searchbuf == 0)
		{
			pt = ttygetlinemessages(_("Search for: "));
		} else
		{
			infstr = initinfstr();
			formatinfstr(infstr, _("Search for [%s]: "), us_searchbuf);
			pt = ttygetlinemessages(returninfstr(infstr));
		}
		if (pt == 0) return(FALSE);
		if (*pt == 0)
		{
			if (us_searchbuf == 0)
			{
				ttyputmsg(_("No previous search string"));
				return(FALSE);
			}
			pt = us_searchbuf;
		} else
		{
			if (us_searchbuf != 0) efree(us_searchbuf);
			(void)allocstring(&us_searchbuf, pt, us_tool->cluster);
		}
		us_editemacsdosearch(win, us_searchbuf, FALSE, FALSE, FALSE, fromuser);
		return(FALSE);
	}

	/* reverse search for a string */
	if (i == CTLR && !m)
	{
		if (us_searchbuf == 0)
		{
			pt = ttygetlinemessages(_("Reverse search for: "));
		} else
		{
			infstr = initinfstr();
			formatinfstr(infstr, _("Reverse search for [%s]: "), us_searchbuf);
			pt = ttygetlinemessages(returninfstr(infstr));
		}
		if (pt == 0) return(FALSE);
		if (*pt == 0)
		{
			if (us_searchbuf == 0)
			{
				ttyputmsg(_("No previous search string"));
				return(FALSE);
			}
			pt = us_searchbuf;
		} else
		{
			if (us_searchbuf != 0) efree(us_searchbuf);
			(void)allocstring(&us_searchbuf, pt, us_tool->cluster);
		}
		us_editemacsdosearch(win, us_searchbuf, TRUE, FALSE, FALSE, fromuser);
		return(FALSE);
	}

	/* delete next character on a line (cannot delete line break) */
	if (i == CTLD && !m)
	{
		if (fromuser) us_editemacsworkingoncurline(e);
		us_editemacsdeletechar(win, fromuser);
		return(FALSE);
	}

	/* delete next word on a line (cannot delete line break) */
	if (i == 'd' && m)
	{
		if (fromuser) us_editemacsworkingoncurline(e);

		/* first delete all nonalphanumeric characters */
		for(;;)
		{
			ch = us_editemacsnextvalidcharacter(e);
			if (ch == 0) break;
			if (isalnum(ch)) break;
			us_editemacsdeletechar(win, fromuser);
		}

		for(;;)
		{
			us_editemacsdeletechar(win, fromuser);
			if (!isalnum(e->textarray[e->curline][e->curchar])) break;
		}
		return(FALSE);
	}

	/* delete previous character on a line (cannot delete line break) */
	if ((i == CTLH || i == DELETEKEY) && !m)
	{
		if (fromuser) us_editemacsworkingoncurline(e);
		e->curchar--;
		if (e->curchar < 0)
		{
			if (e->curline <= 0) { e->curchar++;   return(FALSE); }
			e->curline--;
			e->curchar = estrlen(e->textarray[e->curline]);
		}
		us_editemacsdeletechar(win, fromuser);
		return(FALSE);
	}

	/* delete previous word on a line (cannot delete line break) */
	if ((i == 'h' || i == DELETEKEY) && m)
	{
		if (fromuser) us_editemacsworkingoncurline(e);

		/* first backwards delete all nonalphanumeric characters */
		for(;;)
		{
			e->curchar--;
			if (e->curchar < 0)
			{
				if (e->curline <= 0) { e->curchar++;   return(FALSE); }
				e->curline--;
				e->curchar = estrlen(e->textarray[e->curline]);
			}
			if (isalnum(e->textarray[e->curline][e->curchar])) break;
			us_editemacsdeletechar(win, fromuser);
		}

		/* now backwards delete alphanumeric characters */
		for(;;)
		{
			us_editemacsdeletechar(win, fromuser);
			nextcharno = e->curchar - 1;
			nextlineno = e->curline;
			if (nextcharno < 0)
			{
				if (nextlineno <= 0) return(FALSE);
				nextlineno--;
				nextcharno = estrlen(e->textarray[nextlineno]);
			}
			if (!isalnum(e->textarray[nextlineno][nextcharno])) break;
			e->curchar = nextcharno;
			e->curline = nextlineno;
		}
		return(FALSE);
	}

	/* kill to end of line (delete line if at beginning of empty line) */
	if (i == CTLK && !m)
	{
		if (e->curchar == 0 && e->textarray[e->curline][e->curchar] == 0)
		{
			/* exit editor if only editing one line */
			if ((win->state&WINDOWTYPE) == POPTEXTWINDOW)
			{
				if (fromuser) us_editemacsshipchanges(win);
				return(TRUE);
			}

			/* cannot change number of lines if not allowed */
			if (fromuser && (e->state&LINESFIXED) != 0)
			{
				ttyputerr(_("Cannot delete lines in this edit session"));
				return(FALSE);
			}

			/* delete line in memory */
			if (fromuser) us_editemacsshipchanges(win);
			savecur = e->curline;
			for(; e->curline < e->maxlines; e->curline++)
			{
				if (e->curline == e->maxlines-1) nextline = x_(""); else
					nextline = e->textarray[e->curline+1];
				if (estrcmp(e->textarray[e->curline], nextline) == 0) continue;
				if (fromuser) us_editemacsworkingoncurline(e);
				len = estrlen(nextline);
				while (len > e->maxchars[e->curline])
					us_editemacsaddmorechars(e, e->curline);
				(void)estrcpy(e->textarray[e->curline], nextline);
				if (fromuser) us_editemacsshipchanges(win);
			}
			e->curline = savecur;

			/* delete line on the screen */
			if (e->curline-e->firstline < e->screenlines-1 &&
				e->curline >= e->firstline && (e->state&EGRAPHICSOFF) == 0)
			{
				us_editemacsmovebox(win, 0, e->curline-e->firstline, e->swid,
					e->shei-(e->curline-e->firstline+1)*us_thei, 0,
						e->curline-e->firstline+1);
				if (e->screenlines < e->maxlines)
				{
					us_editemacsclearbox(win, 0, e->screenlines-1, e->swid, us_thei);
					us_editemacsredrawline(win, e->screenlines-1);
				}
			}
		} else
		{
			if (e->textarray[e->curline][e->curchar] == 0)
			{
				ttyputerr(_("Can only delete entire lines"));
				return(FALSE);
			}

			/* kill to end of line */
			if (fromuser) us_editemacsworkingoncurline(e);
			(void)estrcpy(us_killbuf, &e->textarray[e->curline][e->curchar]);
			if (e->curline-e->firstline < e->screenlines &&
				e->curline >= e->firstline && (e->state&EGRAPHICSOFF) == 0)
					us_editemacsclearbox(win, e->curchar, e->curline-e->firstline,
						e->swid-e->curchar*us_twid, us_thei);
			e->textarray[e->curline][e->curchar] = 0;
		}
		return(FALSE);
	}

	if (i == CTLY && !m)
	{
		for(pt = us_killbuf; *pt != 0; pt++)
		{
			if (*pt == CTLY || *pt == CTLK) continue;
			(void)us_editemacsimplementchar(win, *pt, FALSE, fromuser);
		}
		return(FALSE);
	}

	if (i == CTLL && !m)
	{
		us_editemacsredrawscreen(win);
		return(FALSE);
	}

	if (i == CTLB && !m)
	{
		(void)us_editemacsbackupchar(win, fromuser);
		return(FALSE);
	}

	if (i == 'b' && m)
	{
		for(;;)
		{
			if (us_editemacsbackupchar(win, fromuser)) break;
			if (isalnum(e->textarray[e->curline][e->curchar])) break;
		}
		for(;;)
		{
			if (e->curchar <= 0) break;
			if (!isalnum(e->textarray[e->curline][e->curchar-1])) break;
			(void)us_editemacsbackupchar(win, fromuser);
		}
		return(FALSE);
	}

	if (i == CTLF && !m)
	{
		/* exit editor if at end of line and only editing one line */
		if (e->textarray[e->curline][e->curchar] == 0 &&
			(win->state&WINDOWTYPE) == POPTEXTWINDOW)
		{
			if (fromuser) us_editemacsshipchanges(win);
			return(TRUE);
		}

		(void)us_editemacsadvancechar(win, fromuser);
		return(FALSE);
	}

	if (i == 'f' && m)
	{
		/* exit editor if at end of line and only editing one line */
		if (e->textarray[e->curline][e->curchar] == 0 &&
			(win->state&WINDOWTYPE) == POPTEXTWINDOW)
		{
			if (fromuser) us_editemacsshipchanges(win);
			return(TRUE);
		}

		for(;;)
		{
			if (isalnum(e->textarray[e->curline][e->curchar])) break;
			if (us_editemacsadvancechar(win, fromuser)) break;
		}
		for(;;)
		{
			if (us_editemacsadvancechar(win, fromuser)) break;
			if (!isalnum(e->textarray[e->curline][e->curchar])) break;
		}
		return(FALSE);
	}

	if (i == CTLA && !m)
	{
		e->curchar = 0;
		return(FALSE);
	}

	if (i == CTLE && !m)
	{
		for(j=0; e->textarray[e->curline][j] != 0; j++) ;
		e->curchar = j;
		return(FALSE);
	}

	if (i == CTLG && !m)
	{
		ttyputmsg(_("Analyzing this line"));
		if (fromuser) us_editemacsshipchanges(win);
		return(FALSE);
	}

	if (i == CTLP && !m)
	{
		/* exit editor if only editing one line */
		if ((win->state&WINDOWTYPE) == POPTEXTWINDOW)
		{
			if (fromuser) us_editemacsshipchanges(win);
			return(TRUE);
		}

		if (e->curline <= 0) return(FALSE);
		if (fromuser)
		{
			us_editemacsshipchanges(win);
			if (e->curline == e->firstline && e->firstline > 0)
				us_editemacsshiftscreendown(win);
		}
		e->curline--;
		for(j=0; e->textarray[e->curline][j] != 0; j++) ;
		if (e->curchar > j) e->curchar = j;
		return(FALSE);
	}

	if (i == CTLN && !m)
	{
		/* exit editor if only editing one line */
		if ((win->state&WINDOWTYPE) == POPTEXTWINDOW)
		{
			if (fromuser) us_editemacsshipchanges(win);
			return(TRUE);
		}

		if (e->curline >= e->maxlines-1) us_editemacsaddmorelines(e);
		if (fromuser)
		{
			us_editemacsshipchanges(win);
			if (e->curline == e->firstline+e->screenlines-1)
				us_editemacsshiftscreenup(win);
		}
		e->curline++;
		for(j=0; e->textarray[e->curline][j] != 0; j++) ;
		if (e->curchar > j) e->curchar = j;
		return(FALSE);
	}

	if (i == CTLZ && !m)
	{
		/* exit editor if only editing one line */
		if ((win->state&WINDOWTYPE) == POPTEXTWINDOW)
		{
			if (fromuser) us_editemacsshipchanges(win);
			return(TRUE);
		}

		us_editemacsshiftscreenup(win);
		if (fromuser && e->curline == e->firstline) e->curline++;
		return(FALSE);
	}

	if (i == 'z' && m)
	{
		/* exit editor if only editing one line */
		if ((win->state&WINDOWTYPE) == POPTEXTWINDOW)
		{
			if (fromuser) us_editemacsshipchanges(win);
			return(TRUE);
		}

		if (e->firstline <= 0) return(FALSE);
		us_editemacsshiftscreendown(win);
		if (fromuser && e->curline == e->firstline+e->screenlines-1)
			e->curline--;
		return(FALSE);
	}

	if (i == CTLV && !m)
	{
		/* exit editor if only editing one line */
		if ((win->state&WINDOWTYPE) == POPTEXTWINDOW)
		{
			if (fromuser) us_editemacsshipchanges(win);
			return(TRUE);
		}

		j = e->screenlines / 5 * 4;
		e->firstline += j;
		if ((e->state&EGRAPHICSOFF) == 0)
		{
			us_editemacsmovebox(win, 0, 0, e->swid, us_thei*(e->screenlines-j), 0, j);
			us_editemacsclearbox(win, 0, e->screenlines-j, e->swid, us_thei*j);
			for(k=0; k<j; k++)
			{
				if (e->screenlines-j+k+e->firstline < e->maxlines)
					us_editemacsredrawline(win, e->screenlines-j+k);
			}
		}
		if (fromuser && e->curline < e->firstline)
		{
			us_editemacsshipchanges(win);
			e->curline = e->firstline + e->screenlines/2;
		}
		return(FALSE);
	}

	if (i == 'v' && m)
	{
		/* exit editor if only editing one line */
		if ((win->state&WINDOWTYPE) == POPTEXTWINDOW)
		{
			if (fromuser) us_editemacsshipchanges(win);
			return(TRUE);
		}

		if (e->firstline == 0) return(FALSE);
		j = e->screenlines / 5 * 4;
		if (j > e->firstline) j = e->firstline;
		e->firstline -= j;
		if ((e->state&EGRAPHICSOFF) == 0)
		{
			us_editemacsmovebox(win, 0, j, e->swid, us_thei*(e->screenlines-j), 0, 0);
			us_editemacsclearbox(win, 0, 0, e->swid, us_thei*j);
			for(k=0; k<j; k++)
			{
				if (e->firstline+k < e->maxlines) us_editemacsredrawline(win, k);
			}
		}
		if (fromuser && e->curline >= e->screenlines+e->firstline)
		{
			us_editemacsshipchanges(win);
			e->curline = e->firstline + e->screenlines/2;
		}
		return(FALSE);
	}

	if (i == '>' && m)
	{
		if (fromuser) us_editemacsshipchanges(win);
		for(j=e->maxlines-1; j>=0; j--) if (e->textarray[j][0] != 0) break;
		e->curline = j+1;
		if (fromuser && e->curline >= e->firstline+e->screenlines)
		{
			e->firstline = e->curline - e->screenlines/2;
			us_editemacsredrawscreen(win);
		}
		e->curchar = 0;
		return(FALSE);
	}

	if (i == '<' && m)
	{
		if (fromuser)
		{
			us_editemacsshipchanges(win);
			if (e->firstline != 0)
			{
				e->firstline = 0;
				us_editemacsredrawscreen(win);
			}
		}
		e->curline = 0;
		e->curchar = 0;
		return(FALSE);
	}

	if (i == CTLO && !m)
	{
		/* exit editor if only editing one line */
		if ((win->state&WINDOWTYPE) == POPTEXTWINDOW)
		{
			if (fromuser) us_editemacsshipchanges(win);
			return(TRUE);
		}

		/* cannot change number of lines if not allowed */
		if (fromuser && (e->state&LINESFIXED) != 0)
		{
			ttyputerr(_("Cannot insert lines in this edit session"));
			return(FALSE);
		}

		/* see if there is room in the file */
		for(i=e->maxlines-1; i>=0; i--) if (e->textarray[i][0] != 0) break;
		if (i >= e->maxlines-1) us_editemacsaddmorelines(e);

		/* shift lines down */
		if (fromuser) us_editemacsworkingoncurline(e);
		for(j = i+1; j > e->curline; j--)
		{
			if (j == e->curline+1) nextline = &e->textarray[e->curline][e->curchar]; else
				nextline = e->textarray[j-1];
			len = estrlen(nextline) + 1;
			while (len > e->maxchars[j])
				us_editemacsaddmorechars(e, j);
			(void)estrcpy(e->textarray[j], nextline);
		}
		e->textarray[e->curline][e->curchar] = 0;
		if ((e->state&EGRAPHICSOFF) == 0 && e->curline-e->firstline < e->screenlines &&
			e->curline >= e->firstline)
		{
			us_editemacsclearbox(win, e->curchar, e->curline-e->firstline,
				e->swid-e->curchar*us_twid, us_thei);
		}

		/* ship changes on the current line */
		if (fromuser)
		{
			us_editemacsshipchanges(win);
			if (win->changehandler != 0)
				(*win->changehandler)(win, INSERTTEXTLINE, x_(""), e->textarray[e->curline+1], e->curline+1);
			us_editemacsworkingoncurline(e);
		}

		/* shift lines down */
		if (e->curline-e->firstline < e->screenlines-1 &&
			e->curline >= e->firstline && (e->state&EGRAPHICSOFF) == 0)
				us_editemacsmovebox(win, 0, e->curline+1-e->firstline,
					e->swid, e->shei-(e->curline+1-e->firstline)*us_thei, 0,
						e->curline-e->firstline);

		/* draw next line */
		if (e->curline+1-e->firstline < e->screenlines &&
			e->curline+1 >= e->firstline && (e->state&EGRAPHICSOFF) == 0)
		{
			us_editemacsclearbox(win, 0, e->curline+1-e->firstline,
				e->swid, us_thei);
			us_editemacstext(win, e->textarray[e->curline+1], 0, e->curline+1-e->firstline);
		}
		return(FALSE);
	}

	if ((i == '\n' || i == '\r') && !m)
	{
		/* exit editor if only editing one line */
		if ((win->state&WINDOWTYPE) == POPTEXTWINDOW)
		{
			if (fromuser) us_editemacsshipchanges(win);
			return(TRUE);
		}

		/* cannot change number of lines if not allowed */
		if (fromuser && (e->state&LINESFIXED) != 0)
		{
			ttyputerr(_("Cannot insert lines in this edit session"));
			return(FALSE);
		}

		/* see if there is room in the file */
		if (e->curline >= e->maxlines-1) us_editemacsaddmorelines(e);

		/* shift lines down */
		for(i=e->maxlines-1; i>=0; i--) if (e->textarray[i][0] != 0) break;
		if (fromuser) us_editemacsworkingoncurline(e);
		for(j = i+1; j > e->curline; j--)
		{
			if (j == e->curline+1) nextline = &e->textarray[e->curline][e->curchar]; else
				nextline = e->textarray[j-1];
			len = estrlen(nextline) + 1;
			while (len > e->maxchars[j])
				us_editemacsaddmorechars(e, j);
			(void)estrcpy(e->textarray[j], nextline);
		}
		e->textarray[e->curline][e->curchar] = 0;
		if ((e->state&EGRAPHICSOFF) == 0 && e->curline-e->firstline < e->screenlines &&
			e->curline >= e->firstline)
		{
			us_editemacsclearbox(win, e->curchar, e->curline-e->firstline,
				e->swid-e->curchar*us_twid, us_thei);
		}

		if (fromuser) us_editemacsshipchanges(win);
		e->curline++;
		e->curchar = 0;
		if (fromuser)
		{
			if (win->changehandler != 0)
				(*win->changehandler)(win, INSERTTEXTLINE, x_(""), e->textarray[e->curline], e->curline);
			if (e->curline == e->firstline+e->screenlines-1)
				us_editemacsshiftscreenup(win);
			us_editemacsworkingoncurline(e);
		}

		/* shift lines down */
		if (e->curline-e->firstline < e->screenlines-1 &&
			e->curline >= e->firstline && (e->state&EGRAPHICSOFF) == 0)
				us_editemacsmovebox(win, 0, e->curline+1-e->firstline,
					e->swid, e->shei-(e->curline+1-e->firstline)*us_thei, 0,
						e->curline-e->firstline);

		/* clear current line */
		if (e->curline-e->firstline < e->screenlines &&
			e->curline >= e->firstline && (e->state&EGRAPHICSOFF) == 0)
		{
			us_editemacsclearbox(win, 0, e->curline-e->firstline,
				e->swid, us_thei);
			us_editemacstext(win, e->textarray[e->curline], 0, e->curline-e->firstline);
		}
		return(FALSE);
	}

	/* flash the display */
	ttyputerr(_("The key '%s' is not valid in EMACS"),
		us_describeboundkey((INTSML)i, (!m ? 0 : ACCELERATORDOWN), 1));
	return(FALSE);
}

/*
 * routine to back the cursor by one character.  Returns true if at the
 * start of the file
 */
BOOLEAN us_editemacsbackupchar(WINDOWPART *win, BOOLEAN fromuser)
{
	REGISTER EDITOR *e;
	REGISTER INTBIG j;

	e = win->editor;
	if (e == NOEDITOR) return(TRUE);

	if (e->curchar > 0)
	{
		e->curchar--;
		return(FALSE);
	}
	if (e->curline <= 0) return(TRUE);

	if (fromuser)
	{
		us_editemacsshipchanges(win);
		if (e->curline == e->firstline && e->firstline > 0)
			us_editemacsshiftscreendown(win);
	}
	e->curline--;
	for(j=0; e->textarray[e->curline][j] != 0; j++) ;
	e->curchar = j;
	return(FALSE);
}

/*
 * routine to advance the cursor by one character.  Returns true if at the
 * end of the file
 */
BOOLEAN us_editemacsadvancechar(WINDOWPART *win, BOOLEAN fromuser)
{
	REGISTER EDITOR *e;
	REGISTER BOOLEAN atend;
	REGISTER INTBIG j;

	e = win->editor;
	if (e == NOEDITOR) return(TRUE);

	if (e->textarray[e->curline][e->curchar] != 0)
	{
		e->curchar++;
		return(FALSE);
	}

	atend = TRUE;
	for(j=e->curline+1; j < e->maxlines; j++)
	{
		if (e->textarray[j][0] == 0) continue;
		atend = FALSE;
		break;
	}
	if (e->curline >= e->maxlines-1) us_editemacsaddmorelines(e);
	if (fromuser)
	{
		us_editemacsshipchanges(win);
		if (e->curline == e->firstline+e->screenlines-1)
			us_editemacsshiftscreenup(win);
	}
	e->curline++;
	e->curchar = 0;
	return(atend);
}

/*
 * routine to delete the current character
 */
void us_editemacsdeletechar(WINDOWPART *win, BOOLEAN fromuser)
{
	REGISTER INTBIG j, curl, len;
	REGISTER CHAR *nextline;
	CHAR s[2];
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	/* at end of line: delete an entire line */
	if (e->textarray[e->curline][e->curchar] == 0)
	{
		/* only continue if there is a valid next line */
		if (e->curline < e->maxlines-1)
		{
			/* start by appending the next line to this */
			if (fromuser) us_editemacsworkingoncurline(e);
			nextline = e->textarray[e->curline+1];
			len = estrlen(e->textarray[e->curline]) + estrlen(nextline);
			while (len > e->maxchars[e->curline])
				us_editemacsaddmorechars(e, e->curline);
			(void)estrcat(e->textarray[e->curline], nextline);
			us_editemacsshipchanges(win);
			us_editemacsredrawline(win, e->curline);

			/* now report deletion of following line */
			if (win->changehandler != 0)
				(*win->changehandler)(win, DELETETEXTLINE, x_(""), e->textarray[e->curline+1], e->curline+1);
			us_editemacsworkingoncurline(e);

			/* shift up lines in memory */
			for(curl = e->curline+1; curl < e->maxlines; curl++)
			{
				if (curl == e->maxlines-1) nextline = x_(""); else
					nextline = e->textarray[curl+1];
				if (estrcmp(e->textarray[curl], nextline) == 0) continue;
				len = estrlen(nextline);
				while (len > e->maxchars[curl])
					us_editemacsaddmorechars(e, curl);
				(void)estrcpy(e->textarray[curl], nextline);
			}

			/* delete line on the screen */
			curl = e->curline + 1;
			if (curl-e->firstline < e->screenlines-1 &&
				curl >= e->firstline && (e->state&EGRAPHICSOFF) == 0)
			{
				us_editemacsmovebox(win, 0, curl-e->firstline, e->swid,
					e->shei-(curl-e->firstline+1)*us_thei, 0,
						curl-e->firstline+1);
				if (e->screenlines < e->maxlines)
				{
					us_editemacsclearbox(win, 0, e->screenlines-1, e->swid, us_thei);
					us_editemacsredrawline(win, e->screenlines-1);
				}
			}
		}
		return;
	}

	/* just delete the character in the line */
	for(j=e->curchar; j<e->maxchars[e->curline]; j++)
		e->textarray[e->curline][j] = e->textarray[e->curline][j+1];
	if (e->curline-e->firstline < e->screenlines && e->curline >= e->firstline &&
		e->curchar < e->screenchars && (e->state&EGRAPHICSOFF) == 0)
	{
		us_editemacsmovebox(win, e->curchar, e->curline-e->firstline,
			e->swid-e->curchar*us_twid-us_twid, us_thei,
				e->curchar+1, e->curline-e->firstline);
		if ((INTBIG)estrlen(e->textarray[e->curline]) >= e->screenchars)
		{
			us_editemacsclearbox(win, e->screenchars-1, e->curline-e->firstline,
				us_twid, us_thei);
			s[0] = e->textarray[e->curline][e->screenchars-1];   s[1] = 0;
			us_editemacstext(win, s, e->screenchars-1, e->curline-e->firstline);
		}
		us_editemacscleanupline(win, e->curline);
	}
}

/*
 * Routine to search for "str" (search in the reverse direction if "reverse" is true).
 * Start search from the top if "fromtop" is true.  Issue proper calls if this was
 * made by the user ("fromuser" true).
 */
void us_editemacsdosearch(WINDOWPART *win, CHAR *str, BOOLEAN reverse,
	BOOLEAN fromtop, BOOLEAN casesensitive, BOOLEAN fromuser)
{
	REGISTER EDITOR *e;
	REGISTER INTBIG i, j, k, startchr, startlne, match;

	/* search for the string */
	e = win->editor;
	i = estrlen(str);
	if (fromtop)
	{
		/* search from the top of the buffer */
		startchr = 0;   startlne = 0;
	} else
	{
		/* search from the end of the current selection */
		startchr = e->curchar;   startlne = e->curline;
	}
	j = startlne;
	k = startchr;
	for(;;)
	{
		if (reverse)
		{
			/* reverse search: backup one place */
			k--;
			if (k < 0)
			{
				j--;
				if (j < 0) j = e->maxlines - 1;
				k = estrlen(e->textarray[j]);
			}
		} else
		{
			/* forward search: advance by one character */
			if (e->textarray[j][k] != 0) k++; else
			{
				/* advance to the next line */
				k = 0;
				j++;
				if (j >= e->maxlines) j = 0;
			}
		}

		/* if it came all the way around, stop */
		if (k == startchr && j == startlne)
		{
			ttyputmsg(_("Can't find \"%s\""), str);
			break;
		}

		/* if string matches, set cursor pointers */
		if (casesensitive)
		{
			match = estrncmp(str, &e->textarray[j][k], i);
		} else
		{
			match = namesamen(str, &e->textarray[j][k], i);
		}
		if (match == 0)
		{
			if (fromuser) us_editemacsshipchanges(win);
			if (reverse) e->curchar = k; else e->curchar = k+i;
			e->curline = j;
			us_editemacsensuretextshown(win, e->curline);
			break;
		}
	}
}

/*
 * routine to shift the text screen up one line
 */
void us_editemacsshiftscreenup(WINDOWPART *win)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	e->firstline++;
	if ((e->state&EGRAPHICSOFF) != 0) return;
	us_editemacsmovebox(win, 0, 0, e->swid, us_thei*(e->screenlines-1), 0, 1);
	us_editemacsclearbox(win, 0, e->screenlines-1, e->swid, us_thei);
	if (e->screenlines+e->firstline <= e->maxlines)
		us_editemacsredrawline(win, e->screenlines-1);
}

/*
 * routine to shift the text screen down one line
 */
void us_editemacsshiftscreendown(WINDOWPART *win)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	e->firstline--;
	if ((e->state&EGRAPHICSOFF) != 0) return;
	us_editemacsmovebox(win, 0, 1, e->swid, us_thei*(e->screenlines-1), 0, 0);
	us_editemacsclearbox(win, 0, 0, e->swid, us_thei);
	us_editemacsredrawline(win, 0);
}

/*
 * routine to ensure that line "line" is shown in the display
 */
void us_editemacsensuretextshown(WINDOWPART *win, INTBIG line)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	if (line-e->firstline >= e->screenlines || line < e->firstline)
	{
		e->firstline = maxi(0, line - e->screenlines/2);
		us_editemacsredrawscreen(win);
	}
}

/*
 * routine to redisplay the text screen
 */
void us_editemacsredrawscreen(WINDOWPART *win)
{
	REGISTER INTBIG j;
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;
	if ((e->state&EGRAPHICSOFF) != 0) return;

	/* put header on display */
	us_editemacssetheader(win, e->header);

	/* clear the screen */
	us_editemacsclearbox(win, 0, 0, e->swid, e->shei);

	/* rewrite each line */
	for(j=0; j<e->screenlines; j++) if (j+e->firstline < e->maxlines)
		us_editemacsredrawline(win, j);
}

/*
 * routine to write line "j" of the screen
 */
void us_editemacsredrawline(WINDOWPART *win, INTBIG j)
{
	REGISTER INTBIG save;
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	if ((INTBIG)estrlen(e->textarray[j+e->firstline]) > e->screenchars)
	{
		save = e->textarray[j+e->firstline][e->screenchars];
		e->textarray[j+e->firstline][e->screenchars] = 0;
	} else save = 0;
	us_editemacstext(win, e->textarray[j+e->firstline], 0, j);
	if (save != 0) e->textarray[j+e->firstline][e->screenchars] = (CHAR)save;
}

/*
 * routine to write the header string
 */
void us_editemacssetheader(WINDOWPART *win, CHAR *header)
{
	REGISTER INTBIG save;
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;
	screendrawbox(win, win->uselx, win->usehx, win->usely, win->usehy, &us_ebox);
	if ((INTBIG)estrlen(header) > e->screenchars)
	{
		save = header[e->screenchars];
		header[e->screenchars] = 0;
	} else save = 0;
	us_editemacstext(win, header, 0, -2);
	if ((win->state&WINDOWTYPE) == POPTEXTWINDOW)
		us_editemacstext(win, _("EMACS editor: type RETURN when done"), 0, -1); else
	{
		if (estrcmp(win->location, x_("entire")) != 0)
			us_editemacstext(win, _("EMACS editor: type ^Xd when done"), 0, -1); else
		{
			if (graphicshas(CANUSEFRAMES))
				us_editemacstext(win, _("EMACS editor: close the window when done"), 0, -1); else
					us_editemacstext(win, _("EMACS editor: text-only cell"), 0, -1);
		}
	}
	if (save != 0) header[e->screenchars] = (CHAR)save;
	screeninvertbox(win, e->offx-1, e->offx+e->swid,
		e->revy+1, e->revy+us_thei*HEADERLINES);
	us_menufigs.col = el_colmengly;
	screendrawline(win, e->offx-1, e->revy, e->offx-1,
		e->revy-e->shei-1, &us_menufigs, 0);
	screendrawline(win, e->offx+e->swid+1, e->revy,
		e->offx+e->swid+1, e->revy-e->shei-1, &us_menufigs, 0);
	screendrawline(win, e->offx-1, e->revy-e->shei-1,
		e->offx+e->swid+1, e->revy-e->shei-1, &us_menufigs, 0);
}

/*
 * Routine to return the next valid character (ignoring end-of-lines).
 */
CHAR us_editemacsnextvalidcharacter(EDITOR *e)
{
	INTBIG line, chr;

	line = e->curline;   chr = e->curchar;
	while (line < e->maxlines && e->textarray[line][chr] == 0)
	{
		line++;
		chr = 0;
	}
	return(e->textarray[line][chr]);
}

/*
 * routine to invert the character cursor, turning it on or off
 */
void us_editemacsflashcursor(WINDOWPART *win)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	if (e->curline-e->firstline < e->screenlines && e->curline >= e->firstline &&
		e->curchar < e->screenchars && (e->state&EGRAPHICSOFF) == 0)
			us_editemacsinvertbox(win, e->curchar, e->curline-e->firstline, us_twid,
				us_thei);
}

/*
 * turn off the highlighting of a line of text (if one is highlighted)
 */
void us_editemacsoffhighlight(WINDOWPART *win)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	if (e->highlightedline < 0) return;
	if (e->highlightedline-e->firstline < e->screenlines &&
		e->highlightedline >= e->firstline && (e->state&EGRAPHICSOFF) == 0)
			us_editemacsinvertbox(win, 0, e->highlightedline-e->firstline,
				e->swid, us_thei);
	e->highlightedline = -1;
}

/*
 * routine to cleanup the bits at the end of a line that are not a full-width
 * character
 */
void us_editemacscleanupline(WINDOWPART *win, INTBIG line)
{
	REGISTER INTBIG residue;
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	residue = e->swid-e->screenchars*us_twid;
	if (residue == 0) return;
	us_editemacsclearbox(win, e->screenchars, line-e->firstline, residue, us_thei);
}

/*
 * routine to declare that the user is now working on line "line"
 */
void us_editemacsworkingoncurline(EDITOR *e)
{
	if (e->working == -1)
	{
		/* first time: save the line */
		(void)estrcpy(e->formerline, e->textarray[e->curline]);
		e->working = e->curline;
	}
}

/*
 * routine to declare that the user is done with line "line"
 */
void us_editemacsshipchanges(WINDOWPART *win)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	if (e->working == -1) return;
	us_editemacsflashcursor(win);
	if (win->changehandler != 0)
		(*win->changehandler)(win, REPLACETEXTLINE, e->formerline,
			e->textarray[e->curline], e->working);
	us_editemacsflashcursor(win);
	e->working = -1;
}

/******************** ALLOCATION ********************/

/*
 * routine to double the number of lines in the text buffer
 */
void us_editemacsaddmorelines(EDITOR *e)
{
	REGISTER INTBIG oldlines, i;
	REGISTER INTBIG *maxchars;
	REGISTER CHAR **textarray;

	/* save former buffer size, double it */
	oldlines = e->maxlines;
	e->maxlines *= 2;

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
			textarray[i] = (CHAR *)emalloc((e->screenchars+1) * SIZEOFCHAR, us_tool->cluster);
			if (textarray[i] == 0) ttyputnomemory();
			textarray[i][0] = 0;
			maxchars[i] = e->screenchars;
		} else
		{
			textarray[i] = (CHAR *)emalloc((e->maxchars[i]+1) * SIZEOFCHAR, us_tool->cluster);
			if (textarray[i] == 0) ttyputnomemory();
			(void)estrcpy(textarray[i], e->textarray[i]);
			maxchars[i] = e->maxchars[i];
		}
	}

	/* free old arrays */
	for(i=0; i<oldlines; i++) efree(e->textarray[i]);
	efree((CHAR *)e->textarray);
	efree((CHAR *)e->maxchars);

	/* setup pointers correctly */
	e->textarray = textarray;
	e->maxchars = maxchars;
}

/*
 * routine to double the number of characters in line "line"
 */
void us_editemacsaddmorechars(EDITOR *e, INTBIG line)
{
	REGISTER CHAR *oldline;

	oldline = e->textarray[line];
	e->maxchars[line] *= 2;
	e->textarray[line] = (CHAR *)emalloc((e->maxchars[line]+1) * SIZEOFCHAR, us_tool->cluster);
	(void)estrcpy(e->textarray[line], oldline);
	if (e->maxchars[line] > e->mostchars)
	{
		e->mostchars = e->maxchars[line];
		efree(e->formerline);
		us_editemacsgetbuffers(e);
	}
}

/*
 * routine to allocate the single-line buffers
 */
void us_editemacsgetbuffers(EDITOR *e)
{
	e->formerline = (CHAR *)emalloc((e->mostchars+1) * SIZEOFCHAR, us_tool->cluster);
	if (e->formerline == 0) ttyputnomemory();
	if (e->mostchars > us_killbufchars)
	{
		if (us_killbufchars != 0) efree(us_killbuf);
		us_killbuf = (CHAR *)emalloc(e->mostchars * SIZEOFCHAR, us_tool->cluster);
		if (us_killbuf == 0) ttyputnomemory();
		us_killbufchars = e->mostchars;
	}
}

/******************** GRAPHIC SUPPORT ********************/

/*
 * move text starting at character position (sx,sy) to character position
 * (dx, dy).
 */
void us_editemacsmovebox(WINDOWPART *win, INTBIG dx, INTBIG dy, INTBIG wid,
	INTBIG hei, INTBIG sx, INTBIG sy)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;
	screenmovebox(win, sx*us_twid+e->offx, e->revy-(sy*us_thei+hei-1), wid, hei,
		dx*us_twid+e->offx, e->revy-(dy*us_thei+hei-1));
}

/*
 * erase text starting at character position (dx,dy) for (wid, hei)
 */
void us_editemacsclearbox(WINDOWPART *win, INTBIG dx, INTBIG dy, INTBIG wid, INTBIG hei)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	dx = (dx * us_twid) + e->offx;
	dy *= us_thei;
	screendrawbox(win, dx, dx+wid-1, e->revy-(dy+hei-1), e->revy-dy, &us_ebox);
}

/*
 * invert text starting at character position (dx,dy) for (wid, hei)
 */
void us_editemacsinvertbox(WINDOWPART *win, INTBIG dx, INTBIG dy, INTBIG wid, INTBIG hei)
{
	REGISTER EDITOR *e;

	e = win->editor;
	if (e == NOEDITOR) return;

	dx = (dx * us_twid) + e->offx;
	dy *= us_thei;
	screeninvertbox(win, dx, dx+wid-1, e->revy-(dy+hei-1), e->revy-dy);
}

/*
 * write text "str" starting at character position (x,y)
 */
void us_editemacstext(WINDOWPART *win, CHAR *str, INTBIG x, INTBIG y)
{
	REGISTER EDITOR *e;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	e = win->editor;
	if (e == NOEDITOR) return;

	TDCLEAR(descript);
	TDSETSIZE(descript, us_editemacsfont);
	screensettextinfo(win, NOTECHNOLOGY, descript);
	us_menutext.col = el_colmentxt;
	screendrawtext(win, x*us_twid + e->offx, e->revy-(y+1)*us_thei,
		str, &us_menutext);
}
