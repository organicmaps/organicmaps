typedef NS_ENUM(NSUInteger, MWMTip)
{
  MWMTipBookmarks,
  MWMTipSearch,
  MWMTipDiscovery,
  MWMTipSubway,
  MWMTipNone
};

typedef NS_ENUM(NSUInteger, MWMTipEvent)
{
  MWMTipEventAction,
  MWMTipEventGotIt
};

typedef NS_ENUM(NSUInteger, MWMEyeDiscoveryEvent)
{
  MWMEyeDiscoveryEventHotels,
  MWMEyeDiscoveryEventAttractions,
  MWMEyeDiscoveryEventCafes,
  MWMEyeDiscoveryEventLocals,
  MWMEyeDiscoveryEventMoreHotels,
  MWMEyeDiscoveryEventMoreAttractions,
  MWMEyeDiscoveryEventMoreCafes,
  MWMEyeDiscoveryEventMoreLocals
};

@interface MWMEye : NSObject

+ (MWMTip)getTipType;
+ (void)tipShownWithType:(MWMTip)type event:(MWMTipEvent)event;
+ (void)bookingFilterUsed;
+ (void)boomarksCatalogShown;
+ (void)discoveryShown;
+ (void)discoveryItemClickedWithEvent:(MWMEyeDiscoveryEvent)event;

@end
