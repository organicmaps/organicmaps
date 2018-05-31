#import "MWMAuthorizationViewModel.h"
#import <FBSDKCoreKit/FBSDKAccessToken.h>
#import <GoogleSignIn/GoogleSignIn.h>
#import "Statistics.h"

#include "Framework.h"

#include <memory>

@implementation MWMAuthorizationViewModel

+ (NSURL * _Nullable)phoneAuthURL
{
  return [NSURL URLWithString:@(GetFramework().GetUser().GetPhoneAuthUrl("http://localhost").c_str())];
}

+ (BOOL)isAuthenticated { return GetFramework().GetUser().IsAuthenticated(); }

+ (void)authenticateWithToken:(NSString * _Nonnull)token
                         type:(MWMSocialTokenType)type
              privacyAccepted:(BOOL)privacyAccepted
                termsAccepted:(BOOL)termsAccepted
                promoAccepted:(BOOL)promoAccepted
                       source:(MWMAuthorizationSource)source
                   onComplete:(MWMAuthorizationCompleteBlock)onComplete
{
  auto & user = GetFramework().GetUser();
  User::SocialTokenType socialTokenType;
  NSString * provider = nil;
  switch (type)
  {
  case MWMSocialTokenTypeGoogle:
    provider = kStatGoogle;
    socialTokenType = User::SocialTokenType::Google;
    break;
  case MWMSocialTokenTypeFacebook:
    provider = kStatFacebook;
    socialTokenType = User::SocialTokenType::Facebook;
    break;
  case MWMSocialTokenTypePhone:
    provider = kStatPhone;
    socialTokenType = User::SocialTokenType::Phone;
    break;
  }
  auto s = std::make_unique<User::Subscriber>();
  s->m_postCallAction = User::Subscriber::Action::RemoveSubscriber;
  s->m_onAuthenticate = [provider, source, onComplete](bool success) {
    NSString * event = nil;
    switch (source)
    {
    case MWMAuthorizationSourceUGC:
      event = success ? kStatUGCReviewAuthRequestSuccess : kStatUGCReviewAuthError;
      break;
    case MWMAuthorizationSourceBookmarks:
      event = success ? kStatBookmarksAuthRequestSuccess : kStatBookmarksAuthRequestError;
      break;
    }

    if (success)
      [Statistics logEvent:event withParameters:@{kStatProvider: provider}];
    else
      [Statistics logEvent:event withParameters:@{kStatProvider: kStatMapsme, kStatError: @""}];
    dispatch_async(dispatch_get_main_queue(), ^{
      onComplete(success);
    });
  };
  user.AddSubscriber(std::move(s));
  user.Authenticate(token.UTF8String, socialTokenType, privacyAccepted,
                    termsAccepted, promoAccepted);
}

+ (NSString * _Nonnull)termsOfUseLink
{
  return @(User::GetTermsOfUseLink().c_str());
}

+ (NSString * _Nonnull)privacyPolicyLink
{
  return @(User::GetPrivacyPolicyLink().c_str());
}

@end
