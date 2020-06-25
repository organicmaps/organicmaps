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

#include <stdbool.h>
#include <stdint.h>
#include <sys/cdefs.h>

#if defined(__OBJC__)
#import <Foundation/Foundation.h>
#endif

__BEGIN_DECLS

typedef struct {
  int fd;
  int collectionDepth;
  bool needComma;

  bool bufferWrites;
  char* writeBuffer;
  size_t writeBufferLength;

  off_t writtenLength;
} FIRCLSFile;
typedef FIRCLSFile* FIRCLSFileRef;

#define CLS_FILE_MAX_STRING_LENGTH (10240)
#define CLS_FILE_HEX_BUFFER \
  (32)  // must be at least 2, and should be even (to account for 2 chars per hex value)
#define CLS_FILE_MAX_WRITE_ATTEMPTS (50)

extern const size_t FIRCLSWriteBufferLength;

// make sure to stop work if either FIRCLSFileInit... method returns false, because the FIRCLSFile
// struct will contain garbage data!
bool FIRCLSFileInitWithPath(FIRCLSFile* file, const char* path, bool bufferWrites);
bool FIRCLSFileInitWithPathMode(FIRCLSFile* file,
                                const char* path,
                                bool appendMode,
                                bool bufferWrites);

void FIRCLSFileFlushWriteBuffer(FIRCLSFile* file);
bool FIRCLSFileClose(FIRCLSFile* file);
bool FIRCLSFileCloseWithOffset(FIRCLSFile* file, off_t* finalSize);
bool FIRCLSFileIsOpen(FIRCLSFile* file);

bool FIRCLSFileLoopWithWriteBlock(const void* buffer,
                                  size_t length,
                                  ssize_t (^writeBlock)(const void* partialBuffer,
                                                        size_t partialLength));
bool FIRCLSFileWriteWithRetries(int fd, const void* buffer, size_t length);

// writing
void FIRCLSFileWriteSectionStart(FIRCLSFile* file, const char* name);
void FIRCLSFileWriteSectionEnd(FIRCLSFile* file);

void FIRCLSFileWriteHashStart(FIRCLSFile* file);
void FIRCLSFileWriteHashEnd(FIRCLSFile* file);
void FIRCLSFileWriteHashKey(FIRCLSFile* file, const char* key);
void FIRCLSFileWriteHashEntryUint64(FIRCLSFile* file, const char* key, uint64_t value);
void FIRCLSFileWriteHashEntryInt64(FIRCLSFile* file, const char* key, int64_t value);
void FIRCLSFileWriteHashEntryString(FIRCLSFile* file, const char* key, const char* value);
#if defined(__OBJC__)
void FIRCLSFileWriteHashEntryNSString(FIRCLSFile* file, const char* key, NSString* string);
void FIRCLSFileWriteHashEntryNSStringUnlessNilOrEmpty(FIRCLSFile* file,
                                                      const char* key,
                                                      NSString* string);
#endif
void FIRCLSFileWriteHashEntryHexEncodedString(FIRCLSFile* file, const char* key, const char* value);
void FIRCLSFileWriteHashEntryBoolean(FIRCLSFile* file, const char* key, bool value);

void FIRCLSFileWriteArrayStart(FIRCLSFile* file);
void FIRCLSFileWriteArrayEnd(FIRCLSFile* file);
void FIRCLSFileWriteArrayEntryUint64(FIRCLSFile* file, uint64_t value);
void FIRCLSFileWriteArrayEntryString(FIRCLSFile* file, const char* value);
void FIRCLSFileWriteArrayEntryHexEncodedString(FIRCLSFile* file, const char* value);

void FIRCLSFileFDWriteUInt64(int fd, uint64_t number, bool hex);
void FIRCLSFileFDWriteInt64(int fd, int64_t number);
void FIRCLSFileWriteUInt64(FIRCLSFile* file, uint64_t number, bool hex);
void FIRCLSFileWriteInt64(FIRCLSFile* file, int64_t number);

#if defined(__OBJC__) && TARGET_OS_MAC
NSArray* FIRCLSFileReadSections(const char* path,
                                bool deleteOnFailure,
                                NSObject* (^transformer)(id obj));
NSString* FIRCLSFileHexEncodeString(const char* string);
NSString* FIRCLSFileHexDecodeString(const char* string);
#endif

__END_DECLS
