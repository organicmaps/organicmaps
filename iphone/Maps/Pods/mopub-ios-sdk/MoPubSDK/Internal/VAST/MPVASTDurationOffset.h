//
//  MPVASTDurationOffset.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
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
