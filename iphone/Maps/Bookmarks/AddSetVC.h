#import <UIKit/UIKit.h>

@interface AddSetVC : UITableViewController <UITextFieldDelegate>
{
  size_t * m_index;
}

- (id) initWithIndex:(size_t *)index;

@end
