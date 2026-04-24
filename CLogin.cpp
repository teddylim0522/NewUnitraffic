// CLogin.cpp : implementation file
//

#include "stdafx.h"
#include "UniTraffic.h"
#include "CLogin.h"
#include "afxdialogex.h"



#include "CChangePassword.h"

// CLogin dialog

IMPLEMENT_DYNAMIC(CLogin, CDialogEx)

CLogin::CLogin(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_LOGIN, pParent)
{

}

CLogin::~CLogin()
{
}

void CLogin::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_LOGINNOTICE, m_staticNotice);
	DDX_Control(pDX, IDC_EDIT_LOGIN, m_editPassword);
	DDX_Control(pDX, IDC_BTN_CHANGEPASSWORD, m_btnChangePW);
	DDX_Control(pDX, IDC_BTN_LOGIN, m_btnLogin);
}


BEGIN_MESSAGE_MAP(CLogin, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_CHANGEPASSWORD, &CLogin::OnBnClickedBtnChangepassword)
	ON_BN_CLICKED(IDC_BTN_LOGIN, &CLogin::OnBnClickedBtnLogin)	
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CLogin message handlers

BOOL CLogin::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	if (m_pw.GetLength() == 0 || m_pw.Compare("unisemhi") == 0)
	{
		m_staticNotice.SetWindowText("Please Change \"PassWord\" first.");
		m_btnLogin.EnableWindow(FALSE);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

int CLogin::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialogEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  Add your specialized creation code here

	m_status = eLOGIN::eLOGIN_fail;

	//m_decodedPW = "";
	//char* encoded = (char*)malloc(sizeof(char) * m_encodedPW.GetLength());
	//if (encoded)
	//{
	//	sprintf(encoded, "%s", m_encodedPW.GetBuffer(0));
	//	m_decodedPW = decode_base64(encoded, m_encodedPW.GetLength());
	//	free(encoded);
	//	encoded = nullptr;
	//}

	//if (m_pw.GetLength() == 0 || m_pw.Compare("unisemhi") == 0)
	//{
	//	m_staticNotice.SetWindowText("Please Change PassWord First");
	//	m_btnLogin.EnableWindow(FALSE);
	//}

	return 0;
}



void CLogin::OnClose()
{
	// TODO: Add your message handler code here and/or call default
	//CString password;
	//m_editPassword.GetWindowText(password);
	//
	//if (m_pw.GetLength() == 0 || m_pw.Compare("unisemhi") == 0)
	//{
	//	m_staticNotice.SetWindowText("Please Change PassWord First");
	//}
	//else if (password.GetLength() < 8 || m_pw.Compare(password) != 0)
	//{
	//	MessageBox("Please check password");
	//}
	//if(m_status == eLOGIN::eLOGIN_success)	
	//{
	//	
	//}
	CDialogEx::OnClose();
}

void CLogin::OnDestroy()
{
	//CDialogEx::OnDestroy();

	// TODO: Add your message handler code here
	//CString password;
	//m_editPassword.GetWindowText(password);
	//if (m_pw.GetLength() == 0 || m_pw.Compare("unisemhi") == 0)
	//{
	//	m_staticNotice.SetWindowText("Please Change PassWord First");
	//}
	//else if (password.GetLength() < 8 || m_pw.Compare(password) != 0)
	//{
	//	MessageBox("Please check password");
	//}
	//else //ideal close 
	//{
	//	CDialogEx::OnDestroy();
	//}
	if (m_status == eLOGIN::eLOGIN_success)
	{
		CDialogEx::OnClose();
	}
}



BOOL CLogin::PreTranslateMessage(MSG* pMsg)
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
			if (m_pw.GetLength() != 0 && m_pw.Compare("unisemhi") != 0)
			{
				OnBnClickedBtnLogin();
			}
			//return TRUE;
			return TRUE; //break;
		}
		default:
			break;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CLogin::OnBnClickedBtnChangepassword()
{
	// TODO: Add your control notification handler code here
	CChangePassword m_changePW;
	m_changePW.m_currentPW = m_pw;
	int ret = m_changePW.DoModal();
	if (ret == 1000)
	{
		m_btnLogin.EnableWindow(TRUE);

		m_staticNotice.SetWindowText("Please enter \"Password\".");

		m_pw = m_changePW.m_pw;
	}
}


void CLogin::OnBnClickedBtnLogin()
{
	// TODO: Add your control notification handler code here
	CString password;
	m_editPassword.GetWindowText(password);

	if (password.GetLength() < 8 || m_pw.Compare(password) != 0 || m_pw.Compare("unisemhi") == 0)
		MessageBox("Please check password", "Warning");
	else
	{
		m_status = eLOGIN::eLOGIN_success;
		EndDialog(eLOGIN::eLOGIN_success);
	}
}