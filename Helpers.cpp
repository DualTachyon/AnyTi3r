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
#include "Helpers.h"

void HEX_Append(char *pLog, size_t LogSize, const char *pHeader, const uint8_t *pData, size_t DataSize)
{
	size_t i;

	if (pHeader) {
		strcat_s(pLog, LogSize, pHeader);
		strcat_s(pLog, LogSize, ":");
	}

	for (i = 0; i < DataSize; i++) {
		char Tmp[8];
		sprintf_s(Tmp, sizeof(Tmp), " %02X", pData[i]);
		strcat_s(pLog, LogSize, Tmp);
	}
}
