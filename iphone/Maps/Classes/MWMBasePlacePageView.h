//
//  MWMBasePlagePageView.h
//  Maps
//
//  Created by v.mikhaylenko on 23.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@class MWMPlacePageEntity, MWMDirectionView;

@interface MWMBasePlacePageView : UIView

@property (weak, nonatomic) IBOutlet UILabel * titleLabel;
@property (weak, nonatomic) IBOutlet UILabel * typeLabel;
@property (weak, nonatomic) IBOutlet UILabel * distanceLabel;
@property (nonatomic) IBOutlet UIImageView * directionArrow;
@property (weak, nonatomic) IBOutlet UITableView * featureTable;
@property (weak, nonatomic) IBOutlet UIView * separatorView;
@property (weak, nonatomic) IBOutlet UIButton * directionButton;
@property (nonatomic) UIView * typeDescriptionView;
@property (nonatomic) MWMDirectionView * directionView;

- (void)configureWithEntity:(MWMPlacePageEntity *)entity;
- (void)addBookmark;
- (void)removeBookmark;
- (void)reloadBookmarkCell;
- (void)layoutDistanceLabel;

@end
