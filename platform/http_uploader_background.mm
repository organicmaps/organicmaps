#import <Foundation/Foundation.h>

#include "http_uploader_background.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

static NSString *const kSessionId = @"MWMBackgroundUploader_sessionId";

@interface MWMBackgroundUploader : NSObject <NSURLSessionDelegate>

@property(nonatomic, strong) NSURLSession *session;

+ (MWMBackgroundUploader *)sharedUploader;
- (void)upload:(platform::HttpPayload const &)payload;

@end

@implementation MWMBackgroundUploader

+ (MWMBackgroundUploader *)sharedUploader {
  static MWMBackgroundUploader *uploader;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    uploader = [[MWMBackgroundUploader alloc] init];
  });
  return uploader;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    NSURLSessionConfiguration *config =
      [NSURLSessionConfiguration backgroundSessionConfigurationWithIdentifier:kSessionId];
    config.allowsCellularAccess = NO;
    config.sessionSendsLaunchEvents = NO;
    config.discretionary = YES;
    _session = [NSURLSession sessionWithConfiguration:config delegate:self delegateQueue:nil];
  }
  return self;
}

- (void)upload:(platform::HttpPayload const &)payload {
  NSURLComponents *components = [[NSURLComponents alloc] initWithString:@(payload.m_url.c_str())];
  NSMutableArray *newQueryItems = [NSMutableArray arrayWithArray:components.queryItems];
  std::for_each(payload.m_params.begin(), payload.m_params.end(), [newQueryItems](auto const &pair) {
    [newQueryItems addObject:[NSURLQueryItem queryItemWithName:@(pair.first.c_str()) value:@(pair.second.c_str())]];
  });
  [components setQueryItems:newQueryItems];
  NSURL *url = components.URL;
  CHECK(url, ());

  NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
  request.HTTPMethod = @(payload.m_method.c_str());
  std::for_each(payload.m_headers.begin(), payload.m_headers.end(), [request](auto const &pair) {
    [request addValue:@(pair.second.c_str()) forHTTPHeaderField:@(pair.first.c_str())];
  });

  NSURL *fileUrl = [NSURL fileURLWithPath:@(payload.m_filePath.c_str())];
  CHECK(fileUrl, ());

  [[self.session uploadTaskWithRequest:request fromFile:fileUrl] resume];
  NSError *error;
  [[NSFileManager defaultManager] removeItemAtURL:fileUrl error:&error];
  if (error) {
    LOG(LDEBUG, ([error description].UTF8String));
  }
}

- (void)URLSession:(NSURLSession *)session
                  task:(NSURLSessionTask *)task
  didCompleteWithError:(nullable NSError *)error {
  if (error) {
    LOG(LDEBUG, ("Upload failed:", [error description].UTF8String));
  }
}

#if DEBUG
- (void)URLSession:(NSURLSession *)session
  didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge
    completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition,
                                NSURLCredential *_Nullable credential))completionHandler {
  NSURLCredential *credential = [[NSURLCredential alloc] initWithTrust:[challenge protectionSpace].serverTrust];
  completionHandler(NSURLSessionAuthChallengeUseCredential, credential);
}
#endif

@end

namespace platform {
void HttpUploaderBackground::Upload() const {
  [[MWMBackgroundUploader sharedUploader] upload:GetPayload()];
}
}
