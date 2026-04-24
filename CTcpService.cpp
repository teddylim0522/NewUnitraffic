#include "CTcpService.h"
#include "CTcpServer.h"

static const char STR_START[] = "01";

static unsigned int __stdcall _ThreadMsgRecv(LPVOID pArg)
{
	CTcpService *pTs = (CTcpService *)pArg;
	return pTs->ThreadMsgRecv();
}

static unsigned int __stdcall _ThreadRealTimeSend(LPVOID pArg)
{
	CTcpService *pTs = (CTcpService *)pArg;
	return pTs->ThreadRealTimeDataSend();
}

CTcpService::CTcpService(SOCKET sockfd, void *pParent)
	: m_bMsgRecv(FALSE)
	, m_hMsgRecv(NULL)
	, m_dwMsgRecv(0)
	, m_bRealTimeSend(FALSE)
	, m_hRealTimeSend(NULL)
	, m_dwRealTimeSend(0)
	, m_bConnect(FALSE)
{
	m_pParent = pParent;
	m_sock = sockfd;
	InitializeCriticalSection(&m_csSendVdsData);
	CString strLog;
	strLog.Format(_T("[START][%d] Service Start"), m_sock);
	WriteLog(strLog);
	StartThreadMsgRecv();
}

CTcpService::~CTcpService()
{
	m_bConnect = FALSE;
	StopThreadRealTimeDataSend();
	StopThreadMsgRecv();
	CString strLog;
	strLog.Format(_T("[End][%d] Service End"), m_sock);
	WriteLog(strLog);
	closesocket(m_sock);
	EmptySendVdsDataQue();
}

void CTcpService::EmptySendVdsDataQue()
{
	EnterCriticalSection(&m_csSendVdsData);

	while (!m_QSendVdsData.empty())
	{
		CVdsData* pvd = m_QSendVdsData.front();
		m_QSendVdsData.pop();
		delete pvd;
		pvd = NULL;
	}
	LeaveCriticalSection(&m_csSendVdsData);
}

void CTcpService::PushSendVdsData(CVdsData *pVD)
{
	EnterCriticalSection(&m_csSendVdsData);

	m_QSendVdsData.push(pVD);

	if (m_QSendVdsData.size() > MAX_QVDSDATA_SIZE)
	{
		CString strLog;
		strLog.Format(_T("[ERROR] max queue size error"));
		WriteLog(strLog);
		CVdsData *pVD = m_QSendVdsData.front();
		CTcpServer *pTs = (CTcpServer *)m_pParent;

		if (pTs)
			pTs->SaveSendFile(pVD);

		m_QSendVdsData.pop();
		delete pVD;
		pVD = NULL;

	}
	LeaveCriticalSection(&m_csSendVdsData);
}

void CTcpService::StartThreadMsgRecv()
{
	if (m_dwMsgRecv)
		return;
	m_bMsgRecv = TRUE;
	m_hMsgRecv = (HANDLE)_beginthreadex(NULL, 0, _ThreadMsgRecv, this, 0, (unsigned int*)&m_dwMsgRecv);
}

void CTcpService::StopThreadMsgRecv()
{
	m_bMsgRecv = FALSE;
	int nCount = 0;
	while (m_dwMsgRecv)
	{
		if (nCount++ > _THREAD_TIMEOUT)
			break;
		Sleep(1);
	}
}

unsigned int CTcpService::ThreadMsgRecv()
{
	char buffer[MAX_MTU_SIZE];
	FD_SET recvfds;
	struct timeval	timeout;
	CString strLog;

	struct sockaddr_in client_addr;
	int client_addr_size = sizeof(client_addr);

	while (m_bMsgRecv)
	{
		char strStart[2] = { 0 , };

		ZeroMemory(&buffer, MAX_MTU_SIZE);
		int nMsgSize = 0;


		FD_ZERO(&recvfds);
		FD_SET(m_sock, &recvfds);

		//1초
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		int nState = 0;
		nState = select(m_sock, &recvfds, 0, 0, &timeout);
		if (nState == SOCKET_ERROR) // -1
		{
			OutputDebugString(_T("SELECT_SOCKET_ERROR\n"));
			strLog.Format(_T("[ERROR] socket error"));
			WriteLog(strLog);
			m_bConnect = FALSE;
			break;
		}
		else if (nState == 0)
		{
			//timeout
			Sleep(100);
			continue;
		}
		else if (nState == WSAENETDOWN)
		{
			OutputDebugString(_T("WSAENETDOWN_2\n"));

		}
		else if (nState == WSAENOTSOCK)
		{
			OutputDebugString(_T("WSAENOTSOCK_3\n"));
		}
		else
		{
			for (int i = 0; i < (int)recvfds.fd_count; i++)
			{
				if (FD_ISSET(m_sock, &recvfds))
				{
					nMsgSize = recvfrom(m_sock, (char*)buffer, sizeof(buffer), 0, (struct sockaddr*)&client_addr, &client_addr_size);

					CString strText;
					strText.Format(_T("recv [%d] \n"), nMsgSize);
					OutputDebugString(strText);
					if (nMsgSize == 0)
					{
						break;
					}

					if (nMsgSize == SOCKET_ERROR)
					{
						m_bConnect = FALSE;
						break;
					}
					else
					{
						OnRecvMessage((char*)buffer, nMsgSize);
					}
				}
				else
				{
					OutputDebugString(_T("FD_ISSET ERROR\n"));
				}
			}
			//
			if (nMsgSize == SOCKET_ERROR)
			{
				m_bConnect = FALSE;
				OutputDebugString(_T("SOCKET_ERROR\n"));
				strLog.Format(_T("[ERROR] socket error"));
				WriteLog(strLog);
				break;
			}
			if (nMsgSize == 0)
			{
				OutputDebugString(_T("INVALID_SOCKET\n"));
				strLog.Format(_T("[ERROR] invalid_socket"));
				WriteLog(strLog);
				break;
			}
		}

		Sleep(100);
	}
	m_hMsgRecv = NULL;
	return m_dwMsgRecv = 0;
}

