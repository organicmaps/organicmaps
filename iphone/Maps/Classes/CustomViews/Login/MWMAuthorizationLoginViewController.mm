#import "Common.h"
#import "MapsAppDelegate.h"
#import "MWMAlertViewController.h"
#import "MWMAuthorizationCommon.h"
#import "MWMAuthorizationLoginViewController.h"
#import "MWMAuthorizationWebViewLoginViewController.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"

#include "editor/server_api.hpp"

#include "indexer/osm_editor.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

namespace
{
NSString * const kWebViewAuthSegue = @"Authorization2WebViewAuthorizationSegue";
NSString * const kOSMAuthSegue = @"Authorization2OSMAuthorizationSegue";

// I don't use block here because there is big chance to get retain cycle and std::function syntax looks prety easy and cute.
using TActionSheetFunctor = std::function<void()>;
} // namespace

using namespace osm;
using namespace osm_auth_ios;

@interface MWMAuthorizationLoginViewController () <UIActionSheetDelegate>

@property (weak, nonatomic) IBOutlet UIImageView * backgroundImage;
@property (weak, nonatomic) IBOutlet UIButton * loginGoogleButton;
@property (weak, nonatomic) IBOutlet UIButton * loginFacebookButton;
@property (weak, nonatomic) IBOutlet UIButton * loginOSMButton;
@property (weak, nonatomic) IBOutlet UIButton * signupButton;
@property (weak, nonatomic) IBOutlet UILabel * message;
@property (weak, nonatomic) IBOutlet UILabel * signupTitle;
@property (weak, nonatomic) IBOutlet UIButton * logoutButton;
@property (weak, nonatomic) IBOutlet UIImageView * googleImage;
@property (weak, nonatomic) IBOutlet UIImageView * facebookImage;

@property (weak, nonatomic) IBOutlet UIBarButtonItem * leftBarButton;

@property (weak, nonatomic) IBOutlet UIView * profileView;
@property (weak, nonatomic) IBOutlet UIView * localChangesView;
@property (weak, nonatomic) IBOutlet UILabel * localChangesLabel;
@property (weak, nonatomic) IBOutlet UIButton * localChangesActionButton;

@property (weak, nonatomic) IBOutlet UIView * uploadedChangesView;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * uploadedChangesViewTopOffset;
@property (weak, nonatomic) IBOutlet UILabel * uploadedChangesLabel;
@property (weak, nonatomic) IBOutlet UILabel * lastUploadLabel;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * uploadedChangesViewHeight;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * uploadedChangesLabelCenter;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * messageTopOffset;

@property (nonatomic) TActionSheetFunctor actionSheetFunctor;
@end

@implementation MWMAuthorizationLoginViewController

- (void)viewDidLoad
{
  [Statistics logEvent:kStatEventName(kStatAuthorization, kStatOpen)];
  [super viewDidLoad];
  self.backgroundImage.image = [UIImage imageWithColor:[UIColor primary]];
  [self checkConnection];
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  if (AuthorizationHaveCredentials())
    [self configHaveAuth];
  else
    [self configNoAuth:AuthorizationIsNeedCheck() && !AuthorizationIsUserSkip()];

  [self configChanges];
  UINavigationBar * navBar = self.navigationController.navigationBar;
  navBar.barStyle = UIBarStyleBlack;
  navBar.tintColor = [UIColor clearColor];
  navBar.barTintColor = [UIColor clearColor];
  navBar.shadowImage = [[UIImage alloc] init];
  [navBar setBackgroundImage:[[UIImage alloc] init] forBarMetrics:UIBarMetricsDefault];
  navBar.translucent = YES;
  AuthorizationSetNeedCheck(NO);
}

- (void)viewWillDisappear:(BOOL)animated
{
  UINavigationBar * navBar = self.navigationController.navigationBar;
  [MapsAppDelegate customizeAppearanceForNavigationBar:navBar];
}

- (BOOL)shouldAutorotate
{
  return NO;
}

- (void)checkConnection
{
  BOOL const isConnected = Platform::IsConnected();
  self.loginGoogleButton.enabled = isConnected;
  self.loginFacebookButton.enabled = isConnected;
  self.signupButton.enabled = isConnected;

  AuthorizationConfigButton(self.loginGoogleButton, AuthorizationButtonType::AuthorizationButtonTypeGoogle);
  AuthorizationConfigButton(self.loginFacebookButton, AuthorizationButtonType::AuthorizationButtonTypeFacebook);
  AuthorizationConfigButton(self.loginOSMButton, AuthorizationButtonType::AuthorizationButtonTypeOSM);

  if (!isConnected)
  {
    self.loginGoogleButton.layer.borderColor = [UIColor clearColor].CGColor;
    self.loginFacebookButton.layer.borderColor = [UIColor clearColor].CGColor;
  }
}

