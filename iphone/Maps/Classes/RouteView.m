
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

  [self addSubview:self.distanceView];
  [self.distanceView addSubview:self.wrapView];
  [self.wrapView addSubview:self.distanceLabel];
  [self.wrapView addSubview:self.metricsLabel];
  [self addSubview:self.closeButton];
  [self addSubview:self.startButton];

  CGFloat const originY = 20;
  self.distanceView.minY = originY;
  self.closeButton.minY = originY;
  self.startButton.minY = originY;

  return self;
}

- (void)updateDistance:(NSString *)distance withMetrics:(NSString *)metrics
{
  self.distanceLabel.text = distance;
  self.metricsLabel.text = metrics;
  [self layoutSubviews];
}

#define BUTTON_HEIGHT 44

- (void)layoutSubviews
{
  [self.distanceLabel sizeToFit];
  [self.metricsLabel sizeToFit];

  CGFloat const betweenOffset = 0;
  self.wrapView.size = CGSizeMake(self.distanceLabel.width + betweenOffset + self.metricsLabel.width, 40);
  self.distanceLabel.midY = self.wrapView.height / 2;
  self.metricsLabel.maxY = self.distanceLabel.maxY - 4;
  self.distanceLabel.minX = 0;
  self.metricsLabel.minX = self.distanceLabel.minX + self.distanceLabel.width + betweenOffset;

  self.distanceView.size = CGSizeMake(self.wrapView.width + 24, BUTTON_HEIGHT);
  self.wrapView.center = CGPointMake(self.wrapView.superview.width / 2, self.wrapView.superview.height / 2);
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
  [self.delegate routeViewDidStartRouting:self];
}

- (void)setVisible:(BOOL)visible animated:(BOOL)animated
{
  _visible = visible;
  CGFloat const offsetInnerX = 3;
  CGFloat const offsetBetweenX = 0;
  CGFloat const offsetOuterX = 18;
  [UIView animateWithDuration:(animated ? 0.5 : 0) delay:0 damping:0.8 initialVelocity:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
    if (visible)
    {
      self.distanceView.minX = offsetInnerX;
      self.closeButton.maxX = self.width - offsetInnerX;
      self.startButton.maxX = self.closeButton.minX - offsetBetweenX;
    }
    else
    {
      self.distanceView.maxX = -offsetOuterX;
      self.startButton.minX = self.width + offsetOuterX;
      self.closeButton.maxX = self.startButton.maxX + offsetBetweenX;
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
    imageView.center = CGPointMake(_closeButton.width / 2, _closeButton.height / 2);

    [_closeButton addTarget:self action:@selector(closeButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
  }
  return _closeButton;
}

- (UIButton *)startButton
{
  if (!_startButton)
  {
    UIImageView * imageView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"StopRoutingButton"]];
    NSString * title = @"Поехали!";
    UIFont * font = [UIFont fontWithName:@"HelveticaNeue-Light" size:14];
    CGFloat const width = [title sizeWithDrawSize:CGSizeMake(200, 30) font:font].width + imageView.width + 40;

    _startButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, width, BUTTON_HEIGHT)];
    _startButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleBottomMargin;
    UIImage * backgroundImage = [[UIImage imageNamed:@"RoutingButtonBackground"] resizableImageWithCapInsets:UIEdgeInsetsMake(5, 5, 5, 5)];
    [_startButton setBackgroundImage:backgroundImage forState:UIControlStateNormal];

    [_startButton addSubview:imageView];
    imageView.center = CGPointMake(_startButton.width - imageView.width - 4, _startButton.height / 2);

    _startButton.titleLabel.font = font;
    _startButton.titleEdgeInsets = UIEdgeInsetsMake(0, 0, 0, imageView.width + 8);
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
    _distanceLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:34];
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
    _metricsLabel.font = [UIFont fontWithName:@"HelveticaNeue-Thin" size:14];
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
    [_distanceView addSubview:self.wrapView];
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
