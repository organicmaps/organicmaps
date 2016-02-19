#import "MWMAlertViewController.h"
#import "MWMAuthorizationCommon.h"
#import "MWMAuthorizationWebViewLoginViewController.h"
#import "MWMCircularProgress.h"

#include "base/logging.hpp"
#include "editor/osm_auth.hpp"

using namespace osm;

namespace
{
NSString * const kVerifierKey = @"oauth_verifier";

BOOL checkURLHasVerifierKey(NSString * urlString)
{
  return [urlString containsString:kVerifierKey];
}

BOOL checkURLNeedsReload(NSString * urlString)
{
  return [urlString hasSuffix:@"/"] || [urlString containsString:@"/welcome"];
}

NSString * getVerifier(NSString * urlString)
{
  NSString * query = [urlString componentsSeparatedByString:@"?"].lastObject;
  NSArray<NSString *> * queryComponents = [query componentsSeparatedByString:@"&"];
  NSString * verifierValue = nil;
  for (NSString * keyValuePair in queryComponents)
  {
    NSArray * pairComponents = [keyValuePair componentsSeparatedByString:@"="];
    NSString * key = [pairComponents.firstObject stringByRemovingPercentEncoding];
    if (![key isEqualToString:kVerifierKey])
      continue;
    verifierValue = [pairComponents.lastObject stringByRemovingPercentEncoding];
  }
  return verifierValue;
}
}  // namespace

@interface MWMAuthorizationWebViewLoginViewController () <UIWebViewDelegate>

@property (weak, nonatomic) IBOutlet UIWebView * webView;
@property (weak, nonatomic) IBOutlet UIView * spinnerView;

@property (nonatomic) MWMCircularProgress * spinner;

@end

@implementation MWMAuthorizationWebViewLoginViewController
{
  TRequestToken m_requestToken;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self configNavBar];
  [self loadAuthorizationPage];
}

#pragma mark - Configuration

- (void)configNavBar
{
  self.title = L(@"login");
  self.navigationItem.leftBarButtonItem =
      [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel
                                                    target:self
                                                    action:@selector(onCancel)];
  self.navigationController.navigationBar.barStyle = UIBarStyleBlack;
}

- (void)loadAuthorizationPage
{
  [self startSpinner];
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^
  {
    OsmOAuth const auth = OsmOAuth::ServerAuth();
    try
    {
      OsmOAuth::TUrlRequestToken urt;
      switch (self.authType)
      {
      case MWMWebViewAuthorizationTypeGoogle:
        urt = auth.GetGoogleOAuthURL();
        break;
      case MWMWebViewAuthorizationTypeFacebook:
        urt = auth.GetFacebookOAuthURL();
        break;
      }
      self->m_requestToken = urt.second;
      NSURL * url = [NSURL URLWithString:@(urt.first.c_str())];
      NSURLRequest * request = [NSURLRequest requestWithURL:url];
      dispatch_async(dispatch_get_main_queue(), ^
      {
        [self stopSpinner];
        self.webView.hidden = NO;
        [self.webView loadRequest:request];
      });
    }
    catch (exception const & ex)
    {
      dispatch_async(dispatch_get_main_queue(), ^
      {
        [self stopSpinner];
        [self.alertController presentInternalErrorAlert];
      });
      LOG(LWARNING, ("Can't loadAuthorizationPage", ex.what()));
    }
  });
}

- (void)startSpinner
{
  self.spinnerView.hidden = NO;
  self.spinner = [[MWMCircularProgress alloc] initWithParentView:self.spinnerView];
  [self.spinner setInvertColor:YES];
  self.spinner.state = MWMCircularProgressStateSpinner;
  self.webView.userInteractionEnabled = NO;
}

- (void)stopSpinner
{
  self.spinnerView.hidden = YES;
  self.spinner = nil;
  self.webView.userInteractionEnabled = YES;
}

- (void)checkAuthorization:(NSString *)verifier
{
  [self startSpinner];
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^
  {
    OsmOAuth const auth = OsmOAuth::ServerAuth();
    TKeySecret ks;
    try
    {
      ks = auth.FinishAuthorization(self->m_requestToken, verifier.UTF8String);
    }
    catch (exception const & ex)
    {
      LOG(LWARNING, ("checkAuthorization error", ex.what()));
    }
    dispatch_async(dispatch_get_main_queue(), ^
    {
      [self stopSpinner];
      if (OsmOAuth::IsValid(ks))
      {
        osm_auth_ios::AuthorizationStoreCredentials(ks);
        [self dismissViewControllerAnimated:NO completion:nil];
      }
      else
      {
        [self loadAuthorizationPage];
        [self.alertController presentInvalidUserNameOrPasswordAlert];
      }
    });
  });
}

#pragma mark - Actions

- (void)onCancel
{
  [self.navigationController popViewControllerAnimated:YES];
}

#pragma mark - UIWebViewDelegate

- (void)webViewDidStartLoad:(UIWebView *)webView
{
  [self startSpinner];
}

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
  [self stopSpinner];
  NSString * urlString = webView.request.URL.absoluteString;

  if (checkURLNeedsReload(urlString))
  {
    [self loadAuthorizationPage];
  }
  else if (checkURLHasVerifierKey(urlString))
  {
    webView.hidden = YES;
    NSString * verifier = getVerifier(urlString);
    NSAssert(verifier, @"Verifier value is nil");
    [self checkAuthorization:verifier];
  }
}

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error
{
  [self.alertController presentInternalErrorAlert];
}

@end
