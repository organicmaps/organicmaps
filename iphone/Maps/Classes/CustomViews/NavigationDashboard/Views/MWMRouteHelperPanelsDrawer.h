//
//  MWMRouteHelperPanelsDrawer.h
//  Maps
//
//  Created by v.mikhaylenko on 08.09.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMRouteHelperPanel.h"

@interface MWMRouteHelperPanelsDrawer : NSObject

@property (weak, nonatomic, readonly) UIView * parentView;

- (instancetype)initWithView:(UIView *)view;
- (void)drawPanels:(NSArray *)panels;
- (void)invalidateTopBounds:(NSArray *)panels forOrientation:(UIInterfaceOrientation)orientation;

@end
