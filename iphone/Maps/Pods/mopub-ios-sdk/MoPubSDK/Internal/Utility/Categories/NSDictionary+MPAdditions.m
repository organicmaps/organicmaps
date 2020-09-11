//
//  NSDictionary+MPAdditions.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "NSDictionary+MPAdditions.h"

@implementation NSDictionary (MPAdditions)

- (NSInteger)mp_integerForKey:(id)key
{
    return [self mp_integerForKey:key defaultValue:0];
}

- (NSInteger)mp_integerForKey:(id)key defaultValue:(NSInteger)defaultVal
{
    id obj = [self objectForKey:key];
    if ([obj respondsToSelector:@selector(integerValue)]) {
        return [obj integerValue];
    }
    return defaultVal;
}

- (NSUInteger)mp_unsignedIntegerForKey:(id)key
{
    return [self mp_unsignedIntegerForKey:key defaultValue:0];
}

- (NSUInteger)mp_unsignedIntegerForKey:(id)key defaultValue:(NSUInteger)defaultVal
{
    id obj = [self objectForKey:key];
    if ([obj respondsToSelector:@selector(unsignedIntValue)]) {
        return [obj unsignedIntValue];
    }
    return defaultVal;
}

- (double)mp_doubleForKey:(id)key
{
    return [self mp_doubleForKey:key defaultValue:0.0];
}

- (double)mp_doubleForKey:(id)key defaultValue:(double)defaultVal
{
    id obj = [self objectForKey:key];
    if ([obj respondsToSelector:@selector(doubleValue)]) {
        return [obj doubleValue];
    }
    return defaultVal;
}

- (NSString *)mp_stringForKey:(id)key
{
    return [self mp_stringForKey:key defaultValue:nil];
}

- (NSString *)mp_stringForKey:(id)key defaultValue:(NSString *)defaultVal
{
    id obj = [self objectForKey:key];
    if ([obj isKindOfClass:[NSString class]]) {
        return obj;
    }
    return defaultVal;
}

- (BOOL)mp_boolForKey:(id)key
{
    return [self mp_boolForKey:key defaultValue:NO];
}

- (BOOL)mp_boolForKey:(id)key defaultValue:(BOOL)defaultVal
{
    id obj = [self objectForKey:key];
    if ([obj respondsToSelector:@selector(boolValue)]) {
        return [obj boolValue];
    }
    return defaultVal;
}

- (float)mp_floatForKey:(id)key
{
    return [self mp_floatForKey:key defaultValue:0];
}

- (float)mp_floatForKey:(id)key defaultValue:(float)defaultVal
{
    id obj = [self objectForKey:key];
    if ([obj respondsToSelector:@selector(floatValue)]) {
        return [obj floatValue];
    }
    return defaultVal;
}

@end
