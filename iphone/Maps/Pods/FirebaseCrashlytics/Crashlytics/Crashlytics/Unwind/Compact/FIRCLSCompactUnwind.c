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

#include "FIRCLSCompactUnwind_Private.h"
#include "FIRCLSDataParsing.h"
#include "FIRCLSDefines.h"
#include "FIRCLSDwarfUnwind.h"
#include "FIRCLSFeatures.h"
#include "FIRCLSUnwind.h"
#include "FIRCLSUtility.h"

#include <string.h>

#if CLS_COMPACT_UNWINDING_SUPPORTED

#pragma mark Parsing
bool FIRCLSCompactUnwindInit(FIRCLSCompactUnwindContext* context,
                             const void* unwindInfo,
                             const void* ehFrame,
                             uintptr_t loadAddress) {
  if (!FIRCLSIsValidPointer(context)) {
    FIRCLSSDKLog("Error: invalid context passed to compact unwind init");
    return false;
  }
  if (!FIRCLSIsValidPointer(unwindInfo)) {
    FIRCLSSDKLog("Error: invalid unwind info passed to compact unwind init");
    return false;
  }
  if (!FIRCLSIsValidPointer(loadAddress)) {
    FIRCLSSDKLog("Error: invalid load address passed to compact unwind init");
    return false;
  }

  memset(context, 0, sizeof(FIRCLSCompactUnwindContext));

  if (!FIRCLSReadMemory((vm_address_t)unwindInfo, &context->unwindHeader,
                        sizeof(struct unwind_info_section_header))) {
    FIRCLSSDKLog("Error: could not read memory contents of unwindInfo\n");
    return false;
  }

  if (context->unwindHeader.version != UNWIND_SECTION_VERSION) {
    FIRCLSSDKLog("Error: bad unwind_info structure version (%d != %d)\n",
                 context->unwindHeader.version, UNWIND_SECTION_VERSION);
    return false;
  }

  // copy in the values
  context->unwindInfo = unwindInfo;
  context->ehFrame = ehFrame;
  context->loadAddress = loadAddress;

  return true;
}

void* FIRCLSCompactUnwindGetIndexData(FIRCLSCompactUnwindContext* context) {
  return (void*)((uintptr_t)context->unwindInfo +
                 (uintptr_t)context->unwindHeader.indexSectionOffset);
}

compact_unwind_encoding_t* FIRCLSCompactUnwindGetCommonEncodings(
    FIRCLSCompactUnwindContext* context) {
  return (compact_unwind_encoding_t*)((uintptr_t)context->unwindInfo +
                                      (uintptr_t)
                                          context->unwindHeader.commonEncodingsArraySectionOffset);
}

void* FIRCLSCompactUnwindGetSecondLevelData(FIRCLSCompactUnwindContext* context) {
  return (void*)((uintptr_t)context->unwindInfo +
                 context->indexHeader.secondLevelPagesSectionOffset);
}

uintptr_t FIRCLSCompactUnwindGetIndexFunctionOffset(FIRCLSCompactUnwindContext* context) {
  return context->loadAddress + context->indexHeader.functionOffset;
}
uintptr_t FIRCLSCompactUnwindGetTargetAddress(FIRCLSCompactUnwindContext* context, uintptr_t pc) {
  uintptr_t offset = FIRCLSCompactUnwindGetIndexFunctionOffset(context);

  if (pc <= offset) {
    FIRCLSSDKLog("Error: PC is invalid\n");
    return 0;
  }

  return pc - offset;
}

#pragma mark - Parsing and Lookup
bool FIRCLSCompactUnwindLookupFirstLevel(FIRCLSCompactUnwindContext* context, uintptr_t address) {
  if (!context) {
    return false;
  }

  // In practice, it appears that there always one more first level entry
  // than required. This actually makes sense, since we have to use this
  // info to check if we are in range. This implies there must be
  // at least 2 indices at a minimum.

  uint32_t indexCount = context->unwindHeader.indexCount;
  if (indexCount < 2) {
    return false;
  }

  // make sure our address is valid
  if (address < context->loadAddress) {
    return false;
  }

  struct unwind_info_section_header_index_entry* indexEntries =
      FIRCLSCompactUnwindGetIndexData(context);
  if (!indexEntries) {
    return false;
  }

  address -= context->loadAddress;  // search relative to zero

  // minus one because of the extra entry - see comment above
  for (uint32_t index = 0; index < indexCount - 1; ++index) {
    uint32_t value = indexEntries[index].functionOffset;
    uint32_t nextValue = indexEntries[index + 1].functionOffset;

    if (address >= value && address < nextValue) {
      context->firstLevelNextFunctionOffset = nextValue;
      context->indexHeader = indexEntries[index];
      return true;
    }
  }

  return false;
}

