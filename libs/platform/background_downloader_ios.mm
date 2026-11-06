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
/// Identifiers of the tasks cancelled in this session, until their final callback: such a task may still
/// deliver its file after a newer attempt has taken over (and maybe already finished) the destination.
@property(nonatomic, strong) NSMutableSet<NSNumber *> * cancelledTasks;
/// The destination paths requested in this session. A task of the previous app session that isn't attached
/// must not write there, even after the requested attempt has finished and Storage validates its file.
/// Never removed: the downloader doesn't know when Storage is done with the file, and only the tasks missing
/// from |tasks| are checked, so the requests of this session are never blocked.
@property(nonatomic, strong) NSMutableSet<NSString *> * claimedPaths;

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
    // The main delegate queue serializes the session callbacks with the requests and cancellations.
    _session = [NSURLSession sessionWithConfiguration:configuration
                                             delegate:self
                                        delegateQueue:NSOperationQueue.mainQueue];
    _tasks = [NSMutableDictionary dictionary];
    _saveStrategy = saveStrategy;
    _restoredTasks = [NSMutableDictionary dictionary];
    _resumeData = [NSMutableDictionary dictionary];
    _cancelledTasks = [NSMutableSet set];
    _claimedPaths = [NSMutableSet set];

    [_session getAllTasksWithCompletionHandler:^(NSArray<__kindof NSURLSessionTask *> * _Nonnull downloadTasks) {
      dispatch_async(dispatch_get_main_queue(), ^{
        // The snapshot can be older than the requests and cancellations made since it was taken.
        for (NSURLSessionTask * downloadTask in downloadTasks)
        {
          if (self.tasks[@(downloadTask.taskIdentifier)] ||
              [self.cancelledTasks containsObject:@(downloadTask.taskIdentifier)] ||
              downloadTask.state == NSURLSessionTaskStateCanceling ||
              downloadTask.state == NSURLSessionTaskStateCompleted)
            continue;

          // A task of the previous app session duplicating a request of this one is cancelled: the caller holds
          // the identifier of the issued task, and the restored partial progress is lost.
          NSString * urlPath = downloadTask.currentRequest.URL.path;
          if ([self isSupersededTask:downloadTask] || [self hasTaskWithUrlPath:urlPath])
          {
            [downloadTask cancel];
            [self.cancelledTasks addObject:@(downloadTask.taskIdentifier)];
            continue;
          }
          [self.restoredTasks setObject:downloadTask forKey:urlPath];
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
  [self.claimedPaths addObject:[NSURL fileURLWithPath:filePath].path];
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

- (BOOL)hasTaskWithUrlPath:(NSString *)urlPath
{
  for (TaskInfo * info in self.tasks.allValues)
    if ([info.task.currentRequest.URL.path isEqualToString:urlPath])
      return YES;
  return NO;
}

/// A cancelled task, or a task of the previous app session whose destination this session requested or already
/// exists: its result must not touch the destination, the resume data or the callbacks of its owner.
- (BOOL)isSupersededTask:(NSURLSessionTask *)task
{
  if ([self.cancelledTasks containsObject:@(task.taskIdentifier)])
    return YES;
  if (self.tasks[@(task.taskIdentifier)])
    return NO;
  NSURL * destinationUrl = [self.saveStrategy getLocationForTask:task];
  // An existing destination is a complete download that Storage may reuse and validate without a request.
  return destinationUrl && ([self.claimedPaths containsObject:destinationUrl.path] ||
                            [[NSFileManager defaultManager] fileExistsAtPath:destinationUrl.path]);
}

- (void)forgetRestoredTask:(NSURLSessionTask *)task
{
  NSString * urlPath = task.currentRequest.URL.path;
  if (self.restoredTasks[urlPath] == task)
    [self.restoredTasks removeObjectForKey:urlPath];
}

#pragma mark - NSURLSessionDownloadDelegate implementation

- (void)finishDownloading:(NSURLSessionTask *)downloadTask error:(nullable NSError *)error
{
  [self forgetRestoredTask:downloadTask];

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
  // The downloaded file is deleted when this method returns.
  if ([self isSupersededTask:downloadTask])
  {
    [[NSFileManager defaultManager] removeItemAtURL:location.filePathURL error:nil];
    [self forgetRestoredTask:downloadTask];
    return;
  }

  // Check for HTTP errors.
  // TODO: Check and prevent redirects.
  NSInteger statusCode = ((NSHTTPURLResponse *)downloadTask.response).statusCode;
  // 206 for resumed downloads.
  NSError * error = nil;
  if (statusCode != 200 && statusCode != 206)
  {
    LOG(LWARNING,
        ("Failed to download", downloadTask.originalRequest.URL.absoluteString, "HTTP statusCode:", statusCode));
    error = [[NSError alloc] initWithDomain:@"app.omaps.http" code:statusCode userInfo:nil];
    [[NSFileManager defaultManager] removeItemAtURL:location.filePathURL error:nil];
  }
  else
  {
    error = [self saveDownloadedFile:location forTask:downloadTask];
  }
  [self finishDownloading:downloadTask error:error];
}

- (nullable NSError *)saveDownloadedFile:(NSURL *)location forTask:(NSURLSessionTask *)downloadTask
{
  NSURL * destinationUrl = [self.saveStrategy getLocationForTask:downloadTask];
  if (destinationUrl == nil)
  {
    LOG(LWARNING,
        ("Can't resolve a save path for a restored download:", downloadTask.currentRequest.URL.absoluteString));
    [[NSFileManager defaultManager] removeItemAtURL:location.filePathURL error:nil];
    return [[NSError alloc] initWithDomain:@"app.omaps.http" code:500 userInfo:nil];
  }

  // A restored file may arrive after Storage checks for an existing .ready file.
  // Allow the accepted task to publish its completed download.
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
  TaskInfo * info = [self.tasks objectForKey:@(downloadTask.taskIdentifier)];
  if (info)
    info.progress(totalBytesWritten, totalBytesExpectedToWrite);
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)downloadTask didCompleteWithError:(NSError *)error
{
  // The final callback of a task (after didFinishDownloadingToURL): a cancelled one can't save anymore.
  BOOL const superseded = [self isSupersededTask:downloadTask];
  [self.cancelledTasks removeObject:@(downloadTask.taskIdentifier)];
  if (superseded)
    [self forgetRestoredTask:downloadTask];
  else if (error && error.code != NSURLErrorCancelled)
    [self finishDownloading:downloadTask error:error];
}

- (void)URLSessionDidFinishEventsForBackgroundURLSession:(NSURLSession *)session
{
  if (self.backgroundCompletionHandler != nil)
    self.backgroundCompletionHandler();
}

@end
