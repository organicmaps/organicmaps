#import "MWMAuthorizationLoginViewController.h"
#import "MWMAlertViewController.h"
#import "MWMAuthorizationCommon.h"

#include <CoreApi/Framework.h>

namespace
{
NSString * const kWebViewAuthSegue = @"Authorization2WebViewAuthorizationSegue";
NSString * const kOSMAuthSegue = @"Authorization2OSMAuthorizationSegue";

NSString * const kCancel = L(@"cancel");
NSString * const kLogout = L(@"logout");
NSString * const kRefresh = L(@"refresh");
}  // namespace

using namespace osm;
using namespace osm_auth_ios;

@interface MWMAuthorizationLoginViewController ()

@property(weak, nonatomic) IBOutlet UIView * authView;
@property(weak, nonatomic) IBOutlet UIView * accountView;

@property(weak, nonatomic) IBOutlet UIButton * loginWithBrowserOSMButton;
@property(weak, nonatomic) IBOutlet UIButton * loginWithPasswordOSMButton;
@property(weak, nonatomic) IBOutlet UIButton * signupButton;

@property(weak, nonatomic) IBOutlet UILabel * changesCountLabel;
@property(weak, nonatomic) IBOutlet UILabel * lastUpdateLabel;
@property(weak, nonatomic) IBOutlet UITextView * descriptionTextView;
@property(strong, nonatomic) UIImageView * logoImageView;

@end

@implementation MWMAuthorizationLoginViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self setupDescriptionText];
}

- (void)viewDidLayoutSubviews
{
  [super viewDidLayoutSubviews];
  [self applyTextWrapping];
}

- (void)applyTextWrapping
{
  if (!self.logoImageView)
  {
    UIImage * image = [UIImage imageNamed:@"osm_logo"];
    self.logoImageView = [[UIImageView alloc] initWithImage:image];
    self.logoImageView.frame = CGRectMake(5, 10, 60, 60);
    [self.descriptionTextView addSubview:self.logoImageView];
  }

  CGRect imgFrame = self.logoImageView.frame;
  CGRect exclusionRect =
      CGRectMake(imgFrame.origin.x, imgFrame.origin.y, imgFrame.size.width + 7, imgFrame.size.height - 10);
  UIBezierPath * path = [UIBezierPath bezierPathWithRect:exclusionRect];
  self.descriptionTextView.textContainer.exclusionPaths = @[path];
  [self.descriptionTextView.layoutManager
      invalidateLayoutForCharacterRange:NSMakeRange(0, self.descriptionTextView.text.length)
                   actualCharacterRange:NULL];
  [self.descriptionTextView setNeedsLayout];
  [self.descriptionTextView layoutIfNeeded];
}

- (void)setupDescriptionText
{
  NSString * text = self.descriptionTextView.text;

  UIColor * textColor = [[UIColor whiteColor] colorWithAlphaComponent:0.8];

  NSMutableParagraphStyle * paragraphStyle = [[NSMutableParagraphStyle alloc] init];
  paragraphStyle.alignment = NSTextAlignmentJustified;
  paragraphStyle.lineSpacing = 0;
  paragraphStyle.paragraphSpacing = 0;
  NSMutableAttributedString * attr =
      [[NSMutableAttributedString alloc] initWithString:text
                                             attributes:@{
                                               NSForegroundColorAttributeName: textColor,
                                               NSFontAttributeName: self.descriptionTextView.font,
                                               NSParagraphStyleAttributeName: paragraphStyle
                                             }];

  NSRange linkRange = [text rangeOfString:@"OpenStreetMap.org"];
  if (linkRange.location != NSNotFound)
  {
    [attr addAttribute:NSLinkAttributeName
                 value:[NSURL URLWithString:@"https://www.openstreetmap.org"]
                 range:linkRange];
  }

  self.descriptionTextView.attributedText = attr;

  self.descriptionTextView.linkTextAttributes = @{
    NSForegroundColorAttributeName: UIColor.systemBlueColor,
    NSUnderlineStyleAttributeName: @(NSUnderlineStyleNone)
  };

  self.descriptionTextView.delegate = self;
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  [self checkConnection];
  if (AuthorizationHaveCredentials())
    [self configHaveAuth];
  else
    [self configNoAuth];

  AuthorizationSetNeedCheck(NO);
}

- (void)checkConnection
{
  self.signupButton.enabled = Platform::IsConnected();
}

- (void)configHaveAuth
{
  NSString * osmUserName = OSMUserName();
  self.title = osmUserName.length > 0 ? osmUserName : L(@"osm_account");
  self.authView.hidden = YES;
  self.accountView.hidden = NO;

  self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithTitle:@"•••"
                                                                            style:UIBarButtonItemStylePlain
                                                                           target:self
                                                                           action:@selector(showActionSheet)];
  [self refresh:NO];
}

- (void)configNoAuth
{
  self.title = L(@"profile");
  self.authView.hidden = NO;
  self.accountView.hidden = YES;
}

#pragma mark - Actions

- (void)performOnlineAction:(MWMVoidBlock)block
{
  if (Platform::IsConnected())
    block();
  else
    [self.alertController presentNoConnectionAlert];
}

- (IBAction)loginWithBrowser
{
  [self performOnlineAction:^{ [self openUrl:@(OsmOAuth::ServerAuth().BuildOAuth2Url().c_str()) externally:YES]; }];
}

- (IBAction)loginWithPassword
{
  [self
      performOnlineAction:^{ [self performSegueWithIdentifier:kOSMAuthSegue sender:self.loginWithPasswordOSMButton]; }];
}

- (IBAction)signup
{
  [self performOnlineAction:^{ [self openUrl:@(OsmOAuth::ServerAuth().GetRegistrationURL().c_str())]; }];
}

- (IBAction)osmTap
{
  [self openUrl:L(@"osm_wiki_about_url")];
}

- (IBAction)historyTap
{
  [self openUrl:@(OsmOAuth::ServerAuth().GetHistoryURL([OSMUserName() UTF8String]).c_str())];
}

- (void)logout
{
  AuthorizationClearCredentials();
  [self.navigationController popViewControllerAnimated:YES];
}

- (void)refresh:(BOOL)force
{
  self.changesCountLabel.text = @(OSMUserChangesetsCount()).stringValue;
}

#pragma mark - ActionSheet

- (void)showActionSheet
{
  UIAlertController * alertController = [UIAlertController alertControllerWithTitle:nil
                                                                            message:nil
                                                                     preferredStyle:UIAlertControllerStyleActionSheet];
  alertController.popoverPresentationController.barButtonItem = self.navigationItem.rightBarButtonItem;
  [alertController addAction:[UIAlertAction actionWithTitle:kRefresh
                                                      style:UIAlertActionStyleDefault
                                                    handler:^(UIAlertAction * action) { [self refresh:YES]; }]];
  [alertController addAction:[UIAlertAction actionWithTitle:kLogout
                                                      style:UIAlertActionStyleDestructive
                                                    handler:^(UIAlertAction * action) { [self logout]; }]];
  [alertController addAction:[UIAlertAction actionWithTitle:kCancel style:UIAlertActionStyleCancel handler:nil]];

  [self presentViewController:alertController animated:YES completion:nil];
}

#pragma mark - UITextViewDelegate

- (BOOL)textView:(UITextView *)textView
    shouldInteractWithURL:(NSURL *)URL
                  inRange:(NSRange)characterRange
              interaction:(UITextItemInteraction)interaction
{
  [[UIApplication sharedApplication] openURL:URL options:@{} completionHandler:nil];
  return NO;
}

@end
