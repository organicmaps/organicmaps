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

#include "FIRCLSMachO.h"

#include <Foundation/Foundation.h>

#include <mach-o/dyld.h>
#include <mach-o/fat.h>
#include <mach-o/getsect.h>
#include <mach-o/ldsyms.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include <dlfcn.h>
#include <fcntl.h>

#include <stdio.h>

#include <unistd.h>

// This is defined in newer versions of iOS/macOS in usr/include/mach/machine.h
#define CLS_CPU_SUBTYPE_ARM64E ((cpu_subtype_t)2)

static void FIRCLSMachOHeaderValues(FIRCLSMachOSliceRef slice,
                                    const struct load_command** cmds,
                                    uint32_t* cmdCount);
static bool FIRCLSMachOSliceIsValid(FIRCLSMachOSliceRef slice);

bool FIRCLSMachOFileInitWithPath(FIRCLSMachOFileRef file, const char* path) {
  struct stat statBuffer;

  if (!file || !path) {
    return false;
  }

  file->fd = 0;
  file->mappedFile = NULL;
  file->mappedSize = 0;

  file->fd = open(path, O_RDONLY);
  if (file->fd < 0) {
    // unable to open mach-o file
    return false;
  }

  if (fstat(file->fd, &statBuffer) == -1) {
    close(file->fd);
    return false;
  }

  // We need some minimum size for this to even be a possible mach-o file.  I believe
  // its probably quite a bit bigger than this, but this at least covers something.
  // We also need it to be a regular file.
  file->mappedSize = (size_t)statBuffer.st_size;
  if (statBuffer.st_size < 16 || !(statBuffer.st_mode & S_IFREG)) {
    close(file->fd);
    return false;
  }

  // Map the file to memory. MAP_SHARED can potentially reduce the amount of actual private
  // memory needed to do this mapping. Also, be sure to check for the correct failure result.
  file->mappedFile = mmap(0, file->mappedSize, PROT_READ, MAP_FILE | MAP_SHARED, file->fd, 0);
  if (!file->mappedFile || (file->mappedFile == MAP_FAILED)) {
    close(file->fd);
    return false;
  }

  return true;
}

bool FIRCLSMachOFileInitWithCurrent(FIRCLSMachOFileRef file) {
  struct FIRCLSMachOSlice slice = FIRCLSMachOSliceGetCurrent();

  const char* imagePath = FIRCLSMachOSliceGetExecutablePath(&slice);

  return FIRCLSMachOFileInitWithPath(file, imagePath);
}

void FIRCLSMachOFileDestroy(FIRCLSMachOFileRef file) {
  if (!file) {
    return;
  }

  if (file->mappedFile && file->mappedSize > 0) {
    munmap(file->mappedFile, file->mappedSize);
  }

  close(file->fd);
}

void FIRCLSMachOFileEnumerateSlices(FIRCLSMachOFileRef file, FIRCLSMachOSliceIterator block) {
  FIRCLSMachOEnumerateSlicesAtAddress(file->mappedFile, block);
}

void FIRCLSMachOEnumerateSlicesAtAddress(void* executableData, FIRCLSMachOSliceIterator block) {
  // check the magic value, to determine if we have a fat header or not
  uint32_t magicValue;
  uint32_t archCount;
  const struct fat_arch* fatArch;
  struct FIRCLSMachOSlice slice;

  memset(&slice, 0, sizeof(struct FIRCLSMachOSlice));

  magicValue = ((struct fat_header*)executableData)->magic;
  if ((magicValue != FAT_MAGIC) && (magicValue != FAT_CIGAM)) {
    slice.startAddress = executableData;

    // use this to fill in the values
    FIRCLSMachOHeaderValues(&slice, NULL, NULL);

    block(&slice);

    return;
  }

  archCount = OSSwapBigToHostInt32(((struct fat_header*)executableData)->nfat_arch);
  fatArch = executableData + sizeof(struct fat_header);

  for (uint32_t i = 0; i < archCount; ++i) {
    slice.cputype = OSSwapBigToHostInt32(fatArch->cputype);
    slice.cpusubtype = OSSwapBigToHostInt32(fatArch->cpusubtype);
    slice.startAddress = executableData + OSSwapBigToHostInt32(fatArch->offset);

    block(&slice);

    // advance to the next fat_arch structure
    fatArch = (struct fat_arch*)((uintptr_t)fatArch + sizeof(struct fat_arch));
  }
}