- (void)configHaveAuth
{
  NSString * osmUserName = OSMUserName();
  self.title = osmUserName.length > 0 ? osmUserName : L(@"osm_account").capitalizedString;
  self.message.hidden = YES;
  self.loginGoogleButton.hidden = YES;
  self.loginFacebookButton.hidden = YES;
  self.loginOSMButton.hidden = YES;
  self.signupTitle.hidden = YES;
  self.signupButton.hidden = YES;
  self.googleImage.hidden = YES;
  self.facebookImage.hidden = YES;
  self.logoutButton.hidden = NO;
  self.leftBarButton.image = [UIImage imageNamed:@"ic_nav_bar_back"];
}

- (void)configNoAuth:(BOOL)isAfterFirstEdit
{
  self.message.hidden = NO;
  self.loginGoogleButton.hidden = NO;
  self.loginFacebookButton.hidden = NO;
  self.loginOSMButton.hidden = NO;
  self.signupTitle.hidden = NO;
  self.signupButton.hidden = NO;
  self.googleImage.hidden = NO;
  self.facebookImage.hidden = NO;
  self.logoutButton.hidden = YES;
  if (isAfterFirstEdit)
  {
    self.title = L(@"thank_you").capitalizedString;
    self.message.text = L(@"you_have_edited_your_first_object");
    self.leftBarButton.image = [UIImage imageNamed:@"ic_nav_bar_close"];
  }
  else
  {
    self.title = L(@"profile").capitalizedString;
    self.message.text = L(@"login_and_edit_map_motivation_message");
    self.leftBarButton.image = [UIImage imageNamed:@"ic_nav_bar_back"];
  }
}

- (void)configChanges
{
  auto const stats = Editor::Instance().GetStats();
  if (stats.m_edits.empty() && !AuthorizationHaveCredentials())
  {
    self.profileView.hidden = YES;
    self.messageTopOffset.priority = UILayoutPriorityDefaultHigh;
  }
  else
  {
    size_t const totalChanges = stats.m_edits.size();
    size_t const uploadedChanges = stats.m_uploadedCount;
    size_t const localChanges = totalChanges - uploadedChanges;

    BOOL const noLocalChanges = (localChanges == 0);
    [self setLocalChangesHidden:noLocalChanges];
    if (!noLocalChanges)
      self.localChangesLabel.text = [NSString stringWithFormat:@"%@ %@", L(@"editor_profile_unsent_changes"), @(localChanges)];

    BOOL const noUploadedChanges = (uploadedChanges == 0);
    [self setUploadedChangesDisabled:noUploadedChanges];
    self.uploadedChangesLabel.text = [NSString stringWithFormat:@"%@ %@", L(@"editor_profile_changes"), @(uploadedChanges)];
    self.lastUploadLabel.hidden = noUploadedChanges;
    if (!noUploadedChanges)
    {
      NSString * lastUploadDate = [NSDateFormatter
          localizedStringFromDate:[NSDate dateWithTimeIntervalSince1970:stats.m_lastUploadTimestamp]
                        dateStyle:NSDateFormatterShortStyle
                        timeStyle:NSDateFormatterNoStyle];
      self.lastUploadLabel.text = [NSString stringWithFormat:@"%@ %@", L(@"last_upload"), lastUploadDate];
    }
    self.uploadedChangesViewHeight.constant = noUploadedChanges ? 44.0 : 64.0;
    self.uploadedChangesLabelCenter.priority = noUploadedChanges ? UILayoutPriorityDefaultHigh : UILayoutPriorityDefaultLow;
    self.messageTopOffset.priority = UILayoutPriorityDefaultLow;
  }
}

- (void)setLocalChangesHidden:(BOOL)isHidden
{
  self.localChangesView.hidden = isHidden;
  self.uploadedChangesViewTopOffset.priority = isHidden ? UILayoutPriorityDefaultHigh : UILayoutPriorityDefaultLow;
}

- (void)setUploadedChangesDisabled:(BOOL)isDisabled
{
  self.uploadedChangesView.alpha = isDisabled ? 0.4 : 1.;
}

#pragma mark - Actions

- (void)performOnlineAction:(TMWMVoidBlock)block
{
  if (Platform::IsConnected())
    block();
  else
    [self.alertController presentNoConnectionAlert];
}

