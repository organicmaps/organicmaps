#pragma once

#import "MWMRouterTransitStepInfo.h"

struct TransitStepInfo;

@interface MWMRouterTransitStepInfo (Core)

- (instancetype)initWithStepInfo:(TransitStepInfo const &)info;

@end
