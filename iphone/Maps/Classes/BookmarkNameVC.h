
#import <UIKit/UIKit.h>
#include "../../map/bookmark.hpp"

@class BookmarkNameVC;
@protocol BookmarkNameVCDelegate <NSObject>

- (void)bookmarkNameVC:(BookmarkNameVC *)vc didUpdateBookmarkAndCategory:(BookmarkAndCategory const &)bookmarkAndCategory;

@end

@interface BookmarkNameVC : UITableViewController

@property (nonatomic) BookmarkAndCategory bookmarkAndCategory;
@property (nonatomic) NSString * temporaryName;
@property (nonatomic, weak) id <BookmarkNameVCDelegate> delegate;

@end
