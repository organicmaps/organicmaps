#import "MWMMapDownloaderTypes.h"

#include "platform/location.hpp"

@protocol MWMBottomMenuControllerProtocol<NSObject>

- (void)actionDownloadMaps:(mwm::DownloaderMode)mode;
- (void)addPlace:(BOOL)isBusiness hasPoint:(BOOL)hasPoint point:(m2::PointD const &)point;
- (void)didFinishAddingPlace;

@end
