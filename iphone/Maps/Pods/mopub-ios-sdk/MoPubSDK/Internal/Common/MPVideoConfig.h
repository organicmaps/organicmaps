//
//  MPVideoConfig.h
//
//  Copyright 2018-2019 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPVASTResponse.h"

@interface MPVideoConfig : NSObject

/**
 Ad response typically contains multiple video files of different resolutions and bit-rates, and the
 best one is picked when the ad is loaded (not when receiving the ad response).
 */
@property (nonatomic, readonly) NSArray<MPVASTMediaFile *> *mediaFiles;

@property (nonatomic, readonly) NSURL *clickThroughURL;

- (instancetype)initWithVASTResponse:(MPVASTResponse *)response additionalTrackers:(NSDictionary *)additionalTrackers;

/**
 Take a @c MPVideoEvent string for the key, and return an array of @c MPVASTTrackingEvent.
 */
- (NSArray<MPVASTTrackingEvent *> *)trackingEventsForKey:(MPVideoEvent)key;

@end
