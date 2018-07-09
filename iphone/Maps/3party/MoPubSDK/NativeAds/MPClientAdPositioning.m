//
//  MPClientAdPositioning.m
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
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
        MPLogWarn(@"Repeating positions will not be enabled: the provided interval must be greater "
                  @"than 1.");
    }
}

@end
