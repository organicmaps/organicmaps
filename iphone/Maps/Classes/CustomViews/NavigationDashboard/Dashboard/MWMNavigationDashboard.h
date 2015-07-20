//
//  MWMNavigationDashboard.h
//  Maps
//
//  Created by v.mikhaylenko on 20.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "UIKitCategories.h"

@interface MWMNavigationDashboard : SolidTouchView

@property (weak, nonatomic) IBOutlet UIImageView * direction;
@property (weak, nonatomic) IBOutlet UILabel * distanceToNextAction;
@property (weak, nonatomic) IBOutlet UILabel * distanceToNextActionUnits;
@property (weak, nonatomic) IBOutlet UILabel * distanceLeft;
@property (weak, nonatomic) IBOutlet UILabel * eta;

@end
