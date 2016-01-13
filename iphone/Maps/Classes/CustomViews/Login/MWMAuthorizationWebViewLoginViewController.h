#import "ViewController.h"

typedef NS_ENUM(NSUInteger, MWMWebViewAuthorizationType)
{
  MWMWebViewAuthorizationTypeGoogle,
  MWMWebViewAuthorizationTypeFacebook
};

@interface MWMAuthorizationWebViewLoginViewController : ViewController

@property (nonatomic) MWMWebViewAuthorizationType authType;

@end
