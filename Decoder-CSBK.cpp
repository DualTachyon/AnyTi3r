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
#include "Decoder-CSBK.h"
#include "Decoder-internal.h"
#include "Helpers.h"

static bool DecodeAloha(char *pText, size_t TextLength, BitStream_t *pBs)
{
	bool bTsccas;
	bool bSync;
	uint8_t Version;
	bool bOffset;
	bool bActive;
	uint8_t Mask;
	uint8_t Service;
	uint8_t NRand;
	bool bReg;
	uint8_t Backoff;
	uint16_t Code;
	uint32_t MsAddress;

	BS_PopUInt(pBs, 1, &bTsccas, sizeof(bTsccas));
	BS_PopUInt(pBs, 1, &bSync, sizeof(bSync));
	BS_PopUInt(pBs, 3, &Version, sizeof(Version));
	BS_PopUInt(pBs, 1, &bOffset, sizeof(bOffset));
	BS_PopUInt(pBs, 1, &bActive, sizeof(bActive));
	BS_PopUInt(pBs, 5, &Mask, sizeof(Mask));
	BS_PopUInt(pBs, 2, &Service, sizeof(Service));
	BS_PopUInt(pBs, 4, &NRand, sizeof(NRand));
	BS_PopUInt(pBs, 1, &bReg, sizeof(bReg));
	BS_PopUInt(pBs, 4, &Backoff, sizeof(Backoff));
	BS_PopUInt(pBs, 16, &Code, sizeof(Code));
	if (!BS_PopUInt(pBs, 24, &MsAddress, sizeof(MsAddress))) {
		strcat_s(pText, TextLength, "Aloha: Incomplete CSBK!");
		return true;
	}

	pText[0] = 0;
	//sprintf_s(pText, TextLength, "Aloha: Code %d MS Address %d", Code, MsAddress);

	return false; // This CSBK is too noisy
}

static bool DecodePvGrant(char *pText, size_t TextLength, BitStream_t *pBs)
{
	uint16_t Lpcn;
	uint8_t Lcn;
	bool bEmergency;
	bool bOffset;
	uint32_t Ta, Sa;

	BS_PopUInt(pBs, 12, &Lpcn, sizeof(Lpcn));
	BS_PopUInt(pBs, 1, &Lcn, sizeof(Lcn));
	BS_SkipBits(pBs, 1); // Reserved
	BS_PopUInt(pBs, 1, &bEmergency, sizeof(bEmergency));
	BS_PopUInt(pBs, 1, &bOffset, sizeof(bOffset));
	BS_PopUInt(pBs, 24, &Ta, sizeof(Ta));
	if (!BS_PopUInt(pBs, 24, &Sa, sizeof(Sa))) {
		strcat_s(pText, TextLength, "Private Voice Grant: Incomplete CSBK!");
		return true;
	}

	sprintf_s(pText, TextLength, "Private Voice Grant: %sfrom %d to %d on Channel %d TS%d", bEmergency ? "Emergency " : "", Sa, Ta, Lpcn, Lcn + 1);

	return true;
}

static bool DecodeTvGrant(char *pText, size_t TextLength, BitStream_t *pBs)
{
	uint16_t Lpcn;
	uint8_t Lcn;
	bool bLateEntry;
	bool bEmergency;
	bool bOffset;
	uint32_t Ta, Sa;

	BS_PopUInt(pBs, 12, &Lpcn, sizeof(Lpcn));
	BS_PopUInt(pBs, 1, &Lcn, sizeof(Lcn));
	BS_PopUInt(pBs, 1, &bLateEntry, sizeof(bLateEntry));
	BS_PopUInt(pBs, 1, &bEmergency, sizeof(bEmergency));
	BS_PopUInt(pBs, 1, &bOffset, sizeof(bOffset));
	BS_PopUInt(pBs, 24, &Ta, sizeof(Ta));
	if (!BS_PopUInt(pBs, 24, &Sa, sizeof(Sa))) {
		strcat_s(pText, TextLength, "Talkroup Voice Grant: Incomplete CSBK!");
		return true;
	}

	sprintf_s(pText, TextLength, "Talkroup Voice Grant: %sfrom %d to %d on Channel %d TS%d", bEmergency ? "Emergency " : "", Sa, Ta, Lpcn, Lcn + 1);

	return true;
}

