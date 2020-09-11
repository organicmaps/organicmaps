//
//  MPRewardedVideoError.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

typedef enum {
    MPRewardedVideoAdErrorUnknown = -1,

    MPRewardedVideoAdErrorTimeout = -1000,
    MPRewardedVideoAdErrorAdUnitWarmingUp = -1001,
    MPRewardedVideoAdErrorNoAdsAvailable = -1100,
    MPRewardedVideoAdErrorInvalidCustomEvent = -1200,
    MPRewardedVideoAdErrorMismatchingAdTypes = -1300,
    MPRewardedVideoAdErrorAdAlreadyPlayed = -1400,
    MPRewardedVideoAdErrorNoAdReady = -1401,
    MPRewardedVideoAdErrorInvalidAdUnitID = -1500,
    MPRewardedVideoAdErrorInvalidReward = -1600,
    MPRewardedVideoAdErrorNoRewardSelected = -1601,
} MPRewardedVideoErrorCode;

extern NSString * const MoPubRewardedVideoAdsSDKDomain;
