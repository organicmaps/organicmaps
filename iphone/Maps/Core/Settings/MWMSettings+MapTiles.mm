#import "MWMSettings+MapTiles.h"

#include <CoreApi/Framework.h>

@implementation MWMSettings (MapTiles)

+ (BOOL)backgroundTilesEnabled
{
  return GetFramework().IsBackgroundTilesEnabled();
}

+ (void)setBackgroundTilesEnabled:(BOOL)enabled
{
  GetFramework().SetBackgroundTilesEnabled(enabled);
}

+ (NSInteger)backgroundTilesAreaOpacityPct
{
  return static_cast<NSInteger>(GetFramework().GetBackgroundTilesAreaOpacity());
}

+ (NSString *)backgroundTilesURL
{
  return @(Framework::GetBackgroundTilesURL().c_str());
}

+ (NSInteger)backgroundTilesCacheSizeMB
{
  return static_cast<NSInteger>(Framework::GetBackgroundTilesCacheSize());
}

+ (NSInteger)backgroundTilesMinCacheSizeMB
{
  return static_cast<NSInteger>(Framework::kBackgroundTilesMinCacheSizeMB);
}

+ (NSInteger)backgroundTilesMaxCacheSizeMB
{
  return static_cast<NSInteger>(Framework::kBackgroundTilesMaxCacheSizeMB);
}

+ (NSInteger)backgroundTilesMinAreaOpacityPct
{
  return static_cast<NSInteger>(Framework::kBackgroundTilesMinAreaOpacityPct);
}

+ (NSInteger)backgroundTilesMaxAreaOpacityPct
{
  return static_cast<NSInteger>(Framework::kBackgroundTilesMaxAreaOpacityPct);
}

+ (void)setBackgroundTilesEnabled:(BOOL)enabled
                              url:(NSString *)url
                      cacheSizeMB:(NSInteger)cacheSizeMB
                   areaOpacityPct:(NSInteger)areaOpacityPct
{
  GetFramework().SetBackgroundTiles(enabled, url.UTF8String, static_cast<uint32_t>(cacheSizeMB),
                                    static_cast<uint32_t>(areaOpacityPct));
}

+ (BOOL)isWellFormedBackgroundTilesURL:(NSString *)url
{
  return Framework::IsWellFormedBackgroundTilesURL(url.UTF8String);
}

@end
