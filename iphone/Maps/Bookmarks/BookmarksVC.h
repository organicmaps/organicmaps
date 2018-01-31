#import "MWMTableViewController.h"

@interface BookmarksVC : MWMTableViewController <UITextFieldDelegate>
{
  NSUInteger m_categoryId;
}

- (instancetype)initWithCategory:(NSUInteger)index;

@end
