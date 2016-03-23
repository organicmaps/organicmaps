#import "MWMViewController.h"

typedef NS_ENUM(NSUInteger, MWMWebViewAuthorizationType)
{
  MWMWebViewAuthorizationTypeGoogle,
  MWMWebViewAuthorizationTypeFacebook
};

@interface MWMAuthorizationWebViewLoginViewController : MWMViewController

@property (nonatomic) MWMWebViewAuthorizationType authType;

@end
