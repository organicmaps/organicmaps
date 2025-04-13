#import "platform/http_thread_apple.h"

#include "platform/http_request.hpp"
#import "platform/http_session_manager.h"
#include "platform/http_thread_callback.hpp"
#include "platform/platform.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

static const NSTimeInterval kTimeoutInterval = 10.0;

@interface HttpThreadImpl ()<NSURLSessionDataDelegate>
{
  downloader::IHttpThreadCallback * m_callback;
  NSURLSessionDataTask * m_dataTask;
  int64_t m_begRange;
  int64_t m_endRange;
  int64_t m_downloadedBytes;
  int64_t m_expectedSize;
  BOOL m_cancelRequested;
}

@end

@implementation HttpThreadImpl

#ifdef OMIM_OS_IPHONE
static id<DownloadIndicatorProtocol> downloadIndicator = nil;

+ (void)setDownloadIndicatorProtocol:(id<DownloadIndicatorProtocol>)indicator
{
  downloadIndicator = indicator;
}
#endif

- (void)dealloc
{
  LOG(LDEBUG, ("ID:", [self hash], "Connection is destroyed"));
  [m_dataTask cancel];
#ifdef OMIM_OS_IPHONE
  [downloadIndicator enableStandby];
  [downloadIndicator disableDownloadIndicator];
#endif
}

- (void)cancel
{
  [m_dataTask cancel];
  m_cancelRequested = true;
}

