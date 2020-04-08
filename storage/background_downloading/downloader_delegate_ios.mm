#import "storage/background_downloading/downloader_delegate_ios.h"

@interface DownloaderDelegate ()

@property(copy, nonatomic) NSString *downloadPath;
@property(copy, nonatomic) OnFinishDownloadingBlock onFinish;
@property(copy, nonatomic) OnProgressBlock onProgress;

@end

@implementation DownloaderDelegate

- (instancetype)initWithDownloadPath:(NSString *)downloadPath
                 onFinishDownloading:(OnFinishDownloadingBlock)onFinishDownloading
                          onProgress:(OnProgressBlock)onProgress;
{
  self = [super init];

  if (self) {
    _downloadPath = downloadPath;
    _onFinish = onFinishDownloading;
    _onProgress = onProgress;
  }

  return self;
}

- (void)didFinishDownloadingToURL:(NSURL *)location {
  NSURL *destinationUrl = [NSURL fileURLWithPath:self.downloadPath];
  BOOL result = YES;
  if (![location isEqual:destinationUrl]) {
    result = [[NSFileManager defaultManager] moveItemAtURL:location.filePathURL toURL:destinationUrl error:nil];
  }

  downloader::DownloadStatus status =
    result ? downloader::DownloadStatus::Completed : downloader::DownloadStatus::Failed;
  self.onFinish(status);
}

- (void)didCompleteWithError:(NSError *)error {
  if (!error || error.code == NSURLErrorCancelled)
    return;

  downloader::DownloadStatus status = error.code == NSURLErrorFileDoesNotExist
                                        ? downloader::DownloadStatus::FileNotFound
                                        : downloader::DownloadStatus::Failed;
  self.onFinish(status);
}

- (void)downloadingProgressWithTotalBytesWritten:(int64_t)totalBytesWritten
                       totalBytesExpectedToWrite:(int64_t)totalBytesExpectedToWrite {
  self.onProgress(totalBytesWritten, totalBytesExpectedToWrite);
}

@end