void CTcpService::StartThreadRealTimeDataSend()
{
	if (m_dwRealTimeSend)
		return;

	m_bRealTimeSend = TRUE;
	m_hRealTimeSend = (HANDLE)_beginthreadex(NULL, 0, _ThreadRealTimeSend, this, 0, (unsigned int*)&m_dwRealTimeSend);
}

void CTcpService::StopThreadRealTimeDataSend()
{
	m_bRealTimeSend = FALSE;
	int nCount = 0;
	while (m_dwRealTimeSend)
	{
		if (nCount++ > _THREAD_TIMEOUT)
			break;
		Sleep(1);
	}
}

unsigned int CTcpService::ThreadRealTimeDataSend()
{
	while (m_bRealTimeSend)
	{
		if (m_sock != SOCKET_ERROR)
		{
			RealTimeDataSend();
		}
		Sleep(10);
	}
	m_hRealTimeSend = NULL;
	return m_dwRealTimeSend = 0;
}

void CTcpService::OnRecvMessage(char *pszMsg, int nLen)
{
	CString strText;
	CString strLog;
	//strText.Format(" [%04X]  \n [%04X] \n [%04X] \n [%04X] [%04X] [%04X] [%04X] [%04X] [%04X] [%04X] [%04X] [%04X] [%04X] \n [%04X] [%04X] [%04X] [%04X] [%04X] [%04X] [%04X] [%04X] [%04X] [%04X] \n [%04X] [%/04X] /[%04X] [%04X] \n ",
	//	pszMsg[0], pszMsg[1], pszMsg[2], pszMsg[3], pszMsg[4], pszMsg[5], pszMsg[6], pszMsg[7], pszMsg[8], pszMsg[9], pszMsg[10], pszMsg[11], pszMsg[12], pszMsg[13], pszMsg[14], pszMsg[15], pszMsg[16]
	//	, pszMsg[17], pszMsg[18], pszMsg[19], pszMsg[20], pszMsg[21], pszMsg[22], pszMsg[23], pszMsg[24], pszMsg[25], pszMsg[26]);
	//OutputDebugString(strText);

	unsigned int nHeadSize = 0;
	ST_VDS_HEAD stVdsHead;
	CopyMemory(&stVdsHead.strVer, pszMsg, sizeof(stVdsHead.strVer));
	nHeadSize += sizeof(stVdsHead.strVer);
	CopyMemory(&stVdsHead.strType, pszMsg + nHeadSize, sizeof(stVdsHead.strVer));
	nHeadSize += sizeof(stVdsHead.strType);
	CopyMemory(&stVdsHead.strOpCode, pszMsg + nHeadSize, sizeof(stVdsHead.strOpCode));
	nHeadSize += sizeof(stVdsHead.strOpCode);
	CopyMemory(&stVdsHead.strTranscation, pszMsg + nHeadSize, sizeof(stVdsHead.strTranscation));
	nHeadSize += sizeof(stVdsHead.strTranscation);
	CopyMemory(&stVdsHead.strDataSize, pszMsg + nHeadSize, sizeof(stVdsHead.strDataSize));
	nHeadSize += sizeof(stVdsHead.strDataSize);

	// Data Size 500(DEC) > 1F4(HEX) 0x00 0x00 0x01 0xF4 이거 처리 어떻게 할것인가?
	CString strSize;
	strSize.Format("%X%X%X%X", stVdsHead.strDataSize[0], stVdsHead.strDataSize[1], stVdsHead.strDataSize[2], stVdsHead.strDataSize[3]);

	UINT nDataSize = (int)strtol(strSize, NULL, 16);

	if (nDataSize > 0)
	{
		char * pStrData = new char[nDataSize];
		ZeroMemory(pStrData, nDataSize);

		memcpy(pStrData, pszMsg + nHeadSize, nDataSize);

		JsonMsg *pjm = new JsonMsg(nDataSize, pStrData);

		switch (stVdsHead.strOpCode)
		{
		case VDS_CONNECT:
		{
			if (pjm != NULL && pjm->nLen > 0 && pjm->strJson != NULL)
			{
				CString strData = CA2W(pjm->strJson);
				CString strControlId;

				Document dc;
				if (dc.ParseInsitu((char *)pjm->strJson).HasParseError())
				{
					OutputDebugString(_T("parse error\n"));
					strLog.Format(_T("[ERROR][%d] [VDS_CONNECT] parse error"), nLen);
					WriteLog(strLog);
					delete[] pStrData;
					pStrData = NULL;
					delete pjm;
					pjm = NULL;
					return;
				}

				if (dc["vdsControllerId"].IsString())
				{
					strControlId = (CString)dc["vdsControllerId"].GetString();
				}
				strLog.Format(_T("[RECV][%d] [VDS_CONNECT] ID : [%s]"), nLen, strControlId.GetBuffer());
				WriteLog(strLog);

				SendMsg(stVdsHead, strControlId.GetBuffer(), VDS_CONNECT);
				m_bConnect = TRUE;
				StartThreadRealTimeDataSend();

			}

			break;
		}
		case VDS_REALTIME_DATA:
		{
			break;
		}
		case VDS_NOT_SENT_DATA:
		{
			if (pjm != NULL && pjm->nLen > 0 && pjm->strJson != NULL)
			{
				CString strData = CA2W(pjm->strJson);
				CString strStartDateTime;
				CString strEndDateTime;
				UINT nPageSize = 255;
				UINT nPageIndex = 1;
				UINT nTotalSize = 0;

				UINT nResultCode = DEF_SUCCESS_RESULT_CODE;
				CString strResultMsg = "SUCCESS";

				Document dc;
				if (dc.ParseInsitu((char *)pjm->strJson).HasParseError())
				{
					OutputDebugString(_T("parse error\n"));
					strLog.Format(_T("[ERROR][%d] [VDS_NOT_SENT_DATA] parse error"), nLen);
					WriteLog(strLog);
					delete[] pStrData;
					pStrData = NULL;
					delete pjm;
					pjm = NULL;
					return;
				}

				if (dc["startDateTime"].IsString())
				{
					strStartDateTime = (CString)dc["startDateTime"].GetString();
				}

				if (dc["endDateTime"].IsString())
				{
					strEndDateTime = (CString)dc["endDateTime"].GetString();
				}

				if (dc["pageSize"].IsInt())
				{
					nPageSize = (UINT)dc["pageSize"].GetInt();
				}

				if (dc["pageIndex"].IsInt())
				{
					nPageIndex = (UINT)dc["pageIndex"].GetInt();
				}

				if (strStartDateTime.GetLength() <= 0 || strEndDateTime.GetLength() <= 0)
				{
					delete[] pStrData;
					pStrData = NULL;
					delete pjm;
					pjm = NULL;
					return;
				}
				strLog.Format(_T("[RECV][%d] [VDS_NOT_SENT_DATA] startDateTime : [%s], endDateTime : [%s], pageSize : [%d], pageIndex : [%d] "), nLen, strStartDateTime, strEndDateTime, nPageSize, nPageIndex);
				WriteLog(strLog);

				vector<CString> vecFileTimeName;

				nTotalSize = GetTotalNotSendDataCnt(strStartDateTime, strEndDateTime, &vecFileTimeName, &nResultCode, &strResultMsg);

				NotSendMsg(strStartDateTime, strEndDateTime, nPageSize, nPageIndex, nTotalSize, &vecFileTimeName, &nResultCode, &strResultMsg);
			}

			break;
		}
		case VDS_HEART_BEART:
		{
			//	OutputDebugString("VDS_HEART_BEART\n");
			if (pjm != NULL && pjm->nLen > 0 && pjm->strJson != NULL)
			{
				CString strData = CA2W(pjm->strJson);
				CString strControlId;
				CString strHeartBeatTime;

				Document dc;
				if (dc.ParseInsitu((char *)pjm->strJson).HasParseError())
				{
					OutputDebugString(_T("parse error\n"));
					strLog.Format(_T("[ERROR][%d] [VDS_HEART_BEART] parse error"), nLen);
					WriteLog(strLog);
					delete[] pStrData;
					pStrData = NULL;
					delete pjm;
					pjm = NULL;
					return;
				}

				if (dc["vdsControllerId"].IsString())
				{
					strControlId = (CString)dc["vdsControllerId"].GetString();
				}

				if (dc["heartBeatTime"].IsString())
				{
					strHeartBeatTime = (CString)dc["heartBeatTime"].GetString();
				}
				strLog.Format(_T("[RECV][%d] [VDS_HEART_BEART] vdsControllerId : [%s], heartBeatTime : [%s] "), nLen, strControlId, strHeartBeatTime.GetBuffer());
				WriteLog(strLog);

				SendMsg(stVdsHead, strControlId.GetBuffer(), VDS_HEART_BEART);

			}

			break;
		}
		default:
		{
			OutputDebugString("Oops OpCode\n");
			strLog.Format(_T("[ERROR][%d] opcode error"), nLen);
			WriteLog(strLog);
			break;
		}

		}

		delete[] pStrData;
		pStrData = NULL;
		delete pjm;
		pjm = NULL;
	}
}

