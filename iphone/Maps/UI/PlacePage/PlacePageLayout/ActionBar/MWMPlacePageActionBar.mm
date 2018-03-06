#import "MWMPlacePageActionBar.h"
#import "AppInfo.h"
#import "MWMActionBarButton.h"
#import "MWMCircularProgress.h"
#import "MWMNavigationDashboardManager.h"
#import "MWMPlacePageProtocol.h"
#import "MWMRouter.h"
#import "MapViewController.h"

@interface MWMPlacePageActionBar ()<MWMActionBarButtonDelegate>
{
  vector<EButton> m_visibleButtons;
  vector<EButton> m_additionalButtons;
}

@property(nonatomic) NSLayoutConstraint * visibleConstraint;
@property(weak, nonatomic) IBOutlet UIStackView * barButtons;
@property(weak, nonatomic) id<MWMActionBarProtocol> delegate;
@property(weak, nonatomic) id<MWMActionBarSharedData> data;

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

- (void)configureWithData:(id<MWMActionBarSharedData>)data
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

- (void)setFirstButton:(id<MWMActionBarSharedData>)data
{
  vector<EButton> buttons;

  if (self.isAreaNotDownloaded)
  {
    buttons.push_back(EButton::Download);
  }
  else
  {
    BOOL const isRoutePlanning =
        [MWMNavigationDashboardManager manager].state != MWMNavigationDashboardStateHidden;
    if (isRoutePlanning)
      buttons.push_back(EButton::RouteFrom);

    BOOL const isBooking = [data isBooking];
    if (isBooking)
      buttons.push_back(EButton::Booking);
    BOOL const isOpentable = [data isOpentable];
    if (isOpentable)
      buttons.push_back(EButton::Opentable);
    BOOL const isPartner = [data isPartner] && [data sponsoredURL] != nil;
    if (isPartner)
      buttons.push_back(EButton::Partner);
    BOOL const isBookingSearch = [data isBookingSearch];
    if (isBookingSearch)
      buttons.push_back(EButton::BookingSearch);

    BOOL const isPhoneCallAvailable =
        [AppInfo sharedInfo].canMakeCalls && [data phoneNumber].length > 0;
    if (isPhoneCallAvailable)
      buttons.push_back(EButton::Call);

    if (!isRoutePlanning)
      buttons.push_back(EButton::RouteFrom);
  }

  NSAssert(!buttons.empty(), @"Missing first action bar button");
  auto begin = buttons.begin();
  m_visibleButtons.push_back(*begin);
  begin++;
  std::copy(begin, buttons.end(), std::back_inserter(m_additionalButtons));
}

- (void)setSecondButton:(id<MWMActionBarSharedData>)data
{
  vector<EButton> buttons;
  BOOL const isCanAddIntermediatePoint = [MWMRouter canAddIntermediatePoint];
  BOOL const isNavigationReady =
      [MWMNavigationDashboardManager manager].state == MWMNavigationDashboardStateReady;
  if (isCanAddIntermediatePoint && isNavigationReady)
    buttons.push_back(EButton::RouteAddStop);

  buttons.push_back(EButton::Bookmark);

  auto begin = buttons.begin();
  m_visibleButtons.push_back(*begin);
  begin++;
  std::copy(begin, buttons.end(), std::back_inserter(m_additionalButtons));
}

- (void)setThirdButton { m_visibleButtons.push_back(EButton::RouteTo); }

- (void)setFourthButton
{
  if (m_additionalButtons.empty())
  {
    m_visibleButtons.push_back(EButton::Share);
  }
  else
  {
    m_visibleButtons.push_back(EButton::More);
    m_additionalButtons.push_back(EButton::Share);
  }
}

- (void)addButtons2UI:(id<MWMActionBarSharedData>)data
{
  BOOL const isPartner = [data isPartner] && [data sponsoredURL] != nil;
  int const partnerIndex = isPartner ? [data partnerIndex] : -1;

  for (auto const buttonType : m_visibleButtons)
  {
    auto const isSelected = (buttonType == EButton::Bookmark ? [data isBookmark] : NO);
    auto button = [MWMActionBarButton buttonWithDelegate:self
                                              buttonType:buttonType
                                            partnerIndex:partnerIndex
                                              isSelected:isSelected];
    [self.barButtons addArrangedSubview:button];
  }
}

- (void)setSingleButton { m_visibleButtons.push_back(EButton::RouteRemoveStop); }

- (void)setButtons:(id<MWMActionBarSharedData>)data
{
  BOOL const isRoutePoint = [data isRoutePoint];
  if (isRoutePoint)
  {
    [self setSingleButton];
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
      if ([button type] == EButton::Download)
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

- (void)tapOnButtonWithType:(EButton)type
{
  id<MWMActionBarProtocol> delegate = self.delegate;
  auto data = self.data;

  switch (type)
  {
  case EButton::Download: [delegate downloadSelectedArea]; break;
  case EButton::Opentable:
  case EButton::Booking: [delegate book:NO]; break;
  case EButton::BookingSearch: [delegate searchBookingHotels]; break;
  case EButton::Call: [delegate call]; break;
  case EButton::Bookmark:
    if ([data isBookmark])
      [delegate removeBookmark];
    else
      [delegate addBookmark];
    break;
  case EButton::RouteFrom: [delegate routeFrom]; break;
  case EButton::RouteTo: [delegate routeTo]; break;
  case EButton::Share: [delegate share]; break;
  case EButton::More: [self showActionSheet]; break;
  case EButton::RouteAddStop: [delegate routeAddStop]; break;
  case EButton::RouteRemoveStop: [delegate routeRemoveStop]; break;
  case EButton::Partner: [delegate openPartner]; break;
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

  UIViewController * vc = static_cast<UIViewController *>([MapViewController controller]);
  NSMutableArray<NSString *> * titles = [@[] mutableCopy];
  for (auto const buttonType : m_additionalButtons)
  {
    BOOL const isSelected = buttonType == EButton::Bookmark ? [data isBookmark] : NO;
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
