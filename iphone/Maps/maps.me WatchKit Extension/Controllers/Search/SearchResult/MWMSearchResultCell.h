//
//  MWMSearchResultCell.h
//  Maps
//
//  Created by v.mikhaylenko on 08.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>

@class WKInterfaceLabel;

@interface MWMSearchResultCell : NSObject

@property (weak, nonatomic, readonly) IBOutlet WKInterfaceLabel * titleLabel;
@property (weak, nonatomic, readonly) IBOutlet WKInterfaceLabel * categoryLabel;
@property (weak, nonatomic, readonly) IBOutlet WKInterfaceLabel * distanceLabel;

@end
