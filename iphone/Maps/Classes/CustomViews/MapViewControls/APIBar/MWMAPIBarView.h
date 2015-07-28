//
//  MWMAPIBarView.h
//  Maps
//
//  Created by Ilya Grechuhin on 27.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

@interface MWMAPIBarView : UIView

@property (nonatomic) CGFloat targetY;

- (nonnull instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("initWithFrame is not available")));
- (nonnull instancetype)init __attribute__((unavailable("init is not available")));

@end
