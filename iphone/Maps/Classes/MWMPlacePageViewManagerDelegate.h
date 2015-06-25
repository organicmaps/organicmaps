//
//  MWMPlacePageViewManagerDelegate.h
//  Maps
//
//  Created by Ilya Grechuhin on 16.06.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

@protocol MWMPlacePageViewManagerDelegate <NSObject>

- (void)dragPlacePage:(CGPoint)point;
- (void)addPlacePageViews:(NSArray *)views;
- (void)updateStatusBarStyle;

@end