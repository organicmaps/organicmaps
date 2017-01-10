#import "UISwitch+RuntimeAttributes.h"

@implementation UISwitch (RuntimeAttributes)

- (void)setOnTintColorName:(NSString *)colorName
{
  self.onTintColor = [UIColor colorWithName:colorName];
}

@end
