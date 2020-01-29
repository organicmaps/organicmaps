//
//  MPViewabilityAdapter.h
//
//  Copyright 2018-2019 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>

@protocol MPViewabilityAdapter <NSObject>
@property (nonatomic, readonly) BOOL isTracking;

- (instancetype)initWithAdView:(UIView *)webView isVideo:(BOOL)isVideo startTrackingImmediately:(BOOL)startTracking;
- (void)startTracking;
- (void)stopTracking;
- (void)registerFriendlyObstructionView:(UIView *)view;

@end
