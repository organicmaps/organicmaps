#pragma once

#import <Foundation/Foundation.h>

#include "std/target_os.hpp"

#include <string>

namespace downloader { class IHttpThreadCallback; }

#ifdef OMIM_OS_IPHONE
#import "../iphone/Maps/Classes/DownloadIndicatorProtocol.h"
#endif

@interface HttpThreadImpl : NSObject

- (instancetype)initWithURL:(std::string const &)url
                   callback:(downloader::IHttpThreadCallback &)cb
                   begRange:(int64_t)beg
                   endRange:(int64_t)end
               expectedSize:(int64_t)size
                   postBody:(std::string const &)pb;

- (void)cancel;

#ifdef OMIM_OS_IPHONE
+ (void)setDownloadIndicatorProtocol:(id<DownloadIndicatorProtocol>)indicator;
#endif

@end
