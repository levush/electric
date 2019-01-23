/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: graphpcdoc.h
 * interface of the CElectricDoc class
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

#if !defined(AFX_GRAPHPCDOC_H__0767364A_BC3A_11D1_A088_0080C8775F8C__INCLUDED_)
#define AFX_GRAPHPCDOC_H__0767364A_BC3A_11D1_A088_0080C8775F8C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CElectricDoc : public CDocument
{
protected: /* create from serialization only */
	CElectricDoc();
	DECLARE_DYNCREATE(CElectricDoc)

/* Attributes */
public:

/* Operations */
public:

/* Overrides */
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CElectricDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL

/* Implementation */
public:
	virtual ~CElectricDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

/* Generated message map functions */
protected:
	//{{AFX_MSG(CElectricDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHPCDOC_H__0767364A_BC3A_11D1_A088_0080C8775F8C__INCLUDED_)
