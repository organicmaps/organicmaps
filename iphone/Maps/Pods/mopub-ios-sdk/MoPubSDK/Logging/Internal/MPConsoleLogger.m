//
//  MPConsoleLogger.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPConsoleLogger.h"

@implementation MPConsoleLogger

- (instancetype)init {
    if (self = [super init]) {
        // The console logging level is set to `MPBLogLevelInfo` by default in the event that an
        // error needs to be logged to the console prior to SDK initialization. `MPMoPubConfiguration`
        // will set the log level to `MPBLogLevelInfo` during initialization.
        _logLevel = MPBLogLevelInfo;
    }

    return self;
}

- (void)logMessage:(NSString *)message {
    NSLog(@"%@", message);
}

@end
