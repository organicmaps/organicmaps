//
//  MWMSideMenuManager.h
//  Maps
//
//  Created by Ilya Grechuhin on 24.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@class MapViewController;

typedef NS_ENUM(NSUInteger, MWMSideMenuState)
{
  MWMSideMenuStateHidden,
  MWMSideMenuStateInactive,
  MWMSideMenuStateActive
};

@interface MWMSideMenuManager : NSObject

@property (nonatomic) MWMSideMenuState state;

- (instancetype)init __attribute__((unavailable("init is not available")));
- (instancetype)initWithParentController:(MapViewController *)controller;

@end
