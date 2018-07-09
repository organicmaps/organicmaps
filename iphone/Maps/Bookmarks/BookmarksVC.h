#import "MWMTableViewController.h"
#import "MWMTypes.h"

@interface BookmarksVC : MWMTableViewController <UITextFieldDelegate>
{
  MWMMarkGroupID m_categoryId;
}

- (instancetype)initWithCategory:(MWMMarkGroupID)index;

@end
