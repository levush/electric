/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: graphpcmsgview.h
 * Messages window header file
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

#if !defined(AFX_GRAPHPCMSGVIEW_H__E9770C73_CD08_11D1_A09D_0080C8775F8C__INCLUDED_)
#define AFX_GRAPHPCMSGVIEW_H__E9770C73_CD08_11D1_A09D_0080C8775F8C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
/* CElectricMsgView view */

class CElectricMsgView : public CRichEditView
{
protected:
	CElectricMsgView();           /* protected constructor used by dynamic creation */
	DECLARE_DYNCREATE(CElectricMsgView)

/* Attributes */
	static COLORREF fForeColor;
	static COLORREF fBackColor;
	void LoadColors(void);

/* Operations */
public:
	void SetColors(COLORREF fore, COLORREF back);

/* Overrides */
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CElectricMsgView)
	protected:
	virtual void OnDraw(CDC* pDC);      /* overridden to draw this view */
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

/* Implementation */
protected:
	virtual ~CElectricMsgView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	/* Generated message map functions */
protected:
	//{{AFX_MSG(CElectricMsgView)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHPCMSGVIEW_H__E9770C73_CD08_11D1_A09D_0080C8775F8C__INCLUDED_)
