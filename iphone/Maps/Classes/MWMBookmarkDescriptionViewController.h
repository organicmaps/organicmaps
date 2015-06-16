//
//  MWMBookmarkDescriptionViewController.h
//  Maps
//
//  Created by v.mikhaylenko on 03.06.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "ViewController.h"

@class MWMPlacePageViewManager;

@interface MWMBookmarkDescriptionViewController : ViewController

- (instancetype)initWithPlacePageManager:(MWMPlacePageViewManager *)manager;

@property (weak, nonatomic) UINavigationController * iPadOwnerNavigationController;

@end
