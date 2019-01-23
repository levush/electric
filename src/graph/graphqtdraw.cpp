/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design Systems
 *
 * File: graphqt.cpp
 * Qt Window System interface
 * Written by: Dmitry Nadezhin, Instutute for Design Problems in Microelectronics, Russian Academy of Sciences
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
 * info@staticfreesoft.com
 */

#include "global.h"
#include "graphqt.h"
#include "egraphics.h"

#define FLUSHTICKS     60						/* ticks between display flushes */

/****** fonts ******/

#define MAXCACHEDFONTS 200
#define FONTHASHSIZE   211		/* must be prime */
#define MAXFONTCHAR    128
#define MAXCHARBITS     32              /* maximum height of cached characters */

class EFont;

class EFontChar
{
public:
	EFontChar( EFont *font, QChar aCh );
	QChar ch;
	QRect bounding;
	QImage glyphImage;
	int width;
	EFontChar *next;
};

class EFont
{
public:
	EFont( QFont fnt );
	~EFont();
	QFont   font;
	QFontMetrics fontMetrics;
	EFontChar *getFontChar(QChar ch);
private:
	EFontChar *chars[MAXFONTCHAR];
};

static EFont   *gra_fixedfont = 0, *gra_menufont = 0, *gra_tmpfont = 0;
static EFont   *gra_fontcache[MAXCACHEDFONTS];
static INTBIG	gra_numallfaces = 0, gra_numusedfaces = 0;
static char   **gra_allfacelist, **gra_usedfacelist;

typedef struct
{
	EFont   *font;
	INTBIG   face;
	INTBIG   italic;
	INTBIG   bold;
	INTBIG   underline;
	INTBIG   size;
} FONTHASH;

static FONTHASH gra_fonthash[FONTHASHSIZE];

static INTBIG        gra_textrotation = 0;
static char         *gra_textbitsdata;				/* buffer for converting text to bits */
static INTBIG        gra_textbitsdatasize = 0;		/* size of "gra_textbitsdata" */
static char        **gra_textbitsrowstart;			/* pointers to "gra_textbitsdata" */
static INTBIG        gra_textbitsrowstartsize = 0;	/* size of "gra_textbitsrowstart" */
static BOOLEAN       gra_texttoosmall = FALSE;		/* TRUE if text too small to draw */

static EFont *gra_createtextfont(INTBIG fontSize, QString face, INTBIG italic, INTBIG bold,
	INTBIG underline);
static EFont *gra_gettextfont(WINDOWPART *win, TECHNOLOGY *tech, UINTBIG *descript);

/****** EFont ******/
EFontChar::EFontChar( EFont *font, QChar aCh )
{
	ch = aCh;
	bounding = font->fontMetrics.boundingRect( ch );
	width = font->fontMetrics.width( ch );

	/* load the image array */
	if (bounding.width() > gra->charbits.width() || bounding.height() > gra->charbits.height()) {
		gra->charbits.resize( QMAX( bounding.width(), gra->charbits.width() ), QMAX( bounding.height(), gra->charbits.height() ) );
	}

	QPainter p( &gra->charbits );
	p.eraseRect( 0, 0, bounding.width(), bounding.height() );
	p.setFont( font->font );
	p.drawText( -bounding.left(), -bounding.top(), ch );
	p.end();

	QImage image = gra->charbits.convertToImage();
#ifdef CHARPIXMAP
    if (!bounding.isEmpty())
    {
        QImage boundImage = image.copy( 0, 0, bounding.width(), bounding.height() );
        glyphImage = boundImage.convertDepth( 1, Qt::ThresholdDither );
    }
#if 0
    if (aCh == 'N')
    {
        QCString fam = font->font.family().latin1();
        QCString raw = font->font.rawName().latin1();
        printf("Char <%c>%o Font %s(%s) depth=%d(%d)\n",
            aCh.latin1(), aCh.latin1(), (const char*)fam, (const char*)raw, image.depth(), glyphImage.depth());
        for (int i = 0; i < bounding.height(); i++)
        {
            printf("\t");
            for (int j = 0; j < bounding.width(); j++) printf(" %d", qGray( image.pixel(j, i) ) );
            printf("\n");
        }
        for (int i = 0; i < bounding.height(); i++)
        {
            printf("\t");
            for (int j = 0; j < bounding.width(); j++) printf("%c", (glyphImage.pixelIndex(j, i) ? 'X' : ' '));
            printf("\n");
        }
    }
#endif
#else
	glyphImage = image.copy( 0, 0, bounding.width(), bounding.height() );
#endif
}

