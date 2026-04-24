#include "StdAfx.h"
#include "DataQueue.h"


RSData::RSData()
{
	ClearData();
}

RSData::~RSData()
{

}

void RSData::ClearData()
{
	memset(m_Data, NULL, DATA_BUFF);
	m_DataSize = 0;
}

RSDataQueue::RSDataQueue(void)
{
	m_CntPos = 0;
	m_DesPos = 0;

	for (int i = 0; i < DATA_QUEUE; i++)
	{
		m_Queue[i] = new RSData();
	}
}

RSDataQueue::~RSDataQueue(void)
{
	m_CntPos = 0;
	m_DesPos = 0;

	for (int i = 0; i < DATA_QUEUE; i++)
	{
		if (m_Queue[i] != NULL)
		{
			delete m_Queue[i];
			m_Queue[i] = NULL;
		}
	}
}

void RSDataQueue::ClearData()
{
	for (int i = 0; i < DATA_QUEUE; i++)
	{
		if (m_Queue[i] != NULL)
		{
			memset(m_Queue[i]->m_Data, NULL, DATA_BUFF);
			m_Queue[i]->m_DataSize = 0;
		}
	}
}

int RSDataQueue::PutData(unsigned char *pData, int nSize)
{
	TCHAR szErr[256] = { 0 };
	TCHAR szLog[256] = { 0 };

	int nCount;

	try
	{
		if (nSize > DATA_BUFF)
		{
			return 0;
		}

		{
			memset(m_Queue[m_DesPos]->m_Data, NULL, DATA_BUFF);
			memcpy(m_Queue[m_DesPos]->m_Data, pData, nSize);
			m_Queue[m_DesPos]->m_DataSize = nSize;

			nCount = (m_DesPos - m_CntPos + DATA_QUEUE) % DATA_QUEUE;

			m_DesPos++;
			m_DesPos %= DATA_QUEUE;

			// 다음번 저장 위치 Clear
			memset(m_Queue[m_DesPos]->m_Data, NULL, DATA_BUFF);
			m_Queue[m_DesPos]->m_DataSize = 0;
		}
	}
	catch (CException* e)
	{
		e->GetErrorMessage((LPTSTR)szErr, 256, NULL);
		e->Delete();
	}
	return nSize;
}

BYTE* RSDataQueue::GetData(int &ndataCnt, int &nPos, int &nSize)
{
	unsigned char *pData = NULL;
	TCHAR szErr[256] = { 0 };
	TCHAR szLog[256] = { 0 };

	int nCount;

	try
	{
		{
			nCount = (m_DesPos - m_CntPos + DATA_QUEUE) % DATA_QUEUE;
			if (nCount == 0)
			{
				ndataCnt = nCount;
				nPos = m_CntPos;
				nSize = 0;
				return NULL;
			}
			pData = m_Queue[m_CntPos]->m_Data;
			nSize = m_Queue[m_CntPos]->m_DataSize;
			nPos = m_CntPos;
			ndataCnt = nCount;

			m_CntPos++;
			m_CntPos %= DATA_QUEUE;
		}

	}
	catch (CException* e)
	{
		e->GetErrorMessage((LPTSTR)szErr, 256, NULL);
	}

	return pData;
}

int RSDataQueue::GetSize()
{
	int nSize = 0;

	{
		nSize = (m_DesPos - m_CntPos + DATA_QUEUE) % DATA_QUEUE;
	}

	return nSize;
}

