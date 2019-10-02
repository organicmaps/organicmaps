#import <UIKit/UIKit.h>
#import <CoreLocation/CoreLocation.h>

#import "MWMTypes.h"

NS_ASSUME_NONNULL_BEGIN

@interface MWMCarPlayBookmarkObject : NSObject
@property(assign, nonatomic, readonly) MWMMarkID bookmarkId;
@property(strong, nonatomic, readonly) NSString *prefferedName;
@property(strong, nonatomic, readonly) NSString *address;
@property(assign, nonatomic, readonly) CLLocationCoordinate2D coordinate;
@property(assign, nonatomic, readonly) CGPoint mercatorPoint;

- (instancetype)initWithBookmarkId:(MWMMarkID)bookmarkId;
@end

NS_ASSUME_NONNULL_END
