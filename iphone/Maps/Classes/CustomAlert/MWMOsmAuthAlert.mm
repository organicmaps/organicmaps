#import "MWMAlertViewController.h"
#import "MWMOsmAuthAlert.h"
#import "Statistics.h"

#include "editor/osm_auth.hpp"

static NSString * const kMap2OsmLoginSegue = @"Map2OsmLogin";

extern NSString * const kMap2FBLoginSegue;
extern NSString * const kMap2GoogleLoginSegue;

@implementation MWMOsmAuthAlert

+ (instancetype)alert
{
  MWMOsmAuthAlert * alert =
      [NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil].firstObject;
  return alert;
}

- (IBAction)facebookTap
{
  [Statistics logEvent:kStatEditorAuthRequets withParameters:@{kStatValue : kStatFacebook, kStatFrom : kStatEdit}];
  [self close:^{
    [self.alertController.ownerViewController performSegueWithIdentifier:kMap2FBLoginSegue
                                                                  sender:nil];
  }];
}

- (IBAction)googleTap
{
  [Statistics logEvent:kStatEditorAuthRequets withParameters:@{kStatValue : kStatGoogle, kStatFrom : kStatEdit}];
  [self close:^{
    [self.alertController.ownerViewController performSegueWithIdentifier:kMap2GoogleLoginSegue
                                                                  sender:nil];
  }];
}

- (IBAction)osmTap
{
  [Statistics logEvent:kStatEditorAuthRequets withParameters:@{kStatValue : kStatOSM, kStatFrom : kStatEdit}];
  [self close:^{
    [self.alertController.ownerViewController performSegueWithIdentifier:kMap2OsmLoginSegue
                                                                  sender:nil];
  }];
}

- (IBAction)signUpTap
{
  [self close:^{
    [Statistics logEvent:kStatEditorRegRequest withParameters:@{kStatFrom : kStatEdit}];
    NSURL * url = [NSURL URLWithString:@(osm::OsmOAuth::ServerAuth().GetRegistrationURL().c_str())];
    [self.alertController.ownerViewController openUrl:url];
  }];
}

- (IBAction)closeTap
{
  [Statistics logEvent:kStatEditorAuthDeclinedByUser];
  [self close:nil];
}

@end