int RSDataQueue::DeleteData(int nPos)
{
	//int nRet = 1;
	//TCHAR szErr[256] = { 0 };
	//TCHAR szLog[256] = { 0 };

	//try
	//{
	//	//CSingleLock sLock(&m_DataCS);

	//	if (nPos == (DATA_QUEUE - 1)) return 0;

	//	// 나중에 하자..

	//	return 1;

	//}
	//catch (CException *e)
	//{
	//	e->GetErrorMessage((LPTSTR)szErr, 256, NULL);
	//	e->Delete();
	//}
	//return nRet;


	int nRet = 1;
	TCHAR szErr[256] = { 0 };
	TCHAR szLog[256] = { 0 };

	try
	{
		//CSingleLock sLock(&m_DataCS);

		if (nPos == (DATA_QUEUE - 1)) return 0;

		// 나중에 하자..
		if (m_Queue[nPos] != NULL)
		{
			int nCount = (m_DesPos - m_CntPos + DATA_QUEUE) % DATA_QUEUE;

			if (nCount == 0)
			{
				memset(m_Queue[nPos]->m_Data, NULL, DATA_BUFF);
				m_Queue[nPos]->m_DataSize = 0;
				m_DesPos--;
				m_DesPos %= DATA_QUEUE;
				m_CntPos--;
				m_CntPos %= DATA_QUEUE;
			}
			else
			{
				for (int i = nPos; i < m_DesPos; i++)
				{
					memset(m_Queue[i]->m_Data, NULL, DATA_BUFF);
					m_Queue[i]->m_DataSize = 0;
					if (i + 1 < m_DesPos)
					{
						memcpy(m_Queue[i]->m_Data, m_Queue[i + 1]->m_Data, m_Queue[i + 1]->m_DataSize);
						m_Queue[i]->m_DataSize = m_Queue[i + 1]->m_DataSize;
					}
				}
				m_DesPos--;
				m_DesPos %= DATA_QUEUE;
				m_CntPos--;
				m_CntPos %= DATA_QUEUE;
			}
		}

		return 1;

	}
	catch (CException *e)
	{
		e->GetErrorMessage((LPTSTR)szErr, 256, NULL);
		e->Delete();
	}
	return nRet;

}


////////////////////////////////////////////////
//=============== For LPR Pack ================
////////////////////////////////////////////////

RDData::RDData()
{
	ClearData();
}

RDData::~RDData()
{

}

void RDData::ClearData()
{
	memset(m_Data, NULL, DATA_BUFF);
	m_DataSize = 0;
}



RDDataQueue::RDDataQueue(void)
{
	m_CntPos = 0;
	m_DesPos = 0;

	for (int i = 0; i < DATA_QUEUE; i++)
	{
		m_Queue[i] = new RDData();
	}
}

RDDataQueue::~RDDataQueue(void)
{
	m_CntPos = 0;
	m_DesPos = 0;

	for (int i = 0; i < DATA_QUEUE; i++)
	{
		if (m_Queue[i] != NULL)
		{
			delete m_Queue[i];
			m_Queue[i] = NULL;
		}
	}
}

void RDDataQueue::ClearData()
{
	for (int i = 0; i < DATA_QUEUE; i++)
	{
		if (m_Queue[i] != NULL)
		{
			memset(m_Queue[i]->m_Data, NULL, DATA_BUFF);
			m_Queue[i]->m_DataSize = 0;
		}
	}
}

int RDDataQueue::PutData(BYTE *pData, int nSize)
{
	TCHAR szErr[256] = { 0 };
	TCHAR szLog[256] = { 0 };

	int nCount;

	try
	{
		if (nSize > DATA_BUFF)
		{
			return 0;
		}

		{
			memset(m_Queue[m_DesPos]->m_Data, NULL, DATA_BUFF);
			memcpy(m_Queue[m_DesPos]->m_Data, pData, nSize);
			m_Queue[m_DesPos]->m_DataSize = nSize;

			nCount = (m_DesPos - m_CntPos + DATA_QUEUE) % DATA_QUEUE;

			m_DesPos++;
			m_DesPos %= DATA_QUEUE;

			// 다음번 저장 위치 Clear
			memset(m_Queue[m_DesPos]->m_Data, NULL, DATA_BUFF);
			m_Queue[m_DesPos]->m_DataSize = 0;
		}
	}
	catch (CException* e)
	{
		e->GetErrorMessage((LPTSTR)szErr, 256, NULL);
		e->Delete();
	}
	return nSize;
}

BYTE* RDDataQueue::GetData(int &ndataCnt, int &nPos, int &nSize)
{
	BYTE *pData = NULL;
	TCHAR szErr[256] = { 0 };
	TCHAR szLog[256] = { 0 };

	int nCount;

	try
	{
		{
			nCount = (m_DesPos - m_CntPos + DATA_QUEUE) % DATA_QUEUE;
			if (nCount == 0)
			{
				ndataCnt = nCount;
				nPos = m_CntPos;
				nSize = 0;
				return NULL;
			}
			pData = m_Queue[m_CntPos]->m_Data;
			nSize = m_Queue[m_CntPos]->m_DataSize;
			nPos = m_CntPos;
			ndataCnt = nCount;

			m_CntPos++;
			m_CntPos %= DATA_QUEUE;
		}

	}
	catch (CException* e)
	{
		e->GetErrorMessage((LPTSTR)szErr, 256, NULL);
	}

	return pData;
}

