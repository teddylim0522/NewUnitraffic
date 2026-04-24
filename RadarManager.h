#ifndef __SERVER_H__
#define __SERVER_H__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <iostream>
#include <vector>
//#include "PackageForm.h"
//#include "GlobalData.h"
#include "DataQueue.h"
//#include "TraceLog.h"
#include "time.h"



#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define FD_SIZE_VECTOR	10
#define TCP_BUFF_SIZE	4096
#define MAX_CLIENTS		1
#define WM_RADAR_MANAGER (WM_USER + 1018)

typedef struct _RADAR_DATA
{
	//	BYTE	byStartSequence[4];	//CA CB CC CD
	int		nDataLength;
	BYTE	*pByData;
	//	BYTE	byChecksum;
	//	BYTE	byEndSequence[4];	//EA EB EC ED

} RADAR_DATA, *PRADAR_DATA;

class CRadarManager
{
public:
	CRadarManager();
	~CRadarManager();
private:

	void SetServerSockAddr(sockaddr_in *sockAddr, int PortNumber);

protected:
	CCriticalSection m_CS_Server;
	CCriticalSection m_CS_Receive;

public:
	CString m_strRadarIP;
	BOOL InitSocket(HWND hMainWnd, UINT msgUid, int nServerPort);

	HANDLE		m_hServerEvent, m_hServerHandle, m_hRevPkgMakingEvent, m_hRevPkgMakingHandle;
	CWinThread *m_hServerThread, *m_hRevPkgMakingThread;
	BOOL m_bServerThreadFlag, m_bRevPkgMakingThreadFlag;

	void StartServerThread();
	static UINT OnServerThread(LPVOID pThread);
	void ListenSocket();
	int RADAR_CLIENT;

	void RevPkgMakingThread();
	static UINT OnRevPkgMakingThread(LPVOID pThread);
	void RevPkgMaking();

	CString m_statusmess;
	
	HWND m_hMainWnd;
	UINT m_MessageUID;

	BOOL m_nServerPort;
	SOCKET MasterSocket;

	fd_set m_ReadSet, m_WriteSet, m_ExceptSet;
	vector<fd_set> m_ReadFD, m_ExceptFD, m_WriteFD;
	int m_FD_SIZE_COUNT;


	int client_socket[MAX_CLIENTS];
	int addrlen, new_socket, activity, valread, sd;
	int max_sd;
	BOOL opt = TRUE;

	RDDataQueue m_RevQueue;

	string UTF8ToANSI(string s);


	BYTE m_RevData[100];
	//int m_RevIndex;
	//RADAR_DATA RevBuf;
	bool RevPack_Process(BYTE Ch);

	void KillAllThreads();

	BYTE				m_FrameBuffer[DATA_BUFF];	
	DWORD				m_SizeOfFrame;	

	CPtrList			m_PtrListData;
	void RemoveAllData_ListData();
	CCriticalSection	m_csData;
	PRADAR_DATA ReadPtrList(void);

	bool bChang;
	//void RadarInforProcess();
};
#endif

