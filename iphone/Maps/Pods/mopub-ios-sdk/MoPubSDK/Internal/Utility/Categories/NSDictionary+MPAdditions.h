//
//  NSDictionary+MPAdditions.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

@interface NSDictionary (MPAdditions)

- (NSInteger)mp_integerForKey:(id)key;
- (NSInteger)mp_integerForKey:(id)key defaultValue:(NSInteger)defaultVal;
- (NSUInteger)mp_unsignedIntegerForKey:(id)key;
- (NSUInteger)mp_unsignedIntegerForKey:(id)key defaultValue:(NSUInteger)defaultVal;
- (double)mp_doubleForKey:(id)key;
- (double)mp_doubleForKey:(id)key defaultValue:(double)defaultVal;
- (NSString *)mp_stringForKey:(id)key;
- (NSString *)mp_stringForKey:(id)key defaultValue:(NSString *)defaultVal;
- (BOOL)mp_boolForKey:(id)key;
- (BOOL)mp_boolForKey:(id)key defaultValue:(BOOL)defaultVal;
- (float)mp_floatForKey:(id)key;
- (float)mp_floatForKey:(id)key defaultValue:(float)defaultVal;

@end
