#import "MWMTaxiPreviewCell.h"
#import "TimeUtils.h"

#include "partners_api/uber_api.hpp"

#include "base/string_utils.hpp"

@interface MWMTaxiPreviewCell ()

@property(weak, nonatomic) IBOutlet UIImageView * icon;
@property(weak, nonatomic) IBOutlet UILabel * product;
@property(weak, nonatomic) IBOutlet UILabel * waitingTime;
@property(weak, nonatomic) IBOutlet UILabel * price;

@end

@implementation MWMTaxiPreviewCell

- (void)configWithProduct:(uber::Product const &)product;
{
  self.product.text = @(product.m_name.c_str());
  NSTimeInterval time;
  if (!strings::to_double(product.m_time, time))
    NSAssert(false, @"Incorrect time");

  NSString * formatted = [NSDateFormatter estimatedArrivalTimeWithSeconds:time];
  NSString * pattern = [L(@"taxi_wait") stringByReplacingOccurrencesOfString:@"%s" withString:@"%@"];
  self.waitingTime.text = [NSString stringWithFormat:pattern, formatted];
  self.price.text = @(product.m_price.c_str());
}

@end