- (instancetype)initWithURL:(std::string const &)url
                   callback:(downloader::IHttpThreadCallback &)cb
                   begRange:(int64_t)beg
                   endRange:(int64_t)end
               expectedSize:(int64_t)size
                   postBody:(std::string const &)pb
{
  self = [super init];
  
  m_callback = &cb;
  m_begRange = beg;
  m_endRange = end;
  m_downloadedBytes = 0;
  m_expectedSize = size;
  
  NSMutableURLRequest * request =
  [NSMutableURLRequest requestWithURL:[NSURL URLWithString:@(url.c_str())]
                          cachePolicy:NSURLRequestReloadIgnoringLocalCacheData
                      timeoutInterval:kTimeoutInterval];
  
  // use Range header only if we don't download whole file from start
  if (!(beg == 0 && end < 0))
  {
    NSString * val;
    if (end > 0)
    {
      LOG(LDEBUG, (url, "downloading range [", beg, ",", end, "]"));
      val = [[NSString alloc] initWithFormat: @"bytes=%qi-%qi", beg, end];
    }
    else
    {
      LOG(LDEBUG, (url, "resuming download from position", beg));
      val = [[NSString alloc] initWithFormat: @"bytes=%qi-", beg];
    }
    [request addValue:val forHTTPHeaderField:@"Range"];
  }
  
  if (!pb.empty())
  {
    NSData * postData = [NSData dataWithBytes:pb.data() length:pb.size()];
    [request setHTTPBody:postData];
    [request setHTTPMethod:@"POST"];
    [request addValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
  }
  
#ifdef OMIM_OS_IPHONE
  [downloadIndicator disableStandby];
  [downloadIndicator enableDownloadIndicator];
#endif
  
  // create the task with the request and start loading the data
  m_dataTask = [[HttpSessionManager sharedManager] dataTaskWithRequest:request
                                                              delegate:self
                                                     completionHandler:nil];
  
  if (m_dataTask)
  {
    [m_dataTask resume];
    LOG(LDEBUG, ("ID:", [self hash], "Starting data task for", url));
  }
  else
  {
    LOG(LERROR, ("Can't create data task for", url));
    return nil;
  }
  
  return self;
}

/// We cancel and don't support any redirects to avoid data corruption
/// @TODO Display content to user - router is redirecting us somewhere
- (void)URLSession:(NSURLSession *)session
              task:(NSURLSessionTask *)task
willPerformHTTPRedirection:(NSHTTPURLResponse *)response
        newRequest:(NSURLRequest *)request
 completionHandler:(void (^)(NSURLRequest *))completionHandler
{
  LOG(LWARNING, ("Canceling because of redirect from", response.URL.absoluteString.UTF8String, "to",
                 request.URL.absoluteString.UTF8String));
  completionHandler(nil);
  m_callback->OnFinish(static_cast<NSHTTPURLResponse *>(response).statusCode, m_begRange,
                       m_endRange);
}

/// @return -1 if can't decode
+ (int64_t)getContentRange:(NSDictionary *)httpHeader
{
  if (NSString * cr = [httpHeader valueForKey:@"Content-Range"])
  {
    NSArray * arr = [cr componentsSeparatedByString:@"/"];
    if ([arr count])
      return [(NSString *)[arr objectAtIndex:[arr count] - 1] longLongValue];
  }
  
  return -1;
}

- (void)URLSession:(NSURLSession *)session
          dataTask:(NSURLSessionDataTask *)dataTask
didReceiveResponse:(NSURLResponse *)response
 completionHandler:(void (^)(NSURLSessionResponseDisposition disposition))completionHandler
{
  // This method is called when the server has determined that it
  // has enough information to create the NSURLResponse.
  
  // check if this is OK (not a 404 or the like)
  if ([response isKindOfClass:[NSHTTPURLResponse class]])
  {
    NSInteger const statusCode = [(NSHTTPURLResponse *)response statusCode];
    LOG(LDEBUG, ("Got response with status code", statusCode));
    // When we didn't ask for chunks, code should be 200
    // When we asked for a chunk, code should be 206
    bool const isChunk = !(m_begRange == 0 && m_endRange < 0);
    if ((isChunk && statusCode != 206) || (!isChunk && statusCode != 200))
    {
      LOG(LWARNING, ("Received invalid HTTP status code, canceling download", statusCode));
      completionHandler(NSURLSessionResponseCancel);
      m_callback->OnFinish(statusCode, m_begRange, m_endRange);
      return;
    }
    else if (m_expectedSize > 0)
    {
      // get full file expected size from Content-Range header
      int64_t sizeOnServer = [HttpThreadImpl getContentRange:[(NSHTTPURLResponse *)response allHeaderFields]];
      // if it's absent, use Content-Length instead
      if (sizeOnServer < 0)
        sizeOnServer = [response expectedContentLength];
      // We should always check returned size, even if it's invalid (-1)
      if (m_expectedSize != sizeOnServer)
      {
        LOG(LWARNING, ("Canceling download - server replied with invalid size", sizeOnServer,
                       "!=", m_expectedSize));
        completionHandler(NSURLSessionResponseCancel);
        m_callback->OnFinish(downloader::non_http_error_code::kInconsistentFileSize, m_begRange, m_endRange);
        return;
      }
    }
    
    completionHandler(NSURLSessionResponseAllow);
  }
  else
  {
    // In theory, we should never be here.
    ASSERT(false, ("Invalid non-http response, aborting request"));
    completionHandler(NSURLSessionResponseCancel);
    m_callback->OnFinish(downloader::non_http_error_code::kNonHttpResponse, m_begRange, m_endRange);
  }
}

- (void)URLSession:(NSURLSession *)session
          dataTask:(NSURLSessionDataTask *)dataTask
    didReceiveData:(NSData *)data
{
  int64_t const length = [data length];
  m_downloadedBytes += length;
  if(!m_callback->OnWrite(m_begRange + m_downloadedBytes - length, [data bytes], length))
  {
    [m_dataTask cancel];
    m_callback->OnFinish(downloader::non_http_error_code::kWriteException, m_begRange, m_endRange);
  }
}

- (void)URLSession:(NSURLSession *)session
              task:(NSURLSessionTask *)task
didCompleteWithError:(NSError *)error
{
  if (error.code == NSURLErrorCancelled || m_cancelRequested)
    return;
  
  if (error)
    m_callback->OnFinish([error code], m_begRange, m_endRange);
  else
    m_callback->OnFinish(200, m_begRange, m_endRange);
}

@end

class HttpThread
{
public:
  HttpThread(HttpThreadImpl * request)
    : m_request(request)
  {}

  HttpThreadImpl * m_request;
};

///////////////////////////////////////////////////////////////////////////////////////
namespace downloader
{
HttpThread * CreateNativeHttpThread(std::string const & url,
                                    downloader::IHttpThreadCallback & cb,
                                    int64_t beg,
                                    int64_t end,
                                    int64_t size,
                                    std::string const & pb)
{
  HttpThreadImpl * request = [[HttpThreadImpl alloc] initWithURL:url callback:cb begRange:beg endRange:end expectedSize:size postBody:pb];
  return new HttpThread(request);
}

void DeleteNativeHttpThread(HttpThread * request)
{
  [request->m_request cancel];
  delete request;
}
} // namespace downloader
