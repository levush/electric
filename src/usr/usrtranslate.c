/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrtranslate.c
 * User interface tool: language translator
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
 * support@staticfreesoft.com
 */

#include "global.h"
#include "edialogs.h"
#include "usr.h"
#include "usrdiacom.h"

/********************************** CONTROL **********************************/

/* User Interface: Translate */
static DIALOGITEM us_transdialogitems[] =
{
 /*  1 */ {0, {336,132,360,212}, BUTTON, N_("Next")},
 /*  2 */ {0, {148,8,164,96}, MESSAGE, N_("Translation:")},
 /*  3 */ {0, {148,100,196,528}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,324,24,472}, MESSAGE, N_("Total strings:")},
 /*  5 */ {0, {8,473,24,529}, MESSAGE, x_("")},
 /*  6 */ {0, {28,324,44,472}, MESSAGE, N_("Untranslated strings:")},
 /*  7 */ {0, {28,472,44,528}, MESSAGE, x_("")},
 /*  8 */ {0, {48,324,64,472}, MESSAGE, N_("Fuzzy strings:")},
 /*  9 */ {0, {48,472,64,528}, MESSAGE, x_("")},
 /* 10 */ {0, {92,8,108,96}, MESSAGE, N_("English:")},
 /* 11 */ {0, {92,100,140,528}, EDITTEXT, x_("")},
 /* 12 */ {0, {320,440,344,520}, BUTTON, N_("DONE")},
 /* 13 */ {0, {172,8,188,96}, CHECK, N_("Fuzzy")},
 /* 14 */ {0, {336,220,360,300}, BUTTON, N_("Prev")},
 /* 15 */ {0, {336,12,360,92}, BUTTON, N_("First")},
 /* 16 */ {0, {48,8,80,316}, MESSAGE, x_("")},
 /* 17 */ {0, {288,441,312,521}, BUTTON, N_("Save")},
 /* 18 */ {0, {8,8,24,132}, MESSAGE, N_("Language:")},
 /* 19 */ {0, {8,132,24,188}, POPUP, x_("")},
 /* 20 */ {0, {204,100,264,528}, SCROLL, x_("")},
 /* 21 */ {0, {204,8,220,96}, MESSAGE, N_("Comments:")},
 /* 22 */ {0, {272,12,288,100}, MESSAGE, N_("Choose:")},
 /* 23 */ {0, {272,100,288,300}, POPUP, x_("")},
 /* 24 */ {0, {8,192,24,304}, POPUP, x_("")},
 /* 25 */ {0, {272,328,296,408}, BUTTON, N_("Unix-to-Mac")},
 /* 26 */ {0, {305,328,329,408}, BUTTON, N_("Mac-to-UNIX")},
 /* 27 */ {0, {292,100,308,300}, EDITTEXT, x_("")},
 /* 28 */ {0, {292,24,308,84}, BUTTON, N_("Find:")},
 /* 29 */ {0, {68,324,84,472}, MESSAGE, N_("Found strings:")},
 /* 30 */ {0, {68,472,84,528}, MESSAGE, x_("")},
 /* 31 */ {0, {312,24,328,152}, CHECK, N_("Find in English")},
 /* 32 */ {0, {312,156,328,300}, CHECK, N_("Find in translation")},
 /* 33 */ {0, {337,328,361,408}, BUTTON, N_("Validate")}
};
static DIALOG us_transdialog = {{75,75,444,613}, N_("Translate Electric Strings"), 0, 33, us_transdialogitems, 0, 0};

#define DTRN_NEXT       1		/* next string (button) */
#define DTRN_FOREIGN    3		/* translated string (edit text) */
#define DTRN_TOTSTR     5		/* total strings (message) */
#define DTRN_UNTSTR     7		/* untranslated strings (message) */
#define DTRN_FUZSTR     9		/* fuzzy strings (message) */
#define DTRN_ENGLISH   11		/* english string (message) */
#define DTRN_DONE      12		/* exit translator (button) */
#define DTRN_ISFUZZY   13		/* fuzzy indication (check) */
#define DTRN_PREV      14		/* previous string (button) */
#define DTRN_FIRST     15		/* first string (button) */
#define DTRN_MSGID     16		/* current message information (message) */
#define DTRN_SAVE      17		/* save (button) */
#define DTRN_LANGUAGE  19		/* language list (popup) */
#define DTRN_COMMENTS  20		/* comments (scroll) */
#define DTRN_NEXTTYPE  23		/* prev/next action (popup) */
#define DTRN_PLATFORM  24		/* platform list (popup) */
#define DTRN_UNIXTOMAC 25		/* Convert UNIX to Mac (button) */
#define DTRN_MACTOUNIX 26		/* Convert Mac to UNIX (button) */
#define DTRN_FINDSTR   27		/* String to find (message) */
#define DTRN_FINDBUT   28		/* Find (button) */
#define DTRN_FOUNDSTR  30		/* found strings (message) */
#define DTRN_FINDENG   31		/* Find in english (check) */
#define DTRN_FINDTRN   32		/* Find in translation (check) */
#define DTRN_VALIDATE  33		/* Validate translation (button) */

/* the type of strings being shown */
#define CHOOSEUNTRANS    0			/* show untranslated strings */
#define CHOOSEJUSTTRANS  1			/* show just translated strings */
#define CHOOSEFUZZY      2			/* show fuzzy strings */
#define CHOOSENLFUZZY    3			/* show no longer fuzzy strings */
#define CHOOSEFIND       4			/* show strings matching search pattern */
#define CHOOSEALL        5			/* show all strings */

#define NOSTRINGENTRY ((STRINGENTRY *)-1)

/* the meaning of STRINGENTRY->flags: */
#define CFORMAT     1
#define FUZZY       2
#define WASFUZZY    4		/* was fuzzy at start of session */
#define THISTIME    8		/* messages in doubt at start of session */
#define MATCHES    16		/* string matches search criteria */

