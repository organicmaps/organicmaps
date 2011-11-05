#import "http_request_impl_apple.h"

#include "http_request_impl_callback.hpp"
#include "platform.hpp"

#include "../base/logging.hpp"

#define TIMEOUT_IN_SECONDS 15.0

@implementation HttpRequestImpl

- (void) dealloc
{
  LOG(LDEBUG, ("ID:", [self hash], "Connection is destroyed"));
  [m_connection cancel];
  [m_connection release];
  [super dealloc];
}

- (void) cancel
{
  [m_connection cancel];
}

- (id) initWith:(string const &)url callback:(downloader::IHttpRequestImplCallback &)cb
       begRange:(int64_t)beg endRange:(int64_t)end postBody:(string const &)pb
{
  self = [super init];

	m_callback = &cb;
	m_begRange = beg;
	m_endRange = end;
	m_downloadedBytes = 0;

	NSMutableURLRequest * request = [NSMutableURLRequest requestWithURL:
			[NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]]
			cachePolicy:NSURLRequestReloadIgnoringLocalCacheData timeoutInterval:TIMEOUT_IN_SECONDS];

	if (beg > 0)
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
		[val release];
	}

	if (!pb.empty())
	{
		NSData * postData = [NSData dataWithBytes:pb.data() length:pb.size()];
		[request setHTTPBody:postData];
		[request setHTTPMethod:@"POST"];
		[request addValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
	}
	// set user-agent with unique client id only for mapswithme requests
	if (url.find("mapswithme.com") != string::npos)
	{
		static string const uid = GetPlatform().UniqueClientId();
		[request addValue:[NSString stringWithUTF8String: uid.c_str()] forHTTPHeaderField:@"User-Agent"];
	}

	// create the connection with the request and start loading the data
	m_connection = [[NSURLConnection alloc] initWithRequest:request delegate:self];

  if (m_connection == 0)
  {
    LOG(LERROR, ("Can't create connection for", url));
    [self release];
    return nil;
  }
  else
    LOG(LDEBUG, ("ID:", [self hash], "Starting connection to", url));

  return self;
}

- (void) connection: (NSURLConnection *)connection didReceiveResponse: (NSURLResponse *)response
{
	// This method is called when the server has determined that it
	// has enough information to create the NSURLResponse.

  // check if this is OK (not a 404 or the like)
  if ([response isKindOfClass:[NSHTTPURLResponse class]])
  {
    NSInteger const statusCode = [(NSHTTPURLResponse *)response statusCode];
    LOG(LDEBUG, ("Got response with status code", statusCode));
#ifdef DEBUG
//    NSDictionary * fields = [(NSHTTPURLResponse *)response allHeaderFields];
//    for (id key in fields)
//      NSLog(@"%@: %@", key, [fields objectForKey:key]);
#endif
    if (statusCode < 200 || statusCode > 299)
    {
      LOG(LWARNING, ("Received HTTP error, canceling download", statusCode));
      [m_connection cancel];
      m_callback->OnFinish(statusCode, m_begRange, m_endRange);
      return;
    }

    int64_t const expectedLength = [response expectedContentLength];
    if (expectedLength < 0)
      LOG(LDEBUG, ("Server doesn't support HTTP Range"));
    else
      LOG(LDEBUG, ("Expected content length", expectedLength));
  }
  else
  { // in theory, we should never be here
    LOG(LWARNING, ("Invalid non-http response, aborting request"));
    [m_connection cancel];
    m_callback->OnFinish(-1, m_begRange, m_endRange);
  }
}

- (void) connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
  int64_t const length = [data length];
  m_downloadedBytes += length;
  m_callback->OnWrite(m_begRange + m_downloadedBytes - length, [data bytes], length);
}

- (void) connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
  LOG(LWARNING, ("Connection failed", [[error localizedDescription] cStringUsingEncoding:NSUTF8StringEncoding]));
  m_callback->OnFinish([error code], m_begRange, m_endRange);
}

- (void) connectionDidFinishLoading:(NSURLConnection *)connection
{
  m_callback->OnFinish(200, m_begRange, m_endRange);
}

@end

///////////////////////////////////////////////////////////////////////////////////////
namespace downloader
{
HttpRequestImpl * CreateNativeHttpRequest(string const & url,
                                          downloader::IHttpRequestImplCallback & cb,
                                          int64_t beg,
                                          int64_t end,
                                          string const & pb)
{
  return [[HttpRequestImpl alloc] initWith:url callback:cb begRange:beg endRange:end postBody:pb];
}

void DeleteNativeHttpRequest(HttpRequestImpl * request)
{
  [request cancel];
  [request release];
}

} // namespace downloader
