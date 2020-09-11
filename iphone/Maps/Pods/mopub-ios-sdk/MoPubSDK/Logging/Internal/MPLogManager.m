//
//  MPLogManager.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPLogManager.h"
#import "MPConsoleLogger.h"
#import "MPIdentityProvider.h"

// Log format constants
static NSString * const kInfoFormat = @"%@][%@";
static NSString * const kLogFormat = @"\n\t[MoPub][%@] %@";

// Cached IDFA strings used to obfuscate the real IDFA with a sanitized version.
static NSString * sIdentifier;
static NSString * sObfuscatedIdentifier;

@interface MPLogManager()

/**
 Console logger.
 */
@property (nonatomic, strong) MPConsoleLogger * consoleLogger;

/**
 Currently registered loggers.
 */
@property (nonatomic, strong) NSMutableArray<id<MPBLogger>> * loggers;

/**
 Serial dispatch queue to perform logging operations.
 */
@property (nonatomic, strong) dispatch_queue_t queue;

@end

@implementation MPLogManager

#pragma mark - Initialization

+ (MPLogManager *)sharedInstance {
    static dispatch_once_t once;
    static MPLogManager * sharedManager;
    dispatch_once(&once, ^{
        sharedManager = [[self alloc] init];
    });

    return sharedManager;
}

- (instancetype)init {
    if (self = [super init]) {
        _consoleLogger = [[MPConsoleLogger alloc] init];
        _loggers = [NSMutableArray arrayWithObject:_consoleLogger];
        _queue = dispatch_queue_create("com.mopub-ios-sdk.queue", DISPATCH_QUEUE_SERIAL);
    }

    return self;
}

#pragma mark - Computed Properties

- (MPBLogLevel)consoleLogLevel {
    return self.consoleLogger.logLevel;
}

- (void)setConsoleLogLevel:(MPBLogLevel)consoleLogLevel {
    self.consoleLogger.logLevel = consoleLogLevel;
}

#pragma mark - Logger Management

- (void)addLogger:(id<MPBLogger>)logger {
    [self.loggers addObject:logger];
}

- (void)removeLogger:(id<MPBLogger>)logger {
    [self.loggers removeObject:logger];
}

#pragma mark - Logging

- (void)logMessage:(NSString *)message atLogLevel:(MPBLogLevel)level {
    if (level == MPBLogLevelNone) {
        return;
    }

    // Lazily retrieve the IDFA
    if (sIdentifier == nil) {
        sIdentifier = [[MPIdentityProvider identifier] copy];
    }

    // Lazily retrieve the sanitized IDFA
    if (sObfuscatedIdentifier == nil) {
        sObfuscatedIdentifier = [[MPIdentityProvider obfuscatedIdentifier] copy];
    }

    // Replace identifier with a obfuscated version when logging.
    NSString * logMessage = [message stringByReplacingOccurrencesOfString:sIdentifier withString:sObfuscatedIdentifier];

    // Queue up the message for logging.
    __weak __typeof__(self) weakSelf = self;
    dispatch_async(self.queue, ^{
        [weakSelf.loggers enumerateObjectsUsingBlock:^(id<MPBLogger> logger, NSUInteger idx, BOOL *stop) {
            if (logger.logLevel <= level) {
                [logger logMessage:logMessage];
            }
        }];
    });
}

- (void)logEvent:(MPLogEvent *)event source:(NSString *)source fromClass:(NSString *)className {
    NSString * info = (source != nil ? [NSString stringWithFormat:kInfoFormat, className, source] : className);
    NSString * message = [NSString stringWithFormat:kLogFormat, info, event.message];
    [self logMessage:message atLogLevel:event.logLevel];
}

@end