typedef struct Istringentry
{
	INTBIG   headerlinecount;
	CHAR   **headerlines;
	CHAR    *english;
	CHAR    *translated;
	INTBIG   index;
	INTBIG   flags;
	struct Istringentry *nextstringentry;
	struct Istringentry *prevstringentry;
} STRINGENTRY;

STRINGENTRY *us_transfirststringentry;

static INTBIG    us_transheaderlinecount;
static CHAR    **us_transheaderlines;

static INTBIG    us_transgatherlinecount;
static INTBIG    us_transgatherlinetotal = 0;
static CHAR    **us_transgatherlines;
static INTBIG    us_filetypetrans = 0;
static CHAR     *us_translation;
static INTBIG    us_translationsize = 0;

/* prototypes for local routines */
static BOOLEAN      us_transloadmessages(CHAR *file, INTBIG *entrycount, INTBIG *untranslatedcount,
						INTBIG *fuzzycount, void *dia);
static BOOLEAN      us_transgathermessage(CHAR *line);
static STRINGENTRY *us_transfirst(INTBIG choose);
static void         us_transhowentry(STRINGENTRY *se, void *dia);
static void         us_transave(CHAR *language, INTBIG platform);
static CHAR        *us_transgetfiles(CHAR *language, INTBIG platform);
static CHAR        *us_transquoteit(CHAR *message);
static STRINGENTRY *us_translast(INTBIG choose);
static BOOLEAN      us_transmeetscriteria(STRINGENTRY *se, INTBIG choose);
static void         us_transmactounix(CHAR *language);
static void         us_transdumpline(FILE *io, CHAR *line, CHAR *prefix);
static INTBIG       us_namesamennoamp(CHAR *pt1, CHAR *pt2, INTBIG count);
static void         us_transvalidate(void);

/*
 * Routine called to cleanup memory associated with translation.
 */
void us_freetranslatememory(void)
{
	if (us_translationsize > 0) efree((CHAR *)us_translation);
}

/*
 * Routine called to translate Electric.
 */
