#import <CoreApi/MWMTypes.h>
#import "TableSectionDataSource.h"

NS_ASSUME_NONNULL_BEGIN

@interface BookmarksSection : NSObject <TableSectionDataSource>

- (instancetype)initWithTitle:(nullable NSString *)title
                      markIds:(MWMMarkIDCollection)markIds
                   isEditable:(BOOL)isEditable;

@end

NS_ASSUME_NONNULL_END
