//
//  MWMBasePlagePageView.h
//  Maps
//
//  Created by v.mikhaylenko on 23.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@class MWMPlacePageEntity;

@interface MWMBasePlacePageView : UIView

@property (weak, nonatomic) IBOutlet UILabel * titleLabel;
@property (weak, nonatomic) IBOutlet UILabel * typeLabel;
@property (weak, nonatomic) IBOutlet UILabel * distanceLabel;
@property (weak, nonatomic) IBOutlet UIImageView * directionArrow;
@property (weak, nonatomic) IBOutlet UITableView * featureTable;

- (void)configureWithEntity:(MWMPlacePageEntity *)entity;
- (void)addBookmark;

@end
