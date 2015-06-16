//
//  MWMBookmarkColorViewController.h
//  Maps
//
//  Created by v.mikhaylenko on 27.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "ViewController.h"

@class MWMPlacePageViewManager;

@interface MWMBookmarkColorViewController : ViewController

@property (weak, nonatomic) MWMPlacePageViewManager * placePageManager;
@property (weak, nonatomic) UINavigationController * iPadOwnerNavigationController;

@end
