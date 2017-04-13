//
//  UIColor+MPAdditions.m
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "UIColor+MPAdditions.h"

@implementation UIColor (MPAdditions)

+ (UIColor *)mp_colorFromHexString:(NSString *)hexString alpha:(CGFloat)alpha
{
    unsigned rgbValue = 0;
    NSScanner *scanner = [NSScanner scannerWithString:hexString];
    [scanner setScanLocation:1]; // bypass '#' character
    [scanner scanHexInt:&rgbValue];
    return [UIColor colorWithRed:((rgbValue & 0xFF0000) >> 16)/255.0 green:((rgbValue & 0xFF00) >> 8)/255.0 blue:(rgbValue & 0xFF)/255.0 alpha:alpha];
}

@end
