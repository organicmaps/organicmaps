#import "MWMCircularProgress+Swift.h"

#include "std/vector.hpp"

using MWMCircularProgressStateVec = vector<MWMCircularProgressState>;

@interface MWMCircularProgress ()

- (void)setImageName:(nullable NSString *)imageName
           forStates:(MWMCircularProgressStateVec const &)states;
- (void)setColor:(nonnull UIColor *)color forStates:(MWMCircularProgressStateVec const &)states;
- (void)setColoring:(MWMButtonColoring)coloring
          forStates:(MWMCircularProgressStateVec const &)states;
@end
