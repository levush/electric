/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dblangelk.c
 * ELK Lisp interface module
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
#if LANGLISP

#include "global.h"
#include "dblang.h"

static ELKObject lsp_displayablesym;
static BOOLEAN   lsp_strportinited = FALSE;
static ELKObject lsp_strport;

#define Ensure_Type(x,t) if (TYPE(x) != t) { Wrong_Type(x, t); return(Null); }

/* prototypes for local routines */
static int EElectric_Equal(ELKObject, ELKObject);
static int ENodeInst_Print(ELKObject, ELKObject, int, int, int);
static int ENodeProto_Print(ELKObject, ELKObject, int, int, int);
static int EPortArcInst_Print(ELKObject, ELKObject, int, int, int);
static int EPortExpInst_Print(ELKObject, ELKObject, int, int, int);
static int EPortProto_Print(ELKObject, ELKObject, int, int, int);
static int EArcInst_Print(ELKObject, ELKObject, int, int, int);
static int EArcProto_Print(ELKObject, ELKObject, int, int, int);
static int EGeom_Print(ELKObject, ELKObject, int, int, int);
static int ELibrary_Print(ELKObject, ELKObject, int, int, int);
static int ETechnology_Print(ELKObject, ELKObject, int, int, int);
static int ETool_Print(ELKObject, ELKObject, int, int, int);
static int ERTNode_Print(ELKObject, ELKObject, int, int, int);
static int ENetwork_Print(ELKObject, ELKObject, int, int, int);
static int EView_Print(ELKObject, ELKObject, int, int, int);
static int EWindow_Print(ELKObject, ELKObject, int, int, int);
static int EGraphics_Print(ELKObject, ELKObject, int, int, int);
static int EConstraint_Print(ELKObject, ELKObject, int, int, int);
static int EWindowFrame_Print(ELKObject, ELKObject, int, int, int);
static ELKObject P_ENodeInstP(ELKObject);
static ELKObject P_ENodeProtoP(ELKObject);
static ELKObject P_EPortArcInstP(ELKObject);
static ELKObject P_EPortExpInstP(ELKObject);
static ELKObject P_EPortProtoP(ELKObject);
static ELKObject P_EArcInstP(ELKObject);
static ELKObject P_EArcProtoP(ELKObject);
static ELKObject P_EGeomP(ELKObject);
static ELKObject P_ELibraryP(ELKObject);
static ELKObject P_ETechnologyP(ELKObject);
static ELKObject P_EToolP(ELKObject);
static ELKObject P_ERTNodeP(ELKObject);
static ELKObject P_ENetworkP(ELKObject);
static ELKObject P_EViewP(ELKObject);
static ELKObject P_EWindowP(ELKObject);
static ELKObject P_EGraphicsP(ELKObject);
static ELKObject P_EConstraintP(ELKObject);
static ELKObject P_EWindowFrameP(ELKObject);
static ELKObject Make_EElectric(INTBIG, INTBIG);
static void init_lib_electric(void);
static BOOLEAN lsp_getnumericobject(ELKObject, INTBIG*);
static CHAR *lsp_getstringobject(ELKObject);
static void lsp_getaddrandtype(ELKObject, INTBIG*, INTBIG*);
static ELKObject lsp_makevarobject(INTBIG, INTBIG);
static ELKObject lsp_curlib(void);
static ELKObject lsp_curtech(void);
static ELKObject lsp_getval(ELKObject, ELKObject);
static ELKObject lsp_getparentval(ELKObject, ELKObject, ELKObject);
static ELKObject lsp_dogetparentval(CHAR*, ELKObject, INTBIG);
static ELKObject lsp_P(ELKObject oname);
static ELKObject lsp_PD(ELKObject oname, ELKObject odefault);
static ELKObject lsp_PAR(ELKObject oname);
static ELKObject lsp_PARD(ELKObject oname, ELKObject odefault);
static ELKObject lsp_setval(ELKObject, ELKObject, ELKObject, ELKObject);
static ELKObject lsp_setind(ELKObject, ELKObject, ELKObject, ELKObject);
static ELKObject lsp_delval(ELKObject, ELKObject);
static ELKObject lsp_initsearch(ELKObject, ELKObject, ELKObject, ELKObject, ELKObject);
static ELKObject lsp_nextobject(ELKObject);
static ELKObject lsp_gettool(ELKObject);
static ELKObject lsp_maxtool(void);
static ELKObject lsp_indextool(ELKObject);
static ELKObject lsp_toolturnon(ELKObject);
static ELKObject lsp_toolturnoff(ELKObject);
static ELKObject lsp_getlibrary(ELKObject);
static ELKObject lsp_newlibrary(ELKObject, ELKObject);
static ELKObject lsp_killlibrary(ELKObject);
static ELKObject lsp_eraselibrary(ELKObject);
static ELKObject lsp_selectlibrary(ELKObject);
static ELKObject lsp_getnodeproto(ELKObject);
static ELKObject lsp_newnodeproto(ELKObject, ELKObject);
static ELKObject lsp_killnodeproto(ELKObject);
static ELKObject lsp_copynodeproto(ELKObject, ELKObject, ELKObject);
static ELKObject lsp_iconview(ELKObject);
static ELKObject lsp_contentsview(ELKObject);
static ELKObject lsp_newnodeinst(ELKObject, ELKObject, ELKObject, ELKObject, ELKObject, ELKObject, ELKObject, ELKObject);
static ELKObject lsp_modifynodeinst(ELKObject, ELKObject, ELKObject, ELKObject, ELKObject, ELKObject, ELKObject);
static ELKObject lsp_killnodeinst(ELKObject);
static ELKObject lsp_replacenodeinst(ELKObject, ELKObject);
static ELKObject lsp_nodefunction(ELKObject);
static ELKObject lsp_newarcinst(ELKObject, ELKObject, ELKObject, ELKObject, ELKObject, ELKObject, ELKObject, ELKObject,
					ELKObject, ELKObject, ELKObject, ELKObject);
static ELKObject lsp_modifyarcinst(ELKObject, ELKObject, ELKObject, ELKObject, ELKObject, ELKObject);
static ELKObject lsp_killarcinst(ELKObject);
static ELKObject lsp_replacearcinst(ELKObject, ELKObject);
static ELKObject lsp_newportproto(ELKObject, ELKObject, ELKObject, ELKObject);
static ELKObject lsp_portposition(ELKObject, ELKObject);
static ELKObject lsp_getportproto(ELKObject, ELKObject);
static ELKObject lsp_killportproto(ELKObject, ELKObject);
static ELKObject lsp_moveportproto(ELKObject, ELKObject, ELKObject, ELKObject);
static ELKObject lsp_undoabatch(void);
static ELKObject lsp_noundoallowed(void);
static ELKObject lsp_getview(ELKObject);
static ELKObject lsp_newview(ELKObject, ELKObject);
static ELKObject lsp_killview(ELKObject);
static ELKObject lsp_telltool(long, ELKObject[]);
static ELKObject lsp_getarcproto(ELKObject);
static ELKObject lsp_gettechnology(ELKObject);
static ELKObject lsp_getpinproto(ELKObject);
static ELKObject lsp_getnetwork(ELKObject, ELKObject);

/****************************** ELECTRIC OBJECTS ******************************/

/* the internal LISP types */
static long T_ENodeInst, T_ENodeProto, T_EPortArcInst, T_EPortExpInst, T_EPortProto,
	T_EArcInst, T_EArcProto, T_EGeom, T_ELibrary, T_ETechnology, T_ETool,
	T_ERTNode, T_ENetwork, T_EView, T_EWindow, T_EGraphics, T_EConstraint,
	T_EWindowFrame;

/* the LISP structures */
struct S_Electric     { INTBIG       handle; };
struct S_ENodeInst    { NODEINST    *handle; };
struct S_ENodeProto   { NODEPROTO   *handle; };
struct S_EPortArcInst { PORTARCINST *handle; };
struct S_EPortExpInst { PORTEXPINST *handle; };
struct S_EPortProto   { PORTPROTO   *handle; };
struct S_EArcInst     { ARCINST     *handle; };
struct S_EArcProto    { ARCPROTO    *handle; };
struct S_EGeom        { GEOM        *handle; };
struct S_ELibrary     { LIBRARY     *handle; };
struct S_ETechnology  { TECHNOLOGY  *handle; };
struct S_ETool        { TOOL        *handle; };
struct S_ERTNode      { RTNODE      *handle; };
struct S_ENetwork     { NETWORK     *handle; };
struct S_EView        { VIEW        *handle; };
struct S_EWindow      { WINDOWPART  *handle; };
struct S_EGraphics    { GRAPHICS    *handle; };
struct S_EConstraint  { CONSTRAINT  *handle; };
struct S_EWindowFrame { WINDOWFRAME *handle; };

