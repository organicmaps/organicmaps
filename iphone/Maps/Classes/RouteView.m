
#import "RouteView.h"
#import "UIKitCategories.h"

@interface RouteView ()

@property (nonatomic) UIButton * closeButton;
@property (nonatomic) UIButton * startButton;
@property (nonatomic) UILabel * distanceLabel;
@property (nonatomic) UIView * wrapView;
@property (nonatomic) UILabel * metricsLabel;
@property (nonatomic) UIImageView * backgroundView;
@property (nonatomic) UIImageView * distanceView;

@end

@implementation RouteView

- (instancetype)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];
  self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleBottomMargin;

  [self addSubview:self.distanceView];
  [self.distanceView addSubview:self.wrapView];
  [self.wrapView addSubview:self.distanceLabel];
  [self.wrapView addSubview:self.metricsLabel];
  [self addSubview:self.closeButton];
  [self addSubview:self.startButton];

  return self;
}

- (void)updateDistance:(NSString *)distance withMetrics:(NSString *)metrics
{
  self.distanceLabel.text = distance;
  self.metricsLabel.text = metrics.uppercaseString;
  [UIView animateWithDuration:0.2 animations:^{
    [self layoutSubviews];
  }];
}

#define BUTTON_HEIGHT 51

- (void)layoutSubviews
{
  if (self.distanceLabel.text.length == 0)
    self.distanceView.alpha = 0;
  else
  {
    [self.distanceLabel sizeToIntegralFit];
    [self.metricsLabel sizeToIntegralFit];

    CGFloat const betweenOffset = 2;
    self.wrapView.size = CGSizeMake(self.distanceLabel.width + betweenOffset + self.metricsLabel.width, 40);
    self.wrapView.center = CGPointMake(self.wrapView.superview.width / 2, self.wrapView.superview.height / 2);
    self.wrapView.frame = CGRectIntegral(self.wrapView.frame);

    self.distanceLabel.minX = 0;
    self.metricsLabel.minX = self.distanceLabel.minX + self.distanceLabel.width + betweenOffset;
    self.distanceLabel.midY = self.wrapView.height / 2 ;
    self.metricsLabel.maxY = self.distanceLabel.maxY - 5;

    self.distanceView.alpha = 1;
    self.distanceView.size = CGSizeMake(self.wrapView.width + 24, BUTTON_HEIGHT);
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
    self.distanceView.minX = offsetInnerX;
    self.closeButton.maxX = self.width - offsetInnerX;
    self.startButton.maxX = self.closeButton.minX;
    if (visible)
    {
      self.startButton.userInteractionEnabled = YES;
      self.startButton.minY = originY - offsetInnerY;
      self.distanceView.minY = self.startButton.minY;
      self.closeButton.minY = self.startButton.minY;
    }
    else
    {
      self.startButton.maxY = -30;
      self.distanceView.maxY = self.startButton.maxY;
      self.closeButton.maxY = self.startButton.maxY;
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

- (UILabel *)distanceLabel
{
  if (!_distanceLabel)
  {
    _distanceLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _distanceLabel.backgroundColor = [UIColor clearColor];
    _distanceLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:34];
    _distanceLabel.textColor = [UIColor blackColor];
  }
  return _distanceLabel;
}

- (UILabel *)metricsLabel
{
  if (!_metricsLabel)
  {
    _metricsLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _metricsLabel.backgroundColor = [UIColor clearColor];
    _metricsLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:11];
    _metricsLabel.textColor = [UIColor blackColor];
  }
  return _metricsLabel;
}

- (UIImageView *)distanceView
{
  if (!_distanceView)
  {
    _distanceView = [[UIImageView alloc] initWithFrame:CGRectZero];
    UIImage * image = [[UIImage imageNamed:@"RoutingButtonBackground"] resizableImageWithCapInsets:UIEdgeInsetsMake(5, 5, 5, 5)];
    _distanceView.image = image;
    _distanceView.autoresizingMask = UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin;
  }
  return _distanceView;
}

- (UIView *)wrapView
{
  if (!_wrapView)
    _wrapView = [[UIView alloc] initWithFrame:CGRectZero];
  return _wrapView;
}

@end
