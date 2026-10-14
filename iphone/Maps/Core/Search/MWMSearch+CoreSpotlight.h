#import "MWMSearch.h"

@interface MWMSearch (CoreSpotlight)

+ (void)addCategoriesToSpotlight;
+ (nullable NSString *)categoryKeyForSpotlightIdentifier:(nullable NSString *)identifier;
+ (nullable SearchQuery *)searchQueryForSpotlightIdentifier:(nullable NSString *)identifier;

@end
