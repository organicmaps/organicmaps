//
//  MWMPlacePageBookmarkCell.h
//  Maps
//
//  Created by v.mikhaylenko on 29.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MWMPlacePageBookmarkCell : UITableViewCell

@property (weak, nonatomic, readonly) IBOutlet UITextView * title;
@property (weak, nonatomic, readonly) IBOutlet UIButton * categoryButton;
@property (weak, nonatomic, readonly) IBOutlet UIButton * markButton;
@property (weak, nonatomic, readonly) IBOutlet UITextView * descriptionTextView;

@property (weak, nonatomic) UITableView * ownerTableView;
@property (nonatomic) CGFloat actualHeight;

- (void)configure;

@end
