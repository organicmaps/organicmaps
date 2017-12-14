#import "MWMRoutePreview.h"
#import "MWMCircularProgress.h"
#import "MWMCommon.h"
#import "MWMLocationManager.h"
#import "MWMNavigationDashboardEntity.h"
#import "MWMNavigationDashboardManager.h"
#import "MWMRouter.h"
#import "MWMTaxiPreviewDataSource.h"
#import "Statistics.h"
#import "SwiftBridge.h"
#import "UIButton+Orientation.h"
#import "UIImageView+Coloring.h"

#include "platform/platform.hpp"

@interface MWMRoutePreview ()<MWMCircularProgressProtocol>

@property(nonatomic) BOOL isVisible;
@property(weak, nonatomic) IBOutlet UIButton * backButton;
@property(weak, nonatomic) IBOutlet UIView * bicycle;
@property(weak, nonatomic) IBOutlet UIView * contentView;
@property(weak, nonatomic) IBOutlet UIView * pedestrian;
@property(weak, nonatomic) IBOutlet UIView * publicTransport;
@property(weak, nonatomic) IBOutlet UIView * taxi;
@property(weak, nonatomic) IBOutlet UIView * vehicle;

@end

@implementation MWMRoutePreview
{
  map<MWMRouterType, MWMCircularProgress *> m_progresses;
}

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.translatesAutoresizingMaskIntoConstraints = NO;
  [self setupProgresses];
  [self.backButton matchInterfaceOrientation];
}

- (void)setupProgresses
{
  [self addProgress:self.vehicle imageName:@"ic_car" routerType:MWMRouterTypeVehicle];
  [self addProgress:self.pedestrian imageName:@"ic_pedestrian" routerType:MWMRouterTypePedestrian];
  [self addProgress:self.publicTransport
          imageName:@"ic_train"
         routerType:MWMRouterTypePublicTransport];
  [self addProgress:self.bicycle imageName:@"ic_bike" routerType:MWMRouterTypeBicycle];
  [self addProgress:self.taxi imageName:@"ic_taxi" routerType:MWMRouterTypeTaxi];
}

- (void)addProgress:(UIView *)parentView
          imageName:(NSString *)imageName
         routerType:(MWMRouterType)routerType
{
  MWMCircularProgress * progress = [[MWMCircularProgress alloc] initWithParentView:parentView];
  MWMCircularProgressStateVec const imageStates = {MWMCircularProgressStateNormal,
    MWMCircularProgressStateProgress, MWMCircularProgressStateSpinner};

  [progress setImageName:imageName forStates:imageStates];
  [progress setImageName:[imageName stringByAppendingString:@"_selected"] forStates:{MWMCircularProgressStateSelected, MWMCircularProgressStateCompleted}];
  [progress setImageName:@"ic_error" forStates:{MWMCircularProgressStateFailed}];

  [progress setColoring:MWMButtonColoringWhiteText
              forStates:{MWMCircularProgressStateFailed, MWMCircularProgressStateSelected,
                         MWMCircularProgressStateProgress, MWMCircularProgressStateSpinner,
                         MWMCircularProgressStateCompleted}];

  [progress setSpinnerBackgroundColor:UIColor.clearColor];
  [progress setColor:UIColor.whiteColor
           forStates:{MWMCircularProgressStateProgress, MWMCircularProgressStateSpinner}];

  progress.delegate = self;
  m_progresses[routerType] = progress;
}

- (void)statePrepare
{
  for (auto const & progress : m_progresses)
    progress.second.state = MWMCircularProgressStateNormal;

  if (!MWMLocationManager.lastLocation || !Platform::IsConnected())
    [self.taxi removeFromSuperview];
}

- (void)selectRouter:(MWMRouterType)routerType
{
  for (auto const & progress : m_progresses)
    progress.second.state = MWMCircularProgressStateNormal;
  m_progresses[routerType].state = MWMCircularProgressStateSelected;
}

- (void)router:(MWMRouterType)routerType setState:(MWMCircularProgressState)state
{
  m_progresses[routerType].state = state;
}

- (void)router:(MWMRouterType)routerType setProgress:(CGFloat)progress
{
  m_progresses[routerType].progress = progress;
}

#pragma mark - MWMCircularProgressProtocol

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress
{
  [Statistics logEvent:kStatEventName(kStatNavigationDashboard, kStatButton)
        withParameters:@{kStatValue : kStatProgress}];
  MWMCircularProgressState const s = progress.state;
  if (s == MWMCircularProgressStateSelected || s == MWMCircularProgressStateCompleted)
    return;

  for (auto const & prg : m_progresses)
  {
    if (prg.second != progress)
      continue;
    auto const routerType = prg.first;
    [self selectRouter:routerType];
    [MWMRouter setType:routerType];
    [MWMRouter rebuildWithBestRouter:NO];
    NSString * routerTypeString = nil;
    switch (routerType)
    {
    case MWMRouterTypeVehicle: routerTypeString = kStatVehicle; break;
    case MWMRouterTypePedestrian: routerTypeString = kStatPedestrian; break;
    case MWMRouterTypePublicTransport: routerTypeString = kStatPublicTransport; break;
    case MWMRouterTypeBicycle: routerTypeString = kStatBicycle; break;
    case MWMRouterTypeTaxi: routerTypeString = kStatTaxi; break;
    }
    [Statistics logEvent:kStatPointToPoint
          withParameters:@{kStatAction : kStatChangeRoutingMode, kStatValue : routerTypeString}];
  }
}

- (void)addToView:(UIView *)superview
{
  NSAssert(superview != nil, @"Superview can't be nil");
  if ([superview.subviews containsObject:self])
    return;
  [superview addSubview:self];
  [self setupConstraints];
  dispatch_async(dispatch_get_main_queue(), ^{
    self.isVisible = YES;
  });
}

- (void)remove { self.isVisible = NO; }

- (void)setupConstraints {}

- (void)layoutSubviews
{
  [super layoutSubviews];
  [self.vehicle setNeedsLayout];
}

#pragma mark - Properties

- (void)setIsVisible:(BOOL)isVisible
{
  _isVisible = isVisible;
  auto sv = self.superview;
  [sv setNeedsLayout];
  [UIView animateWithDuration:kDefaultAnimationDuration
                   animations:^{ [sv layoutIfNeeded]; }
                   completion:^(BOOL finished) {
                     if (!self.isVisible)
                       [self removeFromSuperview];
                   }];
}

@end
