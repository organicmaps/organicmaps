//
//  MWMSideMenuView.h
//  Maps
//
//  Created by Ilya Grechuhin on 23.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MWMSideMenuView : UIView

@property (weak, nonatomic, readonly) IBOutlet UIView * dimBackground;

- (instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("initWithFrame is not available")));
- (instancetype)init __attribute__((unavailable("init is not available")));

- (void)addSelfToView:(UIView *)parentView;
- (void)removeFromSuperviewAnimated;

@end
