//
//  MWMZoomButtonsView.h
//  Maps
//
//  Created by Ilya Grechuhin on 12.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MWMZoomButtonsView : UIView

@property (nonatomic) BOOL defaultPosition;
@property (nonatomic) CGFloat bottomBound;

- (instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("initWithFrame is not available")));
- (instancetype)init __attribute__((unavailable("init is not available")));

@end
