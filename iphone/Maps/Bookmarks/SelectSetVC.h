
#import <UIKit/UIKit.h>
#import "TableViewController.h"
#include "../../map/bookmark.hpp"

@class SelectSetVC;
@protocol SelectSetVCDelegate <NSObject>

- (void)selectSetVC:(SelectSetVC *)vc didUpdateBookmarkAndCategory:(BookmarkAndCategory const &)bookmarkAndCategory;

@end

@interface SelectSetVC : TableViewController

- (id)initWithBookmarkAndCategory:(BookmarkAndCategory const &)bookmarkAndCategory;

@property (nonatomic, weak) id <SelectSetVCDelegate> delegate;

@end
