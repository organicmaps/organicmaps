//
//  MPConstants.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>

#define MP_HAS_NATIVE_PACKAGE       1

#define DEFAULT_PUB_ID              @"agltb3B1Yi1pbmNyDAsSBFNpdGUYkaoMDA"
#define MP_SERVER_VERSION           @"8"
#define MP_REWARDED_API_VERSION     @"1"
#define MP_BUNDLE_IDENTIFIER        @"com.mopub.mopub"
#define MP_SDK_VERSION              @"5.12.0"

// Sizing constants.
extern CGSize const MOPUB_BANNER_SIZE __attribute__((deprecated("Use kMPPresetMaxAdSizeMatchFrame, kMPPresetMaxAdSizeMatchFrame, kMPPresetMaxAdSize50Height, kMPPresetMaxAdSizeBanner90Height, kMPPresetMaxAdSize90Height, kMPPresetMaxAdSize250Height, kMPPresetMaxAdSize280Height, or a custom maximum desired ad area instead")));
extern CGSize const MOPUB_MEDIUM_RECT_SIZE __attribute__((deprecated("Use kMPPresetMaxAdSizeMatchFrame, kMPPresetMaxAdSizeMatchFrame, kMPPresetMaxAdSize50Height, kMPPresetMaxAdSizeBanner90Height, kMPPresetMaxAdSize90Height, kMPPresetMaxAdSize250Height, kMPPresetMaxAdSize280Height, or a custom maximum desired ad area instead")));
extern CGSize const MOPUB_LEADERBOARD_SIZE __attribute__((deprecated("Use kMPPresetMaxAdSizeMatchFrame, kMPPresetMaxAdSizeMatchFrame, kMPPresetMaxAdSize50Height, kMPPresetMaxAdSizeBanner90Height, kMPPresetMaxAdSize90Height, kMPPresetMaxAdSize250Height, kMPPresetMaxAdSize280Height, or a custom maximum desired ad area instead")));
extern CGSize const MOPUB_WIDE_SKYSCRAPER_SIZE __attribute__((deprecated("Use kMPPresetMaxAdSizeMatchFrame, kMPPresetMaxAdSizeMatchFrame, kMPPresetMaxAdSize50Height, kMPPresetMaxAdSizeBanner90Height, kMPPresetMaxAdSize90Height, kMPPresetMaxAdSize250Height, kMPPresetMaxAdSize280Height, or a custom maximum desired ad area instead")));

// Convenience presets for requesting maximum ad sizes.
// Custom maximum ad sizes can be requested using an explicit `CGSize`.
extern CGSize const kMPPresetMaxAdSizeMatchFrame;
extern CGSize const kMPPresetMaxAdSize50Height;
extern CGSize const kMPPresetMaxAdSize90Height;
extern CGSize const kMPPresetMaxAdSize250Height;
extern CGSize const kMPPresetMaxAdSize280Height;

/**
 Constant denoting that the dimension should be flexible with respect to
 the container.
 */
extern CGFloat const kMPFlexibleAdSize;

// Miscellaneous constants.
#define MINIMUM_REFRESH_INTERVAL            10.0
#define DEFAULT_BANNER_REFRESH_INTERVAL     60    // seconds
#define BANNER_TIMEOUT_INTERVAL             10    // seconds
#define INTERSTITIAL_TIMEOUT_INTERVAL       30    // seconds
#define REWARDED_VIDEO_TIMEOUT_INTERVAL     30    // seconds
#define NATIVE_TIMEOUT_INTERVAL             10    // seconds
#define MOPUB_ADS_EXPIRATION_INTERVAL       14400 // 4 hours converted to seconds

// Feature Flags
#define SESSION_TRACKING_ENABLED            1

@interface MPConstants : NSObject

+ (NSTimeInterval)adsExpirationInterval;

@end
