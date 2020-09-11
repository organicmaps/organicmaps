//
//  NSMutableArray+MPAdditions.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "NSMutableArray+MPAdditions.h"

@implementation NSMutableArray (MPAdditions)

- (id)removeFirst {
    if (self.count == 0) {
        return nil;
    }
    id firstObject = self.firstObject;
    [self removeObjectAtIndex:0];
    return firstObject;
}

@end
