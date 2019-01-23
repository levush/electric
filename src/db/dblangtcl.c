/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dblangtcl.c
 * TCL interface module
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

#include "config.h"
#if LANGTCL

#include "global.h"
#include "dblang.h"
#include "database.h"

#define BUFSIZE 500

       CHAR       *tcl_outputbuffer, *tcl_outputloc;
static CHAR       *tcl_inputbuffer;
       Tcl_Interp *tcl_interp = 0;

/* prototypes for local routines */
static void   tcl_dumpoutput(CHAR *str);
static void   tcl_converttoanyelectric(CHAR *tclstr, INTBIG *addr, INTBIG *type);
static void   tcl_converttoscalartcl(void *infstr, INTBIG addr, INTBIG type);
static CHAR  *tcl_converttotcl(INTBIG addr, INTBIG type);
static INTBIG tcl_converttowarnedelectric(CHAR *name, INTBIG want, CHAR *msg);
static int    tcl_tkconsoleclose(ClientData instanceData, Tcl_Interp *interp);
static int    tcl_tkconsoleinput(ClientData instanceData, char *buf, int bufSize, int *errorCode);
static int    tcl_tkconsoleoutput(ClientData instanceData, char *buf, int toWrite, int *errorCode);
static void   tcl_tkconsolewatch(ClientData instanceData, int mask);

