//
//  MPLogging.m
//  MoPub
//
//  Created by Andrew He on 2/10/11.
//  Copyright 2011 MoPub, Inc. All rights reserved.
//

#import "MPLogging.h"

static MPLogLevel MPLOG_LEVEL = MPLogLevelInfo;

MPLogLevel MPLogGetLevel()
{
    return MPLOG_LEVEL;
}

void MPLogSetLevel(MPLogLevel level)
{
    MPLOG_LEVEL = level;
}

void _MPLogTrace(NSString *format, ...)
{
    if (MPLOG_LEVEL <= MPLogLevelTrace)
    {
        format = [NSString stringWithFormat:@"MOPUB: %@", format];
        va_list args;
        va_start(args, format);
        NSLogv(format, args);
        va_end(args);
    }
}

void _MPLogDebug(NSString *format, ...)
{
    if (MPLOG_LEVEL <= MPLogLevelDebug)
    {
        format = [NSString stringWithFormat:@"MOPUB: %@", format];
        va_list args;
        va_start(args, format);
        NSLogv(format, args);
        va_end(args);
    }
}

void _MPLogWarn(NSString *format, ...)
{
    if (MPLOG_LEVEL <= MPLogLevelWarn)
    {
        format = [NSString stringWithFormat:@"MOPUB: %@", format];
        va_list args;
        va_start(args, format);
        NSLogv(format, args);
        va_end(args);
    }
}

void _MPLogInfo(NSString *format, ...)
{
    if (MPLOG_LEVEL <= MPLogLevelInfo)
    {
        format = [NSString stringWithFormat:@"MOPUB: %@", format];
        va_list args;
        va_start(args, format);
        NSLogv(format, args);
        va_end(args);
    }
}

void _MPLogError(NSString *format, ...)
{
    if (MPLOG_LEVEL <= MPLogLevelError)
    {
        format = [NSString stringWithFormat:@"MOPUB: %@", format];
        va_list args;
        va_start(args, format);
        NSLogv(format, args);
        va_end(args);
    }
}

void _MPLogFatal(NSString *format, ...)
{
    if (MPLOG_LEVEL <= MPLogLevelFatal)
    {
        format = [NSString stringWithFormat:@"MOPUB: %@", format];
        va_list args;
        va_start(args, format);
        NSLogv(format, args);
        va_end(args);
    }
}
