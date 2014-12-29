#import <UIKit/UIKit.h>
#import "TableViewController.h"

@interface BookmarksRootVC : TableViewController <UITextFieldDelegate>
{
  /// Description for the user: how to create/import bookmarks.
  /// We store it here to correctly calculate dynamic table footer height depending on the text formatting.
  UIView * m_hint;
}

- (id)init;

@end
