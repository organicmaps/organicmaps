#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface CircleView : UIView

- (id)initWithFrame:(CGRect)frame andColor:(UIColor *)color;
- (id)initWithFrame:(CGRect)frame andColor:(UIColor *)color andImageName:(nullable NSString *)imageName;

+ (UIImage *)createCircleImageWithFrameSize:(CGFloat)frameSize andDiameter:(CGFloat)diameter andColor:(UIColor *)color;
+ (UIImage *)createCircleImageWithDiameter:(CGFloat)diameter andColor:(UIColor *)color;
+ (UIImage *)createCircleImageWithDiameter:(CGFloat)diameter
                                  andColor:(UIColor *)color
                              andImageName:(NSString *)imageName;

@end

NS_ASSUME_NONNULL_END
