#import "MWMSearch.h"
#import "MWMSearchManager.h"
#import "MWMSearchTabbedViewProtocol.h"
#import "MWMViewController.h"

#include <CoreApi/Framework.h>

@class SearchTextField;
namespace search
{
class Result;
}  // search

@protocol MWMSearchTableViewProtocol<MWMSearchTabbedViewProtocol>

@property(nullable, weak, nonatomic) SearchTextField * searchTextField;

@property(nonatomic) MWMSearchManagerState state;

- (void)processSearchWithResult:(search::Result const &)result;

@end

@interface MWMSearchTableViewController : MWMViewController<MWMSearchObserver>

- (nonnull instancetype)init __attribute__((unavailable("init is not available")));
- (nonnull instancetype)initWithDelegate:(nonnull id<MWMSearchTableViewProtocol>)delegate;

- (void)reloadData;

@end
