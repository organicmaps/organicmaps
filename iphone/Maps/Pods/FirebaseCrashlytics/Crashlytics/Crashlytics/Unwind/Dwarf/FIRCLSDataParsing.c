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

#include "FIRCLSDataParsing.h"
#include "FIRCLSDefines.h"
#include "FIRCLSUtility.h"
#include "dwarf.h"

#include <string.h>

#if CLS_DWARF_UNWINDING_SUPPORTED

uint8_t FIRCLSParseUint8AndAdvance(const void** cursor) {
  uint8_t tmp = **(uint8_t**)cursor;

  *cursor += sizeof(uint8_t);

  return tmp;
}

uint16_t FIRCLSParseUint16AndAdvance(const void** cursor) {
  uint16_t tmp = **(uint16_t**)cursor;

  *cursor += sizeof(uint16_t);

  return tmp;
}

int16_t FIRCLSParseInt16AndAdvance(const void** cursor) {
  int16_t tmp = **(int16_t**)cursor;

  *cursor += sizeof(int16_t);

  return tmp;
}

uint32_t FIRCLSParseUint32AndAdvance(const void** cursor) {
  uint32_t tmp = **(uint32_t**)cursor;

  *cursor += sizeof(uint32_t);

  return tmp;
}

int32_t FIRCLSParseInt32AndAdvance(const void** cursor) {
  int32_t tmp = **(int32_t**)cursor;

  *cursor += sizeof(int32_t);

  return tmp;
}

uint64_t FIRCLSParseUint64AndAdvance(const void** cursor) {
  uint64_t tmp = **(uint64_t**)cursor;

  *cursor += sizeof(uint64_t);

  return tmp;
}

int64_t FIRCLSParseInt64AndAdvance(const void** cursor) {
  int64_t tmp = **(int64_t**)cursor;

  *cursor += sizeof(int64_t);

  return tmp;
}

uintptr_t FIRCLSParsePointerAndAdvance(const void** cursor) {
  uintptr_t tmp = **(uintptr_t**)cursor;

  *cursor += sizeof(uintptr_t);

  return tmp;
}

// Signed and Unsigned LEB128 decoding algorithms taken from Wikipedia -
// http://en.wikipedia.org/wiki/LEB128
uint64_t FIRCLSParseULEB128AndAdvance(const void** cursor) {
  uint64_t result = 0;
  char shift = 0;

  for (int i = 0; i < sizeof(uint64_t); ++i) {
    char byte;

    byte = **(uint8_t**)cursor;

    *cursor += 1;

    result |= ((0x7F & byte) << shift);
    if ((0x80 & byte) == 0) {
      break;
    }

    shift += 7;
  }

  return result;
}

int64_t FIRCLSParseLEB128AndAdvance(const void** cursor) {
  uint64_t result = 0;
  char shift = 0;
  char size = sizeof(int64_t) * 8;
  char byte = 0;

  for (int i = 0; i < sizeof(uint64_t); ++i) {
    byte = **(uint8_t**)cursor;

    *cursor += 1;

    result |= ((0x7F & byte) << shift);
    shift += 7;

    /* sign bit of byte is second high order bit (0x40) */
    if ((0x80 & byte) == 0) {
      break;
    }
  }

  if ((shift < size) && (0x40 & byte)) {
    // sign extend
    result |= -(1 << shift);
  }

  return result;
}

const char* FIRCLSParseStringAndAdvance(const void** cursor) {
  const char* string;

  string = (const char*)(*cursor);

  // strlen doesn't include the null character, which we need to advance past
  *cursor += strlen(string) + 1;

  return string;
}

uint64_t FIRCLSParseRecordLengthAndAdvance(const void** cursor) {
  uint64_t length;

  length = FIRCLSParseUint32AndAdvance(cursor);
  if (length == DWARF_EXTENDED_LENGTH_FLAG) {
    length = FIRCLSParseUint64AndAdvance(cursor);
  }

  return length;
}

uintptr_t FIRCLSParseAddressWithEncodingAndAdvance(const void** cursor, uint8_t encoding) {
  if (encoding == DW_EH_PE_omit) {
    return 0;
  }

  if (!cursor) {
    return CLS_INVALID_ADDRESS;
  }

  if (!*cursor) {
    return CLS_INVALID_ADDRESS;
  }

  intptr_t inputAddr = (intptr_t)*cursor;
  intptr_t addr;

  switch (encoding & DW_EH_PE_VALUE_MASK) {
    case DW_EH_PE_ptr:
      // 32 or 64 bits
      addr = FIRCLSParsePointerAndAdvance(cursor);
      break;
    case DW_EH_PE_uleb128:
      addr = (intptr_t)FIRCLSParseULEB128AndAdvance(cursor);
      break;
    case DW_EH_PE_udata2:
      addr = FIRCLSParseUint16AndAdvance(cursor);
      break;
    case DW_EH_PE_udata4:
      addr = FIRCLSParseUint32AndAdvance(cursor);
      break;
    case DW_EH_PE_udata8:
      addr = (intptr_t)FIRCLSParseUint64AndAdvance(cursor);
      break;
    case DW_EH_PE_sleb128:
      addr = (intptr_t)FIRCLSParseLEB128AndAdvance(cursor);
      break;
    case DW_EH_PE_sdata2:
      addr = FIRCLSParseInt16AndAdvance(cursor);
      break;
    case DW_EH_PE_sdata4:
      addr = FIRCLSParseInt32AndAdvance(cursor);
      break;
    case DW_EH_PE_sdata8:
      addr = (intptr_t)FIRCLSParseInt64AndAdvance(cursor);
      break;
    default:
      FIRCLSSDKLog("Unhandled: encoding 0x%02x\n", encoding);
      return CLS_INVALID_ADDRESS;
  }

  // and now apply the relative offset
  switch (encoding & DW_EH_PE_RELATIVE_OFFSET_MASK) {
    case DW_EH_PE_absptr:
      break;
    case DW_EH_PE_pcrel:
      addr += inputAddr;
      break;
    default:
      FIRCLSSDKLog("Unhandled: relative encoding 0x%02x\n", encoding);
      return CLS_INVALID_ADDRESS;
  }

  // Here's a crazy one. It seems this encoding means you actually look up
  // the value of the address using the result address itself
  if (encoding & DW_EH_PE_indirect) {
    if (!addr) {
      return CLS_INVALID_ADDRESS;
    }

    addr = *(uintptr_t*)addr;
  }

  return addr;
}
#else
INJECT_STRIP_SYMBOL(data_parsing)
#endif
