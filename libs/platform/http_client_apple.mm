/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Alexander Borsuk <me@alex.bio> from Minsk, Belarus

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

#if !__has_feature(objc_arc)
#error This file must be compiled with ARC. Either turn on ARC for the project or use -fobjc-arc flag
#endif

#import <Foundation/Foundation.h>

#include "platform/http_client.hpp"
#import "platform/http_session_manager.h"

#include "base/logging.hpp"

#include <optional>

using CancelChecker = platform::HttpClient::CancelChecker;

// Per-request delegate that bridges NSURLSession callbacks to C++ HttpClient handlers.
// Handles redirect control, streaming data, progress, and completion.
@interface HttpClientDelegate : NSObject <NSURLSessionDataDelegate>
{
  platform::HttpClient::CompletionHandler m_handler;
  platform::HttpClient::ProgressHandler m_progressHandler;
  platform::HttpClient::DataHandler m_dataHandler;
  CancelChecker m_cancelChecker;

  platform::HttpClient::Result m_result;
  NSMutableData * m_accumulatedData;
  NSFileHandle * m_outputFileHandle;
  int64_t m_expectedContentLength;
  int64_t m_downloadedBytes;

  BOOL m_followRedirects;
  BOOL m_loadHeaders;
  BOOL m_writeError;
  BOOL m_dataAborted;
  std::string m_urlRequested;
  std::string m_outputFile;

  // Segment-mode state. When set, m_outputFile is unused and bytes are streamed into
  // m_segment->m_path at m_segment->m_offset after 206 + Content-Range validation.
  std::optional<platform::HttpClient::ReceivedFileSegment> m_segment;
  int64_t m_segmentBytesWritten;
  int m_segmentErrorCode;  // kNoError unless a segment-specific failure overrides HTTP code.
}

- (instancetype)initWithHandler:(platform::HttpClient::CompletionHandler)handler
                progressHandler:(platform::HttpClient::ProgressHandler)progressHandler
                    dataHandler:(platform::HttpClient::DataHandler)dataHandler
                  cancelChecker:(CancelChecker)cancelChecker
                followRedirects:(BOOL)followRedirects
                    loadHeaders:(BOOL)loadHeaders
                   urlRequested:(std::string)urlRequested
                     outputFile:(std::string)outputFile
                        segment:(std::optional<platform::HttpClient::ReceivedFileSegment>)segment;

@end

@implementation HttpClientDelegate

- (instancetype)initWithHandler:(platform::HttpClient::CompletionHandler)handler
                progressHandler:(platform::HttpClient::ProgressHandler)progressHandler
                    dataHandler:(platform::HttpClient::DataHandler)dataHandler
                  cancelChecker:(CancelChecker)cancelChecker
                followRedirects:(BOOL)followRedirects
                    loadHeaders:(BOOL)loadHeaders
                   urlRequested:(std::string)urlRequested
                     outputFile:(std::string)outputFile
                        segment:(std::optional<platform::HttpClient::ReceivedFileSegment>)segment
{
  if (self = [super init])
  {
    m_handler = std::move(handler);
    m_progressHandler = std::move(progressHandler);
    m_dataHandler = std::move(dataHandler);
    m_cancelChecker = std::move(cancelChecker);
    m_followRedirects = followRedirects;
    m_loadHeaders = loadHeaders;
    m_urlRequested = std::move(urlRequested);
    m_outputFile = std::move(outputFile);
    m_segment = std::move(segment);
    m_segmentBytesWritten = 0;
    m_segmentErrorCode = platform::HttpClient::kNoError;
    // Accumulator is only used when not streaming to a file or user handler.
    bool const needAccumulator = !m_dataHandler && m_outputFile.empty() && !m_segment;
    m_accumulatedData = needAccumulator ? [[NSMutableData alloc] init] : nil;
    m_outputFileHandle = nil;
    m_writeError = NO;
    m_dataAborted = NO;
    // Segment mode takes precedence over plain output file and defers file open
    // until 206 + Content-Range are validated in didReceiveResponse.
    if (!m_segment && !m_outputFile.empty())
    {
      // Create/truncate the file and open a handle for streaming writes.
      if (![[NSFileManager defaultManager] createFileAtPath:@(m_outputFile.c_str()) contents:nil attributes:nil])
      {
        LOG(LWARNING, ("Can't create output file:", m_outputFile));
        m_writeError = YES;
      }
      else
      {
        m_outputFileHandle = [NSFileHandle fileHandleForWritingAtPath:@(m_outputFile.c_str())];
        if (!m_outputFileHandle)
        {
          LOG(LWARNING, ("Can't open output file for writing:", m_outputFile));
          m_writeError = YES;
        }
      }
    }
    m_expectedContentLength = -1;
    m_downloadedBytes = 0;
  }
  return self;
}

