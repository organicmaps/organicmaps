//
//  MWMShareLocationActivityItem.h
//  Maps
//
//  Created by Ilya Grechuhin on 05.08.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

@interface MWMShareLocationActivityItem : NSObject <UIActivityItemSource>

- (instancetype)initWithTitle:(NSString *)title location:(CLLocationCoordinate2D)location myPosition:(BOOL)myPosition;

@end
