/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: eio.h
 * Input/output tool: header file
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

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
extern "C"
{
#endif

#include <setjmp.h>

/* #define REPORTCONVERSION 1 */		/* uncomment to report on library conversion */

/* successive versions of the binary file have smaller numbers */
#define MAGIC13              -1597		/* current magic number: version 13 */
#define MAGIC12              -1595		/* older magic number: version 12 */
#define MAGIC11              -1593		/* older magic number: version 11 */
#define MAGIC10              -1591		/* older magic number: version 10 */
#define MAGIC9               -1589		/* older magic number: version 9 */
#define MAGIC8               -1587		/* older magic number: version 8 */
#define MAGIC7               -1585		/* older magic number: version 7 */
#define MAGIC6               -1583		/* older magic number: version 6 */
#define MAGIC5               -1581		/* older magic number: version 5 */
#define MAGIC4               -1579		/* older magic number: version 4 */
#define MAGIC3               -1577		/* older magic number: version 3 */
#define MAGIC2               -1575		/* older magic number: version 2 */
#define MAGIC1               -1573		/* oldest magic number: version 1 */

/* I/O formats: */
#define FBINARY                  0		/* Binary */
#define FBINARYNOBACKUP          1		/* Binary without backup */
#define FCIF                     2		/* CIF */
#define FDXF                     3		/* DXF format */
#define FEDIF                    4		/* Electronic Design Interchange Format */
#define FGDS                     5		/* Calma GDS-II format */
#define FHPGL                    6		/* HPGL format (plotting) */
#define FL                       7		/* L format (Silicon Compilers) */
#define FPOSTSCRIPT              8		/* PostScript (plotting) */
#define FPRINTEDPOSTSCRIPT       9		/* Printed PostScript (plotting) */
#define FQUICKDRAW              10		/* Quickdraw (plotting) */
#define FSDF                    11		/* SDF */
#define FTEXT                   12		/* Text */
#define FVHDL                   13		/* VHDL */
#define FSUE                    14		/* SUE */
#define FLEF                    15		/* LEF (Library Exchange Format) */
#define FDEF                    16		/* DEF (Design Exchange Format) */
#define FSKILL                  17		/* SKILL (CADENCE command language) */
#define FEAGLE                  18		/* EAGLE (CadSoft) netlist */
#define FPADS                   19		/* PADS netlist */
#define FECAD                   20		/* ECAD netlist */

/* tool:inout.IO_state: */
#define NUMIOSTATEBITWORDS       2		/* number of words for these bits */

#define CIFINSQUARE             01		/* 0: bit set for CIF input to square wires */
#define CIFOUTEXACT             02		/* 0: bit set for CIF output to mimic screen */
#define CIFOUTMERGE             04		/* 0: bit set for CIF output to merge polygons */
#define CIFOUTADDDRC           010		/* 0: bit set for CIF output to include DRC layer */
#define IODEBUG                020		/* 0: bit set to debug input */
#define PLOTFOCUS              040		/* 0: bit set to focus plot output */
#define PLOTDATES             0100		/* 0: bit set to include dates in plot output */
#define CIFOUTNORMALIZE       0200		/* 0: bit set for normalized CIF coordinates */
#define GDSOUTMERGE           0400		/* 0: bit set for GDS output to merge polgons */
#define GDSOUTADDDRC         01000		/* 0: bit set for GDS output to include DRC layer */
#define EPSPSCRIPT           02000		/* 0: bit set for Encapsulated PostScript output */
#define HPGL2                04000		/* 0: bit set for HPGL/2 output instead of HPGL */
#define EDIFSCHEMATIC       010000		/* 0: bit set for EDIF write schematic instead of netlist */
#define DXFALLLAYERS        020000		/* 0: bit set for DXF to read all layers instead of technology list */
#define DXFFLATTENINPUT     040000		/* 0: bit set for DXF to flatten input */
#define DEFNOPHYSICAL      0100000		/* 0: bit set to ignore DEF physical data */
#define GDSINTEXT          0200000		/* 0: bit set for GDS input of text */
#define GDSINEXPAND        0400000		/* 0: bit set for GDS expansion of cells */
#define GDSINARRAYS	      01000000		/* 0: bit set for GDS array instantiation */
#define PSCOLOR1          02000000		/* 0: first bit for color PostScript output */
#define PSPLOTTER         04000000		/* 0: bit set for PostScript plotting (continuous roll) */
#define PSROTATE         010000000		/* 0: bit set for PostScript rotation (by 90 degrees) */
#define CIFOUTNOTOPCALL  020000000		/* 0: bit set for CIF output to not call top cell */
#define GDSINIGNOREUKN   040000000		/* 0: bit set for GDS input to ignore unknown layers */
#define DEFNOLOGICAL    0100000000		/* 0: bit set to ignore DEF logical data */
#define BINOUTBACKUP    0600000000		/* 0: bits that determine binary output backups */
#define BINOUTNOBACK    0000000000		/*   for no backup of binary output files */
#define BINOUTONEBACK   0200000000		/*   for one backup of binary output files */
#define BINOUTFULLBACK  0400000000		/*   for full backup of binary output files */
#define CHECKATWRITE   01000000000		/* 0: bit set for database check before write */
#define PSCOLOR2       02000000000		/* 0: second bit for color PostScript output */
#define GDSOUTUC       04000000000		/* 0: bit set to force GDS output to be upper case */
#define PLOTFOCUSDPY  010000000000		/* 0: bit set to focus plot output on display */
#define SKILLNOHIER   020000000000		/* 0: bit set to ignore subcells in SKILL */
#define CIFRESHIGH              01		/* 1: bit set for CIF resolution errors to highlight */
#define SKILLFLATHIER           02		/* 1: bit set to flatten hierarchy in SKILL */
#define CDLNOBRACKETS           04		/* 1: bit set to convert brackets in CDL */
#define GDSOUTPINS             010		/* 1: bit set for GDS to write pins at exports */
#define PSAUTOROTATE           020		/* 1: bit set to automatically rotate PS to fit best */
#define SUEUSE4PORTTRANS       040		/* 1: bit set to make 4-port transistors in Sue input */

#define DEFAULTPSWIDTH         638		/* default PostScript page width (8.5" at 75dpi) */
#define DEFAULTPSHEIGHT        825		/* default PostScript page height (11.0" at 75dpi) */
#define DEFAULTPSMARGIN         56		/* default PostScript page height (0.75" at 75dpi) */


#define NOFAKECELL          ((FAKECELL *)-1)

typedef struct Ifakecell
{
	CHAR              *cellname;			/* name of this fakecell */
	struct Inodeproto *firstincell;			/* first nodeproto in list */
} FAKECELL;

/* miscellaneous */
extern FILE        *io_fileout;			/* channel for output */
extern jmp_buf      io_filerror;		/* nonlocal jump when I/O fails */
extern INTBIG       io_cifbase;			/* index used when writing CIF */
extern INTBIG       io_postscriptfilenamekey;/* key for "IO_postscript_filename" */
extern INTBIG       io_postscriptfiledatekey;/* key for "IO_postscript_filedate" */
extern INTBIG       io_postscriptepsscalekey;/* key for "IO_postscript_EPS_scale" */
extern INTBIG       io_verbose;			/* 0: silent  1:verbose  -1:display cells during input */
extern INTBIG       io_filetypeblib;	/* Binary library disk file descriptor */
extern INTBIG       io_filetypecif;		/* CIF disk file descriptor */
extern INTBIG       io_filetypedef;		/* DEF disk file descriptor */
extern INTBIG       io_filetypedxf;		/* DXF disk file descriptor */
extern INTBIG       io_filetypeeagle;	/* EAGLE netlist disk file descriptor */
extern INTBIG       io_filetypeecad;	/* ECAD netlist disk file descriptor */
extern INTBIG       io_filetypeedif;	/* EDIF disk file descriptor */
extern INTBIG       io_filetypegds;		/* GDS disk file descriptor */
extern INTBIG       io_filetypehpgl;	/* HPGL disk file descriptor */
extern INTBIG       io_filetypehpgl2;	/* HPGL2 disk file descriptor */
extern INTBIG       io_filetypel;		/* L disk file descriptor */
extern INTBIG       io_filetypelef;		/* LEF disk file descriptor */
extern INTBIG       io_filetypepads;	/* PADS netlist disk file descriptor */
extern INTBIG       io_filetypeps;		/* PostScript disk file descriptor */
extern INTBIG       io_filetypeskill;	/* SKILL commands disk file descriptor */
extern INTBIG       io_filetypesdf;		/* SDF disk file descriptor */
extern INTBIG       io_filetypesue;		/* SUE disk file descriptor */
extern INTBIG       io_filetypetlib;	/* Text library disk file descriptor */
extern INTBIG       io_filetypevhdl;	/* VHDL disk file descriptor */
extern INTBIG       io_libinputrecursivedepth;	/* for recursing when reading dependent libraries */
extern INTBIG       io_libinputreadmany;/* nonzero if reading dependent libraries */
extern void        *io_inputprogressdialog;

/* prototypes for tool interface */
void io_init(INTBIG*, CHAR1*[], TOOL*);
void io_done(void);
void io_set(INTBIG, CHAR*[]);
INTBIG io_request(CHAR*, va_list);
void io_slice(void);

/* prototypes for intratool interface */
void       io_buildcellgrouppointersfromnames(LIBRARY*);
INTBIG    *io_getstatebits(void);
void       io_setstatebits(INTBIG *bits);
PORTPROTO *io_convertoldportname(CHAR *portname, NODEPROTO *np);
void       io_fixnewlib(LIBRARY *lib, void *dia);
NODEPROTO *io_convertoldprimitives(TECHNOLOGY *tech, CHAR *name);
INTBIG     io_setuptechorder(TECHNOLOGY *tech);
void       io_queuereadlibraryannouncement(LIBRARY *lib);
INTBIG     io_nextplotlayer(INTBIG i);
INTBIG     io_getoutputbloat(CHAR *layer);
BOOLEAN    io_doreadbinlibrary(LIBRARY *lib, BOOLEAN newprogress);
BOOLEAN    io_doreadtextlibrary(LIBRARY *lib, BOOLEAN newprogress);
BOOLEAN    io_getareatoprint(NODEPROTO *np, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy, BOOLEAN reduce);
BOOLEAN    io_readbinlibrary(LIBRARY *lib);
BOOLEAN    io_readciflibrary(LIBRARY *lib);
BOOLEAN    io_readdeflibrary(LIBRARY *lib);
BOOLEAN    io_readdxflibrary(LIBRARY *lib);
BOOLEAN    io_readediflibrary(LIBRARY *lib);
BOOLEAN    io_readgdslibrary(LIBRARY *lib, INTBIG position);
BOOLEAN    io_readleflibrary(LIBRARY *lib);
BOOLEAN    io_readsdflibrary(LIBRARY *lib);
BOOLEAN    io_readsuelibrary(LIBRARY *lib);
BOOLEAN    io_readtextlibrary(LIBRARY *lib);
BOOLEAN    io_readvhdllibrary(LIBRARY *lib);
BOOLEAN    io_writebinlibrary(LIBRARY *lib, BOOLEAN nobackup);
BOOLEAN    io_writeciflibrary(LIBRARY *lib);
BOOLEAN    io_writedxflibrary(LIBRARY *lib);
BOOLEAN    io_writeeaglelibrary(LIBRARY *lib);
BOOLEAN    io_writeecadlibrary(LIBRARY *lib);
BOOLEAN    io_writeediflibrary(LIBRARY *lib);
BOOLEAN    io_writegdslibrary(LIBRARY *lib);
BOOLEAN    io_writehpgllibrary(LIBRARY *lib);
BOOLEAN    io_writellibrary(LIBRARY *lib);
BOOLEAN    io_writeleflibrary(LIBRARY *lib);
BOOLEAN    io_writepadslibrary(LIBRARY *lib);
BOOLEAN    io_writepostscriptlibrary(LIBRARY *lib, BOOLEAN printit);
BOOLEAN    io_writequickdrawlibrary(LIBRARY *lib);
BOOLEAN    io_writeskilllibrary(LIBRARY *lib);
BOOLEAN    io_writetextlibrary(LIBRARY *lib);
void       io_freebininmemory(void);
void       io_freebinoutmemory(void);
void       io_freecifinmemory(void);
void       io_freecifparsmemory(void);
void       io_freecifoutmemory(void);
void       io_freedxfmemory(void);
void       io_freegdsoutmemory(void);
void       io_freepostscriptmemory(void);
void       io_freesdfimemory(void);
void       io_freesuememory(void);
void       io_freetextinmemory(void);
void       io_freedefimemory(void);
void       io_freelefimemory(void);
void       io_freeedifinmemory(void);
void       io_freegdsinmemory(void);
void       io_initskill(void);
void       io_initdef(void);
void       io_initdxf(void);
void       io_initcif(void);
void       io_initedif(void);
void       io_initgds(void);
void       io_pscolorplot(NODEPROTO *np, BOOLEAN epsformat, BOOLEAN useplotter,
			INTBIG pagewid, INTBIG pagehei, INTBIG pagemargin);
void       io_pswrite(CHAR *s, ...);
void       io_pswritestring(CHAR*);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
