#import <UIKit/UIKit.h>

@interface BookmarksVC : UIViewController<UITableViewDelegate, UITableViewDataSource>
{
  UITableView * m_table;
  // Needed to change Edit/Cancel buttons
  UINavigationItem * m_navItem;
}
@end