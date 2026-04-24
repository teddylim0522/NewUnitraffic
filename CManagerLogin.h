#pragma once


// CManagerLogin dialog
#define PWCHECK_TIMER		1226
class CManagerLogin : public CDialogEx
{
	DECLARE_DYNAMIC(CManagerLogin)

public:
	CManagerLogin(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CManagerLogin();

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_MANAGER_LOGIN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	CString m_password;

	CEdit m_editPW;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedBtnMgrlogin();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
