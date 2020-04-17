#import "platform/background_downloader_ios.h"

#include "platform/downloader_utils.hpp"

@interface TaskInfo : NSObject

@property(nonatomic, strong) NSURLSessionDownloadTask *task;
@property(nonatomic, copy) DownloadCompleteBlock completion;
@property(nonatomic, copy) DownloadProgressBlock progress;

- (instancetype)initWithTask:(NSURLSessionDownloadTask *)task
                  completion:(DownloadCompleteBlock)completion
                    progress:(DownloadProgressBlock)progress;

@end

@implementation TaskInfo

- (instancetype)initWithTask:(NSURLSessionDownloadTask *)task
                  completion:(DownloadCompleteBlock)completion
                    progress:(DownloadProgressBlock)progress {
  self = [super init];
  if (self) {
    _task = task;
    _completion = completion;
    _progress = progress;
  }

  return self;
}

@end

@interface MapFileSaveStrategy : NSObject

- (NSURL *)getLocationForWebUrl:(NSURL *)webUrl;

@end

@implementation MapFileSaveStrategy

- (NSURL *)getLocationForWebUrl:(NSURL *)webUrl {
  NSString *path = @(downloader::GetFilePathByUrl(webUrl.path.UTF8String).c_str());
  return [NSURL fileURLWithPath:path];
}

@end

@interface BackgroundDownloader () <NSURLSessionDownloadDelegate>

@property(nonatomic, strong) NSURLSession *session;
@property(nonatomic, strong) NSMutableDictionary *tasks;
@property(nonatomic, strong) NSHashTable *subscribers;
@property(nonatomic, strong) MapFileSaveStrategy *saveStrategy;

@end

@implementation BackgroundDownloader

+ (BackgroundDownloader *)sharedBackgroundMapDownloader {
  static dispatch_once_t dispatchOnce;
  static BackgroundDownloader *backgroundDownloader;
  dispatch_once(&dispatchOnce, ^{
    MapFileSaveStrategy *mapFileSaveStrategy = [[MapFileSaveStrategy alloc] init];
    backgroundDownloader = [[BackgroundDownloader alloc] initWithSessionName:@"background_map_downloader"
                                                                saveStrategy:mapFileSaveStrategy];
  });

  return backgroundDownloader;
}

- (instancetype)initWithSessionName:(NSString *)name saveStrategy:(MapFileSaveStrategy *)saveStrategy {
  self = [super init];
  if (self) {
    NSURLSessionConfiguration *configuration =
      [NSURLSessionConfiguration backgroundSessionConfigurationWithIdentifier:name];
    [configuration setSessionSendsLaunchEvents:YES];
    _session = [NSURLSession sessionWithConfiguration:configuration delegate:self delegateQueue:nil];
    _tasks = [NSMutableDictionary dictionary];
    _subscribers = [NSHashTable weakObjectsHashTable];
    _saveStrategy = saveStrategy;
  }

  return self;
}

- (void)subscribe:(id<BackgroundDownloaderSubscriber>)subscriber {
  [self.subscribers addObject:subscriber];
}

- (void)unsubscribe:(id<BackgroundDownloaderSubscriber>)subscriber {
  [self.subscribers removeObject:subscriber];
}

- (NSUInteger)downloadWithUrl:(NSURL *)url
                   completion:(DownloadCompleteBlock)completion
                     progress:(DownloadProgressBlock)progress {
  NSURLSessionDownloadTask *task = [self.session downloadTaskWithURL:url];
  TaskInfo *info = [[TaskInfo alloc] initWithTask:task completion:completion progress:progress];
  [self.tasks setObject:info forKey:@([task taskIdentifier])];
  [task resume];

  if ([self.tasks count] == 1) {
    for (id<BackgroundDownloaderSubscriber> subscriber in self.subscribers)
      [subscriber didStartDownloading];
  }

  return task.taskIdentifier;
}

- (void)cancelTaskWithIdentifier:(NSUInteger)taskIdentifier {
  TaskInfo *info = self.tasks[@(taskIdentifier)];
  [info.task cancel];

  [self.tasks removeObjectForKey:@(taskIdentifier)];

  if ([self.tasks count] == 0) {
    for (id<BackgroundDownloaderSubscriber> subscriber in self.subscribers)
      [subscriber didFinishDownloading];
  }
}

- (void)clear {
  BOOL needNotify = [self.tasks count] > 0;
  for (TaskInfo *info in self.tasks) {
    [info.task cancel];
  }

  [self.tasks removeAllObjects];

  if (needNotify) {
    for (id<BackgroundDownloaderSubscriber> subscriber in self.subscribers)
      [subscriber didFinishDownloading];
  }
}

#pragma mark - NSURLSessionDownloadDelegate implementation

- (void)URLSession:(NSURLSession *)session
               downloadTask:(NSURLSessionDownloadTask *)downloadTask
  didFinishDownloadingToURL:(NSURL *)location {
  NSURL *destinationUrl = [self.saveStrategy getLocationForWebUrl:downloadTask.originalRequest.URL];
  NSError *error;
  [[NSFileManager defaultManager] moveItemAtURL:location.filePathURL toURL:destinationUrl error:&error];

  dispatch_async(dispatch_get_main_queue(), ^{
    TaskInfo *info = [self.tasks objectForKey:@(downloadTask.taskIdentifier)];
    if (!info)
      return;

    info.completion(destinationUrl, error);

    [self.tasks removeObjectForKey:@(downloadTask.taskIdentifier)];

    if ([self.tasks count] == 0) {
      for (id<BackgroundDownloaderSubscriber> subscriber in self.subscribers)
        [subscriber didFinishDownloading];
    }
  });
}

- (void)URLSession:(NSURLSession *)session
               downloadTask:(NSURLSessionDownloadTask *)downloadTask
               didWriteData:(int64_t)bytesWritten
          totalBytesWritten:(int64_t)totalBytesWritten
  totalBytesExpectedToWrite:(int64_t)totalBytesExpectedToWrite {
  dispatch_async(dispatch_get_main_queue(), ^{
    TaskInfo *info = [self.tasks objectForKey:@(downloadTask.taskIdentifier)];
    if (!info)
      return;
    info.progress(totalBytesWritten, totalBytesExpectedToWrite);
  });
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)downloadTask didCompleteWithError:(NSError *)error {
  dispatch_async(dispatch_get_main_queue(), ^{
    TaskInfo *info = [self.tasks objectForKey:@(downloadTask.taskIdentifier)];
    if (!info)
      return;

    info.completion(nil, error);

    [self.tasks removeObjectForKey:@(downloadTask.taskIdentifier)];

    if ([self.tasks count] == 0) {
      for (id<BackgroundDownloaderSubscriber> subscriber in self.subscribers)
        [subscriber didFinishDownloading];
    }
  });
}

- (void)URLSessionDidFinishEventsForBackgroundURLSession:(NSURLSession *)session {
  dispatch_async(dispatch_get_main_queue(), ^{
    if (self.backgroundCompletionHandler != nil)
      self.backgroundCompletionHandler();
  });
}

@end