void us_translationdlog(void)
{
	INTBIG itemno, total, untranslatedcount, fuzzycount,  i, j, langcount, len, choose,
		platform, ptlen, selen, tests, foundcount;
	BOOLEAN dirty, findenglish, findtranslation;
	CHAR *language, num[20], *pt, *sept, *par[6], **filelist, intlpath[300], **langlist;
	REGISTER STRINGENTRY *se, *curse;
	REGISTER void *dia;

	if (us_filetypetrans == 0)
		us_filetypetrans = setupfiletype(x_(""), x_("*.*"), MACFSTAG('TEXT'), FALSE, x_("transfile"), _("Translation File"));

	/* examine the translation area */
	esnprintf(intlpath, 300, x_("%sinternational%s"), el_libdir, DIRSEPSTR);
	len = estrlen(intlpath);
	j = filesindirectory(intlpath, &filelist);
	if (j <= 0)
	{
		ttyputerr(_("No folders in '%s' to translate"), intlpath);
		return;
	}
	langlist = (CHAR **)emalloc(j * (sizeof (CHAR *)), el_tempcluster);
	if (langlist == 0) return;
	langcount = 0;
	for(i=0; i<j; i++)
	{
		if (filelist[i][0] == '.' || namesame (filelist[i], x_("CVS")) == 0) continue;
		estrcpy(&intlpath[len], filelist[i]);
		if (fileexistence(intlpath) != 2) continue;
		langlist[langcount++] = filelist[i];
	}

	/* show the dialog */
	dia = DiaInitDialog(&us_transdialog);
	if (dia == 0) return;
	DiaInitTextDialog(dia, DTRN_COMMENTS, DiaNullDlogList, DiaNullDlogItem,
		DiaNullDlogDone, -1, 0);
	DiaSetControl(dia, DTRN_FINDENG, 1);

	/* setup the different languages */
	DiaSetPopup(dia, DTRN_LANGUAGE, langcount, langlist);
	language = langlist[0];

	/* setup the different types of messages to preview */
	par[CHOOSEUNTRANS]   = _("Untranslated");
	par[CHOOSEJUSTTRANS] = _("Just translated");
	par[CHOOSEFUZZY]     = _("Fuzzy");
	par[CHOOSENLFUZZY]   = _("No longer fuzzy");
	par[CHOOSEFIND]      = _("Matching");
	par[CHOOSEALL]       = _("All");
	DiaSetPopup(dia, DTRN_NEXTTYPE, 6, par);
	choose = CHOOSEUNTRANS;
	DiaSetPopupEntry(dia, DTRN_NEXTTYPE, choose);

	/* setup the different translation files */
	par[0] = _("Macintosh");
	par[1] = _("Windows,UNIX");
	DiaSetPopup(dia, DTRN_PLATFORM, 2, par);	
#ifdef MACOS
	platform = 0;
#else
	platform = 1;
#endif
	DiaSetPopupEntry(dia, DTRN_PLATFORM, platform);

	/* load the selected set of messages */
	pt = us_transgetfiles(language, platform);
	if (us_transloadmessages(pt, &total, &untranslatedcount, &fuzzycount, dia))
	{
		DiaDoneDialog(dia);
		return;
	}
	curse = us_transfirst(choose);
	us_transhowentry(curse, dia);

	/* run the dialog */
	dirty = FALSE;
	for(;;)
	{
		itemno = DiaNextHit(dia);
		if (itemno == DTRN_DONE)
		{
			if (dirty)
			{
				i = us_noyesdlog(_("Translations have changed.  Quit without saving?"), par);
				if (i == 0) break;
				if (namesame(par[0], x_("no")) == 0) continue;
			}
			break;
		}
		if (itemno == DTRN_VALIDATE)
		{
			us_transvalidate();
			continue;
		}
		if (itemno == DTRN_FINDENG || itemno == DTRN_FINDTRN)
		{
			DiaSetControl(dia, itemno, 1 - DiaGetControl(dia, itemno));
			continue;
		}
		if (itemno == DTRN_FINDBUT)
		{
			DiaSetPopupEntry(dia, DTRN_NEXTTYPE, CHOOSEFIND);
			itemno = DTRN_NEXTTYPE;
			/* fall into next test */
		}
		if (itemno == DTRN_NEXTTYPE)
		{
			choose = DiaGetPopupEntry(dia, DTRN_NEXTTYPE);
			if (choose == CHOOSEFIND)
			{
				/* do the search and mark strings that match */
				if (DiaGetControl(dia, DTRN_FINDENG) != 0) findenglish = TRUE; else
					findenglish = FALSE;
				if (DiaGetControl(dia, DTRN_FINDTRN) != 0) findtranslation = TRUE; else
					findtranslation = FALSE;
				pt = DiaGetText(dia, DTRN_FINDSTR);
				ptlen = estrlen(pt);
				foundcount = 0;
				for(se = us_transfirststringentry; se != NOSTRINGENTRY; se = se->nextstringentry)
				{
					se->flags &= ~MATCHES;
					if (findenglish && se->english != 0)
					{
						sept = se->english;
						selen = estrlen(sept);
						tests = selen - ptlen + 1;
						for(i=0; i<tests; i++)
							if (us_namesamennoamp(pt, sept++, ptlen) == 0) break;
						if (i < tests)
						{
							se->flags |= MATCHES;
							foundcount++;
							continue;
						}
					}
					if (findtranslation && se->translated != 0)
					{
						sept = se->translated;
						selen = estrlen(sept);
						tests = selen - ptlen + 1;
						for(i=0; i<tests; i++)
							if (namesamen(pt, sept++, ptlen) == 0) break;
						if (i < tests)
						{
							se->flags |= MATCHES;
							foundcount++;
						}
					}
				}
				esnprintf(num, 20, x_("%ld"), foundcount);
				DiaSetText(dia, DTRN_FOUNDSTR, num);
			} else DiaSetText(dia, DTRN_FOUNDSTR, x_(""));
			curse = us_transfirst(choose);
			us_transhowentry(curse, dia);
			continue;
		}
		if (itemno == DTRN_PLATFORM)
		{
			i = DiaGetPopupEntry(dia, DTRN_PLATFORM);
			if (i == platform) continue;
			if (dirty)
			{
				i = us_noyesdlog(_("Translations have changed.  Change languages without saving?"), par);
				if (i == 0) break;
				if (namesame(par[0], x_("no")) == 0) continue;
			}

			pt = us_transgetfiles(language, i);
			if (us_transloadmessages(pt, &total, &untranslatedcount, &fuzzycount, dia))
				continue;

			platform = i;
			curse = us_transfirst(choose);
			us_transhowentry(curse, dia);
			dirty = FALSE;
			continue;
		}
		if (itemno == DTRN_LANGUAGE)
		{
			i = DiaGetPopupEntry(dia, DTRN_LANGUAGE);
			if (estrcmp(langlist[i], language) == 0) continue;
			if (dirty)
			{
				i = us_noyesdlog(_("Translations have changed.  Change languages without saving?"), par);
				if (i == 0) break;
				if (namesame(par[0], x_("no")) == 0) continue;
			}

			pt = us_transgetfiles(langlist[i], platform);
			if (us_transloadmessages(pt, &total, &untranslatedcount, &fuzzycount, dia))
				continue;

			language = langlist[i];
			curse = us_transfirst(choose);
			us_transhowentry(curse, dia);
			dirty = FALSE;
			continue;
		}
		if (itemno == DTRN_SAVE)
		{
			us_transave(language, platform);
			dirty = FALSE;
			continue;
		}
		if (itemno == DTRN_MACTOUNIX)
		{
			if (dirty)
			{
				DiaMessageInDialog(_("Must save before translating files"));
				continue;
			}
			us_transmactounix(language);
			continue;
		}
		if (itemno == DTRN_UNIXTOMAC)
		{
			if (dirty)
			{
				DiaMessageInDialog(_("Must save before translating files"));
				continue;
			}
			DiaMessageInDialog(x_("Can't yet"));
			continue;
		}
		if (itemno == DTRN_NEXT)
		{
			if (curse == NOSTRINGENTRY) curse = us_transfirst(choose); else
			{
				for(se = curse->nextstringentry; se != NOSTRINGENTRY; se = se->nextstringentry)
				{
					if (us_transmeetscriteria(se, choose)) break;
				}
				curse = se;
			}
			us_transhowentry(curse, dia);
			continue;
		}
		if (itemno == DTRN_PREV)
		{
			if (curse == NOSTRINGENTRY) curse = us_translast(choose); else
			{
				for(se = curse->prevstringentry; se != NOSTRINGENTRY; se = se->prevstringentry)
				{
					if (us_transmeetscriteria(se, choose)) break;
				}
				curse = se;
			}
			us_transhowentry(curse, dia);
			continue;
		}
		if (itemno == DTRN_FIRST)
		{
			curse = us_transfirst(choose);
			us_transhowentry(curse, dia);
			continue;
		}
		if (itemno == DTRN_ISFUZZY)
		{
			if ((curse->flags&FUZZY) == 0) continue;
			i = 1 - DiaGetControl(dia, DTRN_ISFUZZY);
			DiaSetControl(dia, DTRN_ISFUZZY, i);
			if (i == 0)
			{
				curse->flags |= WASFUZZY;
				fuzzycount--;
			} else
			{
				curse->flags &= ~WASFUZZY;
				fuzzycount++;
			}
			esnprintf(num, 20, x_("%ld"), fuzzycount);
			DiaSetText(dia, DTRN_FUZSTR, num);
			dirty = TRUE;
			continue;
		}
		if (itemno == DTRN_FOREIGN)
		{
			if (curse == NOSTRINGENTRY) continue;
			pt = DiaGetText(dia, DTRN_FOREIGN);
			if (curse->translated == 0)
			{
				if (*pt == 0) continue;
			} else
			{
				if (estrcmp(pt, curse->translated) == 0) continue;
			}

			if (curse->translated == 0)
			{
				untranslatedcount--;
				esnprintf(num, 20, x_("%ld"), untranslatedcount);
				DiaSetText(dia, DTRN_UNTSTR, num);
			}
			if ((curse->flags & FUZZY) != 0)
			{
				fuzzycount--;
				esnprintf(num, 20, x_("%ld"), fuzzycount);
				DiaSetText(dia, DTRN_FUZSTR, num);
				DiaSetControl(dia, DTRN_ISFUZZY, 0);
				if ((curse->flags&FUZZY) != 0) curse->flags |= WASFUZZY;
			}
			if (curse->translated != 0) efree((CHAR *)curse->translated);
			(void)allocstring(&curse->translated, pt, us_tool->cluster);
			dirty = TRUE;
			continue;
		}
	}
	DiaDoneDialog(dia);
}

