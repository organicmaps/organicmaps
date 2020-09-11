//
//  NSBundle+MPAdditions.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "NSBundle+MPAdditions.h"

@implementation NSBundle (MPAdditions)

+ (NSBundle *)resourceBundleForClass:(Class)aClass {
    NSString * resourceBundlePath = [[NSBundle mainBundle] pathForResource:@"MoPub" ofType:@"bundle"];
    return (resourceBundlePath != nil ? [NSBundle bundleWithPath:resourceBundlePath] : [NSBundle bundleForClass:aClass]);
}

@end
