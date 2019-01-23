/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: egraphics.h
 * Graphical definitions
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

/******************************* GRAPHICS ********************************/

/* bit map colors (in GRAPHICS->bits) */
#define LAYERN   0000			/* nothing                               */
#define LAYERH   0001			/* highlight color and bit plane         */
#define LAYEROE  0002			/* opaque layer escape bit               */
#define LAYERO   0176			/* opaque layers                         */
#define LAYERT1  0004			/* transparent layer 1                   */
#define LAYERT2  0010			/* transparent layer 2                   */
#define LAYERT3  0020			/* transparent layer 3                   */
#define LAYERT4  0040			/* transparent layer 4                   */
#define LAYERT5  0100			/* transparent layer 5                   */
#define LAYERG   0200			/* grid line color and bit plane         */
#define LAYERA   0777			/* everything                            */

/* color map colors (in GRAPHICS->col) */
#define ALLOFF   0000			/* no color                              */
#define HIGHLIT  0001			/* highlight color and bit plane         */
#define COLORT1  0004			/* transparent color 1                   */
#define COLORT2  0010			/* transparent color 2                   */
#define COLORT3  0020			/* transparent color 3                   */
#define COLORT4  0040			/* transparent color 4                   */
#define COLORT5  0100			/* transparent color 5                   */
#define GRID     0200			/* grid line color and bit plane         */

#define WHITE    0002			/* white                                 */
#define BLACK    0006			/* black                                 */
#define RED      0012			/* red                                   */
#define BLUE     0016			/* blue                                  */
#define GREEN    0022			/* green                                 */
#define CYAN     0026			/* cyan                                  */
#define MAGENTA  0032			/* magenta                               */
#define YELLOW   0036			/* yellow                                */
#define CELLTXT  0042			/* cell and port names                   */
#define CELLOUT  0046			/* cell outline                          */
#define WINBOR   0052			/* window border color                   */
#define HWINBOR  0056			/* highlighted window border color       */
#define MENBOR   0062			/* menu border color                     */
#define HMENBOR  0066			/* highlighted menu border color         */
#define MENTXT   0072			/* menu text color                       */
#define MENGLY   0076			/* menu glyph color                      */
#define CURSOR   0102			/* cursor color                          */
#define GRAY     0106			/* gray                                  */
#define ORANGE   0112			/* orange                                */
#define PURPLE   0116			/* purple                                */
#define BROWN    0122			/* brown                                 */
#define LGRAY    0126			/* light gray                            */
#define DGRAY    0132			/* dark gray                             */
#define LRED     0136			/* light red                             */
#define DRED     0142			/* dark red                              */
#define LGREEN   0146			/* light green                           */
#define DGREEN   0152			/* dark green                            */
#define LBLUE    0156			/* light blue                            */
#define DBLUE    0162			/* dark blue                             */
/*               0166 */		/* unassigned                            */
/*               0172 */		/* unassigned                            */
/*               0176 */		/* unassigned (and should stay that way) */

/* drawing styles (in GRAPHICS->style) */
#define NATURE       1			/* choice between solid and patterned */
#define SOLIDC       0			/*   solid colors */
#define PATTERNED    1			/*   stippled with "raster" */
#define INVISIBLE    2			/* don't draw this layer */
#define INVTEMP      4			/* temporary for INVISIBLE bit */
#define OUTLINEPAT 010			/* if NATURE is PATTERNED, outline it */

/* variables with the acutal colors to use for special graphics */
extern INTBIG el_maplength;		/* number of entries in color map */
extern INTBIG el_colcelltxt;	/* color to use for cell text and port names */
extern INTBIG el_colcell;		/* color to use for cell outline */
extern INTBIG el_colwinbor;		/* color to use for window border */
extern INTBIG el_colhwinbor;	/* color to use for highlighted window border */
extern INTBIG el_colmenbor;		/* color to use for menu border */
extern INTBIG el_colhmenbor;	/* color to use for highlighted menu border */
extern INTBIG el_colmentxt;		/* color to use for menu text */
extern INTBIG el_colmengly;		/* color to use for menu glyphs */
extern INTBIG el_colcursor;		/* color to use for cursor */

/******************************* POLYGONS ********************************/

/* drawing styles (in POLYGON->style) */
		/* polygons */
#define FILLED          0			/* closed polygon, filled in                 */
#define CLOSED          1			/* closed polygon, outline                   */
		/* rectangles */
#define FILLEDRECT      2			/* closed rectangle, filled in               */
#define CLOSEDRECT      3			/* closed rectangle, outline                 */
#define CROSSED         4			/* closed rectangle, outline crossed         */
		/* lines */
