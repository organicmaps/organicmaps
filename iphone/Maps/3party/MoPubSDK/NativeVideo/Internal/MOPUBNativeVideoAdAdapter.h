//
//  MOPUBNativeVideoAdAdapter.h
//  Copyright (c) 2015 MoPub. All rights reserved.
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
