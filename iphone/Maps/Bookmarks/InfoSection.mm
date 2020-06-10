#import "InfoSection.h"
#import "MWMCategoryInfoCell.h"
//#import "SwiftBridge.h"

#include <CoreApi/Framework.h>

@interface InfoSection () <MWMCategoryInfoCellDelegate> {
  kml::MarkGroupId _categoryId;
}

@property(nonatomic) BOOL infoExpanded;
@property(weak, nonatomic) id<InfoSectionDelegate> observer;
@property(strong, nonatomic) MWMCategoryInfoCell *infoCell;
@end

@implementation InfoSection

- (instancetype)initWithCategoryId:(MWMMarkGroupID)categoryId
                          expanded:(BOOL)expanded
                          observer:(id<InfoSectionDelegate>)observer {
  self = [super init];
  if (self) {
    _categoryId = categoryId;
    _infoExpanded = expanded;
    _observer = observer;
  }
  return self;
}

- (NSInteger)numberOfRows {
  return 1;
}

- (nullable NSString *)title {
  return L(@"placepage_place_description");
}

- (BOOL)canEdit {
  return NO;
}

- (BOOL)canSelect {
  return NO;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRow:(NSInteger)row {
  if (self.infoCell != nil)
    return self.infoCell;
  
  UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"MWMCategoryInfoCell"];
  CHECK(cell, ("Invalid category info cell."));

  auto &bm = GetFramework().GetBookmarkManager();
  bool const categoryExists = bm.HasBmCategory(_categoryId);
  CHECK(categoryExists, ("Nonexistent category"));

  self.infoCell = (MWMCategoryInfoCell *)cell;
  auto const &categoryData = bm.GetCategoryData(_categoryId);
  [self.infoCell updateWithCategoryData:categoryData delegate:self];
  self.infoCell.expanded = self.infoExpanded;

  return cell;
}

- (void)didSelectRow:(NSInteger)row {
}

- (void)deleteRow:(NSInteger)row {
}

#pragma mark - InfoSectionDelegate

- (void)categoryInfoCellDidPressMore:(MWMCategoryInfoCell *)cell {
  _infoExpanded = YES;
  [self.observer infoSectionUpdates:^{
                   cell.expanded = YES;
                 }];
}

@end
