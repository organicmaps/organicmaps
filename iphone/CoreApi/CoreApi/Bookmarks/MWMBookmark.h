#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

#import "MWMBookmarkColor.h"
#import "MWMTypes.h"

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(Bookmark)
@interface MWMBookmark : NSObject

@property(nonatomic, readonly) MWMMarkID bookmarkId;
@property(nonatomic, readonly) NSString *bookmarkName;
@property(nonatomic, readonly, nullable) NSString *bookmarkType;
@property(nonatomic, readonly) MWMBookmarkColor bookmarkColor;
@property(nonatomic, readonly) NSString *bookmarkIconName;
@property(nonatomic, readonly) CLLocationCoordinate2D locationCoordinate;

@end

NS_ASSUME_NONNULL_END
