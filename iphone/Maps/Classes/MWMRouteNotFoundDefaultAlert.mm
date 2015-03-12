//
//  MWMRouteNotFoundDefaultAlert.m
//  Maps
//
//  Created by v.mikhaylenko on 12.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMRouteNotFoundDefaultAlert.h"
#import "MWMAlertViewController.h"
#import "MWMRouteNotFoundDefaultAlert+Configure.h"

@interface MWMRouteNotFoundDefaultAlert ()

@property (nonatomic, weak, readwrite) IBOutlet UILabel *messageLabel;
@property (nonatomic, weak, readwrite) IBOutlet UIButton *okButton;
@property (nonatomic, weak, readwrite) IBOutlet UIView *deviderLine;

@end

static NSString * const kRouteNotFoundDefaultAlertNibName = @"MWMRouteNotFoundDefaultAlert";

@implementation MWMRouteNotFoundDefaultAlert

+ (instancetype)alert {
  MWMRouteNotFoundDefaultAlert *alert = [[[NSBundle mainBundle] loadNibNamed:kRouteNotFoundDefaultAlertNibName owner:self options:nil] firstObject];
  [alert configure];
  return alert;
}

#pragma mark - Actions

- (IBAction)okButtonTap:(id)sender {
  [self.alertController close];
}

@end
