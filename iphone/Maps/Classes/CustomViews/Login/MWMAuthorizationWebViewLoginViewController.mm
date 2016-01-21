#import "MWMAuthorizationCommon.h"
#import "MWMAuthorizationWebViewLoginViewController.h"
#import "MWMCircularProgress.h"

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
  TKeySecret m_keySecret;
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
    // TODO(AlexZ): Change to production.
    OsmOAuth const auth = OsmOAuth::ServerAuth();
    OsmOAuth::TUrlKeySecret urlKey;
    switch (self.authType)
    {
      case MWMWebViewAuthorizationTypeGoogle:
        urlKey = auth.GetGoogleOAuthURL();
        break;
      case MWMWebViewAuthorizationTypeFacebook:
        urlKey = auth.GetFacebookOAuthURL();
        break;
    }

    self->m_keySecret = urlKey.second;
    NSURL * url = [NSURL URLWithString:@(urlKey.first.c_str())];
    NSURLRequest * request = [NSURLRequest requestWithURL:url];
    dispatch_async(dispatch_get_main_queue(), ^
    {
      [self stopSpinner];
      self.webView.hidden = NO;
      [self.webView loadRequest:request];
    });
  });
}

- (void)startSpinner
{
  self.spinnerView.hidden = NO;
  self.spinner = [[MWMCircularProgress alloc] initWithParentView:self.spinnerView];
  [self.spinner startSpinner];
  self.webView.userInteractionEnabled = NO;
}

- (void)stopSpinner
{
  self.spinnerView.hidden = YES;
  [self.spinnerView.subviews makeObjectsPerformSelector:@selector(removeFromSuperview)];
  [self.spinner stopSpinner];
  self.spinner = nil;
  self.webView.userInteractionEnabled = YES;
}

- (void)checkAuthorization:(NSString *)verifier
{
  [self startSpinner];
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^
  {
    TKeySecret outKeySecret;
    // TODO(AlexZ): Change to production.
    OsmOAuth const auth = OsmOAuth::ServerAuth();
    OsmOAuth::AuthResult const result = auth.FinishAuthorization(self->m_keySecret, verifier.UTF8String, outKeySecret);
    dispatch_async(dispatch_get_main_queue(), ^
    {
      [self stopSpinner];
      if (result == OsmOAuth::AuthResult::OK)
      {
        MWMAuthorizationStoreCredentials(outKeySecret);
        [self dismissViewControllerAnimated:NO completion:nil];
      }
      else
      {
        [self loadAuthorizationPage];
        // TODO Add error handling
        [self showAlert:L(@"authorization_error") withButtonTitle:L(@"ok")];
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
  // TODO Add error handling
  [self showAlert:L(@"authorization_error") withButtonTitle:L(@"ok")];
}

@end
