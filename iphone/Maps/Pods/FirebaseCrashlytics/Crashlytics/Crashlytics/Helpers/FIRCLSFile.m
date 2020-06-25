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

#include "FIRCLSFile.h"

#include "FIRCLSByteUtility.h"
#include "FIRCLSUtility.h"

#if TARGET_OS_MAC
#include <Foundation/Foundation.h>
#endif

#include <sys/stat.h>

#include <stdio.h>
#include <string.h>

#include <unistd.h>

// uint64_t should only have max 19 chars in base 10, and less in base 16
static const size_t FIRCLSUInt64StringBufferLength = 21;
static const size_t FIRCLSStringBufferLength = 16;
const size_t FIRCLSWriteBufferLength = 1000;

static bool FIRCLSFileInit(FIRCLSFile* file, int fdm, bool appendMode, bool bufferWrites);

static void FIRCLSFileWriteToFileDescriptorOrBuffer(FIRCLSFile* file,
                                                    const char* string,
                                                    size_t length);
static void FIRCLSFileWriteToBuffer(FIRCLSFile* file, const char* string, size_t length);
static void FIRCLSFileWriteToFileDescriptor(FIRCLSFile* file, const char* string, size_t length);

short FIRCLSFilePrepareUInt64(char* buffer, uint64_t number, bool hex);

static void FIRCLSFileWriteString(FIRCLSFile* file, const char* string);
static void FIRCLSFileWriteHexEncodedString(FIRCLSFile* file, const char* string);
static void FIRCLSFileWriteBool(FIRCLSFile* file, bool value);

static void FIRCLSFileWriteCollectionStart(FIRCLSFile* file, const char openingChar);
static void FIRCLSFileWriteCollectionEnd(FIRCLSFile* file, const char closingChar);
static void FIRCLSFileWriteColletionEntryProlog(FIRCLSFile* file);
static void FIRCLSFileWriteColletionEntryEpilog(FIRCLSFile* file);

#define CLS_FILE_DEBUG_LOGGING 0

#pragma mark - File Structure
static bool FIRCLSFileInit(FIRCLSFile* file, int fd, bool appendMode, bool bufferWrites) {
  if (!file) {
    FIRCLSSDKLog("Error: file is null\n");
    return false;
  }

  if (fd < 0) {
    FIRCLSSDKLog("Error: file descriptor invalid\n");
    return false;
  }

  memset(file, 0, sizeof(FIRCLSFile));

  file->fd = fd;

  file->bufferWrites = bufferWrites;
  if (bufferWrites) {
    file->writeBuffer = malloc(FIRCLSWriteBufferLength * sizeof(char));
    if (!file->writeBuffer) {
      FIRCLSErrorLog(@"Unable to malloc in FIRCLSFileInit");
      return false;
    }

    file->writeBufferLength = 0;
  }

  file->writtenLength = 0;
  if (appendMode) {
    struct stat fileStats;
    fstat(fd, &fileStats);
    off_t currentFileSize = fileStats.st_size;
    if (currentFileSize > 0) {
      file->writtenLength += currentFileSize;
    }
  }

  return true;
}

bool FIRCLSFileInitWithPath(FIRCLSFile* file, const char* path, bool bufferWrites) {
  return FIRCLSFileInitWithPathMode(file, path, true, bufferWrites);
}

bool FIRCLSFileInitWithPathMode(FIRCLSFile* file,
                                const char* path,
                                bool appendMode,
                                bool bufferWrites) {
  if (!file) {
    FIRCLSSDKLog("Error: file is null\n");
    return false;
  }

  int mask = O_WRONLY | O_CREAT;

  if (appendMode) {
    mask |= O_APPEND;
  } else {
    mask |= O_TRUNC;
  }

  // make sure to call FIRCLSFileInit no matter what
  int fd = -1;
  if (path) {
#if TARGET_OS_IPHONE
    /*
     * data-protected non-portable open(2) :
     * int open_dprotected_np(user_addr_t path, int flags, int class, int dpflags, int mode)
     */
    fd = open_dprotected_np(path, mask, 4, 0, 0644);
#else
    fd = open(path, mask, 0644);
#endif

    if (fd < 0) {
      FIRCLSSDKLog("Error: Unable to open file %s\n", strerror(errno));
    }
  }

  return FIRCLSFileInit(file, fd, appendMode, bufferWrites);
}

