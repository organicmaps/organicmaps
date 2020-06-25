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

#import "FIRCLSURLSessionAvailability.h"

#if FIRCLSURLSESSION_REQUIRED
#import "FIRCLSURLSessionDownloadTask.h"

#import "FIRCLSURLSessionDownloadTask_PrivateMethods.h"

#define DELEGATE ((id<FIRCLSURLSessionDownloadDelegate>)[self delegate])

@interface FIRCLSURLSessionDownloadTask () <NSStreamDelegate>
@end

@implementation FIRCLSURLSessionDownloadTask

@synthesize downloadCompletionHandler = _downloadCompletionHandler;

- (id)init {
  self = [super init];
  if (!self) {
    return nil;
  }

#if __has_feature(objc_arc)
  _targetURL = [self temporaryFileURL];
  _outputStream = [NSOutputStream outputStreamWithURL:_targetURL append:NO];
#else
  _targetURL = [[self temporaryFileURL] retain];
  _outputStream = [[NSOutputStream outputStreamWithURL:_targetURL append:NO] retain];
#endif

  [_outputStream scheduleInRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
  [_outputStream setDelegate:self];

  return self;
}

#if !__has_feature(objc_arc)
- (void)dealloc {
  [_downloadCompletionHandler release];
  [_targetURL release];
  [_outputStream release];

  [super dealloc];
}
#else
- (void)dealloc {
  [_outputStream close];
  _outputStream.delegate = nil;
}
#endif

- (NSURL *)temporaryFileURL {
  NSString *tmpPath;

  tmpPath = [NSTemporaryDirectory()
      stringByAppendingPathComponent:[[NSProcessInfo processInfo] globallyUniqueString]];

  // TODO: make this actually unique
  return [NSURL fileURLWithPath:tmpPath isDirectory:NO];
}

- (void)cleanup {
  // now, remove the temporary file
  [[NSFileManager defaultManager] removeItemAtURL:_targetURL error:nil];
}

- (void)complete {
  // This is an override of FIRCLSURLSessionDataTask's cleanup method

  // call completion handler first
  if (_downloadCompletionHandler) {
    _downloadCompletionHandler(_targetURL, [self response], [self error]);
  }

  // followed by the session delegate, if there was no error
  if (![self error]) {
    [DELEGATE downloadTask:self didFinishDownloadingToURL:_targetURL];
  }

  // and then finally, call the session delegate completion
  [DELEGATE task:self didCompleteWithError:[self error]];
}

- (void)writeDataToStream:(NSData *)data {
  // open the stream first
  if ([_outputStream streamStatus] == NSStreamStatusNotOpen) {
    [_outputStream open];
  }

  if ([data respondsToSelector:@selector(enumerateByteRangesUsingBlock:)]) {
    [data enumerateByteRangesUsingBlock:^(const void *bytes, NSRange byteRange, BOOL *stop) {
      [self->_outputStream write:bytes maxLength:byteRange.length];
    }];

    return;
  }

  // fall back to the less-efficient mechanism for older OSes
  [_outputStream write:[data bytes] maxLength:[data length]];
}

#pragma mark NSURLConnectionDelegate
- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data {
  dispatch_async([self queue], ^{
    [self writeDataToStream:data];
  });
}

- (void)completeForError {
  dispatch_async([self queue], ^{
    [self->_outputStream close];
    [self->_connection cancel];
    if (![self error]) {
      [self setError:[NSError errorWithDomain:@"FIRCLSURLSessionDownloadTaskError"
                                         code:-1
                                     userInfo:nil]];
    }
    [self complete];
  });
}

#pragma mark NSStreamDelegate
- (void)stream:(NSStream *)aStream handleEvent:(NSStreamEvent)eventCode {
  switch (eventCode) {
    case NSStreamEventHasSpaceAvailable:
      break;
    case NSStreamEventErrorOccurred:
      [self completeForError];
      break;
    case NSStreamEventEndEncountered:
      break;
    default:
      break;
  }
}

@end

#else

INJECT_STRIP_SYMBOL(clsurlsessiondownloadtask)

#endif
