#import "Common.h"
#import "UIFont+MapsMeFonts.h"

#include "std/map.hpp"

typedef NS_ENUM(NSUInteger, FontWeight)
{
  FontWeightRegular,
  FontWeightMedium,
  FontWeightLight,
  FontWeightBold
};

NSString * fontName(FontWeight weight, CGFloat size)
{
  if (!isIOSVersionLessThan(9))
  {
    if (size < 20)
    {
      switch (weight)
      {
        case FontWeightRegular: return @".SFUIText-Regular";
        case FontWeightMedium: return @".SFUIText-Medium";
        case FontWeightLight: return @".SFUIText-Light";
        case FontWeightBold: return @".SFUIText-Bold";
      }
    }
    else
    {
      switch (weight)
      {
        case FontWeightRegular: return @".SFUIDisplay-Regular";
        case FontWeightMedium: return @".SFUIDisplay-Medium";
        case FontWeightLight: return @".SFUIDisplay-Light";
        case FontWeightBold: return @".SFUIDisplay-Bold";
      }
    }
  }
  switch (weight)
  {
    case FontWeightRegular: return @"HelveticaNeue";
    case FontWeightMedium: return @"HelveticaNeue-Medium";
    case FontWeightLight: return @"HelveticaNeue-Light";
    case FontWeightBold: return @"HelveticaNeue-Bold";
  }
}

@implementation UIFont (MapsMeFonts)

+ (UIFont *)regular10
{
  CGFloat const size = 10;
  return [UIFont fontWithName:fontName(FontWeightRegular, size) size:size];
}

+ (UIFont *)regular12
{
  CGFloat const size = 12;
  return [UIFont fontWithName:fontName(FontWeightRegular, size) size:size];
}

+ (UIFont *)regular13
{
  CGFloat const size = 13;
  return [UIFont fontWithName:fontName(FontWeightRegular, size) size:size];
}

+ (UIFont *)regular14
{
  CGFloat const size = 14;
  return [UIFont fontWithName:fontName(FontWeightRegular, size) size:size];
}

+ (UIFont *)regular15
{
  CGFloat const size = 15;
  return [UIFont fontWithName:fontName(FontWeightRegular, size) size:size];
}

+ (UIFont *)regular16
{
  CGFloat const size = 16;
  return [UIFont fontWithName:fontName(FontWeightRegular, size) size:size];
}

+ (UIFont *)regular17
{
  CGFloat const size = 17;
  return [UIFont fontWithName:fontName(FontWeightRegular, size) size:size];
}

+ (UIFont *)regular18
{
  CGFloat const size = 18;
  return [UIFont fontWithName:fontName(FontWeightRegular, size) size:size];
}

+ (UIFont *)regular24
{
  CGFloat const size = 24;
  return [UIFont fontWithName:fontName(FontWeightRegular, size) size:size];
}

+ (UIFont *)regular32
{
  CGFloat const size = 32;
  return [UIFont fontWithName:fontName(FontWeightRegular, size) size:size];
}

+ (UIFont *)regular52
{
  CGFloat const size = 52;
  return [UIFont fontWithName:fontName(FontWeightRegular, size) size:size];
}

+ (UIFont *)medium10
{
  CGFloat const size = 10;
  return [UIFont fontWithName:fontName(FontWeightMedium, size) size:size];
}
+ (UIFont *)medium14
{
  CGFloat const size = 14;
  return [UIFont fontWithName:fontName(FontWeightMedium, size) size:size];
}
+ (UIFont *)medium16
{
  CGFloat const size = 16;
  return [UIFont fontWithName:fontName(FontWeightMedium, size) size:size];
}
+ (UIFont *)medium17
{
  CGFloat const size = 17;
  return [UIFont fontWithName:fontName(FontWeightMedium, size) size:size];
}
+ (UIFont *)medium18
{
  CGFloat const size = 18;
  return [UIFont fontWithName:fontName(FontWeightMedium, size) size:size];
}

+ (UIFont *)medium20
{
  CGFloat const size = 20;
  return [UIFont fontWithName:fontName(FontWeightMedium, size) size:size];
}

+ (UIFont *)medium24
{
  CGFloat const size = 24;
  return [UIFont fontWithName:fontName(FontWeightMedium, size) size:size];
}

+ (UIFont *)medium28
{
  CGFloat const size = 28;
  return [UIFont fontWithName:fontName(FontWeightMedium, size) size:size];
}

+ (UIFont *)medium36
{
  CGFloat const size = 36;
  return [UIFont fontWithName:fontName(FontWeightMedium, size) size:size];
}

+ (UIFont *)medium40
{
  CGFloat const size = 40;
  return [UIFont fontWithName:fontName(FontWeightMedium, size) size:size];
}

+ (UIFont *)medium44
{
  CGFloat const size = 44;
  return [UIFont fontWithName:fontName(FontWeightMedium, size) size:size];
}

+ (UIFont *)light10
{
  CGFloat const size = 10;
  return [UIFont fontWithName:fontName(FontWeightLight, size) size:size];
}

+ (UIFont *)light12
{
  CGFloat const size = 12;
  return [UIFont fontWithName:fontName(FontWeightLight, size) size:size];
}

+ (UIFont *)light16
{
  CGFloat const size = 16;
  return [UIFont fontWithName:fontName(FontWeightLight, size) size:size];
}

+ (UIFont *)light17
{
  CGFloat const size = 17;
  return [UIFont fontWithName:fontName(FontWeightLight, size) size:size];
}

+ (UIFont *)bold12
{
  CGFloat const size = 12;
  return [UIFont fontWithName:fontName(FontWeightBold, size) size:size];
}

+ (UIFont *)bold14
{
  CGFloat const size = 14;
  return [UIFont fontWithName:fontName(FontWeightBold, size) size:size];
}

+ (UIFont *)bold16
{
  CGFloat const size = 16;
  return [UIFont fontWithName:fontName(FontWeightBold, size) size:size];
}

+ (UIFont *)bold17
{
  CGFloat const size = 17;
  return [UIFont fontWithName:fontName(FontWeightBold, size) size:size];
}

+ (UIFont *)bold48
{
  CGFloat const size = 48;
  return [UIFont fontWithName:fontName(FontWeightBold, size) size:size];
}

+ (UIFont *)fontWithName:(NSString *)fontName
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  return [[UIFont class] performSelector:NSSelectorFromString(fontName)];
#pragma clang diagnostic pop
}

@end
