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

- (void)setTitle:(NSString *)title {
  [self.manager setCategory:self.categoryId name:title];
}

- (NSString *)author {
  return [self.manager getCategoryAuthorName:self.categoryId];
}

- (NSString *)detailedAnnotation {
  return [self.manager getCategoryDescription:self.categoryId];
}

- (void)setDetailedAnnotation:(NSString *)detailedAnnotation {
  [self.manager setCategory:self.categoryId description:detailedAnnotation];
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

- (void)setVisible:(BOOL)visible {
  [self.manager setCategory:self.categoryId isVisible:visible];
}

- (BOOL)isEmpty {
  return ![self.manager isCategoryNotEmpty:self.categoryId];
}

- (MWMBookmarkGroupAccessStatus)accessStatus {
  return [self.manager getCategoryAccessStatus:self.categoryId];
}

@end
