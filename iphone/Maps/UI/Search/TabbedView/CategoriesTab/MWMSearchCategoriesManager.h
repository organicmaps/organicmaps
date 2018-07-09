#import "MWMSearchTabbedCollectionViewCell.h"
#import "MWMSearchTabbedViewProtocol.h"

@interface MWMSearchCategoriesManager : NSObject <UITableViewDataSource, UITableViewDelegate>

@property (weak, nonatomic) id<MWMSearchTabbedViewProtocol> delegate;

- (void)attachCell:(MWMSearchTabbedCollectionViewCell *)cell;

- (void)resetCategories;

@end
