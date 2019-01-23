/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iogdsi.c
 * Input/output tool: GDSII stream input
 * Written by: Glen Lawson, S-MOS Systems, Inc.
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
 * Notes:
 *	Unsupported features and assumptions:
 *	Map layers with the IO_gds_layer_numbers variable on the current technology.
 *	The pure-layer node must have the HOLDSTRACE option, if not will CRASH
 *		for boundary types.  Will map to the generic:DRC-node if not found.
 *	Case sensitive.
 *	SUBLAYERS or XXXTYPE fields are not supported, perhaps a variable
 *		could be attached to the object with this value.
 *	NODEs and TEXTNODEs - don't have an example
 *	PROPERTIES - could add as a variable to the structure.
 *	PATHTYPE 1 - rounded ends on paths, not supported
 *	Path dogears - no cleanup yet, simpler to map paths into arcs
 *	Absolute angle - ???
 *	REFLIBS - not supported.
 *	I have no BOX type examples.
 *
 * Nice to have:
 *	PATH-ARC mapping - should be done, problem is that any layer can
 *		be a path, only connection layers in electric are arcs.
 *	Someone could make a GDS mapping technology for this purpose,
 *		defaults could be taken from this technology.
 *	Misc. fields mapped to variables - should be done.
 *	AREFs - could build into a separate NODEPROTO, and instance, thus
 *		preserving the original intent and space.  With a timestamp,
 *		can determine if the structure has been disturbed for output.
 *	MAG - no scaling is possible, must create a separate object for each
 *		value, don't scale.  (TEXT does scale)
 */

#include "config.h"
#include "global.h"
#include "egraphics.h"
#include "efunction.h"
#include "database.h"
#include "eio.h"
#include "usr.h"
#include "iogdsi.h"
#include "tech.h"
#include "tecgen.h"
#include "edialogs.h"
#include <math.h>

#define NEWPATH 1		/* path and cell caching optimizations */

/* electric specific globals */
static LIBRARY        *io_gdslibrary = NOLIBRARY;
static jmp_buf         io_gdsenv;
static NODEPROTO      *io_gdsstructure = NONODEPROTO;
static NODEPROTO      *io_gdssref = NONODEPROTO;
static NODEINST       *io_gdsinstance = NONODEINST;
static NODEPROTO      *io_gdslayernode = NONODEPROTO;
static NODEPROTO      *io_gdslayernodes[MAXLAYERS];
static INTBIG          io_gdslayertext[MAXLAYERS];
static INTBIG          io_gdslayershape[MAXLAYERS];
static INTBIG          io_gdslayerrect[MAXLAYERS];
static INTBIG          io_gdslayerpath[MAXLAYERS];
static INTBIG          io_gdslayerpoly[MAXLAYERS];
static BOOLEAN         io_gdslayervisible[MAXLAYERS];
static BOOLEAN         io_gdslayerused;
static INTBIG          io_gdslayerwhich;
static INTBIG          io_gdssrefcount;
static INTBIG          io_gdsrecomputed;
static INTBIG         *io_gdscurstate;
static void           *io_gdsprogressdialog;

/* globals */
static STREAM          io_gdsstream;
static INTBIG          io_gdsbytesread;
static INTBIG          io_gdsrecordcount;
static INTBIG          io_gdsbufferindex;
static INTBIG          io_gdsblockcount;
static gsymbol         io_gdstoken;
static UINTSML         io_gdstokenflags;
static INTSML          io_gdstokenvalue16;
static INTBIG          io_gdstokenvalue32;
static float           io_gdsrealtokensp;
static double          io_gdsrealtokendp;
static CHAR            io_gdstokenstring[TEXTLINE+1];
static point_type      io_gdsvertex[MAXPOINTS];
static point_type      io_gdsboundarydelta[MAXPOINTS];
static INTBIG          io_gdscurlayer, io_gdscursublayer;
static double          io_gdsscale;
static INTBIG          io_gdsfilesize;
static INTBIG         *io_gdsIlayerlist;
static INTBIG          io_gdsIlayerlisttotal = 0;


/* these are not really used */
static CHAR            io_gdsversionstring[IDSTRING+1];
static CHAR            io_gdslibraryname[IDSTRING+1];

static CHAR *io_gdsIerror_string[] =
{
	N_("success"),											/* 0 */
	N_("Invalid GDS II datatype"),							/* 1 */
	N_("GDS II version number is not decipherable"),		/* 2 */
	N_("GDS II header statement is missing"),				/* 3 */
	N_("Library name is missing"),							/* 4 */
	N_("Begin library statement is missing"),				/* 5 */
	N_("Date value is not a valid number"),					/* 6 */
	N_("Libname statement is missing"),						/* 7 */
	N_("Generations value is invalid"),						/* 8 */
	N_("Units statement has invalid number format"),		/* 9 */
	N_("Units statement is missing"),						/* 10 */
	N_("Warning this structure is empty"),					/* 11 */
	N_("Structure name is missing"),						/* 12 */
	N_("Strname statement is missing"),						/* 13 */
	N_("Begin structure statement is missing"),				/* 14 */
	N_("Element end statement is missing"),					/* 15 */
	N_("Structure reference name is missing"),				/* 16 */
	N_("Structure reference has no translation value"),		/* 17 */
	N_("Boundary has no points"),							/* 18 */
	N_("Structure reference is missing its flags field"),	/* 19 */
	N_("Not enough points in the shape"),					/* 20 */
	N_("Too many points in the shape"),						/* 21 */
	N_("Invalid layer number"),								/* 22 */
	N_("Layer statement is missing"),						/* 23 */
	N_("No datatype field"),								/* 24 */
	N_("Path element has no points"),						/* 25 */
	N_("Text element has no reference point"),				/* 26 */
	N_("Array reference has no parameters"),				/* 27 */
	N_("Array reference name is missing"),					/* 28 */
	N_("Property has no value"),							/* 29 */
	N_("Array orientation oblique"),						/* 30 */
	N_("Failed to create AREF"),							/* 31 */
	N_("Failed to create RECTANGLE"),						/* 32 */
	N_("Failed to create POLYGON"),							/* 33 */
	N_("No memory"),										/* 34 */
	N_("Failed to create POLYGON points"),					/* 35 */
	N_("Could not create text marker"),						/* 36 */
	N_("Could not create text string"),						/* 37 */
	N_("Failed to create structure"),						/* 38 */
	N_("Failed to create cell center"),						/* 39 */
	N_("Failed to create NODE"),							/* 40 */
	N_("Failed to create box"),								/* 41 */
	N_("Failed to create outline"),							/* 42 */
	N_("Failed to create SREF proto"),						/* 43 */
	N_("Failed to create SREF")								/* 44 */
};

static gsymbol io_gdsIoption_set[] =
{
	GDS_ATTRTABLE,
	GDS_REFLIBS,
	GDS_FONTS,
	GDS_GENERATIONS,
	GDS_BADFIELD
};

static gsymbol io_gdsIshape_set[] =
{
	GDS_AREF,
	GDS_SREF,
	GDS_BOUNDARY,
	GDS_PATH,
	GDS_NODE,
	GDS_TEXTSYM,
	GDS_BOX,
	GDS_BADFIELD
};

static gsymbol io_gdsIgood_op_set[] =
{
	GDS_HEADER,
	GDS_BGNLIB,
	GDS_LIBNAME,
	GDS_UNITS,
	GDS_ENDLIB,
	GDS_BGNSTR,
	GDS_STRNAME,
	GDS_ENDSTR,
	GDS_BOUNDARY,
	GDS_PATH,
	GDS_SREF,
	GDS_AREF,
	GDS_TEXTSYM,
	GDS_LAYER,
	GDS_DATATYPSYM,
	GDS_WIDTH,
	GDS_XY,
	GDS_ENDEL,
	GDS_SNAME,
	GDS_COLROW,
	GDS_TEXTNODE,
	GDS_NODE,
	GDS_TEXTTYPE,
	GDS_PRESENTATION,
	GDS_STRING,
	GDS_STRANS,
	GDS_MAG,
	GDS_ANGLE,
	GDS_REFLIBS,
	GDS_FONTS,
	GDS_PATHTYPE,
	GDS_GENERATIONS,
	GDS_ATTRTABLE,
	GDS_NODETYPE,
	GDS_PROPATTR,
	GDS_PROPVALUE,
	GDS_BOX,
	GDS_BOXTYPE,
	GDS_FORMAT,
	GDS_MASK,
	GDS_ENDMASKS,
	GDS_BADFIELD
};

static gsymbol io_gdsIbackup_set[] =
{
	GDS_UNITS,
	GDS_REFLIBS,
	GDS_FONTS,
	GDS_ATTRTABLE,
	GDS_BADFIELD
};

static gsymbol io_gdsImask_set[] =
{
	GDS_DATATYPSYM,
	GDS_TEXTTYPE,
	GDS_BOXTYPE,
	GDS_NODETYPE,
	GDS_BADFIELD
};

static gsymbol io_gdsIunsupported_set[] =
{
	GDS_ELFLAGS,
	GDS_PLEX,
	GDS_BADFIELD
};

static gsymbol io_gdsIrecovery_set[100];

