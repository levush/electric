/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: graphpcchildframe.cpp
 * implementation of the CChildFrame class
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
#include "graphpcchildframe.h"
#include "global.h"
#include "usr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
/* CChildFrame */

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CChildFrame)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONDBLCLK()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_PAINT()
	ON_WM_CHAR()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_WM_MDIACTIVATE()
	ON_WM_KEYDOWN()
	ON_WM_MOVE()
	ON_WM_SETCURSOR()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
/* CChildFrame construction/destruction */

CChildFrame::CChildFrame()
{
	m_wndFloating = 0;
}

CChildFrame::~CChildFrame()
{
}

BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if (m_wndFloating != 0)
	{
		cs.dwExStyle |= WS_EX_TOOLWINDOW|WS_EX_PALETTEWINDOW; //|WS_EX_DLGMODALFRAME;
	}
	return CMDIChildWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
/* CChildFrame diagnostics */

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
	CMDIChildWnd::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
	CMDIChildWnd::Dump(dc);
}

#endif

/////////////////////////////////////////////////////////////////////////////
/* CChildFrame message handlers */

void gra_buttonaction(int state, UINT nFlags, CPoint point, CWnd *frm);
void gra_mouseaction(UINT nFlags, CPoint point, CWnd *frm);
void gra_mousewheelaction(UINT nFlags, short zDelta, CPoint point, CWnd *frm);
void gra_keyaction(UINT nChar, INTBIG special, UINT nRepCnt);
void gra_repaint(CChildFrame *frame, CPaintDC *dc);
void gra_resize(CChildFrame *frame, int cx, int cy);
int  gra_closeframe(CChildFrame *frame);
void gra_activateframe(CChildFrame *frame, BOOL bActivate);
void gra_movedwindow(CChildFrame *frame, int x, int y);
void gra_setdefaultcursor(void);
int gra_setpropercursor(CWnd *wnd, int x, int y);

void CChildFrame::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
	gra_buttonaction(0, nFlags|MK_LBUTTON, point, this);
}

void CChildFrame::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	gra_buttonaction(1, nFlags|MK_LBUTTON, point, this);
}

void CChildFrame::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	gra_buttonaction(2, nFlags|MK_LBUTTON, point, this);
}

void CChildFrame::OnMButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
	gra_buttonaction(0, nFlags|MK_MBUTTON, point, this);
}

void CChildFrame::OnMButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	gra_buttonaction(1, nFlags|MK_MBUTTON, point, this);
}

void CChildFrame::OnMButtonDblClk(UINT nFlags, CPoint point)
{
	gra_buttonaction(2, nFlags|MK_MBUTTON, point, this);
}

void CChildFrame::OnRButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
	gra_buttonaction(0, nFlags|MK_RBUTTON, point, this);
}

void CChildFrame::OnRButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	gra_buttonaction(1, nFlags|MK_RBUTTON, point, this);
}

void CChildFrame::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	gra_buttonaction(2, nFlags|MK_RBUTTON, point, this);
}

void CChildFrame::OnMouseMove(UINT nFlags, CPoint point)
{
	gra_mouseaction(nFlags, point, this);
}

BOOL CChildFrame::OnMouseWheel(UINT nFlags, short zDelta, CPoint point) 
{
	gra_mousewheelaction(nFlags, zDelta, point, this);
	return CMDIChildWnd::OnMouseWheel(nFlags, zDelta, point);
}

void CChildFrame::OnPaint()
{
	CPaintDC dc(this); /* device context for painting */

	gra_repaint(this, &dc);

	/* Do not call CMDIChildWnd::OnPaint() for painting messages */
}

void CChildFrame::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	void gra_onint(void);

	/* check for interrupt key */
	if (((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) != 0)
	{
		if (nChar == 'c')
		{
			gra_onint();
			return;
		}
	}
	gra_keyaction(nChar, 0, nRepCnt);
}

void CChildFrame::OnSize(UINT nType, int cx, int cy)
{
	CMDIChildWnd::OnSize(nType, cx, cy);

	gra_resize(this, cx, cy);
}

void CChildFrame::OnMove(int x, int y)
{
	CMDIChildWnd::OnMove(x, y);

	gra_movedwindow(this, x, y);
}

void CChildFrame::OnClose()
{
	if (gra_closeframe(this) == 0) return;

	CMDIChildWnd::OnClose();
}

void CChildFrame::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd)
{
	CMDIChildWnd::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);

	gra_activateframe(this, bActivate);
}

void CChildFrame::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
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

	/* determine which control/shift keys pressed */
	special = 0;
	if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) special |= ACCELERATORDOWN;
	if ((GetKeyState(VK_SHIFT) & 0x8000) != 0) special |= SHIFTDOWN;

	/* handle numeric keypad with control key pressed */
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

	if ((nFlags&0x100) != 0)
	{
		if (nChar == VK_DELETE)
			gra_keyaction(DELETEKEY, 0, nRepCnt);
		if (nChar == VK_UP)
		{
			gra_keyaction(0, SPECIALKEYDOWN|(SPECIALKEYARROWU<<SPECIALKEYSH)|special, nRepCnt);
		}
		if (nChar == VK_DOWN)
		{
			gra_keyaction(0, SPECIALKEYDOWN|(SPECIALKEYARROWD<<SPECIALKEYSH)|special, nRepCnt);
		}
		if (nChar == VK_RIGHT)
		{
			gra_keyaction(0, SPECIALKEYDOWN|(SPECIALKEYARROWR<<SPECIALKEYSH)|special, nRepCnt);
		}
		if (nChar == VK_LEFT)
		{
			gra_keyaction(0, SPECIALKEYDOWN|(SPECIALKEYARROWL<<SPECIALKEYSH)|special, nRepCnt);
		}
	}
	CMDIChildWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CChildFrame::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	POINT p, p2;

	GetCursorPos(&p);
	p2.x = p2.y = 0;
	MapWindowPoints(0, &p2, 1);
	if (gra_setpropercursor(pWnd, p.x-p2.x, p.y-p2.y) != 0) return(1);

	if (CMDIChildWnd::OnSetCursor(pWnd, nHitTest, message)) return(1);
	return(1);
}
