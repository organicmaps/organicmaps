//
//  MPVASTIndustryIcon.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MPVASTDurationOffset.h"
#import "MPVASTModel.h"
#import "MPVASTResource.h"

@interface MPVASTIndustryIcon : MPVASTModel

@property (nonatomic, copy, readonly) NSString *program;
@property (nonatomic, readonly) CGFloat height;
@property (nonatomic, readonly) CGFloat width;
@property (nonatomic, copy, readonly) NSString *xPosition;
@property (nonatomic, copy, readonly) NSString *yPosition;

@property (nonatomic, copy, readonly) NSString *apiFramework;
@property (nonatomic, readonly) NSTimeInterval duration;
@property (nonatomic, readonly) MPVASTDurationOffset *offset;

@property (nonatomic, copy, readonly) NSURL *clickThroughURL;
@property (nonatomic, readonly) NSArray<NSURL *> *clickTrackingURLs;
@property (nonatomic, readonly) NSArray<NSURL *> *viewTrackingURLs;

/**
 Return best @c MPVASTResource that should be displayed. Per VAST specification
 (https://developers.mopub.com/dsps/ad-formats/video/):
    We will prioritize processing companion banners in the following order once we’ve picked the
    best size: Static, HTML, iframe." Here we pick the "best size" that has the number of pixels
    closest to the ad container.

 Side effect: The @c type of the returned @c MPVASTResource is determined and assigned.
 */
- (MPVASTResource *)resourceToDisplay;

@end