/* prototypes for local routines */
static void            io_gdsIappendset(gsymbol[], gsymbol);
static void            io_gdsIattrtable(void);
static void            io_gdsIbackup(void);
static void            io_gdsIbeginstructure(void);
static void            io_gdsIbox(INTBIG*);
static void            io_gdsIcopyset(gsymbol[], gsymbol[]);
static void            io_gdsIdetermine_aref(void);
static void            io_gdsIdetermine_boundary(INTBIG, shape_type*, shape_type*);
static void            io_gdsIdetermine_box(void);
static void            io_gdsIdetermine_justification(INTBIG*, INTBIG*);
static void            io_gdsIdetermine_layer(INTBIG*, INTBIG*);
static void            io_gdsIdetermine_node(void);
static void            io_gdsIdetermine_orientation(INTBIG*, INTBIG*, double*);
static void            io_gdsIdetermine_path(void);
static void            io_gdsIdetermine_points(INTBIG, INTBIG, INTBIG*);
static void            io_gdsIdetermine_property(void);
static void            io_gdsIdetermine_shape(void);
static void            io_gdsIdetermine_sref(void);
static void            io_gdsIdetermine_text(void);
static void            io_gdsIdetermine_time(CHAR*);
static void            io_gdsIelement(BOOLEAN*);
static void            io_gdsIerror(INTBIG);
static NODEPROTO      *io_gdsIfindproto(CHAR *name);
static void            io_gdsIfonts(void);
static void            io_gdsIgetdouble(double*);
static void            io_gdsIgetfloat(float*);
static void            io_gdsIgetinteger(INTBIG*);
static void            io_gdsIgetproto(CHAR*);
static UCHAR1          io_gdsIgetrawbyte(void);
static gsymbol         io_gdsIgetrecord(datatype_symbol*);
static datatype_symbol io_gdsIgetrecordtype(void);
static void            io_gdsIgetstring(CHAR*);
static void            io_gdsIgettoken(void);
static void            io_gdsIgetword(UINTSML*);
static void            io_gdsIheader(void);
static void            io_gdsIinit(void);
static void            io_gdsIitoa(CHAR*, INTBIG, INTBIG);
static void            io_gdsIlibrary(void);
static BOOLEAN         io_gdsIload(FILE*);
static BOOLEAN         io_gdsImember(gsymbol, gsymbol[]);
static void            io_gdsIOnceinit(void);
static void            io_gdsIonlyone(gsymbol[], gsymbol);
static double          io_gdsIpower(INTBIG, INTBIG);
static void            io_gdsIrecompute(NODEPROTO*, INTBIG);
static void            io_gdsIreflibs(void);
static void            io_gdsIshape(INTBIG, shape_type, shape_type);
static void            io_gdsIstep(INTBIG, INTBIG, INTBIG, INTBIG);
static void            io_gdsIstructure(void);
static void            io_gdsItext(CHAR*, INTBIG, INTBIG, INTBIG, INTBIG, double);
static void            io_gdsItransform(point_type, INTBIG, INTBIG);
static void            io_gdsItranslate(INTBIG*, INTBIG*, double, BOOLEAN);
static void            io_gdsIunits(void);
static void            io_gdsIunsupported(gsymbol[]);
static void            io_gdsIparselayernumbers(CHAR *layernumbers, INTBIG **layers, INTBIG *total);

/* modules */

/*
 * Routine to free all memory associated with this module.
 */
void io_freegdsinmemory(void)
{
	if (io_gdsIlayerlisttotal > 0) efree((CHAR *)io_gdsIlayerlist);
}

double io_gdsIpower(INTBIG val, INTBIG power)
{
	double result;
	REGISTER INTBIG count;

	result = 1.0;
	for(count=0; count<power; count++)
		result *= (double)val;
	return(result);
}

BOOLEAN io_gdsImember(gsymbol tok, gsymbol set[])
{
	REGISTER gsymbol *ctok;

	for (ctok = set; *ctok != GDS_BADFIELD; ctok++)
		if (*ctok == tok) return(TRUE);
	return(FALSE);
}

void io_gdsIcopyset(gsymbol set1[], gsymbol set2[])
{
	REGISTER gsymbol *item1, *item2;

	for (item1 = set1, item2 = set2; *item1 != GDS_BADFIELD; item1++, item2++)
		*item2 = *item1;
	*item2 = GDS_BADFIELD;
}

void io_gdsIonlyone(gsymbol set[], gsymbol item)
{
	set[0] = item;
	set[1] = GDS_BADFIELD;
}

void io_gdsIappendset(gsymbol set[], gsymbol item)
{
	REGISTER gsymbol *pos;

	pos = set;
	while (*pos != GDS_BADFIELD) pos++;
	*pos++ = item; *pos = GDS_BADFIELD;
}

UCHAR1 io_gdsIgetrawbyte(void)
{
	if (io_gdsbufferindex >= BLOCKSIZE || io_gdsbufferindex < 0)
	{
		(void)xfread(io_gdsstream.buf, 1, BLOCKSIZE, io_gdsstream.fp);
		io_gdsbufferindex = 0;
		io_gdsblockcount = io_gdsblockcount + 1;
		io_gdsbytesread += BLOCKSIZE;
		if (io_gdsbytesread/5000 != (io_gdsbytesread-BLOCKSIZE)/5000)
		{
			if (io_verbose < 0 && io_gdsfilesize > 0)
			DiaSetProgress(io_gdsprogressdialog, io_gdsbytesread, io_gdsfilesize); else
				ttyputmsg(_("%ld bytes read (%ld%% done)"), io_gdsbytesread,
					io_gdsbytesread*100/io_gdsfilesize);
		}
	}
	io_gdsrecordcount = io_gdsrecordcount - 1;
	return(io_gdsstream.buf[io_gdsbufferindex++]);
}

void io_gdsIgetword(UINTSML *dataword)
{
	UINTSML lowbyte, highbyte;

	highbyte = io_gdsIgetrawbyte() & 0xFF;
	lowbyte = io_gdsIgetrawbyte() & 0xFF;
	(*dataword) = (highbyte << 8) + lowbyte;
}

void io_gdsIgetinteger(INTBIG *datavalue)
{
	UINTSML lowword, highword;

	io_gdsIgetword(&highword);
	io_gdsIgetword(&lowword);
	(*datavalue) = (highword << 16) + lowword;
}

void io_gdsIgetstring(CHAR *streamstring)
{
	CHAR letter, *pos;

	pos = streamstring;
	while (io_gdsrecordcount != 0)
	{
		letter = io_gdsIgetrawbyte();
		*pos++ = letter;
	}
	*pos = '\0';
}

void io_gdsIitoa(CHAR *nstring, INTBIG leng, INTBIG number)
{
	(void)esnprintf(nstring, leng, x_("%ld"), number);
}

void io_gdsIgetfloat(float *realvalue)
{
	unsigned long reg;
	UINTSML	dataword;
	INTBIG	sign;
	INTBIG	binary_exponent;
	INTBIG	shift_count;

	reg = io_gdsIgetrawbyte() & 0xFF;
	if (reg & 0x00000080) sign = -1;
		else sign = 1;
	reg = reg & 0x0000007F;

	/* generate the exponent, currently in Excess-64 representation */
	binary_exponent = (reg - 64) << 2;
	reg = (unsigned long)(io_gdsIgetrawbyte() & 0xFF) << 16;
	io_gdsIgetword(&dataword);
	reg = reg + dataword;
	shift_count = 0;

	/* normalize the matissa */
	while ((reg & 0x00800000) == 0)
	{
		reg = reg << 1;
		shift_count++;
	}

	/* this is the exponent + normalize shift - precision of matissa */
	/* binary_exponent = binary_exponent + shift_count - 24; */
	binary_exponent = binary_exponent - shift_count - 24;

	if (binary_exponent > 0)
	{
		(*realvalue) = (float)(sign * reg * io_gdsIpower(2, binary_exponent));
	} else
	{
		if (binary_exponent < 0)
			(*realvalue) = (float)(sign * reg / io_gdsIpower(2, -binary_exponent)); else
				(*realvalue) = (float)(sign * reg);
	}
}

void io_gdsIgetdouble(double *realvalue)
{
	UINTBIG register1, register2;
	UINTSML	dataword;
	INTBIG	exponent;
	INTBIG	long_integer;
	static double twoto32;
	static double twotoneg56;
	static BOOLEAN init = TRUE;

	if (init)
	{
		init = FALSE;
		twoto32 = io_gdsIpower(2, 32);
		twotoneg56 = 1.0 / io_gdsIpower (2, 56);
	}

	/* first byte is the exponent field (hex) */
	register1 = io_gdsIgetrawbyte() & 0xFF;

	/* plus sign bit */
	if (register1 & 0x00000080) *realvalue = -1.0; else
		*realvalue = 1.0;

	/* the hex exponent is in excess 64 format */
	register1 = register1 & 0x0000007F;
	exponent = register1 - 64;

	/* bytes 2-4 are the high ordered bits of the mantissa */
	register1 = (UINTBIG)(io_gdsIgetrawbyte() & 0xFF) << 16;
	io_gdsIgetword(&dataword);
	register1 = register1 + dataword;

	/* next word completes the matissa (1/16 to 1) */
	io_gdsIgetinteger(&long_integer);
	register2 = (UINTBIG)long_integer;

	/* now normalize the value  */
	if (register1 || register2)
	{
		/* while ((register1 & 0x00800000) == 0) */
		/* check for 0 in the high-order nibble */
		while ((register1 & 0x00F00000) == 0)
		{
			/* register1 = register1 << 1; */
			/* shift the 2 registers by 4 bits */
			register1 = (register1 << 4) + (register2>>28);
			register2 = register2 << 4;
			exponent--;
		}
	} else
	{
		/* true 0 */
		*realvalue = 0.0;
		return;
	}

	/* now create the matissa (fraction between 1/16 to 1) */
	*realvalue *= ((double) register1 * twoto32 + (double) register2) * twotoneg56;
	if (exponent > 0)
	{
		*realvalue = (*realvalue) * io_gdsIpower(16, exponent);
	} else
	{
		if (exponent < 0)
			*realvalue = (*realvalue) / io_gdsIpower(16, -exponent);
	}
}

