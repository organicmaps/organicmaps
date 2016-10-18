#import "MWMTableViewController.h"

@interface BookmarksVC : MWMTableViewController <UITextFieldDelegate>
{
  NSUInteger m_categoryIndex;
}

- (instancetype)initWithCategory:(NSUInteger)index;

@end
