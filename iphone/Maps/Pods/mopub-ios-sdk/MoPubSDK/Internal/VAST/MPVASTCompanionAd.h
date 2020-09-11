//
//  MPVASTCompanionAd.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MPVASTModel.h"
#import "MPVASTResource.h"
#import "MPVASTTrackingEvent.h"

@class MPVASTCompanionAd;

@protocol MPVASTCompanionAdProvider <NSObject>

- (BOOL)hasCompanionAd;

/**
 Return that best companion ad that fits into the provided container size.
 */
- (MPVASTCompanionAd *)companionAdForContainerSize:(CGSize)containerSize;

@end

@interface MPVASTCompanionAd : MPVASTModel

@property (nonatomic, strong, readonly) NSString *identifier; // optional attribute
@property (nonatomic, readonly) CGFloat width; // point width
@property (nonatomic, readonly) CGFloat height; // point height
@property (nonatomic, readonly) CGFloat assetHeight; // optional attribute
@property (nonatomic, readonly) CGFloat assetWidth; // optional attribute
@property (nonatomic, copy, readonly) NSString *apiFramework; // optional attribute

@property (nonatomic, strong, readonly) NSURL *clickThroughURL;
@property (nonatomic, strong, readonly) NSArray<NSURL *> *clickTrackingURLs;

/** Per VAST 3.0 spec 2.3.3.7 Tracking Details:
 The <TrackingEvents> element may contain one or more <Tracking> elements, but the only event
 available for tracking under each Companion is the creativeView event. The creativeView event
 tracks whether the Companion creative was viewed. This view does not count as an impression
 because impressions are only counted for the Ad and the Companion is only one part of the Ad.
 */
@property (nonatomic, strong, readonly) NSArray<MPVASTTrackingEvent *> *creativeViewTrackers;

/**
 Per VAST 3.0 specification, section 2.3.3.5 Companion Attributes, `width` and `height` are required
 attributes for a companion ad. However, if `width` or `height` does not exist or being less than 1
 for any reason, the companion ad view might appear to be empty and causes issue. To ensure the
 bounds of the companion ad view represents at least one pixel, the returned safe ad boudns as
 `CGRect` guarantees the value of `width` and `height` to be at least 1.
 */
- (CGRect)safeAdViewBounds;

/**
 Return the best @c MPVASTResource that should be displayed. Per VAST specification
 (https://developers.mopub.com/dsps/ad-formats/video/):
    We will prioritize processing companion banners in the following order once we’ve picked the
    best size: Static, HTML, iframe." Here we pick the "best size" that has the number of pixels
    closest to the ad container.

 Note: The @c type of the returned @c MPVASTResource is determined and assigned.
 */
- (MPVASTResource *)resourceToDisplay;

/**
 Return best @c MPVASTCompanionAd that should be displayed.
 */
+ (MPVASTCompanionAd *)bestCompanionAdForCandidates:(NSArray<MPVASTCompanionAd *> *)candidates
                                      containerSize:(CGSize)containerSize;

@end
