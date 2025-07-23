#import "SearchItemType.h"

#import <CoreLocation/CoreLocation.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface SearchResult : NSObject

@property(nonatomic, readonly) NSUInteger index;
@property(nonatomic, readonly) NSString * titleText;
@property(nonatomic, readonly) NSString * iconImageName;
@property(nonatomic, readonly) NSString * addressText;
@property(nonatomic, readonly) NSString * infoText;
@property(nonatomic, readonly) CLLocationCoordinate2D coordinate;
@property(nonatomic, readonly) CGPoint point;
@property(nonatomic, readonly, nullable) NSString * distanceText;
@property(nonatomic, readonly, nullable) NSString * openStatusText;
@property(nonatomic, readonly) UIColor * openStatusColor;
@property(nonatomic, readonly) BOOL isPopularHidden;
@property(nonatomic, readonly) NSString * suggestion;
@property(nonatomic, readonly) BOOL isPureSuggest;
@property(nonatomic, readonly) NSArray<NSValue *> * highlightRanges;
@property(nonatomic, readonly) SearchItemType itemType;

/// This initializer is intended only for testing purposes.
- (instancetype)initWithTitleText:(NSString *)titleText type:(SearchItemType)type suggestion:(NSString *)suggestion;

@end

NS_ASSUME_NONNULL_END
