#import "Common.h"
#import "NextTurnPhoneView.h"
#import "RouteOverallInfoView.h"
#import "RouteView.h"
#import "TimeUtils.h"
#import "UIKitCategories.h"

#import "../../3party/Alohalytics/src/alohalytics_objc.h"

extern NSString * const kAlohalyticsTapEventKey;

@interface RouteView ()

@property (nonatomic) UIView * phoneIdiomView;

@property (nonatomic) UIView * phoneTurnInstructions;
@property (nonatomic) RouteOverallInfoView * phoneOverallInfoView;
@property (nonatomic) NextTurnPhoneView * phoneNextTurnView;
@property (nonatomic) UIButton * phoneCloseTurnInstructionsButton;

@property (nonatomic) UIView * routeInfo;
@property (nonatomic) UIButton * startRouteButton;
@property (nonatomic) UIButton * closeRouteInfoButton;
@property (nonatomic) UILabel * distanceLabel;
@property (nonatomic) UILabel * metricsLabel;
@property (nonatomic) UILabel * timeLeftLabel;

@property (nonatomic) UIView * tabletIdiomView;

@end

@implementation RouteView

- (instancetype)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];
  self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleBottomMargin;

  [self addSubview:self.phoneIdiomView];

  [self.phoneIdiomView addSubview:self.phoneTurnInstructions];
  [self.phoneTurnInstructions addSubview:self.phoneOverallInfoView];
  [self.phoneTurnInstructions addSubview:self.phoneNextTurnView];
  [self.phoneTurnInstructions addSubview:self.phoneCloseTurnInstructionsButton];
  
  [self.phoneIdiomView addSubview:self.routeInfo];
  [self.routeInfo addSubview:self.closeRouteInfoButton];
  [self.routeInfo addSubview:self.startRouteButton];
  [self.routeInfo addSubview:self.distanceLabel];
  [self.routeInfo addSubview:self.metricsLabel];
  [self.routeInfo addSubview:self.timeLeftLabel];

  return self;
}

- (void)updateWithInfo:(NSDictionary *)info
{
  [self.phoneOverallInfoView updateWithInfo:info];
  [self.phoneNextTurnView updateWithInfo:info];
  
  self.distanceLabel.text = info[@"targetDistance"];
  self.metricsLabel.text = [info[@"targetMetrics"] uppercaseString];
  self.timeLeftLabel.text = [NSDateFormatter estimatedArrivalTimeWithSeconds:info[@"timeToTarget"]];
  
  [UIView animateWithDuration:0.2 animations:^{
    [self updateSubviews];
  }];
}

- (UIImage *)turnImageWithType:(NSString *)turnType
{
  NSString * turnTypeImageName = [NSString stringWithFormat:@"big-%@", turnType];
  return [UIImage imageNamed:turnTypeImageName];
}

#define BUTTON_HEIGHT 48

- (void)setFrame:(CGRect)frame
{
  [super setFrame:frame];
  [self configureSubviews];
}

- (void)configureSubviews
{
  CGFloat const offsetInnerX = 2;
  CGFloat const originY = 20;
  CGFloat const instructionsViewHeight = 68;
  CGFloat const routeInfoViewHeight = 68;
  self.phoneTurnInstructions.width = self.phoneIdiomView.width;
  self.phoneTurnInstructions.height = instructionsViewHeight;
  self.routeInfo.width = self.phoneTurnInstructions.width;
  self.routeInfo.height = routeInfoViewHeight;
  self.phoneNextTurnView.frame = self.phoneTurnInstructions.bounds;
  self.phoneNextTurnView.minY = originY;
  self.phoneNextTurnView.height -= originY;
  
  self.phoneOverallInfoView.width = self.phoneTurnInstructions.width - 26;
  
  self.phoneCloseTurnInstructionsButton.minY = originY;
  self.phoneCloseTurnInstructionsButton.maxX = self.phoneTurnInstructions.width - offsetInnerX;
  
  self.closeRouteInfoButton.maxY = self.routeInfo.height;
  self.closeRouteInfoButton.maxX = self.routeInfo.width - offsetInnerX;
  
  self.startRouteButton.maxY = self.routeInfo.height;
  self.startRouteButton.maxX = self.closeRouteInfoButton.minX - 6;
  
  [self updateSubviews];
}

