#import <UIKit/UIKit.h>

#include "map/location_state.hpp"

@interface MWMLocationButtonView : UIButton

@property (nonatomic) location::State::Mode locationState;

- (instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("initWithFrame is not available")));
- (instancetype)init __attribute__((unavailable("init is not available")));

@end
