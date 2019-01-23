/*
 * Electric(tm) VLSI Design System
 *
 * File: graphpcdialog.cpp
 * Dialogs Implementation file
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
#include "graphpcdialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
/* CElectricDialog dialog */


CElectricDialog::CElectricDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CElectricDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CElectricDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CElectricDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CElectricDialog)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CElectricDialog, CDialog)
	//{{AFX_MSG_MAP(CElectricDialog)
	ON_WM_CREATE()
	ON_LBN_DBLCLK(ID_DIALOGITEM_0, OnDialogDoubleClick0)
	ON_LBN_DBLCLK(ID_DIALOGITEM_1, OnDialogDoubleClick1)
	ON_LBN_DBLCLK(ID_DIALOGITEM_2, OnDialogDoubleClick2)
	ON_LBN_DBLCLK(ID_DIALOGITEM_3, OnDialogDoubleClick3)
	ON_LBN_DBLCLK(ID_DIALOGITEM_4, OnDialogDoubleClick4)
	ON_LBN_DBLCLK(ID_DIALOGITEM_5, OnDialogDoubleClick5)
	ON_LBN_DBLCLK(ID_DIALOGITEM_6, OnDialogDoubleClick6)
	ON_LBN_DBLCLK(ID_DIALOGITEM_7, OnDialogDoubleClick7)
	ON_LBN_DBLCLK(ID_DIALOGITEM_8, OnDialogDoubleClick8)
	ON_LBN_DBLCLK(ID_DIALOGITEM_9, OnDialogDoubleClick9)
	ON_LBN_DBLCLK(ID_DIALOGITEM_10, OnDialogDoubleClick10)
	ON_LBN_DBLCLK(ID_DIALOGITEM_11, OnDialogDoubleClick11)
	ON_LBN_DBLCLK(ID_DIALOGITEM_12, OnDialogDoubleClick12)
	ON_LBN_DBLCLK(ID_DIALOGITEM_13, OnDialogDoubleClick13)
	ON_LBN_DBLCLK(ID_DIALOGITEM_14, OnDialogDoubleClick14)
	ON_LBN_DBLCLK(ID_DIALOGITEM_15, OnDialogDoubleClick15)
	ON_LBN_DBLCLK(ID_DIALOGITEM_16, OnDialogDoubleClick16)
	ON_LBN_DBLCLK(ID_DIALOGITEM_17, OnDialogDoubleClick17)
	ON_LBN_DBLCLK(ID_DIALOGITEM_18, OnDialogDoubleClick18)
	ON_LBN_DBLCLK(ID_DIALOGITEM_19, OnDialogDoubleClick19)
	ON_LBN_DBLCLK(ID_DIALOGITEM_20, OnDialogDoubleClick20)
	ON_LBN_DBLCLK(ID_DIALOGITEM_21, OnDialogDoubleClick21)
	ON_LBN_DBLCLK(ID_DIALOGITEM_22, OnDialogDoubleClick22)
	ON_LBN_DBLCLK(ID_DIALOGITEM_23, OnDialogDoubleClick23)
	ON_LBN_DBLCLK(ID_DIALOGITEM_24, OnDialogDoubleClick24)
	ON_LBN_DBLCLK(ID_DIALOGITEM_25, OnDialogDoubleClick25)
	ON_LBN_DBLCLK(ID_DIALOGITEM_26, OnDialogDoubleClick26)
	ON_LBN_DBLCLK(ID_DIALOGITEM_27, OnDialogDoubleClick27)
	ON_LBN_DBLCLK(ID_DIALOGITEM_28, OnDialogDoubleClick28)
	ON_LBN_DBLCLK(ID_DIALOGITEM_29, OnDialogDoubleClick29)
	ON_LBN_DBLCLK(ID_DIALOGITEM_30, OnDialogDoubleClick30)
	ON_LBN_DBLCLK(ID_DIALOGITEM_31, OnDialogDoubleClick31)
	ON_LBN_DBLCLK(ID_DIALOGITEM_32, OnDialogDoubleClick32)
	ON_LBN_DBLCLK(ID_DIALOGITEM_33, OnDialogDoubleClick33)
	ON_LBN_DBLCLK(ID_DIALOGITEM_34, OnDialogDoubleClick34)
	ON_LBN_DBLCLK(ID_DIALOGITEM_35, OnDialogDoubleClick35)
	ON_LBN_DBLCLK(ID_DIALOGITEM_36, OnDialogDoubleClick36)
	ON_LBN_DBLCLK(ID_DIALOGITEM_37, OnDialogDoubleClick37)
	ON_LBN_DBLCLK(ID_DIALOGITEM_38, OnDialogDoubleClick38)
	ON_LBN_DBLCLK(ID_DIALOGITEM_39, OnDialogDoubleClick39)
	ON_LBN_DBLCLK(ID_DIALOGITEM_40, OnDialogDoubleClick40)
	ON_LBN_DBLCLK(ID_DIALOGITEM_41, OnDialogDoubleClick41)
	ON_LBN_DBLCLK(ID_DIALOGITEM_42, OnDialogDoubleClick42)
	ON_LBN_DBLCLK(ID_DIALOGITEM_43, OnDialogDoubleClick43)
	ON_LBN_DBLCLK(ID_DIALOGITEM_44, OnDialogDoubleClick44)
	ON_LBN_DBLCLK(ID_DIALOGITEM_45, OnDialogDoubleClick45)
	ON_LBN_DBLCLK(ID_DIALOGITEM_46, OnDialogDoubleClick46)
	ON_LBN_DBLCLK(ID_DIALOGITEM_47, OnDialogDoubleClick47)
	ON_LBN_DBLCLK(ID_DIALOGITEM_48, OnDialogDoubleClick48)
	ON_LBN_DBLCLK(ID_DIALOGITEM_49, OnDialogDoubleClick49)
	ON_LBN_DBLCLK(ID_DIALOGITEM_50, OnDialogDoubleClick50)
	ON_LBN_DBLCLK(ID_DIALOGITEM_51, OnDialogDoubleClick51)
	ON_LBN_DBLCLK(ID_DIALOGITEM_52, OnDialogDoubleClick52)
	ON_LBN_DBLCLK(ID_DIALOGITEM_53, OnDialogDoubleClick53)
	ON_LBN_DBLCLK(ID_DIALOGITEM_54, OnDialogDoubleClick54)
	ON_LBN_DBLCLK(ID_DIALOGITEM_55, OnDialogDoubleClick55)
	ON_LBN_DBLCLK(ID_DIALOGITEM_56, OnDialogDoubleClick56)
	ON_LBN_DBLCLK(ID_DIALOGITEM_57, OnDialogDoubleClick57)
	ON_LBN_DBLCLK(ID_DIALOGITEM_58, OnDialogDoubleClick58)
	ON_LBN_DBLCLK(ID_DIALOGITEM_59, OnDialogDoubleClick59)
	ON_LBN_DBLCLK(ID_DIALOGITEM_60, OnDialogDoubleClick60)
	ON_LBN_DBLCLK(ID_DIALOGITEM_61, OnDialogDoubleClick61)
	ON_LBN_DBLCLK(ID_DIALOGITEM_62, OnDialogDoubleClick62)
	ON_LBN_DBLCLK(ID_DIALOGITEM_63, OnDialogDoubleClick63)
	ON_LBN_DBLCLK(ID_DIALOGITEM_64, OnDialogDoubleClick64)
	ON_LBN_DBLCLK(ID_DIALOGITEM_65, OnDialogDoubleClick65)
	ON_LBN_DBLCLK(ID_DIALOGITEM_66, OnDialogDoubleClick66)
	ON_LBN_DBLCLK(ID_DIALOGITEM_67, OnDialogDoubleClick67)
	ON_LBN_DBLCLK(ID_DIALOGITEM_68, OnDialogDoubleClick68)
	ON_LBN_DBLCLK(ID_DIALOGITEM_69, OnDialogDoubleClick69)
	ON_LBN_DBLCLK(ID_DIALOGITEM_70, OnDialogDoubleClick70)
	ON_EN_CHANGE(ID_DIALOGITEM_0, OnDialogTextChanged0)
	ON_EN_CHANGE(ID_DIALOGITEM_1, OnDialogTextChanged1)
	ON_EN_CHANGE(ID_DIALOGITEM_2, OnDialogTextChanged2)
	ON_EN_CHANGE(ID_DIALOGITEM_3, OnDialogTextChanged3)
	ON_EN_CHANGE(ID_DIALOGITEM_4, OnDialogTextChanged4)
	ON_EN_CHANGE(ID_DIALOGITEM_5, OnDialogTextChanged5)
	ON_EN_CHANGE(ID_DIALOGITEM_6, OnDialogTextChanged6)
	ON_EN_CHANGE(ID_DIALOGITEM_7, OnDialogTextChanged7)
	ON_EN_CHANGE(ID_DIALOGITEM_8, OnDialogTextChanged8)
	ON_EN_CHANGE(ID_DIALOGITEM_9, OnDialogTextChanged9)
	ON_EN_CHANGE(ID_DIALOGITEM_10, OnDialogTextChanged10)
	ON_EN_CHANGE(ID_DIALOGITEM_11, OnDialogTextChanged11)
	ON_EN_CHANGE(ID_DIALOGITEM_12, OnDialogTextChanged12)
	ON_EN_CHANGE(ID_DIALOGITEM_13, OnDialogTextChanged13)
	ON_EN_CHANGE(ID_DIALOGITEM_14, OnDialogTextChanged14)
	ON_EN_CHANGE(ID_DIALOGITEM_15, OnDialogTextChanged15)
	ON_EN_CHANGE(ID_DIALOGITEM_16, OnDialogTextChanged16)
	ON_EN_CHANGE(ID_DIALOGITEM_17, OnDialogTextChanged17)
	ON_EN_CHANGE(ID_DIALOGITEM_18, OnDialogTextChanged18)
	ON_EN_CHANGE(ID_DIALOGITEM_19, OnDialogTextChanged19)
	ON_EN_CHANGE(ID_DIALOGITEM_20, OnDialogTextChanged20)
	ON_EN_CHANGE(ID_DIALOGITEM_21, OnDialogTextChanged21)
	ON_EN_CHANGE(ID_DIALOGITEM_22, OnDialogTextChanged22)
	ON_EN_CHANGE(ID_DIALOGITEM_23, OnDialogTextChanged23)
	ON_EN_CHANGE(ID_DIALOGITEM_24, OnDialogTextChanged24)
	ON_EN_CHANGE(ID_DIALOGITEM_25, OnDialogTextChanged25)
	ON_EN_CHANGE(ID_DIALOGITEM_26, OnDialogTextChanged26)
	ON_EN_CHANGE(ID_DIALOGITEM_27, OnDialogTextChanged27)
	ON_EN_CHANGE(ID_DIALOGITEM_28, OnDialogTextChanged28)
	ON_EN_CHANGE(ID_DIALOGITEM_29, OnDialogTextChanged29)
	ON_EN_CHANGE(ID_DIALOGITEM_30, OnDialogTextChanged30)
	ON_EN_CHANGE(ID_DIALOGITEM_31, OnDialogTextChanged31)
	ON_EN_CHANGE(ID_DIALOGITEM_32, OnDialogTextChanged32)
	ON_EN_CHANGE(ID_DIALOGITEM_33, OnDialogTextChanged33)
	ON_EN_CHANGE(ID_DIALOGITEM_34, OnDialogTextChanged34)
	ON_EN_CHANGE(ID_DIALOGITEM_35, OnDialogTextChanged35)
	ON_EN_CHANGE(ID_DIALOGITEM_36, OnDialogTextChanged36)
	ON_EN_CHANGE(ID_DIALOGITEM_37, OnDialogTextChanged37)
	ON_EN_CHANGE(ID_DIALOGITEM_38, OnDialogTextChanged38)
	ON_EN_CHANGE(ID_DIALOGITEM_39, OnDialogTextChanged39)
	ON_EN_CHANGE(ID_DIALOGITEM_40, OnDialogTextChanged40)
	ON_EN_CHANGE(ID_DIALOGITEM_41, OnDialogTextChanged41)
	ON_EN_CHANGE(ID_DIALOGITEM_42, OnDialogTextChanged42)
	ON_EN_CHANGE(ID_DIALOGITEM_43, OnDialogTextChanged43)
	ON_EN_CHANGE(ID_DIALOGITEM_44, OnDialogTextChanged44)
	ON_EN_CHANGE(ID_DIALOGITEM_45, OnDialogTextChanged45)
	ON_EN_CHANGE(ID_DIALOGITEM_46, OnDialogTextChanged46)
	ON_EN_CHANGE(ID_DIALOGITEM_47, OnDialogTextChanged47)
	ON_EN_CHANGE(ID_DIALOGITEM_48, OnDialogTextChanged48)
	ON_EN_CHANGE(ID_DIALOGITEM_49, OnDialogTextChanged49)
	ON_EN_CHANGE(ID_DIALOGITEM_50, OnDialogTextChanged50)
	ON_EN_CHANGE(ID_DIALOGITEM_51, OnDialogTextChanged51)
	ON_EN_CHANGE(ID_DIALOGITEM_52, OnDialogTextChanged52)
	ON_EN_CHANGE(ID_DIALOGITEM_53, OnDialogTextChanged53)
	ON_EN_CHANGE(ID_DIALOGITEM_54, OnDialogTextChanged54)
	ON_EN_CHANGE(ID_DIALOGITEM_55, OnDialogTextChanged55)
	ON_EN_CHANGE(ID_DIALOGITEM_56, OnDialogTextChanged56)
	ON_EN_CHANGE(ID_DIALOGITEM_57, OnDialogTextChanged57)
	ON_EN_CHANGE(ID_DIALOGITEM_58, OnDialogTextChanged58)
	ON_EN_CHANGE(ID_DIALOGITEM_59, OnDialogTextChanged59)
	ON_EN_CHANGE(ID_DIALOGITEM_60, OnDialogTextChanged60)
	ON_EN_CHANGE(ID_DIALOGITEM_61, OnDialogTextChanged61)
	ON_EN_CHANGE(ID_DIALOGITEM_62, OnDialogTextChanged62)
	ON_EN_CHANGE(ID_DIALOGITEM_63, OnDialogTextChanged63)
	ON_EN_CHANGE(ID_DIALOGITEM_64, OnDialogTextChanged64)
	ON_EN_CHANGE(ID_DIALOGITEM_65, OnDialogTextChanged65)
	ON_EN_CHANGE(ID_DIALOGITEM_66, OnDialogTextChanged66)
	ON_EN_CHANGE(ID_DIALOGITEM_67, OnDialogTextChanged67)
	ON_EN_CHANGE(ID_DIALOGITEM_68, OnDialogTextChanged68)
	ON_EN_CHANGE(ID_DIALOGITEM_69, OnDialogTextChanged69)
	ON_EN_CHANGE(ID_DIALOGITEM_70, OnDialogTextChanged70)
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_VKEYTOITEM()
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
/* CElectricDialog message handlers */
void gra_itemclicked(CElectricDialog *diawin, int nID);
void gra_itemdoubleclicked(CElectricDialog *diawin, int nID);
void gra_diaredrawitem(CElectricDialog*);
void gra_dodialogtextchange(CElectricDialog *diawin, int nID);
void gra_buttonaction(int state, UINT nFlags, CPoint point, CWnd *frm);
void gra_mouseaction(UINT nFlags, CPoint point, CWnd *frm);
int gra_dodialoglistkey(CElectricDialog *diawin, UINT nKey, CListBox* pListBox, UINT nIndex);

