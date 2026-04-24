#pragma once

#include <afxwin.h>
#include <string>
#include <queue>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
//// StringBuffer (Output)
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

//time millisecond
#include <time.h>
#include <sys/timeb.h>

#include "param_set.h"

#include <iostream>
#include <fstream>
#include <sstream>

using namespace rapidjson;
using namespace std;

#define DEF_DEFAULT_PORT		(13488)
#define DEF_MAX_THERMAL_SERVICE (1)

#define _THREAD_TIMEOUT			(5000)
#define MAX_QVDSDATA_SIZE		(30)

#define MAX_MTU_SIZE			(1500)
#define MAX_FILED_CNT			(14)
#define MAX_BUFFER_SIZE			(1024)
#define DEF_TRANSCATION_ID			(_T("VDS SERVER"))
#define DEF_ERROR_RESULT_CODE	(500)
#define DEF_SUCCESS_RESULT_CODE	(100)
enum _VDS_FILED
{
	VF_DETECT_TIME =0,
	VF_LANE,
	VF_DIRECTION,
	VF_LENGTH,
	VF_SPEED,
	VF_CLASS,
	VF_OCCUPTY_TIME,
	VF_LOOP1_OCCPY_TIME,
	VF_LOOP2_OCCPY_TIME,
	VF_REVERSE,
	VF_VEHICLE_GAP,
	VF_ID
};



#if 0
#define VDS_CONNECT_REQ			(16)//0x0010
#define VDS_REALTIME_DATA_REQ		(32)//0x0020
#define VDS_NOT_SENT_DATA_REQ	(33)//0x0021
#define VDS_HEART_BEART			(48)//0x0030
#else
#define VDS_CONNECT				(0x0010)//0x0010
#define VDS_REALTIME_DATA		(0x0020)//0x0020
#define VDS_NOT_SENT_DATA		(0x0021)//0x0021
#define VDS_HEART_BEART			(0x0030)//0x0030
#endif

typedef struct _TCP_CONFIG
{
	CString strHost = "127.0.0.1";
	unsigned int nPort = DEF_DEFAULT_PORT;
	unsigned int nRecvSize = 65536;
	unsigned int nSendSize = 65536;
	CString strSourceURL = "";
	CString strDetectionUrl = "";
}TCP_CONFIG;

typedef struct _ST_VDS_HEAD
{
	BYTE strVer;
	BYTE strType;
	unsigned char strOpCode;
	char strTranscation[20];
	char strDataSize[4];
}ST_VDS_HEAD;


class JsonMsg
{
public:
	JsonMsg() {};
	JsonMsg(int len, char * msg)
	{
		nLen = len;
		//CoTaskMemAlloc
		//strJson = new char[len + 1];
		strJson = (char *)CoTaskMemAlloc(len + 1);
		ZeroMemory(strJson, len + 1);
		memcpy(strJson, msg, len);
	}

	~JsonMsg() {
		clear();
	}

	void clear()
	{
		nLen = 0;
		/*if (strJson)
			delete[] strJson;*/
		if (strJson)
			CoTaskMemFree(strJson);
		strJson = NULL;
	}

public:
	int nLen = 0;
	char * strJson = NULL;
};

class CVdsData
{
public:
	CVdsData()
	{

	};
	~CVdsData()
	{

	}
public:
	std::string strId = "0";
	int nLane = 0;
	int nDirection = 0;
	int nLength = 0;
	double dSpeed = 0.0;
	int nClass = 0;
	int nOccupyTime = 0;
	int nLoop1OccupyTime = 0;
	int nLoop2OccupyTime = 0;
	std::string strReverseRun = "N";
	int nVehicleGap = 0;
	std::string strDetectTime = "";
};


class CTcpService
{
public:
	CTcpService(SOCKET sockfd, void *pParent);
	~CTcpService();

	int GetSockNum()
	{
		return (int)m_sock;
	}

	BOOL GetConStatus() 
	{
		return m_bConnect;
	}

private:
	SOCKET m_sock;
	BOOL m_bConnect;
	void *m_pParent;

	queue<CVdsData *> m_QSendVdsData;
	CRITICAL_SECTION	m_csSendVdsData;
	BOOL m_bMsgRecv;
	DWORD m_dwMsgRecv;
	HANDLE m_hMsgRecv;

	BOOL m_bRealTimeSend;
	DWORD m_dwRealTimeSend;
	HANDLE m_hRealTimeSend;

public:
	void PushSendVdsData(CVdsData *pVD);
	unsigned int ThreadMsgRecv();
	unsigned int ThreadRealTimeDataSend();
	
	//Util
	char* CStToChar(CString strParam);
	UINT CountNL(char *pszIn, char *pszEnd);
	double CompareSystemTime(SYSTEMTIME stStart, SYSTEMTIME stEnd);
	void GetFileTimeName(SYSTEMTIME stStart, SYSTEMTIME stEnd, vector<CString> *pvecFileTimeName);
	void WriteLog(CString strLog);

private:
	void EmptySendVdsDataQue();
	void StartThreadMsgRecv();
	void StopThreadMsgRecv();
	void OnRecvMessage(char *pszMsg, int nLen);
	void SendMsg(ST_VDS_HEAD stVdsHead, std::string strControlId, unsigned char strOpcode);
	void NotSendMsg(CString strST, CString strET, UINT nPageSize, UINT nPageIndex, UINT nTotalSize, vector<CString> *pvecFileTimeName, UINT *pnResultCode, CString *pstrResultMsg);

	void StartThreadRealTimeDataSend();
	void StopThreadRealTimeDataSend();

	void RealTimeDataSend();
	void StringToBytearray(std::string str, unsigned char* &array, int &size);
	
	CString GetRealTimeDataJson(CString *pstrID, CString *pstrDetectTime);
	CString GetNotSendData(CString strStartTime, CString strEndTime, UINT nPageSize, UINT nPageIndex, UINT nTotalSize, vector<CString> *pvecFileTimeName, UINT* pnResultCode, CString* pstrResultMsg, UINT *pnSendSIze);
	UINT GetTotalNotSendDataCnt(CString strStartTime, CString strEndTime, vector<CString> *pvecFileTimeName, UINT* pnResultCode, CString *pstrResultMsg);
	UINT GetFileTotalLine(vector<CString> *pvecFileTimeName);

	TCP_CONFIG GetTcpConfig();
};

