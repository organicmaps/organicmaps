#import "MWMTableViewCell.h"

@protocol MWMBookmarkTitleDelegate <NSObject>

- (void)didFinishEditingBookmarkTitle:(NSString *)title;

@end

@interface MWMBookmarkTitleCell : MWMTableViewCell

- (void)configureWithName:(NSString *)name delegate:(id<MWMBookmarkTitleDelegate>)delegate;

@end