EFont::EFont( QFont fnt )
  : font( fnt ), fontMetrics( fnt )
{
	for (int i = 0; i < MAXFONTCHAR; i++) chars[i] = 0;
}

EFont::~EFont()
{
	for (int i = 0; i < MAXFONTCHAR; i++)
	{
		while (chars[i])
		{
			EFontChar *e = chars[i];
			chars[i] = e->next;
			delete e;
		}	
	}
}

EFontChar *EFont::getFontChar(QChar ch)
{
	int index = ch % MAXFONTCHAR;
	EFontChar *e;
	for (e = chars[index]; e; e = e->next)
	{
		if (e->ch == ch) return e;
	}
	e = new EFontChar( this, ch );
	e->next = chars[index];
	chars[index] = e;
	return e;
}

/****** rectangle saving ******/
#define NOSAVEDBOX ((SAVEDBOX *)-1)
typedef struct Isavedbox
{
	QPixmap     pix;
        GraphicsDraw *draw;
	QPoint      orig;
} SAVEDBOX;

/******************** GRAPHICS LINES ********************/

/*
 * Routine to draw a line in the graphics window.
 */
void screendrawline(WINDOWPART *win, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, GRAPHICS *desc,
	INTBIG texture)
{
	gra_drawline( win, x1, y1, x2, y2, desc, texture );
}

/*
 * Routine to invert bits of the line in the graphics window
 */
void screeninvertline(WINDOWPART *win, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2)
{
	gra_invertline( win, x1, y1, x2, y2 );
}

/******************** GRAPHICS POLYGONS ********************/

/*
 * Routine to draw a polygon in the graphics window.
 */
void screendrawpolygon(WINDOWPART *win, INTBIG *x, INTBIG *y, INTBIG count, GRAPHICS *desc)
{
	gra_drawpolygon( win, x, y, count, desc );
}

/******************** GRAPHICS BOXES ********************/

/*
 * Routine to draw a box in the graphics window.
 */
void screendrawbox(WINDOWPART *win, INTBIG lowx, INTBIG highx, INTBIG lowy, INTBIG highy,
	GRAPHICS *desc)
{
	gra_drawbox( win, lowx, highx, lowy, highy, desc );
}

/*
 * routine to invert the bits in the box from (lowx, lowy) to (highx, highy)
 */
void screeninvertbox(WINDOWPART *win, INTBIG lowx, INTBIG highx, INTBIG lowy, INTBIG highy)
{
	gra_invertbox( win, lowx, highx, lowy, highy );
}

/*
 * routine to move bits on the display starting with the area at
 * (sx,sy) and ending at (dx,dy).  The size of the area to be
 * moved is "wid" by "hei".
 */
void screenmovebox(WINDOWPART *win, INTBIG sx, INTBIG sy, INTBIG wid, INTBIG hei,
	INTBIG dx, INTBIG dy)
{
	gra_movebox( win, sx, sy, wid, hei, dx, dy );
}

/*
 * routine to save the contents of the box from "lx" to "hx" in X and from
 * "ly" to "hy" in Y.  A code is returned that identifies this box for
 * overwriting and restoring.  The routine returns negative if there is a error.
 */
INTBIG screensavebox(WINDOWPART *win, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy)
{
	return gra_savebox( win, lx, hx, ly, hy );
}

