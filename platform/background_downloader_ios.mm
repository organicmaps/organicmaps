#import "platform/background_downloader_ios.h"

#include "platform/downloader_utils.hpp"

@interface TaskInfo : NSObject

@property(nonatomic, strong) NSURLSessionTask *task;
@property(nonatomic, copy) DownloadCompleteBlock completion;
@property(nonatomic, copy) DownloadProgressBlock progress;

- (instancetype)initWithTask:(NSURLSessionTask *)task
                  completion:(DownloadCompleteBlock)completion
                    progress:(DownloadProgressBlock)progress;

@end

@implementation TaskInfo

- (instancetype)initWithTask:(NSURLSessionTask *)task
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
@property(nonatomic, strong) NSMutableDictionary *restoredTasks;
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
    _restoredTasks = [NSMutableDictionary dictionary];

    [_session getAllTasksWithCompletionHandler:^(NSArray<__kindof NSURLSessionTask *> *_Nonnull downloadTasks) {
      dispatch_async(dispatch_get_main_queue(), ^{
        for (NSURLSessionTask *downloadTask in downloadTasks) {
          TaskInfo *info = [self.tasks objectForKey:@(downloadTask.taskIdentifier)];
          if (info)
            continue;

          NSString *urlString = downloadTask.currentRequest.URL.path;

          BOOL isTaskReplaced = NO;
          /// Replacing task with another one which was added into queue earlier (on previous application session).
          for (TaskInfo *info in self.tasks) {
            if (![info.task.currentRequest.URL.path isEqualToString:urlString])
              continue;

            TaskInfo *newInfo = [[TaskInfo alloc] initWithTask:downloadTask
                                                    completion:info.completion
                                                      progress:info.progress];
            [self.tasks setObject:newInfo forKey:@(downloadTask.taskIdentifier)];

            [info.task cancel];
            [self.tasks removeObjectForKey:@(info.task.taskIdentifier)];
            isTaskReplaced = YES;
            break;
          }

          if (!isTaskReplaced)
            [self.restoredTasks setObject:downloadTask forKey:urlString];
        }
      });
    }];
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
  NSUInteger taskIdentifier;
  NSURLSessionTask *restoredTask = [self.restoredTasks objectForKey:url.path];
  if (restoredTask) {
    TaskInfo *info = [[TaskInfo alloc] initWithTask:restoredTask completion:completion progress:progress];
    [self.tasks setObject:info forKey:@(restoredTask.taskIdentifier)];
    [self.restoredTasks removeObjectForKey:url.path];
    taskIdentifier = restoredTask.taskIdentifier;
  } else {
    NSURLSessionTask *task = [self.session downloadTaskWithURL:url];
    TaskInfo *info = [[TaskInfo alloc] initWithTask:task completion:completion progress:progress];
    [self.tasks setObject:info forKey:@(task.taskIdentifier)];
    [task resume];
    taskIdentifier = task.taskIdentifier;
  }

  if ([self.tasks count] == 1) {
    for (id<BackgroundDownloaderSubscriber> subscriber in self.subscribers)
      [subscriber didStartDownloading];
  }

  return taskIdentifier;
}

- (void)cancelTaskWithIdentifier:(NSUInteger)taskIdentifier {
  BOOL needNotify = [self.tasks count] > 0;
  TaskInfo *info = self.tasks[@(taskIdentifier)];
  if (info) {
    [info.task cancel];
    [self.tasks removeObjectForKey:@(taskIdentifier)];
  } else {
    for (NSString *key in self.restoredTasks) {
      NSURLSessionTask *restoredTask = self.restoredTasks[key];
      if (restoredTask.taskIdentifier == taskIdentifier) {
        [restoredTask cancel];
        [self.restoredTasks removeObjectForKey:key];
        break;
      }
    }
  }

  if (needNotify && [self.tasks count] == 0) {
    for (id<BackgroundDownloaderSubscriber> subscriber in self.subscribers)
      [subscriber didFinishDownloading];
  }
}

- (void)clear {
  BOOL needNotify = [self.tasks count] > 0;
  for (TaskInfo *info in self.tasks) {
    [info.task cancel];
  }

  for (NSURLSessionTask *restoredTask in self.restoredTasks) {
    [restoredTask cancel];
  }

  [self.tasks removeAllObjects];
  [self.restoredTasks removeAllObjects];

  if (needNotify) {
    for (id<BackgroundDownloaderSubscriber> subscriber in self.subscribers)
      [subscriber didFinishDownloading];
  }
}

#pragma mark - NSURLSessionDownloadDelegate implementation

- (void)finishDownloading:(NSURLSessionTask *)downloadTask error:(nullable NSError *)error {
  [self.restoredTasks removeObjectForKey:downloadTask.currentRequest.URL.path];

  TaskInfo *info = [self.tasks objectForKey:@(downloadTask.taskIdentifier)];
  if (!info)
    return;

  info.completion(error);

  [self.tasks removeObjectForKey:@(downloadTask.taskIdentifier)];

  if ([self.tasks count] == 0) {
    for (id<BackgroundDownloaderSubscriber> subscriber in self.subscribers)
      [subscriber didFinishDownloading];
  }
}

- (void)URLSession:(NSURLSession *)session
               downloadTask:(NSURLSessionDownloadTask *)downloadTask
  didFinishDownloadingToURL:(NSURL *)location {
  NSURL *destinationUrl = [self.saveStrategy getLocationForWebUrl:downloadTask.currentRequest.URL];
  NSError *error;
  [[NSFileManager defaultManager] moveItemAtURL:location.filePathURL toURL:destinationUrl error:&error];

  dispatch_async(dispatch_get_main_queue(), ^{
    [self finishDownloading:downloadTask error:error];
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
    if (error && error.code != NSURLErrorCancelled)
      [self finishDownloading:downloadTask error:error];
  });
}

- (void)URLSessionDidFinishEventsForBackgroundURLSession:(NSURLSession *)session {
  dispatch_async(dispatch_get_main_queue(), ^{
    if (self.backgroundCompletionHandler != nil)
      self.backgroundCompletionHandler();
  });
}

@end
