//
//  MWMPlacePageBaseCell.h
//  Maps
//
//  Created by v.mikhaylenko on 27.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MWMPlacePageInfoCell : UITableViewCell

- (void)configureWithIconTitle:(NSString *)title info:(NSString *)info;

@property (weak, nonatomic, readonly) IBOutlet UIImageView * icon;
@property (weak, nonatomic, readonly) IBOutlet id textContainer;

@end
