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
#import "UIKitCategories.h"
#import "MWMAlertViewControllerDelegate.h"
#import "ActiveMapsVC.h"

@interface MWMDownloadTransitMapAlert ()
@property (nonatomic, weak) IBOutlet UILabel *titleLabel;
@property (nonatomic, weak) IBOutlet UILabel *messageLabel;
@property (nonatomic, weak) IBOutlet UIButton *notNowButton;
@property (nonatomic, weak) IBOutlet UIButton *downloadButton;
@property (nonatomic, weak) IBOutlet UILabel *sizeLabel;
@property (nonatomic, weak) IBOutlet UILabel *locationLabel;
@property (nonatomic, weak) IBOutlet UILabel *countryLabel;
@property (nonatomic, weak) IBOutlet UIView *specsView;
@end

static NSString * kDownloadTransitMapAlertNibName = @"MWMDownloadTransitMapAlert";

@implementation MWMDownloadTransitMapAlert

+ (instancetype)alert {
  MWMDownloadTransitMapAlert *alert = [[[NSBundle mainBundle] loadNibNamed:kDownloadTransitMapAlertNibName owner:self options:nil] firstObject];
  return alert;
}

#pragma mark - Configure

- (void)configureWithEntity:(MWMAlertEntity *)entity {
  self.sizeLabel.text = [NSString stringWithFormat:@"%@ MB",@(entity.size)];
  self.locationLabel.text = entity.location;
  self.countryLabel.text = entity.contry;
  [self configureViewSize];
}

- (void)configureViewSize {
  [self.messageLabel sizeToFit];
  [self.titleLabel sizeToFit];
  [self.countryLabel sizeToFit];
  [self.locationLabel sizeToFit];
  [self configureSpecsViewSize];
  [self configureMaintViewSize];
}

- (void)configureSpecsViewSize {
  const CGFloat topSpecsViewOffset = 12.;
  const CGFloat bottonSpecsViewOffset = 10.;
  const CGFloat middleSpecsViewOffset = 10.;
  const CGFloat specsViewHeight = topSpecsViewOffset + bottonSpecsViewOffset + middleSpecsViewOffset + self.countryLabel.frame.size.height + self.locationLabel.frame.size.height;
  self.specsView.height = specsViewHeight;
  self.locationLabel.minY = topSpecsViewOffset;
  self.countryLabel.minY = self.locationLabel.frame.origin.y + self.locationLabel.frame.size.height + middleSpecsViewOffset;
}

- (void)configureMaintViewSize {
  const CGFloat topMainViewOffset = 17.;
  const CGFloat secondMainViewOffset = 14.;
  const CGFloat thirdMainViewOffset = 20.;
  const CGFloat bottomMainViewOffset = 52.;
  const CGFloat mainViewHeight = topMainViewOffset + self.titleLabel.frame.size.height + secondMainViewOffset + self.messageLabel.frame.size.height + thirdMainViewOffset + self.specsView.frame.size.height + bottomMainViewOffset;
  self.height = mainViewHeight;
  self.titleLabel.minY = topMainViewOffset;
  self.messageLabel.minY = self.titleLabel.frame.origin.y + self.titleLabel.frame.size.height + secondMainViewOffset;
  self.specsView.minY = self.messageLabel.frame.origin.y + self.messageLabel.frame.size.height + thirdMainViewOffset;
  self.notNowButton.minY = self.specsView.frame.origin.y + self.specsView.frame.size.height;
  self.downloadButton.minY = self.notNowButton.frame.origin.y;
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




