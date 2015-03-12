//
//  MWMMapsDownloaderDelegate.h
//  Maps
//
//  Created by v.mikhaylenko on 07.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MWMAlertViewControllerDelegate.h"

#include "../../../std/vector.hpp"
#include "../../../map/country_status_display.hpp"


@interface MWMAlertDownloaderManager : NSObject <MWMAlertViewControllerDelegate>

- (instancetype)initWithMapsIndexes:(const vector<storage::TIndex>&)indexes;

@property (nonatomic, copy) NSString *countryName;
@property (nonatomic, assign) NSUInteger size;

@end
