#import <CoreApi/MWMTypes.h>
#import "TableSectionDataSource.h"

NS_ASSUME_NONNULL_BEGIN

@interface TracksSection : NSObject <TableSectionDataSource>

- (instancetype)initWithTitle:(nullable NSString *)title
                     trackIds:(MWMTrackIDCollection)trackIds
                   isEditable:(BOOL)isEditable;

@end

NS_ASSUME_NONNULL_END
