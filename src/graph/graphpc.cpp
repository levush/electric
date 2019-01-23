/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: graphpc.cpp
 * Defines the class behaviors for the application
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
#include "graphpcchildframe.h"
#include "graphpcdoc.h"
#include "graphpcview.h"
#include "global.h"
#include <afxmt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#if LANGTCL
# include "dblang.h"
  INTBIG gra_initializetcl(void);
#endif

/////////////////////////////////////////////////////////////////////////////
/* CElectricApp */

BEGIN_MESSAGE_MAP(CElectricApp, CWinApp)
	//{{AFX_MSG_MAP(CElectricApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
/* CElectricApp construction */

CElectricApp::CElectricApp()
{
	/* Place all significant initialization in InitInstance */
}

/////////////////////////////////////////////////////////////////////////////
/* The one and only CElectricApp object */

CElectricApp gra_app;

/////////////////////////////////////////////////////////////////////////////
/* CElectricApp initialization */

BOOL CElectricApp::InitInstance()
{
	/* turn off memory debugging */
#ifdef _DEBUG
	AfxEnableMemoryTracking(false);
#endif

	/* we don't really need this */
	/* AfxInitRichEdit(); */

	/* call this when using MFC in a shared DLL */
#ifdef _AFXDLL
	Enable3dControls();
#endif

	/* change the registry key under which our settings are stored */
	SetRegistryKey(x_("Static-Free-Software"));

	/* load standard INI file options (including MRU) */
	LoadStdProfileSettings();

	/* register the application's document templates.  Document templates */
	/*  serve as the connection between documents, frame windows and views. */
	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(
		IDR_ELECTRTYPE,
		RUNTIME_CLASS(CElectricDoc),
		RUNTIME_CLASS(CChildFrame), /* custom MDI child frame */
		RUNTIME_CLASS(CElectricView));
	AddDocTemplate(pDocTemplate);

	/* create main MDI Frame window */
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

//	m_pMainWnd->SetWindowText("Son of Electric");
//	CMainFrame *wnd = (CMainFrame *)AfxGetMainWnd();
//	wnd->SetWindowText("Daughter of Electric");

	/* enable drag/drop open */
	m_pMainWnd->DragAcceptFiles();

	/* enable DDE Execute open */
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	/* parse command line for standard shell commands, DDE, file open */
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	/* dispatch commands specified on the command line */
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	/* get former location of Electric window (presume maximized if nothing saved) */
	CHAR *pt, tmp[256], subkey[256], value[256];
	WINDOWPLACEMENT wndpl;
	int top, left, bottom, right, fullscreen;
	INTBIG gra_getregistry(HKEY key, CHAR *subkey, CHAR *value, CHAR *result);
	pMainFrame->GetWindowPlacement(&wndpl);
	estrcpy(subkey, x_("Software\\Static Free Software\\Electric"));
	estrcpy(value, x_("Window_Location"));
	if (gra_getregistry(HKEY_LOCAL_MACHINE, subkey, value, tmp) == 0)
	{
		/* key has the format: T,L,B,R,MAX */
		pt = tmp;
		top = eatoi(pt);
		while(*pt != ',' && *pt != 0) pt++;
		if (*pt == ',')
		{
			pt++;
			left = eatoi(pt);
			while(*pt != ',' && *pt != 0) pt++;
			if (*pt == ',')
			{
				pt++;
				bottom = eatoi(pt);
				while(*pt != ',' && *pt != 0) pt++;
				if (*pt == ',')
				{
					pt++;
					right = eatoi(pt);
					while(*pt != ',' && *pt != 0) pt++;
					if (*pt == ',')
					{
						pt++;
						fullscreen = eatoi(pt);
					}
				}
			}
		}
	} else
	{
		/* nothing in the key: just maximize the window */
		fullscreen = SW_SHOWMAXIMIZED;
	}

	/* the main window has been initialized, so show and update it */
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	/* place the main window location as remembered */
	pMainFrame->GetWindowPlacement(&wndpl);
	wndpl.showCmd = fullscreen;
	if (wndpl.showCmd != SW_SHOWMAXIMIZED)
	{
		wndpl.rcNormalPosition.top = top;
		wndpl.rcNormalPosition.left = left;
		wndpl.rcNormalPosition.bottom = bottom;
		wndpl.rcNormalPosition.right = right;
	}
	pMainFrame->SetWindowPlacement(&wndpl);

	/* primary initialization of Electric */
	osprimaryosinit();

	/* secondary initialization of Electric */
	INTBIG argc;
	CHAR1 *argv[4];
	argc = 1;
	argv[0] = b_("electric");
	ossecondaryinit(argc, argv);

#if LANGTCL
	/* initialize TCL here */
	if (gra_initializetcl() == TCL_ERROR)
		error(_("TCL initialization failed: %s\n"), tcl_interp->result);
#endif
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/* App command to run the dialog */
void CElectricApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
/* CElectricApp commands */

void CElectricApp::OnFileNew()
{
}

BOOLEAN el_inslice = FALSE;

BOOL CElectricApp::OnIdle(LONG lCount)
{
	if (!el_inslice)
	{
		el_inslice = TRUE;
		tooltimeslice();
		el_inslice = FALSE;
	}

	return CWinApp::OnIdle(lCount);
}
