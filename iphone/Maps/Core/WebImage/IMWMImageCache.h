#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@protocol IMWMImageCache

- (void)imageForKey:(NSString *)imageKey
         completion:(void (^)(UIImage * _Nullable image, NSError * _Nullable error))completion;
- (void)setImage:(UIImage *)image forKey:(NSString *)imageKey;
- (void)cleanup;

@end

NS_ASSUME_NONNULL_END