void CTcpService::SendMsg(ST_VDS_HEAD stVdsHead, std::string strControlId, unsigned char szOpCode)
{
	//OutputDebugString("SendHeartBeat\n");
	CString strLog;
	char strHead[3] = {};
	char strTId[20] = DEF_TRANSCATION_ID;

	switch (szOpCode)
	{
	case VDS_HEART_BEART:
	{
		strHead[0] = 0x01;
		strHead[1] = 0x02;
		strHead[2] = 0x30;

		CString strTime;

		SYSTEMTIME  tCurTime;
		::GetLocalTime(&tCurTime);
		
		strTime.Format(_T("%04d-%02d-%02d %02d:%02d:%02d.%02d"), (int)tCurTime.wYear, (int)tCurTime.wMonth,
			(int)tCurTime.wDay, (int)tCurTime.wHour, (int)tCurTime.wMinute, (int)tCurTime.wSecond,
			(int)(tCurTime.wMilliseconds/10));

		CString strJson;

		UINT nResultCode = 100;
		CString strMsg = "SUCCESS";

		strJson.Format(
			_T("{\"resultCode\":\%d,"
				"\"resultMessage\":\"%s\","
				"\"vdsControllerId\":\"%s\","
				"\"heartBeatTime\":\"%s\"}"
			)
			, nResultCode
			, strMsg
			, strControlId
			, strTime.GetBuffer()
		);

		strJson.Replace("\\", "\\\\");

		string szJson = strJson.GetBuffer();
		UINT nJsonLen = (UINT)szJson.length(); //67 

	/*	CString strText;
		strText.Format(_T("nJsonLen : [%d]\n"), nJsonLen);
		OutputDebugString(strText);*/
		CString strJsonLen;
		strJsonLen.Format(_T("%08X"), nJsonLen); //67 > 43, 999 > 3E7

		unsigned char *pSz = { 0 };
		int nByteArraySize = 0;
		StringToBytearray(strJsonLen.GetBuffer(), pSz, nByteArraySize);

		UINT nTotalLen = sizeof(strHead) + sizeof(strTId) + nByteArraySize + nJsonLen;
		char * pStrMsg = (char*)new char[nTotalLen];
		ZeroMemory(pStrMsg, nTotalLen);

		UINT nDataSize = 0;
		memcpy(pStrMsg, (char *)strHead, sizeof(strHead));
		nDataSize += sizeof(strHead);
		memcpy(pStrMsg + nDataSize, (char *)strTId, sizeof(strTId));
		nDataSize += sizeof(strTId);
#if 0
		memcpy(pStrMsg + nDataSize, (char *)strDataSize, nByteArraySize);
#else
		memcpy(pStrMsg + nDataSize, (char *)pSz, nByteArraySize);
#endif
		nDataSize += nByteArraySize;
		memcpy(pStrMsg + nDataSize, szJson.c_str(), nJsonLen);

		int nSent = nTotalLen;
		int nMessage = 0;

		//strText.Format(" send packet \n [%04X] \n [%04X] \n [%04X] \n [%04X] [%04X] [%04X] [%04X] [%04X] \n [%04X] [%04X] [%04X] [%04X] [%04X] \n [%04X] [%04X] [%04X] [%04X] [%04X] \n [%04X] [%04X] [%04X] [%04X] [%04X] \n [%04X] [%04X] [%04X] [%04X]\n",
		//	pStrMsg[0], pStrMsg[1], pStrMsg[2], pStrMsg[3], pStrMsg[4], pStrMsg[5], pStrMsg[6], pStrMsg[7], pStrMsg[8], pStrMsg[9], pStrMsg[10], pStrMsg[11], pStrMsg[12], pStrMsg[13], pStrMsg[14], pStrMsg[15], pStrMsg[16]
		//	, pStrMsg[17], pStrMsg[18], pStrMsg[19], pStrMsg[20], pStrMsg[21], pStrMsg[22], pStrMsg[23], pStrMsg[24], pStrMsg[25], pStrMsg[26]);
		//OutputDebugString(strText);
		//WriteLog(strText);
		char * pMsg = pStrMsg;
		while (1)
		{
			//nMessage = send(m_sock, pStrMsg, nSent, 0);
			nMessage = send(m_sock, pMsg, nSent, 0);
			strLog.Format(_T("[SEND][%d] [VDS_HEART_BEART] vdsControllerId : [%s], heartBeatTime : [%s]"), nMessage, strControlId, strTime.GetBuffer());
			WriteLog(strLog);

			if (nMessage == -1)
			{
				m_bConnect = FALSE;
				break;
			}
			nSent -= nMessage;
			pMsg += nMessage;

			if (nSent <= 0)
			{
				break;
			}

		}

		delete[] pStrMsg;
		break;
	}
	case VDS_CONNECT:
	{
		strHead[0] = 0x01;
		strHead[1] = 0x02;
		strHead[2] = 0x10;

		CString strJson;

		UINT nResultCode = 100;
		CString strMsg = "SUCCESS";

		TCP_CONFIG tp;
		tp = GetTcpConfig();

		CString strUrl = tp.strSourceURL;
		strUrl.Replace("192.168.0.100", "223.171.32.245");
		CString strDetectUrl = tp.strDetectionUrl;
		strDetectUrl.Replace("192.168.0.101", "223.171.32.245");
		strJson.Format(
			_T("{\"resultCode\":%d,"
				"\"resultMessage\":\"%s\","
				"\"rtspSourceURL\":\"%s\","
				"\"rtspDetectionURL\":\"%s\"}"
			)
			, nResultCode
			, strMsg
			, strUrl
			, strDetectUrl
		);

		strJson.Replace("\\", "\\\\");

		string szJson = strJson.GetBuffer();
		UINT nJsonLen = (UINT)szJson.length(); //93

		//CString strText;
		//strText.Format(_T("nJsonLen : [%d]\n"), nJsonLen);
		//OutputDebugString(strText);

		CString strJsonLen;
		strJsonLen.Format(_T("%08X"), nJsonLen); //67 > 43, 999 > 3E7

#if 0
		char strDataSize[4] = { 0x00, 0x00, 0x00, 0x5D };

		UINT nTotalLen = sizeof(strHead) + sizeof(strTId) + sizeof(strDataSize) + nJsonLen;
#endif
		unsigned char *pSz = { 0 };
		int nByteArraySize = 0;
		StringToBytearray(strJsonLen.GetBuffer(), pSz, nByteArraySize);

		UINT nTotalLen = sizeof(strHead) + sizeof(strTId) + nByteArraySize + nJsonLen;

		char * pStrMsg = (char*)new char[nTotalLen];
		ZeroMemory(pStrMsg, nTotalLen);

		UINT nDataSize = 0;
		memcpy(pStrMsg, (char *)strHead, sizeof(strHead));
		nDataSize += sizeof(strHead);
		memcpy(pStrMsg + nDataSize, (char *)strTId, sizeof(strTId));
		nDataSize += sizeof(strTId);
		memcpy(pStrMsg + nDataSize, (char *)pSz, nByteArraySize);
		nDataSize += nByteArraySize;
		memcpy(pStrMsg + nDataSize, szJson.c_str(), nJsonLen);

		int nSent = nTotalLen;
		int nMessage = 0;

	/*	strText.Format(" send packet \n [%04X] \n [%04X] \n [%04X] \n [%04X] [%04X] [%04X] [%04X] [%04X] \n [%04X] [%04X] [%04X] [%04X] [%04X] \n [%04X] [%04X] [%04X] [%04X] [%04X] \n [%04X] [%04X] [%04X] [%04X] [%04X] \n [%04X] [%04X] [%04X] [%04X]\n",
			pStrMsg[0], pStrMsg[1], pStrMsg[2], pStrMsg[3], pStrMsg[4], pStrMsg[5], pStrMsg[6], pStrMsg[7], pStrMsg[8], pStrMsg[9], pStrMsg[10], pStrMsg[11], pStrMsg[12], pStrMsg[13], pStrMsg[14], pStrMsg[15], pStrMsg[16]
			, pStrMsg[17], pStrMsg[18], pStrMsg[19], pStrMsg[20], pStrMsg[21], pStrMsg[22], pStrMsg[23], pStrMsg[24], pStrMsg[25], pStrMsg[26]);
		OutputDebugString(strText);
		WriteLog(strText);*/
		char *pMsg = pStrMsg;
		while (1)
		{
			nMessage = send(m_sock, pMsg, nSent, 0);

			strLog.Format(_T("[SEND][%d] [VDS_CONNECT]"), nMessage);
			WriteLog(strLog);

			if (nMessage == -1)
			{
				m_bConnect = FALSE;
				break;
			}
			nSent -= nMessage;
			pMsg += nMessage;
			if (nSent <= 0)
			{
				break;
			}

		}
		delete[] pStrMsg;
		break;
	}
	case VDS_REALTIME_DATA:
	{
		break;
	}
	case VDS_NOT_SENT_DATA:
	{
		break;
	}
	default:
		break;
	}
}

