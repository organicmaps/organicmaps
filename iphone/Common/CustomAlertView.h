#import <UIKit/UIKit.h>

/// Alert View which can automatically close when going to background
/// and call CancelButtonIndex delegate
@interface CustomAlertView : UIAlertView

- (id)initWithTitle:(NSString *)title message:(NSString *)message delegate:(id)delegate cancelButtonTitle:(NSString *)cancelButtonTitle otherButtonTitles:(NSString *)otherButtonTitles, ...;

@end
