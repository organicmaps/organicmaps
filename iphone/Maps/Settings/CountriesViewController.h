#import <UIKit/UIKit.h>

#include "../../../storage/storage.hpp"

@interface CountriesViewController
: UIViewController <UINavigationBarDelegate, UITableViewDelegate, UITableViewDataSource, UIActionSheetDelegate>
{
	storage::Storage * m_storage;
  storage::TIndex m_index;
}

- (id) initWithStorage: (storage::Storage &) storage andIndex: (storage::TIndex const &) index andHeader: (NSString *) header;

- (void) OnCountryChange: (storage::TIndex const &) index;
- (void) OnDownload: (storage::TIndex const &) index withProgress: (HttpProgressT const &) progress;

@end
