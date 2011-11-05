#pragma once

#import <Foundation/Foundation.h>

#include "../std/string.hpp"

namespace downloader { class IHttpRequestImplCallback; }

@interface HttpRequestImpl : NSObject
{
  downloader::IHttpRequestImplCallback * m_callback;
  NSURLConnection * m_connection;
  int64_t m_begRange, m_endRange;
  int64_t m_downloadedBytes;
}

- (id) initWith:(string const &)url callback:(downloader::IHttpRequestImplCallback &)cb
       begRange:(int64_t)beg endRange:(int64_t)end contentType:(string const &)ct
       postBody:(string const &)pb;

- (void) cancel;
@end
