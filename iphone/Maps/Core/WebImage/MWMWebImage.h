#import "IMWMImageCache.h"
#import "IMWMImageCoder.h"
#import "IMWMWebImage.h"

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface MWMWebImage : NSObject <IMWMWebImage>

+ (MWMWebImage *)defaultWebImage;

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithImageCahce:(id<IMWMImageCache>)imageCache imageCoder:(id<IMWMImageCoder>)imageCoder;
- (id<IMWMImageTask>)imageWithUrl:(NSURL *)url callback:(MWMWebImageCompletion)callback;
- (void)cleanup;

@end

NS_ASSUME_NONNULL_END
