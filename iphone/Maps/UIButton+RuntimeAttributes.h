#import <UIKit/UIKit.h>

@interface UIButton (RuntimeAttributes)

@property (nonatomic) NSString * localizedText;

- (void)setBackgroundColor:(UIColor *)color forState:(UIControlState)state;

@end