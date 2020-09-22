#import <CoreApi/MWMTypes.h>
#import "MWMViewController.h"

NS_ASSUME_NONNULL_BEGIN

@class BookmarksVC;

@protocol BookmarksVCDelegate

- (void)bookmarksVCdidUpdateCategory:(BookmarksVC *)viewController;
- (void)bookmarksVCdidDeleteCategory:(BookmarksVC *)viewController;
- (void)bookmarksVCdidViewOnMap:(BookmarksVC *)viewController categoryId:(MWMMarkGroupID)categoryId;

@end

@interface BookmarksVC : MWMViewController <UITextFieldDelegate>

@property(nonatomic) MWMMarkGroupID categoryId;
@property(weak, nonatomic) id<BookmarksVCDelegate> delegate;

- (instancetype)initWithCategory:(MWMMarkGroupID)categoryId;

@end

NS_ASSUME_NONNULL_END
