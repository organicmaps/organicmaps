//
//  MWMLocationButtonStatusLabel.h
//  Maps
//
//  Created by Ilya Grechuhin on 29.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MWMLocationButtonStatusLabel : UILabel

- (instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("initWithFrame is not available")));
- (instancetype)init __attribute__((unavailable("init is not available")));

- (void)show;

@end
