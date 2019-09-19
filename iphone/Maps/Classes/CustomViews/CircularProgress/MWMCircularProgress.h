#import "MWMCircularProgress+Swift.h"

#include <vector>

using MWMCircularProgressStateVec = std::vector<MWMCircularProgressState>;

@interface MWMCircularProgress ()

- (void)setImageName:(nullable NSString *)imageName
           forStates:(MWMCircularProgressStateVec const &)states;
- (void)setColor:(nonnull UIColor *)color forStates:(MWMCircularProgressStateVec const &)states;
- (void)setColoring:(MWMButtonColoring)coloring
          forStates:(MWMCircularProgressStateVec const &)states;
@end
