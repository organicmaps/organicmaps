#import <UIKit/UIKit.h>

@interface SearchCell : UITableViewCell
{
  UILabel * featureName;
  UILabel * featureType;
  UILabel * featureCountry;
  UILabel * featureDistance;
}

@property (nonatomic, retain) IBOutlet UILabel * featureName;
@property (nonatomic, retain) IBOutlet UILabel * featureType;
@property (nonatomic, retain) IBOutlet UILabel * featureCountry;
@property (nonatomic, retain) IBOutlet UILabel * featureDistance;

- (id)initWithReuseIdentifier:(NSString *)identifier;

@end
