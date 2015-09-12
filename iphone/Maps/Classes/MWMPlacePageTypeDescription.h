#import <Foundation/Foundation.h>
#import "MWMPlacePageEntity.h"

@class MWMPlacePageELEDescription, MWMPlacePageHotelDescription;

@interface MWMPlacePageTypeDescription : NSObject

@property (strong, nonatomic) IBOutlet MWMPlacePageELEDescription * eleDescription;
@property (strong, nonatomic) IBOutlet MWMPlacePageHotelDescription * hotelDescription;

- (instancetype)initWithPlacePageEntity:(MWMPlacePageEntity *)entity;

@end
