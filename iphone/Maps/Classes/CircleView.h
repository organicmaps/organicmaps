#import <UIKit/UIKit.h>

@interface CircleView : UIView

- (id)initWithFrame:(CGRect)frame andColor:(UIColor *)color;
+ (UIImage *)createCircleImageWith:(CGFloat)diameter andColor:(UIColor *)color;
+ (UIImage *)createCircleImageWith:(CGFloat)diameter andColor:(UIColor *)color andSubview:(UIView *)view;

@end