void CTcpService::NotSendMsg(CString strST, CString strET, UINT nPageSize, UINT nPageIndex, UINT nTotalSize, vector<CString> *pvecFileTimeName, UINT *pnResultCode, CString *pstrResultMsg)
{
	CString strLog;
	char strHead[3] = {};
	char strTId[20] = DEF_TRANSCATION_ID;

	strHead[0] = 0x01;
	strHead[1] = 0x02;
	strHead[2] = 0x21;

	CString strJson;
	UINT nSendSize = 0;
	strJson = GetNotSendData(strST, strET, nPageSize, nPageIndex, nTotalSize, pvecFileTimeName, pnResultCode, pstrResultMsg, &nSendSize);

	strJson.Replace("\\", "\\\\");

	string szJson = strJson.GetBuffer();
	UINT nJsonLen = (UINT)szJson.length();
	CString str;
	str.Format(_T("nJsonLen = [%d]\n"), nJsonLen);
	OutputDebugString(str);
	CString strJsonLen;
	strJsonLen.Format(_T("%08X"), nJsonLen);
	OutputDebugString(strJsonLen);
	OutputDebugString("\n");

	unsigned char *pSz = { 0 };
	int nByteArraySize = 0;
	StringToBytearray(strJsonLen.GetBuffer(), pSz, nByteArraySize);

	str.Format(_T("[%04X] [%04X] [%04X] [%04X]\n"), pSz[0], pSz[1], pSz[2], pSz[3]);
	OutputDebugString(str);

	UINT nTotalLen = sizeof(strHead) + sizeof(strTId) + nByteArraySize + nJsonLen;

	char * pStrMsg = (char*)new char[nTotalLen];
	ZeroMemory(pStrMsg, nTotalLen);

	UINT nDataSize = 0;
	memcpy(pStrMsg, (char *)strHead, sizeof(strHead));
	nDataSize += sizeof(strHead);
	memcpy(pStrMsg + nDataSize, (char *)strTId, sizeof(strTId));
	nDataSize += sizeof(strTId);
	memcpy(pStrMsg + nDataSize, (char *)pSz, nByteArraySize);
	nDataSize += nByteArraySize;
	memcpy(pStrMsg + nDataSize, szJson.c_str(), nJsonLen);

	int nSent = nTotalLen;
	int nMessage = 0;

	char * pMsg = pStrMsg;
	while (1)
	{
		nMessage = send(m_sock, pMsg, nSent, 0);
		//CString strText;
		//strText.Format(_T("Send [%d]\n"), nMessage);
		//OutputDebugString(strText);

		strLog.Format(_T("[SEND][%d] [VDS_NOT_SENT_DATA] resultCode : [%d], resultMsg : [%s], startDateTime : [%s], endDateTime : [%s], pageSize : [%d], pageIndex : [%d], TotalSize : [%d] SendDataSize: [%d] "), nMessage, *pnResultCode, *pstrResultMsg, strST.GetBuffer(), strET.GetBuffer(), nPageSize, nPageIndex, nTotalSize, nSendSize);
		WriteLog(strLog);
		if (nMessage == -1)
		{
			m_bConnect = FALSE;
			break;
		}
		nSent -= nMessage;
		pMsg += nMessage;
		if (nSent <= 0)
		{
			break;
		}

	}
	delete[] pStrMsg;
}

