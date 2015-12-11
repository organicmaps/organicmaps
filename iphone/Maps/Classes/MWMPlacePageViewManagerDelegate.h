#import "MWMRoutingProtocol.h"

#include "platform/location.hpp"

@protocol MWMPlacePageViewManagerProtocol <MWMRoutingProtocol>

@property (nonatomic, readonly) location::EMyPositionMode myPositionMode;

- (void)dragPlacePage:(CGRect)frame;
- (void)addPlacePageViews:(NSArray *)views;
- (void)updateStatusBarStyle;
- (void)apiBack;
- (void)placePageDidClose;

@end