/*
 * routine to shift the saved box "code" so that it is restored in a different
 * lcoation, offset by (dx,dy)
 */
void screenmovesavedbox(INTBIG code, INTBIG dx, INTBIG dy)
{
	gra_movesavedbox( code, dx, dy );
}

/*
 * routine to restore saved box "code" to the screen.  "destroy" is:
 *  0   restore box, do not free memory
 *  1   restore box, free memory
 * -1   free memory
 */
void screenrestorebox(INTBIG code, INTBIG destroy)
{
	gra_restorebox( code, destroy );
}

/******************** GRAPHICS TEXT ********************/

/*
 * Routine to find face with name "facename" and to return
 * its index in list of used fonts. If font was not used before,
 * then it is appended to list of used fonts.
 * If font "facename" is not available on the system, -1 is returned. 
 */
INTBIG screenfindface(char *facename)
{
	int i;

	for (i = 1; i < gra_numusedfaces; i++)
		if (namesame(facename, gra_usedfacelist[i]) == 0) return i;
	if (gra_numusedfaces >= VTMAXFACE) return(-1);
	for (i = 1; i < gra_numallfaces; i++)
		if (namesame(facename, gra_allfacelist[i]) == 0) break;
	if (i >= gra_numallfaces) return(-1);
	gra_usedfacelist[gra_numusedfaces] = gra_allfacelist[i];
	return(gra_numusedfaces++);
}

/*
 * Routine to return the number of typefaces used (when "all" is FALSE)
 * or available on the system (when "all" is TRUE)
 * and to return their names in the array "list".
 * "screenfindface
 */
INTBIG screengetfacelist(char ***list, BOOLEAN all)
{
	if (all)
	{
		*list = gra_allfacelist;
		return(gra_numallfaces);
	} else
	{
		*list = gra_usedfacelist;
		return(gra_numusedfaces);
	}
}
        
/*
 * Routine to return the default typeface used on the screen.
 */
char *screengetdefaultfacename(void)
{
	return(EApplication::localize(gra->defface));
}

void screensettextinfo(WINDOWPART *win, TECHNOLOGY *tech, UINTBIG *descript)
{
	gra_textrotation = TDGETROTATION(descript);
	gra->curfont = gra_gettextfont(win, tech, descript);
}

static EFont *gra_gettextfont(WINDOWPART *win, TECHNOLOGY *tech, UINTBIG *descript)
{
	gra_texttoosmall = FALSE;
	int font = TDGETSIZE(descript);
	if (font == TXTEDITOR)
	{
		if (gra_fixedfont == 0) gra_fixedfont = new EFont( gra->fixedfont );
		return gra_fixedfont;
	}
	if (gra_menufont == 0) gra_menufont = new EFont( gra->font() );
	if (font == TXTMENU) return gra_menufont;
	/*
		GraphicsMainWindow *qwin = (GraphicsMainWindow*)win->frame->qt;
		gra->curfont = qwin->menuBar()->font();
	*/

	int size = truefontsize(font, win, tech);
	if (size < 1)
	{
		gra_texttoosmall = TRUE;
		return(0);
	}
	if (size < MINIMUMTEXTSIZE) size = MINIMUMTEXTSIZE;
#if 0
    /* Increase font size to match Unix X11 port */
    size += 4;
#endif
	int face = TDGETFACE(descript);
	int italic = TDGETITALIC(descript);
	int bold = TDGETBOLD(descript);
	int underline = TDGETUNDERLINE(descript);
	gra_textrotation = TDGETROTATION(descript);
	if (face == 0 && italic == 0 && bold == 0 && underline == 0)
	{
		if (size >= MAXCACHEDFONTS) size = MAXCACHEDFONTS-1;
		if (gra_fontcache[size] == 0) {
			gra_fontcache[size] = gra_createtextfont(size, gra->defface, 0, 0, 0);
			if (gra_fontcache[size] == 0) return gra_menufont;
		}
		return(gra_fontcache[size]);
	} else
	{
		UINTBIG hash = size + 3*italic + 5*bold + 7*underline + 11*face;
		hash %= FONTHASHSIZE;
		for(int i=0; i<FONTHASHSIZE; i++)
		{	
			if (gra_fonthash[hash].font == 0) break;
			if (gra_fonthash[hash].face == face && gra_fonthash[hash].size == size &&
				gra_fonthash[hash].italic == italic && gra_fonthash[hash].bold == bold &&
				gra_fonthash[hash].underline == underline)
					return(gra_fonthash[hash].font);
			hash++;
			if (hash >= FONTHASHSIZE) hash = 0;
		}
		QString facename = gra->defface;
		if (face > 0 || face < gra_numusedfaces) facename = QString::fromLocal8Bit( gra_usedfacelist[face] );
		EFont *theFont = gra_createtextfont(size, facename, italic, bold, underline);
		if (theFont == 0) return gra_menufont;
		if (gra_fonthash[hash].font == 0)
		{
			gra_fonthash[hash].font = theFont;
			gra_fonthash[hash].face = face;
			gra_fonthash[hash].size = size;
			gra_fonthash[hash].italic = italic;
			gra_fonthash[hash].bold = bold;
			gra_fonthash[hash].underline = underline;
		} else
		{
			delete gra_tmpfont;
			gra_tmpfont = theFont;
		}
		return(theFont);
	}
}

