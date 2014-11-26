//
//  LocalNotificationInfoProvider.h
//  Maps
//
//  Created by Timur Bernikowich on 25/11/2014.
//  Copyright (c) 2014 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface LocalNotificationInfoProvider : NSObject <UIActivityItemSource>

- (instancetype)initWithDictionary:(NSDictionary *)info;

@property (nonatomic) NSDictionary * info;

@end