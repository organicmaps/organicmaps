#import "MWMMyPositionMode.h"

#include "platform/location.hpp"

using TPredictionBlock = void (^)(CLLocation *);

@interface MWMLocationPredictor : NSObject

- (instancetype)initWithOnPredictionBlock:(TPredictionBlock)onPredictBlock;
- (void)reset:(CLLocation *)info;
- (void)setMyPositionMode:(MWMMyPositionMode)mode;

@end
