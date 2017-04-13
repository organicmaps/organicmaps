//
//  MPLogging.m
//  MoPub
//
//  Copyright 2011 MoPub, Inc. All rights reserved.
//

#import "MPLogging.h"
#import "MPIdentityProvider.h"
#import "MPLogProvider.h"

NSString * const kMPClearErrorLogFormatWithAdUnitID = @"No ads found for ad unit: %@";
NSString * const kMPWarmingUpErrorLogFormatWithAdUnitID = @"Ad unit %@ is currently warming up. Please try again in a few minutes.";
NSString * const kMPSystemLogPrefix = @"MOPUB: %@";

static MPLogLevel systemLogLevel = MPLogLevelInfo;

MPLogLevel MPLogGetLevel()
{
    return systemLogLevel;
}

void MPLogSetLevel(MPLogLevel level)
{
    systemLogLevel = level;
}

void _MPLog(MPLogLevel level, NSString *format, va_list args)
{
    static NSString *sIdentifier;
    static NSString *sObfuscatedIdentifier;

    if (!sIdentifier) {
        sIdentifier = [[MPIdentityProvider identifier] copy];
    }

    if (!sObfuscatedIdentifier) {
        sObfuscatedIdentifier = [[MPIdentityProvider obfuscatedIdentifier] copy];
    }

    NSString *logString = [[NSString alloc] initWithFormat:format arguments:args];

    // Replace identifier with a obfuscated version when logging.
    logString = [logString stringByReplacingOccurrencesOfString:sIdentifier withString:sObfuscatedIdentifier];

    [[MPLogProvider sharedLogProvider] logMessage:logString atLogLevel:level];
}

void _MPLogTrace(NSString *format, ...)
{
    format = [NSString stringWithFormat:kMPSystemLogPrefix, format];
    va_list args;
    va_start(args, format);
    _MPLog(MPLogLevelTrace, format, args);
    va_end(args);
}

void _MPLogDebug(NSString *format, ...)
{
    format = [NSString stringWithFormat:kMPSystemLogPrefix, format];
    va_list args;
    va_start(args, format);
    _MPLog(MPLogLevelDebug, format, args);
    va_end(args);
}

void _MPLogWarn(NSString *format, ...)
{
    format = [NSString stringWithFormat:kMPSystemLogPrefix, format];
    va_list args;
    va_start(args, format);
    _MPLog(MPLogLevelWarn, format, args);
    va_end(args);
}

void _MPLogInfo(NSString *format, ...)
{
    format = [NSString stringWithFormat:kMPSystemLogPrefix, format];
    va_list args;
    va_start(args, format);
    _MPLog(MPLogLevelInfo, format, args);
    va_end(args);
}

void _MPLogError(NSString *format, ...)
{
    format = [NSString stringWithFormat:kMPSystemLogPrefix, format];
    va_list args;
    va_start(args, format);
    _MPLog(MPLogLevelError, format, args);
    va_end(args);
}

void _MPLogFatal(NSString *format, ...)
{
    format = [NSString stringWithFormat:kMPSystemLogPrefix, format];
    va_list args;
    va_start(args, format);
    _MPLog(MPLogLevelFatal, format, args);
    va_end(args);
}
