/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: graphpcmsgview.cpp
 * Message window implementation file
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
#include "graphpcmsgview.h"
#include "graphpcchildframe.h"
#include "global.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
/* CElectricMsgView */

IMPLEMENT_DYNCREATE(CElectricMsgView, CRichEditView)

COLORREF CElectricMsgView::fForeColor = RGB(0, 0, 0);
COLORREF CElectricMsgView::fBackColor = RGB(200, 200, 200);

CElectricMsgView::CElectricMsgView()
{
}

CElectricMsgView::~CElectricMsgView()
{
}

BEGIN_MESSAGE_MAP(CElectricMsgView, CRichEditView)
	//{{AFX_MSG_MAP(CElectricMsgView)
	ON_WM_CHAR()
	ON_WM_SIZE()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
/* CElectricMsgView drawing */

void CElectricMsgView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
}

/////////////////////////////////////////////////////////////////////////////
/* CElectricMsgView diagnostics */

#ifdef _DEBUG
void CElectricMsgView::AssertValid() const
{
	CRichEditView::AssertValid();
}

void CElectricMsgView::Dump(CDumpContext& dc) const
{
	CRichEditView::Dump(dc);
}
#endif

/////////////////////////////////////////////////////////////////////////////
/* CElectricMsgView message handlers */

void gra_activateframe(CChildFrame *frame, BOOL bActivate);
void gra_resize(CChildFrame *frame, int cx, int cy);
void gra_keyaction(UINT nChar, INTBIG special, UINT nRepCnt);

void CElectricMsgView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	gra_keyaction(nChar, 0, 1);
}

void CElectricMsgView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	extern CChildFrame *gra_messageswindow;

	CRichEditView::OnActivateView(bActivate, pActivateView, pDeactiveView);
	LoadColors();
	gra_activateframe(gra_messageswindow, bActivate);
}

void CElectricMsgView::OnSize(UINT nType, int cx, int cy)
{
	extern CChildFrame *gra_messageswindow;

	CRichEditView::OnSize(nType, cx, cy);

	if (cx != 0 && cy != 0 && gra_messageswindow != 0)
		gra_resize(gra_messageswindow, cx, cy);
}

void CElectricMsgView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	INTBIG special;

	/* handle function keys */
	if (nChar == VK_F1)  { gra_keyaction(0, SPECIALKEYDOWN|(SPECIALKEYF1 <<SPECIALKEYSH), nRepCnt); return; }
	if (nChar == VK_F2)  { gra_keyaction(0, SPECIALKEYDOWN|(SPECIALKEYF2 <<SPECIALKEYSH), nRepCnt); return; }
	if (nChar == VK_F3)  { gra_keyaction(0, SPECIALKEYDOWN|(SPECIALKEYF3 <<SPECIALKEYSH), nRepCnt); return; }
	if (nChar == VK_F4)  { gra_keyaction(0, SPECIALKEYDOWN|(SPECIALKEYF4 <<SPECIALKEYSH), nRepCnt); return; }
	if (nChar == VK_F5)  { gra_keyaction(0, SPECIALKEYDOWN|(SPECIALKEYF5 <<SPECIALKEYSH), nRepCnt); return; }
	if (nChar == VK_F6)  { gra_keyaction(0, SPECIALKEYDOWN|(SPECIALKEYF6 <<SPECIALKEYSH), nRepCnt); return; }
	if (nChar == VK_F7)  { gra_keyaction(0, SPECIALKEYDOWN|(SPECIALKEYF7 <<SPECIALKEYSH), nRepCnt); return; }
	if (nChar == VK_F8)  { gra_keyaction(0, SPECIALKEYDOWN|(SPECIALKEYF8 <<SPECIALKEYSH), nRepCnt); return; }
	if (nChar == VK_F9)  { gra_keyaction(0, SPECIALKEYDOWN|(SPECIALKEYF9 <<SPECIALKEYSH), nRepCnt); return; }
	if (nChar == VK_F10) { gra_keyaction(0, SPECIALKEYDOWN|(SPECIALKEYF10<<SPECIALKEYSH), nRepCnt); return; }
	if (nChar == VK_F11) { gra_keyaction(0, SPECIALKEYDOWN|(SPECIALKEYF11<<SPECIALKEYSH), nRepCnt); return; }
	if (nChar == VK_F12) { gra_keyaction(0, SPECIALKEYDOWN|(SPECIALKEYF12<<SPECIALKEYSH), nRepCnt); return; }

	/* handle numeric keypad with control key pressed */
	special = 0;
	if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) special |= ACCELERATORDOWN;
	if ((special&ACCELERATORDOWN) != 0)
	{
		if (nChar == VK_NUMPAD0)
			{ gra_keyaction('0', special, nRepCnt); return; }
		if (nChar == VK_NUMPAD1)
			{ gra_keyaction('1', special, nRepCnt); return; }
		if (nChar == VK_NUMPAD2)
			{ gra_keyaction('2', special, nRepCnt); return; }
		if (nChar == VK_NUMPAD3)
			{ gra_keyaction('3', special, nRepCnt); return; }
		if (nChar == VK_NUMPAD4)
			{ gra_keyaction('4', special, nRepCnt); return; }
		if (nChar == VK_NUMPAD5)
			{ gra_keyaction('5', special, nRepCnt); return; }
		if (nChar == VK_NUMPAD6)
			{ gra_keyaction('6', special, nRepCnt); return; }
		if (nChar == VK_NUMPAD7)
			{ gra_keyaction('7', special, nRepCnt); return; }
		if (nChar == VK_NUMPAD8)
			{ gra_keyaction('8', special, nRepCnt); return; }
		if (nChar == VK_NUMPAD9)
			{ gra_keyaction('9', special, nRepCnt); return; }
		if (nChar == VK_OEM_PLUS)
			{ gra_keyaction('+', special, nRepCnt); return; }
		if (nChar == VK_OEM_MINUS)
			{ gra_keyaction('-', special, nRepCnt); return; }
		if (nChar == VK_OEM_PERIOD)
			{ gra_keyaction('.', special, nRepCnt); return; }
		if (nChar == VK_OEM_COMMA)
			{ gra_keyaction(',', special, nRepCnt); return; }
	}

	CRichEditView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CElectricMsgView::SetColors(COLORREF fore, COLORREF back)
{
	fForeColor = fore;
	fBackColor = back;

	LoadColors();
	Invalidate();
}

void CElectricMsgView::LoadColors(void)
{
	/* added part: */
	CRichEditCtrl *ctrl;
	CHARFORMAT2 cf;
	long startsel, endsel;

	ctrl = &GetRichEditCtrl();
	if (ctrl == 0) return;
	ctrl->SetBackgroundColor(FALSE, fBackColor);

	/* set foreground color */
	ctrl->GetSel(startsel, endsel);
	ctrl->SetSel(0, -1);
	cf.cbSize = sizeof (CHARFORMAT2);
	cf.dwMask = CFM_COLOR;
	cf.dwEffects = 0;
	cf.crTextColor = fForeColor;
	ctrl->SetSelectionCharFormat(cf);
	ctrl->SetSel(startsel, endsel);
	ctrl->SetDefaultCharFormat(cf);
}
