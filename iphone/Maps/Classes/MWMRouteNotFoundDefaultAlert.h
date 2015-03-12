//
//  MWMRouteNotFoundDefaultAlert.h
//  Maps
//
//  Created by v.mikhaylenko on 12.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMAlert.h"

@interface MWMRouteNotFoundDefaultAlert : MWMAlert

+ (instancetype)alert;

@property (nonatomic, weak, readonly) IBOutlet UILabel *messageLabel;
@property (nonatomic, weak, readonly) IBOutlet UIButton *okButton;
@property (nonatomic, weak, readonly) IBOutlet UIView *deviderLine;

@end
