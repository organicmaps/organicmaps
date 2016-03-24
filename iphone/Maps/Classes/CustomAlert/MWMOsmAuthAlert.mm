#import "MWMAlertViewController.h"
#import "MWMOsmAuthAlert.h"
#import "Statistics.h"

#include "editor/osm_auth.hpp"

extern NSString * const kMap2OsmLoginSegue;
extern NSString * const kMap2FBLoginSegue;
extern NSString * const kMap2GoogleLoginSegue;

@implementation MWMOsmAuthAlert

+ (instancetype)alert
{
  MWMOsmAuthAlert * alert = [[[NSBundle mainBundle] loadNibNamed:[MWMOsmAuthAlert className] owner:nil options:nil] firstObject];
  return alert;
}

- (IBAction)facebookTap
{
  [Statistics logEvent:kStatEditorAuthRequets withParameters:@{kStatValue : kStatFacebook}];
  [self close];
  [self.alertController.ownerViewController performSegueWithIdentifier:kMap2FBLoginSegue sender:nil];
}

- (IBAction)googleTap
{
  [Statistics logEvent:kStatEditorAuthRequets withParameters:@{kStatValue : kStatGoogle}];
  [self close];
  [self.alertController.ownerViewController performSegueWithIdentifier:kMap2GoogleLoginSegue sender:nil];
}

- (IBAction)osmTap
{
  [Statistics logEvent:kStatEditorAuthRequets withParameters:@{kStatValue : kStatOSM}];
  [self close];
  [self.alertController.ownerViewController performSegueWithIdentifier:kMap2OsmLoginSegue sender:nil];
}

- (IBAction)signUpTap
{
  [self close];
  [Statistics logEvent:kStatEditorRegRequest];
  NSURL * url = [NSURL URLWithString:@(osm::OsmOAuth::ServerAuth().GetRegistrationURL().c_str())];
  [[UIApplication sharedApplication] openURL:url];
}

- (IBAction)closeTap
{
  [Statistics logEvent:kStatEditorAuthDeclinedByUser];
  [self close];
}

@end
