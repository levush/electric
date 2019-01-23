/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: graphpcview.h
 * interface of the CElectricView class
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

#if !defined(AFX_GRAPHPCVIEW_H__0767364C_BC3A_11D1_A088_0080C8775F8C__INCLUDED_)
#define AFX_GRAPHPCVIEW_H__0767364C_BC3A_11D1_A088_0080C8775F8C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif /* _MSC_VER >= 1000 */

class CElectricView : public CView
{
protected: /* create from serialization only */
	CElectricView();
	DECLARE_DYNCREATE(CElectricView)

/* Attributes */
public:
	CElectricDoc* GetDocument();
	void PrintHook(void);

/* Operations */
public:

/* Overrides */
	/* ClassWizard generated virtual function overrides */
	//{{AFX_VIRTUAL(CElectricView)
	public:
	virtual void OnDraw(CDC* pDC);  /* overridden to draw this view */
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

/* Implementation */
public:
	virtual ~CElectricView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

/* Generated message map functions */
protected:
	//{{AFX_MSG(CElectricView)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  /* debug version in graphpcview.cpp */
inline CElectricDoc* CElectricView::GetDocument()
   { return(CElectricDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHPCVIEW_H__0767364C_BC3A_11D1_A088_0080C8775F8C__INCLUDED_)
