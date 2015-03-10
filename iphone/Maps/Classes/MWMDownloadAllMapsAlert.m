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
  return alert;
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


/*
// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect {
    // Drawing code
}
*/

@end
