#import <UIKit/UIKit.h>

@interface CircleView : UIView

- (id)initWithFrame:(CGRect)frame andColor:(UIColor *)color;
- (id)initWithFrame:(CGRect)frame andColor:(UIColor *)color andImageName:(NSString *)imageName;
+ (UIImage *)createCircleImageWith:(CGFloat)diameter andColor:(UIColor *)color;
+ (UIImage *)createCircleImageWith:(CGFloat)diameter andColor:(UIColor *)color andImageName:(NSString *)imageName;
+ (UIImage *)createCircleImageWith:(CGFloat)diameter andColor:(UIColor *)color andSubview:(UIView *)view;

@end
