#import "MWMMapDownloaderMode.h"

#include "geometry/point2d.hpp"

@protocol MWMBottomMenuControllerProtocol<NSObject>

- (void)actionDownloadMaps:(MWMMapDownloaderMode)mode;
- (void)addPlace:(BOOL)isBusiness hasPoint:(BOOL)hasPoint point:(m2::PointD const &)point;
- (void)didFinishAddingPlace;

@end
