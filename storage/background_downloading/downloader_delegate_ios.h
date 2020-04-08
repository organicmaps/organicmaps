#pragma once

#import "platform/background_downloader_ios.h"

#include "storage/background_downloading/downloader_queue_ios.hpp"

#include "platform/downloader_defines.hpp"

typedef void (^OnFinishDownloadingBlock)(downloader::DownloadStatus);
typedef void (^OnProgressBlock)(int64_t bytesWritten, int64_t bytesExpected);

@interface DownloaderDelegate : NSObject <BackgroundDownloaderTaskDelegate>

- (instancetype)initWithDownloadPath:(NSString *)downloadPath
                 onFinishDownloading:(OnFinishDownloadingBlock)onFinishDownloading
                          onProgress:(OnProgressBlock)onProgress;

@end
