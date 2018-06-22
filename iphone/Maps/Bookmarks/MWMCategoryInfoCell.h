#import "MWMTableViewCell.h"

@class MWMCategoryInfoCell;

@protocol MWMCategoryInfoCellDelegate
- (void)categoryInfoCellDidPressMore:(MWMCategoryInfoCell *)cell;
@end

@interface MWMCategoryInfoCell : MWMTableViewCell
@property (nonatomic) BOOL expanded;
@property (weak, nonatomic) id<MWMCategoryInfoCellDelegate> delegate;

- (void)updateWithTitle:(NSString *)title
                 author:(NSString *)author
                   info:(NSString *)info
              shortInfo:(NSString *)shortInfo;
@end