void us_transhowentry(STRINGENTRY *se, void *dia)
{
	CHAR line[100];
	REGISTER INTBIG whichone, i;
	BOOLEAN matching;
	REGISTER STRINGENTRY *ose;

	DiaLoadTextDialog(dia, DTRN_COMMENTS, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);
	DiaSetControl(dia, DTRN_ISFUZZY, 0);
	if (se == NOSTRINGENTRY)
	{
		DiaSetText(dia, DTRN_ENGLISH, x_(""));
		DiaSetText(dia, DTRN_FOREIGN, x_(""));
		DiaSetText(dia, DTRN_MSGID, _("No strings left"));
		return;
	}
	whichone = 0;
	if (DiaGetPopupEntry(dia, DTRN_NEXTTYPE) == CHOOSEFIND) matching = TRUE; else
		matching = FALSE;
	for(ose = us_transfirststringentry; ose != NOSTRINGENTRY; ose = ose->nextstringentry)
	{
		if (ose->english == 0) continue;
		if (matching)
		{
			if ((ose->flags&MATCHES) != 0) whichone++;
		} else
		{
			if ((se->flags&FUZZY) != 0)
			{
				if ((ose->flags&FUZZY) != 0) whichone++;
			} else
			{
				if (ose->translated == 0) whichone++;
			}
		}
		if (ose == se) break;
	}
	if (matching)
	{
		esnprintf(line, 100, _("String %ld (matched string %ld)"), se->index, whichone);
	} else
	{
		if ((se->flags&FUZZY) != 0)
		{
			if ((se->flags&WASFUZZY) != 0)
				esnprintf(line, 100, _("String %ld (was fuzzy)"), se->index); else
					esnprintf(line, 100, _("String %ld (fuzzy string %ld)"), se->index, whichone);
		} else if (se->translated == 0)
		{
			esnprintf(line, 100, _("String %ld (untranslated string %ld)"), se->index, whichone);
		} else if ((se->flags&THISTIME) != 0)
		{
			esnprintf(line, 100, _("String %ld (just translated)"), se->index);
		} else
		{
			esnprintf(line, 100, _("String %ld (already translated)"), se->index);
		}
	}
	DiaSetText(dia, DTRN_MSGID, line);
	if (se->english == 0) DiaSetText(dia, DTRN_ENGLISH, x_("")); else
		DiaSetText(dia, DTRN_ENGLISH, se->english);
	if (se->translated == 0) DiaSetText(dia, -DTRN_FOREIGN, x_("")); else
		DiaSetText(dia, -DTRN_FOREIGN, se->translated);
	if ((se->flags&(FUZZY|WASFUZZY)) == FUZZY) DiaSetControl(dia, DTRN_ISFUZZY, 1);
	for(i=0; i<se->headerlinecount; i++)
		DiaStuffLine(dia, DTRN_COMMENTS, se->headerlines[i]);
	DiaSelectLine(dia, DTRN_COMMENTS, -1);
}

STRINGENTRY *us_transfirst(INTBIG choose)
{
	REGISTER STRINGENTRY *se;

	for(se = us_transfirststringentry; se != NOSTRINGENTRY; se = se->nextstringentry)
	{
		if (us_transmeetscriteria(se, choose)) return(se);
	}
	return(NOSTRINGENTRY);
}

STRINGENTRY *us_translast(INTBIG choose)
{
	REGISTER STRINGENTRY *se, *lastse;

	for(se = us_transfirststringentry; se != NOSTRINGENTRY; se = se->nextstringentry)
		lastse = se;
	for(se = lastse; se != NOSTRINGENTRY; se = se->prevstringentry)
	{
		if (us_transmeetscriteria(se, choose)) return(se);
	}
	return(NOSTRINGENTRY);
}

BOOLEAN us_transmeetscriteria(STRINGENTRY *se, INTBIG choose)
{
	if (se->english == 0) return(FALSE);
	switch (choose)
	{
		case CHOOSEUNTRANS:
			if (se->translated == 0) return(TRUE);
			break;
		case CHOOSEJUSTTRANS:
			if (se->translated != 0 &&
				(se->flags&(FUZZY|WASFUZZY|THISTIME)) == THISTIME) return(TRUE);
			break;
		case CHOOSEFUZZY:
			if ((se->flags&(FUZZY|WASFUZZY)) == FUZZY) return(TRUE);
			break;
		case CHOOSENLFUZZY:
			if ((se->flags&WASFUZZY) != 0) return(TRUE);
			break;
		case CHOOSEFIND:
			if ((se->flags&MATCHES) != 0) return(TRUE);
			break;
		case CHOOSEALL:
			return(TRUE);
	}
	return(FALSE);
}

