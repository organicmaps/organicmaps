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

#include <mach-o/arch.h>
#include <mach-o/loader.h>
#include <sys/types.h>

#include <CoreFoundation/CoreFoundation.h>

struct FIRCLSMachOFile {
  int fd;
  size_t mappedSize;
  void* mappedFile;
};
typedef struct FIRCLSMachOFile* FIRCLSMachOFileRef;

struct FIRCLSMachOSlice {
  const void* startAddress;
  cpu_type_t cputype;
  cpu_subtype_t cpusubtype;
};
typedef struct FIRCLSMachOSlice* FIRCLSMachOSliceRef;

typedef struct {
  uint32_t major;
  uint32_t minor;
  uint32_t bugfix;
} FIRCLSMachOVersion;

typedef struct {
  uint64_t addr;
  uint64_t size;
  uint32_t offset;
} FIRCLSMachOSection;

typedef struct {
  char segname[16];
  uint64_t vmaddr;
  uint64_t vmsize;
} FIRCLSMachOSegmentCommand;

typedef void (^FIRCLSMachOSliceIterator)(FIRCLSMachOSliceRef slice);
typedef void (^FIRCLSMachOLoadCommandIterator)(uint32_t type,
                                               uint32_t size,
                                               const struct load_command* cmd);

__BEGIN_DECLS

bool FIRCLSMachOFileInitWithPath(FIRCLSMachOFileRef file, const char* path);
bool FIRCLSMachOFileInitWithCurrent(FIRCLSMachOFileRef file);
void FIRCLSMachOFileDestroy(FIRCLSMachOFileRef file);
void FIRCLSMachOFileEnumerateSlices(FIRCLSMachOFileRef file, FIRCLSMachOSliceIterator block);
struct FIRCLSMachOSlice FIRCLSMachOFileSliceWithArchitectureName(FIRCLSMachOFileRef file,
                                                                 const char* name);

void FIRCLSMachOEnumerateSlicesAtAddress(void* executableData, FIRCLSMachOSliceIterator block);
void FIRCLSMachOSliceEnumerateLoadCommands(FIRCLSMachOSliceRef slice,
                                           FIRCLSMachOLoadCommandIterator block);
struct FIRCLSMachOSlice FIRCLSMachOSliceGetCurrent(void);
struct FIRCLSMachOSlice FIRCLSMachOSliceWithHeader(void* machHeader);

const char* FIRCLSMachOSliceGetExecutablePath(FIRCLSMachOSliceRef slice);
const char* FIRCLSMachOSliceGetArchitectureName(FIRCLSMachOSliceRef slice);
bool FIRCLSMachOSliceIs64Bit(FIRCLSMachOSliceRef slice);
bool FIRCLSMachOSliceGetSectionByName(FIRCLSMachOSliceRef slice,
                                      const char* segName,
                                      const char* sectionName,
                                      const void** ptr);
bool FIRCLSMachOSliceInitSectionByName(FIRCLSMachOSliceRef slice,
                                       const char* segName,
                                       const char* sectionName,
                                       FIRCLSMachOSection* section);
void FIRCLSMachOSliceGetUnwindInformation(FIRCLSMachOSliceRef slice,
                                          const void** ehFrame,
                                          const void** unwindInfo);

// load-command-specific calls for convenience

// returns a pointer to the 16-byte UUID
uint8_t const* FIRCLSMachOGetUUID(const struct load_command* cmd);
const char* FIRCLSMachOGetDylibPath(const struct load_command* cmd);

// return true if the header indicates the binary is encrypted
bool FIRCLSMachOGetEncrypted(const struct load_command* cmd);

// SDK minimums
FIRCLSMachOVersion FIRCLSMachOGetMinimumOSVersion(const struct load_command* cmd);
FIRCLSMachOVersion FIRCLSMachOGetLinkedSDKVersion(const struct load_command* cmd);

// Helpers
FIRCLSMachOSegmentCommand FIRCLSMachOGetSegmentCommand(const struct load_command* cmd);

#ifdef __OBJC__
NSString* FIRCLSMachONormalizeUUID(CFUUIDBytes* uuidBytes);
NSString* FIRCLSMachOFormatVersion(FIRCLSMachOVersion* version);
#endif
__END_DECLS
