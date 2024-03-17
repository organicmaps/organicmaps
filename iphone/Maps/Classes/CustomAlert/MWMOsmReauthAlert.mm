#import "MWMAlertViewController.h"
#import "MWMOsmReauthAlert.h"
#import "MWMAuthorizationCommon.h"

#include "editor/osm_auth.hpp"

static NSString * const kMap2OsmLoginSegue = @"Map2OsmLogin";

@implementation MWMOsmReauthAlert

+ (instancetype)alert
{
  MWMOsmReauthAlert * alert =
      [NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil].firstObject;
  return alert;
}

- (IBAction)osmTap
{
  [self close:^{
    [self.alertController.ownerViewController performSegueWithIdentifier:kMap2OsmLoginSegue
                                                                  sender:nil];
  }];
}

- (IBAction)closeTap
{
  [self close:nil];
}

@end
