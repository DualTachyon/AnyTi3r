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

#ifndef HELPERS_BITSTREAM_H
#define HELPERS_BITSTREAM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct BitStream_t {
	const uint8_t *pStream;
	size_t Length;
	size_t Position;
} BitStream_t;

void BS_Init(BitStream_t *pBs, const uint8_t *pStream, size_t Length);
bool BS_PopBits(BitStream_t *pBs, size_t Bits, void *pBuffer, size_t Length);
bool BS_PopBytes(BitStream_t *pBs, size_t Bytes, void *pBuffer, size_t Length);
bool BS_PopUInt(BitStream_t *pBs, size_t Bits, void *pBuffer, size_t Length);
bool BS_PopU8(BitStream_t *pBs, uint8_t *pValue);
bool BS_PopU16(BitStream_t *pBs, uint16_t *pValue);
bool BS_PopU32(BitStream_t *pBs, uint32_t *pValue);
bool BS_PopU64(BitStream_t *pBs, uint64_t *pValue);
bool BS_SkipBits(BitStream_t *pBs, size_t Bits);
bool BS_SkipBytes(BitStream_t *pBs, size_t Bytes);
bool BS_GetSubStream(BitStream_t *pBs, BitStream_t *pOut, size_t Length);
bool BS_Need(const BitStream_t *pBs, size_t Bits);
bool BS_Eof(const BitStream_t *pBs);
void BS_Shrink(BitStream_t *pBs, size_t Bits);
size_t BS_GetRemainingBytes(const BitStream_t *pBs);
size_t BS_AdjustLengthBytes(const BitStream_t *pBs, size_t Length);
size_t BS_GetConsumedBytes(const BitStream_t *pBs);
const uint8_t *BS_GetCurrentPtr(const BitStream_t *pBs);

#endif
