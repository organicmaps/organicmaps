//
//  MWMWatchEventInfo.h
//  Maps
//
//  Created by i.grechuhin on 10.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSUInteger, MWMWatchEventInfoRequest)
{
  MWMWatchEventInfoRequestMoveWritableDir
};

@interface NSDictionary (MWMWatchEventInfo)

@property (nonatomic, readonly) MWMWatchEventInfoRequest watchEventInfoRequest;

@end

@interface NSMutableDictionary (MWMWatchEventInfo)

@property (nonatomic) MWMWatchEventInfoRequest watchEventInfoRequest;

@end
