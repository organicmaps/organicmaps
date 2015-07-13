//
//  UIFont+MapsMeFonts.m
//  Maps
//
//  Created by Ilya Grechuhin on 08.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "UIFont+MapsMeFonts.h"

@implementation UIFont (MapsMeFonts)

+ (UIFont *)regular10
{
  return [UIFont fontWithName:@"HelveticaNeue" size:10];
}

+ (UIFont *)regular14
{
  return [UIFont fontWithName:@"HelveticaNeue" size:14];
}

+ (UIFont *)regular16
{
  return [UIFont fontWithName:@"HelveticaNeue" size:16];
}

+ (UIFont *)regular17
{
  return [UIFont fontWithName:@"HelveticaNeue" size:17];
}

+ (UIFont *)regular18
{
  return [UIFont fontWithName:@"HelveticaNeue" size:18];
}

+ (UIFont *)regular24
{
  return [UIFont fontWithName:@"HelveticaNeue" size:24];
}

+ (UIFont *)light10
{
  return [UIFont fontWithName:@"HelveticaNeue-Light" size:10];
}

+ (UIFont *)light12
{
  return [UIFont fontWithName:@"HelveticaNeue-Light" size:12];
}

+ (UIFont *)light16
{
  return [UIFont fontWithName:@"HelveticaNeue-Light" size:16];
}

+ (UIFont *)light17
{
  return [UIFont fontWithName:@"HelveticaNeue-Light" size:17];
}

+ (UIFont *)bold16
{
  return [UIFont fontWithName:@"HelveticaNeue-Bold" size:16];
}

+ (UIFont *)bold48
{
  return [UIFont fontWithName:@"HelveticaNeue-Bold" size:48];
}

+ (UIFont *)fontWithName:(NSString *)fontName
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  return [[UIFont class] performSelector:NSSelectorFromString(fontName)];
#pragma clang diagnostic pop
}

@end
