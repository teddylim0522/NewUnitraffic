#include "stdafx.h"
#include "RadarManager.h"



CRadarManager::CRadarManager()
{
	InitializeCriticalSection(m_CS_Server);
	InitializeCriticalSection(m_CS_Receive);

	m_FD_SIZE_COUNT = -1;
	m_ReadFD.reserve(FD_SIZE_VECTOR);
	m_ExceptFD.reserve(FD_SIZE_VECTOR);
	m_WriteFD.reserve(FD_SIZE_VECTOR);

	for (int i = 0; i<FD_SIZE_VECTOR; i++)
	{
		fd_set ReadSet, WriteSet, ExceptSet;

		FD_ZERO(&ReadSet);
		FD_ZERO(&WriteSet);
		FD_ZERO(&ExceptSet);

		m_ReadFD.push_back(ReadSet);
		m_ExceptFD.push_back(ExceptSet);
		m_WriteFD.push_back(WriteSet);
	}

	for (int idx = 0; idx < MAX_CLIENTS; idx++)
	{
		client_socket[idx] = 0;
	}
}

CRadarManager::~CRadarManager()
{
	DeleteCriticalSection(m_CS_Server);
	DeleteCriticalSection(m_CS_Receive);

	while (m_PtrListData.GetCount())
	{
		PRADAR_DATA pData = (PRADAR_DATA)ReadPtrList();

		if (pData)
		{
			if (pData->pByData)
			{
				delete[] pData->pByData;
				pData->pByData = NULL;
			}

			delete[] pData;
			pData = NULL;
		}
	}
	m_csData.Lock();
	m_PtrListData.RemoveAll();
	m_csData.Unlock();
}


void CRadarManager::RemoveAllData_ListData()
{
	while (m_PtrListData.GetCount())
	{
		PRADAR_DATA pData = (PRADAR_DATA)ReadPtrList();

		if (pData)
		{
			if (pData->pByData)
			{
				delete[] pData->pByData;
				pData->pByData = NULL;
			}

			delete[] pData;
			pData = NULL;
		}
	}
	m_csData.Lock();
	m_PtrListData.RemoveAll();
	m_csData.Unlock();
}

void CRadarManager::SetServerSockAddr(sockaddr_in *sockAddr, int PortNumber)
{
	sockAddr->sin_family = AF_INET;
	sockAddr->sin_port = htons(PortNumber);
	//sockAddr->sin_addr.S_un.S_addr = INADDR_ANY;			// Listen on all available ip's
	sockAddr->sin_addr.s_addr = INADDR_ANY;			// Listen on all available ip's
}



void CRadarManager::StartServerThread()
{
	m_hServerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hServerHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_bServerThreadFlag = TRUE;
	m_hServerThread = AfxBeginThread(OnServerThread, this, THREAD_PRIORITY_NORMAL, 0);
	//m_bServerThreadFlag = FALSE;
}


UINT CRadarManager::OnServerThread(LPVOID pThread)
{
	CRadarManager *pData = (CRadarManager*)pThread;

	pData->ListenSocket();

	return 0;
}


