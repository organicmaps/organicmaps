#import <UIKit/UIKit.h>
#include "LocationManager.h"

@class CompassView;

@interface PlaceAndCompasView : UIView <LocationObserver>

- (void)drawView;

- (id)initWithName:(NSString *)placeName placeSecondaryName:(NSString *)placeSecondaryName placeGlobalPoint:(CGPoint)point width:(CGFloat)width;

@end