/* macros for accessing Electric objects from the Lisp structures */
#define EELECTRIC(obj)    ((struct S_Electric *)POINTER(obj))
#define ENODEINST(obj)    ((struct S_ENodeInst *)POINTER(obj))
#define ENODEPROTO(obj)   ((struct S_ENodeProto *)POINTER(obj))
#define EPORTARCINST(obj) ((struct S_EPortArcInst *)POINTER(obj))
#define EPORTEXPINST(obj) ((struct S_EPortExpInst *)POINTER(obj))
#define EPORTPROTO(obj)   ((struct S_EPortProto *)POINTER(obj))
#define EARCINST(obj)     ((struct S_EArcInst *)POINTER(obj))
#define EARCPROTO(obj)    ((struct S_EArcProto *)POINTER(obj))
#define EGEOM(obj)        ((struct S_EGeom *)POINTER(obj))
#define ELIBRARY(obj)     ((struct S_ELibrary *)POINTER(obj))
#define ETECHNOLOGY(obj)  ((struct S_ETechnology *)POINTER(obj))
#define ETOOL(obj)        ((struct S_ETool *)POINTER(obj))
#define ERTNODE(obj)      ((struct S_ERTNode *)POINTER(obj))
#define ENETWORK(obj)     ((struct S_ENetwork *)POINTER(obj))
#define EVIEW(obj)        ((struct S_EView *)POINTER(obj))
#define EWINDOW(obj)      ((struct S_EWindow *)POINTER(obj))
#define EGRAPHICS(obj)    ((struct S_EGraphics *)POINTER(obj))
#define ECONSTRAINT(obj)  ((struct S_EConstraint *)POINTER(obj))
#define EWINDOWFRAME(obj) ((struct S_EWindowFrame *)POINTER(obj))

/* equality routine */
int EElectric_Equal(ELKObject a, ELKObject b)
{
	return(EELECTRIC(a)->handle == EELECTRIC(b)->handle);
}

/* print routines */
int ENodeInst_Print(ELKObject w, ELKObject port, int raw, int depth, int len)
{ Printf(port, x_("#[nodeinst %u]"), ENODEINST(w)->handle);  return(0); }
int ENodeProto_Print(ELKObject w, ELKObject port, int raw, int depth, int len)
{ Printf(port, x_("#[nodeproto %s]"), describenodeproto(ENODEPROTO(w)->handle));  return(0); }
int EPortArcInst_Print(ELKObject w, ELKObject port, int raw, int depth, int len)
{ Printf(port, x_("#[portarcinst %u]"), EPORTARCINST(w)->handle);  return(0); }
int EPortExpInst_Print(ELKObject w, ELKObject port, int raw, int depth, int len)
{ Printf(port, x_("#[portexpinst %u]"), EPORTEXPINST(w)->handle);  return(0); }
int EPortProto_Print(ELKObject w, ELKObject port, int raw, int depth, int len)
{ Printf(port, x_("#[portproto %s]"), EPORTPROTO(w)->handle->protoname);  return(0); }
int EArcInst_Print(ELKObject w, ELKObject port, int raw, int depth, int len)
{ Printf(port, x_("#[arcinst %u]"), EARCINST(w)->handle);  return(0); }
int EArcProto_Print(ELKObject w, ELKObject port, int raw, int depth, int len)
{ Printf(port, x_("#[arcproto %s]"), EARCPROTO(w)->handle->protoname);  return(0); }
int EGeom_Print(ELKObject w, ELKObject port, int raw, int depth, int len)
{ Printf(port, x_("#[geom %u]"), EGEOM(w)->handle);  return(0); }
int ELibrary_Print(ELKObject w, ELKObject port, int raw, int depth, int len)
{ Printf(port, x_("#[library %s]"), ELIBRARY(w)->handle->libname);  return(0); }
int ETechnology_Print(ELKObject w, ELKObject port, int raw, int depth, int len)
{ Printf(port, x_("#[technology %s]"), ETECHNOLOGY(w)->handle->techname);  return(0); }
int ETool_Print(ELKObject w, ELKObject port, int raw, int depth, int len)
{ Printf(port, x_("#[tool %s]"), ETOOL(w)->handle->toolname);  return(0); }
int ERTNode_Print(ELKObject w, ELKObject port, int raw, int depth, int len)
{ Printf(port, x_("#[r-tree %u]"), ERTNODE(w)->handle);  return(0); }
int ENetwork_Print(ELKObject w, ELKObject port, int raw, int depth, int len)
{ Printf(port, x_("#[network %u]"), ENETWORK(w)->handle);  return(0); }
int EView_Print(ELKObject w, ELKObject port, int raw, int depth, int len)
{ Printf(port, x_("#[view %s]"), EVIEW(w)->handle->viewname);  return(0); }
int EWindow_Print(ELKObject w, ELKObject port, int raw, int depth, int len)
{ Printf(port, x_("#[window %u]"), EWINDOW(w)->handle);  return(0); }
int EGraphics_Print(ELKObject w, ELKObject port, int raw, int depth, int len)
{ Printf(port, x_("#[graphics %u]"), EGRAPHICS(w)->handle);  return(0); }
int EConstraint_Print(ELKObject w, ELKObject port, int raw, int depth, int len)
{ Printf(port, x_("#[constraint %s]"), ECONSTRAINT(w)->handle->conname);  return(0); }
int EWindowFrame_Print(ELKObject w, ELKObject port, int raw, int depth, int len)
{ Printf(port, x_("#[window-frame %u]"), EWINDOWFRAME(w)->handle);  return(0); }

/* type query routines */
ELKObject P_ENodeInstP(ELKObject x)    { return(TYPE(x) == T_ENodeInst    ? Trueob : Falseob); }
ELKObject P_ENodeProtoP(ELKObject x)   { return(TYPE(x) == T_ENodeProto   ? Trueob : Falseob); }
ELKObject P_EPortArcInstP(ELKObject x) { return(TYPE(x) == T_EPortArcInst ? Trueob : Falseob); }
ELKObject P_EPortExpInstP(ELKObject x) { return(TYPE(x) == T_EPortExpInst ? Trueob : Falseob); }
ELKObject P_EPortProtoP(ELKObject x)   { return(TYPE(x) == T_EPortProto   ? Trueob : Falseob); }
ELKObject P_EArcInstP(ELKObject x)     { return(TYPE(x) == T_EArcInst     ? Trueob : Falseob); }
ELKObject P_EArcProtoP(ELKObject x)    { return(TYPE(x) == T_EArcProto    ? Trueob : Falseob); }
ELKObject P_EGeomP(ELKObject x)        { return(TYPE(x) == T_EGeom        ? Trueob : Falseob); }
ELKObject P_ELibraryP(ELKObject x)     { return(TYPE(x) == T_ELibrary     ? Trueob : Falseob); }
ELKObject P_ETechnologyP(ELKObject x)  { return(TYPE(x) == T_ETechnology  ? Trueob : Falseob); }
ELKObject P_EToolP(ELKObject x)        { return(TYPE(x) == T_ETool        ? Trueob : Falseob); }
ELKObject P_ERTNodeP(ELKObject x)      { return(TYPE(x) == T_ERTNode      ? Trueob : Falseob); }
ELKObject P_ENetworkP(ELKObject x)     { return(TYPE(x) == T_ENetwork     ? Trueob : Falseob); }
ELKObject P_EViewP(ELKObject x)        { return(TYPE(x) == T_EView        ? Trueob : Falseob); }
ELKObject P_EWindowP(ELKObject x)      { return(TYPE(x) == T_EWindow      ? Trueob : Falseob); }
ELKObject P_EGraphicsP(ELKObject x)    { return(TYPE(x) == T_EGraphics    ? Trueob : Falseob); }
ELKObject P_EConstraintP(ELKObject x)  { return(TYPE(x) == T_EConstraint  ? Trueob : Falseob); }
ELKObject P_EWindowFrameP(ELKObject x) { return(TYPE(x) == T_EWindowFrame ? Trueob : Falseob); }

ELKObject Make_EElectric(INTBIG obj, INTBIG type)
{
	CHAR *p;
	ELKObject w;

	p = (CHAR *)emalloc(sizeof (struct S_Electric), el_tempcluster);
	SET(w, type, p);
	EELECTRIC(w)->handle = obj;
	return(w);
}

/****************************** UTILITIES ******************************/

void lsp_init(void)
{
	CHAR *av[1], progname[30];

	if (lsp_strportinited) return;
	lsp_strportinited = TRUE;

	/* initialize Lisp (must do it this way to give it stack base info) */
	estrcpy(progname, x_("electric"));
	av[0] = progname;
	Elk_Init (1, av, 0, x_("toplevel.scm"));
	init_lib_electric();

	/* load the top-level interpreter */
/*	(void)General_Load(Make_String("toplevel.scm", 12), The_Environment); */

	/* create the dummy string port */
	lsp_strport = Make_Port(0, (FILE *)0, Make_String ((CHAR *)0, 0));
	Global_GC_Link(lsp_strport);
}

/*
 * routine to convert a "C" string into a lisp ELKObject
 */
ELKObject lsp_makeobject(CHAR *str)
{
	ELKObject ret;
	REGISTER int c, konst = 1;
	ELKObject Read_Sequence (ELKObject port, int vec, int konst);
	ELKObject Read_Atom (ELKObject port, int konst);
	int String_Getc(ELKObject port);
	int Skip_Comment(ELKObject port);
	int String_Ungetc(ELKObject port, int c);

	/* place the string in the port */
	PORT(lsp_strport)->ptr = 0;
	PORT(lsp_strport)->name = Make_String(str, estrlen(str));
	PORT(lsp_strport)->flags = P_STRING|P_INPUT|P_OPEN;

	/* read from the string port into an object */
	ret = Eof;
	for(;;)
	{
		c = String_Getc(lsp_strport);
		if (c == EOF) break;
		if (Whitespace(c)) continue;
		if (c == ';')
		{
			if (Skip_Comment(lsp_strport) == EOF) break;
			continue;
		}
		if (c == '(')
		{
			ret = Read_Sequence(lsp_strport, 0, konst);
		} else
		{
			String_Ungetc(lsp_strport, c);
			ret = Read_Atom(lsp_strport, konst);
		}
		break;
	}

	/* free the string pointer */
	PORT(lsp_strport)->name = Null;
	return(ret);
}

