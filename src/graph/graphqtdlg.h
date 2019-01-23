/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design Systems
 *
 * File: graphqtdlg.h
 * Dialogs header file
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

#ifndef GRAPHQTDLG_H
#define GRAPHQTDLG_H

#include "global.h"

#include <qcheckbox.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qlistbox.h>

#if defined(USEQUI) && QT_VERSION >= 300
#  define QTSTRETCH
#endif
#ifdef QTSTRETCH
#  include <qwidgetfactory.h>
#endif

#ifndef Q_ASSERT
#define Q_ASSERT(x) ASSERT(x)
#endif
#ifndef Q_CHECK_PTR
#define Q_CHECK_PTR(p) CHECK_PTR(p)
#endif

#define MAXLOCKS              3   /* maximum locked pairs of scroll lists */

struct Idialogitem;
class QSignalMapper;
class EScrollField;
class EDialogPrivate;

class EDialogPrivate: public QDialog
{
    friend class EDialog;
    Q_OBJECT
public:
#if QT_VERSION >= 300
    EDialogPrivate( QWidget *parent, const char *name = 0 );
	~EDialogPrivate();
    void init( EDialog *dia, bool modal );
#else
    EDialogPrivate( QWidget *parent, const char *name = 0, bool modal = FALSE );
    void init( EDialog *dia );
#endif
    void showMe();
    static char *localize( QString qstr );
#ifdef QTSTRETCH
    static EDialogPrivate *loadUi( QWidget *parent, char *uiFile, bool ext = FALSE );
    bool checkChildren();
#endif
private:
    QWidget *items[100];
    int numlocks;
    QScrollBar *lock1[MAXLOCKS], *lock2[MAXLOCKS], *lock3[MAXLOCKS];
    QSignalMapper *mapper;
    QPoint iniPos;
    QFont userFont;
public:
    EDialog *dia;
	EDialogPrivate *nexttdialog;
    INTBIG itemHit;

public slots:
    void action(int id);
protected:
    void closeEvent( QCloseEvent *e );
    void keyPressEvent( QKeyEvent *e );
	bool eventFilter( QObject *watched, QEvent *e );
private:
    static void getdialogcoordinates(RECTAREA *rect, INTBIG *x, INTBIG *y, INTBIG *wid, INTBIG *hei);
    static INTBIG scaledialogcoordinate(INTBIG x);
};

class ECheckField: public QCheckBox
{
    Q_OBJECT
public:
    ECheckField( QString &text, QWidget *parent );
    ~ECheckField();
    bool isChecked();
public slots:
    void setChecked( bool value );
private:
    bool checked;
};

class EScrollField: public QListBox
{
    Q_OBJECT
public:
    EScrollField( QWidget *parent );
protected:
    void keyPressEvent( QKeyEvent *e );
};

class EIconField: public QLabel
{
    Q_OBJECT
public:
    EIconField( QWidget *parent, Idialogitem *item );
    void setBits( uchar *bits ); 
signals:
    void clicked();
protected:
    void mousePressEvent( QMouseEvent *e );
};

class EUserDrawnField: public QWidget
{
    Q_OBJECT
public:
    EUserDrawnField( QWidget *parent, Idialogitem *item );
signals:
    void clicked();
    void doubleClicked();
protected:
    void mouseDoubleClickEvent( QMouseEvent *e );
    void mouseMoveEvent( QMouseEvent *e );
    void mousePressEvent( QMouseEvent *e );
    void mouseReleaseEvent( QMouseEvent *e );
    void paintEvent( QPaintEvent *e );
public:
    Idialogitem *it;
	EDialog *dia;
    void (*redispRoutine)(RECTAREA *rect, void *dia);
    bool buttonPressed;
    void (*eachdown)(INTBIG x, INTBIG y);
    bool doubleClickAllowed;
};

#ifdef QTSTRETCH
class EWidgetFactory: public QWidgetFactory
{
public:
    EWidgetFactory();
    QWidget* createWidget( const QString& className, QWidget *parent, const char *name) const;    
};
#endif

#endif
