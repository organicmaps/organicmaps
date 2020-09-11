//
//  MPVASTLinearAd.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPVASTModel.h"

@class MPVASTDurationOffset;
@class MPVASTIndustryIcon;
@class MPVASTMediaFile;
@class MPVASTTrackingEvent;

@interface MPVASTLinearAd : MPVASTModel

@property (nonatomic, copy, readonly) NSURL *clickThroughURL;
@property (nonatomic, readonly) NSArray<NSURL *> *clickTrackingURLs;
@property (nonatomic, readonly) NSArray<NSURL *> *customClickURLs;
@property (nonatomic, readonly) NSTimeInterval duration;
@property (nonatomic, readonly) NSArray<MPVASTIndustryIcon *> *industryIcons;
@property (nonatomic, readonly) NSArray<MPVASTMediaFile *> *mediaFiles;
@property (nonatomic, readonly) MPVASTDurationOffset *skipOffset;
@property (nonatomic, readonly) NSDictionary<NSString *, NSArray<MPVASTTrackingEvent *> *> *trackingEvents;

@end