int RDDataQueue::GetSize()
{
	int nSize = 0;

	{
		nSize = (m_DesPos - m_CntPos + DATA_QUEUE) % DATA_QUEUE;
	}

	return nSize;
}

int RDDataQueue::DeleteData(int nPos)
{
	int nRet = 1;
	TCHAR szErr[256] = { 0 };
	TCHAR szLog[256] = { 0 };

	try
	{
		//CSingleLock sLock(&m_DataCS);

		if (nPos == (DATA_QUEUE - 1)) return 0;

		// 나중에 하자..
		if (m_Queue[nPos] != NULL)
		{
			int nCount = (m_DesPos - m_CntPos + DATA_QUEUE) % DATA_QUEUE;

			if (nCount == 0)
			{
				memset(m_Queue[nPos]->m_Data, NULL, DATA_BUFF);
				m_Queue[nPos]->m_DataSize = 0;
				m_DesPos--;
				m_DesPos %= DATA_QUEUE;
				m_CntPos--;
				m_CntPos %= DATA_QUEUE;
			}
			else
			{
				for (int i = nPos; i < m_DesPos; i++)
				{
					memset(m_Queue[i]->m_Data, NULL, DATA_BUFF);
					m_Queue[i]->m_DataSize = 0;
					if (i + 1 < m_DesPos)
					{
						memcpy(m_Queue[i]->m_Data, m_Queue[i + 1]->m_Data, m_Queue[i + 1]->m_DataSize);
						m_Queue[i]->m_DataSize = m_Queue[i + 1]->m_DataSize;
					}
				}
				m_DesPos--;
				m_DesPos %= DATA_QUEUE;
				m_CntPos--;
				m_CntPos %= DATA_QUEUE;
			}
		}

		/*CString hai;
		hai.Format(_T("des: %d, cnt: %d"), m_DesPos, m_CntPos);
		AfxMessageBox(hai);*/

		return 1;

	}
	catch (CException *e)
	{
		e->GetErrorMessage((LPTSTR)szErr, 256, NULL);
		e->Delete();
	}
	return nRet;
}




//////////////////////////////////////////////////////////////////
////******************** Laser Scanner ***************************
//////////////////////////////////////////////////////////////////

LSData::LSData()
{
	ClearData();
}

LSData::~LSData()
{

}

void LSData::ClearData()
{
	memset(m_Data, NULL, LS_BUFF);
	m_DataSize = 0;
}



LSDataQueue::LSDataQueue(void)
{
	m_CntPos = 0;
	m_DesPos = 0;
	m_CheckPos = -1;

	for (int i = 0; i < LS_QUEUE; i++)
	{
		m_Queue[i] = new LSData();
	}
}

LSDataQueue::~LSDataQueue(void)
{
	m_CntPos = 0;
	m_DesPos = 0;

	/*for (int i = 0; i < LS_QUEUE; i++)
	{
	if (m_Queue[i] != NULL)
	{
	delete m_Queue[i];
	m_Queue[i] = NULL;
	}
	}*/
}

void LSDataQueue::ClearData()
{
	for (int i = 0; i < LS_QUEUE; i++)
	{
		if (m_Queue[i] != NULL)
		{
			memset(m_Queue[i]->m_Data, NULL, LS_BUFF);
			m_Queue[i]->m_DataSize = 0;
		}
	}
}

