#import <Foundation/Foundation.h>
#import "DeepLinkData.h"

NS_ASSUME_NONNULL_BEGIN

@interface DeepLinkSearchData : NSObject <IDeepLinkData>

@property(nonatomic, readonly) DeeplinkParsingResult result;
@property(nonatomic, readonly) NSString* query;
@property(nonatomic, readonly) NSString* locale;
@property(nonatomic, readonly) double centerLat;
@property(nonatomic, readonly) double centerLon;
@property(nonatomic, readonly) BOOL isSearchOnMap;

- (instancetype)init:(DeeplinkParsingResult)result;
- (void)onViewportChanged:(int)zoomLevel;
@end

NS_ASSUME_NONNULL_END
