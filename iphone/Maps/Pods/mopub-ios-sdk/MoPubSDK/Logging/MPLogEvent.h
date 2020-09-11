//
//  MPLogEvent.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPConsentStatus.h"
#import "MPBLogLevel.h"

@protocol MPAdapterConfiguration;
@class MPRewardedVideoReward;
@class MPURLRequest;

NS_ASSUME_NONNULL_BEGIN

/**
 Logging event used to construct pre-formatted messages.
 */
@interface MPLogEvent : NSObject
/**
 Message to be logged.
 */
@property (nonatomic, copy, readonly) NSString * message;

/**
 Level at which the message should be logged.
 */
@property (nonatomic, assign, readonly) MPBLogLevel logLevel;

/**
 Default initialization is disallowed.
 */
- (instancetype)init NS_UNAVAILABLE;

/**
 Initializes the log event with the specified message to be logged at Debug level.
 @param message Message to log
 @return Log event
 */
- (instancetype)initWithMessage:(NSString *)message;

/**
 Initializes the log event with the specified message to be logged at the desired
 log level.
 @param message Message to log
 @param level Level at which the message should be logged
 @return Log event
 */
- (instancetype)initWithMessage:(NSString *)message level:(MPBLogLevel)level NS_DESIGNATED_INITIALIZER;

/**
 Initializes a generic error log event with optional message. The message and error
 will be logged at Debug level.
 @param error Error to log
 @param message Optional message that will prefix the error message.
 @return Log event
 */
+ (instancetype)error:(NSError *)error message:(NSString * _Nullable)message;

/**
 Initializes the log event with the specified message to be logged at the desired
 log level.
 @param message Message to log
 @param level Level at which the message should be logged
 @return Log event
 */
+ (instancetype)eventWithMessage:(NSString *)message level:(MPBLogLevel)level;

@end

@interface MPLogEvent (AdLifeCycle)
+ (instancetype)adRequestedWithRequest:(MPURLRequest *)request;
+ (instancetype)adRequestReceivedResponse:(NSDictionary *)response;
+ (instancetype)adLoadAttempt;
+ (instancetype)adShowAttempt;
+ (instancetype)adShowSuccess;
+ (instancetype)adShowFailedWithError:(NSError *)error;
+ (instancetype)adDidLoad;
+ (instancetype)adFailedToLoadWithError:(NSError *)error;
+ (instancetype)adExpiredWithTimeInterval:(NSTimeInterval)expirationInterval;
+ (instancetype)adWillPresentModal;
+ (instancetype)adDidDismissModal;
+ (instancetype)adTapped;
+ (instancetype)adWillAppear;
+ (instancetype)adDidAppear;
+ (instancetype)adWillDisappear;
+ (instancetype)adDidDisappear;
+ (instancetype)adShouldRewardUserWithReward:(MPRewardedVideoReward *)reward;
+ (instancetype)adWillLeaveApplication;
@end

@interface MPLogEvent (AdapterAdLifeCycle)
+ (instancetype)adLoadAttemptForAdapter:(NSString *)name dspCreativeId:(NSString * _Nullable)creativeId dspName:(NSString * _Nullable)dspName;
+ (instancetype)adLoadSuccessForAdapter:(NSString *)name;
+ (instancetype)adLoadFailedForAdapter:(NSString *)name error:(NSError *)error;
+ (instancetype)adShowAttemptForAdapter:(NSString *)name;
+ (instancetype)adShowSuccessForAdapter:(NSString *)name;
+ (instancetype)adShowFailedForAdapter:(NSString *)name error:(NSError *)error;
+ (instancetype)adWillPresentModalForAdapter:(NSString *)name;
+ (instancetype)adDidDismissModalForAdapter:(NSString *)name;
+ (instancetype)adTappedForAdapter:(NSString *)name;
+ (instancetype)adWillAppearForAdapter:(NSString *)name;
+ (instancetype)adDidAppearForAdapter:(NSString *)name;
+ (instancetype)adWillDisappearForAdapter:(NSString *)name;
+ (instancetype)adDidDisappearForAdapter:(NSString *)name;
+ (instancetype)adWillLeaveApplicationForAdapter:(NSString *)name;
@end

@interface MPLogEvent (Initialization)
+ (instancetype)sdkInitializedWithNetworks:(NSArray<id<MPAdapterConfiguration>> *)networks;
@end

@interface MPLogEvent (Consent)
+ (instancetype)consentSyncAttempted;
+ (instancetype)consentSyncCompletedWithMessage:(NSString * _Nullable)message;
+ (instancetype)consentSyncFailedWithError:(NSError *)error;
+ (instancetype)consentUpdatedTo:(MPConsentStatus)newStatus from:(MPConsentStatus)oldStatus reason:(NSString * _Nullable)reason canCollectPersonalInfo:(BOOL)canCollectPII;
+ (instancetype)consentShouldShowDialog;
+ (instancetype)consentDialogLoadAttempted;
+ (instancetype)consentDialogLoadSuccess;
+ (instancetype)consentDialogLoadFailedWithError:(NSError *)error;
+ (instancetype)consentDialogShowAttempted;
+ (instancetype)consentDialogShowSuccess;
+ (instancetype)consentDialogShowFailedWithError:(NSError *)error;
@end

@interface MPLogEvent (Javascript)
+ (instancetype)javascriptConsoleLogWithMessage:(NSString *)message;
@end

NS_ASSUME_NONNULL_END
