#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, DeeplinkUrlType) {
  DeeplinkUrlTypeIncorrect = 0,
  DeeplinkUrlTypeMap,
  DeeplinkUrlTypeRoute,
  DeeplinkUrlTypeSearch,
  DeeplinkUrlTypeCrosshair,
};

@protocol IDeepLinkData <NSObject>

@property(nonatomic, readonly) DeeplinkUrlType urlType;
@property(nonatomic, readonly) BOOL success;

@end

@interface DeepLinkData : NSObject <IDeepLinkData>

@property(nonatomic, readonly) DeeplinkUrlType urlType;
@property(nonatomic, readonly) BOOL success;

- (instancetype)init:(DeeplinkUrlType)urlType success:(BOOL)success;

@end

NS_ASSUME_NONNULL_END
