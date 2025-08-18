#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface DeepLinkSearchData : NSObject

@property(nonatomic, readonly) NSString * query;
@property(nonatomic, readonly) NSString * locale;
@property(nonatomic, readonly) double centerLat;
@property(nonatomic, readonly) double centerLon;
@property(nonatomic, readonly) BOOL isSearchOnMap;

- (instancetype)init;
- (BOOL)hasValidCenterLatLon;
- (void)onViewportChanged:(int)zoomLevel;
@end

NS_ASSUME_NONNULL_END
