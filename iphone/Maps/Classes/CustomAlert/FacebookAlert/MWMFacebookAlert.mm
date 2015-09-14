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

@implementation MWMFacebookAlert

+ (MWMFacebookAlert *)alert
{
  [Statistics.instance logEvent:[NSString stringWithFormat:@"%@ - %@", kFacebookInviteEventName, @"open"]];
  MWMFacebookAlert * alert = [[[NSBundle mainBundle] loadNibNamed:kFacebookAlertNibName owner:self options:nil] firstObject];
  return alert;
}

#pragma mark - Actions

- (IBAction)shareButtonTap:(id)sender
{
  [Alohalytics logEvent:kFacebookInviteEventName withValue:@"shareTap"];
  [[Statistics instance] logEvent:[NSString stringWithFormat:@"%@ShareTap", kFacebookInviteEventName]];
  [self close];
  [[NSUserDefaults standardUserDefaults] setBool:YES forKey:kUDAlreadySharedKey];
  [[NSUserDefaults standardUserDefaults] synchronize];

  FBSDKAppInviteContent * const content = [[FBSDKAppInviteContent alloc] init];
  content.appLinkURL = [NSURL URLWithString:kFacebookAppName];
  content.appInvitePreviewImageURL = [NSURL URLWithString:kFacebookAlertPreviewImage];
  [FBSDKAppInviteDialog showWithContent:content delegate:nil];
}

- (IBAction)notNowButtonTap:(id)sender
{
  [Alohalytics logEvent:kFacebookInviteEventName withValue:@"notNowTap"];
  [[Statistics instance] logEvent:[NSString stringWithFormat:@"%@NotNowTap", kFacebookInviteEventName]];
  [self close];
}

@end
