#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// The domain of the HTTP status errors (the code is the status, e.g. 404) and of a download that can't be saved (500).
extern NSErrorDomain const BackgroundDownloaderHttpErrorDomain;

typedef void (^DownloadCompleteBlock)(NSError * _Nullable error);
typedef void (^DownloadProgressBlock)(int64_t bytesWritten, int64_t bytesExpected);

/// Note: this class is NOT thread-safe and must be used from main thread only.
@interface BackgroundDownloader : NSObject

@property(copy, nonatomic, nullable) void (^backgroundCompletionHandler)(void);

+ (BackgroundDownloader *)sharedBackgroundMapDownloader;

- (NSUInteger)downloadWithUrl:(NSURL *)url
                     filePath:(NSString *)filePath
                   completion:(DownloadCompleteBlock)completion
                     progress:(DownloadProgressBlock)progress;
- (void)cancelTaskWithIdentifier:(NSUInteger)taskIdentifier;
- (void)clear;

@end

NS_ASSUME_NONNULL_END
