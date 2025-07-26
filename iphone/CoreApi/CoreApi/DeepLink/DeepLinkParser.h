#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, DeeplinkUrlType) {
  DeeplinkUrlTypeIncorrect = 0,
  DeeplinkUrlTypeMap,
  DeeplinkUrlTypeRoute,
  DeeplinkUrlTypeSearch,
  DeeplinkUrlTypeCrosshair,
  DeeplinkUrlTypeOAuth2,
  DeeplinkUrlTypeMenu,
  DeeplinkUrlTypeSettings
};

@interface DeepLinkParser : NSObject

+ (DeeplinkUrlType)parseAndSetApiURL:(NSURL *)url;
+ (void)executeMapApiRequest;
+ (void)addBookmarksFile:(NSURL *)url;
@end

NS_ASSUME_NONNULL_END
