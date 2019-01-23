/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: graphpcmainframe.cpp
 * implementation of the CMainFrame class
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
#include "graphpcmainframe.h"
#include "global.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
/* CMainFrame */

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_MENUSELECT()
	ON_COMMAND(ID_ELECTRIC_MENU01, OnElectricMenu01)
	ON_COMMAND(ID_ELECTRIC_MENU02, OnElectricMenu02)
	ON_COMMAND(ID_ELECTRIC_MENU03, OnElectricMenu03)
	ON_COMMAND(ID_ELECTRIC_MENU04, OnElectricMenu04)
	ON_COMMAND(ID_ELECTRIC_MENU05, OnElectricMenu05)
	ON_COMMAND(ID_ELECTRIC_MENU06, OnElectricMenu06)
	ON_COMMAND(ID_ELECTRIC_MENU07, OnElectricMenu07)
	ON_COMMAND(ID_ELECTRIC_MENU08, OnElectricMenu08)
	ON_COMMAND(ID_ELECTRIC_MENU09, OnElectricMenu09)
	ON_COMMAND(ID_ELECTRIC_MENU10, OnElectricMenu10)
	ON_COMMAND(ID_ELECTRIC_MENU11, OnElectricMenu11)
	ON_COMMAND(ID_ELECTRIC_MENU12, OnElectricMenu12)
	ON_COMMAND(ID_ELECTRIC_MENU13, OnElectricMenu13)
	ON_COMMAND(ID_ELECTRIC_MENU14, OnElectricMenu14)
	ON_COMMAND(ID_ELECTRIC_MENU15, OnElectricMenu15)
	ON_COMMAND(ID_ELECTRIC_MENU16, OnElectricMenu16)
	ON_COMMAND(ID_ELECTRIC_MENU17, OnElectricMenu17)
	ON_COMMAND(ID_ELECTRIC_MENU18, OnElectricMenu18)
	ON_COMMAND(ID_ELECTRIC_MENU19, OnElectricMenu19)
	ON_COMMAND(ID_ELECTRIC_MENU20, OnElectricMenu20)
	ON_COMMAND(ID_ELECTRIC_MENU21, OnElectricMenu21)
	ON_COMMAND(ID_ELECTRIC_MENU22, OnElectricMenu22)
	ON_COMMAND(ID_ELECTRIC_MENU23, OnElectricMenu23)
	ON_COMMAND(ID_ELECTRIC_MENU24, OnElectricMenu24)
	ON_COMMAND(ID_ELECTRIC_MENU25, OnElectricMenu25)
	ON_COMMAND(ID_ELECTRIC_MENU26, OnElectricMenu26)
	ON_COMMAND(ID_ELECTRIC_MENU27, OnElectricMenu27)
	ON_COMMAND(ID_ELECTRIC_MENU28, OnElectricMenu28)
	ON_COMMAND(ID_ELECTRIC_MENU29, OnElectricMenu29)
	ON_COMMAND(ID_ACCELERATOR_0, OnAccelerator0)
	ON_COMMAND(ID_ACCELERATOR_1, OnAccelerator1)
	ON_COMMAND(ID_ACCELERATOR_2, OnAccelerator2)
	ON_COMMAND(ID_ACCELERATOR_3, OnAccelerator3)
	ON_COMMAND(ID_ACCELERATOR_4, OnAccelerator4)
	ON_COMMAND(ID_ACCELERATOR_5, OnAccelerator5)
	ON_COMMAND(ID_ACCELERATOR_6, OnAccelerator6)
	ON_COMMAND(ID_ACCELERATOR_7, OnAccelerator7)
	ON_COMMAND(ID_ACCELERATOR_8, OnAccelerator8)
	ON_COMMAND(ID_ACCELERATOR_9, OnAccelerator9)
	ON_COMMAND(ID_ACCELERATOR_A, OnAcceleratorA)
	ON_COMMAND(ID_ACCELERATOR_B, OnAcceleratorB)
	ON_COMMAND(ID_ACCELERATOR_C, OnAcceleratorC)
	ON_COMMAND(ID_ACCELERATOR_D, OnAcceleratorD)
	ON_COMMAND(ID_ACCELERATOR_E, OnAcceleratorE)
	ON_COMMAND(ID_ACCELERATOR_F, OnAcceleratorF)
	ON_COMMAND(ID_ACCELERATOR_G, OnAcceleratorG)
	ON_COMMAND(ID_ACCELERATOR_H, OnAcceleratorH)
	ON_COMMAND(ID_ACCELERATOR_I, OnAcceleratorI)
	ON_COMMAND(ID_ACCELERATOR_J, OnAcceleratorJ)
	ON_COMMAND(ID_ACCELERATOR_K, OnAcceleratorK)
	ON_COMMAND(ID_ACCELERATOR_L, OnAcceleratorL)
	ON_COMMAND(ID_ACCELERATOR_M, OnAcceleratorM)
	ON_COMMAND(ID_ACCELERATOR_N, OnAcceleratorN)
	ON_COMMAND(ID_ACCELERATOR_O, OnAcceleratorO)
	ON_COMMAND(ID_ACCELERATOR_P, OnAcceleratorP)
	ON_COMMAND(ID_ACCELERATOR_Q, OnAcceleratorQ)
	ON_COMMAND(ID_ACCELERATOR_R, OnAcceleratorR)
	ON_COMMAND(ID_ACCELERATOR_S, OnAcceleratorS)
	ON_COMMAND(ID_ACCELERATOR_T, OnAcceleratorT)
	ON_COMMAND(ID_ACCELERATOR_U, OnAcceleratorU)
	ON_COMMAND(ID_ACCELERATOR_V, OnAcceleratorV)
	ON_COMMAND(ID_ACCELERATOR_W, OnAcceleratorW)
	ON_COMMAND(ID_ACCELERATOR_X, OnAcceleratorX)
	ON_COMMAND(ID_ACCELERATOR_Y, OnAcceleratorY)
	ON_COMMAND(ID_ACCELERATOR_Z, OnAcceleratorZ)
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           /* status line indicator */
};

