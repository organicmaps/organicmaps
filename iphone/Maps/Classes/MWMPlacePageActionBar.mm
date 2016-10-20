#import "MWMPlacePageActionBar.h"
#import "Common.h"
#import "MWMActionBarButton.h"
#import "MWMBasePlacePageView.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageProtocol.h"
#import "MWMPlacePageViewManager.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"

#include "Framework.h"

#include "std/vector.hpp"

extern NSString * const kAlohalyticsTapEventKey;

@interface MWMPlacePageActionBar ()<MWMActionBarButtonDelegate>
{
  vector<EButton> m_visibleButtons;
  vector<EButton> m_additionalButtons;
}

@property(weak, nonatomic) MWMPlacePageViewManager * placePageManager;
@property(copy, nonatomic) IBOutletCollection(UIView) NSArray<UIView *> * buttons;
@property(weak, nonatomic) IBOutlet UIImageView * separator;
@property(nonatomic) BOOL isPrepareRouteMode;

@property(weak, nonatomic) id<MWMActionBarSharedData> data;
@property(weak, nonatomic) id<MWMActionBarProtocol> delegate;

@end

@implementation MWMPlacePageActionBar

+ (MWMPlacePageActionBar *)actionBarWithDelegate:(id<MWMActionBarProtocol>)delegate
{
  MWMPlacePageActionBar * bar =
      [[NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil] firstObject];
  bar.delegate = delegate;
  return bar;
}

- (void)configureWithData:(id<MWMActionBarSharedData>)data
{
  self.data = data;
  self.isPrepareRouteMode = MapsAppDelegate.theApp.routingPlaneMode != MWMRoutingPlaneModeNone;
  self.isBookmark = data.isBookmark;
  [self configureButtons];
  self.autoresizingMask = UIViewAutoresizingNone;
}

+ (MWMPlacePageActionBar *)actionBarForPlacePageManager:(MWMPlacePageViewManager *)placePageManager
{
  MWMPlacePageActionBar * bar =
      [[NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil] firstObject];
  [bar configureWithPlacePageManager:placePageManager];
  return bar;
}

- (void)configureWithPlacePageManager:(MWMPlacePageViewManager *)placePageManager
{
  self.placePageManager = placePageManager;
  self.isPrepareRouteMode = MapsAppDelegate.theApp.routingPlaneMode != MWMRoutingPlaneModeNone;
  self.isBookmark = placePageManager.entity.isBookmark;
  [self configureButtons];
  self.autoresizingMask = UIViewAutoresizingNone;
}

- (void)configureButtons
{
  m_visibleButtons.clear();
  m_additionalButtons.clear();
  auto data =
      static_cast<id<MWMActionBarSharedData>>(IPAD ? self.placePageManager.entity : self.data);
  NSString * phone = data.phoneNumber;

  BOOL const isIphone = [[UIDevice currentDevice].model isEqualToString:@"iPhone"];
  BOOL const isPhoneNotEmpty = phone.length > 0;
  BOOL const isBooking = data.isBooking;
  BOOL const isOpentable = data.isOpentable;
  BOOL const isSponsored = isBooking || isOpentable;
  BOOL const itHasPhoneNumber = isIphone && isPhoneNotEmpty;
  BOOL const isApi = data.isApi;
  BOOL const isP2P = self.isPrepareRouteMode;
  BOOL const isMyPosition = data.isMyPosition;

  EButton const sponsoredButton = isBooking ? EButton::Booking : EButton::Opentable;

  if (isMyPosition)
  {
    m_visibleButtons.push_back(EButton::Spacer);
    m_visibleButtons.push_back(EButton::Bookmark);
    m_visibleButtons.push_back(EButton::Share);
    m_visibleButtons.push_back(EButton::Spacer);
  }
  else if (isApi && isSponsored)
  {
    m_visibleButtons.push_back(EButton::Api);
    m_visibleButtons.push_back(sponsoredButton);
    m_additionalButtons.push_back(EButton::Bookmark);
    m_additionalButtons.push_back(EButton::RouteFrom);
    m_additionalButtons.push_back(EButton::Share);
  }
  else if (isApi && itHasPhoneNumber)
  {
    m_visibleButtons.push_back(EButton::Api);
    m_visibleButtons.push_back(EButton::Call);
    m_additionalButtons.push_back(EButton::Bookmark);
    m_additionalButtons.push_back(EButton::RouteFrom);
    m_additionalButtons.push_back(EButton::Share);
  }
  else if (isApi && isP2P)
  {
    m_visibleButtons.push_back(EButton::Api);
    m_visibleButtons.push_back(EButton::RouteFrom);
    m_additionalButtons.push_back(EButton::Bookmark);
    m_additionalButtons.push_back(EButton::Share);
  }
  else if (isApi)
  {
    m_visibleButtons.push_back(EButton::Api);
    m_visibleButtons.push_back(EButton::Bookmark);
    m_additionalButtons.push_back(EButton::RouteFrom);
    m_additionalButtons.push_back(EButton::Share);
  }
  else if (isSponsored && isP2P)
  {
    m_visibleButtons.push_back(EButton::Bookmark);
    m_visibleButtons.push_back(EButton::RouteFrom);
    m_additionalButtons.push_back(sponsoredButton);
    m_additionalButtons.push_back(EButton::Share);
  }
  else if (itHasPhoneNumber && isP2P)
  {
    m_visibleButtons.push_back(EButton::Bookmark);
    m_visibleButtons.push_back(EButton::RouteFrom);
    m_additionalButtons.push_back(EButton::Call);
    m_additionalButtons.push_back(EButton::Share);
  }
  else if (isSponsored)
  {
    m_visibleButtons.push_back(sponsoredButton);
    m_visibleButtons.push_back(EButton::Bookmark);
    m_additionalButtons.push_back(EButton::RouteFrom);
    m_additionalButtons.push_back(EButton::Share);
  }
  else if (itHasPhoneNumber)
  {
    m_visibleButtons.push_back(EButton::Call);
    m_visibleButtons.push_back(EButton::Bookmark);
    m_additionalButtons.push_back(EButton::RouteFrom);
    m_additionalButtons.push_back(EButton::Share);
  }
  else
  {
    m_visibleButtons.push_back(EButton::Bookmark);
    m_visibleButtons.push_back(EButton::RouteFrom);
  }

  if (!isMyPosition)
  {
    m_visibleButtons.push_back(EButton::RouteTo);
    m_visibleButtons.push_back(m_additionalButtons.empty() ? EButton::Share : EButton::More);
  }

  for (UIView * v in self.buttons)
  {
    [v.subviews makeObjectsPerformSelector:@selector(removeFromSuperview)];
    auto const type = m_visibleButtons[v.tag - 1];
    [MWMActionBarButton addButtonToSuperview:v
                                    delegate:self
                                  buttonType:type
                                  isSelected:type == EButton::Bookmark ? self.isBookmark : NO];
  }
}