static EFont *gra_createtextfont(INTBIG fontSize, QString face, INTBIG italic, INTBIG bold,
	INTBIG underline)
{
	QFont font( face, fontSize );
	font.setStyleStrategy( QFont::NoAntialias );
	font.setItalic( italic );
	font.setBold( bold );
	font.setUnderline( underline );
	return new EFont( font );
}

void screengettextsize(WINDOWPART *win, char *str, INTBIG *x, INTBIG *y)
{
	Q_UNUSED( win );

	INTBIG len, wid, hei;

	len = strlen(str);
	if (len == 0 || gra_texttoosmall)
	{
		*x = *y = 0;
		return;
	}

	/* determine the size of the text */
	QFontMetrics fontMetrics = gra->curfont->fontMetrics;
	QString qstr = QString::fromLocal8Bit( str );
	QRect bounding = fontMetrics.boundingRect( qstr );

	/* fixing qt bug */
	QRect bounding1 = fontMetrics.boundingRect( qstr[0] );
	if (bounding1.left() < bounding.left() )
		bounding.setLeft( bounding1.left() );

	wid = bounding.width();
	hei = QMAX(bounding.bottom(), fontMetrics.descent()) - bounding.top() + 1;

	switch (gra_textrotation)
	{
		case 0:			/* normal */
			*x = wid;
			*y = hei;
			break;
		case 1:			/* 90 degrees counterclockwise */
			*x = -hei;
			*y = wid;
			break;
		case 2:			/* 180 degrees */
			*x = -wid;
			*y = -hei;
			break;
		case 3:			/* 90 degrees clockwise */
			*x = hei;
			*y = -wid;
			break;
	}
}

void screendrawtext(WINDOWPART *win, INTBIG atx, INTBIG aty, char *s, GRAPHICS *desc)
{
	if (gra_texttoosmall) return;
	gra_drawtext( win, atx, aty, gra_textrotation, s, desc );
}

/*
 * Routine to convert the string "msg" (to be drawn into window "win") into an
 * array of pixels.  The size of the array is returned in "wid" and "hei", and
 * the pixels are returned in an array of character vectors "rowstart".
 * The routine returns nonzero if the operation cannot be done.
 */
