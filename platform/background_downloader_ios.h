#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol BackgroundDownloaderTaskDelegate

- (void)didFinishDownloadingToURL:(NSURL *)location;
- (void)didCompleteWithError:(NSError *)error;
- (void)downloadingProgressWithTotalBytesWritten:(int64_t)totalBytesWritten
                       totalBytesExpectedToWrite:(int64_t)totalBytesExpectedToWrite;

@end

@protocol BackgroundDownloaderSubscriber

- (void)didDownloadingStarted;
- (void)didDownloadingFinished;

@end

/// Note: this class is NOT thread-safe and must be used from main thead only.
@interface BackgroundDownloader : NSObject

@property(copy, nonatomic, nullable) void (^backgroundCompletionHandler)(void);

+ (BackgroundDownloader *)sharedBackgroundMapDownloader;

- (void)subscribe:(id<BackgroundDownloaderSubscriber>)subscriber;

- (NSUInteger)downloadWithUrl:(NSURL *)url delegate:(id<BackgroundDownloaderTaskDelegate>)delegate;
- (void)cancelWithTaskIdentifier:(NSUInteger)taskIdentifier;
- (void)clear;

@end

NS_ASSUME_NONNULL_END