BOOLEAN us_transloadmessages(CHAR *file, INTBIG *entrycount, INTBIG *untranslatedcount,
	INTBIG *fuzzycount, void *dia)
{
	REGISTER FILE *io;
	REGISTER INTBIG i, j, eof, inenglish, endchr, haveline, index, lineno;
	REGISTER CHAR *pt, *ptr;
	CHAR line[500], build[700], *truename;
	REGISTER STRINGENTRY *se, *lastse;

	io = xopen(file, us_filetypetrans, 0, &truename);
	if (io == 0)
	{
		ttyputmsg(_("Cannot find: %s"), file);
		return(TRUE);
	}
	lineno = 0;

	/* gather the header lines */
	us_transgatherlinecount = 0;
	for(;;)
	{
		if (xfgets(line, 500, io)) break;
		lineno++;
		if (line[0] == 0) break;
		if (us_transgathermessage(line)) return(TRUE);
	}
	us_transheaderlinecount = us_transgatherlinecount;
	us_transheaderlines = (CHAR **)emalloc(us_transheaderlinecount * (sizeof (CHAR *)), us_tool->cluster);
	if (us_transheaderlines == 0) return(TRUE);
	for(i=0; i<us_transheaderlinecount; i++)
	{
		us_transheaderlines[i] = us_transgatherlines[i];
		us_transgatherlines[i] = 0;
	}

	/* now gather the rest */
	lastse = NOSTRINGENTRY;
	us_transfirststringentry = NOSTRINGENTRY;
	index = 1;
	eof = 0;
	for(;;)
	{
		/* gather comment lines */
		us_transgatherlinecount = 0;
		for(;;)
		{
			if (xfgets(line, 500, io)) { eof = 1;   break; }
			lineno++;
			if (line[0] != '#') break;
			if (us_transgathermessage(line)) return(TRUE);
		}
		if (us_transgatherlinecount == 0 && eof != 0) break;

		/* create a new translation block */
		se = (STRINGENTRY *)emalloc(sizeof (STRINGENTRY), us_tool->cluster);
		if (se == 0) return(TRUE);

		/* look for flags in comment lines */
		se->flags = 0;
		for(i=0; i<us_transgatherlinecount; i++)
		{
			if (us_transgatherlines[i][0] == '#' && us_transgatherlines[i][1] == ',')
			{
				pt = &us_transgatherlines[i][1];
				while (*pt != 0)
				{
					if (estrncmp(pt, x_(", c-format"), 10) == 0)
					{
						se->flags |= CFORMAT;
						pt += 10;
						continue;
					}
					if (estrncmp(pt, x_(", fuzzy"), 7) == 0)
					{
						se->flags |= FUZZY;
						pt += 7;
						continue;
					}
					ttyputmsg(_("Unknown flags on line %ld: '%s'"), lineno, us_transgatherlines[i]);
					break;
				}
				for(j=i; j<us_transgatherlinecount-1; j++)
					us_transgatherlines[j] = us_transgatherlines[j+1];
				us_transgatherlinecount--;
				i--;
			}
		}

		/* store the comment lines */
		se->headerlinecount = us_transgatherlinecount;
		se->headerlines = (CHAR **)emalloc(se->headerlinecount * (sizeof (CHAR *)), us_tool->cluster);
		if (se->headerlines == 0) return(TRUE);
		for(i=0; i<us_transgatherlinecount; i++)
		{
			se->headerlines[i] = us_transgatherlines[i];
			us_transgatherlines[i] = 0;
		}

		/* link it in */
		if (lastse == NOSTRINGENTRY) us_transfirststringentry = se; else
			lastse->nextstringentry = se;
		se->nextstringentry = NOSTRINGENTRY;
		se->prevstringentry = lastse;
		lastse = se;

		/* other initialization */
		se->index = index++;
		se->english = 0;
		se->translated = 0;

		/* get the strings */
		if (eof != 0) break;
		inenglish = 1;
		haveline = 1;
		for(;;)
		{
			if (haveline == 0)
			{
				if (xfgets(line, 500, io)) { eof = 1;   break; }
				lineno++;
			} else haveline = 0;
			if (line[0] == 0) break;
			pt = line;
			if (estrncmp(pt, x_("msgid "), 6) == 0)
			{
				inenglish = 1;
				pt += 6;
			} else if (estrncmp(pt, x_("msgstr "), 7) == 0)
			{
				inenglish = 0;
				pt += 7;
			}
			if (*pt == '"') pt++;
			endchr = estrlen(pt) - 1;
			if (pt[endchr] == '"') pt[endchr] = 0;
			if (*pt == 0) continue;

			/* remove quoted quotes */
			for(ptr = pt; *ptr != 0; ptr++)
			{
				if (*ptr != '\\') continue;
				if (ptr[1] != '"') continue;
				estrcpy(ptr, &ptr[1]);
			}

			if (inenglish != 0)
			{
				build[0] = 0;
				if (se->english != 0)
				{
					estrcpy(build, se->english);
					efree((CHAR *)se->english);
				}
				estrcat(build, pt);
				(void)allocstring(&se->english, build, us_tool->cluster);
			} else
			{
				build[0] = 0;
				if (se->translated != 0)
				{
					estrcpy(build, se->translated);
					efree((CHAR *)se->translated);
				}
				estrcat(build, pt);
				(void)allocstring(&se->translated, build, us_tool->cluster);
			}
		}
	}
	xclose(io);

	/* report information about this translation */
	*entrycount = *untranslatedcount = *fuzzycount = 0;
	for(se = us_transfirststringentry; se != NOSTRINGENTRY; se = se->nextstringentry)
	{
		if (se->english == 0) continue;
		(*entrycount)++;
		if (se->translated == 0)
		{
			(*untranslatedcount)++;
			se->flags |= THISTIME;
		} else if ((se->flags&FUZZY) != 0)
		{
			(*fuzzycount)++;
			se->flags |= THISTIME;
		}
	}
	esnprintf(line, 500, x_("%ld"), *entrycount);
	DiaSetText(dia, DTRN_TOTSTR, line);
	esnprintf(line, 500, x_("%ld"), *untranslatedcount);
	DiaSetText(dia, DTRN_UNTSTR, line);
	esnprintf(line, 500, x_("%ld"), *fuzzycount);
	DiaSetText(dia, DTRN_FUZSTR, line);
	return(FALSE);
}