- (void)URLSession:(NSURLSession *)session
                          task:(NSURLSessionTask *)task
    willPerformHTTPRedirection:(NSHTTPURLResponse *)response
                    newRequest:(NSURLRequest *)newRequest
             completionHandler:(void (^)(NSURLRequest *))completionHandler
{
  if (!m_followRedirects && response.statusCode >= 300 && response.statusCode < 400)
  {
    // Cancel the redirect and deliver the redirect response.
    m_result.m_success = true;
    m_result.m_errorCode = static_cast<int>(response.statusCode);
    NSString * location = [response.allHeaderFields objectForKey:@"Location"];
    if (location)
      m_result.m_urlReceived = location.UTF8String;
    else
      m_result.m_urlReceived = response.URL.absoluteString.UTF8String;
    completionHandler(nil);
  }
  else
  {
    completionHandler(newRequest);
  }
}

- (void)URLSession:(NSURLSession *)session
              dataTask:(NSURLSessionDataTask *)dataTask
    didReceiveResponse:(NSURLResponse *)response
     completionHandler:(void (^)(NSURLSessionResponseDisposition))completionHandler
{
  if (m_writeError)
  {
    [dataTask cancel];
    completionHandler(NSURLSessionResponseCancel);
    return;
  }

  NSHTTPURLResponse * httpResponse = (NSHTTPURLResponse *)response;
  if (!httpResponse)
  {
    completionHandler(NSURLSessionResponseCancel);
    return;
  }

  m_result.m_success = true;
  m_result.m_errorCode = static_cast<int>(httpResponse.statusCode);
  m_expectedContentLength = [response expectedContentLength] != NSURLResponseUnknownLength
                              ? static_cast<int64_t>([response expectedContentLength])
                              : -1;

  NSString * redirectUri = [httpResponse.allHeaderFields objectForKey:@"Location"];
  if (redirectUri)
    m_result.m_urlReceived = redirectUri.UTF8String;
  else
    m_result.m_urlReceived = httpResponse.URL.absoluteString.UTF8String;

  if (m_loadHeaders)
  {
    for (NSString * key in httpResponse.allHeaderFields)
    {
      NSString * obj = httpResponse.allHeaderFields[key];
      m_result.m_headers.emplace(key.lowercaseString.UTF8String, obj.UTF8String);
    }
  }
  else
  {
    NSString * setCookies = [httpResponse.allHeaderFields objectForKey:@"Set-Cookie"];
    if (setCookies)
      m_result.m_headers.emplace("set-cookie", platform::HttpClient::NormalizeServerCookies(setCookies.UTF8String));
  }

  // Segment mode: validate 206 Partial Content and Content-Range against the expected
  // segment BEFORE allowing any body bytes. A 200 (range ignored) or mismatched range
  // would corrupt adjacent chunks at different offsets.
  if (m_segment && ![self validateSegmentResponse:httpResponse])
  {
    m_writeError = YES;
    [dataTask cancel];
    completionHandler(NSURLSessionResponseCancel);
    return;
  }

  completionHandler(NSURLSessionResponseAllow);
}

