typedef NS_ENUM(NSInteger, MWMSocialTokenType) {
  MWMSocialTokenTypeGoogle,
  MWMSocialTokenTypeFacebook
};

@interface MWMAuthorizationViewModel : NSObject

+ (BOOL)isAuthenticated;
+ (void)authenticateWithToken:(NSString* _Nonnull)token type:(enum MWMSocialTokenType)type;

@end
