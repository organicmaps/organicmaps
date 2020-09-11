//
//  MPLogManager.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPLogEvent.h"
#import "MPBLogger.h"
#import "MPLogging.h"
#import "MPBLogLevel.h"

NS_ASSUME_NONNULL_BEGIN

/**
 Manages all logging sources for the MoPub SDK. By default, the manager will always
 contain a console logger destination.
 */
@interface MPLogManager : NSObject

/**
 Current log level of the console logger.
 */
@property (nonatomic, assign) MPBLogLevel consoleLogLevel;

/**
 Retrieves the singleton instance of @c MPLogManager.
 */
+ (instancetype)sharedInstance;

/**
 Registers a logging destination.
 @param logger Logger to receive log events.
 */
- (void)addLogger:(id<MPBLogger>)logger;

/**
 Removes a logger from receiving log events.
 @param logger Logger to remove.
 */
- (void)removeLogger:(id<MPBLogger>)logger;

/**
 Logs the message to all available logging destinations at the
 specified log level.
 @param message Message to log.
 @param level Log level.
 */
- (void)logMessage:(NSString *)message atLogLevel:(MPBLogLevel)level;

/**
 Logs the event generated from the calling class. The format of the log message
 will be:
 @code
 className | source | logEvent.message
 @endcode
 @param event Event to log.
 @param source Optional source of the event. This will generally be ad unit ID for ad-related events.
 @param className Name of the class invoking @c logEvent:fromClass:
 */
- (void)logEvent:(MPLogEvent *)event source:(NSString * _Nullable)source fromClass:(NSString *)className;

@end

NS_ASSUME_NONNULL_END
