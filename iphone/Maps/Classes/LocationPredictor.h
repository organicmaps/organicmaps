#pragma once

#import "../Platform/LocationManager.h"

#import <Foundation/Foundation.h>

@interface LocationPredictor : NSObject

-(id)initWithObserver:(NSObject<LocationObserver> *) observer;
-(void)reset:(location::GpsInfo const &) info;
-(void)setMode:(location::EMyPositionMode) mode;

@end