datatype_symbol io_gdsIgetrecordtype(void)
{
	switch (io_gdsIgetrawbyte())
	{
		case 0:  return(GDS_NONE);
		case 1:  return(GDS_FLAGS);
		case 2:  return(GDS_SHORT_WORD);
		case 3:  return(GDS_LONG_WORD);
		case 4:  return(GDS_SHORT_REAL);
		case 5:  return(GDS_LONG_REAL);
		case 6:  return(GDS_ASCII_STRING);
		default: return(GDS_ERR);
	}
}

gsymbol io_gdsIgetrecord(datatype_symbol *gds_type)
{
	gsymbol temp;
	UINTSML dataword, recordtype;

	io_gdsIgetword(&dataword);
	io_gdsrecordcount = dataword - 2;
	recordtype = io_gdsIgetrawbyte() & 0xFF;
	temp = GDS_NULLSYM;
	switch (recordtype)
	{
		case  0: temp = GDS_HEADER;        break;
		case  1: temp = GDS_BGNLIB;        break;
		case  2: temp = GDS_LIBNAME;       break;
		case  3: temp = GDS_UNITS;         break;
		case  4: temp = GDS_ENDLIB;        break;
		case  5: temp = GDS_BGNSTR;        break;
		case  6: temp = GDS_STRNAME;       break;
		case  7: temp = GDS_ENDSTR;        break;
		case  8: temp = GDS_BOUNDARY;      break;
		case  9: temp = GDS_PATH;          break;
		case 10: temp = GDS_SREF;          break;
		case 11: temp = GDS_AREF;          break;
		case 12: temp = GDS_TEXTSYM;       break;
		case 13: temp = GDS_LAYER;         break;
		case 14: temp = GDS_DATATYPSYM;    break;
		case 15: temp = GDS_WIDTH;         break;
		case 16: temp = GDS_XY;            break;
		case 17: temp = GDS_ENDEL;         break;
		case 18: temp = GDS_SNAME;         break;
		case 19: temp = GDS_COLROW;        break;
		case 20: temp = GDS_TEXTNODE;      break;
		case 21: temp = GDS_NODE;          break;
		case 22: temp = GDS_TEXTTYPE;      break;
		case 23: temp = GDS_PRESENTATION;  break;
		case 24: temp = GDS_SPACING;       break;
		case 25: temp = GDS_STRING;        break;
		case 26: temp = GDS_STRANS;        break;
		case 27: temp = GDS_MAG;           break;
		case 28: temp = GDS_ANGLE;         break;
		case 29: temp = GDS_UINTEGER;      break;
		case 30: temp = GDS_USTRING;       break;
		case 31: temp = GDS_REFLIBS;       break;
		case 32: temp = GDS_FONTS;         break;
		case 33: temp = GDS_PATHTYPE;      break;
		case 34: temp = GDS_GENERATIONS;   break;
		case 35: temp = GDS_ATTRTABLE;     break;
		case 36: temp = GDS_STYPTABLE;     break;
		case 37: temp = GDS_STRTYPE;       break;
		case 38: temp = GDS_ELFLAGS;       break;
		case 39: temp = GDS_ELKEY;         break;
		case 40: temp = GDS_LINKTYPE;      break;
		case 41: temp = GDS_LINKKEYS;      break;
		case 42: temp = GDS_NODETYPE;      break;
		case 43: temp = GDS_PROPATTR;      break;
		case 44: temp = GDS_PROPVALUE;     break;
		case 45: temp = GDS_BOX;           break;
		case 46: temp = GDS_BOXTYPE;       break;
		case 47: temp = GDS_PLEX;          break;
		case 48: temp = GDS_BGNEXTN;       break;
		case 49: temp = GDS_ENDEXTN;       break;
		case 50: temp = GDS_TAPENUM;       break;
		case 51: temp = GDS_TAPECODE;      break;
		case 52: temp = GDS_STRCLASS;      break;
		case 53: temp = GDS_NUMTYPES;      break;
		case 54: temp = GDS_FORMAT;        break;
		case 55: temp = GDS_MASK;          break;
		case 56: temp = GDS_ENDMASKS;      break;
		default: temp = GDS_BADFIELD;
	}
	*gds_type = io_gdsIgetrecordtype();
	return(temp);
}

void io_gdsIerror(INTBIG n)
{
	ttyputerr(_("Error: %s, block %ld, index %ld"), TRANSLATE(io_gdsIerror_string[n]),
		io_gdsblockcount, io_gdsbufferindex);
	longjmp(io_gdsenv, 1);
}

void io_gdsIgettoken(void)
{
	static datatype_symbol valuetype;

	if (io_gdsrecordcount == 0)
	{
		io_gdstoken = io_gdsIgetrecord(&valuetype);
	} else
	{
		switch (valuetype)
		{
			case GDS_NONE:
				break;
			case GDS_FLAGS:
				io_gdsIgetword(&io_gdstokenflags);
				io_gdstoken = GDS_FLAGSYM;
				break;
			case GDS_SHORT_WORD:
				io_gdsIgetword((UINTSML *)&io_gdstokenvalue16);
				io_gdstoken = GDS_SHORT_NUMBER;
				break;
			case GDS_LONG_WORD:
				io_gdsIgetinteger(&io_gdstokenvalue32);
				io_gdstoken = GDS_NUMBER;
				break;
			case GDS_SHORT_REAL:
				io_gdsIgetfloat(&io_gdsrealtokensp);
				io_gdstoken = GDS_REALNUM;
				break;
			case GDS_LONG_REAL:
				io_gdsIgetdouble(&io_gdsrealtokendp);
				io_gdstoken = GDS_REALNUM;
				break;
			case GDS_ASCII_STRING:
				io_gdsIgetstring(io_gdstokenstring);
				io_gdstoken = GDS_IDENT;
				break;
			case GDS_ERR:
				io_gdsIerror(1);
				break;
		}
	}
}

void io_gdsItranslate(INTBIG *ang, INTBIG *trans, double angle, BOOLEAN reflected)
{
	*ang = rounddouble(angle) % 3600;
	*trans = reflected ? 1 : 0;
	if (*trans)
		*ang = (2700 - *ang) % 3600;

	/* should not happen...*/
	if (*ang < 0) *ang = *ang + 3600;
}

void io_gdsItransform(point_type delta, INTBIG angle, INTBIG trans)
{
	XARRAY matrix;

	makeangle(angle, trans, matrix);
	xform(delta.px, delta.py, &delta.px, &delta.py, matrix);
}

void io_gdsIstep(INTBIG nc, INTBIG nr, INTBIG angle, INTBIG trans)
{
	point_type rowinterval, colinterval, ptc, pt;
	INTBIG ic, ir;
	INTBIG cx, cy, ox, oy;
	XARRAY transform;

	if (nc != 1)
	{
		colinterval.px = (io_gdsvertex[1].px - io_gdsvertex[0].px) / nc;
		colinterval.py = (io_gdsvertex[1].py - io_gdsvertex[0].py) / nc;
		io_gdsItransform(colinterval, angle, trans);
	}
	if (nr != 1)
	{
		rowinterval.px = (io_gdsvertex[2].px - io_gdsvertex[0].px) / nr;
		rowinterval.py = (io_gdsvertex[2].py - io_gdsvertex[0].py) / nr;
		io_gdsItransform(rowinterval, angle, trans);
	}

	/* create the node */
	makeangle(angle, trans, transform);

	/* now transform the center */
	cx = (io_gdssref->highx + io_gdssref->lowx) / 2;
	cy = (io_gdssref->highy + io_gdssref->lowy) / 2;
	xform(cx, cy, &ox, &oy, transform);

	/* calculate the offset from the original center */
	ox -= cx;   oy -= cy;

	/* now generate the array */
	ptc.px = io_gdsvertex[0].px;   ptc.py = io_gdsvertex[0].py;
	for (ic = 0; ic < nc; ic++)
	{
		pt = ptc;
		for (ir = 0; ir < nr; ir++)
		{
			/* create the node */
			if ((io_gdscurstate[0]&GDSINARRAYS) != 0 ||
				(ir == 0 && ic == 0) ||
					(ir == (nr-1) && ic == (nc-1)))
			{
				if ((io_gdsinstance = newnodeinst(io_gdssref, pt.px + io_gdssref->lowx + ox,
					pt.px + io_gdssref->highx + ox, pt.py + io_gdssref->lowy + oy,
						pt.py + io_gdssref->highy + oy, trans, angle,
							io_gdsstructure)) == NONODEINST)
								io_gdsIerror(31);
				if ((io_gdscurstate[0] & GDSINEXPAND) != 0)
					io_gdsinstance->userbits |= NEXPAND;
				/* annotate the array info to this element */
			}

			/* add the row displacement */
			pt.px += rowinterval.px;   pt.py += rowinterval.py;
		}

		/* add displacement */
		ptc.px += colinterval.px;   ptc.py += colinterval.py;
	}
}

void io_gdsIdetermine_boundary(INTBIG npts, shape_type *perimeter, shape_type *oclass)
{
	BOOLEAN is90, is45;
	REGISTER INTBIG i;

	is90 = TRUE;
	is45 = TRUE;
	if (io_gdsvertex[0].px == io_gdsvertex[npts-1].px &&
		io_gdsvertex[0].py == io_gdsvertex[npts-1].py)
			(*perimeter) = GDS_CLOSED; else
				(*perimeter) = GDS_LINE;

	for (i=0; i<npts-1 && i<MAXPOINTS-1; i++)
	{
		io_gdsboundarydelta[i].px = io_gdsvertex[i+1].px - io_gdsvertex[i].px;
		io_gdsboundarydelta[i].py = io_gdsvertex[i+1].py - io_gdsvertex[i].py;
	}
	for (i=0; i<npts-1 && i<MAXPOINTS-1; i++)
	{
		if (io_gdsboundarydelta[i].px != 0 && io_gdsboundarydelta[i].py != 0)
		{
			is90 = FALSE;
			if (abs(io_gdsboundarydelta[i].px) != abs(io_gdsboundarydelta[i].py))
				is45 = FALSE;
		}
	}
	if ((*perimeter) == GDS_CLOSED && (is90 || is45))
		(*oclass) = GDS_POLY; else
			(*oclass) = GDS_OBLIQUE;
	if (npts == 5 && is90 && (*perimeter) == GDS_CLOSED)
		(*oclass) = GDS_RECTANGLE;
}

