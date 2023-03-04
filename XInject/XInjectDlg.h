
// XInjectDlg.h : header file
//

#pragma once
#include "HookExInjector.h"
#include "RemoteThreadInjector.h"

// CXInjectDlg dialog
class CXInjectDlg : public CDialogEx
{
// Construction
public:
	CXInjectDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_XINJECT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

private:
	ExplorerHookInjector mExplorerHookInjector;
	RemoteThreadInjector mRemoteThreadInjector;

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnClickedButtonHook();
	afx_msg void OnClickedButtonUnhook();
	afx_msg void OnClickedButtonInject();
	afx_msg void OnClickedButtonEject();
	CEdit mInjectPidEdit;
};