int LSDataQueue::PutData(BYTE *pData, int nSize)
{
	TCHAR szErr[256] = { 0 };
	TCHAR szLog[256] = { 0 };

	int nCount;

	try
	{
		if (nSize > LS_BUFF)
		{
			return 0;
		}

		{
			memset(m_Queue[m_DesPos]->m_Data, NULL, LS_BUFF);
			memcpy(m_Queue[m_DesPos]->m_Data, pData, nSize);
			m_Queue[m_DesPos]->m_DataSize = nSize;

			nCount = (m_DesPos - m_CntPos + LS_QUEUE) % LS_QUEUE;

			m_CheckPos = m_DesPos;
			m_DesPos++;
			m_DesPos %= LS_QUEUE;

			// 다음번 저장 위치 Clear
			memset(m_Queue[m_DesPos]->m_Data, NULL, LS_BUFF);
			m_Queue[m_DesPos]->m_DataSize = 0;
		}
	}
	catch (CException* e)
	{
		e->GetErrorMessage((LPTSTR)szErr, 256, NULL);
		e->Delete();
	}
	return nSize;
}

BYTE* LSDataQueue::GetData(int &ndataCnt, int &nPos, int &nSize)
{
	BYTE *pData = NULL;
	TCHAR szErr[256] = { 0 };
	TCHAR szLog[256] = { 0 };

	int nCount;

	try
	{
		{
			nCount = (m_DesPos - m_CntPos + LS_QUEUE) % LS_QUEUE;
			if (nCount == 0)
			{
				ndataCnt = nCount;
				nPos = m_CntPos;
				nSize = 0;
				return NULL;
			}
			pData = m_Queue[m_CntPos]->m_Data;
			nSize = m_Queue[m_CntPos]->m_DataSize;
			nPos = m_CntPos;
			ndataCnt = nCount;

			m_CntPos++;
			m_CntPos %= LS_QUEUE;
		}

	}
	catch (CException* e)
	{
		e->GetErrorMessage((LPTSTR)szErr, 256, NULL);
	}

	return pData;
}

int LSDataQueue::GetSize()
{
	int nSize = 0;

	{
		nSize = (m_DesPos - m_CntPos + LS_QUEUE) % LS_QUEUE;
	}

	return nSize;
}

int LSDataQueue::DeleteData(int nPos)
{
	int nRet = 1;
	TCHAR szErr[256] = { 0 };
	TCHAR szLog[256] = { 0 };

	try
	{
		//CSingleLock sLock(&m_DataCS);

		if (nPos == (LS_QUEUE - 1)) return 0;

		// 나중에 하자..
		if (m_Queue[nPos] != NULL)
		{
			int nCount = (m_DesPos - m_CntPos + LS_QUEUE) % LS_QUEUE;

			if (nCount == 0)
			{
				memset(m_Queue[nPos]->m_Data, NULL, LS_BUFF);
				m_Queue[nPos]->m_DataSize = 0;
				m_DesPos--;
				m_DesPos %= LS_QUEUE;
				m_CntPos--;
				m_CntPos %= LS_QUEUE;
			}
			else
			{
				for (int i = nPos; i < m_DesPos; i++)
				{
					memset(m_Queue[i]->m_Data, NULL, LS_BUFF);
					m_Queue[i]->m_DataSize = 0;
					if (i + 1 < m_DesPos)
					{
						memcpy(m_Queue[i]->m_Data, m_Queue[i + 1]->m_Data, m_Queue[i + 1]->m_DataSize);
						m_Queue[i]->m_DataSize = m_Queue[i + 1]->m_DataSize;
					}
				}
				m_DesPos--;
				m_DesPos %= LS_QUEUE;
				m_CntPos--;
				m_CntPos %= LS_QUEUE;
			}
		}

		/*CString hai;
		hai.Format(_T("des: %d, cnt: %d"), m_DesPos, m_CntPos);
		AfxMessageBox(hai);*/

		return 1;

	}
	catch (CException *e)
	{
		e->GetErrorMessage((LPTSTR)szErr, 256, NULL);
		e->Delete();
	}
	return nRet;
}


BYTE* LSDataQueue::GetAt(int nPos, int& nSize)
{
	BYTE *pData = NULL;
	TCHAR szErr[256] = { 0 };

	int nCount;

	try
	{
		{
			/*nCount = (m_DesPos - m_CntPos + LS_QUEUE) % LS_QUEUE;
			if (nCount == 0)
			{
				return NULL;
			}*/
			pData = m_Queue[nPos]->m_Data;
			nSize = m_Queue[nPos]->m_DataSize;
		}

	}
	catch (CException* e)
	{
		e->GetErrorMessage((LPTSTR)szErr, 256, NULL);
	}

	return pData;
}

