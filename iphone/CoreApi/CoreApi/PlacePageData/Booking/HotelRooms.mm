#import "HotelRooms+Core.h"
#import "HotelRoom+Core.h"

@implementation HotelRooms

@end

@implementation HotelRooms (Core)

- (instancetype)initWithBlocks:(booking::Blocks const &)blocks {
  self = [super init];
  if (self) {
    _minPrice = blocks.m_totalMinPrice;
    _currency = @(blocks.m_currency.c_str());
    _discount = blocks.m_maxDiscount;
    _isSmartDeal = blocks.m_hasSmartDeal;
    NSMutableArray *roomsArray = [NSMutableArray arrayWithCapacity:blocks.m_blocks.size()];
    for (auto const &block : blocks.m_blocks) {
      HotelRoom *room = [[HotelRoom alloc] initWithBlockInfo:block];
      [roomsArray addObject:room];
    }
    _rooms = roomsArray;
  }
  return self;
}

@end