static bool DecodeBtvGrant(char *pText, size_t TextLength, BitStream_t *pBs)
{
	uint16_t Lpcn;
	uint8_t Lcn;
	bool bLateEntry;
	bool bEmergency;
	bool bOffset;
	uint32_t Ta, Sa;

	BS_PopUInt(pBs, 12, &Lpcn, sizeof(Lpcn));
	BS_PopUInt(pBs, 1, &Lcn, sizeof(Lcn));
	BS_PopUInt(pBs, 1, &bLateEntry, sizeof(bLateEntry));
	BS_PopUInt(pBs, 1, &bEmergency, sizeof(bEmergency));
	BS_PopUInt(pBs, 1, &bOffset, sizeof(bOffset));
	BS_PopUInt(pBs, 24, &Ta, sizeof(Ta));
	if (!BS_PopUInt(pBs, 24, &Sa, sizeof(Sa))) {
		strcat_s(pText, TextLength, "Broadcast Voice Grant: Incomplete CSBK!");
		return true;
	}

	sprintf_s(pText, TextLength, "Broadcast Voice Grant: %sfrom %d to %d on Channel %d TS%d", bEmergency ? "Emergency " : "", Sa, Ta, Lpcn, Lcn + 1);

	return true;
}

static bool DecodeAhoy(char *pText, size_t TextLength, BitStream_t *pBs)
{
	uint8_t Mirror;
	bool bFlag;
	bool bAmbient;
	bool bGroup;
	uint8_t Blocks;
	uint8_t Kind;
	uint32_t Ta, Sa;

	BS_PopUInt(pBs, 7, &Mirror, sizeof(Mirror));
	BS_PopUInt(pBs, 1, &bFlag, sizeof(bFlag));
	BS_PopUInt(pBs, 1, &bAmbient, sizeof(bAmbient));
	BS_PopUInt(pBs, 1, &bGroup, sizeof(bGroup));
	BS_PopUInt(pBs, 2, &Blocks, sizeof(Blocks));
	BS_PopUInt(pBs, 4, &Kind, sizeof(Kind));
	BS_PopUInt(pBs, 24, &Ta, sizeof(Ta));
	if (!BS_PopUInt(pBs, 24, &Sa, sizeof(Sa))) {
		strcat_s(pText, TextLength, "AHOY: Incomplete CSBK!");
		return true;
	}

	sprintf_s(pText, TextLength, "AHOY: From %d to %d, Service %d, Kind %d", Sa, Ta, Mirror, Kind);

	return true;
}

static bool DecodeCAckD(char *pText, size_t TextLength, BitStream_t *pBs)
{
	uint8_t Response;
	uint8_t Reason;
	uint32_t Ta, Sa;

	BS_PopUInt(pBs, 7, &Response, sizeof(Response));
	BS_PopUInt(pBs, 8, &Reason, sizeof(Reason));
	BS_SkipBits(pBs, 1);
	BS_PopUInt(pBs, 24, &Ta, sizeof(Ta));
	if (!BS_PopUInt(pBs, 24, &Sa, sizeof(Sa))) {
		strcat_s(pText, TextLength, "C_ACKD: Incomplete CSBK!");
		return true;
	}

	sprintf_s(pText, TextLength, "C_ACKD: From %d to %d, Response %d Reason %d", Sa, Ta, Response, Reason);

	return true;
}

