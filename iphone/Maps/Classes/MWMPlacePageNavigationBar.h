//
//  MWMPlacePageNavigationBar.h
//  Maps
//
//  Created by v.mikhaylenko on 13.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@class MWMiPhonePortraitPlacePage;

@interface MWMPlacePageNavigationBar : UIView

+ (void)dismissNavigationBar;
+ (void)showNavigationBarForPlacePage:(MWMiPhonePortraitPlacePage *)placePage;
+ (void)remove;

- (instancetype)init __attribute__((unavailable("call navigationBarWithPlacePage: instead")));
- (instancetype)initWithCoder:(NSCoder *)aDecoder __attribute__((unavailable("call navigationBarWithPlacePage: instead")));
- (instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("call navigationBarWithPlacePage: instead")));

@end