void io_gdsIbox(INTBIG *npts)
{
	REGISTER INTBIG i, pxm, pym, pxs, pys;

	pxm = io_gdsvertex[4].px;
	pxs = io_gdsvertex[4].px;
	pym = io_gdsvertex[4].py;
	pys = io_gdsvertex[4].py;
	for (i = 0; i<4; i++)
	{
		if (io_gdsvertex[i].px > pxm) pxm = io_gdsvertex[i].px;
		if (io_gdsvertex[i].px < pxs) pxs = io_gdsvertex[i].px;
		if (io_gdsvertex[i].py > pym) pym = io_gdsvertex[i].py;
		if (io_gdsvertex[i].py < pys) pys = io_gdsvertex[i].py;
	}
	io_gdsvertex[0].px = pxs;
	io_gdsvertex[0].py = pys;
	io_gdsvertex[1].px = pxm;
	io_gdsvertex[1].py = pym;
	(*npts) = 2;
}

void io_gdsIshape(INTBIG npts, shape_type perimeter, shape_type oclass)
{
	INTBIG lx, ly, hx, hy, cx, cy, i;
	INTBIG *trace, *pt;
	NODEINST *ni;

	switch (oclass)
	{
		case GDS_RECTANGLE:
			io_gdsIbox(&npts);

			/* create the rectangle */
			if (io_gdslayerused)
			{
				io_gdslayerrect[io_gdslayerwhich]++;
				io_gdslayerpoly[io_gdslayerwhich]++;
				if (newnodeinst(io_gdslayernode, io_gdsvertex[0].px,
					io_gdsvertex[1].px, io_gdsvertex[0].py, io_gdsvertex[1].py, 0, 0,
						io_gdsstructure) == NONODEINST)
							io_gdsIerror(32);
			}
			break;

		case GDS_OBLIQUE:
		case GDS_POLY:
			if (!io_gdslayerused) break;
			io_gdslayershape[io_gdslayerwhich]++;
			io_gdslayerpoly[io_gdslayerwhich]++;

			/* determine the bounds of the polygon */
			lx = hx = io_gdsvertex[0].px;
			ly = hy = io_gdsvertex[0].py;
			for (i=1; i<npts;i++)
			{
				if (lx > io_gdsvertex[i].px) lx = io_gdsvertex[i].px;
				if (hx < io_gdsvertex[i].px) hx = io_gdsvertex[i].px;
				if (ly > io_gdsvertex[i].py) ly = io_gdsvertex[i].py;
				if (hy < io_gdsvertex[i].py) hy = io_gdsvertex[i].py;
			}

			/* now create the node */
			if ((ni = newnodeinst(io_gdslayernode, lx, hx, ly, hy, 0, 0,
				io_gdsstructure)) == NONODEINST)
					io_gdsIerror(33);

			/* now allocate the trace */
			pt = trace = emalloc((npts*2*SIZEOFINTBIG), el_tempcluster);
			if (trace == 0) io_gdsIerror(34);
			cx = (hx + lx) / 2;   cy = (hy + ly) / 2;
			for (i=0; i<npts; i++)
			{
				*pt++ = io_gdsvertex[i].px - cx;
				*pt++ = io_gdsvertex[i].py - cy;
			}

			/* store the trace information */
			if (setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)trace,
				VINTEGER|VISARRAY|((npts*2)<<VLENGTHSH)) == NOVARIABLE)
					io_gdsIerror(35);

			/* free the polygon memory */
			efree((CHAR *)trace);
			break;
		default:
			break;
	}
}

void io_gdsItext(CHAR *charstring, INTBIG vjust, INTBIG hjust, INTBIG angle,
	INTBIG trans, double scale)
{
	NODEINST *ni;
	VARIABLE *var;
	INTBIG size;

	/* no text */
	if (!io_gdslayerused || !(io_gdscurstate[0]&GDSINTEXT)) return;
	io_gdslayertext[io_gdslayerwhich]++;

	io_gdsvertex[1].px = io_gdsvertex[0].px + MINFONTWIDTH * estrlen(charstring);
	io_gdsvertex[1].py = io_gdsvertex[0].py + MINFONTHEIGHT;

	/* create a holding node */
	ni = newnodeinst(io_gdslayernode, io_gdsvertex[0].px, io_gdsvertex[0].px,
		io_gdsvertex[0].py, io_gdsvertex[0].py, trans, angle, io_gdsstructure);
	if (ni == NONODEINST) io_gdsIerror(36);

	/* now add the text */
	var = setvalkey((INTBIG)ni, VNODEINST, el_node_name_key, (INTBIG)charstring,
		VSTRING|VDISPLAY);
	if (var == NOVARIABLE) io_gdsIerror(37);

	/* set the text size and orientation */
	size = rounddouble(scale);
	if (size <= 0) size = 8;
	if (size > TXTMAXPOINTS) size = TXTMAXPOINTS;
	TDSETSIZE(var->textdescript, size);

	/* determine the presentation */
	TDSETPOS(var->textdescript, VTPOSCENT);
	switch (vjust)
	{
		case 1:		/* top */
			switch (hjust)
			{
				case 1:  TDSETPOS(var->textdescript, VTPOSUPRIGHT); break;
				case 2:  TDSETPOS(var->textdescript, VTPOSUPLEFT);  break;
				default: TDSETPOS(var->textdescript, VTPOSUP);      break;
			}
			break;

		case 2:		/* bottom */
			switch (hjust)
			{
				case 1:  TDSETPOS(var->textdescript, VTPOSDOWNRIGHT); break;
				case 2:  TDSETPOS(var->textdescript, VTPOSDOWNLEFT);  break;
				default: TDSETPOS(var->textdescript, VTPOSDOWN);      break;
			}
			break;

		default:	/* centered */
			switch (hjust)
			{
				case 1:  TDSETPOS(var->textdescript, VTPOSRIGHT); break;
				case 2:  TDSETPOS(var->textdescript, VTPOSLEFT);  break;
				default: TDSETPOS(var->textdescript, VTPOSCENT);  break;
			}
	}
}

/*
 * This routine rewritten by Steven Rubin to be more robust in finding
 * pure-layer nodes associated with the GDS layer numbers.
 */
