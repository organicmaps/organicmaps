//
//  MWMPlacePageActionBar.h
//  Maps
//
//  Created by v.mikhaylenko on 28.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@class MWMPlacePage;

@interface MWMPlacePageActionBar : UIView

@property (weak, nonatomic) IBOutlet UIButton * bookmarkButton;
@property (weak, nonatomic) IBOutlet UIButton * routeButton;

+ (MWMPlacePageActionBar *)actionBarForPlacePage:(MWMPlacePage *)placePage;
- (void)configureForMyPosition:(BOOL)isMyPosition;

- (instancetype)init __attribute__((unavailable("init is unavailable, call actionBarForPlacePage: instead")));
- (instancetype)initWithCoder:(NSCoder *)aDecoder __attribute__((unavailable("initWithCoder: is unavailable, call actionBarForPlacePage: instead")));
- (instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("initWithFrame: is unavailable, call actionBarForPlacePage: instead")));

@end
