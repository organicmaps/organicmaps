//
//  UIColor+MPAdditions.h
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface UIColor (MPAdditions)

+ (UIColor *)mp_colorFromHexString:(NSString *)hexString alpha:(CGFloat)alpha;

@end