uint32_t FIRCLSCompactUnwindGetSecondLevelPageKind(FIRCLSCompactUnwindContext* context) {
  if (!context) {
    return 0;
  }

  return *(uint32_t*)FIRCLSCompactUnwindGetSecondLevelData(context);
}

bool FIRCLSCompactUnwindLookupSecondLevelRegular(FIRCLSCompactUnwindContext* context,
                                                 uintptr_t pc,
                                                 FIRCLSCompactUnwindResult* result) {
  FIRCLSSDKLog("Encountered a regular second-level page\n");
  return false;
}

// this only works for compressed entries right now
bool FIRCLSCompactUnwindBinarySearchSecondLevel(uintptr_t address,
                                                uint32_t* index,
                                                uint16_t entryCount,
                                                uint32_t* entryArray) {
  if (!index || !entryArray) {
    return false;
  }

  if (entryCount == 0) {
    return false;
  }

  if (address == 0) {
    return false;
  }

  uint32_t highIndex = entryCount;
  *index = 0;

  while (*index < highIndex) {
    uint32_t midIndex = (*index + highIndex) / 2;

    //        FIRCLSSDKLog("%u %u %u\n", *index, midIndex, highIndex);

    uintptr_t value = UNWIND_INFO_COMPRESSED_ENTRY_FUNC_OFFSET(entryArray[midIndex]);

    if (value > address) {
      if (highIndex == midIndex) {
        return false;
      }

      highIndex = midIndex;
      continue;
    }

    *index = midIndex;

    // are we at the end of the array?
    if (midIndex == entryCount - 1) {
      return false;
    }

    uintptr_t nextValue = UNWIND_INFO_COMPRESSED_ENTRY_FUNC_OFFSET(entryArray[midIndex + 1]);
    if (nextValue > address) {
      // we've found it
      break;
    }

    *index += 1;
  }

  // check to make sure we're still within bounds
  return *index < entryCount;
}

bool FIRCLSCompactUnwindLookupSecondLevelCompressed(FIRCLSCompactUnwindContext* context,
                                                    uintptr_t pc,
                                                    FIRCLSCompactUnwindResult* result) {
  if (!context || !result) {
    return false;
  }

  void* ptr = FIRCLSCompactUnwindGetSecondLevelData(context);

  if (!ptr) {
    return false;
  }

  memset(result, 0, sizeof(FIRCLSCompactUnwindResult));

  struct unwind_info_compressed_second_level_page_header* header =
      (struct unwind_info_compressed_second_level_page_header*)ptr;

  // adjust address
  uintptr_t targetAddress = FIRCLSCompactUnwindGetTargetAddress(context, pc);

  uint32_t* entryArray = ptr + header->entryPageOffset;

  uint32_t index = 0;

  if (!FIRCLSCompactUnwindBinarySearchSecondLevel(targetAddress, &index, header->entryCount,
                                                  entryArray)) {
    FIRCLSSDKLogInfo("Unable to find PC in second level\n");
    return false;
  }

  uint32_t entry = entryArray[index];

  // Computing the fuction start address is easy
  result->functionStart = UNWIND_INFO_COMPRESSED_ENTRY_FUNC_OFFSET(entry) +
                          FIRCLSCompactUnwindGetIndexFunctionOffset(context);

  // Computing the end is more complex, because we could be on the last entry. In that case, we
  // cannot use the next value as the end.
  result->functionEnd = context->loadAddress;
  if (index < header->entryCount - 1) {
    result->functionEnd += UNWIND_INFO_COMPRESSED_ENTRY_FUNC_OFFSET(entryArray[index + 1]) +
                           context->indexHeader.functionOffset;
  } else {
    result->functionEnd += context->firstLevelNextFunctionOffset;
  }

  //    FIRCLSSDKLog("Located %lx => %lx %lx\n", pc, result->functionStart, result->functionEnd);

  if ((pc < result->functionStart) || (pc >= result->functionEnd)) {
    FIRCLSSDKLog("PC does not match computed function range\n");
    return false;
  }

  uint32_t encodingIndex = UNWIND_INFO_COMPRESSED_ENTRY_ENCODING_INDEX(entry);

  // encoding could be in the common array
  if (encodingIndex < context->unwindHeader.commonEncodingsArrayCount) {
    result->encoding = FIRCLSCompactUnwindGetCommonEncodings(context)[encodingIndex];

    //        FIRCLSSDKLog("Entry has common encoding: 0x%x\n", result->encoding);
  } else {
    encodingIndex = encodingIndex - context->unwindHeader.commonEncodingsArrayCount;

    compact_unwind_encoding_t* encodings = ptr + header->encodingsPageOffset;

    result->encoding = encodings[encodingIndex];

    //        FIRCLSSDKLog("Entry has compressed encoding: 0x%x\n", result->encoding);
  }

  if (result->encoding == 0) {
    FIRCLSSDKLogInfo("Entry has has no unwind info\n");
    return false;
  }

  return true;
}

