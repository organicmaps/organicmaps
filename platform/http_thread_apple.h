#pragma once

#import <Foundation/Foundation.h>

#include "../std/string.hpp"

namespace downloader { class IHttpThreadCallback; }

@interface HttpThread : NSObject
{
  downloader::IHttpThreadCallback * m_callback;
  NSURLConnection * m_connection;
  int64_t m_begRange, m_endRange;
  int64_t m_downloadedBytes;
  int64_t m_expectedSize;
}

- (id) initWith:(string const &)url callback:(downloader::IHttpThreadCallback &)cb begRange:(int64_t)beg
       endRange:(int64_t)end expectedSize:(int64_t)size postBody:(string const &)pb;

- (void) cancel;
@end
