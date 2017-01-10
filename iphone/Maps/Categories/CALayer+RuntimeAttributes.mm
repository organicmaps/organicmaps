#import "CALayer+RuntimeAttributes.h"

@implementation CALayer (RuntimeAttributes)

- (void)setBorderUIColor:(UIColor *)borderUIColor
{
  self.borderColor = borderUIColor.CGColor;
}

- (UIColor *)borderUIColor
{
  return [UIColor colorWithCGColor:static_cast<CGColorRef>(self.borderColor)];
}

- (void)setBorderColorName:(NSString *)colorName
{
  self.borderColor = [UIColor colorWithName:colorName].CGColor;
}

- (void)setShadowUIColor:(UIColor *)shadowUIColor
{
  self.shadowColor = shadowUIColor.CGColor;
}

- (UIColor *)shadowUIColor
{
  return [UIColor colorWithCGColor:static_cast<CGColorRef>(self.shadowColor)];
}

- (void)setShadowColorName:(NSString *)colorName
{
  self.shadowColor = [UIColor colorWithName:colorName].CGColor;
}

@end
