//
//  MWMWKInterfaceController.h
//  Maps
//
//  Created by Ilya Grechuhin on 22.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMWatchLocationTrackerDelegate.h"

#import <WatchKit/WatchKit.h>
#import <Foundation/Foundation.h>

@interface MWMWKInterfaceController : WKInterfaceController <MWMWatchLocationTrackerDelegate>

@property (nonatomic, readonly) BOOL haveLocation;

- (void)checkLocationService;

@end
