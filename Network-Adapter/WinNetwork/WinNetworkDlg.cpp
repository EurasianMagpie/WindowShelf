
// WinNetworkDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "WinNetwork.h"
#include "WinNetworkDlg.h"
#include "afxdialogex.h"

#include "network/NetworkMonitor.h"
#include "network/NetworkUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CWinNetworkDlg dialog



CWinNetworkDlg::CWinNetworkDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_WINNETWORK_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWinNetworkDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CWinNetworkDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_START_MONITOR, OnMessageStartMonitor)
//	ON_BN_CLICKED(IDC_BUTTON1, &CWinNetworkDlg::OnBnClickedButton1)
ON_BN_CLICKED(IDC_BTN_START_MONITOR, &CWinNetworkDlg::OnClickedBtnStartMonitor)
ON_BN_CLICKED(IDC_BTN_STOP_MONITOR, &CWinNetworkDlg::OnClickedBtnStopMonitor)
ON_BN_CLICKED(IDC_BTN_LIST_ADAPTER_INFO, &CWinNetworkDlg::OnBnClickedBtnListAdapterInfo)
END_MESSAGE_MAP()


// CWinNetworkDlg message handlers

BOOL CWinNetworkDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	LogUtil::Init("logs/win_network.log");

	PostMessage(WM_START_MONITOR);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CWinNetworkDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CWinNetworkDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CWinNetworkDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

LRESULT CWinNetworkDlg::OnMessageStartMonitor(WPARAM wParam, LPARAM lParam)
{
	NetworkMonitor::getInstance().start();
	return 0;
}

void CWinNetworkDlg::OnClickedBtnStartMonitor()
{
	NetworkMonitor::getInstance().start();
}

void CWinNetworkDlg::OnClickedBtnStopMonitor()
{
	NetworkMonitor::getInstance().stop();
}


void CWinNetworkDlg::OnBnClickedBtnListAdapterInfo()
{
	NetworkUtil::ListAdapterInfo();
}
