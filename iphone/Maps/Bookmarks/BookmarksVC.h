#import "MWMTableViewController.h"

@interface BookmarksVC : MWMTableViewController <UITextFieldDelegate>
{
  size_t m_categoryIndex;
}

- (instancetype)initWithCategory:(size_t)index;

@end
