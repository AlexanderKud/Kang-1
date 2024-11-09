// (c) 2024, RetiredCoder
// License: GPLv3, see "LICENSE.TXT" file
// https://github.com/RetiredC/Kang-1

#pragma once

#include "defs.h"
#include <Windows.h>

class EcInt
{
public:
	EcInt();

	void Assign(EcInt& val);
	void Set(u64 val);
	void SetZero();
	bool SetHexStr(const char* str);

	bool Add(EcInt& val); //returns true if carry
	bool Sub(EcInt& val); //returns true if carry
	void Neg();
	void Neg256();
	void ShiftRight(int nbits);
	void ShiftLeft(int nbits);
	bool IsLessThan(EcInt& val);
	bool IsEqual(EcInt& val);

	void Mul_u64(EcInt& val, u64 multiplier);
	void Mul_i64(EcInt& val, i64 multiplier);

	void AddModP(EcInt& val);
	void SubModP(EcInt& val);
	void NegModP();
	void NegModN();
	void MulModP(EcInt& val);
	void InvModP();

	void RndBits(int nbits);
	void RndMax(EcInt& max);

	u64 data[4 + 1];
};

class EcPoint
{
public:
	bool IsEqual(EcPoint& pnt);
	EcInt x;
	EcInt y;
};

class Ec
{
public:
	static EcPoint AddPoints(EcPoint& pnt1, EcPoint& pnt2);
	static EcPoint DoublePoint(EcPoint& pnt);
	static EcPoint MultiplyG(EcInt& k);
};

void InitEc();
void SetRndSeed(u64 seed);