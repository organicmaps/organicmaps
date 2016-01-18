#import "Common.h"
#import "MapsAppDelegate.h"
#import "MWMAuthorizationCommon.h"
#import "MWMAuthorizationLoginViewController.h"
#import "MWMAuthorizationWebViewLoginViewController.h"
#import "UIColor+MapsMeColor.h"

@interface MWMAuthorizationLoginViewController ()

@property (weak, nonatomic) IBOutlet UIImageView * backgroundImage;
@property (weak, nonatomic) IBOutlet UIButton * loginGoogleButton;
@property (weak, nonatomic) IBOutlet UIButton * loginFacebookButton;
@property (weak, nonatomic) IBOutlet UIButton * loginOSMButton;
@property (weak, nonatomic) IBOutlet UIButton * signupButton;

@end

@implementation MWMAuthorizationLoginViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.backgroundImage.image = [UIImage imageWithColor:[UIColor primary]];
  [self checkConnection];
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  self.title = L(@"login");
  UINavigationBar * navBar = self.navigationController.navigationBar;
  navBar.barStyle = UIBarStyleBlack;
  navBar.tintColor = [UIColor clearColor];
  navBar.barTintColor = [UIColor clearColor];
  navBar.shadowImage = [UIImage imageWithColor:[UIColor clearColor]];
  navBar.shadowImage = [[UIImage alloc] init];
  [navBar setBackgroundImage:[[UIImage alloc] init] forBarMetrics:UIBarMetricsDefault];
  navBar.translucent = YES;
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

#pragma mark - Actions

- (IBAction)loginGoogle
{
  // TODO: Add login
}

- (IBAction)loginFacebook
{
  // TODO: Add login
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