#define OPENED          5			/* open outline, solid                       */
#define OPENEDT1        6			/* open outline, dotted                      */
#define OPENEDT2        7			/* open outline, dashed                      */
#define OPENEDT3        8			/* open outline, thicker                     */
#define OPENEDO1        9			/* open outline pushed by 1                  */
#define VECTORS        10			/* vector endpoint pairs, solid              */
		/* curves */
#define CIRCLE         11			/* circle at [0] radius to [1]               */
#define THICKCIRCLE    12			/* thick circle at [0] radius to [1]         */
#define DISC           13			/* filled circle                             */
#define CIRCLEARC      14			/* arc of circle at [0] ends [1] and [2]     */
#define THICKCIRCLEARC 15			/* thick arc of circle at [0] ends [1] and [2] */
		/* text */
#define TEXTCENT       16			/* text at center                            */
#define TEXTTOP        17			/* text below top edge                       */
#define TEXTBOT        18			/* text above bottom edge                    */
#define TEXTLEFT       19			/* text to right of left edge                */
#define TEXTRIGHT      20			/* text to left of right edge                */
#define TEXTTOPLEFT    21			/* text to lower-right of top-left corner    */
#define TEXTBOTLEFT    22			/* text to upper-right of bottom-left corner */
#define TEXTTOPRIGHT   23			/* text to lower-left of top-right corner    */
#define TEXTBOTRIGHT   24			/* text to upper-left of bottom-right corner */
#define TEXTBOX        25			/* text that fits in box (may shrink)        */
		/* miscellaneous */
#define GRIDDOTS       26			/* grid dots in the window                   */
#define CROSS          27			/* cross                                     */
#define BIGCROSS       28			/* big cross                                 */

/* text font sizes (in VARIABLE, NODEINST, PORTPROTO, and POLYGON->textdescription) */
#define TXTPOINTS        077		/* points from 1 to TXTMAXPOINTS */
#define TXTPOINTSSH        0		/* right-shift of TXTPOINTS */
#define TXTMAXPOINTS      63
#define TXTSETPOINTS(p)   ((p)<<TXTPOINTSSH)
#define TXTGETPOINTS(p)   (((p)&TXTPOINTS)>>TXTPOINTSSH)

#define TXTQLAMBDA    077700		/* quarter-lambda from 0.25 to TXTMAXQLAMBDA/4 */
#define TXTQLAMBDASH       6		/* right-shift of TXTLAMBDA */
#define TXTMAXQLAMBDA    511
#define TXTSETQLAMBDA(ql) ((ql)<<TXTQLAMBDASH)
#define TXTGETQLAMBDA(ql) (((ql)&TXTQLAMBDA)>>TXTQLAMBDASH)

#define TXTEDITOR     077770		/* fixed-width text for text editing */
#define TXTMENU       077771		/* text for menu selection */

/******************** PROTOTYPES ********************/

void    gra_drawline(WINDOWPART *win, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2,
			GRAPHICS *desc, INTBIG texture); 
void    gra_invertline(WINDOWPART *win, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2);
void    gra_drawpolygon(WINDOWPART *win, INTBIG *x, INTBIG *y, INTBIG count, GRAPHICS *desc);
void    gra_drawbox(WINDOWPART *win, INTBIG lowx, INTBIG highx, INTBIG lowy, INTBIG highy,
			GRAPHICS *desc);
void    gra_invertbox(WINDOWPART *win, INTBIG lowx, INTBIG highx, INTBIG lowy, INTBIG highy);
void    gra_movebox(WINDOWPART *win, INTBIG sx, INTBIG sy, INTBIG wid, INTBIG hei,
			INTBIG dx, INTBIG dy);
INTBIG  gra_savebox(WINDOWPART *win, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy);
void    gra_movesavedbox(INTBIG code, INTBIG dx, INTBIG dy);
BOOLEAN gra_restorebox(INTBIG code, INTBIG destroy);
void    gra_drawtext(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG rotation,
			CHAR *s, GRAPHICS *desc);
void    gra_drawcircle(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius, GRAPHICS *desc);
void    gra_drawthickcircle(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius,
			GRAPHICS *desc);
void    gra_drawdisc(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius, GRAPHICS *desc);
void    gra_drawcirclearc(WINDOWPART *win, INTBIG centerx, INTBIG centery, INTBIG p1_x, INTBIG p1_y,
			INTBIG p2_x, INTBIG p2_y, BOOLEAN thick, GRAPHICS *desc);
void    gra_drawgrid(WINDOWPART *win, POLYGON *obj);
void    gra_setrect(WINDOWFRAME *wf, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy);
void    gra_termgraph(void);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
