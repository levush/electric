/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iogdsi.h
 * Input/output tool: GDSII stream
 * Written by: Glen Lawson
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

/* data declarations */
#define BLOCKSIZE       2048
#define MAXLINELENGTH    132
#define MAXPOINTS       4096
#define MAXLAYERS        256
#define IDSTRING         128
#define TEXTLINE         512
#define MINFONTWIDTH     130
#define MINFONTHEIGHT    190

typedef enum
{
	GDS_POLY,
	GDS_RECTANGLE,
	GDS_OBLIQUE,
	GDS_LINE,
	GDS_CLOSED
} shape_type;

typedef enum
{
	GDS_NULLSYM,		/* 0 */
	GDS_HEADER,			/* 1 */
	GDS_BGNLIB,			/* 2 */
	GDS_LIBNAME,		/* 3 */
	GDS_UNITS,			/* 4 */
	GDS_ENDLIB,			/* 5 */
	GDS_BGNSTR,			/* 6 */
	GDS_STRNAME,		/* 7 */
	GDS_ENDSTR,			/* 8 */
	GDS_BOUNDARY,		/* 9 */
	GDS_PATH,			/* 10 */
	GDS_SREF,			/* 11 */
	GDS_AREF,			/* 12 */
	GDS_TEXTSYM,		/* 13 */
	GDS_LAYER,			/* 14 */
	GDS_DATATYPSYM,		/* 15 */
	GDS_WIDTH,			/* 16 */
	GDS_XY,				/* 17 */
	GDS_ENDEL,			/* 18 */
	GDS_SNAME,			/* 19 */
	GDS_COLROW,			/* 20 */
	GDS_TEXTNODE,		/* 21 */
	GDS_NODE,			/* 22 */
	GDS_TEXTTYPE,		/* 23 */
	GDS_PRESENTATION,	/* 24 */
	GDS_SPACING,		/* 25 */
	GDS_STRING,			/* 26 */
	GDS_STRANS,			/* 27 */
	GDS_MAG,			/* 28 */
	GDS_ANGLE,			/* 29 */
	GDS_UINTEGER,		/* 30 */
	GDS_USTRING,		/* 31 */
	GDS_REFLIBS,		/* 32 */
	GDS_FONTS,			/* 33 */
	GDS_PATHTYPE,		/* 34 */
	GDS_GENERATIONS,	/* 35 */
	GDS_ATTRTABLE,		/* 36 */
	GDS_STYPTABLE,		/* 37 */
	GDS_STRTYPE,		/* 38 */
	GDS_ELFLAGS,		/* 39 */
	GDS_ELKEY,			/* 40 */
	GDS_LINKTYPE,		/* 41 */
	GDS_LINKKEYS,		/* 42 */
	GDS_NODETYPE,		/* 43 */
	GDS_PROPATTR,		/* 44 */
	GDS_PROPVALUE,		/* 45 */
	GDS_BOX,			/* 46 */
	GDS_BOXTYPE,		/* 47 */
	GDS_PLEX,			/* 48 */
	GDS_BGNEXTN,		/* 49 */
	GDS_ENDEXTN,		/* 50 */
	GDS_TAPENUM,		/* 51 */
	GDS_TAPECODE,		/* 52 */
	GDS_STRCLASS,		/* 53 */
	GDS_NUMTYPES,		/* 54 */
	GDS_IDENT,			/* 55 */
	GDS_REALNUM,		/* 56 */
	GDS_SHORT_NUMBER,	/* 57 */
	GDS_NUMBER,			/* 58 */
	GDS_FLAGSYM,		/* 59 */
	GDS_FORMAT,			/* 60 */
	GDS_MASK,			/* 61 */
	GDS_ENDMASKS,		/* 62 */
	GDS_BADFIELD		/* 63 */
} gsymbol;

typedef enum
{
	GDS_ERR,
	GDS_NONE,
	GDS_FLAGS,
	GDS_SHORT_WORD,
	GDS_LONG_WORD,
	GDS_SHORT_REAL,
	GDS_LONG_REAL,
	GDS_ASCII_STRING
} datatype_symbol;

typedef struct
{
	CHAR a11[7];
	CHAR a12[7];
	CHAR a21[7];
	CHAR a22[7];
} orientation_type;

typedef struct
{
	INTBIG	px;
	INTBIG	py;
} point_type;

typedef struct _stream
{
	FILE   *fp;
	UCHAR1   buf[BLOCKSIZE];
} STREAM;
