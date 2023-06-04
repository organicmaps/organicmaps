#import "MWMImageCache.h"
#import "NSString+MD5.h"

static NSTimeInterval kCleanupTimeInterval = 30 * 24 * 60 * 60;

@interface MWMImageCache ()

@property (nonatomic, strong) NSCache<NSString *, UIImage *> *cache;
@property (nonatomic, copy) NSString *cacheDirPath;
@property (nonatomic, strong) dispatch_queue_t diskQueue;
@property (nonatomic, strong) NSFileManager *fileManager;
@property (nonatomic, strong) id<IMWMImageCoder> imageCoder;

@end

@implementation MWMImageCache

- (instancetype)initWithImageCoder:(id<IMWMImageCoder>)imageCoder {
  self = [super init];
  if (self) {
    _cache = [[NSCache alloc] init];
    _cacheDirPath = [NSTemporaryDirectory() stringByAppendingPathComponent:@"images"];
    _diskQueue = dispatch_queue_create("mapsme.imageCache.disk", DISPATCH_QUEUE_SERIAL);
    _fileManager = [NSFileManager defaultManager];
    _imageCoder = imageCoder;

    [_fileManager createDirectoryAtPath:_cacheDirPath
            withIntermediateDirectories:YES
                             attributes:nil
                                  error:nil];
  }

  return self;
}

- (void)imageForKey:(NSString *)imageKey completion:(void (^)(UIImage *image, NSError *error))completion {
  UIImage *image = [self.cache objectForKey:imageKey];
  if (image) {
    completion(image, nil);
  } else {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
      NSString *path = [self.cacheDirPath stringByAppendingPathComponent:imageKey.md5String];
      __block NSData *imageData = nil;
      __block NSError *error = nil;
      dispatch_sync(self.diskQueue, ^{
        imageData = [NSData dataWithContentsOfFile:path options:0 error:&error];
      });
      UIImage *image = nil;
      if (imageData) {
        image = [self.imageCoder imageWithData:imageData];
        if (image) {
          [self.cache setObject:image forKey:imageKey];
        }
      }
      dispatch_async(dispatch_get_main_queue(), ^{
        completion(image, error);
      });
    });
  }
}

- (void)setImage:(UIImage *)image forKey:(NSString *)imageKey {
  [self.cache setObject:image forKey:imageKey];
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    NSData *imageData = [self.imageCoder dataFromImage:image];
    if (imageData) {
      NSString *path = [self.cacheDirPath stringByAppendingPathComponent:imageKey.md5String];
      dispatch_sync(self.diskQueue, ^{
        [imageData writeToFile:path atomically:YES];
      });
    }
  });
}

- (void)cleanup {
  NSDirectoryEnumerator<NSString *> *enumerator = [self.fileManager enumeratorAtPath:self.cacheDirPath];
  for (NSString *fileName in enumerator) {
    NSString *path = [self.cacheDirPath stringByAppendingPathComponent:fileName];
    NSError *error = nil;
    NSDictionary *attributes = [self.fileManager attributesOfItemAtPath:path error:&error];
    if (!error) {
      NSDate *date = attributes[NSFileCreationDate];
      if (fabs(date.timeIntervalSinceNow) > kCleanupTimeInterval) {
        dispatch_sync(self.diskQueue, ^{
          [self.fileManager removeItemAtPath:path error:nil];
        });
      }
    }
  }
}

@end