static int tcl_curlib(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_curtech(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_getval(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_getparentval(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_P(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_PD(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_PAR(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_PARD(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_setval(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_setind(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_delval(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_initsearch(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_nextobject(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_gettool(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_maxtool(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_indextool(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_toolturnon(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_toolturnoff(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_getlibrary(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_newlibrary(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_killlibrary(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_eraselibrary(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_selectlibrary(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_getnodeproto(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_newnodeproto(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_killnodeproto(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_copynodeproto(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_iconview(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_contentsview(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_newnodeinst(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_modifynodeinst(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_killnodeinst(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_replacenodeinst(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_nodefunction(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_newarcinst(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_modifyarcinst(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_killarcinst(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_replacearcinst(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_newportproto(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_portposition(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_getportproto(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_killportproto(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_moveportproto(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_undoabatch(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_noundoallowed(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_getview(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_newview(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_killview(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_telltool(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_asktool(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_getarcproto(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_gettechnology(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_getpinproto(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);
static int tcl_getnetwork(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv);

/************************* TCL: ELECTRIC ROUTINES *************************/
/*
 * Routine to free all memory associated with this module.
 */
void db_freetclmemory(void)
{
	efree((CHAR *)tcl_outputbuffer);
	efree((CHAR *)tcl_inputbuffer);
}

void el_tclinterpreter(Tcl_Interp *interp)
{
	Tcl_Channel consoleChannel;
	CHAR *path;
	static Tcl_ChannelType consoleChannelType =
	{
		"console",				/* Type name. */
		NULL,					/* Always non-blocking.*/
		tcl_tkconsoleclose,		/* Close proc. */
		tcl_tkconsoleinput,		/* Input proc. */
		tcl_tkconsoleoutput,	/* Output proc. */
		NULL,					/* Seek proc. */
		NULL,					/* Set option proc. */
		NULL,					/* Get option proc. */
		tcl_tkconsolewatch,		/* Watch for events on console. */
		NULL,					/* Are events present? */
	};
	REGISTER void *infstr;

	/* remember the interpreter */
	tcl_interp = interp;

	/* allocate terminal I/O buffers */
	tcl_outputbuffer = (CHAR *)emalloc(BUFSIZE * SIZEOFCHAR, db_cluster);
	tcl_inputbuffer = (CHAR *)emalloc(BUFSIZE * SIZEOFCHAR, db_cluster);
	if (tcl_outputbuffer == 0 || tcl_inputbuffer == 0) return;
	tcl_outputloc = tcl_outputbuffer;

	/* set console channels */
	consoleChannel = Tcl_CreateChannel(&consoleChannelType, x_("console0"), (ClientData)TCL_STDIN, TCL_READABLE);
	if (consoleChannel != NULL)
	{
		Tcl_SetChannelOption(NULL, consoleChannel, x_("-translation"), x_("lf"));
		Tcl_SetChannelOption(NULL, consoleChannel, x_("-buffering"), x_("none"));
	}
	Tcl_SetStdChannel(consoleChannel, TCL_STDIN);
	consoleChannel = Tcl_CreateChannel(&consoleChannelType, x_("console1"), (ClientData)TCL_STDOUT, TCL_WRITABLE);
	if (consoleChannel != NULL)
	{
		Tcl_SetChannelOption(NULL, consoleChannel, x_("-translation"), x_("lf"));
		Tcl_SetChannelOption(NULL, consoleChannel, x_("-buffering"), x_("none"));
	}
	Tcl_SetStdChannel(consoleChannel, TCL_STDOUT);
	consoleChannel = Tcl_CreateChannel(&consoleChannelType, x_("console2"), (ClientData)TCL_STDERR, TCL_WRITABLE);
	if (consoleChannel != NULL)
	{
		Tcl_SetChannelOption(NULL, consoleChannel, x_("-translation"), x_("lf"));
		Tcl_SetChannelOption(NULL, consoleChannel, x_("-buffering"), x_("none"));
	}
	Tcl_SetStdChannel(consoleChannel, TCL_STDERR);

	/* set the system version */
	if (Tcl_SetVar(tcl_interp, x_("el_version"), el_version, TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL)
	{
		ttyputerr(_("Tcl_SetVar failed: %s"), tcl_interp->result);
		ttyputmsg(_("couldn't set el_version variable"));
	}

	/* set the Electric library directory */
	if (Tcl_SetVar(tcl_interp, x_("el_libdir"), el_libdir, TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL)
	{
		ttyputerr(_("Tcl_SetVar failed: %s"), tcl_interp->result);
		ttyputmsg(_("couldn't set el_libdir variable"));
	}

	/* set the Electric LISP directory */
	infstr = initinfstr();
	addstringtoinfstr(infstr, el_libdir);
	addstringtoinfstr(infstr, x_("lisp"));
	addtoinfstr(infstr, DIRSEP);
	if (Tcl_SetVar(tcl_interp, x_("el_lispdir"), returninfstr(infstr), TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL)
	{
		ttyputerr(_("Tcl_SetVar failed: %s"), tcl_interp->result);
		ttyputmsg(_("couldn't set el_lispdir variable"));
	}

	/* set the Electric Tcl directory for extra TCL code */
	infstr = initinfstr();
	addstringtoinfstr(infstr, el_libdir);
	addstringtoinfstr(infstr, x_("tcl"));
	addtoinfstr(infstr, DIRSEP);
	path = returninfstr(infstr);
	if (Tcl_SetVar(tcl_interp, x_("el_tcldir"), path, TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL)
	{
		ttyputerr(_("Tcl_SetVar failed: %s"), tcl_interp->result);
		ttyputmsg(_("couldn't set el_tcldir variable"));
	}

	/* create the Electric commands */
	Tcl_CreateCommand(tcl_interp, x_("curlib"),          tcl_curlib,          (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("curtech"),         tcl_curtech,         (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("getval"),          tcl_getval,          (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("getparentval"),    tcl_getparentval,    (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("P"),               tcl_P,               (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("PD"),              tcl_PD,              (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("PAR"),             tcl_PAR,             (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("PARD"),            tcl_PARD,            (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("setval"),          tcl_setval,          (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("setind"),          tcl_setind,          (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("delval"),          tcl_delval,          (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("initsearch"),      tcl_initsearch,      (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("nextobject"),      tcl_nextobject,      (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("gettool"),         tcl_gettool,         (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("maxtool"),         tcl_maxtool,         (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("indextool"),       tcl_indextool,       (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("toolturnon"),      tcl_toolturnon,      (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("toolturnoff"),     tcl_toolturnoff,     (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("getlibrary"),      tcl_getlibrary,      (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("newlibrary"),      tcl_newlibrary,      (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("killlibrary"),     tcl_killlibrary,     (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("eraselibrary"),    tcl_eraselibrary,    (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("selectlibrary"),   tcl_selectlibrary,   (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("getnodeproto"),    tcl_getnodeproto,    (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("newnodeproto"),    tcl_newnodeproto,    (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("killnodeproto"),   tcl_killnodeproto,   (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("copynodeproto"),   tcl_copynodeproto,   (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("iconview"),        tcl_iconview,        (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("contentsview"),    tcl_contentsview,    (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("newnodeinst"),     tcl_newnodeinst,     (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("modifynodeinst"),  tcl_modifynodeinst,  (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("killnodeinst"),    tcl_killnodeinst,    (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("replacenodeinst"), tcl_replacenodeinst, (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("nodefunction"),    tcl_nodefunction,    (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("newarcinst"),      tcl_newarcinst,      (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("modifyarcinst"),   tcl_modifyarcinst,   (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("killarcinst"),     tcl_killarcinst,     (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("replacearcinst"),  tcl_replacearcinst,  (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("newportproto"),    tcl_newportproto,    (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("portposition"),    tcl_portposition,    (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("getportproto"),    tcl_getportproto,    (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("killportproto"),   tcl_killportproto,   (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("moveportproto"),   tcl_moveportproto,   (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("undoabatch"),      tcl_undoabatch,      (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("noundoallowed"),   tcl_noundoallowed,   (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("getview"),         tcl_getview,         (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("newview"),         tcl_newview,         (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("killview"),        tcl_killview,        (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("telltool"),        tcl_telltool,        (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("asktool"),         tcl_asktool,         (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("getarcproto"),     tcl_getarcproto,     (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("gettechnology"),   tcl_gettechnology,   (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("getpinproto"),     tcl_getpinproto,     (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(tcl_interp, x_("getnetwork"),      tcl_getnetwork,      (ClientData)0, (Tcl_CmdDeleteProc *)NULL);
}

void tcl_converttoanyelectric(CHAR *tclstr, INTBIG *addr, INTBIG *type)
{
	REGISTER CHAR *pt;
	REGISTER float fval;
	REGISTER void *infstr;

	/* simple if it is a number */
	if (isanumber(tclstr))
	{
		for(pt = tclstr; *pt != 0; pt++)
			if (*pt == '.')
		{
			/* with a decimal point, use floating point representation */
			fval = (float)eatof(tclstr);
			*addr = castint(fval);
			*type = VFLOAT;
			return;
		}

		/* just an integer */
		*addr = myatoi(tclstr);
		*type = VINTEGER;
		return;
	}

	/* check for special Electric object names */
	if (namesamen(tclstr, x_("#nodeinst"),     9) == 0) { *addr = myatoi(&tclstr[9]);  *type = VNODEINST;    return; }
	if (namesamen(tclstr, x_("#nodeproto"),   10) == 0) { *addr = myatoi(&tclstr[10]); *type = VNODEPROTO;   return; }
	if (namesamen(tclstr, x_("#portarcinst"), 12) == 0) { *addr = myatoi(&tclstr[12]); *type = VPORTARCINST; return; }
	if (namesamen(tclstr, x_("#portexpinst"), 12) == 0) { *addr = myatoi(&tclstr[12]); *type = VPORTEXPINST; return; }
	if (namesamen(tclstr, x_("#portproto"),   10) == 0) { *addr = myatoi(&tclstr[10]); *type = VPORTPROTO;   return; }
	if (namesamen(tclstr, x_("#arcinst"),      8) == 0) { *addr = myatoi(&tclstr[8]);  *type = VARCINST;     return; }
	if (namesamen(tclstr, x_("#arcproto"),     9) == 0) { *addr = myatoi(&tclstr[9]);  *type = VARCPROTO;    return; }
	if (namesamen(tclstr, x_("#geom"),         5) == 0) { *addr = myatoi(&tclstr[5]);  *type = VGEOM;        return; }
	if (namesamen(tclstr, x_("#library"),      8) == 0) { *addr = myatoi(&tclstr[8]);  *type = VLIBRARY;     return; }
	if (namesamen(tclstr, x_("#technology"),  11) == 0) { *addr = myatoi(&tclstr[11]); *type = VTECHNOLOGY;  return; }
	if (namesamen(tclstr, x_("#tool"),         5) == 0) { *addr = myatoi(&tclstr[5]);  *type = VTOOL;        return; }
	if (namesamen(tclstr, x_("#rtnode"),       7) == 0) { *addr = myatoi(&tclstr[7]);  *type = VRTNODE;      return; }
	if (namesamen(tclstr, x_("#network"),      8) == 0) { *addr = myatoi(&tclstr[8]);  *type = VNETWORK;     return; }
	if (namesamen(tclstr, x_("#view"),         5) == 0) { *addr = myatoi(&tclstr[5]);  *type = VVIEW;        return; }
	if (namesamen(tclstr, x_("#window"),       7) == 0) { *addr = myatoi(&tclstr[7]);  *type = VWINDOWPART;  return; }
	if (namesamen(tclstr, x_("#graphics"),     9) == 0) { *addr = myatoi(&tclstr[9]);  *type = VGRAPHICS;    return; }
	if (namesamen(tclstr, x_("#constraint"),  11) == 0) { *addr = myatoi(&tclstr[11]); *type = VCONSTRAINT;  return; }
	if (namesamen(tclstr, x_("#windowframe"), 12) == 0) { *addr = myatoi(&tclstr[7]);  *type = VWINDOWFRAME; return; }

	/* just a string */
	infstr = initinfstr();
	addstringtoinfstr(infstr, tclstr);
	*addr = (INTBIG)returninfstr(infstr);
	*type = VSTRING;
}

INTBIG tcl_converttoelectric(CHAR *tclstr, INTBIG type)
{
	REGISTER void *infstr;

	switch (type&VTYPE)
	{
		case VSTRING:
			infstr = initinfstr();
			addstringtoinfstr(infstr, tclstr);
			return((INTBIG)returninfstr(infstr));

		case VINTEGER:
		case VSHORT:
		case VBOOLEAN:
		case VADDRESS:
			return(myatoi(tclstr));

		case VFLOAT:
		case VDOUBLE:
			return(castint((float)eatof(tclstr)));

		case VFRACT:
			return(myatoi(tclstr) * WHOLE);

		case VNODEINST:    if (namesamen(tclstr, x_("#nodeinst"),     9) == 0) return(myatoi(&tclstr[9]));  break;
		case VNODEPROTO:   if (namesamen(tclstr, x_("#nodeproto"),   10) == 0) return(myatoi(&tclstr[10])); break;
		case VPORTARCINST: if (namesamen(tclstr, x_("#portarcinst"), 12) == 0) return(myatoi(&tclstr[12])); break;
		case VPORTEXPINST: if (namesamen(tclstr, x_("#portexpinst"), 12) == 0) return(myatoi(&tclstr[12])); break;
		case VPORTPROTO:   if (namesamen(tclstr, x_("#portproto"),   10) == 0) return(myatoi(&tclstr[10])); break;
		case VARCINST:     if (namesamen(tclstr, x_("#arcinst"),      8) == 0) return(myatoi(&tclstr[8]));  break;
		case VARCPROTO:    if (namesamen(tclstr, x_("#arcproto"),     9) == 0) return(myatoi(&tclstr[9]));  break;
		case VGEOM:        if (namesamen(tclstr, x_("#geom"),         5) == 0) return(myatoi(&tclstr[5]));  break;
		case VLIBRARY:     if (namesamen(tclstr, x_("#library"),      8) == 0) return(myatoi(&tclstr[8]));  break;
		case VTECHNOLOGY:  if (namesamen(tclstr, x_("#technology"),  11) == 0) return(myatoi(&tclstr[11])); break;
		case VTOOL:        if (namesamen(tclstr, x_("#tool"),         5) == 0) return(myatoi(&tclstr[5]));  break;
		case VRTNODE:      if (namesamen(tclstr, x_("#rtnode"),       7) == 0) return(myatoi(&tclstr[7]));  break;
		case VNETWORK:     if (namesamen(tclstr, x_("#network"),      8) == 0) return(myatoi(&tclstr[8]));  break;
		case VVIEW:        if (namesamen(tclstr, x_("#view"),         5) == 0) return(myatoi(&tclstr[5]));  break;
		case VWINDOWPART:  if (namesamen(tclstr, x_("#window"),       7) == 0) return(myatoi(&tclstr[7]));  break;
		case VGRAPHICS:    if (namesamen(tclstr, x_("#graphics"),     9) == 0) return(myatoi(&tclstr[9]));  break;
		case VCONSTRAINT:  if (namesamen(tclstr, x_("#constraint"),  11) == 0) return(myatoi(&tclstr[11])); break;
		case VWINDOWFRAME: if (namesamen(tclstr, x_("#windowframe"), 12) == 0) return(myatoi(&tclstr[12])); break;
	}
	return(-1);
}

INTBIG tcl_converttowarnedelectric(CHAR *name, INTBIG want, CHAR *msg)
{
	INTBIG addr, type;

	tcl_converttoanyelectric(name, &addr, &type);
	if (type != want)
	{
		Tcl_AppendResult(tcl_interp, msg, _(" argument has the wrong type"), (CHAR *)NULL);
		return(-1);
	}
	return(addr);
}

CHAR *tcl_converttotcl(INTBIG addr, INTBIG type)
{
	REGISTER INTBIG saddr, stype, len, i;
	REGISTER void *infstr;

	/* handle scalars easily */
	if ((type&VISARRAY) == 0)
	{
		infstr = initinfstr();
		tcl_converttoscalartcl(infstr, addr, type);
		return(returninfstr(infstr));
	}

	/* handle arrays */
	infstr = initinfstr();
	len = (type&VLENGTH) >> VLENGTHSH;
	if (len != 0)
	{
		stype = type & VTYPE;
		for(i=0; i<len; i++)
		{
			if ((type&VTYPE) == VGENERAL)
			{
				stype = ((INTBIG *)addr)[i*2+1];
				saddr = ((INTBIG *)addr)[i*2];
			} else
			{
				if ((type&VTYPE) == VCHAR) saddr = ((CHAR *)addr)[i]; else
					if ((type&VTYPE) == VDOUBLE) saddr = (INTBIG)(((double *)addr)[i]); else
						if ((type&VTYPE) == VSHORT) saddr = ((INTSML *)addr)[i]; else
							saddr = ((INTBIG *)addr)[i];
			}
			if (i != 0) addtoinfstr(infstr, ' ');
			tcl_converttoscalartcl(infstr, saddr, stype);
		}
	} else
	{
		for(i=0; ; i++)
		{
			if ((type&VTYPE) == VCHAR)
			{
				if ((((CHAR *)addr)[i]&0377) == 0377) break;
				saddr = ((CHAR *)addr)[i];
			} else if ((type&VTYPE) == VDOUBLE)
			{
				if (((double *)addr)[i] == -1) break;
				saddr = (INTBIG)(((double *)addr)[i]);
			} else if ((type&VTYPE) == VSHORT)
			{
				if (((INTSML *)addr)[i] == -1) break;
				saddr = ((INTSML *)addr)[i];
			} else
			{
				saddr = ((INTBIG *)addr)[i];
				if (saddr == -1) break;
			}
			if (i != 0) addtoinfstr(infstr, ' ');
			tcl_converttoscalartcl(infstr, saddr, type);
		}
	}
	return(returninfstr(infstr));
}

void tcl_converttoscalartcl(void *infstr, INTBIG addr, INTBIG type)
{
	static CHAR line[50];

	switch (type&VTYPE)
	{
		case VSTRING:
			addtoinfstr(infstr, '"');
			addstringtoinfstr(infstr, (CHAR *)addr);
			addtoinfstr(infstr, '"');
			return;
		case VINTEGER:
		case VSHORT:
		case VBOOLEAN:
		case VADDRESS:     esnprintf(line, 50, x_("%ld"), addr);                break;
		case VFLOAT:
		case VDOUBLE:      esnprintf(line, 50, x_("%g"), castfloat(addr));      break;
		case VFRACT:       esnprintf(line, 50, x_("%g"), (float)addr / WHOLE);  break;
		case VNODEINST:    esnprintf(line, 50, x_("#nodeinst%ld"),    addr);    break;
		case VNODEPROTO:   esnprintf(line, 50, x_("#nodeproto%ld"),   addr);    break;
		case VPORTARCINST: esnprintf(line, 50, x_("#portarcinst%ld"), addr);    break;
		case VPORTEXPINST: esnprintf(line, 50, x_("#portexpinst%ld"), addr);    break;
		case VPORTPROTO:   esnprintf(line, 50, x_("#portproto%ld"),   addr);    break;
		case VARCINST:     esnprintf(line, 50, x_("#arcinst%ld"),     addr);    break;
		case VARCPROTO:    esnprintf(line, 50, x_("#arcproto%ld"),    addr);    break;
		case VGEOM:        esnprintf(line, 50, x_("#geom%ld"),        addr);    break;
		case VLIBRARY:     esnprintf(line, 50, x_("#library%ld"),     addr);    break;
		case VTECHNOLOGY:  esnprintf(line, 50, x_("#technology%ld"),  addr);    break;
		case VTOOL:        esnprintf(line, 50, x_("#tool%ld"),        addr);    break;
		case VRTNODE:      esnprintf(line, 50, x_("#rtnode%ld"),      addr);    break;
		case VNETWORK:     esnprintf(line, 50, x_("#network%ld"),     addr);    break;
		case VVIEW:        esnprintf(line, 50, x_("#view%ld"),        addr);    break;
		case VWINDOWPART:  esnprintf(line, 50, x_("#window%ld"),      addr);    break;
		case VGRAPHICS:    esnprintf(line, 50, x_("#graphics%ld"),    addr);    break;
		case VCONSTRAINT:  esnprintf(line, 50, x_("#constraint%ld"),  addr);    break;
		case VWINDOWFRAME: esnprintf(line, 50, x_("#windowframe%ld"), addr);    break;
		default: return;
	}
	addstringtoinfstr(infstr, line);
}

int tcl_tkconsoleclose(ClientData instanceData, Tcl_Interp *interp) { return 0; }

int tcl_tkconsoleinput(ClientData instanceData, char *buf, int bufSize, int *errorCode)
{
	REGISTER CHAR *pt;
	REGISTER INTBIG len, i;

	*errorCode = 0;
	*tcl_outputloc = 0;
	pt = ttygetlinemessages(tcl_outputbuffer);
	tcl_outputloc = tcl_outputbuffer;
	len = estrlen(pt);
	if (len > bufSize) len = bufSize;
	for(i=0; i<len; i++) buf[i] = pt[i];
	return(len);
}

int tcl_tkconsoleoutput(ClientData instanceData, char *buf, int toWrite, int *errorCode)
{
	CHAR save;

	*errorCode = 0;
	Tcl_SetErrno(0);

	save = buf[toWrite];
	buf[toWrite] = 0;
	tcl_dumpoutput(buf);
	buf[toWrite] = save;
	return(toWrite);
}

void tcl_tkconsolewatch(ClientData instanceData, int mask) {}

void tcl_dumpoutput(CHAR *str)
{
	while (*str != 0)
	{
		if (*str == '\n')
		{
			*tcl_outputloc = 0;
			ttyputmsg(x_("%s"), tcl_outputbuffer);
			tcl_outputloc = tcl_outputbuffer;
		} else
		{
			if (tcl_outputloc >= tcl_outputbuffer+BUFSIZE-2)
			{
				*tcl_outputloc = 0;
				ttyputmsg(x_("%s"), tcl_outputbuffer);
				tcl_outputloc = tcl_outputbuffer;
			}
			*tcl_outputloc++ = *str;
		}
		str++;
	}
}

/************************* DATABASE EXAMINATION ROUTINES *************************/

int tcl_curlib(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	if (argc != 1)
	{
		Tcl_AppendResult(interp, x_("Usage: curlib"), (CHAR *)NULL);
		return TCL_ERROR;
	}

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)el_curlib, VLIBRARY), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_curtech(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	if (argc != 1)
	{
		Tcl_AppendResult(interp, x_("Usage: curtech"), (CHAR *)NULL);
		return TCL_ERROR;
	}

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)el_curtech, VTECHNOLOGY), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_getval(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	INTBIG addr, type;
	CHAR *retval;
	REGISTER VARIABLE *var;

	/* get input */
	if (argc != 3)
	{
		Tcl_AppendResult(interp, x_("Usage: getval OBJECT QUAL"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	tcl_converttoanyelectric(argv[1], &addr, &type);

	/* call Electric */
	var = getval(addr, type, -1, argv[2]);

	/* convert result */
	if (var != NOVARIABLE)
	{
		retval = tcl_converttotcl(var->addr, var->type);
		Tcl_AppendResult(interp, retval, (CHAR *)NULL);
	}
	return TCL_OK;
}

int tcl_getparentval(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	INTBIG height;
	CHAR *retval;
	REGISTER VARIABLE *var;

	/* get input */
	if (argc != 4)
	{
		Tcl_AppendResult(interp, x_("Usage: getparentval OBJECT DEFAULT HEIGHT"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	height = myatoi(argv[3]);

	/* call Electric */
	var = getparentval(argv[1], height);

	/* convert result */
	if (var == NOVARIABLE) retval = argv[2]; else
		retval = tcl_converttotcl(var->addr, var->type);
	Tcl_AppendResult(interp, retval, (CHAR *)NULL);
	return TCL_OK;
}

int tcl_P(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	CHAR *retval, line[300];
	REGISTER VARIABLE *var;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: P OBJECT"), (CHAR *)NULL);
		return TCL_ERROR;
	}

	/* call Electric */
	esnprintf(line, 300, x_("ATTR_%s"), argv[1]);
	var = getparentval(line, 1);

	/* convert result */
	if (var != NOVARIABLE)
	{
		retval = tcl_converttotcl(var->addr, var->type);
		Tcl_AppendResult(interp, retval, (CHAR *)NULL);
	}
	return TCL_OK;
}

int tcl_PD(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	CHAR *retval, line[300];
	REGISTER VARIABLE *var;

	/* get input */
	if (argc != 3)
	{
		Tcl_AppendResult(interp, x_("Usage: PD OBJECT DEFAULT"), (CHAR *)NULL);
		return TCL_ERROR;
	}

	/* call Electric */
	esnprintf(line, 300, x_("ATTR_%s"), argv[1]);
	var = getparentval(line, 1);

	/* convert result */
	if (var == NOVARIABLE) retval = argv[2]; else
		retval = tcl_converttotcl(var->addr, var->type);
	Tcl_AppendResult(interp, retval, (CHAR *)NULL);
	return TCL_OK;
}

int tcl_PAR(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	CHAR *retval, line[300];
	REGISTER VARIABLE *var;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: PAR OBJECT"), (CHAR *)NULL);
		return TCL_ERROR;
	}

	/* call Electric */
	esnprintf(line, 300, x_("ATTR_%s"), argv[1]);
	var = getparentval(line, 0);

	/* convert result */
	if (var != NOVARIABLE)
	{
		retval = tcl_converttotcl(var->addr, var->type);
		Tcl_AppendResult(interp, retval, (CHAR *)NULL);
	}
	return TCL_OK;
}

int tcl_PARD(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	CHAR *retval, line[300];
	REGISTER VARIABLE *var;

	/* get input */
	if (argc != 3)
	{
		Tcl_AppendResult(interp, x_("Usage: PARD OBJECT DEFAULT"), (CHAR *)NULL);
		return TCL_ERROR;
	}

	/* call Electric */
	esnprintf(line, 300, x_("ATTR_%s"), argv[1]);
	var = getparentval(line, 0);

	/* convert result */
	if (var == NOVARIABLE) retval = argv[2]; else
		retval = tcl_converttotcl(var->addr, var->type);
	Tcl_AppendResult(interp, retval, (CHAR *)NULL);
	return TCL_OK;
}

int tcl_setval(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	INTBIG addr, type, naddr, ntype, i, *naddrlist;
	REGISTER VARIABLE *ret;
	CHAR *retval, **setArgv;
	int setArgc;

	/* get input */
	if (argc != 4 && argc != 5)
	{
		Tcl_AppendResult(interp, x_("Usage: setval OBJECT QUAL NEWVALUE [OPTIONS]"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	tcl_converttoanyelectric(argv[1], &addr, &type);

	/* see how many entries are being set */
	i = Tcl_SplitList(interp, argv[3], &setArgc, &setArgv);
	if (i != TCL_OK) return TCL_ERROR;

	/* see if the "newvalue" is an array */
	if (setArgc > 1)
	{
		/* setting an array */
		naddrlist = (INTBIG *)emalloc(setArgc * SIZEOFINTBIG * 2, el_tempcluster);
		if (naddrlist == 0) return TCL_ERROR;
		for(i=0; i<setArgc; i++)
			tcl_converttoanyelectric(setArgv[i], &naddrlist[i*2], &naddrlist[i*2+1]);
		ckfree((CHAR *)setArgv);

		/* see if the array is uniform */
		for(i=1; i<setArgc; i++)
			if (naddrlist[(i-1)*2+1] != naddrlist[i*2+1]) break;
		if (i < setArgc)
		{
			/* general array */
			ret = setval(addr, type, argv[2], (INTBIG)naddrlist,
				VGENERAL|VISARRAY|((setArgc*2) << VLENGTHSH));
		} else
		{
			/* a uniform array */
			ntype = naddrlist[1];
			for(i=1; i<setArgc; i++) naddrlist[i] = naddrlist[i*2];
			ret = setval(addr, type, argv[2], (INTBIG)naddrlist,
				ntype|VISARRAY|(setArgc << VLENGTHSH));
		}
		efree((CHAR *)naddrlist);
	} else
	{
		/* just setting a single value */
		tcl_converttoanyelectric(argv[3], &naddr, &ntype);

		/* call Electric */
		ret = setval(addr, type, argv[2], naddr, ntype);
	}
	if (argc == 5)
	{
		if (namesame(argv[4], x_("displayable")) == 0) ntype |= VDISPLAY;
	}

	/* convert result */
	if (ret != NOVARIABLE)
	{
		retval = tcl_converttotcl(ret->addr, ret->type);
		Tcl_AppendResult(interp, retval, (CHAR *)NULL);
	}
	return TCL_OK;
}

int tcl_setind(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	INTBIG addr, type, aindex, naddr, ntype;
	BOOLEAN ret;

	/* get input */
	if (argc != 5)
	{
		Tcl_AppendResult(interp, x_("Usage: setind OBJECT QUAL INDEX NEWVALUE"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	tcl_converttoanyelectric(argv[1], &addr, &type);
	aindex = myatoi(argv[3]);
	tcl_converttoanyelectric(argv[4], &naddr, &ntype);

	/* call Electric */
	ret = setind(addr, type, argv[2], aindex, naddr);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl(ret, VINTEGER), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_delval(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	INTBIG addr, type;
	BOOLEAN ret;

	/* get input */
	if (argc != 3)
	{
		Tcl_AppendResult(interp, x_("Usage: delval OBJECT QUAL"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	tcl_converttoanyelectric(argv[1], &addr, &type);

	/* call Electric */
	ret = delval(addr, type, argv[2]);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl(ret, VINTEGER), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_initsearch(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER INTBIG lx, hx, ly, hy, sea;
	REGISTER NODEPROTO *np;

	/* get input */
	if (argc != 6)
	{
		Tcl_AppendResult(interp, x_("Usage: initsearch LX HX LY HY CELL"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	lx = myatoi(argv[1]);
	hx = myatoi(argv[2]);
	ly = myatoi(argv[3]);
	hy = myatoi(argv[4]);
	np = (NODEPROTO *)tcl_converttowarnedelectric(argv[5], VNODEPROTO, x_("fifth"));
	if (np == NONODEPROTO) return TCL_ERROR;

	/* call Electric */
	sea = initsearch(lx, hx, ly, hy, np);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl(sea, VINTEGER), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_nextobject(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER INTBIG sea;
	REGISTER GEOM *g;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: nextobject SEARCH"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	sea = myatoi(argv[1]);

	/* call Electric */
	g = nextobject(sea);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)g, VGEOM), (CHAR *) NULL);
	return TCL_OK;
}

/****************************** TOOL ROUTINES ******************************/

int tcl_gettool(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER TOOL *tool;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: gettool NAME"), (CHAR *)NULL);
		return TCL_ERROR;
	}

	/* call Electric */
	tool = gettool(argv[1]);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)tool, VTOOL), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_maxtool(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	/* get input */
	if (argc != 1)
	{
		Tcl_AppendResult(interp, x_("Usage: maxtool"), (CHAR *)NULL);
		return TCL_ERROR;
	}

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl(el_maxtools, VINTEGER), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_indextool(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER TOOL *tool;
	REGISTER INTBIG aindex;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: indextool INDEX"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	aindex = myatoi(argv[1]);

	/* call Electric */
	if (aindex < 0 || aindex >= el_maxtools) tool = NOTOOL; else tool = &el_tools[aindex];

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)tool, VTOOL), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_toolturnon(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER TOOL *tool;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: toolturnon TOOL"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	tool = (TOOL *)tcl_converttowarnedelectric(argv[1], VTOOL, x_("first"));
	if (tool == NOTOOL) return TCL_ERROR;

	/* call Electric */
	toolturnon(tool);
	return TCL_OK;
}

int tcl_toolturnoff(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER TOOL *tool;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: toolturnoff TOOL"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	tool = (TOOL *)tcl_converttowarnedelectric(argv[1], VTOOL, x_("first"));
	if (tool == NOTOOL) return TCL_ERROR;

	/* call Electric */
	toolturnoff(tool, TRUE);
	return TCL_OK;
}

/****************************** LIBRARY ROUTINES ******************************/

int tcl_getlibrary(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER LIBRARY *lib;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: getlibrary NAME"), (CHAR *)NULL);
		return TCL_ERROR;
	}

	/* call Electric */
	lib = getlibrary(argv[1]);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)lib, VLIBRARY), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_newlibrary(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER LIBRARY *lib;

	/* get input */
	if (argc != 3)
	{
		Tcl_AppendResult(interp, x_("Usage: newlibrary NAME FILE"), (CHAR *)NULL);
		return TCL_ERROR;
	}

	/* call Electric */
	lib = newlibrary(argv[1], argv[2]);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)lib, VLIBRARY), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_killlibrary(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER LIBRARY *lib;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: killlibrary LIBRARY"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	lib = (LIBRARY *)tcl_converttowarnedelectric(argv[1], VLIBRARY, x_("first"));
	if (lib == NOLIBRARY) return TCL_ERROR;

	/* call Electric */
	killlibrary(lib);
	return TCL_OK;
}

int tcl_eraselibrary(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER LIBRARY *lib;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: eraselibrary LIBRARY"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	lib = (LIBRARY *)tcl_converttowarnedelectric(argv[1], VLIBRARY, x_("first"));
	if (lib == NOLIBRARY) return TCL_ERROR;

	/* call Electric */
	eraselibrary(lib);
	return TCL_OK;
}

int tcl_selectlibrary(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER LIBRARY *lib;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: selectlibrary LIBRARY"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	lib = (LIBRARY *)tcl_converttowarnedelectric(argv[1], VLIBRARY, x_("first"));
	if (lib == NOLIBRARY) return TCL_ERROR;

	/* call Electric */
	selectlibrary(lib, TRUE);
	return TCL_OK;
}

/****************************** NODEPROTO ROUTINES ******************************/

int tcl_getnodeproto(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER NODEPROTO *np;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: getnodeproto NAME"), (CHAR *)NULL);
		return TCL_ERROR;
	}

	/* call Electric */
	np = getnodeproto(argv[1]);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)np, VNODEPROTO), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_newnodeproto(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;

	/* get input */
	if (argc != 3)
	{
		Tcl_AppendResult(interp, x_("Usage: newnodeproto NAME LIBRARY"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	lib = (LIBRARY *)tcl_converttowarnedelectric(argv[2], VLIBRARY, x_("second"));
	if (lib == NOLIBRARY) return TCL_ERROR;

	/* call Electric */
	np = newnodeproto(argv[1], lib);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)np, VNODEPROTO), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_killnodeproto(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER NODEPROTO *np;
	REGISTER BOOLEAN ret;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: killnodeproto CELL"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	np = (NODEPROTO *)tcl_converttowarnedelectric(argv[1], VNODEPROTO, x_("first"));
	if (np == NONODEPROTO) return TCL_ERROR;

	/* call Electric */
	ret = killnodeproto(np);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl(ret?1:0, VINTEGER), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_copynodeproto(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np, *nnp;

	/* get input */
	if (argc != 4)
	{
		Tcl_AppendResult(interp, x_("Usage: copynodeproto CELL LIBRARY NAME"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	np = (NODEPROTO *)tcl_converttowarnedelectric(argv[1], VNODEPROTO, x_("first"));
	lib = (LIBRARY *)tcl_converttowarnedelectric(argv[2], VLIBRARY, x_("second"));
	if (lib == NOLIBRARY || np == NONODEPROTO) return TCL_ERROR;

	/* call Electric */
	nnp = copynodeproto(np, lib, argv[3], FALSE);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)nnp, VNODEPROTO), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_iconview(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER NODEPROTO *np, *inp;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: iconview CELL"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	np = (NODEPROTO *)tcl_converttowarnedelectric(argv[1], VNODEPROTO, x_("first"));
	if (np == NONODEPROTO) return TCL_ERROR;

	/* call Electric */
	inp = iconview(np);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)inp, VNODEPROTO), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_contentsview(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER NODEPROTO *np, *cnp;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: contentsview CELL"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	np = (NODEPROTO *)tcl_converttowarnedelectric(argv[1], VNODEPROTO, x_("first"));
	if (np == NONODEPROTO) return TCL_ERROR;

	/* call Electric */
	cnp = contentsview(np);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)cnp, VNODEPROTO), (CHAR *) NULL);
	return TCL_OK;
}

/****************************** NODEINST ROUTINES ******************************/

int tcl_newnodeinst(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np, *cell;
	REGISTER INTBIG lx, hx, ly, hy, rot, tran;

	/* get input */
	if (argc != 9)
	{
		Tcl_AppendResult(interp, x_("Usage: newnodeinst PROTO LX HX LY HY TRAN ROT CELL"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	np = (NODEPROTO *)tcl_converttowarnedelectric(argv[1], VNODEPROTO, x_("first"));
	lx = myatoi(argv[2]);
	hx = myatoi(argv[3]);
	ly = myatoi(argv[4]);
	hy = myatoi(argv[5]);
	tran = myatoi(argv[6]);
	rot = myatoi(argv[7]);
	cell = (NODEPROTO *)tcl_converttowarnedelectric(argv[8], VNODEPROTO, x_("eighth"));
	if (np == NONODEPROTO || cell == NONODEPROTO) return TCL_ERROR;

	/* call Electric */
	ni = newnodeinst(np, lx, hx, ly, hy, tran, rot, cell);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)ni, VNODEINST), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_modifynodeinst(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER NODEINST *ni;
	REGISTER INTBIG dlx, dly, dhx, dhy, drot, dtran;

	/* get input */
	if (argc != 8)
	{
		Tcl_AppendResult(interp, x_("Usage: modifynodeinst NODE dLX dLY dHX dHY dROT dTRAN"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	ni = (NODEINST *)tcl_converttowarnedelectric(argv[1], VNODEINST, x_("first"));
	dlx = myatoi(argv[2]);
	dly = myatoi(argv[3]);
	dhx = myatoi(argv[4]);
	dhy = myatoi(argv[5]);
	drot = myatoi(argv[6]);
	dtran = myatoi(argv[7]);
	if (ni == NONODEINST) return TCL_ERROR;

	/* call Electric */
	modifynodeinst(ni, dlx, dly, dhx, dhy, drot, dtran);
	return TCL_OK;
}

int tcl_killnodeinst(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER NODEINST *ni;
	REGISTER BOOLEAN ret;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: killnodeinst NODE"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	ni = (NODEINST *)tcl_converttowarnedelectric(argv[1], VNODEINST, x_("first"));
	if (ni == NONODEINST) return TCL_ERROR;

	/* call Electric */
	ret = killnodeinst(ni);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl(ret?1:0, VINTEGER), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_replacenodeinst(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni, *nni;

	/* get input */
	if (argc != 3)
	{
		Tcl_AppendResult(interp, x_("Usage: replacenodeinst NODE NEWPROTO"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	ni = (NODEINST *)tcl_converttowarnedelectric(argv[1], VNODEINST, x_("first"));
	np = (NODEPROTO *)tcl_converttowarnedelectric(argv[2], VNODEPROTO, x_("second"));
	if (ni == NONODEINST || np == NONODEPROTO) return TCL_ERROR;

	/* call Electric */
	nni = replacenodeinst(ni, np, FALSE, FALSE);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)nni, VNODEINST), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_nodefunction(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER NODEINST *ni;
	REGISTER INTBIG fun;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: nodefunction NODE"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	ni = (NODEINST *)tcl_converttowarnedelectric(argv[1], VNODEINST, x_("first"));
	if (ni == NONODEINST) return TCL_ERROR;

	/* call Electric */
	fun = nodefunction(ni);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl(fun, VINTEGER), (CHAR *) NULL);
	return TCL_OK;
}

/****************************** ARCINST ROUTINES ******************************/

int tcl_newarcinst(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni1, *ni2;
	REGISTER PORTPROTO *pp1, *pp2;
	REGISTER ARCPROTO *ap;
	REGISTER NODEPROTO *np;
	REGISTER INTBIG w, bits, x1, y1, x2, y2;

	/* get input */
	if (argc != 13)
	{
		Tcl_AppendResult(interp, x_("Usage: newarcinst PROTO WID BITS NODE1 PORT1 X1 Y1 NODE2 PORT2 X2 Y2 CELL"),
			(CHAR *)NULL);
		return TCL_ERROR;
	}
	ap = (ARCPROTO *)tcl_converttowarnedelectric(argv[1], VARCPROTO, x_("first"));
	w = myatoi(argv[2]);
	bits = myatoi(argv[3]);
	ni1 = (NODEINST *)tcl_converttowarnedelectric(argv[4], VNODEINST, x_("fourth"));
	pp1 = (PORTPROTO *)tcl_converttowarnedelectric(argv[5], VPORTPROTO, x_("fifth"));
	x1 = myatoi(argv[6]);
	y1 = myatoi(argv[7]);
	ni2 = (NODEINST *)tcl_converttowarnedelectric(argv[8], VNODEINST, x_("eighth"));
	pp2 = (PORTPROTO *)tcl_converttowarnedelectric(argv[9], VPORTPROTO, x_("ninth"));
	x2 = myatoi(argv[10]);
	y2 = myatoi(argv[11]);
	np = (NODEPROTO *)tcl_converttowarnedelectric(argv[12], VNODEPROTO, x_("twelvth"));
	if (ap == NOARCPROTO || ni1 == NONODEINST || pp1 == NOPORTPROTO || ni2 == NONODEINST ||
		pp2 == NOPORTPROTO || np == NONODEPROTO) return TCL_ERROR;

	/* call Electric */
	ai = newarcinst(ap, w, bits, ni1, pp1, x1, y1, ni2, pp2, x2, y2, np);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)ai, VARCINST), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_modifyarcinst(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER ARCINST *ai;
	REGISTER INTBIG dw, dx1, dy1, dx2, dy2;
	REGISTER BOOLEAN ret;

	/* get input */
	if (argc != 7)
	{
		Tcl_AppendResult(interp, x_("Usage: modifyarcinst ARC dW dX1 dY1 dX2 dY2"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	ai = (ARCINST *)tcl_converttowarnedelectric(argv[1], VARCINST, x_("first"));
	if (ai == NOARCINST) return TCL_ERROR;
	dw = myatoi(argv[2]);
	dx1 = myatoi(argv[3]);
	dy1 = myatoi(argv[4]);
	dx2 = myatoi(argv[5]);
	dy2 = myatoi(argv[6]);

	/* call Electric */
	ret = modifyarcinst(ai, dw, dx1, dy1, dx2, dy2);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl(ret?1:0, VINTEGER), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_killarcinst(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER ARCINST *ai;
	REGISTER BOOLEAN ret;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: killarcinst ARC"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	ai = (ARCINST *)tcl_converttowarnedelectric(argv[1], VARCINST, x_("first"));
	if (ai == NOARCINST) return TCL_ERROR;

	/* call Electric */
	ret = killarcinst(ai);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl(ret?1:0, VINTEGER), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_replacearcinst(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER ARCPROTO *ap;
	REGISTER ARCINST *ai, *nai;

	/* get input */
	if (argc != 3)
	{
		Tcl_AppendResult(interp, x_("Usage: replacearcinst ARC NEWPROTO"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	ai = (ARCINST *)tcl_converttowarnedelectric(argv[1], VARCINST, x_("first"));
	ap = (ARCPROTO *)tcl_converttowarnedelectric(argv[2], VARCPROTO, x_("second"));
	if (ai == NOARCINST || ap == NOARCPROTO) return TCL_ERROR;

	/* call Electric */
	nai = replacearcinst(ai, ap);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)nai, VARCINST), (CHAR *) NULL);
	return TCL_OK;
}

/****************************** PORTPROTO ROUTINES ******************************/

int tcl_newportproto(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER PORTPROTO *pp, *npp;

	/* get input */
	if (argc != 5)
	{
		Tcl_AppendResult(interp, x_("Usage: newportproto CELL NODE PORT NAME"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	np = (NODEPROTO *)tcl_converttowarnedelectric(argv[1], VNODEPROTO, x_("first"));
	ni = (NODEINST *)tcl_converttowarnedelectric(argv[2], VNODEINST, x_("second"));
	pp = (PORTPROTO *)tcl_converttowarnedelectric(argv[3], VPORTPROTO, x_("third"));
	if (np == NONODEPROTO || ni == NONODEINST || pp == NOPORTPROTO) return TCL_ERROR;

	/* call Electric */
	npp = newportproto(np, ni, pp, argv[4]);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)npp, VPORTPROTO), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_portposition(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER NODEINST *ni;
	REGISTER PORTPROTO *pp;
	INTBIG x, y;
	CHAR line[50];

	/* get input */
	if (argc != 3)
	{
		Tcl_AppendResult(interp, x_("Usage: portposition NODE PORT"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	ni = (NODEINST *)tcl_converttowarnedelectric(argv[1], VNODEINST, x_("first"));
	pp = (PORTPROTO *)tcl_converttowarnedelectric(argv[2], VPORTPROTO, x_("second"));
	if (ni == NONODEINST || pp == NOPORTPROTO) return TCL_ERROR;

	/* call Electric */
	portposition(ni, pp, &x, &y);

	/* convert result */
	(void)esnprintf(line, 50, x_("%ld %ld"), x, y);
	Tcl_AppendResult(interp, line, (CHAR *) NULL);
	return TCL_OK;
}

int tcl_getportproto(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;

	/* get input */
	if (argc != 3)
	{
		Tcl_AppendResult(interp, x_("Usage: getportproto CELL NAME"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	np = (NODEPROTO *)tcl_converttowarnedelectric(argv[1], VNODEPROTO, x_("first"));
	if (np == NONODEPROTO) return TCL_ERROR;

	/* call Electric */
	pp = getportproto(np, argv[2]);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)pp, VPORTPROTO), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_killportproto(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
	REGISTER BOOLEAN ret;

	/* get input */
	if (argc != 3)
	{
		Tcl_AppendResult(interp, x_("Usage: killportproto CELL PORT"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	np = (NODEPROTO *)tcl_converttowarnedelectric(argv[1], VNODEPROTO, x_("first"));
	pp = (PORTPROTO *)tcl_converttowarnedelectric(argv[2], VPORTPROTO, x_("second"));
	if (np == NONODEPROTO || pp == NOPORTPROTO) return TCL_ERROR;

	/* call Electric */
	ret = killportproto(np, pp);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl(ret?1:0, VINTEGER), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_moveportproto(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *opp, *npp;
	REGISTER NODEINST *nni;
	REGISTER BOOLEAN ret;

	/* get input */
	if (argc != 5)
	{
		Tcl_AppendResult(interp, x_("Usage: moveportproto CELL PORT NEWNODE NEWPORT"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	np = (NODEPROTO *)tcl_converttowarnedelectric(argv[1], VNODEPROTO, x_("first"));
	opp = (PORTPROTO *)tcl_converttowarnedelectric(argv[2], VPORTPROTO, x_("second"));
	nni = (NODEINST *)tcl_converttowarnedelectric(argv[3], VNODEINST, x_("third"));
	npp = (PORTPROTO *)tcl_converttowarnedelectric(argv[4], VPORTPROTO, x_("fourth"));
	if (np == NONODEPROTO || opp == NOPORTPROTO || nni == NONODEINST || npp == NOPORTPROTO) return TCL_ERROR;

	/* call Electric */
	ret = moveportproto(np, opp, nni, npp);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl(ret?1:0, VINTEGER), (CHAR *) NULL);
	return TCL_OK;
}

/*************************** CHANGE CONTROL ROUTINES ***************************/

int tcl_undoabatch(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	TOOL *tool;

	if (argc != 1)
	{
		Tcl_AppendResult(interp, x_("Usage: undoabatch"), (CHAR *)NULL);
		return TCL_ERROR;
	}

	/* call Electric */
	if (undoabatch(&tool) == 0)
	{
		Tcl_AppendResult(interp, _("Error during undoabatch"), (CHAR *)NULL);
		return TCL_ERROR;
	}

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)tool, VTOOL), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_noundoallowed(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	if (argc != 1)
	{
		Tcl_AppendResult(interp, x_("Usage: noundoallowed"), (CHAR *)NULL);
		return TCL_ERROR;
	}

	/* call Electric */
	noundoallowed();
	return TCL_OK;
}

/****************************** VIEW ROUTINES ******************************/

int tcl_getview(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER VIEW *v;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: getview NAME"), (CHAR *)NULL);
		return TCL_ERROR;
	}

	/* call Electric */
	v = getview(argv[1]);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)v, VVIEW), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_newview(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER VIEW *v;

	/* get input */
	if (argc != 3)
	{
		Tcl_AppendResult(interp, x_("Usage: newview NAME SNAME"), (CHAR *)NULL);
		return TCL_ERROR;
	}

	/* call Electric */
	v = newview(argv[1], argv[2]);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)v, VVIEW), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_killview(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER VIEW *v;
	REGISTER BOOLEAN ret;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: killview VIEW"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	v = (VIEW *)tcl_converttowarnedelectric(argv[1], VVIEW, x_("first"));
	if (v == NOVIEW) return TCL_ERROR;

	/* call Electric */
	ret = killview(v);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl(ret?1:0, VINTEGER), (CHAR *) NULL);
	return TCL_OK;
}

/*************************** MISCELLANEOUS ROUTINES ***************************/

int tcl_telltool(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER TOOL *tool;
	int toolArgc, ret;
	CHAR **toolArgv;

	/* get input */
	if (argc != 3)
	{
		Tcl_AppendResult(interp, x_("Usage: telltool TOOL {PARAMETERS}"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	tool = (TOOL *)tcl_converttowarnedelectric(argv[1], VTOOL, x_("first"));
	ret = Tcl_SplitList(interp, argv[2], &toolArgc, &toolArgv);
	if (tool == NOTOOL || ret != TCL_OK) return TCL_ERROR;

	/* call Electric */
	telltool(tool, toolArgc, toolArgv);
	ckfree((CHAR *)toolArgv);

	/* convert result */
	ret = 0;		/* actually should not return a value!!! */
	Tcl_AppendResult(interp, tcl_converttotcl(ret, VINTEGER), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_asktool(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER TOOL *tool;
	INTBIG addr, addr0, addr1, addr2, addr3, type;
	int toolArgc, ret, i, argcS, first;
	CHAR **toolArgv, **argvS, *command;
	REGISTER void *infstr;

	/* get input */
	if (argc != 3)
	{
		Tcl_AppendResult(interp, x_("Usage: asktool TOOL {PARAMETERS}"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	tool = (TOOL *)tcl_converttowarnedelectric(argv[1], VTOOL, x_("first"));
	ret = Tcl_SplitList(interp, argv[2], &toolArgc, &toolArgv);
	if (tool == NOTOOL || ret != TCL_OK) return TCL_ERROR;

	command = toolArgv[0];
	for (i=0; i<toolArgc; i++) toolArgv[i] = toolArgv[i+1];
	toolArgv[toolArgc] = NULL;
	toolArgc--;

	/* call Electric */
	if (toolArgc == 0)
	{
		ret = asktool(tool, command);
	} else if (toolArgc == 1)
	{
		/* special case for user_tool: show-multiple command */
		if (estrcmp(command, x_("show-multiple")) == 0)
		{
			ret = Tcl_SplitList(interp, toolArgv[0], &argcS, &argvS);
			if (ret == TCL_OK)
			{
				infstr = initinfstr();
				first = 0;
				for (i=0; i<argcS; i++)
				{
					if (first != 0) addtoinfstr(infstr, '\n');
					first++;
					tcl_converttoanyelectric(argvS[i], &addr, &type);
					formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0"),
						describenodeproto(((GEOM *)addr)->entryaddr.ni->parent), addr);
				}
				ret = asktool(tool, command, (INTBIG)returninfstr(infstr));
			}
		} else
		{
			tcl_converttoanyelectric(toolArgv[0], &addr0, &type);
			ret = asktool(tool, command, addr0);
		}
	} else if (toolArgc == 2)
	{
		tcl_converttoanyelectric(toolArgv[0], &addr0, &type);
		tcl_converttoanyelectric(toolArgv[1], &addr1, &type);
		ret = asktool(tool, command, addr0, addr1);
	} else if (toolArgc == 3)
	{
		tcl_converttoanyelectric(toolArgv[0], &addr0, &type);
		tcl_converttoanyelectric(toolArgv[1], &addr1, &type);
		tcl_converttoanyelectric(toolArgv[2], &addr2, &type);
		ret = asktool(tool, command, addr0, addr1, addr2);
	} else if (toolArgc == 4)
	{
		tcl_converttoanyelectric(toolArgv[0], &addr0, &type);
		tcl_converttoanyelectric(toolArgv[1], &addr1, &type);
		tcl_converttoanyelectric(toolArgv[2], &addr2, &type);
		tcl_converttoanyelectric(toolArgv[3], &addr3, &type);
		ret = asktool(tool, command, addr0, addr1, addr2, addr3);
	}
	ckfree((CHAR *)toolArgv);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl(ret, VINTEGER), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_getarcproto(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER ARCPROTO *ap;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: getarcproto NAME"), (CHAR *)NULL);
		return TCL_ERROR;
	}

	/* call Electric */
	ap = getarcproto(argv[1]);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)ap, VARCPROTO), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_gettechnology(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER TECHNOLOGY *tech;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: gettechnology NAME"), (CHAR *)NULL);
		return TCL_ERROR;
	}

	/* call Electric */
	tech = gettechnology(argv[1]);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)tech, VTECHNOLOGY), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_getpinproto(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER NODEPROTO *np;
	REGISTER ARCPROTO *ap;

	/* get input */
	if (argc != 2)
	{
		Tcl_AppendResult(interp, x_("Usage: getpinproto ARCPROTO"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	ap = (ARCPROTO *)tcl_converttowarnedelectric(argv[1], VARCPROTO, x_("first"));
	if (ap == NOARCPROTO) return TCL_ERROR;

	/* call Electric */
	np = getpinproto(ap);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)np, VNODEPROTO), (CHAR *) NULL);
	return TCL_OK;
}

int tcl_getnetwork(ClientData dummy, Tcl_Interp *interp, int argc, CHAR **argv)
{
	REGISTER NETWORK *net;
	REGISTER NODEPROTO *np;

	/* get input */
	if (argc != 3)
	{
		Tcl_AppendResult(interp, x_("Usage: getnetwork NAME CELL"), (CHAR *)NULL);
		return TCL_ERROR;
	}
	np = (NODEPROTO *)tcl_converttowarnedelectric(argv[2], VNODEPROTO, x_("second"));
	if (np == NONODEPROTO) return TCL_ERROR;

	/* call Electric */
	net = getnetwork(argv[1], np);

	/* convert result */
	Tcl_AppendResult(interp, tcl_converttotcl((INTBIG)net, VNETWORK), (CHAR *) NULL);
	return TCL_OK;
}

#endif  /* LANGTCL - at top */