void io_gdsIOnceinit(void)
{
	INTBIG i, j, k, count, which, *layers, numlayers, fakelayer[1];
	VARIABLE *var;
	static POLYGON *poly = NOPOLYGON;
	REGISTER NODEINST *ni;
	NODEINST node;
	REGISTER NODEPROTO *np;

	/* create the polygon */
	(void)needstaticpolygon(&poly, 4, io_tool->cluster);

	/* run through the node prototypes in this technology */
	ni = &node;   initdummynode(ni);

	/* create the layer mapping table */
	for (i=0; i<MAXLAYERS; i++)
	{
		io_gdslayernodes[i] = NONODEPROTO;
		io_gdslayertext[i] = 0;
		io_gdslayershape[i] = 0;
		io_gdslayerrect[i] = 0;
		io_gdslayerpath[i] = 0;
		io_gdslayerpoly[i] = 0;
		io_gdslayervisible[i] = TRUE;
	}
	var = getval((INTBIG)el_curtech, VTECHNOLOGY, -1, x_("IO_gds_layer_numbers"));
	if (var != NOVARIABLE && (var->type&VISARRAY) == 0) var = NOVARIABLE;
	if (var != NOVARIABLE)
	{
		count = getlength(var);
		for (i=0; i<count; i++)
		{
			if ((var->type&VTYPE) == VSTRING)
			{
				io_gdsIparselayernumbers(((CHAR **)var->addr)[i], &layers, &numlayers);
			} else
			{
				fakelayer[0] = ((INTBIG *)var->addr)[i];
				layers = fakelayer;
				if (fakelayer[0] < 0) numlayers = 0; else
					numlayers = 1;
			}
			for(k=0; k<numlayers; k++)
			{
				which = layers[k];
				if (which < 0) continue;
				if (which >= MAXLAYERS)
				{
					ttyputmsg(_("GDS layer %ld is too large (limit is %ld)"), which, MAXLAYERS-1);
					continue;
				}
				for(np = el_curtech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				{
					if (((np->userbits&NFUNCTION)>>NFUNCTIONSH) != NPNODE) continue;
					ni->proto = np;
					ni->lowx  = np->lowx;
					ni->highx = np->highx;
					ni->lowy  = np->lowy;
					ni->highy = np->highy;
					j = nodepolys(ni, 0, NOWINDOWPART);
					if (j != 1) continue;
					shapenodepoly(ni, 0, poly);
					if (i == poly->layer)
					{
						/* ignore if this layer is not to be drawn */
						if ((poly->desc->colstyle&INVISIBLE) != 0)
							io_gdslayervisible[which] = FALSE;
						break;
					}
				}
				if (np != NONODEPROTO)
				{
					io_gdslayernodes[which] = np;
					continue;
				}
			}
		}
	} else ttyputmsg(_("Technology has no GDSII layer numbers"));
}

/*
 * Routine to parse a string of layer numbers in "layernumbers" and return those layers
 * in the array "layers", with the size of the array in "total".
 */
void io_gdsIparselayernumbers(CHAR *layernumbers, INTBIG **layers, INTBIG *total)
{
	REGISTER CHAR *pt;
	REGISTER INTBIG count, i, newtotal, *newlist, number;

	count = 0;
	pt = layernumbers;
	for(;;)
	{
		while (*pt == ' ') pt++;
		if (*pt == 0) break;
		number = myatoi(pt);
		if (count >= io_gdsIlayerlisttotal)
		{
			newtotal = io_gdsIlayerlisttotal * 2;
			if (count >= newtotal) newtotal = count+5;
			newlist = (INTBIG *)emalloc(newtotal * SIZEOFINTBIG, io_tool->cluster);
			if (newlist == 0) return;
			for(i=0; i<count; i++)
				newlist[i] = io_gdsIlayerlist[i];
			if (io_gdsIlayerlisttotal > 0) efree((CHAR *)io_gdsIlayerlist);
			io_gdsIlayerlist = newlist;
			io_gdsIlayerlisttotal = newtotal;
		}
		io_gdsIlayerlist[count++] = number;
		while (*pt != 0 && *pt != ',') pt++;
		if (*pt == ',') pt++;
	}
	*layers = io_gdsIlayerlist;
	*total = count;
}

/*
 * Routine to initialize the input of a GDS file
 */
void io_gdsIinit(void)
{
	io_gdsbufferindex = -1;
	io_gdsrecordcount = 0;
	io_gdsblockcount = 1;
	io_gdscurlayer = io_gdscursublayer = 0;
}

void io_gdsIdetermine_time(CHAR *time_string)
{
	REGISTER INTBIG i;
	CHAR time_array[7][IDSTRING+1];

	for (i = 0; i < 6; i++)
	{
		if (io_gdstoken == GDS_SHORT_NUMBER)
		{
			if (i == 0 && io_gdstokenvalue16 < 1900)
			{
				/* handle Y2K date issues */
				if (io_gdstokenvalue16 > 60) io_gdstokenvalue16 += 1900; else
					io_gdstokenvalue16 += 2000;
			}
			(void)esnprintf(time_array[i], IDSTRING+1, x_("%02d"), io_gdstokenvalue16);
			io_gdsIgettoken();
		} else
		{
			io_gdsIonlyone(io_gdsIrecovery_set, GDS_SHORT_NUMBER);
			io_gdsIerror(6);
		}
	}
	(void)esnprintf(time_string, IDSTRING+1, x_("%s-%s-%s at %s:%s:%s"), time_array[1], time_array[2],
		time_array[0], time_array[3], time_array[4], time_array[5]);
}

void io_gdsIheader(void)
{
	if (io_gdstoken == GDS_HEADER)
	{
		io_gdsIgettoken();
		if (io_gdstoken == GDS_SHORT_NUMBER)
		{
			io_gdsIitoa(io_gdsversionstring, 129, io_gdstokenvalue16);
		} else
		{
			io_gdsIonlyone(io_gdsIrecovery_set, GDS_BGNLIB);
			io_gdsIerror(2);
		}
	} else
	{
		io_gdsIonlyone(io_gdsIrecovery_set, GDS_BGNLIB);
		io_gdsIerror(3);
	}
}

void io_gdsIlibrary(void)
{
	CHAR mod_time[IDSTRING+1], create_time[IDSTRING+1];

	if (io_gdstoken == GDS_BGNLIB)
	{
		io_gdsIgettoken();
		io_gdsIdetermine_time(create_time);
		io_gdsIdetermine_time(mod_time);
		if (io_gdstoken == GDS_LIBNAME)
		{
			io_gdsIgettoken();
			if (io_gdstoken == GDS_IDENT)
			{
				(void)estrcpy(io_gdslibraryname, io_gdstokenstring);
			} else
			{
				io_gdsIonlyone(io_gdsIrecovery_set, GDS_UNITS);
				io_gdsIerror(4);
			}
		} else
		{
			io_gdsIonlyone(io_gdsIrecovery_set, GDS_UNITS);
		}
	} else
	{
		io_gdsIonlyone(io_gdsIrecovery_set, GDS_UNITS);
		io_gdsIerror(5);
	}
}

void io_gdsIreflibs(void)
{
	io_gdsIgettoken();
	io_gdsIgettoken();
}

void io_gdsIfonts(void)
{
	io_gdsIgettoken();
	io_gdsIgettoken();
}

void io_gdsIbackup(void)
{
	CHAR backup_string[IDSTRING+1];

	io_gdsIgettoken();
	if (io_gdstoken == GDS_SHORT_NUMBER)
	{
		io_gdsIitoa(backup_string, 129, io_gdstokenvalue16);
	} else
	{
		io_gdsIcopyset(io_gdsIbackup_set, io_gdsIrecovery_set);
		io_gdsIerror(8);
	}
	io_gdsIgettoken();
}

void io_gdsIattrtable(void)
{
	io_gdsIgettoken();
	if (io_gdstoken == GDS_IDENT)
	{
		io_gdsIgettoken();
	}
}

void io_gdsIunits(void)
{
	double meter_unit, db_unit;
	double microns_per_user_unit;
	CHAR realstring[IDSTRING+1];

	if (io_gdstoken == GDS_UNITS)
	{
		io_gdsIgettoken();
		if (io_gdstoken == GDS_REALNUM)
		{
			db_unit = io_gdsrealtokendp;
			/* io_gdstechnologyscalefactor = rounddouble(1.0 / io_gdsrealtokendp); */
			io_gdsIgettoken();
			meter_unit = io_gdsrealtokendp;
			microns_per_user_unit = (meter_unit / db_unit) * 1.0e6;
			(void)esnprintf(realstring, IDSTRING+1, x_("%6.3f"), microns_per_user_unit);

			/* don't change the cast in this equation - roundoff error! */
			io_gdsscale = meter_unit  * (double)(1000000 * scalefromdispunit(1.0, DISPUNITMIC));
		} else
		{
			io_gdsIonlyone(io_gdsIrecovery_set, GDS_BGNSTR);
			io_gdsIerror(9);
		}
	} else
	{
		io_gdsIonlyone(io_gdsIrecovery_set, GDS_BGNSTR);
		io_gdsIerror(10);
	}
}

void io_gdsIunsupported(gsymbol bad_op_set[])
{
	if (io_gdsImember(io_gdstoken, bad_op_set))
	do
	{
		io_gdsIgettoken();
	} while (!io_gdsImember(io_gdstoken, io_gdsIgood_op_set));
}

void io_gdsIdetermine_points(INTBIG min_points, INTBIG max_points, INTBIG *point_counter)
{
	(*point_counter) = 0;
	while (io_gdstoken == GDS_NUMBER)
	{
		io_gdsvertex[*point_counter].px = rounddouble((double)io_gdstokenvalue32 * io_gdsscale);
		io_gdsIgettoken();
		io_gdsvertex[*point_counter].py = rounddouble((double)io_gdstokenvalue32 * io_gdsscale);
		(*point_counter)++;
		if (*point_counter > max_points)
		{
			ttyputmsg(_("Found %ld points"), *point_counter);
			io_gdsIerror(21);
		}
		io_gdsIgettoken();
	}
	if (*point_counter < min_points)
	{
		ttyputmsg(_("Found %ld points"), *point_counter);
		io_gdsIerror(20);
	}
}

void io_gdsIdetermine_orientation(INTBIG *angle, INTBIG *trans, double *scale)
{
	BOOLEAN mirror_x;
	double anglevalue;

	anglevalue = 0.0;
	*scale = 1.0;
	mirror_x = FALSE;
	io_gdsIgettoken();
	if (io_gdstoken == GDS_FLAGSYM)
	{
		if ((io_gdstokenflags&0100000) != 0) mirror_x = TRUE;
		io_gdsIgettoken();
	} else
	{
		io_gdsIonlyone(io_gdsIrecovery_set, GDS_XY);
		io_gdsIerror(19);
	}
	if (io_gdstoken == GDS_MAG)
	{
		io_gdsIgettoken();
		*scale = io_gdsrealtokendp;
		io_gdsIgettoken();
	}
	if (io_gdstoken == GDS_ANGLE)
	{
		io_gdsIgettoken();
		anglevalue = io_gdsrealtokendp * 10;
		io_gdsIgettoken();
	}
	io_gdsItranslate(angle, trans, anglevalue, mirror_x);
}

void io_gdsIdetermine_layer(INTBIG *layer, INTBIG *sublayer)
{
	*layer = io_gdscurlayer;
	*sublayer = io_gdscursublayer;
	if (io_gdstoken == GDS_LAYER)
	{
		io_gdsIgettoken();
		if (io_gdstoken == GDS_SHORT_NUMBER)
		{
			io_gdscurlayer = *layer = io_gdstokenvalue16;
			if (io_gdscurlayer >= MAXLAYERS)
			{
				ttyputmsg(_("GDS layer %ld is too high (limit is %d)"),
					io_gdscurlayer, MAXLAYERS-1);
				io_gdscurlayer = MAXLAYERS-1;
			}
			if (io_gdslayernodes[io_gdscurlayer] == NONODEPROTO)
			{
				if ((io_gdscurstate[0]&GDSINIGNOREUKN) == 0)
				{
					ttyputmsg(_("GDS layer %ld unknown, using Generic:DRC"),
						io_gdscurlayer);
				} else
				{
					ttyputmsg(_("GDS layer %ld unknown, ignoring it"),
						io_gdscurlayer);
				}
				io_gdslayernodes[io_gdscurlayer] = gen_drcprim;
			}
			io_gdslayernode = io_gdslayernodes[io_gdscurlayer];
			if (io_gdslayernode == gen_drcprim &&
				(io_gdscurstate[0]&GDSINIGNOREUKN) != 0)
			{
				io_gdslayerused = FALSE;
			} else
			{
				io_gdslayerused = io_gdslayervisible[io_gdscurlayer];
			}
			io_gdslayerwhich = io_gdscurlayer;

			io_gdsIgettoken();
			if (io_gdsImember(io_gdstoken, io_gdsImask_set))
			{
				io_gdsIgettoken();
				if (io_gdstokenvalue16 != 0)
				{
					io_gdscursublayer = *sublayer = io_gdstokenvalue16;
				}
			} else
			{
				io_gdsIonlyone(io_gdsIrecovery_set, GDS_XY);
				io_gdsIerror(24);
			}
		} else
		{
			io_gdsIonlyone(io_gdsIrecovery_set, GDS_XY);
			io_gdsIerror(22);
		}
	} else
	{
		io_gdsIonlyone(io_gdsIrecovery_set, GDS_XY);
		io_gdsIerror(23);
	}
}

NODEPROTO *io_gdsIfindproto(CHAR *name)
{
	REGISTER NODEPROTO *np;

	for(np = io_gdslibrary->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		if (namesame(name, np->protoname) == 0) return(np);
	return(NONODEPROTO);
}

void io_gdsIbeginstructure(void)
{
	CHAR create_time[IDSTRING+1], mod_time[IDSTRING+1];
	REGISTER void *infstr;

	if (io_gdstoken == GDS_BGNSTR)
	{
		io_gdsIgettoken();
		io_gdsIdetermine_time(create_time);
		io_gdsIdetermine_time(mod_time);
		if (io_gdstoken == GDS_STRNAME)
		{
			io_gdsIgettoken();
			if (io_gdstoken == GDS_IDENT)
			{
				/* look for this nodeproto */
				io_gdsstructure = io_gdsIfindproto(io_gdstokenstring);
				if (io_gdsstructure == NONODEPROTO)
				{
					/* create the proto */
					io_gdsstructure = us_newnodeproto(io_gdstokenstring, io_gdslibrary);
					if (io_gdsstructure == NONODEPROTO)
						io_gdsIerror(38);
					if (io_verbose < 0 && io_gdsfilesize > 0)
					{
						infstr = initinfstr();
						formatinfstr(infstr, _("Reading %s"), io_gdstokenstring);
						DiaSetTextProgress(io_gdsprogressdialog, returninfstr(infstr));
					} else if (io_verbose != 0) ttyputmsg(_("Reading %s"), io_gdstokenstring);
					if (io_gdslibrary->curnodeproto == NONODEPROTO)
						io_gdslibrary->curnodeproto = io_gdsstructure;
				}
			} else
			{
				io_gdsIcopyset(io_gdsIshape_set, io_gdsIrecovery_set);
				io_gdsIappendset(io_gdsIrecovery_set, GDS_ENDSTR);
				io_gdsIerror(12);
			}
		} else
		{
			io_gdsIcopyset(io_gdsIshape_set, io_gdsIrecovery_set);
			io_gdsIappendset(io_gdsIrecovery_set, GDS_ENDSTR);
			io_gdsIerror(13);
		}
	} else
	{
		io_gdsIcopyset(io_gdsIshape_set, io_gdsIrecovery_set);
		io_gdsIappendset(io_gdsIrecovery_set, GDS_ENDSTR);
		io_gdsIerror(14);
	}
}

void io_gdsIdetermine_property(void)
{
	CHAR propvalue[IDSTRING+1];

	io_gdsIgettoken();
	io_gdsIgettoken();
	if (io_gdstoken == GDS_PROPVALUE)
	{
		io_gdsIgettoken();
		(void)estrcpy(propvalue, io_gdstokenstring);

		/* add to the current structure as a variable? */
		io_gdsIgettoken();
	} else
	{
		io_gdsIonlyone(io_gdsIrecovery_set, GDS_ENDEL);
		io_gdsIerror(29);
	}
	/* close Attribute */
}

/* need more info ... */
void io_gdsIdetermine_node(void)
{
	INTBIG n;

	io_gdsIgettoken();
	io_gdsIunsupported(io_gdsIunsupported_set);
	if (io_gdstoken == GDS_LAYER)
	{
		io_gdsIgettoken();
		if (io_gdstoken == GDS_SHORT_NUMBER)
		{
			io_gdscurlayer = io_gdstokenvalue16;
			if (io_gdscurlayer >= MAXLAYERS)
			{
				ttyputmsg(_("GDS layer %ld is too high (limit is %d)"),
					io_gdscurlayer, MAXLAYERS-1);
				io_gdscurlayer = MAXLAYERS-1;
			}
			if (io_gdslayernodes[io_gdscurlayer] == NONODEPROTO)
			{
				ttyputmsg(_("GDS layer %ld unknown, using Generic:DRC"),
					io_gdscurlayer);
				io_gdslayernodes[io_gdscurlayer] = gen_drcprim;
			}
			io_gdslayernode = io_gdslayernodes[io_gdscurlayer];

			io_gdsIgettoken();
		}
	} else
	{
		io_gdsIonlyone(io_gdsIrecovery_set, GDS_ENDEL);
		io_gdsIerror(18);
	}

	/* should do something with node type??? */
	if (io_gdstoken == GDS_NODETYPE)
	{
		io_gdsIgettoken();
		io_gdsIgettoken();
	}

	/* make a dot */
	if (io_gdstoken == GDS_XY)
	{
		io_gdsIgettoken();
		io_gdsIdetermine_points(1, 1, &n);

		/* create the node */
		if (newnodeinst(io_gdslayernode, io_gdsvertex[0].px,
			io_gdsvertex[0].px, io_gdsvertex[0].py, io_gdsvertex[0].py, 0, 0,
				io_gdsstructure) == NONODEINST)
					io_gdsIerror(40);
	} else
	{
		io_gdsIonlyone(io_gdsIrecovery_set, GDS_ENDEL);
		io_gdsIerror(18);
	}
}

/* untested feature, I don't have a box type */
void io_gdsIdetermine_box(void)
{
	INTBIG n, layer, sublayer;

	io_gdsIgettoken();
	io_gdsIunsupported(io_gdsIunsupported_set);
	io_gdsIdetermine_layer(&layer, &sublayer);
	if (io_gdstoken == GDS_XY)
	{
		io_gdsIgettoken();
		io_gdsIdetermine_points(2, MAXPOINTS, &n);
		if (io_gdslayerused)
		{
			/* create the box */
			io_gdslayerrect[io_gdslayerwhich]++;
		    io_gdslayerpoly[io_gdslayerwhich]++;
			if (newnodeinst(io_gdslayernode, io_gdsvertex[0].px, io_gdsvertex[1].px,
				io_gdsvertex[0].py, io_gdsvertex[1].py , 0, 0,
					io_gdsstructure) == NONODEINST)
						io_gdsIerror(41);
		}
	} else
	{
		io_gdsIonlyone(io_gdsIrecovery_set, GDS_ENDEL);
		io_gdsIerror(18);
	}
}

void io_gdsIdetermine_shape(void)
{
	INTBIG n, layer, sublayer;
	shape_type perimeter, oclass;

	io_gdsIgettoken();
	io_gdsIunsupported(io_gdsIunsupported_set);
	io_gdsIdetermine_layer(&layer, &sublayer);
	io_gdsIgettoken();
	if (io_gdstoken == GDS_XY)
	{
		io_gdsIgettoken();
		io_gdsIdetermine_points(3, MAXPOINTS, &n);
		io_gdsIdetermine_boundary(n, &perimeter, &oclass);
		io_gdsIshape(n, perimeter, oclass);
	} else
	{
		io_gdsIonlyone(io_gdsIrecovery_set, GDS_ENDEL);
		io_gdsIerror(18);
	}
}

void io_gdsIdetermine_path(void)
{
	INTBIG endcode, n, layer, sublayer, off[8], thisAngle, lastAngle, nextAngle,
		bgnextend, endextend, fextend, textend, ang;
	INTBIG width, length, lx, ly, hx, hy, i, j, fx, fy, tx, ty, cx, cy;
	REGISTER NODEINST *ni;
	static POLYGON *poly = NOPOLYGON;

	endcode = 0;
	io_gdsIgettoken();
	io_gdsIunsupported(io_gdsIunsupported_set);
	io_gdsIdetermine_layer(&layer, &sublayer);
	io_gdsIgettoken();
	if (io_gdstoken == GDS_PATHTYPE)
	{
		io_gdsIgettoken();
		endcode = io_gdstokenvalue16;
		io_gdsIgettoken();
	}
	if (io_gdstoken == GDS_WIDTH)
	{
		io_gdsIgettoken();
		width = rounddouble((double)io_gdstokenvalue32 * io_gdsscale);
		io_gdsIgettoken();
	} else
	{
		width = 0;
	}
#ifdef NEWPATH
	bgnextend = endextend = (endcode == 0 || endcode == 4 ? 0 : width/2);
	if (io_gdstoken == GDS_BGNEXTN)
	{
		io_gdsIgettoken();
		if (endcode == 4)
			bgnextend = rounddouble((double)io_gdstokenvalue32 * io_gdsscale);
		io_gdsIgettoken();
	}
	if (io_gdstoken == GDS_ENDEXTN)
	{
		io_gdsIgettoken();
		if (endcode == 4)
			endextend = rounddouble((double)io_gdstokenvalue32 * io_gdsscale);
		io_gdsIgettoken();
	}
#endif
	if (io_gdstoken == GDS_XY)
	{
		io_gdsIgettoken();
		io_gdsIdetermine_points(2, MAXPOINTS, &n);
		if (io_gdslayerused)
			io_gdslayerpath[io_gdslayerwhich]++;

		/* construct the path */
		for (i=0; i < n-1; i++)
		{
			fx = io_gdsvertex[i].px;    fy = io_gdsvertex[i].py;
			tx = io_gdsvertex[i+1].px;  ty = io_gdsvertex[i+1].py;

			/* determine whether either end needs to be shrunk */
			fextend = textend = width / 2;
			thisAngle = figureangle(fx, fy, tx, ty);
			if (i > 0)
			{
				lastAngle = figureangle(io_gdsvertex[i-1].px, io_gdsvertex[i-1].py,
					io_gdsvertex[i].px, io_gdsvertex[i].py);
				if (abs(thisAngle-lastAngle) % 900 != 0)
				{
					ang = abs(thisAngle-lastAngle) / 10;
					if (ang > 180) ang = 360 - ang;
					if (ang > 90) ang = 180 - ang;
					fextend = tech_getextendfactor(width, ang);
				}
#ifdef NEWPATH
			} else
			{
				fextend = bgnextend;
#endif
			}
			if (i+1 < n-1)
			{
				nextAngle = figureangle(io_gdsvertex[i+1].px, io_gdsvertex[i+1].py,
					io_gdsvertex[i+2].px, io_gdsvertex[i+2].py);
				if (abs(thisAngle-nextAngle) % 900 != 0)
				{
					ang = abs(thisAngle-nextAngle) / 10;
					if (ang > 180) ang = 360 - ang;
					if (ang > 90) ang = 180 - ang;
					textend = tech_getextendfactor(width, ang);
				}
#ifdef NEWPATH
			} else
			{
				textend = endextend;
#endif
			}

			/* handle arbitrary angle path segment */
			if (io_gdslayerused)
			{
				io_gdslayerpoly[io_gdslayerwhich]++;

				/* get polygon */
				(void)needstaticpolygon(&poly, 4, io_tool->cluster);

				/* determine shape of segment */
				length = computedistance(fx, fy, tx, ty);
#ifndef NEWPATH
				if (endcode == 0) fextend = textend = 0;
#endif
				j = figureangle(fx, fy, tx, ty);
				tech_makeendpointpoly(length, width, j, fx, fy, fextend,
					tx, ty, textend, poly);

				/* make the node for this segment */
				if (isbox(poly, &lx, &hx, &ly, &hy))
				{
					if (newnodeinst(io_gdslayernode, lx, hx, ly, hy, 0, 0,
						io_gdsstructure) == NONODEINST)
							io_gdsIerror(42);
				} else
				{
					getbbox(poly, &lx, &hx, &ly, &hy);
					ni = newnodeinst(io_gdslayernode, lx, hx, ly, hy, 0, 0, io_gdsstructure);
					if (ni == NONODEINST)
						io_gdsIerror(42);

					/* set the shape into the node */
					cx = (lx + hx) / 2;
					cy = (ly + hy) / 2;
					for(j=0; j<4; j++)
					{
						off[j*2] = poly->xv[j] - cx;
						off[j*2+1] = poly->yv[j] - cy;
					}
					(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)off,
						VINTEGER|VISARRAY|(8<<VLENGTHSH));
				}
			}
		}
	} else
	{
		io_gdsIonlyone(io_gdsIrecovery_set, GDS_ENDEL);
		io_gdsIerror(25);
	}
}

void io_gdsIgetproto(CHAR *name)
{
	NODEPROTO *np;
	REGISTER void *infstr;

	/* scan for this proto */
	np = io_gdsIfindproto(name);
	if (np == NONODEPROTO)
	{
		/* FILO order, create this nodeproto */
		if ((np = us_newnodeproto(io_gdstokenstring, io_gdslibrary)) == NONODEPROTO)
			io_gdsIerror(43);
		if (io_verbose < 0 && io_gdsfilesize > 0)
		{
			infstr = initinfstr();
			formatinfstr(infstr, _("Reading %s"), io_gdstokenstring);
			DiaSetTextProgress(io_gdsprogressdialog, returninfstr(infstr));
		} else if (io_verbose != 0) ttyputmsg(_("Reading %s"), io_gdstokenstring);
	}

	/* set the reference node proto */
	io_gdssref = np;
}

void io_gdsIdetermine_sref(void)
{
	INTBIG n, angle, trans, cx, cy, ox, oy;
	XARRAY transform;
	double scale;

	io_gdsIgettoken();
	io_gdsIunsupported(io_gdsIunsupported_set);
	if (io_gdstoken == GDS_SNAME)
	{
		io_gdsIgettoken();
		io_gdsIgetproto(io_gdstokenstring);
		io_gdsIgettoken();
		if (io_gdstoken == GDS_STRANS)
			io_gdsIdetermine_orientation(&angle, &trans, &scale);
		else
		{
			angle = 0; trans = 0;
			scale = 1.0;
		}
		if (io_gdstoken == GDS_XY)
		{
			io_gdsIgettoken();
			io_gdsIdetermine_points(1, 1, &n);
			/* close Translate */
		} else
		{
			io_gdsIonlyone(io_gdsIrecovery_set, GDS_ENDEL);
			io_gdsIerror(17);
		}

		/* create the node */
		makeangle(angle, trans, transform);

		/* now transform the center */
		cx = (io_gdssref->highx + io_gdssref->lowx) / 2;
		cy = (io_gdssref->highy + io_gdssref->lowy) / 2;
		xform(cx, cy, &ox, &oy, transform);

		/* calculate the offset from the original center */
		ox -= cx;   oy -= cy;

		if ((io_gdsinstance = newnodeinst(io_gdssref,
			io_gdsvertex[0].px + io_gdssref->lowx + ox,
			io_gdsvertex[0].px + io_gdssref->highx + ox,
			io_gdsvertex[0].py + io_gdssref->lowy + oy,
			io_gdsvertex[0].py + io_gdssref->highy + oy,
			trans, angle, io_gdsstructure)) == NONODEINST)
				io_gdsIerror(44);
		if (io_gdscurstate[0]&GDSINEXPAND)
			io_gdsinstance->userbits |= NEXPAND;
		io_gdssrefcount++;
	} else
	{
		io_gdsIonlyone(io_gdsIrecovery_set, GDS_ENDEL);
		io_gdsIerror(16);
	}
}

void io_gdsIdetermine_aref(void)
{
	INTBIG n, ncols, nrows, angle, trans;
	double scale;

	io_gdsIgettoken();
	io_gdsIunsupported(io_gdsIunsupported_set);
	if (io_gdstoken == GDS_SNAME)
	{
		io_gdsIgettoken();

		/* get this nodeproto */
		io_gdsIgetproto(io_gdstokenstring);
		io_gdsIgettoken();
		if (io_gdstoken == GDS_STRANS)
			io_gdsIdetermine_orientation(&angle, &trans, &scale);
		else
		{
			angle = trans = 0;
			scale = 1.0;
		}
		if (io_gdstoken == GDS_COLROW)
		{
			io_gdsIgettoken();
			ncols = io_gdstokenvalue16;
			io_gdsIgettoken();
			nrows = io_gdstokenvalue16;
			io_gdsIgettoken();
		}
		if (io_gdstoken == GDS_XY)
		{
			io_gdsIgettoken();
			io_gdsIdetermine_points(3, 3, &n);
			io_gdsIstep(ncols, nrows, angle, trans);
		} else
		{
			io_gdsIonlyone(io_gdsIrecovery_set, GDS_ENDEL);
			io_gdsIerror(27);
		}
	} else
	{
		io_gdsIonlyone(io_gdsIrecovery_set, GDS_ENDEL);
		io_gdsIerror(28);
	}
}

void io_gdsIdetermine_justification(INTBIG *vert_just, INTBIG *horiz_just)
{
	INTBIG font_libno;

	io_gdsIgettoken();
	if (io_gdstoken == GDS_FLAGSYM)
	{
		font_libno = io_gdstokenflags & 0x0030;
		font_libno = font_libno >> 4;
		(*vert_just) = io_gdstokenflags & 0x000C;
		(*vert_just) = (*vert_just) >> 2;
		(*horiz_just) = io_gdstokenflags & 0x0003;
		io_gdsIgettoken();
	} else
	{
		io_gdsIonlyone(io_gdsIrecovery_set, GDS_XY);
		io_gdsIerror(27);
	}
}

void io_gdsIdetermine_text(void)
{
	INTBIG vert_just, horiz_just, layer, sublayer;
	CHAR textstring[TEXTLINE+1];
	INTBIG n, angle, trans;
	double scale;

	io_gdsIgettoken();
	io_gdsIunsupported(io_gdsIunsupported_set);
	io_gdsIdetermine_layer(&layer, &sublayer);
	io_gdsIgettoken();
	vert_just = -1;
	horiz_just = -1;
	if (io_gdstoken == GDS_PRESENTATION)
		io_gdsIdetermine_justification(&vert_just, &horiz_just);
	if (io_gdstoken == GDS_PATHTYPE)
	{
		io_gdsIgettoken();
		/* pathcode = io_gdstokenvalue16; */
		io_gdsIgettoken();
	}
	if (io_gdstoken == GDS_WIDTH)
	{
		io_gdsIgettoken();
		/* pathwidth = rounddouble((double)io_gdstokenvalue32 * io_gdsscale); */
		io_gdsIgettoken();
	}
	angle = trans = 0;
	scale = 1.0;
	for(;;)
	{
		if (io_gdstoken == GDS_STRANS)
		{
			io_gdsIdetermine_orientation(&angle, &trans, &scale);
			continue;
		}
		if (io_gdstoken == GDS_XY)
		{
			io_gdsIgettoken();
			io_gdsIdetermine_points(1, 1, &n);
			continue;
		}
		if (io_gdstoken == GDS_ANGLE)
		{
			io_gdsIgettoken();
			angle = (INTBIG)(io_gdsrealtokendp * 10.0);
			io_gdsIgettoken();
			continue;
		}
		if (io_gdstoken == GDS_STRING)
		{
			if (io_gdsrecordcount == 0) textstring[0] = '\0'; else
			{
				io_gdsIgettoken();
				(void)estrcpy(textstring, io_gdstokenstring);
			}
			io_gdsIgettoken();
			break;
		}
		if (io_gdstoken == GDS_MAG)
		{
			io_gdsIgettoken();
			io_gdsIgettoken();
			continue;
		}
		io_gdsIonlyone(io_gdsIrecovery_set, GDS_ENDEL);
		io_gdsIerror(26);
		break;
	}
	io_gdsItext(textstring, vert_just, horiz_just, angle, trans, scale);
}

void io_gdsIelement(BOOLEAN *empty_structure)
{
	while (io_gdsImember(io_gdstoken, io_gdsIshape_set))
	{
		switch (io_gdstoken)
		{
			case GDS_AREF:     io_gdsIdetermine_aref();    break;
			case GDS_SREF:     io_gdsIdetermine_sref();    break;
			case GDS_BOUNDARY: io_gdsIdetermine_shape();   break;
			case GDS_PATH:     io_gdsIdetermine_path();    break;
			case GDS_NODE:     io_gdsIdetermine_node();    break;
			case GDS_TEXTSYM:  io_gdsIdetermine_text();    break;
			case GDS_BOX:      io_gdsIdetermine_box();     break;
			default:                                       break;
		}
	}

	while (io_gdstoken == GDS_PROPATTR)
		io_gdsIdetermine_property();
	if (io_gdstoken != GDS_ENDEL)
	{
		io_gdsIonlyone(io_gdsIrecovery_set, GDS_BGNSTR);
		io_gdsIerror(15);
	}
	(*empty_structure) = FALSE;
}

void io_gdsIstructure(void)
{
	BOOLEAN empty_structure;

	io_gdsIbeginstructure();
	empty_structure = TRUE;
	io_gdsIgettoken();
	while (io_gdstoken != GDS_ENDSTR)
	{
		io_gdsIelement(&empty_structure);
		io_gdsIgettoken();
	}
	if (empty_structure)
	{
		ttyputmsg(x_("%s"), TRANSLATE(io_gdsIerror_string[11]));
		ttyputerr(_("Error at block number %5d and offset %4d"),
			io_gdsblockcount, io_gdsbufferindex);
		return;
	}

	db_boundcell(io_gdsstructure, &io_gdsstructure->lowx, &io_gdsstructure->highx,
		&io_gdsstructure->lowy, &io_gdsstructure->highy);
}

BOOLEAN io_gdsIload(FILE *input)
{
	io_gdsIinit();
	io_gdsIgettoken();
	io_gdsIheader();
	io_gdsIgettoken();
	io_gdsIlibrary();
	io_gdsIgettoken();
	while (io_gdsImember(io_gdstoken, io_gdsIoption_set))
		switch (io_gdstoken)
	{
		case GDS_REFLIBS:
			io_gdsIreflibs();     break;
		case GDS_FONTS:
			io_gdsIfonts();       break;
		case GDS_ATTRTABLE:
			io_gdsIattrtable();   break;
		case GDS_GENERATIONS:
			io_gdsIbackup();      break;
		default:                  break;
	}
	while (io_gdstoken != GDS_UNITS)
		io_gdsIgettoken();
	io_gdsIunits();
	io_gdsIgettoken();
	while (io_gdstoken != GDS_ENDLIB)
	{
		io_gdsIstructure();
		io_gdsIgettoken();
	}
	return(FALSE);
}

/* module: io_readgdslibrary
 * function:  Electric entry-point in gdsII stream library read.
 * inputs:
 * lib - the library to read
 * position: 0 for single-file input,
 *           1 for first of series
 *           2 for midseriese
 *           3 for end of series
 * returns false on success, true on error
 */
BOOLEAN io_readgdslibrary(LIBRARY *lib, INTBIG position)
{
	FILE *input;
	BOOLEAN err;
	INTBIG i;
	INTBIG count;
	CHAR *filename;
	NODEPROTO *np;

	io_gdslibrary = lib;

	/* get current state of I/O tool */
	io_gdscurstate = io_getstatebits();

	/* get the gds file */
	if ((input = xopen(io_gdslibrary->libfile, io_filetypegds, x_(""), &filename)) == NULL)
	{
		ttyputerr(_("File %s not found"), io_gdslibrary->libfile);
		return(TRUE);
	}

	io_gdsstream.fp = input;
	io_gdsfilesize = filesize(input);
	if (io_verbose < 0 && io_gdsfilesize > 0)
	{
		io_gdsprogressdialog = DiaInitProgress(0, 0);
		if (io_gdsprogressdialog == 0)
		{
			xclose(input);
			return(TRUE);
		}
		DiaSetProgress(io_gdsprogressdialog, 0, io_gdsfilesize);
	}
	io_gdsbytesread = 0;

	/* setup the error return location */
	if (setjmp(io_gdsenv) != 0)
	{
		xclose(input);
		if (io_verbose < 0 && io_gdsfilesize > 0) DiaDoneProgress(io_gdsprogressdialog);
		return(TRUE);
	}

	/* set default objects */
	io_gdslayernode = gen_drcprim;
	if (position == 0 || position == 1)
	{
		io_gdsIOnceinit();
		io_gdssrefcount = 0;
	}

	err = io_gdsIload(input);

	/* output layer use table */
	if (io_verbose == 1)
	{
		if (position == 0 || position == 3)
		{
			if (position == 0)
				ttyputmsg(_("%s information:"), skippath(io_gdslibrary->libfile)); else
					ttyputmsg(_("Summary information:"));
			ttyputmsg(x_("  %ld %s"), io_gdssrefcount, makeplural(x_("SREF"), io_gdssrefcount));
			for (i = 0; i < MAXLAYERS; i++)
			{
				if (io_gdslayertext[i] || io_gdslayerpoly[i])
				{
					if (i < el_curtech->layercount)
					{
						ttyputmsg(_("  %ld objects on layer %ld (%s)"),
							io_gdslayerpoly[i]+io_gdslayertext[i], i,
								describenodeproto(io_gdslayernodes[i]));
					} else
					{
						ttyputmsg(_("  %ld objects on layer %ld"),
							io_gdslayerpoly[i]+io_gdslayertext[i], i);
					}
				}
			}
		}
	}

	/* finish up */
	xclose(input);

	if (position == 0 || position == 3)
	{
		if (io_verbose < 0 && io_gdsfilesize > 0)
		{
			DiaSetTextProgress(io_gdsprogressdialog, _("Cleaning up..."));
			DiaSetProgress(io_gdsprogressdialog, 0, io_gdssrefcount);
		}

		/* determine which cells need to be recomputed */
		count = io_gdssrefcount;
		for (i = 0; i < MAXLAYERS; i++)
			count += io_gdslayertext[i] + io_gdslayerpoly[i];
		for(np = io_gdslibrary->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = 0;
		io_gdsrecomputed = 0;
		for(np = io_gdslibrary->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			if (np->temp1 == 0)
				io_gdsIrecompute(np, count);

	}
	if (io_verbose < 0 && io_gdsfilesize > 0) DiaDoneProgress(io_gdsprogressdialog);
	return(err);
}

void io_gdsIrecompute(NODEPROTO *np, INTBIG count)
{
	REGISTER INTBIG cx, cy;
	CHAR numsofar[50];
	INTBIG ox, oy;
	XARRAY trans;
	REGISTER NODEINST *ni;

	/* rebuild the geometry database, note that this is faster then
	   just updating the SREF info */
	/* first clear the geometry structure */
	db_freertree(np->rtree);
	(void)geomstructure(np);
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex == 0)
		{
			if (ni->proto->temp1 == 0)
				io_gdsIrecompute(ni->proto, count);
			if ((ni->highx-ni->lowx) != (ni->proto->highx-ni->proto->lowx) ||
				(ni->highy-ni->lowy) != (ni->proto->highy-ni->proto->lowy))
			{
				makeangle(ni->rotation, ni->transpose, trans);

				/* now transform the center */
				cx = (ni->proto->highx + ni->proto->lowx) / 2;
				cy = (ni->proto->highy + ni->proto->lowy) / 2;
				xform(cx, cy, &ox, &oy, trans);

				/* calculate the offset from the original center */
				ox -= cx;   oy -= cy;
				ni->lowx += (ni->proto->lowx + ox);
				ni->highx += (ni->proto->highx + ox);
				ni->lowy += (ni->proto->lowy + oy);
				ni->highy += (ni->proto->highy + oy);
				boundobj(ni->geom, &(ni->geom->lowx), &(ni->geom->highx),
					&(ni->geom->lowy), &(ni->geom->highy));
			}
		}

		/* now link into the geometry list */
		linkgeom(ni->geom, np);

		io_gdsrecomputed++;
		if ((io_gdsrecomputed%1000) == 0 && io_verbose < 0 && io_gdsfilesize > 0)
		{
			(void)esnprintf(numsofar, 50, _("Cleaned up %ld instances..."), io_gdsrecomputed);
			DiaSetTextProgress(io_gdsprogressdialog, numsofar);
			if (count > 0)
				DiaSetProgress(io_gdsprogressdialog, io_gdsrecomputed, count);
		}
	}

	db_boundcell(np, &np->lowx, &np->highx, &np->lowy, &np->highy);
	np->temp1 = 1;
}
