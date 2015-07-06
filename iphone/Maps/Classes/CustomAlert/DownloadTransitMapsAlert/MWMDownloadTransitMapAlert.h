//
//  MWMGetTransitionMapAlert.h
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMAlert.h"

#include "std/vector.hpp"
#include "storage/storage.hpp"

@interface MWMDownloadTransitMapAlert : MWMAlert

+ (instancetype)alertWithCountryIndex:(const storage::TIndex)index;
+ (instancetype)alertWithMaps:(vector<storage::TIndex> const &)maps routes:(vector<storage::TIndex> const &)routes;
- (void)showDownloadDetail:(UIButton *)sender;

@end
