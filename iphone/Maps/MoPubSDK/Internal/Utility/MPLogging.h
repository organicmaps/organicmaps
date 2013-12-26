//
//  MPLogging.h
//  MoPub
//
//  Created by Andrew He on 2/10/11.
//  Copyright 2011 MoPub, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPConstants.h"

// Lower = finer-grained logs.
typedef enum
{
    MPLogLevelAll        = 0,
    MPLogLevelTrace        = 10,
    MPLogLevelDebug        = 20,
    MPLogLevelInfo        = 30,
    MPLogLevelWarn        = 40,
    MPLogLevelError        = 50,
    MPLogLevelFatal        = 60,
    MPLogLevelOff        = 70
} MPLogLevel;

MPLogLevel MPLogGetLevel(void);
void MPLogSetLevel(MPLogLevel level);
void _MPLogTrace(NSString *format, ...);
void _MPLogDebug(NSString *format, ...);
void _MPLogInfo(NSString *format, ...);
void _MPLogWarn(NSString *format, ...);
void _MPLogError(NSString *format, ...);
void _MPLogFatal(NSString *format, ...);

#if MP_DEBUG_MODE && !SPECS

#define MPLogTrace(...) _MPLogTrace(__VA_ARGS__)
#define MPLogDebug(...) _MPLogDebug(__VA_ARGS__)
#define MPLogInfo(...) _MPLogInfo(__VA_ARGS__)
#define MPLogWarn(...) _MPLogWarn(__VA_ARGS__)
#define MPLogError(...) _MPLogError(__VA_ARGS__)
#define MPLogFatal(...) _MPLogFatal(__VA_ARGS__)

#else

#define MPLogTrace(...) {}
#define MPLogDebug(...) {}
#define MPLogInfo(...) {}
#define MPLogWarn(...) {}
#define MPLogError(...) {}
#define MPLogFatal(...) {}

#endif
