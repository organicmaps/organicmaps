//
//  MOPUBNativeVideoAdAdapter.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPNativeAdAdapter.h"

@class MPAdConfiguration;

@interface MOPUBNativeVideoAdAdapter : NSObject <MPNativeAdAdapter>

@property (nonatomic, weak) id<MPNativeAdAdapterDelegate> delegate;
@property (nonatomic, readonly) NSArray *impressionTrackerURLs;
@property (nonatomic, readonly) NSArray *clickTrackerURLs;
@property (nonatomic) MPAdConfiguration *adConfiguration;

- (instancetype)initWithAdProperties:(NSMutableDictionary *)properties;

- (void)handleVideoViewImpression;
- (void)handleVideoViewClick;
- (void)handleVideoHasProgressedToTime:(NSTimeInterval)playbackTime;

@end
