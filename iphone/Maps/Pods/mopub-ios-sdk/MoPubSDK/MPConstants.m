//
//  MPConstants.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPConstants.h"

CGFloat const kMPFlexibleAdSize = -1.0f;

CGSize const MOPUB_BANNER_SIZE = { .width = 320.0f, .height = 50.0f };
CGSize const MOPUB_MEDIUM_RECT_SIZE = { .width = 300.0f, .height = 250.0f };
CGSize const MOPUB_LEADERBOARD_SIZE = { .width = 728.0f, .height = 90.0f };
CGSize const MOPUB_WIDE_SKYSCRAPER_SIZE = { .width = 160.0f, .height = 600.0f };

CGSize const kMPPresetMaxAdSizeMatchFrame = { .width = kMPFlexibleAdSize, .height = kMPFlexibleAdSize };
CGSize const kMPPresetMaxAdSize50Height   = { .width = kMPFlexibleAdSize, .height = 50.0f };
CGSize const kMPPresetMaxAdSize90Height   = { .width = kMPFlexibleAdSize, .height = 90.0f };
CGSize const kMPPresetMaxAdSize250Height  = { .width = kMPFlexibleAdSize, .height = 250.0f };
CGSize const kMPPresetMaxAdSize280Height  = { .width = kMPFlexibleAdSize, .height = 280.0f };

@implementation MPConstants

+ (NSTimeInterval)adsExpirationInterval {
    return MOPUB_ADS_EXPIRATION_INTERVAL;
}

@end
