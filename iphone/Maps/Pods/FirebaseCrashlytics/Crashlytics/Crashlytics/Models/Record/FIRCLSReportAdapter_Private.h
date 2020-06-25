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

#import "FIRCLSRecordApplication.h"
#import "FIRCLSRecordHost.h"
#import "FIRCLSRecordIdentity.h"

pb_bytes_array_t *FIRCLSEncodeString(NSString *string);
pb_bytes_array_t *FIRCLSEncodeData(NSData *data);

@interface FIRCLSReportAdapter ()

@property(nonatomic, readonly) BOOL hasCrashed;

@property(nonatomic, strong) NSString *folderPath;
@property(nonatomic, strong) NSString *googleAppID;

// From metadata.clsrecord
@property(nonatomic, strong) FIRCLSRecordIdentity *identity;
@property(nonatomic, strong) FIRCLSRecordHost *host;
@property(nonatomic, strong) FIRCLSRecordApplication *application;

@property(nonatomic) google_crashlytics_Report report;

- (google_crashlytics_Report)protoReport;
- (NSArray<NSString *> *)clsRecordFilePaths;

@end
