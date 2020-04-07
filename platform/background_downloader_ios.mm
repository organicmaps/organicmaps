#import "platform/background_downloader_ios.h"

#include "platform/downloader_utils.hpp"

@interface TaskInfo : NSObject

@property(nonatomic) NSURLSessionDownloadTask *task;
@property(nonatomic) id<BackgroundDownloaderTaskDelegate> delegate;

- (instancetype)initWithTask:(NSURLSessionDownloadTask *)task delegate:(id<BackgroundDownloaderTaskDelegate>)delegate;

@end

@implementation TaskInfo

- (instancetype)initWithTask:(NSURLSessionDownloadTask *)task delegate:(id<BackgroundDownloaderTaskDelegate>)delegate {
  self = [super init];
  if (self) {
    _task = task;
    _delegate = delegate;
  }

  return self;
}

@end

@protocol FileSaveStrategy

- (NSURL *)getLocationForWebUrl:(NSURL *)webUrl;

@end

@interface MapFileSaveStrategy : NSObject <FileSaveStrategy>

- (NSURL *)getLocationForWebUrl:(NSURL *)webUrl;

@end

@implementation MapFileSaveStrategy

- (NSURL *)getLocationForWebUrl:(NSURL *)webUrl {
  NSString *path = @(downloader::GetFilePathByUrl(webUrl.path.UTF8String).c_str());
  return [NSURL fileURLWithPath:path];
}

@end

@interface BackgroundDownloader () <NSURLSessionDownloadDelegate>

@property(nonatomic) NSURLSession *session;
@property(nonatomic) NSMutableDictionary *tasks;
@property(nonatomic) NSMutableArray *subscribers;
@property(nonatomic) id<FileSaveStrategy> saveStrategy;

@end

@implementation BackgroundDownloader

+ (BackgroundDownloader *)sharedBackgroundMapDownloader {
  static dispatch_once_t dispatchOnce;
  static BackgroundDownloader *backgroundDownloader;
  static MapFileSaveStrategy *mapFileSaveStrategy;
  dispatch_once(&dispatchOnce, ^{
    mapFileSaveStrategy = [[MapFileSaveStrategy alloc] init];
    backgroundDownloader = [[BackgroundDownloader alloc] initWithSessionName:@"background_map_downloader"
                                                                saveStrategy:mapFileSaveStrategy];
  });

  return backgroundDownloader;
}

- (instancetype)initWithSessionName:(NSString *)name saveStrategy:(id<FileSaveStrategy>)saveStrategy {
  self = [super init];
  if (self) {
    NSURLSessionConfiguration *configuration =
      [NSURLSessionConfiguration backgroundSessionConfigurationWithIdentifier:name];
    [configuration setSessionSendsLaunchEvents:YES];
    _session = [NSURLSession sessionWithConfiguration:configuration delegate:self delegateQueue:nil];
    _tasks = [NSMutableDictionary dictionary];
    _subscribers = [NSMutableArray array];
    _saveStrategy = saveStrategy;
  }

  return self;
}

- (void)subscribe:(id<BackgroundDownloaderSubscriber>)subscriber {
  [self.subscribers addObject:subscriber];
}

- (NSUInteger)downloadWithUrl:(NSURL *)url delegate:(id<BackgroundDownloaderTaskDelegate>)delegate {
  NSURLSessionDownloadTask *task = [self.session downloadTaskWithURL:url];
  TaskInfo *info = [[TaskInfo alloc] initWithTask:task delegate:delegate];
  [self.tasks setObject:info forKey:@([task taskIdentifier])];
  [task resume];

  if ([self.tasks count] == 1) {
    for (id<BackgroundDownloaderSubscriber> subscriber in self.subscribers)
      [subscriber didDownloadingStarted];
  }

  return task.taskIdentifier;
}

- (void)cancelWithTaskIdentifier:(NSUInteger)taskIdentifier {
  TaskInfo *info = self.tasks[@(taskIdentifier)];
  [info.task cancelByProducingResumeData:^(NSData *resumeData){
  }];

  [self.tasks removeObjectForKey:@(taskIdentifier)];

  if ([self.tasks count] == 0) {
    for (id<BackgroundDownloaderSubscriber> subscriber in self.subscribers)
      [subscriber didDownloadingFinished];
  }
}

- (void)clear {
  BOOL needNotify = [self.tasks count];
  for (TaskInfo *info in self.tasks) {
    [info.task cancelByProducingResumeData:^(NSData *resumeData){
    }];
  }

  [self.tasks removeAllObjects];

  if (needNotify) {
    for (id<BackgroundDownloaderSubscriber> subscriber in self.subscribers)
      [subscriber didDownloadingFinished];
  }
}

#pragma mark - NSURLSessionDownloadDelegate implementation

- (void)URLSession:(NSURLSession *)session
               downloadTask:(NSURLSessionDownloadTask *)downloadTask
  didFinishDownloadingToURL:(NSURL *)location {
  NSURL *destinationUrl = [self.saveStrategy getLocationForWebUrl:downloadTask.originalRequest.URL];
  NSError *error;
  BOOL result = [[NSFileManager defaultManager] moveItemAtURL:location.filePathURL toURL:destinationUrl error:&error];

  dispatch_async(dispatch_get_main_queue(), ^{
    TaskInfo *info = [self.tasks objectForKey:@(downloadTask.taskIdentifier)];
    if (!info)
      return;

    if (!result)
      [info.delegate didCompleteWithError:error];
    else
      [info.delegate didFinishDownloadingToURL:destinationUrl];

    [self.tasks removeObjectForKey:@(downloadTask.taskIdentifier)];

    if ([self.tasks count] == 0) {
      for (id<BackgroundDownloaderSubscriber> subscriber in self.subscribers)
        [subscriber didDownloadingFinished];
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
    [info.delegate downloadingProgressWithTotalBytesWritten:totalBytesWritten
                                  totalBytesExpectedToWrite:totalBytesExpectedToWrite];
  });
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)downloadTask didCompleteWithError:(NSError *)error {
  dispatch_async(dispatch_get_main_queue(), ^{
    TaskInfo *info = [self.tasks objectForKey:@(downloadTask.taskIdentifier)];
    if (!info)
      return;

    [info.delegate didCompleteWithError:error];

    [self.tasks removeObjectForKey:@(downloadTask.taskIdentifier)];

    if ([self.tasks count] == 0) {
      for (id<BackgroundDownloaderSubscriber> subscriber in self.subscribers)
        [subscriber didDownloadingFinished];
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