CString CTcpService::GetRealTimeDataJson(CString *pstrID, CString *pstrDetectTime)
{
	CString strJson;
	CVdsData *pVD = NULL;

	EnterCriticalSection(&m_csSendVdsData);
	if (m_QSendVdsData.empty() == FALSE)
	{
		pVD = m_QSendVdsData.front();
		m_QSendVdsData.pop();
	}

	LeaveCriticalSection(&m_csSendVdsData);

	if (pVD != NULL)
	{
		UINT nTotalCnt = 1;

		strJson.Format(
			_T("{\"totalCount\":%d,"
				"\"trafficDataList\": [ {"
				"\"id\":\"%s\","
				"\"lane\":%d,"
				"\"direction\":%d,"
				"\"length\":%d,"
				"\"speed\":%f,"
				"\"class\":%d,"
				"\"occupyTime\":%d,"
				"\"loop1OccupyTime\":%d,"
				"\"loop2OccupyTime\":%d,"
				"\"reverseRunYN\":\"%s\","
				"\"vehicleGap\":%d,"
				"\"detectTime\":\"%s\""
				"}]}"
			)
			, nTotalCnt
			, pVD->strId.c_str()
			, pVD->nLane
			, pVD->nDirection
			, pVD->nLength
			, pVD->dSpeed
			, pVD->nClass
			, pVD->nOccupyTime
			, pVD->nLoop1OccupyTime
			, pVD->nLoop2OccupyTime
			, pVD->strReverseRun
			, pVD->nVehicleGap
			, pVD->strDetectTime.c_str()
		);
		*pstrID = pVD->strId.c_str();
		*pstrDetectTime = pVD->strDetectTime.c_str();
		delete pVD;
		pVD = NULL;
	}

	strJson.Replace("\\", "\\\\");
	return strJson;
}

