#import "MWMRateAlert.h"
#import "AppInfo.h"
#import "MWMAlertViewController.h"
#import "MWMMailViewController.h"
#import "Statistics.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "platform/platform.hpp"

extern NSString * const kUDAlreadyRatedKey;
extern NSString * const kLocaleUsedInSupportEmails;
extern NSString * const kRateAlertEventName = @"rateAlertEvent";
static NSString * const kRateAlertNibName = @"MWMRateAlert";
static NSString * const kRateEmail = @"rating@maps.me";

static NSString * const kStatisticsEvent = @"Rate Alert";

@interface MWMRateAlert ()<MFMailComposeViewControllerDelegate>

@property(nonatomic) IBOutletCollection(UIButton) NSArray * buttons;
@property(nonatomic, weak) IBOutlet UIButton * rateButton;
@property(nonatomic, weak) IBOutlet UILabel * title;
@property(nonatomic, weak) IBOutlet UILabel * message;
@property(nonatomic) NSUInteger selectedTag;

@end

@implementation MWMRateAlert

+ (instancetype)alert
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatOpen}];
  MWMRateAlert * alert =
      [NSBundle.mainBundle loadNibNamed:kRateAlertNibName owner:self options:nil].firstObject;
  [alert configureButtons];
  return alert;
}

#pragma mark - Actions

- (void)configureButtons
{
  UIImage * i = [self.buttons.firstObject imageForState:UIControlStateSelected];
  for (UIButton * b in self.buttons)
    [b setImage:i forState:(UIControlStateHighlighted | UIControlStateSelected)];
}

- (IBAction)starTap:(UIButton *)sender
{
  if (!self.rateButton.enabled)
  {
    self.rateButton.enabled = YES;
    auto color = UIColor.linkBlue;
    self.rateButton.layer.borderColor = color.CGColor;
    [self.rateButton setTitleColor:color forState:UIControlStateNormal];
  }

  NSUInteger const tag = sender.tag;
  if (tag == 5)
  {
    [self.rateButton setTitle:L(@"rate_the_app") forState:UIControlStateNormal];
    self.message.text = L(@"rate_alert_five_star_message");
  }
  else
  {
    if (tag == 4)
      self.message.text = L(@"rate_alert_four_star_message");
    else
      self.message.text = L(@"rate_alert_less_than_four_star_message");
    [self.rateButton setTitle:L(@"leave_a_review") forState:UIControlStateNormal];
  }
  [self setNeedsLayout];
  for (UIButton * b in self.buttons)
  {
    if (b.tag > tag)
      b.selected = NO;
    else
      b.selected = YES;
  }
  self.selectedTag = tag;
}

- (IBAction)starHighlighted:(UIButton *)sender { [self setHighlighted:sender.tag]; }
- (IBAction)starTouchCanceled
{
  for (UIButton * b in self.buttons)
    b.highlighted = NO;
}

- (IBAction)starDragInside:(UIButton *)sender { [self setHighlighted:sender.tag]; }
- (void)setHighlighted:(NSUInteger)tag
{
  for (UIButton * b in self.buttons)
  {
    if (b.tag > tag)
      b.highlighted = NO;
    else
      b.highlighted = !b.selected;
  }
}

- (IBAction)doneTap
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatClose}];
  [Alohalytics logEvent:kRateAlertEventName withValue:@"notNowTap"];
  [self close:nil];
}

- (IBAction)rateTap
{
  NSUInteger const tag = self.selectedTag;
  [Statistics logEvent:kStatEventName(kStatisticsEvent, kStatRate)
        withParameters:@{
          kStatValue : @(tag).stringValue
        }];
  if (tag == 5)
  {
    [UIApplication.sharedApplication rateVersionFrom:@"ios_pro_popup"];
    [Alohalytics logEvent:kRateAlertEventName withValue:@"fiveStar"];
    [self close:^{
      auto ud = NSUserDefaults.standardUserDefaults;
      [ud setBool:YES forKey:kUDAlreadyRatedKey];
      [ud synchronize];
    }];
  }
  else
  {
    [self sendFeedback];
  }
}

- (void)sendFeedback
{
  [Statistics logEvent:kStatEventName(kStatisticsEvent, kStatSendEmail)];
  [Alohalytics logEvent:kRateAlertEventName withValue:@"sendFeedback"];
  self.alpha = 0.;
  MWMAlertViewController * alertController = self.alertController;
  alertController.view.alpha = 0.;
  if ([MWMMailViewController canSendMail])
  {
    NSString * deviceModel = [AppInfo sharedInfo].deviceModel;
    NSString * languageCode = NSLocale.preferredLanguages.firstObject;
    NSString * language = [[NSLocale localeWithLocaleIdentifier:kLocaleUsedInSupportEmails]
        displayNameForKey:NSLocaleLanguageCode
                    value:languageCode];
    NSString * locale = [NSLocale.currentLocale objectForKey:NSLocaleCountryCode];
    NSString * country = [[NSLocale localeWithLocaleIdentifier:kLocaleUsedInSupportEmails]
        displayNameForKey:NSLocaleCountryCode
                    value:locale];
    NSString * bundleVersion = AppInfo.sharedInfo.bundleVersion;
    NSString * text = [NSString stringWithFormat:@"\n\n\n\n- %@ (%@)\n- MAPS.ME %@\n- %@/%@",
                                                 deviceModel, UIDevice.currentDevice.systemVersion,
                                                 bundleVersion, language, country];
    MWMMailViewController * mailController = [[MWMMailViewController alloc] init];
    mailController.mailComposeDelegate = self;
    [mailController setSubject:[NSString stringWithFormat:@"%@ : %@", L(@"rating_just_rated"),
                                                          @(self.selectedTag)]];
    [mailController setToRecipients:@[ kRateEmail ]];
    [mailController setMessageBody:text isHTML:NO];
    mailController.navigationBar.tintColor = UIColor.blackColor;
    [alertController.ownerViewController presentViewController:mailController
                                                      animated:YES
                                                    completion:nil];
  }
  else
  {
    NSString * text = [NSString stringWithFormat:L(@"email_error_body"), kRateEmail];
    [[[UIAlertView alloc] initWithTitle:L(@"email_error_title")
                                message:text
                               delegate:nil
                      cancelButtonTitle:L(@"ok")
                      otherButtonTitles:nil] show];
  }
}

#pragma mark - MFMailComposeViewControllerDelegate

- (void)mailComposeController:(MFMailComposeViewController *)controller
          didFinishWithResult:(MFMailComposeResult)result
                        error:(NSError *)error
{
  [self.alertController.ownerViewController
      dismissViewControllerAnimated:YES
                         completion:^{
                           [Statistics logEvent:kStatEventName(kStatisticsEvent, kStatClose)];
                           [self close:nil];
                         }];
}

@end
