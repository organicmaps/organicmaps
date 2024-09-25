#import <UIKit/UIKit.h>

@interface BackButtonWithText : UIView

@property (nonatomic, strong) UIButton *backButton;
@property (nonatomic, strong) UILabel *backLabel;

- (instancetype)initWithFrame:(CGRect)frame;
- (void)setBackButtonAction:(SEL)action target:(id)target;

@end

@implementation BackButtonWithText

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        [self setupBackButton];
        [self setupBackLabel];
    }
    return self;
}

- (void)setupBackButton {
    self.backButton = [UIButton buttonWithType:UIButtonTypeCustom];
    [self.backButton setImage:[UIImage imageNamed:@"back_arrow"] forState:UIControlStateNormal];
    self.backButton.frame = CGRectMake(0, 0, 44, 44);
    [self addSubview:self.backButton];
}

- (void)setupBackLabel {
    self.backLabel = [[UILabel alloc] initWithFrame:CGRectMake(44, 0, 100, 44)];
    self.backLabel.text = @"Back";
    self.backLabel.textColor = [UIColor blackColor];
    self.backLabel.userInteractionEnabled = YES;
    
    UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(labelTapped:)];
    [self.backLabel addGestureRecognizer:tapGesture];
    
    [self addSubview:self.backLabel];
}

- (void)setBackButtonAction:(SEL)action target:(id)target {
    [self.backButton addTarget:target action:action forControlEvents:UIControlEventTouchUpInside];
}

- (void)labelTapped:(UITapGestureRecognizer *)gesture {
    [self.backButton sendActionsForControlEvents:UIControlEventTouchUpInside];
}

@end
