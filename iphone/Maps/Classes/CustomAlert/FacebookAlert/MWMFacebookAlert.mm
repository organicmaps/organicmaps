#import "MWMAlertViewController.h"
#import "MWMFacebookAlert.h"
#import "Statistics.h"
#import <FBSDKShareKit/FBSDKShareKit.h>

#import "3party/Alohalytics/src/alohalytics_objc.h"

static NSString * const kFacebookAlertNibName = @"MWMFacebookAlert";
static NSString * const kFacebookAppName = @"https://fb.me/958251974218933";
static NSString * const kFacebookURL = @"http://www.facebook.com/MapsWithMe";
static NSString * const kFacebookScheme = @"fb://profile/111923085594432";
static NSString * const kFacebookAlertPreviewImage = @"http://maps.me/images/fb_app_invite_banner.png";
static NSString * const kFacebookInviteEventName = @"facebookInviteEvent";
extern NSString * const kUDAlreadySharedKey;

static NSString * const kStatisticsEvent = @"Facebook Alert";

@implementation MWMFacebookAlert

+ (MWMFacebookAlert *)alert
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatOpen}];
  MWMFacebookAlert * alert =
      [NSBundle.mainBundle loadNibNamed:kFacebookAlertNibName owner:self options:nil].firstObject;
  return alert;
}

#pragma mark - Actions

- (IBAction)shareButtonTap:(id)sender
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatApply}];
  [Alohalytics logEvent:kFacebookInviteEventName withValue:@"shareTap"];
  [self close:nil];
  [NSUserDefaults.standardUserDefaults setBool:YES forKey:kUDAlreadySharedKey];
  [NSUserDefaults.standardUserDefaults synchronize];

  FBSDKAppInviteContent * const content = [[FBSDKAppInviteContent alloc] init];
  content.appLinkURL = [NSURL URLWithString:kFacebookAppName];
  content.appInvitePreviewImageURL = [NSURL URLWithString:kFacebookAlertPreviewImage];
  [FBSDKAppInviteDialog showFromViewController:self.alertController.ownerViewController withContent:content delegate:nil];
}

- (IBAction)notNowButtonTap:(id)sender
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatClose}];
  [Alohalytics logEvent:kFacebookInviteEventName withValue:@"notNowTap"];
  [self close:nil];
}

@end
