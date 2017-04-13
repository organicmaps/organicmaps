//
//  MPVASTDurationOffset.h
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPVASTModel.h"

typedef NS_ENUM(NSUInteger, MPVASTDurationOffsetType) {
    MPVASTDurationOffsetTypeAbsolute,
    MPVASTDurationOffsetTypePercentage,
};

@interface MPVASTDurationOffset : MPVASTModel

@property (nonatomic, readonly) MPVASTDurationOffsetType type;
@property (nonatomic, copy, readonly) NSString *offset;

- (NSTimeInterval)timeIntervalForVideoWithDuration:(NSTimeInterval)duration;

@end
