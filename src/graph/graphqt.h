/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design Systems
 *
 * File: graphqt.h
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

#ifndef GRAPHQT_H
#define GRAPHQT_H

#include "global.h"

#include <qapplication.h>
#include <qbitmap.h>
#include <qcursor.h>
#include <qframe.h>
#include <qimage.h>
#include <qmainwindow.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpopupmenu.h>
#include <qprocess.h>
#include <qvbox.h>

#ifdef MACOSX
/* CHARPIXMAP - use 32-bit QPixmap instead of 1-bit QBitmap for glyph drawing */
#  define CHARPIXMAP
#  define MESSMENU /* menu in messages window */
#  define DOUBLESELECT
#endif

//#define USEMDI

#ifndef Q_ASSERT
#  define Q_ASSERT(x) ASSERT(x)
#endif
#ifndef Q_CHECK_PTR
#  define Q_CHECK_PTR(p) CHECK_PTR(p)
#endif

#define GTRACE          0       /* etrace flag of usual graphics trace */
#define GTRACE_CONT     0       /* etrace flag of usual graphics trace (continue line) */

struct Ipopupmenu;
class QHBox;
class QLabel;
class QLineEdit;
class QTextEdit;
class QPaintDevice;
class QWorkspace;
class EFont;
class EMessages;
class EStatusItem;

class EMessages: public QVBox
{
	Q_OBJECT
public:
    EMessages( QWidget *parent = 0 );
	void clear();
	char *getString(char *prompt);
	void putString(char *s, BOOLEAN important);
	void setFont();
	void setColor( QColor &bg, QColor &fg );
public slots:
    void lineEntered();
    void setIcons();
protected:
	void windowActivationChange( bool oldActive );
    void keyPressEvent( QKeyEvent *e );
    bool eventFilter( QObject *watched, QEvent *e ); 
private:
    QTextEdit *scroll;
    QHBox *input;
    QLabel *prompt;
    QLineEdit *line;
    bool stopped;
};

void gra_initfaces( QStringList &facelist );
void gra_termfaces();

class GraphicsDraw: public QWidget
{
    Q_OBJECT
public:
    GraphicsDraw( QWidget *parent );
    void focusInEvent( QFocusEvent *e );
    void focusOutEvent( QFocusEvent *e );
    void paintEvent( QPaintEvent *e );
    void keyPressEvent( QKeyEvent *e );
	void keyReleaseEvent( QKeyEvent *e );
    void mouseDoubleClickEvent( QMouseEvent *e );
    void mouseMoveEvent( QMouseEvent *e );
    void mousePressEvent( QMouseEvent *e );
    void mouseReleaseEvent( QMouseEvent *e );
    void resizeEvent( QResizeEvent *e );
    void wheelEvent( QWheelEvent * e);
    bool eventFilter( QObject *watched, QEvent *e ); 
protected:
	void windowActivationChange( bool oldActive );
private:
    void keepCursorPos( QPoint pos );
    void buttonPressed( QPoint pos, INTSML but );
    void recalcSize();
public:
    WINDOWFRAME *wf;
    int colorvalue[256];
    /* offscreen data is in "image" and drawn by "graphdraw.c" */
    QImage image;
private:
	bool metaheld; // Unix Meta key mod doesn't come through in key modifier state
};

class EMainWindow: public QMainWindow
{
	Q_OBJECT
public:
	EMainWindow( QWidget *parent, char *name, WFlags f, BOOLEAN status );
	~EMainWindow();
    void makepdmenu( Ipopupmenu *pm, INTSML value);
    INTSML pulldownindex( Ipopupmenu *pm );
    void pulldownmenuload();
    void nativemenurename( Ipopupmenu *pm, INTBIG pindex);
	void addStatusItem( int stretch, char *str );
	void removeStatusItem( int i );
	void changeStatusItem( int i, char *str );
public slots:
    void menuAction( int id );
private:
    EStatusItem *statusItems[100];
    QPopupMenu **pulldownmenus;		/* list of Windows pulldown menus */
    char       **pulldowns;		/* list of Electric pulldown menu names */
    INTSML       pulldownmenucount;	/* number of pulldown menus */
};

class EApplicationWindow: public EMainWindow
{
	Q_OBJECT
public:
	EApplicationWindow();
#ifdef USEMDI
	QWorkspace *ws;
#endif
};

class EApplication: public QApplication
{
    Q_OBJECT
public:
    EApplication( int &argc, char **argv);
    ~EApplication();
	bool notify( QObject *receiver, QEvent *e );
	EMessages *messages;
	EApplicationWindow *mw;
    static char *localize (QString qstr);
    static char *localizeFilePath (QString filePath, bool addSeparator );
    int translatekey(QKeyEvent *e, INTBIG *special);
#ifdef DOUBLESELECT
	void activationChange();
#endif
    
    QBitmap textbits;
#ifdef CHARPIXMAP
    QPixmap charbits;
#else
    QBitmap charbits;
#endif
    EFont *curfont;
    QStringList facelist;
    QString defface;
    QFont fixedfont;

    QBitmap programicon;
    QCursor realcursor;
    QCursor nomousecursor;
    QCursor drawcursor;
    QCursor nullcursor;
    QCursor menucursor;
    QCursor handcursor;
    QCursor techcursor;
    QCursor ibeamcursor;
    QCursor lrcursor;
    QCursor udcursor;
    
    /* trackcursor */
    INTSML waitforpush;
    BOOLEAN (*whileup)(INTBIG x, INTBIG y);
    void (*whendown)(void);
    BOOLEAN (*eachdown)(INTBIG x, INTBIG y);
    BOOLEAN (*eachchar)(INTBIG x, INTBIG y, INTSML c);
    
    /* modalloop */
    BOOLEAN (*charhandler)(INTSML chr, INTBIG special);
    BOOLEAN (*buttonhandler)(INTBIG x, INTBIG y, INTBIG but);

public slots:
	void toolTimeSlice(); 
};

class EStatusItem: public QFrame
{
    Q_OBJECT
public:
    EStatusItem( QWidget *parent, char *name = 0 );
    void setText( const QString &text );
protected:
    void	 drawContents( QPainter *p );
private:
    QString stext;
};

class EPopupMenu: public QPopupMenu
{
	Q_OBJECT
public:
	EPopupMenu( QWidget *parent, struct Ipopupmenu *menu );
private:
	void keyPressEvent( QKeyEvent *e );
	struct Ipopupmenu *menu;
};

class EProcessPrivate: public QProcess
{
	Q_OBJECT
	friend class EProcess;
public:
	EProcessPrivate();
private:
	void idle();
	QByteArray buf;
	uint bufp;
};

extern EApplication *gra;

#endif
