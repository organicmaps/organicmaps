#import <UIKit/UIKit.h>


class Framework;

@interface BookmarksVC : UIViewController
  <UITableViewDelegate, UITableViewDataSource>
{
  Framework * m_framework;
  UITableView * m_table;
}

- (id)initWithFramework:(Framework *)f;
@end
