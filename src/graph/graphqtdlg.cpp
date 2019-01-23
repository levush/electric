/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design Systems
 *
 * File: graphqtdlg.cpp
 * Dialogs implementation on Qt
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
#include "graphqtdlg.h"
#include "edialogs.h"

#include <qapplication.h>
#include <qbitmap.h>
#include <qbuttongroup.h>
#include <qclipboard.h>
#include <qcombobox.h>
#include <qcursor.h>
#include <qlineedit.h>
#include <qobjectlist.h>
#include <qpainter.h>
#include <qprogressbar.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qsignalmapper.h>
#include <qtimer.h>
#include <qwmatrix.h>

#define GTRACE          0       /* etrace flag of usual graphics trace */

/****** the dialogs ******/

#define MAXSCROLLMULTISELECT 1000

#define DIALOGNUM            16
#define DIALOGDEN            12
#define MAXICONS             20

/* maximal time (in ms) to wait for next event */
#define MAXEVENTTIME         100

static EDialogPrivate *gra_firstactivemodaldialog = 0;
static EDialogPrivate *gra_firstmodelessdialog = 0;

extern EApplication *gra;

static INTBIG gra_dialogstringpos;
extern "C"
{
	static int gra_stringposascending(const void *e1, const void *e2);
}


/****************************** C Dialog API  ******************************/

/*
 * Routine to initialize a dialog described by "dialog".
 * Returns the address of the dialog object (0 if dialog cannot be initialized).
 */
void *DiaInitDialog(DIALOG *dialog)
{
	return(new EDialogModal( dialog ));
}

/*
 * Routine to handle actions and return the next item hit.
 */
INTBIG DiaNextHit(void *vdia)
{
	EDialog *dia = (EDialog *)vdia;
	return(dia->nextHit());
}

void DiaDoneDialog(void *vdia)
{
	EDialog *dia = (EDialog *)vdia;
	delete dia;
}

/*
 * Routine to change the size of the dialog
 */
void DiaResizeDialog(void *vdia, INTBIG wid, INTBIG hei)
{
	EDialog *dia = (EDialog *)vdia;
	dia->resizeDialog( wid, hei );
}

/*
 * Routine to forces dialog be displayed on top of other windows
 */
void DiaBringToTop(void *vdia)
{
	EDialog *dia = (EDialog *)vdia;
	dia->bringToTop();
}

/*
 * Routine to set the text in item "item" to "msg"
 */
void DiaSetText(void *vdia, INTBIG item, CHAR *msg)
{
	EDialog *dia = (EDialog *)vdia;
	dia->setText(item, msg);
}

/*
 * Routine to return the text in item "item"
 */
CHAR *DiaGetText(void *vdia, INTBIG item)
{
	EDialog *dia = (EDialog *)vdia;
	return(dia->getText(item));
}

/*
 * Routine to set the value in item "item" to "value"
 */
void DiaSetControl(void *vdia, INTBIG item, INTBIG value)
{
	EDialog *dia = (EDialog *)vdia;
	dia->setControl(item, value);
}

/*
 * Routine to return the value in item "item"
 */
INTBIG DiaGetControl(void *vdia, INTBIG item)
{
	EDialog *dia = (EDialog *)vdia;
	return(dia->getControl(item));
}

/*
 * Routine to check item "item" to make sure that there is
 * text in it.  If so, it returns true.  Otherwise it beeps and returns false.
 */
BOOLEAN DiaValidEntry(void *vdia, INTBIG item)
{
	EDialog *dia = (EDialog *)vdia;
	return(dia->validEntry(item));
}

/*
 * Routine to dim item "item"
 */
void DiaDimItem(void *vdia, INTBIG item)
{
	EDialog *dia = (EDialog *)vdia;
	dia->dimItem(item);
}

/*
 * Routine to un-dim item "item"
 */
void DiaUnDimItem(void *vdia, INTBIG item)
{
	EDialog *dia = (EDialog *)vdia;
	dia->unDimItem(item);
}

/*
 * Routine to change item "item" to be a message rather
 * than editable text
 */
void DiaNoEditControl(void *vdia, INTBIG item)
{
	EDialog *dia = (EDialog *)vdia;
	dia->noEditControl(item);
}

/*
 * Routine to change item "item" to be editable text rather
 * than a message
 */
void DiaEditControl(void *vdia, INTBIG item)
{
	EDialog *dia = (EDialog *)vdia;
	dia->editControl(item);
}

void DiaOpaqueEdit(void *vdia, INTBIG item)
{
	EDialog *dia = (EDialog *)vdia;
	dia->opaqueEdit(item);
}

/*
 * Routine to cause item "item" to be the default button
 */
void DiaDefaultButton(void *vdia, INTBIG item)
{
	EDialog *dia = (EDialog *)vdia;
	dia->defaultButton(item);
}

/*
 * Routine to change the icon in item "item" to be the 32x32 bitmap (128 bytes) at "addr".
 */
void DiaChangeIcon(void *vdia, INTBIG item, UCHAR1 *addr)
{
	EDialog *dia = (EDialog *)vdia;
	dia->changeIcon(item, addr);
}

/*
 * Routine to change item "item" into a popup with "count" entries
 * in "names".
 */
void DiaSetPopup(void *vdia, INTBIG item, INTBIG count, CHAR **names)
{
	EDialog *dia = (EDialog *)vdia;
	dia->setPopup(item, count, names);
}

/*
 * Routine to change popup item "item" so that the current entry is "entry".
 */
void DiaSetPopupEntry(void *vdia, INTBIG item, INTBIG entry)
{
	EDialog *dia = (EDialog *)vdia;
	dia->setPopupEntry(item, entry);
}

/*
 * Routine to return the current item in popup menu item "item".
 */
INTBIG DiaGetPopupEntry(void *vdia, INTBIG item)
{
	EDialog *dia = (EDialog *)vdia;
	return(dia->getPopupEntry(item));
}

void DiaInitTextDialog(void *vdia, INTBIG item, BOOLEAN (*toplist)(CHAR **), CHAR *(*nextinlist)(void),
	void (*donelist)(void), INTBIG sortpos, INTBIG flags)
{
	EDialog *dia = (EDialog *)vdia;
	dia->initTextDialog(item, toplist, nextinlist, donelist, sortpos, flags);
}

void DiaLoadTextDialog(void *vdia, INTBIG item, BOOLEAN (*toplist)(CHAR **), CHAR *(*nextinlist)(void),
	void (*donelist)(void), INTBIG sortpos)
{
	EDialog *dia = (EDialog *)vdia;
	dia->loadTextDialog(item, toplist, nextinlist, donelist, sortpos);
}

/*
 * Routine to stuff line "line" at the end of the edit buffer.
 */
void DiaStuffLine(void *vdia, INTBIG item, CHAR *line)
{
	EDialog *dia = (EDialog *)vdia;
	dia->stuffLine(item, line);
}

/*
 * Routine to select line "line" of scroll item "item".
 */
void DiaSelectLine(void *vdia, INTBIG item, INTBIG line)
{
	EDialog *dia = (EDialog *)vdia;
	dia->selectLine(item, line);
}

/*
 * Routine to select "count" lines in "lines" of scroll item "item".
 */
void DiaSelectLines(void *vdia, INTBIG item, INTBIG count, INTBIG *lines)
{
	EDialog *dia = (EDialog *)vdia;
	dia->selectLines(item, count, lines);
}

/*
 * Returns the currently selected line in the scroll list "item".
 */
INTBIG DiaGetCurLine(void *vdia, INTBIG item)
{
	EDialog *dia = (EDialog *)vdia;
	return(dia->getCurLine(item));
}

/*
 * Returns the currently selected lines in the scroll list "item".  The returned
 * array is terminated with -1.
 */
INTBIG *DiaGetCurLines(void *vdia, INTBIG item)
{
	EDialog *dia = (EDialog *)vdia;
	return(dia->getCurLines(item));
}

INTBIG DiaGetNumScrollLines(void *vdia, INTBIG item)
{
	EDialog *dia = (EDialog *)vdia;
	return(dia->getNumScrollLines(item));
}

