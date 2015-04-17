//
//  MWMBasePlagePageView.h
//  Maps
//
//  Created by v.mikhaylenko on 23.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

#include "map/user_mark.hpp"

@class MWMPlacePageViewManager, MWMPlacePageActionBar, MWMPlacePageEntity;

typedef NS_ENUM(NSUInteger, MWMPlacePageState) {
  MWMPlacePageStateClosed,
  MWMPlacePageStatePreview,
  MWMPlacePageStateOpen
};

@interface MWMBasePlacePageView : UIView

@property (weak, nonatomic) IBOutlet UILabel *titleLabel;
@property (weak, nonatomic) IBOutlet UILabel *typeLabel;
@property (weak, nonatomic) IBOutlet UILabel *distanceLabel;
@property (weak, nonatomic) IBOutlet UIImageView *directionArrow;
@property (weak, nonatomic) IBOutlet UITableView *featureTable;
@property (weak, nonatomic) MWMPlacePageActionBar *actionBar;
@property (weak, nonatomic) MWMPlacePageViewManager *placePageManager;
@property (nonatomic) MWMPlacePageState state;

- (instancetype)init;
- (void)configureWithEntity:(MWMPlacePageEntity *)entity;

- (CGPoint)targetPoint;
- (IBAction)didTap:(UITapGestureRecognizer *)sender;

- (void)show;
- (void)dismiss;
- (void)addBookmark;

@end

@interface MWMBasePlacePageView (Animations)

- (void)cancelSpringAnimation;
- (void)startAnimatingView:(MWMBasePlacePageView *)view initialVelocity:(CGPoint)initialVelocity;

@end
