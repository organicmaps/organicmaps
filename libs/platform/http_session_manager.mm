#import "platform/http_session_manager.h"

@interface DataTaskInfo : NSObject

@property(nonatomic, weak) id<NSURLSessionDataDelegate> delegate;
@property(nonatomic) NSURLSessionDataTask * task;

- (instancetype)initWithTask:(NSURLSessionDataTask *)task
                    delegate:(id<NSURLSessionDataDelegate>)delegate;

@end

@implementation DataTaskInfo

- (instancetype)initWithTask:(NSURLSessionDataTask *)task
                    delegate:(id<NSURLSessionDataDelegate>)delegate
{
  self = [super init];
  if (self)
  {
    _task = task;
    _delegate = delegate;
  }

  return self;
}

@end

@interface HttpSessionManager ()<NSURLSessionDataDelegate>

@property(nonatomic) NSURLSession * session;
@property(nonatomic) NSMutableDictionary * taskInfoByTaskID;
@property(nonatomic) dispatch_queue_t taskInfoQueue;
@property(nonatomic) dispatch_queue_t delegateQueue;

@end

@implementation HttpSessionManager

+ (HttpSessionManager *)sharedManager
{
  static dispatch_once_t sOnceToken;
  static HttpSessionManager * sManager;
  dispatch_once(&sOnceToken, ^{
    sManager = [[HttpSessionManager alloc] init];
  });

  return sManager;
}

- (instancetype)init
{
  self = [super init];
  if (self)
  {
    _session = [NSURLSession sessionWithConfiguration:[NSURLSessionConfiguration ephemeralSessionConfiguration]
                                             delegate:self
                                        delegateQueue:nil];
    _taskInfoByTaskID = [NSMutableDictionary dictionary];
    _taskInfoQueue = dispatch_queue_create("http_session_manager.queue", DISPATCH_QUEUE_CONCURRENT);
    // TODO(AB): As the main thread in tests that are using synchronous HTTP calls is blocked
    // by dispatch_group_wait(group, DISPATCH_TIME_FOREVER) in http_client_apple.mm,
    // and delegate should not strictly use only one (main) thread and can be run on
    // any thread, this workaround is needed.
    // Refactor out the whole sync HTTP implementation to use async + lambdas/callbacks.
    BOOL const isSyncHttpTest = [NSBundle.mainBundle.executableURL.lastPathComponent hasSuffix:@"osm_auth_tests"];
    _delegateQueue = isSyncHttpTest ? _taskInfoQueue : dispatch_get_main_queue();
  }

  return self;
}

- (NSURLSessionDataTask *)dataTaskWithRequest:(NSURLRequest *)request
                                     delegate:(id<NSURLSessionDataDelegate>)delegate
                            completionHandler:(void (^)(NSData * data, NSURLResponse * response,
                                                        NSError * error))completionHandler
{
  NSURLSessionDataTask * task = [self.session dataTaskWithRequest:request
                                                completionHandler:completionHandler];

  DataTaskInfo * taskInfo = [[DataTaskInfo alloc] initWithTask:task delegate:delegate];
  [self setDataTaskInfo:taskInfo forTask:task];

  return task;
}

- (void)setDataTaskInfo:(DataTaskInfo *)taskInfo forTask:(NSURLSessionTask *)task
{
  dispatch_barrier_sync(self.taskInfoQueue, ^{
    self.taskInfoByTaskID[@(task.taskIdentifier)] = taskInfo;
  });
}

- (void)removeTaskInfoForTask:(NSURLSessionTask *)task
{
  dispatch_barrier_sync(self.taskInfoQueue, ^{
    [self.taskInfoByTaskID removeObjectForKey:@(task.taskIdentifier)];
  });
}

- (DataTaskInfo *)taskInfoForTask:(NSURLSessionTask *)task
{
  __block DataTaskInfo * taskInfo = nil;
  dispatch_sync(self.taskInfoQueue, ^{
    taskInfo = self.taskInfoByTaskID[@(task.taskIdentifier)];
  });

  return taskInfo;
}

- (void)URLSession:(NSURLSession *)session
                          task:(NSURLSessionTask *)task
    willPerformHTTPRedirection:(NSHTTPURLResponse *)response
                    newRequest:(NSURLRequest *)newRequest
             completionHandler:(void (^)(NSURLRequest *))completionHandler
{
  DataTaskInfo * taskInfo = [self taskInfoForTask:task];
  if ([taskInfo.delegate
       respondsToSelector:@selector(URLSession:task:willPerformHTTPRedirection:newRequest:completionHandler:)])
  {
    dispatch_async(self.delegateQueue, ^{
      [taskInfo.delegate URLSession:session
                                task:task
          willPerformHTTPRedirection:response
                          newRequest:newRequest
                   completionHandler:completionHandler];
    });
  }
  else
  {
    completionHandler(newRequest);
  }
}

- (void)URLSession:(NSURLSession *)session
                    task:(NSURLSessionTask *)task
    didCompleteWithError:(NSError *)error
{
  DataTaskInfo * taskInfo = [self taskInfoForTask:task];
  [self removeTaskInfoForTask:task];

  if ([taskInfo.delegate respondsToSelector:@selector(URLSession:task:didCompleteWithError:)])
  {
    dispatch_async(self.delegateQueue, ^{
      [taskInfo.delegate URLSession:session task:task didCompleteWithError:error];
    });
  }
}

- (void)URLSession:(NSURLSession *)session
              dataTask:(NSURLSessionDataTask *)dataTask
    didReceiveResponse:(NSURLResponse *)response
     completionHandler:(void (^)(NSURLSessionResponseDisposition disposition))completionHandler
{
  DataTaskInfo * taskInfo = [self taskInfoForTask:dataTask];
  if ([taskInfo.delegate
          respondsToSelector:@selector(URLSession:dataTask:didReceiveResponse:completionHandler:)])
  {
    dispatch_async(self.delegateQueue, ^{
      [taskInfo.delegate URLSession:session
                           dataTask:dataTask
                 didReceiveResponse:response
                  completionHandler:completionHandler];
    });
  }
  else
  {
    completionHandler(NSURLSessionResponseAllow);
  }
}

- (void)URLSession:(NSURLSession *)session
          dataTask:(NSURLSessionDataTask *)dataTask
    didReceiveData:(NSData *)data
{
  DataTaskInfo * taskInfo = [self taskInfoForTask:dataTask];
  if ([taskInfo.delegate respondsToSelector:@selector(URLSession:dataTask:didReceiveData:)])
  {
    dispatch_async(self.delegateQueue, ^{
      [taskInfo.delegate URLSession:session dataTask:dataTask didReceiveData:data];
    });
  }
}

#if DEBUG
- (void)URLSession:(NSURLSession *)session
    didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge
      completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition,
                                  NSURLCredential * _Nullable credential))completionHandler
{
  NSURLCredential * credential =
      [[NSURLCredential alloc] initWithTrust:[challenge protectionSpace].serverTrust];
  completionHandler(NSURLSessionAuthChallengeUseCredential, credential);
}
#endif

@end
