//
//  MWMZoomButtons.h
//  Maps
//
//  Created by Ilya Grechuhin on 12.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface MWMZoomButtons : NSObject

@property (nonatomic) BOOL hidden;

- (instancetype)init __attribute__((unavailable("init is not available")));
- (instancetype)initWithParentView:(UIView *)view;
- (void)resetVisibility;
- (void)setTopBound:(CGFloat)bound;
- (void)setBottomBound:(CGFloat)bound;

@end