CHAR *DiaGetScrollLine(void *vdia, INTBIG item, INTBIG line)
{
	EDialog *dia = (EDialog *)vdia;
	return(dia->getScrollLine(item, line));
}

void DiaSetScrollLine(void *vdia, INTBIG item, INTBIG line, CHAR *msg)
{
	EDialog *dia = (EDialog *)vdia;
	dia->setScrollLine(item, line, msg);
}

void DiaSynchVScrolls(void *vdia, INTBIG item1, INTBIG item2, INTBIG item3)
{
	EDialog *dia = (EDialog *)vdia;
	dia->synchVScrolls(item1, item2, item3);
}

void DiaUnSynchVScrolls(void *vdia)
{
	EDialog *dia = (EDialog *)vdia;
	dia->unSynchVScrolls();
}

void DiaItemRect(void *vdia, INTBIG item, RECTAREA *rect)
{
	EDialog *dia = (EDialog *)vdia;
	dia->itemRect(item, rect);
}

void DiaPercent(void *vdia, INTBIG item, INTBIG percent)
{
	EDialog *dia = (EDialog *)vdia;
	dia->percent(item, percent);
}

void DiaRedispRoutine(void *vdia, INTBIG item, void (*routine)(RECTAREA*, void*))
{
	EDialog *dia = (EDialog *)vdia;
	dia->redispRoutine(item, routine);
}

void DiaAllowUserDoubleClick(void *vdia)
{
	EDialog *dia = (EDialog *)vdia;
	dia->allowUserDoubleClick();
}

void DiaFillPoly(void *vdia, INTBIG item, INTBIG *xv, INTBIG *yv, INTBIG count, INTBIG r, INTBIG g, INTBIG b)
{
	EDialog *dia = (EDialog *)vdia;
	dia->fillPoly(item, xv, yv, count, r, g, b);
}

void DiaDrawRect(void *vdia, INTBIG item, RECTAREA *rect, INTBIG r, INTBIG g, INTBIG b)
{
	EDialog *dia = (EDialog *)vdia;
	dia->drawRect(item, rect, r, g, b);
}

void DiaFrameRect(void *vdia, INTBIG item, RECTAREA *r)
{
	EDialog *dia = (EDialog *)vdia;
	dia->frameRect(item, r);
}

void DiaInvertRect(void *vdia, INTBIG item, RECTAREA *r)
{
	EDialog *dia= (EDialog *)vdia;
	dia->invertRect(item, r);
}

void DiaDrawLine(void *vdia, INTBIG item, INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty, INTBIG mode)
{
	EDialog *dia = (EDialog *)vdia;
	dia->drawLine(item, fx, fy, tx, ty, mode);
}

void DiaSetTextSize(void *vdia, INTBIG size)
{
	EDialog *dia = (EDialog *)vdia;
	dia->setTextSize(size);
}

void DiaGetTextInfo(void *vdia, CHAR *msg, INTBIG *wid, INTBIG *hei)
{
	EDialog *dia = (EDialog *)vdia;
	dia->getTextInfo(msg, wid, hei);
}

void DiaPutText(void *vdia, INTBIG item, CHAR *msg, INTBIG x, INTBIG y)
{
	EDialog *dia = (EDialog *)vdia;
	dia->putText(item, msg, x, y);
}

void DiaTrackCursor(void *vdia, void (*eachdown)(INTBIG x, INTBIG y))
{
	EDialog *dia = (EDialog *)vdia;
	dia->trackCursor(eachdown);
}

void DiaGetMouse(void *vdia, INTBIG *x, INTBIG *y)
{
	EDialog *dia = (EDialog *)vdia;
	dia->getMouse(x, y);
}

/****************************** EDialog class ******************************/

EDialog::EDialog(DIALOG *dialog )
    : d(0), itemdesc( dialog )
{
	/* be sure the dialog is translated */
	DiaTranslate( dialog );
}

EDialog::~EDialog()
{
	/* remove this from the list of active modal dialogs */
    if (d->isModal()) gra_firstactivemodaldialog = d->nexttdialog;

    /* adjust the dialog position if it was moved */
    QPoint disp = (d->pos() - d->iniPos) * DIALOGDEN / DIALOGNUM;
    itemdesc->windowRect.left += disp.x();
    itemdesc->windowRect.right += disp.x();
    itemdesc->windowRect.top += disp.y();
    itemdesc->windowRect.bottom += disp.y();

	d->dia = 0;
    d->deleteLater();
	gra->toolTimeSlice();
}

void EDialog::reset()
{
}

/*
 * Routine to handle actions and return the next item hit.
 */
INTBIG EDialog::nextHit(void)
{
#ifdef ETRACE
	etrace(GTRACE, "{ DiaNextHit\n");
#endif
	/* flush the screen */
	flushscreen();

	while(d->itemHit == -1)
	{
#if 0
		QTimer timer( d );
		timer.start( MAXEVENTTIME, TRUE );
		qApp->processOneEvent();
#else
		qApp->processEvents(MAXEVENTTIME);
#endif
	}
	INTBIG item = d->itemHit;
	d->itemHit = -1;
#ifdef ETRACE
	etrace(GTRACE, "} DiaNextHit: item=%d\n", item);
#endif
	return(item);
}

void EDialog::itemHitAction(INTBIG itemHit)
{
    if (d->itemHit != -1 && d->itemHit != itemHit) ttyputerr(_("Dialog hit lost"));
    d->itemHit = itemHit;
}

void EDialog::showExtension( BOOLEAN showIt )
{
	if (d->extension())
	{
		d->showExtension( showIt );
		return;
	}
    if (itemdesc->briefHeight == 0) return;
    if (isExtended == showIt) return;
    isExtended = showIt;
    RECTAREA& r = itemdesc->windowRect;
	int wid = r.right - r.left;
	int hei = (isExtended ? r.bottom  - r.top : itemdesc->briefHeight );
    d->setFixedSize( d->scaledialogcoordinate(wid), d->scaledialogcoordinate(hei) );
}

BOOLEAN EDialog::extension()
{
    return isExtended;
}

void EDialog::resizeDialog(INTBIG wid, INTBIG hei)
{
#ifdef ETRACE
    etrace(GTRACE, "{ DiaResizeDialog wid=%d hei=%d\n", wid, hei);
#endif
    d->setFixedSize( d->scaledialogcoordinate(wid), d->scaledialogcoordinate(hei) );
#ifdef ETRACE
    etrace(GTRACE, "} DiaResizeDialog\n");
#endif
}

void EDialog::bringToTop()
{
    d->raise();
}

extern "C" DIALOG us_eprogressdialog, us_progressdialog;

/*
 * Routine to set the text in item "item" to "msg"
 */
void EDialog::setText(INTBIG item, char *msg)
{
	bool highlight = FALSE;
	if (item < 0)
	{
		item = -item;
		highlight = TRUE;
	}
	item--;
	INTBIG type = itemdesc->list[item].type;
	QString qmsg = QString::fromLocal8Bit( msg );
	switch (type&ITEMTYPE) {
	case EDITTEXT: {
	    QLineEdit *lineEdit = (QLineEdit*)d->items[item];
	    QObject::disconnect(lineEdit,SIGNAL(textChanged(const QString&)),d->mapper,SLOT(map()));
	    lineEdit->setText ( qmsg );
	    lineEdit->setCursorPosition( lineEdit->text().length() );
	    if (highlight) lineEdit->selectAll(); else lineEdit->deselect();
	    QObject::connect(lineEdit,SIGNAL(textChanged(const QString&)),d->mapper,SLOT(map()));
	}
	break;
	case MESSAGE: {
	  QLabel *label = (QLabel*)d->items[item];
	  label->setText( qmsg );
	  if (itemdesc == &us_eprogressdialog || itemdesc == &us_progressdialog)
		  {
			  label->repaint();
			  qApp->flush();
		  }
	}
	break;
	case BUTTON:
	case DEFBUTTON:
	case CHECK:
	case RADIO: {
	  QButton *button = (QButton*)d->items[item];
	  button->setText( qmsg );
	}
	break;
	default:
	  qDebug("Bad itemtype in DiaSetText %lx", type);
	}
}

