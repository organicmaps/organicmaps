
#import <UIKit/UIKit.h>
#include "../../map/bookmark.hpp"

@class SelectSetVC;
@protocol SelectSetVCDelegate <NSObject>

- (void)selectSetVC:(SelectSetVC *)vc didUpdateBookmarkAndCategory:(BookmarkAndCategory const &)bookmarkAndCategory;

@end

@interface SelectSetVC : UITableViewController

- (id)initWithBookmarkAndCategory:(BookmarkAndCategory const &)bookmarkAndCategory;

@property (nonatomic, weak) id <SelectSetVCDelegate> delegate;

@end
