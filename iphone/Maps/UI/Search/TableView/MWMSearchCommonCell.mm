#import "MWMSearchCommonCell.h"
#import "CLLocation+Mercator.h"
#import "MWMLocationManager.h"
#import "SwiftBridge.h"
#import "SearchResult.h"

#include "map/place_page_info.hpp"

#include "geometry/mercator.hpp"

#include "platform/localization.hpp"
#include "platform/distance.hpp"

@interface MWMSearchCommonCell ()

@property(weak, nonatomic) IBOutlet UILabel * distanceLabel;
@property(weak, nonatomic) IBOutlet UILabel * infoLabel;
@property(weak, nonatomic) IBOutlet UILabel * locationLabel;
@property(weak, nonatomic) IBOutlet UILabel * openLabel;
@property(weak, nonatomic) IBOutlet UIView * popularView;

@end

@implementation MWMSearchCommonCell

- (void)configureWith:(SearchResult * _Nonnull)result {
  [super configureWith:result];
  self.locationLabel.text = result.addressText;
  [self.locationLabel sizeToFit];

  self.infoLabel.text = result.infoText;
  self.distanceLabel.text = result.distanceText;
  self.popularView.hidden = YES;
  self.openLabel.text = result.openStatusText;
  self.openLabel.textColor = result.openStatusColor;
  [self.openLabel setHidden:result.openStatusText.length == 0];
  [self setStyleNameAndApply: @"Background"];
}

- (NSDictionary *)selectedTitleAttributes
{
  return @{
    NSForegroundColorAttributeName : [UIColor blackPrimaryText],
    NSFontAttributeName : [UIFont bold17]
  };
}

- (NSDictionary *)unselectedTitleAttributes
{
  return @{
    NSForegroundColorAttributeName : [UIColor blackPrimaryText],
    NSFontAttributeName : [UIFont regular17]
  };
}

@end
