#pragma once


// CChnagePassword dialog

class CChangePassword : public CDialogEx
{
	DECLARE_DYNAMIC(CChangePassword)

public:
	CChangePassword(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CChangePassword();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CHANEPAWSSROD };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_currentPW;
	CEdit m_editCurrentPW;
	CEdit m_editNewPW;
	CEdit m_editconfirmPW;
	afx_msg void OnBnClickedBtnChangepasswordconfirm();
	CString m_pw;
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	
	virtual BOOL OnInitDialog();
	virtual BOOL Create(LPCTSTR lpszTemplateName, CWnd* pParentWnd = NULL);
};
