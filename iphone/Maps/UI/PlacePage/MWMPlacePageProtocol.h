#import "MWMMapViewControlsManager.h"

struct FeatureID;

@protocol MWMFeatureHolder <NSObject>

- (FeatureID const &)featureId;

@end

@protocol MWMPlacePageProtocol <MWMFeatureHolder>

- (BOOL)isPPShown;

@end
