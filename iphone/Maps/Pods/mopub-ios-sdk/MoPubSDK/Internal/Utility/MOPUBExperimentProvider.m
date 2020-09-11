//
//  MOPUBExperimentProvider.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MOPUBExperimentProvider.h"

@interface MOPUBExperimentProvider ()
@property (nonatomic, assign) BOOL isDisplayAgentOverriddenByClient;
@end

@implementation MOPUBExperimentProvider

@synthesize displayAgentType = _displayAgentType;

- (instancetype)init {
    self = [super init];
    if (self != nil) {
        _isDisplayAgentOverriddenByClient = NO;
        _displayAgentType = MOPUBDisplayAgentTypeInApp;
    }
    return self;
}

+ (instancetype)sharedInstance {
    static dispatch_once_t once;
    static id _sharedInstance;
    dispatch_once(&once, ^{
        _sharedInstance = [self new];
    });
    return _sharedInstance;
}

- (void)setDisplayAgentType:(MOPUBDisplayAgentType)displayAgentType {
    _isDisplayAgentOverriddenByClient = YES;
    _displayAgentType = displayAgentType;
}

- (void)setDisplayAgentFromAdServer:(MOPUBDisplayAgentType)displayAgentType {
    if (!self.isDisplayAgentOverriddenByClient) {
        _displayAgentType = displayAgentType;
    }
}

@end
