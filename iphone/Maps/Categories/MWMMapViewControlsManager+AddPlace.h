#import <Foundation/Foundation.h>
#include "geometry/point2d.hpp"

NS_ASSUME_NONNULL_BEGIN

@interface MWMMapViewControlsManager (AddPlace)

- (void)addPlace:(BOOL)isBusiness hasPoint:(BOOL)hasPoint point:(m2::PointD const &)point;

@end

NS_ASSUME_NONNULL_END
