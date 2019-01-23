/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: graphpcdoc.cpp
 * implementation of the Cgraphpcdoc class
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

#include "graphpcstdafx.h"
#include "graphpc.h"

#include "graphpcdoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
/* CElectricDoc */

IMPLEMENT_DYNCREATE(CElectricDoc, CDocument)

BEGIN_MESSAGE_MAP(CElectricDoc, CDocument)
	//{{AFX_MSG_MAP(CElectricDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
/* CElectricDoc construction/destruction */

CElectricDoc::CElectricDoc()
{
}

CElectricDoc::~CElectricDoc()
{
}

BOOL CElectricDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
/* CElectricDoc serialization */

void CElectricDoc::Serialize(CArchive& ar)
{
}

/////////////////////////////////////////////////////////////////////////////
/* CElectricDoc diagnostics */

#ifdef _DEBUG
void CElectricDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CElectricDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif

/////////////////////////////////////////////////////////////////////////////
/* CElectricDoc commands */

extern "C" { void us_wanttoread(CHAR *name); }

BOOL CElectricDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;

	/* User double-clicked on a library: read it in */
	us_wanttoread((CHAR *)lpszPathName);

	return FALSE;
}