BOOLEAN gettextbits(WINDOWPART *win, char *msg, INTBIG *wid, INTBIG *hei, UCHAR1 ***rowstart)
{
	Q_UNUSED( win );

	REGISTER INTBIG len, datasize;

	/* quit if string is null */
	*wid = *hei = 0;
	len = strlen(msg);
	if (len == 0 || gra_texttoosmall) return(FALSE);

	/* determine the size of the text */
	QFontMetrics fontMetrics = gra->curfont->fontMetrics;
	QString qstr = QString::fromLocal8Bit( msg );
	QRect bounding = fontMetrics.boundingRect( qstr );

#if QT_VERSION < 0x030005
	/* fixing qt bug */
	QRect bounding1 = fontMetrics.boundingRect( qstr[0] );
	if (bounding1.left() < bounding.left() )
		bounding.setLeft( bounding1.left() );
#endif

	*wid = bounding.width();
	*hei = QMAX(bounding.bottom(), fontMetrics.descent()) - bounding.top() + 1;

	/* allocate space for this */
	datasize = *wid * *hei;
	if (datasize > gra_textbitsdatasize)
	{
		if (gra_textbitsdatasize > 0) efree((char *)gra_textbitsdata);
		gra_textbitsdatasize = 0;

		gra_textbitsdata = (char *)emalloc(datasize, us_tool->cluster);
		if (gra_textbitsdata == 0) return(TRUE);
		gra_textbitsdatasize = datasize;
	}
	if (*hei > gra_textbitsrowstartsize)
	{
		if (gra_textbitsrowstartsize > 0) efree((char *)gra_textbitsrowstart);
		gra_textbitsrowstartsize = 0;
		gra_textbitsrowstart = (char **)emalloc(*hei * (sizeof (UCHAR1 *)), us_tool->cluster);
		if (gra_textbitsrowstart == 0) return(TRUE);
		gra_textbitsrowstartsize = *hei;
	}

	/* load the row start array */
	for(int y=0; y < *hei; y++)
	{
		gra_textbitsrowstart[y] = &gra_textbitsdata[*wid * y];
	}
	*rowstart = (UCHAR1 **)gra_textbitsrowstart;

	if (*hei > MAXCHARBITS)
	{
		/* load the image array */
		if (*wid > gra->textbits.width() || *hei > gra->textbits.height()) {
			gra->textbits.resize( QMAX( *wid, gra->textbits.width() ), QMAX( *hei, gra->textbits.height() ) );
		}

		QPainter p( &gra->textbits );
		p.eraseRect( 0, 0, *wid, *hei );
		p.setFont( gra->curfont->font );
		p.drawText( -bounding.left(), -bounding.top(), qstr );
		p.end();

		QImage image = gra->textbits.convertToImage();
		for (int i=0; i<*hei; i++) {
			for (int j=0; j < *wid; j++) gra_textbitsrowstart[i][j] = image.pixelIndex( j, i );
		}
	} else
	{
		int i, j;
		int x = -bounding.left();

		for (i = 0; i < *hei; i++) {
			if (gra->curfont->font.underline() &&
			    i + bounding.top() >= gra->curfont->fontMetrics.underlinePos() &&
			    i + bounding.top() < gra->curfont->fontMetrics.underlinePos() + gra->curfont->fontMetrics.lineWidth())
			{
				for (int j=0; j < *wid; j++) gra_textbitsrowstart[i][j] = 1;
			} else
			{
				for (int j=0; j < *wid; j++) gra_textbitsrowstart[i][j] = 0;
			}
		}
		for (uint k = 0; k < qstr.length(); k++) {
			EFontChar *efc = gra->curfont->getFontChar( qstr[k] );
			int xl = x + efc->bounding.left();
			int jmin = (xl >= 0 ? 0 : -xl);
			int jmax = efc->glyphImage.width();
			if (xl + jmax > *wid) jmax = *wid - xl;
			for (i = 0; i < efc->glyphImage.height(); i++) {
			        int y = efc->bounding.top() + i - bounding.top();
				if (y < 0 || y >= *hei) continue;
				for (j = jmin; j < jmax; j++) {
					if (efc->glyphImage.pixelIndex( j, i )) {
						gra_textbitsrowstart[y][xl + j] = 1;
					}
				}	
			}
			x += efc->width;
		}
	}

	return(FALSE);
}