struct FIRCLSMachOSlice FIRCLSMachOFileSliceWithArchitectureName(FIRCLSMachOFileRef file,
                                                                 const char* name) {
  __block struct FIRCLSMachOSlice value;

  memset(&value, 0, sizeof(struct FIRCLSMachOSlice));

  FIRCLSMachOFileEnumerateSlices(file, ^(FIRCLSMachOSliceRef slice) {
    if (strcmp(FIRCLSMachOSliceGetArchitectureName(slice), name) == 0) {
      value = *slice;
    }
  });

  return value;
}

static void FIRCLSMachOHeaderValues(FIRCLSMachOSliceRef slice,
                                    const struct load_command** cmds,
                                    uint32_t* cmdCount) {
  const struct mach_header* header32 = (const struct mach_header*)slice->startAddress;
  const struct mach_header_64* header64 = (const struct mach_header_64*)slice->startAddress;
  uint32_t commandCount;
  const void* commandsAddress;

  if (cmds) {
    *cmds = NULL;
  }

  if (cmdCount) {
    *cmdCount = 0;
  }

  if (!slice->startAddress) {
    return;
  }

  // the 32 and 64 bit versions have an identical structures, so this will work
  switch (header32->magic) {
    case MH_MAGIC:  // 32-bit
    case MH_CIGAM:
      slice->cputype = header32->cputype;
      slice->cpusubtype = header32->cpusubtype;
      commandCount = header32->ncmds;
      commandsAddress = slice->startAddress + sizeof(struct mach_header);
      break;
    case MH_MAGIC_64:  // 64-bit
    case MH_CIGAM_64:
      slice->cputype = header64->cputype;
      slice->cpusubtype = header64->cpusubtype;
      commandCount = header64->ncmds;
      commandsAddress = slice->startAddress + sizeof(struct mach_header_64);
      break;
    default:
      // not a valid header
      return;
  }

  // assign everything back by reference
  if (cmds) {
    *cmds = commandsAddress;
  }

  if (cmdCount) {
    *cmdCount = commandCount;
  }
}

static bool FIRCLSMachOSliceIsValid(FIRCLSMachOSliceRef slice) {
  if (!slice) {
    return false;
  }

  if (!slice->startAddress) {
    return false;
  }

  return true;
}

void FIRCLSMachOSliceEnumerateLoadCommands(FIRCLSMachOSliceRef slice,
                                           FIRCLSMachOLoadCommandIterator block) {
  const struct load_command* cmd;
  uint32_t cmdCount;

  if (!block) {
    return;
  }

  if (!FIRCLSMachOSliceIsValid(slice)) {
    return;
  }

  FIRCLSMachOHeaderValues(slice, &cmd, &cmdCount);

  for (uint32_t i = 0; cmd != NULL && i < cmdCount; ++i) {
    block(cmd->cmd, cmd->cmdsize, cmd);

    cmd = (struct load_command*)((uintptr_t)cmd + cmd->cmdsize);
  }
}

struct FIRCLSMachOSlice FIRCLSMachOSliceGetCurrent(void) {
  const NXArchInfo* archInfo;
  struct FIRCLSMachOSlice slice;
  void* executableSymbol;
  Dl_info dlinfo;

  archInfo = NXGetLocalArchInfo();
  if (archInfo) {
    slice.cputype = archInfo->cputype;
    slice.cpusubtype = archInfo->cpusubtype;
  }

  slice.startAddress = NULL;

  // This call can fail when Exported Symbols File in Build Settings is missing the symbol value
  // defined as _MH_EXECUTE_SYM (if you look in the header the underscored MH_EXECUTE_SYM define is
  // there)
  executableSymbol = dlsym(RTLD_MAIN_ONLY, MH_EXECUTE_SYM);

  // get the address of the main function
  if (dladdr(executableSymbol, &dlinfo) != 0) {
    slice.startAddress = dlinfo.dli_fbase;
  }

  return slice;
}

