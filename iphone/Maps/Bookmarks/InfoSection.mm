#import "InfoSection.h"
#include "Framework.h"
#import "MWMCategoryInfoCell.h"
#import "SwiftBridge.h"

@interface InfoSection () <MWMCategoryInfoCellDelegate> {
  kml::MarkGroupId m_categoryId;
}

@property(nonatomic) BOOL infoExpanded;
@property(weak, nonatomic) id<InfoSectionObserver> observer;

@end

@implementation InfoSection

- (instancetype)initWithCategoryId:(MWMMarkGroupID)categoryId
                          expanded:(BOOL)expanded
                          observer:(id<InfoSectionObserver>)observer {
  self = [super init];
  if (self) {
    m_categoryId = static_cast<kml::MarkGroupId>(categoryId);
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

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRow:(NSInteger)row {
  UITableViewCell *cell = [tableView dequeueReusableCellWithCellClass:MWMCategoryInfoCell.class];
  CHECK(cell, ("Invalid category info cell."));

  auto &f = GetFramework();
  auto &bm = f.GetBookmarkManager();
  bool const categoryExists = bm.HasBmCategory(m_categoryId);
  CHECK(categoryExists, ("Nonexistent category"));

  auto infoCell = (MWMCategoryInfoCell *)cell;
  auto const &categoryData = bm.GetCategoryData(m_categoryId);
  [infoCell updateWithCategoryData:categoryData delegate:self];
  infoCell.expanded = _infoExpanded;

  return cell;
}

- (BOOL)didSelectRow:(NSInteger)row {
  return NO;
}

- (void)deleteRow:(NSInteger)row {
}

#pragma mark - InfoSectionObserver

- (void)categoryInfoCellDidPressMore:(MWMCategoryInfoCell *)cell {
  _infoExpanded = YES;
  [self.observer infoSectionUpdates:^{
                   cell.expanded = YES;
                 }
                           expanded:_infoExpanded];
}

@end