/*
 * Routine to return the text in item "item"
 */
char *EDialog::getText(INTBIG item)
{
	item--;
	INTBIG type = itemdesc->list[item].type;
	switch (type&ITEMTYPE) {
	case EDITTEXT: {
	    QLineEdit *lineEdit = (QLineEdit*)d->items[item];
	    QString qstr = lineEdit->text();
	    return d->localize( qstr );
	}
	break;
	case MESSAGE: {
	    QLabel *label = (QLabel*)d->items[item];
	    return d->localize( label->text() );
	}
	break;
	default:
	    qDebug("Bad itemtype in DiaGetText %lx", type);
	}
	return("");
}

/*
 * Routine to set the value in item "item" to "value"
 */
void EDialog::setControl(INTBIG item, INTBIG value)
{
	item--;
	INTBIG type = itemdesc->list[item].type;
	switch (type&ITEMTYPE) {
        case RADIO:
            {
                QRadioButton *radio = (QRadioButton*)d->items[item];
                radio->setChecked( value );
            }
            break;
        case CHECK:
            if ((type&ITEMTYPEEXT) == AUTOCHECK)
            {
                QCheckBox *check = (ECheckField*)d->items[item];
                check->setChecked( value );
            } else
            {
                ECheckField *check = (ECheckField*)d->items[item];
                check->setChecked( value );
            }
            break;
        default:
            qDebug("Bad itemtype in DiaSetControl %lx", type);
	}
}

/*
 * Routine to return the value in item "item"
 */
INTBIG EDialog::getControl(INTBIG item)
{
	item--;
	INTBIG type = itemdesc->list[item].type;
	switch (type&ITEMTYPE) {
        case RADIO:
            {
                QRadioButton *radio = (QRadioButton*)d->items[item];
                return(radio->isChecked());
            }
            break;
        case CHECK:
            if ((type&ITEMTYPEEXT) == AUTOCHECK)
            {
                QCheckBox *check = (ECheckField*)d->items[item];
                return(check->isChecked());
            } else
            {
                ECheckField *check = (ECheckField*)d->items[item];
                return(check->isChecked());
            }
            break;
        default:
            qDebug("Bad itemtype in DiaGetControl %lx", type);
            return(0);
	}
}

/*
 * Routine to check item "item" to make sure that there is
 * text in it.  If so, it returns true.  Otherwise it beeps and returns false.
 */
BOOLEAN EDialog::validEntry(INTBIG item)
{
	char *msg = getText( item );
	while (*msg == ' ' || *msg == '\t') msg++;
	if (*msg != 0) return(TRUE);
	ttybeep(SOUNDBEEP, TRUE);
	return(FALSE);
}

/*
 * Routine to dim item "item"
 */
void EDialog::dimItem(INTBIG item)
{
	item--;
	QWidget *widget = d->items[item];
	widget->setEnabled( FALSE );
}

/*
 * Routine to un-dim item "item"
 */
void EDialog::unDimItem(INTBIG item)
{
	item--;
	QWidget *widget = d->items[item];
	widget->setEnabled( TRUE );
}

/*
 * Routine to change item "item" to be a message rather
 * than editable text
 */
void EDialog::noEditControl(INTBIG item)
{
	item--;
	INTBIG type = itemdesc->list[item].type;
	switch (type&ITEMTYPE) {
	case EDITTEXT: {
	    QLineEdit *lineEdit = (QLineEdit*)d->items[item];
	    lineEdit->setReadOnly( TRUE );
	}
	break;
	default:
	    qDebug("Bad itemtype in DiaNoEditControl %lx", type);
	}
}

/*
 * Routine to change item "item" to be editable text rather
 * than a message
 */
void EDialog::editControl(INTBIG item)
{
	item--;
	INTBIG type = itemdesc->list[item].type;
	switch (type&ITEMTYPE) {
	case EDITTEXT: {
	    QLineEdit *lineEdit = (QLineEdit*)d->items[item];
	    lineEdit->setReadOnly( FALSE );
	}
	break;
	default:
	    qDebug("Bad itemtype in DiaEditControl %lx", type);
	}
}

void EDialog::opaqueEdit(INTBIG item)
{
	item--;
	INTBIG type = itemdesc->list[item].type;
	switch (type&ITEMTYPE) {
	case EDITTEXT: {
	    QLineEdit *lineEdit = (QLineEdit*)d->items[item];
	    lineEdit->setEchoMode( QLineEdit::Password );
	}
	break;
	default:
	    qDebug("Bad itemtype in DiaOpaqueText %lx", type);
	}
}

/*
 * Routine to cause item "item" to be the default button
 */
void EDialog::defaultButton(INTBIG item)
{
//	qDebug("DiaDefaultButton %d", item);
	item--;
	INTBIG type = itemdesc->list[item].type;
	switch (type&ITEMTYPE) {
	case BUTTON:
	case DEFBUTTON: {
	    QPushButton *button = (QPushButton*)d->items[item];
	    button->setDefault( TRUE );
//	    button->setFocus();
	}
	break;
	default:
	    qDebug("Bad itemtype in DiaDefaultButton %lx", type);
	}
}

void EDialog::changeIcon(INTBIG item, UCHAR1 *addr)
{
	item--;
	INTBIG type = itemdesc->list[item].type;
	switch (type&ITEMTYPE) {
	case ICON: {
	    EIconField *icon = (EIconField*)d->items[item];
	    itemdesc->list[item].data = (INTBIG)addr;
	    icon->setBits( addr );
	}
	break;
	default:
	    qDebug("Bad itemtype in DiaChangeIcon %lx", type);
	}
}

/*
 * Routine to change item "item" into a popup with "count" entries
 * in "names".
 */
void EDialog::setPopup(INTBIG item, INTBIG count, char **names)
{
	item--;
	INTBIG type = itemdesc->list[item].type;
	switch (type&ITEMTYPE) {
	case POPUP: {
	    QComboBox *comboBox = (QComboBox*)d->items[item];
	    comboBox->clear();
	    for (int i = 0; i < count; i++) {
	        comboBox->insertItem( QString::fromLocal8Bit( names[i] ), i );
	    }
	}
	break;
	default:
	    qDebug("Bad itemtype in DiaSetPopup %lx", type);
	}
}

/*
 * Routine to change popup item "item" so that the current entry is "entry".
 */
void EDialog::setPopupEntry(INTBIG item, INTBIG entry)
{
	item--;
	INTBIG type = itemdesc->list[item].type;
	switch (type&ITEMTYPE) {
	case POPUP: {
	    QComboBox *comboBox = (QComboBox*)d->items[item];
	    comboBox->setCurrentItem( entry );
	}
	break;
	default:
	    qDebug("Bad itemtype in DiaSetPopupEntry %lx", type);
	}
}

/*
 * Routine to return the current item in popup menu item "item".
 */
INTBIG EDialog::getPopupEntry(INTBIG item)
{
	item--;
	INTBIG type = itemdesc->list[item].type;
	switch (type&ITEMTYPE) {
	case POPUP: {
	    QComboBox *comboBox = (QComboBox*)d->items[item];
	    return comboBox->currentItem();
	}
	break;
	default:
	    qDebug("Bad itemtype in DiaSetPopup %lx", type);
	}
	return(0);
}

void EDialog::initTextDialog(INTBIG item, BOOLEAN (*toplist)(char **), char *(*nextinlist)(void),
	void (*donelist)(void), INTBIG sortpos, INTBIG flags)
{
	item--;
	EScrollField *scroll = (EScrollField*)d->items[item];
	if ((flags&SCDOUBLEQUIT) != 0)
	{
		INTBIG type0 = itemdesc->list[0].type&ITEMTYPE;
		Q_ASSERT( type0 == DEFBUTTON || type0 == BUTTON );
		QPushButton *button0 = (QPushButton*)d->items[0];
		d->connect(scroll,SIGNAL(doubleClicked(QListBoxItem*)),button0,SLOT(animateClick()));
	}
	if ((flags&SCFIXEDWIDTH) != 0) {
	    QFont fixedfont( QString::null );
	    fixedfont.setStyleHint( QFont::TypeWriter );
	    fixedfont.setFixedPitch( TRUE );
	    scroll->setFont( fixedfont );
	}
	loadTextDialog( item+1, toplist, nextinlist, donelist, sortpos);
}

