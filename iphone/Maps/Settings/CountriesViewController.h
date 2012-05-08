#import <UIKit/UIKit.h>

#include "../../../storage/storage.hpp"

@interface CountriesViewController
: UIViewController <UINavigationBarDelegate, UITableViewDelegate, UITableViewDataSource,
  UIActionSheetDelegate, UIAlertViewDelegate>
{
  storage::TIndex m_index;
}

- (id) initWithIndex: (storage::TIndex const &) index andHeader: (NSString *) header;

- (void) OnCountryChange: (storage::TIndex const &) index;
- (void) OnDownload: (storage::TIndex const &) index withProgress: (pair<int64_t, int64_t> const &) progress;

@end
