#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(MapSearchResult)
@interface MWMMapSearchResult : NSObject

@property(nonatomic, readonly) NSString *countryId;
@property(nonatomic, readonly) NSString *matchedName;

@end

NS_ASSUME_NONNULL_END