BOOL CElectricDialog::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	void gra_onint(void);

	if (((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN) & GetKeyState('c')) & 0x8000) != 0)
	{
		gra_onint();
		return(1);
	}

	switch (nCode)
	{
		case 1:
		case BN_CLICKED:
			gra_itemclicked(this, nID);
			break;
	}
	return CDialog::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

int CElectricDialog::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;
	return 0;
}

BOOL CElectricDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	return TRUE;
}

void CElectricDialog::OnOK()
{
	gra_itemclicked(this, 1);
}

void CElectricDialog::OnCancel()
{
	gra_itemclicked(this, 2);
}

void CElectricDialog::OnDialogDoubleClick0() { gra_itemdoubleclicked(this, 0); }
void CElectricDialog::OnDialogDoubleClick1() { gra_itemdoubleclicked(this, 1); }
void CElectricDialog::OnDialogDoubleClick2() { gra_itemdoubleclicked(this, 2); }
void CElectricDialog::OnDialogDoubleClick3() { gra_itemdoubleclicked(this, 3); }
void CElectricDialog::OnDialogDoubleClick4() { gra_itemdoubleclicked(this, 4); }
void CElectricDialog::OnDialogDoubleClick5() { gra_itemdoubleclicked(this, 5); }
void CElectricDialog::OnDialogDoubleClick6() { gra_itemdoubleclicked(this, 6); }
void CElectricDialog::OnDialogDoubleClick7() { gra_itemdoubleclicked(this, 7); }
void CElectricDialog::OnDialogDoubleClick8() { gra_itemdoubleclicked(this, 8); }
void CElectricDialog::OnDialogDoubleClick9() { gra_itemdoubleclicked(this, 9); }
void CElectricDialog::OnDialogDoubleClick10() { gra_itemdoubleclicked(this, 10); }
void CElectricDialog::OnDialogDoubleClick11() { gra_itemdoubleclicked(this, 11); }
void CElectricDialog::OnDialogDoubleClick12() { gra_itemdoubleclicked(this, 12); }
void CElectricDialog::OnDialogDoubleClick13() { gra_itemdoubleclicked(this, 13); }
void CElectricDialog::OnDialogDoubleClick14() { gra_itemdoubleclicked(this, 14); }
void CElectricDialog::OnDialogDoubleClick15() { gra_itemdoubleclicked(this, 15); }
void CElectricDialog::OnDialogDoubleClick16() { gra_itemdoubleclicked(this, 16); }
void CElectricDialog::OnDialogDoubleClick17() { gra_itemdoubleclicked(this, 17); }
void CElectricDialog::OnDialogDoubleClick18() { gra_itemdoubleclicked(this, 18); }
void CElectricDialog::OnDialogDoubleClick19() { gra_itemdoubleclicked(this, 19); }
void CElectricDialog::OnDialogDoubleClick20() { gra_itemdoubleclicked(this, 20); }
void CElectricDialog::OnDialogDoubleClick21() { gra_itemdoubleclicked(this, 21); }
void CElectricDialog::OnDialogDoubleClick22() { gra_itemdoubleclicked(this, 22); }
void CElectricDialog::OnDialogDoubleClick23() { gra_itemdoubleclicked(this, 23); }
void CElectricDialog::OnDialogDoubleClick24() { gra_itemdoubleclicked(this, 24); }
void CElectricDialog::OnDialogDoubleClick25() { gra_itemdoubleclicked(this, 25); }
void CElectricDialog::OnDialogDoubleClick26() { gra_itemdoubleclicked(this, 26); }
void CElectricDialog::OnDialogDoubleClick27() { gra_itemdoubleclicked(this, 27); }
void CElectricDialog::OnDialogDoubleClick28() { gra_itemdoubleclicked(this, 28); }
void CElectricDialog::OnDialogDoubleClick29() { gra_itemdoubleclicked(this, 29); }
void CElectricDialog::OnDialogDoubleClick30() { gra_itemdoubleclicked(this, 30); }
void CElectricDialog::OnDialogDoubleClick31() { gra_itemdoubleclicked(this, 31); }
void CElectricDialog::OnDialogDoubleClick32() { gra_itemdoubleclicked(this, 32); }
void CElectricDialog::OnDialogDoubleClick33() { gra_itemdoubleclicked(this, 33); }
void CElectricDialog::OnDialogDoubleClick34() { gra_itemdoubleclicked(this, 34); }
void CElectricDialog::OnDialogDoubleClick35() { gra_itemdoubleclicked(this, 35); }
void CElectricDialog::OnDialogDoubleClick36() { gra_itemdoubleclicked(this, 36); }
void CElectricDialog::OnDialogDoubleClick37() { gra_itemdoubleclicked(this, 37); }
void CElectricDialog::OnDialogDoubleClick38() { gra_itemdoubleclicked(this, 38); }
void CElectricDialog::OnDialogDoubleClick39() { gra_itemdoubleclicked(this, 39); }
void CElectricDialog::OnDialogDoubleClick40() { gra_itemdoubleclicked(this, 40); }
void CElectricDialog::OnDialogDoubleClick41() { gra_itemdoubleclicked(this, 41); }
void CElectricDialog::OnDialogDoubleClick42() { gra_itemdoubleclicked(this, 42); }
void CElectricDialog::OnDialogDoubleClick43() { gra_itemdoubleclicked(this, 43); }
void CElectricDialog::OnDialogDoubleClick44() { gra_itemdoubleclicked(this, 44); }
void CElectricDialog::OnDialogDoubleClick45() { gra_itemdoubleclicked(this, 45); }
void CElectricDialog::OnDialogDoubleClick46() { gra_itemdoubleclicked(this, 46); }
void CElectricDialog::OnDialogDoubleClick47() { gra_itemdoubleclicked(this, 47); }
void CElectricDialog::OnDialogDoubleClick48() { gra_itemdoubleclicked(this, 48); }
void CElectricDialog::OnDialogDoubleClick49() { gra_itemdoubleclicked(this, 49); }
void CElectricDialog::OnDialogDoubleClick50() { gra_itemdoubleclicked(this, 50); }
void CElectricDialog::OnDialogDoubleClick51() { gra_itemdoubleclicked(this, 51); }
void CElectricDialog::OnDialogDoubleClick52() { gra_itemdoubleclicked(this, 52); }
void CElectricDialog::OnDialogDoubleClick53() { gra_itemdoubleclicked(this, 53); }
void CElectricDialog::OnDialogDoubleClick54() { gra_itemdoubleclicked(this, 54); }
void CElectricDialog::OnDialogDoubleClick55() { gra_itemdoubleclicked(this, 55); }
void CElectricDialog::OnDialogDoubleClick56() { gra_itemdoubleclicked(this, 56); }
void CElectricDialog::OnDialogDoubleClick57() { gra_itemdoubleclicked(this, 57); }
void CElectricDialog::OnDialogDoubleClick58() { gra_itemdoubleclicked(this, 58); }
void CElectricDialog::OnDialogDoubleClick59() { gra_itemdoubleclicked(this, 59); }
void CElectricDialog::OnDialogDoubleClick60() { gra_itemdoubleclicked(this, 60); }
void CElectricDialog::OnDialogDoubleClick61() { gra_itemdoubleclicked(this, 61); }
void CElectricDialog::OnDialogDoubleClick62() { gra_itemdoubleclicked(this, 62); }
void CElectricDialog::OnDialogDoubleClick63() { gra_itemdoubleclicked(this, 63); }
void CElectricDialog::OnDialogDoubleClick64() { gra_itemdoubleclicked(this, 64); }
void CElectricDialog::OnDialogDoubleClick65() { gra_itemdoubleclicked(this, 65); }
void CElectricDialog::OnDialogDoubleClick66() { gra_itemdoubleclicked(this, 66); }
void CElectricDialog::OnDialogDoubleClick67() { gra_itemdoubleclicked(this, 67); }
void CElectricDialog::OnDialogDoubleClick68() { gra_itemdoubleclicked(this, 68); }
void CElectricDialog::OnDialogDoubleClick69() { gra_itemdoubleclicked(this, 69); }
void CElectricDialog::OnDialogDoubleClick70() { gra_itemdoubleclicked(this, 70); }

