#import "MWMAlert.h"

@interface MWMOsmReauthAlert : MWMAlert

+ (instancetype)alert;

@property (nonatomic, retain) IBOutlet UITextView *messageLabel;

@end
