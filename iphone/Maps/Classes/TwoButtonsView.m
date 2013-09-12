#import "TwoButtonsView.h"

#define MARGIN 10

@interface TwoButtonsView()

@property (nonatomic, retain) UIButton * leftButton;
@property (nonatomic, retain) UIButton * rightButton;
@end

@implementation TwoButtonsView

-(id)initWithFrame:(CGRect)frame leftButtonSelector:(SEL)leftSel rightButtonSelector:(SEL)rightSel leftButtonTitle:(NSString *)leftTitle rightButtontitle:(NSString *)rightTitle target:(id)target
{
  self = [super initWithFrame:frame];
  if (self)
  {
    self.leftButton = [UIButton buttonWithType:UIButtonTypeRoundedRect];
    [self.leftButton addTarget:target action:leftSel forControlEvents:UIControlEventTouchUpInside];
    [self.leftButton setTitle:leftTitle forState:UIControlStateNormal];
    self.leftButton.titleLabel.lineBreakMode = UILineBreakModeWordWrap;
    self.leftButton.titleLabel.textAlignment = UITextAlignmentCenter;

    self.rightButton = [UIButton buttonWithType:UIButtonTypeRoundedRect];
    [self.rightButton addTarget:target action:rightSel forControlEvents:UIControlEventTouchUpInside];
    [self.rightButton setTitle:rightTitle forState:UIControlStateNormal];
    self.rightButton.titleLabel.lineBreakMode = UILineBreakModeWordWrap;
    self.rightButton.titleLabel.textAlignment = UITextAlignmentCenter;

    [self addSubview:self.leftButton];
    [self addSubview:self.rightButton];
  }
  return self;
}

-(void)layoutSubviews
{
  [super layoutSubviews];
  double const buttonWidth = (self.superview.bounds.size.width - 3 * MARGIN) / 2;
  [self.leftButton setFrame:CGRectMake(MARGIN, 0, buttonWidth, self.frame.size.height)];
  [self.rightButton setFrame:CGRectMake(2 * MARGIN + buttonWidth, 0, buttonWidth, self.frame.size.height)];
}

-(void)dealloc
{
  self.leftButton = nil;
  self.rightButton = nil;
  [super dealloc];
}
@end
