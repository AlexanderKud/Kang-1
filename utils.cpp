// (c) 2024, RetiredCoder
// License: GPLv3, see "LICENSE.TXT" file
// https://github.com/RetiredC/Kang-1

#include "utils.h"
#include <process.h>
#include <wchar.h>


TMemPool::TMemPool()
{
	MemBlockSize = 2 * 1024 * 1024;
	pnt = 0;
}

TMemPool::~TMemPool()
{
	Clear();
}

void TMemPool::Init(int _MemBlockSize)
{
	if (_MemBlockSize)
		MemBlockSize = _MemBlockSize;
}

void TMemPool::Clear()
{
	int cnt = (int)mems.size();
	for (int i = 0; i < cnt; i++)
	{
		free(mems[i]);
	}
	mems.clear();
	pnt = 0;
}

BYTE* TMemPool::GetMem(size_t size)
{
	BYTE* mem;
	if (mems.empty() || (pnt + size > MemBlockSize))
	{
		size_t max = MemBlockSize > size ? MemBlockSize : size;
		mem = (BYTE*)malloc(max);
		mems.push_back(mem);
		pnt = 0;
	}
	mem = mems[mems.size() - 1] + pnt;
	pnt += size;
	return mem;
}

///////////////////////////////////////////////////////////////////////////////

#define INIT_LIST_SIZE			16
#define	GROW_LIST_SIZE_MIN		16

TFastBase::TFastBase()
{
	memset(lists, 0, sizeof(lists));
	IsEmpty = true;
	FindSize = 20 - 2;
}

TFastBase::~TFastBase()
{
	Clear(true);
}

void TFastBase::Init(int _FindSize, int _BaseRecSize, int _AdvRecOffset, int _AdvRecSize)
{
	FindSize = _FindSize - 2;
	BaseRecSize = _BaseRecSize;
	AdvRecOffset = _AdvRecOffset;
	AdvRecSize = _AdvRecSize;
	mempool.Init(0);
}

void TFastBase::Clear(bool all)
{
	if (!all && IsEmpty)
		return;

	for (int i = 0; i < 256; i++)
		for (int j = 0; j < 256; j++)
		{
			if (lists[i][j].data)
				free(lists[i][j].data);
			lists[i][j].data = NULL;
			lists[i][j].capacity = 0;
			lists[i][j].cnt = 0;
		}	
	mempool.Clear();
	IsEmpty = true;
}

int TFastBase::GetBlockCnt()
{
	if (IsEmpty)
		return 0;

	int blockCount = 0;
	for (int i = 0; i < 256; i++)
		for (int j = 0; j < 256; j++)
			blockCount += lists[i][j].cnt;
	return blockCount;
}

// http://en.cppreference.com/w/cpp/algorithm/lower_bound
int TFastBase::lower_bound(TListRec* list, BYTE* data)
{
	int count = list->cnt;
	int it, first, step;
	first = 0;
	while (count > 0)
	{
		it = first;
		step = count / 2;
		it += step;
		if (memcmp(((BYTE*)list->data[it]) + 2, data + 2, FindSize) < 0) 
		{
			first = ++it;
			count -= step + 1;
		}
		else
			count = step;
	}
	return first;
}

BYTE* TFastBase::AddDataBlock(BYTE* data, int datalen, int pos)
{
	BYTE* p = mempool.GetMem(datalen);
	memcpy(p, data, datalen);

	TListRec* list = &lists[data[0]][data[1]];
	unsigned int cap = list->capacity;
	if (list->cnt >= cap)
	{
		int grow = cap / 4;
		if (grow < GROW_LIST_SIZE_MIN)
			grow = GROW_LIST_SIZE_MIN;
		int newcap = cap + grow;
		list->data = (void**)realloc(list->data, newcap * sizeof(void*));
		list->capacity = newcap;
	}
	int first = (pos < 0) ? lower_bound(list, p) : pos;
	memmove(&list->data[first + 1], &list->data[first], (list->cnt - first) * sizeof(void*));
	list->data[first] = p;
	list->cnt++;
	IsEmpty = false;
	return p;
}

BYTE* TFastBase::FindDataBlock(BYTE* data)
{
	bool res = false;
	TListRec* list = &lists[data[0]][data[1]];
	int first = lower_bound(list, data);
	if (first == list->cnt)
		return NULL;
	if (memcmp(((BYTE*)list->data[first]) + 2, data + 2, FindSize))
		return NULL;
	return (BYTE*)list->data[first];
}

BYTE* TFastBase::FindOrAddDataBlock(BYTE* data, int datalen)
{
	TListRec* list = &lists[data[0]][data[1]];
	int first = lower_bound(list, data);
	if (first == list->cnt)
		goto label_not_found;
	if (memcmp(((BYTE*)list->data[first]) + 2, data + 2, FindSize))
		goto label_not_found;
	return (BYTE*)list->data[first];
label_not_found:
	AddDataBlock(data, datalen, first);
	return NULL;
}

