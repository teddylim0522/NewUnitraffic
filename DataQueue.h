#pragma once

#define DATA_BUFF 0x2048	// 한번에 받을 버퍼 크기
#define DATA_QUEUE 0x1024	// Queue 크기


#define LS_BUFF 0x2048		// Laser Scanner: 한번에 받을 버퍼 크기
#define LS_QUEUE 0x1200		// Laser Scanner: Queue 크기

class RSData
{
public:
	RSData();
	virtual ~RSData();
	unsigned char m_Data[DATA_BUFF];
	int m_DataSize;
	void ClearData();
};

class RSDataQueue
{
public:
	RSDataQueue();
	virtual ~RSDataQueue();

	int PutData(unsigned char *pData, int nSize);	// MAX input data : DATA_BUFF
	unsigned char* GetData(int &ndataCnt, int &nPos, int &nSize);
	int DeleteData(int nPos);
	int GetSize();
	void ClearData();

private:
	RSData *m_Queue[DATA_QUEUE];
	int m_CntPos;
	int m_DesPos;

};


//========== For LPR Socket =========
class RDData
{
public:
	RDData();
	virtual ~RDData();
	BYTE m_Data[DATA_BUFF];
	int m_DataSize;
	void ClearData();
};


class RDDataQueue
{
public:
	RDDataQueue();
	virtual ~RDDataQueue();

	int PutData(BYTE *pData, int nSize);	// MAX input data : DATA_BUFF
	BYTE* GetData(int &ndataCnt, int &nPos, int &nSize);
	int DeleteData(int nPos);
	int GetSize();
	void ClearData();

private:
	RDData *m_Queue[DATA_QUEUE];
	int m_CntPos;
	int m_DesPos;

};




//========== For Laser Scanner =========
class LSData
{
public:
	LSData();
	virtual ~LSData();
	BYTE m_Data[LS_BUFF];
	int m_DataSize;
	void ClearData();
};


class LSDataQueue
{
public:
	LSDataQueue();
	virtual ~LSDataQueue();

	int PutData(BYTE *pData, int nSize);	// MAX input data : LS_BUFF
	BYTE* GetData(int &ndataCnt, int &nPos, int &nSize);
	int DeleteData(int nPos);
	int GetSize();
	void ClearData();
	BYTE* GetAt(int nPos, int& nSize);
	int m_CheckPos;
private:
	LSData *m_Queue[LS_QUEUE];
	int m_CntPos;
	int m_DesPos;

};
