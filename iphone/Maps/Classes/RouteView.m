
#import "RouteView.h"
#import "UIKitCategories.h"

@interface RouteView ()

@property (nonatomic) UIButton * closeButton;
@property (nonatomic) UIButton * startButton;

@property (nonatomic) UIImageView * turnTypeView;
@property (nonatomic) UIImageView * turnView;

@property (nonatomic) UIImageView * nextTurnDistanceView;
@property (nonatomic) UIView * wrapView;
@property (nonatomic) UILabel * nextTurnDistanceLabel;
@property (nonatomic) UILabel * nextTurnMetricsLabel;

@property (nonatomic) UIImageView * overallInfoView;
@property (nonatomic) UILabel * distanceLabel;
@property (nonatomic) UILabel * timeLeftLabel;

@end

@implementation RouteView

- (instancetype)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];
  self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleBottomMargin;

  [self addSubview:self.turnView];
  
  [self addSubview:self.nextTurnDistanceView];
  [self.nextTurnDistanceView addSubview:self.wrapView];
  [self.wrapView addSubview:self.nextTurnDistanceLabel];
  [self.wrapView addSubview:self.nextTurnMetricsLabel];
  
  [self addSubview:self.overallInfoView];
  [self.overallInfoView addSubview:self.distanceLabel];
  [self.overallInfoView addSubview:self.timeLeftLabel];
  
  [self addSubview:self.closeButton];
  [self addSubview:self.startButton];

  return self;
}

- (void)updateWithInfo:(NSDictionary *)info
{
  self.turnTypeView.image = info[@"turnTypeImage"];
  self.nextTurnDistanceLabel.text = info[@"turnDistance"];
  self.nextTurnMetricsLabel.text = [info[@"turnMetrics"] uppercaseString];
  self.distanceLabel.text = [info[@"targetDistance"] stringByAppendingString:[info[@"targetMetrics"] uppercaseString]];
  self.timeLeftLabel.text = [self secondsToString:info[@"timeToTarget"]];
  
  [UIView animateWithDuration:0.2 animations:^{
    [self layoutSubviews];
  }];
}

- (NSString *)secondsToString:(NSNumber *)seconds
{
  NSString * minutesString = [NSString stringWithFormat:@"%d min", ([seconds integerValue] / 60)];
  return minutesString;
}

#define BUTTON_HEIGHT 51

- (void)layoutSubviews
{
  if (self.nextTurnDistanceLabel.text.length == 0)
    self.nextTurnDistanceView.alpha = 0;
  else
  {
    [self.nextTurnDistanceLabel sizeToIntegralFit];
    [self.nextTurnMetricsLabel sizeToIntegralFit];

    CGFloat const betweenOffset = 2;
    self.wrapView.size = CGSizeMake(self.nextTurnDistanceLabel.width + betweenOffset + self.nextTurnMetricsLabel.width, 40);
    self.wrapView.center = CGPointMake(self.wrapView.superview.width / 2, self.wrapView.superview.height / 2);
    self.wrapView.frame = CGRectIntegral(self.wrapView.frame);

    self.nextTurnDistanceLabel.minX = 0;
    self.nextTurnMetricsLabel.minX = self.nextTurnDistanceLabel.minX + self.nextTurnDistanceLabel.width + betweenOffset;
    self.nextTurnDistanceLabel.midY = self.wrapView.height / 2 ;
    self.nextTurnMetricsLabel.maxY = self.nextTurnDistanceLabel.maxY - 5;

    self.nextTurnDistanceView.alpha = 1;
    self.nextTurnDistanceView.size = CGSizeMake(self.wrapView.width + 24, BUTTON_HEIGHT);
    
    [self.distanceLabel sizeToIntegralFit];
    [self.timeLeftLabel sizeToIntegralFit];
    CGFloat const overallInfoViewPadding = 12;
    self.overallInfoView.width = MAX(self.distanceLabel.width, self.timeLeftLabel.width) + 2 * overallInfoViewPadding;
    self.overallInfoView.minX = self.nextTurnDistanceView.maxX;
    self.distanceLabel.minX = overallInfoViewPadding;
    self.distanceLabel.minY = 10;
    self.timeLeftLabel.minX = overallInfoViewPadding;
    self.timeLeftLabel.maxY = self.timeLeftLabel.superview.height - 10;
  }
}

