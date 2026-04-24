#include "CTcpServer.h"

static unsigned int __stdcall _ThreadCheckData(LPVOID pArg)
{
	CTcpServer *pTcp = (CTcpServer *)pArg;
	return pTcp->ThreadCheckData();
}

static unsigned int __stdcall _ThreadAccept(LPVOID pArg)
{
	CTcpServer *pTcp = (CTcpServer *)pArg;
	return pTcp->ThreadAccept();
}

CTcpServer::CTcpServer()
	: m_bOpenned(FALSE)
	, m_hThreadAccept(NULL)
	, m_dwAccept(0)
	, m_bAccept(FALSE)
	, m_sock(SOCKET_ERROR)
	, m_hThreadCheckData(NULL)
	, m_dwCheckData(0)
	, m_bCheckData(FALSE)
{
	InitializeCriticalSection(&m_csService);
	InitializeCriticalSection(&m_csVdsData);
}

CTcpServer::~CTcpServer()
{
	StopThreadAccpet();
	StopThreadCheckData();
	CloseSocket();
	ClearService();
	EmptyVdsDataQue();
}

void CTcpServer::SocketInit()
{	
	CreateSocket();
	if (m_bOpenned)
	{
		SetSocket();
		SocketListen();
		StartThreadCheckData();

		CString strLog;
		strLog.Format(_T("[Start] SocketInit()"));
		WriteLog(strLog);
	}
}

void CTcpServer::CreateSocket()
{
	m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_sock == INVALID_SOCKET)
	{
		int temp = WSAGetLastError();
		TRACE(_T("Socket Error :%x(%d)\n"), temp, temp);
		m_bOpenned = FALSE;
	}
	else
	{
		m_bOpenned = TRUE;
	}
}

void CTcpServer::SetSocket()
{
	int temp = 0;
	int fBroadcast = 1;
	if (m_bOpenned)
	{
		ZeroMemory(&m_server_addr, sizeof(m_server_addr));

		m_server_addr.sin_family = AF_INET;
		m_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		m_server_addr.sin_port = htons(m_tc.nPort);

		int reuseAddress = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseAddress, sizeof(reuseAddress));

		int state, len;
		int snd_buf, rcv_buf;

		len = sizeof(snd_buf);
		state = getsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (char*)&snd_buf, &len);
		if (state)
		{
			TRACE(_T("getsockopt() Error\n"));
		}
		len = sizeof(rcv_buf);

		state = getsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (char*)&rcv_buf, &len);
		if (state)
		{
			TRACE(_T("getsockopt() Error\n"));
		}

		state = setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (char*)&m_tc.nRecvSize, sizeof(m_tc.nRecvSize));
		if (state)
		{
			TRACE(_T("setsockopt() Error\n"));
		}

		state = setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (char*)&m_tc.nSendSize, sizeof(m_tc.nSendSize));
		if (state)
		{
			TRACE(_T("setsockopt() Error\n"));
		}
	}
	else {
		TRACE(_T("Socket Not Open\n"));
	}
}

void CTcpServer::SocketListen()
{
	CString strLog;
	int temp = 0;

	temp = ::bind(m_sock, (SOCKADDR *)&m_server_addr, sizeof(m_server_addr));

	if (temp == INVALID_SOCKET)
	{
		TRACE(_T("bind fail \n"));
		strLog.Format(_T("[ERROR] CTcpServer Socket Bind Fail"));
		WriteLog(strLog);
		return;
	}

	if (listen(m_sock, 5) < 0) {
		TRACE(_T("listen fail \n"));
		strLog.Format(_T("[ERROR] CTcpServer Socket Listen Fail"));
		WriteLog(strLog);
		return;
	}

	StartThreadAccept();
	TRACE(_T("Wait..\n"));
}

void CTcpServer::StartThreadAccept()
{
	if (m_dwAccept)
		return;
	m_bAccept = TRUE;
	CString strLog;
	strLog.Format(_T("[START] CTcpServer StartThreadAccept"));
	WriteLog(strLog);
	m_hThreadAccept = ((HANDLE)_beginthreadex(NULL, 0, _ThreadAccept, this, 0, (unsigned int*)&m_dwAccept));
}

void CTcpServer::StopThreadAccpet()
{
	m_bAccept = FALSE;

	int nCount = 0;
	while (m_dwAccept)
	{
		if (nCount++ > _THREAD_TIMEOUT)
			break;
		Sleep(1);
	}
}

