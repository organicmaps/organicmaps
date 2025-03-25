#import "RoutePreviewView.h"

@interface MWMRoutePreview : UIView <RoutePreviewView>

@property(nonatomic) MWMDrivingOptionsState drivingOptionsState;
@property(weak, nonatomic) id<MWMRoutePreviewDelegate> delegate;

@end
