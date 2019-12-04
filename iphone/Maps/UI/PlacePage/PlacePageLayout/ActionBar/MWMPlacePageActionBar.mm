#import "MWMPlacePageActionBar.h"
#import <CoreApi/AppInfo.h>
#import "MWMActionBarButton.h"
#import "MWMCircularProgress.h"
#import "MWMNavigationDashboardManager.h"
#import "MWMPlacePageProtocol.h"
#import "MWMRouter.h"
#import "MapViewController.h"
#import "MWMPlacePageData.h"

@interface MWMPlacePageActionBar ()<MWMActionBarButtonDelegate>
{
  std::vector<MWMActionBarButtonType> m_visibleButtons;
  std::vector<MWMActionBarButtonType> m_additionalButtons;
}

@property(nonatomic) NSLayoutConstraint * visibleConstraint;
@property(weak, nonatomic) IBOutlet UIStackView * barButtons;
@property(weak, nonatomic) id<MWMActionBarProtocol> delegate;
@property(weak, nonatomic) MWMPlacePageData * data;

@end

@implementation MWMPlacePageActionBar

+ (MWMPlacePageActionBar *)actionBarWithDelegate:(id<MWMActionBarProtocol>)delegate
{
  MWMPlacePageActionBar * bar =
      [NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil].firstObject;
  bar.delegate = delegate;
  bar.translatesAutoresizingMaskIntoConstraints = NO;
  return bar;
}

- (void)configureWithData:(MWMPlacePageData *)data
{
  self.data = data;
  [self configureButtons];
  self.autoresizingMask = UIViewAutoresizingNone;
  [self setNeedsLayout];
}

- (void)clearButtons
{
  m_visibleButtons.clear();
  m_additionalButtons.clear();
  [self.barButtons.subviews makeObjectsPerformSelector:@selector(removeFromSuperview)];
}

- (void)setFirstButton:(MWMPlacePageData *)data
{
  std::vector<MWMActionBarButtonType> buttons;

  if (self.isAreaNotDownloaded)
  {
    buttons.push_back(MWMActionBarButtonTypeDownload);
  }
  else
  {
    BOOL const isRoutePlanning =
        [MWMNavigationDashboardManager manager].state != MWMNavigationDashboardStateHidden;
    if (isRoutePlanning)
      buttons.push_back(MWMActionBarButtonTypeRouteFrom);

    BOOL const isBooking = [data isBooking];
    if (isBooking)
      buttons.push_back(MWMActionBarButtonTypeBooking);
    BOOL const isOpentable = [data isOpentable];
    if (isOpentable)
      buttons.push_back(MWMActionBarButtonTypeOpentable);
    BOOL const isPartner = [data isPartner] && [data sponsoredURL] != nil;
    if (isPartner)
      buttons.push_back(MWMActionBarButtonTypePartner);
    BOOL const isBookingSearch = [data isBookingSearch];
    if (isBookingSearch)
      buttons.push_back(MWMActionBarButtonTypeBookingSearch);

    BOOL const isPhoneCallAvailable =
        [AppInfo sharedInfo].canMakeCalls && [data phoneNumber].length > 0;
    if (isPhoneCallAvailable)
      buttons.push_back(MWMActionBarButtonTypeCall);

    if (!isRoutePlanning)
      buttons.push_back(MWMActionBarButtonTypeRouteFrom);
  }

  NSAssert(!buttons.empty(), @"Missing first action bar button");
  auto begin = buttons.begin();
  m_visibleButtons.push_back(*begin);
  begin++;
  std::copy(begin, buttons.end(), std::back_inserter(m_additionalButtons));
}

