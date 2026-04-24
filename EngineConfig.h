#pragma once

#include "common.h"
#include "cuda_runtime_api.h"

// CEngineConfig dialog

class CEngineConfig : public CDialogEx
{
	DECLARE_DYNAMIC(CEngineConfig)

public:
	CEngineConfig(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CEngineConfig();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ENGINE_CONFIG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:

	int m_nGroupNum;
	int m_nSelect;
	
	int m_nOldGpuID[CAMGROUP_NUM];
	int m_nNewGpuID[CAMGROUP_NUM];

	float m_fOldThresh[CAMGROUP_NUM];
	float m_fNewThresh[CAMGROUP_NUM];

	bool bGPUChanged[CAMGROUP_NUM];
	bool bThreshChanged[CAMGROUP_NUM];

	void display_defaultParameter();
	void update_Parameter();

	CComboBox m_EngineListCtrl;
	CComboBox m_GpuSelectCtrl;
	afx_msg void OnClose();
	afx_msg void OnBnClickedEngineApply();
	afx_msg void OnBnClickedEngineCancel();
	afx_msg void OnCbnSelchangeEngineListbox();
};
