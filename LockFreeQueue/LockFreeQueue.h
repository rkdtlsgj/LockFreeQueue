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
	long _size;
	Node* _head;        // 시작노드를 포인트한다.
	Node* _tail;        // 마지막노드를 포인트한다.
	CMemoryPool<Node>* pMemoryPool;
public:
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
		//Node* node = new Node;
		node->_data = t;
		node->_pNext = NULL;

		/*Node* tail = NULL;
		Node* next = NULL;*/

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

		InterlockedIncrement(&_size);// InterlockedExchangeAdd(&_size, 1);
	}

	int Dequeue(T& t)
	{
		if (InterlockedDecrement(&_size) < 0)
		{
			InterlockedIncrement(&_size);
			return false;
		}

	/*	Node* head = NULL;
		Node* next = NULL;
*/
		while (true)
		{
			Node* head = _head;
			Node* next = head->_pNext;		
			Node* tail = _tail;
			Node* tNext = _tail->_pNext;
			long size = _size;

			/*if (next == NULL)
			{
				return false;
			}*/
			if (head == tail)
			{
				if (tail->_pNext != NULL)
				{
					InterlockedCompareExchangePointer((PVOID*)& _tail, tail->_pNext, tail);
				}
			}
			else
			{
				if (next != NULL)
				{					
					t = next->_data;
					if (InterlockedCompareExchangePointer((PVOID*)& _head, next, head) == head)
					{						
						//delete head;
						pMemoryPool->Free(head);
						break;
					}
				}
			}
		}
		// InterlockedExchangeAdd(&_size, -1);
		return true;
	}
};