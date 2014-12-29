
#import <UIKit/UIKit.h>
#import "TableViewController.h"
#include "../../map/bookmark.hpp"

@class BookmarkDescriptionVC;
@protocol BookmarkDescriptionVCDelegate <NSObject>

- (void)bookmarkDescriptionVC:(BookmarkDescriptionVC *)vc didUpdateBookmarkAndCategory:(BookmarkAndCategory const &)bookmarkAndCategory;

@end

@interface BookmarkDescriptionVC : TableViewController

@property (nonatomic, weak) id <BookmarkDescriptionVCDelegate> delegate;
@property (nonatomic) BookmarkAndCategory bookmarkAndCategory;

@end
