/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: edialogs.h
 * Header file for dialog handling
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

/*
 *    DIALOG CONTROL
 * d = DiaInitDialog(dia)                  initializes dialog from table "dia"
 * d = DiaInitDialogModeless(dia, hitroutine) initializes modeless dialog, call "hitroutine" for hits
 * item = DiaNextHit(d)                    obtains next "item" hit in dialog processing
 * DiaDoneDialog(d)                        removes and terminates dialog
 * DiaResizeDialog(d, wid, hei)            changes dialog size
 * DiaBringToTop(d)                        brings the dialog to the top if it is obscured
 *
 *    CONTROL OF STANDARD DIALOG ITEMS
 * DiaSetText(d, item, msg)                puts "msg" in "item" in the dialog
 * line = DiaGetText(d, item)              gets text in "item" in the dialog
 * DiaSetControl(d, item, value)           puts "value" in "item" in the dialog
 * value = DiaGetControl(d, item)          gets integer in "item" in the dialog
 * valid = DiaValidEntry(d, item)          returns true if "item" is nonblank in the dialog
 *
 *    SPECIAL DIALOG CONTROL
 * DiaDimItem(d, item)                     makes "item" dimmed in the dialog
 * DiaUnDimItem(d, item)                   makes "item" undimmed in the dialog
 * DiaNoEditControl(d, item)               makes "item" not editable in the dialog
 * DiaEditControl(d, item)                 makes "item" editable in the dialog
 * DiaOpaqueEdit(d, item)                  makes "item" not display text
 * DiaDefaultButton(d, item)               makes "item" the default button
 * DiaChangeIcon(d, item, addr)            sets the icon in "item" to "addr"
 *
 *    CONTROL OF POPUP ITEMS
 * DiaSetPopup(d, item, count, names)      makes "item" a popup with "count" entries in "names"
 * DiaSetPopupEntry(d, item, entry)        makes popup "item" use entry "entry" as current
 * entry = DiaGetPopupEntry(d, item)       gets current entry in popup "item"
 *
 *    CONTROL OF SCROLLABLE ITEMS
 * DiaInitTextDialog(d, item, top, next, done, sortpos, flags)
 * DiaLoadTextDialog(d, item, top, next, done, sortpos)
 * DiaStuffLine(d, item, msg)              add line "msg" to item "item"
 * DiaSelectLine(d, item, l)               select line "l" in item "item"
 * DiaSelectLines(d, item, c, l)           select "c" lines in "l" in item "item"
 * which = DiaGetCurLine(d, item)          get current line number in scroll item "item"
 * *which = DiaGetCurLines(d, item)        get array of selected lines in scroll item "item"
 * DiaGetNumScrollLines(d, item)           returns number of lines in scroll item "item"
 * msg = DiaGetScrollLine(d, item, l)      get line "l" in scroll item "item"
 * DiaSetScrollLine(d, item, l, msg)       set line "l" in scroll item "item"
 * DiaSynchVScrolls(d, item1, item2, item3) synchronizes vertical scroll items "item1/2/3"
 * DiaUnSynchVScrolls(d)                   removes all vertical scroll synchronization
 *
 *    USER-CONTROL OF DIALOG ITEMS
 * DiaItemRect(d, item, r)                 get rectangle for item "item"
 * DiaPercent(d, item, p)                  fill item "item" with "p" percent progress
 * DiaRedispRoutine(d, item, routine)      call "routine" to redisplay item "item"
 * DiaAllowUserDoubleClick(d)              double-click in user items reports "OK" item
 * DiaFillPoly(d, item, x, y, c, r, g, b)  draw "c" points in (x,y) with color (r,g,b)
 * DiaDrawRect(d, item, rect, r, g, b)     fill rectangle "rect" with "(r,g,b)"
 * DiaFrameRect(d, item, r)                draw frame in rectangle "r"
 * DiaInvertRect(d, item, r)               invert color in rectangle "r"
 * DiaDrawLine(d, item, fx, fy, tx, ty, m) draw line from (fx,fy) to (tx,ty) mode "m"
 * DiaSetTextSize(d, size)                 set size of text to "size"
 * DiaGetTextInfo(d, msg, wid, hei)        get "wid" and "hei" of "msg"
 * DiaPutText(d, item, msg, x, y)          put text "msg" at (x,y)
 * DiaTrackCursor(d, eachdown)             track cursor calling "eachdown" on mouse move
 * DiaGetMouse(d, x, y)                    get current mouse position into "x/y"
 */

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
extern "C"
{
#endif

/** @defgroup Dialogs Dialogs
 * Dialogs API permits to describe structure and behaviour of a dialog in a platform-independent way.
 * The structure is described by DIALOG type. Appearance of the dialog on Qt can be additionally specified in
 * ".ui" form.
 *
 * The behaviour of dialog can be described either in C++ or in C style.
 * In C++ style, the behaviour of modal dialog is specified by EDialogModal,
 * behaviour of modeless dialogs - by EDialogModeless.
 * These classes inherit most member functions from EDialog.
 * The C styles routines match member functions of EDialog.
 *
 * The are also standard dialogs: message dialog, progress dialog.
 */
/*@{*/

/** default button numbers */
enum DiaButtonNumber {
	OK           =   1,         /**< default "OK" button number */
	CANCEL       =   2          /**< default "Cancel" button number */
};

/** type of DIALOGITEM::type */
enum DiaItemType {
	ITEMTYPE    =  017,         /**< mask for types of item */
	BUTTON      =    0,         /**< standard push-button */
	DEFBUTTON   =    1,         /**< default push-button */
	CHECK       =    2,         /**< standard check-box */
	RADIO       =    3,         /**< standard radio-button */
	SCROLL      =    4,         /**< scrolling list of text */
	SCROLLMULTI =    5,         /**< scrolling list of text (multiple lines can be selected) */
	MESSAGE     =    6,         /**< non-editable text */
	EDITTEXT    =    7,         /**< editable text */
	ICON        =  010,         /**< 32x32 icon */
	USERDRAWN   =  011,         /**< open area for user graphics */
	POPUP       =  012,         /**< popup menu */
	PROGRESS    =  013,         /**< progress bar (0-100%) */
	DIVIDELINE  =  014,         /**< dividing line */
	INACTIVE    = 0200,         /**< flag of inactive dialog item. Set if item is dimmed */
	ITEMTYPEEXT =03400|ITEMTYPE,/**< mask for extended types of item */
	AUTOCHECK   =00400|CHECK,   /**< check-button with automatic switching */
	RADIOA      =00400|RADIO,   /**< radio-button from radio-group A */
	RADIOB      =01000|RADIO,   /**< radio-button from radio-group B */
	RADIOC      =01400|RADIO,   /**< radio-button from radio-group C */
	RADIOD      =02000|RADIO,   /**< radio-button from radio-group D */
	RADIOE      =02400|RADIO,   /**< radio-button from radio-group E */
	RADIOF      =03000|RADIO,   /**< radio-button from radio-group F */
	RADIOG      =03400|RADIO,   /**< radio-button from radio-group G */
	OPAQUEEDIT  =00400|EDITTEXT	/**< non-displayed editable text */
};

/** the last parameter to "DiaInitTextDialog" */
enum DiaTextFlag {
	SCSELMOUSE   =   1,         /**< mouse clicks select in scroll area */
	SCSELKEY     =   2,         /**< key strokes select in scroll area */
	SCDOUBLEQUIT =   4,         /**< double-click in scroll area exits dialog */
	SCREPORT     =   8,         /**< report selections in the scroll area */
	SCHORIZBAR   =  16,         /**< want horizontal scroll-bar in scroll area */
	SCSMALLFONT  =  32,         /**< want small text in scroll area */
	SCFIXEDWIDTH =  64          /**< want fixed-width text in scroll area */
};

/** the mode to "DiaDrawLine" */
enum DiaDrawLineMode {
	DLMODEON     =   1,			/**< draw the line on */
	DLMODEOFF    =   2,			/**< draw the line off */
	DLMODEINVERT =   3			/**< draw the line inverted */
};

/** Description of dialog item.
 * Items are contained in DIALOG.
 */
typedef struct Idialogitem
{
	INTBIG        data;        /**< data storage for item (initially 0). Internal data storage
                                  that is used by the dialog system. It should be 0 initially. */
	RECTAREA      r;           /**< location of item. The bounds of the item */
	INTBIG        type;        /**< the type of the item - OR'ed #DialogItemType. */
	CHAR         *msg;         /**< initial item string placed in the item */
} DIALOGITEM;

/** Platform-independent description of dialog structure.
 * It consists of series of DIALOGITEM's.
 */
typedef struct
{
	RECTAREA    windowRect;    /**< location of dialog on the display */
	CHAR       *movable;       /**< dialog title (o if none).
                                  If 0, no title bar will be displayed for the dialog. */
	INTBIG      translated;    /**< translated into foreign language */
	INTBIG      items;         /**< number of items in the dialog */
	DIALOGITEM *list;          /**< array of dialog items. An array of DIALOGITEM
                                  structures that define the items in the dialog */
    CHAR       *uiFile;        /**< name of Qt ".ui" file with form description */
    INTBIG      briefHeight;   /**< height of brief part of extensible dialog.
                                  If 0, dialog is not extensible */
} DIALOG;

#ifdef __cplusplus

class EDialogPrivate;
#ifdef USEQT
class QStrList;
#endif

/** The EDialog is abstract class that provides platform-independent dialogs API.
 * There are two kinds of dialog - modal and modeless. They are implemented by
 * EDialogModal and EDialogModeless subclasses. In any case itemHitAction event
 * handler receives user events. The event handler can control the dialog using
 * EDialog member functions from dialog control and item control groups.
 * Dialog description is loaded from DIALOG structure. On Qt DIALOG::uiFile can
 * contain filename with ".ui" form which defines layout of stretchable dialog.
 */

class EDialog
{
	friend class EDialogPrivate;
public:
	/* Dialog control */
	/** Abstract dialog constructor.
	 * Dialogs usually are created by EDialogModal and EDialogModeless constructors.
     */
	EDialog(DIALOG *dialog); 
#if defined(__GNUC__) && __GNUC__ >= 3
	virtual ~EDialog(); //!< Destroys the dialog
#else
	~EDialog(); //!< Destroys the dialog
#endif
	/** This function can be reimplemented in a subclass to return dialog state to initial.
     * It is called when it is necessary to reset state of dialog to initial (for example,
     * after library saving.
	 */
	virtual void reset();
	/** obtains next item hit in dialog processing (obsolete) .
	 *	This function is obsolete. Use EDialogModal::exec() instead
	 */
	INTBIG nextHit();
	/** This event handler, for item \a itemHit, can be reimplemented in a subclass to receive events for the dialog.
	 * The \a itemHit is a item, hit by user.
	 */
	virtual void itemHitAction(INTBIG itemHit); 

	/** \name C++ API - Dialog control */
	/*@{*/	
	//! If showIt is TRUE, the dialog's extension is shown; otherwise the extension is hidden. 
    void showExtension( BOOLEAN showIt );
    BOOLEAN extension(); //!< Return TRUE, if the dialog's extension is shown (will be changed)
	/** changes dialog size (obsolete).
		This function is obsolete. Use EDialog::showExtension instead.
	 */
	void resizeDialog(INTBIG wid, INTBIG hei);
	void bringToTop(); //!< brings dialog to top.
	/*@}*/

	/** \name C++ API - Control of standard dialog items */
	/*@{*/	
	void setText(INTBIG item, CHAR *msg); //!< puts \a msg in \a item in the dialog
	CHAR *getText(INTBIG item); //!< gets text in \a item in the dialog
	void setControl(INTBIG item, INTBIG value); //!< puts \a value in \a item in the dialog
	INTBIG getControl(INTBIG item); //!< gets integer in \a item in the dialog
	BOOLEAN validEntry(INTBIG item); //!< returns true if \a item is nonblank in the dialog
	/*@}*/

	/** \name C++ API - Special dialog control */
	/*@{*/
	void dimItem(INTBIG item); //!< makes \a item dimmed in the dialog */
	void unDimItem(INTBIG item); //!< makes \a item undimmed in the dialog
	void noEditControl(INTBIG item); //!< makes \a item not editable in the dialog
	void editControl(INTBIG item); //!< makes \a item editable in the dialog
	void opaqueEdit(INTBIG item); //!< makes \a item not display text
	void defaultButton(INTBIG item); //!< makes \a item the default button
	void changeIcon(INTBIG item, UCHAR1 *icon); //!< sets the icon in \a item to \a addr
	/*@}*/

	/** \name C++ API -  Control of popup items */
	/*@{*/
	void setPopup(INTBIG item, INTBIG count, CHAR **names); //!< makes \a item a popup with \a count entries in \a names
	void setPopupEntry(INTBIG item, INTBIG entry); //!< makes popup \a item use entry \a entry as current
	INTBIG getPopupEntry(INTBIG item); //!< gets current entry in popup \a item
	/*@}*/

	/** \name C++ API - Control of scrollable items */
	/*@{*/
	/** This routine initializes the scroll-list \a item to contain strings obtained by the
     * \a toplist, \a nextinlist and \a donelist routines in the same way as initTextDialog does.
     * The scroll list has flags set to \a flags, which can be any combination of the following:
	 * ::SCSELMOUSE to allow mouse selection in the list; ::SCSELKEY to allow keystrokes to select
     * items; ::SCDOUBLEQUIT to allow double-clicks on the item to terminate dialog; ::SCREPORT to
     * report all hits in the scroll list; ::SCHORIZBAR to display horizontal scroll-bar in the list;
     * ::SCSMALLFONT to display the list with a small font; and ::SCFIXEDWIDTH to display the list with
     * a fixed-width font.
     */
	void initTextDialog(INTBIG item, BOOLEAN (*toplist)(CHAR **), CHAR *(*nextinlist)(void),
			void (*donelist)(void), INTBIG sortpos, INTBIG flags);
	/** This routine reloads the scroll-list \a item.
     * The scroll-list \a item will contain strings obtained by \a toplist, \a nextinlist,
     * and \a donelist routines. The routine first calls \a toplist to initialize the reporting
     * of strings (it takes the address of a string that is the starting point for parsing and it
     * should return nonzero), then makes repeative calls to \a nextinlist which returns strings
     * to be added to the scroll-list item (returns zero when done), and finally calls \a donelist
     * to terminate the reporting of strings. If no initial list is desired, use the routines
     * ::DiaNullDlogList, ::DiaNullDlogItem, ::DiaNullDlogDone. The items are sorted in character
     * position \a sortpos (-1 to not sort). \sa initTextDialog
	 */
	void loadTextDialog(INTBIG item, BOOLEAN (*toplist)(CHAR **), CHAR *(*nextinlist)(void),
			void (*donelist)(void), INTBIG sortpos);
	void stuffLine(INTBIG item, CHAR *line); //!< add \a line to \a item
	void selectLine(INTBIG item, INTBIG line); //!< select line \a l in \a item
	void selectLines(INTBIG item, INTBIG count, INTBIG *lines); //!< select \a count lines in \a lines in \a item
	INTBIG getCurLine(INTBIG item); //!< get current line number in scroll \a item
	INTBIG *getCurLines(INTBIG item); //!< get array of selected lines in scroll \a item
	INTBIG getNumScrollLines(INTBIG item); //!< returns number of lines in scroll \a item
	CHAR *getScrollLine(INTBIG item, INTBIG line); //!< get \a line in scroll \a item
	void setScrollLine(INTBIG item, INTBIG line, CHAR *msg); //!< set line \a which in scroll \a item
	void synchVScrolls(INTBIG item1, INTBIG item2, INTBIG item3); //!< synchronizes vertical scroll items \a item1 \a item2 \a item3
	void unSynchVScrolls(); //!< removes all vertical scroll synchronization
	/*@}*/

	/** \name C++ API - User-control of dialog items */
	/*@{*/
	void itemRect(INTBIG item, RECTAREA *rect); //!< get rectangle for \a item
	void percent(INTBIG item, INTBIG percent); //!< fill \a item with \a percent progress
	void redispRoutine(INTBIG item, void (*routine)(RECTAREA *rect, void *dia)); //!< call \a routine to redisplay \a item
	void allowUserDoubleClick(void); //!< double-click in user items reports #OK item
	//! draw \a count points in (\a xv, \a yv) with color (\a r, \a g, \a b)
	void fillPoly(INTBIG item, INTBIG *xv, INTBIG *yv, INTBIG count, INTBIG r, INTBIG g, INTBIG b); 
	void drawRect(INTBIG item, RECTAREA *rect, INTBIG r, INTBIG g, INTBIG b); //!< fill rectangle \a rect with "(\a r, \a g, \a b)
	void frameRect(INTBIG item, RECTAREA *r); //!< draw frame in rectangle \a r
	void invertRect(INTBIG item, RECTAREA *r); //!< invert color in rectangle \a r
	//! draw line from (\a fx, \a fy) to (\a tx, \a ty) with \a mode
	void drawLine(INTBIG item, INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty, INTBIG mode);
	void setTextSize(INTBIG size); //!< set size of text to \a size
	void getTextInfo(CHAR *msg, INTBIG *wid, INTBIG *hei); //!< get \a wid and \a hei of \a msg
	void putText(INTBIG item, CHAR *msg, INTBIG x, INTBIG y); //!< put text \ msg at (\a x, \a y)
	void trackCursor(void (*eachdown)(INTBIG x, INTBIG y)); //!< track cursor calling \a eachdown on mouse move
	void getMouse(INTBIG *x, INTBIG *y); //!< get current mouse position into \a x/ \a y
	/*@}*/

	DIALOG *itemDesc() { return itemdesc; } //!< DIALOG structure, describing this dialog
#ifdef USEQT
	static QStrList *itemNamesFromUi( CHAR *uiFile, BOOLEAN ext = FALSE );
#endif
protected:
	EDialogPrivate *d;
private:
    BOOLEAN isExtended;
	DIALOG *itemdesc;
};

/** The EDialogModal is a class that provides platform-independent modal dialogs API */

class EDialogModal: public EDialog
{
public:
	EDialogModal(DIALOG *dialog);
	enum DialogCode { Rejected, Accepted };
	DialogCode exec();
protected:
	void accept() { result = Accepted; in_loop = FALSE; }
	void reject() { result = Rejected; in_loop = FALSE; }
private:
	BOOLEAN in_loop;
	DialogCode result;
};

/** The EDialogModeless is a class that provides platform-independent modeless dialogs API.
 * Modeless dialog operates independently of other windows in the same application.
 * Call show() to display a modeless dialog. show() returns immediately so the flow of control
 * will continue in the calling code. Call hide() to hide dialog from screen.
 * EDialog::itemHitAction event handler receives user events.
 */

class EDialogModeless: public EDialog
{
public:
	EDialogModeless(DIALOG *dialog); //!< Creates modeless dialog from description
	void show(); //!< show dialog. Control returns immediately
	void hide(); //!< hide dialog. Dialog is not destroyed
	BOOLEAN isHidden(); //!< returns TRUE, if the dialog is hidden
};
#endif

/** \name C API - Dialog control */
/*@{*/
/** initializes dialog from table \a dialog.
 * This routine displays a dialog defined by \a dialog and set it to be the current dialog for
 * subsequent calls. If there is already a dialog on the screen, this one replaces it as the
 * curent one.
 * Returns the address of the dialog object (0 if dialog cannot be initialized).
 * \sa EDialogModal::EDialogModal(DIALOG *dialog)
 */
void   *DiaInitDialog(DIALOG *dialog);
/** initializes modeless dialog, call \a itemhit for hits.
 * Routine to initialize dialog \a dialog in modeless style, calling
 * \a itemhit routine with for each hit. First argument of \a itemhit is the \a handler,
 * second is the number of hitted item,
 * \sa EDialogModeless::EDialogModeless(DIALOG *dialog)
 */
void   *DiaInitDialogModeless(DIALOG *dialog, void (*itemhit)(void *dia, INTBIG item));
/** removes and terminates dialog.
 * This routine terminates the dialog \a dia and removes it from the screen
 */
void    DiaDoneDialog(void *dia);
/** changes dialog size.
 * This routine changes the size of the dialog \a dia to width \a wid and height \a hei
 */
void    DiaResizeDialog(void *dia, INTBIG wid, INTBIG hei);
/** brings dialog to top.
 * This routine forces dialog \a dia be displayed on top of other windows
 */
void    DiaBringToTop(void *dia);
/** obtains next item hit in dialog processing.
 * This routine handles actions and returns the next item that is hit in the dialog \a dia.
 */
INTBIG  DiaNextHit(void *dia);
/*@}*/

/** \name C API - Control of standard dialog items */
/*@{*/
/** puts \a msg in \a item in the dialog
 */
void    DiaSetText(void *dia, INTBIG item, CHAR *msg);
/** gets text in \a item in the dialog
 */
CHAR   *DiaGetText(void *dia, INTBIG item);
/** puts \a value in \a item in the dialog
 */
void    DiaSetControl(void *dia, INTBIG item, INTBIG value);
/** gets integer in \a item in the dialog
 */
INTBIG  DiaGetControl(void *dia, INTBIG item);
/** returns true if \a item is nonblank in the dialog
 */
BOOLEAN DiaValidEntry(void *dia, INTBIG item);
/*@}*/

/** \name C API - Special dialog control */
/*@{*/
/** makes \a item dimmed in the dialog */
void    DiaDimItem(void *dia, INTBIG item);
/** makes \a item undimmed in the dialog
 */
void    DiaUnDimItem(void *dia, INTBIG item);
/** makes \a item not editable in the dialog
 */
void    DiaNoEditControl(void *dia, INTBIG item);
/** makes \a item editable in the dialog
 */
void    DiaEditControl(void *dia, INTBIG item);
/** makes \a item not display text
 */
void    DiaOpaqueEdit(void *dia, INTBIG item);
/** makes \a item the default button
 */
void    DiaDefaultButton(void *dia, INTBIG item);
/** sets the icon in \a item to \a addr
 */
void    DiaChangeIcon(void *dia, INTBIG item, UCHAR1 *addr);
/*@}*/

/** \name C API - Control of popup items */
/*@{*/
/** makes \a item a popup with \a count entries in \a names
 */
void    DiaSetPopup(void *dia, INTBIG item, INTBIG count, CHAR **names);
/** makes popup \a item use entry \a entry as current
 */
void    DiaSetPopupEntry(void *dia, INTBIG item, INTBIG entry);
/** gets current entry in popup \a item
 */
INTBIG  DiaGetPopupEntry(void *dia, INTBIG item);
/*@}*/

/** \name C API - Control of scrollable items */
/*@{*/
/** \sa EDialog::initTextDialog
  */
void    DiaInitTextDialog(void *dia, INTBIG item, BOOLEAN (*toplist)(CHAR **),
			CHAR *(*nextinlist)(void), void (*donelist)(void),
            INTBIG sortpos, INTBIG flags);
/** \sa EDialog::loadTextDialog
 */
void    DiaLoadTextDialog(void *dia, INTBIG item, BOOLEAN (*toplist)(CHAR **),
			CHAR *(*nextinlist)(void), void (*donelist)(void), INTBIG sortpos);
/** add line \a msg to \a item
 */
void    DiaStuffLine(void *dia, INTBIG item, CHAR *line);
/** select line \a l in \a item
 */
void    DiaSelectLine(void *dia, INTBIG item, INTBIG which);
/** select \a c lines in \a l in \a item
 */
void    DiaSelectLines(void *dia, INTBIG item, INTBIG count, INTBIG *lines);
/** get current line number in scroll \a item
 */
INTBIG  DiaGetCurLine(void *dia, INTBIG item);
/** get array of selected lines in scroll \a item
 */
INTBIG *DiaGetCurLines(void *dia, INTBIG item);
/** returns number of lines in scroll \a item
 */
INTBIG  DiaGetNumScrollLines(void *dia, INTBIG item);
/** get line \a l in scroll \a item
 */
CHAR   *DiaGetScrollLine(void *dia, INTBIG item, INTBIG which);
/** set line \a which in scroll \a item
 */
void    DiaSetScrollLine(void *dia, INTBIG item, INTBIG which, CHAR *msg);
/** synchronizes vertical scroll items \a item1 \a item2 \a item3
 */
void    DiaSynchVScrolls(void *dia, INTBIG item1, INTBIG item2, INTBIG item3);
/** removes all vertical scroll synchronization
 */
void    DiaUnSynchVScrolls(void *dia);
/*@}*/

/** \name C API - User-control of dialog items */
/*@{*/
/** get rectangle for \a item
 */
void    DiaItemRect(void *dia, INTBIG item, RECTAREA *rect);
/** fill \a item with \a p percent progress
 */
void    DiaPercent(void *dia, INTBIG item, INTBIG p);
/** call \a routine to redisplay \a item
 */
void    DiaRedispRoutine(void *dia, INTBIG item, void (*routine)(RECTAREA *rect, void *dia));
/** double-click in user items reports #OK item
 */
void    DiaAllowUserDoubleClick(void *dia);
/** draw \a count points in (\a x, \a y) with color (\a r, \a g, \a b)
 */
void    DiaFillPoly(void *dia, INTBIG item, INTBIG *x, INTBIG *y, INTBIG count, INTBIG r, INTBIG g, INTBIG b);
/** fill rectangle \a rect with "(\a r, \a g, \a b)
 */
void    DiaDrawRect(void *dia, INTBIG item, RECTAREA *rect, INTBIG r, INTBIG g, INTBIG b);
/** draw frame in rectangle \a r
 */
void    DiaFrameRect(void *dia, INTBIG item, RECTAREA *r);
/** invert color in rectangle \a r
 */
void    DiaInvertRect(void *dia, INTBIG item, RECTAREA *r);
/** draw line from (\a fx, \a fy) to (\a tx, \a ty) with \a mode
 */
void    DiaDrawLine(void *dia, INTBIG item, INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty, INTBIG mode);
/** set size of text to \a size
 */
void    DiaSetTextSize(void *dia, INTBIG size);
/** get \a wid and \a hei of \a msg
 */
void    DiaGetTextInfo(void *dia, CHAR *msg, INTBIG *wid, INTBIG *hei);
/** put text \a msg at (\a x, \a y)
 */
void    DiaPutText(void *dia, INTBIG item, CHAR *msg, INTBIG x, INTBIG y);
/** track cursor calling \a eachdown on mouse move
 */
void    DiaTrackCursor(void *dia, void (*eachdown)(INTBIG x, INTBIG y));
/** get current mouse position into \a x/ \a y
 */
void    DiaGetMouse(void *dia, INTBIG *x, INTBIG *y);
/*@}*/

/** routine to show message in a message dialog */
void    DiaMessageInDialog(CHAR*, ...);

/** routine used by tools to create private dialogs.
 * The parameter \a terminputkeyword is the keyword that the "terminal input" command
 * will use to identify this dialog (zero if none). The parameter \a getlinecomp is the
 * command completion objects that, when parsed, will invoke the dialog. The parameter
 * \a routine is the code that runs the dialog.
 */
void    DiaDeclareHook(CHAR *terminputkeyword, COMCOMP *getlinecomp, void (*routine)(void));
/** routine used for toplist argument EDialog::initTextDialog and EDialog::loadTextDialog if empty initial list is desired */
BOOLEAN DiaNullDlogList(CHAR**);
/** routine used for nextinlist argument EDialog::initTextDialog and EDialog::loadTextDialog if empty initial list is desired */
CHAR   *DiaNullDlogItem(void);
/** routine used for donelist argument EDialog::initTextDialog and EDialog::loadTextDialog if empty initial list is desired */
void    DiaNullDlogDone(void);
/** routine translates dialog \a dia into a foreign-language.
 * It is called internally by the initialization routine and need not be used in standard dialog sequences
 */
void    DiaTranslate(DIALOG *dia);
/** routine to close (hide() and reset()) all modeless dialogs */
void    DiaCloseAllModeless();

/** \name Progress dialog
 * Progress dialog provides feedback on the progress of a slow operation.
 * It contains progress bar, text label and caption text. On Qt progress dialog
 * is shown only for long operations ( more than 4 sec ) and is hidden when
 * (\a progress == \a totalSteps)
 */
/*@{*/
/** init and show progress dialog.
 * If \a label is 0 "Reading file..." label is shown.
 * If \a caption is 0, caption is not shown.
 * Returned pointer is passed as dia argument to other progress bar routines
 */
void   *DiaInitProgress(CHAR *label, CHAR *caption);
/** update progress bar of progress dialog.
 * Progress bar will show \a dia to \a progress / \a totalSteps * 100 percent.
 * On Qt dialog is hidden if \a progress == \a totalSteps .
 */
void    DiaSetProgress(void *dia, INTBIG progress, INTBIG totalSteps);
/** change label of progress dialog */
void    DiaSetTextProgress(void *dia, CHAR *label);
/** get lable of progress dialog */
CHAR   *DiaGetTextProgress(void *dia);
/** change caption of progress dialog */
void    DiaSetCaptionProgress(void *dia, CHAR *caption);
/** destroy progress dialog */
void    DiaDoneProgress(void *dia);
/*@}*/

/*@}*/

extern DIALOG us_eprogressdialog;
extern DIALOG us_progressdialog;

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