- (void)setSecondButton:(MWMPlacePageData *)data
{
  std::vector<MWMActionBarButtonType> buttons;
  BOOL const isCanAddIntermediatePoint = [MWMRouter canAddIntermediatePoint];
  BOOL const isNavigationReady =
      [MWMNavigationDashboardManager manager].state == MWMNavigationDashboardStateReady;
  if (isCanAddIntermediatePoint && isNavigationReady)
    buttons.push_back(MWMActionBarButtonTypeRouteAddStop);

  buttons.push_back(MWMActionBarButtonTypeBookmark);

  auto begin = buttons.begin();
  m_visibleButtons.push_back(*begin);
  begin++;
  std::copy(begin, buttons.end(), std::back_inserter(m_additionalButtons));
}

- (void)setThirdButton { m_visibleButtons.push_back(MWMActionBarButtonTypeRouteTo); }

- (void)setFourthButton
{
  if (m_additionalButtons.empty())
  {
    m_visibleButtons.push_back(MWMActionBarButtonTypeShare);
  }
  else
  {
    m_visibleButtons.push_back(MWMActionBarButtonTypeMore);
    m_additionalButtons.push_back(MWMActionBarButtonTypeShare);
  }
}

- (void)addButtons2UI:(MWMPlacePageData *)data
{
  BOOL const isPartner = [data isPartner] && [data sponsoredURL] != nil;
  int const partnerIndex = isPartner ? [data partnerIndex] : -1;

  for (auto const buttonType : m_visibleButtons)
  {
    auto const isSelected = (buttonType == MWMActionBarButtonTypeBookmark ? [data isBookmark] : NO);
    auto const isDisabled = (buttonType == MWMActionBarButtonTypeBookmark && [data isBookmark] && !data.isBookmarkEditable);
    auto button = [MWMActionBarButton buttonWithDelegate:self
                                              buttonType:buttonType
                                            partnerIndex:partnerIndex
                                              isSelected:isSelected
                                              isDisabled:isDisabled];
    [self.barButtons addArrangedSubview:button];
  }
}

- (void)setSingleButton { m_visibleButtons.push_back(MWMActionBarButtonTypeRouteRemoveStop); }

- (void)setButtons:(MWMPlacePageData *)data
{
  RoadWarningMarkType roadType = [data roadType];
  if ([data isRoutePoint])
  {
    [self setSingleButton];
  } else if (roadType != RoadWarningMarkType::Count) {
    switch (roadType) {
      case RoadWarningMarkType::Toll:
        m_visibleButtons.push_back(MWMActionBarButtonTypeAvoidToll);
        break;
      case RoadWarningMarkType::Ferry:
        m_visibleButtons.push_back(MWMActionBarButtonTypeAvoidFerry);
        break;
      case RoadWarningMarkType::Dirty:
        m_visibleButtons.push_back(MWMActionBarButtonTypeAvoidDirty);
        break;
      default:
        break;
    }
  }
  else
  {
    [self setFirstButton:data];
    [self setSecondButton:data];
    [self setThirdButton];
    [self setFourthButton];
  }
}

- (void)configureButtons
{
  auto data = self.data;
  [self clearButtons];

  [self setButtons:data];
  [self addButtons2UI:data];
}

- (void)setDownloadingState:(MWMCircularProgressState)state
{
  self.progressFromActiveButton.state = state;
}

- (void)setDownloadingProgress:(CGFloat)progress
{
  self.progressFromActiveButton.progress = progress;
}

- (MWMCircularProgress *)progressFromActiveButton
{
  if (self.isAreaNotDownloaded)
  {
    for (MWMActionBarButton * button in self.barButtons.subviews)
    {
      if ([button type] == MWMActionBarButtonTypeDownload)
        return button.mapDownloadProgress;
    }
  }
  return nil;
}

- (void)setIsAreaNotDownloaded:(BOOL)isAreaNotDownloaded
{
  if (_isAreaNotDownloaded == isAreaNotDownloaded)
    return;

  _isAreaNotDownloaded = isAreaNotDownloaded;
  [self configureButtons];
}

- (UIView *)shareAnchor { return self.barButtons.subviews.lastObject; }

#pragma mark - MWMActionBarButtonDelegate

