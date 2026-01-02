/* Copyright 2026 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include <string.h>

#include "BitStream.h"

#define GEN_DOWN_SHIFT(x)	(8U - (x))
#define GEN_MASK(x)		((1U << (x)) - 1U)

static uint8_t GetBits(BitStream_t *pBs, size_t Bits)
{
	size_t Index = pBs->Position / 8U;
	const size_t BitPos = pBs->Position % 8U;
	uint32_t Mask;
	size_t DownShift;
	size_t UpShift;
	const size_t LeftCount = GEN_DOWN_SHIFT(BitPos);
	uint8_t Value;

	// Check if the bits are contained within a single byte
	if (BitPos + Bits <= 8U) {
		Mask = GEN_MASK(Bits);
		DownShift = GEN_DOWN_SHIFT(BitPos) - Bits;

		pBs->Position += Bits;

		return (pBs->pStream[Index] >> DownShift) & Mask;
	}

	Mask = GEN_MASK(LeftCount);
	DownShift = GEN_DOWN_SHIFT(BitPos) - LeftCount;
	UpShift = Bits - LeftCount;

	pBs->Position += LeftCount;
	Bits -= LeftCount;

	Value = (pBs->pStream[Index++] >> DownShift) & Mask;
	Value <<= UpShift;

	Mask = GEN_MASK(Bits);
	DownShift = GEN_DOWN_SHIFT(Bits);
	Value |= (pBs->pStream[Index] >> DownShift) & Mask;

	pBs->Position += Bits;

	return Value;
}

void BS_Init(BitStream_t *pBs, const uint8_t *pStream, size_t Length)
{
	memset(pBs, 0, sizeof(*pBs));
	pBs->pStream = pStream;
	pBs->Length = Length * 8U;
}

bool BS_PopBits(BitStream_t *pBs, size_t Bits, void *pBuffer, size_t Length)
{
	uint8_t *pBytes = (uint8_t *)pBuffer;
	const uint8_t Count = Bits & 7U;

	if (!pBs || !Bits || !pBuffer) {
		return false;
	}
	if (Bits > Length * 8U) {
		return false;
	}
	if (pBs->Position + Bits > pBs->Length) {
		return false;
	}

	memset(pBuffer, 0, Length);

	if (Count > 0U) {
		*pBytes++ = GetBits(pBs, Count);
		Bits -= Count;
	}

	while (Bits > 0) {
		*pBytes++ = GetBits(pBs, 8U);
		Bits -= 8U;
	}

	return true;
}

bool BS_PopBytes(BitStream_t *pBs, size_t Bytes, void *pBuffer, size_t Length)
{
	return BS_PopBits(pBs, Bytes * 8U, pBuffer, Length);
}

bool BS_PopUInt(BitStream_t *pBs, size_t Bits, void *pBuffer, size_t Length)
{
	uint8_t *pBytes = (uint8_t *)pBuffer;
	const uint8_t Count = Bits & 7U;

	if (!pBs || !Bits || !pBuffer) {
		return false;
	}
	if (Bits > Length * 8U) {
		return false;
	}
	if (pBs->Position + Bits > pBs->Length) {
		return false;
	}

	memset(pBuffer, 0, Length);

	pBytes += (Bits - 1U) / 8U;
	if (Count > 0U) {
		*pBytes-- = GetBits(pBs, Count);
		Bits -= Count;
	}

	while (Bits > 0U) {
		*pBytes-- = GetBits(pBs, 8U);
		Bits -= 8U;
	}

	return true;
}

bool BS_PopU8(BitStream_t *pBs, uint8_t *pValue)
{
	return BS_PopBits(pBs, 8U, pValue, sizeof(uint8_t));
}

bool BS_PopU16(BitStream_t *pBs, uint16_t *pValue)
{
	uint8_t Array[2];
	bool Success;

	Success = BS_PopBits(pBs, 16U, Array, sizeof(Array));

	if (Success) {
		*pValue = (uint16_t)((Array[0] << 8) | Array[1]);
	}

	return Success;
}

bool BS_PopU32(BitStream_t *pBs, uint32_t *pValue)
{
	uint8_t Array[4];
	bool Success;

	Success = BS_PopBits(pBs, 32U, Array, sizeof(Array));

	if (Success) {
		*pValue = (uint32_t)((Array[0] << 24) | (Array[1] << 16) | (Array[2] << 8) | Array[3]);
	}

	return Success;
}

bool BS_PopU64(BitStream_t *pBs, uint64_t *pValue)
{
	uint8_t Array[8];
	bool Success;

	Success = BS_PopBits(pBs, 64U, Array, sizeof(Array));

	if (Success) {
		*pValue = ((uint64_t)Array[0] << 56) | ((uint64_t)Array[1] << 48) | ((uint64_t)Array[2] << 40) | ((uint64_t)Array[3] << 32) | ((uint64_t)Array[4] << 24) | ((uint64_t)Array[5] << 16) | ((uint64_t)Array[6] << 8) | (uint64_t)Array[7];
	}

	return Success;
}

bool BS_SkipBits(BitStream_t *pBs, size_t Bits)
{
	if (pBs->Position + Bits > pBs->Length) {
		return false;
	} else {
		pBs->Position += Bits;
		return true;
	}
}

bool BS_SkipBytes(BitStream_t *pBs, size_t Bytes)
{
	return BS_SkipBits(pBs, Bytes * 8U);
}

bool BS_GetSubStream(BitStream_t *pBs, BitStream_t *pOut, size_t Length)
{
	if (!Length) {
		return true;
	}

	if (pBs->Position == pBs->Length) {
		return false;
	}

	Length *= 8U;

	if (pBs->Position + Length > pBs->Length) {
		Length = pBs->Length - pBs->Position;
	}

	memset(pOut, 0, sizeof(*pOut));
	pOut->pStream = pBs->pStream + (pBs->Position / 8U);
	pOut->Length = Length + (pBs->Position & 7U);
	pOut->Position = pBs->Position & 7U;

	pBs->Position += Length;

	return true;
}

bool BS_Need(const BitStream_t *pBs, size_t Bits)
{
	if (!pBs || pBs->Position + Bits > pBs->Length) {
		return false;
	}

	return true;
}

bool BS_Eof(const BitStream_t *pBs)
{
	if (!pBs || pBs->Position == pBs->Length) {
		return true;
	}

	return false;
}

void BS_Shrink(BitStream_t *pBs, size_t Bits)
{
	if (pBs->Length > Bits) {
		pBs->Length -= Bits;
	} else {
		pBs->Length = 0U;
	}
}

size_t BS_GetRemainingBytes(const BitStream_t *pBs)
{
	return (pBs->Length - pBs->Position) / 8U;
}

size_t BS_AdjustLengthBytes(const BitStream_t *pBs, size_t Length)
{
	const size_t RemainingLength = BS_GetRemainingBytes(pBs);

	if (Length > RemainingLength) {
		return RemainingLength;
	}

	return Length;
}

size_t BS_GetConsumedBytes(const BitStream_t *pBs)
{
	return (pBs->Position + 7U) / 8U;
}

const uint8_t *BS_GetCurrentPtr(const BitStream_t *pBs)
{
	return pBs->pStream + BS_GetConsumedBytes(pBs);
}

