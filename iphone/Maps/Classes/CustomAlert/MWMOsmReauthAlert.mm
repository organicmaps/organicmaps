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

  //[alert.messageLabel setScrollEnabled:NO];
  alert.messageLabel.attributedText = [self buildAlertMessage];
  UITapGestureRecognizer *tapGesture =
        [[UITapGestureRecognizer alloc] initWithTarget:alert
                                                action:@selector(showMoreInfo)];
  [alert.messageLabel addGestureRecognizer:tapGesture];

  return alert;
}

+ (NSMutableAttributedString*)buildAlertMessage
{
  auto linkAttrs = @{NSForegroundColorAttributeName : UIColor.linkBlue};

  NSMutableAttributedString *alertMessage = [[NSMutableAttributedString alloc] initWithString:L(@"alert_reauth_message_ios")];
  [alertMessage appendAttributedString:([[NSAttributedString alloc] initWithString:@" "])];
  NSAttributedString *alertLinkText = [[NSAttributedString alloc] initWithString:L(@"alert_reauth_link_text_ios") attributes:linkAttrs];
  [alertMessage appendAttributedString:alertLinkText];
  return alertMessage;
}

- (IBAction)osmTap
{
  [self close:^{
    [self.alertController.ownerViewController performSegueWithIdentifier:kMap2OsmLoginSegue
                                                                  sender:nil];
  }];
}

- (IBAction)showMoreInfo
{
  NSLog(@"Navigate to URL: ");
}

- (IBAction)closeTap
{
  [self close:nil];
}

@end
