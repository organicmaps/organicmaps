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

#if !STRIP_GTM_FETCH_LOGGING

#include <sys/stat.h>
#include <unistd.h>

#import "GTMHTTPFetcherLogging.h"

// Sensitive credential strings are replaced in logs with _snip_
//
// Apps that must see the contents of sensitive tokens can set this to 1
#ifndef SKIP_GTM_FETCH_LOGGING_SNIPPING
#define SKIP_GTM_FETCH_LOGGING_SNIPPING 0
#endif

// If GTMReadMonitorInputStream is available, it can be used for
// capturing uploaded streams of data
//
// We locally declare methods of GTMReadMonitorInputStream so we
// do not need to import the header, as some projects may not have it available
@interface GTMReadMonitorInputStream : NSInputStream
+ (id)inputStreamWithStream:(NSInputStream *)input;
@property (assign) id readDelegate;
@property (assign) SEL readSelector;
@property (retain) NSArray *runLoopModes;
@end

// If GTMNSJSONSerialization is available, it is used for formatting JSON
#if (TARGET_OS_MAC && !TARGET_OS_IPHONE && (MAC_OS_X_VERSION_MAX_ALLOWED < 1070)) || \
  (TARGET_OS_IPHONE && (__IPHONE_OS_VERSION_MAX_ALLOWED < 50000))
@interface GTMNSJSONSerialization : NSObject
+ (NSData *)dataWithJSONObject:(id)obj options:(NSUInteger)opt error:(NSError **)error;
+ (id)JSONObjectWithData:(NSData *)data options:(NSUInteger)opt error:(NSError **)error;
@end
#endif

// Otherwise, if SBJSON is available, it is used for formatting JSON
@interface GTMFetcherSBJSON
- (void)setHumanReadable:(BOOL)flag;
- (NSString*)stringWithObject:(id)value error:(NSError**)error;
- (id)objectWithString:(NSString*)jsonrep error:(NSError**)error;
@end

@interface GTMHTTPFetcher (GTMHTTPFetcherLoggingUtilities)
+ (NSString *)headersStringForDictionary:(NSDictionary *)dict;

- (void)inputStream:(GTMReadMonitorInputStream *)stream
     readIntoBuffer:(void *)buffer
             length:(NSUInteger)length;

// internal file utilities for logging
+ (BOOL)fileOrDirExistsAtPath:(NSString *)path;
+ (BOOL)makeDirectoryUpToPath:(NSString *)path;
+ (BOOL)removeItemAtPath:(NSString *)path;
+ (BOOL)createSymbolicLinkAtPath:(NSString *)newPath
             withDestinationPath:(NSString *)targetPath;

+ (NSString *)snipSubstringOfString:(NSString *)originalStr
                 betweenStartString:(NSString *)startStr
                          endString:(NSString *)endStr;

+ (id)JSONObjectWithData:(NSData *)data;
+ (id)stringWithJSONObject:(id)obj;
@end

@implementation GTMHTTPFetcher (GTMHTTPFetcherLogging)

// fetchers come and fetchers go, but statics are forever
static BOOL gIsLoggingEnabled = NO;
static BOOL gIsLoggingToFile = YES;
static NSString *gLoggingDirectoryPath = nil;
static NSString *gLoggingDateStamp = nil;
static NSString* gLoggingProcessName = nil;

+ (void)setLoggingDirectory:(NSString *)path {
  [gLoggingDirectoryPath autorelease];
  gLoggingDirectoryPath = [path copy];
}

+ (NSString *)loggingDirectory {

  if (!gLoggingDirectoryPath) {
    NSArray *arr = nil;
#if GTM_IPHONE && TARGET_IPHONE_SIMULATOR
    // default to a directory called GTMHTTPDebugLogs into a sandbox-safe
    // directory that a developer can find easily, the application home
    arr = [NSArray arrayWithObject:NSHomeDirectory()];
#elif GTM_IPHONE
    // Neither ~/Desktop nor ~/Home is writable on an actual iPhone device.
    // Put it in ~/Documents.
    arr = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,
                                              NSUserDomainMask, YES);
#else
    // default to a directory called GTMHTTPDebugLogs in the desktop folder
    arr = NSSearchPathForDirectoriesInDomains(NSDesktopDirectory,
                                              NSUserDomainMask, YES);
#endif

    if ([arr count] > 0) {
      NSString *const kGTMLogFolderName = @"GTMHTTPDebugLogs";

      NSString *desktopPath = [arr objectAtIndex:0];
      NSString *logsFolderPath = [desktopPath stringByAppendingPathComponent:kGTMLogFolderName];

      BOOL doesFolderExist = [[self class] fileOrDirExistsAtPath:logsFolderPath];

      if (!doesFolderExist) {
        // make the directory
        doesFolderExist = [self makeDirectoryUpToPath:logsFolderPath];
      }

      if (doesFolderExist) {
        // it's there; store it in the global
        gLoggingDirectoryPath = [logsFolderPath copy];
      }
    }
  }
  return gLoggingDirectoryPath;
}

+ (void)setLoggingEnabled:(BOOL)flag {
  gIsLoggingEnabled = flag;
}

+ (BOOL)isLoggingEnabled {
  return gIsLoggingEnabled;
}

+ (void)setLoggingToFileEnabled:(BOOL)flag {
  gIsLoggingToFile = flag;
}