void CTcpService::RealTimeDataSend()
{
	CString strLog;
	char strHead[3] = {};
	char strTId[20] = "VDS Server";

	strHead[0] = 0x01;
	strHead[1] = 0x02;
	strHead[2] = 0x20; //VDS_REALTIME_DATA
	CString strDetectTime;
	CString strId;
	CString strJson = GetRealTimeDataJson(&strId, &strDetectTime);
	string szJson = strJson.GetBuffer();
	UINT nJsonLen = (UINT)szJson.length(); //223

	if (nJsonLen <= 0)
		return;

	CString strJsonLen;
	strJsonLen.Format(_T("%08X"), nJsonLen); //67 > 43, 999 > 3E7

	unsigned char *pSz = { 0 };
	int nByteArraySize = 0;
	StringToBytearray(strJsonLen.GetBuffer(), pSz, nByteArraySize);

	UINT nTotalLen = sizeof(strHead) + sizeof(strTId) + nByteArraySize + nJsonLen;

	char * pStrMsg = (char*)new char[nTotalLen];
	ZeroMemory(pStrMsg, nTotalLen);

	UINT nDataSize = 0;
	memcpy(pStrMsg, (char *)strHead, sizeof(strHead));
	nDataSize += sizeof(strHead);
	memcpy(pStrMsg + nDataSize, (char *)strTId, sizeof(strTId));
	nDataSize += sizeof(strTId);
	memcpy(pStrMsg + nDataSize, (char *)pSz, nByteArraySize);
	nDataSize += nByteArraySize;
	memcpy(pStrMsg + nDataSize, szJson.c_str(), nJsonLen);

	int nSent = nTotalLen;
	int nMessage = 0;

	//CString strText;

	//strText.Format(" [%04X] [%04X] [%04X] [%04X] [%04X] [%04X] [%04X] [%04X] \n [%04X] [%04X] [%04X] [%04X] [%04X] [%04X] [%04X] [%04X] \n [%04X] [%04X] [%04X] [%04X] [%04X] [%04X] [%04X] [%04X] \n",
	//	pStrMsg[0], pStrMsg[1], pStrMsg[2], pStrMsg[3], pStrMsg[4], pStrMsg[5], pStrMsg[6], pStrMsg[7], pStrMsg[8], pStrMsg[9], pStrMsg[10], pStrMsg[11], pStrMsg[12], pStrMsg[13], pStrMsg[14], pStrMsg[15], pStrMsg[16]
	//	, pStrMsg[17], pStrMsg[18], pStrMsg[19], pStrMsg[20], pStrMsg[21], pStrMsg[22], pStrMsg[23]);
	//OutputDebugString(strText);
	char *pMsg = pStrMsg;
	while (1)
	{
		nMessage = send(m_sock, pMsg, nSent, 0);
		//CString strText;
		//strText.Format(_T("Send [%d]\n"), nMessage);
		//OutputDebugString(strText);
		strLog.Format(_T("[SEND][%d] [VDS_REALTIME_DATA] id : [%s], detectTime: [%s]"), nMessage, strId, strDetectTime);
		WriteLog(strLog);
		if (nMessage == -1)
		{
			m_bConnect = FALSE;
			break;
		}
		nSent -= nMessage;
		pMsg += nMessage;
		if (nSent <= 0)
		{
			break;
		}

	}

	delete[] pStrMsg;
}

void CTcpService::StringToBytearray(std::string str, unsigned char* &array, int &size)
{
	int length = (int)str.length();

	if (length % 2 == 1)
	{
		str = "0" + str;
		length++;
	}

	array = new unsigned char[length / 2];
	size = length / 2;

	std::stringstream sstr(str);
	for (int i = 0; i < size; i++)
	{
		char ch1, ch2;
		sstr >> ch1 >> ch2;

		int dig1, dig2;
		if (isdigit(ch1))
			dig1 = ch1 - '0';
		else if (ch1 >= 'A' && ch1 <= 'F')
			dig1 = ch1 - 'A' + 10;
		else if (ch1 >= 'a' && ch1 <= 'f')
			dig1 = ch1 - 'a' + 10;

		if (isdigit(ch2))
			dig2 = ch2 - '0';
		else if (ch2 >= 'A' && ch2 <= 'F')
			dig2 = ch2 - 'A' + 10;
		else if (ch2 >= 'a' && ch2 <= 'f')
			dig2 = ch2 - 'a' + 10;

		array[i] = dig1 * 16 + dig2;
	}
}