struct FIRCLSMachOSlice FIRCLSMachOSliceWithHeader(void* machHeader) {
  struct FIRCLSMachOSlice slice;

  slice.startAddress = machHeader;

  return slice;
}

const char* FIRCLSMachOSliceGetExecutablePath(FIRCLSMachOSliceRef slice) {
  Dl_info info;

  if (!FIRCLSMachOSliceIsValid(slice)) {
    return NULL;
  }

  // use dladdr here to look up the information we need for a binary image
  if (dladdr(slice->startAddress, &info) == 0) {
    return NULL;
  }

  return info.dli_fname;
}

const char* FIRCLSMachOSliceGetArchitectureName(FIRCLSMachOSliceRef slice) {
  const NXArchInfo* archInfo;

  // there are some special cases here for types not handled by earlier OSes
  if (slice->cputype == CPU_TYPE_ARM && slice->cpusubtype == CPU_SUBTYPE_ARM_V7S) {
    return "armv7s";
  }

  if (slice->cputype == (CPU_TYPE_ARM | CPU_ARCH_ABI64)) {
    if (slice->cpusubtype == CLS_CPU_SUBTYPE_ARM64E) {
      return "arm64e";
    } else if (slice->cpusubtype == CPU_SUBTYPE_ARM64_ALL) {
      return "arm64";
    }
  }

  if (slice->cputype == (CPU_TYPE_ARM) && slice->cpusubtype == CPU_SUBTYPE_ARM_V7K) {
    return "armv7k";
  }

  archInfo = NXGetArchInfoFromCpuType(slice->cputype, slice->cpusubtype);
  if (!archInfo) {
    return "unknown";
  }

  return archInfo->name;
}

bool FIRCLSMachOSliceIs64Bit(FIRCLSMachOSliceRef slice) {
  // I'm pretty sure this is sufficient...
  return (slice->cputype & CPU_ARCH_ABI64) == CPU_ARCH_ABI64;
}

bool FIRCLSMachOSliceGetSectionByName(FIRCLSMachOSliceRef slice,
                                      const char* segName,
                                      const char* sectionName,
                                      const void** ptr) {
  if (!ptr) {
    return false;
  }

  *ptr = NULL;  // make sure this is set before returning

  FIRCLSMachOSection section;

  if (!FIRCLSMachOSliceInitSectionByName(slice, segName, sectionName, &section)) {
    return false;
  }

  // WARNING: this calculation isn't correct, but is here to maintain backwards
  // compatibility for now with callers of FIRCLSMachOSliceGetSectionByName. All new
  // users should be calling FIRCLSMachOSliceInitSectionByName
  *ptr = (const void*)((uintptr_t)slice->startAddress + section.offset);

  return true;
}

bool FIRCLSMachOSliceInitSectionByName(FIRCLSMachOSliceRef slice,
                                       const char* segName,
                                       const char* sectionName,
                                       FIRCLSMachOSection* section) {
  if (!FIRCLSMachOSliceIsValid(slice)) {
    return false;
  }

  if (!section) {
    return false;
  }

  memset(section, 0, sizeof(FIRCLSMachOSection));

  if (FIRCLSMachOSliceIs64Bit(slice)) {
    const struct section_64* sect =
        getsectbynamefromheader_64(slice->startAddress, segName, sectionName);
    if (!sect) {
      return false;
    }

    section->addr = sect->addr;
    section->size = sect->size;
    section->offset = sect->offset;
  } else {
    const struct section* sect = getsectbynamefromheader(slice->startAddress, segName, sectionName);
    if (!sect) {
      return false;
    }

    section->addr = sect->addr;
    section->size = sect->size;
    section->offset = sect->offset;
  }

  return true;
}