void CRadarManager::ListenSocket()
{
	const int on = 1;
	const int val = 1;

	sockaddr_in ServerAddress;

	//set of socket descriptors
	fd_set readfds;

	//a message
	char *message = "Radar Hi";
	

	//LPR is not connected
	RADAR_CLIENT = -1;

	//initialise all client_socket[] to 0 so not checked
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		client_socket[i] = 0;
	}


	//set master socket to allow multiple connections , this is just a good habit, it will work without this
 	if (setsockopt(MasterSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
	{
		m_statusmess = _T("error in SO_REUSEADDR");// +m_statusmess;


		::PostMessage(m_hMainWnd, m_MessageUID, 0, 0);
	}

	if (setsockopt(MasterSocket, SOL_SOCKET, SO_KEEPALIVE, (const char FAR *)&val, sizeof(val))<0)
	{
		m_statusmess = _T("error in SO_KEEPALIVE");// +m_statusmess;


		::PostMessage(m_hMainWnd, m_MessageUID, 0, 0);
	}

	//type of socket created
	SetServerSockAddr(&ServerAddress, m_nServerPort);


	//bind the socket
	if (SOCKET_ERROR == bind(MasterSocket, (struct sockaddr *) &ServerAddress, sizeof(ServerAddress)))
	{
		int nErr = WSAGetLastError();
		closesocket(MasterSocket);

		m_statusmess = _T("bind socket Error");// +m_statusmess;


		::PostMessage(m_hMainWnd, m_MessageUID, 0, 0);
		return;
	}

	//listen on the socket for clients
	if (SOCKET_ERROR == listen(MasterSocket, SOMAXCONN))
	{
		closesocket(MasterSocket);

		m_statusmess = _T("listen socket Error");// +m_statusmess;

		::PostMessage(m_hMainWnd, m_MessageUID, 0, 0);
		return;
	}
	else
	{
		m_statusmess = _T("Waiting for clients...");// +m_statusmess;
		::PostMessage(m_hMainWnd, m_MessageUID, 0, 0);
	}

	//accept the incoming connection
	addrlen = sizeof(ServerAddress);

	CString CstrAdd;

	while (1)
	{
		DWORD dwRet;
		dwRet = WaitForSingleObject(m_hServerEvent, 10);

		if (dwRet == WAIT_TIMEOUT)
		{
			//clear the socket set
			FD_ZERO(&readfds);

			//add master socket to set
			FD_SET(MasterSocket, &readfds);
			max_sd = MasterSocket;

			//add child sockets to set
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				//socket descriptor
				sd = client_socket[i];

				//if valid socket descriptor then add to read list
				if (sd > 0)
					FD_SET(sd, &readfds);

				//highest file descriptor number, need it for the select function
				if (sd > max_sd)
					max_sd = sd;
			}

			//wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
			activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

			if ((activity < 0) && (errno != EINTR))
			{
				m_statusmess = _T("Select error...");// +m_statusmess;

				::SendMessage(m_hMainWnd, m_MessageUID, 0, 0);
				return;
			}

			//If something happened on the master socket , then its an incoming connection
			if (FD_ISSET(MasterSocket, &readfds))
			{
				if ((new_socket = accept(MasterSocket, (struct sockaddr *)&ServerAddress, &addrlen))<0)
				{
					m_statusmess = _T("Accept error...");// +m_statusmess;

					::SendMessage(m_hMainWnd, m_MessageUID, 0, 0);
					return;
				}
				else
				{
					//inform user of socket number - used in send and receive commands
					CString CstrConnect;
					CstrConnect.Format(_T("New connection , socket fd is %d , IP: %s , Port: %d"), new_socket,
						(CString)inet_ntoa(ServerAddress.sin_addr), ntohs(ServerAddress.sin_port));
					m_statusmess = CstrConnect;// +m_statusmess;
					::SendMessage(m_hMainWnd, m_MessageUID, 0, 0);
					//if ((CString)inet_ntoa(ServerAddress.sin_addr) == LPRENG_IP)
					//	bLPR = TRUE;

				}

				////send new connection greeting message
				//if (send(new_socket, message, (int)strlen(message), 0) != SOCKET_ERROR)
				//{
				//	m_statusmess = _T("Welcome message sent successfully.");// +m_statusmess;
				//	::SendMessage(m_hMainWnd, m_MessageUID, 0, 0);
				//}

				//add new socket to array of sockets
				for (int i = 0; i < MAX_CLIENTS; i++)
				{
					//if position is empty
					if (client_socket[i] == 0)
					{
						client_socket[i] = new_socket;
						if ((CString)inet_ntoa(ServerAddress.sin_addr) == m_strRadarIP) RADAR_CLIENT = i;

						CString CstrAdding;
						CstrAdding.Format(_T("Adding to list of sockets as %d"), i);

						m_statusmess = CstrAdding;// +m_statusmess;
						::SendMessage(m_hMainWnd, m_MessageUID, 0, 0);
						break;
					}
				}

			}
			m_bServerThreadFlag = TRUE;

			while (m_bServerThreadFlag)
			{
				//else its some IO operation on some other socket :)
				EnterCriticalSection(m_CS_Server);

				for (int i = 0; i < MAX_CLIENTS; i++)
				{
					sd = client_socket[i];

					//if (FD_ISSET(sd, &readfds) && i == RADAR_CLIENT) // only receive pack from LPRs
					if (i == RADAR_CLIENT)
					{
						//Check if it was for closing , and also read the incoming message

						BYTE recvbuf[TCP_BUFF_SIZE] = { 0 };
						int BytesRec = recv(sd, (char*)recvbuf, sizeof(recvbuf), 0);

						if (BytesRec == 0)
						{
							if (i == RADAR_CLIENT) RADAR_CLIENT = -1;
							m_statusmess = _T("The client closed the connection");// +m_statusmess;
							::SendMessage(m_hMainWnd, m_MessageUID, 0, 0);

							//Close the socket and mark as 0 in list for reuse
							closesocket(sd);
							client_socket[i] = 0;
							//closesocket(MasterSocket);
							m_bServerThreadFlag = FALSE;
							
							RemoveAllData_ListData();
							break;
						}
						else if (BytesRec == SOCKET_ERROR)
						{
							if (i == RADAR_CLIENT) RADAR_CLIENT = -1;
							m_statusmess = _T("The client seems to be offline.");// +m_statusmess;
							::SendMessage(m_hMainWnd, m_MessageUID, 0, 0);

							//Close the socket and mark as 0 in list for reuse
							closesocket(sd);
							client_socket[i] = 0;
							//closesocket(MasterSocket);
							m_bServerThreadFlag = FALSE;
							RemoveAllData_ListData();
							break;
						}
						else// if (BytesRec == 33)
						{
							m_RevQueue.PutData(recvbuf, BytesRec);
						}
					}

					if (!m_bServerThreadFlag)
					{
						closesocket(sd);
						client_socket[i] = 0;
						//closesocket(MasterSocket);
						RemoveAllData_ListData();

						CstrAdd.Format(_T("Close 1 socket number: %d"), sd);

						m_statusmess = CstrAdd;// +m_statusmess;
						::SendMessage(m_hMainWnd, m_MessageUID, 0, 0);

						break;
					}
				}

				if (!m_bServerThreadFlag)
				{
					closesocket(sd);
					client_socket[RADAR_CLIENT] = 0;
					//closesocket(MasterSocket);
					RemoveAllData_ListData();

					CstrAdd.Format(_T("Close 2 socket number: %d"), sd);

					m_statusmess = CstrAdd;// +m_statusmess;
					::SendMessage(m_hMainWnd, m_MessageUID, 0, 0);

					//break;
				}
				
				LeaveCriticalSection(m_CS_Server);
			}
		}
		else if (dwRet == WAIT_OBJECT_0)
		{
			closesocket(sd);
			client_socket[RADAR_CLIENT] = 0;
			closesocket(MasterSocket);
			RemoveAllData_ListData();

			CstrAdd.Format(_T("Close 3 socket number: %d"), sd);

			m_statusmess = CstrAdd;// +m_statusmess;
			::SendMessage(m_hMainWnd, m_MessageUID, 0, 0);

			break;
		}
		
	}
	::SetEvent(m_hServerHandle);

}


