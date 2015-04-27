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

- (instancetype)initWithParentView:(UIView *)view;
- (void)resetVisibility;

@end
