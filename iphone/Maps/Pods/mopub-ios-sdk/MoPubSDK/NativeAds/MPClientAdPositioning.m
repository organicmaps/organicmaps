//
//  MPClientAdPositioning.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPClientAdPositioning.h"
#import "MPLogging.h"

@implementation MPClientAdPositioning

+ (instancetype)positioning
{
    return [[self alloc] init];
}

- (void)addFixedIndexPath:(NSIndexPath *)indexPath
{
    [self.fixedPositions addObject:indexPath];
}

- (void)enableRepeatingPositionsWithInterval:(NSUInteger)interval
{
    if (interval > 1) {
        self.repeatingInterval = interval;
    } else {
        MPLogInfo(@"Repeating positions will not be enabled: the provided interval must be greater "
                  @"than 1.");
    }
}

@end
