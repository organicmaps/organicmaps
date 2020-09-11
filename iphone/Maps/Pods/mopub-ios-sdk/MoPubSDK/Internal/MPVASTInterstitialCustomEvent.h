//
//  MPVASTInterstitialCustomEvent.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPInterstitialCustomEvent.h"
#import "MPPrivateInterstitialCustomEventDelegate.h"
#import "MPPrivateRewardedVideoCustomEventDelegate.h"
#import "MPRewardedVideoCustomEvent.h"

NS_ASSUME_NONNULL_BEGIN

@interface MPVASTInterstitialCustomEvent: MPInterstitialCustomEvent

/**
 The actual delegate might only conform to one of the delegate protocol.
 */
@property (nonatomic, weak) id<MPPrivateInterstitialCustomEventDelegate, MPPrivateRewardedVideoCustomEventDelegate> delegate;

@end

@interface MPVASTInterstitialCustomEvent (MPRewardedVideoCustomEvent) <MPRewardedVideoCustomEvent>
@end

NS_ASSUME_NONNULL_END