- (void)updateSubviews
{
  [self.distanceLabel sizeToIntegralFit];
  self.distanceLabel.minX = 10;
  self.distanceLabel.minY = 34;
  [self.metricsLabel sizeToIntegralFit];
  self.metricsLabel.minX = self.distanceLabel.maxX + 2;
  self.metricsLabel.maxY = self.distanceLabel.maxY - 2;
  [self.timeLeftLabel sizeToIntegralFit];
  self.timeLeftLabel.minX = 96;
  self.timeLeftLabel.maxY = self.distanceLabel.maxY;
}

- (void)didMoveToSuperview
{
  self.minY = 0.0;
  [self setState:RouteViewStateHidden animated:NO];
}

- (void)closeButtonPressed:(id)sender
{
  switch (self.state)
  {
    case RouteViewStateInfo:
      [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"routeClose"];
      break;
    case RouteViewStateTurnInstructions:
      [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"routeGoClose"];
      break;
    default:
      break;
  }
  [self.delegate routeViewDidCancelRouting:self];
}

- (void)startButtonPressed:(id)sender
{
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"routeGo"];
  [self.delegate routeViewDidStartFollowing:self];
}

- (void)setState:(RouteViewState)state animated:(BOOL)animated
{
  [self.delegate routeViewWillEnterState:state];
  _state = state;
  
  [UIView animateWithDuration:(animated ? 0.5 : 0) delay:0 damping:0.83 initialVelocity:0 options:UIViewAnimationOptionCurveEaseInOut animations:^ {
    self.phoneTurnInstructions.userInteractionEnabled = NO;
    self.phoneTurnInstructions.alpha = 0.0;
    self.phoneTurnInstructions.minY = - self.phoneTurnInstructions.height;
    self.routeInfo.userInteractionEnabled = NO;
    self.routeInfo.alpha = 0.0;
    self.routeInfo.minY = self.phoneTurnInstructions.minY;
    
    switch (state)
    {
      case RouteViewStateHidden:
      {
        break;
      }
      case RouteViewStateInfo:
      {
        self.routeInfo.userInteractionEnabled = YES;
        self.routeInfo.alpha = 1.0;
        self.routeInfo.minY = 0;
        break;
      }
      case RouteViewStateTurnInstructions:
      {
        self.phoneTurnInstructions.userInteractionEnabled = YES;
        self.phoneTurnInstructions.alpha = 1.0;
        self.phoneTurnInstructions.minY = 0;
        break;
      }
      default:
        break;
    }
  } completion:nil];
}

- (UIView *)phoneIdiomView
{
  if (!_phoneIdiomView)
  {
    _phoneIdiomView = [[UIView alloc] initWithFrame:self.bounds];
    _phoneIdiomView.backgroundColor = [UIColor clearColor];
    _phoneIdiomView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
  }
  return _phoneIdiomView;
}

- (UIView *)phoneTurnInstructions
{
  if (!_phoneTurnInstructions)
  {
    _phoneTurnInstructions = [[UIView alloc] initWithFrame:CGRectZero];
    _phoneTurnInstructions.backgroundColor = [[UIColor whiteColor] colorWithAlphaComponent:0.94];
    _phoneTurnInstructions.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleBottomMargin;
    
    UIImageView * shadow = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, _phoneTurnInstructions.width, 18)];
    shadow.image = [[UIImage imageNamed:@"PlacePageShadow"] resizableImageWithCapInsets:UIEdgeInsetsZero];
    shadow.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin;
    shadow.minY = _phoneTurnInstructions.height;
    [_phoneTurnInstructions addSubview:shadow];
  }
  return _phoneTurnInstructions;
}

- (UIButton *)phoneCloseTurnInstructionsButton
{
  if (!_phoneCloseTurnInstructionsButton)
  {
    _phoneCloseTurnInstructionsButton = [self closeButton];
  }
  return _phoneCloseTurnInstructionsButton;
}

