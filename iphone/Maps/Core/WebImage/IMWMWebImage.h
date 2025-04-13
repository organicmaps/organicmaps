#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@protocol IMWMImageTask <NSObject>

- (void)cancel;

@end

typedef void (^MWMWebImageCompletion)(UIImage * _Nullable image, NSError * _Nullable error);

@protocol IMWMWebImage

- (id<IMWMImageTask>)imageWithUrl:(NSURL *)url
                       completion:(MWMWebImageCompletion)completion;
- (void)cleanup;

@end

NS_ASSUME_NONNULL_END