// TODO: this is left in-place just to ensure that old crashltyics + new fabric are still compatible
// with each other. As a happy bonus, if that situation does come up, this will also fix the bug
// that was preventing compact unwind on arm64 + iOS 9 from working correctly.
void FIRCLSMachOSliceGetUnwindInformation(FIRCLSMachOSliceRef slice,
                                          const void** ehFrame,
                                          const void** unwindInfo) {
  if (!unwindInfo && !ehFrame) {
    return;
  }

  bool found = false;
  intptr_t slide = 0;

  // This is inefficient, but we have no other safe way to do this correctly. Modifying the
  // FIRCLSMachOSlice structure is tempting, but could introduce weird binary-compatibility issues
  // with version mis-matches.
  for (uint32_t i = 0; i < _dyld_image_count(); ++i) {
    const struct mach_header* header = _dyld_get_image_header(i);

    if (header == slice->startAddress) {
      found = true;
      slide = _dyld_get_image_vmaddr_slide(i);
      break;
    }
  }

  // make sure we were able to find a matching value
  if (!found) {
    return;
  }

  FIRCLSMachOSection section;

  if (unwindInfo) {
    if (FIRCLSMachOSliceInitSectionByName(slice, SEG_TEXT, "__unwind_info", &section)) {
      *unwindInfo = (void*)(section.addr + slide);
    }
  }

  if (ehFrame) {
    if (FIRCLSMachOSliceInitSectionByName(slice, SEG_TEXT, "__eh_frame", &section)) {
      *ehFrame = (void*)(section.addr + slide);
    }
  }
}

uint8_t const* FIRCLSMachOGetUUID(const struct load_command* cmd) {
  return ((const struct uuid_command*)cmd)->uuid;
}

const char* FIRCLSMachOGetDylibPath(const struct load_command* cmd) {
  const struct dylib_command* dylibcmd = (const struct dylib_command*)cmd;

  return (const char*)((uintptr_t)cmd + dylibcmd->dylib.name.offset);
}

bool FIRCLSMachOGetEncrypted(const struct load_command* cmd) {
  return ((struct encryption_info_command*)cmd)->cryptid > 0;
}

static FIRCLSMachOVersion FIRCLSMachOVersionFromEncoded(uint32_t encoded) {
  FIRCLSMachOVersion version;

  version.major = (encoded & 0xffff0000) >> 16;
  version.minor = (encoded & 0x0000ff00) >> 8;
  version.bugfix = encoded & 0x000000ff;

  return version;
}

FIRCLSMachOVersion FIRCLSMachOGetMinimumOSVersion(const struct load_command* cmd) {
  return FIRCLSMachOVersionFromEncoded(((const struct version_min_command*)cmd)->version);
}

FIRCLSMachOVersion FIRCLSMachOGetLinkedSDKVersion(const struct load_command* cmd) {
  return FIRCLSMachOVersionFromEncoded(((const struct version_min_command*)cmd)->sdk);
}

FIRCLSMachOSegmentCommand FIRCLSMachOGetSegmentCommand(const struct load_command* cmd) {
  FIRCLSMachOSegmentCommand segmentCommand;

  memset(&segmentCommand, 0, sizeof(FIRCLSMachOSegmentCommand));

  if (!cmd) {
    return segmentCommand;
  }

  if (cmd->cmd == LC_SEGMENT) {
    struct segment_command* segCmd = (struct segment_command*)cmd;

    memcpy(segmentCommand.segname, segCmd->segname, 16);
    segmentCommand.vmaddr = segCmd->vmaddr;
    segmentCommand.vmsize = segCmd->vmsize;
  } else if (cmd->cmd == LC_SEGMENT_64) {
    struct segment_command_64* segCmd = (struct segment_command_64*)cmd;

    memcpy(segmentCommand.segname, segCmd->segname, 16);
    segmentCommand.vmaddr = segCmd->vmaddr;
    segmentCommand.vmsize = segCmd->vmsize;
  }

  return segmentCommand;
}

NSString* FIRCLSMachONormalizeUUID(CFUUIDBytes* uuidBytes) {
  CFUUIDRef uuid = CFUUIDCreateFromUUIDBytes(kCFAllocatorDefault, *uuidBytes);

  NSString* string = CFBridgingRelease(CFUUIDCreateString(kCFAllocatorDefault, uuid));

  CFRelease(uuid);

  return [[string stringByReplacingOccurrencesOfString:@"-" withString:@""] lowercaseString];
}

NSString* FIRCLSMachOFormatVersion(FIRCLSMachOVersion* version) {
  if (!version) {
    return nil;
  }

  return [NSString stringWithFormat:@"%d.%d.%d", version->major, version->minor, version->bugfix];
}
