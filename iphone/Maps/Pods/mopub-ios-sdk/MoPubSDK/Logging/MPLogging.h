//
//  MPLogging.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPLogEvent.h"
#import "MPBLogger.h"
#import "MPBLogLevel.h"

NS_ASSUME_NONNULL_BEGIN

extern NSString * const kMPClearErrorLogFormatWithAdUnitID;
extern NSString * const kMPWarmingUpErrorLogFormatWithAdUnitID;

#define MPLogDebug(...) [MPLogging logEvent:[MPLogEvent eventWithMessage:[NSString stringWithFormat:__VA_ARGS__] level:MPBLogLevelDebug] source:nil fromClass:self.class]
#define MPLogInfo(...) [MPLogging logEvent:[MPLogEvent eventWithMessage:[NSString stringWithFormat:__VA_ARGS__] level:MPBLogLevelInfo] source:nil fromClass:self.class]

// MPLogTrace, MPLogWarn, MPLogError, and MPLogFatal will be deprecated in
// future SDK versions. Please use MPLogInfo or MPLogDebug
#define MPLogTrace(...) MPLogDebug(__VA_ARGS__)
#define MPLogWarn(...) MPLogDebug(__VA_ARGS__)
#define MPLogError(...) MPLogDebug(__VA_ARGS__)
#define MPLogFatal(...) MPLogDebug(__VA_ARGS__)

// Logs ad lifecycle events
#define MPLogAdEvent(event, adUnitId) [MPLogging logEvent:event source:adUnitId fromClass:self.class]

// Logs general events
#define MPLogEvent(event) [MPLogging logEvent:event source:nil fromClass:self.class]

/**
 SDK logging support.
 */
@interface MPLogging : NSObject
/**
 Current log level of the SDK console logger. The default value is @c MPBLogLevelNone.
 */
@property (class, nonatomic, assign) MPBLogLevel consoleLogLevel;

/**
Registers a logging destination.
@param logger Logger to receive log events.
*/
+ (void)addLogger:(id<MPBLogger>)logger;

/**
 Removes a logger from receiving log events.
 @param logger Logger to remove.
 */
+ (void)removeLogger:(id<MPBLogger>)logger;

/**
 Logs the event generated from the calling class. The format of the log message
 will be:
 @code
 className | source | logEvent.message
 @endcode
 @param event Event to log.
 @param source Optional source of the event. This will generally be ad unit ID for ad-related events.
 @param aClass Class that generated the event.
 */
+ (void)logEvent:(MPLogEvent *)event source:(NSString * _Nullable)source fromClass:(Class _Nullable)aClass;

@end

NS_ASSUME_NONNULL_END
