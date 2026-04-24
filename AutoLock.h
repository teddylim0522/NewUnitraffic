#pragma once

#ifndef AUTOLOCK_DEFINED
#define AUTOLOCK_DEFINED

#include <windows.h>

class CAutoLock
{
public:

	inline CAutoLock(HANDLE lock)
	{
		mLock = lock;// CreateSemaphore(nullptr, 1, 1, nullptr);

		DWORD dwResult = -1;
		dwResult = WaitForSingleObject(mLock, INFINITE);

		switch (dwResult)
		{
		case WAIT_OBJECT_0:
			break;
		case WAIT_ABANDONED:
			break;
		case WAIT_TIMEOUT:
			break;
		}
	}

	inline CAutoLock(HANDLE lock, int time)
	{
		mLock = lock;// CreateSemaphore(nullptr, 1, 1, nullptr);

		DWORD dwResult = -1;
		dwResult = WaitForSingleObject(mLock, time);

		switch (dwResult)
		{
		case WAIT_OBJECT_0:
			break;
		case WAIT_ABANDONED:
			break;
		case WAIT_TIMEOUT:
			break;
		}
	}

	inline virtual ~CAutoLock(void)
	{
		LONG	lPreviousCount = 0;
		::ReleaseSemaphore(mLock, 1, &lPreviousCount);
		//CloseHandle(mLock);
		mLock = nullptr;
	}

private:

	HANDLE	mLock;
};

#endif