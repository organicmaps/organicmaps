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

@end

static NSString * const kRateAlertNibName = @"MWMRateAlert";

@implementation MWMRateAlert

+ (instancetype)alert
{
  MWMRateAlert * alert = [[[NSBundle mainBundle] loadNibNamed:kRateAlertNibName owner:self options:nil] firstObject];
  return alert;
}

#pragma mark - Actions

- (IBAction)oneStarTap:(UILongPressGestureRecognizer *)sender
{
  [UIView animateWithDuration:0.15 animations:^
  {
    self.oneStarPushImageView.alpha = 1.;
    self.oneStarButton.selected = YES;
  }
  completion:^(BOOL finished)
  {
    [UIView animateWithDuration:0.15 animations:^
    {
      self.oneStarPushImageView.alpha = 0.;
    }
    completion:^(BOOL finished)
    {
      [self presentAlertWithStarsCount:1];
    }];
  }];
}

- (IBAction)twoStarTap:(UILongPressGestureRecognizer *)sender
{
  [UIView animateWithDuration:0.15 animations:^
  {
    self.twoStarPushImageView.alpha = 1.;
    self.oneStarButton.selected = YES;
    self.twoStarButton.selected = YES;
  }
  completion:^(BOOL finished)
  {
    [UIView animateWithDuration:0.15 animations:^
    {
      self.twoStarPushImageView.alpha = 0.;
    }
    completion:^(BOOL finished)
    {
      [self presentAlertWithStarsCount:2];
    }];
  }];
}

- (IBAction)threeStarTap:(UILongPressGestureRecognizer *)sender
{
  [UIView animateWithDuration:0.15 animations:^
  {
    self.threeStarPushImageView.alpha = 1.;
    self.oneStarButton.selected = YES;
    self.twoStarButton.selected = YES;
    self.threeStarButton.selected = YES;
  }
  completion:^(BOOL finished)
  {
    [UIView animateWithDuration:0.15 animations:^
    {
      self.threeStarPushImageView.alpha = 0.;
    }
    completion:^(BOOL finished)
    {
      [self presentAlertWithStarsCount:3];
    }];
  }];
}

- (IBAction)fourStarTap:(UILongPressGestureRecognizer *)sender
{
  [UIView animateWithDuration:0.15 animations:^
  {
    self.fourStarPushImageView.alpha = 1.;
    self.oneStarButton.selected = YES;
    self.twoStarButton.selected = YES;
    self.threeStarButton.selected = YES;
    self.fourStarButton.selected = YES;
  }
  completion:^(BOOL finished)
  {
    [UIView animateWithDuration:0.15 animations:^
    {
      self.fourStarPushImageView.alpha = 0.;
    }
    completion:^(BOOL finished)
    {
      [self presentAlertWithStarsCount:4];
    }];
  }];
}

- (IBAction)fiveStarTap:(UILongPressGestureRecognizer *)sender
{
  [Alohalytics logEvent:kRateAlertEventName withValue:@"fiveStar"];
  [UIView animateWithDuration:0.15 animations:^
  {
    self.fiveStarPushImageView.alpha = 1.;
    self.oneStarButton.selected = YES;
    self.twoStarButton.selected = YES;
    self.threeStarButton.selected = YES;
    self.fourStarButton.selected = YES;
    self.fiveStarButton.selected = YES;
  }
  completion:^(BOOL finished)
  {
    [UIView animateWithDuration:0.15 animations:^
    {
      self.fiveStarPushImageView.alpha = 0.;
    }
    completion:^(BOOL finished)
    {
      dlg_settings::SaveResult(dlg_settings::AppStore, dlg_settings::OK);
      [[UIApplication sharedApplication] rateVersionFrom:@"ios_pro_popup"];
      [self close];
      [self setupAlreadyRatedInUserDefaults];
    }];
  }];
}

- (IBAction)notNowTap
{
  [Alohalytics logEvent:kRateAlertEventName withValue:@"notNowTap"];
  [self close];
}

- (void)presentAlertWithStarsCount:(NSUInteger)starsCount
{
  [self removeFromSuperview];
  [self.alertController presentFeedbackAlertWithStarsCount:starsCount];
  [self setupAlreadyRatedInUserDefaults];
}

- (void)setupAlreadyRatedInUserDefaults {
  [[NSUserDefaults standardUserDefaults] setBool:YES forKey:kUDAlreadyRatedKey];
  [[NSUserDefaults standardUserDefaults] synchronize];
}

@end
