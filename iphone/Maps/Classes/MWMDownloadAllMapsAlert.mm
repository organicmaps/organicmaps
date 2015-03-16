//
//  MWMDownloadAllMapsAlert.m
//  Maps
//
//  Created by v.mikhaylenko on 10.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMDownloadAllMapsAlert.h"
#import "MWMAlertViewController.h"
#import "CountryTreeVC.h"
#import "MWMDownloadAllMapsAlert+Configure.h"

@interface MWMDownloadAllMapsAlert ()
@property (nonatomic, weak, readwrite) IBOutlet UIView *specsView;
@property (nonatomic, weak, readwrite) IBOutlet UILabel *titleLabel;
@property (nonatomic, weak, readwrite) IBOutlet UILabel *messageLabel;
@property (nonatomic, weak, readwrite) IBOutlet UIButton *notNowButton;
@property (nonatomic, weak, readwrite) IBOutlet UIButton *downloadButton;
@property (nonatomic, weak, readwrite) IBOutlet UILabel *downloadMapsLabel;
@end

static NSString * const kDownloadAllMapsAlertNibName = @"MWMDownloadAllMapsAlert";
extern UIColor * const kActiveDownloaderViewColor;
static NSInteger const kNodePositionForDownloadMaps = -1;

@implementation MWMDownloadAllMapsAlert

+ (instancetype)alert {
  MWMDownloadAllMapsAlert *alert = [[[NSBundle mainBundle] loadNibNamed:kDownloadAllMapsAlertNibName owner:self options:nil] firstObject];
  [alert configure];
  return alert;
}

#pragma mark - Actions

- (IBAction)notNowButtonTap:(id)sender {
  [self.alertController closeAlert];
}

- (IBAction)downloadMapsButtonTap:(id)sender {
  [UIView animateWithDuration:0.2f animations:^{
    self.specsView.backgroundColor = kActiveDownloaderViewColor;
  } completion:^(BOOL finished) {
    [self.alertController closeAlert];
    CountryTreeVC *viewController = [[CountryTreeVC alloc] initWithNodePosition:kNodePositionForDownloadMaps];
    [self.alertController.ownerViewController.navigationController pushViewController:viewController animated:YES];
  }];
}

@end
