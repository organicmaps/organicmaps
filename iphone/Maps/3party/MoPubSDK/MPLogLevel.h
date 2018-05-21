//
//  MPLogLevel.h
//  MoPubSDK
//
//  Copyright © 2017 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

/**
 * SDK logging level.
 * @remark Lower values equate to more detailed logs.
 */
typedef enum {
    MPLogLevelAll   = 0,
    MPLogLevelTrace = 10,
    MPLogLevelDebug = 20,
    MPLogLevelInfo  = 30,
    MPLogLevelWarn  = 40,
    MPLogLevelError = 50,
    MPLogLevelFatal = 60,
    MPLogLevelOff   = 70
} MPLogLevel;
