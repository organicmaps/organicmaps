//
//  MOPUBExperimentProvider.h
//  MoPub
//
//  Copyright © 2017 MoPub. All rights reserved.
//

#import "MPAdDestinationDisplayAgent.h"

@interface MOPUBExperimentProvider : NSObject

+ (void)setDisplayAgentType:(MOPUBDisplayAgentType)displayAgentType;
+ (void)setDisplayAgentFromAdServer:(MOPUBDisplayAgentType)displayAgentType;
+ (MOPUBDisplayAgentType)displayAgentType;

@end