+ (BOOL)isLoggingToFileEnabled {
  return gIsLoggingToFile;
}

+ (void)setLoggingProcessName:(NSString *)str {
  [gLoggingProcessName release];
  gLoggingProcessName = [str copy];
}

+ (NSString *)loggingProcessName {

  // get the process name (once per run) replacing spaces with underscores
  if (!gLoggingProcessName) {

    NSString *procName = [[NSProcessInfo processInfo] processName];
    NSMutableString *loggingProcessName;
    loggingProcessName = [[NSMutableString alloc] initWithString:procName];

    [loggingProcessName replaceOccurrencesOfString:@" "
                                        withString:@"_"
                                           options:0
                                             range:NSMakeRange(0, [gLoggingProcessName length])];
    gLoggingProcessName = loggingProcessName;
  }
  return gLoggingProcessName;
}

+ (void)setLoggingDateStamp:(NSString *)str {
  [gLoggingDateStamp release];
  gLoggingDateStamp = [str copy];
}

+ (NSString *)loggingDateStamp {
  // we'll pick one date stamp per run, so a run that starts at a later second
  // will get a unique results html file
  if (!gLoggingDateStamp) {
    // produce a string like 08-21_01-41-23PM

    NSDateFormatter *formatter = [[[NSDateFormatter alloc] init] autorelease];
    [formatter setFormatterBehavior:NSDateFormatterBehavior10_4];
    [formatter setDateFormat:@"M-dd_hh-mm-ssa"];

    gLoggingDateStamp = [[formatter stringFromDate:[NSDate date]] retain] ;
  }
  return gLoggingDateStamp;
}

// formattedStringFromData returns a prettyprinted string for XML or JSON input,
// and a plain string for other input data
- (NSString *)formattedStringFromData:(NSData *)inputData
                          contentType:(NSString *)contentType
                                 JSON:(NSDictionary **)outJSON {
  if (inputData == nil) return nil;

  // if the content type is JSON and we have the parsing class available,
  // use that
  if ([contentType hasPrefix:@"application/json"]
      && [inputData length] > 5) {
    // convert from JSON string to NSObjects and back to a formatted string
    NSMutableDictionary *obj = [[self class] JSONObjectWithData:inputData];
    if (obj) {
      if (outJSON) *outJSON = obj;
      if ([obj isKindOfClass:[NSMutableDictionary class]]) {
        // for security and privacy, omit OAuth 2 response access and refresh
        // tokens
        if ([obj valueForKey:@"refresh_token"] != nil) {
          [obj setObject:@"_snip_" forKey:@"refresh_token"];
        }
        if ([obj valueForKey:@"access_token"] != nil) {
          [obj setObject:@"_snip_" forKey:@"access_token"];
        }
      }
      NSString *formatted = [[self class] stringWithJSONObject:obj];
      if (formatted) return formatted;
    }
  }

#if !GTM_FOUNDATION_ONLY && !GTM_SKIP_LOG_XMLFORMAT
  // verify that this data starts with the bytes indicating XML

  NSString *const kXMLLintPath = @"/usr/bin/xmllint";
  static BOOL hasCheckedAvailability = NO;
  static BOOL isXMLLintAvailable;

  if (!hasCheckedAvailability) {
    isXMLLintAvailable = [[self class] fileOrDirExistsAtPath:kXMLLintPath];
    hasCheckedAvailability = YES;
  }

  if (isXMLLintAvailable
      && [inputData length] > 5
      && strncmp([inputData bytes], "<?xml", 5) == 0) {

    // call xmllint to format the data
    NSTask *task = [[[NSTask alloc] init] autorelease];
    [task setLaunchPath:kXMLLintPath];

    // use the dash argument to specify stdin as the source file
    [task setArguments:[NSArray arrayWithObjects:@"--format", @"-", nil]];
    [task setEnvironment:[NSDictionary dictionary]];

    NSPipe *inputPipe = [NSPipe pipe];
    NSPipe *outputPipe = [NSPipe pipe];
    [task setStandardInput:inputPipe];
    [task setStandardOutput:outputPipe];

    [task launch];

    [[inputPipe fileHandleForWriting] writeData:inputData];
    [[inputPipe fileHandleForWriting] closeFile];

    // drain the stdout before waiting for the task to exit
    NSData *formattedData = [[outputPipe fileHandleForReading] readDataToEndOfFile];

    [task waitUntilExit];

    int status = [task terminationStatus];
    if (status == 0 && [formattedData length] > 0) {
      // success
      inputData = formattedData;
    }
  }
#else
  // we can't call external tasks on the iPhone; leave the XML unformatted
#endif

  NSString *dataStr = [[[NSString alloc] initWithData:inputData
                                             encoding:NSUTF8StringEncoding] autorelease];
  return dataStr;
}

- (void)setupStreamLogging {
  // if logging is enabled, it needs a buffer to accumulate data from any
  // NSInputStream used for uploading.  Logging will wrap the input
  // stream with a stream that lets us keep a copy the data being read.
  if ([GTMHTTPFetcher isLoggingEnabled]
      && loggedStreamData_ == nil
      && postStream_ != nil) {
    loggedStreamData_ = [[NSMutableData alloc] init];

    BOOL didCapture = [self logCapturePostStream];
    if (!didCapture) {
      // upload stream logging requires the class
      // GTMReadMonitorInputStream be available
      NSString const *str = @"<<Uploaded stream log unavailable without GTMReadMonitorInputStream>>";
      [loggedStreamData_ setData:[str dataUsingEncoding:NSUTF8StringEncoding]];
    }
  }
}

