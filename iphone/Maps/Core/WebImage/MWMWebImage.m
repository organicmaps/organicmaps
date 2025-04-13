#import "MWMWebImage.h"
#import "MWMImageCache.h"
#import "MWMImageCoder.h"

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
@property (nonatomic, strong) id<IMWMImageCoder> imageCoder;

@end

@implementation MWMWebImage

+ (MWMWebImage *)defaultWebImage {
  static MWMWebImage *instanse;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    MWMImageCoder *coder = [MWMImageCoder new];
    instanse = [[self alloc] initWithImageCahce:[[MWMImageCache alloc] initWithImageCoder:coder]
                                     imageCoder:coder];
  });
  return instanse;
}

- (instancetype)initWithImageCahce:(id<IMWMImageCache>)imageCache
                        imageCoder:(id<IMWMImageCoder>)imageCoder {
  self = [super init];
  if (self) {
    _urlSession = [NSURLSession sessionWithConfiguration:[NSURLSessionConfiguration ephemeralSessionConfiguration]
                                                delegate:self
                                           delegateQueue:nil];
    _imageCache = imageCache;
    _imageCoder = imageCoder;
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
                          image = [self.imageCoder imageWithData:data];
                          if (image) {
                            [self.imageCache setImage:image forKey:cacheKey];
                          }
                        }

                        dispatch_async(dispatch_get_main_queue(), ^{
                          if (!imageTask.cancelled) {
                            completion(image, error);
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
