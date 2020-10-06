#import <Foundation/Foundation.h>

#import "MWMTypes.h"

@class MWMBookmark;
@class MWMTrack;

NS_ASSUME_NONNULL_BEGIN

@class MWMBookmarksManager;

NS_SWIFT_NAME(BookmarkGroup)
@interface MWMBookmarkGroup : NSObject

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithCategoryId:(MWMMarkGroupID)categoryId
                            bookmarksManager:(MWMBookmarksManager *)manager;

@property(nonatomic, readonly) MWMMarkGroupID categoryId;
@property(nonatomic, readonly) NSString *title;
@property(nonatomic, readonly) NSString *author;
@property(nonatomic, readonly) NSString *annotation;
@property(nonatomic, readonly) NSString *detailedAnnotation;
@property(nonatomic, readonly) NSString *serverId;
@property(nonatomic, readonly, nullable) NSURL *imageUrl;
@property(nonatomic, readonly) NSInteger bookmarksCount;
@property(nonatomic, readonly) NSInteger trackCount;
@property(nonatomic, readonly, getter=isVisible) BOOL visible;
@property(nonatomic, readonly, getter=isEmpty) BOOL empty;
@property(nonatomic, readonly, getter=isEditable) BOOL editable;
@property(nonatomic, readonly, getter=isGuide) BOOL guide;
@property(nonatomic, readonly, getter=isLonelyPlanet) BOOL lonelyPlanet;
@property(nonatomic, readonly) BOOL hasDescription;
@property(nonatomic, readonly) MWMBookmarkGroupAccessStatus accessStatus;
@property(nonatomic, readonly) NSArray<MWMBookmark *> *bookmarks;
@property(nonatomic, readonly) NSArray<MWMTrack *> *tracks;

@end

NS_ASSUME_NONNULL_END
