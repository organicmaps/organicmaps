//
//  MPBLogger.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPBLogLevel.h"

/**
 Objects which are capable of consuming log messages.
 */
@protocol MPBLogger <NSObject>

/**
 Current logging level.
 */
@property (nonatomic, readonly) MPBLogLevel logLevel;

/**
 Message to be logged.
 @param message Message to be logged.
 */
- (void)logMessage:(NSString * _Nullable)message;

@end
