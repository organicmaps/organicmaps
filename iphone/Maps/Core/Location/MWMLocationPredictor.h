#import "MWMTypes.h"

#include "platform/location.hpp"

using TPredictionBlock = void (^)(location::GpsInfo const &);

@interface MWMLocationPredictor : NSObject

- (instancetype)initWithOnPredictionBlock:(TPredictionBlock)onPredictBlock;
- (void)reset:(location::GpsInfo const &)info;
- (void)setMyPositionMode:(location::EMyPositionMode)mode;

@end
