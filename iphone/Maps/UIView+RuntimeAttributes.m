//
//  UIView+RuntimeAttributes.m
//  Maps
//
//  Created by Ilya Grechuhin on 09.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "UIColor+MapsMeColor.h"
#import "UIView+RuntimeAttributes.h"

@implementation UIView  (RuntimeAttributes)

- (void)setBackgroundColorName:(NSString *)colorName
{
  self.backgroundColor = [UIColor colorWithName:colorName];
}

@end
