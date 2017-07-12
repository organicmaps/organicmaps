#import "MWMRoutePreview.h"
#import "MWMCircularProgress.h"
#import "MWMCommon.h"
#import "MWMLocationManager.h"
#import "MWMNavigationDashboardManager.h"
#import "MWMRouter.h"
#import "MWMTaxiPreviewDataSource.h"
#import "Statistics.h"
#import "SwiftBridge.h"
#import "UIButton+Orientation.h"
#import "UIImageView+Coloring.h"

#include "platform/platform.hpp"

namespace
{
CGFloat constexpr kAdditionalHeight = 20.;
}  // namespace

@interface MWMRoutePreview ()<MWMCircularProgressProtocol>

@property(weak, nonatomic) IBOutlet UIButton * backButton;
@property(weak, nonatomic) IBOutlet UIView * pedestrian;
@property(weak, nonatomic) IBOutlet UIView * vehicle;
@property(weak, nonatomic) IBOutlet UIView * bicycle;
@property(weak, nonatomic) IBOutlet UIView * taxi;
@property(weak, nonatomic) IBOutlet UIButton * goButton;
@property(weak, nonatomic) IBOutlet UIView * statusBox;
@property(weak, nonatomic) IBOutlet UIView * planningBox;
@property(weak, nonatomic) IBOutlet UIView * resultsBox;
@property(weak, nonatomic) IBOutlet UIView * heightBox;
@property(weak, nonatomic) IBOutlet UIView * taxiBox;
@property(weak, nonatomic) IBOutlet UIView * errorBox;
@property(weak, nonatomic) IBOutlet UILabel * errorLabel;
@property(weak, nonatomic) IBOutlet UILabel * resultLabel;
@property(weak, nonatomic) IBOutlet UILabel * arriveLabel;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * statusBoxHeight;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * resultsBoxHeight;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * heightBoxHeight;
@property(weak, nonatomic) IBOutlet UIImageView * heightProfileImage;
@property(weak, nonatomic) IBOutlet UIView * heightProfileElevation;
@property(weak, nonatomic) IBOutlet UIImageView * elevationImage;
@property(weak, nonatomic) IBOutlet UILabel * elevationHeight;
@property(weak, nonatomic) IBOutlet MWMTaxiCollectionView * taxiCollectionView;

@property(nonatomic) UIImageView * movingCellImage;

@property(nonatomic) BOOL isNeedToMove;
@property(nonatomic) NSIndexPath * indexPathOfMovingCell;

@end

@implementation MWMRoutePreview
{
  map<MWMRouterType, MWMCircularProgress *> m_progresses;
}

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  self.layer.shouldRasterize = YES;
  self.layer.rasterizationScale = UIScreen.mainScreen.scale;
  [self setupProgresses];

  [self.backButton matchInterfaceOrientation];

  self.elevationImage.mwm_coloring = MWMImageColoringBlue;
}

- (void)setupProgresses
{
  [self addProgress:self.vehicle imageName:@"ic_car" routerType:MWMRouterTypeVehicle];
  [self addProgress:self.pedestrian imageName:@"ic_pedestrian" routerType:MWMRouterTypePedestrian];
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

  [progress setSpinnerBackgroundColor:[UIColor clearColor]];
  [progress setColor:[UIColor whiteColor]
           forStates:{MWMCircularProgressStateProgress, MWMCircularProgressStateSpinner}];

  progress.delegate = self;
  m_progresses[routerType] = progress;
}

- (void)didMoveToSuperview { [self setupActualHeight]; }
- (void)statePrepare
{
  [[MWMNavigationDashboardManager manager] updateStartButtonTitle:self.goButton];
  for (auto const & progress : m_progresses)
    progress.second.state = MWMCircularProgressStateNormal;

  self.goButton.hidden = NO;
  self.goButton.enabled = NO;
  [self setupActualHeight];
  self.statusBox.hidden = YES;
  self.resultsBox.hidden = YES;
  self.heightBox.hidden = YES;
  self.heightProfileElevation.hidden = YES;
  self.planningBox.hidden = YES;
  self.errorBox.hidden = YES;
  self.taxiBox.hidden = YES;

  if (!MWMLocationManager.lastLocation || !Platform::IsConnected())
    [self.taxi removeFromSuperview];
}

- (void)stateError
{
  [[MWMNavigationDashboardManager manager] updateStartButtonTitle:self.goButton];
  self.goButton.hidden = NO;
  self.goButton.enabled = NO;
  self.statusBox.hidden = NO;
  self.planningBox.hidden = YES;
  self.resultsBox.hidden = YES;
  self.heightBox.hidden = YES;
  self.taxiBox.hidden = YES;
  self.heightProfileElevation.hidden = YES;
  self.errorBox.hidden = NO;
  if (IPAD)
    [self iPadNotReady];
}

