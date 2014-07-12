/* Copyright (c) 2010 Google Inc.
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

#import "GTMHTTPFetcher.h"

// GTM HTTP Logging
//
// All traffic using GTMHTTPFetcher can be easily logged.  Call
//
//   [GTMHTTPFetcher setLoggingEnabled:YES];
//
// to begin generating log files.
//
// Log files are put into a folder on the desktop called "GTMHTTPDebugLogs"
// unless another directory is specified with +setLoggingDirectory.
//
// In the iPhone simulator, the default logs location is the user's home
// directory in ~/Library/Application Support.  On the iPhone device, the
// default logs location is the application's documents directory on the device.
//
// Tip: use the Finder's "Sort By Date" to find the most recent logs.
//
// Each run of an application gets a separate set of log files.  An html
// file is generated to simplify browsing the run's http transactions.
// The html file includes javascript links for inline viewing of uploaded
// and downloaded data.
//
// A symlink is created in the logs folder to simplify finding the html file
// for the latest run of the application; the symlink is called
//
//   AppName_http_log_newest.html
//
// For better viewing of XML logs, use Camino or Firefox rather than Safari.
//
// Each fetcher may be given a comment to be inserted as a label in the logs,
// such as
//   [fetcher setCommentWithFormat:@"retrieve item %@", itemName];
//
// Projects may define STRIP_GTM_FETCH_LOGGING to remove logging code.

#if !STRIP_GTM_FETCH_LOGGING

@interface GTMHTTPFetcher (GTMHTTPFetcherLogging)

// Note: the default logs directory is ~/Desktop/GTMHTTPDebugLogs; it will be
// created as needed.  If a custom directory is set, the directory should
// already exist.
+ (void)setLoggingDirectory:(NSString *)path;
+ (NSString *)loggingDirectory;

// client apps can turn logging on and off
+ (void)setLoggingEnabled:(BOOL)flag;
+ (BOOL)isLoggingEnabled;

// client apps can turn off logging to a file if they want to only check
// the fetcher's log property
+ (void)setLoggingToFileEnabled:(BOOL)flag;
+ (BOOL)isLoggingToFileEnabled;

// client apps can optionally specify process name and date string used in
// log file names
+ (void)setLoggingProcessName:(NSString *)str;
+ (NSString *)loggingProcessName;

+ (void)setLoggingDateStamp:(NSString *)str;
+ (NSString *)loggingDateStamp;

// internal; called by fetcher
- (void)logFetchWithError:(NSError *)error;
- (BOOL)logCapturePostStream;

// Applications may provide alternative body strings to be displayed in the
// log, such as for binary requests or responses.  If deferring is turned
// on, the response log will not be sent until deferring is turned off,
// allowing the application to write the response body after the response
// data has been parsed.
- (void)setLogRequestBody:(NSString *)bodyString;
- (NSString *)logRequestBody;
- (void)setLogResponseBody:(NSString *)bodyString;
- (NSString *)logResponseBody;
- (void)setShouldDeferResponseBodyLogging:(BOOL)flag;
- (BOOL)shouldDeferResponseBodyLogging;

@end

#endif  // !STRIP_GTM_FETCH_LOGGING
