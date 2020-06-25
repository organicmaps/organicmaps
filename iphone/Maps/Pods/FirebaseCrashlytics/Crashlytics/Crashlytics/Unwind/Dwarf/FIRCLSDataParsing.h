// Copyright 2019 Google
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stdint.h>

#include "FIRCLSFeatures.h"

#if CLS_DWARF_UNWINDING_SUPPORTED

#if CLS_CPU_64BIT
#define CLS_INVALID_ADDRESS (0xffffffffffffffff)
#else
#define CLS_INVALID_ADDRESS (0xffffffff)
#endif

// basic data types
uint8_t FIRCLSParseUint8AndAdvance(const void** cursor);
uint16_t FIRCLSParseUint16AndAdvance(const void** cursor);
int16_t FIRCLSParseInt16AndAdvance(const void** cursor);
uint32_t FIRCLSParseUint32AndAdvance(const void** cursor);
int32_t FIRCLSParseInt32AndAdvance(const void** cursor);
uint64_t FIRCLSParseUint64AndAdvance(const void** cursor);
int64_t FIRCLSParseInt64AndAdvance(const void** cursor);
uintptr_t FIRCLSParsePointerAndAdvance(const void** cursor);
uint64_t FIRCLSParseULEB128AndAdvance(const void** cursor);
int64_t FIRCLSParseLEB128AndAdvance(const void** cursor);
const char* FIRCLSParseStringAndAdvance(const void** cursor);

// FDE/CIE-specifc structures
uint64_t FIRCLSParseRecordLengthAndAdvance(const void** cursor);
uintptr_t FIRCLSParseAddressWithEncodingAndAdvance(const void** cursor, uint8_t encoding);

#endif
