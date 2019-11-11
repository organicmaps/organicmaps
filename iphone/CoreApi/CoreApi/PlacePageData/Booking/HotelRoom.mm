#import "HotelRoom+Core.h"

static HotelRoomDealType convertDeal(booking::Deals::Type const &dealType) {
  switch (dealType) {
    case booking::Deals::Type::Smart:
      return HotelRoomDealTypeSmart;
    case booking::Deals::Type::LastMinute:
      return HotelRoomDealTypeLastMinute;
  }
}

@implementation HotelRoomDeal

- (instancetype)initWithDeal:(booking::Deals const &)deal {
  if (deal.m_discount == 0 || deal.m_types.empty()) {
    return nil;
  }
  
  self = [super init];
  if (self) {
    _discount = deal.m_discount;
    NSMutableArray *typesArray = [NSMutableArray arrayWithCapacity:deal.m_types.size()];
    for (auto const &type : deal.m_types) {
      NSNumber *n = [NSNumber numberWithInteger:convertDeal(type)];
      [typesArray addObject:n];
    }
    _types = [typesArray copy];
  }
  return self;
}

@end

@implementation HotelRoom

@end

@implementation HotelRoom (Core)

- (instancetype)initWithBlockInfo:(booking::BlockInfo const &)blockInfo {
  self = [super init];
  if (self) {
    _offerId = @(blockInfo.m_blockId.c_str());
    _name = @(blockInfo.m_name.c_str());
    _roomDescription = @(blockInfo.m_description.c_str());
    _maxOccupancy = blockInfo.m_maxOccupancy;
    _price = blockInfo.m_minPrice;
    _currency = @(blockInfo.m_currency.c_str());
    _isBreakfastIncluded = blockInfo.m_breakfastIncluded;
    _isDepositRequired = blockInfo.m_depositRequired;
    _discount = blockInfo.m_deals.m_discount;
    _deal = [[HotelRoomDeal alloc] initWithDeal:blockInfo.m_deals];
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(blockInfo.m_refundableUntil.time_since_epoch()).count();
    _refundableUntil = seconds > 0 ? [NSDate dateWithTimeIntervalSince1970:seconds] : nil;
    NSMutableArray *photosArray = [NSMutableArray arrayWithCapacity:blockInfo.m_photos.size()];
    for (auto const &photo : blockInfo.m_photos) {
      [photosArray addObject:@(photo.c_str())];
    }
    _photos = [photosArray copy];
  }
  return self;
}

@end
