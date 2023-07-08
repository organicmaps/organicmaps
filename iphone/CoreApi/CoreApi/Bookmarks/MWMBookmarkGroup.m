#import "MWMBookmarkGroup.h"
#import "MWMBookmarksManager.h"

@interface MWMBookmarkGroup ()

@property(weak, nonatomic) MWMBookmarksManager *manager;

@end

@implementation MWMBookmarkGroup

- (instancetype)initWithCategoryId:(MWMMarkGroupID)categoryId
                  bookmarksManager:(MWMBookmarksManager *)manager {
  self = [super init];
  if (self) {
    _manager = manager;
    _categoryId = categoryId;
  }

  return self;
}

- (NSString *)title {
  return [self.manager getCategoryName:self.categoryId];
}

- (NSString *)author {
  return [self.manager getCategoryAuthorName:self.categoryId];
}

- (NSString *)annotation {
  return [self.manager getCategoryAnnotation:self.categoryId];
}

- (NSString *)detailedAnnotation {
  return [self.manager getCategoryDescription:self.categoryId];
}

- (NSURL *)imageUrl {
  return [self.manager getCategoryImageUrl:self.categoryId];
}

- (NSInteger)bookmarksCount {
  return [self.manager getCategoryMarksCount:self.categoryId];
}

- (NSInteger)trackCount {
  return [self.manager getCategoryTracksCount:self.categoryId];
}

- (BOOL)isVisible {
  return [self.manager isCategoryVisible:self.categoryId];
}

- (BOOL)isEmpty {
  return ![self.manager isCategoryNotEmpty:self.categoryId];
}

- (BOOL)hasDescription {
  return [self.manager hasExtraInfo:self.categoryId];
}

- (MWMBookmarkGroupAccessStatus)accessStatus {
  return [self.manager getCategoryAccessStatus:self.categoryId];
}

- (NSArray<MWMBookmark *> *)bookmarks {
  return [self.manager bookmarksForGroup:self.categoryId];
}

- (NSArray<MWMTrack *> *)tracks {
  return [self.manager tracksForGroup:self.categoryId];
}

- (NSArray<MWMBookmarkGroup *> *)collections {
  return [self.manager collectionsForGroup:self.categoryId];
}

- (NSArray<MWMBookmarkGroup *> *)categories {
  return [self.manager categoriesForGroup:self.categoryId];
}

- (MWMBookmarkGroupType)type {
  return [self.manager getCategoryGroupType:self.categoryId];
}

- (BOOL)isHtmlDescription {
  return [self.manager isHtmlDescription:self.categoryId];
}

@end
