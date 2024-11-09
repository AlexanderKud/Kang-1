// (c) 2024, RetiredCoder
// License: GPLv3, see "LICENSE.TXT" file
// https://github.com/RetiredC/Kang-1

#pragma once

#include <Windows.h>
#include <stdio.h>
#include <vector>
#include "defs.h"

class TMemPool
{
private:
	std::vector <BYTE*> mems;
	int MemBlockSize;
	size_t pnt;
public:
	TMemPool();
	~TMemPool();
	BYTE* GetMem(size_t size);
	void Init(int _MemBlockSize);
	void Clear();
};

struct TListRec
{
	unsigned int cnt;
	unsigned int capacity;
	void** data;
};

class TFastBase
{
private:
	TMemPool mempool;
	int FindSize;
	TListRec lists[256][256];

	int BaseRecSize;
	int AdvRecOffset;
	int AdvRecSize;

	int lower_bound(TListRec* list, BYTE* data);
	bool IsEmpty;
public:
	TFastBase();
	~TFastBase();

	void Init(int _FindSize, int _BaseRecSize, int _AdvRecOffset, int _AdvRecSize);
	void Clear(bool all);

	BYTE* AddDataBlock(BYTE* data, int datalen, int pos = -1);
	BYTE* FindDataBlock(BYTE* data);
	BYTE* FindOrAddDataBlock(BYTE* data, int datalen);
	int GetBlockCnt();
};

