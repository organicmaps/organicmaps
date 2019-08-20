#import "TableSectionDataSource.h"

NS_ASSUME_NONNULL_BEGIN

@protocol InfoSectionObserver

- (void)infoSectionUpdates:(void (^_Nullable)(void))updates expanded:(BOOL)expanded;

@end

@interface InfoSection : NSObject <TableSectionDataSource>

- (instancetype)initWithCategoryId:(MWMMarkGroupID)categoryId
                          expanded:(BOOL)expanded
                          observer:(id<InfoSectionObserver>)observer;

@end

NS_ASSUME_NONNULL_END
