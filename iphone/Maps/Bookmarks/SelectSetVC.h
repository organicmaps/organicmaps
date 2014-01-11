#import <UIKit/UIKit.h>

@interface SelectSetVC : UITableViewController
{
  size_t * m_index;
}
- (id)initWithIndex:(size_t *)index;

@end