/*
 * routine to convert Lisp ELKObject "obj" into an Electric address of type "type" and return
 * it in "retval".  Returns true on error.
 */
BOOLEAN lsp_describeobject(ELKObject obj, INTBIG type, INTBIG *retval)
{
	static CHAR retstr[100];
	static INTBIG retarray[1];
	INTBIG len;
	INTBIG t;
	ELKObject strobj;
	CHAR *str, *p;

	t = TYPE(obj);
	switch (type&VTYPE)
	{
		case VINTEGER:
		case VSHORT:
		case VBOOLEAN:
		case VADDRESS:
		case VFRACT:
			switch (t)
			{
				case T_Fixnum:
				case T_Boolean:
					*retval = FIXNUM(obj);
					break;
				case T_Bignum:
					PORT(lsp_strport)->ptr = 0;
					PORT(lsp_strport)->name = Make_String(0, 0);
					PORT(lsp_strport)->flags = P_STRING|P_OPEN;
					Print_Bignum(lsp_strport, obj);
					strobj = PORT(lsp_strport)->name;
					str = STRING(strobj)->data;
					len = PORT(lsp_strport)->ptr;
					(void)estrncpy(retstr, str, len);
					retstr[len] = 0;
					*retval = myatoi(retstr);
					break;
				case T_Flonum:
					*retval = (INTBIG)FLONUM(obj)->val;
					break;
				default:
					return(TRUE);
			}
			if ((type&VTYPE) == VFRACT) *retval *= WHOLE;
			break;

		case VFLOAT:
		case VDOUBLE:
			switch (t)
			{
				case T_Fixnum:
				case T_Boolean:
					*retval = castint((float)FIXNUM(obj));
					break;
				case T_Bignum:
					PORT(lsp_strport)->ptr = 0;
					PORT(lsp_strport)->name = Make_String(0, 0);
					PORT(lsp_strport)->flags = P_STRING|P_OPEN;
					Print_Bignum(lsp_strport, obj);
					strobj = PORT(lsp_strport)->name;
					str = STRING(strobj)->data;
					len = PORT(lsp_strport)->ptr;
					(void)estrncpy(retstr, str, len);
					retstr[len] = 0;
					*retval = castint((float)eatof(retstr));
					break;
				case T_Flonum:
					*retval = castint((float)FLONUM(obj)->val);
					break;
				default:
					return(TRUE);
			}
			break;

		case VCHAR:
			if (t != T_Character) return(TRUE);
			*retval = ELKCHAR(obj);
			break;

		case VSTRING:
			*retval = (INTBIG)x_("");
			switch (t)
			{
				case T_Null:
					*retval = (INTBIG)x_("()");
					break;
				case T_Fixnum:
					(void)esnprintf(retstr, 100, x_("%d"), (int)FIXNUM(obj));
					*retval = (INTBIG)retstr;
					break;
				case T_Bignum:
					PORT(lsp_strport)->ptr = 0;
					PORT(lsp_strport)->name = Make_String(0, 0);
					PORT(lsp_strport)->flags = P_STRING|P_OPEN;
					Print_Bignum(lsp_strport, obj);
					strobj = PORT(lsp_strport)->name;
					str = STRING(strobj)->data;
					len = PORT(lsp_strport)->ptr;
					(void)estrncpy(retstr, str, len);
					retstr[len] = 0;
					*retval = (INTBIG)retstr;
					break;
				case T_Flonum:
					(void)esnprintf(retstr, 100, x_("%g"), FLONUM(obj)->val);
					*retval = (INTBIG)retstr;
					break;
				case T_Boolean:
					if (FIXNUM(obj) != 0) *retval = (INTBIG)x_("t"); else
						*retval = (INTBIG)x_("f");
					break;
				case T_Unbound:
					*retval = (INTBIG)x_("#[unbound]");
					break;
				case T_Special:
					*retval = (INTBIG)x_("#[special]");
					break;
				case T_Character:
					retstr[0] = ELKCHAR(obj);
					retstr[1] = 0;
					*retval = (INTBIG)retstr;
					break;
				case T_Symbol:
					strobj = SYMBOL(obj)->name;
					str = STRING(strobj)->data;
					len = STRING(strobj)->size;
					(void)estrncpy(retstr, str, len);
					retstr[len] = 0;
					*retval = (INTBIG)retstr;
					break;
				case T_Pair:
					PORT(lsp_strport)->ptr = 0;
					PORT(lsp_strport)->name = Make_String(0, 0);
					PORT(lsp_strport)->flags = P_STRING|P_OPEN;
					Pr_List(lsp_strport, obj, 1, DEF_PRINT_DEPTH, DEF_PRINT_LEN);
					strobj = PORT(lsp_strport)->name;
					str = STRING(strobj)->data;
					len = PORT(lsp_strport)->ptr;
					(void)estrncpy(retstr, str, len);
					retstr[len] = 0;
					*retval = (INTBIG)retstr;
					break;
				case T_Environment:
					(void)esnprintf(retstr, 100, x_("#[environment %u]"), (unsigned int)POINTER(obj));
					*retval = (INTBIG)retstr;
					break;
				case T_String:
					str = STRING(obj)->data;
					len = STRING(obj)->size;
					(void)estrncpy(retstr, str, len);
					retstr[len] = 0;
					*retval = (INTBIG)retstr;
					break;
				case T_Vector:
					PORT(lsp_strport)->ptr = 0;
					PORT(lsp_strport)->name = Make_String(0, 0);
					PORT(lsp_strport)->flags = P_STRING|P_OPEN;
					Pr_Vector(lsp_strport, obj, 1, DEF_PRINT_DEPTH, DEF_PRINT_LEN);
					strobj = PORT(lsp_strport)->name;
					str = STRING(strobj)->data;
					len = PORT(lsp_strport)->ptr;
					(void)estrncpy(retstr, str, len);
					retstr[len] = 0;
					*retval = (INTBIG)retstr;
					break;
				case T_Primitive:
					(void)esnprintf(retstr, 100, x_("#[primitive %s]"), PRIM(obj)->name);
					*retval = (INTBIG)retstr;
					break;
				case T_Compound:
					if (Nullp(COMPOUND(obj)->name))
					{
						(void)esnprintf(retstr, 100, x_("#[compound %u]"), (unsigned int)POINTER(obj));
						*retval = (INTBIG)retstr;
						break;
					}
					PORT(lsp_strport)->ptr = 0;
					PORT(lsp_strport)->name = Make_String(0, 0);
					PORT(lsp_strport)->flags = P_STRING|P_OPEN;
					Printf(lsp_strport, x_("#[compound "));
					Print_Object(COMPOUND(obj)->name, lsp_strport, 1, DEF_PRINT_DEPTH,
						DEF_PRINT_LEN);
					Print_Char(lsp_strport, ']');
					strobj = PORT(lsp_strport)->name;
					str = STRING(strobj)->data;
					len = PORT(lsp_strport)->ptr;
					(void)estrncpy(retstr, str, len);
					retstr[len] = 0;
					*retval = (INTBIG)retstr;
					break;
				case T_Control_Point:
					(void)esnprintf(retstr, 100, x_("#[control-point %u]"), (unsigned int)POINTER(obj));
					*retval = (INTBIG)retstr;
					break;
				case T_Promise:
					(void)esnprintf(retstr, 100, x_("#[promise %u]"), (unsigned int)POINTER(obj));
					*retval = (INTBIG)retstr;
					break;
				case T_Port:
					switch (PORT(obj)->flags & (P_INPUT|P_BIDIR))
					{
						case 0:       p = x_("output");       break;
						case P_INPUT: p = x_("input");        break;
						default:      p = x_("input-output"); break;
					}
					if ((PORT(obj)->flags&P_STRING) != 0)
						(void)esnprintf(retstr, 100, x_("#[string-%s-port %u]"), p,
							(unsigned int)POINTER(obj)); else
					{
						strobj = PORT(obj)->name;
						str = STRING(strobj)->data;
						len = STRING(strobj)->size;
						(void)esnprintf(retstr, 100, x_("#[file-%s-port "), p);
						(void)estrncat(retstr, str, len);
						(void)estrcat(retstr, x_("]"));
					}
					*retval = (INTBIG)retstr;
					break;
				case T_End_Of_File:
					*retval = (INTBIG)x_("#[end-of-file]");
					break;
				case T_Autoload:
					PORT(lsp_strport)->ptr = 0;
					PORT(lsp_strport)->name = Make_String(0, 0);
					PORT(lsp_strport)->flags = P_STRING|P_OPEN;
					Printf(lsp_strport, x_("#[autoload "));
					Print_Object(AUTOLOAD(obj)->files, lsp_strport, 1, DEF_PRINT_DEPTH,
						DEF_PRINT_LEN);
					Print_Char(lsp_strport, ']');
					strobj = PORT(lsp_strport)->name;
					str = STRING(strobj)->data;
					len = PORT(lsp_strport)->ptr;
					(void)estrncpy(retstr, str, len);
					retstr[len] = 0;
					*retval = (INTBIG)retstr;
					break;
				case T_Macro:
					if (Nullp(MACRO(obj)->name))
					{
						(void)esnprintf(retstr, 100, x_("#[macro %u]"), (unsigned int)POINTER(obj));
						*retval = (INTBIG)retstr;
						break;
					}
					PORT(lsp_strport)->ptr = 0;
					PORT(lsp_strport)->name = Make_String(0, 0);
					PORT(lsp_strport)->flags = P_STRING|P_OPEN;
					Printf(lsp_strport, x_("#[macro "));
					Print_Object(MACRO(obj)->name, lsp_strport, 1, DEF_PRINT_DEPTH,
						DEF_PRINT_LEN);
					Print_Char(lsp_strport, ']');
					strobj = PORT(lsp_strport)->name;
					str = STRING(strobj)->data;
					len = PORT(lsp_strport)->ptr;
					(void)estrncpy(retstr, str, len);
					retstr[len] = 0;
					*retval = (INTBIG)retstr;
					break;
				case T_Broken_Heart:
					*retval = (INTBIG)x_("!!broken-heart!!");
					break;
				default:
					if (t < 0) break;
					if (!Types[t].name) break;
					PORT(lsp_strport)->ptr = 0;
					PORT(lsp_strport)->name = Make_String(0, 0);
					PORT(lsp_strport)->flags = P_STRING|P_OPEN;
					(*Types[t].print)(obj, lsp_strport, 1, DEF_PRINT_DEPTH, DEF_PRINT_LEN);
					strobj = PORT(lsp_strport)->name;
					str = STRING(strobj)->data;
					len = PORT(lsp_strport)->ptr;
					(void)estrncpy(retstr, str, len);
					retstr[len] = 0;
					*retval = (INTBIG)retstr;
					break;
			}
			break;

		default:
			return(TRUE);
	}

	if ((type&VISARRAY) != 0)
	{
		retarray[0] = *retval;
		*retval = (INTBIG)retarray;
	}
	return(FALSE);
}