void CElectricDialog::OnDialogTextChanged0() { gra_dodialogtextchange(this, 0); }
void CElectricDialog::OnDialogTextChanged1() { gra_dodialogtextchange(this, 1); }
void CElectricDialog::OnDialogTextChanged2() { gra_dodialogtextchange(this, 2); }
void CElectricDialog::OnDialogTextChanged3() { gra_dodialogtextchange(this, 3); }
void CElectricDialog::OnDialogTextChanged4() { gra_dodialogtextchange(this, 4); }
void CElectricDialog::OnDialogTextChanged5() { gra_dodialogtextchange(this, 5); }
void CElectricDialog::OnDialogTextChanged6() { gra_dodialogtextchange(this, 6); }
void CElectricDialog::OnDialogTextChanged7() { gra_dodialogtextchange(this, 7); }
void CElectricDialog::OnDialogTextChanged8() { gra_dodialogtextchange(this, 8); }
void CElectricDialog::OnDialogTextChanged9() { gra_dodialogtextchange(this, 9); }
void CElectricDialog::OnDialogTextChanged10() { gra_dodialogtextchange(this, 10); }
void CElectricDialog::OnDialogTextChanged11() { gra_dodialogtextchange(this, 11); }
void CElectricDialog::OnDialogTextChanged12() { gra_dodialogtextchange(this, 12); }
void CElectricDialog::OnDialogTextChanged13() { gra_dodialogtextchange(this, 13); }
void CElectricDialog::OnDialogTextChanged14() { gra_dodialogtextchange(this, 14); }
void CElectricDialog::OnDialogTextChanged15() { gra_dodialogtextchange(this, 15); }
void CElectricDialog::OnDialogTextChanged16() { gra_dodialogtextchange(this, 16); }
void CElectricDialog::OnDialogTextChanged17() { gra_dodialogtextchange(this, 17); }
void CElectricDialog::OnDialogTextChanged18() { gra_dodialogtextchange(this, 18); }
void CElectricDialog::OnDialogTextChanged19() { gra_dodialogtextchange(this, 19); }
void CElectricDialog::OnDialogTextChanged20() { gra_dodialogtextchange(this, 20); }
void CElectricDialog::OnDialogTextChanged21() { gra_dodialogtextchange(this, 21); }
void CElectricDialog::OnDialogTextChanged22() { gra_dodialogtextchange(this, 22); }
void CElectricDialog::OnDialogTextChanged23() { gra_dodialogtextchange(this, 23); }
void CElectricDialog::OnDialogTextChanged24() { gra_dodialogtextchange(this, 24); }
void CElectricDialog::OnDialogTextChanged25() { gra_dodialogtextchange(this, 25); }
void CElectricDialog::OnDialogTextChanged26() { gra_dodialogtextchange(this, 26); }
void CElectricDialog::OnDialogTextChanged27() { gra_dodialogtextchange(this, 27); }
void CElectricDialog::OnDialogTextChanged28() { gra_dodialogtextchange(this, 28); }
void CElectricDialog::OnDialogTextChanged29() { gra_dodialogtextchange(this, 29); }
void CElectricDialog::OnDialogTextChanged30() { gra_dodialogtextchange(this, 30); }
void CElectricDialog::OnDialogTextChanged31() { gra_dodialogtextchange(this, 31); }
void CElectricDialog::OnDialogTextChanged32() { gra_dodialogtextchange(this, 32); }
void CElectricDialog::OnDialogTextChanged33() { gra_dodialogtextchange(this, 33); }
void CElectricDialog::OnDialogTextChanged34() { gra_dodialogtextchange(this, 34); }
void CElectricDialog::OnDialogTextChanged35() { gra_dodialogtextchange(this, 35); }
void CElectricDialog::OnDialogTextChanged36() { gra_dodialogtextchange(this, 36); }
void CElectricDialog::OnDialogTextChanged37() { gra_dodialogtextchange(this, 37); }
void CElectricDialog::OnDialogTextChanged38() { gra_dodialogtextchange(this, 38); }
void CElectricDialog::OnDialogTextChanged39() { gra_dodialogtextchange(this, 39); }
void CElectricDialog::OnDialogTextChanged40() { gra_dodialogtextchange(this, 40); }
void CElectricDialog::OnDialogTextChanged41() { gra_dodialogtextchange(this, 41); }
void CElectricDialog::OnDialogTextChanged42() { gra_dodialogtextchange(this, 42); }
void CElectricDialog::OnDialogTextChanged43() { gra_dodialogtextchange(this, 43); }
void CElectricDialog::OnDialogTextChanged44() { gra_dodialogtextchange(this, 44); }
void CElectricDialog::OnDialogTextChanged45() { gra_dodialogtextchange(this, 45); }
void CElectricDialog::OnDialogTextChanged46() { gra_dodialogtextchange(this, 46); }
void CElectricDialog::OnDialogTextChanged47() { gra_dodialogtextchange(this, 47); }
void CElectricDialog::OnDialogTextChanged48() { gra_dodialogtextchange(this, 48); }
void CElectricDialog::OnDialogTextChanged49() { gra_dodialogtextchange(this, 49); }
void CElectricDialog::OnDialogTextChanged50() { gra_dodialogtextchange(this, 50); }
void CElectricDialog::OnDialogTextChanged51() { gra_dodialogtextchange(this, 51); }
void CElectricDialog::OnDialogTextChanged52() { gra_dodialogtextchange(this, 52); }
void CElectricDialog::OnDialogTextChanged53() { gra_dodialogtextchange(this, 53); }
void CElectricDialog::OnDialogTextChanged54() { gra_dodialogtextchange(this, 54); }
void CElectricDialog::OnDialogTextChanged55() { gra_dodialogtextchange(this, 55); }
void CElectricDialog::OnDialogTextChanged56() { gra_dodialogtextchange(this, 56); }
void CElectricDialog::OnDialogTextChanged57() { gra_dodialogtextchange(this, 57); }
void CElectricDialog::OnDialogTextChanged58() { gra_dodialogtextchange(this, 58); }
void CElectricDialog::OnDialogTextChanged59() { gra_dodialogtextchange(this, 59); }
void CElectricDialog::OnDialogTextChanged60() { gra_dodialogtextchange(this, 60); }
void CElectricDialog::OnDialogTextChanged61() { gra_dodialogtextchange(this, 61); }
void CElectricDialog::OnDialogTextChanged62() { gra_dodialogtextchange(this, 62); }
void CElectricDialog::OnDialogTextChanged63() { gra_dodialogtextchange(this, 63); }
void CElectricDialog::OnDialogTextChanged64() { gra_dodialogtextchange(this, 64); }
void CElectricDialog::OnDialogTextChanged65() { gra_dodialogtextchange(this, 65); }
void CElectricDialog::OnDialogTextChanged66() { gra_dodialogtextchange(this, 66); }
void CElectricDialog::OnDialogTextChanged67() { gra_dodialogtextchange(this, 67); }
void CElectricDialog::OnDialogTextChanged68() { gra_dodialogtextchange(this, 68); }
void CElectricDialog::OnDialogTextChanged69() { gra_dodialogtextchange(this, 69); }
void CElectricDialog::OnDialogTextChanged70() { gra_dodialogtextchange(this, 70); }


void CElectricDialog::OnPaint()
{
	CPaintDC dc(this); /* device context for painting */

	gra_diaredrawitem(this);
}

void CElectricDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	gra_mouseaction(nFlags, point, this);
	CDialog::OnMouseMove(nFlags, point);
}

void CElectricDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	gra_buttonaction(0, nFlags|MK_LBUTTON, point, this);
	CDialog::OnLButtonDown(nFlags, point);
}

void CElectricDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	gra_buttonaction(1, nFlags|MK_LBUTTON, point, this);
	CDialog::OnLButtonUp(nFlags, point);
}

int CElectricDialog::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex)
{
	if (gra_dodialoglistkey(this, nKey, pListBox, nIndex) == 0) return(-2);
	return(-1);
}

void CElectricDialog::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	gra_buttonaction(2, nFlags|MK_LBUTTON, point, this);
	CDialog::OnLButtonDblClk(nFlags, point);
}
