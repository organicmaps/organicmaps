#import "MWMSearchManager.h"
#import "MWMSearchTextField.h"
#import "MWMSearchTabbedViewProtocol.h"
#import "ViewController.h"

@protocol MWMSearchTableViewProtocol <MWMSearchTabbedViewProtocol>

@property (weak, nonatomic) MWMSearchTextField * searchTextField;

@property (nonatomic) MWMSearchManagerState state;

- (void)processSearchWithResult:(search::Result const &)result
                          query:(search::QuerySaver::TSearchRequest const &)query;

@end

@interface MWMSearchTableViewController : ViewController

@property (nonatomic) BOOL searchOnMap;

- (nonnull instancetype)init __attribute__((unavailable("init is not available")));
- (nonnull instancetype)initWithDelegate:(nonnull id<MWMSearchTableViewProtocol>)delegate;

- (void)searchText:(nonnull NSString *)text forInputLocale:(nullable NSString *)locale;
- (search::SearchParams const &)searchParams;

@end
