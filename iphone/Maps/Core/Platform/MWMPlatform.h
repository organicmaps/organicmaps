typedef NS_ENUM(NSInteger, MWMNetworkConnectionType) {
  MWMNetworkConnectionTypeNone,
  MWMNetworkConnectionTypeWifi,
  MWMNetworkConnectionTypeWwan
};

@interface MWMPlatform : NSObject

+ (MWMNetworkConnectionType)networkConnectionType;

@end
