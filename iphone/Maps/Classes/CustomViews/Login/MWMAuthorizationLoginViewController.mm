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

CGFloat const kUploadedChangesTopOffset = 48.;
CGFloat const kStatusBarHeight = 20.;

// I don't use block here because there is big chance to get retain cycle and std::function syntax looks prety easy and cute.
using TActionSheetFunctor = std::function<void()>;
} // namespace

using namespace osm;
using namespace osm_auth_ios;

@interface MWMAuthorizationLoginViewController () <UIActionSheetDelegate>

@property (weak, nonatomic) IBOutlet UIView * changesView;
@property (weak, nonatomic) IBOutlet UIView * authView;

@property (weak, nonatomic) IBOutlet UIButton * loginGoogleButton;
@property (weak, nonatomic) IBOutlet UIButton * loginFacebookButton;
@property (weak, nonatomic) IBOutlet UIButton * loginOSMButton;
@property (weak, nonatomic) IBOutlet UIButton * signupButton;

@property (weak, nonatomic) IBOutlet UIButton * logoutButton;

@property (weak, nonatomic) IBOutlet UIView * localChangesView;
@property (weak, nonatomic) IBOutlet UILabel * localChangesLabel;

@property (weak, nonatomic) IBOutlet UIView * uploadedChangesView;
@property (weak, nonatomic) IBOutlet UILabel * uploadedChangesLabel;
@property (weak, nonatomic) IBOutlet UILabel * lastUploadLabel;

@property (weak, nonatomic) IBOutlet UIImageView * emptyProfileImage;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * uploadedChangesTop;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * scrollViewContentHeight;

@property (nonatomic) TActionSheetFunctor actionSheetFunctor;
@end

@implementation MWMAuthorizationLoginViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  [Statistics logEvent:kStatEventName(kStatAuthorization, kStatOpen)];
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

  [self configChanges];
  [self configEmptyProfile];
  [self setContentHeight];
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [self setContentHeight];
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
  [self setContentHeight];
}

- (void)setContentHeight
{
  CGSize const size = [UIScreen mainScreen].bounds.size;
  CGFloat const height = MAX(size.width, size.height) - self.navigationController.navigationBar.height - kStatusBarHeight;
  self.scrollViewContentHeight.constant = height;
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
  self.authView.hidden = YES;
  self.logoutButton.hidden = NO;
}

- (void)configNoAuth
{
  self.title = L(@"profile").capitalizedString;
  self.logoutButton.hidden = YES;
  self.authView.hidden = NO;
}

- (void)configEmptyProfile
{
  if (self.authView.hidden && self.changesView.hidden)
    self.emptyProfileImage.hidden = NO;
}

- (void)configChanges
{
  auto const stats = Editor::Instance().GetStats();
  if (stats.m_edits.empty())
  {
    self.changesView.hidden = YES;
  }
  else
  {
    self.changesView.hidden = NO;
    size_t const totalChanges = stats.m_edits.size();
    size_t const uploadedChanges = stats.m_uploadedCount;
    size_t const localChanges = totalChanges - uploadedChanges;

    BOOL const noLocalChanges = (localChanges == 0);
    [self setLocalChangesHidden:noLocalChanges];

    if (!noLocalChanges)
      self.localChangesLabel.text = [NSString stringWithFormat:@"%@ %@", L(@"editor_profile_unsent_changes"), @(localChanges)];

    BOOL const noUploadedChanges = (uploadedChanges == 0);
    self.uploadedChangesView.hidden = noUploadedChanges;
    if (noUploadedChanges)
      return;

    self.uploadedChangesLabel.text = [NSString stringWithFormat:@"%@ %@", L(@"editor_profile_changes"), @(uploadedChanges)];

    NSString * lastUploadDate = [NSDateFormatter
        localizedStringFromDate:[NSDate dateWithTimeIntervalSince1970:stats.m_lastUploadTimestamp]
                      dateStyle:NSDateFormatterShortStyle
                      timeStyle:NSDateFormatterNoStyle];
    self.lastUploadLabel.text = [NSString stringWithFormat:@"%@ %@", L(@"last_upload"), lastUploadDate];
  }
}

- (void)setLocalChangesHidden:(BOOL)hidden
{
  self.uploadedChangesView.hidden = hidden;
  self.uploadedChangesTop.constant = hidden ? 0 : kUploadedChangesTopOffset;
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
  [self performOnlineAction:^
  {
    [Statistics logEvent:kStatEditorAuthRequets withParameters:@{kStatValue : kStatGoogle, kStatFrom : kStatProfile}];
    [self performSegueWithIdentifier:kWebViewAuthSegue sender:self.loginGoogleButton];
  }];
}

- (IBAction)loginFacebook
{
  [self performOnlineAction:^
  {
    [Statistics logEvent:kStatEditorAuthRequets withParameters:@{kStatValue : kStatFacebook, kStatFrom : kStatProfile}];
    [self performSegueWithIdentifier:kWebViewAuthSegue sender:self.loginFacebookButton];
  }];
}

- (IBAction)loginOSM
{
  [self performOnlineAction:^
  {
    [Statistics logEvent:kStatEditorAuthRequets withParameters:@{kStatValue : kStatOSM, kStatFrom : kStatProfile}];
    [self performSegueWithIdentifier:kOSMAuthSegue sender:self.loginOSMButton];
  }];
}

- (IBAction)signup
{
  [self performOnlineAction:^
  {
    [Statistics logEvent:kStatEditorRegRequest withParameters:@{kStatFrom : kStatProfile}];
    OsmOAuth const auth = OsmOAuth::ServerAuth();
    NSURL * url = [NSURL URLWithString:@(auth.GetRegistrationURL().c_str())];
    [[UIApplication sharedApplication] openURL:url];
  }];
}

- (IBAction)logout:(UIButton *)sender
{
  self.actionSheetFunctor = [self]
  {
    [Statistics logEvent:kStatEventName(kStatAuthorization, kStatLogout)];
    AuthorizationStoreCredentials({});
    [self cancel];
  };
  [self showWarningActionSheetWithActionTitle:L(@"logout") sourceView:sender];
}

- (IBAction)cancel
{
  [self.navigationController popViewControllerAnimated:YES];
}

- (IBAction)localChangesAction:(UIButton *)sender
{
  self.actionSheetFunctor = [self]
  {
    Editor::Instance().ClearAllLocalEdits();
    [self configChanges];
    [self configEmptyProfile];
  };
  [self showWarningActionSheetWithActionTitle:L(@"delete") sourceView:sender];
}

#pragma mark - ActionSheet

- (void)showWarningActionSheetWithActionTitle:(NSString *)title sourceView:(UIView *)view
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

    if (IPAD)
    {
      UIPopoverPresentationController * popPresenter = [alertController popoverPresentationController];
      popPresenter.sourceView = view;
    }
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
