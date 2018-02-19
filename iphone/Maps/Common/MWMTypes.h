typedef void (^MWMVoidBlock)(void);
typedef void (^MWMStringBlock)(NSString *);
typedef void (^MWMURLBlock)(NSURL *);

typedef NS_ENUM(NSUInteger, MWMDayTime) { MWMDayTimeDay, MWMDayTimeNight };

typedef NS_ENUM(NSUInteger, MWMUnits) { MWMUnitsMetric, MWMUnitsImperial };

typedef NS_ENUM(NSUInteger, MWMTheme) {
  MWMThemeDay,
  MWMThemeNight,
  MWMThemeVehicleDay,
  MWMThemeVehicleNight,
  MWMThemeAuto
};

typedef uint32_t MWMMarkID;
typedef uint32_t MWMLineID;
typedef uint32_t MWMMarkGroupID;
typedef NSArray<NSNumber *> * MWMGroupIDCollection;
