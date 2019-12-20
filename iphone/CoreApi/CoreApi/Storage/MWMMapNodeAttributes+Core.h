#import "MWMMapNodeAttributes.h"

#include <CoreApi/Framework.h>

NS_ASSUME_NONNULL_BEGIN

@interface MWMMapNodeAttributes (Core)

- (instancetype)initWithCoreAttributes:(storage::NodeAttrs const &)attributes
                             countryId:(NSString *)countryId
                             hasParent:(BOOL)hasParent
                           hasChildren:(BOOL)hasChildren;

@end

NS_ASSUME_NONNULL_END