/*
 * routine to convert a numeric Lisp ELKObject into an integer.  Returns true on error
 */
BOOLEAN lsp_getnumericobject(ELKObject obj, INTBIG *val)
{
	switch (TYPE(obj))
	{
		case T_Fixnum: *val = FIXNUM(obj);              return(FALSE);
		case T_Bignum: *val = Bignum_To_Integer(obj);   return(FALSE);
		case T_Flonum: *val = (INTBIG)FLONUM(obj)->val; return(FALSE);
		case T_Symbol:
			if (EQ(obj, lsp_displayablesym)) { *val = VDISPLAY;   return(FALSE); }
			break;
	}
	return(TRUE);
}

/*
 * routine to convert a numeric Lisp ELKObject into a string.  Returns zero on error
 */
CHAR *lsp_getstringobject(ELKObject obj)
{
	static CHAR retval[256];
	CHAR *str;
	INTBIG len;

	switch (TYPE(obj))
	{
		case T_Symbol:
			obj = SYMBOL(obj)->name;
			/* FALLTHROUGH */ 
		case T_String:
			str = STRING(obj)->data;
			len = STRING(obj)->size;
			(void)estrncpy(retval, str, len);
			retval[len] = 0;
			return(retval);
	}
	return(0);
}


/*
 * routine to convert a Lisp ELKObject into an Electric addr/type pair
 */
void lsp_getaddrandtype(ELKObject oaddr, INTBIG *addr, INTBIG *type)
{
	INTBIG otype;
	static CHAR retval[256];
	CHAR *str;
	INTBIG len;

	*type = VUNKNOWN;
	otype = TYPE(oaddr);
	if (otype == T_Fixnum)
	{
		*type = VINTEGER;
		*addr = FIXNUM(oaddr);
		return;
	}
	if (otype == T_Bignum)
	{
		*type = VINTEGER;
		*addr = Bignum_To_Integer(oaddr);
		return;
	}
	if (otype == T_Flonum)
	{
		*type = VFLOAT;
		*addr = castint((float)FLONUM(oaddr)->val);
		return;
	}
	if (otype == T_String)
	{
		*type = VSTRING;
		str = STRING(oaddr)->data;
		len = STRING(oaddr)->size;
		(void)estrncpy(retval, str, len);
		retval[len] = 0;
		*addr = (INTBIG)retval;
		return;
	}
	if (otype == T_ENodeInst)    *type = VNODEINST; else
	if (otype == T_ENodeProto)   *type = VNODEPROTO; else
	if (otype == T_EPortArcInst) *type = VPORTARCINST; else
	if (otype == T_EPortExpInst) *type = VPORTEXPINST; else
	if (otype == T_EPortProto)   *type = VPORTPROTO; else
	if (otype == T_EArcInst)     *type = VARCINST; else
	if (otype == T_EArcProto)    *type = VARCPROTO; else
	if (otype == T_EGeom)        *type = VGEOM; else
	if (otype == T_ELibrary)     *type = VLIBRARY; else
	if (otype == T_ETechnology)  *type = VTECHNOLOGY; else
	if (otype == T_ETool)        *type = VTOOL; else
	if (otype == T_ERTNode)      *type = VRTNODE; else
	if (otype == T_ENetwork)     *type = VNETWORK; else
	if (otype == T_EView)        *type = VVIEW; else
	if (otype == T_EWindow)      *type = VWINDOWPART; else
	if (otype == T_EGraphics)    *type = VGRAPHICS; else
	if (otype == T_EConstraint)  *type = VCONSTRAINT; else
	if (otype == T_EWindowFrame) *type = VWINDOWFRAME; else
		return;
	*addr = EELECTRIC(oaddr)->handle;
}

ELKObject lsp_makevarobject(INTBIG type, INTBIG addr)
{
	ELKObject ret;

	/* convert back to a Lisp object */
	switch (type&VTYPE)
	{
		case VINTEGER:     return(Make_Integer(addr));
		case VSHORT:
		case VBOOLEAN:     return(Make_Fixnum(addr));
		case VADDRESS:     return(Make_Unsigned(addr));
/*	case VCHAR: */        /* character variable */
/*	case VFRACT: */       /* fractional integer (scaled by WHOLE) */
		case VSTRING:      return(Make_String((CHAR *)addr, estrlen((CHAR *)addr)));
		case VFLOAT:
		case VDOUBLE:      return(Make_Reduced_Flonum((double)castfloat(addr)));
		case VNODEINST:
			ret = Make_EElectric(addr, T_ENodeInst);
			if (ENODEINST(ret)->handle == NONODEINST) return(Null); else return(ret);
		case VNODEPROTO:
			ret = Make_EElectric(addr, T_ENodeProto);
			if (ENODEPROTO(ret)->handle == NONODEPROTO) return(Null); else return(ret);
		case VPORTARCINST:
			ret = Make_EElectric(addr, T_EPortArcInst);
			if (EPORTARCINST(ret)->handle == NOPORTARCINST) return(Null); else return(ret);
		case VPORTEXPINST:
			ret = Make_EElectric(addr, T_EPortExpInst);
			if (EPORTEXPINST(ret)->handle == NOPORTEXPINST) return(Null); else return(ret);
		case VPORTPROTO:
			ret = Make_EElectric(addr, T_EPortProto);
			if (EPORTPROTO(ret)->handle == NOPORTPROTO) return(Null); else return(ret);
		case VARCINST:
			ret = Make_EElectric(addr, T_EArcInst);
			if (EARCINST(ret)->handle == NOARCINST) return(Null); else return(ret);
		case VARCPROTO:
			ret = Make_EElectric(addr, T_EArcProto);
			if (EARCPROTO(ret)->handle == NOARCPROTO) return(Null); else return(ret);
		case VGEOM:
			ret = Make_EElectric(addr, T_EGeom);
			if (EGEOM(ret)->handle == NOGEOM) return(Null); else return(ret);
		case VLIBRARY:
			ret = Make_EElectric(addr, T_ELibrary);
			if (ELIBRARY(ret)->handle == NOLIBRARY) return(Null); else return(ret);
		case VTECHNOLOGY:
			ret = Make_EElectric(addr, T_ETechnology);
			if (ETECHNOLOGY(ret)->handle == NOTECHNOLOGY) return(Null); else return(ret);
		case VTOOL:
			ret = Make_EElectric(addr, T_ETool);
			if (ETOOL(ret)->handle == NOTOOL) return(Null); else return(ret);
		case VRTNODE:
			ret = Make_EElectric(addr, T_ERTNode);
			if (ERTNODE(ret)->handle == NORTNODE) return(Null); else return(ret);
		case VNETWORK:
			ret = Make_EElectric(addr, T_ENetwork);
			if (ENETWORK(ret)->handle == NONETWORK) return(Null); else return(ret);
		case VVIEW:
			ret = Make_EElectric(addr, T_EView);
			if (EVIEW(ret)->handle == NOVIEW) return(Null); else return(ret);
		case VWINDOWPART:
			ret = Make_EElectric(addr, T_EWindow);
			if (EWINDOW(ret)->handle == NOWINDOWPART) return(Null); else return(ret);
		case VGRAPHICS:
			ret = Make_EElectric(addr, T_EGraphics);
			if (EGRAPHICS(ret)->handle == NOGRAPHICS) return(Null); else return(ret);
		case VCONSTRAINT:
			ret = Make_EElectric(addr, T_EConstraint);
			if (ECONSTRAINT(ret)->handle == NOCONSTRAINT) return(Null); else return(ret);
		case VWINDOWFRAME:
			ret = Make_EElectric(addr, T_EWindowFrame);
			if (EWINDOWFRAME(ret)->handle == NOWINDOWFRAME) return(Null); else return(ret);
	}
	return(Null);
}

/************************* DATABASE EXAMINATION ROUTINES *************************/

