#import <UIKit/UIKit.h>

@interface BackButtonWithText : UIView

@property (nonatomic, strong) UIButton *backButton;
@property (nonatomic, strong) UILabel *backLabel;

- (instancetype)initWithFrame:(CGRect)frame;
- (void)setBackButtonAction:(SEL)action target:(id)target;

@end
