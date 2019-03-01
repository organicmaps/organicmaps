#import "UIView+RuntimeAttributes.h"

@implementation UIView (RuntimeAttributes)

- (void)setBackgroundColorName:(NSString *)colorName {
  self.backgroundColor = [UIColor colorWithName:colorName];
}

- (void)setTintColorName:(NSString *)colorName {
  self.tintColor = [UIColor colorWithName:colorName];
}

@end