- (void)didMoveToSuperview
{
  self.minY = [self viewMinY];
  [self setVisible:NO animated:NO];
}

- (void)closeButtonPressed:(id)sender
{
  [self.delegate routeViewDidCancelRouting:self];
}

- (void)startButtonPressed:(id)sender
{
  [self.delegate routeViewDidStartFollowing:self];
}

- (void)hideFollowButton
{
  self.startButton.userInteractionEnabled = NO;
  [UIView animateWithDuration:0.5 delay:0.1 damping:0.83 initialVelocity:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
    self.startButton.maxY = -30;
  } completion:nil];
}

- (void)setVisible:(BOOL)visible animated:(BOOL)animated
{
  _visible = visible;
  CGFloat const offsetInnerY = 2;
  CGFloat const offsetInnerX = 2;
  CGFloat const originY = 20;
  [UIView animateWithDuration:(animated ? 0.5 : 0) delay:0 damping:0.83 initialVelocity:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
    self.turnView.minX = offsetInnerX;
    self.nextTurnDistanceView.minX = self.turnView.maxX;
    self.overallInfoView.minX = self.nextTurnDistanceView.maxX;
    self.closeButton.maxX = self.width - offsetInnerX;
    self.startButton.maxX = self.closeButton.minX + 3;
    if (visible)
    {
      self.startButton.userInteractionEnabled = YES;
      self.startButton.minY = originY - offsetInnerY;
      self.turnView.minY = self.startButton.minY;
      self.nextTurnDistanceView.minY = self.startButton.minY;
      self.overallInfoView.minY = self.startButton.minY;
      self.closeButton.minY = self.startButton.minY;
      self.startButton.alpha = 1.0;
      self.turnView.alpha = 1.0;
      self.nextTurnDistanceView.alpha = 1.0;
      self.overallInfoView.alpha = 1.0;
      self.closeButton.alpha = 1.0;
    }
    else
    {
      self.startButton.maxY = -30;
      self.turnView.maxY = self.startButton.maxY;
      self.nextTurnDistanceView.maxY = self.startButton.maxY;
      self.overallInfoView.maxY = self.startButton.maxY;
      self.closeButton.maxY = self.startButton.maxY;
      self.startButton.alpha = 0.0;
      self.turnView.alpha = 0.0;
      self.nextTurnDistanceView.alpha = 0.0;
      self.overallInfoView.alpha = 0.0;
      self.closeButton.alpha = 0.0;
    }
  } completion:nil];
}

- (CGFloat)viewMinY
{
  return SYSTEM_VERSION_IS_LESS_THAN(@"7") ? -20 : 0;
}

- (UIButton *)closeButton
{
  if (!_closeButton)
  {
    _closeButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 50, BUTTON_HEIGHT)];
    _closeButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleBottomMargin;
    UIImage * backgroundImage = [[UIImage imageNamed:@"RoutingButtonBackground"] resizableImageWithCapInsets:UIEdgeInsetsMake(5, 5, 5, 5)];
    [_closeButton setBackgroundImage:backgroundImage forState:UIControlStateNormal];

    UIImageView * imageView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"StopRoutingButton"]];
    [_closeButton addSubview:imageView];
    imageView.center = CGPointMake(_closeButton.width / 2, _closeButton.height / 2 + 2);
    imageView.frame = CGRectIntegral(imageView.frame);

    [_closeButton addTarget:self action:@selector(closeButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
  }
  return _closeButton;
}