- (UIView *)shareAnchor
{
  UIView * last = nil;
  auto const size = self.buttons.count;
  for (UIView * v in self.buttons)
  {
    if (v.tag == size)
      last = v;
  }
  return last;
}

#pragma mark - MWMActionBarButtonDelegate

- (void)tapOnButtonWithType:(EButton)type
{
  id<MWMActionBarProtocol> delegate = IPAD ? self.placePageManager : self.delegate;

  switch (type)
  {
  case EButton::Api: [delegate apiBack]; break;
  case EButton::Opentable:
  case EButton::Booking: [delegate book:NO]; break;
  case EButton::Call: [delegate call]; break;
  case EButton::Bookmark:
    if (self.isBookmark)
      [delegate removeBookmark];
    else
      [delegate addBookmark];

    self.isBookmark = !self.isBookmark;
    break;
  case EButton::RouteFrom: [delegate routeFrom]; break;
  case EButton::RouteTo: [delegate routeTo]; break;
  case EButton::Share: [delegate share]; break;
  case EButton::More: [self showActionSheet]; break;
  case EButton::Spacer: break;
  }
}

#pragma mark - ActionSheet

- (void)showActionSheet
{
  NSString * cancel = L(@"cancel");
  auto data =
      static_cast<id<MWMActionBarSharedData>>(IPAD ? self.placePageManager.entity : self.data);
  BOOL const isTitleNotEmpty = data.title.length > 0;
  NSString * title = isTitleNotEmpty ? data.title : data.subtitle;
  NSString * subtitle = isTitleNotEmpty ? data.subtitle : nil;

  UIViewController * vc = static_cast<UIViewController *>([MapViewController controller]);
  NSMutableArray<NSString *> * titles = [@[] mutableCopy];
  for (auto const buttonType : m_additionalButtons)
  {
    BOOL const isSelected = buttonType == EButton::Bookmark ? data.isBookmark : NO;
    if (NSString * title = titleForButton(buttonType, isSelected))
      [titles addObject:title];
    else
      NSAssert(false, @"Title can't be nil!");
  }

  UIAlertController * alertController =
      [UIAlertController alertControllerWithTitle:title
                                          message:subtitle
                                   preferredStyle:UIAlertControllerStyleActionSheet];
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

  if (IPAD)
  {
    UIPopoverPresentationController * popPresenter =
        [alertController popoverPresentationController];
    popPresenter.sourceView = self.shareAnchor;
    popPresenter.sourceRect = self.shareAnchor.bounds;
  }
  [vc presentViewController:alertController animated:YES completion:nil];
}

#pragma mark - Layout

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.width = self.superview.width;
  if (IPAD)
    self.origin = {0, self.superview.height - self.height};

  self.separator.width = self.width;
  CGFloat const buttonWidth = self.width / self.buttons.count;
  for (UIView * button in self.buttons)
  {
    button.minX = buttonWidth * (button.tag - 1);
    button.width = buttonWidth;
  }
}

#pragma mark - Properties

- (void)setIsBookmark:(BOOL)isBookmark
{
  _isBookmark = isBookmark;
  [self configureButtons];
}

@end
