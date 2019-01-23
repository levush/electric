/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dbterminal.c
 * General messages terminal output handler
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
#include "usr.h"
#include "edialogs.h"

static INTBIG   us_ttymute = 0;				/* nonzero to supress unimportant messages */
static INTBIG	us_ttyttywriteseparator = 0;	/* nonzero to write separator before next text */
static INTBIG   us_sepcount = 0;			/* index of separator lines */

/***************************** TERMINAL OUTPUT *****************************/

/*
 * routine to mute nonerror messages if flag is nonzero
 * (returns previous state)
 */
INTBIG ttyquiet(INTBIG flag)
{
	REGISTER INTBIG prev;

	prev = us_ttymute;
	us_ttymute = flag;
	return(prev);
}

void ttynewcommand(void)
{
	us_ttyttywriteseparator = 1;
}

/*
 * routine to output a normal message into the messages window
 * in the style of "printf"
 */
void ttyputmsg(CHAR *msg, ...)
{
	va_list ap;

	/* don't print or save this message if muted */
	if (us_ttymute != 0) return;

	/* don't print this message if quit or aborted */
	if (el_pleasestop != 0) return;

	var_start(ap, msg);
	us_ttyprint(FALSE, msg, ap);
	va_end(ap);
}

/*
 * routine to output a "verbose" message (those that provide unimportant
 * information that can be done without) into the messages window
 * in the style of "printf"
 */
void ttyputverbose(CHAR *msg, ...)
{
	va_list ap;

	/* ignore this message if facts are turned off */
	if ((us_tool->toolstate&JUSTTHEFACTS) != 0) return;

	/* don't print or save this message if muted */
	if (us_ttymute != 0) return;

	/* don't print this message if quit or aborted */
	if (el_pleasestop != 0) return;

	var_start(ap, msg);
	us_ttyprint(FALSE, msg, ap);
	va_end(ap);
}

/*
 * routine to output an error message into the messages window
 * in the style of "printf"
 */
void ttyputerr(CHAR *msg, ...)
{
	va_list ap;

	var_start(ap, msg);
	ttybeep(SOUNDBEEP, FALSE);
	us_ttyprint(TRUE, msg, ap);
	va_end(ap);
}

/*
 * Routine to put out a message that is in 2 parts: a keystroke and a meaning.
 * The length of the "keystroke" field is "length".
 */
void ttyputinstruction(CHAR *keystroke, INTBIG length, CHAR *meaning)
{
	REGISTER INTBIG i;
	REGISTER void *infstr;

	infstr = initinfstr();
	for(i=0; i<length; i++)
	{
		if (*keystroke == 0) addtoinfstr(infstr, ' '); else
			addtoinfstr(infstr, *keystroke++);
	}
	addstringtoinfstr(infstr, meaning);
	ttyputmsg(x_("%s"), returninfstr(infstr));
}

void ttyputusage(CHAR *usage)
{
	ttyputerr(_("Usage: %s"), usage);
}

void ttyputbadusage(CHAR *command)
{
	ttyputerr(_("Bad '%s' command"), command);
}

void ttyputnomemory(void)
{
	ttyputerr(_("No memory"));
}

/*
 * routine to cause the current command to be aborted by removing it from
 * any macro and by printing the message on the terminal
 */
void us_abortcommand(CHAR *msg, ...)
{
	va_list ap;

	var_start(ap, msg);
	us_ttyprint(TRUE, msg, ap);
	va_end(ap);
	us_state |= COMMANDFAILED;
	if (getvalkey((INTBIG)us_tool, VTOOL, VSTRING, us_macrobuildingkey) != NOVARIABLE)
		ttyputmsg(_("Command ignored from macro definition"));
	us_unknowncommand();
}

/*
 * routine to cause further single-key commands to be ignored until a
 * carriage-return is typed.
 */
void us_unknowncommand(void)
{
	REGISTER INTBIG i;

	if ((us_tool->toolstate&NOKEYLOCK) != 0) return;
	i = ttyquiet(0);
	ttyputmsg(_("Single-key commands will be ignored until the next RETURN"));
	(void)ttyquiet(i);
	us_state |= SKIPKEYS;
}

void us_abortedmsg(void)
{
	if ((us_tool->toolstate & JUSTTHEFACTS) == 0) us_abortcommand(_("Aborted")); else
	{
		us_state |= COMMANDFAILED;
		if (getvalkey((INTBIG)us_tool, VTOOL, VSTRING, us_macrobuildingkey) != NOVARIABLE)
			ttyputmsg(_("Command ignored from macro definition"));
		us_unknowncommand();
	}
}

/****************************** TERMINAL INPUT ******************************/

