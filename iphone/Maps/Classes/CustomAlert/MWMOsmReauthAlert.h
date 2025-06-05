#import "MWMAlert.h"

@interface MWMOsmReauthAlert : MWMAlert <UITextViewDelegate>

+ (instancetype)alert;

@property(nonatomic) IBOutlet UITextView * messageLabel;

@end
