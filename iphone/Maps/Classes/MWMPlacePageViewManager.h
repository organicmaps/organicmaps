//
//  MWMPlacePageViewManager.h
//  Maps
//
//  Created by v.mikhaylenko on 23.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>

#include "map/user_mark.hpp"

@class MWMPlacePageEntity;

@interface MWMPlacePageViewManager : NSObject

@property (weak, nonatomic, readonly) UIViewController * ownerViewController;
@property (nonatomic, readonly) MWMPlacePageEntity * entity;

- (instancetype)initWithViewController:(UIViewController *)viewController;
- (void)showPlacePageWithUserMark:(unique_ptr<UserMarkCopy>)userMark;
- (void)dismissPlacePage;
- (void)buildRoute;
- (void)stopBuildingRoute;
- (void)share;
- (void)addBookmark;
- (void)removeBookmark;
- (void)layoutPlacePageToOrientation:(UIInterfaceOrientation)orientation;
- (void)reloadBookmark;

- (instancetype)init __attribute__((unavailable("init is unavailable, call initWithViewController: instead")));

@end