- (void)tapOnButtonWithType:(MWMActionBarButtonType)type
{
  id<MWMActionBarProtocol> delegate = self.delegate;
  auto data = self.data;

  switch (type)
  {
  case MWMActionBarButtonTypeDownload: [delegate downloadSelectedArea]; break;
  case MWMActionBarButtonTypeOpentable:
  case MWMActionBarButtonTypeBooking: [delegate book]; break;
  case MWMActionBarButtonTypeBookingSearch: [delegate searchBookingHotels]; break;
  case MWMActionBarButtonTypeCall: [delegate call]; break;
  case MWMActionBarButtonTypeBookmark:
    if ([data isBookmark])
      [delegate removeBookmark];
    else
      [delegate addBookmark];
    break;
  case MWMActionBarButtonTypeRouteFrom: [delegate routeFrom]; break;
  case MWMActionBarButtonTypeRouteTo: [delegate routeTo]; break;
  case MWMActionBarButtonTypeShare: [delegate share]; break;
  case MWMActionBarButtonTypeMore: [self showActionSheet]; break;
  case MWMActionBarButtonTypeRouteAddStop: [delegate routeAddStop]; break;
  case MWMActionBarButtonTypeRouteRemoveStop: [delegate routeRemoveStop]; break;
  case MWMActionBarButtonTypeAvoidToll: [delegate avoidToll]; break;
  case MWMActionBarButtonTypeAvoidDirty: [delegate avoidDirty]; break;
  case MWMActionBarButtonTypeAvoidFerry: [delegate avoidFerry]; break;
  case MWMActionBarButtonTypePartner: [delegate openPartner]; break;
  }
}

#pragma mark - ActionSheet

- (void)showActionSheet
{
  NSString * cancel = L(@"cancel");
  auto data = self.data;
  BOOL const isTitleNotEmpty = [data title].length > 0;
  NSString * title = isTitleNotEmpty ? [data title] : [data subtitle];
  NSString * subtitle = isTitleNotEmpty ? [data subtitle] : nil;

  UIViewController * vc = static_cast<UIViewController *>([MapViewController sharedController]);
  NSMutableArray<NSString *> * titles = [@[] mutableCopy];
  for (auto const buttonType : m_additionalButtons)
  {
    BOOL const isSelected = buttonType == MWMActionBarButtonTypeBookmark ? [data isBookmark] : NO;
    if (NSString * title = titleForButton(buttonType, [data partnerIndex], isSelected))
      [titles addObject:title];
    else
      NSAssert(false, @"Title can't be nil!");
  }

  UIAlertController * alertController =
      [UIAlertController alertControllerWithTitle:title
                                          message:subtitle
                                   preferredStyle:UIAlertControllerStyleActionSheet];
  alertController.popoverPresentationController.sourceView = self.shareAnchor;
  alertController.popoverPresentationController.sourceRect = self.shareAnchor.bounds;

  UIAlertAction * cancelAction =
      [UIAlertAction actionWithTitle:cancel style:UIAlertActionStyleCancel handler:nil];

  for (auto i = 0; i < titles.count; i++)
  {
    UIAlertAction * commonAction =
        [UIAlertAction actionWithTitle:titles[i]
                                 style:UIAlertActionStyleDefault
                               handler:^(UIAlertAction * action) {
                                 [self tapOnButtonWithType:self->m_additionalButtons[i]];
                               }];
    [alertController addAction:commonAction];
  }
  [alertController addAction:cancelAction];

  [vc presentViewController:alertController animated:YES completion:nil];
}

- (void)setVisible:(BOOL)visible
{
  self.visibleConstraint.active = NO;
  NSLayoutYAxisAnchor * bottomAnchor = self.superview.bottomAnchor;
  if (@available(iOS 11.0, *))
    bottomAnchor = self.superview.safeAreaLayoutGuide.bottomAnchor;
  self.alpha = visible ? 1 : 0;
  self.visibleConstraint =
      [bottomAnchor constraintEqualToAnchor:visible ? self.bottomAnchor : self.topAnchor];
  self.visibleConstraint.active = YES;
}

@end