CString CTcpService::GetNotSendData(CString strSt, CString strEt, UINT nPageSize, UINT nPageIndex, UINT nTotalSize, vector<CString> *pvecFileTimeName, UINT* pnResultCode, CString* pstrResultMsg, UINT *npSendSize)
{
	UINT nStartIndex = (nPageSize * (nPageIndex - 1)) + 1;
	UINT nEndIndex = nPageSize * nPageIndex;
	UINT nCurrentIndex = 0;
	CString strJson;

	if (nTotalSize > 0)
	{
		UINT nIndexCheck = ceil(float((float)nTotalSize / (float)nPageSize));
		if (nIndexCheck < nPageIndex)
		{
			*pnResultCode = DEF_ERROR_RESULT_CODE;
			*pstrResultMsg = "Invalid page index";
		}
	}

	char currentPath[MAX_PATH], makePath[MAX_PATH];
	sprintf(currentPath, "%s", getModulPath());
	wsprintf(makePath, "%s\\VDS_data\\sendfail", currentPath);

	vector<string> vecData;

	for (auto itr = pvecFileTimeName->begin(); itr != pvecFileTimeName->end(); itr++)
	{
		if (nCurrentIndex > nEndIndex)
			break;

		CString szOut = *itr;
		char fileName[MAX_PATH];
		ZeroMemory(fileName, MAX_PATH);
		wsprintf(fileName, "%s\\Log_%s.csv", makePath, szOut.GetBuffer());
		try
		{
			ifstream fin(fileName);
			string szLine;

			if (fin.is_open())
			{
				while (getline(fin, szLine))
				{
					nCurrentIndex++;
					// Cur : 1 -> 256~610
					if (nCurrentIndex >= nStartIndex && nCurrentIndex <= nEndIndex)
						vecData.push_back(szLine);
				}

				fin.close();
			}

		}
		catch (const std::exception& e)
		{
			CString strError;
			strError.Format(_T("error : %s\n"), e.what());
			OutputDebugString(strError);
		}
	}

	strJson.Format(
		_T("{\"resultCode\":%d,"
			"\"resultMessage\":\"%s\","
			"\"startDateTime\":\"%s\","
			"\"endDateTime\":\"%s\","
			"\"pageSize\":%d,"
			"\"pageIndex\":%d,"
			"\"totalCount\":%d,"
			"\"trafficDataList\":["
		)
		, *pnResultCode
		, *pstrResultMsg
		, strSt
		, strEt
		, nPageSize
		, nPageIndex
		, nTotalSize
	);

	if (nTotalSize > 0 && *pnResultCode == DEF_SUCCESS_RESULT_CODE)
	{

		CString strTD;
		UINT nCount = 0;
		for (auto itr = vecData.begin(); itr != vecData.end(); itr++)
		{
			string szLenData = *itr;
			stringstream ss(szLenData);
			string szValue[MAX_FILED_CNT];
			int nNum = 0;
			while (getline(ss, szValue[nNum], ','))
			{
				nNum++;
			}

			strTD.Format(
				_T("{\"id\":\"%s\","
					"\"lane\":%s,"
					"\"direction\":%s,"
					"\"length\":%s,"
					"\"speed\":%s,"
					"\"class\":%s,"
					"\"occupyTime\":%d,"
					"\"loop1OccupyTime\":%s,"
					"\"loop2OccupyTime\":%s,"
					"\"reverseRunYN\":\"%s\","
					"\"vehicleGap\":%d,"
					"\"detectTime\":\"%s\""
					"}"
				)
				, szValue[VF_ID]
				, szValue[VF_LANE]
				, szValue[VF_DIRECTION]
				, szValue[VF_LENGTH]
				, szValue[VF_SPEED]
				, szValue[VF_CLASS]
				, stoi(szValue[VF_OCCUPTY_TIME])
				, szValue[VF_LOOP1_OCCPY_TIME]
				, szValue[VF_LOOP2_OCCPY_TIME]
				, szValue[VF_REVERSE]
				, stoi(szValue[VF_VEHICLE_GAP])
				, szValue[VF_DETECT_TIME].c_str()
			);
			//OutputDebugString(strTD);
			//OutputDebugString("\n");

			nCount++;
			if (strTD.GetLength() > 0)
			{
				strJson += strTD;
			}
			if (nCount < vecData.size())
			{
				strJson += " , ";
			}
		}
	}

	strJson += "]}";
	//	OutputDebugString(strJson);
	//	OutputDebugString("\n");
	*npSendSize = vecData.size();
	return strJson;
}

UINT CTcpService::GetTotalNotSendDataCnt(CString strSt, CString strEt, vector<CString> *pvecFileTimeName, UINT* pnResultCode, CString *pstrResultMsg)
{
	CString strJson;
	UINT nTotal = 0;

	SYSTEMTIME sTime;
	ZeroMemory(&sTime, sizeof(SYSTEMTIME));
	sscanf_s(strSt, "%04hd-%02hd-%02hd %02hd:%02hd:%02hd.%03hd",
		&sTime.wYear,
		&sTime.wMonth,
		&sTime.wDay,
		&sTime.wHour,
		&sTime.wMinute,
		&sTime.wSecond,
		&sTime.wMilliseconds);

	SYSTEMTIME eTime;
	ZeroMemory(&eTime, sizeof(SYSTEMTIME));
	sscanf_s(strEt, "%04hd-%02hd-%02hd %02hd:%02hd:%02hd.%03hd",
		&eTime.wYear,
		&eTime.wMonth,
		&eTime.wDay,
		&eTime.wHour,
		&eTime.wMinute,
		&eTime.wSecond,
		&eTime.wMilliseconds);

	if (CompareSystemTime(sTime, eTime) < 0)
	{
		*pnResultCode = DEF_ERROR_RESULT_CODE;
		*pstrResultMsg = "Start time is greater than End time";

		OutputDebugString(_T("StartTime > EndTime \n"));

		return nTotal;
	}

	GetFileTimeName(sTime, eTime, pvecFileTimeName);

	ULONGLONG nTick = GetTickCount64();

	nTotal = GetFileTotalLine(pvecFileTimeName);

	return nTotal;
}

