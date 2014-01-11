#import <UIKit/UIKit.h>

@interface SearchCell : UITableViewCell

@property (nonatomic, readonly) UILabel * featureName;
@property (nonatomic, readonly) UILabel * featureType;
@property (nonatomic, readonly) UILabel * featureCountry;
@property (nonatomic, readonly) UILabel * featureDistance;

- (id)initWithReuseIdentifier:(NSString *)identifier;

@end
