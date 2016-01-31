#import "Common.h"
#import "MapsAppDelegate.h"
#import "MWMAuthorizationCommon.h"
#import "MWMAuthorizationCommon.h"
#import "MWMAuthorizationLoginViewController.h"
#import "MWMAuthorizationWebViewLoginViewController.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"

#include "editor/osm_auth.hpp"
#include "editor/server_api.hpp"

using namespace osm;

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
@property (weak, nonatomic) IBOutlet UILabel * localChangesLabel;
@property (weak, nonatomic) IBOutlet UILabel * localChangesNotUploadedLabel;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * localChangesViewHeight;
@property (weak, nonatomic) IBOutlet UIButton * localChangesActionButton;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * localChangesLabelCenter;

@property (weak, nonatomic) IBOutlet UILabel * uploadedChangesLabel;
@property (weak, nonatomic) IBOutlet UILabel * lastUploadLabel;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * uploadedChangesViewHeight;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * uploadedChangesLabelCenter;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * messageTopOffset;
@end

@implementation MWMAuthorizationLoginViewController

- (void)viewDidLoad
{
  [[Statistics instance] logEvent:kStatEventName(kStatAuthorization, kStatOpen)];
  [super viewDidLoad];
  self.backgroundImage.image = [UIImage imageWithColor:[UIColor primary]];
  [self checkConnection];
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  if (MWMAuthorizationHaveCredentials())
    [self configHaveAuth];
  else
    [self configNoAuth:MWMAuthorizationIsNeedCheck() && !MWMAuthorizationIsUserSkip()];

  [self configChanges];
  UINavigationBar * navBar = self.navigationController.navigationBar;
  navBar.barStyle = UIBarStyleBlack;
  navBar.tintColor = [UIColor clearColor];
  navBar.barTintColor = [UIColor clearColor];
  navBar.shadowImage = [UIImage imageWithColor:[UIColor clearColor]];
  navBar.shadowImage = [[UIImage alloc] init];
  [navBar setBackgroundImage:[[UIImage alloc] init] forBarMetrics:UIBarMetricsDefault];
  navBar.translucent = YES;
  MWMAuthorizationSetNeedCheck(NO);
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

  MWMAuthorizationConfigButton(self.loginGoogleButton, MWMAuthorizationButtonTypeGoogle);
  MWMAuthorizationConfigButton(self.loginFacebookButton, MWMAuthorizationButtonTypeFacebook);
  MWMAuthorizationConfigButton(self.loginOSMButton, MWMAuthorizationButtonTypeOSM);

  if (!isConnected)
  {
    self.loginGoogleButton.layer.borderColor = [UIColor clearColor].CGColor;
    self.loginFacebookButton.layer.borderColor = [UIColor clearColor].CGColor;
  }
}

- (void)configHaveAuth
{
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^
  {
    ServerApi06 const api(OsmOAuth::ServerAuth(MWMAuthorizationGetCredentials()));
    try
    {
      UserPreferences const prefs = api.GetUserPreferences();
      dispatch_async(dispatch_get_main_queue(), ^
      {
        self.title = @(prefs.m_displayName.c_str());
      });
    }
    catch (exception const & ex)
    {
      // TODO(@igrechuhin): Should we display some error here?
      LOG(LWARNING, ("Can't load user preferences from OSM server:", ex.what()));
    }
  });
  // TODO(@igrechuhin): Cache user name and other info to display while offline.
  // Note that this cache should be reset if user logs out.
  self.title = @"";
  self.message.hidden = YES;
  self.loginGoogleButton.hidden = YES;
  self.loginFacebookButton.hidden = YES;
  self.loginOSMButton.hidden = YES;
  self.signupTitle.hidden = YES;
  self.signupButton.hidden = YES;
  self.googleImage.hidden = YES;
  self.facebookImage.hidden = YES;
  self.logoutButton.hidden = NO;
  self.leftBarButton.image = [UIImage imageNamed:@"btn_back_arrow"];
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
    self.title = L(@"thank_you");
    self.message.text = L(@"thank_you_message");
    self.leftBarButton.image = [UIImage imageNamed:@"ic_nav_bar_close"];
  }
  else
  {
    self.title = L(@"profile");
    self.message.text = L(@"profile_message");
    self.leftBarButton.image = [UIImage imageNamed:@"btn_back_arrow"];
  }
}

