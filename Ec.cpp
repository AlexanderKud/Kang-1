// (c) 2024, RetiredCoder
// License: GPLv3, see "LICENSE.TXT" file
// https://github.com/RetiredC/Kang-1

#include "defs.h"
#include "Ec.h"
#include <intrin.h>
#include <random>


// https://en.bitcoin.it/wiki/Secp256k1
EcInt g_P; //FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE FFFFFC2F
EcInt g_N; //FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE BAAEDCE6 AF48A03B BFD25E8C D0364141
EcPoint g_G; //Generator point

#define P_REV	0x00000001000003D1

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool EcPoint::IsEqual(EcPoint& pnt)
{
	return this->x.IsEqual(pnt.x) && this->y.IsEqual(pnt.y);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// https://en.bitcoin.it/wiki/Secp256k1
void InitEc()
{
	g_P.SetHexStr("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F"); //Fp
	g_G.x.SetHexStr("79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798"); //G.x
	g_G.y.SetHexStr("483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8"); //G.y
	g_N.SetHexStr("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141"); //order of G
};

// https://en.wikipedia.org/wiki/Elliptic_curve_point_multiplication#Point_addition
EcPoint Ec::AddPoints(EcPoint& pnt1, EcPoint& pnt2)
{
	EcPoint res;
	EcInt dx, dy, lambda, lambda2;

	dx = pnt2.x;
	dx.SubModP(pnt1.x);
	dx.InvModP();

	dy = pnt2.y;
	dy.SubModP(pnt1.y);

	lambda = dy;
	lambda.MulModP(dx);
	lambda2 = lambda;
	lambda2.MulModP(lambda);

	res.x = lambda2;
	res.x.SubModP(pnt1.x);
	res.x.SubModP(pnt2.x);

	res.y = pnt2.x;
	res.y.SubModP(res.x);
	res.y.MulModP(lambda);
	res.y.SubModP(pnt2.y);
	return res;
}

// https://en.wikipedia.org/wiki/Elliptic_curve_point_multiplication#Point_doubling
EcPoint Ec::DoublePoint(EcPoint& pnt)
{
	EcPoint res;
	EcInt t1, t2, lambda, lambda2;

	t1 = pnt.y;
	t1.AddModP(pnt.y);
	t1.InvModP();

	t2 = pnt.x;
	t2.MulModP(pnt.x);
	lambda = t2;
	lambda.AddModP(t2);
	lambda.AddModP(t2);
	lambda.MulModP(t1);
	lambda2 = lambda;
	lambda2.MulModP(lambda);

	res.x = lambda2;
	res.x.SubModP(pnt.x);
	res.x.SubModP(pnt.x);

	res.y = pnt.x;
	res.y.SubModP(res.x);
	res.y.MulModP(lambda);
	res.y.SubModP(pnt.y);
	return res;
}

//k up to 256 bits
EcPoint Ec::MultiplyG(EcInt& k)
{
	EcPoint res;
	EcPoint t = g_G;
	bool first = true;
	int n = 3;
	while ((n >= 0) && !k.data[n])
		n--;
	if (n < 0)
		return res; //error
	int index;
	_BitScanReverse64((DWORD*)&index, k.data[n]);
	for (int i = 0; i <= 64 * n + index; i++)
	{
		u8 v = (k.data[i / 64] >> (i % 64)) & 1;	
		if (v)
		{
			if (first)
			{
				first = false;
				res = t;
			}
			else
				res = Ec::AddPoints(res, t);
		}
		t = Ec::DoublePoint(t);
	}
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Mul256_by_64(u64* input, u64 multiplier, u64* result)
{
	u64 h1, h2;
	result[0] = _umul128(input[0], multiplier, &h1);
	u8 carry = _addcarry_u64(0, _umul128(input[1], multiplier, &h2), h1, result + 1);
	carry = _addcarry_u64(carry, _umul128(input[2], multiplier, &h1), h2, result + 2);
	carry = _addcarry_u64(carry, _umul128(input[3], multiplier, &h2), h1, result + 3);
	_addcarry_u64(carry, 0, h2, result + 4);
}

void Mul320_by_64(u64* input, u64 multiplier, u64* result)
{
	u64 h1, h2;
	result[0] = _umul128(input[0], multiplier, &h1);
	u8 carry = _addcarry_u64(0, _umul128(input[1], multiplier, &h2), h1, result + 1);
	carry = _addcarry_u64(carry, _umul128(input[2], multiplier, &h1), h2, result + 2);
	carry = _addcarry_u64(carry, _umul128(input[3], multiplier, &h2), h1, result + 3);
	_addcarry_u64(carry, _umul128(input[4], multiplier, &h1), h2, result + 4);
}

void Add320_to_256(u64* in_out, u64* val)
{
	u8 c = _addcarry_u64(0, in_out[0], val[0], in_out);
	c = _addcarry_u64(c, in_out[1], val[1], in_out + 1);
	c = _addcarry_u64(c, in_out[2], val[2], in_out + 2);
	c = _addcarry_u64(c, in_out[3], val[3], in_out + 3);
	_addcarry_u64(c, 0, val[4], in_out + 4);
}

EcInt::EcInt()
{
	SetZero();
}

void EcInt::Assign(EcInt& val)
{
	memcpy(data, val.data, sizeof(data));
}

void EcInt::Set(u64 val)
{
	SetZero();
	data[0] = val;
}

void EcInt::SetZero()
{
	memset(data, 0, sizeof(data));
}

bool EcInt::SetHexStr(const char* str)
{
	SetZero();
	if (strlen(str) != 64)
		return false;
	for (int i = 0; i < 32; i++)
	{
		int n = 62 - 2 * i;
		char cl = toupper(str[n + 1]);
		char ch = toupper(str[n]);
		if (((cl < '0') || (cl > '9')) && ((cl < 'A') || (cl > 'F')))
			return false;
		if (((ch < '0') || (ch > '9')) && ((ch < 'A') || (ch > 'F')))
			return false;
		u8 l = ((cl >= '0') && (cl <= '9')) ? (cl - '0') : (cl - 'A' + 10);
		u8 h = ((ch >= '0') && (ch <= '9')) ? (ch - '0') : (ch - 'A' + 10);
		((u8*)data)[i] = l + (h << 4);
	}
	return true;
}

//returns carry
bool EcInt::Add(EcInt& val)
{
	u8 c = _addcarry_u64(0, data[0], val.data[0], data + 0);
	c = _addcarry_u64(c, data[1], val.data[1], data + 1);
	c = _addcarry_u64(c, data[2], val.data[2], data + 2);
	c = _addcarry_u64(c, data[3], val.data[3], data + 3);
	return _addcarry_u64(c, data[4], val.data[4], data + 4) != 0;
}

//returns carry
bool EcInt::Sub(EcInt& val)
{
	u8 c = _subborrow_u64(0, data[0], val.data[0], data + 0);
	c = _subborrow_u64(c, data[1], val.data[1], data + 1);
	c = _subborrow_u64(c, data[2], val.data[2], data + 2);
	c = _subborrow_u64(c, data[3], val.data[3], data + 3);
	return _subborrow_u64(c, data[4], val.data[4], data + 4) != 0;
}

void EcInt::Neg()
{
	u8 c = _subborrow_u64(0, 0, data[0], data + 0);
	c = _subborrow_u64(c, 0, data[1], data + 1);
	c = _subborrow_u64(c, 0, data[2], data + 2);
	c = _subborrow_u64(c, 0, data[3], data + 3);
	_subborrow_u64(c, 0, data[4], data + 4);
}

void EcInt::Neg256()
{
	u8 c = _subborrow_u64(0, 0, data[0], data + 0);
	c = _subborrow_u64(c, 0, data[1], data + 1);
	c = _subborrow_u64(c, 0, data[2], data + 2);
	c = _subborrow_u64(c, 0, data[3], data + 3);
	data[4] = 0;
}

bool EcInt::IsLessThan(EcInt& val)
{
	int i = 4;
	while (i >= 0)
	{
		if (data[i] != val.data[i])
			break;
		i--;
	}
	if (i < 0)
		return false;
	return data[i] < val.data[i];
}

bool EcInt::IsEqual(EcInt& val)
{
	return memcmp(val.data, this->data, 40) == 0;
}

void EcInt::AddModP(EcInt& val)
{
	Add(val);
	if (!IsLessThan(g_P))
		Sub(g_P);
}

void EcInt::SubModP(EcInt& val)
{
	if (Sub(val))
		Add(g_P);
}

//assume value < P
void EcInt::NegModP()
{
	Neg();
	Add(g_P);
}

//assume value < N
void EcInt::NegModN()
{
	Neg();
	Add(g_N);
}

//nbits must be less than 64
void EcInt::ShiftRight(int nbits)
{
	data[0] = __shiftright128(data[0], data[1], nbits);
	data[1] = __shiftright128(data[1], data[2], nbits);
	data[2] = __shiftright128(data[2], data[3], nbits);
	data[3] = __shiftright128(data[3], data[4], nbits);
	data[4] = ((i64)data[4]) >> nbits;
}

//nbits must be less than 64
void EcInt::ShiftLeft(int nbits)
{
	data[4] = __shiftleft128(data[3], data[4], nbits);
	data[3] = __shiftleft128(data[2], data[3], nbits);
	data[2] = __shiftleft128(data[1], data[2], nbits);
	data[1] = __shiftleft128(data[0], data[1], nbits);
	data[0] = data[0] << nbits;
}

void EcInt::MulModP(EcInt& val)
{	
	u64 buff[8], tmp[5], h;
	//calc 512 bits
	Mul256_by_64(val.data, data[0], buff);
	Mul256_by_64(val.data, data[1], tmp);
	Add320_to_256(buff + 1, tmp);
	Mul256_by_64(val.data, data[2], tmp);
	Add320_to_256(buff + 2, tmp);
	Mul256_by_64(val.data, data[3], tmp);
	Add320_to_256(buff + 3, tmp);
	//fast mod P
	Mul256_by_64(buff + 4, P_REV, tmp);
	u8 c = _addcarry_u64(0, buff[0], tmp[0], buff);
	c = _addcarry_u64(c, buff[1], tmp[1], buff + 1);
	c = _addcarry_u64(c, buff[2], tmp[2], buff + 2);
	tmp[4] += _addcarry_u64(c, buff[3], tmp[3], buff + 3);
	c = _addcarry_u64(0, buff[0], _umul128(tmp[4], P_REV, &h), data);
	c = _addcarry_u64(c, buff[1], h, data + 1);
	c = _addcarry_u64(c, 0, buff[2], data + 2);
	data[4] = _addcarry_u64(c, buff[3], 0, data + 3);
}

void EcInt::Mul_u64(EcInt& val, u64 multiplier)
{
	Assign(val);
	Mul320_by_64(data, (u64)multiplier, data);
}

void EcInt::Mul_i64(EcInt& val, i64 multiplier)
{
	Assign(val);
	if (multiplier < 0)
	{
		Neg();
		multiplier = -multiplier;
	}
	Mul320_by_64(data, (u64)multiplier, data);
}

#define APPLY_DIV_SHIFT() kbnt -= index; val >>= index; matrix[0] <<= index; matrix[1] <<= index; 
	
// https://tches.iacr.org/index.php/TCHES/article/download/8298/7648/4494
//a bit tricky
void DIV_62(i64& kbnt, i64 modp, i64 val, i64* matrix)
{
	int index, cnt;
	_BitScanForward64((DWORD*)&index, val | 0x4000000000000000);
	APPLY_DIV_SHIFT();
	cnt = 62 - index;
	while (cnt > 0)
	{
		if (kbnt < 0)
		{
			kbnt = -kbnt;
			i64 tmp = -modp; modp = val; val = tmp;
			tmp = -matrix[0]; matrix[0] = matrix[2]; matrix[2] = tmp;
			tmp = -matrix[1]; matrix[1] = matrix[3]; matrix[3] = tmp;
		}
		int thr = cnt;
		if ((kbnt + 1) < cnt)
			thr = (int)(kbnt + 1);
		i64 mul = (-modp * val) & ((UINT64_MAX >> (64 - thr)) & 0x07);
		val += (modp * mul);
		matrix[2] += (matrix[0] * mul);
		matrix[3] += (matrix[1] * mul);
		_BitScanForward64((DWORD*)&index, val | (1ull << cnt));
		APPLY_DIV_SHIFT();
		cnt -= index;
	}
}

void EcInt::InvModP()
{
	i64 matrix[4];
	EcInt result, a, tmp, tmp2;
	EcInt modp, val;
	i64 kbnt = -1;
	matrix[1] = matrix[2] = 0;
	matrix[0] = matrix[3] = 1;	
	DIV_62(kbnt, g_P.data[0], data[0], matrix);
	modp.Mul_i64(g_P, matrix[0]);
	tmp.Mul_i64(*this, matrix[1]);
	modp.Add(tmp);
	modp.ShiftRight(62);
	val.Mul_i64(g_P, matrix[2]);
	tmp.Mul_i64(*this, matrix[3]);
	val.Add(tmp);
	val.ShiftRight(62);
	if (matrix[1] >= 0)
		result.Set(matrix[1]);
	else
	{
		result.Set(-matrix[1]);
		result.Neg();
	}
	if (matrix[3] >= 0)
		a.Set(matrix[3]);
	else
	{ 
		a.Set(-matrix[3]);
		a.Neg();
	}
	Mul320_by_64(g_P.data, (result.data[0] * 0xD838091DD2253531) & 0x3FFFFFFFFFFFFFFF, tmp.data);
	result.Add(tmp);
	result.ShiftRight(62);
	Mul320_by_64(g_P.data, (a.data[0] * 0xD838091DD2253531) & 0x3FFFFFFFFFFFFFFF, tmp.data);
	a.Add(tmp);
	a.ShiftRight(62);
	
	while (val.data[0] || val.data[1] || val.data[2] || val.data[3])
	{
		matrix[1] = matrix[2] = 0;
		matrix[0] = matrix[3] = 1;	
		DIV_62(kbnt, modp.data[0], val.data[0], matrix);
		tmp.Mul_i64(modp, matrix[0]);
		tmp2.Mul_i64(val, matrix[1]);
		tmp.Add(tmp2);
		tmp2.Mul_i64(val, matrix[3]);
		val.Mul_i64(modp, matrix[2]);
		val.Add(tmp2);
		val.ShiftRight(62);
		modp = tmp;
		modp.ShiftRight(62);
		tmp.Mul_i64(result, matrix[0]);
		tmp2.Mul_i64(a, matrix[1]);
		tmp.Add(tmp2);
		tmp2.Mul_i64(a, matrix[3]);
		a.Mul_i64(result, matrix[2]);
		a.Add(tmp2);
		Mul320_by_64(g_P.data, (a.data[0] * 0xD838091DD2253531) & 0x3FFFFFFFFFFFFFFF, tmp2.data);
		a.Add(tmp2);
		a.ShiftRight(62);	
		Mul320_by_64(g_P.data, (tmp.data[0] * 0xD838091DD2253531) & 0x3FFFFFFFFFFFFFFF, tmp2.data);
		result = tmp;
		result.Add(tmp2);
		result.ShiftRight(62);
	}
	Assign(result);
	if (modp.data[4] >> 63)
	{
		Neg();
		modp.Neg();	
	}

	if (modp.data[0] == 1) 
	{
		if (data[4] >> 63)
			Add(g_P);
		if (data[4] >> 63)
			Add(g_P);
		if (!IsLessThan(g_P))
			Sub(g_P);
		if (!IsLessThan(g_P))
			Sub(g_P);
	}
	else
		SetZero(); //error
}

std::mt19937_64 rng;
CRITICAL_SECTION cs_rnd;
bool cs_rnd_inited = false;

void SetRndSeed(u64 seed)
{
	if (!cs_rnd_inited)
	{
		cs_rnd_inited = true;
		InitializeCriticalSection(&cs_rnd);
	}
	rng.seed(seed);
}

void EcInt::RndBits(int nbits)
{
	SetZero();
	if (nbits > 256)
		nbits = 256;
	EnterCriticalSection(&cs_rnd);
	for (int i = 0; i < (nbits + 63) / 64; i++)
		data[i] = rng();
	LeaveCriticalSection(&cs_rnd);
	data[nbits / 64] &= (1ull << (nbits % 64)) - 1;
}

//up to 256 bits only
void EcInt::RndMax(EcInt& max)
{
	SetZero();
	int n = 3;
	while ((n >= 0) && !max.data[n])
		n--;
	if (n < 0)
		return;
	u64 val = max.data[n];
	int k = 0;
	while ((val & 0x8000000000000000) == 0)
	{
		val <<= 1;
		k++;
	}
	int bits = 64 * n + (64 - k);
	RndBits(bits);
	while (!IsLessThan(max)) // :)
		RndBits(bits);
}