void EDialog::loadTextDialog(INTBIG item, BOOLEAN (*toplist)(char **), char *(*nextinlist)(void),
	void (*donelist)(void), INTBIG sortpos)
{
	INTBIG i, it;
	char *next, **list, line[256];
	QString qstr;

	item--;
	EScrollField *scroll = (EScrollField*)d->items[item];

	scroll->blockSignals( TRUE );

	/* clear the list */
	scroll->clear();

	if (sortpos < 0)
	{
		/* unsorted: load the list directly */
		line[0] = 0;
		next = line;
		(void)(*toplist)(&next);
		for(it=0; ; it++)
		{
			next = (*nextinlist)();
			if (next == 0) break;
			qstr = QString::fromLocal8Bit( next );
			scroll->insertItem( qstr );
		}
		(*donelist)();
	} else
	{
		/* count the number of items to be put in the text editor */
		line[0] = 0;
		next = line;
		(void)(*toplist)(&next);
		for(it=0; ; it++) if ((*nextinlist)() == 0) break;
		(*donelist)();

		/* allocate space for the strings */
		list = 0;
		if (it > 0)
		{
			list = (char **)emalloc(it * (sizeof (char *)), el_tempcluster);
			if (list == 0) return;
		}

		/* get the list */
		line[0] = 0;
		next = line;
		(void)(*toplist)(&next);
		for(i=0; i<it; i++)
		{
			next = (*nextinlist)();
			if (next == 0) next = "???";
			list[i] = (char *)emalloc(strlen(next)+1, el_tempcluster);
			if (list[i] == 0) return;
			strcpy(list[i], next);
		}
		(*donelist)();

		/* sort the list */
		gra_dialogstringpos = sortpos;
		esort(list, it, sizeof (char *), gra_stringposascending);

		/* stuff the list into the text editor */
		for(i=0; i<it; i++)
		{
			qstr = QString::fromLocal8Bit( list[i] );
			scroll->insertItem( qstr );
		}

		/* deallocate the list */
		if (it > 0)
		{
			for(i=0; i<it; i++) efree((char *)list[i]);
			efree((char *)(char *)list);
		}
	}
	if (it > 0) scroll->setSelected( 0, TRUE );

	scroll->blockSignals( FALSE );
}

/*
 * Routine to stuff line "line" at the end of the edit buffer.
 */
void EDialog::stuffLine(INTBIG item, char *line)
{
	item--;
	EScrollField *scroll = (EScrollField*)d->items[item];
	QString qstr = QString::fromLocal8Bit( line );
	scroll->blockSignals( TRUE );
	scroll->insertItem( qstr );
	scroll->blockSignals( FALSE );
}

/*
 * Routine to select line "line" of scroll item "item".
 */
void EDialog::selectLine(INTBIG item, INTBIG line)
{
	item--;
	EScrollField *scroll = (EScrollField*)d->items[item];
	scroll->blockSignals( TRUE );
	if (line < 0) scroll->clearSelection(); else scroll->setSelected( line, TRUE );
	scroll->blockSignals( FALSE );

	/* make sure lines are visible */
	int first = scroll->topItem();
	int visible = scroll->numItemsVisible();
	if (line < first || line >= first+visible-1) {
		first = QMAX( line - visible/2 , 0);
		scroll->setTopItem( first );
	}
}

/*
 * Routine to select "count" lines in "lines" of scroll item "item".
 */
void EDialog::selectLines(INTBIG item, INTBIG count, INTBIG *lines)
{
	item--;
	EScrollField *scroll = (EScrollField*)d->items[item];
	scroll->blockSignals( TRUE );
	scroll->clearSelection();
	int low=0, high=0;
	for(int i=0; i<count; i++)
	{
		int line = lines[i];
		scroll->setSelected( i, TRUE );
		if (i == 0) low = high = line; else
		{
			if (line < low) low = line;
			if (line > high) high = line;
		}
	}
	scroll->blockSignals( FALSE );

	/* make sure lines are visible */
	int first = scroll->topItem();
	int visible = scroll->numItemsVisible();
	if (high < first || low >= first+visible) scroll->setTopItem( low );
}

/*
 * Returns the currently selected line in the scroll list "item".
 */
INTBIG EDialog::getCurLine(INTBIG item)
{
	item--;
	EScrollField *scroll = (EScrollField*)d->items[item];
	int selected = scroll->currentItem();
	return(selected);
}

/*
 * Returns the currently selected lines in the scroll list "item".  The returned
 * array is terminated with -1.
 */
INTBIG *EDialog::getCurLines(INTBIG item)
{
	static INTBIG selected[MAXSCROLLMULTISELECT];

	item--;
	EScrollField *scroll = (EScrollField*)d->items[item];
	int count = 0;
	for (uint i = 0; i < scroll->count(); i++) {
	  if (scroll->isSelected( i ) && count < MAXSCROLLMULTISELECT - 1) {
	    selected[count] = i;
	    count++;
	  }
	}
	selected[count] = -1;
	return(selected);
}

/*
 * Returns the number of lines in the scroll list "item".
 */
INTBIG EDialog::getNumScrollLines(INTBIG item)
{
	item--;
	EScrollField *scroll = (EScrollField*)d->items[item];
	return(scroll->count());
}

char *EDialog::getScrollLine(INTBIG item, INTBIG line)
{
	item--;
	EScrollField *scroll = (EScrollField*)d->items[item];
	return d->localize( scroll->text( line ) );
}

void EDialog::setScrollLine(INTBIG item, INTBIG line, char *msg)
{
	item--;
	EScrollField *scroll = (EScrollField*)d->items[item];
	QString qmsg = QString::fromLocal8Bit( msg );
	scroll->blockSignals( TRUE );
	scroll->changeItem( qmsg, line );
	scroll->blockSignals( FALSE );
}

void EDialog::synchVScrolls(INTBIG item1, INTBIG item2, INTBIG item3)
{
	if (item1 <= 0 || item1 > itemdesc->items) return;
	if (item2 <= 0 || item2 > itemdesc->items) return;
	if (item3 < 0 || item3 > itemdesc->items) return;
	if (d->numlocks >= MAXLOCKS) return;
	d->lock1[d->numlocks] = ((EScrollField*)d->items[item1 - 1])->verticalScrollBar();
	d->lock2[d->numlocks] = ((EScrollField*)d->items[item2 - 1])->verticalScrollBar();
	d->lock3[d->numlocks] = item3 > 0 ? ((EScrollField*)d->items[item3 - 1])->verticalScrollBar() : 0;

	QObject::connect(d->lock1[d->numlocks],SIGNAL(valueChanged(int)), d->lock2[d->numlocks],SLOT(setValue(int)));
	if(d->lock3[d->numlocks])
	{
		QObject::connect(d->lock2[d->numlocks],SIGNAL(valueChanged(int)), d->lock3[d->numlocks],SLOT(setValue(int)));
		QObject::connect(d->lock3[d->numlocks],SIGNAL(valueChanged(int)), d->lock1[d->numlocks],SLOT(setValue(int)));
	} else
	{
		QObject::connect(d->lock2[d->numlocks],SIGNAL(valueChanged(int)), d->lock1[d->numlocks],SLOT(setValue(int)));
	}
	d->numlocks++;
}

