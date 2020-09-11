//
//  MPError.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPError.h"

NSString * const kNSErrorDomain = @"com.mopub.iossdk";

@implementation NSError (MoPub)

+ (NSError *)errorWithCode:(MOPUBErrorCode)code {
    return [NSError errorWithCode:code localizedDescription:nil];
}

+ (NSError *)errorWithCode:(MOPUBErrorCode)code localizedDescription:(NSString *)description {
    NSDictionary * userInfo = nil;
    if (description != nil) {
        userInfo = @{ NSLocalizedDescriptionKey: description };
    }

    return [self errorWithDomain:kNSErrorDomain code:code userInfo:userInfo];
}

@end

@implementation NSError (Initialization)

+ (instancetype)sdkMinimumOsVersion:(int)osVersion {
    return [NSError errorWithCode:MOPUBErrorSDKNotInitialized localizedDescription:[NSString stringWithFormat:@"MoPub SDK requires iOS %d and up", osVersion]];
}

+ (instancetype)sdkInitializationInProgress {
    return [NSError errorWithCode:MOPUBErrorSDKInitializationInProgress localizedDescription:@"Attempted to initialize the SDK while a prior SDK initialization is in progress."];
}

@end

@implementation NSError (AdLifeCycle)

+ (instancetype)adAlreadyLoading {
    return [NSError errorWithCode:MOPUBErrorAdLoadAlreadyInProgress localizedDescription:@"An ad is already being loaded. Please wait for the previous load to finish."];
}

+ (instancetype)customEventClass:(Class)customEventClass doesNotInheritFrom:(Class)baseClass {
    NSString * description = [NSString stringWithFormat:@"%@ is an invalid custom event class because it does not extend %@", NSStringFromClass(customEventClass), NSStringFromClass(baseClass)];
    return [NSError errorWithCode:MOPUBErrorInvalidCustomEventClass localizedDescription:description];
}

+ (instancetype)networkResponseIsNotHTTP {
    return [NSError errorWithCode:MOPUBErrorUnexpectedNetworkResponse localizedDescription:@"Network response is not of type NSHTTPURLResponse"];
}

+ (instancetype)networkResponseContainedNoData {
    return [NSError errorWithCode:MOPUBErrorNoNetworkData localizedDescription:@"No data found in the NSHTTPURLResponse"];
}

+ (instancetype)adLoadFailedBecauseSdkNotInitialized {
    return [NSError errorWithCode:MOPUBErrorSDKNotInitialized localizedDescription:@"Ad prevented from loading. Error: Ad requested before initializing MoPub SDK. The MoPub SDK requires initializeSdkWithConfiguration:completion: to be called on MoPub.sharedInstance before attempting to load ads. Please update your integration."];
}

+ (instancetype)serializationOfJson:(NSDictionary *)json failedWithError:(NSError *)serializationError {
    NSString * errorMessage = [NSString stringWithFormat:@"Failed to generate a JSON string from:\n%@\nReason: %@", json, serializationError.localizedDescription];
    return [NSError errorWithCode:MOPUBErrorJSONSerializationFailed localizedDescription:errorMessage];
}

+ (instancetype)adResponseFailedToParseWithError:(NSError *)serializationError {
    NSString * errorMessage = [NSString stringWithFormat:@"Failed to parse ad response into JSON: %@", serializationError.localizedDescription];
    return [NSError errorWithCode:MOPUBErrorUnableToParseAdResponse localizedDescription:errorMessage];
}

+ (instancetype)adResponsesNotFound {
    return [NSError errorWithCode:MOPUBErrorUnableToParseJSONAdResponse localizedDescription:@"No ad responses"];
}

+ (instancetype)fullscreenAdAlreadyOnScreen {
    return [NSError errorWithCode:MOPUBErrorFullScreenAdAlreadyOnScreen localizedDescription:@"Cannot present a full screen ad that is already on-screen."];
}

+ (instancetype)frameWidthNotSetForFlexibleSize {
    return [NSError errorWithCode:MOPUBErrorFrameWidthNotSetForFlexibleSize localizedDescription:@"Cannot determine a size for flexible width because the frame width is not set."];
}

+ (instancetype)frameHeightNotSetForFlexibleSize {
    return [NSError errorWithCode:MOPUBErrorFrameHeightNotSetForFlexibleSize localizedDescription:@"Cannot determine a size for flexible height because the frame height is not set."];
}

@end

@implementation NSError (Consent)

+ (instancetype)consentDialogAlreadyShowing {
    return [NSError errorWithCode:MOPUBErrorConsentDialogAlreadyShowing localizedDescription:@"Consent dialog is already being presented modally."];
}

+ (instancetype)noConsentDialogLoaded {
    return [NSError errorWithCode:MOPUBErrorNoConsentDialogLoaded localizedDescription:@"Consent dialog has not been loaded."];
}

@end

@implementation NSError (RateLimit)
+ (instancetype)tooManyRequests {
    return [NSError errorWithCode:MOPUBErrorTooManyRequests localizedDescription:@"Could not perform ad request because too many requests have been sent to the server."];
}
@end
