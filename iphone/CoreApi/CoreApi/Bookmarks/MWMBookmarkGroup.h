#import <Foundation/Foundation.h>

#import "MWMTypes.h"

NS_ASSUME_NONNULL_BEGIN

@class MWMBookmarksManager;

NS_SWIFT_NAME(BookmarkGroup)
@interface MWMBookmarkGroup : NSObject

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithCategoryId:(MWMMarkGroupID)categoryId
                            bookmarksManager:(MWMBookmarksManager *)manager;

@property(nonatomic, readonly) MWMMarkGroupID categoryId;
@property(nonatomic, copy) NSString *title;
@property(nonatomic, readonly) NSString *author;
@property(nonatomic, readonly) NSString *annotation;
@property(nonatomic, copy) NSString *detailedAnnotation;
@property(nonatomic, readonly) NSInteger bookmarksCount;
@property(nonatomic, readonly) NSInteger trackCount;
@property(nonatomic, getter=isVisible) BOOL visible;
@property(nonatomic, readonly, getter=isEmpty) BOOL empty;
@property(nonatomic, readonly) MWMBookmarkGroupAccessStatus accessStatus;

@end

NS_ASSUME_NONNULL_END
