#import "MWMWebImage.h"
#import "MWMImageCache.h"

@interface MWMWebImageTask : NSObject <IMWMImageTask>

@property (nonatomic) BOOL cancelled;
@property (nonatomic, strong) NSURLSessionTask *dataTask;

@end

@implementation MWMWebImageTask

- (void)cancel {
  self.cancelled = YES;
  [self.dataTask cancel];
}

@end

@interface MWMWebImage () <NSURLSessionDelegate>

@property (nonatomic, strong) NSURLSession *urlSession;
@property (nonatomic, strong) id<IMWMImageCache> imageCache;

@end

@implementation MWMWebImage

+ (MWMWebImage *)defaultWebImage {
  static MWMWebImage *instanse;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instanse = [[self alloc] initWithImageCahce:[MWMImageCache new]];
  });
  return instanse;
}

- (instancetype)initWithImageCahce:(id<IMWMImageCache>)imageCache {
  self = [super init];
  if (self) {
    _urlSession = [NSURLSession sessionWithConfiguration:[NSURLSessionConfiguration ephemeralSessionConfiguration]
                                                delegate:self
                                           delegateQueue:nil];
    _imageCache = imageCache;
  }
  return self;
}

- (id<IMWMImageTask>)imageWithUrl:(NSURL *)url
          completion:(MWMWebImageCompletion)completion {
  MWMWebImageTask *imageTask = [MWMWebImageTask new];
  NSString *cacheKey = url.absoluteString;
  [self.imageCache imageForKey:cacheKey completion:^(UIImage *image, NSError *error) {
    if (imageTask.cancelled) {
      return;
    }

    if (image) {
      completion(image, nil);
    } else {
      NSURLSessionTask *dataTask = [self.urlSession dataTaskWithURL:url
                      completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
                        UIImage *image = nil;
                        if (data) {
                          image = [UIImage imageWithData:data];
                          if (image) {
                            [self.imageCache setImage:image forKey:cacheKey];
                          }
                        }

                        dispatch_async(dispatch_get_main_queue(), ^{
                          if (!imageTask.cancelled) {
                            completion(image, error); //TODO: replace error with generic error
                          }
                        });
                      }];
      imageTask.dataTask = dataTask;
      [dataTask resume];
    }
  }];

  return imageTask;
}

- (void)cleanup {
  [self.imageCache cleanup];
}

#pragma mark - NSURLSessionDelegate

#if DEBUG
- (void)URLSession:(NSURLSession *)session
didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge
completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition,
                            NSURLCredential *credential))completionHandler
{
  NSURLCredential *credential = [NSURLCredential credentialForTrust:challenge.protectionSpace.serverTrust];
  completionHandler(NSURLSessionAuthChallengeUseCredential, credential);
}
#endif

@end
