/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: graphpcview.cpp
 * implementation of the CElectricView class
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
#include "graphpcview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
/* CElectricView */

IMPLEMENT_DYNCREATE(CElectricView, CView)

BEGIN_MESSAGE_MAP(CElectricView, CView)
	//{{AFX_MSG_MAP(CElectricView)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
/* CElectricView construction/destruction */

CElectricView::CElectricView()
{
}

CElectricView::~CElectricView()
{
}

BOOL CElectricView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
/* CElectricView drawing */

void CElectricView::OnDraw(CDC* pDC)
{
	CElectricDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
}

/////////////////////////////////////////////////////////////////////////////
/* CElectricView printing */

void CElectricView::PrintHook(void)
{
	CView::OnFilePrint();
}

BOOL CElectricView::OnPreparePrinting(CPrintInfo* pInfo)
{
	/* default preparation */
	return DoPreparePrinting(pInfo);
}

void CElectricView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

void CElectricView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

/////////////////////////////////////////////////////////////////////////////
/* CElectricView diagnostics */

#ifdef _DEBUG
void CElectricView::AssertValid() const
{
	CView::AssertValid();
}

void CElectricView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CElectricDoc* CElectricView::GetDocument() /* non-debug version is inline */
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CElectricDoc)));
	return(CElectricDoc*)m_pDocument;
}
#endif
