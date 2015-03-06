//
//  MWMAlertViewController.h
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MWMAlert.h"

@interface MWMAlertViewController : UIViewController
- (instancetype)initWithViewController:(UIViewController *)viewController;
- (void)presentAlertWithType:(MWMAlertType)type;
- (void)close;
@end