unsigned int CTcpServer::ThreadAccept()
{
	int client_len;
	CString strLog;
	fd_set fd_reads, cpset_reads;
	unsigned int fd_selchang;

	FD_ZERO(&fd_reads);
	FD_SET(m_sock, &fd_reads);
	TIMEVAL timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	ZeroMemory(&m_client_addr, sizeof(sockaddr_in));

	while (m_bAccept)
	{
		cpset_reads = fd_reads;
		if ((fd_selchang = select(0, &cpset_reads, 0, 0, &timeout)) == SOCKET_ERROR)
			break;
		if (fd_selchang == 0)
		{
			Sleep(100);
			continue;
		}

		ClearService();

		for (int i = 0; i < (int)fd_reads.fd_count; i++)
		{
			if (FD_ISSET(fd_reads.fd_array[i], &cpset_reads))
			{
				if (fd_reads.fd_array[i] == m_sock)
				{
					client_len = sizeof(m_client_addr);
					SOCKET clientsock = accept(m_sock, (SOCKADDR *)&m_client_addr, &client_len);

					if (clientsock == INVALID_SOCKET)
						continue;

					EnterCriticalSection(&m_csService);

					if (m_vectorService.size() < DEF_MAX_THERMAL_SERVICE)
					{
						CTcpService *pService = new CTcpService(clientsock, this);
						m_vectorService.push_back(pService);
					}
					else
					{
						closesocket(clientsock);
					}

					LeaveCriticalSection(&m_csService);


					char address[INET_ADDRSTRLEN];
					ZeroMemory(&address, sizeof(INET_ADDRSTRLEN));
					//inet_ntop(AF_INET, &m_client_addr.sin_addr, address, sizeof(address));
					inet_ntoa(m_client_addr.sin_addr);
					TRACE(_T("Socket Connect Success [%d] [%d]"), ntohs(m_client_addr.sin_port), clientsock);
					
					strLog.Format(_T("[Connect] Socket Connect Success [%d]"));
					WriteLog(strLog);
				}
			}
		}
		Sleep(100);
	}
	m_hThreadAccept = NULL;
	strLog.Format(_T("[STOP] CTcpServer::ThreadAccept() STOP"));
	WriteLog(strLog);
	return m_dwAccept = 0;
}

void CTcpServer::CloseSocket()
{
	if (m_bOpenned)
	{
		CString strLog;
		strLog.Format(_T("[End]CTcpServer::CTcpServer() End"));
		WriteLog(strLog);
	}
	closesocket(m_sock);
	m_sock = SOCKET_ERROR;
	m_bOpenned = FALSE;
}

void CTcpServer::EmptyVdsDataQue()
{
	EnterCriticalSection(&m_csVdsData);

	while (!m_QVdsData.empty())
	{
		CVdsData* pVd = m_QVdsData.front();
		m_QVdsData.pop();

		SaveSendFile(pVd);

		delete pVd;
		pVd = NULL;
	}
	LeaveCriticalSection(&m_csVdsData);
}

void CTcpServer::ClearService()
{
	EnterCriticalSection(&m_csService);
	auto itr = m_vectorService.begin();
	for (; itr != m_vectorService.end(); itr++)
	{
		CTcpService * ps = (CTcpService *)(*itr);
		if (ps)
		{
			delete ps;
		}
	}
	m_vectorService.clear();
	LeaveCriticalSection(&m_csService);
}

void CTcpServer::PushVdsData(CVdsData *pVD)
{
	EnterCriticalSection(&m_csVdsData);

	m_QVdsData.push(pVD);

	if (m_QVdsData.size() > MAX_QVDSDATA_SIZE)
	{
		OutputDebugString(_T("m_QVdsData.size() > MAX_QVDSDATA_SIZE\n"));
		CVdsData *pVD = m_QVdsData.front();
		if (pVD)
			SaveSendFile(pVD);
		m_QVdsData.pop();
		delete pVD;
		pVD = NULL;

	}

	LeaveCriticalSection(&m_csVdsData);
}

void CTcpServer::StartThreadCheckData()
{
	if (m_dwCheckData)
		return;

	m_bCheckData = TRUE;
	m_hThreadCheckData = ((HANDLE)_beginthreadex(NULL, 0, _ThreadCheckData, this, 0, (unsigned int*)&m_dwCheckData));
}

void CTcpServer::StopThreadCheckData()
{
	m_bCheckData = FALSE;
	int nCount = 0;
	while (m_dwCheckData)
	{
		if (nCount++ > _THREAD_TIMEOUT)
			break;
		Sleep(1);
	}
}

unsigned int CTcpServer::ThreadCheckData()
{
	while (m_bCheckData)
	{
		if (CheckConService())
		{
			//OutputDebugString(_T("########## Push Send VDS Data \n"));
			PushSendVdsData();
		}
		else
		{
			SaveNotSentVdsData();
			//OutputDebugString(_T("########## Save File \n"));
		}
		Sleep(10);
	}
	m_hThreadCheckData = NULL;
	return m_dwCheckData = 0;
}