- (RouteOverallInfoView *)phoneOverallInfoView
{
  if (!_phoneOverallInfoView)
  {
    _phoneOverallInfoView = [[RouteOverallInfoView alloc] initWithFrame:CGRectMake(12, 22, 32, 32)];
    _phoneOverallInfoView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleBottomMargin;
  }
  return _phoneOverallInfoView;
}

- (NextTurnPhoneView *)phoneNextTurnView
{
  if (!_phoneNextTurnView)
  {
    _phoneNextTurnView = [[NextTurnPhoneView alloc] initWithFrame:self.phoneTurnInstructions.bounds];
    _phoneNextTurnView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin;
  }
  return _phoneNextTurnView;
}

- (UIButton *)closeButton
{
  UIButton * closeButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 50, BUTTON_HEIGHT)];
  closeButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleBottomMargin;
  closeButton.backgroundColor = [UIColor clearColor];
  
  UIImageView * imageView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"StopRoutingButton"]];
  [closeButton addSubview:imageView];
  imageView.center = CGPointMake(closeButton.width / 2, closeButton.height / 2 + 2);
  imageView.frame = CGRectIntegral(imageView.frame);
  
  [closeButton addTarget:self action:@selector(closeButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
  
  return closeButton;
}

- (UIView *)routeInfo
{
  if (!_routeInfo)
  {
    _routeInfo = [[UIView alloc] initWithFrame:CGRectZero];
    _routeInfo.backgroundColor = [[UIColor whiteColor] colorWithAlphaComponent:0.94];
    _routeInfo.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleBottomMargin;
    
    UIImageView * shadow = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, _routeInfo.width, 18)];
    shadow.image = [[UIImage imageNamed:@"PlacePageShadow"] resizableImageWithCapInsets:UIEdgeInsetsZero];
    shadow.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin;
    shadow.minY = _routeInfo.height;
    [_routeInfo addSubview:shadow];
  }
  return _routeInfo;
}

- (UIButton *)closeRouteInfoButton
{
  if (!_closeRouteInfoButton)
  {
    _closeRouteInfoButton = [self closeButton];
  }
  return _closeRouteInfoButton;
}

- (UIButton *)startRouteButton
{
  if (!_startRouteButton)
  {
    NSString * title = L(@"routing_go");
    UIFont * font = [UIFont fontWithName:@"HelveticaNeue-Light" size:19];
    CGFloat const width = [title sizeWithDrawSize:CGSizeMake(200, 30) font:font].width + 20;
    
    _startRouteButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, width, BUTTON_HEIGHT)];
    _startRouteButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleBottomMargin;
    _startRouteButton.titleLabel.font = font;
    _startRouteButton.titleEdgeInsets = UIEdgeInsetsMake(2, 0, 0, 0);
    [_startRouteButton setTitle:title forState:UIControlStateNormal];
    [_startRouteButton setTitleColor:[UIColor colorWithColorCode:@"179E4D"] forState:UIControlStateNormal];
    
    UIView * underline = [[UIView alloc] initWithFrame:CGRectMake(0, _startRouteButton.height - 2, _startRouteButton.width, 2)];
    underline.autoresizingMask = UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleWidth;
    underline.backgroundColor = [UIColor colorWithColorCode:@"179E4D"];
    [_startRouteButton addSubview:underline];
    
    [_startRouteButton addTarget:self action:@selector(startButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
  }
  return _startRouteButton;
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

- (UILabel *)distanceLabel
{
  if (!_distanceLabel)
  {
    _distanceLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _distanceLabel.backgroundColor = [UIColor clearColor];
    _distanceLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:19];
    _distanceLabel.textColor = [UIColor blackColor];
  }
  return _distanceLabel;
}

- (UILabel *)timeLeftLabel
{
  if (!_timeLeftLabel)
  {
    _timeLeftLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _timeLeftLabel.backgroundColor = [UIColor clearColor];
    _timeLeftLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:19];
    _timeLeftLabel.textColor = [UIColor blackColor];
  }
  return _timeLeftLabel;
}

@end
