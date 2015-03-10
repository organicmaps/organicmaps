//
//  MWMAlertViewControllerDelegate.h
//  Maps
//
//  Created by v.mikhaylenko on 07.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol MWMAlertViewControllerDelegate <NSObject>

@required
- (void)downloadMaps;
@property (nonatomic, copy) NSString *countryName;
@property (nonatomic, assign) NSUInteger size;
@end