/*
 * ttygetchar gets the next character from the text keyboard.
 * The routine must call the graphics module to do input.
 */
INTSML ttygetchar(INTBIG *special)
{
	return(getnxtchar(special));
}

/* ttygetline() Dialog */
static DIALOGITEM us_ttyinputdialogitems[] =
{
 /*  1 */ {0, {96,200,120,264}, BUTTON, N_("OK")},
 /*  2 */ {0, {96,24,120,88}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {58,38,74,246}, EDITTEXT, x_("")},
 /*  4 */ {0, {3,9,51,281}, MESSAGE, x_("")}
};
static DIALOG us_ttyinputdialog = {{50,75,188,371}, x_(""), 0, 4, us_ttyinputdialogitems, 0, 0};

/* special items for the "ttygetline" dialog: */
#define DGTL_LINE     3		/* line (edit text) */
#define DGTL_PROMPT   4		/* prompt (stat text) */

/*
 * routine to print "prompt" and then read a line of text from the terminal
 * The address of the text line is returned (0 if cancelled).
 */
CHAR *ttygetline(CHAR *prompt)
{
	INTBIG itemHit;
	CHAR *line, *pt, *defaultval, localprompt[256];
	REGISTER void *dia;

	/* parse default value */
	estrcpy(localprompt, prompt);
	defaultval = x_("");
	for(pt=localprompt; *pt != 0; pt++) if (*pt == '[') break;
	if (*pt != 0)
	{
		*pt++ = 0;
		defaultval = pt;
		for( ; *pt != 0; pt++) if (*pt == ']') break;
		if (*pt == ']') *pt = 0;
	}

	/* display the dialog box */
	dia = DiaInitDialog(&us_ttyinputdialog);
	if (dia == 0) return(0);
	DiaSetText(dia, DGTL_PROMPT, localprompt);
	DiaSetText(dia, -DGTL_LINE, defaultval);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL) break;
		if (itemHit == OK) break;
	}

	if (itemHit != CANCEL)
		line = us_putintoinfstr(DiaGetText(dia, DGTL_LINE));

	/* terminate the dialog */
	DiaDoneDialog(dia);
	if (itemHit == CANCEL) return(0);
	return(line);
}

/*
 * routine to display "prompt" and then accept a line of text from the
 * messages window.  The address of the text line is returned.  Returns
 * zero if end-of-file is typed (^D).
 */
CHAR *ttygetlinemessages(CHAR *prompt)
{
	return(getmessagesstring(prompt));
}

/*
 * Routine to clear the messages window.
 */
void ttyclearmessages(void)
{
	clearmessageswindow();
	us_sepcount = 0;
}

/*
 * Routine to write information into trace file
 */
void etrace(INTBIG mode, CHAR *s, ...)
{
	va_list ap;
	CHAR line[500];

	if (us_tracefile == NULL)
	{
		us_tracefile = xcreate(x_("trace.txt"), el_filetypetext, 0, 0);
		if (us_tracefile == NULL) return;
	}

	/* build the error message */
	var_start(ap, s);
	evsnprintf(line, 500, s, ap);
	va_end(ap);

	efprintf(us_tracefile, line);
	fflush(us_tracefile);
}

/***************************** INTERNAL SUPPORT *****************************/

/*
 * internal routine to print a message in the scrolling area with the
 * style of "printf".  Pops up the messages window if "important" is true.
 */
void us_ttyprint(BOOLEAN important, CHAR *msg, va_list ap)
{
	CHAR line[8192];
	REGISTER INTBIG i, j, k;

	if (us_ttyttywriteseparator != 0)
	{
		us_sepcount++;
		esnprintf(line, 8192,
			x_("================================= %ld ================================="),
				us_sepcount);
		putmessagesstring(line, important);
		if (us_termaudit != 0) xprintf(us_termaudit, x_("%s\n"), line);
		us_ttyttywriteseparator = 0;
	}

	/* build the output line */
	evsnprintf(line, 8192, msg, ap);

	/* remove excessive trailing space */
	i = estrlen(line);
	while (i > 1 && (line[i-1] == ' ' || line[i-1] == '\t') &&
		(line[i-2] == ' ' || line[i-2] == '\t')) line[--i] = 0;

	/* break the line at newline characters */
	for(k=j=0; j<i; j++)
	{
		if (line[j] == '\n')
		{
			line[j] = 0;
			putmessagesstring(&line[k], important);
			if (us_termaudit != 0) xprintf(us_termaudit, x_("%s\n"), &line[k]);
			k = j + 1;
		}
	}

	if (k < i)
	{
		putmessagesstring(&line[k], important);
		if (us_termaudit != 0) xprintf(us_termaudit, x_("%s\n"), &line[k]);
	}
}
