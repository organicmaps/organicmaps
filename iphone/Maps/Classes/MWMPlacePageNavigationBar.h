//
//  MWMPlacePageNavigationBar.h
//  Maps
//
//  Created by v.mikhaylenko on 13.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@class MWMiPhonePortraitPlacePageView;

@interface MWMPlacePageNavigationBar : UIView

@property (weak, nonatomic, readonly) IBOutlet UILabel *titleLabel;

+ (instancetype)navigationBarWithPlacePage:(MWMiPhonePortraitPlacePageView *)placePage;

@end
