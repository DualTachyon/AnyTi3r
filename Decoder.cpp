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
#include <stdlib.h>
#include <string.h>
#include "BitStream.h"
#include "Decoder.h"
#include "Decoder-CSBK.h"
#include "Decoder-Voice.h"
#include "Decoder-internal.h"
#include "Helpers.h"

static const uint8_t kMagic[3] = { 0x84, 0xA9, 0x61 };

// Private

static bool DecodeDmrCc(Decoder_t *pDecoder, char *pText, size_t TextLength)
{
	if (!BS_PopU8(&pDecoder->Bs, &pDecoder->Cc)) {
		return false;
	}

	sprintf_s(pText, TextLength, "CC%02d", pDecoder->Cc);

	return false;
}

static bool DecodeDigcDataFrame(Decoder_t *pDecoder, char *pText, size_t TextLength)
{
	const char *pType = NULL;
	bool bRet = false;
	bool bBurst;
	uint8_t Type;
	size_t Skip;

	BS_PopUInt(&pDecoder->Bs, 1, &pDecoder->bTs, sizeof(pDecoder->bTs));
	BS_SkipBits(&pDecoder->Bs, 2);
	BS_PopUInt(&pDecoder->Bs, 1, &bBurst, sizeof(bBurst));
	BS_PopUInt(&pDecoder->Bs, 4, &Type, sizeof(Type));

	Skip = sprintf_s(pText, TextLength, "TS%d-C%02d: ", pDecoder->bTs + 1, pDecoder->Cc);
	pText += Skip;
	TextLength -= Skip;

	if (bBurst) {
		strcat_s(pText, TextLength, "Voice Burst: ");
	} else {
		strcat_s(pText, TextLength, "Data Burst: ");
	}

	switch (Type) {
	case 0: pType = "PI Header"; break;
	case 1: bRet = VOICE_Decode(pText, TextLength, pDecoder); break;
	case 2: bRet = TERM_Decode(pText, TextLength, pDecoder); break;
	case 3: bRet = CSBK_Decode(pText, TextLength, pDecoder); break;
	case 4: pType = "MBC Header"; break;
	case 5: pType = "MBC Continuation"; break;
	case 6: pType = "Data Header"; break;
	case 7: pType = "Rate 1/2 Data"; break;
	case 8: pType = "Rate 3/4 Data"; break;
	case 9: pType = "Idle"; break;
	case 10: pType = "Rate 1 Data"; break;
	case 11: pType = "Reserved 11"; break;
	case 12: pType = "Reserved 12"; break;
	case 13: pType = "Reserved 13"; break;
	case 14: pType = "Reserved 14"; break;
	case 15: pType = "Reserved 15"; break;
	}

	if (pType) {
		Skip = BS_GetRemainingBytes(&pDecoder->Bs);
		HEX_Append(pText, TextLength, pType, BS_GetCurrentPtr(&pDecoder->Bs), Skip);
		BS_SkipBytes(&pDecoder->Bs, Skip);
		return true;
	}

	return bRet;
}

static bool DecodeCach(Decoder_t *pDecoder, char *pText, size_t TextLength)
{
	bool bBsSync;
	bool bSlotVerified;
	bool bSlotChanged;
	bool bBusy;
	bool bTs;
	uint8_t Type;

	BS_PopUInt(&pDecoder->Bs, 1, &bBsSync, sizeof(bBsSync));
	BS_PopUInt(&pDecoder->Bs, 1, &bSlotVerified, sizeof(bSlotVerified));
	BS_PopUInt(&pDecoder->Bs, 1, &bSlotChanged, sizeof(bSlotChanged));
	BS_SkipBits(&pDecoder->Bs, 1);
	BS_PopUInt(&pDecoder->Bs, 1, &bBusy, sizeof(bBusy));
	BS_PopUInt(&pDecoder->Bs, 1, &bTs, sizeof(bTs));
	BS_PopUInt(&pDecoder->Bs, 2, &Type, sizeof(Type));

	sprintf_s(pText, TextLength, "CACH: %s Sync", bBsSync ? "BS" : "MS");
	if (bSlotVerified) {
		strcat_s(pText, TextLength, ", Slot Verified");
	}
	if (bSlotChanged) {
		strcat_s(pText, TextLength, ", Slot Changed");
	}
	strcat_s(pText, TextLength, bBusy ? ", Inbound busy" : ", Inbound idle");
	strcat_s(pText, TextLength, bTs ? ", Outbound TS2" : ", Outbound TS1");
	if (bBsSync) {
		switch (Type) {
		case 0: strcat_s(pText, TextLength, ", Single/First fragment"); break;
		case 1: strcat_s(pText, TextLength, ", First fragment"); break;
		case 2: strcat_s(pText, TextLength, ", Last fragment"); break;
		case 3: strcat_s(pText, TextLength, ", Continuing fragment"); break;
		}
	} else {
		switch (Type) {
		case 0: strcat_s(pText, TextLength, ", No TDMA Sync"); break;
		case 1: strcat_s(pText, TextLength, ", TS1 Sync"); break;
		case 2: strcat_s(pText, TextLength, ", TS2 Sync"); break;
		}
	}

	return true;
}

// Public

Decoder_t *DECODER_New(void)
{
	return (Decoder_t *)calloc(1, sizeof(Decoder_t));
}

