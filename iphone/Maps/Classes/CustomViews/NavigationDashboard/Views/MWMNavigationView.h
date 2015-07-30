//
//  MWMNavigationView.h
//  Maps
//
//  Created by Ilya Grechuhin on 22.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMNavigationViewProtocol.h"
#import "UIKitCategories.h"

@interface MWMNavigationView : SolidTouchView

@property (nonatomic) CGFloat topBound;
@property (nonatomic, readonly) CGFloat visibleHeight;
@property (nonatomic, readonly) BOOL isVisible;
@property (weak, nonatomic) id<MWMNavigationViewProtocol> delegate;

- (void)addToView:(UIView *)superview;
- (void)remove;

@end
