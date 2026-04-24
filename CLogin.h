#pragma once

enum eLOGIN
{
	eLOGIN_success = 0
	, eLOGIN_defualtPW
	, eLOGIN_worngPW
	, eLOGIN_fail
};



// CLogin dialog

class CLogin : public CDialogEx
{
	DECLARE_DYNAMIC(CLogin)

public:
	CLogin(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CLogin();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_LOGIN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	int m_status;
	CString m_pw;
	//CString m_encodedPW;
	//CString m_decodedPW;
	CStatic m_staticNotice;
	CEdit m_editPassword;
	CButton m_btnChangePW;
	afx_msg void OnBnClickedBtnChangepassword();
	CButton m_btnLogin;
	afx_msg void OnBnClickedBtnLogin();
};