void EDialog::unSynchVScrolls(void)
{
	while (d->numlocks > 0) {
		d->numlocks--;	

		QObject::disconnect(d->lock1[d->numlocks],SIGNAL(valueChanged(int)), d->lock2[d->numlocks],SLOT(setValue(int)));
		if(d->lock3[d->numlocks])
		{
			QObject::disconnect(d->lock2[d->numlocks],SIGNAL(valueChanged(int)), d->lock3[d->numlocks],SLOT(setValue(int)));
			QObject::disconnect(d->lock3[d->numlocks],SIGNAL(valueChanged(int)), d->lock1[d->numlocks],SLOT(setValue(int)));
		} else
		{
			QObject::disconnect(d->lock2[d->numlocks],SIGNAL(valueChanged(int)), d->lock1[d->numlocks],SLOT(setValue(int)));
		}
	}
}

void EDialog::itemRect(INTBIG item, RECTAREA *rect)
{
	item--;
	if (item < 0 || item >= itemdesc->items) return;
	rect->left = itemdesc->list[item].r.left+1;
	rect->right = itemdesc->list[item].r.right-1;
	rect->top = itemdesc->list[item].r.top+1;
	rect->bottom = itemdesc->list[item].r.bottom-1;
}

void EDialog::percent(INTBIG item, INTBIG percent)
{
	item--;
	QProgressBar *progress = (QProgressBar*)d->items[item];
	progress->setProgress( percent );
	qApp->flush();
}

void EDialog::redispRoutine(INTBIG item, void (*routine)(RECTAREA *rect, void *dia))
{
    item--;
    Q_ASSERT( (itemdesc->list[item].type&ITEMTYPE) == USERDRAWN );
    EUserDrawnField *user = (EUserDrawnField*)d->items[item];
    user->redispRoutine = routine;
	user->dia = this;
}

void EDialog::allowUserDoubleClick(void)
{
	INTBIG type0 = itemdesc->list[0].type&ITEMTYPE;
	Q_ASSERT( type0 == DEFBUTTON || type0 == BUTTON );
	QPushButton *button0 = (QPushButton*)d->items[0];

	for( int i=0; i<itemdesc->items; i++)
	{
		if((itemdesc->list[i].type&ITEMTYPE) == USERDRAWN)
		{
			EUserDrawnField *user = (EUserDrawnField*)d->items[i];
			user->doubleClickAllowed = TRUE;
			QObject::connect(user,SIGNAL(doubleClicked()),button0,SLOT(animateClick()));
		}
	}
}

void EDialog::fillPoly(INTBIG item, INTBIG *xv, INTBIG *yv, INTBIG count, INTBIG r, INTBIG g, INTBIG b)
{
    item--;
    Q_ASSERT( (itemdesc->list[item].type&ITEMTYPE) == USERDRAWN );
    int xoff = itemdesc->list[item].r.left;
    int yoff = itemdesc->list[item].r.top;
    QPainter p( d->items[item] );
    QPointArray pointlist( count );
    for( int i=0; i<count; i++ ) {
        pointlist.setPoint( i, d->scaledialogcoordinate(xv[i]-xoff) ,
			       d->scaledialogcoordinate(yv[i]-yoff) );
    }
    p.setPen( Qt::NoPen );
    p.setBrush( QColor( r, g, b ) );
    p.drawPolygon( pointlist );
}

void EDialog::drawRect(INTBIG item, RECTAREA *rect, INTBIG r, INTBIG g, INTBIG b)
{
    item--;
    Q_ASSERT( (itemdesc->list[item].type&ITEMTYPE) == USERDRAWN );
    int xoff = itemdesc->list[item].r.left;
    int yoff = itemdesc->list[item].r.top;
    QPainter p( d->items[item] );
    int x1 = d->scaledialogcoordinate(rect->left-xoff);
    int y1 = d->scaledialogcoordinate(rect->top-yoff);
    int x2 = d->scaledialogcoordinate(rect->right-xoff);
    int y2 = d->scaledialogcoordinate(rect->bottom-yoff);
    p.setPen( Qt::NoPen );
    p.setBrush( QColor( r, g, b ) );
    p.drawRect( x1, y1, x2 - x1, y2 - y1 );
}

void EDialog::frameRect(INTBIG item, RECTAREA *r)
{
    item--;
    Q_ASSERT( (itemdesc->list[item].type&ITEMTYPE) == USERDRAWN );
    int xoff = itemdesc->list[item].r.left;
    int yoff = itemdesc->list[item].r.top;
    QPainter p( d->items[item] );
    int x1 = d->scaledialogcoordinate(r->left-xoff);
    int y1 = d->scaledialogcoordinate(r->top-yoff);
    int x2 = d->scaledialogcoordinate(r->right-xoff);
    int y2 = d->scaledialogcoordinate(r->bottom-yoff);
    p.setPen( Qt::black );
    p.setBrush( Qt::white );
    p.drawRect( x1, y1, x2 - x1, y2 - y1 );
}

void EDialog::invertRect(INTBIG item, RECTAREA *r)
{
    item--;
    Q_ASSERT( (itemdesc->list[item].type&ITEMTYPE) == USERDRAWN );
    int xoff = itemdesc->list[item].r.left;
    int yoff = itemdesc->list[item].r.top;
    QPainter p( d->items[item] );
    int x1 = d->scaledialogcoordinate(r->left-xoff);
    int y1 = d->scaledialogcoordinate(r->top-yoff);
    int x2 = d->scaledialogcoordinate(r->right-xoff);
    int y2 = d->scaledialogcoordinate(r->bottom-yoff);
    p.setRasterOp( Qt::NotROP );
    p.drawRect( x1, y1, x2 - x1, y2 - y1 );
}

void EDialog::drawLine(INTBIG item, INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty, INTBIG mode)
{
    item--;
    Q_ASSERT( (itemdesc->list[item].type&ITEMTYPE) == USERDRAWN );
    int xoff = itemdesc->list[item].r.left;
    int yoff = itemdesc->list[item].r.top;
    QPainter p( d->items[item] );
    int x1 = d->scaledialogcoordinate(fx-xoff);
    int y1 = d->scaledialogcoordinate(fy-yoff);
    int x2 = d->scaledialogcoordinate(tx-xoff);
    int y2 = d->scaledialogcoordinate(ty-yoff);
    switch( mode ) {
    case DLMODEON:
        p.setPen( Qt::black );
	break;
    case DLMODEOFF:
        p.setPen( Qt::white );
	break;
    case DLMODEINVERT:
        p.setRasterOp( Qt::NotROP );
	break;
    }
    p.drawLine( x1, y1, x2, y2 );
}

void EDialog::setTextSize(INTBIG size)
{
    d->userFont.setPixelSize( size );
}

void EDialog::getTextInfo(char *msg, INTBIG *wid, INTBIG *hei)
{
    QPainter p( d );
    p.setFont( d->userFont );
    QSize size = p.fontMetrics().boundingRect( QString::fromLocal8Bit( msg ) ).size();
    size = size * DIALOGDEN / DIALOGNUM;
    *wid = size.width();
    *hei = size.height();
}

void EDialog::putText(INTBIG item, char *msg, INTBIG x, INTBIG y)
{
    item--;
    Q_ASSERT( (itemdesc->list[item].type&ITEMTYPE) == USERDRAWN );
    int xoff = itemdesc->list[item].r.left;
    int yoff = itemdesc->list[item].r.top;
    QPainter p( d->items[item] );
    int x1 = d->scaledialogcoordinate(x-xoff);
    int y1 = d->scaledialogcoordinate(y-yoff);
    p.setPen( Qt::black );
    p.setFont( d->userFont );
    p.drawText( x1, y1 + p.fontMetrics().ascent(), QString::fromLocal8Bit( msg ) );
}

void EDialog::trackCursor(void (*eachdown)(INTBIG x, INTBIG y))
{
    /* find user defined field which grab mouse */
    EUserDrawnField *user = 0;
    for( int i=0; i<itemdesc->items && user == 0; i++ ) {
        if ((itemdesc->list[i].type&ITEMTYPE) != USERDRAWN) continue;
		user = (EUserDrawnField*)d->items[i];
		if (!user->buttonPressed) user = 0;
    }
    if (user) {
        user->eachdown = eachdown;
        qApp->enter_loop();
        Q_ASSERT( user->eachdown == 0 );
    }
}

