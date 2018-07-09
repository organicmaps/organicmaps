//
//  MPLogProvider.h
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPLogging.h"

@protocol MPLogger;

@interface MPLogProvider : NSObject

+ (MPLogProvider *)sharedLogProvider;
- (void)addLogger:(id<MPLogger>)logger;
- (void)removeLogger:(id<MPLogger>)logger;
- (void)logMessage:(NSString *)message atLogLevel:(MPLogLevel)logLevel;

@end

@protocol MPLogger <NSObject>

- (MPLogLevel)logLevel;
- (void)logMessage:(NSString *)message;

@end
