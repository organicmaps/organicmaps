#import "platform/background_downloader_ios.h"

#include "base/logging.hpp"
#include "platform/downloader_utils.hpp"

// How many seconds to wait before the request fails.
static constexpr NSTimeInterval kTimeoutIntervalInSeconds = 10;

@interface TaskInfo : NSObject

@property(nonatomic, strong) NSURLSessionTask * task;
@property(nonatomic, copy) DownloadCompleteBlock completion;
@property(nonatomic, copy) DownloadProgressBlock progress;

- (instancetype)initWithTask:(NSURLSessionTask *)task
                  completion:(DownloadCompleteBlock)completion
                    progress:(DownloadProgressBlock)progress;

@end

@implementation TaskInfo

- (instancetype)initWithTask:(NSURLSessionTask *)task
                  completion:(DownloadCompleteBlock)completion
                    progress:(DownloadProgressBlock)progress
{
  self = [super init];
  if (self)
  {
    _task = task;
    _completion = completion;
    _progress = progress;
  }

  return self;
}

@end

@interface MapFileSaveStrategy : NSObject

- (nullable NSURL *)getLocationForTask:(NSURLSessionTask *)task;

@end

@implementation MapFileSaveStrategy

- (nullable NSURL *)getLocationForTask:(NSURLSessionTask *)task
{
  NSString * path = task.taskDescription;
  if (path.length == 0)
  {
    // A background task restored after an app relaunch can reach here before it is re-attached
    // (which sets taskDescription), so derive the save path from the URL. That only works for the
    // official CDN layout (maps/<v>/<f>.mwm); for any other layout (e.g. a custom ?map-download-server
    // with a base path) bail out instead of letting GetFilePathByUrl abort on a CHECK.
    std::string const urlPath = task.currentRequest.URL.path.UTF8String;
    if (!downloader::IsUrlSupported(urlPath))
      return nil;
    path = @(downloader::GetFilePathByUrl(urlPath).c_str());
  }
  return [NSURL fileURLWithPath:path];
}

@end

@interface BackgroundDownloader () <NSURLSessionDownloadDelegate>

@property(nonatomic, strong) NSURLSession * session;
@property(nonatomic, strong) NSMutableDictionary * tasks;
@property(nonatomic, strong) NSMutableDictionary * restoredTasks;
/// Maps a full download URL string => resume NSData. Keyed by the full URL, not just the path,
/// because resume data is bound to the server that produced it: -downloadTaskWithResumeData:
/// rebuilds the request from the blob and re-issues it to that server, so reusing it for a
/// different mirror (CDN fallback or ?map-download-server) would ignore the newly selected server.
@property(nonatomic, strong) NSMutableDictionary * resumeData;
@property(nonatomic, strong) MapFileSaveStrategy * saveStrategy;

@end

@implementation BackgroundDownloader

+ (BackgroundDownloader *)sharedBackgroundMapDownloader
{
  static dispatch_once_t dispatchOnce;
  static BackgroundDownloader * backgroundDownloader;
  dispatch_once(&dispatchOnce, ^{
    MapFileSaveStrategy * mapFileSaveStrategy = [[MapFileSaveStrategy alloc] init];
    backgroundDownloader = [[BackgroundDownloader alloc] initWithSessionName:@"background_map_downloader"
                                                                saveStrategy:mapFileSaveStrategy];
  });

  return backgroundDownloader;
}