bool FIRCLSFileClose(FIRCLSFile* file) {
  return FIRCLSFileCloseWithOffset(file, NULL);
}

bool FIRCLSFileCloseWithOffset(FIRCLSFile* file, off_t* finalSize) {
  if (!FIRCLSIsValidPointer(file)) {
    return false;
  }

  if (file->bufferWrites && FIRCLSIsValidPointer(file->writeBuffer)) {
    if (file->writeBufferLength > 0) {
      FIRCLSFileFlushWriteBuffer(file);
    }
    free(file->writeBuffer);
  }

  if (FIRCLSIsValidPointer(finalSize)) {
    *finalSize = file->writtenLength;
  }

  if (close(file->fd) != 0) {
    FIRCLSSDKLog("Error: Unable to close file %s\n", strerror(errno));
    return false;
  }

  memset(file, 0, sizeof(FIRCLSFile));
  file->fd = -1;

  return true;
}

bool FIRCLSFileIsOpen(FIRCLSFile* file) {
  if (!FIRCLSIsValidPointer(file)) {
    return false;
  }

  return file->fd > -1;
}

#pragma mark - Core Writing API
void FIRCLSFileFlushWriteBuffer(FIRCLSFile* file) {
  if (!FIRCLSIsValidPointer(file)) {
    return;
  }

  if (!file->bufferWrites) {
    return;
  }

  FIRCLSFileWriteToFileDescriptor(file, file->writeBuffer, file->writeBufferLength);
  file->writeBufferLength = 0;
}

static void FIRCLSFileWriteToFileDescriptorOrBuffer(FIRCLSFile* file,
                                                    const char* string,
                                                    size_t length) {
  if (file->bufferWrites) {
    if (file->writeBufferLength + length > FIRCLSWriteBufferLength - 1) {
      // fill remaining space in buffer
      size_t remainingSpace = FIRCLSWriteBufferLength - file->writeBufferLength - 1;
      FIRCLSFileWriteToBuffer(file, string, remainingSpace);
      FIRCLSFileFlushWriteBuffer(file);

      // write remainder of string to newly-emptied buffer
      size_t remainingLength = length - remainingSpace;
      FIRCLSFileWriteToFileDescriptorOrBuffer(file, string + remainingSpace, remainingLength);
    } else {
      FIRCLSFileWriteToBuffer(file, string, length);
    }
  } else {
    FIRCLSFileWriteToFileDescriptor(file, string, length);
  }
}

static void FIRCLSFileWriteToFileDescriptor(FIRCLSFile* file, const char* string, size_t length) {
  if (!FIRCLSFileWriteWithRetries(file->fd, string, length)) {
    return;
  }

  file->writtenLength += length;
}

// Beware calling this method directly: it will truncate the input string if it's longer
// than the remaining space in the buffer. It's safer to call through
// FIRCLSFileWriteToFileDescriptorOrBuffer.
static void FIRCLSFileWriteToBuffer(FIRCLSFile* file, const char* string, size_t length) {
  size_t writeLength = length;
  if (file->writeBufferLength + writeLength > FIRCLSWriteBufferLength - 1) {
    writeLength = FIRCLSWriteBufferLength - file->writeBufferLength - 1;
  }
  strncpy(file->writeBuffer + file->writeBufferLength, string, writeLength);
  file->writeBufferLength += writeLength;
  file->writeBuffer[file->writeBufferLength] = '\0';
}