//============================== Package Receiving Processing Thread ============================

void CRadarManager::RevPkgMakingThread()
{
	m_hRevPkgMakingEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hRevPkgMakingHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_bRevPkgMakingThreadFlag = TRUE;
	m_hRevPkgMakingThread = AfxBeginThread(OnRevPkgMakingThread, this, THREAD_PRIORITY_NORMAL, 0);
	//m_bRevPkgMakingThreadFlag = TRUE;
}


UINT CRadarManager::OnRevPkgMakingThread(LPVOID pThread)
{
	CRadarManager *pData = (CRadarManager*)pThread;

	pData->RevPkgMaking();

	return 0;
}



void CRadarManager::RevPkgMaking()
{
	while (m_bRevPkgMakingThreadFlag)
	{
		DWORD dwRet;
		dwRet = WaitForSingleObject(m_hRevPkgMakingEvent, 5);

		if (dwRet == WAIT_TIMEOUT)
		{
			EnterCriticalSection(m_CS_Receive);

			int nGetCount = m_RevQueue.GetSize();

			int nCount = 0, nGetSize = 0, nDataPos = 0;

			TCHAR szErr[256] = { 0 };

			if (nGetCount > 0)
			{
				//for (int idx = 0; idx < nGetCount; idx++)
				try
				{
					while (nGetCount>0)
					{
						BYTE *pGetData = NULL;

						pGetData = m_RevQueue.GetData(nCount, nDataPos, nGetSize);
						if (nGetSize == 0)
						{
							m_statusmess = _T("nGetSize 0");// +m_statusmess;
							::SendMessage(m_hMainWnd, m_MessageUID, 0, 0);
						}

						for (int j = 0; j < nGetSize; j++)
						{
							if (RevPack_Process(pGetData[j]))
							{
								m_statusmess = _T("Finish making pakage");// +m_statusmess;
								//::SendMessage(m_hMainWnd, m_MessageUID, 0, 0);
							}
							else
							{
								m_statusmess = _T("making pakage...");// +m_statusmess;
								//::SendMessage(m_hMainWnd, m_MessageUID, 0, 0);
							}
							if (!m_bRevPkgMakingThreadFlag)
								break;
						}

						nGetCount = m_RevQueue.GetSize();

						if (!m_bRevPkgMakingThreadFlag)
							break;
					}
				}
				catch (CException* e)
				{
					e->GetErrorMessage((LPTSTR)szErr, 256, NULL);
					e->Delete();
				}
			}
			LeaveCriticalSection(m_CS_Receive);
		}
		else if (dwRet == WAIT_OBJECT_0)
		{
			break;
		}
		if (!m_bRevPkgMakingThreadFlag)
			break;
	}
	::SetEvent(m_hRevPkgMakingHandle);
}




