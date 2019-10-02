#import "MWMViewController.h"
#import <CoreApi/MWMTypes.h>

NS_ASSUME_NONNULL_BEGIN

@class BookmarksVC;

@protocol BookmarksVCDelegate

- (void)bookmarksVCdidUpdateCategory:(BookmarksVC *)viewController;
- (void)bookmarksVCdidDeleteCategory:(BookmarksVC *)viewController;

@end

@interface BookmarksVC : MWMViewController <UITextFieldDelegate>

@property (nonatomic) MWMMarkGroupID categoryId;
@property (weak, nonatomic) id<BookmarksVCDelegate> delegate;

- (instancetype)initWithCategory:(MWMMarkGroupID)categoryId;

@end

NS_ASSUME_NONNULL_END
