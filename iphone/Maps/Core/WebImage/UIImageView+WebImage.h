#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface UIImageView (WebImage)

- (void)wi_setImageWithUrl:(NSURL *)url;
- (void)wi_setImageWithUrl:(NSURL *)url
        transitionDuration:(NSTimeInterval)duration
                completion:(void (^ _Nullable)(UIImage * _Nullable image, NSError * _Nullable error))completion;
- (void)wi_cancelImageRequest;

@end

NS_ASSUME_NONNULL_END
