#import "MWMTableViewController.h"

@interface BookmarksVC : MWMTableViewController <UITextFieldDelegate>
{
  int m_categoryIndex;
}

- (instancetype)initWithCategory:(int)index;

@end
