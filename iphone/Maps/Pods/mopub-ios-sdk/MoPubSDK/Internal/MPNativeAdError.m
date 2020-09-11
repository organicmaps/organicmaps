//
//  MPNativeAdError.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPNativeAdError.h"

NSString * const MoPubNativeAdsSDKDomain = @"com.mopub.nativeads";

NSError *MPNativeAdNSErrorForInvalidAdServerResponse(NSString *reason) {
    if (reason.length == 0) {
        reason = @"Invalid ad server response";
    }

    return [NSError errorWithDomain:MoPubNativeAdsSDKDomain code:MPNativeAdErrorInvalidServerResponse userInfo:@{NSLocalizedDescriptionKey : [reason copy]}];
}

NSError *MPNativeAdNSErrorForAdUnitWarmingUp() {
    return [NSError errorWithDomain:MoPubNativeAdsSDKDomain code:MPNativeAdErrorAdUnitWarmingUp userInfo:@{NSLocalizedDescriptionKey : @"Ad unit is warming up"}];
}

NSError *MPNativeAdNSErrorForNoInventory() {
    return [NSError errorWithDomain:MoPubNativeAdsSDKDomain code:MPNativeAdErrorNoInventory userInfo:@{NSLocalizedDescriptionKey : @"Ad server returned no inventory"}];
}

NSError *MPNativeAdNSErrorForNetworkConnectionError() {
    return [NSError errorWithDomain:MoPubNativeAdsSDKDomain code:MPNativeAdErrorHTTPError userInfo:@{NSLocalizedDescriptionKey : @"Connection error"}];
}

NSError *MPNativeAdNSErrorForInvalidImageURL() {
    return MPNativeAdNSErrorForInvalidAdServerResponse(@"Invalid image URL");
}

NSError *MPNativeAdNSErrorForImageDownloadFailure() {
    return [NSError errorWithDomain:MoPubNativeAdsSDKDomain code:MPNativeAdErrorImageDownloadFailed userInfo:@{NSLocalizedDescriptionKey : @"Failed to download images"}];
}

NSError *MPNativeAdNSErrorForVASTParsingFailure() {
    return [NSError errorWithDomain:MoPubNativeAdsSDKDomain code:MPNativeAdErrorVASTParsingFailed userInfo:@{NSLocalizedDescriptionKey : @"Failed to parse VAST tag"}];
}

NSError *MPNativeAdNSErrorForVideoConfigInvalid() {
    return [NSError errorWithDomain:MoPubNativeAdsSDKDomain code:MPNativeAdErrorVideoConfigInvalid userInfo:@{NSLocalizedDescriptionKey : @"Native Video Config Values in Adserver response are invalid"}];
}

NSError *MPNativeAdNSErrorForContentDisplayErrorMissingRootController() {
    return [NSError errorWithDomain:MoPubNativeAdsSDKDomain code:MPNativeAdErrorContentDisplayError userInfo:@{NSLocalizedDescriptionKey : @"Cannot display content without a root view controller"}];
}

NSError *MPNativeAdNSErrorForContentDisplayErrorInvalidURL() {
    return [NSError errorWithDomain:MoPubNativeAdsSDKDomain code:MPNativeAdErrorContentDisplayError userInfo:@{NSLocalizedDescriptionKey : @"Cannot display content without a valid URL"}];
}

NSError *MPNativeAdNSErrorForRenderValueTypeError() {
    return [NSError errorWithDomain:MoPubNativeAdsSDKDomain code:MPNativeAdErrorRenderError userInfo:@{NSLocalizedDescriptionKey : @"Native ad property was an incorrect data type"}];
}