//void CRadarManager::RadarInforProcess()
//{
//
//
//}
//
//
bool CRadarManager::RevPack_Process(BYTE Ch)
{
	CString str;
	if (m_SizeOfFrame < DATA_BUFF)
	{
		m_FrameBuffer[m_SizeOfFrame] = Ch;
		m_SizeOfFrame++;
	}

	//Start sequence(4) + data(n) + Checksum(1) + End sequence(4)
	//Ch == m_FrameBuffer[m_SizeOfFrame-1]
	switch (m_SizeOfFrame)
	{
	case 1:
	{
		if (Ch == 0xCA)
			return false;
		else
		{
			m_SizeOfFrame = 0;
			return false;
		}
	}
	break;
	case 2:
	{
		if (Ch == 0xCB)
			return false;
		else
		{
			m_SizeOfFrame = 0;
			return false;
		}
	}
	break;
	case 3:
	{
		if (Ch == 0xCC)
			return false;
		else
		{
			m_SizeOfFrame = 0;
			return false;
		}
	}
	break;
	case 4:
	{
		if (Ch == 0xCD)
			return false;
		else
		{
			m_SizeOfFrame = 0;
			return false;
		}
	}
	break;
	default:
	{
		if (Ch == 0xED)
		{
			if (m_FrameBuffer[m_SizeOfFrame - 2] == 0xEC)
			{
				if (m_FrameBuffer[m_SizeOfFrame - 3] == 0xEB)
				{
					if (m_FrameBuffer[m_SizeOfFrame - 4] == 0xEA)
					{
						//한 사이클 패킷 완성
					}
					else
					{
						return false;
					}
				}
				else
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	break;
	}

	//한 사이클 패킷 종료
	int nStartSequenceSize = 4;	//CA, CB, CC, CD	
	int nCheckSumSize = 1;
	int nEndSequenceSize = 4;	//EA, EB, EC, ED

	PRADAR_DATA pData = (PRADAR_DATA) new BYTE[sizeof(RADAR_DATA)];
	memset(pData, 0, sizeof(RADAR_DATA));

	pData->nDataLength = m_SizeOfFrame - nStartSequenceSize - nCheckSumSize - nEndSequenceSize;
	pData->pByData = new BYTE[pData->nDataLength];
	memcpy(pData->pByData, m_FrameBuffer + nStartSequenceSize, pData->nDataLength);

	/*memset(RevBuf, 0, sizeof(RADAR_DATA));
	RevBuf->nDataLength = m_SizeOfFrame - nStartSequenceSize - nCheckSumSize - nEndSequenceSize;
	RevBuf->pByData = new BYTE[RevBuf->nDataLength];
	memcpy(RevBuf->pByData, m_FrameBuffer + nStartSequenceSize, RevBuf->nDataLength);*/

	m_csData.Lock();
	m_PtrListData.AddTail((PRADAR_DATA)pData);
	m_csData.Unlock();

	//SetEvent(m_hEventArray[1]);

	m_SizeOfFrame = 0;
	ZeroMemory(m_FrameBuffer, DATA_BUFF);

	return true;
}

PRADAR_DATA CRadarManager::ReadPtrList(void)
{
	PRADAR_DATA pData = NULL;

	if (m_PtrListData.GetCount())
	{
		m_csData.Lock();

		pData = (PRADAR_DATA)m_PtrListData.RemoveHead();

		m_csData.Unlock();
	}

	return pData;
}

string CRadarManager::UTF8ToANSI(string s)
{
	BSTR    bstrWide;
	char*   pszAnsi;
	int     nLength;
	const char *pszCode = s.c_str();

	nLength = MultiByteToWideChar(CP_UTF8, 0, pszCode, strlen(pszCode) + 1, NULL, NULL);
	bstrWide = SysAllocStringLen(NULL, nLength);

	MultiByteToWideChar(CP_UTF8, 0, pszCode, strlen(pszCode) + 1, bstrWide, nLength);

	nLength = WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, NULL, 0, NULL, NULL);
	pszAnsi = new char[nLength + 1];
	memset(pszAnsi, NULL, nLength + 1);
	WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, pszAnsi, nLength, NULL, NULL);
	SysFreeString(bstrWide);

	string r(pszAnsi);
	delete[] pszAnsi;
	return r;
}


//=============== Initilation ===================

BOOL CRadarManager::InitSocket(HWND hMainWnd, UINT msgUid, int nServerPort)
{
	WSADATA wsaData;
	int nResult;
	nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (NO_ERROR != nResult)
	{
		return FALSE;
	}

	m_hMainWnd = hMainWnd;
	m_MessageUID = msgUid; 
	m_nServerPort = nServerPort;

	//create a master socket
	if ((MasterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == 0)
	{
		m_statusmess = _T("Could not create socket");// +m_statusmess;
		::PostMessage(m_hMainWnd, m_MessageUID, 0, 0);

		return FALSE;
	}

	//m_SendQueue.ClearData();
	//m_RevQueue.ClearData();

	return TRUE;
}




void CRadarManager::KillAllThreads()
{
	if (m_hServerThread != NULL)
	{
		m_bServerThreadFlag = FALSE;

		BOOL retcode = ::SetEvent(m_hServerEvent);
		if (!retcode)
		{

		}
		if (::WaitForSingleObject(m_hServerHandle, 1000) == WAIT_TIMEOUT)
		{
			::TerminateThread(m_hServerThread->m_hThread, 1);
		}
		// Wait a bit
		::Sleep(10);
		m_hServerThread = NULL;

		::CloseHandle(m_hServerEvent);
		::CloseHandle(m_hServerHandle);

		m_hServerEvent = NULL;
		m_hServerHandle = NULL;


		closesocket(sd);
		client_socket[RADAR_CLIENT] = 0;
		closesocket(MasterSocket);
		RemoveAllData_ListData();

		CString CstrAdd;
		CstrAdd.Format(_T("Kill thread socket number: %d"), sd);

		m_statusmess = CstrAdd;// +m_statusmess;
		::SendMessage(m_hMainWnd, m_MessageUID, 0, 0);

		LeaveCriticalSection(m_CS_Server);

	}



	if (m_hRevPkgMakingThread != NULL)
	{
		m_bRevPkgMakingThreadFlag = FALSE;
		BOOL retcode = ::SetEvent(m_hRevPkgMakingEvent);
		if (!retcode)
		{

		}
		if (::WaitForSingleObject(m_hRevPkgMakingHandle, 1000) == WAIT_TIMEOUT)
		{
			::TerminateThread(m_hRevPkgMakingThread->m_hThread, 1);
		}
		// Wait a bit
		::Sleep(10);
		m_hRevPkgMakingThread = NULL;

		::CloseHandle(m_hRevPkgMakingEvent);
		::CloseHandle(m_hRevPkgMakingHandle);

		m_hRevPkgMakingEvent = NULL;
		m_hRevPkgMakingHandle = NULL;

		LeaveCriticalSection(m_CS_Receive);
	}
}




