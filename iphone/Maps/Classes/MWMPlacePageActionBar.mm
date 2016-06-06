#import "Common.h"
#import "MapsAppDelegate.h"
#import "MWMActionBarButton.h"
#import "MWMBasePlacePageView.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManager.h"

#include "Framework.h"

#include "std/vector.hpp"

extern NSString * const kAlohalyticsTapEventKey;

namespace
{
NSString * const kPlacePageActionBarNibName = @"PlacePageActionBar";

}  // namespace

@interface MWMPlacePageActionBar () <MWMActionBarButtonDelegate, UIActionSheetDelegate>
{
  vector<EButton> m_visibleButtons;
  vector<EButton> m_additionalButtons;
}

@property (weak, nonatomic) MWMPlacePageViewManager * placePageManager;
@property (copy, nonatomic) IBOutletCollection(UIView) NSArray<UIView *> * buttons;
@property (nonatomic) BOOL isPrepareRouteMode;

@end

@implementation MWMPlacePageActionBar

+ (MWMPlacePageActionBar *)actionBarForPlacePageManager:(MWMPlacePageViewManager *)placePageManager
{
  MWMPlacePageActionBar * bar = [[NSBundle.mainBundle
                                 loadNibNamed:kPlacePageActionBarNibName owner:nil options:nil] firstObject];
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
  MWMPlacePageEntity * entity = self.placePageManager.entity;
  NSString * phone = [entity getCellValue:MWMPlacePageCellTypePhoneNumber];

  BOOL const isIphone = [[UIDevice currentDevice].model isEqualToString:@"iPhone"];
  BOOL const isPhoneNotEmpty = phone.length > 0;
  BOOL const isBooking = entity.isBooking;
  BOOL const itHasPhoneNumber = isIphone && isPhoneNotEmpty;
  BOOL const isApi = entity.isApi;
  BOOL const isP2P = self.isPrepareRouteMode;
  BOOL const isMyPosition = entity.isMyPosition;

  if (isMyPosition)
  {
    m_visibleButtons.push_back(EButton::Spacer);
    m_visibleButtons.push_back(EButton::Bookmark);
    m_visibleButtons.push_back(EButton::Share);
    m_visibleButtons.push_back(EButton::Spacer);
  }
  else if (isApi && isBooking)
  {
    m_visibleButtons.push_back(EButton::Api);
    m_visibleButtons.push_back(EButton::Booking);
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
  else if (isBooking && isP2P)
  {
    m_visibleButtons.push_back(EButton::Bookmark);
    m_visibleButtons.push_back(EButton::RouteFrom);
    m_additionalButtons.push_back(EButton::Booking);
    m_additionalButtons.push_back(EButton::Share);
  }
  else if (itHasPhoneNumber && isP2P)
  {
    m_visibleButtons.push_back(EButton::Bookmark);
    m_visibleButtons.push_back(EButton::RouteFrom);
    m_additionalButtons.push_back(EButton::Call);
    m_additionalButtons.push_back(EButton::Share);
  }
  else if (isBooking)
  {
    m_visibleButtons.push_back(EButton::Booking);
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
    if (v.tag == size + 1)
      last = v;
  }
  return last;
}

#pragma mark - MWMActionBarButtonDelegate

- (void)tapOnButtonWithType:(EButton)type
{
  switch (type)
  {
  case EButton::Api:
    [self.placePageManager apiBack];
    break;
  case EButton::Booking:
    [self.placePageManager book:NO];
    break;
  case EButton::Call:
    [self.placePageManager call];
    break;
  case EButton::Bookmark:
    if (self.isBookmark)
      [self.placePageManager removeBookmark];
    else
      [self.placePageManager addBookmark];
    break;
  case EButton::RouteFrom:
    [self.placePageManager routeFrom];
    break;
  case EButton::RouteTo:
    if (self.isPrepareRouteMode)
      [self.placePageManager routeTo];
    else
      [self.placePageManager buildRoute];
    break;
  case EButton::Share:
    [self.placePageManager share];
    break;
  case EButton::More:
    [self showActionSheet];
    break;
  case EButton::Spacer:
    break;
  }
}

#pragma mark - ActionSheet

- (void)showActionSheet
{
  NSString * cancel = L(@"cancel");
  MWMPlacePageEntity * entity = self.placePageManager.entity;
  BOOL const isTitleNotEmpty = entity.title.length > 0;
  NSString * title = isTitleNotEmpty ? entity.title : entity.subtitle;
  NSString * subtitle = isTitleNotEmpty ? entity.subtitle : nil;
  
  UIViewController * vc = static_cast<UIViewController *>(MapsAppDelegate.theApp.mapViewController);
  NSMutableArray<NSString *> * titles = [@[] mutableCopy];
  for (auto const buttonType : m_additionalButtons)
  {
    BOOL const isSelected = buttonType == EButton::Bookmark ? self.isBookmark : NO;
    if (NSString * title = titleForButton(buttonType, isSelected))
      [titles addObject:title];
    else
      NSAssert(false, @"Title can't be nil!");
  }

  if (isIOS7)
  {
    UIActionSheet * actionSheet = [[UIActionSheet alloc] initWithTitle:title delegate:self cancelButtonTitle:cancel destructiveButtonTitle:nil otherButtonTitles:nil];

    for (NSString * title in titles)
      [actionSheet addButtonWithTitle:title];

    [actionSheet showInView:vc.view];
  }
  else
  {
    UIAlertController * alertController = [UIAlertController alertControllerWithTitle:title message:subtitle preferredStyle:UIAlertControllerStyleActionSheet];
    UIAlertAction * cancelAction = [UIAlertAction actionWithTitle:cancel style:UIAlertActionStyleCancel handler:nil];

    for (auto i = 0; i < titles.count; i++)
    {
      UIAlertAction * commonAction = [UIAlertAction actionWithTitle:titles[i] style:UIAlertActionStyleDefault handler:^(UIAlertAction * action)
                                      {
                                        [self tapOnButtonWithType:self->m_additionalButtons[i]];
                                      }];
      [alertController addAction:commonAction];
    }
    [alertController addAction:cancelAction];

    if (IPAD)
    {
      UIPopoverPresentationController * popPresenter = [alertController popoverPresentationController];
      popPresenter.sourceView = self.shareAnchor;
    }
    [vc presentViewController:alertController animated:YES completion:nil];
  }
}

#pragma mark - UIActionSheetDelegate

- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex
{
  [actionSheet dismissWithClickedButtonIndex:buttonIndex animated:YES];

  // Using buttonIndex - 1 because there is cancel button at index 0
  // Only iOS7
  if (buttonIndex > 0)
    [self tapOnButtonWithType:m_additionalButtons[buttonIndex - 1]];
}

@end
