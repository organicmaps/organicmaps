#import "MWMTableViewController.h"

struct BookmarkAndCategory;

@protocol MWMSelectSetDelegate <NSObject>

- (void)didSelectCategory:(NSString *)category withCategoryIndex:(size_t)categoryIndex;

@end

@interface SelectSetVC : MWMTableViewController

- (instancetype)initWithCategory:(NSString *)category
                   categoryIndex:(size_t)categoryIndex
                        delegate:(id<MWMSelectSetDelegate>)delegate;

@end
