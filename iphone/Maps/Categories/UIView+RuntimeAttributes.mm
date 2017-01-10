#import "UIView+RuntimeAttributes.h"

@implementation UIView  (RuntimeAttributes)

- (void)setBackgroundColorName:(NSString *)colorName
{
  self.backgroundColor = [UIColor colorWithName:colorName];
}

@end
