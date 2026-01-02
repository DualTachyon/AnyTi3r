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

#include <stdio.h>
#include <string.h>
#include "BitStream.h"
#include "Decoder-Internal.h"
#include "Decoder-Voice.h"
#include "Helpers.h"

static bool DecodeGroup(char *pText, size_t TextLength, bool bTs, const char *pType, BitStream_t *pBs)
{
	uint8_t Options;
	uint32_t Ta, Sa;

	BS_PopU8(pBs, &Options);
	BS_PopUInt(pBs, 24, &Ta, sizeof(Ta));
	BS_PopUInt(pBs, 24, &Sa, sizeof(Sa));

	sprintf_s(pText, TextLength, "TS%d Group call %sfrom %d to %d", bTs + 1, pType, Sa, Ta);

	return true;
}

static bool DecodePrivate(char *pText, size_t TextLength, bool bTs, const char *pType, BitStream_t *pBs)
{
	uint8_t Options;
	uint32_t Ta, Sa;

	BS_PopU8(pBs, &Options);
	BS_PopUInt(pBs, 24, &Ta, sizeof(Ta));
	BS_PopUInt(pBs, 24, &Sa, sizeof(Sa));

	sprintf_s(pText, TextLength, "TS%d Private call %sfrom %d to %d", bTs + 1, pType, Sa, Ta);

	return true;
}

static bool DecodeTalker(char *pText, size_t TextLength, bool bTs, uint8_t Type, BitStream_t *pBs)
{
	static uint8_t Previous[2] = { 0xFF, 0xFF };
	static uint8_t Format[2];
	static uint8_t Bits[2];
	static uint8_t Length[2];
	static uint8_t Index[2];
	static char Alias[2][64];

	switch (Type) {
	case 4:
		Index[bTs] = 0;
		BS_PopUInt(pBs, 2, Format + bTs, sizeof(Format[bTs]));
		BS_PopUInt(pBs, 5, Length + bTs, sizeof(Length[bTs]));
		if (Format[bTs] == 3) {
			Previous[bTs] = 0xFF;
			strcat_s(pText, TextLength, "UTF-16 not yet supported!");
			return true;
		}
		Bits[bTs] = (Format[bTs] == 0) ? 7 : 8;
		if (Bits[bTs] == 8) {
			BS_SkipBits(pBs, 1);
		}
		Length[bTs] *= Bits[bTs];
		if (!Length[bTs]) {
			Type = 0xFF;
			break;
		}
		while (Length[bTs] >= Bits[bTs] && !BS_Eof(pBs)) {
			if (BS_PopBits(pBs, Bits[bTs], Alias[bTs] + Index[bTs], 1)) {
				Index[bTs]++;
				Length[bTs] -= Bits[bTs];
			}
		}
		Alias[bTs][Index[bTs]] = 0;
		break;

	case 5:
	case 6:
	case 7:
		if ((Previous[bTs] + 1) == Type) {
			if (!Length[bTs] || !Bits[bTs]) {
				Type = 0xFF;
				break;
			}
			while (Length[bTs] >= Bits[bTs] && !BS_Eof(pBs)) {
				if (BS_PopBits(pBs, Bits[bTs], Alias[bTs] + Index[bTs], 1)) {
					Index[bTs]++;
					Length[bTs] -= Bits[bTs];
				}
			}
			Alias[bTs][Index[bTs]] = 0;
		}
		break;
	}
	Previous[bTs] = Type;
	if (!Length[bTs] && Index[bTs]) {
		sprintf_s(pText, TextLength, "TS%d TA(%d): %s", bTs + 1, Format[bTs], Alias[bTs]);
		Index[bTs] = 0;
		Previous[bTs] = 0xFF;

		return true;
	}

	return false;
}

bool VOICE_Decode(char *pText, size_t TextLength, Decoder_t *pDecoder)
{
	uint8_t Private;
	uint8_t R;
	uint8_t Opcode;
	uint8_t Fid;
	const uint8_t *pData = BS_GetCurrentPtr(&pDecoder->Bs);
	size_t Length;

	BS_PopUInt(&pDecoder->Bs, 8, &Length, sizeof(Length));
	if (Length < 9) {
		strcat_s(pText, TextLength, "Incomplete Voice LC Header!");
		return true;
	}
	if (Length > 9) {
		Length = 9;
	}

	BS_PopUInt(&pDecoder->Bs, 1, &Private, sizeof(Private));
	BS_PopUInt(&pDecoder->Bs, 1, &R, sizeof(R));
	BS_PopUInt(&pDecoder->Bs, 6, &Opcode, sizeof(Opcode));
	BS_PopU8(&pDecoder->Bs, &Fid);

	switch (Opcode) {
	case 0: return DecodeGroup(pText, TextLength, pDecoder->bTs, "", &pDecoder->Bs);
	case 3: return DecodePrivate(pText, TextLength, pDecoder->bTs, "", &pDecoder->Bs);
	case 4: case 5: case 6: case 7:
		return DecodeTalker(pText, TextLength, pDecoder->bTs, Opcode, &pDecoder->Bs);
	default:
		HEX_Append(pText, TextLength, "VOICE_LC:", pData, Length);
		BS_SkipBytes(&pDecoder->Bs, Length);
		return true;
	}
}

bool TERM_Decode(char *pText, size_t TextLength, Decoder_t *pDecoder)
{
	uint8_t Private;
	uint8_t R;
	uint8_t Opcode;
	uint8_t Fid;
	const uint8_t *pData = BS_GetCurrentPtr(&pDecoder->Bs);
	size_t Length;

	BS_PopUInt(&pDecoder->Bs, 8, &Length, sizeof(Length));
	if (Length < 9) {
		strcat_s(pText, TextLength, "Incomplete Term LC Header!");
		return true;
	}
	if (Length > 9) {
		Length = 9;
	}

	BS_PopUInt(&pDecoder->Bs, 1, &Private, sizeof(Private));
	BS_PopUInt(&pDecoder->Bs, 1, &R, sizeof(R));
	BS_PopUInt(&pDecoder->Bs, 6, &Opcode, sizeof(Opcode));
	BS_PopU8(&pDecoder->Bs, &Fid);

	switch (Opcode) {
	case 0: return DecodeGroup(pText, TextLength, pDecoder->bTs, "ended ", &pDecoder->Bs);
	case 3: return DecodePrivate(pText, TextLength, pDecoder->bTs, "ended ", &pDecoder->Bs);
	default:
		HEX_Append(pText, TextLength, "TERM_LC:", pData, Length);
		BS_SkipBytes(&pDecoder->Bs, Length);
		return true;
	}
}
