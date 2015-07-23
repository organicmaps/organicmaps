//
//  MWMSideMenuButtonDelegate.h
//  Maps
//
//  Created by Ilya Grechuhin on 25.06.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

@protocol MWMSideMenuButtonTapProtocol <NSObject>

- (void)handleSingleTap;
- (void)handleDoubleTap;

@end

@protocol MWMSideMenuButtonLayoutProtocol <NSObject>

- (void)menuButtonDidUpdateLayout;

@end