/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: vhdl.h
 * Header file for VHDL compiler
 * Written by: Andrew R. Kostiuk, Queen's University
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

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
extern "C"
{
#endif

/********** General Constants ******************************************/

/* #define VHDL50           1 */			/* uncomment to enable VHDL 5.0 */
#define FNAMESIZE			200				/* maximum size of file names */
#define MAXVHDLLINE			1000			/* maximum line length */
#define NOTARGET			0				/* no output target, compile only */
#define TARGET_ALS			1				/* output target is ALS file */
#define TARGET_QUISC		2				/* output target is QUISC file */
#define TARGET_NETLISP		3				/* output target is NETLISP file */
#define TARGET_RSIM			4				/* output target is NETLISP for RSIM */
#define TARGET_SILOS		5				/* output target is SILOS  (V.01)*/

extern TOOL *vhdl_tool;

/********** Token Definitions ******************************************/

#define NOTOKEN				-1
/********** Delimiters **********/
#define TOKEN_AMPERSAND		 0
#define TOKEN_APOSTROPHE	 1
#define TOKEN_LEFTBRACKET	 2
#define TOKEN_RIGHTBRACKET	 3
#define TOKEN_STAR			 4
#define TOKEN_PLUS			 5
#define TOKEN_COMMA			 6
#define TOKEN_MINUS			 7
#define TOKEN_PERIOD		 8
#define TOKEN_SLASH			 9
#define TOKEN_COLON			10
#define TOKEN_SEMICOLON		11
#define TOKEN_LT			12
#define TOKEN_EQ			13
#define TOKEN_GT			14
#define TOKEN_VERTICALBAR	15
/********** Compound Delimiters **********/
#define TOKEN_ARROW			16
#define TOKEN_DOUBLEDOT		17
#define TOKEN_DOUBLESTAR	18
#define TOKEN_VARASSIGN		19
#define TOKEN_NE			20
#define TOKEN_GE			21
#define TOKEN_LE			22
#define TOKEN_BOX			23
/********** Other Token **********/
#define TOKEN_UNKNOWN		24
#define TOKEN_IDENTIFIER	25				/* alphanumeric (first char alpha) */
#define TOKEN_KEYWORD		26				/* reserved keyword of the language */
#define TOKEN_DECIMAL		27				/* decimal literal */
#define TOKEN_BASED			28				/* based literal */
#define TOKEN_CHAR			29				/* character literal */
#define TOKEN_STRING		30				/* string enclosed in double quotes */
#define TOKEN_BIT_STRING	31				/* bit string */

/********** Keyword Constants ******************************************/

#define KEY_ABS				 0
#define KEY_AFTER			 1
#define KEY_ALIAS			 2
#define KEY_AND				 3
#ifdef VHDL50
#define KEY_ARCHITECTURAL	 4
#else
#define KEY_ARCHITECTURE	 4
#endif
#define KEY_ARRAY			 5
#define KEY_ASSERTION		 6
#define KEY_ATTRIBUTE		 7
#define KEY_BEHAVIORAL		 8
#define KEY_BEGIN			 9
#define KEY_BODY			10
#define KEY_CASE			11
#define KEY_COMPONENT		12
#define KEY_CONNECT			13
#define KEY_CONSTANT		14
#define KEY_CONVERT			15
#define KEY_DOT				16
#define KEY_DOWNTO			17
#define KEY_ELSE			18
#define KEY_ELSIF			19
#define KEY_END				20
#define KEY_ENTITY			21
#define KEY_EXIT			22
#define KEY_FOR				23
#define KEY_FUNCTION		24
#define KEY_GENERATE		25
#define KEY_GENERIC			26
#define KEY_IF				27
#define KEY_IN				28
#define KEY_INOUT			29
#define KEY_IS				30
#define KEY_LINKAGE			31
#define KEY_LOOP			32
#define KEY_MOD				33
#define KEY_NAND			34
#define KEY_NEXT			35
#define KEY_NOR				36
#define KEY_NOT				37
#define KEY_NULL			38
#define KEY_OF				39
#define KEY_OR				40
#define KEY_OTHERS			41
#define KEY_OUT				42
#define KEY_PACKAGE			43
#define KEY_PORT			44
#define KEY_RANGE			45
#define KEY_RECORD			46
#define KEY_REM				47
#define KEY_REPORT			48
#define KEY_RESOLVE			49
#define KEY_RETURN			50
#define KEY_SEVERITY		51
#define KEY_SIGNAL			52
#define KEY_STANDARD		53
#define KEY_STATIC			54
#define KEY_SUBTYPE			55
#define KEY_THEN			56
#define KEY_TO				57
#define KEY_TYPE			58
#define KEY_UNITS			59
#define KEY_USE				60
#define KEY_VARIABLE		61
#define KEY_WHEN			62
#define KEY_WHILE			63
#define KEY_WITH			64
#define KEY_XOR				65

/* Added to support IEEE Standard */
#ifndef VHDL50
#define KEY_OPEN			66
#define KEY_MAP				67
#define KEY_ALL				68
#endif

#define KEY_LIBRARY			69

/********** Keyword Structures *****************************************/

#define NOVKEYWORD	((VKEYWORD *)NULL)

typedef struct
{
	CHAR	*name;							/* string defining keyword */
	INTBIG	num;							/* number of keyword */
} VKEYWORD;

/********** Token Structures *****************************************/

#define NOTOKENLIST	((TOKENLIST *)NULL)

typedef struct Itokenlist
{
	INTBIG	token;							/* token number */
	CHAR	*pointer;						/* NULL if delimiter, */
											/* pointer to global name space if identifier, */
											/* pointer to keyword table if keyword, */
											/* pointer to string if decimal literal, */
											/* pointer to string if based literal, */
											/* value of character if character literal, */
											/* pointer to string if string literal, */
											/* pointer to string if bit string literal */
	INTBIG	space;							/* TRUE if space before next token */
	INTBIG	line_num;						/* line number token occurred */
	struct Itokenlist *next;				/* next in list */
	struct Itokenlist *last;				/* previous in list */
} TOKENLIST;

/******** Identifier Table Structures **********************************/

/* #define IDENT_TABLE_SIZE 10007 */		/* maximum number of identifiers */
#define IDENT_TABLE_SIZE 80021				/* maximum number of identifiers */
											/* this value should be prime */
											/* for the HASH function */
#define MAX_HASH_TRYS	  1000

typedef struct
{
	CHAR	*string;						/* pointer to string, NULL if empty */
} IDENTTABLE;

/********** Symbol Trees **********************************************/

#define NOSYMBOL			0
#define SYMBOL_ENTITY		1
#define SYMBOL_BODY			2
#define SYMBOL_TYPE			3
#define SYMBOL_FPORT		4
#define SYMBOL_COMPONENT	5
#define SYMBOL_SIGNAL		6
#define SYMBOL_INSTANCE		7
#define SYMBOL_VARIABLE		8
#define SYMBOL_LABEL		9
#define SYMBOL_PACKAGE		10
#define SYMBOL_CONSTANT		11

typedef struct Isymboltree
{
	IDENTTABLE				*value;			/* identifier */
	INTBIG					type;			/* type of item */
	CHAR					*pointer;		/* pointer to item */
	struct Isymboltree		*lptr;			/* left pointer */
	struct Isymboltree		*rptr;			/* right pointer */
	INTBIG                   seen;			/* flag for deallocation */
} SYMBOLTREE;

typedef struct Isymbollist
{
	struct Isymboltree		*root;			/* root of symbol tree */
	struct Isymbollist		*last;			/* previous in stack */
	struct Isymbollist		*next;			/* next in list */
} SYMBOLLIST;

/********** Gate Entity Structures *************************************/

typedef struct Igate
{
	IDENTTABLE				*name;			/* name of gate */
	CHAR					*header;		/* header line */
	struct Igateline		*lines;			/* lines of gate def'n */
	INTBIG					flags;			/* flags for general use */
	struct Igate			*next;			/* next gate in list */
} GATE;

typedef struct Igateline
{
	CHAR					*line;			/* line of gate def'n */
	struct Igateline		*next;			/* next line in def'n */
} GATELINE;

/********** Unresolved Reference List **********************************/

typedef struct Iunreslist
{
	IDENTTABLE				*interfacef;	/* name of reference */
	INTBIG					numref;			/* number of references */
	struct Iunreslist		*next;			/* next in list */
} UNRESLIST;

extern UNRESLIST         *vhdl_unresolved_list;

/********** ALS Generation Constants *********************************/

#define TOP_ENTITY_FLAG		0x0001			/* flag the entity as called */
#define ENTITY_WRITTEN		0x0002			/* flag the entity as written */

/***********************************************************************/

/*
 * File:  db.h
 * Description: Header file for VHDL compiler including data base structures and
 *  constants.
 */

typedef struct Idbunits
{
	struct Idbinterface		*interfaces;	/* list of interfaces */
	struct Idbbody			*bodies;		/* list of bodies */
} DBUNITS;

typedef struct Idbpackage
{
	IDENTTABLE				*name;			/* name of package */
	struct Isymboltree		*root;			/* root of symbol tree */
} DBPACKAGE;

typedef struct Idbinterface
{
	IDENTTABLE				*name;			/* name of interface */
	struct Idbportlist		*ports;			/* list of ports */
	CHAR					*interfacef;	/* interface declarations */
	INTBIG					flags;			/* for later code gen */
	struct Idbbody			*bodies;		/* associated bodies */
	struct Isymbollist		*symbols;		/* local symbols */
	struct Idbinterface		*next;			/* next interface */
} DBINTERFACE;

#define DBMODE_IN			1
#define DBMODE_OUT			2
#define DBMODE_DOTOUT		3
#define DBMODE_INOUT		4
#define DBMODE_LINKAGE		5
typedef struct Idbportlist
{
	IDENTTABLE				*name;			/* name of port */
	INTBIG					mode;			/* mode of port */
	struct Idbltype			*type;			/* type of port */
	INTBIG					flags;			/* general flags */
	struct Idbportlist		*next;			/* next in port list */
} DBPORTLIST;

#define DBTYPE_SINGLE		1
#define DBTYPE_ARRAY		2
typedef struct Idbltype
{
	IDENTTABLE				*name;			/* name of type */
	INTBIG					type;			/* type of type */
	CHAR					*pointer;		/* pointer to info */
	struct Idbltype			*subtype;		/* possible subtype */
} DBLTYPE;

/********** Bodies *****************************************************/

#define DBBODY_BEHAVIORAL		1
#define DBBODY_ARCHITECTURAL	2
typedef struct Idbbody
{
	INTBIG					classnew;		/* class of body */
	IDENTTABLE				*name;			/* name of body - identifier */
	IDENTTABLE				*entity;		/* parent entity of body */
	struct Idbbodydeclare	*declare;		/* declarations */
	struct Idbstatements	*statements;	/* statements in body */
	struct Idbinterface		*parent;		/* pointer to parent */
	struct Idbbody			*same_parent;	/* bodies of same parent */
	struct Idbbody			*next;			/* next body */
} DBBODY;

typedef struct Idbbodydeclare
{
	struct Idbcomponents	*components;	/* components */
	struct Idbsignals		*bodysignals;		/* signals */
} DBBODYDECLARE;

typedef struct Idbcomponents
{
	IDENTTABLE				*name;			/* name of component */
	struct Idbportlist		*ports;			/* list of ports */
	struct Idbcomponents	*next;			/* next component */
} DBCOMPONENTS;

typedef struct Idbsignals
{
	IDENTTABLE				*name;			/* name of signal */
	struct Idbltype			*type;			/* type of signal */
	struct Idbsignals		*next;			/* next signal */
} DBSIGNALS;

/********** Architectural Statements ***********************************/

typedef struct Idbstatements
{
	struct Idbinstance		*instances;
} DBSTATEMENTS;

typedef struct Idbinstance
{
	IDENTTABLE				*name;			/* identifier */
	struct Idbcomponents	*compo;			/* component */
	struct Idbaportlist		*ports;			/* ports on instance */
	struct Idbinstance		*next;			/* next instance in list */
} DBINSTANCE;

typedef struct Idbaportlist
{
	struct Idbname			*name;			/* name of port */
	struct Idbportlist		*port;			/* pointer to port on comp */
	INTBIG					flags;			/* flags for processing */
	struct Idbaportlist		*next;			/* next in list */
} DBAPORTLIST;

/********** Names ******************************************************/

#define DBNAME_IDENTIFIER	1
#define DBNAME_INDEXED		2
#define DBNAME_CONCATENATED	3
typedef struct Idbname
{
	IDENTTABLE				*name;			/* name of name */
	INTBIG					type;			/* type of name */
	CHAR					*pointer;		/* NULL if identifier */
								 			/* pointer to DBEXPRLIST if indexed */
								 			/* pointer to DBNAMELIST if concatenated */
	struct Idbltype	*dbtype; 				/* pointer to type */
} DBNAME;

typedef struct Idbexprlist
{
	INTBIG					value;			/* value */
	struct Idbexprlist		*next;			/* next in list */
} DBEXPRLIST;

typedef struct Idbdiscreterange
{
	INTBIG					start;			/* start of range */
	INTBIG					end;			/* end of range */
} DBDISCRETERANGE;

typedef struct Idbindexrange
{
	struct Idbdiscreterange	*drange;		/* discrete range */
	struct Idbindexrange	*next;			/* next in list */
} DBINDEXRANGE;

typedef struct Idbnamelist
{
	struct Idbname			*name;			/* name in list */
	struct Idbnamelist		*next;			/* next in list */
} DBNAMELIST;

/***********************************************************************/
/***********************************************************************/

/*
 * File:  syntax.h
 * Description: Header file for VHDL compiler including parse tree structures and
 *  constants.
 *    Modified January 3, 1989 for IEEE Standard syntax
 *  Allan G. Jost, Technical University of Nova Scotia
 */

#define PARSE_ERR		-1					/* parsing error */

/******** Parser Constants and Structures ******************************/

#define NOUNIT			0
#define UNIT_INTERFACE	1
#define UNIT_FUNCTION	2
#define UNIT_PACKAGE	3
#define UNIT_BODY		4

#ifdef VHDL50
#define UNIT_WITH		5
#endif

#define UNIT_USE		6
typedef struct Iptree
{
	INTBIG					type;			/* type of entity */
	CHAR					*pointer;		/* pointer to design unit */
	struct Iptree			*next;			/* pointer to next */
} PTREE;

/********** Packages ***************************************************/

typedef struct Ipackage
{
	struct Itokenlist		*name;			/* package name */
	struct Ipackagedpart	*declare;		/* package declare part */
} PACKAGE;

typedef struct Ipackagedpart
{
	struct Ibasicdeclare	*item;			/* package declare item */
	struct Ipackagedpart	*next;			/* pointer to next */
} PACKAGEDPART;

#ifdef VHDL50
typedef struct Iwith
{
	struct Itokenlist		*unit;			/* unit */
	struct Iwith			*next;			/* next in list */
} WITH;
#endif

typedef struct Iuse
{
	struct Itokenlist		*unit;			/* unit */
	struct Iuse				*next;			/* next in list */
} USE;

/********** Interfaces *************************************************/

typedef struct Ivinterface
{
	struct Itokenlist		*name;			/* name of entity */
	struct Ifportlist		*ports;			/* list of ports */
	CHAR					*interfacef;	/* interface declarations */
} VINTERFACE;

#define NOMODE				0
#define MODE_IN				1
#define MODE_OUT			2
#define MODE_DOTOUT			3
#define MODE_INOUT			4
#define MODE_LINKAGE		5
typedef struct Ifportlist
{
	struct Iidentlist		*names;			/* names of port */
	INTBIG					mode;			/* mode of port */
	struct Ivname			*type;			/* type of port */
	struct Ifportlist		*next;			/* next in port list */
} FPORTLIST;

typedef struct Iidentlist
{
	struct Itokenlist		*identifier;	/* identifier */
	struct Iidentlist		*next;			/* next in list */
} IDENTLIST;

/********** Bodies *****************************************************/

#define NOBODY				0
#define BODY_BEHAVIORAL		1
#define BODY_ARCHITECTURAL	2
typedef struct Ibody
{
	INTBIG					classnew;		/* class of body */
	struct Itokenlist		*name;			/* name of body - identifier */
	struct Isimplename		*entity;		/* parent entity of body */
	struct Ibodydeclare		*body_declare;	/* body declarations */
	struct Istatements		*statements;	/* statements in body */
} BODY;

#define NOBODYDECLARE			0
#define BODYDECLARE_BASIC		1
#define BODYDECLARE_COMPONENT	2
#define BODYDECLARE_RESOLUTION	3
#define BODYDECLARE_LOCAL		4
typedef struct Ibodydeclare
{
	INTBIG					type;			/* type of declaration */
	CHAR					*pointer;		/* pointer to part tree */
	struct Ibodydeclare		*next;			/* next in list */
} BODYDECLARE;

/********** Basic Declarations *****************************************/

#define NOBASICDECLARE			0
#define BASICDECLARE_OBJECT		1
#define BASICDECLARE_TYPE		2
#define BASICDECLARE_SUBTYPE	3
#define BASICDECLARE_CONVERSION	4
#define BASICDECLARE_ATTRIBUTE	5
#define BASICDECLARE_ATT_SPEC	6
typedef struct Ibasicdeclare
{
	INTBIG					type;			/* type of basic declare */
	CHAR					*pointer;		/* pointer to parse tree */
} BASICDECLARE;

#define NOOBJECTDECLARE			0
#define OBJECTDECLARE_CONSTANT	1
#define OBJECTDECLARE_SIGNAL	2
#define OBJECTDECLARE_VARIABLE	3
#define OBJECTDECLARE_ALIAS		4
typedef struct Iobjectdeclare
{
	INTBIG					type;			/* type of object declare */
	CHAR					*pointer;		/* pointer to parse tree */
} OBJECTDECLARE;

typedef struct Isignaldeclare
{
	struct Iidentlist		*names;			/* list of identifiers */
	struct Isubtypeind		*subtype;		/* subtype indicator */
} SIGNALDECLARE;

typedef struct Ivcomponent
{
	struct Itokenlist		*name;			/* name of component */
	struct Ifportlist		*ports;			/* ports of component */
} VCOMPONENT;

typedef struct Iconstantdeclare
{
	struct Itokenlist		*identifier;	/* name of constant */
	struct Isubtypeind		*subtype;		/* subtype indicator */
	struct Iexpression		*expression;	/* expression */
} CONSTANTDECLARE;

/********** Types ******************************************************/

typedef struct Isubtypeind
{
	struct Ivname			*type;			/* type of subtype */
	struct Iconstraint		*constraint;	/* optional constaint */
} SUBTYPEIND;

#define NOCONSTAINT			0
#define CONSTAINT_RANGE		1
#define CONSTAINT_FLOAT		2
#define CONSTAINT_INDEX		3
typedef struct Iconstaint
{
	INTBIG					type;			/* type of constaint */
	CHAR					*pointer;		/* pointer to parse tree */
} CONSTAINT;

#define NOTYPE				0
#define TYPE_SCALAR			1
#define TYPE_COMPOSITE		2
typedef struct Itype
{
	struct Itokenlist		*identifier;	/* name of type */
	INTBIG					type;			/* type definition */
	CHAR					*pointer;		/* pointer to type */
} TYPE;

#define NOCOMPOSITE			0
#define COMPOSITE_ARRAY		1
#define COMPOSITE_RECORD	2
typedef struct Icomposite
{
	INTBIG					type;			/* type of composite */
	CHAR					*pointer;		/* pointer to composite */
} COMPOSITE;

#define NOARRAY				0
#define ARRAY_UNCONSTRAINED	1
#define ARRAY_CONSTRAINED	2
typedef struct Iarray
{
	INTBIG					type;			/* (un)constrained array */
	CHAR					*pointer;		/* pointer to array */
} ARRAY;

typedef struct Iconstrained
{
	struct Iindexconstraint	*constraint;	/* index constraint */
	struct Isubtypeind		*subtype;		/* subtype indication */
} CONSTRAINED;

typedef struct Iindexconstraint
{
	struct Idiscreterange	*discrete;		/* discrete range */
	struct Iindexconstraint	*next;			/* possible more */
} INDEXCONSTRAINT;

/********** Architectural Statements ***********************************/

#define NOARCHSTATE				0
#define ARCHSTATE_GENERATE		1
#define ARCHSTATE_SIG_ASSIGN	2
#define ARCHSTATE_IF			3
#define ARCHSTATE_CASE			4
#define ARCHSTATE_INSTANCE		5
#define ARCHSTATE_NULL			6
typedef struct Istatements
{
	INTBIG					type;			/* type of statement */
	CHAR					*pointer;		/* pointer to parse tree */
	struct Istatements		*next;			/* pointer to next */
} STATEMENTS;

typedef struct Ivinstance
{
	struct Itokenlist		*name;			/* optional identifier */
	struct Isimplename		*entity;		/* entity of instance */
	struct Iaportlist		*ports;			/* ports on instance */
} VINSTANCE;

#define NOAPORTLIST				0
#define APORTLIST_NAME			1
#define APORTLIST_TYPE_NAME		2
#define APORTLIST_EXPRESSION	3
typedef struct Iaportlist
{
	INTBIG					type;			/* type of actual port */
	CHAR					*pointer;		/* pointer to parse tree */
	struct Iaportlist		*next;			/* next in list */
} APORTLIST;

typedef struct Iaportlisttype
{
	struct Iname			*type;			/* type */
	struct Iname			*name;			/* name */
} APORTLISTTYPE;

typedef struct Igenerate
{
	struct Itokenlist		*label;			/* optional label */
	struct Igenscheme		*gen_scheme;	/* generate scheme */
	struct Istatements		*statements;	/* statements */
} GENERATE;

#define GENSCHEME_FOR		0
#define GENSCHEME_IF		1
typedef struct Igenscheme
{
	INTBIG					scheme;			/* scheme (for or if) */
	struct Itokenlist		*identifier;	/* if FOR scheme */
	struct Idiscreterange	*range;			/* if FOR scheme */
	struct Iexpression		*condition;		/* if IF scheme */
} GENSCHEME;

typedef struct Isigassign
{
	struct Isignallist		*signal;		/* list of signals */
	struct Iwaveformlist	*driver;		/* list of waveforms */
} SIGASSIGN;

typedef struct Isignallist
{
	struct Iname			*name;			/* name of signal */
	struct Isignallist		*next;			/* next signal in list */
} SIGNALLIST;

typedef struct Iwaveformlist
{
	struct Iwaveform		*waveform;		/* waveform element */
	struct Iwaveformlist	*next;			/* next waveform in list */
} WAVEFORMLIST;

typedef struct Iwaveform
{
	struct Iexpression		*expression;	/* expression */
	struct Iexpression		*time_expr;		/* time expression */
} WAVEFORM;

/********** Names ******************************************************/

#define NONAME				0
#define NAME_SINGLE			1
#define NAME_CONCATENATE	2
#define NAME_ATTRIBUTE		3
typedef struct Ivname
{
	INTBIG					type;			/* type of name */
	CHAR					*pointer;		/* pointer to parse tree */
} VNAME;

#define NOSINGLENAME		0
#define SINGLENAME_SIMPLE	1
#define SINGLENAME_SELECTED	2
#define SINGLENAME_INDEXED	3
#define SINGLENAME_SLICE	4
typedef struct Isinglename
{
	INTBIG					type;			/* type of simple name */
	CHAR					*pointer;		/* pointer to parse tree */
} SINGLENAME;

typedef struct Isimplename
{
	struct Itokenlist		*identifier;	/* identifier */
} SIMPLENAME;

typedef struct Iselectedname
{
	struct Iselectprefix	*prefix;		/* prefix */
	struct Iselectsuffix	*suffix;		/* suffix */
} SELECTEDNAME;

#define NOSELECTPREFIX			0
#define SELECTPREFIX_PREFIX		1
#define SELECTPREFIX_STANDARD	2
typedef struct Iselectprefix
{
	INTBIG					type;			/* type of prefix */
	CHAR					*pointer;		/* pointer to parse tree */
} SELECTPREFIX;

#define NOPREFIX				0
#define PREFIX_NAME				1
#define PREFIX_FUNCTION_CALL	2
typedef struct Iprefix
{
	INTBIG					type;			/* type of prefix */
	CHAR					*pointer;		/* pointer to parse tree */
} PREFIX;

#define NOSELECTSUFFIX			0
#define SELECTSUFFIX_SIMPLENAME	1
#define SELECTSUFFIX_CHAR_LIT	2			/* character */
typedef struct Iselectsuffix
{
	INTBIG					type;			/* type of suffix */
	CHAR					*pointer;		/* pointer to parse tree */
} SELECTSUFFIX;

typedef struct Iindexedname
{
	struct Iprefix			*prefix;		/* prefix */
	struct Iexprlist		*expr_list;		/* expression list */
} INDEXEDNAME;

typedef struct Iexprlist
{
	struct Iexpression		*expression;	/* expression */
	struct Iexprlist		*next;			/* next in list */
} EXPRLIST;

typedef struct Islicename
{
	struct Iprefix			*prefix;		/* prefix */
	struct Idiscreterange	*range;			/* discrete range */
} SLICENAME;

#define NODISCRETERANGE			0
#define DISCRETERANGE_SUBTYPE	1
#define DISCRETERANGE_RANGE		2
typedef struct Idiscreterange
{
	INTBIG					type;			/* type of discrete range */
	CHAR					*pointer;		/* pointer to parse tree */
} DISCRETERANGE;

#define NORANGE				0
#define RANGE_ATTRIBUTE		1
#define RANGE_SIMPLE_EXPR	2
typedef struct Irange
{
	INTBIG					type;			/* type of range */
	CHAR					*pointer;		/* pointer to parse tree */
} RANGE;

typedef struct Irangesimple
{
	struct Isimpleexpr		*start;			/* start of range */
	struct Isimpleexpr		*end;			/* end of range */
} RANGESIMPLE;

typedef struct Iconcatenatedname
{
	struct Isinglename		*name;			/* single name */
	struct Iconcatenatedname	*next;		/* next in list */
} CONCATENATEDNAME;

/********** Expressions ************************************************/

#define NOLOGOP			0
#define LOGOP_AND		1
#define LOGOP_OR		2
#define LOGOP_NAND		3
#define LOGOP_NOR		4
#define LOGOP_XOR		5
typedef struct Iexpression
{
	struct Irelation		*relation;		/* first relation */
	struct Imrelations		*next;			/* more relations */
} EXPRESSION;

#define NORELOP			0
#define RELOP_EQ		1
#define RELOP_NE		2
#define RELOP_LT		3
#define RELOP_LE		4
#define RELOP_GT		5
#define RELOP_GE		6
typedef struct Irelation
{
	struct Isimpleexpr		*simple_expr;	/* simple expression */
	INTBIG					rel_operator;	/* possible operator */
	struct Isimpleexpr		*simple_expr2;	/* possible expression */
} RELATION;

typedef struct Imrelations
{
	INTBIG					log_operator;	/* logical operator */
	struct Irelation		*relation;		/* relation */
	struct Imrelations		*next;			/* more relations */
} MRELATIONS;

#define NOADDOP			0
#define ADDOP_ADD		1
#define ADDOP_SUBTRACT		2
typedef struct Isimpleexpr
{
	INTBIG					sign;			/* sign (1 or  -1) */
	struct Iterm			*term;			/* first term */
	struct Imterms			*next;			/* additional terms */
} SIMPLEEXPR;

#define NOMULOP				0
#define MULOP_MULTIPLY		1
#define MULOP_DIVIDE		2
#define MULOP_MOD			3
#define MULOP_REM			4
typedef struct Iterm
{
	struct Ifactor			*factor;		/* first factor */
	struct Imfactors		*next;			/* additional factors */
} TERM;

typedef struct Imterms
{
	INTBIG					add_operator;	/* add operator */
	struct Iterm			*term;			/* next term */
	struct Imterms			*next;			/* any more terms */
} MTERMS;

#define NOMISCOP			0
#define MISCOP_POWER		1
#define MISCOP_ABS			2
#define MISCOP_NOT			3
typedef struct Ifactor
{
	struct Iprimary			*primary;		/* first primary */
	INTBIG					misc_operator;	/* possible operator */
	struct Iprimary			*primary2;		/* possible primary */
} FACTOR;

typedef struct Imfactors
{
	INTBIG					mul_operator;	/* operator */
	struct Ifactor			*factor;		/* next factor */
	struct Imfactors		*next;			/* possible more factors */
} MFACTORS;

#define NOPRIMARY				0
#define PRIMARY_NAME			1
#define PRIMARY_LITERAL			2
#define PRIMARY_AGGREGATE		3
#define PRIMARY_CONCATENATION	4
#define PRIMARY_FUNCTION_CALL	5
#define PRIMARY_TYPE_CONVERSION	6
#define PRIMARY_QUALIFIED_EXPR	7
#define PRIMARY_EXPRESSION		8
typedef struct Iprimary
{
	INTBIG					type;			/* type of primary */
	CHAR					*pointer;		/* pointer to primary */
} PRIMARY;

#define NOLITERAL			0
#define LITERAL_NUMERIC		1
#define LITERAL_ENUMERATION	2
#define LITERAL_STRING		3
#define LITERAL_BIT_STRING	4
typedef struct Iliteral
{
	INTBIG					type;			/* type of literal */
	CHAR					*pointer;		/* pointer to parse tree */
} LITERAL;

#define NONUMERICLITERAL	0

/* prototypes for tool interface */
void           vhdl_init(INTBIG*, CHAR1*[], TOOL*);
void           vhdl_done(void);
void           vhdl_set(INTBIG, CHAR*[]);
INTBIG         vhdl_request(CHAR*, va_list);
void           vhdl_slice(void);

/* prototypes for intratool interface */
INTBIG         vhdl_evalexpression(EXPRESSION*);
INTBIG         vhdl_evalsimpleexpr(SIMPLEEXPR*);
IDENTTABLE    *vhdl_findidentkey(CHAR*);
DBINTERFACE   *vhdl_findtopinterface(DBUNITS*);
void           vhdl_freeparsermemory(void);
void           vhdl_freesemantic(void);
void           vhdl_freeunresolvedlist(UNRESLIST**);
void           vhdl_genals(LIBRARY*, NODEPROTO*);
void           vhdl_gennet(INTBIG);
void           vhdl_genquisc(void);
void           vhdl_gensilos(void);
IDENTTABLE    *vhdl_getnameident(VNAME*);
TOKENLIST     *vhdl_getnametoken(VNAME*);
void           vhdl_getnexttoken(void);
IDENTTABLE    *vhdl_getprefixident(PREFIX*);
TOKENLIST     *vhdl_getprefixtoken(PREFIX*);
DBLTYPE        *vhdl_gettype(IDENTTABLE*);
BOOLEAN        vhdl_keysame(TOKENLIST*, INTBIG);
IDENTTABLE    *vhdl_makeidentkey(CHAR*);
DISCRETERANGE *vhdl_parsediscrete_range(void);
EXPRESSION    *vhdl_parseexpression(void);
VNAME         *vhdl_parsename(void);
BOOLEAN        vhdl_parser(TOKENLIST*);
SIMPLEEXPR    *vhdl_parsesimpleexpression(void);
SIMPLENAME    *vhdl_parsesimplename(void);
SUBTYPEIND    *vhdl_parsesubtype_indication(void);
TYPE          *vhdl_parsetype(void);
void           vhdl_print(CHAR *fstring, ...);
void           vhdl_printoneline(CHAR *fstring, ...);
void           vhdl_reporterrormsg(TOKENLIST*, CHAR*);
SYMBOLTREE    *vhdl_searchsymbol(IDENTTABLE*, SYMBOLLIST*);
BOOLEAN        vhdl_semantic(void);
DBNAME        *vhdl_semname(VNAME*);
void           vhdl_unresolved(IDENTTABLE*, UNRESLIST**);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
