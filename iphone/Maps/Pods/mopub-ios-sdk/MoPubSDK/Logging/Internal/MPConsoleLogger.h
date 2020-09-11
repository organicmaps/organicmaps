//
//  MPConsoleLogger.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPBLogger.h"

/**
 Console logging destination routes all log messages to @c NSLog.
 */
@interface MPConsoleLogger : NSObject<MPBLogger>

/**
 Log level. By default, this is set to @c MPBLogLevelInfo.
 */
@property (nonatomic, assign) MPBLogLevel logLevel;

@end
