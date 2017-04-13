//
//  MPVASTDurationOffset.m
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPVASTDurationOffset.h"
#import "MPVASTStringUtilities.h"

@implementation MPVASTDurationOffset

- (instancetype)initWithDictionary:(NSDictionary *)dictionary
{
    self = [super initWithDictionary:dictionary];
    if (self) {
        _offset = dictionary[@"offset"] ?: dictionary[@"skipoffset"];
        if (!_offset) {
            return nil;
        }

        BOOL isPercentage = [MPVASTStringUtilities stringRepresentsNonNegativePercentage:_offset];
        BOOL isDuration = [MPVASTStringUtilities stringRepresentsNonNegativeDuration:_offset];
        if (!isPercentage && !isDuration) {
            return nil;
        }

        _type = isDuration ? MPVASTDurationOffsetTypeAbsolute : MPVASTDurationOffsetTypePercentage;
    }
    return self;
}

- (NSTimeInterval)timeIntervalForVideoWithDuration:(NSTimeInterval)duration
{
    if (duration < 0) {
        return 0;
    }

    if (self.type == MPVASTDurationOffsetTypeAbsolute) {
        return [MPVASTStringUtilities timeIntervalFromString:self.offset];
    } else if (self.type == MPVASTDurationOffsetTypePercentage) {
        NSInteger percentage = [MPVASTStringUtilities percentageFromString:self.offset];
        return duration * percentage / 100.0f;
    } else {
        return 0;
    }
}

@end
