/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: graphpcmainframe.h
 * interface of the CMainFrame class
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

/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_GRAPHPCMAINFRAME_H__07673646_BC3A_11D1_A088_0080C8775F8C__INCLUDED_)
#define AFX_GRAPHPCMAINFRAME_H__07673646_BC3A_11D1_A088_0080C8775F8C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

/* Attributes */
public:

/* Operations */
public:

/* Overrides */
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

/* Implementation */
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

public:  /* control bar embedded members */
	CStatusBar   m_wndStatusBar;
	CFrameWnd   *m_wndComponentMenu;

/* Generated message map functions */
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu);
	afx_msg void OnElectricMenu01();
	afx_msg void OnElectricMenu02();
	afx_msg void OnElectricMenu03();
	afx_msg void OnElectricMenu04();
	afx_msg void OnElectricMenu05();
	afx_msg void OnElectricMenu06();
	afx_msg void OnElectricMenu07();
	afx_msg void OnElectricMenu08();
	afx_msg void OnElectricMenu09();
	afx_msg void OnElectricMenu10();
	afx_msg void OnElectricMenu11();
	afx_msg void OnElectricMenu12();
	afx_msg void OnElectricMenu13();
	afx_msg void OnElectricMenu14();
	afx_msg void OnElectricMenu15();
	afx_msg void OnElectricMenu16();
	afx_msg void OnElectricMenu17();
	afx_msg void OnElectricMenu18();
	afx_msg void OnElectricMenu19();
	afx_msg void OnElectricMenu20();
	afx_msg void OnElectricMenu21();
	afx_msg void OnElectricMenu22();
	afx_msg void OnElectricMenu23();
	afx_msg void OnElectricMenu24();
	afx_msg void OnElectricMenu25();
	afx_msg void OnElectricMenu26();
	afx_msg void OnElectricMenu27();
	afx_msg void OnElectricMenu28();
	afx_msg void OnElectricMenu29();
	afx_msg void OnAccelerator0();
	afx_msg void OnAccelerator1();
	afx_msg void OnAccelerator2();
	afx_msg void OnAccelerator3();
	afx_msg void OnAccelerator4();
	afx_msg void OnAccelerator5();
	afx_msg void OnAccelerator6();
	afx_msg void OnAccelerator7();
	afx_msg void OnAccelerator8();
	afx_msg void OnAccelerator9();
	afx_msg void OnAcceleratorA();
	afx_msg void OnAcceleratorB();
	afx_msg void OnAcceleratorC();
	afx_msg void OnAcceleratorD();
	afx_msg void OnAcceleratorE();
	afx_msg void OnAcceleratorF();
	afx_msg void OnAcceleratorG();
	afx_msg void OnAcceleratorH();
	afx_msg void OnAcceleratorI();
	afx_msg void OnAcceleratorJ();
	afx_msg void OnAcceleratorK();
	afx_msg void OnAcceleratorL();
	afx_msg void OnAcceleratorM();
	afx_msg void OnAcceleratorN();
	afx_msg void OnAcceleratorO();
	afx_msg void OnAcceleratorP();
	afx_msg void OnAcceleratorQ();
	afx_msg void OnAcceleratorR();
	afx_msg void OnAcceleratorS();
	afx_msg void OnAcceleratorT();
	afx_msg void OnAcceleratorU();
	afx_msg void OnAcceleratorV();
	afx_msg void OnAcceleratorW();
	afx_msg void OnAcceleratorX();
	afx_msg void OnAcceleratorY();
	afx_msg void OnAcceleratorZ();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHPCMAINFRAME_H__07673646_BC3A_11D1_A088_0080C8775F8C__INCLUDED_)
