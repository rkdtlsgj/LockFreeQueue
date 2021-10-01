#pragma once
#include <malloc.h>
#include <Windows.h>
#include <winnt.h>
#include "CMemoryPool.h"

template<class T>

class CLockFreeQueue
{
private:
	struct Node
	{
		T _data;
		Node* _pNext;
	};

	struct TopNode
	{
		__int64 _iNumber;
		Node* _pNode;
	};


	long _size;
	__int64 iNumber;
	//TopNode* _head;
	//TopNode* _tail;
	Node* _head;
	Node* _tail;
	CMemoryPool<Node>* pMemoryPool;
public:

	//CLockFreeQueue()
	//{

	//	pMemoryPool = new CMemoryPool<Node>(0);

	//	_head = (TopNode*)_aligned_malloc(sizeof(TopNode), 16);
	//	_head->_pNode = pMemoryPool->Alloc();
	//	_head->_pNode->_pNext = NULL;
	//	_head->_iNumber = 0;

	//	_tail = (TopNode*)_aligned_malloc(sizeof(TopNode), 16);
	//	_tail->_pNode = _head->_pNode;
	//	_tail->_iNumber = 0;


	//	_size = 0;
	//	iNumber = 0;
	//	//_head = new Node;
	//	//_head = pMemoryPool->Alloc();
	//	//_head->_pNext = NULL;

	//	//_tail = _head;

	//}

	//void EnQueue(T t)
	//{
	//	Node* node = pMemoryPool->Alloc();
	//	node->_data = t;
	//	node->_pNext = NULL;
	//	volatile __int64 enqNumber = InterlockedIncrement64(&iNumber);

	//	TopNode tTempNode;
	//	Node* next;
	//	while (true)
	//	{
	//		tTempNode._iNumber = _tail->_iNumber;
	//		tTempNode._pNode = _tail->_pNode;

	//		next = tTempNode._pNode->_pNext;

	//		if (next == NULL)
	//		{
	//			if (InterlockedCompareExchangePointer((PVOID*)& tTempNode._pNode->_pNext, node, next) == next)
	//			{					
	//				if (InterlockedCompareExchange128((volatile LONG64*)_tail, (LONG64)node, enqNumber, (LONG64*)& tTempNode) == false)
	//				{
	//					break;
	//				};
	//				break;
	//			}
	//		}
	//		else
	//		{
	//			InterlockedCompareExchange128((volatile LONG64*)_tail, (LONG64)next ,enqNumber, (LONG64*)& tTempNode);
	//		}
	//	}

	//	InterlockedIncrement(&_size);
	//}

	//int Dequeue(T& t)
	//{
	//	if (InterlockedDecrement(&_size) < 0)
	//	{
	//		InterlockedIncrement(&_size);
	//		return false;
	//	}

	//	volatile __int64 deqNumber = InterlockedIncrement64(&iNumber);

	//	TopNode hTempNode;
	//	TopNode tTempNode;
	//	Node* hNext;
	//	while (true)
	//	{			
	//		hTempNode._iNumber = _head->_iNumber;
	//		hTempNode._pNode = _head->_pNode;

	//		tTempNode._iNumber = _tail->_iNumber;
	//		tTempNode._pNode = _tail->_pNode;

	//		hNext = hTempNode._pNode->_pNext;

	//		//Head와 Tail이 같은경우
	//		//Enqueue에서 값이 들어왔지만 아직 Tail을 못민경우			
	//		//진짜 데이터가 없는 경우???
	//		if (tTempNode._pNode->_pNext != NULL)
	//		{
	//			//InterlockedCompareExchange128((volatile LONG64*)_tail, (LONG64)tTempNode._pNode->_pNext, deqNumber, (LONG64*)& tTempNode);
	//		}
	//		else// if (hNext != NULL)
	//		{
	//			t = hNext->_data;
	//			//if (InterlockedCompareExchangePointer((PVOID*)& _head, next, head) == head)
	//			if(InterlockedCompareExchange128((volatile LONG64*)_head, (LONG64)hNext, deqNumber, (LONG64*)& hTempNode) == true)
	//			{
	//				pMemoryPool->Free(hTempNode._pNode);
	//				break;
	//			}
	//		}

	//	}
	//	// InterlockedExchangeAdd(&_size, -1);
	//	return true;
	//}

	CLockFreeQueue()
	{

		pMemoryPool = new CMemoryPool<Node>(0);

		_size = 0;
		//_head = new Node;
		_head = pMemoryPool->Alloc();
		_head->_pNext = NULL;
		_tail = _head;

	}

	void EnQueue(T t)
	{
		Node* node = pMemoryPool->Alloc();
		node->_data = t;
		node->_pNext = NULL;

		while (true)
		{
			Node* tail = _tail;
			Node* next = tail->_pNext;

			if (next == NULL)
			{
				if (InterlockedCompareExchangePointer((PVOID*)& tail->_pNext, node, next) == next)
				{
					InterlockedCompareExchangePointer((PVOID*)& _tail, node, tail);
					break;
				}
			}
			else
			{
				InterlockedCompareExchangePointer((PVOID*)& _tail, next, tail);
			}
		}

		InterlockedIncrement(&_size);
	}

	int Dequeue(T& t)
	{
		if (InterlockedDecrement(&_size) < 0)
		{
			InterlockedIncrement(&_size);
			return false;
		}

		while (true)
		{
			Node* head = _head;
			Node* next = head->_pNext;
			Node* tail = _tail;

			/*{
				InterlockedCompareExchangePointer((PVOID*)& _tail, tail->_pNext, tail);
			}*/
			//Head와 Tail이 같은경우
			//Enqueue에서 값이 들어왔지만 아직 Tail을 못민경우			
			//진짜 데이터가 없는 경우???
			if (tail->_pNext != NULL)
			{
				InterlockedCompareExchangePointer((PVOID*)& _tail, tail->_pNext, tail);
			}
			else if (next != NULL)
			{
				t = next->_data;
				if (InterlockedCompareExchangePointer((PVOID*)& _head, next, head) == head)
				{
					pMemoryPool->Free(head);
					break;
				}
			}

		}
		// InterlockedExchangeAdd(&_size, -1);
		return true;
	}
};

//Head의 Next가 Null이 되는문제
//Tail의 Next가 Tail되는 문제