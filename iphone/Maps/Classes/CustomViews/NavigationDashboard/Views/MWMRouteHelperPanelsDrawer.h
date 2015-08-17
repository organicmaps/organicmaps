//
//  MWMRouteHelperPanelsDrawer.h
//  Maps
//
//  Created by v.mikhaylenko on 08.09.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMRouteHelperPanel.h"

@interface MWMRouteHelperPanelsDrawer : NSObject

@property (nonatomic, weak, readonly) UIView * parentView;

- (instancetype)initWithView:(UIView *)view;
- (void)drawPanels:(vector<MWMRouteHelperPanel *> const &)panels;
- (void)invalidateTopBounds:(vector<MWMRouteHelperPanel *> const &)panels forOrientation:(UIInterfaceOrientation)orientation;

@end
