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

- (NSString *)serverId {
  return [self.manager getServerId:self.categoryId];
}

- (NSString *)imageUrl {
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

- (BOOL)isEditable {
  return [self.manager isCategoryEditable:self.categoryId];
}

- (BOOL)isGuide {
  return [self.manager isGuide:self.categoryId];
}

- (BOOL)isLonelyPlanet {
  return [[self.manager getCategoryAuthorId:self.categoryId] isEqualToString: @"28035594-6457-466d-8f6f-8499607df570"];
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

@end