static bool DecodeCBcast(char *pText, size_t TextLength, BitStream_t *pBs)
{
	uint8_t Type;
	uint16_t Params1;
	bool bReg;
	uint8_t Backoff;
	uint16_t Code;
	uint32_t Params2;

	BS_PopUInt(pBs, 5, &Type, sizeof(Type));
	BS_PopUInt(pBs, 14, &Params1, sizeof(Params1));
	BS_PopUInt(pBs, 1, &bReg, sizeof(bReg));
	BS_PopUInt(pBs, 4, &Backoff, sizeof(Backoff));
	BS_PopUInt(pBs, 16, &Code, sizeof(Code));
	if (!BS_PopUInt(pBs, 24, &Params2, sizeof(Params2))) {
		strcat_s(pText, TextLength, "C_BCAST: Incomplete CSBK!");
		return true;
	}

	sprintf_s(pText, TextLength, "C_BCAST: Type %d Code %d, Params (0x%X, 0x%X)", Type, Code, Params1, Params2);

	return true;
}

static bool DecodePProtect(char *pText, size_t TextLength, BitStream_t *pBs)
{
	uint8_t Kind;
	bool bGroup;
	uint32_t Ta, Sa;
	const char *pKind;

	BS_SkipBits(pBs, 12);
	BS_PopUInt(pBs, 3, &Kind, sizeof(Kind));
	BS_PopUInt(pBs, 1, &bGroup, sizeof(bGroup));
	BS_PopUInt(pBs, 24, &Ta, sizeof(Ta));
	if (!BS_PopUInt(pBs, 24, &Sa, sizeof(Sa))) {
		strcat_s(pText, TextLength, "Channel Protect: Incomplete CSBK!");
		return true;
	}

	switch (Kind) {
	case 0: pKind = "Disable PTT"; break;
	case 1: pKind = "Enable PTT"; break;
	case 2: pKind = "Illegally parked"; break;
	case 3: pKind = "Enable PTT for Target only"; break;
	default: pKind = "Reserved"; break;
	}

	sprintf_s(pText, TextLength, "Channel Protect: From %d to %d, Kind: %s", Sa, Ta, pKind);

	return true;
}

bool CSBK_Decode(char *pText, size_t TextLength, Decoder_t *pDecoder)
{
	uint8_t LastBlock;
	uint8_t PrivateFlag;
	uint8_t Opcode;
	const uint8_t *pCsbk;
	size_t Length;

	BS_PopUInt(&pDecoder->Bs, 8, &Length, sizeof(Length));
	if (Length < 10) {
		strcat_s(pText, TextLength, "Incomplete CSBK!");
		return true;
	}

	pCsbk = BS_GetCurrentPtr(&pDecoder->Bs);

	BS_PopUInt(&pDecoder->Bs, 1, &LastBlock, sizeof(LastBlock));
	BS_PopUInt(&pDecoder->Bs, 1, &PrivateFlag, sizeof(PrivateFlag));
	BS_PopUInt(&pDecoder->Bs, 6, &Opcode, sizeof(Opcode));
	BS_SkipBits(&pDecoder->Bs, 8);

	switch (Opcode) {
	case 0x19: return DecodeAloha(pText, TextLength, &pDecoder->Bs);
	case 0x30: return DecodePvGrant(pText, TextLength, &pDecoder->Bs);
	case 0x31: return DecodeTvGrant(pText, TextLength, &pDecoder->Bs);
	case 0x32: return DecodeBtvGrant(pText, TextLength, &pDecoder->Bs);
	case 0x1C: return DecodeAhoy(pText, TextLength, &pDecoder->Bs);
	case 0x20: return DecodeCAckD(pText, TextLength, &pDecoder->Bs);
	case 0x28: return DecodeCBcast(pText, TextLength, &pDecoder->Bs);
	case 0x2F: return DecodePProtect(pText, TextLength, &pDecoder->Bs);
	default:
		HEX_Append(pText, TextLength, "CSBK", pCsbk, Length);
		BS_SkipBytes(&pDecoder->Bs, 8); // We already popped 2 bytes
		return true;
	}
}
