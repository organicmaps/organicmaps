#import "MWMEye.h"

#include "Framework.h"

@implementation MWMEye

+ (MWMTip)getTipType
{
  auto tutorialType = GetFramework().GetTipsApi().GetTip();
  return tutorialType ? (MWMTip)*tutorialType : MWMTipNone;
}

+ (void)tipClickedWithType:(MWMTip)type event:(MWMTipEvent)event
{
  eye::Eye::Event::TipClicked((eye::Tip::Type)type, (eye::Tip::Event)event);
}

+ (void)bookingFilterUsed
{
  eye::Eye::Event::BookingFilterUsed();
}

+ (void)boomarksCatalogShown
{
  eye::Eye::Event::BoomarksCatalogShown();
}

+ (void)discoveryShown
{
  eye::Eye::Event::DiscoveryShown();
}

+ (void)discoveryItemClickedWithEvent:(MWMEyeDiscoveryEvent)event
{
  eye::Eye::Event::DiscoveryItemClicked((eye::Discovery::Event)event);
}

+ (void)transitionToBookingWithPos:(CGPoint)pos
{
   eye::Eye::Event::TransitionToBooking({pos.x, pos.y});
}

+ (void)promoAfterBookingShownWithCityId:(NSString *)cityId
{
  eye::Eye::Event::PromoAfterBookingShown(cityId.UTF8String);
}

@end