bool FIRCLSFileLoopWithWriteBlock(const void* buffer,
                                  size_t length,
                                  ssize_t (^writeBlock)(const void* buf, size_t len)) {
  for (size_t count = 0; length > 0 && count < CLS_FILE_MAX_WRITE_ATTEMPTS; ++count) {
    // try to write all that is left
    ssize_t ret = writeBlock(buffer, length);
    if (ret >= 0 && ret == length) {
      return true;
    }

    // Write was unsuccessful (out of space, etc)
    if (ret < 0) {
      return false;
    }

    // We wrote more bytes than we expected, abort
    if (ret > length) {
      return false;
    }

    // wrote a portion of the data, adjust and keep trying
    if (ret > 0) {
      length -= ret;
      buffer += ret;
      continue;
    }

    // return value is <= 0, which is an error
    break;
  }

  return false;
}

bool FIRCLSFileWriteWithRetries(int fd, const void* buffer, size_t length) {
  return FIRCLSFileLoopWithWriteBlock(buffer, length,
                                      ^ssize_t(const void* partialBuffer, size_t partialLength) {
                                        return write(fd, partialBuffer, partialLength);
                                      });
}

#pragma mark - Strings

static void FIRCLSFileWriteUnbufferedStringWithSuffix(FIRCLSFile* file,
                                                      const char* string,
                                                      size_t length,
                                                      char suffix) {
  char suffixBuffer[2];

  // collaspe the quote + suffix into one single write call, for a small performance win
  suffixBuffer[0] = '"';
  suffixBuffer[1] = suffix;

  FIRCLSFileWriteToFileDescriptorOrBuffer(file, "\"", 1);
  FIRCLSFileWriteToFileDescriptorOrBuffer(file, string, length);
  FIRCLSFileWriteToFileDescriptorOrBuffer(file, suffixBuffer, suffix == 0 ? 1 : 2);
}

static void FIRCLSFileWriteStringWithSuffix(FIRCLSFile* file,
                                            const char* string,
                                            size_t length,
                                            char suffix) {
  // 2 for quotes, 1 for suffix (if present) and 1 more for null character
  const size_t maxStringSize = FIRCLSStringBufferLength - (suffix == 0 ? 3 : 4);

  if (length >= maxStringSize) {
    FIRCLSFileWriteUnbufferedStringWithSuffix(file, string, length, suffix);
    return;
  }

  // we are trying to achieve this in one write call
  // <"><string contents><"><suffix>

  char buffer[FIRCLSStringBufferLength];

  buffer[0] = '"';

  strncpy(buffer + 1, string, length);

  buffer[length + 1] = '"';
  length += 2;

  if (suffix) {
    buffer[length] = suffix;
    length += 1;
  }

  // Always add the terminator. strncpy above would copy the terminator, if we supplied length + 1,
  // but since we do this suffix adjustment here, it's easier to just fix it up in both cases.
  buffer[length + 1] = 0;

  FIRCLSFileWriteToFileDescriptorOrBuffer(file, buffer, length);
}

void FIRCLSFileWriteString(FIRCLSFile* file, const char* string) {
  if (!string) {
    FIRCLSFileWriteToFileDescriptorOrBuffer(file, "null", 4);
    return;
  }

  FIRCLSFileWriteStringWithSuffix(file, string, strlen(string), 0);
}