ELKObject lsp_curlib(void)
{
	return(Make_EElectric((INTBIG)el_curlib, T_ELibrary));
}

ELKObject lsp_curtech(void)
{
	return(Make_EElectric((INTBIG)el_curtech, T_ETechnology));
}

ELKObject lsp_getval(ELKObject oaddr, ELKObject oattr)
{
	INTBIG type, addr, len, i;
	CHAR *name;
	ELKObject v;
	VARIABLE *var;
	GC_Node;

	/* get inputs from LISP */
	lsp_getaddrandtype(oaddr, &addr, &type);
	if (type == VUNKNOWN) return(Null);
	name = lsp_getstringobject(oattr);
	if (name == 0) return(Null);

	/* get the variable */
	var = getval(addr, type, -1, name);
	if (var == NOVARIABLE) return(Null);
	if ((var->type&VISARRAY) == 0)
		return(lsp_makevarobject(var->type, var->addr));
	len = getlength(var);
	v = Make_Vector(len, Null);
	GC_Link(v);
	for(i=0; i<len; i++)
		VECTOR(v)->data[i] = lsp_makevarobject(var->type, ((INTBIG *)var->addr)[i]);
	GC_Unlink;
	return(v);
}

ELKObject lsp_getparentval(ELKObject oname, ELKObject odef, ELKObject oheight)
{
	INTBIG height;
	CHAR *name;

	/* get inputs from LISP */
	name = lsp_getstringobject(oname);
	if (name == 0) return(Null);
	if (lsp_getnumericobject(oheight, &height)) return(Null);

	/* call common evaluation code */
	return(lsp_dogetparentval(name, odef, height));
}

ELKObject lsp_dogetparentval(CHAR *name, ELKObject odef, INTBIG height)
{
	INTBIG len, i;
	ELKObject v;
	VARIABLE *var;
	GC_Node;

	/* get the variable */
	var = getparentval(name, height);
	if (var == NOVARIABLE) return(odef);
	if ((var->type&VISARRAY) == 0)
		return(lsp_makevarobject(var->type, var->addr));
	len = getlength(var);
	v = Make_Vector(len, Null);
	GC_Link(v);
	for(i=0; i<len; i++)
		VECTOR(v)->data[i] = lsp_makevarobject(var->type, ((INTBIG *)var->addr)[i]);
	GC_Unlink;
	return(v);
}

ELKObject lsp_P(ELKObject oname)
{
	CHAR *name, fullname[300];

	/* get inputs from LISP */
	name = lsp_getstringobject(oname);
	if (name == 0) return(Null);
	esnprintf(fullname, 300, x_("ATTR_%s"), name);

	/* call common evaluation code */
	return(lsp_dogetparentval(fullname, Null, 1));
}

ELKObject lsp_PD(ELKObject oname, ELKObject odef)
{
	CHAR *name, fullname[300];

	/* get inputs from LISP */
	name = lsp_getstringobject(oname);
	if (name == 0) return(Null);
	esnprintf(fullname, 300, x_("ATTR_%s"), name);

	/* call common evaluation code */
	return(lsp_dogetparentval(fullname, odef, 1));
}

ELKObject lsp_PAR(ELKObject oname)
{
	CHAR *name, fullname[300];

	/* get inputs from LISP */
	name = lsp_getstringobject(oname);
	if (name == 0) return(Null);
	esnprintf(fullname, 300, x_("ATTR_%s"), name);

	/* call common evaluation code */
	return(lsp_dogetparentval(fullname, Null, 0));
}

ELKObject lsp_PARD(ELKObject oname, ELKObject odef)
{
	CHAR *name, fullname[300];

	/* get inputs from LISP */
	name = lsp_getstringobject(oname);
	if (name == 0) return(Null);
	esnprintf(fullname, 300, x_("ATTR_%s"), name);

	/* call common evaluation code */
	return(lsp_dogetparentval(fullname, odef, 0));
}

ELKObject lsp_setval(ELKObject oaddr, ELKObject oname, ELKObject onaddr, ELKObject ontypebits)
{
	INTBIG type, addr, len, i, ntype, naddr, ntypebits, lasttype, thisaddr, ifloat;
	CHAR *name;
	VARIABLE *var;
	float f;

	/* get inputs from LISP */
	lsp_getaddrandtype(oaddr, &addr, &type);
	if (type == VUNKNOWN) return(Null);
	name = lsp_getstringobject(oname);
	if (name == 0) return(Null);
	if (TYPE(onaddr) == T_Vector)
	{
		/* setting an array */
		len = VECTOR(onaddr)->size;
		naddr = (INTBIG)emalloc(len * SIZEOFINTBIG, el_tempcluster);
		if (naddr == 0) return(Null);
		for(i=0; i<len; i++)
		{
			lsp_getaddrandtype(VECTOR(onaddr)->data[i], &thisaddr, &ntype);
			if (ntype == VSTRING)
				(void)allocstring(&((CHAR **)naddr)[i], (CHAR *)thisaddr,
					el_tempcluster); else
						((INTBIG *)naddr)[i] = thisaddr;

			/* LINTED "lasttype" used in proper order */
			if (i != 0 && lasttype == VINTEGER && ntype == VFLOAT)
			{
				f = castfloat(thisaddr);
				ifloat = (INTBIG)f;
				if (ifloat == (INTBIG)f)
				{
					((INTBIG *)naddr)[i] = ifloat;
					ntype = VINTEGER;
				}
			}
			if (i != 0 && lasttype == VFLOAT && ntype == VINTEGER)
			{
				f = castfloat(((INTBIG *)naddr)[i-1]);
				ifloat = (INTBIG)f;
				if (ifloat == (INTBIG)f)
				{
					((INTBIG *)naddr)[i-1] = ifloat;
					lasttype = VINTEGER;
				}
			}
			if (i != 0 && ntype != lasttype)
			{
				ttyputerr(_("Inconsistent type in array"));
				return(Null);
			}
			lasttype = ntype;
		}
		ntype |= VISARRAY | (len << VLENGTHSH);
	} else
	{
		/* setting a scalar */
		lsp_getaddrandtype(onaddr, &naddr, &ntype);
		if (ntype == VUNKNOWN) return(Null);
	}
	if (lsp_getnumericobject(ontypebits, &ntypebits)) return(Null);
	ntype |= ntypebits;

	/* set the variable */
	var = setval(addr, type, name, naddr, ntype);
	if ((ntype&VISARRAY) != 0)
	{
		if ((ntype&VTYPE) == VSTRING)
			for(i=0; i<len; i++) efree(((CHAR **)naddr)[i]);
		efree((CHAR *)naddr);
	}
	return(Make_Fixnum(var != NOVARIABLE));
}

ELKObject lsp_setind(ELKObject oaddr, ELKObject oname, ELKObject oindex, ELKObject onaddr)
{
	INTBIG type, addr, ntype, naddr, aindex;
	CHAR *name;

	/* get inputs from LISP */
	lsp_getaddrandtype(oaddr, &addr, &type);
	if (type == VUNKNOWN) return(Null);
	name = lsp_getstringobject(oname);
	if (name == 0) return(Null);
	if (lsp_getnumericobject(oindex, &aindex)) return(Null);
	lsp_getaddrandtype(onaddr, &naddr, &ntype);
	if (ntype == VUNKNOWN) return(Null);

	/* set the variable */
	return(Make_Fixnum(setind(addr, type, name, aindex, naddr)));
}

ELKObject lsp_delval(ELKObject oaddr, ELKObject oname)
{
	INTBIG type, addr;
	CHAR *name;

	/* get inputs from LISP */
	lsp_getaddrandtype(oaddr, &addr, &type);
	if (type == VUNKNOWN) return(Null);
	name = lsp_getstringobject(oname);
	if (name == 0) return(Null);

	/* delete the variable */
	return(Make_Fixnum(delval(addr, type, name)));
}

ELKObject lsp_initsearch(ELKObject olx, ELKObject ohx, ELKObject oly, ELKObject ohy, ELKObject onp)
{
	INTBIG lx, hx, ly, hy, sea;
	REGISTER NODEPROTO *np;

	/* get inputs from LISP */
	if (lsp_getnumericobject(olx, &lx)) return(Null);
	if (lsp_getnumericobject(ohx, &hx)) return(Null);
	if (lsp_getnumericobject(oly, &ly)) return(Null);
	if (lsp_getnumericobject(ohy, &hy)) return(Null);
	Ensure_Type(onp, T_ENodeProto);   np = ENODEPROTO(onp)->handle;

	sea = initsearch(lx, hx, ly, hy, np);
	if (sea == -1) return(Null);
	return(Make_Integer(sea));
}

ELKObject lsp_nextobject(ELKObject osea)
{
	INTBIG sea;
	REGISTER GEOM *g;

	/* get inputs from LISP */
	if (lsp_getnumericobject(osea, &sea)) return(Null);

	g = nextobject(sea);
	if (g == NOGEOM) return(Null);
	return(Make_EElectric((INTBIG)g, T_EGeom));
}

/****************************** TOOL ROUTINES ******************************/

ELKObject lsp_gettool(ELKObject oname)
{
	CHAR *name;
	REGISTER TOOL *tool;

	/* get inputs from LISP */
	name = lsp_getstringobject(oname);
	if (name == 0) return(Null);

	tool = gettool(name);
	if (tool == NOTOOL) return(Null);
	return(Make_EElectric((INTBIG)tool, T_ETool));
}

