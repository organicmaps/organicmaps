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

  alert.messageLabel.userInteractionEnabled = YES;
  alert.messageLabel.attributedText = [self buildAlertMessage];
  alert.messageLabel.textAlignment = NSTextAlignmentCenter;
  UITapGestureRecognizer *tapGesture =
        [[UITapGestureRecognizer alloc] initWithTarget:alert
                                                action:@selector(showMoreInfo)];
  [alert.messageLabel addGestureRecognizer:tapGesture];

  return alert;
}

// Build attributet string in format "{alert_reauth_message_ios} {alert_reauth_link_text_ios}"
// where {alert_reauth_link_text_ios} has blue color as a link
+ (NSMutableAttributedString*)buildAlertMessage
{
  auto textAttrs = @{NSFontAttributeName : UIFont.regular17};
  auto linkAttrs = @{NSForegroundColorAttributeName : UIColor.linkBlue,
                     NSFontAttributeName : UIFont.regular17};

  NSMutableAttributedString *alertMessage =
    [[NSMutableAttributedString alloc] initWithString: L(@"alert_reauth_message_ios")
                                           attributes: textAttrs];
  // Add space char
  [alertMessage appendAttributedString:([[NSMutableAttributedString alloc] initWithString: @" "
                                                                               attributes: textAttrs])];
  NSAttributedString *alertLinkText =
    [[NSAttributedString alloc] initWithString: L(@"alert_reauth_link_text_ios")
                                    attributes: linkAttrs];
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
  NSLog(@"Navigate to URL: https://github.com/organicmaps/organicmaps/issues/6144");
}

- (IBAction)closeTap
{
  [self close:nil];
}

@end