BOOLEAN us_transgathermessage(CHAR *line)
{
	REGISTER INTBIG newtotal, i;
	REGISTER CHAR **newlines;

	if (us_transgatherlinecount >= us_transgatherlinetotal)
	{
		newtotal = us_transgatherlinetotal * 2;
		if (us_transgatherlinecount >= newtotal)
			newtotal = us_transgatherlinecount + 30;
		newlines = (CHAR **)emalloc(newtotal * (sizeof (CHAR *)), us_tool->cluster);
		if (newlines == 0) return(TRUE);
		for(i=0; i<us_transgatherlinecount; i++)
			newlines[i] = us_transgatherlines[i];
		for(i=us_transgatherlinecount; i<newtotal; i++)
			newlines[i] = 0;
		if (us_transgatherlinetotal > 0)
			efree((CHAR *)us_transgatherlines);
		us_transgatherlines = newlines;
		us_transgatherlinetotal = newtotal;
	}
	if (us_transgatherlines[us_transgatherlinecount] != 0)
		efree((CHAR *)us_transgatherlines[us_transgatherlinecount]);
	if (allocstring(&us_transgatherlines[us_transgatherlinecount], line,
		us_tool->cluster)) return(TRUE);
	us_transgatherlinecount++;
	return(FALSE);
}

void us_transave(CHAR *language, INTBIG platform)
{
	REGISTER STRINGENTRY *se;
	REGISTER INTBIG i;
	INTBIG year, month, mday, hour, minute, second;
	REGISTER time_t olddate;
	CHAR filename[300], rename[300], datesuffix[100], *truename;
	FILE *io;
	REGISTER void *infstr;

	/* rename the old file */
	estrcpy(filename, us_transgetfiles(language, platform));
	olddate = filedate(filename);
	parsetime(olddate, &year, &month, &mday, &hour, &minute, &second);
	for(i=0; i<1000; i++)
	{
		if (i == 0) esnprintf(datesuffix, 100, x_("-%ld-%ld-%ld"), year, month+1, mday); else
			esnprintf(datesuffix, 100, x_("-%ld-%ld-%ld--%ld"), year, month+1, mday, i);
		(void)estrcpy(rename, filename);
		(void)estrcat(rename, datesuffix);
		if (fileexistence(rename) == 0) break;
	}
	if (erename(filename, rename) != 0)
		ttyputerr(_("Could not rename file '%s' to '%s'"), filename, rename);

	/* create the new file */
	io = xcreate(filename, us_filetypetrans, 0, &truename);
	if (io == 0)
	{
		ttyputerr(_("Cannot write %s"), filename);
		return;
	}

	/* write the header lines */
	for(i=0; i<us_transheaderlinecount; i++)
		efprintf(io, x_("%s\n"), us_transheaderlines[i]);

	/* write the translations */
	for(se = us_transfirststringentry; se != NOSTRINGENTRY; se = se->nextstringentry)
	{
		efprintf(io, x_("\n"));
		for(i=0; i<se->headerlinecount; i++)
			efprintf(io, x_("%s\n"), se->headerlines[i]);
		if ((se->flags&(FUZZY|WASFUZZY)) == FUZZY ||
			(se->flags&CFORMAT) != 0)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("#"));
			if ((se->flags&(FUZZY|WASFUZZY)) == FUZZY) addstringtoinfstr(infstr, x_(", fuzzy"));
			if ((se->flags&CFORMAT) != 0) addstringtoinfstr(infstr, x_(", c-format"));
			efprintf(io, x_("%s\n"), returninfstr(infstr));
		}
		if (se->english != 0)
			us_transdumpline(io, us_transquoteit(se->english), x_("msgid"));
		if (se->translated != 0)
			us_transdumpline(io, us_transquoteit(se->translated), x_("msgstr"));
	}
	xclose(io);
}

#define MAXLEN 80

void us_transdumpline(FILE *io, CHAR *line, CHAR *prefix)
{
	REGISTER INTBIG len, preflen, i;
	CHAR save;

	preflen = estrlen(prefix) + 3;
	len = estrlen(line);
	if (len+preflen <= MAXLEN)
	{
		efprintf(io, x_("%s \"%s\"\n"), prefix, line);
		return;
	}
	efprintf(io, x_("%s \"\"\n"), prefix);
	while (len > MAXLEN-4)
	{
		i = MAXLEN-4;
		while (i > 0 && line[i] != ' ') i--;
		if (i == 0) i = MAXLEN-4;
		i++;
		save = line[i];
		line[i] = 0;
		efprintf(io, x_("\"%s\"\n"), line);
		line[i] = save;
		line = &line[i];
		len = estrlen(line);
	}
	if (len > 0)
		efprintf(io, x_("\"%s\"\n"), line);
}

/*
 * Routine to do a string comparison between "pt1" and "pt2" up to "count" characters.
 * Ignores "&" characters.
 */
INTBIG us_namesamennoamp(CHAR *pt1, CHAR *pt2, INTBIG count)
{
	REGISTER INTBIG c1, c2, i;

	for(i=0; i<count; i++)
	{
		while (*pt1 == '&') pt1++;
		while (*pt2 == '&') pt2++;
		c1 = tolower(*pt1++ & 0377);
		c2 = tolower(*pt2++ & 0377);
		if (c1 != c2) return(c1-c2);
		if (c1 == 0) break;
	}
	return(0);
}

CHAR *us_transquoteit(CHAR *message)
{
	REGISTER CHAR *pt;
	REGISTER INTBIG j;
	static CHAR result[500];

	j = 0;
	for(pt = message; *pt != 0; pt++)
	{
		if (*pt == '"') result[j++] = '\\';
		result[j++] = *pt;
	}
	result[j] = 0;
	return(result);
}

CHAR *us_transgetfiles(CHAR *language, INTBIG platform)
{
	REGISTER CHAR *onmac;
	REGISTER void *infstr;

	if (platform == 0) onmac = x_("mac"); else
		onmac = x_("");
	infstr = initinfstr();
	addstringtoinfstr(infstr, el_libdir);
	formatinfstr(infstr, x_("international%s%s%sLC_MESSAGES%s%selectric.%s"),
		DIRSEPSTR, language, DIRSEPSTR, DIRSEPSTR, onmac, language);
	return(returninfstr(infstr));
}

