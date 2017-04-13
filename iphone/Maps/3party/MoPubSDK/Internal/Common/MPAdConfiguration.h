//
//  MPAdConfiguration.h
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPGlobal.h"

@class MPRewardedVideoReward;

enum {
    MPAdTypeUnknown = -1,
    MPAdTypeBanner = 0,
    MPAdTypeInterstitial = 1
};
typedef NSUInteger MPAdType;

extern NSString * const kAdTypeHeaderKey;
extern NSString * const kAdUnitWarmingUpHeaderKey;
extern NSString * const kClickthroughHeaderKey;
extern NSString * const kCreativeIdHeaderKey;
extern NSString * const kCustomSelectorHeaderKey;
extern NSString * const kCustomEventClassNameHeaderKey;
extern NSString * const kCustomEventClassDataHeaderKey;
extern NSString * const kFailUrlHeaderKey;
extern NSString * const kHeightHeaderKey;
extern NSString * const kImpressionTrackerHeaderKey;
extern NSString * const kInterceptLinksHeaderKey;
extern NSString * const kLaunchpageHeaderKey;
extern NSString * const kNativeSDKParametersHeaderKey;
extern NSString * const kNetworkTypeHeaderKey;
extern NSString * const kRefreshTimeHeaderKey;
extern NSString * const kAdTimeoutHeaderKey;
extern NSString * const kScrollableHeaderKey;
extern NSString * const kWidthHeaderKey;
extern NSString * const kDspCreativeIdKey;
extern NSString * const kPrecacheRequiredKey;
extern NSString * const kIsVastVideoPlayerKey;
extern NSString * const kRewardedVideoCurrencyNameHeaderKey;
extern NSString * const kRewardedVideoCurrencyAmountHeaderKey;
extern NSString * const kRewardedVideoCompletionUrlHeaderKey;
extern NSString * const kRewardedCurrenciesHeaderKey;
extern NSString * const kRewardedPlayableDurationHeaderKey;
extern NSString * const kRewardedPlayableRewardOnClickHeaderKey;

extern NSString * const kInterstitialAdTypeHeaderKey;
extern NSString * const kOrientationTypeHeaderKey;

extern NSString * const kAdTypeHtml;
extern NSString * const kAdTypeInterstitial;
extern NSString * const kAdTypeMraid;
extern NSString * const kAdTypeClear;
extern NSString * const kAdTypeNative;
extern NSString * const kAdTypeNativeVideo;

@interface MPAdConfiguration : NSObject

@property (nonatomic, assign) MPAdType adType;
@property (nonatomic, assign) BOOL adUnitWarmingUp;
@property (nonatomic, copy) NSString *networkType;
@property (nonatomic, assign) CGSize preferredSize;
@property (nonatomic, strong) NSURL *clickTrackingURL;
@property (nonatomic, strong) NSURL *impressionTrackingURL;
@property (nonatomic, strong) NSURL *failoverURL;
@property (nonatomic, strong) NSURL *interceptURLPrefix;
@property (nonatomic, assign) BOOL shouldInterceptLinks;
@property (nonatomic, assign) BOOL scrollable;
@property (nonatomic, assign) NSTimeInterval refreshInterval;
@property (nonatomic, assign) NSTimeInterval adTimeoutInterval;
@property (nonatomic, copy) NSData *adResponseData;
@property (nonatomic, strong) NSDictionary *nativeSDKParameters;
@property (nonatomic, copy) NSString *customSelectorName;
@property (nonatomic, assign) Class customEventClass;
@property (nonatomic, strong) NSDictionary *customEventClassData;
@property (nonatomic, assign) MPInterstitialOrientationType orientationType;
@property (nonatomic, copy) NSString *dspCreativeId;
@property (nonatomic, assign) BOOL precacheRequired;
@property (nonatomic, assign) BOOL isVastVideoPlayer;
@property (nonatomic, strong) NSDate *creationTimestamp;
@property (nonatomic, copy) NSString *creativeId;
@property (nonatomic, copy) NSString *headerAdType;
@property (nonatomic, assign) NSInteger nativeVideoPlayVisiblePercent;
@property (nonatomic, assign) NSInteger nativeVideoPauseVisiblePercent;
@property (nonatomic, assign) NSInteger nativeVideoImpressionMinVisiblePercent;
@property (nonatomic, assign) NSTimeInterval nativeVideoImpressionVisible;
@property (nonatomic, assign) NSTimeInterval nativeVideoMaxBufferingTime;
@property (nonatomic) NSDictionary *nativeVideoTrackers;
@property (nonatomic, readonly) NSArray *availableRewards;
@property (nonatomic, strong) MPRewardedVideoReward *selectedReward;
@property (nonatomic, copy) NSString *rewardedVideoCompletionUrl;
@property (nonatomic, assign) NSTimeInterval rewardedPlayableDuration;
@property (nonatomic, assign) BOOL rewardedPlayableShouldRewardOnClick;
//TODO: Remove `forceUIWebView` once WKWebView is proven
@property (nonatomic, assign) BOOL forceUIWebView;

- (id)initWithHeaders:(NSDictionary *)headers data:(NSData *)data;

- (BOOL)hasPreferredSize;
- (NSString *)adResponseHTMLString;
- (NSString *)clickDetectionURLPrefix;

@end
