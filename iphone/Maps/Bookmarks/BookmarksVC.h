#import "MWMViewController.h"
#import "MWMTypes.h"

@class BookmarksVC;

@protocol BookmarksVCDelegate

- (void)bookmarksVCdidUpdateCategory:(BookmarksVC *)viewController;
- (void)bookmarksVCdidDeleteCategory:(BookmarksVC *)viewController;

@end

@interface BookmarksVC : MWMViewController <UITextFieldDelegate>
{
  MWMMarkGroupID m_categoryId;
}

@property (weak, nonatomic) id<BookmarksVCDelegate> delegate;

- (instancetype)initWithCategory:(MWMMarkGroupID)index;

@end
