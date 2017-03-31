#import "MWMCoreBanner.h"

@interface MWMCoreBanner ()

@property(copy, nonatomic, readwrite) NSString * bannerID;

@end

@implementation MWMCoreBanner

- (instancetype)initWith:(MWMBannerType)type bannerID:(NSString *)bannerID
{
  self = [super init];
  if (self)
  {
    _mwmType = type;
    _bannerID = bannerID;
  }
  return self;
}

@end
