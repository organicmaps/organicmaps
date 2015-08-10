//
//  MWMPlacePageViewManagerDelegate.h
//  Maps
//
//  Created by Ilya Grechuhin on 16.06.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

@protocol MWMPlacePageViewManagerProtocol <NSObject>

- (void)dragPlacePage:(CGPoint)point;
- (void)addPlacePageViews:(NSArray *)views;
- (void)updateStatusBarStyle;
- (void)buildRoute:(m2::PointD)destination;
- (void)apiBack;

@end