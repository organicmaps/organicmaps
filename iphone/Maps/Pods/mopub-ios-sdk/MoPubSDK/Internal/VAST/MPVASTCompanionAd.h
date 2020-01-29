//
//  MPVASTCompanionAd.h
//
//  Copyright 2018-2019 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MPVASTModel.h"

@class MPVASTResource;
@class MPVASTTrackingEvent;

@interface MPVASTCompanionAd : MPVASTModel

@property (nonatomic, strong, readonly) NSString *identifier; // optional attribute
@property (nonatomic, readonly) CGFloat width;
@property (nonatomic, readonly) CGFloat height;
@property (nonatomic, readonly) CGFloat assetHeight; // optional attribute
@property (nonatomic, readonly) CGFloat assetWidth; // optional attribute

@property (nonatomic, strong, readonly) NSURL *clickThroughURL;
@property (nonatomic, strong, readonly) NSArray<NSURL *> *clickTrackingURLs;

/** Per VAST 3.0 spec 2.3.3.7 Tracking Details:
 The <TrackingEvents> element may contain one or more <Tracking> elements, but the only event
 available for tracking under each Companion is the creativeView event. The creativeView event
 tracks whether the Companion creative was viewed. This view does not count as an impression
 because impressions are only counted for the Ad and the Companion is only one part of the Ad.
 */
@property (nonatomic, strong, readonly) NSArray<MPVASTTrackingEvent *> *creativeViewTrackers;

/** Per VAST 3.0 spec 2.3.3.2 Companion Resource Elements:
 Companion resource types are described below:
 • StaticResource: Describes non-html creative where an attribute for creativeType is used to
    identify the creative resource platform. The video player uses the creativeType information to
    determine how to display the resource:
    o Image/gif,image/jpeg,image/png:displayedusingtheHTMLtag<img>andthe resource URI as the src attribute.
    o Application/x-javascript:displayedusingtheHTMLtag<script>andtheresource URI as the src attribute.
 • IFrameResource: Describes a resource that is an HTML page that can be displayed within an
    Iframe on the publisher’s page.
 • HTMLResource: Describes a “snippet” of HTML code to be inserted directly within the publisher’s
    HTML page code.
 */
@property (nonatomic, strong, readonly) NSArray<MPVASTResource *> *HTMLResources;
@property (nonatomic, strong, readonly) NSArray<MPVASTResource *> *iframeResources;
@property (nonatomic, strong, readonly) NSArray<MPVASTResource *> *staticResources;

/**
 Return whether the companion ad has any of static, HTML, or iFrame resources.
 */
- (BOOL)hasResources;

@end
