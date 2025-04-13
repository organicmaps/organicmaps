#import "IMWMImageCache.h"
#import "IMWMImageCoder.h"

NS_ASSUME_NONNULL_BEGIN

@interface MWMImageCache : NSObject <IMWMImageCache>

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithImageCoder:(id<IMWMImageCoder>)imageCoder;

@end

NS_ASSUME_NONNULL_END
