#import "MWMAuthorizationOSMLoginViewController.h"
#import "MWMAlertViewController.h"
#import "MWMAuthorizationCommon.h"
#import "MWMCircularProgress.h"
#import "MWMSettingsViewController.h"
#import "Statistics.h"
#import "UITextField+RuntimeAttributes.h"

#include "base/logging.hpp"
#include "editor/server_api.hpp"
#include "platform/platform.hpp"
#include "private.h"

typedef NS_OPTIONS(NSUInteger, MWMFieldCorrect) {
  MWMFieldCorrectNO = 0,
  MWMFieldCorrectLogin = 1 << 0,
  MWMFieldCorrectPassword = 1 << 1,
  MWMFieldCorrectAll = MWMFieldCorrectLogin | MWMFieldCorrectPassword
};

using namespace osm;

@interface MWMAuthorizationOSMLoginViewController ()<UITextFieldDelegate>

@property(weak, nonatomic) IBOutlet UITextField * loginTextField;
@property(weak, nonatomic) IBOutlet UITextField * passwordTextField;
@property(weak, nonatomic) IBOutlet UIButton * loginButton;
@property(weak, nonatomic) IBOutlet UIButton * forgotButton;
@property(weak, nonatomic) IBOutlet UIView * spinnerView;

@property(nonatomic) MWMFieldCorrect isCorrect;

@property(nonatomic) MWMCircularProgress * spinner;

@end

@implementation MWMAuthorizationOSMLoginViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"osm_account").capitalizedString;
  self.isCorrect = MWMFieldCorrectNO;
  [self checkConnection];
  [self stopSpinner];
}

- (void)viewDidAppear:(BOOL)animated
{
  [super viewDidAppear:animated];
  if (!self.loginTextField.text.length && !self.passwordTextField.text.length)
    [self.loginTextField becomeFirstResponder];
}

- (BOOL)shouldAutorotate { return NO; }
- (void)checkConnection { self.forgotButton.enabled = Platform::IsConnected(); }
#pragma mark - UITextFieldDelegate

- (BOOL)textField:(UITextField *)textField
    shouldChangeCharactersInRange:(NSRange)range
                replacementString:(NSString *)string
{
  NSString * newString =
      [textField.text stringByReplacingCharactersInRange:range withString:string];
  BOOL const isValid = [textField.validator validateString:newString];

  if ([textField isEqual:self.loginTextField])
  {
    if (isValid)
      self.isCorrect |= MWMFieldCorrectLogin;
    else
      self.isCorrect &= ~MWMFieldCorrectLogin;
  }
  else if ([textField isEqual:self.passwordTextField])
  {
    if (isValid)
      self.isCorrect |= MWMFieldCorrectPassword;
    else
      self.isCorrect &= ~MWMFieldCorrectPassword;
  }

  return YES;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  if ([textField isEqual:self.loginTextField])
  {
    [self.passwordTextField becomeFirstResponder];
  }
  else if ([textField isEqual:self.passwordTextField])
  {
    [textField resignFirstResponder];
    [self login];
  }
  return YES;
}

- (void)startSpinner
{
  self.spinnerView.hidden = NO;
  self.spinner = [[MWMCircularProgress alloc] initWithParentView:self.spinnerView];
  [self.spinner setInvertColor:YES];
  self.spinner.state = MWMCircularProgressStateSpinner;
  self.loginTextField.enabled = NO;
  self.passwordTextField.enabled = NO;
  self.forgotButton.enabled = NO;
  [self.loginButton setTitle:@"" forState:UIControlStateNormal];
}

- (void)stopSpinner
{
  self.spinnerView.hidden = YES;
  self.spinner = nil;
  self.loginTextField.enabled = YES;
  self.passwordTextField.enabled = YES;
  self.forgotButton.enabled = YES;
  [self.loginButton setTitle:L(@"login") forState:UIControlStateNormal];
}

#pragma mark - Actions

- (IBAction)login
{
  if (!self.loginButton.enabled || self.spinner)
    return;
  if (Platform::IsConnected())
  {
    [self startSpinner];
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
      std::string const username = self.loginTextField.text.UTF8String;
      std::string const password = self.passwordTextField.text.UTF8String;
      OsmOAuth auth = OsmOAuth::ServerAuth();
      try
      {
        auth.AuthorizePassword(username, password);
      }
      catch (std::exception const & ex)
      {
        LOG(LWARNING, ("Error login", ex.what()));
        [Statistics logEvent:@"Editor_Auth_request_result"
              withParameters:@{
                kStatIsSuccess : kStatNo,
                kStatErrorData : @(ex.what()),
                kStatType : kStatOSM
              }];
      }
      dispatch_async(dispatch_get_main_queue(), ^{
        [self stopSpinner];
        if (auth.IsAuthorized())
        {
          osm_auth_ios::AuthorizationStoreCredentials(auth.GetKeySecret());
          [Statistics logEvent:@"Editor_Auth_request_result"
                withParameters:@{kStatIsSuccess : kStatYes, kStatType : kStatOSM}];
          UIViewController * svc = nil;
          for (UIViewController * vc in self.navigationController.viewControllers)
          {
            if ([vc isKindOfClass:[MWMSettingsViewController class]])
            {
              svc = vc;
              break;
            }
          }
          if (svc)
            [self.navigationController popToViewController:svc animated:YES];
          else
            [self.navigationController popToRootViewControllerAnimated:YES];
        }
        else
        {
          [self.alertController presentInvalidUserNameOrPasswordAlert];
        }
      });
    });
  }
  else
  {
    [self.alertController presentNoConnectionAlert];
  }
}

- (IBAction)cancel { [self.navigationController popViewControllerAnimated:YES]; }
- (IBAction)forgotPassword
{
  [self openUrl:[NSURL URLWithString:@(OsmOAuth::ServerAuth().GetResetPasswordURL().c_str())]];
}

#pragma mark - Properties

- (void)setIsCorrect:(MWMFieldCorrect)isCorrect
{
  _isCorrect = isCorrect;
  self.loginButton.enabled = isCorrect == MWMFieldCorrectAll;
}

@end
