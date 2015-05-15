//
//  MWMCategoriesInterfaceCell.h
//  Maps
//
//  Created by v.mikhaylenko on 06.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>

@class WKInterfaceLabel, WKInterfaceImage;

@interface MWMCategoriesInterfaceCell : NSObject

@property (weak, nonatomic, readonly) IBOutlet WKInterfaceImage * icon;
@property (weak, nonatomic, readonly) IBOutlet WKInterfaceLabel * label;

@end
