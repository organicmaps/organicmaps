#import <UIKit/UIKit.h>

@interface TwoButtonsView : UIView
-(id)initWithFrame:(CGRect)frame leftButtonSelector:(SEL)leftSel rightButtonSelector:(SEL)rightTitle leftButtonTitle:(NSString *)leftTitle rightButtontitle:(NSString *)rightTitle target:(id)target;
@end
