//
//  MWMAlertEntity.h
//  Maps
//
//  Created by v.mikhaylenko on 06.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface MWMAlertEntity : NSObject

@property (nonatomic, copy) NSString *title;
@property (nonatomic, copy) NSString *message;
@property (nonatomic, copy) NSString *country;
@property (nonatomic, copy) NSString *location;
@property (nonatomic, assign) NSUInteger size;

@end
