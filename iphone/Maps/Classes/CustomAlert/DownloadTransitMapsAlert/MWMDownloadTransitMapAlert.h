//
//  MWMGetTransitionMapAlert.h
//  Maps
//
//  Created by v.mikhaylenko on 05.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMAlert.h"

#include "../../../../../std/vector.hpp"
#include "../../../../../storage/storage.hpp"

@interface MWMDownloadTransitMapAlert : MWMAlert

+ (instancetype)alertWithCountrieIndex:(const storage::TIndex)index;

@property (nonatomic, weak, readonly) IBOutlet UILabel *titleLabel;
@property (nonatomic, weak, readonly) IBOutlet UILabel *messageLabel;
@property (nonatomic, weak, readonly) IBOutlet UIButton *notNowButton;
@property (nonatomic, weak, readonly) IBOutlet UIButton *downloadButton;
@property (nonatomic, weak, readonly) IBOutlet UILabel *sizeLabel;
@property (nonatomic, weak, readonly) IBOutlet UILabel *countryLabel;
@property (nonatomic, weak, readonly) IBOutlet UIView *specsView;

@end
