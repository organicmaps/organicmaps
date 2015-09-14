#import "MWMSearchManager.h"
#import "MWMSearchTextField.h"
#import "MWMSearchTabbedViewProtocol.h"

@protocol MWMSearchTableViewProtocol <MWMSearchTabbedViewProtocol>

@property (nonnull, weak, nonatomic) MWMSearchTextField * searchTextField;

@property (nonatomic) MWMSearchManagerState state;

@end

@interface MWMSearchTableViewController : UIViewController

@property (nonatomic) BOOL searchOnMap;

- (nonnull instancetype)init __attribute__((unavailable("init is not available")));
- (nonnull instancetype)initWithDelegate:(nonnull id<MWMSearchTableViewProtocol>)delegate;

- (void)searchText:(nonnull NSString *)text forInputLocale:(nullable NSString *)locale;

@end
