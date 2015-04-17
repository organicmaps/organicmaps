//
//  MWMPlacePageViewManager.h
//  Maps
//
//  Created by v.mikhaylenko on 23.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>

#include "map/user_mark.hpp"

@interface MWMPlacePageViewManager : NSObject

@property (weak, nonatomic, readonly) UIViewController *ownerViewController;

- (instancetype)initWithViewController:(UIViewController *)viewController;
- (void)showPlacePageWithUserMark:(std::unique_ptr<UserMarkCopy>)userMark;
- (void)dismissPlacePage;
- (void)layoutPlacePage;

@end
