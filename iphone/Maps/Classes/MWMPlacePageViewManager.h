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
@protocol MWMPlacePageViewDragDelegate;

@interface MWMPlacePageViewManager : NSObject

@property (weak, nonatomic, readonly) UIViewController<MWMPlacePageViewDragDelegate> * ownerViewController;
@property (nonatomic, readonly) MWMPlacePageEntity * entity;

- (instancetype)initWithViewController:(UIViewController<MWMPlacePageViewDragDelegate> *)viewController;
- (void)showPlacePageWithUserMark:(unique_ptr<UserMarkCopy>)userMark;
- (void)dismissPlacePage;
- (void)buildRoute;
- (void)stopBuildingRoute;
- (void)share;
- (void)addBookmark;
- (void)removeBookmark;
- (void)layoutPlacePageToOrientation:(UIInterfaceOrientation)orientation;
- (void)reloadBookmark;
- (void)dragPlacePage:(CGPoint)point;

- (instancetype)init __attribute__((unavailable("init is unavailable, call initWithViewController: instead")));

@end
