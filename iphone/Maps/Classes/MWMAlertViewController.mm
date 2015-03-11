//
//  MWMAlertViewController.m
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMAlertViewController.h"
#import "MWMDownloadTransitMapAlert.h"
#import "MWMAlertEntity.h"
#import "MWMAlertViewControllerDelegate.h"

static NSString * const kAlertControllerNibIdentifier = @"MWMAlertViewController";

@interface MWMAlertViewController ()
@property (nonatomic, weak, readwrite) UIViewController *ownerViewController;
@end

@implementation MWMAlertViewController

- (instancetype)initWithViewController:(UIViewController *)viewController {
  self = [super initWithNibName:kAlertControllerNibIdentifier bundle:nil];
  if (self) {
    self.ownerViewController = viewController;
  }
  return self;
}

- (void)viewDidLoad {
  [super viewDidLoad];
  self.view.frame = UIApplication.sharedApplication.delegate.window.bounds;
}

#pragma mark - Actions

- (void)presentAlertWithType:(MWMAlertType)type {
  MWMAlert *alert = [MWMAlert alertWithType:type];
  alert.alertController = self;
  switch (type) {
    case MWMAlertTypeDownloadTransitMap: {
      MWMAlertEntity *entity = [[MWMAlertEntity alloc] init];
      entity.contry = self.delegate.countryName;
      entity.size = self.delegate.size;
      [alert configureWithEntity:entity];
      break;
    }
    case MWMAlertTypeDownloadAllMaps:
      
      break;
  }
  [self.ownerViewController addChildViewController:self];
  self.view.center = self.ownerViewController.view.center;
  self.view.userInteractionEnabled = YES;
  [self.ownerViewController.view addSubview:self.view];
  [self.view addSubview:alert];
  alert.center = self.view.center;
  if ([[[UIDevice currentDevice] systemVersion] intValue] > 6) {
    [[[[UIApplication sharedApplication] delegate] window] addSubview:self.view];
  }
}

- (void)close {
  self.ownerViewController.view.userInteractionEnabled = YES;
  [self.view removeFromSuperview];
  [self removeFromParentViewController];
}

@end
