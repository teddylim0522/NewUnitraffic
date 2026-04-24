// EngineConfig.cpp : implementation file
//

#include "stdafx.h"
#include "UniTraffic.h"
#include "EngineConfig.h"
#include "afxdialogex.h"
#include "UniTrafficDoc.h"
#include "UniTrafficView.h"

// CEngineConfig dialog

IMPLEMENT_DYNAMIC(CEngineConfig, CDialogEx)

CEngineConfig::CEngineConfig(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_ENGINE_CONFIG, pParent)
{
	m_nGroupNum = 0;
	m_nSelect = -1;
	for (int i = 0; i < CAMGROUP_NUM; i++)
	{
     bGPUChanged[i] = false;
		bThreshChanged[i] = false;
	}
}

CEngineConfig::~CEngineConfig()
{

}

void CEngineConfig::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ENGINE_LISTBOX, m_EngineListCtrl);
	DDX_Control(pDX, IDC_GPU_ID, m_GpuSelectCtrl);
}


BEGIN_MESSAGE_MAP(CEngineConfig, CDialogEx)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_ENGINE_APPLY, &CEngineConfig::OnBnClickedEngineApply)
	ON_BN_CLICKED(IDC_ENGINE_CANCEL, &CEngineConfig::OnBnClickedEngineCancel)
	ON_CBN_SELCHANGE(IDC_ENGINE_LISTBOX, &CEngineConfig::OnCbnSelchangeEngineListbox)
END_MESSAGE_MAP()


// CEngineConfig message handlers

void CEngineConfig::display_defaultParameter()
{
	CString strTemp;

	if (theApp.m_nGroupNum > m_EngineListCtrl.GetCount())
	{
		for (int i = m_EngineListCtrl.GetCount(); i < theApp.m_nGroupNum; i++)
		{
			strTemp.Format(_T("ENGINE%d"), i + 1);
			m_EngineListCtrl.AddString(strTemp);
		}
	}

	int nGPUofEngine = 0, nGPUCount = 0;
	cudaGetDeviceCount(&nGPUCount);
	if (nGPUCount > m_GpuSelectCtrl.GetCount())
	{
		for (int i = m_GpuSelectCtrl.GetCount(); i < nGPUCount; i++)
		{
			strTemp.Format(_T("%d"), i);
			m_GpuSelectCtrl.AddString(strTemp);
		}
	}

	CRect MainRect, MainClientRect, EngineListrect, GPUListRect;

	this->GetWindowRect(MainRect);
	this->GetClientRect(MainClientRect);
	int cxBorder = (MainRect.Width() - MainClientRect.Width()) / 2;
	int cyBorder = MainRect.Height() - MainClientRect.Height() - 2 * GetSystemMetrics(SM_CYBORDER);

	m_EngineListCtrl.GetWindowRect(&EngineListrect);
	m_EngineListCtrl.SetWindowPos(nullptr, EngineListrect.left - MainRect.left - cxBorder, EngineListrect.top - MainRect.top - cyBorder, EngineListrect.Width(),
		(m_EngineListCtrl.GetCount() + 1) * EngineListrect.Height(), SWP_NOACTIVATE | SWP_NOZORDER);

	m_GpuSelectCtrl.GetWindowRect(&GPUListRect);
	m_GpuSelectCtrl.SetWindowPos(nullptr, GPUListRect.left - MainRect.left - cxBorder, GPUListRect.top - MainRect.top - cyBorder, GPUListRect.Width(),
		(m_GpuSelectCtrl.GetCount() + 1) * GPUListRect.Height(), SWP_NOACTIVATE | SWP_NOZORDER);

	m_EngineListCtrl.SetCurSel(0);

	for (int i = 0; i < nGPUCount; i++)
	{
		if (m_nOldGpuID[0] == i)
		{
			nGPUofEngine = i;
		}
	}

	m_GpuSelectCtrl.SetCurSel(nGPUofEngine);

	strTemp.Format(_T("%.2f"), m_fOldThresh[0]);
	GetDlgItem(IDC_THRESHOLD)->SetWindowText(strTemp);
}

void CEngineConfig::OnClose()
{
	// TODO: Add your message handler code here and/or call default
	DestroyWindow();
	CDialogEx::OnClose();
}


