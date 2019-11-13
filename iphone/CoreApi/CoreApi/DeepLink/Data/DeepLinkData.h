#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, DeeplinkParsingResult) {
  DeeplinkParsingResultIncorrect = 0,
  DeeplinkParsingResultMap,
  DeeplinkParsingResultRoute,
  DeeplinkParsingResultSearch,
  DeeplinkParsingResultLead,
  DeeplinkParsingResultCatalogue,
  DeeplinkParsingResultCataloguePath,
  DeeplinkParsingResultSubscription
};

@protocol IDeepLinkData <NSObject>

@property (nonatomic, readonly) DeeplinkParsingResult result;

@end

@interface DeepLinkData : NSObject <IDeepLinkData>

@property (nonatomic, readonly) DeeplinkParsingResult result;

- (instancetype)init:(DeeplinkParsingResult)result;

@end

NS_ASSUME_NONNULL_END
