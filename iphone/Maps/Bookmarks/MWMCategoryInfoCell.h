#import "MWMTableViewCell.h"

#include "Framework.h"

@class MWMCategoryInfoCell;

@protocol MWMCategoryInfoCellDelegate

- (void)categoryInfoCellDidPressMore:(MWMCategoryInfoCell *)cell;

@end

@interface MWMCategoryInfoCell : MWMTableViewCell

@property (nonatomic) BOOL expanded;
@property (weak, nonatomic) id<MWMCategoryInfoCellDelegate> delegate;

- (void)updateWithCategoryData:(kml::CategoryData const &)data
                      delegate:(id<MWMCategoryInfoCellDelegate>)delegate;

@end
