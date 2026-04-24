// CManagerLogin.cpp : implementation file
//

#include "stdafx.h"
#include "UniTraffic.h"
#include "CManagerLogin.h"
#include "afxdialogex.h"


// CManagerLogin dialog

IMPLEMENT_DYNAMIC(CManagerLogin, CDialogEx)

CManagerLogin::CManagerLogin(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_MANAGER_LOGIN, pParent)
{
}

CManagerLogin::~CManagerLogin()
{
	//EndDialog(m_checked);
}

void CManagerLogin::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_MNGPASSWORD, m_editPW);
}


BEGIN_MESSAGE_MAP(CManagerLogin, CDialogEx)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTN_MGRLOGIN, &CManagerLogin::OnBnClickedBtnMgrlogin)
END_MESSAGE_MAP()


// CManagerLogin message handlers

void CManagerLogin::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	switch (nIDEvent)
	{
	case PWCHECK_TIMER:
	{
		//KillTimer(PWCHECK_TIMER);

		//CWnd* msgWindow = FindWindow(nullptr, TEXT("Warning"));
		//if (msgWindow != nullptr)
		//	SendMessage(WM_CLOSE, (WPARAM)msgWindow->m_hWnd, 0);
		//	//PostMessage(WM_CLOSE, (WPARAM)msgWindow->m_hWnd, 0);

		break;
	}
	default:
		break;
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CManagerLogin::OnBnClickedBtnMgrlogin()
{
	// TODO: Add your control notification handler code here
	m_editPW.GetWindowText(m_password);

	if (m_password.GetLength() > 0 && m_password.Compare("unisemhi") == 0)
	{
		EndDialog(1000);
	}
	else
	{
		MessageBox("Password not matching.", "Warning");
		//SetTimer(PWCHECK_TIMER, 5000, NULL);
	}
}


BOOL CManagerLogin::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_ESCAPE:
		{
			break;
		}
		case VK_RETURN:
		{
			OnBnClickedBtnMgrlogin();
			return TRUE;
			//break;
		}
		default:
			break;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}
