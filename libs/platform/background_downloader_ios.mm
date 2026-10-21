#import "platform/background_downloader_ios.h"

#include "base/logging.hpp"
#include "platform/downloader_utils.hpp"

// How many seconds to wait before the request fails.
static constexpr NSTimeInterval kTimeoutIntervalInSeconds = 10;

NSErrorDomain const BackgroundDownloaderHttpErrorDomain = @"app.omaps.http";

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
    // official maps/diffs/terrain layout. Custom base paths cannot be resolved here.
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
/// Identifiers of the tasks cancelled in this session, until their final callback: such a task may still
/// deliver its file after a newer attempt has taken over (and maybe already finished) the destination.
@property(nonatomic, strong) NSMutableSet<NSNumber *> * cancelledTasks;

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
    _cancelledTasks = [NSMutableSet set];

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

            downloadTask.taskDescription = info.task.taskDescription;
            TaskInfo * newInfo = [[TaskInfo alloc] initWithTask:downloadTask
                                                     completion:info.completion
                                                       progress:info.progress];
            [self.tasks setObject:newInfo forKey:@(downloadTask.taskIdentifier)];

            [info.task cancel];
            [self.cancelledTasks addObject:@(info.task.taskIdentifier)];
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
    [self.cancelledTasks addObject:@(taskIdentifier)];
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
        [self.cancelledTasks addObject:@(taskIdentifier)];
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
  {
    [info.task cancel];
    [self.cancelledTasks addObject:@(info.task.taskIdentifier)];
  }

  for (NSURLSessionTask * restoredTask in self.restoredTasks.allValues)
  {
    [restoredTask cancel];
    [self.cancelledTasks addObject:@(restoredTask.taskIdentifier)];
  }

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
    error = [[NSError alloc] initWithDomain:BackgroundDownloaderHttpErrorDomain code:statusCode userInfo:nil];
    [[NSFileManager defaultManager] removeItemAtURL:location.filePathURL error:nil];
  }
  else
  {
    // The task ownership changes on the main queue (cancel, restart), and the downloaded file lives only
    // until this method returns: check the ownership and replace the destination there.
    __block NSError * saveError = nil;
    dispatch_sync(dispatch_get_main_queue(), ^{ saveError = [self saveDownloadedFile:location forTask:downloadTask]; });
    error = saveError;
  }
  dispatch_async(dispatch_get_main_queue(), ^{ [self finishDownloading:downloadTask error:error]; });
}

- (nullable NSError *)saveDownloadedFile:(NSURL *)location forTask:(NSURLSessionTask *)downloadTask
{
  NSURL * destinationUrl = [self.saveStrategy getLocationForTask:downloadTask];
  if (destinationUrl == nil)
  {
    LOG(LWARNING,
        ("Can't resolve a save path for a restored download:", downloadTask.currentRequest.URL.absoluteString));
    [[NSFileManager defaultManager] removeItemAtURL:location.filePathURL error:nil];
    return [[NSError alloc] initWithDomain:BackgroundDownloaderHttpErrorDomain code:500 userInfo:nil];
  }

  // A cancelled or replaced task must not replace the file of the attempt that took over its destination,
  // even when that attempt has already finished here and Storage still validates or publishes the file.
  if ([self.cancelledTasks containsObject:@(downloadTask.taskIdentifier)])
  {
    [[NSFileManager defaultManager] removeItemAtURL:location.filePathURL error:nil];
    return nil;
  }
  // A task of the previous app session, not attached yet, still saves: its file is reused when requested.
  if (self.tasks[@(downloadTask.taskIdentifier)] == nil)
  {
    for (TaskInfo * info in self.tasks.allValues)
    {
      if ([[self.saveStrategy getLocationForTask:info.task].path isEqualToString:destinationUrl.path])
      {
        [[NSFileManager defaultManager] removeItemAtURL:location.filePathURL error:nil];
        return nil;
      }
    }
  }

  // The destination may hold a complete file of an interrupted earlier attempt
  // (e.g. a .twm.ready landed while the app was dead): moveItemAtURL refuses to
  // overwrite, which would fail-loop the re-download forever.
  NSError * error = nil;
  [[NSFileManager defaultManager] removeItemAtURL:destinationUrl error:nil];
  [[NSFileManager defaultManager] moveItemAtURL:location.filePathURL toURL:destinationUrl error:&error];
  return error;
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
    // The final callback of a task (after didFinishDownloadingToURL): a cancelled one can't save anymore.
    [self.cancelledTasks removeObject:@(downloadTask.taskIdentifier)];
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
