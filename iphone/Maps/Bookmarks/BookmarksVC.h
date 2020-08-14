#import "MWMViewController.h"
#import <CoreApi/MWMTypes.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, BookmarksVCSelectedType) {
  BookmarksVCSelectedTypeNone,
  BookmarksVCSelectedTypeBookmark,
  BookmarksVCSelectedTypeTrack
};

@class BookmarksVC;

@protocol BookmarksVCDelegate

- (void)bookmarksVCdidUpdateCategory:(BookmarksVC *)viewController;
- (void)bookmarksVCdidDeleteCategory:(BookmarksVC *)viewController;
- (void)bookmarksVCdidViewOnMap:(BookmarksVC *)viewController type:(BookmarksVCSelectedType) type;

@end

@interface BookmarksVC : MWMViewController <UITextFieldDelegate>

@property (nonatomic) MWMMarkGroupID categoryId;
@property (weak, nonatomic) id<BookmarksVCDelegate> delegate;

- (instancetype)initWithCategory:(MWMMarkGroupID)categoryId;

@end

NS_ASSUME_NONNULL_END
