#include <stdio.h>
#include <conio.h>
#include <process.h>

#pragma comment(lib, "winmm.lib")

#include "CrashDump.h"

#include "LockFreeQueue.h"



#define THREAD_COUNT 4
#define TEST_COUNT 5000

struct TEST_DATA
{
	LONG64	lData;
	LONG64	lCount;
};


CLockFreeQueue<TEST_DATA*> testPool;

__int64 pushTps = 0;
__int64 popTps = 0;

unsigned __stdcall WorkerThread(void* p);

void main()
{
	HANDLE handle[THREAD_COUNT];
	for (int i = 0; i < THREAD_COUNT; i++)
	{
		handle[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, 0, 0, NULL);
	}

	while (1)
	{
		wprintf(L"Push TPS [%d]\n", pushTps);
		wprintf(L"Pop  TPS [%d]\n\n", popTps);


		InterlockedExchange64(&pushTps, 0);
		InterlockedExchange64(&popTps, 0);

		Sleep(999);

	}

	WaitForMultipleObjects(THREAD_COUNT, handle, TRUE, INFINITE);
}

//long testCount = 0;
unsigned __stdcall WorkerThread(void* p)
{
	int iCnt;
	long testCount = 0;
	TEST_DATA* testData[TEST_COUNT];
	TEST_DATA* testTemp;

	for (iCnt = 0; iCnt < TEST_COUNT; iCnt++)
	{
		testData[iCnt] = new TEST_DATA;
		testData[iCnt]->lData = 0x0000000055555555;
		testData[iCnt]->lCount = 0;
	}
	for (iCnt = 0; iCnt < TEST_COUNT; iCnt++)
	{
		testPool.EnQueue(testData[iCnt]);
		InterlockedIncrement64(&pushTps);
		
	}



	while (1)
	{
		for (iCnt = 0; iCnt < TEST_COUNT; iCnt++)
		{
			if (testPool.Dequeue(testTemp) == false)
			{
				CCrashDump::Crash();
			}

			InterlockedIncrement64(&popTps);

			if (testTemp == NULL)
			{
				CCrashDump::Crash();
			}

			testData[iCnt] = testTemp;
		}

		for (iCnt = 0; iCnt < TEST_COUNT; iCnt++)
		{
			if (testData[iCnt]->lData != 0x0000000055555555 || testData[iCnt]->lCount != 0)
				CCrashDump::Crash();
		}

		for (iCnt = 0; iCnt < TEST_COUNT; iCnt++)
		{
			InterlockedIncrement64(&testData[iCnt]->lCount);
			InterlockedIncrement64(&testData[iCnt]->lData);
		}

		Sleep(0);

		for (iCnt = 0; iCnt < TEST_COUNT; iCnt++)
		{
			if (testData[iCnt]->lData != 0x0000000055555556 || testData[iCnt]->lCount != 1)
				CCrashDump::Crash();
		}

		Sleep(0);

		for (iCnt = 0; iCnt < TEST_COUNT; iCnt++)
		{
			InterlockedDecrement64(&testData[iCnt]->lCount);
			InterlockedDecrement64(&testData[iCnt]->lData);
		}

		for (iCnt = 0; iCnt < TEST_COUNT; iCnt++)
		{
			if (testData[iCnt]->lData != 0x0000000055555555 || testData[iCnt]->lCount != 0)
				CCrashDump::Crash();
		}

		for (iCnt = 0; iCnt < TEST_COUNT; iCnt++)
		{
			testPool.EnQueue(testData[iCnt]);
			InterlockedIncrement64(&pushTps);
		}
	}
}