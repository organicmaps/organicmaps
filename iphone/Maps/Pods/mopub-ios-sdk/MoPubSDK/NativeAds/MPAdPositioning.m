//
//  MPAdPositioning.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPAdPositioning.h"

@interface MPAdPositioning ()

@property (nonatomic, strong) NSMutableOrderedSet *fixedPositions;

@end

@implementation MPAdPositioning

- (id)init
{
    self = [super init];
    if (self) {
        _fixedPositions = [[NSMutableOrderedSet alloc] init];
    }

    return self;
}


#pragma mark - <NSCopying>

- (id)copyWithZone:(NSZone *)zone
{
    MPAdPositioning *newPositioning = [[[self class] allocWithZone:zone] init];
    newPositioning.fixedPositions = [self.fixedPositions copyWithZone:zone];
    newPositioning.repeatingInterval = self.repeatingInterval;
    return newPositioning;
}

@end