BOOL CTcpServer::CheckConService()
{
	BOOL bResult = FALSE;
	CTcpService *pTcpService = NULL;
	EnterCriticalSection(&m_csService);

	if (m_vectorService.empty() == FALSE)
	{
		pTcpService = m_vectorService.front();
	}
	LeaveCriticalSection(&m_csService);

	if (pTcpService != NULL && pTcpService->GetSockNum() != SOCKET_ERROR && pTcpService->GetConStatus() != FALSE)
	{
		bResult = TRUE;
	}
	return bResult;
}

void CTcpServer::PushSendVdsData()
{
	CTcpService *pTcpService = NULL;
	CVdsData *pVD = NULL;

	EnterCriticalSection(&m_csService);

	if (m_vectorService.empty() == FALSE)
	{
		pTcpService = m_vectorService.front();
	}
	LeaveCriticalSection(&m_csService);

	if (pTcpService != NULL)
	{
		EnterCriticalSection(&m_csVdsData);

		if (m_QVdsData.empty() == FALSE)
		{
			pVD = m_QVdsData.front();
			m_QVdsData.pop();
		}
		LeaveCriticalSection(&m_csVdsData);

		if (pVD != NULL)
			pTcpService->PushSendVdsData(pVD);
	}

}

void CTcpServer::SaveNotSentVdsData()
{
	CVdsData *pVD = NULL;

	EnterCriticalSection(&m_csVdsData);

	if (m_QVdsData.empty() == FALSE)
	{
		pVD = m_QVdsData.front();
		m_QVdsData.pop();
	}
	LeaveCriticalSection(&m_csVdsData);

	if (pVD)
		SaveSendFile(pVD);
}

void CTcpServer::SaveSendFile(CVdsData *pVD)
{
	if (pVD == NULL)
		return;

	SYSTEMTIME  tCurTime;
	::GetLocalTime(&tCurTime);

	char currentPath[MAX_PATH], makePath[MAX_PATH];
	sprintf(currentPath, "%s", getModulPath());
	wsprintf(makePath, "%s\\VDS_data\\sendfail", currentPath);
	CreateDirectory(makePath, nullptr);

	CString strLog;
	strLog.Format(
		"%s,%d,"
		"%d,%d,%.2f,"
		"%d,%04d,%d,%d,"
		"%s,%04d,%s",
		pVD->strDetectTime.c_str(), pVD->nLane,
		pVD->nDirection, pVD->nLength, pVD->dSpeed,
		pVD->nClass, pVD->nOccupyTime, pVD->nLoop1OccupyTime, pVD->nLoop2OccupyTime,
		pVD->strReverseRun, pVD->nVehicleGap, pVD->strId.c_str());

	char fileName[MAX_PATH];
	wsprintf(fileName, "%s\\Log_%04d%02d%02d%02d_%02d.csv", makePath, (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay, (int)tCurTime.wHour, m_nCamIndex);

	try
	{
		FILE *log = nullptr;
		if (fopen_s(&log, fileName, "at") == 0)
		{
			fprintf_s(log, "%s\n", strLog.GetBuffer());
			fclose(log);
		}
	}
	catch (const std::exception& e)
	{
		CString strError;
		strError.Format(_T("error : %s\n"), e.what());
		OutputDebugString(strError);
	}

}

void CTcpServer::WriteLog(CString strLog)
{
	SYSTEMTIME  tCurTime;
	::GetLocalTime(&tCurTime);

	char currentPath[MAX_PATH], makePath[MAX_PATH];
	sprintf(currentPath, "%s", getModulPath());
	wsprintf(makePath, "%s\\CommLog", currentPath);

	CFileStatus fs;
	if (!CFile::GetStatus(makePath, fs))
	{
		CreateDirectory(makePath, nullptr);
	}

	char todayfolder[MAX_PATH];
	wsprintf(todayfolder, "%s\\%04d%02d%02d", makePath, (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay);

	if (!CFile::GetStatus(todayfolder, fs))
	{
		CreateDirectory(todayfolder, nullptr);
	}

	char fileName[MAX_PATH];
	wsprintf(fileName, "%s\\VDS_TCP_%04d%02d%02d_%02d.log", todayfolder, (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay, m_nCamIndex);

	CString strText;
	strText.Format(_T("%04d/%02d/%02d %02d:%02d:%02d:%03d : %s"), (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay, (int)tCurTime.wHour, (int)tCurTime.wMinute, (int)tCurTime.wSecond, (int)tCurTime.wMilliseconds, strLog.GetBuffer());

	try
	{
		FILE *log = nullptr;
		if (fopen_s(&log, fileName, "at") == 0)
		{
			fprintf_s(log, "%s\n", strText.GetBuffer());
			fclose(log);
		}
	}
	catch (const std::exception& e)
	{
		CString strError;
		strError.Format(_T("Log Write error : %s\n"), e.what());
		OutputDebugString(strError);
	}

}