//
//  UIColor+MPAdditions.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>

@interface UIColor (MPAdditions)

+ (UIColor *)mp_colorFromHexString:(NSString *)hexString alpha:(CGFloat)alpha;

@end
