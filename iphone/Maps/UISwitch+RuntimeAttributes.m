#import "UISwitch+RuntimeAttributes.h"
#import "UIColor+MapsMeColor.h"

@implementation UISwitch (RuntimeAttributes)

- (void)setOnTintColorName:(NSString *)colorName
{
  self.onTintColor = [UIColor colorWithName:colorName];
}

@end
