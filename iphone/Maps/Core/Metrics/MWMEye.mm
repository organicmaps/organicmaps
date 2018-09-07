#import "MWMEye.h"

#include "Framework.h"
#include "metrics/eye.hpp"

@implementation MWMEye

+ (MWMTip)getTipType
{
  auto tutorialType = GetFramework().GetTipsApi().GetTip();
  return tutorialType ? (MWMTip)*tutorialType : MWMTipNone;
}

+ (void)tipShownWithType:(MWMTip)type event:(MWMTipEvent)event
{
  eye::Eye::Event::TipShown((eye::Tip::Type)type, (eye::Tip::Event)event);
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

@end
