#import <UIKit/UIKit.h>


class Framework;

@interface BookmarksVC : UIViewController
  <UITableViewDelegate, UITableViewDataSource>
{
  Framework * m_framework;
  UITableView * m_table;
  // Needed to change Edit/Cancel buttons
  UINavigationItem * m_navItem;
}

- (id)initWithFramework:(Framework *)f;
@end
