#import <UIKit/UIKit.h>

namespace storage { class Storage; }

@interface CountriesViewController 
: UIViewController <UINavigationBarDelegate, UITableViewDelegate, UITableViewDataSource, UIActionSheetDelegate>
{
}

- (id) initWithStorage: (storage::Storage &) storage;

@end
