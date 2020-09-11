//
//  MPError.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

extern NSString * const kNSErrorDomain;

typedef enum {
    MOPUBErrorUnknown = -1,
    MOPUBErrorNoInventory = 0,
    MOPUBErrorAdUnitWarmingUp = 1,
    MOPUBErrorNetworkTimedOut = 4,
    MOPUBErrorServerError = 8,
    MOPUBErrorAdapterNotFound = 16,
    MOPUBErrorAdapterInvalid = 17,
    MOPUBErrorAdapterHasNoInventory = 18,
    MOPUBErrorUnableToParseJSONAdResponse,
    MOPUBErrorUnexpectedNetworkResponse,
    MOPUBErrorHTTPResponseNot200,
    MOPUBErrorNoNetworkData,
    MOPUBErrorSDKNotInitialized,
    MOPUBErrorSDKInitializationInProgress,
    MOPUBErrorAdRequestTimedOut,
    MOPUBErrorNoRenderer,
    MOPUBErrorAdLoadAlreadyInProgress,
    MOPUBErrorInvalidCustomEventClass,
    MOPUBErrorJSONSerializationFailed,
    MOPUBErrorUnableToParseAdResponse,
    MOPUBErrorConsentDialogAlreadyShowing,
    MOPUBErrorNoConsentDialogLoaded,
    MOPUBErrorAdapterFailedToLoadAd,
    MOPUBErrorFullScreenAdAlreadyOnScreen,
    MOPUBErrorTooManyRequests,
    MOPUBErrorFrameWidthNotSetForFlexibleSize,
    MOPUBErrorFrameHeightNotSetForFlexibleSize,
    MOPUBErrorVideoPlayerFailedToPlay,
    MOPUBErrorNoHTMLToLoad,
    MOPUBErrorNoHTMLUrlToLoad,
} MOPUBErrorCode;

@interface NSError (MoPub)

+ (NSError *)errorWithCode:(MOPUBErrorCode)code;
+ (NSError *)errorWithCode:(MOPUBErrorCode)code localizedDescription:(NSString *)description;

@end

@interface NSError (Initialization)
+ (instancetype)sdkMinimumOsVersion:(int)osVersion;
+ (instancetype)sdkInitializationInProgress;
@end

@interface NSError (AdLifeCycle)
+ (instancetype)adAlreadyLoading;
+ (instancetype)customEventClass:(Class)customEventClass doesNotInheritFrom:(Class)baseClass;
+ (instancetype)networkResponseIsNotHTTP;
+ (instancetype)networkResponseContainedNoData;
+ (instancetype)adLoadFailedBecauseSdkNotInitialized;
+ (instancetype)serializationOfJson:(NSDictionary *)json failedWithError:(NSError *)serializationError;
+ (instancetype)adResponseFailedToParseWithError:(NSError *)serializationError;
+ (instancetype)adResponsesNotFound;
+ (instancetype)fullscreenAdAlreadyOnScreen;
+ (instancetype)frameWidthNotSetForFlexibleSize;
+ (instancetype)frameHeightNotSetForFlexibleSize;
@end

@interface NSError (Consent)
+ (instancetype)consentDialogAlreadyShowing;
+ (instancetype)noConsentDialogLoaded;
@end

@interface NSError (RateLimit)
+ (instancetype)tooManyRequests;
@end
