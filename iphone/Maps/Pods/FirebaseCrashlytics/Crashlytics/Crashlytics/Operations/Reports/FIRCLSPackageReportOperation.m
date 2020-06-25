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

#import "FIRCLSPackageReportOperation.h"

#include <errno.h>
#include <zlib.h>

#import "FIRCLSFileManager.h"
#import "FIRCLSInternalReport.h"

#import "FIRCLSUtility.h"

#import "FIRCLSByteUtility.h"
#import "FIRCLSMultipartMimeStreamEncoder.h"

#import "FIRCLSSettings.h"

@interface FIRCLSPackageReportOperation ()

@property(nonatomic, copy) NSString *finalPath;

@end

@implementation FIRCLSPackageReportOperation

- (instancetype)initWithReport:(FIRCLSInternalReport *)report
                   fileManager:(FIRCLSFileManager *)fileManager
                      settings:(FIRCLSSettings *)settings {
  self = [super init];
  if (!self) {
    return nil;
  }

  _report = report;
  _fileManager = fileManager;
  _settings = settings;

  return self;
}

- (BOOL)compressData:(NSData *)data toPath:(NSString *)path {
  gzFile file = gzopen([path fileSystemRepresentation], "w");
  if (file == Z_NULL) {
    FIRCLSSDKLogError("Error: unable to open file for compression %s\n", strerror(errno));
    return NO;
  }

  __block BOOL success = [data length] > 0;

  FIRCLSEnumerateByteRangesOfNSDataUsingBlock(
      data, ^(const void *bytes, NSRange byteRange, BOOL *stop) {
        size_t length = byteRange.length;

        if (![self writeBytes:bytes length:length toGZFile:file]) {
          *stop = YES;
          success = NO;
        }
      });

  gzclose(file);

  return success;
}

- (BOOL)writeBytes:(const void *)buffer length:(size_t)length toGZFile:(gzFile)file {
  return FIRCLSFileLoopWithWriteBlock(
      buffer, length, ^ssize_t(const void *partialBuffer, size_t partialLength) {
        errno = 0;
        int ret = gzwrite(file, buffer, (unsigned int)length);

        if (ret == 0) {
          int zerror = 0;
          const char *errorString = gzerror(file, &zerror);

          FIRCLSSDKLogError("Error: failed to write compressed bytes %d, %s, %s \n", zerror,
                            errorString, strerror(errno));
        }

        return ret;
      });
}

- (NSString *)reportPath {
  return [self.report path];
}

- (NSString *)destinationDirectory {
  return [self.fileManager legacyPreparedPath];
}

- (NSString *)packagedPathWithName:(NSString *)name {
  // the output file will use the boundary as the filename, and "multipartmime" as the extension
  return [[self.destinationDirectory stringByAppendingPathComponent:name]
      stringByAppendingPathExtension:@"multipartmime"];
}

- (void)main {
  NSString *reportOrgID = self.settings.orgID;
  if (!reportOrgID) {
    FIRCLSDebugLog(
        @"[Crashlytics:PackageReport] Skipping packaging of report with id '%@' this run of the "
        @"app because Organization ID was nil. Report will upload once settings are download "
        @"successfully",
        self.report.identifier);

    return;
  }

  self.finalPath = nil;

  NSString *boundary = [FIRCLSMultipartMimeStreamEncoder generateBoundary];
  NSString *destPath = [self packagedPathWithName:boundary];

  // try to read the metadata file, which could always fail
  NSString *reportSessionId = self.report.identifier;

  NSOutputStream *stream = [NSOutputStream outputStreamToFileAtPath:destPath append:NO];

  FIRCLSMultipartMimeStreamEncoder *encoder =
      [FIRCLSMultipartMimeStreamEncoder encoderWithStream:stream andBoundary:boundary];
  if (!encoder) {
    return;
  }

  [encoder encode:^{
    [encoder addValue:reportOrgID fieldName:@"org_id"];

    if (reportSessionId) {
      [encoder addValue:reportSessionId fieldName:@"report_id"];
    }

    [self.fileManager
        enumerateFilesInDirectory:self.reportPath
                       usingBlock:^(NSString *filePath, NSString *extension) {
                         if (self.cancelled) {
                           return;
                         }

                         // Do not package or include already gz'ed files. These can get left over
                         // from previously-submitted reports. There's an opportinity here to avoid
                         // compressed certain types of files that cannot be changed.
                         if ([extension isEqualToString:@"gz"]) {
                           return;
                         }

                         NSData *data = [NSData dataWithContentsOfFile:filePath
                                                               options:0
                                                                 error:nil];
                         if ([data length] == 0) {
                           const char *filename = [[filePath lastPathComponent] UTF8String];

                           FIRCLSSDKLogError("Error: unable to read data for compression: %s\n",
                                             filename);
                           return;
                         }

                         [self encode:encoder data:data fromPath:filePath];
                       }];
  }];

  if (self.cancelled) {
    [self.fileManager removeItemAtPath:destPath];
    return;
  }

  self.finalPath = destPath;
}

- (void)encode:(FIRCLSMultipartMimeStreamEncoder *)encoder
          data:(NSData *)data
      fromPath:(NSString *)path {
  // must be non-nil and > 0 length
  if ([path length] == 0) {
    FIRCLSSDKLogError("Error: path is invalid\n");
    return;
  }

  NSString *uploadPath = [path stringByAppendingPathExtension:@"gz"];
  NSString *fieldname = [path lastPathComponent];
  NSString *filename = [uploadPath lastPathComponent];
  NSString *mimeType = @"application/x-gzip";

  // first, attempt to compress
  if (![self compressData:data toPath:uploadPath]) {
    FIRCLSSDKLogError("Error: compression failed for %s\n", [filename UTF8String]);

    // attempt the upload without compression
    mimeType = @"text/plain";
    uploadPath = path;
  }

  [encoder addFile:[NSURL fileURLWithPath:uploadPath]
          fileName:filename
          mimeType:mimeType
         fieldName:fieldname];
}

@end
