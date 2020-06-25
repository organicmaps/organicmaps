/*
 * Copyright 2020 Google
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#import "FIRCLSReportAdapter.h"
#import "FIRCLSReportAdapter_Private.h"

#import "FIRCLSInternalReport.h"
#import "FIRCLSLogger.h"

#import "FIRCLSUserLogging.h"

#import <nanopb/pb.h>
#import <nanopb/pb_decode.h>
#import <nanopb/pb_encode.h>

@implementation FIRCLSReportAdapter

- (instancetype)initWithPath:(NSString *)folderPath googleAppId:(NSString *)googleAppID {
  self = [super init];
  if (self) {
    _folderPath = folderPath;
    _googleAppID = googleAppID;

    [self loadMetaDataFile];

    _report = [self protoReport];
  }
  return self;
}

- (void)dealloc {
  pb_release(google_crashlytics_Report_fields, &_report);
}

//
// MARK: Load from persisted crash files
//

/// Reads from metadata.clsrecord
- (void)loadMetaDataFile {
  NSString *path = [self.folderPath stringByAppendingPathComponent:FIRCLSReportMetadataFile];
  NSDictionary *dict = [FIRCLSReportAdapter combinedDictionariesFromFilePath:path];

  self.identity = [[FIRCLSRecordIdentity alloc] initWithDict:dict[@"identity"]];
  self.host = [[FIRCLSRecordHost alloc] initWithDict:dict[@"host"]];
  self.application = [[FIRCLSRecordApplication alloc] initWithDict:dict[@"application"]];
}

/// Return the persisted crash file as a combined dictionary that way lookups can occur with a key
/// (to avoid ordering dependency)
/// @param filePath Persisted crash file path
+ (NSDictionary *)combinedDictionariesFromFilePath:(NSString *)filePath {
  NSMutableDictionary *joinedDict = [[NSMutableDictionary alloc] init];
  for (NSDictionary *dict in [self dictionariesFromEachLineOfFile:filePath]) {
    [joinedDict addEntriesFromDictionary:dict];
  }
  return joinedDict;
}

/// The persisted crash files contains JSON on separate lines. Read each line and return the JSON
/// data as a dictionary.
/// @param filePath Persisted crash file path
+ (NSArray<NSDictionary *> *)dictionariesFromEachLineOfFile:(NSString *)filePath {
  NSString *content = [[NSString alloc] initWithContentsOfFile:filePath
                                                      encoding:NSUTF8StringEncoding
                                                         error:nil];
  NSArray *lines =
      [content componentsSeparatedByCharactersInSet:NSCharacterSet.newlineCharacterSet];

  NSMutableArray<NSDictionary *> *array = [[NSMutableArray<NSDictionary *> alloc] init];

  int lineNum = 0;
  for (NSString *line in lines) {
    lineNum++;

    if (line.length == 0) {
      // Likely newline at the end of the file
      continue;
    }

    NSError *error;
    NSDictionary *dict =
        [NSJSONSerialization JSONObjectWithData:[line dataUsingEncoding:NSUTF8StringEncoding]
                                        options:0
                                          error:&error];

    if (error) {
      FIRCLSErrorLog(@"Failed to read JSON from file (%@) line (%d) with error: %@", filePath,
                     lineNum, error);
    } else {
      [array addObject:dict];
    }
  }

  return array;
}

//
// MARK: GDTCOREventDataObject
//

- (NSData *)transportBytes {
  pb_ostream_t sizestream = PB_OSTREAM_SIZING;

  // Encode 1 time to determine the size.
  if (!pb_encode(&sizestream, google_crashlytics_Report_fields, &_report)) {
    FIRCLSErrorLog(@"Error in nanopb encoding for size: %s", PB_GET_ERROR(&sizestream));
  }

  // Encode a 2nd time to actually get the bytes from it.
  size_t bufferSize = sizestream.bytes_written;
  CFMutableDataRef dataRef = CFDataCreateMutable(CFAllocatorGetDefault(), bufferSize);
  CFDataSetLength(dataRef, bufferSize);
  pb_ostream_t ostream = pb_ostream_from_buffer((void *)CFDataGetBytePtr(dataRef), bufferSize);
  if (!pb_encode(&ostream, google_crashlytics_Report_fields, &_report)) {
    FIRCLSErrorLog(@"Error in nanopb encoding for bytes: %s", PB_GET_ERROR(&ostream));
  }

  return CFBridgingRelease(dataRef);
}

//
// MARK: NanoPB conversions
//

- (google_crashlytics_Report)protoReport {
  google_crashlytics_Report report = google_crashlytics_Report_init_default;
  report.sdk_version = FIRCLSEncodeString(self.identity.build_version);
  report.gmp_app_id = FIRCLSEncodeString(self.googleAppID);
  report.platform = [self protoPlatformFromString:self.host.platform];
  report.installation_uuid = FIRCLSEncodeString(self.identity.install_id);
  report.build_version = FIRCLSEncodeString(self.application.build_version);
  report.display_version = FIRCLSEncodeString(self.application.display_version);
  report.apple_payload = [self protoFilesPayload];
  return report;
}

- (google_crashlytics_FilesPayload)protoFilesPayload {
  google_crashlytics_FilesPayload apple_payload = google_crashlytics_FilesPayload_init_default;

  NSArray<NSString *> *clsRecords = [self clsRecordFilePaths];
  google_crashlytics_FilesPayload_File *files =
      malloc(sizeof(google_crashlytics_FilesPayload_File) * clsRecords.count);

  if (files == NULL) {
    // files and files_count are initialized to NULL and 0 by default.
    return apple_payload;
  }
  for (NSUInteger i = 0; i < clsRecords.count; i++) {
    google_crashlytics_FilesPayload_File file = google_crashlytics_FilesPayload_File_init_default;
    file.filename = FIRCLSEncodeString(clsRecords[i].lastPathComponent);

    NSError *error;
    file.contents = FIRCLSEncodeData([NSData dataWithContentsOfFile:clsRecords[i]
                                                            options:0
                                                              error:&error]);
    if (error) {
      FIRCLSErrorLog(@"Failed to read from %@ with error: %@", clsRecords[i], error);
    }

    files[i] = file;
  }

  apple_payload.files = files;
  apple_payload.files_count = (pb_size_t)clsRecords.count;

  return apple_payload;
}

- (NSArray<NSString *> *)clsRecordFilePaths {
  NSMutableArray<NSString *> *clsRecords = [[NSMutableArray<NSString *> alloc] init];

  NSError *error;
  NSArray *files = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:self.folderPath
                                                                       error:&error];

  if (error) {
    FIRCLSErrorLog(@"Failed to find .clsrecords from %@ with error: %@", self.folderPath, error);
    return clsRecords;
  }

  [files enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
    NSString *filename = (NSString *)obj;
    NSString *lowerExtension = filename.pathExtension.lowercaseString;
    if ([lowerExtension isEqualToString:@"clsrecord"] ||
        [lowerExtension isEqualToString:@"symbolicated"]) {
      [clsRecords addObject:[self.folderPath stringByAppendingPathComponent:filename]];
    }
  }];

  return [clsRecords sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)];
}

- (google_crashlytics_Platforms)protoPlatformFromString:(NSString *)str {
  NSString *platform = str.lowercaseString;

  if ([platform isEqualToString:@"ios"]) {
    return google_crashlytics_Platforms_IOS;
  } else if ([platform isEqualToString:@"mac"]) {
    return google_crashlytics_Platforms_MAC_OS_X;
  } else if ([platform isEqualToString:@"tvos"]) {
    return google_crashlytics_Platforms_TVOS;
  } else {
    return google_crashlytics_Platforms_UNKNOWN_PLATFORM;
  }
}

/** Mallocs a pb_bytes_array and copies the given NSString's bytes into the bytes array.
 * @note Memory needs to be freed manually, through pb_free or pb_release.
 * @param string The string to encode as pb_bytes.
 */
pb_bytes_array_t *FIRCLSEncodeString(NSString *string) {
  if ([string isMemberOfClass:[NSNull class]]) {
    FIRCLSErrorLog(@"Expected encodable string, but found NSNull instead. "
                   @"Set a symbolic breakpoint at FIRCLSEncodeString to debug.");
    string = nil;
  }
  NSString *stringToEncode = string ? string : @"";
  NSData *stringBytes = [stringToEncode dataUsingEncoding:NSUTF8StringEncoding];
  return FIRCLSEncodeData(stringBytes);
}

/** Mallocs a pb_bytes_array and copies the given NSData bytes into the bytes array.
 * @note Memory needs to be free manually, through pb_free or pb_release.
 * @param data The data to copy into the new bytes array.
 */
pb_bytes_array_t *FIRCLSEncodeData(NSData *data) {
  pb_bytes_array_t *pbBytes = malloc(PB_BYTES_ARRAY_T_ALLOCSIZE(data.length));
  if (pbBytes == NULL) {
    return NULL;
  }
  memcpy(pbBytes->bytes, [data bytes], data.length);
  pbBytes->size = (pb_size_t)data.length;
  return pbBytes;
}

@end
