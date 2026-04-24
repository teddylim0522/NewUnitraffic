#pragma once

#include <afxwin.h>
#include "CTcpService.h"

using namespace std;


class CTcpServer
{
public:
	CTcpServer();
	~CTcpServer();

public:
	int m_nCamIndex;
	void SetConfig(TCP_CONFIG tc, int nIndex) { 
		m_tc = tc; 
		m_nCamIndex = nIndex;
	};
	void SocketInit();	

	unsigned int ThreadAccept();
	unsigned int ThreadCheckData();
	void SetURL(char *pszSourceURL, char *pszDetectionURL) 
	{ 
		m_tc.strSourceURL = CA2W(pszSourceURL);
		m_tc.strDetectionUrl = CA2W(pszDetectionURL);
	}
	TCP_CONFIG GetConfig() { return m_tc; };

	void PushVdsData(CVdsData *pVD);
	void SaveSendFile(CVdsData *pVD);
	void WriteLog(CString strLog);

private:
	TCP_CONFIG m_tc;
	bool m_bOpenned;

	HANDLE m_hThreadAccept;
	DWORD m_dwAccept;
	BOOL m_bAccept;

	SOCKET m_sock;

	struct sockaddr_in m_server_addr;
	struct sockaddr_in m_client_addr;

	queue<CVdsData *> m_QVdsData;
	CRITICAL_SECTION	m_csVdsData;

	std::vector<CTcpService *> m_vectorService;
	CRITICAL_SECTION	m_csService;

	HANDLE m_hThreadCheckData;
	DWORD m_dwCheckData;
	BOOL m_bCheckData;

private:
	void CreateSocket();
	void CloseSocket();
	void SetSocket();
	void SocketListen();

	void StartThreadAccept();
	void StopThreadAccpet();

	void EmptyVdsDataQue();
	void ClearService();

	void StartThreadCheckData();
	void StopThreadCheckData();

	BOOL CheckConService();
	void PushSendVdsData();

	void SaveNotSentVdsData();
};