ELKObject lsp_maxtool(void)
{
	return(Make_Fixnum(el_maxtools));
}

ELKObject lsp_indextool(ELKObject oindex)
{
	INTBIG aindex;

	/* get inputs from LISP */
	if (lsp_getnumericobject(oindex, &aindex)) return(Null);

	if (aindex < 0 || aindex >= el_maxtools) return(Null);
	return(Make_EElectric((INTBIG)&el_tools[aindex], T_ETool));
}

ELKObject lsp_toolturnon(ELKObject otool)
{
	REGISTER TOOL *tool;

	/* get inputs from LISP */
	Ensure_Type(otool, T_ETool);   tool = ETOOL(otool)->handle;

	toolturnon(tool);
	return(Null);
}

ELKObject lsp_toolturnoff(ELKObject otool)
{
	REGISTER TOOL *tool;

	/* get inputs from LISP */
	Ensure_Type(otool, T_ETool);   tool = ETOOL(otool)->handle;

	toolturnoff(tool, TRUE);
	return(Null);
}

/****************************** LIBRARY ROUTINES ******************************/

ELKObject lsp_getlibrary(ELKObject oname)
{
	CHAR *name;
	REGISTER LIBRARY *lib;

	/* get inputs from LISP */
	name = lsp_getstringobject(oname);
	if (name == 0) return(Null);

	lib = getlibrary(name);
	if (lib == NOLIBRARY) return(Null);
	return(Make_EElectric((INTBIG)lib, T_ELibrary));
}

ELKObject lsp_newlibrary(ELKObject oname, ELKObject ofile)
{
	CHAR *name, *file;
	REGISTER LIBRARY *lib;

	/* get inputs from LISP */
	name = lsp_getstringobject(oname);
	if (name == 0) return(Null);
	file = lsp_getstringobject(ofile);
	if (file == 0) return(Null);

	lib = newlibrary(name, file);
	if (lib == NOLIBRARY) return(Null);
	return(Make_EElectric((INTBIG)lib, T_ELibrary));
}

ELKObject lsp_killlibrary(ELKObject olib)
{
	REGISTER LIBRARY *lib;

	/* get inputs from LISP */
	Ensure_Type(olib, T_ELibrary);   lib = ELIBRARY(olib)->handle;

	killlibrary(lib);
	return(Null);
}

ELKObject lsp_eraselibrary(ELKObject olib)
{
	REGISTER LIBRARY *lib;

	/* get inputs from LISP */
	Ensure_Type(olib, T_ELibrary);   lib = ELIBRARY(olib)->handle;

	eraselibrary(lib);
	return(Null);
}

ELKObject lsp_selectlibrary(ELKObject olib)
{
	REGISTER LIBRARY *lib;

	/* get inputs from LISP */
	Ensure_Type(olib, T_ELibrary);   lib = ELIBRARY(olib)->handle;

	selectlibrary(lib, TRUE);
	return(Null);
}

/****************************** NODEPROTO ROUTINES ******************************/

ELKObject lsp_getnodeproto(ELKObject oname)
{
	CHAR *name;
	REGISTER NODEPROTO *np;

	/* get inputs from LISP */
	name = lsp_getstringobject(oname);
	if (name == 0) return(Null);

	np = getnodeproto(name);
	if (np == NONODEPROTO) return(Null);
	return(Make_EElectric((INTBIG)np, T_ENodeProto));
}

ELKObject lsp_newnodeproto(ELKObject oname, ELKObject olib)
{
	CHAR *name;
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;

	/* get inputs from LISP */
	Ensure_Type(olib, T_ELibrary);   lib = ELIBRARY(olib)->handle;
	name = lsp_getstringobject(oname);
	if (name == 0) return(Null);

	np = newnodeproto(name, lib);
	if (np == NONODEPROTO) return(Null);
	return(Make_EElectric((INTBIG)np, T_ENodeProto));
}

ELKObject lsp_killnodeproto(ELKObject onp)
{
	REGISTER NODEPROTO *np;

	/* get inputs from LISP */
	Ensure_Type(onp, T_ENodeProto);   np = ENODEPROTO(onp)->handle;

	return(Make_Fixnum(killnodeproto(np)?1:0));
}

ELKObject lsp_copynodeproto(ELKObject onp, ELKObject otlib, ELKObject otname)
{
	CHAR *tname;
	REGISTER LIBRARY *tlib;
	REGISTER NODEPROTO *np, *nnp;

	/* get inputs from LISP */
	Ensure_Type(otlib, T_ELibrary);   tlib = ELIBRARY(otlib)->handle;
	Ensure_Type(onp, T_ENodeProto);   np = ENODEPROTO(onp)->handle;
	tname = lsp_getstringobject(otname);
	if (tname == 0) return(Null);

	nnp = copynodeproto(np, tlib, tname, FALSE);
	if (nnp == NONODEPROTO) return(Null);
	return(Make_EElectric((INTBIG)nnp, T_ENodeProto));
}

ELKObject lsp_iconview(ELKObject onp)
{
	REGISTER NODEPROTO *np, *inp;

	/* get inputs from LISP */
	Ensure_Type(onp, T_ENodeProto);   np = ENODEPROTO(onp)->handle;

	inp = iconview(np);
	if (inp == NONODEPROTO) return(Null);
	return(Make_EElectric((INTBIG)inp, T_ENodeProto));
}

ELKObject lsp_contentsview(ELKObject onp)
{
	REGISTER NODEPROTO *np, *cnp;

	/* get inputs from LISP */
	Ensure_Type(onp, T_ENodeProto);   np = ENODEPROTO(onp)->handle;

	cnp = contentsview(np);
	if (cnp == NONODEPROTO) return(Null);
	return(Make_EElectric((INTBIG)cnp, T_ENodeProto));
}

/****************************** NODEINST ROUTINES ******************************/

ELKObject lsp_newnodeinst(ELKObject opro, ELKObject olx, ELKObject ohx, ELKObject oly, ELKObject ohy,
	ELKObject otr, ELKObject orot, ELKObject onp)
{
	REGISTER NODEPROTO *pro, *np;
	INTBIG lx, hx, ly, hy, tr, rot;
	REGISTER NODEINST *ni;

	/* get inputs from LISP */
	Ensure_Type(opro, T_ENodeProto);   pro = ENODEPROTO(opro)->handle;
	Ensure_Type(onp, T_ENodeProto);    np = ENODEPROTO(onp)->handle;
	if (lsp_getnumericobject(olx, &lx)) return(Null);
	if (lsp_getnumericobject(ohx, &hx)) return(Null);
	if (lsp_getnumericobject(oly, &ly)) return(Null);
	if (lsp_getnumericobject(ohy, &hy)) return(Null);
	if (lsp_getnumericobject(otr, &tr)) return(Null);
	if (lsp_getnumericobject(orot, &rot)) return(Null);

	ni = newnodeinst(pro, lx, hx, ly, hy, tr, rot, np);
	if (ni == NONODEINST) return(Null);
	return(Make_EElectric((INTBIG)ni, T_ENodeInst));
}

ELKObject lsp_modifynodeinst(ELKObject oni, ELKObject odlx, ELKObject odly, ELKObject odhx, ELKObject odhy,
	ELKObject odrot, ELKObject odtr)
{
	INTBIG dlx, dly, dhx, dhy, drot, dtr;
	REGISTER NODEINST *ni;

	/* get inputs from LISP */
	Ensure_Type(oni, T_ENodeInst);   ni = ENODEINST(oni)->handle;
	if (lsp_getnumericobject(odlx, &dlx)) return(Null);
	if (lsp_getnumericobject(odly, &dly)) return(Null);
	if (lsp_getnumericobject(odhx, &dhx)) return(Null);
	if (lsp_getnumericobject(odhy, &dhy)) return(Null);
	if (lsp_getnumericobject(odrot, &drot)) return(Null);
	if (lsp_getnumericobject(odtr, &dtr)) return(Null);

	modifynodeinst(ni, dlx, dly, dhx, dhy, drot, dtr);
	return(Null);
}

ELKObject lsp_killnodeinst(ELKObject oni)
{
	REGISTER NODEINST *ni;

	/* get inputs from LISP */
	Ensure_Type(oni, T_ENodeInst);   ni = ENODEINST(oni)->handle;

	return(Make_Fixnum(killnodeinst(ni)));
}

ELKObject lsp_replacenodeinst(ELKObject oni, ELKObject opr)
{
	REGISTER NODEINST *ni, *nni;
	REGISTER NODEPROTO *pr;

	/* get inputs from LISP */
	Ensure_Type(oni, T_ENodeInst);   ni = ENODEINST(oni)->handle;
	Ensure_Type(opr, T_ENodeProto);  pr = ENODEPROTO(opr)->handle;

	nni = replacenodeinst(ni, pr, FALSE, FALSE);
	if (nni == NONODEINST) return(Null);
	return(Make_EElectric((INTBIG)nni, T_ENodeInst));
}

ELKObject lsp_nodefunction(ELKObject oni)
{
	REGISTER NODEINST *ni;

	/* get inputs from LISP */
	Ensure_Type(oni, T_ENodeInst);   ni = ENODEINST(oni)->handle;

	return(Make_Integer(nodefunction(ni)));
}

/****************************** ARCINST ROUTINES ******************************/