- (void)setLogRequestBody:(NSString *)bodyString {
  @synchronized(self) {
    [logRequestBody_ release];
    logRequestBody_ = [bodyString copy];
  }
}

- (NSString *)logRequestBody {
  @synchronized(self) {
    return logRequestBody_;
  }
}

- (void)setLogResponseBody:(NSString *)bodyString {
  @synchronized(self) {
    [logResponseBody_ release];
    logResponseBody_ = [bodyString copy];
  }
}

- (NSString *)logResponseBody {
  @synchronized(self) {
    return logResponseBody_;
  }
}

- (void)setShouldDeferResponseBodyLogging:(BOOL)flag {
  @synchronized(self) {
    if (flag != shouldDeferResponseBodyLogging_) {
      shouldDeferResponseBodyLogging_ = flag;
      if (!flag) {
        [self performSelectorOnMainThread:@selector(logFetchWithError:)
                               withObject:nil
                            waitUntilDone:NO];
      }
    }
  }
}

- (BOOL)shouldDeferResponseBodyLogging {
  @synchronized(self) {
    return shouldDeferResponseBodyLogging_;
  }
}

// stringFromStreamData creates a string given the supplied data
//
// If NSString can create a UTF-8 string from the data, then that is returned.
//
// Otherwise, this routine tries to find a MIME boundary at the beginning of
// the data block, and uses that to break up the data into parts. Each part
// will be used to try to make a UTF-8 string.  For parts that fail, a
// replacement string showing the part header and <<n bytes>> is supplied
// in place of the binary data.

- (NSString *)stringFromStreamData:(NSData *)data
                       contentType:(NSString *)contentType {

  if (data == nil) return nil;

  // optimistically, see if the whole data block is UTF-8
  NSString *streamDataStr = [self formattedStringFromData:data
                                              contentType:contentType
                                                     JSON:NULL];
  if (streamDataStr) return streamDataStr;

  // Munge a buffer by replacing non-ASCII bytes with underscores,
  // and turn that munged buffer an NSString.  That gives us a string
  // we can use with NSScanner.
  NSMutableData *mutableData = [NSMutableData dataWithData:data];
  unsigned char *bytes = [mutableData mutableBytes];

  for (unsigned int idx = 0; idx < [mutableData length]; idx++) {
    if (bytes[idx] > 0x7F || bytes[idx] == 0) {
      bytes[idx] = '_';
    }
  }

  NSString *mungedStr = [[[NSString alloc] initWithData:mutableData
                                   encoding:NSUTF8StringEncoding] autorelease];
  if (mungedStr != nil) {

    // scan for the boundary string
    NSString *boundary = nil;
    NSScanner *scanner = [NSScanner scannerWithString:mungedStr];

    if ([scanner scanUpToString:@"\r\n" intoString:&boundary]
        && [boundary hasPrefix:@"--"]) {

      // we found a boundary string; use it to divide the string into parts
      NSArray *mungedParts = [mungedStr componentsSeparatedByString:boundary];

      // look at each of the munged parts in the original string, and try to
      // convert those into UTF-8
      NSMutableArray *origParts = [NSMutableArray array];
      NSUInteger offset = 0;
      for (NSString *mungedPart in mungedParts) {
        NSUInteger partSize = [mungedPart length];

        NSRange range = NSMakeRange(offset, partSize);
        NSData *origPartData = [data subdataWithRange:range];

        NSString *origPartStr = [[[NSString alloc] initWithData:origPartData
                                   encoding:NSUTF8StringEncoding] autorelease];
        if (origPartStr) {
          // we could make this original part into UTF-8; use the string
          [origParts addObject:origPartStr];
        } else {
          // this part can't be made into UTF-8; scan the header, if we can
          NSString *header = nil;
          NSScanner *headerScanner = [NSScanner scannerWithString:mungedPart];
          if (![headerScanner scanUpToString:@"\r\n\r\n" intoString:&header]) {
            // we couldn't find a header
            header = @"";
          }

          // make a part string with the header and <<n bytes>>
          NSString *binStr = [NSString stringWithFormat:@"\r%@\r<<%lu bytes>>\r",
            header, (long)(partSize - [header length])];
          [origParts addObject:binStr];
        }
        offset += partSize + [boundary length];
      }

      // rejoin the original parts
      streamDataStr = [origParts componentsJoinedByString:boundary];
    }
  }

  if (!streamDataStr) {
    // give up; just make a string showing the uploaded bytes
    streamDataStr = [NSString stringWithFormat:@"<<%u bytes>>",
                     (unsigned int)[data length]];
  }
  return streamDataStr;
}

// logFetchWithError is called following a successful or failed fetch attempt
//
// This method does all the work for appending to and creating log files

