//
//  MWMAlertViewController.m
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMAlertViewController.h"
#import "MWMDownloadAllMapsAlert.h"
#import "MWMAlertEntity.h"

static NSString * const kAlertControllerNibIdentifier = @"MWMAlertViewController";

@interface MWMAlertViewController ()
@property (nonatomic, weak) UIViewController *ownerViewController;
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
  self.view.frame = self.ownerViewController.view.bounds;
}

#pragma mark - Actions

- (void)presentAlertWithType:(MWMAlertType)type {
  MWMAlert *alert = [MWMAlert alertWithType:type];
  alert.alertController = self;
  MWMAlertEntity *entity = [[MWMAlertEntity alloc] init];
  entity.title = @"Download all maps";
  entity.message = @"hihihihihihihihihi hihihihihi hihihihihih ihihihihihihi hihihihih ihihihihih ihihihihihihihihi hihihihihihihihi hihihihihihihi hihihihihihihi hihihihihihi hihihihihi hihihihihi hihihihihi hihihihihi";
  entity.contry = @"adbsgndlkgndklsnfkldsnglfdksjnglkdfsgjn";
  entity.location = @"dkjsgnkdsnjglkdsng";
  entity.size = @"1000";
  
  [alert configureWithEntity:entity];
  [self.view addSubview:alert];
  alert.center = self.view.center;
  [self.ownerViewController addChildViewController:self];
  self.view.center = self.ownerViewController.view.center;
  self.view.userInteractionEnabled = YES;
  [self.ownerViewController.view addSubview:self.view];
  [[[[UIApplication sharedApplication] delegate] window] addSubview:self.view];
}

- (void)close {
  self.ownerViewController.view.userInteractionEnabled = YES;
  [self.view removeFromSuperview];
  [self.view removeFromSuperview];
  [self removeFromParentViewController];
}


@end