/******************** CIRCLE DRAWING ********************/
void screendrawcircle(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius, GRAPHICS *desc)
{
	gra_drawcircle( win, atx, aty, radius, desc ); 
}

void screendrawthickcircle(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius,
	GRAPHICS *desc)
{
	gra_drawthickcircle( win, atx, aty, radius, desc );
}

/******************** DISC DRAWING ********************/

/*
 * routine to draw a filled-in circle of radius "radius"
 */
void screendrawdisc(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius, GRAPHICS *desc)
{
	gra_drawdisc( win, atx, aty, radius, desc );
}

/******************** ARC DRAWING ********************/

/*
 * draws a thin arc centered at (centerx, centery), clockwise,
 * passing by (x1,y1) and (x2,y2)
 */
void screendrawcirclearc(WINDOWPART *win, INTBIG centerx, INTBIG centery, INTBIG p1_x, INTBIG p1_y,
	INTBIG p2_x, INTBIG p2_y, GRAPHICS *desc)
{
	gra_drawcirclearc( win, centerx, centery, p1_x, p1_y, p2_x, p2_y, FALSE, desc );
}

/*
 * draws a thick arc centered at (centerx, centery), clockwise,
 * passing by (x1,y1) and (x2,y2)
 */
void screendrawthickcirclearc(WINDOWPART *win, INTBIG centerx, INTBIG centery, INTBIG p1_x, INTBIG p1_y,
	INTBIG p2_x, INTBIG p2_y, GRAPHICS *desc)
{
	gra_drawcirclearc( win, centerx, centery, p1_x, p1_y, p2_x, p2_y, TRUE, desc );
}

/******************** GRID CONTROL ********************/

/*
 * fast grid drawing routine
 */
void screendrawgrid(WINDOWPART *win, POLYGON *obj)
{
	gra_drawgrid( win, obj );
}

void gra_initfaces( QStringList &facelist )
{
    uint i;

    for(i=0; i<MAXCACHEDFONTS; i++) gra_fontcache[i] = 0;
    for(i=0; i<FONTHASHSIZE; i++) gra_fonthash[i].font = 0;

    gra_numallfaces = facelist.count() + 1;
    gra_allfacelist = (char **)emalloc(gra_numallfaces * (sizeof (char *)), us_tool->cluster);
    Q_CHECK_PTR( gra_allfacelist );
    (void)allocstring(&gra_allfacelist[0], _("DEFAULT FACE"), us_tool->cluster);
    for(i=0; i<facelist.count(); i++) {
        char *facename = gra->localize( facelist[i] );
        (void)allocstring(&gra_allfacelist[i+1], facename, us_tool->cluster);
    }
    gra_usedfacelist = (char **)emalloc(VTMAXFACE * (sizeof (char *)), us_tool->cluster);
    Q_CHECK_PTR( gra_usedfacelist );
    gra_numusedfaces = 1;
    gra_usedfacelist[0] = gra_allfacelist[0];
}

void gra_termfaces()
{
    int i;

    for(i=0; i<MAXCACHEDFONTS; i++) {
	if(gra_fontcache[i]) delete gra_fontcache[i];
    }
    for(i=0; i<FONTHASHSIZE; i++) {
	if(gra_fonthash[i].font) delete gra_fonthash[i].font;
    }

    for(i=0; i<gra_numallfaces; i++) efree((char *)gra_allfacelist[i]);
    if (gra_numallfaces > 0) efree((char *)gra_allfacelist);
    if (gra_numusedfaces > 0) efree((char *)gra_usedfacelist);

    if (gra_textbitsdatasize > 0) efree((char *)gra_textbitsdata);
    if (gra_textbitsrowstartsize > 0) efree((char *)gra_textbitsrowstart);
}

