//
//  MPVideoConfig.h
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPVASTResponse.h"

@interface MPVideoConfig : NSObject

@property (nonatomic, readonly) NSURL *mediaURL;
@property (nonatomic, readonly) NSURL *clickThroughURL;
@property (nonatomic, readonly) NSArray *clickTrackingURLs;
@property (nonatomic, readonly) NSArray *errorURLs;
@property (nonatomic, readonly) NSArray *impressionURLs;

/** @name Tracking Events */

@property (nonatomic, readonly) NSArray *creativeViewTrackers;
@property (nonatomic, readonly) NSArray *startTrackers;
@property (nonatomic, readonly) NSArray *firstQuartileTrackers;
@property (nonatomic, readonly) NSArray *midpointTrackers;
@property (nonatomic, readonly) NSArray *thirdQuartileTrackers;
@property (nonatomic, readonly) NSArray *completionTrackers;
@property (nonatomic, readonly) NSArray *muteTrackers;
@property (nonatomic, readonly) NSArray *unmuteTrackers;
@property (nonatomic, readonly) NSArray *pauseTrackers;
@property (nonatomic, readonly) NSArray *rewindTrackers;
@property (nonatomic, readonly) NSArray *resumeTrackers;
@property (nonatomic, readonly) NSArray *fullscreenTrackers;
@property (nonatomic, readonly) NSArray *exitFullscreenTrackers;
@property (nonatomic, readonly) NSArray *expandTrackers;
@property (nonatomic, readonly) NSArray *collapseTrackers;
@property (nonatomic, readonly) NSArray *acceptInvitationLinearTrackers;
@property (nonatomic, readonly) NSArray *closeLinearTrackers;
@property (nonatomic, readonly) NSArray *skipTrackers;
@property (nonatomic, readonly) NSArray *otherProgressTrackers;

/** @name Viewability */

@property (nonatomic, readonly) NSTimeInterval minimumViewabilityTimeInterval;
@property (nonatomic, readonly) double minimumFractionOfVideoVisible;
@property (nonatomic, readonly) NSURL *viewabilityTrackingURL;

- (instancetype)initWithVASTResponse:(MPVASTResponse *)response additionalTrackers:(NSDictionary *)additionalTrackers;

@end