ELKObject lsp_newarcinst(ELKObject opro, ELKObject owid, ELKObject obit, ELKObject ona, ELKObject opa,
	ELKObject oxa, ELKObject oya, ELKObject onb, ELKObject opb, ELKObject oxb, ELKObject oyb, ELKObject onp)
{
	REGISTER ARCPROTO *pro;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *na, *nb;
	REGISTER PORTPROTO *pa, *pb;
	INTBIG wid, bit, xa, ya, xb, yb;
	REGISTER ARCINST *ai;

	/* get inputs from LISP */
	Ensure_Type(opro, T_EArcProto);   pro = EARCPROTO(opro)->handle;
	if (lsp_getnumericobject(owid, &wid)) return(Null);
	if (lsp_getnumericobject(obit, &bit)) return(Null);
	Ensure_Type(ona, T_ENodeInst);    na = ENODEINST(ona)->handle;
	Ensure_Type(opa, T_EPortProto);   pa = EPORTPROTO(opa)->handle;
	if (lsp_getnumericobject(oxa, &xa)) return(Null);
	if (lsp_getnumericobject(oya, &ya)) return(Null);
	Ensure_Type(onb, T_ENodeInst);    nb = ENODEINST(onb)->handle;
	Ensure_Type(opb, T_EPortProto);   pb = EPORTPROTO(opb)->handle;
	if (lsp_getnumericobject(oxb, &xb)) return(Null);
	if (lsp_getnumericobject(oyb, &yb)) return(Null);
	Ensure_Type(onp, T_ENodeProto);   np = ENODEPROTO(onp)->handle;

	ai = newarcinst(pro, wid, bit, na, pa, xa, ya, nb, pb, xb, yb, np);
	if (ai == NOARCINST) return(Null);
	return(Make_EElectric((INTBIG)ai, T_EArcInst));
}

ELKObject lsp_modifyarcinst(ELKObject oai, ELKObject odw, ELKObject odx1, ELKObject ody1, ELKObject odx2,
	ELKObject ody2)
{
	INTBIG dw, dx1, dy1, dx2, dy2;
	REGISTER ARCINST *ai;

	/* get inputs from LISP */
	Ensure_Type(oai, T_EArcInst);   ai = EARCINST(oai)->handle;
	if (lsp_getnumericobject(odw, &dw)) return(Null);
	if (lsp_getnumericobject(odx1, &dx1)) return(Null);
	if (lsp_getnumericobject(ody1, &dy1)) return(Null);
	if (lsp_getnumericobject(odx2, &dx2)) return(Null);
	if (lsp_getnumericobject(ody2, &dy2)) return(Null);

	return(Make_Fixnum(modifyarcinst(ai, dw, dx1, dy1, dx2, dy2)?1:0));
}

ELKObject lsp_killarcinst(ELKObject oai)
{
	REGISTER ARCINST *ai;

	/* get inputs from LISP */
	Ensure_Type(oai, T_EArcInst);   ai = EARCINST(oai)->handle;

	return(Make_Fixnum(killarcinst(ai)?1:0));
}

ELKObject lsp_replacearcinst(ELKObject oai, ELKObject opr)
{
	REGISTER ARCINST *ai, *nai;
	REGISTER ARCPROTO *pr;

	/* get inputs from LISP */
	Ensure_Type(oai, T_EArcInst);    ai = EARCINST(oai)->handle;
	Ensure_Type(opr, T_EArcProto);   pr = EARCPROTO(opr)->handle;

	nai = replacearcinst(ai, pr);
	if (nai == NOARCINST) return(Null);
	return(Make_EElectric((INTBIG)nai, T_EArcInst));
}

/****************************** PORTPROTO ROUTINES ******************************/

ELKObject lsp_newportproto(ELKObject onp, ELKObject oni, ELKObject opp, ELKObject oname)
{
	CHAR *name;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER PORTPROTO *pp;

	/* get inputs from LISP */
	Ensure_Type(onp, T_ENodeProto);   np = ENODEPROTO(onp)->handle;
	Ensure_Type(oni, T_ENodeInst);    ni = ENODEINST(oni)->handle;
	Ensure_Type(opp, T_EPortProto);   pp = EPORTPROTO(opp)->handle;
	name = lsp_getstringobject(oname);
	if (name == 0) return(Null);

	pp = newportproto(np, ni, pp, name);
	if (pp == NOPORTPROTO) return(Null);
	return(Make_EElectric((INTBIG)pp, T_EPortProto));
}

ELKObject lsp_portposition(ELKObject oni, ELKObject opp)
{
	REGISTER NODEINST *ni;
	REGISTER PORTPROTO *pp;
	ELKObject v;
	INTBIG x, y;
	GC_Node;

	/* get inputs from LISP */
	Ensure_Type(oni, T_ENodeInst);    ni = ENODEINST(oni)->handle;
	Ensure_Type(opp, T_EPortProto);   pp = EPORTPROTO(opp)->handle;

	portposition(ni, pp, &x, &y);
	v = Make_Vector(2, Null);
	GC_Link(v);
	VECTOR(v)->data[0] = Make_Integer(x);
	VECTOR(v)->data[1] = Make_Integer(y);
	GC_Unlink;
	return(v);
}

ELKObject lsp_getportproto(ELKObject onp, ELKObject oname)
{
	CHAR *name;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;

	/* get inputs from LISP */
	Ensure_Type(onp, T_ENodeProto);   np = ENODEPROTO(onp)->handle;
	name = lsp_getstringobject(oname);
	if (name == 0) return(Null);

	pp = getportproto(np, name);
	if (pp == NOPORTPROTO) return(Null);
	return(Make_EElectric((INTBIG)pp, T_EPortProto));
}

ELKObject lsp_killportproto(ELKObject onp, ELKObject opp)
{
	REGISTER PORTPROTO *pp;
	REGISTER NODEPROTO *np;

	/* get inputs from LISP */
	Ensure_Type(opp, T_EPortProto);   pp = EPORTPROTO(opp)->handle;
	Ensure_Type(onp, T_ENodeProto);   np = ENODEPROTO(onp)->handle;

	return(Make_Fixnum(killportproto(np, pp)?1:0));
}

ELKObject lsp_moveportproto(ELKObject onp, ELKObject oopp, ELKObject onni, ELKObject onpp)
{
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *opp, *npp;
	REGISTER NODEINST *nni;

	/* get inputs from LISP */
	Ensure_Type(onp, T_ENodeProto);    np = ENODEPROTO(onp)->handle;
	Ensure_Type(oopp, T_EPortProto);   opp = EPORTPROTO(oopp)->handle;
	Ensure_Type(onni, T_ENodeInst);    nni = ENODEINST(onni)->handle;
	Ensure_Type(onpp, T_EPortProto);   npp = EPORTPROTO(onpp)->handle;

	return(Make_Fixnum(moveportproto(np, opp, nni, npp)?1:0));
}

/*************************** CHANGE CONTROL ROUTINES ***************************/

ELKObject lsp_undoabatch(void)
{
	INTBIG tool;

	if (undoabatch((TOOL **)&tool) == 0) return(Null);
	return(Make_Fixnum(tool));
}

ELKObject lsp_noundoallowed(void)
{
	noundoallowed();
	return(Null);
}

/****************************** VIEW ROUTINES ******************************/

ELKObject lsp_getview(ELKObject oname)
{
	CHAR *name;
	REGISTER VIEW *v;

	/* get inputs from LISP */
	name = lsp_getstringobject(oname);
	if (name == 0) return(Null);

	v = getview(name);
	if (v == NOVIEW) return(Null);
	return(Make_EElectric((INTBIG)v, T_EView));
}

ELKObject lsp_newview(ELKObject oname, ELKObject osname)
{
	CHAR *name, *sname;
	REGISTER VIEW *v;

	/* get inputs from LISP */
	name = lsp_getstringobject(oname);
	if (name == 0) return(Null);
	sname = lsp_getstringobject(osname);
	if (sname == 0) return(Null);

	v = newview(name, sname);
	if (v == NOVIEW) return(Null);
	return(Make_EElectric((INTBIG)v, T_EView));
}

ELKObject lsp_killview(ELKObject ov)
{
	REGISTER VIEW *v;

	/* get inputs from LISP */
	Ensure_Type(ov, T_EView);   v = EVIEW(ov)->handle;

	return(Make_Fixnum(killview(v)?1:0));
}

/*************************** MISCELLANEOUS ROUTINES ***************************/

ELKObject lsp_telltool(long argc, ELKObject argv[])
{
	REGISTER TOOL *tool;
	REGISTER INTBIG i;
	CHAR *par[20], *ret;

	/* get inputs from LISP */
	if (argc < 1) return(Null);
	Ensure_Type(argv[0], T_ETool);   tool = ETOOL(argv[0])->handle;
	argc--;   argv++;
	for(i=0; i<argc; i++)
	{
		ret = lsp_getstringobject(argv[i]);
		if (ret == 0) return(Null);
		(void)allocstring(&par[i], ret, el_tempcluster);
	}

	telltool(tool, argc, par);
	for(i=0; i<argc; i++) efree(par[i]);
	return(Make_Fixnum(0));		/* !!! actually should not return a value */
}

ELKObject lsp_getarcproto(ELKObject oname)
{
	CHAR *name;
	REGISTER ARCPROTO *ap;

	/* get inputs from LISP */
	name = lsp_getstringobject(oname);
	if (name == 0) return(Null);

	ap = getarcproto(name);
	if (ap == NOARCPROTO) return(Null);
	return(Make_EElectric((INTBIG)ap, T_EArcProto));
}