- (void)configChanges
{
  auto const stats = Editor::Instance().GetStats();
  if (stats.m_edits.empty() && !MWMAuthorizationHaveCredentials())
  {
    self.profileView.hidden = YES;
    self.messageTopOffset.priority = UILayoutPriorityDefaultHigh;
  }
  else
  {
    size_t const totalChanges = stats.m_edits.size();
    size_t const uploadedChanges = stats.m_uploadedCount;
    size_t const localChanges = totalChanges - uploadedChanges;

    self.localChangesLabel.text = [NSString stringWithFormat:@"%@: %@", L(@"changes"), @(localChanges).stringValue];
    BOOL const noLocalChanges = (localChanges == 0);
    self.localChangesNotUploadedLabel.hidden = noLocalChanges;
    self.localChangesActionButton.hidden = noLocalChanges;
    self.localChangesViewHeight.constant = noLocalChanges ? 44.0 : 64.0;
    self.localChangesLabelCenter.priority = noLocalChanges ? UILayoutPriorityDefaultHigh : UILayoutPriorityDefaultLow;

    BOOL const noUploadedChanges = (uploadedChanges == 0);
    self.uploadedChangesLabel.text = [NSString stringWithFormat:@"%@: %@", L(@"changes"), @(uploadedChanges).stringValue];
    self.lastUploadLabel.hidden = noUploadedChanges;
    if (!noUploadedChanges)
      self.lastUploadLabel.text = [NSDateFormatter
          localizedStringFromDate:[NSDate dateWithTimeIntervalSince1970:stats.m_lastUploadTimestamp]
                        dateStyle:NSDateFormatterShortStyle
                        timeStyle:NSDateFormatterNoStyle];
    self.uploadedChangesViewHeight.constant = noUploadedChanges ? 44.0 : 64.0;
    self.uploadedChangesLabelCenter.priority = noUploadedChanges ? UILayoutPriorityDefaultHigh : UILayoutPriorityDefaultLow;
    self.messageTopOffset.priority = UILayoutPriorityDefaultLow;
  }
}

#pragma mark - Actions

- (IBAction)loginGoogle
{
  [[Statistics instance] logEvent:kStatEventName(kStatAuthorization, kStatGoogle)];
}

- (IBAction)loginFacebook
{
  [[Statistics instance] logEvent:kStatEventName(kStatAuthorization, kStatFacebook)];
}

- (IBAction)loginOSM
{
  [[Statistics instance] logEvent:kStatEventName(kStatAuthorization, kStatOSM)];
}

- (IBAction)signup
{
  [[Statistics instance] logEvent:kStatEventName(kStatAuthorization, kStatSignup)];
  OsmOAuth const auth = OsmOAuth::ServerAuth();
  NSURL * url = [NSURL URLWithString:@(auth.GetRegistrationURL().c_str())];
  [[UIApplication sharedApplication] openURL:url];
}

- (IBAction)logout
{
  [[Statistics instance] logEvent:kStatEventName(kStatAuthorization, kStatLogout)];
  MWMAuthorizationStoreCredentials({});
  [self cancel];
}

- (IBAction)cancel
{
  if (!self.isCalledFromSettings)
    MWMAuthorizationSetUserSkip();
  UINavigationController * parentNavController = self.navigationController.navigationController;
  if (parentNavController)
    [parentNavController popViewControllerAnimated:YES];
  else
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (IBAction)localChangesAction
{
  NSString * cancel = L(@"cancel");
  NSString * del = L(@"delete");
  if (isIOS7)
  {
    UIActionSheet * actionSheet = [[UIActionSheet alloc] initWithTitle:nil delegate:self cancelButtonTitle:cancel destructiveButtonTitle:del otherButtonTitles:nil];
    [actionSheet showInView:self.view];
  }
  else
  {
    UIAlertController * alertController = [UIAlertController alertControllerWithTitle:nil message:nil preferredStyle:UIAlertControllerStyleActionSheet];
    UIAlertAction * cancelAction = [UIAlertAction actionWithTitle:cancel style:UIAlertActionStyleCancel handler:nil];
    UIAlertAction * openSettingsAction = [UIAlertAction actionWithTitle:del style:UIAlertActionStyleDestructive handler:^(UIAlertAction * action)
    {
      Editor::Instance().ClearAllLocalEdits();
      [self configChanges];
    }];
    [alertController addAction:cancelAction];
    [alertController addAction:openSettingsAction];
    [self presentViewController:alertController animated:YES completion:nil];
  }
}

#pragma mark - UIActionSheetDelegate

- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex
{
  if (actionSheet.destructiveButtonIndex != buttonIndex)
    return;
  Editor::Instance().ClearAllLocalEdits();
  [self configChanges];
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