void us_transvalidate(void)
{
	REGISTER STRINGENTRY *se;
	REGISTER INTBIG i;
	REGISTER CHAR *pt;

	for(se = us_transfirststringentry; se != NOSTRINGENTRY; se = se->nextstringentry)
	{
		if (se->translated == 0) continue;

		/* see if the use of elipses at the end matches */
		i = estrlen(se->translated);
		if (i > 3 && estrcmp(&se->translated[i-3], x_("...")) == 0)
		{
			/* translation has ellipses: make sure original does, too */
			i = estrlen(se->english);
			if (i < 3 || estrcmp(&se->english[i-3], x_("...")) != 0)
			{
				ttyputmsg(x_("'%s' should not have '...' in translation"), se->english);
			}
		} else
		{
			/* translation does not have ellipses: make sure original doesn't either */
			i = estrlen(se->english);
			if (i > 3 && estrcmp(&se->english[i-3], x_("...")) == 0)
			{
				ttyputmsg(x_("'%s' must have '...' in translation"), se->english);
			}
		}

		/* see if translation includes "&" erroneously */
		for(pt = se->translated; *pt != 0; pt++)
			if (*pt == '&') break;
		if (*pt != 0)
		{
			/* see if this comes from the "lib" directory */
			for(i=0; i<se->headerlinecount; i++)
			{
				pt = se->headerlines[i];
				while (*pt == '#' || *pt == ' ') pt++;
				if (estrncmp(pt, x_("lib"), 3) == 0) break;
			}
			if (i < se->headerlinecount)
			{
				/* this should not have "&" */
				ttyputmsg(x_("'%s' should not have '&' in translation"), se->english);
			}
		}
	}
}

void us_transmactounix(CHAR *language)
{
	REGISTER CHAR *macfile, *unixfile;
	CHAR buf[1000], *dummy;
	REGISTER INTBIG lineno;
	FILE *in, *out;

	macfile = us_transgetfiles(language, 0);
	in = xopen(macfile, el_filetypetext, 0, &dummy);
	if (in == 0)
	{
		DiaMessageInDialog(x_("Cannot read %s"), macfile);
		return;
	}
	unixfile = us_transgetfiles(language, 1);
	out = xcreate(unixfile, el_filetypetext, 0, 0);
	if (out == 0)
	{
		DiaMessageInDialog(x_("Cannot write %s"), unixfile);
		return;
	}
	lineno = 0;
	for(;;)
	{
		if (xfgets(buf, 999, in)) break;
		lineno++;
		buf[estrlen(buf)] = 0;
		if (estrncmp(buf, x_("msgstr \""), 8) != 0)
		{
			efprintf(out, x_("%s\n"), buf);
			continue;
		}

		/* found "msgstr", look for strange characters */
		estrcpy(&buf[7], us_convertmactoworld(&buf[7]));
		efprintf(out, x_("%s\n"), buf);
		for(;;)
		{
			if (xfgets(buf, 999, in)) break;
			lineno++;
			buf[estrlen(buf)] = 0;
			if (buf[0] == 0)
			{
				efprintf(out, x_("\n"));
				break;
			}
			estrcpy(buf, us_convertmactoworld(buf));
			efprintf(out, x_("%s\n"), buf);
		}
	}
	fclose(in);
	fclose(out);
}

/*
 *		WINDOWS		MACINTOSH			WINDOWS		MACINTOSH
 * à	0xE0		0x88			À	0xC0		0xCB
 * á	0xE1		0x87			Á	0xC1		none
 * â	0xE2		0x89			Â	0xC2		none
 * ã	0xE3		0x8B			Ã	0xC3		0xCC
 * ä	0xE4		0x8A			Ä	0xC4		0x80
 * å	0xE5		0x8C			Å	0xC5		0x81
 *
 * è	0xE8		0x8F			È	0xC8		none
 * é	0xE9		0x8E			É	0xC9		0x83
 * ê	0xEA		0x90			Ê	0xCA		none
 * ë	0xEB		0x91			Ë	0xCB		none
 *
 * ì	0xEC		0x93			Ì	0xCC		none
 * í	0xED		0x92			Í	0xCD		none
 * î	0xEE		0x94			Î	0xCE		none
 * ï	0xEF		0x95			Ï	0xCF		none
 *
 * ò	0xF2		0x98			Ò	0xD2		none
 * ó	0xF3		0x97			Ó	0xD3		none
 * ô	0xF4		0x99			Ô	0xD4		none
 * õ	0xF5		0x9B			Õ	0xD5		0xCD
 * ö	0xF6		0x9A			Ö	0xD6		0x85
 *
 * ù	0xF9		0x9D			Ù	0xD9		none
 * ú	0xFA		0x9C			Ú	0xDA		none
 * û	0xFB		0x9E			Û	0xDB		none
 * ü	0xFC		0x9F			Ü	0xDC		0x86
 *
 * ç	0xE7		0x8D			Ç	0xC7		0x82
 * æ	0xE6		0xBE			Æ	0xC6		0xAE
 * ñ	0xF1		0x96			Ñ	0xD1		0x84
 * oe	none		0xCF			OE	none		0xCE
 */
typedef struct
{
	int origchar;
	int newchar;
	int secondchar;
} TRANSLATE;

