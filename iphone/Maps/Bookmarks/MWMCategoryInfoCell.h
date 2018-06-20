#import <UIKit/UIKit.h>

@class MWMCategoryInfoCell;

@protocol MWMCategoryInfoCellDelegate
- (void)categoryInfoCellDidPressMore:(MWMCategoryInfoCell *)cell;
@end

@interface MWMCategoryInfoCell : UITableViewCell
@property (nonatomic) BOOL expanded;
@property (weak, nonatomic) id<MWMCategoryInfoCellDelegate> delegate;

- (void)updateWithTitle:(NSString *)title
                 author:(NSString *)author
                   info:(NSString *)info
              shortInfo:(NSString *)shortInfo;
@end
