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

#ifndef DECODER_H
#define DECODER_H

#include <stdbool.h>
#include <stddef.h>

enum {
	ANYTONE_MAX_FRAME_LENGTH = 330
};

typedef struct Decoder_t Decoder_t;

Decoder_t *DECODER_New(void);
int DECODER_AddBytes(Decoder_t *pDecoder, const void *pBuffer, size_t Length);
bool DECODER_Check(Decoder_t *pDecoder);
bool DECODER_GetText(Decoder_t *pDecoder, bool bSkip, char *pText, size_t TextLength);
size_t DECODER_GetFrameLength(Decoder_t *pDecoder);

#endif

