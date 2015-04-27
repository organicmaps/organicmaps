//
//  MWMSideMenuButton.h
//  Maps
//
//  Created by Ilya Grechuhin on 24.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MWMSideMenuButton : UIButton

- (void)addSelfToView:(UIView *)parentView;
- (void)addSelfHiddenToView:(UIView *)parentView;

- (instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("initWithFrame is not available")));
- (instancetype)init __attribute__((unavailable("init is not available")));

@end
