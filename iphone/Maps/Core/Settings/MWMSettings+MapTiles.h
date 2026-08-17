#import "MWMSettings.h"

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(MapTilesSettings)
@protocol MWMMapTilesSettings

+ (BOOL)backgroundTilesEnabled;
+ (NSString *)backgroundTilesURL;
+ (NSInteger)backgroundTilesCacheSizeMB;
+ (NSInteger)backgroundTilesAreaOpacityPct;
+ (NSInteger)backgroundTilesMinCacheSizeMB;
+ (NSInteger)backgroundTilesMaxCacheSizeMB;
+ (NSInteger)backgroundTilesMinAreaOpacityPct;
+ (NSInteger)backgroundTilesMaxAreaOpacityPct;
+ (void)setBackgroundTilesEnabled:(BOOL)enabled;
+ (void)setBackgroundTilesEnabled:(BOOL)enabled
                              url:(NSString *)url
                      cacheSizeMB:(NSInteger)cacheSizeMB
                   areaOpacityPct:(NSInteger)areaOpacityPct
    NS_SWIFT_NAME(setBackgroundTiles(enabled:url:cacheSizeMB:areaOpacityPct:));
+ (BOOL)isWellFormedBackgroundTilesURL:(NSString *)url;

@end

@interface MWMSettings (MapTiles) <MWMMapTilesSettings>
@end

NS_ASSUME_NONNULL_END
