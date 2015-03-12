//
//  MWMGetTransitionMapAlert.m
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMDownloadTransitMapAlert.h"
#import "MWMAlertViewController.h"
#import "MWMAlertEntity.h"
#import "MWMAlertViewControllerDelegate.h"
#import "ActiveMapsVC.h"
#import "MWMDownloadTransitMapAlert+Configure.h"

@interface MWMDownloadTransitMapAlert ()
@property (nonatomic, weak, readwrite) IBOutlet UILabel *titleLabel;
@property (nonatomic, weak, readwrite) IBOutlet UILabel *messageLabel;
@property (nonatomic, weak, readwrite) IBOutlet UIButton *notNowButton;
@property (nonatomic, weak, readwrite) IBOutlet UIButton *downloadButton;
@property (nonatomic, weak, readwrite) IBOutlet UILabel *sizeLabel;
@property (nonatomic, weak, readwrite) IBOutlet UILabel *countryLabel;
@property (nonatomic, weak, readwrite) IBOutlet UIView *specsView;
@end

static NSString * kDownloadTransitMapAlertNibName = @"MWMDownloadTransitMapAlert";

@implementation MWMDownloadTransitMapAlert

+ (instancetype)alert {
  MWMDownloadTransitMapAlert *alert = [[[NSBundle mainBundle] loadNibNamed:kDownloadTransitMapAlertNibName owner:self options:nil] firstObject];
  return alert;
}

- (void)configureWithEntity:(MWMAlertEntity *)entity {
  self.sizeLabel.text = [NSString stringWithFormat:@"%@ MB",@(entity.size)];
  self.countryLabel.text = entity.country;
  [self configure];
}

#pragma mark - Actions

- (IBAction)notNowButtonTap:(id)sender {
  [self.alertController close];
}

- (IBAction)downloadButtonTap:(id)sender {
  [self downloadMaps];
}

- (IBAction)specsViewTap:(id)sender {
  [UIView animateWithDuration:0.2f animations:^{
    self.specsView.backgroundColor = [UIColor colorWithRed:211/255. green:209/255. blue:205/255. alpha:1.];
  } completion:^(BOOL finished) {
    [self downloadMaps];
  }];
}

- (void)downloadMaps {
  [self.alertController.delegate downloadMaps];
  [self.alertController close];
  ActiveMapsVC *activeMapsViewController = [[ActiveMapsVC alloc] init];
  [self.alertController.ownerViewController.navigationController pushViewController:activeMapsViewController animated:YES];
}

@end




