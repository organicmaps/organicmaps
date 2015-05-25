//
//  MWMPlacePageBookmarkCell.h
//  Maps
//
//  Created by v.mikhaylenko on 29.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@class MWMPlacePageEntity, MWMPlacePage, MWMTextView;

@interface MWMPlacePageBookmarkCell : UITableViewCell

@property (weak, nonatomic, readonly) IBOutlet UITextField * title;
@property (weak, nonatomic, readonly) IBOutlet UIButton * categoryButton;
@property (weak, nonatomic, readonly) IBOutlet UIButton * markButton;
@property (weak, nonatomic, readonly) IBOutlet UILabel * descriptionLabel;
@property (weak, nonatomic) UITableView * ownerTableView;
@property (weak, nonatomic) MWMPlacePage * placePage;

- (void)configure;

@end
