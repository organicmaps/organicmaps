typedef void (^TMWMVoidBlock)();
typedef void (^MWMStringBlock)(NSString *);


typedef NS_ENUM(NSUInteger, MWMUnits) { MWMUnitsMetric, MWMUnitsImperial };

typedef NS_ENUM(NSUInteger, MWMRouterType) {
  MWMRouterTypeVehicle,
  MWMRouterTypePedestrian,
  MWMRouterTypeBicycle,
  MWMRouterTypeTaxi
};
