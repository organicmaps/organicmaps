#import <UIKit/UIKit.h>

namespace mapinfo { class Storage; }

@interface CountriesViewController 
: UIViewController <UINavigationBarDelegate, UITableViewDelegate, UITableViewDataSource, UIActionSheetDelegate>
{
}

- (id) initWithStorage: (mapinfo::Storage &) storage;

@end