TRANSLATE us_trans_mactoworld[] =
{
	{0x80, 0xC4, 0},	/* "Ä" */
	{0x81, 0xC5, 0},	/* "Å" */
	{0x82, 0xC7, 0},	/* "Ç" */
	{0x83, 0xC9, 0},	/* "É" */
	{0x84, 0xD1, 0},	/* "Ñ" */
	{0x85, 0xD6, 0},	/* "Ö" */
	{0x86, 0xDC, 0},	/* "Ü" */
	{0x87, 0xE1, 0},	/* "á" */
	{0x88, 0xE0, 0},	/* "à" */
	{0x89, 0xE2, 0},	/* "â" */
	{0x8A, 0xE4, 0},	/* "ä" */
	{0x8B, 0xE3, 0},	/* "ã" */
	{0x8C, 0xE5, 0},	/* "å" */
	{0x8D, 0xE7, 0},	/* "ç" */
	{0x8E, 0xE9, 0},	/* "é" */
	{0x8F, 0xE8, 0},	/* "è" */
	{0x90, 0xEA, 0},	/* "ê" */
	{0x91, 0xEB, 0},	/* "ë" */
	{0x92, 0xED, 0},	/* "í" */
	{0x93, 0xEC, 0},	/* "ì" */
	{0x94, 0xEE, 0},	/* "î" */
	{0x95, 0xEF, 0},	/* "ï" */
	{0x96, 0xF1, 0},	/* "ñ" */
	{0x97, 0xF3, 0},	/* "ó" */
	{0x98, 0xF2, 0},	/* "ò" */
	{0x99, 0xF4, 0},	/* "ô" */
	{0x9A, 0xF6, 0},	/* "ö" */
	{0x9B, 0xF5, 0},	/* "õ" */
	{0x9C, 0xFA, 0},	/* "ú" */
	{0x9D, 0xF9, 0},	/* "ù" */
	{0x9E, 0xFB, 0},	/* "û" */
	{0x9F, 0xFC, 0},	/* "ü" */

	{0xAE, 0xC6, 0},	/* "Æ" */
	{0xBE, 0xE6, 0},	/* "æ" */
	{0xCB, 0xC0, 0},	/* "À" */
	{0xCC, 0xC3, 0},	/* "Ã" */
	{0xCD, 0xD5, 0},	/* "Õ" */
	{0xCE, 'O', 'E'},	/* "OE" */
	{0xCF, 'o', 'e'},	/* "oe" */
	{0,0,0}
};

/*
 * Routine to convert the string "buf" (which is has international characters
 * encoded Macintosh-sytle) and return the buffer in international characters
 * encoded Windows/UNIX style.
 */
CHAR *us_convertmactoworld(CHAR *buf)
{
	CHAR *pt;
	int i, len, thechar, outchar;

	len = estrlen(buf) * 2;
	if (len > us_translationsize)
	{
		if (us_translationsize > 0) efree((CHAR *)us_translation);
		us_translation = (CHAR *)emalloc(len * SIZEOFCHAR, us_tool->cluster);
		if (us_translation == 0) return(buf);
		us_translationsize = len;
	}
	outchar = 0;
	for(pt = buf; *pt != 0; pt++)
	{
		thechar = *pt & 0xFF;
		if (isalnum(thechar) != 0 ||
			thechar == '"' || thechar == '!' || thechar == '@' ||
			thechar == '#' || thechar == '$' || thechar == '%' ||
			thechar == '&' || thechar == '*' || thechar == '(' ||
			thechar == ')' || thechar == '-' || thechar == '_' ||
			thechar == '=' || thechar == '+' || thechar == '[' ||
			thechar == ']' || thechar == '{' || thechar == '}' ||
			thechar == '\\' || thechar == '|' || thechar == ':' ||
			thechar == ';' || thechar == ',' || thechar == '.' ||
			thechar == '<' || thechar == '>' || thechar == '?' ||
			thechar == '/' || thechar == '~' || thechar == '\'' ||
			thechar == '\n' || thechar == ' ' || thechar == '^')
		{
			us_translation[outchar++] = *pt;
			continue;
		}

		/* see if it is in the table */
		for(i=0; us_trans_mactoworld[i].origchar != 0; i++)
			if (thechar == us_trans_mactoworld[i].origchar) break;
		if (us_trans_mactoworld[i].origchar != 0)
		{
			us_translation[outchar++] = us_trans_mactoworld[i].newchar;
			if (us_trans_mactoworld[i].secondchar != 0)
				us_translation[outchar++] = us_trans_mactoworld[i].secondchar;
			continue;
		}

		/* foreign character found */
		us_translation[outchar++] = *pt;
	}
	us_translation[outchar] = 0;
	return(us_translation);
}

CHAR *us_convertworldtomac(CHAR *buf)
{
	CHAR *pt;
	int i, len, thechar, outchar;

	len = estrlen(buf);
	if (len > us_translationsize)
	{
		if (us_translationsize > 0) efree((CHAR *)us_translation);
		us_translation = (CHAR *)emalloc(len * SIZEOFCHAR, us_tool->cluster);
		if (us_translation == 0) return(buf);
		us_translationsize = len;
	}
	outchar = 0;
	for(pt = buf; *pt != 0; pt++)
	{
		thechar = *pt & 0xFF;
		if (isalnum(thechar) != 0 ||
			thechar == '"' || thechar == '!' || thechar == '@' ||
			thechar == '#' || thechar == '$' || thechar == '%' ||
			thechar == '&' || thechar == '*' || thechar == '(' ||
			thechar == ')' || thechar == '-' || thechar == '_' ||
			thechar == '=' || thechar == '+' || thechar == '[' ||
			thechar == ']' || thechar == '{' || thechar == '}' ||
			thechar == '\\' || thechar == '|' || thechar == ':' ||
			thechar == ';' || thechar == ',' || thechar == '.' ||
			thechar == '<' || thechar == '>' || thechar == '?' ||
			thechar == '/' || thechar == '~' || thechar == '\'' ||
			thechar == '\n' || thechar == ' ' || thechar == '^')
		{
			us_translation[outchar++] = *pt;
			continue;
		}

		/* see if it is in the table */
		for(i=0; us_trans_mactoworld[i].newchar != 0; i++)
			if (thechar == us_trans_mactoworld[i].newchar &&
				us_trans_mactoworld[i].secondchar == 0) break;
		if (us_trans_mactoworld[i].newchar != 0)
		{
			us_translation[outchar++] = us_trans_mactoworld[i].origchar;
			continue;
		}

		/* foreign character found */
		us_translation[outchar++] = *pt;
	}
	us_translation[outchar] = 0;
	return(us_translation);
}
