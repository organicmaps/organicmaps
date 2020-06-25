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

#include "FIRCLSBinaryImage.h"

#include <libkern/OSAtomic.h>
#include <mach-o/dyld.h>

#include <mach-o/getsect.h>

#include <stdatomic.h>

#include "FIRCLSByteUtility.h"
#include "FIRCLSFeatures.h"
#include "FIRCLSFile.h"
#include "FIRCLSGlobals.h"
#include "FIRCLSHost.h"
#include "FIRCLSMachO.h"
#include "FIRCLSUtility.h"

#include <dispatch/dispatch.h>

// this is defined only if __OPEN_SOURCE__ is *not* defined in the TVOS SDK's mach-o/loader.h
// also, it has not yet made it back to the OSX SDKs, for example
#ifndef LC_VERSION_MIN_TVOS
#define LC_VERSION_MIN_TVOS 0x2F
#endif

#pragma mark Prototypes
static bool FIRCLSBinaryImageOpenIfNeeded(bool* needsClosing);

static void FIRCLSBinaryImageAddedCallback(const struct mach_header* mh, intptr_t vmaddr_slide);
static void FIRCLSBinaryImageRemovedCallback(const struct mach_header* mh, intptr_t vmaddr_slide);
static void FIRCLSBinaryImageChanged(bool added,
                                     const struct mach_header* mh,
                                     intptr_t vmaddr_slide);
static bool FIRCLSBinaryImageFillInImageDetails(FIRCLSBinaryImageDetails* details);

static void FIRCLSBinaryImageStoreNode(bool added, FIRCLSBinaryImageDetails imageDetails);
static void FIRCLSBinaryImageRecordSlice(bool added, const FIRCLSBinaryImageDetails imageDetails);

#pragma mark - Core API
void FIRCLSBinaryImageInit(FIRCLSBinaryImageReadOnlyContext* roContext,
                           FIRCLSBinaryImageReadWriteContext* rwContext) {
  // initialize our node array to all zeros
  memset(&_firclsContext.writable->binaryImage, 0, sizeof(_firclsContext.writable->binaryImage));
  _firclsContext.writable->binaryImage.file.fd = -1;

  dispatch_async(FIRCLSGetBinaryImageQueue(), ^{
    if (!FIRCLSUnlinkIfExists(_firclsContext.readonly->binaryimage.path)) {
      FIRCLSSDKLog("Unable to reset the binary image log file %s\n", strerror(errno));
    }

    bool needsClosing;  // unneeded
    if (!FIRCLSBinaryImageOpenIfNeeded(&needsClosing)) {
      FIRCLSSDKLog("Error: Unable to open the binary image log file during init\n");
    }
  });

  _dyld_register_func_for_add_image(FIRCLSBinaryImageAddedCallback);
  _dyld_register_func_for_remove_image(FIRCLSBinaryImageRemovedCallback);

  dispatch_async(FIRCLSGetBinaryImageQueue(), ^{
    FIRCLSFileClose(&_firclsContext.writable->binaryImage.file);
  });
}

static bool FIRCLSBinaryImageOpenIfNeeded(bool* needsClosing) {
  if (!FIRCLSIsValidPointer(_firclsContext.writable)) {
    return false;
  }

  if (!FIRCLSIsValidPointer(_firclsContext.readonly)) {
    return false;
  }

  if (!FIRCLSIsValidPointer(needsClosing)) {
    return false;
  }

  *needsClosing = false;

  if (FIRCLSFileIsOpen(&_firclsContext.writable->binaryImage.file)) {
    return true;
  }

  if (!FIRCLSFileInitWithPath(&_firclsContext.writable->binaryImage.file,
                              _firclsContext.readonly->binaryimage.path, false)) {
    FIRCLSSDKLog("Error: unable to open binary image log file\n");
    return false;
  }

  *needsClosing = true;

  return true;
}

