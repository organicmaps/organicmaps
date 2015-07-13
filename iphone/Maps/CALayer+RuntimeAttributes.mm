//
//  CALayer+RuntimeAttributes.m
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "CALayer+RuntimeAttributes.h"
#import "UIColor+MapsMeColor.h"

@implementation CALayer (RuntimeAttributes)

- (void)setBorderUIColor:(UIColor *)borderUIColor
{
  self.borderColor = borderUIColor.CGColor;
}

- (UIColor *)borderUIColor
{
  return [UIColor colorWithCGColor:self.borderColor];
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
  return [UIColor colorWithCGColor:self.shadowColor];
}

- (void)setShadowColorName:(NSString *)colorName
{
  self.shadowColor = [UIColor colorWithName:colorName].CGColor;
}

@end
