typedef void (^MWMVoidBlock)();
typedef void (^MWMStringBlock)(NSString *);

typedef NS_ENUM(NSUInteger, MWMDayTime) { MWMDayTimeDay, MWMDayTimeNight };

typedef NS_ENUM(NSUInteger, MWMUnits) { MWMUnitsMetric, MWMUnitsImperial };

typedef NS_ENUM(NSUInteger, MWMRouterType) {
  MWMRouterTypeVehicle,
  MWMRouterTypePedestrian,
  MWMRouterTypeBicycle,
  MWMRouterTypeTaxi
};

typedef NS_ENUM(NSUInteger, MWMTheme) { MWMThemeDay, MWMThemeNight, MWMThemeAuto };