int DECODER_AddBytes(Decoder_t *pDecoder, const void *pBuffer, size_t Length)
{
	const uint8_t *pBytes = (const uint8_t *)pBuffer;
	size_t Max;
	bool bAdjustRPos = false;

	if (!pDecoder || Length > sizeof(pDecoder->Buffer)) {
		return -1;
	}

	Max = sizeof(pDecoder->Buffer) - pDecoder->WPos;
	if (Length < Max) {
		Max = Length;
		Length = 0;
	} else {
		Length -= Max;
	}
	memcpy(pDecoder->Buffer + pDecoder->WPos, pBytes, Max);
	pBytes += Max;
	pDecoder->Length += Max;
	if (pDecoder->Length > sizeof(pDecoder->Buffer)) {
		pDecoder->Length = sizeof(pDecoder->Buffer);
		bAdjustRPos = true;
	}
	if (Length) {
		memcpy(pDecoder->Buffer, pBytes, Length);
		pDecoder->WPos = Length;
		pDecoder->Length += Length;
	} else {
		pDecoder->WPos = (pDecoder->WPos + Max) % sizeof(pDecoder->Buffer);
	}

	if (bAdjustRPos) {
		pDecoder->RPos = pDecoder->WPos;
		pDecoder->State = 0;
		pDecoder->FPos = 0;
	}

	return 0;
}

bool DECODER_Check(Decoder_t *pDecoder)
{
	uint16_t i;

	for (i = 0; i < pDecoder->Length; i++) {
		switch (pDecoder->State) {
		case 0:
		case 1:
		case 2:
			if (pDecoder->Buffer[pDecoder->RPos] == kMagic[pDecoder->State]) {
				pDecoder->Frame[pDecoder->FPos++] = pDecoder->Buffer[pDecoder->RPos];
				pDecoder->State++;
			} else {
				pDecoder->FPos = 0;
				pDecoder->State = 0;
				if (pDecoder->Buffer[pDecoder->RPos] == kMagic[0]) {
					pDecoder->Frame[pDecoder->FPos++] = pDecoder->Buffer[pDecoder->RPos];
					pDecoder->State++;
				}
			}
			break;

		case 3:
			pDecoder->DataLength = pDecoder->Buffer[pDecoder->RPos] << 8;
			pDecoder->Frame[pDecoder->FPos++] = pDecoder->Buffer[pDecoder->RPos];
			pDecoder->State++;
			break;

		case 4:
			pDecoder->DataLength |= pDecoder->Buffer[pDecoder->RPos];
			pDecoder->Frame[pDecoder->FPos++] = pDecoder->Buffer[pDecoder->RPos];
			if (pDecoder->DataLength % 2) {
				pDecoder->DataLength++;
			}
			if (!pDecoder->DataLength || pDecoder->DataLength + 6 > sizeof(pDecoder->Frame)) {
				pDecoder->State = 0;
				pDecoder->FPos = 0;
			} else {
				pDecoder->State++;
			}
			break;

		case 5:
			pDecoder->Frame[pDecoder->FPos++] = pDecoder->Buffer[pDecoder->RPos];
			pDecoder->State++;
			break;

		default:
			pDecoder->Frame[pDecoder->FPos++] = pDecoder->Buffer[pDecoder->RPos];
			pDecoder->DataLength--;
			break;
		}

		pDecoder->RPos = (pDecoder->RPos + 1) % sizeof(pDecoder->Buffer);

		if (pDecoder->State > 5 && !pDecoder->DataLength) {
			pDecoder->FrameLength = pDecoder->FPos;
			pDecoder->Length -= i + 1;
			pDecoder->State = 0;
			pDecoder->FPos = 0;

			return true;
		}

		if (pDecoder->FPos == sizeof(pDecoder->Frame)) {
			pDecoder->FPos = 0;
		}
	}

	pDecoder->Length = 0;

	return false;
}

bool DECODER_GetText(Decoder_t *pDecoder, bool bSkip, char *pText, size_t TextLength)
{
	uint16_t Length;
	uint8_t PacketType;
	uint8_t Id;
	bool bPrint;

	if (!pDecoder || !pText || !TextLength || !pDecoder->FrameLength) {
		return false;
	}

	BS_Init(&pDecoder->Bs, pDecoder->Frame, pDecoder->FrameLength);

	if (!bSkip) {
		BS_SkipBits(&pDecoder->Bs, 24); // Skip the magic bytes
		BS_PopU16(&pDecoder->Bs, &Length);
		BS_PopU8(&pDecoder->Bs, &PacketType);
	}

	if (!BS_PopU8(&pDecoder->Bs, &Id)) {
		return false;
	}

	pText[0] = 0;

	switch (Id) {
#if 0
	case 0x43:
		bPrint = DecodeDigcDataFrame(pDecoder, pText, TextLength);
		break;

	case 0x77:
		bPrint = DecodeDmrCc(pDecoder, pText, TextLength);
		break;

	case 0x7F:
		bPrint = DecodeCach(pDecoder, pText, TextLength);
		break;
#endif

	default:
#if 1 // Skip commands we currently don't care about
		sprintf_s(pText, TextLength, "Frame %02X", Id);
		HEX_Append(pText, TextLength, "", pDecoder->Frame, pDecoder->FrameLength);
		bPrint = true;
#else
		bPrint = false;
#endif
		BS_SkipBytes(&pDecoder->Bs, BS_GetRemainingBytes(&pDecoder->Bs));
		break;
	}

	pDecoder->FrameLength = BS_GetRemainingBytes(&pDecoder->Bs);
	// Skip the potential padding byte
	if (pDecoder->FrameLength <= 1) {
		pDecoder->FrameLength = 0;
	} else {
		memcpy(pDecoder->Frame, BS_GetCurrentPtr(&pDecoder->Bs), pDecoder->FrameLength);
	}

	return bPrint;
}

size_t DECODER_GetFrameLength(Decoder_t *pDecoder)
{
	return pDecoder->FrameLength;
}
