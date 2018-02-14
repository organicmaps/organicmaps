#import "MWMTableViewController.h"

#include "drape_frontend/user_marks_global.hpp"

@interface BookmarksVC : MWMTableViewController <UITextFieldDelegate>
{
  df::MarkGroupID m_categoryId;
}

- (instancetype)initWithCategory:(df::MarkGroupID)index;

@end
