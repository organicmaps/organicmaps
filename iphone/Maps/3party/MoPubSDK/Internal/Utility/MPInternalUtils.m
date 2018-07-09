//
//  MPInternalUtils.m
//  MoPubSDK
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import "MPInternalUtils.h"

@implementation MPInternalUtils

@end

@implementation NSMutableDictionary (MPInternalUtils)

- (void)mp_safeSetObject:(id)obj forKey:(id<NSCopying>)key
{
    if (obj != nil) {
        [self setObject:obj forKey:key];
    }
}

- (void)mp_safeSetObject:(id)obj forKey:(id<NSCopying>)key withDefault:(id)defaultObj
{
    if (obj != nil) {
        [self setObject:obj forKey:key];
    } else {
        [self setObject:defaultObj forKey:key];
    }
}

@end
