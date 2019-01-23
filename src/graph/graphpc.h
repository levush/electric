/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: graphpc.h
 * main header file for the ELECTRIC application
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

#if !defined(AFX_ELECTRIC_H__07673642_BC3A_11D1_A088_0080C8775F8C__INCLUDED_)
#define AFX_ELECTRIC_H__07673642_BC3A_11D1_A088_0080C8775F8C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'graphpcstdafx.h' before including this file for PCH
#endif

#include "graphpcresource.h"       /* main symbols */

/////////////////////////////////////////////////////////////////////////////
/* CElectricApp: */
/* See Electric.cpp for the implementation of this class */

class CElectricApp : public CWinApp
{
public:
	CElectricApp();

/* Overrides */
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CElectricApp)
	public:
	virtual BOOL InitInstance();
	virtual BOOL OnIdle(LONG lCount);
	//}}AFX_VIRTUAL

/* Implementation */

	//{{AFX_MSG(CElectricApp)
	afx_msg void OnAppAbout();
	afx_msg void OnFileNew();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ELECTRIC_H__07673642_BC3A_11D1_A088_0080C8775F8C__INCLUDED_)