- (IBAction)loginGoogle
{
  [Statistics logEvent:kStatEditorAuthRequets withParameters:@{kStatValue : kStatGoogle}];
  [self performOnlineAction:^
  {
    [Statistics logEvent:kStatEventName(kStatAuthorization, kStatGoogle)];
    [self performSegueWithIdentifier:kWebViewAuthSegue sender:self.loginGoogleButton];
  }];
}

- (IBAction)loginFacebook
{
  [Statistics logEvent:kStatEditorAuthRequets withParameters:@{kStatValue : kStatFacebook}];
  [self performOnlineAction:^
  {
    [Statistics logEvent:kStatEventName(kStatAuthorization, kStatFacebook)];
    [self performSegueWithIdentifier:kWebViewAuthSegue sender:self.loginFacebookButton];
  }];
}

- (IBAction)loginOSM
{
  [Statistics logEvent:kStatEditorAuthRequets withParameters:@{kStatValue : kStatOSM}];
  [self performOnlineAction:^
  {
    [Statistics logEvent:kStatEventName(kStatAuthorization, kStatOSM)];
    [self performSegueWithIdentifier:kOSMAuthSegue sender:self.loginOSMButton];
  }];
}

- (IBAction)signup
{
  [Statistics logEvent:kStatEditorRegRequest];
  [self performOnlineAction:^
  {
    [Statistics logEvent:kStatEventName(kStatAuthorization, kStatSignup)];
    OsmOAuth const auth = OsmOAuth::ServerAuth();
    NSURL * url = [NSURL URLWithString:@(auth.GetRegistrationURL().c_str())];
    [[UIApplication sharedApplication] openURL:url];
  }];
}

- (IBAction)logout
{
  self.actionSheetFunctor = [self]
  {
    [Statistics logEvent:kStatEventName(kStatAuthorization, kStatLogout)];
    AuthorizationStoreCredentials({});
    [self cancel];
  };
  [self showWarningActionSheetWithActionTitle:L(@"logout")];
}

- (IBAction)cancel
{
  if (!self.isCalledFromSettings)
  {
    [Statistics logEvent:kStatEditorAuthDeclinedByUser];
    AuthorizationSetUserSkip();
  }
  UINavigationController * parentNavController = self.navigationController.navigationController;
  if (parentNavController)
    [parentNavController popViewControllerAnimated:YES];
  else
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (IBAction)localChangesAction
{
  self.actionSheetFunctor = [self]
  {
    Editor::Instance().ClearAllLocalEdits();
    [self configChanges];
  };
  [self showWarningActionSheetWithActionTitle:L(@"delete")];
}

#pragma mark - ActionSheet

- (void)showWarningActionSheetWithActionTitle:(NSString *)title
{
  NSString * cancel = L(@"cancel");
  if (isIOS7)
  {
    UIActionSheet * actionSheet = [[UIActionSheet alloc] initWithTitle:nil delegate:self cancelButtonTitle:cancel destructiveButtonTitle:title otherButtonTitles:nil];
    [actionSheet showInView:self.view];
  }
  else
  {
    UIAlertController * alertController = [UIAlertController alertControllerWithTitle:nil message:nil preferredStyle:UIAlertControllerStyleActionSheet];
    UIAlertAction * cancelAction = [UIAlertAction actionWithTitle:cancel style:UIAlertActionStyleCancel handler:nil];
    UIAlertAction * commonAction = [UIAlertAction actionWithTitle:title style:UIAlertActionStyleDestructive handler:^(UIAlertAction * action)
    {
      [self performActionSheetFunctor];
    }];
    [alertController addAction:cancelAction];
    [alertController addAction:commonAction];
    [self presentViewController:alertController animated:YES completion:nil];
  }
}

#pragma mark - UIActionSheetDelegate

- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex
{
  if (actionSheet.destructiveButtonIndex != buttonIndex)
    return;
  [self performActionSheetFunctor];
}

- (void)performActionSheetFunctor
{
  if (!self.actionSheetFunctor)
    return;
  self.actionSheetFunctor();
  self.actionSheetFunctor = nullptr;
}

#pragma mark - Segue

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
  MWMAuthorizationWebViewLoginViewController * dvc = segue.destinationViewController;
  if ([self.loginGoogleButton isEqual:sender])
    dvc.authType = MWMWebViewAuthorizationTypeGoogle;
  else if ([self.loginFacebookButton isEqual:sender])
    dvc.authType = MWMWebViewAuthorizationTypeFacebook;
}

@end
