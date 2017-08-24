#import "MWMAlertViewController.h"
#import "MWMAuthorizationCommon.h"
#import "MWMAuthorizationLoginViewController.h"
#import "MWMAuthorizationWebViewLoginViewController.h"
#import "Statistics.h"

#include "Framework.h"

namespace
{
NSString * const kWebViewAuthSegue = @"Authorization2WebViewAuthorizationSegue";
NSString * const kOSMAuthSegue = @"Authorization2OSMAuthorizationSegue";

NSString * const kCancel = L(@"cancel");
NSString * const kLogout = L(@"logout");
NSString * const kRefresh = L(@"refresh");
} // namespace

using namespace osm;
using namespace osm_auth_ios;

@interface MWMAuthorizationLoginViewController ()

@property (weak, nonatomic) IBOutlet UIView * authView;
@property (weak, nonatomic) IBOutlet UIView * accountView;

@property (weak, nonatomic) IBOutlet UIButton * loginGoogleButton;
@property (weak, nonatomic) IBOutlet UIButton * loginFacebookButton;
@property (weak, nonatomic) IBOutlet UIButton * loginOSMButton;
@property (weak, nonatomic) IBOutlet UIButton * signupButton;

@property (weak, nonatomic) IBOutlet UILabel * changesCountLabel;
@property (weak, nonatomic) IBOutlet UILabel * lastUpdateLabel;
@property (weak, nonatomic) IBOutlet UILabel * rankLabel;
@property (weak, nonatomic) IBOutlet UILabel * changesToNextPlaceLabel;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * yourPlaceLabelCenterYAlignment;

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
    self.loginGoogleButton.layer.borderColor = UIColor.clearColor.CGColor;
    self.loginFacebookButton.layer.borderColor = UIColor.clearColor.CGColor;
  }
}

- (void)configHaveAuth
{
  NSString * osmUserName = OSMUserName();
  self.title = osmUserName.length > 0 ? osmUserName : L(@"osm_account").capitalizedString;
  self.authView.hidden = YES;
  self.accountView.hidden = NO;

  self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithTitle:@"•••" style:UIBarButtonItemStylePlain target:self action:@selector(showActionSheet)];
  [self refresh:NO];
}

- (void)configNoAuth
{
  self.title = L(@"profile").capitalizedString;
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
    [self openUrl:[NSURL URLWithString:@(OsmOAuth::ServerAuth().GetRegistrationURL().c_str())]];
  }];
}

- (IBAction)osmTap
{
  [self openUrl:[NSURL URLWithString:@"https://wiki.openstreetmap.org/wiki/Main_Page"]];
}

- (void)logout
{
  [Statistics logEvent:kStatEventName(kStatAuthorization, kStatLogout)];
  NSString * osmUserName = OSMUserName();
  if (osmUserName.length > 0)
    GetFramework().DropUserStats(osmUserName.UTF8String);
  AuthorizationStoreCredentials({});
  [self.navigationController popViewControllerAnimated:YES];
}

- (void)refresh:(BOOL)force
{
  [self updateUI];
  __weak auto weakSelf = self;
  auto const policy = force ? editor::UserStatsLoader::UpdatePolicy::Force
                            : editor::UserStatsLoader::UpdatePolicy::Lazy;
  NSString * osmUserName = OSMUserName();
  if (osmUserName.length > 0)
    GetFramework().UpdateUserStats(osmUserName.UTF8String, policy, ^{ [weakSelf updateUI]; });
}

- (void)updateUI
{
  NSString * osmUserName = OSMUserName();
  if (osmUserName.length == 0)
    return;
  editor::UserStats stats = GetFramework().GetUserStats(osmUserName.UTF8String);
  if (!stats)
    return;
  int32_t changesCount;
  if (stats.GetChangesCount(changesCount))
    self.changesCountLabel.text = @(changesCount).stringValue;
  int32_t rank;
  if (stats.GetRank(rank))
    self.rankLabel.text = @(rank).stringValue;
  string levelUpFeat;
  if (stats.GetLevelUpRequiredFeat(levelUpFeat))
  {
    self.yourPlaceLabelCenterYAlignment.priority = UILayoutPriorityDefaultLow;
    self.changesToNextPlaceLabel.hidden = NO;
    self.changesToNextPlaceLabel.text =
        [NSString stringWithFormat:@"%@ %@", L(@"editor_profile_changes_for_next_place"),
                                   @(levelUpFeat.c_str())];
  }
  else
  {
    self.yourPlaceLabelCenterYAlignment.priority = UILayoutPriorityDefaultHigh;
    self.changesToNextPlaceLabel.hidden = YES;
  }

  NSString * lastUploadDate = [NSDateFormatter
      localizedStringFromDate:[NSDate dateWithTimeIntervalSince1970:stats.GetLastUpdate()]
                    dateStyle:NSDateFormatterShortStyle
                    timeStyle:NSDateFormatterShortStyle];
  self.lastUpdateLabel.text =
      [NSString stringWithFormat:L(@"last_update"), lastUploadDate.UTF8String];
}

#pragma mark - ActionSheet

- (void)showActionSheet
{
  UIAlertController * alertController =
      [UIAlertController alertControllerWithTitle:nil
                                          message:nil
                                   preferredStyle:UIAlertControllerStyleActionSheet];
  [alertController addAction:[UIAlertAction actionWithTitle:kRefresh
                                                      style:UIAlertActionStyleDefault
                                                    handler:^(UIAlertAction * action) {
                                                      [self refresh:YES];
                                                    }]];
  [alertController addAction:[UIAlertAction actionWithTitle:kLogout
                                                      style:UIAlertActionStyleDestructive
                                                    handler:^(UIAlertAction * action) {
                                                      [self logout];
                                                    }]];
  [alertController
      addAction:[UIAlertAction actionWithTitle:kCancel style:UIAlertActionStyleCancel handler:nil]];

  if (IPAD)
  {
    UIPopoverPresentationController * popPresenter =
        [alertController popoverPresentationController];
    popPresenter.barButtonItem = self.navigationItem.rightBarButtonItem;
  }
  [self presentViewController:alertController animated:YES completion:nil];
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
