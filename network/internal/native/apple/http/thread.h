#pragma once

#include "std/target_os.hpp"

#import <Foundation/Foundation.h>
#include <string>

#include "network/http/thread.hpp"

#ifdef OMIM_OS_IPHONE
#import "../../../../../iphone/Maps/Classes/DownloadIndicatorProtocol.h"
#endif

@interface HttpThreadImpl : NSObject

- (instancetype)initWithURL:(std::string const &)url
                   callback:(om::network::http::IThreadCallback &)cb
                   begRange:(int64_t)beg
                   endRange:(int64_t)end
               expectedSize:(int64_t)size
                   postBody:(std::string const &)pb;

- (void)cancel;

#ifdef OMIM_OS_IPHONE
+ (void)setDownloadIndicatorProtocol:(id<DownloadIndicatorProtocol>)indicator;
#endif

@end