- (void)logFetchWithError:(NSError *)error {

  if (![[self class] isLoggingEnabled]) return;

  // TODO: (grobbins)  add Javascript to display response data formatted in hex

  NSString *parentDir = [[self class] loggingDirectory];
  NSString *processName = [[self class] loggingProcessName];
  NSString *dateStamp = [[self class] loggingDateStamp];

  // make a directory for this run's logs, like
  //   SyncProto_logs_10-16_01-56-58PM
  NSString *dirName = [NSString stringWithFormat:@"%@_log_%@",
                       processName, dateStamp];
  NSString *logDirectory = [parentDir stringByAppendingPathComponent:dirName];

  if (gIsLoggingToFile) {
    // be sure that the first time this app runs, it's not writing to
    // a preexisting folder
    static BOOL shouldReuseFolder = NO;
    if (!shouldReuseFolder) {
      shouldReuseFolder = YES;
      NSString *origLogDir = logDirectory;
      for (int ctr = 2; ctr < 20; ctr++) {
        if (![[self class] fileOrDirExistsAtPath:logDirectory]) break;

        // append a digit
        logDirectory = [origLogDir stringByAppendingFormat:@"_%d", ctr];
      }
    }
    if (![[self class] makeDirectoryUpToPath:logDirectory]) return;
  }
  // each response's NSData goes into its own xml or txt file, though all
  // responses for this run of the app share a main html file.  This
  // counter tracks all fetch responses for this run of the app.
  //
  // we'll use a local variable since this routine may be reentered while
  // waiting for XML formatting to be completed by an external task
  static int zResponseCounter = 0;
  int responseCounter = ++zResponseCounter;

  // file name for an image data file
  NSString *responseDataFileName = nil;
  NSUInteger responseDataLength;
  if (downloadFileHandle_) {
    responseDataLength = (NSUInteger) [downloadFileHandle_ offsetInFile];
  } else {
    responseDataLength = [downloadedData_ length];
  }

  NSURLResponse *response = [self response];
  NSDictionary *responseHeaders = [self responseHeaders];

  NSString *responseBaseName = nil;
  NSString *responseDataStr = nil;
  NSDictionary *responseJSON = nil;

  // if there's response data, decide what kind of file to put it in based
  // on the first bytes of the file or on the mime type supplied by the server
  NSString *responseMIMEType = [response MIMEType];
  BOOL isResponseImage = NO;
  NSData *dataToWrite = nil;

  if (responseDataLength > 0) {
    NSString *responseDataExtn = nil;

    // generate a response file base name like
    responseBaseName = [NSString stringWithFormat:@"fetch_%d_response",
                        responseCounter];

    NSString *responseType = [responseHeaders valueForKey:@"Content-Type"];
    responseDataStr = [self formattedStringFromData:downloadedData_
                                        contentType:responseType
                                               JSON:&responseJSON];
    if (responseDataStr) {
      // we were able to make a UTF-8 string from the response data
      if ([responseMIMEType isEqual:@"application/atom+xml"]
          || [responseMIMEType hasSuffix:@"/xml"]) {
        responseDataExtn = @"xml";
        dataToWrite = [responseDataStr dataUsingEncoding:NSUTF8StringEncoding];
      }
    } else if ([responseMIMEType isEqual:@"image/jpeg"]) {
      responseDataExtn = @"jpg";
      dataToWrite = downloadedData_;
      isResponseImage = YES;
    } else if ([responseMIMEType isEqual:@"image/gif"]) {
      responseDataExtn = @"gif";
      dataToWrite = downloadedData_;
      isResponseImage = YES;
    } else if ([responseMIMEType isEqual:@"image/png"]) {
      responseDataExtn = @"png";
      dataToWrite = downloadedData_;
      isResponseImage = YES;
    } else {
     // add more non-text types here
    }

    // if we have an extension, save the raw data in a file with that
    // extension
    if (responseDataExtn && dataToWrite) {
      responseDataFileName = [responseBaseName stringByAppendingPathExtension:responseDataExtn];
      NSString *responseDataFilePath = [logDirectory stringByAppendingPathComponent:responseDataFileName];

      NSError *downloadedError = nil;
      if (gIsLoggingToFile
          && ![dataToWrite writeToFile:responseDataFilePath
                               options:0
                                 error:&downloadedError]) {
            NSLog(@"%@ logging write error:%@ (%@)",
                  [self class], downloadedError, responseDataFileName);
          }
    }
  }

  // we'll have one main html file per run of the app
  NSString *htmlName = @"aperçu_http_log.html";
  NSString *htmlPath =[logDirectory stringByAppendingPathComponent:htmlName];

  // if the html file exists (from logging previous fetches) we don't need
  // to re-write the header or the scripts
  BOOL didFileExist = [[self class] fileOrDirExistsAtPath:htmlPath];

  NSMutableString* outputHTML = [NSMutableString string];
  NSURLRequest *request = [self mutableRequest];

  // we need a header to say we'll have UTF-8 text
  if (!didFileExist) {
    [outputHTML appendFormat:@"<html><head><meta http-equiv=\"content-type\" "
      "content=\"text/html; charset=UTF-8\"><title>%@ HTTP fetch log %@</title>",
      processName, dateStamp];
  }

  // now write the visible html elements
  NSString *copyableFileName = [NSString stringWithFormat:@"fetch_%d.txt",
                                responseCounter];

  // write the date & time, the comment, and the link to the plain-text
  // (copyable) log
  NSString *const dateLineFormat = @"<b>%@ &nbsp;&nbsp;&nbsp;&nbsp; ";
  [outputHTML appendFormat:dateLineFormat, [NSDate date]];

  NSString *comment = [self comment];
  if (comment) {
    NSString *const commentFormat = @"%@ &nbsp;&nbsp;&nbsp;&nbsp; ";
    [outputHTML appendFormat:commentFormat, comment];
  }

  NSString *const reqRespFormat = @"</b><a href='%@'><i>request/response log</i></a><br>";
  [outputHTML appendFormat:reqRespFormat, copyableFileName];

  // write the request URL
  NSString *requestMethod = [request HTTPMethod];
  NSURL *requestURL = [request URL];
  [outputHTML appendFormat:@"<b>request:</b> %@ <code>%@</code><br>\n",
   requestMethod, requestURL];

  // write the request headers
  NSDictionary *requestHeaders = [request allHTTPHeaderFields];
  NSUInteger numberOfRequestHeaders = [requestHeaders count];
  if (numberOfRequestHeaders > 0) {
    // Indicate if the request is authorized; warn if the request is
    // authorized but non-SSL
    NSString *auth = [requestHeaders objectForKey:@"Authorization"];
    NSString *headerDetails = @"";
    if (auth) {
      headerDetails = @"&nbsp;&nbsp;&nbsp;<i>authorized</i>";
      BOOL isInsecure = [[requestURL scheme] isEqual:@"http"];
      if (isInsecure) {
        headerDetails = @"&nbsp;&nbsp;&nbsp;<i>authorized, non-SSL</i>"
          "<FONT COLOR='#FF00FF'> &#x26A0;</FONT> "; // 26A0 = ⚠
      }
    }
    NSString *cookiesHdr = [requestHeaders objectForKey:@"Cookie"];
    if (cookiesHdr) {
      headerDetails = [headerDetails stringByAppendingString:
                       @"&nbsp;&nbsp;&nbsp;<i>cookies</i>"];
    }
    NSString *matchHdr = [requestHeaders objectForKey:@"If-Match"];
    if (matchHdr) {
      headerDetails = [headerDetails stringByAppendingString:
                       @"&nbsp;&nbsp;&nbsp;<i>if-match</i>"];
    }
    matchHdr = [requestHeaders objectForKey:@"If-None-Match"];
    if (matchHdr) {
      headerDetails = [headerDetails stringByAppendingString:
                       @"&nbsp;&nbsp;&nbsp;<i>if-none-match</i>"];
    }
    [outputHTML appendFormat:@"&nbsp;&nbsp; headers: %d  %@<br>",
     (int)numberOfRequestHeaders, headerDetails];
  } else {
    [outputHTML appendFormat:@"&nbsp;&nbsp; headers: none<br>"];
  }

  // write the request post data, toggleable
  NSData *postData;
  if (loggedStreamData_) {
    postData = loggedStreamData_;
  } else if (postData_) {
    postData = postData_;
  } else {
    postData = [request_ HTTPBody];
  }

  NSString *postDataStr = nil;
  NSUInteger postDataLength = [postData length];
  NSString *postType = [requestHeaders valueForKey:@"Content-Type"];

  if (postDataLength > 0) {
    [outputHTML appendFormat:@"&nbsp;&nbsp; data: %d bytes, <code>%@</code><br>\n",
     (int)postDataLength, postType ? postType : @"<no type>"];

    if (logRequestBody_) {
      postDataStr = [[logRequestBody_ copy] autorelease];
      [logRequestBody_ release];
      logRequestBody_ = nil;
    } else {
      postDataStr = [self stringFromStreamData:postData
                                   contentType:postType];
      if (postDataStr) {
        // remove OAuth 2 client secret and refresh token
        postDataStr = [[self class] snipSubstringOfString:postDataStr
                                       betweenStartString:@"client_secret="
                                                endString:@"&"];

        postDataStr = [[self class] snipSubstringOfString:postDataStr
                                       betweenStartString:@"refresh_token="
                                                endString:@"&"];

        // remove ClientLogin password
        postDataStr = [[self class] snipSubstringOfString:postDataStr
                                       betweenStartString:@"&Passwd="
                                                endString:@"&"];
      }
    }
  } else {
    // no post data
  }

  // write the response status, MIME type, URL
  NSInteger status = [self statusCode];
  if (response) {
    NSString *statusString = @"";
    if (status != 0) {
      if (status == 200 || status == 201) {
        statusString = [NSString stringWithFormat:@"%ld", (long)status];

        // report any JSON-RPC error
        if ([responseJSON isKindOfClass:[NSDictionary class]]) {
          NSDictionary *jsonError = [responseJSON objectForKey:@"error"];
          if ([jsonError isKindOfClass:[NSDictionary class]]) {
            NSString *jsonCode = [[jsonError valueForKey:@"code"] description];
            NSString *jsonMessage = [jsonError valueForKey:@"message"];
            if (jsonCode || jsonMessage) {
              NSString *const jsonErrFmt = @"&nbsp;&nbsp;&nbsp;<i>JSON error:</i> <FONT"
                @" COLOR='#FF00FF'>%@ %@ &nbsp;&#x2691;</FONT>"; // 2691 = ⚑
              statusString = [statusString stringByAppendingFormat:jsonErrFmt,
                              jsonCode ? jsonCode : @"",
                              jsonMessage ? jsonMessage : @""];
            }
          }
        }
      } else {
        // purple for anything other than 200 or 201
        NSString *flag = (status >= 400 ? @"&nbsp;&#x2691;" : @""); // 2691 = ⚑
        NSString *const statusFormat = @"<FONT COLOR='#FF00FF'>%ld %@</FONT>";
        statusString = [NSString stringWithFormat:statusFormat,
                        (long)status, flag];
      }
    }

    // show the response URL only if it's different from the request URL
    NSString *responseURLStr =  @"";
    NSURL *responseURL = [response URL];

    if (responseURL && ![responseURL isEqual:[request URL]]) {
      NSString *const responseURLFormat = @"<FONT COLOR='#FF00FF'>response URL:"
        "</FONT> <code>%@</code><br>\n";
      responseURLStr = [NSString stringWithFormat:responseURLFormat,
        [responseURL absoluteString]];
    }

    [outputHTML appendFormat:@"<b>response:</b>&nbsp;&nbsp;status %@<br>\n%@",
      statusString, responseURLStr];

    // Write the response headers
    NSUInteger numberOfResponseHeaders = [responseHeaders count];
    if (numberOfResponseHeaders > 0) {
      // Indicate if the server is setting cookies
      NSString *cookiesSet = [responseHeaders valueForKey:@"Set-Cookie"];
      NSString *cookiesStr = (cookiesSet ? @"&nbsp;&nbsp;<FONT COLOR='#990066'>"
                              "<i>sets cookies</i></FONT>" : @"");
      // Indicate if the server is redirecting
      NSString *location = [responseHeaders valueForKey:@"Location"];
      BOOL isRedirect = (status >= 300 && status <= 399 && location != nil);
      NSString *redirectsStr = (isRedirect ? @"&nbsp;&nbsp;<FONT COLOR='#990066'>"
                                "<i>redirects</i></FONT>" : @"");

      [outputHTML appendFormat:@"&nbsp;&nbsp; headers: %d  %@ %@<br>\n",
       (int)numberOfResponseHeaders, cookiesStr, redirectsStr];
    } else {
      [outputHTML appendString:@"&nbsp;&nbsp; headers: none<br>\n"];
    }
  }

  // error
  if (error) {
    [outputHTML appendFormat:@"<b>Error:</b> %@ <br>\n", [error description]];
  }

  // Write the response data
  if (responseDataFileName) {
    NSString *escapedResponseFile = [responseDataFileName stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
    if (isResponseImage) {
      // Make a small inline image that links to the full image file
      [outputHTML appendFormat:@"&nbsp;&nbsp; data: %d bytes, <code>%@</code><br>",
       (int)responseDataLength, responseMIMEType];
      NSString *const fmt = @"<a href=\"%@\"><img src='%@' alt='image'"
        " style='border:solid thin;max-height:32'></a>\n";
      [outputHTML appendFormat:fmt,
       escapedResponseFile, escapedResponseFile];
    } else {
      // The response data was XML; link to the xml file
      NSString *const fmt = @"&nbsp;&nbsp; data: %d bytes, <code>"
        "%@</code>&nbsp;&nbsp;&nbsp;<i><a href=\"%@\">%@</a></i>\n";
      [outputHTML appendFormat:fmt,
       (int)responseDataLength, responseMIMEType,
       escapedResponseFile, [escapedResponseFile pathExtension]];
    }
  } else {
    // The response data was not an image; just show the length and MIME type
    [outputHTML appendFormat:@"&nbsp;&nbsp; data: %d bytes, <code>%@</code>\n",
     (int)responseDataLength, responseMIMEType];
  }

  // Make a single string of the request and response, suitable for copying
  // to the clipboard and pasting into a bug report
  NSMutableString *copyable = [NSMutableString string];
  if (comment) {
    [copyable appendFormat:@"%@\n\n", comment];
  }
  [copyable appendFormat:@"%@\n", [NSDate date]];
  [copyable appendFormat:@"Request: %@ %@\n", requestMethod, requestURL];
  if ([requestHeaders count] > 0) {
    [copyable appendFormat:@"Request headers:\n%@\n",
     [[self class] headersStringForDictionary:requestHeaders]];
  }

  if (postDataLength > 0) {
    [copyable appendFormat:@"Request body: (%u bytes)\n",
     (unsigned int) postDataLength];
    if (postDataStr) {
      [copyable appendFormat:@"%@\n", postDataStr];
    }
    [copyable appendString:@"\n"];
  }

  if (response) {
    [copyable appendFormat:@"Response: status %d\n", (int) status];
    [copyable appendFormat:@"Response headers:\n%@\n",
     [[self class] headersStringForDictionary:responseHeaders]];
    [copyable appendFormat:@"Response body: (%u bytes)\n",
     (unsigned int) responseDataLength];
    if (responseDataLength > 0) {
      if (logResponseBody_) {
        responseDataStr = [[logResponseBody_ copy] autorelease];
        [logResponseBody_ release];
        logResponseBody_ = nil;
      }
      if (responseDataStr != nil) {
        [copyable appendFormat:@"%@\n", responseDataStr];
      } else if (status >= 400 && [temporaryDownloadPath_ length] > 0) {
        // Try to read in the saved data, which is probably a server error
        // message
        NSStringEncoding enc;
        responseDataStr = [NSString stringWithContentsOfFile:temporaryDownloadPath_
                                                usedEncoding:&enc
                                                       error:NULL];
        if ([responseDataStr length] > 0) {
          [copyable appendFormat:@"%@\n", responseDataStr];
        } else {
          [copyable appendFormat:@"<<%u bytes to file>>\n",
           (unsigned int) responseDataLength];
        }
      } else {
        // Even though it's redundant, we'll put in text to indicate that all
        // the bytes are binary
        [copyable appendFormat:@"<<%u bytes>>\n",
         (unsigned int) responseDataLength];
      }
    }
  }

  if (error) {
    [copyable appendFormat:@"Error: %@\n", error];
  }

  // Save to log property before adding the separator
  self.log = copyable;

  [copyable appendString:@"-----------------------------------------------------------\n"];


  // Write the copyable version to another file (linked to at the top of the
  // html file, above)
  //
  // Ideally, something to just copy this to the clipboard like
  //   <span onCopy='window.event.clipboardData.setData(\"Text\",
  //   \"copyable stuff\");return false;'>Copy here.</span>"
  // would work everywhere, but it only works in Safari as of 8/2010
  if (gIsLoggingToFile) {
    NSString *copyablePath = [logDirectory stringByAppendingPathComponent:copyableFileName];
    NSError *copyableError = nil;
    if (![copyable writeToFile:copyablePath
                    atomically:NO
                      encoding:NSUTF8StringEncoding
                         error:&copyableError]) {
      // Error writing to file
      NSLog(@"%@ logging write error:%@ (%@)",
            [self class], copyableError, copyablePath);
    }

    [outputHTML appendString:@"<br><hr><p>"];

    // Append the HTML to the main output file
    const char* htmlBytes = [outputHTML UTF8String];
    NSOutputStream *stream = [NSOutputStream outputStreamToFileAtPath:htmlPath
                                                               append:YES];
    [stream open];
    [stream write:(const uint8_t *) htmlBytes maxLength:strlen(htmlBytes)];
    [stream close];

    // Make a symlink to the latest html
    NSString *symlinkName = [NSString stringWithFormat:@"%@_log_newest.html",
                             processName];
    NSString *symlinkPath = [parentDir stringByAppendingPathComponent:symlinkName];

    [[self class] removeItemAtPath:symlinkPath];
    [[self class] createSymbolicLinkAtPath:symlinkPath
                       withDestinationPath:htmlPath];

#if GTM_IPHONE
    static BOOL gReportedLoggingPath = NO;
    if (!gReportedLoggingPath) {
      gReportedLoggingPath = YES;
      NSLog(@"GTMHTTPFetcher logging to \"%@\"", parentDir);
    }
#endif
  }
}

- (BOOL)logCapturePostStream {
  // This is called when beginning a fetch.  The caller should have already
  // verified that logging is enabled, and should have allocated
  // loggedStreamData_ as a mutable object.

  // If the class GTMReadMonitorInputStream is not available, bail now, since
  // we cannot capture this upload stream
  Class monitorClass = NSClassFromString(@"GTMReadMonitorInputStream");
  if (!monitorClass) return NO;

  // If we're logging, we need to wrap the upload stream with our monitor
  // stream that will call us back with the bytes being read from the stream

  // Our wrapper will retain the old post stream
  [postStream_ autorelease];

  postStream_ = [monitorClass inputStreamWithStream:postStream_];
  [postStream_ retain];

  [(GTMReadMonitorInputStream *)postStream_ setReadDelegate:self];
  [(GTMReadMonitorInputStream *)postStream_ setRunLoopModes:[self runLoopModes]];

  SEL readSel = @selector(inputStream:readIntoBuffer:length:);
  [(GTMReadMonitorInputStream *)postStream_ setReadSelector:readSel];

  return YES;
}

@end

@implementation GTMHTTPFetcher (GTMHTTPFetcherLoggingUtilities)

- (void)inputStream:(GTMReadMonitorInputStream *)stream
     readIntoBuffer:(void *)buffer
             length:(NSUInteger)length {
  // append the captured data
  [loggedStreamData_ appendBytes:buffer length:length];
}

#pragma mark Internal file routines

// We implement plain Unix versions of NSFileManager methods to avoid
// NSFileManager's issues with being used from multiple threads

+ (BOOL)fileOrDirExistsAtPath:(NSString *)path {
  struct stat buffer;
  int result = stat([path fileSystemRepresentation], &buffer);
  return (result == 0);
}

+ (BOOL)makeDirectoryUpToPath:(NSString *)path {
  int result = 0;

  // Recursively create the parent directory of the requested path
  NSString *parent = [path stringByDeletingLastPathComponent];
  if (![self fileOrDirExistsAtPath:parent]) {
    result = [self makeDirectoryUpToPath:parent];
  }

  // Make the leaf directory
  if (result == 0 && ![self fileOrDirExistsAtPath:path]) {
    result = mkdir([path fileSystemRepresentation], S_IRWXU); // RWX for owner
  }
  return (result == 0);
}

+ (BOOL)removeItemAtPath:(NSString *)path {
  int result = unlink([path fileSystemRepresentation]);
  return (result == 0);
}

+ (BOOL)createSymbolicLinkAtPath:(NSString *)newPath
             withDestinationPath:(NSString *)targetPath {
  int result = symlink([targetPath fileSystemRepresentation],
                       [newPath fileSystemRepresentation]);
  return (result == 0);
}

#pragma mark Fomatting Utilities

+ (NSString *)snipSubstringOfString:(NSString *)originalStr
                 betweenStartString:(NSString *)startStr
                          endString:(NSString *)endStr {
#if SKIP_GTM_FETCH_LOGGING_SNIPPING
  return originalStr;
#else
  if (originalStr == nil) return nil;

  // Find the start string, and replace everything between it
  // and the end string (or the end of the original string) with "_snip_"
  NSRange startRange = [originalStr rangeOfString:startStr];
  if (startRange.location == NSNotFound) return originalStr;

  // We found the start string
  NSUInteger originalLength = [originalStr length];
  NSUInteger startOfTarget = NSMaxRange(startRange);
  NSRange targetAndRest = NSMakeRange(startOfTarget,
                                      originalLength - startOfTarget);
  NSRange endRange = [originalStr rangeOfString:endStr
                                        options:0
                                          range:targetAndRest];
  NSRange replaceRange;
  if (endRange.location == NSNotFound) {
    // Found no end marker so replace to end of string
    replaceRange = targetAndRest;
  } else {
    // Replace up to the endStr
    replaceRange = NSMakeRange(startOfTarget,
                               endRange.location - startOfTarget);
  }

  NSString *result = [originalStr stringByReplacingCharactersInRange:replaceRange
                                                          withString:@"_snip_"];
  return result;
#endif // SKIP_GTM_FETCH_LOGGING_SNIPPING
}

+ (NSString *)headersStringForDictionary:(NSDictionary *)dict {
  // Format the dictionary in http header style, like
  //   Accept:        application/json
  //   Cache-Control: no-cache
  //   Content-Type:  application/json; charset=utf-8
  //
  // Pad the key names, but not beyond 16 chars, since long custom header
  // keys just create too much whitespace
  NSArray *keys = [[dict allKeys] sortedArrayUsingSelector:@selector(compare:)];

  NSMutableString *str = [NSMutableString string];
  for (NSString *key in keys) {
    NSString *value = [dict valueForKey:key];
    if ([key isEqual:@"Authorization"]) {
      // Remove OAuth 1 token
      value = [[self class] snipSubstringOfString:value
                               betweenStartString:@"oauth_token=\""
                                        endString:@"\""];

      // Remove OAuth 2 bearer token (draft 16, and older form)
      value = [[self class] snipSubstringOfString:value
                               betweenStartString:@"Bearer "
                                        endString:@"\n"];
      value = [[self class] snipSubstringOfString:value
                               betweenStartString:@"OAuth "
                                        endString:@"\n"];

      // Remove Google ClientLogin
      value = [[self class] snipSubstringOfString:value
                               betweenStartString:@"GoogleLogin auth="
                                        endString:@"\n"];
    }
    [str appendFormat:@"  %@: %@\n", key, value];
  }
  return str;
}

+ (id)JSONObjectWithData:(NSData *)data {
  Class serializer = NSClassFromString(@"NSJSONSerialization");
  if (serializer) {
    const NSUInteger kOpts = (1UL << 0); // NSJSONReadingMutableContainers
    NSMutableDictionary *obj;
    obj = [serializer JSONObjectWithData:data
                                 options:kOpts
                                   error:NULL];
    return obj;
  } else {
    // Try SBJsonParser or SBJSON
    Class jsonParseClass = NSClassFromString(@"SBJsonParser");
    if (!jsonParseClass) {
      jsonParseClass = NSClassFromString(@"SBJSON");
    }
    if (jsonParseClass) {
      GTMFetcherSBJSON *parser = [[[jsonParseClass alloc] init] autorelease];
      NSString *jsonStr = [[[NSString alloc] initWithData:data
                                                 encoding:NSUTF8StringEncoding] autorelease];
      if (jsonStr) {
        NSMutableDictionary *obj = [parser objectWithString:jsonStr error:NULL];
        return obj;
      }
    }
  }
  return nil;
}

+ (id)stringWithJSONObject:(id)obj {
  Class serializer = NSClassFromString(@"NSJSONSerialization");
  if (serializer) {
    const NSUInteger kOpts = (1UL << 0); // NSJSONWritingPrettyPrinted
    NSData *data;
    data = [serializer dataWithJSONObject:obj
                                  options:kOpts
                                    error:NULL];
    if (data) {
      NSString *jsonStr = [[[NSString alloc] initWithData:data
                                                 encoding:NSUTF8StringEncoding] autorelease];
      return jsonStr;
    }
  } else {
    // Try SBJsonParser or SBJSON
    Class jsonWriterClass = NSClassFromString(@"SBJsonWriter");
    if (!jsonWriterClass) {
      jsonWriterClass = NSClassFromString(@"SBJSON");
    }
    if (jsonWriterClass) {
      GTMFetcherSBJSON *writer = [[[jsonWriterClass alloc] init] autorelease];
      [writer setHumanReadable:YES];
      NSString *jsonStr = [writer stringWithObject:obj error:NULL];
      return jsonStr;
    }
  }
  return nil;
}

@end

#endif // !STRIP_GTM_FETCH_LOGGING
