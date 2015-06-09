//
//  MWMFacebookAlert.m
//  Maps
//
//  Created by v.mikhaylenko on 01.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMFacebookAlert.h"
#import "MWMAlertViewController.h"
#import "UIKitCategories.h"
#import <FBSDKShareKit/FBSDKShareKit.h>

#import "Statistics.h"
#import "3party/Alohalytics/src/alohalytics_objc.h"

static NSString * const kFacebookAlertNibName = @"MWMFacebookAlert";
static NSString * const kFacebookAppName = @"https://fb.me/958251974218933";
static NSString * const kFacebookURL = @"http://www.facebook.com/MapsWithMe";
static NSString * const kFacebookScheme = @"fb://profile/111923085594432";
static NSString * const kFacebookAlertPreviewImage = @"http://maps.me/images/fb_app_invite_banner.png";
static NSString * const kFacebookInviteEventName = @"facebookInviteEvent";
extern NSString * const kUDAlreadySharedKey;

@interface MWMFacebookAlert ()

@property (nonatomic, weak) IBOutlet UILabel *titleLabel;
@property (nonatomic, weak) IBOutlet UILabel *messageLabel;
@property (nonatomic, weak) IBOutlet UIButton *notNowButton;
@property (nonatomic, weak) IBOutlet UIButton *shareButton;

@end

@implementation MWMFacebookAlert

+ (MWMFacebookAlert *)alert
{
  MWMFacebookAlert *alert = [[[NSBundle mainBundle] loadNibNamed:kFacebookAlertNibName owner:self options:nil] firstObject];
  [alert configure];
  return alert;
}

#pragma mark - Actions

- (IBAction)shareButtonTap:(id)sender
{
  [Alohalytics logEvent:kFacebookInviteEventName withValue:@"shareTap"];
  [[Statistics instance] logEvent:[NSString stringWithFormat:@"%@ShareTap", kFacebookInviteEventName]];
  [self.alertController closeAlert];
  [[NSUserDefaults standardUserDefaults] setBool:YES forKey:kUDAlreadySharedKey];
  [[NSUserDefaults standardUserDefaults] synchronize];

  FBSDKAppInviteContent * const content = [[FBSDKAppInviteContent alloc] init];
  content.appLinkURL = [NSURL URLWithString:kFacebookAppName];
  content.previewImageURL = [NSURL URLWithString:kFacebookAlertPreviewImage];
  [FBSDKAppInviteDialog showWithContent:content delegate:nil];
}

- (IBAction)notNowButtonTap:(id)sender
{
  [Alohalytics logEvent:kFacebookInviteEventName withValue:@"notNowTap"];
  [[Statistics instance] logEvent:[NSString stringWithFormat:@"%@NotNowTap", kFacebookInviteEventName]];
  [self.alertController closeAlert];
}

#pragma mark - Configure

- (void)configure
{
  CGFloat const topMainViewOffset = 17.;
  CGFloat const secondMainViewOffset = 14.;
  CGFloat const thirdMainViewOffset = 20.;
  [self.titleLabel sizeToFit];
  [self.messageLabel sizeToFit];
  CGFloat const mainViewHeight = topMainViewOffset + secondMainViewOffset + thirdMainViewOffset + self.titleLabel.height + self.messageLabel.height + self.notNowButton.height;
  self.height = mainViewHeight;
  self.titleLabel.minY = topMainViewOffset;
  self.messageLabel.minY = self.titleLabel.maxY + secondMainViewOffset;
  self.notNowButton.minY = self.messageLabel.maxY + thirdMainViewOffset;
  self.shareButton.minY = self.notNowButton.minY;
}

@end