- (UIButton *)startButton
{
  if (!_startButton)
  {
    NSString * title = L(@"routing_go");
    UIFont * font = [UIFont fontWithName:@"HelveticaNeue-Light" size:19];
    CGFloat const width = [title sizeWithDrawSize:CGSizeMake(200, 30) font:font].width + 38;

    _startButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, width, BUTTON_HEIGHT)];
    _startButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleBottomMargin;
    UIImage * backgroundImage = [[UIImage imageNamed:@"StartRoutingButtonBackground"] resizableImageWithCapInsets:UIEdgeInsetsMake(5, 5, 5, 5)];
    [_startButton setBackgroundImage:backgroundImage forState:UIControlStateNormal];

    _startButton.titleLabel.font = font;
    _startButton.titleEdgeInsets = UIEdgeInsetsMake(2, 0, 0, 0);
    [_startButton setTitle:title forState:UIControlStateNormal];
    [_startButton setTitleColor:[UIColor colorWithColorCode:@"179E4D"] forState:UIControlStateNormal];

    [_startButton addTarget:self action:@selector(startButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
  }
  return _startButton;
}

- (UILabel *)nextTurnDistanceLabel
{
  if (!_nextTurnDistanceLabel)
  {
    _nextTurnDistanceLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _nextTurnDistanceLabel.backgroundColor = [UIColor clearColor];
    _nextTurnDistanceLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:34];
    _nextTurnDistanceLabel.textColor = [UIColor blackColor];
  }
  return _nextTurnDistanceLabel;
}

- (UILabel *)nextTurnMetricsLabel
{
  if (!_nextTurnMetricsLabel)
  {
    _nextTurnMetricsLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _nextTurnMetricsLabel.backgroundColor = [UIColor clearColor];
    _nextTurnMetricsLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:11];
    _nextTurnMetricsLabel.textColor = [UIColor blackColor];
  }
  return _nextTurnMetricsLabel;
}

- (UIImageView *)nextTurnDistanceView
{
  if (!_nextTurnDistanceView)
  {
    _nextTurnDistanceView = [[UIImageView alloc] initWithFrame:CGRectZero];
    UIImage * image = [[UIImage imageNamed:@"RoutingButtonBackground"] resizableImageWithCapInsets:UIEdgeInsetsMake(5, 5, 5, 5)];
    _nextTurnDistanceView.image = image;
    _nextTurnDistanceView.autoresizingMask = UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin;
  }
  return _nextTurnDistanceView;
}

- (UIImageView *)turnTypeView
{
  if (!_turnTypeView)
  {
    _turnTypeView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, 44, 44)];
    _turnTypeView.autoresizingMask = UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin | UIViewAutoresizingFlexibleLeftMargin;
  }
  return _turnTypeView;
}

- (UIImageView *)turnView
{
  if (!_turnView)
  {
    _turnView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, BUTTON_HEIGHT, BUTTON_HEIGHT)];
    UIImage * image = [[UIImage imageNamed:@"RoutingButtonBackground"] resizableImageWithCapInsets:UIEdgeInsetsMake(5, 5, 5, 5)];
    _turnView.image = image;
    _turnView.autoresizingMask = UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin;
    self.turnTypeView.midX = _turnView.width / 2.0;
    self.turnTypeView.midY = _turnView.height / 2.0;
    [_turnView addSubview:self.turnTypeView];
  }
  return _turnView;
}

- (UIView *)wrapView
{
  if (!_wrapView)
    _wrapView = [[UIView alloc] initWithFrame:CGRectZero];
  return _wrapView;
}

- (UIImageView *)overallInfoView
{
  if (!_overallInfoView) {
    _overallInfoView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, BUTTON_HEIGHT, BUTTON_HEIGHT)];
    UIImage * image = [[UIImage imageNamed:@"RoutingButtonBackground"] resizableImageWithCapInsets:UIEdgeInsetsMake(5, 5, 5, 5)];
    _overallInfoView.image = image;
    _overallInfoView.autoresizingMask = UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin;
  }
  return _overallInfoView;
}

- (UILabel *)distanceLabel
{
  if (!_distanceLabel) {
    _distanceLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _distanceLabel.backgroundColor = [UIColor clearColor];
    _distanceLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:12];
    _distanceLabel.textColor = [UIColor blackColor];
  }
  return _distanceLabel;
}

- (UILabel *)timeLeftLabel
{
  if (!_timeLeftLabel) {
    _timeLeftLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _timeLeftLabel.backgroundColor = [UIColor clearColor];
    _timeLeftLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:12];
    _timeLeftLabel.textColor = [UIColor blackColor];
  }
  return _timeLeftLabel;
}

@end
