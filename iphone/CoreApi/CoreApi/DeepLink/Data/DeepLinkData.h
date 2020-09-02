#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, DeeplinkUrlType) {
  DeeplinkUrlTypeIncorrect = 0,
  DeeplinkUrlTypeMap,
  DeeplinkUrlTypeRoute,
  DeeplinkUrlTypeSearch,
  DeeplinkUrlTypeLead,
  DeeplinkUrlTypeCatalogue,
  DeeplinkUrlTypeCataloguePath,
  DeeplinkUrlTypeSubscription
};

@protocol IDeepLinkData <NSObject>

@property (nonatomic, readonly) DeeplinkUrlType urlType;

@end

@interface DeepLinkData : NSObject <IDeepLinkData>

@property (nonatomic, readonly) DeeplinkUrlType urlType;

- (instancetype)init:(DeeplinkUrlType)urlType;

@end

NS_ASSUME_NONNULL_END
