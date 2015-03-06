//
//  CALayer+RuntimeAttributes.m
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "CALayer+RuntimeAttributes.h"

@implementation CALayer (RuntimeAttributes)

- (void)setBorderUIColor:(UIColor *)borderUIColor
{
  self.borderColor = borderUIColor.CGColor;
}

- (UIColor *)borderUIColor
{
  return [UIColor colorWithCGColor:self.borderColor];
}


- (void)setShadowUIColor:(UIColor *)shadowUIColor {
  self.shadowColor = shadowUIColor.CGColor;
}

- (UIColor *)shadowUIColor {
  return [UIColor colorWithCGColor:self.shadowColor];
}

@end
