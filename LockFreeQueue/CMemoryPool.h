
/*---------------------------------------------------------------

	procademy MemoryPool.

	�޸� Ǯ Ŭ���� (������Ʈ Ǯ / ��������Ʈ)
	Ư�� ����Ÿ(����ü,Ŭ����,����)�� ������ �Ҵ� �� ��������.

	- ����.

	procademy::CMemoryPool<DATA> MemPool(300, FALSE);
	DATA *pData = MemPool.Alloc();

	pData ���

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
	// �� �� �տ� ���� ��� ����ü.
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
	// ������, �ı���.
	//
	// Parameters:	(int) �ʱ� �� ����.
	//				(bool) Alloc �� ������ / Free �� �ı��� ȣ�� ����
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
	// �� �ϳ��� �Ҵ�޴´�.  
	//
	// Parameters: ����.
	// Return: (DATA *) ����Ÿ �� ������.
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
	// ������̴� ���� �����Ѵ�.
	//
	// Parameters: (DATA *) �� ������.
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
	// ���� Ȯ�� �� �� ������ ��´�. (�޸�Ǯ ������ ��ü ����)
	//
	// Parameters: ����.
	// Return: (int) �޸� Ǯ ���� ��ü ����
	//////////////////////////////////////////////////////////////////////////
	int		GetAllocCount(void) { return m_iAllocCount; }

	//////////////////////////////////////////////////////////////////////////
	// ���� ������� �� ������ ��´�.
	//
	// Parameters: ����.
	// Return: (int) ������� �� ����.
	//////////////////////////////////////////////////////////////////////////
	int		GetUseCount(void) { return m_iUseCount; }


	// ���� ������� ��ȯ�� (�̻��) ������Ʈ ���� ����.

private:
	st_TOP_NODE* _pFreeNode;

	bool _bPlacementNew;
	__int64 m_iUseCount;
	__int64 m_iAllocCount;
	UINT_PTR m_iUniqeNumber;

	__int64 m_iUniqIndex;
};
