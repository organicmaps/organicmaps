//
//  FacebookAdapterConfiguration.m
//  MoPubSDK
//
//  Copyright © 2017 MoPub. All rights reserved.
//

#import "FacebookAdapterConfiguration.h"
#import <FBAudienceNetwork/FBAudienceNetwork.h>
#import "FacebookAdapterConfiguration.h"

#if __has_include("MoPub.h")
#import "MPLogging.h"
#import "MPConstants.h"
#endif

#define FACEBOOK_ADAPTER_VERSION             @"5.9.0.0"
#define MOPUB_NETWORK_NAME                   @"facebook"

static NSString * const kFacebookPlacementIDs = @"placement_ids";
static Boolean *sIsNativeBanner = nil;

@interface FacebookAdapterConfiguration()
@property (class, nonatomic, readwrite) Boolean * isNativeBanner;
@end

@implementation FacebookAdapterConfiguration

#pragma mark - Test Mode

+ (BOOL)isTestMode {
    return FBAdSettings.isTestMode;
}

+ (void)setIsTestMode:(BOOL)isTestMode {
    // Transition to test mode by adding the current device hash as a test device.
    if (isTestMode && !FBAdSettings.isTestMode) {
        [FBAdSettings addTestDevice:FBAdSettings.testDeviceHash];
    }
    // Transition out of test mode by removing the current device hash.
    else if (!isTestMode && FBAdSettings.isTestMode) {
        [FBAdSettings clearTestDevice:FBAdSettings.testDeviceHash];
    }
}

#pragma mark - MPAdapterConfiguration

- (NSString *)adapterVersion {
    return FACEBOOK_ADAPTER_VERSION;
}

- (NSString *)biddingToken {
    return [FBAdSettings bidderToken];
}

- (NSString *)moPubNetworkName {
    return MOPUB_NETWORK_NAME;
}

- (NSString *)networkSdkVersion {
    return FB_AD_SDK_VERSION;
}

+ (NSString*)mediationString {
    return [NSString stringWithFormat:@"MOPUB_%@:%@", MP_SDK_VERSION, FACEBOOK_ADAPTER_VERSION];
}

+ (Boolean *)isNativeBanner
{
    return sIsNativeBanner;
}

+ (void)setIsNativeBanner:(Boolean *)pref
{
    sIsNativeBanner = pref;
}

- (void)initializeNetworkWithConfiguration:(NSDictionary<NSString *, id> *)configuration
                                  complete:(void(^)(NSError *))complete {
    FBAdInitSettings *fbSettings = [[FBAdInitSettings alloc]
                                    initWithPlacementIDs:configuration[kFacebookPlacementIDs]
                                    mediationService:[FacebookAdapterConfiguration mediationString]];
    [FBAudienceNetworkAds
     initializeWithSettings:fbSettings
     completionHandler:^(FBAdInitResults *results) {
         if (results.success) {
             MPLogDebug(@"Initialized Facebook Audience Network");
             complete(nil);
         } else {
             NSError *error =
             [NSError errorWithDomain:@"FacebookAdapterConfiguration"
                                 code:0
                             userInfo:@{NSLocalizedDescriptionKey : results.message}];
             complete(error);
         }
     }];
    
    if (configuration != nil && [configuration count] > 0) {
        FacebookAdapterConfiguration.isNativeBanner = [[configuration objectForKey:@"native_banner"] boolValue];
    }

}

@end
