//
//  MWMMapViewControlsWrapper.h
//  Maps
//
//  Created by Ilya Grechuhin on 02.06.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MWMMapViewControlsWrapper : UIView

- (instancetype)init __attribute__((unavailable("init is not available")));
- (instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("initWithFrame is not available")));
- (instancetype)initWithCoder:(NSCoder *)aDecoder __attribute__((unavailable("initWithCoder is not available")));

- (instancetype)initWithParentView:(UIView *)view;

@end
