#import <Foundation/Foundation.h>

@class MWMBookmark;
@class MWMTrack;

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(BookmarksSection)
@interface MWMBookmarksSection : NSObject

@property(nonatomic, readonly) NSString *sectionName;
@property(nonatomic, readonly, nullable) NSArray<MWMBookmark *> *bookmarks;
@property(nonatomic, readonly, nullable) NSArray<MWMTrack *> *tracks;

- (instancetype)initWithTitle:(NSString *)title
                    bookmarks:(nullable NSArray<MWMBookmark *> *)bookmarks
                       tracks:(nullable NSArray<MWMTrack *> *)tracks;

@end

NS_ASSUME_NONNULL_END