void FIRCLSFileWriteHexEncodedString(FIRCLSFile* file, const char* string) {
  if (!file) {
    return;
  }

  if (!string) {
    FIRCLSFileWriteToFileDescriptorOrBuffer(file, "null", 4);
    return;
  }

  char buffer[CLS_FILE_HEX_BUFFER];

  memset(buffer, 0, sizeof(buffer));

  size_t length = strlen(string);

  FIRCLSFileWriteToFileDescriptorOrBuffer(file, "\"", 1);

  int bufferIndex = 0;
  for (int i = 0; i < length; ++i) {
    FIRCLSHexFromByte(string[i], &buffer[bufferIndex]);

    bufferIndex += 2;  // 1 char => 2 hex values at a time

    // we can continue only if we have enough space for two more hex
    // characters *and* a terminator. So, we need three total chars
    // of space
    if (bufferIndex >= CLS_FILE_HEX_BUFFER) {
      FIRCLSFileWriteToFileDescriptorOrBuffer(file, buffer, CLS_FILE_HEX_BUFFER);
      bufferIndex = 0;
    }
  }

  // Copy the remainder, which could even be the entire string, if it
  // fit into the buffer completely. Be careful with bounds checking here.
  // The string needs to be non-empty, and we have to have copied at least
  // one pair of hex characters in.
  if (bufferIndex > 0 && length > 0) {
    FIRCLSFileWriteToFileDescriptorOrBuffer(file, buffer, bufferIndex);
  }

  FIRCLSFileWriteToFileDescriptorOrBuffer(file, "\"", 1);
}

#pragma mark - Integers
void FIRCLSFileWriteUInt64(FIRCLSFile* file, uint64_t number, bool hex) {
  char buffer[FIRCLSUInt64StringBufferLength];
  short i = FIRCLSFilePrepareUInt64(buffer, number, hex);
  char* beginning = &buffer[i];  // Write from a pointer to the begining of the string.
  FIRCLSFileWriteToFileDescriptorOrBuffer(file, beginning, strlen(beginning));
}

void FIRCLSFileFDWriteUInt64(int fd, uint64_t number, bool hex) {
  char buffer[FIRCLSUInt64StringBufferLength];
  short i = FIRCLSFilePrepareUInt64(buffer, number, hex);
  char* beginning = &buffer[i];  // Write from a pointer to the begining of the string.
  FIRCLSFileWriteWithRetries(fd, beginning, strlen(beginning));
}

void FIRCLSFileWriteInt64(FIRCLSFile* file, int64_t number) {
  if (number < 0) {
    FIRCLSFileWriteToFileDescriptorOrBuffer(file, "-", 1);
    number *= -1;  // make it positive
  }

  FIRCLSFileWriteUInt64(file, number, false);
}

void FIRCLSFileFDWriteInt64(int fd, int64_t number) {
  if (number < 0) {
    FIRCLSFileWriteWithRetries(fd, "-", 1);
    number *= -1;  // make it positive
  }

  FIRCLSFileFDWriteUInt64(fd, number, false);
}

short FIRCLSFilePrepareUInt64(char* buffer, uint64_t number, bool hex) {
  uint32_t base = hex ? 16 : 10;

  // zero it out, which will add a terminator
  memset(buffer, 0, FIRCLSUInt64StringBufferLength);

  // TODO: look at this closer
  // I'm pretty sure there is a bug in this code that
  // can result in numbers with leading zeros. Technically,
  // those are not valid json.

  // Set current index.
  short i = FIRCLSUInt64StringBufferLength - 1;

  // Loop through filling in the chars from the end.
  do {
    char value = number % base + '0';
    if (value > '9') {
      value += 'a' - '9' - 1;
    }

    buffer[--i] = value;
  } while ((number /= base) > 0 && i > 0);

  // returns index pointing to the beginning of the string.
  return i;
}

void FIRCLSFileWriteBool(FIRCLSFile* file, bool value) {
  if (value) {
    FIRCLSFileWriteToFileDescriptorOrBuffer(file, "true", 4);
  } else {
    FIRCLSFileWriteToFileDescriptorOrBuffer(file, "false", 5);
  }
}

void FIRCLSFileWriteSectionStart(FIRCLSFile* file, const char* name) {
  FIRCLSFileWriteHashStart(file);
  FIRCLSFileWriteHashKey(file, name);
}

void FIRCLSFileWriteSectionEnd(FIRCLSFile* file) {
  FIRCLSFileWriteHashEnd(file);
  FIRCLSFileWriteToFileDescriptorOrBuffer(file, "\n", 1);
}

