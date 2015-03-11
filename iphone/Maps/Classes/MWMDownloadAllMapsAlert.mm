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

@interface MWMDownloadAllMapsAlert ()
@property (nonatomic, weak) IBOutlet UIView *specsView;
@property (nonatomic, weak) IBOutlet UILabel *titleLabel;
@property (nonatomic, weak) IBOutlet UILabel *messageLabel;
@property (nonatomic, weak) IBOutlet UIButton *notNowButton;
@property (nonatomic, weak) IBOutlet UIButton *downloadButton;
@property (nonatomic, weak) IBOutlet UILabel *downloadMapsLabel;
@end

static NSString * const kDownloadAllMapsAlertNibName = @"MWMDownloadAllMapsAlert";

@implementation MWMDownloadAllMapsAlert

+ (instancetype)alert {
  MWMDownloadAllMapsAlert *alert = [[[NSBundle mainBundle] loadNibNamed:kDownloadAllMapsAlertNibName owner:self options:nil] firstObject];
  [alert configureViewSize];
  return alert;
}

- (void)configureViewSize {
  [self.titleLabel sizeToFit];
  [self.messageLabel sizeToFit];
  [self configureMainViewSize];
}

- (void)configureMainViewSize {
  const CGFloat topMainViewOffset = 17.;
  const CGFloat secondMainViewOffset = 14.;
  const CGFloat thirdMainViewOffset = 20.;
  const CGFloat bottomMainViewOffset = 52.;
  const CGFloat mainViewHeight = topMainViewOffset + self.titleLabel.height + secondMainViewOffset + self.messageLabel.height + thirdMainViewOffset + self.specsView.height + bottomMainViewOffset;
  self.autoresizesSubviews = YES;
  self.height = mainViewHeight;
  self.titleLabel.minY = topMainViewOffset;
  self.messageLabel.minY = self.titleLabel.minY + self.titleLabel.height + secondMainViewOffset;
  self.specsView.minY = self.messageLabel.minY + self.messageLabel.height + thirdMainViewOffset;
  self.notNowButton.minY = self.specsView.minY + self.specsView.height;
}

- (IBAction)notNowButtonTap:(id)sender {
  [self.alertController close];
}

- (IBAction)downloadMapsButtonTap:(id)sender {
  [UIView animateWithDuration:0.2f animations:^{
    self.specsView.backgroundColor = [UIColor colorWithRed:211/255. green:209/255. blue:205/255. alpha:1.];
  } completion:^(BOOL finished) {
    [self.alertController close];
    CountryTreeVC *viewController = [[CountryTreeVC alloc] initWithNodePosition:-1];
    [self.alertController.ownerViewController.navigationController pushViewController:viewController animated:YES];
  }];
}

@end
