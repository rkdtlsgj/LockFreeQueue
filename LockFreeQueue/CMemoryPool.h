
/*---------------------------------------------------------------

	procademy MemoryPool.

	메모리 풀 클래스 (오브젝트 풀 / 프리리스트)
	특정 데이타(구조체,클래스,변수)를 일정량 할당 후 나눠쓴다.

	- 사용법.

	procademy::CMemoryPool<DATA> MemPool(300, FALSE);
	DATA *pData = MemPool.Alloc();

	pData 사용

	MemPool.Free(pData);


----------------------------------------------------------------*/
#pragma once
#include <new.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <winnt.h>

template <class DATA>
class CMemoryPool
{
private:

	/* **************************************************************** */
	// 각 블럭 앞에 사용될 노드 구조체.
	/* **************************************************************** */
	struct st_BLOCK_NODE
	{
		st_BLOCK_NODE()
		{
			stpNextBlock = NULL;
		}

		DATA cData;
		UINT_PTR iUniqeNumber;
		st_BLOCK_NODE* stpNextBlock;		
	};

	struct st_TOP_NODE
	{
		st_BLOCK_NODE* _pNode;
		__int64 iUniqIndex;
	};


public:

	//////////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Parameters:	(int) 초기 블럭 개수.
	//				(bool) Alloc 시 생성자 / Free 시 파괴자 호출 여부
	// Return:
	//////////////////////////////////////////////////////////////////////////
	CMemoryPool(int iBlockNum, bool bPlacementNew = false)
	{
		m_iAllocCount = iBlockNum;
		_bPlacementNew = bPlacementNew;
		m_iUseCount = 0;
		m_iUniqIndex = 0;

		m_iUniqeNumber = (UINT_PTR)this;

		st_BLOCK_NODE* temp;

		_pFreeNode = (st_TOP_NODE*)_aligned_malloc(sizeof(st_TOP_NODE), 16);  //(st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
		_pFreeNode->iUniqIndex = 0;
		_pFreeNode->_pNode = NULL;

		if (_bPlacementNew == true)
		{			
			for (int i = 0; i < m_iAllocCount; i++)
			{
				temp = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
				temp->iUniqeNumber = m_iUniqeNumber;
				temp->stpNextBlock = _pFreeNode->_pNode;
				_pFreeNode->_pNode = temp;
		
			}
		}
		else
		{

			for (int i = 0; i < m_iAllocCount; i++)
			{
				temp = new st_BLOCK_NODE;
				temp->iUniqeNumber = m_iUniqeNumber;
				temp->stpNextBlock = _pFreeNode->_pNode;
				_pFreeNode->_pNode = temp;

			}
		}

	}

	virtual ~CMemoryPool()
	{		

		st_BLOCK_NODE* temp = _pFreeNode->_pNode;

		if (_bPlacementNew == true)
		{
			while (temp != NULL)
			{
				_pFreeNode->_pNode = _pFreeNode->_pNode->stpNextBlock;

				temp->cData.~DATA();
				free(temp);

				temp = _pFreeNode->_pNode;
			}

		}
		else
		{
			while (temp != NULL)
			{
				_pFreeNode->_pNode = _pFreeNode->_pNode->stpNextBlock;
				delete temp;
				temp = _pFreeNode->_pNode;
				InterlockedDecrement64(&m_iAllocCount);
			}
		}

		_aligned_free(_pFreeNode);
	}


	//////////////////////////////////////////////////////////////////////////
	// 블럭 하나를 할당받는다.  
	//
	// Parameters: 없음.
	// Return: (DATA *) 데이타 블럭 포인터.
	//////////////////////////////////////////////////////////////////////////
	DATA* Alloc()
	{
		st_BLOCK_NODE* newBlock = NULL; 
		st_BLOCK_NODE* pNext;


		__int64 allocNumber = InterlockedIncrement64(&m_iUniqIndex);
		st_TOP_NODE temp;
		do
		{
			temp.iUniqIndex = _pFreeNode->iUniqIndex;
			temp._pNode = _pFreeNode->_pNode;

			if (temp._pNode == NULL)
			{
				newBlock = new st_BLOCK_NODE;
				newBlock->iUniqeNumber = m_iUniqeNumber;
				newBlock->stpNextBlock = NULL;

				InterlockedIncrement64(&m_iAllocCount);
				break;
			}

		} while (InterlockedCompareExchange128((volatile LONG64*)_pFreeNode, allocNumber, (LONG64)temp._pNode->stpNextBlock, (LONG64*)& temp) == false);


		if(newBlock == NULL)
			newBlock = temp._pNode;

		if (_bPlacementNew == true)
		{
			new (&newBlock->cData) DATA();
		}


		InterlockedIncrement64(&m_iUseCount);

		return &newBlock->cData;

	}

	//////////////////////////////////////////////////////////////////////////
	// 사용중이던 블럭을 해제한다.
	//
	// Parameters: (DATA *) 블럭 포인터.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool	Free(DATA* pData)
	{
		st_BLOCK_NODE* block = (st_BLOCK_NODE*)pData;	
		UINT_PTR iUniqeNumber = block->iUniqeNumber;

	
		if (iUniqeNumber == m_iUniqeNumber)
		{
			__int64 freeNumber = InterlockedIncrement64(&m_iUniqIndex);

			st_TOP_NODE temp;
			do
			{
				temp.iUniqIndex = _pFreeNode->iUniqIndex;
				temp._pNode = _pFreeNode->_pNode;


				block = (st_BLOCK_NODE*)pData;
				block->stpNextBlock = _pFreeNode->_pNode;


			} while (InterlockedCompareExchange128((volatile LONG64*)_pFreeNode, freeNumber, (LONG64)block, (LONG64*)& temp) == false);


			if (_bPlacementNew == true)
			{
				_pFreeNode->_pNode->cData.~DATA();
			}

			InterlockedDecrement64(&m_iUseCount);

			return true;

		}
		else
		{
			return false;
		}
	}


	//////////////////////////////////////////////////////////////////////////
	// 현재 확보 된 블럭 개수를 얻는다. (메모리풀 내부의 전체 개수)
	//
	// Parameters: 없음.
	// Return: (int) 메모리 풀 내부 전체 개수
	//////////////////////////////////////////////////////////////////////////
	int		GetAllocCount(void) { return m_iAllocCount; }

	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 블럭 개수를 얻는다.
	//
	// Parameters: 없음.
	// Return: (int) 사용중인 블럭 개수.
	//////////////////////////////////////////////////////////////////////////
	int		GetUseCount(void) { return m_iUseCount; }


	// 스택 방식으로 반환된 (미사용) 오브젝트 블럭을 관리.

private:
	st_TOP_NODE* _pFreeNode;

	bool _bPlacementNew;
	__int64 m_iUseCount;
	__int64 m_iAllocCount;
	UINT_PTR m_iUniqeNumber;

	__int64 m_iUniqIndex;
};
