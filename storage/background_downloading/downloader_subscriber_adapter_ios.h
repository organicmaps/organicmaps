#pragma once

#import <Foundation/Foundation.h>
#import "platform/background_downloader_ios.h"

#include "storage/map_files_downloader.hpp"

#include <vector>

NS_ASSUME_NONNULL_BEGIN

@interface SubscriberAdapter : NSObject <BackgroundDownloaderSubscriber>

- (instancetype)initWithSubscribers:(std::vector<storage::MapFilesDownloader::Subscriber *> &)subscribers;

- (void)didStartDownloading;
- (void)didFinishDownloading;

@end

NS_ASSUME_NONNULL_END
