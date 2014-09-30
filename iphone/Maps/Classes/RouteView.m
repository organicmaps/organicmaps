
#import "RouteView.h"
#import "UIKitCategories.h"

@interface RouteView ()

@property (nonatomic) UIButton * closeButton;
@property (nonatomic) UILabel * distanceLabel;
@property (nonatomic) UIView * wrapView;
@property (nonatomic) UILabel * metricsLabel;
@property (nonatomic) UIImageView * backgroundView;
@property (nonatomic, readonly) BOOL visible;

@end

@implementation RouteView

- (instancetype)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];

  [self addSubview:self.backgroundView];
  [self addSubview:self.wrapView];
  [self.wrapView addSubview:self.distanceLabel];
  [self.wrapView addSubview:self.metricsLabel];
  [self addSubview:self.closeButton];

  return self;
}

- (void)updateDistance:(NSString *)distance withMetrics:(NSString *)metrics
{
  self.distanceLabel.text = distance;
  self.metricsLabel.text = metrics;
  [self layoutSubviews];
}

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
  self.wrapView.center = CGPointMake(self.width / 2, self.height - 41);

  self.closeButton.center = CGPointMake(self.width - 22, self.height - 40);
}

- (void)didMoveToSuperview
{
  [self setVisible:NO animated:NO];
}

- (void)closeButtonPressed:(id)sender
{
  [self.delegate routeViewDidCancelRouting:self];
}

- (void)setVisible:(BOOL)visible animated:(BOOL)animated
{
  _visible = visible;
  [UIView animateWithDuration:(animated ? 0.5 : 0) delay:0 damping:0.9 initialVelocity:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
    if (visible)
      self.minY = [self viewMinY];
    else
      self.maxY = 0;
  } completion:nil];
}

- (CGFloat)viewMinY
{
  return SYSTEM_VERSION_IS_LESS_THAN(@"7") ? -20 : 0;
}

- (UIImageView *)backgroundView
{
  if (!_backgroundView)
  {
    _backgroundView = [[UIImageView alloc] initWithFrame:self.bounds];
    _backgroundView.image = [[UIImage imageNamed:@"PlacePageBackground"] resizableImageWithCapInsets:UIEdgeInsetsMake(0, 0, 18, 0)];
    _backgroundView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  }
  return _backgroundView;
}

- (UIButton *)closeButton
{
  if (!_closeButton)
  {
    _closeButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 50, 44)];
    _closeButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleBottomMargin;
    [_closeButton setImage:[UIImage imageNamed:@"PlacePageCancelRouteButton"] forState:UIControlStateNormal];
    [_closeButton addTarget:self action:@selector(closeButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
  }
  return _closeButton;
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

- (UIView *)wrapView
{
  if (!_wrapView)
    _wrapView = [[UIView alloc] initWithFrame:CGRectZero];
  return _wrapView;
}

@end