/////////////////////////////////////////////////////////////////////////////
/* CMainFrame construction/destruction */

CMainFrame::CMainFrame()
{
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE(x_("%s"), x_("Failed to create status bar\n"));
		return -1;      /* fail to create */
	}

	/* count off tenths of a second */
	SetTimer(1, 100, 0);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT &cs)
{
	return CMDIFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
/* CMainFrame diagnostics */

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif

/////////////////////////////////////////////////////////////////////////////
/* CMainFrame message handlers */

short gWhichMenu;
void gra_nativemenudoone(long item, long menuindex);
void gra_resizemain(int cx, int cy);
int gra_closeworld(void);
void gra_timerticked(void);
void gra_keyaction(UINT nChar, INTBIG special, UINT nRepCnt);

void CMainFrame::OnElectricMenu01() { gra_nativemenudoone( 1, gWhichMenu); }
void CMainFrame::OnElectricMenu02() { gra_nativemenudoone( 2, gWhichMenu); }
void CMainFrame::OnElectricMenu03() { gra_nativemenudoone( 3, gWhichMenu); }
void CMainFrame::OnElectricMenu04() { gra_nativemenudoone( 4, gWhichMenu); }
void CMainFrame::OnElectricMenu05() { gra_nativemenudoone( 5, gWhichMenu); }
void CMainFrame::OnElectricMenu06() { gra_nativemenudoone( 6, gWhichMenu); }
void CMainFrame::OnElectricMenu07() { gra_nativemenudoone( 7, gWhichMenu); }
void CMainFrame::OnElectricMenu08() { gra_nativemenudoone( 8, gWhichMenu); }
void CMainFrame::OnElectricMenu09() { gra_nativemenudoone( 9, gWhichMenu); }
void CMainFrame::OnElectricMenu10() { gra_nativemenudoone(10, gWhichMenu); }
void CMainFrame::OnElectricMenu11() { gra_nativemenudoone(11, gWhichMenu); }
void CMainFrame::OnElectricMenu12() { gra_nativemenudoone(12, gWhichMenu); }
void CMainFrame::OnElectricMenu13() { gra_nativemenudoone(13, gWhichMenu); }
void CMainFrame::OnElectricMenu14() { gra_nativemenudoone(14, gWhichMenu); }
void CMainFrame::OnElectricMenu15() { gra_nativemenudoone(15, gWhichMenu); }
void CMainFrame::OnElectricMenu16() { gra_nativemenudoone(16, gWhichMenu); }
void CMainFrame::OnElectricMenu17() { gra_nativemenudoone(17, gWhichMenu); }
void CMainFrame::OnElectricMenu18() { gra_nativemenudoone(18, gWhichMenu); }
void CMainFrame::OnElectricMenu19() { gra_nativemenudoone(19, gWhichMenu); }
void CMainFrame::OnElectricMenu20() { gra_nativemenudoone(20, gWhichMenu); }
void CMainFrame::OnElectricMenu21() { gra_nativemenudoone(21, gWhichMenu); }
void CMainFrame::OnElectricMenu22() { gra_nativemenudoone(22, gWhichMenu); }
void CMainFrame::OnElectricMenu23() { gra_nativemenudoone(23, gWhichMenu); }
void CMainFrame::OnElectricMenu24() { gra_nativemenudoone(24, gWhichMenu); }
void CMainFrame::OnElectricMenu25() { gra_nativemenudoone(25, gWhichMenu); }
void CMainFrame::OnElectricMenu26() { gra_nativemenudoone(26, gWhichMenu); }
void CMainFrame::OnElectricMenu27() { gra_nativemenudoone(27, gWhichMenu); }
void CMainFrame::OnElectricMenu28() { gra_nativemenudoone(28, gWhichMenu); }
void CMainFrame::OnElectricMenu29() { gra_nativemenudoone(29, gWhichMenu); }

void CMainFrame::OnAccelerator0() { gra_keyaction('0', ACCELERATORDOWN, 1); }
void CMainFrame::OnAccelerator1() { gra_keyaction('1', ACCELERATORDOWN, 1); }
void CMainFrame::OnAccelerator2() { gra_keyaction('2', ACCELERATORDOWN, 1); }
void CMainFrame::OnAccelerator3() { gra_keyaction('3', ACCELERATORDOWN, 1); }
void CMainFrame::OnAccelerator4() { gra_keyaction('4', ACCELERATORDOWN, 1); }
void CMainFrame::OnAccelerator5() { gra_keyaction('5', ACCELERATORDOWN, 1); }
void CMainFrame::OnAccelerator6() { gra_keyaction('6', ACCELERATORDOWN, 1); }
void CMainFrame::OnAccelerator7() { gra_keyaction('7', ACCELERATORDOWN, 1); }
void CMainFrame::OnAccelerator8() { gra_keyaction('8', ACCELERATORDOWN, 1); }
void CMainFrame::OnAccelerator9() { gra_keyaction('9', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorA() { gra_keyaction('a', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorB() { gra_keyaction('b', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorC() { gra_keyaction('c', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorD() { gra_keyaction('d', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorE() { gra_keyaction('e', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorF() { gra_keyaction('f', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorG() { gra_keyaction('g', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorH() { gra_keyaction('h', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorI() { gra_keyaction('i', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorJ() { gra_keyaction('j', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorK() { gra_keyaction('k', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorL() { gra_keyaction('l', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorM() { gra_keyaction('m', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorN() { gra_keyaction('n', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorO() { gra_keyaction('o', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorP() { gra_keyaction('p', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorQ() { gra_keyaction('q', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorR() { gra_keyaction('r', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorS() { gra_keyaction('s', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorT() { gra_keyaction('t', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorU() { gra_keyaction('u', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorV() { gra_keyaction('v', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorW() { gra_keyaction('w', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorX() { gra_keyaction('x', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorY() { gra_keyaction('y', ACCELERATORDOWN, 1); }
void CMainFrame::OnAcceleratorZ() { gra_keyaction('z', ACCELERATORDOWN, 1); }

void CMainFrame::OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
	extern CMenu **gra_pulldownmenus;
	extern long    gra_pulldownmenucount;
	int i;

	if (hSysMenu == 0) return;
	if ((nFlags&MF_SEPARATOR) != 0) return;
	if ((nFlags&MF_POPUP) != 0) return;
	for(i=0; i<gra_pulldownmenucount; i++)
	{
		/*
		 * For some reason, the first version of Visual Studio (.NET)
		 * does not pass the HMENU as the 3rd argument to this routine,
		 * but instead passes the CMenu*.
		 *
		 * Unfortunately, the header is still declared as HMENU.
		 *
		 * This means that different code must be used to look up the value.
		 *
		 * To tell whether this is the first version of Visual Studio .NET,
		 * look at the compiler version.  It is:
		 *   1200 for Visual Studio 6,
		 *   1300 for the first version of Visual Studio .NET
		 *   1310 for the second version of Visual Studio .NET
		 */
#if _MSC_VER == 1300
		if ((CMenu *)hSysMenu == gra_pulldownmenus[i])
#else
		if (hSysMenu == gra_pulldownmenus[i]->m_hMenu)
#endif
		{
			gWhichMenu = i;
			return;
		}
	}
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CMDIFrameWnd::OnSize(nType, cx, cy);

	if (IsIconic()) return;
	gra_resizemain(cx, cy);
}

void CMainFrame::OnClose()
{
	if (gra_closeworld() != 0) return;

	CMDIFrameWnd::OnClose();
}

void CMainFrame::OnTimer(UINT nIDEvent) 
{
	gra_timerticked();
	
	CMDIFrameWnd::OnTimer(nIDEvent);
}
