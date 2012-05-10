#import <UIKit/UIKit.h>

class BookmarkCategory;

@interface BookmarksVC : UIViewController<UITableViewDelegate, UITableViewDataSource>
{
  UITableView * m_table;
  // Needed to change Edit/Cancel buttons
  UINavigationItem * m_navItem;

  // Current category to show.
  BookmarkCategory * m_category;
}
@end
