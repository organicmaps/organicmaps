//
//  MWMPlacePage+MapsMeColor.m
//  Maps
//
//  Created by v.mikhaylenko on 25.06.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "UIColor+MapsMeColor.h"

static CGFloat const alpha12 = 0.12;
static CGFloat const alpha26 = 0.26;
static CGFloat const alpha54 = 0.54;
static CGFloat const alpha80 = 0.80;
static CGFloat const alpha87 = 0.87;
static CGFloat const alpha100 = 1.;

@implementation UIColor (MapsMeColor)

// Dark green color
+ (UIColor *)primaryDark
{
  return [UIColor colorWithRed:scaled(24.) green:scaled(128) blue:scaled(68.) alpha:alpha100];
}

// Green color
+ (UIColor *)primary
{
  return [UIColor colorWithRed:scaled(32.) green:scaled(152.) blue:scaled(82.) alpha:alpha100];
}

// Light green color
+ (UIColor *)primaryLight
{
  return [UIColor colorWithRed:scaled(36.) green:scaled(180.) blue:scaled(98.) alpha:alpha100];
}

// Use for opaque fullscreen
+ (UIColor *)fadeBackground
{
  return [[UIColor blackColor] colorWithAlphaComponent:alpha80];
}

// Background color && press color
+ (UIColor *)pressBackground
{
  return [UIColor colorWithRed:scaled(245.) green:scaled(245.) blue:scaled(245.) alpha:alpha100];
}
// Red color (use for status closed in place page)
+ (UIColor *)red
{
  return [UIColor colorWithRed:scaled(230.) green:scaled(15.) blue:scaled(35.) alpha:alpha100];
}
// Orange color (use for status 15 min in place page)
+ (UIColor *)orange
{
  return [UIColor colorWithRed:1. green:scaled(120.) blue:scaled(5.) alpha:alpha100];
}

// Blue color (use for links and phone numbers)
+ (UIColor *)linkBlue
{
  return [UIColor colorWithRed:scaled(30.) green:scaled(150.) blue:scaled(240.) alpha:alpha100];
}

+ (UIColor *)blackPrimaryText
{
  return [[UIColor blackColor] colorWithAlphaComponent:alpha87];
}

+ (UIColor *)blackSecondaryText
{
  return [[UIColor blackColor] colorWithAlphaComponent:alpha54];
}

+ (UIColor *)blackHintText
{
  return [[UIColor blackColor] colorWithAlphaComponent:alpha26];
}

+ (UIColor *)blackDividers
{
  return [[UIColor blackColor] colorWithAlphaComponent:alpha12];
}

+ (UIColor *)whitePrimaryText
{
  return [[UIColor whiteColor] colorWithAlphaComponent:alpha87];
}

+ (UIColor *)whiteSecondaryText
{
  return [[UIColor whiteColor] colorWithAlphaComponent:alpha54];
}

+ (UIColor *)whiteHintText
{
  return [[UIColor whiteColor] colorWithAlphaComponent:alpha26];
}

+ (UIColor *)whiteDividers
{
  return [[UIColor whiteColor] colorWithAlphaComponent:alpha12];
}

+ (UIColor *)buttonEnabledBlueText
{
  return [UIColor colorWithRed:scaled(3.) green:scaled(122.) blue:scaled(255.) alpha:alpha100];
}

+ (UIColor *)buttonDisabledBlueText
{
  return [self.buttonEnabledBlueText colorWithAlphaComponent:alpha26];
}

+ (UIColor *)buttonHighlightedBlueText
{
  return [UIColor colorWithRed:scaled(3.) green:scaled(122.) blue:scaled(255.) alpha:alpha54];
}

+ (UIColor *)alertBackground
{
  return [UIColor colorWithWhite:1. alpha:.88f];
}

+ (UIColor *)colorWithName:(NSString *)colorName
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  return [[UIColor class] performSelector:NSSelectorFromString(colorName)];
#pragma clang diagnostic pop
}

CGFloat scaled(CGFloat f)
{
  return f / 255.;
}

@end