- (instancetype)initWithSessionName:(NSString *)name saveStrategy:(MapFileSaveStrategy *)saveStrategy
{
  self = [super init];
  if (self)
  {
    NSURLSessionConfiguration * configuration =
        [NSURLSessionConfiguration backgroundSessionConfigurationWithIdentifier:name];
    configuration.sessionSendsLaunchEvents = YES;
    configuration.timeoutIntervalForRequest = kTimeoutIntervalInSeconds;
    _session = [NSURLSession sessionWithConfiguration:configuration delegate:self delegateQueue:nil];
    _tasks = [NSMutableDictionary dictionary];
    _saveStrategy = saveStrategy;
    _restoredTasks = [NSMutableDictionary dictionary];
    _resumeData = [NSMutableDictionary dictionary];

    [_session getAllTasksWithCompletionHandler:^(NSArray<__kindof NSURLSessionTask *> * _Nonnull downloadTasks) {
      dispatch_async(dispatch_get_main_queue(), ^{
        for (NSURLSessionTask * downloadTask in downloadTasks)
        {
          TaskInfo * info = [self.tasks objectForKey:@(downloadTask.taskIdentifier)];
          if (info)
            continue;

          NSString * urlString = downloadTask.currentRequest.URL.path;

          BOOL isTaskReplaced = NO;
          // Replacing task with another one which was added into queue earlier (on previous application session).
          for (id key in self.tasks)
          {
            TaskInfo * info = [self.tasks objectForKey:key];
            if (![info.task.currentRequest.URL.path isEqualToString:urlString])
              continue;

            TaskInfo * newInfo = [[TaskInfo alloc] initWithTask:downloadTask
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

- (NSUInteger)downloadWithUrl:(NSURL *)url
                     filePath:(NSString *)filePath
                   completion:(DownloadCompleteBlock)completion
                     progress:(DownloadProgressBlock)progress
{
  NSUInteger taskIdentifier;
  NSURLSessionTask * restoredTask = [self.restoredTasks objectForKey:url.path];
  if (restoredTask)
  {
    restoredTask.taskDescription = filePath;
    TaskInfo * info = [[TaskInfo alloc] initWithTask:restoredTask completion:completion progress:progress];
    [self.tasks setObject:info forKey:@(restoredTask.taskIdentifier)];
    [self.restoredTasks removeObjectForKey:url.path];
    taskIdentifier = restoredTask.taskIdentifier;
  }
  else
  {
    NSData * resumeData = self.resumeData[url.absoluteString];
    NSURLSessionTask * task =
        resumeData ? [self.session downloadTaskWithResumeData:resumeData] : [self.session downloadTaskWithURL:url];
    task.taskDescription = filePath;
    TaskInfo * info = [[TaskInfo alloc] initWithTask:task completion:completion progress:progress];
    [self.tasks setObject:info forKey:@(task.taskIdentifier)];
    [task resume];
    taskIdentifier = task.taskIdentifier;
  }

  return taskIdentifier;
}

- (void)cancelTaskWithIdentifier:(NSUInteger)taskIdentifier
{
  TaskInfo * info = self.tasks[@(taskIdentifier)];
  if (info)
  {
    [info.task cancel];
    [self.resumeData removeObjectForKey:info.task.currentRequest.URL.absoluteString];
    [self.tasks removeObjectForKey:@(taskIdentifier)];
  }
  else
  {
    for (NSString * key in self.restoredTasks)
    {
      NSURLSessionTask * restoredTask = self.restoredTasks[key];
      if (restoredTask.taskIdentifier == taskIdentifier)
      {
        [restoredTask cancel];
        [self.resumeData removeObjectForKey:restoredTask.currentRequest.URL.absoluteString];
        [self.restoredTasks removeObjectForKey:key];
        break;
      }
    }
  }
}

- (void)clear
{
  // Fast enumeration over an NSDictionary yields its keys, not its values: self.tasks is keyed by
  // task identifier and restoredTasks by URL path. Iterate allValues to cancel the task objects.
  for (TaskInfo * info in self.tasks.allValues)
    [info.task cancel];

  for (NSURLSessionTask * restoredTask in self.restoredTasks.allValues)
    [restoredTask cancel];

  [self.tasks removeAllObjects];
  [self.restoredTasks removeAllObjects];
  [self.resumeData removeAllObjects];
}

#pragma mark - NSURLSessionDownloadDelegate implementation

- (void)finishDownloading:(NSURLSessionTask *)downloadTask error:(nullable NSError *)error
{
  NSString * urlPath = downloadTask.currentRequest.URL.path;
  [self.restoredTasks removeObjectForKey:urlPath];

  // Resume data is keyed by the full URL (see resumeData declaration), unlike restoredTasks above.
  NSString * urlString = downloadTask.currentRequest.URL.absoluteString;
  if (error && error.userInfo && error.userInfo[NSURLSessionDownloadTaskResumeData])
    self.resumeData[urlString] = error.userInfo[NSURLSessionDownloadTaskResumeData];
  else
    [self.resumeData removeObjectForKey:urlString];

  TaskInfo * info = [self.tasks objectForKey:@(downloadTask.taskIdentifier)];
  if (!info)
    return;

  info.completion(error);

  [self.tasks removeObjectForKey:@(downloadTask.taskIdentifier)];
}

- (void)URLSession:(NSURLSession *)session
                 downloadTask:(NSURLSessionDownloadTask *)downloadTask
    didFinishDownloadingToURL:(NSURL *)location
{
  NSError * error;
  // Check for HTTP errors.
  // TODO: Check and prevent redirects.
  NSInteger statusCode = ((NSHTTPURLResponse *)downloadTask.response).statusCode;
  // 206 for resumed downloads.
  if (statusCode != 200 && statusCode != 206)
  {
    LOG(LWARNING,
        ("Failed to download", downloadTask.originalRequest.URL.absoluteString, "HTTP statusCode:", statusCode));
    error = [[NSError alloc] initWithDomain:@"app.omaps.http" code:statusCode userInfo:nil];
    [[NSFileManager defaultManager] removeItemAtURL:location.filePathURL error:nil];
  }
  else
  {
    NSURL * destinationUrl = [self.saveStrategy getLocationForTask:downloadTask];
    if (destinationUrl != nil)
    {
      [[NSFileManager defaultManager] moveItemAtURL:location.filePathURL toURL:destinationUrl error:&error];
    }
    else
    {
      LOG(LWARNING,
          ("Can't resolve a save path for a restored download:", downloadTask.currentRequest.URL.absoluteString));
      error = [[NSError alloc] initWithDomain:@"app.omaps.http" code:500 userInfo:nil];
      [[NSFileManager defaultManager] removeItemAtURL:location.filePathURL error:nil];
    }
  }
  dispatch_async(dispatch_get_main_queue(), ^{ [self finishDownloading:downloadTask error:error]; });
}

- (void)URLSession:(NSURLSession *)session
                 downloadTask:(NSURLSessionDownloadTask *)downloadTask
                 didWriteData:(int64_t)bytesWritten
            totalBytesWritten:(int64_t)totalBytesWritten
    totalBytesExpectedToWrite:(int64_t)totalBytesExpectedToWrite
{
  dispatch_async(dispatch_get_main_queue(), ^{
    TaskInfo * info = [self.tasks objectForKey:@(downloadTask.taskIdentifier)];
    if (!info)
      return;
    info.progress(totalBytesWritten, totalBytesExpectedToWrite);
  });
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)downloadTask didCompleteWithError:(NSError *)error
{
  dispatch_async(dispatch_get_main_queue(), ^{
    if (error && error.code != NSURLErrorCancelled)
      [self finishDownloading:downloadTask error:error];
  });
}

- (void)URLSessionDidFinishEventsForBackgroundURLSession:(NSURLSession *)session
{
  dispatch_async(dispatch_get_main_queue(), ^{
    if (self.backgroundCompletionHandler != nil)
      self.backgroundCompletionHandler();
  });
}

@end
