#import <Foundation/Foundation.h>
#include "geometry/point2d.hpp"

@interface MWMMapViewControlsManager (AddPlace)

- (void)addPlace:(BOOL)isBusiness position:(nullable m2::PointD const *)optionalPosition;

@end
