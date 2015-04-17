//
//  MWMPlacePageActionBar.h
//  Maps
//
//  Created by v.mikhaylenko on 28.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@class MWMBasePlacePageView;

@interface MWMPlacePageActionBar : UIView

+ (MWMPlacePageActionBar *)actionBarForPlacePage:(MWMBasePlacePageView *)placePage;

@end