- (void)stateReady
{
  [[MWMNavigationDashboardManager manager] updateStartButtonTitle:self.goButton];
  self.goButton.hidden = NO;
  self.goButton.enabled = YES;
  self.statusBox.hidden = NO;
  self.planningBox.hidden = YES;
  self.errorBox.hidden = YES;
  self.resultsBox.hidden = NO;
  BOOL const hasAltitude = [MWMRouter hasRouteAltitude];
  self.heightBox.hidden = !hasAltitude;
  self.heightProfileElevation.hidden = !hasAltitude;
  if (IPAD)
    [self iPadReady];
}

- (void)iPadReady
{
  [self layoutIfNeeded];
  if ([MWMRouter isTaxi])
  {
    self.statusBoxHeight.constant = self.taxiBox.height;
    self.taxiBox.hidden = NO;
  }
  else
  {
    self.statusBoxHeight.constant =
        self.resultsBoxHeight.constant +
        ([MWMRouter hasRouteAltitude] ? self.heightBoxHeight.constant : 0);
    [UIView animateWithDuration:kDefaultAnimationDuration animations:^
    {
      [self layoutIfNeeded];
    }
    completion:^(BOOL finished)
    {
      [UIView animateWithDuration:kDefaultAnimationDuration animations:^
      {
        self.arriveLabel.alpha = 1.;
      }
      completion:^(BOOL finished)
      {
        [self updateHeightProfile];
      }];
    }];
  }
}

- (void)updateHeightProfile
{
  if (![MWMRouter hasRouteAltitude])
    return;
  dispatch_async(dispatch_get_main_queue(), ^{
    [MWMRouter routeAltitudeImageForSize:self.heightProfileImage.frame.size
                              completion:^(UIImage * image, NSString * altitudeElevation) {
                                self.heightProfileImage.image = image;
                                self.elevationHeight.text = altitudeElevation;
                              }];
  });
}

- (void)iPadNotReady
{
  self.taxiBox.hidden = YES;
  self.errorLabel.text = [MWMRouter isTaxi] ? L(@"taxi_not_found") : L(@"routing_planning_error");
  self.arriveLabel.alpha = 0.;
  [self layoutIfNeeded];
  self.statusBoxHeight.constant = self.resultsBoxHeight.constant;
  [UIView animateWithDuration:kDefaultAnimationDuration
                   animations:^{
                     [self layoutIfNeeded];
                   }];
}

- (void)selectRouter:(MWMRouterType)routerType
{
  for (auto const & progress : m_progresses)
    progress.second.state = MWMCircularProgressStateNormal;
  m_progresses[routerType].state = MWMCircularProgressStateSelected;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  [self setupActualHeight];
  [self.delegate routePreviewDidChangeFrame:self.frame];
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
    switch (routerType)
    {
    case MWMRouterTypeVehicle:
      [Statistics logEvent:kStatPointToPoint
            withParameters:@{kStatAction : kStatChangeRoutingMode, kStatValue : kStatVehicle}];
      break;
    case MWMRouterTypePedestrian:
      [Statistics logEvent:kStatPointToPoint
            withParameters:@{kStatAction : kStatChangeRoutingMode, kStatValue : kStatPedestrian}];
      break;
    case MWMRouterTypeBicycle:
      [Statistics logEvent:kStatPointToPoint
            withParameters:@{kStatAction : kStatChangeRoutingMode, kStatValue : kStatBicycle}];
      break;
    case MWMRouterTypeTaxi:
      [Statistics logEvent:kStatPointToPoint
            withParameters:@{kStatAction : kStatChangeRoutingMode, kStatValue : kStatTaxi}];
      break;
    }
  }
}

#pragma mark - Properties

- (CGRect)defaultFrame
{
  if (!IPAD)
    return super.defaultFrame;
  CGFloat const width = 320.;
  CGFloat const origin = self.isVisible ? 0. : -width;
  return {{origin, self.topBound}, {width, self.superview.height - kAdditionalHeight}};
}

- (void)setupActualHeight
{
  if (!self.superview)
    return;
  CGFloat const selfHeight = IPAD ? self.superview.height - kAdditionalHeight : 44;
  self.defaultHeight = selfHeight;
  self.height = selfHeight;
}

#pragma mark - MWMNavigationDashboardInfoProtocol

- (void)updateNavigationInfo:(MWMNavigationDashboardEntity *)info
{
  self.resultLabel.attributedText = info.estimate;
  if (!IPAD)
    return;

  NSString * arriveStr = [NSDateFormatter
      localizedStringFromDate:[[NSDate date] dateByAddingTimeInterval:info.timeToTarget]
                    dateStyle:NSDateFormatterNoStyle
                    timeStyle:NSDateFormatterShortStyle];
  self.arriveLabel.text = [NSString stringWithFormat:L(@"routing_arrive"), arriveStr.UTF8String];
}

#pragma mark - VisibleArea

- (MWMAvailableAreaAffectDirections)visibleAreaAffectDirections
{
  return IPAD ? MWMAvailableAreaAffectDirectionsLeft : MWMAvailableAreaAffectDirectionsTop;
}

#pragma mark - PlacePageArea

- (MWMAvailableAreaAffectDirections)placePageAreaAffectDirections
{
  return IPAD ? MWMAvailableAreaAffectDirectionsLeft : MWMAvailableAreaAffectDirectionsNone;
}

@end
