
// WinNetworkDlg.h : header file
//

#pragma once

#define WM_START_MONITOR	WM_USER + 777

// CWinNetworkDlg dialog
class CWinNetworkDlg : public CDialogEx
{
// Construction
public:
	CWinNetworkDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_WINNETWORK_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnMessageStartMonitor(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnClickedBtnStartMonitor();
	afx_msg void OnClickedBtnStopMonitor();
	afx_msg void OnBnClickedBtnListAdapterInfo();
};
