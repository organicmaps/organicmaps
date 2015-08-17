//
//  MWMRouteHelperPanel.h
//  Maps
//
//  Created by v.mikhaylenko on 26.08.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

@interface MWMRouteHelperPanel : UIView

@property (weak, nonatomic) UIView * parentView;
@property (nonatomic) CGFloat topBound;

- (CGFloat)defaultHeight;

@end
