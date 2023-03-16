#import "MWMAlertViewController.h"
#import "MWMOsmAuthAlert.h"

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
  [self close:^{
    [self.alertController.ownerViewController performSegueWithIdentifier:kMap2FBLoginSegue
                                                                  sender:nil];
  }];
}

- (IBAction)googleTap
{
  [self close:^{
    [self.alertController.ownerViewController performSegueWithIdentifier:kMap2GoogleLoginSegue
                                                                  sender:nil];
  }];
}

- (IBAction)osmTap
{
  [self close:^{
    [self.alertController.ownerViewController performSegueWithIdentifier:kMap2OsmLoginSegue
                                                                  sender:nil];
  }];
}

- (IBAction)signUpTap
{
  [self close:^{
    [self.alertController.ownerViewController openUrl:@(osm::OsmOAuth::ServerAuth().GetRegistrationURL().c_str())];
  }];
}

- (IBAction)closeTap
{
  [self close:nil];
}

@end