#if CLS_COMPACT_UNWINDING_SUPPORTED
bool FIRCLSBinaryImageSafeFindImageForAddress(uintptr_t address,
                                              FIRCLSBinaryImageRuntimeNode* image) {
  if (!FIRCLSContextIsInitialized()) {
    return false;
  }

  if (address == 0) {
    return false;
  }

  if (!FIRCLSIsValidPointer(image)) {
    return false;
  }

  FIRCLSBinaryImageRuntimeNode* nodes = _firclsContext.writable->binaryImage.nodes;
  if (!nodes) {
    FIRCLSSDKLogError("The node structure is NULL\n");
    return false;
  }

  for (uint32_t i = 0; i < CLS_BINARY_IMAGE_RUNTIME_NODE_COUNT; ++i) {
    FIRCLSBinaryImageRuntimeNode* node = &nodes[i];
    if (!FIRCLSIsValidPointer(node)) {
      FIRCLSSDKLog(
          "Invalid node pointer encountered in context's writable binary image at index %i", i);
      continue;
    }

    if ((address >= (uintptr_t)node->baseAddress) &&
        (address < (uintptr_t)node->baseAddress + node->size)) {
      *image = *node;  // copy the image
      return true;
    }
  }

  return false;
}

bool FIRCLSBinaryImageSafeHasUnwindInfo(FIRCLSBinaryImageRuntimeNode* image) {
  return FIRCLSIsValidPointer(image->unwindInfo);
}
#endif

bool FIRCLSBinaryImageFindImageForUUID(const char* uuidString,
                                       FIRCLSBinaryImageDetails* imageDetails) {
  if (!imageDetails || !uuidString) {
    FIRCLSSDKLog("null input\n");
    return false;
  }

  uint32_t imageCount = _dyld_image_count();

  for (uint32_t i = 0; i < imageCount; ++i) {
    const struct mach_header* mh = _dyld_get_image_header(i);

    FIRCLSBinaryImageDetails image;

    image.slice = FIRCLSMachOSliceWithHeader((void*)mh);
    FIRCLSBinaryImageFillInImageDetails(&image);

    if (strncmp(uuidString, image.uuidString, FIRCLSUUIDStringLength) == 0) {
      *imageDetails = image;
      return true;
    }
  }

  return false;
}

#pragma mark - DYLD callback handlers
static void FIRCLSBinaryImageAddedCallback(const struct mach_header* mh, intptr_t vmaddr_slide) {
  FIRCLSBinaryImageChanged(true, mh, vmaddr_slide);
}

static void FIRCLSBinaryImageRemovedCallback(const struct mach_header* mh, intptr_t vmaddr_slide) {
  FIRCLSBinaryImageChanged(false, mh, vmaddr_slide);
}

#if CLS_BINARY_IMAGE_RUNTIME_NODE_RECORD_NAME
static bool FIRCLSBinaryImagePopulateRuntimeNodeName(FIRCLSBinaryImageDetails* details) {
  if (!FIRCLSIsValidPointer(details)) {
    return false;
  }

  memset(details->node.name, 0, CLS_BINARY_IMAGE_RUNTIME_NODE_NAME_SIZE);

  // We have limited storage space for the name. And, we really want to store
  // "CoreFoundation", not "/System/Library/Fram", so we have to play tricks
  // to make sure we get the right side of the string.
  const char* imageName = FIRCLSMachOSliceGetExecutablePath(&details->slice);
  if (!imageName) {
    return false;
  }

  const size_t imageNameLength = strlen(imageName);

  // Remember to leave one character for null-termination.
  if (imageNameLength > CLS_BINARY_IMAGE_RUNTIME_NODE_NAME_SIZE - 1) {
    imageName = imageName + (imageNameLength - (CLS_BINARY_IMAGE_RUNTIME_NODE_NAME_SIZE - 1));
  }

  // subtract one to make sure the string is always null-terminated
  strncpy(details->node.name, imageName, CLS_BINARY_IMAGE_RUNTIME_NODE_NAME_SIZE - 1);

  return true;
}
#endif

