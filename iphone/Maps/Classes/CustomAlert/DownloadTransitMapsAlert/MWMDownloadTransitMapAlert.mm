//
//  MWMGetTransitionMapAlert.m
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMDownloadTransitMapAlert.h"
#import "MWMAlertViewController.h"
#import "ActiveMapsVC.h"
#import "UIKitCategories.h"

typedef void (^MWMDownloaderBlock)();

@interface MWMDownloadTransitMapAlert ()
@property (nonatomic, weak) IBOutlet UILabel *titleLabel;
@property (nonatomic, weak) IBOutlet UILabel *messageLabel;
@property (nonatomic, weak) IBOutlet UIButton *notNowButton;
@property (nonatomic, weak) IBOutlet UIButton *downloadButton;
@property (nonatomic, weak) IBOutlet UILabel *sizeLabel;
@property (nonatomic, weak) IBOutlet UILabel *countryLabel;
@property (nonatomic, weak) IBOutlet UIView *specsView;
@property (nonatomic, copy) MWMDownloaderBlock downloaderBlock;
@end

static NSString * const kDownloadTransitMapAlertNibName = @"MWMDownloadTransitMapAlert";
extern UIColor * const kActiveDownloaderViewColor;

@implementation MWMDownloadTransitMapAlert

+ (instancetype)alertWithCountryIndex:(const storage::TIndex)index {
  MWMDownloadTransitMapAlert *alert = [[[NSBundle mainBundle] loadNibNamed:kDownloadTransitMapAlertNibName owner:self options:nil] firstObject];
  ActiveMapsLayout& layout = GetFramework().GetCountryTree().GetActiveMapLayout();
  alert.countryLabel.text = [NSString stringWithUTF8String:layout.GetFormatedCountryName(index).c_str()];
  alert.sizeLabel.text = [NSString
      stringWithFormat:@"%@ %@",
                       @(layout.GetCountrySize(index, TMapOptions::EMapWithCarRouting).second /
                         (1024 * 1024)),
                       L(@"mb")];
  alert.downloaderBlock = ^{ layout.DownloadMap(index, TMapOptions::EMapWithCarRouting); };
  [alert configure];
  return alert;
}

#pragma mark - Actions

- (IBAction)notNowButtonTap:(id)sender {
  [self.alertController closeAlert];
}

- (IBAction)downloadButtonTap:(id)sender {
  [self downloadMaps];
}

- (IBAction)specsViewTap:(id)sender {
  [UIView animateWithDuration:0.2f animations:^{
    self.specsView.backgroundColor = kActiveDownloaderViewColor;
  } completion:^(BOOL finished) {
    [self downloadMaps];
  }];
}

- (void)downloadMaps {
  self.downloaderBlock();
  [self.alertController closeAlert];
  ActiveMapsVC *activeMapsViewController = [[ActiveMapsVC alloc] init];
  [self.alertController.ownerViewController.navigationController pushViewController:activeMapsViewController animated:YES];
}

#pragma mark - Configure

- (void)configure {
  [self.messageLabel sizeToFit];
  [self.titleLabel sizeToFit];
  [self.countryLabel sizeToFit];
  [self configureSpecsViewSize];
  [self configureMaintViewSize];
}

- (void)configureSpecsViewSize {
  const CGFloat topSpecsViewOffset = 16.;
  const CGFloat specsViewHeight = 2 * topSpecsViewOffset + self.countryLabel.frame.size.height;
  self.specsView.height = specsViewHeight;
  self.countryLabel.minY = topSpecsViewOffset;
  self.sizeLabel.center = CGPointMake(self.sizeLabel.center.x, self.countryLabel.center.y);
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

@end
