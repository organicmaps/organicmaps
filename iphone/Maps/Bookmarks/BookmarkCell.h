#import "MWMTableViewCell.h"

@interface BookmarkCell : MWMTableViewCell

@property (nonatomic, readonly) UILabel * bmName;
@property (nonatomic, readonly) UILabel * bmDistance;

- (id)initWithReuseIdentifier:(NSString *)identifier;

@end
