#import <CoreLocation/CoreLocation.h>
#import <UIKit/UIKit.h>

#import "MWMTypes.h"

NS_ASSUME_NONNULL_BEGIN

@interface MWMCarPlayBookmarkObject : NSObject
@property(assign, nonatomic, readonly) MWMMarkID bookmarkId;
@property(strong, nonatomic, readonly) NSString * prefferedName;
@property(strong, nonatomic, readonly) NSString * address;
@property(assign, nonatomic, readonly) CLLocationCoordinate2D coordinate;
@property(assign, nonatomic, readonly) CGPoint mercatorPoint;

/// Takes a snapshot of a live bookmark on the Core thread. IDs retained by UI must be checked before calling.
- (instancetype)initWithBookmarkId:(MWMMarkID)bookmarkId;
@end

NS_ASSUME_NONNULL_END