ELKObject lsp_gettechnology(ELKObject oname)
{
	CHAR *name;
	REGISTER TECHNOLOGY *tech;

	/* get inputs from LISP */
	name = lsp_getstringobject(oname);
	if (name == 0) return(Null);

	tech = gettechnology(name);
	if (tech == NOTECHNOLOGY) return(Null);
	return(Make_EElectric((INTBIG)tech, T_ETechnology));
}

ELKObject lsp_getpinproto(ELKObject oap)
{
	REGISTER ARCPROTO *ap;
	REGISTER NODEPROTO *np;

	/* get inputs from LISP */
	Ensure_Type(oap, T_EArcProto);   ap = EARCPROTO(oap)->handle;

	np = getpinproto(ap);
	if (np == NONODEPROTO) return(Null);
	return(Make_EElectric((INTBIG)np, T_ENodeProto));
}

ELKObject lsp_getnetwork(ELKObject oname, ELKObject onp)
{
	CHAR *name;
	REGISTER NETWORK *net;
	REGISTER NODEPROTO *np;

	/* get inputs from LISP */
	name = lsp_getstringobject(oname);
	if (name == 0) return(Null);
	Ensure_Type(onp, T_ENodeProto);   np = ENODEPROTO(onp)->handle;

	net = getnetwork(name, np);
	if (net == NONETWORK) return(Null);
	return(Make_EElectric((INTBIG)net, T_ENetwork));
}

/****************************** INITIALIZATION ******************************/

void init_lib_electric(void)
{
	T_ENodeInst = Define_Type(0, x_("nodeinst"), NOFUNC, sizeof (struct S_ENodeInst),
		EElectric_Equal, EElectric_Equal, ENodeInst_Print, NOFUNC);
	T_ENodeProto = Define_Type(0, x_("nodeproto"), NOFUNC, sizeof (struct S_ENodeProto),
		EElectric_Equal, EElectric_Equal, ENodeProto_Print, NOFUNC);
	T_EPortArcInst = Define_Type(0, x_("portarcinst"), NOFUNC, sizeof (struct S_EPortArcInst),
		EElectric_Equal, EElectric_Equal, EPortArcInst_Print, NOFUNC);
	T_EPortExpInst = Define_Type(0, x_("portexpinst"), NOFUNC, sizeof (struct S_EPortExpInst),
		EElectric_Equal, EElectric_Equal, EPortExpInst_Print, NOFUNC);
	T_EPortProto = Define_Type(0, x_("portproto"), NOFUNC, sizeof (struct S_EPortProto),
		EElectric_Equal, EElectric_Equal, EPortProto_Print, NOFUNC);
	T_EArcInst = Define_Type(0, x_("arcinst"), NOFUNC, sizeof (struct S_EArcInst),
		EElectric_Equal, EElectric_Equal, EArcInst_Print, NOFUNC);
	T_EArcProto = Define_Type(0, x_("arcproto"), NOFUNC, sizeof (struct S_EArcProto),
		EElectric_Equal, EElectric_Equal, EArcProto_Print, NOFUNC);
	T_EGeom = Define_Type(0, x_("geom"), NOFUNC, sizeof (struct S_EGeom),
		EElectric_Equal, EElectric_Equal, EGeom_Print, NOFUNC);
	T_ELibrary = Define_Type(0, x_("library"), NOFUNC, sizeof (struct S_ELibrary),
		EElectric_Equal, EElectric_Equal, ELibrary_Print, NOFUNC);
	T_ETechnology = Define_Type(0, x_("technology"), NOFUNC, sizeof (struct S_ETechnology),
		EElectric_Equal, EElectric_Equal, ETechnology_Print, NOFUNC);
	T_ETool = Define_Type(0, x_("tool"), NOFUNC, sizeof (struct S_ETool),
		EElectric_Equal, EElectric_Equal, ETool_Print, NOFUNC);
	T_ERTNode = Define_Type(0, x_("rtnode"), NOFUNC, sizeof (struct S_ERTNode),
		EElectric_Equal, EElectric_Equal, ERTNode_Print, NOFUNC);
	T_ENetwork = Define_Type(0, x_("network"), NOFUNC, sizeof (struct S_ENetwork),
		EElectric_Equal, EElectric_Equal, ENetwork_Print, NOFUNC);
	T_EView = Define_Type(0, x_("view"), NOFUNC, sizeof (struct S_EView),
		EElectric_Equal, EElectric_Equal, EView_Print, NOFUNC);
	T_EWindow = Define_Type(0, x_("window"), NOFUNC, sizeof (struct S_EWindow),
		EElectric_Equal, EElectric_Equal, EWindow_Print, NOFUNC);
	T_EGraphics = Define_Type(0, x_("graphics"), NOFUNC, sizeof (struct S_EGraphics),
		EElectric_Equal, EElectric_Equal, EGraphics_Print, NOFUNC);
	T_EConstraint = Define_Type(0, x_("constraint"), NOFUNC, sizeof (struct S_EConstraint),
		EElectric_Equal, EElectric_Equal, EConstraint_Print, NOFUNC);
	T_EWindowFrame = Define_Type(0, x_("windowframe"), NOFUNC, sizeof (struct S_EWindowFrame),
		EElectric_Equal, EElectric_Equal, EWindowFrame_Print, NOFUNC);

	/* define the query predicates */
	Define_Primitive((ELKObject(*)(ELLIPSIS))P_ENodeInstP, x_("nodeinst?"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))P_ENodeProtoP, x_("nodeproto?"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))P_EPortArcInstP, x_("portarcinst?"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))P_EPortExpInstP, x_("portexpinst?"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))P_EPortProtoP, x_("portproto?"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))P_EArcInstP, x_("arcinst?"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))P_EArcProtoP, x_("arcproto?"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))P_EGeomP, x_("geom?"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))P_ELibraryP, x_("library?"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))P_ETechnologyP, x_("technology?"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))P_EToolP, x_("tool?"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))P_ERTNodeP, x_("rtnode?"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))P_ENetworkP, x_("network?"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))P_EViewP, x_("view?"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))P_EWindowP, x_("window?"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))P_EGraphicsP, x_("graphics?"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))P_EConstraintP, x_("constraint?"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))P_EWindowFrameP, x_("windowframe?"), 1, 1, EVAL);

	/* define symbols */
	Define_Symbol(&lsp_displayablesym, x_("displayable"));

	/* define the database examination predicates */
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_curlib, x_("curlib"), 0, 0, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_curtech, x_("curtech"), 0, 0, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_getval, x_("getval"), 2, 2, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_getparentval, x_("getparentval"), 3, 3, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_P, x_("P"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_PD, x_("PD"), 2, 2, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_PAR, x_("PAR"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_PARD, x_("PARD"), 2, 2, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_setval, x_("setval"), 4, 4, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_setind, x_("setind"), 4, 4, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_delval, x_("delval"), 2, 2, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_initsearch, x_("initsearch"), 5, 5, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_nextobject, x_("nextobject"), 1, 1, EVAL);

	/* define the tool predicates */
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_gettool, x_("gettool"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_maxtool, x_("maxtool"), 0, 0, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_indextool, x_("indextool"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_toolturnon, x_("toolturnon"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_toolturnoff, x_("toolturnoff"), 1, 1, EVAL);

	/* define the library predicates */
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_getlibrary, x_("getlibrary"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_newlibrary, x_("newlibrary"), 2, 2, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_killlibrary, x_("killlibrary"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_eraselibrary, x_("eraselibrary"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_selectlibrary, x_("selectlibrary"), 1, 1, EVAL);

	/* define the nodeproto predicates */
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_getnodeproto, x_("getnodeproto"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_newnodeproto, x_("newnodeproto"), 2, 2, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_killnodeproto, x_("killnodeproto"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_copynodeproto, x_("copynodeproto"), 3, 3, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_iconview, x_("iconview"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_contentsview, x_("contentsview"), 1, 1, EVAL);

	/* define the nodeinst predicates */
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_newnodeinst, x_("newnodeinst"), 8, 8, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_modifynodeinst, x_("modifynodeinst"), 7, 7, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_killnodeinst, x_("killnodeinst"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_replacenodeinst, x_("replacenodeinst"), 2, 2,
		EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_nodefunction, x_("nodefunction"), 1, 1, EVAL);

	/* define the arcinst predicates */
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_newarcinst, x_("newarcinst"), 12, 12, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_modifyarcinst, x_("modifyarcinst"), 6, 6, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_killarcinst, x_("killarcinst"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_replacearcinst, x_("replacearcinst"), 2, 2, EVAL);

	/* define the portproto predicates */
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_getportproto, x_("getportproto"), 2, 2, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_newportproto, x_("newportproto"), 4, 4, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_portposition, x_("portposition"), 2, 2, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_killportproto, x_("killportproto"), 2, 2, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_moveportproto, x_("moveportproto"), 4, 4, EVAL);

	/* define the change control predicates */
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_undoabatch, x_("undoabatch"), 0, 0, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_noundoallowed, x_("noundoallowed"), 0, 0, EVAL);

	/* define the view predicates */
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_getview, x_("getview"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_newview, x_("newview"), 2, 2, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_killview, x_("killview"), 1, 1, EVAL);

	/* define the miscellaneous predicates */
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_telltool, x_("telltool"), 1, MANY, VARARGS);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_getarcproto, x_("getarcproto"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_gettechnology, x_("gettechnology"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_getpinproto, x_("getpinproto"), 1, 1, EVAL);
	Define_Primitive((ELKObject(*)(ELLIPSIS))lsp_getnetwork, x_("getnetwork"), 2, 2, EVAL);
}

#endif
