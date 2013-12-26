//
//  MMSDK.h
//  MMSDK
//
//  Copyright (c) 2013 Millennial Media Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MMRequest.h"

// NSNotification keys
extern NSString * const MillennialMediaAdWillTerminateApplication;
extern NSString * const MillennialMediaAdWasTapped;
extern NSString * const MillennialMediaAdModalWillAppear;
extern NSString * const MillennialMediaAdModalDidAppear;
extern NSString * const MillennialMediaAdModalWillDismiss;
extern NSString * const MillennialMediaAdModalDidDismiss;
extern NSString * const MillennialMediaKeyboardWillObscureAd;
extern NSString * const MillennialMediaKeyboardWillHide;

// NSNotification userInfo keys
extern NSString * const MillennialMediaAdObjectKey;
extern NSString * const MillennialMediaAPIDKey;
extern NSString * const MillennialMediaAdTypeKey;

// Millennial Media Ad Type keys
extern NSString * const MillennialMediaAdTypeBanner;
extern NSString * const MillennialMediaAdTypeInterstitial;

typedef enum ErrorCode {
    MMAdUnknownError = 0,
    MMAdServerError = -500,
    MMAdUnavailable = -503,
    MMAdDisabled    = -9999999
} MMErrorCode;

typedef enum LogLevel {
    MMLOG_LEVEL_OFF   = 0,
    MMLOG_LEVEL_INFO  = 1 << 0,
    MMLOG_LEVEL_DEBUG = 1 << 1,
    MMLOG_LEVEL_ERROR = 1 << 2,
    MMLOG_LEVEL_FATAL = 1 << 3
} MMLogLevel;

typedef void (^MMCompletionBlock) (BOOL success, NSError *error);

@interface MMSDK : NSObject

+ (void)initialize;
+ (NSString *)version;
+ (void)trackConversionWithGoalId:(NSString *)goalId;
+ (void)trackConversionWithGoalId:(NSString *)goalId requestData:(MMRequest *)request;
+ (void)setLogLevel:(MMLogLevel)level;
+ (void)trackEventWithId:(NSString *)eventId;

@end
