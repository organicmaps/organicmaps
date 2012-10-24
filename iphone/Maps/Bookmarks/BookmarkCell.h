#import <UIKit/UIKit.h>


@interface BookmarkCell : UITableViewCell

@property (nonatomic, readonly) UILabel * bmName;
@property (nonatomic, readonly) UILabel * bmDistance;

- (id)initWithReuseIdentifier:(NSString *)identifier;

@end
