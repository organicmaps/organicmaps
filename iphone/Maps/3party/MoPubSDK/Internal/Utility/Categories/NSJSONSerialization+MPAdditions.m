//
//  NSJSONSerialization+MPAdditions.m
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import "NSJSONSerialization+MPAdditions.h"

@interface NSMutableDictionary (RemoveNullObjects)

- (void)mp_removeNullsRecursively;

@end

@interface NSMutableArray (RemoveNullObjects)

- (void)mp_removeNullsRecursively;

@end

@implementation NSJSONSerialization (MPAdditions)

+ (id)mp_JSONObjectWithData:(NSData *)data options:(NSJSONReadingOptions)opt clearNullObjects:(BOOL)clearNulls error:(NSError **)error
{
    if (clearNulls) {
        opt |= NSJSONReadingMutableContainers;
    }

    id JSONObject = [NSJSONSerialization JSONObjectWithData:data options:opt error:error];

    if (error || !clearNulls) {
        return JSONObject;
    }

    [JSONObject mp_removeNullsRecursively];

    return JSONObject;
}

@end

@implementation NSMutableDictionary (RemovingNulls)

-(void)mp_removeNullsRecursively
{
    // First, filter out directly stored nulls
    NSMutableArray *nullKeys = [NSMutableArray array];
    NSMutableArray *arrayKeys = [NSMutableArray array];
    NSMutableArray *dictionaryKeys = [NSMutableArray array];

    [self enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL *stop) {
         if ([obj isEqual:[NSNull null]]) {
             [nullKeys addObject:key];
         } else if ([obj isKindOfClass:[NSDictionary  class]]) {
             [dictionaryKeys addObject:key];
         } else if ([obj isKindOfClass:[NSArray class]]) {
             [arrayKeys addObject:key];
         }
     }];

    // Remove all the nulls
    [self removeObjectsForKeys:nullKeys];

    // Cascade down the dictionaries
    for (id dictionaryKey in dictionaryKeys) {
        NSMutableDictionary *dictionary = [self objectForKey:dictionaryKey];
        [dictionary mp_removeNullsRecursively];
    }

    // Recursively remove nulls from arrays
    for (id arrayKey in arrayKeys) {
        NSMutableArray *array = [self objectForKey:arrayKey];
        [array mp_removeNullsRecursively];
    }
}

@end

@implementation NSMutableArray (RemovingNulls)

-(void)mp_removeNullsRecursively
{
    [self removeObjectIdenticalTo:[NSNull null]];

    for (id object in self) {
        if ([object respondsToSelector:@selector(mp_removeNullsRecursively)]) {
            [(NSMutableDictionary *)object mp_removeNullsRecursively];
        }
    }
}

@end
