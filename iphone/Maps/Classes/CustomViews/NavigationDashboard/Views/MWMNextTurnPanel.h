//
//  MWMNextTurnPanel.h
//  Maps
//
//  Created by v.mikhaylenko on 24.08.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMRouteHelperPanel.h"

@interface MWMNextTurnPanel : MWMRouteHelperPanel

+ (instancetype)turnPanelWithOwnerView:(UIView *)ownerView;
- (void)configureWithImage:(UIImage *)image;

@end
