//
//  MWMLanesPanel.h
//  Maps
//
//  Created by v.mikhaylenko on 20.08.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMRouteHelperPanel.h"

#include "platform/location.hpp"

@interface MWMLanesPanel : MWMRouteHelperPanel

- (instancetype)initWithParentView:(UIView *)parentView;
- (void)configureWithLanes:(std::vector<location::FollowingInfo::SingleLaneInfoClient> const &)lanes;

@end