void CEngineConfig::OnBnClickedEngineApply()
{
	// TODO: Add your control notification handler code here

	m_nSelect = m_EngineListCtrl.GetCurSel();
	if (m_nSelect < 0)
	{
		CString text;
		m_EngineListCtrl.GetWindowTextA(text);
		m_nSelect = atoi(text);
	}

	int nGPUCount = 0;
	cudaGetDeviceCount(&nGPUCount);
	m_nNewGpuID[m_nSelect] = m_GpuSelectCtrl.GetCurSel();
	if (m_nNewGpuID[m_nSelect] < 0)
	{
		CString text;
		m_GpuSelectCtrl.GetWindowTextA(text);
		m_nNewGpuID[m_nSelect] = atoi(text);
	}

	if ((m_nSelect < 0 || m_nSelect > theApp.m_nGroupNum)
		|| (m_nNewGpuID[m_nSelect]<0 || m_nNewGpuID[m_nSelect] > nGPUCount - 1))
	{
		CString msg;
		if (theApp.m_nGroupNum == 1)
			msg.Format("Engine must be 1\n");
		else
			msg.Format("Engine must be in 1~%d\n", theApp.m_nGroupNum);

		if (nGPUCount == 1)
			msg.Format("%sGPUID must be 0", msg);
		else
			msg.Format("%sGPUID must be 0~%d", msg, nGPUCount - 1);
		MessageBox(msg, "Warning", MB_OK);
		return;
	}

	CString strTemp;
	GetDlgItem(IDC_THRESHOLD)->GetWindowText(strTemp);
	m_fNewThresh[m_nSelect] = _ttof(strTemp);
	if (m_fNewThresh[m_nSelect] < 0.1f || m_fNewThresh[m_nSelect] > 1.0f)
	{
		MessageBox(_T("Threshold must be in 0.1 ~ 1.0."), "Warning", MB_OK);
		return;
	}

	//check if any changed
	if (m_nNewGpuID[m_nSelect] != m_nOldGpuID[m_nSelect])
	{
		bGPUChanged[m_nSelect] = true;
	}

	if (m_fNewThresh[m_nSelect] != m_fOldThresh[m_nSelect])
	{
		bThreshChanged[m_nSelect] = true;
	}

	//apply
	if (bGPUChanged[m_nSelect] || bThreshChanged[m_nSelect])
	{
		if (MessageBox(_T("Do you want to change this Engine configuration?"), _T("Confirm"), MB_YESNO) == IDYES)
		{
			m_nOldGpuID[m_nSelect] = m_nNewGpuID[m_nSelect];
			m_fOldThresh[m_nSelect] = m_fNewThresh[m_nSelect];

			theApp.paramGroup[m_nSelect].nGpuId = m_nNewGpuID[m_nSelect];
			theApp.paramGroup[m_nSelect].thresh = m_fNewThresh[m_nSelect];

			update_Parameter();

			theApp.bTensorRestart[m_nSelect] = true;
			theApp.m_IsModelGroupLoaded[m_nSelect] = false;

			//if (bGPUChanged[m_nSelect])
			{
				for (int i = 0; i < theApp.m_nCameraNum; i++)
				{
					theApp.bCamRestart[m_nSelect][i] = true;
				}
			}

			theApp.KillTensorThread(m_nSelect);
			theApp.TensorDetectThread_Process(m_nSelect);
		}
		else
		{
			OnBnClickedEngineCancel();
		}
	}
	else
	{
		AfxMessageBox(_T("Nothing is changed. Please check again"));
	}

	bGPUChanged[m_nSelect] = false;
	bThreshChanged[m_nSelect] = false;
	m_nSelect = -1;
}


void CEngineConfig::OnBnClickedEngineCancel()
{
	// TODO: Add your control notification handler code here

	int nSelect = m_EngineListCtrl.GetCurSel();

	m_nNewGpuID[nSelect] = m_nOldGpuID[nSelect];
	m_fNewThresh[nSelect] = m_fOldThresh[nSelect];

	CString strTemp;

	int nGPUofEngine = 0, nGPUCount = 0;
	cudaGetDeviceCount(&nGPUCount);
	for (int i = 0; i < nGPUCount; i++)
	{
		if (m_nOldGpuID[nSelect] == i)
		{
			nGPUofEngine = i;
		}
	}

	m_GpuSelectCtrl.SetCurSel(nGPUofEngine);

	strTemp.Format(_T("%.2f"), m_fOldThresh[nSelect]);
	GetDlgItem(IDC_THRESHOLD)->SetWindowText(strTemp);
}


void CEngineConfig::update_Parameter()
{
	char currentPath[MAX_PATH], strIniFilePath[MAX_PATH];
	char m_strReturnString[MAX_PATH];

	sprintf(currentPath, "%s", getModulPath());
	wsprintf(strIniFilePath, "%s\\UniTraffic_Engine.ini", currentPath);

	//20190803
	struct stat   buffer;
	if (stat((const char*)(CT2CA)strIniFilePath, &buffer) != 0)
	{
		return;
	}

	char strEngineName[MAX_PATH];
	wsprintf(strEngineName, "ENGINE%d", m_nSelect + 1);

	CString tmp;
	tmp.Format(_T("%d"), m_nOldGpuID[m_nSelect]);
	WritePrivateProfileString(strEngineName, _T("GPU_ID"), tmp, strIniFilePath);

	tmp.Format(_T("%.2f"), m_fOldThresh[m_nSelect]);
	WritePrivateProfileString(strEngineName, _T("DETECT_THRESH"), tmp, strIniFilePath);
}

void CEngineConfig::OnCbnSelchangeEngineListbox()
{
	// TODO: Add your control notification handler code here

	m_nSelect = m_EngineListCtrl.GetCurSel();

	CString strTemp;

	int nGPUofEngine = 0, nGPUCount = 0;
	cudaGetDeviceCount(&nGPUCount);
	for (int i = 0; i < nGPUCount; i++)
	{
		if (m_nOldGpuID[m_nSelect] == i)
		{
			nGPUofEngine = i;
		}
	}

	m_GpuSelectCtrl.SetCurSel(nGPUofEngine);

	strTemp.Format(_T("%.2f"), m_fOldThresh[m_nSelect]);
	GetDlgItem(IDC_THRESHOLD)->SetWindowText(strTemp);

}
