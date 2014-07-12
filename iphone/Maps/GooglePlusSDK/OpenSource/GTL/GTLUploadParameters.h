/* Copyright (c) 2011 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
//  GTLUploadParameters.h
//

// Uploading documentation:
// https://code.google.com/p/google-api-objectivec-client/wiki/Introduction#Uploading_Files

#import <Foundation/Foundation.h>

#import "GTLDefines.h"

@interface GTLUploadParameters : NSObject <NSCopying> {
 @private
  NSString *MIMEType_;
  NSData *data_;
  NSFileHandle *fileHandle_;
  NSURL *uploadLocationURL_;
  NSString *slug_;
  BOOL shouldSendUploadOnly_;
}

// Uploading requires MIME type and one of
// - data to be uploaded
// - file handle for uploading
@property (copy) NSString *MIMEType;
@property (retain) NSData *data;
@property (retain) NSFileHandle *fileHandle;

// Resuming an in-progress upload is done with the upload location URL,
// and requires a file handle for uploading
@property (retain) NSURL *uploadLocationURL;

// Some services need a slug (filename) header
@property (copy) NSString *slug;

// Uploads may be done without a JSON body in the initial request
@property (assign) BOOL shouldSendUploadOnly;

+ (GTLUploadParameters *)uploadParametersWithData:(NSData *)data
                                         MIMEType:(NSString *)mimeType GTL_NONNULL((1,2));

+ (GTLUploadParameters *)uploadParametersWithFileHandle:(NSFileHandle *)fileHandle
                                               MIMEType:(NSString *)mimeType GTL_NONNULL((1,2));

@end
