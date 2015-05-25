//
//  MWMBookmarkColorViewController.h
//  Maps
//
//  Created by v.mikhaylenko on 27.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@class MWMPlacePageViewManager;

@interface MWMBookmarkColorViewController : UIViewController

@property (weak, nonatomic) MWMPlacePageViewManager * placePageManager;
@property (weak, nonatomic) UINavigationController * ownerNavigationController;

@end
