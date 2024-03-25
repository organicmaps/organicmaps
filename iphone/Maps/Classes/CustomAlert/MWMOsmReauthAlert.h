#import "MWMAlert.h"

@interface MWMOsmReauthAlert : MWMAlert <UITextViewDelegate>

+ (instancetype)alert;

@property (nonatomic, retain) IBOutlet UITextView *messageLabel;

@end
