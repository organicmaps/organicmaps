//
//  MWMDirectionView.h
//  Maps
//
//  Created by v.mikhaylenko on 27.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MWMDirectionView : UIView

@property (weak, nonatomic) IBOutlet UILabel * titleLabel;
@property (weak, nonatomic) IBOutlet UILabel * typeLabel;
@property (weak, nonatomic) IBOutlet UILabel * distanceLabel;
@property (weak, nonatomic) IBOutlet UIImageView * directionArrow;
@property (weak, nonatomic) IBOutlet UIView * contentView;

+ (MWMDirectionView *)directionViewForViewController:(UIViewController *)viewController;
- (void)setDirectionArrowTransform:(CGAffineTransform)transform;

@end
