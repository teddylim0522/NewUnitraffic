// CChnagePassword.cpp : implementation file
//

#include "stdafx.h"
#include "UniTraffic.h"
#include "CChangePassword.h"
#include "afxdialogex.h"

#include "base64.h"

// CChnagePassword dialog

IMPLEMENT_DYNAMIC(CChangePassword, CDialogEx)

CChangePassword::CChangePassword(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CHANEPAWSSROD, pParent)
{

}

CChangePassword::~CChangePassword()
{
}

void CChangePassword::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_CURRENTPASSWORD, m_editCurrentPW);
	DDX_Control(pDX, IDC_EDIT_NEWPASSWORD, m_editNewPW);
	DDX_Control(pDX, IDC_EDIT_CONFIRMPASSWORD, m_editconfirmPW);
}


BEGIN_MESSAGE_MAP(CChangePassword, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_CHANGEPASSWORDCONFIRM, &CChangePassword::OnBnClickedBtnChangepasswordconfirm)
END_MESSAGE_MAP()


// CChnagePassword message handlers


BOOL CChangePassword::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	if (m_currentPW.GetLength() < 8 || m_currentPW.Compare("unisemhi") == 0)
	{
		m_editCurrentPW.EnableWindow(FALSE);
	}
	else
		m_editCurrentPW.EnableWindow(TRUE);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CChangePassword::OnBnClickedBtnChangepasswordconfirm()
{
	// TODO: Add your control notification handler code here
	CString currentPW, newPW, confirmPW;
	m_editCurrentPW.GetWindowText(currentPW);
	m_editNewPW.GetWindowText(newPW);
	m_editconfirmPW.GetWindowText(confirmPW);

	if (m_currentPW.GetLength() != 0 && m_currentPW.Compare("unisemhi") != 0)
	{
		if (currentPW.GetLength() < 8 || currentPW.Compare(m_currentPW) != 0)
		{
			MessageBox("Please check current password", "Warning");
			return;
		}
	}

	if (newPW.GetLength() < 8 || confirmPW.GetLength() < 8)
	{
		MessageBox("Plese set \"Password\" length more than 8 chracters.", "Warning");
	}
	else if (newPW.Compare(confirmPW) != 0)
	{
		MessageBox("Confirm Password not same New Password.", "Warning");
	}
	else if (newPW.Compare("unisemhi") == 0)
	{
		MessageBox("Can't use default password.", "Warning");
	}
	else
	{
		m_pw = newPW;

		//save on ini file
		char currentPath[MAX_PATH], strIniFilePath[MAX_PATH];

		sprintf(currentPath, "%s", getModulPath());
		wsprintf(strIniFilePath, "%s\\UniTraffic_Engine.ini", currentPath);

		struct stat   buffer;
		if (stat((const char*)(CT2CA)strIniFilePath, &buffer) != 0)
		{

		}
		else
		{
			int length = (m_pw.GetLength() / 3 + 1) * 4;
			char* output = (char*)malloc(sizeof(char)*length);
			if (output)
			{
				encode_base64((unsigned char *)output, (unsigned char *)m_pw.GetBuffer(), m_pw.GetLength());

				WritePrivateProfileString("INFO", "CAM_RUN_INFO", output, strIniFilePath);

				//m_encodedPW = output; //m_encodedPW.Format("%s", output);

				free(output);
				output = nullptr;
			}
		}

		EndDialog(1000);
	}
}


BOOL CChangePassword::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class

	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_ESCAPE:
		{
			return TRUE; //break;
		}
		case VK_RETURN:
		{
			OnBnClickedBtnChangepasswordconfirm();
			return TRUE;
			//break;
		}
		default:
			break;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

BOOL CChangePassword::Create(LPCTSTR lpszTemplateName, CWnd* pParentWnd)
{
	// TODO: Add your specialized code here and/or call the base class
	if (m_currentPW.Compare("unisemhi") == 0)
	{
		m_editCurrentPW.EnableWindow(FALSE);
	}
	else
		m_editCurrentPW.EnableWindow(TRUE);

	return CDialogEx::Create(lpszTemplateName, pParentWnd);
}
