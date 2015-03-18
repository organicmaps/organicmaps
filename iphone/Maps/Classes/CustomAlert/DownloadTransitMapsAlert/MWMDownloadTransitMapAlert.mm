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
#import "MWMDownloadTransitMapAlert+Configure.h"

typedef void (^MWMDownloaderBlock)();

@interface MWMDownloadTransitMapAlert ()
@property (nonatomic, weak, readwrite) IBOutlet UILabel *titleLabel;
@property (nonatomic, weak, readwrite) IBOutlet UILabel *messageLabel;
@property (nonatomic, weak, readwrite) IBOutlet UIButton *notNowButton;
@property (nonatomic, weak, readwrite) IBOutlet UIButton *downloadButton;
@property (nonatomic, weak, readwrite) IBOutlet UILabel *sizeLabel;
@property (nonatomic, weak, readwrite) IBOutlet UILabel *countryLabel;
@property (nonatomic, weak, readwrite) IBOutlet UIView *specsView;
@property (nonatomic, copy) MWMDownloaderBlock downloaderBlock;
@end

static NSString * const kDownloadTransitMapAlertNibName = @"MWMDownloadTransitMapAlert";
extern UIColor * const kActiveDownloaderViewColor;

@implementation MWMDownloadTransitMapAlert

+ (instancetype)alertWithCountrieIndex:(const storage::TIndex)index {
  MWMDownloadTransitMapAlert *alert = [[[NSBundle mainBundle] loadNibNamed:kDownloadTransitMapAlertNibName owner:self options:nil] firstObject];
  ActiveMapsLayout& layout = GetFramework().GetCountryTree().GetActiveMapLayout();
  alert.countryLabel.text = [NSString stringWithUTF8String:layout.GetFormatedCountryName(index).c_str()];
  alert.sizeLabel.text = [NSString stringWithFormat:@"%@ %@", @(layout.GetCountrySize(index, storage::TMapOptions::EMapWithCarRouting).second/(1024 * 1024)), L(@"mb")];
  alert.downloaderBlock = ^{
    layout.DownloadMap(index, storage::TMapOptions::EMapWithCarRouting);
  };
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

@end