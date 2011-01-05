#import <UIKit/UIKit.h>

#include "../../../storage/storage.hpp"

@interface CountriesViewController 
: UIViewController <UINavigationBarDelegate, UITableViewDelegate, UITableViewDataSource, UIActionSheetDelegate>
{
	storage::Storage * m_storage;
  storage::TIndex m_index;
}

- (id) initWithStorage: (storage::Storage &) storage andIndex: (storage::TIndex const &) index andHeader: (NSString *) header;

@end