void EDialog::getMouse(INTBIG *x, INTBIG *y)
{
    QPoint pos = d->mapFromGlobal( QCursor::pos() ) * DIALOGDEN / DIALOGNUM;
    *x = pos.x();
    *y = pos.y();
}

QStrList *EDialog::itemNamesFromUi( char *uiFile, BOOLEAN ext )
{
#ifdef QTSTRETCH
    EDialogPrivate *d = EDialogPrivate::loadUi( 0, uiFile, ext );
    if (d == 0) return 0;
	/* load the items */
    QStrList *slist = new QStrList( TRUE );
    slist->setAutoDelete( TRUE );
    QObjectList ol ( *d->children() );
    QCString name;
	for(uint li = 0; li < ol.count(); li++)
	{
        if (!ol.at(li)->isWidgetType()) continue;
        QWidget *widget = (QWidget*)ol.at(li);
        slist->append( widget->name() );
	}
    return slist;
#else
	Q_UNUSED( uiFile );
	Q_UNUSED( ext );
    return 0;
#endif
}

EDialogModal::EDialogModal( DIALOG *dialog )
	: EDialog( dialog ), in_loop(FALSE), result(Rejected)
{
	QWidget *base=0;
	
	/* make the dialog structure */
	if (gra_firstactivemodaldialog != 0) base = gra_firstactivemodaldialog;
#ifndef MACOSX
	else if (el_curwindowpart == NOWINDOWPART) base = qApp->mainWidget(); else
		base = ((QWidget*)el_curwindowpart->frame->draw)->parentWidget();
#endif

    d = new EDialogPrivate( base, itemDesc()->movable );
    d->init( this, TRUE );

	/* add this to the list of active dialogs */
    d->nexttdialog = gra_firstactivemodaldialog;
    gra_firstactivemodaldialog = d;
	d->showMe();
}

EDialogModal::DialogCode EDialogModal::exec()
{
    ttyputerr(_("exec() is not implemented"));
    return Rejected;
}

#ifdef QTSTRETCH
EWidgetFactory::EWidgetFactory()
    : QWidgetFactory()
{
}

QWidget* EWidgetFactory::createWidget( const QString& className, QWidget *parent, const char *name) const
{
    if (className == "EScrollField") return new EScrollField( parent );
    if (className == "EDialogPrivate") return new EDialogPrivate( parent, name );
    QCString s = className.latin1();
    printf("createWidget %s\n", (const char*)s);
    return 0;
}

EWidgetFactory *widgetFactory = 0;

EDialogPrivate *EDialogPrivate::loadUi( QWidget *parent, char *uiFile, bool ext )
{
    if (widgetFactory == 0)
    {
        widgetFactory = new EWidgetFactory();
        QWidgetFactory::addWidgetFactory( widgetFactory );
    }
        
    void *infstr = initinfstr();
    formatinfstr(infstr, "%sui/%s%s.ui", el_libdir, uiFile, (ext ? "_ext" : "") );
    return (EDialogPrivate*)QWidgetFactory::create(returninfstr(infstr), 0, parent );
}
#endif

EDialogModeless::EDialogModeless( DIALOG *dialog )
    : EDialog( dialog )
{
	/* make the dialog structure */
#  ifdef QTSTRETCH
    if (dialog->uiFile != 0)
    {
        d = EDialogPrivate::loadUi( qApp->mainWidget(), dialog->uiFile );
        if (d != 0)
        {
            d->dia = this;
			if (dialog->briefHeight != 0)
			{
				EDialogPrivate *ext = EDialogPrivate::loadUi( d, dialog->uiFile, TRUE );
				if (ext != 0)
				{
					d->setExtension( ext );
					d->setOrientation( Qt::Vertical );
				} else
				{
					delete d;
					d = 0;
				}
			}
        }
		if (d != 0)
		{
            if (d->checkChildren()) return;
            delete d;
		}
    }
#  endif
    d = new EDialogPrivate( qApp->mainWidget(), itemDesc()->movable );
    d->init( this, FALSE );

	/* add this to the list of modeless dialogs */
    d->nexttdialog = gra_firstmodelessdialog;
    gra_firstmodelessdialog = d;

}

void DiaCloseAllModeless()
{
    EDialogPrivate *p;

    for (p = gra_firstmodelessdialog; p != 0; p = p->nexttdialog)
    {
        EDialogModeless *dia = (EDialogModeless*)p->dia;
        dia->hide();
        dia->reset();
    }
}

void EDialogModeless::show()
{
    d->show();
    d->raise();
}

void EDialogModeless::hide()
{
    d->hide();
	gra->toolTimeSlice();
}

bool EDialogModeless::isHidden()
{
    return d->isHidden();
}

EDialogPrivate::EDialogPrivate(QWidget *parent, const char *name )
    : QDialog( parent, name ),
      numlocks( 0 ), mapper( 0 ), dia( 0 ), nexttdialog( 0 ), itemHit( -1 )
{
}

EDialogPrivate::~EDialogPrivate()
{
#ifdef DOUBLESELECT
	gra->activationChange();
#endif
}

void EDialogPrivate::init( EDialog *dia, bool modal )
{
	INTBIG i, j, itemtype, x, y, wid, hei;
    DIALOG *dialog = dia->itemDesc();

    EDialogPrivate::dia = dia;
    if (modal) setWFlags(WShowModal);

	getdialogcoordinates(&dialog->windowRect, &x, &y, &wid, &hei);
    if (dialog->briefHeight != 0)
        hei = scaledialogcoordinate( dialog->briefHeight );
    setFixedSize( wid, hei );
    move( x, y );
	setCaption( QString::fromLocal8Bit( dialog->movable ) );

	/* load the items */
	mapper = new QSignalMapper( this, "mapper");
	connect(mapper,SIGNAL(mapped(int)),this,SLOT(action(int)));
	QWidget *focus = 0;
	QPushButton *defbutton = 0;
	QPushButton *buttonzero = 0;
	for(i=0; i<dialog->items; i++)
	{
		getdialogcoordinates(&dialog->list[i].r, &x, &y, &wid, &hei);
		itemtype = dialog->list[i].type;
		QString unimsg = QString::fromLocal8Bit( dialog->list[i].msg );
		QWidget *widget = 0;
		QPushButton *button;
		ECheckField *check;
        QCheckBox *autocheck;
		QRadioButton *radio;
		QProgressBar *progress;
		QLineEdit *lineEdit;
		QLabel *label;
		QComboBox *comboBox;
		EUserDrawnField *user;

		switch (itemtype&ITEMTYPE)
		{
			case BUTTON:
			case DEFBUTTON:
                widget = button = new QPushButton(unimsg, this);
				connect(button,SIGNAL(clicked()),mapper,SLOT(map()));
				if (i == 0) buttonzero = button;
				if ((itemtype&ITEMTYPE) == DEFBUTTON) defbutton = button;
				break;
			case CHECK:
                if ((itemtype&ITEMTYPEEXT) == AUTOCHECK)
                {
                    widget = autocheck = new QCheckBox(unimsg, this );
                    connect(autocheck,SIGNAL(clicked()),mapper,SLOT(map()));
                } else
                {
                    widget = check = new ECheckField(unimsg, this );
                    connect(check,SIGNAL(clicked()),mapper,SLOT(map()));
                }
				break;
			case RADIO:
				widget = radio = new QRadioButton(unimsg, this );
				connect(radio,SIGNAL(clicked()),mapper,SLOT(map()));
                if ((itemtype&ITEMTYPEEXT) != RADIO)
                {
                    /* Radio-button group */
                    int exttype = itemtype&ITEMTYPEEXT;
                    /* check if this is the last button in the group */
                    for (j = i + 1; j < dialog->items; j++)
                        if ((dialog->list[j].type&ITEMTYPEEXT) == exttype) break;
                    if (j == dialog->items)
                    {
                        /* last button, hence make the group */
                        QButtonGroup *group = new QButtonGroup(this);
                        for (j = 0; j < i; j++)
                        {
                            if ((dialog->list[j].type&ITEMTYPEEXT) == exttype)
                                group->insert( (QRadioButton*)( items[j] ) );
                        }
                        group->insert( radio );
                        group->hide();
                    }
                }
				break;
			case EDITTEXT:
				widget = lineEdit = new QLineEdit( this );
#if 1
				widget->installEventFilter( this );
#endif
				connect(lineEdit,SIGNAL(textChanged(const QString&)),mapper,SLOT(map()));
				if (focus == 0) focus = lineEdit;
				break;
			case MESSAGE:
				widget = label = new QLabel( unimsg, this );
                label->setAlignment( AlignAuto | AlignTop | ExpandTabs | WordBreak | BreakAnywhere);
				break;
			case PROGRESS:
				widget = progress = new QProgressBar( 100, this );
				break;
			case DIVIDELINE:
				widget = new QWidget( this );
				widget->setBackgroundMode( PaletteForeground );
				break;
			case USERDRAWN:
				widget = user = new EUserDrawnField ( this, &dialog->list[i] );
				user->dia = dia;
				connect(user,SIGNAL(clicked()),mapper,SLOT(map()));
				break;
			case POPUP:
				widget = comboBox = new QComboBox( this );
				connect(comboBox,SIGNAL(activated(int)),mapper,SLOT(map()));
				break;
			case ICON:
				{
					dialog->list[i].data = (INTBIG)dialog->list[i].msg;
					EIconField *icon = new EIconField( this, &dialog->list[i] );
					widget = icon;
					if ((itemtype&INACTIVE) == 0)
					{
						connect(icon, SIGNAL(clicked()), mapper, SLOT(map()));
					}
				}
				break;
			case SCROLL:
			case SCROLLMULTI:
				{
			        EScrollField *scroll;
					widget = scroll = new EScrollField( this );
					connect(scroll,SIGNAL(selectionChanged()),mapper,SLOT(map()));
					if ((itemtype&ITEMTYPE) == SCROLLMULTI)
						scroll->setSelectionMode( EScrollField::Extended );
				}
				break;
			default:
				break;
		}
		items[i] = widget;
		if (widget != 0) {
            widget->setFixedSize( wid, hei );
            widget->move( x, y );
			mapper->setMapping( widget, i );
		}
	}
	if (defbutton == 0 && buttonzero != 0) defbutton = buttonzero;
	if (defbutton != 0) defbutton->setDefault(TRUE);
	if (focus == 0) focus = defbutton;
	if (focus == 0) focus = items[0];
	if (focus != 0) focus->setFocus();
}

