#import <Foundation/Foundation.h>
#import "DeepLinkData.h"

NS_ASSUME_NONNULL_BEGIN

@interface DeepLinkSubscriptionData : NSObject <IDeepLinkData>

@property (nonatomic, readonly) DeeplinkParsingResult result;
@property(nonatomic, readonly) NSString* deliverable;

- (instancetype)init:(DeeplinkParsingResult)result;

@end

NS_ASSUME_NONNULL_END
