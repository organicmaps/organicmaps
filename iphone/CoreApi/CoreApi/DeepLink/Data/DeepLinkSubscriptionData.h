#import <Foundation/Foundation.h>
#import "DeepLinkData.h"

NS_ASSUME_NONNULL_BEGIN

@interface DeepLinkSubscriptionData : NSObject <IDeepLinkData>

@property(nonatomic, readonly) DeeplinkUrlType result;
@property(nonatomic, readonly) BOOL success;
@property(nonatomic, readonly) NSString *groups;

- (instancetype)init:(DeeplinkUrlType)result success:(BOOL)success;

@end

NS_ASSUME_NONNULL_END
