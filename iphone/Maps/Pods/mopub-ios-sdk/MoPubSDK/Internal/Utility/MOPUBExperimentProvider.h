//
//  MOPUBExperimentProvider.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPAdDestinationDisplayAgent.h"

@interface MOPUBExperimentProvider : NSObject

@property (nonatomic, assign) MOPUBDisplayAgentType displayAgentType;

+ (instancetype)sharedInstance;

- (void)setDisplayAgentFromAdServer:(MOPUBDisplayAgentType)displayAgentType;

@end
