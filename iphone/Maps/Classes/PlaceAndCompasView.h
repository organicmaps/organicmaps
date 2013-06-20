//
//  PlaceAndCompasView.h
//  Maps
//
//  Created by Kirill on 22/06/2013.
//  Copyright (c) 2013 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>
#include "LocationManager.h"

@class CompassView;

@interface PlaceAndCompasView : UIView <LocationObserver>

-(void)drawView;

- (id)initWithName:(NSString *)placeName placeSecondaryName:(NSString *)placeSecondaryName placeGlobalPoint:(CGPoint)point width:(CGFloat)width;

@end