void EDialogPrivate::showMe()
{
    QWidget::show();
    iniPos = pos();
    itemHit = -1;
}

char *EDialogPrivate::localize (QString qstr)
{
    static QCString str[2];
	static int i = 0;

    str[i] = qstr.local8Bit();
    const char *s = str[i];
	i = (i + 1) % (sizeof(str)/sizeof(str[0]));
    return((char*)s);
}

#ifdef QTSTRETCH
#define REORDER
bool EDialogPrivate::checkChildren()
{
	INTBIG i, j, itemtype;
    DIALOG *dialog = dia->itemDesc();
#ifdef REORDER
	char line[40];
#endif

	/* load the items */
	mapper = new QSignalMapper( this, "mapper");
	connect(mapper,SIGNAL(mapped(int)),this,SLOT(action(int)));
	QWidget *focus = 0;
	QPushButton *defbutton = 0;
	QPushButton *buttonzero = 0;
#ifdef REORDER
	for (i = 0; i < dialog->items; i++) items[i] = 0;
#else
    i = 0;
#endif

	for (uint isExt = FALSE; isExt <= (dialog->briefHeight != 0); isExt++)
	{
		QObjectList ol ( *(isExt ? extension()->children() : children() ) );
		for(uint li = 0; li < ol.count(); li++)
		{
			if (!ol.at(li)->isWidgetType() || ol.at(li) == extension()) continue;
			QWidget *widget = (QWidget*)ol.at(li);
#ifdef REORDER
			strncpy(line, widget->name(), sizeof(line));
			for (i = 0; i < (int)sizeof(line); i++)
				if (line[i] == '_' || i == sizeof(line)-1) line[i] = 0;
			i = atol(line)-1;
			if (i < 0 || i >= dialog->items)
			{
				ttyputerr("Invalid dialog item number %s", widget->name());
				return FALSE;
			}
			if (items[i])
			{
				ttyputerr("Duplicated dialog item number %s", widget->name());
				return FALSE;
			}
#else
			if (i >= dialog->items)
			{
				ttyputerr("Too many items in .ui");
				return FALSE;
			}
#endif

			itemtype = dialog->list[i].type;
			switch (itemtype&ITEMTYPE)
			{
				case BUTTON:
				case DEFBUTTON:
					if (!widget->isA("QPushButton"))
					{
						ttyputerr("Invalid item %d BUTTON in .ui", i);
						return FALSE;
					}
					{
						QPushButton *button = (QPushButton*)widget;
						connect(button,SIGNAL(clicked()),mapper,SLOT(map()));
						if (i == 0) buttonzero = button;
						if ((itemtype&ITEMTYPE) == DEFBUTTON) defbutton = button;
					}
					break;
				case CHECK:
					if ((itemtype&ITEMTYPEEXT) == AUTOCHECK)
					{
						if (!widget->isA("QCheckBox"))
						{
							ttyputerr("Invalid item %d AUTOCHECK in .ui", i);
							return FALSE;
						}
						connect(widget,SIGNAL(clicked()),mapper,SLOT(map()));
					} else
					{
						if (!widget->isA("ECheckField"))
						{
							ttyputerr("Invalid item %d CHECK in .ui", i);
							return FALSE;
						}
						connect(widget,SIGNAL(clicked()),mapper,SLOT(map()));
					}
					break;
				case RADIO:
					if (!widget->isA("QRadioButton"))
					{
						ttyputerr("Invalid item %d RADIO in .ui", i);
						return FALSE;
					}
					connect(widget,SIGNAL(clicked()),mapper,SLOT(map()));
					if ((itemtype&ITEMTYPEEXT) != RADIO)
					{
						/* Radio-button group */
						int exttype = itemtype&ITEMTYPEEXT;
						/* check if this is the last button in the group */
						for (j = i + 1; j < dialog->items; j++)
							if ((dialog->list[j].type&ITEMTYPEEXT) == exttype) break;
						if (j == dialog->items)
						{
							/* last button, hence make the group */
							QButtonGroup *group = new QButtonGroup(this);
							for (j = 0; j < i; j++)
							{
								if ((dialog->list[j].type&ITEMTYPEEXT) == exttype)
									group->insert( (QRadioButton*)( items[j] ) );
							}
							group->insert( (QRadioButton*)widget );
							group->hide();
						}
					}
					break;
				case EDITTEXT:
					if (!widget->isA("QLineEdit"))
					{
						ttyputerr("Invalid item %d EDITTEXT in .ui", i);
						return FALSE;
					}
					connect(widget,SIGNAL(textChanged(const QString&)),mapper,SLOT(map()));
					if (focus == 0) focus = widget;
					break;
				case MESSAGE:
					if (!widget->isA("QLabel"))
					{
						ttyputerr("Invalid item %d MESSAGE in .ui", i);
						return FALSE;
					}
					break;
				case PROGRESS:
					if (!widget->isA("QProgressBar"))
					{
						ttyputerr("Invalid item %d PROGRESS in .ui", i);
						return FALSE;
					}
					break;
				case DIVIDELINE:
					if (!widget->isA("QWidget"))
					{
						ttyputerr("Invalid item %d DIVIDELINE in .ui", i);
						return FALSE;
					}
					widget->setBackgroundMode( PaletteForeground );
					break;
				case USERDRAWN:
					if (!widget->isA("EUserDrawnField"))
					{
						ttyputerr("Invalid item %d USERDRAWN in .ui", i);
						return FALSE;
					}
					((EUserDrawnField*)widget)->dia = dia;
					connect(widget,SIGNAL(clicked()),mapper,SLOT(map()));
					break;
				case POPUP:
					if (!widget->isA("QComboBox"))
					{
						ttyputerr("Invalid item %d POPUP in .ui", i);
						return FALSE;
					}
					connect(widget,SIGNAL(activated(int)),mapper,SLOT(map()));
					break;
				case ICON:
					{
						if (!widget->isA("EIconField"))
						{
							ttyputerr("Invalid item %d ICON in .ui", i);
							return FALSE;
						}
						dialog->list[i].data = (INTBIG)dialog->list[i].msg;
						if ((itemtype&INACTIVE) == 0)
						{
							connect(widget, SIGNAL(clicked()), mapper, SLOT(map()));
						}
					}
					break;
				case SCROLL:
				case SCROLLMULTI:
					if (!widget->isA("EScrollField"))
					{
						ttyputerr("Invalid item %d SCROLL in .ui", i);
						return FALSE;
					}
					connect(widget,SIGNAL(selectionChanged()),mapper,SLOT(map()));
					if ((itemtype&ITEMTYPE) == SCROLLMULTI)
						((EScrollField*)widget)->setSelectionMode( EScrollField::Extended );
					break;
				default:
					break;
			}
#ifdef REORDER
			items[i] = widget;
			mapper->setMapping( widget, i );
#else
			items[i] = widget;
			mapper->setMapping( widget, i );
			i++;
#endif
		}
	}
#ifdef REORDER
	for (i = 0; i < dialog->items; i++)
	{
		if (!items[i])
		{
			ttyputerr("Dialog item %d is absent", i+1);
			return FALSE;
		}
	}
#else
    if (i != dialog->items)
    {
        ttyputerr("Too few items in .ui");
        return FALSE;
    }
#endif
	if (defbutton == 0 && buttonzero != 0) defbutton = buttonzero;
	if (defbutton != 0) defbutton->setDefault(TRUE);
	if (focus == 0) focus = defbutton;
	if (focus == 0) focus = items[0];
	if (focus != 0) focus->setFocus();
    return TRUE;
}
#endif