- (BOOL)validateSegmentResponse:(NSHTTPURLResponse *)httpResponse
{
  int const httpCode = static_cast<int>(httpResponse.statusCode);
  NSString * crValue = [httpResponse.allHeaderFields objectForKey:@"Content-Range"];
  auto const validation = platform::HttpClient::ValidateReceivedFileSegmentResponse(
      httpCode, crValue ? std::string_view(crValue.UTF8String) : std::string_view(), *m_segment);
  if (!validation.m_ok)
  {
    m_segmentErrorCode = validation.m_errorCode;
    return NO;
  }

  // Open the existing target file without truncating and seek to the segment offset.
  NSString * path = @(m_segment->m_path.c_str());
  m_outputFileHandle = [NSFileHandle fileHandleForUpdatingAtPath:path];
  if (!m_outputFileHandle)
  {
    LOG(LWARNING, ("Can't open segment file for updating:", m_segment->m_path));
    m_segmentErrorCode = platform::HttpClient::kWriteException;
    return NO;
  }
  NSError * seekError = nil;
  if (![m_outputFileHandle seekToOffset:static_cast<unsigned long long>(m_segment->m_offset) error:&seekError])
  {
    LOG(LWARNING, ("Can't seek segment file", m_segment->m_path, "to offset", m_segment->m_offset, ":",
                   seekError.localizedDescription.UTF8String));
    [m_outputFileHandle closeFile];
    m_outputFileHandle = nil;
    m_segmentErrorCode = platform::HttpClient::kWriteException;
    return NO;
  }
  return YES;
}

