/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: graphpdcialog.h
 * Dialogs header file
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

#if !defined(AFX_GRAPHPDCIALOG_H__A58819F4_DD21_11D1_A0B2_0080C8775F8C__INCLUDED_)
#define AFX_GRAPHPDCIALOG_H__A58819F4_DD21_11D1_A0B2_0080C8775F8C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
/* CElectricDialog dialog */

class CElectricDialog : public CDialog
{
/* Construction */
public:
	CElectricDialog(CWnd* pParent = NULL);   /* standard constructor */

/* Dialog Data */
	//{{AFX_DATA(CElectricDialog)
	enum { IDD = IDD_ELECTRIC_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


/* Overrides */
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CElectricDialog)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

/* Implementation */
protected:

	// Generated message map functions
	//{{AFX_MSG(CElectricDialog)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDialogDoubleClick0();
	afx_msg void OnDialogDoubleClick1();
	afx_msg void OnDialogDoubleClick2();
	afx_msg void OnDialogDoubleClick3();
	afx_msg void OnDialogDoubleClick4();
	afx_msg void OnDialogDoubleClick5();
	afx_msg void OnDialogDoubleClick6();
	afx_msg void OnDialogDoubleClick7();
	afx_msg void OnDialogDoubleClick8();
	afx_msg void OnDialogDoubleClick9();
	afx_msg void OnDialogDoubleClick10();
	afx_msg void OnDialogDoubleClick11();
	afx_msg void OnDialogDoubleClick12();
	afx_msg void OnDialogDoubleClick13();
	afx_msg void OnDialogDoubleClick14();
	afx_msg void OnDialogDoubleClick15();
	afx_msg void OnDialogDoubleClick16();
	afx_msg void OnDialogDoubleClick17();
	afx_msg void OnDialogDoubleClick18();
	afx_msg void OnDialogDoubleClick19();
	afx_msg void OnDialogDoubleClick20();
	afx_msg void OnDialogDoubleClick21();
	afx_msg void OnDialogDoubleClick22();
	afx_msg void OnDialogDoubleClick23();
	afx_msg void OnDialogDoubleClick24();
	afx_msg void OnDialogDoubleClick25();
	afx_msg void OnDialogDoubleClick26();
	afx_msg void OnDialogDoubleClick27();
	afx_msg void OnDialogDoubleClick28();
	afx_msg void OnDialogDoubleClick29();
	afx_msg void OnDialogDoubleClick30();
	afx_msg void OnDialogDoubleClick31();
	afx_msg void OnDialogDoubleClick32();
	afx_msg void OnDialogDoubleClick33();
	afx_msg void OnDialogDoubleClick34();
	afx_msg void OnDialogDoubleClick35();
	afx_msg void OnDialogDoubleClick36();
	afx_msg void OnDialogDoubleClick37();
	afx_msg void OnDialogDoubleClick38();
	afx_msg void OnDialogDoubleClick39();
	afx_msg void OnDialogDoubleClick40();
	afx_msg void OnDialogDoubleClick41();
	afx_msg void OnDialogDoubleClick42();
	afx_msg void OnDialogDoubleClick43();
	afx_msg void OnDialogDoubleClick44();
	afx_msg void OnDialogDoubleClick45();
	afx_msg void OnDialogDoubleClick46();
	afx_msg void OnDialogDoubleClick47();
	afx_msg void OnDialogDoubleClick48();
	afx_msg void OnDialogDoubleClick49();
	afx_msg void OnDialogDoubleClick50();
	afx_msg void OnDialogDoubleClick51();
	afx_msg void OnDialogDoubleClick52();
	afx_msg void OnDialogDoubleClick53();
	afx_msg void OnDialogDoubleClick54();
	afx_msg void OnDialogDoubleClick55();
	afx_msg void OnDialogDoubleClick56();
	afx_msg void OnDialogDoubleClick57();
	afx_msg void OnDialogDoubleClick58();
	afx_msg void OnDialogDoubleClick59();
	afx_msg void OnDialogDoubleClick60();
	afx_msg void OnDialogDoubleClick61();
	afx_msg void OnDialogDoubleClick62();
	afx_msg void OnDialogDoubleClick63();
	afx_msg void OnDialogDoubleClick64();
	afx_msg void OnDialogDoubleClick65();
	afx_msg void OnDialogDoubleClick66();
	afx_msg void OnDialogDoubleClick67();
	afx_msg void OnDialogDoubleClick68();
	afx_msg void OnDialogDoubleClick69();
	afx_msg void OnDialogDoubleClick70();
	afx_msg void OnDialogTextChanged0();
	afx_msg void OnDialogTextChanged1();
	afx_msg void OnDialogTextChanged2();
	afx_msg void OnDialogTextChanged3();
	afx_msg void OnDialogTextChanged4();
	afx_msg void OnDialogTextChanged5();
	afx_msg void OnDialogTextChanged6();
	afx_msg void OnDialogTextChanged7();
	afx_msg void OnDialogTextChanged8();
	afx_msg void OnDialogTextChanged9();
	afx_msg void OnDialogTextChanged10();
	afx_msg void OnDialogTextChanged11();
	afx_msg void OnDialogTextChanged12();
	afx_msg void OnDialogTextChanged13();
	afx_msg void OnDialogTextChanged14();
	afx_msg void OnDialogTextChanged15();
	afx_msg void OnDialogTextChanged16();
	afx_msg void OnDialogTextChanged17();
	afx_msg void OnDialogTextChanged18();
	afx_msg void OnDialogTextChanged19();
	afx_msg void OnDialogTextChanged20();
	afx_msg void OnDialogTextChanged21();
	afx_msg void OnDialogTextChanged22();
	afx_msg void OnDialogTextChanged23();
	afx_msg void OnDialogTextChanged24();
	afx_msg void OnDialogTextChanged25();
	afx_msg void OnDialogTextChanged26();
	afx_msg void OnDialogTextChanged27();
	afx_msg void OnDialogTextChanged28();
	afx_msg void OnDialogTextChanged29();
	afx_msg void OnDialogTextChanged30();
	afx_msg void OnDialogTextChanged31();
	afx_msg void OnDialogTextChanged32();
	afx_msg void OnDialogTextChanged33();
	afx_msg void OnDialogTextChanged34();
	afx_msg void OnDialogTextChanged35();
	afx_msg void OnDialogTextChanged36();
	afx_msg void OnDialogTextChanged37();
	afx_msg void OnDialogTextChanged38();
	afx_msg void OnDialogTextChanged39();
	afx_msg void OnDialogTextChanged40();
	afx_msg void OnDialogTextChanged41();
	afx_msg void OnDialogTextChanged42();
	afx_msg void OnDialogTextChanged43();
	afx_msg void OnDialogTextChanged44();
	afx_msg void OnDialogTextChanged45();
	afx_msg void OnDialogTextChanged46();
	afx_msg void OnDialogTextChanged47();
	afx_msg void OnDialogTextChanged48();
	afx_msg void OnDialogTextChanged49();
	afx_msg void OnDialogTextChanged50();
	afx_msg void OnDialogTextChanged51();
	afx_msg void OnDialogTextChanged52();
	afx_msg void OnDialogTextChanged53();
	afx_msg void OnDialogTextChanged54();
	afx_msg void OnDialogTextChanged55();
	afx_msg void OnDialogTextChanged56();
	afx_msg void OnDialogTextChanged57();
	afx_msg void OnDialogTextChanged58();
	afx_msg void OnDialogTextChanged59();
	afx_msg void OnDialogTextChanged60();
	afx_msg void OnDialogTextChanged61();
	afx_msg void OnDialogTextChanged62();
	afx_msg void OnDialogTextChanged63();
	afx_msg void OnDialogTextChanged64();
	afx_msg void OnDialogTextChanged65();
	afx_msg void OnDialogTextChanged66();
	afx_msg void OnDialogTextChanged67();
	afx_msg void OnDialogTextChanged68();
	afx_msg void OnDialogTextChanged69();
	afx_msg void OnDialogTextChanged70();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg int OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHPDCIALOG_H__A58819F4_DD21_11D1_A0B2_0080C8775F8C__INCLUDED_)
