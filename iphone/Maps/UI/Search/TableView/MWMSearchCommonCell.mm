#import "MWMSearchCommonCell.h"
#import "CLLocation+Mercator.h"
#import "MWMLocationManager.h"
#import "SearchResult.h"
#import "SwiftBridge.h"

@interface MWMSearchCommonCell ()

@property(weak, nonatomic) IBOutlet UILabel * distanceLabel;
@property(weak, nonatomic) IBOutlet UILabel * infoLabel;
@property(weak, nonatomic) IBOutlet UILabel * locationLabel;
@property(weak, nonatomic) IBOutlet UILabel * openLabel;
@property(weak, nonatomic) IBOutlet UIView * popularView;
@property(weak, nonatomic) IBOutlet UIImageView * iconImageView;

@end

@implementation MWMSearchCommonCell

- (void)configureWith:(SearchResult * _Nonnull)result isPartialMatching:(BOOL)isPartialMatching
{
  [super configureWith:result isPartialMatching:isPartialMatching];
  self.locationLabel.text = result.addressText;
  [self.locationLabel sizeToFit];
  self.infoLabel.text = result.infoText;
  self.distanceLabel.text = result.distanceText;
  self.popularView.hidden = YES;
  self.openLabel.text = result.openStatusText;
  self.openLabel.textColor = result.openStatusColor;
  [self.openLabel setHidden:result.openStatusText.length == 0];
  [self setStyleNameAndApply:@"Background"];
  [self.iconImageView setStyleNameAndApply:@"BlueBackground"];
  self.iconImageView.image = [UIImage imageNamed:result.iconImageName];
  self.separatorInset = UIEdgeInsetsMake(0, kSearchCellSeparatorInset, 0, 0);
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  [self.iconImageView.layer setCornerRadius:self.iconImageView.height / 2];
}

- (NSDictionary *)selectedTitleAttributes
{
  return @{NSForegroundColorAttributeName: [UIColor blackPrimaryText], NSFontAttributeName: [UIFont bold17]};
}

- (NSDictionary *)unselectedTitleAttributes
{
  return @{NSForegroundColorAttributeName: [UIColor blackPrimaryText], NSFontAttributeName: [UIFont regular17]};
}

@end