void FIRCLSFileWriteCollectionStart(FIRCLSFile* file, const char openingChar) {
  char string[2];

  string[0] = ',';
  string[1] = openingChar;

  if (file->needComma) {
    FIRCLSFileWriteToFileDescriptorOrBuffer(file, string, 2);  // write the seperator + opening char
  } else {
    FIRCLSFileWriteToFileDescriptorOrBuffer(file, &string[1], 1);  // write only the opening char
  }

  file->collectionDepth++;

  file->needComma = false;
}

void FIRCLSFileWriteCollectionEnd(FIRCLSFile* file, const char closingChar) {
  FIRCLSFileWriteToFileDescriptorOrBuffer(file, &closingChar, 1);

  if (file->collectionDepth <= 0) {
    //        FIRCLSSafeLog("Collection depth invariant violated\n");
    return;
  }

  file->collectionDepth--;

  file->needComma = file->collectionDepth > 0;
}

void FIRCLSFileWriteColletionEntryProlog(FIRCLSFile* file) {
  if (file->needComma) {
    FIRCLSFileWriteToFileDescriptorOrBuffer(file, ",", 1);
  }
}

void FIRCLSFileWriteColletionEntryEpilog(FIRCLSFile* file) {
  file->needComma = true;
}

void FIRCLSFileWriteHashStart(FIRCLSFile* file) {
  FIRCLSFileWriteCollectionStart(file, '{');
}

void FIRCLSFileWriteHashEnd(FIRCLSFile* file) {
  FIRCLSFileWriteCollectionEnd(file, '}');
}

void FIRCLSFileWriteHashKey(FIRCLSFile* file, const char* key) {
  FIRCLSFileWriteColletionEntryProlog(file);

  FIRCLSFileWriteStringWithSuffix(file, key, strlen(key), ':');

  file->needComma = false;
}

void FIRCLSFileWriteHashEntryUint64(FIRCLSFile* file, const char* key, uint64_t value) {
  // no prolog needed because it comes from the key

  FIRCLSFileWriteHashKey(file, key);
  FIRCLSFileWriteUInt64(file, value, false);

  FIRCLSFileWriteColletionEntryEpilog(file);
}

void FIRCLSFileWriteHashEntryInt64(FIRCLSFile* file, const char* key, int64_t value) {
  // prolog from key
  FIRCLSFileWriteHashKey(file, key);
  FIRCLSFileWriteInt64(file, value);

  FIRCLSFileWriteColletionEntryEpilog(file);
}

void FIRCLSFileWriteHashEntryString(FIRCLSFile* file, const char* key, const char* value) {
  FIRCLSFileWriteHashKey(file, key);
  FIRCLSFileWriteString(file, value);

  FIRCLSFileWriteColletionEntryEpilog(file);
}

void FIRCLSFileWriteHashEntryNSString(FIRCLSFile* file, const char* key, NSString* string) {
  FIRCLSFileWriteHashEntryString(file, key, [string UTF8String]);
}

void FIRCLSFileWriteHashEntryNSStringUnlessNilOrEmpty(FIRCLSFile* file,
                                                      const char* key,
                                                      NSString* string) {
  if ([string length] > 0) {
    FIRCLSFileWriteHashEntryString(file, key, [string UTF8String]);
  }
}

void FIRCLSFileWriteHashEntryHexEncodedString(FIRCLSFile* file,
                                              const char* key,
                                              const char* value) {
  FIRCLSFileWriteHashKey(file, key);
  FIRCLSFileWriteHexEncodedString(file, value);

  FIRCLSFileWriteColletionEntryEpilog(file);
}

void FIRCLSFileWriteHashEntryBoolean(FIRCLSFile* file, const char* key, bool value) {
  FIRCLSFileWriteHashKey(file, key);
  FIRCLSFileWriteBool(file, value);

  FIRCLSFileWriteColletionEntryEpilog(file);
}

void FIRCLSFileWriteArrayStart(FIRCLSFile* file) {
  FIRCLSFileWriteCollectionStart(file, '[');
}

