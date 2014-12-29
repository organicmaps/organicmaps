
#import <UIKit/UIKit.h>
#import "TableViewController.h"
#include "../../map/bookmark.hpp"

@class BookmarkNameVC;
@protocol BookmarkNameVCDelegate <NSObject>

- (void)bookmarkNameVC:(BookmarkNameVC *)vc didUpdateBookmarkAndCategory:(BookmarkAndCategory const &)bookmarkAndCategory;

@end

@interface BookmarkNameVC : TableViewController

@property (nonatomic) BookmarkAndCategory bookmarkAndCategory;
@property (nonatomic) NSString * temporaryName;
@property (nonatomic, weak) id <BookmarkNameVCDelegate> delegate;

@end
