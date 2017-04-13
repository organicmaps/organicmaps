//
//  MPVASTLinearAd.h
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPVASTModel.h"

@class MPVASTDurationOffset;
@class MPVASTMediaFile;

@interface MPVASTLinearAd : MPVASTModel

@property (nonatomic, copy, readonly) NSURL *clickThroughURL;
@property (nonatomic, readonly) NSArray *clickTrackingURLs;
@property (nonatomic, readonly) NSArray *customClickURLs;
@property (nonatomic, readonly) NSTimeInterval duration;
@property (nonatomic, readonly) NSArray *industryIcons;
@property (nonatomic, readonly) NSArray *mediaFiles;
@property (nonatomic, readonly) MPVASTDurationOffset *skipOffset;
@property (nonatomic, readonly) NSDictionary *trackingEvents;

@end

@interface MPVASTLinearAd (Media)
@property (nonatomic, readonly) MPVASTMediaFile *highestBitrateMediaFile;

@end
