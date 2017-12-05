#import "Statistics.h"

#include "platform/platform.hpp"

@interface Statistics (ConnectionTypeLogging)

+ (NSString *)connectionTypeToString:(Platform::EConnectionType)type;

@end