- (void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)dataTask didReceiveData:(NSData *)data
{
  if (m_dataAborted)
    return;

  if (m_writeError)
    return;

  if (m_cancelChecker && m_cancelChecker())
    return;

  int64_t const length = static_cast<int64_t>(data.length);
  m_downloadedBytes += length;

  if (m_dataHandler)
  {
    if (!m_dataHandler(data.bytes, static_cast<size_t>(data.length)))
    {
      m_dataAborted = YES;
      [dataTask cancel];
      return;
    }
  }
  else if (m_outputFileHandle)
  {
    // Segment mode: refuse to write past the expected segment length — the server is
    // sending more than advertised in Content-Range.
    if (m_segment)
    {
      int64_t const remaining = m_segment->m_expectedBytes - m_segmentBytesWritten;
      if (length > remaining)
      {
        LOG(LWARNING, ("Segment overflow:", length, "bytes received, only", remaining, "expected"));
        m_writeError = YES;
        m_segmentErrorCode = platform::HttpClient::kInconsistentFileSize;
        [dataTask cancel];
        return;
      }
    }
    NSError * writeError = nil;
    if (![m_outputFileHandle writeData:data error:&writeError])
    {
      LOG(LERROR, ("File write error:", writeError.localizedDescription.UTF8String));
      m_writeError = YES;
      if (m_segment)
        m_segmentErrorCode = platform::HttpClient::kWriteException;
      [dataTask cancel];
      return;
    }
    if (m_segment)
      m_segmentBytesWritten += length;
  }
  else
  {
    [m_accumulatedData appendData:data];
  }

  if (m_progressHandler)
    m_progressHandler(m_downloadedBytes, m_expectedContentLength);
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didCompleteWithError:(NSError *)error
{
  if (m_cancelChecker && m_cancelChecker())
  {
    m_result.m_errorCode = platform::HttpClient::kCancelled;
    m_result.m_success = false;
    if (m_handler)
      m_handler(std::move(m_result));
    return;
  }

  if (error)
  {
    if (error.code == NSURLErrorCancelled)
    {
      // If we already have a result (e.g. redirect was cancelled), deliver it.
      // But not when the cancellation was triggered by a file write error or DataHandler abort.
      if (m_result.m_success && !m_writeError && !m_dataAborted)
      {
        if (m_handler)
          m_handler(std::move(m_result));
        return;
      }
      // Self-inflicted cancels (m_writeError / m_dataAborted) must NOT be mapped to
      // kCancelled — the real error is set later, and preserving the HTTP status here
      // lets segment mode return 404 etc.
      if (!m_writeError && !m_dataAborted)
        m_result.m_errorCode = platform::HttpClient::kCancelled;
    }
    else if (error.code == NSURLErrorUserCancelledAuthentication)
    {
      m_result.m_success = true;
      m_result.m_errorCode = 401;
    }
    else
    {
      m_result.m_errorCode = static_cast<int>(error.code);
      LOG(LDEBUG, ("Error: ", m_result.m_errorCode, ':', error.localizedDescription.UTF8String, "while connecting to",
                   m_urlRequested));
    }
  }

  // Close output file handle if streaming to file.
  if (m_outputFileHandle)
  {
    [m_outputFileHandle closeFile];
    m_outputFileHandle = nil;
  }

  // Segment-mode: verify the server delivered exactly the number of bytes advertised
  // by Content-Range. A short body means the transfer ended before the segment was
  // fully received, which must NOT be treated as a successful chunk.
  if (m_segment && !m_writeError && !error && m_segmentBytesWritten != m_segment->m_expectedBytes &&
      m_segmentErrorCode == platform::HttpClient::kNoError)
  {
    LOG(LWARNING, ("Segment underflow:", m_segmentBytesWritten, "bytes written, expected", m_segment->m_expectedBytes));
    m_writeError = YES;
    m_segmentErrorCode = platform::HttpClient::kInconsistentFileSize;
  }

  // Report file write/open failure.
  if (m_writeError)
  {
    m_result.m_success = false;
    if (m_segmentErrorCode != platform::HttpClient::kNoError)
      m_result.m_errorCode = m_segmentErrorCode;
    else if (!m_segment)
      m_result.m_errorCode = platform::HttpClient::kWriteException;
    // Segment mode with m_segmentErrorCode == kNoError: preserve the HTTP code (e.g. 404),
    // so the downloader's 404 → FileNotFound mapping still works.
  }

  // DataHandler requested abort — override the HTTP success set in didReceiveResponse.
  if (m_dataAborted)
    m_result.m_success = false;

  // Store accumulated data in result (only when not streaming to DataHandler, file, or segment).
  if (!m_dataHandler && m_outputFile.empty() && !m_segment && m_accumulatedData.length > 0)
    m_result.m_serverResponse.assign(reinterpret_cast<char const *>(m_accumulatedData.bytes), m_accumulatedData.length);

  if (m_handler)
    m_handler(std::move(m_result));
}

@end

namespace platform
{
HttpClient::RequestHandle HttpClient::RunHttpRequestAsync(CompletionHandler handler)
{
  RequestHandle handle;
  handle.m_impl = std::make_shared<RequestHandle::Impl>();

  // Copy all config we need into local variables for the block/delegate capture.
  std::string const urlRequested = m_urlRequested;
  std::string const httpMethod = m_httpMethod;
  std::string const bodyData = m_bodyData;
  std::string const inputFile = m_inputFile;
  std::string const outputFile = m_outputFile;
  auto const segment = m_receivedFileSegment;
  std::string const cookies = m_cookies;
  Headers const headers = m_headers;
  bool const followRedirects = m_followRedirects;
  bool const loadHeaders = m_loadHeaders;
  double const timeoutSec = m_timeoutSec;

  NSURL * url = [NSURL URLWithString:@(urlRequested.c_str())];
  NSMutableURLRequest * request = [NSMutableURLRequest requestWithURL:url
                                                          cachePolicy:NSURLRequestReloadIgnoringLocalCacheData
                                                      timeoutInterval:timeoutSec];
  // We handle cookies manually.
  request.HTTPShouldHandleCookies = NO;

  request.HTTPMethod = @(httpMethod.c_str());
  for (auto const & header : headers)
    [request setValue:@(header.second.c_str()) forHTTPHeaderField:@(header.first.c_str())];

  if (!cookies.empty())
    [request setValue:@(cookies.c_str()) forHTTPHeaderField:@"Cookie"];

  if (!bodyData.empty())
  {
    request.HTTPBody = [NSData dataWithBytes:bodyData.data() length:bodyData.size()];
    LOG(LDEBUG, ("Uploading buffer of size", bodyData.size(), "bytes"));
  }
  else if (!inputFile.empty())
  {
    NSError * err = nil;
    NSString * path = @(inputFile.c_str());
    unsigned long long const file_size =
        [[NSFileManager defaultManager] attributesOfItemAtPath:path error:&err].fileSize;
    if (err)
    {
      Result result;
      result.m_errorCode = static_cast<int>(err.code);
      LOG(LDEBUG, ("Error: ", result.m_errorCode, err.localizedDescription.UTF8String));
      if (handler)
        handler(std::move(result));
      return handle;
    }
    request.HTTPBodyStream = [NSInputStream inputStreamWithFileAtPath:path];
    [request setValue:[NSString stringWithFormat:@"%llu", file_size] forHTTPHeaderField:@"Content-Length"];
    LOG(LDEBUG, ("Uploading file", inputFile, file_size, "bytes"));
  }

  // Use delegate-based task when streaming is needed: DataHandler, ProgressHandler, file
  // output, or segment output. The delegate's didReceiveData: writes chunks to disk
  // incrementally, avoiding buffering the entire response body in memory.
  bool const useDelegate = m_progressHandler || m_dataHandler || !outputFile.empty() || segment.has_value();

  NSURLSessionDataTask * task = nil;

  if (useDelegate)
  {
    // Delegate-based task for streaming DataHandler and ProgressHandler support.
    // HttpSessionManager serializes callbacks for each request on its own background queue.
    HttpClientDelegate * delegate = [[HttpClientDelegate alloc] initWithHandler:std::move(handler)
                                                                progressHandler:m_progressHandler
                                                                    dataHandler:m_dataHandler
                                                                  cancelChecker:handle.MakeCancelChecker()
                                                                followRedirects:followRedirects ? YES : NO
                                                                    loadHeaders:loadHeaders ? YES : NO
                                                                   urlRequested:urlRequested
                                                                     outputFile:outputFile
                                                                        segment:segment];

    task = [[HttpSessionManager sharedManager] dataTaskWithRequest:request delegate:delegate completionHandler:nil];
  }
  else
  {
    // Completion-handler tasks still use HttpSessionManager for redirect decisions.
    // The final completion callback is dispatched on the same per-request callback queue.
    auto cancelChecker = handle.MakeCancelChecker();

    task = [[HttpSessionManager sharedManager]
        dataTaskWithRequest:request
                   delegate:[[HttpClientDelegate alloc] initWithHandler:nullptr
                                                        progressHandler:nullptr
                                                            dataHandler:nullptr
                                                          cancelChecker:CancelChecker()
                                                        followRedirects:followRedirects ? YES : NO
                                                            loadHeaders:NO
                                                           urlRequested:urlRequested
                                                             outputFile:std::string()
                                                                segment:std::nullopt]
          completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
            Result result;

            if (cancelChecker())
            {
              result.m_errorCode = kCancelled;
              if (handler)
                handler(std::move(result));
              return;
            }

            NSHTTPURLResponse * httpResponse = static_cast<NSHTTPURLResponse *>(response);

            if (httpResponse)
            {
              result.m_success = true;
              result.m_errorCode = static_cast<int>(httpResponse.statusCode);

              NSString * redirectUri = [httpResponse.allHeaderFields objectForKey:@"Location"];
              if (redirectUri)
                result.m_urlReceived = redirectUri.UTF8String;
              else
                result.m_urlReceived = httpResponse.URL.absoluteString.UTF8String;

              if (loadHeaders)
              {
                for (NSString * key in httpResponse.allHeaderFields)
                {
                  NSString * obj = httpResponse.allHeaderFields[key];
                  result.m_headers.emplace(key.lowercaseString.UTF8String, obj.UTF8String);
                }
              }
              else
              {
                NSString * setCookies = [httpResponse.allHeaderFields objectForKey:@"Set-Cookie"];
                if (setCookies)
                  result.m_headers.emplace("set-cookie", NormalizeServerCookies(setCookies.UTF8String));
              }

              if (data)
                result.m_serverResponse.assign(reinterpret_cast<char const *>(data.bytes), data.length);
            }
            else if (error)
            {
              if (error.code == NSURLErrorUserCancelledAuthentication)
              {
                result.m_success = true;
                result.m_errorCode = 401;
              }
              else
              {
                result.m_errorCode = static_cast<int>(error.code);
                LOG(LDEBUG, ("Error: ", result.m_errorCode, ':', error.localizedDescription.UTF8String,
                             "while connecting to", urlRequested));
              }
            }

            if (handler)
              handler(std::move(result));
          }];
  }

  // Store the task for cancellation.
  {
    std::lock_guard lock(handle.m_impl->m_mu);
    NSURLSessionDataTask * __weak weakTask = task;
    handle.m_impl->m_platformCancel = [weakTask] { [weakTask cancel]; };
  }

  [task resume];

  return handle;
}
}  // namespace platform