/*
 * Routine to convert dialog coordinates
 */
void EDialogPrivate::getdialogcoordinates(RECTAREA *rect, INTBIG *x, INTBIG *y, INTBIG *wid, INTBIG *hei)
{
	*x = scaledialogcoordinate(rect->left);
	*y = scaledialogcoordinate(rect->top);
	*wid = scaledialogcoordinate(rect->right - rect->left);
	*hei = scaledialogcoordinate(rect->bottom - rect->top);
}

INTBIG EDialogPrivate::scaledialogcoordinate(INTBIG x)
{
	return((x * DIALOGNUM + DIALOGDEN/2) / DIALOGDEN);
}

void EDialogPrivate::action(int id)
{
    INTBIG item;

    item = ((int)id) & 0xFFFF;
	if ( dia )
		dia->itemHitAction( item+1 );
}

void EDialogPrivate::closeEvent( QCloseEvent *e )
{
    e->ignore();
}

void EDialogPrivate::keyPressEvent( QKeyEvent *e )
{
    /* handle escape key (fakes a click on item 2 */
    if( e->key() == Key_Escape && isModal() && dia )
	{
		dia->itemHitAction(2);
		return;
	}
    QDialog::keyPressEvent( e );
}

bool EDialogPrivate::eventFilter( QObject *watched, QEvent *e )
{
#ifdef ETRACE
    if (e->type () == QEvent::KeyPress) {
		QKeyEvent* ke = (QKeyEvent*) e;
		etrace(GTRACE, "  EDialogPrivate::eventFilter %s KeyPress <%c>\n", watched->className(), ke->ascii());
	} else if (e->type() != QEvent::Paint && e->type() != QEvent::MouseMove)
	{
		/* Event type codes are in "/qt/include/qevent.h" */
		etrace(GTRACE, "  EDialogPrivate::eventFilter %s type=%d\n", watched->className(), e->type());
	}
#endif
    return QWidget::eventFilter( watched, e );
}

/******************************** Check Field ********************************/

ECheckField::ECheckField( QString &text, QWidget *parent )
    : QCheckBox( text, parent )
{
    checked = FALSE;
}

ECheckField::~ECheckField()
{
    if (isChecked() != QCheckBox::isChecked())
        ttyputerr(_("CheckBox field appearance differs from value"));
}

bool ECheckField::isChecked()
{
    return checked;
}

void ECheckField::setChecked( bool value )
{
    checked = value;
    QCheckBox::setChecked( value );
}

/******************************** Scroll Field ********************************/

EScrollField::EScrollField( QWidget *parent )
    : QListBox( parent )
{
}

void EScrollField::keyPressEvent( QKeyEvent *e )
{
	if ( e->key() == Key_Enter || e->key() == Key_Return)
	{
		e->ignore();
		return;
	}
    if ( e->key() == Key_C && (e->state() & ControlButton) ||
	 e->key() == Key_F16) { // Copy key on Sun keyboards
        qDebug( "Copy list");
        QStringList strList;
        for( uint i=0; i < count(); i++) strList.append( text( i ) );
	qApp->clipboard()->setText( strList.join( "\n" ) );
	qDebug( qApp->clipboard()->text() );
        return;
    }
    QListBox::keyPressEvent( e );
}

/****************************** Icon Field ******************************/

EIconField::EIconField( QWidget *parent, DIALOGITEM *item )
    : QLabel( parent )
{
    setBits( (uchar*)item->data );
}

void EIconField::setBits( uchar *bits )
{
    QBitmap bitmap(32, 32, bits, FALSE);
    QWMatrix m;
    double scale = (double)DIALOGNUM / double(DIALOGDEN);
    m.scale( scale, scale );
    QPixmap pixmap = bitmap.xForm( m );
    setPixmap( pixmap );
}

void EIconField::mousePressEvent( QMouseEvent *e )
{
	Q_UNUSED( e );
    emit clicked();
};

/****************************** User Drawn Field ******************************/

EUserDrawnField::EUserDrawnField(QWidget *parent, DIALOGITEM *item)
  : QWidget(parent), it(item), redispRoutine(0), buttonPressed(FALSE), eachdown(0), doubleClickAllowed(FALSE)
{
}

void EUserDrawnField::mouseMoveEvent( QMouseEvent *e )
{
    if (eachdown == 0) return;
    QPoint pos = mapToParent( e->pos() ) * DIALOGDEN / DIALOGNUM;
    (*eachdown)( pos.x(), pos.y() );
}

void EUserDrawnField::mouseDoubleClickEvent( QMouseEvent *e )
{
	Q_UNUSED( e );
    buttonPressed = TRUE;
    if (doubleClickAllowed)
      emit doubleClicked();
    else
      emit clicked();
}

void EUserDrawnField::mousePressEvent( QMouseEvent *e )
{
	Q_UNUSED( e );
    buttonPressed = TRUE;
    emit clicked();
}

void EUserDrawnField::mouseReleaseEvent( QMouseEvent *e )
{
	Q_UNUSED( e );
    buttonPressed = FALSE;
    if (eachdown != 0) {
        eachdown = 0;
        qApp->exit_loop();
    }
}

void EUserDrawnField::paintEvent( QPaintEvent *e )
{
	Q_UNUSED( e );
    RECTAREA rect;
    rect.left = it->r.left+1;
    rect.right = it->r.right-1;
    rect.top = it->r.top+1;
    rect.bottom = it->r.bottom-1;
    if ( redispRoutine )
        (*redispRoutine)(&rect, dia);
}



/****************************** DIALOG SUPPORT ******************************/

/*
 * Helper routine for "DiaLoadTextDialog" that makes strings go in ascending order.
 */
int gra_stringposascending(const void *e1, const void *e2)
{
	REGISTER char *c1, *c2;

	c1 = *((char **)e1);
	c2 = *((char **)e2);
	return(namesame(&c1[gra_dialogstringpos], &c2[gra_dialogstringpos]));
}

