//
//  MWMBookmarkColorCell.h
//  Maps
//
//  Created by v.mikhaylenko on 27.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MWMBookmarkColorCell : UITableViewCell

@property (weak, nonatomic) IBOutlet UIButton * colorButton;
@property (weak, nonatomic) IBOutlet UILabel * titleLabel;
@property (weak, nonatomic) IBOutlet UIImageView * approveImageView;

- (void)configureWithColorString:(NSString *)colorString;

@end
