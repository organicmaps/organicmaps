
#import "RouteView.h"
#import "UIKitCategories.h"
#import "RouteOverallInfoView.h"
#import "NextTurnPhoneView.h"

@interface RouteView ()

@property (nonatomic) UIView * phoneIdiomView;

//@property (nonatomic) UIButton * closeButton;
//@property (nonatomic) UIButton * startButton;

@property (nonatomic) UIView * phoneTurnInstructions;
@property (nonatomic) RouteOverallInfoView * phoneOverallInfoView;
@property (nonatomic) NextTurnPhoneView * phoneNextTurnView;
@property (nonatomic) UIButton * phoneCloseTurnInstructionsButton;

@property (nonatomic) UIView * routeInfo;
@property (nonatomic) UIButton * startRouteButton;
@property (nonatomic) UIButton * closeRouteInfoButton;

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

  return self;
}

- (void)updateWithInfo:(NSDictionary *)info
{
  [self.phoneOverallInfoView updateWithInfo:info];
  [self.phoneNextTurnView updateWithInfo:info];
  
  [UIView animateWithDuration:0.2 animations:^{
    [self layoutSubviews];
  }];
}

- (UIImage *)turnImageWithType:(NSString *)turnType
{
  NSString * turnTypeImageName = [NSString stringWithFormat:@"big-%@", turnType];
  return [UIImage imageNamed:turnTypeImageName];
}

- (NSString *)secondsToString:(NSNumber *)seconds
{
  static NSDateFormatter * dateFormatter;
  if (!dateFormatter)
  {
    dateFormatter = [NSDateFormatter new];
    dateFormatter.dateStyle = NSDateFormatterNoStyle;
    dateFormatter.timeStyle = NSDateFormatterShortStyle;
  }
  NSDate * date = [NSDate dateWithTimeIntervalSinceNow:[seconds floatValue]];
  NSString * string = [dateFormatter stringFromDate:date];
  return string;
}

#define BUTTON_HEIGHT 48

- (void)layoutSubviews
{
  [self.phoneOverallInfoView layoutSubviews];
  [self.phoneNextTurnView layoutSubviews];
  
  CGFloat const offsetInnerX = 2;
  CGFloat const originY = 20;
  self.phoneTurnInstructions.width = self.phoneIdiomView.width;
  self.phoneTurnInstructions.height = 68;
  self.routeInfo.width = self.phoneTurnInstructions.width;
  self.routeInfo.height = self.phoneTurnInstructions.height;
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
//    CGFloat const overallInfoViewPadding = 12;
//    self.overallInfoView.width = MAX(self.distanceLabel.width, self.timeLeftLabel.width) + 2 * overallInfoViewPadding;
//    self.overallInfoView.minX = self.nextTurnDistanceView.maxX;
//    self.distanceLabel.minX = overallInfoViewPadding;
//    self.distanceLabel.minY = 10;
//    self.timeLeftLabel.minX = overallInfoViewPadding;
//    self.timeLeftLabel.maxY = self.timeLeftLabel.superview.height - 10;
}

- (void)didMoveToSuperview
{
  self.minY = [self viewMinY];
  [self setState:RouteViewStateHidden animated:NO];
}

- (void)closeButtonPressed:(id)sender
{
  [self.delegate routeViewDidCancelRouting:self];
}

- (void)startButtonPressed:(id)sender
{
  [self.delegate routeViewDidStartFollowing:self];
}

- (void)setState:(RouteViewState)state animated:(BOOL)animated
{
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

- (CGFloat)viewMinY
{
  return SYSTEM_VERSION_IS_LESS_THAN(@"7") ? -20 : 0;
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
    _phoneTurnInstructions.backgroundColor = [[UIColor whiteColor] colorWithAlphaComponent:0.94];;
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

- (UIButton *)startRouteButton
{
  if (!_startRouteButton)
  {
    NSString * title = L(@"routing_go");
    UIFont * font = [UIFont fontWithName:@"HelveticaNeue-Light" size:19];
    CGFloat const width = [title sizeWithDrawSize:CGSizeMake(200, 30) font:font].width + 38;

    _startRouteButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, width, BUTTON_HEIGHT)];
    _startRouteButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleBottomMargin;
    //UIImage * backgroundImage = [[UIImage imageNamed:@"StartRoutingButtonBackground"] resizableImageWithCapInsets:UIEdgeInsetsMake(5, 5, 5, 5)];
    //[_startRouteButton setBackgroundImage:backgroundImage forState:UIControlStateNormal];

    _startRouteButton.titleLabel.font = font;
    [_startRouteButton setTitle:title forState:UIControlStateNormal];
    [_startRouteButton setTitleColor:[UIColor colorWithColorCode:@"179E4D"] forState:UIControlStateNormal];

    [_startRouteButton addTarget:self action:@selector(startButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
  }
  return _startRouteButton;
}

- (UIView *)routeInfo
{
  if (!_routeInfo)
  {
    _routeInfo = [[UIView alloc] initWithFrame:CGRectZero];
    _routeInfo.backgroundColor = [[UIColor whiteColor] colorWithAlphaComponent:0.94];;
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

@end
