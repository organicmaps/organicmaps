//
//  MWMRateAlert.m
//  Maps
//
//  Created by v.mikhaylenko on 25.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMRateAlert.h"
#import "MWMAlertViewController.h"
#import "UIKitCategories.h"
#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "map/dialog_settings.hpp"

extern NSString * const kUDAlreadyRatedKey;
extern NSString * const kRateAlertEventName = @"rateAlertEvent";

@interface MWMRateAlert ()

@property (nonatomic, weak) IBOutlet UILabel *titleLabel;
@property (nonatomic, weak) IBOutlet UILabel *messageLabel;
@property (nonatomic, weak) IBOutlet UIButton *oneStarButton;
@property (nonatomic, weak) IBOutlet UIButton *twoStarButton;
@property (nonatomic, weak) IBOutlet UIButton *threeStarButton;
@property (nonatomic, weak) IBOutlet UIButton *fourStarButton;
@property (nonatomic, weak) IBOutlet UIButton *fiveStarButton;
@property (nonatomic, weak) IBOutlet UIImageView *oneStarPushImageView;
@property (nonatomic, weak) IBOutlet UIImageView *twoStarPushImageView;
@property (nonatomic, weak) IBOutlet UIImageView *threeStarPushImageView;
@property (nonatomic, weak) IBOutlet UIImageView *fourStarPushImageView;
@property (nonatomic, weak) IBOutlet UIImageView *fiveStarPushImageView;
@property (nonatomic, weak) IBOutlet UILabel *oneStarLabel;
@property (nonatomic, weak) IBOutlet UILabel *twoStarLabel;
@property (nonatomic, weak) IBOutlet UILabel *threeStarLabel;
@property (nonatomic, weak) IBOutlet UILabel *fourStarLabel;
@property (nonatomic, weak) IBOutlet UILabel *fiveStarLabel;
@property (nonatomic, weak) IBOutlet UIView *rateView;
@property (nonatomic, weak) IBOutlet UIButton *notNowBotton;
@property (nonatomic, weak) IBOutlet UIView *deviderView;

@end

static NSString * const kRateAlertNibName = @"MWMRateAlert";

@implementation MWMRateAlert

+ (instancetype)alert {
  MWMRateAlert *alert = [[[NSBundle mainBundle] loadNibNamed:kRateAlertNibName owner:self options:nil] firstObject];
  [alert configure];
  return alert;
}

#pragma mark - Actions

- (IBAction)oneStarTap:(UILongPressGestureRecognizer *)sender {
  [self starButtonLongTap:sender withBegan:^{
    [UIView animateWithDuration:0.35f animations:^{
      self.oneStarPushImageView.alpha = 1;
      self.oneStarButton.selected = YES;
    } completion:^(BOOL finished) {
      [UIView animateWithDuration:0.35f animations:^{
        self.oneStarPushImageView.alpha = 0.;
      }];
    }];
  } completion:^{
    [self presentAlertWithStarsCount:1];
  }];
}

- (IBAction)twoStarTap:(UILongPressGestureRecognizer *)sender {
  [self starButtonLongTap:sender withBegan:^{
    [UIView animateWithDuration:0.35f animations:^{
      self.twoStarPushImageView.alpha = 1;
      self.oneStarButton.selected = YES;
      self.twoStarButton.selected = YES;
    } completion:^(BOOL finished) {
      [UIView animateWithDuration:0.35f animations:^{
        self.twoStarPushImageView.alpha = 0.;
      }];
    }];
  } completion:^{
    [self presentAlertWithStarsCount:2];
  }];
}

- (IBAction)threeStarTap:(UILongPressGestureRecognizer *)sender {
  [self starButtonLongTap:sender withBegan:^{
    [UIView animateWithDuration:0.35f animations:^{
      self.threeStarPushImageView.alpha = 1.;
      self.oneStarButton.selected = YES;
      self.twoStarButton.selected = YES;
      self.threeStarButton.selected = YES;
    } completion:^(BOOL finished) {
      [UIView animateWithDuration:0.35f animations:^{
        self.threeStarPushImageView.alpha = 0.;
      }];
    }];
  } completion:^{
    [self presentAlertWithStarsCount:3];
  }];
}

