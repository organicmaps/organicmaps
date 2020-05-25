#import "MWMUser.h"

#include <CoreApi/Framework.h>

@implementation MWMUser

+ (nullable NSURL *)phoneAuthURL {
  return [NSURL URLWithString:@(GetFramework().GetUser().GetPhoneAuthUrl("http://localhost").c_str())];
}

+ (BOOL)isAuthenticated {
  return GetFramework().GetUser().IsAuthenticated();
}

+ (NSString *)termsOfUseLink {
  return @(User::GetTermsOfUseLink().c_str());
}

+ (NSString *)privacyPolicyLink {
  return @(User::GetPrivacyPolicyLink().c_str());
}

+ (void)logOut {
  GetFramework().GetUser().ResetAccessToken();
}

+ (void)authenticateWithToken:(NSString *)token
                         type:(MWMSocialTokenType)type
              privacyAccepted:(BOOL)privacyAccepted
                termsAccepted:(BOOL)termsAccepted
                promoAccepted:(BOOL)promoAccepted
                    firstName:(NSString *)firstName
                     lastName:(NSString *)lastName
                   onComplete:(MWMBoolBlock)onComplete {
  auto &user = GetFramework().GetUser();
  User::SocialTokenType socialTokenType;
  NSString *provider = nil;
  switch (type) {
    case MWMSocialTokenTypeGoogle:
      socialTokenType = User::SocialTokenType::Google;
      break;
    case MWMSocialTokenTypeFacebook:
      socialTokenType = User::SocialTokenType::Facebook;
      break;
    case MWMSocialTokenTypePhone:
      socialTokenType = User::SocialTokenType::Phone;
      break;
    case MWMSocialTokenTypeApple:
      socialTokenType = User::SocialTokenType::Apple;
      break;
  }
  auto s = std::make_unique<User::Subscriber>();
  s->m_postCallAction = User::Subscriber::Action::RemoveSubscriber;
  s->m_onAuthenticate = [provider, onComplete](bool success) {
    dispatch_async(dispatch_get_main_queue(), ^{
      onComplete(success);
    });
  };
  user.AddSubscriber(std::move(s));
  user.Authenticate(token.UTF8String, socialTokenType, privacyAccepted, termsAccepted, promoAccepted,
                    firstName.UTF8String, lastName.UTF8String);
}

@end
