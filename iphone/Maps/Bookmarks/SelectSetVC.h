#import "MWMTableViewController.h"

struct BookmarkAndCategory;

@protocol MWMSelectSetDelegate <NSObject>

- (void)didSelectCategory:(NSString *)category withBac:(BookmarkAndCategory const &)bac;

@end

@interface SelectSetVC : MWMTableViewController

- (instancetype)initWithCategory:(NSString *)category
                             bac:(BookmarkAndCategory const &)bac
                        delegate:(id<MWMSelectSetDelegate>)delegate;

@end