- (IBAction)fourStarTap:(UILongPressGestureRecognizer *)sender {
  [self starButtonLongTap:sender withBegan:^{
    [UIView animateWithDuration:0.35f animations:^{
        self.fourStarPushImageView.alpha = 1.;
        self.oneStarButton.selected = YES;
        self.twoStarButton.selected = YES;
        self.threeStarButton.selected = YES;
        self.fourStarButton.selected = YES;
    } completion:^(BOOL finished) {
      [UIView animateWithDuration:0.35f animations:^{
        self.fourStarPushImageView.alpha = 0.;
      }];
    }];
  } completion:^{
    [self presentAlertWithStarsCount:4];
  }];
}

- (IBAction)fiveStarTap:(UILongPressGestureRecognizer *)sender {
  [Alohalytics logEvent:kRateAlertEventName withValue:@"fiveStar"];
  [self starButtonLongTap:sender withBegan:^{
    [UIView animateWithDuration:0.35f animations:^{
      self.fiveStarPushImageView.alpha = 1.;
      self.oneStarButton.selected = YES;
      self.twoStarButton.selected = YES;
      self.threeStarButton.selected = YES;
      self.fourStarButton.selected = YES;
      self.fiveStarButton.selected = YES;
    } completion:^(BOOL finished) {
      [UIView animateWithDuration:0.35f animations:^{
        self.fiveStarPushImageView.alpha = 0.;
      }];
    }];
  } completion:^{
    dlg_settings::SaveResult(dlg_settings::AppStore, dlg_settings::OK);
    [[UIApplication sharedApplication] rateVersionFrom:@"ios_pro_popup"];
    [self.alertController closeAlert];
    [self setupAlreadyRatedInUserDefaults];
  }];
}

- (IBAction)notNowTap:(UILongPressGestureRecognizer *)sender {
  [Alohalytics logEvent:kRateAlertEventName withValue:@"notNowTap"];
  [self.alertController closeAlert];
}

- (void)presentAlertWithStarsCount:(NSUInteger)starsCount {
  [self removeFromSuperview];
  [self.alertController presentFeedbackAlertWithStarsCount:starsCount];
  [self setupAlreadyRatedInUserDefaults];
}

- (void)setupAlreadyRatedInUserDefaults {
  [[NSUserDefaults standardUserDefaults] setBool:YES forKey:kUDAlreadyRatedKey];
  [[NSUserDefaults standardUserDefaults] synchronize];
}

- (void)starButtonLongTap:(UILongPressGestureRecognizer *) tap withBegan:(void (^)())began completion:(void (^)())completion {
  switch (tap.state) {
    case UIGestureRecognizerStateBegan:
      began();
      break;
    
    case UIGestureRecognizerStateEnded:
      completion();
      break;
      
    default:
      break;
  }
}

#pragma mark - Configure

- (void)configure {
  self.oneStarPushImageView.alpha = 0.;
  self.twoStarPushImageView.alpha = 0.;
  self.threeStarPushImageView.alpha = 0.;
  self.fourStarPushImageView.alpha = 0.;
  self.fiveStarPushImageView.alpha = 0.;
  [self.titleLabel sizeToFit];
  [self.messageLabel sizeToFit];
  CGFloat const topMainViewOffset = 17.;
  CGFloat const secondMainViewOffset = 14.;
  CGFloat const thirdMainViewOffset = 20.;
  CGFloat const minMainViewHeight = 144. + self.deviderView.height + self.notNowBotton.height;
  CGFloat const actualMainViewHeight = topMainViewOffset + secondMainViewOffset + (2 * thirdMainViewOffset) + self.titleLabel.height + self.messageLabel.height + self.rateView.height + self.deviderView.height + self.notNowBotton.height;
  self.height = actualMainViewHeight >= minMainViewHeight ? actualMainViewHeight : minMainViewHeight;
  self.titleLabel.minY = topMainViewOffset;
  self.messageLabel.minY = self.titleLabel.maxY + secondMainViewOffset;
  self.notNowBotton.minY = self.height - self.notNowBotton.height;
  self.deviderView.minY = self.notNowBotton.minY - 1;
  self.rateView.minY = self.messageLabel.maxY + thirdMainViewOffset;
}

@end
