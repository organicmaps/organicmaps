#import "TableSectionDataSource.h"

NS_ASSUME_NONNULL_BEGIN

@protocol InfoSectionDelegate

- (void)infoSectionUpdates:(void (^)(void))updates;

@end

@interface InfoSection : NSObject <TableSectionDataSource>

- (instancetype)initWithCategoryId:(MWMMarkGroupID)categoryId
                          expanded:(BOOL)expanded
                          observer:(id<InfoSectionDelegate>)observer;

@end

NS_ASSUME_NONNULL_END
