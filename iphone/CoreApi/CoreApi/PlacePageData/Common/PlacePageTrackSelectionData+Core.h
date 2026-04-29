#include <CoreApi/Framework.h>
#import "PlacePageTrackSelectionData.h"

NS_ASSUME_NONNULL_BEGIN

@interface PlacePageTrackSelectionData (Core)

@property(nonatomic, readonly) FeatureID relationId;

- (instancetype)initWithTrackId:(MWMTrackID)trackId
                     relationId:(FeatureID)relationId
                          title:(NSString *)title
                          color:(UIColor *)color
                     isSelected:(BOOL)isSelected;

@end

NS_ASSUME_NONNULL_END
