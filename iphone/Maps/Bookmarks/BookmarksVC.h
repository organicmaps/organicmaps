#import "MWMTableViewController.h"

#include "drape_frontend/user_marks_global.hpp"

@interface BookmarksVC : MWMTableViewController <UITextFieldDelegate>
{
  df::MarkGroupID m_categoryId;
  NSMutableArray * m_bookmarkIds;
  NSMutableArray * m_trackIds;
}

- (instancetype)initWithCategory:(df::MarkGroupID)index;

@end
