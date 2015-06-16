//
//  MWMPlacePageTypeDescription.h
//  Maps
//
//  Created by v.mikhaylenko on 07.06.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MWMPlacePageEntity.h"

@class MWMPlacePageELEDescription, MWMPlacePageHotelDescription;

@interface MWMPlacePageTypeDescription : NSObject

@property (strong, nonatomic) IBOutlet MWMPlacePageELEDescription * eleDescription;
@property (strong, nonatomic) IBOutlet MWMPlacePageHotelDescription * hotelDescription;

- (instancetype)initWithPlacePageEntity:(MWMPlacePageEntity *)entity;

@end
