#import "MWMTableViewCell.h"

namespace kml
{
struct CategoryData;
}

@class MWMCategoryInfoCell;

@protocol MWMCategoryInfoCellDelegate

- (void)categoryInfoCellDidPressMore:(MWMCategoryInfoCell *)cell;

@end

@interface MWMCategoryInfoCell : MWMTableViewCell

@property (nonatomic) BOOL expanded;

- (void)updateWithCategoryData:(kml::CategoryData const &)data
                      delegate:(id<MWMCategoryInfoCellDelegate>)delegate;

@end
