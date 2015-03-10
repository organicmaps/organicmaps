//
//  MWMAlertViewController.h
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MWMAlert.h"
@protocol MWMAlertViewControllerDelegate;

@interface MWMAlertViewController : UIViewController

@property (nonatomic, weak) id<MWMAlertViewControllerDelegate> delegate;
@property (nonatomic, weak, readonly) UIViewController *ownerViewController;

- (instancetype)initWithViewController:(UIViewController *)viewController;
- (void)presentAlertWithType:(MWMAlertType)type;
- (void)close;

@end
