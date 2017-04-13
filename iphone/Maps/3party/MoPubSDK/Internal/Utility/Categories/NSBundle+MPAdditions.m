//
//  NSBundle+MPAdditions.m
//  MoPubSDK
//
//  Copyright © 2016 MoPub. All rights reserved.
//

#import "NSBundle+MPAdditions.h"

@implementation NSBundle (MPAdditions)

+ (NSBundle *)resourceBundleForClass:(Class)aClass {
    NSString * resourceBundlePath = [[NSBundle mainBundle] pathForResource:@"MoPub" ofType:@"bundle"];
    return (resourceBundlePath != nil ? [NSBundle bundleWithPath:resourceBundlePath] : [NSBundle bundleForClass:aClass]);
}

@end