// There were plans later to replace this with FIRCLSMachO
static FIRCLSMachOSegmentCommand FIRCLSBinaryImageMachOGetSegmentCommand(
    const struct load_command* cmd) {
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

static bool FIRCLSBinaryImageMachOSliceInitSectionByName(FIRCLSMachOSliceRef slice,
                                                         const char* segName,
                                                         const char* sectionName,
                                                         FIRCLSMachOSection* section) {
  if (!FIRCLSIsValidPointer(slice)) {
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

static bool FIRCLSBinaryImageFillInImageDetails(FIRCLSBinaryImageDetails* details) {
  if (!FIRCLSIsValidPointer(details)) {
    return false;
  }

  if (!FIRCLSIsValidPointer(details->slice.startAddress)) {
    return false;
  }

#if CLS_BINARY_IMAGE_RUNTIME_NODE_RECORD_NAME
  // this is done for debugging purposes, so if it fails, its ok to continue
  FIRCLSBinaryImagePopulateRuntimeNodeName(details);
#endif

  // This cast might look a little dubious, but its just because we're using the same
  // struct types in a few different places.
  details->node.baseAddress = (void* volatile)details->slice.startAddress;

  FIRCLSMachOSliceEnumerateLoadCommands(
      &details->slice, ^(uint32_t type, uint32_t size, const struct load_command* cmd) {
        switch (type) {
          case LC_UUID: {
            const uint8_t* uuid = FIRCLSMachOGetUUID(cmd);
            FIRCLSSafeHexToString(uuid, 16, details->uuidString);
          } break;
          case LC_ENCRYPTION_INFO:
            details->encrypted = FIRCLSMachOGetEncrypted(cmd);
            break;
          case LC_SEGMENT:
          case LC_SEGMENT_64: {
            FIRCLSMachOSegmentCommand segmentCommand = FIRCLSBinaryImageMachOGetSegmentCommand(cmd);

            if (strncmp(segmentCommand.segname, SEG_TEXT, sizeof(SEG_TEXT)) == 0) {
              details->node.size = segmentCommand.vmsize;
            }
          } break;
          case LC_VERSION_MIN_MACOSX:
          case LC_VERSION_MIN_IPHONEOS:
          case LC_VERSION_MIN_TVOS:
          case LC_VERSION_MIN_WATCHOS:
            details->minSDK = FIRCLSMachOGetMinimumOSVersion(cmd);
            details->builtSDK = FIRCLSMachOGetLinkedSDKVersion(cmd);
            break;
        }
      });

  // We look up the section we want, and we *should* be able to use:
  //
  // address of data we want = start address + section.offset
  //
  // However, the offset value is coming back funky in iOS 9. So, instead we look up the address
  // the section should be loaded at, and compute the offset by looking up the address of the
  // segment itself.

  FIRCLSMachOSection section;

#if CLS_COMPACT_UNWINDING_SUPPORTED
  if (FIRCLSBinaryImageMachOSliceInitSectionByName(&details->slice, SEG_TEXT, "__unwind_info",
                                                   &section)) {
    details->node.unwindInfo = (void*)(section.addr + details->vmaddr_slide);
  }
#endif

#if CLS_DWARF_UNWINDING_SUPPORTED
  if (FIRCLSBinaryImageMachOSliceInitSectionByName(&details->slice, SEG_TEXT, "__eh_frame",
                                                   &section)) {
    details->node.ehFrame = (void*)(section.addr + details->vmaddr_slide);
  }
#endif

  if (FIRCLSBinaryImageMachOSliceInitSectionByName(&details->slice, SEG_DATA, "__crash_info",
                                                   &section)) {
    details->node.crashInfo = (void*)(section.addr + details->vmaddr_slide);
  }

  return true;
}

static void FIRCLSBinaryImageChanged(bool added,
                                     const struct mach_header* mh,
                                     intptr_t vmaddr_slide) {
  //    FIRCLSSDKLog("Binary image %s %p\n", added ? "loaded" : "unloaded", mh);

  FIRCLSBinaryImageDetails imageDetails;

  memset(&imageDetails, 0, sizeof(FIRCLSBinaryImageDetails));

  imageDetails.slice = FIRCLSMachOSliceWithHeader((void*)mh);
  imageDetails.vmaddr_slide = vmaddr_slide;
  FIRCLSBinaryImageFillInImageDetails(&imageDetails);

  // this is an atomic operation
  FIRCLSBinaryImageStoreNode(added, imageDetails);

  // this isn't, so do it on a serial queue
  dispatch_async(FIRCLSGetBinaryImageQueue(), ^{
    FIRCLSBinaryImageRecordSlice(added, imageDetails);
  });
}

#pragma mark - In-Memory Storage
static void FIRCLSBinaryImageStoreNode(bool added, FIRCLSBinaryImageDetails imageDetails) {
  // This function is mutating a structure that needs to be accessed at crash time. We
  // need to make sure the structure is always in as valid a state as possible.
  //    FIRCLSSDKLog("Storing %s node %p\n", added ? "loaded" : "unloaded",
  //    (void*)imageDetails.node.baseAddress);

  if (!_firclsContext.writable) {
    FIRCLSSDKLog("Error: Writable context is NULL\n");
    return;
  }

  void* searchAddress = NULL;
  bool success = false;
  FIRCLSBinaryImageRuntimeNode* nodes = _firclsContext.writable->binaryImage.nodes;

  if (!added) {
    // capture the search address first
    searchAddress = imageDetails.node.baseAddress;

    // If we are removing a node, we need to set its entries to zero. By clearing all of
    // these values, we can just copy in imageDetails.node. Using memset here is slightly
    // weird, since we have to restore one field. But, this way, if/when the structure changes,
    // we still do the right thing.
    memset(&imageDetails.node, 0, sizeof(FIRCLSBinaryImageRuntimeNode));

    // restore the baseAddress, which just got zeroed, and is used for indexing
    imageDetails.node.baseAddress = searchAddress;
  }

  for (uint32_t i = 0; i < CLS_BINARY_IMAGE_RUNTIME_NODE_COUNT; ++i) {
    FIRCLSBinaryImageRuntimeNode* node = &nodes[i];

    if (!node) {
      FIRCLSSDKLog("Error: Binary image storage is NULL\n");
      break;
    }

    // navigate through the array, looking for our matching address
    if (node->baseAddress != searchAddress) {
      continue;
    }

    // Attempt to swap the base address with whatever we are searching for. Success means that
    // entry has been claims/cleared. Failure means some other thread beat us to it.
    if (atomic_compare_exchange_strong(&node->baseAddress, &searchAddress,
                                       imageDetails.node.baseAddress)) {
      *node = imageDetails.node;
      success = true;

      break;
    }

    // If this is an unload, getting here means two threads unloaded at the same time. I think
    // that's highly unlikely, and possibly even impossible. So, I'm choosing to abort the process
    // at this point.
    if (!added) {
      FIRCLSSDKLog("Error: Failed to swap during image unload\n");
      break;
    }
  }

  if (!success) {
    FIRCLSSDKLog("Error: Unable to track a %s node %p\n", added ? "loaded" : "unloaded",
                 (void*)imageDetails.node.baseAddress);
  }
}

#pragma mark - On-Disk Storage
static void FIRCLSBinaryImageRecordDetails(FIRCLSFile* file,
                                           const FIRCLSBinaryImageDetails imageDetails) {
  if (!file) {
    FIRCLSSDKLog("Error: file is invalid\n");
    return;
  }

  FIRCLSFileWriteHashEntryString(file, "uuid", imageDetails.uuidString);
  FIRCLSFileWriteHashEntryUint64(file, "base", (uintptr_t)imageDetails.slice.startAddress);
  FIRCLSFileWriteHashEntryUint64(file, "size", imageDetails.node.size);
}

static void FIRCLSBinaryImageRecordLibraryFrameworkInfo(FIRCLSFile* file, const char* path) {
  if (!file) {
    FIRCLSSDKLog("Error: file is invalid\n");
    return;
  }

  if (!path) {
    return;
  }

  // Because this function is so expensive, we've decided to omit this info for all Apple-supplied
  // frameworks. This really isn't that bad, because we can know their info ahead of time (within a
  // small margin of error). With this implemenation, we will still record this info for any
  // user-built framework, which in the end is the most important thing.
  if (strncmp(path, "/System", 7) == 0) {
    return;
  }

  // check to see if this is a potential framework bundle
  if (!strstr(path, ".framework")) {
    return;
  }

  // My.framework/Versions/A/My for OS X
  // My.framework/My for iOS

  NSString* frameworkPath = [NSString stringWithUTF8String:path];
#if TARGET_OS_IPHONE
  frameworkPath = [frameworkPath stringByDeletingLastPathComponent];
#else
  frameworkPath = [frameworkPath stringByDeletingLastPathComponent];  // My.framework/Versions/A
  frameworkPath = [frameworkPath stringByDeletingLastPathComponent];  // My.framework/Versions
  frameworkPath = [frameworkPath stringByDeletingLastPathComponent];  // My.framework
#endif

  NSBundle* const bundle = [NSBundle bundleWithPath:frameworkPath];

  if (!bundle) {
    return;
  }

  FIRCLSFileWriteHashEntryNSStringUnlessNilOrEmpty(file, "bundle_id", [bundle bundleIdentifier]);
  FIRCLSFileWriteHashEntryNSStringUnlessNilOrEmpty(
      file, "build_version", [bundle objectForInfoDictionaryKey:@"CFBundleVersion"]);
  FIRCLSFileWriteHashEntryNSStringUnlessNilOrEmpty(
      file, "display_version", [bundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"]);
}

static void FIRCLSBinaryImageRecordSlice(bool added, const FIRCLSBinaryImageDetails imageDetails) {
  bool needsClosing = false;
  if (!FIRCLSBinaryImageOpenIfNeeded(&needsClosing)) {
    FIRCLSSDKLog("Error: unable to open binary image log file\n");
    return;
  }

  FIRCLSFile* file = &_firclsContext.writable->binaryImage.file;

  FIRCLSFileWriteSectionStart(file, added ? "load" : "unload");

  FIRCLSFileWriteHashStart(file);

  const char* path = FIRCLSMachOSliceGetExecutablePath((FIRCLSMachOSliceRef)&imageDetails.slice);

  FIRCLSFileWriteHashEntryString(file, "path", path);

  if (added) {
    // this won't work if the binary has been unloaded
    FIRCLSBinaryImageRecordLibraryFrameworkInfo(file, path);
  }

  FIRCLSBinaryImageRecordDetails(file, imageDetails);

  FIRCLSFileWriteHashEnd(file);

  FIRCLSFileWriteSectionEnd(file);

  if (needsClosing) {
    FIRCLSFileClose(file);
  }
}

bool FIRCLSBinaryImageRecordMainExecutable(FIRCLSFile* file) {
  FIRCLSBinaryImageDetails imageDetails;

  memset(&imageDetails, 0, sizeof(FIRCLSBinaryImageDetails));

  imageDetails.slice = FIRCLSMachOSliceGetCurrent();
  FIRCLSBinaryImageFillInImageDetails(&imageDetails);

  FIRCLSFileWriteSectionStart(file, "executable");
  FIRCLSFileWriteHashStart(file);

  FIRCLSFileWriteHashEntryString(file, "architecture",
                                 FIRCLSMachOSliceGetArchitectureName(&imageDetails.slice));

  FIRCLSBinaryImageRecordDetails(file, imageDetails);
  FIRCLSFileWriteHashEntryBoolean(file, "encrypted", imageDetails.encrypted);
  FIRCLSFileWriteHashEntryString(file, "minimum_sdk_version",
                                 [FIRCLSMachOFormatVersion(&imageDetails.minSDK) UTF8String]);
  FIRCLSFileWriteHashEntryString(file, "built_sdk_version",
                                 [FIRCLSMachOFormatVersion(&imageDetails.builtSDK) UTF8String]);

  FIRCLSFileWriteHashEnd(file);
  FIRCLSFileWriteSectionEnd(file);

  return true;
}
