//
//  MWMGetTransitionMapAlert.h
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMAlert.h"

#include "storage/storage.hpp"
#include "std/vector.hpp"

@interface MWMDownloadTransitMapAlert : MWMAlert

+ (instancetype)downloaderAlertWithMaps:(vector<storage::TIndex> const &)maps
                                 routes:(vector<storage::TIndex> const &)routes
                                   code:(routing::IRouter::ResultCode)code;
- (void)showDownloadDetail:(UIButton *)sender;

@end