UINT CTcpService::GetFileTotalLine(vector<CString> *pvecFileTimeName)
{

	UINT nTotalLine = 0;

	char currentPath[MAX_PATH], makePath[MAX_PATH];
	sprintf(currentPath, "%s", getModulPath());
	wsprintf(makePath, "%s\\VDS_data\\sendfail", currentPath);

	for (auto itr = pvecFileTimeName->begin(); itr != pvecFileTimeName->end(); itr++)
	{
		CString szOut = *itr;
		char fileName[MAX_PATH];
		ZeroMemory(fileName, MAX_PATH);
		wsprintf(fileName, "%s\\Log_%s.csv", makePath, szOut.GetBuffer());

		try
		{
			FILE *fp;
			size_t size;
			char buf[4096];
			int nLine = 0;

			if (fopen_s(&fp, fileName, "r") == 0)
			{
				while (!feof(fp))
				{
					size = fread(buf, 1, sizeof(buf), fp);
					nLine += CountNL(buf, buf + size);
				}

				fclose(fp);
			}
			nTotalLine += nLine;
		}
		catch (const std::exception& e)
		{
			CString strError;
			strError.Format(_T("error : %s\n"), e.what());
			OutputDebugString(strError);
		}

	}

	return nTotalLine;
}

char* CTcpService::CStToChar(CString strParam)
{
	CStringA cstraParam(strParam);
	size_t len = cstraParam.GetLength() + 1;
	char *nCharStr = new char[len];
	strcpy_s(nCharStr, len, cstraParam);
	return nCharStr;
}

UINT CTcpService::CountNL(char *pszIn, char *pszEnd)
{
	UINT nCnt = 0;
	for (; pszIn != pszEnd; ++pszIn)
	{
		if (*pszIn == '\n')
			nCnt++;
	}
	return nCnt;
}

double CTcpService::CompareSystemTime(SYSTEMTIME stStart, SYSTEMTIME stEnd)
{
	double dResult = 0.0;

	tm tmSt, tmEt;
	time_t timeSt, timeEt;

	tmSt.tm_sec = stStart.wSecond;
	tmSt.tm_min = stStart.wMinute;
	tmSt.tm_hour = stStart.wHour;
	tmSt.tm_mday = stStart.wDay;
	tmSt.tm_mon = stStart.wMonth - 1;
	tmSt.tm_year = stStart.wYear - 1900;
	tmSt.tm_isdst = 0;
	timeSt = ::mktime(&tmSt);

	tmEt.tm_sec = stEnd.wSecond;
	tmEt.tm_min = stEnd.wMinute;
	tmEt.tm_hour = stEnd.wHour;
	tmEt.tm_mday = stEnd.wDay;
	tmEt.tm_mon = stEnd.wMonth - 1;
	tmEt.tm_year = stEnd.wYear - 1900;
	tmEt.tm_isdst = 0;
	timeEt = ::mktime(&tmEt);

	dResult = difftime(timeEt, timeSt);
	return dResult;
}

void CTcpService::GetFileTimeName(SYSTEMTIME stStart, SYSTEMTIME stEnd, vector<CString> *pvecFileTimeName)
{
	double dResult = 0.0;
	UINT nDifHour = 0;

	tm tmSt, tmEt;
	time_t timeSt, timeEt;

	//tmSt.tm_sec = stStart.wSecond;
	tmSt.tm_sec = 0;
	//tmSt.tm_min = stStart.wMinute;
	tmSt.tm_min = 0;
	tmSt.tm_hour = stStart.wHour;
	tmSt.tm_mday = stStart.wDay;
	tmSt.tm_mon = stStart.wMonth - 1;
	tmSt.tm_year = stStart.wYear - 1900;
	tmSt.tm_isdst = 0;
	timeSt = ::mktime(&tmSt);

	//tmEt.tm_sec = stEnd.wSecond;
	tmEt.tm_sec = 0;
	//tmEt.tm_min = stEnd.wMinute;
	tmEt.tm_min = 0;
	tmEt.tm_hour = stEnd.wHour;
	tmEt.tm_mday = stEnd.wDay;
	tmEt.tm_mon = stEnd.wMonth - 1;
	tmEt.tm_year = stEnd.wYear - 1900;
	tmEt.tm_isdst = 0;
	timeEt = ::mktime(&tmEt);

	//2021-06-15  11:45:44   > 2021-06-15 11:00:00
	//2021-06-15  12:15:44   > 2021-06-15 12:00:00
	//1시간 미만이면, 0

	dResult = difftime(timeEt, timeSt);
	nDifHour = (UINT)dResult / (60 * 60);

	CString strFileTime;
	strFileTime.Format(_T("%04d%02d%02d%02d"), tmSt.tm_year + 1900, tmSt.tm_mon + 1, tmSt.tm_mday, tmSt.tm_hour);
	pvecFileTimeName->push_back(strFileTime);

	if (nDifHour >= 0)
	{
		for (int i = 0; i < nDifHour; i++)
		{
			tmSt.tm_hour += 1;
			timeSt = ::mktime(&tmSt);

			strFileTime.Format(_T("%04d%02d%02d%02d"), tmSt.tm_year + 1900, tmSt.tm_mon + 1, tmSt.tm_mday, tmSt.tm_hour);
			pvecFileTimeName->push_back(strFileTime);
		}
	}
}

void CTcpService::WriteLog(CString strLog)
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
	wsprintf(fileName, "%s\\VDS_TCP_%04d%02d%02d.log", todayfolder, (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay, (int)tCurTime.wHour);

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

TCP_CONFIG CTcpService::GetTcpConfig()
{
	TCP_CONFIG tp;

	CTcpServer *pTs = (CTcpServer *)m_pParent;

	if (pTs)
	{
		tp = pTs->GetConfig();
	}
	return tp;
}