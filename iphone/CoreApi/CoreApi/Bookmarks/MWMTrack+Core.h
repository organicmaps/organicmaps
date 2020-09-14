#import "MWMTrack.h"

#include <CoreApi/Framework.h>

@interface MWMTrack (Core)

- (instancetype)initWithTrackId:(MWMMarkID)markId trackData:(Track const *)track;

@end