bool FIRCLSCompactUnwindLookupSecondLevel(FIRCLSCompactUnwindContext* context,
                                          uintptr_t pc,
                                          FIRCLSCompactUnwindResult* result) {
  switch (FIRCLSCompactUnwindGetSecondLevelPageKind(context)) {
    case UNWIND_SECOND_LEVEL_REGULAR:
      FIRCLSSDKLogInfo("Found a second level regular header\n");
      if (FIRCLSCompactUnwindLookupSecondLevelRegular(context, pc, result)) {
        return true;
      }
      break;
    case UNWIND_SECOND_LEVEL_COMPRESSED:
      FIRCLSSDKLogInfo("Found a second level compressed header\n");
      if (FIRCLSCompactUnwindLookupSecondLevelCompressed(context, pc, result)) {
        return true;
      }
      break;
    default:
      FIRCLSSDKLogError("Unrecognized header kind - unable to continue\n");
      break;
  }

  return false;
}

bool FIRCLSCompactUnwindLookup(FIRCLSCompactUnwindContext* context,
                               uintptr_t pc,
                               FIRCLSCompactUnwindResult* result) {
  if (!context || !result) {
    return false;
  }

  // step 1 - find the pc in the first-level index
  if (!FIRCLSCompactUnwindLookupFirstLevel(context, pc)) {
    FIRCLSSDKLogWarn("Unable to find pc in first level\n");
    return false;
  }

  FIRCLSSDKLogDebug("Found first level (second => %u)\n",
                    context->indexHeader.secondLevelPagesSectionOffset);

  // step 2 - use that info to find the second-level information
  // that second actually has the encoding info we're looking for.
  if (!FIRCLSCompactUnwindLookupSecondLevel(context, pc, result)) {
    FIRCLSSDKLogInfo("Second-level PC lookup failed\n");
    return false;
  }

  return true;
}

#pragma mark - Unwinding
bool FIRCLSCompactUnwindLookupAndCompute(FIRCLSCompactUnwindContext* context,
                                         FIRCLSThreadContext* registers) {
  if (!context || !registers) {
    return false;
  }

  uintptr_t pc = FIRCLSThreadContextGetPC(registers);

  // little sanity check
  if (pc < context->loadAddress) {
    return false;
  }

  FIRCLSCompactUnwindResult result;

  memset(&result, 0, sizeof(result));

  if (!FIRCLSCompactUnwindLookup(context, pc, &result)) {
    FIRCLSSDKLogInfo("Unable to lookup compact unwind for pc %p\n", (void*)pc);
    return false;
  }

  // Ok, armed with the encoding, we can actually attempt to modify the registers. Because
  // the encoding is arch-specific, this function has to be defined per-arch.
  if (!FIRCLSCompactUnwindComputeRegisters(context, &result, registers)) {
    FIRCLSSDKLogError("Failed to compute registers\n");
    return false;
  }

  return true;
}

#if CLS_DWARF_UNWINDING_SUPPORTED
bool FIRCLSCompactUnwindDwarfFrame(FIRCLSCompactUnwindContext* context,
                                   uintptr_t dwarfOffset,
                                   FIRCLSThreadContext* registers) {
  if (!context || !registers) {
    return false;
  }

  // Everyone's favorite! Dwarf unwinding!
  FIRCLSSDKLogInfo("Trying to read dwarf data with offset %lx\n", dwarfOffset);

  FIRCLSDwarfCFIRecord record;

  if (!FIRCLSDwarfParseCFIFromFDERecordOffset(&record, context->ehFrame, dwarfOffset)) {
    FIRCLSSDKLogError("Unable to init FDE\n");
    return false;
  }

  if (!FIRCLSDwarfUnwindComputeRegisters(&record, registers)) {
    FIRCLSSDKLogError("Failed to compute DWARF registers\n");
    return false;
  }

  return true;
}
#endif

#else
INJECT_STRIP_SYMBOL(compact_unwind)
#endif
