#import "MWMTrafficButtonViewController.h"
#import "Common.h"
#import "MWMButton.h"
#import "MWMMapViewControlsCommon.h"
#import "MWMMapViewControlsManager.h"
#import "MapViewController.h"
#import "UIColor+MapsMeColor.h"

typedef NS_ENUM(NSUInteger, MWMTrafficButtonState) {
  MWMTrafficButtonStateOff,
  MWMTrafficButtonStateOn,
  MWMTrafficButtonStateUpdate
};

namespace
{
CGFloat const kTopOffset = 26;
CGFloat const kTopShiftedOffset = 6;
}  // namespace

@interface MWMMapViewControlsManager ()

@property(nonatomic) MWMTrafficButtonViewController * trafficButton;

@end

@interface MWMTrafficButtonViewController ()

@property(nonatomic) NSLayoutConstraint * topOffset;
@property(nonatomic) NSLayoutConstraint * leftOffset;
@property(nonatomic) MWMTrafficButtonState state;

@end

@implementation MWMTrafficButtonViewController

+ (MWMTrafficButtonViewController *)controller
{
  return [MWMMapViewControlsManager manager].trafficButton;
}

- (instancetype)init
{
  self = [super init];
  if (self)
  {
    MapViewController * ovc = [MapViewController controller];
    [ovc addChildViewController:self];
    [ovc.view addSubview:self.view];
    [self configLayout];
    self.state = MWMTrafficButtonStateOff;
  }
  return self;
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  [self refreshAppearance];
}

- (void)configLayout
{
  UIView * sv = self.view;
  UIView * ov = sv.superview;

  self.topOffset = [NSLayoutConstraint constraintWithItem:sv
                                                attribute:NSLayoutAttributeTop
                                                relatedBy:NSLayoutRelationEqual
                                                   toItem:ov
                                                attribute:NSLayoutAttributeTop
                                               multiplier:1
                                                 constant:kTopOffset];
  self.leftOffset = [NSLayoutConstraint constraintWithItem:sv
                                                 attribute:NSLayoutAttributeLeading
                                                 relatedBy:NSLayoutRelationEqual
                                                    toItem:ov
                                                 attribute:NSLayoutAttributeLeading
                                                multiplier:1
                                                  constant:kViewControlsOffsetToBounds];

  [ov addConstraints:@[ self.topOffset, self.leftOffset ]];
}

- (void)mwm_refreshUI
{
  [self.view mwm_refreshUI];
  [self refreshAppearance];
}

- (void)setHidden:(BOOL)hidden
{
  _hidden = hidden;
  [self refreshLayout];
}

- (void)setTopBound:(CGFloat)topBound
{
  if (_topBound == topBound)
    return;
  _topBound = topBound;
  [self refreshLayout];
}

- (void)setLeftBound:(CGFloat)leftBound
{
  if (_leftBound == leftBound)
    return;
  _leftBound = leftBound;
  [self refreshLayout];
}

- (void)setState:(MWMTrafficButtonState)state
{
  _state = state;
  [self refreshAppearance];
}

- (void)refreshLayout
{
  runAsyncOnMainQueue(^{
    CGFloat const topOffset = self.topBound > 0 ? self.topBound + kTopShiftedOffset : kTopOffset;
    CGFloat const leftOffset =
        self.hidden ? -self.view.width : self.leftBound + kViewControlsOffsetToBounds;
    UIView * ov = self.view.superview;
    [ov layoutIfNeeded];
    self.topOffset.constant = topOffset;
    self.leftOffset.constant = leftOffset;
    [UIView animateWithDuration:kDefaultAnimationDuration
                     animations:^{
                       [ov layoutIfNeeded];
                     }];
  });
}

- (void)refreshAppearance
{
  MWMButton * btn = static_cast<MWMButton *>(self.view);
  switch (self.state)
  {
  case MWMTrafficButtonStateOff: btn.imageName = @"btn_traffic_off"; break;
  case MWMTrafficButtonStateOn: btn.imageName = @"btn_traffic_on"; break;
  case MWMTrafficButtonStateUpdate:
  {
    NSUInteger const imagesCount = 3;
    NSMutableArray<UIImage *> * images = [NSMutableArray arrayWithCapacity:imagesCount];
    NSString * mode = [UIColor isNightMode] ? @"dark" : @"light";
    for (NSUInteger i = 1; i <= imagesCount; i += 1)
    {
      NSString * name =
          [NSString stringWithFormat:@"btn_traffic_update_%@_%@", mode, @(i).stringValue];
      [images addObject:[UIImage imageNamed:name]];
    }
    UIImageView * iv = btn.imageView;
    iv.animationImages = images;
    iv.animationDuration = 0.8;
    iv.image = images.lastObject;
    [iv startAnimating];
    break;
  }
  }
}

@end
