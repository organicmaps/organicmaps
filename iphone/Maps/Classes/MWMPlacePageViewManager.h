//
//  MWMPlacePageViewManager.h
//  Maps
//
//  Created by v.mikhaylenko on 23.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>

#include "map/user_mark.hpp"

@class MWMPlacePageEntity, MWMPlacePageNavigationBar;
@protocol MWMPlacePageViewManagerDelegate;

@interface MWMPlacePageViewManager : NSObject

@property (weak, nonatomic, readonly) UIViewController<MWMPlacePageViewManagerDelegate> * ownerViewController;
@property (nonatomic, readonly) MWMPlacePageEntity * entity;
@property (nonatomic) MWMPlacePageNavigationBar * iPhoneNavigationBar;
@property (nonatomic) CGFloat topBound;

- (instancetype)initWithViewController:(UIViewController<MWMPlacePageViewManagerDelegate> *)viewController;
- (void)showPlacePageWithUserMark:(unique_ptr<UserMarkCopy>)userMark;
- (void)refreshPlacePage;
- (void)dismissPlacePage;
- (void)buildRoute;
- (void)stopBuildingRoute;
- (void)share;
- (void)addBookmark;
- (void)removeBookmark;
- (void)layoutPlacePageToOrientation:(UIInterfaceOrientation)orientation;
- (void)reloadBookmark;
- (void)dragPlacePage:(CGPoint)point;
- (void)showDirectionViewWithTitle:(NSString *)title type:(NSString *)type;
- (void)addSubviews:(NSArray *)views withNavigationController:(UINavigationController *)controller;

- (instancetype)init __attribute__((unavailable("init is unavailable, call initWithViewController: instead")));

@end
