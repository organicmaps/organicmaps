//
//  MWMMapViewControlsManager.h
//  Maps
//
//  Created by Ilya Grechuhin on 14.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSUInteger, MWMMapViewControlsButton)
{
  MWMMapViewControlsButtonZoom,
  MWMMapViewControlsButtonMenu,
  MWMMapViewControlsButtonLocation
};

@class MapViewController;

@interface MWMMapViewControlsManager : NSObject

@property (nonatomic) BOOL hidden;
@property (nonatomic) BOOL zoomHidden;
@property (nonatomic) BOOL menuHidden;
@property (nonatomic) BOOL locationHidden;

- (instancetype)initWithParentController:(MapViewController *)controller;
- (void)resetZoomButtonsVisibility;
- (void)moveButton:(MWMMapViewControlsButton)button toDefaultPosition:(BOOL)defaultPosition;
- (void)setBottomBound:(CGFloat)bound;

@end
