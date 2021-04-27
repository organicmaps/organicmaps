#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef void (^DownloadCompleteBlock)(NSError *_Nullable error);
typedef void (^DownloadProgressBlock)(int64_t bytesWritten, int64_t bytesExpected);

/// Note: this class is NOT thread-safe and must be used from main thread only.
@interface BackgroundDownloader : NSObject

@property(copy, nonatomic, nullable) void (^backgroundCompletionHandler)(void);

+ (BackgroundDownloader *)sharedBackgroundMapDownloader;

- (NSUInteger)downloadWithUrl:(NSURL *)url
                   completion:(DownloadCompleteBlock)completion
                     progress:(DownloadProgressBlock)progress;
- (void)cancelTaskWithIdentifier:(NSUInteger)taskIdentifier;
- (void)clear;

@end

NS_ASSUME_NONNULL_END
