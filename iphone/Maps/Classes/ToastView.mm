#import "ToastView.h"
#import "UIColor+MapsMeColor.h"

@interface ToastView ()

@property (nonatomic) UILabel * messageLabel;
@property (nonatomic) NSTimer * timer;

@end

@implementation ToastView

- (id)initWithMessage:(NSString *)message
{
  CGFloat const xOffset = 18;
  CGFloat const yOffset = 12;
  CGSize textSize = [message sizeWithDrawSize:CGSizeMake(245 - 2 * xOffset, 1000) font:self.messageLabel.font];

  self = [super initWithFrame:CGRectMake(0, 0, textSize.width + 2 * xOffset, textSize.height + 2 * yOffset)];
  self.backgroundColor = [UIColor pressBackground];
  self.layer.cornerRadius = 4;
  self.layer.masksToBounds = NO;
  self.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleTopMargin;

  [self addSubview:self.messageLabel];
  self.messageLabel.frame = CGRectMake(xOffset, yOffset - 1, textSize.width, textSize.height);
  self.messageLabel.text = message;

  UITapGestureRecognizer * tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tap:)];
  [self addGestureRecognizer:tap];

  return self;
}

- (UILabel *)messageLabel
{
  if (!_messageLabel)
  {
    _messageLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 0, 0)];
    _messageLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:12.5];
    _messageLabel.textAlignment = NSTextAlignmentCenter;
    _messageLabel.textColor = [UIColor blackPrimaryText];
    _messageLabel.numberOfLines = 0;
    _messageLabel.lineBreakMode = NSLineBreakByWordWrapping;
  }
  return _messageLabel;
}

- (void)tap:(UITapGestureRecognizer *)sender
{
  [self hide];
}

- (void)timerSel:(id)sender
{
  [self hide];
}

- (void)hide
{
  [UIView animateWithDuration:0.3 animations:^{
    self.alpha = 0;
  } completion:^(BOOL finished){
    [self removeFromSuperview];
  }];
}

- (void)show
{
  UIWindow * mainWindow = [[UIApplication sharedApplication].windows firstObject];
  UIWindow * window = [[mainWindow subviews] firstObject];
  [window addSubview:self];

  self.midX = window.width / 2;
  self.maxY = window.height - 68;
  self.alpha = 0;
  [UIView animateWithDuration:0.3 animations:^{
    self.alpha = 1;
  }];

  self.timer = [NSTimer scheduledTimerWithTimeInterval:3 target:self selector:@selector(timerSel:) userInfo:nil repeats:NO];
}

- (void)dealloc
{
  [self.timer invalidate];
}

@end
