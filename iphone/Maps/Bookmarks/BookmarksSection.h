#import "TableSectionDataSource.h"

#include "kml/type_utils.hpp"

@class BookmarksSection;

@protocol BookmarksSectionDelegate <NSObject>

- (NSInteger)numberOfBookmarksInSection:(BookmarksSection *)bookmarkSection;
- (NSString *)titleOfBookmarksSection:(BookmarksSection *)bookmarkSection;
- (BOOL)canEditBookmarksSection:(BookmarksSection *)bookmarkSection;
- (kml::MarkId)bookmarkSection:(BookmarksSection *)bookmarkSection getBookmarkIdByRow:(NSInteger)row;
- (BOOL)bookmarkSection:(BookmarksSection *)bookmarkSection onDeleteBookmarkInRow:(NSInteger)row;

@end

@interface BookmarksSection : NSObject <TableSectionDataSource>

@property (nullable, nonatomic) NSNumber * blockIndex;

- (instancetype)initWithDelegate:(id<BookmarksSectionDelegate>)delegate;
- (instancetype)initWithBlockIndex:(NSNumber *)blockIndex delegate:(id<BookmarksSectionDelegate>)delegate;

@end
