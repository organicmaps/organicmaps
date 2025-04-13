#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@protocol IMWMImageCoder

- (UIImage * _Nullable)imageWithData:(NSData *)data;
- (NSData * _Nullable)dataFromImage:(UIImage *)image;

@end

NS_ASSUME_NONNULL_END