void FIRCLSFileWriteArrayEnd(FIRCLSFile* file) {
  FIRCLSFileWriteCollectionEnd(file, ']');
}

void FIRCLSFileWriteArrayEntryUint64(FIRCLSFile* file, uint64_t value) {
  FIRCLSFileWriteColletionEntryProlog(file);

  FIRCLSFileWriteUInt64(file, value, false);

  FIRCLSFileWriteColletionEntryEpilog(file);
}

void FIRCLSFileWriteArrayEntryString(FIRCLSFile* file, const char* value) {
  FIRCLSFileWriteColletionEntryProlog(file);

  FIRCLSFileWriteString(file, value);

  FIRCLSFileWriteColletionEntryEpilog(file);
}

void FIRCLSFileWriteArrayEntryHexEncodedString(FIRCLSFile* file, const char* value) {
  FIRCLSFileWriteColletionEntryProlog(file);

  FIRCLSFileWriteHexEncodedString(file, value);

  FIRCLSFileWriteColletionEntryEpilog(file);
}

NSArray* FIRCLSFileReadSections(const char* path,
                                bool deleteOnFailure,
                                NSObject* (^transformer)(id obj)) {
  if (!FIRCLSIsValidPointer(path)) {
    FIRCLSSDKLogError("Error: input path is invalid\n");
    return nil;
  }

  NSString* pathString = [NSString stringWithUTF8String:path];
  NSString* contents = [NSString stringWithContentsOfFile:pathString
                                                 encoding:NSUTF8StringEncoding
                                                    error:nil];
  NSArray* components = [contents componentsSeparatedByString:@"\n"];

  if (!components) {
    if (deleteOnFailure) {
      unlink(path);
    }

    FIRCLSSDKLog("Unable to read file %s\n", path);
    return nil;
  }

  NSMutableArray* array = [NSMutableArray array];

  // loop through all the entires, and
  for (NSString* component in components) {
    NSData* data = [component dataUsingEncoding:NSUTF8StringEncoding];

    id obj = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
    if (!obj) {
      continue;
    }

    if (transformer) {
      obj = transformer(obj);
    }

    if (!obj) {
      continue;
    }

    [array addObject:obj];
  }

  return array;
}

NSString* FIRCLSFileHexEncodeString(const char* string) {
  size_t length = strlen(string);
  char* encodedBuffer = malloc(length * 2 + 1);

  if (!encodedBuffer) {
    FIRCLSErrorLog(@"Unable to malloc in FIRCLSFileHexEncodeString");
    return nil;
  }

  memset(encodedBuffer, 0, length * 2 + 1);

  int bufferIndex = 0;
  for (int i = 0; i < length; ++i) {
    FIRCLSHexFromByte(string[i], &encodedBuffer[bufferIndex]);

    bufferIndex += 2;  // 1 char => 2 hex values at a time
  }

  NSString* stringObject = [NSString stringWithUTF8String:encodedBuffer];

  free(encodedBuffer);

  return stringObject;
}

NSString* FIRCLSFileHexDecodeString(const char* string) {
  size_t length = strlen(string);
  char* decodedBuffer = malloc(length);  // too long, but safe
  if (!decodedBuffer) {
    FIRCLSErrorLog(@"Unable to malloc in FIRCLSFileHexDecodeString");
    return nil;
  }

  memset(decodedBuffer, 0, length);

  for (int i = 0; i < length / 2; ++i) {
    size_t index = i * 2;

    uint8_t hiNybble = FIRCLSNybbleFromChar(string[index]);
    uint8_t lowNybble = FIRCLSNybbleFromChar(string[index + 1]);

    if (hiNybble == FIRCLSInvalidCharNybble || lowNybble == FIRCLSInvalidCharNybble) {
      // char is invalid, abort loop
      break;
    }

    decodedBuffer[i] = (hiNybble << 4) | lowNybble;
  }

  NSString* strObject = [NSString stringWithUTF8String:decodedBuffer];

  free(decodedBuffer);

  return strObject;
}
